#pragma once
#include "../../common/service.h"
#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraConfig.h"

class CHydraDungeon final
{
	private:
		struct map_destroy_event_info : public event_info_data
		{
			long lMapIndex;
		};

		struct stage_event_info : public event_info_data
		{
			CHydraDungeon* pDungeon;
		};

	public:
		enum EStages
		{
			STAGE_ONE,
			STAGE_TWO,
			STAGE_THREE,
			STAGE_FOUR,
			STAGE_FIVE,
			STAGE_SIX,
			STAGE_SEVEN,
			STAGE_EIGHT,
			STAGE_NINE,

			STAGE_COMPLETED = 253,
			STAGE_FAIL = 254,
			STAGE_WAITING = 255
		};

	public:
		CHydraDungeon(DWORD owner);
		~CHydraDungeon();

		void OnLogin(LPCHARACTER ch);
		void OnLogout(LPCHARACTER ch);
		void OnKill(LPCHARACTER ch, LPCHARACTER victim);
		void ProcessDestroy(LPCHARACTER ch);

		long CreateDungeon();
		long ProcessStage(bool isTimerEvent);

		void NotifyHydraDmg(LPCHARACTER hydra, int* dmg);

		DWORD GetOwner() { return m_dwOwnerID; }
		EStages GetStage() { return m_bStage; };

		void ClearStageEvent() { m_stageEvent = NULL; }

	private:
		static long map_destroy_event(LPEVENT event, long processing_time);
		static long stage_event(LPEVENT event, long processing_time);

		void DestroyDungeon();
		void WarpToExit(LPCHARACTER ch = NULL);
		void ClearMonsters();
		void SetStage(EStages nextStage);
		void ProcessFailure();
		void SendStageNotice(LPCHARACTER ch = NULL);

		template <class Func> void ForEachMember(Func & f)
		{
			for (auto it = m_vecPC.begin(); it != m_vecPC.end(); ++it)
				f(*it);
		}

	private:
		std::vector<LPCHARACTER> m_vecPC;
		std::map<DWORD, LPCHARACTER> m_monsterKeep;
		std::map<DWORD, LPCHARACTER> m_hydraEggs;

		long m_lMapIndex;
		DWORD m_dwOwnerID;

		LPEVENT m_destroyEvent;
		LPEVENT m_stageEvent;
		LPEVENT m_warpOutTimer;

		LPCHARACTER m_mastNPC;
		LPCHARACTER m_hydraBoss;
		LPCHARACTER m_rewardChest;

		bool m_bEggsSpawned;
		EStages m_bStage;
		time_t m_runStart;
		time_t m_runEnd;

		LPCHARACTER	m_hydraHeads[HYDRA_HEADS_MAX_NUM];
};

class CHydraDungeonManager final : public singleton<CHydraDungeonManager>
{
	public:
		CHydraDungeonManager();
		~CHydraDungeonManager();
		void Destroy();

		bool CreateDungeon(LPCHARACTER owner);
		void DestroyDungeon(long lMapIndex);
		void OnLogin(LPCHARACTER ch);
		void OnDestroy(LPCHARACTER ch); 
		void OnKill(LPCHARACTER ch, LPCHARACTER victim);

		bool CanAttack(LPCHARACTER ch, LPCHARACTER victim);

		void NotifyHydraDmg(LPCHARACTER hydra, int * dmg);

	private:
		std::map<long, CHydraDungeon*> m_mapDungeon;
};

inline void CHydraDungeonManager::OnDestroy(LPCHARACTER ch)
{
	if (ch->IsPrivateMap(DUNGEON_MAPINDEX))
	{
		auto it = m_mapDungeon.find(ch->GetMapIndex());
		if (it == m_mapDungeon.end())
			return;

		if (ch->IsPC())
		{
			sys_log(0, "pc %d is about to log off from hydra map %d", ch->GetPlayerID(), ch->GetMapIndex());
			it->second->OnLogout(ch);
		}
		else
			it->second->ProcessDestroy(ch);
	}
}

inline void CHydraDungeonManager::OnKill(LPCHARACTER ch, LPCHARACTER victim)
{
	if (!ch || !victim || ch == victim)
		return;

	if (ch->IsPrivateMap(DUNGEON_MAPINDEX))
	{
		auto it = m_mapDungeon.find(ch->GetMapIndex());
		if (it == m_mapDungeon.end())
			return;

		it->second->OnKill(ch, victim);
	}
}

inline bool CHydraDungeonManager::CanAttack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (ch == victim || !ch || !victim)
		return false;

	if (!ch->IsPrivateMap(DUNGEON_MAPINDEX) || ch->IsPC() && !victim->IsMonster() && !victim->IsStone())
		return false;

	if (ch->IsMonster() && (victim->IsMonster() || victim->IsStone()))
		return false;

	return true;
}

inline void CHydraDungeonManager::NotifyHydraDmg(LPCHARACTER hydra, int * dmg)
{
	auto it = m_mapDungeon.find(hydra->GetMapIndex());
	if (it == m_mapDungeon.end())
		return;

	it->second->NotifyHydraDmg(hydra, dmg);
}

#endif