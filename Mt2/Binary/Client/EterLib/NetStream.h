#pragma once
#include "../UserInterface/Locale_inc.h"
#ifdef _IMPROVED_PACKET_ENCRYPTION_
#include "../eterBase/cipher.h"
#endif
#include "../eterBase/tea.h"
#include "NetAddress.h"

#include "../UserInterface/headers.hpp"

namespace network
{
	struct InputPacket
	{
		TPacketHeader header;
		const char* content;
		uint32_t content_size;

		InputPacket()
		{
			content_size = 0;
		}

		uint16_t get_header() const noexcept
		{
			return header.header;
		}
		template <typename HeaderType>
		typename std::enable_if<std::is_enum<HeaderType>::value, HeaderType>::type get_header() const noexcept
		{
			return static_cast<HeaderType>(header.header);
		}

		template <typename T>
		std::unique_ptr<T> get() const
		{
			T* packet = new T();

			auto packet_base = static_cast<::google::protobuf::Message*>(packet);
			if (content_size > 0)
			{
				bool ret = packet_base->ParseFromArray(content, content_size);
				if (!ret)
					TraceError("invalid packet with header %u", get_header());
				/*if (!ret || packet_base->ByteSize() != content_size)
					sys_err("invalid packet with header %u content_size %u real_size %u ret %d",
						get_header(), content_size, packet_base->ByteSize(), ret);*/
			}

			return std::unique_ptr<T>(packet);
		}
	};
}

class CNetworkStream
{
	public:
		CNetworkStream();
		virtual ~CNetworkStream();		

		void SetRecvBufferSize(int recvBufSize);
		void SetSendBufferSize(int sendBufSize);

//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//		void SetSecurityMode(bool isSecurityMode, const char* c_szTeaKey);
//		void SetSecurityMode(bool isSecurityMode, const char* c_szTeaEncryptKey, const char* c_szTeaDecryptKey);
//#endif
#ifdef _IMPROVED_PACKET_ENCRYPTION_
		bool IsSecurityMode();
#endif

		int	GetRecvBufferSize();

		void Clear();
		void ClearRecvBuffer();

		void Process();

		bool Connect(const CNetworkAddress& c_rkNetAddr, int limitSec = 6);
		bool Connect(const char* c_szAddr, int port, int limitSec = 6);
		bool Connect(DWORD dwAddr, int port, int limitSec = 6);
		void Disconnect();

		void SetPacketVerifyKey(uint32_t key);

		bool Recv(network::InputPacket& packet);
		template <typename T>
		bool Send(network::CGOutputPacket<T>& packet) { return __Send(packet.header, &packet.data, false); }
		template <typename T>
		bool SendFlush(network::CGOutputPacket<T>& packet) { return __Send(packet.header, &packet.data, true); }
		bool Send(network::TCGHeader header);
		bool SendFlush(network::TCGHeader header);


		bool IsOnline();

	protected:
		void ClearPacketVerifyKey();

		bool __Send(network::TPacketHeader& header, const ::google::protobuf::Message* data, bool flush = false);
		bool __Send(int len, const void* c_pvData, bool flush = false);

		virtual void OnConnectSuccess();				
		virtual void OnConnectFailure();
		virtual void OnRemoteDisconnect();
		virtual void OnDisconnect();		
		virtual bool OnProcess();

		bool __SendInternalBuffer();
		bool __RecvInternalBuffer();

		void __PopSendBuffer();

		int __GetSendBufferSize();

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		size_t Prepare(void* buffer, size_t* length);
		bool Activate(size_t agreed_length, const void* buffer, size_t length);
		void ActivateCipher();
#endif

	private:
		time_t	m_connectLimitTime;

		char*	m_recvTEABuf;
		int		m_recvTEABufInputPos;
		int		m_recvTEABufSize;

		char*	m_recvBuf;
		int		m_recvBufSize;
		int		m_recvBufInputPos;
		int		m_recvBufOutputPos;

		char*	m_sendBuf;
		int		m_sendBufSize;
		int		m_sendBufInputPos;
		int		m_sendBufOutputPos;

		char*	m_sendTEABuf;
		int		m_sendTEABufSize;
		int		m_sendTEABufInputPos;

		bool	m_isOnline;

		uint32_t	m_packetVerifyKey;
		bool		m_packetVerifyKeyIsSet;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		Cipher	m_cipher;
#else
		// Obsolete encryption stuff here
		//bool	m_isSecurityMode;
		//char	m_szEncryptKey[TEA_KEY_LENGTH]; // Client 에서 보낼 패킷을 Encrypt 할때 사용하는 Key
		//char	m_szDecryptKey[TEA_KEY_LENGTH]; // Server 에서 전송된 패킷을 Decrypt 할때 사용하는 Key
#endif

		SOCKET	m_sock;

		CNetworkAddress m_addr;
};
