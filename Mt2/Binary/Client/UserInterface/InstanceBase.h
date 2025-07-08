#pragma once

#include "../gamelib/RaceData.h"
#include "../gamelib/ActorInstance.h"

#include "AffectFlagContainer.h"

class CInstanceBase
{	
	public:
		struct SCreateData
		{	
#ifdef __PRESTIGE__
			BYTE	m_bPrestigeLevel;
#endif
			BYTE	m_bType;
			DWORD	m_dwStateFlags;
			DWORD	m_dwLevel;
			DWORD	m_dwVID;
			DWORD	m_dwRace;
			DWORD	m_dwAtkSpd;
			long	m_lPosY;
			DWORD	m_dwEmpireID;
			DWORD	m_dwGuildID;
			DWORD	m_dwMovSpd;
			FLOAT	m_fRot;
			long	m_lPosX;
			DWORD	m_dwArmor;
			DWORD	m_dwWeapon;
			DWORD	m_dwHair;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			DWORD   m_dwAcce;
#endif
#ifdef ENABLE_ALPHA_EQUIP
			int		m_iWeaponAlphaEquip;
#endif

			DWORD	m_dwMountVnum;
			
			int		m_iAlignment;
			BYTE	m_byPKMode;
			CAffectFlagContainer	m_kAffectFlags;

			std::string m_stName;

#ifdef COMBAT_ZONE
			BYTE					combat_zone_rank;
			DWORD					combat_zone_points;
#endif

			bool	m_isMain;
#ifdef CHANGE_SKILL_COLOR
			DWORD	m_dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif
		};

	public:
		typedef DWORD TType;

		enum EDirection
		{
			DIR_NORTH,
			DIR_NORTHEAST,
			DIR_EAST,
			DIR_SOUTHEAST,
			DIR_SOUTH,
			DIR_SOUTHWEST,
			DIR_WEST,
			DIR_NORTHWEST,
			DIR_MAX_NUM,
		};

		enum
		{
			FUNC_WAIT,
			FUNC_MOVE,
			FUNC_ATTACK,
			FUNC_COMBO,
			FUNC_MOB_SKILL,
			FUNC_EMOTION,
			FUNC_SKILL = 0x80,
		};

		enum
		{
			AFFECT_GAMEMASTER,
			AFFECT_INVISIBILITY,
			AFFECT_SPAWN,

			AFFECT_POISON,
			AFFECT_SLOW,
			AFFECT_STUN,

			AFFECT_DUNGEON_READY,			// 던전에서 준비 상태
			AFFECT_SHOW_ALWAYS,				// AFFECT_DUNGEON_UNIQUE 에서 변경(클라이언트에서 컬링되지않음)

			AFFECT_BUILDING_CONSTRUCTION_SMALL,
			AFFECT_BUILDING_CONSTRUCTION_LARGE,
			AFFECT_BUILDING_UPGRADE,

			AFFECT_MOV_SPEED_POTION,		// 11
			AFFECT_ATT_SPEED_POTION,		// 12

			AFFECT_FISH_MIND,				// 13

			AFFECT_JEONGWI,					// 14 전귀혼
			AFFECT_GEOMGYEONG,				// 15 검경
			AFFECT_GEOMGYEONG_PERFECT,		// 16
			AFFECT_CHEONGEUN,				// 16 천근추
			AFFECT_CHEONGEUN_PERFECT,
			AFFECT_GYEONGGONG,				// 17 경공술
			AFFECT_EUNHYEONG,				// 18 은형법
			AFFECT_GWIGEOM,					// 19 귀검
			AFFECT_GWIGEOM_PERFECT,
			AFFECT_GONGPO,					// 20 공포
			AFFECT_GONGPO_PERFECT,
			AFFECT_JUMAGAP,					// 21 주마갑
			AFFECT_HOSIN,					// 22 호신
			AFFECT_HOSIN_PERFECT,
			AFFECT_BOHO,					// 23 보호
			AFFECT_KWAESOK,					// 24 쾌속
			AFFECT_HEUKSIN,					// 25 흑신수호
			AFFECT_HEUKSIN_PERFECT,
			AFFECT_MUYEONG,					// 26 무영진
			AFFECT_REVIVE_INVISIBILITY,		// 27 부활 무적
			AFFECT_FIRE,					// 28 지속 불
			AFFECT_GICHEON,					// 29 기천 대공
			AFFECT_GICHEON_PERFECT,
			AFFECT_JEUNGRYEOK,				// 30 증력술 
			AFFECT_DASH,					// 31 대쉬
			AFFECT_PABEOP,					// 32 파법술
			AFFECT_FALLEN_CHEONGEUN,		// 33 다운 그레이드 천근추
			AFFECT_POLYMORPH,				// 34 폴리모프
			AFFECT_WAR_FLAG1,				// 35
			AFFECT_WAR_FLAG2,				// 36
			AFFECT_WAR_FLAG3,				// 37
			AFFECT_CHINA_FIREWORK,			// 38

#ifdef ENABLE_WOLFMAN
			AFFECT_BLEEDING,
			AFFECT_RED_POSSESSION,
			AFFECT_BLUE_POSSESSION,
#endif

			AFFECT_MOVE_THROUGH_MONSTER,

#ifdef ENABLE_MELEY_LAIR_DUNGEON
			AFFECT_STATUE1,
			AFFECT_STATUE2,
			AFFECT_STATUE3,
			AFFECT_STATUE4,
#endif

#ifdef ENABLE_HYDRA_DUNGEON
			AFFECT_HYDRA,
#endif

#ifdef ENABLE_AUCTION
			AFFECT_AUCTION_SHOP_OWNER,
#endif

			AFFECT_NUM = 64,

			AFFECT_HWAYEOM = AFFECT_GEOMGYEONG,
		};

		enum
		{
			NEW_AFFECT_MOV_SPEED            = 200,
			NEW_AFFECT_ATT_SPEED,
			NEW_AFFECT_ATT_GRADE,
			NEW_AFFECT_INVISIBILITY,
			NEW_AFFECT_STR,
			NEW_AFFECT_DEX,                 // 205
			NEW_AFFECT_CON,
			NEW_AFFECT_INT,
			NEW_AFFECT_FISH_MIND_PILL,

			NEW_AFFECT_POISON,
			NEW_AFFECT_STUN,                // 210
			NEW_AFFECT_SLOW,
			NEW_AFFECT_DUNGEON_READY,
			NEW_AFFECT_DUNGEON_UNIQUE,

			NEW_AFFECT_BUILDING,
			NEW_AFFECT_REVIVE_INVISIBLE,    // 215
			NEW_AFFECT_FIRE,
			NEW_AFFECT_CAST_SPEED,
			NEW_AFFECT_HP_RECOVER_CONTINUE,
			NEW_AFFECT_SP_RECOVER_CONTINUE, 

			NEW_AFFECT_POLYMORPH,           // 220
			NEW_AFFECT_MOUNT,

			NEW_AFFECT_WAR_FLAG,            // 222

			NEW_AFFECT_BLOCK_CHAT,          // 223
			NEW_AFFECT_CHINA_FIREWORK,

			NEW_AFFECT_BOW_DISTANCE,        // 225

			NEW_AFFECT_EXP_BONUS         = 500, // °æÇèÀÇ ¹ÝÁö
			NEW_AFFECT_ITEM_BONUS        = 501, // µµµÏÀÇ Àå°©
			NEW_AFFECT_SAFEBOX           = 502, // PREMIUM_SAFEBOX,
			NEW_AFFECT_AUTOLOOT          = 503, // PREMIUM_AUTOLOOT,
			NEW_AFFECT_FISH_MIND         = 504, // PREMIUM_FISH_MIND,
			NEW_AFFECT_MARRIAGE_FAST     = 505, // ¿ø¾ÓÀÇ ±êÅÐ (±Ý½½),
			NEW_AFFECT_GOLD_BONUS        = 506,

		    NEW_AFFECT_MALL              = 510, // ¸ô ¾ÆÀÌÅÛ ¿¡ÆåÆ®
			NEW_AFFECT_NO_DEATH_PENALTY  = 511, // ¿ë½ÅÀÇ °¡È£ (°æÇèÄ¡ ÆÐ³ÎÆ¼¸¦ ÇÑ¹ø ¸·¾ÆÁØ´Ù)
			NEW_AFFECT_SKILL_BOOK_BONUS  = 512, // ¼±ÀÎÀÇ ±³ÈÆ (Ã¥ ¼ö·Ã ¼º°ø È®·üÀÌ 50% Áõ°¡)
			NEW_AFFECT_SKILL_BOOK_NO_DELAY  = 513, // ÁÖ¾È ¼ú¼­ (Ã¥ ¼ö·Ã µô·¹ÀÌ ¾øÀ½)

			NEW_AFFECT_EXP_BONUS_EURO_FREE = 516, // °æÇèÀÇ ¹ÝÁö (À¯·´ ¹öÀü 14 ·¹º§ ÀÌÇÏ ±âº» È¿°ú)
			NEW_AFFECT_EXP_BONUS_EURO_FREE_UNDER_15 = 517,

#ifdef COMBAT_ZONE
			NEW_AFFECT_COMBAT_ZONE_POTION = 519,
#endif

			NEW_AFFECT_AUTO_HP_RECOVERY		= 534,		// ÀÚµ¿¹°¾à HP
			NEW_AFFECT_AUTO_SP_RECOVERY		= 535,		// ÀÚµ¿¹°¾à SP

			NEW_AFFECT_MOVE_THROUGH_MONSTER = 536,

#ifdef ENABLE_DRAGONSOUL
			NEW_AFFECT_DRAGON_SOUL_QUALIFIED = 540,
			NEW_AFFECT_DRAGON_SOUL_DECK1 = 541,
			NEW_AFFECT_DRAGON_SOUL_DECK2 = 542,
#endif
			
			NEW_AFFECT_BLEND = 555,

			NEW_AFFECT_RAMADAN_ABILITY = 300,
			NEW_AFFECT_RAMADAN_RING    = 301,			// ¶ó¸¶´Ü ÀÌº¥Æ®¿ë Æ¯¼ö¾ÆÀÌÅÛ ÃÊ½Â´ÞÀÇ ¹ÝÁö Âø¿ë À¯¹«

			NEW_AFFECT_NOG_POCKET_ABILITY = 302,

			NEW_AFFECT_RAMADAN1 = 304,
			NEW_AFFECT_RAMADAN2 = 305,
	
#ifdef ENABLE_VOTE4BUFF
			NEW_AFFECT_VOTE4BUFF = 600,
#endif
			NEW_AFFECT_ALIGNMENT_GOOD = 601,
			NEW_AFFECT_ALIGNMENT_BAD = 602,
#ifdef ENABLE_HYDRA_DUNGEON
			NEW_AFFECT_HYDRA = 603,
#endif

#ifdef ENABLE_RUNE_SYSTEM
			NEW_AFFECT_RUNE_MOUNT_PARALYZE = 606,
#endif

#ifdef ENABLE_RUNE_AFFECT_ICONS
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_1 = 901,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_2,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_3,

			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_13,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_14,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_15,

			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_25,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_26,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_27,

			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_37,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_38,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_39,

			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_49,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_50,
			NEW_AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_51,
#endif
			NEW_AFFECT_QUEST_START_IDX   = 1000,
		};

		enum
		{
			STONE_SMOKE1 = 0,	// 99%
			STONE_SMOKE2 = 1,	// 85%
			STONE_SMOKE3 = 2,	// 80%
			STONE_SMOKE4 = 3,	// 60%
			STONE_SMOKE5 = 4,	// 45%
			STONE_SMOKE6 = 5,	// 40%
			STONE_SMOKE7 = 6,	// 20%
			STONE_SMOKE8 = 7,	// 10%
			STONE_SMOKE_NUM = 4,
		};

		enum EBuildingAffect
		{
			BUILDING_CONSTRUCTION_SMALL = 0,
			BUILDING_CONSTRUCTION_LARGE = 1,
			BUILDING_UPGRADE = 2,
		};

		enum
		{
			WEAPON_DUALHAND,
			WEAPON_ONEHAND,
			WEAPON_TWOHAND,
			WEAPON_NUM,
		};

		enum
		{
			EMPIRE_NONE,
			EMPIRE_A,
			EMPIRE_B,
			EMPIRE_C,
			EMPIRE_NUM,
		};

		enum
		{	
			NAMECOLOR_MOB,
			NAMECOLOR_NPC,
			NAMECOLOR_PC,
			NAMECOLOR_PC_END = NAMECOLOR_PC + EMPIRE_NUM,							
			NAMECOLOR_NORMAL_MOB,
			NAMECOLOR_NORMAL_NPC,
			NAMECOLOR_NORMAL_PC,
			NAMECOLOR_NORMAL_PC_END = NAMECOLOR_NORMAL_PC + EMPIRE_NUM,
			NAMECOLOR_EMPIRE_MOB,
			NAMECOLOR_EMPIRE_NPC,
			NAMECOLOR_EMPIRE_PC,
			NAMECOLOR_EMPIRE_PC_END = NAMECOLOR_EMPIRE_PC + EMPIRE_NUM,
			NAMECOLOR_FUNC,
			NAMECOLOR_PK,
			NAMECOLOR_PVP,
			NAMECOLOR_PARTY,
			NAMECOLOR_WARP,
			NAMECOLOR_BOSS,
			NAMECOLOR_STONE,
			NAMECOLOR_WAYPOINT,	
			NAMECOLOR_EXTRA = NAMECOLOR_FUNC + 10,
			NAMECOLOR_NUM = NAMECOLOR_EXTRA + 10,
		};
				
		enum
		{
			ALIGNMENT_TYPE_WHITE,
			ALIGNMENT_TYPE_NORMAL,
			ALIGNMENT_TYPE_DARK,
		};

		enum
		{
			EMOTICON_EXCLAMATION	= 1,
			EMOTICON_FISH			= 11,
			EMOTICON_NUM			= 128,

			TITLE_NUM				= 9,
			TITLE_NONE				= 4,
		};

		enum	//¾Æ·¡ ¹øÈ£°¡ ¹Ù²î¸é registerEffect ÂÊµµ ¹Ù²Ù¾î Áà¾ß ÇÑ´Ù.
		{
			EFFECT_REFINED_NONE,

			EFFECT_SWORD_REFINED7,
			EFFECT_SWORD_REFINED8,
			EFFECT_SWORD_REFINED9,

			EFFECT_BOW_REFINED7,
			EFFECT_BOW_REFINED8,
			EFFECT_BOW_REFINED9,

			EFFECT_FANBELL_REFINED7,
			EFFECT_FANBELL_REFINED8,
			EFFECT_FANBELL_REFINED9,

			EFFECT_SMALLSWORD_REFINED7,
			EFFECT_SMALLSWORD_REFINED8,
			EFFECT_SMALLSWORD_REFINED9,

			EFFECT_SMALLSWORD_REFINED7_LEFT,
			EFFECT_SMALLSWORD_REFINED8_LEFT,
			EFFECT_SMALLSWORD_REFINED9_LEFT,

#ifdef ENABLE_ALPHA_EQUIP
			EFFECT_SWORD_ALPHA,
			EFFECT_BOW_ALPHA,
			EFFECT_FANBELL_ALPHA,
			EFFECT_SMALLSWORD_ALPHA,
			EFFECT_SMALLSWORD_LEFT_ALPHA,
#endif

			EFFECT_BODYARMOR_REFINED7,
			EFFECT_BODYARMOR_REFINED8,
			EFFECT_BODYARMOR_REFINED9,

			EFFECT_BODYARMOR_SPECIAL,	// °©¿Ê 4-2-1
			EFFECT_BODYARMOR_SPECIAL2,	// °©¿Ê 4-2-2

			//DRSHOP_V7
			EFFECT_WEAPON_SPD,
			EFFECT_WEAPON_SPD1,
			EFFECT_WEAPON_PUGN,
			EFFECT_WEAPON_PUGN2,
			EFFECT_WEAPON_SPADONE,
			EFFECT_WEAPON_ARC,
			EFFECT_WEAPON_CMP,
			EFFECT_WEAPON_VENT,

			//DRSHOP_V9
			EFFECT_WEAPON_SPD_2,
			EFFECT_WEAPON_SPD1_2,
			EFFECT_WEAPON_PUGN_2,
			EFFECT_WEAPON_PUGN2_2,
			EFFECT_WEAPON_SPADONE_2,
			EFFECT_WEAPON_ARC_2,
			EFFECT_WEAPON_CMP_2,
			EFFECT_WEAPON_VENT_2,
			
			// lv30
			EFFECT_WEAPON_FMS,
			EFFECT_WEAPON_RIB,
			EFFECT_WEAPON_KOZIK,
			EFFECT_WEAPON_KOZIK_2,
			EFFECT_WEAPON_JELONEK,
			EFFECT_WEAPON_ANTYK,
			EFFECT_WEAPON_JESION,
			
			// lv75
			EFFECT_WEAPON_ZATRUTY2,
			EFFECT_WEAPON_LWI,
			EFFECT_WEAPON_ZAL,
			EFFECT_WEAPON_SKRZYDLA,
			EFFECT_WEAPON_SKRZYDLA_2,
			EFFECT_WEAPON_KRUK,
			EFFECT_WEAPON_BAMBUS,
			
			// lv90
			EFFECT_WEAPON_TRYTON,
			EFFECT_WEAPON_SWIETY,
			EFFECT_WEAPON_PIEKIELNE,
			EFFECT_WEAPON_BEZDUSZNE,
			EFFECT_WEAPON_BEZDUSZNE_2,
			EFFECT_WEAPON_DIABLA_L,
			EFFECT_WEAPON_SZCZEKI,
			EFFECT_WEAPON_DIABLA_W,
			
			EFFECT_BODYARMOR_SPECIAL3,	// °©¿Ê 4-2-2
			
			EFFECT_WEAPON_ANGEL_SW,
			EFFECT_WEAPON_ANGEL_TW,
			EFFECT_WEAPON_ANGEL_DA,
			EFFECT_WEAPON_ANGEL_DA2,
			EFFECT_WEAPON_ANGEL_BO,
			EFFECT_WEAPON_ANGEL_BE,
			EFFECT_WEAPON_ANGEL_FA,
			EFFECT_WEAPON_DEMON_SW,
			EFFECT_WEAPON_DEMON_TW,
			EFFECT_WEAPON_DEMON_DA,
			EFFECT_WEAPON_DEMON_DA2,
			EFFECT_WEAPON_DEMON_BO,
			EFFECT_WEAPON_DEMON_BE,
			EFFECT_WEAPON_DEMON_FA,
			
			
			EFFECT_BODYARMOR_SPECIAL4,
			EFFECT_BODYARMOR_SPECIAL5,
			EFFECT_BODYARMOR_SPECIAL6,
			EFFECT_BODYARMOR_SPECIAL7,
			EFFECT_BODYARMOR_SPECIAL8,
			
			//July
			EFFECT_JULY_SW,
			EFFECT_WEAPON_JULY_TW,
			EFFECT_WEAPON_JULY_DA,
			EFFECT_WEAPON_JULY_DA2,
			EFFECT_WEAPON_JULY_BO,
			EFFECT_WEAPON_JULY_BE,
			EFFECT_WEAPON_JULY_FA,
			
			//august
			EFFECT_WEAPON_AUG_SW,
			EFFECT_WEAPON_AUG_SW_2,
			EFFECT_WEAPON_AUG_TW,
			EFFECT_WEAPON_AUG_DA,
			EFFECT_WEAPON_AUG_DA2,
			EFFECT_WEAPON_AUG_BO,
			EFFECT_WEAPON_AUG_BE,
			EFFECT_WEAPON_AUG_FA,
			
			//galaxy
			EFFECT_WEAPON_GAL_SW,
			EFFECT_WEAPON_GAL_SW_2,
			EFFECT_WEAPON_GAL_TW,
			EFFECT_WEAPON_GAL_DA,
			EFFECT_WEAPON_GAL_DA2,
			EFFECT_WEAPON_GAL_BO,
			EFFECT_WEAPON_GAL_BE,
			EFFECT_WEAPON_GAL_FA,
			
			//halloween2019
			EFFECT_WEAPON_HALLOWEEN_SW,
			EFFECT_WEAPON_HALLOWEEN_TW,
			EFFECT_WEAPON_HALLOWEEN_DA,
			EFFECT_WEAPON_HALLOWEEN_DA2,
			EFFECT_WEAPON_HALLOWEEN_BO,
			EFFECT_WEAPON_HALLOWEEN_BE,
			EFFECT_WEAPON_HALLOWEEN_FA,
			
			EFFECT_BODYARMOR_SPECIAL9,

			EFFECT_REFINED_NUM,
		};
		
		enum DamageFlag
		{
			DAMAGE_NORMAL	= (1<<0),
			DAMAGE_POISON	= (1<<1),
			DAMAGE_DODGE	= (1<<2),
			DAMAGE_BLOCK	= (1<<3),
			DAMAGE_PENETRATE= (1<<4),
			DAMAGE_CRITICAL = (1<<5),
#ifdef ENABLE_WOLFMAN
			DAMAGE_BLEEDING = (1<<6),
#endif
			// ¹Ý-_-»ç
		};

		enum
		{
			EFFECT_DUST,
			EFFECT_STUN,
			EFFECT_HIT,
			EFFECT_FLAME_ATTACK,
			EFFECT_FLAME_HIT,
			EFFECT_FLAME_ATTACH,
			EFFECT_ELECTRIC_ATTACK,
			EFFECT_ELECTRIC_HIT,
			EFFECT_ELECTRIC_ATTACH,
			EFFECT_SPAWN_APPEAR,
			EFFECT_SPAWN_DISAPPEAR,
			EFFECT_LEVELUP,
			EFFECT_SKILLUP,
			EFFECT_HPUP_RED,
			EFFECT_SPUP_BLUE,
			EFFECT_SPEEDUP_GREEN,
			EFFECT_DXUP_PURPLE,
			EFFECT_CRITICAL,
			EFFECT_PENETRATE,
			EFFECT_BLOCK,
			EFFECT_DODGE,
			EFFECT_FIRECRACKER,
			EFFECT_SPIN_TOP,
			EFFECT_WEAPON,
			EFFECT_WEAPON_END = EFFECT_WEAPON + WEAPON_NUM,
#ifdef ENABLE_LEGENDARY_SKILL
			EFFECT_WEAPON_LEGENDARY,
			EFFECT_WEAPON_LEGENDARY_END = EFFECT_WEAPON_LEGENDARY + WEAPON_NUM,
#endif
			EFFECT_AFFECT,
			EFFECT_AFFECT_GYEONGGONG = EFFECT_AFFECT + AFFECT_GYEONGGONG,
			EFFECT_AFFECT_KWAESOK = EFFECT_AFFECT + AFFECT_KWAESOK,
			EFFECT_AFFECT_END = EFFECT_AFFECT + AFFECT_NUM,
			EFFECT_EMOTICON,
			EFFECT_EMOTICON_END = EFFECT_EMOTICON + EMOTICON_NUM,
			EFFECT_SELECT,
			EFFECT_TARGET,
			EFFECT_EMPIRE,
			EFFECT_EMPIRE_END = EFFECT_EMPIRE + EMPIRE_NUM,
			EFFECT_HORSE_DUST,
			EFFECT_REFINED,
			EFFECT_REFINED_END = EFFECT_REFINED + EFFECT_REFINED_NUM,
			EFFECT_DAMAGE_TARGET,
			EFFECT_DAMAGE_NOT_TARGET,
			EFFECT_DAMAGE_SELFDAMAGE,
			EFFECT_DAMAGE_SELFDAMAGE2,
			EFFECT_DAMAGE_POISON,
			EFFECT_DAMAGE_MISS,
			EFFECT_DAMAGE_TARGETMISS,
			EFFECT_DAMAGE_CRITICAL,
			EFFECT_SUCCESS,
			EFFECT_FAIL,
			EFFECT_FR_SUCCESS,			
			EFFECT_LEVELUP_ON_14_FOR_GERMANY,	//·¹º§¾÷ 14ÀÏ¶§ ( µ¶ÀÏÀü¿ë )
			EFFECT_LEVELUP_UNDER_15_FOR_GERMANY,//·¹º§¾÷ 15ÀÏ¶§ ( µ¶ÀÏÀü¿ë )
			EFFECT_PERCENT_DAMAGE1,
			EFFECT_PERCENT_DAMAGE2,
			EFFECT_PERCENT_DAMAGE3,
			EFFECT_AUTO_HPUP,
			EFFECT_AUTO_SPUP,
			EFFECT_RAMADAN_RING_EQUIP,			// ÃÊ½Â´Þ ¹ÝÁö Âø¿ë ¼ø°£¿¡ ¹ßµ¿ÇÏ´Â ÀÌÆåÆ®
			EFFECT_HALLOWEEN_CANDY_EQUIP,		// ÇÒ·ÎÀ© »çÅÁ Âø¿ë ¼ø°£¿¡ ¹ßµ¿ÇÏ´Â ÀÌÆåÆ®
			EFFECT_HAPPINESS_RING_EQUIP,				// Çàº¹ÀÇ ¹ÝÁö Âø¿ë ¼ø°£¿¡ ¹ßµ¿ÇÏ´Â ÀÌÆåÆ®
			EFFECT_LOVE_PENDANT_EQUIP,				// Çàº¹ÀÇ ¹ÝÁö Âø¿ë ¼ø°£¿¡ ¹ßµ¿ÇÏ´Â ÀÌÆåÆ®
			EFFECT_TEMP,
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			EFFECT_ACCE_SUCESS_ABSORB,
			EFFECT_ACCE_EQUIP,
			EFFECT_ACCE_BACK,
#endif
#ifdef ENABLE_AGGREGATE_MONSTER_EFFECT
			EFFECT_AGGREGATE_MONSTER,
#endif

			EFFECT_HEALER,

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			EFFECT_ACCE_BACK_WING1,
			EFFECT_ACCE_BACK_WING2,
			EFFECT_ACCE_BACK_WING3,
			EFFECT_ACCE_BACK_WING4,
			EFFECT_ACCE_BACK_WING5,
			EFFECT_ACCE_BACK_WING6,
			EFFECT_ACCE_BACK_WING7,
#endif
#ifdef COMBAT_ZONE
			EFFECT_COMBAT_ZONE_POTION,
#endif
			EFEKT_BOSSA,
#ifdef ENABLE_PET_ADVANCED
			EFFECT_PET_LEVELUP,
#endif
			EFFECT_NUM,
		};

		enum
		{
			DUEL_NONE,
			DUEL_CANNOTATTACK,
			DUEL_START,
		};

	public:
		static void DestroySystem();
		static void CreateSystem(UINT uCapacity);
		static bool RegisterEffect(UINT eEftType, const char* c_szEftAttachBone, const char* c_szEftName, bool isCache, bool isAlwaysRender = false);
		static void RegisterTitleName(int iIndex, const char * c_szTitleName);
		static bool RegisterNameColor(UINT uIndex, UINT r, UINT g, UINT b);
		static bool RegisterTitleColor(UINT uIndex, UINT r, UINT g, UINT b);
		static bool ChangeEffectTexture(UINT eEftType, const char* c_szSrcFileName, const char* c_szDstFileName);

		static void SetDustGap(float fDustGap);
		static void SetHorseDustGap(float fDustGap);

		static void SetEmpireNameMode(bool isEnable);
		static const D3DXCOLOR& GetIndexedNameColor(UINT eNameColor);

	public:
		void SetMainInstance();

		void OnSelected();
		void OnUnselected();
		void OnTargeted();
		void OnUntargeted();

	protected:
		bool __IsExistMainInstance();
		bool __IsMainInstance();
		bool __MainCanSeeHiddenThing();
		float __GetBowRange();

	protected:
		DWORD	__AttachEffect(UINT eEftType);
		DWORD	__AttachEffect(char filename[128]);
		void	__DetachEffect(DWORD dwEID);

	public:		
		void CreateSpecialEffect(DWORD iEffectIndex);
		void AttachSpecialEffect(DWORD effect);

		void AttachShiningEffect(DWORD effect);
		void DetachShining();
		static bool RegisterShiningEffect(UINT eEftType, BYTE bWeaponType, const char* c_szEftName);

	protected:
		static std::string ms_astAffectEffectAttachBone[EFFECT_NUM];
		static DWORD ms_adwCRCAffectEffect[EFFECT_NUM];
		static float ms_fDustGap;
		static float ms_fHorseDustGap;

		std::vector<DWORD> m_vShinings;
		static std::map<DWORD, std::map<BYTE, std::vector<DWORD>>> m_mapShiningData;

	public:
		CInstanceBase();
		virtual ~CInstanceBase();

		bool LessRenderOrder(CInstanceBase* pkInst);

		void MountHorse(UINT eRace);
		void DismountHorse();		

		// ½ºÅ©¸³Æ®¿ë Å×½ºÆ® ÇÔ¼ö. ³ªÁß¿¡ ¾ø¿¡ÀÚ
		void SCRIPT_SetAffect(UINT eAffect, bool isVisible); 

		float CalculateDistanceSq3d(const TPixelPosition& c_rkPPosDst);

		// Instance Data
		bool IsFlyTargetObject();
		void ClearFlyTargetInstance();
		void SetFlyTargetInstance(CInstanceBase& rkInstDst);
		void AddFlyTargetInstance(CInstanceBase& rkInstDst);
		void AddFlyTargetPosition(const TPixelPosition& c_rkPPosDst);

		float GetFlyTargetDistance();

		void SetAlpha(float fAlpha);

		void DeleteBlendOut();

		void					AttachTextTail();
		void					DetachTextTail();
		void					UpdateTextTailLevel(DWORD level);

		void					RefreshTextTail();
		void					RefreshTextTailTitle();

		bool					Create(const SCreateData& c_rkCreateData);

		bool					CreateDeviceObjects();
		void					DestroyDeviceObjects();

		void					Destroy();

		void					Update();
		bool					UpdateDeleting();

		void					Transform();
		void					Deform();
		void					Render();
		void					RenderTrace();
		void					RenderToShadowMap();
		void					RenderCollision();
		void					RegisterBoundingSphere();

		// Temporary
		void					GetBoundBox(D3DXVECTOR3 * vtMin, D3DXVECTOR3 * vtMax);

		void					SetNameString(const char* c_szName, int len);
		bool					SetRace(DWORD dwRaceIndex);
		void					SetVirtualID(DWORD wVirtualNumber);
		void					SetVirtualNumber(DWORD dwVirtualNumber);
		void					SetInstanceType(int iInstanceType);
		void					SetAlignment(int iAlignment);
		void					SetPKMode(BYTE byPKMode);
		void					SetKiller(bool bFlag);
		void					SetPartyMemberFlag(bool bFlag);
		void					SetStateFlags(DWORD dwStateFlags);

		void					SetArmor(DWORD dwArmor);
		void					SetShape(DWORD eShape, float fSpecular=0.0f);
		void					SetHair(DWORD eHair);
#ifdef ENABLE_ALPHA_EQUIP
		bool					SetWeapon(DWORD eWeapon, int alphaEquipVal);
#else
		bool					SetWeapon(DWORD eWeapon, bool useSpecular = true);
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM		
		bool					SetAcce(DWORD eAcce);
#endif
		bool					ChangeArmor(DWORD dwArmor);
#ifdef ENABLE_ALPHA_EQUIP
		void					ChangeWeapon(DWORD eWeapon, int alphaEquipVal);
#else
		void					ChangeWeapon(DWORD eWeapon);
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void					ChangeAcce(DWORD eAcce);
#endif
#ifdef CHANGE_SKILL_COLOR
		void					ChangeSkillColor(const DWORD *dwSkillColor);
#endif
		void					ChangeHair(DWORD eHair);
		void					ChangeGuild(DWORD dwGuildID);
		DWORD					GetWeaponType();

		void					SetComboType(UINT uComboType);
		void					SetAttackSpeed(UINT uAtkSpd);
		void					SetMoveSpeed(UINT uMovSpd);
		void					SetRotationSpeed(float fRotSpd);

#ifdef COMBAT_ZONE
		bool					IsCombatZoneMap();
		void					SetCombatZonePoints(DWORD dwValue);
		DWORD					GetCombatZonePoints();
		void					SetCombatZoneRank(BYTE bValue);
		BYTE					GetCombatZoneRank();
#endif

		const char *			GetNameString();
		int						GetInstanceType();
		DWORD					GetPart(CRaceData::EParts part);
#ifdef ENABLE_ALPHA_EQUIP
		int						GetWeaponAlphaEquipVal();
#endif
		DWORD					GetShape();
		DWORD					GetRace();
		DWORD					GetVirtualID();
		DWORD					GetVirtualNumber();
		DWORD					GetEmpireID();
		DWORD					GetGuildID();
		int						GetAlignment();
		UINT					GetAlignmentGrade();
		int						GetAlignmentType();
		BYTE					GetPKMode();
		bool					IsKiller();
		bool					IsPartyMember();

		void					ActDualEmotion(CInstanceBase & rkDstInst, WORD dwMotionNumber1, WORD dwMotionNumber2);
		void					ActEmotion(DWORD dwMotionNumber);
		void					LevelUp();
		void					SkillUp();
		void					UseSpinTop();
		void					Revive();
		void					Stun();
		void					Die();
		void					Hide();
		void					Show();

#ifdef ENABLE_ZODIAC
		bool					CanAct(bool skipIsDeadItem = false);
#else
		bool					CanAct();
#endif
		bool					CanMove();
		bool					CanAttack();
		bool					CanUseSkill();
		bool					CanFishing();
		bool					IsConflictAlignmentInstance(CInstanceBase& rkInstVictim);
		bool					IsAttackableInstance(CInstanceBase& rkInstVictim);
		bool					IsTargetableInstance(CInstanceBase& rkInstVictim);
		bool					IsPVPInstance(CInstanceBase& rkInstVictim);
		bool					CanChangeTarget();
		bool					CanPickInstance();
		bool					CanViewTargetHP(CInstanceBase& rkInstVictim);


		// Movement
		BOOL					IsGoing();
		bool					NEW_Goto(const TPixelPosition& c_rkPPosDst, float fDstRot);
		void					EndGoing();

		void					SetRunMode();
		void					SetWalkMode();

		bool					IsAffect(UINT uAffect);
		BOOL					IsInvisibility();
		BOOL					IsParalysis();
		BOOL					IsGameMaster();
		BOOL					IsSameEmpire(CInstanceBase& rkInstDst);
		BOOL					IsBowMode();
		BOOL					IsHandMode();
		BOOL					IsFishingMode();
		BOOL					IsFishing();

		BOOL					IsWearingDress();
		BOOL					IsHoldingPickAxe();
		BOOL					IsMountingHorse();
		BOOL					IsNewMount();
		BOOL					IsForceVisible();
		BOOL					IsInSafe();
		BOOL					IsEnemy();
		BOOL					IsBoss();
		BOOL					IsStone();
		BOOL					IsResource();
		BOOL					IsNPC();
		BOOL					IsMount();
		BOOL					IsPet();
		BOOL					IsPC();
#ifdef ENABLE_FAKEBUFF
		BOOL					IsMyFakeBuff();
		BOOL					IsFakeBuff();
#endif
		BOOL					IsPoly();
		BOOL					IsWarp();
		BOOL					IsGoto();
		BOOL					IsObject();
		BOOL					IsDoor();
		BOOL					IsBuilding();
		BOOL					IsWoodenDoor();
		BOOL					IsStoneDoor();
		BOOL					IsFlag();
		BOOL					IsGuildWall();

		BOOL					IsDead();
		BOOL					IsStun();
		BOOL					IsSleep();
		BOOL					__IsSyncing();
		BOOL					IsWaiting();
		BOOL					IsWalking();
		BOOL					IsPushing();
		BOOL					IsAttacking();
		BOOL					IsActingEmotion();
		BOOL					IsAttacked();
		BOOL					IsKnockDown();
		BOOL					IsUsingSkill();
		BOOL					IsUsingMovingSkill();
		BOOL					CanCancelSkill();
		BOOL					CanAttackHorseLevel();

#ifdef __MOVIE_MODE__
		BOOL					IsMovieMode(); // ¿î¿µÀÚ¿ë ¿ÏÀüÈ÷ ¾Èº¸ÀÌ´Â°Å
#endif
		bool					NEW_CanMoveToDestPixelPosition(const TPixelPosition& c_rkPPosDst);

		void					NEW_SetAdvancingRotationFromPixelPosition(const TPixelPosition& c_rkPPosSrc, const TPixelPosition& c_rkPPosDst);
		void					NEW_SetAdvancingRotationFromDirPixelPosition(const TPixelPosition& c_rkPPosDir);
		bool					NEW_SetAdvancingRotationFromDestPixelPosition(const TPixelPosition& c_rkPPosDst);
		void					SetAdvancingRotation(float fRotation);

		void					EndWalking(float fBlendingTime=0.15f);
		void					EndWalkingWithoutBlending();

		// Battle
		void					SetEventHandler(CActorInstance::IEventHandler* pkEventHandler);

		void					PushUDPState(DWORD dwCmdTime, const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg);
		void					PushTCPState(DWORD dwCmdTime, const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg);
		void					PushTCPStateExpanded(DWORD dwCmdTime, const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg, UINT uTargetVID);

		void					NEW_Stop();

		bool					NEW_UseSkill(UINT uSkill, UINT uMot, UINT uMotLoopCount, bool isMovingSkill);
		void					NEW_Attack();
		void					NEW_Attack(float fDirRot);
		void					NEW_AttackToDestPixelPositionDirection(const TPixelPosition& c_rkPPosDst);
		bool					NEW_AttackToDestInstanceDirection(CInstanceBase& rkInstDst, IFlyEventHandler* pkFlyHandler);
		bool					NEW_AttackToDestInstanceDirection(CInstanceBase& rkInstDst);

		bool					NEW_MoveToDestPixelPositionDirection(const TPixelPosition& c_rkPPosDst);
		void					NEW_MoveToDestInstanceDirection(CInstanceBase& rkInstDst);
		void					NEW_MoveToDirection(float fDirRot);

		float					NEW_GetDistanceFromDirPixelPosition(const TPixelPosition& c_rkPPosDir);
		float					NEW_GetDistanceFromDestPixelPosition(const TPixelPosition& c_rkPPosDst);
		float					NEW_GetDistanceFromDestInstance(CInstanceBase& rkInstDst);

		float					NEW_GetRotation();
		float					NEW_GetRotationFromDestPixelPosition(const TPixelPosition& c_rkPPosDst);
		float					NEW_GetRotationFromDirPixelPosition(const TPixelPosition& c_rkPPosDir);
		float					NEW_GetRotationFromDestInstance(CInstanceBase& rkInstDst);

		float					NEW_GetAdvancingRotationFromDirPixelPosition(const TPixelPosition& c_rkPPosDir);
		float					NEW_GetAdvancingRotationFromDestPixelPosition(const TPixelPosition& c_rkPPosDst);
		float					NEW_GetAdvancingRotationFromPixelPosition(const TPixelPosition& c_rkPPosSrc, const TPixelPosition& c_rkPPosDst);

		BOOL					NEW_IsClickableDistanceDestPixelPosition(const TPixelPosition& c_rkPPosDst);
		BOOL					NEW_IsClickableDistanceDestInstance(CInstanceBase& rkInstDst);

		bool					NEW_GetFrontInstance(CInstanceBase ** ppoutTargetInstance, float fDistance);
		void					NEW_GetRandomPositionInFanRange(CInstanceBase& rkInstTarget, TPixelPosition* pkPPosDst);
		bool					NEW_GetInstanceVectorInFanRange(float fSkillDistance, CInstanceBase& rkInstTarget, std::vector<CInstanceBase*>* pkVct_pkInst);
		bool					NEW_GetInstanceVectorInCircleRange(float fSkillDistance, std::vector<CInstanceBase*>* pkVct_pkInst);

		void					NEW_SetOwner(DWORD dwOwnerVID);
		void					NEW_SyncPixelPosition(long & nPPosX, long & nPPosY);
		void					NEW_SyncCurrentPixelPosition();

		void					NEW_SetPixelPosition(const TPixelPosition& c_rkPPosDst);

		bool					NEW_IsLastPixelPosition();
		const TPixelPosition&	NEW_GetLastPixelPositionRef();


		// Battle
		BOOL					isNormalAttacking();
		BOOL					isComboAttacking();
		MOTION_KEY				GetNormalAttackIndex();
		DWORD					GetComboIndex();
		float					GetAttackingElapsedTime();
		void					InputNormalAttack(float fAtkDirRot);
		void					InputComboAttack(float fAtkDirRot);

		void					RunNormalAttack(float fAtkDirRot);
		void					RunComboAttack(float fAtkDirRot, DWORD wMotionIndex);

		CInstanceBase*			FindNearestVictim();
		BOOL					CheckAdvancing();


		bool					AvoidObject(const CGraphicObjectInstance& c_rkBGObj);		
		bool					IsBlockObject(const CGraphicObjectInstance& c_rkBGObj);
		void					BlockMovement();

	public:
		BOOL					CheckAttacking(CInstanceBase& rkInstVictim);
		void					ProcessHitting(DWORD dwMotionKey, CInstanceBase * pVictimInstance);
		void					ProcessHitting(DWORD dwMotionKey, BYTE byEventIndex, CInstanceBase * pVictimInstance);
		void					GetBlendingPosition(TPixelPosition * pPixelPosition);
		void					SetBlendingPosition(const TPixelPosition & c_rPixelPosition);

		int						HasAffect(DWORD index);

		// Fishing
		void					StartFishing(float frot);
		void					StopFishing();
		void					ReactFishing();
		void					CatchSuccess();
		void					CatchFail();
		BOOL					GetFishingRot(int * pirot);

		// Render Mode
		void					RestoreRenderMode();
		void					SetAddRenderMode();
		void					SetModulateRenderMode();
		void					SetRenderMode(int iRenderMode);
		void					SetAddColor(const D3DXCOLOR & c_rColor);

		// Position
		void					SCRIPT_SetPixelPosition(float fx, float fy);
		void					NEW_GetPixelPosition(TPixelPosition * pPixelPosition);

		// Rotation
		void					NEW_LookAtFlyTarget();
		void					NEW_LookAtDestInstance(CInstanceBase& rkInstDst);
		void					NEW_LookAtDestPixelPosition(const TPixelPosition& c_rkPPosDst);

		float					GetRotation();
		float					GetAdvancingRotation();
		void					SetRotation(float fRotation);
		void					BlendRotation(float fRotation, float fBlendTime = 0.1f);

		void					SetScale(float x, float y, float z);

		void					SetDirection(int dir);
		void					BlendDirection(int dir, float blendTime);
		float					GetDegreeFromDirection(int dir);

		// Motion
		//	Motion Deque
		BOOL					isLock();

		void					SetMotionMode(int iMotionMode);
		int						GetMotionMode(DWORD dwMotionIndex);

		// Motion
		//	Pushing Motion
		void					ResetLocalTime();
		void					SetLoopMotion(WORD wMotion, float fBlendTime=0.1f, float fSpeedRatio=1.0f);
		void					PushOnceMotion(WORD wMotion, float fBlendTime=0.1f, float fSpeedRatio=1.0f);
		void					PushLoopMotion(WORD wMotion, float fBlendTime=0.1f, float fSpeedRatio=1.0f);
		void					SetEndStopMotion();

		// Intersect
		bool					IntersectDefendingSphere();
		bool					IntersectBoundingBox();

		// Part
		//void					SetParts(const WORD * c_pParts);
		void					Refresh(DWORD dwMotIndex, bool isLoop);

		//void					AttachEffectByID(DWORD dwParentPartIndex, const char * c_pszBoneName, DWORD dwEffectID, int dwLife = CActorInstance::EFFECT_LIFE_INFINITE ); // ¼ö¸íÀº ms´ÜÀ§ÀÔ´Ï´Ù.
		//void					AttachEffectByName(DWORD dwParentPartIndex, const char * c_pszBoneName, const char * c_pszEffectName, int dwLife = CActorInstance::EFFECT_LIFE_INFINITE ); // ¼ö¸íÀº ms´ÜÀ§ÀÔ´Ï´Ù.

		float					GetDistance(CInstanceBase * pkTargetInst);
		float					GetDistance(const TPixelPosition & c_rPixelPosition);

		// ETC
		CActorInstance&			GetGraphicThingInstanceRef();
		CActorInstance*			GetGraphicThingInstancePtr();		
		
		bool __Background_IsWaterPixelPosition(const TPixelPosition& c_rkPPos);
		bool __Background_GetWaterHeight(const TPixelPosition& c_rkPPos, float* pfHeight);

		// 2004.07.25.myevan.ÀÌÆåÆ® ¾È³ª¿À´Â ¹®Á¦
		/////////////////////////////////////////////////////////////
		void __ClearAffectFlagContainer();
		void __ClearAffects();
		/////////////////////////////////////////////////////////////

		void __SetAffect(UINT eAffect, bool isVisible);
		
		void SetAffectFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer);

		void __SetNormalAffectFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer);		
		void __SetStoneSmokeFlagContainer(const CAffectFlagContainer& c_rkAffectFlagContainer);

		void SetEmoticon(UINT eEmoticon);		
		void SetFishEmoticon();
		bool IsPossibleEmoticon();

	protected:
		UINT					__LessRenderOrder_GetLODLevel();
		void					__Initialize();
		void					__InitializeRotationSpeed();

		void					__Create_SetName(const SCreateData& c_rkCreateData);
		void					__Create_SetWarpName(const SCreateData& c_rkCreateData);

		CInstanceBase*			__GetMainInstancePtr();
		CInstanceBase*			__FindInstancePtr(DWORD dwVID);

		bool  __FindRaceType(DWORD dwRace, BYTE* pbType);
		DWORD __GetRaceType();

		bool __IsShapeAnimalWear();
		BOOL __IsChangableWeapon(int iWeaponID);

		void __EnableSkipCollision();
		void __DisableSkipCollision();

		void __ClearMainInstance();

		void __Shaman_SetParalysis(bool isParalysis);
#ifdef ENABLE_LEGENDARY_SKILL
		void __Warrior_SetGeomgyeongAffect(bool isVisible, bool isLegendary);
#else
		void __Warrior_SetGeomgyeongAffect(bool isVisible);
#endif
		void __Assassin_SetEunhyeongAffect(bool isVisible);

		BOOL __CanProcessNetworkStatePacket();
		
		bool __IsInDustRange();

		// Emotion
		void __ProcessFunctionEmotion(DWORD dwMotionNumber, DWORD dwTargetVID, const TPixelPosition & c_rkPosDst);
		void __EnableChangingTCPState();
		void __DisableChangingTCPState();
		BOOL __IsEnableTCPProcess(UINT eCurFunc);

		// 2004.07.17.levites.isShow¸¦ ViewFrustumCheck·Î º¯°æ
		bool __CanRender();
		bool __IsInViewFrustum();

		// HORSE
		void __AttachHorseSaddle();
		void __DetachHorseSaddle();
		
		struct SHORSE
		{
			bool m_isMounting;
			CActorInstance* m_pkActor;
			
			SHORSE();			
			~SHORSE();
			
			void Destroy();
			void Create(const TPixelPosition& c_rkPPos, UINT eRace, UINT eHitEffect);
			
			void SetAttackSpeed(UINT uAtkSpd);
			void SetMoveSpeed(UINT uMovSpd);
			void Deform();
			void Render();
			CActorInstance& GetActorRef();
			CActorInstance* GetActorPtr();

			bool IsMounting();
			bool CanAttack();
			bool CanUseSkill();

			UINT GetLevel();
			bool IsNewMount();

			void __Initialize();
		} m_kHorse;


	protected:
		// Blend Mode
		void					__SetBlendRenderingMode();
		void					__SetAlphaValue(float fAlpha);
		float					__GetAlphaValue();

		void					__ComboProcess();
		void					MovementProcess();
		void					TodoProcess();
		void					StateProcess();
		void					AttackProcess();

		void					StartWalking();
		float					GetLocalTime();

		void					RefreshState(DWORD dwMotIndex, bool isLoop);
		void					RefreshActorInstance();

	protected:
		void					OnSyncing();
		void					OnWaiting();
		void					OnMoving();

		void					NEW_SetCurPixelPosition(const TPixelPosition& c_rkPPosDst);
		void					NEW_SetSrcPixelPosition(const TPixelPosition& c_rkPPosDst);
		void					NEW_SetDstPixelPosition(const TPixelPosition& c_rkPPosDst);
		void					NEW_SetDstPixelPositionZ(FLOAT z);

		const TPixelPosition&	NEW_GetCurPixelPositionRef();
		const TPixelPosition&	NEW_GetSrcPixelPositionRef();

	public:
		const TPixelPosition&	NEW_GetDstPixelPositionRef();
		short 					GetLevel() { return m_dwLevel; }
		
	protected:
		BOOL m_isTextTail;		

		// Instance Data
		std::string				m_stName;

		DWORD					m_awPart[CRaceData::PART_MAX_NUM];
#ifdef ENABLE_ALPHA_EQUIP
		int						m_iWeaponAlphaEquip;
#endif

		DWORD					m_dwLevel;
		DWORD					m_dwEmpireID;
		DWORD					m_dwGuildID;
#ifdef PROMETA
	public:
		std::string				GetName() { return m_stName; }
#endif

	protected:		
		CAffectFlagContainer	m_kAffectFlagContainer;
		DWORD					m_adwCRCAffectEffect[AFFECT_NUM];
		
		UINT	__GetRefinedEffect(CItemData* pItem);		
		void	__ClearWeaponRefineEffect();
		void	__ClearArmorRefineEffect();
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void	__ClearAcceRefineEffect();
#endif

		void __SetAcceRefineEffectActive(bool bActive);

	protected:
		void __AttachSelectEffect();
		void __DetachSelectEffect();

		void __AttachTargetEffect();
		void __DetachTargetEffect();

		void __AttachEmpireEffect(DWORD eEmpire);

	protected:
		struct SEffectContainer
		{
			typedef std::map<DWORD, DWORD> Dict;
			Dict m_kDct_dwEftID;
		} m_kEffectContainer;

		void __EffectContainer_Initialize();
		void __EffectContainer_Destroy();
		void __AttachEfektBossa();

		DWORD __EffectContainer_AttachEffect(DWORD eEffect);
		void __EffectContainer_DetachEffect(DWORD eEffect);

		SEffectContainer::Dict& __EffectContainer_GetDict();

	protected:
		struct SStoneSmoke 
		{
			DWORD m_dwEftID;
		} m_kStoneSmoke;

		void __StoneSmoke_Inialize();
		void __StoneSmoke_Destroy();
		void __StoneSmoke_Create(DWORD eSmoke);


	protected:
		// Emoticon
		//DWORD					m_adwCRCEmoticonEffect[EMOTICON_NUM];

		BYTE					m_eType;
		BYTE					m_eRaceType;
		DWORD					m_eShape;
		DWORD					m_dwRace;
		DWORD					m_dwVirtualNumber;
		int						m_iAlignment;
		BYTE					m_byPKMode;
		bool					m_isKiller;
		bool					m_isPartyMember;

#ifdef COMBAT_ZONE
		BYTE					combat_zone_rank;
		DWORD					combat_zone_points;
#endif

		// Movement
		int						m_iRotatingDirection;

		DWORD					m_dwAdvActorVID;
		DWORD					m_dwLastDmgActorVID;

		long					m_nAverageNetworkGap;
		DWORD					m_dwNextUpdateHeightTime;

		bool					m_isGoing;

		TPixelPosition			m_kPPosDust;

		DWORD					m_dwLastComboIndex;

		DWORD					m_swordRefineEffectRight;
		DWORD					m_swordRefineEffectLeft;
#ifdef ENABLE_ALPHA_EQUIP
		DWORD					m_swordAlphaEffectRight;
		DWORD					m_swordAlphaEffectLeft;
#endif
		DWORD					m_armorRefineEffect;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		DWORD					m_acceRefineEffect;
#endif

#ifdef __PERFORMANCE_UPGRADE__
		BYTE					m_BRenderPulse;
#endif

		struct SMoveAfterFunc
		{
			UINT eFunc;
			UINT uArg;

			// For Emotion Function
			UINT uArgExpanded;
			TPixelPosition kPosDst;
		};

		SMoveAfterFunc m_kMovAfterFunc;

		float m_fDstRot;
		float m_fAtkPosTime;
		float m_fRotSpd;
		float m_fMaxRotSpd;

		BOOL m_bEnableTCPState;

		// Graphic Instance
		CActorInstance m_GraphicThingInstance;


	protected:
		struct SCommand
		{
			DWORD	m_dwChkTime;
			DWORD	m_dwCmdTime;
			float	m_fDstRot;
			UINT 	m_eFunc;
			UINT 	m_uArg;
			UINT	m_uTargetVID;
			TPixelPosition m_kPPosDst;
		};

		typedef std::list<SCommand> CommandQueue;

		DWORD		m_dwBaseChkTime;
		DWORD		m_dwBaseCmdTime;

		DWORD		m_dwSkipTime;

		CommandQueue m_kQue_kCmdNew;

		BOOL		m_bDamageEffectType;

		struct SEffectDamage
		{
			DWORD damage;
			BYTE flag;
			BOOL bSelf;
			BOOL bTarget;
		};

		typedef std::list<SEffectDamage> CommandDamageQueue;
		CommandDamageQueue m_DamageQueue;

		void ProcessDamage();

	public:
		void AddDamageEffect(DWORD damage,BYTE flag,BOOL bSelf,BOOL bTarget);

	protected:
		struct SWarrior
		{
			DWORD m_dwGeomgyeongEffect;
		};

		SWarrior m_kWarrior;

		void __Warrior_Initialize();

	public:
		static void ClearPVPKeySystem();

		static void InsertPVPKey(DWORD dwSrcVID, DWORD dwDstVID);
		static void InsertPVPReadyKey(DWORD dwSrcVID, DWORD dwDstVID);
		static void RemovePVPKey(DWORD dwSrcVID, DWORD dwDstVID);

		static void InsertGVGKey(DWORD dwSrcGuildVID, DWORD dwDstGuildVID);
		static void RemoveGVGKey(DWORD dwSrcGuildVID, DWORD dwDstGuildVID);

		static void InsertDUELKey(DWORD dwSrcVID, DWORD dwDstVID);

		UINT GetNameColorIndex();

		const D3DXCOLOR& GetNameColor();
		const D3DXCOLOR& GetTitleColor();

	protected:
		static DWORD __GetPVPKey(DWORD dwSrcVID, DWORD dwDstVID);
		static bool __FindPVPKey(DWORD dwSrcVID, DWORD dwDstVID);
		static bool __FindPVPReadyKey(DWORD dwSrcVID, DWORD dwDstVID);
		static bool __FindGVGKey(DWORD dwSrcGuildID, DWORD dwDstGuildID);
		static bool __FindDUELKey(DWORD dwSrcGuildID, DWORD dwDstGuildID);

	protected:
		CActorInstance::IEventHandler* GetEventHandlerPtr();
		CActorInstance::IEventHandler& GetEventHandlerRef();

	protected:
		static float __GetBackgroundHeight(float x, float y);
		static DWORD __GetShadowMapColor(float x, float y);

#ifdef __PERFORMANCE_CHECKER__
	public:
		static void ResetPerformanceCounter();
		static void GetInfo(std::string* pstInfo);
#endif
		
	public:
		static CInstanceBase* New();
		static void Delete(CInstanceBase* pkInst);

		static CDynamicPool<CInstanceBase>	ms_kPool;

	protected:
		static DWORD ms_dwUpdateCounter;
		static DWORD ms_dwRenderCounter;
		static DWORD ms_dwDeformCounter;

	public:		
		DWORD					GetDuelMode();
		void					SetDuelMode(DWORD type);
	protected:
		DWORD					m_dwDuelMode;
		DWORD					m_dwEmoticonTime;

#ifdef ENABLE_MOB_SCALING
	public:
		float	GetMobScalingSize();

	private:
		float	m_fScalingMobSize;
#endif
	public:
		float					GetBaseHeight();
		
	public:
		void SetLODLimits(DWORD index, float fLimit);
	protected:
		bool m_IsAlwaysRender;
	public:
		bool IsAlwaysRender() const;
		void SetAlwaysRender(bool val);
#ifdef CHANGE_SKILL_COLOR
	private:
		DWORD	m_dwSkillColor[CRaceMotionData::SKILL_NUM][ESkillColorLength::MAX_EFFECT_COUNT];
#endif

#ifdef __PRESTIGE__
	public:
		void					SetPrestigeLevel(BYTE bLevel);
		BYTE					GetPrestigeLevel();
	private:
		BYTE					m_bPrestigeLevel;
#endif
		
};

inline int RaceToJob(int race)
{
#ifdef ENABLE_WOLFMAN
	if (race >= 8)
	{
		return race != 8 ? -1 : 4;
	}
#endif

	const int JOB_NUM = 4;
	return race % JOB_NUM;
}

inline int RaceToSex(int race)
{
	switch (race)
	{
		case 0:
		case 2:
		case 5:
		case 7:
#ifdef ENABLE_WOLFMAN
		case 8:
#endif
			return 1;
		case 1:
		case 3:
		case 4:
		case 6:
			return 0;

	}
	return 0;
}