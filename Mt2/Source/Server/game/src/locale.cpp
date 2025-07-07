#include "stdafx.h"
#include "locale.hpp"
#include "config.h"
#include "../../common/length.h"
#include "utils.h"
#include "db.h"
#include "buffer_manager.h"
#include "char.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "log.h"
#include "cmd.h"
#include "questpc.h"
#include "questmanager.h"

#ifdef LOCALE_SAVE_LAST_USAGE
#include "p2p.h"
#endif

using namespace std;

string g_stLocale = "utf8";
string g_stLocaleBasePath = "locale";
string g_stLocaleMapPath = g_stLocaleBasePath + "/map";
string g_stLocaleQuestPath = g_stLocaleBasePath + "/quest";
string g_stLocaleQuestObjectPath = g_stLocaleQuestPath + "/object";

string astLocaleStringNames[LANGUAGE_MAX_NUM];
string astLocaleStringShortNames[LANGUAGE_MAX_NUM];

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
}

CLocaleManager::~CLocaleManager()
{
}

void CLocaleManager::Initialize()
{
#ifdef LOCALE_SAVE_LAST_USAGE
	SaveLastUsage();
#endif

	// clear all
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		astLocaleStringNames[i].clear();
		for (int j = 0; j < LANG_TYPE_MAX_NUM; ++j)
			m_map_LanguageStrings[j][i].clear();
	}

	// intialize languages
	InitializeLanguages();

	if (g_bAuthServer)
		return;

	// initialize language strings
	if (g_bLoadLocaleStringFromFile)
	{
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		{
			string stLocaleStringFile = g_stLocaleBasePath + "/" + strlower(astLocaleStringNames[i].c_str()) + "/" + "locale_string.txt";

			fprintf(stderr, "Load LocaleString [%d] %s\n", i, stLocaleStringFile.c_str());
			InitializeFromFile(i, stLocaleStringFile);
		}

		if (g_bSaveLocaleStringToDatabase)
		{
			char szInsertQuery[QUERY_MAX_LEN];
			char szBaseText[2048];
			char szLangText[2048];
			for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			{
				const char* szLangName = strlower(astLocaleStringNames[i].c_str());
				for (TLanguageMap::iterator it = m_map_LanguageStrings[LANG_TYPE_GAME][i].begin(); it != m_map_LanguageStrings[LANG_TYPE_GAME][i].end(); ++it)
				{
					DBManager::instance().EscapeString(szBaseText, sizeof(szBaseText), it->first.c_str(), it->first.length());
					DBManager::instance().EscapeString(szLangText, sizeof(szLangText), it->second.c_str(), it->second.length());

					snprintf(szInsertQuery, sizeof(szInsertQuery), "INSERT INTO locale_string (`type`, lang_base, lang_base_md5, lang_%s) VALUES (%d, '%s', md5('%s'), '%s') "
						"ON DUPLICATE KEY UPDATE lang_%s = '%s'",
						szLangName, LANG_TYPE_GAME, szBaseText, szBaseText, szLangText, szLangName, szLangText);
					AccountDB::instance().AsyncQuery(szInsertQuery);
				}
			}
		}
	}
	else
		InitializeFromDatabase();
}

#ifdef LOCALE_SAVE_LAST_USAGE
void CLocaleManager::SaveLastUsage()
{
#ifdef __HOMEPAGE_COMMAND__
	if (!g_bProcessHomepageCommands)
		return;
#endif

	std::string query = "";
	for (const auto &c_rstBaseString : m_vec_LanguageStringUsed)
	{
		if (query.empty())
		{
			query = "UPDATE common.locale_string SET last_use=NOW() WHERE lang_base IN('" + c_rstBaseString + "'";
			continue;
		}

		query += ",'" + c_rstBaseString + "'";
		if (query.length() > 1500)
		{
			query += ");";
			AccountDB::instance().AsyncQuery(query.c_str());
			query = "";
		}
	}
	query += ");";
	AccountDB::instance().AsyncQuery(query.c_str());
}

void CLocaleManager::UpdateLastUsage(const char* szString)
{
	if (g_bAuthServer || guild_mark_server)
		return;
	
	m_vec_LanguageStringUsed.push_back(szString);
}
#endif

/*******************************************************************\
| [PRIVATE] (De-)Initialize Functions
\*******************************************************************/

void CLocaleManager::InitializeLanguages()
{
	const string stKeyBase = "LANGUAGE_";

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT mKey, mValue FROM locale WHERE mKey LIKE '%s%%'", stKeyBase.c_str());
	auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));

	if (pMsg->Get()->uiNumRows != LANGUAGE_MAX_NUM)
	{
		fprintf(stderr, "LOAD_LANGUAGES not enough language names (expected %d got %d)", LANGUAGE_MAX_NUM, pMsg->Get()->uiNumRows);
		exit(1);
		return;
	}

	while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		int iLanguageID = atoi(row[0] + stKeyBase.length()) - 1;
		if (iLanguageID < 0 || iLanguageID >= LANGUAGE_MAX_NUM)
		{
			fprintf(stderr, "LOAD_LANGUAGES invalid language id %d (range 0 - %d allowed)", iLanguageID + 1, LANGUAGE_MAX_NUM - 1);
			exit(1);
			return;
		}

		if (!astLocaleStringNames[iLanguageID].empty())
		{
			fprintf(stderr, "LOAD_LANGUAGES double language id %d found", iLanguageID + 1);
			exit(1);
			return;
		}

		char* szPos = strchr(row[1], '|');
		if (szPos == NULL)
		{
			fprintf(stderr, "LOAD_LANGUAGES invalid row line %s (language id %d)", row[1], iLanguageID + 1);
			exit(1);
			return;
		}

		*szPos = '\0';
		astLocaleStringNames[iLanguageID] = row[1];
		astLocaleStringShortNames[iLanguageID] = szPos + 1;
	}
}

void CLocaleManager::InitializeFromFile(BYTE bLanguageID, const string& stFileName)
{
	FILE* fp = fopen(stFileName.c_str(), "r");
	if (!fp)
	{
		fprintf(stderr, "cannot load locale file [%d] %s\n", bLanguageID, stFileName.c_str());
		exit(1);
		return;
	}

	string stBaseString;
	string stLangString;

	char szInputBuf[1024 + 1];
	while (fgets(szInputBuf, sizeof(szInputBuf), fp))
	{
		char* pStrStart = strchr(szInputBuf, '"');
		if (!pStrStart)
			continue;
		char* pStrEnd = strchr(pStrStart + 1, '"');
		if (!pStrEnd)
		{
			fprintf(stderr, " LOAD_LOCALE_FILE[%s] INVALID LINE [%s] (no \" at end of line)\n", stFileName.c_str(), szInputBuf);
			continue;
		}

		if (stBaseString.empty())
		{
			stBaseString.assign(pStrStart + 1, pStrEnd - (pStrStart + 1));
		}
		else
		{
			stLangString.assign(pStrStart + 1, pStrEnd - (pStrStart + 1));

			// add to map
			if (test_server)
				printf("FILE_LoadLocaleString[%d][%s] = %s\n", bLanguageID, stBaseString.c_str(), stLangString.c_str());
			for (int i = 1; i < LANG_TYPE_MAX_NUM; ++i)
				m_map_LanguageStrings[i][bLanguageID][stBaseString] = stLangString;
			stBaseString.clear();
			stLangString.clear();
		}
	}

	if (!stBaseString.empty())
	{
		fprintf(stderr, "LOAD_LOCALE_FILE[%s] INVALID END (base string %s)\n", stFileName.c_str(), stBaseString.c_str());
		exit(1);
		return;
	}
}

void CLocaleManager::InitializeFromDatabase()
{
	AccountDB::instance().DirectQuery("DELETE FROM log.translation_error");
	char szQuery[QUERY_MAX_LEN];

	int iLen = snprintf(szQuery, sizeof(szQuery), "SELECT `type`+0, lang_base");
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, ", lang_%s", strlower(astLocaleStringNames[i].c_str()));
	iLen += snprintf(szQuery + iLen, sizeof(szQuery) - iLen, " FROM locale_string");

	wstring wstBaseString;
	string stBaseString;
	string stLangString;

	auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));
	while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		int col = 0;

		BYTE bType;
		str_to_number(bType, row[col++]);
		wstBaseString = (wchar_t*)row[col];
		stBaseString = row[col++];

		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		{
			if (row[col] && *row[col])
				stLangString = row[col];
			else if (row[1 + LANGUAGE_DEFAULT] && *row[1 + LANGUAGE_DEFAULT])
				stLangString = row[1 + LANGUAGE_DEFAULT];
			else
				stLangString = stBaseString;

			// if (stBaseString == "¶³¾îÁø ¾ÆÀÌÅÛÀº 3ºÐ ÈÄ »ç¶óÁý´Ï´Ù.")
				// printf("*** FOUND STR\n");

			// add to map
			m_map_LanguageStrings[bType][i][stBaseString] = stLangString;
			// if (test_server)
				// printf("LoadLocaleString[%d][%d][%s][%ws] = %s[%s] (COMPARE: %d)\n", bType, i, stBaseString.c_str(), wstBaseString.c_str(), stLangString.c_str(), row[col] ? row[col] : "*", strcmp("¶³¾îÁø ¾ÆÀÌÅÛÀº 3ºÐ ÈÄ »ç¶óÁý´Ï´Ù.", stBaseString.c_str()));

			++col;
		}
	}
}

/*******************************************************************\
| String Modify Functions
\*******************************************************************/

string CLocaleManager::GetIgnoreTranslationString(const char* szString)
{
	static string s_stResult = IGNORE_TRANSLATION_STRING;
	return s_stResult + szString;
}

bool CLocaleManager::IsIgnoreTranslationString(const char* szString)
{
	return !strncmp(szString, IGNORE_TRANSLATION_STRING, strlen(IGNORE_TRANSLATION_STRING));
}

const char* CLocaleManager::ReverseIgnoreTranslationString(const char* szString)
{
	return szString + strlen(IGNORE_TRANSLATION_STRING);
}

const char* CLocaleManager::GetEscapedString(const char* szString)
{
	static std::string stResult;
	stResult = strReplaceAll(szString, "[", "\\[");
	stResult = strReplaceAll(stResult, "]", "\\]");
	return stResult.c_str();
}

const char* CLocaleManager::StringToArgument(const char* szString)
{
	static std::string s_stRet;
	s_stRet = (((string) "[") + GetEscapedString(szString) + "]");
	return s_stRet.c_str();
}

const char* CLocaleManager::ExtractStringByPattern(const char* szString, char cStartPatter, char cEndPatter, const string& c_rstReplace, vector<string>& rvec_ExtractedData)
{
	static TEMP_BUFFER kResultBuf;
	kResultBuf.reset();

	// init pattern string
	static char szPatternString[2 + 1];
	szPatternString[0] = cStartPatter;
	szPatternString[1] = cEndPatter;
	szPatternString[2] = '\0';

	// init vars
	string stTemp;
	const char* szFindStart = NULL;
	const char* szFindEnd = szString - 1;
	const char* szLastEnd = szString - 1;

	// find all pattern
	while ((szFindStart = strchr_new(szFindEnd + 1, cStartPatter, true)) && (szFindEnd = strchr_new(szFindStart + 1, cEndPatter, true)))
	{
		kResultBuf.write(szLastEnd + 1, szFindStart - (szLastEnd + 1));

		// add extracted data
		stTemp.assign(szFindStart + 1, szFindEnd - (szFindStart + 1));
		rvec_ExtractedData.push_back(strunescape(stTemp.c_str(), szPatternString));

		// write replaced
		kResultBuf.write(c_rstReplace.c_str(), c_rstReplace.length());

		// save end ptr
		szLastEnd = szFindEnd;
	}

	// write end of string (after last found pattern)
	const char* szEndPos = szString + strlen(szString);
	kResultBuf.write(szLastEnd + 1, szEndPos - (szLastEnd + 1) + 1);

	return strunescape((char*)kResultBuf.read_peek(), szPatternString);
}

const char* CLocaleManager::ReverseExtractStringByPattern(const char* szString, char cStartPatter, char cEndPatter, const string& c_rstReplace, vector<string>& rvec_ExtractedData)
{
	static TEMP_BUFFER kResultBuf;
	kResultBuf.reset();

	// init vars
	string stTemp;
	const char* szFindStart = NULL;
	const char* szFindEnd = szString - 1;
	const char* szLastEnd = szString - 1;

	// find all pattern
	while ((szFindStart = strchr_new(szFindEnd + 1, cStartPatter, true)) && (szFindEnd = strchr_new(szFindStart + 1, cEndPatter, true)))
	{
		stTemp.assign(szLastEnd + 1, szFindStart - (szLastEnd + 1));
		if (!stTemp.empty())
		{
			rvec_ExtractedData.push_back(stTemp);

			// write replaced
			kResultBuf.write(c_rstReplace.c_str(), c_rstReplace.length());
		}

		// add extracted data
		kResultBuf.write(szFindStart, szFindEnd - szFindStart + 1);

		// save end ptr
		szLastEnd = szFindEnd;
	}

	// write end of string (after last found pattern)
	const char* szEndPos = szString + strlen(szString);
	stTemp.assign(szLastEnd + 1, szEndPos - (szLastEnd + 1));
	if (!stTemp.empty())
	{
		rvec_ExtractedData.push_back(stTemp);

		// write replaced
		kResultBuf.write(c_rstReplace.c_str(), c_rstReplace.length());
	}

	kResultBuf.write(szEndPos, sizeof(char));
	return (char*)kResultBuf.read_peek();
}

const char* CLocaleManager::GetStringByExtractedData(const char* szString, const string& c_rstReplace, const vector<string>& rvec_ExtractedData)
{
	static TEMP_BUFFER kResultBuf;
	kResultBuf.reset();

	// init vars
	int iDataIndex = 0;
	const char* szFind = NULL;
	const char* szLastFind = szString - c_rstReplace.length();

	// find all occurences
	while ((szFind = strstr(szLastFind + c_rstReplace.length(), c_rstReplace.c_str())) && iDataIndex < rvec_ExtractedData.size())
	{
		kResultBuf.write(szLastFind + c_rstReplace.length(), szFind - (szLastFind + c_rstReplace.length()));

		// write extracted data
		kResultBuf.write(rvec_ExtractedData[iDataIndex].c_str(), rvec_ExtractedData[iDataIndex].length());

		// save end ptr
		szLastFind = szFind;

		// inc data index
		++iDataIndex;
	}

	// write end of string (after last found occurence)
	const char* szEndPos = szString + strlen(szString);
	kResultBuf.write(szLastFind + c_rstReplace.length(), szEndPos - (szLastFind + c_rstReplace.length()) + 1);

	return (char*)kResultBuf.read_peek();
}

// works like ExtractStringByPattern but replaces all numbers out of patterns as well
const char* CLocaleManager::ExtractStringByPatternEx(const char* szString, char cStartPatter, char cEndPatter, const string& c_rstReplace, vector<string>& rvec_ExtractedData)
{
	static TEMP_BUFFER kResultBuf;
	kResultBuf.reset();

	// init pattern string
	static char szPatternString[2 + 1];
	szPatternString[0] = cStartPatter;
	szPatternString[1] = cEndPatter;
	szPatternString[2] = '\0';

	// init vars
	string stTemp;
	const char* szPtrCur = szString;
	const char* szPtrEnd = szString + strlen(szString);
	const char* szFindStart = NULL;

	// find all pattern
	while (szPtrCur < szPtrEnd)
	{
		// found number
		if (szFindStart)
		{
			// if number go on
			if (chr_is_number(*szPtrCur))
			{
				++szPtrCur;
				continue;
			}

			// add extracted data
			stTemp.assign(szFindStart, szPtrCur - szFindStart);
			rvec_ExtractedData.push_back(stTemp);

			// write replaced
			kResultBuf.write(c_rstReplace.c_str(), c_rstReplace.length());

			// reset find ptr
			szFindStart = NULL;
		}
		// check for number
		else if (chr_is_number(*szPtrCur))
		{
			// set ptrs
			szFindStart = szPtrCur;
			++szPtrCur;
			continue;
		}

		// check for pattern
		if (*szPtrCur == cStartPatter)
		{
			// find end pattern
			const char* szFindEnd = strchr(szPtrCur + 1, cEndPatter);
			if (szFindEnd)
			{
				// add extracted data
				stTemp.assign(szPtrCur + 1, szFindEnd - (szPtrCur + 1));
				rvec_ExtractedData.push_back(strunescape(stTemp.c_str(), szPatternString));

				// write replaced
				kResultBuf.write(c_rstReplace.c_str(), c_rstReplace.length());

				// set ptr
				szPtrCur = szFindEnd + 1;
				continue;
			}
		}

		kResultBuf.write(szPtrCur++, 1);
	}

	if (szFindStart)
	{
		// add extracted data
		stTemp.assign(szFindStart, szPtrCur - szFindStart);
		rvec_ExtractedData.push_back(stTemp);

		// write replaced
		kResultBuf.write(c_rstReplace.c_str(), c_rstReplace.length());
	}

	kResultBuf.write(szPtrCur, 1);
	return strunescape((char*)kResultBuf.read_peek(), szPatternString);
}

bool CLocaleManager::CheckFormatStringPair(BYTE bLanguageType, BYTE bLanguageID, const char* szString1, const char* szString2)
{
	const char* szPosBase = szString1 - 1;
	const char* szPosNew = szString2;

	// check order of %*
	while ((szPosNew = strchr(szPosNew, '%')) != NULL && *(szPosNew + 1) != '\0')
	{
		if (*(szPosNew + 1) == '%')
		{
			szPosNew += 2;
			continue;
		}

		do {
			szPosBase = strchr(szPosBase + 1, '%');
			if (szPosBase == NULL || *(szPosBase + 1) == '\0')
			{
				LogManager::instance().TranslationErrorLog(bLanguageType, szString1, astLocaleStringNames[bLanguageID].c_str(), szString2,
					"too many arguments");
				return false;
			}
		} while (*(szPosBase + 1) == '%');

		if (*(szPosNew + 1) != *(szPosBase + 1))
		{
			char szError[50];
			snprintf(szError, sizeof(szError), "invalid argument [pos %d expected %c got %c]",
				szPosNew - szString2, *(szPosBase + 1), *(szPosNew + 1));
			LogManager::instance().TranslationErrorLog(bLanguageType, szString1, astLocaleStringNames[bLanguageID].c_str(), szString2,
				szError);
			return false;
		}

		++szPosNew;
	}

	// check if there are % are missing
	do {
		szPosBase = strchr(szPosBase + 1, '%');
		if (szPosBase == NULL || *(szPosBase + 1) == '\0')
			return true;
	} while (*(++szPosBase) == '%');

	LogManager::instance().TranslationErrorLog(bLanguageType, szString1, astLocaleStringNames[bLanguageID].c_str(), szString2,
		"not enough arguments");
	return false;
}

const char*	CLocaleManager::GetValueByKVString(BYTE bLanguageType, BYTE bLanguageID, const char* szInputString)
{
	const char* szPtrFind = strchr_new(szInputString, '=', true);
	if (szPtrFind == NULL)
		return szInputString;

	string stKey;
	stKey.assign(strlower(szInputString), szPtrFind - szInputString);

	string stValue;
	stValue.assign(szPtrFind + 1);

	if (stKey == "item")
	{
		int iVnum = 0;
		str_to_number(iVnum, szPtrFind + 1);

		auto pProto = ITEM_MANAGER::instance().GetTable(iVnum);
		return pProto ? pProto->locale_name(bLanguageID).c_str() : "UNKNOWN_ITEM";
	}
	else if (stKey == "mob")
	{
		int iVnum = 0;
		str_to_number(iVnum, szPtrFind + 1);

		const CMob* pMob = CMobManager::instance().Get(iVnum);
		return pMob ? pMob->m_table.locale_name(bLanguageID).c_str() : "UNKNOWN_MOB";
	}
	else if (stKey == "lc_text")
	{
		return GetTranslatedString(bLanguageType, bLanguageID, stValue.c_str());
	}

	return szInputString;
}

/*******************************************************************\
| Translation Functions
\*******************************************************************/

const char*	CLocaleManager::FindTranslation(BYTE bLanguageType, BYTE bLanguageID, const char* szBaseString, bool bTranslateBase)
{
	if (!*szBaseString)
		return szBaseString;

	if (IsIgnoreTranslationString(szBaseString))
		return ReverseIgnoreTranslationString(szBaseString);

	vector<string> vec_Arguments;

	const char* szExtractedString = ExtractStringByPattern(szBaseString, '[', ']', "%s", vec_Arguments); // copy out arguments
	std::string stTranslatedString = bTranslateBase && strlen(szExtractedString) > 2 * vec_Arguments.size() ? GetTranslatedString(bLanguageType, bLanguageID, szExtractedString) : szExtractedString; // translate string

	for (int i = 0; i < vec_Arguments.size(); ++i)
	{
		const char* szNewStr = GetValueByKVString(bLanguageType, bLanguageID, vec_Arguments[i].c_str());
		if (szNewStr != vec_Arguments[i].c_str())
			vec_Arguments[i] = szNewStr;
	}

	return GetStringByExtractedData(stTranslatedString.c_str(), "%s", vec_Arguments);
}

const char* CLocaleManager::FindTranslation(BYTE bLanguageType, LPCHARACTER ch, const char* szBaseString, bool bTranslateBase)
{
	return FindTranslation(bLanguageType, ch?ch->GetLanguageID():LANGUAGE_DEFAULT, szBaseString, bTranslateBase);
}

const char*	CLocaleManager::FindTranslationEx(BYTE bLanguageType, BYTE bLanguageID, const char* szBaseString)
{
	if (!*szBaseString)
		return szBaseString;

	if (IsIgnoreTranslationString(szBaseString))
		return ReverseIgnoreTranslationString(szBaseString);

	vector<string> vec_Arguments;

	const char* szExtractedString = ExtractStringByPatternEx(szBaseString, '[', ']', "%s", vec_Arguments); // copy out arguments
	const char* szTranslatedString = GetTranslatedString(bLanguageType, bLanguageID, szExtractedString); // translate string

	for (int i = 0; i < vec_Arguments.size(); ++i)
	{
		const char* szNewStr = GetValueByKVString(bLanguageType, bLanguageID, vec_Arguments[i].c_str());
		if (szNewStr != vec_Arguments[i].c_str())
			vec_Arguments[i] = szNewStr;
	}

	return GetStringByExtractedData(szTranslatedString, "%s", vec_Arguments);
}

const char* CLocaleManager::FindTranslationEx(BYTE bLanguageType, LPCHARACTER ch, const char* szBaseString)
{
	return FindTranslationEx(bLanguageType, ch?ch->GetLanguageID():LANGUAGE_DEFAULT, szBaseString);
}

const char* CLocaleManager::FindTranslation_Quest(BYTE bLanguageID, const char* szString)
{
	vector<string> vec_Strings;
	vector<string> vec_TranslatedStrings;

	const char* szMainString = ReverseExtractStringByPattern(szString, '{', '}', "%s", vec_Strings);

	for (int i = 0; i < vec_Strings.size(); ++i)
	{
		if (vec_Strings[i][vec_Strings[i].length() - 1] == ' ')
			vec_Strings[i].erase(--vec_Strings[i].end());

		vec_TranslatedStrings.push_back(FindTranslationEx(LANG_TYPE_QUEST, bLanguageID, vec_Strings[i].c_str()));
	}
	
	return GetStringByExtractedData(szMainString, "%s", vec_TranslatedStrings);
}

const char* CLocaleManager::FindTranslation_Quest(LPCHARACTER ch, const char* szString)
{
	return FindTranslation_Quest(ch?ch->GetLanguageID():LANGUAGE_DEFAULT, szString);
}

const char* CLocaleManager::GetLanguageName(BYTE bIndex) const
{
	if (bIndex >= LANGUAGE_MAX_NUM)
	{
		sys_err("invalid language %u", bIndex);
		return "";
	}

	return astLocaleStringNames[bIndex].c_str();
}

const char* CLocaleManager::GetShortLanguageName(BYTE bIndex) const
{
	if (bIndex >= LANGUAGE_MAX_NUM)
	{
		sys_err("invalid language %u", bIndex);
		return "";
	}

	return astLocaleStringShortNames[bIndex].c_str();
}

const char* CLocaleManager::GetTranslatedString(BYTE bLanguageType, BYTE bLanguageID, const string& c_rstBaseString)
{
	static string stLangString;

	TLanguageMap::iterator it = m_map_LanguageStrings[bLanguageType][bLanguageID].find(c_rstBaseString);

	if (it == m_map_LanguageStrings[bLanguageType][bLanguageID].end())
	{
		stLangString = c_rstBaseString;

		// FIX_UMLAUTS
		//	stLangString = strreplace(stLangString.c_str(), "Ã¼|Ãœ|Ã¤|Ã„|Ã¶|Ã–", "ü|Ü|ä|Ä|ö|Ö");
		//	stLangString = strreplace(stLangString.c_str(), "ã|º|þ|â|Î|î", "a|s|t|a|I|i");
		// END_OF_FIX_UMLAUTS

		// find language string in other language types
		bool bFoundTranslation = false;
		for (int i = 1; i < LANG_TYPE_MAX_NUM; ++i)
		{
			if (i == bLanguageType)
				continue;

			it = m_map_LanguageStrings[i][bLanguageID].find(c_rstBaseString);
			if (it != m_map_LanguageStrings[i][bLanguageID].end())
			{
				for (int j = 0; j < LANGUAGE_MAX_NUM; ++j)
				{
					if (j == bLanguageID)
						m_map_LanguageStrings[bLanguageType][j][c_rstBaseString] = it->second;
					else
						m_map_LanguageStrings[bLanguageType][j][c_rstBaseString] = m_map_LanguageStrings[i][j][c_rstBaseString];
				}

				stLangString = it->second;

				bFoundTranslation = true;

				break;
			}
		}

		// add into map
		if (!bFoundTranslation)
		{
			if (test_server)
			{
				char szBuf[2048];
				snprintf(szBuf, sizeof(szBuf), GetIgnoreTranslationString("ADD_LOCALE: %s").c_str(), c_rstBaseString.c_str());
				BroadcastNotice(szBuf);
			}

			for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			{
				string& rstNewLangString = m_map_LanguageStrings[bLanguageType][i][c_rstBaseString];
				if (rstNewLangString.empty())
					rstNewLangString = stLangString;
			}
		}
		// if locale is using database table, then add missing
		if ((!g_bLoadLocaleStringFromFile || g_bSaveLocaleStringToDatabase) && !bFoundTranslation)
		{
			char szEscapedBaseString[2048];
			char szEscapedLangString[2048];
			DBManager::instance().EscapeString(szEscapedBaseString, sizeof(szEscapedBaseString), c_rstBaseString.c_str(), c_rstBaseString.length());
			DBManager::instance().EscapeString(szEscapedLangString, sizeof(szEscapedLangString), stLangString.c_str(), stLangString.length());

			char szQuery[QUERY_MAX_LEN];

			quest::PC* pPC = quest::CQuestManager::instance().GetCurrentPC();
			
			int iLen = snprintf(szQuery, sizeof(szQuery), "REPLACE INTO locale_string SET `type`=%d, lang_base_md5=md5('%s'), lang_base='%s', lang_%s='%s', quest_name='%s'",
				bLanguageType, szEscapedBaseString, szEscapedBaseString, astLocaleStringNames[LANGUAGE_DEFAULT].c_str(), szEscapedLangString, (bLanguageType==LANG_TYPE_QUEST && pPC) ? pPC->GetCurrentQuestName().c_str() : "");

			AccountDB::instance().AsyncQuery(szQuery);
		}

		// error, is it really an error?
		if (test_server)
			sys_err("GetTranslatedString [%d][%d]: %s (-> %s)", bLanguageType, bLanguageID, c_rstBaseString.c_str(), stLangString.c_str());

		// return lang string
		return stLangString.c_str();
	}

#ifdef LOCALE_SAVE_LAST_USAGE
	if (std::find(m_vec_LanguageStringUsed.begin(), m_vec_LanguageStringUsed.end(), c_rstBaseString.c_str()) == m_vec_LanguageStringUsed.end())
	{
		UpdateLastUsage(c_rstBaseString.c_str());

		network::GGOutputPacket<network::GGLocaleUpdateLastUsagePacket> pack;
		pack->set_lang_base(c_rstBaseString.c_str());

		P2P_MANAGER::instance().Send(pack);
	}
#endif

	stLangString = it->second.c_str();

	// check string for equal %-order and count
	if (!CheckFormatStringPair(bLanguageType, bLanguageID, c_rstBaseString.c_str(), it->second.c_str()))
		stLangString = c_rstBaseString.c_str();

	//stLangString = strreplace(stLangString.c_str(), "ã|º|þ|â|Î|î", "a|s|t|a|I|i");

	// return language string
	return stLangString.c_str();
}

/*******************************************************************\
| Data Functions
\*******************************************************************/

const string& CLocaleManager::GetLocale() const
{
	return g_stLocale;
}

const string& CLocaleManager::GetLocalePath() const
{
	return g_stLocaleBasePath;
}

const string& CLocaleManager::GetMapPath() const
{
	return g_stLocaleMapPath;
}

const string& CLocaleManager::GetQuestPath() const
{
	return g_stLocaleQuestPath;
}

const string& CLocaleManager::GetQuestObjectPath() const
{
	return g_stLocaleQuestObjectPath;
}
