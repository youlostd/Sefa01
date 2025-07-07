#ifndef __LOCALE_MANAGER__
#define __LOCALE_MANAGER__

#include "../common/length.h"

class CLocaleManager : public singleton<CLocaleManager> {
public:
	enum ELanguageTypes {
		LANG_TYPE_NONE,
		LANG_TYPE_GAME,
		LANG_TYPE_QUEST,
		LANG_TYPE_MAX_NUM,
	};

public:
	CLocaleManager();
	~CLocaleManager();

	void	Initialize();
#ifdef LOCALE_SAVE_LAST_USAGE
	void	SaveLastUsage();
	void	UpdateLastUsage(const char* szString);
#endif
	
private:
	void	InitializeLanguages();
	void	InitializeFromFile(BYTE bLanguageID, const std::string& c_rstFileName);
	void	InitializeFromDatabase();

public:
	std::string	GetIgnoreTranslationString(const char* szString);
private:
	bool		IsIgnoreTranslationString(const char* szString);
	const char*	ReverseIgnoreTranslationString(const char* szString);

public:
	const char*	GetEscapedString(const char* szString);
	const char*	StringToArgument(const char* szString);

private:
	const char*	ExtractStringByPattern(const char* szString, char cStartPatter, char cEndPatter, const std::string& c_rstReplace, std::vector<std::string>& rvec_ExtractedData);
	const char*	ReverseExtractStringByPattern(const char* szString, char cStartPatter, char cEndPatter, const std::string& c_rstReplace, std::vector<std::string>& rvec_ExtractedData);
	const char*	GetStringByExtractedData(const char* szString, const std::string& c_rstReplace, const std::vector<std::string>& rvec_ExtractedData);

	const char*	ExtractStringByPatternEx(const char* szString, char cStartPatter, char cEndPatter, const std::string& c_rstReplace, std::vector<std::string>& rvec_ExtractedData);

	bool		CheckFormatStringPair(BYTE bLanguageType, BYTE bLanguageID, const char* szString1, const char* szString2);

private:
	const char*	GetValueByKVString(BYTE bLanguageType, BYTE bLanguageID, const char* szInputString);

public:
	const char*	FindTranslation(BYTE bLanguageType, BYTE bLanguageID, const char* szBaseString, bool bTranslateBase = true);
	const char*	FindTranslation(BYTE bLanguageType, LPCHARACTER ch, const char* szBaseString, bool bTranslateBase = true);

	const char*	FindTranslationEx(BYTE bLanguageType, BYTE bLanguageID, const char* szBaseString);
	const char*	FindTranslationEx(BYTE bLanguageType, LPCHARACTER ch, const char* szBaseString);

	const char* FindTranslation_Quest(BYTE bLanguageID, const char* szString);
	const char* FindTranslation_Quest(LPCHARACTER ch, const char* szString);

	const char*	GetLanguageName(BYTE bIndex) const;
	const char*	GetShortLanguageName(BYTE bIndex) const;

private:
	const char*	GetTranslatedString(BYTE bLanguageType, BYTE bLanguageID, const std::string& c_rstBaseString);

public:
	const std::string&	GetLocale() const;
	const std::string&	GetLocalePath() const;
	const std::string&	GetMapPath() const;
	const std::string&	GetQuestPath() const;
	const std::string&	GetQuestObjectPath() const;

private:
	typedef std::map<std::string, std::string> TLanguageMap;
#ifdef LOCALE_SAVE_LAST_USAGE
	std::vector<std::string> m_vec_LanguageStringUsed;
#endif
	TLanguageMap	m_map_LanguageStrings[LANG_TYPE_MAX_NUM][LANGUAGE_MAX_NUM];
};

#define Locale_GetLocale() CLocaleManager::instance().GetLocale()
#define Locale_GetBasePath() CLocaleManager::instance().GetLocalePath()
#define Locale_GetMapPath() CLocaleManager::instance().GetMapPath()
#define Locale_GetQuestPath() CLocaleManager::instance().GetQuestPath()
#define Locale_GetQuestObjectPath() CLocaleManager::instance().GetQuestObjectPath()

#define LC_TEXT(lang_or_ch, str) CLocaleManager::instance().FindTranslation(CLocaleManager::LANG_TYPE_GAME, lang_or_ch, str)
#define LC_TEXT_TYPE(type, lang_or_ch, str) CLocaleManager::instance().FindTranslation(type, lang_or_ch, str)
#define LC_TEXT_EX(lang_or_ch, str) CLocaleManager::instance().FindTranslationEx(CLocaleManager::LANG_TYPE_GAME, lang_or_ch, str)
#define LC_TEXT_TYPE_EX(type, lang_or_ch, str) CLocaleManager::instance().FindTranslationEx(type, lang_or_ch, str)
#define LC_QUEST_TEXT(lang_or_ch, str) CLocaleManager::instance().FindTranslation_Quest(lang_or_ch, str)

#endif
