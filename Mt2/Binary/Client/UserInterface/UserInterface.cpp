#include "StdAfx.h"
#include "PythonApplication.h"
#include "PythonExceptionSender.h"
#include "resource.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include "../eterPack/EterPackManager.h"
#include "../eterLib/Util.h"
#include "../CWebBrowser/CWebBrowser.h"
#include "../eterBase/CPostIt.h"

//#include "PythonPackModule.cpp"
extern char pc_name[255], user_name[255], pc_hash[32*2+1];

extern "C" {  
extern int _fltused;  
volatile int _AVOID_FLOATING_POINT_LIBRARY_BUG = _fltused;  
};  
      
#pragma comment(linker, "/NODEFAULTLIB:libci.lib")
//  ssfsdde
#pragma comment( lib, "version.lib" )
#pragma comment( lib, "imagehlp.lib" )
#pragma comment( lib, "devil.lib" )
#pragma comment( lib, "granny2.lib" )
#pragma comment( lib, "mss32.lib" )
//#pragma comment( lib, "winmm.lib" ) ////////////////////////
#pragma comment( lib, "imm32.lib" )
#pragma comment( lib, "SpeedTreeRT.lib" )
#pragma comment( lib, "dinput8.lib" )
#pragma comment( lib, "dxguid.lib" )
#pragma comment( lib, "ws2_32.lib" )
#pragma comment( lib, "strmiids.lib" )
#pragma comment( lib, "ddraw.lib" )
#pragma comment( lib, "User32.lib" )
#pragma comment( lib, "Shell32.lib" )
#pragma comment( lib, "OleAut32.lib" )
#pragma comment( lib, "Ole32.lib" )
#pragma comment( lib, "Gdi32.lib" )
#pragma comment( lib, "ilu.lib" )

//#pragma comment( lib, "legacy_stdio_wide_specifiers_vs.lib" )
#pragma comment(lib, "legacy_stdio_definitions_vs.lib" )
#pragma comment(lib, "IPHLPAPI.lib")

//#if defined(USE_CCC) && !defined(AELDRA)
#pragma comment( lib, "lz4.lib" )
#pragma comment( lib, "xxhash.lib" )
#pragma comment( lib, "CCC.lib" )
//#endif

#pragma comment(lib, "libprotobuf.lib")

//#define USE_CUSTOM_ALLOCATOR  
#ifdef USE_CUSTOM_ALLOCATOR
#include "SmMalloc/smmalloc.h"
//#pragma comment( lib, "rpmalloc.lib")
#endif

#ifdef __USE_CYTHON__
#include "PythonrootlibManager.h"
#include "PythonuiscriptlibManager.h"
#endif

#include <stdlib.h>
#include <cryptopp/cryptoppLibLink.h>
bool __IS_TEST_SERVER_MODE__ = false;
bool test_server = false;
DWORD closeF = 0;

#include "VersionData.h"

#ifdef ELONIA
const char* g_szCurrentClientVersion = VERSION_ELONIA;
#else
const char* g_szCurrentClientVersion = VERSION_AELDRA;
#endif

/*
#ifdef LIVE
#include <CrashRpt.h>
crash_rpt::CrashRpt g_crashRpt(
#ifdef AELDRA
	"6df5a4b7-ffe6-4ccd-b265-ead0574b06e5",
	L"Aeldra",
	L"Aeldra"
#elif defined(ELONIA)
	"d8621025-0955-4a75-95f0-3cd00adf2d80",
	L"Elonia",
	L"Elonia"
#endif
);
#endif*/

extern bool SetDefaultCodePage(DWORD codePage);

static const char * sc_apszPythonLibraryFilenames[] =
{
	"UserDict.pyc",
	"__future__.pyc",
	"copy_reg.pyc",
	"linecache.pyc",
	"ntpath.pyc",
	"os.pyc",
	"site.pyc",
	"stat.pyc",
	"string.pyc",
	"traceback.pyc",
	"types.pyc",
	"\n",
};

char gs_szErrorString[512] = "";

void ApplicationSetErrorString(const char* szErrorString)
{
	strcpy(gs_szErrorString, szErrorString);
}

bool CheckPythonLibraryFilenames()
{
	return true;
	for (int i = 0; *sc_apszPythonLibraryFilenames[i] != '\n'; ++i)
	{
		std::string stFilename = "lib\\";
		stFilename += sc_apszPythonLibraryFilenames[i];

		if (_access(stFilename.c_str(), 0) != 0)
		{
			return false;
		}

		MoveFile(stFilename.c_str(), stFilename.c_str());
	}

	return true;
}

struct ApplicationStringTable 
{
	HINSTANCE m_hInstance;
	std::map<DWORD, std::string> m_kMap_dwID_stLocale;
} gs_kAppStrTable;

void ApplicationStringTable_Initialize(HINSTANCE hInstance)
{
	gs_kAppStrTable.m_hInstance=hInstance;
}

const std::string& ApplicationStringTable_GetString(DWORD dwID, LPCSTR szKey)
{
	char szBuffer[512];
	char szIniFileName[256];
	char szLocale[256];

	::GetCurrentDirectory(sizeof(szIniFileName), szIniFileName);
	if(szIniFileName[lstrlen(szIniFileName)-1] != '\\')
		strcat(szIniFileName, "\\");
	strcat(szIniFileName, "metin2client.dat");

	strcpy(szLocale, CLocaleManager::instance().GetLanguageShortName());
	::GetPrivateProfileString(szLocale, szKey, NULL, szBuffer, sizeof(szBuffer)-1, szIniFileName);
	if(szBuffer[0] == '\0')
		LoadString(gs_kAppStrTable.m_hInstance, dwID, szBuffer, sizeof(szBuffer)-1);
	if(szBuffer[0] == '\0')
		::GetPrivateProfileString("en", szKey, NULL, szBuffer, sizeof(szBuffer)-1, szIniFileName);
	if(szBuffer[0] == '\0')
		strcpy(szBuffer, szKey);

	std::string& rstLocale=gs_kAppStrTable.m_kMap_dwID_stLocale[dwID];
	rstLocale=szBuffer;

	return rstLocale;
}

const std::string& ApplicationStringTable_GetString(DWORD dwID)
{
	char szBuffer[512];

	LoadString(gs_kAppStrTable.m_hInstance, dwID, szBuffer, sizeof(szBuffer)-1);
	std::string& rstLocale=gs_kAppStrTable.m_kMap_dwID_stLocale[dwID];
	rstLocale=szBuffer;

	return rstLocale;
}

const char* ApplicationStringTable_GetStringz(DWORD dwID, LPCSTR szKey)
{
	return ApplicationStringTable_GetString(dwID, szKey).c_str();
}

const char* ApplicationStringTable_GetStringz(DWORD dwID)
{
	return ApplicationStringTable_GetString(dwID).c_str();
}

////////////////////////////////////////////

int Setup(LPSTR lpCmdLine); // Internal function forward

bool PackInitialize(const char * c_pszFolder)
{
#if defined(USE_FOX_FS) || defined(USE_ZFS) || defined(USE_CCC)
	NANOBEGIN
		if (_access(c_pszFolder, 0) != 0)
			return true;

	std::string stFolder(c_pszFolder);
	stFolder += "/";

	CTextFileLoader::SetCacheMode();
#if defined(USE_RELATIVE_PATH)
	CEterPackManager::Instance().SetRelativePathMode();
#endif
	CEterPackManager::Instance().SetCacheMode();
	CEterPackManager::Instance().SetSearchMode(CEterPackManager::SEARCH_PACK);
	CSoundData::SetPackMode();

#ifdef AELDRA
	if (access("pack/ae_patch", 0) != -1)
		CEterPackManager::Instance().RegisterPack("pack/ae_patch", "*");
	if (access("pack/ae_yw_patch", 0) != -1)
		CEterPackManager::Instance().RegisterPack("pack/ae_yw_patch", "d:/ymir work/");
#endif
#ifdef ELONIA
	if (access("pack/el_patch", 0) != -1)
		CEterPackManager::Instance().RegisterPack("pack/el_patch", "*");
	if (access("pack/el_yw_patch", 0) != -1)
		CEterPackManager::Instance().RegisterPack("pack/el_yw_patch", "d:/ymir work/");
#endif
	
	// defaults
	CEterPackManager::Instance().RegisterPack("pack/bgm", "bgm/");
	CEterPackManager::Instance().RegisterPack("pack/effect", "d:/ymir work/effect/");
	CEterPackManager::Instance().RegisterPack("pack/etc", "*");
	CEterPackManager::Instance().RegisterPack("pack/guild", "d:/ymir work/guild/");
	CEterPackManager::Instance().RegisterPack("pack/icon", "icon/");
	CEterPackManager::Instance().RegisterPack("pack/indoor", "indoor/");
	CEterPackManager::Instance().RegisterPack("pack/item", "d:/ymir work/item/");
	CEterPackManager::Instance().RegisterPack("pack/locale", "locale/");
	CEterPackManager::Instance().RegisterPack("pack/monster", "d:/ymir work/monster/");
	CEterPackManager::Instance().RegisterPack("pack/monster2", "d:/ymir work/monster2/");
	CEterPackManager::Instance().RegisterPack("pack/season1", "season1/");
	CEterPackManager::Instance().RegisterPack("pack/season2", "season2/");
	CEterPackManager::Instance().RegisterPack("pack/npc", "d:/ymir work/npc/");
	CEterPackManager::Instance().RegisterPack("pack/npc2", "d:/ymir work/npc2/");
	CEterPackManager::Instance().RegisterPack("pack/npc_pet", "d:/ymir work/npc_pet/");
	CEterPackManager::Instance().RegisterPack("pack/npc_mount", "d:/ymir work/npc_mount/");
	CEterPackManager::Instance().RegisterPack("pack/outdoor", "outdoor/");
	CEterPackManager::Instance().RegisterPack("pack/pc", "d:/ymir work/pc/");
	CEterPackManager::Instance().RegisterPack("pack/pc2", "d:/ymir work/pc2/");
	CEterPackManager::Instance().RegisterPack("pack/property", "property");
	CEterPackManager::Instance().RegisterPack("pack/sound", "sound/");
	CEterPackManager::Instance().RegisterPack("pack/terrain", "d:/ymir work/terrainmaps/");
	CEterPackManager::Instance().RegisterPack("pack/textureset", "textureset/");
	CEterPackManager::Instance().RegisterPack("pack/tree", "d:/ymir work/");
	CEterPackManager::Instance().RegisterPack("pack/zone", "d:/ymir work/zone/");
	CEterPackManager::Instance().RegisterPack("pack/uiloading", "d:/ymir work/uiloading");

#ifdef __USE_CYTHON__
	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root")).c_str());
#else
	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root_dev")).c_str());
#endif

	NANOEND
		return true;
#else
	NANOBEGIN
	if (_access(c_pszFolder, 0) != 0)
		return true;

	std::string stFolder(c_pszFolder);
	stFolder += "/";

	std::string stFileName(stFolder);
	stFileName += "Index";

	CMappedFile file;
	LPCVOID pvData;

	if (!file.Create(stFileName.c_str(), &pvData, 0, 0))
	{
		LogBoxf("FATAL ERROR! File not exist: %s", stFileName.c_str());
		TraceError("FATAL ERROR! File not exist: %s", stFileName.c_str());
		return true;
	}

	CMemoryTextFileLoader TextLoader;
	TextLoader.Bind(file.Size(), pvData);

	bool bPackFirst = TRUE;

	const std::string& strPackType = TextLoader.GetLineString(0);

	if (strPackType.compare("FILE") && strPackType.compare("PACK"))
	{
		TraceError("Pack/Index has invalid syntax. First line must be 'PACK' or 'FILE'");
		return false;
	}

#ifndef _DISTRIBUTE
	bPackFirst = FALSE;
#endif

	CTextFileLoader::SetCacheMode();
#if defined(USE_RELATIVE_PATH)
	CEterPackManager::Instance().SetRelativePathMode();
#endif
	CEterPackManager::Instance().SetCacheMode();
	CEterPackManager::Instance().SetSearchMode(bPackFirst);

	CSoundData::SetPackMode(); // Miles 파일 콜백을 셋팅

	std::string strPackName, strTexCachePackName;
	for (DWORD i = 1; i < TextLoader.GetLineCount() - 1; i += 2)
	{
		const std::string & c_rstFolder = TextLoader.GetLineString(i);
		const std::string & c_rstName = TextLoader.GetLineString(i + 1);

		strPackName = stFolder + c_rstName;
		strTexCachePackName = strPackName + "_texcache";

		CEterPackManager::Instance().RegisterPack(strPackName.c_str(), c_rstFolder.c_str());
		CEterPackManager::Instance().RegisterPack(strTexCachePackName.c_str(), c_rstFolder.c_str());
	}

	CEterPackManager::Instance().RegisterRootPack((stFolder + std::string("root")).c_str());
	NANOEND
	return true;
#endif
}

bool RunMainScript(CPythonLauncher& pyLauncher, const char* lpCmdLine)
{
	initpack();
	initdbg();
	initime();
	initgrp();
	initgrpImage();
	initgrpText();
	initwndMgr();
	/////////////////////////////////////////////
	initapp();
	initsystem();
	initchr();
	initchrmgr();
	initPlayer();
	initItem();
#ifdef ENABLE_PET_ADVANCED
	initItemPtr();
#endif
	initNonPlayer();
	initTrade();
	initChat();
	initTextTail();
	initnet();
	initMiniMap();
	initEvent();
	initeffect();
	initfly();
	initsnd();
	initeventmgr();
	initshop();
	initskill();
	initquest();
	initBackground();
	initMessenger();
	initsafebox();
	initguild();
#ifdef ENABLE_PYTHON_CONFIG
	initcfg();
#endif
#ifdef ENABLE_ACCOUNT_MANAGER
	initAccountManager();
#endif
#ifdef ENABLE_AUCTION
	initAuction();
#endif
#ifdef COMBAT_ZONE
	initCombatZoneSystem();
#endif
#ifdef ENABLE_RUNE_SYSTEM
	initRune();
#endif
#ifdef __USE_CYTHON__
	inituiscriptlibManager();
	initrootlibManager();
#endif

#ifdef INGAME_WIKI
	initWiki();
#endif
#ifdef ENABLE_PET_ADVANCED
	initPetAdvanced();
#endif
	
	initwhispermgr();

	NANOBEGIN	

	PyObject * builtins = PyImport_ImportModule("__builtin__");
#ifdef _DISTRIBUTE
	PyModule_AddIntConstant(builtins, "__DEBUG__", 0);
#else
	PyModule_AddIntConstant(builtins, "__DEBUG__", 1);
#endif

#ifdef __USE_CYTHON__
	PyModule_AddIntConstant(builtins, "__USE_CYTHON__", 1);
#else
	PyModule_AddIntConstant(builtins, "__USE_CYTHON__", 0);
#endif

		// RegisterCommandLine
	{
		std::string stRegisterCmdLine;

		const char * loginMark = "-cs";
		const char * loginMark_NonEncode = "-ncs";
		const char * seperator = " ";

		std::string stCmdLine;
		const int CmdSize = 3;
		vector<std::string> stVec;
		SplitLine(lpCmdLine, seperator, &stVec);
		if (CmdSize == stVec.size() && stVec[0] == loginMark)
		{
			char buf[MAX_PATH];	//TODO 아래 함수 string 형태로 수정
			base64_decode(stVec[2].c_str(), buf);
			stVec[2] = buf;
			string_join(seperator, stVec, &stCmdLine);
		}
		else if (CmdSize <= stVec.size() && stVec[0] == loginMark_NonEncode)
		{
			stVec[0] = loginMark;
			string_join(" ", stVec, &stCmdLine);
		}
		else
			stCmdLine = lpCmdLine;

		PyModule_AddStringConstant(builtins, "__COMMAND_LINE__", stCmdLine.c_str());
	}

	{
		vector<std::string> stVec;
		SplitLine(lpCmdLine, " ", &stVec);

#ifdef __USE_CYTHON__
		if (!pyLauncher.RunLine("import rootlib\nrootlib.moduleImport('system')"))
#else
		if (!pyLauncher.RunFile("system.py"))
#endif
		{
			TraceError("RunMain Error");
			return false;
		}
	}

	NANOEND
	return true;
}

#ifdef ENABLE_NEW_ANTI_RE
// AntiDump/RE
// https://www.codeproject.com/KB/security/AntiReverseEngineering.aspx?fid=1529949&df=90&mpp=25&noise=3&sort=Position&view=Quick&fr=51#SizeOfImage
// Any unreasonably large value will work say for example 0x100000 or 100,000h
void ChangeSizeOfImage()
{
    __asm
    {
        mov eax, fs:[0x30] // PEB
        mov eax, [eax + 0x0c] // PEB_LDR_DATA
        mov eax, [eax + 0x0c] // InOrderModuleList
        mov dword ptr [eax + 0x20], 0x100000 // SizeOfImage
    }
}

// This function will erase the current images
// PE header from memory preventing a successful image
// if dumped
inline void ErasePEHeaderFromMemory()
{
    DWORD OldProtect = 0;
    
    // Get base address of module
    char *pBaseAddr = (char*)GetModuleHandle(NULL);

    // Change memory protection
    VirtualProtect(pBaseAddr, 4096, // Assume x86 page size
            PAGE_READWRITE, &OldProtect);

    // Erase the header
    ZeroMemory(pBaseAddr, 4096);
}

// https://github.com/nemesisqp/al-khaser/blob/master/DebuggerDetection.cpp
BOOL NtSetInformationThread_ThreadHideFromDebugger()
{
	 /* Calling NtSetInformationThread will attempt with ThreadInformationClass set to  x11 (ThreadHideFromDebugger)
	 to hide a thread from the debugger, Passing NULL for hThread will cause the function to hide the thread the
	 function is running in. Also, the function returns false on failure and true on success. When  the  function
	 is called, the thread will continue  to run but a debugger will no longer receive any events related  to  that  thread. */

	// Function Pointer Typedef for NtQueryInformationProcess
	typedef NTSTATUS (WINAPI *pNtSetInformationThread)(IN HANDLE, IN UINT, IN PVOID, IN ULONG);

	// ThreadHideFromDebugger
	const int ThreadHideFromDebugger =  0x11;

	// We have to import the function
	pNtSetInformationThread NtSetInformationThread = NULL;

	// Other Vars
	NTSTATUS Status;
	BOOL IsBeingDebug = FALSE;

	HMODULE hNtDll = LoadLibrary(TEXT("ntdll.dll"));
	if(hNtDll == NULL)
	{
		// Handle however.. chances of this failing
		// is essentially 0 however since
		// ntdll.dll is a vital system resource
	}
 
    NtSetInformationThread = (pNtSetInformationThread)GetProcAddress(hNtDll, "NtSetInformationThread");
	
	if(NtSetInformationThread == NULL)
	{
		// Handle however it fits your needs but as before,
		// if this is missing there are some SERIOUS issues with the OS
	}

	 // Time to finally make the call
	Status = NtSetInformationThread(GetCurrentThread(), ThreadHideFromDebugger, NULL, 0);
    
	if(Status)
		IsBeingDebug = TRUE;

return IsBeingDebug;
}
#endif

bool Main(HINSTANCE hInstance, LPSTR lpCmdLine)
{
#ifdef ENABLE_NEW_ANTI_RE
	ChangeSizeOfImage();
	// ErasePEHeaderFromMemory();
	// NtSetInformationThread_ThreadHideFromDebugger();
#endif

#ifdef LOCALE_SERVICE_YMIR
	extern bool g_isScreenShotKey;
	g_isScreenShotKey = true;
#endif

	DWORD dwRandSeed = time(NULL) + DWORD(GetCurrentProcess());
	srandom(dwRandSeed);
	srand(random());

	SetLogLevel(1);

#ifdef LEAK_DETECT
	SymInitialize(GetCurrentProcess(), 0, true);
	SymSetOptions(SYMOPT_LOAD_LINES);
#endif

#ifndef __VTUNE__
	ilInit();
#endif
	if (!Setup(lpCmdLine))
		return false;

#ifdef _DEBUG
	OpenConsoleWindow();
	OpenLogFile(true); // true == uses syserr.txt and log.txt
#else
#ifdef _USE_LOG_FILE
	OpenLogFile(true); // true == uses syserr.txt and log.txt
#else
	OpenLogFile(false); // false == uses syserr.txt only
#endif
#endif

	__IS_TEST_SERVER_MODE__ = access("________dev.txt", 0) != -1 || access("dev.txt", 0) != -1;
	test_server = __IS_TEST_SERVER_MODE__;

	static CLZO				lzo;
	static CEterPackManager	EterPackManager;

	if (!PackInitialize("pack"))
	{
		LogBox("Pack Initialization failed. Check log.txt file..");
		return false;
	}

#ifdef ENABLE_PYTHON_CONFIG
	static CPythonConfig m_pyConfig;
	m_pyConfig.Initialize("config.cfg");
	
	if (m_pyConfig.GetInteger(CPythonConfig::CLASS_GENERAL, "init", 0) == 0)
	{		
		SetFileAttributes("temp", FILE_ATTRIBUTE_HIDDEN);
		//SetFileAttributes("miles", FILE_ATTRIBUTE_HIDDEN);
		//SetFileAttributes("mark", FILE_ATTRIBUTE_HIDDEN);
		//SetFileAttributes("lib", FILE_ATTRIBUTE_HIDDEN);
		m_pyConfig.Write(CPythonConfig::CLASS_GENERAL, "init", 1);
	}
	
	// m_pyConfig.LoadBlockNameList("block.dat");
#endif
	
	static CLocaleManager kLocaleManager;
	kLocaleManager.Initialize();

	CPythonApplication * app = new CPythonApplication;

	app->Initialize(hInstance);

	bool ret=false;
	{
		CPythonLauncher pyLauncher;
		CPythonExceptionSender pyExceptionSender;
		SetExceptionSender(&pyExceptionSender);

		if (pyLauncher.Create())
		{
			ret=RunMainScript(pyLauncher, lpCmdLine);
		}

		app->Clear();

		timeEndPeriod(1);
		pyLauncher.Clear();
	}

	app->Destroy();
	delete app;
#ifdef ENABLE_PYTHON_CONFIG
	// m_pyConfig.SaveBlockNameList();
#endif
	
#ifdef LEAK_DETECT
	FILE* o = fopen("stack.log", "w");
	if (o)
	{
		StackTraceLog(o, 1);
		fflush(o);
		fclose(o);
	}
#endif

	return ret;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DWORD dwbuf1 = sizeof(pc_name);
	DWORD dwbuf2 = sizeof(user_name);
	GetComputerName(pc_name, &dwbuf1);
	GetUserName(user_name, &dwbuf2);

	std::string message = std::string(pc_name);
	message.erase(std::remove_if(message.begin(), message.end(), [](char c) { return !std::isalnum(c); }), message.end());
	strcpy(pc_name, message.c_str());
	
	message = std::string(user_name);
	message.erase(std::remove_if(message.begin(), message.end(), [](char c) { return !std::isalnum(c); }), message.end());
	strcpy(user_name, message.c_str());

	std::string md5str = std::string(pc_name) + " @ " + std::string(user_name);
	strcpy(pc_hash, CPythonSystem::Instance().GetMD5(std::string(md5str + 'a' + 'b' + 'c' + '5').c_str()));
#ifdef USE_CUSTOM_ALLOCATOR
	sm_allocator space = _sm_allocator_create(4, (16 * 1024 * 1024));

#define malloc_backup malloc
#define free_backup free
#undef malloc
#undef free
#define malloc _sm_malloc
#define free _sm_free
#endif

#ifdef _DEBUG
	_CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_CHECK_CRT_DF | _CRTDBG_LEAK_CHECK_DF );
	//_CrtSetBreakAlloc( 110247 ); 
#endif

	ApplicationStringTable_Initialize(hInstance);

	bool bQuit = false;
	int nArgc = 0;

	WebBrowser_Startup(hInstance);

	if (!CheckPythonLibraryFilenames())
	{
		LogBoxf("Please redownload the Client! (Python Files not found)");
		return 0;
	}

	Main(hInstance, lpCmdLine);

	WebBrowser_Cleanup();

	::CoUninitialize();

	if(gs_szErrorString[0])
		MessageBox(NULL, gs_szErrorString, ApplicationStringTable_GetStringz(IDS_APP_NAME, "APP_NAME"), MB_ICONSTOP);

#ifdef USE_CUSTOM_ALLOCATOR
	_sm_allocator_destroy(space);
#endif
	return 0;
}

int Setup(LPSTR lpCmdLine)
{
	TIMECAPS tc; 
	UINT wTimerRes; 

	if (timeGetDevCaps(&tc, sizeof(TIMECAPS)) != TIMERR_NOERROR) 
		return 0;

	wTimerRes = MINMAX(tc.wPeriodMin, 1, tc.wPeriodMax); 
	timeBeginPeriod(wTimerRes); 

	//GrannySetLogFileName("granny_log.txt", true);
	return 1;
}
