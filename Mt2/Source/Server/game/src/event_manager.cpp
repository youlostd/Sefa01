#include "stdafx.h"

#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#include "text_file_loader.h"
#include "char_manager.h"
#include "char.h"
#include "desc.h"
#include "p2p.h"
#include "sectree_manager.h"
#include "map_location.h"
#include "config.h"
#include "start_position.h"
#include "item.h"
#include "unique_item.h"
#include "../../common/VnumHelper.h"
#include "questmanager.h"
#include "cmd.h"
#include "buffer_manager.h"

#ifdef HALLOWEEN_MINIGAME
#include "item_manager.h"
#include "db.h"
#include <algorithm>
#include <random>
#include "log.h"
#endif

#ifdef ENABLE_REACT_EVENT
#include "dungeon.h"
#include "cmd.h"
#include "utils.h"
#endif

template <typename T>
struct FSendPacket
{
	network::GCOutputPacket<T>& packet;
	
	FSendPacket(network::GCOutputPacket<T>& packet) : packet(packet) {}

	void operator()(LPENTITY ent)
	{
		LPCHARACTER pkChr = (LPCHARACTER)ent;
		if (pkChr->GetDesc()->IsPhase(PHASE_GAME) || pkChr->GetDesc()->IsPhase(PHASE_DEAD))
			pkChr->GetDesc()->Packet(packet);
	}
};

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CEventManager - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/
CEventManager::CEventManager()
{
	m_dwRunningEventIndex = EVENT_NONE;
	m_bIsAnnouncementRunning = false;
#ifdef HALLOWEEN_MINIGAME
	m_bHalloweenLoaded = false;
#endif
}

CEventManager::~CEventManager()
{
}

bool CEventManager::Initialize(const char* c_pszEventDataFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszEventDataFileName))
		return false;

	DWORD dwCount = loader.GetChildNodeCount();
	sys_log(0, "CEventManager::Initialize: load %u event datas", dwCount);

	for (DWORD i = 0; i < dwCount; ++i)
	{
		std::string stName("");
		loader.GetCurrentNodeName(&stName);

		loader.SetChildNode(i);

		TEventData kEventData;
		PIXEL_POSITION kBasePos;

		kEventData.bLoaded = true;

		if (!loader.GetTokenDoubleWord("index", &kEventData.dwIndex))
		{
			sys_err("Syntax error %s : no index, node %d %s", c_pszEventDataFileName, i, stName.c_str());
			loader.SetParentNode();
			return false;
		}
		char event_name[100];
		if (!loader.GetTokenString("name", event_name, sizeof(event_name)))
		{
			sys_err("Syntax error %s : no name, node %d %s", c_pszEventDataFileName, i, stName.c_str());
			loader.SetParentNode();
			return false;
		}
		kEventData.name = event_name;
		
		loader.GetTokenByte("levellimit", &kEventData.bLevelLimit);

		// map warp
		if ((!loader.GetTokenDoubleWord("mapindex", &kEventData.dwMapIndex) ||
			!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(kEventData.dwMapIndex, kBasePos)) &&
			(!loader.GetTokenBoolean("isnotice", &kEventData.bIsNotice) || !kEventData.bIsNotice))
		{
			sys_err("Syntax error %s : invalid mapindex %u, node %d %s", c_pszEventDataFileName, kEventData.dwMapIndex, i, stName.c_str());
			loader.SetParentNode();
			return false;
		}
		if (loader.GetTokenDoubleWord("x", &kEventData.dwX) && loader.GetTokenDoubleWord("y", &kEventData.dwY) && loader.GetTokenDoubleWord("x2", &kEventData.dwX2) && loader.GetTokenDoubleWord("y2", &kEventData.dwY2))
		{
			kEventData.dwX = kBasePos.x + kEventData.dwX * 100;
			kEventData.dwY = kBasePos.y + kEventData.dwY * 100;
			kEventData.dwX2 = kBasePos.x + kEventData.dwX2 * 100;
			kEventData.dwY2 = kBasePos.y + kEventData.dwY2 * 100;
		}
		else if (loader.GetTokenDoubleWord("x", &kEventData.dwX) && loader.GetTokenDoubleWord("y", &kEventData.dwY))
		{
			kEventData.dwX = kBasePos.x + kEventData.dwX * 100;
			kEventData.dwY = kBasePos.y + kEventData.dwY * 100;
		}
		// notice
		else if (!kEventData.bIsNotice)
		{
			PIXEL_POSITION kSpawnPos;
		//	BYTE bEmpire = MAX(1, SECTREE_MANAGER::instance().GetEmpireFromMapIndex(kEventData.dwMapIndex));
			if (!SECTREE_MANAGER::instance().GetSpawnPositionByMapIndex(kEventData.dwMapIndex, kSpawnPos))
			{
				sys_err("cannot get spawn position by map index %u, node %d %s", kEventData.dwMapIndex, i, stName.c_str());
				loader.SetParentNode();
				return false;
			}

			kEventData.bSpawnRecall = true;
		}
		// desc
		char event_desc[512];
		if (!loader.GetTokenString("description", event_desc, sizeof(event_desc)))
		{
			sys_err("Syntax error %s : no description, node %d %s", c_pszEventDataFileName, i, stName.c_str());
			loader.SetParentNode();
			return false;
		}
		kEventData.description = event_desc;

		if (kEventData.dwIndex == EVENT_NONE || kEventData.dwIndex >= EVENT_MAX_NUM)
		{
			sys_err("Invalid Index %u max[%d] (node %d %s)", kEventData.dwIndex, EVENT_MAX_NUM, i, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		m_akEventData[kEventData.dwIndex] = kEventData;
		loader.SetParentNode();

		sys_log(0, "CEventManager::Initialize: load event data %u map_index %u x %u (%u ~ %u) y %u (%u ~ %u)",
			kEventData.dwIndex, kEventData.dwMapIndex, kEventData.dwX, kEventData.dwX, kEventData.dwX2, kEventData.dwY, kEventData.dwY, kEventData.dwY2);
	}
	
	return true;
}

void CEventManager::Shutdown()
{
	TagTeam_Shutdown();

	P2P_CloseEventRegistration(true);
}

/*******************************************************************\
| [PRIVATE] Data Functions
\*******************************************************************/
const CEventManager::TEventData* CEventManager::GetEventData(DWORD dwIndex) const
{
	if (dwIndex == EVENT_NONE || dwIndex >= EVENT_MAX_NUM)
		return NULL;

	const TEventData* pkEventData = &m_akEventData[dwIndex];
	if (!pkEventData->bLoaded)
	{
		sys_err("event date %u is not loaded", dwIndex);
		return NULL;
	}

	return pkEventData;
}

/*******************************************************************\
| [PUBLIC] Main Functions
\*******************************************************************/
void CEventManager::OpenEventRegistration(DWORD dwEventIndex)
{
	if (!dwEventIndex)
		dwEventIndex = GetRunningEventIndex();

	const TEventData* pkEventData = GetEventData(dwEventIndex);
	if (!pkEventData)
	{
		sys_err("cannot open event registration for event %u (no event data)", dwEventIndex);
		return;
	}

	P2P_OpenEventRegistration(dwEventIndex);

	// send p2p packet
	network::GGOutputPacket<network::GGEventManagerOpenRegistrationPacket> p2p_packet;
	p2p_packet->set_event_index(dwEventIndex);
	P2P_MANAGER::instance().Send(p2p_packet);
}

void CEventManager::P2P_OpenEventRegistration(DWORD dwEventIndex)
{
	sys_log(0, "P2P_OpenEventRegistration %u", dwEventIndex);

	SetRunningEventIndex(dwEventIndex);

	// remove lists
	m_set_WFRPlayer.clear();
	m_set_IgnorePlayer.clear();

	// send notice to all
	if (GetRunningEventData()->bIsNotice)
	{
		char szBuf[256];
		snprintf(szBuf, sizeof(szBuf), "The event [LC_TEXT=%s] has started.", GetRunningEventData()->name.c_str());
		SendNotice(szBuf);

		const char* szDescription = GetRunningEventData()->description.c_str();
		if (*szDescription)
		{
			std::vector<std::string> vec_stNotices;
			split_string(szDescription, ';', vec_stNotices);
			for (int i = 0; i < vec_stNotices.size(); ++i)
				SendNotice(vec_stNotices[i].c_str());
		}
	}
	// send packet to all players
	else
	{
		network::GCOutputPacket<network::GCEventRequestPacket> packet;

		const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
		for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
		{
			LPCHARACTER pkChr = it->second;
			if (!pkChr->GetDesc())
				continue;

			if (!CanSendRequest(pkChr))
				continue;

			LPDESC pkDesc = pkChr->GetDesc();
			if (pkDesc->IsPhase(PHASE_GAME) || pkDesc->IsPhase(PHASE_DEAD))
			{
				m_set_WFRPlayer.insert(pkChr);
				EncodeEventRequestPacket(packet, pkChr);
				pkDesc->Packet(packet);
			}
		}

		// EMPIREWAR
		if (dwEventIndex == EVENT_EMPIREWAR)
		{
			network::GCOutputPacket<network::GCEventEmpireWarLoadPacket> packet;
			EmpireWar_EncodeLoadPacket(packet, true);

			FSendPacket<network::GCEventEmpireWarLoadPacket> f(packet);
			SECTREE_MANAGER::instance().for_each_pc(f);
		}
		// END_OF_EMPIREWAR
	}
}

void CEventManager::CloseEventRegistration(bool bClearEventIndex)
{
	if (GetRunningEventData() == NULL)
	{
		sys_err("no event running");
		return;
	}

	P2P_CloseEventRegistration(bClearEventIndex);

	// send p2p packet
	network::GGOutputPacket<network::GGEventManagerCloseRegistrationPacket> p2p_packet;
	p2p_packet->set_clear_event_index(bClearEventIndex);
	P2P_MANAGER::instance().Send(p2p_packet);
}

void CEventManager::P2P_CloseEventRegistration(bool bClearEventIndex)
{
	if (!GetRunningEventData())
	{
		sys_err("no running event data !!!");
		return;
	}

	sys_log(0, "P2P_CloseEventRegistration %d", GetRunningEventIndex());

	// EMPIREWAR
	if (GetRunningEventIndex() == EVENT_EMPIREWAR)
		EmpireWar_SendFinishPacket();
	// END_OF_EMPIREWAR

	// send notice to all
	if (GetRunningEventData()->bIsNotice)
	{
		char szBuf[256];
		snprintf(szBuf, sizeof(szBuf), "The event [LC_TEXT=%s] has ended.", GetRunningEventData()->name.c_str());
		SendNotice(szBuf);
	}
	// send packet to all wait-for-reply players
	else
	{
		network::GCOutputPacket<network::GCEventCancelPacket> packet;
		EncodeEventCancelPacket(packet);

		for (itertype(m_set_WFRPlayer) it = m_set_WFRPlayer.begin(); it != m_set_WFRPlayer.end(); ++it)
			(*it)->GetDesc()->Packet(packet);

		// send letter
/*		int iQuestIndex = quest::CQuestManager::instance().GetQuestIndexByName("event_manager");
		network::GCOutputPacket<network::GCQuestDeletePacket> quest_packet;
		quest_packet->set_index(iQuestIndex);
		for (itertype(m_set_IgnorePlayer) it = m_set_IgnorePlayer.begin(); it != m_set_IgnorePlayer.end(); ++it)
		{
			if (LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(*it))
				pkChr->GetDesc()->Packet(quest_packet);
		}*/
	}

	// remove lists
	m_set_WFRPlayer.clear();
	m_set_IgnorePlayer.clear();

	if (bClearEventIndex)
		SetRunningEventIndex(EVENT_NONE);
}

void CEventManager::OpenEventAnnouncement(DWORD dwType, time_t tmStamp)
{
	// open for this core
	P2P_OpenEventAnnouncement(dwType, tmStamp);

	// send p2p packet
	network::GGOutputPacket<network::GGEventManagerOpenAnnouncementPacket> p2p_packet;
	p2p_packet->set_type(dwType);
	p2p_packet->set_tm_stamp(tmStamp);
	P2P_MANAGER::instance().Send(p2p_packet);
}

void CEventManager::P2P_OpenEventAnnouncement(DWORD dwType, time_t tmStamp)
{
	m_bIsAnnouncementRunning = ( dwType != 0 );
	m_dwEventAnnouncementType = dwType;
	m_tmEventAnnouncementStamp = tmStamp;

	const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();

	for(itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
	{
		LPCHARACTER pkChr = it->second;

		if(!pkChr->GetDesc())
			continue;

		LPDESC pkDesc = pkChr->GetDesc();
		
		if(pkDesc->IsPhase(PHASE_GAME) || pkDesc->IsPhase(PHASE_DEAD))
		{
			time_t difference = m_tmEventAnnouncementStamp - time(0);
			pkChr->ChatPacket(CHAT_TYPE_COMMAND, "EventAnnouncement %d %d", dwType, difference);
		}
	}
}

void CEventManager::EventAnnouncement_OnPlayerLogin(LPCHARACTER pkChr)
{
	if(!m_bIsAnnouncementRunning)
		return;

	if(!m_dwEventAnnouncementType)
		return;

	time_t difference = m_tmEventAnnouncementStamp - time(0);

	if(difference <= 0)
		return;

	pkChr->ChatPacket(CHAT_TYPE_COMMAND, "EventAnnouncement %d %d", m_dwEventAnnouncementType, difference);
}

void CEventManager::OnEventOver()
{
	if (GetRunningEventData() == NULL)
	{
		sys_err("no event running");
		return;
	}

	P2P_OnEventOver();

	// send p2p packet
	P2P_MANAGER::instance().Send(network::TGGHeader::EVENT_MANAGER_OVER);
}

void CEventManager::P2P_OnEventOver()
{
	SetRunningEventIndex(EVENT_NONE);
}

bool CEventManager::IsIgnorePlayer(DWORD dwPID)
{
	return m_set_IgnorePlayer.find(dwPID) != m_set_IgnorePlayer.end();
}

void CEventManager::P2P_IgnorePlayer(DWORD dwPID)
{
	m_set_IgnorePlayer.insert(dwPID);
}

bool CEventManager::CanSendRequest(LPCHARACTER pkChr)
{
	if (m_set_WFRPlayer.find(pkChr) != m_set_WFRPlayer.end())
		return false;

	if (pkChr->GetLevel() < GetRunningEventData()->bLevelLimit)
		return false;
	if (GetRunningEventData()->dwMapIndex == pkChr->GetMapIndex())
		return false;

	return true;
}

void CEventManager::SendRequest(LPCHARACTER pkChr)
{
	network::GCOutputPacket<network::GCEventRequestPacket> packet;
	EncodeEventRequestPacket(packet, pkChr);

	m_set_IgnorePlayer.erase(pkChr->GetPlayerID());
	m_set_WFRPlayer.insert(pkChr);
	pkChr->GetDesc()->Packet(packet);
}

/*******************************************************************\
| [PUBLIC] Trigger Functions
\*******************************************************************/
void CEventManager::OnPlayerLogin(LPCHARACTER pkChr)
{
	EventAnnouncement_OnPlayerLogin(pkChr);

	long lMapIndex = pkChr->GetMapIndex();
	if (lMapIndex >= 10000)
		lMapIndex /= 10000;

	if (GetRunningEventIndex() == EVENT_EMPIREWAR)
		EmpireWar_OnPlayerLogin(pkChr);

	if (lMapIndex == EVENT_LABYRINTH_MAP_INDEX)
	{
		TagTeam_OnPlayerLogin(pkChr);
	}
	else if (lMapIndex == PVP_TOURNAMENT_MAP_INDEX || lMapIndex == OXEVENT_MAP_INDEX || lMapIndex == EVENT_LABYRINTH_MAP_INDEX)
	{
		pkChr->RemoveGoodAffect();
	}
#ifdef ENABLE_REACT_EVENT
	else if (lMapIndex == REACT_EVENT_MAP)
	{
		React_OnPlayerLogin(pkChr);
	}
#endif
	else
	{
		const TEventData* pkEventData = GetRunningEventData();
		if (!pkEventData)
			return;

		if (!CanSendRequest(pkChr))
			return;

		if (lMapIndex == pkEventData->dwMapIndex)
			return;

		if (pkEventData->bIsNotice)
		{
			char szBuf[256];
			snprintf(szBuf, sizeof(szBuf), LC_TEXT(pkChr, "The event %s is running!"), LC_TEXT(pkChr, GetRunningEventData()->name.c_str()));
			pkChr->ChatPacket(CHAT_TYPE_NOTICE, szBuf);

			const char* szDescription = GetRunningEventData()->description.c_str();
			if (*szDescription)
			{
				std::vector<std::string> vec_stNotices;
				split_string(szDescription, ';', vec_stNotices);
				for (int i = 0; i < vec_stNotices.size(); ++i)
					pkChr->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(pkChr, vec_stNotices[i].c_str()));
			}
		}
		else
		{
			if (IsIgnorePlayer(pkChr->GetPlayerID()))
				return;

			// send packet
			network::GCOutputPacket<network::GCEventRequestPacket> packet;
			EncodeEventRequestPacket(packet, pkChr);

			m_set_WFRPlayer.insert(pkChr);
			pkChr->GetDesc()->Packet(packet);
		}
	}
}

void CEventManager::OnPlayerLogout(LPCHARACTER pkChr)
{
	TagTeam_OnPlayerLogout(pkChr);

#ifdef ENABLE_REACT_EVENT
	React_OnPlayerLogout(pkChr);
#endif

	if (!GetRunningEventData())
		return;

	itertype(m_set_WFRPlayer) it = m_set_WFRPlayer.find(pkChr);
	if (it == m_set_WFRPlayer.end())
		return;

	m_set_WFRPlayer.erase(it);
}

void CEventManager::OnPlayerLoadAffect(LPCHARACTER pkChr)
{
	TagTeam_OnPlayerLoadAffect(pkChr);
}

void CEventManager::OnPlayerDead(LPCHARACTER pkChr)
{
	TagTeam_OnPlayerDead(pkChr);
}

void CEventManager::OnPlayerAnswer(LPCHARACTER pkChr, bool bAccept)
{
	itertype(m_set_WFRPlayer) it = m_set_WFRPlayer.find(pkChr);
	if (it == m_set_WFRPlayer.end())
		return;

	m_set_WFRPlayer.erase(it);

	// decline request
	if (bAccept == false)
	{
		P2P_IgnorePlayer(pkChr->GetPlayerID());

		// send letter
		int iQuestIndex = quest::CQuestManager::instance().GetQuestIndexByName("event_manager");
		int iStateIndex = quest::CQuestManager::instance().GetQuestStateIndex("event_manager", "start");
		quest::CQuestManager::instance().Letter(pkChr->GetPlayerID(), iQuestIndex, iStateIndex);

		// send p2p packet
		network::GGOutputPacket<network::GGEventManagerIgnorePlayerPacket> p2p_packet;
		p2p_packet->set_pid(pkChr->GetPlayerID());
		P2P_MANAGER::instance().Send(p2p_packet);
	}
	// accept request
	else
	{
		const TEventData* pkEventData = GetRunningEventData();
		if (!pkEventData)
		{
			sys_err("cannot get event data (pid %u accept %d)", pkChr->GetPlayerID(), bAccept);
			return;
		}

		PIXEL_POSITION kSpawnPos;
		if (!pkEventData->bSpawnRecall)
		{
			if (pkEventData->dwX2 || pkEventData->dwY2)
			{
				kSpawnPos.x = random_number(pkEventData->dwX, pkEventData->dwX2);
				kSpawnPos.y = random_number(pkEventData->dwY, pkEventData->dwY2);
			}
			else
			{
				kSpawnPos.x = pkEventData->dwX;
				kSpawnPos.y = pkEventData->dwY;
			}
#ifdef __ANGELSDEMON_EVENT2__
			if (pkEventData->dwIndex == EVENT_ANGELDEAMON)
			{
				quest::PC* pPC = quest::CQuestManager::Instance().GetPCForce(pkChr->GetPlayerID());

				if (pPC && pPC->GetFlag("anniversary_event.selected_fraction") == FRACTION_DEMONS)
				{
					kSpawnPos.x = 81677;
					kSpawnPos.y = 253140;

					P2P_IgnorePlayer(pkChr->GetPlayerID());
					// send p2p packet
					network::GGOutputPacket<network::GGEventManagerIgnorePlayerPacket> p2p_packet;
					p2p_packet->set_pid(pkChr->GetPlayerID());
					P2P_MANAGER::instance().Send(p2p_packet);
				}
			}
#endif
		}
		else
		{
			if (!SECTREE_MANAGER::instance().GetRecallPositionByEmpire(pkEventData->dwMapIndex, pkChr->GetEmpire(), kSpawnPos))
			{
				sys_err("cannot get spawn position by map index %u", pkEventData->dwMapIndex);
				return;
			}
		}

		pkChr->WarpSet(kSpawnPos.x, kSpawnPos.y);
	}
}

/*******************************************************************\
| [PUBLIC] Check Functions
\*******************************************************************/
bool CEventManager::CanUseItem(LPCHARACTER pkChr, LPITEM pkItem)
{
	long lMapIndex = pkChr->GetMapIndex();
	if (lMapIndex >= 10000)
		lMapIndex /= 10000;

	// if (GetRunningEventIndex() == EVENT_TAG_TEAM_BATTLE)
	// {
	if (lMapIndex == EVENT_LABYRINTH_MAP_INDEX)
	{
		itertype(m_map_EventTagTeamByPID) it = m_map_EventTagTeamByPID.find(pkChr->GetPlayerID());
		if (it != m_map_EventTagTeamByPID.end())
			return TagTeam_CanUseItem(it->second, pkItem);
	}
	// }
	// else if (GetRunningEventIndex() == EVENT_PVP_TOURNAMENT)
	// {
	else if (lMapIndex == PVP_TOURNAMENT_MAP_INDEX)
	{
		return PVPTournament_CanUseItem(pkChr, pkItem);
	}
	// }
	// else if (GetRunningEventIndex() == EVENT_EMPIREWAR)
	// {
	else if (lMapIndex == EMPIREWAR_MAP_INDEX)
	{
		pkChr->tchat("CanuseItem %d", EmpireWar_CanUseItem(pkItem));
		return EmpireWar_CanUseItem(pkItem);
	}
	// }

	return true;
}

bool CEventManager::PVPTournament_CanUseItem(LPCHARACTER pkChr, LPITEM pkItem)
{
	if (pkItem->IsPotionItem(true))
		return false;
	if (pkItem->IsAutoPotionItem(true))
		return false;

	if (pkItem->GetType() == ITEM_USE && pkItem->GetSubType() == USE_INVISIBILITY)
		return false;

	return true;
}

/*******************************************************************\
| [PRIVATE] Packet Functions
\*******************************************************************/
void CEventManager::EncodeEventRequestPacket(network::GCOutputPacket<network::GCEventRequestPacket>& rkPacket, LPCHARACTER pkChr)
{
	BYTE bLangID = pkChr->GetLanguageID();
	const TEventData* pkEventData = GetRunningEventData();
	if (!pkEventData)
	{
		sys_err("cannot encode request event packet while no event is running");
		return;
	}

	rkPacket->set_event_index(GetRunningEventIndex());
	rkPacket->set_name(LC_TEXT(bLangID, pkEventData->name.c_str()));
	rkPacket->set_desc(LC_TEXT(bLangID, pkEventData->description.c_str()));
}

void CEventManager::EncodeEventCancelPacket(network::GCOutputPacket<network::GCEventCancelPacket>& rkPacket)
{
	const TEventData* pkEventData = GetRunningEventData();
	if (!pkEventData)
	{
		sys_err("cannot encode cancel event packet while no event is running");
		return;
	}

	rkPacket->set_event_index(GetRunningEventIndex());
}

/*******************************************************************\
| [PUBLIC] Event: EmpireWar Functions
\*******************************************************************/
void CEventManager::EmpireWar_EncodeLoadPacket(network::GCOutputPacket<network::GCEventEmpireWarLoadPacket>& rkPacket, bool bIsNowStart)
{
	if (quest::CQuestManager::instance().GetEventFlag("empirewar_goal_type") == 3)
	{
		int iTimeSec = quest::CQuestManager::instance().GetEventFlag("empirewar_goal_value") * 60;
		if (!bIsNowStart)
		{
			int iStartTime = quest::CQuestManager::instance().GetEventFlag("empirewar_starttime");
			rkPacket->set_time_left(iTimeSec - (get_global_time() - iStartTime));
		}
		else
		{
			rkPacket->set_time_left(iTimeSec);
		}
	}

	for (int i = 0; i < EMPIRE_MAX_NUM - 1; ++i)
	{
		rkPacket->add_kills(0);
		rkPacket->add_deaths(0);
	}

	if (!bIsNowStart)
	{
		char szFlagName[30 + 1];
		for (int i = 0; i < EMPIRE_MAX_NUM-1; ++i)
		{
			snprintf(szFlagName, sizeof(szFlagName), "empirewar_kills_%d", i + 1);
			rkPacket->set_kills(i, quest::CQuestManager::instance().GetEventFlag(szFlagName));
			snprintf(szFlagName, sizeof(szFlagName), "empirewar_deads_%d", i + 1);
			rkPacket->set_deaths(i, quest::CQuestManager::instance().GetEventFlag(szFlagName));
		}
	}
}

void CEventManager::EmpireWar_SendUpdatePacket(BYTE bEmpire, WORD wKills, WORD wDeads)
{
	network::GCOutputPacket<network::GCEventEmpireWarUpdatePacket> packet;
	packet->set_empire(bEmpire);
	packet->set_kills(wKills);
	packet->set_deaths(wDeads);

	FSendPacket<network::GCEventEmpireWarUpdatePacket> f(packet);
	SECTREE_MANAGER::instance().for_each_pc(f);

	// send p2p
	network::GGOutputPacket<network::GGPlayerPacket> p2p_packet;
	p2p_packet->set_relay_header(packet.get_header());
	std::vector<uint8_t> buf;
	buf.resize(packet->ByteSize());
	packet->SerializeToArray(&buf[0], buf.size());
	p2p_packet->set_relay(&buf[0], buf.size());
	P2P_MANAGER::instance().Send(p2p_packet);
}

void CEventManager::EmpireWar_SendFinishPacket()
{
	SECTREE_MANAGER::instance().for_each_pc([](LPENTITY ent) {
		auto ch = (LPCHARACTER) ent;
		ch->GetDesc()->Packet(network::TGCHeader::EVENT_EMPIRE_WAR_FINISH);
	});
}

void CEventManager::EmpireWar_OnPlayerLogin(LPCHARACTER pkChr)
{
	if (GetRunningEventIndex() != EVENT_EMPIREWAR)
	{
		if (!pkChr->IsGM())
			pkChr->GoHome();

		return;
	}

	network::GCOutputPacket<network::GCEventEmpireWarLoadPacket> packet;
	EmpireWar_EncodeLoadPacket(packet);
	pkChr->GetDesc()->Packet(packet);

	if (pkChr->GetMapIndex() == EMPIREWAR_MAP_INDEX)
	{
		pkChr->SetPVPTeam(pkChr->GetEmpire());

		if (pkChr->GetGMLevel(true) == GM_PLAYER)
		{
			BYTE bEmpire = pkChr->GetEmpire();
			pkChr->SetWarpLocation(EMPIRE_START_MAP(bEmpire), EMPIRE_BASE_X(bEmpire), EMPIRE_BASE_Y(bEmpire));
		}
	}
}

bool CEventManager::EmpireWar_CanUseItem(LPITEM pkItem)
{
	// if (pkItem->IsPotionItem(true))
	// 	return false;
	// if (pkItem->IsAutoPotionItem(true))
	// 	return false;

	if (pkItem->GetType() == ITEM_USE && pkItem->GetSubType() == USE_INVISIBILITY)
		return false;

 #ifdef __FAKE_BUFF__
	if (CItemVnumHelper::IsFakeBuffSpawn(pkItem->GetVnum()))
		return false;
#endif

	return true;
}


/*******************************************************************\
| [PUBLIC] Event: Tag Team Functions
\*******************************************************************/
void CEventManager::TagTeam_Register(LPCHARACTER pkChr1, LPCHARACTER pkChr2)
{
	if (TagTeam_IsRegistered(pkChr1))
	{
		sys_err("cannot register %u %s with %u %s - player 1 is already registered",
			pkChr1->GetPlayerID(), pkChr1->GetName(), pkChr2->GetPlayerID(), pkChr2->GetName());
		return;
	}

	if (TagTeam_IsRegistered(pkChr2))
	{
		sys_err("cannot register %u %s with %u %s - player 2 is already registered",
			pkChr1->GetPlayerID(), pkChr1->GetName(), pkChr2->GetPlayerID(), pkChr2->GetName());
		return;
	}

	int iMaxLvl = pkChr1->GetLevel() >= pkChr2->GetLevel() ? pkChr1->GetLevel() : pkChr2->GetLevel();
	BYTE bGroupIdx = iMaxLvl >= 90 ? 1 : 0;

	P2P_TagTeam_Register(pkChr1->GetPlayerID(), pkChr2->GetPlayerID(), bGroupIdx);

	// send p2p packet
	network::GGOutputPacket<network::GGEventManagerTagTeamRegisterPacket> p2p_packet;
	p2p_packet->set_pid1(pkChr1->GetPlayerID());
	p2p_packet->set_pid2(pkChr2->GetPlayerID());
	p2p_packet->set_groupidx(bGroupIdx);
	P2P_MANAGER::instance().Send(p2p_packet);
}

void CEventManager::P2P_TagTeam_Register(DWORD dwPID1, DWORD dwPID2, BYTE bGroupIdx)
{
	m_map_TagTeamRegistrations[bGroupIdx][dwPID1] = dwPID2;
	m_map_TagTeamRegistrations[bGroupIdx][dwPID2] = dwPID1;
}

void CEventManager::TagTeam_Unregister(LPCHARACTER pkChr)
{
	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "[%s] has left your tag team.", pkChr->GetName());
	if (TagTeam_RemoveRegistration(pkChr, szBuf))
		pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChr, "You have left your tag team."));
}

bool CEventManager::TagTeam_IsRegistered(LPCHARACTER pkChr)
{
	return m_map_TagTeamRegistrations[0].find(pkChr->GetPlayerID()) != m_map_TagTeamRegistrations[0].end() ||
		m_map_TagTeamRegistrations[1].find(pkChr->GetPlayerID()) != m_map_TagTeamRegistrations[1].end();
}

bool CEventManager::TagTeam_RemoveRegistration(LPCHARACTER pkChr, const char* szMessage)
{
	for (int i = 0; i < 2; ++i)
	{
		itertype(m_map_TagTeamRegistrations[i]) it = m_map_TagTeamRegistrations[i].find(pkChr->GetPlayerID());
		if (it == m_map_TagTeamRegistrations[i].end())
			continue;

		LPCHARACTER pkOther = CHARACTER_MANAGER::instance().FindByPID(it->second);

		// send p2p packet
		network::GGOutputPacket<network::GGEventManagerTagTeamUnregisterPacket> p2p_packet;
		p2p_packet->set_pid1(it->first);
		p2p_packet->set_pid2(it->second);
		p2p_packet->set_groupidx(i);
		P2P_MANAGER::instance().Send(p2p_packet);

		P2P_TagTeam_RemoveRegistration(it->first, it->second, i);

		if (pkOther && szMessage)
			pkOther->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkOther, szMessage));

		return true;
	}

	return false;
}

void CEventManager::P2P_TagTeam_RemoveRegistration(DWORD dwPID1, DWORD dwPID2, BYTE bGroupIdx)
{
	m_map_TagTeamRegistrations[bGroupIdx].erase(dwPID1);
	m_map_TagTeamRegistrations[bGroupIdx].erase(dwPID2);
}

void CEventManager::TagTeam_Create(const ::google::protobuf::RepeatedPtrField<TTagTeam>& teams)
{
	if (teams.size() != (test_server ? 3 : CEventTagTeam::MAX_TEAM_COUNT))
		return;

	// remove teams from registration
	for (auto& team : teams)
	{
		for (int iGroupIdx = 0; iGroupIdx < 2; ++iGroupIdx)
		{
			itertype(m_map_TagTeamRegistrations[iGroupIdx]) it = m_map_TagTeamRegistrations[iGroupIdx].find(team.pid1());
			if (it != m_map_TagTeamRegistrations[iGroupIdx].end())
			{
				m_map_TagTeamRegistrations[iGroupIdx].erase(it->second);
				m_map_TagTeamRegistrations[iGroupIdx].erase(it);
				break;
			}
		}
	}

	// wrong core
	if (!map_allow_find(EVENT_LABYRINTH_MAP_INDEX))
	{
		network::GGOutputPacket<network::GGEventManagerTagTeamCreatePacket> p2p_packet;
		*p2p_packet->mutable_teams() = teams;

		long lAddr;
		WORD wPort;
		if (!CMapLocation::instance().Get(EVENT_LABYRINTH_MAP_INDEX, lAddr, wPort))
		{
			sys_err("cannot find core for map %u", EVENT_LABYRINTH_MAP_INDEX);
			return;
		}

		P2P_MANAGER::instance().Send(p2p_packet);
		return;
	}

	// create match
	CEventTagTeam* pkEventTagTeam = new CEventTagTeam();
	for (auto& team : teams)
	{
		if (team.pid1() == 0 || team.pid2() == 0)
			continue;

		pkEventTagTeam->JoinTagTeam(team.pid1(), team.name1().c_str(), team.pid2(), team.name2().c_str());

		m_map_EventTagTeamByPID[team.pid1()] = pkEventTagTeam;
		m_map_EventTagTeamByPID[team.pid2()] = pkEventTagTeam;
	}

	m_set_EventTagTeam.insert(pkEventTagTeam);
	quest::CQuestManager::instance().RequestSetEventFlag("tagteam_mapidx", pkEventTagTeam->GetMapIndex());
}

void CEventManager::TagTeam_Destroy(CEventTagTeam* pkEventTagTeam)
{
	if(quest::CQuestManager::instance().GetEventFlag("tagteam_mapidx") == pkEventTagTeam->GetMapIndex())
		quest::CQuestManager::instance().RequestSetEventFlag("tagteam_mapidx", 0);

	m_set_EventTagTeam.erase(pkEventTagTeam);

	pkEventTagTeam->Destroy();
}

void CEventManager::TagTeam_Shutdown()
{
	while (!m_set_EventTagTeam.empty())
		TagTeam_Destroy(*(m_set_EventTagTeam.begin()));
	quest::CQuestManager::instance().RequestSetEventFlag("tagteam_mapidx", 0);
}

void CEventManager::TagTeam_RemovePID(DWORD dwPID)
{
	m_map_EventTagTeamByPID.erase(dwPID);
}

void CEventManager::TagTeam_OnPlayerLogin(LPCHARACTER pkChr)
{
	if (!test_server && pkChr->IsGM())
		return;

	itertype(m_map_EventTagTeamByPID) it = m_map_EventTagTeamByPID.find(pkChr->GetPlayerID());
	if (it == m_map_EventTagTeamByPID.end())
	{
		if (quest::CQuestManager::instance().GetEventFlag("disable_tagteam_observe") == 0)
			pkChr->SetObserverMode(true);
		else
			pkChr->GoHome();

		return;
	}

	CEventTagTeam* pkEventTagTeam = it->second;
	pkEventTagTeam->OnPlayerOnline(pkChr);
}

void CEventManager::TagTeam_OnPlayerLogout(LPCHARACTER pkChr)
{
	TagTeam_RemoveRegistration(pkChr, "Your tag team has been removed due to a logout of your partner.");

	itertype(m_map_EventTagTeamByPID) it = m_map_EventTagTeamByPID.find(pkChr->GetPlayerID());
	if (it == m_map_EventTagTeamByPID.end())
		return;

	CEventTagTeam* pkEventTagTeam = it->second;
	pkEventTagTeam->OnPlayerOffline(pkChr);
}

void CEventManager::TagTeam_OnPlayerLoadAffect(LPCHARACTER pkChr)
{
	itertype(m_map_EventTagTeamByPID) it = m_map_EventTagTeamByPID.find(pkChr->GetPlayerID());
	if (it == m_map_EventTagTeamByPID.end())
		return;

	CEventTagTeam* pkEventTagTeam = it->second;
	pkEventTagTeam->OnPlayerLoadAffect(pkChr);
}

void CEventManager::TagTeam_OnPlayerDead(LPCHARACTER pkChr)
{
	itertype(m_map_EventTagTeamByPID) it = m_map_EventTagTeamByPID.find(pkChr->GetPlayerID());
	if (it == m_map_EventTagTeamByPID.end())
		return;

	CEventTagTeam* pkEventTagTeam = it->second;
	pkEventTagTeam->OnPlayerDead(pkChr);
}

bool CEventManager::TagTeam_CanUseItem(CEventTagTeam* pkTagTeam, LPITEM pkItem)
{
	if (pkItem->IsPotionItem(true))
		return false;
	if (pkItem->IsAutoPotionItem(true))
		return false;

	if (pkItem->GetType() == ITEM_USE && pkItem->GetSubType() == USE_INVISIBILITY)
		return false;

	return true;
}
#endif

#ifdef ENABLE_REACT_EVENT
int CEventManager::React_Manager(BYTE idx)
{
	if (test_server)	BroadcastNotice(std::string(std::string("Manager:( )") + std::to_string(idx)).c_str());
	switch (idx)
	{
		case 0:
			SetRunningEventIndex(EVENT_REACT);
			break;
		case 1:
			SetRunningEventIndex(EVENT_NONE);
			break;
		case 2:
			return React_GetParticipants();
			break;

		case 10:
			React_SetTask();
			break;
		case 11:
			return React_ResolveTask();
			break;
		case 12:
			React_WarpLosers();
			break;

		case 20:
		{
			const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
			for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
			{
				LPCHARACTER pkChr = it->second;
				if (!pkChr->GetDesc())
					continue;

				if (pkChr->GetMapIndex() == REACT_EVENT_MAP && !pkChr->GetGMLevel())
					pkChr->GoHome();
			}
		}
		break;
	
		default:
			break;
	}

	return 0;
}

void CEventManager::React_OnPlayerLogin(LPCHARACTER ch)
{
	if (!ch->GetGMLevel() || test_server)
		m_map_EventReactMembers.emplace(ch, SReactMember());
}

void CEventManager::React_OnPlayerLogout(LPCHARACTER ch)
{
	if (!ch->GetGMLevel() || test_server)
		m_map_EventReactMembers.erase(ch);

	if (std::find(m_vec_EventReactLosers.begin(), m_vec_EventReactLosers.end(), ch) != m_vec_EventReactLosers.end())
		m_vec_EventReactLosers.erase(std::find(m_vec_EventReactLosers.begin(), m_vec_EventReactLosers.end(), ch));
}

void CEventManager::React_OnPlayerReact(LPCHARACTER ch, std::string answer)
{
	ch->tchat("Your reaction: %s", answer.c_str());

	if (answer != m_strAnswer)
	{
		m_vec_EventReactLosers.push_back(ch);
		m_map_EventReactMembers.erase(ch);
		return;
	}

	int participants = m_map_EventReactMembers.size();
	int maxReactions = (participants * 0.75) + 0.5;

	int reactions = 0;
	for (const auto &it : m_map_EventReactMembers)
	{
		if (it.second.reactTime)
			++reactions;
	}

	if (participants == 2)
		maxReactions = 1;

	if (reactions >= maxReactions)
	{
		m_vec_EventReactLosers.push_back(ch);
		m_map_EventReactMembers.erase(ch);
		return;
	}

	auto it = m_map_EventReactMembers.find(ch);
	it->second.reactTime = get_dword_time();
}

void CEventManager::React_WarpLosers()
{
	static PIXEL_POSITION basePos;
	SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(REACT_EVENT_MAP, basePos);
	static int warp_pos[4][2] = {
		{173, 56},
		{150, 168},
		{100, 209},
		{44, 113}
	};

	for (auto &it : m_vec_EventReactLosers)
	{
		if (!it)
			continue;

		BYTE random = random_number(0,3);
		it->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(it, "Wrong reaction or too slowly!"));
		it->Show(REACT_EVENT_MAP, basePos.x + warp_pos[random][0]*100, basePos.y + warp_pos[random][1]*100, 0);
		it->Stop();
		it->ChatPacket(CHAT_TYPE_COMMAND, "REACT_CLOSE");
	}
	m_vec_EventReactLosers.clear();
}

bool CEventManager::React_ResolveTask()
{
	if (test_server)	BroadcastNotice("React_ResolveTask( )");
	auto it = m_map_EventReactMembers.begin();

	for (; it != m_map_EventReactMembers.end(); )
	{
		it->first->ChatPacket(CHAT_TYPE_COMMAND, "REACT_CLEAR");
		if (!it->second.reactTime)
		{
			m_vec_EventReactLosers.push_back(it->first);
			m_map_EventReactMembers.erase(it++);
		}
		else
			++it;
	}

	if (m_map_EventReactMembers.size() <= 1)
	{
		for (auto &ch : m_map_EventReactMembers)
		{
			ch.first->AutoGiveItem(quest::CQuestManager::instance().GetEventFlag("event_react_vnum"), quest::CQuestManager::instance().GetEventFlag("event_react_count"));
			std::string win_msg = std::string(ch.first->GetName()) + " won the Reaction Event!";
			BroadcastNotice(win_msg.c_str());
		}

		React_Manager(1);
		if (test_server)	BroadcastNotice("ONLY <= 1 participants( )");
		return false;
	}

	if (!m_map_EventReactMembers.size())
	{
		BroadcastNotice("Nobody won the reaction event!");
		React_Manager(1);
		return false;
	}

	if (test_server)	BroadcastNotice("React_ResolveTask end( )");
	return true;
}

void CEventManager::React_SetTask()
{
	if (test_server)	BroadcastNotice("SetTask( )");
	BYTE task = random_number(REACT_TYPE_START+1, REACT_TYPE_END-1);

	std::string msg = "";
	std::string msg_answer = "";
	if (task == REACT_TYPE_EMOTION)
	{
		BYTE size = sizeof(emotion_types) / sizeof(emotion_type_s);
		BYTE random = random_number(3, size-1);

		m_strAnswer = emotion_types[random].command_to_client;
		msg = "Execute emotion: ";
		msg_answer = "EMOTION/" + m_strAnswer;
	}
	else if (task == REACT_TYPE_NUMBER)
	{
		msg = "Type the number ";
		m_strAnswer = std::to_string(random_number(1000, 9999));
		m_strAnswer = strReplaceAll(m_strAnswer, "0", std::to_string(random_number(1,9)));
	}
	else if (task == REACT_TYPE_SYMBOL)
	{
		BYTE random = random_number(0,18);
		static const int symbols[18][2] = {
			{1,0},
			{2,0},
			{3,0},
			{4,0},
			{5,0},
			{1,1},
			{2,2},
			{3,3},
			{4,4},
			{5,5},
			{1,2},
			{3,4},
			{5,4},
			{1,3},
			{1,5},
			{2,4},
			{5,3},
			{4,2}
		};

		msg = "Press the following Symbols: ";
		m_strAnswer = "Symbol" + std::to_string(symbols[random][0]);
		msg_answer = "|Ecircle_" + std::to_string(symbols[random][0]) + "_hover|e";

		if (symbols[random][1] > 0)
		{
			m_strAnswer += "Symbol" + std::to_string(symbols[random][1]);
			msg_answer +=  "|Ecircle_" + std::to_string(symbols[random][1]) + "_hover|e";
		}
	}

	if (msg_answer.empty())
		msg_answer = m_strAnswer;

	for (auto &it : m_map_EventReactMembers)
	{
		it.first->ChatPacket(CHAT_TYPE_COMMAND, "REACT_TIMER %d", 20);
		it.second.reactTime = 0;

		std::string send_msg = strReplaceAll(LC_TEXT(it.first, msg.c_str()), " ", "#");
		std::string send_answer = msg_answer;

		send_answer = strReplaceAll(send_answer, " ", "#");
		it.first->ChatPacket(CHAT_TYPE_COMMAND, "REACT_NOTICE %s%s", send_msg.c_str(), msg_answer.c_str());
	}

	if (test_server)	BroadcastNotice("SetTask!( )");
}
#endif

#ifdef HALLOWEEN_MINIGAME
void CEventManager::HalloweenMinigame_Initialize()
{
	m_bHalloweenLoaded = false;
}

void CEventManager::HalloweenMinigame_StartRound(LPCHARACTER ch)
{
	const DWORD HALLOWEEN_MINIGAME_ITEM = 95364;

	bool isGMItem = false;
	if (ch->CountSpecifyItem(HALLOWEEN_MINIGAME_ITEM))
	{
        LogManager::instance().CharLog(ch, ch->CountSpecifyItem(HALLOWEEN_MINIGAME_ITEM), "HALLOWEENGAME", "");
		isGMItem = ch->RemoveSpecifyItem(HALLOWEEN_MINIGAME_ITEM, 1);
	}
	else
	{
        auto pTable = ITEM_MANAGER::instance().GetTable(HALLOWEEN_MINIGAME_ITEM);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need %s."), pTable ? pTable->locale_name(ch ? ch->GetLanguageID() : LANGUAGE_DEFAULT).c_str() : "");
		return;
	}

	static std::vector<THalloweenRewardData> vecRewards;
	if (!m_bHalloweenLoaded || vecRewards.empty())
	{
		m_bHalloweenLoaded = true;
		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT vnum,count,chance,event_index FROM player.event_reward WHERE event_index BETWEEN %d AND %d", 20, 21));
		if (!pMsg->Get()->uiNumRows)
			return;

		while (auto row = mysql_fetch_row(pMsg->Get()->pSQLResult))
		{
			THalloweenRewardData data;
			data.vnum = atoi(row[0]);
			data.count = atoi(row[1]);
			data.chance = atoi(row[2]);
			data.type = atoi(row[3]) == 21 ? 1 : 0;
			vecRewards.push_back(data);
		}
	}

	WORD chances = 0;
	auto rng = std::default_random_engine {};
	std::shuffle(std::begin(vecRewards), std::end(vecRewards), rng);
	std::vector<THalloweenRewardData>	reward_pool;
	std::string cmd = "";

	for (auto &it : vecRewards)
	{
		if (it.type == 1)
		{
			reward_pool.push_back(it);
			chances += it.chance;
			cmd += " " + std::to_string(it.vnum) + "|" + std::to_string(it.count);
			break;
		}
	}

	for (auto &it : vecRewards)
	{
		if (it.type == 0)
		{
			chances += it.chance;
			reward_pool.push_back(it);
			cmd += "#" + std::to_string(it.vnum) + "|" + std::to_string(it.count);
			if (reward_pool.size() == 10)
				break;
		}
	}

	if (reward_pool.size() != 10)
	{
		ch->tchat("Pool != 10 (%d/%d)", reward_pool.size(), vecRewards.size());
		return;
	}

	WORD rand = random_number(0, chances);
	WORD counter = 0;
	BYTE index = 0;

	for (auto &spin_reward : reward_pool)
	{
		counter += spin_reward.chance;
		if (counter >= rand)
			break;
		index++;
	}

	LogManager::instance().CharLog(ch, ch->CountSpecifyItem(HALLOWEEN_MINIGAME_ITEM), "HALLOWEENGAME_REWARD", 
		std::string(std::to_string(reward_pool[index].vnum) + "|" + std::to_string(reward_pool[index].count) + "|" + std::to_string(reward_pool[index].type)).c_str());
	DBManager::instance().DirectQuery("INSERT INTO player.item_award (`pid`,`login`,`vnum`,`count`,`why`,`mall`, `no_trade`) VALUES (%d, '%s', %d, %d, 'HalloweenMinigame', 1, %d);", 
		ch->GetPlayerID(), ch->GetDesc()->GetAccountTable().login().c_str(), reward_pool[index].vnum, reward_pool[index].count, isGMItem);

	cmd = "HALLOWEENGAME " + std::to_string(random_number(3,6) * 10 + index) + cmd;
	ch->ChatPacket(CHAT_TYPE_COMMAND, cmd.c_str());
	ch->tchat(cmd.c_str());
}
#endif
