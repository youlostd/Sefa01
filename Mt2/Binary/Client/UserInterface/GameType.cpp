#include "StdAfx.h"
#include "GameType.h"

std::string g_strResourcePath = "d:/ymir work/";
std::string g_strImagePath = "d:/ymir work/ui/";

std::string g_strGuildSymbolPathName = "mark/10/";

// DEFAULT_FONT
static std::string gs_strDefaultFontName = "±Ľ¸˛ĂĽ:12.fnt";
static std::string gs_strDefaultNewFontName1 = "Tahoma-12.fnt";
static std::string gs_strDefaultNewFontName2 = "Arial:14.fnt";
static std::string gs_strDefaultItalicFontName = "±Ľ¸˛ĂĽ:12i.fnt";
static CResource* gs_pkDefaultFont = NULL;
static CResource* gs_pkDefaultNewFont1 = NULL;
static CResource* gs_pkDefaultNewFont2 = NULL;
static CResource* gs_pkDefaultItalicFont = NULL;

static bool gs_isReloadDefaultFont = false;

typedef struct SApplyInfo
{
	BYTE	bPointType;                          // APPLY -> POINT
} TApplyInfo;

const TApplyInfo aApplyInfo[CItemData::MAX_APPLY_NUM] =
{
	// Point Type
	{ POINT_NONE, },   // APPLY_NONE,		0
	{ POINT_MAX_HP, },   // APPLY_MAX_HP,		1
	{ POINT_MAX_SP, },   // APPLY_MAX_SP,		2
	{ POINT_HT, },   // APPLY_CON,		3
	{ POINT_IQ, },   // APPLY_INT,		4
	{ POINT_ST, },   // APPLY_STR,		5
	{ POINT_DX, },   // APPLY_DEX,		6
	{ POINT_ATT_SPEED, },   // APPLY_ATT_SPEED,	7
	{ POINT_MOV_SPEED, },   // APPLY_MOV_SPEED,	8
	{ POINT_CASTING_SPEED, },   // APPLY_CAST_SPEED,	9
	{ POINT_HP_REGEN, },   // APPLY_HP_REGEN,		10
	{ POINT_SP_REGEN, },   // APPLY_SP_REGEN,		11
	{ POINT_POISON_PCT, },   // APPLY_POISON_PCT,	12
	{ POINT_STUN_PCT, },   // APPLY_STUN_PCT,		13
	{ POINT_SLOW_PCT, },   // APPLY_SLOW_PCT,		14
	{ POINT_CRITICAL_PCT, },   // APPLY_CRITICAL_PCT,	15
	{ POINT_PENETRATE_PCT, },   // APPLY_PENETRATE_PCT,	16
	{ POINT_ATTBONUS_HUMAN, },   // APPLY_ATTBONUS_HUMAN,	17
	{ POINT_ATTBONUS_ANIMAL, },   // APPLY_ATTBONUS_ANIMAL,	18
	{ POINT_ATTBONUS_ORC, },   // APPLY_ATTBONUS_ORC,	19
	{ POINT_ATTBONUS_MILGYO, },   // APPLY_ATTBONUS_MILGYO,	20
	{ POINT_ATTBONUS_UNDEAD, },   // APPLY_ATTBONUS_UNDEAD,	21
	{ POINT_ATTBONUS_DEVIL, },   // APPLY_ATTBONUS_DEVIL,	22
	{ POINT_STEAL_HP, },   // APPLY_STEAL_HP,		23
	{ POINT_STEAL_SP, },   // APPLY_STEAL_SP,		24
	{ POINT_MANA_BURN_PCT, },   // APPLY_MANA_BURN_PCT,	25
	{ POINT_DAMAGE_SP_RECOVER, },   // APPLY_DAMAGE_SP_RECOVER,26
	{ POINT_BLOCK, },   // APPLY_BLOCK,		27
	{ POINT_DODGE, },   // APPLY_DODGE,		28
	{ POINT_RESIST_SWORD, },   // APPLY_RESIST_SWORD,	29
	{ POINT_RESIST_TWOHAND, },   // APPLY_RESIST_TWOHAND,	30
	{ POINT_RESIST_DAGGER, },   // APPLY_RESIST_DAGGER,	31
	{ POINT_RESIST_BELL, },   // APPLY_RESIST_BELL,	32
	{ POINT_RESIST_FAN, },   // APPLY_RESIST_FAN,	33
	{ POINT_RESIST_BOW, },   // APPLY_RESIST_BOW,	34
	{ POINT_RESIST_FIRE, },   // APPLY_RESIST_FIRE,	35
	{ POINT_RESIST_ELEC, },   // APPLY_RESIST_ELEC,	36
	{ POINT_RESIST_MAGIC, },   // APPLY_RESIST_MAGIC,	37
	{ POINT_RESIST_WIND, },   // APPLY_RESIST_WIND,	38
	{ POINT_REFLECT_MELEE, },   // APPLY_REFLECT_MELEE,	39
	{ POINT_REFLECT_CURSE, },   // APPLY_REFLECT_CURSE,	40
	{ POINT_POISON_REDUCE, },   // APPLY_POISON_REDUCE,	41
	{ POINT_KILL_SP_RECOVER, },   // APPLY_KILL_SP_RECOVER,	42
	{ POINT_EXP_DOUBLE_BONUS, },   // APPLY_EXP_DOUBLE_BONUS,	43
	{ POINT_GOLD_DOUBLE_BONUS, },   // APPLY_GOLD_DOUBLE_BONUS,44
	{ POINT_ITEM_DROP_BONUS, },   // APPLY_ITEM_DROP_BONUS,	45
	{ POINT_POTION_BONUS, },   // APPLY_POTION_BONUS,	46
	{ POINT_KILL_HP_RECOVERY, },   // APPLY_KILL_HP_RECOVER,	47
	{ POINT_IMMUNE_STUN, },   // APPLY_IMMUNE_STUN,	48
	{ POINT_IMMUNE_SLOW, },   // APPLY_IMMUNE_SLOW,	49
	{ POINT_IMMUNE_FALL, },   // APPLY_IMMUNE_FALL,	50
	{ POINT_NONE, },   // APPLY_SKILL,		51
	{ POINT_BOW_DISTANCE, },   // APPLY_BOW_DISTANCE,	52
	{ POINT_ATT_GRADE_BONUS, },   // APPLY_ATT_GRADE,	53
	{ POINT_DEF_GRADE_BONUS, },   // APPLY_DEF_GRADE,	54
	{ POINT_MAGIC_ATT_GRADE_BONUS, },   // APPLY_MAGIC_ATT_GRADE,	55
	{ POINT_MAGIC_DEF_GRADE_BONUS, },   // APPLY_MAGIC_DEF_GRADE,	56
	{ POINT_CURSE_PCT, },   // APPLY_CURSE_PCT,	57
	{ POINT_MAX_STAMINA },   // APPLY_MAX_STAMINA	58
	{ POINT_ATTBONUS_WARRIOR },   // APPLY_ATTBONUS_WARRIOR  59
	{ POINT_ATTBONUS_ASSASSIN },   // APPLY_ATTBONUS_ASSASSIN 60
	{ POINT_ATTBONUS_SURA },   // APPLY_ATTBONUS_SURA	 61
	{ POINT_ATTBONUS_SHAMAN },   // APPLY_ATTBONUS_SHAMAN   62
	{ POINT_ATTBONUS_MONSTER },   //	APPLY_ATTBONUS_MONSTER  63
	{ POINT_ATT_BONUS },   // 64 // APPLY_MALL_ATTBONUS
	{ POINT_MALL_DEFBONUS },   // 65
	{ POINT_MALL_EXPBONUS },   // 66 APPLY_MALL_EXPBONUS
	{ POINT_MALL_ITEMBONUS },   // 67
	{ POINT_MALL_GOLDBONUS },   // 68
	{ POINT_MAX_HP_PCT },		// 69
	{ POINT_MAX_SP_PCT },		// 70
	{ POINT_SKILL_DAMAGE_BONUS },	// 71
	{ POINT_NORMAL_HIT_DAMAGE_BONUS },	// 72

										// DEFEND_BONUS_ATTRIBUTES
	{ POINT_SKILL_DEFEND_BONUS },	// 73
	{ POINT_NORMAL_HIT_DEFEND_BONUS },	// 74
										// END_OF_DEFEND_BONUS_ATTRIBUTES

	{ POINT_NONE, },				// 75 »ç¿ë½Ã HP ¼Ò¸ð APPLY_EXTRACT_HP_PCT

	{ POINT_RESIST_WARRIOR, },		// 76 ¹«»ç¿¡°Ô ÀúÇ× APPLY_RESIST_WARRIOR
	{ POINT_RESIST_ASSASSIN, },		// 77 ÀÚ°´¿¡°Ô ÀúÇ× APPLY_RESIST_ASSASSIN
	{ POINT_RESIST_SURA, },		// 78 ¼ö¶ó¿¡°Ô ÀúÇ× APPLY_RESIST_SURA
	{ POINT_RESIST_SHAMAN, },		// 79 ¹«´ç¿¡°Ô ÀúÇ× APPLY_RESIST_SHAMAN
	{ POINT_ENERGY },		// 80 ±â·Â 
	{ POINT_DEF_GRADE },		// 81 ¹æ¾î·Â. DEF_GRADE_BONUS´Â Å¬¶ó¿¡¼­ µÎ¹è·Î º¸¿©Áö´Â ÀÇµµµÈ ¹ö±×(...)°¡ ÀÖ´Ù.
	{ POINT_COSTUME_ATTR_BONUS },		// 82 ÄÚ½ºÆ¬¿¡ ºÙÀº ¼Ó¼º¿¡ ´ëÇØ¼­¸¸ º¸³Ê½º¸¦ ÁÖ´Â ±â·Â
	{ POINT_MAGIC_ATT_BONUS_PER },			// 83 ¸¶¹ý °ø°Ý·Â +x%
	{ POINT_MELEE_MAGIC_ATT_BONUS_PER },			// 84 APPLY_MELEE_MAGIC_ATTBONUS_PER
	{ POINT_RESIST_ICE, },   // APPLY_RESIST_ICE,	85
	{ POINT_RESIST_EARTH, },   // APPLY_RESIST_EARTH,	86
	{ POINT_RESIST_DARK, },   // APPLY_RESIST_DARK,	87
	{ POINT_RESIST_CRITICAL, },   // APPLY_ANTI_CRITICAL_PCT,	88
	{ POINT_RESIST_PENETRATE, },   // APPLY_ANTI_PENETRATE_PCT,	89
	{ POINT_EXP_REAL_BONUS, },	// APPLY_EXP_REAL_BONUS	90

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	{ POINT_ACCEDRAIN_RATE, },	 // APPLY_ACCEDRAIN_RATE
#endif

#ifdef __ANIMAL_SYSTEM__
#ifdef __PET_SYSTEM__
	{ POINT_PET_EXP_BONUS },
#endif
	{ POINT_MOUNT_EXP_BONUS },
#endif

	{ POINT_MOUNT_BUFF_BONUS },

	{ POINT_RESIST_MONSTER },
	{ POINT_ATTBONUS_METIN },
	{ POINT_ATTBONUS_BOSS, },
	{ POINT_RESIST_HUMAN, },

	{ POINT_RESIST_SWORD_PEN }, //APPLY_RESIST_SWORD_PEN
	{ POINT_RESIST_TWOHAND_PEN }, //	APPLY_RESIST_TWOHAND_PEN
	{ POINT_RESIST_DAGGER_PEN }, //	APPLY_RESIST_DAGGER_PEN
	{ POINT_RESIST_BELL_PEN }, //	APPLY_RESIST_BELL_PEN
	{ POINT_RESIST_FAN_PEN }, //	APPLY_RESIST_FAN_PEN
	{ POINT_RESIST_BOW_PEN }, //	APPLY_RESIST_BOW_PEN
	{ POINT_RESIST_ATTBONUS_HUMAN }, //	APPLY_RESIST_ATTBONUS_HUMAN
// #ifdef __ELEMENT_SYSTEM__
	{ POINT_ATTBONUS_ELEC, },
	{ POINT_ATTBONUS_FIRE, },
	{ POINT_ATTBONUS_ICE, },
	{ POINT_ATTBONUS_WIND, },
	{ POINT_ATTBONUS_EARTH, },
	{ POINT_ATTBONUS_DARK, },
// #endif

	{ POINT_DEF_BONUS }, // APPLY_DEFENSE_BONUS
	{ POINT_ANTI_RESIST_MAGIC }, // APPLY_ANTI_RESIST_MAGIC
	{ POINT_BLOCK_IGNORE_BONUS }, // APPLY_BLOCK_IGNORE_BONUS
	
// #ifdef ENABLE_RUNE_SYSTEM
	{ POINT_RUNE_SHIELD_PER_HIT }, // APPLY_RUNE_SHIELD_PER_HIT
	{ POINT_RUNE_HEAL_ON_KILL }, // APPLY_RUNE_HEAL_ON_KILL
	{ POINT_RUNE_BONUS_DAMAGE_AFTER_HIT }, // APPLY_RUNE_BONUS_DAMAGE_AFTER_HIT
	{ POINT_RUNE_3RD_ATTACK_BONUS }, // APPLY_POINT_RUNE_3RD_ATTACK_BONUS
	{ POINT_RUNE_FIRST_NORMAL_HIT_BONUS }, // APPLY_RUNE_FIRST_NORMAL_HIT_BONUS
	{ POINT_RUNE_MSHIELD_PER_SKILL }, // APPLY_RUNE_MSHIELD_PER_SKILL
	{ POINT_RUNE_HARVEST }, // APPLY_RUNE_HARVEST
	{ POINT_RUNE_DAMAGE_AFTER_3 }, // APPLY_RUNE_DAMAGE_AFTER_3
	{ POINT_RUNE_OUT_OF_COMBAT_SPEED }, // APPLY_RUNE_OUT_OF_COMBAT_SPEED
	{ POINT_RUNE_RESET_SKILL }, // APPLY_RUNE_RESET_SKILL
	{ POINT_RUNE_COMBAT_CASTING_SPEED }, // APPLY_RUNE_COMBAT_CASTING_SPEED
	{ POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT }, // APPLY_RUNE_MAGIC_DAMAGE_AFTER_HIT
	{ POINT_RUNE_MOVSPEED_AFTER_3 }, // APPLY_RUNE_MOVSPEED_AFTER_3
	{ POINT_RUNE_SLOW_ON_ATTACK }, // APPLY_RUNE_SLOW_ON_ATTACK
// #endif
	
	{ POINT_HEAL_EFFECT_BONUS }, // APPLY_HEAL_EFFECT_BONUS
	{ POINT_CRITICAL_DAMAGE_BONUS }, // APPLY_CRITICAL_DAMAGE_BONUS
	{ POINT_DOUBLE_ITEM_DROP_BONUS }, // APPLY_DOUBLE_ITEM_DROP_BONUS
	{ POINT_DAMAGE_BY_SP_BONUS }, // APPLY_DAMAGE_BY_SP_BONUS
	{ POINT_SINGLETARGET_SKILL_DAMAGE_BONUS }, // APPLY_SINGLETARGET_SKILL_DAMAGE_BONUS
	{ POINT_MULTITARGET_SKILL_DAMAGE_BONUS }, // APPLY_MULTITARGET_SKILL_DAMAGE_BONUS
	{ POINT_MIXED_DEFEND_BONUS }, // APPLY_MIXED_DEFEND_BONUS
	{ POINT_EQUIP_SKILL_BONUS }, //APPLY_EQUIP_SKILL_BONUS
	{ POINT_AURA_HEAL_EFFECT_BONUS },	// APPLY_AURA_HEAL_EFFECT_BONUS
	{ POINT_AURA_EQUIP_SKILL_BONUS },	// APPLY_AURA_EQUIP_SKILL_BONUS

// #ifdef ENABLE_RUNE_SYSTEM
	{ POINT_RUNE_LEADERSHIP_BONUS },
	{ POINT_RUNE_MOUNT_PARALYZE },
//#ifdef RUNE_CRITICAL_POINT
	{ POINT_RUNE_CRITICAL_PVM },	// APPLY_RUNE_CRITICAL_PVM
//#endif
// #endif

	{ POINT_ATTBONUS_ALL_ELEMENTS }, // APPLY_ATTBONUS_ALL_ELEMENTS
// #ifdef ENABLE_ZODIAC
	{ POINT_ATTBONUS_ZODIAC }, // APPLY_ATTBONUS_ZODIAC
// #endif

	{ POINT_BONUS_UPGRADE_CHANCE }, //APPLY_BONUS_UPGRADE_CHANCE
	{ POINT_LOWER_DUNGEON_CD }, // APPLY_LOWER_DUNGEON_CD
	{ POINT_LOWER_BIOLOG_CD }, // APPLY_LOWER_BIOLOG_CD

#ifdef __WOLFMAN__
	{ POINT_BLEEDING_PCT, },	 // APPLY_BLEEDING_PCT
	{ POINT_BLEEDING_REDUCE, },	 // APPLY_BLEEDING_REDUCE
	{ POINT_ATTBONUS_WOLFMAN, },	 // APPLY_ATTBONUS_WOLFMAN
	{ POINT_RESIST_WOLFMAN, },	 // APPLY_RESIST_WOLFMAN
	{ POINT_RESIST_CLAW },		 // APPLY_RESIST_CLAW
#endif

	{ POINT_RESIST_MAGIC_REDUCTION }, // APPLY_RESIST_MAGIC_REDUCTION
// #ifdef STANDARD_SKILL_DURATION
	{ POINT_SKILL_DURATION },
// #endif
	{ POINT_RESIST_BOSS },	
};

void DefaultFont_Startup()
{
	gs_pkDefaultFont = NULL;
	gs_pkDefaultNewFont1 = NULL;
	gs_pkDefaultNewFont2 = NULL;
}

void DefaultFont_Cleanup()
{
	if (gs_pkDefaultFont)
		gs_pkDefaultFont->Release();

	if (gs_pkDefaultNewFont1)
		gs_pkDefaultNewFont1->Release();

	if (gs_pkDefaultNewFont2)
		gs_pkDefaultNewFont2->Release();
}

void DefaultFont_SetName(const char * c_szFontName)
{
	gs_strDefaultFontName = c_szFontName;
	gs_strDefaultFontName += ".fnt";

	gs_strDefaultItalicFontName = c_szFontName;
	if(strchr(c_szFontName, ':'))
		gs_strDefaultItalicFontName += "i";
	gs_strDefaultItalicFontName += ".fnt";

	gs_isReloadDefaultFont = true;
}

bool ReloadDefaultFonts()
{
	CResourceManager& rkResMgr = CResourceManager::Instance();

	gs_isReloadDefaultFont = false;

	CResource* pkNewFont = rkResMgr.GetResourcePointer(gs_strDefaultFontName.c_str());
	pkNewFont->AddReference();
	if (gs_pkDefaultFont)
		gs_pkDefaultFont->Release();
	gs_pkDefaultFont = pkNewFont;

	CResource* pkNewFont2 = rkResMgr.GetResourcePointer(gs_strDefaultNewFontName1.c_str());
	pkNewFont2->AddReference();
	if (gs_pkDefaultNewFont1)
		gs_pkDefaultNewFont1->Release();
	gs_pkDefaultNewFont1 = pkNewFont2;

	CResource* pkNewFont3 = rkResMgr.GetResourcePointer(gs_strDefaultNewFontName2.c_str());
	pkNewFont2->AddReference();
	if (gs_pkDefaultNewFont2)
		gs_pkDefaultNewFont2->Release();
	gs_pkDefaultNewFont2 = pkNewFont3;

	CResource* pkNewItalicFont = rkResMgr.GetResourcePointer(gs_strDefaultItalicFontName.c_str());
	pkNewItalicFont->AddReference();
	if (gs_pkDefaultItalicFont)
		gs_pkDefaultItalicFont->Release();
	gs_pkDefaultItalicFont = pkNewItalicFont;

	return true;
}

CResource* DefaultFont_GetResource()
{	
	if (!gs_pkDefaultFont || gs_isReloadDefaultFont)
		ReloadDefaultFonts();
	return gs_pkDefaultFont;
}

CResource* DefaultNewFont1_GetResource()
{
	if (!gs_pkDefaultNewFont1 || gs_isReloadDefaultFont)
		ReloadDefaultFonts();
	return gs_pkDefaultNewFont1;
}

CResource* DefaultNewFont2_GetResource()
{
	if (!gs_pkDefaultNewFont2 || gs_isReloadDefaultFont)
		ReloadDefaultFonts();
	return gs_pkDefaultNewFont2;
}

CResource* DefaultItalicFont_GetResource()
{	
	if (!gs_pkDefaultItalicFont || gs_isReloadDefaultFont)
		ReloadDefaultFonts();
	return gs_pkDefaultItalicFont;
}

// END_OF_DEFAULT_FONT

void SetGuildSymbolPath(const char * c_szPathName)
{
	g_strGuildSymbolPathName = "mark/";
	g_strGuildSymbolPathName += c_szPathName;
	g_strGuildSymbolPathName += "/";
}

const char * GetGuildSymbolFileName(DWORD dwGuildID)
{
	return _getf("%s%03d.jpg", g_strGuildSymbolPathName.c_str(), dwGuildID);
}

BYTE c_aSlotTypeToInvenType[SLOT_TYPE_MAX] =
{
	RESERVED_WINDOW,		// SLOT_TYPE_NONE
	INVENTORY,				// SLOT_TYPE_INVENTORY
	RESERVED_WINDOW,		// SLOT_TYPE_SKILL
	RESERVED_WINDOW,		// SLOT_TYPE_EMOTION
	RESERVED_WINDOW,		// SLOT_TYPE_SHOP
	RESERVED_WINDOW,		// SLOT_TYPE_EXCHANGE_OWNER
	RESERVED_WINDOW,		// SLOT_TYPE_EXCHANGE_TARGET
	RESERVED_WINDOW,		// SLOT_TYPE_QUICK_SLOT
	RESERVED_WINDOW,		// SLOT_TYPE_SAFEBOX	<- SAFEBOX, MALLŔÇ °ćżě ÇĎµĺ ÄÚµůµÇľîŔÖ´Â LEGACY ÄÚµĺ¸¦ ŔŻÁöÇÔ.
	RESERVED_WINDOW,		// SLOT_TYPE_PRIVATE_SHOP
	RESERVED_WINDOW,		// SLOT_TYPE_MALL		<- SAFEBOX, MALLŔÇ °ćżě ÇĎµĺ ÄÚµůµÇľîŔÖ´Â LEGACY ÄÚµĺ¸¦ ŔŻÁöÇÔ.
#ifdef ENABLE_GUILD_SAFEBOX
	RESERVED_WINDOW,		// SLOT_TYPE_GUILD_SAFEBOX
#endif
#ifdef ENABLE_DRAGONSOUL
	DRAGON_SOUL_INVENTORY,
#endif
#ifdef ENABLE_SKILL_INVENTORY
	SKILLBOOK_INVENTORY,
#endif
	UPPITEM_INVENTORY,
	STONE_INVENTORY,
	ENCHANT_INVENTORY,
#ifdef ENABLE_COSTUME_INVENTORY
	COSTUME_INVENTORY,
#endif
	RECOVERY_INVENTORY,
#ifdef AHMET_FISH_EVENT_SYSTEM
	RESERVED_WINDOW,
#endif
#ifdef ENABLE_AUCTION
	AUCTION_SHOP,
#endif
};

BYTE SlotTypeToInvenType(BYTE bSlotType)
{
	if (bSlotType >= SLOT_TYPE_MAX)
		return RESERVED_WINDOW;
	else
		return c_aSlotTypeToInvenType[bSlotType];
}

BYTE ApplyTypeToPointType(BYTE bApplyType)
{
	if (bApplyType >= CItemData::MAX_APPLY_NUM)
		return POINT_NONE;
	else
		return aApplyInfo[bApplyType].bPointType;
}
