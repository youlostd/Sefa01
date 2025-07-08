#pragma once
#include "../GameLib/ItemData.h"
#include "protobuf_data_item.h"

#ifdef ENABLE_64BIT_MODE
#define LONG long long
#endif

struct SAffects
{
	enum
	{
		AFFECT_MAX_NUM = 32,
	};

	SAffects() : dwAffects(0) {}
	SAffects(const DWORD & c_rAffects)
	{
		__SetAffects(c_rAffects);
	}
	int operator = (const DWORD & c_rAffects)
	{
		__SetAffects(c_rAffects);
	}

	BOOL IsAffect(BYTE byIndex)
	{
		return dwAffects & (1 << byIndex);
	}

	void __SetAffects(const DWORD & c_rAffects)
	{
		dwAffects = c_rAffects;
	}

	DWORD dwAffects;
};

extern std::string g_strGuildSymbolPathName;

const DWORD c_Name_Max_Length = 64;
const DWORD c_FileName_Max_Length = 128;
const DWORD c_Short_Name_Max_Length = 32;

const DWORD c_Inventory_Page_X_SlotCount = 5;
const DWORD c_Inventory_Page_Y_SlotCount = 9;
const DWORD c_Inventory_Page_Size = c_Inventory_Page_X_SlotCount * c_Inventory_Page_Y_SlotCount;
#ifdef PROMETA
const DWORD c_Inventory_Page_Count = 10;
#elif defined(ELONIA)
const DWORD c_Inventory_Page_Count = 4;
#else // AELDRA
const DWORD c_Inventory_Page_Count = 5;
#endif
const DWORD c_ItemSlot_Count = c_Inventory_Page_Size * c_Inventory_Page_Count;
const DWORD c_Equipment_Count = 12;

#ifdef ENABLE_SKILL_INVENTORY
const DWORD c_SkillbookInv_Page_Size = 5 * 10;
const DWORD c_SkillbookInv_Page_Count = 2;
const DWORD c_SkillbookInv_Count = c_SkillbookInv_Page_Size * c_SkillbookInv_Page_Count;
#endif

const DWORD c_UppitemInv_Page_Size = 5 * 10;
#ifdef PROMETA
const DWORD c_UppitemInv_Page_Count = 3;
#elif defined(ELONIA)
const DWORD c_UppitemInv_Page_Count = 4;
#else // AELDRA
const DWORD c_UppitemInv_Page_Count = 5;
#endif
const DWORD c_UppitemInv_Count = c_UppitemInv_Page_Size * c_UppitemInv_Page_Count;

const DWORD c_StoneInv_Page_Size = 5 * 10;
#ifdef ELONIA
const DWORD c_StoneInv_Page_Count = 2;
#else
const DWORD c_StoneInv_Page_Count = 3;
#endif
const DWORD c_StoneInv_Count = c_StoneInv_Page_Size * c_StoneInv_Page_Count;

const DWORD c_EnchantInv_Page_Size = 5 * 10;
#ifdef ELONIA
const DWORD c_EnchantInv_Page_Count = 4;
#else
const DWORD c_EnchantInv_Page_Count = 3;
#endif
const DWORD c_EnchantInv_Count = c_EnchantInv_Page_Size * c_EnchantInv_Page_Count;

#ifdef ENABLE_COSTUME_INVENTORY
const DWORD c_CostumeInv_Page_Size = 5 * 10;
const DWORD c_CostumeInv_Page_Count = 3;
const DWORD c_CostumeInv_Count = c_CostumeInv_Page_Size * c_CostumeInv_Page_Count;
#endif

const DWORD c_Wear_Max = 32;


#ifdef ENABLE_DRAGONSOUL
enum EDragonSoulDeckType
{
	DS_DECK_1,
	DS_DECK_2,
	DS_DECK_MAX_NUM = 2,
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

const DWORD c_DragonSoul_Inventory_Start = 0;
const DWORD c_DragonSoul_Inventory_Box_Size = 32;
const DWORD c_DragonSoul_Inventory_Count = CItemData::DS_SLOT_NUM_TYPES * DRAGON_SOUL_GRADE_MAX * c_DragonSoul_Inventory_Box_Size;
const DWORD c_DragonSoul_Inventory_End = c_DragonSoul_Inventory_Start + c_DragonSoul_Inventory_Count;

enum EDSInventoryMaxNum
{
	DS_INVENTORY_MAX_NUM = c_DragonSoul_Inventory_Count,
	DS_REFINE_WINDOW_MAX_NUM = 15,
};

const DWORD c_DragonSoul_Equip_Slot_Max = 6;
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
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

enum ESlotData {
	INVENTORY_SLOT_START,
	INVENTORY_SLOT_END = INVENTORY_SLOT_START + c_ItemSlot_Count,
#ifdef ENABLE_SKILL_INVENTORY
	SKILLBOOK_INV_SLOT_START,
	SKILLBOOK_INV_SLOT_END = SKILLBOOK_INV_SLOT_START + c_SkillbookInv_Count,
#endif
	UPPITEM_INV_SLOT_START,
	UPPITEM_INV_SLOT_END = UPPITEM_INV_SLOT_START + c_UppitemInv_Count,
	STONE_INV_SLOT_START,
	STONE_INV_SLOT_END = STONE_INV_SLOT_START + c_StoneInv_Count,
	ENCHANT_INV_SLOT_START,
	ENCHANT_INV_SLOT_END = ENCHANT_INV_SLOT_START + c_EnchantInv_Count,

#ifdef ENABLE_COSTUME_INVENTORY
	COSTUME_INV_SLOT_START,
	COSTUME_INV_SLOT_END = COSTUME_INV_SLOT_START + c_CostumeInv_Count,
#endif

	EQUIPMENT_SLOT_START,
	EQUIPMENT_SLOT_END = EQUIPMENT_SLOT_START + c_Wear_Max,
#ifdef ENABLE_DRAGONSOUL
	DRAGON_SOUL_EQUIP_SLOT_START = EQUIPMENT_SLOT_END,
	DRAGON_SOUL_EQUIP_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_START + (c_DragonSoul_Equip_Slot_Max * DS_DECK_MAX_NUM),
	DRAGON_SOUL_EQUIP_RESERVED_SLOT_END = DRAGON_SOUL_EQUIP_SLOT_END + (c_DragonSoul_Equip_Slot_Max * 3),

	SHINING_EQUIP_SLOT_START = DRAGON_SOUL_EQUIP_RESERVED_SLOT_END,
	SHINING_EQUIP_SLOT_END = SHINING_EQUIP_SLOT_START + SHINING_MAX_NUM,
	SHINING_EQUIP_RESERVED_SLOT_END = SHINING_EQUIP_SLOT_END + SHINING_RESERVED,
#else
	SHINING_EQUIP_SLOT_START = EQUIPMENT_SLOT_END,
	SHINING_EQUIP_SLOT_END = SHINING_EQUIP_SLOT_START + SHINING_MAX_NUM,
	SHINING_EQUIP_RESERVED_SLOT_END = SHINING_EQUIP_SLOT_END + SHINING_RESERVED,
#endif

#ifdef ENABLE_SKIN_SYSTEM
	SKINSYSTEM_EQUIP_SLOT_START = SHINING_EQUIP_RESERVED_SLOT_END,
	SKINSYSTEM_EQUIP_SLOT_END = SKINSYSTEM_EQUIP_SLOT_START + SKINSYSTEM_MAX_NUM,
	SKINSYSTEM_EQUIP_RESERVED_SLOT_END = SKINSYSTEM_EQUIP_SLOT_END + SKINSYSTEM_RESERVED,
#endif
};

const DWORD c_Equipment_Start = EQUIPMENT_SLOT_START;

const DWORD c_Equipment_Body	= c_Equipment_Start + 0;
const DWORD c_Equipment_Head	= c_Equipment_Start + 1;
const DWORD c_Equipment_Shoes	= c_Equipment_Start + 2;
const DWORD c_Equipment_Wrist	= c_Equipment_Start + 3;
const DWORD c_Equipment_Weapon	= c_Equipment_Start + 4;
const DWORD c_Equipment_Neck	= c_Equipment_Start + 5;
const DWORD c_Equipment_Ear		= c_Equipment_Start + 6;
const DWORD c_Equipment_Unique1	= c_Equipment_Start + 7;
const DWORD c_Equipment_Unique2	= c_Equipment_Start + 8;
const DWORD c_Equipment_Arrow	= c_Equipment_Start + 9;
const DWORD c_Equipment_Shield	= c_Equipment_Start + 10;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
const DWORD c_Equipment_Acce	= c_Equipment_Start + 24;
#endif
#ifdef ENABLE_BELT_SYSTEM
const DWORD c_Equipment_Belt	= c_Equipment_Start + 25;
#endif
const DWORD c_Equipment_Unique3 = c_Equipment_Start + 26;
const DWORD c_Equipment_Unique4 = c_Equipment_Start + 27;

const DWORD c_Equipment_AcceCostume = c_Equipment_Start + 29;

#ifdef ENABLE_SKIN_SYSTEM
const DWORD c_SkinSystem_Slot_Pet = SKINSYSTEM_EQUIP_SLOT_START;
const DWORD c_SkinSystem_Slot_Mount = c_SkinSystem_Slot_Pet + 1;
const DWORD c_SkinSystem_Slot_BuffiBody = c_SkinSystem_Slot_Mount + 1;
const DWORD c_SkinSystem_Slot_BuffiWeapon = c_SkinSystem_Slot_BuffiBody + 1;
const DWORD c_SkinSystem_Slot_BuffiHair = c_SkinSystem_Slot_BuffiWeapon + 1;
const DWORD c_Inventory_Count = SKINSYSTEM_EQUIP_RESERVED_SLOT_END;
#else
const DWORD c_Inventory_Count = SHINING_EQUIP_RESERVED_SLOT_END;
#endif

#ifdef ENABLE_COSTUME_SYSTEM
	const DWORD c_Costume_Slot_Start	= c_Equipment_Start + 19;	// [주의] 숫자(19) 하드코딩 주의. 현재 서버에서 코스츔 슬롯은 19부터임. 서버 common/length.h 파일의 EWearPositions 열거형 참고.
	const DWORD	c_Costume_Slot_Body		= c_Costume_Slot_Start + 0;
	const DWORD	c_Costume_Slot_Hair		= c_Costume_Slot_Start + 1;
	const DWORD c_Costume_Slot_Count	= 2;
	const DWORD c_Costume_Slot_End		= c_Costume_Slot_Start + c_Costume_Slot_Count;
#endif

enum ESlotType
{
	SLOT_TYPE_NONE,
	SLOT_TYPE_INVENTORY,
	SLOT_TYPE_SKILL,
	SLOT_TYPE_EMOTION,
	SLOT_TYPE_SHOP,
	SLOT_TYPE_EXCHANGE_OWNER,
	SLOT_TYPE_EXCHANGE_TARGET,
	SLOT_TYPE_QUICK_SLOT,
	SLOT_TYPE_SAFEBOX,
	SLOT_TYPE_PRIVATE_SHOP,
	SLOT_TYPE_MALL,
#ifdef ENABLE_GUILD_SAFEBOX
	SLOT_TYPE_GUILD_SAFEBOX,
#endif
#ifdef ENABLE_DRAGONSOUL
	SLOT_TYPE_DRAGON_SOUL_INVENTORY,
#endif
#ifdef ENABLE_SKILL_INVENTORY
	SLOT_TYPE_SKILLBOOK_INVENTORY,
#endif
	SLOT_TYPE_UPPITEM_INVENTORY,
	SLOT_TYPE_STONE_INVENTORY,
	SLOT_TYPE_ENCHANT_INVENTORY,

// here?
#ifdef ENABLE_COSTUME_INVENTORY
	SLOT_TYPE_COSTUME_INVENTORY,
#endif

	SLOT_TYPE_RECOVERY_INVENTORY,
#ifdef AHMET_FISH_EVENT_SYSTEM
	SLOT_TYPE_FISH_EVENT,
#endif
#ifdef ENABLE_AUCTION
	SLOT_TYPE_AUCTION_SHOP,
#endif

	SLOT_TYPE_MAX,
};

enum EWindows
{
	RESERVED_WINDOW,
	INVENTORY,
#ifdef ENABLE_SKILL_INVENTORY
	SKILLBOOK_INVENTORY,
#endif
#ifdef ENABLE_DRAGONSOUL
	DRAGON_SOUL_INVENTORY,
#endif
	EQUIPMENT,
	UPPITEM_INVENTORY,
	STONE_INVENTORY,
	SAFEBOX,
	MALL,
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	ACCEREFINE,
#endif
#ifdef ENABLE_GUILD_SAFEBOX
	GUILD_SAFEBOX,
#endif
#ifdef ENABLE_AUCTION
	AUCTION_SHOP,
	AUCTION,
#endif
	ENCHANT_INVENTORY,

#ifdef ENABLE_COSTUME_INVENTORY
	COSTUME_INVENTORY,
#endif

	RECOVERY_INVENTORY,
	GROUND,
	WINDOW_TYPE_MAX,
};

#ifdef AHMET_FISH_EVENT_SYSTEM
enum EFishEventInfo
{
	FISH_EVENT_SHAPE_NONE,
	FISH_EVENT_SHAPE_1,
	FISH_EVENT_SHAPE_2,
	FISH_EVENT_SHAPE_3,
	FISH_EVENT_SHAPE_4,
	FISH_EVENT_SHAPE_5,
	FISH_EVENT_SHAPE_6,
	FISH_EVENT_SHAPE_7,
	FISH_EVENT_SHAPE_MAX_NUM,
};
#endif

enum ETaxes {
	PRIVATE_SHOP_TAX = 3,
};

#ifdef CHANGE_SKILL_COLOR
enum ESkillColorLength
{
#ifdef ENABLE_FAKEBUFF
	MAX_SKILL_COUNT = 9,
#else
	MAX_SKILL_COUNT = 6,
#endif
	MAX_EFFECT_COUNT = 5,
};
#endif

#pragma pack (push, 1)
#define WORD_MAX 0xffff

typedef struct SItemPos
{
	BYTE window_type;
	WORD cell;
    SItemPos ()
    {
		window_type =     INVENTORY;
		cell = WORD_MAX;
    }
	SItemPos (BYTE _window_type, WORD _cell)
    {
        window_type = _window_type;
        cell = _cell;
    }

	SItemPos(const network::TItemPos& _cell)
	{
		window_type = _cell.window_type();
		cell = _cell.cell();
	}

	operator network::TItemPos() const
	{
		network::TItemPos ret;
		ret.set_window_type(window_type);
		ret.set_cell(cell);

		return ret;
	}

	// 기존에 cell의 형을 보면 BYTE가 대부분이지만, oi
	// 어떤 부분은 int, 어떤 부분은 WORD로 되어있어,
	// 가장 큰 자료형인 int로 받는다.
  //  int operator=(const int _cell)
  //  {
		//window_type = INVENTORY;
  //      cell = _cell;
  //      return cell;
  //  }
	bool IsValidCell()
	{
		switch (window_type)
		{
		case INVENTORY:
#ifdef ENABLE_SKILL_INVENTORY
		case SKILLBOOK_INVENTORY:
#endif
		case UPPITEM_INVENTORY:
		case STONE_INVENTORY:
		case ENCHANT_INVENTORY:
#ifdef ENABLE_COSTUME_INVENTORY
		case COSTUME_INVENTORY:
#endif
		case EQUIPMENT:
			return cell < c_Inventory_Count;
			break;
#ifdef ENABLE_DRAGONSOUL
		case DRAGON_SOUL_INVENTORY:
			return cell < (DS_INVENTORY_MAX_NUM);
			break;
#endif
		default:
			return false;
		}
	}
	bool IsEquipCell()
	{
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if (cell >= SHINING_EQUIP_SLOT_START && cell < SHINING_EQUIP_SLOT_END)
				return true;

#ifdef ENABLE_SKIN_SYSTEM
			if(cell >= SKINSYSTEM_EQUIP_SLOT_START && cell < SKINSYSTEM_EQUIP_SLOT_END)
				return true;
#endif

			return (c_Equipment_Start + c_Wear_Max > cell) && (c_Equipment_Start <= cell);
			break;

		default:
			return false;
		}
	}

	bool operator==(const struct SItemPos& rhs) const
	{
		return (window_type == rhs.window_type) && (cell == rhs.cell);
	}

	bool operator<(const struct SItemPos& rhs) const
	{
		return (window_type < rhs.window_type) || ((window_type == rhs.window_type) && (cell < rhs.cell));
	}
} TItemPos;
const ::TItemPos NPOS(RESERVED_WINDOW, WORD_MAX);

namespace std {

	template <>
	struct hash<::TItemPos>
	{
		std::size_t operator()(const ::TItemPos& k) const
		{
			using std::size_t;
			using std::hash;
			using std::string;

			// Compute individual hash values for first,
			// second and third and combine them using XOR
			// and bit shifting:

			size_t cell = k.cell;
			return (static_cast<size_t>(k.cell) << 8) + k.window_type;
		}
	};

}
#pragma pack(pop)

const DWORD c_QuickBar_Line_Count = 3;
const DWORD c_QuickBar_Slot_Count = 12;

const float c_Idle_WaitTime = 5.0f;

const int c_Monster_Race_Start_Number = 6;
const int c_Monster_Model_Start_Number = 20001;

const float c_fAttack_Delay_Time = 0.2f;
const float c_fHit_Delay_Time = 0.1f;
const float c_fCrash_Wave_Time = 0.2f;
const float c_fCrash_Wave_Distance = 3.0f;

const float c_fHeight_Step_Distance = 50.0f;

enum
{
	DISTANCE_TYPE_FOUR_WAY,
	DISTANCE_TYPE_EIGHT_WAY,
	DISTANCE_TYPE_ONE_WAY,
	DISTANCE_TYPE_MAX_NUM,
};

const float c_fMagic_Script_Version = 1.0f;
const float c_fSkill_Script_Version = 1.0f;
const float c_fMagicSoundInformation_Version = 1.0f;
const float c_fBattleCommand_Script_Version = 1.0f;
const float c_fEmotionCommand_Script_Version = 1.0f;
const float c_fActive_Script_Version = 1.0f;
const float c_fPassive_Script_Version = 1.0f;

// Used by PushMove
const float c_fWalkDistance = 175.0f;
const float c_fRunDistance = 310.0f;

#define FILE_MAX_LEN 128

enum
{
	ITEM_SOCKET_SLOT_MAX_NUM = 3,
	ITEM_NORMAL_ATTRIBUTE_SLOT_MAX_NUM = 5,
	ITEM_ATTRIBUTE_SLOT_MAX_NUM = 7,
};

inline float GetSqrtDistance(int ix1, int iy1, int ix2, int iy2) // By sqrt
{
	float dx, dy;

	dx = float(ix1 - ix2);
	dy = float(iy1 - iy2);

	return sqrtf(dx*dx + dy*dy);
}

// DEFAULT_FONT
void DefaultFont_Startup();
void DefaultFont_Cleanup();
void DefaultFont_SetName(const char * c_szFontName);
CResource* DefaultFont_GetResource();
CResource* DefaultNewFont1_GetResource();
CResource* DefaultNewFont2_GetResource();
CResource* DefaultItalicFont_GetResource();
// END_OF_DEFAULT_FONT

void SetGuildSymbolPath(const char * c_szPathName);
const char * GetGuildSymbolFileName(DWORD dwGuildID);
BYTE SlotTypeToInvenType(BYTE bSlotType);
BYTE ApplyTypeToPointType(BYTE bApplyType);
