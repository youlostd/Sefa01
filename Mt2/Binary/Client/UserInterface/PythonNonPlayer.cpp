#include "stdafx.h"
#include "../eterPack/EterPackManager.h"
#include "pythonnonplayer.h"
#include "InstanceBase.h"
#include "PythonCharacterManager.h"

bool CPythonNonPlayer::LoadNonPlayerData(const char * c_szFileName)
{
	static DWORD s_adwMobProtoKey[4] =
	{   
		4813894,
		18955,
		552631,
		6822045
	};

	CMappedFile file;
	LPCVOID pvData;

	Tracef("CPythonNonPlayer::LoadNonPlayerData: %s, sizeof(TMobTable)=%u\n", c_szFileName, sizeof(TMobTable));

	if (!CEterPackManager::Instance().Get(file, c_szFileName, &pvData))
		return false;

	DWORD dwFourCC, dwDataSize;

	file.Read(&dwFourCC, sizeof(DWORD));

	if (dwFourCC != MAKEFOURCC('M', 'M', 'P', 'T'))
	{
		TraceError("CPythonNonPlayer::LoadNonPlayerData: invalid Mob proto type %s", c_szFileName);
		return false;
	}

	file.Read(&dwDataSize, sizeof(DWORD));

	BYTE * pbData = new BYTE[dwDataSize];
	file.Read(pbData, dwDataSize);
	/////

	CLZObject zObj;

	if (!CLZO::Instance().Decompress(zObj, pbData, s_adwMobProtoKey))
	{
		delete [] pbData;
		return false;
	}

	network::TRepeatedMobTable mobTables;
	if (!mobTables.ParseFromArray(zObj.GetBuffer(), zObj.GetBufferSize()))
	{
		TraceError("cannot parse mob_proto data");
		delete[] pbData;
		return false;
	}

    for (auto& table : mobTables.data())
	{
		auto pNonPlayerData = std::make_unique<TMobTable>(table);

		//TraceError("%d : %s type[%d] color[%d]", pNonPlayerData->dwVnum, pNonPlayerData->szLocaleName, pNonPlayerData->bType, pNonPlayerData->dwMonsterColor);
		m_NonPlayerDataMap[table.vnum()].mobTable = std::move(pNonPlayerData);
		m_NonPlayerDataMap[table.vnum()].isSet = false;
		m_NonPlayerDataMap[table.vnum()].isFiltered = false;
		m_NonPlayerDataMap[table.vnum()].dropList.clear();
	}

	delete [] pbData;
	return true;
}

DWORD CPythonNonPlayer::GetVnumByNamePart(const char* c_pszName)
{
	if (m_vecWikiNameSort.size() == 0)
		return 0;

	int iMaxLimit = m_vecWikiNameSort.size() - 1;
	int iMinLimit = 0;
	int iIndex = m_vecWikiNameSort.size() / 2;
	TMobTable* pData = NULL;

	std::string stLowerItemName = c_pszName;
	std::transform(stLowerItemName.begin(), stLowerItemName.end(), stLowerItemName.begin(), ::tolower);

	std::string stLowerNameCurrent;

	int iCompareLen = stLowerItemName.length();
	if (!iCompareLen)
		return 0;

	while (true)
	{
		pData = m_vecWikiNameSort[iIndex];
		const char* c_pszNameNow = pData->locale_name(CLocaleManager::Instance().GetLanguage()).c_str();
		stLowerNameCurrent = c_pszNameNow;
		std::transform(stLowerNameCurrent.begin(), stLowerNameCurrent.end(), stLowerNameCurrent.begin(), ::tolower);

		int iRetCompare = stLowerItemName.compare(0, iCompareLen, stLowerNameCurrent, 0, iCompareLen);
		if (iRetCompare != 0)
		{
			if (iRetCompare < 0) // search item name < current item name
				iMaxLimit = iIndex - 1;
			else // search item name > current item name
				iMinLimit = iIndex + 1;

			if (iMinLimit > iMaxLimit)
				return 0;

			iIndex = iMinLimit + (iMaxLimit - iMinLimit) / 2;
			continue;
		}

		int iOldIndex = iIndex;
		int iLastIndex, iLowSize = stLowerNameCurrent.length(), iHighSize = stLowerNameCurrent.length();
		int iLowIndex = iIndex;
		int iHighIndex = iIndex;

		iLastIndex = iIndex;
		while (iLastIndex > 0)
		{
			int iNewIndex = iLastIndex - 1;
			c_pszNameNow = m_vecWikiNameSort[iNewIndex]->locale_name(CLocaleManager::Instance().GetLanguage()).c_str();
			if (strnicmp(c_pszNameNow, stLowerItemName.c_str(), iCompareLen))
				break;

			iLastIndex = iNewIndex;
			if (strlen(c_pszNameNow) <= iLowSize)
			{
				iLowIndex = iNewIndex;
				iLowSize = strlen(c_pszNameNow);
			}
		}

		iLastIndex = iIndex;
		while (iLastIndex < m_vecWikiNameSort.size() - 1 - 1)
		{
			int iNewIndex = iLastIndex + 1;
			c_pszNameNow = m_vecWikiNameSort[iNewIndex]->locale_name(CLocaleManager::Instance().GetLanguage()).c_str();
			if (strnicmp(c_pszNameNow, stLowerItemName.c_str(), iCompareLen))
				break;

			iLastIndex = iNewIndex;
			if (strlen(c_pszNameNow) < iHighSize)
			{
				iHighIndex = iNewIndex;
				iHighSize = strlen(c_pszNameNow);
			}
		}

		if (iHighSize < iLowSize)
			iIndex = iHighIndex;
		else
			iIndex = iLowIndex;

		pData = m_vecWikiNameSort[iIndex];
		return pData->vnum();
	}
}

void CPythonNonPlayer::BuildWikiSearchList()
{
	m_vecWikiNameSort.clear();
	for (auto it = m_NonPlayerDataMap.begin(); it != m_NonPlayerDataMap.end(); ++it)
		if (!it->second.isFiltered)
			m_vecWikiNameSort.push_back(it->second.mobTable.get());

	SortMobDataName();
}

void CPythonNonPlayer::SortMobDataName()
{
	std::qsort(&m_vecWikiNameSort[0], m_vecWikiNameSort.size(), sizeof(m_vecWikiNameSort[0]), [](const void* a, const void* b)
	{
		TMobTable* pItem1 = *(TMobTable**)(static_cast<const TMobTable*>(a));
		TMobTable* pItem2 = *(TMobTable**)(static_cast<const TMobTable*>(b));
		std::string stRealName1 = pItem1->locale_name(CLocaleManager::Instance().GetLanguage());
		std::transform(stRealName1.begin(), stRealName1.end(), stRealName1.begin(), ::tolower);
		std::string stRealName2 = pItem2->locale_name(CLocaleManager::Instance().GetLanguage());
		std::transform(stRealName2.begin(), stRealName2.end(), stRealName2.begin(), ::tolower);

		int iSmallLen = min(stRealName1.length(), stRealName2.length());
		int iRetCompare = stRealName1.compare(0, iSmallLen, stRealName2, 0, iSmallLen);

		if (iRetCompare != 0)
			return iRetCompare;

		if (stRealName1.length() < stRealName2.length())
			return -1;
		else if (stRealName2.length() < stRealName1.length())
			return 1;

		return 0;
	});
}

CPythonNonPlayer::TWikiInfoTable* CPythonNonPlayer::GetWikiTable(DWORD dwVnum)
{
	TNonPlayerDataMap::iterator itor = m_NonPlayerDataMap.find(dwVnum);

	if (itor == m_NonPlayerDataMap.end())
		return NULL;

	return &(itor->second);
}

bool CPythonNonPlayer::GetName(DWORD dwVnum, const char ** c_pszName)
{
	const TMobTable * p = GetTable(dwVnum);

	if (!p)
		return false;

	*c_pszName = p->locale_name(CLocaleManager::Instance().GetLanguage()).c_str();

	return true;
}

bool CPythonNonPlayer::GetInstanceType(DWORD dwVnum, BYTE* pbType)
{
	const TMobTable * p = GetTable(dwVnum);

	// dwVnum를 찾을 수 없으면 플레이어 캐릭터로 간주 한다. 문제성 코드 -_- [cronan]
	if (!p)
		return false;

	*pbType=p->type();
	
	return true;
}

const CPythonNonPlayer::TMobTable * CPythonNonPlayer::GetTable(DWORD dwVnum)
{
	TNonPlayerDataMap::iterator itor = m_NonPlayerDataMap.find(dwVnum);

	if (itor == m_NonPlayerDataMap.end())
		return NULL;

	return itor->second.mobTable.get();
}

BYTE CPythonNonPlayer::GetEventType(DWORD dwVnum)
{
	const TMobTable * p = GetTable(dwVnum);

	if (!p)
	{
		//Tracef("CPythonNonPlayer::GetEventType - Failed to find virtual number\n");
		return ON_CLICK_EVENT_NONE;
	}

	return p->on_click_type();
}

BYTE CPythonNonPlayer::GetEventTypeByVID(DWORD dwVID)
{
	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(dwVID);

	if (NULL == pInstance)
	{
		//Tracef("CPythonNonPlayer::GetEventTypeByVID - There is no Virtual Number\n");
		return ON_CLICK_EVENT_NONE;
	}

	WORD dwVnum = pInstance->GetVirtualNumber();
	return GetEventType(dwVnum);
}

const char*	CPythonNonPlayer::GetMonsterName(DWORD dwVnum)
{	
	const CPythonNonPlayer::TMobTable * c_pTable = GetTable(dwVnum);
	if (!c_pTable)
	{
		static const char* sc_szEmpty="";
		return sc_szEmpty;
	}

	return c_pTable->locale_name(CLocaleManager::Instance().GetLanguage()).c_str();
}

DWORD CPythonNonPlayer::GetMonsterColor(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable * c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->mob_color();
}

void CPythonNonPlayer::GetMatchableMobList(int iLevel, int iInterval, TMobTableList * pMobTableList)
{
/*
	pMobTableList->clear();

	TNonPlayerDataMap::iterator itor = m_NonPlayerDataMap.begin();
	for (; itor != m_NonPlayerDataMap.end(); ++itor)
	{
		TMobTable * pMobTable = itor->second;

		int iLowerLevelLimit = iLevel-iInterval;
		int iUpperLevelLimit = iLevel+iInterval;

		if ((pMobTable->abLevelRange[0] >= iLowerLevelLimit && pMobTable->abLevelRange[0] <= iUpperLevelLimit) ||
			(pMobTable->abLevelRange[1] >= iLowerLevelLimit && pMobTable->abLevelRange[1] <= iUpperLevelLimit))
		{
			pMobTableList->push_back(pMobTable);
		}
	}
*/
}

size_t CPythonNonPlayer::WikiLoadClassMobs(BYTE bType, WORD fromLvl, WORD toLvl)
{
	m_vecTempMob.clear();
	for (auto it = m_NonPlayerDataMap.begin(); it != m_NonPlayerDataMap.end(); ++it)
	{
		if (!it->second.isFiltered && it->second.mobTable->level() >= fromLvl && it->second.mobTable->level() < toLvl)
		{
			if (bType == 0 && it->second.mobTable->type() == MONSTER && it->second.mobTable->rank() >= 4)
				m_vecTempMob.push_back(it->first);
			else if (bType == 1 && it->second.mobTable->type() == MONSTER && it->second.mobTable->rank() < 4)
				m_vecTempMob.push_back(it->first);
			else if (bType == 2 && it->second.mobTable->type() == STONE)
				m_vecTempMob.push_back(it->first);
		}
	}
	return m_vecTempMob.size();
}

void CPythonNonPlayer::WikiSetBlacklisted(DWORD vnum)
{
	auto it = m_NonPlayerDataMap.find(vnum);
	if (it != m_NonPlayerDataMap.end())
		it->second.isFiltered = true;
}

BYTE CPythonNonPlayer::GetMobLevel(DWORD dwVnum)
{
	const CPythonNonPlayer::TMobTable * c_pTable = GetTable(dwVnum);
	if (!c_pTable)
		return 0;

	return c_pTable->level();
}

void CPythonNonPlayer::Clear()
{
}

void CPythonNonPlayer::Destroy()
{
	m_NonPlayerDataMap.clear();
}

CPythonNonPlayer::CPythonNonPlayer()
{
	Clear();
}

CPythonNonPlayer::~CPythonNonPlayer(void)
{
	Destroy();
}
