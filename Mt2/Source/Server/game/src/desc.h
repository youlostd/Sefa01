#ifndef __INC_METIN_II_GAME_DESC_H__
#define __INC_METIN_II_GAME_DESC_H__

#include "constants.h"
#include "input.h"
#ifdef _IMPROVED_PACKET_ENCRYPTION_
#include "cipher.h"
#endif
#ifdef ACCOUNT_TRADE_BLOCK
#include "utils.h"
#endif

#include "headers.hpp"
#include "protobuf_data.h"
#include <google/protobuf/port_def.inc>
#include <google/protobuf/message.h>

#define MAX_ALLOW_USER				  4096
//#define MAX_INPUT_LEN			2048
#define MAX_INPUT_LEN			65536

#define HANDSHAKE_RETRY_LIMIT		32

class CInputProcessor;

enum EDescType
{
	DESC_TYPE_ACCEPTOR,
	DESC_TYPE_CONNECTOR
};

class CLoginKey
{
	public:
		CLoginKey(DWORD dwKey, LPDESC pkDesc) : m_dwKey(dwKey), m_pkDesc(pkDesc)
		{
			m_dwExpireTime = 0;
		}

		void Expire()
		{
			m_dwExpireTime = get_dword_time();
			m_pkDesc = NULL;
		}

		operator DWORD() const
		{
			return m_dwKey;
		}

		DWORD   m_dwKey;
		DWORD   m_dwExpireTime;
		LPDESC  m_pkDesc;
};


// sequence ¹ö±× Ã£±â¿ë µ¥ÀÌÅ¸
struct seq_t
{
	BYTE	hdr;
	BYTE	seq;
};
typedef std::vector<seq_t>	seq_vector_t;
// sequence ¹ö±× Ã£±â¿ë µ¥ÀÌÅ¸

class DESC
{
	public:
		EVENTINFO(desc_event_info)
		{
			LPDESC desc;

			desc_event_info() 
			: desc(0)
			{
			}
		};

	public:
		DESC();
		virtual ~DESC();

		virtual BYTE		GetType() { return DESC_TYPE_ACCEPTOR; }
		virtual void		Destroy();
		virtual void		SetPhase(int _phase);
		virtual int			GetPhase() { return m_iPhase; }

		void			FlushOutput();

		bool			Setup(LPFDWATCH _fdw, socket_t _fd, const struct sockaddr_in & c_rSockAddr, DWORD _handle, DWORD _handshake);

		socket_t		GetSocket() const	{ return m_sock; }
		const char *	GetHostName()		{ return m_stHost.c_str(); }
		WORD			GetPort()			{ return m_wPort; }

		void			SetListenPort(WORD w)	{ m_wListenPort = w; }
		WORD			GetListenPort()			{ return m_wListenPort; }

		void			SetP2P(const char * h, WORD w, BYTE b) { m_stP2PHost = h; m_wP2PPort = w; m_bP2PChannel = b; }
		const char *	GetP2PHost()		{ return m_stP2PHost.c_str();	}
		WORD			GetP2PPort() const		{ return m_wP2PPort; }
		BYTE			GetP2PChannel() const	{ return m_bP2PChannel;	}

		template <typename T>
		void			Packet(network::OutputPacket<T>& packet) { PacketSend(packet.header, &packet.data, false); }
		template <typename T>
		void			LargePacket(network::OutputPacket<T>& packet) { PacketSend(packet.header, &packet.data, true); }
		void			Packet(network::TGCHeader header, const ::PROTOBUF_NAMESPACE_ID::Message* packet = nullptr);
		void			P2PPacket(network::TGGHeader header, const ::PROTOBUF_NAMESPACE_ID::Message* packet = nullptr);
		void			DirectPacket(network::TGCHeader header, const void* serializedData, size_t size);

		int			ProcessInput();		// returns -1 if error
		int			ProcessOutput();	// returns -1 if error

		CInputProcessor	*	GetInputProcessor()	{ return m_pInputProcessor; }

		DWORD			GetHandle() const	{ return m_dwHandle; }
		LPBUFFER		GetOutputBuffer()	{ return m_lpOutputBuffer; }

		void			BindAccountTable(network::TAccountTable* pTable);
		network::TAccountTable &		GetAccountTable()	{ return m_accountTable; }

		void			BindCharacter(LPCHARACTER ch);
		LPCHARACTER		GetCharacter()		{ return m_lpCharacter; }

		bool			IsPhase(int phase) const	{ return m_iPhase == phase ? true : false; }

		const struct sockaddr_in & GetAddr()		{ return m_SockAddr;	}

		void			Log(const char * format, ...);

		// ÇÚµå½¦ÀÌÅ© (½Ã°£ µ¿±âÈ­)
		void			StartHandshake(DWORD _dw);
		void			SendHandshake(DWORD dwCurTime, int iNewDelta);
		bool			HandshakeProcess(const void* cryptData, size_t dataLen);
		bool			IsHandshaking();

		DWORD			GetHandshake() const	{ return m_dwHandshake; }
		DWORD			GetClientTime();

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		void SendKeyAgreement();
		void SendKeyAgreementCompleted();
		bool FinishHandshake(size_t agreed_length, const void* buffer, size_t length);
		bool IsCipherPrepared();
#else
		// Obsolete encryption stuff here
		//void			SetSecurityKey(const DWORD * c_pdwKey);
		//const DWORD *	GetEncryptionKey() const { return &m_adwEncryptionKey[0]; }
		//const DWORD *	GetDecryptionKey() const { return &m_adwDecryptionKey[0]; }
#endif

		// Á¦±¹
		BYTE			GetEmpire();

#ifdef ACCOUNT_TRADE_BLOCK
		bool			IsTradeblocked() { return m_accountTable.tradeblock() > get_global_time() || m_accountTable.hwid2ban() > 0; };
		void			SetTradeblocked(DWORD timestamp = 0) { m_accountTable.set_tradeblock(0); };
		bool			IsHWID2Banned() { return m_accountTable.hwid2ban() > 0; };
#endif

		// for p2p
		void			SetRelay(const char * c_pszName);
		void			SetRelay(DWORD pid);
		bool			DelayedDisconnect(int iSec);
		void			DisconnectOfSameLogin();

		void			SetAdminMode();
		bool			IsAdminMode();		// Handshake ¿¡¼­ ¾îµå¹Î ¸í·ÉÀ» ¾µ¼öÀÖ³ª?

		void			SetPong(bool b);
		bool			IsPong();

		void			SendLoginSuccessPacket();

		void			SetLoginKey(DWORD dwKey);
		void			SetLoginKey(CLoginKey * pkKey);
		DWORD			GetLoginKey();

#ifdef __DEPRECATED_BILLING__
		void			SetBillingExpireSecond(DWORD dwSec);
		DWORD			GetBillingExpireSecond();
#endif

		void			SetClientVersion(const char * c_pszTimestamp) { m_stClientVersion = c_pszTimestamp; }
		const char *		GetClientVersion() { return m_stClientVersion.c_str(); }

		bool			HasVerifyPacketKey() const { return m_packetVerifyKeyIsSet; }
		uint32_t		GetVerifyPacketKey() const { return m_packetVerifyKey; }

	protected:
		void			SetPhase(int _phase, bool generate_verify_key);

		void			Initialize();

		void			PacketSend(network::TPacketHeader& header, const ::PROTOBUF_NAMESPACE_ID::Message* data, bool is_large = false);
		void			PacketSend(const void* c_pvData, int iSize, bool is_large = false);

		void			GenerateVerifyPacketKey();

	protected:
		CInputProcessor *	m_pInputProcessor;
		CInputClose		m_inputClose;
		CInputHandshake	m_inputHandshake;
		CInputLogin		m_inputLogin;
		CInputMain		m_inputMain;
		CInputDead		m_inputDead;
		CInputAuth		m_inputAuth;

		uint32_t		m_packetVerifyKey;
		bool			m_packetVerifyKeyIsSet;

		LPFDWATCH		m_lpFdw;
		socket_t		m_sock;
		int				m_iPhase;
		DWORD			m_dwHandle;

		std::string		m_stHost;
		WORD			m_wPort;
		WORD			m_wListenPort;
		time_t			m_LastTryToConnectTime;

		LPBUFFER		m_lpInputBuffer;
		int				m_iMinInputBufferLen;
	
		DWORD			m_dwHandshake;
		DWORD			m_dwHandshakeSentTime;
		int				m_iHandshakeRetry;
		DWORD			m_dwClientTime;
		bool			m_bHandshaking;

		LPBUFFER		m_lpBufferedOutputBuffer;
		LPBUFFER		m_lpOutputBuffer;

		LPEVENT			m_pkPingEvent;
		LPCHARACTER		m_lpCharacter;
		network::TAccountTable		m_accountTable;

		struct sockaddr_in	m_SockAddr;

		FILE *			m_pLogFile;
		std::string		m_stRelayName;
		DWORD			m_dwRelayPID;

		std::string		m_stP2PHost;
		WORD			m_wP2PPort;
		BYTE			m_bP2PChannel;

		bool			m_bAdminMode; // Handshake ¿¡¼­ ¾îµå¹Î ¸í·ÉÀ» ¾µ¼öÀÖ³ª?
		bool			m_bPong;

		CLoginKey *		m_pkLoginKey;
		DWORD			m_dwLoginKey;

#ifdef __DEPRECATED_BILLING__
		DWORD			m_dwBillingExpireSecond;
#endif
		std::string		m_stClientVersion;

		std::string		m_Login;
		int				m_outtime;
		int				m_playtime;
		int				m_offtime;

		bool			m_bDestroyed;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		Cipher cipher_;
#else
		// Obsolete encryption stuff here
		//bool			m_bEncrypted;
		//DWORD			m_adwDecryptionKey[4];
		//DWORD			m_adwEncryptionKey[4];
#endif

	public:
		LPEVENT			m_pkDisconnectEvent;

	public:
		void SetLogin( const std::string & login ) { m_Login = login; }
		void SetLogin( const char * login ) { m_Login = login; }
		const std::string& GetLogin() { return m_Login; }

		void SetOutTime( int outtime ) { m_outtime = outtime; }
		void SetOffTime( int offtime ) { m_offtime = offtime; }
		void SetPlayTime( int playtime ) { m_playtime = playtime; }

		void RawPacket(const void * c_pvData, int iSize);
		void ChatPacket(BYTE type, const char * format, ...);

	public:
		seq_vector_t	m_seq_vector;
		void			push_seq(BYTE hdr, BYTE seq);

	public:
		void	SetLoginAllow(bool bLoginAllow)	{ m_bIsLoginAllow = bLoginAllow; }
		bool	IsLoginAllow() const			{ return m_bIsLoginAllow; }
	private:
		bool	m_bIsLoginAllow;

	private:
		void*	m_lpvClientAuthKey;
		DWORD	m_dwAuthCryptKey;

#ifdef __HAIR_SELECTOR__
	public:
		DWORD	GetOldHairBase(BYTE bIdx) const;
		bool	IsHairBaseChanged(BYTE bIdx) const;
		void	SaveOldHairBase(BYTE bIdx);
		void	SaveHairBases();

	private:
		int		m_aiOldHairBase[PLAYER_PER_ACCOUNT];
#endif

	public:
		void	SetIsDisconnecting(bool result) { m_bIsDisconnecting = result; }
		bool	GetIsDisconnecting() { return m_bIsDisconnecting; }
	private:
		bool	m_bIsDisconnecting;
	
};

#include <google/protobuf/port_undef.inc>

#endif
