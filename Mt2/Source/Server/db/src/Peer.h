// vim: ts=8 sw=4
#ifndef __INC_PEER_H__
#define __INC_PEER_H__

#include "PeerBase.h"
#include "headers.hpp"

namespace network
{
	struct InputPacket
	{
		TPacketHeader header;
		const char* content;
		uint16_t content_size;

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
			if (!packet)
				return std::unique_ptr<T>();

			auto packet_base = static_cast<::google::protobuf::Message*>(packet);
			if (content_size > 0)
				packet_base->ParseFromArray(content, content_size);

			return std::unique_ptr<T>(packet);
		}
	};
}

class CPeer : public CPeerBase
{
    protected:
	virtual void OnAccept();
	virtual void OnClose();
	virtual void OnConnect();

    public:
#pragma pack(1)
	typedef struct _header
	{   
	    BYTE    bHeader;
	    DWORD   dwHandle;
	    DWORD   dwSize;
	} HEADER;
#pragma pack()
	enum EState
	{
	    STATE_CLOSE = 0,
	    STATE_PLAYING = 1
	};

	CPeer();
	virtual ~CPeer();

	bool 	PeekPacket(int& bytes_received, network::InputPacket& packet, DWORD& handle);

	void	ProcessInput();
	int	Send();

	DWORD	GetHandle();

	void	SetPublicIP(const char * ip)	{ m_stPublicIP = ip; }
	const char * GetPublicIP()		{ return m_stPublicIP.c_str(); }

	void	SetChannel(BYTE bChannel)	{ m_bChannel = bChannel; }
	BYTE	GetChannel()			{ return m_bChannel; }

	void	SetListenPort(WORD wPort) { m_wListenPort = wPort; }
	WORD	GetListenPort() { return m_wListenPort; }

	void	SetP2PPort(WORD wPort);
	WORD	GetP2PPort() { return m_wP2PPort; }

	void	SetMaps(const long* pl);
	void	SetMaps(const google::protobuf::RepeatedField<google::protobuf::uint32>& pl);
	long *	GetMaps() { return &m_alMaps[0]; }

#ifdef PROCESSOR_CORE
	void	SetProcessorCore(bool isProcessorCore) { m_bProcessorCore = isProcessorCore; }
	bool	IsProcessorCore() const { return m_bProcessorCore; }
#endif

	bool	SetItemIDRange(network::TItemIDRangeTable itemRange);
	bool	SetSpareItemIDRange(network::TItemIDRangeTable itemRange);
	bool	CheckItemIDRangeCollision(const network::TItemIDRangeTable& itemRange);
	void	SendSpareItemIDRange();

	void	SetLogonPIDVector(const std::vector<DWORD>* pkVecLogonPID) { m_pkVecLogonPID = pkVecLogonPID; }

    private:
	int	m_state;

	BYTE	m_bChannel;
	DWORD	m_dwHandle;
	WORD	m_wListenPort;	// 게임서버가 클라이언트를 위해 listen 하는 포트
	WORD	m_wP2PPort;	// 게임서버가 게임서버 P2P 접속을 위해 listen 하는 포트
	long	m_alMaps[MAP_ALLOW_LIMIT];	// 어떤 맵을 관장하고 있는가?
#ifdef PROCESSOR_CORE
	bool	m_bProcessorCore;
#endif

	network::TItemIDRangeTable m_itemRange;
	network::TItemIDRangeTable m_itemSpareRange;

	std::string m_stPublicIP;

	const std::vector<DWORD>* m_pkVecLogonPID;
};

#endif
