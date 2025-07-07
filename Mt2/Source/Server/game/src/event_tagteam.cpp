#include "stdafx.h"

#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#include "event_tagteam.h"
#include "config.h"
#include "constants.h"
#include "sectree_manager.h"
#include "char.h"
#include "p2p.h"
#include "desc.h"
#include "char_manager.h"
#include "start_position.h"
#include "db.h"
#include "item.h"
#include "cmd.h"

/*******************************************************************\
| [NONCLASS] Events
\*******************************************************************/
EVENTFUNC(tag_team_player_timeout_event)
{
	CEventTagTeam::attender_event_info* info = dynamic_cast<CEventTagTeam::attender_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("tag_team_player_timeout_event info == NULL");
		return 0;
	}

	CEventTagTeam* pkTagTeam = info->pkEventTagTeam;
	pkTagTeam->EventAttenderTimeout(info->dwAttenderPID);

	return 0;
}

EVENTFUNC(tag_team_player_exit_event)
{
	CEventTagTeam::attender_event_info* info = dynamic_cast<CEventTagTeam::attender_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("tag_team_player_exit_event info == NULL");
		return 0;
	}

	CEventTagTeam* pkTagTeam = info->pkEventTagTeam;
	pkTagTeam->EventAttenderExit(info->dwAttenderPID);

	return 0;
}

EVENTFUNC(tag_team_destroy_event)
{
	CEventTagTeam::tag_team_event_info* info = dynamic_cast<CEventTagTeam::tag_team_event_info*>(event->info);

	if (info == NULL)
	{
		sys_err("tag_team_destroy_event info == NULL");
		return 0;
	}

	CEventTagTeam* pkTagTeam = info->pkEventTagTeam;
	CEventManager::instance().TagTeam_Destroy(pkTagTeam);

	return 0;
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CEventTagTeam - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/
CEventTagTeam::CEventTagTeam()
{
	m_dwMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(EVENT_LABYRINTH_MAP_INDEX);
	m_bAttenderCount = 0;
	m_bTagTeamCount = 0;
	m_bIsOver = false;
	m_pkDestroyEvent = NULL;

	for (int i = 0; i < MAX_TEAM_COUNT; ++i)
		m_vec_PositionIndexes.push_back(i);
}

CEventTagTeam::~CEventTagTeam()
{
	SECTREE_MANAGER::instance().DestroyPrivateMap(m_dwMapIndex);
}

void CEventTagTeam::Destroy()
{
	// remove attenders
	while (!m_set_Attender.empty())
	{
		DWORD dwPID = m_set_Attender.begin()->dwPID;
		RemoveAttender(dwPID);
	}

	// cancel destroy event
	event_cancel(&m_pkDestroyEvent);

	// delete instance
	M2_DELETE_ONLY(this);
}

/*******************************************************************\
| [PUBLIC] Main Functions
\*******************************************************************/
void CEventTagTeam::JoinTagTeam(DWORD dwPID1, const char* szName1, DWORD dwPID2, const char* szName2)
{
	// get random position index
	BYTE bVectorIndex = random_number(0, m_vec_PositionIndexes.size() - 1);
	BYTE bPositionIndex = m_vec_PositionIndexes[bVectorIndex];
	m_vec_PositionIndexes.erase(m_vec_PositionIndexes.begin() + bVectorIndex);

	// get team index
	BYTE bTeamIndex = m_bTagTeamCount++;

	// add attender
	AddAttender(dwPID1, szName1, bTeamIndex, bPositionIndex);
	AddAttender(dwPID2, szName2, bTeamIndex, bPositionIndex);
}

/*******************************************************************\
| [PUBLIC] Trigger Functions
\*******************************************************************/
void CEventTagTeam::OnPlayerOnline(LPCHARACTER pkChr)
{
	TPlayerInfo* pkPlayerInfo = GetAttenderInfo(pkChr->GetPlayerID());
	if (!pkPlayerInfo)
	{
		long lMapIndex = pkChr->GetMapIndex();
		if (lMapIndex >= 10000)
			lMapIndex /= 10000;
		long lWarpMapIndex = pkChr->GetWarpMapIndex();
		if (lWarpMapIndex >= 10000)
			lWarpMapIndex /= 10000;
		if (lMapIndex == lWarpMapIndex/*  || !pkChr->ExitToSavedLocation() */)
			pkChr->GoHome();
		return;
	}

	pkChr->SetPVPTeam(pkPlayerInfo->bTeamIndex);
	event_cancel(&pkPlayerInfo->pkTimeoutEvent);
}

void CEventTagTeam::OnPlayerOffline(LPCHARACTER pkChr)
{
	TPlayerInfo* pkPlayerInfo = GetAttenderInfo(pkChr->GetPlayerID());
	if (!pkPlayerInfo)
		return;

	// dead
	if (pkPlayerInfo->pkExitEvent)
	{
		RemoveAttender(pkChr->GetPlayerID());
	}
	// not dead
	else
	{
		if (!pkPlayerInfo->pkTimeoutEvent)
		{
			attender_event_info* info = AllocEventInfo<attender_event_info>();
			info->pkEventTagTeam = this;
			info->dwAttenderPID = pkChr->GetPlayerID();
			pkPlayerInfo->pkTimeoutEvent = event_create(tag_team_player_timeout_event, info, PASSES_PER_SEC(ATTENDER_TIMEOUT_TIME));
		}
	}
}

void CEventTagTeam::OnPlayerLoadAffect(LPCHARACTER pkChr)
{
	pkChr->RemoveGoodAffect();
	if (m_bAttenderCount <= 2)
		pkChr->StopAutoRecovery(AFFECT_AUTO_HP_RECOVERY);
}

void CEventTagTeam::OnPlayerDead(LPCHARACTER pkChr)
{
	if (m_bIsOver)
		return;

	TPlayerInfo* pkPlayerInfo = GetAttenderInfo(pkChr->GetPlayerID());
	if (!pkPlayerInfo)
		return;

	if (pkPlayerInfo->pkExitEvent)
	{
		sys_err("already has exit event pid %u", pkChr->GetPlayerID());
		return;
	}

	// decrease attender count
	--m_bAttenderCount;

	// stop all auto potions
	if (m_bAttenderCount == 2)
	{
		for (TAttenderSet::iterator it = m_set_Attender.begin(); it != m_set_Attender.end(); ++it)
		{
			if (!it->pkTimeoutEvent && !it->pkExitEvent)
			{
				LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(it->dwPID);
				if (pkChr)
					pkChr->StopAutoRecovery(AFFECT_AUTO_HP_RECOVERY);
			}
		}
	}

	attender_event_info* info = AllocEventInfo<attender_event_info>();
	info->pkEventTagTeam = this;
	info->dwAttenderPID = pkChr->GetPlayerID();
	pkPlayerInfo->pkExitEvent = event_create(tag_team_player_exit_event, info, PASSES_PER_SEC(ATTENDER_EXIT_TIME));

	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "The player [%s] of the tag team [%d] died.", pkChr->GetName(), pkPlayerInfo->bTeamIndex + 1);
	SendNoticeMap(szBuf, m_dwMapIndex, false);

	bool bEntireTeamDead = true;
	for (TAttenderSet::iterator it = m_set_Attender.begin(); it != m_set_Attender.end(); ++it)
	{
		if (it->bTeamIndex != pkPlayerInfo->bTeamIndex)
			continue;

		if (it->dwPID != pkPlayerInfo->dwPID)
		{
			if (!it->pkExitEvent)
				bEntireTeamDead = false;
		}
	}
	if (bEntireTeamDead)
	{
		snprintf(szBuf, sizeof(szBuf), "All player of team [%d] died.", pkPlayerInfo->bTeamIndex + 1);
		SendNoticeMap(szBuf, m_dwMapIndex, false);
	}

	CheckMatch();
}

/*******************************************************************\
| [PUBLIC] Event Functions
\*******************************************************************/
void CEventTagTeam::EventAttenderTimeout(DWORD dwPID)
{
	TPlayerInfo* pkPlayerInfo = GetAttenderInfo(dwPID);
	if (!pkPlayerInfo)
	{
		sys_err("cannot get attender info by pid %u", dwPID);
		return;
	}

	RemoveAttender(dwPID);

	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "The player [%s] of team [%d] has been removed from the game due to a too long disconnect.",
		pkPlayerInfo->szName, pkPlayerInfo->bTeamIndex + 1);
	SendNoticeMap(szBuf, m_dwMapIndex, false);

	CheckMatch();
}

void CEventTagTeam::EventAttenderExit(DWORD dwPID)
{
	ExitAttender(dwPID);
}

/*******************************************************************\
| [PUBLIC] Data Functions
\*******************************************************************/
BYTE CEventTagTeam::GetAttenderCount() const
{
	return m_bAttenderCount;
}

/*******************************************************************\
| [PRIVATE] Data Functions
\*******************************************************************/
void CEventTagTeam::AddAttender(DWORD dwPID, const char* szName, BYTE bTeamIndex, BYTE bPositionIndex)
{
	// add into set
	TPlayerInfo kPlayerInfo;
	memset(&kPlayerInfo, 0, sizeof(TPlayerInfo));
	kPlayerInfo.dwPID = dwPID;
	strlcpy(kPlayerInfo.szName, szName, sizeof(kPlayerInfo.szName));
	kPlayerInfo.bTeamIndex = bTeamIndex;

	attender_event_info* info = AllocEventInfo<attender_event_info>();
	info->pkEventTagTeam = this;
	info->dwAttenderPID = dwPID;
	kPlayerInfo.pkTimeoutEvent = event_create(tag_team_player_timeout_event, info, PASSES_PER_SEC(ATTENDER_TIMEOUT_TIME));

	m_set_Attender.insert(kPlayerInfo);

	// add into name map
	TTeamNameMap::iterator it = m_map_TeamName.find(bTeamIndex);
	if (it == m_map_TeamName.end())
	{
		TTagTeamName& rkTagTeamName = m_map_TeamName[bTeamIndex];
		strlcpy(rkTagTeamName.szName1, szName, sizeof(rkTagTeamName.szName1));
	}
	else
	{
		TTagTeamName& rkTagTeamName = it->second;
		strlcpy(rkTagTeamName.szName2, szName, sizeof(rkTagTeamName.szName2));
	}

	// increase attender count
	++m_bAttenderCount;

	// get warp position
	const TPosition& rkLocalSpawn = EventTagTeamSpawnPosition[bPositionIndex];
	PIXEL_POSITION kBasePos;
	if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(EVENT_LABYRINTH_MAP_INDEX, kBasePos))
	{
		sys_err("cannot get base pos of map %u", EVENT_LABYRINTH_MAP_INDEX);
		return;
	}

	// warp character
	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (!pkChr)
	{
		CCI* pkCCI = P2P_MANAGER::instance().FindByPID(dwPID);
		if (pkCCI)
		{
			LPDESC pkDesc = pkCCI->pkDesc;

			network::GGOutputPacket<network::GGWarpCharacterPacket> p2p_packet;
			p2p_packet->set_pid(dwPID);
			p2p_packet->set_target_pid(0);
			p2p_packet->set_x(kBasePos.x + rkLocalSpawn.dwX * 100);
			p2p_packet->set_y(kBasePos.y + rkLocalSpawn.dwY * 100);
			p2p_packet->set_map_index(m_dwMapIndex);
			pkDesc->Packet(p2p_packet);
		}
		else
		{
			sys_err("cannot send player packet to %u (player not found)", dwPID);
		}
	}
	else
	{
		pkChr->WarpSet(kBasePos.x + rkLocalSpawn.dwX * 100, kBasePos.y + rkLocalSpawn.dwY * 100, m_dwMapIndex);
	}
}

void CEventTagTeam::RemoveAttender(DWORD dwPID)
{
	TPlayerInfo* pkPlayerInfo = GetAttenderInfo(dwPID);
	if (!pkPlayerInfo)
	{
		sys_err("cannot find player to remove %u", dwPID);
		return;
	}

	// decrease attender count if no exit event
	if (!pkPlayerInfo->pkExitEvent)
		--m_bAttenderCount;

	event_cancel(&pkPlayerInfo->pkTimeoutEvent);
	event_cancel(&pkPlayerInfo->pkExitEvent);

	m_set_Attender.erase(dwPID);

	CEventManager::instance().TagTeam_RemovePID(dwPID);
}

void CEventTagTeam::ExitAttender(DWORD dwPID)
{
	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (pkChr)
	{
		long lMapIndex = pkChr->GetMapIndex();
		if (lMapIndex >= 10000)
			lMapIndex /= 10000;
		long lWarpMapIndex = pkChr->GetWarpMapIndex();
		if (lWarpMapIndex >= 10000)
			lWarpMapIndex /= 10000;
		if (lMapIndex == lWarpMapIndex /* || !pkChr->ExitToSavedLocation() */)
			pkChr->GoHome();
	}

	RemoveAttender(dwPID);
}

CEventTagTeam::TPlayerInfo* CEventTagTeam::GetAttenderInfo(DWORD dwPID)
{
	TAttenderSet::iterator it = m_set_Attender.find(dwPID);
	if (it == m_set_Attender.end())
		return NULL;
	return const_cast<TPlayerInfo*>(&(*it));
}

/*******************************************************************\
| [PRIVATE] Compute Functions
\*******************************************************************/
void CEventTagTeam::CheckMatch()
{
	if (m_bIsOver)
	{
		sys_err("already game over");
		return;
	}

	// check if there is only one team left
	int iLastTeamIndex = -1;
	for (TAttenderSet::iterator it = m_set_Attender.begin(); it != m_set_Attender.end(); ++it)
	{
		if (it->pkExitEvent)
			continue;

		if (iLastTeamIndex == -1)
			iLastTeamIndex = it->bTeamIndex;
		else if (iLastTeamIndex != it->bTeamIndex)
			return;
	}

	if (iLastTeamIndex == -1)
	{
		sys_err("invalid team index -1 !!");
		return;
	}

	// set over
	m_bIsOver = true;

	TTagTeamName& rkTagTeamName = m_map_TeamName[iLastTeamIndex];
	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "The tag team of [%s] and [%s] won the tag team battle!", rkTagTeamName.szName1, rkTagTeamName.szName2);
	SendNoticeMap(szBuf, m_dwMapIndex, true);
	BroadcastSuccessNotice(szBuf);

	// give rewards
	GiveReward(iLastTeamIndex);

	// start exit event for every player
	for (TAttenderSet::iterator it = m_set_Attender.begin(); it != m_set_Attender.end(); ++it)
	{
		if (it->pkExitEvent)
			continue;

		TPlayerInfo* pkPlayerInfo = const_cast<TPlayerInfo*>(&(*it));

		attender_event_info* info = AllocEventInfo<attender_event_info>();
		info->pkEventTagTeam = this;
		info->dwAttenderPID = it->dwPID;
		pkPlayerInfo->pkExitEvent = event_create(tag_team_player_exit_event, info, PASSES_PER_SEC(ATTENDER_EXIT_TIME));
	}

	// start destroy event
	tag_team_event_info* info = AllocEventInfo<tag_team_event_info>();
	info->pkEventTagTeam = this;
	m_pkDestroyEvent = event_create(tag_team_destroy_event, info, PASSES_PER_SEC(DESTROY_EVENT_TIME));
}

void CEventTagTeam::GiveReward(int iTeamIndex)
{
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT vnum,count,chance FROM event_reward WHERE event_index = %u", EVENT_TAG_TEAM_BATTLE));
	int iRewardCount = pMsg->Get()->uiNumRows;

	// no rewards
	if (iRewardCount == 0)
	{
		sys_log(0, "CEventTagTeam::GiveReward: no rewards");
		return;
	}

	// get reward list
	std::vector<TRewardInfo> vecRewardList;
	vecRewardList.reserve(iRewardCount);

	int i = 0;
	DWORD dwMaxChance = 0;

	while (auto row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		int col = 0;

		TRewardInfo& rkInfo = vecRewardList[i++];
		str_to_number(rkInfo.dwItemVnum, row[col++]);
		str_to_number(rkInfo.bItemCount, row[col++]);
		str_to_number(rkInfo.dwChance, row[col++]);

		if (!rkInfo.dwChance)
		{
			sys_err("invalid item %u chance 0 !!", rkInfo.dwItemVnum);
			return;
		}

		dwMaxChance += rkInfo.dwChance;
	}

	// give rewards to player
	for (TAttenderSet::iterator it = m_set_Attender.begin(); it != m_set_Attender.end(); ++it)
	{
		if (it->bTeamIndex == iTeamIndex && !it->pkExitEvent && !it->pkTimeoutEvent)
		{
			// check character
			LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(it->dwPID);
			if (!pkChr)
			{
				sys_err("cannot find player by pid %u for reward", it->dwPID);
				continue;
			}

			// generate random reward
			DWORD dwRandomNumber = random_number(1, dwMaxChance);

			DWORD dwCurChance = 0;
			TRewardInfo* pkRewardInfo = NULL;
			for (int i = 0; i < iRewardCount; ++i)
			{
				dwCurChance += vecRewardList[i].dwChance;

				if (dwCurChance >= dwRandomNumber)
				{
					pkRewardInfo = &vecRewardList[i];
					break;
				}
			}

			if (!pkRewardInfo)
			{
				sys_err("invalid rewards !! no reward info for pid %u !!", it->dwPID);
				continue;
			}

			// give reward
			LPITEM pkItem = pkChr->AutoGiveItem(pkRewardInfo->dwItemVnum, pkRewardInfo->bItemCount, -1, false);
			if (pkItem)
			{
				sys_log(0, "CEventTagTeam::GiveReward: give reward %u (%ux) [id: %u] to player %u %s",
					pkRewardInfo->dwItemVnum, pkRewardInfo->bItemCount, pkItem->GetID(), pkChr->GetPlayerID(), pkChr->GetName());
				pkChr->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkChr, "You received %ux %s as a reward."), pkRewardInfo->bItemCount, pkItem->GetName(pkChr->GetLanguageID()));
			}
			else
			{
				sys_err("cannot give reward %u (%ux) to player %u %s",
					pkRewardInfo->dwItemVnum, pkRewardInfo->bItemCount, pkChr->GetPlayerID(), pkChr->GetName());
				pkChr->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkChr, "An error occured while giving the reward. Please contact a teamler."));
			}
		}
	}
}
#endif
