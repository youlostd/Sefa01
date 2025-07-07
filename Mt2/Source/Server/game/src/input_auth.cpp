#include "stdafx.h" 
#include "constants.h"
#include "config.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "protocol.h"
#include "db.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

using namespace network;

#ifdef __ANTI_BRUTEFORCE__
std::map<std::string, std::pair<DWORD, DWORD> > m_mapWrongLoginByHWID;
std::map<std::string, std::pair<DWORD, DWORD> > m_mapWrongLoginByIP;
#endif

extern time_t get_global_time();

bool FN_IS_VALID_LOGIN_STRING(const char *str)
{
	const char*	tmp;

	if (!str || !*str)
		return false;

	if (strlen(str) < 2)
		return false;

	for (tmp = str; *tmp; ++tmp)
	{
		// ¾ËÆÄºª°ú ¼öÀÚ¸¸ Çã¿ë
		if (isdigit(*tmp) || isalpha(*tmp))
			continue;

		return false;
	}

	return true;
}

void CInputAuth::LoginVersionCheck(LPDESC d, std::unique_ptr<CGLoginVersionCheckPacket> p)
{
	network::GCOutputPacket<network::GCLoginVersionAnswerPacket> packet;
	// packet->set_answer(!strcmp(p->version(), g_stClientVersion.c_str()));
	if (!str_is_number(p->version().c_str()))
	{
		packet->set_answer(false);
	}
	else
	{
		int iClientVersion = atoi(p->version().c_str());
		int iServerVersion = atoi(g_stClientVersion.c_str());

		packet->set_answer(iClientVersion >= iServerVersion);
	}
	
	d->Packet(packet);

	if (!packet->answer())
		sys_log(0, "clientVersionWrong [client %s server %s]", p->version().c_str(), g_stClientVersion.c_str());

	d->SetLoginAllow(packet->answer());
}

void CInputAuth::Login(LPDESC d, std::unique_ptr<CGAuthLoginPacket> pinfo)
{
	if (pinfo->client_keys_size() != PACK_CLIENT_KEY_COUNT)
		return;

	const char* c_szIP = inet_ntoa(d->GetAddr().sin_addr);
	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", c_szIP);
		d->DelayedDisconnect(5);
		return;
	}

	if (!d->IsLoginAllow() && !test_server)
	{
		sys_err("No login allow user %s. IP %s might be a hacker.", pinfo->login().c_str(), c_szIP);
		d->DelayedDisconnect(5);
		return;
	}

	if (pinfo->login().length() > LOGIN_MAX_LEN || pinfo->passwd().length() > PASSWD_MAX_LEN || pinfo->hwid().length() > HWID_MAX_LEN)
	{
		sys_err("invalid login data (too large)");
		d->DelayedDisconnect(5);
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (pinfo->pc_name().length() > HWID_MAX_LEN || pinfo->user_name().length() > HWID_MAX_LEN)
		return;

	char szEscapedPcName[HWID_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szEscapedPcName, sizeof(szEscapedPcName), pinfo->pc_name().c_str(), pinfo->pc_name().length());

	char szEscapedUserName[HWID_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szEscapedUserName, sizeof(szEscapedUserName), pinfo->user_name().c_str(), pinfo->user_name().length());

	char szFullComputername[HWID_MAX_LEN * 4 + 3 + 1];
	sprintf(szFullComputername, "%s @ %s", szEscapedPcName, szEscapedUserName);

	std::string computer_name_real = std::string(pinfo->pc_name_real() + " @ " + pinfo->user_name_real());
	std::string pc_hash = computer_name_real + 'a' + 'b' + 'c' + '5';

	static char buf[512 + 1];
	char sas[33];
	MD5_CTX ctx;

	snprintf(buf, sizeof(buf), "%s", pc_hash.c_str());
	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char *)buf, strlen(buf));
#ifdef __FreeBSD__
	MD5End(&ctx, sas);
#else
	static const char hex[] = "0123456789abcdef";
	unsigned char digest[16];
	MD5Final(digest, &ctx);
	int i;
	for (i = 0; i < 16; ++i) {
		sas[i + i] = hex[digest[i] >> 4];
		sas[i + i + 1] = hex[digest[i] & 0x0f];
	}
	sas[i + i] = '\0';
#endif
	if (std::string(sas) != pinfo->hash())
	{
		sys_err("PCName Hash missmatch[acc %s] f1:%s f2:%s; h1:%s h2:%s", pinfo->login().c_str(), szFullComputername, computer_name_real.c_str(), sas, pinfo->hash().c_str());
	}
#endif

#ifdef __ANTI_BRUTEFORCE__
	constexpr auto bruteforce_limit_time = 60 * 20;
	// Hwid Check
	{
		auto it = m_mapWrongLoginByHWID.find(pinfo->hwid());
		if (it != m_mapWrongLoginByHWID.end())
		{
			if (it->second.first >= __ANTI_BRUTEFORCE__)
			{
				if (get_global_time() - it->second.second >= bruteforce_limit_time)
					m_mapWrongLoginByHWID.erase(it);
				else
				{
					LoginFailure(d, "BRTFORCE", bruteforce_limit_time - (get_global_time() - it->second.second));
					return;
				}
			}
			else if (it->second.second - get_global_time() >= bruteforce_limit_time)
				m_mapWrongLoginByHWID.erase(it);
		}
	}
	// New IP CHECK.. Because of FAKE_HWIDs
	{
		auto it = m_mapWrongLoginByIP.find(d->GetHostName());
		if (it != m_mapWrongLoginByIP.end())
		{
			if (it->second.first >= __ANTI_BRUTEFORCE__)
			{
				if (get_global_time() - it->second.second >= bruteforce_limit_time)
					m_mapWrongLoginByIP.erase(it);
				else
				{
					LoginFailure(d, "BRTFORCE", bruteforce_limit_time - (get_global_time() - it->second.second));
					return;
				}
			}
			else if (it->second.second - get_global_time() >= bruteforce_limit_time)
				m_mapWrongLoginByIP.erase(it);
		}
	}
#endif
	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login().c_str(), login, sizeof(login));

	char passwd[PASSWD_MAX_LEN + 1];
	strlcpy(passwd, pinfo->passwd().c_str(), sizeof(passwd));

	sys_log(0, "InputAuth::Login : %s(%d) desc %p",
			login, strlen(login), get_pointer(d));

	// check login string
	if (false == FN_IS_VALID_LOGIN_STRING(login))
	{
		sys_log(0, "InputAuth::Login : IS_NOT_VALID_LOGIN_STRING(%s) desc %p",
				login, get_pointer(d));
		LoginFailure(d, "WRONGPWD");
		return;
	}

	if (g_bNoMoreClient)
	{
		network::GCOutputPacket<network::GCLoginFailurePacket> failurePacket;

		failurePacket->set_status("SHUTDOWN");

		d->Packet(failurePacket);
		return;
	}

	if (DESC_MANAGER::instance().FindByLoginName(login))
	{
		LoginFailure(d, "ALREADY");
		return;
	}

	DWORD dwKey = DESC_MANAGER::instance().CreateLoginKey(d);

	sys_log(0, "InputAuth::Login : key %u login %s", dwKey, login);

	auto* p = new CGAuthLoginPacket(*pinfo);

	char szPasswd[PASSWD_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPasswd, sizeof(szPasswd), passwd, strlen(passwd));

	char szLogin[LOGIN_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szLogin, sizeof(szLogin), login, strlen(login));

	char szHWID[HWID_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szHWID, sizeof(szHWID), pinfo->hwid().c_str(), strlen(pinfo->hwid().c_str()));

#ifndef __TESTSERVER_NO_TABLE_CHECK__
	if (test_server && strcmp(szLogin, "admin") != 0)
	{
		std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery("SELECT name FROM common.hwid_testserver WHERE expire > NOW() AND hwid='%s'", szHWID));
		if (pMsg->Get()->uiNumRows == 0)
		{
			LoginFailure(d, "TESTSERVER");
			sys_err("no allow hwid [%s] login %s - if you want to allow insert it into common.hwid_testserver table", szHWID, login);
			return;
		}
	}
#endif

#ifdef __WIN32__
	DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
		"SELECT acc.id, LOWER(acc.login),PASSWORD('%s'),acc.password,hwid_ban.from,TIMESTAMPDIFF(SECOND, NOW(), hwid_ban.expire),acc.social_id,acc.status,acc.availDt - NOW() > 0,TIME_TO_SEC(TIMEDIFF(acc.availDt, NOW())),"
		"UNIX_TIMESTAMP(acc.create_time), acc.hwid"
#ifdef ACCOUNT_TRADE_BLOCK
		",IF(verified is null or verified = 1, 0, UNIX_TIMESTAMP(DATE_ADD(account_key.date, INTERVAL 3 DAY)))"
		",IFNULL(verified,3)"
		",IFNULL(TIMESTAMPDIFF(SECOND, NOW(), hwid_ban2.expire), 0)"
		",IFNULL(hwid_ban2.power, 0)"
#endif
		",IF(coins+special_coins > 0, 1, 0)"
		" FROM account AS acc"
		" LEFT JOIN hwid_ban ON hwid_ban.hwid = '%s' AND hwid_ban.expire > NOW() "

#ifdef ACCOUNT_TRADE_BLOCK
		" LEFT JOIN hwid_ban2 ON hwid_ban2.hwid = '%s' AND hwid_ban2.expire > NOW() "
		" LEFT JOIN account_key ON account_key.id = acc.id AND account_key.hwid = '%s' "
#endif
		" WHERE login='%s'",
#ifdef ACCOUNT_TRADE_BLOCK
		szPasswd, szHWID, szFullComputername, szHWID, szLogin);
#else
		szPasswd, szHWID, szLogin);
#endif
#else
	DBManager::instance().ReturnQuery(QID_AUTH_LOGIN, dwKey, p,
		"SELECT acc.id, LOWER(acc.login),'%s',acc.password,hwid_ban.from,TIMESTAMPDIFF(SECOND, NOW(), hwid_ban.expire),acc.social_id,acc.status,acc.availDt - NOW() > 0,TIME_TO_SEC(TIMEDIFF(acc.availDt, NOW())),"
		"UNIX_TIMESTAMP(acc.create_time), acc.hwid"
#ifdef ACCOUNT_TRADE_BLOCK
		",IF(verified is null or verified = 1, 0, UNIX_TIMESTAMP(DATE_ADD(account_key.date, INTERVAL 3 DAY)))"
		",IFNULL(verified,3)"
		",IFNULL(TIMESTAMPDIFF(SECOND, NOW(), hwid_ban2.expire), 0)"
		",IFNULL(hwid_ban2.power, 0)"
#endif
		",IF(coins+special_coins > 0, 1, 0)"
		" FROM account AS acc"
		" LEFT JOIN hwid_ban ON hwid_ban.hwid = '%s' AND hwid_ban.expire > NOW() "

#ifdef ACCOUNT_TRADE_BLOCK
		" LEFT JOIN hwid_ban2 ON hwid_ban2.hwid = '%s' AND hwid_ban2.expire > NOW() "
		" LEFT JOIN account_key ON account_key.id = acc.id AND account_key.hwid = '%s' "
#endif
		" WHERE login='%s'",
#ifdef ACCOUNT_TRADE_BLOCK
		szPasswd, szHWID, szFullComputername, szHWID, szLogin);
#else
		szPasswd, szHWID, szLogin);
#endif
#endif // __WIN32__
}

bool CInputAuth::Analyze(LPDESC d, const network::InputPacket& packet)
{
	if (!g_bAuthServer)
	{
		sys_err ("CInputAuth class is not for game server. IP %s might be a hacker.", 
			inet_ntoa(d->GetAddr().sin_addr));
		d->DelayedDisconnect(5);
		return 0;
	}

	sys_log(0, "CInputAuth::Analyze : %u", packet.get_header());

	switch (packet.get_header<TCGHeader>())
	{
		case TCGHeader::PONG:
			Pong(d);
			break;

		case TCGHeader::LOGIN_VERSION_CHECK:
			LoginVersionCheck(d, packet.get<CGLoginVersionCheckPacket>());
			break;

		case TCGHeader::AUTH_LOGIN:
			Login(d, packet.get<CGAuthLoginPacket>());
			break;

		case TCGHeader::HANDSHAKE:
			break;

		case TCGHeader::CLIENT_VERSION:
			Version(d->GetCharacter() , packet.get<CGClientVersionPacket>());
			break;

		default:
			sys_err("This phase does not handle this header %d (0x%x)(phase: AUTH)" , packet.get_header(), packet.get_header());
			return false;
	}

	return true;
}
