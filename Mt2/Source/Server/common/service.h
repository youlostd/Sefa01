#ifndef __INC_SERVICE_H__
#define __INC_SERVICE_H__
#include "server.h"

//------Service*///------>
#define ENABLE_AUTODETECT_INTERNAL_IP // autodetect internal ip if the public one is missing
#define __IPV6_FIX__
#define __ANTI_BRUTEFORCE__ 9
#define __MAINTENANCE__
// #define __MAINTENANCE_DIFFERENT_SERVER__
// #define _IMPROVED_PACKET_ENCRYPTION_
// #define CHECK_IP_ON_CONNECT  // potential networking error
// #define DMG_METER
// #define QUERY_POOLING
// #define USE_QUERY_LOGGING
// #define __ENABLE_FULL_LOGS__
// #define P2P_SAFE_BUFFERING	// NO NEED ANYMORE, P2P PROBLEMS FIXED ITSELF SOMEHOW...
// #define	__CHECK_P2P_BROKEN__
// #define __DEPRECATED_BILLING__
#define ENABLE_CORE_FPS_CHECK
#define FIX_MESSENGER_QUERY_SPAM
#define PACKET_ERROR_DUMP
#define __PYTHON_REPORT_PACKET__
#define __ANTI_FLOOD__
#define __ANTI_CHEAT_FIXES__
#define __DISABLE_DEATHBLOW__
#define _PACKET_ERROR_NOQUIT_
#define __DISABLE_SPEEDHACK_CHECK__
#define LOGOUT_DISABLE_CLIENT_SEND_PACKETS
#define CHARACTER_BUG_REPORT
#define ENABLE_SPAM_FILTER
#define ACCOUNT_TRADE_BLOCK	72





//------Systems*///----->
#define AUCTION_SYSTEM
#define PROCESSOR_CORE
	#ifdef PROCESSOR_CORE
	#	define AUCTION_SYSTEM
	#	ifdef AUCTION_SYSTEM
	#		define AUCTION_SYSTEM_REQUIRE_SAME_MAP
	#	endif
	#endif
#define __COSTUME_INVENTORY__
#define __ACCE_COSTUME__
#define __COSTUME_ACCE__
#define __PET_SYSTEM__
#define __DRAGONSOUL__
#define __SWITCHBOT__
#define __BELT_SYSTEM__
#define __GAYA_SYSTEM__
#define DMG_RANKING
#define __GUILD_SAFEBOX__
#define GUILD_SAFEBOX_GOLD_MAX (1000000000LL)
#define __ITEM_SWAP_SYSTEM__
#define __EQUIPMENT_CHANGER__
#define INGAME_WIKI
#define __ELEMENT_SYSTEM__
#define __COSTUME_BONUS_TRANSFER__
#define __SKILLBOOK_INVENTORY__
#define __VOTE4BUFF__
#define __QUEST_CATEGORIES__
#define __SKIN_SYSTEM__
#define __TRADE_BLOCK_SYSTEM__
// #define __PET_ADVANCED__
#define __NEW_SPAWN_SYSTEM__
#define SOUL_SYSTEM
#define __FAKE_BUFF__
#define ENABLE_RUNE_SYSTEM




//------Options*///----->
#define GOLD_MAX_SELL 5000000000ULL
#define GOLD_MAX_OWN 25000000000LL
#define INCREASE_GOLD_MAX
	#ifdef INCREASE_GOLD_MAX
	#	undef GOLD_MAX_SELL
	#	undef GOLD_MAX_OWN
	#	define GOLD_MAX_SELL 100000000000ULL
	#	define GOLD_MAX_OWN 100000000000LL
	#endif
#define __SHOP_SELL_MAX_PRICE__ GOLD_MAX_SELL
#define __MAP_CONFIG__
#define __PARTY_GLOBAL__
#define __ATTRIBUTES_TO_CLIENT__
// #define __ANIMAL_SYSTEM__
// #define __FAKE_PRIV_BONI__ 2
// #define __EXP_DOUBLE_UNTIL30__
// #define __ALL_AUTO_LOOT__
// #define __DAMAGE_QUEST_TRIGGER__
// #define __HP_CALC_OFFICIAL__
// #define __QUEST_SAFE__
// #define __LIMIT_SHOP_NPCS__
#define __HAIR_SELECTOR__
#define __MARK_NEW_ITEM_SYSTEM__
#define __LEGENDARY_SKILL__
#define __INVENTORY_SORT__
#define __ITEM_REFUND__
#define LOCALE_SAVE_LAST_USAGE
#define IGNORE_KNOCKBACK
#define DUNGEON_REPAIR_TRIGGER
#define GOHOME_FOR_INVALID_LOCATION
#define __IGNORE_LOWER_BUFFS__
#define __DISABLE_STONE_LV20_DROP__
#define __DISABLE_BOSS_LV25_DROP__
#define __AGGREGATE_MONSTER_EFFECT__
#define __MOUNT_EXTRA_SPEED__
#define __POLY_NO_PVP_DMG__
#define ENABLE_BLOCK_PKMODE
#define __BLOCK_DISPEL_ON_MOBS__
#define __P2P_ONLINECOUNT__
#define __NO_DUNGEON_TOWN_RESTART__
#define __REAL_STUN_IMMUNE__
#define __TIMER_BIO_DUNGEON__
#define __LEVELUP_NO_POTIONS__
#define __SKILLUP_ON_17__
#define MOB_SKILL_VNUM_FIX
#define ENABLE_FASTER_LOGOUT
#define DS_TIME_ELIXIR_FIX
#define ENABLE_LEVEL_LIMIT_MAX
#define USER_ATK_SPEED_LIMIT
#define NEW_TARGET_UI
#define ENABLE_COMPANION_NAME
#define ENABLE_PERMANENT_POTIONS
#define STANDARD_SKILL_DURATION
#define KEEP_SKILL_AFFECTS
#define DS_QUALIFIED_LV1
#define CHECK_TIME_AFTER_PVP	// Pvp Patch
#define SKILL_AFFECT_DEATH_REMAIN
//#define RESET_SKILL_COLOR
//#define TARGET_DMG_VID
#define SHOP_SYSTEM_SELL_WITHOUT_SHOP
#define ENABLE_EXTENDED_ITEMNAME_ON_GROUND
// #define __ATTRTREE__
// #define ITEM_RARE_ATTR
#define __HOMEPAGE_COMMAND__





/*-----Dungeons*///----->
#define ENABLE_ZODIAC_TEMPLE
#define ENABLE_HYDRA_DUNGEON
#define COMBAT_ZONE
#define COMBAT_ZONE_HIDE_INFO_USER
#define __DUNGEON_FOR_GUILD__
#ifdef __DUNGEON_FOR_GUILD__
	#define __MELEY_LAIR_DUNGEON__
	#ifdef __MELEY_LAIR_DUNGEON__
	#	define __DESTROY_INFINITE_STATUES_GM__
	#	define __LASER_EFFECT_ON_75HP__
	#	define __LASER_EFFECT_ON_50HP__
	#endif
#endif





/*--Events/Minigame*///-->
#define __EVENT_MANAGER__
#define BLACKJACK
#define SUNDAE_EVENT
#define ENABLE_REACT_EVENT
#define HALLOWEEN_MINIGAME
#define __ANGELSDEMON_EVENT2__
#define BOSSHUNT_EVENT_UPDATE
#define AHMET_FISH_EVENT_SYSTEM
#define ENABLE_XMAS_EVENT





//////////////////////////////////////////////////[> Update <]//////////////////////////////////////////////////
/*#@@Update 2.4.0 */
#define UPDATE_2_4_0
#define ENABLE_WARP_BIND_RING
#define ENABLE_DS_SET_BONUS
#define INFINITY_ITEMS
#define BRAVERY_CAPE_STORE
#define INCREASE_GOLD_MAX
#define INCREASE_ITEM_STACK
#define SORT_AND_STACK_ITEMS

/*#@@Update 3.0.0 */
#define UPDATE_3_0_0
#define EQUIP_WHILE_ATTACKING
#define SPECIAL_NAME_LETTERS
#define CHANGE_SKILL_COLOR
#define ENABLE_NEW_EMOTES
#define SECOND_ITEM_PRICE

/*#@@Update 3.1.0 */
#define UPDATE_3_1_0
#define LEADERSHIP_EXTENSION
#define __DUNGEON_RANKING__
#define ENABLE_UPGRADE_STONE
#define AGGREGATE_MONSTER_MOVESPEED 60
#define ENABLE_MESSENGER_BLOCK



/*|~| PROMETA |~|*/
#ifdef PROMETA
//	#define __WOLFMAN__
//	#define __PRESTIGE__
#	define ITEMPROTO_SKIP_INSTEAD_EXIT
//	#define __ALPHA_EQUIP__
//	#define __RIFT_SYSTEM__
#	define NEW_OFFICIAL
#endif

/*|~| ELONIA |~|*/
#ifdef ELONIA
#	define HORSE_SECOND_GRADE
#	define SHOPSEARCH_INMAP_ONLY
#	define __AUCTION_SORT_ONLY_SHUFFLE_EVERYTHING__
#	define EL_COSTUME_ATTR
#	define ACCE_COMBINE_CHANGES
#	define SEARCH_SHOP_BOOK_NAME
#	define BATTLEPASS_EXTENSION
#	define NO_PARTY_DROP
#	define MOB_STATUS_BUG_ENABLE
#	define __TESTSERVER_NO_TABLE_CHECK__
#	define __NEW_DROP_SYSTEM__
#	define NO_MOUNTING_IN_PVP
#	define DS_ALCHEMY_SHOP_BUTTON
#	define CRYSTAL_SYSTEM
#endif

#endif
