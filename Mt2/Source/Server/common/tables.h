#ifndef __INC_TABLES_H__
#define __INC_TABLES_H__

#include "length.h"
#include "protobuf_data.h"
#include "protobuf_data_player.h"
#include <chrono>
#define LONGLONG long long
#define ULONGLONG unsigned long long

typedef	DWORD IDENT;

#ifdef ENABLE_RUNE_SYSTEM
struct RuneShieldStat
{
	DWORD shieldHPTotal;
	DWORD shieldHPByAttack;
	DWORD magicShield;
	bool attackShieldFull;
};
struct RuneCharData
{
	BYTE killCount;
	BYTE normalHitCount;
	BYTE skillHitCount;
	bool isNewHit;
	bool isNewSkill;
	WORD lastSkillVnum;

	int storedMovementSpeed;
	int storedAttackSpeed;

	WORD bonusMowSpeed;
	DWORD soulsHarvested;
	std::chrono::steady_clock::time_point lastHitStart;
	std::chrono::steady_clock::time_point timeSinceLastHit;
	RuneShieldStat shield;
};
#endif
/* ----------------------------------------------
 * table
 * ----------------------------------------------
 */

/* game Server -> DB Server */
#pragma pack(1)

enum EPlayerTableChanged
{
	PC_TAB_CHANGED_SKILLS,
	PC_TAB_CHANGED_QUICKSLOT,
#ifdef ENABLE_RUNE_SYSTEM
	PC_TAB_CHANGED_RUNE,
#endif
	PC_TAB_CHANGED_MAX_NUM,
};

#ifdef ENABLE_RUNE_SYSTEM
enum ERuneGroups {
	RUNE_GROUP_PRECISION,
	RUNE_GROUP_DOMINATION,
	RUNE_GROUP_SORCERY,
	RUNE_GROUP_RESOLVE,
	RUNE_GROUP_INSPIRATION,
	RUNE_GROUP_MAX_NUM,
};

enum ESubGroups {
	RUNE_SUBGROUP_SECONDARY1,
	RUNE_SUBGROUP_SECONDARY2,
	RUNE_SUBGROUP_SECONDARY3,
	RUNE_SUBGROUP_PRIMARY,
	RUNE_SUBGROUP_MAX_NUM,
};

enum ERuneInfo {
	RUNE_MAIN_COUNT = 4,
	RUNE_SUB_COUNT = 2,
#ifdef ENABLE_RUNE_PAGES
	RUNE_PAGE_COUNT = 4,
#endif
};
#endif

enum eDungeonRanking
{
	DUNGEON_AZRAEL,
	DUNGEON_SLIME,
	DUNGEON_RAZADOR,
	DUNGEON_NEMERE,
	DUNGEON_JOTUN,
	DUNGEON_CRYTAL,
	DUNGEON_THRANDUIL,
	DUNGEON_INFECTED,

	DUNGEON_RANKING_MAX_NUM
};

#define QUEST_NAME_MAX_LEN	32
#define QUEST_STATE_MAX_LEN	64

#define SAFEBOX_MAX_NUM			5 * 10 * 3
#define SAFEBOX_PASSWORD_MAX_LEN	6

enum BlockSystem
{
	SYST_BLOCK,
	SYST_FRIEND
};

typedef struct SGuildReserve
{
	SGuildReserve() {}
	SGuildReserve(DWORD id, DWORD guild_from, DWORD guild_to, DWORD time, BYTE type, long war_price, long initial_score,
		bool started, DWORD bet_from, DWORD bet_to, long power_from, long power_to, long handicap) :
		dwID(id), dwGuildFrom(guild_from), dwGuildTo(guild_to), dwTime(time), bType(type), lWarPrice(war_price),
		lInitialScore(initial_score), bStarted(started), dwBetFrom(bet_from), dwBetTo(bet_to), lPowerFrom(power_from),
		lPowerTo(power_to), lHandicap(handicap)
	{}

	DWORD       dwID;
	DWORD       dwGuildFrom;
	DWORD       dwGuildTo;
	DWORD       dwTime;
	BYTE        bType;
	long        lWarPrice;
	long        lInitialScore;
	bool        bStarted;
	DWORD	dwBetFrom;
	DWORD	dwBetTo;
	long	lPowerFrom;
	long	lPowerTo;
	long	lHandicap;
} TGuildWarReserve;

#ifdef __GUILD_SAFEBOX__
#define GUILD_SAFEBOX_PASSWORD_MAX_LEN 6
#define GUILD_SAFEBOX_DEFAULT_SIZE 2
#define GUILD_SAFEBOX_ITEM_WIDTH 5
#define GUILD_SAFEBOX_ITEM_HEIGHT 10
#define GUILD_SAFEBOX_MAX_NUM 200
#define GUILD_SAFEBOX_LOG_MAX_COUNT 400
#define GUILD_SAFEBOX_LOG_SEND_COUNT 50

enum EGuildSafeboxLogTypes {
	GUILD_SAFEBOX_LOG_CREATE,
	GUILD_SAFEBOX_LOG_ITEM_GIVE,
	GUILD_SAFEBOX_LOG_ITEM_TAKE,
	GUILD_SAFEBOX_LOG_GOLD_GIVE,
	GUILD_SAFEBOX_LOG_GOLD_TAKE,
	GUILD_SAFEBOX_LOG_SIZE,
};
#endif

typedef struct SDungeonCompleted
{
	DWORD dwPID;
	BYTE bDungeon;
	DWORD dwTime;
} TDungeonCompleted;

#ifdef BLACKJACK
enum {
	BLACKJACK_MAX_CARDS = 52,
	BLACKJACK_CARD_TYPES = 4,
	BLACKJACK_STAKE_ITEM = 95217,
	BLACKJACK_REWARD1 = 95221,
	BLACKJACK_REWARD2 = 95222,
	BLACKJACK_REWARD3 = 95223,
	BLACKJACK_ACTION_HIT = 0,
	BLACKJACK_ACTION_STAY = 1,
};

typedef struct SBlackJackCard
{
	std::string	sCard;
	BYTE	bPoints;
} TBlackJackCard;

static std::string BlackJackCardTypes = "HSCD";

static TBlackJackCard BlackJackDefaultDeck[BLACKJACK_MAX_CARDS/BLACKJACK_CARD_TYPES] = {
	{ "2", 2 },
	{ "3", 3 },
	{ "4", 4 },
	{ "5", 5 },
	{ "6", 6 },
	{ "7", 7 },
	{ "8", 8 },
	{ "9", 9 },
	{ "10", 10 },
	{ "J", 10 },
	{ "Q", 10 },
	{ "K", 10 },
	{ "A", 11 },
};

#endif

#ifdef DMG_RANKING
enum TypeDmg
{
	TYPE_DMG_NORMAL,
	TYPE_DMG_SKILL,

	TYPE_DMG_MAX_NUM,

	TYPE_DMG_RANKING_MAX = 10
};

typedef struct SRankDamageEntry
{
	SRankDamageEntry()
		//: name{'\0'}, dmg(0)
		: dmg(0)
	{
		name[CHARACTER_NAME_MAX_LEN + 1] = { '\0' }; // fix for building dump_proto - cannot specify explicit initializer for arrays
	}

	SRankDamageEntry(const char *name, int value)
		: dmg(value)
	{
		strcpy(this->name, name);
	}

	char name[CHARACTER_NAME_MAX_LEN + 1];
	int dmg;
} TRankDamageEntry;
#endif

#ifdef __GUILD_SAFEBOX__
enum EGuildSafeboxHeader {
	HEADER_DG_GUILD_SAFEBOX_SET,
	HEADER_DG_GUILD_SAFEBOX_DEL,
	HEADER_DG_GUILD_SAFEBOX_GIVE,
	HEADER_DG_GUILD_SAFEBOX_GOLD,
	HEADER_DG_GUILD_SAFEBOX_CREATE,
	HEADER_DG_GUILD_SAFEBOX_SIZE,
	HEADER_DG_GUILD_SAFEBOX_LOAD,
	HEADER_DG_GUILD_SAFEBOX_LOG,
};
#endif

class TAffectSaveElement {
	private:
		DWORD _pid;
		TPacketAffectElement _elem;

	public:
		DWORD pid() const { return _pid; }
		void set_pid(DWORD pid) { _pid = pid; }
		void set_elem(const TPacketAffectElement *elem) { _elem = *elem; }

		TPacketAffectElement* mutable_elem() { return &_elem; }
		const TPacketAffectElement& elem() const { return _elem; }
};

typedef struct SItemPriceListTable
{
	DWORD	dwOwnerID;
	BYTE	byCount;

	network::TItemPriceInfo	aPriceInfo[SHOP_PRICELIST_MAX_NUM];
} TItemPriceListTable;

#pragma pack()
#endif
