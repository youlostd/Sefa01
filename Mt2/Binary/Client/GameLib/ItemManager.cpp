#include "StdAfx.h"
#include "../eterPack/EterPackManager.h"
#include "../eterLib/ResourceManager.h"
#include "../UserInterface/PythonSkill.h"

#include "ItemManager.h"

static DWORD s_adwItemProtoKey[4] =
{
	173217,
	72619434,
	408587239,
	27973291
};

BOOL CItemManager::SelectItemData(DWORD dwIndex)
{
	TItemMap::iterator f = m_ItemMap.find(dwIndex);

	if (m_ItemMap.end() == f)
	{
		int n = m_vec_ItemRange.size();
		for (int i = 0; i < n; i++)
		{
			CItemData * p = m_vec_ItemRange[i];
			const CItemData::TItemTable * pTable = p->GetTable(); 
			if ((pTable->vnum() < dwIndex) &&
				dwIndex < (pTable->vnum() + pTable->vnum_range()))
			{
				m_pSelectedItemData = p;
				return TRUE;
			}
		}
		Tracef(" CItemManager::SelectItemData - FIND ERROR [%d]\n", dwIndex);
		return FALSE;
	}

	m_pSelectedItemData = f->second;

	return TRUE;
}

BOOL CItemManager::SelectItemData(CItemData* pItemData)
{
	m_pSelectedItemData = pItemData;
	return m_pSelectedItemData != NULL;
}

CItemData * CItemManager::GetSelectedItemDataPointer()
{
	return m_pSelectedItemData;
}

BOOL CItemManager::GetItemDataPointer(DWORD dwItemID, CItemData ** ppItemData)
{
	if (0 == dwItemID)
		return FALSE;

	TItemMap::iterator f = m_ItemMap.find(dwItemID);

	if (m_ItemMap.end() == f)
	{
		int n = m_vec_ItemRange.size();
		for (int i = 0; i < n; i++)
		{
			CItemData * p = m_vec_ItemRange[i];
			const CItemData::TItemTable * pTable = p->GetTable(); 
			if ((pTable->vnum() < dwItemID) &&
				dwItemID < (pTable->vnum() + pTable->vnum_range()))
			{
				*ppItemData = p;
				return TRUE;
			}
		}
		Tracef(" CItemManager::GetItemDataPointer - FIND ERROR [%d]\n", dwItemID);
		return FALSE;
	}

	*ppItemData = f->second;

	return TRUE;
}

BOOL CItemManager::GetBlendInfoPointer(DWORD dwItemID, TBlendInfo ** ppBlendInfo)
{
	if (0 == dwItemID)
		return FALSE;

	TBlendMap::iterator f = m_BlendMap.find(dwItemID);
	if (f == m_BlendMap.end())
		return FALSE;

	*ppBlendInfo = &f->second;
	return TRUE;
}

BOOL CItemManager::GetBlendInfoPointerByApplyType(BYTE bApplyType, TBlendInfo ** ppBlendInfo)
{
	if (CItemData::APPLY_NONE == bApplyType)
		return FALSE;

	for (auto& elem : m_BlendMap)
	{
		if (elem.second.bApplyType == bApplyType)
		{
			*ppBlendInfo = &elem.second;
			return TRUE;
		}
	}

	return FALSE;
}

CItemData * CItemManager::MakeItemData(DWORD dwIndex)
{
	TItemMap::iterator f = m_ItemMap.find(dwIndex);

	if (m_ItemMap.end() == f)
	{
		CItemData * pItemData = CItemData::New();

		m_ItemMap.insert(TItemMap::value_type(dwIndex, pItemData));

		//SortItemDataName();

		return pItemData;
	}

	return f->second;
}

void CItemManager::SortItemDataName()
{
	std::qsort(&m_vec_ItemData_NameSort[0], m_vec_ItemData_NameSort.size(), sizeof(m_vec_ItemData_NameSort[0]), [](const void* a, const void* b)
	{
		CItemData* pItem1 = *(CItemData**)(static_cast<const CItemData*>(a));
		CItemData* pItem2 = *(CItemData**)(static_cast<const CItemData*>(b));
		const char* c_pszName1 = pItem1->GetName();
		const char* c_pszName2 = pItem2->GetName();

		std::string stRealName1 = c_pszName1;
		std::transform(stRealName1.begin(), stRealName1.end(), stRealName1.begin(), ::tolower);
		std::string stRealName2 = c_pszName2;
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
	std::qsort(&m_vec_wikiItemData_NameSort[0], m_vec_wikiItemData_NameSort.size(), sizeof(m_vec_wikiItemData_NameSort[0]), [](const void* a, const void* b)
	{
		CItemData* pItem1 = *(CItemData**)(static_cast<const CItemData*>(a));
		CItemData* pItem2 = *(CItemData**)(static_cast<const CItemData*>(b));
		const char* c_pszName1 = pItem1->GetName();
		const char* c_pszName2 = pItem2->GetName();

		std::string stRealName1 = c_pszName1;
		std::transform(stRealName1.begin(), stRealName1.end(), stRealName1.begin(), ::tolower);
		std::string stRealName2 = c_pszName2;
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

void CItemManager::AppendSkillNameItems()
{
	{
		const DWORD adwSkillIDs[] = {
			1, 2, 3, 4, 5, 6,
			16, 17, 18, 19, 20, 21,

			31, 32, 33, 34, 35, 36,
			46, 47, 48, 49, 50, 51,

			61, 62, 63, 64, 65, 66,
			76, 77, 78, 79, 80, 81,

			91, 92, 93, 94, 95, 96,
			106, 107, 108, 109, 110, 111,
		};

		CItemData* pBaseData;
		if (GetItemDataPointer(50300, &pBaseData))
		{
			for (int i = 0; i < sizeof(adwSkillIDs) / sizeof(adwSkillIDs[0]); ++i)
			{
				CPythonSkill::TSkillData* pSkillData;
				if (!CPythonSkill::Instance().GetSkillData(adwSkillIDs[i], &pSkillData))
					continue;

				char szNewName[CItemData::ITEM_NAME_MAX_LEN + 1];
				_snprintf(szNewName, sizeof(szNewName), "%s %s", pSkillData->strName.c_str(), pBaseData->GetName());
				
				CItemData* pItemData = CItemData::New();
				pItemData->SetItemTableData(pBaseData->GetTable());
				pItemData->OverwriteName(szNewName);
				pItemData->SetSocket(0, adwSkillIDs[i]);

				m_vec_ItemData_NameSort.push_back(pItemData);
			}
		}
	}

	{
		const DWORD adwSkillIDs[] = {
			7, 8,
			22, 23,

			37, 38,
			52, 53,

			67, 68,
			82, 83,

			97, 98,
			112, 113,
		};

		// CItemData* pBaseData[2];
		// if (GetItemDataPointer(92192, &pBaseData[0]) && GetItemDataPointer(92193, &pBaseData[1]))
		// {
			// for (int i = 0; i < sizeof(adwSkillIDs) / sizeof(adwSkillIDs[0]); ++i)
			// {
				// CItemData* pCurBaseData = pBaseData[i % 2];

				// DWORD dwRealSkillID = adwSkillIDs[i];
				// switch (dwRealSkillID)
				// {
				// case 7:
				// case 8:
					// dwRealSkillID = 2;
					// break;
				// case 22:
				// case 23:
					// dwRealSkillID = 16;
					// break;
				// case 37:
				// case 38:
					// dwRealSkillID = 31;
					// break;
				// case 52:
				// case 53:
					// dwRealSkillID = 48;
					// break;
				// case 67:
				// case 68:
					// dwRealSkillID = 61;
					// break;
				// case 82:
				// case 83:
					// dwRealSkillID = 76;
					// break;
				// case 97:
				// case 98:
					// dwRealSkillID = 92;
					// break;
				// case 112:
				// case 113:
					// dwRealSkillID = 107;
					// break;
				// }

				// CPythonSkill::TSkillData* pSkillData;
				// if (!CPythonSkill::Instance().GetSkillData(dwRealSkillID, &pSkillData))
					// continue;

				// char szNewName[CItemData::ITEM_NAME_MAX_LEN + 1];
				// _snprintf(szNewName, sizeof(szNewName), "%s %s", pSkillData->strName.c_str(), pCurBaseData->GetName());

				// CItemData* pItemData = CItemData::New();
				// pItemData->SetItemTableData(pCurBaseData->GetTable());
				// pItemData->OverwriteName(szNewName);
				// pItemData->SetSocket(0, adwSkillIDs[i]);

				// m_vec_ItemData_NameSort.push_back(pItemData);
			// }
		// }
	}

	SortItemDataName();
}

CItemData* CItemManager::FindItemDataByName(const char* c_pszName, bool isWiki)
{
	std::vector<CItemData*>* selectVec;
	if (isWiki)
		selectVec = &m_vec_wikiItemData_NameSort;
	else
		selectVec = &m_vec_ItemData_NameSort;
	if ((*selectVec).size() == 0)
		return NULL;

	int iMaxLimit = (*selectVec).size() - 1;
	int iMinLimit = 0;
	int iIndex = (*selectVec).size() / 2;
	CItemData* pData = NULL;

	std::string stLowerItemName = c_pszName;
	std::transform(stLowerItemName.begin(), stLowerItemName.end(), stLowerItemName.begin(), ::tolower);

	std::string stLowerNameCurrent;

	int iCompareLen = stLowerItemName.length();
	if (!iCompareLen)
		return NULL;

	while (1)
	{
		pData = (*selectVec)[iIndex];
		const char* c_pszNameNow = pData->GetName();
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
				return NULL;

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
			c_pszNameNow = (*selectVec)[iNewIndex]->GetName();
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
		while (iLastIndex < (*selectVec).size() - 1 - 1)
		{
			int iNewIndex = iLastIndex + 1;
			c_pszNameNow = (*selectVec)[iNewIndex]->GetName();
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

		pData = (*selectVec)[iIndex];
		return pData;
	}
}

void CItemManager::CreateSortedItemData()
{
	m_vecItemDataSorted.clear();

	for (auto& it : m_ItemMap)
	{
		m_vecItemDataSorted.push_back(it.second);
	}

	std::qsort(&m_vecItemDataSorted[0], m_vecItemDataSorted.size(), sizeof(CItemData*), [](const void* first, const void* second) {
		CItemData* a = (CItemData*) *((const CItemData**)first);
		CItemData* b = (CItemData*) *((const CItemData**)second);

		return stricmp(a->GetName(), b->GetName());
	});
}

CItemManager::TItemRange FindItemNameRange(CItemManager::TItemRange itRange, char charValue, int charIndex)
{
	int dist = std::distance(itRange.first, itRange.second);
	charValue = tolower(charValue);

	const int alphabetMaxNum = 'z' - 'a' + 1; // +1 because starts with idx 0
	int alphabetIdx = MAX(0, MIN(alphabetMaxNum - 1, charValue - 'a'));
	int startIterIdx = dist * alphabetIdx / alphabetMaxNum;

	// find first iter
	CItemManager::TItemVector::const_iterator itCur = itRange.first + startIterIdx;
	CItemManager::TItemRange tmpRange;
	tmpRange.first = itRange.first;
	tmpRange.second = itRange.second;

	while (itCur != tmpRange.second)
	{
		CItemData* pCurItem = *itCur;
		char currentChar = strlen(pCurItem->GetName()) > charIndex ? tolower(pCurItem->GetName()[charIndex]) : 0;
		
		// current character is smaller than searched value
		if (currentChar < charValue)
		{
			itRange.first = itCur;
			tmpRange.first = itCur;
			itCur += MAX(1, std::distance(itCur, tmpRange.second) / 2);
		}
		// current character is equal or larger than searched value
		else if (itCur != tmpRange.first)
		{
			if (
					// current character is equal but the one before this is equal as well
					(
						currentChar == charValue &&
						strlen((*(itCur - 1))->GetName()) > charIndex &&
						tolower((*(itCur - 1))->GetName()[charIndex]) == charValue
					)
					// current character is larger than the searched value
					|| (currentChar > charValue)
				)
			{
				if (currentChar > charValue)
					itRange.second = itCur;
				tmpRange.second = itCur;
				itCur -= MAX(1, std::distance(tmpRange.first, itCur) / 2);
			}
			// current character is equal and the one before this is not equal
			else
			{
				break;
			}
		}
		else if (currentChar == charValue)
		{
			break;
		}
		// current character is larger and the iterator reached begin of vector -> no results found -> set to end iterator
		else
		{
			itCur = itRange.second;
		}
	}

	itRange.first = itCur;

	// find last iter if first iter was found
	while (itCur != itRange.second)
	{
		CItemData* pCurItem = *itCur;

		if (strlen(pCurItem->GetName()) < charIndex || tolower(pCurItem->GetName()[charIndex]) != charValue)
		{
			itRange.second = itCur;
			break;
		}

		itCur++;
	}

	return itRange;
}

CItemManager::TItemRange CItemManager::GetProtosByName(const char* name) const
{
	auto it_start = m_vecItemDataSorted.begin();
	auto it_end = m_vecItemDataSorted.end();

	int nameLen = strlen(name);
	for (int i = 0; i < nameLen; ++i)
	{
		TItemRange ret = FindItemNameRange(TItemRange(it_start, it_end), name[i], i);
		it_start = ret.first;
		it_end = ret.second;

		if (it_start == it_end)
			break;
	}

	return TItemRange(it_start, it_end);
}

////////////////////////////////////////////////////////////////////////////////////////
// Load Item Table

bool CItemManager::LoadItemList(const char * c_szFileName)
{
	CMappedFile File;
	LPCVOID pData;

	if (!CEterPackManager::Instance().Get(File, c_szFileName, &pData))
		return false;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(File.Size(), pData);

	CTokenVector TokenVector;
    for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLine(i, &TokenVector, "\t"))
			continue;

		if (!(TokenVector.size() == 3 || TokenVector.size() == 4))
		{
			TraceError(" CItemManager::LoadItemList(%s) - StrangeLine in %d\n", c_szFileName, i);
			continue;
		}

		const std::string & c_rstrID = TokenVector[0];
		//const std::string & c_rstrType = TokenVector[1];
		const std::string & c_rstrIcon = TokenVector[2];

		DWORD dwItemVNum=atoi(c_rstrID.c_str());

		CItemData * pItemData = MakeItemData(dwItemVNum);

		extern BOOL USE_VIETNAM_CONVERT_WEAPON_VNUM;
		if (USE_VIETNAM_CONVERT_WEAPON_VNUM)
		{
			extern DWORD Vietnam_ConvertWeaponVnum(DWORD vnum);
			DWORD dwMildItemVnum = Vietnam_ConvertWeaponVnum(dwItemVNum);
			if (dwMildItemVnum == dwItemVNum)
			{
				if (4 == TokenVector.size())
				{
					const std::string & c_rstrModelFileName = TokenVector[3];
					pItemData->SetDefaultItemData(c_rstrIcon.c_str(), c_rstrModelFileName.c_str());
				}
				else
				{
					pItemData->SetDefaultItemData(c_rstrIcon.c_str());
				}
			}
			else
			{
				DWORD dwMildBaseVnum = dwMildItemVnum / 10 * 10;
				char szMildIconPath[MAX_PATH];				
				sprintf(szMildIconPath, "icon/item/%.5d.tga", dwMildBaseVnum);
				if (4 == TokenVector.size())
				{
					char szMildModelPath[MAX_PATH];
					sprintf(szMildModelPath, "d:/ymir work/item/weapon/%.5d.gr2", dwMildBaseVnum);	
					pItemData->SetDefaultItemData(szMildIconPath, szMildModelPath);
				}
				else
				{
					pItemData->SetDefaultItemData(szMildIconPath);
				}
			}
		}
		else
		{
			if (4 == TokenVector.size())
			{
				const std::string & c_rstrModelFileName = TokenVector[3];
				pItemData->SetDefaultItemData(c_rstrIcon.c_str(), c_rstrModelFileName.c_str());
			}
			else
			{
				pItemData->SetDefaultItemData(c_rstrIcon.c_str());
			}
		}
	}

	return true;
}

const std::string& __SnapString(const std::string& c_rstSrc, std::string& rstTemp)
{
	UINT uSrcLen=c_rstSrc.length();
	if (uSrcLen<2)
		return c_rstSrc;

	if (c_rstSrc[0]!='"')
		return c_rstSrc;

	UINT uLeftCut=1;
	
	UINT uRightCut=uSrcLen;
	if (c_rstSrc[uSrcLen-1]=='"')
		uRightCut=uSrcLen-1;	

	rstTemp=c_rstSrc.substr(uLeftCut, uRightCut-uLeftCut);
	return rstTemp;
}

#define ATTR_INFO std::pair<const char*, BYTE>
BYTE CItemManager::GetAttrTypeByName(const std::string& stAttrType)
{
	if (stAttrType == "")
		return CItemData::APPLY_NONE;

	static const ATTR_INFO astAttrInfo[] = {
		ATTR_INFO("NONE", CItemData::APPLY_NONE),
		ATTR_INFO("MAX_HP", CItemData::APPLY_MAX_HP),
		ATTR_INFO("MAX_SP", CItemData::APPLY_MAX_SP),
		ATTR_INFO("CON", CItemData::APPLY_CON),
		ATTR_INFO("STR", CItemData::APPLY_STR),
		ATTR_INFO("INT", CItemData::APPLY_INT),
		ATTR_INFO("DEX", CItemData::APPLY_DEX),
		ATTR_INFO("ATT_SPEED", CItemData::APPLY_ATT_SPEED),
		ATTR_INFO("ATTACK_SPEED", CItemData::APPLY_ATT_SPEED),
		ATTR_INFO("MOV_SPEED", CItemData::APPLY_MOV_SPEED),
		ATTR_INFO("STUN_PCT", CItemData::APPLY_STUN_PCT),
		ATTR_INFO("SLOW_PCT", CItemData::APPLY_SLOW_PCT),
		ATTR_INFO("MAGIC_ATT_GRADE", CItemData::APPLY_MAGIC_ATT_GRADE),
		ATTR_INFO("MAGIC_DEF_GRADE", CItemData::APPLY_MAGIC_DEF_GRADE),
		ATTR_INFO("EXP_REAL_BONUS", CItemData::APPLY_EXP_REAL_BONUS),
		ATTR_INFO("CRITICAL_PCT", CItemData::APPLY_CRITICAL_PCT),
		ATTR_INFO("PENETRATE_PCT", CItemData::APPLY_PENETRATE_PCT),
		ATTR_INFO("POISON_PCT", CItemData::APPLY_POISON_PCT),
		ATTR_INFO("BLOCK", CItemData::APPLY_BLOCK),
		ATTR_INFO("DODGE", CItemData::APPLY_DODGE),
		ATTR_INFO("RESIST_MAGIC", CItemData::APPLY_RESIST_MAGIC),
		ATTR_INFO("ITEM_DROP_BONUS", CItemData::APPLY_ITEM_DROP_BONUS),
		ATTR_INFO("ATT_BONUS", CItemData::APPLY_ATT_GRADE_BONUS),
		ATTR_INFO("ATT_GRADE_BONUS", CItemData::APPLY_ATT_GRADE_BONUS),
		ATTR_INFO("ATTBONUS_HUMAN", CItemData::APPLY_ATTBONUS_HUMAN),
		ATTR_INFO("ATTBONUS_MONSTER", CItemData::APPLY_ATT_BONUS_TO_MONSTER),
		ATTR_INFO("DEF_BONUS", CItemData::APPLY_DEF_GRADE_BONUS),
		ATTR_INFO("DEF_GRADE_BONUS", CItemData::APPLY_DEF_GRADE_BONUS),
		ATTR_INFO("NORMAL_HIT_DAMAGE_BONUS", CItemData::APPLY_NORMAL_HIT_DAMAGE_BONUS),
		ATTR_INFO("NORMAL_HIT_DEFEND_BONUS", CItemData::APPLY_NORMAL_HIT_DEFEND_BONUS),
		ATTR_INFO("SKILL_DAMAGE_BONUS", CItemData::APPLY_SKILL_DAMAGE_BONUS),
		ATTR_INFO("SKILL_DEFEND_BONUS", CItemData::APPLY_SKILL_DEFEND_BONUS),
		ATTR_INFO("ANTI_PENETRATE_PCT", CItemData::APPLY_ANTI_PENETRATE_PCT),
		ATTR_INFO("ATTBONUS_METIN", CItemData::APPLY_ATTBONUS_METIN),
		ATTR_INFO("ATTBONUS_UNDEAD", CItemData::APPLY_ATTBONUS_UNDEAD),
		ATTR_INFO("CAST_SPEED", CItemData::APPLY_CAST_SPEED),
		ATTR_INFO("ENERGY", CItemData::APPLY_ENERGY),
		ATTR_INFO("ATTBONUS_ANIMAL", CItemData::APPLY_ATTBONUS_ANIMAL),
		ATTR_INFO("ATTBONUS_ORC", CItemData::APPLY_ATTBONUS_ORC),
#ifdef ENABLE_ZODIAC
		ATTR_INFO("ATTBONUS_ZODIAC", CItemData::APPLY_ATTBONUS_ZODIAC),
#endif
		ATTR_INFO("ATTBONUS_MILGYO", CItemData::APPLY_ATTBONUS_MILGYO),
		ATTR_INFO("ATTBONUS_DEVIL", CItemData::APPLY_ATTBONUS_DEVIL),
		ATTR_INFO("RESIST_ICE", CItemData::APPLY_RESIST_ICE),
		ATTR_INFO("RESIST_EARTH", CItemData::APPLY_RESIST_EARTH),
		ATTR_INFO("RESIST_DARK", CItemData::APPLY_RESIST_DARK),
		ATTR_INFO("RESIST_FIRE", CItemData::APPLY_RESIST_FIRE),
		ATTR_INFO("RESIST_ELEC", CItemData::APPLY_RESIST_ELEC),
		ATTR_INFO("RESIST_WIND", CItemData::APPLY_RESIST_WIND),
		ATTR_INFO("ANTI_CRITICAL_PCT", CItemData::APPLY_ANTI_CRITICAL_PCT),
		ATTR_INFO("REFLECT_MELEE", CItemData::APPLY_REFLECT_MELEE),
		ATTR_INFO("HP_REGEN", CItemData::APPLY_HP_REGEN),
		ATTR_INFO("SP_REGEN", CItemData::APPLY_SP_REGEN),
		ATTR_INFO("STEAL_HP", CItemData::APPLY_STEAL_HP),
		ATTR_INFO("STEAL_SP", CItemData::APPLY_STEAL_SP),
		ATTR_INFO("POTION_BONUS", CItemData::APPLY_POTION_BONUS),
		ATTR_INFO("MALL_ATTBONUS", CItemData::APPLY_MALL_ATTBONUS),
		ATTR_INFO("MALL_DEFBONUS", CItemData::APPLY_MALL_DEFBONUS),
		ATTR_INFO("MAX_HP_PCT", CItemData::APPLY_MAX_HP_PCT),
		ATTR_INFO("MAX_SP_PCT", CItemData::APPLY_MAX_SP_PCT),
		ATTR_INFO("ATTBONUS_BOSS", CItemData::APPLY_ATTBONUS_BOSS),
		ATTR_INFO("IMMUNE_STUN", CItemData::APPLY_IMMUNE_STUN),
		ATTR_INFO("IMMUNE_FALL", CItemData::APPLY_IMMUNE_FALL),
		ATTR_INFO("EXP_REAL_BONUS", CItemData::APPLY_EXP_REAL_BONUS),
		ATTR_INFO("GOLD_DOUBLE_BONUS", CItemData::APPLY_GOLD_DOUBLE_BONUS),
		ATTR_INFO("POISON_REDUCE", CItemData::APPLY_POISON_REDUCE),
		ATTR_INFO("DEF_GRADE", CItemData::APPLY_DEF_GRADE),
		ATTR_INFO("ATTBONUS_WARRIOR", CItemData::APPLY_ATT_BONUS_TO_WARRIOR),
		ATTR_INFO("ATTBONUS_ASSASSIN", CItemData::APPLY_ATT_BONUS_TO_ASSASSIN),
		ATTR_INFO("ATTBONUS_SURA", CItemData::APPLY_ATT_BONUS_TO_SURA),
		ATTR_INFO("ATTBONUS_SHAMAN", CItemData::APPLY_ATT_BONUS_TO_SHAMAN),
		ATTR_INFO("RESIST_WARRIOR", CItemData::APPLY_RESIST_WARRIOR),
		ATTR_INFO("RESIST_ASSASSIN", CItemData::APPLY_RESIST_ASSASSIN),
		ATTR_INFO("RESIST_SURA", CItemData::APPLY_RESIST_SURA),
		ATTR_INFO("RESIST_SHAMAN", CItemData::APPLY_RESIST_SHAMAN),
		ATTR_INFO("EXP_DOUBLE_BONUS", CItemData::APPLY_EXP_DOUBLE_BONUS),
		ATTR_INFO("RESIST_SWORD", CItemData::APPLY_RESIST_SWORD),
		ATTR_INFO("RESIST_TWOHAND", CItemData::APPLY_RESIST_TWOHAND),
		ATTR_INFO("RESIST_DAGGER", CItemData::APPLY_RESIST_DAGGER),
		ATTR_INFO("RESIST_BOW", CItemData::APPLY_RESIST_BOW),
		ATTR_INFO("RESIST_BELL", CItemData::APPLY_RESIST_BELL),
		ATTR_INFO("RESIST_FAN", CItemData::APPLY_RESIST_FAN),
		ATTR_INFO("MANA_BURN_PCT", CItemData::APPLY_MANA_BURN_PCT),

		ATTR_INFO("RESIST_SWORD_PEN", CItemData::APPLY_RESIST_SWORD_PEN),
		ATTR_INFO("RESIST_TWOHAND_PEN", CItemData::APPLY_RESIST_TWOHAND_PEN),
		ATTR_INFO("RESIST_DAGGER_PEN", CItemData::APPLY_RESIST_DAGGER_PEN),
		ATTR_INFO("RESIST_BELL_PEN", CItemData::APPLY_RESIST_BELL_PEN),
		ATTR_INFO("RESIST_FAN_PEN", CItemData::APPLY_RESIST_FAN_PEN),
		ATTR_INFO("RESIST_BOW_PEN", CItemData::APPLY_RESIST_BOW_PEN),
		ATTR_INFO("RESIST_ATTBONUS_HUMAN", CItemData::APPLY_RESIST_ATTBONUS_HUMAN),

		ATTR_INFO("DEFENSE_BONUS", CItemData::APPLY_DEFENSE_BONUS),
		
		ATTR_INFO("ATT_BONUS_TO_WARRIOR", CItemData::APPLY_ATT_BONUS_TO_WARRIOR),
		ATTR_INFO("ATT_BONUS_TO_ASSASSIN", CItemData::APPLY_ATT_BONUS_TO_ASSASSIN),
		ATTR_INFO("ATT_BONUS_TO_SURA", CItemData::APPLY_ATT_BONUS_TO_SURA),
		ATTR_INFO("ATT_BONUS_TO_SHAMAN", CItemData::APPLY_ATT_BONUS_TO_SHAMAN),
		ATTR_INFO("APPLY_ANTI_CRITICAL_PCT", CItemData::APPLY_ANTI_CRITICAL_PCT),
		ATTR_INFO("APPLY_ANTI_PENETRATE_PCT", CItemData::APPLY_ANTI_PENETRATE_PCT),
		ATTR_INFO("APPLY_RESIST_HUMAN", CItemData::APPLY_RESIST_HUMAN),
		ATTR_INFO("APPLY_ATTBONUS_METIN", CItemData::APPLY_ATTBONUS_METIN),
		ATTR_INFO("APPLY_ATTBONUS_BOSS", CItemData::APPLY_ATTBONUS_BOSS),
		ATTR_INFO("ATT_BONUS_TO_MONSTER", CItemData::APPLY_ATT_BONUS_TO_MONSTER),
		ATTR_INFO("APPLY_RESIST_MONSTER", CItemData::APPLY_RESIST_MONSTER),
		ATTR_INFO("APPLY_BLOCK_IGNORE_BONUS", CItemData::APPLY_BLOCK_IGNORE_BONUS),
		ATTR_INFO("RUNE_3RD_ATTACK_BONUS", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_BONUS_DAMAGE_AFTER_HIT", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_FIRST_NORMAL_HIT_BONUS", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_DAMAGE_AFTER_3", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_OUT_OF_COMBAT_SPEED", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_HARVEST", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_SHIELD_PER_HIT", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_RESET_SKILL", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_MOVSPEED_AFTER_3", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_MAGIC_DAMAGE_AFTER_HIT", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_HEAL_ON_KILL", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_MSHIELD_PER_SKILL", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_COMBAT_CASTING_SPEED", CItemData::APPLY_NONE),
		ATTR_INFO("RUNE_SLOW_ON_ATTACK", CItemData::APPLY_NONE), 
		ATTR_INFO("POINT_RUNE_SLOW_ON_ATTACK", CItemData::APPLY_NONE), 
		ATTR_INFO("HEAL_EFFECT_BONUS", CItemData::APPLY_HEAL_EFFECT_BONUS),
		ATTR_INFO("CRITICAL_DAMAGE_BONUS", CItemData::APPLY_CRITICAL_DAMAGE_BONUS),
		ATTR_INFO("DOUBLE_ITEM_DROP_BONUS", CItemData::APPLY_DOUBLE_ITEM_DROP_BONUS),
		ATTR_INFO("DAMAGE_BY_SP_BONUS", CItemData::APPLY_DAMAGE_BY_SP_BONUS),
		ATTR_INFO("SINGLETARGET_SKILL_DAMAGE_BONUS", CItemData::APPLY_SINGLETARGET_SKILL_DAMAGE_BONUS),
		ATTR_INFO("MULTITARGET_SKILL_DAMAGE_BONUS", CItemData::APPLY_MULTITARGET_SKILL_DAMAGE_BONUS),
		ATTR_INFO("MIXED_DEFEND_BONUS", CItemData::APPLY_MIXED_DEFEND_BONUS),
		ATTR_INFO("EQUIP_SKILL_BONUS", CItemData::APPLY_EQUIP_SKILL_BONUS),
		ATTR_INFO("AURA_HEAL_EFFECT_BONUS", CItemData::APPLY_AURA_HEAL_EFFECT_BONUS),
		ATTR_INFO("AURA_EQUIP_SKILL_BONUS", CItemData::APPLY_AURA_EQUIP_SKILL_BONUS),
		ATTR_INFO("BOW_DISTANCE", CItemData::APPLY_BOW_DISTANCE),
		ATTR_INFO("MOUNT_BUFF_BONUS", CItemData::APPLY_MOUNT_BUFF_BONUS),
		ATTR_INFO("RUNE_LEADERSHIP_BONUS", CItemData::APPLY_NONE),
#ifdef ENABLE_RUNE_SYSTEM
		ATTR_INFO("RUNE_MOUNT_PARALYZE", CItemData::APPLY_RUNE_MOUNT_PARALYZE),
#else
		ATTR_INFO("RUNE_MOUNT_PARALYZE", CItemData::APPLY_NONE),
#endif
		ATTR_INFO("APPLY_POTION_BONUS", CItemData::APPLY_POTION_BONUS),
#ifdef ENABLE_ZODIAC
		ATTR_INFO("APPLY_ATTBONUS_ZODIAC", CItemData::APPLY_ATTBONUS_ZODIAC),
#else
		ATTR_INFO("APPLY_ATTBONUS_ZODIAC", CItemData::APPLY_NONE),
#endif
#ifdef STANDARD_SKILL_DURATION
		ATTR_INFO("APPLY_SKILL_DURATION", CItemData::APPLY_SKILL_DURATION),
#else
		ATTR_INFO("APPLY_SKILL_DURATION", CItemData::APPLY_NONE),
#endif
	};

	for (int i = 0; i < sizeof(astAttrInfo) / sizeof(ATTR_INFO); ++i)
	{
		if (!stricmp(stAttrType.c_str(), astAttrInfo[i].first))
			return astAttrInfo[i].second;
	}

	TraceError("GetAttrTypeByName(%s) - cannot get attribute type", stAttrType.c_str());
	return CItemData::APPLY_NONE;
}
#undef ATTR_INFO

bool CItemManager::LoadItemDesc(const char* c_szFileName)
{
	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, c_szFileName, &pvData))
	{
		Tracenf("CItemManager::LoadItemDesc(c_szFileName=%s) - Load Error", c_szFileName);
		return false;
	}

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(kFile.Size(), pvData);

	std::string stTemp;

	CTokenVector kTokenVector;
	for (DWORD i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &kTokenVector))
			continue;

		while (kTokenVector.size()<ITEMDESC_COL_NUM)
			kTokenVector.push_back("");

		//assert(kTokenVector.size()==ITEMDESC_COL_NUM);
		
		DWORD dwVnum=atoi(kTokenVector[ITEMDESC_COL_VNUM].c_str());
		const std::string& c_rstDesc=kTokenVector[ITEMDESC_COL_DESC];
		const std::string& c_rstSumm=kTokenVector[ITEMDESC_COL_SUMM];
		TItemMap::iterator f = m_ItemMap.find(dwVnum);
		if (m_ItemMap.end() == f)
			continue;

		CItemData* pkItemDataFind = f->second;

		pkItemDataFind->SetDescription(__SnapString(c_rstDesc, stTemp));
		pkItemDataFind->SetSummary(__SnapString(c_rstSumm, stTemp));
	}
	return true;
}

DWORD GetHashCode( const char* pString )
{
	   unsigned long i,len;
	   unsigned long ch;
	   unsigned long result;
	   
	   len     = strlen( pString );
	   result = 5381;
	   for( i=0; i<len; i++ )
	   {
	   	   ch = (unsigned long)pString[i];
	   	   result = ((result<< 5) + result) + ch; // hash * 33 + ch
	   }	  

	   return result;
}

bool CItemManager::LoadItemTable(const char* c_szFileName)
{	
	CMappedFile file;
	LPCVOID pvData;

	if (!CEterPackManager::Instance().Get(file, c_szFileName, &pvData))
	{
		TraceError("cannot find item_proto %s", c_szFileName);
		return false;
	}

	DWORD dwFourCC, dwDataSize;

	file.Read(&dwFourCC, sizeof(DWORD));

	if (dwFourCC != MAKEFOURCC('M', 'I', 'P', 'X'))
	{
		TraceError("CPythonItem::LoadItemTable: invalid item proto type %s", c_szFileName);
		return false;
	}

	file.Read(&dwDataSize, sizeof(DWORD));

	BYTE * pbData = new BYTE[dwDataSize];
	file.Read(pbData, dwDataSize);

	/////

	CLZObject zObj;

	if (!CLZO::Instance().Decompress(zObj, pbData, s_adwItemProtoKey))
	{
		TraceError("cannot decompress item_proto");
		delete [] pbData;
		return false;
	}

	/////

	network::TRepeatedItemTable itemTables;
	if (!itemTables.ParseFromArray(zObj.GetBuffer(), zObj.GetBufferSize()))
	{
		TraceError("cannot parse item_proto data");
		delete [] pbData;
		return false;
	}

	char szName[64 + 1];
	std::map<DWORD,DWORD> itemNameMap;

	std::set<std::string> setItemNameSort;
	m_vec_ItemData_NameSort.clear();
	m_vec_wikiItemData_NameSort.clear();

	for (auto& table : itemTables.data())
	{
		CItemData * pItemData;
		DWORD dwVnum = table.vnum();

		TItemMap::iterator f = m_ItemMap.find(dwVnum);
		if (m_ItemMap.end() == f)
		{
			_snprintf(szName, sizeof(szName), "icon/item/%05d.tga", dwVnum);
#ifdef INGAME_WIKI
			pItemData = CItemData::New();
#endif
			if (CResourceManager::Instance().IsFileExist(szName) == false)
			{
				std::map<DWORD, DWORD>::iterator itVnum = itemNameMap.find(GetHashCode(table.name().c_str()));
				
				if (itVnum != itemNameMap.end())
					_snprintf(szName, sizeof(szName), "icon/item/%05d.tga", itVnum->second);
				else
					_snprintf(szName, sizeof(szName), "icon/item/%05d.tga", dwVnum-dwVnum % 10);

				if (CResourceManager::Instance().IsFileExist(szName) == false)
				{
					bool bExists = false;

					if (itVnum != itemNameMap.end())
					{
						_snprintf(szName, sizeof(szName), "icon/item/%05d.tga", dwVnum - dwVnum % 10);
						bExists = CResourceManager::Instance().IsFileExist(szName);
					}

					if (!bExists)
					{
#ifdef INGAME_WIKI
						pItemData->ValidateImage(false);
#endif
#ifdef _DEBUG
						TraceError("%16s(#%-5d) cannot find icon file. setting to default.", table->szName, dwVnum);
#endif
						const DWORD EmptyBowl = 27995;
						_snprintf(szName, sizeof(szName), "icon/item/%05d.tga", EmptyBowl);
					}
				}
			}
				
#ifndef INGAME_WIKI
			pItemData = CItemData::New();
#endif

			pItemData->SetDefaultItemData(szName);
			m_ItemMap.insert(TItemMap::value_type(dwVnum, pItemData));
#ifdef INGAME_WIKI
			pItemData->SetItemTableData(&table);
			if (!CResourceManager::Instance().IsFileExist(pItemData->GetIconFileName().c_str()))
				pItemData->ValidateImage(false);
#endif
		}
		else
		{
			pItemData = f->second;
#ifdef INGAME_WIKI
			pItemData->SetItemTableData(&table);
#endif
		}
		static auto i = 0;
		if (itemNameMap.find(GetHashCode(table.name().c_str())) == itemNameMap.end())
			itemNameMap.insert(std::map<DWORD,DWORD>::value_type(GetHashCode(table.name().c_str()),table.vnum()));
#ifndef INGAME_WIKI
		pItemData->SetItemTableData(table);
#endif
		if (setItemNameSort.find(pItemData->GetName()) == setItemNameSort.end())
		{
			setItemNameSort.insert(pItemData->GetName());
			m_vec_ItemData_NameSort.push_back(pItemData);
			if ((pItemData->GetType() == CItemData::ITEM_TYPE_ARMOR || pItemData->GetType() == CItemData::ITEM_TYPE_WEAPON) && dwVnum % 10 == 9)
				m_vec_wikiItemData_NameSort.push_back(pItemData);
		}
		if (0 != table.vnum_range())
		{
			m_vec_ItemRange.push_back(pItemData);
		}
	}

	SortItemDataName();

	CreateSortedItemData();

//!@#
//	CItemData::TItemTable * table = (CItemData::TItemTable *) zObj.GetBuffer();
//	for (DWORD i = 0; i < dwElements; ++i, ++table)
//	{
//		CItemData * pItemData;
//		DWORD dwVnum = table->dwVnum;
//
//		TItemMap::iterator f = m_ItemMap.find(dwVnum);
//
//		if (m_ItemMap.end() == f)
//		{
//			pItemData = CItemData::New();
//
//			pItemData->LoadItemData(_getf("d:/ymir work/item/%05d.msm", dwVnum));
//			m_ItemMap.insert(TItemMap::value_type(dwVnum, pItemData));
//		}
//		else
//		{
//			pItemData = f->second;
//		}
//		pItemData->SetItemTableData(table);
//	}

	delete [] pbData;
	return true;
}

bool CItemManager::LoadItemBlend(const char* c_szFileName)
{
	CMappedFile File;
	LPCVOID pData;

	if (!CEterPackManager::Instance().Get(File, c_szFileName, &pData))
		return false;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(File.Size(), pData);

	TBlendInfo kBlendInfo;
	BYTE bIndex = 0;

	CTokenVector TokenVector;
	for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLine(i, &TokenVector, "\t"))
			continue;

		switch (bIndex)
		{
			case 0:
			{
				if (TokenVector.size() >= 1 && TokenVector[0].find_first_of("section") == 0)
				{
					kBlendInfo.dwVnum = 0;
					kBlendInfo.bApplyType = 0;
					kBlendInfo.vec_iApplyValue.clear();
					kBlendInfo.vec_iApplyDuration.clear();
					++bIndex;
				}
			}
			break;

			case 1:
			{
				if (TokenVector.size() >= 2)
				{
					if (TokenVector[0].find_first_of("item_vnum") != 0)
					{
						TraceError("invalid section line [item_vnum] %u", i);
						bIndex = 0;
						break;
					}

					kBlendInfo.dwVnum = atoi(TokenVector[1].c_str());
					++bIndex;
				}
			}
			break;

			case 2:
			{
				if (TokenVector.size() >= 2)
				{
					if (TokenVector[0].find_first_of("apply_type") != 0)
					{
						TraceError("invalid section line [apply_type] %u", i);
						bIndex = 0;
						break;
					}

					kBlendInfo.bApplyType = GetAttrTypeByName(TokenVector[1]);
					++bIndex;
				}
			}
			break;

			case 3:
			{
				if (TokenVector.size() >= 2)
				{
					if (TokenVector[0].find_first_of("apply_value") != 0)
					{
						TraceError("invalid section line [apply_value] %u", i);
						bIndex = 0;
						break;
					}

					for (int j = 1; j < TokenVector.size(); ++j)
					{
						kBlendInfo.vec_iApplyValue.push_back(atoi(TokenVector[j].c_str()));
					}
					++bIndex;
				}
			}
			break;

			case 4:
			{
				if (TokenVector.size() >= 2)
				{
					if (TokenVector[0].find_first_of("apply_duration") != 0)
					{
						TraceError("invalid section line [apply_duration] %u", i);
						bIndex = 0;
						break;
					}

					for (int j = 1; j < TokenVector.size(); ++j)
					{
						kBlendInfo.vec_iApplyDuration.push_back(atoi(TokenVector[j].c_str()));
					}
					++bIndex;
				}
			}
			break;

			case 5:
			{
				if (TokenVector.size() >= 1 && TokenVector[0].find_first_of("end") == 0)
				{
					bIndex = 0;

					if (kBlendInfo.dwVnum == 0)
					{
						TraceError("invalid vnum for blend line %u vnum 0", i);
						break;
					}

					if (kBlendInfo.bApplyType == 0)
					{
						TraceError("invalid apply type for blend vnum %u", kBlendInfo.dwVnum);
						break;
					}

					if (kBlendInfo.vec_iApplyValue.size() != kBlendInfo.vec_iApplyDuration.size())
					{
						TraceError("invalid size compare of iApplyValue and iApplyDuration vnum %u", kBlendInfo.dwVnum);
						break;
					}

					TBlendInfo& rkInfo = m_BlendMap[kBlendInfo.dwVnum];
					rkInfo.dwVnum = kBlendInfo.dwVnum;
					rkInfo.bApplyType = kBlendInfo.bApplyType;
					for (int j = 0; j < kBlendInfo.vec_iApplyValue.size(); ++j)
					{
						rkInfo.vec_iApplyValue.push_back(kBlendInfo.vec_iApplyValue[j]);
						rkInfo.vec_iApplyDuration.push_back(kBlendInfo.vec_iApplyDuration[j]);
					}
				}
			}
			break;
		}
	}

	return true;
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
bool CItemManager::LoadItemScale(const char* szItemScale)
{
	CMappedFile File;
	LPCVOID pData;

	if (!CEterPackManager::Instance().Get(File, szItemScale, &pData))
		return false;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(File.Size(), pData);

	CTokenVector TokenVector;
	for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLine(i, &TokenVector, "\t"))
			continue;

		if (!(TokenVector.size() == 9))
		{
			TraceError(" CItemManager::LoadItemScale(%s) - LoadItemScale in %d\n", szItemScale, i);
			continue;
		}

		const std::string & c_rstrID = TokenVector[0];
		const std::string & c_rstrJob = TokenVector[1];
		const std::string & c_rstrSex = TokenVector[2];
		const std::string & c_rstrX = TokenVector[3];
		const std::string & c_rstrY = TokenVector[4];
		const std::string & c_rstrZ = TokenVector[5];
		const std::string & c_rstrPosX = TokenVector[6];
		const std::string & c_rstrPosY = TokenVector[7];
		const std::string & c_rstrPosZ = TokenVector[8];

		DWORD dwItemVNum = atoi(c_rstrID.c_str());
		float xScale = atof(c_rstrX.c_str()) * 0.01f;
		float yScale = atof(c_rstrY.c_str()) * 0.01f;
		float zScale = atof(c_rstrZ.c_str()) * 0.01f;
		float yPosScale = atof(c_rstrPosX.c_str()) * 100.0f;
		float xPosScale = atof(c_rstrPosY.c_str()) * 100.0f;
		float zPosScale = atof(c_rstrPosZ.c_str()) * 100.0f;

		DWORD dwJob;
		bool bMount = false;
		if (c_rstrJob == "JOB_WARRIOR")
		{
			dwJob = NRaceData::JOB_WARRIOR;
		}

		else if (c_rstrJob == "JOB_ASSASSIN")
		{
			dwJob = NRaceData::JOB_ASSASSIN;
		}

		else if (c_rstrJob == "JOB_SURA")
		{
			dwJob = NRaceData::JOB_SURA;
		}

		else if (c_rstrJob == "JOB_SHAMAN")
		{
			dwJob = NRaceData::JOB_SHAMAN;
		}
#ifdef ENABLE_WOLFMAN
		else if (c_rstrJob == "JOB_WOLFMAN")
		{
			dwJob = NRaceData::JOB_WOLFMAN;
		}
#endif
		else if (c_rstrJob == "JOB_WARRIOR_MOUNT")
		{
			dwJob = NRaceData::JOB_WARRIOR;
			bMount = true;
		}

		else if (c_rstrJob == "JOB_ASSASSIN_MOUNT")
		{
			dwJob = NRaceData::JOB_ASSASSIN;
			bMount = true;
		}

		else if (c_rstrJob == "JOB_SURA_MOUNT")
		{
			dwJob = NRaceData::JOB_SURA;
			bMount = true;
		}

		else if (c_rstrJob == "JOB_SHAMAN_MOUNT")
		{
			dwJob = NRaceData::JOB_SHAMAN;
			bMount = true;
		}
#ifdef ENABLE_WOLFMAN
		else if (c_rstrJob == "JOB_WOLFMAN_MOUNT")
		{
			dwJob = NRaceData::JOB_WOLFMAN;
			bMount = true;
		}
#endif

		else
		{
			TraceError("CItemManager::LoadItemScale(%s) - LoadItemScale in %d: invalid job [%s]\n", szItemScale, i, c_rstrJob.c_str());
			continue;
		}

		DWORD dwSex = c_rstrSex[0] == 'M';


		for (int i = 0; i <= 4; ++i)
		{
			CItemData * pItemData = MakeItemData(dwItemVNum + i);
			//TraceError("Set Sash %d to scale %f %f %f and  pos = %f %f %f for job %d, sex %d", dwItemVNum+i, xScale, yScale, zScale, xPosScale, yPosScale, zPosScale, dwJob, dwSex);
			pItemData->SetItemTableScaleData(dwJob, dwSex, xScale, yScale, zScale, xPosScale, yPosScale, zPosScale, bMount);

			if (!bMount) // mount default..
				pItemData->SetItemTableScaleData(dwJob, dwSex, xScale, yScale, zScale, xPosScale, yPosScale, zPosScale, false);
		}

	}
	return true;
}
#endif

void CItemManager::Destroy()
{
	TItemMap::iterator i;
	for (i=m_ItemMap.begin(); i!=m_ItemMap.end(); ++i)
		CItemData::Delete(i->second);
	for (int i = 0; i < m_vec_tempSkillItemData.size(); ++i)
		CItemData::Delete(m_vec_tempSkillItemData[i]);

	m_ItemMap.clear();
#ifdef INGAME_WIKI
	m_tempItemVec.clear();
#endif
	m_vec_tempSkillItemData.clear();
	m_vec_ItemData_NameSort.clear();
	m_vec_wikiItemData_NameSort.clear();
	m_BlendMap.clear();
}

#ifdef INGAME_WIKI
bool CItemManager::IsFilteredAntiflag(CItemData* itemData, DWORD raceFilter)
{
	if (raceFilter != 0)
	{
		if (!itemData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_SHAMAN) && raceFilter & CItemData::ITEM_ANTIFLAG_SHAMAN)
			return false;

		if (!itemData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_SURA) && raceFilter & CItemData::ITEM_ANTIFLAG_SURA)
			return false;

		if (!itemData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_ASSASSIN) && raceFilter & CItemData::ITEM_ANTIFLAG_ASSASSIN)
			return false;

		if (!itemData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_WARRIOR) && raceFilter & CItemData::ITEM_ANTIFLAG_WARRIOR)
			return false;
	}

	return true;
}

size_t CItemManager::WikiLoadClassItems(BYTE classType, DWORD raceFilter)
{
	m_tempItemVec.clear();

	for (TItemMap::iterator it = m_ItemMap.begin(); it != m_ItemMap.end(); ++it)
	{
		if (!it->second->IsValidImage() || it->first < 10 || it->second->IsBlacklisted())// || it->second->GetIconFileName().find("D:\\ymir work\\ui\\game\\quest\\questicon\\level_") != std::string::npos)
			continue;

		switch (classType)
		{
		case 0://weapon
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_WEAPON && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 1://body
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_BODY && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 2:
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_EAR && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 3:
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_FOOTS && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 4:
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_HEAD && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 5:
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_NECK && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 6:
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_SHIELD && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 7:
			if (it->first % 10 == 0 && it->second->GetType() == CItemData::ITEM_TYPE_ARMOR && it->second->GetSubType() == CItemData::ARMOR_WRIST && !IsFilteredAntiflag(it->second, raceFilter))
				m_tempItemVec.push_back(it->first);
			break;
		case 8://chests
			if (it->second->GetType() == CItemData::ITEM_TYPE_GIFTBOX)
				m_tempItemVec.push_back(it->first);
			break;
		case 9: // belts
			if (it->second->GetType() == CItemData::ITEM_TYPE_BELT)
				m_tempItemVec.push_back(it->first);
			break;
		case 10: // talisman
			if (it->first == 94326)
				m_tempItemVec.push_back(it->first);
			break;
		}
	}

	return m_tempItemVec.size();
}

DWORD CItemManager::WikiSearchItem(std::string subStr)
{
	m_tempItemVec.clear();

	for (TItemMap::iterator i = m_ItemMap.begin(); i != m_ItemMap.end(); i++)
	{
		const CItemData::TItemTable* tbl = i->second->GetTable();
		if (!i->second->IsValidImage() || i->first < 10 || tbl->vnum() < 10 || i->second->IsBlacklisted() || !CResourceManager::Instance().IsFileExist(i->second->GetIconFileName().c_str()))
			continue;

		char tempName[25];
		memcpy(tempName, tbl->locale_name(CLocaleManager::Instance().GetLanguage()).c_str(), sizeof(tempName));
		for (int j = 0; j < sizeof(tempName); j++)
			tempName[j] = tolower(tempName[j]);

		std::string tempString = tempName;
		if (tempString.find(subStr) != std::string::npos)
			m_tempItemVec.push_back(i->first);
	}

	return m_tempItemVec.size();
}
#endif

CItemManager::CItemManager() : m_pSelectedItemData(NULL)
{
}
CItemManager::~CItemManager()
{
	Destroy();
}
