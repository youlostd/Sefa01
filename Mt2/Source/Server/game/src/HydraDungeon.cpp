#include "stdafx.h"
#ifdef ENABLE_HYDRA_DUNGEON
#include "config.h"
#include "constants.h"
#include "sectree_manager.h"
#include "char.h"
#include "party.h"

#include "HydraDungeon.h"

BYTE bDungeonWaitingTime = EHydraConfig::DUNGEON_WAITINGTIME;

struct HydraDungeonWarp
{
	long lMapIndex;
	DWORD ownerID;
	bool isInside;
	long lX;
	long lY;
	bool initSuccess;
	HydraDungeonWarp(long mapIdx, DWORD owner, long x = WARP_POS_X, long y = WARP_POS_Y, bool inside = true) : lMapIndex(mapIdx), ownerID(owner), isInside(inside)
	{
		PIXEL_POSITION basePos;
		if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, basePos))
		{
			sys_err("cannot get base pos %d", lMapIndex);
			initSuccess = false;
			return;
		}

		lX = x * 100 + basePos.x;
		lY = y * 100 + basePos.y;
		initSuccess = true;
	}

	void operator()(LPCHARACTER pc)
	{
		if (!initSuccess)
			return;

		pc->SetQuestFlag("hydra_dungeon.current_id_" + std::to_string(g_bChannel), isInside ? lMapIndex : 0);
		pc->SetQuestFlag("hydra_dungeon.current_owner_" + std::to_string(g_bChannel), isInside ? ownerID : 0);
		pc->WarpSet(lX, lY, lMapIndex);
	}
};

struct KillAllMob
{
	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() || pChar->IsMonster())
				pChar->Dead(NULL, true);
		}
	}
};

struct HydraDebuff
{
	bool bAdd;
	HydraDebuff(bool add = false) : bAdd(add)
	{}

	void operator()(LPCHARACTER ch)
	{
		if (bAdd)
			ch->AddAffect(AFFECT_HYDRA, APPLY_NONE, MINMAX(0, 100 - HYDRA_DEBUFF_PERCENTAGE, 100), AFF_HYDRA, 3600 * 24 * 7, 0, true);
		else
			ch->RemoveAffect(AFFECT_HYDRA);
	}
};

struct SendHydraNotice
{
	std::string text;
	char * outBuff;
	size_t outBuffLength;
	ArgumentPrinter printer;
	SendHydraNotice(const char* format, size_t defArgBufferLength = 512) : 
		text(format), outBuff(new char[defArgBufferLength * 4]), outBuffLength(defArgBufferLength * 4), printer(defArgBufferLength)
	{
	}

	~SendHydraNotice()
	{
		delete[] outBuff;
	}

	void operator()(LPCHARACTER ch)
	{
		if (ch->GetDesc())
		{
			const char* trans = LC_TEXT(ch, text.c_str());
			if (printer.PrintWithArg(trans, outBuff, outBuffLength))
				ch->ChatPacket(CHAT_TYPE_NOTICE, outBuff);
			else
				ch->ChatPacket(CHAT_TYPE_NOTICE, trans);
		}
	}
};

struct SendMastHP
{
	int currHP, maxHP;
	SendMastHP(int _currHP = 0, int _maxHP = 0) : currHP(_currHP), maxHP(_maxHP)
	{}

	void operator()(LPCHARACTER ch) { ch->ChatPacket(CHAT_TYPE_COMMAND, "mast_hp %d %d", currHP, maxHP); }
};

void CalcRunTime(time_t compTime, BYTE &hours, BYTE &mins, BYTE &secs)
{
	hours = compTime / 3600;
	compTime -= hours * 3600;

	mins = compTime / 60;
	compTime -= mins * 60;

	secs = compTime;
	//compTime -= secs;
}

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Dungeon handling ////////////////////////////////////////

CHydraDungeon::CHydraDungeon(DWORD owner) : m_lMapIndex(0), m_dwOwnerID(owner), m_destroyEvent(NULL), m_stageEvent(NULL),
	m_warpOutTimer(NULL), m_mastNPC(NULL), m_hydraBoss(NULL), m_rewardChest(NULL), m_bEggsSpawned(false), m_bStage(STAGE_WAITING),
	m_runStart(0), m_runEnd(0)
{
	memset(m_hydraHeads, 0, sizeof(m_hydraHeads));
	if (test_server)
		bDungeonWaitingTime = 5;
}

CHydraDungeon::~CHydraDungeon()
{
	DestroyDungeon();
}

void CHydraDungeon::OnLogin(LPCHARACTER ch)
{
	if (m_destroyEvent)
		event_cancel(&m_destroyEvent);

	if (!m_stageEvent && m_bStage == STAGE_WAITING)
	{
		stage_event_info* pInfo = AllocEventInfo<stage_event_info>();
		pInfo->pDungeon = this;
		m_stageEvent = event_create(stage_event, pInfo, PASSES_PER_SEC(bDungeonWaitingTime));
	}

	SendStageNotice(ch);
	if (m_hydraBoss && (!m_bEggsSpawned || !m_hydraEggs.empty()))
	{
		HydraDebuff f(true);
		f(ch);
	}

	int currHP = 0;
	int maxHP = 0;
	if (m_mastNPC)
	{
		currHP = m_mastNPC->GetHP();
		maxHP = m_mastNPC->GetMaxHP();
	}
	SendMastHP f(currHP, maxHP);
	f(ch);

	m_vecPC.push_back(ch);
	sys_log(0, "adding %d to hydra map", ch->GetPlayerID());
}

void CHydraDungeon::OnLogout(LPCHARACTER ch)
{
	m_vecPC.erase(std::remove(m_vecPC.begin(), m_vecPC.end(), ch), m_vecPC.end());
	if (m_vecPC.empty() && !m_destroyEvent)
	{
		map_destroy_event_info* pEvent = AllocEventInfo<map_destroy_event_info>();
		pEvent->lMapIndex = m_lMapIndex;
		m_destroyEvent = event_create(map_destroy_event, pEvent, PASSES_PER_SEC(DUNGEON_TIMEOUT));
	}

	/*if (m_bStage == STAGE_COMPLETED)
		ch->SetQuestFlag("hydra_dungeon.next_entry", get_global_time() + HYDRA_DUNGEON_COOLDOWN);*/
	sys_log(0, "removing %d from hydra map", ch->GetPlayerID());
}

void CHydraDungeon::OnKill(LPCHARACTER ch, LPCHARACTER victim)
{
	if (victim->IsMonster())
	{
		auto it = m_monsterKeep.find(victim->GetVID());
		if (it != m_monsterKeep.end())
			m_monsterKeep.erase(it);

		if (victim == m_hydraBoss)
		{
			m_hydraBoss = NULL;
			if (m_bStage == STAGE_THREE)
			{
				SendHydraNotice f("One less head to worry about! Keep up the good work!");
				ForEachMember(f);
			}
			else if (m_bStage == STAGE_SIX)
			{
				SendHydraNotice f("Another one falls... Don't lose focus!");
				ForEachMember(f);
			}
			else if (m_bStage == STAGE_NINE)
			{
				SendHydraNotice f("Yesss! That was the last one!");
				ForEachMember(f);
			}
		}

		ProcessStage(false);
	}
	else if (victim->IsStone())
	{
		auto it = m_hydraEggs.find(victim->GetVID());
		if (it != m_hydraEggs.end())
			m_hydraEggs.erase(it);
		
		if (m_hydraEggs.empty())
		{
			m_hydraBoss->ResetArmada();
			m_hydraBoss->LockOnTarget(false);

			HydraDebuff f(false);
			ForEachMember(f);

			SendStageNotice();
		}
		ProcessStage(false);
	}

	if (victim == m_mastNPC)
	{
		m_mastNPC = NULL;
		ProcessFailure();
	}
}

void CHydraDungeon::ProcessDestroy(LPCHARACTER ch)
{
	if (ch == m_mastNPC)
	{
		m_mastNPC = NULL;
		sys_log(0, "mast npc set to null on hydra map %d", m_lMapIndex);
		return;
	}
	else if (ch == m_hydraBoss)
	{
		m_hydraBoss = NULL;
		return;
	}
	else if (ch == m_rewardChest)
	{
		m_rewardChest = NULL;
		return;
	}

	if (ch->IsMonster())
	{
		auto it = m_monsterKeep.find(ch->GetVID());
		if (it != m_monsterKeep.end())
		{
			m_monsterKeep.erase(it);
			return;
		}

		for (BYTE i = 0; i < HYDRA_HEADS_MAX_NUM; ++i)
			if (m_hydraHeads[i] == ch)
			{
				m_hydraHeads[i] = NULL;
				return;
			}
	}
	else if (ch->IsStone())
	{
		auto it = m_hydraEggs.find(ch->GetVID());
		if (it != m_hydraEggs.end())
			m_hydraEggs.erase(it);
	}
}

long CHydraDungeon::map_destroy_event(LPEVENT event, long processing_time)
{
	map_destroy_event_info* pEventInfo = dynamic_cast<map_destroy_event_info*>(event->info);
	if (!pEventInfo)
	{
		sys_err("<null factor> map_destroy_event_info is invalid");
		return 0;
	}

	CHydraDungeonManager::instance().DestroyDungeon(pEventInfo->lMapIndex);
	return 0;
}

void CHydraDungeon::DestroyDungeon()
{
	WarpToExit();
	if (m_destroyEvent)
		event_cancel(&m_destroyEvent);

	if (m_stageEvent)
		event_cancel(&m_stageEvent);

	if (m_warpOutTimer)
		event_cancel(&m_warpOutTimer);

	if (m_mastNPC)
		M2_DESTROY_CHARACTER(m_mastNPC); // no need to set m_mastNPC to 0 cus its done in the char's dtor

	if (m_hydraBoss)
		M2_DESTROY_CHARACTER(m_hydraBoss);

	if (m_rewardChest)
		M2_DESTROY_CHARACTER(m_rewardChest);

	for (BYTE i = 0; i < HYDRA_HEADS_MAX_NUM; ++i)
		if (m_hydraHeads[i])
			M2_DESTROY_CHARACTER(m_hydraHeads[i]);

	m_bEggsSpawned = false;
	m_bStage = STAGE_WAITING;

	if (m_lMapIndex)
		SECTREE_MANAGER::instance().DestroyPrivateMap(m_lMapIndex);

	m_vecPC.clear();
	m_monsterKeep.clear();
	m_hydraEggs.clear();
	m_runEnd = m_runStart = 0;
}

void CHydraDungeon::WarpToExit(LPCHARACTER ch)
{
	HydraDungeonWarp f(DUNGEON_EXIT_MAPINDEX, 0, WARP_EXIT_POS_X, WARP_EXIT_POS_Y, false);
	if (ch)
		f(ch);
	else
		ForEachMember(f);
}

void CHydraDungeon::ClearMonsters()
{
	LPSECTREE_MAP map = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (!map)
		return;

	KillAllMob f;
	map->for_each(f);
}

long CHydraDungeon::CreateDungeon()
{
	DestroyDungeon();

	m_lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(DUNGEON_MAPINDEX);

	if (m_lMapIndex)
	{
		map_destroy_event_info* pEvent = AllocEventInfo<map_destroy_event_info>();
		pEvent->lMapIndex = m_lMapIndex;
		m_destroyEvent = event_create(map_destroy_event, pEvent, PASSES_PER_SEC(DUNGEON_TIMEOUT));

		PIXEL_POSITION basePos;
		SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(m_lMapIndex, basePos);
		m_mastNPC = CHARACTER_MANAGER::instance().SpawnMob(MAST_VNUM, m_lMapIndex, basePos.x + MAST_X * 100, basePos.y + MAST_Y * 100, 0);
		if (m_mastNPC)
		{
			if (FORCE_SET_MAST_NPC_HP)
			{
				m_mastNPC->SetMaxHP(FORCE_SET_MAST_NPC_HP);
				m_mastNPC->SetHP(m_mastNPC->GetMaxHP());
			}
		}
		else
			return 0;

		for (BYTE i = 0; i < HYDRA_HEADS_MAX_NUM; ++i)
		{
			const SpawnPos curr = hydraBaseSpawn[i];
			m_hydraHeads[i] = CHARACTER_MANAGER::instance().SpawnMob(curr.vnum, m_lMapIndex, basePos.x + curr.x * 100, basePos.y + curr.y * 100, 0, false, 180);
			if (!m_hydraHeads[i])
			{
				sys_err("failed to spawn hydra head %d (%d)", curr.vnum, i);
				return 0;
			}

			m_hydraHeads[i]->SetArmada();
		}
	}

	return m_lMapIndex;
}



////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Gameplay handling ///////////////////////////////////////

long CHydraDungeon::stage_event(LPEVENT event, long processing_time)
{
	stage_event_info* pEventInfo = dynamic_cast<stage_event_info*>(event->info);
	if (!pEventInfo)
	{
		sys_err("<null factor> stage_event_info is invalid");
		return 0;
	}

	CHydraDungeon* pDungeon = pEventInfo->pDungeon;
	long nextTime = pDungeon->ProcessStage(true);
	if (nextTime)
		return PASSES_PER_SEC(nextTime);

	pDungeon->ClearStageEvent();
	return 0;
}

long CHydraDungeon::ProcessStage(bool isTimerEvent)
{
	//bool needEvent = m_stageEvent == NULL;
	long ret = 0;
	switch (m_bStage)
	{
		case STAGE_WAITING:
			m_runStart = get_global_time();
			if (!test_server)
			{
				SetStage(STAGE_ONE);
				ret = SIDEWAVE_TIMEOUT;
			}
			else
			{
				SetStage((EStages)TEST_START_STAGE);
				ret = TEST_STAGE_RETURN_TIME;
			}
			break;

		case STAGE_ONE:
		case STAGE_FOUR:
		case STAGE_SEVEN:
			if (m_monsterKeep.empty())
			{
				ret = SIDEWAVE_TIMEOUT;

				if (m_stageEvent && !isTimerEvent)
					event_reset_time(m_stageEvent, PASSES_PER_SEC(ret));

				SetStage((EStages)((BYTE)m_bStage + 1));
			}
			else if (isTimerEvent)
				ProcessFailure();
			break;

		case STAGE_TWO:
		case STAGE_FIVE:
		case STAGE_EIGHT:
			if (m_monsterKeep.empty())
			{
				ret = 0;
				if (m_stageEvent && !isTimerEvent)
					event_cancel(&m_stageEvent);

				SetStage((EStages)((BYTE)m_bStage + 1));
			}
			else if (isTimerEvent)
				ProcessFailure();
			break;

		case STAGE_THREE:
		case STAGE_SIX:
		case STAGE_NINE:
			if (!m_hydraBoss)
			{
				m_bEggsSpawned = false;
				ret = SIDEWAVE_TIMEOUT;
				if (!m_stageEvent && !isTimerEvent)
				{
					stage_event_info* pInfo = AllocEventInfo<stage_event_info>();
					pInfo->pDungeon = this;
					m_stageEvent = event_create(stage_event, pInfo, PASSES_PER_SEC(ret));
				}

				if (m_bStage == STAGE_NINE)
				{
					SetStage(STAGE_COMPLETED);
					ret = 0;
				}
				else
					SetStage((EStages)((BYTE)m_bStage + 1));
			}
			break;
	}

	/*if (needEvent && ret && !isTimerEvent)
	{
		stage_event_info* pInfo = AllocEventInfo<stage_event_info>();
		pInfo->pDungeon = this;
		m_stageEvent = event_create(stage_event, pInfo, PASSES_PER_SEC(ret));
	}*/
	return ret;
}

void CHydraDungeon::SetStage(EStages nextStage)
{
	m_bStage = nextStage;

	if (nextStage != STAGE_FAIL)
		for (auto it = m_monsterKeep.cbegin(); it != m_monsterKeep.cend();)
		{
			LPCHARACTER curr = it->second;
			m_monsterKeep.erase(it++);
			M2_DESTROY_CHARACTER(curr);
		}

	if (nextStage < waveSpawn.size())
	{
		PIXEL_POSITION basePos;
		bool found = SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(m_lMapIndex, basePos);

		for (auto it = waveSpawn[nextStage].begin(); it != waveSpawn[nextStage].end() && found; ++it)
		{
			const SpawnPos curr = *it;

			LPCHARACTER spawn = CHARACTER_MANAGER::instance().SpawnMob(curr.vnum, m_lMapIndex, basePos.x + curr.x * 100, basePos.y + curr.y * 100, 0);
			if (spawn)
			{
				if (curr.isLockTarget)
				{
					spawn->SetVictim(m_mastNPC);
					spawn->SetPosition(POS_FIGHTING);
					spawn->SetNextStatePulse(1);
					spawn->LockOnTarget(true);
				}

				m_monsterKeep[spawn->GetVID()] = spawn;
			}
		}

		if (found && (nextStage == CHydraDungeon::STAGE_THREE || nextStage == CHydraDungeon::STAGE_SIX || nextStage == CHydraDungeon::STAGE_NINE))
		{
			m_bEggsSpawned = false;
			BYTE currHead = MINMAX(0, nextStage / 3, HYDRA_HEADS_MAX_NUM - 1);
			if (m_hydraHeads[currHead])
			{
				M2_DESTROY_CHARACTER(m_hydraHeads[currHead]);
				m_hydraHeads[currHead] = NULL;
			}

			if (m_hydraBoss)
				M2_DESTROY_CHARACTER(m_hydraBoss);

			m_hydraBoss = CHARACTER_MANAGER::instance().SpawnMob(HYDRA_BOSS_VNUM, m_lMapIndex, basePos.x + HYDRA_BOSS_X * 100, basePos.y + HYDRA_BOSS_Y * 100, 0, false, 355);
			if (m_hydraBoss && HYDRA_BOSS_FORCE_ATTACK_RANGE)
				m_hydraBoss->SetForceMonsterAttackRange(HYDRA_BOSS_FORCE_ATTACK_RANGE);

			HydraDebuff f(true);
			ForEachMember(f);
		}
	}

	if (nextStage == STAGE_COMPLETED && !m_rewardChest)
	{
		m_runEnd = get_global_time();
		
		PIXEL_POSITION basePos;
		if (SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(m_lMapIndex, basePos))
			m_rewardChest = CHARACTER_MANAGER::instance().SpawnMob(HYDRA_CHEST_VNUM, m_lMapIndex, basePos.x + HYDRA_CHEST_X * 100, basePos.y + HYDRA_CHEST_Y * 100, 0, false, HYDRA_CHEST_ROTATION);

		if (!m_warpOutTimer)
		{
			map_destroy_event_info* pEvent = AllocEventInfo<map_destroy_event_info>();
			pEvent->lMapIndex = m_lMapIndex;
			m_warpOutTimer = event_create(map_destroy_event, pEvent, PASSES_PER_SEC(DUNGEON_WARPOUT_COMPLETE_TIME));
		}
	}

	SendStageNotice();
}

void CHydraDungeon::ProcessFailure()
{
	if (m_stageEvent)
		event_cancel(&m_stageEvent);

	if (m_destroyEvent)
		event_cancel(&m_destroyEvent);

	if (m_warpOutTimer)
		event_cancel(&m_warpOutTimer);

	BYTE currStage = m_bStage;
	SetStage(STAGE_FAIL);

	ClearMonsters();

	SendHydraNotice f("You failed to complete stage %u. You will be warped out in %d seconds!");
	f.printer.PushArg(currStage + 1);
	f.printer.PushArg(DUNGEON_WARPOUT_TIME);
	ForEachMember(f);

	map_destroy_event_info* pEvent = AllocEventInfo<map_destroy_event_info>();
	pEvent->lMapIndex = m_lMapIndex;
	m_warpOutTimer = event_create(map_destroy_event, pEvent, PASSES_PER_SEC(DUNGEON_WARPOUT_TIME));
}

void CHydraDungeon::SendStageNotice(LPCHARACTER ch)
{
	if (m_bStage == CHydraDungeon::STAGE_ONE || m_bStage == CHydraDungeon::STAGE_TWO || m_bStage == CHydraDungeon::STAGE_FOUR ||
		m_bStage == CHydraDungeon::STAGE_FIVE || m_bStage == CHydraDungeon::STAGE_SEVEN || m_bStage == CHydraDungeon::STAGE_EIGHT)
	{
		long remainingTime;
		if (!ch)
			remainingTime = SIDEWAVE_TIMEOUT;
		else
			remainingTime = m_stageEvent ? SIDEWAVE_TIMEOUT - event_processing_time(m_stageEvent) / passes_per_sec : 0;

		SendHydraNotice f("<Stage %d> You have %d seconds to kill all monsters!");
		f.printer.PushArg(m_bStage + 1);
		f.printer.PushArg((long long)remainingTime);
		if (ch)
			f(ch);
		else
			ForEachMember(f);
	}
	else if (m_bStage == CHydraDungeon::STAGE_THREE || m_bStage == CHydraDungeon::STAGE_SIX || m_bStage == CHydraDungeon::STAGE_NINE)
	{
		if (!m_hydraEggs.empty())
		{
			SendHydraNotice f("<Stage %d> Destroy the Hydra Eggs!");
			f.printer.PushArg(m_bStage + 1);
			if (ch)
				f(ch);
			else
				ForEachMember(f);
		}
		else
		{
			SendHydraNotice f("<Stage %d> Kill the Hydra's head before it sinks the ship!");
			f.printer.PushArg(m_bStage + 1);
			if (ch)
				f(ch);
			else
				ForEachMember(f);
		}
	}
	else if (m_bStage == CHydraDungeon::STAGE_WAITING)
	{
		long startTime = m_stageEvent ? bDungeonWaitingTime - event_processing_time(m_stageEvent) / passes_per_sec : 0;
		SendHydraNotice f("<Waiting Stage> The Hydra Run will begin in %d seconds!");
		f.printer.PushArg((long long)startTime);
		if (ch)
			f(ch);
		else
			ForEachMember(f);
	}
	else if (m_bStage == CHydraDungeon::STAGE_COMPLETED)
	{
		BYTE hours, mins, secs;
		CalcRunTime(m_runEnd - m_runStart, hours, mins, secs);
		SendHydraNotice f("Congratulations! You have successfully completed the Hydra Dungeon under %02u:%02u:%02u!");
		f.printer.PushArg(hours);
		f.printer.PushArg(mins);
		f.printer.PushArg(secs);
		if (ch)
			f(ch);
		else
			ForEachMember(f);

		CalcRunTime(m_warpOutTimer ? event_time(m_warpOutTimer) / passes_per_sec : DUNGEON_WARPOUT_COMPLETE_TIME, hours, mins, secs);
		SendHydraNotice f2("Claim your rewards! The dungeon will dissapear and you will be teleported out in %02u:%02u!");
		f2.printer.PushArg(mins);
		f2.printer.PushArg(secs);
		if (ch)
			f2(ch);
		else
			ForEachMember(f2);
	}
}

void CHydraDungeon::NotifyHydraDmg(LPCHARACTER hydra, int* dmg)
{
	if (hydra == m_mastNPC)
	{
		SendMastHP f(hydra->GetHP() - *dmg, hydra->GetMaxHP());
		ForEachMember(f);
		return;
	}

	if (hydra != m_hydraBoss || m_bEggsSpawned)
		return;

	int maxHP = hydra->GetMaxHP();
	if (hydra->GetHP() - *dmg <= maxHP / 2)
	{
		hydra->SetHP(maxHP);
		*dmg = 0;
		hydra->SetArmada();
		hydra->SetVictim(m_mastNPC);
		hydra->SetPosition(POS_FIGHTING);
		hydra->SetNextStatePulse(1);
		hydra->LockOnTarget(true);

		m_bEggsSpawned = true;

		PIXEL_POSITION basePos;
		bool found = SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(m_lMapIndex, basePos);
		for (auto it = hydraEggSpawn.begin(); it != hydraEggSpawn.end() && found; ++it)
		{
			const SpawnPos curr = *it;
			LPCHARACTER spawn = CHARACTER_MANAGER::instance().SpawnMob(curr.vnum, m_lMapIndex, basePos.x + curr.x * 100, basePos.y + curr.y * 100, 0, true, -1, true);
			if (spawn)
			{
				spawn->SetAggressive();
				m_hydraEggs[spawn->GetVID()] = spawn;
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// Manager functions ///////////////////////////////////////


CHydraDungeonManager::CHydraDungeonManager()
{
}

CHydraDungeonManager::~CHydraDungeonManager()
{
	Destroy();
}

void CHydraDungeonManager::Destroy()
{
	for (auto it = m_mapDungeon.begin(); it != m_mapDungeon.end(); ++it)
		M2_DELETE(it->second);

	m_mapDungeon.clear();
}

bool CHydraDungeonManager::CreateDungeon(LPCHARACTER owner)
{
	if (!owner || !owner->IsPC()) return false;

	CHydraDungeon* newDungeon = M2_NEW CHydraDungeon(owner->GetPlayerID());
	long lMapIndex = newDungeon->CreateDungeon();
	if (lMapIndex && m_mapDungeon.find(lMapIndex) == m_mapDungeon.end())
	{
		sys_log(0, "hydra dungeon created with id %d", lMapIndex);
		m_mapDungeon[lMapIndex] = newDungeon;

		HydraDungeonWarp f(lMapIndex, owner->GetPlayerID());
		if (owner->GetParty())
			owner->GetParty()->ForEachNearMember(f);
		else
			f(owner);
		return true;
	}
	else
		M2_DELETE(newDungeon);

	sys_err("failed to create hydra dungeon id: %d", lMapIndex);
	return false;
}

void CHydraDungeonManager::DestroyDungeon(long lMapIndex)
{
	auto it = m_mapDungeon.find(lMapIndex);
	if (it != m_mapDungeon.end())
	{
		sys_log(0, "destroying hydra dungeon with mapindex %d", lMapIndex);
		M2_DELETE(it->second);
		m_mapDungeon.erase(it);
	}
}

void CHydraDungeonManager::OnLogin(LPCHARACTER ch)
{
	if (!ch->IsPC())
		return;

	if (ch->IsPrivateMap(DUNGEON_MAPINDEX))
	{
		sys_log(0, "%d joining to hydra dungeon (%d)", ch->GetPlayerID(), ch->GetMapIndex());
		int oldOwner = ch->GetQuestFlag("hydra_dungeon.current_owner_" + std::to_string(g_bChannel));
		auto it = m_mapDungeon.find(ch->GetMapIndex());
		if (it != m_mapDungeon.end() && it->second->GetOwner() == oldOwner && it->second->GetStage() != CHydraDungeon::STAGE_FAIL)
		{
			it->second->OnLogin(ch);
			ch->SaveExitLocation();
			return;
		}
	}
	else if (ch->GetMapIndex() == DUNGEON_MAPINDEX)
	{
		int oldIndex = ch->GetQuestFlag("hydra_dungeon.current_id_" + std::to_string(g_bChannel));
		int oldOwner = ch->GetQuestFlag("hydra_dungeon.current_owner_" + std::to_string(g_bChannel));
		auto it = m_mapDungeon.find(oldIndex);
		if (it != m_mapDungeon.end() && it->second->GetOwner() == oldOwner && it->second->GetStage() != CHydraDungeon::STAGE_FAIL)
		{
			sys_log(0, "%d warping back to hydra dungeon (%d)", ch->GetPlayerID(), oldIndex);
			HydraDungeonWarp f(oldIndex, oldOwner);
			f(ch);
			return;
		}
	}
	else
		return;

	sys_log(0, "%d warping out from hydra to exit pos (map: %d)", ch->GetPlayerID(), ch->GetMapIndex());
	HydraDungeonWarp f(DUNGEON_EXIT_MAPINDEX, 0, WARP_EXIT_POS_X, WARP_EXIT_POS_Y, false);
	f(ch);
}

#endif