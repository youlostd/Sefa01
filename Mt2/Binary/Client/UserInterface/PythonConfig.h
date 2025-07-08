#ifndef __PYTHON_CONFIG_HEADER__
#define __PYTHON_CONFIG_HEADER__

#include "StdAfx.h"

#ifdef ENABLE_PYTHON_CONFIG
#include <minini_12b/minIni.h>

class CPythonConfig : public CSingleton<CPythonConfig>
{
public:
	enum EClassTypes {
		CLASS_GENERAL,
		CLASS_CHAT,
		CLASS_OPTION,
		CLASS_UNIQUE,
#ifdef ENABLE_AUCTION
		CLASS_AUCTIONHOUSE,
#endif
		CLASS_WHISPER,
#ifdef ENABLE_PET_SYSTEM
		CLASS_PET,
#endif
		CLASS_PLAYER,
	};

	enum EPickUpFlags
	{
		DISABLE_PICKUP_ARMOR = 1 << 0,
		DISABLE_PICKUP_WEAPON = 1 << 1,
		DISABLE_PICKUP_ETC = 1 << 2,
#ifdef NEW_PICKUP_FILTER
		DISABLE_PICKUP_POTION = 1 << 3,
		DISABLE_PICKUP_BOOK = 1 << 4,
		DISABLE_PICKUP_SIZE_1 = 1 << 5,
		DISABLE_PICKUP_SIZE_2 = 1 << 6,
		DISABLE_PICKUP_SIZE_3 = 1 << 7,
#endif
	};

public:
	CPythonConfig();
	~CPythonConfig();
	void Initialize(const std::string& stFileName);
	void Initialize_LoadDefault(EClassTypes eType, const std::string& stKey, float fDefaultValue);
	void Initialize_LoadDefault(EClassTypes eType, const std::string& stKey, const std::string& stDefaultValue);
	void Initialize_LoadDefaults();

	std::string	GetClassNameByType(EClassTypes eType) const;

	void		Write(EClassTypes eType, const std::string& stKey, const std::string& stValue);
	void		Write(EClassTypes eType, const std::string& stKey, const char* szValue);
	void		Write(EClassTypes eType, const std::string& stKey, int iValue);
	void		Write(EClassTypes eType, const std::string& stKey, bool bValue);

	std::string	GetString(EClassTypes eType, const std::string& stKey, const std::string& stDefaultValue = "") const;
	int			GetInteger(EClassTypes eType, const std::string& stKey, int iDefaultValue = 0) const;
	float		GetFloat(EClassTypes eType, const std::string& stKey, float fDefaultValue = 0.0f) const;
	bool		GetBool(EClassTypes eType, const std::string& stKey, bool bDefaultValue = false) const;

	void		RemoveSection(EClassTypes eType);

	void		ReloadBlockNameList();
	void		LoadBlockNameList(const std::string& stFileName);
	DWORD		GetBlockNameCount() const;
	const char*	GetBlockName(DWORD dwIndex);
	void		AddBlockName(const std::string& c_rstName);
	void		RemoveBlockName(const std::string& c_rstName);
	bool		IsBlockName(const std::string& c_rstName);
	void		SaveBlockNameList();
	void		SendReloadBlockListToOthers();
	std::string	GetBlockNameFileName() const { return m_stBlockNameFileName; }

private:
	minIni*		m_pkIniFile;
	std::string	m_stFileName;

	std::string	m_stBlockNameFileName;
	std::set<std::string>	m_set_BlockNameList;
};
#endif

#endif
