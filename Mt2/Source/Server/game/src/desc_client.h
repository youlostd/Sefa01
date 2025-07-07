#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "desc.h"

class CLIENT_DESC : public DESC
{
	public:
		CLIENT_DESC();
		virtual ~CLIENT_DESC();

		virtual BYTE	GetType() { return DESC_TYPE_CONNECTOR; }
		virtual void	Destroy();
		virtual void	SetPhase(int phase);

		bool 		Connect(int iPhaseWhenSucceed = 0);
		void		Setup(LPFDWATCH _fdw, const char * _host, WORD _port);

		void		SetRetryWhenClosed(bool);

		template <typename T>
		void		DBPacket(network::GDOutputPacket<T>& packet, uint32_t handle = 0) { DBPacketSend(packet.header, &packet.data, handle); }
		void		DBPacket(network::TGDHeader header, uint32_t handle = 0);
		bool		IsRetryWhenClosed();

		void		Update(DWORD t);

		// Non-destructive close for reuse
		void Reset();

	private:
		void DBPacketSend(network::TPacketHeader& header, const ::google::protobuf::Message* data, uint32_t handle);
		void DBPacketSend(const void * c_pvData, int iSize);
		void InitializeBuffers();

	protected:
		BYTE		m_bDBSentHeader;

		int			m_iPhaseWhenSucceed;
		bool		m_bRetryWhenClosed;
		time_t		m_LastTryToConnectTime;

		CInputDB 	m_inputDB;
		CInputP2P 	m_inputP2P;
};


extern LPCLIENT_DESC db_clientdesc;
extern LPCLIENT_DESC g_pkAuthMasterDesc;
extern LPCLIENT_DESC g_NetmarbleDBDesc;

#endif
