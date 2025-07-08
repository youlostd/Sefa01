#pragma once

#include "../EterLib/NetStream.h"
#include "MarkManager.h"

class CGuildMarkDownloader : public CNetworkStream, public CSingleton<CGuildMarkDownloader>
{
	public:
		CGuildMarkDownloader();
		virtual ~CGuildMarkDownloader();

		bool Connect(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey);
		bool ConnectToRecvSymbol(const CNetworkAddress& c_rkNetAddr, DWORD dwHandle, DWORD dwRandomKey, const std::vector<DWORD> & c_rkVec_dwGuildID);

		void Process();

	private:
		enum
		{
			STATE_OFFLINE,
			STATE_LOGIN,
			STATE_COMPLETE,
		};

		enum
		{
			TODO_RECV_NONE,
			TODO_RECV_MARK,
			TODO_RECV_SYMBOL,
		};

	private:
		void OnConnectFailure();
		void OnConnectSuccess();
		void OnRemoteDisconnect();
		void OnDisconnect();

		void __Initialize();
		bool __StateProcess();

		UINT __GetPacketSize(UINT header);
		bool __DispatchPacket(UINT header);

		void __OfflineState_Set();
		void __CompleteState_Set();

		void __LoginState_Set();
		bool __LoginState_Process();
		bool __LoginState_RecvPhase(std::unique_ptr<network::GCPhasePacket> pack);
		bool __LoginState_RecvHandshake(std::unique_ptr<network::GCHandshakePacket> pack);
		bool __LoginState_RecvPing();
		bool __LoginState_RecvMarkIndex(std::unique_ptr<network::GCMarkIDXListPacket> pack);
		bool __LoginState_RecvMarkBlock(std::unique_ptr<network::GCMarkBlockPacket> pack);
		bool __LoginState_RecvSymbolData(std::unique_ptr<network::GCGuildSymbolDataPacket> pack);
#ifdef _IMPROVED_PACKET_ENCRYPTION_
		bool __LoginState_RecvKeyAgreement(std::unique_ptr<network::GCKeyAgreementPacket> pack);
		bool __LoginState_RecvKeyAgreementCompleted();
#endif
		bool __SendMarkIDXList();
		bool __SendMarkCRCList();
		bool __SendSymbolCRCList();

	private:
		DWORD m_dwHandle;
		DWORD m_dwRandomKey;
		DWORD m_dwTodo;

		std::vector<DWORD> m_kVec_dwGuildID;

		UINT m_eState;

		BYTE m_currentRequestingImageIndex;

		CGuildMarkManager * m_pkMarkMgr;

		DWORD m_dwBlockIndex;
		DWORD m_dwBlockDataSize;
		DWORD m_dwBlockDataPos;
};
