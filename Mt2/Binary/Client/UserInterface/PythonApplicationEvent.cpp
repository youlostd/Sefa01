#include "StdAfx.h"
#include "PythonApplication.h"
#include "../eterlib/Camera.h"
#if defined(ENABLE_TAB_NEXT_TARGET)
	#include "PythonCharacterManager.h"
	#include "PythonPlayer.h"
	#include "InstanceBase.h"
#endif

void CPythonApplication::OnCameraUpdate()
{
	if ( m_pyBackground.IsMapReady() )
	{
		CCamera* pkCameraMgr = CCameraManager::Instance().GetCurrentCamera();
		if (pkCameraMgr)
			pkCameraMgr->Update();
	}
}

void CPythonApplication::OnUIUpdate()
{
	UI::CWindowManager& rkUIMgr=UI::CWindowManager::Instance();
	rkUIMgr.Update();
}

void CPythonApplication::OnUIRender()
{
	UI::CWindowManager& rkUIMgr=UI::CWindowManager::Instance();
	rkUIMgr.Render();
}

void CPythonApplication::OnSizeChange(int width, int height)
{	
}

void CPythonApplication::OnMouseMiddleButtonDown(int x, int y)
{
	CCameraManager& rkCmrMgr=CCameraManager::Instance();
	CCamera* pkCmrCur=rkCmrMgr.GetCurrentCamera();
	if (pkCmrCur)
		pkCmrCur->BeginDrag(x, y);

	if ( !m_pyBackground.IsMapReady() )
		return;

	SetCursorNum(CAMERA_ROTATE);
	if ( CURSOR_MODE_HARDWARE == GetCursorMode())
		SetCursorVisible(FALSE, true);
}

void CPythonApplication::OnMouseMiddleButtonUp(int x, int y)
{
	CCameraManager& rkCmrMgr=CCameraManager::Instance();
	CCamera* pkCmrCur=rkCmrMgr.GetCurrentCamera();
	if (pkCmrCur)
		pkCmrCur->EndDrag();

	if ( !m_pyBackground.IsMapReady() )
		return;

	SetCursorNum(NORMAL);
	if ( CURSOR_MODE_HARDWARE == GetCursorMode())
		SetCursorVisible(TRUE);
}

void CPythonApplication::OnMouseWheel(int nLen)
{
	UI::CWindowManager& rkWndMgr = UI::CWindowManager::Instance();
	if (rkWndMgr.RunMouseWheel(nLen / WHEEL_DELTA))
		return;

	CCameraManager& rkCmrMgr=CCameraManager::Instance();
	CCamera* pkCmrCur=rkCmrMgr.GetCurrentCamera();
	if (pkCmrCur)
		pkCmrCur->Wheel(nLen);
}


void CPythonApplication::OnMouseMove(int x, int y)
{
	CCameraManager& rkCmrMgr=CCameraManager::Instance();
	CCamera* pkCmrCur;

	POINT Point;
	rkCmrMgr.SetCurrentCamera(CCameraManager::SHOPDECO_CAMERA);
	pkCmrCur = rkCmrMgr.GetCurrentCamera();
	bool dragDone = false;
	if (pkCmrCur)
	{
		if (pkCmrCur->Drag(x, y, &Point))
		{
			x = Point.x;
			y = Point.y;
			ClientToScreen(m_hWnd, &Point);

			SetCursorPos(Point.x, Point.y);
			dragDone = true;
		}
	}
	rkCmrMgr.ResetToPreviousCamera();

	pkCmrCur = rkCmrMgr.GetCurrentCamera();
	if (pkCmrCur && !dragDone)
	{
		if ( CPythonBackground::Instance().IsMapReady() && pkCmrCur->Drag(x, y, &Point) )
		{
			x = Point.x;
			y = Point.y;
			ClientToScreen(m_hWnd, &Point);

			// 2004.07.26.myevan.��ö��HackShield�� �浹
			SetCursorPos(Point.x, Point.y);

		}
	}
	
	RECT rcWnd;
	GetClientRect(&rcWnd);
	
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.SetResolution(rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top);

	rkWndMgr.RunMouseMove(x, y);
}

void CPythonApplication::OnMouseLeftButtonDown(int x, int y)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunMouseLeftButtonDown(x, y);
}

void CPythonApplication::OnMouseLeftButtonUp(int x, int y)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunMouseLeftButtonUp(x, y);
}

void CPythonApplication::OnMouseLeftButtonDoubleClick(int x, int y)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunMouseLeftButtonDown(x, y);
	rkWndMgr.RunMouseLeftButtonDoubleClick(x, y);
}

void CPythonApplication::OnMouseRightButtonDown(int x, int y)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunMouseRightButtonDown(x, y);
}

void CPythonApplication::OnMouseRightButtonUp(int x, int y)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunMouseRightButtonUp(x, y);
}

void CPythonApplication::OnKeyDown(int iIndex)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();

	if (DIK_ESCAPE == iIndex)
	{
		rkWndMgr.RunPressEscapeKey();
	}
	else if (DIK_RETURN == iIndex)
	{
		if (rkWndMgr.RunPressReturnKey())
			return;
	}
	
#if defined(ENABLE_TAB_NEXT_TARGET)
	if (DIK_TAB == iIndex)
	{
		const auto rkPlayer = CPythonPlayer::InstancePtr();
		if (rkPlayer)
		{
			const auto pkInstTarget = CPythonCharacterManager::Instance().GetTabNextTargetPointer(rkPlayer->NEW_GetMainActorPtr());
			if (pkInstTarget)
				rkPlayer->SetTarget(pkInstTarget->GetVirtualID(), true);
		}
	}
#endif

	rkWndMgr.RunKeyDown(iIndex);
}

void CPythonApplication::OnKeyUp(int iIndex)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunKeyUp(iIndex);
}

void CPythonApplication::RunIMEUpdate()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunIMEUpdate();
}
void CPythonApplication::RunIMETabEvent()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunIMETabEvent();
}
void CPythonApplication::RunIMEReturnEvent()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunIMEReturnEvent();
}
void CPythonApplication::OnIMEKeyDown(int iIndex)
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunIMEKeyDown(iIndex);
}
/////////////////////////////

void CPythonApplication::RunIMEChangeCodePage()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunChangeCodePage();
}
void CPythonApplication::RunIMEOpenCandidateListEvent()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunOpenCandidate();
}
void CPythonApplication::RunIMECloseCandidateListEvent()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunCloseCandidate();
}
void CPythonApplication::RunIMEOpenReadingWndEvent()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunOpenReading();
}
void CPythonApplication::RunIMECloseReadingWndEvent()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunCloseReading();
}

/////////////////////////////
void CPythonApplication::RunPressExitKey()
{
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.RunPressExitKey();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPythonApplication::OnMouseUpdate()
{
#ifdef _DEBUG
	if (!m_poMouseHandler)
	{
		//assert(!" CPythonApplication::OnMouseUpdate - Mouse handler has not set!");
		return;
	}
#endif _DEBUG

	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	long lx, ly;
	rkWndMgr.GetMousePosition(lx, ly);
	PyCallClassMemberFunc(m_poMouseHandler, "Update", Py_BuildValue("(ii)", lx, ly));
}

void CPythonApplication::OnMouseRender()
{
#ifdef _DEBUG
	if (!m_poMouseHandler)
	{
		//assert(!" CPythonApplication::OnMouseRender - Mouse handler has not set!");
		return;
	}
#endif _DEBUG

	PyCallClassMemberFunc(m_poMouseHandler, "Render", Py_BuildValue("()"));
}
