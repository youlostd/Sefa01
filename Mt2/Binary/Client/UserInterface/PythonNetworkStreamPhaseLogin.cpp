#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"
#include "Test.h"
#include "AccountConnector.h"
#include "PythonConfig.h"

using namespace network;

// Login ---------------------------------------------------------------------------
void CPythonNetworkStream::LoginPhase()
{
	InputPacket packet;

	if (Recv(packet))
	{
		switch (packet.get_header<TGCHeader>())
		{
		case TGCHeader::PHASE:
			RecvPhasePacket(packet.get<GCPhasePacket>());
			break;

		case TGCHeader::LOGIN_SUCCESS:
			__RecvLoginSuccessPacket(packet.get<GCLoginSuccessPacket>());
			break;


		case TGCHeader::LOGIN_FAILURE:
			__RecvLoginFailurePacket(packet.get<GCLoginFailurePacket>());
			break;

		case TGCHeader::EMPIRE:
			__RecvEmpirePacket(packet.get<GCEmpirePacket>());
			break;

		case TGCHeader::PING:
			RecvPingPacket();
			break;

		default:
			RecvErrorPacket(packet);
			break;
		}
	}
}

void CPythonNetworkStream::SetLoginPhase()
{
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//	const char* key = LocaleService_GetSecurityKey();
//	SetSecurityMode(true, key);
//#endif

	if ("Login" != m_strPhase)
		m_phaseLeaveFunc.Run();

#ifdef _USE_LOG_FILE
	Tracen("");
#endif
#ifdef _USE_LOG_FILE
	Tracen("## Network - Login Phase ##");
#endif
#ifdef _USE_LOG_FILE
	Tracen("");
#endif

	m_strPhase = "Login";	

	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::LoginPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveLoginPhase);

	m_dwChangingPhaseTime = ELTimer_GetMSec();

	if (__DirectEnterMode_IsSet())
	{
		// if (0 != m_dwLoginKey)
		SendLoginPacketNew(m_stID.c_str(), m_stPassword.c_str());

		// 비밀번호를 메모리에 계속 갖고 있는 문제가 있어서, 사용 즉시 날리는 것으로 변경
		ClearLoginInfo();
		CAccountConnector & rkAccountConnector = CAccountConnector::Instance();
		rkAccountConnector.ClearLoginInfo();
	}
	else
	{
		// if (0 != m_dwLoginKey)
		SendLoginPacketNew(m_stID.c_str(), m_stPassword.c_str());

		// 비밀번호를 메모리에 계속 갖고 있는 문제가 있어서, 사용 즉시 날리는 것으로 변경
		ClearLoginInfo();
		CAccountConnector & rkAccountConnector = CAccountConnector::Instance();
		rkAccountConnector.ClearLoginInfo();

		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginStart", Py_BuildValue("()"));

		__ClearSelectCharacterData();
	}
}

bool CPythonNetworkStream::__RecvEmpirePacket(std::unique_ptr<network::GCEmpirePacket> pack)
{
	m_dwEmpireID=pack->empire();
	return true;
}

bool CPythonNetworkStream::__RecvLoginSuccessPacket(std::unique_ptr<network::GCLoginSuccessPacket> pack)
{	
	for (int i = 0; i<PLAYER_PER_ACCOUNT4; ++i)
		m_akSimplePlayerInfo[i]=pack->players(i);

	m_kMarkAuth.m_dwHandle=pack->handle();
	m_kMarkAuth.m_dwRandomKey=pack->random_key();	

	if (__DirectEnterMode_IsSet())
	{
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "Refresh", Py_BuildValue("()"));		
	}

	return true;
}


void CPythonNetworkStream::OnConnectFailure()
{
	if (__DirectEnterMode_IsSet())
	{
		ClosePhase();
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnConnectFailure", Py_BuildValue("()"));	
	}
}


bool CPythonNetworkStream::__RecvLoginFailurePacket(std::unique_ptr<GCLoginFailurePacket> pack)
{
#ifdef ENABLE_IPV6_FIX
	if (!stricmp(pack->status().c_str(), "ALREADY"))
	{
		DWORD dwWarpTime = CPythonNetworkStream::Instance().GetLastWarpTime();
		if (dwWarpTime && time(0) - dwWarpTime < 5)
			CPythonConfig::Instance().Write(CPythonConfig::CLASS_GENERAL, "ipv6_fix_enable", 1);
	}
#endif

	TraceError("LoginFailure: %s", pack->status().c_str());

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_LOGIN], "OnLoginFailure", Py_BuildValue("(si)", pack->status().c_str(), pack->data()));
#ifdef _DEBUG
	Tracef(" RecvLoginFailurePacket : [%s]\n", pack->status());
#endif
	return true;
}

bool CPythonNetworkStream::SendLoginPacketNew(const char * c_szName, const char * c_szPassword)
{
	CGOutputPacket<CGLoginByKeyPacket> pack;
	pack->set_login_key(m_dwLoginKey);
	pack->set_login(c_szName);

	extern DWORD g_adwEncryptKey[4];
	extern DWORD g_adwDecryptKey[4];
	for (DWORD i = 0; i < 4; ++i)
		pack->add_client_key(g_adwEncryptKey[i]);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendLogin Error");
#endif
		return false;
	}

	__SendInternalBuffer();

//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//	SetSecurityMode(true, (const char *) g_adwEncryptKey, (const char *) g_adwDecryptKey);
//#endif
	return true;
}
