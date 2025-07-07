#include "stdafx.h"
#include "../../common/stl.h"

#include "constants.h"
#include "config.h"
#include "p2p.h"
#include "desc_p2p.h"
#include "char.h"
#include "char_manager.h"
#include "sectree_manager.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "marriage.h"
#include "utils.h"
#include "map_location.h"
#include <sstream>
#ifdef P2P_SAFE_BUFFERING
#include "mtrand.h"
#endif

#ifdef __P2P_ONLINECOUNT__
#include "desc_manager.h"
#endif
#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#endif

P2P_MANAGER::P2P_MANAGER()
{
	m_pkInputProcessor = NULL;
	m_iHandleCount = 0;

	memset(m_aiEmpireUserCount, 0, sizeof(m_aiEmpireUserCount));

#ifdef PROCESSOR_CORE
	m_processorCore = nullptr;
#endif
}

P2P_MANAGER::~P2P_MANAGER()
{
}

void P2P_MANAGER::Boot(LPDESC d)
{
	CHARACTER_MANAGER::NAME_MAP & map = CHARACTER_MANAGER::instance().GetPCMap();
	CHARACTER_MANAGER::NAME_MAP::iterator it = map.begin();

	network::GGOutputPacket<network::GGLoginPacket> p;

	while (it != map.end())
	{
		LPCHARACTER ch = it->second;
		it++;

		p->set_name(ch->GetName());
		p->set_pid(ch->GetPlayerID());
		p->set_empire(ch->GetEmpire());
		p->set_map_index(SECTREE_MANAGER::instance().GetMapIndex(ch->GetX(), ch->GetY()));
		p->set_is_in_dungeon(ch->GetMapIndex() >= 10000);
		p->set_channel(g_bChannel);
		p->set_race(ch->GetRaceNum());
		p->set_language(ch->GetLanguageID());
		p->set_temp_login(ch->is_temp_login());

		d->Packet(p);
	}
}

void P2P_MANAGER::FlushOutput()
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pkPeers.begin();

	while (it != m_set_pkPeers.end())
	{
		LPDESC pkDesc = *it++;
		pkDesc->FlushOutput();
	}
}

void P2P_MANAGER::RegisterAcceptor(LPDESC d)
{
	sys_log(0, "P2P Acceptor opened (host %s)", d->GetHostName());
	m_set_pkPeers.insert(d);
	Boot(d);
}

void P2P_MANAGER::UnregisterAcceptor(LPDESC d)
{
	sys_log(0, "P2P Acceptor closed (host %s)", d->GetHostName());
	EraseUserByDesc(d);

#ifdef AUCTION_SYSTEM
	AuctionManager::instance().on_disconnect_peer(d);
#endif

#ifdef PROCESSOR_CORE
	if (m_processorCore == d)
		m_processorCore = nullptr;
#endif

	m_set_pkPeers.erase(d);
}

void P2P_MANAGER::RegisterConnector(LPDESC d)
{
	sys_log(0, "P2P Connector opened (host %s)", d->GetHostName());
	m_set_pkPeers.insert(d);
	Boot(d);

	network::GGOutputPacket<network::GGSetupPacket> p;
	p->set_port(p2p_port);
	p->set_listen_port(mother_port);
	p->set_channel(g_bChannel);
#ifdef PROCESSOR_CORE
	p->set_processor_core(g_isProcessorCore);
#endif
#ifdef P2P_SAFE_BUFFERING
	MTRandom mrnd(p->header());
	unsigned int sv = mrnd.next();
	TEMP_BUFFER buff;
	buff.write(&sv, sizeof(sv));
	buff.write(p);
	sv = mrnd.next();
	buff.write(&sv, sizeof(sv));

	d->Packet(buff.read_peek(), buff.size());
#else
	d->Packet(p);
#endif
}

void P2P_MANAGER::UnregisterConnector(LPDESC d)
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pkPeers.find(d);

	if (it != m_set_pkPeers.end())
	{
		sys_log(0, "P2P Connector closed (host %s)", d->GetHostName());
		EraseUserByDesc(d);

#ifdef AUCTION_SYSTEM
		AuctionManager::instance().on_disconnect_peer(d);
#endif

#ifdef PROCESSOR_CORE
		if (m_processorCore == d)
			m_processorCore = nullptr;
#endif

		m_set_pkPeers.erase(it);
	}
}

void P2P_MANAGER::EraseUserByDesc(LPDESC d)
{
	TCCIMap::iterator it = m_map_pkCCI.begin();

	while (it != m_map_pkCCI.end())
	{
		CCI * pkCCI = it->second;
		it++;

		if (pkCCI->pkDesc == d)
			Logout(pkCCI);
	}
}

LPDESC P2P_MANAGER::FindPeerByMap(int iIndex, BYTE bChannel) const
{
	long addr;
	WORD port;

	if (test_server)
		sys_log(0, "P2P_MANAGER::FindPeerByMap (index %d channel %d)", iIndex, bChannel);

	if (bChannel == 0)
	{
		if (!CMapLocation::instance().Get(iIndex, addr, port))
			return nullptr;
	}
	else
	{
		if (!CMapLocation::instance().GetByChannel(bChannel, iIndex, addr, port))
			return nullptr;
	}

	if (test_server)
		sys_log(0, " -> found addr %ld and port %u", addr, port);

	for (auto peer : m_set_pkPeers)
	{
		if (inet_addr(peer->GetHostName()) != addr)
			continue;

		if (peer->GetListenPort() != port)
			continue;

		return peer;
	}

	if (test_server)
		sys_log(0, " ... peer not found.");
	return nullptr;
}

void P2P_MANAGER::Login(LPDESC d, std::unique_ptr<network::GGLoginPacket> p)
{
	CCI* pkCCI = Find(p->name().c_str());

	bool UpdateP2P = false;

	if (NULL == pkCCI)
	{
		UpdateP2P = true;
		pkCCI = M2_NEW CCI;

		strlcpy(pkCCI->szName, p->name().c_str(), sizeof(pkCCI->szName));

		pkCCI->dwPID = p->pid();
		pkCCI->bEmpire = p->empire();

		if (p->channel() == g_bChannel)
		{
			if (pkCCI->bEmpire < EMPIRE_MAX_NUM)
			{
				++m_aiEmpireUserCount[pkCCI->bEmpire];
			}
			else
			{
				sys_err("LOGIN_EMPIRE_ERROR: %d > MAX(%d)", pkCCI->bEmpire, EMPIRE_MAX_NUM);
			}
		}

		m_map_pkCCI.insert(std::make_pair(pkCCI->szName, pkCCI));
		m_map_dwPID_pkCCI.insert(std::make_pair(pkCCI->dwPID, pkCCI));
	}

	pkCCI->lMapIndex = p->map_index();
	pkCCI->isInDungeon = p->is_in_dungeon();
	pkCCI->pkDesc = d;
	pkCCI->bChannel = p->channel();
	pkCCI->bLanguage = p->language();
	pkCCI->bRace = p->race();
	pkCCI->bTempLogin = p->temp_login();
	sys_log(0, "P2P: Login %s map_index: %d", pkCCI->szName, pkCCI->lMapIndex);

#ifdef AUCTION_SYSTEM
	AuctionManager::instance().on_player_login(pkCCI->dwPID, pkCCI);
#endif

	CGuildManager::instance().P2PLoginMember(pkCCI->dwPID);
	CPartyManager::instance().P2PLogin(pkCCI->dwPID, pkCCI->szName);

	// CCI°¡ »ý¼º½Ã¿¡¸¸ ¸Þ½ÅÀú¸¦ ¾÷µ¥ÀÌÆ®ÇÏ¸é µÈ´Ù.
	if (UpdateP2P) {
		std::string name(pkCCI->szName);
		MessengerManager::instance().P2PLogin(name);
	}
}

void P2P_MANAGER::Logout(CCI * pkCCI)
{
	if (pkCCI->bChannel == g_bChannel)
	{
		if (pkCCI->bEmpire < EMPIRE_MAX_NUM)
		{
			--m_aiEmpireUserCount[pkCCI->bEmpire];
			if (m_aiEmpireUserCount[pkCCI->bEmpire] < 0)
			{
				sys_err("m_aiEmpireUserCount[%d] < 0", pkCCI->bEmpire);
			}
		}
		else
		{
			sys_err("LOGOUT_EMPIRE_ERROR: %d >= MAX(%d)", pkCCI->bEmpire, EMPIRE_MAX_NUM);
		}
	}

	std::string name(pkCCI->szName);

	CGuildManager::instance().P2PLogoutMember(pkCCI->dwPID);
	CPartyManager::instance().P2PLogout(pkCCI->dwPID);
	MessengerManager::instance().P2PLogout(name);
	marriage::CManager::instance().Logout(pkCCI->dwPID);

#ifdef AUCTION_SYSTEM
	AuctionManager::instance().on_player_logout(pkCCI->dwPID);
#endif

	m_map_pkCCI.erase(name);
	m_map_dwPID_pkCCI.erase(pkCCI->dwPID);
	M2_DELETE(pkCCI);
}

void P2P_MANAGER::Logout(const char * c_pszName)
{
	CCI * pkCCI = Find(c_pszName);

	if (!pkCCI)
		return;

	Logout(pkCCI);
	sys_log(0, "P2P: Logout %s", c_pszName);
}

CCI * P2P_MANAGER::FindByPID(DWORD pid)
{
	TPIDCCIMap::iterator it = m_map_dwPID_pkCCI.find(pid);
	if (it == m_map_dwPID_pkCCI.end())
		return NULL;
	return it->second;
}

CCI * P2P_MANAGER::Find(const char * c_pszName)
{
	TCCIMap::const_iterator it = m_map_pkCCI.find(c_pszName);

	if (it == m_map_pkCCI.end())
		return NULL;

	return it->second;
}

int P2P_MANAGER::GetCount()
{
	//return m_map_pkCCI.size();
	return m_aiEmpireUserCount[1] + m_aiEmpireUserCount[2] + m_aiEmpireUserCount[3];
}

int P2P_MANAGER::GetEmpireUserCount(int idx)
{
	assert(idx < EMPIRE_MAX_NUM);
	return m_aiEmpireUserCount[idx];
}


int P2P_MANAGER::GetDescCount()
{
	return m_set_pkPeers.size();
}

void P2P_MANAGER::GetP2PHostNames(std::string& hostNames)
{
	TR1_NS::unordered_set<LPDESC>::iterator it = m_set_pkPeers.begin();

	std::ostringstream oss(std::ostringstream::out);

	while (it != m_set_pkPeers.end())
	{
		LPDESC pkDesc = *it++;

		oss << pkDesc->GetP2PHost() << " " << pkDesc->GetP2PPort() << "\n";

	}
	hostNames += oss.str();
}

#ifdef __P2P_ONLINECOUNT__
DWORD P2P_MANAGER::GetOnlinePlayerInfo(std::vector<network::TOnlinePlayerInfo>& rvec_PlayerInfo)
{
	// get user by desc
	const DESC_MANAGER::DESC_SET& rset_ClientDesc = DESC_MANAGER::instance().GetClientSet();
	for(itertype(rset_ClientDesc) it = rset_ClientDesc.begin(); it != rset_ClientDesc.end(); ++it)
	{
		LPCHARACTER pkChr = ( *it )->GetCharacter();

		if(pkChr && !( *it )->IsPhase(PHASE_LOADING))
		{
			static network::TOnlinePlayerInfo kInfo;

			kInfo.set_pid(pkChr->GetPlayerID());
			kInfo.set_map_index(pkChr->GetMapIndex());
			kInfo.set_channel(g_bChannel);

			rvec_PlayerInfo.push_back(kInfo);
		}
	}

	// get user by p2p manager
	const P2P_MANAGER::TPIDCCIMap* pP2PMap = P2P_MANAGER::instance().GetP2PCCIMap();
	for(itertype(*pP2PMap) it = pP2PMap->begin(); it != pP2PMap->end(); ++it)
	{
		CCI* pCCI = it->second;

		static network::TOnlinePlayerInfo kInfo;

		kInfo.set_pid(pCCI->dwPID);
		kInfo.set_map_index(pCCI->lMapIndex);
		kInfo.set_channel(pCCI->bChannel);

		rvec_PlayerInfo.push_back(kInfo);
	}

	return rvec_PlayerInfo.size();
}
#endif
