#include "StdAfx.h"
#include "AccountConnector.h"
#include "Packet.h"
#include "PythonNetworkStream.h"
#include "../EterBase/tea.h"
#include "../EterPack/EterPackManager.h"
#include "PythonSystem.h"
#include "../EterPack/mtrand.h"

#include "headers.hpp"

using namespace network;

extern DWORD g_adwEncryptKey[4];
extern DWORD g_adwDecryptKey[4];

extern char pc_name[255], user_name[255], pc_hash[32*2+1];

extern const char * g_szCurrentClientVersion;

void CAccountConnector::SetHandler(PyObject* poHandler)
{
	m_poHandler = poHandler;
}

void CAccountConnector::SetLoginInfo(const char * c_szName, const char * c_szPwd)
{
	m_strID = c_szName;
	m_strPassword = c_szPwd;
}

void CAccountConnector::ClearLoginInfo( void )
{
	m_strPassword = "";
}

bool CAccountConnector::Connect(const char * c_szAddr, int iPort, const char * c_szAccountAddr, int iAccountPort)
{
	m_strAddr = c_szAddr;
	m_iPort = iPort;

	__OfflineState_Set();
	__BuildClientKey_20050304Myevan();

	return CNetworkStream::Connect(c_szAccountAddr, iAccountPort);
}

void CAccountConnector::Disconnect()
{
	CNetworkStream::Disconnect();
	__OfflineState_Set();
}

void CAccountConnector::Process()
{
	CNetworkStream::Process();

	if (!__StateProcess())
	{
		__OfflineState_Set();
		Disconnect();
	}
}

bool CAccountConnector::__StateProcess()
{
	static InputPacket packet;
	while (Recv(packet))
	{
		auto header = packet.get_header<TGCHeader>();

		if (m_eState == STATE_HANDSHAKE)
		{
			switch (header)
			{
			case TGCHeader::SET_VERIFY_KEY:
				SetPacketVerifyKey(packet.get<GCSetVerifyKeyPacket>()->verify_key());
				break;

			case TGCHeader::PHASE:
				CAccountConnector::__AuthState_RecvPhase(packet.get<GCPhasePacket>());
				return true;

			case TGCHeader::HANDSHAKE:
				CAccountConnector::__AuthState_RecvHandshake(packet.get<GCHandshakePacket>());
				break;

			case TGCHeader::PING:
				CAccountConnector::__AuthState_RecvPing();
				break;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
			case TGCHeader::KEY_AGREEMENT:
				CAccountConnector::__AuthState_RecvKeyAgreement(packet.get<GCPhasePacket>);
				break;

			case TGCHeader::KEY_AGREEMENT_COMPLETED:
				CAccountConnector::__AuthState_RecvKeyAgreementCompleted(packet.get<GCPhasePacket>);
				break;
#endif

			default:
				TraceError("Invalid header for handshake state %u", packet.get_header());
				break;
			}
		}
		else if (m_eState == STATE_AUTH)
		{
			switch (header)
			{
			case TGCHeader::PHASE:
				CAccountConnector::__AuthState_RecvPhase(packet.get<GCPhasePacket>());
				return true;

			case TGCHeader::PING:
				CAccountConnector::__AuthState_RecvPing();
				break;

			case TGCHeader::AUTH_SUCCESS:
				CAccountConnector::__AuthState_RecvAuthSuccess(packet.get<GCAuthSuccessPacket>());
				break;

			case TGCHeader::LOGIN_VERSION_ANSWER:
				CAccountConnector::__AuthState_RecvVersionAnswer(packet.get<GCLoginVersionAnswerPacket>());
				break;

			case TGCHeader::LOGIN_FAILURE:
				CAccountConnector::__AuthState_RecvAuthFailure(packet.get<GCLoginFailurePacket>());
				break;

			case TGCHeader::HANDSHAKE:
				CAccountConnector::__AuthState_RecvHandshake(packet.get<GCHandshakePacket>());
				break;

			default:
				TraceError("Invalid header for auth state %u", packet.get_header());
				break;

			}
		}
		else
		{
			TraceError("Invalid header for unknown state (%u) %u", m_eState, packet.get_header());
		}
	}

	return true;
}

bool CAccountConnector::__AuthState_RecvPhase(std::unique_ptr<GCPhasePacket> p)
{
	Tracenf("AccountConnector -> SetPhase %u", p->phase());

	if (p->phase() == PHASE_HANDSHAKE)
	{
		__HandshakeState_Set();
	}
	else if (p->phase() == PHASE_AUTH)
	{
	// *((int*)0) = 0;
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//		const char* key = LocaleService_GetSecurityKey();
//		SetSecurityMode(true, key);
//#endif

		network::CGOutputPacket<network::CGLoginVersionCheckPacket> Packet;

		Packet->set_version(g_szCurrentClientVersion);

		if (!Send(Packet))
			return false;

		__AuthState_Set();
	}

	return true;
}
extern inline int mb_encryptAuth(DWORD *dest, const DWORD *src, const DWORD key, int size);
bool CAccountConnector::__AuthState_RecvHandshake(std::unique_ptr<GCHandshakePacket> p)
{
	Tracenf("HANDSHAKE RECV %u", p->time());

	ELTimer_SetServerMSec(p->time());

	//DWORD dwBaseServerTime = kPacketHandshake.dwTime+ kPacketHandshake.lDelta;
	//DWORD dwBaseClientTime = ELTimer_GetMSec();

	network::CGOutputPacket<network::CGHandshakePacket> p2;
	p2->set_handshake(p->handshake());

	MTRandom rnd = MTRandom(p->crypt_key());
	DWORD keys[8];
	for (size_t i = 0; i < 8; i++)
		keys[i] = rnd.next();

	mb_encryptAuth(keys, keys, p->crypt_key(), sizeof(keys));
	p2->set_crypt_data(keys, sizeof(keys));

	Tracenf("HANDSHAKE SEND %u", p->time());

	if (!Send(p2))
	{
		Tracen(" CAccountConnector::__AuthState_RecvHandshake - SendHandshake Error");
		return false;
	}

	return true;
}

bool CAccountConnector::__AuthState_RecvPing()
{
	__AuthState_SendPong();
	return true;
}

bool CAccountConnector::__AuthState_SendPong()
{
	if (!Send(TCGHeader::PONG))
		return false;

	return true;
}

bool CAccountConnector::__AuthState_RecvVersionAnswer(std::unique_ptr<GCLoginVersionAnswerPacket> p)
{
	if (p->answer() == false)
	{
		if (m_poHandler)
		{
			PyCallClassMemberFunc(m_poHandler, "OnVersionCheckFailed", Py_BuildValue("()"));
		}
		return true;
	}

	network::CGOutputPacket<network::CGAuthLoginPacket> LoginPacket;
	LoginPacket->set_login(m_strID);
	LoginPacket->set_passwd(m_strPassword);
	LoginPacket->set_hwid(CPythonSystem::Instance().GetHWIDHash());
#ifdef ACCOUNT_TRADE_BLOCK
	char buf1[255], buf2[255];
	DWORD dwbuf1 = sizeof(buf1);
	DWORD dwbuf2 = sizeof(buf2);
	GetUserName(buf1, &dwbuf1);

	std::string message = std::string(buf1);
	message.erase(std::remove_if(message.begin(), message.end(), [](char c) { return !std::isalnum(c); } ), message.end());
	LoginPacket->set_user_name(message.c_str());

	GetComputerName(buf2, &dwbuf2);
	message = std::string(buf2);
	message.erase(std::remove_if(message.begin(), message.end(), [](char c) { return !std::isalnum(c); } ), message.end());
	LoginPacket->set_pc_name(message.c_str());

	LoginPacket->set_pc_name_real(pc_name);
	LoginPacket->set_user_name_real(user_name);
	LoginPacket->set_hash(pc_hash);
#endif
	LoginPacket->set_language(CLocaleManager::instance().GetLanguage());

	// 비밀번호를 메모리에 계속 갖고 있는 문제가 있어서, 사용 즉시 날리는 것으로 변경
	ClearLoginInfo();
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.ClearLoginInfo();

	m_strPassword = "";

	for (DWORD i = 0; i < 4; ++i)
		LoginPacket->add_client_keys(g_adwEncryptKey[i]);

	if (!Send(LoginPacket))
	{
		Tracen(" CAccountConnector::__AuthState_RecvPhase - SendLogin3 Error");
		return false;
	}

	return true;
}

bool CAccountConnector::__AuthState_RecvAuthSuccess(std::unique_ptr<GCAuthSuccessPacket> p)
{
	if (!p->result())
	{
		if (m_poHandler)
			PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(si)", "BESAMEKEY", 0));
	}
	else
	{
		CPythonNetworkStream & rkNet = CPythonNetworkStream::Instance();
		rkNet.SetLoginKey(p->login_key());
		rkNet.Connect(m_strAddr.c_str(), m_iPort);
	}

	Disconnect();
	__OfflineState_Set();

	return true;
}

bool CAccountConnector::__AuthState_RecvAuthFailure(std::unique_ptr<GCLoginFailurePacket> p)
{
	if (m_poHandler)
		PyCallClassMemberFunc(m_poHandler, "OnLoginFailure", Py_BuildValue("(si)", p->status().c_str(), p->data()));

	return true;
}

void CAccountConnector::__OfflineState_Set()
{
	__Inialize();
}

void CAccountConnector::__HandshakeState_Set()
{
	m_eState=STATE_HANDSHAKE;
}

void CAccountConnector::__AuthState_Set()
{
	m_eState=STATE_AUTH;
}

void CAccountConnector::OnConnectFailure()
{
	if (m_poHandler)
		PyCallClassMemberFunc(m_poHandler, "OnConnectFailure", Py_BuildValue("()"));

	__OfflineState_Set();
}

void CAccountConnector::OnConnectSuccess()
{
	m_eState = STATE_HANDSHAKE;
}

void CAccountConnector::OnRemoteDisconnect()
{
	if (m_isWaitKey && m_poHandler)
	{
		PyCallClassMemberFunc(m_poHandler, "OnExit", Py_BuildValue("()"));
		return;
	}

	__OfflineState_Set();
}

void CAccountConnector::OnDisconnect()
{
	__OfflineState_Set();
}

void CAccountConnector::__Inialize()
{
	m_eState=STATE_OFFLINE;
	m_isWaitKey = FALSE;
}

CAccountConnector::CAccountConnector()
{
	m_poHandler = NULL;
	m_strAddr = "";
	m_iPort = 0;

	SetLoginInfo("", "");
	SetRecvBufferSize(1024 * 128);
	SetSendBufferSize(2048);
	__Inialize();
}

CAccountConnector::~CAccountConnector()
{
	__OfflineState_Set();
}
