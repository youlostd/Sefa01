/*********************************************************************
* title_name		: Combat Zone (Official Webzen 16.4)
* date_created		: 2017.05.21
* filename			: combat_zone.cpp
* author			: VegaS
* version_actual	: Version 0.2.0
*/

#include "stdafx.h"
#ifdef COMBAT_ZONE
#include "desc.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "item.h"
#include "char_manager.h"
#include "affect.h"
#include "start_position.h"
#include "p2p.h"
#include "db.h"
#include "skill.h"
#include "dungeon.h"
#include <string>
#include <boost/algorithm/string/replace.hpp>
#include "desc_manager.h"
#include "buffer_manager.h"
#include "dev_log.h"
#include <fstream>
#include <algorithm>
#include <iostream>
#include "constants.h"
#include "questmanager.h"
#include "desc_client.h"
#include "sectree_manager.h"
#include "regen.h"
#include <boost/format.hpp>
#include "item_manager.h"
#include "combat_zone.h"
#include "target.h"
#include "party.h"
#include "mount_system.h"
#include "cmd.h"
#include "log.h"
#define ON_SUCCES_RESTART(ch) ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow"); ch->GetDesc()->SetPhase(PHASE_GAME); ch->SetPosition(POS_STANDING); ch->StartRecoveryEvent();

std::map<std::string, DWORD> m_kMapCombatZoneTargetSign;

DWORD m_pCombatZoneDateTime[DAY_MAX_NUM][8] = {
	{
        0, 0,
        0, 1
    },
	{
        0, 0,
        0, 1
    },
	{
        0, 0,
        0, 1
    },
	{
        0, 0,
        0, 1
    },
	{
        0, 0,
        0, 1
    },
	{
        0, 0,
        0, 1
    },
	{
        0, 0,
        0, 1
    }
    // { 00, 00, 00, 00},
    // { 19, 00, 20, 00},
    // { 00, 00, 00, 00},
    // { 19, 00, 20, 00},
    // { 00, 00, 00, 00},
    // { 20, 00, 21, 00},
    // { 20, 00, 21, 00},
};

#define COMBAT_ZONE_BASE_POS_X 409600
#define COMBAT_ZONE_BASE_POS_Y 588800

SCombatZoneRespawnData objectPos[COMBAT_ZONE_MAX_POS_TELEPORT] =
{
	{ COMBAT_ZONE_BASE_POS_X + 189 * 100, COMBAT_ZONE_BASE_POS_Y + 266 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 331 * 100, COMBAT_ZONE_BASE_POS_Y + 304 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 291 * 100, COMBAT_ZONE_BASE_POS_Y + 215 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 331 * 100, COMBAT_ZONE_BASE_POS_Y + 314 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 281 * 100, COMBAT_ZONE_BASE_POS_Y + 319 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 225 * 100, COMBAT_ZONE_BASE_POS_Y + 200 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 200 * 100, COMBAT_ZONE_BASE_POS_Y + 200 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 200 * 100, COMBAT_ZONE_BASE_POS_Y + 190 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 170 * 100, COMBAT_ZONE_BASE_POS_Y + 277 * 100 },
	{ COMBAT_ZONE_BASE_POS_X + 223 * 100, COMBAT_ZONE_BASE_POS_Y + 296 * 100 }
};

#undef COMBAT_ZONE_BASE_POS_X
#undef COMBAT_ZONE_BASE_POS_Y

EVENTINFO(TCombatZoneWarpEventInfo)
{
	DynamicCharacterPtr ch;
	DWORD bType, bSeconds;
	TCombatZoneWarpEventInfo() : ch(), bType(0), bSeconds(0) {}
};

EVENTINFO(TCombatZoneLeaveEventInfo)
{
	DynamicCharacterPtr ch;
	DWORD bSeconds;
	TCombatZoneLeaveEventInfo() : ch(), bSeconds(0){}
};

EVENTINFO(TCombatZoneEventInfo)
{
	CCombatZoneManager *pInstanceManager;
	TCombatZoneEventInfo()
		: pInstanceManager(0)
	{
	}
};

EVENTFUNC(combat_zone_warp_event)
{
	TCombatZoneWarpEventInfo* info = dynamic_cast<TCombatZoneWarpEventInfo*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER	ch = info->ch;
	if (!ch || !ch->GetDesc())
		return 0;
	
	if (info->bSeconds > 0) 
	{
		switch (info->bType)
		{
			case COMBAT_ZONE_ACTION_LEAVE:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%d second(s) left to leave the Combat Zone."), info->bSeconds);
				break;
				
			case COMBAT_ZONE_ACTION_PARTICIPATE:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%d second(s) left to join on the Combat Zone."), info->bSeconds);
				break;
		}

		--info->bSeconds;
		return PASSES_PER_SEC(1);
	}
	
	switch (info->bType)
	{
		case COMBAT_ZONE_ACTION_LEAVE:
			ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			break;
				
		case COMBAT_ZONE_ACTION_PARTICIPATE:
		{
			// IMPORTANT: implement party->IsInDungeon check here too
			if (ch->GetParty())
			{
				LPPARTY pParty = ch->GetParty();

				if (pParty->GetMemberCount() == 2)
					CPartyManager::instance().DeleteParty(pParty);
				else
					pParty->Quit(ch->GetPlayerID());
			}
			
			DWORD dwIndex = random_number(0, COMBAT_ZONE_MAX_POS_TELEPORT - 1);
			ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
			ch->WarpSet(objectPos[dwIndex].x, objectPos[dwIndex].y);
		}
		break;
	}

	ch->m_pkCombatZoneWarpEvent = NULL;
	return 0;
}

void WarpSetByTime(LPCHARACTER ch, DWORD bType, DWORD bSeconds)
{
	if (ch->m_pkCombatZoneWarpEvent) // abort event
	{
		event_cancel(&ch->m_pkCombatZoneWarpEvent);
		return;
	}

	TCombatZoneWarpEventInfo* info = AllocEventInfo<TCombatZoneWarpEventInfo>();
	info->ch = ch;
	info->bType = bType;
	info->bSeconds = bSeconds;
	ch->m_pkCombatZoneWarpEvent = event_create(combat_zone_warp_event, info, 1);
}

EVENTFUNC(combat_zone_leave_event)
{
	TCombatZoneLeaveEventInfo* info = dynamic_cast<TCombatZoneLeaveEventInfo*>(event->info);
	if (!info)
		return 0;

	LPCHARACTER	ch = info->ch;
	if (!ch || !ch->GetDesc())
		return 0;
	
	// If the player who announced his withdrawal and had the target attached above his head was killed, he will not be allowed to leave the war zone successfully and the withdrawal will be void.
	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your withdrawal was canceled because you got killed."));
		m_kMapCombatZoneTargetSign.erase(ch->GetName());
		ch->m_pkCombatZoneLeaveEvent = NULL;		
		return 0;
	}
	
	if (info->bSeconds > 0) 
	{
		if (info->bSeconds <= COMBAT_ZONE_LEAVE_WITH_TARGET_COUNTDOWN_WARP_SECONDS)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%d second(s) left to leave the Combat Zone."), info->bSeconds);

		--info->bSeconds;
		return PASSES_PER_SEC(1);
	}

	ch->UpdateCombatZoneRankings(ch->GetName(), ch->GetEmpire(), ch->GetCombatZonePoints()); // Update ranking with the points what he was collected this time.
	ch->SetCombatZonePoints(ch->GetRealCombatZonePoints() + ch->GetCombatZonePoints());
	if (ch->GetCombatZonePoints() > 0)
		ch->AutoGiveItem(ITEM_COMBAT_ZONE_REWARD_ITEM_CURRENCY, ch->GetCombatZonePoints());
	ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
	ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
	ch->m_pkCombatZoneLeaveEvent = NULL;
	return 0;
}

EVENTFUNC(combat_zone_event)
{
	if (!event || !event->info)
		return 0;

	TCombatZoneEventInfo* info = dynamic_cast<TCombatZoneEventInfo*>(event->info);

	if (!info)
		return 0;

	CCombatZoneManager* pInstanceManager = info->pInstanceManager;

	if (!pInstanceManager)
		return 0;

	if (pInstanceManager->AnalyzeTimeZone(COMBAT_ZONE_CAN_START, pInstanceManager->GetCurrentDay()))
	{
		pInstanceManager->SetStatus(STATE_OPENED);

		std::string s = "|cFFFF0000|h CombatZone is running! Join now";

		network::GGOutputPacket<network::GGNoticePacket> p;
		p->set_message(s);
		p->set_lang_id(-1);
		P2P_MANAGER::instance().Send(p);

		SendNotice(s.c_str());
			
		return PASSES_PER_SEC(25);
	}
	else if (pInstanceManager->AnalyzeTimeZone(COMBAT_ZONE_CAN_FINISH, pInstanceManager->GetCurrentDay()))
	{
		pInstanceManager->InitializeRanking();
		pInstanceManager->SetStatus(STATE_CLOSED);
		return PASSES_PER_SEC(25);
	}	
	return PASSES_PER_SEC(1);
}

EVENTFUNC(combat_zone_announcement)
{
	if (!event || !event->info)
		return 0;

	TCombatZoneEventInfo* info = dynamic_cast<TCombatZoneEventInfo*>(event->info);

	if (!info)
		return 0;

	CCombatZoneManager* pInstanceManager = info->pInstanceManager;

	if (!pInstanceManager)
		return 0;

	if (pInstanceManager->GetStatus() == STATE_CLOSED && quest::CQuestManager::instance().GetEventFlag("combat_zone_notice_disabled") == 0)
	{
		for (int i = 1; i < 11; ++i)
		{
			if (pInstanceManager->AnalyzeTimeZone(COMBAT_ZONE_CAN_START, pInstanceManager->GetCurrentDay(), 60 * i))
			{
				std::string s = "|cFFFF0000|h CombatZone will start in " + std::to_string(i) + " min.";
				
				network::GGOutputPacket<network::GGNoticePacket> p;
				p->set_lang_id(-1);
				p->set_message(s);
				P2P_MANAGER::instance().Send(p);

				SendNotice(s.c_str());
				
				return PASSES_PER_SEC(60);
			}
		}
	}
	else if (pInstanceManager->GetStatus() == STATE_OPENED && quest::CQuestManager::instance().GetEventFlag("combat_zone_notice_disabled") == 0)
	{
		bool closeSoon = false;
		for (int i = 0; i < 15; ++i)
		{
			if (pInstanceManager->AnalyzeTimeZone(COMBAT_ZONE_CAN_FINISH, pInstanceManager->GetCurrentDay(), 60 * i))
			{
				closeSoon = true;
				break;
			}
		}

		if (!closeSoon)
		{
			std::string s = "|cFFFF0000|h CombatZone is running! Join now";

			network::GGOutputPacket<network::GGNoticePacket> p;
			p->set_lang_id(-1);
			p->set_message(s);
			P2P_MANAGER::instance().Send(p);

			SendNotice(s.c_str());
			
			return PASSES_PER_SEC(60*5);
		}
	}
	
	return PASSES_PER_SEC(1);
}

EVENTFUNC(combat_zone_ranking_event)
{
	if (!event || !event->info)
		return 0;

	if (quest::CQuestManager::instance().GetEventFlag("combat_zone_rankingevent"))
		return 0;

	TCombatZoneEventInfo* info = dynamic_cast<TCombatZoneEventInfo*>(event->info);

	if (!info)
		return 0;

	CCombatZoneManager* pInstanceManager = info->pInstanceManager;

	if (!pInstanceManager->IsRunning())
		return PASSES_PER_SEC(60*2);

	pInstanceManager->InitializeRanking();
	return PASSES_PER_SEC(60*5);
}

void CCombatZoneManager::Initialize()
{
	InitializeRanking();
	CheckEventStatus();
	if (map_allow_find(COMBAT_ZONE_MAP_INDEX))
	// if (g_bChannel == COMBAT_ZONE_NEED_CHANNEL)
	{
		TCombatZoneEventInfo* info = AllocEventInfo<TCombatZoneEventInfo>();
		info->pInstanceManager = this;
		m_pkCombatZoneEvent = event_create(combat_zone_event, info, PASSES_PER_SEC(20));
		m_pkCombatZoneAnnouncement = event_create(combat_zone_announcement, info, PASSES_PER_SEC(20));
		m_pkCombatZoneRankingEvent = event_create(combat_zone_ranking_event, info, PASSES_PER_SEC(60));
	}
}

void CCombatZoneManager::Destroy()
{
	CheckEventStatus();
}

std::vector<DWORD> parse_array(DWORD arg1 = 0, DWORD arg2 = 0, DWORD arg3 = 0, DWORD arg4 = 0)
{
    std::vector<DWORD> m_vec_infoData;
    m_vec_infoData.push_back(arg1);
    m_vec_infoData.push_back(arg2);
    m_vec_infoData.push_back(arg3);
    m_vec_infoData.push_back(arg4);
    return m_vec_infoData;
}

void CCombatZoneManager::SendCombatZoneInfoPacket(LPCHARACTER pkTarget, DWORD sub_header, std::vector<DWORD> m_vec_infoData)
{
	if (!pkTarget)
		return;	
	
	DWORD m_pDataArray[COMBAT_ZONE_MAX_ARGS] = {m_vec_infoData[0], m_vec_infoData[1], m_vec_infoData[2], m_vec_infoData[3]};

	network::GCOutputPacket<network::GCSendCombatZonePacket> pack;
	pack->set_sub_header(sub_header);
	pack->set_is_running(IsRunning());

	for (DWORD data : m_pDataArray)
		pack->add_data_infos(data);

	switch (sub_header)
	{
		case COMBAT_ZONE_SUB_HEADER_OPEN_RANKING:
			for (auto& elem : m_pCombatZoneDateTime)
			{
				for (DWORD date_time : elem)
					pack->add_data_days(date_time);
			}
			break;
		default:
			break;
	}

	pkTarget->GetDesc()->Packet(pack);
}

struct FCombatZoneSendLeavingTargetSign
{
	LPCHARACTER pkLeaver;
	DWORD stateType;
	FCombatZoneSendLeavingTargetSign(LPCHARACTER ch, DWORD state) : pkLeaver(ch), stateType(state) {}

	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = static_cast<LPCHARACTER>(ent);
			if (ch && ch->IsPC())
			{
				switch (stateType)
				{
					case COMBAT_ZONE_ADD_LEAVING_TARGET:
					{
						TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(ch->GetPlayerID(), TARGET_TYPE_COMBAT_ZONE, pkLeaver->GetVID());
						if (!pInfo)
							CTargetManager::Instance().CreateTarget(ch->GetPlayerID(), COMBAT_ZONE_INDEX_TARGET, pkLeaver->GetName(), TARGET_TYPE_COMBAT_ZONE, pkLeaver->GetVID(), 0, ch->GetMapIndex(), "1");
					}
					break;

					case COMBAT_ZONE_REMOVE_LEAVING_TARGET:
						CTargetManager::instance().DeleteTarget(ch->GetPlayerID(), COMBAT_ZONE_INDEX_TARGET, pkLeaver->GetName());
						break;
				}
			}
		}
	}
};

struct FCombatZoneWarpToHome
{
	void operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = static_cast<LPCHARACTER>(ent);
			if (ch && ch->IsPC())
			{
				if (CCombatZoneManager::instance().GetStatus() == STATE_CLOSED)
				{
					ch->UpdateCombatZoneRankings(ch->GetName(), ch->GetEmpire(), ch->GetCombatZonePoints()); // Update ranking with the points what he was collected this time.
					ch->SetCombatZonePoints(ch->GetRealCombatZonePoints() + ch->GetCombatZonePoints());
					if (ch->GetCombatZonePoints() > 0)
						ch->AutoGiveItem(ITEM_COMBAT_ZONE_REWARD_ITEM_CURRENCY, ch->GetCombatZonePoints());
				}

				ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_JOIN, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_PARTICIPATE);
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			}
		}
	}
};

void CCombatZoneManager::SendLeavingTargetSign(LPCHARACTER ch, DWORD dwType)
{
	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(COMBAT_ZONE_MAP_INDEX);
	if (pkSectreeMap)
	{
		FCombatZoneSendLeavingTargetSign f(ch, dwType);
		pkSectreeMap->for_each(f);
	}
}

void CCombatZoneManager::RefreshLeavingTargetSign(LPCHARACTER ch)
{
	for (std::map<std::string, DWORD>::const_iterator it = m_kMapCombatZoneTargetSign.begin(); it != m_kMapCombatZoneTargetSign.end(); ++it)
	{
		TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(ch->GetPlayerID(), TARGET_TYPE_COMBAT_ZONE, it->second);
		if (!pInfo)
			CTargetManager::Instance().CreateTarget(ch->GetPlayerID(), COMBAT_ZONE_INDEX_TARGET, it->first.c_str(), TARGET_TYPE_COMBAT_ZONE, it->second, 0, ch->GetMapIndex(), "1");
	}
}

bool CCombatZoneManager::CanUseAction(LPCHARACTER ch, DWORD bType)
{
	DWORD iTimeElapsed = ch->GetQuestFlag((bType == COMBAT_ZONE_ACTION_PARTICIPATE) ? COMBAT_ZONE_FLAG_WAIT_TIME_JOIN : COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION);
	if (iTimeElapsed && (get_global_time() < iTimeElapsed))
	{	
		DWORD iAmount = (iTimeElapsed - get_global_time());
		DWORD iSec = iAmount % 60;
			iAmount /= 60;
		DWORD iMin = iAmount % 60;
			iAmount /= 60;
		DWORD iHour = iAmount % 24;
		DWORD iDay = iAmount / 24;
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to wait [%d Day's] [%d Hour's] [%d Min] [%d Sec] to do that."), iDay, iHour, iMin, iSec);
		return false;
	}
	
	return true;
}

bool CCombatZoneManager::CanJoin(LPCHARACTER ch)
{
	if (!CCombatZoneManager::instance().IsRunning())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Combatzone hasn't started yet."));
		return false;
	}
	
/*	if (g_bChannel != COMBAT_ZONE_NEED_CHANNEL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Is available only on channel %d."), COMBAT_ZONE_NEED_CHANNEL);
		return false;
	}*/
	
	// All 50 or higher level players can enter.
	if (ch->GetLevel() < COMBAT_ZONE_MIN_LEVEL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your level has to be %d to join."), COMBAT_ZONE_MIN_LEVEL);
		return false;	
	}

	if (ch->GetPoint(POINT_ATTBONUS_HUMAN) < 100)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You don't have more than %d%% Strong against Halfhuman to join the Combat Zone"), 100);
		return false;
	}
	
	if (!CCombatZoneManager::instance().CanUseAction(ch, COMBAT_ZONE_ACTION_PARTICIPATE))
		return false;
	
	return true;
}

void CCombatZoneManager::RequestPotion(LPCHARACTER ch)
{
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, "Disabled.");
	return;
	
	// Check the last time when you request the potion
	if (!CCombatZoneManager::instance().CanUseAction(ch, COMBAT_ZONE_ACTION_REQUEST_POTION))
		return;
	
	DWORD bMonsterCount = ch->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED);
	if (bMonsterCount < COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You haven't reached the killed metin limit (%d from %d)."), bMonsterCount, COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT);
		return;
	}
	
	ch->AutoGiveItem(ITEM_COMBAT_ZONE_BATTLE_POTION, ITEM_COMBAT_ZONE_BATTLE_POTION_COUNT);
	ch->SetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION, get_global_time() + COMBAT_ZONE_WAIT_TIME_TO_REQUEST_POTION);
	ch->SetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED, 0);
	CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_OPEN_RANKING, parse_array(ch->GetRealCombatZonePoints(), (ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) > 0) ? ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) - get_global_time() : 0, ch->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED), COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT));
}

void CCombatZoneManager::Leave(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	DWORD iCombatZonePoints = ch->GetCombatZonePoints(); // Points collected by kills on map
	
	// No Battle Points: You can leave the War Zone immediately by tapping the symbol on the side of the small map.
	if (iCombatZonePoints == 0)
	{
		WarpSetByTime(ch, COMBAT_ZONE_ACTION_LEAVE, COMBAT_ZONE_LEAVE_REGULAR_COUNTDOWN_WARP_SECONDS);
		return;
	}
	
	// Under 5 Battle Points: If a player who announced his withdrawal from the War Zone is killed and dropped below 5 points, he will be able to leave the in 15 seconds. It does not have to be revived.
	if (ch->IsDead() && (iCombatZonePoints < COMBAT_ZONE_REQUIRED_POINTS_TO_LEAVING_WHEN_DEAD && iCombatZonePoints != 0))
	{
		ch->SetRealCombatZonePoints(ch->GetRealCombatZonePoints() + ch->GetCombatZonePoints());
		WarpSetByTime(ch, COMBAT_ZONE_ACTION_LEAVE, COMBAT_ZONE_LEAVE_WHEN_DEAD_UNDER_MIN_POINTS);
		return;
	}
	
	if (iCombatZonePoints < COMBAT_ZONE_REQUIRED_POINTS_TO_LEAVING)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't announced your withdrawal if you not have more then %d points."), COMBAT_ZONE_REQUIRED_POINTS_TO_LEAVING);
		return;
	}
	
	// You can't announce withdrawal when you are dead, you can do this just if you have more less then 5 points.
	if (ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't leave until you are dead."));
		return;
	}
	
	itertype(m_kMapCombatZoneTargetSign) it = m_kMapCombatZoneTargetSign.find(ch->GetName());
	bool isAttachedTargetSign = it != m_kMapCombatZoneTargetSign.end();

	if (isAttachedTargetSign)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your withrawal action is still active."));
		return;
	}

	/*
		With battle points:
		You can leave the War Zone immediately by tapping the symbol on the side of the small map. As soon as you do that, an arrow above your head will mark you while you wait to leave the War Zone. 
		Any player who will kill you during this time will accumulate 5 points of battle.
		Your job is to survive another 2 minutes in the War Zone and get rid of the intact battle points.

		If you die in the 2 minutes, your retreat from the War Zone will be interrupted and you will lose 50% of the battle points. 
		After that, you will be able to announce your withdrawal again from the War Zone.
	*/
	
	TCombatZoneLeaveEventInfo* info = AllocEventInfo<TCombatZoneLeaveEventInfo>();
	info->ch = ch;
	info->bSeconds = COMBAT_ZONE_TARGET_NEED_TO_STAY_ALIVE;
	ch->m_pkCombatZoneLeaveEvent = event_create(combat_zone_leave_event, info, 1);

	ch->AddAffect(AFFECT_COMBAT_ZONE_MOVEMENT, 0, 0, 0, INFINITE_AFFECT_DURATION, 0, false); // Player will suffer a reduction in its Movement Speed	
	ch->SetPoint(POINT_MOV_SPEED, 145);
#ifdef ENABLE_RUNE_SYSTEM
	ch->GetRuneData().storedMovementSpeed = 145;
#endif
	ch->ComputePoints();
	ch->UpdatePacket();
	
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You've announced your withdrawal, you have to stay standing for %d minutes."), (COMBAT_ZONE_TARGET_NEED_TO_STAY_ALIVE / 60));

	CCombatZoneManager::instance().ActTargetSignMap(ch, COMBAT_ZONE_ADD_LEAVING_TARGET);
	CCombatZoneManager::instance().SendLeavingTargetSign(ch, COMBAT_ZONE_ADD_LEAVING_TARGET);
	CCombatZoneManager::instance().Announcement("A player has announced their withdrawal, kill him to get points.");
}

void CCombatZoneManager::Join(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (!CCombatZoneManager::instance().CanJoin(ch))
		return;

	WarpSetByTime(ch, COMBAT_ZONE_ACTION_PARTICIPATE, COMBAT_ZONE_JOIN_WARP_SECOND);
}

void CCombatZoneManager::SetStatus(DWORD bStatus)
{
	m_eState = bStatus;
	switch (bStatus)
	{
		case STATE_CLOSED:
		{
			CCombatZoneManager::instance().Announcement("It's over, congratulations to all the participants.");

			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(COMBAT_ZONE_MAP_INDEX);
			if (pkSectreeMap)
			{
				struct FCombatZoneWarpToHome f;
				pkSectreeMap->for_each(f);
			}
			break;
		}
	}

	quest::CQuestManager::instance().RequestSetEventFlag("combat_zone_event", bStatus);	
}

bool CCombatZoneManager::IsRunning()
{
	return quest::CQuestManager::instance().GetEventFlag("combat_zone_event");
}

bool CCombatZoneManager::AnalyzeTimeZone(DWORD searchType, DWORD searchDay, DWORD addition)
{
	time_t ct = get_global_time() + addition;
	struct tm tm = *localtime(&ct);
	switch (searchType)
	{
		/*
			SearchDay result the current day from freebsd date_time
			Check if current info h/m/s is equal with settings by day in config from m_pCombatZoneDateTime array.
		*/

		// lets asume that the combat zone starts and finishes on the same day (so no things like starts 23:00 and finished at 03:00)
		case COMBAT_ZONE_CAN_START:
			return m_eState != STATE_OPENED && 
				(tm.tm_hour == m_pCombatZoneDateTime[searchDay][0] && tm.tm_min >= m_pCombatZoneDateTime[searchDay][1] ||
				tm.tm_hour > m_pCombatZoneDateTime[searchDay][0]) &&
				(tm.tm_hour < m_pCombatZoneDateTime[searchDay][2] ||
				tm.tm_hour == m_pCombatZoneDateTime[searchDay][2] && tm.tm_min < m_pCombatZoneDateTime[searchDay][3]);

		case COMBAT_ZONE_CAN_FINISH:
			return m_eState != STATE_CLOSED &&
				(tm.tm_hour == m_pCombatZoneDateTime[searchDay][2] && tm.tm_min >= m_pCombatZoneDateTime[searchDay][3] ||
				tm.tm_hour > m_pCombatZoneDateTime[searchDay][2]);
	}

	return false;
}

void CCombatZoneManager::CheckEventStatus()
{
	m_kMapCombatZoneTargetSign.clear();
	SetStatus(STATE_CLOSED);

	if (m_pkCombatZoneEvent)
	{
		event_cancel(&m_pkCombatZoneEvent);
		m_pkCombatZoneEvent = NULL;
	}
	if (m_pkCombatZoneRankingEvent)
	{
		event_cancel(&m_pkCombatZoneRankingEvent);
		m_pkCombatZoneRankingEvent = NULL;
	}
	if (m_pkCombatZoneAnnouncement)
	{
		event_cancel(&m_pkCombatZoneAnnouncement);
		m_pkCombatZoneAnnouncement = NULL;
	}
}

struct FuncFlash
{
	FuncFlash()
	{}

	void operator () (LPDESC d)
	{
		// Operator to send flash on buttonon minimap (all channels) when combat zone start is running for full periodly.
		if (d->GetCharacter() && !CCombatZoneManager::instance().IsCombatZoneMap(d->GetCharacter()->GetMapIndex()))
			CCombatZoneManager::instance().SendCombatZoneInfoPacket(d->GetCharacter(), COMBAT_ZONE_SUB_HEADER_FLASH_ON_MINIMAP, parse_array());	
	}
};

void CCombatZoneManager::Flash()
{
	const DESC_MANAGER::DESC_SET & f = DESC_MANAGER::instance().GetClientSet();
	std::for_each(f.begin(), f.end(), FuncFlash());
}

void CCombatZoneManager::CalculatePointsByKiller(LPCHARACTER ch, bool isAttachedTargetSign)
{
	ch->SetCombatZonePoints(ch->GetCombatZonePoints() + (isAttachedTargetSign ? COMBAT_ZONE_ADD_POINTS_TARGET_KILLING : COMBAT_ZONE_ADD_POINTS_NORMAL_KILLING));
	ch->UpdatePacket();
}

void CCombatZoneManager::ActTargetSignMap(LPCHARACTER ch, DWORD bType)
{
	switch (bType)
	{
		case COMBAT_ZONE_ADD_LEAVING_TARGET:
			m_kMapCombatZoneTargetSign.insert(std::make_pair(ch->GetName(), ch->GetVID()));
			break;
		case COMBAT_ZONE_REMOVE_LEAVING_TARGET:
			m_kMapCombatZoneTargetSign.erase(ch->GetName());
			break;	
	}
}

void CCombatZoneManager::OnDead(LPCHARACTER pkKiller, LPCHARACTER pkVictim)
{
	// Check if exist the killer and victim and if they are players not monsters, npc, stone.
	if (!pkKiller || !pkVictim || !pkKiller->IsPC())
		return;

	/************************************************************************/
	/* Kill monsters for can get potion										*/
	/************************************************************************/	
	
	if (pkVictim->IsStone())
	{
		DWORD iMonstersKilled = pkKiller->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED);
		if (iMonstersKilled < COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT)
		{
			iMonstersKilled += 1;
			pkKiller->SetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED, iMonstersKilled);
			pkKiller->tchat("You killed a metin, current status: (%d from %d).", iMonstersKilled, COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT);
			return;
		}
	}
	
	/************************************************************************/
	/* Kill players on combat zone map										*/
	/************************************************************************/
	
	if (CCombatZoneManager::instance().IsCombatZoneMap(pkKiller->GetMapIndex()))
	{
		// Ugly this but that happens when you try in same time to kills mobs for potion and teleporting to combat zone and if you have a delay 3-5 seconds after warp, victim (mobs) what you wass killing will take like a pc because setMapIndex already was seted to combat zone map.
		if (!IsCombatZoneMap(pkVictim->GetMapIndex()))
			return;
		
		char c_pszTime[128];
		snprintf(c_pszTime, sizeof(c_pszTime), COMBAT_ZONE_FLAG_KILL_LAST_TIME, pkVictim->GetPlayerID());
		int dwKillLastTime = pkKiller->GetQuestFlag(c_pszTime);
		
		// If victim dead was effect of potion battle will be deleted.
		CCombatZoneManager::instance().RemoveAffect(pkVictim);
		
		// If victim dead have attached the target on head
		itertype(m_kMapCombatZoneTargetSign) it = m_kMapCombatZoneTargetSign.find(pkVictim->GetName());
		bool isAttachedTargetSign = it != m_kMapCombatZoneTargetSign.end();
		
		if (isAttachedTargetSign)
			CCombatZoneManager::instance().ActTargetSignMap(pkVictim, COMBAT_ZONE_REMOVE_LEAVING_TARGET);
		
		/*
			* If victim have attached target on head the killer will receive 5 points, it not 1.
			* Victim dead will lost 50% of current points.
			* Kill the same player again within 5 minutes, however, and you won't receive any points.
		*/
		
		if (get_global_time() < dwKillLastTime)
		{
			pkKiller->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkKiller, "You didn't receive any points, wait for %d seconds to kill the same player again."), COMBAT_ZONE_WAIT_TIME_KILL_AGAIN_PLAYER);
		}
		else
		{
			CCombatZoneManager::instance().CalculatePointsByKiller(pkKiller, isAttachedTargetSign);
			pkKiller->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkKiller, "Currently you have %d points. Your total score is %d."), (isAttachedTargetSign ? COMBAT_ZONE_ADD_POINTS_TARGET_KILLING : COMBAT_ZONE_ADD_POINTS_NORMAL_KILLING), pkKiller->GetCombatZonePoints());
			pkKiller->SetQuestFlag(c_pszTime, get_global_time() + (!test_server ? COMBAT_ZONE_WAIT_TIME_KILL_AGAIN_PLAYER : 0));
		}

		pkVictim->SetCombatZoneDeaths(pkVictim->GetCombatZoneDeaths() + COMBAT_ZONE_ADD_DEATHS_POINTS);
		// pkVictim->SetCombatZonePoints(pkVictim->GetCombatZonePoints() / COMBAT_ZONE_DIVIDE_NUM_POINTS);
		pkVictim->SetCombatZonePoints(pkVictim->GetCombatZonePoints() - COMBAT_ZONE_DEATH_DELETE_NUM_POINTS);
		
		if (isAttachedTargetSign)
			pkVictim->SetCombatZonePoints(pkVictim->GetCombatZonePoints() - COMBAT_ZONE_DEATH_DELETE_WHILE_EAVE_NUM_POINTS);

		pkVictim->UpdatePacket();

		if (pkVictim->GetCombatZonePoints())
			pkVictim->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkVictim, "You lost half of the points. Points left: %d."), pkVictim->GetCombatZonePoints());
	}
}

void CCombatZoneManager::RemoveAffect(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	const DWORD m_pkAffectCombatZone[3] = {
		AFFECT_COMBAT_ZONE_MOVEMENT, 
		AFFECT_COMBAT_ZONE_POTION, 
		AFFECT_COMBAT_ZONE_DEFENSE
	};
	
	for (int i=0; i<_countof(m_pkAffectCombatZone); i++)
	{
		const CAffect* pAffect = ch->FindAffect(m_pkAffectCombatZone[i]);

		if (pAffect)
			ch->RemoveAffect(const_cast<CAffect*>(pAffect));
	}
			
	if (ch->FindAffect(AFFECT_MOUNT))
	{
		ch->RemoveAffect(AFFECT_MOUNT);
		ch->RemoveAffect(AFFECT_MOUNT_BONUS);
		ch->MountVnum(0);
	}
		
	if (ch->IsPolymorphed())
	{
		ch->SetPolymorph(0);
		ch->RemoveAffect(AFFECT_POLYMORPH);
	}

	if (ch->GetMountSystem() && ch->GetMountSystem()->IsRiding())
		ch->GetMountSystem()->StopRiding();
}

void CCombatZoneManager::OnLogout(LPCHARACTER ch)
{
	if (!ch)
		return;

	if (CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
	{
		CCombatZoneManager::instance().RemoveAffect(ch);

		// If he logout from map the points collected and deaths collected will be deleted and will teleport on map1 and give the skills back by cache.
		CCombatZoneManager::instance().ActTargetSignMap(ch, COMBAT_ZONE_REMOVE_LEAVING_TARGET);
#if defined(COMBAT_ZONE_SET_SKILL_PERFECT)
		CCombatZoneManager::instance().SetSkill(ch, COMBAT_ZONE_GET_SKILL_BACK_BY_CACHE);
#endif
		ch->SetCombatZoneDeaths(0);	 // Set deaths points to 0
		ch->SetCombatZonePoints(0);	 // Set points collected to 0
	}
}

struct FCheckIP
{
	LPCHARACTER ch;
	LPCHARACTER chReturn;
	FCheckIP(LPCHARACTER p)
	{
		ch = p;
		chReturn = NULL;
	}

	void operator()(LPENTITY ent)
	{
		if (NULL != ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pkChr = static_cast<LPCHARACTER>(ent);
				if (pkChr->IsPC() && pkChr != ch)
				{
					if (!strcmp(ch->GetEscapedHWID(), pkChr->GetEscapedHWID()) || !strcmp(ch->GetDesc()->GetHostName(), pkChr->GetDesc()->GetHostName()))
						chReturn = pkChr;
				}
			}
		}
	}
};

void CCombatZoneManager::OnLogin(LPCHARACTER ch)
{
	if (!ch)
		return;
	
	if (CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
	{
		CCombatZoneManager::instance().RemoveAffect(ch);

		// ch->RemoveGoodAffect();
		
		ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
		ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

		// If try to login on combat zone on another channel with other methods then join button, warp them on map1.
		// If a player maybe he login on map when combat zone isn't active, that can happens very rare like if he login after some days and still remaning in map, because action to logout doesn't was with succesfully.
		if (!CCombatZoneManager::instance().IsRunning()/* || g_bChannel != COMBAT_ZONE_NEED_CHANNEL*/)
		{
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
			return;
		}
		
		CCombatZoneManager::instance().ActTargetSignMap(ch, COMBAT_ZONE_REMOVE_LEAVING_TARGET);
		CCombatZoneManager::instance().RefreshLeavingTargetSign(ch);
#if defined(COMBAT_ZONE_SET_SKILL_PERFECT)
		CCombatZoneManager::instance().AppendSkillCache(ch); // All players' skills are set to 'Perfect Master',
#endif		

		DWORD ADDED_DEFENSE = (DEF_ADDED_BONUS - (DEF_MULTIPLIER * (ch->GetLevel() - COMBAT_ZONE_MIN_LEVEL)) - (ch->GetLevel() - COMBAT_ZONE_MIN_LEVEL));
		ch->AddAffect(AFFECT_COMBAT_ZONE_DEFENSE, POINT_DEF_GRADE, (ADDED_DEFENSE < 0 ? 0 : ADDED_DEFENSE), AFF_NONE, INFINITE_AFFECT_DURATION, 0, false, false);
		ch->UpdatePacket();

		// Kick out double accounts...
		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
		FCheckIP f(ch);
		pSecMap->for_each(f);

		if (f.chReturn)
		{
			LogManager::instance().HackDetectionLog(ch, "COMBAT_ZONE_MULTI", ch->GetDesc()->GetHostName());
			LogManager::instance().HackDetectionLog(f.chReturn, "COMBAT_ZONE_MULTI", f.chReturn->GetDesc()->GetHostName());
			ch->ChatPacket(CHAT_TYPE_INFO, "Only 1 participant by IP/PC");
			// ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
}

const DWORD * CCombatZoneManager::GetSkillList(LPCHARACTER ch)
{
	static const DWORD matrixArraySkill[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][SKILL_COUNT_INDEX] =
	{
		{ {	1,	2,	3,	4,	5,	6	}, {	16,	17,	18,	19,	20,	21	} }, // WARRIOR
		{ {	31,	32,	33,	34,	35,	36	}, {	46,	47,	48,	49,	50,	51	} }, // ASSASSIN
		{ {	61,	62,	63,	64,	65,	66	}, {	76,	77,	78,	79,	80,	81	} }, // SURA
		{ {	91,	92,	93,	94,	95,	96	}, {	106,107,108,109,110,111	} }, // SHAMAN
#if defined(ENABLE_WOLFMAN_CHARACTER) || defined(WOLFMAN_CHARACTER)
		{ { 170,171,172,173,174,175 }, {	170,171,172,173,174,175 } } // WOLFMAN
#endif
	};
	return matrixArraySkill[ch->GetJob()][ch->GetSkillGroup() - 1];
}

void CCombatZoneManager::AppendSkillCache(LPCHARACTER ch)
{
	// He don't have the skills selected so we dont will store nothing.
	if (ch->GetSkillGroup() == 0)
		return;

	const DWORD * matrixArraySkill = CCombatZoneManager::instance().GetSkillList(ch);
	
	network::GDOutputPacket<network::GDCombatZoneSkillsCachePacket> p;
	p->set_pid(ch->GetPlayerID());
	p->set_skill_level1(ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_1]));	
	p->set_skill_level2(ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_2]));	
	p->set_skill_level3(ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_3]));	
	p->set_skill_level4(ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_4]));	
	p->set_skill_level5(ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_5]));	
	p->set_skill_level6(ch->GetSkillLevel(matrixArraySkill[SKILL_VNUM_6]));		
	db_clientdesc->DBPacket(p);

	// Set perfect skills after cached all old skills
	CCombatZoneManager::instance().SetSkill(ch, COMBAT_ZONE_SET_SKILL_MAX_LEVEL);
}

void CCombatZoneManager::SetSkill(LPCHARACTER ch, DWORD bState)
{
	if (!ch)
		return;

	if (ch->GetSkillGroup() == 0)
		return;

	const DWORD * matrixArraySkill = CCombatZoneManager::instance().GetSkillList(ch);
	switch (bState)
	{
		case COMBAT_ZONE_SET_SKILL_MAX_LEVEL:
		{
			for (int i = 0; i < SKILL_COUNT_INDEX; ++i)
				ch->SetSkillLevel(matrixArraySkill[i], SKILL_MAX_LEVEL);	
		}
		break;

		case COMBAT_ZONE_GET_SKILL_BACK_BY_CACHE:
		{
			std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT skillLevel1, skillLevel2, skillLevel3, skillLevel4, skillLevel5, skillLevel6 FROM player.combat_zone_skills_cache WHERE pid = '%d'", ch->GetPlayerID()));
			if (!pMsg->Get()->uiNumRows)
				return;

			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult); // crash fix .. foreach cols instead of rows

			for (int i = 0; i < SKILL_COUNT_INDEX; ++i)
			{
				if (!matrixArraySkill[i]) // if class dont got 6 skills
					continue;

				ch->SetSkillLevel(matrixArraySkill[i], atoi(row[i]));
			}
		}
		break;
	}

	ch->ComputePoints();
	ch->SkillLevelPacket();
}

void CCombatZoneManager::OnRestart(LPCHARACTER ch, int subcmd)
{
	if (!ch)
		return;
	
	// The waiting time for restart is' 10 seconds', which rises by 5 seconds whenever the character is killed within the map, up to a maximum of 30 seconds; 
	// When he collect the 30 seconds to wait, next death will be reseted the deaths to 0 for can start again calculation seconds by deaths;
	if (ch->GetCombatZoneDeaths() == COMBAT_ZONE_MAX_DEATHS_TO_INCREASE_TIMER_RESTART - 1)
		ch->SetCombatZoneDeaths(0);

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);
	int iSecondsRequestToWait = 170 - (ch->GetCombatZoneDeaths() * COMBAT_ZONE_INCREASE_SECONDS_RESTART);

	switch (subcmd)
	{
		case SCMD_RESTART_HERE:
		case SCMD_RESTART_TOWN:
		{
			if (iTimeToDead > iSecondsRequestToWait)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can not restart from town yet. Wait for %d of seconds."), (iTimeToDead - iSecondsRequestToWait));
				return;
			}

			// Coordinates random for each restart on town
			DWORD dwIndex = GetRandomPos();

			ON_SUCCES_RESTART(ch);
			ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
			ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
			ch->Show(COMBAT_ZONE_MAP_INDEX, objectPos[dwIndex].x, objectPos[dwIndex].y, 0);
			ch->Stop();
			ch->UpdatePacket();
		}
		break;
			
		case SCMD_RESTART_COMBAT_ZONE:
		{
			if (!CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
				return;
			
			if (!ch->CountSpecifyItem(ITEM_COMBAT_ZONE_REINCARNATION))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need item %s to allows you to revive without the waiting period."), ITEM_MANAGER::instance().GetTable(ITEM_COMBAT_ZONE_REINCARNATION)->locale_name(ch->GetLanguageID()).c_str());
				return;
			}
			
			ON_SUCCES_RESTART(ch);
			ch->RemoveSpecifyItem(ITEM_COMBAT_ZONE_REINCARNATION, 1);
			ch->RestartAtSamePos();
			ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
			ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
			ch->ReviveInvisible(3);
		}
		break;
	}
}

void CCombatZoneManager::ShowCurrentTimeZone(LPCHARACTER ch)
{
	if (!ch)
		return;

	time_t currentTime;
	struct tm *localTime;
	time(&currentTime);
	localTime = localtime(&currentTime);
	
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Current local timezone on server: %s"), asctime(localTime));
}

/************************************************************************/
/* MEMBER ACTION														*/
/************************************************************************/
void CCombatZoneManager::RequestAction(LPCHARACTER ch, std::unique_ptr<network::CGCombatZoneRequestActionPacket> p)
{
	if (!ch)
		return;
	
	switch (p->action())
	{
		case COMBAT_ZONE_ACTION_OPEN_RANKING:
#if defined(COMBAT_ZONE_SHOW_SERVER_TIME_ZONE_ON_CHAT)
			ShowCurrentTimeZone(ch);
#endif
			CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_OPEN_RANKING, parse_array(ch->GetRealCombatZonePoints(), (ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) > 0) ? ch->GetQuestFlag(COMBAT_ZONE_FLAG_WAIT_TIME_REQUEST_POTION) - get_global_time() : 0, ch->GetQuestFlag(COMBAT_ZONE_FLAG_MONSTERS_KILLED), COMBAT_ZONE_MONSTER_KILL_MAX_LIMIT));
			CCombatZoneManager::instance().RequestRanking(ch, COMBAT_ZONE_TYPE_RANKING_WEEKLY);
			return;
			
		case COMBAT_ZONE_ACTION_CHANGE_PAGE_RANKING:
		{
			switch (p->value())
			{
				case COMBAT_ZONE_TYPE_RANKING_WEEKLY: 
					CCombatZoneManager::instance().RequestRanking(ch, COMBAT_ZONE_TYPE_RANKING_WEEKLY);
					return;

				case COMBAT_ZONE_TYPE_RANKING_ALL: 
					CCombatZoneManager::instance().RequestRanking(ch, COMBAT_ZONE_TYPE_RANKING_ALL); 
					return;

				default:
					return;
			}
		}
			
		case COMBAT_ZONE_ACTION_PARTICIPATE:
			CCombatZoneManager::instance().Join(ch);
			return;
		
		case COMBAT_ZONE_ACTION_LEAVE:
			CCombatZoneManager::instance().Leave(ch);
			return;
			
		case COMBAT_ZONE_ACTION_REQUEST_POTION:
			CCombatZoneManager::instance().RequestPotion(ch);
			return;

		default:
			return;
	}
}

void CCombatZoneManager::Announcement(const char * format, ...)
{
	if (!format)
		return;

	char szBuf[CHAT_MAX_LEN + 1];
	va_list args;

	// Initializes ap to retrieve the additional arguments after parameter
	// Write formatted data from variable argument list to sized buffer
	va_start(args, format);
	vsnprintf(szBuf, sizeof(szBuf), format, args);
	va_end(args);
	SendNoticeMap(szBuf, COMBAT_ZONE_MAP_INDEX, true);
}

bool CCombatZoneManager::IsCombatZoneMap(int iMapIndex)
{
	if (iMapIndex == COMBAT_ZONE_MAP_INDEX)
		return true;
		
	return false;
}

/************************************************************************/
/* GUI RANKING IN-GAME													*/
/************************************************************************/
void CCombatZoneManager::RequestRanking(LPCHARACTER ch, DWORD bType)
{
	BYTE i = 0;
	network::TCombatZoneRankingPlayer *rankingData;

	network::GCOutputPacket<network::GCCombatZoneRankingDataPacket> p;

	switch (bType)
	{
		case COMBAT_ZONE_TYPE_RANKING_WEEKLY:
			rankingData = weeklyRanking;
			break;
		case COMBAT_ZONE_TYPE_RANKING_ALL:
			rankingData = generalRanking;
			break;
	}

	for (i = 0; i < COMBAT_ZONE_MAX_ROWS_RANKING; i++)
	{
		auto elem = p->add_data();
		elem->set_rank(i);
		elem->set_name(rankingData[i].name());
		elem->set_empire(rankingData[i].empire());		
		elem->set_points(rankingData[i].points());
		ch->tchat("CombatZoneRanking[%d] #%d %s %d %d", bType, elem->rank(), elem->name().c_str(), elem->empire(), elem->points());
	}

	// // Special slot rank for can see by self.
	// std::auto_ptr<SQLMsg> pFindSQL(DBManager::instance().DirectQuery(
	// 	"SELECT memberName, memberEmpire, memberPoints, "
	// 	"FIND_IN_SET (memberPoints, (SELECT GROUP_CONCAT(memberPoints ORDER BY memberPoints DESC) "
	// 		"FROM %s)) AS rank "
	// 	"FROM %s WHERE memberName = '%s'", szQuery, szQuery, ch->GetName()
	// ));
	
	// BYTE bLastSlot = COMBAT_ZONE_MAX_ROWS_RANKING;
	// if (pFindSQL->Get()->uiNumRows > 0)
	// {
	// 	MYSQL_ROW rows = mysql_fetch_row(pFindSQL->Get()->pSQLResult);
		
	// 	p->ranking_data()[bLastSlot] = TPacketGCCombatZoneRanking();
	// 	str_to_number(p->ranking_data()[bLastSlot].rank, rows[3]);	
	// 	strncpy(p->ranking_data()[bLastSlot].name, rows[0], sizeof(p->ranking_data()[bLastSlot].name));
	// 	str_to_number(p->ranking_data()[bLastSlot].empire, rows[1]);		
	// 	str_to_number(p->ranking_data()[bLastSlot].points, rows[2]);	
	// }
	// else
	// {
	// 	p->ranking_data()[bLastSlot] = TPacketGCCombatZoneRanking();
	// 	p->ranking_data()[bLastSlot].rank = COMBAT_ZONE_EMPTY_VALUE_ROW;
	// 	strncpy(p->ranking_data()[bLastSlot].name, "", sizeof(p->ranking_data()[bLastSlot].name));
	// 	p->ranking_data()[bLastSlot].empire = COMBAT_ZONE_EMPTY_VALUE_ROW;
	// 	p->ranking_data()[bLastSlot].points = COMBAT_ZONE_EMPTY_VALUE_ROW;
	// }

	ch->GetDesc()->Packet(p);
}

bool CCombatZoneManager::CanUseItem(LPCHARACTER ch, LPITEM item)
{
	if (!ch || !item)
		return false;

	switch (item->GetVnum())
	{
		case ITEM_COMBAT_ZONE_BATTLE_POTION:
		{
			if (!CCombatZoneManager::instance().IsCombatZoneMap(ch->GetMapIndex()))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this on this map."));
				return false;
			}
			
			if (ch->FindAffect(AFFECT_COMBAT_ZONE_POTION))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The effect is still active."));
				return false;
			}
			
			// Affect have attached bonus until you are not dead, when you dead lost the bonus hp.
			ch->AddAffect(AFFECT_COMBAT_ZONE_POTION, POINT_MAX_HP, BATTLE_POTION_MAX_HP, AFF_NONE, INFINITE_AFFECT_DURATION, 0, 0, true);
			ch->AddAffect(AFFECT_COMBAT_ZONE_POTION, POINT_ATT_GRADE_BONUS, BATTLE_POTION_MAX_ATT, AFF_NONE, INFINITE_AFFECT_DURATION, 0, 0, false);
			ch->RemoveSpecifyItem(item->GetVnum(), 1);
#if defined(COMBAT_ZONE_SHOW_EFFECT_POTION)
			ch->EffectPacket(SE_COMBAT_ZONE_POTION);
#endif
		}
		break;

		case ITEM_COMBAT_ZONE_FIELD_BOX_1:
		case ITEM_COMBAT_ZONE_FIELD_BOX_2:
		case ITEM_COMBAT_ZONE_WOODEN_CHEST:
		{
			std::vector <DWORD> dwVnums;
			std::vector <DWORD> dwCounts;
			std::vector <LPITEM> item_gets;
			int count = 0;
			
			if (ch->GiveItemFromSpecialItemGroup(item->GetVnum(), dwVnums, dwCounts, item_gets, count))
			{
				item->SetSocket(0, item->GetSocket(0) + 1);

				if (item->GetSocket(0) >= COMBAT_ZONE_CHEST_MAX_OPENED)
				{
					ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (COMBAT_ZONE_ITEM_CHEST)");
					return false;
				}
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "There are no items in this box."));
				return false;
			}
		}
		break;
	}
	
	return true;
}

DWORD CCombatZoneManager::GetFirstDayHour()
{
	time_t time_now = time(NULL);
	tm * time_struct = localtime(&time_now);
	time_struct->tm_hour = 0;
	time_struct->tm_min = 0;
	time_struct->tm_sec = 0;

	time_t time_stamp_hour = mktime(time_struct);
    return time_stamp_hour;
}

DWORD CCombatZoneManager::GetCurrentDay()
{
	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);
	
	switch (tm.tm_wday)
	{
		case 0:
			return DAY_SUNDAY;
		case 1:
			return DAY_MONDAY;
		case 2:
			return DAY_TUESDAY;
		case 3:
			return DAY_WEDNESDAY;
		case 4:
			return DAY_THURSDAY;
		case 5:
			return DAY_FRIDAY;
		case 6:
			return DAY_SATURDAY;
	}

	return DAY_MONDAY;
}

CCombatZoneManager::CCombatZoneManager() : m_eState(0)
{
	//Initialize();
}

bool CCombatZoneManager::InitializeRanking()
{
	if (!map_allow_find(COMBAT_ZONE_MAP_INDEX))
		return false;

	std::auto_ptr<SQLMsg> pFindSQL(DBManager::instance().DirectQuery(
		"SELECT memberName, memberEmpire, memberPoints FROM combat_zone_ranking_weekly ORDER BY memberPoints DESC LIMIT %d", COMBAT_ZONE_MAX_ROWS_RANKING
	));

	for (auto& elem : weeklyRanking)
		elem.Clear();

	if (pFindSQL->Get()->uiNumRows > 0)
	{
		BYTE i = 0;
		MYSQL_ROW data = NULL;
		while ((data = mysql_fetch_row(pFindSQL->Get()->pSQLResult)))
		{
			BYTE col = 0;
			weeklyRanking[i].set_name(data[col++]);
			weeklyRanking[i].set_empire(std::atoi(data[col++]));
			weeklyRanking[i].set_points(std::atoi(data[col++]));
			i++;
		}
	}

	std::auto_ptr<SQLMsg> pFindSQL2(DBManager::instance().DirectQuery(
		"SELECT memberName, memberEmpire, memberPoints FROM combat_zone_ranking_general ORDER BY memberPoints DESC LIMIT %d", COMBAT_ZONE_MAX_ROWS_RANKING
	));
	
	for (auto& elem : generalRanking)
		elem.Clear();

	if (pFindSQL2->Get()->uiNumRows > 0)
	{
		BYTE i = 0;
		MYSQL_ROW data = NULL;
		while ((data = mysql_fetch_row(pFindSQL2->Get()->pSQLResult)))
		{
			BYTE col = 0;
			generalRanking[i].set_name(data[col++]);
			generalRanking[i].set_empire(std::atoi(data[col++]));
			generalRanking[i].set_points(std::atoi(data[col++]));
			i++;
		}
	}

	network::GGOutputPacket<network::GGCombatZoneRankingPacket> p;
	for (auto& player : weeklyRanking)
		*p->add_weekly() = player;
	for (auto& player : generalRanking)
		*p->add_general() = player;
	P2P_MANAGER::instance().Send(p);

	return true;
}

bool CCombatZoneManager::InitializeRanking(const ::google::protobuf::RepeatedPtrField<network::TCombatZoneRankingPlayer>& weekly, const ::google::protobuf::RepeatedPtrField<network::TCombatZoneRankingPlayer>& general)
{
	for (int i = 0; i < MIN(weekly.size(), COMBAT_ZONE_MAX_ROWS_RANKING); ++i)
		weeklyRanking[i] = weekly[i];
	for (int i = 0; i < MIN(general.size(), COMBAT_ZONE_MAX_ROWS_RANKING); ++i)
		generalRanking[i] = general[i];
	return true;
}

BYTE CCombatZoneManager::GetPlayerRank(LPCHARACTER ch)
{
	for (BYTE i = 0; i < 3; i++)
	{
		if (!strcmp(ch->GetName(), weeklyRanking[i].name().c_str()))
			return i + 1;
	}
	return 0;
}

#endif