#include "stdafx.h"
#include "utils.h"
#include "char.h"
#include "party.h"
#include "char_manager.h"
#include "config.h"
#include "p2p.h"
#include "desc_client.h"
#include "dungeon.h"
#include "unique_item.h"

CPartyManager::CPartyManager()
{
	Initialize();
}

CPartyManager::~CPartyManager()
{
}

void CPartyManager::Initialize()
{
	m_bEnablePCParty = false;
}

void CPartyManager::DeleteAllParty()
{
	TPCPartySet::iterator it = m_set_pkPCParty.begin();

	while (it != m_set_pkPCParty.end())
	{
		DeleteParty(*it);
		it = m_set_pkPCParty.begin();
	}
}

bool CPartyManager::SetParty(LPCHARACTER ch)	// PC¸¸ »ç¿ëÇØ¾ß ÇÑ´Ù!!
{
	TPartyMap::iterator it = m_map_pkParty.find(ch->GetPlayerID());

	if (it == m_map_pkParty.end())
		return false;

	LPPARTY pParty = it->second;
	pParty->Link(ch);
	return true;
}

void CPartyManager::P2PLogin(DWORD pid, const char* name)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it == m_map_pkParty.end())
		return;

	it->second->UpdateOnlineState(pid, name);
}
void CPartyManager::P2PLogout(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it == m_map_pkParty.end())
		return;

	it->second->UpdateOfflineState(pid);
}

void CPartyManager::P2PJoinParty(DWORD leader, DWORD pid, BYTE role)
{
	TPartyMap::iterator it = m_map_pkParty.find(leader);

	if (it != m_map_pkParty.end())
	{
		it->second->P2PJoin(pid);

		if (role >= PARTY_ROLE_MAX_NUM)
			role = PARTY_ROLE_NORMAL;

		it->second->SetRole(pid, role, true);
	}
	else
	{
		sys_err("No such party with leader [%d]", leader);
	}
}

void CPartyManager::P2PQuitParty(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it != m_map_pkParty.end())
	{
		it->second->P2PQuit(pid);
	}
	else
	{
		sys_err("No such party with member [%d]", pid);
	}
}

LPPARTY CPartyManager::P2PCreateParty(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);
	if (it != m_map_pkParty.end())
		return it->second;

	LPPARTY pParty = M2_NEW CParty;

	m_set_pkPCParty.insert(pParty);

	SetPartyMember(pid, pParty);
	pParty->SetPCParty(true);
	pParty->P2PJoin(pid);

	return pParty;
}

void CPartyManager::P2PDeleteParty(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it != m_map_pkParty.end())
	{
		m_set_pkPCParty.erase(it->second);
		M2_DELETE(it->second);
	}
	else
		sys_err("PARTY P2PDeleteParty Cannot find party [%u]", pid);
}

LPPARTY CPartyManager::CreateParty(LPCHARACTER pLeader)
{
	if (pLeader->GetParty())
		return pLeader->GetParty();

	LPPARTY pParty = M2_NEW CParty;

	if (pLeader->IsPC())
	{
		//network::GGOutputPacket<network::GGPartyPacket> p;
		//p->set_subheader(PARTY_SUBHEADER_GG_CREATE);
		//p->set_pid(pLeader->GetPlayerID());
		//P2P_MANAGER::instance().Send(p);
		network::GDOutputPacket<network::GDPartyCreatePacket> p;
		p->set_leader_pid(pLeader->GetPlayerID());

		db_clientdesc->DBPacket(p);

		sys_log(0, "PARTY: Create %s pid %u", pLeader->GetName(), pLeader->GetPlayerID());
		pParty->SetPCParty(true);
		pParty->Join(pLeader->GetPlayerID());

		m_set_pkPCParty.insert(pParty);
	}
	else
	{
		pParty->SetPCParty(false);
		pParty->Join(pLeader->GetVID());
	}

	pParty->Link(pLeader);
	return (pParty);
}

void CPartyManager::DeleteParty(LPPARTY pParty)
{
	//network::GGOutputPacket<network::GGPartyPacket> p;
	//p->set_subheader(PARTY_SUBHEADER_GG_DESTROY);
	//p->set_pid(pParty->GetLeaderPID());
	//P2P_MANAGER::instance().Send(p);

	network::GDOutputPacket<network::GDPartyDeletePacket> p;
	p->set_leader_pid(pParty->GetLeaderPID());

	db_clientdesc->DBPacket(p);

	m_set_pkPCParty.erase(pParty);
	M2_DELETE(pParty);
}

void CPartyManager::SetPartyMember(DWORD dwPID, LPPARTY pParty)
{
	TPartyMap::iterator it = m_map_pkParty.find(dwPID);

	if (pParty == NULL)
	{
		if (it != m_map_pkParty.end())
			m_map_pkParty.erase(it);
	}
	else
	{
		if (it != m_map_pkParty.end())
		{
			if (it->second != pParty)
			{
				it->second->Quit(dwPID);
				it->second = pParty;
			}
		}
		else
			m_map_pkParty.insert(TPartyMap::value_type(dwPID, pParty));
	}
}

EVENTINFO(party_update_event_info)
{
	DWORD pid;

	party_update_event_info()
	: pid( 0 )
	{
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// CParty begin!
//
/////////////////////////////////////////////////////////////////////////////
EVENTFUNC(party_update_event)
{
	party_update_event_info* info = dynamic_cast<party_update_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "party_update_event> <Factor> Null pointer" );
		return 0;
	}

	DWORD pid = info->pid;
	LPCHARACTER leader = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (leader && leader->GetDesc())
	{
		LPPARTY pParty = leader->GetParty();

		if (pParty)
			pParty->Update();
	}

	return PASSES_PER_SEC(3);
}

CParty::CParty()
{
	Initialize();
}

CParty::~CParty()
{
	Destroy();
}

void CParty::Initialize()
{
	sys_log(2, "Party::Initialize");

	m_iExpDistributionMode = PARTY_EXP_DISTRIBUTION_NON_PARITY;
	m_pkChrExpCentralize = NULL;

	m_dwLeaderPID = 0;

	m_eventUpdate = NULL;

	memset(&m_anRoleCount, 0, sizeof(m_anRoleCount));
	memset(&m_anMaxRole, 0, sizeof(m_anMaxRole));
	m_anMaxRole[PARTY_ROLE_NORMAL] = 32;

	m_dwPartyStartTime = get_dword_time();
	m_iLongTimeExpBonus = 0;

	m_dwPartyHealTime = get_dword_time();
	m_bPartyHealReady = false;
	m_bCanUsePartyHeal = false;

	m_iLeadership = 0;
	m_iExpBonus = 0;
	m_iAttBonus = 0;
	m_iDefBonus = 0;

	m_itNextOwner = m_memberMap.begin();

	m_iCountNearPartyMember = 0;

	m_pkChrLeader = NULL;
	m_bPCParty = false;
	m_pkDungeon = NULL;
	m_pkDungeon_for_Only_party = NULL;
}


void CParty::Destroy()
{
	sys_log(2, "Party::Destroy");

	// PC°¡ ¸¸µç ÆÄÆ¼¸é ÆÄÆ¼¸Å´ÏÀú¿¡ ¸Ê¿¡¼­ PID¸¦ »èÁ¦ÇØ¾ß ÇÑ´Ù.
	if (m_bPCParty)
	{
		for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
			CPartyManager::instance().SetPartyMember(it->first, NULL);
	}

	event_cancel(&m_eventUpdate); 

	RemoveBonus();

	TMemberMap::iterator it = m_memberMap.begin();

	DWORD dwTime = get_dword_time();

	while (it != m_memberMap.end())
	{
		TMember & rMember = it->second;
		++it;

		if (rMember.pCharacter)
		{
			if (rMember.pCharacter->GetDesc())
			{
				network::GCOutputPacket<network::GCPartyRemovePacket> p;
				p->set_pid(rMember.pCharacter->GetPlayerID());
				rMember.pCharacter->GetDesc()->Packet(p);
				rMember.pCharacter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(rMember.pCharacter, "<ÆÄÆ¼> ÆÄÆ¼°¡ ÇØ»ê µÇ¾ú½À´Ï´Ù."));
			}
			else
			{
				// NPCÀÏ °æ¿ì ÀÏÁ¤ ½Ã°£ ÈÄ ÀüÅõ ÁßÀÌ ¾Æ´Ò ¶§ »ç¶óÁö°Ô ÇÏ´Â ÀÌº¥Æ®¸¦ ½ÃÀÛ½ÃÅ²´Ù.
				rMember.pCharacter->SetLastAttacked(dwTime);
				rMember.pCharacter->StartDestroyWhenIdleEvent();
			}

			rMember.pCharacter->SetParty(NULL);
		}
	}

	m_memberMap.clear();
	m_itNextOwner = m_memberMap.begin();
	
	if (m_pkDungeon_for_Only_party != NULL)
	{
		m_pkDungeon_for_Only_party->SetPartyNull();
		m_pkDungeon_for_Only_party = NULL;
	}
}

void CParty::ChatPacketToAllMember(BYTE type, const char* format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		TMember & rMember = it->second;

		if (rMember.pCharacter)
		{
			if (rMember.pCharacter->GetDesc())
			{
				rMember.pCharacter->ChatPacket(type, "%s", LC_TEXT(rMember.pCharacter, chatbuf));
			}
		}
	}
}

void CParty::ChatPacketToAllMemberNonTranslation(BYTE type, const char* format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		TMember & rMember = it->second;

		if (rMember.pCharacter)
		{
			if (rMember.pCharacter->GetDesc())
			{
				rMember.pCharacter->ChatPacket(type, chatbuf);
			}
		}
	}
}

DWORD CParty::GetLeaderPID()
{
	return m_dwLeaderPID;
}

DWORD CParty::GetMemberCount()
{
	return m_memberMap.size();
}

void CParty::P2PJoin(DWORD dwPID)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
	{
		TMember Member;

		Member.pCharacter	= NULL;
		Member.bNear		= false;
		Member.bLeader		= false;
		Member.bRole		= PARTY_ROLE_NORMAL;

		if (m_memberMap.empty())
		{
			Member.bLeader = true;
			m_dwLeaderPID = dwPID;
		}

		if (m_bPCParty)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

			if (ch)
			{
				sys_log(0, "PARTY: Join %s pid %u leader %u", ch->GetName(), dwPID, m_dwLeaderPID);
				Member.strName = ch->GetName();

				if (Member.bLeader)
					m_iLeadership = ch->GetLeadershipSkillLevel();

#ifdef LEADERSHIP_EXTENSION
				ch->SetLeadershipState(0);
#endif
			}
			else
			{
				CCI * pcci = P2P_MANAGER::instance().FindByPID(dwPID);

				if (!pcci);
				else if (pcci->bChannel == g_bChannel)
					Member.strName = pcci->szName;
				else
				{
					if (test_server)
						sys_err("member is not in same channel PID: %u channel %d, this channel %d", dwPID, pcci->bChannel, g_bChannel);
					else
						sys_log(0, "member is not in same channel PID: %u channel %d, this channel %d", dwPID, pcci->bChannel, g_bChannel);
				}
			}
		}

		sys_log(2, "PARTY[%d] MemberCountChange %d -> %d", GetLeaderPID(), GetMemberCount(), GetMemberCount()+1);

		m_memberMap.insert(TMemberMap::value_type(dwPID, Member));

		if (m_memberMap.size() == 1)
			m_itNextOwner = m_memberMap.begin();

		if (m_bPCParty)
		{
			CPartyManager::instance().SetPartyMember(dwPID, this);
			SendPartyJoinOneToAll(dwPID);

			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

			if (ch)
				SendParameter(ch);
		}
	}

	if (m_pkDungeon)
	{
		m_pkDungeon->QuitParty(this);
	}
}

void CParty::Join(DWORD dwPID)
{
	P2PJoin(dwPID);

	if (m_bPCParty)
	{
		network::GDOutputPacket<network::GDPartyAddPacket> p;
		p->set_leader_pid(GetLeaderPID());
		p->set_pid(dwPID);
		p->set_state(PARTY_ROLE_NORMAL); // #0000790: [M2EU] CZ Å©·¡½¬ Áõ°¡: ÃÊ±âÈ­ Áß¿ä! 
		db_clientdesc->DBPacket(p);
	}
}

// returns true if party is destroyed
bool CParty::P2PQuit(DWORD dwPID)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
		return false;

	if (m_bPCParty)
		SendPartyRemoveOneToAll(dwPID);

	if (it == m_itNextOwner)
		IncreaseOwnership();

	if (m_bPCParty)
		RemoveBonusForOne(dwPID);

	LPCHARACTER ch = it->second.pCharacter;
	BYTE bRole = it->second.bRole;
	bool bLeader = it->second.bLeader;

	m_memberMap.erase(it);

	sys_log(2, "PARTY[%d] MemberCountChange %d -> %d", GetLeaderPID(), GetMemberCount(), GetMemberCount() - 1);

	if (bRole < PARTY_ROLE_MAX_NUM)
	{
		--m_anRoleCount[bRole];
	}
	else
	{
		sys_err("ROLE_COUNT_QUIT_ERROR: INDEX(%d) > MAX(%d)", bRole, PARTY_ROLE_MAX_NUM);
	}

	if (ch)
	{
		ch->SetParty(NULL);
		ComputeRolePoint(ch, bRole, false);
	}

	if (m_bPCParty)
		CPartyManager::instance().SetPartyMember(dwPID, NULL);

	// ¸®´õ°¡ ³ª°¡¸é ÆÄÆ¼´Â ÇØ»êµÇ¾î¾ß ÇÑ´Ù.
	if (bLeader)
		CPartyManager::instance().DeleteParty(this);

	return bLeader;
	// ÀÌ ¾Æ·¡´Â ÄÚµå¸¦ Ãß°¡ÇÏÁö ¸» °Í!!! À§ DeleteParty ÇÏ¸é this´Â ¾ø´Ù.
}

bool CParty::IsInDungeon()
{
	if (m_pkDungeon)
		return true;

	LPCHARACTER curr;
	CCI* currCCI;
	for (auto it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		curr = CHARACTER_MANAGER::instance().FindByPID(it->first);
		if (curr)
		{
			if (curr->GetDungeon() || curr->GetMapIndex() >= 10000)
				return true;
		}
		else if ((currCCI = P2P_MANAGER::instance().FindByPID(it->first)) && currCCI->isInDungeon)
			return true;
	}
	return false;
}

void CParty::Quit(DWORD dwPID)
{
	// Always PC
	if (P2PQuit(dwPID))
		return;

	if (!m_bPCParty)
	{
		sys_log(!test_server, "No m_bPCParty[%d] to quit from %d", dwPID, m_bPCParty);
		return;
	}
	
	if (!GetLeaderPID())
	{
		sys_log(0, "Can't get[%d] partyLeaderPID() %d", dwPID, GetLeaderPID());
		return;
	}

	if (m_bPCParty && dwPID != GetLeaderPID())
	{
		//network::GGOutputPacket<network::GGPartyPacket> p;
		//p->set_subheader(PARTY_SUBHEADER_GG_QUIT);
		//p->set_pid(dwPID);
		//p->set_leaderpid(GetLeaderPID());
		//P2P_MANAGER::instance().Send(p);

		network::GDOutputPacket<network::GDPartyRemovePacket> p;
		p->set_pid(dwPID);
		p->set_leader_pid(GetLeaderPID());
		db_clientdesc->DBPacket(p);
	}
}

void CParty::Link(LPCHARACTER pkChr)
{
	TMemberMap::iterator it;

	if (pkChr->IsPC())
		it = m_memberMap.find(pkChr->GetPlayerID());
	else
		it = m_memberMap.find(pkChr->GetVID());

	if (it == m_memberMap.end())
	{
		sys_err("%s is not member of this party", pkChr->GetName());
		return;
	}

	// ÇÃ·¹ÀÌ¾î ÆÄÆ¼ÀÏ °æ¿ì ¾÷µ¥ÀÌÆ® ÀÌº¥Æ® »ý¼º
	if (m_bPCParty && !m_eventUpdate)
	{
		party_update_event_info* info = AllocEventInfo<party_update_event_info>();
		info->pid = m_dwLeaderPID;
		m_eventUpdate = event_create(party_update_event, info, PASSES_PER_SEC(3));
	}

	if (it->second.bLeader)
		m_pkChrLeader = pkChr;

	sys_log(2, "PARTY[%d] %s linked to party", GetLeaderPID(), pkChr->GetName());

	it->second.pCharacter = pkChr;
	pkChr->SetParty(this);

	if (pkChr->IsPC())
	{
		if (it->second.strName.empty())
		{
			it->second.strName = pkChr->GetName();
		}

		SendPartyJoinOneToAll(pkChr->GetPlayerID());

		SendPartyJoinAllToOne(pkChr);
		SendPartyLinkOneToAll(pkChr);
		SendPartyLinkAllToOne(pkChr);
		SendPartyInfoAllToOne(pkChr);
		SendPartyInfoOneToAll(pkChr);

		SendParameter(pkChr);

		//sys_log(0, "PARTY-DUNGEON connect %p %p", this, GetDungeon());
		if (GetDungeon() && GetDungeon()->GetMapIndex() == pkChr->GetMapIndex())
		{
			pkChr->SetDungeon(GetDungeon());
		}

		RequestSetMemberLevel(pkChr->GetPlayerID(), pkChr->GetLevel());

	}
}

void CParty::RequestSetMemberLevel(DWORD pid, BYTE level)
{
	network::GDOutputPacket<network::GDPartySetMemberLevelPacket> p;
	p->set_leader_pid(GetLeaderPID());
	p->set_pid(pid);
	p->set_level(level);
	db_clientdesc->DBPacket(p);
}

void CParty::P2PSetMemberLevel(DWORD pid, BYTE level)
{
	if (!m_bPCParty)
		return;

	TMemberMap::iterator it;

	sys_log(!test_server, "PARTY P2PSetMemberLevel leader %d pid %d level %d", GetLeaderPID(), pid, level);

	it = m_memberMap.find(pid);
	if (it != m_memberMap.end())
	{
		it->second.bLevel = level;
	}
}

namespace 
{
	struct FExitDungeon
	{
		void operator()(LPCHARACTER ch)
		{
			ch->ExitToSavedLocation();
		}
	};
}

void CParty::Unlink(LPCHARACTER pkChr)
{
	TMemberMap::iterator it;

	if (pkChr->IsPC())
		it = m_memberMap.find(pkChr->GetPlayerID());
	else
		it = m_memberMap.find(pkChr->GetVID());

	if (it == m_memberMap.end())
	{
		sys_err("%s is not member of this party", pkChr->GetName());
		return;
	}

	if (pkChr->IsPC())
	{
		SendPartyUnlinkOneToAll(pkChr);
		//SendPartyUnlinkAllToOne(pkChr); // ²÷±â´Â °ÍÀÌ¹Ç·Î ±¸Áö Unlink ÆÐÅ¶À» º¸³¾ ÇÊ¿ä ¾ø´Ù.

		if (it->second.bLeader)
		{
			RemoveBonus();

			if (it->second.pCharacter->GetDungeon())
			{
				// TODO: ´øÁ¯¿¡ ÀÖÀ¸¸é ³ª¸ÓÁöµµ ³ª°£´Ù
				FExitDungeon f;
				ForEachNearMember(f);
			}
		}
	}

	if (it->second.bLeader)
		m_pkChrLeader = NULL;

	it->second.pCharacter = NULL;
	pkChr->SetParty(NULL);
}

void CParty::SendPartyRemoveOneToAll(DWORD pid)
{
	TMemberMap::iterator it;

	network::GCOutputPacket<network::GCPartyRemovePacket> p;
	p->set_pid(pid);

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(p);
	}
}

void CParty::SendPartyJoinOneToAll(DWORD pid)
{
	const TMember& r = m_memberMap[pid];

	network::GCOutputPacket<network::GCPartyAddPacket> p;

	p->set_pid(pid);
	p->set_name(r.strName.c_str());

	for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(p);
	}
}

void CParty::SendPartyJoinAllToOne(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	network::GCOutputPacket<network::GCPartyAddPacket> p;

	for (TMemberMap::iterator it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		p->set_pid(it->first);
		p->set_name(it->second.strName);
		ch->GetDesc()->Packet(p);
	}
}

void CParty::SendPartyUnlinkOneToAll(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	network::GCOutputPacket<network::GCPartyLinkPacket> p;
	p->set_pid(ch->GetPlayerID());
	p->set_vid((DWORD)ch->GetVID());

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
		{
			it->second.pCharacter->GetDesc()->Packet(p);
		}
	}
}

void CParty::SendPartyLinkOneToAll(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	network::GCOutputPacket<network::GCPartyLinkPacket> p;
	p->set_vid(ch->GetVID());
	p->set_pid(ch->GetPlayerID());

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
		{
			it->second.pCharacter->GetDesc()->Packet(p);
		}
	}
}

void CParty::SendPartyLinkAllToOne(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	network::GCOutputPacket<network::GCPartyLinkPacket> p;

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter)
		{
			p->set_vid(it->second.pCharacter->GetVID());
			p->set_pid(it->second.pCharacter->GetPlayerID());
			ch->GetDesc()->Packet(p);
		}
	}
}

void CParty::SendPartyInfoOneToAll(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return;

	if (it->second.pCharacter)
	{
		SendPartyInfoOneToAll(it->second.pCharacter);
		return;
	}

	// Data Building
	network::GCOutputPacket<network::GCPartyUpdatePacket> p;
	p->set_pid(pid);
	p->set_percent_hp(255);
	p->set_role(it->second.bRole);
	p->set_leader(it->second.bLeader);

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if ((it->second.pCharacter) && (it->second.pCharacter->GetDesc()))
		{
			//sys_log(2, "PARTY send info %s[%d] to %s[%d]", ch->GetName(), (DWORD)ch->GetVID(), it->second.pCharacter->GetName(), (DWORD)it->second.pCharacter->GetVID());
			it->second.pCharacter->GetDesc()->Packet(p);
		}
	}
}

void CParty::SendPartyInfoOneToAll(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	// Data Building
	network::GCOutputPacket<network::GCPartyUpdatePacket> p;
	ch->BuildUpdatePartyPacket(p);

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if ((it->second.pCharacter) && (it->second.pCharacter->GetDesc()))
		{
			sys_log(2, "PARTY send info %s[%d] to %s[%d]", ch->GetName(), (DWORD)ch->GetVID(), it->second.pCharacter->GetName(), (DWORD)it->second.pCharacter->GetVID());
			it->second.pCharacter->GetDesc()->Packet(p);
		}
	}
}

void CParty::SendPartyInfoAllToOne(LPCHARACTER ch)
{
	TMemberMap::iterator it;

	network::GCOutputPacket<network::GCPartyUpdatePacket> p;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (!it->second.pCharacter)
		{
			DWORD pid = it->first;
			p->set_pid(pid);
			p->set_percent_hp(255);
			p->set_role(it->second.bRole);
			p->set_leader(it->second.bLeader);
			ch->GetDesc()->Packet(p);
			continue;
		}

		it->second.pCharacter->BuildUpdatePartyPacket(p);
		sys_log(2, "PARTY send info %s[%d] to %s[%d]", it->second.pCharacter->GetName(), (DWORD)it->second.pCharacter->GetVID(), ch->GetName(), (DWORD)ch->GetVID());
		ch->GetDesc()->Packet(p);
	}
}

void CParty::SendMessage(LPCHARACTER ch, BYTE bMsg, DWORD dwArg1, DWORD dwArg2)
{
	if (ch->GetParty() != this)
	{
		sys_err("%s is not member of this party %p", ch->GetName(), this);
		return;
	}

	switch (bMsg)
	{
		case PM_ATTACK:
			break;

		case PM_RETURN:
			{
				TMemberMap::iterator it = m_memberMap.begin();

				while (it != m_memberMap.end())
				{
					TMember & rMember = it->second;
					++it;

					LPCHARACTER pkChr;

					if ((pkChr = rMember.pCharacter) && ch != pkChr)
					{
						DWORD x = dwArg1 + random_number(-500, 500);
						DWORD y = dwArg2 + random_number(-500, 500);

						pkChr->SetVictim(NULL);
						pkChr->SetRotationToXY(x, y);

						if (pkChr->Goto(x, y))
						{
							LPCHARACTER victim = pkChr->GetVictim();
							sys_log(0, "%s %p RETURN victim %p", pkChr->GetName(), get_pointer(pkChr), get_pointer(victim));
							pkChr->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
						}
					}
				}
			}
			break;

		case PM_ATTACKED_BY:	// °ø°Ý ¹Þ¾ÒÀ½, ¸®´õ¿¡°Ô µµ¿òÀ» ¿äÃ»
			{
				// ¸®´õ°¡ ¾øÀ» ¶§
				LPCHARACTER pkChrVictim = ch->GetVictim();

				if (!pkChrVictim)
					return;

				TMemberMap::iterator it = m_memberMap.begin();

				while (it != m_memberMap.end())
				{
					TMember & rMember = it->second;
					++it;

					LPCHARACTER pkChr;

					if ((pkChr = rMember.pCharacter) && ch != pkChr)
					{
						if (pkChr->CanBeginFight())
							pkChr->BeginFight(pkChrVictim);
					}
				}
			}
			break;

		case PM_AGGRO_INCREASE:
			{
				LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(dwArg2);

				if (!victim)
					return;

				TMemberMap::iterator it = m_memberMap.begin();

				while (it != m_memberMap.end())
				{
					TMember & rMember = it->second;
					++it;

					LPCHARACTER pkChr;

					if ((pkChr = rMember.pCharacter) && ch != pkChr)
					{
						pkChr->UpdateAggrPoint(victim, DAMAGE_TYPE_SPECIAL, dwArg1);
					}
				}
			}
			break;
	}
}

LPCHARACTER CParty::GetLeaderCharacter()
{
	return m_memberMap[GetLeaderPID()].pCharacter;
}

bool CParty::SetRole(DWORD dwPID, BYTE bRole, bool bSet)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
	{
		return false;
	}

	LPCHARACTER ch = it->second.pCharacter;

	if (bSet)
	{
		if (m_anRoleCount[bRole] >= m_anMaxRole[bRole])
			return false;

		if (it->second.bRole != PARTY_ROLE_NORMAL)
			return false;

		it->second.bRole = bRole;

		if (ch && GetLeader())
			ComputeRolePoint(ch, bRole, true);

		if (bRole < PARTY_ROLE_MAX_NUM)
		{
			++m_anRoleCount[bRole];
		}
		else
		{
			sys_err("ROLE_COUNT_INC_ERROR: INDEX(%d) > MAX(%d)", bRole, PARTY_ROLE_MAX_NUM);
		}
	}
	else
	{
		if (it->second.bRole == PARTY_ROLE_NORMAL)
			return false;

		it->second.bRole = PARTY_ROLE_NORMAL;

		if (ch && GetLeader())
			ComputeRolePoint(ch, PARTY_ROLE_NORMAL, false);

		if (bRole < PARTY_ROLE_MAX_NUM)
		{
			--m_anRoleCount[bRole];
		}
		else
		{
			sys_err("ROLE_COUNT_DEC_ERROR: INDEX(%d) > MAX(%d)", bRole, PARTY_ROLE_MAX_NUM);
		}
	}

	SendPartyInfoOneToAll(dwPID);
	return true;
}

BYTE CParty::GetRole(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return PARTY_ROLE_NORMAL;
	else
		return it->second.bRole;
}

bool CParty::IsRole(DWORD pid, BYTE bRole)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return false;

	return it->second.bRole == bRole;
}

void CParty::RemoveBonus()
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch;

		if ((ch = it->second.pCharacter))
		{
			ComputeRolePoint(ch, it->second.bRole, false);
		}

		it->second.bNear = false;
	}
}

void CParty::RemoveBonusForOne(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return;

	LPCHARACTER ch;

	if ((ch = it->second.pCharacter))
		ComputeRolePoint(ch, it->second.bRole, false);
}

void CParty::HealParty()
{
	// XXX DELETEME Å¬¶óÀÌ¾ðÆ® ¿Ï·áµÉ¶§±îÁö
	{
		return;
	}
	if (!m_bPartyHealReady)
		return;

	TMemberMap::iterator it;
	LPCHARACTER l = GetLeaderCharacter();

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (!it->second.pCharacter)
			continue;

		LPCHARACTER ch = it->second.pCharacter;

		if (DISTANCE_APPROX(l->GetX()-ch->GetX(), l->GetY()-ch->GetY()) < PARTY_DEFAULT_RANGE)
		{
			ch->PointChange(POINT_HP, ch->GetMaxHP()-ch->GetHP());
			ch->PointChange(POINT_SP, ch->GetMaxSP()-ch->GetSP());
		}
	}

	m_bPartyHealReady = false;
	m_dwPartyHealTime = get_dword_time();
}

void CParty::SummonToLeader(DWORD pid)
{
	int xy[12][2] = 
	{
		{	250,	0		},
		{	216,	125		},
		{	125,	216		},
		{	0,		250		},
		{	-125,	216		},
		{	-216,	125		},
		{	-250,	0		},
		{	-216,	-125	},
		{	-125,	-216	},
		{	0,		-250	},
		{	125,	-216	},
		{	216,	-125	},
	};

	int n = 0;
	int x[12], y[12];

	SECTREE_MANAGER & s = SECTREE_MANAGER::instance();
	LPCHARACTER l = GetLeaderCharacter();

	if (m_memberMap.find(pid) == m_memberMap.end())
	{
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(l, "<ÆÄÆ¼> ¼ÒÈ¯ÇÏ·Á´Â ´ë»óÀ» Ã£À» ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	LPCHARACTER ch = m_memberMap[pid].pCharacter;

	if (!ch)
	{
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(l, "<ÆÄÆ¼> ¼ÒÈ¯ÇÏ·Á´Â ´ë»óÀ» Ã£À» ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (!ch->CanSummon(m_iLeadership))
	{
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(l, "<ÆÄÆ¼> ´ë»óÀ» ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	for (int i = 0; i < 12; ++i)
	{
		PIXEL_POSITION p;

		if (s.GetMovablePosition(l->GetMapIndex(), l->GetX() + xy [i][0], l->GetY() + xy[i][1], p))
		{
			x[n] = p.x;
			y[n] = p.y;
			n++;
		}
	}

	if (n == 0)
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(l, "<ÆÄÆ¼> ÆÄÆ¼¿øÀ» ÇöÀç À§Ä¡·Î ¼ÒÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
	else
	{
		int i = random_number(0, n - 1);
		ch->Show(l->GetMapIndex(), x[i], y[i]);
		ch->Stop();
	}
}

void CParty::IncreaseOwnership()
{
	if (m_memberMap.empty())
	{
		m_itNextOwner = m_memberMap.begin();
		return;
	}

	if (m_itNextOwner == m_memberMap.end())
		m_itNextOwner = m_memberMap.begin();
	else
	{
		m_itNextOwner++;

		if (m_itNextOwner == m_memberMap.end())
			m_itNextOwner = m_memberMap.begin();
	}
}

LPCHARACTER CParty::GetNextOwnership(LPCHARACTER ch, long x, long y)
{
	if (m_itNextOwner == m_memberMap.end())
		return ch;

	int size = m_memberMap.size();

	while (size-- > 0)
	{
		LPCHARACTER pkMember = m_itNextOwner->second.pCharacter;

		if (pkMember && DISTANCE_APPROX(pkMember->GetX() - x, pkMember->GetY() - y) < 3000)
		{
			IncreaseOwnership();
			return pkMember;
		}

		IncreaseOwnership();
	}

	return ch;
}

void CParty::ComputeRolePoint(LPCHARACTER ch, BYTE bRole, bool bAdd)
{
	if (ch->GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No party-bonus on this map.");
		return;
	}
	
	if (!bAdd)
	{
		const BYTE abResetPoints[] = {
			POINT_PARTY_ATTACKER_BONUS,
			POINT_PARTY_TANKER_BONUS,
			POINT_PARTY_BUFFER_BONUS,
			POINT_PARTY_SKILL_MASTER_BONUS,
			POINT_PARTY_DEFENDER_BONUS,
			POINT_PARTY_HASTE_BONUS,
		};

		bool bResetAny = false;
		for (BYTE bPoint : abResetPoints)
		{
			int iCurPoint = ch->GetPoint(bPoint);
			if (iCurPoint)
			{
				bResetAny = true;
				ch->PointChange(bPoint, -iCurPoint);
			}
		}
		if (bResetAny)
			ch->ComputePoints();

		return;
	}

	//SKILL_POWER_BY_LEVEL
	float k = (float) ch->GetSkillPowerByLevel( MIN(SKILL_MAX_LEVEL, m_iLeadership ) )/ 100.0f;
	//END_SKILL_POWER_BY_LEVEL

#ifdef ENABLE_RUNE_SYSTEM
	if(m_pkChrLeader && m_pkChrLeader->GetPoint(POINT_RUNE_LEADERSHIP_BONUS))
	{
		float flLeadershipBonus = m_pkChrLeader->GetPoint(POINT_RUNE_LEADERSHIP_BONUS) / 100.f;
		k += ( k * flLeadershipBonus );
	}
#endif

	switch (bRole)
	{
		case PARTY_ROLE_ATTACKER:
			{
				int iBonus = (int) (10 + 88 * k);
				// ch->tchat("PARTY_ROLE_ATTACKER == %i", iBonus);
				if (ch->GetPoint(POINT_PARTY_ATTACKER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_ATTACKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_TANKER:
			{
				int iBonus = (int) (50 + 1960 * k);

				if (ch->GetPoint(POINT_PARTY_TANKER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_TANKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_TANKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_BUFFER:
			{
				int iBonus = (int) (5 + 92 * k);
				// ch->tchat("PARTY_ROLE_BUFFER == %i", iBonus);

				if (ch->GetPoint(POINT_PARTY_BUFFER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_BUFFER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_SKILL_MASTER:
			{
				int iBonus = (int) (25 + 780 * k);
				// ch->tchat("PARTY_ROLE_SKILL_MASTER == %i", iBonus);

				if (ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_HASTE:
			{
				int iBonus = (int) (1+7.2*k);
				// ch->tchat("PARTY_ROLE_HASTE == %i", iBonus);
				if (ch->GetPoint(POINT_PARTY_HASTE_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_HASTE_BONUS, iBonus - ch->GetPoint(POINT_PARTY_HASTE_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_DEFENDER:
			{
				int iBonus = (int) (5+76*k);
				// ch->tchat("PARTY_ROLE_DEFENDER == %i", iBonus);
				if (ch->GetPoint(POINT_PARTY_DEFENDER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_DEFENDER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
	}
}

void CParty::Update()
{
	sys_log(1, "PARTY::Update");

	LPCHARACTER l = GetLeaderCharacter();

	if (!l)
		return;

	TMemberMap::iterator it;

	int iNearMember = 0;
	bool bResendAll = false;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;

		it->second.bNear = false;

		if (!ch)
			continue;

		if (l->GetDungeon())
			it->second.bNear = l->GetDungeon() == ch->GetDungeon();
		else
			it->second.bNear = l->GetMapIndex() == ch->GetMapIndex() && (DISTANCE_APPROX(l->GetX()-ch->GetX(), l->GetY()-ch->GetY()) < PARTY_DEFAULT_RANGE * 3);

		if (it->second.bNear)
		{
			++iNearMember;
			//sys_log(0,"NEAR %s", ch->GetName());
		}
	}

	if (iNearMember <= 1 && !l->GetDungeon())
	{
		for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
			it->second.bNear = false;

		iNearMember = 0;
	}

	if (iNearMember != m_iCountNearPartyMember)
	{
		m_iCountNearPartyMember = iNearMember;
		bResendAll = true;
	}

	m_iLeadership = l->GetLeadershipSkillLevel();
	int iNewExpBonus = ComputePartyBonusExpPercent();
	m_iAttBonus = ComputePartyBonusAttackGrade();
	m_iDefBonus = ComputePartyBonusDefenseGrade();

	if (m_iExpBonus != iNewExpBonus)
	{
		bResendAll = true;
		m_iExpBonus = iNewExpBonus;
	}

	bool bLongTimeExpBonusChanged = false;

	// ÆÄÆ¼ °á¼º ÈÄ ÃæºÐÇÑ ½Ã°£ÀÌ Áö³ª¸é °æÇèÄ¡ º¸³Ê½º¸¦ ¹Þ´Â´Ù.
	if (!m_iLongTimeExpBonus && (get_dword_time() - m_dwPartyStartTime > PARTY_ENOUGH_MINUTE_FOR_EXP_BONUS * 60 * 1000))
	{
		bLongTimeExpBonusChanged = true;
		m_iLongTimeExpBonus = 5;
		bResendAll = true;
	}

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;

		if (!ch)
			continue;

		if (bLongTimeExpBonusChanged && ch->GetDesc())
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÆÄÆ¼ÀÇ Çùµ¿·ÂÀÌ ³ô¾ÆÁ® Áö±ÝºÎÅÍ Ãß°¡ °æÇèÄ¡ º¸³Ê½º¸¦ ¹Þ½À´Ï´Ù."));

		bool bNear = it->second.bNear || it->second.bLeader;

		ComputeRolePoint(ch, it->second.bRole, bNear);

		if (bNear)
		{
			if (!bResendAll)
				SendPartyInfoOneToAll(ch);
		}
	}

	// PARTY_ROLE_LIMIT_LEVEL_BUG_FIX
	m_anMaxRole[PARTY_ROLE_ATTACKER]	 = m_iLeadership >= 10 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_HASTE]	 = m_iLeadership >= 20 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_TANKER]	 = m_iLeadership >= 20 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_BUFFER]	 = m_iLeadership >= 25 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_SKILL_MASTER] = m_iLeadership >= 35 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_DEFENDER] 	 = m_iLeadership >= 40 ? 1 : 0;

	if (m_iLeadership >= 40)
		m_anMaxRole[PARTY_ROLE_ATTACKER] = 3;
	// END_OF_PARTY_ROLE_LIMIT_LEVEL_BUG_FIX

	// Party Heal Update
	if (!m_bPartyHealReady)
	{
		if (!m_bCanUsePartyHeal && m_iLeadership >= 18)
			m_dwPartyHealTime = get_dword_time();

		m_bCanUsePartyHeal = m_iLeadership >= 18; // Åë¼Ö·Â 18 ÀÌ»óÀº ÈúÀ» »ç¿ëÇÒ ¼ö ÀÖÀ½.

		// Åë¼Ö·Â 40ÀÌ»óÀº ÆÄÆ¼ Èú ÄðÅ¸ÀÓÀÌ Àû´Ù.
		DWORD PartyHealCoolTime = (m_iLeadership >= 40) ? PARTY_HEAL_COOLTIME_SHORT * 60 * 1000 : PARTY_HEAL_COOLTIME_LONG * 60 * 1000;

		if (m_bCanUsePartyHeal)
		{
			if (get_dword_time() > m_dwPartyHealTime + PartyHealCoolTime)
			{
				m_bPartyHealReady = true;

				// send heal ready
				if (0) // XXX  DELETEME Å¬¶óÀÌ¾ðÆ® ¿Ï·áµÉ¶§±îÁö
					if (GetLeaderCharacter())
						GetLeaderCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "PartyHealReady");
			}
		}
	}

	if (bResendAll)
	{
		for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
			if (it->second.pCharacter)
				SendPartyInfoOneToAll(it->second.pCharacter);
	}
}

void CParty::UpdateOnlineState(DWORD dwPID, const char* name)
{
	TMember& r = m_memberMap[dwPID];

	network::GCOutputPacket<network::GCPartyAddPacket> p;

	p->set_pid(dwPID);
	r.strName = name;
	p->set_name(name);

	for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(p);
	}
}
void CParty::UpdateOfflineState(DWORD dwPID)
{
	//const TMember& r = m_memberMap[dwPID];

	network::GCOutputPacket<network::GCPartyAddPacket> p;
	p->set_pid(dwPID);

	for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(p);
	}
}

int CParty::GetFlag(const std::string& name)
{
	TFlagMap::iterator it = m_map_iFlag.find(name);

	if (it != m_map_iFlag.end())
	{
		//sys_log(0,"PARTY GetFlag %s %d", name.c_str(), it->second);
		return it->second;
	}

	//sys_log(0,"PARTY GetFlag %s 0", name.c_str());
	return 0;
}

void CParty::SetFlag(const std::string& name, int value)
{
	TFlagMap::iterator it = m_map_iFlag.find(name);

	//sys_log(0,"PARTY SetFlag %s %d", name.c_str(), value);
	if (it == m_map_iFlag.end())
	{
		m_map_iFlag.insert(make_pair(name, value));
	}
	else if (it->second != value)
	{
		it->second = value;
	}
}

void CParty::SetDungeon(LPDUNGEON pDungeon)
{
	m_pkDungeon = pDungeon;
	m_map_iFlag.clear();
}

LPDUNGEON CParty::GetDungeon()
{
	return m_pkDungeon;
}

void CParty::SetDungeon_for_Only_party(LPDUNGEON pDungeon)
{
	m_pkDungeon_for_Only_party = pDungeon;
}

LPDUNGEON CParty::GetDungeon_for_Only_party()
{
	return m_pkDungeon_for_Only_party;
}


bool CParty::IsPositionNearLeader(LPCHARACTER ch)
{
	if (!m_pkChrLeader)
		return false;

	if (DISTANCE_APPROX(ch->GetX() - m_pkChrLeader->GetX(), ch->GetY() - m_pkChrLeader->GetY()) >= PARTY_DEFAULT_RANGE)
		return false;

	return true;
}


int CParty::GetExpBonusPercent()
{
	if (GetNearMemberCount() <= 1)
		return 0;

	return m_iExpBonus + m_iLongTimeExpBonus;
}

bool CParty::IsNearLeader(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return false;	

	return it->second.bNear;
}

BYTE CParty::CountMemberByVnum(DWORD dwVnum)
{
	if (m_bPCParty)
		return 0;

	LPCHARACTER tch;
	BYTE bCount = 0;

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (!(tch = it->second.pCharacter))
			continue;

		if (tch->IsPC())
			continue;

		if (tch->GetMobTable().vnum() == dwVnum)
			++bCount;
	}

	return bCount;
}

void CParty::SendParameter(LPCHARACTER ch)
{
	network::GCOutputPacket<network::GCPartyParameterPacket> p;

	p->set_distribute_mode(m_iExpDistributionMode);

	LPDESC d = ch->GetDesc();

	if (d)
	{
		d->Packet(p);
	}
}

void CParty::SendParameterToAll()
{
	if (!m_bPCParty)
		return;

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		if (it->second.pCharacter)
			SendParameter(it->second.pCharacter);
}

void CParty::SetParameter(int iMode)
{
	if (iMode >= PARTY_EXP_DISTRIBUTION_MAX_NUM)
	{
		sys_err("Invalid exp distribution mode %d", iMode);
		return;
	}

	m_iExpDistributionMode = iMode;
	SendParameterToAll();
}

int CParty::GetExpDistributionMode()
{
	return m_iExpDistributionMode;
}

void CParty::SetExpCentralizeCharacter(DWORD dwPID)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
		return;

	m_pkChrExpCentralize = it->second.pCharacter;
}

LPCHARACTER CParty::GetExpCentralizeCharacter()
{
	return m_pkChrExpCentralize;
}

BYTE CParty::GetMemberMaxLevel()
{
	BYTE bMax = 0;

	itertype(m_memberMap) it = m_memberMap.begin();
	while (it!=m_memberMap.end())
	{
		if (!it->second.bLevel)
		{
			++it;
			continue;
		}

		if (!bMax)
			bMax = it->second.bLevel;
		else if (it->second.bLevel)
			bMax = MAX(bMax, it->second.bLevel);
		++it;
	}
	return bMax;
}

BYTE CParty::GetMemberMinLevel()
{
	BYTE bMin = PLAYER_MAX_LEVEL_CONST;

	itertype(m_memberMap) it = m_memberMap.begin();
	while (it!=m_memberMap.end())
	{
		if (!it->second.bLevel)
		{
			++it;
			continue;
		}

		if (!bMin)
			bMin = it->second.bLevel;
		else if (it->second.bLevel)
			bMin = MIN(bMin, it->second.bLevel);
		++it;
	}
	return bMin;
}

int CParty::ComputePartyBonusExpPercent()
{
	if (GetNearMemberCount() <= 1)
		return 0;

	LPCHARACTER leader = GetLeaderCharacter();

	int iBonusPartyExpFromItem = 0;

	// UPGRADE_PARTY_BONUS
	int iMemberCount=MIN(8, GetNearMemberCount());

	if (leader && (leader->IsEquipUniqueItem(UNIQUE_ITEM_PARTY_BONUS_EXP) || leader->IsEquipUniqueItem(UNIQUE_ITEM_PARTY_BONUS_EXP_MALL)
		|| leader->IsEquipUniqueItem(UNIQUE_ITEM_PARTY_BONUS_EXP_GIFT) || leader->IsEquipUniqueGroup(10010)))
	{
		iBonusPartyExpFromItem = 30;
	}

	return iBonusPartyExpFromItem + CHN_aiPartyBonusExpPercentByMemberCount[iMemberCount];
	// END_OF_UPGRADE_PARTY_BONUS
}

