#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"

extern DWORD g_adwEncryptKey[4];
extern DWORD g_adwDecryptKey[4];

using namespace network;

// Select Character ---------------------------------------------------------------------------
void CPythonNetworkStream::SetSelectPhase()
{
	if ("Select" != m_strPhase)
		m_phaseLeaveFunc.Run();

	Tracen("");
	Tracen("## Network - Select Phase ##");
	Tracen("");

	m_strPhase = "Select";

#ifndef _IMPROVED_PACKET_ENCRYPTION_
	//SetSecurityMode(true, (const char *) g_adwEncryptKey, (const char *) g_adwDecryptKey);
#endif

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::SelectPhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveSelectPhase);

	if (__DirectEnterMode_IsSet())
	{
		PyCallClassMemberFunc(m_poHandler, "SetLoadingPhase", Py_BuildValue("()"));
	}
	else
	{
		if (IsSelectedEmpire())
			PyCallClassMemberFunc(m_poHandler, "SetSelectCharacterPhase", Py_BuildValue("()"));
		else
			PyCallClassMemberFunc(m_poHandler, "SetSelectEmpirePhase", Py_BuildValue("()"));
	}
}

void CPythonNetworkStream::SelectPhase()
{
	InputPacket packet;

	while (Recv(packet))
	{
		switch (packet.get_header<TGCHeader>())
		{
			case TGCHeader::SET_VERIFY_KEY:
				SetPacketVerifyKey(packet.get<GCSetVerifyKeyPacket>()->verify_key());
				break;

			case TGCHeader::PHASE:
				RecvPhasePacket(packet.get<GCPhasePacket>());
				return;

			case TGCHeader::EMPIRE:
				__RecvEmpirePacket(packet.get<GCEmpirePacket>());
				break;

			case TGCHeader::LOGIN_SUCCESS:
				__RecvLoginSuccessPacket(packet.get<GCLoginSuccessPacket>());
				break;

			case TGCHeader::PLAYER_CREATE_SUCCESS:
				__RecvPlayerCreateSuccessPacket(packet.get<GCPlayerCreateSuccessPacket>());
				break;

			case TGCHeader::CREATE_FAILURE:
				__RecvPlayerCreateFailurePacket(packet.get<GCCreateFailurePacket>());
				break;

			case TGCHeader::PLAYER_DELETE_SUCCESS:
				__RecvPlayerDestroySuccessPacket(packet.get<GCDeleteSuccessPacket>());
				break;

			case TGCHeader::DELETE_FAILURE:
				__RecvPlayerDestroyFailurePacket();
				break;

			case TGCHeader::CHANGE_NAME:
				__RecvChangeName(packet.get<GCChangeNamePacket>());
				break;

			case TGCHeader::HANDSHAKE:
				RecvHandshakePacket(packet.get<GCHandshakePacket>());
				break;

			case TGCHeader::HANDSHAKE_OK:
				RecvHandshakeOKPacket();
				break;

		/*	case TGCHeader::HYBRIDCRYPT_KEYS:
				RecvHybridCryptKeyPacket(packet.get<GCHybridCryptKeysPacket>());
				break;

			case TGCHeader::HYBRIDCRYPT_SDB:
				RecvHybridCryptSDBPacket(packet.get<GCHybridCryptSDBPacket>());
				break;*/

#ifdef _IMPROVED_PACKET_ENCRYPTION_
			case TGCHeader::KEY_AGREEMENT:
				RecvKeyAgreementPacket(packet.get<GCKeyAgreementPacket>());
				break;

			case TGCHeader::KEY_AGREEMENT_COMPLETED:
				RecvKeyAgreementCompletedPacket();
				break;
#endif

			case TGCHeader::PING:
				RecvPingPacket();
				break;

			case TGCHeader::POINTS:
			case TGCHeader::POINT_CHANGE:
				break;

			case TGCHeader::INVENTORY_MAX_NUM:
				RecvInventoryMaxNum(packet.get<GCInventoryMaxNumPacket>());
				break;

			default:
				RecvErrorPacket(packet);
				break;
		}

		if (packet.get_header<TGCHeader>() == TGCHeader::PHASE)
			break;
	}
}

bool CPythonNetworkStream::SendSelectEmpirePacket(DWORD dwEmpireID)
{
	CGOutputPacket<CGEmpirePacket> pack;
	pack->set_empire(dwEmpireID);

	if (!Send(pack))
	{
		Tracen("SendSelectEmpirePacket - Error");
		return false;
	}

	SetEmpireID(dwEmpireID);
	return true;
}

bool CPythonNetworkStream::SendSelectCharacterPacket(BYTE Index)
{
	CGOutputPacket<CGPlayerSelectPacket> pack;
	pack->set_index(Index);

	if (!Send(pack))
	{
		Tracen("SendSelectCharacterPacket - Error");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendDestroyCharacterPacket(BYTE index, const char * szPrivateCode)
{
	CGOutputPacket<CGPlayerDeletePacket> pack;
	pack->set_index(index);
	pack->set_private_code(szPrivateCode);

	if (!Send(pack))
	{
		Tracen("SendDestroyCharacterPacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendCreateCharacterPacket(BYTE index, const char *name, BYTE job, BYTE shape, BYTE byCON, BYTE byINT, BYTE bySTR, BYTE byDEX)
{
	CGOutputPacket<CGPlayerCreatePacket> pack;
	
	pack->set_index(index);
	pack->set_name(name);
	pack->set_job(job);
	pack->set_shape(shape);
	pack->set_con(byCON);
	pack->set_int_(byINT);
	pack->set_str(bySTR);
	pack->set_dex(byDEX);

	if (!Send(pack))
	{
		Tracen("Failed to SendCreateCharacterPacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendChangeNamePacket(BYTE index, const char *name)
{
	CGOutputPacket<CGPlayerChangeNamePacket> pack;
	pack->set_index(index);
	pack->set_name(name);

	if (!Send(pack))
	{
		Tracen("Failed to SendChangeNamePacket");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::__RecvPlayerCreateSuccessPacket(std::unique_ptr<GCPlayerCreateSuccessPacket> pack)
{
	if (pack->account_index()>=PLAYER_PER_ACCOUNT4)
	{
		TraceError("CPythonNetworkStream::RecvPlayerCreateSuccessPacket - OUT OF RANGE SLOT(%d) > PLATER_PER_ACCOUNT(%d)",
			pack->account_index(), PLAYER_PER_ACCOUNT4);
		return true;
	}

	m_akSimplePlayerInfo[pack->account_index()]=pack->player();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateSuccess", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::__RecvPlayerCreateFailurePacket(std::unique_ptr<GCCreateFailurePacket> pack)
{
	
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_CREATE], "OnCreateFailure", Py_BuildValue("(i)", pack->type()));
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", pack->type()));
	return true;
}

bool CPythonNetworkStream::__RecvPlayerDestroySuccessPacket(std::unique_ptr<GCDeleteSuccessPacket> pack)
{
	m_akSimplePlayerInfo[pack->account_index()].Clear();

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteSuccess", Py_BuildValue("(i)", pack->account_index()));
	return true;
}

bool CPythonNetworkStream::__RecvPlayerDestroyFailurePacket()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnDeleteFailure", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::__RecvChangeName(std::unique_ptr<GCChangeNamePacket> pack)
{
	for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
	{
		if (pack->pid() == m_akSimplePlayerInfo[i].id())
		{
			m_akSimplePlayerInfo[i].set_change_name(FALSE);
			m_akSimplePlayerInfo[i].set_name(pack->name());

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnChangeName", Py_BuildValue("(is)", i, pack->name().c_str()));
			return true;
		}
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_SELECT], "OnCreateFailure", Py_BuildValue("(i)", 100));
	return true;
}
