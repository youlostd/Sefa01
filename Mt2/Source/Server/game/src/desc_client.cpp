#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "guild_manager.h"
#include "db.h"

#include "party.h"

using namespace network;

extern LPFDWATCH	main_fdw;

LPCLIENT_DESC db_clientdesc = NULL;
LPCLIENT_DESC g_pkAuthMasterDesc = NULL;
LPCLIENT_DESC g_NetmarbleDBDesc = NULL;

static const char* GetKnownClientDescName(LPCLIENT_DESC desc) {
	if (desc == db_clientdesc) {
		return "db_clientdesc";
	} else if (desc == g_pkAuthMasterDesc) {
		return "g_pkAuthMasterDesc";
	} else if (desc == g_NetmarbleDBDesc) {
		return "g_NetmarbleDBDesc";
	}
	return "unknown";
}

CLIENT_DESC::CLIENT_DESC()
{
	m_iPhaseWhenSucceed = 0;
	m_bRetryWhenClosed = false;
	m_LastTryToConnectTime = 0;
}

CLIENT_DESC::~CLIENT_DESC()
{
}

void CLIENT_DESC::Destroy()
{
	if (m_sock != INVALID_SOCKET)
	{
		// DESC_MANAGER::instance().RemoveClientDesc(this); wtf
		P2P_MANAGER::instance().UnregisterConnector(this);

		if (this == db_clientdesc)
		{
			CPartyManager::instance().DeleteAllParty();
			CPartyManager::instance().DisablePCParty();
			CGuildManager::instance().StopAllGuildWar();
		}

		fdwatch_del_fd(m_lpFdw, m_sock);

		sys_log(0, "SYSTEM: closing client socket. DESC #%d", m_sock);

		socket_close(m_sock);
		m_sock = INVALID_SOCKET;
	}

	// Chain up to base class Destroy()
	DESC::Destroy();
}

void CLIENT_DESC::SetRetryWhenClosed(bool b)
{
	m_bRetryWhenClosed = b;
}

bool CLIENT_DESC::Connect(int iPhaseWhenSucceed)
{
	if (iPhaseWhenSucceed != 0)
		m_iPhaseWhenSucceed = iPhaseWhenSucceed;

	if (get_global_time() - m_LastTryToConnectTime < 3)	// 3ÃÊ
		return false;

	m_LastTryToConnectTime = get_global_time();

	if (m_sock != INVALID_SOCKET)
		return false;

	sys_log(0, "SYSTEM: Trying to connect to %s:%d", m_stHost.c_str(), m_wPort);

	m_sock = socket_connect(m_stHost.c_str(), m_wPort);

	if (m_sock != INVALID_SOCKET)
	{
		sys_log(0, "SYSTEM: connected to server (fd %d, ptr %p)", m_sock, this);
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, false);
		SetPhase(m_iPhaseWhenSucceed);
		return true;
	}
	else
	{
		SetPhase(PHASE_CLIENT_CONNECTING);
		return false;
	}
}

void CLIENT_DESC::Setup(LPFDWATCH _fdw, const char * _host, WORD _port)
{
	// 1MB input/output buffer
	m_lpFdw = _fdw;
	m_stHost = _host;
	m_wPort = _port;

	InitializeBuffers();

	m_sock = INVALID_SOCKET;
}

void CLIENT_DESC::SetPhase(int iPhase)
{
	switch (iPhase)
	{
		case PHASE_CLIENT_CONNECTING:
			sys_log(1, "PHASE_CLIENT_DESC::CONNECTING");
			m_pInputProcessor = NULL;
			break;

		case PHASE_DBCLIENT:
			{
				sys_log(1, "PHASE_DBCLIENT");

				if (!g_bAuthServer)
				{
					static bool bSentBoot = false;

					if (!bSentBoot)
					{
						bSentBoot = true;
						network::GDOutputPacket<network::GDBootPacket> p;
						p->set_ip(g_szPublicIP);
						DBPacket(p);
					}
				}

				network::GDOutputPacket<network::GDSetupPacket> p;
				p->set_public_ip(g_szPublicIP);
#ifdef PROCESSOR_CORE
				p->set_processor_core(g_isProcessorCore);
#endif

				if (!g_bAuthServer)
				{
					p->set_channel(g_bChannel);
					p->set_listen_port(mother_port);
					p->set_p2p_port(p2p_port);
					p->set_auth_server(false);
					map_allow_copy(p->mutable_maps());

					const DESC_MANAGER::DESC_SET & c_set = DESC_MANAGER::instance().GetClientSet();
					DESC_MANAGER::DESC_SET::const_iterator it;

					for (it = c_set.begin(); it != c_set.end(); ++it)
					{
						LPDESC d = *it;

						auto& r = d->GetAccountTable();
						if (r.id() != 0)
						{
							auto cur = p->add_logins();
							cur->set_id(r.id());
							cur->set_login(r.login());
							cur->set_social_id(r.social_id());
							cur->set_host(d->GetHostName());
							cur->set_login_key(d->GetLoginKey());
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//							for (int i = 0; i < 4; ++i)
//								cur->add_client_keys(d->GetDecryptionKey()[i]);
//#endif
						}
					}

					sys_log(0, "DB_SETUP current user %d size %d", p->logins_size(), p->ByteSize());

					// ÆÄÆ¼¸¦ Ã³¸®ÇÒ ¼ö ÀÖ°Ô µÊ.
					CPartyManager::instance().EnablePCParty();
					//CPartyManager::instance().SendPartyToDB();
				}
				else
				{
					p->set_auth_server(true);
				}

				DBPacket(p);
				m_pInputProcessor = &m_inputDB;
			}
			break;

		case PHASE_P2P:
			sys_log(1, "PHASE_P2P");
			
			if (m_lpInputBuffer)
				buffer_reset(m_lpInputBuffer);

			if (m_lpOutputBuffer)
				buffer_reset(m_lpOutputBuffer);

			m_pInputProcessor = &m_inputP2P;
			break;

		case PHASE_CLOSE:
			m_pInputProcessor = NULL;
			break;
	}

	m_iPhase = iPhase;
}

void CLIENT_DESC::DBPacket(TGDHeader header, uint32_t handle)
{
	network::TPacketHeader data;
	data.header = static_cast<uint16_t>(header);
	data.size = sizeof(data) + sizeof(uint32_t);

	DBPacketSend(&data, sizeof(data));
	DBPacketSend(&handle, sizeof(uint32_t));
}

void CLIENT_DESC::DBPacketSend(network::TPacketHeader& header, const ::google::protobuf::Message* data, uint32_t handle)
{
	auto extra_size = data->ByteSize();
	header.size = sizeof(header) + extra_size + sizeof(uint32_t);

	std::vector<char> data_array;
	data_array.resize(extra_size);
	if (extra_size && !data->SerializeToArray(&data_array[0], extra_size))
		return;

	DBPacketSend(&header, sizeof(header));
	DBPacketSend(&handle, sizeof(uint32_t));
	if (extra_size)
		DBPacketSend(&data_array[0], extra_size);
}

void CLIENT_DESC::DBPacketSend(const void * c_pvData, int iSize)
{
	if (m_sock == INVALID_SOCKET)
		sys_log(0, "CLIENT_DESC [%s] trying Packet() while not connected", GetKnownClientDescName(this));

	buffer_write(m_lpOutputBuffer, c_pvData, iSize);
}

bool CLIENT_DESC::IsRetryWhenClosed()
{
	return (0 == thecore_is_shutdowned() && m_bRetryWhenClosed);
}

void CLIENT_DESC::Update(DWORD t)
{
}

void CLIENT_DESC::Reset()
{
	// Backup connection target info
	LPFDWATCH fdw = m_lpFdw;
	std::string host = m_stHost;
	WORD port = m_wPort;

	Destroy();
	Initialize();

	// Restore connection target info
	m_lpFdw = fdw;
	m_stHost = host;
	m_wPort = port;

	InitializeBuffers();
}

void CLIENT_DESC::InitializeBuffers()
{
	SAFE_BUFFER_DELETE(m_lpOutputBuffer);
	SAFE_BUFFER_DELETE(m_lpInputBuffer);

	m_lpOutputBuffer = buffer_new(1024 * 1024);
	m_lpInputBuffer = buffer_new(1024 * 1024);
	m_iMinInputBufferLen = 1024 * 1024;
}
