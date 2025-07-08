#include "StdAfx.h"
#include "PythonWindow.h"
#include "PythonSlotWindow.h"
#include "PythonGridSlotWindow.h"

bool PyTuple_GetWindow(PyObject* poArgs, int pos, UI::CWindow ** ppRetWindow)
{
	int iHandle;
	if (!PyTuple_GetInteger(poArgs, pos, &iHandle))
		return false;
	if (!iHandle)
		return false;

	*ppRetWindow = (UI::CWindow*)iHandle;
	return true;
}

PyObject * wndMgrGetAspect(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("f", UI::CWindowManager::Instance().GetAspect());
}

PyObject * wndMgrOnceIgnoreMouseLeftButtonUpEvent(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindowManager::Instance().OnceIgnoreMouseLeftButtonUpEvent();
	return Py_BuildNone();
}

PyObject * wndMgrSetMouseHandler(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * poHandler;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetMouseHandler(poHandler);
	return Py_BuildNone();
}

/////////////////////////////////////////////////////////////////////////////////////////////////
///// Register /////
// Window
PyObject * wndMgrRegister(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindowManager& kWndMgr=UI::CWindowManager::Instance();
	UI::CWindow * pWindow = kWndMgr.RegisterWindow(po, szLayer);
	if (!pWindow)
		return Py_BuildException();
	
	return Py_BuildValue("i", pWindow);
}

// SlotWindow
PyObject * wndMgrRegisterSlotWindow(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterSlotWindow(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// GridSlotWindow
PyObject * wndMgrRegisterGridSlotWindow(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterGridSlotWindow(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// TextLine
PyObject * wndMgrRegisterTextLine(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterTextLine(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// MarkBox
PyObject * wndMgrRegisterMarkBox(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterMarkBox(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// ImageBox
PyObject * wndMgrRegisterImageBox(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterImageBox(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// ExpandedImageBox
PyObject * wndMgrRegisterExpandedImageBox(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterExpandedImageBox(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// AniImageBox
PyObject * wndMgrRegisterAniImageBox(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterAniImageBox(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// RegisterButton
PyObject * wndMgrRegisterButton(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterButton(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// RadioButton
PyObject * wndMgrRegisterRadioButton(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterRadioButton(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// ToggleButton
PyObject * wndMgrRegisterToggleButton(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterToggleButton(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// DragButton
PyObject * wndMgrRegisterDragButton(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterDragButton(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// Box
PyObject * wndMgrRegisterBox(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterBox(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// Bar
PyObject * wndMgrRegisterBar(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterBar(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// Line
PyObject * wndMgrRegisterLine(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterLine(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// Slot
PyObject * wndMgrRegisterBar3D(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterBar3D(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// NumberLine
PyObject * wndMgrRegisterNumberLine(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterNumberLine(po, szLayer);
	return Py_BuildValue("i", pWindow);
}

// RenderTarget
PyObject * wndMgrRegisterRenderTarget(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * po;
	if (!PyTuple_GetObject(poArgs, 0, &po))
		return Py_BuildException();
	char * szLayer;
	if (!PyTuple_GetString(poArgs, 1, &szLayer))
		return Py_BuildException();

	UI::CWindow * pWindow = UI::CWindowManager::Instance().RegisterRenderTarget(po, szLayer);
	return Py_BuildValue("i", pWindow);
}
///// Register /////
/////////////////////////////////////////////////////////////////////////////////////////////////

PyObject * wndMgrDestroy(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	UI::CWindowManager::Instance().DestroyWindow(pWin);
	return Py_BuildNone();
}

PyObject * wndMgrIsFocus(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("i", pWindow == UI::CWindowManager::Instance().GetActivateWindow());
}

PyObject * wndMgrSetFocus(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	UI::CWindowManager::Instance().ActivateWindow(pWin);
	return Py_BuildNone();
}

PyObject * wndMgrKillFocus(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (PyTuple_GetWindow(poArgs, 0, &pWin))
	{
		if (pWin == UI::CWindowManager::Instance().GetActivateWindow())
		{
			UI::CWindowManager::Instance().DeactivateWindow();
		}
	}
	else
	{
		UI::CWindowManager::Instance().DeactivateWindow();
	}

	return Py_BuildNone();
}

PyObject * wndMgrLock(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	UI::CWindowManager::Instance().LockWindow(pWin);
	return Py_BuildNone();
}

PyObject * wndMgrUnlock(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindowManager::Instance().UnlockWindow();
	return Py_BuildNone();
}

PyObject * wndMgrSetName(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	char * szName;
	if (!PyTuple_GetString(poArgs, 1, &szName))
		return Py_BuildException();

	pWin->SetName(szName);
	return Py_BuildNone();
}

PyObject * wndMgrSetTop(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetTop(pWin);
	return Py_BuildNone();
}

PyObject * wndMgrShow(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	pWin->Show();
	return Py_BuildNone();
}

PyObject * wndMgrHide(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	pWin->Hide();
	return Py_BuildNone();
}

PyObject * wndMgrIsShow(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	return Py_BuildValue("i", pWin->IsShow());
}

PyObject * wndMgrIsRTL(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	return Py_BuildValue("i", pWin->IsFlag(UI::CWindow::FLAG_RTL));
}

PyObject * wndMgrSetAlpha(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fAlpha;
	if (!PyTuple_GetFloat(poArgs, 1, &fAlpha))
		return Py_BuildException();

	pWindow->SetSingleAlpha(fAlpha);

	return Py_BuildNone();
}

PyObject * wndMgrGetAlpha(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("f", pWindow->GetAlpha());
}

PyObject * wndMgrSetAllAlpha(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	float fAlpha;
	if (!PyTuple_GetFloat(poArgs, 1, &fAlpha))
		return Py_BuildException();

	pWin->SetAllAlpha(fAlpha);

	return Py_BuildNone();
}

PyObject * wndMgrSetScreenSize(PyObject * poSelf, PyObject * poArgs)
{
	int width;
	if (!PyTuple_GetInteger(poArgs, 0, &width))
		return Py_BuildException();
	int height;
	if (!PyTuple_GetInteger(poArgs, 1, &height))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetScreenSize(width, height);
	return Py_BuildNone();
}

PyObject * wndMgrSetParent(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	UI::CWindow * pParentWin;
	if (!PyTuple_GetWindow(poArgs, 1, &pParentWin))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetParent(pWin, pParentWin);
	return Py_BuildNone();
}

PyObject * wndMgrSetPickAlways(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetPickAlways(pWin);
	return Py_BuildNone();
}

PyObject * wndMgrSetWndSize(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int width;
	if (!PyTuple_GetInteger(poArgs, 1, &width))
		return Py_BuildException();
	int height;
	if (!PyTuple_GetInteger(poArgs, 2, &height))
		return Py_BuildException();

	pWin->SetSize(width, height);
	return Py_BuildNone();
}

PyObject * wndMgrSetWndPosition(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int x;
	if (!PyTuple_GetInteger(poArgs, 1, &x))
		return Py_BuildException();
	int y;
	if (!PyTuple_GetInteger(poArgs, 2, &y))
		return Py_BuildException();

	pWin->SetPosition(x, y);
	return Py_BuildNone();
}

PyObject * wndMgrGetName(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	return Py_BuildValue("s", pWin->GetName());
}

PyObject * wndMgrGetWndWidth(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	return Py_BuildValue("i", pWin->GetWidth());
}

PyObject * wndMgrGetWndHeight(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	return Py_BuildValue("i", pWin->GetHeight());
}

PyObject * wndMgrGetWndLocalPosition(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	long lx, ly;
	pWin->GetPosition(&lx, &ly);

	return Py_BuildValue("ii", lx, ly);
}

PyObject * wndMgrGetWndGlobalPosition(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	RECT & rRect = pWindow->GetRect();
	return Py_BuildValue("ii", rRect.left, rRect.top);
}

PyObject * wndMgrGetWindowRect(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	RECT & rRect = pWin->GetRect();
	return Py_BuildValue("iiii", rRect.left, rRect.top, rRect.right - rRect.left, rRect.bottom - rRect.top);
}

PyObject * wndMgrSetWindowHorizontalAlign(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iAlign;
	if (!PyTuple_GetInteger(poArgs, 1, &iAlign))
		return Py_BuildException();

	pWin->SetHorizontalAlign(iAlign);

	return Py_BuildNone();
}

PyObject * wndMgrSetWindowVerticalAlign(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iAlign;
	if (!PyTuple_GetInteger(poArgs, 1, &iAlign))
		return Py_BuildException();

	pWin->SetVerticalAlign(iAlign);

	return Py_BuildNone();
}

PyObject * wndMgrIsIn(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	bool bCheckChilds;
	if (!PyTuple_GetBoolean(poArgs, 1, &bCheckChilds))
		return Py_BuildException();

	UI::CWindow* pWinPoint = UI::CWindowManager::Instance().GetPointWindow();
	if (pWin == pWinPoint)
		return Py_BuildValue("b", true);

	if (bCheckChilds && pWinPoint)
	{
		if (pWin->IsChild(pWinPoint, true))
			return Py_BuildValue("b", true);
	}

	return Py_BuildValue("b", false);
}

PyObject * wndMgrGetMouseLocalPosition(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	long lx, ly;
	pWin->GetMouseLocalPosition(lx, ly);
	return Py_BuildValue("ii", lx, ly);
}

inline int DISTANCE_APPROX(int dx, int dy)
{
	int min, max;

	if (dx < 0)
		dx = -dx;

	if (dy < 0)
		dy = -dy;

	if (dx < dy)
	{
		min = dx;
		max = dy;
	}
	else
	{
		min = dy;
		max = dx;
	}

	// coefficients equivalent to ( 123/128 * max ) and ( 51/128 * min )
	return (((max << 8) + (max << 3) - (max << 4) - (max << 1) +
		(min << 7) - (min << 5) + (min << 3) - (min << 1)) >> 8);
}

PyObject * wndMgrIsInCircle(PyObject * poSelf, PyObject * poArgs)
{
	int iCenterX;
	if (!PyTuple_GetInteger(poArgs, 0, &iCenterX))
		return Py_BadArgument();
	int iCenterY;
	if (!PyTuple_GetInteger(poArgs, 1, &iCenterY))
		return Py_BadArgument();
	int iRadius;
	if (!PyTuple_GetInteger(poArgs, 2, &iRadius))
		return Py_BadArgument();
	int iX;
	if (!PyTuple_GetInteger(poArgs, 3, &iX))
		return Py_BadArgument();
	int iY;
	if (!PyTuple_GetInteger(poArgs, 4, &iY))
		return Py_BadArgument();

	int iSize = pow((iCenterX - iX), 2) + pow((iCenterY - iY), 2);
	bool bInCircle = iSize < iRadius * iRadius;

	if (!bInCircle)
	{
		int iDist = DISTANCE_APPROX(iCenterX - iX, iCenterY - iY);
		bInCircle = iDist < iRadius;
	}

	return Py_BuildValue("b", bInCircle);
}

PyObject * wndMgrIsMouseInCircle(PyObject * poSelf, PyObject * poArgs)
{
	int iCenterX;
	if (!PyTuple_GetInteger(poArgs, 0, &iCenterX))
		return Py_BadArgument();
	int iCenterY;
	if (!PyTuple_GetInteger(poArgs, 1, &iCenterY))
		return Py_BadArgument();
	int iRadius;
	if (!PyTuple_GetInteger(poArgs, 2, &iRadius))
		return Py_BadArgument();

	long lMouseX, lMouseY;
	UI::CWindowManager::Instance().GetMousePosition(lMouseX, lMouseY);

	int iSize = pow((iCenterX - lMouseX), 2) + pow((iCenterY - lMouseY), 2);
	bool bInCircle = iSize < iRadius * iRadius;

	if (!bInCircle)
	{
		int iDist = DISTANCE_APPROX(iCenterX - lMouseX, iCenterY - lMouseY);
		bInCircle = iDist < iRadius;
	}

	return Py_BuildValue("b", bInCircle);
}

PyObject * wndMgrGetColorAtPosition(PyObject * poSelf, PyObject * poArgs)
{
	int iX;
	if (!PyTuple_GetInteger(poArgs, 0, &iX))
		return Py_BadArgument();
	int iY;
	if (!PyTuple_GetInteger(poArgs, 1, &iY))
		return Py_BadArgument();
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 2, &pWin))
		pWin = NULL;

	const POINT ptZero = { 0, 0 };
	HMONITOR hWndMonitor = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO kInfo;
	ZeroMemory(&kInfo, sizeof(kInfo));
	kInfo.cbSize = sizeof(kInfo);
	GetMonitorInfo(hWndMonitor, &kInfo);

	HDC pkDC = GetDC(NULL);
	COLORREF rkColor = GetPixel(pkDC, iX * GetDeviceCaps(pkDC, DESKTOPHORZRES) / GetDeviceCaps(pkDC, HORZRES),
		iY * GetDeviceCaps(pkDC, DESKTOPVERTRES) / GetDeviceCaps(pkDC, VERTRES));
	ReleaseDC(NULL, pkDC);

	BYTE bRed = GetRValue(rkColor);
	BYTE bGreen = GetGValue(rkColor);
	BYTE bBlue = GetBValue(rkColor);

	return Py_BuildValue("iii", bRed, bGreen, bBlue);
}

PyObject * wndMgrGetHyperlink(PyObject * poSelf, PyObject * poArgs)
{
	char retBuf[1024];
	int retLen = CGraphicTextInstance::Hyperlink_GetText(retBuf, sizeof(retBuf)-1);
	retBuf[retLen] = '\0';

	return Py_BuildValue("s#", retBuf, retLen);
}

PyObject * wndMgrGetScreenWidth(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", UI::CWindowManager::Instance().GetScreenWidth());
}

PyObject * wndMgrGetScreenHeight(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", UI::CWindowManager::Instance().GetScreenHeight());
}

PyObject * wndMgrGetMousePosition(PyObject * poSelf, PyObject * poArgs)
{
	long lx, ly;
	UI::CWindowManager::Instance().GetMousePosition(lx, ly);
	return Py_BuildValue("ii", lx, ly);
}

PyObject * wndMgrGetRealMousePosition(PyObject * poSelf, PyObject * poArgs)
{
	POINT kCursorPos;
	GetCursorPos(&kCursorPos);
	return Py_BuildValue("ii", kCursorPos.x, kCursorPos.y);
}

PyObject * wndMgrIsDragging(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", UI::CWindowManager::Instance().IsDragging());
}

PyObject * wndMgrIsPickedWindow(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	UI::CWindow * pPickedWin = UI::CWindowManager::Instance().GetPointWindow();
	return Py_BuildValue("i", pWin == pPickedWin ? 1 : 0);
}

PyObject * wndMgrGetChildCount(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	return Py_BuildValue("i", pWin->GetChildCount());
}

PyObject * wndMgrAddFlag(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	char * pszFlag;
	if (!PyTuple_GetString(poArgs, 1, &pszFlag))
		return Py_BuildException();

	if (pszFlag && *pszFlag)
	{
		if (!stricmp(pszFlag, "movable"))
			pWin->AddFlag(UI::CWindow::FLAG_MOVABLE);
		else if (!stricmp(pszFlag, "limit"))
			pWin->AddFlag(UI::CWindow::FLAG_LIMIT);
		else if (!stricmp(pszFlag, "dragable"))
			pWin->AddFlag(UI::CWindow::FLAG_DRAGABLE);
		else if (!stricmp(pszFlag, "attach"))
			pWin->AddFlag(UI::CWindow::FLAG_ATTACH);
		else if (!stricmp(pszFlag, "restrict_x"))
			pWin->AddFlag(UI::CWindow::FLAG_RESTRICT_X);
		else if (!stricmp(pszFlag, "restrict_y"))
			pWin->AddFlag(UI::CWindow::FLAG_RESTRICT_Y);
		else if (!stricmp(pszFlag, "float"))
			pWin->AddFlag(UI::CWindow::FLAG_FLOAT);
		else if (!stricmp(pszFlag, "not_pick"))
			pWin->AddFlag(UI::CWindow::FLAG_NOT_PICK);
		else if (!stricmp(pszFlag, "ignore_size"))
			pWin->AddFlag(UI::CWindow::FLAG_IGNORE_SIZE);
		else if (!stricmp(pszFlag, "rtl"))
			pWin->AddFlag(UI::CWindow::FLAG_RTL);
		else if (!stricmp(pszFlag, "ltr"))
			pWin->RemoveFlag(UI::CWindow::FLAG_RTL);
		else
			TraceError("Unknown window flag %s", pszFlag);
	}

	return Py_BuildNone();
}

PyObject * wndMgrSetLimitBias(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int l;
	if (!PyTuple_GetInteger(poArgs, 1, &l))
		return Py_BuildException();
	int r;
	if (!PyTuple_GetInteger(poArgs, 2, &r))
		return Py_BuildException();
	int t;
	if (!PyTuple_GetInteger(poArgs, 3, &t))
		return Py_BuildException();
	int b;
	if (!PyTuple_GetInteger(poArgs, 4, &b))
		return Py_BuildException();

	pWin->SetLimitBias(l, r, t, b);
	return Py_BuildNone();
}

PyObject * wndMgrUpdateRect(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	pWin->UpdateRect();
	return Py_BuildNone();
}

PyObject * wndMgrAppendSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iIndex))
		return Py_BuildException();

	int ixPosition;
	if (!PyTuple_GetInteger(poArgs, 2, &ixPosition))
		return Py_BuildException();

	int iyPosition;
	if (!PyTuple_GetInteger(poArgs, 3, &iyPosition))
		return Py_BuildException();

	int ixCellSize;
	if (!PyTuple_GetInteger(poArgs, 4, &ixCellSize))
		return Py_BuildException();

	int iyCellSize;
	if (!PyTuple_GetInteger(poArgs, 5, &iyCellSize))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->AppendSlot(iIndex, ixPosition, iyPosition, ixCellSize, iyCellSize);

	return Py_BuildNone();
}

PyObject * wndMgrArrangeSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iStartIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iStartIndex))
		return Py_BuildException();

	int ixCellCount;
	if (!PyTuple_GetInteger(poArgs, 2, &ixCellCount))
		return Py_BuildException();

	int iyCellCount;
	if (!PyTuple_GetInteger(poArgs, 3, &iyCellCount))
		return Py_BuildException();

	int ixCellSize;
	if (!PyTuple_GetInteger(poArgs, 4, &ixCellSize))
		return Py_BuildException();

	int iyCellSize;
	if (!PyTuple_GetInteger(poArgs, 5, &iyCellSize))
		return Py_BuildException();

	int ixBlank;
	if (!PyTuple_GetInteger(poArgs, 6, &ixBlank))
		return Py_BuildException();

	int iyBlank;
	if (!PyTuple_GetInteger(poArgs, 7, &iyBlank))
		return Py_BuildException();

	if (!pWin->IsType(UI::CGridSlotWindow::Type()))
	{
		TraceError("wndMgr.ArrangeSlot : not a grid window");
		return Py_BuildException();
	}

	UI::CGridSlotWindow * pGridSlotWin = (UI::CGridSlotWindow *)pWin;
	pGridSlotWin->ArrangeGridSlot(iStartIndex, ixCellCount, iyCellCount, ixCellSize, iyCellSize, ixBlank, iyBlank);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotBaseImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	float fr;
	if (!PyTuple_GetFloat(poArgs, 2, &fr))
		return Py_BuildException();
	float fg;
	if (!PyTuple_GetFloat(poArgs, 3, &fg))
		return Py_BuildException();
	float fb;
	if (!PyTuple_GetFloat(poArgs, 4, &fb))
		return Py_BuildException();
	float fa;
	if (!PyTuple_GetFloat(poArgs, 5, &fa))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
	{
		TraceError("wndMgr.ArrangeSlot : not a slot window");
		return Py_BuildException();
	}

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlotBaseImage(szFileName, fr, fg, fb, fa);

	return Py_BuildNone();
}

PyObject * wndMgrSetCoverButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	char * szUpImageName;
	if (!PyTuple_GetString(poArgs, 2, &szUpImageName))
		return Py_BuildException();
	char * szOverImageName;
	if (!PyTuple_GetString(poArgs, 3, &szOverImageName))
		return Py_BuildException();
	char * szDownImageName;
	if (!PyTuple_GetString(poArgs, 4, &szDownImageName))
		return Py_BuildException();
	char * szDisableImageName;
	if (!PyTuple_GetString(poArgs, 5, &szDisableImageName))
		return Py_BuildException();

	int iLeftButtonEnable;
	if (!PyTuple_GetInteger(poArgs, 6, &iLeftButtonEnable))
		return Py_BuildException();
	int iRightButtonEnable;
	if (!PyTuple_GetInteger(poArgs, 7, &iRightButtonEnable))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->SetCoverButton(iSlotIndex, szUpImageName, szOverImageName, szDownImageName, szDisableImageName, iLeftButtonEnable, iRightButtonEnable);

	return Py_BuildNone();
}

PyObject * wndMgrEnableCoverButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->EnableCoverButton(iSlotIndex);

	return Py_BuildNone();
}

PyObject * wndMgrDisableCoverButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->DisableCoverButton(iSlotIndex);

	return Py_BuildNone();
}

PyObject * wndMgrIsDisableCoverButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	return Py_BuildValue("i", pSlotWin->IsDisableCoverButton(iSlotIndex));
}

PyObject * wndMgrSetAlwaysRenderCoverButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotIndex;
	bool bAlwaysRender = false;

	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!PyTuple_GetBoolean(poArgs, 2, &bAlwaysRender))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->SetAlwaysRenderCoverButton(iSlotIndex, bAlwaysRender);

	return Py_BuildNone();
}

PyObject * wndMgrAppendSlotButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	char * szUpImageName;
	if (!PyTuple_GetString(poArgs, 1, &szUpImageName))
		return Py_BuildException();
	char * szOverImageName;
	if (!PyTuple_GetString(poArgs, 2, &szOverImageName))
		return Py_BuildException();
	char * szDownImageName;
	if (!PyTuple_GetString(poArgs, 3, &szDownImageName))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->AppendSlotButton(szUpImageName, szOverImageName, szDownImageName);

	return Py_BuildNone();
}

PyObject * wndMgrAppendRequirementSignImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	char * szImageName;
	if (!PyTuple_GetString(poArgs, 1, &szImageName))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->AppendRequirementSignImage(szImageName);

	return Py_BuildNone();
}

PyObject * wndMgrShowSlotButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotNumber;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotNumber))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->ShowSlotButton(iSlotNumber);

	return Py_BuildNone();
}

PyObject * wndMgrHideAllSlotButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->HideAllSlotButton();

	return Py_BuildNone();
}

PyObject * wndMgrShowRequirementSign(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotNumber;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotNumber))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->ShowRequirementSign(iSlotNumber);

	return Py_BuildNone();
}

PyObject * wndMgrHideRequirementSign(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotNumber;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotNumber))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;
	pSlotWin->HideRequirementSign(iSlotNumber);

	return Py_BuildNone();
}

PyObject * wndMgrRefreshSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->RefreshSlot();

	return Py_BuildNone();
}

PyObject * wndMgrSetUseMode(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetUseMode(iFlag);

	return Py_BuildNone();
}

PyObject * wndMgrSetUsableItem(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetUsableItem(iFlag);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotDiffuseColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	int iSlotColor;
	if (!PyTuple_GetInteger(poArgs, 2, &iSlotColor))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetDiffuseColor(iSlotIndex, iSlotColor);

	return Py_BuildNone();
}

#ifdef ENABLE_ITEM_SWAP_SYSTEM
PyObject * wndMgrSetUsableItem2(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetUsableItem2(iFlag);

	return Py_BuildNone();
}
#endif

PyObject * wndMgrClearSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->ClearSlot(iSlotIndex);

	return Py_BuildNone();
}

PyObject * wndMgrClearAllSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->ClearAllSlot();

	return Py_BuildNone();
}

PyObject * wndMgrHasSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;

	return Py_BuildValue("i", pSlotWin->HasSlot(iSlotIndex));
}

PyObject * wndMgrSetSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 2, &iItemIndex))
		return Py_BuildException();

	int iWidth;
	if (!PyTuple_GetInteger(poArgs, 3, &iWidth))
		return Py_BuildException();

	int iHeight;
	if (!PyTuple_GetInteger(poArgs, 4, &iHeight))
		return Py_BuildException();

	int iImageHandle;
	if (!PyTuple_GetInteger(poArgs, 5, &iImageHandle))
		return Py_BuildException();

	D3DXCOLOR diffuseColor;
	PyObject* pTuple;
	if (!PyTuple_GetObject(poArgs, 6, &pTuple))
	{
		diffuseColor = D3DXCOLOR(1.0, 1.0, 1.0, 1.0);
		//return Py_BuildException();
	}
	else
	// get diffuse color from pTuple
	{
		if (PyTuple_Size(pTuple) != 4)
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 0, &diffuseColor.r))
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 1, &diffuseColor.g))
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 2, &diffuseColor.b))
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 3, &diffuseColor.a))
			return Py_BuildException();
	}

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlot(iSlotIndex, iItemIndex, iWidth, iHeight, (CGraphicImage *)iImageHandle, diffuseColor);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotNew(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 2, &iItemIndex))
		return Py_BuildException();

	int iWidth;
	if (!PyTuple_GetInteger(poArgs, 3, &iWidth))
		return Py_BuildException();

	int iHeight;
	if (!PyTuple_GetInteger(poArgs, 4, &iHeight))
		return Py_BuildException();

	float fScale;
	if (!PyTuple_GetFloat(poArgs, 5, &fScale))
		return Py_BuildException();

	bool bImageCenter;
	if (!PyTuple_GetBoolean(poArgs, 6, &bImageCenter))
		return Py_BuildException();

	int iImageHandle;
	if (!PyTuple_GetInteger(poArgs, 7, &iImageHandle))
		return Py_BuildException();

	float fImageAlpha;
	if (!PyTuple_GetFloat(poArgs, 8, &fImageAlpha))
		return Py_BuildException();

	int pImageOverlayHandle;
	if (!PyTuple_GetInteger(poArgs, 9, &pImageOverlayHandle))
		return Py_BuildException();

	float fImageOverlayAlpha;
	if (!PyTuple_GetFloat(poArgs, 10, &fImageOverlayAlpha))
		return Py_BuildException();

	int ixAppendPos = 0;
	PyTuple_GetInteger(poArgs, 11, &ixAppendPos);
	int iyAppendPos = 0;
	PyTuple_GetInteger(poArgs, 12, &iyAppendPos);

	D3DXCOLOR diffuseColor(1.0, 1.0, 1.0, fImageAlpha);
	D3DXCOLOR diffuseOverlayColor(1.0, 1.0, 1.0, fImageOverlayAlpha);
	/*PyObject* pTuple;
	if (!PyTuple_GetObject(poArgs, 8, &pTuple))
	{
		diffuseColor = D3DXCOLOR(1.0, 1.0, 1.0, 1.0);
		//return Py_BuildException();
	}
	else
		// get diffuse color from pTuple
	{
		if (PyTuple_Size(pTuple) != 4)
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 0, &diffuseColor.r))
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 1, &diffuseColor.g))
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 2, &diffuseColor.b))
			return Py_BuildException();
		if (!PyTuple_GetFloat(pTuple, 3, &diffuseColor.a))
			return Py_BuildException();
	}*/

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	//pSlotWin->SetSlotNew(iSlotIndex, iItemIndex, iWidth, iHeight, fScale, bImageCenter, (CGraphicImage *)iImageHandle, diffuseColor, (CGraphicImage *)pImageOverlayHandle, diffuseOverlayColor, ixAppendPos, iyAppendPos);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotCount(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	int iCount;
	if (!PyTuple_GetInteger(poArgs, 2, &iCount))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlotCount(iSlotIndex, iCount);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotCountNew(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	int iGrade;
	if (!PyTuple_GetInteger(poArgs, 2, &iGrade))
		return Py_BuildException();

	int iCount;
	if (!PyTuple_GetInteger(poArgs, 3, &iCount))
		return Py_BuildException();

	int iGrade2 = 0;
	PyTuple_GetInteger(poArgs, 4, &iGrade2);

	int iCount2 = 0;
	PyTuple_GetInteger(poArgs, 5, &iCount2);

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlotCountNew(iSlotIndex, iGrade, iCount);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotCoolTime(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	float fCoolTime;
	if (!PyTuple_GetFloat(poArgs, 2, &fCoolTime))
		return Py_BuildException();

	float fElapsedTime = 0.0f;
	PyTuple_GetFloat(poArgs, 3, &fElapsedTime);

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlotCoolTime(iSlotIndex, fCoolTime, fElapsedTime);

	return Py_BuildNone();
}

PyObject * wndMgrSetToggleSlot(PyObject * poSelf, PyObject * poArgs)
{
	assert(!"wndMgrSetToggleSlot - ");
	return Py_BuildNone();
}

PyObject * wndMgrActivateSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->ActivateSlot(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrDeactivateSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->DeactivateSlot(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrEnableSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->EnableSlot(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrDisableSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->DisableSlot(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrSetUnusableSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	bool bUnusableFlag;
	if (!PyTuple_GetBoolean(poArgs, 2, &bUnusableFlag))
		bUnusableFlag = true;

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetUnusableSlot(iSlotIndex, bUnusableFlag);
	return Py_BuildNone();
}

PyObject * wndMgrSetUnusableSlotWorld(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	bool bUnusableFlag;
	if (!PyTuple_GetBoolean(poArgs, 2, &bUnusableFlag))
		bUnusableFlag = true;

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetUnusableSlotWorld(iSlotIndex, bUnusableFlag);
	return Py_BuildNone();
}

PyObject * wndMgrShowSlotBaseImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->ShowSlotBaseImage(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrHideSlotBaseImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->HideSlotBaseImage(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrSetSlotType(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iType;
	if (!PyTuple_GetInteger(poArgs, 1, &iType))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlotType(iType);

	return Py_BuildNone();
}

PyObject * wndMgrSetSlotStyle(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iStyle;
	if (!PyTuple_GetInteger(poArgs, 1, &iStyle))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SetSlotStyle(iStyle);

	return Py_BuildNone();
}

PyObject * wndMgrSelectSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->SelectSlot(iIndex);

	return Py_BuildNone();
}

PyObject * wndMgrClearSelected(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->ClearSelected();

	return Py_BuildNone();
}

PyObject * wndMgrGetSelectedSlotCount(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	return Py_BuildValue("i", pSlotWin->GetSelectedSlotCount());
}

PyObject * wndMgrGetSelectedSlotNumber(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iSlotNumber;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotNumber))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	return Py_BuildValue("i", pSlotWin->GetSelectedSlotNumber(iSlotNumber));
}

PyObject * wndMgrIsSelectedSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iSlotNumber;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotNumber))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	return Py_BuildValue("i", pSlotWin->isSelectedSlot(iSlotNumber));
}

PyObject * wndMgrGetSlotCount(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	return Py_BuildValue("i", pSlotWin->GetSlotCount());
}

PyObject * wndMgrLockSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->LockSlot(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrUnlockSlot(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
	pSlotWin->UnlockSlot(iSlotIndex);
	return Py_BuildNone();
}

PyObject * wndMgrGetSlotPosition(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (!pWin->IsType(UI::CSlotWindow::Type()))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;

	int iLeft, iTop;
	pSlotWin->GetSlotPosition(iSlotIndex, &iLeft, &iTop);
	return Py_BuildValue("ii", iLeft, iTop);
}

PyObject * wndSetRenderingRect(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fLeft;
	if (!PyTuple_GetFloat(poArgs, 1, &fLeft))
		return Py_BuildException();
	float fTop;
	if (!PyTuple_GetFloat(poArgs, 2, &fTop))
		return Py_BuildException();
	float fRight;
	if (!PyTuple_GetFloat(poArgs, 3, &fRight))
		return Py_BuildException();
	float fBottom;
	if (!PyTuple_GetFloat(poArgs, 4, &fBottom))
		return Py_BuildException();

	/*
	if (pWindow->IsType(UI::CExpandedImageBox::Type()))
		((UI::CExpandedImageBox*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	else if (pWindow->IsType(UI::CAniImageBox::Type()))
		((UI::CAniImageBox*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	else if (pWindow->IsType(UI::CButton::Type()))
		((UI::CButton*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	else if (pWindow->IsType(UI::CTextLine::Type()))
		((UI::CTextLine*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	else if (pWindow->IsType(UI::CSlotWindow::Type()))
		((UI::CSlotWindow*)pWindow)->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	else
		TraceError("invalid window type for SetRenderingRect");
		*/

	pWindow->SetRenderingRect(fLeft, fTop, fRight, fBottom);
	return Py_BuildNone();
}

PyObject * wndBarSetColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iColor;
	if (!PyTuple_GetInteger(poArgs, 1, &iColor))
		return Py_BuildException();

	if (pWindow->IsType(UI::CBar3D::Type()))
	{
		int iLeftColor = iColor;

		int iRightColor;
		if (!PyTuple_GetInteger(poArgs, 2, &iRightColor))
			return Py_BuildException();
		int iCenterColor;
		if (!PyTuple_GetInteger(poArgs, 3, &iCenterColor))
			return Py_BuildException();

		((UI::CBar3D *)pWindow)->SetColor(iLeftColor, iRightColor, iCenterColor);
	}
	else
	{
		((UI::CWindow *)pWindow)->SetColor(iColor);
	}
	return Py_BuildNone();
}

PyObject * wndMgrAttachIcon(PyObject * poSelf, PyObject * poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BuildException();
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iIndex))
		return Py_BuildException();
	int iSlotNumber;
	if (!PyTuple_GetInteger(poArgs, 2, &iSlotNumber))
		return Py_BuildException();
	int iWidth;
	if (!PyTuple_GetInteger(poArgs, 3, &iWidth))
		return Py_BuildException();
	int iHeight;
	if (!PyTuple_GetInteger(poArgs, 4, &iHeight))
		return Py_BuildException();

	UI::CWindowManager::Instance().AttachIcon(iType, iIndex, iSlotNumber, iWidth, iHeight);
	return Py_BuildNone();
}

PyObject * wndMgrDeattachIcon(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindowManager::Instance().DeattachIcon();
	return Py_BuildNone();
}

PyObject * wndMgrSetAttachingFlag(PyObject * poSelf, PyObject * poArgs)
{
	BOOL bFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &bFlag))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetAttachingFlag(bFlag);
	return Py_BuildNone();
}

// Text
PyObject * wndTextSetMax(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iMax;
	if (!PyTuple_GetInteger(poArgs, 1, &iMax))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetMax(iMax);
	return Py_BuildNone();
}
PyObject * wndTextSetHorizontalAlign(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iType;
	if (!PyTuple_GetInteger(poArgs, 1, &iType))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetHorizontalAlign(iType);
	return Py_BuildNone();
}
PyObject * wndTextSetVerticalAlign(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iType;
	if (!PyTuple_GetInteger(poArgs, 1, &iType))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetVerticalAlign(iType);
	return Py_BuildNone();
}
PyObject * wndTextSetSecret(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetSecret(iFlag);
	return Py_BuildNone();
}
PyObject * wndTextSetOutline(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetOutline(iFlag);
	return Py_BuildNone();
}
PyObject * wndTextSetOutlineColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	if (2 == PyTuple_Size(poArgs))
	{
		int iColor;
		if (!PyTuple_GetInteger(poArgs, 1, &iColor))
			return Py_BuildException();
		((UI::CTextLine*)pWindow)->SetOutlineColor(iColor);
	}
	else if (4 == PyTuple_Size(poArgs))
	{
		float fr;
		if (!PyTuple_GetFloat(poArgs, 1, &fr))
			return Py_BuildException();
		float fg;
		if (!PyTuple_GetFloat(poArgs, 2, &fg))
			return Py_BuildException();
		float fb;
		if (!PyTuple_GetFloat(poArgs, 3, &fb))
			return Py_BuildException();
		float fa = 1.0f;

		BYTE argb[4] =
		{
			(BYTE) (255.0f * fb),
			(BYTE) (255.0f * fg),
			(BYTE) (255.0f * fr),
			(BYTE) (255.0f * fa)
		};
		((UI::CTextLine*)pWindow)->SetOutlineColor(*((DWORD *) argb));
	}
	else
	{
		return Py_BuildException();
	}

	return Py_BuildNone();
}
PyObject * wndTextSetFeather(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetFeather(iFlag);
	return Py_BuildNone();
}
PyObject * wndTextSetMultiLine(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetMultiLine(iFlag);
	return Py_BuildNone();
}
PyObject * wndTextSetFontName(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFontName;
	if (!PyTuple_GetString(poArgs, 1, &szFontName))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetFontName(szFontName);
	return Py_BuildNone();
}
PyObject * wndTextSetFontColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	if (2 == PyTuple_Size(poArgs))
	{
		int iColor;
		if (!PyTuple_GetInteger(poArgs, 1, &iColor))
			return Py_BuildException();
		((UI::CTextLine*)pWindow)->SetFontColor(iColor);
	}
	else if (4 == PyTuple_Size(poArgs))
	{
		float fr;
		if (!PyTuple_GetFloat(poArgs, 1, &fr))
			return Py_BuildException();
		float fg;
		if (!PyTuple_GetFloat(poArgs, 2, &fg))
			return Py_BuildException();
		float fb;
		if (!PyTuple_GetFloat(poArgs, 3, &fb))
			return Py_BuildException();
		float fa = 1.0f;

		BYTE argb[4] =
		{
			(BYTE) (255.0f * fb),
			(BYTE) (255.0f * fg),
			(BYTE) (255.0f * fr),
			(BYTE) (255.0f * fa)
		};
		((UI::CTextLine*)pWindow)->SetFontColor(*((DWORD *) argb));
	}
	else
	{
		return Py_BuildException();
	}

	return Py_BuildNone();
}
PyObject * wndTextSetLimitWidth(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fWidth;
	if (!PyTuple_GetFloat(poArgs, 1, &fWidth))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetLimitWidth(fWidth);
	return Py_BuildNone();
}
PyObject * wndTextSetText(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szText;
	if (!PyTuple_GetString(poArgs, 1, &szText))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetText(szText);
	return Py_BuildNone();
}

PyObject * wndTextSetFixedRenderPos(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int startPos, endPos;
	if (!PyTuple_GetInteger(poArgs, 1, &startPos))
		return Py_BuildException();
	if (!PyTuple_GetInteger(poArgs, 2, &endPos))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetFixedRenderPos(startPos, endPos);
	return Py_BuildNone();
}

PyObject * wndTextGetRenderPos(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	WORD startPos, endPos;

	((UI::CTextLine*)pWindow)->GetRenderPositions(startPos, endPos);
	return Py_BuildValue("ii", startPos, endPos);
}

PyObject * wndTextGetText(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("s", ((UI::CTextLine*)pWindow)->GetText());
}

PyObject * wndTextGetTextSize(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int nWidth;
	int nHeight;

	UI::CTextLine* pkTextLine=(UI::CTextLine*)pWindow;
	pkTextLine->GetTextSize(&nWidth, &nHeight);

	return Py_BuildValue("(ii)", nWidth, nHeight);
}


PyObject * wndTextShowCursor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->ShowCursor();
	return Py_BuildNone();
}
PyObject * wndTextHideCursor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->HideCursor();
	return Py_BuildNone();
}
PyObject * wndTextIsShowCursor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("b", ((UI::CTextLine*)pWindow)->IsShowCursor());
}
PyObject * wndTextGetCursorPosition(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("i", ((UI::CTextLine*)pWindow)->GetCursorPosition());
}

#ifdef __ARABIC_LANG__
PyObject * wndTextSetInverse(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CTextLine*)pWindow)->SetInverse();
	return Py_BuildNone();
}
#endif

PyObject * wndNumberSetNumber(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szNumber;
	if (!PyTuple_GetString(poArgs, 1, &szNumber))
		return Py_BuildException();

	((UI::CNumberLine*)pWindow)->SetNumber(szNumber);

	return Py_BuildNone();
}

PyObject * wndNumberSetNumberHorizontalAlignCenter(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CNumberLine*)pWindow)->SetHorizontalAlign(UI::CWindow::HORIZONTAL_ALIGN_CENTER);

	return Py_BuildNone();
}

PyObject * wndNumberSetNumberHorizontalAlignRight(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CNumberLine*)pWindow)->SetHorizontalAlign(UI::CWindow::HORIZONTAL_ALIGN_RIGHT);

	return Py_BuildNone();
}

PyObject * wndNumberSetPath(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szPath;
	if (!PyTuple_GetString(poArgs, 1, &szPath))
		return Py_BuildException();

	((UI::CNumberLine*)pWindow)->SetPath(szPath);

	return Py_BuildNone();
}

PyObject * wndMarkBox_SetImageFilename(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	((UI::CMarkBox*)pWindow)->LoadImage(szFileName);
	return Py_BuildNone();
}

PyObject * wndMarkBox_SetImage(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildNone();
}

PyObject * wndMarkBox_Load(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildNone();
}

PyObject * wndMarkBox_SetIndex(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int nIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &nIndex))
		return Py_BuildException();

	((UI::CMarkBox*)pWindow)->SetIndex(nIndex);
	return Py_BuildNone();
}

PyObject * wndMarkBox_SetScale(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	float fScale;
	if (!PyTuple_GetFloat(poArgs, 1, &fScale))
		return Py_BuildException();

	((UI::CMarkBox*)pWindow)->SetScale(fScale);
	return Py_BuildNone();
}


PyObject * wndMarkBox_SetDiffuseColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fr;
	if (!PyTuple_GetFloat(poArgs, 1, &fr))
		return Py_BuildException();
	float fg;
	if (!PyTuple_GetFloat(poArgs, 2, &fg))
		return Py_BuildException();
	float fb;
	if (!PyTuple_GetFloat(poArgs, 3, &fb))
		return Py_BuildException();
	float fa;
	if (!PyTuple_GetFloat(poArgs, 4, &fa))
		return Py_BuildException();

	((UI::CMarkBox*)pWindow)->SetDiffuseColor(fr, fg, fb, fa);

	return Py_BuildNone();
}


PyObject * wndImageLoadImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	if (!((UI::CImageBox*)pWindow)->LoadImage(szFileName))
		//return Py_BuildException("Failed to load image (filename: %s)", szFileName);
		return Py_BuildValue("b", false);

	return Py_BuildValue("b", true);
}

PyObject * wndImageSetImageCoolTime(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException("no window");
	float fDuration;
	if (!PyTuple_GetFloat(poArgs, 1, &fDuration))
		return Py_BuildException("no duration");

	((UI::CImageBox*)pWindow)->SetCoolTime(fDuration);
	return Py_BuildNone();
}

PyObject * wndImageLoadScaledImageByPtr(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException("no window");
	int iImageHandle;
	if (!PyTuple_GetInteger(poArgs, 1, &iImageHandle))
		return Py_BuildException();
	int iWidth;
	if (!PyTuple_GetInteger(poArgs, 2, &iWidth))
		return Py_BuildException();
	int iHeight;
	if (!PyTuple_GetInteger(poArgs, 3, &iHeight))
		return Py_BuildException();

	CGraphicImage* pScaledImage;
	if (!UI::CWindowManager::Instance().GetScaledImagePtr((CGraphicImage*)iImageHandle, iWidth, iHeight, &pScaledImage))
	{
		TraceError("could not load scaled image by ptr");
		return Py_BuildValue("b", false);
	}

	bool bRet = ((UI::CImageBox*)pWindow)->LoadImage(pScaledImage);
	return Py_BuildValue("b", bRet);
}

PyObject * wndImageLoadImagePtr(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iImageHandle;
	if (!PyTuple_GetInteger(poArgs, 1, &iImageHandle))
		return Py_BuildException();

	if (!((UI::CImageBox*)pWindow)->LoadImage((CGraphicImage*)iImageHandle))
		//return Py_BuildException("Failed to load image (filename: %s)", szFileName);
		return Py_BuildValue("b", false);

	return Py_BuildValue("b", true);
}

PyObject * wndImageHideImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CImageBox*)pWindow)->HideImage();
	return Py_BuildNone();
}

PyObject * wndImageShowImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CImageBox*)pWindow)->ShowImage();
	return Py_BuildNone();
}

PyObject * wndImageSetDiffuseColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fr;
	if (!PyTuple_GetFloat(poArgs, 1, &fr))
		return Py_BuildException();
	float fg;
	if (!PyTuple_GetFloat(poArgs, 2, &fg))
		return Py_BuildException();
	float fb;
	if (!PyTuple_GetFloat(poArgs, 3, &fb))
		return Py_BuildException();
	float fa;
	if (!PyTuple_GetFloat(poArgs, 4, &fa))
		return Py_BuildException();

	if (pWindow->IsType(UI::CImageBox::Type()) || pWindow->IsType(UI::CExpandedImageBox::Type()))
		((UI::CImageBox*)pWindow)->SetDiffuseColor(fr, fg, fb, fa);
	else if (pWindow->IsType(UI::CAniImageBox::Type()))
		((UI::CAniImageBox*)pWindow)->SetDiffuseColor(fr, fg, fb, fa);
	else if (pWindow->IsType(UI::CButton::Type()))
		((UI::CButton*)pWindow)->SetDiffuseColor(fr, fg, fb, fa);

	return Py_BuildNone();
}

PyObject * wndImageGetWidth(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("i", ((UI::CImageBox*)pWindow)->GetWidth());
}

PyObject * wndImageGetHeight(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("i", ((UI::CImageBox*)pWindow)->GetHeight());
}

PyObject * wndImageSetScale(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fx;
	if (!PyTuple_GetFloat(poArgs, 1, &fx))
		return Py_BuildException();
	float fy;
	if (!PyTuple_GetFloat(poArgs, 2, &fy))
		return Py_BuildException();

	if (pWindow->IsType(UI::CExpandedImageBox::Type()))
		((UI::CExpandedImageBox*)pWindow)->SetScale(fx, fy);
	else if (pWindow->IsType(UI::CImageBox::Type()))
		((UI::CImageBox*)pWindow)->SetScale(fx, fy);

	return Py_BuildNone();
}
PyObject * wndImageSetOrigin(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fx;
	if (!PyTuple_GetFloat(poArgs, 1, &fx))
		return Py_BuildException();
	float fy;
	if (!PyTuple_GetFloat(poArgs, 2, &fy))
		return Py_BuildException();

	((UI::CExpandedImageBox*)pWindow)->SetOrigin(fx, fy);

	return Py_BuildNone();
}
PyObject * wndImageSetRotation(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fRotation;
	if (!PyTuple_GetFloat(poArgs, 1, &fRotation))
		return Py_BuildException();

	((UI::CExpandedImageBox*)pWindow)->SetRotation(fRotation);

	return Py_BuildNone();
}
PyObject * wndImageSetExpandedRenderingRect(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fLeftTop;
	if (!PyTuple_GetFloat(poArgs, 1, &fLeftTop))
		return Py_BadArgument();
	float fLeftBottom;
	if (!PyTuple_GetFloat(poArgs, 2, &fLeftBottom))
		return Py_BadArgument();
	float fTopLeft;
	if (!PyTuple_GetFloat(poArgs, 3, &fTopLeft))
		return Py_BadArgument();
	float fTopRight;
	if (!PyTuple_GetFloat(poArgs, 4, &fTopRight))
		return Py_BadArgument();
	float fRightTop;
	if (!PyTuple_GetFloat(poArgs, 5, &fRightTop))
		return Py_BadArgument();
	float fRightBottom;
	if (!PyTuple_GetFloat(poArgs, 6, &fRightBottom))
		return Py_BadArgument();
	float fBottomLeft;
	if (!PyTuple_GetFloat(poArgs, 7, &fBottomLeft))
		return Py_BadArgument();
	float fBottomRight;
	if (!PyTuple_GetFloat(poArgs, 8, &fBottomRight))
		return Py_BadArgument();

	if (pWindow->IsType(UI::CExpandedImageBox::Type()))
		((UI::CExpandedImageBox*)pWindow)->SetExpandedRenderingRect(fLeftTop, fLeftBottom, fTopLeft, fTopRight, fRightTop, fRightBottom, fBottomLeft, fBottomRight);

	return Py_BuildNone();
}
PyObject * wndImageSetTextureRenderingRect(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fLeft;
	if (!PyTuple_GetFloat(poArgs, 1, &fLeft))
		return Py_BuildException();
	float fTop;
	if (!PyTuple_GetFloat(poArgs, 2, &fTop))
		return Py_BuildException();
	float fRight;
	if (!PyTuple_GetFloat(poArgs, 3, &fRight))
		return Py_BuildException();
	float fBottom;
	if (!PyTuple_GetFloat(poArgs, 4, &fBottom))
		return Py_BuildException();

	if (pWindow->IsType(UI::CExpandedImageBox::Type()))
		((UI::CExpandedImageBox*)pWindow)->SetTextureRenderingRect(fLeft, fTop, fRight, fBottom);

	return Py_BuildNone();
}
PyObject * wndImageSetRenderingMode(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iMode;
	if (!PyTuple_GetInteger(poArgs, 1, &iMode))
		return Py_BuildException();

	if (pWindow->IsType(UI::CExpandedImageBox::Type()))
		((UI::CExpandedImageBox*)pWindow)->SetRenderingMode(iMode);

	return Py_BuildNone();
}

PyObject * wndImageGetPixelColor(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int x;
	if (!PyTuple_GetInteger(poArgs, 1, &x))
		return Py_BuildException();
	int y;
	if (!PyTuple_GetInteger(poArgs, 2, &y))
		return Py_BuildException();

	DWORD dwColor = ((UI::CExpandedImageBox*)pWindow)->GetPixelColor(x, y);
	BYTE bR = GetRValue(dwColor);
	BYTE bG = GetGValue(dwColor);
	BYTE bB = GetBValue(dwColor);
	BYTE bA = (LOBYTE((dwColor)>>24));
	return Py_BuildValue("iiii", bR, bG, bB, bA);
}

PyObject * wndImageSetDelay(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	float fDelay;
	if (!PyTuple_GetFloat(poArgs, 1, &fDelay))
		return Py_BuildException();

	((UI::CAniImageBox*)pWindow)->SetDelay(fDelay);

	return Py_BuildNone();
}
PyObject * wndImageSetSkipCount(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iSkipCount;
	if (!PyTuple_GetInteger(poArgs, 1, &iSkipCount))
		return Py_BuildException();

	((UI::CAniImageBox*)pWindow)->SetSkipCount(iSkipCount);

	return Py_BuildNone();
}
PyObject * wndImageGetFrameIndex(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("i", ((UI::CAniImageBox*)pWindow)->GetFrameIndex());
}
PyObject * wndImageSetTimedDelay(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int iDelay;
	if (!PyTuple_GetInteger(poArgs, 1, &iDelay))
		return Py_BuildException();

	((UI::CAniImageBox*)pWindow)->SetTimedDelay(iDelay);

	return Py_BuildNone();
}
PyObject * wndImageLoadGIFImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	bool bRet = ((UI::CAniImageBox*)pWindow)->LoadGIFImage(szFileName);

	return Py_BuildValue("b", bRet);
}
PyObject * wndImageLoadScaledGIFImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();
	int iMaxWidth;
	if (!PyTuple_GetInteger(poArgs, 2, &iMaxWidth))
		return Py_BuildException();
	int iMaxHeight;
	if (!PyTuple_GetInteger(poArgs, 3, &iMaxHeight))
		return Py_BuildException();

	bool bRet = ((UI::CAniImageBox*)pWindow)->LoadScaledGIFImage(szFileName, iMaxWidth, iMaxHeight);

	return Py_BuildValue("b", bRet);
}
PyObject * wndImageAppendImage(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	((UI::CAniImageBox*)pWindow)->AppendImage(szFileName);

	return Py_BuildNone();
}
PyObject * wndImageResetFrame(PyObject * poSelf, PyObject * poArgs)
{
    UI::CWindow * pWindow;
    if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
        return Py_BuildException();

    ((UI::CAniImageBox*)pWindow)->ResetFrame();

    return Py_BuildNone();
}
PyObject * wndImageClearImages(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	
	((UI::CAniImageBox*)pWindow)->ClearImages();

	return Py_BuildNone();
}

PyObject * wndButtonSetUpVisual(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	((UI::CButton*)pWindow)->SetUpVisual(szFileName);

	return Py_BuildNone();
}
PyObject * wndButtonSetOverVisual(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	((UI::CButton*)pWindow)->SetOverVisual(szFileName);

	return Py_BuildNone();
}
PyObject * wndButtonSetDownVisual(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	((UI::CButton*)pWindow)->SetDownVisual(szFileName);

	return Py_BuildNone();
}
PyObject * wndButtonSetDisableVisual(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	((UI::CButton*)pWindow)->SetDisableVisual(szFileName);

	return Py_BuildNone();
}
PyObject * wndButtonGetUpVisualFileName(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("s", ((UI::CButton*)pWindow)->GetUpVisualFileName());
}
PyObject * wndButtonGetOverVisualFileName(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("s", ((UI::CButton*)pWindow)->GetOverVisualFileName());
}

#ifdef COMBAT_ZONE
PyObject * wndButtonFlashEx(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->FlashEx();

	return Py_BuildNone();
}
#endif

PyObject * wndButtonGetDownVisualFileName(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("s", ((UI::CButton*)pWindow)->GetDownVisualFileName());
}
PyObject * wndButtonFlash(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->Flash();

	return Py_BuildNone();
}
PyObject * wndButtonEnable(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->Enable();

	return Py_BuildNone();
}
PyObject * wndButtonDisable(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->Disable();

	return Py_BuildNone();
}
PyObject * wndButtonIsEnabled(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("b", ((UI::CButton*)pWindow)->IsEnabled());
}
PyObject * wndButtonDown(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->Down();

	return Py_BuildNone();
}
PyObject * wndButtonSetUp(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->SetUp();

	return Py_BuildNone();
}
PyObject * wndButtonSetRestrictMovementArea(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();
	int ix;
	if (!PyTuple_GetInteger(poArgs, 1, &ix))
		return Py_BuildException();
	int iy;
	if (!PyTuple_GetInteger(poArgs, 2, &iy))
		return Py_BuildException();
	int iwidth;
	if (!PyTuple_GetInteger(poArgs, 3, &iwidth))
		return Py_BuildException();
	int iheight;
	if (!PyTuple_GetInteger(poArgs, 4, &iheight))
		return Py_BuildException();

	((UI::CDragButton*)pWindow)->SetRestrictMovementArea(ix, iy, iwidth, iheight);

	return Py_BuildNone();
}

PyObject * wndButtonIsDown(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	return Py_BuildValue("i", ((UI::CButton*)pWindow)->IsPressed());
}

#ifdef AHMET_FISH_EVENT_SYSTEM
PyObject * wndMgrSetPickedAreaRender(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if(!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int iFlag;
	if(!PyTuple_GetInteger(poArgs, 1, &iFlag))
		return Py_BuildException();

	if(!pWin->IsType(UI::CGridSlotWindow::Type()))
	{
		TraceError("wndMgr.SetPickedAreaRender : not a grid window");
		return Py_BuildException();
	}

	UI::CGridSlotWindow * pGridSlotWin = ( UI::CGridSlotWindow * )pWin;
	pGridSlotWin->SetPickedAreaRender(iFlag ? true : false);

	return Py_BuildNone();
}
PyObject * wndMgrDeleteCoverButton(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if(!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotIndex;
	if(!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = ( UI::CSlotWindow * )pWindow;
	pSlotWin->DeleteCoverButton(iSlotIndex);

	return Py_BuildNone();
}
PyObject * wndMgrSetDisableDeattach(PyObject * poSelf, PyObject * poArgs)
{
	BOOL bFlag;
	if(!PyTuple_GetInteger(poArgs, 0, &bFlag))
		return Py_BuildException();

	UI::CWindowManager::Instance().SetDisableDeattach(bFlag ? true : false);
	return Py_BuildNone();
}
#endif

PyObject * wndButtonOver(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	((UI::CButton*)pWindow)->Over();
	return Py_BuildNone();
}

extern BOOL g_bOutlineBoxEnable;
extern BOOL g_bShowOverInWindowName;

PyObject * wndMgrSetOutlineFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BuildException();

	g_bOutlineBoxEnable = iFlag;

	return Py_BuildNone();
}

PyObject * wndMgrShowOverInWindowName(PyObject * poSelf, PyObject * poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BuildException();

	g_bShowOverInWindowName = iFlag;

	return Py_BuildNone();
}

PyObject * wndMgrSetCardSlot(PyObject * poSelf, PyObject * poArgs)
{
    UI::CWindow * pWin;
    if (!PyTuple_GetWindow(poArgs, 0, &pWin))
        return Py_BuildException();

    int iSlotIndex;
    if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
        return Py_BuildException();

    int iItemIndex;
    if (!PyTuple_GetInteger(poArgs, 2, &iItemIndex))
        return Py_BuildException();

    int iWidth;
    if (!PyTuple_GetInteger(poArgs, 3, &iWidth))
        return Py_BuildException();

    int iHeight;
    if (!PyTuple_GetInteger(poArgs, 4, &iHeight))
        return Py_BuildException();

    char * c_szFileName;
    if (!PyTuple_GetString(poArgs, 5, &c_szFileName))
        return Py_BuildException();

    D3DXCOLOR diffuseColor;
    PyObject* pTuple;
    if (!PyTuple_GetObject(poArgs, 6, &pTuple))
    {
        diffuseColor = D3DXCOLOR(1.0, 1.0, 1.0, 1.0);
        //return Py_BuildException();
    }
    else
    // get diffuse color from pTuple
    {
        if (PyTuple_Size(pTuple) != 4)
            return Py_BuildException();
        if (!PyTuple_GetFloat(pTuple, 0, &diffuseColor.r))
            return Py_BuildException();
        if (!PyTuple_GetFloat(pTuple, 1, &diffuseColor.g))
            return Py_BuildException();
        if (!PyTuple_GetFloat(pTuple, 2, &diffuseColor.b))
            return Py_BuildException();
        if (!PyTuple_GetFloat(pTuple, 3, &diffuseColor.a))
            return Py_BuildException();
    }

    if (!pWin->IsType(UI::CSlotWindow::Type()))
        return Py_BuildException();

    UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWin;
    pSlotWin->SetCardSlot(iSlotIndex, iItemIndex, iWidth, iHeight, c_szFileName, diffuseColor);

    return Py_BuildNone();
}

PyObject * wndMgrSetRenderTarget(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int renderWindow = -1;
	if (!PyTuple_GetInteger(poArgs, 1, &renderWindow))
		return Py_BuildException();

	((UI::CRenderTarget*)pWin)->SetRenderTarget(renderWindow);
	return Py_BuildNone();
}

PyObject * wndMgrSetWikiRenderTarget(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	int renderWindow = -1;
	if (!PyTuple_GetInteger(poArgs, 1, &renderWindow))
		return Py_BuildException();

	((UI::CRenderTarget*)pWin)->SetWikiRenderTarget(renderWindow);
	return Py_BuildNone();
}

PyObject * wndDisplayImageProcent(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWindow;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	float procent = 0.0f;
	if (!PyTuple_GetFloat(poArgs, 1, &procent))
		return Py_BuildException();

	((UI::CImageBox*)pWindow)->DisplayImageProcent(procent);
	return Py_BuildNone();
}

PyObject * wndMgrSetInsideRender(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	bool val;
	if (!PyTuple_GetBoolean(poArgs, 1, &val))
		return Py_BuildException();

	pWin->SetInsideRender(val);
	return Py_BuildNone();
}

PyObject * wndMgrGetRenderBox(PyObject * poSelf, PyObject * poArgs)
{
	UI::CWindow * pWin;
	if (!PyTuple_GetWindow(poArgs, 0, &pWin))
		return Py_BuildException();

	RECT val;
	pWin->GetRenderBox(&val);
	return Py_BuildValue("iiii", val.left, val.top, val.right, val.bottom);
}

PyObject* wndMgrIsEmptySlot(PyObject* poSelf, PyObject* poArgs)
{
	UI::CWindow* pWindow = nullptr;
	if (!PyTuple_GetWindow(poArgs, 0, &pWindow))
		return Py_BuildException();

	int iSlotNumber = 0;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotNumber))
		return Py_BuildException();

	UI::CSlotWindow * pSlotWin = (UI::CSlotWindow *)pWindow;

	return Py_BuildValue("i", pSlotWin->IsEmptySlot(iSlotNumber));
}


void initwndMgr()
{
	static PyMethodDef s_methods[] =
	{
		// WindowManager
		{ "DisplayImageProcent",		wndDisplayImageProcent,				METH_VARARGS },
		{ "SetMouseHandler",			wndMgrSetMouseHandler,				METH_VARARGS },
		{ "SetScreenSize",				wndMgrSetScreenSize,				METH_VARARGS },
		{ "GetScreenWidth",				wndMgrGetScreenWidth,				METH_VARARGS },
		{ "GetScreenHeight",			wndMgrGetScreenHeight,				METH_VARARGS },
		{ "AttachIcon",					wndMgrAttachIcon,					METH_VARARGS },
		{ "DeattachIcon",				wndMgrDeattachIcon,					METH_VARARGS },
		{ "SetAttachingFlag",			wndMgrSetAttachingFlag,				METH_VARARGS },
		{ "GetAspect",					wndMgrGetAspect,					METH_VARARGS },
		{ "GetHyperlink",				wndMgrGetHyperlink,					METH_VARARGS },
		{ "OnceIgnoreMouseLeftButtonUpEvent",	wndMgrOnceIgnoreMouseLeftButtonUpEvent,		METH_VARARGS },

		// Window
		{ "Register",					wndMgrRegister,						METH_VARARGS },
		{ "RegisterSlotWindow",			wndMgrRegisterSlotWindow,			METH_VARARGS },
		{ "RegisterGridSlotWindow",		wndMgrRegisterGridSlotWindow,		METH_VARARGS },
		{ "RegisterTextLine",			wndMgrRegisterTextLine,				METH_VARARGS },
		{ "RegisterMarkBox",			wndMgrRegisterMarkBox,				METH_VARARGS },
		{ "RegisterImageBox",			wndMgrRegisterImageBox,				METH_VARARGS },
		{ "RegisterExpandedImageBox",	wndMgrRegisterExpandedImageBox,		METH_VARARGS },
		{ "RegisterAniImageBox",		wndMgrRegisterAniImageBox,			METH_VARARGS },
		{ "RegisterButton",				wndMgrRegisterButton,				METH_VARARGS },
		{ "RegisterRadioButton",		wndMgrRegisterRadioButton,			METH_VARARGS },
		{ "RegisterToggleButton",		wndMgrRegisterToggleButton,			METH_VARARGS },
		{ "RegisterDragButton",			wndMgrRegisterDragButton,			METH_VARARGS },
		{ "RegisterBox",				wndMgrRegisterBox,					METH_VARARGS },
		{ "RegisterBar",				wndMgrRegisterBar,					METH_VARARGS },
		{ "RegisterLine",				wndMgrRegisterLine,					METH_VARARGS },
		{ "RegisterBar3D",				wndMgrRegisterBar3D,				METH_VARARGS },
		{ "RegisterNumberLine",			wndMgrRegisterNumberLine,			METH_VARARGS },
		{ "RegisterRenderTarget",		wndMgrRegisterRenderTarget,			METH_VARARGS },
		{ "Destroy",					wndMgrDestroy,						METH_VARARGS },
		{ "AddFlag",					wndMgrAddFlag,						METH_VARARGS },
		{ "IsRTL",						wndMgrIsRTL,						METH_VARARGS },
		{ "SetAlpha",					wndMgrSetAlpha,						METH_VARARGS },
		{ "GetAlpha",					wndMgrGetAlpha,						METH_VARARGS },
		{ "SetAllAlpha",				wndMgrSetAllAlpha,					METH_VARARGS },

		// Base Window
		{ "SetName",					wndMgrSetName,						METH_VARARGS },
		{ "GetName",					wndMgrGetName,						METH_VARARGS },

		{ "SetTop",						wndMgrSetTop,						METH_VARARGS },
		{ "Show",						wndMgrShow,							METH_VARARGS },
		{ "Hide",						wndMgrHide,							METH_VARARGS },
		{ "IsShow",						wndMgrIsShow,						METH_VARARGS },
		{ "SetParent",					wndMgrSetParent,					METH_VARARGS },
		{ "SetPickAlways",				wndMgrSetPickAlways,				METH_VARARGS },
		{ "SetInsideRender",			wndMgrSetInsideRender,				METH_VARARGS },

		{ "IsFocus",					wndMgrIsFocus,						METH_VARARGS },
		{ "SetFocus",					wndMgrSetFocus,						METH_VARARGS },
		{ "KillFocus",					wndMgrKillFocus,					METH_VARARGS },
		{ "Lock",						wndMgrLock,							METH_VARARGS },
		{ "Unlock",						wndMgrUnlock,						METH_VARARGS },

		{ "SetWindowSize",				wndMgrSetWndSize,					METH_VARARGS },
		{ "SetWindowPosition",			wndMgrSetWndPosition,				METH_VARARGS },
		{ "GetWindowWidth",				wndMgrGetWndWidth,					METH_VARARGS },
		{ "GetWindowHeight",			wndMgrGetWndHeight,					METH_VARARGS },
		{ "GetWindowLocalPosition",		wndMgrGetWndLocalPosition,			METH_VARARGS },
		{ "GetWindowGlobalPosition",	wndMgrGetWndGlobalPosition,			METH_VARARGS },
		{ "GetWindowRect",				wndMgrGetWindowRect,				METH_VARARGS },
		{ "SetWindowHorizontalAlign",	wndMgrSetWindowHorizontalAlign,		METH_VARARGS },
		{ "SetWindowVerticalAlign",		wndMgrSetWindowVerticalAlign,		METH_VARARGS },

		{ "GetChildCount",				wndMgrGetChildCount,				METH_VARARGS },

		{ "IsPickedWindow",				wndMgrIsPickedWindow,				METH_VARARGS },
		{ "IsIn",						wndMgrIsIn,							METH_VARARGS },
		{ "GetMouseLocalPosition",		wndMgrGetMouseLocalPosition,		METH_VARARGS },
		{ "IsInCircle",					wndMgrIsInCircle,					METH_VARARGS },
		{ "IsMouseInCircle",			wndMgrIsMouseInCircle,				METH_VARARGS },
		{ "GetColorAtPosition",			wndMgrGetColorAtPosition,			METH_VARARGS },
		{ "GetMousePosition",			wndMgrGetMousePosition,				METH_VARARGS },
		{ "GetRealMousePosition",		wndMgrGetRealMousePosition,			METH_VARARGS },
		{ "IsDragging",					wndMgrIsDragging,					METH_VARARGS },

		{ "SetLimitBias",				wndMgrSetLimitBias,					METH_VARARGS },

		{ "UpdateRect",					wndMgrUpdateRect,					METH_VARARGS },

		// Slot Window
		{ "AppendSlot",					wndMgrAppendSlot,					METH_VARARGS },
		{ "ArrangeSlot",				wndMgrArrangeSlot,					METH_VARARGS },
		{ "ClearSlot",					wndMgrClearSlot,					METH_VARARGS },
		{ "ClearAllSlot",				wndMgrClearAllSlot,					METH_VARARGS },
		{ "HasSlot",					wndMgrHasSlot,						METH_VARARGS },
		{ "SetSlot",					wndMgrSetSlot,						METH_VARARGS },
		{ "SetCardSlot",                wndMgrSetCardSlot,                    METH_VARARGS },
		{ "SetSlotCount",				wndMgrSetSlotCount,					METH_VARARGS },
		{ "SetSlotCountNew",			wndMgrSetSlotCountNew,				METH_VARARGS },
		{ "SetSlotCoolTime",			wndMgrSetSlotCoolTime,				METH_VARARGS },
		{ "SetToggleSlot",				wndMgrSetToggleSlot,				METH_VARARGS },
		{ "ActivateSlot",				wndMgrActivateSlot,					METH_VARARGS },
		{ "DeactivateSlot",				wndMgrDeactivateSlot,				METH_VARARGS },
		{ "EnableSlot",					wndMgrEnableSlot,					METH_VARARGS },
		{ "DisableSlot",				wndMgrDisableSlot,					METH_VARARGS },
		{ "SetUnusableSlot",			wndMgrSetUnusableSlot,				METH_VARARGS },
		{ "SetUnusableSlotWorld",		wndMgrSetUnusableSlotWorld,			METH_VARARGS },
		{ "ShowSlotBaseImage",			wndMgrShowSlotBaseImage,			METH_VARARGS },
		{ "HideSlotBaseImage",			wndMgrHideSlotBaseImage,			METH_VARARGS },
		{ "SetSlotType",				wndMgrSetSlotType,					METH_VARARGS },
		{ "SetSlotStyle",				wndMgrSetSlotStyle,					METH_VARARGS },
		{ "SetSlotBaseImage",			wndMgrSetSlotBaseImage,				METH_VARARGS },

		{ "SetCoverButton",				wndMgrSetCoverButton,				METH_VARARGS },
		{ "EnableCoverButton",			wndMgrEnableCoverButton,			METH_VARARGS },
		{ "DisableCoverButton",			wndMgrDisableCoverButton,			METH_VARARGS },
		{ "IsDisableCoverButton",		wndMgrIsDisableCoverButton,			METH_VARARGS },
		{ "SetAlwaysRenderCoverButton",	wndMgrSetAlwaysRenderCoverButton,	METH_VARARGS },
		
		{ "AppendSlotButton",			wndMgrAppendSlotButton,				METH_VARARGS },
		{ "AppendRequirementSignImage",	wndMgrAppendRequirementSignImage,	METH_VARARGS },
		{ "ShowSlotButton",				wndMgrShowSlotButton,				METH_VARARGS },
		{ "HideAllSlotButton",			wndMgrHideAllSlotButton,			METH_VARARGS },
		{ "ShowRequirementSign",		wndMgrShowRequirementSign,			METH_VARARGS },
		{ "HideRequirementSign",		wndMgrHideRequirementSign,			METH_VARARGS },
		{ "RefreshSlot",				wndMgrRefreshSlot,					METH_VARARGS },
		{ "SetUseMode",					wndMgrSetUseMode,					METH_VARARGS },
		{ "SetUsableItem",				wndMgrSetUsableItem,				METH_VARARGS },
		{ "SetSlotDiffuseColor",		wndMgrSetSlotDiffuseColor,			METH_VARARGS },
#ifdef ENABLE_ITEM_SWAP_SYSTEM
		{ "SetUsableItem2",				wndMgrSetUsableItem2,				METH_VARARGS },
#endif

		{ "SelectSlot",					wndMgrSelectSlot,					METH_VARARGS },
		{ "ClearSelected",				wndMgrClearSelected,				METH_VARARGS },
		{ "GetSelectedSlotCount",		wndMgrGetSelectedSlotCount,			METH_VARARGS },
		{ "GetSelectedSlotNumber",		wndMgrGetSelectedSlotNumber,		METH_VARARGS },
		{ "IsSelectedSlot",				wndMgrIsSelectedSlot,				METH_VARARGS },
		{ "GetSlotCount",				wndMgrGetSlotCount,					METH_VARARGS },
		{ "LockSlot",					wndMgrLockSlot,						METH_VARARGS },
		{ "UnlockSlot",					wndMgrUnlockSlot,					METH_VARARGS },

		{ "GetSlotPosition",			wndMgrGetSlotPosition,				METH_VARARGS },
		{ "IsEmptySlot",				wndMgrIsEmptySlot,					METH_VARARGS },

		// Rendering
		{ "SetRenderingRect",			wndSetRenderingRect,				METH_VARARGS },
		{ "GetRenderBox",				wndMgrGetRenderBox,					METH_VARARGS },

		// Bar
		{ "SetColor",					wndBarSetColor,						METH_VARARGS },

		// TextLine
		{ "SetMax",						wndTextSetMax,						METH_VARARGS },
		{ "SetHorizontalAlign",			wndTextSetHorizontalAlign,			METH_VARARGS },
		{ "SetVerticalAlign",			wndTextSetVerticalAlign,			METH_VARARGS },
		{ "SetSecret",					wndTextSetSecret,					METH_VARARGS },
		{ "SetOutline",					wndTextSetOutline,					METH_VARARGS },
		{ "SetOutlineColor",			wndTextSetOutlineColor,				METH_VARARGS },
		{ "SetFeather",					wndTextSetFeather,					METH_VARARGS },
		{ "SetMultiLine",				wndTextSetMultiLine,				METH_VARARGS },
		{ "SetText",					wndTextSetText,						METH_VARARGS },
		{ "SetFontName",				wndTextSetFontName,					METH_VARARGS },
		{ "SetFontColor",				wndTextSetFontColor,				METH_VARARGS },
		{ "SetLimitWidth",				wndTextSetLimitWidth,				METH_VARARGS },
		{ "GetText",					wndTextGetText,						METH_VARARGS },
		{ "GetTextSize",				wndTextGetTextSize,					METH_VARARGS },
		{ "ShowCursor",					wndTextShowCursor,					METH_VARARGS },
		{ "HideCursor",					wndTextHideCursor,					METH_VARARGS },
		{ "IsShowCursor",				wndTextIsShowCursor,				METH_VARARGS },
		{ "GetCursorPosition",			wndTextGetCursorPosition,			METH_VARARGS },
#ifdef __ARABIC_LANG__
		{ "SetInverse",					wndTextSetInverse,					METH_VARARGS },
#endif
		{ "GetRenderPos",				wndTextGetRenderPos,				METH_VARARGS },
		{ "SetFixedRenderPos",			wndTextSetFixedRenderPos,			METH_VARARGS },

		// NumberLine
		{ "SetNumber",						wndNumberSetNumber,							METH_VARARGS },
		{ "SetNumberHorizontalAlignCenter",	wndNumberSetNumberHorizontalAlignCenter,	METH_VARARGS },
		{ "SetNumberHorizontalAlignRight",	wndNumberSetNumberHorizontalAlignRight,		METH_VARARGS },
		{ "SetPath",						wndNumberSetPath,							METH_VARARGS },

		// ImageBox
		{ "MarkBox_SetImage",			wndMarkBox_SetImage,				METH_VARARGS },
		{ "MarkBox_SetImageFilename",	wndMarkBox_SetImageFilename,		METH_VARARGS },
		{ "MarkBox_Load",				wndMarkBox_Load,					METH_VARARGS },
		{ "MarkBox_SetIndex",			wndMarkBox_SetIndex,				METH_VARARGS },
		{ "MarkBox_SetScale",			wndMarkBox_SetScale,				METH_VARARGS },
		{ "MarkBox_SetDiffuseColor",	wndMarkBox_SetDiffuseColor,			METH_VARARGS },

		// ImageBox
		{ "LoadImage",					wndImageLoadImage,					METH_VARARGS },
		{ "SetImageCoolTime",			wndImageSetImageCoolTime,			METH_VARARGS },
		{ "LoadImagePtr",				wndImageLoadImagePtr,				METH_VARARGS },
		{ "LoadScaledImageByPtr",		wndImageLoadScaledImageByPtr,		METH_VARARGS },
		{ "HideImage",					wndImageHideImage,					METH_VARARGS },
		{ "ShowImage",					wndImageShowImage,					METH_VARARGS },
		{ "SetDiffuseColor",			wndImageSetDiffuseColor,			METH_VARARGS },
		{ "GetWidth",					wndImageGetWidth,					METH_VARARGS },
		{ "GetHeight",					wndImageGetHeight,					METH_VARARGS },
		{ "SetScale",					wndImageSetScale,					METH_VARARGS },
		// ExpandedImageBox
		{ "SetOrigin",					wndImageSetOrigin,					METH_VARARGS },
		{ "SetRotation",				wndImageSetRotation,				METH_VARARGS },
		{ "SetExpandedRenderingRect",	wndImageSetExpandedRenderingRect,	METH_VARARGS },
		{ "SetTextureRenderingRect",	wndImageSetTextureRenderingRect,	METH_VARARGS },
		{ "SetRenderingMode",			wndImageSetRenderingMode,			METH_VARARGS },
		{ "GetPixelColor",				wndImageGetPixelColor,				METH_VARARGS },
		// AniImageBox
		{ "SetDelay",					wndImageSetDelay,					METH_VARARGS },
		{ "SetSkipCount",				wndImageSetSkipCount,				METH_VARARGS },
		{ "ResetFrame",					wndImageResetFrame,					METH_VARARGS },
		{ "GetFrameIndex",				wndImageGetFrameIndex,				METH_VARARGS },
		{ "SetTimedDelay",				wndImageSetTimedDelay,				METH_VARARGS },
		{ "LoadGIFImage",				wndImageLoadGIFImage,				METH_VARARGS },
		{ "LoadScaledGIFImage",			wndImageLoadScaledGIFImage,			METH_VARARGS },
		{ "AppendImage",				wndImageAppendImage,				METH_VARARGS },
		{ "ClearImages",				wndImageClearImages,				METH_VARARGS },

		// Button
		{ "SetUpVisual",				wndButtonSetUpVisual,				METH_VARARGS },
		{ "SetOverVisual",				wndButtonSetOverVisual,				METH_VARARGS },
		{ "SetDownVisual",				wndButtonSetDownVisual,				METH_VARARGS },
		{ "SetDisableVisual",			wndButtonSetDisableVisual,			METH_VARARGS },
		{ "GetUpVisualFileName",		wndButtonGetUpVisualFileName,		METH_VARARGS },
		{ "GetOverVisualFileName",		wndButtonGetOverVisualFileName,		METH_VARARGS },
		{ "GetDownVisualFileName",		wndButtonGetDownVisualFileName,		METH_VARARGS },
#ifdef COMBAT_ZONE
		{ "FlashEx",					wndButtonFlashEx,					METH_VARARGS },
#endif
		{ "Flash",						wndButtonFlash,						METH_VARARGS },
		{ "Enable",						wndButtonEnable,					METH_VARARGS },
		{ "Disable",					wndButtonDisable,					METH_VARARGS },
		{ "IsEnabled",					wndButtonIsEnabled,					METH_VARARGS },
		{ "Down",						wndButtonDown,						METH_VARARGS },
		{ "SetUp",						wndButtonSetUp,						METH_VARARGS },
		{ "IsDown",						wndButtonIsDown,					METH_VARARGS },
		{ "Over",						wndButtonOver,						METH_VARARGS },

#ifdef AHMET_FISH_EVENT_SYSTEM
		{ "SetPickedAreaRender",		wndMgrSetPickedAreaRender,			METH_VARARGS },
		{ "DeleteCoverButton",			wndMgrDeleteCoverButton,			METH_VARARGS },
		{ "SetDisableDeattach",			wndMgrSetDisableDeattach,			METH_VARARGS },
#endif

		// DragButton
		{ "SetRestrictMovementArea",	wndButtonSetRestrictMovementArea,	METH_VARARGS },

		// RenderTarget
		{ "SetRenderTarget",			wndMgrSetRenderTarget,				METH_VARARGS },
		{ "SetWikiRenderTarget",		wndMgrSetWikiRenderTarget,			METH_VARARGS },
		{ "SetWikiRenderTarget",		wndMgrSetWikiRenderTarget,			METH_VARARGS },
			

		// For Debug
		{ "SetOutlineFlag",				wndMgrSetOutlineFlag,				METH_VARARGS },
		{ "ShowOverInWindowName",		wndMgrShowOverInWindowName,			METH_VARARGS },

		{ NULL,							NULL,								NULL },
	};

	PyObject * poModule = Py_InitModule("wndMgr", s_methods);


//	PyObject * poMgrModule = Py_InitModule("wndMgr", s_methods);
//	PyObject * poTextModule = Py_InitModule("wndText", s_methods);
//	PyObject * poSlotModule = Py_InitModule("wndSlot", s_methods);

	PyModule_AddIntConstant(poModule, "SLOT_STYLE_NONE",				UI::SLOT_STYLE_NONE);
	PyModule_AddIntConstant(poModule, "SLOT_STYLE_PICK_UP",				UI::SLOT_STYLE_PICK_UP);
	PyModule_AddIntConstant(poModule, "SLOT_STYLE_SELECT",				UI::SLOT_STYLE_SELECT);

	PyModule_AddIntConstant(poModule, "TEXT_HORIZONTAL_ALIGN_LEFT",		CGraphicTextInstance::HORIZONTAL_ALIGN_LEFT);
	PyModule_AddIntConstant(poModule, "TEXT_HORIZONTAL_ALIGN_RIGHT",	CGraphicTextInstance::HORIZONTAL_ALIGN_RIGHT);
	PyModule_AddIntConstant(poModule, "TEXT_HORIZONTAL_ALIGN_CENTER",	CGraphicTextInstance::HORIZONTAL_ALIGN_CENTER);
	PyModule_AddIntConstant(poModule, "TEXT_VERTICAL_ALIGN_TOP",		CGraphicTextInstance::VERTICAL_ALIGN_TOP);
	PyModule_AddIntConstant(poModule, "TEXT_VERTICAL_ALIGN_BOTTOM",		CGraphicTextInstance::VERTICAL_ALIGN_BOTTOM);
	PyModule_AddIntConstant(poModule, "TEXT_VERTICAL_ALIGN_CENTER",		CGraphicTextInstance::VERTICAL_ALIGN_CENTER);

	PyModule_AddIntConstant(poModule, "HORIZONTAL_ALIGN_LEFT",			UI::CWindow::HORIZONTAL_ALIGN_LEFT);
	PyModule_AddIntConstant(poModule, "HORIZONTAL_ALIGN_CENTER",		UI::CWindow::HORIZONTAL_ALIGN_CENTER);
	PyModule_AddIntConstant(poModule, "HORIZONTAL_ALIGN_RIGHT",			UI::CWindow::HORIZONTAL_ALIGN_RIGHT);
	PyModule_AddIntConstant(poModule, "VERTICAL_ALIGN_TOP",				UI::CWindow::VERTICAL_ALIGN_TOP);
	PyModule_AddIntConstant(poModule, "VERTICAL_ALIGN_CENTER",			UI::CWindow::VERTICAL_ALIGN_CENTER);
	PyModule_AddIntConstant(poModule, "VERTICAL_ALIGN_BOTTOM",			UI::CWindow::VERTICAL_ALIGN_BOTTOM);

	PyModule_AddIntConstant(poModule, "RENDERING_MODE_MODULATE",		CGraphicExpandedImageInstance::RENDERING_MODE_MODULATE);

	PyModule_AddIntConstant(poModule, "COLOR_TYPE_ORANGE", UI::SLOT_COLOR_TYPE_ORANGE);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_WHITE", UI::SLOT_COLOR_TYPE_WHITE);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_RED", UI::SLOT_COLOR_TYPE_RED);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_GREEN", UI::SLOT_COLOR_TYPE_GREEN);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_YELLOW", UI::SLOT_COLOR_TYPE_YELLOW);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_SKY", UI::SLOT_COLOR_TYPE_SKY);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_PINK", UI::SLOT_COLOR_TYPE_PINK);
	PyModule_AddIntConstant(poModule, "COLOR_TYPE_DARK_RED", UI::SLOT_COLOR_TYPE_DARK_RED);
}
