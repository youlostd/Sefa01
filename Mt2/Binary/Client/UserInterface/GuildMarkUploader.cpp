#include "StdAfx.h"
#include "GuildMarkUploader.h"
#include "Packet.h"
#include "Test.h"
#include "../EterPack/mtrand.h"

using namespace network;

CGuildMarkUploader::CGuildMarkUploader()
 : m_pbySymbolBuf(NULL)
{
	SetRecvBufferSize(1024);
	SetSendBufferSize(1024);
	__Inialize();
}

CGuildMarkUploader::~CGuildMarkUploader()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::Disconnect()
{
	__OfflineState_Set();
}

bool CGuildMarkUploader::IsCompleteUploading()
{
	return STATE_OFFLINE == m_eState;
}

bool CGuildMarkUploader::__Save(const char* c_szFileName)
{
	/* 업로더에서 저장하지 않아야 함;
	ILuint uImg;
	ilGenImages(1, &uImg);
	ilBindImage(uImg);
	ilEnable(IL_FILE_OVERWRITE);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

	if (!ilLoad(IL_TYPE_UNKNOWN, (const ILstring)c_szFileName))
	{
		return false;
	}

	if (ilGetInteger(IL_IMAGE_WIDTH) != CGuildMarkImage::WIDTH)
	{
		return false;
	}

	if (ilGetInteger(IL_IMAGE_HEIGHT) != CGuildMarkImage::HEIGHT)
	{
		return false;
	}

	ilConvertImage(IL_BGRA, IL_BYTE);

	UINT uColCount = CGuildMarkImage::MARK_COL_COUNT;
	UINT uCol = m_dwGuildID % uColCount;
	UINT uRow = m_dwGuildID / uColCount;

	ilSetPixels(uCol*SGuildMark::WIDTH, uRow*SGuildMark::HEIGHT, 0, SGuildMark::WIDTH, SGuildMark::HEIGHT, 1, IL_BGRA, IL_BYTE, (ILvoid*)m_kMark.m_apxBuf);

	ilSave(IL_TGA, (ILstring)c_szFileName);

	ilDeleteImages(1, &uImg);
	*/
	return true;
}

bool CGuildMarkUploader::__Load(const char* c_szFileName, UINT* peError)
{
	ILuint uImg;
	ilGenImages(1, &uImg);
	ilBindImage(uImg);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);

	if (!ilLoad(IL_TYPE_UNKNOWN, (const ILstring)c_szFileName))
	{
		*peError=ERROR_LOAD;
		return false;
	}

	if (ilGetInteger(IL_IMAGE_WIDTH)!=SGuildMark::WIDTH)
	{
		*peError=ERROR_WIDTH;
		return false;
	}

	if (ilGetInteger(IL_IMAGE_HEIGHT)!=SGuildMark::HEIGHT)
	{
		*peError=ERROR_HEIGHT;
		return false;
	}

	ilConvertImage(IL_BGRA, IL_BYTE);
	
	ilCopyPixels(0, 0, 0, SGuildMark::WIDTH, SGuildMark::HEIGHT, 1, IL_BGRA, IL_UNSIGNED_BYTE, m_kMark.m_apxBuf);

	ilDeleteImages(1, &uImg);
	return true;
}

bool CGuildMarkUploader::__LoadSymbol(const char* c_szFileName, UINT* peError)
{
	//	For Check Image
	ILuint uImg;
	ilGenImages(1, &uImg);
	ilBindImage(uImg);
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
	if (!ilLoad(IL_TYPE_UNKNOWN, (const ILstring)c_szFileName))
	{
		*peError=ERROR_LOAD;
		return false;
	}
	if (ilGetInteger(IL_IMAGE_WIDTH) != 64)
	{
		*peError=ERROR_WIDTH;
		return false;
	}
	if (ilGetInteger(IL_IMAGE_HEIGHT) != 128)
	{
		*peError=ERROR_HEIGHT;
		return false;
	}
	ilDeleteImages(1, &uImg);
	ilShutDown();

	/////

	FILE * file = fopen(c_szFileName, "rb");
	if (!file)
	{
		*peError=ERROR_LOAD;
	}

	fseek(file, 0, SEEK_END);
	m_dwSymbolBufSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	m_pbySymbolBuf = new BYTE [m_dwSymbolBufSize];
	fread(m_pbySymbolBuf, m_dwSymbolBufSize, 1, file);

	fclose(file);

	/////

	m_dwSymbolCRC32 = GetFileCRC32(c_szFileName);
	return true;
}

bool CGuildMarkUploader::Connect(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey, DWORD dwGuildID, const char* c_szFileName, UINT* peError)
{
	__OfflineState_Set();
	SetRecvBufferSize(1024);
	SetSendBufferSize(1024);

	if (!CNetworkStream::Connect(c_rkNetAddr))
	{
		*peError=ERROR_CONNECT;
		return false;
	}

	m_dwSendType=SEND_TYPE_MARK;
	m_dwHandle=dwHandle;
	m_dwRandomKey=dwRandomKey;
	m_dwGuildID=dwGuildID;

	if (!__Load(c_szFileName, peError))
		return false;

	//if (!__Save(CGraphicMarkInstance::GetImageFileName().c_str()))
	//	return false;
	//CGraphicMarkInstance::ReloadImageFile();
	return true;
}

bool CGuildMarkUploader::ConnectToSendSymbol(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey, DWORD dwGuildID, const char* c_szFileName, UINT* peError)
{
	__OfflineState_Set();
	SetRecvBufferSize(1024);
	SetSendBufferSize(64*1024);

	if (!CNetworkStream::Connect(c_rkNetAddr))
	{
		*peError=ERROR_CONNECT;
		return false;
	}

	m_dwSendType=SEND_TYPE_SYMBOL;
	m_dwHandle=dwHandle;
	m_dwRandomKey=dwRandomKey;
	m_dwGuildID=dwGuildID;

	if (!__LoadSymbol(c_szFileName, peError))
		return false;

	return true;
}

void CGuildMarkUploader::Process()
{
	CNetworkStream::Process();

	if (!__StateProcess())
	{
		__OfflineState_Set();
		Disconnect();
	}
}

void CGuildMarkUploader::OnConnectFailure()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::OnConnectSuccess()
{
	__LoginState_Set();
}

void CGuildMarkUploader::OnRemoteDisconnect()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::OnDisconnect()
{
	__OfflineState_Set();
}

void CGuildMarkUploader::__Inialize()
{
	m_eState = STATE_OFFLINE;

	m_dwGuildID = 0;
	m_dwHandle = 0;
	m_dwRandomKey = 0;

	if (m_pbySymbolBuf)
	{
		delete m_pbySymbolBuf;
	}

	m_dwSymbolBufSize = 0;
	m_pbySymbolBuf = NULL;
}

bool CGuildMarkUploader::__StateProcess()
{
	switch (m_eState)
	{
		case STATE_LOGIN:
			return __LoginState_Process();
			break;
	}

	return true;
}

void CGuildMarkUploader::__OfflineState_Set()
{
	__Inialize();
}

void CGuildMarkUploader::__CompleteState_Set()
{
	m_eState=STATE_COMPLETE;

	__OfflineState_Set();
}


void CGuildMarkUploader::__LoginState_Set()
{
	m_eState=STATE_LOGIN;
}

bool CGuildMarkUploader::__LoginState_Process()
{
	InputPacket packet;
	while (Recv(packet))
	{
		switch (packet.get_header<TGCHeader>())
		{
			case TGCHeader::SET_VERIFY_KEY:
				SetPacketVerifyKey(packet.get<GCSetVerifyKeyPacket>()->verify_key());
				return true;

			case TGCHeader::PHASE:
				return __LoginState_RecvPhase(packet.get<GCPhasePacket>());

			case TGCHeader::HANDSHAKE:
				return __LoginState_RecvHandshake(packet.get<GCHandshakePacket>());

			case TGCHeader::PING:
				return __LoginState_RecvPing();

#ifdef _IMPROVED_PACKET_ENCRYPTION_
			case TGCHeader::KEY_AGREEMENT:
				return __LoginState_RecvKeyAgreement(packet.get<GCKeyAgreementPacket>());

			case TGCHeader::KEY_AGREEMENT_COMPLETED:
				return __LoginState_RecvKeyAgreementCompleted();
#endif
		}

		return false;
	}

	return true;
}

bool CGuildMarkUploader::__SendMarkPacket()
{
	CGOutputPacket<CGMarkUploadPacket> pack;
	pack->set_guild_id(m_dwGuildID);
	pack->set_image(m_kMark.m_apxBuf, sizeof(m_kMark.m_apxBuf));

	if (!Send(pack))
		return false;

	return true;
}
bool CGuildMarkUploader::__SendSymbolPacket()
{
	if (!m_pbySymbolBuf)
		return false;

	CGOutputPacket<CGGuildSymbolUploadPacket> pack;
	pack->set_guild_id(m_dwGuildID);
	pack->set_image(m_pbySymbolBuf, m_dwSymbolBufSize);

	if (!Send(pack))
		return false;

	CNetworkStream::__SendInternalBuffer();
	__CompleteState_Set();

	return true;
}

bool CGuildMarkUploader::__LoginState_RecvPhase(std::unique_ptr<GCPhasePacket> pack)
{
	if (pack->phase() == PHASE_LOGIN)
	{
#ifndef _IMPROVED_PACKET_ENCRYPTION_
	//	const char* key = LocaleService_GetSecurityKey();
	//	SetSecurityMode(true, key);
#endif

		if (SEND_TYPE_MARK == m_dwSendType)
		{
			if (!__SendMarkPacket())
				return false;
		}
		else if (SEND_TYPE_SYMBOL == m_dwSendType)
		{
			if (!__SendSymbolPacket())
				return false;
		}
	}

	return true;
}

extern int mb_encryptAuth(DWORD* dest, const DWORD* src, const DWORD key, int size);
bool CGuildMarkUploader::__LoginState_RecvHandshake(std::unique_ptr<GCHandshakePacket> pack)
{
	CGOutputPacket<CGHandshakePacket> pack_send;
	pack_send->set_handshake(pack->handshake());

	MTRandom rnd = MTRandom(pack->crypt_key());
	DWORD keys[8];
	for (size_t i = 0; i < 8; i++)
		keys[i] = rnd.next();

	mb_encryptAuth(keys, keys, pack->crypt_key(), sizeof(keys));
	pack_send->set_crypt_data(keys, sizeof(keys));

	if (!Send(pack_send))
	{
		Tracen(" CAccountConnector::__AuthState_RecvHandshake - SendHandshake Error");
		return false;
	}

	return true;
}

bool CGuildMarkUploader::__LoginState_RecvPing()
{
	if (!Send(TCGHeader::PONG))
		return false;

	return true;
}

#ifdef _IMPROVED_PACKET_ENCRYPTION_
bool CGuildMarkUploader::__LoginState_RecvKeyAgreement(std::unique_ptr<GCKeyAgreementPacket> pack)
{
	Tracenf("KEY_AGREEMENT RECV %u", pack->data().length());

	char dataToSend[KEY_AGREEMENT_MAX_LEN];
	size_t dataLength = KEY_AGREEMENT_MAX_LEN;
	size_t agreedLength = Prepare(dataToSend, &dataLength);
	if (agreedLength == 0)
	{
		// 초기화 실패
		Disconnect();
		return false;
	}
	assert(dataLength <= KEY_AGREEMENT_MAX_LEN);

	if (Activate(pack->agreed_length(), pack->data().c_str(), pack->data().length()))
	{
		network::CGOutputPacket<network::CGKeyAgreementPacket> pack_send;
		pack_send->set_agreed_length(agreedLength);
		pack_send->set_data_length(dataLength);
		pack_send->set_data(dataToSend, KEY_AGREEMENT_MAX_LEN);

		if (!Send(pack_send))
		{
			Tracen(" CAccountConnector::__AuthState_RecvKeyAgreement - SendKeyAgreement Error");
			return false;
		}
		Tracenf("KEY_AGREEMENT SEND %u", pack_send->data_length());
	}
	else
	{
		// 키 협상 실패
		Disconnect();
		return false;
	}
	return true;
}

bool CGuildMarkUploader::__LoginState_RecvKeyAgreementCompleted()
{
	Tracenf("KEY_AGREEMENT_COMPLETED RECV");

	ActivateCipher();

	return true;
}
#endif // _IMPROVED_PACKET_ENCRYPTION_
