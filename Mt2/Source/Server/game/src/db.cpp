#include "stdafx.h"
#include <sstream>
#ifdef __DEPRECATED_BILLING__
#include "../../common/billing.h"
#endif
#include "../../common/length.h"

#include "db.h"

#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "p2p.h"
#include "log.h"
#include "login_data.h"
#include "picosha2.h"
#include "messenger_manager.h"
#include "guild_manager.h"
#include "guild.h"

#ifdef ACCOUNT_TRADE_BLOCK
#include "questmanager.h"
#endif

#ifdef __HOMEPAGE_COMMAND__
extern unsigned int g_iHomepageCommandSleepTime;
extern std::queue<std::pair<std::string, bool> > g_queueSleepHomepageCommands;
#endif

#ifdef __ANTI_BRUTEFORCE__
extern std::map<std::string, std::pair<DWORD, DWORD> > m_mapWrongLoginByHWID;
extern std::map<std::string, std::pair<DWORD, DWORD> > m_mapWrongLoginByIP;
#endif

DBManager::DBManager() : m_bIsConnect(false)
{
}

DBManager::~DBManager()
{
}

bool DBManager::Connect(const char * host, const int port, const char * user, const char * pwd, const char * db)
{
	if (m_sql.Setup(host, user, pwd, db, Locale_GetLocale().c_str(), false, port))
		m_bIsConnect = true;

	if (!m_sql_direct.Setup(host, user, pwd, db, Locale_GetLocale().c_str(), true, port))
		sys_err("cannot open direct sql connection to host %s", host);

	return m_bIsConnect;
}

void DBManager::Query(const char * c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

#ifdef USE_QUERY_LOGGING
	if (test_server)
		sys_err("Query %s", szQuery);
#endif
	m_sql.AsyncQuery(szQuery);
}

SQLMsg * DBManager::DirectQuery(const char * c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

#ifdef USE_QUERY_LOGGING
	if (test_server)
		sys_err("DirectQuery %s", szQuery);
#endif
	return m_sql_direct.DirectQuery(szQuery);
}

bool DBManager::IsConnected()
{
	return m_bIsConnect;
}

void DBManager::ReturnQuery(int iType, DWORD dwIdent, void * pvData, const char * c_pszFormat, ...)
{
	//sys_log(0, "ReturnQuery %s", c_pszQuery);
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	CReturnQueryInfo * p = M2_NEW CReturnQueryInfo;

	p->iQueryType = QUERY_TYPE_RETURN;
	p->iType = iType;
	p->dwIdent = dwIdent;
	p->pvData = pvData;

#ifdef USE_QUERY_LOGGING
	if (test_server)
		sys_err("ReturnQuery %s", szQuery);
#endif
	m_sql.ReturnQuery(szQuery, p);
}

SQLMsg * DBManager::PopResult()
{
	SQLMsg * p;

	if (m_sql.PopResult(&p))
		return p;

	return NULL;
}

void DBManager::Process()
{
	SQLMsg* pMsg = NULL;

	while ((pMsg = PopResult()))
	{
		if( NULL != pMsg->pvUserData )
		{
			switch( reinterpret_cast<CQueryInfo*>(pMsg->pvUserData)->iQueryType )
			{
				case QUERY_TYPE_RETURN:
					AnalyzeReturnQuery(pMsg);
					break;

				case QUERY_TYPE_FUNCTION:
					{
						CFuncQueryInfo* qi = reinterpret_cast<CFuncQueryInfo*>( pMsg->pvUserData );
						qi->f(pMsg);
						M2_DELETE(qi);
					}
					break;

				case QUERY_TYPE_AFTER_FUNCTION:
					{
						CFuncAfterQueryInfo* qi = reinterpret_cast<CFuncAfterQueryInfo*>( pMsg->pvUserData );
						qi->f();
						M2_DELETE(qi);
					}
					break;
			}
		}

		delete pMsg;
	}
}

CLoginData * DBManager::GetLoginData(DWORD dwKey)
{
	std::map<DWORD, CLoginData *>::iterator it = m_map_pkLoginData.find(dwKey);

	if (it == m_map_pkLoginData.end())
		return NULL;

	return it->second;
}

void DBManager::InsertLoginData(CLoginData * pkLD)
{
	m_map_pkLoginData.insert(std::make_pair(pkLD->GetKey(), pkLD));
}

void DBManager::DeleteLoginData(CLoginData * pkLD)
{
	std::map<DWORD, CLoginData *>::iterator it = m_map_pkLoginData.find(pkLD->GetKey());

	if (it == m_map_pkLoginData.end())
		return;

	sys_log(0, "DeleteLoginData %s %p", pkLD->GetLogin(), pkLD);

#ifdef __DEPRECATED_BILLING__
	mapLDBilling.erase(pkLD->GetLogin());
#endif

	M2_DELETE(it->second);
	m_map_pkLoginData.erase(it);
}

#ifdef __DEPRECATED_BILLING__
void DBManager::SetBilling(DWORD dwKey, bool bOn, bool bSkipPush)
{
	std::map<DWORD, CLoginData *>::iterator it = m_map_pkLoginData.find(dwKey);

	if (it == m_map_pkLoginData.end())
	{
		sys_err("cannot find login key %u", dwKey);
		return;
	}

	CLoginData * ld = it->second;

	itertype(mapLDBilling) it2 = mapLDBilling.find(ld->GetLogin());

	if (it2 != mapLDBilling.end())
		if (it2->second != ld)
			DeleteLoginData(it2->second);

	mapLDBilling.insert(std::make_pair(ld->GetLogin(), ld));

	if (ld->IsBilling() && !bOn && !bSkipPush)
		PushBilling(ld);

	SendLoginPing(ld->GetLogin());
	ld->SetBilling(bOn);
}

void DBManager::PushBilling(CLoginData * pkLD)
{
	TUseTime t;

	t.dwUseSec = (get_dword_time() - pkLD->GetLogonTime()) / 1000;

	if (t.dwUseSec <= 0)
		return;

	pkLD->SetLogonTime();
	long lRemainSecs = pkLD->GetRemainSecs() - t.dwUseSec;
	pkLD->SetRemainSecs(MAX(0, lRemainSecs));

	t.dwLoginKey = pkLD->GetKey();
	t.bBillType = pkLD->GetBillType();

	sys_log(0, "BILLING: PUSH %s %u type %u", pkLD->GetLogin(), t.dwUseSec, t.bBillType);

	if (t.bBillType == BILLING_IP_FREE || t.bBillType == BILLING_IP_TIME || t.bBillType == BILLING_IP_DAY)
		snprintf(t.szLogin, sizeof(t.szLogin), "%u", pkLD->GetBillID());
	else
		strlcpy(t.szLogin, pkLD->GetLogin(), sizeof(t.szLogin));

	strlcpy(t.szIP, pkLD->GetIP(), sizeof(t.szIP));

	m_vec_kUseTime.push_back(t);
}

void DBManager::FlushBilling(bool bForce)
{
	if (bForce)
	{
		std::map<DWORD, CLoginData *>::iterator it = m_map_pkLoginData.begin();

		while (it != m_map_pkLoginData.end())
		{
			CLoginData * pkLD = (it++)->second;

			if (pkLD->IsBilling())	
				PushBilling(pkLD);
		}
	}

	if (!m_vec_kUseTime.empty())
	{
		DWORD dwCount = 0;

		std::vector<TUseTime>::iterator it = m_vec_kUseTime.begin();

		while (it != m_vec_kUseTime.end())
		{
			TUseTime * p = &(*(it++));

			// DISABLE_OLD_BILLING_CODE
			if (!g_bBilling)
			{
				++dwCount;
				continue;
			}

			Query("INSERT GameTimeLog (login, type, logon_time, logout_time, use_time, ip, server) "
					"VALUES('%s', %u, DATE_SUB(NOW(), INTERVAL %u SECOND), NOW(), %u, '%s', '%s')",
					p->szLogin, p->bBillType, p->dwUseSec, p->dwUseSec, p->szIP, g_stHostname.c_str());
			// DISABLE_OLD_BILLING_CODE_END

			switch (p->bBillType)
			{
				case BILLING_FREE:
				case BILLING_IP_FREE:
					break;

				case BILLING_DAY:
					{
						if (!bForce)
						{
							TUseTime * pInfo = M2_NEW TUseTime;
							thecore_memcpy(pInfo, p, sizeof(TUseTime));
							ReturnQuery(QID_BILLING_CHECK, 0, pInfo,
									"SELECT UNIX_TIMESTAMP(LimitDt)-UNIX_TIMESTAMP(NOW()),LimitTime FROM GameTime WHERE UserID='%s'", p->szLogin);
						}
					}
					break;

				case BILLING_TIME:
					{
						Query("UPDATE GameTime SET LimitTime=LimitTime-%u WHERE UserID='%s'", p->dwUseSec, p->szLogin);

						if (!bForce)
						{
							TUseTime * pInfo = M2_NEW TUseTime;
							thecore_memcpy(pInfo, p, sizeof(TUseTime));
							ReturnQuery(QID_BILLING_CHECK, 0, pInfo,
									"SELECT UNIX_TIMESTAMP(LimitDt)-UNIX_TIMESTAMP(NOW()),LimitTime FROM GameTime WHERE UserID='%s'", p->szLogin);
						}
					}
					break;

				case BILLING_IP_DAY:
					{
						if (!bForce)
						{
							TUseTime * pInfo = M2_NEW TUseTime;
							thecore_memcpy(pInfo, p, sizeof(TUseTime));
							ReturnQuery(QID_BILLING_CHECK, 0, pInfo,
									"SELECT UNIX_TIMESTAMP(LimitDt)-UNIX_TIMESTAMP(NOW()),LimitTime FROM GameTimeIP WHERE ipid=%s", p->szLogin);
						}
					}
					break;

				case BILLING_IP_TIME:
					{
						Query("UPDATE GameTimeIP SET LimitTime=LimitTime-%u WHERE ipid=%s", p->dwUseSec, p->szLogin);

						if (!bForce)
						{
							TUseTime * pInfo = M2_NEW TUseTime;
							thecore_memcpy(pInfo, p, sizeof(TUseTime));
							ReturnQuery(QID_BILLING_CHECK, 0, pInfo,
									"SELECT UNIX_TIMESTAMP(LimitDt)-UNIX_TIMESTAMP(NOW()),LimitTime FROM GameTimeIP WHERE ipid=%s", p->szLogin);
						}
					}
					break;
			}

			if (!bForce && ++dwCount >= 1000)
				break;
		}

		if (dwCount < m_vec_kUseTime.size())
		{   
			int nNewSize = m_vec_kUseTime.size() - dwCount;
			thecore_memcpy(&m_vec_kUseTime[0], &m_vec_kUseTime[dwCount], sizeof(TUseTime) * nNewSize);
			m_vec_kUseTime.resize(nNewSize);
		}
		else
			m_vec_kUseTime.clear();

		sys_log(0, "FLUSH_USE_TIME: count %u", dwCount);
	}

	if (m_vec_kUseTime.size() < 10240)
	{
		DWORD dwCurTime = get_dword_time();

		std::map<DWORD, CLoginData *>::iterator it = m_map_pkLoginData.begin();

		while (it != m_map_pkLoginData.end())
		{
			CLoginData * pkLD = (it++)->second;

			if (!pkLD->IsBilling())
				continue;

			switch (pkLD->GetBillType())
			{
				case BILLING_IP_FREE:
				case BILLING_FREE:
					break;

				case BILLING_IP_DAY:
				case BILLING_DAY:
				case BILLING_IP_TIME:
				case BILLING_TIME:
					if (pkLD->GetRemainSecs() < 0)
					{
						DWORD dwSecsConnected = (dwCurTime - pkLD->GetLogonTime()) / 1000;

						if (dwSecsConnected % 10 == 0)
							SendBillingExpire(pkLD->GetLogin(), BILLING_DAY, 0, pkLD, 1);
					}
					else if (pkLD->GetRemainSecs() <= 600) // if remain seconds lower than 10 minutes
					{
						DWORD dwSecsConnected = (dwCurTime - pkLD->GetLogonTime()) / 1000;

						if (dwSecsConnected >= 60) // 60 second cycle
						{
							sys_log(0, "BILLING 1 %s remain %d connected secs %u",
									pkLD->GetLogin(), pkLD->GetRemainSecs(), dwSecsConnected);
							PushBilling(pkLD);
						}
					}
					else
					{
						DWORD dwSecsConnected = (dwCurTime - pkLD->GetLogonTime()) / 1000;

						if (dwSecsConnected > (DWORD) (pkLD->GetRemainSecs() - 600) || dwSecsConnected >= 600)
						{
							sys_log(0, "BILLING 2 %s remain %d connected secs %u",
									pkLD->GetLogin(), pkLD->GetRemainSecs(), dwSecsConnected);
							PushBilling(pkLD);
						}
					}
					break;
			}
		}
	}

}

void DBManager::CheckBilling()
{
	std::vector<DWORD> vec;
	vec.push_back(0); // Ä«¿îÆ®¸¦ À§ÇØ ¹Ì¸® ºñ¿öµÐ´Ù.

	//sys_log(0, "CheckBilling: map size %d", m_map_pkLoginData.size());

	itertype(m_map_pkLoginData) it = m_map_pkLoginData.begin();

	while (it != m_map_pkLoginData.end())
	{
		CLoginData * pkLD = (it++)->second;

		if (pkLD->IsBilling())
		{
			sys_log(0, "BILLING: CHECK %u", pkLD->GetKey());
			vec.push_back(pkLD->GetKey());
		}
	}

	vec[0] = vec.size() - 1; // ºñ¿öµÐ °÷¿¡ »çÀÌÁî¸¦ ³Ö´Â´Ù, »çÀÌÁî ÀÚ½ÅÀº Á¦¿ÜÇØ¾ß ÇÏ¹Ç·Î -1

	network::GDOutputPacket<network::GDBillingCheckPacket> pkg;
	for (DWORD key : vec)
		pkg->add_keys(key);
	db_clientdesc->DBPacket(pkg);
}
#endif

void DBManager::SendLoginPing(const char * c_pszLogin)
{
	
	if (strlen(c_pszLogin) < 3) // P2P Crack spam...
	{
		sys_err("%s:%d c_pszLogin : '%s' strlen() DONT BROADCAST PACKET", __FILE__, __LINE__, c_pszLogin, strlen(c_pszLogin));
		return;
	}
	else if (!check_name(c_pszLogin)) // P2P Crack spam...
	{
		sys_err("%s:%d check_name(c_pszLogin) : '%s' failed..  DONT BROADCAST PACKET", __FILE__, __LINE__, c_pszLogin, strlen(c_pszLogin));
		return;
	}
	
	network::GGOutputPacket<network::GGLoginPingPacket> ptog;

	ptog->set_login(c_pszLogin);

	if (!g_pkAuthMasterDesc)  // If I am master, broadcast to others
	{
		P2P_MANAGER::instance().Send(ptog);
	}
	else // If I am slave send login ping to master
	{
		g_pkAuthMasterDesc->Packet(ptog);
	}
}

void DBManager::SendAuthLogin(LPDESC d)
{
	auto& r = d->GetAccountTable();

	CLoginData * pkLD = GetLoginData(d->GetLoginKey());

	if (!pkLD)
		return;

	network::GDOutputPacket<network::GDAuthLoginPacket> ptod;
	ptod->set_account_id(r.id());
	
	char login_name[LOGIN_MAX_LEN + 1];
	trim_and_lower(r.login().c_str(), login_name, sizeof(login_name));
	ptod->set_login(login_name);

	ptod->set_social_id(r.social_id());
	ptod->set_hwid(r.hwid());
	ptod->set_login_key(d->GetLoginKey());
#ifdef __DEPRECATED_BILLING__
	ptod->set_bill_type(pkLD->GetBillType());
	ptod->set_bill_id(pkLD->GetBillID());
#endif
	ptod->set_language(r.language());
#ifdef ACCOUNT_TRADE_BLOCK
	ptod->set_tradeblock(r.tradeblock());
	ptod->set_hwid2ban(r.hwid2ban());
	ptod->set_hwid2(r.hwid2());
#endif
	ptod->set_coins(r.coins());
	ptod->set_temp_login(r.temp_login());

#ifdef CHECK_IP_ON_CONNECT
	ptod->set_ip(d->GetHostName());
#endif

	for (int i = 0; i < PREMIUM_MAX_NUM; ++i)
		ptod->add_premium_times(pkLD->GetPremium(i));
	for (int i = 0; i < PACK_CLIENT_KEY_COUNT; ++i)
		ptod->add_client_keys(pkLD->GetClientKey()[i]);

	db_clientdesc->DBPacket(ptod, d->GetHandle());
	sys_log(0, "SendAuthLogin %s acc %u", ptod->login().c_str(), ptod->account_id());

	SendLoginPing(r.login().c_str());
}

#ifdef __DEPRECATED_BILLING__
void DBManager::LoginPrepare(BYTE bBillType, DWORD dwBillID, long lRemainSecs, LPDESC d, DWORD * pdwClientKey, int * paiPremiumTimes)
#else
void DBManager::LoginPrepare(LPDESC d, DWORD* pdwClientKey, int* paiPremiumTimes)
#endif
{
	auto& r = d->GetAccountTable();

	CLoginData * pkLD = M2_NEW CLoginData;

	pkLD->SetKey(d->GetLoginKey());
	pkLD->SetLogin(r.login().c_str());
#ifdef __DEPRECATED_BILLING__
	pkLD->SetBillType(bBillType);
	pkLD->SetBillID(dwBillID);
	pkLD->SetRemainSecs(lRemainSecs);
#endif
	pkLD->SetIP(d->GetHostName());
	pkLD->SetClientKey(pdwClientKey);

	if (paiPremiumTimes)
		pkLD->SetPremium(paiPremiumTimes);

	InsertLoginData(pkLD);

	SendAuthLogin(d);
}

#ifdef __DEPRECATED_BILLING__
bool GetGameTimeIP(MYSQL_RES * pRes, BYTE & bBillType, DWORD & dwBillID, int & seconds, const char * c_pszIP)
{
	if (!pRes)
		return true;

	MYSQL_ROW row = mysql_fetch_row(pRes);
	int col = 0;

	str_to_number(dwBillID, row[col++]);

	int ip_start = 0;
	str_to_number(ip_start, row[col++]);

	int ip_end = 0;
	str_to_number(ip_end, row[col++]);

	int type = 0;
	str_to_number(type, row[col++]);

	str_to_number(seconds, row[col++]);

	int day_seconds = 0;
	str_to_number(day_seconds, row[col++]);

	char szIP[MAX_HOST_LENGTH + 1];
	strlcpy(szIP, c_pszIP, sizeof(szIP));

	char * p = strrchr(szIP, '.');
	++p;

	int ip_postfix = 0;
	str_to_number(ip_postfix, p);
	int valid_ip = false;

	if (ip_start <= ip_postfix && ip_end >= ip_postfix)
		valid_ip = true;

	bBillType = BILLING_NONE;

	if (valid_ip)
	{
		if (type == -1)
			return false;

		if (type == 0)
			bBillType = BILLING_IP_FREE;
		else if (day_seconds > 0)
		{
			bBillType = BILLING_IP_DAY;
			seconds = day_seconds;
		}
		else if (seconds > 0)
			bBillType = BILLING_IP_TIME;
	}

	return true;
}

bool GetGameTime(MYSQL_RES * pRes, BYTE & bBillType, int & seconds)
{
	if (!pRes)
		return true;

	MYSQL_ROW row = mysql_fetch_row(pRes);
	sys_log(1, "GetGameTime %p %p %p", row[0], row[1], row[2]);

	int type = 0;
	str_to_number(type, row[0]);
	str_to_number(seconds, row[1]);
	int day_seconds = 0;
	str_to_number(day_seconds, row[2]);
	bBillType = BILLING_NONE;

	if (type == -1)
		return false;
	else if (type == 0)
		bBillType = BILLING_FREE;
	else if (day_seconds > 0)
	{
		bBillType = BILLING_DAY;
		seconds = day_seconds;
	}
	else if (seconds > 0)
		bBillType = BILLING_TIME;

	if (!g_bBilling)
		bBillType = BILLING_FREE;

	return true;
}

void SendBillingExpire(const char * c_pszLogin, BYTE bBillType, int iSecs, CLoginData * pkLD, DWORD num)
{
	if (strlen(c_pszLogin) < 3) // P2P Crack spam...
	{
		sys_err("%s:%d c_pszLogin : '%s' strlen()", __FILE__, __LINE__, c_pszLogin, strlen(c_pszLogin));
		return;
	}
	
	network::GDOutputPacket<network::GDBillingExpirePacket> ptod;
	ptod->set_login(c_pszLogin);
	ptod->set_bill_type(bBillType);
	ptod->set_remain_seconds(MAX(0, iSecs));
	db_clientdesc->DBPacket(ptod);
	sys_log(0, "BILLING: EXPIRE #%d login:'%s' type %d sec %d ptr %p", num, c_pszLogin, bBillType, iSecs, pkLD);
}
#endif

void DBManager::AnalyzeReturnQuery(SQLMsg * pMsg)
{
	CReturnQueryInfo * qi = (CReturnQueryInfo *) pMsg->pvUserData;

	switch (qi->iType)
	{
		case QID_AUTH_LOGIN:
			{
				network::CGAuthLoginPacket * pinfo = (network::CGAuthLoginPacket *) qi->pvData;
				LPDESC d = DESC_MANAGER::instance().FindByLoginKey(qi->dwIdent);

				if (!d)
				{
					M2_DELETE(pinfo);
					break;
				}
				//À§Ä¡ º¯°æ - By SeMinZ
				d->SetLogin(pinfo->login());

				sys_log(0, "QID_AUTH_LOGIN: START %u %p", qi->dwIdent, get_pointer(d));

#ifdef ACCOUNT_TRADE_BLOCK
				char szHWID2[HWID2_MAX_LEN * 2 + 1];
				char szEscapedPcName[HWID_MAX_LEN * 2 + 1];
				DBManager::instance().EscapeString(szEscapedPcName, sizeof(szEscapedPcName), pinfo->pc_name().c_str(), pinfo->pc_name().length());
				char szEscapedUserName[HWID_MAX_LEN * 2 + 1];
				DBManager::instance().EscapeString(szEscapedUserName, sizeof(szEscapedUserName), pinfo->user_name().c_str(), pinfo->user_name().length());
				sprintf(szHWID2, "%s @ %s", szEscapedPcName, szEscapedUserName);
#else
				char szHWID2[50 * 2 + 1];
				strlcpy(szHWID2, "", sizeof(szHWID2));
#endif

				if (pMsg->Get()->uiNumRows == 0)
				{
#ifdef __ANTI_BRUTEFORCE__
					// HWID
					{
						auto it = m_mapWrongLoginByHWID.find(pinfo->hwid().c_str());
						if (it == m_mapWrongLoginByHWID.end())
							m_mapWrongLoginByHWID[pinfo->hwid().c_str()] = std::make_pair(2, get_global_time());
						else
							it->second.first += 2;
					}
					// IP
					{
						auto it = m_mapWrongLoginByIP.find(d->GetHostName());
						if (it == m_mapWrongLoginByIP.end())
							m_mapWrongLoginByIP[d->GetHostName()] = std::make_pair(2, get_global_time());
						else
							it->second.first += 2;
					}
					if (test_server)
						sys_err("__ANTI_BRUTEFORCE__: %d", m_mapWrongLoginByHWID[pinfo->hwid().c_str()].first);
#endif
					sys_log(0, "   NOID");
					LoginFailure(d, "WRONGPWD");
					LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "NOID", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
					M2_DELETE(pinfo);
				}
				else
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
					int col = 0;

					// id, login, '%s', password, securitycode, social_id, status
					DWORD dwID = 0;
					char szLoginID[26] = { 0, };
					char szEncrytPassword[64 * 2] = { 0, };
					char szPassword[64 * 2 + 1] = { 0, };
					char szSocialID[SOCIAL_ID_MAX_LEN + 1];
					char szStatus[ACCOUNT_STATUS_MAX_LEN + 1];
					char szHWID[HWID_MAX_LEN + 1];

					if (!row[col])
					{
						sys_err("error column %d", col);
						M2_DELETE(pinfo);
						break;
					}
					// sys_log(0, "%dcol[%d]=%s",__LINE__, col, row[col]);
					str_to_number(dwID, row[col++]);

					if (!row[col])
					{
						sys_err("error column %d", col);
						M2_DELETE(pinfo);
						break;
					}

					// sys_log(0, "%dcol[%d]=%s",__LINE__, col, row[col]);
					strlcpy(szLoginID, row[col++], sizeof(szLoginID));

					if (!row[col])
					{
						sys_err("error column %d", col);
						M2_DELETE(pinfo);
						break;
					}

					// sys_log(0, "%dcol[%d]=%s",__LINE__, col, row[col]);
					const char* szUnencryptedPassword = row[col++];
					char szEncryptBuffer[100];
					strlcpy(szEncryptBuffer, szLoginID, sizeof(szEncryptBuffer));
					strcat(szEncryptBuffer, szUnencryptedPassword);
					snprintf(szEncrytPassword, sizeof(szEncrytPassword), "%s", picosha2::hash256_hex_string(szEncryptBuffer, szEncryptBuffer + strlen(szEncryptBuffer)).c_str());
					sys_err("New Pass: %s", szEncrytPassword);

					if (!row[col])
					{
						sys_err("error column %d", col);
						M2_DELETE(pinfo);
						break;
					}

					strlcpy(szPassword, row[col++], sizeof(szPassword));

					if (row[col++])
					{
						int iTimeLeft;
						str_to_number(iTimeLeft, row[col++]);

#ifdef __ANTI_BRUTEFORCE__
						// HWID
						{
							auto it = m_mapWrongLoginByHWID.find(pinfo->hwid().c_str());
							if (it == m_mapWrongLoginByHWID.end())
								m_mapWrongLoginByHWID[pinfo->hwid().c_str()] = std::make_pair(10, get_global_time());
							else
								it->second.first += 10;
						}
						// IP
						{
							auto it = m_mapWrongLoginByIP.find(d->GetHostName());
							if (it == m_mapWrongLoginByIP.end())
								m_mapWrongLoginByIP[d->GetHostName()] = std::make_pair(10, get_global_time());
							else
								it->second.first += 10;
						}
#endif
						LoginFailure(d, "HWIDBAN", iTimeLeft);
						sys_log(0, "   HWIDBAN %d", iTimeLeft);
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "HWIDBAN", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
						break;
					}
					++col;

					if (!row[col])
					{
						sys_err("error column %d", col);
						M2_DELETE(pinfo);
						break;
					}

					strlcpy(szSocialID, row[col++], sizeof(szSocialID));

					if (!row[col])
					{
						sys_err("error column %d", col);
						M2_DELETE(pinfo);
						break;
					}

					strlcpy(szStatus, row[col++], sizeof(szStatus));

					BYTE bNotAvail = 0;
					str_to_number(bNotAvail, row[col++]);

					int iAvailTimeDif = 0;
					str_to_number(iAvailTimeDif, row[col++]);

					int aiPremiumTimes[PREMIUM_MAX_NUM];
					memset(&aiPremiumTimes, 0, sizeof(aiPremiumTimes));

					char szCreateDate[256] = "00000000";

					aiPremiumTimes[PREMIUM_EXP] = get_global_time() + 60 * 60 * 24 * 365;
					aiPremiumTimes[PREMIUM_ITEM] = get_global_time() + 60 * 60 * 24 * 365;
					aiPremiumTimes[PREMIUM_AUTOLOOT] = get_global_time() + 60 * 60 * 24 * 365;

					long retValue = 0;
					str_to_number(retValue, row[col++]);

					time_t create_time = retValue;
					struct tm * tm1;
					tm1 = localtime(&create_time);
					strftime(szCreateDate, 255, "%Y%m%d", tm1);

					sys_log(0, "Create_Time %d %s", retValue, szCreateDate);

					strlcpy(szHWID, row[col++], sizeof(szHWID));

#ifdef ACCOUNT_TRADE_BLOCK
					DWORD dwTradebock = 0;
					str_to_number(dwTradebock, row[col++]);
					BYTE bVerifiedHWID = 0;
					str_to_number(bVerifiedHWID, row[col++]);

					if (bVerifiedHWID == 1)
						dwTradebock = 0;

					DWORD dwHWIDBan2 = 0;
					str_to_number(dwHWIDBan2, row[col++]);

					BYTE bHWIDBan2Power = 0;
					str_to_number(bHWIDBan2Power, row[col++]);
#endif

					bool coins = false;
					str_to_number(coins, row[col++]);
					
					// test_server no password check
					int nPasswordDiff = strcmp(szEncrytPassword, szPassword) && !test_server;

					if (nPasswordDiff)
					{
#ifdef __ANTI_BRUTEFORCE__
						// HWID
						{
							auto it = m_mapWrongLoginByHWID.find(pinfo->hwid().c_str());
							if (it == m_mapWrongLoginByHWID.end())
								m_mapWrongLoginByHWID[pinfo->hwid().c_str()] = std::make_pair(1, get_global_time());
							else
								it->second.first += 1;
						}	
						// IP
						{
							auto it = m_mapWrongLoginByIP.find(d->GetHostName());
							if (it == m_mapWrongLoginByIP.end())
								m_mapWrongLoginByIP[d->GetHostName()] = std::make_pair(1, get_global_time());
							else
								it->second.first += 1;
						}
#endif
						LoginFailure(d, "WRONGPWD");
						sys_log(0, "   WRONGPWD");
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "WRONGPWD", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
					else if (strcmp(szStatus, "OK") && !strcmp(szStatus, "BLOCK"))
					{
						LoginFailure(d, szStatus);
						sys_log(0, "   STATUS: %s", szStatus);
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), szStatus, pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
					else if (bNotAvail)
					{
						LoginFailure(d, "NOTAVAIL", iAvailTimeDif);
						sys_log(0, "   NOTAVAIL %d", iAvailTimeDif);
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "NOTAVAIL", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
					else if (DESC_MANAGER::instance().FindByLoginName(pinfo->login().c_str()))
					{
						LoginFailure(d, "ALREADY");
						sys_log(0, "   ALREADY");
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "ALREADY", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
					else if (strcmp(szStatus, "OK"))
					{
						LoginFailure(d, szStatus);
						sys_log(0, "   STATUS: %s", szStatus);
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), szStatus, pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
#ifdef ACCOUNT_TRADE_BLOCK
					/*else if (bHWIDBan2Power == 1)
					{
						LoginFailure(d, "WRONGPWD");
						sys_log(0, "   bHWIDBan2Power");
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "HWIDBAN2", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
					else if (bHWIDBan2Power == 2)
					{
						LoginFailure(d, "ALREADY");
						sys_log(0, "   bHWIDBan2Power");
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "HWIDBAN2", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
					else if (bHWIDBan2Power == 3)
					{
						LoginFailure(d, "HWIDBAN");
						sys_log(0, "   bHWIDBan2Power");
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "HWIDBAN2", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}*/
					else if (bHWIDBan2Power > 0)
					{
						LoginFailure(d, "OFFLINE");
						sys_log(0, "   bHWIDBan2Power");
						LogManager::instance().LoginFailLog(pinfo->hwid().c_str(), "HWIDBAN2", pinfo->login().c_str(), pinfo->passwd().c_str(), d->GetHostName(), szHWID2);
						M2_DELETE(pinfo);
					}
#endif
					else
					{
						char szEscapedHWID[HWID_MAX_LEN * 2 + 1];
						DBManager::instance().EscapeString(szEscapedHWID, sizeof(szEscapedHWID), pinfo->hwid().c_str(), strlen(pinfo->hwid().c_str()));

						auto& r = d->GetAccountTable();
						char szQuery[1024];
#ifdef ACCOUNT_TRADE_BLOCK
						if (!strcmp(szHWID, ""))
						{
							char szQuery2[1024];
							snprintf(szQuery2, sizeof(szQuery2), "INSERT INTO account_key (id, verified, hwid, ip, pc_name) VALUES(%d, 1, '%s', '%s', '%s')", 
																dwID, szEscapedHWID, d->GetHostName(), szHWID2);
							DBManager::instance().Query(szQuery2);
						}
						else if (bVerifiedHWID == 3 && strcmp(szHWID, pinfo->hwid().c_str()) != 0 && strcmp(szHWID, "") != 0 && quest::CQuestManager::instance().GetEventFlag("acctradeblock_disabled") == 0)
						{
							sys_log(0, "Set Trade Block! ['%s' => '%s']", szHWID, pinfo->hwid().c_str());
							dwTradebock = ACCOUNT_TRADE_BLOCK * 60 * 60 + get_global_time();

							BYTE verificationStatus = 0;

							char szQueryCheck[2048];
							snprintf(szQueryCheck, sizeof(szQuery), "SELECT id FROM player.account_key WHERE id=%d AND (pc_name='%s' OR ip='%s') AND verified=1", dwID, szHWID2, d->GetHostName());
							std::auto_ptr<SQLMsg> pkMsg( DBManager::instance().DirectQuery(szQueryCheck) );
							SQLResult * pRes = pkMsg->Get();
							if (pRes->uiNumRows > 0)
							{
								verificationStatus = 1;
								dwTradebock = 0;
							}

							char szQuery2[1024];
							snprintf(szQuery2, sizeof(szQuery2), "INSERT INTO account_key (id, verified, hwid, ip, pc_name) VALUES(%d, %d, '%s', '%s', '%s')", 
																dwID, verificationStatus, szEscapedHWID, d->GetHostName(), szHWID2);
							DBManager::instance().Query(szQuery2);
						}
#endif
						snprintf(szQuery, sizeof(szQuery), "UPDATE account SET last_play=NOW(), language=%d, hwid='%s' WHERE id=%u", pinfo->language(), szEscapedHWID, dwID);

						std::auto_ptr<SQLMsg> msg( DBManager::instance().DirectQuery(szQuery) );

						r.set_id(dwID);

						char login_name[LOGIN_MAX_LEN + 1];
						trim_and_lower(pinfo->login().c_str(), login_name, sizeof(login_name));
						r.set_login(login_name);

						r.set_passwd(pinfo->passwd());
						r.set_social_id(szSocialID);
						r.set_hwid(pinfo->hwid());
						r.set_language(MIN(pinfo->language(), LANGUAGE_MAX_NUM - 1));
#ifdef ACCOUNT_TRADE_BLOCK
						r.set_tradeblock(dwTradebock);
						r.set_hwid2ban(dwHWIDBan2);
						r.set_hwid2(szHWID2);
#endif
						r.set_coins(coins);
						r.set_temp_login(!strcmp(szUnencryptedPassword, "Temp"));
						DESC_MANAGER::instance().ConnectAccount(r.login(), d);

#ifdef __DEPRECATED_BILLING__
						if (!g_bBilling)
						{
							DWORD client_keys[PACK_CLIENT_KEY_COUNT];
							for (int i = 0; i < PACK_CLIENT_KEY_COUNT; ++i)
								client_keys[i] = pinfo->client_keys(i);

							LoginPrepare(BILLING_FREE, 0, 0, d, client_keys, aiPremiumTimes);
							M2_DELETE(pinfo);
							break;
						}
#else
						DWORD client_keys[PACK_CLIENT_KEY_COUNT];
						for (int i = 0; i < PACK_CLIENT_KEY_COUNT; ++i)
							client_keys[i] = pinfo->client_keys(i);

						LoginPrepare(d, client_keys, aiPremiumTimes);
						M2_DELETE(pinfo);
#endif

						//sys_log(0, "QID_AUTH_LOGIN: SUCCESS %s", pinfo->login().c_str());
						sys_log(0, "QID_AUTH_LOGIN: SUCCESS");
					}
				}
			}
			break;

#ifdef __DEPRECATED_BILLING__
		case QID_BILLING_CHECK:
			{
				TUseTime * pinfo = (TUseTime *) qi->pvData;
				int iRemainSecs = 0;

				CLoginData * pkLD = NULL;

				if (pMsg->Get()->uiNumRows > 0)
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
					
					int iLimitDt = 0;
					str_to_number(iLimitDt, row[0]);

					int iLimitTime = 0;
					str_to_number(iLimitTime, row[1]);

					pkLD = GetLoginData(pinfo->dwLoginKey);

					if (pkLD)
					{
						switch (pkLD->GetBillType())
						{
							case BILLING_TIME:
								if (iLimitTime <= 600 && iLimitDt > 0)
								{
									iRemainSecs = iLimitDt;
									pkLD->SetBillType(BILLING_DAY);
									pinfo->bBillType = BILLING_DAY;
								}
								else
									iRemainSecs = iLimitTime;
								break;

							case BILLING_IP_TIME:
								if (iLimitTime <= 600 && iLimitDt > 0)
								{
									iRemainSecs = iLimitDt;
									pkLD->SetBillType(BILLING_IP_DAY);
									pinfo->bBillType = BILLING_IP_DAY;
								}
								else
									iRemainSecs = iLimitTime;
								break;

							case BILLING_DAY:
							case BILLING_IP_DAY:
								iRemainSecs = iLimitDt;
								break;
						}

						pkLD->SetRemainSecs(iRemainSecs);
					}
				}

				SendBillingExpire(pinfo->szLogin, pinfo->bBillType, MAX(0, iRemainSecs), pkLD, 2);
				M2_DELETE(pinfo);
			}
			break;
#endif

		case QID_MESSENGER_LIST_LOAD:
		{
			MessengerManager::instance().LoadList(pMsg);
			break;
		}
		case QID_GUILD_LOAD_DATA:
		{
			CGuild* guild = CGuildManager::instance().FindGuild(qi->dwIdent);
			if (guild)
				guild->LoadGuildData(pMsg);
			break;
		}
		case QID_GUILD_LOAD_GRADE:
		{
			CGuild* guild = CGuildManager::instance().FindGuild(qi->dwIdent);
			if (guild)
				guild->LoadGuildGradeData(pMsg);
			break;
		}
		case QID_GUILD_LOAD_MEMBER:
		{
			CGuild* guild = CGuildManager::instance().FindGuild(qi->dwIdent);
			if (guild)
				guild->LoadGuildMemberData(pMsg);
			break;
		}
		case QID_GUILD_CHANGE_GRADE:
		{
			CGuild* guild = CGuildManager::instance().FindGuild(qi->dwIdent);
			if (guild)
				guild->P2PUpdateGrade(pMsg);
			break;
		}
		case QID_GUILD_CHANGE_LEADER:
		{
			CGuild* guild = CGuildManager::instance().FindGuild(qi->dwIdent);
			if (guild)
				guild->SendGuildDataUpdateToAllMember(pMsg);
			break;
		}
		
		case QID_SAFEBOX_SIZE:
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(qi->dwIdent);

				if (ch)
				{
					if (pMsg->Get()->uiNumRows > 0)
					{
						MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
						int	size = 0;
						str_to_number(size, row[0]);
						bool bNeedPassword = false;
						str_to_number(bNeedPassword, row[1]);
						ch->SetSafeboxSize(SAFEBOX_PAGE_SIZE * size);
						ch->SetSafeboxNeedPassword(bNeedPassword);

						if (test_server)
							sys_log(0, "CHAR[%s] SAFEBOX_SIZE: %d; PWD: %d (row[1] = %s)", ch->GetName(), size, bNeedPassword, row[1]);
						//if (size)
						//	ch->ChatPacket(CHAT_TYPE_COMMAND, "EnableSafebox");
					}

					if (ch->GetSafeboxSize() <= 0)
					{
						network::GDOutputPacket<network::GDSafeboxChangeSizePacket> p;
						p->set_account_id(ch->GetDesc()->GetAccountTable().id());
						p->set_size(3);
						db_clientdesc->DBPacket(p, ch->GetDesc()->GetHandle());

						ch->SetSafeboxSize(SAFEBOX_PAGE_SIZE * p->size());
					}
				}
			}
			break;

			// BLOCK_CHAT
		case QID_BLOCK_CHAT_LIST:
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(qi->dwIdent);
				
				if (ch == NULL)
					break;
				if (pMsg->Get()->uiNumRows)
				{
					MYSQL_ROW row;
					while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "%s %s sec", row[0], row[1]);
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "No one currently blocked.");
				}
			}
			break;
			// END_OF_BLOCK_CHAT

#ifdef __HOMEPAGE_COMMAND__
		case QID_PROCESS_HOMEPAGE_COMMANDS:
		{
			if (pMsg->Get()->uiNumRows == 0)
				return;

			char szCommand[CHAT_MAX_LEN];
			char szDate[50];
			bool bForwarding;
			while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
			{
				int col = 0;

				strlcpy(szCommand, row[col++], sizeof(szCommand));
				strlcpy(szDate, row[col++], sizeof(szDate));
				if (row[col] && *row[col])
					bForwarding = true;
				else
					bForwarding = false;
				++col;

				if (g_iHomepageCommandSleepTime)
				{
					sys_log(0, "QUEUE command [%s, %d] (sleep time %u dword_time %u)", szCommand, bForwarding, g_iHomepageCommandSleepTime, get_dword_time());

					g_queueSleepHomepageCommands.push(std::make_pair(szCommand, bForwarding));
				}
				else
				{
					if (bForwarding)
					{
						sys_log(0, "HomepageCommand FORWARDING command [%s] (isForwarding %d \"%s\")", szCommand, bForwarding, row[col - 1]);

						network::GGOutputPacket<network::GGHomepageCommandPacket> kCommandPacket;
						kCommandPacket->set_command(szCommand);
						P2P_MANAGER::instance().Send(kCommandPacket);
					}

					direct_interpret_command(szCommand);
				}
			}

			DBManager::instance().Query("UPDATE homepage_command SET executed = 1 WHERE date <= '%s'", szDate);
		}
		break;
#endif

		case QID_UPPITEM_LIST_RELOAD:
		{
			ITEM_MANAGER::instance().OnLoadUppItemList(pMsg->Get()->pSQLResult);
		}
		break;

		case QID_PULL_OFFLINE_MESSAGES:
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(qi->dwIdent);
			if (!ch)
				break;

			DBManager::instance().Query("DELETE FROM offline_messages WHERE pid = %u", qi->dwIdent);

			google::protobuf::RepeatedPtrField<network::TOfflineMessage> offlineMessageList;
			while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
			{
				int col = 0;

				network::TOfflineMessage& kMessage = *offlineMessageList.Add();
				kMessage.set_sender(row[col++]);
				kMessage.set_message(row[col++]);
				kMessage.set_is_gm(row[col++] != nullptr);
			}

			ch->LoadOfflineMessages(offlineMessageList);
			ch->SendOfflineMessages();
		}
		break;

#ifdef __VOTE4BUFF__
		case QID_V4B_LOAD:
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(qi->dwIdent);
			if (!ch)
				break;

			if (!pMsg->Get()->uiNumRows)
			{
				ch->V4B_SetLoaded();
				break;
			}

			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

			int col = 0;

			DWORD dwTimeEnd;
			str_to_number(dwTimeEnd, row[col++]);
			BYTE bApplyType;
			str_to_number(bApplyType, row[col++]);
			int iApplyValue;
			str_to_number(iApplyValue, row[col++]);

			ch->V4B_GiveBuff(dwTimeEnd, bApplyType, iApplyValue);
			ch->V4B_SetLoaded();
		}
		break;
#endif
		case QID_GUILD_LOAD_MEMBER_LASTPLAY:
		{
			if (pMsg->Get()->uiNumRows == 0)
				return;
			
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(qi->dwIdent);
			if (!ch)
				break;

			ch->GetGuild()->SendLastplayedListPacket(ch, pMsg);
		}

		default:
			sys_err("FATAL ERROR!!! Unhandled return query id %d", qi->iType);
			break;
	}

	M2_DELETE(qi);
}

#ifdef __DEPRECATED_BILLING__
void DBManager::StopAllBilling()
{
	for (itertype(m_map_pkLoginData) it = m_map_pkLoginData.begin(); it != m_map_pkLoginData.end(); ++it)
	{
		SetBilling(it->first, false);
	}
}
#endif

size_t DBManager::EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize)
{
	return m_sql_direct.EscapeString(dst, dstSize, src, srcSize);
}

//
// Common SQL 
//
AccountDB::AccountDB() :
	m_IsConnect(false)
{
}

bool AccountDB::IsConnected()
{
	return m_IsConnect;
}

bool AccountDB::Connect(const char * host, const int port, const char * user, const char * pwd, const char * db)
{
	m_IsConnect = m_sql_direct.Setup(host, user, pwd, db, "utf8", true, port);

	if (false == m_IsConnect)
	{
		fprintf(stderr, "cannot open direct sql connection to host: %s user: %s db: %s\n", host, user, db);
		return false;
	}

	return m_IsConnect;
}

bool AccountDB::ConnectAsync(const char * host, const int port, const char * user, const char * pwd, const char * db, const char * locale)
{
	m_sql.Setup(host, user, pwd, db, locale, false, port);
	return true;
}

void AccountDB::SetLocale(const std::string & stLocale)
{
	m_sql_direct.SetLocale(stLocale);
	m_sql_direct.QueryLocaleSet();
}

SQLMsg* AccountDB::DirectQuery(const char * query)
{
#ifdef USE_QUERY_LOGGING
	if (test_server)
		sys_err("DirectQuery %s", query);
#endif
	return m_sql_direct.DirectQuery(query);
}

void AccountDB::AsyncQuery(const char* query)
{
#ifdef USE_QUERY_LOGGING
	if (test_server)
		sys_err("DirectQuery %s", query);
#endif
	m_sql.AsyncQuery(query);
}

void AccountDB::ReturnQuery(int iType, DWORD dwIdent, void * pvData, const char * c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	CReturnQueryInfo * p = M2_NEW CReturnQueryInfo;

	p->iQueryType = QUERY_TYPE_RETURN;
	p->iType = iType;
	p->dwIdent = dwIdent;
	p->pvData = pvData;

#ifdef USE_QUERY_LOGGING
	if (test_server)
		sys_err("DirectQuery %s", szQuery);
#endif
	m_sql.ReturnQuery(szQuery, p);
}

SQLMsg * AccountDB::PopResult()
{
	SQLMsg * p;

	if (m_sql.PopResult(&p))
		return p;

	return NULL;
}

void AccountDB::Process()
{
	SQLMsg* pMsg = NULL;

	while ((pMsg = PopResult()))
	{
		CQueryInfo* qi = (CQueryInfo *) pMsg->pvUserData;

		switch (qi->iQueryType)
		{
			case QUERY_TYPE_RETURN:
				AnalyzeReturnQuery(pMsg);
				break;
		}
	}

	delete pMsg;
}

void AccountDB::AnalyzeReturnQuery(SQLMsg * pMsg)
{
	CReturnQueryInfo * qi = (CReturnQueryInfo *) pMsg->pvUserData;
	M2_DELETE(qi);
}