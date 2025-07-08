#pragma once
#include "Locale_m2.h"

#include "protobuf_data.h"

/*
 *	NPC 데이터 프로토 타잎을 관리 한다.
 */
class CPythonNonPlayer : public CSingleton<CPythonNonPlayer>
{
	public:
		enum EMobTypes
		{
			MONSTER,
			NPC,
			STONE,
			WARP,
			DOOR, 
			BUILDING, 
			PC, 
			POLYMORPH_PC, 
			HORSE, 
			GOTO
		};

		enum  EClickEvent
		{
			ON_CLICK_EVENT_NONE		= 0,
			ON_CLICK_EVENT_BATTLE	= 1,
			ON_CLICK_EVENT_SHOP		= 2,
			ON_CLICK_EVENT_TALK		= 3,
			ON_CLICK_EVENT_VEHICLE	= 4,

			ON_CLICK_EVENT_MAX_NUM,
		};

		enum EMobEnchants
		{   
			MOB_ENCHANT_CURSE,
			MOB_ENCHANT_SLOW,   
			MOB_ENCHANT_POISON,
			MOB_ENCHANT_STUN,   
			MOB_ENCHANT_CRITICAL,
			MOB_ENCHANT_PENETRATE,
			MOB_ENCHANT_IGNORE_BLOCK,
			MOB_ENCHANTS_MAX_NUM
		};
		enum EMobResists
		{
			MOB_RESIST_SWORD,
			MOB_RESIST_TWOHAND,
			MOB_RESIST_DAGGER,
			MOB_RESIST_BELL,
			MOB_RESIST_FAN,
			MOB_RESIST_BOW,
			MOB_RESIST_CLAW,
			MOB_RESIST_FIRE,
			MOB_RESIST_ELECT,
			MOB_RESIST_MAGIC,
			MOB_RESIST_WIND,
			MOB_RESIST_POISON,
			MOB_RESIST_BLEEDING,
			MOB_RESIST_EARTH,
			MOB_RESIST_ICE,
			MOB_RESIST_DARK,
			MOB_RESISTS_MAX_NUM
		};

		enum EMobRaceFlags
		{
			RACE_FLAG_ANIMAL	= (1 << 0),
			RACE_FLAG_UNDEAD	= (1 << 1),
			RACE_FLAG_DEVIL		= (1 << 2),
			RACE_FLAG_HUMAN		= (1 << 3),
			RACE_FLAG_ORC		= (1 << 4),
			RACE_FLAG_MILGYO	= (1 << 5),
			RACE_FLAG_INSECT	= (1 << 6),
			RACE_FLAG_DESERT	= (1 << 7),
			RACE_FLAG_TREE		= (1 << 8),
			RACE_FLAG_ELEC		= (1 << 9),
			RACE_FLAG_WIND		= (1 << 10),
			RACE_FLAG_EARTH		= (1 << 11),
			RACE_FLAG_DARK		= (1 << 12),
			RACE_FLAG_FIRE		= (1 << 13),
			RACE_FLAG_ICE		= (1 << 14),
			RACE_FLAG_ZODIAC	= (1 << 15),
			RACE_FLAG_MAX_NUM	= 16,
		};

		#define MOB_ATTRIBUTE_MAX_NUM	12
		#define MOB_SKILL_MAX_NUM		5

		using TMobTable = network::TMobTable;

		typedef struct SWikiInfoTable
		{
			std::unique_ptr<TMobTable> mobTable;

			bool isSet;
			bool isFiltered;
			std::vector<DWORD> dropList;
		} TWikiInfoTable;

		typedef std::list<TMobTable *> TMobTableList;
		typedef std::map<DWORD, TWikiInfoTable> TNonPlayerDataMap;

	public:
		CPythonNonPlayer(void);
		virtual ~CPythonNonPlayer(void);

		void Clear();
		void Destroy();

		bool				LoadNonPlayerData(const char * c_szFileName);

		const TMobTable *	GetTable(DWORD dwVnum);
		TWikiInfoTable*		GetWikiTable(DWORD dwVnum);
		bool				GetName(DWORD dwVnum, const char ** c_pszName);
		bool				GetInstanceType(DWORD dwVnum, BYTE* pbType);
		BYTE				GetEventType(DWORD dwVnum);
		BYTE				GetEventTypeByVID(DWORD dwVID);
		DWORD				GetMonsterColor(DWORD dwVnum);
		const char*			GetMonsterName(DWORD dwVnum);
		BYTE				GetMobLevel(DWORD dwVnum);
#ifdef INGAME_WIKI
		size_t				WikiLoadClassMobs(BYTE bType, WORD fromLvl, WORD toLvl);
		std::vector<DWORD>* WikiGetLastMobs() { return &m_vecTempMob; }
		void				WikiSetBlacklisted(DWORD vnum);
#endif
		
		// Function for outer
		void				GetMatchableMobList(int iLevel, int iInterval, TMobTableList * pMobTableList);

		void				BuildWikiSearchList();
		DWORD				GetVnumByNamePart(const char* c_pszName);

	protected:
		TNonPlayerDataMap	m_NonPlayerDataMap;
#ifdef INGAME_WIKI
		void				SortMobDataName();
		std::vector<DWORD>	m_vecTempMob;
		std::vector<TMobTable *> m_vecWikiNameSort;
#endif
};