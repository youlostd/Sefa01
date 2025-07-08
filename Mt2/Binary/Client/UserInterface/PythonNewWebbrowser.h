#pragma once

#include "StdAfx.h"

#ifdef ENABLE_NEW_WEBBROWSER
#include <Awesomium\WebCore.h>

#define WEBBROWSER_OFFSCREEN // if disabled it's rendered in a new window

class CPythonNewWeb : public CSingleton<CPythonNewWeb>
{
	public:
		class CWebViewListener : public Awesomium::WebViewListener::View
		{
			void OnAddConsoleMessage(Awesomium::WebView* caller,
				const Awesomium::WebString& message,
				int line_number,
				const Awesomium::WebString& source);
			void OnChangeTitle(Awesomium::WebView* caller,
				const Awesomium::WebString& title) {}
			void OnChangeAddressBar(Awesomium::WebView* caller,
				const Awesomium::WebURL& url);
			void OnChangeTooltip(Awesomium::WebView* caller,
				const Awesomium::WebString& tooltip) {}
			void OnChangeTargetURL(Awesomium::WebView* caller,
				const Awesomium::WebURL& url);
			void OnChangeCursor(Awesomium::WebView* caller,
				Awesomium::Cursor cursor) {}
			void OnChangeFocus(Awesomium::WebView* caller,
				Awesomium::FocusedElementType focused_type) {}
			void OnShowCreatedWebView(Awesomium::WebView* caller,
				Awesomium::WebView* new_view,
				const Awesomium::WebURL& opener_url,
				const Awesomium::WebURL& target_url,
				const Awesomium::Rect& initial_pos,
				bool is_popup);
		};

		class CWebLoadListener : public Awesomium::WebViewListener::Load
		{
			void OnBeginLoadingFrame(Awesomium::WebView* caller,
				int64 frame_id,
				bool is_main_frame,
				const Awesomium::WebURL& url,
				bool is_error_page) {}
			void OnFailLoadingFrame(Awesomium::WebView* caller,
				int64 frame_id,
				bool is_main_frame,
				const Awesomium::WebURL& url,
				int error_code,
				const Awesomium::WebString& error_desc) {}
			void OnFinishLoadingFrame(Awesomium::WebView* caller,
				int64 frame_id,
				bool is_main_frame,
				const Awesomium::WebURL& url) {}
			void OnDocumentReady(Awesomium::WebView* caller,
				const Awesomium::WebURL& url);
		};

#ifdef WEBBROWSER_OFFSCREEN
		class CInputMethodListener : public Awesomium::WebViewListener::InputMethodEditor
		{
			void OnUpdateIME(Awesomium::WebView *caller, Awesomium::TextInputType type, int caret_x, int caret_y);
			void OnCancelIME(Awesomium::WebView *caller);
			void OnChangeIMERange(Awesomium::WebView *caller, unsigned int start, unsigned int end);
		};
#endif

		typedef struct {
			Awesomium::WebView*		pWebView;
			CWebViewListener		kWebViewListener;
			CWebLoadListener		kWebLoadListener;
#ifdef WEBBROWSER_OFFSCREEN
			CInputMethodListener	kInputMethodListener;
#endif
			std::string				m_stLastURL;
		} TWebViewData;
		typedef std::map<std::string, TWebViewData> TWebViewMap;

#ifdef WEBBROWSER_OFFSCREEN
		typedef union SARGBConverterUnion {
			BYTE	a;
			BYTE	r;
			BYTE	g;
			BYTE	b;
			DWORD	color;
		} TARGBConverterUnion;

		enum EEventList {
			EVENT_MOUSE_MOVE,
			EVENT_MOUSE_LEFT_DOWN,
			EVENT_MOUSE_LEFT_UP,
			EVENT_MOUSE_RIGHT_DOWN,
			EVENT_MOUSE_RIGHT_UP,
			EVENT_MOUSE_WHEEL,
			EVENT_MAX_NUM,
		};
#endif

	public:
		CPythonNewWeb();
		~CPythonNewWeb();

		void	Initialize();
		void	Destroy();

	private:
		Awesomium::WebView*	__CreateWebView(int iWidth, int iHeight, bool bEnableJS);
		TWebViewData*		__GetWebViewData(const char* c_pszWebType, const char* c_pszWebPage = NULL, bool* pbIsPreloaded = NULL, int iWidth = 0, int iHeight = 0);
		Awesomium::WebView*	__GetWebView(const char* c_pszWebType, const char* c_pszWebPage = NULL, bool* pbIsPreloaded = NULL, int iWidth = 0, int iHeight = 0);
		void	__CallLoadFinishEvent();

#ifdef WEBBROWSER_OFFSCREEN
		void	__CreateRenderTexture(int iWidth, int iHeight);
		void	__DestroyRenderTexture();
#endif

	public:
		void	SetLoadFinishEvent(PyObject* poHandle, const std::string& c_rstFuncName);
		void	ExecLoadFinishEvent();
		
		void	SetCurrentEventHandle(PyObject* poHandle);
		void	CallEventHandle(const std::string& c_rstEventName, PyObject* poArgs = NULL);

		void	Create(const char* c_pszTypeName, int iWidth, int iHeight);
		void	PreLoad(const char* c_pszTypeName, const char* c_pszWebPage, int iWidth, int iHeight);
		void	Show(const char* c_pszWebPage, const RECT& c_rkRect, const char* c_pszTypeName = NULL);
		void	OnShow();
		void	Move(const RECT& c_rkRect);
		void	Hide(bool bForce = false);

		void	ExecuteJavascript(const std::string& c_rstJavascriptCode);

		void	WindowResize(int iNewWidth, int iNewHeight);
		const char*	GetWindowURL();
		void	GetWindowSize(int& iRetWidth, int& iRetHeight);

		void	Update();
#ifdef WEBBROWSER_OFFSCREEN
		void	Render();

		void	SetFocus();
		void	KillFocus();
		bool	IsFocus() const { return m_bIsFocused; }
#endif

#ifdef WEBBROWSER_OFFSCREEN
	public:
		void	OnMouseMove(int iMouseX, int iMouseY);
		void	OnMouseDown(Awesomium::MouseButton kMouseButton);
		void	OnMouseUp(Awesomium::MouseButton kMouseButton);
		void	OnMouseWheel(int iScrollVert, int iScrollHori);
		bool	OnKeyEvent(UINT uiMsg, WPARAM wParam, LPARAM lParam);
		bool	CanRecvKey() const;
#endif

	private:
		Awesomium::WebCore*	m_pWebCore;
		Awesomium::WebPreferences m_kWebPreferences;
		Awesomium::WebPreferences m_kWebPreferences_NoJS;
		Awesomium::WebSession* m_pWebSession;
		Awesomium::WebSession* m_pWebSession_NoJS;
		Awesomium::WebView* m_pCurrentWebView;
		TWebViewMap			m_map_pWebView;

		bool	m_bIsShow;
		bool	m_bIsLoading;
		RECT	m_kRect;

		PyObject*	m_poLoadFinishHandle;
		std::string	m_stLoadFinishFunc;


		PyObject*	m_poCurrentEventHandle;

#ifdef WEBBROWSER_OFFSCREEN
		CGraphicImageTexture*	m_pRenderTexture;
		TARGBConverterUnion		m_argbConverterUnion;

	protected:
		bool					m_bCanHandleIME;
		bool					m_bIsFocused;
#endif


};
#endif
