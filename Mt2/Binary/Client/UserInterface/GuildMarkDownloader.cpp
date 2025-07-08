#include "StdAfx.h"
#include "GuildMarkDownloader.h"
#include "PythonCharacterManager.h"
#include "Packet.h"
#include "Test.h"
#include "../EterPack/mtrand.h"

using namespace network;

// MARK_BUG_FIX
struct SMarkIndex
{
	WORD guild_id;
	WORD mark_id;
};

// END_OFMARK_BUG_FIX

CGuildMarkDownloader::CGuildMarkDownloader()
{
	SetRecvBufferSize(640*1024);
	SetSendBufferSize(1024);
	__Initialize();
}

CGuildMarkDownloader::~CGuildMarkDownloader()
{
	__OfflineState_Set();
}

bool CGuildMarkDownloader::Connect(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey)
{
	__OfflineState_Set();

	m_dwHandle=dwHandle;
	m_dwRandomKey=dwRandomKey;
	m_dwTodo=TODO_RECV_MARK;
	return CNetworkStream::Connect(c_rkNetAddr);
}

bool CGuildMarkDownloader::ConnectToRecvSymbol(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey, const std::vector<DWORD> & c_rkVec_dwGuildID)
{
	__OfflineState_Set();

	m_dwHandle=dwHandle;
	m_dwRandomKey=dwRandomKey;
	m_dwTodo=TODO_RECV_SYMBOL;
	m_kVec_dwGuildID = c_rkVec_dwGuildID;
	return CNetworkStream::Connect(c_rkNetAddr);
}

void CGuildMarkDownloader::Process()
{
	CNetworkStream::Process();

	if (!__StateProcess())
	{
		__OfflineState_Set();
		Disconnect();
	}
}

void CGuildMarkDownloader::OnConnectFailure()
{
	__OfflineState_Set();
}

void CGuildMarkDownloader::OnConnectSuccess()
{
	__LoginState_Set();
}

void CGuildMarkDownloader::OnRemoteDisconnect()
{
	__OfflineState_Set();
}

void CGuildMarkDownloader::OnDisconnect()
{
	__OfflineState_Set();
}

void CGuildMarkDownloader::__Initialize()
{
	m_eState=STATE_OFFLINE;
	m_pkMarkMgr=NULL;
	m_currentRequestingImageIndex=0;
	m_dwBlockIndex=0;
	m_dwBlockDataPos=0;
	m_dwBlockDataSize=0;

	m_dwHandle=0;
	m_dwRandomKey=0;
	m_dwTodo=TODO_RECV_NONE;
	m_kVec_dwGuildID.clear();
}

bool CGuildMarkDownloader::__StateProcess()
{
	switch (m_eState)
	{
		case STATE_LOGIN:
			return __LoginState_Process();
			break;
		case STATE_COMPLETE:
			return false;
	}

	return true;
}

void CGuildMarkDownloader::__OfflineState_Set()
{
	__Initialize();
}

void CGuildMarkDownloader::__CompleteState_Set()
{
	m_eState = STATE_COMPLETE;
	CPythonCharacterManager::instance().RefreshAllGuildMark();
}

void CGuildMarkDownloader::__LoginState_Set()
{
	m_eState = STATE_LOGIN;
}

bool CGuildMarkDownloader::__LoginState_Process()
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

			case TGCHeader::MARK_IDX_LIST:
				return __LoginState_RecvMarkIndex(packet.get<GCMarkIDXListPacket>());

			case TGCHeader::MARK_BLOCK:
				return __LoginState_RecvMarkBlock(packet.get<GCMarkBlockPacket>());

			case TGCHeader::GUILD_SYMBOL_DATA:
				return __LoginState_RecvSymbolData(packet.get<GCGuildSymbolDataPacket>());

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

extern int mb_encryptAuth(DWORD* dest, const DWORD* src, const DWORD key, int size);
bool CGuildMarkDownloader::__LoginState_RecvHandshake(std::unique_ptr<GCHandshakePacket> pack)
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

bool CGuildMarkDownloader::__LoginState_RecvPing()
{
	if (!Send(TCGHeader::PONG))
		return false;

	return true;
}

bool CGuildMarkDownloader::__LoginState_RecvPhase(std::unique_ptr<GCPhasePacket> pack)
{
	if (pack->phase() == PHASE_LOGIN)
	{
#ifndef _IMPROVED_PACKET_ENCRYPTION_
	//	const char* key = LocaleService_GetSecurityKey();
	//	SetSecurityMode(true, key);
#endif

		switch (m_dwTodo)
		{
			case TODO_RECV_NONE:
			{
				assert(!"CGuildMarkDownloader::__LoginState_RecvPhase - Todo type is none");
				break;
			}
			case TODO_RECV_MARK:
			{
				// MARK_BUG_FIX
				if (!__SendMarkIDXList())
					return false;
				// END_OF_MARK_BUG_FIX
				break;
			}
			case TODO_RECV_SYMBOL:
			{
				if (!__SendSymbolCRCList())
					return false;
				break;
			}
		}
	}

	return true;
}

// MARK_BUG_FIX
bool CGuildMarkDownloader::__SendMarkIDXList()
{
	if (!Send(TCGHeader::MARK_IDX_LIST))
		return false;

	return true;
}

bool CGuildMarkDownloader::__LoginState_RecvMarkIndex(std::unique_ptr<GCMarkIDXListPacket> pack)
{
	for (auto& elem : pack->elems())
	{
		CGuildMarkManager::Instance().AddMarkIDByGuildID(elem.guild_id(), elem.mark_id());
	}

	CGuildMarkManager::Instance().LoadMarkImages();

	m_currentRequestingImageIndex = 0;
	__SendMarkCRCList();
	return true;
}

bool CGuildMarkDownloader::__SendMarkCRCList()
{
	network::CGOutputPacket<network::CGMarkCRCListPacket> pack_send;

	DWORD crclist[CGuildMarkImage::BLOCK_ROW_COUNT * CGuildMarkImage::BLOCK_COL_COUNT];
	if (!CGuildMarkManager::Instance().GetBlockCRCList(m_currentRequestingImageIndex, crclist))
		__CompleteState_Set();
	else
	{
		pack_send->set_image_index(m_currentRequestingImageIndex);
		for (DWORD crc : crclist)
			pack_send->add_crclist(crc);
		++m_currentRequestingImageIndex;

		if (!Send(pack_send))
			return false;
	}
	return true;
}

bool CGuildMarkDownloader::__LoginState_RecvMarkBlock(std::unique_ptr<GCMarkBlockPacket> pack)
{
	BYTE posBlock;
	DWORD compSize;

	const char* ptr = pack->image().data();

	for (DWORD i = 0; i < pack->block_count(); ++i)
	{
		posBlock = *(BYTE*) ptr;
		ptr += sizeof(BYTE);
		compSize = *(DWORD*) ptr;
		ptr += sizeof(DWORD);

		if (compSize > SGuildMarkBlock::MAX_COMP_SIZE)
		{
			TraceError("RecvMarkBlock: data corrupted");
		}
		else
		{
			if (!CGuildMarkManager::Instance().SaveBlockFromCompressedData(pack->image_index(), posBlock, (const BYTE*)ptr, compSize))
				TraceError("Could not recv mark block %d", i);
		}

		ptr += compSize;
	}

	if (pack->block_count() > 0)
	{
		CGuildMarkManager::Instance().SaveMarkImage(pack->image_index());

		std::string imagePath;

		if (CGuildMarkManager::Instance().GetMarkImageFilename(pack->image_index(), imagePath))
		{
			CResource * pResource = CResourceManager::Instance().GetResourcePointer(imagePath.c_str());
			if (pResource->IsType(CGraphicImage::Type()))
			{
				CGraphicImage* pkGrpImg=static_cast<CGraphicImage*>(pResource);
				pkGrpImg->Reload();
			}
		}
	}

	// 더 요청할 것이 있으면 요청하고 아니면 이미지를 저장하고 종료
	if (m_currentRequestingImageIndex < CGuildMarkManager::Instance().GetMarkImageCount())
		__SendMarkCRCList();
	else
		__CompleteState_Set();

	return true;
}
// END_OF_MARK_BUG_FIX

#ifdef _IMPROVED_PACKET_ENCRYPTION_
bool CGuildMarkDownloader::__LoginState_RecvKeyAgreement(std::unique_ptr<GCKeyAgreementPacket> pack)
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

bool CGuildMarkDownloader::__LoginState_RecvKeyAgreementCompleted()
{
	Tracenf("KEY_AGREEMENT_COMPLETED RECV");

	ActivateCipher();

	return true;
}
#endif // _IMPROVED_PACKET_ENCRYPTION_

bool CGuildMarkDownloader::__SendSymbolCRCList()
{
	for (DWORD i=0; i<m_kVec_dwGuildID.size(); ++i)
	{
		network::CGOutputPacket<network::CGGuildSymbolCRCPacket> pack_send;
		pack_send->set_guild_id(m_kVec_dwGuildID[i]);

		std::string strFileName = GetGuildSymbolFileName(m_kVec_dwGuildID[i]);
		pack_send->set_crc(GetFileCRC32(strFileName.c_str()));
		pack_send->set_symbol_size(GetFileSize(strFileName.c_str()));
		if (!Send(pack_send))
			return false;
	}

	return true;
}

bool CGuildMarkDownloader::__LoginState_RecvSymbolData(std::unique_ptr<GCGuildSymbolDataPacket> pack)
{
	MyCreateDirectory(g_strGuildSymbolPathName.c_str());

	std::string strFileName = GetGuildSymbolFileName(pack->guild_id());

	FILE * File = fopen(strFileName.c_str(), "wb");
	if (!File)
		return false;
	fwrite(pack->image().c_str(), pack->image().length(), 1, File);
	fclose(File);

	return true;
}
