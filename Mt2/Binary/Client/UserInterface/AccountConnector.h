#pragma 

#include "../EterLib/NetStream.h"
#include "../EterLib/FuncObject.h"

class CAccountConnector : public CNetworkStream, public CSingleton<CAccountConnector>
{
	public:
		enum
		{
			STATE_OFFLINE,
			STATE_HANDSHAKE,
			STATE_AUTH,
		};

	public:
		CAccountConnector();
		virtual ~CAccountConnector();

		void SetHandler(PyObject* poHandler);
		void SetLoginInfo(const char * c_szName, const char * c_szPwd);
		const char* GetLoginID() { return m_strID.c_str(); }
		void ClearLoginInfo( void );
		bool Connect(const char * c_szAddr, int iPort, const char * c_szAccountAddr, int iAccountPort);
		void Disconnect();
		void Process();

	protected:
		void OnConnectFailure();
		void OnConnectSuccess();
		void OnRemoteDisconnect();
		void OnDisconnect();

	protected:
		void __Inialize();
		bool __StateProcess();

		void __OfflineState_Set();
		void __HandshakeState_Set();
		void __AuthState_Set();

		bool __AuthState_RecvPhase(std::unique_ptr<network::GCPhasePacket> p);
		bool __AuthState_RecvHandshake(std::unique_ptr<network::GCHandshakePacket> p);
		bool __AuthState_RecvPing();
		bool __AuthState_SendPong();
		bool __AuthState_RecvVersionAnswer(std::unique_ptr<network::GCLoginVersionAnswerPacket> p);
		bool __AuthState_RecvAuthSuccess(std::unique_ptr<network::GCAuthSuccessPacket> p);
		bool __AuthState_RecvAuthFailure(std::unique_ptr<network::GCLoginFailurePacket> p);

	protected:
		UINT m_eState;
		std::string m_strID;
		std::string m_strPassword;

		std::string m_strAddr;
		int m_iPort;
		BOOL m_isWaitKey;

		PyObject * m_poHandler;

		void __BuildClientKey_20050304Myevan();
};
