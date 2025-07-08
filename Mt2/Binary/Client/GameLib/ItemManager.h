#pragma once

#include "ItemData.h"

class CItemManager : public CSingleton<CItemManager>
{
	public:
		enum EItemDescCol
		{
			ITEMDESC_COL_VNUM,
			// ITEMDESC_COL_NAME,
			ITEMDESC_COL_DESC,
			ITEMDESC_COL_SUMM,
			ITEMDESC_COL_NUM,
		};

	public:
		typedef std::map<DWORD, CItemData*> TItemMap;
#ifdef INGAME_WIKI
		typedef std::vector<CItemData*> TItemVec;
		typedef std::vector<DWORD> TItemNumVec;
#endif
		typedef std::map<std::string, CItemData*> TItemNameMap;
		typedef std::vector<CItemData*> TItemVector;
		typedef std::pair<CItemManager::TItemVector::const_iterator, CItemManager::TItemVector::const_iterator> TItemRange;

		typedef struct SBlendInfo {
			DWORD	dwVnum;
			BYTE	bApplyType;
			std::vector<int>	vec_iApplyValue;
			std::vector<int>	vec_iApplyDuration;
		} TBlendInfo;
		typedef std::map<DWORD, TBlendInfo> TBlendMap;

	public:
		CItemManager();
		virtual ~CItemManager();
		
		void			Destroy();

		BOOL			SelectItemData(DWORD dwIndex);
		BOOL			SelectItemData(CItemData* pItemData);
		CItemData *		GetSelectedItemDataPointer();

		BOOL			GetItemDataPointer(DWORD dwItemID, CItemData ** ppItemData);
		BOOL			GetBlendInfoPointer(DWORD dwItemID, TBlendInfo ** ppBlendInfo);
		BOOL			GetBlendInfoPointerByApplyType(BYTE bApplyType, TBlendInfo ** ppBlendInfo);
		
		/////
		BYTE			GetAttrTypeByName(const std::string& stAttrType);

		bool			LoadItemDesc(const char* c_szFileName);
		bool			LoadItemList(const char* c_szFileName);
		bool			LoadItemTable(const char* c_szFileName);
		bool			LoadItemBlend(const char* c_szFileName);
		CItemData *		MakeItemData(DWORD dwIndex);

		void			SortItemDataName();
		CItemData*		FindItemDataByName(const char* c_pszName, bool isWiki = false);

		void			AppendSkillNameItems();

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		bool			LoadItemScale(const char* szItemScale);
#endif

		TItemRange		GetProtosByName(const char* name) const;

private:
		void			CreateSortedItemData();

	protected:
		TItemMap m_ItemMap;
		TBlendMap m_BlendMap;
		TItemVector	m_vec_tempSkillItemData;
		TItemVector	m_vec_ItemRange;
		TItemVector	m_vec_ItemData_NameSort;
		TItemVector	m_vec_wikiItemData_NameSort;
		TItemVector	m_vecItemDataSorted;
		CItemData * m_pSelectedItemData;

#ifdef INGAME_WIKI
	protected:
		TItemNumVec m_tempItemVec;

	public:
		size_t WikiLoadClassItems(BYTE classType, DWORD raceFilter);
		TItemNumVec* WikiGetLastItems() { return &m_tempItemVec; }

		void WikiAddVnumToBlacklist(DWORD vnum)
		{
			auto it = m_ItemMap.find(vnum);
			if (it != m_ItemMap.end())
				it->second->SetBlacklisted(true);
		};

		DWORD WikiSearchItem(std::string subStr);

	private:
		bool IsFilteredAntiflag(CItemData* itemData, DWORD raceFilter);
#endif
};
