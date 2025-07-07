#include "stdafx.h"
#include <sstream>

#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "buffer_manager.h"
#include "config.h"
#include "profiler.h"
#include "p2p.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "fishing.h"
#include "priv_manager.h"
#include "dev_log.h"
#ifdef P2P_SAFE_BUFFERING
#include "mtrand.h"
#endif

using namespace network;

extern time_t get_global_time();

CInputProcessor::CInputProcessor() : m_iBufferLeft(0)
{
}

bool CInputProcessor::Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed)
{
	const char * c_pData = (const char *) c_pvOrig;

	network::InputPacket packet;

	for (m_iBufferLeft = iBytes; m_iBufferLeft > 0;)
	{
/*		if (*c_pData == 0)
		{
			c_pData++;
			m_iBufferLeft--;
			r_iBytesProceed++;
			continue;
		}*/

		size_t headerSize = sizeof(network::TPacketHeader);
		if (lpDesc->HasVerifyPacketKey())
			headerSize += sizeof(uint32_t);

		if (m_iBufferLeft < headerSize)
			break;

		packet.header = *(network::TPacketHeader*) (c_pData);
		c_pData += sizeof(network::TPacketHeader);

		bool analyze_packet = true;

		if (lpDesc->HasVerifyPacketKey())
		{
			uint32_t verify_key = *(uint32_t*) c_pData;
			c_pData += sizeof(uint32_t);

			if (verify_key != lpDesc->GetVerifyPacketKey())
			{
				sys_err("Cannot analyze packet with header %u : invalid verify key[%s]", packet.get_header(), lpDesc->GetHostName());
				analyze_packet = false;
			}
		}

		if (packet.header.size < headerSize || m_iBufferLeft < packet.header.size)
		{
			sys_log(0, "wait for packet size %u (header %u)", packet.header.size, packet.get_header());
			return true;
		}

		sys_log(!test_server, "Recv packet from desc %p header %u size %u", this, packet.get_header(), packet.header.size);

		//sys_log(0, "<DEBUG> PACKET_INFO :: USER %s (%x) :: Header, Size, Sequence :: [%d, %d, %d]",
		//	(lpDesc && lpDesc->GetCharacter()) ? lpDesc->GetCharacter()->GetName() : "UNKNOWN",
		//	get_pointer(lpDesc),
		//	bHeader,
		//	iPacketLen,
		//	m_pPacketInfo->IsSequence(bHeader) ? 1 : 0);

		if (packet.get_header() && analyze_packet)
		{
			if (static_cast<TCGHeader>(packet.get_header()) != TCGHeader::MOVE && static_cast<TCGHeader>(packet.get_header()) != TCGHeader::PING_TIMER)
				sys_log(!test_server, "Packet Analyze [Header %d][bufferLeft %d][size %d] ", packet.get_header(), m_iBufferLeft, packet.header.size);

			packet.content = (const uint8_t*) c_pData;
			packet.content_size = packet.header.size - headerSize;
			if (!Analyze(lpDesc, packet))
			{
				sys_err("failed to Analyze packet [Header %d][bufferLeft %d][size %d]",
					packet.get_header(), m_iBufferLeft, packet.header.size);
			}
		}

		c_pData	+= packet.header.size - headerSize;

		m_iBufferLeft -= packet.header.size;
		r_iBytesProceed += packet.header.size;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

void CInputProcessor::Pong(LPDESC d)
{
	d->SetPong(true);
}

void CInputProcessor::Handshake(LPDESC d, std::unique_ptr<CGHandshakePacket> p)
{
	if (d->GetHandshake() != p->handshake())
	{
		sys_err("Invalid Handshake on %d", d->GetSocket());
		d->SetPhase(PHASE_CLOSE);
	}
	else
	{
		if (d->IsPhase(PHASE_HANDSHAKE))
		{
			if (d->HandshakeProcess(p->crypt_data().data(), p->crypt_data().length()))
			{
#ifdef _IMPROVED_PACKET_ENCRYPTION_
				d->SendKeyAgreement();
#else
				if (g_bAuthServer)
					d->SetPhase(PHASE_AUTH);
				else
					d->SetPhase(PHASE_LOGIN);
#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_
			}
		}
		/*else
			d->HandshakeProcess(p->time(), p->delta(), true);*/
	}
}

void CInputProcessor::Version(LPCHARACTER ch, std::unique_ptr<CGClientVersionPacket> p)
{
	if (!ch)
		return;

	sys_log(0, "VERSION: %s %s %s", ch->GetName(), p->timestamp().c_str(), p->filename().c_str());
	ch->GetDesc()->SetClientVersion(p->timestamp().c_str());
}

void LoginFailure(LPDESC d, const char * c_pszStatus, int iData)
{
	if (!d)
		return;

	network::GCOutputPacket<network::GCLoginFailurePacket> failurePacket;

	failurePacket->set_status(c_pszStatus);
	failurePacket->set_data(iData);

	d->Packet(failurePacket);
}

std::vector<TPlayerTable> g_vec_save;

// BLOCK_CHAT
ACMD(do_block_chat);
// END_OF_BLOCK_CHAT

bool CInputHandshake::Analyze(LPDESC d, const network::InputPacket& packet)
{
	if (test_server)
		sys_log(0, "Handshake[%p]: Analyze %u", d, packet.get_header());
	
	auto header = packet.get_header<TCGHeader>();

	if (header == TCGHeader::MARK_LOGIN)
	{
		if (!guild_mark_server)
		{
			// ²÷¾î¹ö·Á! - ¸¶Å© ¼­¹ö°¡ ¾Æ´Ñµ¥ ¸¶Å©¸¦ ¿äÃ»ÇÏ·Á°í?
			sys_err("Guild Mark login requested but i'm not a mark server!");
			d->SetPhase(PHASE_CLOSE);
			return 0;
		}

		// ¹«Á¶°Ç ÀÎÁõ --;
		sys_log(0, "MARK_SERVER: Login");
		d->SetPhase(PHASE_LOGIN);
		return 0;
	}
	else if (header == TCGHeader::PONG)
		Pong(d);
	else if (header == TCGHeader::CLIENT_VERSION)
		Version(d->GetCharacter(), packet.get<CGClientVersionPacket>());	
	else if (header == TCGHeader::HANDSHAKE)
		Handshake(d, packet.get<CGHandshakePacket>());
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else if (bHeader == TCGHeader::KEY_AGREEMENT)
	{
		// Send out the key agreement completion packet first
		// to help client to enter encryption mode
		d->SendKeyAgreementCompleted();
		// Flush socket output before going encrypted
		d->ProcessOutput();

		TPacketKeyAgreement* p = (TPacketKeyAgreement*)c_pData;
		if (!d->IsCipherPrepared())
		{
			sys_err ("Cipher isn't prepared. %s maybe a Hacker.", inet_ntoa(d->GetAddr().sin_addr));
			d->DelayedDisconnect(5);
			return 0;
		}
		if (d->FinishHandshake(p->wAgreedLength, p->data, p->wDataLength)) {
			// Handshaking succeeded
			if (g_bAuthServer) {
				d->SetPhase(PHASE_AUTH);
			} else {
				d->SetPhase(PHASE_LOGIN);
			}
		} else {
			sys_log(0, "[CInputHandshake] Key agreement failed: al=%u dl=%u",
				p->wAgreedLength, p->wDataLength);
			d->SetPhase(PHASE_CLOSE);
		}
	}
#endif // _IMPROVED_PACKET_ENCRYPTION_
	else if (header != TCGHeader::CHAT)
	{
		sys_err("Handshake phase does not handle packet %d (fd %d)", packet.get_header(), d->GetSocket());
		return false;
	}

	return true;
}
