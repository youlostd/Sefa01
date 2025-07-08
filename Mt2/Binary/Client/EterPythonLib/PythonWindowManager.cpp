#include "StdAfx.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"
#include "PythonGridSlotWindow.h"
#include "PythonWindowManager.h"

//#define __WINDOW_LEAK_CHECK__

BOOL g_bShowOverInWindowName = FALSE;

#ifdef ENABLE_NEW_WEBBROWSER
extern void CPythonNewWeb__KillFocus();
#endif

namespace UI
{	
	static PyObject* gs_poEmptyTuple = NULL;

	PyObject * BuildEmptyTuple()
	{
		Py_INCREF(gs_poEmptyTuple);
		return gs_poEmptyTuple;
	}

	std::set<CWindow*> gs_kSet_pkWnd;
	
	CWindowManager::CWindowManager()
		: 
		m_pActiveWindow(NULL),
		m_pPointWindow(NULL),
		m_pLeftCaptureWindow(NULL),
		m_pMiddleCaptureWindow(NULL),
		m_pRightCaptureWindow(NULL),
		m_pLockWindow(NULL),
		m_bAttachingFlag(FALSE),
		m_dwAttachingType(0),
		m_dwAttachingIndex(0),
		m_dwAttachingSlotNumber(0),
		m_poMouseHandler(NULL),
		m_iHres(0),
		m_iVres(0),
		m_bOnceIgnoreMouseLeftButtonUpEventFlag(FALSE),
		m_iIgnoreIMEReturnEndTime(0)
	{		
		m_pRootWindow = new CWindow(NULL);
		m_pRootWindow->SetName("root");
		m_pRootWindow->Show();

		const char * layerTbl[] = {"GAME","UI_BOTTOM","UI","TOP_MOST","CURTAIN"};

		for(DWORD layer = 0; layer < sizeof(layerTbl)/sizeof(layerTbl[0]); layer++)
		{
			CWindow * pLayer = new CLayer(NULL);
			pLayer->SetName(layerTbl[layer]);
			pLayer->Show();
			m_LayerWindowMap.insert(TLayerContainer::value_type(layerTbl[layer], pLayer));
			m_pRootWindow->AddChild(pLayer);
			m_LayerWindowList.push_back(pLayer);
		}

		m_ActiveWindowList.clear();
		m_LockWindowList.clear();

		gs_poEmptyTuple = Py_BuildValue("()");
	}

	CWindowManager::~CWindowManager()
	{		
		Py_DECREF(gs_poEmptyTuple);

		stl_wipe_second(m_LayerWindowMap);
		m_LayerWindowMap.clear();
		m_LayerWindowList.clear();
		m_KeyCaptureWindowMap.clear();

		delete m_pRootWindow;
		m_pRootWindow = NULL;
	}

	void CWindowManager::Destroy()
	{
		__ClearReserveDeleteWindowList();
#ifdef __WINDOW_LEAK_CHECK__
		std::set<CWindow*>::iterator i;
		for (i=gs_kSet_pkWnd.begin(); i!=gs_kSet_pkWnd.end(); ++i)
		{
			CWindow* pkWnd=*i;
			Logf(1, "CWindowManager::Destroy LOST WINDOW %s\n", pkWnd->GetName());
		}
#endif		

	}

	void CWindowManager::SetMouseHandler(PyObject * poMouseHandler)
	{
		m_poMouseHandler = poMouseHandler;
	}

	CWindow * CWindowManager::RegisterWindow(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CWindow(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);
#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return (pWin);
	}

	CWindow *	CWindowManager::__NewWindow(PyObject * po, DWORD dwWndType)
	{
		switch(dwWndType)
		{
			case WT_SLOT:
				return new CSlotWindow(po);
				break;
			case WT_GRIDSLOT:
				return new CGridSlotWindow(po);
				break;
			case WT_TEXTLINE:
				return new CTextLine(po);
				break;
			case WT_MARKBOX:
				return new CMarkBox(po);
				break;
			case WT_IMAGEBOX:
				return new CImageBox(po);
				break;
			case WT_EXP_IMAGEBOX:
				return new CExpandedImageBox(po);
				break;
			case WT_ANI_IMAGEBOX:
				return new CAniImageBox(po);
				break;
			case WT_BUTTON:
				return new CButton(po);
				break;
			case WT_RATIOBUTTON:
				return new CRadioButton(po);
				break;
			case WT_TOGGLEBUTTON:
				return new CToggleButton(po);
				break;
			case WT_DRAGBUTTON:
				return new CDragButton(po);
				break;
			case WT_BOX:
				return new CBox(po);
				break;
			case WT_BAR:
				return new CBar(po);
				break;
			case WT_LINE:
				return new CLine(po);
				break;
			case WT_BAR3D:
				return new CBar3D(po);
				break;
			case WT_NUMLINE:
				return new CNumberLine(po);
				break;
			case WT_RENDER_TARGET:
				return new CRenderTarget(po);
				break;
			default:
				assert(!"CWindowManager::__NewWindow");
				break;
		}	
		return new CWindow(po);				
	}

	CWindow *	CWindowManager::RegisterTypeWindow(PyObject * po, DWORD dwWndType, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = __NewWindow(po, dwWndType);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif		
		return pWin;
	}

	CWindow * CWindowManager::RegisterSlotWindow(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CSlotWindow(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif		
		return pWin;
	}

	CWindow * CWindowManager::RegisterGridSlotWindow(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CGridSlotWindow(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterTextLine(PyObject * po, const char * c_szLayer)
	{
		auto it = m_LayerWindowMap.find(c_szLayer);
		if (it == m_LayerWindowMap.end() || it->second == NULL)
		{
			TraceError("INVALID WINDOW LAYER %s", c_szLayer);
			return NULL;
		}

		CWindow * pWin = new CTextLine(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterImageBox(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CImageBox(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterMarkBox(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CMarkBox(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterExpandedImageBox(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CExpandedImageBox(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterAniImageBox(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CAniImageBox(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterButton(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CButton(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterRadioButton(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CRadioButton(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterToggleButton(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CToggleButton(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterDragButton(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CDragButton(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterBox(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CBox(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterBar(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CBar(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterLine(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CLine(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterBar3D(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CBar3D(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterNumberLine(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CNumberLine(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	CWindow * CWindowManager::RegisterRenderTarget(PyObject * po, const char * c_szLayer)
	{
		assert(m_LayerWindowMap.end() != m_LayerWindowMap.find(c_szLayer));

		CWindow * pWin = new CRenderTarget(po);
		m_LayerWindowMap[c_szLayer]->AddChild(pWin);

#ifdef __WINDOW_LEAK_CHECK__
		gs_kSet_pkWnd.insert(pWin);
#endif
		return pWin;
	}

	void CWindowManager::NotifyDestroyWindow(CWindow * pWindow)
	{
		if (pWindow == m_pActiveWindow)
			m_pActiveWindow = NULL;

		if (pWindow == m_pPointWindow)
			m_pPointWindow = NULL;

		if (pWindow == m_pLeftCaptureWindow)
			m_pLeftCaptureWindow = NULL;

		if (pWindow == m_pMiddleCaptureWindow)
			m_pMiddleCaptureWindow = NULL;

		if (pWindow == m_pRightCaptureWindow)
			m_pRightCaptureWindow = NULL;

		if (pWindow == m_pLockWindow)
			m_pLockWindow = NULL;

		m_LockWindowList.remove(pWindow);
		m_ActiveWindowList.remove(pWindow);
		m_PickAlwaysWindowList.remove(pWindow);

		TKeyCaptureWindowMap::iterator itor = m_KeyCaptureWindowMap.begin();
		for (; itor != m_KeyCaptureWindowMap.end();)
		{
			if (pWindow == itor->second)
			{
				itor = m_KeyCaptureWindowMap.erase(itor);
			}
			else
			{
				++itor;
			}
		}
	}

	void CWindowManager::DestroyWindow(CWindow * pWin)
	{
		pWin->OnHideWithChilds();

		NotifyDestroyWindow(pWin);
		if (pWin->HasParent())
		{
			CWindow * pParentWin = pWin->GetParent();
			pParentWin->DeleteChild(pWin);
		}
		pWin->Clear();
		m_ReserveDeleteWindowList.push_back(pWin);
	}

	BOOL CWindowManager::IsDragging()
	{
		long ldx = abs(m_lMouseX - m_lPickedX);
		long ldy = abs(m_lMouseY - m_lPickedY);
		if (ldx+ldy < 10)
			return FALSE;

		return TRUE;
	}

	BOOL CWindowManager::IsAttaching()
	{
		return m_bAttachingFlag;
	}

	DWORD CWindowManager::GetAttachingType()
	{
		return m_dwAttachingType;
	}

	DWORD CWindowManager::GetAttachingIndex()
	{
		return m_dwAttachingIndex;
	}

	DWORD CWindowManager::GetAttachingSlotNumber()
	{
		return m_dwAttachingSlotNumber;
	}

	void CWindowManager::GetAttachingIconSize(BYTE * pbyWidth, BYTE * pbyHeight)
	{
		*pbyWidth = m_byAttachingIconWidth;
		*pbyHeight = m_byAttachingIconHeight;
	}

	void CWindowManager::AttachIcon(DWORD dwType, DWORD dwIndex, DWORD dwSlotNumber, BYTE byWidth, BYTE byHeight)
	{
		m_bAttachingFlag = TRUE;
		m_dwAttachingType = dwType;
		m_dwAttachingIndex = dwIndex;
		m_dwAttachingSlotNumber = dwSlotNumber;

		m_byAttachingIconWidth = byWidth;
		m_byAttachingIconHeight = byHeight;
	}

	void CWindowManager::SetAttachingFlag(BOOL bFlag)
	{
		m_bAttachingFlag = bFlag;
	}

	void CWindowManager::DeattachIcon()
	{
#ifdef AHMET_FISH_EVENT_SYSTEM
		if(IsDisableDeattach())
			return;
#endif

		SetAttachingFlag(FALSE);
		if (m_poMouseHandler)
			PyCallClassMemberFunc(m_poMouseHandler, "DeattachObject", BuildEmptyTuple());
	}

	void CWindowManager::SetParent(CWindow * pWindow, CWindow * pParentWindow)
	{
		if (!pWindow)
		{
			assert(!"CWindowManager::SetParent - There is no self window!");
			return;
		}
		if (!pParentWindow)
		{
			assert(!"There is no parent window");
			return;
		}

		if (pWindow->HasParent())
		{
			CWindow * pOldParentWindow = pWindow->GetParent();

			if (pParentWindow == pOldParentWindow)
				return;

			pOldParentWindow->DeleteChild(pWindow);
		}

		pParentWindow->AddChild(pWindow);
	}

	void CWindowManager::SetPickAlways(CWindow * pWindow)
	{
		m_PickAlwaysWindowList.push_back(pWindow);
	}

	void CWindowManager::OnceIgnoreMouseLeftButtonUpEvent()
	{
		m_bOnceIgnoreMouseLeftButtonUpEventFlag = TRUE;
		m_iIgnoreEndTime = timeGetTime() + 500;
	}

	void CWindowManager::LockWindow(CWindow * pWin)
	{
		if (m_pActiveWindow)
			m_pActiveWindow->OnKillFocus();

		// 이미 락된 윈도우리스트안에 있다면 제거한다..
		m_LockWindowList.remove(pWin);

		if (m_pLockWindow)
		{
			if (m_pLockWindow==pWin)
				return;

			m_LockWindowList.push_back(m_pLockWindow);
		}

		m_pLockWindow = pWin;
	}

	void CWindowManager::UnlockWindow()
	{
		if (m_pLockWindow)
		{
			if (m_LockWindowList.empty())
			{
				m_pLockWindow = NULL;
				if (m_pActiveWindow)
					m_pActiveWindow->OnSetFocus();
			}
			else
			{
				m_pLockWindow = m_LockWindowList.back();
				m_LockWindowList.pop_back();
			}			
		}
	}

	void CWindowManager::ActivateWindow(CWindow * pWin)
	{
		m_ActiveWindowList.remove(pWin);

		if (pWin == m_pActiveWindow)
			return;

		if (m_pActiveWindow)
		{
			// NOTE : 누적된 Window가 많아지면 Clear를 해줘야 할까?
			//        일단은 중복 누적이 안되며 포커스 되는 갯수 자체가 5개 미만이니 굳이 필요하지는 않을 듯.. - [levites]
			m_ActiveWindowList.push_back(m_pActiveWindow);
			m_pActiveWindow->OnKillFocus();
		}

		m_pActiveWindow = pWin;

		if (m_pActiveWindow)
		{
			m_pActiveWindow->OnSetFocus();
		}
	}

	void CWindowManager::DeactivateWindow()
	{
		if (m_pActiveWindow)
		{
			if (m_ActiveWindowList.empty())
			{
				m_pActiveWindow->OnKillFocus();
				m_pActiveWindow = NULL;
			}
			else
			{
				m_pActiveWindow->OnKillFocus();

				m_pActiveWindow = m_ActiveWindowList.back();
				m_ActiveWindowList.pop_back();

				m_pActiveWindow->OnSetFocus();
			}
		}
	}

	void CWindowManager::SetTop(CWindow * pWin)
	{
		if (!pWin->HasParent())
			return;

		CWindow * pParentWindow = pWin->GetParent();
		pParentWindow->SetTop(pWin);

		// NOTE : Capture가 리셋된다..? - [levites]
		// NOTE : 인벤토리에서 아이템을 드래그 해서 밖에다 놓을때 캡춰가 남아서 창의 버튼을 두번 눌러야 하는 버그를 위해 추가
//		ResetCapture();
	}

	void CWindowManager::SetTopUIWindow()
	{
		if (m_pLockWindow)
			return;

		// GameLayer에 속해 있는 윈도우가 피킹 됐다면 무조건 SetTop을 해준다.
		TLayerContainer::iterator itor = m_LayerWindowMap.find("UI");
		if (itor == m_LayerWindowMap.end())
			return;
		CWindow * pGameLayer = itor->second;
		CWindow * pTopWindow = pGameLayer->PickTopWindow(m_lMouseX, m_lMouseY);
		if (pTopWindow)
		{
			SetTop(pTopWindow);
		}
	}

	CWindow * CWindowManager::GetActivateWindow()
	{
		return m_pActiveWindow;
	}

	void CWindowManager::ResetCapture()
	{
		m_pLeftCaptureWindow = NULL;
		m_pMiddleCaptureWindow = NULL;
		m_pRightCaptureWindow = NULL;
	}

	void CWindowManager::SetResolution(int hres, int vres)
	{
		if (hres<=0 || vres<=0)
			return;

		m_iHres = hres;
		m_iVres = vres;
	}

	float CWindowManager::GetAspect()
	{
		return (m_iHres)/float(m_iVres);
	}

	void CWindowManager::SetScreenSize(long lWidth, long lHeight)
	{
		m_lWidth	= lWidth;
		m_lHeight	= lHeight;

		for (TLayerContainer::iterator itor = m_LayerWindowMap.begin(); itor != m_LayerWindowMap.end(); ++itor)
		{
			itor->second->SetSize(lWidth, lHeight);
		}
	}

	void CWindowManager::__ClearReserveDeleteWindowList()
	{
		for (TWindowContainer::iterator itor = m_ReserveDeleteWindowList.begin(); itor != m_ReserveDeleteWindowList.end(); ++itor)
		{
			CWindow * pWin = *itor;
#ifdef __WINDOW_LEAK_CHECK__
			gs_kSet_pkWnd.erase(pWin);
#endif
			delete pWin;
		}
		m_ReserveDeleteWindowList.clear();

	}	

	void CWindowManager::Update()
	{
		__ClearReserveDeleteWindowList();
		
		m_pRootWindow->Update();
	}

	void CWindowManager::Render()
	{
		m_pRootWindow->Render();
	}

	CWindow * CWindowManager::__PickWindow(long x, long y)
	{
		if (m_pLockWindow)
		{
			return m_pLockWindow->PickWindow(x, y);
		}

		for (TWindowContainer::iterator itor = m_PickAlwaysWindowList.begin(); itor != m_PickAlwaysWindowList.end(); ++itor)
		{
			CWindow * pWindow = *itor;
			if (pWindow->IsRendering())
			if (pWindow->IsIn(x, y))
				return pWindow;
		}

		for (TWindowContainer::reverse_iterator ritor = m_LayerWindowList.rbegin(); ritor != m_LayerWindowList.rend(); ++ritor)
		{
			CWindow * pLayer = *ritor;
			CWindow * pPickedWindow = pLayer->PickWindow(x, y);

			if (pPickedWindow != pLayer)
				return pPickedWindow;
		}

		return NULL;
	}

	void CWindowManager::SetMousePosition(long x, long y)
	{
		if (m_iHres==0)
			return;

		if (m_iVres==0)
			return;

		m_lMouseX = m_lWidth * x / m_iHres;
		m_lMouseY = m_lHeight * y / m_iVres;
	}

	void CWindowManager::GetMousePosition(long & rx, long & ry)
	{
		rx = m_lMouseX;
		ry = m_lMouseY;
	}

	void CWindowManager::RunMouseMove(long x, long y)
	{
		if (IsAttaching())
		{
			if (x > m_lWidth)
				x = m_lWidth;
			if (y > m_lHeight)
				y = m_lHeight;
		}

		SetMousePosition(x, y);
		CWindow * pPointWindow = __PickWindow(m_lMouseX, m_lMouseY);

		if (g_bShowOverInWindowName)
		{
			if (pPointWindow)
			{
				static std::string strPickWindowName = "";
				if (0 != strPickWindowName.compare(pPointWindow->GetName()))
				{
					Tracef(" OverInWindowName [%s]\n", pPointWindow->GetName());
					strPickWindowName = pPointWindow->GetName();
				}
			}
		}

		if (m_pLeftCaptureWindow)
		{
			CWindow * pWin = m_pLeftCaptureWindow;

			if (pWin->IsFlag(CWindow::FLAG_MOVABLE))
			{
				long x = m_lMouseX - m_lDragX;
				long y = m_lMouseY - m_lDragY;
				if (pWin->HasParent())
				{
					x -= pWin->GetParent()->GetRect().left;
					y -= pWin->GetParent()->GetRect().top;
				}

				long lx, ly;
				pWin->GetPosition(&lx, &ly);
				if (pWin->IsFlag(CWindow::FLAG_RESTRICT_X))
				{
					x = lx;
				}
				if (pWin->IsFlag(CWindow::FLAG_RESTRICT_Y))
				{
					y = ly;
				}

				if (pWin->IsFlag(CWindow::FLAG_LIMIT))
				{
					RECT limitRect = pWin->GetLimitBias();

					limitRect.right = m_lWidth - limitRect.right;
					limitRect.bottom = m_lHeight - limitRect.bottom;

					if (x < limitRect.left)
						x = limitRect.left;
					else if (x + pWin->GetWidth() >= limitRect.right)
						x = limitRect.right - pWin->GetWidth();

					if (y < limitRect.top)
						y = limitRect.top;
					else if (y + pWin->GetHeight() >= limitRect.bottom)
						y = limitRect.bottom - pWin->GetHeight();
				}

				pWin->SetPosition(x, y);
				pWin->OnMoveWindow(x, y);
			}
			else if (m_pLeftCaptureWindow->IsFlag(CWindow::FLAG_DRAGABLE))
			{
				long x = m_lMouseX - m_lDragX;
				long y = m_lMouseY - m_lDragY;
				m_pLeftCaptureWindow->OnMouseDrag(x, y);
			}
		}

		if (m_pPointWindow != pPointWindow)
		{
#ifdef _DEBUG
			if (pPointWindow && pPointWindow->GetName())
				Tracenf("PointWindow: %s", pPointWindow->GetName());
#endif
			if (m_pPointWindow)
				m_pPointWindow->OnMouseOverOut();

			m_pPointWindow = pPointWindow;

			if (m_pPointWindow)
				m_pPointWindow->OnMouseOverIn();
		}

		if (m_pPointWindow)
		{
			m_pPointWindow->OnMouseOver();
		}
	}

	void CWindowManager::RunMouseLeftButtonDown(long x, long y)
	{
		SetTopUIWindow();

		/////

		SetMousePosition(x, y);
		CWindow * pWin = GetPointWindow();

		if (!pWin)
			return;

#ifdef ENABLE_NEW_WEBBROWSER
		CPythonNewWeb__KillFocus();
#endif

		// Attach
		if (pWin->IsFlag(CWindow::FLAG_ATTACH))
			pWin = pWin->GetRoot();

		// Drag
		if (!pWin->IsFlag(CWindow::FLAG_NOT_CAPTURE))
			m_pLeftCaptureWindow = pWin;

		m_lDragX = m_lMouseX - pWin->GetRect().left;
		m_lDragY = m_lMouseY - pWin->GetRect().top;
		m_lPickedX = m_lMouseX;
		m_lPickedY = m_lMouseY;

		// Event
		pWin->OnMouseLeftButtonDown();
	}

	void CWindowManager::RunMouseLeftButtonUp(long x, long y)
	{
		if (m_bOnceIgnoreMouseLeftButtonUpEventFlag)
		{
			m_bOnceIgnoreMouseLeftButtonUpEventFlag = FALSE;

			if (timeGetTime() < m_iIgnoreEndTime)
			{
				return;
			}
		}

		SetMousePosition(x, y);

		if (m_pLeftCaptureWindow)
		{
			if (m_pLeftCaptureWindow->OnMouseLeftButtonUp())
			{
				// NOTE : 여기서 m_pLeftCaptureWindow가 NULL 일 수 있습니다!! - [levites]
				m_pLeftCaptureWindow = NULL;
				return;
			}
		}

		CWindow * pWin = GetPointWindow();
		if (pWin)
			pWin->OnMouseLeftButtonUp();

		m_pLeftCaptureWindow = NULL;
	}

	void CWindowManager::RunMouseLeftButtonDoubleClick(long x, long y)
	{
		SetMousePosition(x, y);

		CWindow * pWin = GetPointWindow();
		if (!pWin)
			return;

		pWin->OnMouseLeftButtonDoubleClick();
	}

	void CWindowManager::RunMouseRightButtonDown(long x, long y)
	{
		SetTopUIWindow();

		///////////////////////////////////////////////////////////////////////////////////////////

		SetMousePosition(x, y);
		CWindow * pWin = GetPointWindow();
		if (!pWin)
			return;

#ifdef ENABLE_NEW_WEBBROWSER
		CPythonNewWeb__KillFocus();
#endif

		// Attach
		if (pWin->IsFlag(CWindow::FLAG_ATTACH))
			pWin = pWin->GetRoot();

		// Drag
		if (!pWin->IsFlag(CWindow::FLAG_NOT_CAPTURE))
			m_pRightCaptureWindow = pWin;

		pWin->OnMouseRightButtonDown();
	}

	void CWindowManager::RunMouseRightButtonUp(long x, long y)
	{
		if (m_pRightCaptureWindow)
		{
			if (m_pRightCaptureWindow->OnMouseRightButtonUp())
			{
				m_pRightCaptureWindow = NULL;
				return;
			}
		}

		CWindow * pWin = GetPointWindow();
		if (pWin)
			pWin->OnMouseRightButtonUp();

		m_pRightCaptureWindow = NULL;
		DeattachIcon();
	}

	void CWindowManager::RunMouseRightButtonDoubleClick(long x, long y)
	{
		SetMousePosition(x, y);

		CWindow * pWin = GetPointWindow();
		if (pWin)
		{
			pWin->OnMouseRightButtonDoubleClick();
		}
	}

	void CWindowManager::RunMouseMiddleButtonDown(long x, long y)
	{
		SetMousePosition(x, y);

		CWindow * pWin = GetPointWindow();
		if (!pWin)
			return;

		m_pMiddleCaptureWindow = pWin;

		pWin->OnMouseMiddleButtonDown();
	}

	void CWindowManager::RunMouseMiddleButtonUp(long x, long y)
	{
		SetMousePosition(x, y);

		if (m_pMiddleCaptureWindow)
		{
			if (m_pMiddleCaptureWindow->OnMouseMiddleButtonUp())
			{
				m_pMiddleCaptureWindow = NULL;
				return;
			}
		}

		CWindow * pWin = GetPointWindow();
		if (!pWin)
			return;

		pWin->OnMouseMiddleButtonUp();
		m_pMiddleCaptureWindow = NULL;
	}

	bool CWindowManager::RunMouseWheel(int iLen)
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnMouseWheel(iLen);
			return true;
		}

		if (m_pActiveWindow)
			if (m_pActiveWindow->IsRendering())
			{
				if (m_pActiveWindow->OnMouseWheel(iLen))
					return true;
			}

		return m_pRootWindow->RunMouseWheelEvent(iLen);
	}

	// IME
	void CWindowManager::RunIMEUpdate()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMEUpdate();
			return;
		}

		if (!m_pActiveWindow)
			return;
		if (!m_pActiveWindow->IsRendering())
			return;

		m_pActiveWindow->OnIMEUpdate();
	}

	void CWindowManager::RunIMETabEvent()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMETabEvent();
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMETabEvent())
				return;
		}

		if (!m_pRootWindow->RunIMETabEvent())
		{
			if (!m_ActiveWindowList.empty())
			{
				CWindow * pWindow = *(m_ActiveWindowList.begin());
				ActivateWindow(pWindow);

				/////////////////////////////////////////////

				CWindow * pParentWindow = pWindow;
				CWindow * pCurrentWindow = pWindow->GetParent();

				DWORD dwMaxLoopCount = 20;
				for (DWORD i = 0; i < dwMaxLoopCount; ++i)
				{
					if (!pParentWindow)
						break;

					if (pParentWindow == m_LayerWindowMap["GAME"])
					{
						SetTop(pCurrentWindow);
						break;
					}

					pCurrentWindow = pParentWindow;
					pParentWindow = pCurrentWindow->GetParent();
				}
			}
		}
	}

	void CWindowManager::RunIMEReturnEvent()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMEReturnEvent();
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMEReturnEvent())
				return;
		}

		m_pRootWindow->RunIMEReturnEvent();
	}

	void CWindowManager::RunIMEKeyDown(int vkey)
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnKeyDown(vkey);
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMEKeyDownEvent(vkey))
				return;
		}

		// NOTE : 전체로 돌리지 않고 Activate되어있는 EditLine에만 보내는 이벤트
	}

	void CWindowManager::RunChangeCodePage()
	{
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMEChangeCodePage())
				return;
		}
	}
	void CWindowManager::RunOpenCandidate()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMEOpenCandidateListEvent();
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMEOpenCandidateListEvent())
				return;
		}
	}

	void CWindowManager::RunCloseCandidate()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMECloseCandidateListEvent();
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMECloseCandidateListEvent())
				return;
		}
	}

	void CWindowManager::RunOpenReading()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMEOpenReadingWndEvent();
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMEOpenReadingWndEvent())
				return;
		}
	}

	void CWindowManager::RunCloseReading()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnIMECloseReadingWndEvent();
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsRendering())
		{
			if (m_pActiveWindow->OnIMECloseReadingWndEvent())
				return;
		}
	}
	// IME

	void CWindowManager::RunKeyDown(int vkey)
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnKeyDown(vkey);
			m_KeyCaptureWindowMap.insert(make_pair(vkey, m_pLockWindow));
			return;
		}
		if (m_pActiveWindow)
		if (m_pActiveWindow->IsShow())
		{
			if (m_pActiveWindow->OnKeyDown(vkey))
			{
				m_KeyCaptureWindowMap.insert(make_pair(vkey, m_pActiveWindow));
				return;
			}
		}

		CWindow * pKeyCaptureWindow = m_pRootWindow->RunKeyDownEvent(vkey);
		if (pKeyCaptureWindow)
		if (m_ReserveDeleteWindowList.end() == std::find(m_ReserveDeleteWindowList.begin(), m_ReserveDeleteWindowList.end(), pKeyCaptureWindow))
		{
			m_KeyCaptureWindowMap.insert(TKeyCaptureWindowMap::value_type(vkey, pKeyCaptureWindow));
		}
	}

	void CWindowManager::RunKeyUp(int vkey)
	{
		TKeyCaptureWindowMap::iterator itor = m_KeyCaptureWindowMap.find(vkey);
		if (m_KeyCaptureWindowMap.end() != itor)
		{
			CWindow * pKeyCaptureWindow = itor->second;
			if (pKeyCaptureWindow)
				pKeyCaptureWindow->OnKeyUp(vkey);

			m_KeyCaptureWindowMap.erase(itor);
			return;
		}

		if (m_pLockWindow)
		{
			m_pLockWindow->OnKeyUp(vkey);
			return;
		}

		if (m_pActiveWindow)
		{
			if (m_pActiveWindow->OnKeyUp(vkey))
				return;
		}

		m_pRootWindow->RunKeyUpEvent(vkey);
	}

	bool CWindowManager::RunPressReturnKey()
	{
		if (m_pLockWindow)
			return m_pLockWindow->OnPressReturnKey();

		if (m_pRootWindow->RunPressReturnKeyEvent())
		{
			m_iIgnoreIMEReturnEndTime = timeGetTime() + 50;
			return true;
		}

		return false;
	}

	void CWindowManager::RunPressEscapeKey()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnPressEscapeKey();
			return;
		}

		m_pRootWindow->RunPressEscapeKeyEvent();
	}

	void CWindowManager::RunPressExitKey()
	{
		if (m_pLockWindow)
		{
			m_pLockWindow->OnPressExitKey();
			return;
		}
		if (m_pActiveWindow)
		{
			if (m_pActiveWindow->OnPressExitKey())
				return;
		}

		m_pRootWindow->RunPressExitKeyEvent();
	}

	bool CWindowManager::GetGIFImagePart(const char* c_pszFileName, WORD wIndex, CGraphicImage** pCurImage)
	{
		TGIFImageData* pData;
		if (GetMapGIFImageData(c_pszFileName, &pData))
		{
			if (wIndex >= pData->vecImg.size())
				return false;

			*pCurImage = pData->vecImg.at(wIndex);
			return true;
		}

		return false;
	}

	bool CWindowManager::GetMapGIFImageData(const char* c_pszLinkName, TGIFImageData** ppkData, bool bTryCreate)
	{
		auto it = m_GIFImageMap.find(c_pszLinkName);
		if (it == m_GIFImageMap.end())
		{
			if (bTryCreate)
			{
				ILuint uImg;
				ilGenImages(1, &uImg);
				ilBindImage(uImg);
				ilEnable(IL_ORIGIN_SET);
				ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

				if (!ilLoadImage((const ILstring)c_pszLinkName))
				{
					TraceError("cannot get map gif [load image %s] error %d", c_pszLinkName, ilGetError());
					ilDeleteImages(1, &uImg);
					return false;
				}

				if (!__LoadCurrentGIFImage(uImg, c_pszLinkName))
				{
					ilDeleteImages(1, &uImg);
					return false;
				}

				ilDeleteImages(1, &uImg);

				return GetMapGIFImageData(c_pszLinkName, ppkData, false);
			}

			return false;
		}

		*ppkData = &it->second;
		return true;
	}

	bool CWindowManager::GetResizedGIFImageData(const char * c_pszLinkName, DWORD dwMaxWidth, DWORD dwMaxHeight, TGIFImageData** pkData)
	{
		TGIFImageData* pCurData;
		if (!GetMapGIFImageData(c_pszLinkName, &pCurData))
		{
			TraceError("GetResizedGIFImageData: cannot get current data of [%s]", c_pszLinkName);
			return false;
		}

		if (pCurData->vecImg[0]->GetWidth() <= dwMaxWidth && pCurData->vecImg[0]->GetHeight() <= dwMaxHeight)
		{
			*pkData = pCurData;
			return true;
		}

		char szNewLinkName[1024];
		snprintf(szNewLinkName, sizeof(szNewLinkName), "%s_MAXWIDTH%u_MAXHEIGHT%u", c_pszLinkName, dwMaxWidth, dwMaxHeight);
		if (GetMapGIFImageData(szNewLinkName, pkData, false))
			return true;

		TGIFImageData* pNewData = &m_GIFImageMap[szNewLinkName];
		pNewData->vecImg.reserve(pCurData->vecImg.size());

		ILuint uImg;
		ilGenImages(1, &uImg);
		ilBindImage(uImg);
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

		if (!ilLoadImage((const ILstring)c_pszLinkName))
		{
			ilDeleteImages(1, &uImg);
			m_GIFImageMap.erase(szNewLinkName);
			TraceError("Cannot load image [%s] : %d", c_pszLinkName, ilGetError());
			return false;
		}

		DWORD dwImageWidth = ilGetInteger(IL_IMAGE_WIDTH);
		DWORD dwImageHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		DWORD dwBufSize = dwImageWidth * dwImageHeight * 4;

		int iImageCount = ilGetInteger(IL_NUM_IMAGES);
		pNewData->dwDuration = ilGetInteger(IL_IMAGE_DURATION);

		if (dwImageWidth != pCurData->vecImg[0]->GetWidth() || dwImageHeight != pCurData->vecImg[0]->GetHeight() || iImageCount != pCurData->vecImg.size())
		{
			TraceError("width[%d|%d] height[%d|%d] imgcount[%d|%d]", dwImageWidth, pCurData->vecImg[0]->GetWidth(), dwImageHeight, pCurData->vecImg[0]->GetHeight(), iImageCount, pCurData->vecImg.size());
			ilDeleteImages(1, &uImg);
			m_GIFImageMap.erase(szNewLinkName);
			return false;
		}

		float factor_width = 1.0;
		float factor_height = 1.0;
		if (dwImageWidth > dwMaxWidth)
			factor_width = (float)dwMaxWidth / (float)dwImageWidth;
		if (dwImageHeight > dwMaxHeight)
			factor_height = (float)dwMaxHeight / (float)dwImageHeight;

		float factor = (factor_width < factor_height) ? factor_width : factor_height;
		dwImageWidth *= factor;
		dwImageHeight *= factor;

		BYTE* pbImageBuffer = new BYTE[dwBufSize];
		for (int iImgIdx = 0; iImgIdx < iImageCount; ++iImgIdx)
		{
			ilBindImage(uImg);
			ilActiveImage(iImgIdx + 1);

			iluImageParameter(ILU_FILTER, ILU_SCALE_LANCZOS3);
			iluScale(dwImageWidth, dwImageHeight, 1);

			ilCopyPixels(0, 0, 0, dwImageWidth, dwImageHeight, 1, IL_BGRA, IL_UNSIGNED_BYTE, pbImageBuffer);
			CGraphicImage* pCurImg = new CGraphicImage(pbImageBuffer, dwImageWidth, dwImageHeight, D3DFMT_R8G8B8);
			if (pCurImg)
				pNewData->vecImg.push_back(pCurImg);
		}

		delete[] pbImageBuffer;

		*pkData = pNewData;
		return true;
	}

	bool CWindowManager::GetMapGIFImagePart(const char* c_pszLinkName, WORD wIndex, CGraphicImage** pCurImage)
	{
		TGIFImageData* pkData;
		if (!GetMapGIFImageData(c_pszLinkName, &pkData))
			return false;

		if (pkData->vecImg.size() <= wIndex)
			return false;

		*pCurImage = pkData->vecImg.at(wIndex);
		return true;
	}

	bool CWindowManager::__LoadCurrentGIFImage(ILuint uImg, const char* c_pszLinkName)
	{
		auto& rkData = m_GIFImageMap[c_pszLinkName];
		rkData.vecImg.clear();

		DWORD dwImageWidth = ilGetInteger(IL_IMAGE_WIDTH);
		DWORD dwImageHeight = ilGetInteger(IL_IMAGE_HEIGHT);
		DWORD dwBufSize = dwImageWidth * dwImageHeight * 4;
		BYTE* pbImageBuffer = new BYTE[dwBufSize];

		int iImageCount = ilGetInteger(IL_NUM_IMAGES);
		rkData.dwDuration = ilGetInteger(IL_IMAGE_DURATION);

		for (int iImgIdx = 0; iImgIdx < iImageCount; ++iImgIdx)
		{
			ilBindImage(uImg);
			ilActiveImage(iImgIdx + 1);

			ilCopyPixels(0, 0, 0, dwImageWidth, dwImageHeight, 1, IL_BGRA, IL_UNSIGNED_BYTE, pbImageBuffer);
			CGraphicImage* pCurImg = new CGraphicImage(pbImageBuffer, dwImageWidth, dwImageHeight, D3DFMT_R8G8B8);
			if (pCurImg)
				rkData.vecImg.push_back(pCurImg);
		}

		delete[] pbImageBuffer;
		return true;
	}

	bool CWindowManager::GetScaledImagePtr(CGraphicImage* pImage, DWORD dwWidth, DWORD dwHeight, CGraphicImage** ppImage)
	{
		if (!pImage)
		{
			TraceError("GetScaledImagePtr : no image given");
			return false;
		}

		TImageScaleKeyDataByPtr kKey;
		kKey.first = pImage;
		kKey.second.first = dwWidth;
		kKey.second.second = dwHeight;

		auto it = m_ScaledImageMapByPtr.find(kKey);
		if (it != m_ScaledImageMapByPtr.end())
		{
			*ppImage = it->second;
			return true;
		}

		if (!pImage->GetTexturePointer())
		{
			TraceError("GetScaledImagePtr : image has no texture pointer");
			return false;
		}

		if (!pImage->GetTexturePointer()->GetD3DTexture())
		{
			TraceError("GetScaledImagePtr : TexturePointer has no D3DTexture");
			return false;
		}

		ILuint uImg;
		ilGenImages(1, &uImg);
		ilBindImage(uImg);
		ilEnable(IL_ORIGIN_SET);
		ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

		D3DLOCKED_RECT lockedRect;
		HRESULT hr = pImage->GetTexturePointer()->GetD3DTexture()->LockRect(0, &lockedRect, NULL, 0);
		if (hr != D3D_OK)
		{
			TraceError("Could not create meme from image [d3d error %u]", hr);
			ilDeleteImages(1, &uImg);
			return false;
		}

		DWORD* adwColorMap = new DWORD[pImage->GetWidth() * pImage->GetHeight()];
		// read colors
		for (DWORD y = 0; y < pImage->GetHeight(); ++y)
		{
			for (DWORD x = 0; x < pImage->GetWidth(); ++x)
			{
				DWORD y_reverse = pImage->GetHeight() - y - 1; // the image is whyever y-mirrored

				DWORD dwIndex = x * 4 + y * lockedRect.Pitch;
				adwColorMap[y_reverse * pImage->GetWidth() + x] = *(DWORD*)(&((BYTE*)lockedRect.pBits)[dwIndex]);
			}
		}

		pImage->GetTexturePointer()->GetD3DTexture()->UnlockRect(0);

		if (!ilTexImage(pImage->GetWidth(), pImage->GetHeight(), 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, adwColorMap))
		{
			delete[] adwColorMap;
			ilDeleteImages(1, &uImg);

			TraceError("Could not create meme from image [tex image error]");
			return false;
		}
		delete[] adwColorMap;

		BYTE* pbData = __ScaleCurrentImage(uImg, dwWidth, dwHeight);
		*ppImage = new CGraphicImage(pbData, dwWidth, dwHeight, D3DFMT_R8G8B8);
		delete[] pbData;

		m_ScaledImageMapByPtr[kKey] = *ppImage;
		return true;
	}

	BYTE* CWindowManager::__ScaleCurrentImage(ILuint& img, DWORD dwNewWidth, DWORD dwNewHeight, DWORD dwScaleFilter)
	{
		iluImageParameter(ILU_FILTER, dwScaleFilter);
		iluScale(dwNewWidth, dwNewHeight, 1);

		DWORD dwBufSize = dwNewWidth * dwNewHeight * 4;
		BYTE* pbImageBuffer = new BYTE[dwBufSize];
		ilCopyPixels(0, 0, 0, dwNewWidth, dwNewHeight, 1, IL_BGRA, IL_UNSIGNED_BYTE, pbImageBuffer);
		ilDeleteImages(1, &img);

		return pbImageBuffer;
	}
}
