#pragma once

#include "Locale_inc.h"
#include "../EterBase/Singleton.h"

#define IGNORE_TRANSLATION_STRING "[NOTRANSLATION]"
#define IGNORE_TRANSLATION_STRING_LEN (sizeof(IGNORE_TRANSLATION_STRING) - 1)

#define LSS_SECURITY_KEY "testtesttesttest"

enum
{
	LANGUAGE_ENGLISH,
	LANGUAGE_GERMAN,
	LANGUAGE_TURKISH,
	LANGUAGE_ROMANIA,
	LANGUAGE_POLISH,
	LANGUAGE_ITALIAN,
	LANGUAGE_SPANISH,
	LANGUAGE_HUNGARIAN,
	LANGUAGE_CZECH,
	LANGUAGE_PORTUGUESE,
	LANGUAGE_FRENCH,
	LANGUAGE_ARABIC,
	LANGUAGE_GREEK,
	LANGUAGE_MAX_NUM,

	LANGUAGE_DEFAULT = LANGUAGE_GERMAN,
};

class CLocaleManager : public singleton<CLocaleManager>
{
private:
	typedef struct SLanguageInfo {
		std::string stName;
		std::string stShortName;
	} TLanguageNameInfo;

	typedef struct SLanguageIndexInfo {
		WORD wSystemIndex;
		WORD wClientIndex;
	} TLanguageIndexInfo;

public:
	CLocaleManager();
	~CLocaleManager();

	void Initialize();
	void Destroy();

public:
	BYTE GetLanguageByName(const char* szName, bool bShort = true) const;
	const char*	GetLanguageNameByID(BYTE bLanguageID, bool bShort = true) const;

public:
	BYTE		GetLanguage() const;
	void		SetLanguage(BYTE bLanguageID);
	const char*	GetLanguageName() const;
	const char*	GetLanguageShortName() const;

	const std::string&	GetLocaleBasePath() const;
	const std::string&	GetDefaultLocalePath() const;
	const std::string&	GetLocalePath() const;

public:
	DWORD		GetSkillPower(BYTE bLevel);

private:
	TLanguageNameInfo	m_akLanguageNameInfo[LANGUAGE_MAX_NUM];
	BYTE				m_bLanguage;
};
