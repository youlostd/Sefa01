#ifndef __INC_METIN_II_GAME_CONSTANTS_H__
#define __INC_METIN_II_GAME_CONSTANTS_H__

#include "../../common/tables.h"
#include "protobuf_data.h"
#ifdef __EVENT_MANAGER__
#include "event_tagteam.h"
#endif

// MAP_INDEX
enum EMapIndexList
{
	HOME_MAP_INDEX_RED_1 = 1,
	HOME_MAP_INDEX_RED_2 = 2,
	HOME_MAP_INDEX_YELLOW_1 = 3,
	HOME_MAP_INDEX_YELLOW_2 = 4,
	HOME_MAP_INDEX_BLUE_1 = 5,
	HOME_MAP_INDEX_BLUE_2 = 6,

	SNOW_MAP_INDEX = 10,
	FLAME_MAP_INDEX = 11,
	DESERT_MAP_INDEX = 12,
	THREEWAY_MAP_INDEX = 13,
	MILGYO_MAP_INDEX = 14,
	TRENT_1_MAP_INDEX = 15,
	TRENT_2_MAP_INDEX = 16,

	SPIDER_MAP_INDEX_1 = 17,
	SPIDER_MAP_INDEX_2 = 18,
	SPIDER_MAP_INDEX_3 = 19,
	
	SKIPIA_MAP_INDEX_1 = 20,
	SKIPIA_MAP_INDEX_2 = 21,

	DRAGON_CAPE_MAP_INDEX = 22,

	NEW_ENCHANTED_FOREST = 26,

	COMBAT_ZONE_MAP_INDEX = 29,

	DEVILTOWER_MAP_INDEX = 40,
	DEVILSCATACOMB_MAP_INDEX = 41,
	SKIPIA_MAP_INDEX_BOSS = 42,

	GUILD_WAR_MAP_INDEX = 47,
	GUILD_FLAG_WAR_MAP_INDEX = 48,

	EMPIREWAR_MAP_INDEX = 49,
	PVP_TOURNAMENT_MAP_INDEX = 50,
	EVENT_LABYRINTH_MAP_INDEX = 51,

	HYDRA_RUN_MAP_INDEX = 53,
	HYDRA_WAIT_MAP_INDEX = 54,
	
	EVENT_VALENTINE2019_DUNGEON = 61,
	DUNGEON_RAZADOR_MAP_INDEX = 65,
	// pls make sure if its not used don't have used mapindexes
	// NON USED 
	// NUSLUCK_MAP_INDEX = 69,

#ifdef ENABLE_ZODIAC_TEMPLE
	ZODIAC_MAP_INDEX = 69,
#endif
	DUEL_MAP_INDEX = 70,

	MONKEY_EASY_MAP_INDEX_1 = 55,
	MONKEY_EASY_MAP_INDEX_2 = 56,
	MONKEY_EASY_MAP_INDEX_3 = 57,
	MONKEY_MEDIUM_MAP_INDEX = 71,
	MONKEY_EXPERT_MAP_INDEX = 72,

	WEDDING_MAP_INDEX = 93,
	OXEVENT_MAP_INDEX = 94,
	// NON USED
#ifdef ENABLE_REACT_EVENT
	REACT_EVENT_MAP = 62,
#endif
	
};
// MAP_INDEX_END
typedef struct SPosition
{
	DWORD	dwX;
	DWORD	dwY;
} TPosition;

#ifdef __EVENT_MANAGER__
extern const TPosition EventTagTeamSpawnPosition[CEventTagTeam::MAX_TEAM_COUNT];
#endif

#ifdef __FAKE_BUFF__
#define FAKE_BUFF_SKILL_COUNT 3
extern const DWORD adwFakeBuffSkillVnums[FAKE_BUFF_SKILL_COUNT];
#endif

#ifdef __EQUIPMENT_CHANGER__
extern const BYTE EquipmentChangerSlots[EQUIPMENT_PAGE_MAX_PARTS];
#endif

#define ARMOR_COLLECTION_MAX_COUNT 10
extern const DWORD adwArmorCollection[JOB_MAX_NUM][ARMOR_COLLECTION_MAX_COUNT];

enum EMonsterChatState
{
	MONSTER_CHAT_WAIT,
	MONSTER_CHAT_ATTACK,
	MONSTER_CHAT_CHASE,
	MONSTER_CHAT_ATTACKED,
};

typedef struct SNPCMovingPosition
{
	long	lX;
	long	lY;
	bool	bRun;
} TNPCMovingPosition;

typedef struct SMobRankStat
{
	int iGoldPercent;   // µ·ÀÌ ³ª¿Ã È®·ü
} TMobRankStat;

typedef struct SMobStat
{
	BYTE	byLevel;
	WORD	HP;
	DWORD	dwExp;
	WORD	wDefGrade;
} TMobStat;

typedef struct SBattleTypeStat
{
	int		AttGradeBias;
	int		DefGradeBias;
	int		MagicAttGradeBias;
	int		MagicDefGradeBias;
} TBattleTypeStat;

typedef struct SJobInitialPoints
{
	int		st, ht, dx, iq;
	int		max_hp, max_sp;
	int		hp_per_ht, sp_per_iq;
	int		hp_per_lv_begin, hp_per_lv_end;
	int		sp_per_lv_begin, sp_per_lv_end;
	int		max_stamina;
	int		stamina_per_con;
	int		stamina_per_lv_begin, stamina_per_lv_end;
} TJobInitialPoints;

#ifdef __ANIMAL_SYSTEM__
typedef struct SAnimalStatData {
	BYTE	bType;
	short	sValuePerLevel;
} TAnimalStatData;

typedef struct SAnimalMainStatData {
	BYTE	bType;
	short	sValuePerLevel;
	short	sValueBase;
} TAnimalMainStatData;

typedef struct SAnimalLevelData {
	TAnimalMainStatData	kMainStat;
	TAnimalStatData	kStat[4];
} TAnimalLevelData;

extern const TAnimalLevelData MountAnimalData;
#ifdef __PET_SYSTEM__
extern const TAnimalLevelData PetAnimalData;
#endif
#endif

typedef struct __coord
{
	int		x, y;
} Coord;

typedef struct SApplyInfo
{
	BYTE	bPointType;						  // APPLY -> POINT
} TApplyInfo;

enum {
	FORTUNE_BIG_LUCK,
	FORTUNE_LUCK,
	FORTUNE_SMALL_LUCK,
	FORTUNE_NORMAL,
	FORTUNE_SMALL_BAD_LUCK,
	FORTUNE_BAD_LUCK,
	FORTUNE_BIG_BAD_LUCK,
	FORTUNE_MAX_NUM,
};

const int STONE_INFO_MAX_NUM = 7;
const int STONE_LEVEL_MAX_NUM = 4;

struct SStoneDropInfo
{
	DWORD dwMobVnum;
	int iDropPct;
	int iLevelPct[STONE_LEVEL_MAX_NUM+1];
};

inline bool operator < (const SStoneDropInfo& l, DWORD r)
{
	return l.dwMobVnum < r;
}

inline bool operator < (DWORD l, const SStoneDropInfo& r)
{
	return l < r.dwMobVnum;
}

inline bool operator < (const SStoneDropInfo& l, const SStoneDropInfo& r)
{
	return l.dwMobVnum < r.dwMobVnum;
}

#define AURA_MAX_DISTANCE 3000
#define AURA_BUFF_MAX_NUM 2
extern const DWORD adwAuraBuffList[AURA_BUFF_MAX_NUM];

extern const TApplyInfo		aApplyInfo[MAX_APPLY_NUM];
extern const TMobRankStat	   MobRankStats[MOB_RANK_MAX_NUM];

extern TBattleTypeStat		BattleTypeStats[BATTLE_TYPE_MAX_NUM];

extern const DWORD		party_exp_distribute_table[PLAYER_MAX_LEVEL_CONST + 1];

extern const DWORD		exp_table[PLAYER_EXP_TABLE_MAX + 1];
#ifdef __PET_ADVANCED__
extern const long long	pet_exp_table[PET_MAX_LEVEL + 1];
#endif

#ifdef __ANIMAL_SYSTEM__
#ifdef __PET_SYSTEM__
extern const long		pet_exp_table[ANIMAL_EXP_TABLE_MAX + 1];
#endif
extern const long		mount_exp_table[ANIMAL_EXP_TABLE_MAX + 1];
#endif

extern const DWORD		guild_exp_table[GUILD_MAX_LEVEL + 1];
extern const DWORD		guild_exp_table2[GUILD_MAX_LEVEL + 1];

#define MAX_EXP_DELTA_OF_LEV	31
#ifdef ELONIA
#define PERCENT_LVDELTA(me, victim) aiPercentByDeltaLev[MINMAX(0, (victim + 20) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#define PERCENT_LVDELTA_BOSS(me, victim) aiPercentByDeltaLevForBoss[MINMAX(0, (victim + 20) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#else
#define PERCENT_LVDELTA(me, victim) aiPercentByDeltaLev[MINMAX(0, (victim + 15) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#define PERCENT_LVDELTA_BOSS(me, victim) aiPercentByDeltaLevForBoss[MINMAX(0, (victim + 15) - me, MAX_EXP_DELTA_OF_LEV - 1)]
#endif
#define CALCULATE_VALUE_LVDELTA(me, victim, val) ((val * PERCENT_LVDELTA(me, victim)) / 100)
extern const int		aiPercentByDeltaLev[MAX_EXP_DELTA_OF_LEV];
extern const int		aiPercentByDeltaLevForBoss[MAX_EXP_DELTA_OF_LEV];

extern const DWORD adwHorseRageTime[HORSE_RAGE_MAX_LEVEL + 1];

#define ARROUND_COORD_MAX_NUM	161
extern Coord			aArroundCoords[ARROUND_COORD_MAX_NUM];
extern TJobInitialPoints	JobInitialPoints[JOB_MAX_NUM];

extern const int		aiMobEnchantApplyIdx[MOB_ENCHANTS_MAX_NUM];
extern const int		aiMobResistsApplyIdx[MOB_RESISTS_MAX_NUM];

extern const int		aSkillAttackAffectProbByRank[MOB_RANK_MAX_NUM];

extern const int aiItemMagicAttributePercentHigh[ITEM_ATTRIBUTE_MAX_LEVEL]; // 1°³±îÁö
extern const int aiItemMagicAttributePercentLow[ITEM_ATTRIBUTE_MAX_LEVEL];

extern const int aiItemAttributeAddPercent[ITEM_ATTRIBUTE_MAX_NUM];

extern const int aiWeaponSocketQty[WEAPON_NUM_TYPES];
extern const int aiArmorSocketQty[ARMOR_NUM_TYPES];
extern const int aiSocketPercentByQty[5][4];

extern const int aiExpLossPercents[PLAYER_EXP_TABLE_MAX + 1];

extern const int aiSkillPowerByLevel[SKILL_MAX_LEVEL + 1];

extern const int aiPolymorphPowerByLevel[SKILL_MAX_LEVEL + 1];

extern const int aiSkillBookCountForLevelUp[10];
extern const int aiGrandMasterSkillBookCountForLevelUp[10];
extern const int aiGrandMasterSkillBookMinCount[10];
extern const int aiGrandMasterSkillBookMaxCount[10];
extern const int CHN_aiPartyBonusExpPercentByMemberCount[9];
extern const int KOR_aiPartyBonusExpPercentByMemberCount[9];
extern const int KOR_aiUniqueItemPartyBonusExpPercentByMemberCount[9];

typedef std::map<DWORD, network::TItemAttrTable> TItemAttrMap;
extern TItemAttrMap g_map_itemAttr;
#ifdef EL_COSTUME_ATTR
extern TItemAttrMap g_map_itemCostumeAttr;
#endif
#ifdef ITEM_RARE_ATTR
extern TItemAttrMap g_map_itemRare;
#endif

extern const int aiChainLightningCountBySkillLevel[SKILL_MAX_LEVEL + 1];

extern const char * c_apszEmpireNames[EMPIRE_MAX_NUM];
extern const char * c_apszPrivNames[MAX_PRIV_NUM];
extern const SStoneDropInfo aStoneDrop[STONE_INFO_MAX_NUM];

typedef struct
{
	long lMapIndex;
	int iWarPrice;
	int iWinnerPotionRewardPctToWinner;
	int iLoserPotionRewardPctToWinner;
	int iInitialScore;
	int iEndScore;
} TGuildWarInfo;

extern TGuildWarInfo KOR_aGuildWarInfo[GUILD_WAR_TYPE_MAX_NUM];

// ACCESSORY_REFINE
enum 
{
	ITEM_ACCESSORY_SOCKET_MAX_NUM = 3
};

extern const int aiAccessorySocketAddPct[ITEM_ACCESSORY_SOCKET_MAX_NUM];
extern const int aiAccessorySocketEffectivePct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
extern const int aiAccessorySocketDegradeTime[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
extern const int aiAccessorySocketPutPct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
extern const int aiAccessorySocketPutPermaPct[ITEM_ACCESSORY_SOCKET_MAX_NUM + 1];
long FN_get_apply_type(const char *apply_type_string);

// END_OF_ACCESSORY_REFINE

long FN_get_apply_type(const char *apply_type_string);

enum eBossHunt
{
	BOSSHUNT_BOSS_VNUM = 6320,
#ifdef BOSSHUNT_EVENT_UPDATE
	BOSSHUNT_BOSS_VNUM2 = 6322,
#endif
};

typedef struct SBossHuntSpawnPos
{
	long lMapIndex;
	std::vector<Coord> vCoords;
} BossHuntSpawnPos;

extern const std::vector<BossHuntSpawnPos> g_bossHuntSpawnPos;
#endif

