
#ifndef P2P_MANAGER_H_
#define P2P_MANAGER_H_

#include <unordered_map>

#include "input.h"
#include "desc.h"
#include "../../common/stl.h"
#include "protobuf_data.h"
#include "protobuf_gg_packets.h"

typedef struct _CCI
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	DWORD	dwPID;
	unsigned char	bEmpire;
	long	lMapIndex;
	bool	isInDungeon;
	unsigned char	bChannel;
	BYTE	bRace;
	BYTE	bLanguage;
	bool	bTempLogin;

	LPDESC	pkDesc;
} CCI;

class P2P_MANAGER : public singleton<P2P_MANAGER>
{
	public:
		typedef std::unordered_map<std::string, CCI *, stringhash> TCCIMap;
		typedef std::unordered_map<DWORD, CCI*> TPIDCCIMap;

	public:
		P2P_MANAGER();
		~P2P_MANAGER();

		// ¾Æ·¡ Register* Unregister* pairµéÀº ³»ºÎÀûÀ¸·Î »ç½Ç °°Àº ·çÆ¾À» »ç¿ëÇÑ´Ù.
		// ´ÜÁö ¸í½ÃÀûÀ¸·Î Ç¥½ÃÇÏ±â À§ÇÑ °Í
		void			RegisterAcceptor(LPDESC d);
		void			UnregisterAcceptor(LPDESC d);

		void			RegisterConnector(LPDESC d);
		void			UnregisterConnector(LPDESC d);

#ifdef PROCESSOR_CORE
		void			SetProcessorCore(LPDESC d) { m_processorCore = d; }
		LPDESC			GetProcessorCore() const { return m_processorCore; }
		template <typename T>
		bool			SendProcessorCore(network::GGOutputPacket<T>& packet)
		{
			if (m_processorCore == nullptr)
				return false;

			m_processorCore->Packet(packet);
			return true;
		}
#endif

		void			EraseUserByDesc(LPDESC d);	// ÇØ´ç desc¿¡ ÀÖ´Â À¯ÀúµéÀ» Áö¿î´Ù.

		void			FlushOutput();

		void			Boot(LPDESC d);	// p2p Ã³¸®¿¡ ÇÊ¿äÇÑ Á¤º¸¸¦ º¸³»ÁØ´Ù. (Àü Ä³¸¯ÅÍÀÇ ·Î±×ÀÎ Á¤º¸ µî)

		template <typename T>
		void			Send(network::GGOutputPacket<T>& packet, LPDESC except = NULL)
		{
			std::for_each(m_set_pkPeers.begin(), m_set_pkPeers.end(), [&packet, except](LPDESC desc) {
				if (except == desc)
					return;

				desc->Packet(packet);
			});
		}
		template <typename T>
		bool			Send(network::GGOutputPacket<T>& packet, WORD wPort, bool bP2PPort = false) {
			for (LPDESC pkDesc : m_set_pkPeers)
			{
				if ((bP2PPort && pkDesc->GetP2PPort() == wPort) || (!bP2PPort && pkDesc->GetListenPort() == wPort)) {
					sys_log(0, "P2P_Send: send to port %u", wPort);
					pkDesc->Packet(packet);
					return true;
				}
			}

			return false;
		}
		void			Send(network::TGGHeader header, LPDESC except = NULL)
		{
			std::for_each(m_set_pkPeers.begin(), m_set_pkPeers.end(), [header, except](LPDESC desc) {
				if (except == desc)
					return;

				desc->P2PPacket(header);
			});
		}
		template <typename T>
		void			SendByPID(DWORD dwPID, network::GGOutputPacket<T>& packet)
		{
			CCI* pkCCI = FindByPID(dwPID);
			if (!pkCCI)
				return;

			pkCCI->pkDesc->SetRelay(pkCCI->szName);
			pkCCI->pkDesc->Packet(packet);
		}

		LPDESC			FindPeerByMap(int iIndex, BYTE bChannel = 0) const;

		void			Login(LPDESC d, std::unique_ptr<network::GGLoginPacket> p);
		void			Logout(const char * c_pszName);

		CCI *			Find(const char * c_pszName);
		CCI *			FindByPID(DWORD pid);

		int				GetCount();
		int				GetEmpireUserCount(int idx);
		int				GetDescCount();
		void			GetP2PHostNames(std::string& hostNames);

		const TPIDCCIMap*	GetP2PCCIMap() { return &m_map_dwPID_pkCCI; }

#ifdef __P2P_ONLINECOUNT__
		DWORD			GetOnlinePlayerInfo(std::vector<network::TOnlinePlayerInfo>& rvec_PlayerInfo);
#endif

	private:
		void			Logout(CCI * pkCCI);

		CInputProcessor *	m_pkInputProcessor;
		int			m_iHandleCount;

		TR1_NS::unordered_set<LPDESC> m_set_pkPeers;
		TCCIMap			m_map_pkCCI;
		TPIDCCIMap		m_map_dwPID_pkCCI;
		int			m_aiEmpireUserCount[EMPIRE_MAX_NUM];

#ifdef PROCESSOR_CORE
		LPDESC		m_processorCore;
#endif
};

#endif /* P2P_MANAGER_H_ */

