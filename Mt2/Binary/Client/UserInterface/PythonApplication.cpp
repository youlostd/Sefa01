#include "StdAfx.h"
#include "../eterBase/Error.h"
#include "../eterlib/Camera.h"
#include "../eterlib/AttributeInstance.h"
#include "../gamelib/AreaTerrain.h"
#include "../EterGrnLib/Material.h"
#include "../CWebBrowser/CWebBrowser.h"

#include "resource.h"
#include "PythonApplication.h"
#include "PythonCharacterManager.h"
#include "PythonMyShopDecoManager.h"

#include "Test.h"

#ifdef ENABLE_LEGENDARY_SKILL
bool g_bUseLegendarySkills = false;
#endif

extern const char* g_szCurrentClientVersion;

extern void GrannyCreateSharedDeformBuffer();
extern void GrannyDestroySharedDeformBuffer();

#ifndef CAMY_MODULE
float MIN_FOG = 2400.0f;
#else
float MIN_FOG = 12800.0f;
#endif
double g_specularSpd=0.007f;

CPythonApplication * CPythonApplication::ms_pInstance;

float c_fDefaultCameraRotateSpeed = 1.5f;
float c_fDefaultCameraPitchSpeed = 1.5f;
float c_fDefaultCameraZoomSpeed = 0.05f;

CPythonApplication::CPythonApplication() :
m_bCursorVisible(TRUE),
m_bLiarCursorOn(false),
m_iCursorMode(CURSOR_MODE_HARDWARE),
m_isWindowed(false),
m_isFrameSkipDisable(false),
m_poMouseHandler(NULL),
m_dwUpdateFPS(0),
m_dwRenderFPS(0),
m_fAveRenderTime(0.0f),
m_dwFaceCount(0),
m_fGlobalTime(0.0f),
m_fGlobalElapsedTime(0.0f),
m_dwLButtonDownTime(0),
m_dwLastIdleTime(0)
{
#ifndef _DEBUG
	SetEterExceptionHandler();
#endif

	CTimer::Instance().UseCustomTime();
	m_dwWidth = 800;
	m_dwHeight = 600;

	ms_pInstance = this;
	m_isWindowFullScreenEnable = FALSE;

	m_v3CenterPosition = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
	m_dwStartLocalTime = ELTimer_GetMSec();
	m_tServerTime = 0;
	m_tLocalStartTime = 0;

	m_iPort = 0;
	m_iFPS = 60;

	m_isActivateWnd = false;
	m_isMinimizedWnd = true;

	m_fRotationSpeed = 0.0f;
	m_fPitchSpeed = 0.0f;
	m_fZoomSpeed = 0.0f;

	m_fFaceSpd=0.0f;

	m_dwFaceAccCount=0;
	m_dwFaceAccTime=0;

	m_dwFaceSpdSum=0;
	m_dwFaceSpdCount=0;

	m_FlyingManager.SetMapManagerPtr(&m_pyBackground);

	m_iCursorNum = CURSOR_SHAPE_NORMAL;
	m_iContinuousCursorNum = CURSOR_SHAPE_NORMAL;

	m_isSpecialCameraMode = FALSE;
	m_fCameraRotateSpeed = c_fDefaultCameraRotateSpeed;
	m_fCameraPitchSpeed = c_fDefaultCameraPitchSpeed;
	m_fCameraZoomSpeed = c_fDefaultCameraZoomSpeed;

#ifdef CAMY_MODULE
	m_bCamyIsActive = false;
#endif
	m_iCameraMode = CAMERA_MODE_NORMAL;
	m_fBlendCameraStartTime = 0.0f;
	m_fBlendCameraBlendTime = 0.0f;

	m_iForceSightRange = -1;
}

CPythonApplication::~CPythonApplication()
{
	ClearDynamicImagePtr();
}

void CPythonApplication::GetMousePosition(POINT* ppt)
{
	CMSApplication::GetMousePosition(ppt);
}

void CPythonApplication::SetMinFog(float fMinFog)
{
	MIN_FOG = fMinFog;
}

void CPythonApplication::SetFrameSkip(bool isEnable)
{
	if (isEnable)
		m_isFrameSkipDisable=false;
	else
		m_isFrameSkipDisable=true;
}

void CPythonApplication::NotifyHack(const char* c_szFormat, ...)
{
	char szBuf[1024];

	va_list args;
	va_start(args, c_szFormat);	
	_vsnprintf(szBuf, sizeof(szBuf), c_szFormat, args);
	va_end(args);
	m_pyNetworkStream.NotifyHack(szBuf);
}

/*void CPythonApplication::GetInfo(UINT eInfo, std::string* pstInfo)
{
	switch (eInfo)
	{
	case INFO_ACTOR:
		m_kChrMgr.GetInfo(pstInfo);
		break;
	case INFO_EFFECT:
		m_kEftMgr.GetInfo(pstInfo);			
		break;
	case INFO_ITEM:
		m_pyItem.GetInfo(pstInfo);
		break;
	case INFO_TEXTTAIL:
		m_pyTextTail.GetInfo(pstInfo);
		break;
	}
}*/

void CPythonApplication::Abort()
{
	TraceError("============================================================================================================");
	TraceError("Abort!!!!\n\n");

	PostQuitMessage(0);
}

void CPythonApplication::Exit()
{
	PostQuitMessage(0);
}

#ifdef __PERFORMANCE_CHECKER__
bool PERF_CHECKER_RENDER_GAME = true;
#else
bool PERF_CHECKER_RENDER_GAME = false;
#endif

void CPythonApplication::RenderGame()
{	
	if (!PERF_CHECKER_RENDER_GAME)
	{
		m_pyMyShopDecoManager.RenderBackground();
		float fAspect=m_kWndMgr.GetAspect();
		float fFarClip=m_pyBackground.GetFarClip();

		m_pyGraphic.SetPerspective(30.0f, fAspect, 100.0, fFarClip);

		CCullingManager::Instance().Process();

		m_kChrMgr.Deform();
		m_pyMyShopDecoManager.DeformModel();
		m_pyWikiModelViewManager.DeformModel();
		m_kEftMgr.Update();

		m_pyBackground.RenderCharacterShadowToTexture();

		m_pyGraphic.SetGameRenderState();
		m_pyGraphic.PushState();

		{
			long lx, ly;
			m_kWndMgr.GetMousePosition(lx, ly);
			m_pyGraphic.SetCursorPosition(lx, ly);
		}

		m_pyBackground.RenderSky();

		m_pyBackground.RenderBeforeLensFlare();

		m_pyBackground.RenderCloud();

		m_pyBackground.BeginEnvironment();
		m_pyBackground.Render();

		m_pyBackground.SetCharacterDirLight();
		m_pyMyShopDecoManager.RenderModel();
		m_pyWikiModelViewManager.RenderModel();
		m_kChrMgr.Render();

		m_pyBackground.SetBackgroundDirLight();
		m_pyBackground.RenderWater();
		m_pyBackground.RenderSnow();
		m_pyBackground.RenderEffect();

		m_pyBackground.EndEnvironment();

		m_kEftMgr.Render();
		m_pyItem.Render();
		m_FlyingManager.Render();

		m_pyBackground.BeginEnvironment();
		m_pyBackground.RenderPCBlocker();
		m_pyBackground.EndEnvironment();

		m_pyBackground.RenderAfterLensFlare();

		return;
	}

	//if (GetAsyncKeyState(VK_Z))
	//	STATEMANAGER.SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);

	DWORD t1=ELTimer_GetMSec();
	m_kChrMgr.Deform();
	DWORD t2=ELTimer_GetMSec();
	m_kEftMgr.Update();
	DWORD t3=ELTimer_GetMSec();
	m_pyBackground.RenderCharacterShadowToTexture();
	DWORD t4=ELTimer_GetMSec();

	m_pyGraphic.SetGameRenderState();
	m_pyGraphic.PushState();

	float fAspect=m_kWndMgr.GetAspect();
	float fFarClip=m_pyBackground.GetFarClip();

	m_pyGraphic.SetPerspective(30.0f, fAspect, 100.0, fFarClip);

	DWORD t5=ELTimer_GetMSec();

	CCullingManager::Instance().Process();

	DWORD t6=ELTimer_GetMSec();

	{
		long lx, ly;
		m_kWndMgr.GetMousePosition(lx, ly);
		m_pyGraphic.SetCursorPosition(lx, ly);
	}

	m_pyBackground.RenderSky();
	DWORD t7=ELTimer_GetMSec();
	m_pyBackground.RenderBeforeLensFlare();
	DWORD t8=ELTimer_GetMSec();
	m_pyBackground.RenderCloud();
	DWORD t9=ELTimer_GetMSec();
	m_pyBackground.BeginEnvironment();
	m_pyBackground.Render();

	m_pyBackground.SetCharacterDirLight();
	DWORD t10=ELTimer_GetMSec();
	m_kChrMgr.Render();
	DWORD t11=ELTimer_GetMSec();

	m_pyBackground.SetBackgroundDirLight();
	m_pyBackground.RenderWater();
	DWORD t12=ELTimer_GetMSec();
	m_pyBackground.RenderEffect();
	DWORD t13=ELTimer_GetMSec();
	m_pyBackground.EndEnvironment();
	m_kEftMgr.Render();
	DWORD t14=ELTimer_GetMSec();
	m_pyItem.Render();
	DWORD t15=ELTimer_GetMSec();
	m_FlyingManager.Render();
	DWORD t16=ELTimer_GetMSec();
	m_pyBackground.BeginEnvironment();
	m_pyBackground.RenderPCBlocker();
	m_pyBackground.EndEnvironment();
	DWORD t17=ELTimer_GetMSec();
	m_pyBackground.RenderAfterLensFlare();
	DWORD t18=ELTimer_GetMSec();
	DWORD tEnd=ELTimer_GetMSec();

#ifdef __PERFORMANCE_CHECKER__
	if (GetAsyncKeyState(VK_Z))
		STATEMANAGER.SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	if (tEnd-t1<3)
		return;

	static FILE* fp=fopen("perf_game_render.txt", "w");

	fprintf(fp, "GR.Total %d (Time %.3f)\n", tEnd-t1, ELTimer_GetMSec()/1000.0f);
	fprintf(fp, "GR.DFM %d\n", t2-t1);
	fprintf(fp, "GR.EFT.UP %d\n", t3-t2);
	fprintf(fp, "GR.SHW %d\n", t4-t3);
	fprintf(fp, "GR.STT %d\n", t5-t4);
	fprintf(fp, "GR.CLL %d\n", t6-t5);
	fprintf(fp, "GR.BG.SKY %d\n", t7-t6);
	fprintf(fp, "GR.BG.LEN %d\n", t8-t7);
	fprintf(fp, "GR.BG.CLD %d\n", t9-t8);
	fprintf(fp, "GR.BG.MAIN %d\n", t10-t9);		
	fprintf(fp, "GR.CHR %d\n",	t11-t10);
	fprintf(fp, "GR.BG.WTR %d\n", t12-t11);
	fprintf(fp, "GR.BG.EFT %d\n", t13-t12);
	fprintf(fp, "GR.EFT %d\n", t14-t13);
	fprintf(fp, "GR.ITM %d\n", t15-t14);
	fprintf(fp, "GR.FLY %d\n", t16-t15);
	fprintf(fp, "GR.BG.BLK %d\n", t17-t16);
	fprintf(fp, "GR.BG.LEN %d\n", t18-t17);
	fprintf(fp, "-------------------------------- \n");


	fflush(fp);
#endif
}

void CPythonApplication::UpdateGame()
{
	DWORD t1=ELTimer_GetMSec();
	POINT ptMouse;
	GetMousePosition(&ptMouse);

	CGraphicTextInstance::Hyperlink_UpdateMousePos(ptMouse.x, ptMouse.y);

	DWORD t2=ELTimer_GetMSec();

	//!@# Alt+Tab �� SetTransfor ���� ƨ�� ���� �ذ��� ���� - [levites]
	//if (m_isActivateWnd)
	{
		CScreen s;
		float fAspect = UI::CWindowManager::Instance().GetAspect();
		float fFarClip = CPythonBackground::Instance().GetFarClip();

		s.SetPerspective(30.0f,fAspect, 100.0f, fFarClip);
		s.BuildViewFrustum();
	}

	DWORD t3=ELTimer_GetMSec();
	TPixelPosition kPPosMainActor;
	
#ifndef CAMY_MODULE
	m_pyPlayer.NEW_GetMainActorPosition(&kPPosMainActor);
#else
	if (!m_bCamyIsActive) {
		m_pyPlayer.NEW_GetMainActorPosition(&kPPosMainActor);
	} else {
		CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
		m_pyGraphic.GetCameraPosition(&kPPosMainActor.x, &kPPosMainActor.y, &kPPosMainActor.z);
	}
#endif

	// if (closeF > 0 && GetGlobalTime() > closeF)
	// 	exit(random_range(-1, 5));

	DWORD t4=ELTimer_GetMSec();
	m_pyBackground.Update(kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);

	DWORD t5=ELTimer_GetMSec();
	m_GameEventManager.SetCenterPosition(kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
	m_GameEventManager.Update();

	DWORD t6=ELTimer_GetMSec();
	m_kChrMgr.Update();	
	DWORD t7=ELTimer_GetMSec();
	m_kEftMgr.UpdateSound();
	DWORD t8=ELTimer_GetMSec();
	m_FlyingManager.Update();
	DWORD t9=ELTimer_GetMSec();
	m_pyItem.Update(ptMouse);
	DWORD t10=ELTimer_GetMSec();
	m_pyPlayer.Update();
	DWORD t11=ELTimer_GetMSec();

	// NOTE : Update ���� ��ġ ���� �ٲ�Ƿ� �ٽ� ��� �ɴϴ� - [levites]
	//        �� �κ� ������ ���� �ɸ����� Sound�� ���� ��ġ���� �÷��� �Ǵ� ������ �־���.
	m_pyPlayer.NEW_GetMainActorPosition(&kPPosMainActor);
	SetCenterPosition(kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
	DWORD t12=ELTimer_GetMSec();

	m_pyMyShopDecoManager.UpdateModel();
	m_pyWikiModelViewManager.UpdateModel();

#ifdef __PERFORMANCE_CHECKER__
	if (PERF_CHECKER_RENDER_GAME)
	{
		if (t12-t1>5)
		{
			static FILE* fp=fopen("perf_game_update.txt", "w");

			fprintf(fp, "GU.Total %d (Time %d)\n", t12-t1, ELTimer_GetMSec());
			fprintf(fp, "GU.GMP %d\n", t2-t1);
			fprintf(fp, "GU.SCR %d\n", t3-t2);
			fprintf(fp, "GU.MPS %d\n", t4-t3);
			fprintf(fp, "GU.BG %d\n", t5-t4);
			fprintf(fp, "GU.GEM %d\n", t6-t5);
			fprintf(fp, "GU.CHR %d\n", t7-t6);
			fprintf(fp, "GU.EFT %d\n", t8-t7);
			fprintf(fp, "GU.FLY %d\n", t9-t8);
			fprintf(fp, "GU.ITM %d\n", t10-t9);
			fprintf(fp, "GU.PLR %d\n", t11-t10);
			fprintf(fp, "GU.POS %d\n", t12-t11);
			fflush(fp);
		}
	}
#endif
}

void CPythonApplication::SkipRenderBuffering(DWORD dwSleepMSec)
{
	m_dwBufSleepSkipTime=ELTimer_GetMSec()+dwSleepMSec;
}

BOOL CALLBACK EnumWindowsProc(_In_ HWND hwnd, _In_ LPARAM lParam)
{
	if (hwnd == CPythonApplication::Instance().GetWindowHandle())
		return TRUE;

	std::vector<HWND>* pvecHWNDs = (std::vector<HWND>*)lParam;

	char szWindowTitle[128];
	GetWindowText(hwnd, szWindowTitle, sizeof(szWindowTitle));
	if (!strcmp(szWindowTitle, CPythonApplication::Instance().GetWindowTitleName()))
		pvecHWNDs->push_back(hwnd);

	return TRUE;
}

void CPythonApplication::SendMessageToOthers(UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	std::vector<HWND> vecHWNDs;
	EnumWindows(EnumWindowsProc, (LPARAM)&vecHWNDs);

	for (int i = 0; i < vecHWNDs.size(); ++i)
		SendMessage(vecHWNDs[i], uiMsg, wParam, lParam);
}

bool CPythonApplication::Process()
{
#if defined(CHECK_LATEST_DATA_FILES)
	if (CheckLatestFiles_PollEvent())
		return false;
#endif
	ELTimer_SetFrameMSec();

	DWORD dwStart = ELTimer_GetMSec();

	///////////////////////////////////////////////////////////////////////////////////////////////////
	static DWORD	s_dwUpdateFrameCount = 0;
	static DWORD	s_dwRenderFrameCount = 0;
	static DWORD	s_dwFaceCount = 0;
	static UINT		s_uiLoad = 0;
	static DWORD	s_dwCheckTime = ELTimer_GetMSec();

	if (ELTimer_GetMSec() - s_dwCheckTime > 1000)
	{
		m_dwUpdateFPS		= s_dwUpdateFrameCount;
		m_dwRenderFPS		= s_dwRenderFrameCount;
		m_dwLoad			= s_uiLoad;

		m_dwFaceCount		= s_dwFaceCount / MAX(1, s_dwRenderFrameCount);

		s_dwCheckTime		= ELTimer_GetMSec();

		s_uiLoad = s_dwFaceCount = s_dwUpdateFrameCount = s_dwRenderFrameCount = 0;
	}

	// Update Time
	static BOOL s_bFrameSkip = false;
	static UINT s_uiNextFrameTime = ELTimer_GetMSec();

#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime1=ELTimer_GetMSec();
#endif
	CTimer& rkTimer=CTimer::Instance();
	rkTimer.Advance();

	m_fGlobalTime = rkTimer.GetCurrentSecond();
	m_fGlobalElapsedTime = rkTimer.GetElapsedSecond();

	UINT uiFrameTime = rkTimer.GetElapsedMilliecond();
	s_uiNextFrameTime += uiFrameTime;	//17 - 1�ʴ� 60fps����.

	DWORD updatestart = ELTimer_GetMSec();
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime2=ELTimer_GetMSec();
#endif
	// Network I/O	
	m_pyNetworkStream.Process();	
	//m_pyNetworkDatagram.Process();

	m_kGuildMarkUploader.Process();

	m_kGuildMarkDownloader.Process();
	m_kAccountConnector.Process();

#ifdef __PERFORMANCE_CHECK__		
	DWORD dwUpdateTime3=ELTimer_GetMSec();
#endif
	//////////////////////
	// Input Process
	// Keyboard
	UpdateKeyboard();
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime4=ELTimer_GetMSec();
#endif
	// Mouse
	POINT Point;
	if (GetCursorPos(&Point))
	{
		ScreenToClient(m_hWnd, &Point);
		OnMouseMove(Point.x, Point.y);		
	}
	//////////////////////
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime5=ELTimer_GetMSec();
#endif
	//!@# Alt+Tab �� SetTransfor ���� ƨ�� ���� �ذ��� ���� - [levites]
	//if (m_isActivateWnd)
	__UpdateCamera();
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime6=ELTimer_GetMSec();
#endif
	// Update Game Playing
	CResourceManager::Instance().Update();
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime7=ELTimer_GetMSec();
#endif
	OnCameraUpdate();
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime8=ELTimer_GetMSec();
#endif
	OnMouseUpdate();
#ifdef __PERFORMANCE_CHECK__
	DWORD dwUpdateTime9=ELTimer_GetMSec();
#endif
	OnUIUpdate();

#ifdef ENABLE_NEW_WEBBROWSER
	m_pyNewWebbrowser.Update();
#endif

#ifdef __PERFORMANCE_CHECK__		
	DWORD dwUpdateTime10=ELTimer_GetMSec();

	if (dwUpdateTime10-dwUpdateTime1>10)
	{			
		static FILE* fp=fopen("perf_app_update.txt", "w");

		fprintf(fp, "AU.Total %d (Time %d)\n", dwUpdateTime9-dwUpdateTime1, ELTimer_GetMSec());
		fprintf(fp, "AU.TU %d\n", dwUpdateTime2-dwUpdateTime1);
		fprintf(fp, "AU.NU %d\n", dwUpdateTime3-dwUpdateTime2);
		fprintf(fp, "AU.KU %d\n", dwUpdateTime4-dwUpdateTime3);
		fprintf(fp, "AU.MP %d\n", dwUpdateTime5-dwUpdateTime4);
		fprintf(fp, "AU.CP %d\n", dwUpdateTime6-dwUpdateTime5);
		fprintf(fp, "AU.RU %d\n", dwUpdateTime7-dwUpdateTime6);
		fprintf(fp, "AU.CU %d\n", dwUpdateTime8-dwUpdateTime7);
		fprintf(fp, "AU.MU %d\n", dwUpdateTime9-dwUpdateTime8);
		fprintf(fp, "AU.UU %d\n", dwUpdateTime10-dwUpdateTime9);			
		fprintf(fp, "----------------------------------\n");
		fflush(fp);
	}		
#endif

	//Update�ϴµ� �ɸ��ð�.delta��
	m_dwCurUpdateTime = ELTimer_GetMSec() - updatestart;

	DWORD dwCurrentTime = ELTimer_GetMSec();
	BOOL  bCurrentLateUpdate = FALSE;

	s_bFrameSkip = false;

	if (dwCurrentTime > s_uiNextFrameTime)
	{
		int dt = dwCurrentTime - s_uiNextFrameTime;
		int nAdjustTime = ((float)dt / (float)uiFrameTime) * uiFrameTime; 

		if ( dt >= 500 )
		{
			s_uiNextFrameTime += nAdjustTime; 
			printf("FrameSkip ���� %d\n",nAdjustTime);
			CTimer::Instance().Adjust(nAdjustTime);
		}

		s_bFrameSkip = true;
		bCurrentLateUpdate = TRUE;
	}

	//s_bFrameSkip = false;

	//if (dwCurrentTime > s_uiNextFrameTime)
	//{
	//	int dt = dwCurrentTime - s_uiNextFrameTime;

	//	//�ʹ� �ʾ��� ��� ������´�.
	//	//�׸��� m_dwCurUpdateTime�� delta�ε� delta�� absolute time�̶� ���ϸ� ��¼�ڴ°�?
	//	//if (dt >= 500 || m_dwCurUpdateTime > s_uiNextFrameTime)

	//	//�����ڵ��� �ϸ� 0.5�� ���� ���̳� ���·� update�� ���ӵǸ� ��� rendering frame skip�߻�
	//	if (dt >= 500 || m_dwCurUpdateTime > s_uiNextFrameTime)
	//	{
	//		s_uiNextFrameTime += dt / uiFrameTime * uiFrameTime; 
	//		printf("FrameSkip ���� %d\n", dt / uiFrameTime * uiFrameTime);
	//		CTimer::Instance().Adjust((dt / uiFrameTime) * uiFrameTime);
	//		s_bFrameSkip = true;
	//	}
	//}

	if (m_isFrameSkipDisable)
		s_bFrameSkip = false;

#ifdef __VTUNE__
	s_bFrameSkip = false;
#endif
	/*
	static bool s_isPrevFrameSkip=false;
	static DWORD s_dwFrameSkipCount=0;
	static DWORD s_dwFrameSkipEndTime=0;

	static DWORD ERROR_FRAME_SKIP_COUNT = 60*5;
	static DWORD ERROR_FRAME_SKIP_TIME = ERROR_FRAME_SKIP_COUNT*18;

	//static DWORD MAX_FRAME_SKIP=0;

	if (IsActive())
	{
	DWORD dwFrameSkipCurTime=ELTimer_GetMSec();

	if (s_bFrameSkip)
	{
	// ���� �����ӵ� ��ŵ�̶��..
	if (s_isPrevFrameSkip)
	{
	if (s_dwFrameSkipEndTime==0)
	{
	s_dwFrameSkipCount=0; // ������ üũ�� �ε� ���
	s_dwFrameSkipEndTime=dwFrameSkipCurTime+ERROR_FRAME_SKIP_TIME; // �ð� üũ�� �ε��� ������ ��ŵ üũ

	//printf("FrameSkipCheck Start\n");
	}
	++s_dwFrameSkipCount;

	//if (MAX_FRAME_SKIP<s_dwFrameSkipCount)
	//	MAX_FRAME_SKIP=s_dwFrameSkipCount;

	//printf("u %d c %d/%d t %d\n", 
	//	dwUpdateTime9-dwUpdateTime1,
	//	s_dwFrameSkipCount, 
	//	MAX_FRAME_SKIP,
	//	s_dwFrameSkipEndTime);

	//#ifndef _DEBUG
	// ���� �ð����� ��� ������ ��ŵ�� �Ѵٸ�...
	if (s_dwFrameSkipCount>ERROR_FRAME_SKIP_COUNT && s_dwFrameSkipEndTime<dwFrameSkipCurTime)
	{
	s_isPrevFrameSkip=false;
	s_dwFrameSkipEndTime=0;
	s_dwFrameSkipCount=0;

	//m_pyNetworkStream.AbsoluteExitGame();

	/*
	TraceError("���� ������ ��ŵ���� ������ �����մϴ�");

	{
	FILE* fp=fopen("errorlog.txt", "w");
	if (fp)
	{
	fprintf(fp, "FRAMESKIP\n");
	fprintf(fp, "Total %d\n",		dwUpdateTime9-dwUpdateTime1);
	fprintf(fp, "Timer %d\n",		dwUpdateTime2-dwUpdateTime1);
	fprintf(fp, "Network %d\n",		dwUpdateTime3-dwUpdateTime2);
	fprintf(fp, "Keyboard %d\n",	dwUpdateTime4-dwUpdateTime3);
	fprintf(fp, "Controll %d\n",	dwUpdateTime5-dwUpdateTime4);
	fprintf(fp, "Resource %d\n",	dwUpdateTime6-dwUpdateTime5);
	fprintf(fp, "Camera %d\n",		dwUpdateTime7-dwUpdateTime6);
	fprintf(fp, "Mouse %d\n",		dwUpdateTime8-dwUpdateTime7);
	fprintf(fp, "UI %d\n",			dwUpdateTime9-dwUpdateTime8);
	fclose(fp);

	WinExec("errorlog.exe", SW_SHOW);
	}
	}
	}
	}

	s_isPrevFrameSkip=true;
	}
	else
	{
	s_isPrevFrameSkip=false;
	s_dwFrameSkipCount=0;
	s_dwFrameSkipEndTime=0;
	}
	}
	else
	{
	s_isPrevFrameSkip=false;
	s_dwFrameSkipCount=0;
	s_dwFrameSkipEndTime=0;
	}
	*/
	if (!s_bFrameSkip)
	{
		//		static double pos=0.0f;
		//		CGrannyMaterial::TranslateSpecularMatrix(fabs(sin(pos)*0.005), fabs(cos(pos)*0.005), 0.0f);
		//		pos+=0.01f;

		CGrannyMaterial::TranslateSpecularMatrix(g_specularSpd, g_specularSpd, 0.0f);

		DWORD dwRenderStartTime = ELTimer_GetMSec();		

		bool canRender = true;

		if (m_isMinimizedWnd)
		{
			canRender = false;
		}
		else
		{
			if (m_pyGraphic.IsLostDevice())
			{
				CPythonBackground& rkBG = CPythonBackground::Instance();
				rkBG.ReleaseCharacterShadowTexture();
				m_kRenderTargetManager.ReleaseRenderTargetTextures();

				if (m_pyGraphic.RestoreDevice())					
				{
					rkBG.CreateCharacterShadowTexture();
					m_kRenderTargetManager.CreateRenderTargetTextures();
				}
				else
					canRender = false;				
			}
		}

		if (!IsActive())
		{
			SkipRenderBuffering(3000);
		}

		// ������� ó������ ����� ���� �ð������� ���۸��� ���� �ʴ´�
		if (!canRender)
		{
			SkipRenderBuffering(3000);
		}
		else
		{
			// RestoreLostDevice
			CCullingManager::Instance().Update();
			if (m_pyGraphic.Begin())
			{

				m_pyGraphic.ClearDepthBuffer();

#ifdef _DEBUG
				m_pyGraphic.SetClearColor(0.3f, 0.3f, 0.3f);
				m_pyGraphic.Clear();
#endif

				/////////////////////
				// Interface
				m_pyGraphic.SetInterfaceRenderState();

				OnUIRender();
				OnMouseRender();
				/////////////////////

				m_pyGraphic.End();

				//DWORD t1 = ELTimer_GetMSec();
				m_pyGraphic.Show();
				//DWORD t2 = ELTimer_GetMSec();

				DWORD dwRenderEndTime = ELTimer_GetMSec();

				static DWORD s_dwRenderCheckTime = dwRenderEndTime;
				static DWORD s_dwRenderRangeTime = 0;
				static DWORD s_dwRenderRangeFrame = 0;

				m_dwCurRenderTime = dwRenderEndTime - dwRenderStartTime;			
				s_dwRenderRangeTime += m_dwCurRenderTime;				
				++s_dwRenderRangeFrame;			

				if (dwRenderEndTime-s_dwRenderCheckTime>1000)
				{
					m_fAveRenderTime=float(double(s_dwRenderRangeTime)/double(s_dwRenderRangeFrame));

					s_dwRenderCheckTime=ELTimer_GetMSec();
					s_dwRenderRangeTime=0;
					s_dwRenderRangeFrame=0;
				}										

				DWORD dwCurFaceCount=m_pyGraphic.GetFaceCount();
				m_pyGraphic.ResetFaceCount();
				s_dwFaceCount += dwCurFaceCount;

				if (dwCurFaceCount > 5000)
				{
					// ������ ���� ó��
					if (dwRenderEndTime > m_dwBufSleepSkipTime)
					{	
						static float s_fBufRenderTime = 0.0f;

						float fCurRenderTime = m_dwCurRenderTime;

						if (fCurRenderTime > s_fBufRenderTime)
						{
							float fRatio = fMAX(0.5f, (fCurRenderTime - s_fBufRenderTime) / 30.0f);
							s_fBufRenderTime = (s_fBufRenderTime * (100.0f - fRatio) + (fCurRenderTime + 5) * fRatio) / 100.0f;
						}
						else
						{
							float fRatio = 0.5f;
							s_fBufRenderTime = (s_fBufRenderTime * (100.0f - fRatio) + fCurRenderTime * fRatio) / 100.0f;
						}

						// �Ѱ�ġ�� ���Ѵ�
						if (s_fBufRenderTime > 100.0f)
							s_fBufRenderTime = 100.0f;

						DWORD dwBufRenderTime = s_fBufRenderTime;

						if (m_isWindowed)
						{						
							if (dwBufRenderTime>58)
								dwBufRenderTime=64;
							else if (dwBufRenderTime>42)
								dwBufRenderTime=48;
							else if (dwBufRenderTime>26)
								dwBufRenderTime=32;
							else if (dwBufRenderTime>10)
								dwBufRenderTime=16;
							else
								dwBufRenderTime=8;
						}

						// ���� ������ �ӵ��� ���߾��ִ��ʿ� ���� ���ϴ�
						// �Ʒ����� �ѹ� �ϸ� ��?
						//if (m_dwCurRenderTime<dwBufRenderTime)
						//	Sleep(dwBufRenderTime-m_dwCurRenderTime);			

						m_fAveRenderTime=s_fBufRenderTime;
					}

					m_dwFaceAccCount += dwCurFaceCount;
					m_dwFaceAccTime += m_dwCurRenderTime;

					m_fFaceSpd=(m_dwFaceAccCount/m_dwFaceAccTime);

					// �Ÿ� �ڵ� ����
					if (-1 == m_iForceSightRange)
					{
						static float s_fAveRenderTime = 16.0f;
						float fRatio=0.3f;
						s_fAveRenderTime=(s_fAveRenderTime*(100.0f-fRatio)+MAX(16.0f, m_dwCurRenderTime)*fRatio)/100.0f;


						float fFar=25600.0f;
						float fNear=MIN_FOG;
						double dbAvePow=double(1000.0f/s_fAveRenderTime);
						double dbMaxPow=60.0;
						float fDistance=MAX(fNear+(fFar-fNear)*(dbAvePow)/dbMaxPow, fNear);
						m_pyBackground.SetViewDistanceSet(0, fDistance);
					}
					// �Ÿ� ���� ������
					else
					{
						m_pyBackground.SetViewDistanceSet(0, float(m_iForceSightRange));
					}
				}
				else
				{
					// 10000 ������ ���� �������� ���� �ָ� ���̰� �Ѵ�
					m_pyBackground.SetViewDistanceSet(0, 25600.0f);
				}

				++s_dwRenderFrameCount;
			}
		}
	}

	int rest = s_uiNextFrameTime - ELTimer_GetMSec();

	if (rest > 0 && !bCurrentLateUpdate )
	{
		s_uiLoad -= rest;	// �� �ð��� �ε忡�� ����..
		Sleep(rest);
	}	

	++s_dwUpdateFrameCount;

	s_uiLoad += ELTimer_GetMSec() - dwStart;
	return true;
}

void CPythonApplication::UpdateClientRect()
{
	RECT rcApp;
	GetClientRect(&rcApp);
	OnSizeChange(rcApp.right - rcApp.left, rcApp.bottom - rcApp.top);
}

void CPythonApplication::SetMouseHandler(PyObject* poMouseHandler)
{	
	m_poMouseHandler = poMouseHandler;
}

int CPythonApplication::CheckDeviceState()
{
	CGraphicDevice::EDeviceState e_deviceState = m_grpDevice.GetDeviceState();

	switch (e_deviceState)
	{
		// ����̽��� ������ ���α׷��� ���� �Ǿ�� �Ѵ�.
	case CGraphicDevice::DEVICESTATE_NULL:
		return DEVICE_STATE_FALSE;

		// DEVICESTATE_BROKEN�� ���� ���� �������� ���� �� �� �ֵ��� ���� �Ѵ�.
		// �׳� ������ ��� DrawPrimitive ���� ���� �ϸ� ���α׷��� ������.
	case CGraphicDevice::DEVICESTATE_BROKEN:
		return DEVICE_STATE_SKIP;

	case CGraphicDevice::DEVICESTATE_NEEDS_RESET:
		if (!m_grpDevice.Reset())
			return DEVICE_STATE_SKIP;

		break;
	}

	return DEVICE_STATE_OK;
}

bool CPythonApplication::CreateDevice(int width, int height, int Windowed, int bit /* = 32*/, int frequency /* = 0*/)
{
	int iRet;

	m_grpDevice.InitBackBufferCount(2);
	m_grpDevice.RegisterWarningString(CGraphicDevice::CREATE_BAD_DRIVER, ApplicationStringTable_GetStringz(IDS_WARN_BAD_DRIVER, "WARN_BAD_DRIVER"));
	m_grpDevice.RegisterWarningString(CGraphicDevice::CREATE_NO_TNL, ApplicationStringTable_GetStringz(IDS_WARN_NO_TNL, "WARN_NO_TNL"));

	iRet = m_grpDevice.Create(GetWindowHandle(), width, height, Windowed ? true : false, bit,frequency);

	switch (iRet)
	{
	case CGraphicDevice::CREATE_OK:
		return true;

	case CGraphicDevice::CREATE_REFRESHRATE:
		return true;

	case CGraphicDevice::CREATE_ENUM:
	case CGraphicDevice::CREATE_DETECT:
		SET_EXCEPTION(CREATE_NO_APPROPRIATE_DEVICE);
		TraceError("CreateDevice: Enum & Detect failed");
		return false;

	case CGraphicDevice::CREATE_NO_DIRECTX:
		//PyErr_SetString(PyExc_RuntimeError, "DirectX 8.1 or greater required to run game");
		SET_EXCEPTION(CREATE_NO_DIRECTX);
		TraceError("CreateDevice: DirectX 8.1 or greater required to run game");
		return false;

	case CGraphicDevice::CREATE_DEVICE:
		//PyErr_SetString(PyExc_RuntimeError, "GraphicDevice create failed");
		SET_EXCEPTION(CREATE_DEVICE);
		TraceError("CreateDevice: GraphicDevice create failed");
		return false;

	case CGraphicDevice::CREATE_FORMAT:
		SET_EXCEPTION(CREATE_FORMAT);
		TraceError("CreateDevice: Change the screen format");
		return false;

		/*case CGraphicDevice::CREATE_GET_ADAPTER_DISPLAY_MODE:
		//PyErr_SetString(PyExc_RuntimeError, "GetAdapterDisplayMode failed");
		SET_EXCEPTION(CREATE_GET_ADAPTER_DISPLAY_MODE);
		TraceError("CreateDevice: GetAdapterDisplayMode failed");
		return false;*/

	case CGraphicDevice::CREATE_GET_DEVICE_CAPS:
		PyErr_SetString(PyExc_RuntimeError, "GetDevCaps failed");
		TraceError("CreateDevice: GetDevCaps failed");
		return false;

	case CGraphicDevice::CREATE_GET_DEVICE_CAPS2:
		PyErr_SetString(PyExc_RuntimeError, "GetDevCaps2 failed");
		TraceError("CreateDevice: GetDevCaps2 failed");
		return false;

	default:
		if (iRet & CGraphicDevice::CREATE_OK)
		{
			//if (iRet & CGraphicDevice::CREATE_BAD_DRIVER)
			//{
			//	LogBox(ApplicationStringTable_GetStringz(IDS_WARN_BAD_DRIVER), NULL, GetWindowHandle());
			//}
			if (iRet & CGraphicDevice::CREATE_NO_TNL)
			{
				CGrannyLODController::SetMinLODMode(true);
				//LogBox(ApplicationStringTable_GetStringz(IDS_WARN_NO_TNL), NULL, GetWindowHandle());
			}
			return true;
		}

		//PyErr_SetString(PyExc_RuntimeError, "Unknown Error!");
		SET_EXCEPTION(UNKNOWN_ERROR);
		TraceError("CreateDevice: Unknown Error!");
		return false;
	}
}

void CPythonApplication::Loop()
{	
	while (1)
	{	
		if (IsMessage())
		{
			if (!MessageProcess())
				break;
		}
		else
		{
			if (!Process())
				break;

			m_dwLastIdleTime=ELTimer_GetMSec();
		}
	}
}

// SUPPORT_NEW_KOREA_SERVER
bool LoadLocaleData()
{
	NANOBEGIN

	const char* localePath = CLocaleManager::instance().GetLocaleBasePath().c_str();
	const char* localeLangPath = CLocaleManager::instance().GetLocalePath().c_str();

	CPythonNonPlayer&	rkNPCMgr	= CPythonNonPlayer::Instance();
	CItemManager&		rkItemMgr	= CItemManager::Instance();	
	CPythonSkill&		rkSkillMgr	= CPythonSkill::Instance();
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
#ifdef ENABLE_RUNE_SYSTEM
	CPythonRune&		rkRune = CPythonRune::Instance();
#endif
#ifdef ENABLE_PET_ADVANCED
	CPythonPetAdvanced& rkPetAdvanced = CPythonPetAdvanced::Instance();
#endif

	char szItemList[256];
	char szItemProto[256];
	char szItemDesc[256];
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	char szItemScale[256];
#endif
	char szMobProto[256];
	char szSkillDescFileName[256];
	char szSkillTableFileName[256];
	char szBlendFileName[256];
	char szInsultList[256];
#ifdef ENABLE_RUNE_SYSTEM
	char szRuneProtoFileName[256];
	char szRuneDescFileName[256];
#endif
#ifdef ENABLE_PET_ADVANCED
	char szPetSkillTable[256];
	char szPetEvolveTable[256];
	char szPetSkillDescFileName[256];
#endif
	snprintf(szItemList,	sizeof(szItemList) ,	"%s/item_list.txt",	localePath);


  	BYTE serverId = 1;
#ifdef ELONIA
  	serverId = 2;
#endif

	snprintf(szItemProto,	sizeof(szItemProto),	"%s/item_proto_s%d", localePath, serverId); // Server splitted
	snprintf(szMobProto,	sizeof(szMobProto),		"%s/mob_proto_s%d", localePath, serverId); // Server splitted

	snprintf(szItemDesc,	sizeof(szItemDesc),	"%s/itemdesc.txt",	localeLangPath);	
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	snprintf(szItemScale, sizeof(szItemScale), "%s/item_scale.txt", localePath);
#endif
	snprintf(szSkillDescFileName, sizeof(szSkillDescFileName),	"%s/SkillDesc_s%d.txt", localeLangPath, serverId); // Server splitted
	snprintf(szSkillTableFileName, sizeof(szSkillTableFileName),	"%s/SkillTable_s%d.txt", localePath, serverId); // Server splitted
	snprintf(szBlendFileName, sizeof(szBlendFileName),	"%s/blend.txt", localePath);
	snprintf(szInsultList,	sizeof(szInsultList),	"%s/insult.txt", localeLangPath);

#ifdef ENABLE_RUNE_SYSTEM
	snprintf(szRuneProtoFileName, sizeof(szRuneProtoFileName), "%s/rune.txt", localePath);
	snprintf(szRuneDescFileName, sizeof(szRuneDescFileName), "%s/rune_desc.txt", localeLangPath);
#endif
#ifdef ENABLE_PET_ADVANCED
	snprintf(szPetSkillTable, sizeof(szPetSkillTable), "%s/Pet_SkillTable.txt", localePath);
	snprintf(szPetEvolveTable, sizeof(szPetEvolveTable), "%s/Pet_EvolveTable.txt", localePath);
	snprintf(szPetSkillDescFileName, sizeof(szPetSkillDescFileName), "%s/Pet_SkillDesc.txt", localeLangPath);
#endif

	rkNPCMgr.Destroy();
	rkItemMgr.Destroy();	
	rkSkillMgr.Destroy();

#ifdef ENABLE_RUNE_SYSTEM
	rkRune.Destroy();
#endif

	if (!rkItemMgr.LoadItemList(szItemList))
	{
		TraceError("LoadLocaleData - LoadItemList(%s) Error", szItemList);
	}	

	if (!rkItemMgr.LoadItemTable(szItemProto))
	{
		TraceError("LoadLocaleData - LoadItemProto(%s) Error", szItemProto);
		return false;
	}

	if (!rkItemMgr.LoadItemDesc(szItemDesc))
	{
		Tracenf("LoadLocaleData - LoadItemDesc(%s) Error", szItemDesc);	
	}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (!rkItemMgr.LoadItemScale(szItemScale))
	{
		TraceError("LoadLocaleData - LoadItemScale(%s) Error", szItemScale);
		return false;
	}
#endif

	if (!rkNPCMgr.LoadNonPlayerData(szMobProto))
	{
		TraceError("LoadLocaleData - LoadMobProto(%s) Error", szMobProto);
		return false;
	}

	if (!rkSkillMgr.RegisterSkillDesc(szSkillDescFileName))
	{
		TraceError("LoadLocaleData - RegisterSkillDesc(%s) Error", szSkillDescFileName);
		return false;
	}

	if (!rkSkillMgr.RegisterSkillTable(szSkillTableFileName))
	{
		TraceError("LoadLocaleData - RegisterSkillTable(%s) Error", szSkillTableFileName);
		return false;
	}

	if (!rkItemMgr.LoadItemBlend(szBlendFileName))
	{
		TraceError("LoadLocaleData - LoadItemBlend(%s) Error", szBlendFileName);
		return false;
	}

#ifdef ENABLE_RUNE_SYSTEM
	if (!rkRune.LoadProto(szRuneProtoFileName))
	{
		TraceError("LoadLocaleData - Rune LoadProto(%s) Error", szRuneProtoFileName);
		return false;
	}

	if (!rkRune.LoadDesc(szRuneDescFileName))
	{
		TraceError("LoadLocaleData - Rune LoadDesc(%s) Error", szRuneDescFileName);
		return false;
	}
#endif

#ifdef ENABLE_PET_ADVANCED
	if (!rkPetAdvanced.LoadSkillProto(szPetSkillTable))
	{
		TraceError("LoadLocaleData - LoadPetSkillTable(%s) Error", szPetSkillTable);
		return false;
	}

	if (!rkPetAdvanced.LoadSkillDesc(szPetSkillDescFileName))
	{
		TraceError("LoadLocaleData - LoadPetSkillDesc(%s) Error", szPetSkillDescFileName);
		return false;
	}

	if (!rkPetAdvanced.LoadEvolveProto(szPetEvolveTable))
	{
		TraceError("LoadLocaleData - LoadPetEvolveProto(%s) Error", szPetEvolveTable);
		return false;
	}
#endif

	rkItemMgr.AppendSkillNameItems();
	
	NANOEND
		return true;
}
// END_OF_SUPPORT_NEW_KOREA_SERVER

unsigned __GetWindowMode(bool windowed)
{
	if (windowed)
		return WS_OVERLAPPED | WS_CAPTION |   WS_SYSMENU | WS_MINIMIZEBOX;

	return WS_POPUP;
}

bool CPythonApplication::MyShopDecoBGCreate()
{
	return m_pyMyShopDecoManager.CreateBackground(m_dwWidth, m_dwHeight);
}

bool CPythonApplication::Create(PyObject * poSelf, const char * c_szName, int width, int height, int Windowed)
{
	NANOBEGIN
		Windowed = CPythonSystem::Instance().IsWindowed() ? 1 : 0;

	bool bAnotherWindow = false;

	m_stWindowTitleName = c_szName;

	if (access("________dev.txt", 0) != -1 || access("dev.txt", 0) != -1)
	{
		/*
		m_stWindowTitleName += " [Game Version ";
		m_stWindowTitleName += g_szCurrentClientVersion_Display;
		m_stWindowTitleName += "]";
		*/
		m_stWindowTitleName += " TESTSERVER";
	}

	if (FindWindow(NULL, m_stWindowTitleName.c_str()))
		bAnotherWindow = true;

	m_dwWidth = width;
	m_dwHeight = height;

	// Window
	UINT WindowMode = __GetWindowMode(Windowed ? true : false);

	if (!CMSWindow::Create(m_stWindowTitleName.c_str(), 4, 0, WindowMode, ::LoadIcon(GetInstance(), MAKEINTRESOURCE(IDI_METIN2)), IDC_CURSOR_NORMAL))
	{
		//PyErr_SetString(PyExc_RuntimeError, "CMSWindow::Create failed");
		TraceError("CMSWindow::Create failed");
		SET_EXCEPTION(CREATE_WINDOW);
		return false;
	}

	if (m_pySystem.IsUseDefaultIME())
	{
		CPythonIME::Instance().UseDefaultIME();
	}

	// Ǯ��ũ�� ����̰�
	// ����Ʈ IME �� ����ϰų� ���� �����̸�
	// ������ Ǯ��ũ�� ��带 ����Ѵ�
	if (!m_pySystem.IsWindowed())
	{
		m_isWindowed = false;
		m_isWindowFullScreenEnable = TRUE;
		__SetFullScreenWindow(GetWindowHandle(), width, height, m_pySystem.GetBPP());

		Windowed = true;
	}
	else
	{
		AdjustSize(m_pySystem.GetWidth(), m_pySystem.GetHeight());

		if (Windowed)
		{
			m_isWindowed = true;

			if (bAnotherWindow)
			{
				RECT rc;

				GetClientRect(&rc);

				int windowWidth = rc.right - rc.left;
				int windowHeight = (rc.bottom - rc.top);

				CMSApplication::SetPosition(GetScreenWidth() - windowWidth, GetScreenHeight() - 60 - windowHeight);
			}
		}
		else
		{
			m_isWindowed = false;
			SetPosition(0, 0);
		}
	}

	NANOEND
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// Cursor
		if (!CreateCursors())
		{
			//PyErr_SetString(PyExc_RuntimeError, "CMSWindow::Cursors Create Error");
			TraceError("CMSWindow::Cursors Create Error");
			SET_EXCEPTION("CREATE_CURSOR");
			return false;
		}

		if (!m_pySystem.IsNoSoundCard())
		{
			// Sound
			if (!m_SoundManager.Create())
			{
				// NOTE : �߱����� ��û���� ����
				//		LogBox(ApplicationStringTable_GetStringz(IDS_WARN_NO_SOUND_DEVICE));
			}
		}

		extern bool GRAPHICS_CAPS_SOFTWARE_TILING;

		if (!m_pySystem.IsAutoTiling())
			GRAPHICS_CAPS_SOFTWARE_TILING = m_pySystem.IsSoftwareTiling();

		// Device
		if (!CreateDevice(m_pySystem.GetWidth(), m_pySystem.GetHeight(), Windowed, m_pySystem.GetBPP(), m_pySystem.GetFrequency()))
			return false;

		GrannyCreateSharedDeformBuffer();

		if (m_pySystem.IsAutoTiling())
		{
			if (m_grpDevice.IsFastTNL())
			{
				m_pyBackground.ReserveSoftwareTilingEnable(false);
			}
			else
			{
				m_pyBackground.ReserveSoftwareTilingEnable(true);
			}
		}
		else
		{
			m_pyBackground.ReserveSoftwareTilingEnable(m_pySystem.IsSoftwareTiling());
		}

		SetVisibleMode(true);

		if (m_isWindowFullScreenEnable) //m_pySystem.IsUseDefaultIME() && !m_pySystem.IsWindowed())
		{
			SetWindowPos(GetWindowHandle(), HWND_TOP, 0, 0, width, height, SWP_SHOWWINDOW);
		}

		if (!InitializeKeyboard(GetWindowHandle()))
			return false;

		m_pySystem.GetDisplaySettings();

		// Mouse
		if (m_pySystem.IsSoftwareCursor())
			SetCursorMode(CURSOR_MODE_SOFTWARE);
		else
			SetCursorMode(CURSOR_MODE_HARDWARE);

		// Network
		if (!m_netDevice.Create())
		{
			//PyErr_SetString(PyExc_RuntimeError, "NetDevice::Create failed");
			TraceError("NetDevice::Create failed");
			SET_EXCEPTION("CREATE_NETWORK");
			return false;
		}

		if (!m_grpDevice.IsFastTNL())
			CGrannyLODController::SetMinLODMode(true);

		m_pyItem.Create();

		// Other Modules
		DefaultFont_Startup();

		CPythonIME::Instance().Create(GetWindowHandle());
		CPythonIME::Instance().SetText("", 0);
		CPythonTextTail::Instance().Initialize();

		// Light Manager
		m_LightManager.Initialize();

		CGraphicImageInstance::CreateSystem(32);

		// ���
		STICKYKEYS sStickKeys;
		memset(&sStickKeys, 0, sizeof(sStickKeys));
		sStickKeys.cbSize = sizeof(sStickKeys);
		SystemParametersInfo( SPI_GETSTICKYKEYS, sizeof(sStickKeys), &sStickKeys, 0 );
		m_dwStickyKeysFlag = sStickKeys.dwFlags;

		// ����
		sStickKeys.dwFlags &= ~(SKF_AVAILABLE|SKF_HOTKEYACTIVE);
		SystemParametersInfo( SPI_SETSTICKYKEYS, sizeof(sStickKeys), &sStickKeys, 0 );

		// SphereMap
		CGrannyMaterial::CreateSphereMap(0, "d:/ymir work/special/spheremap.jpg");
		CGrannyMaterial::CreateSphereMap(1, "d:/ymir work/special/spheremap01.jpg");

#ifdef ENABLE_NEW_WEBBROWSER
		CPythonNewWeb::Instance().Initialize();
#endif

		return true;
}

void CPythonApplication::SetGlobalCenterPosition(long x, long y)
{
	CPythonBackground& rkBG=CPythonBackground::Instance();
	rkBG.GlobalPositionToLocalPosition(x, y);

	float z = CPythonBackground::Instance().GetHeight(x, y);

	CPythonApplication::Instance().SetCenterPosition(x, y, z);
}

void CPythonApplication::SetCenterPosition(float fx, float fy, float fz)
{
	m_v3CenterPosition.x = +fx;
	m_v3CenterPosition.y = -fy;
	m_v3CenterPosition.z = +fz;
}

void CPythonApplication::GetCenterPosition(TPixelPosition * pPixelPosition)
{
	pPixelPosition->x = +m_v3CenterPosition.x;
	pPixelPosition->y = -m_v3CenterPosition.y;
	pPixelPosition->z = +m_v3CenterPosition.z;
}


void CPythonApplication::SetServerTime(time_t tTime)
{
	m_dwStartLocalTime	= ELTimer_GetMSec();
	m_tServerTime		= tTime;
	m_tLocalStartTime	= time(0);
}

time_t CPythonApplication::GetServerTime()
{
	return (ELTimer_GetMSec() - m_dwStartLocalTime) + m_tServerTime;
}

// 2005.03.28 - MALL �����ۿ� ����ִ� �ð��� ������ �������� time(0) ���� ���������
//              ���̱� ������ ������ ���߱� ���� �ð� ���� ó���� ������ �߰�
time_t CPythonApplication::GetServerTimeStamp()
{
	return (time(0) - m_tLocalStartTime) + m_tServerTime;
}

float CPythonApplication::GetGlobalTime()
{
	return m_fGlobalTime;
}

float CPythonApplication::GetGlobalElapsedTime()
{
	return m_fGlobalElapsedTime;
}

void CPythonApplication::SetFPS(int iFPS)
{
	m_iFPS = iFPS;
}

int CPythonApplication::GetWidth()
{
	return m_dwWidth;
}

int CPythonApplication::GetHeight()
{
	return m_dwHeight;
}

void CPythonApplication::SetConnectData(const char * c_szIP, int iPort)
{
	m_strIP = c_szIP;
	m_iPort = iPort;
}

void CPythonApplication::GetConnectData(std::string & rstIP, int & riPort)
{
	rstIP	= m_strIP;
	riPort	= m_iPort;
}

void CPythonApplication::EnableSpecialCameraMode()
{
	m_isSpecialCameraMode = TRUE;
}

void CPythonApplication::SetCameraSpeed(int iPercentage)
{
	m_fCameraRotateSpeed = c_fDefaultCameraRotateSpeed * float(iPercentage) / 100.0f;
	m_fCameraPitchSpeed = c_fDefaultCameraPitchSpeed * float(iPercentage) / 100.0f;
	m_fCameraZoomSpeed = c_fDefaultCameraZoomSpeed * float(iPercentage) / 100.0f;
}

void CPythonApplication::SetForceSightRange(int iRange)
{
	m_iForceSightRange = iRange;
}

#ifdef ENABLE_MULTI_DESIGN
const std::string& CPythonApplication_GetSelectedDesignName()
{
	return CPythonApplication::Instance().GetSelectedDesignName();
}

const std::string& CPythonApplication_GetDefaultDesignName()
{
	static std::string s_stDefaultDesign = "de";
	return s_stDefaultDesign;
}
#endif

// GET HWND
struct ProcessWindowsInfo
{
	DWORD ProcessID;
	std::vector<HWND> Windows;

	ProcessWindowsInfo(DWORD const AProcessID)
		: ProcessID(AProcessID)
	{
	}
};

BOOL __stdcall EnumProcessWindowsProc(HWND hwnd, LPARAM lParam)
{
	ProcessWindowsInfo *Info = reinterpret_cast<ProcessWindowsInfo*>(lParam);
	DWORD WindowProcessID;

	GetWindowThreadProcessId(hwnd, &WindowProcessID);

	if (WindowProcessID == Info->ProcessID)
		Info->Windows.push_back(hwnd);

	return true;
}

const std::vector<HWND>& CPythonApplication::GetMainHWND()
{
	static ProcessWindowsInfo Info(GetProcessId(GetCurrentProcess()));

	EnumWindows((WNDENUMPROC)EnumProcessWindowsProc,
		reinterpret_cast<LPARAM>(&Info));

	return Info.Windows;
}
// GET HWND END

CGraphicImage* CPythonApplication::LoadDynamicImagePtr(const char* c_pszFileName)
{
	auto it = m_map_DynamicImageRef.find(c_pszFileName);
	if (it != m_map_DynamicImageRef.end())
		return it->second.GetPointer();

	CResource* pRes = CResourceManager::Instance().GetResourcePointer(c_pszFileName);
	if (!pRes->IsType(CGraphicImage::Type()))
		return NULL;

	CGraphicImage* pImgRes = (CGraphicImage*) pRes;
	m_map_DynamicImageRef[c_pszFileName] = pImgRes;

	return pImgRes;
}

void CPythonApplication::Clear()
{
	SetMouseHandler(NULL);
	m_pySystem.Clear();
}

void CPythonApplication::Destroy()
{
	WebBrowser_Destroy();

	// SphereMap
	CGrannyMaterial::DestroySphereMap();

	m_kWndMgr.Destroy();

	CPythonSystem::Instance().SaveConfig();

	DestroyCollisionInstanceSystem();

	m_kRenderTargetManager.Destroy();
	m_pyMyShopDecoManager.Destroy();

	m_pySystem.SaveInterfaceStatus();

	m_pyEventManager.Destroy();	
	m_FlyingManager.Destroy();

	m_pyMiniMap.Destroy();

	m_pyTextTail.Destroy();
	m_pyChat.Destroy();	
	m_kChrMgr.Destroy();
	m_RaceManager.Destroy();

	m_pyItem.Destroy();
	m_kItemMgr.Destroy();

	m_pyBackground.Destroy();

	m_kEftMgr.Destroy();
	m_LightManager.Destroy();

	// DEFAULT_FONT
	DefaultFont_Cleanup();
	// END_OF_DEFAULT_FONT

	GrannyDestroySharedDeformBuffer();

	m_pyGraphic.Destroy();
	//m_pyNetworkDatagram.Destroy();	

	m_pyRes.Destroy();

	m_kGuildMarkDownloader.Disconnect();

	CGrannyModelInstance::DestroySystem();
	CGraphicImageInstance::DestroySystem();


	m_SoundManager.Destroy();
	m_grpDevice.Destroy();

	// FIXME : ������� ���� ���� - [levites]
	//CSpeedTreeForestDirectX8::Instance().Clear();

	CAttributeInstance::DestroySystem();
	CTextFileLoader::DestroySystem();
	DestroyCursors();

	CMSApplication::Destroy();

	STICKYKEYS sStickKeys;
	memset(&sStickKeys, 0, sizeof(sStickKeys));
	sStickKeys.cbSize = sizeof(sStickKeys);
	sStickKeys.dwFlags = m_dwStickyKeysFlag;
	SystemParametersInfo( SPI_SETSTICKYKEYS, sizeof(sStickKeys), &sStickKeys, 0 );
}
