﻿#ifndef __INC_METIN_II_GAME_DESC_MANAGER_H__
#define __INC_METIN_II_GAME_DESC_MANAGER_H__

#include <unordered_map>

#include "../../common/stl.h"
#include "../../common/length.h"

class CLoginKey;

class DESC_MANAGER : public singleton<DESC_MANAGER>
{
	public:
		typedef TR1_NS::unordered_set<LPDESC>			DESC_SET;
		typedef TR1_NS::unordered_set<LPCLIENT_DESC>	CLIENT_DESC_SET;
		typedef std::map<int, LPDESC>					DESC_HANDLE_MAP;
		typedef std::map<DWORD, LPDESC>					DESC_HANDSHAKE_MAP;
		typedef std::map<DWORD, LPDESC>					DESC_ACCOUNTID_MAP;
		typedef std::unordered_map<std::string, LPDESC>	DESC_LOGINNAME_MAP;
		typedef std::map<DWORD, DWORD>					DESC_HANDLE_RANDOM_KEY_MAP;

	public:
		DESC_MANAGER();
		virtual ~DESC_MANAGER();

		void			Initialize();
		void			Destroy();

		LPDESC			AcceptDesc(LPFDWATCH fdw, socket_t s);
		LPDESC			AcceptP2PDesc(LPFDWATCH fdw, socket_t s);
		void			DestroyDesc(LPDESC d, bool erase_from_set = true);

		DWORD			CreateHandshake();

		LPCLIENT_DESC		CreateConnectionDesc(LPFDWATCH fdw, const char * host, WORD port, int iPhaseWhenSucceed, bool bRetryWhenClosed);
		void			TryConnect();

		LPDESC			FindByHandle(DWORD handle);
		LPDESC			FindByHandshake(DWORD dwHandshake);

		LPDESC			FindByCharacterName(const char* name);
		LPDESC			FindByLoginName(const std::string& login);
		void			ConnectAccount(const std::string& login, LPDESC d);
		void			DisconnectAccount(const std::string& login);

		void			DestroyClosed();

		const DESC_SET &	GetClientSet();

		DWORD			MakeRandomKey(DWORD dwHandle);
		bool			GetRandomKey(DWORD dwHandle, DWORD* prandom_key);

		DWORD			CreateLoginKey(LPDESC d);
		LPDESC			FindByLoginKey(DWORD dwKey);
		void			ProcessExpiredLoginKey();

		bool			IsDisconnectInvalidCRC() { return m_bDisconnectInvalidCRC; }
		void			SetDisconnectInvalidCRCMode(bool bMode) { m_bDisconnectInvalidCRC = bMode; }

		bool			IsP2PDescExist(const char * szHost, WORD wPort);

	private:
		bool				m_bDisconnectInvalidCRC;

		DESC_HANDLE_RANDOM_KEY_MAP	m_map_handle_random_key;

		CLIENT_DESC_SET		m_set_pkClientDesc;
		DESC_SET			m_set_pkDesc;

		DESC_HANDLE_MAP			m_map_handle;
		DESC_HANDSHAKE_MAP		m_map_handshake;
		//DESC_ACCOUNTID_MAP		m_AccountIDMap;
		DESC_LOGINNAME_MAP		m_map_loginName;
		std::map<DWORD, CLoginKey *>	m_map_pkLoginKey;

		int				m_iSocketsConnected;

		int				m_iHandleCount;

		bool			m_bDestroyed;

	public:
		void						AddBlockedHost(std::string hostname) { m_vecBlockedHosts.push_back(hostname); };
		bool						IsBlockedHost(std::string hostname) { return std::find(m_vecBlockedHosts.begin(), m_vecBlockedHosts.end(), hostname) != m_vecBlockedHosts.end(); };
#ifdef CHECK_IP_ON_CONNECT
		void						AddWhitelistedIP(const std::string& hostname) { m_vecWhitelistIPs.push_back(hostname); };
		bool						IsWhitelistedIP(const std::string& hostname) { return std::find(m_vecWhitelistIPs.begin(), m_vecWhitelistIPs.end(), hostname) != m_vecWhitelistIPs.end(); };
#endif
	private:
		std::vector<std::string>	m_vecBlockedHosts;
#ifdef CHECK_IP_ON_CONNECT
		std::vector<std::string>	m_vecWhitelistIPs;
#endif

};

#endif
