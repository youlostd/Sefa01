#ifndef __INC_METIN_II_GAME_ITEM_H__
#define __INC_METIN_II_GAME_ITEM_H__

#include "entity.h"
#include "constants.h"

#include "protobuf_data.h"
#include "protobuf_data_item.h"

#include "../../common/item_length.h"

#ifdef __PET_ADVANCED__
class CPetAdvanced;
#endif

class CItem : public CEntity
{
#ifdef CRYSTAL_SYSTEM
	public:
		static constexpr size_t CRYSTAL_TIME_SOCKET = 0;
		static constexpr size_t CRYSTAL_CLARITY_TYPE_SOCKET = 1;
		static constexpr size_t CRYSTAL_CLARITY_LEVEL_SOCKET = 2;

		static constexpr size_t CRYSTAL_MIN_ACTIVE_DURATION = 60;
		static constexpr size_t CRYSTAL_MAX_DURATION = 60 * 60 * 24;
#endif

	protected:
		// override methods from ENTITY class
		virtual void	EncodeInsertPacket(LPENTITY entity);
		virtual void	EncodeRemovePacket(LPENTITY entity);

	public:
		CItem(DWORD dwVnum);
		virtual ~CItem();

		int			GetLevelLimit();

		bool		CheckItemUseLevel(int nLevel);

		long		FindApplyValue(BYTE bApplyType);

		bool		IsStackable()		{ return (GetFlag() & ITEM_FLAG_STACKABLE)?true:false; }

		void		Initialize();
		void		Destroy();

		void		Save();

		void		SetWindow(BYTE b)	{ m_bWindow = b; }
		BYTE		GetWindow()		{ return m_bWindow; }

		void		SetID(DWORD id)		{ m_dwID = id;	}
		DWORD		GetID()			{ return m_dwID; }

		void			SetProto(const network::TItemTable * table);
		network::TItemTable const *	GetProto() const	{ return m_pProto; }

		int		GetGold();
		int		GetShopBuyPrice();
		

#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
		const char * GetName(BYTE bLanguageID = LANGUAGE_DEFAULT) const;
#else
		const char *	GetName(BYTE bLanguageID = LANGUAGE_DEFAULT) const		{ return m_pProto ? m_pProto->szLocaleName[bLanguageID] : NULL; }
#endif

		
		const char *	GetBaseName() const		{ return m_pProto ? m_pProto->name().c_str() : NULL; }
		BYTE		GetSize()		{ return m_pProto ? m_pProto->size() : 0;	}

		void		SetFlag(long flag)	{ m_lFlag = flag;	}
		long		GetFlag()		{ return m_lFlag;	}

		void		AddFlag(long bit);
		void		RemoveFlag(long bit);

		DWORD		GetWearFlag()		{ return m_pProto ? m_pProto->wear_flags() : 0; }
		DWORD		GetAntiFlag()		{ return m_pProto ? m_pProto->anti_flags() : 0; }
		DWORD		GetImmuneFlag()		{ return m_pProto ? m_pProto->immune_flags() : 0; }

		void		SetVID(DWORD vid)	{ m_dwVID = vid;	}
		DWORD		GetVID()		{ return m_dwVID;	}

		bool		SetCount(DWORD count);
		DWORD		GetCount();

		// GetVnum°ú GetOriginalVnum¿¡ ´ëÇÑ comment
		// GetVnumÀº Masking µÈ VnumÀÌ´Ù. ÀÌ¸¦ »ç¿ëÇÔÀ¸·Î½á, ¾ÆÀÌÅÛÀÇ ½ÇÁ¦ VnumÀº 10ÀÌÁö¸¸, VnumÀÌ 20ÀÎ °ÍÃ³·³ µ¿ÀÛÇÒ ¼ö ÀÖ´Â °ÍÀÌ´Ù.
		// Masking °ªÀº ori_to_new.txt¿¡¼­ Á¤ÀÇµÈ °ªÀÌ´Ù.
		// GetOriginalVnumÀº ¾ÆÀÌÅÛ °íÀ¯ÀÇ VnumÀ¸·Î, ·Î±× ³²±æ ¶§, Å¬¶óÀÌ¾ðÆ®¿¡ ¾ÆÀÌÅÛ Á¤º¸ º¸³¾ ¶§, ÀúÀåÇÒ ¶§´Â ÀÌ VnumÀ» »ç¿ëÇÏ¿©¾ß ÇÑ´Ù.
		// 
		DWORD		GetVnum() const		{ return m_dwMaskVnum ? m_dwMaskVnum : m_dwVnum;	}
		DWORD		GetDisplayVnum() const { return GetData() ? GetData() : GetVnum(); }
		DWORD		GetOriginalVnum() const		{ return m_dwVnum;	}
		BYTE		GetType() const		{ return m_pProto ? m_pProto->type() : 0;	}
		BYTE		GetSubType() const	{ return m_pProto ? m_pProto->sub_type() : 0;	}
		BYTE		GetLimitType(DWORD idx) const { return m_pProto ? m_pProto->limits(idx).type() : 0;	}
		long		GetLimitValue(DWORD idx) const { return m_pProto ? m_pProto->limits(idx).value() : 0;	}

		long		GetValue(DWORD idx) const;

		void		SetCell(LPCHARACTER ch, WORD pos)	{ m_pOwner = ch, m_wCell = pos;	}
		WORD		GetCell()				{ return m_wCell;	}

#ifdef __SKIN_SYSTEM__
		bool		IsEquippedInBuffiSkinCell();
		bool		IsBuffiSkinCell(WORD cell);
#endif

		LPITEM		RemoveFromCharacter();
		bool		AddToCharacter(LPCHARACTER ch, ::TItemPos Cell);
		LPCHARACTER	GetOwner()		{ return m_pOwner; }
		bool		CanStackWith(LPITEM pOtherItem, bool bCheckSocket = true);

		LPITEM		RemoveFromGround();
		bool		AddToGround(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck = false);

		int			FindEquipCell(LPCHARACTER ch, int bCandidateCell = -1);
		bool		IsEquipped() const		{ return m_bEquipped;	}
		bool		EquipTo(LPCHARACTER ch, BYTE bWearCell);
		bool		IsEquipable() const;

		int			GetScrollRefineType() const;
		bool		CanUsedBy(LPCHARACTER ch);

		bool		DistanceValid(LPCHARACTER ch);

		void		UpdatePacket();

		void		SetExchanging(bool isOn = true);
		bool		IsExchanging()		{ return m_bExchanging;	}

		bool		IsTwohanded();

		bool		IsPolymorphItem();

		void		ModifyPoints(bool bAdd, LPCHARACTER pkChr = NULL, float fFactor = 1.0f);	// ¾ÆÀÌÅÛÀÇ È¿°ú¸¦ Ä³¸¯ÅÍ¿¡ ºÎ¿© ÇÑ´Ù. bAdd°¡ falseÀÌ¸é Á¦°ÅÇÔ

		bool		CreateSocket(BYTE bSlot, BYTE bGold);
		const long *	GetSockets()		{ return &m_alSockets[0];	}
		long		GetSocket(int i) const	{ return m_alSockets[i];	}

		void		SetSockets(const long* sockets);
		void		SetSockets(const ::google::protobuf::RepeatedField<::google::protobuf::int32> sockets);
#ifdef __ENABLE_FULL_LOGS__
		void		SetSocket(int i, long v, bool bLog = true);
#else
		void		SetSocket(int i, long v, bool bLog = false);
#endif

		int		GetSocketCount();
		bool		AddSocket();

		const network::TItemAttribute* GetAttributes()		{ return m_aAttr;	}
		const network::TItemAttribute& GetAttribute(int i)	{ return m_aAttr[i];	}

		BYTE		GetAttributeType(int i)	{ return m_aAttr[i].type();	}
		short		GetAttributeValue(int i){ return m_aAttr[i].value();	}

		void		SetAttributes(const ::google::protobuf::RepeatedPtrField<network::TItemAttribute>& c_pAttribute);
		
		int		FindAttribute(BYTE bType);
		bool		RemoveAttributeAt(int index);
		bool		RemoveAttributeType(BYTE bType);

		bool		HasAttr(BYTE bApply);
#ifdef ITEM_RARE_ATTR
		bool		HasRareAttr(BYTE bApply);
#endif
		void		SetDestroyEvent(LPEVENT pkEvent);
		void		StartDestroyEvent(int iSec=60);

		DWORD		GetRefinedVnum() const	{ return m_pProto ? m_pProto->refined_vnum() : 0; }
		DWORD		GetRefineFromVnum();
		int		GetRefineLevel() const;

		void		SetSkipSave(bool b)	{ m_bSkipSave = b; }
		bool		GetSkipSave()		{ return m_bSkipSave; }

		bool		IsOwnership(LPCHARACTER ch);
		void		SetOwnership(LPCHARACTER ch, int iSec = 10);
		void		SetOwnershipEvent(LPEVENT pkEvent);

#ifdef __MARK_NEW_ITEM_SYSTEM__
		void		SetLastOwnerPID(DWORD dwPID)	{ m_dwLastOwnerPID = dwPID; }
#endif
		DWORD		GetLastOwnerPID()				{ return m_dwLastOwnerPID; }

		int		GetAttributeSetIndex(); // ¼Ó¼º ºÙ´Â°ÍÀ» ÁöÁ¤ÇÑ ¹è¿­ÀÇ ¾î´À ÀÎµ¦½º¸¦ »ç¿ëÇÏ´ÂÁö µ¹·ÁÁØ´Ù.
		void		AlterToMagicItem();
		void		AlterToSocketItem(int iSocketCount);

		WORD		GetRefineSet()		{ return m_pProto ? m_pProto->refine_set() : 0;	}
		bool		IsRefinedOtherItem() const;

		void		StartUniqueExpireEvent();
		void		SetUniqueExpireEvent(LPEVENT pkEvent);

		void		StartTimerBasedOnWearExpireEvent();
		void		SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent);

		void		StartRealTimeExpireEvent();
		bool		IsRealTimeItem();

		void		StopUniqueExpireEvent();
		void		StopTimerBasedOnWearExpireEvent();
		void		StopAccessorySocketExpireEvent();

		//			ÀÏ´Ü REAL_TIME°ú TIMER_BASED_ON_WEAR ¾ÆÀÌÅÛ¿¡ ´ëÇØ¼­¸¸ Á¦´ë·Î µ¿ÀÛÇÔ.
		int			GetDuration();

		int		GetAttributeCount();
		void		ClearAttribute();
		void		ClearAllAttribute();
		void		ChangeAttribute(const int* aiChangeProb=NULL);
		void		AddAttribute();
		void		AddAttribute(BYTE bType, short sValue);

		void		ApplyAddon(int iAddonType);

		int		GetSpecialGroup() const;
		bool	IsSameSpecialGroup(const LPITEM item) const;

		// ACCESSORY_REFINE
		// ¾×¼¼¼­¸®¿¡ ±¤»êÀ» ÅëÇØ ¼ÒÄÏÀ» Ãß°¡
		bool		IsAccessoryForSocket();

		BYTE	GetAccessorySocketGradeTotal();
		BYTE	GetAccessorySocketGrade(bool countPerma);
		BYTE	GetAccessorySocketMaxGrade();
		int		GetAccessorySocketDownGradeTime();
		BYTE	GetAccessorySocketType();
		DWORD	GetAccessorySocketByType(BYTE type, bool isPerma);

		void		SetAccessorySocketType(BYTE bType);
		void		SetAccessorySocketGrade(BYTE iGrade, bool setPerma);
		void		SetAccessorySocketMaxGrade(int iMaxGrade);
		void		SetAccessorySocketDownGradeTime(DWORD time);

		void		AccessorySocketDegrade();

		// ¾Ç¼¼»ç¸® ¸¦ ¾ÆÀÌÅÛ¿¡ ¹Û¾ÒÀ»¶§ Å¸ÀÌ¸Ó µ¹¾Æ°¡´Â°Í( ±¸¸®, µî )
		void		StartAccessorySocketExpireEvent();
		void		SetAccessorySocketExpireEvent(LPEVENT pkEvent);

		bool		CanPutInto(LPITEM item);
		// END_OF_ACCESSORY_REFINE

		void		CopyAttributeTo(LPITEM pItem);
		void		CopySocketTo(LPITEM pItem);

#ifdef ITEM_RARE_ATTR
		int			GetRareAttrCount();
		bool		AddRareAttribute();
		bool		ChangeRareAttribute();
#endif
		
		void		AttrLog();

		void		Lock(bool f) { m_isLocked = f; }
		bool		isLocked() const { return m_isLocked; }

	// private :
	public:
		void		SetAttribute(int i, BYTE bType, short sValue);
		void		SetForceAttribute(int i, BYTE bType, short sValue);


	public:
		WORD	GetHorseUsedBottles(BYTE bBonusID);
		void	SetHorseUsedBottles(BYTE bBonusID, WORD wCount);
		BYTE	GetHorseBonusLevel(BYTE bBonusID);
		void	SetHorseBonusLevel(BYTE bBonusID, BYTE bLevel);

	public:
		void	DisableItem();
		void	EnableItem();
		bool	IsItemDisabled() const;

	private:
		bool	m_bIsDisabledItem;
		
	protected:
		bool		EquipEx(bool is_equip);
		bool		Unequip();

		void		AddAttr(BYTE bApply, BYTE bLevel);
		void		PutAttribute(const int * aiAttrPercentTable);
	public:
		void		PutAttributeWithLevel(BYTE bLevel);

	protected:
		friend class CInputDB;
		bool		OnAfterCreatedItem();			// ¼­¹ö»ó¿¡ ¾ÆÀÌÅÛÀÌ ¸ðµç Á¤º¸¿Í ÇÔ²² ¿ÏÀüÈ÷ »ý¼º(·Îµå)µÈ ÈÄ ºÒ¸®¿ì´Â ÇÔ¼ö.

	public:
		bool		IsRideItem();

		void		ClearMountAttributeAndAffect();
		bool		IsNewMountItem();

		bool		IsNormalEquipItem() const;

		DWORD		GetRealImmuneFlag();

#ifdef __DRAGONSOUL__
	public:
		bool		IsDragonSoul();
#ifdef DS_TIME_ELIXIR_FIX
		int			GiveMoreTime_Per(float fPercent, int time = -1);
#else
		int			GiveMoreTime_Per(float fPercent);
#endif
		int			GiveMoreTime_Fix(DWORD dwTime);
#endif

#ifdef CRYSTAL_SYSTEM
	public:
		void		start_crystal_use(bool set_last_use_time = true);
		void		stop_crystal_use();

		bool		is_crystal_using() const { return _crystal_timeout_event != nullptr; }
		DWORD		get_crystal_duration() const;

		void		restore_crystal_energy(DWORD restore_duration);
		void		set_crystal_grade(const std::shared_ptr<network::TCrystalProto>& proto);

		void		on_crystal_timeout_event();

	private:
		void		start_crystal_timeout_event();
		void		clear_crystal_char_events();
#endif

	private:
		network::TItemTable const * m_pProto;		// ÇÁ·ÎÅä Å¸ÀÙ

		DWORD		m_dwVnum;
		LPCHARACTER	m_pOwner;

		BYTE		m_bWindow;		// ÇöÀç ¾ÆÀÌÅÛÀÌ À§Ä¡ÇÑ À©µµ¿ì 
		DWORD		m_dwID;			// °íÀ¯¹øÈ£
		bool		m_bEquipped;	// ÀåÂø µÇ¾ú´Â°¡?
		DWORD		m_dwVID;		// VID
		WORD		m_wCell;		// À§Ä¡
		DWORD		m_dwCount;		// °³¼ö
		long		m_lFlag;		// Ãß°¡ flag
		DWORD		m_dwLastOwnerPID;	// ¸¶Áö¸· °¡Áö°í ÀÖ¾ú´ø »ç¶÷ÀÇ PID

		bool		m_bExchanging;	///< ÇöÀç ±³È¯Áß »óÅÂ 

		long		m_alSockets[ITEM_SOCKET_MAX_NUM];	// ¾ÆÀÌÅÛ ¼ÒÄ¹
		network::TItemAttribute	m_aAttr[ITEM_ATTRIBUTE_MAX_NUM];

		LPEVENT		m_pkDestroyEvent;
		LPEVENT		m_pkExpireEvent;
		LPEVENT		m_pkUniqueExpireEvent;
		LPEVENT		m_pkTimerBasedOnWearExpireEvent;
		LPEVENT		m_pkRealTimeExpireEvent;
		LPEVENT		m_pkAccessorySocketExpireEvent;
		LPEVENT		m_pkOwnershipEvent;

		DWORD		m_dwOwnershipPID;

		bool		m_bSkipSave;

		bool		m_isLocked;
		
		DWORD		m_dwMaskVnum;
		DWORD		m_dwSIGVnum;
	public:
		void SetSIGVnum(DWORD dwSIG)
		{
			m_dwSIGVnum = dwSIG;
		}
		DWORD	GetSIGVnum() const
		{
			return m_dwSIGVnum;
		}

	private:
		enum EGMOwnerTypes {
			GM_OWNER_UNSET,
			GM_OWNER_PLAYER,
			GM_OWNER_GM,
		};
		BYTE		m_bIsGMOwner;
	public:
		bool		IsGMOwner() const;
		void		SetGMOwner(bool bGMOwner)	{ m_bIsGMOwner = bGMOwner ? GM_OWNER_GM : GM_OWNER_PLAYER; }

#ifdef __ANIMAL_SYSTEM__
	public:
		enum EAnimalStatus {
			ANIMAL_STATUS_MAIN = 0,
			ANIMAL_STATUS1,
			ANIMAL_STATUS2,
			ANIMAL_STATUS3,
			ANIMAL_STATUS4,

			ANIMAL_STATUS_COUNT = 4,
		};

		const TAnimalLevelData*	Animal_GetLevelData();
		bool	Animal_IsAnimal() const;
		void	Animal_SetLevel(BYTE bLevel);
		BYTE	Animal_GetLevel();
		void	Animal_GiveEXP(long lExp);
		void	Animal_SetEXP(long lExp);
		long	Animal_GetEXP();
		long	Animal_GetMaxEXP();
		void	Animal_SetStatusPoints(int iPoints);
		int		Animal_GetStatusPoints();
		void	Animal_SetStatus(BYTE bStatusID, short sValue);
		short	Animal_GetStatus(BYTE bStatusID);
		short	Animal_GetApplyStatus(BYTE bStatusID, BYTE* pbRetType = NULL);
		void	Animal_GiveBuff(bool bAdd = true, LPCHARACTER pkTarget = NULL);
		void	Animal_GiveBuff(BYTE bStatusID, bool bAdd, LPCHARACTER pkTarget = NULL);
		void	Animal_IncreaseStatus(BYTE bStatusID);
		
		BYTE	Animal_GetType() const;
		void	Animal_SummonPacket(const char* c_pszName);
		void	Animal_LevelPacket();
		void	Animal_ExpPacket();
		void	Animal_StatsPacket();
		void	Animal_UnsummonPacket();
#endif

	public:
		bool	CanPutItemIntoShop(bool bMessage = true);

	public:
		void		SetData(int iData);
		int			GetData() const { return m_iData; }
		void  SetRealOwnerPID(DWORD dwOwnerPID) { m_dwRealOwnerPID = dwOwnerPID; }
		DWORD  GetRealOwnerPID() const    { return m_dwRealOwnerPID; }

	private:
		int			m_iData;
		DWORD  m_dwRealOwnerPID;

	public:
		bool IsPotionItem(bool bOnlyHP = false, bool bCheckSpecialPotion = false) const;
		bool IsAutoPotionItem(bool bOnlyHP = false) const;
#ifdef INFINITY_ITEMS
		bool IsInfinityItem() const;
#endif

#ifdef __COSTUME_ACCE__
	public:
		BYTE	AcceCostumeGetReceptiveGrade();
#endif

#ifdef __ALPHA_EQUIP__
	public:
		int GetAlphaEquipMinValue();
		int GetAlphaEquipMaxValue();

		void RerollAlphaEquipValue();
		bool UpgradeAlphaEquipValue();

		void LoadAlphaEquipValue(int iValue)	{ m_iAlphaEquipValue = iValue; }
		int GetRealAlphaEquipValue() const		{ return m_iAlphaEquipValue; }

		void SetAlphaEquipValue(int iValue);
		int	GetAlphaEquipValue() const;

		void SetAlphaEquip(bool bIsAlpha);
		bool IsAlphaEquip() const;

	private:
		int	m_iAlphaEquipValue;
#endif

	// private:
		// DWORD m_dwCooltime;

	public:
		bool	IsHelperSpawnItem() const;

		void	SetItemCooltime(int lCooltime = 0);
		int		GetItemCooltime();
		// bool	IsCooltime();

#ifdef __PET_ADVANCED__
	public:
		bool			IsAdvancedPet() const { return GetType() == ITEM_PET_ADVANCED && GetSubType() == static_cast<uint32_t>(EPetAdvancedItem::SUMMON); }
		CPetAdvanced* GetAdvancedPet() const { return m_petAdvanced; }

	private:
		CPetAdvanced* m_petAdvanced;
#endif

#ifdef CRYSTAL_SYSTEM
	private:
		LPEVENT	_crystal_timeout_event;
		LPEVENT _crystal_update_event;

		LPCHARACTER _crystal_event_char;
		DWORD _crystal_event_handle;
		DWORD _crystal_event_destroy_handle;

		DWORD _crystal_last_use_time;
#endif
};

EVENTINFO(item_event_info)
{
	LPITEM item;
	char szOwnerName[CHARACTER_NAME_MAX_LEN];

	item_event_info() 
	: item( 0 )
	{
		::memset( szOwnerName, 0, CHARACTER_NAME_MAX_LEN );
	}
};

EVENTINFO(item_vid_event_info)
{
	DWORD item_vid;

	item_vid_event_info() 
	: item_vid( 0 )
	{
	}
};

#endif
