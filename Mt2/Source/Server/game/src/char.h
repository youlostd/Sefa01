#ifndef __INC_METIN_II_CHAR_H__
#define __INC_METIN_II_CHAR_H__

#include <unordered_map>

#include "../../common/stl.h"
#include "entity.h"
#include "FSM.h"
#include "vid.h"
#include "constants.h"
#include "affect.h"
#include "affect_flag.h"
#include "cube.h"
#include "mining.h"
#include "config.h"
#include "utils.h"
#include "char_manager.h"
#include "cmd.h"

#include <google/protobuf/repeated_field.h>
#include "protobuf_data_item.h"
#include "protobuf_data_player.h"

class CBuffOnAttributes;
#ifdef __PET_SYSTEM__
class CPetSystem;
class CPetActor;
#endif
#ifdef __PET_ADVANCED__
class CPetAdvanced;
#endif

#define INSTANT_FLAG_DEATH_PENALTY		(1 << 0)
#define INSTANT_FLAG_SHOP			(1 << 1)
#define INSTANT_FLAG_EXCHANGE			(1 << 2)
#define INSTANT_FLAG_STUN			(1 << 3)
#define INSTANT_FLAG_NO_REWARD			(1 << 4)

#define AI_FLAG_NPC				(1 << 0)
#define AI_FLAG_AGGRESSIVE			(1 << 1)
#define AI_FLAG_HELPER				(1 << 2)
#define AI_FLAG_STAYZONE			(1 << 3)
#define MAX_CARDS_IN_HAND    5
#define MAX_CARDS_IN_FIELD    3

#define SET_OVER_TIME(ch, time)	(ch)->SetOverTime(time)

extern int g_nPortalLimitTime;
extern int g_nPortalGoldLimitTime;

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN = 1,
	YONGSIN_SCROLL = 2,
	MUSIN_SCROLL = 3,
	YAGONG_SCROLL = 4,
	MEMO_SCROLL = 5,
	BDRAGON_SCROLL = 6,
	STONE_LV5_SCROLL = 7,
	YAGONG_SCROLL_KEEPLEVEL = 8,
};

enum
{
	MAIN_RACE_WARRIOR_M,
	MAIN_RACE_ASSASSIN_W,
	MAIN_RACE_SURA_M,
	MAIN_RACE_SHAMAN_W,
	MAIN_RACE_WARRIOR_W,
	MAIN_RACE_ASSASSIN_M,
	MAIN_RACE_SURA_W,
	MAIN_RACE_SHAMAN_M,
#ifdef __WOLFMAN__
	MAIN_RACE_WOLFMAN_M,
#endif
	MAIN_RACE_MAX_NUM,
};

enum
{
	MAIN_RACE_MOB_WARRIOR_M = 12015,
	MAIN_RACE_MOB_ASSASSIN_W,
	MAIN_RACE_MOB_SURA_M,
	MAIN_RACE_MOB_SHAMAN_W,
	MAIN_RACE_MOB_WARRIOR_W,
	MAIN_RACE_MOB_ASSASSIN_M,
	MAIN_RACE_MOB_SURA_W,
	MAIN_RACE_MOB_SHAMAN_M,
#ifdef __WOLFMAN__
	MAIN_RACE_MOB_WOLFMAN_M,
	MAIN_RACE_MOB_MAX_NUM = MAIN_RACE_MOB_WOLFMAN_M,
#else
	MAIN_RACE_MOB_MAX_NUM = MAIN_RACE_MOB_SHAMAN_M,
#endif
};

enum
{
	POISON_LENGTH = 30,
#ifdef __WOLFMAN__
	BLEEDING_LENGTH = 30,
#endif
	STAMINA_PER_STEP = 1,
	SAFEBOX_PAGE_SIZE = 10,
	AI_CHANGE_ATTACK_POISITION_TIME_NEAR = 10000,
	AI_CHANGE_ATTACK_POISITION_TIME_FAR = 1000,
	AI_CHANGE_ATTACK_POISITION_DISTANCE = 100,
	SUMMON_MONSTER_COUNT = 3,
};

enum
{
	FLY_NONE,
	FLY_EXP,
	FLY_HP_MEDIUM,
	FLY_HP_BIG,
	FLY_SP_SMALL,
	FLY_SP_MEDIUM,
	FLY_SP_BIG,
	FLY_FIREWORK1,
	FLY_FIREWORK2,
	FLY_FIREWORK3,
	FLY_FIREWORK4,
	FLY_FIREWORK5,
	FLY_FIREWORK6,
	FLY_FIREWORK_CHRISTMAS,
	FLY_CHAIN_LIGHTNING,
	FLY_HP_SMALL,
	FLY_SKILL_MUYEONG,
};

enum EDamageType
{
	DAMAGE_TYPE_NONE,
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_NORMAL_RANGE,
	//½ºÅ³
	DAMAGE_TYPE_MELEE,
	DAMAGE_TYPE_RANGE,
	DAMAGE_TYPE_FIRE,
	DAMAGE_TYPE_ICE,
	DAMAGE_TYPE_ELEC,
	DAMAGE_TYPE_MAGIC,
	DAMAGE_TYPE_POISON,
	DAMAGE_TYPE_SPECIAL,

#ifdef ENABLE_RUNE_SYSTEM
	DAMAGE_TYPE_RUNE,
#endif

	DAMAGE_TYPE_MAX_NUM,
};

enum EPointTypes
{
	POINT_NONE,				 // 0
	POINT_LEVEL,				// 1
	POINT_VOICE,				// 2
	POINT_EXP,				  // 3
	POINT_NEXT_EXP,			 // 4
	POINT_HP,				   // 5
	POINT_MAX_HP,			   // 6
	POINT_SP,				   // 7
	POINT_MAX_SP,			   // 8  
	POINT_STAMINA,			  // 9  ½ºÅ×¹Ì³Ê
	POINT_MAX_STAMINA,		  // 10 ÃÖ´ë ½ºÅ×¹Ì³Ê

	POINT_GOLD,				 // 11
	POINT_ST,				   // 12 ±Ù·Â
	POINT_HT,				   // 13 Ã¼·Â
	POINT_DX,				   // 14 ¹ÎÃ¸¼º
	POINT_IQ,				   // 15 Á¤½Å·Â
	POINT_DEF_GRADE,		// 16 ...
	POINT_ATT_SPEED,			// 17 °ø°Ý¼Óµµ
	POINT_ATT_GRADE,		// 18 °ø°Ý·Â MAX
	POINT_MOV_SPEED,			// 19 ÀÌµ¿¼Óµµ
	POINT_CLIENT_DEF_GRADE,	// 20 ¹æ¾îµî±Þ
	POINT_CASTING_SPEED,		// 21 ÁÖ¹®¼Óµµ (Äð´Ù¿îÅ¸ÀÓ*100) / (100 + ÀÌ°ª) = ÃÖÁ¾ Äð´Ù¿î Å¸ÀÓ
	POINT_MAGIC_ATT_GRADE,	  // 22 ¸¶¹ý°ø°Ý·Â
	POINT_MAGIC_DEF_GRADE,	  // 23 ¸¶¹ý¹æ¾î·Â
	POINT_EMPIRE_POINT,		 // 24 Á¦±¹Á¡¼ö
	POINT_LEVEL_STEP,		   // 25 ÇÑ ·¹º§¿¡¼­ÀÇ ´Ü°è.. (1 2 3 µÉ ¶§ º¸»ó, 4 µÇ¸é ·¹º§ ¾÷)
	POINT_STAT,				 // 26 ´É·ÂÄ¡ ¿Ã¸± ¼ö ÀÖ´Â °³¼ö
	POINT_SUB_SKILL,		// 27 º¸Á¶ ½ºÅ³ Æ÷ÀÎÆ®
	POINT_SKILL,		// 28 ¾×Æ¼ºê ½ºÅ³ Æ÷ÀÎÆ®
	POINT_WEAPON_MIN,		// 29 ¹«±â ÃÖ¼Ò µ¥¹ÌÁö
	POINT_WEAPON_MAX,		// 30 ¹«±â ÃÖ´ë µ¥¹ÌÁö
	POINT_PLAYTIME,			 // 31 ÇÃ·¹ÀÌ½Ã°£
	POINT_HP_REGEN,			 // 32 HP È¸º¹·ü
	POINT_SP_REGEN,			 // 33 SP È¸º¹·ü

	POINT_BOW_DISTANCE,		 // 34 È° »çÁ¤°Å¸® Áõ°¡Ä¡ (meter)

	POINT_HP_RECOVERY,		  // 35 Ã¼·Â È¸º¹ Áõ°¡·®
	POINT_SP_RECOVERY,		  // 36 Á¤½Å·Â È¸º¹ Áõ°¡·®

	POINT_POISON_PCT,		   // 37 µ¶ È®·ü
	POINT_STUN_PCT,			 // 38 ±âÀý È®·ü
	POINT_SLOW_PCT,			 // 39 ½½·Î¿ì È®·ü
	POINT_CRITICAL_PCT,		 // 40 Å©¸®Æ¼ÄÃ È®·ü
	POINT_PENETRATE_PCT,		// 41 °üÅëÅ¸°Ý È®·ü
	POINT_CURSE_PCT,			// 42 ÀúÁÖ È®·ü

	POINT_ATTBONUS_HUMAN,	   // 43 ÀÎ°£¿¡°Ô °­ÇÔ
	POINT_ATTBONUS_ANIMAL,	  // 44 µ¿¹°¿¡°Ô µ¥¹ÌÁö % Áõ°¡
	POINT_ATTBONUS_ORC,		 // 45 ¿õ±Í¿¡°Ô µ¥¹ÌÁö % Áõ°¡
	POINT_ATTBONUS_MILGYO,	  // 46 ¹Ð±³¿¡°Ô µ¥¹ÌÁö % Áõ°¡
	POINT_ATTBONUS_UNDEAD,	  // 47 ½ÃÃ¼¿¡°Ô µ¥¹ÌÁö % Áõ°¡
	POINT_ATTBONUS_DEVIL,	   // 48 ¸¶±Í(¾Ç¸¶)¿¡°Ô µ¥¹ÌÁö % Áõ°¡
	POINT_ATTBONUS_INSECT,	  // 49 ¹ú·¹Á·
	POINT_ATTBONUS_FIRE,		// 50 È­¿°Á·
	POINT_ATTBONUS_ICE,		 // 51 ºù¼³Á·
	POINT_ATTBONUS_DESERT,	  // 52 »ç¸·Á·
	POINT_ATTBONUS_MONSTER,	 // 53 ¸ðµç ¸ó½ºÅÍ¿¡°Ô °­ÇÔ
	POINT_ATTBONUS_WARRIOR,	 // 54 ¹«»ç¿¡°Ô °­ÇÔ
	POINT_ATTBONUS_ASSASSIN,	// 55 ÀÚ°´¿¡°Ô °­ÇÔ
	POINT_ATTBONUS_SURA,		// 56 ¼ö¶ó¿¡°Ô °­ÇÔ
	POINT_ATTBONUS_SHAMAN,		// 57 ¹«´ç¿¡°Ô °­ÇÔ
	POINT_ATTBONUS_TREE,	 	// 58 ³ª¹«¿¡°Ô °­ÇÔ 20050729.myevan UNUSED5 

	POINT_RESIST_WARRIOR,		// 59 ¹«»ç¿¡°Ô ÀúÇ×
	POINT_RESIST_ASSASSIN,		// 60 ÀÚ°´¿¡°Ô ÀúÇ×
	POINT_RESIST_SURA,			// 61 ¼ö¶ó¿¡°Ô ÀúÇ×
	POINT_RESIST_SHAMAN,		// 62 ¹«´ç¿¡°Ô ÀúÇ×

	POINT_STEAL_HP,			 // 63 »ý¸í·Â Èí¼ö
	POINT_STEAL_SP,			 // 64 Á¤½Å·Â Èí¼ö

	POINT_MANA_BURN_PCT,		// 65 ¸¶³ª ¹ø

	/// ÇÇÇØ½Ã º¸³Ê½º ///

	POINT_DAMAGE_SP_RECOVER,	// 66 °ø°Ý´çÇÒ ½Ã Á¤½Å·Â È¸º¹ È®·ü

	POINT_BLOCK,				// 67 ºí·°À²
	POINT_DODGE,				// 68 È¸ÇÇÀ²

	POINT_RESIST_SWORD,		 // 69
	POINT_RESIST_TWOHAND,	   // 70
	POINT_RESIST_DAGGER,		// 71
	POINT_RESIST_BELL,		  // 72
	POINT_RESIST_FAN,		   // 73
	POINT_RESIST_BOW,		   // 74  È­»ì   ÀúÇ×   : ´ë¹ÌÁö °¨¼Ò
	POINT_RESIST_FIRE,		  // 75  È­¿°   ÀúÇ×   : È­¿°°ø°Ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò
	POINT_RESIST_ELEC,		  // 76  Àü±â   ÀúÇ×   : Àü±â°ø°Ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò
	POINT_RESIST_MAGIC,		 // 77  ¼ú¹ý   ÀúÇ×   : ¸ðµç¼ú¹ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò
	POINT_RESIST_WIND,		  // 78  ¹Ù¶÷   ÀúÇ×   : ¹Ù¶÷°ø°Ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò

	POINT_REFLECT_MELEE,		// 79 °ø°Ý ¹Ý»ç

	/// Æ¯¼ö ÇÇÇØ½Ã ///
	POINT_REFLECT_CURSE,		// 80 ÀúÁÖ ¹Ý»ç
	POINT_POISON_REDUCE,		// 81 µ¶µ¥¹ÌÁö °¨¼Ò

	/// Àû ¼Ò¸ê½Ã ///
	POINT_KILL_SP_RECOVER,		// 82 Àû ¼Ò¸ê½Ã MP È¸º¹
	POINT_EXP_DOUBLE_BONUS,		// 83
	POINT_GOLD_DOUBLE_BONUS,		// 84
	POINT_ITEM_DROP_BONUS,		// 85

	/// È¸º¹ °ü·Ã ///
	POINT_POTION_BONUS,			// 86
	POINT_KILL_HP_RECOVERY,		// 87

	POINT_IMMUNE_STUN,			// 88
	POINT_IMMUNE_SLOW,			// 89
	POINT_IMMUNE_FALL,			// 90
	//////////////////

	POINT_PARTY_ATTACKER_BONUS,		// 91
	POINT_PARTY_TANKER_BONUS,		// 92

	POINT_ATT_BONUS,			// 93
	POINT_DEF_BONUS,			// 94

	POINT_ATT_GRADE_BONUS,		// 95
	POINT_DEF_GRADE_BONUS,		// 96
	POINT_MAGIC_ATT_GRADE_BONUS,	// 97
	POINT_MAGIC_DEF_GRADE_BONUS,	// 98

	POINT_RESIST_NORMAL_DAMAGE,		// 99

	POINT_HIT_HP_RECOVERY,		// 100
	POINT_HIT_SP_RECOVERY, 		// 101
	POINT_MANASHIELD,			// 102 Èæ½Å¼öÈ£ ½ºÅ³¿¡ ÀÇÇÑ ¸¶³ª½¯µå È¿°ú Á¤µµ

	POINT_PARTY_BUFFER_BONUS,		// 103
	POINT_PARTY_SKILL_MASTER_BONUS,	// 104

	POINT_HP_RECOVER_CONTINUE,		// 105
	POINT_SP_RECOVER_CONTINUE,		// 106

	POINT_STEAL_GOLD,			// 107 
	POINT_POLYMORPH,			// 108 º¯½ÅÇÑ ¸ó½ºÅÍ ¹øÈ£
	POINT_MOUNT,			// 109 Å¸°íÀÖ´Â ¸ó½ºÅÍ ¹øÈ£

	POINT_PARTY_HASTE_BONUS,		// 110
	POINT_PARTY_DEFENDER_BONUS,		// 111
	POINT_STAT_RESET_COUNT,		// 112 ÇÇÀÇ ´Ü¾à »ç¿ëÀ» ÅëÇÑ ½ºÅÝ ¸®¼Â Æ÷ÀÎÆ® (1´ç 1Æ÷ÀÎÆ® ¸®¼Â°¡´É)

	POINT_HORSE_SKILL,			// 113

	POINT_MALL_ATTBONUS,		// 114 °ø°Ý·Â +x%
	POINT_MALL_DEFBONUS,		// 115 ¹æ¾î·Â +x%
	POINT_MALL_EXPBONUS,		// 116 °æÇèÄ¡ +x%
	POINT_MALL_ITEMBONUS,		// 117 ¾ÆÀÌÅÛ µå·ÓÀ² x/10¹è
	POINT_MALL_GOLDBONUS,		// 118 µ· µå·ÓÀ² x/10¹è

	POINT_MAX_HP_PCT,			// 119 ÃÖ´ë»ý¸í·Â +x%
	POINT_MAX_SP_PCT,			// 120 ÃÖ´ëÁ¤½Å·Â +x%

	POINT_SKILL_DAMAGE_BONUS,		// 121 ½ºÅ³ µ¥¹ÌÁö *(100+x)%
	POINT_NORMAL_HIT_DAMAGE_BONUS,	// 122 ÆòÅ¸ µ¥¹ÌÁö *(100+x)%

	// DEFEND_BONUS_ATTRIBUTES
	POINT_SKILL_DEFEND_BONUS,		// 123 ½ºÅ³ ¹æ¾î µ¥¹ÌÁö
	POINT_NORMAL_HIT_DEFEND_BONUS,	// 124 ÆòÅ¸ ¹æ¾î µ¥¹ÌÁö
	// END_OF_DEFEND_BONUS_ATTRIBUTES

	POINT_RAMADAN_CANDY_BONUS_EXP,			// ¶ó¸¶´Ü »çÅÁ °æÇèÄ¡ Áõ°¡¿ë

	POINT_ENERGY = 128,					// 128 ±â·Â

	// ±â·Â ui ¿ë.
	// ¼­¹ö¿¡¼­ ¾²Áö ¾Ê±â¸¸, Å¬¶óÀÌ¾ðÆ®¿¡¼­ ±â·ÂÀÇ ³¡ ½Ã°£À» POINT·Î °ü¸®ÇÏ±â ¶§¹®¿¡ ÀÌ·¸°Ô ÇÑ´Ù.
	// ¾Æ ºÎ²ô·´´Ù
	POINT_ENERGY_END_TIME = 129,					// 129 ±â·Â Á¾·á ½Ã°£

	POINT_COSTUME_ATTR_BONUS = 130,
	POINT_MAGIC_ATT_BONUS_PER = 131,
	POINT_MELEE_MAGIC_ATT_BONUS_PER = 132,

	// Ãß°¡ ¼Ó¼º ÀúÇ×
	POINT_RESIST_ICE = 133,		  //   ³Ã±â ÀúÇ×   : ¾óÀ½°ø°Ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò
	POINT_RESIST_EARTH = 134,		//   ´ëÁö ÀúÇ×   : ¾óÀ½°ø°Ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò
	POINT_RESIST_DARK = 135,		 //   ¾îµÒ ÀúÇ×   : ¾óÀ½°ø°Ý¿¡ ´ëÇÑ ´ë¹ÌÁö °¨¼Ò

	POINT_RESIST_CRITICAL = 136,		// Å©¸®Æ¼ÄÃ ÀúÇ×	: »ó´ëÀÇ Å©¸®Æ¼ÄÃ È®·üÀ» °¨¼Ò
	POINT_RESIST_PENETRATE = 137,		// °üÅëÅ¸°Ý ÀúÇ×	: »ó´ëÀÇ °üÅëÅ¸°Ý È®·üÀ» °¨¼Ò

	POINT_EXP_REAL_BONUS = 138,		// exp real bonus 5 = 5 % 

#ifdef __ACCE_COSTUME__
	POINT_ACCEDRAIN_RATE = 144,
#endif

#ifdef __ANIMAL_SYSTEM__
#ifdef __PET_SYSTEM__
	POINT_PET_EXP_BONUS = 145,
#endif
#endif

	POINT_RESIST_MONSTER = 146,
	POINT_ATTBONUS_METIN = 147,
	POINT_ATTBONUS_BOSS = 148,

	POINT_ANTI_EXP = 149,

	POINT_RESIST_HUMAN = 150,

#ifdef __ANIMAL_SYSTEM__
	POINT_MOUNT_EXP_BONUS = 151,
#endif
	
	POINT_MOUNT_BUFF_BONUS = 152,

// #ifdef __GAYA_SYSTEM__
	POINT_GAYA = 153,
// #endif
	POINT_RESIST_SWORD_PEN,
	POINT_RESIST_TWOHAND_PEN,
	POINT_RESIST_DAGGER_PEN,
	POINT_RESIST_BELL_PEN,
	POINT_RESIST_FAN_PEN,
	POINT_RESIST_BOW_PEN,
	POINT_RESIST_ATTBONUS_HUMAN,
// #ifdef __ELEMENT_SYSTEM__
	POINT_ATTBONUS_ELEC,
	POINT_ATTBONUS_WIND,
	POINT_ATTBONUS_EARTH,
	POINT_ATTBONUS_DARK,
// #endif

	POINT_ANTI_RESIST_MAGIC,

	POINT_BLOCK_IGNORE_BONUS,

	POINT_EMPIRE_A_KILLED,
	POINT_EMPIRE_B_KILLED,
	POINT_EMPIRE_C_KILLED,
	POINT_DUELS_WON,
	POINT_DUELS_LOST,
	POINT_MONSTERS_KILLED,
	POINT_BOSSES_KILLED,
	POINT_STONES_DESTROYED,
// #ifdef ENABLE_RUNE_SYSTEM
	POINT_RUNE_SHIELD_PER_HIT,
	POINT_RUNE_HEAL_ON_KILL,
	POINT_RUNE_BONUS_DAMAGE_AFTER_HIT,
	POINT_RUNE_3RD_ATTACK_BONUS,
	POINT_RUNE_FIRST_NORMAL_HIT_BONUS,
	POINT_RUNE_MSHIELD_PER_SKILL,
	POINT_RUNE_HARVEST,
	POINT_RUNE_DAMAGE_AFTER_3,
	POINT_RUNE_OUT_OF_COMBAT_SPEED,
	POINT_RUNE_RESET_SKILL,
	POINT_RUNE_COMBAT_CASTING_SPEED,
	POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT,
	POINT_RUNE_MOVSPEED_AFTER_3,
	POINT_RUNE_SLOW_ON_ATTACK,
	POINT_RUNE_ATTACK_SLOW_PCT,
	POINT_RUNE_MOVEMENT_SLOW_PCT,
// #endif
	POINT_HEAL_EFFECT_BONUS,
	POINT_CRITICAL_DAMAGE_BONUS,
	POINT_DOUBLE_ITEM_DROP_BONUS,
	POINT_DAMAGE_BY_SP_BONUS,
	POINT_AURA_HEAL_EFFECT_BONUS,
	POINT_AURA_EQUIP_SKILL_BONUS,
	POINT_SINGLETARGET_SKILL_DAMAGE_BONUS,
	POINT_MULTITARGET_SKILL_DAMAGE_BONUS,
	POINT_MIXED_DEFEND_BONUS,
	POINT_EQUIP_SKILL_BONUS,

// #ifdef ENABLE_RUNE_SYSTEM
	POINT_RUNE_LEADERSHIP_BONUS,
	POINT_RUNE_MOUNT_PARALYZE,
// #ifdef RUNE_CRITICAL_POINT
	POINT_RUNE_CRITICAL_PVM,
// #endif
// #endif
// #ifdef STANDARD_SKILL_DURATION
	POINT_SKILL_DURATION,
// #endif

	POINT_ATTBONUS_ALL_ELEMENTS = 205,
// #ifdef ENABLE_ZODIAC_TEMPLE
	POINT_ANIMASPHERE = 206,
	POINT_ATTBONUS_ZODIAC,
// #endif
	POINT_BONUS_UPGRADE_CHANCE,
	POINT_LOWER_DUNGEON_CD,
	POINT_LOWER_BIOLOG_CD,

#ifdef __WOLFMAN__
		POINT_BLEEDING_PCT,
		POINT_BLEEDING_REDUCE,
		POINT_ATTBONUS_WOLFMAN,
		POINT_RESIST_WOLFMAN,
		POINT_RESIST_CLAW,
#endif

	POINT_RESIST_MAGIC_REDUCTION,
	POINT_RESIST_BOSS,

	POINT_ATTBONUS_MONSTER_DIV10,
};

enum EPKModes
{
	PK_MODE_PEACE,
	PK_MODE_REVENGE,
	PK_MODE_FREE,
	PK_MODE_PROTECT,
	PK_MODE_GUILD,
	PK_MODE_MAX_NUM
};

enum EPositions
{
	POS_DEAD,
	POS_SLEEPING,
	POS_RESTING,
	POS_SITTING,
	POS_FISHING,
	POS_FIGHTING,
	POS_MOUNTING,
	POS_STANDING
};

enum EBlockAction
{
	BLOCK_EXCHANGE		= (1 << 0),
	BLOCK_PARTY_INVITE		= (1 << 1),
	BLOCK_GUILD_INVITE		= (1 << 2),
	BLOCK_WHISPER		= (1 << 3),
	BLOCK_MESSENGER_INVITE	= (1 << 4),
	BLOCK_PARTY_REQUEST		= (1 << 5),
};

enum EHideCostumes
{
	HIDE_COSTUME_WEAPON,
	HIDE_COSTUME_ARMOR,
	HIDE_COSTUME_HAIR,
	HIDE_COSTUME_ACCE,
	HIDE_COSTUME_MAX
};

// <Factor> Dynamically evaluated CHARACTER* equivalent.
// Referring to SCharDeadEventInfo.
struct DynamicCharacterPtr {
	DynamicCharacterPtr() : is_pc(false), id(0) {}
	DynamicCharacterPtr(const DynamicCharacterPtr& o)
		: is_pc(o.is_pc), id(o.id) {}

	// Returns the LPCHARACTER found in CHARACTER_MANAGER.
	LPCHARACTER Get() const; 
	// Clears the current settings.
	void Reset() {
		is_pc = false;
		id = 0;
	}

	// Basic assignment operator.
	DynamicCharacterPtr& operator=(const DynamicCharacterPtr& rhs) {
		is_pc = rhs.is_pc;
		id = rhs.id;
		return *this;
	}
	// Supports assignment with LPCHARACTER type.
	DynamicCharacterPtr& operator=(LPCHARACTER character);
	// Supports type casting to LPCHARACTER.
	operator LPCHARACTER() const {
		return Get();
	}

	bool is_pc;
	uint32_t id;
};

/* ÀúÀåÇÏ´Â µ¥ÀÌÅÍ */
typedef struct character_point
{
	long long		points[POINT_MAX_NUM];

	BYTE			job;
	BYTE			voice;

	BYTE			level;
#ifdef __PRESTIGE__
	BYTE			prestige_level;
#endif
	DWORD			exp;
	long long		gold;
#ifdef ENABLE_ZODIAC_TEMPLE
	BYTE			animasphere;
#endif
	int				hp;
	int				sp;

	int				stamina;

	BYTE			skill_group;
} CHARACTER_POINT;

/* ÀúÀåµÇÁö ¾Ê´Â Ä³¸¯ÅÍ µ¥ÀÌÅÍ */
typedef struct character_point_instant
{
	LONGLONG		points[POINT_MAX_NUM];

	float			fRot;

	int				iMaxHP;
	int				iMaxSP;

	long			position;

	long			instant_flag;
	DWORD			dwAIFlag;
	DWORD			dwImmuneFlag;
	DWORD			dwLastShoutPulse;

	DWORD			parts[PART_MAX_NUM];

	LPITEM			pItems[INVENTORY_AND_EQUIP_SLOT_MAX];
	WORD			wItemGrid[INVENTORY_AND_EQUIP_SLOT_MAX];
#ifdef __DRAGONSOUL__
	LPITEM			pDSItems[DRAGON_SOUL_INVENTORY_MAX_NUM];
	WORD			wDSItemGrid[DRAGON_SOUL_INVENTORY_MAX_NUM];
#endif

#ifdef __ACCE_COSTUME__
	WORD			pAcceSlots[ACCE_SLOT_MAX_NUM];
#endif

	// by mhh
	DWORD			pCubeItemIDs[CUBE_MAX_NUM];
	LPCHARACTER		pCubeNpc;

	LPCHARACTER			battle_victim;

	BYTE			gm_level;

	BYTE			bBasePart;	// Æò»óº¹ ¹øÈ£

	int				iMaxStamina;

	BYTE			bBlockMode;

#ifdef __DRAGONSOUL__
	int				iDragonSoulActiveDeck;
	LPENTITY		m_pDragonSoulRefineWindowOpener;
#endif
#ifdef __COSTUME_BONUS_TRANSFER__
	LPENTITY		m_pCBTWindowOpener;
#endif
} CHARACTER_POINT_INSTANT;

#define TRIGGERPARAM		LPCHARACTER ch, LPCHARACTER causer

typedef struct trigger
{
	BYTE	type;
	int		(*func) (TRIGGERPARAM);
	long	value;
} TRIGGER;

class CTrigger
{
	public:
		CTrigger() : bType(0), pFunc(NULL)
		{
		}

		BYTE	bType;
		int	(*pFunc) (TRIGGERPARAM);
};

EVENTINFO(char_event_info)
{
	DynamicCharacterPtr ch;
};

struct TSkillUseInfo
{
	int		iHitCount;
	int		iMaxHitCount;
	int		iSplashCount;
	DWORD   dwNextSkillUsableTime;
	int		iRange;
	bool	bUsed;
	DWORD   dwVID;
	bool	isGrandMaster;

	std::unordered_map<VID, size_t> TargetVIDMap;

	TSkillUseInfo()
		: iHitCount(0), iMaxHitCount(0), iSplashCount(0), dwNextSkillUsableTime(0), iRange(0), bUsed(false),
		dwVID(0), isGrandMaster(false)
   	{}

	bool	HitOnce(DWORD dwVnum = 0);

	bool	IsCooltimeOver() const;
	bool	UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount = 1, int hitcount = -1, int range = -1);
	DWORD   GetMainTargetVID() const	{ return dwVID; }
	void	SetMainTargetVID(DWORD vid) { dwVID=vid; }
	void	ResetHitCount() { if (iSplashCount) { iHitCount = iMaxHitCount; iSplashCount--; } }
};

typedef struct packet_party_update TPacketGCPartyUpdate;
class CExchange;
class CSkillProto;
class CParty;
class CDungeon;
class CWarMap;
class CAffect;
class CGuild;
class CSafebox;
class CArena;
class CMountSystem;

class CShop;
typedef class CShop * LPSHOP;

class CMob;
class CMobInstance;
typedef struct SMobSkillInfo TMobSkillInfo;

//SKILL_POWER_BY_LEVEL
extern int GetSkillPowerByLevelFromType(int job, int skillgroup, int skilllevel);
//END_SKILL_POWER_BY_LEVEL

namespace marriage
{
	class WeddingMap;
}
enum e_overtime
{
	OT_NONE,
	OT_3HOUR,
	OT_5HOUR,
};

class CHARACTER : public CEntity, public CFSM
{
	protected:
		//////////////////////////////////////////////////////////////////////////////////
		// Entity °ü·Ã
		virtual void	EncodeInsertPacket(LPENTITY entity);
		virtual void	EncodeRemovePacket(LPENTITY entity);
		template <typename T>
		void			PacketView(network::GCOutputPacket<T>& packet, LPENTITY except = NULL)
		{
			if (IsGMInvisible() || (!IsPC() && GetRider() && GetRider()->IsGMInvisible()))
			{
				if (except != this && GetDesc())
					GetDesc()->Packet(packet);
				else if (!IsPC() && GetRider() && except != GetRider() && GetRider()->GetDesc()) {
					GetRider()->GetDesc()->Packet(packet);
				}
				return;
			}

			CEntity::PacketView(packet, except);
		}
		//////////////////////////////////////////////////////////////////////////////////

	public:
		LPCHARACTER			FindCharacterInView(const char * name, bool bFindPCOnly);
		void				UpdatePacket();

		//////////////////////////////////////////////////////////////////////////////////
		// FSM (Finite State Machine) °ü·Ã
	protected:
		CStateTemplate<CHARACTER>	m_stateMove;
		CStateTemplate<CHARACTER>	m_stateBattle;
		CStateTemplate<CHARACTER>	m_stateIdle;

	public:
		virtual void		StateMove();
		virtual void		StateBattle();
		virtual void		StateIdle();
		virtual void		StateFlag();
		virtual void		StateFlagBase();
		void				StateHorse();

	protected:
		// STATE_IDLE_REFACTORING
		void				__StateIdle_Monster();
		void				__StateIdle_Stone();
		void				__StateIdle_NPC();
		// END_OF_STATE_IDLE_REFACTORING

	public:
		DWORD GetAIFlag() const	{ return m_pointsInstant.dwAIFlag; }
	
		void				SetAggressive();
		void				ToggleAggressive() { TOGGLE_BIT(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE); }
		bool				IsAggressive() const;

		void				SetCoward();
		bool				IsCoward() const;
		void				CowardEscape();

		void				SetNoAttackShinsu();
		bool				IsNoAttackShinsu() const;

		void				SetNoAttackChunjo();
		bool				IsNoAttackChunjo() const;

		void				SetNoAttackJinno();
		bool				IsNoAttackJinno() const;

		void				SetAttackMob();
		bool				IsAttackMob() const;

		void				SetNoMove();

		bool				CanNPCFollowTarget(LPCHARACTER pkTarget);

		virtual void			BeginStateEmpty();
		virtual void			EndStateEmpty() {}

		void				RestartAtSamePos();
		void				RestartAtPos(long lX, long lY);

	protected:
		DWORD				m_dwStateDuration;
		//////////////////////////////////////////////////////////////////////////////////

	public:
		CHARACTER();
		virtual ~CHARACTER();

		void			Create(const char * c_pszName, DWORD vid, bool isPC);
		void			Destroy();

		void			Disconnect(const char * c_pszReason);

	protected:
		void			Initialize();

		//////////////////////////////////////////////////////////////////////////////////
		// Basic Points
	public:
		DWORD			GetPlayerID() const	{ return m_dwPlayerID; }

		void			SetPlayerProto(const TPlayerTable * table);
		void			CreatePlayerProto(TPlayerTable & tab);	// ÀúÀå ½Ã »ç¿ë

		void			SetProto(const CMob * c_pkMob);
		WORD			GetRaceNum() const;

		void			Save();		// DelayedSave
		void			SaveReal();	// ½ÇÁ¦ ÀúÀå
		void			FlushDelayedSaveItem();

		const char *	GetName(BYTE bLanguageID = LANGUAGE_DEFAULT) const;
		const VID &		GetVID() const		{ return m_vid;		}

		void			SetName(const std::string& name) { m_stName = name; }

		void			SetRace(BYTE race);
		bool			ChangeSex();

		DWORD			GetAID() const;
		int				ChangeEmpire(BYTE empire);

		BYTE			GetJob() const;
		BYTE			GetCharType() const;

		bool			IsPC() const		{ return GetDesc() ? true : false; }
		bool			IsNPC()	const		{ return m_bCharType != CHAR_TYPE_PC; }
		bool			IsMonster()	const	{ return m_bCharType == CHAR_TYPE_MONSTER; }
		bool			IsStone() const		{ return m_bCharType == CHAR_TYPE_STONE; }
		bool			IsDoor() const		{ return m_bCharType == CHAR_TYPE_DOOR; } 
		bool			IsBuilding() const	{ return m_bCharType == CHAR_TYPE_BUILDING;  }
		bool			IsWarp() const		{ return m_bCharType == CHAR_TYPE_WARP; }
		bool			IsGoto() const		{ return m_bCharType == CHAR_TYPE_GOTO; }
		bool			IsMount() const		{ return m_bCharType == CHAR_TYPE_MOUNT; }

		DWORD			GetLastShoutPulse() const	{ return m_pointsInstant.dwLastShoutPulse; }
		void			SetLastShoutPulse(DWORD pulse) { m_pointsInstant.dwLastShoutPulse = pulse; }
#ifdef __FAKE_PC__
		int				GetLevel() const		{ return FakePC_Check() ? FakePC_GetOwner()->GetLevel() : m_points.level; }
#else
		int				GetLevel() const		{ return m_points.level;	}
#endif
		void			SetLevel(BYTE level);
#ifdef __PRESTIGE__
#ifdef __FAKE_PC__
		int				GetPrestigeLevel() const	{ return FakePC_Check() ? FakePC_GetOwner()->GetPrestigeLevel() : MIN(m_points.prestige_level, gPrestigeMaxLevel - 1); }
#else
		int				GetPrestigeLevel() const	{ return MIN(m_points.prestige_level, gPrestigeMaxLevel - 1); }
#endif
		void			SetPrestigeLevel(BYTE bLevel)	{ m_points.prestige_level = bLevel; }
#endif

		BYTE			GetGMLevel(bool bIgnoreTestServer = false) const;
		BOOL 			IsGM() const;
		void			SetGMLevel(); 

		bool			IsGMInvisible() const			{ return m_bGMInvisible; }
		bool			IsGMInvisibleChanged() const	{ return m_bGMInvisibleChanged; }
		void			SetGMInvisible(bool bActive, bool bTemporary = false);
		void			ResetGMInvisibleChanged()		{ m_bGMInvisibleChanged = false; }

		DWORD			GetExp() const		{ return m_points.exp;	}
		void			SetExp(DWORD exp)	{ m_points.exp = exp;	}
		DWORD			GetNextExp() const;
		LPCHARACTER		DistributeExp();	// Á¦ÀÏ ¸¹ÀÌ ¶§¸° »ç¶÷À» ¸®ÅÏÇÑ´Ù.
		void			DistributeHP(LPCHARACTER pkKiller);
		void			DistributeSP(LPCHARACTER pkKiller, int iMethod=0);

		void			SetPosition(int pos);
		bool			IsPosition(int pos) const	{ return m_pointsInstant.position == pos ? true : false; }
		int				GetPosition() const		{ return m_pointsInstant.position; }

		void			SetPart(BYTE bPartPos, DWORD dwVal);
		DWORD			GetPart(BYTE bPartPos) const;
		DWORD			GetOriginalPart(BYTE bPartPos) const;

		void			SetHP(int hp)		{ m_points.hp = hp; }
		int				GetHP() const		{ return m_points.hp; }

		void			SetSP(int sp)		{ m_points.sp = sp; }
		int				GetSP() const		{ return m_points.sp; }

		void			SetStamina(int stamina)	{ m_points.stamina = stamina; }
		int				GetStamina() const		{ return m_points.stamina; }

		void			SetMaxHP(int iVal)	{ m_pointsInstant.iMaxHP = iVal; }
		int				GetMaxHP() const	{ return m_pointsInstant.iMaxHP; }

		void			SetMaxSP(int iVal)	{ m_pointsInstant.iMaxSP = iVal; }
		int				GetMaxSP() const	{ return m_pointsInstant.iMaxSP; }

		void			SetMaxStamina(int iVal)	{ m_pointsInstant.iMaxStamina = iVal; }
		int				GetMaxStamina() const	{ return m_pointsInstant.iMaxStamina; }

		int				GetHPPct() const;

		void			SetRealPoint(BYTE idx, LONGLONG val);
		LONGLONG		GetRealPoint(BYTE idx) const;

		void			SetPoint(BYTE idx, LONGLONG val);
		int				GetPoint(unsigned char idx, bool bGetSum = true) const;
		
		void			SetPointF(unsigned char idx, float val);
		float			GetPointF(unsigned char idx, bool bGetSum = true) const;
		
		LONGLONG		GetLimitPoint(BYTE idx) const;
		LONGLONG		GetPolymorphPoint(BYTE idx) const;

		const network::TMobTable &	GetMobTable() const;
		BYTE				GetMobRank() const;
		BYTE				GetMobBattleType() const;
		BYTE				GetMobSize() const;
		DWORD				GetMobDamageMin() const;
		DWORD				GetMobDamageMax() const;
		WORD				GetMobAttackRange() const;
		DWORD				GetMobDropItemVnum() const;
		float				GetMobDamageMultiply() const;

		// NEWAI
		bool			IsBerserker() const;
		bool			IsBerserk() const;
		void			SetBerserk(bool mode);

		bool			IsStoneSkinner() const;

		bool			IsGodSpeeder() const;
		bool			IsGodSpeed() const;
		void			SetGodSpeed(bool mode);

		bool			IsDeathBlower() const;
		bool			IsDeathBlow() const;

		bool			IsReviver() const;
		bool			HasReviverInParty() const;
		bool			IsRevive() const;
		void			SetRevive(bool mode);
		// NEWAI END

		bool			IsRaceFlag(DWORD dwBit) const;
		bool			IsSummonMonster() const;
		DWORD			GetSummonVnum() const;

		DWORD			GetPolymorphItemVnum() const;
		DWORD			GetMonsterDrainSPPoint() const;

		void			MainCharacterPacket();	// ³»°¡ ¸ÞÀÎÄ³¸¯ÅÍ¶ó°í º¸³»ÁØ´Ù.

		void			ComputePoints();
		void			ComputeBattlePoints();
		void			ComputePassiveSkillPoints(bool bAdd = true);
		void			PointChange(BYTE type, long long amount, bool bAmount = false, bool bBroadcast = false);
		void			PointsPacket();
		void			ApplyPoint(BYTE bApplyType, int iVal);
		void			ApplyPointF(unsigned char bApplyType, float fVal);
		
		void			CheckMaximumPoints();	// HP, SP µîÀÇ ÇöÀç °ªÀÌ ÃÖ´ë°ª º¸´Ù ³ôÀºÁö °Ë»çÇÏ°í ³ô´Ù¸é ³·Ãá´Ù.

		bool			Show(long lMapIndex, long x, long y, long z = LONG_MAX, bool bShowSpawnMotion = false);

		void			SetRotation(float fRot);
		void			SetRotationToXY(long x, long y);
		float			GetRotation() const	{ return m_pointsInstant.fRot; }

		void			MotionPacketEncode(BYTE motion, LPCHARACTER victim, network::GCOutputPacket<network::GCMotionPacket>& packet);
		void			Motion(BYTE motion, LPCHARACTER victim = NULL);

		void			ChatPacket(BYTE type, const char *format, ...);
		void			tchat(const char *format, ...);
		void			MonsterChat(BYTE bMonsterChatType);

		void			ResetPoint(int iLv);

		void			SetBlockMode(BYTE bFlag);
		void			SetBlockModeForce(BYTE bFlag);
		bool			IsBlockMode(BYTE bFlag) const	{ return (m_pointsInstant.bBlockMode & bFlag)?true:false; }

		bool			IsPolymorphed() const		{ return m_dwPolymorphRace>0; }
		bool			IsPolyMaintainStat() const	{ return m_bPolyMaintainStat; } // ÀÌÀü ½ºÅÝÀ» À¯ÁöÇÏ´Â Æú¸®¸ðÇÁ.
		void			SetPolymorph(DWORD dwRaceNum, bool bMaintainStat = false);
		DWORD			GetPolymorphVnum() const	{ return m_dwPolymorphRace; }
		int				GetPolymorphPower() const;

		// FISING	
		void			fishing();
		void			fishing_take();
		// END_OF_FISHING

		// MINING
		void			mining(LPCHARACTER chLoad);
		void			mining_cancel();
		void			mining_take();
		// END_OF_MINING

		void			ResetPlayTime(DWORD dwTimeRemain = 0);

		void			CreateFly(BYTE bType, LPCHARACTER pkVictim);

		void			ResetChatCounter();
		BYTE			IncreaseChatCounter();
		BYTE			GetChatCounter() const;

		void		SetSkipSave(bool b)	{ m_bSkipSave = b; }
		bool		GetSkipSave()		{ return m_bSkipSave; }
		
	protected:
		DWORD			m_dwPolymorphRace;
		bool			m_bPolyMaintainStat;
		DWORD			m_dwLoginPlayTime;
		DWORD			m_dwPlayerID;
		VID				m_vid;
		std::string		m_stName;
		BYTE			m_bCharType;

		CHARACTER_POINT		m_points;
		CHARACTER_POINT_INSTANT	m_pointsInstant;
		float					m_pointsInstantF[POINT_MAX_NUM];

		int				m_iMoveCount;
		DWORD			m_dwPlayStartTime;
		BYTE			m_bAddChrState;
		bool			m_bSkipSave;
		BYTE			m_bChatCounter;

		bool			m_bGMInvisible;
		bool			m_bGMInvisibleChanged;

		// End of Basic Points

		//////////////////////////////////////////////////////////////////////////////////
		// Move & Synchronize Positions
		//////////////////////////////////////////////////////////////////////////////////
	public:
		bool			IsStateMove() const			{ return IsState((CState&)m_stateMove); }
		bool			IsStateIdle() const			{ return IsState((CState&)m_stateIdle); }
		bool			IsWalking() const			{ return m_bNowWalking || GetStamina()<=0; }
		void			SetWalking(bool bWalkFlag)	{ m_bWalking=bWalkFlag; }
		void			SetNowWalking(bool bWalkFlag);	
		void			ResetWalking()			{ SetNowWalking(m_bWalking); }

		void			SetMovingWay(const TNPCMovingPosition* pWay, int iMaxNum, bool bRepeat = false, bool bLocal = false);
		bool			DoMovingWay();
		bool			Goto(long x, long y);	// ¹Ù·Î ÀÌµ¿ ½ÃÅ°Áö ¾Ê°í ¸ñÇ¥ À§Ä¡·Î BLENDING ½ÃÅ²´Ù.
		void			Stop();

		bool			CanMove() const;		// ÀÌµ¿ÇÒ ¼ö ÀÖ´Â°¡?

		void			SyncPacket();
		bool			Sync(long x, long y);	// ½ÇÁ¦ ÀÌ ¸Þ¼Òµå·Î ÀÌµ¿ ÇÑ´Ù (°¢ Á¾ Á¶°Ç¿¡ ÀÇÇÑ ÀÌµ¿ ºÒ°¡°¡ ¾øÀ½)
		bool			Move(long x, long y);	// Á¶°ÇÀ» °Ë»çÇÏ°í Sync ¸Þ¼Òµå¸¦ ÅëÇØ ÀÌµ¿ ÇÑ´Ù.
		void			OnMove(bool bIsAttack = false, bool bPVP = false);	// ¿òÁ÷ÀÏ¶§ ºÒ¸°´Ù. Move() ¸Þ¼Òµå ÀÌ¿Ü¿¡¼­µµ ºÒ¸± ¼ö ÀÖ´Ù.
		DWORD			GetMotionMode() const;
		DWORD			GetMotionModeBySubType(BYTE bSubType) const;
		float			GetMoveMotionSpeed() const;
		float			GetMoveSpeed() const;
		void			CalculateMoveDuration();
		void			SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime=0, int iRot=-1);
		DWORD			GetCurrentMoveDuration() const	{ return m_dwMoveDuration; }
		DWORD			GetWalkStartTime() const	{ return m_dwWalkStartTime; }
		DWORD			GetLastMoveTime() const		{ return m_dwLastMoveTime; }
		DWORD			GetLastAttackTime() const	{ return m_dwLastAttackTime; }
		DWORD			GetLastAttackPVPTime() const	{ return m_dwLastAttackPVPTime; }

		void			SetLastAttacked(DWORD time);	// ¸¶Áö¸·À¸·Î °ø°Ý¹ÞÀº ½Ã°£ ¹× À§Ä¡¸¦ ÀúÀåÇÔ

		DWORD			GetLastAttackedByPC() const { return m_dwLastAttackedByPC; }
		void			SetLastAttackedByPC() { m_dwLastAttackedByPC = get_dword_time(); }

		bool			SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList = true);
		bool			IsSyncOwner(LPCHARACTER ch) const;

		bool			WarpSet(long x, long y, long lRealMapIndex = 0, DWORD dwPIDAddr = 0);
		void			SetWarpLocation(long lMapIndex, long x, long y);
		void			WarpEnd();
		const PIXEL_POSITION & GetWarpPosition() const { return m_posWarp; }
		long			GetWarpMapIndex() const { return m_lWarpMapIndex; }
		bool			WarpToPID(DWORD dwPID);

		void			SaveExitLocation();
		void			GetExitLocation(long& lMapIndex, long& x, long& y);
		void			ExitToSavedLocation();

		void			StartStaminaConsume();
		void			StopStaminaConsume();
		bool			IsStaminaConsume() const;
		bool			IsStaminaHalfConsume() const;

		void			ResetStopTime();
		DWORD			GetStopTime() const;

	protected:
		void			ClearSync();

		float			m_fSyncTime;
		LPCHARACTER		m_pkChrSyncOwner;
		CHARACTER_LIST	m_kLst_pkChrSyncOwned;	// ³»°¡ SyncOwnerÀÎ ÀÚµé

		const TNPCMovingPosition*	m_pMovingWay;
		int							m_iMovingWayIndex;
		int							m_iMovingWayMaxNum;
		bool						m_bMovingWayRepeat;
		long						m_lMovingWayBaseX;
		long						m_lMovingWayBaseY;

		PIXEL_POSITION	m_posDest;
		PIXEL_POSITION	m_posStart;
		PIXEL_POSITION	m_posWarp;
		long			m_lWarpMapIndex;

		PIXEL_POSITION	m_posExit;
		long			m_lExitMapIndex;

		DWORD			m_dwMoveStartTime;
		DWORD			m_dwMoveDuration;

		DWORD			m_dwLastMoveTime;
		DWORD			m_dwLastAttackTime;
		DWORD			m_dwLastAttackPVPTime;
		DWORD			m_dwWalkStartTime;
		DWORD			m_dwStopTime;

		DWORD			m_dwLastAttackedByPC;

		bool			m_bWalking;
		bool			m_bNowWalking;
		bool			m_bStaminaConsume;
		// End

#ifdef ENABLE_HYDRA_DUNGEON
	protected:
		bool			m_bLockTarget;
	public:
		void			LockOnTarget(bool bSet) { m_bLockTarget = bSet; }
#endif

		// Quickslot °ü·Ã
	public:
		void			LoadQuickslot(const ::google::protobuf::RepeatedPtrField<TQuickslot>& data);
		void			SyncQuickslot(BYTE bType, WORD wOldPos, WORD wNewPos);
#ifdef __ITEM_SWAP_SYSTEM__
		void			SyncSwapQuickslot(BYTE a, BYTE b);
#endif
		bool			GetQuickslot(BYTE pos, TQuickslot ** ppSlot);
		bool			SetQuickslot(BYTE pos, const TQuickslot & rSlot);
		bool			DelQuickslot(BYTE pos);
		bool			SwapQuickslot(BYTE a, BYTE b);
		void			ChainQuickslotItem(LPITEM pItem, BYTE bType, BYTE bOldPos);

	protected:
		TQuickslot		m_quickslot[QUICKSLOT_MAX_NUM];

#ifdef AHMET_FISH_EVENT_SYSTEM
		TPlayerFishEventSlot m_fishSlots[FISH_EVENT_SLOTS_NUM];
#endif	
		////////////////////////////////////////////////////////////////////////////////////////
		// Affect
	public:
		void			StartAffectEvent();
#ifdef SKILL_AFFECT_DEATH_REMAIN
		void			ClearAffect(bool bSave = false, bool isExceptGood = false); // isExceptGood 12 noiembrie 2018
#else
		void			ClearAffect(bool bSave=false);
#endif
		void			ComputeAffect(CAffect * pkAff, bool bAdd);
		bool			AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube = false);
		void			RefreshAffect();
		bool			RemoveAffect(DWORD dwType, bool useCompute = true, bool onlySave = false);
		bool			IsAffectFlag(DWORD dwAff) const;

		bool			UpdateAffect();	// called from EVENT
		int				ProcessAffect();

		void			LoadAffect(const ::google::protobuf::RepeatedPtrField<TPacketAffectElement>& elements);
		void			SaveAffect();

		// Affect loadingÀÌ ³¡³­ »óÅÂÀÎ°¡?
		bool			IsLoadedAffect() const	{ return m_bIsLoadedAffect; }		
		bool			IsGoodAffectSkill(BYTE bAffectType);

#ifdef SKILL_AFFECT_DEATH_REMAIN
		bool			IsGoodAffect(BYTE bAffectType);
#else
		bool			IsGoodAffect(BYTE bAffectType) const;
#endif

		void			RemoveGoodAffect(bool bSave = false);
		void			RemoveBadAffect();

		CAffect *		FindAffect(DWORD dwType, BYTE bApply=APPLY_NONE) const;
		const std::list<CAffect *> & GetAffectContainer() const	{ return m_list_pkAffect; }
		bool			RemoveAffect(CAffect * pkAff, bool useCompute = true, bool onlySave = false);
		void			RestoreSavedAffects();

	protected:
		bool			m_bIsLoadedAffect;
		TAffectFlag		m_afAffectFlag;
		std::list<CAffect *>	m_list_pkAffect;
		std::list<CAffect *>	m_list_pkAffectSave;

	public:
		// PARTY_JOIN_BUG_FIX
		void			SetParty(LPPARTY pkParty);
		LPPARTY			GetParty() const	{ return m_pkParty; }

		bool			RequestToParty(LPCHARACTER leader);
		void			DenyToParty(LPCHARACTER member);
		void			AcceptToParty(LPCHARACTER member);

		/// ÀÚ½ÅÀÇ ÆÄÆ¼¿¡ ´Ù¸¥ character ¸¦ ÃÊ´ëÇÑ´Ù.
		/**
		 * @param	pchInvitee ÃÊ´ëÇÒ ´ë»ó character. ÆÄÆ¼¿¡ Âü¿© °¡´ÉÇÑ »óÅÂÀÌ¾î¾ß ÇÑ´Ù.
		 *
		 * ¾çÃø character ÀÇ »óÅÂ°¡ ÆÄÆ¼¿¡ ÃÊ´ëÇÏ°í ÃÊ´ë¹ÞÀ» ¼ö ÀÖ´Â »óÅÂ°¡ ¾Æ´Ï¶ó¸é ÃÊ´ëÇÏ´Â Ä³¸¯ÅÍ¿¡°Ô ÇØ´çÇÏ´Â Ã¤ÆÃ ¸Þ¼¼Áö¸¦ Àü¼ÛÇÑ´Ù.
		 */
		void			PartyInvite(LPCHARACTER pchInvitee);

		/// ÃÊ´ëÇß´ø character ÀÇ ¼ö¶ôÀ» Ã³¸®ÇÑ´Ù.
		/**
		 * @param	pchInvitee ÆÄÆ¼¿¡ Âü¿©ÇÒ character. ÆÄÆ¼¿¡ Âü¿©°¡´ÉÇÑ »óÅÂÀÌ¾î¾ß ÇÑ´Ù.
		 *
		 * pchInvitee °¡ ÆÄÆ¼¿¡ °¡ÀÔÇÒ ¼ö ÀÖ´Â »óÈ²ÀÌ ¾Æ´Ï¶ó¸é ÇØ´çÇÏ´Â Ã¤ÆÃ ¸Þ¼¼Áö¸¦ Àü¼ÛÇÑ´Ù.
		 */
		void			PartyInviteAccept(LPCHARACTER pchInvitee);

		/// ÃÊ´ëÇß´ø character ÀÇ ÃÊ´ë °ÅºÎ¸¦ Ã³¸®ÇÑ´Ù.
		/**
		 * @param [in]	dwPID ÃÊ´ë Çß´ø character ÀÇ PID
		 */
		void			PartyInviteDeny(DWORD dwPID);

		bool			BuildUpdatePartyPacket(network::GCOutputPacket<network::GCPartyUpdatePacket> & out);
		int				GetLeadershipSkillLevel() const;

		bool			CanSummon(int iLeaderShip);

		void			SetPartyRequestEvent(LPEVENT pkEvent) { m_pkPartyRequestEvent = pkEvent; }

	protected:

		/// ÆÄÆ¼¿¡ °¡ÀÔÇÑ´Ù.
		/**
		 * @param	pkLeader °¡ÀÔÇÒ ÆÄÆ¼ÀÇ ¸®´õ
		 */
		void			PartyJoin(LPCHARACTER pkLeader);

		/**
		 * ÆÄÆ¼ °¡ÀÔÀ» ÇÒ ¼ö ¾øÀ» °æ¿ìÀÇ ¿¡·¯ÄÚµå.
		 * Error code ´Â ½Ã°£¿¡ ÀÇÁ¸ÀûÀÎ°¡¿¡ µû¶ó º¯°æ°¡´ÉÇÑ(mutable) type °ú Á¤Àû(static) type À¸·Î ³ª´¶´Ù.
		 * Error code ÀÇ °ªÀÌ PERR_SEPARATOR º¸´Ù ³·À¸¸é º¯°æ°¡´ÉÇÑ type ÀÌ°í ³ôÀ¸¸é Á¤Àû type ÀÌ´Ù.
		 */
		enum PartyJoinErrCode {
			PERR_NONE		= 0,	///< Ã³¸®¼º°ø
			PERR_SERVER,			///< ¼­¹ö¹®Á¦·Î ÆÄÆ¼°ü·Ã Ã³¸® ºÒ°¡
			PERR_DUNGEON,			///< Ä³¸¯ÅÍ°¡ ´øÀü¿¡ ÀÖÀ½
			PERR_OBSERVER,			///< °üÀü¸ðµåÀÓ
			PERR_LVBOUNDARY,		///< »ó´ë Ä³¸¯ÅÍ¿Í ·¹º§Â÷ÀÌ°¡ ³²
			PERR_LOWLEVEL,			///< »ó´ëÆÄÆ¼ÀÇ ÃÖ°í·¹º§º¸´Ù 30·¹º§ ³·À½
			PERR_HILEVEL,			///< »ó´ëÆÄÆ¼ÀÇ ÃÖÀú·¹º§º¸´Ù 30·¹º§ ³ôÀ½
			PERR_ALREADYJOIN,		///< ÆÄÆ¼°¡ÀÔ ´ë»ó Ä³¸¯ÅÍ°¡ ÀÌ¹Ì ÆÄÆ¼Áß
			PERR_PARTYISFULL,		///< ÆÄÆ¼ÀÎ¿ø Á¦ÇÑ ÃÊ°ú
			PERR_SEPARATOR,			///< Error type separator.
			PERR_DIFFEMPIRE,		///< »ó´ë Ä³¸¯ÅÍ¿Í ´Ù¸¥ Á¦±¹ÀÓ
			PERR_MAX				///< Error code ÃÖ°íÄ¡. ÀÌ ¾Õ¿¡ Error code ¸¦ Ãß°¡ÇÑ´Ù.
		};

		/// ÆÄÆ¼ °¡ÀÔÀÌ³ª °á¼º °¡´ÉÇÑ Á¶°ÇÀ» °Ë»çÇÑ´Ù.
		/**
		 * @param 	pchLeader ÆÄÆ¼ÀÇ leader ÀÌ°Å³ª ÃÊ´ëÇÑ character
		 * @param	pchGuest ÃÊ´ë¹Þ´Â character
		 * @return	¸ðµç PartyJoinErrCode °¡ ¹ÝÈ¯µÉ ¼ö ÀÖ´Ù.
		 */
		static PartyJoinErrCode	IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		/// ÆÄÆ¼ °¡ÀÔÀÌ³ª °á¼º °¡´ÉÇÑ µ¿ÀûÀÎ Á¶°ÇÀ» °Ë»çÇÑ´Ù.
		/**
		 * @param 	pchLeader ÆÄÆ¼ÀÇ leader ÀÌ°Å³ª ÃÊ´ëÇÑ character
		 * @param	pchGuest ÃÊ´ë¹Þ´Â character
		 * @return	mutable type ÀÇ code ¸¸ ¹ÝÈ¯ÇÑ´Ù.
		 */
		static PartyJoinErrCode	IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest);

		LPPARTY			m_pkParty;
		DWORD			m_dwLastDeadTime;
		LPEVENT			m_pkPartyRequestEvent;

		/**
		 * ÆÄÆ¼ÃÊÃ» Event map.
		 * key: ÃÊ´ë¹ÞÀº Ä³¸¯ÅÍÀÇ PID
		 * value: eventÀÇ pointer
		 *
		 * ÃÊ´ëÇÑ Ä³¸¯ÅÍµé¿¡ ´ëÇÑ event map.
		 */
		typedef std::map< DWORD, LPEVENT >	EventMap;
		EventMap		m_PartyInviteEventMap;

		// END_OF_PARTY_JOIN_BUG_FIX

		////////////////////////////////////////////////////////////////////////////////////////
		// Dungeon
	public:
		void			SetDungeon(LPDUNGEON pkDungeon);
		LPDUNGEON		GetDungeon() const	{ return m_pkDungeon; }
		LPDUNGEON		GetDungeonForce() const;
	protected:
		LPDUNGEON	m_pkDungeon;
		int			m_iEventAttr;

		////////////////////////////////////////////////////////////////////////////////////////
		// Guild
	public:
		void			SetGuild(CGuild * pGuild);
		CGuild*			GetGuild() const	{ return m_pGuild; }

		void			SetWarMap(CWarMap* pWarMap);
		CWarMap*		GetWarMap() const	{ return m_pWarMap; }

	protected:
		CGuild *		m_pGuild;
		DWORD			m_dwUnderGuildWarInfoMessageTime;
		CWarMap *		m_pWarMap;

		////////////////////////////////////////////////////////////////////////////////////////
		// Item related
	public:
		bool			CanHandleItem(bool bSkipRefineCheck = false, bool bSkipObserver = false); // ¾ÆÀÌÅÛ °ü·Ã ÇàÀ§¸¦ ÇÒ ¼ö ÀÖ´Â°¡?

		bool			IsItemLoaded() const	{ return m_bItemLoaded; }
		void			SetItemLoaded()	{ m_bItemLoaded = true; }

		void			ClearItem();
#ifndef __MARK_NEW_ITEM_SYSTEM__
		void			SetItem(TItemPos Cell, LPITEM item);
#else
		void			SetItem(TItemPos Cell, LPITEM item, bool bWereMine = false);
#endif
		void			EncodeItemPacket(LPITEM item, network::GCOutputPacket<network::GCItemSetPacket>& pack) const;
		LPITEM			GetItem(TItemPos Cell) const;
		LPITEM			GetInventoryItem(WORD wCell) const;
		bool			IsEmptyItemGrid(TItemPos Cell, BYTE size, int iExceptionCell = -1) const;

		void			SetWear(BYTE bCell, LPITEM item);
		LPITEM			GetWear(BYTE bCell) const;

		// MYSHOP_PRICE_LIST
		void			UseSilkBotary(void); 		/// ºñ´Ü º¸µû¸® ¾ÆÀÌÅÛÀÇ »ç¿ë

		/// DB Ä³½Ã·Î ºÎÅÍ ¹Þ¾Æ¿Â °¡°ÝÁ¤º¸ ¸®½ºÆ®¸¦ À¯Àú¿¡°Ô Àü¼ÛÇÏ°í º¸µû¸® ¾ÆÀÌÅÛ »ç¿ëÀ» Ã³¸®ÇÑ´Ù.
		/**
		 * @param [in] p	°¡°ÝÁ¤º¸ ¸®½ºÆ® ÆÐÅ¶
		 *
		 * Á¢¼ÓÇÑ ÈÄ Ã³À½ ºñ´Ü º¸µû¸® ¾ÆÀÌÅÛ »ç¿ë ½Ã UseSilkBotary ¿¡¼­ DB Ä³½Ã·Î °¡°ÝÁ¤º¸ ¸®½ºÆ®¸¦ ¿äÃ»ÇÏ°í
		 * ÀÀ´ä¹ÞÀº ½ÃÁ¡¿¡ ÀÌ ÇÔ¼ö¿¡¼­ ½ÇÁ¦ ºñ´Üº¸µû¸® »ç¿ëÀ» Ã³¸®ÇÑ´Ù.
		 */
		void			UseSilkBotaryReal(const google::protobuf::RepeatedPtrField<network::TItemPriceInfo>& p);
		// END_OF_MYSHOP_PRICE_LIST

		bool			UseItemEx(LPITEM item, TItemPos DestCell);
		bool			UseItem(TItemPos Cell, TItemPos DestCell = NPOS);

		// ADD_REFINE_BUILDING
		bool			IsRefineThroughGuild() const;
		CGuild *		GetRefineGuild() const;
		int				ComputeRefineFee(int iCost, int iMultiply = 5) const;
		void			PayRefineFee(int iTotalMoney);
		void			SetRefineNPC(LPCHARACTER ch);
		DWORD			GetRefineNPCVID() const { return m_dwRefineNPCVID; }
		LPCHARACTER		GetRefineNPC() const;
		void			SetRefinedByCount(DWORD dwVID, DWORD dwCount = 1);
		DWORD			GetRefinedByCount(DWORD dwVID) const;
		// END_OF_ADD_REFINE_BUILDING

		bool			RefineItem(LPITEM pkItem, LPITEM pkTarget);
#ifdef INCREASE_ITEM_STACK
		bool			DropItem(TItemPos Cell, WORD bCount = 0);
#else
		bool			DropItem(TItemPos Cell,  BYTE bCount=0);
#endif
		bool			GiveRecallItem(LPITEM item);
		void			ProcessRecallItem(LPITEM item);

		//	void			PotionPacket(int iPotionType);
		void			EffectPacket(int enumEffectType);
		void			SpecificEffectPacket(const char* filename);

		bool			CanRefineBySmith(LPITEM pkItem, BYTE bType, LPITEM pkItemScroll = NULL, LPCHARACTER pkSmith = NULL, bool bMessage = false);

		// ADD_MONSTER_REFINE
		bool			DoRefine(LPITEM item, bool bMoneyOnly = false, bool bFastRefine = false);
		// END_OF_ADD_MONSTER_REFINE

		bool			DoRefineWithScroll(LPITEM item, bool bFastRefine = false);
		bool			RefineInformation(WORD wCell, BYTE bType, int iAdditionalCell = -1);

		void			SetRefineMode(int iAdditionalCell = -1);
		void			ClearRefineMode();

		bool			GiveItem(LPCHARACTER victim, TItemPos Cell);
		bool			CanReceiveItem(LPCHARACTER from, LPITEM item) const;
		void			ReceiveItem(LPCHARACTER from, LPITEM item);
		bool			GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector <DWORD> &dwItemVnums, 
							std::vector <DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count, bool bIsGMOwner = false);

#ifdef INCREASE_ITEM_STACK
		bool			MoveItem(TItemPos pos, TItemPos change_pos, WORD num);
#else
		bool			MoveItem(TItemPos pos, TItemPos change_pos, BYTE num);
#endif
		bool			PickupItem(DWORD vid);
		bool			PickupItem_PartyStack(LPCHARACTER owner, LPITEM item);
		bool			EquipItem(LPITEM item, int iCandidateCell = -1, bool bCheckNecessaryOnly = false, bool bCheckLimits = true, bool force = false);
		bool			UnequipItem(LPITEM item);

		// ÇöÀç itemÀ» Âø¿ëÇÒ ¼ö ÀÖ´Â Áö È®ÀÎÇÏ°í, ºÒ°¡´É ÇÏ´Ù¸é Ä³¸¯ÅÍ¿¡°Ô ÀÌÀ¯¸¦ ¾Ë·ÁÁÖ´Â ÇÔ¼ö
		bool			CanEquipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS, bool force = false);

		// Âø¿ëÁßÀÎ itemÀ» ¹þÀ» ¼ö ÀÖ´Â Áö È®ÀÎÇÏ°í, ºÒ°¡´É ÇÏ´Ù¸é Ä³¸¯ÅÍ¿¡°Ô ÀÌÀ¯¸¦ ¾Ë·ÁÁÖ´Â ÇÔ¼ö
		bool			CanUnequipNow(const LPITEM item, const TItemPos& srcCell = NPOS, const TItemPos& destCell = NPOS);

		bool			SwapItem(WORD wCell, WORD wDestCell);
#ifdef INCREASE_ITEM_STACK
		LPITEM			AutoGiveItem(DWORD dwItemVnum, WORD bCount=1, int iRarePct = -1, bool bMsg = true);
#else
		LPITEM			AutoGiveItem(DWORD dwItemVnum, BYTE bCount=1, int iRarePct = -1, bool bMsg = true);
#endif
		LPITEM			AutoGiveItem(LPITEM item, bool longOwnerShip = false);
		LPITEM			AutoGiveItem(const network::TItemData* pTable, bool bUseCell = true);

		bool			CanStackItem(LPITEM item, LPITEM* ppNewItem = NULL, bool bCheckFullCount = true);
		bool			TryStackItem(LPITEM item, LPITEM* ppNewItem = NULL);
		void			DoStackItem(LPITEM item, LPITEM pkNewItem, bool removeOld = true);
		bool			StackFillItem(LPITEM item, LPITEM* ppNewItem = NULL);
		
		int				GetEmptySlotInWindow(BYTE bWindow, BYTE bSize) const;
		int				GetEmptyInventory(BYTE size) const;
		int				GetEmptyInventoryNew(BYTE window, BYTE size) const;
		bool			GetInventorySlotRange(BYTE bWindow, WORD& wSlotStart, WORD& wSlotEnd) const;
#ifdef __DRAGONSOUL__
		int				GetEmptyDragonSoulInventory(LPITEM pItem) const;
		void			CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const;
#endif
		int				CountEmptyInventory() const;
		int				GetEmptyNewInventory(BYTE bWindow) const;

		int				CountSpecifyItem(DWORD vnum, LPITEM except = NULL) const;
		bool			RemoveSpecifyItem(DWORD vnum, DWORD count = 1, LPITEM except = NULL); // returns true if any gm item removed
		LPITEM			FindSpecifyItem(DWORD vnum) const;
#ifdef __EQUIPMENT_CHANGER__
		LPITEM			FindItemByID(DWORD id, bool search_equip = false) const;
#else
		LPITEM			FindItemByID(DWORD id) const;
#endif
		LPITEM			FindItemByVID(DWORD vid) const;

		int				CountSpecifyTypeItem(BYTE type) const;
		void			RemoveSpecifyTypeItem(BYTE type, DWORD count = 1);

		bool			IsEquipUniqueItem(DWORD dwItemVnum) const;

		// CHECK_UNIQUE_GROUP
		bool			IsEquipUniqueGroup(DWORD dwGroupVnum) const;
		// END_OF_CHECK_UNIQUE_GROUP

		void			SendEquipment(LPCHARACTER ch);
		// End of Item

	protected:

		/// ÇÑ ¾ÆÀÌÅÛ¿¡ ´ëÇÑ °¡°ÝÁ¤º¸¸¦ Àü¼ÛÇÑ´Ù.
		/**
		 * @param [in]	dwItemVnum ¾ÆÀÌÅÛ vnum
		 * @param [in]	dwItemPrice ¾ÆÀÌÅÛ °¡°Ý
		 */
		void			SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice);

		bool			m_bNoOpenedShop;	///< ÀÌ¹ø Á¢¼Ó ÈÄ °³ÀÎ»óÁ¡À» ¿¬ ÀûÀÌ ÀÖ´ÂÁöÀÇ ¿©ºÎ(¿­¾ú´ø ÀûÀÌ ¾ø´Ù¸é true)

		bool			m_bItemLoaded;
		int				m_iRefineAdditionalCell;
		bool			m_bUnderRefine;
		DWORD			m_dwRefineNPCVID;
		std::map<DWORD, DWORD>	m_map_refinedByCount;

	public:
		////////////////////////////////////////////////////////////////////////////////////////
		// Money related
		long long		GetGold() const		{ return m_points.gold;	}
		void			SetGold(long long gold)	{ m_points.gold = gold;	}
		bool			DropGold(long long gold);
		long long		GetAllowedGold() const;
		void			GiveGold(long long iAmount);	// ÆÄÆ¼°¡ ÀÖÀ¸¸é ÆÄÆ¼ ºÐ¹è, ·Î±× µîÀÇ Ã³¸®
		// End of Money
#ifdef ENABLE_ZODIAC_TEMPLE
		int				GetAnimasphere() const		{ return m_points.animasphere;	}
		void			SetAnimasphere(BYTE animasphere)	{ m_points.animasphere = animasphere;	}
		void			SetZodiacBadges(BYTE id, BYTE value);
		BYTE			GetZodiacBadges(BYTE value);
#endif
		////////////////////////////////////////////////////////////////////////////////////////
		// Shop related
	public:
		void			SetShop(LPSHOP pkShop);
		LPSHOP			GetShop() const { return m_pkShop; }
		void			ShopPacket(BYTE bSubHeader);

		void			SetShopOwner(LPCHARACTER ch) { m_pkChrShopOwner = ch; }
		LPCHARACTER		GetShopOwner() const { return m_pkChrShopOwner;}

		bool			CanShopNow() const;
		void			OpenMyShop(const char* c_pszSign, const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& table);
		LPSHOP			GetMyShop() const { return m_pkMyShop; }
		void			CloseMyShop();

#ifdef AUCTION_SYSTEM
		void			SetAuctionShop(const std::string& sign, BYTE sign_style, float color_red, float color_green, float color_blue)
		{
			m_stShopSign = sign;
			m_ShopSignStyle = sign_style;
			m_ShopSignRed = color_red;
			m_ShopSignGreen = color_green;
			m_ShopSignBlue = color_blue;
		}
#endif

	protected:

		LPSHOP			m_pkShop;
		LPSHOP			m_pkMyShop;
		std::string		m_stShopSign;
#ifdef AUCTION_SYSTEM
		BYTE			m_ShopSignStyle;
		float			m_ShopSignRed;
		float			m_ShopSignGreen;
		float			m_ShopSignBlue;
#endif
		LPCHARACTER		m_pkChrShopOwner;
		// End of shop

#ifdef __DAMAGE_QUEST_TRIGGER__
	public:
		void			SetQuestDamage(int iDmg) { iQuestDamage = iDmg; }
		int				GetQuestDamage() { return iQuestDamage; }
	private:
		int				iQuestDamage;
#endif
		////////////////////////////////////////////////////////////////////////////////////////
		// Exchange related
	public:
		bool			ExchangeStart(LPCHARACTER victim);
		void			SetExchange(CExchange * pkExchange);
		CExchange *		GetExchange() const	{ return m_pkExchange;	}

	protected:
		CExchange *		m_pkExchange;
		// End of Exchange

	public:

#ifdef AHMET_FISH_EVENT_SYSTEM
		void 			FishEventGeneralInfo();
		void			FishEventUseBox(TItemPos itemPos);
		bool 			FishEventIsValidPosition(BYTE shapePos, BYTE shapeType);
		void 			FishEventPlaceShape(BYTE shapePos, BYTE shapeType);
		void 			FishEventAddShape(BYTE shapePos);
		void 			FishEventCheckEnd();
#endif

		////////////////////////////////////////////////////////////////////////////////////////
		// Battle
	public:
		struct TBattleInfo
		{
			int iTotalDamage;
#ifdef __FAKE_PC__
			int iTotalFakePCDamage;
#endif
			int iAggro;

#ifdef __FAKE_PC__
			TBattleInfo(int iTot, int iTotFakePC, int iAggr)
				: iTotalDamage(iTot), iTotalFakePCDamage(iTotFakePC), iAggro(iAggr)
#else
			TBattleInfo(int iTot, int iAggr)
				: iTotalDamage(iTot), iAggro(iAggr)
#endif
				{}
		};
		typedef std::map<VID, TBattleInfo>	TDamageMap;

		typedef struct SAttackLog
		{
			DWORD	dwVID;
			DWORD	dwTime;
		} AttackLog;

		bool				Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		bool				__Profile__Damage(LPCHARACTER pAttacker, int dam, EDamageType type = DAMAGE_TYPE_NORMAL);
		void				DeathPenalty(BYTE bExpLossPercent);
		void				ReviveInvisible(int iDur);

		bool				Attack(LPCHARACTER pkVictim, BYTE bType = 0);
		bool				IsAlive() const		{ return m_pointsInstant.position == POS_DEAD ? false : true; }
		bool				CanFight() const;

		bool				CanBeginFight() const;
		void				BeginFight(LPCHARACTER pkVictim); // pkVictimr°ú ½Î¿ì±â ½ÃÀÛÇÑ´Ù. (°­Á¦ÀûÀÓ, ½ÃÀÛÇÒ ¼ö ÀÖ³ª Ã¼Å©ÇÏ·Á¸é CanBeginFightÀ» »ç¿ë)

		bool				CounterAttack(LPCHARACTER pkChr); // ¹Ý°ÝÇÏ±â (¸ó½ºÅÍ¸¸ »ç¿ë)

		bool				IsStun() const;
		void				Stun();
		bool				IsDead() const;
		void				Dead(LPCHARACTER pkKiller = NULL, bool bImmediateDead=false);

		void				Reward(bool bItemDrop);
		void				RewardGold(LPCHARACTER pkAttacker);
		void				UpdateStatOnKill(LPCHARACTER pkVictim);

		bool				Shoot(BYTE bType);
		void				FlyTarget(DWORD dwTargetVID, long x, long y, bool is_add);

		void				ForgetMyAttacker();
		void				AggregateMonster(bool allMonster = false);
		void				AttractRanger();
		void				PullMonster();

		int					GetArrowAndBow(LPITEM * ppkBow, LPITEM * ppkArrow, int iArrowCount = 1);
		void				UseArrow(LPITEM pkArrow, DWORD dwArrowCount);

		void				AttackedByPoison(LPCHARACTER pkAttacker);
		void				RemovePoison();

		void				AttackedByFire(LPCHARACTER pkAttacker, int amount, int count);
		void				RemoveFire();

		void				UpdateAlignment(int iAmount);
		int					GetAlignment() const;

		//¼±¾ÇÄ¡ ¾ò±â 
		int					GetRealAlignment() const;
		void				ShowAlignment(bool bShow);
		int					GetAlignmentIndex() const;

		void				CheckAlignmentAffect();

		void				SetKillerMode(bool bOn);
		bool				IsKillerMode() const;
		void				UpdateKillerMode();

		BYTE				GetPKMode() const;
		void				SetPKMode(BYTE bPKMode);
#ifdef ENABLE_BLOCK_PKMODE
		void				SetBlockPKMode(BYTE bPKMode, bool block);
		bool				IsPKModeBlocked() { return m_blockPKMode; }
#endif

		void				ItemDropPenalty(LPCHARACTER pkKiller);

		void				UpdateAggrPoint(LPCHARACTER ch, EDamageType type, int dam);

		//
		// HACK
		// 
	public:
		void SetComboSequence(BYTE seq);
		BYTE GetComboSequence() const;

		void SetLastComboTime(DWORD time);
		DWORD GetLastComboTime() const;

		int GetValidComboInterval() const;
		void SetValidComboInterval(int interval);

		BYTE GetComboIndex() const;

		void IncreaseComboHackCount(int k = 1);
		void ResetComboHackCount();
		void SkipComboAttackByTime(int interval);
		DWORD GetSkipComboAttackByTime() const;
		
		void SetLastSpacebarTime(DWORD time) { m_dwLastSpacebar = time; }
		DWORD GetLastSpacebarTime() const { return m_dwLastSpacebar; }

	protected:
		BYTE m_bComboSequence;
		DWORD m_dwLastComboTime;
		int m_iValidComboInterval;
		BYTE m_bComboIndex;
		int m_iComboHackCount;
		DWORD m_dwSkipComboAttackByTime;
		DWORD m_dwLastSpacebar;

	protected:
		void				UpdateAggrPointEx(LPCHARACTER ch, EDamageType type, int dam, TBattleInfo & info);
		void				ChangeVictimByAggro(int iNewAggro, LPCHARACTER pNewVictim);

		DWORD				m_dwFlyTargetID;
		std::vector<DWORD>	m_vec_dwFlyTargets;
		TDamageMap			m_map_kDamage;	// ¾î¶² Ä³¸¯ÅÍ°¡ ³ª¿¡°Ô ¾ó¸¶¸¸Å­ÀÇ µ¥¹ÌÁö¸¦ ÁÖ¾ú´Â°¡?
//		AttackLog			m_kAttackLog;
		DWORD				m_dwKillerPID;

		int					m_iAlignment;		// Lawful/Chaotic value -200000 ~ 200000
		int					m_iRealAlignment;
		int					m_iKillerModePulse;
		BYTE				m_bPKMode;
#ifdef ENABLE_BLOCK_PKMODE
		bool				m_blockPKMode;
#endif

		// Aggro
		DWORD				m_dwLastVictimSetTime;
		int					m_iMaxAggro;
		// End of Battle

		// Stone
	public:
		void				SetStone(LPCHARACTER pkChrStone);
		void				ClearStone();
		void				DetermineDropMetinStone();
		DWORD				GetDropMetinStoneVnum() const { return m_dwDropMetinStone; }
		BYTE				GetDropMetinStonePct() const { return m_bDropMetinStonePct; }

	protected:
		LPCHARACTER			m_pkChrStone;		// ³ª¸¦ ½ºÆùÇÑ µ¹
		CHARACTER_SET		m_set_pkChrSpawnedBy;	// ³»°¡ ½ºÆùÇÑ ³ðµé
		DWORD				m_dwDropMetinStone;
		BYTE				m_bDropMetinStonePct;
		// End of Stone

	public:
		enum
		{
			SKILL_UP_BY_POINT,
			SKILL_UP_BY_BOOK,
			SKILL_UP_BY_TRAIN,

			// ADD_GRANDMASTER_SKILL
			SKILL_UP_BY_QUEST,
			// END_OF_ADD_GRANDMASTER_SKILL
#ifdef __LEGENDARY_SKILL__
			SKILL_UP_BY_QUEST2,
#endif
		};

		void				SkillLevelPacket();
		void				SkillLevelUp(DWORD dwVnum, BYTE bMethod = SKILL_UP_BY_POINT);
		bool				SkillLevelDown(DWORD dwVnum);
		// ADD_GRANDMASTER_SKILL
		bool				UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster = true);
		void				ResetSkill();
		void				SetSkillLevelChanged();
		void				SetSkillLevel(DWORD dwVnum, BYTE bLev);
		int					GetUsedSkillMasterType(DWORD dwVnum);

		bool				IsLearnableSkill(DWORD dwSkillVnum) const;
		// END_OF_ADD_GRANDMASTER_SKILL

		bool				CheckSkillHitCount(const BYTE SkillID, const VID dwTargetVID);
		const DWORD*		GetUsableSkillList() const;
		bool				CanUseSkill(DWORD dwSkillVnum) const;
		bool				IsUsableSkillMotion(DWORD dwMotionIndex) const;
		int					GetSkillLevel(DWORD dwVnum) const;
		int					GetSkillMasterType(DWORD dwVnum) const;
		int					GetSkillPower(DWORD dwVnum, BYTE bLevel = 0) const;

		time_t				GetSkillNextReadTime(DWORD dwVnum) const;
		void				SetSkillNextReadTime(DWORD dwVnum, time_t time);
		void				SkillLearnWaitMoreTimeMessage(DWORD dwVnum);

		void				ComputePassiveSkill(DWORD dwVnum);
		int					ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel = 0);
		int					ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel = 0);
		void				ComputeSkillPoints();

		void				SetSkillGroup(BYTE bSkillGroup);
#ifdef __FAKE_PC__
		BYTE				GetSkillGroup() const		{ return FakePC_Check() ? FakePC_GetOwner()->m_points.skill_group : m_points.skill_group; }
#else
		BYTE				GetSkillGroup() const		{ return m_points.skill_group; }
#endif

		int					ComputeCooltime(int time);

		void				GiveRandomSkillBook();

		void				DisableCooltime();
		bool				LearnSkillByBook(DWORD dwSkillVnum, BYTE bProb = 0, BYTE bItemSubType = 0);
#ifdef __FAKE_BUFF__
		bool				LearnGrandMasterSkill(DWORD dwSkillVnum, bool bFakeBuff);
		const DWORD* 		FakeBuff_GetUsableSkillList() const;
#else
		bool				LearnGrandMasterSkill(DWORD dwSkillVnum);
#endif
#ifdef __LEGENDARY_SKILL__
		bool				LearnLegendarySkill(DWORD dwSkillVnum);
#endif

	private:
		BYTE				m_bRandom;
	public:
		void				SetRandom(BYTE rand) { m_bRandom = rand; }
		BYTE				GetRandom() { return m_bRandom; }

	private:
		bool				m_bDisableCooltime;
		DWORD				m_dwLastSkillTime;	///< ¸¶Áö¸·À¸·Î skill À» ¾´ ½Ã°£(millisecond).
		DWORD				m_dwLastSkillVnum;
		// End of Skill

		// MOB_SKILL
	public:
		bool				HasMobSkill() const;
		size_t				CountMobSkill() const;
		const TMobSkillInfo * GetMobSkill(unsigned int idx) const;
		bool				CanUseMobSkill(unsigned int idx) const;
		bool				UseMobSkill(unsigned int idx);
		void				ResetMobSkillCooltime();
	protected:
		DWORD				m_adwMobSkillCooltime[MOB_SKILL_MAX_NUM];
		// END_OF_MOB_SKILL

		// for SKILL_MUYEONG
	public:
		void				StartMuyeongEvent();
		void				StopMuyeongEvent();

	private:
		LPEVENT				m_pkMuyeongEvent;

		// for SKILL_CHAIN lighting
	public:
		int					GetChainLightningIndex() const { return m_iChainLightingIndex; }
		void				IncChainLightningIndex() { ++m_iChainLightingIndex; }
		void				AddChainLightningExcept(LPCHARACTER ch) { m_setExceptChainLighting.insert(ch); }
		void				ResetChainLightningIndex() { m_iChainLightingIndex = 0; m_setExceptChainLighting.clear(); }
		int					GetChainLightningMaxCount() const;
		const CHARACTER_SET& GetChainLightingExcept() const { return m_setExceptChainLighting; }

	private:
		int					m_iChainLightingIndex;
		CHARACTER_SET m_setExceptChainLighting;

		// for SKILL_EUNHYUNG
	public:
		void				SetAffectedEunhyung();
		void				ClearAffectedEunhyung() { m_dwAffectedEunhyungLevel = 0; }
		bool				GetAffectedEunhyung() const { return m_dwAffectedEunhyungLevel; }

	private:
		DWORD				m_dwAffectedEunhyungLevel;

		//
		// Skill levels
		//
	protected:
		TPlayerSkill*					m_pSkillLevels;
		std::unordered_map<BYTE, int>		m_SkillDamageBonus;
		std::map<int, TSkillUseInfo>	m_SkillUseInfo;

		////////////////////////////////////////////////////////////////////////////////////////
		// AI related
	public:
		void			AssignTriggers(const network::TMobTable * table);
		LPCHARACTER		GetVictim() const;	// °ø°ÝÇÒ ´ë»ó ¸®ÅÏ
		void			SetVictim(LPCHARACTER pkVictim);
		LPCHARACTER		GetNearestVictim(LPCHARACTER pkChr);
		LPCHARACTER		GetProtege() const;	// º¸È£ÇØ¾ß ÇÒ ´ë»ó ¸®ÅÏ

		bool			Follow(LPCHARACTER pkChr, float fMinimumDistance = 150.0f);
		bool			Return();
		bool			IsGuardNPC() const;
		bool			IsChangeAttackPosition(LPCHARACTER target) const;
		void			ResetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time() - AI_CHANGE_ATTACK_POISITION_TIME_NEAR;}
		void			SetChangeAttackPositionTime() { m_dwLastChangeAttackPositionTime = get_dword_time();}

		bool			OnIdle();

		void			OnAttack(LPCHARACTER pkChrAttacker);
		void			OnClick(LPCHARACTER pkChrCauser);

		VID				m_kVIDVictim;

	protected:
		DWORD			m_dwLastChangeAttackPositionTime;
		CTrigger		m_triggerOnClick;
		// End of AI

		////////////////////////////////////////////////////////////////////////////////////////
		// Target
	protected:
		LPCHARACTER				m_pkChrTarget;		// ³» Å¸°Ù
		CHARACTER_SET	m_set_pkChrTargetedBy;	// ³ª¸¦ Å¸°ÙÀ¸·Î °¡Áö°í ÀÖ´Â »ç¶÷µé

	public:
		void				SetTarget(LPCHARACTER pkChrTarget);
		void				BroadcastTargetPacket();
		void				ClearTarget();
		void				CheckTarget();
		LPCHARACTER			GetTarget() const { return m_pkChrTarget; }

		////////////////////////////////////////////////////////////////////////////////////////
		// Safebox
	public:
		int					GetSafeboxSize() const;
		void				QuerySafeboxSize();
		void				SetSafeboxSize(int size);
		void				SetSafeboxNeedPassword(bool bNeedPassword);
		bool				IsNeedSafeboxPassword() const;

		CSafebox *			GetSafebox() const;
		void				LoadSafebox(int iSize, DWORD dwGold, const ::google::protobuf::RepeatedPtrField<network::TItemData>& items);
		void				ChangeSafeboxSize(BYTE bSize);
		void				CloseSafebox();

		/// Ã¢°í ¿­±â ¿äÃ»
		/**
		 * @param [in]	pszPassword 1ÀÚ ÀÌ»ó 6ÀÚ ÀÌÇÏÀÇ Ã¢°í ºñ¹Ð¹øÈ£
		 *
		 * DB ¿¡ Ã¢°í¿­±â¸¦ ¿äÃ»ÇÑ´Ù.
		 * Ã¢°í´Â Áßº¹À¸·Î ¿­Áö ¸øÇÏ¸ç, ÃÖ±Ù Ã¢°í¸¦ ´ÝÀº ½Ã°£À¸·Î ºÎÅÍ 10ÃÊ ÀÌ³»¿¡´Â ¿­ Áö ¸øÇÑ´Ù.
		 */
		void				ReqSafeboxLoad(const char* pszPassword);

		/// Ã¢°í ¿­±â ¿äÃ»ÀÇ Ãë¼Ò
		/**
		 * ReqSafeboxLoad ¸¦ È£ÃâÇÏ°í CloseSafebox ÇÏÁö ¾Ê¾ÒÀ» ¶§ ÀÌ ÇÔ¼ö¸¦ È£ÃâÇÏ¸é Ã¢°í¸¦ ¿­ ¼ö ÀÖ´Ù.
		 * Ã¢°í¿­±âÀÇ ¿äÃ»ÀÌ DB ¼­¹ö¿¡¼­ ½ÇÆÐÀÀ´äÀ» ¹Þ¾ÒÀ» °æ¿ì ÀÌ ÇÔ¼ö¸¦ »ç¿ëÇØ¼­ ¿äÃ»À» ÇÒ ¼ö ÀÖ°Ô ÇØÁØ´Ù.
		 */
		void				CancelSafeboxLoad( void ) { m_bOpeningSafebox = false; }

		void				SetMallLoadTime(int t) { m_iMallLoadTime = t; }
		int					GetMallLoadTime() const { return m_iMallLoadTime; }

		CSafebox *			GetMall() const;
		void				LoadMall(const ::google::protobuf::RepeatedPtrField<network::TItemData>& items);
		void				CloseMall();

		void				SetSafeboxOpenPosition();
		float				GetDistanceFromSafeboxOpen() const;

	protected:
		CSafebox *			m_pkSafebox;
		int					m_iSafeboxSize;
		bool				m_bSafeboxNeedPassword;
		int					m_iSafeboxLoadTime;
		bool				m_bOpeningSafebox;	///< Ã¢°í°¡ ¿­±â ¿äÃ» ÁßÀÌ°Å³ª ¿­·ÁÀÖ´Â°¡ ¿©ºÎ, true ÀÏ °æ¿ì ¿­±â¿äÃ»ÀÌ°Å³ª ¿­·ÁÀÖÀ½.

		CSafebox *			m_pkMall;
		int					m_iMallLoadTime;

		PIXEL_POSITION		m_posSafeboxOpen;

		////////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////
		// Mounting
	public:
		enum {
			HORSE_MAX_GRADE = 3,
			HORSES_HOE_TIMEOUT_TIME = 7 * 24 * 60 * 60,
		};

		void				MountVnum(DWORD vnum);
		DWORD				GetMountVnum() const { return m_dwMountVnum; }
		DWORD				GetLastMountTime() const { return m_dwMountTime; }

		CMountSystem*		GetMountSystem() const	{ return m_pkMountSystem; }
		void				FinishMountLoading();

		void				SetHorseGrade(BYTE bGrade, bool bPacket = true);
		BYTE				GetHorseGrade() const		{ return m_bHorseGrade; }
		void				SetHorseRageLevel(BYTE bLevel, bool bPacket = true);
		BYTE				GetHorseRageLevel() const	{ return m_bHorseRageLevel; }
		void				SetHorseRagePct(short sPct, bool bPacket = true);
		short				GetHorseRagePct() const		{ return m_sHorseRagePct; }
		void				SetHorseRageTimeout(DWORD dwTimeout, bool bPacket = true);
		DWORD				GetHorseRageTimeout() const	{ return IsHorseRage() ? m_dwHorseRageTimeout : 0; }
		bool				IsHorseRage() const			{ return m_pkHorseRageTimeoutEvent != NULL; }
		DWORD				GetNextHorseRageDecTime();
		void				StartHorseRage();
		void				StopHorseRage();
		void				SendHorseRagePacket();
		bool				UpgradeHorseGrade();
		void				SetHorsesHoeTimeout(DWORD dwTimeout, bool bAddGlobalTime = true);
		DWORD				GetHorsesHoeTimeout() const { return m_dwHorsesHoeTimeout; }
		void				StartHorsesHoeTimeoutEvent();
		void				OnHorsesHoeTimeout();
		bool				NeedHorsesHoe() const;
		bool				IsHorsesHoeTimeout() const;

		void				SetHorseName(const std::string& c_rstName);
		const std::string&	GetHorseName() const		{ return m_stHorseName; }

		bool				IsHorseDead() const;
		bool				IsHorseSummoned() const { return m_pkHorseDeadEvent != NULL; }
		void				SetHorseElapsedTime(DWORD dwTime);
		DWORD				GetHorseElapsedTime() const;
		DWORD				GetHorseMaxLifeTime() const;
		void				StartHorseDeadEvent();
		void				StopHorseDeadEvent();
		void				OnHorseDeadEvent();
		bool				HorseRevive();
		bool				HorseFeed(int iPct, int iRagePct);

		bool				CanUseHorseSkill() const;

		LPCHARACTER			GetRider() const			{ return m_chRider; }
		void				SetRider(LPCHARACTER ch)	{ m_chRider = ch; }

		bool				IsRiding() const;

	private:
		CMountSystem*		m_pkMountSystem;
		BYTE				m_bTempMountState;
		BYTE				m_bHorseGrade;
		BYTE				m_bHorseRageLevel;
		short				m_sHorseRagePct;
		DWORD				m_dwHorseRageTimeout;
		LPEVENT				m_pkHorseRageTimeoutEvent;
		LPEVENT				m_pkHorseRageDecEvent;
		std::string			m_stHorseName;
		DWORD				m_dwHorseElapsedLifeTime;
		LPEVENT				m_pkHorseDeadEvent;
		DWORD				m_dwHorsesHoeTimeout;
		LPEVENT				m_pkHorsesHoeTimeoutEvent;

		BYTE				m_bShinings[SHINING_MAX_NUM];

	protected:
		LPCHARACTER			m_chRider;

		DWORD				m_dwMountVnum;
		DWORD				m_dwMountTime;


		////////////////////////////////////////////////////////////////////////////////////////
		// Detailed Log
	public:
		void				DetailLog() { m_bDetailLog = !m_bDetailLog; }
		void				ToggleMonsterLog();
		void				MonsterLog(const char* format, ...);
	private:
		bool				m_bDetailLog;
		bool				m_bMonsterLog;

#ifdef ENABLE_RUNE_SYSTEM
	public:
		void			SetRuneOwned(DWORD dwVnum);
		bool			IsRuneOwned(DWORD dwVnum) { return m_setRuneOwned.find(dwVnum) != m_setRuneOwned.end(); }

		bool			SetRunePage(const TRunePageData* pData);

		bool			IsRuneLoaded() const { return m_bRuneLoaded; }
		void			SetRuneLoaded();

		bool			IsValidRunePage(const TRunePageData* pData);

#ifdef ENABLE_RUNE_AFFECT_ICONS
		void			RuneAffectHelper(DWORD type, bool bAdd);
#endif
		void			GiveRuneBuff(DWORD dwVnum, bool bAdd);
		void			GiveRunePageBuff(bool bAdd = true);

		void			SendRunePacket(DWORD dwVnum);
		void			SendRunePagePacket();

		RuneCharData&	GetRuneData() { return m_runeData; }

		bool			IsRuneOnCD(BYTE index) { return m_runeCooldown[index] > std::chrono::high_resolution_clock::now(); };
		void			SetRuneCD(BYTE index, long long cd) { m_runeCooldown[index] = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(cd); }
		void			CancelRuneEvent(bool removeOnly = false);
		void			StartRuneEvent(bool isReset, long restartTime = 0);

		void			ResetSkillTime(WORD wVnum);
		void			GiveRunePermaBonus(bool force = false);

		bool			ResetRunes();

	private:
		LPEVENT			m_runeEvent;
		bool			m_bRuneLoaded;
		bool			m_bRunePermaBonusSet;
		std::set<DWORD>	m_setRuneOwned;
		TRunePageData	m_runePage;
		RuneCharData	m_runeData;
		std::map<BYTE, std::chrono::steady_clock::time_point> m_runeCooldown;
#endif

		////////////////////////////////////////////////////////////////////////////////////////
		// Empire

	public:
		void 				SetEmpire(BYTE bEmpire);
		BYTE				GetEmpire() const { return (GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX || GetMapIndex() == EVENT_LABYRINTH_MAP_INDEX) ? 1 : m_bEmpire; }
		BYTE				GetRealEmpire() const { return m_bEmpire; }

	protected:
		BYTE				m_bEmpire;

		////////////////////////////////////////////////////////////////////////////////////////
		// Regen
	public:
		void				SetRegen(LPREGEN pkRegen);

	protected:
		PIXEL_POSITION			m_posRegen;
		float				m_fRegenAngle;
		LPREGEN				m_pkRegen;
		size_t				regen_id_; // to help dungeon regen identification
		// End of Regen

		////////////////////////////////////////////////////////////////////////////////////////
		// Resists & Proofs
	public:
		bool				CannotMoveByAffect() const;	// Æ¯Á¤ È¿°ú¿¡ ÀÇÇØ ¿òÁ÷ÀÏ ¼ö ¾ø´Â »óÅÂÀÎ°¡?
		bool				IsImmune(DWORD dwImmuneFlag);
		void				SetImmuneFlag(DWORD dw) { m_pointsInstant.dwImmuneFlag = dw; }
		DWORD				GetImmuneFlag() const	{ return m_pointsInstant.dwImmuneFlag; }

		void				SetShining(BYTE slot, BYTE shiningID) { if (slot < SHINING_MAX_NUM) m_bShinings[slot] = shiningID; };
		BYTE				GetShining(BYTE slot) { return slot < SHINING_MAX_NUM ? m_bShinings[slot] : 0; };
		bool				HasShining(BYTE shiningID);

	protected:
		void				ApplyMobAttribute(const network::TMobTable* table);
		// End of Resists & Proofs

		////////////////////////////////////////////////////////////////////////////////////////
		// QUEST
		// 
	public:
		void				SetQuestNPCID(DWORD vid);
		DWORD				GetQuestNPCID() const { return m_dwQuestNPCVID; }
		LPCHARACTER			GetQuestNPC() const;

		void				SetQuestItemPtr(LPITEM item);
		void				ClearQuestItemPtr();
		LPITEM				GetQuestItemPtr() const;

		void				SetQuestBy(DWORD dwQuestVnum)	{ m_dwQuestByVnum = dwQuestVnum; }
		DWORD				GetQuestBy() const			{ return m_dwQuestByVnum; }

		int					GetQuestFlag(const std::string& flag) const;
		void				SetQuestFlag(const std::string& flag, int value);

		void				ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID);

	private:
		DWORD				m_dwQuestNPCVID;
		DWORD				m_dwQuestByVnum;
		LPITEM				m_pQuestItem;

		int					m_iQuestArgument;

	public:
		void				SetQuestArgument(int arg) { m_iQuestArgument = arg; };
		int					GetQuestArgument() { return m_iQuestArgument; };

		// Events
	public:
		bool				StartStateMachine(int iPulse = 1);
		void				StopStateMachine();
		void				UpdateStateMachine(DWORD dwPulse);
		void				SetNextStatePulse(int iPulseNext);

		// Ä³¸¯ÅÍ ÀÎ½ºÅÏ½º ¾÷µ¥ÀÌÆ® ÇÔ¼ö. ±âÁ¸¿£ ÀÌ»óÇÑ »ó¼Ó±¸Á¶·Î CFSM::Update ÇÔ¼ö¸¦ È£ÃâÇÏ°Å³ª UpdateStateMachine ÇÔ¼ö¸¦ »ç¿ëÇß´Âµ¥, º°°³ÀÇ ¾÷µ¥ÀÌÆ® ÇÔ¼ö Ãß°¡ÇÔ.
		void				UpdateCharacter(DWORD dwPulse);

	protected:
		DWORD				m_dwNextStatePulse;

		// Marriage
	public:
		LPCHARACTER			GetMarryPartner() const;
		void				SetMarryPartner(LPCHARACTER ch);
		int					GetMarriageBonus(DWORD dwItemVnum, bool bSum = true);

		void				SetWeddingMap(marriage::WeddingMap* pMap);
		marriage::WeddingMap* GetWeddingMap() const { return m_pWeddingMap; }

	private:
		marriage::WeddingMap* m_pWeddingMap;
		LPCHARACTER			m_pkChrMarried;

		// Warp Character
	public:
		void				StartWarpNPCEvent();

	public:
		void				StartSaveEvent();
		void				StartRecoveryEvent();
		void				StartCheckSpeedHackEvent();
		void				StartDestroyWhenIdleEvent();

		LPEVENT				m_pkDeadEvent;
		LPEVENT				m_pkStunEvent;
		LPEVENT				m_pkSaveEvent;
		LPEVENT				m_pkRecoveryEvent;
		LPEVENT				m_pkTimedEvent;
		LPEVENT				m_pkChannelSwitchEvent;
		LPEVENT				m_pkFishingEvent;
		LPEVENT				m_pkAffectEvent;
		LPEVENT				m_pkPoisonEvent;
		LPEVENT				m_pkFireEvent;
		LPEVENT				m_pkWarpNPCEvent;
		//DELAYED_WARP
		//END_DELAYED_WARP

		// MINING
		LPEVENT				m_pkMiningEvent;
		// END_OF_MINING
		LPEVENT				m_pkWarpEvent;
		LPEVENT				m_pkCheckSpeedHackEvent;
		LPEVENT				m_pkDestroyWhenIdleEvent;
#ifdef __PET_SYSTEM__
		LPEVENT				m_pkPetSystemUpdateEvent;
#endif

		void StartPolymorphEvent();
		void CancelPolymorphEvent();

		LPEVENT				m_PolymorphEvent;
		DWORD				m_dwPolymorphEventRace;

		bool IsWarping() const { return m_pkWarpEvent ? true : false; }

		bool				m_bHasPoisoned;

		const CMob *		m_pkMobData;
		CMobInstance *		m_pkMobInst;

		std::map<int, LPEVENT> m_mapMobSkillEvent;

		friend struct FuncSplashDamage;
		friend struct FuncSplashAffect;
		friend class CFuncShoot;

	public:
		int				GetPremiumRemainSeconds(BYTE bType) const;

	private:
		int				m_aiPremiumTimes[PREMIUM_MAX_NUM];

		// CHANGE_ITEM_ATTRIBUTES
		static const DWORD		msc_dwDefaultChangeItemAttrCycle;	///< µðÆúÆ® ¾ÆÀÌÅÛ ¼Ó¼ºº¯°æ °¡´É ÁÖ±â
		static const char		msc_szLastChangeItemAttrFlag[];		///< ÃÖ±Ù ¾ÆÀÌÅÛ ¼Ó¼ºÀ» º¯°æÇÑ ½Ã°£ÀÇ Quest Flag ÀÌ¸§
		static const char		msc_szChangeItemAttrCycleFlag[];		///< ¾ÆÀÌÅÛ ¼Ó¼ºº´°æ °¡´É ÁÖ±âÀÇ Quest Flag ÀÌ¸§
		// END_OF_CHANGE_ITEM_ATTRIBUTES

		// NEW_HAIR_STYLE_ADD
	public :
		bool ItemProcess_Hair(LPITEM item, int iDestCell);
		// END_NEW_HAIR_STYLE_ADD

	public :
		void ClearSkill();
		void ClearSubSkill();

		// RESET_ONE_SKILL
		bool ResetOneSkill(DWORD dwVnum);
		// END_RESET_ONE_SKILL

	private :
		void SendDamagePacket(LPCHARACTER pAttacker, int Damage, BYTE DamageFlag);

	// ARENA
	private :
		CArena *m_pArena;
		bool m_ArenaObserver;
		int m_nPotionLimit;

	public :
		void 	SetArena(CArena* pArena) { m_pArena = pArena; }
		void	SetArenaObserverMode(bool flag) { m_ArenaObserver = flag; }

		CArena* GetArena() const { return m_pArena; }
		bool	GetArenaObserverMode() const { return m_ArenaObserver; }

		void	SetPotionLimit(int count) { m_nPotionLimit = count; }
		int		GetPotionLimit() const { return m_nPotionLimit; }
	// END_ARENA

		//PREVENT_TRADE_WINDOW
	public:
		bool	IsOpenSafebox() const { return m_isOpenSafebox ? true : false; }
		void 	SetOpenSafebox(bool b) { m_isOpenSafebox = b; }

		int		GetSafeboxLoadTime() const { return m_iSafeboxLoadTime; }
		void	SetSafeboxLoadTime() { m_iSafeboxLoadTime = thecore_pulse(); }
		//END_PREVENT_TRADE_WINDOW
	private:
		bool	m_isOpenSafebox;

	public:
		int		GetSkillPowerByLevel(int level, bool bMob = false) const;
		
		//PREVENT_REFINE_HACK
		int		GetRefineTime() const { return m_iRefineTime; }
		void	SetRefineTime() { m_iRefineTime = thecore_pulse(); } 
		int		m_iRefineTime;
		//END_PREVENT_REFINE_HACK

		//RESTRICT_USE_SEED_OR_MOONBOTTLE
		int 	GetUseSeedOrMoonBottleTime() const { return m_iSeedTime; }
		void  	SetUseSeedOrMoonBottleTime() { m_iSeedTime = thecore_pulse(); }
		int 	m_iSeedTime;
		//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
		
		//PREVENT_PORTAL_AFTER_EXCHANGE
		int		GetExchangeTime() const { return m_iExchangeTime; }
		void	SetExchangeTime() { m_iExchangeTime = thecore_pulse(); }
		int		m_iExchangeTime;
		//END_PREVENT_PORTAL_AFTER_EXCHANGE
		
		int 	m_iMyShopTime;
		int		GetMyShopTime() const	{ return m_iMyShopTime; }
		void	SetMyShopTime() { m_iMyShopTime = thecore_pulse(); }

		// Hack ¹æÁö¸¦ À§ÇÑ Ã¼Å©.
		bool	IsHack(bool bSendMsg = true, bool bCheckShopOwner = true, int limittime = g_nPortalLimitTime);

		void Say(const std::string & s);

	public:
		bool ItemProcess_Polymorph(LPITEM item);

		// by mhh
		DWORD*	GetCubeItemID() { return m_pointsInstant.pCubeItemIDs; }
		bool IsCubeOpen () const	{ return (m_pointsInstant.pCubeNpc?true:false); }
		void SetCubeNpc(LPCHARACTER npc)	{ m_pointsInstant.pCubeNpc = npc; }
		bool CanDoCube() const;

	private:
		//Áß±¹ Àü¿ë
		//18¼¼ ¹Ì¸¸ Àü¿ë
		//3½Ã°£ : 50 % 5 ½Ã°£ 0%
		e_overtime m_eOverTime;

	public:
		bool IsOverTime(e_overtime e) const { return (e == m_eOverTime); }
		void SetOverTime(e_overtime e) { m_eOverTime = e; }

	private:
		int		m_deposit_pulse;

	public:
		void	UpdateDepositPulse();
		bool	CanDeposit() const;

		bool	__CanMakePrivateShop();

	private:
		void	__OpenPrivateShop();

	public:
		struct AttackedLog
		{
			DWORD 	dwPID;
			DWORD	dwAttackedTime;
			
			AttackedLog() : dwPID(0), dwAttackedTime(0)
			{
			}
		};

		AttackLog	m_kAttackLog;
		AttackedLog m_AttackedLog;
		int			m_speed_hack_count;

	private :
		std::string m_strNewName;

	public :
		const std::string GetNewName() const { return this->m_strNewName; }
		void SetNewName(const std::string name) { this->m_strNewName = name; }

	public :
		void GoHome();

	private :
		std::set<DWORD>	m_known_guild;

	public :
		void SendGuildName(CGuild* pGuild);
		void SendGuildName(DWORD dwGuildID);

	private :
		DWORD m_dwLogOffInterval;

	public :
		DWORD GetLogOffInterval() const { return m_dwLogOffInterval; }

#ifdef AHMET_FISH_EVENT_SYSTEM
	private:
		DWORD m_dwFishUseCount;
		BYTE m_bFishAttachedShape;
	public:
		DWORD GetFishEventUseCount() const
		{
			return m_dwFishUseCount;
		}
		void FishEventIncreaseUseCount()
		{
			m_dwFishUseCount++;
		}

		BYTE GetFishAttachedShape() const
		{
			return m_bFishAttachedShape;
		}
		void SetFishAttachedShape(BYTE bShape)
		{
			m_bFishAttachedShape = bShape;
		}

		void SendFishInfoAsCommand(BYTE bSubHeader, DWORD dwFirstArg, DWORD dwSecondArg = 0);
#endif

	public:
		bool UnEquipSpecialRideUniqueItem ();

		bool CanWarp () const;

	private:
		DWORD m_dwLastGoldDropTime;

	public:
		void AutoRecoveryItemProcess (const EAffectTypes);

	public:
		void BuffOnAttr_AddBuffsFromItem(LPITEM pItem);
		void BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem);

	private:
		void BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue);
		void BuffOnAttr_ClearAll();

		typedef std::map <BYTE, CBuffOnAttributes*> TMapBuffOnAttrs;
		TMapBuffOnAttrs m_map_buff_on_attrs;
		// ¹«Àû : ¿øÈ°ÇÑ Å×½ºÆ®¸¦ À§ÇÏ¿©.
	public:
		void SetArmada() { cannot_dead = true; }
		void ResetArmada() { cannot_dead = false; }
	private:
		bool cannot_dead;

#ifdef __PET_SYSTEM__
	private:
		bool m_bIsPet;
		CPetSystem*	m_petSystem;
		CPetActor* m_petOwnerActor;

	public:
		void SetPet(CPetActor* pPetOwnerActor) { m_bIsPet = true; m_petOwnerActor = pPetOwnerActor; }
#ifdef __PET_ADVANCED__
		bool IsPet() const { return !IsPC() && (m_bIsPet || GetPetAdvanced() != NULL); }
#else
		bool IsPet() const { return m_bIsPet; }
#endif
		CPetSystem*	GetPetSystem() { return m_petSystem; }
#endif

#ifdef __SKIN_SYSTEM__
		void ResummonPet();
		void ResummonMount();
#endif

	//ÃÖÁ¾ µ¥¹ÌÁö º¸Á¤.
	private:
		float m_fAttMul;
		float m_fDamMul;
	public:
		float GetAttMul() { return this->m_fAttMul; }
		void SetAttMul(float newAttMul) {this->m_fAttMul = newAttMul; }
		float GetDamMul() { return this->m_fDamMul; }
		void SetDamMul(float newDamMul) {this->m_fDamMul = newDamMul; }

	private:
		bool IsValidItemPosition(TItemPos Pos) const;

		//µ¶ÀÏ ¼±¹° ±â´É ÆÐÅ¶ ÀÓ½Ã ÀúÀå
	private:
		unsigned int itemAward_vnum;
		char		 itemAward_cmd[20];
		//bool		 itemAward_flag;
	public:
		unsigned int GetItemAward_vnum() { return itemAward_vnum; }
		char*		 GetItemAward_cmd() { return itemAward_cmd;	  }
		//bool		 GetItemAward_flag() { return itemAward_flag; }
		void		 SetItemAward_vnum(unsigned int vnum) { itemAward_vnum = vnum; }
		void		 SetItemAward_cmd(const char* cmd) { strcpy(itemAward_cmd,cmd); }
		//void		 SetItemAward_flag(bool flag) { itemAward_flag = flag; }

	private:
		// SyncPositionÀ» ¾Ç¿ëÇÏ¿© Å¸À¯Àú¸¦ ÀÌ»óÇÑ °÷À¸·Î º¸³»´Â ÇÙ ¹æ¾îÇÏ±â À§ÇÏ¿©,
		// SyncPositionÀÌ ÀÏ¾î³¯ ¶§¸¦ ±â·Ï.
		timeval		m_tvLastSyncTime;
		int			m_iSyncHackCount;
	public:
		void			SetLastSyncTime(const timeval &tv) { thecore_memcpy(&m_tvLastSyncTime, &tv, sizeof(timeval)); }
		const timeval&	GetLastSyncTime() { return m_tvLastSyncTime; }
		void			SetSyncHackCount(int iCount) { m_iSyncHackCount = iCount;}
		int				GetSyncHackCount() { return m_iSyncHackCount; }

		bool			IsNearWater() const;

	private:
		bool		m_bIsEXPDisabled;
	public:
		bool		IsEXPDisabled() const { return m_bIsEXPDisabled; }
		void		SetEXPDisabled() { m_bIsEXPDisabled = true; SetPoint(POINT_ANTI_EXP, 1); PointChange(POINT_ANTI_EXP, 0); }
		void		SetEXPEnabled() { m_bIsEXPDisabled = false; SetPoint(POINT_ANTI_EXP, 0); PointChange(POINT_ANTI_EXP, 0); }

	public:
		bool		IsPrivateMap(long lMapIndex = 0) const;
		void		SetNoPVPPacketMode(bool bMode) { if (bMode == m_bNoPacketPVP) return; m_bNoPacketPVP = bMode; m_bNoPacketPVPModeChange = true; UpdateSectree(); }

	public:
		void		SetPVPTeam(short sTeam);
		short		GetPVPTeam() const { return m_sPVPTeam; }
	private:
		short		m_sPVPTeam;

#ifdef __IPV6_FIX__
	public:
		void		SetIPV6FixEnabled() { m_bIPV6FixEnabled = true; }

	private:
		bool		m_bIPV6FixEnabled;
#endif

	public:
		BYTE		GetLanguageID() const;

	public:
		void		SetIsShowTeamler(bool bIsShowTeamler);
		bool		IsShowTeamler() const { return m_bIsShowTeamler; }
		void		ShowTeamlerPacket();
		void		GlobalShowTeamlerPacket(bool bIsOnline);
		void		SetHaveToRemoveFromMgr(bool isSet) { m_bHaveToRemoveFromMgr = isSet; }
	private:
		bool		m_bIsShowTeamler;
		bool		m_bHaveToRemoveFromMgr;

#ifdef __ACCE_COSTUME__
	public:
		void		AcceRefineCheckin(BYTE acceSlot, TItemPos currentCell);
		void		AcceRefineCheckout(BYTE acceSlot);
		void		AcceRefineAccept(int windowType);
		void		AcceRefineCancel();
		void		AcceClose();

	 private:
		 BYTE m_AcceWindowType;

	 public:

		 void SetAcceWindowType(int windowType) { m_AcceWindowType = windowType; }
		 int GetAcceWindowType() { return m_AcceWindowType; }
		 bool IsAcceWindowOpen() const { return m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] != WORD_MAX || m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT] != WORD_MAX; }
#endif

	public:
		void			LoadOfflineMessages(const ::google::protobuf::RepeatedPtrField<network::TOfflineMessage>& elements);
		void			SendOfflineMessages();
		void			SaveOfflineMessages();
	private:
		std::vector<network::TOfflineMessage>	m_vec_OfflineMessages;

	public:
		bool			IsPurgeable() const;
		bool			IsEnemy() const;
		
	public:
		void				SetForceMonsterAttackRange(DWORD dwRange)	{ m_dwForceMonsterAttackRange = dwRange; }
		DWORD				GetForceMonsterAttackRange() const			{ return m_dwForceMonsterAttackRange; }
	private:
		DWORD				m_dwForceMonsterAttackRange;

#ifdef __ITEM_REFUND__
	public:
		void			AddItemRefundCommand(const char *format, ...);
		void			SendItemRefundCommands();
	private:
		std::vector<std::string>	m_vec_ItemRefundCommands;
#endif

	public:
		bool	ChangeChannel(long lNewChannelHost, int iNewChannel);

	public:
		WORD	GetInventoryMaxNum() const { return MINMAX(INVENTORY_MAX_NUM_MIN, m_wInventoryMaxNum, INVENTORY_MAX_NUM); }
		void	SetInventoryMaxNum(WORD wMaxNum, BYTE invType);

		WORD	GetUppitemInventoryMaxNum() const { return MINMAX(UPPITEM_INV_MIN_NUM, m_wUppitemInventoryMaxNum, UPPITEM_INV_MAX_NUM); }
		WORD	GetSkillbookInventoryMaxNum() const { return MINMAX(SKILLBOOK_INV_MIN_NUM, m_wSkillbookInventoryMaxNum, SKILLBOOK_INV_MAX_NUM); }
		WORD	GetStoneInventoryMaxNum() const { return MINMAX(STONE_INV_MIN_NUM, m_wStoneInventoryMaxNum, STONE_INV_MAX_NUM); }
		WORD	GetEnchantInventoryMaxNum() const { return MINMAX(ENCHANT_INV_MIN_NUM, m_wEnchantInventoryMaxNum, ENCHANT_INV_MAX_NUM); }

	private:
		WORD	m_wInventoryMaxNum;
		WORD	m_wUppitemInventoryMaxNum;
		WORD	m_wSkillbookInventoryMaxNum;
		WORD	m_wStoneInventoryMaxNum;
		WORD	m_wEnchantInventoryMaxNum;

	public:
#ifdef INCREASE_ITEM_STACK
		bool	DestroyItem(TItemPos pos, WORD num);
#else
		bool	DestroyItem(TItemPos pos, BYTE num);
#endif

#ifdef __SWITCHBOT__
	private:
		DWORD								m_dwSwitchbotSpeed;
		LPEVENT								m_pkSwitchbotEvent;
		std::map<DWORD, network::TSwitchbotTable>	m_map_Switchbots;
	public:
		void			StartSwitchbotEvent();
		void			ChangeSwitchbotSpeed(DWORD dwNewMS);
		void			AppendSwitchbotData(const network::TSwitchbotTable* c_pkSwitchbot);
		void			AppendSwitchbotDataPremium(const network::TSwitchbotTable* c_pkSwitchbot);
		void			RemoveSwitchbotData(DWORD dwItemID);
		void			RemoveSwitchbotDataBySlot(WORD wSlot);
		DWORD			OnSwitchbotEvent();
#endif

#ifdef __ATTRIBUTES_TO_CLIENT__
	public:
		void			SendAttributesToClient();
		void			SendAttributesToClient(BYTE bItemType, char cItemSubType, BYTE bAttrSetIndex);
#endif

#ifdef __VOTE4BUFF__
	public:
		void	V4B_Initialize();
		void	V4B_SetLoaded();
		bool	V4B_IsLoaded() const { return m_bV4B_Loaded; }
		void	V4B_GiveBuff(DWORD dwTimeEnd, BYTE bApplyType, int iApplyValue);
		bool	V4B_IsBuff() const;
	private:
		void	V4B_AddAffect();

	private:
		bool	m_bV4B_Loaded;
		DWORD	m_dwV4B_TimeEnd;
		BYTE	m_bV4B_ApplyType;
		int		m_iV4B_ApplyValue;
#endif

#ifdef __FAKE_PC__
	public:
		// init / destroy
		void							FakePC_Load(LPCHARACTER pkOwner, LPITEM pkSpawnItem = NULL);
		void							FakePC_Destroy();

		// set / get
		LPCHARACTER						FakePC_GetOwner()			{ return m_pkFakePCOwner; }
		const LPCHARACTER				FakePC_GetOwner() const		{ return m_pkFakePCOwner; }
		LPITEM							FakePC_GetOwnerItem()		{ return m_pkFakePCSpawnItem; }
		bool							FakePC_Check() const		{ return m_pkFakePCOwner != NULL; }
		bool							FakePC_IsSupporter() const	{ return FakePC_Check() && GetPVPTeam() != SHRT_MAX; }
		float							FakePC_GetDamageFactor()	{ return m_fFakePCDamageFactor; }

		void							FakePC_Owner_SetName(const char* szName)	{ m_stFakePCName = szName; }
		const char*						FakePC_Owner_GetName() const				{ return m_stFakePCName.c_str(); }

		void							FakePC_Owner_ResetAfkEvent();
		void							FakePC_Owner_ClearAfkEvent();

		bool							FakePC_CanAddAffect(CAffect* pkAff);
		void							FakePC_AddAffect(CAffect* pkAff);
		void							FakePC_RemoveAffect(CAffect* pkAff);

		void							FakePC_Owner_AddSpawned(LPCHARACTER pkFakePC, LPITEM pkSpawnItem = NULL);
		bool							FakePC_Owner_RemoveSpawned(LPCHARACTER pkFakePC);
		bool							FakePC_Owner_RemoveSpawned(LPITEM pkSpawnItem);
		LPCHARACTER						FakePC_Owner_GetSpawnedByItem(LPITEM pkItem);

		LPCHARACTER						FakePC_Owner_GetSupporter();
		LPCHARACTER						FakePC_Owner_GetSupporterByItem(LPITEM pkItem);
		DWORD							FakePC_Owner_CountSummonedByItem();

		void							FakePC_Owner_AddAffect(CAffect* pkAff);
		void							FakePC_Owner_RemoveAffect(CAffect* pkAff);

		// compute
		void							FakePC_Owner_ApplyPoint(BYTE bType, int lValue);
		void							FakePC_Owner_ItemPoints(LPITEM pkItem, bool bAdd);
		void							FakePC_Owner_MountBuff(bool bAdd);
		void							FakePC_ComputeDamageFactor();

		// spawn
		LPCHARACTER						FakePC_Owner_Spawn(int lX, int lY, LPITEM pkItem = NULL, bool bIsEnemy = false, bool bIsRedPotionEnabled = true);
		void							FakePC_Owner_DespawnAll();
		bool							FakePC_Owner_DespawnAllSupporter();
		bool							FakePC_Owner_DespawnByItem(LPITEM pkItem);

		// attack
		void							FakePC_Owner_ForceFocus(LPCHARACTER pkVictim);
		BYTE							FakePC_ComputeComboIndex();

		void							FakePC_SetNoAttack()		{ m_bIsNoAttackFakePC = true; }
		bool							FakePC_CanAttack() const	{ return !m_bIsNoAttackFakePC; }

		// skills
		bool							FakePC_IsSkillNeeded(CSkillProto* pkSkill);
		bool							FakePC_IsBuffSkill(DWORD dwVnum);
		bool							FakePC_UseSkill(LPCHARACTER pkTarget = NULL);
		void							FakePC_SendSkillPacket(CSkillProto* pkSkill);

		// exec func for all fake pcs
		void							FakePC_Owner_ExecFunc(void(CHARACTER::* func)());

	private:
		std::string						m_stFakePCName;

		LPEVENT							m_pkFakePCAfkEvent;

		LPCHARACTER						m_pkFakePCOwner;
		std::set<LPCHARACTER>			m_set_pkFakePCSpawns;

		LPITEM							m_pkFakePCSpawnItem;
		std::map<LPITEM, LPCHARACTER>	m_map_pkFakePCSpawnItems;

		std::map<CAffect*, CAffect*>	m_map_pkFakePCAffects;

		float							m_fFakePCDamageFactor;

		bool							m_bIsNoAttackFakePC;
#endif

#ifdef __DRAGONSOUL__
	public:
		void	DragonSoul_Initialize();

		bool	DragonSoul_IsQualified() const;
		void	DragonSoul_GiveQualification();

		int		DragonSoul_GetActiveDeck() const;
		bool	DragonSoul_IsDeckActivated() const;
		bool	DragonSoul_ActivateDeck(int deck_idx);

		void	DragonSoul_DeactivateAll();
		void	DragonSoul_CleanUp();
	public:
		bool		DragonSoul_RefineWindow_Open(LPENTITY pEntity);
		bool		DragonSoul_RefineWindow_Close();
		LPENTITY	DragonSoul_RefineWindow_GetOpener() { return  m_pointsInstant.m_pDragonSoulRefineWindowOpener; }
		bool		DragonSoul_RefineWindow_CanRefine();
#endif

#ifdef COMBAT_ZONE
	private:
		DWORD m_iCombatZonePoints;
		DWORD m_iCombatZoneDeaths;
		DWORD m_dwCombatZonePoints;
		BYTE  m_bCombatZoneRank;

	public:
		LPEVENT m_pkCombatZoneLeaveEvent;
		LPEVENT m_pkCombatZoneWarpEvent;

		BYTE	GetCombatZoneRank();

		DWORD	GetRealCombatZonePoints() { return MAX(m_dwCombatZonePoints, 0); };
		void	SetRealCombatZonePoints(DWORD dwValue) { m_dwCombatZonePoints += MAX(m_iCombatZonePoints,0); };

		void	UpdateCombatZoneRankings(const char* memberName, DWORD memberEmpire, DWORD memberPoints);

		DWORD	GetCombatZonePoints() { return MAX(m_iCombatZonePoints, 0); }
		void	SetCombatZonePoints(DWORD dwValue) { m_iCombatZonePoints = MAX(dwValue, 0); }
		
		DWORD	GetCombatZoneDeaths() { return m_iCombatZoneDeaths; }
		void	SetCombatZoneDeaths(DWORD dwValue) { m_iCombatZoneDeaths = dwValue; }
#endif

#ifdef __GAYA_SYSTEM__
	private:
		DWORD	m_dwGaya;

	public:
		DWORD	GetGaya() const { return m_dwGaya; }
		void	ChangeGaya(int iChange) { if (iChange < 0 && abs(iChange) > GetGaya()) iChange = -(int)GetGaya(); SetGaya(GetGaya() + iChange); }

		void	SetGaya(DWORD dwGaya);
#endif

	public:
		network::TAccountTable*	GetAccountTablePtr() const;
		network::TAccountTable&	GetAccountTable() const;
		DWORD GetAuraApplyByPoint(DWORD dwPoint);
		void ClearAuraBuffs();
		void ClearGivenAuraBuffs();
		void StartUpdateAuraEvent(bool bIsRestart = false);
		void Event_UpdateAura();
		void UpdateAuraByPoint(DWORD dwPoint);
	private:
		typedef std::set<DWORD> TAuraAffectSet;
		typedef std::map<LPCHARACTER, TAuraAffectSet> TAuraBuffMap;
		typedef std::set<LPCHARACTER> TAuraBuffedPlayerSet;
		TAuraBuffMap			m_map_AuraBuffBonus;
		TAuraBuffedPlayerSet	m_set_AuraBuffedPlayer;
		LPEVENT					m_pkAuraUpdateEvent;
	public:
		const char*	GetEscapedHWID();
		bool IsPacketLoggingEnabled();

	public:
		//PREVENT_GOLD_HACK
		int		GetGoldTime() const { return m_iGoldActionTime; }
		void	SetGoldTime() { m_iGoldActionTime = thecore_pulse(); }
	private:
		int		m_iGoldActionTime;
		//END_PREVENT_GOLD_HACK
		
   public:
        struct S_CARD
        {
            DWORD    type;
            DWORD    value;
        };

        struct CARDS_INFO
        {
            S_CARD cards_in_hand[MAX_CARDS_IN_HAND];
            S_CARD cards_in_field[MAX_CARDS_IN_FIELD];
            DWORD    cards_left;
            DWORD    field_points;
            DWORD    points;
        };
    
        void            Cards_open(DWORD safemode);
        void            Cards_clean_list();
        int            GetEmptySpaceInHand();
        void            Cards_pullout();
        void            RandomizeCards();
        bool            CardWasRandomized(DWORD type, DWORD value);
        void            SendUpdatedInformations();
        void            SendReward();
        void            CardsDestroy(DWORD reject_index);
        void            CardsAccept(DWORD accept_index);
        void            CardsRestore(DWORD restore_index);
        int            GetEmptySpaceInField();
        DWORD            GetAllCardsCount();
        bool            TypesAreSame();
        bool            ValuesAreSame();
        bool            CardsMatch();
        DWORD            GetLowestCard();
        bool            CheckReward();
        void            CheckCards();
        void            RestoreField();
        void            ResetField();
        void            CardsEnd();
        void            GetGlobalRank(char * buffer, size_t buflen);
        void            GetRundRank(char * buffer, size_t buflen);
    protected:
        CARDS_INFO    character_cards;
		S_CARD    randomized_cards[24];

	public:
		void	ResetDataChanged(BYTE bIndex) { m_abPlayerDataChanged[bIndex] = false; }
	private:
		bool	m_abPlayerDataChanged[PC_TAB_CHANGED_MAX_NUM];

#ifdef __PYTHON_REPORT_PACKET__
	public:
		void	DetectionHackLog(const char* c_pszType, const char* c_pszDetail);
	private:
		std::map<std::string, DWORD>	m_mapLastDetectionLog;
#endif		

#ifdef __FAKE_BUFF__
	public:
		// init / destroy
		void							FakeBuff_Load(LPCHARACTER pkOwner, LPITEM pkSpawnItem);
		void							FakeBuff_Destroy();

		// set / get
		bool							FakeBuff_Check() const		{ return m_pkFakeBuffOwner != NULL; }
		LPCHARACTER						FakeBuff_GetOwner()			{ return m_pkFakeBuffOwner; }
		const LPCHARACTER				FakeBuff_GetOwner() const	{ return m_pkFakeBuffOwner; }
		LPITEM							FakeBuff_GetItem()			{ return m_pkFakeBuffItem; }
		LPCHARACTER						FakeBuff_Owner_GetSpawn()	{ return m_pkFakeBuffSpawn; }

		void							FakeBuff_Owner_SetName(const char* szName)	{ m_stFakeBuffName = szName; }
		const char*						FakeBuff_Owner_GetName() const				{ return m_stFakeBuffName.c_str(); }

		void							FakeBuff_SetItem(LPITEM pkItem)				{ m_pkFakeBuffItem = pkItem; }
		void							FakeBuff_Owner_SetSpawn(LPCHARACTER pkChr)	{ m_pkFakeBuffSpawn = pkChr; }

		DWORD							FakeBuff_GetPart(BYTE bPart) const;

		// spawn
		LPCHARACTER						FakeBuff_Owner_Spawn(int lX, int lY, LPITEM pkItem);
		bool							FakeBuff_Owner_Despawn();
		void							FakeBuff_Local_Warp(long lMapIndex, int lX, int lY);

		// skills
		bool							FakeBuff_IsSkillNeeded(CSkillProto* pkSkill);
		bool							FakeBuff_UseSkill(LPCHARACTER pkTarget = NULL);
		void							FakeBuff_SendSkillPacket(CSkillProto* pkSkill);

		// new
		BYTE							GetFakeBuffSkillIdx(DWORD dwSkillVnum) const;
		DWORD							GetFakeBuffSkillVnum(BYTE bIndex) const;
		void							SetFakeBuffSkillLevel(DWORD dwSkillVnum, BYTE bLevel);
		BYTE							GetFakeBuffSkillLevel(DWORD dwSkillVnum) const;
		bool							CanFakeBuffSkillUp(DWORD dwSkillVnum, bool bUseBook);
		void							SendFakeBuffSkills(DWORD dwSkillVnum = 0);

	private:
		BYTE							m_abFakeBuffSkillLevel[3];
		std::string						m_stFakeBuffName;
		LPCHARACTER						m_pkFakeBuffOwner;
		LPCHARACTER						m_pkFakeBuffSpawn;
		LPITEM							m_pkFakeBuffItem;
#endif

#ifdef __ATTRTREE__
	public:
		void	SetAttrTreeLevel(BYTE row, BYTE col, BYTE level);
		BYTE	GetAttrTreeLevel(BYTE row, BYTE col) const;

		void	GiveAttrTreeBonus(bool bAdd = true);
		void	GiveAttrTreeBonus(BYTE row, BYTE col, bool bAdd = true);

		void	SendAttrTree();
		void	SendAttrTreeLevel(BYTE row, BYTE col);

		const network::TRefineTable*	GetAttrTreeRefine(BYTE id) const;

	private:
		BYTE	m_aAttrTree[ATTRTREE_ROW_NUM][ATTRTREE_COL_NUM];
#endif

	public:
		void	GetDataByAbilityUp(DWORD dwItemVnum, int& iEffectPacket, DWORD& dwAffType, WORD& wPointType, DWORD& dwAffFlag);

	public:
		void	RemoveItemBuffs(network::TItemTable* pItemTable);
		void	CheckForDisabledItemBuffs(const std::set<DWORD>* pDisabledList = NULL);
		void	CheckForDisabledItems(const std::set<DWORD>* pDisabledList);
		void	CheckForDisabledItems(bool bOnlyCheckIfListExists = true);
		
	public:
		void	SetDungeonComplete(BYTE bDungeon) { bDungeonComplete = bDungeon; }
		BYTE	GetDungeonComplete() { return bDungeonComplete; }
		
		void	SetItemDropQuest(int iVnum) { iItemDropQuest = iVnum; iLastItemDropQuest = iVnum; }
		int	GetItemDropQuest() { return iItemDropQuest; }
		int	GetLastItemDropQuest() { return iLastItemDropQuest; }
		
	private:
		BYTE	bDungeonComplete;
		int		iItemDropQuest;
		int		iLastItemDropQuest;
		
	public:
		DWORD	GetCurrentHair() const;

#ifdef __HAIR_SELECTOR__
	private:
		DWORD	m_dwSelectedHairPart;
#endif

	public:
		void	SetUsedRenameGuildItem(int iPos) { m_iUsedRenameGuildItem = iPos; }
		int		GetUsedRenameGuildItem() const { return m_iUsedRenameGuildItem; }
	private:
		int		m_iUsedRenameGuildItem;
		
	public:
		void		StopAutoRecovery(const EAffectTypes type);

#ifdef __COSTUME_BONUS_TRANSFER__
	private:
		bool		m_bIsOpenedCostumeBonusTransferWindow;
		TItemPos	m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MAX];
	public:
		bool		CBT_IsWindowOpen() const { return m_bIsOpenedCostumeBonusTransferWindow; }
		void		CBT_WindowOpen(LPENTITY pOpener);
		LPENTITY	CBT_GetWindowOpener() { return  m_pointsInstant.m_pCBTWindowOpener; }
		void		CBT_WindowClose();
		bool		CBT_CanAct();
		void		CBT_CheckIn(BYTE CBTPos, TItemPos ItemCell);
		void		CBT_CheckOut(BYTE CBTPos);
		void		CBT_Accept();
#endif

	public:
		void	SetCostumeHide(BYTE bSlot, bool bStatus) { m_bHideCostume[bSlot] = bStatus; }
		bool	IsCostumeHide(BYTE bSlot) { return m_bHideCostume[bSlot]; }
	private:
		bool	m_bHideCostume[HIDE_COSTUME_MAX];
		
	public:
		bool	IsKillcountChanged() { return m_bKillcounterStatsChanged; }
	private:
		bool	m_bKillcounterStatsChanged;
		
#ifdef ENABLE_XMAS_EVENT
	public:
		void	SetXmasEventPulse() { m_iXmasEventPulse = thecore_pulse(); }
		int		GetXmasEventPulse() const { return m_iXmasEventPulse; }
	private:
		int		m_iXmasEventPulse;
#endif

#ifdef ENABLE_WARP_BIND_RING
	public:
		void	SetWarpBindPulse() { m_iWarpBindPulse = thecore_pulse(); }
		int		GetWarpBindPulse() const { return m_iWarpBindPulse; }
	private:
		int		m_iWarpBindPulse;
#endif

#ifdef __TRADE_BLOCK_SYSTEM__
	public:
		bool	IsTradeBlocked() { return GetQuestFlag("trade_blocked.x") > 0; }
#endif

#ifdef ENABLE_RUNE_PAGES
	public:
		void	SaveSelectedRunes();
		void	LoadSelectedRunes();
		TRunePageData	m_selectedRunes[RUNE_PAGE_COUNT];
#endif
#ifdef SORT_AND_STACK_ITEMS
	public:
		void	SetSortingPulse() { m_iSortingPulse = thecore_pulse(); }
		int		GetSortingPulse() const { return m_iSortingPulse; }
		void	SetStackingPulse() { m_iStackingPulse = thecore_pulse(); }
		int		GetStackingPulse() const { return m_iStackingPulse; }
	private:
		int		m_iSortingPulse;
		int		m_iStackingPulse;
#endif
#ifdef CHANGE_SKILL_COLOR
	public:
		void	SetSkillColor(DWORD * dwSkillColor);
		void	SetSkillColor(const ::google::protobuf::RepeatedPtrField<::google::protobuf::RepeatedField<google::protobuf::uint32>>& skillColor);
		DWORD*	GetSkillColor() { return m_dwSkillColor[0]; }
	protected:
		DWORD	m_dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif
#ifdef ENABLE_COMPANION_NAME
	public:
		void	SaveCompanionNames();
		void	LoadCompanionNames();
		std::string	GetPetName() { return m_stPetName; };
		std::string	GetMountName() { return m_stMountName; };
		void	SetCompanionHasName(bool hasName) { m_companionHasName = hasName; } // could maybe instead set owner and get the name
		void	SetPetName(const char *name);
		void	SetMountName(const char *name);
		void	SetFakebuffName(const char *name);
		void	SendCompanionNameInfo();
	private:
		DWORD		m_petNameTimeLeft;
		std::string	m_stPetName;
		DWORD		m_mountNameTimeLeft;
		std::string	m_stMountName;
		bool		m_companionHasName;
#endif
	public:
		void	SetIsHacker() {	m_bIsHacker = true; }
		bool	IsHacker() { return m_bIsHacker; }
	private:
		bool	m_bIsHacker;
#ifdef LEADERSHIP_EXTENSION
	public:
		void	SetLeadershipState(BYTE state);
#endif
#ifdef BATTLEPASS_EXTENSION
	public: // WIP
		network::TBattlepassData	m_battlepassData[10];
		void	SendBattlepassData(int index);
#endif
#ifdef KEEP_SKILL_AFFECTS
	private:
		bool	m_isDestroyed;
#endif

#ifdef CHECK_TIME_AFTER_PVP
	private:
		DWORD	m_dwLastAttackTimePVP;
		DWORD	m_dwLastTimeAttackedPVP;
	public:
		DWORD	GetLastAttackTimePVP() const { return m_dwLastAttackTimePVP; }
		DWORD	GetLastTimeAttackedPVP() const { return m_dwLastTimeAttackedPVP; }
		bool	IsPVPFighting(int seconds = 3) const { return get_dword_time() - GetLastAttackTimePVP() <= seconds * 1000 || get_dword_time() - GetLastTimeAttackedPVP() <= seconds * 1000; };
#endif

#ifdef __EQUIPMENT_CHANGER__
	public:
		void					SetEquipmentChangerPageIndex(DWORD dwPageIndex, bool bForce = false);
		DWORD					GetEquipmentChangerPageIndex() const { return m_dwEquipmentChangerPageIndex; }
		void					LoadEquipmentChanger(const ::google::protobuf::RepeatedPtrField<network::TEquipmentChangerTable>& tables);
		void					SendEquipmentChangerLoadPacket(bool bForce = false);
		void					AddEquipmentChangerPage(const char* szName);
		void					DeleteEquipmentChangerPage(DWORD dwIndex);
		// void					UpdateEquipmentChangerItem(BYTE bWearSlot);
		void					SetEquipmentChangerName(DWORD dwIndex, const char* szPageName);
		void					SaveEquipmentChanger();
		void					SetEquipmentChangerLastChange(DWORD dwLastChange)	{ m_dwEquipmentChangerLastChange = dwLastChange; }
		DWORD					GetEquipmentChangeLastChange() const				{ return m_dwEquipmentChangerLastChange; }
	private:
		typedef std::vector<network::TEquipmentChangerTable> TEquipmentChangerVector;
		bool					m_bIsEquipmentChangerLoaded;
		DWORD					m_dwEquipmentChangerPageIndex;
		TEquipmentChangerVector	m_vec_EquipmentChangerPages;
		DWORD					m_dwEquipmentChangerLastChange;
#endif

#ifdef BLACKJACK
	private:
		typedef std::vector<TBlackJackCard> TBlackJackDeck;
		TBlackJackDeck			m_vec_BlackJackDeck;
		TBlackJackDeck			m_vec_BlackJackPcHand;
		TBlackJackDeck			m_vec_BlackJackDealerHand;
		BYTE					bBlackJackStatus;
		BYTE					bStake;

	public:
		void					BlackJack_Start(BYTE stake);
		bool					BlackJack_DealerThinking();
		void					BlackJack_Check(bool end);
		void					BlackJack_Turn(BOOL hit, BYTE cdNum, BOOL pc_stay=false);
		void					BlackJack_PopCard();
		BYTE					BlackJack_Count(TBlackJackDeck m_hand);
		void					BlackJack_Reward();
		void					BlackJack_End();

#endif
	public:
		bool					IsIgnorePenetrateCritical();

#ifdef DMG_RANKING
	private:
		BYTE	m_dummyHitCount;
		DWORD	m_dummyHitStartTime;
		int		m_totalDummyDamage;
	public:
		void	registerDamageToDummy(const EDamageType &type, const int &dmg);
#endif

#ifdef __PRESTIGE__
	public:
		void	Prestige_SetLevel(BYTE bLevel);
#ifdef __FAKE_PC__
		BYTE	Prestige_GetLevel() const { return FakePC_Check() ? FakePC_GetOwner()->Prestige_GetLevel() : m_bPrestigeLevel; }
#else
		BYTE	Prestige_GetLevel() const { return m_bPrestigeLevel; }
#endif
	private:
		BYTE		m_bPrestigeLevel;
#endif
	public:
		bool		check_item_sex(LPCHARACTER ch, LPITEM item);
		
	public:
		void		SetBraveCapeLastPulse(int iPulse)	{ m_BraveCapePulse = iPulse; }

		int			GetBraveCapeLastPulse() { return m_BraveCapePulse; }

	private:
		int			m_BraveCapePulse;

#ifdef __PET_ADVANCED__
	private:
		CPetAdvanced*	m_petAdvanced;
	public:
		void			SetPetAdvanced(CPetAdvanced* petAdvanced) { m_petAdvanced = petAdvanced; }
		CPetAdvanced*	GetPetAdvanced() { return m_petAdvanced; }
		const CPetAdvanced* GetPetAdvanced() const { return m_petAdvanced; }
#endif

	public:
		enum class EEventTypes {
			COMPUTE_POINTS,
			DESTROY,
			MAX_NUM,
		};

		using TCharacterEventFunction = std::function<void(LPCHARACTER)>;

		DWORD add_event(EEventTypes type, TCharacterEventFunction&& func)
		{
			auto casted_type = static_cast<BYTE>(type);

			if (_dynamic_events_executing[casted_type])
				return 0;

			_dynamic_event_funcs[casted_type][++_dynamic_events_counter[casted_type]] = std::move(func);
			return _dynamic_events_counter[casted_type];
		}
		void remove_event(EEventTypes type, DWORD id)
		{
			auto casted_type = static_cast<BYTE>(type);

			if (_dynamic_events_executing[casted_type])
				return;

			_dynamic_event_funcs[casted_type].erase(id);
		}

	private:
		void _exec_events(EEventTypes type, bool clear)
		{
			auto casted_type = static_cast<BYTE>(type);

			_dynamic_events_executing[casted_type] = true;
			for (auto& pair : _dynamic_event_funcs[casted_type])
				pair.second(this);
			if (clear)
				_dynamic_event_funcs[casted_type].clear();
			_dynamic_events_executing[casted_type] = false;
		}

	private:
		std::unordered_map<DWORD, TCharacterEventFunction> _dynamic_event_funcs[static_cast<BYTE>(EEventTypes::MAX_NUM)];
		DWORD _dynamic_events_counter[static_cast<BYTE>(EEventTypes::MAX_NUM)];
		bool _dynamic_events_executing[static_cast<BYTE>(EEventTypes::MAX_NUM)];

#ifdef AUCTION_SYSTEM
	public:
		void set_auction_shop_owner(DWORD pid) noexcept { _auction_shop_owner = pid; }
		DWORD get_auction_shop_owner() const noexcept { return _auction_shop_owner; }

	private:
		DWORD _auction_shop_owner;
#endif		

#ifdef CRYSTAL_SYSTEM
	public:
		void set_active_crystal_id(DWORD item_id);
		void clear_active_crystal_id();
		DWORD get_active_crystal_id() const;
		void check_active_crystal();

	private:
		static constexpr auto ACTIVE_CRYSTAL_ID_FLAG = "crystal_system.active_item_id";
#endif

	public:
		bool is_temp_login() const { return GetDesc() ? GetDesc()->GetAccountTable().temp_login() : false; }
};

ESex GET_SEX(LPCHARACTER ch);

#endif
