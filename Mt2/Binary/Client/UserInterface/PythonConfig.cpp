#include "StdAfx.h"

#ifdef ENABLE_PYTHON_CONFIG
#include "PythonConfig.h"
#include <minini_12b/minIni.h>
#include "PythonApplication.h"
#include <thread>
#include "AbstractPlayer.h"

CPythonConfig::CPythonConfig()
{
	m_pkIniFile = NULL;
}
CPythonConfig::~CPythonConfig()
{
	if (m_pkIniFile)
	{
		delete m_pkIniFile;
		m_pkIniFile = NULL;
	}
}

void CPythonConfig::Initialize(const std::string& szFileName)
{
	if (m_pkIniFile)
	{
		if (m_stFileName == szFileName)
			return;

		delete m_pkIniFile;
	}

	m_pkIniFile = new minIni(szFileName);
	m_stFileName = szFileName;

	Initialize_LoadDefaults();
}

void CPythonConfig::Initialize_LoadDefault(EClassTypes eType, const std::string& stKey, float fDefaultValue)
{
	Initialize_LoadDefault(eType, stKey, std::to_string(fDefaultValue));
}

void CPythonConfig::Initialize_LoadDefault(EClassTypes eType, const std::string& stKey, const std::string& stDefaultValue)
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to load default key %s with value %s", stKey.c_str(), stDefaultValue.c_str());
		return;
	}

	const char* c_pszNoKeyVal = "!---nokeygiven";
	if (c_pszNoKeyVal == GetString(eType, stKey, c_pszNoKeyVal))
	{
		Write(eType, stKey, stDefaultValue);
	}
}

void CPythonConfig::Initialize_LoadDefaults()
{
	int iCPUCoreCount = std::thread::hardware_concurrency();

	float fPerfTreeRange = 15000.0f;
	float fPerfGravelRange = 15000.0f;
	float fPrefEffectRange = 15000.0f;
	float fPrefShopRange = 15000.0f;
	if (iCPUCoreCount < 8)
	{
		fPerfTreeRange = 1500.0f;
		fPerfGravelRange = 1500.0f;
		fPrefEffectRange = 1500.0f;
		fPrefShopRange = 1500.0f;
	}

	Initialize_LoadDefault(CLASS_OPTION, "perf_tree_range", fPerfTreeRange);
	Initialize_LoadDefault(CLASS_OPTION, "perf_gravel_range", fPerfGravelRange);
	Initialize_LoadDefault(CLASS_OPTION, "perf_effect_range", fPrefEffectRange);
	Initialize_LoadDefault(CLASS_OPTION, "perf_shop_range", fPrefShopRange);

	extern long SHOP_SHOW_RANGE;
	SHOP_SHOW_RANGE = (long)GetFloat(CLASS_OPTION, "perf_shop_range", 1500.0f);
}

std::string CPythonConfig::GetClassNameByType(EClassTypes eType) const
{
	switch (eType)
	{
	case CLASS_GENERAL:
		return "GENERAL";
		break;

	case CLASS_CHAT:
		return "CHAT";
		break;

	case CLASS_OPTION:
		return "OPTION";
		break;

	case CLASS_UNIQUE:
		return "UNIQUE";
		break;

#ifdef ENABLE_AUCTION
	case CLASS_AUCTIONHOUSE:
		return "AUCTIONHOUSE";
		break;
#endif

	case CLASS_WHISPER:
		return "WHISPER";
		break;

#ifdef ENABLE_PET_SYSTEM
	case CLASS_PET:
		return "PET";
		break;
#endif

	case CLASS_PLAYER:
		return IAbstractPlayer::GetSingleton().GetName();
		break;
	}

	TraceError("unkown class type %u", eType);
	return "UNKOWN";
}

void CPythonConfig::Write(EClassTypes eType, const std::string& stKey, const std::string& stValue)
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to write key %s with value %s", stKey.c_str(), stValue.c_str());
		return;
	}

	m_pkIniFile->put(GetClassNameByType(eType), stKey, stValue);
}
void CPythonConfig::Write(EClassTypes eType, const std::string& stKey, const char* szValue)
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to write key %s with value %s", stKey.c_str(), szValue);
		return;
	}

	m_pkIniFile->put(GetClassNameByType(eType), stKey, szValue);
}
void CPythonConfig::Write(EClassTypes eType, const std::string& stKey, int iValue)
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to write key %s with value %d", stKey.c_str(), iValue);
		return;
	}

	m_pkIniFile->put(GetClassNameByType(eType), stKey, iValue);
}
void CPythonConfig::Write(EClassTypes eType, const std::string& stKey, bool bValue)
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to write key %s with value %d", stKey.c_str(), bValue);
		return;
	}

	m_pkIniFile->put(GetClassNameByType(eType), stKey, bValue);
}

std::string CPythonConfig::GetString(EClassTypes eType, const std::string& stKey, const std::string& stDefaultValue) const
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to read key %s", stKey.c_str());
		return stDefaultValue;
	}

	return m_pkIniFile->gets(GetClassNameByType(eType), stKey, stDefaultValue);
}
int CPythonConfig::GetInteger(EClassTypes eType, const std::string& stKey, int iDefaultValue) const
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to read key %s", stKey.c_str());
		return iDefaultValue;
	}

	return m_pkIniFile->geti(GetClassNameByType(eType), stKey, iDefaultValue);
}
float CPythonConfig::GetFloat(EClassTypes eType, const std::string& stKey, float fDefaultValue) const
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to read key %s", stKey.c_str());
		return fDefaultValue;
	}

	return m_pkIniFile->getf(GetClassNameByType(eType), stKey, fDefaultValue);
}
bool CPythonConfig::GetBool(EClassTypes eType, const std::string& stKey, bool bDefaultValue) const
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to read key %s", stKey.c_str());
		return bDefaultValue;
	}

	return m_pkIniFile->getbool(GetClassNameByType(eType), stKey, bDefaultValue);
}

void CPythonConfig::RemoveSection(EClassTypes eType)
{
	if (!m_pkIniFile)
	{
		TraceError("not initialized when trying to remove type %u", eType);
		return;
	}

	m_pkIniFile->del(GetClassNameByType(eType));
}

void CPythonConfig::ReloadBlockNameList()
{
	Tracenf("Reload BlockNameList");

	m_set_BlockNameList.clear();
	LoadBlockNameList(CPythonConfig::Instance().GetBlockNameFileName());

	PyObject* poGameWnd = CPythonNetworkStream::Instance().GetPhaseWindow(CPythonNetworkStream::PHASE_WINDOW_GAME);
	if (poGameWnd)
		PyCallClassMemberFunc(poGameWnd, "BINARY_ReloadBlockList", Py_BuildValue("()"));
}

void CPythonConfig::LoadBlockNameList(const std::string& stFileName)
{
	if (!m_set_BlockNameList.empty())
	{
		TraceError("already initalized BlockNameList reinitialize");
		return;
	}

	m_stBlockNameFileName = stFileName;

	FILE* pf;
	fopen_s(&pf, stFileName.c_str(), "rb");
	if (pf)
	{
		BYTE bNameLen;
		char szNameBuf[CHARACTER_NAME_MAX_LEN + 1];
		while (fread_s(&bNameLen, sizeof(BYTE), sizeof(BYTE), 1, pf))
		{
			if (bNameLen > CHARACTER_NAME_MAX_LEN)
			{
				TraceError("invalid block name list file (invalid name len) - remove");
				m_set_BlockNameList.clear();
				return;
			}

			if (!fread_s(szNameBuf, bNameLen, bNameLen, 1, pf))
			{
				TraceError("invalid block name list file (invalid name) - remove");
				m_set_BlockNameList.clear();
				return;
			}

			szNameBuf[bNameLen] = '\0';
			m_set_BlockNameList.insert(szNameBuf);
		}

		fclose(pf);
	}
}

DWORD CPythonConfig::GetBlockNameCount() const
{
	return m_set_BlockNameList.size();
}

const char* CPythonConfig::GetBlockName(DWORD dwIndex)
{
	int i = 0;
	for (auto it = m_set_BlockNameList.begin(); it != m_set_BlockNameList.end(); ++it, ++i)
	{
		if (i == dwIndex)
			return it->c_str();
	}

	TraceError("invalid block name index %u (out of range, max index %u)", dwIndex, m_set_BlockNameList.size());
	return "";
}

void CPythonConfig::AddBlockName(const std::string& c_rstName)
{
	m_set_BlockNameList.insert(c_rstName);
}

void CPythonConfig::RemoveBlockName(const std::string& c_rstName)
{
	m_set_BlockNameList.erase(c_rstName);
}

bool CPythonConfig::IsBlockName(const std::string& c_rstName)
{
	return m_set_BlockNameList.find(c_rstName) != m_set_BlockNameList.end();
}

void CPythonConfig::SaveBlockNameList()
{
	if (m_stBlockNameFileName.empty())
	{
		TraceError("no block name file name given");
		return;
	}

	FILE* pf;
	fopen_s(&pf, m_stBlockNameFileName.c_str(), "wb");
	if (!pf)
	{
		TraceError("cannot open block name file to save list (%s)", m_stBlockNameFileName.c_str());
		return;
	}

	for (auto it = m_set_BlockNameList.begin(); it != m_set_BlockNameList.end(); ++it)
	{
		BYTE bNameLen = it->length();
		fwrite(&bNameLen, sizeof(BYTE), 1, pf);
		fwrite(it->c_str(), bNameLen, 1, pf);
	}

	fclose(pf);
}

void CPythonConfig::SendReloadBlockListToOthers()
{
	CPythonApplication::Instance().SendMessageToOthers(WM_NEW_RELOAD_BLOCKLIST);
}

float CPythonConfig__GetOptionFloat(const std::string& stKey, float fDefaultValue)
{
	return CPythonConfig::Instance().GetFloat(CPythonConfig::CLASS_OPTION, stKey, fDefaultValue);
}
#endif
