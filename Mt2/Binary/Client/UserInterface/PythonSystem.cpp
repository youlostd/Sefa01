#include "StdAfx.h"
#include "PythonSystem.h"
#include "PythonApplication.h"
#include <IPHlpApi.h>

#define DEFAULT_VALUE_ALWAYS_SHOW_NAME		true

void CPythonSystem::SetInterfaceHandler(PyObject * poHandler)
{
// NOTE : 레퍼런스 카운트는 바꾸지 않는다. 레퍼런스가 남아 있어 Python에서 완전히 지워지지 않기 때문.
//        대신에 __del__때 Destroy를 호출해 Handler를 NULL로 셋팅한다. - [levites]
//	if (m_poInterfaceHandler)
//		Py_DECREF(m_poInterfaceHandler);

	m_poInterfaceHandler = poHandler;

//	if (m_poInterfaceHandler)
//		Py_INCREF(m_poInterfaceHandler);
}

void CPythonSystem::DestroyInterfaceHandler()
{
	m_poInterfaceHandler = NULL;
}

void CPythonSystem::SaveWindowStatus(int iIndex, int iVisible, int iMinimized, int ix, int iy, int iHeight)
{
	m_WindowStatus[iIndex].isVisible	= iVisible;
	m_WindowStatus[iIndex].isMinimized	= iMinimized;
	m_WindowStatus[iIndex].ixPosition	= ix;
	m_WindowStatus[iIndex].iyPosition	= iy;
	m_WindowStatus[iIndex].iHeight		= iHeight;
}

void CPythonSystem::GetDisplaySettings()
{
	memset(m_ResolutionList, 0, sizeof(TResolution) * RESOLUTION_MAX_NUM);
	m_ResolutionCount = 0;

	LPDIRECT3D8 lpD3D = CPythonGraphic::Instance().GetD3D();

	D3DADAPTER_IDENTIFIER8 d3dAdapterIdentifier;
	D3DDISPLAYMODE d3ddmDesktop;

	lpD3D->GetAdapterIdentifier(0, D3DENUM_NO_WHQL_LEVEL, &d3dAdapterIdentifier);
	lpD3D->GetAdapterDisplayMode(0, &d3ddmDesktop);

	// 이 어뎁터가 가지고 있는 디스플래이 모드갯수를 나열한다..
	DWORD dwNumAdapterModes = lpD3D->GetAdapterModeCount(0);

	for (UINT iMode = 0; iMode < dwNumAdapterModes; iMode++)
	{
		D3DDISPLAYMODE DisplayMode;
		lpD3D->EnumAdapterModes(0, iMode, &DisplayMode);
		DWORD bpp = 0;

		// 800 600 이상만 걸러낸다.
		if (DisplayMode.Width < 800 || DisplayMode.Height < 600)
			continue;

		// 일단 16bbp 와 32bbp만 취급하자.
		// 16bbp만 처리하게끔 했음 - [levites]
		if (DisplayMode.Format == D3DFMT_R5G6B5)
			bpp = 16;
		else if (DisplayMode.Format == D3DFMT_X8R8G8B8)
			bpp = 32;
		else
			continue;

		int check_res = false;

		for (int i = 0; !check_res && i < m_ResolutionCount; ++i)
		{
			if (m_ResolutionList[i].bpp != bpp ||
				m_ResolutionList[i].width != DisplayMode.Width ||
				m_ResolutionList[i].height != DisplayMode.Height)
				continue;

			int check_fre = false;

			// 프리퀀시만 다르므로 프리퀀시만 셋팅해준다.
			for (int j = 0; j < m_ResolutionList[i].frequency_count; ++j)
			{
				if (m_ResolutionList[i].frequency[j] == DisplayMode.RefreshRate)
				{
					check_fre = true;
					break;
				}
			}

			if (!check_fre)
				if (m_ResolutionList[i].frequency_count < FREQUENCY_MAX_NUM)
					m_ResolutionList[i].frequency[m_ResolutionList[i].frequency_count++] = DisplayMode.RefreshRate;

			check_res = true;
		}

		if (!check_res)
		{
			// 새로운 거니까 추가해주자.
			if (m_ResolutionCount < RESOLUTION_MAX_NUM)
			{
				m_ResolutionList[m_ResolutionCount].width			= DisplayMode.Width;
				m_ResolutionList[m_ResolutionCount].height			= DisplayMode.Height;
				m_ResolutionList[m_ResolutionCount].bpp				= bpp;
				m_ResolutionList[m_ResolutionCount].frequency[0]	= DisplayMode.RefreshRate;
				m_ResolutionList[m_ResolutionCount].frequency_count	= 1;

				++m_ResolutionCount;
			}
		}
	}
}

int	CPythonSystem::GetResolutionCount()
{
	return m_ResolutionCount;
}

int CPythonSystem::GetFrequencyCount(int index)
{
	if (index >= m_ResolutionCount)
		return 0;

    return m_ResolutionList[index].frequency_count;
}

bool CPythonSystem::GetResolution(int index, OUT DWORD *width, OUT DWORD *height, OUT DWORD *bpp)
{
	if (index >= m_ResolutionCount)
		return false;

	*width = m_ResolutionList[index].width;
	*height = m_ResolutionList[index].height;
	*bpp = m_ResolutionList[index].bpp;
	return true;
}

bool CPythonSystem::GetFrequency(int index, int freq_index, OUT DWORD *frequncy)
{
	if (index >= m_ResolutionCount)
		return false;

	if (freq_index >= m_ResolutionList[index].frequency_count)
		return false;

	*frequncy = m_ResolutionList[index].frequency[freq_index];
	return true;
}

int	CPythonSystem::GetResolutionIndex(DWORD width, DWORD height, DWORD bit)
{
	DWORD re_width, re_height, re_bit;
	int i = 0;

	while (GetResolution(i, &re_width, &re_height, &re_bit))
	{
		if (re_width == width)
			if (re_height == height)
				if (re_bit == bit)
					return i;
		i++;
	}

	return 0;
}

int	CPythonSystem::GetFrequencyIndex(int res_index, DWORD frequency)
{
	DWORD re_frequency;
	int i = 0;

	while (GetFrequency(res_index, i, &re_frequency))
	{
		if (re_frequency == frequency)
			return i;

		i++;
	}

	return 0;
}

DWORD CPythonSystem::GetWidth()
{
	return m_Config.width;
}

DWORD CPythonSystem::GetHeight()
{
	return m_Config.height;
}
DWORD CPythonSystem::GetBPP()
{
	return m_Config.bpp;
}
DWORD CPythonSystem::GetFrequency()
{
	return m_Config.frequency;
}

bool CPythonSystem::IsNoSoundCard()
{
	return m_Config.bNoSoundCard;
}

bool CPythonSystem::IsSoftwareCursor()
{
	return m_Config.is_software_cursor;
}

float CPythonSystem::GetMusicVolume()
{
	return m_Config.music_volume;
}

int CPythonSystem::GetSoundVolume()
{
	return m_Config.voice_volume;
}

void CPythonSystem::SetMusicVolume(float fVolume)
{
	m_Config.music_volume = fVolume;
}

void CPythonSystem::SetSoundVolumef(float fVolume)
{
	m_Config.voice_volume = int(5 * fVolume);
}

int CPythonSystem::GetDistance()
{
	return m_Config.iDistance;
}

int CPythonSystem::GetShadowLevel()
{
	return m_Config.iShadowLevel;
}

void CPythonSystem::SetShadowLevel(unsigned int level)
{
	m_Config.iShadowLevel = MIN(level, 5);
	CPythonBackground::instance().RefreshShadowLevel();
}

int CPythonSystem::IsSaveID()
{
	return m_Config.isSaveID;
}

const char * CPythonSystem::GetSaveID()
{
	return m_Config.SaveID;
}

bool CPythonSystem::isViewCulling()
{
	return m_Config.is_object_culling;
}

void CPythonSystem::SetSaveID(int iValue, const char * c_szSaveID)
{
	if (iValue != 1)
		return;
	
	m_Config.isSaveID = iValue;
	strncpy(m_Config.SaveID, c_szSaveID, sizeof(m_Config.SaveID) - 1);
}

CPythonSystem::TConfig * CPythonSystem::GetConfig()
{
	return &m_Config;
}

void CPythonSystem::SetConfig(TConfig * pNewConfig)
{
	m_Config = *pNewConfig;
}

void CPythonSystem::SetDefaultConfig()
{
	memset(&m_Config, 0, sizeof(m_Config));

	m_Config.width				= 1024;
	m_Config.height				= 768;
	m_Config.bpp				= 32;

#if defined( LOCALE_SERVICE_WE_JAPAN )
	m_Config.bWindowed			= true;
#else
	m_Config.bWindowed			= false;
#endif

	m_Config.is_software_cursor	= false;
	m_Config.is_object_culling	= true;
	m_Config.iDistance			= 3;

	m_Config.gamma				= 3;
	m_Config.music_volume		= 1.0f;
	m_Config.voice_volume		= 5;

	m_Config.bDecompressDDS		= 0;
	m_Config.bSoftwareTiling	= 0;
	m_Config.iShadowLevel		= 3;
	m_Config.bViewChat			= true;
	m_Config.bAlwaysShowName	= DEFAULT_VALUE_ALWAYS_SHOW_NAME;
	m_Config.bShowDamage		= true;
	m_Config.bShowSalesText		= true;
#ifdef ENABLE_CHANGE_TEXTURE_SNOW
	m_Config.bSnowTexturesMode	= false;
#endif
}

bool CPythonSystem::IsWindowed()
{
	return m_Config.bWindowed;
}

bool CPythonSystem::IsViewChat()
{
	return m_Config.bViewChat;
}

void CPythonSystem::SetViewChatFlag(int iFlag)
{
	m_Config.bViewChat = iFlag == 1 ? true : false;
}

bool CPythonSystem::IsAlwaysShowName()
{
	return m_Config.bAlwaysShowName;
}

void CPythonSystem::SetAlwaysShowNameFlag(int iFlag)
{
	m_Config.bAlwaysShowName = iFlag == 1 ? true : false;
}

bool CPythonSystem::IsShowDamage()
{
	return m_Config.bShowDamage;
}

void CPythonSystem::SetShowDamageFlag(int iFlag)
{
	m_Config.bShowDamage = iFlag == 1 ? true : false;
}

bool CPythonSystem::IsShowSalesText()
{
	return m_Config.bShowSalesText;
}

#ifdef ENABLE_CHANGE_TEXTURE_SNOW
bool CPythonSystem::IsSnowTexturesMode()
{
	return m_Config.bSnowTexturesMode;
}
void CPythonSystem::SetSnowTexturesMode(int iFlag)
{
	m_Config.bSnowTexturesMode = iFlag == 1 ? true : false;
}
#endif

#ifdef HIDE_NPC_OPTION
void CPythonSystem::SetHideNPC(int category, bool show)
{
	m_Config.bHiddenNPC[category] = show;
}

bool CPythonSystem::IsHiddenNPC(int category)
{
	return m_Config.bHiddenNPC[category];
}
#endif

void CPythonSystem::SetShowSalesTextFlag(int iFlag)
{
	m_Config.bShowSalesText = iFlag == 1 ? true : false;
}

bool CPythonSystem::IsAutoTiling()
{
	if (m_Config.bSoftwareTiling == 0)
		return true;

	return false;
}

void CPythonSystem::SetSoftwareTiling(bool isEnable)
{
	if (isEnable)
		m_Config.bSoftwareTiling=1;
	else
		m_Config.bSoftwareTiling=2;
}

bool CPythonSystem::IsSoftwareTiling()
{
	if (m_Config.bSoftwareTiling==1)
		return true;

	return false;
}

bool CPythonSystem::IsUseDefaultIME()
{
	return m_Config.bUseDefaultIME;
}

bool CPythonSystem::LoadConfig()
{
	FILE * fp = NULL;

	if (NULL == (fp = fopen("metin2.cfg", "rt")))
		return false;

	char buf[256];
	char command[256];
	char value[256];

	while (fgets(buf, 256, fp))
	{
		if (sscanf(buf, " %s %s\n", command, value) == EOF)
			break;

		if (!stricmp(command, "WIDTH"))
			m_Config.width		= atoi(value);
		else if (!stricmp(command, "HEIGHT"))
			m_Config.height	= atoi(value);
		else if (!stricmp(command, "BPP"))
			m_Config.bpp		= atoi(value);
		else if (!stricmp(command, "FREQUENCY"))
			m_Config.frequency = atoi(value);
		else if (!stricmp(command, "SOFTWARE_CURSOR"))
			m_Config.is_software_cursor = atoi(value) ? true : false;
		else if (!stricmp(command, "OBJECT_CULLING"))
			m_Config.is_object_culling = atoi(value) ? true : false;
		else if (!stricmp(command, "VISIBILITY"))
			m_Config.iDistance = atoi(value);
		else if (!stricmp(command, "MUSIC_VOLUME")) {
			if(strchr(value, '.') == 0) { // Old compatiability
				m_Config.music_volume = pow(10.0f, (-1.0f + (((float) atoi(value)) / 5.0f)));
				if(atoi(value) == 0)
					m_Config.music_volume = 0.0f;
			} else
				m_Config.music_volume = atof(value);
		} else if (!stricmp(command, "VOICE_VOLUME"))
			m_Config.voice_volume = (char) atoi(value);
		else if (!stricmp(command, "GAMMA"))
			m_Config.gamma = atoi(value);
		else if (!stricmp(command, "IS_SAVE_ID"))
			m_Config.isSaveID = atoi(value);
		else if (!stricmp(command, "SAVE_ID"))
			strncpy(m_Config.SaveID, value, 20);
		else if (!stricmp(command, "PRE_LOADING_DELAY_TIME"))
			g_iLoadingDelayTime = atoi(value);
		else if (!stricmp(command, "WINDOWED"))
		{
			m_Config.bWindowed = atoi(value) == 1 ? true : false;
		}
		else if (!stricmp(command, "USE_DEFAULT_IME"))
			m_Config.bUseDefaultIME = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "SOFTWARE_TILING"))
			m_Config.bSoftwareTiling = atoi(value);
		else if (!stricmp(command, "SHADOW_LEVEL"))
			m_Config.iShadowLevel = atoi(value);
		else if (!stricmp(command, "DECOMPRESSED_TEXTURE"))
			m_Config.bDecompressDDS = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "NO_SOUND_CARD"))
			m_Config.bNoSoundCard = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "VIEW_CHAT"))
			m_Config.bViewChat = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "ALWAYS_VIEW_NAME"))
			m_Config.bAlwaysShowName = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "SHOW_DAMAGE"))
			m_Config.bShowDamage = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "SHOW_SALESTEXT"))
			m_Config.bShowSalesText = atoi(value) == 1 ? true : false;
#ifdef ENABLE_CHANGE_TEXTURE_SNOW
		else if (!stricmp(command, "SNOW_TEXTURE_MODE"))
			m_Config.bSnowTexturesMode = atoi(value) == 1 ? true : false;
#endif
#ifdef HIDE_NPC_OPTION
		else if (!stricmp(command, "HIDE_NPC_CATEGORY_0"))
			m_Config.bHiddenNPC[0] = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "HIDE_NPC_CATEGORY_1"))
			m_Config.bHiddenNPC[1] = atoi(value) == 1 ? true : false;
		else if (!stricmp(command, "HIDE_NPC_CATEGORY_2"))
			m_Config.bHiddenNPC[2] = atoi(value) == 1 ? true : false;
#endif
	}

	if (m_Config.bWindowed)
	{
		unsigned screen_width = GetSystemMetrics(SM_CXFULLSCREEN);
		unsigned screen_height = GetSystemMetrics(SM_CYFULLSCREEN);

		if (m_Config.width >= screen_width)
		{
			m_Config.width = screen_width;
		}
		if (m_Config.height >= screen_height)
		{
			m_Config.height = screen_height;
		}
	}
	
	m_OldConfig = m_Config;

	fclose(fp);

//	Tracef("LoadConfig: Resolution: %dx%d %dBPP %dHZ Software Cursor: %d, Music/Voice Volume: %d/%d Gamma: %d\n",
//		m_Config.width,
//		m_Config.height,
//		m_Config.bpp,
//		m_Config.frequency,
//		m_Config.is_software_cursor,
//		m_Config.music_volume,
//		m_Config.voice_volume,
//		m_Config.gamma);

	return true;
}

bool CPythonSystem::SaveConfig()
{
	FILE *fp;

	if (NULL == (fp = fopen("metin2.cfg", "wt")))
		return false;

	fprintf(fp, "WIDTH						%d\n"
				"HEIGHT						%d\n"
				"BPP						%d\n"
				"FREQUENCY					%d\n"
				"SOFTWARE_CURSOR			%d\n"
				"OBJECT_CULLING				%d\n"
				"VISIBILITY					%d\n"
				"MUSIC_VOLUME				%.3f\n"
				"VOICE_VOLUME				%d\n"
				"GAMMA						%d\n"
				"IS_SAVE_ID					%d\n"
				"SAVE_ID					%s\n"
				"PRE_LOADING_DELAY_TIME		%d\n"
				"DECOMPRESSED_TEXTURE		%d\n",
				m_Config.width,
				m_Config.height,
				m_Config.bpp,
				m_Config.frequency,
				m_Config.is_software_cursor,
				m_Config.is_object_culling,
				m_Config.iDistance,
				m_Config.music_volume,
				m_Config.voice_volume,
				m_Config.gamma,
				m_Config.isSaveID,
				m_Config.SaveID,
				g_iLoadingDelayTime,
				m_Config.bDecompressDDS);

	if (m_Config.bWindowed == 1)
		fprintf(fp, "WINDOWED				%d\n", m_Config.bWindowed);
	if (m_Config.bViewChat == 0)
		fprintf(fp, "VIEW_CHAT				%d\n", m_Config.bViewChat);
	if (m_Config.bAlwaysShowName != DEFAULT_VALUE_ALWAYS_SHOW_NAME)
		fprintf(fp, "ALWAYS_VIEW_NAME		%d\n", m_Config.bAlwaysShowName);
	if (m_Config.bShowDamage == 0)
		fprintf(fp, "SHOW_DAMAGE		%d\n", m_Config.bShowDamage);
	if (m_Config.bShowSalesText == 0)
		fprintf(fp, "SHOW_SALESTEXT		%d\n", m_Config.bShowSalesText);

	fprintf(fp, "USE_DEFAULT_IME		%d\n", m_Config.bUseDefaultIME);
	fprintf(fp, "SOFTWARE_TILING		%d\n", m_Config.bSoftwareTiling);
	fprintf(fp, "SHADOW_LEVEL			%d\n", m_Config.iShadowLevel);
#ifdef ENABLE_CHANGE_TEXTURE_SNOW
	fprintf(fp, "SNOW_TEXTURE_MODE		%d\n", m_Config.bSnowTexturesMode);
#endif
#ifdef HIDE_NPC_OPTION
	fprintf(fp, "HIDE_NPC_CATEGORY_0	%d\n", m_Config.bHiddenNPC[0]);
	fprintf(fp, "HIDE_NPC_CATEGORY_1	%d\n", m_Config.bHiddenNPC[1]);
	fprintf(fp, "HIDE_NPC_CATEGORY_2	%d\n", m_Config.bHiddenNPC[2]);
#endif
	fprintf(fp, "\n");

	fclose(fp);
	return true;
}

bool CPythonSystem::LoadInterfaceStatus()
{
	FILE * File;
	File = fopen("interface.cfg", "rb");

	if (!File)
		return false;

	fread(m_WindowStatus, 1, sizeof(TWindowStatus) * WINDOW_MAX_NUM, File);
	fclose(File);
	return true;
}

void CPythonSystem::SaveInterfaceStatus()
{
	if (!m_poInterfaceHandler)
		return;

	PyCallClassMemberFunc(m_poInterfaceHandler, "OnSaveInterfaceStatus", Py_BuildValue("()"));

	FILE * File;

	File = fopen("interface.cfg", "wb");

	if (!File)
	{
		TraceError("Cannot open interface.cfg");
		return;
	}

	fwrite(m_WindowStatus, 1, sizeof(TWindowStatus) * WINDOW_MAX_NUM, File);
	fclose(File);
}

bool CPythonSystem::isInterfaceConfig()
{
	return m_isInterfaceConfig;
}

const CPythonSystem::TWindowStatus & CPythonSystem::GetWindowStatusReference(int iIndex)
{
	return m_WindowStatus[iIndex];
}

void CPythonSystem::ApplyConfig() // 이전 설정과 현재 설정을 비교해서 바뀐 설정을 적용 한다.
{
	if (m_OldConfig.gamma != m_Config.gamma)
	{
		float val = 1.0f;
		
		switch (m_Config.gamma)
		{
			case 0: val = 0.4f;	break;
			case 1: val = 0.7f; break;
			case 2: val = 1.0f; break;
			case 3: val = 1.2f; break;
			case 4: val = 1.4f; break;
		}
		
		CPythonGraphic::Instance().SetGamma(val);
	}

	if (m_OldConfig.is_software_cursor != m_Config.is_software_cursor)
	{
		if (m_Config.is_software_cursor)
			CPythonApplication::Instance().SetCursorMode(CPythonApplication::CURSOR_MODE_SOFTWARE);
		else
			CPythonApplication::Instance().SetCursorMode(CPythonApplication::CURSOR_MODE_HARDWARE);
	}

	m_OldConfig = m_Config;

	ChangeSystem();
}

void CPythonSystem::ChangeSystem()
{
	// Shadow
	/*
	if (m_Config.is_shadow)
		CScreen::SetShadowFlag(true);
	else
		CScreen::SetShadowFlag(false);
	*/
	CSoundManager& rkSndMgr = CSoundManager::Instance();
	/*
	float fMusicVolume;
	if (0 == m_Config.music_volume)
		fMusicVolume = 0.0f;
	else
		fMusicVolume= (float)pow(10.0f, (-1.0f + (float)m_Config.music_volume / 5.0f));
		*/
	rkSndMgr.SetMusicVolume(m_Config.music_volume);

	/*
	float fVoiceVolume;
	if (0 == m_Config.voice_volume)
		fVoiceVolume = 0.0f;
	else
		fVoiceVolume = (float)pow(10.0f, (-1.0f + (float)m_Config.voice_volume / 5.0f));
	*/
	rkSndMgr.SetSoundVolumeGrade(m_Config.voice_volume);	
}

void CPythonSystem::Clear()
{
	SetInterfaceHandler(NULL);
}

bool CPythonSystem::ReadFromRegistry(char* szPIDName, char* szPath, char* szBuf, bool bLocalMachine)
{
	HKEY Registry;
	HKEY key = HKEY_CURRENT_USER;
	if (bLocalMachine)
		key = HKEY_LOCAL_MACHINE;

	DWORD regType = 0, regSize = 0;
	long ReturnStatus = RegOpenKeyEx(key, szPath, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &Registry);
	if (ReturnStatus != ERROR_SUCCESS)
	{
		strcpy(szBuf, "2183");
		return false;
	}

	RegQueryValueEx(Registry, szPIDName, 0, &regType, 0, &regSize);
	RegQueryValueEx(Registry, szPIDName, 0, &regType, (LPBYTE)szBuf, &regSize);
	RegCloseKey(Registry);

	if (regSize > 1)
		return true;
	else
	{
		strcpy(szBuf, "2183");
		return false;
	}
}

void CPythonSystem::WriteToRegistry(char* szPIDName, char* szPath, char* szContent, bool bLocalMachine)
{
	HKEY Registry;
	HKEY key = HKEY_CURRENT_USER;
	if (bLocalMachine)
		key = HKEY_LOCAL_MACHINE;

	long ReturnStatus = RegCreateKeyEx(key, szPath, 0L, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &Registry, NULL);
	if (ReturnStatus != ERROR_SUCCESS)
	{
		RegCloseKey(Registry);
		return;
	}

	RegSetValueEx(Registry, szPIDName, 0, REG_SZ, (BYTE*)szContent, strlen(szContent));
	RegCloseKey(Registry);
}

const char* CPythonSystem::GetMD5(const char* szString)
{
	static char s_szResult[16 * 2 + 1];
	static unsigned char* s_pszBuf;
	static int s_iLen;

	s_iLen = strlen(szString);
	s_pszBuf = new unsigned char[s_iLen + 1];
	memcpy(s_pszBuf, szString, s_iLen);
	s_pszBuf[s_iLen] = '\0';

	MD5_CTX md5;
	MD5Init(&md5);
	MD5Update(&md5, s_pszBuf, s_iLen);
	MD5Final(&md5);

	for (int i = 0; i < 16; ++i)
		sprintf(s_szResult + (i * 2), "%02x", md5.digest[i]);

	return s_szResult;
}

const char* CPythonSystem::GetHWID()
{
	static char s_szHWIDBuffer[2048];
	static bool s_bSet = false;

	if (!s_bSet)
	{
		try
		{
			const static int cs_iGeneratedNameLen = 50;

			// get system data
			SYSTEM_INFO kSystemInfo;
			GetSystemInfo(&kSystemInfo);

			OSVERSIONINFO kOSInfo;
			ZeroMemory(&kOSInfo, sizeof(OSVERSIONINFO));
			kOSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
			GetVersionEx(&kOSInfo);

			char szGeneratedID[cs_iGeneratedNameLen + 1];

			if (!ReadFromRegistry("ComputerID", "SOFTWARE\\AU", szGeneratedID, false))
			{
				for (int i = 0; i < cs_iGeneratedNameLen; ++i)
					szGeneratedID[i] = random_range('A', 'Z');
				szGeneratedID[cs_iGeneratedNameLen] = '\0';

				WriteToRegistry("ComputerID", "SOFTWARE\\AU", szGeneratedID, false);
			}
			else
				szGeneratedID[cs_iGeneratedNameLen] = '\0';

			char szProductID[512 + 1];
			ReadFromRegistry("ProductId", "SOFTWARE\\MICROSOFT\\Windows NT\\CurrentVersion", szProductID, true);

			unsigned long ulOutBufLen = sizeof(IP_ADAPTER_INFO);
			IP_ADAPTER_INFO* pAdapterInfo = (IP_ADAPTER_INFO*) new char[ulOutBufLen];
			DWORD dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
			if (dwRetVal != ERROR_SUCCESS)
			{
				if (dwRetVal == ERROR_BUFFER_OVERFLOW)
				{
					pAdapterInfo = (IP_ADAPTER_INFO*) new char[ulOutBufLen];
					dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen);
				}
			}

			// write into string
			std::string stTemp;
			char szNum[20 + 1];
#define AddNumber(num) _itoa_s(num, szNum, 10), stTemp += szNum

			stTemp += "*3a!#\"y";
			AddNumber(kSystemInfo.wProcessorArchitecture);
			AddNumber(kSystemInfo.dwNumberOfProcessors);
			AddNumber(kSystemInfo.dwProcessorType);
			AddNumber(kSystemInfo.wProcessorLevel);
			AddNumber(kSystemInfo.wProcessorRevision);

			AddNumber(kOSInfo.dwMajorVersion);
			stTemp += szProductID;

			AddNumber(IsProcessorFeaturePresent(PF_3DNOW_INSTRUCTIONS_AVAILABLE));
			AddNumber(IsProcessorFeaturePresent(PF_CHANNELS_ENABLED));
			AddNumber(IsProcessorFeaturePresent(PF_COMPARE_EXCHANGE_DOUBLE));
			AddNumber(IsProcessorFeaturePresent(PF_FLOATING_POINT_EMULATED));
			AddNumber(IsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE));
			AddNumber(IsProcessorFeaturePresent(PF_PAE_ENABLED));
			AddNumber(IsProcessorFeaturePresent(PF_RDTSC_INSTRUCTION_AVAILABLE));
			AddNumber(IsProcessorFeaturePresent(PF_XMMI_INSTRUCTIONS_AVAILABLE));
			AddNumber(IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE));
#undef AddNumber

			stTemp += szGeneratedID;

			strcpy(s_szHWIDBuffer, stTemp.c_str());
			strcat(s_szHWIDBuffer, "aurelion##1+9gde=  ");

			if (dwRetVal == ERROR_SUCCESS)
			{
				for (int i = 0; i < pAdapterInfo->AddressLength; ++i)
				{
					char szTemp[20];
					if (i == pAdapterInfo->AddressLength - 1)
						snprintf(szTemp, sizeof(szTemp), "%.2X", (int)pAdapterInfo->Address[i]);
					else
						snprintf(szTemp, sizeof(szTemp), "%.2X-", (int)pAdapterInfo->Address[i]);
					strcat(s_szHWIDBuffer, szTemp);
				}
				strcat(s_szHWIDBuffer, pAdapterInfo->AdapterName);
			}

			delete[]((char*)pAdapterInfo);

			// get serial number of C: drive
			for (int i = 0; i < 2; ++i)
			{
				char szDrives[256];
				char szVolumeName[256];
				char szFileSystem[256];
				DWORD dwSerialNumber;

				GetLogicalDriveStrings(256, szDrives);
				char* p = szDrives;

				while (*p != '\0')
				{
					DWORD dwDriveType = GetDriveType(p);
					if (i == 1 || strstr(p, "C:"))
					{
						if (dwDriveType != DRIVE_REMOVABLE && dwDriveType != DRIVE_REMOTE && dwDriveType != DRIVE_CDROM && dwDriveType != DRIVE_RAMDISK)
						{
							GetVolumeInformation(p, szVolumeName, 256, &dwSerialNumber, 0, 0, szFileSystem, 256);

							_itoa_s(dwSerialNumber, szNum, 10);
							strcat(s_szHWIDBuffer, szNum);

							s_bSet = true;

							return s_szHWIDBuffer;
						}
					}

					p = strchr(p, '\0');
					p += 1;
				}
			}
		}
		catch (int e)
		{
			snprintf(s_szHWIDBuffer, sizeof(s_szHWIDBuffer), "ERROR: %d", e);
		}

		s_bSet = true;
	}

	return s_szHWIDBuffer;
}

const char* CPythonSystem::GetHWIDHash()
{
	return GetMD5(GetHWID());
}

const char* CPythonSystem::GetSecurityHWIDHash()
{
	std::string stHWID = GetHWID();
	return GetMD5((stHWID + "_CLIENTSECURITY_AURELION").c_str());
}

CPythonSystem::CPythonSystem()
{
	memset(&m_Config, 0, sizeof(TConfig));

	m_poInterfaceHandler = NULL;

	SetDefaultConfig();

	LoadConfig();

	ChangeSystem();

	if (LoadInterfaceStatus())
		m_isInterfaceConfig = true;
	else
		m_isInterfaceConfig = false;
}

CPythonSystem::~CPythonSystem()
{
	assert(m_poInterfaceHandler==NULL && "CPythonSystem MUST CLEAR!");
}
