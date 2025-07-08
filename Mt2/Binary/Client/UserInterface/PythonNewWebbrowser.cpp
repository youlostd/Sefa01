#include "StdAfx.h"

#ifdef ENABLE_NEW_WEBBROWSER
#include "PythonNewWebbrowser.h"
#include <Awesomium\STLHelpers.h>
#ifdef WEBBROWSER_OFFSCREEN
#include <Awesomium\BitmapSurface.h>
#endif
#include "PythonApplication.h"
#include "Test.h"
#include <ShellAPI.h>
#ifdef ENABLE_VOTEBOT
#include <thread>
#include "PythonWebVotebot.h"
#include "../VotebotM2G/VotebotM2G/Config.h"
#endif

//#define DEBUG

#define DEFAULT_WEB_WIDTH 100
#define DEFAULT_WEB_HEIGHT 100

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CWebViewListener - SUB CLASS of CPythonNewWeb
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

void CPythonNewWeb::CWebViewListener::OnAddConsoleMessage(Awesomium::WebView* caller,
	const Awesomium::WebString& message,
	int line_number,
	const Awesomium::WebString& source)
{
	TraceError("[WebConsole] %d: %ws (src %ws)", line_number, message.data(), source.data());
}

void CPythonNewWeb::CWebViewListener::OnChangeAddressBar(Awesomium::WebView* caller,
	const Awesomium::WebURL& url)
{
	char szBuf[512];
	snprintf(szBuf, sizeof(szBuf), "%ws", url.spec().data());

	CPythonNewWeb::Instance().CallEventHandle("WebOnChangeAdressBar", Py_BuildValue("(s)", szBuf));
}

void CPythonNewWeb::CWebViewListener::OnChangeTargetURL(Awesomium::WebView* caller,
	const Awesomium::WebURL& url)
{
	char szBuf[512];
	snprintf(szBuf, sizeof(szBuf), "%ws", url.spec().data());

	CPythonNewWeb::Instance().CallEventHandle("WebOnChangeTargetURL", Py_BuildValue("(s)", szBuf));
}

void CPythonNewWeb::CWebViewListener::OnShowCreatedWebView(Awesomium::WebView* caller,
	Awesomium::WebView* new_view,
	const Awesomium::WebURL& opener_url,
	const Awesomium::WebURL& target_url,
	const Awesomium::Rect& initial_pos,
	bool is_popup)
{
	new_view->Destroy();
	ShellExecuteW(NULL, NULL, (LPCWSTR)target_url.spec().data(), NULL, NULL, SW_SHOW);
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CWebLoadListener - SUB CLASS of CPythonNewWeb
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

void CPythonNewWeb::CWebLoadListener::OnDocumentReady(Awesomium::WebView* caller,
	const Awesomium::WebURL& url)
{
	char szBuf[512];
	snprintf(szBuf, sizeof(szBuf), "%ws", url.spec().data());
	// TraceError("%s", szBuf);
	CPythonNewWeb::Instance().CallEventHandle("WebOnDocumentReady", Py_BuildValue("(s)", szBuf));
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CInputMethodListener - SUB CLASS of CPythonNewWeb
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

#ifdef WEBBROWSER_OFFSCREEN
void CPythonNewWeb::CInputMethodListener::OnUpdateIME(Awesomium::WebView *caller, Awesomium::TextInputType type, int caret_x, int caret_y)
{
	CPythonNewWeb::Instance().m_bCanHandleIME = type != Awesomium::kTextInputType_None;
	/*char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "OnUpdateIME Type %d caret_x %d caret_y %d canHandle %d", type, caret_x, caret_y, CPythonNewWeb::Instance().m_bCanHandleIME);
	CPythonChat::Instance().AppendChat(CHAT_TYPE_INFO, szBuf);*/
}

void CPythonNewWeb::CInputMethodListener::OnCancelIME(Awesomium::WebView *caller)
{
	CPythonNewWeb::Instance().m_bCanHandleIME = false;
	/*char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "OnCancelIME");
	CPythonChat::Instance().AppendChat(CHAT_TYPE_INFO, szBuf);*/
}

void CPythonNewWeb::CInputMethodListener::OnChangeIMERange(Awesomium::WebView *caller, unsigned int start, unsigned int end)
{
	CPythonNewWeb::Instance().m_bCanHandleIME = false;
	/*char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "OnChangeIMERange start %d end %d", start, end);
	CPythonChat::Instance().AppendChat(CHAT_TYPE_INFO, szBuf);*/
}
#endif

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPythonNewWeb - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPythonNewWeb::CPythonNewWeb()
{
	m_pWebCore = NULL;
	m_pCurrentWebView = NULL;
	m_bIsShow = false;
	m_bIsLoading = false;
	m_poLoadFinishHandle = NULL;
	m_poCurrentEventHandle = NULL;

#ifdef WEBBROWSER_OFFSCREEN
	m_pRenderTexture = NULL;
	m_bCanHandleIME = false;
	m_bIsFocused = false;
#endif

	m_kWebPreferences.enable_web_security = false;
	m_kWebPreferences.enable_gpu_acceleration = true;
	m_kWebPreferences.enable_web_gl = true;
	m_kWebPreferences.allow_universal_access_from_file_url = true;
	m_kWebPreferences.allow_file_access_from_file_url = true;

	m_kWebPreferences_NoJS = m_kWebPreferences;
	m_kWebPreferences_NoJS.enable_javascript = false;

#ifdef ENABLE_VOTEBOT
	m_pVotebot = NULL;
	m_pVotebotThread = NULL;
#endif
}

CPythonNewWeb::~CPythonNewWeb()
{
	Destroy();
}

void CPythonNewWeb::Initialize()
{
	Destroy();

	Awesomium::WebConfig kConfig;
#ifdef ENABLE_VOTEBOT
	const char* c_pszUserAgentList[] = { HTTP_USERAGENT1, HTTP_USERAGENT2, HTTP_USERAGENT3, HTTP_USERAGENT4, HTTP_USERAGENT5 };
	const int c_iUserAgentCount = sizeof(c_pszUserAgentList) / sizeof(c_pszUserAgentList[0]);
	kConfig.user_agent = Awesomium::WSLit(c_pszUserAgentList[random_range(0, c_iUserAgentCount - 1)]);
#else
	kConfig.user_agent = Awesomium::WSLit("Mozilla/5.0 (Windows NT 6.1; rv:52.0) Gecko/20100101 Firefox/52.0");
#endif
	m_kWebPreferences.accept_language = Awesomium::WSLit(CLocaleManager::instance().GetLanguageShortName());

	m_pWebCore = Awesomium::WebCore::Initialize(kConfig);
	m_pWebSession = m_pWebCore->CreateWebSession(Awesomium::WSLit("temp\\websession"), m_kWebPreferences);
	m_pWebSession_NoJS = m_pWebCore->CreateWebSession(Awesomium::WSLit("temp\\websession"), m_kWebPreferences_NoJS);

#ifdef WEBBROWSER_OFFSCREEN
	m_pRenderTexture = NULL;
	m_bCanHandleIME = false;
	m_bIsFocused = false;
#endif

#ifdef ENABLE_VOTEBOT
	m_pVotebot = new CPythonWebVotebot(m_pWebCore);
#endif
}

Awesomium::WebView* CPythonNewWeb::__CreateWebView(int iWidth, int iHeight, bool bEnableJS)
{
#ifdef WEBBROWSER_OFFSCREEN
	Awesomium::WebView* pWebView = m_pWebCore->CreateWebView(iWidth, iHeight, bEnableJS ? m_pWebSession : m_pWebSession_NoJS, Awesomium::kWebViewType_Offscreen);
	pWebView->ActivateIME(true);
#else
	Awesomium::WebView* pWebView = m_pWebCore->CreateWebView(iWidth, iHeight, bEnableJS ? m_pWebSession : m_pWebSession_NoJS, Awesomium::kWebViewType_Window);
	pWebView->set_parent_window(CPythonApplication::Instance().GetWindowHandle());

	ShowWindow(pWebView->window(), SW_HIDE);
#endif

	return pWebView;
}

CPythonNewWeb::TWebViewData* CPythonNewWeb::__GetWebViewData(const char* c_pszWebType, const char* c_pszWebPage, bool* pbIsPreloaded, int iWidth, int iHeight)
{
	if (!c_pszWebType || !*c_pszWebType)
		c_pszWebType = "none";

	auto it = m_map_pWebView.find(c_pszWebType);
	if (it != m_map_pWebView.end())
	{
		if (pbIsPreloaded)
			*pbIsPreloaded = c_pszWebPage ? it->second.m_stLastURL == c_pszWebPage : !it->second.m_stLastURL.empty();

		if (c_pszWebPage)
			it->second.m_stLastURL = c_pszWebPage;
		return &it->second;
	}

	if (pbIsPreloaded)
		*pbIsPreloaded = false;

	if (iWidth == 0 || iHeight == 0)
	{
		iWidth = DEFAULT_WEB_WIDTH;
		iHeight = DEFAULT_WEB_HEIGHT;
	}

	Awesomium::WebView* pWebView = __CreateWebView(iWidth, iHeight, true);//strcmp(c_pszWebType, "m2pserverinfo"));
	
	TWebViewData& rkResult = m_map_pWebView[c_pszWebType];
	pWebView->set_view_listener(&rkResult.kWebViewListener);
	pWebView->set_load_listener(&rkResult.kWebLoadListener);
#ifdef WEBBROWSER_OFFSCREEN
	pWebView->set_input_method_editor_listener(&rkResult.kInputMethodListener);
#endif
	rkResult.pWebView = pWebView;
	if (c_pszWebPage)
		rkResult.m_stLastURL = c_pszWebPage;
	return &rkResult;
}

Awesomium::WebView* CPythonNewWeb::__GetWebView(const char* c_pszWebType, const char* c_pszWebPage, bool* pbIsPreloaded, int iWidth, int iHeight)
{
	TWebViewData* pData = __GetWebViewData(c_pszWebType, c_pszWebPage, pbIsPreloaded, iWidth, iHeight);
	return pData->pWebView;
}

#ifdef WEBBROWSER_OFFSCREEN
void CPythonNewWeb::__CreateRenderTexture(int iWidth, int iHeight)
{
	__DestroyRenderTexture();

	m_pRenderTexture = new CGraphicImageTexture;
	m_pRenderTexture->Create(iWidth, iHeight, D3DFMT_A8R8G8B8);
}

void CPythonNewWeb::__DestroyRenderTexture()
{
	if (m_pRenderTexture)
	{
		delete m_pRenderTexture;
		m_pRenderTexture = NULL;
	}
}
#endif

/*******************************************************************\
| [PUBLIC] General Functions
\*******************************************************************/

void CPythonNewWeb::SetLoadFinishEvent(PyObject* poHandle, const std::string& c_rstFuncName)
{
	m_poLoadFinishHandle = poHandle;
	m_stLoadFinishFunc = c_rstFuncName;

	if (m_bIsShow && !m_bIsLoading)
	{
		ExecLoadFinishEvent();
	}
}

void CPythonNewWeb::ExecLoadFinishEvent()
{
	if (m_poLoadFinishHandle)
	{
		PyCallClassMemberFunc(m_poLoadFinishHandle, m_stLoadFinishFunc.c_str(), Py_BuildValue("()"));

		m_poLoadFinishHandle = NULL;
		m_stLoadFinishFunc.clear();
	}
}

void CPythonNewWeb::SetCurrentEventHandle(PyObject* poHandle)
{
	m_poCurrentEventHandle = poHandle;
}

void CPythonNewWeb::CallEventHandle(const std::string& c_rstEventName, PyObject* poArgs)
{
	if (m_poCurrentEventHandle)
		PyCallClassMemberFunc(m_poCurrentEventHandle, c_rstEventName.c_str(), poArgs ? poArgs : Py_BuildValue("()"));
}

void CPythonNewWeb::Create(const char* c_pszTypeName, int iWidth, int iHeight)
{
	__GetWebView(c_pszTypeName, NULL, NULL, iWidth, iHeight);
}

void CPythonNewWeb::PreLoad(const char* c_pszTypeName, const char* c_pszWebPage, int iWidth, int iHeight)
{
	bool bIsLoaded;
	Awesomium::WebView* pWebView = __GetWebView(c_pszTypeName, c_pszWebPage, &bIsLoaded, iWidth, iHeight);

	if (!bIsLoaded)
		pWebView->LoadURL(Awesomium::WebURL(Awesomium::WSLit(c_pszWebPage)));
	else
		pWebView->Reload(true);
}

void CPythonNewWeb::Show(const char* c_pszWebPage, const RECT& c_rkRect, const char* c_pszWebTypeName)
{
	if (m_bIsShow)
		return;

	m_bIsShow = true;
	m_bIsLoading = true;

	bool bIsLoaded;
	TWebViewData* pViewData = __GetWebViewData((c_pszWebTypeName && *c_pszWebTypeName) ? c_pszWebTypeName : c_pszWebPage, c_pszWebPage, &bIsLoaded);

	m_pCurrentWebView = pViewData->pWebView;

	m_kRect = c_rkRect;
	m_pCurrentWebView->Resize(c_rkRect.right - c_rkRect.left, c_rkRect.bottom - c_rkRect.top);
	__CreateRenderTexture(c_rkRect.right - c_rkRect.left, c_rkRect.bottom - c_rkRect.top);

	if (!bIsLoaded || stricmp(c_pszWebPage, GetWindowURL()))
	{
		m_pCurrentWebView->LoadURL(Awesomium::WebURL(Awesomium::WSLit(c_pszWebPage)));
	}
	else
	{
		if (__IS_TEST_SERVER_MODE__)
			m_pCurrentWebView->Reload(true);
		else
			m_pCurrentWebView->Reload(false);

		if (!m_pCurrentWebView->IsLoading())
		{
			m_bIsLoading = false;
			OnShow();
		}
	}
}

void CPythonNewWeb::OnShow()
{
#ifndef WEBBROWSER_OFFSCREEN
	m_pCurrentWebView->ResumeRendering();
#endif
	ExecLoadFinishEvent();

#ifndef WEBBROWSER_OFFSCREEN
	MoveWindow(m_pCurrentWebView->window(), m_kRect.left, m_kRect.top, m_kRect.right - m_kRect.left, m_kRect.bottom - m_kRect.top, true);
	ShowWindow(m_pCurrentWebView->window(), SW_SHOW);
	CPythonApplication::Instance().OnShowWebPage(m_kRect);
#else
	m_bIsFocused = true;
#endif
}

void CPythonNewWeb::Move(const RECT& c_rkRect)
{
	if (!m_bIsShow)
		return;

#ifndef WEBBROWSER_OFFSCREEN
	MoveWindow(m_pCurrentWebView->window(), c_rkRect.left, c_rkRect.top, c_rkRect.right - c_rkRect.left, c_rkRect.bottom - c_rkRect.top, true);
	CPythonApplication::Instance().OnMoveWebPage(m_kRect);
#endif
	m_kRect = c_rkRect;
}

#ifndef WEBBROWSER_OFFSCREEN
BOOL CALLBACK ActivateVisibleWindow(HWND hwnd, LPARAM lParam)
{
	const DWORD TITLE_SIZE = 1024;
	TCHAR windowTitle[TITLE_SIZE];

	if (hwnd != (HWND)lParam)
	{
		if (GetWindowText(hwnd, windowTitle, TITLE_SIZE))
		{
			if (IsWindowVisible(hwnd))
			{
				SetActiveWindow(hwnd);
				return false;
			}
		}
	}
	return true; // Need to continue enumerating windows
}
#endif

void CPythonNewWeb::ExecuteJavascript(const std::string& c_rstJavascriptCode)
{
	if (!m_bIsShow)
		return;

	m_pCurrentWebView->ExecuteJavascript(Awesomium::WSLit(c_rstJavascriptCode.c_str()), Awesomium::WSLit(""));
}

void CPythonNewWeb::WindowResize(int iNewWidth, int iNewHeight)
{
	if (!m_bIsShow)
		return;

	m_pCurrentWebView->Resize(iNewWidth, iNewHeight);
}

const char* CPythonNewWeb::GetWindowURL()
{
	if (!m_bIsShow)
		return "";

	static char szWindowURL[1024];
	snprintf(szWindowURL, sizeof(szWindowURL), "%ws", m_pCurrentWebView->url().spec().data());

	return szWindowURL;
}

void CPythonNewWeb::GetWindowSize(int& iRetWidth, int& iRetHeight)
{
	Awesomium::JSValue clientWidth = m_pCurrentWebView->ExecuteJavascriptWithResult(Awesomium::WSLit("document.body.clientWidth"), Awesomium::WSLit(""));
	Awesomium::JSValue clientHeight = m_pCurrentWebView->ExecuteJavascriptWithResult(Awesomium::WSLit("document.body.clientHeight"), Awesomium::WSLit(""));

	iRetWidth = clientWidth.ToInteger();
	iRetHeight = clientHeight.ToInteger();
}

void CPythonNewWeb::Hide(bool bForce)
{
	if (!m_bIsShow && !bForce)
		return;

	m_poCurrentEventHandle = NULL;
	
	if (m_bIsShow)
	{
		m_bIsShow = false;

#ifdef WEBBROWSER_OFFSCREEN
		m_bCanHandleIME = false;
		__DestroyRenderTexture();
#else
		CPythonApplication::Instance().OnHideWebPage();
		m_pCurrentWebView->PauseRendering();
		ShowWindow(m_pCurrentWebView->window(), SW_HIDE);
#endif
	}

#ifndef WEBBROWSER_OFFSCREEN
	if (!CPythonApplication::Instance().IsMaximizedWindow())
	{
		ShowWindow(CPythonApplication::Instance().GetWindowHandle(), SW_HIDE);
		ShowWindow(CPythonApplication::Instance().GetWindowHandle(), SW_SHOW);
	}
	else
	{
		ShowWindow(CPythonApplication::Instance().GetWindowHandle(), SW_HIDE);
		ShowWindow(CPythonApplication::Instance().GetWindowHandle(), SW_SHOWMAXIMIZED);
		SendMessage(CPythonApplication::Instance().GetWindowHandle(), WM_ACTIVATEAPP, WA_ACTIVE, 0);
	}
#endif

	//EnumWindows(ActivateVisibleWindow, (LPARAM)CPythonApplication::Instance().GetWindowHandle());

	m_pCurrentWebView = NULL;
}

void CPythonNewWeb::Destroy()
{
	m_map_pWebView.clear();

#ifdef ENABLE_VOTEBOT
	if (m_pVotebot)
	{
		delete m_pVotebot;
		m_pVotebot = NULL;
	}
#endif

	if (m_pWebCore)
	{
		if (m_pWebSession)
		{
			m_pWebSession->Release();
			m_pWebSession = NULL;
		}

		Awesomium::WebCore::Shutdown();
		m_pWebCore = NULL;
		m_pCurrentWebView = NULL;
	}
}

void CPythonNewWeb::Update()
{
#ifdef ENABLE_VOTEBOT
	if (m_pVotebot)
		m_pVotebot->Update();
#endif

	if (!m_bIsShow)
	{
		bool bAnyLoading = false;
		for (auto it = m_map_pWebView.begin(); it != m_map_pWebView.end(); ++it)
		{
			if (it->second.pWebView->IsLoading())
			{
				bAnyLoading = true;
				break;
			}
		}

#ifdef ENABLE_VOTEBOT
		if (m_pWebCore && m_pVotebot && m_pVotebot->IsVoting())
			m_pWebCore->Update();
#endif

		if (!bAnyLoading)
			return;
	}

	if (m_pWebCore)
		m_pWebCore->Update();

	if (m_bIsShow && m_bIsLoading)
	{
		if (!m_pCurrentWebView->IsLoading())
		{
			m_bIsLoading = false;
			OnShow();
		}
	}

#ifdef WEBBROWSER_OFFSCREEN
	if (m_bIsShow && !m_bIsLoading)
	{
		Awesomium::BitmapSurface* pSurface = static_cast<Awesomium::BitmapSurface*> (m_pCurrentWebView->surface());

		if (!m_pRenderTexture)
			return;

		DWORD* pdwDst;
		int pitch;

		if (!m_pRenderTexture->Lock(&pitch, (void**)&pdwDst))
			return;

		pitch /= sizeof(DWORD);

		int width = pSurface->width();
		int height = pSurface->height();

		DWORD * pdwSrc = (DWORD*)pSurface->buffer();

		for (int y = 0; y < height; ++y, pdwDst += pitch, pdwSrc += width)
			for (int x = 0; x < width; ++x)
			{
				m_argbConverterUnion.color = pdwSrc[x];
				// swap colors due to different color format
				std::swap(m_argbConverterUnion.a, m_argbConverterUnion.b);
				std::swap(m_argbConverterUnion.r, m_argbConverterUnion.g);
				// swap end
				pdwDst[x] = m_argbConverterUnion.color;
			}

		m_pRenderTexture->Unlock();
	}
#endif
}

#ifdef WEBBROWSER_OFFSCREEN
void CPythonNewWeb::Render()
{
	if (!m_bIsShow || m_bIsLoading)
		return;

	static const D3DXCOLOR diffuseColor = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);

	float fimgWidth = m_kRect.right - m_kRect.left;
	float fimgHeight = m_kRect.bottom - m_kRect.top;

	float texReverseWidth = 1.0f / float(m_pRenderTexture->GetWidth());
	float texReverseHeight = 1.0f / float(m_pRenderTexture->GetHeight());
	float su = 0.0f * texReverseWidth;
	float sv = 0.0f * texReverseHeight;
	float eu = fimgWidth * texReverseWidth;
	float ev = fimgHeight * texReverseHeight;

	TPDTVertex vertices[4];
	vertices[0].position.x = m_kRect.left - 0.5f;
	vertices[0].position.y = m_kRect.top - 0.5f;
	vertices[0].position.z = 0.0f;
	vertices[0].texCoord = TTextureCoordinate(su, sv);
	vertices[0].diffuse = diffuseColor;

	vertices[1].position.x = m_kRect.left + fimgWidth - 0.5f;
	vertices[1].position.y = m_kRect.top - 0.5f;
	vertices[1].position.z = 0.0f;
	vertices[1].texCoord = TTextureCoordinate(eu, sv);
	vertices[1].diffuse = diffuseColor;

	vertices[2].position.x = m_kRect.left - 0.5f;
	vertices[2].position.y = m_kRect.top + fimgHeight - 0.5f;
	vertices[2].position.z = 0.0f;
	vertices[2].texCoord = TTextureCoordinate(su, ev);
	vertices[2].diffuse = diffuseColor;

	vertices[3].position.x = m_kRect.left + fimgWidth - 0.5f;
	vertices[3].position.y = m_kRect.top + fimgHeight - 0.5f;
	vertices[3].position.z = 0.0f;
	vertices[3].texCoord = TTextureCoordinate(eu, ev);
	vertices[3].diffuse = diffuseColor;

	// 2004.11.18.myevan.ctrl+alt+del 반복 사용시 튕기는 문제 
	if (CGraphicBase::SetPDTStream(vertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER.SetTexture(0, m_pRenderTexture->GetD3DTexture());
		STATEMANAGER.SetTexture(1, NULL);
		STATEMANAGER.SetVertexShader(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
		STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
	}
}

void CPythonNewWeb::SetFocus()
{
	if (!m_bIsShow)
		return;

	m_pCurrentWebView->Focus();
	m_bIsFocused = true;
}

void CPythonNewWeb::KillFocus()
{
	if (!m_bIsShow)
		return;

	m_pCurrentWebView->Unfocus();
	m_bIsFocused = false;
}
void CPythonNewWeb__KillFocus()
{
	CPythonNewWeb::Instance().KillFocus();
}
#endif

/*******************************************************************\
| [PUBLIC] Event Functions
\*******************************************************************/

#ifdef WEBBROWSER_OFFSCREEN
void CPythonNewWeb::OnMouseMove(int iMouseX, int iMouseY)
{
	if (!m_bIsShow)
		return;

	//SetFocus();
	m_pCurrentWebView->InjectMouseMove(iMouseX, iMouseY);
}

void CPythonNewWeb::OnMouseDown(Awesomium::MouseButton kMouseButton)
{
	if (!m_bIsShow)
		return;

	//SetFocus();
	m_pCurrentWebView->InjectMouseDown(kMouseButton);
}

void CPythonNewWeb::OnMouseUp(Awesomium::MouseButton kMouseButton)
{
	if (!m_bIsShow)
		return;

	//SetFocus();
	m_pCurrentWebView->InjectMouseUp(kMouseButton);
}

void CPythonNewWeb::OnMouseWheel(int iScrollVert, int iScrollHori)
{
	if (!m_bIsShow)
		return;

	//SetFocus();
	m_pCurrentWebView->InjectMouseWheel(iScrollVert, iScrollHori);
}

bool CPythonNewWeb::OnKeyEvent(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	if (!CanRecvKey())
		return false;

	SetFocus();

	Awesomium::WebKeyboardEvent kEvent(uiMsg, wParam, lParam);
	m_pCurrentWebView->InjectKeyboardEvent(kEvent);

	return true;
}

bool CPythonNewWeb::CanRecvKey() const
{
	return m_bIsShow && m_bCanHandleIME && m_bIsFocused;
}
#endif
#endif
