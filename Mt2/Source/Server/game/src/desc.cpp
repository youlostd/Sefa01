#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "protocol.h"
#include "packet.h"
#include "messenger_manager.h"
#include "sectree_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "sequence.h"
#include "guild.h"
#include "guild_manager.h"
#include "log.h"
#include "item_manager.h"
#include "mtrand.h"
#include "dungeon.h"

#include <google/protobuf/port_def.inc>

#ifdef ACCOUNT_TRADE_BLOCK
#include "questmanager.h"
#endif

DESC::DESC()
{
	Initialize();
}

DESC::~DESC()
{
}

void DESC::Initialize()
{
	m_bDestroyed = false;

	m_pInputProcessor = NULL;
	m_lpFdw = NULL;
	m_sock = INVALID_SOCKET;
	m_iPhase = PHASE_CLOSE;
	m_dwHandle = 0;

	m_wPort = 0;
	m_wListenPort = 0;
	m_LastTryToConnectTime = 0;

	m_lpInputBuffer = NULL;
	m_iMinInputBufferLen = 0;

	m_dwHandshake = 0;
	m_dwHandshakeSentTime = 0;
	m_iHandshakeRetry = 0;
	m_dwClientTime = 0;
	m_bHandshaking = false;

	m_lpBufferedOutputBuffer = NULL;
	m_lpOutputBuffer = NULL;

	m_pkPingEvent = NULL;
	m_lpCharacter = NULL;

	memset( &m_SockAddr, 0, sizeof(m_SockAddr) );

	m_pLogFile = NULL;
	m_dwRelayPID = 0;

//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//	m_bEncrypted = false;
//#endif

	m_wP2PPort = 0;
	m_bP2PChannel = 0;

	m_bAdminMode = false;
	m_bPong = true;

	m_pkLoginKey = NULL;
	m_dwLoginKey = 0;

//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//	memset( m_adwDecryptionKey, 0, sizeof(m_adwDecryptionKey) );
//	memset( m_adwEncryptionKey, 0, sizeof(m_adwEncryptionKey) );
//#endif

#ifdef __DEPRECATED_BILLING__
	m_dwBillingExpireSecond = 0;
#endif

	m_outtime = 0;
	m_playtime = 0;
	m_offtime = 0;

	m_pkDisconnectEvent = NULL;

	m_seq_vector.clear();

	m_bIsLoginAllow = false;

#ifdef __HAIR_SELECTOR__
	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		m_aiOldHairBase[i] = -1;
#endif

	m_lpvClientAuthKey = NULL;
	m_dwAuthCryptKey = 0;
	m_bIsDisconnecting = false;

	m_packetVerifyKeyIsSet = false;
}

void DESC::Destroy()
{
	if (m_bDestroyed) {
		return;
	}
	m_bDestroyed = true;

	if (m_pkLoginKey)
		m_pkLoginKey->Expire();

	if (GetAccountTable().id())
		DESC_MANAGER::instance().DisconnectAccount(GetAccountTable().login());

	if (m_pLogFile)
	{
		fclose(m_pLogFile);
		m_pLogFile = NULL;
	}

	if (m_lpCharacter)
	{
		m_lpCharacter->Disconnect("DESC::~DESC");
		m_lpCharacter = NULL;
	}

	SAFE_BUFFER_DELETE(m_lpOutputBuffer);
	SAFE_BUFFER_DELETE(m_lpInputBuffer);

	event_cancel(&m_pkPingEvent);
	event_cancel(&m_pkDisconnectEvent);

	if (!g_bAuthServer)
	{
		if (!m_accountTable.login().empty() && !m_accountTable.passwd().empty())
		{
			network::GDOutputPacket<network::GDLogoutPacket> pack;
			pack->set_login(m_accountTable.login());
			pack->set_passwd(m_accountTable.passwd());
			db_clientdesc->DBPacket(pack, m_dwHandle);
		}
	}

	if (m_sock != INVALID_SOCKET)
	{
		sys_log(0, "SYSTEM: closing socket. DESC #%d", m_sock);
		Log("SYSTEM: closing socket. DESC #%d", m_sock);
		fdwatch_del_fd(m_lpFdw, m_sock);

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		cipher_.CleanUp();
#endif

		socket_close(m_sock);
		m_sock = INVALID_SOCKET;
	}

	m_seq_vector.clear();

	if (m_lpvClientAuthKey)
		free(m_lpvClientAuthKey);
}

EVENTFUNC(ping_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "ping_event> <Factor> Null pointer" );
		return 0;
	}

	LPDESC desc = info->desc;

	if (desc->IsAdminMode())
		return (ping_event_second_cycle);

	if (!desc->IsPong() || desc->IsPhase(PHASE_HANDSHAKE))
	{
		sys_log(0, "PING_EVENT: no pong %s", desc->GetHostName());

		if (test_server)
			sys_err("PHASE_CLOSE %s : no pong", desc->GetAccountTable().login().c_str());

		if (desc->IsPhase(PHASE_HANDSHAKE))
			sys_err("no handshake response in time, dropping connection (ip: %s)", desc->GetHostName());

		desc->SetPhase(PHASE_CLOSE);
		return (ping_event_second_cycle);
	}
	else
	{
		desc->Packet(network::TGCHeader::PING);
		desc->SetPong(false);
	}

	//desc->SendHandshake(get_dword_time(), 0);

	return (ping_event_second_cycle);
}

bool DESC::IsPong()
{
	return m_bPong;
}

void DESC::SetPong(bool b)
{
	m_bPong = b;
}

bool DESC::Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in & c_rSockAddr, DWORD _handle, DWORD _handshake)
{
	m_lpFdw		= _fdw;
	m_sock		= _fd;

	m_stHost		= inet_ntoa(c_rSockAddr.sin_addr);
	m_wPort			= c_rSockAddr.sin_port;
	m_dwHandle		= _handle;

	m_lpOutputBuffer = buffer_new(g_iClientOutputBufferStartingSize);

	m_iMinInputBufferLen = MAX_INPUT_LEN >> 1;
	m_lpInputBuffer = buffer_new(MAX_INPUT_LEN);

	m_SockAddr = c_rSockAddr;

	fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_READ, false);

	// Ping Event 
	desc_event_info* info = AllocEventInfo<desc_event_info>();

	info->desc = this;

//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//	thecore_memcpy(m_adwEncryptionKey, "1234abcd5678efgh", sizeof(DWORD) * 4);
//	thecore_memcpy(m_adwDecryptionKey, "1234abcd5678efgh", sizeof(DWORD) * 4);
//#endif // _IMPROVED_PACKET_ENCRYPTION_

	// Set Phase to handshake
	SetPhase(PHASE_HANDSHAKE, true);
	StartHandshake(_handshake);

	assert(m_pkPingEvent == NULL);
	if (!guild_mark_server)
		m_pkPingEvent = event_create(ping_event, info, PASSES_PER_SEC(2));

	sys_log(0, "SYSTEM: new connection from [%s] fd: %d handshake %u output input_len %d, ptr %p",
			m_stHost.c_str(), m_sock, m_dwHandshake, buffer_size(m_lpInputBuffer), this);

	Log("SYSTEM: new connection from [%s] fd: %d handshake %u ptr %p", m_stHost.c_str(), m_sock, m_dwHandshake, this);
	return true;
}

int DESC::ProcessInput()
{
	ssize_t bytes_read;

	if (!m_lpInputBuffer)
	{
		sys_err("DESC::ProcessInput : nil input buffer");
		return -1;
	}

#ifdef P2P_SAFE_BUFFERING
	bool isFirst = true;
	do // read everything from os's network buffer
	{
		if (!isFirst)
			sys_log(0, "1 buffer adjust size was not enough, reading again");
#endif
		buffer_adjust_size(m_lpInputBuffer, m_iMinInputBufferLen);
		bytes_read = socket_read(m_sock, (char *)buffer_write_peek(m_lpInputBuffer), buffer_has_space(m_lpInputBuffer));

		if (bytes_read < 0)
			return -1;
		else if (bytes_read == 0
#ifdef P2P_SAFE_BUFFERING
			&& isFirst
#endif
			)
			return 0;

		buffer_write_proceed(m_lpInputBuffer, bytes_read);
#ifdef P2P_SAFE_BUFFERING
		isFirst = false;
	} while (buffer_has_space(m_lpInputBuffer) == 0);
#endif

	if (!m_pInputProcessor)
		sys_err("no input processor");
#ifdef _IMPROVED_PACKET_ENCRYPTION_
	else
	{
		if (cipher_.activated()) {
			cipher_.Decrypt(const_cast<void*>(buffer_read_peek(m_lpInputBuffer)), buffer_size(m_lpInputBuffer));
		}

		int iBytesProceed = 0;

		// false°¡ ¸®ÅÏ µÇ¸é ´Ù¸¥ phase·Î ¹Ù²ï °ÍÀÌ¹Ç·Î ´Ù½Ã ÇÁ·Î¼¼½º·Î µ¹ÀÔÇÑ´Ù!
		while (!m_pInputProcessor->Process(this, buffer_read_peek(m_lpInputBuffer), buffer_size(m_lpInputBuffer), iBytesProceed))
		{
			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
			iBytesProceed = 0;
		}

		buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
	}
#else
	else if (true/*!m_bEncrypted*/)
	{
		int iBytesProceed = 0;

		m_pInputProcessor->Process(this, buffer_read_peek(m_lpInputBuffer), buffer_size(m_lpInputBuffer), iBytesProceed);
		buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
		iBytesProceed = 0;
	}
	//else
	//{
	//	int iSizeBuffer = buffer_size(m_lpInputBuffer);

	//	// 8¹ÙÀÌÆ® ´ÜÀ§·Î¸¸ Ã³¸®ÇÑ´Ù. 8¹ÙÀÌÆ® ´ÜÀ§¿¡ ºÎÁ·ÇÏ¸é Àß¸øµÈ ¾ÏÈ£È­ ¹öÆÛ¸¦ º¹È£È­
	//	// ÇÒ °¡´É¼ºÀÌ ÀÖÀ¸¹Ç·Î Â©¶ó¼­ Ã³¸®ÇÏ±â·Î ÇÑ´Ù.
	//	if (iSizeBuffer & 7) // & 7Àº % 8°ú °°´Ù. 2ÀÇ ½Â¼ö¿¡¼­¸¸ °¡´É
	//		iSizeBuffer -= iSizeBuffer & 7;

	//	if (iSizeBuffer > 0)
	//	{
	//		TEMP_BUFFER	tempbuf;
	//		LPBUFFER lpBufferDecrypt = tempbuf.getptr();
	//		buffer_adjust_size(lpBufferDecrypt, iSizeBuffer);

	//		int iSizeAfter = TEA_Decrypt((DWORD *) buffer_write_peek(lpBufferDecrypt),
	//				(DWORD *) buffer_read_peek(m_lpInputBuffer),
	//				GetDecryptionKey(),
	//				iSizeBuffer);

	//		buffer_write_proceed(lpBufferDecrypt, iSizeAfter);

	//		int iBytesProceed = 0;

	//		// false°¡ ¸®ÅÏ µÇ¸é ´Ù¸¥ phase·Î ¹Ù²ï °ÍÀÌ¹Ç·Î ´Ù½Ã ÇÁ·Î¼¼½º·Î µ¹ÀÔÇÑ´Ù!
	//		while (!m_pInputProcessor->Process(this, buffer_read_peek(lpBufferDecrypt), buffer_size(lpBufferDecrypt), iBytesProceed))
	//		{
	//			if (iBytesProceed > iSizeBuffer)
	//			{
	//				buffer_read_proceed(m_lpInputBuffer, iSizeBuffer);
	//				iSizeBuffer = 0;
	//				iBytesProceed = 0;
	//				break;
	//			}

	//			buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
	//			iSizeBuffer -= iBytesProceed;

	//			buffer_read_proceed(lpBufferDecrypt, iBytesProceed);
	//			iBytesProceed = 0;
	//		}

	//		buffer_read_proceed(m_lpInputBuffer, iBytesProceed);
	//	}
	//}
#endif // _IMPROVED_PACKET_ENCRYPTION_

	return (bytes_read);
}

int DESC::ProcessOutput()
{
	if (buffer_size(m_lpOutputBuffer) <= 0)
		return 0;

	int buffer_left = fdwatch_get_buffer_size(m_lpFdw, m_sock);

	if (buffer_left <= 0)
		return 0;

	int bytes_to_write = MIN(buffer_left, buffer_size(m_lpOutputBuffer));

	if (bytes_to_write == 0)
		return 0;

	int result = socket_write(m_sock, (const char *) buffer_read_peek(m_lpOutputBuffer), bytes_to_write);

	if (result == 0)
	{
		//sys_log(0, "%d bytes written to %s first %u", bytes_to_write, GetHostName(), *(BYTE *) buffer_read_peek(m_lpOutputBuffer));
		//Log("%d bytes written", bytes_to_write);

		buffer_read_proceed(m_lpOutputBuffer, bytes_to_write);

		if (buffer_size(m_lpOutputBuffer) != 0)
			fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
	}

	return (result);
}

void DESC::Packet(network::TGCHeader header, const ::PROTOBUF_NAMESPACE_ID::Message* packet)
{
	network::TPacketHeader data;
	data.header = static_cast<uint16_t>(header);

	PacketSend(data, packet);
}

void DESC::P2PPacket(network::TGGHeader header, const ::PROTOBUF_NAMESPACE_ID::Message* packet)
{
	network::TPacketHeader data;
	data.header = static_cast<uint16_t>(header);

	PacketSend(data, packet);
}

void DESC::DirectPacket(network::TGCHeader header, const void* serializedData, size_t size)
{
	network::TPacketHeader fullHeader;
	fullHeader.header = static_cast<uint16_t>(header);
	fullHeader.size = sizeof(fullHeader) + size;

	bool send_verify = m_packetVerifyKeyIsSet && header != network::TGCHeader::SET_VERIFY_KEY;
	if (send_verify)
		fullHeader.size += sizeof(uint32_t);

	std::vector<uint8_t> data_array;
	data_array.resize(fullHeader.size);
	size_t data_array_pos = 0;

	thecore_memcpy(&data_array[data_array_pos], &fullHeader, sizeof(fullHeader));
	data_array_pos += sizeof(fullHeader);

	if (send_verify)
	{
		thecore_memcpy(&data_array[data_array_pos], &m_packetVerifyKey, sizeof(uint32_t));
		data_array_pos += sizeof(uint32_t);
	}

	if (size)
		thecore_memcpy(&data_array[data_array_pos], serializedData, size);

	PacketSend(&data_array[0], fullHeader.size);
}

void DESC::PacketSend(network::TPacketHeader& header, const ::PROTOBUF_NAMESPACE_ID::Message* data, bool is_large)
{
	if (m_stRelayName.length() != 0 || m_dwRelayPID != 0)
	{
		network::GGOutputPacket<network::GGRelayPacket> p;

		p->set_name(m_stRelayName);
		p->set_pid(m_dwRelayPID);

		p->set_relay_header(header.header);
		
		if (data)
		{
			std::vector<BYTE> buf;
			buf.resize(data->ByteSize());
			data->SerializeToArray(&buf[0], buf.size());
			p->set_relay(&buf[0], buf.size());
		}

		m_stRelayName.clear();
		m_dwRelayPID = 0;
		Packet(p);
	}
	else
	{
		auto extra_size = data ? data->ByteSize() : 0;
		header.size = sizeof(header) + extra_size;

		bool send_verify = m_packetVerifyKeyIsSet && header.header != static_cast<uint16_t>(network::TGCHeader::SET_VERIFY_KEY);
		if (send_verify)
			header.size += sizeof(uint32_t);

		std::vector<uint8_t> data_array;
		data_array.resize(header.size);
		size_t data_array_pos = 0;

		thecore_memcpy(&data_array[data_array_pos], &header, sizeof(header));
		data_array_pos += sizeof(header);

		if (send_verify)
		{
			thecore_memcpy(&data_array[data_array_pos], &m_packetVerifyKey, sizeof(uint32_t));
			data_array_pos += sizeof(uint32_t);
		}

		if (extra_size && !data->SerializeToArray(&data_array[data_array_pos], extra_size))
			return;

		PacketSend(&data_array[0], header.size, is_large);
	}
}

void DESC::PacketSend(const void * c_pvData, int iSize, bool is_large)
{
	assert(iSize > 0);

	if (m_iPhase == PHASE_CLOSE) // ²÷´Â »óÅÂ¸é º¸³»Áö ¾Ê´Â´Ù.
		return;

	if (is_large)
	{
		buffer_adjust_size(m_lpOutputBuffer, iSize);
		sys_log(0, "LargePacket Size %d", iSize, buffer_size(m_lpOutputBuffer));
	}

	{
		if (m_lpBufferedOutputBuffer)
		{
			buffer_write(m_lpBufferedOutputBuffer, c_pvData, iSize);

			c_pvData = buffer_read_peek(m_lpBufferedOutputBuffer);
			iSize = buffer_size(m_lpBufferedOutputBuffer);
		}

		if (test_server)
			sys_log(0, "DESC::Send %d size %u", *(uint16_t*) c_pvData, iSize);

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		void* buf = buffer_write_peek(m_lpOutputBuffer);

		
		if (packet_encode(m_lpOutputBuffer, c_pvData, iSize))
		{
			if (cipher_.activated()) {
				cipher_.Encrypt(buf, iSize);
			}
		}
		else
		{
			if (test_server)
				sys_err("PHASE_CLOSE %s : packet_encode", GetAccountTable().login().c_str());
			m_iPhase = PHASE_CLOSE;
		}
#else
		//if (!m_bEncrypted)
		{
			if (!packet_encode(m_lpOutputBuffer, c_pvData, iSize))
			{
				m_iPhase = PHASE_CLOSE;
			}
		}
		//else
		//{
		//	if (buffer_has_space(m_lpOutputBuffer) < iSize + 8)
		//	{
		//		sys_err("desc buffer mem_size overflow. memsize(%u) write_pos(%u) iSize(%d)", 
		//				m_lpOutputBuffer->mem_size, m_lpOutputBuffer->write_point_pos, iSize);

		//		m_iPhase = PHASE_CLOSE;
		//	}
		//	else
		//	{
		//		// ¾ÏÈ£È­¿¡ ÇÊ¿äÇÑ ÃæºÐÇÑ ¹öÆÛ Å©±â¸¦ È®º¸ÇÑ´Ù.
		//		/* buffer_adjust_size(m_lpOutputBuffer, iSize + 8); */
		//		DWORD * pdwWritePoint = (DWORD *) buffer_write_peek(m_lpOutputBuffer);

		//		if (packet_encode(m_lpOutputBuffer, c_pvData, iSize))
		//		{
		//			int iSize2 = TEA_Encrypt(pdwWritePoint, pdwWritePoint, GetEncryptionKey(), iSize);

		//			if (iSize2 > iSize)
		//				buffer_write_proceed(m_lpOutputBuffer, iSize2 - iSize);
		//		}
		//	}
		//}
#endif // _IMPROVED_PACKET_ENCRYPTION_

		SAFE_BUFFER_DELETE(m_lpBufferedOutputBuffer);
	}

	//sys_log(0, "%d bytes written (first byte %d)", iSize, *(BYTE *) c_pvData);
	if (m_iPhase != PHASE_CLOSE)
		fdwatch_add_fd(m_lpFdw, m_sock, this, FDW_WRITE, true);
}

void DESC::SetPhase(int _phase)
{
	SetPhase(_phase, false);
}

void DESC::SetPhase(int _phase, bool generate_verify_key)
{
	if (test_server)
		sys_log(0, "DESC[%p]::SetPhase %d gen_verify %d", this, _phase, generate_verify_key);

	m_iPhase = _phase;

	// Send verify packet key
	if (generate_verify_key)
	{
		GenerateVerifyPacketKey();
		network::GCOutputPacket<network::GCSetVerifyKeyPacket> packVerify;
		packVerify->set_verify_key(m_packetVerifyKey);
		Packet(packVerify);
	}

	network::GCOutputPacket<network::GCPhasePacket> pack;
	pack->set_phase(_phase);
	Packet(pack);

	switch (m_iPhase)
	{
		case PHASE_CLOSE:
			// ¸Þ½ÅÀú°¡ Ä³¸¯ÅÍ´ÜÀ§°¡ µÇ¸é¼­ »èÁ¦
			//MessengerManager::instance().Logout(GetAccountTable().login());
#ifdef __HAIR_SELECTOR__
			SaveHairBases();
#endif
			m_pInputProcessor = &m_inputClose;
			break;

		case PHASE_HANDSHAKE:
			m_pInputProcessor = &m_inputHandshake;
			break;

		case PHASE_SELECT:
			// ¸Þ½ÅÀú°¡ Ä³¸¯ÅÍ´ÜÀ§°¡ µÇ¸é¼­ »èÁ¦
			//MessengerManager::instance().Logout(GetAccountTable().login()); // ÀÇµµÀûÀ¸·Î break ¾È°Ë
		case PHASE_LOGIN:
		case PHASE_LOADING:
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//			m_bEncrypted = true;
//#endif
			m_pInputProcessor = &m_inputLogin;
			break;

		case PHASE_GAME:
		case PHASE_DEAD:
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//			m_bEncrypted = true;
//#endif
			m_pInputProcessor = &m_inputMain;
			break;

		case PHASE_AUTH:
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//			m_bEncrypted = true;
//#endif
			m_pInputProcessor = &m_inputAuth;
			sys_log(0, "AUTH_PHASE %p", this);
			break;
	}
}

void DESC::BindAccountTable(network::TAccountTable * pAccountTable)
{
	assert(pAccountTable != NULL);
	m_accountTable = *pAccountTable;
	DESC_MANAGER::instance().ConnectAccount(m_accountTable.login(), this);
}

void DESC::Log(const char * format, ...)
{
	if (!m_pLogFile)
		return;

	va_list args;

	time_t ct = get_global_time();
	struct tm tm = *localtime(&ct);

	fprintf(m_pLogFile,
			"%02d %02d %02d:%02d:%02d | ",
			tm.tm_mon + 1,
			tm.tm_mday,
			tm.tm_hour,
			tm.tm_min,
			tm.tm_sec);

	va_start(args, format);
	vfprintf(m_pLogFile, format, args);
	va_end(args);

	fputs("\n", m_pLogFile);

	fflush(m_pLogFile);
}

void DESC::StartHandshake(DWORD _handshake)
{
	// Handshake
	m_dwHandshake = _handshake;

	m_dwAuthCryptKey = MTRandom(time(NULL)).next();
	if (!m_lpvClientAuthKey)
		m_lpvClientAuthKey = malloc(4 * 8);

	MTRandom rnd = MTRandom(m_dwAuthCryptKey);
	DWORD key;
	for (size_t i = 0; i < 8; ++i)
	{
		key = rnd.next();
		memcpy((BYTE*)m_lpvClientAuthKey + i * 4, &key, 4);
	}
	mb_encrypt((DWORD*)m_lpvClientAuthKey, (DWORD*)m_lpvClientAuthKey, m_dwAuthCryptKey, 4 * 8);

	SendHandshake(get_dword_time(), 0);

	m_iHandshakeRetry = 0;
}

void DESC::SendHandshake(DWORD dwCurTime, int iNewDelta)
{
	network::GCOutputPacket<network::GCHandshakePacket> pack;
	pack->set_handshake(m_dwHandshake);
	pack->set_time(dwCurTime);
	pack->set_crypt_key(m_dwAuthCryptKey);

	Packet(pack);

	m_dwHandshakeSentTime = dwCurTime;
	m_bHandshaking = true;
}

bool DESC::HandshakeProcess(const void* cryptData, size_t dataLen)
{
	if (!m_lpvClientAuthKey)
	{
		SetPhase(PHASE_CLOSE);
		return false;
	}

	if (dataLen != 4 * 8 || memcmp(cryptData, m_lpvClientAuthKey, 4 * 8))
	{
		sys_err("handshake crypt data doesnt match for %s (len recv %zu)", m_stHost.c_str(), dataLen);
		//BanIP(m_SockAddr.sin_addr);
		SetPhase(PHASE_CLOSE);
		return false;
	}

	m_bHandshaking = false;
	return true;
}

bool DESC::IsHandshaking()
{
	return m_bHandshaking;
}

DWORD DESC::GetClientTime()
{
	return m_dwClientTime;
}

#ifdef _IMPROVED_PACKET_ENCRYPTION_
void DESC::SendKeyAgreement()
{
	TPacketKeyAgreement packet;

	size_t data_length = TPacketKeyAgreement::MAX_DATA_LEN;
	size_t agreed_length = cipher_.Prepare(packet.data, &data_length);
	if (agreed_length == 0) {
		// Initialization failure
		if (test_server)
			sys_err("PHASE_CLOSE %s : key agreement", GetAccountTable().login().c_str());
		SetPhase(PHASE_CLOSE);
		return;
	}
	assert(data_length <= TPacketKeyAgreement::MAX_DATA_LEN);

	packet.bHeader = HEADER_GC_KEY_AGREEMENT;
	packet.wAgreedLength = (WORD)agreed_length;
	packet.wDataLength = (WORD)data_length;

	Packet(&packet, sizeof(packet));
}

void DESC::SendKeyAgreementCompleted()
{
	TPacketKeyAgreementCompleted packet;

	packet.bHeader = HEADER_GC_KEY_AGREEMENT_COMPLETED;

	Packet(&packet, sizeof(packet));
}

bool DESC::FinishHandshake(size_t agreed_length, const void* buffer, size_t length)
{
	return cipher_.Activate(false, agreed_length, buffer, length);
}

bool DESC::IsCipherPrepared()
{
	return cipher_.IsKeyPrepared();
}
#endif // #ifdef _IMPROVED_PACKET_ENCRYPTION_

void DESC::SetRelay(const char * c_pszName)
{
	// sys_log(0, "SetRelay(%s)", c_pszName);
	m_stRelayName = c_pszName;
	m_dwRelayPID = 0;
}

void DESC::SetRelay(DWORD pid)
{
	m_stRelayName = "";
	m_dwRelayPID = pid;
}

void DESC::BindCharacter(LPCHARACTER ch)
{
	m_lpCharacter = ch;
}

void DESC::FlushOutput()
{
	if (m_sock == INVALID_SOCKET) {
		return;
	}

	if (buffer_size(m_lpOutputBuffer) <= 0)
		return;

	struct timeval sleep_tv, now_tv, start_tv;
	int event_triggered = false;

	gettimeofday(&start_tv, NULL);

	socket_block(m_sock);
	sys_log(0, "FLUSH START %d", buffer_size(m_lpOutputBuffer));

	while (buffer_size(m_lpOutputBuffer) > 0)
	{
		gettimeofday(&now_tv, NULL);

		int iSecondsPassed = now_tv.tv_sec - start_tv.tv_sec;

		if (iSecondsPassed > 10)
		{
			if (!event_triggered || iSecondsPassed > 20)
			{
				if (test_server)
					sys_err("PHASE_CLOSE %s : flush output", GetAccountTable().login().c_str());
				SetPhase(PHASE_CLOSE);
				break;
			}
		}

		sleep_tv.tv_sec = 0;
		sleep_tv.tv_usec = 10000;

		int num_events = fdwatch(m_lpFdw, &sleep_tv);

		if (num_events < 0)
		{
			sys_err("num_events < 0 : %d", num_events);
			break;
		}

		int event_idx;

		for (event_idx = 0; event_idx < num_events; ++event_idx)
		{
			LPDESC d2 = (LPDESC) fdwatch_get_client_data(m_lpFdw, event_idx);

			if (d2 != this)
				continue;

			switch (fdwatch_check_event(m_lpFdw, m_sock, event_idx))
			{
				case FDW_WRITE:
					event_triggered = true;

					if (ProcessOutput() < 0)
					{
						sys_err("Cannot flush output buffer");
						SetPhase(PHASE_CLOSE);
					}
					break;

				case FDW_EOF:
					if (test_server)
						sys_err("PHASE_CLOSE %s : eof", GetAccountTable().login().c_str());
					SetPhase(PHASE_CLOSE);
					break;
			}
		}

		if (IsPhase(PHASE_CLOSE))
			break;
	}

	if (buffer_size(m_lpOutputBuffer) == 0)
		sys_log(0, "FLUSH SUCCESS");
	else
		sys_log(0, "FLUSH FAIL");

	usleep(250000);
}

EVENTFUNC(disconnect_event)
{
	DESC::desc_event_info* info = dynamic_cast<DESC::desc_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "disconnect_event> <Factor> Null pointer" );
		return 0;
	}

	LPDESC d = info->desc;

	if (test_server)
		sys_err("PHASE_CLOSE %s : disconnect_event", d->GetAccountTable().login().c_str());

	d->m_pkDisconnectEvent = NULL;
	d->SetPhase(PHASE_CLOSE);
	return 0;
}

bool DESC::DelayedDisconnect(int iSec)
{
	if (m_pkDisconnectEvent != NULL) {
		return false;
	}

	desc_event_info* info = AllocEventInfo<desc_event_info>();
	info->desc = this;

	m_pkDisconnectEvent = event_create(disconnect_event, info, PASSES_PER_SEC(iSec));
	return true;
}

void DESC::DisconnectOfSameLogin()
{
	if (GetCharacter())
	{
		if (m_pkDisconnectEvent)
			return;

#ifdef ENABLE_ZODIAC_TEMPLE
		if (GetCharacter()->IsPrivateMap(ZODIAC_MAP_INDEX))
		{
			CDungeonManager::instance().RemovePlayerInfo(GetCharacter()->GetPlayerID());
		}
#endif
	
		GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetCharacter(), "´Ù¸¥ ÄÄÇ»ÅÍ¿¡¼­ ·Î±×ÀÎ ÇÏ¿© Á¢¼ÓÀ» Á¾·á ÇÕ´Ï´Ù."));
		DelayedDisconnect(5);
	}
	else
	{
		if (test_server)
			sys_err("PHASE_CLOSE %s : disconnect of same login", GetAccountTable().login().c_str());
		SetPhase(PHASE_CLOSE);
	}
}

void DESC::SetAdminMode()
{
	m_bAdminMode = true;
}

bool DESC::IsAdminMode()
{
	return m_bAdminMode;
}

void DESC::GenerateVerifyPacketKey()
{
	if (m_packetVerifyKeyIsSet)
		return;

	m_packetVerifyKey = thecore_random();
	m_packetVerifyKeyIsSet = true;
}

void DESC::SendLoginSuccessPacket()
{
	auto& rTable = GetAccountTable();

	network::GCOutputPacket<network::GCLoginSuccessPacket> p;
	p->set_handle(GetHandle());
	p->set_random_key(DESC_MANAGER::instance().MakeRandomKey(GetHandle()));
	for (auto& player : rTable.players())
		*p->add_players() = player;

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{   
		if (rTable.players_size() <= i)
		{
			sys_err("!!! rTable.players_size() <= %d?", i);
			sys_err("!!! rTable.players_size() <= %d?", i);
			break;
		}

		CGuild* g = CGuildManager::instance().GetLinkedGuild(rTable.players(i).id());

		if (g)
		{   
			p->mutable_players(i)->set_guild_id(g->GetID());
			p->mutable_players(i)->set_guild_name(g->GetName());
		}   
		else
		{
			p->mutable_players(i)->clear_guild_id();
			p->mutable_players(i)->clear_guild_name();
		}
	}

	Packet(p);
}

void DESC::SetLoginKey(DWORD dwKey)
{
	m_dwLoginKey = dwKey;
}

void DESC::SetLoginKey(CLoginKey * pkKey)
{
	m_pkLoginKey = pkKey;
	sys_log(0, "SetLoginKey %u", m_pkLoginKey->m_dwKey);
}

DWORD DESC::GetLoginKey()
{
	if (m_pkLoginKey)
		return m_pkLoginKey->m_dwKey;

	return m_dwLoginKey;
}

const BYTE* GetKey_20050304Myevan()
{   
	static bool bGenerated = false;
	static DWORD s_adwKey[1938]; 

	if (!bGenerated) 
	{
		bGenerated = true;
		DWORD seed = 1491971513; 

		for (UINT i = 0; i < BYTE(seed); ++i)
		{
			seed ^= 2148941891ul;
			seed += 3592385981ul;

			s_adwKey[i] = seed;
		}
	}

	return (const BYTE*)s_adwKey;
}

//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//void DESC::SetSecurityKey(const DWORD * c_pdwKey)
//{
//	const BYTE * c_pszKey = (const BYTE *) "JyTxtHljHJlVJHorRM301vf@4fvj10-v";
//
//	c_pszKey = GetKey_20050304Myevan() + 37;
//
//	thecore_memcpy(&m_adwDecryptionKey, c_pdwKey, 16);
//	TEA_Encrypt(&m_adwEncryptionKey[0], &m_adwDecryptionKey[0], (const DWORD *) c_pszKey, 16);
//
//	sys_log(0, "SetSecurityKey decrypt %u %u %u %u encrypt %u %u %u %u", 
//			m_adwDecryptionKey[0], m_adwDecryptionKey[1], m_adwDecryptionKey[2], m_adwDecryptionKey[3],
//			m_adwEncryptionKey[0], m_adwEncryptionKey[1], m_adwEncryptionKey[2], m_adwEncryptionKey[3]);
//}
//#endif // _IMPROVED_PACKET_ENCRYPTION_

#ifdef __DEPRECATED_BILLING__
void DESC::SetBillingExpireSecond(DWORD dwSec)
{
	m_dwBillingExpireSecond = dwSec;
}

DWORD DESC::GetBillingExpireSecond()
{
	return m_dwBillingExpireSecond;
}
#endif

void DESC::push_seq(BYTE hdr, BYTE seq)
{
	if (m_seq_vector.size()>=20)
	{
		m_seq_vector.erase(m_seq_vector.begin());
	}

	seq_t info = { hdr, seq };
	m_seq_vector.push_back(info);
}

BYTE DESC::GetEmpire()
{
	return m_accountTable.empire();
}

void DESC::ChatPacket(BYTE type, const char * format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	network::GCOutputPacket<network::GCChatPacket> pack_chat;
	pack_chat->set_type(type);
	pack_chat->set_empire(GetEmpire());
	pack_chat->set_message(chatbuf);
	Packet(pack_chat);
}

#ifdef __HAIR_SELECTOR__
DWORD DESC::GetOldHairBase(BYTE bIdx) const
{
	if (m_aiOldHairBase[bIdx] == -1)
		return m_accountTable.players(bIdx).hair_base_part();

	return (DWORD)m_aiOldHairBase[bIdx];
}

bool DESC::IsHairBaseChanged(BYTE bIdx) const
{
	if (m_accountTable.players_size() <= bIdx)
		return false;

	DWORD dwCur = m_accountTable.players(bIdx).hair_base_part();
	return GetOldHairBase(bIdx) != dwCur;
}

void DESC::SaveOldHairBase(BYTE bIdx)
{
	if (m_aiOldHairBase[bIdx] != -1)
		return;

	m_aiOldHairBase[bIdx] = m_accountTable.players(bIdx).hair_base_part();
}

void DESC::SaveHairBases()
{
	auto& r = m_accountTable;

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{
		if (IsHairBaseChanged(i))
		{
			char szQuery[512];

			auto c_pOldProto = GetOldHairBase(i) != 0 ? ITEM_MANAGER::instance().GetTable(GetOldHairBase(i)) : NULL;
			if ((!c_pOldProto && !r.players(i).hair_part()) || (c_pOldProto && c_pOldProto->values(3) == r.players(i).hair_part()))
			{
				auto c_pProto = ITEM_MANAGER::instance().GetTable(r.players(i).hair_base_part());
				snprintf(szQuery, sizeof(szQuery), "UPDATE player SET part_hair = %u, part_hair_base = %u WHERE id = %u",
					c_pProto ? c_pProto->values(3) : 0, r.players(i).hair_base_part(), r.players(i).id());
			}
			else
				snprintf(szQuery, sizeof(szQuery), "UPDATE player SET part_hair_base = %u WHERE id = %u", r.players(i).hair_base_part(), r.players(i).id());

			//if (i == pinfo->index)
			std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("%s", szQuery));
			//else
			//	DBManager::instance().Query("%s", szQuery);

			sys_log(0, "save_hair_selector : pid %u query %s", r.players(i).id(), szQuery);
			m_aiOldHairBase[i] = r.players(i).hair_base_part();
		}
	}
}
#endif
