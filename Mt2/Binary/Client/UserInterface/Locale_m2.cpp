#include "StdAfx.h"
#include "Locale_m2.h"
#include "PythonConfig.h"

std::string g_stLocalePath = "locale";

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CLocaleManager - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CLocaleManager::CLocaleManager()
{
	const TLanguageNameInfo kNameInitInfo[] = {
		{ "english", "en" },
		{ "german", "de" },
		{ "turkish", "tr" },
		{ "romania", "ro" },
		{ "polish", "pl" },
		{ "italian", "it" },
		{ "spanish", "es" },
		{ "hungarian", "hu" },
		{ "czech", "cz" },
		{ "portuguese", "pt" },
		{ "french", "fr" },
		{ "arabic", "ar" },
		{ "greek", "gr" },
	};

	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		m_akLanguageNameInfo[i].stName = kNameInitInfo[i].stName;
		m_akLanguageNameInfo[i].stShortName = kNameInitInfo[i].stShortName;
	}

	// default language
	m_bLanguage = LANGUAGE_ENGLISH;

	// get system language and set default is available
	const TLanguageIndexInfo kIndexInitInfo[] = {
		{ LANG_ENGLISH, LANGUAGE_ENGLISH },
		{ LANG_GERMAN, LANGUAGE_GERMAN },
		{ LANG_ROMANIAN, LANGUAGE_ROMANIA },
		{ LANG_POLISH, LANGUAGE_POLISH },
		{ LANG_TURKISH, LANGUAGE_TURKISH },
		{ LANG_ITALIAN, LANGUAGE_ITALIAN },
		{ LANG_SPANISH, LANGUAGE_SPANISH },
		{ LANG_HUNGARIAN, LANGUAGE_HUNGARIAN },
		{ LANG_CZECH, LANGUAGE_CZECH },
		{ LANG_PORTUGUESE, LANGUAGE_PORTUGUESE },
		{ LANG_FRENCH, LANGUAGE_FRENCH },
		{ LANG_ARABIC, LANGUAGE_ARABIC },
		{ LANG_GREEK, LANGUAGE_GREEK },
	};

	for (int i = 0; i < ARRAYSIZE(kIndexInitInfo); ++i)
	{
		if (PRIMARYLANGID(GetSystemDefaultLangID()) == kIndexInitInfo[i].wSystemIndex)
		{
			m_bLanguage = kIndexInitInfo[i].wClientIndex;
			break;
		}
	}

	SetDefaultCodePage(CP_UTF8);
}

CLocaleManager::~CLocaleManager()
{

}

void CLocaleManager::Initialize()
{
	BYTE bLanguage = GetLanguageByName(CPythonConfig::Instance().GetString(CPythonConfig::CLASS_GENERAL, "language", "").c_str(), false);
	if (bLanguage < LANGUAGE_MAX_NUM)
		m_bLanguage = bLanguage;

	if (m_bLanguage == LANGUAGE_ARABIC)
		SetDefaultCodePage(CP_ARABIC);
	else
		SetDefaultCodePage(CP_UTF8);
}

void CLocaleManager::Destroy()
{
}

/*******************************************************************\
| [PUBLIC] Convert Functions
\*******************************************************************/

BYTE CLocaleManager::GetLanguageByName(const char* szName, bool bShort) const
{
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		if (bShort)
		{
			if (m_akLanguageNameInfo[i].stShortName == szName)
				return i;
		}
		else
		{
			if (m_akLanguageNameInfo[i].stName == szName)
				return i;
		}
	}

	return LANGUAGE_MAX_NUM;
}

const char* CLocaleManager::GetLanguageNameByID(BYTE bLanguageID, bool bShort) const
{
	if (bLanguageID >= LANGUAGE_MAX_NUM)
		return "";

	if (bShort)
		return m_akLanguageNameInfo[bLanguageID].stShortName.c_str();
	else
		return m_akLanguageNameInfo[bLanguageID].stName.c_str();
}

/*******************************************************************\
| [PUBLIC] Data Functions
\*******************************************************************/

BYTE CLocaleManager::GetLanguage() const
{
	return m_bLanguage;
}

void CLocaleManager::SetLanguage(BYTE bLanguageID)
{
	m_bLanguage = bLanguageID;
	CPythonConfig::Instance().Write(CPythonConfig::CLASS_GENERAL, "language", GetLanguageName());
}

const char* CLocaleManager::GetLanguageName() const
{
	return GetLanguageNameByID(GetLanguage(), false);
}

const char* CLocaleManager::GetLanguageShortName() const
{
	return GetLanguageNameByID(GetLanguage(), true);
}

const std::string& CLocaleManager::GetLocaleBasePath() const
{
	return g_stLocalePath;
}

const std::string& CLocaleManager::GetDefaultLocalePath() const
{
	static std::string stLocalePath;
	stLocalePath = g_stLocalePath + "/" + GetLanguageNameByID(LANGUAGE_DEFAULT, true);
	return stLocalePath;
}

const std::string& CLocaleManager::GetLocalePath() const
{
	static std::string stLocalePath;
	stLocalePath = g_stLocalePath + "/" + GetLanguageShortName();
	return stLocalePath;
}

/*******************************************************************\
| [PUBLIC] Locale Functions
\*******************************************************************/

DWORD CLocaleManager::GetSkillPower(BYTE level)
{
	static const BYTE SKILL_POWER_NUM = 51;

	if (level >= SKILL_POWER_NUM)
		return 0;
	
	// 0 5 6 8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 40 50 52 54 56 58 60 63 66 69 72 82 85 88 91 94 98 102 106 110 115 125 127 131 134 138 141 145 148 152 155 165
	static unsigned INTERNATIONAL_SKILL_POWERS[SKILL_POWER_NUM]=
	{
		0, 
			5,  6,  8, 10, 12, 
			14, 16, 18, 20, 22, 
			24, 26, 28, 30, 32, 
			34, 36, 38, 40, 50, // master
			52, 54, 56, 58, 60, 
			63, 66, 69, 72, 82, // grand_master
			85, 88, 91, 94, 98, 
			102,106,110,115,125,// perfect_master
			127,131,134,138,141,
			145,148,152,155,165,// legendary_master
	};
	return INTERNATIONAL_SKILL_POWERS[level];
}
