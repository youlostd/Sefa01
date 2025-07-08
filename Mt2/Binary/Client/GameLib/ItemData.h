#pragma once

// NOTE : Item의 통합 관리 클래스다.
//        Icon, Model (droped on ground), Game Data

#include "../eterLib/GrpSubImage.h"
#include "../eterGrnLib/Thing.h"
#include "../UserInterface/Locale_m2.h"
#include "../UserInterface/protobuf_data.h"

enum EShiningThings
{
	SHINING_BODY_MAX = 2,
	SHINING_WEAPON_MAX = 2,
	SHINING_MAX_NUM = SHINING_BODY_MAX + SHINING_WEAPON_MAX,
	SHINING_RESERVED = 8,
};

#ifdef ENABLE_SKIN_SYSTEM
enum ESkinSystem
{
	SKINSYSTEM_PET_MAX = 1,
	SKINSYSTEM_MOUNT_MAX = 1,
	SKINSYSTEM_BODY_MAX = 2,
	SKINSYSTEM_WEAPON_MAX = 2,
	SKINSYSTEM_HAIR_MAX = 1,
	SKINSYSTEM_MAX_NUM = SKINSYSTEM_PET_MAX + SKINSYSTEM_MOUNT_MAX + SKINSYSTEM_BODY_MAX + SKINSYSTEM_WEAPON_MAX + SKINSYSTEM_HAIR_MAX,
	SKINSYSTEM_RESERVED = 12,
};
#endif

class CItemData
{
	public:
		enum
		{
			ITEM_NAME_MAX_LEN = 100,
			ITEM_LIMIT_MAX_NUM = 2,
			ITEM_VALUES_MAX_NUM = 6,
			ITEM_SMALL_DESCR_MAX_LEN = 256,
			ITEM_APPLY_MAX_NUM = 4,
			ITEM_SOCKET_MAX_NUM = 3,
#ifdef PROMETA
			ACCE_SOCKET_MAX_NUM = 1,
#endif
			ITEM_BROKEN_METIN_VNUM = 28960,
		};

		enum EItemType
		{
			ITEM_TYPE_NONE,					//0
			ITEM_TYPE_WEAPON,				//1//무기
			ITEM_TYPE_ARMOR,				//2//갑옷
			ITEM_TYPE_USE,					//3//아이템 사용
			ITEM_TYPE_AUTOUSE,				//4
			ITEM_TYPE_MATERIAL,				//5
			ITEM_TYPE_SPECIAL,				//6 //스페셜 아이템
			ITEM_TYPE_TOOL,					//7
			ITEM_TYPE_LOTTERY,				//8//복권
			ITEM_TYPE_ELK,					//9//돈
			ITEM_TYPE_METIN,				//10
			ITEM_TYPE_CONTAINER,			//11
			ITEM_TYPE_FISH,					//12//낚시
			ITEM_TYPE_ROD,					//13
			ITEM_TYPE_RESOURCE,				//14
			ITEM_TYPE_CAMPFIRE,				//15
			ITEM_TYPE_UNIQUE,				//16
			ITEM_TYPE_SKILLBOOK,			//17
			ITEM_TYPE_QUEST,				//18
			ITEM_TYPE_POLYMORPH,			//19
			ITEM_TYPE_TREASURE_BOX,			//20//보물상자
			ITEM_TYPE_TREASURE_KEY,			//21//보물상자 열쇠
			ITEM_TYPE_SKILLFORGET,			//22
			ITEM_TYPE_GIFTBOX,				//23
			ITEM_TYPE_PICK,					//24
			ITEM_TYPE_HAIR,					//25//머리
			ITEM_TYPE_TOTEM,				//26//토템
			ITEM_TYPE_BLEND,				//27//생성될때 랜덤하게 속성이 붙는 약물
			ITEM_TYPE_COSTUME,				//28//코스츔 아이템 (2011년 8월 추가된 코스츔 시스템용 아이템)
			ITEM_TYPE_SECONDARY_COIN,			//29 명도전.
			ITEM_TYPE_RING,						//30 반지 (유니크 슬롯이 아닌 순수 반지 슬롯)
//#ifdef ENABLE_PET_SYSTEM
			ITEM_TYPE_PET,
//#endif
			ITEM_TYPE_MOUNT,
//#ifdef ENABLE_BELT_SYSTEM
			ITEM_TYPE_BELT,
//#endif
//#ifdef ENABLE_DRAGONSOUL
			ITEM_TYPE_DS,
			ITEM_TYPE_SPECIAL_DS,
			ITEM_TYPE_EXTRACT,
//#endif
			ITEM_TYPE_ANIMAL_BOTTLE,
			ITEM_TYPE_SKILLBOOK_NEW,
			ITEM_TYPE_SHINING,
			ITEM_TYPE_SOUL,
#ifdef ENABLE_PET_ADVANCED
			ITEM_TYPE_PET_ADVANCED,
#endif
//#ifdef CRYSTAL_SYSTEM
			ITEM_TYPE_CRYSTAL,
//#endif
			ITEM_TYPE_MAX_NUM,				
		};

		enum ESoulSubTypes
		{
			SOUL_NONE,
			SOUL_DREAM,
			SOUL_HEAVEN,
		};

		enum EShiningSubTypes
		{
			SHINING_BODY,
			SHINING_WEAPON,
		};

		enum EWeaponSubTypes
		{
			WEAPON_SWORD,
			WEAPON_DAGGER,	//이도류
			WEAPON_BOW,
			WEAPON_TWO_HANDED,
			WEAPON_BELL,
			WEAPON_FAN,
			WEAPON_ARROW,
			WEAPON_MOUNT_SPEAR,
#ifdef ENABLE_WOLFMAN
			WEAPON_CLAW,
#endif
			WEAPON_QUIVER,
			WEAPON_NUM_TYPES,

			WEAPON_NONE = WEAPON_NUM_TYPES+1,
		};

		enum EMaterialSubTypes
		{
			MATERIAL_LEATHER,
			MATERIAL_BLOOD,
			MATERIAL_ROOT,
			MATERIAL_NEEDLE,
			MATERIAL_JEWEL,
//#ifdef ENABLE_DRAGONSOUL
			MATERIAL_DS_REFINE_NORMAL,
			MATERIAL_DS_REFINE_BLESSED,
			MATERIAL_DS_REFINE_HOLLY,
//#endif
		};

		enum EArmorSubTypes
		{
			ARMOR_BODY,
			ARMOR_HEAD,
			ARMOR_SHIELD,
			ARMOR_WRIST,
			ARMOR_FOOTS,
		    ARMOR_NECK,
			ARMOR_EAR,
			ARMOR_NUM_TYPES
		};

#ifdef CRYSTAL_SYSTEM
		enum class ECrystalItem
		{
			CRYSTAL,
			FRAGMENT,
			UPGRADE_SCROLL,
			TIME_ELIXIR,
		};
#endif

#ifdef ENABLE_PET_ADVANCED
		enum class EPetAdvanced
		{
			EGG,
			SUMMON,
			SKILL_BOOK,
			HEROIC_SKILL_BOOK,
			SKILL_REVERT,
			SKILLPOWER_REROLL,
		};
#endif

		enum EFishSubTypes
		{
			FISH_ALIVE,
			FISH_DEAD,
		};

		enum ECostumeSubTypes
		{
			COSTUME_BODY,				//0	갑옷(main look)
			COSTUME_HAIR,				//1	헤어(탈착가능)
//#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			COSTUME_ACCE,				//2
//#endif
			COSTUME_WEAPON,
			COSTUME_ACCE_COSTUME,
			COSTUME_PET,
			COSTUME_MOUNT,
			COSTUME_NUM_TYPES,
		};

		enum ECostumeSpecificAddonTypes
		{
			ADDON_COSTUME_NONE,
			ADDON_COSTUME_WEAPON,
			ADDON_COSTUME_ARMOR,
			ADDON_COSTUME_HAIR,
		};

		enum EUseSubTypes
		{
			USE_POTION,					// 0
			USE_TALISMAN,
			USE_TUNING,
			USE_MOVE,
			USE_TREASURE_BOX,
			USE_MONEYBAG,
			USE_BAIT,
			USE_ABILITY_UP,
			USE_AFFECT,
			USE_CREATE_STONE,
			USE_SPECIAL,				// 10
			USE_POTION_NODELAY,
			USE_CLEAR,
			USE_INVISIBILITY,
			USE_DETACHMENT,
			USE_BUCKET,
			USE_POTION_CONTINUE,
			USE_CLEAN_SOCKET,
			USE_CHANGE_ATTRIBUTE,
			USE_ADD_ATTRIBUTE,
			USE_ADD_ACCESSORY_SOCKET,	// 20
			USE_PUT_INTO_ACCESSORY_SOCKET,
			USE_ADD_ATTRIBUTE2,
			USE_RECIPE,
			USE_CHANGE_ATTRIBUTE2,
			USE_BIND,
			USE_UNBIND,
			USE_TIME_CHARGE_PER,
			USE_TIME_CHARGE_FIX,				// 28
			USE_PUT_INTO_RING_SOCKET,			// 29 반지 소켓에 사용할 수 있는 아이템 (유니크 반지 말고, 새로 추가된 반지 슬롯)
			USE_ADD_SPECIFIC_ATTRIBUTE,
			USE_DETACH_STONE,
			USE_DETACH_ATTR,
//#ifdef ENABLE_DRAGONSOUL
			USE_DS_CHANGE_ATTR,
//#endif
			USE_PUT_INTO_ACCESSORY_SOCKET_PERMA,
			USE_CHANGE_SASH_COSTUME_ATTR,
			USE_DEL_LAST_PERM_ORE,
		};

#ifdef ENABLE_DRAGONSOUL
		enum EDragonSoulSubType
		{
			DS_SLOT1,
			DS_SLOT2,
			DS_SLOT3,
			DS_SLOT4,
			DS_SLOT5,
			DS_SLOT6,
			DS_SLOT_NUM_TYPES = 6,
		};
#endif

		enum EMetinSubTypes
		{
			METIN_NORMAL,
			METIN_GOLD,
//#ifdef PROMETA
			METIN_ACCE,
//#endif
		};

		enum EMountSubTypes {
			MOUNT_SUB_SUMMON,
			MOUNT_SUB_FOOD,
			MOUNT_SUB_REVIVE,
		};

		enum ELimitTypes
		{
			LIMIT_NONE,

			LIMIT_LEVEL,
			LIMIT_STR,
			LIMIT_DEX,
			LIMIT_INT,
			LIMIT_CON,

			/// 착용 여부와 상관 없이 실시간으로 시간 차감 (socket0에 소멸 시간이 박힘: unix_timestamp 타입)
			LIMIT_REAL_TIME,						

			/// 아이템을 맨 처음 사용(혹은 착용) 한 순간부터 리얼타임 타이머 시작 
			/// 최초 사용 전에는 socket0에 사용가능시간(초단위, 0이면 프로토의 limit value값 사용) 값이 쓰여있다가 
			/// 아이템 사용시 socket1에 사용 횟수가 박히고 socket0에 unix_timestamp 타입의 소멸시간이 박힘.
			LIMIT_REAL_TIME_START_FIRST_USE,

			/// 아이템을 착용 중일 때만 사용 시간이 차감되는 아이템
			/// socket0에 남은 시간이 초단위로 박힘. (아이템 최초 사용시 해당 값이 0이면 프로토의 limit value값을 socket0에 복사)
			LIMIT_TIMER_BASED_ON_WEAR,

//#ifdef ENABLE_LEVEL_LIMIT_MAX
			LIMIT_LEVEL_MAX,
//#endif

//#ifdef NEW_OFFICIAL
			PC_BANG,
//#endif
			LIMIT_MAX_NUM
		};

		enum EItemAntiFlag
		{
			ITEM_ANTIFLAG_FEMALE        = (1 << 0),		// 여성 사용 불가
			ITEM_ANTIFLAG_MALE          = (1 << 1),		// 남성 사용 불가
			ITEM_ANTIFLAG_WARRIOR       = (1 << 2),		// 무사 사용 불가
			ITEM_ANTIFLAG_ASSASSIN      = (1 << 3),		// 자객 사용 불가
			ITEM_ANTIFLAG_SURA          = (1 << 4),		// 수라 사용 불가 
			ITEM_ANTIFLAG_SHAMAN        = (1 << 5),		// 무당 사용 불가
			ITEM_ANTIFLAG_GET           = (1 << 6),		// 집을 수 없음
			ITEM_ANTIFLAG_DROP          = (1 << 7),		// 버릴 수 없음
			ITEM_ANTIFLAG_SELL          = (1 << 8),		// 팔 수 없음
			ITEM_ANTIFLAG_EMPIRE_A      = (1 << 9),		// A 제국 사용 불가
			ITEM_ANTIFLAG_EMPIRE_B      = (1 << 10),	// B 제국 사용 불가
			ITEM_ANTIFLAG_EMPIRE_R      = (1 << 11),	// C 제국 사용 불가
			ITEM_ANTIFLAG_SAVE          = (1 << 12),	// 저장되지 않음
			ITEM_ANTIFLAG_GIVE          = (1 << 13),	// 거래 불가
			ITEM_ANTIFLAG_PKDROP        = (1 << 14),	// PK시 떨어지지 않음
			ITEM_ANTIFLAG_STACK         = (1 << 15),	// 합칠 수 없음
			ITEM_ANTIFLAG_MYSHOP        = (1 << 16),	// 개인 상점에 올릴 수 없음
			ITEM_ANTIFLAG_SAFEBOX		= (1 << 17),
#ifdef ENABLE_WOLFMAN
			ITEM_ANTIFLAG_WOLFMAN		= (1 << 18),
#endif
			ITEM_ANTIFLAG_DESTROY		= (1 << 19),
			ITEM_ANTIFLAG_APPLY			= (1 << 20),
		};

		enum EItemFlag
		{
			ITEM_FLAG_REFINEABLE        = (1 << 0),		// 개량 가능
			ITEM_FLAG_SAVE              = (1 << 1),
			ITEM_FLAG_STACKABLE         = (1 << 2),     // 여러개 합칠 수 있음
			ITEM_FLAG_COUNT_PER_1GOLD   = (1 << 3),		// 가격이 개수 / 가격으로 변함
			ITEM_FLAG_SLOW_QUERY        = (1 << 4),		// 게임 종료시에만 SQL에 쿼리함
			ITEM_FLAG_RARE              = (1 << 5),
			ITEM_FLAG_UNIQUE            = (1 << 6),
			ITEM_FLAG_MAKECOUNT			= (1 << 7),
			ITEM_FLAG_IRREMOVABLE		= (1 << 8),
			ITEM_FLAG_CONFIRM_WHEN_USE	= (1 << 9),
			ITEM_FLAG_QUEST_USE         = (1 << 10),    // 퀘스트 스크립트 돌리는지?
			ITEM_FLAG_QUEST_USE_MULTIPLE= (1 << 11),    // 퀘스트 스크립트 돌리는지?
			ITEM_FLAG_UNUSED03          = (1 << 12),    // UNUSED03
			ITEM_FLAG_LOG               = (1 << 13),    // 사용시 로그를 남기는 아이템인가?
			ITEM_FLAG_APPLICABLE		= (1 << 14),
			ITEM_FLAG_CONFIRM_GM_ITEM	= (1 << 15),
		};

		enum EWearPositions
		{
			WEAR_BODY,		// 0
			WEAR_HEAD,		// 1
			WEAR_FOOTS,		// 2
			WEAR_WRIST,		// 3
			WEAR_WEAPON,	// 4
			WEAR_NECK,		// 5
			WEAR_EAR,		// 6
			WEAR_UNIQUE1,	// 7
			WEAR_UNIQUE2,	// 8
			WEAR_ARROW,		// 9
			WEAR_SHIELD,	// 10
			WEAR_ABILITY1,  // 11
			WEAR_ABILITY2,  // 12
			WEAR_ABILITY3,  // 13
			WEAR_ABILITY4,  // 14
			WEAR_ABILITY5,  // 15
			WEAR_ABILITY6,  // 16
			WEAR_ABILITY7,  // 17
			WEAR_ABILITY8,  // 18
			WEAR_COSTUME_BODY,	// 19
			WEAR_COSTUME_HAIR,	// 20
			WEAR_COSTUME_WEAPON,	// 21

			WEAR_RING1,			// 22	: 신규 반지슬롯1 (왼쪽)
			WEAR_RING2,			// 23	: 신규 반지슬롯2 (오른쪽)

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			WEAR_ACCE,			// 24
#endif

#ifdef ENABLE_BELT_SYSTEM
			WEAR_BELT,			// 25
#endif

			WEAR_UNIQUE3,
			WEAR_UNIQUE4,

			WEAR_TOTEM,

			WEAR_COSTUME_ACCE,

			WEAR_MAX_NUM,
		};

		enum EItemWearableFlag
		{
			WEARABLE_BODY	= (1 << 0),
			WEARABLE_HEAD	= (1 << 1),
			WEARABLE_FOOTS	= (1 << 2),
			WEARABLE_WRIST	= (1 << 3),
			WEARABLE_WEAPON	= (1 << 4),
			WEARABLE_NECK	= (1 << 5),
			WEARABLE_EAR	= (1 << 6),
			WEARABLE_UNIQUE	= (1 << 7),
			WEARABLE_SHIELD	= (1 << 8),
			WEARABLE_ARROW	= (1 << 9),
			WEARABLE_HAIR	= (1 << 10),
			WEARABLE_ABILITY		= (1 << 11),
			WEARABLE_COSTUME_BODY	= (1 << 12),
			WEARABLE_COSTUME_UNIQUE	= (1 << 13),
			WEARABLE_TOTEM	= (1 << 14),
		};

		enum EApplyTypes
		{
			APPLY_NONE,                 // 0
			APPLY_MAX_HP,               // 1
			APPLY_MAX_SP,               // 2
			APPLY_CON,                  // 3
			APPLY_INT,                  // 4
			APPLY_STR,                  // 5
			APPLY_DEX,                  // 6
			APPLY_ATT_SPEED,            // 7
			APPLY_MOV_SPEED,            // 8
			APPLY_CAST_SPEED,           // 9
			APPLY_HP_REGEN,             // 10
			APPLY_SP_REGEN,             // 11
			APPLY_POISON_PCT,           // 12
			APPLY_STUN_PCT,             // 13
			APPLY_SLOW_PCT,             // 14
			APPLY_CRITICAL_PCT,         // 15
			APPLY_PENETRATE_PCT,        // 16
			APPLY_ATTBONUS_HUMAN,       // 17
			APPLY_ATTBONUS_ANIMAL,      // 18
			APPLY_ATTBONUS_ORC,         // 19
			APPLY_ATTBONUS_MILGYO,      // 20
			APPLY_ATTBONUS_UNDEAD,      // 21
			APPLY_ATTBONUS_DEVIL,       // 22
			APPLY_STEAL_HP,             // 23
			APPLY_STEAL_SP,             // 24
			APPLY_MANA_BURN_PCT,        // 25
			APPLY_DAMAGE_SP_RECOVER,    // 26
			APPLY_BLOCK,                // 27
			APPLY_DODGE,                // 28
			APPLY_RESIST_SWORD,         // 29
			APPLY_RESIST_TWOHAND,       // 30
			APPLY_RESIST_DAGGER,        // 31
			APPLY_RESIST_BELL,          // 32
			APPLY_RESIST_FAN,           // 33
			APPLY_RESIST_BOW,           // 34
			APPLY_RESIST_FIRE,          // 35
			APPLY_RESIST_ELEC,          // 36
			APPLY_RESIST_MAGIC,         // 37
			APPLY_RESIST_WIND,          // 38
			APPLY_REFLECT_MELEE,        // 39
			APPLY_REFLECT_CURSE,        // 40
			APPLY_POISON_REDUCE,        // 41
			APPLY_KILL_SP_RECOVER,      // 42
			APPLY_EXP_DOUBLE_BONUS,     // 43
			APPLY_GOLD_DOUBLE_BONUS,    // 44
			APPLY_ITEM_DROP_BONUS,      // 45
			APPLY_POTION_BONUS,         // 46
			APPLY_KILL_HP_RECOVER,      // 47
			APPLY_IMMUNE_STUN,          // 48
			APPLY_IMMUNE_SLOW,          // 49
			APPLY_IMMUNE_FALL,          // 50
			APPLY_SKILL,                // 51
			APPLY_BOW_DISTANCE,         // 52
			APPLY_ATT_GRADE_BONUS,            // 53
			APPLY_DEF_GRADE_BONUS,            // 54
			APPLY_MAGIC_ATT_GRADE,      // 55
			APPLY_MAGIC_DEF_GRADE,      // 56
			APPLY_CURSE_PCT,            // 57
			APPLY_MAX_STAMINA,			// 58
			APPLY_ATT_BONUS_TO_WARRIOR, // 59
			APPLY_ATT_BONUS_TO_ASSASSIN,// 60
			APPLY_ATT_BONUS_TO_SURA,    // 61
			APPLY_ATT_BONUS_TO_SHAMAN,  // 62
			APPLY_ATT_BONUS_TO_MONSTER, // 63
			APPLY_MALL_ATTBONUS,        // 64 공격력 +x%
			APPLY_MALL_DEFBONUS,        // 65 방어력 +x%
			APPLY_MALL_EXPBONUS,        // 66 경험치 +x%
			APPLY_MALL_ITEMBONUS,       // 67 아이템 드롭율 x/10배
			APPLY_MALL_GOLDBONUS,       // 68 돈 드롭율 x/10배
			APPLY_MAX_HP_PCT,           // 69 최대 생명력 +x%
			APPLY_MAX_SP_PCT,           // 70 최대 정신력 +x%
			APPLY_SKILL_DAMAGE_BONUS,   // 71 스킬 데미지 * (100+x)%
			APPLY_NORMAL_HIT_DAMAGE_BONUS,      // 72 평타 데미지 * (100+x)%
			APPLY_SKILL_DEFEND_BONUS,   // 73 스킬 데미지 방어 * (100-x)%
			APPLY_NORMAL_HIT_DEFEND_BONUS,      // 74 평타 데미지 방어 * (100-x)%
			APPLY_EXTRACT_HP_PCT,		//75
			APPLY_RESIST_WARRIOR,			//78
			APPLY_RESIST_ASSASSIN ,			//79
			APPLY_RESIST_SURA,				//80
			APPLY_RESIST_SHAMAN,			//81
			APPLY_ENERGY,					//82
			APPLY_DEF_GRADE,				// 83 방어력. DEF_GRADE_BONUS는 클라에서 두배로 보여지는 의도된 버그(...)가 있다.
			APPLY_COSTUME_ATTR_BONUS,		// 84 코스튬 아이템에 붙은 속성치 보너스
			APPLY_MAGIC_ATTBONUS_PER,		// 85 마법 공격력 +x%
			APPLY_MELEE_MAGIC_ATTBONUS_PER,			// 86 마법 + 밀리 공격력 +x%
			
			APPLY_RESIST_ICE,		// 87 냉기 저항
			APPLY_RESIST_EARTH,		// 88 대지 저항
			APPLY_RESIST_DARK,		// 89 어둠 저항

			APPLY_ANTI_CRITICAL_PCT,	//90 크리티컬 저항
			APPLY_ANTI_PENETRATE_PCT,	//91 관통타격 저항

			APPLY_EXP_REAL_BONUS,

//#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			APPLY_ACCEDRAIN_RATE,
//#endif

#ifdef ENABLE_ANIMAL_SYSTEM
#ifdef ENABLE_PET_SYSTEM
			APPLY_PET_EXP_BONUS,
#endif
			APPLY_MOUNT_EXP_BONUS,
#endif
			APPLY_MOUNT_BUFF_BONUS,				// 123

			APPLY_RESIST_MONSTER,
			APPLY_ATTBONUS_METIN,
			APPLY_ATTBONUS_BOSS,
			APPLY_RESIST_HUMAN,

			APPLY_RESIST_SWORD_PEN,
			APPLY_RESIST_TWOHAND_PEN,
			APPLY_RESIST_DAGGER_PEN,
			APPLY_RESIST_BELL_PEN,
			APPLY_RESIST_FAN_PEN,
			APPLY_RESIST_BOW_PEN,
			APPLY_RESIST_ATTBONUS_HUMAN,
//#ifdef __ELEMENT_SYSTEM__
			APPLY_ATTBONUS_ELEC,
			APPLY_ATTBONUS_FIRE,
			APPLY_ATTBONUS_ICE,
			APPLY_ATTBONUS_WIND,
			APPLY_ATTBONUS_EARTH,
			APPLY_ATTBONUS_DARK,
//#endif
			APPLY_DEFENSE_BONUS,
			APPLY_ANTI_RESIST_MAGIC,
			APPLY_BLOCK_IGNORE_BONUS,

//#ifdef ENABLE_RUNE_SYSTEM
			APPLY_RUNE_SHIELD_PER_HIT,
			APPLY_RUNE_HEAL_ON_KILL,
			APPLY_RUNE_BONUS_DAMAGE_AFTER_HIT,
			APPLY_RUNE_3RD_ATTACK_BONUS,
			APPLY_RUNE_FIRST_NORMAL_HIT_BONUS,
			APPLY_RUNE_MSHIELD_PER_SKILL,
			APPLY_RUNE_HARVEST,
			APPLY_RUNE_DAMAGE_AFTER_3,
			APPLY_RUNE_OUT_OF_COMBAT_SPEED,
			APPLY_RUNE_RESET_SKILL,
			APPLY_RUNE_COMBAT_CASTING_SPEED,
			APPLY_RUNE_MAGIC_DAMAGE_AFTER_HIT,
			APPLY_RUNE_MOVSPEED_AFTER_3,
			APPLY_RUNE_SLOW_ON_ATTACK,
//#endif
			APPLY_HEAL_EFFECT_BONUS,
			APPLY_CRITICAL_DAMAGE_BONUS,
			APPLY_DOUBLE_ITEM_DROP_BONUS,
			APPLY_DAMAGE_BY_SP_BONUS,
			APPLY_SINGLETARGET_SKILL_DAMAGE_BONUS,
			APPLY_MULTITARGET_SKILL_DAMAGE_BONUS,
			APPLY_MIXED_DEFEND_BONUS,
			APPLY_EQUIP_SKILL_BONUS,
			APPLY_AURA_HEAL_EFFECT_BONUS,
			APPLY_AURA_EQUIP_SKILL_BONUS,

//#ifdef ENABLE_RUNE_SYSTEM
			APPLY_RUNE_LEADERSHIP_BONUS,
			APPLY_RUNE_MOUNT_PARALYZE,

//	#ifdef RUNE_CRITICAL_POINT
			APPLY_RUNE_CRITICAL_PVM,
//	#endif
//#endif
			APPLY_ATTBONUS_ALL_ELEMENTS,
//#ifdef ENABLE_ZODIAC
			APPLY_ATTBONUS_ZODIAC,
//#endif

			APPLY_BONUS_UPGRADE_CHANCE,
			APPLY_LOWER_DUNGEON_CD,
			APPLY_LOWER_BIOLOG_CD,

#ifdef ENABLE_WOLFMAN
			APPLY_BLEEDING_PCT,
			APPLY_BLEEDING_REDUCE,
			APPLY_ATTBONUS_WOLFMAN,
			APPLY_RESIST_WOLFMAN,
			APPLY_RESIST_CLAW,
#endif

			APPLY_RESIST_MAGIC_REDUCTION,

//#ifdef STANDARD_SKILL_DURATION
			APPLY_SKILL_DURATION,
//#endif

			APPLY_RESIST_BOSS,

   			MAX_APPLY_NUM,
		};

		enum EImmuneFlags
		{
			IMMUNE_STUN		= (1 << 0),
			IMMUNE_SLOW		= (1 << 1),
			IMMUNE_FALL		= (1 << 2),
			IMMUNE_CURSE	= (1 << 3),
			IMMUNE_POISON	= (1 << 4),
			IMMUNE_TERROR	= (1 << 5),
			IMMUNE_REFLECT	= (1 << 6),

			IMMUNE_FLAG_MAX_NUM = 7
		};

		using TItemTable = network::TItemTable;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		typedef struct SItemScaleTable {
			D3DXVECTOR3 scalePos[2][2][4];
			D3DXVECTOR3 scale[2][2][4];

		} TItemScaleTable;
#endif

	public:
		CItemData();
		virtual ~CItemData();

		void Clear();
		void SetSummary(const std::string& c_rstSumm);
		void SetDescription(const std::string& c_rstDesc);

		CGraphicThing * GetModelThing();
		CGraphicThing * GetSubModelThing();
		CGraphicThing * GetDropModelThing();
		CGraphicSubImage * GetIconImage();

		DWORD GetLODModelThingCount();
		BOOL GetLODModelThingPointer(DWORD dwIndex, CGraphicThing ** ppModelThing);

		DWORD GetAttachingDataCount();
		BOOL GetCollisionDataPointer(DWORD dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData);
		BOOL GetAttachingDataPointer(DWORD dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData);

		/////
		void OverwriteName(const char* c_pszNewName);
		void OverwriteValue(unsigned char byIndex, long lNewValue);

		const TItemTable*	GetTable() const;
		DWORD GetIndex() const;
		const char * GetName() const;
		const char * GetDescription() const;
		const char * GetSummary() const;
		BYTE GetType() const;
		BYTE GetSubType() const;
		UINT GetRefine() const;
		const char* GetUseTypeString() const;
		DWORD GetWeaponType() const;
		BYTE GetSize() const;
		BOOL IsAntiFlag(DWORD dwFlag) const;
		DWORD GetFlags() const;
		BOOL IsFlag(DWORD dwFlag) const;
		BOOL IsWearableFlag(DWORD dwFlag) const;
		BOOL HasNextGrade() const;
		DWORD GetWearFlags() const;
		DWORD GetIBuyItemPrice() const;
		DWORD GetISellItemPrice() const;
		BOOL GetLimit(BYTE byIndex, network::TItemLimit * pItemLimit) const;
		BOOL GetApply(BYTE byIndex, network::TItemApply * pItemApply) const;
		long GetValue(BYTE byIndex) const;
		long GetSocket(BYTE byIndex) const;
		long SetSocket(BYTE byIndex,DWORD value);
		int GetSocketCount() const;
		DWORD GetIconNumber() const;
		DWORD GetRefinedVnum() const { return m_ItemTable.refined_vnum(); }

		UINT	GetSpecularPoweru() const;
		float	GetSpecularPowerf() const;
	
		/////

		BOOL IsEquipment() const;

		/////

		//BOOL LoadItemData(const char * c_szFileName);
		void SetDefaultItemData(const char * c_szIconFileName, const char * c_szModelFileName  = NULL);
		void SetItemTableData(const TItemTable * pItemTable);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void SetItemTableScaleData(int dwJob, int dwSex, float fScaleX, float fScaleY, float fScaleZ, float fScalePosX, float fScalePosY, float fScalePosZ, bool bMount);
		D3DXVECTOR3 & GetItemScalePosition(int dwJob, int dwSex, bool bMount);
		D3DXVECTOR3 & GetItemScale(int dwJob, int dwSex, bool bMount);
#endif

	protected:
		void __LoadFiles();
		void __SetIconImage(const char * c_szFileName);

	protected:
		std::string m_strModelFileName;
		std::string m_strSubModelFileName;
		std::string m_strDropModelFileName;
		std::string m_strIconFileName;
		std::string m_strDescription;
		std::string m_strSummary;
		std::vector<std::string> m_strLODModelFileNameVector;

		CGraphicThing * m_pModelThing;
		CGraphicThing * m_pSubModelThing;
		CGraphicThing * m_pDropModelThing;
		CGraphicSubImage * m_pIconImage;
		std::vector<CGraphicThing *> m_pLODModelThingVector;

		NRaceData::TAttachingDataVector m_AttachingDataVector;
		DWORD		m_dwVnum;
		TItemTable m_ItemTable;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		TItemScaleTable m_ItemScaleTable;
#endif

#ifdef INGAME_WIKI
	protected:
		bool m_isValidImage;
		bool m_isBlacklisted;

	public:
		typedef struct SWikiItemInfo
		{
			bool isSet;
			bool hasData;

			bool bIsCommon;
			DWORD dwOrigin;
			google::protobuf::RepeatedPtrField<network::TWikiRefineInfo> pRefineData;
			google::protobuf::RepeatedPtrField<network::TWikiChestDropInfo> pChestInfo;
			google::protobuf::RepeatedPtrField<network::TWikiItemOriginInfo> pOriginInfo;

			SWikiItemInfo()
			{
				isSet = false;
				hasData = false;
			}
		}TWikiItemInfo;

		bool IsValidImage() { return m_isValidImage; }

		void ValidateImage(bool isValidImage) { m_isValidImage = isValidImage; }

		std::string GetIconFileName() { return m_strIconFileName; }

		TWikiItemInfo* GetWikiTable() { return &m_wikiInfo; }

		void SetBlacklisted(bool val) { m_isBlacklisted = val; }
		bool IsBlacklisted() { return m_isBlacklisted; }

	private:
		TWikiItemInfo m_wikiInfo;
#endif
		
	public:
		static void DestroySystem();

		static CItemData* New();
		static void Delete(CItemData* pkItemData);

		static CDynamicPool<CItemData>		ms_kPool;
};
