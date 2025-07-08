#include "StdAfx.h"
#include "../eterBase/CRC32.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"
#include "PythonWindowManager.h"
#include "../eterLib/RenderTargetManager.h"

BOOL g_bOutlineBoxEnable = FALSE;

namespace UI
{
	
	CWindow::CWindow(PyObject * ppyObject) : 
		m_x(0),
		m_y(0),
		m_lWidth(0),
		m_lHeight(0),
		m_poHandler(ppyObject),
		m_bShow(false),
		m_pParent(NULL),
		m_dwFlag(0),
		m_isUpdatingChildren(FALSE),
		m_bSingleAlpha(false),
		m_fSingleAlpha(1.0f),
		m_fWindowAlpha(1.0f),
		m_bAllAlpha(false),
		m_fAllAlpha(1.0f),
		m_isInsideRender(false)
	{			
#ifdef _DEBUG
		static DWORD DEBUG_dwGlobalCounter=0;
		DEBUG_dwCounter=DEBUG_dwGlobalCounter++;	

		m_strName = "!!debug";
#endif
		//assert(m_poHandler != NULL);
		m_HorizontalAlign = HORIZONTAL_ALIGN_LEFT;
		m_VerticalAlign = VERTICAL_ALIGN_TOP;
		m_rect.bottom = m_rect.left = m_rect.right = m_rect.top = 0;
		m_limitBiasRect.bottom = m_limitBiasRect.left = m_limitBiasRect.right = m_limitBiasRect.top = 0;
		m_v2Scale.x = m_v2Scale.y = 1.0f;
		memset(&m_renderBox, 0, sizeof(m_renderBox));
	}

	CWindow::~CWindow()
	{
	}

	DWORD CWindow::Type()
	{
		static DWORD s_dwType = GetCRC32("CWindow", strlen("CWindow"));
		return (s_dwType);
	}

	BOOL CWindow::IsType(DWORD dwType)
	{
		return OnIsType(dwType);
	}

	BOOL CWindow::OnIsType(DWORD dwType)
	{
		if (CWindow::Type() == dwType)
			return TRUE;

		return FALSE;
	}

	struct FClear
	{
		void operator () (CWindow * pWin)
		{
			pWin->Clear();
		}
	};

	void CWindow::Clear()
	{
		// FIXME : Children을 즉시 Delete하지는 않는다.
		//         어차피 Python쪽에서 Destroy가 하나씩 다시 호출 될 것이므로..
		//         하지만 만약을 위해 링크는 끊어 놓는다.
		//         더 좋은 형태는 있는가? - [levites]
		std::for_each(m_pChildList.begin(), m_pChildList.end(), FClear());
		m_pChildList.clear();

		m_pParent = NULL;
		DestroyHandle();
		Hide();
	}

	void CWindow::DestroyHandle()
	{
		m_poHandler = NULL;
	}

	void CWindow::Show()
	{
		m_bShow = true;
	}

	void CWindow::Hide()
	{
		if (!m_bShow)
			return;

		m_bShow = false;

		OnHideWithChilds();
	}

	void CWindow::OnHideWithChilds()
	{
		OnHide();

		std::for_each(m_pChildList.begin(), m_pChildList.end(), std::void_mem_fun(&CWindow::OnHideWithChilds));
	}

	void CWindow::OnHide()
	{
		PyCallClassMemberFunc(m_poHandler, "OnHide", BuildEmptyTuple());
	}

	// NOTE : IsShow는 "자신이 보이는가?" 이지만, __IsShowing은 "자신이 그려지고 있는가?" 를 체크한다
	//        자신은 Show 지만 Tree 위쪽의 Parent 중 하나는 Hide 일 수 있으므로.. - [levites]
	bool CWindow::IsRendering()
	{
		if (!IsShow())
			return false;

		if (!m_pParent)
			return true;

		return m_pParent->IsRendering();
	}

	void CWindow::__RemoveReserveChildren()
	{
		if (m_pReserveChildList.empty())
			return;

		TWindowContainer::iterator it;
		for(it = m_pReserveChildList.begin(); it != m_pReserveChildList.end(); ++it)
		{
			m_pChildList.remove(*it);
		}
		m_pReserveChildList.clear();
	}

	void CWindow::SetSingleAlpha(float fAlpha)
	{
		m_bSingleAlpha = true;
		m_fSingleAlpha = fAlpha;

		if (m_bAllAlpha)
			SetAlpha(fAlpha * m_fAllAlpha);
		else
			SetAlpha(fAlpha);
	}

	void CWindow::SetAllAlpha(float fAlpha)
	{
		if (m_bSingleAlpha)
			return;

		if (m_bAllAlpha && m_fAllAlpha == fAlpha)
			return;

		m_bAllAlpha = true;
		m_fAllAlpha = fAlpha;

		if (m_bSingleAlpha)
			SetAlpha(fAlpha * m_fSingleAlpha);
		else
			SetAlpha(fAlpha);

		TWindowContainer::iterator it;
		for (it = m_pChildList.begin(); it != m_pChildList.end();)
		{
			TWindowContainer::iterator it_next = it;
			++it_next;
			(*it)->SetAllAlpha(fAlpha);
			it = it_next;
		}
	}

	void CWindow::Update()
	{
		if (!IsShow())
			return;

		__RemoveReserveChildren();

		OnUpdate();

		m_isUpdatingChildren = TRUE;
		TWindowContainer::iterator it;
		for(it = m_pChildList.begin(); it != m_pChildList.end();)
		{
			TWindowContainer::iterator it_next = it;
			++it_next;
			(*it)->Update();
			it = it_next;
		}
		m_isUpdatingChildren = FALSE;		
	}

	bool CWindow::IsShow()
	{
		if (!m_bShow)
			return false;

		if (m_isInsideRender)
			if (m_renderBox.left + m_renderBox.right >= m_lWidth || m_renderBox.top + m_renderBox.bottom >= m_lHeight)
				return false;

		return true;
	}

	void CWindow::Render()
	{
		if (!IsShow())
			return;

		OnRender();

		if (g_bOutlineBoxEnable)
		{
			CPythonGraphic::Instance().SetDiffuseColor(1.0f, 1.0f, 1.0f);
			CPythonGraphic::Instance().RenderBox2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
		}

		std::for_each(m_pChildList.begin(), m_pChildList.end(), std::void_mem_fun(&CWindow::Render));

		OnAfterRender();
	}

	void CWindow::OnUpdate()
	{
		if (!m_poHandler)
			return;

		if (!IsShow())
			return;

		static PyObject* poFuncName_OnUpdate = PyString_InternFromString("OnUpdate");

		//PyCallClassMemberFunc(m_poHandler, "OnUpdate", BuildEmptyTuple());
		PyCallClassMemberFunc_ByPyString(m_poHandler, poFuncName_OnUpdate, BuildEmptyTuple());
		
	}

	void CWindow::OnRender()
	{
		if (!m_poHandler)
			return;

		if (!IsShow())
			return;

		//PyCallClassMemberFunc(m_poHandler, "OnRender", BuildEmptyTuple());
		PyCallClassMemberFunc(m_poHandler, "OnRender", BuildEmptyTuple());
	}

	void CWindow::OnAfterRender()
	{
		if (!m_poHandler)
			return;

		if (!IsShow())
			return;

		PyCallClassMemberFunc(m_poHandler, "OnAfterRender", BuildEmptyTuple());
	}

	void CWindow::SetName(const char * c_szName)
	{
		m_strName = c_szName;
	}

	void CWindow::SetSize(long width, long height)
	{
		m_lWidth = width;
		m_lHeight = height;

		m_rect.right = m_rect.left + m_lWidth;
		m_rect.bottom = m_rect.top + m_lHeight;

		if (m_isInsideRender)
			UpdateRenderBoxRecursive();
	}

	void CWindow::SetHorizontalAlign(DWORD dwAlign)
	{
		m_HorizontalAlign = (EHorizontalAlign)dwAlign;
		UpdateRect();
	}

	void CWindow::SetVerticalAlign(DWORD dwAlign)
	{
		m_VerticalAlign = (EVerticalAlign)dwAlign;
		UpdateRect();
	}

	void CWindow::SetPosition(long x, long y)
	{
		m_x = x;
		m_y = y; 

		UpdateRect();
		if (m_isInsideRender)
			UpdateRenderBoxRecursive();
	}

	void CWindow::UpdateRenderBoxRecursive()
	{
		UpdateRenderBox();
		for (auto it = m_pChildList.begin(); it != m_pChildList.end(); ++it)
			(*it)->UpdateRenderBoxRecursive();
	}

	void CWindow::SetScale(float fx, float fy)
	{
		m_v2Scale.x = fx;
		m_v2Scale.y = fy;

		UpdateRect();
	}

	void CWindow::GetPosition(long * plx, long * ply)
	{
		*plx = m_x;
		*ply = m_y;
	}

	long CWindow::UpdateRect()
	{
		m_rect.top		= m_y;
		if (m_pParent)
		{
			switch (m_VerticalAlign)
			{
				case VERTICAL_ALIGN_BOTTOM:
					m_rect.top = m_pParent->GetHeight() - m_rect.top;
					break;
				case VERTICAL_ALIGN_CENTER:
					m_rect.top = (m_pParent->GetHeight() - GetHeight() * m_v2Scale.y) / 2 + m_rect.top;
					break;
			}
			m_rect.top += m_pParent->m_rect.top;
		}
		m_rect.bottom	= m_rect.top + m_lHeight * m_v2Scale.y;

#if defined( _USE_CPP_RTL_FLIP )
		if( m_pParent == NULL ) {
			m_rect.left		= m_x;
			m_rect.right	= m_rect.left + m_lWidth;
		} else {
			if( m_pParent->IsFlag(UI::CWindow::FLAG_RTL) == true ) {
				m_rect.left = m_pParent->GetWidth() - m_lWidth - m_x;
				switch (m_HorizontalAlign)
				{
					case HORIZONTAL_ALIGN_RIGHT:
						m_rect.left = - m_x;
						break;
					case HORIZONTAL_ALIGN_CENTER:
						m_rect.left = m_pParent->GetWidth() / 2 - GetWidth() - m_x;
						break;
				}
				m_rect.left += m_pParent->m_rect.left;
				m_rect.right = m_rect.left + m_lWidth;
			} else {
				m_rect.left		= m_x;
				switch (m_HorizontalAlign)
				{
					case HORIZONTAL_ALIGN_RIGHT:
						m_rect.left = m_pParent->GetWidth() - m_rect.left;
						break;
					case HORIZONTAL_ALIGN_CENTER:
						m_rect.left = (m_pParent->GetWidth() - GetWidth()) / 2 + m_rect.left;
						break;
				}
				m_rect.left += m_pParent->m_rect.left;
				m_rect.right = m_rect.left + m_lWidth;
			}
		}
#else
		m_rect.left		= m_x;
		if (m_pParent)
		{
			switch (m_HorizontalAlign)
			{
				case HORIZONTAL_ALIGN_RIGHT:
					m_rect.left = ::abs(m_pParent->GetWidth()) - m_rect.left;
					break;
				case HORIZONTAL_ALIGN_CENTER:
					m_rect.left = m_pParent->GetWidth() / 2 - GetWidth() * m_v2Scale.x / 2 + m_rect.left;
					break;
			}
			m_rect.left += 0L < m_pParent->GetWidth() ? m_pParent->m_rect.left : m_pParent->m_rect.right + ::abs(m_pParent->GetWidth());
		}
		m_rect.right = m_rect.left + m_lWidth * m_v2Scale.x;
#endif
		std::for_each(m_pChildList.begin(), m_pChildList.end(), std::mem_fun(&CWindow::UpdateRect));

		OnChangePosition();

		return 1;
	}

	void CWindow::GetLocalPosition(long & rlx, long & rly)
	{
		rlx = rlx - m_rect.left;
		rly = rly - m_rect.top;
	}

	void CWindow::GetMouseLocalPosition(long & rlx, long & rly)
	{
		CWindowManager::Instance().GetMousePosition(rlx, rly);
		rlx = rlx - m_rect.left;
		rly = rly - m_rect.top;
	}

	void CWindow::AddChild(CWindow * pWin)
	{
		m_pChildList.push_back(pWin);
		pWin->m_pParent = this;

		if (m_isInsideRender && !pWin->m_isInsideRender)
			pWin->SetInsideRender(m_isInsideRender);
	}

	void CWindow::SetInsideRender(BOOL flag)
	{
		if (!m_pParent || m_isInsideRender && m_pParent->m_isInsideRender)
			return;

		if (m_isInsideRender && flag)
			return;

		m_isInsideRender = flag;
		UpdateRenderBox();
		for (auto it = m_pChildList.begin(); it != m_pChildList.end(); ++it)
			(*it)->SetInsideRender(m_isInsideRender);
	}

	void CWindow::GetRenderBox(RECT* box)
	{
		memcpy(box, &m_renderBox, sizeof(RECT));
	}

	void CWindow::UpdateRenderBox()
	{
		if (!m_isInsideRender || !m_pParent)
			memset(&m_renderBox, 0, sizeof(m_renderBox));
		else
		{
			int width = m_lWidth;
			int height = m_lHeight;
			int pWidth = m_pParent->GetWidth();
			int pHeight = m_pParent->GetHeight();

			if (IsType(CTextLine::Type()))
				((CTextLine*)this)->GetTextSize(&width, &height);
			if (m_pParent->IsType(CTextLine::Type()))
				((CTextLine*)m_pParent)->GetTextSize(&pWidth, &pHeight);

			if (m_x - m_pParent->m_renderBox.left < 0)
				m_renderBox.left = -m_x + m_pParent->m_renderBox.left;
			else
				m_renderBox.left = 0;

			if (m_y - m_pParent->m_renderBox.top < 0)
				m_renderBox.top = -m_y + m_pParent->m_renderBox.top;
			else
				m_renderBox.top = 0;

			if (m_x + width > pWidth - m_pParent->m_renderBox.right)
				m_renderBox.right = m_x + width - pWidth + m_pParent->m_renderBox.right;
			else
				m_renderBox.right = 0;

			if (m_y + height > pHeight - m_pParent->m_renderBox.bottom)
				m_renderBox.bottom = m_y + height - pHeight + m_pParent->m_renderBox.bottom;
			else
				m_renderBox.bottom = 0;
		}

		OnUpdateRenderBox();
	}

	CWindow * CWindow::GetRoot()
	{
		if (m_pParent)
			if (m_pParent->IsWindow())
				return m_pParent->GetRoot();

		return this;
	}

	CWindow * CWindow::GetParent()
	{
		return m_pParent;
	}

	bool CWindow::IsChild(CWindow * pWin, bool bCheckRecursive)
	{
		std::list<CWindow *>::iterator itor = m_pChildList.begin();

		while (itor != m_pChildList.end())
		{
			if (*itor == pWin)
				return true;

			if (bCheckRecursive)
			{
				if ((*itor)->IsChild(pWin, true))
					return true;
			}

			++itor;
		}

		return false;
	}

	void CWindow::DeleteChild(CWindow * pWin)
	{
		if (m_isUpdatingChildren)
		{
			m_pReserveChildList.push_back(pWin);
		}
		else
		{
			m_pChildList.remove(pWin);
		}
	}

	void CWindow::SetTop(CWindow * pWin)
	{
		if (!pWin->IsFlag(CWindow::FLAG_FLOAT))
			return;

		TWindowContainer::iterator itor = std::find(m_pChildList.begin(), m_pChildList.end(), pWin);
		if (m_pChildList.end() != itor)
		{
			m_pChildList.push_back(*itor);
			m_pChildList.erase(itor);

			pWin->OnTop();
		}
		else
		{
			TraceError(" CWindow::SetTop - Failed to find child window\n");
		}
	}

	void CWindow::OnMouseDrag(long lx, long ly)
	{
		PyCallClassMemberFunc(m_poHandler, "OnMouseDrag", Py_BuildValue("(ii)", lx, ly));
	}

	void CWindow::OnMoveWindow(long lx, long ly)
	{
		PyCallClassMemberFunc(m_poHandler, "OnMoveWindow", Py_BuildValue("(ii)", lx, ly));
	}

	void CWindow::OnSetFocus()
	{
		//PyCallClassMemberFunc(m_poHandler, "OnSetFocus", BuildEmptyTuple());
		PyCallClassMemberFunc(m_poHandler, "OnSetFocus", BuildEmptyTuple());
	}

	void CWindow::OnKillFocus()
	{
		PyCallClassMemberFunc(m_poHandler, "OnKillFocus", BuildEmptyTuple());
	}

	void CWindow::OnMouseOverIn()
	{
		PyCallClassMemberFunc(m_poHandler, "OnMouseOverIn", BuildEmptyTuple());
	}

	void CWindow::OnMouseOverOut()
	{
		PyCallClassMemberFunc(m_poHandler, "OnMouseOverOut", BuildEmptyTuple());
	}

	void CWindow::OnMouseOver()
	{
	}

	void CWindow::OnDrop()
	{
		PyCallClassMemberFunc(m_poHandler, "OnDrop", BuildEmptyTuple());
	}

	void CWindow::OnTop()
	{
		PyCallClassMemberFunc(m_poHandler, "OnTop", BuildEmptyTuple());
	}

	void CWindow::OnIMEUpdate()
	{
		PyCallClassMemberFunc(m_poHandler, "OnIMEUpdate", BuildEmptyTuple());
	}

	BOOL CWindow::RunIMETabEvent()
	{
		if (!IsRendering())
			return FALSE;

		if (OnIMETabEvent())
			return TRUE;

		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->RunIMETabEvent())
				return TRUE;
		}

		return FALSE;
	}

	BOOL CWindow::RunIMEReturnEvent()
	{
		if (!IsRendering())
			return FALSE;

		if (OnIMEReturnEvent())
			return TRUE;

		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->RunIMEReturnEvent())
				return TRUE;
		}

		return FALSE;
	}

	BOOL CWindow::RunIMEKeyDownEvent(int ikey)
	{
		if (!IsRendering())
			return FALSE;

		if (OnIMEKeyDownEvent(ikey))
			return TRUE;

		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->RunIMEKeyDownEvent(ikey))
				return TRUE;
		}

		return FALSE;
	}

	CWindow * CWindow::RunKeyDownEvent(int ikey)
	{
		if (OnKeyDown(ikey))
			return this;

		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->IsShow())
			{
				CWindow * pProcessedWindow = pWindow->RunKeyDownEvent(ikey);
				if (NULL != pProcessedWindow)
				{
					return pProcessedWindow;
				}
			}
		}

		return NULL;
	}

	BOOL CWindow::RunKeyUpEvent(int ikey)
	{
		if (OnKeyUp(ikey))
			return TRUE;

		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->IsShow())
			if (pWindow->RunKeyUpEvent(ikey))
				return TRUE;
		}

		return FALSE;
	}

	BOOL CWindow::RunPressReturnKeyEvent()
	{
		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->IsShow())
				if (pWindow->RunPressReturnKeyEvent())
					return TRUE;
		}

		if (OnPressReturnKey())
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::RunPressEscapeKeyEvent()
	{
		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->IsShow())
			if (pWindow->RunPressEscapeKeyEvent())
				return TRUE;
		}

		if (OnPressEscapeKey())
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::RunPressExitKeyEvent()
	{
		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->RunPressExitKeyEvent())
				return TRUE;

			if (pWindow->IsShow())
			if (pWindow->OnPressExitKey())
				return TRUE;
		}

		return FALSE;
	}

	BOOL CWindow::OnMouseLeftButtonDown()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseLeftButtonDown", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseLeftButtonUp()
	{
		PyCallClassMemberFunc(m_poHandler, "OnMouseLeftButtonUp", BuildEmptyTuple());
		return TRUE; // NOTE : ButtonUp은 예외로 무조건 TRUE
	}

	BOOL CWindow::OnMouseLeftButtonDoubleClick()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseLeftButtonDoubleClick", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseRightButtonDown()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseRightButtonDown", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseRightButtonUp()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseRightButtonUp", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseRightButtonDoubleClick()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseRightButtonDoubleClick", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseMiddleButtonDown()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseMiddleButtonDown", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseMiddleButtonUp()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseMiddleButtonUp", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::RunMouseWheelEvent(int iLen)
	{
		if (!IsRendering())
			return FALSE;


		TWindowContainer::reverse_iterator itor;
		for (itor = m_pChildList.rbegin(); itor != m_pChildList.rend(); ++itor)
		{
			CWindow * pWindow = *itor;

			if (pWindow->RunMouseWheelEvent(iLen))
				return TRUE;
		}

		if (OnMouseWheel(iLen))
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnMouseWheel(int iLen)
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnMouseWheel", Py_BuildValue("(i)", iLen), &lValue))
			if (0 != lValue)
				return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMETabEvent()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMETab", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMEReturnEvent()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMEReturn", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMEKeyDownEvent(int ikey)
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMEKeyDown", Py_BuildValue("(i)", ikey), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMEChangeCodePage()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMEChangeCodePage", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMEOpenCandidateListEvent()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMEOpenCandidateList", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMECloseCandidateListEvent()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMECloseCandidateList", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMEOpenReadingWndEvent()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMEOpenReadingWnd", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnIMECloseReadingWndEvent()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnIMECloseReadingWnd", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnKeyDown(int ikey)
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnKeyDown", Py_BuildValue("(i)", ikey), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnKeyUp(int ikey)
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnKeyUp", Py_BuildValue("(i)", ikey), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnPressReturnKey()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnPressReturnKey", BuildEmptyTuple(), &lValue))
			if (0 != lValue)
				return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnPressEscapeKey()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnPressEscapeKey", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	BOOL CWindow::OnPressExitKey()
	{
		long lValue;
		if (PyCallClassMemberFunc(m_poHandler, "OnPressExitKey", BuildEmptyTuple(), &lValue))
		if (0 != lValue)
			return TRUE;

		return FALSE;
	}

	/////

	bool CWindow::IsIn(long x, long y)
	{
		if (x >= m_rect.left && x <= m_rect.right)
			if (y >= m_rect.top && y <= m_rect.bottom)
				return true;

		return false;
	}

	bool CWindow::IsIn()
	{
		long lx, ly;
		UI::CWindowManager::Instance().GetMousePosition(lx, ly);

		return IsIn(lx, ly);
	}

	CWindow * CWindow::PickWindow(long x, long y)
	{
		std::list<CWindow *>::reverse_iterator ritor = m_pChildList.rbegin();
		for (; ritor != m_pChildList.rend(); ++ritor)
		{
			CWindow * pWin = *ritor;
			if (pWin->IsShow())
			{
				if (!pWin->IsFlag(CWindow::FLAG_IGNORE_SIZE))
				{
					if (!pWin->IsIn(x, y)) {
						if (0L <= pWin->GetWidth()) {
							continue;
						}
					}
				}

				CWindow * pResult = pWin->PickWindow(x, y);
				if (pResult)
					return pResult;
			}
		}

		if (IsFlag(CWindow::FLAG_NOT_PICK))
			return NULL;

		return (this);
	}

	CWindow * CWindow::PickTopWindow(long x, long y)
	{
		std::list<CWindow *>::reverse_iterator ritor = m_pChildList.rbegin();
		for (; ritor != m_pChildList.rend(); ++ritor)
		{
			CWindow * pWin = *ritor;
			if (pWin->IsShow())
				if (pWin->IsIn(x, y))
					if (!pWin->IsFlag(CWindow::FLAG_NOT_PICK))
						return pWin;
		}

		return NULL;
	}

	void CWindow::iSetRenderingRect(int iLeft, int iTop, int iRight, int iBottom)
	{
		m_renderingRect.left = iLeft;
		m_renderingRect.top = iTop;
		m_renderingRect.right = iRight;
		m_renderingRect.bottom = iBottom;

		OnSetRenderingRect();
	}

	void CWindow::SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom)
	{
		float fWidth = float(GetWidth());
		float fHeight = float(GetHeight());
		if (IsType(CTextLine::Type()))
		{
			int iWidth, iHeight;
			((CTextLine*)this)->GetTextSize(&iWidth, &iHeight);
			fWidth = float(iWidth);
			fHeight = float(iHeight);
		}
		/*else if (IsType(CNumberLine::Type()))
		{
			CNumberLine* pThis = (CNumberLine*) this;
			fWidth = float(pThis->GetWidthSummary());
			fHeight = float(pThis->GetMaxHeight());
		}*/

		iSetRenderingRect(fWidth * fLeft, fHeight * fTop, fWidth * fRight, fHeight * fBottom);
	}

	int CWindow::GetRenderingWidth()
	{
		return max(0, GetWidth() + m_renderingRect.right + m_renderingRect.left);
	}

	int CWindow::GetRenderingHeight()
	{
		return max(0, GetHeight() + m_renderingRect.bottom + m_renderingRect.top);
	}

	void CWindow::ResetRenderingRect(bool bCallEvent)
	{
		m_renderingRect.bottom = m_renderingRect.left = m_renderingRect.right = m_renderingRect.top = 0;

		if (bCallEvent)
			OnSetRenderingRect();
	}

	void CWindow::OnSetRenderingRect()
	{
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CBox::CBox(PyObject * ppyObject) : CWindow(ppyObject), m_dwColor(0xff000000)
	{
	}
	CBox::~CBox()
	{
	}

	void CBox::SetColor(DWORD dwColor)
	{
		m_dwColor = dwColor;
	}

	void CBox::OnRender()
	{
		CPythonGraphic::Instance().SetDiffuseColor(m_dwColor);
		CPythonGraphic::Instance().RenderBox2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CBar::CBar(PyObject * ppyObject) : CWindow(ppyObject), m_dwColor(0xff000000)
	{
	}
	CBar::~CBar()
	{
	}

	void CBar::SetColor(DWORD dwColor)
	{
		m_dwColor = dwColor;
	}

	void CBar::OnRender()
	{
		CPythonGraphic::Instance().SetDiffuseColor(m_dwColor);
		CPythonGraphic::Instance().RenderBar2d(m_rect.left + m_renderBox.left, m_rect.top + m_renderBox.top, m_rect.right - m_renderBox.right, m_rect.bottom - m_renderBox.bottom);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CLine::CLine(PyObject * ppyObject) : CWindow(ppyObject), m_dwColor(0xff000000)
	{
	}
	CLine::~CLine()
	{
	}

	void CLine::SetColor(DWORD dwColor)
	{
		m_dwColor = dwColor;
	}

	void CLine::OnRender()
	{
		CPythonGraphic & rkpyGraphic = CPythonGraphic::Instance();
		rkpyGraphic.SetDiffuseColor(m_dwColor);
		rkpyGraphic.RenderLine2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DWORD CBar3D::Type()
	{
		static DWORD s_dwType = GetCRC32("CBar3D", strlen("CBar3D"));
		return (s_dwType);
	}

	CBar3D::CBar3D(PyObject * ppyObject) : CWindow(ppyObject)
	{
		m_dwLeftColor = D3DXCOLOR(0.2f, 0.2f, 0.2f, 1.0f);
		m_dwRightColor = D3DXCOLOR(0.7f, 0.7f, 0.7f, 1.0f);
		m_dwCenterColor = D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	}
	CBar3D::~CBar3D()
	{
	}

	void CBar3D::SetColor(DWORD dwLeft, DWORD dwRight, DWORD dwCenter)
	{
		m_dwLeftColor = dwLeft;
		m_dwRightColor = dwRight;
		m_dwCenterColor = dwCenter;
	}

	void CBar3D::OnRender()
	{
		CPythonGraphic & rkpyGraphic = CPythonGraphic::Instance();

		rkpyGraphic.SetDiffuseColor(m_dwCenterColor);
		rkpyGraphic.RenderBar2d(m_rect.left, m_rect.top, m_rect.right, m_rect.bottom);

		rkpyGraphic.SetDiffuseColor(m_dwLeftColor);
		rkpyGraphic.RenderLine2d(m_rect.left, m_rect.top, m_rect.right, m_rect.top);
		rkpyGraphic.RenderLine2d(m_rect.left, m_rect.top, m_rect.left, m_rect.bottom);

		rkpyGraphic.SetDiffuseColor(m_dwRightColor);
		rkpyGraphic.RenderLine2d(m_rect.left, m_rect.bottom, m_rect.right, m_rect.bottom);
		rkpyGraphic.RenderLine2d(m_rect.right, m_rect.top, m_rect.right, m_rect.bottom);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DWORD CTextLine::Type()
	{
		static DWORD s_dwType = GetCRC32("CTextLine", strlen("CTextLine"));
		return (s_dwType);
	}

	BOOL CTextLine::OnIsType(DWORD dwType)
	{
		if (CTextLine::Type() == dwType)
			return TRUE;

		return FALSE;
	}

	CTextLine::CTextLine(PyObject * ppyObject) : CWindow(ppyObject)
	{
		m_TextInstance.SetColor(0.78f, 0.78f, 0.78f);
		m_TextInstance.SetHorizonalAlign(CGraphicTextInstance::HORIZONTAL_ALIGN_LEFT);
		m_TextInstance.SetVerticalAlign(CGraphicTextInstance::VERTICAL_ALIGN_TOP);
#ifdef __ARABIC_LANG__
		m_inverse = false;
#endif
	}
	CTextLine::~CTextLine()
	{
		m_TextInstance.Destroy();
	}

	void CTextLine::SetMax(int iMax)
	{
		m_TextInstance.SetMax(iMax);
	}
	void CTextLine::SetHorizontalAlign(int iType)
	{
		m_TextInstance.SetHorizonalAlign(iType);
	}
	void CTextLine::SetVerticalAlign(int iType)
	{
		m_TextInstance.SetVerticalAlign(iType);
	}
	void CTextLine::SetSecret(BOOL bFlag)
	{
		m_TextInstance.SetSecret(bFlag ? true : false);
	}
	void CTextLine::SetOutline(BOOL bFlag)
	{
		m_TextInstance.SetOutline(bFlag ? true : false);
	}
	void CTextLine::SetOutlineColor(DWORD dwColor)
	{
		m_TextInstance.SetOutLineColor(dwColor);
	}
	void CTextLine::SetFeather(BOOL bFlag)
	{
		m_TextInstance.SetFeather(bFlag ? true : false);
	}
	void CTextLine::SetMultiLine(BOOL bFlag)
	{
		m_TextInstance.SetMultiLine(bFlag ? true : false);
	}
	void CTextLine::SetFontName(const char * c_szFontName)
	{
		std::string stFontName = c_szFontName;
		stFontName += ".fnt";
		
		CResourceManager& rkResMgr=CResourceManager::Instance();
		CResource* pkRes = rkResMgr.GetTypeResourcePointer(stFontName.c_str());
		CGraphicText* pkResFont=static_cast<CGraphicText*>(pkRes);
		m_TextInstance.SetTextPointer(pkResFont);
	}
	void CTextLine::SetFontColor(DWORD dwColor)
	{
		m_TextInstance.SetColor(dwColor);
	}
	void CTextLine::SetLimitWidth(float fWidth)
	{
		m_TextInstance.SetLimitWidth(fWidth);
	}
	void CTextLine::SetText(const char * c_szText)
	{
		OnSetText(c_szText);
	}
	void CTextLine::GetTextSize(int* pnWidth, int* pnHeight)
	{
		m_TextInstance.GetTextSize(pnWidth, pnHeight);
	}
	const char * CTextLine::GetText()
	{
		return m_TextInstance.GetValueStringReference().c_str();
	}
	void CTextLine::ShowCursor()
	{
		m_TextInstance.ShowCursor();
	}
	void CTextLine::HideCursor()
	{
		m_TextInstance.HideCursor();
	}
	bool CTextLine::IsShowCursor()
	{
		return m_TextInstance.IsShowCursor();
	}
	int CTextLine::GetCursorPosition()
	{
		long lx, ly;
		CWindow::GetMouseLocalPosition(lx, ly);
		return m_TextInstance.PixelPositionToCharacterPosition(lx);
	}

	void CTextLine::OnSetText(const char * c_szText)
	{
		m_TextInstance.SetValue(c_szText);
		m_TextInstance.Update();
		/*int width, height;
		m_TextInstance.GetTextSize(&width, &height);
		SetSize(width, height);*/

		if (m_isInsideRender)
			UpdateRenderBoxRecursive();
	}

	bool CTextLine::IsShow()
	{
		if (!m_bShow)
			return false;

		if (m_isInsideRender)
		{
			int cW, cH;
			GetTextSize(&cW, &cH);
			if (m_renderBox.left + m_renderBox.right >= cW || m_renderBox.top + m_renderBox.bottom >= cH)
				return false;
		}

		return true;
	}

	void CTextLine::OnUpdate()
	{
		if (IsShow())
			m_TextInstance.Update();
	}
	void CTextLine::OnRender()
	{
		if (IsShow())
			m_TextInstance.Render();
	}

	void CTextLine::OnChangePosition()
	{
		// FOR_ARABIC_ALIGN
		//if (m_TextInstance.GetHorizontalAlign() == CGraphicTextInstance::HORIZONTAL_ALIGN_ARABIC)
		if( GetDefaultCodePage() == CP_ARABIC )
		{
#ifdef __ARABIC_LANG__
			m_TextInstance.SetPosition(m_inverse ? m_rect.right : m_rect.left, m_rect.top);
#else
			m_TextInstance.SetPosition(m_rect.right, m_rect.top);
#endif
		}
		else
		{
			m_TextInstance.SetPosition(m_rect.left, m_rect.top);
		}
	}

#ifdef __ARABIC_LANG__
	void CTextLine::SetInverse()
	{
		m_TextInstance.SetInverse();
		m_inverse = true;
	}
#endif

	int CTextLine::GetRenderingWidth()
	{
		int iTextWidth, iTextHeight;
		GetTextSize(&iTextWidth, &iTextHeight);

		return iTextWidth + m_renderingRect.right + m_renderingRect.left;
	}

	int CTextLine::GetRenderingHeight()
	{
		int iTextWidth, iTextHeight;
		GetTextSize(&iTextWidth, &iTextHeight);

		return iTextHeight + m_renderingRect.bottom + m_renderingRect.top;
	}

	void CTextLine::OnSetRenderingRect()
	{
		int iTextWidth, iTextHeight;
		GetTextSize(&iTextWidth, &iTextHeight);

		m_TextInstance.iSetRenderingRect(m_renderingRect.left, -m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
	}

	void CTextLine::SetAlpha(float fAlpha)
	{
		D3DXCOLOR kColor = m_TextInstance.GetColor();
		kColor.a = fAlpha;

		m_TextInstance.SetColor(kColor);
	}
	float CTextLine::GetAlpha() const
	{
		D3DXCOLOR kColor = m_TextInstance.GetColor();
		return kColor.a;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CNumberLine::CNumberLine(PyObject * ppyObject) : CWindow(ppyObject)
	{
		m_strPath = "d:/ymir work/ui/game/taskbar/";
		m_iHorizontalAlign = HORIZONTAL_ALIGN_LEFT;
		m_dwWidthSummary = 0;
	}
	CNumberLine::CNumberLine(CWindow * pParent) : CWindow(NULL)
	{
		m_strPath = "d:/ymir work/ui/game/taskbar/";
		m_iHorizontalAlign = HORIZONTAL_ALIGN_LEFT;
		m_dwWidthSummary = 0;

		m_pParent = pParent;
	}
	CNumberLine::~CNumberLine()
	{
		ClearNumber();
	}

	void CNumberLine::SetPath(const char * c_szPath)
	{
		m_strPath = c_szPath;
	}
	void CNumberLine::SetHorizontalAlign(int iType)
	{
		m_iHorizontalAlign = iType;
	}
	void CNumberLine::SetNumber(const char * c_szNumber)
	{
		if (0 == m_strNumber.compare(c_szNumber))
			return;

		ClearNumber();

		m_strNumber = c_szNumber;

		for (DWORD i = 0; i < m_strNumber.size(); ++i)
		{
			char cChar = m_strNumber[i];
			std::string strImageFileName;

			if (':' == cChar)
			{
				strImageFileName = m_strPath + "colon.sub";
			}
			else if ('?' == cChar)
			{
				strImageFileName = m_strPath + "questionmark.sub";
			}
			else if ('/' == cChar)
			{
				strImageFileName = m_strPath + "slash.sub";
			}
			else if ('%' == cChar)
			{
				strImageFileName = m_strPath + "percent.sub";
			}
			else if ('+' == cChar)
			{
				strImageFileName = m_strPath + "plus.sub";
			}
			else if ('.' == cChar)
			{
				strImageFileName = m_strPath + "dot.sub";
			}
			else if ('m' == cChar)
			{
				strImageFileName = m_strPath + "m.sub";
			}
			else if ('g' == cChar)
			{
				strImageFileName = m_strPath + "g.sub";
			}
			else if ('p' == cChar)
			{
				strImageFileName = m_strPath + "p.sub";
			}
			else if ('L' == cChar || 'l' == cChar)
			{
				strImageFileName = m_strPath + "l.sub";
			}
			else if ('v' == cChar)
			{
				strImageFileName = m_strPath + "v.sub";
			}
			else if (cChar >= '0' && cChar <= '9')
			{
				strImageFileName = m_strPath;
				strImageFileName += cChar;
				strImageFileName += ".sub";
			}
			else
				continue;

			if (!CResourceManager::Instance().IsFileExist(strImageFileName.c_str()))
			{
				TraceError("cannot display character %c", cChar);
				continue;
			}

			CGraphicImage * pImage = (CGraphicImage *)CResourceManager::Instance().GetResourcePointer(strImageFileName.c_str());

			CGraphicImageInstance * pInstance = CGraphicImageInstance::New();
			pInstance->SetImagePointer(pImage);
			m_ImageInstanceVector.push_back(pInstance);

			m_dwWidthSummary += pInstance->GetWidth();
		}
	}

	void CNumberLine::ClearNumber()
	{
		m_ImageInstanceVector.clear();
		m_dwWidthSummary = 0;
		m_strNumber = "";
	}

	void CNumberLine::OnRender()
	{
		for (DWORD i = 0; i < m_ImageInstanceVector.size(); ++i)
		{
			m_ImageInstanceVector[i]->Render();
		}
	}

	void CNumberLine::OnChangePosition()
	{
		int ix = m_x;
		int iy = m_y;

		if (m_pParent)
		{
			ix = m_rect.left;
			iy = m_rect.top;
		}

		switch (m_iHorizontalAlign)
		{
			case HORIZONTAL_ALIGN_LEFT:
				break;
			case HORIZONTAL_ALIGN_CENTER:
				ix -= int(m_dwWidthSummary) / 2;
				break;
			case HORIZONTAL_ALIGN_RIGHT:
				ix -= int(m_dwWidthSummary);
				break;
		}

		for (DWORD i = 0; i < m_ImageInstanceVector.size(); ++i)
		{
			m_ImageInstanceVector[i]->SetPosition(ix, iy);
			ix += m_ImageInstanceVector[i]->GetWidth();
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	static map<int, const char*> imagesMap;

	DWORD CImageBox::Type()
	{
		static DWORD s_dwType = GetCRC32("CImageBox", strlen("CImageBox"));
		return (s_dwType);
	}

	BOOL CImageBox::OnIsType(DWORD dwType)
	{
		if (CImageBox::Type() == dwType)
			return TRUE;

		return FALSE;
	}

	CImageBox::CImageBox(PyObject * ppyObject) : CWindow(ppyObject), m_fAlpha(1.0f)
	{
		m_pImageInstance = NULL;
		m_bIsShowImage = true;
		m_fCoolTimeStart = 0.0f;
		m_fCoolTimeDuration = 0.0f;
		m_fDisplayProcent = -1.0f;
	}
	CImageBox::~CImageBox()
	{
		auto key = (int)std::addressof(m_pImageInstance);
		imagesMap.erase(key);

		OnDestroyInstance();
	}

	void CImageBox::OnCreateInstance()
	{
		OnDestroyInstance();
		
		m_pImageInstance = CGraphicImageInstance::New();
	}
	void CImageBox::OnDestroyInstance()
	{
		if (m_pImageInstance)
		{
			CGraphicImageInstance::Delete(m_pImageInstance);
			m_pImageInstance=NULL;
		}
	}

	void CImageBox::SetCoolTime(float fDuration)
	{
		m_fCoolTimeStart = CTimer::Instance().GetCurrentSecond();
		m_fCoolTimeDuration = fDuration;
	}

	BOOL CImageBox::LoadImage(const char * c_szFileName)
	{
		if (!c_szFileName[0])
			return FALSE;

		OnCreateInstance();

		CResource * pResource = CResourceManager::Instance().GetResourcePointer(c_szFileName);
		if (!pResource)
			return FALSE;
		if (!pResource->IsType(CGraphicImage::Type()))
			return FALSE;

		m_pImageInstance->SetImagePointer(static_cast<CGraphicImage*>(pResource));
		if (m_pImageInstance->IsEmpty())
			return FALSE;

		SetSize(m_pImageInstance->GetWidth(), m_pImageInstance->GetHeight());
		UpdateRect();

		auto key = (int)std::addressof(m_pImageInstance);
		imagesMap[key] = c_szFileName;

		return TRUE;
	}

	BOOL CImageBox::LoadImage(CGraphicImage* pImagePtr)
	{
		if (!pImagePtr)
			return FALSE;

		OnCreateInstance();

		m_pImageInstance->SetImagePointer(pImagePtr);
		if (m_pImageInstance->IsEmpty())
			return FALSE;

		SetSize(m_pImageInstance->GetWidth(), m_pImageInstance->GetHeight());
		UpdateRect();

		auto key = (int)std::addressof(pImagePtr);
		imagesMap[key] = pImagePtr->GetFileName();

		return TRUE;
	}

	void CImageBox::SetDiffuseColor(float fr, float fg, float fb, float fa)
	{
		if (!m_pImageInstance)
			return;

		m_fAlpha = fa;
		m_pImageInstance->SetDiffuseColor(fr, fg, fb, fa);
	}

	void CImageBox::SetScale(float fx, float fy)
	{
		if (!m_pImageInstance)
			return;

		((CGraphicImageInstance*)m_pImageInstance)->SetScale(fx, fy);
		CWindow::SetScale(fx, fy);
	}

	int CImageBox::GetWidth()
	{
		if (!m_pImageInstance)
			return 0;

		return m_pImageInstance->GetWidth();
	}

	int CImageBox::GetHeight()
	{
		if (!m_pImageInstance)
			return 0;

		return m_pImageInstance->GetHeight();
	}

	void CImageBox::SetAlpha(float fAlpha)
	{
		SetDiffuseColor(1.0, 1.0, 1.0, fAlpha);
	}

	float CImageBox::GetAlpha() const
	{
		return m_fAlpha;
	}

	void CImageBox::OnUpdate()
	{
		CWindow::OnUpdate();
	}

	void CImageBox::OnRender()
	{
		if (!m_pImageInstance)
			return;

		if (IsShow() && m_bIsShowImage)
		{
			if (m_fDisplayProcent != -1.0f)
				m_pImageInstance->DisplayImageProcent(m_fDisplayProcent);
			else
				m_pImageInstance->Render();

			if (m_fCoolTimeStart != 0.0f)
			{
				float fPercentage = (CTimer::Instance().GetCurrentSecond() - m_fCoolTimeStart) / m_fCoolTimeDuration;
				CPythonGraphic::Instance().RenderCoolTimeBox(m_rect.left + CWindow::GetWidth() / 2.0f, m_rect.top + CWindow::GetHeight() / 2.0f,
					(CWindow::GetWidth() + CWindow::GetHeight()) / 4.0f, fPercentage);
			}

			CWindow::OnRender();
		}
	}
	void CImageBox::OnChangePosition()
	{
		if (!m_pImageInstance)
			return;

		m_pImageInstance->SetPosition(m_rect.left, m_rect.top);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// MarkBox - 마크 출력용 UI 윈도우
	///////////////////////////////////////////////////////////////////////////////////////////////
	CMarkBox::CMarkBox(PyObject * ppyObject) : CWindow(ppyObject)
	{
		m_pMarkInstance = NULL;
	}

	CMarkBox::~CMarkBox()
	{
		OnDestroyInstance();
	}

	void CMarkBox::OnCreateInstance()
	{
		OnDestroyInstance();
		m_pMarkInstance = CGraphicMarkInstance::New();
	}

	void CMarkBox::OnDestroyInstance()
	{
		if (m_pMarkInstance)
		{
			CGraphicMarkInstance::Delete(m_pMarkInstance);
			m_pMarkInstance=NULL;
		}
	}

	void CMarkBox::LoadImage(const char * c_szFilename)
	{
		OnCreateInstance();
		
		m_pMarkInstance->SetImageFileName(c_szFilename);
		m_pMarkInstance->Load();
		SetSize(m_pMarkInstance->GetWidth(), m_pMarkInstance->GetHeight());

		UpdateRect();
	}

	void CMarkBox::SetScale(FLOAT fScale)
	{
		if (!m_pMarkInstance)
			return;

		m_pMarkInstance->SetScale(fScale);
	}

	void CMarkBox::SetIndex(UINT uIndex)
	{
		if (!m_pMarkInstance)
			return;

		m_pMarkInstance->SetIndex(uIndex);
	}

	void CMarkBox::SetDiffuseColor(float fr, float fg, float fb, float fa)
	{
		if (!m_pMarkInstance)
			return;

		m_pMarkInstance->SetDiffuseColor(fr, fg, fb, fa);
	}

	void CMarkBox::OnUpdate()
	{
	}

	void CMarkBox::OnRender()
	{
		if (!m_pMarkInstance)
			return;

		if (IsShow())
			m_pMarkInstance->Render();
	}

	void CMarkBox::OnChangePosition()
	{
		if (!m_pMarkInstance)
			return;

		m_pMarkInstance->SetPosition(m_rect.left, m_rect.top);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DWORD CExpandedImageBox::Type()
	{
		static DWORD s_dwType = GetCRC32("CExpandedImageBox", strlen("CExpandedImageBox"));
		return (s_dwType);
	}

	BOOL CExpandedImageBox::OnIsType(DWORD dwType)
	{
		if (CExpandedImageBox::Type() == dwType)
			return TRUE;

		return FALSE;
	}

	CExpandedImageBox::CExpandedImageBox(PyObject * ppyObject) : CImageBox(ppyObject)
	{
	}
	CExpandedImageBox::~CExpandedImageBox()
	{
		OnDestroyInstance();
	}

	void CExpandedImageBox::OnCreateInstance()
	{
		OnDestroyInstance();

		m_pImageInstance = CGraphicExpandedImageInstance::New();
	}
	void CExpandedImageBox::OnDestroyInstance()
	{
		if (m_pImageInstance)
		{
			CGraphicExpandedImageInstance::Delete((CGraphicExpandedImageInstance*)m_pImageInstance);
			m_pImageInstance=NULL;
		}
	}

	void CExpandedImageBox::SetScale(float fx, float fy)
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetScale(fx, fy);
		CWindow::SetSize(long(float(GetWidth())*fx), long(float(GetHeight())*fy));
	}
	void CExpandedImageBox::SetOrigin(float fx, float fy)
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetOrigin(fx, fy);
	}
	void CExpandedImageBox::SetRotation(float fRotation)
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetRotation(fRotation);
	}
	int CExpandedImageBox::GetRenderingWidth()
	{
		return CWindow::GetWidth() + m_renderingRect.right + m_renderingRect.left;
	}
	int CExpandedImageBox::GetRenderingHeight()
	{
		return CWindow::GetHeight() + m_renderingRect.bottom + m_renderingRect.top;
	}
	void CExpandedImageBox::OnSetRenderingRect()
	{
		if (!m_pImageInstance)
			return;

		// TraceError("OnSetRenderingRect %s [%d, %d, %d, %d]", m_pImageInstance->GetGraphicImagePointer()->GetFileName(), m_renderingRect.left, m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
		((CGraphicExpandedImageInstance*)m_pImageInstance)->iSetRenderingRect(m_renderingRect.left, m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
	}
	void CExpandedImageBox::SetExpandedRenderingRect(float fLeftTop, float fLeftBottom, float fTopLeft, float fTopRight, float fRightTop, float fRightBottom, float fBottomLeft, float fBottomRight)
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetExpandedRenderingRect(fLeftTop, fLeftBottom, fTopLeft, fTopRight, fRightTop, fRightBottom, fBottomLeft, fBottomRight);
	}
	void CExpandedImageBox::SetTextureRenderingRect(float fLeft, float fTop, float fRight, float fBottom)
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetTextureRenderingRect(fLeft, fTop, fRight, fBottom);
	}

	void CExpandedImageBox::SetRenderingMode(int iMode)
	{
		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetRenderingMode(iMode);
	}

	DWORD CExpandedImageBox::GetPixelColor(DWORD x, DWORD y)
	{
		return ((CGraphicExpandedImageInstance*)m_pImageInstance)->GetPixelColor(x, y);
	}

	bool CExpandedImageBox::LoadImage(const char * c_szFileName)
	{
		if (!CImageBox::LoadImage(c_szFileName))
			return FALSE;

		return TRUE;
	}

	void CExpandedImageBox::OnUpdate()
	{
		if (!m_pImageInstance)
			return;

		CWindow::OnUpdate();
	}

	void CExpandedImageBox::OnUpdateRenderBox()
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetRenderBox(m_renderBox);
	}

	void CExpandedImageBox::OnRender()
	{
		if (!m_pImageInstance)
			return;

		CImageBox::OnRender();
	}

	void CExpandedImageBox::OnChangePosition()
	{
		if (!m_pImageInstance)
			return;

		((CGraphicExpandedImageInstance*)m_pImageInstance)->SetPosition(m_rect.left, m_rect.top);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DWORD CAniImageBox::Type()
	{
		static DWORD s_dwType = GetCRC32("CAniImageBox", strlen("CAniImageBox"));
		return (s_dwType);
	}

	BOOL CAniImageBox::OnIsType(DWORD dwType)
	{
		if (CAniImageBox::Type() == dwType)
			return TRUE;

		return FALSE;
	}

	CAniImageBox::CAniImageBox(PyObject * ppyObject)
		:	CWindow(ppyObject),
			m_bycurDelay(0),
			m_byDelay(4),
			m_bySkipCount(0),
			m_wcurIndex(0),
			m_lRealWidth(0),
			m_lRealHeight(0),
			m_fAlpha(1.0f),
			m_bUseTimedDelay(false),
			m_dwTimedDelay(0),
			m_dwcurTimedDelay(0)
	{
		m_ImageVector.clear();
	}
	CAniImageBox::~CAniImageBox()
	{
		ClearImages();
	}

	void CAniImageBox::SetDelay(int iDelay)
	{
		m_bUseTimedDelay = false;
		m_byDelay = iDelay;
	}
	void CAniImageBox::SetSkipCount(int iSkipCount)
	{
		m_bySkipCount = iSkipCount;
	}

	void CAniImageBox::SetTimedDelay(DWORD dwDelayInMS)
	{
		m_bUseTimedDelay = true;
		m_dwTimedDelay = dwDelayInMS;
		m_dwcurTimedDelay = CTimer::Instance().GetCurrentMillisecond();
	}

	bool CAniImageBox::LoadGIFImage(const char * c_szFileName)
	{
		CWindowManager::TGIFImageData* pData;
		if (!CWindowManager::Instance().GetMapGIFImageData(c_szFileName, &pData))
		{
			TraceError("Could not load gif file %s", c_szFileName);
			return false;
		}

		ClearImages();
		for (CGraphicImage* pImage : pData->vecImg)
			AppendImage(pImage);

		SetTimedDelay(pData->dwDuration);
		m_wcurIndex = 0;

		return true;
	}

	bool CAniImageBox::LoadScaledGIFImage(const char * c_szFileName, DWORD dwMaxWidth, DWORD dwMaxHeight)
	{
		CWindowManager::TGIFImageData* pData;
		if (!CWindowManager::Instance().GetResizedGIFImageData(c_szFileName, dwMaxWidth, dwMaxHeight, &pData))
		{
			TraceError("Could not load gif file %s", c_szFileName);
			return false;
		}

		ClearImages();
		for (CGraphicImage* pImage : pData->vecImg)
			AppendImage(pImage);

		SetTimedDelay(pData->dwDuration);
		m_wcurIndex = 0;

		return true;
	}

	void CAniImageBox::AppendImage(const char * c_szFileName)
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer(c_szFileName);
		if (!pResource->IsType(CGraphicImage::Type()))
			return;

		CGraphicExpandedImageInstance * pImageInstance = CGraphicExpandedImageInstance::New();

		pImageInstance->SetImagePointer(static_cast<CGraphicImage*>(pResource));
		if (pImageInstance->IsEmpty())
		{
			CGraphicExpandedImageInstance::Delete(pImageInstance);
			return;
		}

		pImageInstance->SetDiffuseColor(1.0f, 1.0f, 1.0f, m_fAlpha);

		m_ImageVector.push_back(pImageInstance);

		m_wcurIndex = rand() % m_ImageVector.size();

		m_lRealWidth = MAX(m_lRealWidth, pImageInstance->GetWidth());
		m_lRealHeight = MAX(m_lRealHeight, pImageInstance->GetHeight());
//		SetSize(pImageInstance->GetWidth(), pImageInstance->GetHeight());
//		UpdateRect();
	}
	void CAniImageBox::AppendImage(CGraphicImage * pResource)
	{
		CGraphicExpandedImageInstance * pImageInstance = CGraphicExpandedImageInstance::New();

		pImageInstance->SetImagePointer(pResource);
		if (pImageInstance->IsEmpty())
		{
			CGraphicExpandedImageInstance::Delete(pImageInstance);
			return;
		}

		pImageInstance->SetDiffuseColor(1.0f, 1.0f, 1.0f, m_fAlpha);

		m_ImageVector.push_back(pImageInstance);

		m_wcurIndex = rand() % m_ImageVector.size();

		m_lRealWidth = MAX(m_lRealWidth, pImageInstance->GetWidth());
		m_lRealHeight = MAX(m_lRealHeight, pImageInstance->GetHeight());
//		SetSize(pImageInstance->GetWidth(), pImageInstance->GetHeight());
//		UpdateRect();
	}
	void CAniImageBox::ClearImages()
	{
		for_each(m_ImageVector.begin(), m_ImageVector.end(), CGraphicExpandedImageInstance::DeleteExpandedImageInstance);
		m_ImageVector.clear();

		m_lRealWidth = 0;
		m_lRealHeight = 0;
	}

	struct FSetRenderingRect
	{
		float fLeft, fTop, fRight, fBottom;
		void operator () (CGraphicExpandedImageInstance * pInstance)
		{
			pInstance->SetRenderingRect(fLeft, fTop, fRight, fBottom);
		}
	};
	void CAniImageBox::SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom)
	{
		FSetRenderingRect setRenderingRect;
		setRenderingRect.fLeft = fLeft;
		setRenderingRect.fTop = fTop;
		setRenderingRect.fRight = fRight;
		setRenderingRect.fBottom = fBottom;
		for_each(m_ImageVector.begin(), m_ImageVector.end(), setRenderingRect);
	}

	struct FSetRenderingMode
	{
		int iMode;
		void operator () (CGraphicExpandedImageInstance * pInstance)
		{
			pInstance->SetRenderingMode(iMode);
		}
	};
	void CAniImageBox::SetRenderingMode(int iMode)
	{
		FSetRenderingMode setRenderingMode;
		setRenderingMode.iMode = iMode;
		for_each(m_ImageVector.begin(), m_ImageVector.end(), setRenderingMode);
	}

	struct FSetDiffuseColor
	{
		float r;
		float g;
		float b;
		float a;

		void operator () (CGraphicExpandedImageInstance * pInstance)
		{
			pInstance->SetDiffuseColor(r, g, b, a);
		}
	};

	void CAniImageBox::SetDiffuseColor(float r, float g, float b, float a)
	{
		FSetDiffuseColor setDiffuseColor;
		setDiffuseColor.r = r;
		setDiffuseColor.g = g;
		setDiffuseColor.b = b;
		setDiffuseColor.a = a;

		for_each(m_ImageVector.begin(), m_ImageVector.end(), setDiffuseColor);
	}

	void CAniImageBox::ResetFrame()
	{
		m_wcurIndex = 0;
	}

	void CAniImageBox::SetAlpha(float fAlpha)
	{
		m_fAlpha = fAlpha;

		for (auto it = m_ImageVector.begin(); it != m_ImageVector.end(); ++it)
			(*it)->SetDiffuseColor(1.0f, 1.0f, 1.0f, fAlpha);
	}

	float CAniImageBox::GetAlpha() const
	{
		return m_fAlpha;
	}

	void CAniImageBox::OnUpdate()
	{
		if (m_bUseTimedDelay)
		{
			DWORD dwTimeCur = CTimer::Instance().GetCurrentMillisecond();
			if (dwTimeCur - m_dwcurTimedDelay < m_dwTimedDelay)
				return;

			m_dwcurTimedDelay += m_dwTimedDelay;
		}
		else
		{
			++m_bycurDelay;
			if (m_bycurDelay < m_byDelay)
				return;

			m_bycurDelay = 0;
		}

		m_wcurIndex += 1 + m_bySkipCount;
		OnNextFrame();

		if (m_wcurIndex >= m_ImageVector.size())
		{
			m_wcurIndex = 0;

			OnEndFrame();
		}
	}
	void CAniImageBox::OnRender()
	{
		if (!IsShow())
			return;

		if (m_wcurIndex < m_ImageVector.size())
		{
			CGraphicExpandedImageInstance * pImage = m_ImageVector[m_wcurIndex];
			pImage->Render();
		}
	}

	struct FChangePosition
	{
		float fx, fy;
		void operator () (CGraphicExpandedImageInstance * pInstance)
		{
			pInstance->SetPosition(fx, fy);
		}
	};

	void CAniImageBox::OnChangePosition()
	{
		FChangePosition changePosition;
		changePosition.fx = m_rect.left;
		changePosition.fy = m_rect.top;
		for_each(m_ImageVector.begin(), m_ImageVector.end(), changePosition);
	}

	void CAniImageBox::OnNextFrame()
	{
		PyCallClassMemberFunc(m_poHandler, "OnNextFrame", BuildEmptyTuple());
	}

	void CAniImageBox::OnEndFrame()
	{
		PyCallClassMemberFunc(m_poHandler, "OnEndFrame", BuildEmptyTuple());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	DWORD CButton::Type()
	{
		static DWORD s_dwType = GetCRC32("CButton", strlen("CButton"));
		return (s_dwType);
	}

	BOOL CButton::OnIsType(DWORD dwType)
	{
		if (CButton::Type() == dwType)
			return TRUE;

		return FALSE;
	}

	CButton::CButton(PyObject * ppyObject)
		:	CWindow(ppyObject),
			m_pcurVisual(NULL),
			m_bEnable(TRUE),
			m_isPressed(FALSE),
			m_isFlash(FALSE),
			m_fAlpha(1.0f) 
#ifdef COMBAT_ZONE
			,m_isFlashEx(FALSE)
#endif
	{
		CWindow::AddFlag(CWindow::FLAG_NOT_CAPTURE);
	}
	CButton::~CButton()
	{
	}

	BOOL CButton::SetUpVisual(const char * c_szFileName)
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer(c_szFileName);
		if (!pResource->IsType(CGraphicImage::Type()))
			return FALSE;

		m_upVisual.SetImagePointer(static_cast<CGraphicImage*>(pResource));
		if (m_upVisual.IsEmpty())
			return FALSE;

		SetSize(m_upVisual.GetWidth(), m_upVisual.GetHeight());
		//
		SetCurrentVisual(&m_upVisual);
		//

		return TRUE;
	}
	BOOL CButton::SetOverVisual(const char * c_szFileName)
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer(c_szFileName);
		if (!pResource->IsType(CGraphicImage::Type()))
			return FALSE;

		m_overVisual.SetImagePointer(static_cast<CGraphicImage*>(pResource));
		if (m_overVisual.IsEmpty())
			return FALSE;

		SetSize(m_overVisual.GetWidth(), m_overVisual.GetHeight());

		return TRUE;
	}
	BOOL CButton::SetDownVisual(const char * c_szFileName)
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer(c_szFileName);
		if (!pResource->IsType(CGraphicImage::Type()))
			return FALSE;

		m_downVisual.SetImagePointer(static_cast<CGraphicImage*>(pResource));
		if (m_downVisual.IsEmpty())
			return FALSE;

		SetSize(m_downVisual.GetWidth(), m_downVisual.GetHeight());

		return TRUE;
	}
	BOOL CButton::SetDisableVisual(const char * c_szFileName)
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer(c_szFileName);
		if (!pResource->IsType(CGraphicImage::Type()))
			return FALSE;

		m_disableVisual.SetImagePointer(static_cast<CGraphicImage*>(pResource));
		if (m_downVisual.IsEmpty())
			return FALSE;

		SetSize(m_disableVisual.GetWidth(), m_disableVisual.GetHeight());

		return TRUE;
	}

	const char * CButton::GetUpVisualFileName()
	{
		return m_upVisual.GetGraphicImagePointer()->GetFileName();
	}
	const char * CButton::GetOverVisualFileName()
	{
		return m_overVisual.GetGraphicImagePointer()->GetFileName();
	}
	const char * CButton::GetDownVisualFileName()
	{
		return m_downVisual.GetGraphicImagePointer()->GetFileName();
	}

	void CButton::Flash()
	{
		m_isFlash = TRUE;
	}

	void CButton::Enable()
	{
		if (!m_bEnable)
		{
			SetUp();
			m_bEnable = TRUE;
		}
	}
#ifdef COMBAT_ZONE
	void CButton::FlashEx()
	{
		m_isFlashEx = TRUE;
	}
#endif
	void CButton::Disable()
	{
		if (m_bEnable)
		{
			m_bEnable = FALSE;
			if (!m_disableVisual.IsEmpty())
				SetCurrentVisual(&m_disableVisual);
		}
	}

	bool CButton::IsEnabled()
	{
		return m_bEnable;
	}

	BOOL CButton::IsDisable()
	{
		return m_bEnable;
	}

	void CButton::SetUp()
	{
		SetCurrentVisual(&m_upVisual);
		m_isPressed = FALSE;
	}
	void CButton::Up()
	{
		if (IsIn())
			SetCurrentVisual(&m_overVisual);
		else
			SetCurrentVisual(&m_upVisual);

		PyCallClassMemberFunc(m_poHandler, "CallEvent", BuildEmptyTuple());
	}
	void CButton::Over()
	{
		SetCurrentVisual(&m_overVisual);
	}
	void CButton::Down()
	{
		m_isPressed = TRUE;
		SetCurrentVisual(&m_downVisual);
		PyCallClassMemberFunc(m_poHandler, "DownEvent", BuildEmptyTuple());
	}

	void CButton::OnUpdate()
	{
		CWindow::OnUpdate();
	}
	void CButton::OnRender()
	{
		if (!IsShow())
			return;

		if (m_pcurVisual)
		{
			if (m_isFlash)
			if (!IsIn())
			if (int(timeGetTime() / 500)%2)
			{
				return;
			}
#ifdef COMBAT_ZONE
			if (m_isFlashEx && !IsIn())
			{
				if (int(timeGetTime() / 500) % 2)
					Over();
				if (int(timeGetTime() / (500 - 15)) % 2)
					Down();
			}
#endif

			m_pcurVisual->Render();
		}

		PyCallClassMemberFunc(m_poHandler, "OnRender", BuildEmptyTuple());
	}
	void CButton::OnChangePosition()
	{
		if (m_pcurVisual)
			m_pcurVisual->SetPosition(m_rect.left, m_rect.top);
	}

	BOOL CButton::OnMouseLeftButtonDown()
	{
		if (!IsEnable())
			return TRUE;

		m_isPressed = TRUE;
		Down();

		return TRUE;
	}
	BOOL CButton::OnMouseLeftButtonDoubleClick()
	{
		if (!IsEnable())
			return TRUE;

		OnMouseLeftButtonDown();

		return TRUE;
	}
	BOOL CButton::OnMouseLeftButtonUp()
	{
		if (!IsEnable())
			return TRUE;
		if (!IsPressed())
			return TRUE;

		m_isPressed = FALSE;
		Up();

		return TRUE;
	}
	void CButton::OnMouseOverIn()
	{
		CWindow::OnMouseOverIn();

		if (!IsEnable())
			return;

		Over();
		PyCallClassMemberFunc(m_poHandler, "ShowToolTip", BuildEmptyTuple());
	}
	void CButton::OnMouseOverOut()
	{
		CWindow::OnMouseOverOut();

		if (!IsEnable())
			return;

		SetUp();
		PyCallClassMemberFunc(m_poHandler, "HideToolTip", BuildEmptyTuple());
	}

	void CButton::SetCurrentVisual(CGraphicExpandedImageInstance * pVisual)
	{
		m_pcurVisual = pVisual;
		m_pcurVisual->SetPosition(m_rect.left, m_rect.top);

		if (m_pcurVisual == &m_upVisual)
			PyCallClassMemberFunc(m_poHandler, "OnSetUpVisual", BuildEmptyTuple());
		else if (m_pcurVisual == &m_overVisual)
			PyCallClassMemberFunc(m_poHandler, "OnSetOverVisual", BuildEmptyTuple());
		else if (m_pcurVisual == &m_downVisual)
			PyCallClassMemberFunc(m_poHandler, "OnSetDownVisual", BuildEmptyTuple());
		else if (m_pcurVisual == &m_disableVisual)
			PyCallClassMemberFunc(m_poHandler, "OnSetDisableVisual", BuildEmptyTuple());
	}

	BOOL CButton::IsEnable()
	{
		return m_bEnable;
	}

	BOOL CButton::IsPressed()
	{
		return m_isPressed;
	}

	void CButton::OnSetRenderingRect()
	{
		m_upVisual.iSetRenderingRect(m_renderingRect.left, m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
		m_overVisual.iSetRenderingRect(m_renderingRect.left, m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
		m_downVisual.iSetRenderingRect(m_renderingRect.left, m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
		m_disableVisual.iSetRenderingRect(m_renderingRect.left, m_renderingRect.top, m_renderingRect.right, m_renderingRect.bottom);
	}

	void CButton::SetAlpha(float fAlpha)
	{
		m_fAlpha = fAlpha;

		m_upVisual.SetDiffuseColor(1.0f, 1.0f, 1.0f, m_fAlpha);
		m_overVisual.SetDiffuseColor(1.0f, 1.0f, 1.0f, m_fAlpha);
		m_downVisual.SetDiffuseColor(1.0f, 1.0f, 1.0f, m_fAlpha);
		m_disableVisual.SetDiffuseColor(1.0f, 1.0f, 1.0f, m_fAlpha);
	}

	float CButton::GetAlpha() const
	{
		return m_fAlpha;
	}

	void CButton::SetDiffuseColor(float r, float g, float b, float a)
	{
		m_upVisual.SetDiffuseColor(r, g, b, a);
		m_overVisual.SetDiffuseColor(r, g, b, a);
		m_downVisual.SetDiffuseColor(r, g, b, a);
		m_disableVisual.SetDiffuseColor(r, g, b, a);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CRadioButton::CRadioButton(PyObject * ppyObject) : CButton(ppyObject)
	{
	}
	CRadioButton::~CRadioButton()
	{
	}

	BOOL CRadioButton::OnMouseLeftButtonDown()
	{
		if (!IsEnable())
			return TRUE;

		if (!m_isPressed)
		{
			Down();
			PyCallClassMemberFunc(m_poHandler, "CallEvent", BuildEmptyTuple());
		}

		return TRUE;
	}
	BOOL CRadioButton::OnMouseLeftButtonUp()
	{
		return TRUE;
	}
	void CRadioButton::OnMouseOverIn()
	{
		if (!IsEnable())
			return;

		if (!m_isPressed)
		{
			SetCurrentVisual(&m_overVisual);
		}

		PyCallClassMemberFunc(m_poHandler, "ShowToolTip", BuildEmptyTuple());
	}
	void CRadioButton::OnMouseOverOut()
	{
		if (!IsEnable())
			return;

		if (!m_isPressed)
		{
			SetCurrentVisual(&m_upVisual);
		}

		PyCallClassMemberFunc(m_poHandler, "HideToolTip", BuildEmptyTuple());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CToggleButton::CToggleButton(PyObject * ppyObject) : CButton(ppyObject)
	{
	}
	CToggleButton::~CToggleButton()
	{
	}

	BOOL CToggleButton::OnMouseLeftButtonDown()
	{
		if (!IsEnable())
			return TRUE;

		if (m_isPressed)
		{
			SetUp();
			if (IsIn())
				SetCurrentVisual(&m_overVisual);
			else
				SetCurrentVisual(&m_upVisual);
			PyCallClassMemberFunc(m_poHandler, "OnToggleUp", BuildEmptyTuple());
		}
		else
		{
			Down();
			PyCallClassMemberFunc(m_poHandler, "OnToggleDown", BuildEmptyTuple());
		}

		return TRUE;
	}
	BOOL CToggleButton::OnMouseLeftButtonUp()
	{
		return TRUE;
	}

	void CToggleButton::OnMouseOverIn()
	{
		if (!IsEnable())
			return;

		if (!m_isPressed)
		{
			SetCurrentVisual(&m_overVisual);
		}

		PyCallClassMemberFunc(m_poHandler, "ShowToolTip", BuildEmptyTuple());
	}
	void CToggleButton::OnMouseOverOut()
	{
		if (!IsEnable())
			return;

		if (!m_isPressed)
		{
			SetCurrentVisual(&m_upVisual);
		}

		PyCallClassMemberFunc(m_poHandler, "HideToolTip", BuildEmptyTuple());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CDragButton::CDragButton(PyObject * ppyObject) : CButton(ppyObject)
	{
		CWindow::RemoveFlag(CWindow::FLAG_NOT_CAPTURE);
		m_restrictArea.left = 0;
		m_restrictArea.top = 0;
		m_restrictArea.right = CWindowManager::Instance().GetScreenWidth();
		m_restrictArea.bottom = CWindowManager::Instance().GetScreenHeight();
	}
	CDragButton::~CDragButton()
	{
	}

	void CDragButton::SetRestrictMovementArea(int ix, int iy, int iwidth, int iheight)
	{
		m_restrictArea.left = ix;
		m_restrictArea.top = iy;
		m_restrictArea.right = ix + iwidth;
		m_restrictArea.bottom = iy + iheight;
	}

	void CDragButton::OnChangePosition()
	{
		m_x = max(m_x, m_restrictArea.left);
		m_y = max(m_y, m_restrictArea.top);
		m_x = min(m_x, max(0, m_restrictArea.right - m_lWidth));
		m_y = min(m_y, max(0, m_restrictArea.bottom - m_lHeight));

		m_rect.left = m_x;
		m_rect.top = m_y;

		if (m_pParent)
		{
			const RECT & c_rRect = m_pParent->GetRect();
			m_rect.left += c_rRect.left;
			m_rect.top += c_rRect.top;
		}

		m_rect.right = m_rect.left + m_lWidth;
		m_rect.bottom = m_rect.top + m_lHeight;

		std::for_each(m_pChildList.begin(), m_pChildList.end(), std::mem_fun(&CWindow::UpdateRect));

		if (m_pcurVisual)
			m_pcurVisual->SetPosition(m_rect.left, m_rect.top);

		if (IsPressed())
			PyCallClassMemberFunc(m_poHandler, "OnMove", BuildEmptyTuple());
	}

	void CDragButton::OnMouseOverIn()
	{
		if (!IsEnable())

			return;

		CButton::OnMouseOverIn();
		PyCallClassMemberFunc(m_poHandler, "OnMouseOverIn", BuildEmptyTuple());
	}

	void CDragButton::OnMouseOverOut()
	{
		if (!IsEnable())
			return;

		CButton::OnMouseOverIn();
		PyCallClassMemberFunc(m_poHandler, "OnMouseOverOut", BuildEmptyTuple());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	CRenderTarget::CRenderTarget(PyObject * ppyObject) :
		CWindow(ppyObject),
		m_iRenderTargetIndex(-1)
	{
		m_pRenderTexture = NULL;
	}

	CRenderTarget::~CRenderTarget()
	{
		m_pRenderTexture = NULL;
		m_iRenderTargetIndex = -1;
	}

	bool CRenderTarget::SetRenderTarget(int iRenderTargetInex)
	{
		m_iRenderTargetIndex = iRenderTargetInex;

		CRenderTargetManager& rkRTMgr = CRenderTargetManager::Instance();
		//CGraphicRenderTargetTexture* pTex = rkRTMgr.GetRenderTarget(iRenderTargetInex);
		//if (!pTex)
		//{
			//delete pTex;
			//TraceError("CRenderTarget::SetRenderTarget Cannot create RenderTargetIndex %d", iRenderTargetInex);
			//return false;
			if (!rkRTMgr.CreateRenderTargetWithIndex(m_lWidth, m_lHeight, iRenderTargetInex))
			{
				TraceError("CRenderTarget::SetRenderTarget Cannot create RenderTargetIndex %d", iRenderTargetInex);
				return false;
			}
			CGraphicRenderTargetTexture * pTex = rkRTMgr.GetRenderTarget(iRenderTargetInex);
			if (!pTex)
			{
				TraceError("CRenderTarget::SetRenderTarget Cannot create RenderTargetIndex %d", iRenderTargetInex);
				return false;
			}
		//}

		m_pRenderTexture = pTex;
		return true;
	}

	bool CRenderTarget::SetWikiRenderTarget(int iRenderTargetInex)
	{
		m_iRenderTargetIndex = iRenderTargetInex;

		CGraphicRenderTargetTexture* pTex = CRenderTargetManager::Instance().CreateWikiRenderTarget(iRenderTargetInex, m_lWidth, m_lHeight);
		if (!pTex)
		{
			TraceError("CRenderTarget::SetWikiRenderTarget Cannot create WikiRenderTargetIndex %d", iRenderTargetInex);
			return false;
		}

		m_pRenderTexture = pTex;
		return true;
	}

	void CRenderTarget::OnUpdate()
	{
		if (!m_pRenderTexture)
			return;

		if (!IsShow())
			return;

		m_pRenderTexture->SetRenderingRect(m_rect);
	}

	void CRenderTarget::OnRender()
	{
		if (!m_pRenderTexture)
			return;

		if (!IsShow())
			return;

		m_pRenderTexture->Render();
	}

	void CRenderTarget::OnUpdateRenderBox()
	{
		if (!m_pRenderTexture)
			return;

		m_pRenderTexture->SetRenderBox(m_renderBox);
	}


};
