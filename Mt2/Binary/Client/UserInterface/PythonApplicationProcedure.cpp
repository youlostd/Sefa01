﻿#include "StdAfx.h"
#include "PythonApplication.h"
#include "../eterlib/Camera.h"
#include "../CWebBrowser/CWebBrowser.h"

#include <winuser.h>

static int gs_nMouseCaptureRef = 0;

void CPythonApplication::SafeSetCapture()
{
	SetCapture(m_hWnd);
	gs_nMouseCaptureRef++;
}

void CPythonApplication::SafeReleaseCapture()
{
	gs_nMouseCaptureRef--;
	if (gs_nMouseCaptureRef==0)
		ReleaseCapture();
}

void CPythonApplication::__SetFullScreenWindow(HWND hWnd, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
	DEVMODE DevMode;
	DevMode.dmSize = sizeof(DevMode);
	DevMode.dmBitsPerPel = dwBPP;
	DevMode.dmPelsWidth = dwWidth;
	DevMode.dmPelsHeight = dwHeight;
	DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

	long Error = ChangeDisplaySettings(&DevMode, CDS_FULLSCREEN);
	if(Error == DISP_CHANGE_RESTART)
	{
		ChangeDisplaySettings(0,0);
	}
}

void CPythonApplication::__MinimizeFullScreenWindow(HWND hWnd, DWORD dwWidth, DWORD dwHeight)
{
	ChangeDisplaySettings(0, 0);
	SetWindowPos(hWnd, 0, 0, 0,
				 dwWidth,
				 dwHeight,
				 SWP_SHOWWINDOW);
	ShowWindow(hWnd, SW_MINIMIZE);
}

#ifdef ENABLE_CAMERA_BUGFIX_MINIMIZED
void CPythonApplication::__ResetCameraWhenMinimize()
{
	CCameraManager& rkCmrMgr=CCameraManager::Instance();
	CCamera* pkCmrCur=rkCmrMgr.GetCurrentCamera();
	if (pkCmrCur) 
	{
		pkCmrCur->EndDrag();
	}
	
	SetCursorNum(NORMAL);
	if ( CURSOR_MODE_HARDWARE == GetCursorMode())
		SetCursorVisible(TRUE);
}
#endif

LRESULT CPythonApplication::WindowProcedure(HWND hWnd, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	const int c_DoubleClickTime = 300;
	const int c_DoubleClickBox = 5;
	static int s_xDownPosition = 0;
	static int s_yDownPosition = 0;
	static int s_iLastKeyDownVal = 0;

	switch (uiMsg)
	{
		case WM_ACTIVATEAPP:
			{
				m_isActivateWnd = (wParam == WA_ACTIVE) || (wParam == WA_CLICKACTIVE);

				if (m_isActivateWnd)
				{
					m_SoundManager.RestoreVolume();

					//////////////////

					if (m_isWindowFullScreenEnable)
					{
						__SetFullScreenWindow(hWnd, m_dwWidth, m_dwHeight, m_pySystem.GetBPP());
					}
				}
				else
				{
					m_SoundManager.SaveVolume();

					//////////////////
#ifndef ENABLE_CAMERA_BUGFIX_MINIMIZED
					if (m_isWindowFullScreenEnable)
					{
						__MinimizeFullScreenWindow(hWnd, m_dwWidth, m_dwHeight);
					}
#else
					if (m_isWindowFullScreenEnable)
					{
						__MinimizeFullScreenWindow(hWnd, m_dwWidth, m_dwHeight);
						__ResetCameraWhenMinimize();
					}
					else
					{
						__ResetCameraWhenMinimize();
					}
#endif
				}
			}
			break;

		case WM_INPUTLANGCHANGE:
			return CPythonIME::Instance().WMInputLanguage(hWnd, uiMsg, wParam, lParam);
			break;

		case WM_IME_STARTCOMPOSITION:
			return CPythonIME::Instance().WMStartComposition(hWnd, uiMsg, wParam, lParam);
			break;

		case WM_IME_COMPOSITION:
			return CPythonIME::Instance().WMComposition(hWnd, uiMsg, wParam, lParam);
			break;

		case WM_IME_ENDCOMPOSITION:
			return CPythonIME::Instance().WMEndComposition(hWnd, uiMsg, wParam, lParam);
			break;

		case WM_IME_NOTIFY:
			return CPythonIME::Instance().WMNotify(hWnd, uiMsg, wParam, lParam);
			break;

		case WM_IME_SETCONTEXT:
			lParam &= ~(ISC_SHOWUICOMPOSITIONWINDOW | ISC_SHOWUIALLCANDIDATEWINDOW);
			break;

		case WM_CHAR:
#ifdef ENABLE_CHAR_INPUT_LOG
			TraceError("WM_CHAR[%u(%u)|%u]", wParam, (wchar_t)(wParam & 0xffff), lParam);
#endif
#ifdef ENABLE_NEW_WEBBROWSER
			if (!CPythonNewWeb::Instance().OnKeyEvent(uiMsg, wParam, lParam))
				return CPythonIME::Instance().WMChar(hWnd, uiMsg, wParam, lParam, s_iLastKeyDownVal);

			return 0;
#else
			return CPythonIME::Instance().WMChar(hWnd, uiMsg, wParam, lParam, s_iLastKeyDownVal);
#endif
			break;

		case WM_KEYDOWN:
		{
			s_iLastKeyDownVal = LOWORD(wParam);
#ifdef ENABLE_CHAR_INPUT_LOG
			char bits[32 + 1];
			for (int i = 0; i < 32; ++i) {
				bits[i] = IS_SET(lParam, 1 << i) ? '1' : '0';
			}
			bits[32] = '\0';
			TraceError("WM_KEYDOWN[%u(%u)|%s]", wParam, LOWORD(wParam), bits);
#endif
#ifdef ENABLE_NEW_WEBBROWSER
			if (!CPythonNewWeb::Instance().OnKeyEvent(uiMsg, wParam, lParam))
				OnIMEKeyDown(LOWORD(wParam));
#else
			OnIMEKeyDown(LOWORD(wParam));
#endif
		}
			break;

		case WM_LBUTTONDOWN:
			SafeSetCapture();

			if (ELTimer_GetMSec() - m_dwLButtonDownTime < c_DoubleClickTime &&
				abs(LOWORD(lParam) - s_xDownPosition) < c_DoubleClickBox &&
				abs(HIWORD(lParam) - s_yDownPosition) < c_DoubleClickBox)
			{
				m_dwLButtonDownTime = 0;

				OnMouseLeftButtonDoubleClick(short(LOWORD(lParam)), short(HIWORD(lParam)));
			}
			else
			{
				m_dwLButtonDownTime = ELTimer_GetMSec();

				OnMouseLeftButtonDown(short(LOWORD(lParam)), short(HIWORD(lParam)));
			}

			s_xDownPosition = LOWORD(lParam);
			s_yDownPosition = HIWORD(lParam);
			return 0;

		case WM_LBUTTONUP:
			m_dwLButtonUpTime = ELTimer_GetMSec();

			if (hWnd == GetCapture())
			{
				SafeReleaseCapture();
				OnMouseLeftButtonUp(short(LOWORD(lParam)), short(HIWORD(lParam)));
			}
			return 0;

		case WM_MBUTTONDOWN:
			SafeSetCapture();

			UI::CWindowManager::Instance().RunMouseMiddleButtonDown(short(LOWORD(lParam)), short(HIWORD(lParam)));
//			OnMouseMiddleButtonDown(short(LOWORD(lParam)), short(HIWORD(lParam)));
			break;

		case WM_MBUTTONUP:
			if (GetCapture() == hWnd)
			{
				SafeReleaseCapture();

				UI::CWindowManager::Instance().RunMouseMiddleButtonUp(short(LOWORD(lParam)), short(HIWORD(lParam)));
//				OnMouseMiddleButtonUp(short(LOWORD(lParam)), short(HIWORD(lParam)));
			}
			break;

		case WM_RBUTTONDOWN:
			SafeSetCapture();
			OnMouseRightButtonDown(short(LOWORD(lParam)), short(HIWORD(lParam)));
			return 0;

		case WM_RBUTTONUP:
			if (hWnd == GetCapture()) 
			{
				SafeReleaseCapture();

				OnMouseRightButtonUp(short(LOWORD(lParam)), short(HIWORD(lParam)));
			}
			return 0;

		case 0x20a:
			if (WebBrowser_IsVisible())
			{
				// À¥ºê¶ó¿ìÀú »óÅÂÀÏ¶§´Â ÈÙ ÀÛµ¿ ¾ÈµÇµµ·Ï Ã³¸®
			}
			else
			{
				OnMouseWheel(short(HIWORD(wParam)));
			}
			break;

		case WM_SIZE:
			switch (wParam)
			{
				case SIZE_RESTORED:
				case SIZE_MAXIMIZED:
					{
						RECT rcWnd; 
						GetClientRect(&rcWnd); 
				
						UINT uWidth=rcWnd.right-rcWnd.left; 
						UINT uHeight=rcWnd.bottom-rcWnd.left; 
						m_grpDevice.ResizeBackBuffer(uWidth, uHeight);				
					}
					break;
			}

			if (wParam==SIZE_MINIMIZED)
				m_isMinimizedWnd=true;
			else
				m_isMinimizedWnd=false;

			OnSizeChange(short(LOWORD(lParam)), short(HIWORD(lParam)));

			break;

		case WM_EXITSIZEMOVE:    
			{
				RECT rcWnd; 
				GetClientRect(&rcWnd); 
				
				UINT uWidth=rcWnd.right-rcWnd.left; 
				UINT uHeight=rcWnd.bottom-rcWnd.left; 
				m_grpDevice.ResizeBackBuffer(uWidth, uHeight);				
				OnSizeChange(short(LOWORD(lParam)), short(HIWORD(lParam)));
			}
			break; 

		case WM_SYSKEYDOWN:
			switch (LOWORD(wParam))
			{
				case VK_F10:
					break;
			}
			break;

		case WM_SYSKEYUP:
			switch(LOWORD(wParam))
			{
				case 18:
					return FALSE;
					break;
				case VK_F10:
					break;
			}
			break;

		case WM_SETCURSOR:
			if (IsActive())
			{
				if (m_bCursorVisible && CURSOR_MODE_HARDWARE == m_iCursorMode)
				{
					SetCursor((HCURSOR) m_hCurrentCursor);
					return 0;
				}
				else
				{
					SetCursor(NULL);
					return 0;
				}
			}
			break;

		case WM_CLOSE:
#ifdef _DEBUG
			PostQuitMessage(0);
#else	
			RunPressExitKey();
#endif
			return 0;

#ifdef ENABLE_PYTHON_CONFIG
		case WM_NEW_RELOAD_BLOCKLIST:
			CPythonConfig::Instance().ReloadBlockNameList();
			break;
#endif

		case WM_DESTROY:
			return 0;
		default:
			//Tracenf("%x msg %x", timeGetTime(), uiMsg);
			break;
	}	

	return CMSApplication::WindowProcedure(hWnd, uiMsg, wParam, lParam);
}
