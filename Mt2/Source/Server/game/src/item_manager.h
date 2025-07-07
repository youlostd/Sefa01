#ifndef __INC_ITEM_MANAGER__
#define __INC_ITEM_MANAGER__

#include "packet.h"
#include "db.h"

#include "protobuf_data.h"

// special_item_group.txt¿¡¼­ Á¤ÀÇÇÏ´Â ¼Ó¼º ±×·ì
// type attr·Î ¼±¾ðÇÒ ¼ö ÀÖ´Ù.
// ÀÌ ¼Ó¼º ±×·ìÀ» ÀÌ¿ëÇÒ ¼ö ÀÖ´Â °ÍÀº special_item_group.txt¿¡¼­ Special typeÀ¸·Î Á¤ÀÇµÈ ±×·ì¿¡ ¼ÓÇÑ UNIQUE ITEMÀÌ´Ù.
class CSpecialAttrGroup
{
public:
	CSpecialAttrGroup(DWORD vnum)
		: m_dwVnum(vnum)
	{}
	struct CSpecialAttrInfo
	{
		CSpecialAttrInfo (DWORD _apply_type, DWORD _apply_value)
			: apply_type(_apply_type), apply_value(_apply_value)
		{}
		DWORD apply_type;
		DWORD apply_value;

	};
	DWORD m_dwVnum;
	std::string	m_stEffectFileName;
	std::vector<CSpecialAttrInfo> m_vecAttrs;
};

class CSpecialItemGroup
{
	public:
		enum EGiveType
		{
			NONE,
			GOLD,
			EXP,
			MOB,
			SLOW,
			DRAIN_HP,
			POISON,
			MOB_GROUP,
			POLY_MARBLE,
			TYPE_MAX_NUM,
		};

		// QUEST Å¸ÀÔÀº Äù½ºÆ® ½ºÅ©¸³Æ®¿¡¼­ vnum.sig_use¸¦ »ç¿ëÇÒ ¼ö ÀÖ´Â ±×·ìÀÌ´Ù.
		//		´Ü, ÀÌ ±×·ì¿¡ µé¾î°¡±â À§ÇØ¼­´Â ITEM ÀÚÃ¼ÀÇ TYPEÀÌ QUEST¿©¾ß ÇÑ´Ù.
		// SPECIAL Å¸ÀÔÀº idx, item_vnum, attr_vnumÀ» ÀÔ·ÂÇÑ´Ù. attr_vnumÀº À§¿¡ CSpecialAttrGroupÀÇ VnumÀÌ´Ù.
		//		ÀÌ ±×·ì¿¡ µé¾îÀÖ´Â ¾ÆÀÌÅÛÀº °°ÀÌ Âø¿ëÇÒ ¼ö ¾ø´Ù.
		enum ESIGType { NORMAL, PCT, QUEST, SPECIAL };

		struct CSpecialItemInfo
		{
			DWORD vnum_start;
			DWORD vnum_end;
			int count;
			int rare;

			CSpecialItemInfo(DWORD _vnum_start, DWORD _vnum_end, int _count, int _rare)
				: vnum_start(_vnum_start), vnum_end(_vnum_end), count(_count), rare(_rare)
			{
				if (vnum_end == 0)
					vnum_end = vnum_start;
			}
		};

		CSpecialItemGroup(DWORD vnum, BYTE type=0)
			: m_dwVnum(vnum), m_bType(type)
			{}

		void AddItem(DWORD vnum_start, DWORD vnum_end, int count, int prob, int rare)
		{
			if (!prob)
				return;
			if (!m_vecProbs.empty())
				prob += m_vecProbs.back();
			if (prob < 100 || m_bType != PCT)
			{
				m_vecProbs.push_back(prob);
				m_vecItems.push_back(CSpecialItemInfo(vnum_start, vnum_end, count, rare));
			}
			else
			{
				m_vec100PctItems.push_back(CSpecialItemInfo(vnum_start, vnum_end, count, rare));
			}
		}

		bool IsEmpty() const
		{
			return m_vecProbs.empty();
		}

		// Type Multi, Áï m_bType == PCT ÀÎ °æ¿ì,
		// È®·üÀ» ´õÇØ°¡Áö ¾Ê°í, µ¶¸³ÀûÀ¸·Î °è»êÇÏ¿© ¾ÆÀÌÅÛÀ» »ý¼ºÇÑ´Ù.
		// µû¶ó¼­ ¿©·¯ °³ÀÇ ¾ÆÀÌÅÛÀÌ »ý¼ºµÉ ¼ö ÀÖ´Ù.
		// by rtsummit
		int GetMultiIndex(std::vector <int> &idx_vec) const
		{
			idx_vec.clear();
			if (m_bType == PCT)
			{
				/*int count = 0;
				if (random_number(1,100) <= m_vecProbs[0])
				{
					idx_vec.push_back(0);
					count++;
				}
				for (uint i = 1; i < m_vecProbs.size(); i++)
				{
					if (random_number(1,100) <= m_vecProbs[i] - m_vecProbs[i-1])
					{
						idx_vec.push_back(i);
						count++;
					}
				}
				return count;
				*/
				if (m_vecItems.size() > 0)
					idx_vec.push_back(GetOneIndex());

				// add 100%
				for (int i = 0; i < Get100Size(); ++i)
					idx_vec.push_back(m_vecItems.size() + i);
				return idx_vec.size();
			}
			else
			{
				if (m_vecItems.size() > 0)
				{
					idx_vec.push_back(GetOneIndex());
					return 1;
				}
			}

			return 0;
		}

		int GetOneIndex() const
		{
			int n = random_number(1, m_vecProbs.back());
			itertype(m_vecProbs) it = lower_bound(m_vecProbs.begin(), m_vecProbs.end(), n);
			return std::distance(m_vecProbs.begin(), it);
		}

		int Get100Vnum(int idx) const
		{
			if (m_vec100PctItems[idx].vnum_start != m_vec100PctItems[idx].vnum_end)
				return GetVnumByRange(m_vec100PctItems[idx].vnum_start, m_vec100PctItems[idx].vnum_end);
			else
				return m_vec100PctItems[idx].vnum_start;
		}

		int Get100Count(int idx) const
		{
			return m_vec100PctItems[idx].count;
		}

		int Get100RarePct(int idx) const
		{
			return m_vec100PctItems[idx].rare;
		}

		int Get100Size() const
		{
			return m_vec100PctItems.size();
		}

		int GetVnum(int idx) const
		{
			if (idx >= m_vecItems.size())
				return Get100Vnum(idx - m_vecItems.size());

			if (m_vecItems[idx].vnum_start != m_vecItems[idx].vnum_end)
				return GetVnumByRange(m_vecItems[idx].vnum_start, m_vecItems[idx].vnum_end);
			else
				return m_vecItems[idx].vnum_start;
		}

		int GetCount(int idx) const
		{
			if (idx >= m_vecItems.size())
				return Get100Count(idx - m_vecItems.size());

			return m_vecItems[idx].count;
		}

		int GetRarePct(int idx) const
		{
			if (idx >= m_vecItems.size())
				return Get100RarePct(idx - m_vecItems.size());

			return m_vecItems[idx].rare;
		}

		bool Contains(DWORD dwVnum) const
		{
			for (DWORD i = 0; i < m_vecItems.size(); i++)
			{
				if (m_vecItems[i].vnum_start <= dwVnum && m_vecItems[i].vnum_end >= dwVnum)
					return true;
			}
			return false;
		}
		
		// GroupÀÇ TypeÀÌ SpecialÀÎ °æ¿ì¿¡
		// dwVnum¿¡ ¸ÅÄªµÇ´Â AttrVnumÀ» returnÇØÁØ´Ù.
		DWORD GetAttrVnum(DWORD dwVnum) const
		{
			if (CSpecialItemGroup::SPECIAL != m_bType)
				return 0;
			for (itertype(m_vecItems) it = m_vecItems.begin(); it != m_vecItems.end(); it++)
			{
				if (it->vnum_start <= dwVnum && it->vnum_end >= dwVnum)
				{
					return it->count;
				}
			}
			return 0;
		}

		int GetProb(int idx)
		{
			return m_vecProbs[idx];
		}

		// GroupÀÇ Size¸¦ returnÇØÁØ´Ù.
		int GetGroupSize() const
		{
			return m_vecProbs.size();
		}

		DWORD m_dwVnum;
		BYTE	m_bType;
		std::vector<int> m_vecProbs;
		std::vector<CSpecialItemInfo> m_vecItems; // vnum, count
		std::vector<CSpecialItemInfo> m_vec100PctItems; // vnum, count

	private:
		int GetVnumByRange(DWORD dwStart, DWORD dwEnd) const;
};

class CMobItemGroup
{
	public:
		struct SMobItemGroupInfo
		{
			DWORD dwItemVnumStart;
			DWORD dwItemVnumEnd;
			int iCount;
			int iRarePct;

			SMobItemGroupInfo(DWORD dwItemVnumStart, DWORD dwItemVnumEnd, int iCount, int iRarePct)
				: dwItemVnumStart(dwItemVnumStart), dwItemVnumEnd(dwItemVnumEnd),
			iCount(iCount),
			iRarePct(iRarePct)
			{
			}
		};

		CMobItemGroup(DWORD dwMobVnum, int iKillDrop, const std::string& r_stName)
			:
			m_dwMobVnum(dwMobVnum),
		m_iKillDrop(iKillDrop),
		m_stName(r_stName)
		{
		}

		int GetKillPerDrop() const
		{
			return m_iKillDrop;
		}

		void AddItem(DWORD dwItemVnumStart, DWORD dwItemVnumEnd, int iCount, int iPartPct, int iRarePct)
		{
			if (!m_vecProbs.empty())
				iPartPct += m_vecProbs.back();
			m_vecProbs.push_back(iPartPct);
			m_vecItems.push_back(SMobItemGroupInfo(dwItemVnumStart, dwItemVnumEnd, iCount, iRarePct));
		}

		// MOB_DROP_ITEM_BUG_FIX
		bool IsEmpty() const
		{
			return m_vecProbs.empty();
		}

		int GetOneIndex() const
		{
			int n = random_number(1, m_vecProbs.back());
			itertype(m_vecProbs) it = lower_bound(m_vecProbs.begin(), m_vecProbs.end(), n);
			return std::distance(m_vecProbs.begin(), it);
		}
		// END_OF_MOB_DROP_ITEM_BUG_FIX

		const SMobItemGroupInfo& GetOne() const
		{
			return m_vecItems[GetOneIndex()];
		}

		const std::vector<int>& GetProbVector() const
		{
			return m_vecProbs;
		}

		const std::vector<SMobItemGroupInfo>& GetItemVector() const
		{
			return m_vecItems;
		}

	private:
		DWORD m_dwMobVnum;
		int m_iKillDrop;
		std::string m_stName;
		std::vector<int> m_vecProbs;
		std::vector<SMobItemGroupInfo> m_vecItems;
};

class CDropItemGroup
{
	public:
		struct SDropItemGroupInfo
		{
			DWORD	dwVnumStart;
			DWORD	dwVnumEnd;
			DWORD	dwPct;
			int	iCount;

			SDropItemGroupInfo(DWORD dwVnumStart, DWORD dwVnumEnd, DWORD dwPct, int iCount)
				: dwVnumStart(dwVnumStart), dwVnumEnd(dwVnumEnd), dwPct(dwPct), iCount(iCount)
				{}
		};

	public:
	CDropItemGroup(DWORD dwVnum, DWORD dwMobVnum, const std::string& r_stName)
		:
		m_dwVnum(dwVnum),
	m_dwMobVnum(dwMobVnum),
	m_stName(r_stName)
	{
	}

	const std::vector<SDropItemGroupInfo> & GetVector()
	{
		return m_vec_items;
	}

	void AddItem(DWORD dwItemVnumStart, DWORD dwItemVnumEnd, DWORD dwPct, int iCount)
	{
		m_vec_items.push_back(SDropItemGroupInfo(dwItemVnumStart, dwItemVnumEnd, dwPct, iCount));
	}

	private:
	DWORD m_dwVnum;
	DWORD m_dwMobVnum;
	std::string m_stName;
	std::vector<SDropItemGroupInfo> m_vec_items;
};

class CLevelItemGroup
{
	public:
		struct SLevelItemGroupInfo
		{
			DWORD dwVNumStart, dwVNumEnd;
			DWORD dwPct;
			int iCount;

			SLevelItemGroupInfo(DWORD dwVnumStart, DWORD dwVnumEnd, DWORD dwPct, int iCount)
				: dwVNumStart(dwVnumStart), dwVNumEnd(dwVnumEnd), dwPct(dwPct), iCount(iCount)
			{ }
		};

	private :
		DWORD m_dwLevelLimit;
		std::string m_stName;
		std::vector<SLevelItemGroupInfo> m_vec_items;

	public :
		CLevelItemGroup(DWORD dwLevelLimit)
			: m_dwLevelLimit(dwLevelLimit)
		{}

		DWORD GetLevelLimit() { return m_dwLevelLimit; }

		void AddItem(DWORD dwItemVnumStart, DWORD dwItemVnumEnd, DWORD dwPct, int iCount)
		{
			m_vec_items.push_back(SLevelItemGroupInfo(dwItemVnumStart, dwItemVnumEnd, dwPct, iCount));
		}

		const std::vector<SLevelItemGroupInfo> & GetVector()
		{
			return m_vec_items;
		}
};

class CBuyerThiefGlovesItemGroup
{
	struct SThiefGroupInfo
	{
		DWORD	dwVnumStart;
		DWORD	dwVnumEnd;
		DWORD	dwPct;
		int	iCount;

		SThiefGroupInfo(DWORD dwVnumStart, DWORD dwVnumEnd, DWORD dwPct, int iCount)
			: dwVnumStart(dwVnumStart), dwVnumEnd(dwVnumEnd), dwPct(dwPct), iCount(iCount)
			{}
	};

	public:
	CBuyerThiefGlovesItemGroup(DWORD dwVnum, DWORD dwMobVnum, const std::string& r_stName)
		:
		m_dwVnum(dwVnum),
	m_dwMobVnum(dwMobVnum),
	m_stName(r_stName)
	{
	}

	const std::vector<SThiefGroupInfo> & GetVector()
	{
		return m_vec_items;
	}

	void AddItem(DWORD dwItemVnumStart, DWORD dwItemVnumEnd, DWORD dwPct, int iCount)
	{
		m_vec_items.push_back(SThiefGroupInfo(dwItemVnumStart, dwItemVnumEnd, dwPct, iCount));
	}

	private:
	DWORD m_dwVnum;
	DWORD m_dwMobVnum;
	std::string m_stName;
	std::vector<SThiefGroupInfo> m_vec_items;
};

class ITEM;

class ITEM_MANAGER : public singleton<ITEM_MANAGER>
{
	public:
		ITEM_MANAGER();
		virtual ~ITEM_MANAGER();

		bool            Initialize(const google::protobuf::RepeatedPtrField<network::TItemTable>& table);
		void			InitializeSoulProto(const ::google::protobuf::RepeatedPtrField<network::TSoulProtoTable>& table);
		const network::TSoulProtoTable* GetSoulProto(DWORD vnum);
		bool			ChangeSashAttr(LPITEM item);
		void			Destroy();
		void			Update();	// ¸Å ·çÇÁ¸¶´Ù ºÎ¸¥´Ù.
		void			GracefulShutdown();

#ifdef INGAME_WIKI
		network::TWikiInfoTable* GetItemWikiInfo(DWORD vnum);
		std::vector<network::TWikiItemOriginInfo>& GetItemOrigin(DWORD vnum) { return m_itemOriginMap[vnum]; }
#endif

		DWORD			GetNewID();
		bool			SetMaxItemID(network::TItemIDRangeTable range); // ÃÖ´ë °íÀ¯ ¾ÆÀÌµð¸¦ ÁöÁ¤
		bool			SetMaxSpareItemID(network::TItemIDRangeTable range);

		// DelayedSave: ¾î¶°ÇÑ ·çÆ¾ ³»¿¡¼­ ÀúÀåÀ» ÇØ¾ß ÇÒ ÁþÀ» ¸¹ÀÌ ÇÏ¸é ÀúÀå
		// Äõ¸®°¡ ³Ê¹« ¸¹¾ÆÁö¹Ç·Î "ÀúÀåÀ» ÇÑ´Ù" ¶ó°í Ç¥½Ã¸¸ ÇØµÎ°í Àá±ñ
		// (¿¹: 1 frame) ÈÄ¿¡ ÀúÀå½ÃÅ²´Ù.
		void			DelayedSave(LPITEM item);
		void			FlushDelayedSave(LPITEM item); // Delayed ¸®½ºÆ®¿¡ ÀÖ´Ù¸é Áö¿ì°í ÀúÀåÇÑ´Ù. ²÷±è Ã³¸®½Ã »ç¿ë µÊ.
		void			SaveSingleItem(LPITEM item);

		LPITEM			CreateItem(DWORD vnum, DWORD count = 1, DWORD dwID = 0, bool bTryMagic = false, int iRarePct = -1, bool bSkipSave = false, DWORD owner_pid = 0);
		LPITEM			CreateItem(const network::TItemData* pTable, bool bSkipSave = false, bool bSetSocket = true);

		void			DestroyItem(LPITEM item);
		void			RemoveItem(LPITEM item, const char * c_pszReason=NULL); // »ç¿ëÀÚ·Î ºÎÅÍ ¾ÆÀÌÅÛÀ» Á¦°Å
		void			RemoveItemForFurtherUse(LPITEM item);

		LPITEM			Find(DWORD id);
		LPITEM				  FindByVID(DWORD vid);
		network::TItemTable *			GetTable(DWORD vnum);
		bool			GetVnum(const char * c_pszName, DWORD & r_dwVnum, int iLanguageID = -1);
		bool			GetVnumByOriginalName(const char * c_pszName, DWORD & r_dwVnum);

		bool			GetDropPct(LPCHARACTER pkChr, LPCHARACTER pkKiller, OUT int& iDeltaPercent, OUT int& iRandRange);
		void			GetDropItemList(DWORD dwNPCRace, std::vector<network::TTargetMonsterDropInfoTable>& vec_item, BYTE bLevel);
		bool			CreateDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item, std::vector<LPITEM> & vec_item_auto_pickup);

		bool			ReadCommonDropItemFile(const char * c_pszFileName);
		bool			ReadEtcDropItemFile(const char * c_pszFileName);
		bool			ReadDropItemGroup(const char * c_pszFileName);
		bool			ReadMonsterDropItemGroup(const char * c_pszFileName);
		bool			ReadSpecialDropItemFile(const char * c_pszFileName);

		static DWORD	GetItemRealTimeout(const network::TItemTable* proto, const google::protobuf::RepeatedField<google::protobuf::int32>& sockets);
		
		// convert name -> vnum special_item_group.txt
		bool			ConvSpecialDropItemFile();
		// convert name -> vnum special_item_group.txt

		DWORD			GetRefineFromVnum(DWORD dwVnum);

		static void		CopyAllAttrTo(LPITEM pkOldItem, LPITEM pkNewItem);		// pkNewItemÀ¸·Î ¸ðµç ¼Ó¼º°ú ¼ÒÄÏ °ªµéÀ» ¸ñ»çÇÏ´Â ÇÔ¼ö.


		const CSpecialItemGroup* GetSpecialItemGroup(DWORD dwVnum);
		const CSpecialAttrGroup* GetSpecialAttrGroup(DWORD dwVnum);

		const std::vector<network::TItemTable> & GetTable() { return m_vec_prototype; }

		// CHECK_UNIQUE_GROUP
		int			GetSpecialGroupFromItem(DWORD dwVnum) const { itertype(m_ItemToSpecialGroup) it = m_ItemToSpecialGroup.find(dwVnum); return (it == m_ItemToSpecialGroup.end()) ? 0 : it->second; }
		// END_OF_CHECK_UNIQUE_GROUP

	protected:
		int					 RealNumber(DWORD vnum);
		void			CreateQuestDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item, int iDeltaPercent, int iRandRange);

	public:
		std::map<DWORD, std::vector<CMobItemGroup*> >&		GetMobItemGroupMap()			{ return m_map_pkMobItemGroup; }
		std::map<DWORD, CDropItemGroup*>&					GetDropItemGroupMap()			{ return m_map_pkDropItemGroup; }
		std::map<DWORD, CLevelItemGroup*>&					GetLevelItemGroupMap()			{ return m_map_pkLevelItemGroup; }
		std::map<DWORD, CSpecialItemGroup*>&				GetSpecialItemGroupMap()		{ return m_map_pkSpecialItemGroup; }

	protected:
		typedef std::map<DWORD, LPITEM> ITEM_VID_MAP;		

		std::vector<network::TItemTable>		m_vec_prototype;
		std::map<DWORD, network::TSoulProtoTable> m_map_soulProtoTable;
		std::vector<network::TItemTable*> m_vec_item_vnum_range_info;
		std::map<DWORD, DWORD>		m_map_ItemRefineFrom;
		int				m_iTopOfTable;

#ifdef INGAME_WIKI
		std::map<DWORD, std::unique_ptr<network::TWikiInfoTable>> m_wikiInfoMap;
		std::map<DWORD, std::vector<network::TWikiItemOriginInfo>> m_itemOriginMap;
#endif
		ITEM_VID_MAP			m_VIDMap;			///< m_dwVIDCount ÀÇ °ª´ÜÀ§·Î ¾ÆÀÌÅÛÀ» ÀúÀåÇÑ´Ù.
		DWORD				m_dwVIDCount;			///< ÀÌ³à¼® VID°¡ ¾Æ´Ï¶ó ±×³É ÇÁ·Î¼¼½º ´ÜÀ§ À¯´ÏÅ© ¹øÈ£´Ù.
		DWORD				m_dwCurrentID;
		network::TItemIDRangeTable	m_ItemIDRange;
		network::TItemIDRangeTable	m_ItemIDSpareRange;

		TR1_NS::unordered_set<LPITEM> m_set_pkItemForDelayedSave;
		std::map<DWORD, LPITEM>		m_map_pkItemByID;
		std::map<DWORD, DWORD>		m_map_dwEtcItemDropProb;
		std::map<DWORD, CDropItemGroup*> m_map_pkDropItemGroup;
		std::map<DWORD, CSpecialItemGroup*> m_map_pkSpecialItemGroup;
		std::map<DWORD, CSpecialItemGroup*> m_map_pkQuestItemGroup;
		std::map<DWORD, CSpecialAttrGroup*> m_map_pkSpecialAttrGroup;
		std::map<DWORD, std::vector<CMobItemGroup*> > m_map_pkMobItemGroup;
		std::map<DWORD, CLevelItemGroup*> m_map_pkLevelItemGroup;
		std::map<DWORD, CBuyerThiefGlovesItemGroup*> m_map_pkGloveItemGroup;

		// CHECK_UNIQUE_GROUP
		std::map<DWORD, int>		m_ItemToSpecialGroup;
		// END_OF_CHECK_UNIQUE_GROUP

	public:	
		std::map<DWORD, network::TItemTable>  m_map_vid;
		std::map<DWORD, network::TItemTable>&  GetVIDMap() { return m_map_vid; }
		std::vector<network::TItemTable>& GetVecProto() { return m_vec_prototype; }	
		
		const static int MAX_NORM_ATTR_NUM = 5;
		const static int MAX_COSTUME_ATTR_NUM = 3;
		const static int MAX_COSTUME_BONUS_ATTR_NUM = 2;
		const static int MAX_COSTUME_ATTR_NUM_TOTAL = MAX_COSTUME_ATTR_NUM + MAX_COSTUME_BONUS_ATTR_NUM;
		const static int MAX_TOTEM_NORM_ATTR_NUM = 3;
		const static int MAX_RARE_ATTR_NUM = 2;

	public:
		bool	GetVnumRangeByString(const std::string& stVnumRange, DWORD & r_dwVnumStart, DWORD & r_dwVnumEnd);

	public:
		DWORD	GetInventoryPageSize(BYTE bWindow);
		DWORD	GetInventoryMaxNum(BYTE bWindow, const LPCHARACTER ch = NULL);
		DWORD	GetInventoryStart(BYTE bWindow);
		BYTE	GetTargetWindow(LPITEM pkItem);
		BYTE	GetTargetWindow(DWORD dwVnum);
		const char*	GetTargetWindowName(BYTE bWindow);
		bool	GetInventorySlotRange(BYTE bWindow, WORD& wSlotStart, WORD& wSlotEnd, const LPCHARACTER ch = NULL);
		bool	GetInventorySlotRange(BYTE& bWindow, WORD& wSlotStart, WORD& wSlotEnd, bool bSetInvOnErr, const LPCHARACTER ch);
		bool	IsNewWindow(BYTE bWindow);
		BYTE	GetWindowBySlot(WORD wSlotPos);

		void	LoadUppItemList();
		void	OnLoadUppItemList(MYSQL_RES* pSQSLRes);
		bool	IsUppItem(DWORD dwVnum) const;

	private:
		BYTE	GetTargetWindow(DWORD dwVnum, BYTE bItemType, BYTE bItemSubType);

	public:
		void	LoadDisabledItemList();
		const std::set<DWORD>*	GetDisabledItemList(long lMapIndex);
		bool	IsDisabledItem(DWORD dwVnum, long lMapIndex);

	private:
		std::map<long, std::set<DWORD> >	m_map_DisabledItemList;
		
	public:
		const char*		GetItemLink(DWORD dwItemVnum);
		void			GetPlayerItem(LPITEM item, network::TItemData* pRet);
		void			GiveItemRefund(const network::TItemData* pTab);
		void			GiveGoldRefund(DWORD pid, long long gold);

	private:
		std::set<DWORD>	m_set_UppItemVnums;
};

#define M2_DESTROY_ITEM(ptr) ITEM_MANAGER::instance().DestroyItem(ptr)

#endif