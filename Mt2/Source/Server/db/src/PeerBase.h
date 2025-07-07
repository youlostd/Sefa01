// vim: ts=8 sw=4
#ifndef __INC_PEERBASE_H__
#define __INC_PEERBASE_H__

#include "NetBase.h"
#include <google/protobuf/message.h>
#include "headers.hpp"

class CPeerBase : public CNetBase
{
    public:
	enum
	{ 
	    MAX_HOST_LENGTH		= 30,
	    MAX_INPUT_LEN		= 1024 * 1024 * 2,
	    DEFAULT_PACKET_BUFFER_SIZE	= 1024 * 1024 * 2
	};

    protected:
	virtual void	OnAccept() = 0;
	virtual void	OnConnect() = 0;
	virtual void	OnClose() = 0;

    public:
	bool		Accept(socket_t accept_fd);
	bool		Connect(const char* host, WORD port);
	void		Close();

    public:
	CPeerBase();
	virtual ~CPeerBase();

	void		Disconnect();
	void		Destroy();

	socket_t	GetFd() { return m_fd; }

	template <typename T>
	void		Packet(network::DGOutputPacket<T>& packet, uint32_t handle = 0) { PacketSend(packet.header, handle, &packet.data); }
	void		Packet(network::TDGHeader header, uint32_t handle = 0, const ::google::protobuf::Message* packet = nullptr);
	int		Send();

	int		Recv();
	void		RecvEnd(int proceed_bytes);
	int		GetRecvLength();
	const void *	GetRecvBuffer();

	int		GetSendLength();

	const char *	GetHost() { return m_host; }

	protected:
	void		PacketSend(network::TPacketHeader& header, uint32_t handle, const ::google::protobuf::Message* data);
	void		PacketSend(const void * c_pvData, int iSize);

    protected:
	char		m_host[MAX_HOST_LENGTH + 1];
	socket_t	m_fd;

    private:
	int		m_BytesRemain;
	LPBUFFER	m_outBuffer;
	LPBUFFER	m_inBuffer;
};

#endif
