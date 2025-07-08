#include "StdAfx.h"
#include "Resource.h"
#include "PythonApplication.h"
#include "PythonNetworkStream.h"
#include "../EterLib/Camera.h"
#include <Shellapi.h>

extern bool PERF_CHECKER_RENDER_GAME;
extern D3DXCOLOR g_fSpecularColor;
extern BOOL bVisibleNotice = true;
extern BOOL bTestServerFlag = FALSE;
extern int TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE = 0;

#ifdef ENABLE_LEGENDARY_SKILL
extern bool g_bUseLegendarySkills;
#endif

int g_iUseNewFont = 0;

#ifdef ENABLE_NEW_WEBBROWSER
PyObject* appCreateWebPage(PyObject* poSelf, PyObject* poArgs)
{
	char* szWebTypeName;
	if (!PyTuple_GetString(poArgs, 0, &szWebTypeName))
		return Py_BuildException();
	int iWebWidth = 0;
	PyTuple_GetInteger(poArgs, 1, &iWebWidth);
	int iWebHeight = 0;
	PyTuple_GetInteger(poArgs, 2, &iWebHeight);

	CPythonNewWeb::Instance().Create(szWebTypeName, iWebWidth, iWebHeight);
	return Py_BuildNone();
}

PyObject* appPreLoadWebPage(PyObject* poSelf, PyObject* poArgs)
{
	char* szWebTypeName;
	if (!PyTuple_GetString(poArgs, 0, &szWebTypeName))
		return Py_BuildException();
	char* szWebPage;
	if (!PyTuple_GetString(poArgs, 1, &szWebPage))
		return Py_BuildException();
	int iWebWidth = 0;
	PyTuple_GetInteger(poArgs, 2, &iWebWidth);
	int iWebHeight = 0;
	PyTuple_GetInteger(poArgs, 3, &iWebHeight);

	CPythonNewWeb::Instance().PreLoad(szWebTypeName, szWebPage, iWebWidth, iWebHeight);
	return Py_BuildNone();
}

PyObject* appWebExecuteJavascript(PyObject* poSelf, PyObject* poArgs)
{
	char* szJavascriptCode;
	if (!PyTuple_GetString(poArgs, 0, &szJavascriptCode))
		return Py_BuildException();

	CPythonNewWeb::Instance().ExecuteJavascript(szJavascriptCode);
	return Py_BuildNone();
}

PyObject* appWebResize(PyObject* poSelf, PyObject* poArgs)
{
	int iWidth;
	if (!PyTuple_GetInteger(poArgs, 0, &iWidth))
		return Py_BuildNone();
	int iHeight;
	if (!PyTuple_GetInteger(poArgs, 1, &iHeight))
		return Py_BuildNone();

	CPythonNewWeb::Instance().WindowResize(iWidth, iHeight);

	return Py_BuildNone();
}

PyObject* appWebGetWindowSize(PyObject* poSelf, PyObject* poArgs)
{
	int iWidth, iHeight;
	CPythonNewWeb::Instance().GetWindowSize(iWidth, iHeight);

	return Py_BuildValue("ii", iWidth, iHeight);
}

PyObject* appWebGetCurrentURL(PyObject* poSelf, PyObject* poArgs)
{
	const char* c_pszURL = CPythonNewWeb::Instance().GetWindowURL();
	return Py_BuildValue("s", c_pszURL);
}

PyObject* appSetWebEventHandler(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poHandler;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BadArgument();

	CPythonNewWeb::Instance().SetCurrentEventHandle(poHandler);
	return Py_BuildNone();
}

#ifdef ENABLE_VOTEBOT
PyObject* appWebNewVoteSystem(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNewWeb::Instance().TryAutoVote();
	return Py_BuildNone();
}
#endif

#ifdef WEBBROWSER_OFFSCREEN
PyObject* appCallWebEvent(PyObject* poSelf, PyObject* poArgs)
{
	BYTE bEventNum;
	if (!PyTuple_GetByte(poArgs, 0, &bEventNum))
		return Py_BadArgument();

	switch (bEventNum)
	{
	case CPythonNewWeb::EVENT_MOUSE_MOVE:
	{
		int iMouseX;
		if (!PyTuple_GetInteger(poArgs, 1, &iMouseX))
			return Py_BadArgument();
		int iMouseY;
		if (!PyTuple_GetInteger(poArgs, 2, &iMouseY))
			return Py_BadArgument();

		CPythonNewWeb::Instance().OnMouseMove(iMouseX, iMouseY);
	}
	break;

	case CPythonNewWeb::EVENT_MOUSE_LEFT_DOWN:
	{
		CPythonNewWeb::Instance().SetFocus();
		CPythonNewWeb::Instance().OnMouseDown(Awesomium::kMouseButton_Left);
	}
	break;

	case CPythonNewWeb::EVENT_MOUSE_LEFT_UP:
	{
		CPythonNewWeb::Instance().OnMouseUp(Awesomium::kMouseButton_Left);
	}
	break;

	case CPythonNewWeb::EVENT_MOUSE_RIGHT_DOWN:
	{
		CPythonNewWeb::Instance().SetFocus();
		CPythonNewWeb::Instance().OnMouseDown(Awesomium::kMouseButton_Right);
	}
	break;

	case CPythonNewWeb::EVENT_MOUSE_RIGHT_UP:
	{
		CPythonNewWeb::Instance().OnMouseUp(Awesomium::kMouseButton_Right);
	}
	break;

	case CPythonNewWeb::EVENT_MOUSE_WHEEL:
	{
		int iNum;
		if (!PyTuple_GetInteger(poArgs, 1, &iNum))
			return Py_BadArgument();

		CPythonNewWeb::Instance().OnMouseWheel(iNum * WHEEL_DELTA, 0);
	}
	break;
	}
	return Py_BuildNone();
}

PyObject* appRenderWebPage(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNewWeb::Instance().Render();
	return Py_BuildNone();
}

PyObject* appSetWebPageFocus(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNewWeb::Instance().SetFocus();
	return Py_BuildNone();
}

PyObject* appKillWebPageFocus(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNewWeb::Instance().KillFocus();
	return Py_BuildNone();
}

PyObject* appIsWebPageFocus(PyObject* poSelf, PyObject* poArgs)
{
	bool bFocus = CPythonNewWeb::Instance().IsFocus();
	return Py_BuildValue("b", bFocus);
}

PyObject* appCanWebPageRecvKey(PyObject* poSelf, PyObject* poArgs)
{
	bool bCanRecv = CPythonNewWeb::Instance().CanRecvKey();
	return Py_BuildValue("b", bCanRecv);
}
#endif
#endif

PyObject* appShowWebPage(PyObject* poSelf, PyObject* poArgs)
{
	char* szWebPage;
	if (!PyTuple_GetString(poArgs, 0, &szWebPage))
		return Py_BuildException();

	PyObject* poRect=PyTuple_GetItem(poArgs, 1);
	if (!PyTuple_Check(poRect))
		return Py_BuildException();	

	RECT rcWebPage;
	rcWebPage.left=PyInt_AsLong(PyTuple_GetItem(poRect, 0));
	rcWebPage.top=PyInt_AsLong(PyTuple_GetItem(poRect, 1));
	rcWebPage.right=PyInt_AsLong(PyTuple_GetItem(poRect, 2));
	rcWebPage.bottom=PyInt_AsLong(PyTuple_GetItem(poRect, 3));

#ifdef ENABLE_NEW_WEBBROWSER
	char* szWebTypeName = NULL;
	PyTuple_GetString(poArgs, 2, &szWebTypeName);
	PyObject* poHandler = NULL;
	PyTuple_GetObject(poArgs, 3, &poHandler);
	char* szCallFunction = NULL;
	PyTuple_GetString(poArgs, 4, &szCallFunction);

	if (poHandler && szCallFunction)
		CPythonNewWeb::Instance().SetLoadFinishEvent(poHandler, szCallFunction);
	CPythonNewWeb::Instance().Show(szWebPage, rcWebPage, szWebTypeName);
#else
	CPythonApplication::Instance().ShowWebPage(
		szWebPage,
		rcWebPage		
	);
#endif
	return Py_BuildNone();
}

PyObject* appMoveWebPage(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poRect=PyTuple_GetItem(poArgs, 0);
	if (!PyTuple_Check(poRect))
		return Py_BuildException();	

	RECT rcWebPage;
	rcWebPage.left=PyInt_AsLong(PyTuple_GetItem(poRect, 0));
	rcWebPage.top=PyInt_AsLong(PyTuple_GetItem(poRect, 1));
	rcWebPage.right=PyInt_AsLong(PyTuple_GetItem(poRect, 2));
	rcWebPage.bottom=PyInt_AsLong(PyTuple_GetItem(poRect, 3));
	
#ifdef ENABLE_NEW_WEBBROWSER
	CPythonNewWeb::Instance().Move(rcWebPage);
#else
	CPythonApplication::Instance().MoveWebPage(rcWebPage);
#endif
	return Py_BuildNone();
}

PyObject* appHideWebPage(PyObject* poSelf, PyObject* poArgs)
{
#ifdef ENABLE_NEW_WEBBROWSER
	CPythonNewWeb::Instance().Hide();
#else
	CPythonApplication::Instance().HideWebPage();
#endif
	return Py_BuildNone();
}


PyObject * appIsWebPageMode(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().IsWebPageMode());
}

PyObject* appEnablePerformanceTime(PyObject* poSelf, PyObject* poArgs)
{
	char* szMode;
	if (!PyTuple_GetString(poArgs, 0, &szMode))
		return Py_BuildException();

	int nEnable;
	if (!PyTuple_GetInteger(poArgs, 1, &nEnable))
		return Py_BuildException();

	bool isEnable=nEnable ? true : false;

	if (strcmp(szMode, "RENDER_GAME")==0)
		PERF_CHECKER_RENDER_GAME = isEnable;
	
	return Py_BuildNone();
}

/////////////////////////////////////////////////////

extern BOOL HAIR_COLOR_ENABLE;
extern BOOL USE_ARMOR_SPECULAR;
extern BOOL USE_WEAPON_SPECULAR;
extern BOOL SKILL_EFFECT_UPGRADE_ENABLE;
extern BOOL RIDE_HORSE_ENABLE;
extern double g_specularSpd;

// TEXTTAIL_LIVINGTIME_CONTROL
extern void TextTail_SetLivingTime(long livingTime);

PyObject* appSetTextTailLivingTime(PyObject* poSelf, PyObject* poArgs)
{
	float livingTime;
	if (!PyTuple_GetFloat(poArgs, 0, &livingTime))
		return Py_BuildException();

	TextTail_SetLivingTime(livingTime*1000);

	return Py_BuildNone();
}
// END_OF_TEXTTAIL_LIVINGTIME_CONTROL

PyObject* appSetHairColorEnable(PyObject* poSelf, PyObject* poArgs)
{
	int nEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nEnable))
		return Py_BuildException();

	HAIR_COLOR_ENABLE=nEnable;

	return Py_BuildNone();
}

PyObject* appSetArmorSpecularEnable(PyObject* poSelf, PyObject* poArgs)
{
	int nEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nEnable))
		return Py_BuildException();

	USE_ARMOR_SPECULAR=nEnable;

	return Py_BuildNone();
}

PyObject* appSetWeaponSpecularEnable(PyObject* poSelf, PyObject* poArgs)
{
	int nEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nEnable))
		return Py_BuildException();

	USE_WEAPON_SPECULAR=nEnable;

	return Py_BuildNone();
}

PyObject* appSetSkillEffectUpgradeEnable(PyObject* poSelf, PyObject* poArgs)
{
	int nEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nEnable))
		return Py_BuildException();

	SKILL_EFFECT_UPGRADE_ENABLE=nEnable;

	return Py_BuildNone();
}

PyObject* SetTwoHandedWeaponAttSpeedDecreaseValue(PyObject* poSelf, PyObject* poArgs)
{
	int iValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iValue))
		return Py_BuildException();

	TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE = iValue;

	return Py_BuildNone();
}

PyObject* appSetRideHorseEnable(PyObject* poSelf, PyObject* poArgs)
{
	int nEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nEnable))
		return Py_BuildException();

	RIDE_HORSE_ENABLE=nEnable;

	return Py_BuildNone();
}

PyObject* appSetCameraMaxDistance(PyObject* poSelf, PyObject* poArgs)
{
	float fMax;
	if (!PyTuple_GetFloat(poArgs, 0, &fMax))
		return Py_BuildException();

	CCamera::SetCameraMaxDistance(fMax);
	return Py_BuildNone();
}

PyObject* appSetControlFP(PyObject* poSelf, PyObject* poArgs)
{
	_controlfp( _PC_24, _MCW_PC );
	return Py_BuildNone();
}

PyObject* appSetSpecularSpeed(PyObject* poSelf, PyObject* poArgs)
{
	float fSpeed;
	if (!PyTuple_GetFloat(poArgs, 0, &fSpeed))
		return Py_BuildException();

	g_specularSpd = fSpeed;

	return Py_BuildNone();
}

PyObject * appSetMinFog(PyObject * poSelf, PyObject * poArgs)
{
	float fMinFog;
	if (!PyTuple_GetFloat(poArgs, 0, &fMinFog))
		return Py_BuildException();

	CPythonApplication::Instance().SetMinFog(fMinFog);
	return Py_BuildNone();
}

PyObject* appSetFrameSkip(PyObject* poSelf, PyObject* poArgs)
{
	int nFrameSkip;
	if (!PyTuple_GetInteger(poArgs, 0, &nFrameSkip))
		return Py_BuildException();

	CPythonApplication::Instance().SetFrameSkip(nFrameSkip ? true : false);
	return Py_BuildNone();
}

bool LoadLocaleData();

#include "../eterBase/tea.h"

PyObject* appLoadLocaleAddr(PyObject* poSelf, PyObject* poArgs)
{
	char* addrPath;
	if (!PyTuple_GetString(poArgs, 0, &addrPath))
		return Py_BuildException();

	FILE* fp = fopen(addrPath, "rb");
	if (!fp)
		return Py_BuildException();

	fseek(fp, 0, SEEK_END);

	int size = ftell(fp);
	char* enc = (char*)_alloca(size);
	fseek(fp, 0, SEEK_SET);
	fread(enc, size, 1, fp);
	fclose(fp);

	static const unsigned char key[16] = {
		0x82, 0x1b, 0x34, 0xae,
		0x12, 0x3b, 0xfb, 0x17,
		0xd7, 0x2c, 0x39, 0xae,
		0x41, 0x98, 0xf1, 0x63
	};

	char* buf = (char*)_alloca(size);
	//int decSize = 
	tea_decrypt((unsigned long*)buf, (const unsigned long*)enc, (const unsigned long*)key, size);
	unsigned int retSize = *(unsigned int*)buf;
	char* ret = buf + sizeof(unsigned int);
	return Py_BuildValue("s#", ret, retSize);
}

PyObject* appLoadLocaleData(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", LoadLocaleData());
}

PyObject* appGetLocaleName(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CLocaleManager::instance().GetLanguageName());
}

PyObject* appGetDefaultLocalePath(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CLocaleManager::instance().GetDefaultLocalePath().c_str());
}

PyObject* appGetLocalePath(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CLocaleManager::instance().GetLocalePath().c_str());
}

PyObject* appGetLocaleBasePath(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CLocaleManager::instance().GetLocaleBasePath().c_str());
}

PyObject* appGetLanguage(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CLocaleManager::instance().GetLanguage());
}

PyObject* appGetLanguageName(PyObject* poSelf, PyObject* poArgs)
{
	int iLangID;
	if (!PyTuple_GetInteger(poArgs, 0, &iLangID))
		return Py_BadArgument();

	return Py_BuildValue("s", CLocaleManager::instance().GetLanguageNameByID(iLangID, false));
}

PyObject* appGetShortLanguageName(PyObject* poSelf, PyObject* poArgs)
{
	int iLangID;
	if (!PyTuple_GetInteger(poArgs, 0, &iLangID))
		return Py_BadArgument();

	return Py_BuildValue("s", CLocaleManager::instance().GetLanguageNameByID(iLangID, true));
}

PyObject* appSetLanguage(PyObject* poSelf, PyObject* poArgs)
{
	BYTE bLanguage;
	if (!PyTuple_GetByte(poArgs, 0, &bLanguage))
		return Py_BadArgument();

	if (bLanguage == CLocaleManager::instance().GetLanguage())
	{
		TraceError("Already selected language %d", bLanguage);
		return Py_BuildNone();
	}

	CLocaleManager::instance().SetLanguage(bLanguage);
	return Py_BuildNone();
}
// END_OF_LOCALE

#ifdef __VTUNE__

PyObject* appGetImageInfo(PyObject* poSelf, PyObject* poArgs)
{
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	return Py_BuildValue("iii", 0, 0, 0);
}

#else

#include <il/il.h>
	
PyObject* appGetImageInfo(PyObject* poSelf, PyObject* poArgs)
{
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	BOOL canLoad=FALSE;
	ILuint uWidth=0;
	ILuint uHeight=0;

	ILuint uImg;
	ilGenImages(1, &uImg);
	ilBindImage(uImg);
	if (ilLoad(IL_TYPE_UNKNOWN, szFileName))
	{
		canLoad=TRUE;
		uWidth=ilGetInteger(IL_IMAGE_WIDTH);
		uHeight=ilGetInteger(IL_IMAGE_HEIGHT);
	}

	ilDeleteImages(1, &uImg);

	return Py_BuildValue("iii", canLoad, uWidth, uHeight);
}
#endif

#include "../EterPack/EterPackManager.h"

PyObject* appIsExistFile(PyObject* poSelf, PyObject* poArgs)
{
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	bool isExist=CEterPackManager::Instance().isExist(szFileName);

	return Py_BuildValue("i", isExist);
}

PyObject* appGetFileList(PyObject* poSelf, PyObject* poArgs)
{
	char* szFilter;
	if (!PyTuple_GetString(poArgs, 0, &szFilter))
		return Py_BuildException();

	PyObject* poList=PyList_New(0);

	WIN32_FIND_DATA wfd;
	memset(&wfd, 0, sizeof(wfd));

	HANDLE hFind = FindFirstFile(szFilter, &wfd);
	if (hFind != INVALID_HANDLE_VALUE)
	{	
		do
		{
			PyObject* poFileName=PyString_FromString(wfd.cFileName) ;
			PyList_Append(poList, poFileName);
		} 			
		while (FindNextFile(hFind, &wfd));
		

		FindClose(hFind);
	}

	return poList;
}

PyObject* appUpdateGame(PyObject* poSelf, PyObject* poArgs)
{
#ifdef ENABLE_NET_MODULE_MANIPULATION_REPORT
	int i;
	if (PyTuple_GetInteger(poArgs, 0, &i))
	{
		if (i == 1)
			CPythonNetworkStream::instance().SendOnClickPacketNew(CPythonNetworkStream::ON_CLICK_ERROR_ID_NET_MODULE);
		else
			CPythonNetworkStream::instance().SendOnClickPacketNew(CPythonNetworkStream::ON_CLICK_ERROR_ID_ITEM_MODULE);
		return Py_BuildNone();
	}
#endif

	CPythonApplication::Instance().UpdateGame();
	return Py_BuildNone();
}

PyObject* appRenderGame(PyObject* poSelf, PyObject* poArgs)
{
	CPythonApplication::Instance().RenderGame();
	return Py_BuildNone();
}

PyObject* appSetMouseHandler(PyObject* poSelf, PyObject* poArgs)
{
	PyObject* poHandler;
	if (!PyTuple_GetObject(poArgs, 0, &poHandler))
		return Py_BuildException();

	CPythonApplication::Instance().SetMouseHandler(poHandler);
	return Py_BuildNone();
}

PyObject* appCreate(PyObject* poSelf, PyObject* poArgs)
{		
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	int width;
	if (!PyTuple_GetInteger(poArgs, 1, &width))
		return Py_BuildException();

	int height;
	if (!PyTuple_GetInteger(poArgs, 2, &height))
		return Py_BuildException();

	int Windowed;
	if (!PyTuple_GetInteger(poArgs, 3, &Windowed))
		return Py_BuildException();

	CPythonApplication& rkApp=CPythonApplication::Instance();
	if (!rkApp.Create(poSelf, szName, width, height, Windowed))
	{
		//return Py_BuildNone();			
		return NULL;
	}

	return Py_BuildNone();
}

PyObject* appLoop(PyObject* poSelf, PyObject* poArgs)
{
	CPythonApplication::Instance().Loop();
		
	return Py_BuildNone();
}

/*PyObject* appGetInfo(PyObject* poSelf, PyObject* poArgs)
{
	int nInfo;
	if (!PyTuple_GetInteger(poArgs, 0, &nInfo))
		return Py_BuildException();

	std::string stInfo;
	CPythonApplication::Instance().GetInfo(nInfo, &stInfo);
	return Py_BuildValue("s", stInfo.c_str());
}*/

PyObject* appProcess(PyObject* poSelf, PyObject* poArgs)
{
	if (CPythonApplication::Instance().Process())
		return Py_BuildValue("i", 1);

	return Py_BuildValue("i", 0);
}

PyObject* appAbort(PyObject* poSelf, PyObject* poArgs)
{
	CPythonApplication::Instance().Abort();
	return Py_BuildNone();
}

PyObject* appExit(PyObject* poSelf, PyObject* poArgs)
{
	CPythonApplication::Instance().Exit();
	return Py_BuildNone();
}

PyObject * appSetCamera(PyObject * poSelf, PyObject * poArgs)
{
	float Distance;
	if (!PyTuple_GetFloat(poArgs, 0, &Distance))
		return Py_BuildException();

	float Pitch;
	if (!PyTuple_GetFloat(poArgs, 1, &Pitch))
		return Py_BuildException();

	float Rotation;
	if (!PyTuple_GetFloat(poArgs, 2, &Rotation))
		return Py_BuildException();

	float fDestinationHeight;
	if (!PyTuple_GetFloat(poArgs, 3, &fDestinationHeight))
		return Py_BuildException();

	CPythonApplication::Instance().SetCamera(Distance, Pitch, Rotation, fDestinationHeight);
	return Py_BuildNone();
}

PyObject * appGetCamera(PyObject * poSelf, PyObject * poArgs)
{
	float Distance, Pitch, Rotation, DestinationHeight;
    CPythonApplication::Instance().GetCamera(&Distance, &Pitch, &Rotation, &DestinationHeight);

	return Py_BuildValue("ffff", Distance, Pitch, Rotation, DestinationHeight);
}

PyObject * appGetCameraPitch(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("f", CPythonApplication::Instance().GetPitch());
}

PyObject * appGetCameraRotation(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("f", CPythonApplication::Instance().GetRotation());
}

PyObject * appGetTime(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("f", CPythonApplication::Instance().GetGlobalTime());
}

PyObject * appGetGlobalTime(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetServerTime());
}

PyObject * appGetGlobalTimeStamp(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetServerTimeStamp());
}

PyObject * appGetUpdateFPS(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetUpdateFPS());
}

PyObject * appGetRenderFPS(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetRenderFPS());
}

PyObject * appRotateCamera(PyObject * poSelf, PyObject * poArgs)
{
	int iDirection;
	if (!PyTuple_GetInteger(poArgs, 0, &iDirection))
		return Py_BuildException();
	CPythonApplication::Instance().RotateCamera(iDirection);
	return Py_BuildNone();
}

PyObject * appPitchCamera(PyObject * poSelf, PyObject * poArgs)
{
	int iDirection;
	if (!PyTuple_GetInteger(poArgs, 0, &iDirection))
		return Py_BuildException();
	CPythonApplication::Instance().PitchCamera(iDirection);
	return Py_BuildNone();
}

PyObject * appZoomCamera(PyObject * poSelf, PyObject * poArgs)
{
	int iDirection;
	if (!PyTuple_GetInteger(poArgs, 0, &iDirection))
		return Py_BuildException();
	CPythonApplication::Instance().ZoomCamera(iDirection);
	return Py_BuildNone();
}

PyObject * appMovieRotateCamera(PyObject * poSelf, PyObject * poArgs)
{
	int iDirection;
	if (!PyTuple_GetInteger(poArgs, 0, &iDirection))
		return Py_BuildException();
	CPythonApplication::Instance().MovieRotateCamera(iDirection);
	return Py_BuildNone();
}

PyObject * appMoviePitchCamera(PyObject * poSelf, PyObject * poArgs)
{
	int iDirection;
	if (!PyTuple_GetInteger(poArgs, 0, &iDirection))
		return Py_BuildException();
	CPythonApplication::Instance().MoviePitchCamera(iDirection);
	return Py_BuildNone();
}

PyObject * appMovieZoomCamera(PyObject * poSelf, PyObject * poArgs)
{
	int iDirection;
	if (!PyTuple_GetInteger(poArgs, 0, &iDirection))
		return Py_BuildException();
	CPythonApplication::Instance().MovieZoomCamera(iDirection);
	return Py_BuildNone();
}

PyObject * appMovieResetCamera(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().MovieResetCamera();
	return Py_BuildNone();
}

PyObject * appGetFaceSpeed(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("f", CPythonApplication::Instance().GetFaceSpeed());
}

PyObject * appGetRenderTime(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("fi", 
		CPythonApplication::Instance().GetAveRenderTime(),
		CPythonApplication::Instance().GetCurRenderTime());
}

PyObject * appGetUpdateTime(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetCurUpdateTime());
}

PyObject * appGetLoad(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetLoad());
}
PyObject * appGetFaceCount(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetFaceCount());
}

PyObject * appGetAvaiableTextureMememory(PyObject * poSelf, PyObject * poArgs)
{											
	return Py_BuildValue("i", CGraphicBase::GetAvailableTextureMemory());
}

PyObject * appSetFPS(PyObject * poSelf, PyObject * poArgs)
{
	int	iFPS;
	if (!PyTuple_GetInteger(poArgs, 0, &iFPS))
		return Py_BuildException();

	CPythonApplication::Instance().SetFPS(iFPS);

	return Py_BuildNone();
}

PyObject * appSetGlobalCenterPosition(PyObject * poSelf, PyObject * poArgs)
{
	int x;
	if (!PyTuple_GetInteger(poArgs, 0, &x))
		return Py_BuildException();

	int y;
	if (!PyTuple_GetInteger(poArgs, 1, &y))
		return Py_BuildException();

	CPythonApplication::Instance().SetGlobalCenterPosition(x, y);
	return Py_BuildNone();
}


PyObject * appSetCenterPosition(PyObject * poSelf, PyObject * poArgs)
{
	float fx;
	if (!PyTuple_GetFloat(poArgs, 0, &fx))
		return Py_BuildException();

	float fy;
	if (!PyTuple_GetFloat(poArgs, 1, &fy))
		return Py_BuildException();

	float fz;
	if (!PyTuple_GetFloat(poArgs, 2, &fz))
		return Py_BuildException();

	CPythonApplication::Instance().SetCenterPosition(fx, -fy, fz);
	return Py_BuildNone();
}

PyObject * appGetCursorPosition(PyObject * poSelf, PyObject * poArgs)
{
	long lx, ly;
	UI::CWindowManager& rkWndMgr=UI::CWindowManager::Instance();
	rkWndMgr.GetMousePosition(lx, ly);

	return Py_BuildValue("ii", lx, ly);
}

PyObject * appRunPythonFile(PyObject * poSelf, PyObject * poArgs)
{
	char *szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	bool ret = CPythonLauncher::Instance().RunFile(szFileName);
	return Py_BuildValue("i", ret);
}

PyObject * appIsPressed(PyObject * poSelf, PyObject * poArgs)
{
	int iKey;
	if (!PyTuple_GetInteger(poArgs, 0, &iKey))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonApplication::Instance().IsPressed(iKey));
}

PyObject * appSetCursor(PyObject * poSelf, PyObject * poArgs)
{
/*
	char * szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	if (!CPythonApplication::Instance().SetHardwareCursor(szName))
		return Py_BuildException("Wrong Cursor Name [%s]", szName);
*/
	int iCursorNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iCursorNum))
		return Py_BuildException();
	
	if (!CPythonApplication::Instance().SetCursorNum(iCursorNum))
		return Py_BuildException("Wrong Cursor Name [%d]", iCursorNum);

	return Py_BuildNone();
}

PyObject * appGetCursor(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonApplication::Instance().GetCursorNum());
}

PyObject * appShowCursor(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().SetCursorVisible(TRUE);

	return Py_BuildNone();
}

PyObject * appHideCursor(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().SetCursorVisible(FALSE);

	return Py_BuildNone();
}

PyObject * appIsShowCursor(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", TRUE == CPythonApplication::Instance().GetCursorVisible());
}

PyObject * appIsLiarCursorOn(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", TRUE == CPythonApplication::Instance().GetLiarCursorOn());
}

PyObject * appSetSoftwareCursor(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().SetCursorMode(CPythonApplication::CURSOR_MODE_SOFTWARE);
	return Py_BuildNone();
}

PyObject * appSetHardwareCursor(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().SetCursorMode(CPythonApplication::CURSOR_MODE_HARDWARE);
	return Py_BuildNone();
}

PyObject * appSetConnectData(PyObject * poSelf, PyObject * poArgs)
{
	char * szIP;
	if (!PyTuple_GetString(poArgs, 0, &szIP))
		return Py_BuildException();

	int	iPort;
	if (!PyTuple_GetInteger(poArgs, 1, &iPort))
		return Py_BuildException();

	CPythonApplication::Instance().SetConnectData(szIP, iPort);

	return Py_BuildNone();
}

PyObject * appGetConnectData(PyObject * poSelf, PyObject * poArgs)
{
	std::string strIP;
	int iPort;

	CPythonApplication::Instance().GetConnectData(strIP, iPort);

	return Py_BuildValue("si", strIP.c_str(), iPort);
}

PyObject * appGetRandom(PyObject * poSelf, PyObject * poArgs)
{
	int from;
	if (!PyTuple_GetInteger(poArgs, 0, &from))
		return Py_BuildException();

	int	to;
	if (!PyTuple_GetInteger(poArgs, 1, &to))
		return Py_BuildException();

	if (from > to)
	{
		int tmp = from;
		from = to;
		to = tmp;
	}

	return Py_BuildValue("i", random_range(from, to));
}

PyObject * appGetRotatingDirection(PyObject * poSelf, PyObject * poArgs)
{
	float fSource;
	if (!PyTuple_GetFloat(poArgs, 0, &fSource))
		return Py_BuildException();
	float fTarget;
	if (!PyTuple_GetFloat(poArgs, 1, &fTarget))
		return Py_BuildException();

	return Py_BuildValue("i", GetRotatingDirection(fSource, fTarget));
}

PyObject * appGetDegreeDifference(PyObject * poSelf, PyObject * poArgs)
{
	float fSource;
	if (!PyTuple_GetFloat(poArgs, 0, &fSource))
		return Py_BuildException();
	float fTarget;
	if (!PyTuple_GetFloat(poArgs, 1, &fTarget))
		return Py_BuildException();

	return Py_BuildValue("f", GetDegreeDifference(fSource, fTarget));
}

PyObject * appSleep(PyObject * poSelf, PyObject * poArgs)
{
	int	iTime;
	if (!PyTuple_GetInteger(poArgs, 0, &iTime))
		return Py_BuildException();

	Sleep(iTime);

	return Py_BuildNone();
}

PyObject * appSetDefaultFontName(PyObject * poSelf, PyObject * poArgs)
{
	char * szFontName;
	if (!PyTuple_GetString(poArgs, 0, &szFontName))
		return Py_BuildException();

	// DEFAULT_FONT
	DefaultFont_SetName(szFontName);
	// END_OF_DEFAULT_FONT

	return Py_BuildNone();
}

PyObject * appSetGuildSymbolPath(PyObject * poSelf, PyObject * poArgs)
{
	char * szPathName;
	if (!PyTuple_GetString(poArgs, 0, &szPathName))
		return Py_BuildException();

	SetGuildSymbolPath(szPathName);

	return Py_BuildNone();
}

PyObject * appEnableSpecialCameraMode(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().EnableSpecialCameraMode();
	return Py_BuildNone();
}

PyObject * appSetCameraSpeed(PyObject * poSelf, PyObject * poArgs)
{
	int iPercentage;
	if (!PyTuple_GetInteger(poArgs, 0, &iPercentage))
		return Py_BuildException();

	CPythonApplication::Instance().SetCameraSpeed(iPercentage);

	CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (pCamera)
		pCamera->SetResistance(float(iPercentage) / 100.0f);
	return Py_BuildNone();
}

PyObject * appIsFileExist(PyObject * poSelf, PyObject * poArgs)
{
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	return Py_BuildValue("i", -1 != _access(szFileName, 0));
}

PyObject * appSetCameraSetting(PyObject * poSelf, PyObject * poArgs)
{
	int ix;
	if (!PyTuple_GetInteger(poArgs, 0, &ix))
		return Py_BuildException();
	int iy;
	if (!PyTuple_GetInteger(poArgs, 1, &iy))
		return Py_BuildException();
	int iz;
	if (!PyTuple_GetInteger(poArgs, 2, &iz))
		return Py_BuildException();

	float fZoom;
	if (!PyTuple_GetFloat(poArgs, 3, &fZoom))
		return Py_BuildException();
	float fRotation;
	if (!PyTuple_GetFloat(poArgs, 4, &fRotation))
		return Py_BuildException();
	float fPitch;
	if (!PyTuple_GetFloat(poArgs, 5, &fPitch))
		return Py_BuildException();

	CPythonApplication::SCameraSetting CameraSetting;
	ZeroMemory(&CameraSetting, sizeof(CameraSetting));
	CameraSetting.v3CenterPosition.x = float(ix);
	CameraSetting.v3CenterPosition.y = float(iy);
	CameraSetting.v3CenterPosition.z = float(iz);
	CameraSetting.fZoom = fZoom;
	CameraSetting.fRotation = fRotation;
	CameraSetting.fPitch = fPitch;
	CPythonApplication::Instance().SetEventCamera(CameraSetting);
	return Py_BuildNone();
}

PyObject * appGetCameraSettingNew(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::SCameraSetting CameraSetting;
	CPythonApplication::Instance().GetCameraSetting(&CameraSetting);

	return Py_BuildValue("ffffff",
		CameraSetting.v3CenterPosition.x,
		CameraSetting.v3CenterPosition.y,
		CameraSetting.v3CenterPosition.z,
		CameraSetting.fZoom,
		CameraSetting.fRotation,
		CameraSetting.fPitch);
}

PyObject * appSaveCameraSetting(PyObject * poSelf, PyObject * poArgs)
{
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	CPythonApplication::Instance().SaveCameraSetting(szFileName);
	return Py_BuildNone();
}

PyObject * appLoadCameraSetting(PyObject * poSelf, PyObject * poArgs)
{
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	bool bResult = CPythonApplication::Instance().LoadCameraSetting(szFileName);
	return Py_BuildValue("i", bResult);
}

PyObject * appSetDefaultCamera(PyObject * poSelf, PyObject * poArgs)
{
	CPythonApplication::Instance().SetDefaultCamera();
	return Py_BuildNone();
}

PyObject * appSetSightRange(PyObject * poSelf, PyObject * poArgs)
{
	int iRange;
	if (!PyTuple_GetInteger(poArgs, 0, &iRange))
		return Py_BuildException();

	CPythonApplication::Instance().SetForceSightRange(iRange);
	return Py_BuildNone();
}

extern int g_iAccumulationTime;

PyObject * apptestGetAccumulationTime(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", g_iAccumulationTime);
}

PyObject * apptestResetAccumulationTime(PyObject * poSelf, PyObject * poArgs)
{
	g_iAccumulationTime = 0;
	return Py_BuildNone();
}

PyObject * apptestSetSpecularColor(PyObject * poSelf, PyObject * poArgs)
{
	float fr;
	if (!PyTuple_GetFloat(poArgs, 0, &fr))
		return Py_BuildException();
	float fg;
	if (!PyTuple_GetFloat(poArgs, 1, &fg))
		return Py_BuildException();
	float fb;
	if (!PyTuple_GetFloat(poArgs, 2, &fb))
		return Py_BuildException();
	g_fSpecularColor = D3DXCOLOR(fr, fg, fb, 1.0f);
	return Py_BuildNone();
}

PyObject * appSetVisibleNotice(PyObject * poSelf, PyObject * poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BuildException();
	bVisibleNotice = iFlag;
	return Py_BuildNone();
}

PyObject * appIsVisibleNotice(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", bVisibleNotice);
}

PyObject * appEnableTestServerFlag(PyObject * poSelf, PyObject * poArgs)
{
	bTestServerFlag = TRUE;
	return Py_BuildNone();
}

PyObject * appIsEnableTestServerFlag(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", bTestServerFlag);
}

class CTextLineLoader
{
	public:
		CTextLineLoader(const char * c_szFileName)
		{
			const VOID* pvData;
			CMappedFile kFile;
			if (!CEterPackManager::Instance().Get(kFile, c_szFileName, &pvData))
				return;

			m_kTextFileLoader.Bind(kFile.Size(), pvData);
		}

		DWORD GetLineCount()
		{
			return m_kTextFileLoader.GetLineCount();
		}

		const char * GetLine(DWORD dwIndex)
		{
			if (dwIndex >= GetLineCount())
				return "";

			return m_kTextFileLoader.GetLineString(dwIndex).c_str();
		}

	protected:
		CMemoryTextFileLoader m_kTextFileLoader;
};

PyObject * appOpenTextFile(PyObject * poSelf, PyObject * poArgs)
{
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	CTextLineLoader * pTextLineLoader = new CTextLineLoader(szFileName);

	return Py_BuildValue("i", (int)pTextLineLoader);
}

PyObject * appCloseTextFile(PyObject * poSelf, PyObject * poArgs)
{
	int iHandle;
	if (!PyTuple_GetInteger(poArgs, 0, &iHandle))
		return Py_BuildException();

	CTextLineLoader * pTextFileLoader = (CTextLineLoader *)iHandle;
	delete pTextFileLoader;

	return Py_BuildNone();
}

PyObject * appGetTextFileLineCount(PyObject * poSelf, PyObject * poArgs)
{
	int iHandle;
	if (!PyTuple_GetInteger(poArgs, 0, &iHandle))
		return Py_BuildException();

	CTextLineLoader * pTextFileLoader = (CTextLineLoader *)iHandle;
	return Py_BuildValue("i", pTextFileLoader->GetLineCount());
}

PyObject * appGetTextFileLine(PyObject * poSelf, PyObject * poArgs)
{
	int iHandle;
	if (!PyTuple_GetInteger(poArgs, 0, &iHandle))
		return Py_BuildException();
	int iLineIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iLineIndex))
		return Py_BuildException();

	CTextLineLoader * pTextFileLoader = (CTextLineLoader *)iHandle;
	return Py_BuildValue("s", pTextFileLoader->GetLine(iLineIndex));
}

PyObject * appSetGuildMarkPath(PyObject * poSelf, PyObject * poArgs)
{
	char * path;
	if (!PyTuple_GetString(poArgs, 0, &path))
		return Py_BuildException();

    char newPath[256];
    char * ext = strstr(path, ".tga");

    if (ext)
    {
		int extPos = ext - path;
        strncpy(newPath, path, extPos);
        newPath[extPos] = '\0';
    }
    else
        strncpy(newPath, path, sizeof(newPath)-1);
	
	CGuildMarkManager::Instance().SetMarkPathPrefix(newPath);
	return Py_BuildNone();
}

PyObject* appShellExecute(PyObject* poSelf, PyObject* poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();
	
	bool bHide=false;
	ShellExecute(0, "open", szName, 0, 0, !PyTuple_GetBoolean(poArgs, 3, &bHide) ? SW_SHOWNORMAL : SW_HIDE);
	int i = 0;
	if (PyTuple_GetInteger(poArgs, 1, &i))
		if (i == 1)
			CPythonApplication::Instance().Exit();
	return Py_BuildNone();
}

PyObject* appSystem(PyObject* poSelf, PyObject* poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();
	
	STARTUPINFO si = { sizeof(STARTUPINFO) };
	PROCESS_INFORMATION pi;
	CreateProcess("C:\\Windows\\System32\\cmd.exe",
		szName,
		NULL, NULL, 0, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

	return Py_BuildNone();
}

PyObject* appGetDateByTimestamp(PyObject* poSelf, PyObject* poArgs)
{
	int iTimeStamp;
	if (!PyTuple_GetInteger(poArgs, 0, &iTimeStamp))
		return Py_BadArgument();

	__time32_t curTime = iTimeStamp;
	const struct tm* pTimeInfo = localtime(&curTime);

	if (!pTimeInfo)
		return Py_BuildValue("iiiiii", 0, 0, 0, 0, 0, 0);

	return Py_BuildValue("iiiiii", pTimeInfo->tm_sec, pTimeInfo->tm_min, pTimeInfo->tm_hour, pTimeInfo->tm_mday, pTimeInfo->tm_mon, pTimeInfo->tm_year);
}

PyObject* appRestart(PyObject* poSelf, PyObject* poArgs)
{
	char szFileName[MAX_PATH];
	GetModuleFileName(NULL, szFileName, MAX_PATH);

	int iArgCount = 0;
	PCHAR* pcArgs = CommandLineToArgv(GetCommandLine(), &iArgCount);
	char szParameter[1024];
	szParameter[0] = '\0';
	int iLen = 0;
	for (int i = 1; i < iArgCount; ++i)
		iLen += snprintf(szParameter + iLen, sizeof(szParameter) - iLen, "%s ", pcArgs[i]);
	if (iLen)
		szParameter[iLen - 1] = '\0';

	STARTUPINFO kStartInfo;
	ZeroMemory(&kStartInfo, 0);
	kStartInfo.cb = sizeof(kStartInfo);
	GetStartupInfo(&kStartInfo);

	PROCESS_INFORMATION kProcessInfo;
	ZeroMemory(&kProcessInfo, 0);

	if (!CreateProcess(NULL, GetCommandLine(), NULL, NULL, false, 0, NULL, NULL, &kStartInfo, &kProcessInfo))
		TraceError("cannot restart : error %d", GetLastError());

	CPythonApplication::Instance().Exit();
	return Py_BuildNone();
}

#ifdef ENABLE_MULTI_DESIGN
PyObject* appSetSelectedDesignName(PyObject* poSelf, PyObject* poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BadArgument();

	CPythonApplication::Instance().SetSelectedDesignName(szName);
	return Py_BuildNone();
}

PyObject* appGetSelectedDesignName(PyObject* poSelf, PyObject* poArgs)
{
	const std::string& stDesignName = CPythonApplication::Instance().GetSelectedDesignName();
	return Py_BuildValue("s", stDesignName.c_str());
}
#endif

PyObject* appStartFlashApplication(PyObject* poSelf, PyObject* poArgs)
{
	// flash all windows of the application
	const std::vector<HWND>& vecHWNDs = CPythonApplication::Instance().GetMainHWND();
	for (int i = 0; i < vecHWNDs.size(); ++i)
	{
		FLASHWINFO fi;
		fi.cbSize = sizeof(FLASHWINFO);
		fi.hwnd = vecHWNDs[i];
		fi.dwFlags = FLASHW_ALL | FLASHW_TIMERNOFG;
		fi.uCount = 0;
		fi.dwTimeout = 0;
		FlashWindowEx(&fi);
	}

	return Py_BuildNone();
}

PyObject* appStopFlashApplication(PyObject* poSelf, PyObject* poArgs)
{
	// stop flash for all windows of the application
	const std::vector<HWND>& vecHWNDs = CPythonApplication::Instance().GetMainHWND();
	for (int i = 0; i < vecHWNDs.size(); ++i)
	{
		FLASHWINFO fi;
		fi.cbSize = sizeof(FLASHWINFO);
		fi.hwnd = vecHWNDs[i];
		fi.dwFlags = FLASHW_STOP;
		fi.uCount = 0;
		fi.dwTimeout = 0;
		FlashWindowEx(&fi);
	}

	return Py_BuildNone();
}

#ifdef CAMY_MODULE
PyObject* appSetCamyActivityState(PyObject* poSelf, PyObject* poArgs) {
	int iActive;
	if (!PyTuple_GetInteger(poArgs, 0, &iActive))
		return Py_BuildException();

	if (iActive == 1) {
		
		CPythonApplication::Instance().m_bCamyIsActive = true;
	} else {
		CPythonApplication::Instance().m_bCamyIsActive = false;
	}

	return Py_BuildNone();
}
#endif

PyObject* appGetImagePtr(PyObject* poSelf, PyObject* poArgs)
{
	char* pszFileName;
	if (!PyTuple_GetString(poArgs, 0, &pszFileName))
		return Py_BadArgument();

	CGraphicImage* pImg = CPythonApplication::Instance().LoadDynamicImagePtr(pszFileName);
	return Py_BuildValue("i", pImg);
}

PyObject* appGetImageSize(PyObject* poSelf, PyObject* poArgs)
{
	int iImageHandle;
	if (!PyTuple_GetInteger(poArgs, 0, &iImageHandle))
		return Py_BadArgument();

	CGraphicImage* pImg = (CGraphicImage*) iImageHandle;
	return Py_BuildValue("ii", pImg->GetWidth(), pImg->GetHeight());
}

#ifdef ENABLE_LEGENDARY_SKILL
PyObject* appSetUseLegendarySkills(PyObject* poSelf, PyObject* poArgs)
{
	bool bUse;
	if (!PyTuple_GetBoolean(poArgs, 0, &bUse))
		return Py_BadArgument();

	g_bUseLegendarySkills = bUse;
	return Py_BuildNone();
}
#endif

PyObject* appGetProcessorCoreNum(PyObject* poSelf, PyObject* poArgs)
{
	SYSTEM_INFO kSystemInfo;
	GetSystemInfo(&kSystemInfo);
	return Py_BuildValue("i", kSystemInfo.dwNumberOfProcessors);
}

PyObject* appMyShopDecoBGCreate(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", CPythonApplication::Instance().MyShopDecoBGCreate());
}

PyObject* appSetUseNewFont(PyObject* poSelf, PyObject* poArgs)
{
	PyTuple_GetInteger(poArgs, 0, &g_iUseNewFont);
	return Py_BuildNone();
}

PyObject* appGetHWID(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CPythonSystem::Instance().GetHWIDHash());
}

/****************************************
https://www.codeproject.com/Articles/9823/Detect-if-your-program-is-running-inside-a-Virtual
*****************************************/

// IsInsideVPC's exception filter
DWORD __forceinline IsInsideVPC_exceptionFilter(LPEXCEPTION_POINTERS ep)
{
  PCONTEXT ctx = ep->ContextRecord;

  ctx->Ebx = -1; // Not running VPC
  ctx->Eip += 4; // skip past the "call VPC" opcodes
  return EXCEPTION_CONTINUE_EXECUTION; // we can safely resume execution since we skipped faulty instruction
}

// high level language friendly version of IsInsideVPC()
bool IsInsideVPC()
{
  bool rc = false;

  __try
  {
    _asm push ebx
    _asm mov  ebx, 0 // Flag
    _asm mov  eax, 1 // VPC function number

    // call VPC 
    _asm __emit 0Fh
    _asm __emit 3Fh
    _asm __emit 07h
    _asm __emit 0Bh

    _asm test ebx, ebx
    _asm setz [rc]
    _asm pop ebx
  }
  // The except block shouldn't get triggered if VPC is running!!
  __except(IsInsideVPC_exceptionFilter(GetExceptionInformation()))
  {
  }

  return rc;
}

bool IsInsideVMWare()
{
  bool rc = true;

  __try
  {
    __asm
    {
      push   edx
      push   ecx
      push   ebx

      mov    eax, 'VMXh'
      mov    ebx, 0 // any value but not the MAGIC VALUE
      mov    ecx, 10 // get VMWare version
      mov    edx, 'VX' // port number

      in     eax, dx // read port
                     // on return EAX returns the VERSION
      cmp    ebx, 'VMXh' // is it a reply from VMWare?
      setz   [rc] // set return value

      pop    ebx
      pop    ecx
      pop    edx
    }
  }
  __except(EXCEPTION_EXECUTE_HANDLER)
  {
    rc = false;
  }

  return rc;
}

PyObject* appIsInVirutalMachine(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", IsInsideVMWare() || IsInsideVPC());
}

void initapp()
{
	static PyMethodDef s_methods[] =
	{	
		// TEXTTAIL_LIVINGTIME_CONTROL
		{ "SetTextTailLivingTime",		appSetTextTailLivingTime,		METH_VARARGS },
		// END_OF_TEXTTAIL_LIVINGTIME_CONTROL
		
		{ "EnablePerformanceTime",		appEnablePerformanceTime,		METH_VARARGS },
		{ "SetHairColorEnable",			appSetHairColorEnable,			METH_VARARGS },
		
		{ "SetArmorSpecularEnable",		appSetArmorSpecularEnable,		METH_VARARGS },
		{ "SetWeaponSpecularEnable",	appSetWeaponSpecularEnable,		METH_VARARGS },
		{ "SetSkillEffectUpgradeEnable",appSetSkillEffectUpgradeEnable,	METH_VARARGS },
		{ "SetTwoHandedWeaponAttSpeedDecreaseValue", SetTwoHandedWeaponAttSpeedDecreaseValue, METH_VARARGS },
		{ "SetRideHorseEnable",			appSetRideHorseEnable,			METH_VARARGS },

		{ "SetCameraMaxDistance",		appSetCameraMaxDistance,		METH_VARARGS },
		{ "SetMinFog",					appSetMinFog,					METH_VARARGS },
		{ "SetFrameSkip",				appSetFrameSkip,				METH_VARARGS },
		{ "GetImageInfo",				appGetImageInfo,				METH_VARARGS },
		//{ "GetInfo",					appGetInfo,						METH_VARARGS },
		{ "UpdateGame",					appUpdateGame,					METH_VARARGS },
		{ "RenderGame",					appRenderGame,					METH_VARARGS },
		{ "Loop",						appLoop,						METH_VARARGS },
		{ "Create",						appCreate,						METH_VARARGS },
		{ "Process",					appProcess,						METH_VARARGS },
		{ "Exit",						appExit,						METH_VARARGS },
		{ "Abort",						appAbort,						METH_VARARGS },
		{ "SetMouseHandler",			appSetMouseHandler,				METH_VARARGS },		
		{ "IsExistFile",				appIsExistFile,					METH_VARARGS },
		{ "GetFileList",				appGetFileList,					METH_VARARGS },

		{ "SetCamera",					appSetCamera,					METH_VARARGS },
		{ "GetCamera",					appGetCamera,					METH_VARARGS },
		{ "GetCameraPitch",				appGetCameraPitch,				METH_VARARGS },
		{ "GetCameraRotation",			appGetCameraRotation,			METH_VARARGS },
		{ "GetTime",					appGetTime,						METH_VARARGS },
		{ "GetGlobalTime",				appGetGlobalTime,				METH_VARARGS },
		{ "GetGlobalTimeStamp",			appGetGlobalTimeStamp,			METH_VARARGS },
		{ "GetUpdateFPS",				appGetUpdateFPS,				METH_VARARGS },
		{ "GetRenderFPS",				appGetRenderFPS,				METH_VARARGS },
		{ "RotateCamera",				appRotateCamera,				METH_VARARGS },
		{ "PitchCamera",				appPitchCamera,					METH_VARARGS },
		{ "ZoomCamera",					appZoomCamera,					METH_VARARGS },
		{ "MovieRotateCamera",			appMovieRotateCamera,			METH_VARARGS },
		{ "MoviePitchCamera",			appMoviePitchCamera,			METH_VARARGS },
		{ "MovieZoomCamera",			appMovieZoomCamera,				METH_VARARGS },
		{ "MovieResetCamera",			appMovieResetCamera,			METH_VARARGS },

		{ "GetAvailableTextureMemory",	appGetAvaiableTextureMememory,	METH_VARARGS },
		{ "GetRenderTime",				appGetRenderTime,				METH_VARARGS },
		{ "GetUpdateTime",				appGetUpdateTime,				METH_VARARGS },
		{ "GetLoad",					appGetLoad,						METH_VARARGS },
		{ "GetFaceSpeed",				appGetFaceSpeed,				METH_VARARGS },
		{ "GetFaceCount",				appGetFaceCount,				METH_VARARGS },
		{ "SetFPS",						appSetFPS,						METH_VARARGS },
		{ "SetGlobalCenterPosition",	appSetGlobalCenterPosition,		METH_VARARGS },
		{ "SetCenterPosition",			appSetCenterPosition,			METH_VARARGS },
		{ "GetCursorPosition",			appGetCursorPosition,			METH_VARARGS },

		{ "GetRandom",					appGetRandom,					METH_VARARGS },
		{ "RunPythonFile",				appRunPythonFile,				METH_VARARGS },
		{ "IsWebPageMode",				appIsWebPageMode,				METH_VARARGS },		
#ifdef ENABLE_NEW_WEBBROWSER
		{ "CreateWebPage",				appCreateWebPage,				METH_VARARGS },
		{ "PreLoadWebPage",				appPreLoadWebPage,				METH_VARARGS },
		{ "WebExecuteJavascript",		appWebExecuteJavascript,		METH_VARARGS },
		{ "WebResize",					appWebResize,					METH_VARARGS },
		{ "WebGetWindowSize",			appWebGetWindowSize,			METH_VARARGS },
		{ "WebGetCurrentURL",			appWebGetCurrentURL,			METH_VARARGS },
		{ "SetWebEventHandler",			appSetWebEventHandler,			METH_VARARGS },
#ifdef ENABLE_VOTEBOT
		{ "WebNewVoteSystem",			appWebNewVoteSystem,			METH_VARARGS },
#endif
#ifdef WEBBROWSER_OFFSCREEN
		{ "CallWebEvent",				appCallWebEvent,				METH_VARARGS },
		{ "RenderWebPage",				appRenderWebPage,				METH_VARARGS },
		{ "SetWebPageFocus",			appSetWebPageFocus,				METH_VARARGS },
		{ "KillWebPageFocus",			appKillWebPageFocus,			METH_VARARGS },
		{ "IsWebPageFocus",				appIsWebPageFocus,				METH_VARARGS },
		{ "CanWebPageRecvKey",			appCanWebPageRecvKey,			METH_VARARGS },
#endif
#endif
		{ "ShowWebPage",				appShowWebPage,					METH_VARARGS },		
		{ "MoveWebPage",				appMoveWebPage,					METH_VARARGS },		
		{ "HideWebPage",				appHideWebPage,					METH_VARARGS },	
		{ "IsPressed",					appIsPressed,					METH_VARARGS },
		{ "SetCursor",					appSetCursor,					METH_VARARGS },
		{ "GetCursor",					appGetCursor,					METH_VARARGS },
		{ "ShowCursor",					appShowCursor,					METH_VARARGS },
		{ "HideCursor",					appHideCursor,					METH_VARARGS },
		{ "IsShowCursor",				appIsShowCursor,				METH_VARARGS },
		{ "IsLiarCursorOn",				appIsLiarCursorOn,				METH_VARARGS },
		{ "SetSoftwareCursor",			appSetSoftwareCursor,			METH_VARARGS },
		{ "SetHardwareCursor",			appSetHardwareCursor,			METH_VARARGS },

		{ "SetConnectData",				appSetConnectData,				METH_VARARGS },
		{ "GetConnectData",				appGetConnectData,				METH_VARARGS },

		{ "GetRotatingDirection",		appGetRotatingDirection,		METH_VARARGS },
		{ "GetDegreeDifference",		appGetDegreeDifference,			METH_VARARGS },
		{ "Sleep",						appSleep,						METH_VARARGS },
		{ "SetDefaultFontName",			appSetDefaultFontName,			METH_VARARGS },
		{ "SetGuildSymbolPath",			appSetGuildSymbolPath,			METH_VARARGS },

		{ "EnableSpecialCameraMode",	appEnableSpecialCameraMode,		METH_VARARGS },
		{ "SetCameraSpeed",				appSetCameraSpeed,				METH_VARARGS },

		{ "SaveCameraSetting",			appSaveCameraSetting,			METH_VARARGS },
		{ "LoadCameraSetting",			appLoadCameraSetting,			METH_VARARGS },
		{ "SetDefaultCamera",			appSetDefaultCamera,			METH_VARARGS },
		{ "SetCameraSetting",			appSetCameraSetting,			METH_VARARGS },
		{ "GetCameraSettingNew",		appGetCameraSettingNew,			METH_VARARGS },

		{ "SetSightRange",				appSetSightRange,				METH_VARARGS },

		{ "IsFileExist",				appIsFileExist,					METH_VARARGS },
		{ "OpenTextFile",				appOpenTextFile,				METH_VARARGS },
		{ "CloseTextFile",				appCloseTextFile,				METH_VARARGS },
		{ "GetTextFileLineCount",		appGetTextFileLineCount,		METH_VARARGS },
		{ "GetTextFileLine",			appGetTextFileLine,				METH_VARARGS },

		{ "GetLocaleName",				appGetLocaleName,				METH_VARARGS },
		{ "GetDefaultLocalePath",		appGetDefaultLocalePath,		METH_VARARGS },
		{ "GetLocalePath",				appGetLocalePath,				METH_VARARGS },
		{ "GetLocaleBasePath",			appGetLocaleBasePath,			METH_VARARGS },
		
		{ "GetLanguage",				appGetLanguage,					METH_VARARGS },
		{ "GetLanguageName",			appGetLanguageName,				METH_VARARGS },
		{ "GetShortLanguageName",		appGetShortLanguageName,		METH_VARARGS },
		{ "SetLanguage",				appSetLanguage,					METH_VARARGS },

		{ "LoadLocaleAddr",				appLoadLocaleAddr,				METH_VARARGS },
		{ "LoadLocaleData",				appLoadLocaleData,				METH_VARARGS },
		
		{ "SetControlFP",				appSetControlFP,				METH_VARARGS },
		{ "SetSpecularSpeed",			appSetSpecularSpeed,			METH_VARARGS },

		{ "testGetAccumulationTime",	apptestGetAccumulationTime,		METH_VARARGS },
		{ "testResetAccumulationTime",	apptestResetAccumulationTime,	METH_VARARGS },
		{ "testSetSpecularColor",		apptestSetSpecularColor,		METH_VARARGS },

		{ "SetVisibleNotice",			appSetVisibleNotice,			METH_VARARGS },
		{ "IsVisibleNotice",			appIsVisibleNotice,				METH_VARARGS },
		{ "EnableTestServerFlag",		appEnableTestServerFlag,		METH_VARARGS },
		{ "IsEnableTestServerFlag",		appIsEnableTestServerFlag,		METH_VARARGS },

		{ "SetGuildMarkPath",			appSetGuildMarkPath,			METH_VARARGS },

		{ "ShellExecute",				appShellExecute,				METH_VARARGS },
		{ "System",						appSystem,						METH_VARARGS },
		{ "GetDateByTimestamp",			appGetDateByTimestamp,			METH_VARARGS },
		{ "Restart",					appRestart,						METH_VARARGS },

#ifdef ENABLE_MULTI_DESIGN
		{ "SetSelectedDesignName",		appSetSelectedDesignName,		METH_VARARGS },
		{ "GetSelectedDesignName",		appGetSelectedDesignName,		METH_VARARGS },
#endif
	
		{ "StartFlashApplication",		appStartFlashApplication,		METH_VARARGS },
		{ "StopFlashApplication",		appStopFlashApplication,		METH_VARARGS },
#ifdef CAMY_MODULE
		{ "SetCamyActivityState",		appSetCamyActivityState,		METH_VARARGS },
#endif

		{ "GetImagePtr",				appGetImagePtr,					METH_VARARGS },
		{ "GetImageSize",				appGetImageSize,				METH_VARARGS },

#ifdef ENABLE_LEGENDARY_SKILL
		{ "SetUseLegendarySkills",		appSetUseLegendarySkills,		METH_VARARGS },
#endif

		{ "SetUseNewFont",				appSetUseNewFont,				METH_VARARGS },
	
		{ "GetProcessorCoreNum",		appGetProcessorCoreNum,			METH_VARARGS },
		{ "IsInVirutalMachine",			appIsInVirutalMachine,			METH_VARARGS },

		{ "MyShopDecoBGCreate",			appMyShopDecoBGCreate,			METH_VARARGS },
		{ "GetHWID",					appGetHWID,						METH_VARARGS },

		{ NULL, NULL },
	};

	PyObject * poModule = Py_InitModule("app", s_methods);

	PyModule_AddIntConstant(poModule, "INFO_ITEM",		CPythonApplication::INFO_ITEM);
	PyModule_AddIntConstant(poModule, "INFO_ACTOR",		CPythonApplication::INFO_ACTOR);
	PyModule_AddIntConstant(poModule, "INFO_EFFECT",	CPythonApplication::INFO_EFFECT);
	PyModule_AddIntConstant(poModule, "INFO_TEXTTAIL",	CPythonApplication::INFO_TEXTTAIL);

	PyModule_AddIntConstant(poModule, "DEGREE_DIRECTION_SAME",		DEGREE_DIRECTION_SAME);
	PyModule_AddIntConstant(poModule, "DEGREE_DIRECTION_RIGHT",		DEGREE_DIRECTION_RIGHT);
	PyModule_AddIntConstant(poModule, "DEGREE_DIRECTION_LEFT",		DEGREE_DIRECTION_LEFT);

	PyModule_AddIntConstant(poModule, "VK_LEFT",	     VK_LEFT);
	PyModule_AddIntConstant(poModule, "VK_RIGHT",	     VK_RIGHT);
	PyModule_AddIntConstant(poModule, "VK_UP",		     VK_UP);
	PyModule_AddIntConstant(poModule, "VK_DOWN",	     VK_DOWN);
	PyModule_AddIntConstant(poModule, "VK_HOME",	     VK_HOME);
	PyModule_AddIntConstant(poModule, "VK_END",          VK_END);
	PyModule_AddIntConstant(poModule, "VK_DELETE",	     VK_DELETE);

	PyModule_AddIntConstant(poModule, "DIK_ESCAPE",      DIK_ESCAPE);
	PyModule_AddIntConstant(poModule, "DIK_ESC",         DIK_ESCAPE);	// 편의를 위해
	PyModule_AddIntConstant(poModule, "DIK_1",           DIK_1);
	PyModule_AddIntConstant(poModule, "DIK_2",           DIK_2);
	PyModule_AddIntConstant(poModule, "DIK_3",           DIK_3);
	PyModule_AddIntConstant(poModule, "DIK_4",           DIK_4);
	PyModule_AddIntConstant(poModule, "DIK_5",           DIK_5);
	PyModule_AddIntConstant(poModule, "DIK_6",           DIK_6);
	PyModule_AddIntConstant(poModule, "DIK_7",           DIK_7);
	PyModule_AddIntConstant(poModule, "DIK_8",           DIK_8);
	PyModule_AddIntConstant(poModule, "DIK_9",           DIK_9);
	PyModule_AddIntConstant(poModule, "DIK_0",           DIK_0);
	PyModule_AddIntConstant(poModule, "DIK_MINUS",       DIK_MINUS);        /* - on main keyboard */
	PyModule_AddIntConstant(poModule, "DIK_EQUALS",      DIK_EQUALS);         
	PyModule_AddIntConstant(poModule, "DIK_BACK",        DIK_BACK);           /* backspace */
	PyModule_AddIntConstant(poModule, "DIK_TAB",         DIK_TAB);            
	PyModule_AddIntConstant(poModule, "DIK_Q",           DIK_Q);
	PyModule_AddIntConstant(poModule, "DIK_W",           DIK_W);
	PyModule_AddIntConstant(poModule, "DIK_E",           DIK_E);
	PyModule_AddIntConstant(poModule, "DIK_R",           DIK_R);
	PyModule_AddIntConstant(poModule, "DIK_T",           DIK_T);
	PyModule_AddIntConstant(poModule, "DIK_Y",           DIK_Y);
	PyModule_AddIntConstant(poModule, "DIK_U",           DIK_U);
	PyModule_AddIntConstant(poModule, "DIK_I",           DIK_I);
	PyModule_AddIntConstant(poModule, "DIK_O",           DIK_O);
	PyModule_AddIntConstant(poModule, "DIK_P",           DIK_P);
	PyModule_AddIntConstant(poModule, "DIK_LBRACKET",    DIK_LBRACKET);       
	PyModule_AddIntConstant(poModule, "DIK_RBRACKET",    DIK_RBRACKET);       
	PyModule_AddIntConstant(poModule, "DIK_RETURN",      DIK_RETURN);         /* Enter on main keyboard */
	PyModule_AddIntConstant(poModule, "DIK_LCONTROL",    DIK_LCONTROL);       
	PyModule_AddIntConstant(poModule, "DIK_A",           DIK_A);
	PyModule_AddIntConstant(poModule, "DIK_S",           DIK_S);
	PyModule_AddIntConstant(poModule, "DIK_D",           DIK_D);
	PyModule_AddIntConstant(poModule, "DIK_F",           DIK_F);
	PyModule_AddIntConstant(poModule, "DIK_G",           DIK_G);
	PyModule_AddIntConstant(poModule, "DIK_H",           DIK_H);
	PyModule_AddIntConstant(poModule, "DIK_J",           DIK_J);
	PyModule_AddIntConstant(poModule, "DIK_K",           DIK_K);
	PyModule_AddIntConstant(poModule, "DIK_L",           DIK_L);
	PyModule_AddIntConstant(poModule, "DIK_SEMICOLON",   DIK_SEMICOLON);      
	PyModule_AddIntConstant(poModule, "DIK_APOSTROPHE",  DIK_APOSTROPHE);     
	PyModule_AddIntConstant(poModule, "DIK_GRAVE",       DIK_GRAVE);          /* accent grave */
	PyModule_AddIntConstant(poModule, "DIK_LSHIFT",      DIK_LSHIFT);         
	PyModule_AddIntConstant(poModule, "DIK_BACKSLASH",   DIK_BACKSLASH);      
	PyModule_AddIntConstant(poModule, "DIK_Z",           DIK_Z);
	PyModule_AddIntConstant(poModule, "DIK_X",           DIK_X);
	PyModule_AddIntConstant(poModule, "DIK_C",           DIK_C);
	PyModule_AddIntConstant(poModule, "DIK_V",           DIK_V);
	PyModule_AddIntConstant(poModule, "DIK_B",           DIK_B);
	PyModule_AddIntConstant(poModule, "DIK_N",           DIK_N);
	PyModule_AddIntConstant(poModule, "DIK_M",           DIK_M);
	PyModule_AddIntConstant(poModule, "DIK_COMMA",       DIK_COMMA);          
	PyModule_AddIntConstant(poModule, "DIK_PERIOD",      DIK_PERIOD);         /* . on main keyboard */
	PyModule_AddIntConstant(poModule, "DIK_SLASH",       DIK_SLASH);          /* / on main keyboard */
	PyModule_AddIntConstant(poModule, "DIK_RSHIFT",      DIK_RSHIFT);         
	PyModule_AddIntConstant(poModule, "DIK_MULTIPLY",    DIK_MULTIPLY);       /* * on numeric keypad */
	PyModule_AddIntConstant(poModule, "DIK_LALT",        DIK_LMENU);          /* left Alt */
	PyModule_AddIntConstant(poModule, "DIK_SPACE",       DIK_SPACE);          
	PyModule_AddIntConstant(poModule, "DIK_CAPITAL",     DIK_CAPITAL);        
	PyModule_AddIntConstant(poModule, "DIK_F1",          DIK_F1);
	PyModule_AddIntConstant(poModule, "DIK_F2",          DIK_F2);
	PyModule_AddIntConstant(poModule, "DIK_F3",          DIK_F3);
	PyModule_AddIntConstant(poModule, "DIK_F4",          DIK_F4);
	PyModule_AddIntConstant(poModule, "DIK_F5",          DIK_F5);
	PyModule_AddIntConstant(poModule, "DIK_F6",          DIK_F6);
	PyModule_AddIntConstant(poModule, "DIK_F7",          DIK_F7);
	PyModule_AddIntConstant(poModule, "DIK_F8",          DIK_F8);
	PyModule_AddIntConstant(poModule, "DIK_F9",          DIK_F9);
	PyModule_AddIntConstant(poModule, "DIK_F10",         DIK_F10);
	PyModule_AddIntConstant(poModule, "DIK_NUMLOCK",     DIK_NUMLOCK);
	PyModule_AddIntConstant(poModule, "DIK_SCROLL",      DIK_SCROLL);         /* Scroll Lock */
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD7",     DIK_NUMPAD7);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD8",     DIK_NUMPAD8);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD9",     DIK_NUMPAD9);        
	PyModule_AddIntConstant(poModule, "DIK_SUBTRACT",    DIK_SUBTRACT);       /* - on numeric keypad */
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD4",     DIK_NUMPAD4);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD5",     DIK_NUMPAD5);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD6",     DIK_NUMPAD6);        
	PyModule_AddIntConstant(poModule, "DIK_ADD",         DIK_ADD);            /* + on numeric keypad */
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD1",     DIK_NUMPAD1);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD2",     DIK_NUMPAD2);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD3",     DIK_NUMPAD3);        
	PyModule_AddIntConstant(poModule, "DIK_NUMPAD0",     DIK_NUMPAD0);        
	PyModule_AddIntConstant(poModule, "DIK_DECIMAL",     DIK_DECIMAL);        /* . on numeric keypad */
	PyModule_AddIntConstant(poModule, "DIK_F11",         DIK_F11);            
	PyModule_AddIntConstant(poModule, "DIK_F12",         DIK_F12);            
	PyModule_AddIntConstant(poModule, "DIK_NEXTTRACK",   DIK_NEXTTRACK);      /* Next Track */
	PyModule_AddIntConstant(poModule, "DIK_NUMPADENTER", DIK_NUMPADENTER);    /* Enter on numeric keypad */
	PyModule_AddIntConstant(poModule, "DIK_RCONTROL",    DIK_RCONTROL);       
	PyModule_AddIntConstant(poModule, "DIK_MUTE",        DIK_MUTE);           /* Mute */
	PyModule_AddIntConstant(poModule, "DIK_CALCULATOR",  DIK_CALCULATOR);     /* Calculator */
	PyModule_AddIntConstant(poModule, "DIK_PLAYPAUSE",   DIK_PLAYPAUSE);      /* Play / Pause */
	PyModule_AddIntConstant(poModule, "DIK_MEDIASTOP",   DIK_MEDIASTOP);      /* Media Stop */
	PyModule_AddIntConstant(poModule, "DIK_VOLUMEDOWN",  DIK_VOLUMEDOWN);     /* Volume - */
	PyModule_AddIntConstant(poModule, "DIK_VOLUMEUP",    DIK_VOLUMEUP);       /* Volume + */
	PyModule_AddIntConstant(poModule, "DIK_WEBHOME",     DIK_WEBHOME);        /* Web home */
	PyModule_AddIntConstant(poModule, "DIK_NUMPADCOMMA", DIK_NUMPADCOMMA);    /* , on numeric keypad (NEC PC98) */
	PyModule_AddIntConstant(poModule, "DIK_DIVIDE",      DIK_DIVIDE);         /* / on numeric keypad */
	PyModule_AddIntConstant(poModule, "DIK_SYSRQ",       DIK_SYSRQ);          
	PyModule_AddIntConstant(poModule, "DIK_RALT",        DIK_RMENU);          /* right Alt */
	PyModule_AddIntConstant(poModule, "DIK_PAUSE",       DIK_PAUSE);          /* Pause */
	PyModule_AddIntConstant(poModule, "DIK_HOME",        DIK_HOME);           /* Home on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_UP",          DIK_UP);             /* UpArrow on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_PGUP",        DIK_PRIOR);          /* PgUp on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_LEFT",        DIK_LEFT);           /* LeftArrow on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_RIGHT",       DIK_RIGHT);          /* RightArrow on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_END",         DIK_END);            /* End on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_DOWN",        DIK_DOWN);           /* DownArrow on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_PGDN",        DIK_NEXT);           /* PgDn on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_INSERT",      DIK_INSERT);         /* Insert on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_DELETE",      DIK_DELETE);         /* Delete on arrow keypad */
	PyModule_AddIntConstant(poModule, "DIK_LWIN",        DIK_LWIN);           /* Left Windows key */
	PyModule_AddIntConstant(poModule, "DIK_RWIN",        DIK_RWIN);           /* Right Windows key */
	PyModule_AddIntConstant(poModule, "DIK_APPS",        DIK_APPS);           /* AppMenu key */

	// Cursor
	PyModule_AddIntConstant(poModule, "NORMAL",			CPythonApplication::CURSOR_SHAPE_NORMAL);
	PyModule_AddIntConstant(poModule, "ATTACK",			CPythonApplication::CURSOR_SHAPE_ATTACK);
	PyModule_AddIntConstant(poModule, "TARGET",			CPythonApplication::CURSOR_SHAPE_TARGET);
	PyModule_AddIntConstant(poModule, "TALK",			CPythonApplication::CURSOR_SHAPE_TALK);
	PyModule_AddIntConstant(poModule, "CANT_GO",		CPythonApplication::CURSOR_SHAPE_CANT_GO);
	PyModule_AddIntConstant(poModule, "PICK",			CPythonApplication::CURSOR_SHAPE_PICK);

	PyModule_AddIntConstant(poModule, "DOOR",			CPythonApplication::CURSOR_SHAPE_DOOR);
	PyModule_AddIntConstant(poModule, "CHAIR",			CPythonApplication::CURSOR_SHAPE_CHAIR);
	PyModule_AddIntConstant(poModule, "MAGIC",			CPythonApplication::CURSOR_SHAPE_MAGIC);
	PyModule_AddIntConstant(poModule, "BUY",			CPythonApplication::CURSOR_SHAPE_BUY);
	PyModule_AddIntConstant(poModule, "SELL",			CPythonApplication::CURSOR_SHAPE_SELL);

	PyModule_AddIntConstant(poModule, "CAMERA_ROTATE",	CPythonApplication::CURSOR_SHAPE_CAMERA_ROTATE);
	PyModule_AddIntConstant(poModule, "HSIZE",			CPythonApplication::CURSOR_SHAPE_HSIZE);
	PyModule_AddIntConstant(poModule, "VSIZE",			CPythonApplication::CURSOR_SHAPE_VSIZE);
	PyModule_AddIntConstant(poModule, "HVSIZE",			CPythonApplication::CURSOR_SHAPE_HVSIZE);

	PyModule_AddIntConstant(poModule, "CAMERA_TO_POSITIVE",		CPythonApplication::CAMERA_TO_POSITIVE);
	PyModule_AddIntConstant(poModule, "CAMERA_TO_NEGATIVE",		CPythonApplication::CAMERA_TO_NEGITIVE);
	PyModule_AddIntConstant(poModule, "CAMERA_STOP",			CPythonApplication::CAMERA_STOP);
	
	PyModule_AddIntConstant(poModule, "LANG_COUNT",		LANGUAGE_MAX_NUM);
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		char szLangName[50];
		strcpy(szLangName, CLocaleManager::instance().GetLanguageNameByID(i, false));
		char szLangNameBuf[50];
		snprintf(szLangNameBuf, sizeof(szLangNameBuf), "LANG_%s", strupr(szLangName));
		PyModule_AddIntConstant(poModule, szLangNameBuf, i);
		snprintf(szLangNameBuf, sizeof(szLangNameBuf), "LANG_%u", i, szLangName);
		PyModule_AddStringConstant(poModule, szLangNameBuf, szLangName);
	}
	PyModule_AddIntConstant(poModule, "LANG_SELECTED",	CLocaleManager::Instance().GetLanguage());

#ifdef ENABLE_COSTUME_SYSTEM
	PyModule_AddIntConstant(poModule, "ENABLE_COSTUME_SYSTEM",	1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_COSTUME_SYSTEM",	0);
#endif

#ifdef ENABLE_MESSENGER_BLOCK
	PyModule_AddIntConstant(poModule, "ENABLE_MESSENGER_BLOCK",	1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_MESSENGER_BLOCK",	0);
#endif

#ifdef __ELEMENT_SYSTEM__
	PyModule_AddIntConstant(poModule, "ENABLE_VIEW_ELEMENT", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_VIEW_ELEMENT", 0);
#endif

#ifdef ENABLE_ENERGY_SYSTEM
	PyModule_AddIntConstant(poModule, "ENABLE_ENERGY_SYSTEM",	1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_ENERGY_SYSTEM",	0);
#endif

#ifdef ENABLE_SKILL_INVENTORY
	PyModule_AddIntConstant(poModule, "ENABLE_SKILLBOOK_INV",	1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_SKILLBOOK_INV",	0);
#endif

#ifdef ENABLE_HYDRA_DUNGEON
	PyModule_AddIntConstant(poModule, "ENABLE_HYDRA_DUNGEON", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_HYDRA_DUNGEON", 0);
#endif

PyModule_AddIntConstant(poModule, "USE_OPENID",	0);
PyModule_AddIntConstant(poModule, "OPENID_TEST",	0);


#ifdef ENABLE_DRAGONSOUL
	PyModule_AddIntConstant(poModule, "ENABLE_DRAGON_SOUL_SYSTEM", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_DRAGON_SOUL_SYSTEM", 0);
#endif
#ifdef CAMY_MODULE
	//PyModule_AddIntConstant(poModule, "CAMY_COMPATIBLE_CLIENT", TRUE);
	PyModule_AddIntConstant(poModule, "CAMY_COMPATIBLE_CLIENT",	FALSE);
#else
	PyModule_AddIntConstant(poModule, "CAMY_COMPATIBLE_CLIENT", FALSE);
#endif

#ifdef ENABLE_MELEY_LAIR_DUNGEON
	PyModule_AddIntConstant(poModule, "ENABLE_MELEY_LAIR_DUNGEON", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_MELEY_LAIR_DUNGEON", 0);
#endif

#ifdef ENABLE_WOLFMAN
	PyModule_AddIntConstant(poModule, "ENABLE_WOLFMAN_CHARACTER", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_WOLFMAN_CHARACTER", 0);
#endif

#ifdef ENABLE_VOTEBOT
	PyModule_AddIntConstant(poModule, "ENABLE_VOTEBOT", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_VOTEBOT", 0);
#endif

	PyModule_AddIntConstant(poModule, "ENABLE_BATTLE_FIELD", 0);
	PyModule_AddIntConstant(poModule, "ENABLE_DELETE_FAILURE_TYPE", 0);
	PyModule_AddIntConstant(poModule, "ENABLE_MONSTER_CARD", 0);
	PyModule_AddIntConstant(poModule, "ENABLE_STEAM", 0);

	PyModule_AddIntConstant(poModule, "ENABLE_GUILDRENEWAL_SYSTEM", 1);
	PyModule_AddIntConstant(poModule, "ENABLE_SECOND_GUILDRENEWAL_SYSTEM", 1);
	PyModule_AddIntConstant(poModule, "ENABLE_CHEQUE_SYSTEM", 0);
	PyModule_AddIntConstant(poModule, "ENABLE_10TH_EVENT", 0); 
	PyModule_AddIntConstant(poModule, "ENABLE_GUILD_MARK_RENEWAL", 0);

#ifdef WEBBROWSER_OFFSCREEN
	PyModule_AddIntConstant(poModule, "ENABLE_WEB_OFFSCREEN", 1);

	PyModule_AddIntConstant(poModule, "EVENT_MOUSE_MOVE", CPythonNewWeb::EVENT_MOUSE_MOVE);
	PyModule_AddIntConstant(poModule, "EVENT_MOUSE_LEFT_DOWN", CPythonNewWeb::EVENT_MOUSE_LEFT_DOWN);
	PyModule_AddIntConstant(poModule, "EVENT_MOUSE_LEFT_UP", CPythonNewWeb::EVENT_MOUSE_LEFT_UP);
	PyModule_AddIntConstant(poModule, "EVENT_MOUSE_RIGHT_DOWN", CPythonNewWeb::EVENT_MOUSE_RIGHT_DOWN);
	PyModule_AddIntConstant(poModule, "EVENT_MOUSE_RIGHT_UP", CPythonNewWeb::EVENT_MOUSE_RIGHT_UP);
	PyModule_AddIntConstant(poModule, "EVENT_MOUSE_WHEEL", CPythonNewWeb::EVENT_MOUSE_WHEEL);
	PyModule_AddIntConstant(poModule, "EVENT_MAX_NUM", CPythonNewWeb::EVENT_MAX_NUM);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_WEB_OFFSCREEN", 0);
#endif

#ifdef ENABLE_AGGREGATE_MONSTER_EFFECT
	PyModule_AddIntConstant(poModule, "ENABLE_AGGREGATE_MONSTER_EFFECT",	1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_AGGREGATE_MONSTER_EFFECT",	0);
#endif
#ifdef ENABLE_COSTUME_BONUS_TRANSFER
	PyModule_AddIntConstant(poModule, "ENABLE_COSTUME_BONUS_TRANSFER", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_COSTUME_BONUS_TRANSFER", 0);
#endif

#ifdef COMBAT_ZONE
	PyModule_AddIntConstant(poModule, "COMBAT_ZONE", 1);
#else
	PyModule_AddIntConstant(poModule, "COMBAT_ZONE", 0);
#endif

#if defined(ENABLE_AFFECT_POLYMORPH_REMOVE)
	PyModule_AddIntConstant(poModule, "ENABLE_AFFECT_POLYMORPH_REMOVE", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_AFFECT_POLYMORPH_REMOVE", 0);
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_1", FISH_EVENT_SHAPE_1);
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_2", FISH_EVENT_SHAPE_2);
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_3", FISH_EVENT_SHAPE_3);
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_4", FISH_EVENT_SHAPE_4);
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_5", FISH_EVENT_SHAPE_5);
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_6", FISH_EVENT_SHAPE_6);
	PyModule_AddIntConstant(poModule, "FISH_EVENT_SHAPE_7", FISH_EVENT_SHAPE_7);
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
	PyModule_AddIntConstant(poModule, "AHMET_FISH_EVENT_SYSTEM", 1);
#else
	PyModule_AddIntConstant(poModule, "AHMET_FISH_EVENT_SYSTEM", 0);
#endif

	PyModule_AddIntConstant(poModule, "RENDER_TARGET_MYSHOPDECO", CPythonApplication::RENDER_TARGET_MYSHOPDECO);
	PyModule_AddIntConstant(poModule, "RENDER_TARGET_MAX_NUM", CPythonApplication::RENDER_TARGET_MAX_NUM);

#ifdef ENABLE_SKIN_SYSTEM
	PyModule_AddIntConstant(poModule, "ENABLE_SKIN_SYSTEM", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_SKIN_SYSTEM", 0);
#endif

#ifdef ENABLE_ZODIAC
	PyModule_AddIntConstant(poModule, "ENABLE_ZODIAC",	1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_ZODIAC",	0);
#endif

#ifdef __ARABIC_LANG__
	PyModule_AddIntConstant(poModule, "ARABIC_LANG", 1);
#else
	PyModule_AddIntConstant(poModule, "ARABIC_LANG", 0);
#endif

#ifdef ENABLE_RUNE_AFFECT_ICONS
	PyModule_AddIntConstant(poModule, "ENABLE_RUNE_AFFECT_ICONS", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_RUNE_AFFECT_ICONS", 0);
#endif

#ifdef PROMETA
	PyModule_AddIntConstant(poModule, "PROMETA", 1);
#else
	PyModule_AddIntConstant(poModule, "PROMETA", 0);
#endif

}