#include "stdafx.h"
#include "MeleyLair.h"

#ifdef __MELEY_LAIR_DUNGEON__
#include "db.h"
#include "log.h"
#include "item.h"
#include "char.h"
#include "utils.h"
#include "party.h"
#include "regen.h"
#include "config.h"
#include "packet.h"
#include "motion.h"
#include "item_manager.h"
#include "guild_manager.h"
#include "start_position.h"
#include "locale.hpp"
#include <boost/lexical_cast.hpp> 

namespace MeleyLair
{
	int stoneSpawnPos[MOBCOUNT_RESPAWN_STONE_STEP2][2] =
	{
		{140, 131},
		{130, 122},
		{121, 128},
		{128, 140}
	};
	
	int monsterSpawnPos[MOBCOUNT_RESPAWN_COMMON_STEP][2] =
	{
		{140, 131},
		{130, 122},
		{121, 128},
		{128, 140},
		{128, 142},
		{128, 131},
		{135, 130},
		{141, 126},
		{128, 122},
		{117, 127},
		{118, 136},
		{126, 142}
	};
	
	struct FNotice
	{
		FNotice(const char * psz) : m_psz(psz) {}
		void operator() (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pkChar = (LPCHARACTER) ent;
				pkChar->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkChar, m_psz));
			}
		}
		
		const char * m_psz;
	};
	
	struct FExitAndGoTo
	{
		FExitAndGoTo() {};
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pkChar = (LPCHARACTER) ent;
				if (pkChar->IsPC())
				{
					PIXEL_POSITION posSub = CMgr::instance().GetSubXYZ();
					if (!posSub.x)
						pkChar->WarpSet(EMPIRE_START_X(pkChar->GetEmpire()), EMPIRE_START_Y(pkChar->GetEmpire()));
					else
						pkChar->WarpSet(posSub.x, posSub.y);
				}
			}
		}
	};
	
	EVENTINFO(r_meleystatues_info)
	{
		bool	bFirst;
		CMgrMap*	pMap;
	};
	
	EVENTFUNC(r_meleystatues_event)
	{
		r_meleystatues_info* pEventInfo = dynamic_cast<r_meleystatues_info*>(event->info);
		if (pEventInfo)
		{
			if (pEventInfo->bFirst)
			{
				CMgrMap* pMap = pEventInfo->pMap;
				if (pMap)
					pMap->DungeonResult();
			}
			else
			{
				pEventInfo->bFirst = true;
				return PASSES_PER_SEC((DWORD)(MeleyLair::TIME_LIMIT_TO_KILL_STATUE));
			}
		}
		
		return 0;
	}
	
	EVENTINFO(r_meleylimit_info)
	{
		bool	bWarp;
		CMgrMap*	pMap;
	};
	
	EVENTFUNC(r_meleylimit_event)
	{
		r_meleylimit_info* pEventInfo = dynamic_cast<r_meleylimit_info*>(event->info);
		if (pEventInfo)
		{
			CMgrMap* pMap = pEventInfo->pMap;
			if (pMap)
			{
				if (pEventInfo->bWarp)
					pMap->EndDungeonWarp();
				else
					pMap->EndDungeon(false, false);
			}
		}
		
		return 0;
	}
	
	EVENTINFO(r_meleyspawn_info)
	{
		CMgrMap*	pMap;
		bool	bFirst;
		BYTE	bStep;
		DWORD	dwTimeReload, dwMobVnum, dwMobCount;
	};
	
	EVENTFUNC(r_meleyspawn_event)
	{
		r_meleyspawn_info* pEventInfo = dynamic_cast<r_meleyspawn_info*>(event->info);
		if (!pEventInfo)
			return 0;
		
		CMgrMap* pMap = pEventInfo->pMap;
		if (!pMap)
			return 0;
		
		if (!pMap->GetMapSectree())
			return 0;

		DWORD dwTimeReload = pEventInfo->dwTimeReload, dwMobVnum = pEventInfo->dwMobVnum, dwMobCount = pEventInfo->dwMobCount;
		size_t mob_count = SECTREE_MANAGER::instance().GetMonsterCountInMap(pMap->GetMapIndex(), dwMobVnum, true);
		bool bRespawnCommon = dwMobCount > mob_count ? true : false;
		if (bRespawnCommon)
		{
			DWORD dwDiff = dwMobCount - mob_count;
			for (DWORD i = 0; i < dwDiff; ++i)
			{
				DWORD dwRandom = random_number(3, 10);
				pMap->Spawn(dwMobVnum, monsterSpawnPos[i][0], monsterSpawnPos[i][1] + dwRandom, 0);
			}
		}
		
		if ((pEventInfo->bStep == 2) || (pEventInfo->bStep == 3))
		{
			size_t stones_count = SECTREE_MANAGER::instance().GetMonsterCountInMap(pMap->GetMapIndex(), (DWORD)(MeleyLair::MOBVNUM_RESPAWN_STONE_STEP2), true);
			if (stones_count == 0)
			{
				DWORD dwTime = get_global_time();
				//if (dwTime >= pMap->GetLastStoneKilledTime())
				if (pMap->GetLastStoneKilledTime() == 0)
				{
					for (DWORD i = 0; i < MOBCOUNT_RESPAWN_STONE_STEP2; ++i)
					{
						DWORD dwRandom = random_number(3, 10);
						pMap->Spawn((DWORD)(MeleyLair::MOBVNUM_RESPAWN_STONE_STEP2), stoneSpawnPos[i][0], stoneSpawnPos[i][1] + dwRandom, 0);
					}
					
					if (pEventInfo->bStep == 3)
					{
						if (pMap->GetStatue1Char() && !pMap->GetStatue1Char()->IsAffectFlag(AFF_STATUE3))
							pMap->GetStatue1Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						if (pMap->GetStatue2Char() && !pMap->GetStatue2Char()->IsAffectFlag(AFF_STATUE3))
							pMap->GetStatue2Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						if (pMap->GetStatue3Char() && !pMap->GetStatue3Char()->IsAffectFlag(AFF_STATUE3))
							pMap->GetStatue3Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						if (pMap->GetStatue4Char() && !pMap->GetStatue4Char()->IsAffectFlag(AFF_STATUE3))
							pMap->GetStatue4Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						pMap->SetKillCountStones(0);
					}
					else
					{
						if (pMap->GetStatue1Char() && !pMap->GetStatue1Char()->IsAffectFlag(AFF_STATUE2))
							pMap->GetStatue1Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						if (pMap->GetStatue2Char() && !pMap->GetStatue2Char()->IsAffectFlag(AFF_STATUE2))
							pMap->GetStatue2Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						if (pMap->GetStatue3Char() && !pMap->GetStatue3Char()->IsAffectFlag(AFF_STATUE2))
							pMap->GetStatue3Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
						
						if (pMap->GetStatue4Char() && !pMap->GetStatue4Char()->IsAffectFlag(AFF_STATUE2))
							pMap->GetStatue4Char()->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
					}
				}
			}
			
			if ((pEventInfo->bStep == 3) && (pEventInfo->bFirst))
			{
				for (DWORD i = 0; i < MOBCOUNT_RESPAWN_BOSS_STEP3; ++i)
				{
					DWORD dwRandom = random_number(3, 10);
					if(!pMap->Spawn((DWORD)(MeleyLair::MOBVNUM_RESPAWN_BOSS_STEP3), stoneSpawnPos[i][0], stoneSpawnPos[i][1] + dwRandom, 0))
						sys_err("ERROR SPAWN GUARD %s:%d", __FILE__, __LINE__);
					if(!pMap->Spawn((DWORD)(MeleyLair::MOBVNUM_RESPAWN_SUBBOSS_STEP3), stoneSpawnPos[i][0], stoneSpawnPos[i][1] + dwRandom, 0))
						sys_err("ERROR SPAWN GUARD %s:%d", __FILE__, __LINE__);
					if(!pMap->Spawn((DWORD)(MeleyLair::MOBVNUM_RESPAWN_SUBBOSS_STEP3), stoneSpawnPos[i][0], stoneSpawnPos[i][1] + dwRandom, 0))
						sys_err("ERROR SPAWN GUARD %s:%d", __FILE__, __LINE__);
				}
				
				pEventInfo->bFirst = false;
			}
		}
		
		return PASSES_PER_SEC(dwTimeReload);
	}
	
	EVENTINFO(r_meleyeffect_info)
	{
		CMgrMap*	pMap;
		BYTE	bStep;
		BYTE	bEffectStep;
	};
	
	EVENTFUNC(r_meleyeffect_event)
	{
		r_meleyeffect_info* pEventInfo = dynamic_cast<r_meleyeffect_info*>(event->info);
		if (!pEventInfo)
			return 0;
		
		CMgrMap* pMap = pEventInfo->pMap;
		if (!pMap)
			return 0;
		
		if ((!pMap->GetBossChar()) || (!pMap->GetStatue1Char()) || (!pMap->GetStatue2Char()) || (!pMap->GetStatue3Char()) || (!pMap->GetStatue4Char()))
			return 0;
		
		BYTE bStep = pEventInfo->bStep, bEffectStep = pEventInfo->bEffectStep;
		if (bStep == 1)
		{
			if (bEffectStep == 1)
			{
				#ifdef __LASER_EFFECT_ON_75HP__
				time_t timeNow = static_cast<time_t>(get_dword_time());
				
				if (pMap->GetMapSectree())
				{
					PIXEL_POSITION pos = MeleyLair::CMgr::instance().GetXYZ();
					pos.x = pMap->GetMapSectree()->m_setting.iBaseX + 130 * 100;
					pos.y = pMap->GetMapSectree()->m_setting.iBaseY + 77 * 100;
					pos.z = 0;
					pMap->GetStatue1Char()->SetRotationToXY(pos.x, pos.y);
					pMap->GetStatue2Char()->SetRotationToXY(pos.x, pos.y);
					pMap->GetStatue3Char()->SetRotationToXY(pos.x, pos.y);
					pMap->GetStatue4Char()->SetRotationToXY(pos.x, pos.y);
				}
				
				pMap->GetStatue1Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue1Char()->GetX(), pMap->GetStatue1Char()->GetY(), 0, timeNow);
				pMap->GetStatue2Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue2Char()->GetX(), pMap->GetStatue2Char()->GetY(), 0, timeNow);
				pMap->GetStatue3Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue3Char()->GetX(), pMap->GetStatue3Char()->GetY(), 0, timeNow);
				pMap->GetStatue4Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue4Char()->GetX(), pMap->GetStatue4Char()->GetY(), 0, timeNow);
				#endif
				
				pEventInfo->bEffectStep = 2;
			}
			else
			{
				pMap->StartDungeonStep(2);
				return 0;
			}
		}
		else if (bStep == 2)
		{
			if (bEffectStep == 1)
			{
				#ifdef __LASER_EFFECT_ON_75HP__
				time_t timeNow = static_cast<time_t>(get_dword_time());
				
				if (pMap->GetMapSectree())
				{
					PIXEL_POSITION pos = MeleyLair::CMgr::instance().GetXYZ();
					pos.x = pMap->GetMapSectree()->m_setting.iBaseX + 130 * 100;
					pos.y = pMap->GetMapSectree()->m_setting.iBaseY + 77 * 100;
					pos.z = 0;
					pMap->GetStatue1Char()->SetRotationToXY(pos.x, pos.y);
					pMap->GetStatue2Char()->SetRotationToXY(pos.x, pos.y);
					pMap->GetStatue3Char()->SetRotationToXY(pos.x, pos.y);
					pMap->GetStatue4Char()->SetRotationToXY(pos.x, pos.y);
				}
				
				pMap->GetStatue1Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue1Char()->GetX(), pMap->GetStatue1Char()->GetY(), 0, timeNow);
				pMap->GetStatue2Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue2Char()->GetX(), pMap->GetStatue2Char()->GetY(), 0, timeNow);
				pMap->GetStatue3Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue3Char()->GetX(), pMap->GetStatue3Char()->GetY(), 0, timeNow);
				pMap->GetStatue4Char()->SendMovePacket(FUNC_MOB_SKILL, 0, pMap->GetStatue4Char()->GetX(), pMap->GetStatue4Char()->GetY(), 0, timeNow);
				#endif
				
				pEventInfo->bEffectStep = 2;
			}
			else
			{
				pMap->StartDungeonStep(3);
				return 0;
			}
		}
		
		return PASSES_PER_SEC(5);
	}
	
	CMgrMap::CMgrMap(long lMapIndex, DWORD dwGuildID)
	{
		e_pEndEvent = NULL;
		e_pWarpEvent = NULL;
		e_SpawnEvent = NULL;
		e_SEffectEvent = NULL;
		e_DestroyStatues = NULL;
		v_Partecipants.clear();
		v_Already.clear();
		v_Rewards.clear();
		map_index = lMapIndex;
		guild_id = dwGuildID;
		dungeon_step = 0;
		reward = 0;
		pkSectreeMap = SECTREE_MANAGER::instance().GetMap(map_index);
		pkMainNPC = NULL, pkGate = NULL, pkBoss = NULL, pkStatue1 = NULL, pkStatue2 = NULL, pkStatue3 = NULL, pkStatue4 = NULL;
		Start();
	}
	
	CMgrMap::~CMgrMap()
	{
		if (e_pEndEvent != NULL)
			event_cancel(&e_pEndEvent);
		
		if (e_pWarpEvent != NULL)
			event_cancel(&e_pWarpEvent);
		
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		
		if (e_SEffectEvent != NULL)
			event_cancel(&e_SEffectEvent);
		
		if (e_DestroyStatues != NULL)
			event_cancel(&e_DestroyStatues);
	}
	
	void CMgrMap::Destroy()
	{
		if (e_pEndEvent != NULL)
			event_cancel(&e_pEndEvent);
		
		if (e_pWarpEvent != NULL)
			event_cancel(&e_pWarpEvent);
		
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		
		if (e_SEffectEvent != NULL)
			event_cancel(&e_SEffectEvent);
		
		if (e_DestroyStatues != NULL)
			event_cancel(&e_DestroyStatues);
		
		e_pEndEvent = NULL;
		e_pWarpEvent = NULL;
		e_SpawnEvent = NULL;
		e_SEffectEvent = NULL;
		e_DestroyStatues = NULL;
		v_Partecipants.clear();
		v_Already.clear();
		v_Rewards.clear();
		map_index = 0;
		guild_id = 0;
		dungeon_step = 0;
		reward = 0;
		pkSectreeMap = NULL;
		pkMainNPC = NULL, pkGate = NULL, pkBoss = NULL, pkStatue1 = NULL, pkStatue2 = NULL, pkStatue3 = NULL, pkStatue4 = NULL;
	}
	
	DWORD CMgrMap::GetPartecipantsCount() const
	{
		return v_Partecipants.size();
	}
	
	void CMgrMap::Partecipant(bool bInsert, DWORD dwPlayerID)
	{
		if (bInsert)
		{
			bool bCheck = std::find(v_Partecipants.begin(), v_Partecipants.end(), dwPlayerID) != v_Partecipants.end();
			if (!bCheck)
				v_Partecipants.push_back(dwPlayerID);
		}
		else
			v_Partecipants.erase(std::remove(v_Partecipants.begin(), v_Partecipants.end(), dwPlayerID), v_Partecipants.end());
	}
	
	bool CMgrMap::IsPartecipant(DWORD dwPlayerID)
	{
		bool bCheck = std::find(v_Partecipants.begin(), v_Partecipants.end(), dwPlayerID) != v_Partecipants.end();
		return bCheck;
	}
	
	LPCHARACTER CMgrMap::Spawn(DWORD dwVnum, int iX, int iY, int iDir, bool bSpawnMotion)
	{
		if (!pkSectreeMap)
			return NULL;
		
		LPCHARACTER pkMob = CHARACTER_MANAGER::instance().SpawnMob(dwVnum, GetMapIndex(), pkSectreeMap->m_setting.iBaseX + iX * 100, pkSectreeMap->m_setting.iBaseY + iY * 100, 0, bSpawnMotion, iDir == 0 ? -1 : (iDir - 1) * 45);
		if (pkMob)
			sys_log(0, "<MeleyLair> Spawn: %s (map index: %d).", pkMob->GetName(), GetMapIndex());
		
		return pkMob;
	}
	
	void CMgrMap::SetDungeonStep(BYTE bStep)
	{
		if (bStep == dungeon_step)
			return;
		
		dungeon_step = bStep;
		
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		
		e_SpawnEvent = NULL;

		
		if (bStep == 1)
		{
			SetDungeonTimeStart(get_global_time());
			SetKillCountStones(0);
			r_meleyspawn_info* pEventInfo = AllocEventInfo<r_meleyspawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bFirst = false;
			pEventInfo->bStep = bStep;
			pEventInfo->dwTimeReload = (DWORD)(TIME_RESPAWN_COMMON_STEP1);
			pEventInfo->dwMobVnum = (DWORD)(MOBVNUM_RESPAWN_COMMON_STEP1);
			pEventInfo->dwMobCount = (DWORD)(MOBCOUNT_RESPAWN_COMMON_STEP);
			e_SpawnEvent = event_create(r_meleyspawn_event, pEventInfo, PASSES_PER_SEC(1));
		}
		else if (bStep == 2)
		{
			if ((!pkBoss) || (!pkStatue1) || (!pkStatue2) || (!pkStatue3) || (!pkStatue4))
				EndDungeon(false, true);
			
			if (e_SEffectEvent != NULL)
				event_cancel(&e_SEffectEvent);
			
			e_SEffectEvent = NULL;
			
			r_meleyeffect_info* pEventInfo = AllocEventInfo<r_meleyeffect_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = 1;
			pEventInfo->bEffectStep = 1;
			e_SEffectEvent = event_create(r_meleyeffect_event, pEventInfo, PASSES_PER_SEC(5));
		}
		else if (bStep == 3)
		{
			if ((!pkBoss) || (!pkStatue1) || (!pkStatue2) || (!pkStatue3) || (!pkStatue4))
				EndDungeon(false, true);
			
			if (e_SEffectEvent != NULL)
				event_cancel(&e_SEffectEvent);
			
			e_SEffectEvent = NULL;
			
			r_meleyeffect_info* pEventInfo = AllocEventInfo<r_meleyeffect_info>();
			pEventInfo->pMap = this;
			pEventInfo->bStep = 2;
			pEventInfo->bEffectStep = 1;
			e_SEffectEvent = event_create(r_meleyeffect_event, pEventInfo, PASSES_PER_SEC(5));
		}
		else if (bStep == 4)
		{
			if ((!pkBoss) || (!pkStatue1) || (!pkStatue2) || (!pkStatue3) || (!pkStatue4))
				EndDungeon(false, true);
			
			if (e_SEffectEvent != NULL)
				event_cancel(&e_SEffectEvent);
			
			if (e_DestroyStatues != NULL)
				event_cancel(&e_DestroyStatues);
			
			e_SEffectEvent = NULL;
			e_DestroyStatues = NULL;
			
			v_Already.clear();
			
			/*pkStatue1->SetArmada();
			pkStatue2->SetArmada();
			pkStatue3->SetArmada();
			pkStatue4->SetArmada();*/

			pkStatue1->RemoveAffect(AFFECT_STATUE);
			pkStatue2->RemoveAffect(AFFECT_STATUE);
			pkStatue3->RemoveAffect(AFFECT_STATUE);
			pkStatue4->RemoveAffect(AFFECT_STATUE);

			pkStatue1->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE4, 3600, 0, true);
			pkStatue2->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE4, 3600, 0, true);
			pkStatue3->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE4, 3600, 0, true);
			pkStatue4->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE4, 3600, 0, true);
			
			char szBuf[512];
			snprintf(szBuf, sizeof(szBuf), "You have %d second(s) to destroy the statues, hurry up!", TIME_LIMIT_TO_KILL_STATUE);
			FNotice f(szBuf);
			GetMapSectree()->for_each(f);
			
			r_meleystatues_info* pEventInfo = AllocEventInfo<r_meleystatues_info>();
			pEventInfo->bFirst = false;
			pEventInfo->pMap = this;
			e_DestroyStatues = event_create(r_meleystatues_event, pEventInfo, PASSES_PER_SEC(3));
		}
	}
	
	void CMgrMap::StartDungeonStep(BYTE bStep)
	{
		if (e_SpawnEvent != NULL)
			event_cancel(&e_SpawnEvent);
		
		if (e_SEffectEvent != NULL)
			event_cancel(&e_SEffectEvent);
		
		e_SpawnEvent = NULL;
		e_SEffectEvent = NULL;
		
		if (bStep == 2)
		{
			SetLastStoneKilledTime(0);
			r_meleyspawn_info* pEventInfo = AllocEventInfo<r_meleyspawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bFirst = false;
			pEventInfo->bStep = bStep;
			pEventInfo->dwTimeReload = (DWORD)(TIME_RESPAWN_COMMON_STEP2);
			pEventInfo->dwMobVnum = (DWORD)(MOBVNUM_RESPAWN_COMMON_STEP2);
			pEventInfo->dwMobCount = (DWORD)(MOBCOUNT_RESPAWN_COMMON_STEP);
			e_SpawnEvent = event_create(r_meleyspawn_event, pEventInfo, PASSES_PER_SEC(1));
		}
		else if (bStep == 3)
		{
			SetLastStoneKilledTime(0);
			SetKillCountStones(0);
			SetKillCountBosses(0);
			r_meleyspawn_info* pEventInfo = AllocEventInfo<r_meleyspawn_info>();
			pEventInfo->pMap = this;
			pEventInfo->bFirst = true;
			pEventInfo->bStep = bStep;
			pEventInfo->dwTimeReload = (DWORD)(TIME_RESPAWN_COMMON_STEP3);
			pEventInfo->dwMobVnum = (DWORD)(MOBVNUM_RESPAWN_COMMON_STEP3);
			pEventInfo->dwMobCount = (DWORD)(MOBCOUNT_RESPAWN_COMMON_STEP);
			e_SpawnEvent = event_create(r_meleyspawn_event, pEventInfo, PASSES_PER_SEC(1));
		}
	}
	
	void CMgrMap::Start()
	{
		if (!pkSectreeMap)
			EndDungeon(false, true);
		else
			pkMainNPC = Spawn((DWORD)(NPC_VNUM), 135, 179, 8), pkGate = Spawn((DWORD)(GATE_VNUM), 129, 175, 5), pkBoss = Spawn((DWORD)(BOSS_VNUM), 130, 77, 1), pkStatue1 = Spawn((DWORD)(STATUE_VNUM), 123, 137, 5, true), pkStatue2 = Spawn((DWORD)(STATUE_VNUM), 123, 124, 5, true), pkStatue3 = Spawn((DWORD)(STATUE_VNUM), 136, 123, 5, true), pkStatue4 = Spawn((DWORD)(STATUE_VNUM), 137, 137, 5, true);
	}
	
	void CMgrMap::StartDungeon(LPCHARACTER pkChar)
	{
		if (!IsPartecipant(pkChar->GetPlayerID()))
			return;
		else if (e_pEndEvent != NULL)
		{
			EndDungeon(false, true);
			return;
		}
		else if ((!pkMainNPC) || (!pkGate) || (!pkBoss) || (!pkStatue1) || (!pkStatue2) || (!pkStatue3) || (!pkStatue4))
		{
			EndDungeon(false, true);
			return;
		}
		
		CGuild* pkGuild = CGuildManager::instance().FindGuild(GetGuildID());
		if (!pkGuild)
			return;
		
		char szBuf1[512], szBuf2[512];
		#ifdef __MULTI_LANGUAGE_SYSTEM__
		snprintf(szBuf1, sizeof(szBuf1), LC_TEXT(ENGLISH, "<Guild> The fight against %s just start."), pkBoss->GetName());
		snprintf(szBuf2, sizeof(szBuf2), LC_TEXT(ENGLISH, "<Guild> The shelter's doors are now opened. The time limit is %d hour(s)."), (int)(TIME_LIMIT_DUNGEON / 3600));
		#else
		snprintf(szBuf1, sizeof(szBuf1), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> The fight against %s just start."), pkBoss->GetName());
		snprintf(szBuf2, sizeof(szBuf2), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> The shelter's doors are now opened. The time limit is %d hour(s)."), (int)(TIME_LIMIT_DUNGEON / 3600));
		#endif
		pkGuild->Chat(szBuf1);
		pkGuild->Chat(szBuf2);
		
		pkChar->SetQuestNPCID(0);
		pkGate->Dead();
		pkGate = NULL;
		
		SetDungeonStep(1);
		if (e_pEndEvent != NULL)
			event_cancel(&e_pEndEvent);
		
		r_meleylimit_info* pEventInfo = AllocEventInfo<r_meleylimit_info>();
		pEventInfo->pMap = this;
		pEventInfo->bWarp = false;
		e_pEndEvent = event_create(r_meleylimit_event, pEventInfo, PASSES_PER_SEC(TIME_LIMIT_DUNGEON));
	}
	
	void CMgrMap::EndDungeon(bool bSuccess, bool bGiveBack)
	{
		sys_log(0, "%s:%d", __FILE__, __LINE__);
		int iWarpTime = 0, iReturn = 0;
		CGuild* pkGuild = CGuildManager::instance().FindGuild(GetGuildID());
		if (pkGuild)
		{
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			char szBuf1[512], szBuf2[512];
			if (bGiveBack)
			{
				sys_log(0, "%s:%d", __FILE__, __LINE__);
				if (bSuccess)
				{
					sys_log(0, "%s:%d", __FILE__, __LINE__);
					std::string time;
					DWORD dwNecessaryTime = get_global_time() - GetDungeonTimeStart();
					sys_log(0, "%s:%d", __FILE__, __LINE__);
					LogManager::instance().MeleyLog(GetGuildID(), GetPartecipantsCount(), dwNecessaryTime);
					int iM = (dwNecessaryTime / 60) % 60, iS = dwNecessaryTime % 60;
					if (iM > 0)
						time.append(boost::lexical_cast<std::string>(iM) + LC_TEXT(LANGUAGE_ENGLISH, " minute(s)"));
					sys_log(0, "%s:%d", __FILE__, __LINE__);
					if (iS > 0)
					{
						sys_log(0, "%s:%d", __FILE__, __LINE__);
						if (iM > 0)
							time.append(", ");
						
						time.append(boost::lexical_cast<std::string>(iS) + LC_TEXT(LANGUAGE_ENGLISH, " second(s)"));
					}
					sys_log(0, "%s:%d", __FILE__, __LINE__);
					
					DWORD dwCoolDownTime = get_global_time() + COOLDOWN_DUNGEON + 5;
					//pkGuild->SetDungeonCooldown(dwCoolDownTime);
					
					iWarpTime = 60*3;
					iReturn = (int)(LADDER_POINTS_COST);
					snprintf(szBuf1, sizeof(szBuf1), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> Compliments, you defeat Meley."));
					snprintf(szBuf2, sizeof(szBuf2), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> Necessary time: %s."), time.c_str());

				}
				else
				{
					sys_log(0, "%s:%d", __FILE__, __LINE__);
					iWarpTime = 10;
					iReturn = (int)(LADDER_POINTS_COST);
					snprintf(szBuf1, sizeof(szBuf1), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> The dungeon can't be started, please contact our staff."));
					snprintf(szBuf2, sizeof(szBuf2), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> %d ladder points will be returned."), iReturn);
				}
			}
			else
			{
				DWORD dwCoolDownTime = get_global_time() + COOLDOWN_DUNGEON + 5;
				//pkGuild->SetDungeonCooldown(dwCoolDownTime);
				sys_log(0, "%s:%d", __FILE__, __LINE__);
				iWarpTime = 10;
				iReturn = (int)(LADDER_POINTS_RETURN);
				snprintf(szBuf1, sizeof(szBuf1), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> The time limit to fight %s has ended."), pkBoss->GetName());
				snprintf(szBuf2, sizeof(szBuf2), ".");
			}
			
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			pkGuild->Chat(szBuf1);
			if (strlen(szBuf2) > 1)
				pkGuild->Chat(szBuf2);
			
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			if (pkSectreeMap)
			{
				char szBuf[512];
				if (iWarpTime == 60)
					snprintf(szBuf, sizeof(szBuf), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> For your safety, you will be teleported out of the refuge in %d second(s)."), iWarpTime);
				else
					snprintf(szBuf, sizeof(szBuf), LC_TEXT(LANGUAGE_ENGLISH, "<Guild> All the partecipants will be teleported out in %d seconds."), iWarpTime);
				
				pkGuild->Chat(szBuf);
			}
			
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			if (iReturn)
			{
				sys_log(0, "%s:%d", __FILE__, __LINE__);
				DWORD dwCost = iReturn;
				pkGuild->ChangeLadderPoint(+dwCost);
				pkGuild->SetLadderPoint(pkGuild->GetLadderPoint() + dwCost);
			}
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			
			pkGuild->RequestDungeon(0, 0);
			sys_log(0, "%s:%d", __FILE__, __LINE__);
		}
		
		sys_log(0, "%s:%d", __FILE__, __LINE__);
		if (!iWarpTime)
			EndDungeonWarp();
		else
		{
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			if (e_pEndEvent != NULL)
				event_cancel(&e_pEndEvent);
			
			e_pEndEvent = NULL;
			
			sys_log(0, "%s:%d", __FILE__, __LINE__);
			if (e_pWarpEvent != NULL)
				event_cancel(&e_pWarpEvent);
			
			r_meleylimit_info* pEventInfo = AllocEventInfo<r_meleylimit_info>();
			pEventInfo->pMap = this;
			pEventInfo->bWarp = true;
			e_pWarpEvent = event_create(r_meleylimit_event, pEventInfo, PASSES_PER_SEC(iWarpTime));
		}
		sys_log(0, "%s:%d", __FILE__, __LINE__);
	}
	
	void CMgrMap::EndDungeonWarp()
	{
		if (pkSectreeMap)
		{
			FExitAndGoTo f;
			pkSectreeMap->for_each(f);
		}
		
		CMgr::instance().Remove(GetGuildID());
		SECTREE_MANAGER::instance().DestroyPrivateMap(GetMapIndex());
		Destroy();
		M2_DELETE_ONLY(this);
	}
	
	bool CMgrMap::Damage(LPCHARACTER pkStatue)
	{
		BYTE bStep = GetDungeonStep();
		if ((!bStep) || ((pkStatue != pkStatue1) && (pkStatue != pkStatue2) && (pkStatue != pkStatue3) && (pkStatue != pkStatue4)))
			return false;
		else if ((pkStatue->IsAffectFlag(AFF_STATUE1)) || pkStatue->IsAffectFlag(AFF_STATUE2) || pkStatue->IsAffectFlag(AFF_STATUE3) || pkStatue->IsAffectFlag(AFF_STATUE4))
			return false;
		
		int iHPCalc = 0;
		if (bStep == 1)
			iHPCalc = pkStatue->GetMaxHP() * 75 / 100;
		else if (bStep == 2)
			iHPCalc = pkStatue->GetMaxHP() * 50 / 100;
		else if (bStep == 3)
			iHPCalc = pkStatue->GetMaxHP() * 1 / 100;
		
		if (pkStatue->GetHP() <= iHPCalc)
		{
			sys_log(0, "statueMaxHP %u step %u curHP %d hpCalc %d", pkStatue->GetMaxHP(), bStep, pkStatue->GetHP(), iHPCalc);
			int iCalc = iHPCalc - pkStatue->GetHP();
			pkStatue->PointChange(POINT_HP, iCalc);
			if (bStep == 1)
			{
				if (!pkStatue->IsAffectFlag(AFF_STATUE1))
					pkStatue->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE1, 3600, 0, true);
				
				bool bNextStep = ((pkStatue1->IsAffectFlag(AFF_STATUE1)) && (pkStatue2->IsAffectFlag(AFF_STATUE1)) && (pkStatue3->IsAffectFlag(AFF_STATUE1)) && (pkStatue4->IsAffectFlag(AFF_STATUE1))) ? true : false;
				if (bNextStep)
					SetDungeonStep(2);
				
				return false;
			}
			else if (bStep == 2)
			{
				if (!pkStatue->IsAffectFlag(AFF_STATUE2))
					pkStatue->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE2, 3600, 0, true);
				
				bool bNextStep = ((pkStatue1->IsAffectFlag(AFF_STATUE2)) && (pkStatue2->IsAffectFlag(AFF_STATUE2)) && (pkStatue3->IsAffectFlag(AFF_STATUE2)) && (pkStatue4->IsAffectFlag(AFF_STATUE2))) ? true : false;
				if (bNextStep)
					SetDungeonStep(3);
				
				return false;
			}
			else if (bStep == 3)
			{
				if (!pkStatue->IsAffectFlag(AFF_STATUE3))
					pkStatue->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE3, 3600, 0, true);
				
				bool bNextStep = ((pkStatue1->IsAffectFlag(AFF_STATUE3)) && (pkStatue2->IsAffectFlag(AFF_STATUE3)) && (pkStatue3->IsAffectFlag(AFF_STATUE3)) && (pkStatue4->IsAffectFlag(AFF_STATUE3))) ? true : false;
				if (bNextStep)
					SetDungeonStep(4);
				
				return false;
			}
		}
		if (bStep == 4)
		{
			if (!pkStatue->IsAffectFlag(AFF_STATUE4))
				pkStatue->AddAffect(AFFECT_STATUE, POINT_NONE, 0, AFF_STATUE4, 3600, 0, true);
			return false;
		}
		
		return true;
	}
	
	void CMgrMap::OnKill(DWORD dwVnum)
	{
		BYTE bStep = GetDungeonStep();
		if (!bStep)
			return;

		if (((bStep == 2) || (bStep == 3)) && (dwVnum == (DWORD)(MOBVNUM_RESPAWN_STONE_STEP2)))
		{
			DWORD dwLimit = (DWORD)(MOBCOUNT_RESPAWN_STONE_STEP2) - 1;
			if (GetKillCountStones() >= dwLimit)
			{
				if (bStep == 2)
					SetKillCountStones(0);
				else
					SetKillCountStones(GetKillCountStones() + 1);
				
				DWORD dwRandomMin = random_number(2, 4);
				DWORD dwLastKilledTime = get_global_time() + (60 * dwRandomMin);
				SetLastStoneKilledTime(dwLastKilledTime);
				
				bool bDoAff = true;
				if (bStep == 3)
				{
					DWORD dwLimit2 = (DWORD)(MOBCOUNT_RESPAWN_BOSS_STEP3);
					if (GetKillCountBosses() >= dwLimit2)
						SetKillCountStones(0);
					else
						bDoAff = false;
				}
				
				if (bDoAff)
				{
					if (pkStatue1 && ((pkStatue1->IsAffectFlag(AFF_STATUE1)) || ((bStep == 3) && (pkStatue1->IsAffectFlag(AFF_STATUE2)))))
						pkStatue1->RemoveAffect(AFFECT_STATUE);
					
					if (pkStatue2 && ((pkStatue2->IsAffectFlag(AFF_STATUE1)) || ((bStep == 3) && (pkStatue2->IsAffectFlag(AFF_STATUE2)))))
						pkStatue2->RemoveAffect(AFFECT_STATUE);
					
					if (pkStatue3 && ((pkStatue3->IsAffectFlag(AFF_STATUE1)) || ((bStep == 3) && (pkStatue3->IsAffectFlag(AFF_STATUE2)))))
						pkStatue3->RemoveAffect(AFFECT_STATUE);
					
					if (pkStatue4 && ((pkStatue4->IsAffectFlag(AFF_STATUE1)) || ((bStep == 3) && (pkStatue4->IsAffectFlag(AFF_STATUE2)))))
						pkStatue4->RemoveAffect(AFFECT_STATUE);
				}
			}
			else
				SetKillCountStones(GetKillCountStones() + 1);
		}
		else if ((bStep == 3) && (dwVnum == (DWORD)(MOBVNUM_RESPAWN_BOSS_STEP3)))
		{
			DWORD dwLimit = (DWORD)(MOBCOUNT_RESPAWN_BOSS_STEP3) - 1;
			if (GetKillCountBosses() >= dwLimit)
			{
				SetKillCountBosses(GetKillCountBosses() + 1);
				
				bool bDoAff = true;
				DWORD dwLimit2 = (DWORD)(MOBCOUNT_RESPAWN_STONE_STEP2);
				if (GetKillCountStones() >= dwLimit2)
					SetKillCountStones(0);
				else
					bDoAff = false;
				
				if (bDoAff)
				{
					if (pkStatue1 && ((pkStatue1->IsAffectFlag(AFF_STATUE1)) || (pkStatue1->IsAffectFlag(AFF_STATUE2))))
						pkStatue1->RemoveAffect(AFFECT_STATUE);
					
					if (pkStatue2 && ((pkStatue2->IsAffectFlag(AFF_STATUE1)) || (pkStatue2->IsAffectFlag(AFF_STATUE2))))
						pkStatue2->RemoveAffect(AFFECT_STATUE);
					
					if (pkStatue3 && ((pkStatue3->IsAffectFlag(AFF_STATUE1)) || (pkStatue3->IsAffectFlag(AFF_STATUE2))))
						pkStatue3->RemoveAffect(AFFECT_STATUE);
					
					if (pkStatue4 && ((pkStatue4->IsAffectFlag(AFF_STATUE1)) || (pkStatue4->IsAffectFlag(AFF_STATUE2))))
						pkStatue4->RemoveAffect(AFFECT_STATUE);
				}
			}
			else
				SetKillCountBosses(GetKillCountBosses() + 1);
		}
	}
	
	void CMgrMap::OnKillStatue(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkStatue)
	{
		BYTE bStep = GetDungeonStep();
		if ((!pkItem) || (!pkChar) || (!pkStatue) || (bStep != 4) || (!pkBoss))
			return;
		
		if (!pkStatue->IsAffectFlag(AFF_STATUE4) || pkStatue->IsDead())
			return;
		
		DWORD dwPlayerID = pkChar->GetPlayerID();
		bool bCheck = std::find(v_Already.begin(), v_Already.end(), dwPlayerID) != v_Already.end();
		#ifdef __DESTROY_INFINITE_STATUES_GM__
		if ((bCheck) && (pkChar->GetGMLevel() <= GM_PLAYER))
		#else
		if (bCheck)
		#endif
			return;
		else
		{
			#ifdef __DESTROY_INFINITE_STATUES_GM__
			if (!bCheck)
				v_Already.push_back(dwPlayerID);
			#else
			v_Already.push_back(dwPlayerID);
			#endif
		}
		
		ITEM_MANAGER::instance().RemoveItem(pkItem);
		pkStatue->RemoveAffect(AFFECT_STATUE);
		pkStatue->Dead();
		if (pkStatue1 == pkStatue)
			pkStatue1 = NULL;
		else if (pkStatue2 == pkStatue)
			pkStatue2 = NULL;
		else if (pkStatue3 == pkStatue)
			pkStatue3 = NULL;
		else
			pkStatue4 = NULL;

		bool bNextStepPass = !pkStatue1 && !pkStatue2 && !pkStatue3 && !pkStatue4;
		if (bNextStepPass)
			DungeonResult();
	}

	struct FKillAllMonsterFunc
	{
		void operator()(LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return;

			LPCHARACTER tch = (LPCHARACTER) ent;
			if (tch->IsNPC() && tch->IsEnemy())
				tch->Dead();
		}
	};
	
	void CMgrMap::DungeonResult()
	{
		if (e_DestroyStatues != NULL)
			event_cancel(&e_DestroyStatues);
		
		e_DestroyStatues = NULL;
		
		bool bNextStep = !pkStatue1 && !pkStatue2 && !pkStatue3 && !pkStatue4;
		if (!bNextStep)
			EndDungeon(false, false);
		else
		{
			v_Rewards.clear();
			SetRewardTime(get_global_time() + 10);
			pkBoss->Dead();
			sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
			Spawn(CHEST_VNUM, 130, 130, 1);
			sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
			
			EndDungeon(true, true);
			sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);

			LPSECTREE_MAP pkMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());
			if (pkMap)
			{
				sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
				FKillAllMonsterFunc f;
				pkMap->for_each(f);
			}
			sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
		}
	}
	
	bool CMgrMap::CheckRewarder(DWORD dwPlayerID)
	{
		sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
		bool bCheck = std::find(v_Rewards.begin(), v_Rewards.end(), dwPlayerID) != v_Rewards.end();
		return bCheck;
	}
	
	void CMgrMap::GiveReward(LPCHARACTER pkChar, BYTE bReward)
	{
		sys_log(0,"%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
		DWORD dwPlayerID = pkChar->GetPlayerID();
		if (!IsPartecipant(dwPlayerID))
			return;
		else if (CheckRewarder(dwPlayerID))
			return;
		
		v_Rewards.push_back(dwPlayerID);
		DWORD dwVnumReward = bReward == 1 ? REWARD_ITEMCHEST_VNUM_1 : REWARD_ITEMCHEST_VNUM_2;
		pkChar->AutoGiveItem(dwVnumReward, 1);
	}
	
	void CMgr::Initialize()
	{
		m_RegGuilds.clear();
		SetXYZ(0, 0, 0);
		SetSubXYZ(0, 0, 0);
	}
	
	void CMgr::Destroy()
	{
		itertype(m_RegGuilds) iter = m_RegGuilds.begin();
		for (; iter != m_RegGuilds.end(); ++iter)
		{
			CMgrMap* pMap = iter->second;
			SECTREE_MANAGER::instance().DestroyPrivateMap(pMap->GetMapIndex());
			pMap->Destroy();
			M2_DELETE(pMap);
		}
		
		m_RegGuilds.clear();
		SetXYZ(0, 0, 0);
		SetSubXYZ(0, 0, 0);
	}
	
	bool CMgr::isRegistered(CGuild* pkGuild, int & iCH)
	{
		iCH = 0;
		if (pkGuild)
		{
			long lMinIndex = MAP_INDEX * 10000, lMaxIndex = MAP_INDEX * 10000 + 10000;
			if ((pkGuild->GetDungeonMapIndex() >= lMinIndex) && (pkGuild->GetDungeonMapIndex() <= lMaxIndex))
			{
				iCH = pkGuild->GetDungeonCH();
				return true;
			}
		}
		
		return false;
	}
	
	void CMgr::Register(LPCHARACTER pkChar, int & iRes1, int & iRes2)
	{
		iRes1 = 0;
		iRes2 = 0;
		if (!pkChar)
			return;
		
		CGuild* pkGuild = pkChar->GetGuild();
		if (!pkGuild)
			return;
		else if (pkGuild->GetMasterPID() != pkChar->GetPlayerID())
		{
			TGuildMember* pGuildMember = pkGuild->GetMember(pkChar->GetPlayerID());
			if (!pGuildMember || !pkGuild->HasGradeAuth(pGuildMember->grade, GUILD_AUTH_MELEY_ALLOW))
				return;
		}
		
		int iCH;
		if (isRegistered(pkGuild, iCH))
		{
			iRes1 = 1;
			iRes2 = iCH;
			return;
		}
		else if (pkGuild->GetDungeonCH())
		{
			iRes1 = 2;
			iRes2 = 0;
			return;
		}
		else if (pkGuild->GetLevel() < MIN_LVL)
		{
			iRes1 = 3;
			iRes2 = (int)(MIN_LVL);
			return;
		}
		else if (pkGuild->GetLadderPoint() < LADDER_POINTS_COST)
		{
			iRes1 = 4;
			iRes2 = (int)(LADDER_POINTS_COST);
			return;
		}
		
		DWORD dwTimeNow = get_global_time();
		/*if ((pkGuild->GetDungeonCooldown() > dwTimeNow) && (!pkGuild->GetDungeonCH()))
		{
			int iDif = pkGuild->GetDungeonCooldown() - dwTimeNow;
			iRes1 = 7;
			iRes2 = iDif;
			return;
		}*/
		if (pkChar->GetQuestFlag("meleylair_zone.next_entry") > dwTimeNow) {
			int iDif = pkChar->GetQuestFlag("meleylair_zone.next_entry") - dwTimeNow;
			iRes1 = 7;
			iRes2 = iDif;
			return;
		}
		
		long lNormalMapIndex = (long)(MAP_INDEX);
		long lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(lNormalMapIndex);
		if (!lMapIndex)
		{
			iRes1 = 5;
			return;
		}
		
		DWORD dwCost = (DWORD)(LADDER_POINTS_COST);
		pkGuild->ChangeLadderPoint(-dwCost);
		pkGuild->SetLadderPoint(pkGuild->GetLadderPoint() - dwCost);
		
		PIXEL_POSITION pos = GetXYZ(), posSub = GetSubXYZ();
		if (!pos.x)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(MeleyLair::MAP_INDEX));
			if (pkSectreeMap)
				SetXYZ(pkSectreeMap->m_setting.iBaseX + 130 * 100, pkSectreeMap->m_setting.iBaseY + 130 * 100, 0);
		}
		
		if (!posSub.x)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(MeleyLair::SUBMAP_INDEX));
			if (pkSectreeMap)
				SetSubXYZ(pkSectreeMap->m_setting.iBaseX + 87 * 100, pkSectreeMap->m_setting.iBaseY + 853 * 100, 0);
		}
		
		pkGuild->RequestDungeon(g_bChannel, lMapIndex);
		//pkGuild->SetDungeonCooldown(0);
		m_RegGuilds.insert(std::make_pair(pkGuild->GetID(), M2_NEW CMgrMap(lMapIndex, pkGuild->GetID())));
		
		iRes1 = 6;
		iRes2 = (int)(LADDER_POINTS_RETURN);
	}
	
	bool CMgr::Enter(CGuild* pkGuild, LPCHARACTER pkChar, int & iLimit)
	{
		iLimit = 0;
		if ((pkGuild) && (pkChar))
		{
			DWORD dwGuildID = pkGuild->GetID();
			if (m_RegGuilds.find(dwGuildID) == m_RegGuilds.end())
			{
				BYTE bCH = pkGuild->GetDungeonCH();
				if ((bCH) && (g_bChannel != bCH))
				{
					iLimit = pkGuild->GetDungeonCH();
					return false;
				}
				else if (g_bChannel != pkGuild->GetDungeonCH())
					return false;
				
				long lNormalMapIndex = (long)(MAP_INDEX);
				long lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(lNormalMapIndex);
				if (!lMapIndex)
				{
					DWORD dwCost = (DWORD)(LADDER_POINTS_COST);
					pkGuild->ChangeLadderPoint(+dwCost);
					pkGuild->SetLadderPoint(pkGuild->GetLadderPoint() + dwCost);
					return false;
				}
				
				PIXEL_POSITION pos = GetXYZ(), posSub = GetSubXYZ();
				if (!pos.x)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(MeleyLair::MAP_INDEX));
					if (pkSectreeMap)
						SetXYZ(pkSectreeMap->m_setting.iBaseX + 130 * 100, pkSectreeMap->m_setting.iBaseY + 130 * 100, 0);
				}
				
				if (!posSub.x)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap((long)(MeleyLair::SUBMAP_INDEX));
					if (pkSectreeMap)
						SetSubXYZ(pkSectreeMap->m_setting.iBaseX + 87 * 100, pkSectreeMap->m_setting.iBaseY + 853 * 100, 0);
				}
				
				pkGuild->RequestDungeon(g_bChannel, lMapIndex);
				m_RegGuilds.insert(std::make_pair(dwGuildID, M2_NEW CMgrMap(lMapIndex, dwGuildID)));
			}
			
			CMgrMap* pMap = m_RegGuilds.find(dwGuildID)->second;
			if (!pMap)
				return false;
			else if (pMap->GetPartecipantsCount() >= PARTECIPANTS_LIMIT)
			{
				iLimit = 1;
				return true;
			}
			else if (pMap->GetDungeonStep() >= 4)
			{
				iLimit = 3;
				return true;
			}
			// else if (pkChar->GetPKMode() != PK_MODE_GUILD)
			// {
				// iLimit = 4;
				// return true;
			// }
			
			PIXEL_POSITION mPos;
			if (!SECTREE_MANAGER::instance().GetRecallPositionByEmpire((int)(MAP_INDEX), 0, mPos))
			{
				iLimit = 2;
				return true;
			}
			
			pMap->Partecipant(true, pkChar->GetPlayerID());
			pkChar->SaveExitLocation();
			pkChar->WarpSet(mPos.x, mPos.y, pMap->GetMapIndex());
			return true;
		}
		
		return false;
	}
	
	void CMgr::Leave(CGuild* pkGuild, LPCHARACTER pkChar, bool bSendOut)
	{
		if (!pkChar)
			return;
		
		if (pkGuild)
		{
			DWORD dwGuildID = pkGuild->GetID();
			if (m_RegGuilds.find(dwGuildID) != m_RegGuilds.end())
			{
				CMgrMap* pMap = m_RegGuilds.find(dwGuildID)->second;
				if (pMap)
				{
					if (!bSendOut)
					{
						if (pkChar->GetMapIndex() == pMap->GetMapIndex())
							pMap->Partecipant(false, pkChar->GetPlayerID());
					}
					else if (!pMap->IsPartecipant(pkChar->GetPlayerID()))
						WarpOut(pkChar);
				}
				else
					WarpOut(pkChar);
			}
			else
				WarpOut(pkChar);
		}
		else if (bSendOut)
			WarpOut(pkChar);
	}
	
	void CMgr::LeaveRequest(CGuild* pkGuild, LPCHARACTER pkChar)
	{
		if (!pkChar)
			return;
		
		if (pkGuild)
		{
			DWORD dwGuildID = pkGuild->GetID();
			if (m_RegGuilds.find(dwGuildID) != m_RegGuilds.end())
			{
				CMgrMap* pMap = m_RegGuilds.find(dwGuildID)->second;
				if (pMap)
				{
					if (pMap->IsPartecipant(pkChar->GetPlayerID()))
						pMap->Partecipant(false, pkChar->GetPlayerID());
					
					WarpOut(pkChar);
				}
				else
					WarpOut(pkChar);
			}
			else
				WarpOut(pkChar);
		}
		else
			WarpOut(pkChar);
	}
	
	bool CMgr::IsMeleyMap(long lMapIndex)
	{
		long lMinIndex = (long)(MAP_INDEX) * 10000, lMaxIndex = (long)(MAP_INDEX) * 10000 + 10000;
		if (((lMapIndex >= lMinIndex) && (lMapIndex < lMaxIndex)) || (lMapIndex == (long)(MAP_INDEX)))
			return true;
		
		return false;
	}
	
	void CMgr::Check(CGuild* pkGuild, LPCHARACTER pkChar)
	{
		if (!pkChar)
			return;
		
		if (pkGuild)
		{
			DWORD dwGuildID = pkGuild->GetID();
			if (m_RegGuilds.find(dwGuildID) != m_RegGuilds.end())
			{
				CMgrMap* pMap = m_RegGuilds.find(dwGuildID)->second;
				if (pMap)
				{
					if (!pMap->IsPartecipant(pkChar->GetPlayerID()))
						WarpOut(pkChar);
				}
				else
					WarpOut(pkChar);
			}
			else
				WarpOut(pkChar);
		}
		else
			WarpOut(pkChar);
	}
	
	void CMgr::WarpOut(LPCHARACTER pkChar)
	{
		if (!pkChar)
			return;
		
		PIXEL_POSITION posSub = GetSubXYZ();
		if (!posSub.x)
			pkChar->WarpSet(EMPIRE_START_X(pkChar->GetEmpire()), EMPIRE_START_Y(pkChar->GetEmpire()));
		else
			pkChar->WarpSet(posSub.x, posSub.y);
	}
	
	void CMgr::Start(LPCHARACTER pkChar, CGuild* pkGuild)
	{
		if ((!pkChar) || (!pkGuild))
			return;
		
		if (m_RegGuilds.find(pkGuild->GetID()) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return;
		}

		CMgrMap* pMap = m_RegGuilds.find(pkGuild->GetID())->second;
		if (pMap)
			pMap->StartDungeon(pkChar);
	}
	
	bool CMgr::Damage(LPCHARACTER pkStatue, CGuild* pkGuild)
	{
		if ((!pkStatue) || (!pkGuild) || (!pkGuild->GetID()))
			return false;
		
		if (m_RegGuilds.find(pkGuild->GetID()) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return false;
		}
		
		CMgrMap* pMap = m_RegGuilds.find(pkGuild->GetID())->second;
		if (!pMap)
			return false;
		
		return pMap->Damage(pkStatue);
	}
	
	void CMgr::Remove(DWORD dwGuildID)
	{
		itertype(m_RegGuilds) iter = m_RegGuilds.find(dwGuildID), iterEnd = m_RegGuilds.end();
		if (iter != iterEnd)
			m_RegGuilds.erase(iter);
	}
	
	void CMgr::OnKill(DWORD dwVnum, CGuild* pkGuild)
	{
		if ((!dwVnum) || (!pkGuild))
			return;
		
		if (m_RegGuilds.find(pkGuild->GetID()) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return;
		}

		CMgrMap* pMap = m_RegGuilds.find(pkGuild->GetID())->second;
		if (!pMap)
			return;
		
		pMap->OnKill(dwVnum);
	}
	
	void CMgr::OnKillStatue(LPITEM pkItem, LPCHARACTER pkChar, LPCHARACTER pkStatue, CGuild* pkGuild)
	{
		if ((!pkItem) || (!pkChar) || (!pkStatue) || (!pkGuild))
			return;
		
		if (pkItem->GetOriginalVnum() != (DWORD)(SEAL_VNUM_KILL_STATUE))
			return;
		
		if (m_RegGuilds.find(pkGuild->GetID()) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return;
		}

		CMgrMap* pMap = m_RegGuilds.find(pkGuild->GetID())->second;
		if (!pMap)
			return;
		else if (!pMap->IsPartecipant(pkChar->GetPlayerID()))
			return;
		
		pMap->OnKillStatue(pkItem, pkChar, pkStatue);
	}
	
	void CMgr::OnKillCommon(LPCHARACTER pkMonster, LPCHARACTER pkChar, DWORD guild_id)
	{
		if (!pkMonster || !pkChar || !guild_id)
			return;
		
		if (m_RegGuilds.find(guild_id) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return;
		}

		CMgrMap* pMap = m_RegGuilds.find(guild_id)->second;
		if (!pMap)
			return;
		else if (!pkChar || !pkChar->IsPC() || !pkChar->GetPlayerID() || !pMap->IsPartecipant(pkChar->GetPlayerID()))
			return;
		else if (pMap->GetDungeonStep() < 3)
			return;
		
		int iChance = random_number(1, 100);
		if (iChance > 75)
		{
			LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(SEAL_VNUM_KILL_STATUE);
			if (!pkItem)
				return;
			
			PIXEL_POSITION mPos;
			mPos.x = pkMonster->GetX();
			mPos.y = pkMonster->GetY();
			
			pkItem->AddToGround(pMap->GetMapIndex(), mPos);
			pkItem->StartDestroyEvent();
			pkItem->SetOwnership(pkChar, 60);
		}
	}
	
	bool CMgr::CanGetReward(LPCHARACTER pkChar, CGuild* pkGuild)
	{
		if ((!pkChar) || (!pkGuild))
			return false;
		
		if (m_RegGuilds.find(pkGuild->GetID()) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return false;
		}

		CMgrMap* pMap = m_RegGuilds.find(pkGuild->GetID())->second;
		if (!pMap)
			return false;

		else if (!pMap->IsPartecipant(pkChar->GetPlayerID()))
			return false;

		else if (pMap->GetDungeonStep() != 4)
			return false;
		
		/* DWORD dwTime = get_global_time();
		if (pMap->GetRewardTime() > dwTime)
		{
			sys_err("%s:%d %s", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			return false;
		}
		else  */
		if (pMap->CheckRewarder(pkChar->GetPlayerID()))
			return false;
		
		return true;
	}
	
	void CMgr::Reward(LPCHARACTER pkChar, CGuild* pkGuild, BYTE bReward)
	{
		if ((!pkChar) || (!pkGuild))
			return;
		
		if (m_RegGuilds.find(pkGuild->GetID()) == m_RegGuilds.end())
		{
			sys_err("Meley liar would CRASH!?! %s:%d", __FILE__, __LINE__);
			return;
		}
		
		CMgrMap* pMap = m_RegGuilds.find(pkGuild->GetID())->second;
		if (!pMap)
			return;
		
		pMap->GiveReward(pkChar, bReward);
	}
	
	void CMgr::MemberRemoved(LPCHARACTER pkChar, CGuild* pkGuild)
	{
		if ((!pkChar) || (!pkGuild))
			return;
		
		if (!MeleyLair::CMgr::instance().IsMeleyMap(pkChar->GetMapIndex()))
			return;
		
		DWORD dwGuildID = pkGuild->GetID();
		if (m_RegGuilds.find(dwGuildID) != m_RegGuilds.end())
		{
			CMgrMap* pMap = m_RegGuilds.find(dwGuildID)->second;
			if (pMap)
			{
				if (pMap->IsPartecipant(pkChar->GetPlayerID()))
					pMap->Partecipant(false, pkChar->GetPlayerID());
				
				WarpOut(pkChar);
			}
		}
	}
	
	void CMgr::GuildRemoved(CGuild* pkGuild)
	{
		if (!pkGuild)
			return;
		
		DWORD dwGuildID = pkGuild->GetID();
		if (m_RegGuilds.find(dwGuildID) != m_RegGuilds.end())
		{
			CMgrMap* pMap = m_RegGuilds.find(dwGuildID)->second;
			if (pMap)
				pMap->EndDungeonWarp();
		}
	}
};
#endif
