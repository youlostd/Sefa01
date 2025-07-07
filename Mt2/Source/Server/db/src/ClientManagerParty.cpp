// vim:ts=4 sw=4
#include "stdafx.h"
#include "ClientManager.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"

using namespace network;

bool CClientManager::PARTY_FIND_BY_LEADER(DWORD dwLeaderPID, TPartyMap::iterator& itRet, TPartyMap** ppRetPartyMap)
{
#ifdef __PARTY_GLOBAL__
	itRet = m_map_pkParty.find(dwLeaderPID);
	if (itRet != m_map_pkParty.end())
	{
		if (ppRetPartyMap)
			*ppRetPartyMap = &m_map_pkParty;
		return true;
	}
#else
	for (auto it = m_map_pkChannelParty.begin(); it != m_map_pkChannelParty.end(); ++it)
	{
		if (it->first == 99)
			continue;

		itRet = it->second.find(dwLeaderPID);
		if (itRet != it->second.end())
		{
			if (ppRetPartyMap)
				*ppRetPartyMap = &it->second;
			return true;
		}
	}
#endif

	return false;
}

void CClientManager::QUERY_PARTY_CREATE(CPeer* peer, std::unique_ptr<network::GDPartyCreatePacket> p)
{
#ifdef __PARTY_GLOBAL__
	TPartyMap & pm = m_map_pkParty;
#else
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
#endif
	auto it = pm.find(p->leader_pid());

	if (it == pm.end() && !PARTY_FIND_BY_LEADER(p->leader_pid(), it))
	{
		pm.insert(make_pair(p->leader_pid(), TPartyMember()));
#ifdef __PARTY_GLOBAL__
		DGOutputPacket<DGPartyCreatePacket> pack;
		pack->set_leader_pid(p->leader_pid());
		ForwardPacket(pack, 0, peer);
#else
		ForwardPacket(HEADER_DG_PARTY_CREATE, p, sizeof(TPacketPartyCreate), peer->GetChannel(), peer);
#endif
		sys_log(0, "PARTY Create [%lu]", p->leader_pid());
	}
	else
	{
		sys_err("PARTY Create - Already exists [%lu]", p->leader_pid());
	}
}

void CClientManager::QUERY_PARTY_DELETE(CPeer* peer, std::unique_ptr<network::GDPartyDeletePacket> p)
{
#ifdef __PARTY_GLOBAL__
	TPartyMap* pm = &m_map_pkParty;
#else
	TPartyMap* pm = &m_map_pkChannelParty[peer->GetChannel()];
#endif
	auto it = pm->find(p->leader_pid());

	if (it == pm->end() && !PARTY_FIND_BY_LEADER(p->leader_pid(), it, &pm))
	{
		sys_err("PARTY Delete - Non exists [%lu]", p->leader_pid());
		return;
	}

	pm->erase(it);
#ifdef __PARTY_GLOBAL__
	network::DGOutputPacket<network::DGPartyDeletePacket> pack;
	pack->set_leader_pid(p->leader_pid());
	ForwardPacket(pack, 0, peer);
#else
	ForwardPacket(HEADER_DG_PARTY_DELETE, p, sizeof(TPacketPartyDelete), peer->GetChannel(), peer);
#endif
	sys_log(0, "PARTY Delete [%lu]", p->leader_pid());
}

void CClientManager::QUERY_PARTY_ADD(CPeer* peer, std::unique_ptr<network::GDPartyAddPacket> p)
{
#ifdef __PARTY_GLOBAL__
	TPartyMap & pm = m_map_pkParty;
#else
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
#endif
	auto it = pm.find(p->leader_pid());

	if (it == pm.end() && !PARTY_FIND_BY_LEADER(p->leader_pid(), it))
	{
		sys_err("PARTY Add - Non exists [%lu]", p->leader_pid());
		return;
	}

	if (it->second.find(p->pid()) == it->second.end())
	{
		it->second.insert(std::make_pair(p->pid(), TPartyInfo()));
#ifdef __PARTY_GLOBAL__
		network::DGOutputPacket<network::DGPartyAddPacket> pack;
		pack->set_leader_pid(p->leader_pid());
		pack->set_pid(p->pid());
		pack->set_state(p->state());
		ForwardPacket(pack, 0, peer);
#else
		ForwardPacket(HEADER_DG_PARTY_ADD, p, sizeof(TPacketPartyAdd), peer->GetChannel(), peer);
#endif
		sys_log(0, "PARTY Add [%lu] to [%lu]", p->pid(), p->leader_pid());
	}
	else
	{
		sys_err("PARTY Add - Already [%lu] in party [%lu]", p->pid(), p->leader_pid());
	}
}

void CClientManager::QUERY_PARTY_REMOVE(CPeer* peer, std::unique_ptr<network::GDPartyRemovePacket> p)
{
#ifdef __PARTY_GLOBAL__
	TPartyMap & pm = m_map_pkParty;
#else
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
#endif
	auto it = pm.find(p->leader_pid());

	if (it == pm.end() && !PARTY_FIND_BY_LEADER(p->leader_pid(), it))
	{
		sys_err("PARTY Remove - Non exists [%lu] cannot remove [%lu]", p->leader_pid(), p->pid());
		return;
	}

	auto pit = it->second.find(p->pid());

	if (pit != it->second.end())
	{
		it->second.erase(pit);
#ifdef __PARTY_GLOBAL__
		network::DGOutputPacket<network::DGPartyRemovePacket> pack;
		pack->set_pid(p->pid());
		ForwardPacket(pack, 0, peer);
#else
		ForwardPacket(HEADER_DG_PARTY_REMOVE, p, sizeof(TPacketPartyRemove), peer->GetChannel(), peer);
#endif
		sys_log(0, "PARTY Remove [%lu] to [%lu]", p->pid(), p->leader_pid());
	}
	else
	{
		sys_err("PARTY Remove - Cannot find [%lu] in party [%lu]", p->pid(), p->leader_pid());
	}
}

void CClientManager::QUERY_PARTY_STATE_CHANGE(CPeer* peer, std::unique_ptr<network::GDPartyStateChangePacket> p)
{
#ifdef __PARTY_GLOBAL__
	TPartyMap & pm = m_map_pkParty;
#else
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
#endif
	auto it = pm.find(p->leader_pid());

	if (it == pm.end() && !PARTY_FIND_BY_LEADER(p->leader_pid(), it))
	{
		sys_err("PARTY StateChange - Non exists [%lu] cannot state change [%lu]", p->leader_pid(), p->pid());
		return;
	}

	auto pit = it->second.find(p->pid());

	if (pit == it->second.end())
	{
		sys_err("PARTY StateChange - Cannot find [%lu] in party [%lu]", p->pid(), p->leader_pid());
		return;
	}

	if (p->flag())
		pit->second.bRole = p->role();
	else
		pit->second.bRole = 0;

#ifdef __PARTY_GLOBAL__
	network::DGOutputPacket<network::DGPartyStateChangePacket> pack;
	pack->set_leader_pid(p->leader_pid());
	pack->set_pid(p->pid());
	pack->set_role(p->role());
	pack->set_flag(p->flag());
	ForwardPacket(pack, 0, peer);
#else
	ForwardPacket(HEADER_DG_PARTY_STATE_CHANGE, p, sizeof(TPacketPartyStateChange), peer->GetChannel(), peer);
#endif
	sys_log(0, "PARTY StateChange [%lu] at [%lu] from %d %d", p->pid(), p->leader_pid(), p->role(), p->flag());
}

void CClientManager::QUERY_PARTY_SET_MEMBER_LEVEL(CPeer* peer, std::unique_ptr<network::GDPartySetMemberLevelPacket> p)
{
	char szHint[256];

#ifdef __PARTY_GLOBAL__
	TPartyMap & pm = m_map_pkParty;
#else
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
#endif
	auto it = pm.find(p->leader_pid());

	if (it == pm.end() && !PARTY_FIND_BY_LEADER(p->leader_pid(), it))
	{
		snprintf(szHint, sizeof(szHint), "PARTY_NOT_EXISTS | newLevel %d", p->level());
		sys_err("PARTY SetMemberLevel - Non exists [%lu] cannot level change [%lu]", p->leader_pid(), p->pid());
		return;
	}

	auto pit = it->second.find(p->pid());

	if (pit == it->second.end())
	{
		snprintf(szHint, sizeof(szHint), "NOT_IN_PARTY | newLevel %d", p->level());
		sys_err("PARTY SetMemberLevel - Cannot find [%lu] in party [%lu]", p->pid(), p->leader_pid());
		return;
	}

	pit->second.bLevel = p->level();

	snprintf(szHint, sizeof(szHint), "newLevel %d", p->level());
#ifdef __PARTY_GLOBAL__
	network::DGOutputPacket<network::DGPartySetMemberLevelPacket> pack;
	pack->set_leader_pid(p->leader_pid());
	pack->set_pid(p->pid());
	pack->set_level(p->level());
	ForwardPacket(pack, 0);
#else
	ForwardPacket(HEADER_DG_PARTY_SET_MEMBER_LEVEL, p, sizeof(TPacketPartySetMemberLevel), peer->GetChannel());
#endif
	sys_log(0, "PARTY SetMemberLevel pid [%lu] level %d", p->pid(), p->level());
}
