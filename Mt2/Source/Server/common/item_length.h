#ifndef __INC_METIN2_ITEM_LENGTH_H__
#define __INC_METIN2_ITEM_LENGTH_H__

#include "service.h"

enum EItemMisc
{
	ITEM_NAME_MAX_LEN			= 100,
	ITEM_VALUES_MAX_NUM			= 6,
	ITEM_SMALL_DESCR_MAX_LEN	= 256,
	ITEM_LIMIT_MAX_NUM			= 2,
	ITEM_APPLY_MAX_NUM			= 4,
	ITEM_SOCKET_MAX_NUM			= 3,
#ifdef PROMETA
	ACCE_SOCKET_MAX_NUM = 1,
#endif
#ifdef INCREASE_ITEM_STACK
#ifdef ELONIA
	ITEM_MAX_COUNT				= 200,
#else
	ITEM_MAX_COUNT				= 1000,
#endif
#else
	ITEM_MAX_COUNT				= 200,
#endif
	ITEM_ATTRIBUTE_MAX_NUM		= 7,
	ITEM_ATTRIBUTE_MAX_LEVEL	= 5,
	ITEM_AWARD_WHY_MAX_LEN		= 50,

	REFINE_MATERIAL_MAX_NUM		= 5,

	ITEM_ELK_VNUM				= 50026,
	ITEM_BROKEN_METIN_VNUM		= 28960,

#ifdef __ACCE_COSTUME__
	ITEM_MAX_ACCEDRAIN = 25,
	ITEM_MIN_ACCEDRAIN = 20,
#endif
};

const BYTE ITEM_SOCKET_REMAIN_SEC = 0;
enum EItemValueIdice
{
	ITEM_VALUE_DRAGON_SOUL_POLL_OUT_BONUS_IDX = 0,
	ITEM_VALUE_CHARGING_AMOUNT_IDX = 0,
	ITEM_VALUE_SECONDARY_COIN_UNIT_IDX = 0,
};
// �� �̰� ��ģ�� �ƴϾ�?
// ���߿� ���� Ȯ���ϸ� ��¼���� ������ -_-;;;
enum EItemUniqueSockets
{
	ITEM_SOCKET_UNIQUE_SAVE_TIME = ITEM_SOCKET_MAX_NUM - 2,
	ITEM_SOCKET_UNIQUE_REMAIN_TIME = ITEM_SOCKET_MAX_NUM - 1
};

#ifdef __COSTUME_BONUS_TRANSFER__
enum ECostumeBonusTransferSlotType
{
	CBT_SLOT_MEDIUM,
	CBT_SLOT_MATERIAL,
	CBT_SLOT_TARGET,
	CBT_SLOT_RESULT,
	CBT_SLOT_MAX
};

enum ECostumeBonusTransferMisc
{
	CBT_MEDIUM_ITEM_VNUM = 92936,
};
#endif

enum EItemTypes
{
    ITEM_NONE,              //0
    ITEM_WEAPON,            //1//����
    ITEM_ARMOR,             //2//����
    ITEM_USE,               //3//������ ���
    ITEM_AUTOUSE,           //4
    ITEM_MATERIAL,          //5
    ITEM_SPECIAL,           //6 //����� ������
    ITEM_TOOL,              //7
    ITEM_LOTTERY,           //8//����
    ITEM_ELK,               //9//��
    ITEM_METIN,             //10
    ITEM_CONTAINER,         //11
    ITEM_FISH,              //12//����
    ITEM_ROD,               //13
    ITEM_RESOURCE,          //14
    ITEM_CAMPFIRE,          //15
    ITEM_UNIQUE,            //16
    ITEM_SKILLBOOK,         //17
    ITEM_QUEST,             //18
    ITEM_POLYMORPH,         //19
    ITEM_TREASURE_BOX,      //20//��������
    ITEM_TREASURE_KEY,      //21//�������� ����
    ITEM_SKILLFORGET,       //22
    ITEM_GIFTBOX,           //23
    ITEM_PICK,              //24
    ITEM_HAIR,              //25//�Ӹ�
    ITEM_TOTEM,             //26//����
	ITEM_BLEND,				//27//�����ɶ� �����ϰ� �Ӽ��� �ٴ� �๰
	ITEM_COSTUME,			//28//�ڽ��� ������ (2011�� 8�� �߰��� �ڽ��� �ý��ۿ� ������)
	ITEM_SECONDARY_COIN,	//29 ?? ����??
	ITEM_RING,				//30 ����
//#ifdef __PET_SYSTEM__
	ITEM_PET,
//#endif
	ITEM_MOUNT,
//#ifdef __BELT_SYSTEM__
	ITEM_BELT,
//#endif
//#ifdef __DRAGONSOUL__
	ITEM_DS,
	ITEM_SPECIAL_DS,
	ITEM_EXTRACT,
//#endif
	ITEM_ANIMAL_BOTTLE,
	ITEM_SKILLBOOK_NEW,
	ITEM_SHINING,
	ITEM_SOUL,
#ifdef __PET_ADVANCED__
	ITEM_PET_ADVANCED,
#endif
//#ifdef CRYSTAL_SYSTEM
	ITEM_CRYSTAL,
//#endif
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

#ifdef __PET_ADVANCED__
enum class EPetAdvancedItem
{
	EGG,
	SUMMON,
	SKILL_BOOK,
	HEROIC_SKILL_BOOK,
	SKILL_REVERT,
	SKILLPOWER_REROLL,
};
#endif

enum ESoulSubTypes
{
	SOUL_NONE,
	SOUL_DREAM,
	SOUL_HEAVEN,
};

enum EMountSubTypes
{
	MOUNT_SUB_SUMMON,
	MOUNT_SUB_FOOD,
	MOUNT_SUB_REVIVE,
};

enum EMetinSubTypes
{
	METIN_NORMAL,
	METIN_GOLD,
//#ifdef PROMETA
	METIN_ACCE,
//#endif
};

enum EWeaponSubTypes
{
	WEAPON_SWORD,
	WEAPON_DAGGER,
	WEAPON_BOW,
	WEAPON_TWO_HANDED,
	WEAPON_BELL,
	WEAPON_FAN,
	WEAPON_ARROW,
	WEAPON_MOUNT_SPEAR,
#ifdef __WOLFMAN__
	WEAPON_CLAW,
#endif
	WEAPON_QUIVER,
	
	WEAPON_NUM_TYPES,
};

enum EShiningSubTypes
{
	SHINING_BODY,
	SHINING_WEAPON,
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

enum ECostumeSpecificAddonTypes
{
	ADDON_COSTUME_NONE,
	ADDON_COSTUME_WEAPON,
	ADDON_COSTUME_ARMOR,
	ADDON_COSTUME_HAIR,
};

enum ECostumeSubTypes
{
	COSTUME_BODY,			// [�߿�!!] ECostumeSubTypes enum value��  �������� EArmorSubTypes�� �װͰ� ���ƾ� ��.
	COSTUME_HAIR,			// �̴� �ڽ��� �����ۿ� �߰� �Ӽ��� ���̰ڴٴ� ������� ��û�� ���� ���� ������ Ȱ���ϱ� ������.
//#ifdef __ACCE_COSTUME__
	COSTUME_ACCE,
//#endif
	COSTUME_WEAPON,
	COSTUME_ACCE_COSTUME,
	COSTUME_PET,
	COSTUME_MOUNT,
	COSTUME_NUM_TYPES,
};

#ifdef __ACCE_COSTUME__
enum {
	ABSORB,
	COMBINE,
};

enum {
	ACCE_SLOT_LEFT,
	ACCE_SLOT_RIGHT,
	ACCE_SLOT_RESULT,
	ACCE_SLOT_MAX_NUM,
};
#endif

enum EFishSubTypes
{
	FISH_ALIVE,
	FISH_DEAD,
};

enum EResourceSubTypes
{
	RESOURCE_FISHBONE,
	RESOURCE_WATERSTONEPIECE,
	RESOURCE_WATERSTONE,
	RESOURCE_BLOOD_PEARL,
	RESOURCE_BLUE_PEARL,
	RESOURCE_WHITE_PEARL,
	RESOURCE_BUCKET,
	RESOURCE_CRYSTAL,
	RESOURCE_GEM,
	RESOURCE_STONE,
	RESOURCE_METIN,
	RESOURCE_ORE,
};

enum EUniqueSubTypes
{
	UNIQUE_NONE,
	UNIQUE_BOOK,
	UNIQUE_SPECIAL_RIDE,
	UNIQUE_SPECIAL_MOUNT_RIDE,
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
	USE_PUT_INTO_RING_SOCKET,			// 29 ���� ���Ͽ� ����� �� �ִ� ������ (����ũ ���� ����, ���� �߰��� ���� ����)
	USE_ADD_SPECIFIC_ATTRIBUTE,
	USE_DETACH_STONE,
	USE_DETACH_ATTR,
//#ifdef __DRAGONSOUL__
	USE_DS_CHANGE_ATTR,
//#endif
	USE_PUT_INTO_ACCESSORY_SOCKET_PERMA,
	USE_CHANGE_SASH_COSTUME_ATTR,
	USE_DEL_LAST_PERM_ORE,
};

enum ESkillBookSubTypes
{
	SKILLBOOK_NORMAL,
	SKILLBOOK_MASTER,
};

enum EAutoUseSubTypes
{
	AUTOUSE_POTION,
	AUTOUSE_ABILITY_UP,
	AUTOUSE_BOMB,
	AUTOUSE_GOLD,
	AUTOUSE_MONEYBAG,
	AUTOUSE_TREASURE_BOX
};

enum EMaterialSubTypes
{
	MATERIAL_LEATHER,
	MATERIAL_BLOOD,
	MATERIAL_ROOT,
	MATERIAL_NEEDLE,
	MATERIAL_JEWEL,
#ifdef __DRAGONSOUL__
	MATERIAL_DS_REFINE_NORMAL, 
	MATERIAL_DS_REFINE_BLESSED, 
	MATERIAL_DS_REFINE_HOLLY,
#endif
};

enum ESpecialSubTypes
{
	SPECIAL_MAP,
	SPECIAL_KEY,
	SPECIAL_DOC,
	SPECIAL_SPIRIT,
};

enum EToolSubTypes
{
	TOOL_FISHING_ROD
};

enum ELotterySubTypes
{
	LOTTERY_TICKET,
	LOTTERY_INSTANT
};

enum EItemFlag
{
	ITEM_FLAG_REFINEABLE		= (1 << 0),
	ITEM_FLAG_SAVE			= (1 << 1),
	ITEM_FLAG_STACKABLE		= (1 << 2),	// ������ ��ĥ �� ����
	ITEM_FLAG_COUNT_PER_1GOLD	= (1 << 3),
	ITEM_FLAG_SLOW_QUERY		= (1 << 4),
	ITEM_FLAG_UNUSED01		= (1 << 5),	// UNUSED
	ITEM_FLAG_UNIQUE		= (1 << 6),
	ITEM_FLAG_MAKECOUNT		= (1 << 7),
	ITEM_FLAG_IRREMOVABLE		= (1 << 8),
	ITEM_FLAG_CONFIRM_WHEN_USE	= (1 << 9),
	ITEM_FLAG_QUEST_USE		= (1 << 10),
	ITEM_FLAG_QUEST_USE_MULTIPLE	= (1 << 11),
	ITEM_FLAG_QUEST_GIVE		= (1 << 12),
	ITEM_FLAG_LOG			= (1 << 13),
	ITEM_FLAG_APPLICABLE		= (1 << 14),
	ITEM_FLAG_CONFIRM_GM_ITEM=(1 << 15),
#ifdef __ALPHA_EQUIP__
	ITEM_FLAG_ALPHA_EQUIP	= (1 << 16),
#endif
};

enum EItemAntiFlag
{
	ITEM_ANTIFLAG_FEMALE	= (1 << 0), // ���� ��� �Ұ�
	ITEM_ANTIFLAG_MALE		= (1 << 1), // ���� ��� �Ұ�
	ITEM_ANTIFLAG_WARRIOR	= (1 << 2), // ���� ��� �Ұ�
	ITEM_ANTIFLAG_ASSASSIN	= (1 << 3), // �ڰ� ��� �Ұ�
	ITEM_ANTIFLAG_SURA		= (1 << 4), // ���� ��� �Ұ� 
	ITEM_ANTIFLAG_SHAMAN	= (1 << 5), // ���� ��� �Ұ�
	ITEM_ANTIFLAG_GET		= (1 << 6), // ���� �� ����
	ITEM_ANTIFLAG_DROP		= (1 << 7), // ���� �� ����
	ITEM_ANTIFLAG_SELL		= (1 << 8), // �� �� ����
	ITEM_ANTIFLAG_EMPIRE_A	= (1 << 9), // A ���� ��� �Ұ�
	ITEM_ANTIFLAG_EMPIRE_B	= (1 << 10), // B ���� ��� �Ұ�
	ITEM_ANTIFLAG_EMPIRE_C	= (1 << 11), // C ���� ��� �Ұ�
	ITEM_ANTIFLAG_SAVE		= (1 << 12), // ������� ����
	ITEM_ANTIFLAG_GIVE		= (1 << 13), // �ŷ� �Ұ�
	ITEM_ANTIFLAG_PKDROP	= (1 << 14), // PK�� �������� ����
	ITEM_ANTIFLAG_STACK		= (1 << 15), // ��ĥ �� ����
	ITEM_ANTIFLAG_MYSHOP	= (1 << 16), // ���� ������ �ø� �� ����
	ITEM_ANTIFLAG_SAFEBOX	= (1 << 17), // â�� ���� �� ����
#ifdef __WOLFMAN__
	ITEM_ANTIFLAG_WOLFMAN	= (1 << 18), // Wolf
#endif
	ITEM_ANTIFLAG_DESTROY	= (1 << 19),
	ITEM_ANTIFLAG_APPLY	= (1 << 20),
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

enum ELimitTypes
{
	LIMIT_NONE,

	LIMIT_LEVEL,
	LIMIT_STR,
	LIMIT_DEX,
	LIMIT_INT,
	LIMIT_CON,

	/// ���� ���ο� ��� ���� �ǽð����� �ð� ���� (socket0�� �Ҹ� �ð��� ����: unix_timestamp Ÿ��)
	LIMIT_REAL_TIME,						

	/// �������� �� ó�� ���(Ȥ�� ����) �� �������� ����Ÿ�� Ÿ�̸� ���� 
	/// ���� ��� ������ socket0�� ��밡�ɽð�(�ʴ���, 0�̸� �������� limit value�� ���) ���� �����ִٰ� 
	/// ������ ���� socket1�� ��� Ƚ���� ������ socket0�� unix_timestamp Ÿ���� �Ҹ�ð��� ����.
	LIMIT_REAL_TIME_START_FIRST_USE,

	/// �������� ���� ���� ���� ��� �ð��� �����Ǵ� ������
	/// socket0�� ���� �ð��� �ʴ����� ����. (������ ���� ���� �ش� ���� 0�̸� �������� limit value���� socket0�� ����)
	LIMIT_TIMER_BASED_ON_WEAR,

//#ifdef ENABLE_LEVEL_LIMIT_MAX
	LEVEL_LIMIT_MAX,
//#endif

//#ifdef NEW_OFFICIAL
	PC_BANG,
//#endif
	LIMIT_MAX_NUM
};

enum EAttrAddonTypes
{
	ATTR_ADDON_NONE,
	// positive values are reserved for set
	ATTR_DAMAGE_ADDON = -1,
};

enum ERefineType
{
	REFINE_TYPE_NORMAL,
	REFINE_TYPE_NOT_USED1,
	REFINE_TYPE_SCROLL,
	REFINE_TYPE_HYUNIRON,
	REFINE_TYPE_MONEY_ONLY,
	REFINE_TYPE_MUSIN,
	REFINE_TYPE_BDRAGON,
	REFINE_TYPE_MAX_NUM,
};

#ifdef __DRAGONSOUL__
enum EExtractSubTypes
{
	EXTRACT_DRAGON_SOUL,
	EXTRACT_DRAGON_HEART,
};

enum EDragonSoulSubType
{
	DS_SLOT1,
	DS_SLOT2,
	DS_SLOT3,
	DS_SLOT4,
	DS_SLOT5,
	DS_SLOT6,
	DS_SLOT_MAX,
};

enum EDragonSoulGradeTypes
{
	DRAGON_SOUL_GRADE_NORMAL,
	DRAGON_SOUL_GRADE_BRILLIANT,
	DRAGON_SOUL_GRADE_RARE,
	DRAGON_SOUL_GRADE_ANCIENT,
	DRAGON_SOUL_GRADE_LEGENDARY,
	DRAGON_SOUL_GRADE_MAX,

};

enum EDragonSoulStepTypes
{
	DRAGON_SOUL_STEP_LOWEST,
	DRAGON_SOUL_STEP_LOW,
	DRAGON_SOUL_STEP_MID,
	DRAGON_SOUL_STEP_HIGH,
	DRAGON_SOUL_STEP_HIGHEST,
	DRAGON_SOUL_STEP_MAX,
};
#define DRAGON_SOUL_STRENGTH_MAX 7

enum EDSInventoryMaxNum
{
	DRAGON_SOUL_INVENTORY_MAX_NUM = DS_SLOT_MAX * DRAGON_SOUL_GRADE_MAX * DRAGON_SOUL_BOX_SIZE,
};

enum EItemDragonSoulSockets
{
	ITEM_SOCKET_DRAGON_SOUL_ACTIVE_IDX = 2,
	ITEM_SOCKET_CHARGING_AMOUNT_IDX = 2,
};
#endif

#endif
