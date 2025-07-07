
#include "stdafx.h"

#include "ClientManager.h"

#include "Main.h"
#include "Config.h"
#include "QID.h"
#include "Cache.h"

extern bool CreatePlayerTableFromRes(MYSQL_RES * res, TPlayerTable * pkTab);
extern int g_test_server;
extern int g_log;

bool CClientManager::InsertLogonAccount(const char * c_pszLogin, DWORD dwHandle, const char * c_pszIP)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	auto it = m_map_kLogonAccount.find(szLogin);

	if (m_map_kLogonAccount.end() != it)
		return false;

	CLoginData * pkLD = GetLoginDataByLogin(c_pszLogin);

	if (!pkLD)
		return false;

	pkLD->SetConnectedPeerHandle(dwHandle);
	pkLD->SetIP(c_pszIP);

	m_map_kLogonAccount.insert(TLogonAccountMap::value_type(szLogin, pkLD));
	return true;
}

bool CClientManager::DeleteLogonAccount(const char * c_pszLogin, DWORD dwHandle)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	auto it = m_map_kLogonAccount.find(szLogin);

	if (it == m_map_kLogonAccount.end())
		return false;

	CLoginData * pkLD = it->second;

	if (pkLD->GetConnectedPeerHandle() != dwHandle)
	{
		sys_err("%s tried to logout in other peer handle %lu, current handle %lu", szLogin, dwHandle, pkLD->GetConnectedPeerHandle());
		return false;
	}

	if (pkLD->IsPlay())
	{
		pkLD->SetPlay(false);
#ifdef __DEPRECATED_BILLING__
		SendLoginToBilling(pkLD, false);
#endif
	}

	if (pkLD->IsDeleted())
	{
		delete pkLD;
	}

	m_map_kLogonAccount.erase(it);
	return true;
}

bool CClientManager::FindLogonAccount(const char * c_pszLogin)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	if (m_map_kLogonAccount.end() == m_map_kLogonAccount.find(szLogin))
		return false;

	return true;
}

void CClientManager::QUERY_LOGIN_BY_KEY(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<GDLoginByKeyPacket> p)
{
#ifdef ENABLE_LIMIT_TIME
	static int s_updateCount = 0;
	static int s_curTime = time(0);
	if (s_updateCount > 100)
	{
		s_curTime = time(0);
		s_updateCount = 0;
	}
	++s_updateCount;

	if (s_curTime >= GLOBAL_LIMIT_TIME)
	{
		sys_err("Server life time expired.");
		exit(0);
		return;
	}
#endif

	CLoginData * pkLoginData = GetLoginData(p->login_key());
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(p->login().c_str(), szLogin, sizeof(szLogin));

	if (!pkLoginData)
	{
		sys_log(0, "LOGIN_BY_KEY key not exist %s %lu", szLogin, p->login_key());
		pkPeer->Packet(TDGHeader::LOGIN_NOT_EXIST, dwHandle);
		return;
	}

	TAccountTable & r = pkLoginData->GetAccountRef();

	if (FindLogonAccount(r.login().c_str()))
	{
		sys_log(0, "LOGIN_BY_KEY already login %s %lu", r.login().c_str(), p->login_key());

		DGOutputPacket<DGLoginAlreadyPacket> ptog;
		ptog->set_login(p->login());
		pkPeer->Packet(ptog, dwHandle);
		return;
	}

	if (strcasecmp(r.login().c_str(), szLogin))
	{
		sys_log(0, "LOGIN_BY_KEY login differ %s %lu input %s", r.login().c_str(), p->login_key(), szLogin);
		pkPeer->Packet(TDGHeader::LOGIN_NOT_EXIST, dwHandle);
		return;
	}

	DWORD client_key[4];
	for (int i = 0; i < 4; ++i)
		client_key[i] = p->client_key(i);
	if (memcmp(pkLoginData->GetClientKey(), client_key, sizeof(DWORD) * 4))
	{
		const DWORD * pdwClientKey = pkLoginData->GetClientKey();

		sys_log(0, "LOGIN_BY_KEY client key differ %s %lu %lu %lu %lu, %lu %lu %lu %lu",
			r.login().c_str(),
			p->client_key(0), p->client_key(1), p->client_key(2), p->client_key(3),
			pdwClientKey[0], pdwClientKey[1], pdwClientKey[2], pdwClientKey[3]);

		pkPeer->Packet(network::TDGHeader::LOGIN_NOT_EXIST, dwHandle);
		return;
	}

	TAccountTable * pkTab = new TAccountTable;
	*pkTab = r;
	pkTab->set_status("OK");

	ClientHandleInfo * info = new ClientHandleInfo(dwHandle);
	info->pAccountTable = pkTab;
	strlcpy(info->ip, p->ip().c_str(), sizeof(info->ip));

	sys_log(0, "LOGIN_BY_KEY success %s %lu %s", r.login().c_str(), p->login_key(), info->ip);
	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT pid1, pid2, pid3, pid4, empire FROM player_index WHERE id=%u", r.id());
	CDBManager::instance().ReturnQuery(szQuery, QID_LOGIN_BY_KEY, pkPeer->GetHandle(), info);
}

void CClientManager::RESULT_LOGIN_BY_KEY(CPeer * peer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * info = (ClientHandleInfo *) qi->pvData;

	if (msg->uiSQLErrno != 0)
	{
		peer->Packet(network::TDGHeader::LOGIN_NOT_EXIST, info->dwHandle);
		delete info;
		return;
	}

	char szQuery[QUERY_MAX_LEN];

	if (msg->Get()->uiNumRows == 0)
	{
		DWORD account_id = info->pAccountTable->id();
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "SELECT pid1, pid2, pid3, pid4, empire FROM player_index WHERE id=%u", account_id);
		std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_PLAYER));
		
		sys_log(0, "RESULT_LOGIN_BY_KEY FAIL player_index's NULL : ID:%d", account_id);

		if (pMsg->Get()->uiNumRows == 0)
		{
			sys_log(0, "RESULT_LOGIN_BY_KEY FAIL player_index's NULL : ID:%d", account_id);

			// PLAYER_INDEX_CREATE_BUG_FIX
			snprintf(szQuery, sizeof(szQuery), "INSERT INTO player_index (id) VALUES(%u)", info->pAccountTable->id());
			CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_INDEX_CREATE, peer->GetHandle(), info);
			// END_PLAYER_INDEX_CREATE_BUF_FIX
		}
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

	int col = 0;

	for (; col < PLAYER_PER_ACCOUNT; ++col)
	{
		if (col >= info->pAccountTable->players_size())
			info->pAccountTable->add_players();

		info->pAccountTable->mutable_players(col)->set_id(std::stoi(row[col]));
	}

	info->pAccountTable->set_empire(std::stoi(row[col++]));
	info->account_index = 1;
#
	int iQueryLen = snprintf(szQuery, sizeof(szQuery), "SELECT id, name, job, level, playtime, st, ht, dx, iq, part_main, part_hair, x, y, skill_group, change_name");
#ifdef __HAIR_SELECTOR__
	iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen,
		", part_hair_base");
#endif
#ifdef __ACCE_COSTUME__
	iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen,
		", part_acce");
#endif
#ifdef __PRESTIGE__
	iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen,
		", prestige");
#endif
	iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen, ", IFNULL(UNIX_TIMESTAMP(last_play), 0) FROM player WHERE account_id=%u", info->pAccountTable->id());

	CDBManager::instance().ReturnQuery(szQuery, QID_LOGIN, peer->GetHandle(), info);
}

// PLAYER_INDEX_CREATE_BUG_FIX
void CClientManager::RESULT_PLAYER_INDEX_CREATE(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * info = (ClientHandleInfo *) qi->pvData;

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT pid1, pid2, pid3, pid4, empire FROM player_index WHERE id=%u", 
			info->pAccountTable->id());
	CDBManager::instance().ReturnQuery(szQuery, QID_LOGIN_BY_KEY, pkPeer->GetHandle(), info);
}
// END_PLAYER_INDEX_CREATE_BUG_FIX

TAccountTable * CreateAccountTableFromRes(MYSQL_RES * res)
{
	char input_pwd[PASSWD_MAX_LEN + 1];
	MYSQL_ROW row = NULL;
	DWORD col;

	row = mysql_fetch_row(res);
	col = 0;

	TAccountTable * pkTab = new TAccountTable;

	strlcpy(input_pwd, row[col++], sizeof(input_pwd));
	pkTab->set_id(std::stoi(row[col++]));
	pkTab->set_login(row[col++]);
	pkTab->set_passwd(row[col++]);
	pkTab->set_social_id(row[col++]);
	pkTab->set_empire(std::stoi(row[col++]));

	for (int j = 0; j < PLAYER_PER_ACCOUNT; ++j)
		pkTab->add_players()->set_id(std::stoi(row[col++]));

	pkTab->set_status(row[col++]);

	if (strcmp(pkTab->passwd().c_str(), input_pwd))
	{
		delete pkTab;
		return NULL;
	}

	return pkTab;
}

void CreateAccountPlayerDataFromRes(MYSQL_RES * pRes, TAccountTable * pkTab)
{
	if (!pRes)
		return;

	for (DWORD i = 0; i < mysql_num_rows(pRes); ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pRes);
		int col = 0;

		DWORD player_id = 0;
		!row[col++] ? 0 : str_to_number(player_id, row[col - 1]);

		if (!player_id)
			continue;

		int j;

		for (j = 0; j < PLAYER_PER_ACCOUNT; ++j)
		{
			if (pkTab->players(j).id() == player_id)
			{
				CClientManager::TPlayerCache * pc = CClientManager::instance().GetPlayerCache(player_id, false);
				TPlayerTable * pt = pc && pc->pPlayer ? pc->pPlayer->Get(false) : NULL;

				auto cur = pkTab->mutable_players(j);

				if (pt)
				{
					cur->set_name(pt->name());

					cur->set_job(pt->job());
					cur->set_level(pt->level());
					cur->set_play_minutes(pt->playtime());
					cur->set_st(pt->st());
					cur->set_ht(pt->ht());
					cur->set_dx(pt->dx());
					cur->set_iq(pt->iq());
					cur->set_main_part(pt->parts(PART_MAIN));
					cur->set_hair_part(pt->parts(PART_HAIR));
#ifdef __ACCE_COSTUME__
					cur->set_acce_part(pt->parts(PART_ACCE));
#endif

					cur->set_x(pt->x());
					cur->set_y(pt->y());
					cur->set_skill_group(pt->skill_group());
					cur->set_change_name(false);

					cur->set_hair_base_part(pt->part_hair_base());
					cur->set_last_playtime(pt->last_play_time());

#ifdef __PRESTIGE__
					cur->set_prestige(pt->prestige());
#endif
				}
				else
				{
					if (!row[col++])
						cur->clear_name();
					else
						cur->set_name(row[col - 1]);

					cur->set_job(std::stoi(row[col++]));
					cur->set_level(std::stoi(row[col++]));
					cur->set_play_minutes(std::stoi(row[col++]));
					cur->set_st(std::stoi(row[col++]));
					cur->set_ht(std::stoi(row[col++]));
					cur->set_dx(std::stoi(row[col++]));
					cur->set_iq(std::stoi(row[col++]));
					cur->set_main_part(std::stoi(row[col++]));
					cur->set_hair_part(std::stoi(row[col++]));

					cur->set_x(std::stoi(row[col++]));
					cur->set_y(std::stoi(row[col++]));
					cur->set_skill_group(std::stoi(row[col++]));
					cur->set_change_name(std::stoi(row[col++]));

#ifdef __HAIR_SELECTOR__
					cur->set_hair_base_part(std::stoi(row[col++]));
#endif
#ifdef __ACCE_COSTUME__
					cur->set_acce_part(std::stoi(row[col++]));
#endif
#ifdef __PRESTIGE__
					cur->set_prestige(std::stoi(row[col++]));
#endif
					cur->set_last_playtime(std::stoi(row[col++]));
				}

#ifdef __PRESTIGE__
				sys_log(0, "%s %lu %lu hair %u hair base %d prestige %d",
						pkTab->players(j).name().c_str(), pkTab->players(j).x(), pkTab->players(j).y(), pkTab->players(j).hair_part(), pkTab->players(j).hair_base_part(), pkTab->players(j).prestige());
#else
				sys_log(0, "%s %lu %lu hair %u hair base %d",
						pkTab->players(j).name().c_str(), pkTab->players(j).x(), pkTab->players(j).y(), pkTab->players(j).hair_part(), pkTab->players(j).hair_base_part());
#endif
				break;
			}
		}
		/*
		   if (j == PLAYER_PER_ACCOUNT)
		   sys_err("cannot find player_id on this account (login: %s id %lu account %lu %lu %lu)", 
		   pkTab->login().c_str(), player_id,
		   pkTab->players[0].dwID,
		   pkTab->players[1].dwID,
		   pkTab->players[2].dwID);
		   */
	}
}

void CClientManager::RESULT_LOGIN(CPeer * peer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * info = (ClientHandleInfo *) qi->pvData;

	if (info->account_index == 0)
	{
		// 계정이 없네?
		if (msg->Get()->uiNumRows == 0)
		{
			sys_log(0, "RESULT_LOGIN: no account");
			peer->Packet(TDGHeader::LOGIN_NOT_EXIST, info->dwHandle);
			delete info;
			return;
		}

		info->pAccountTable = CreateAccountTableFromRes(msg->Get()->pSQLResult);

		if (!info->pAccountTable)
		{
			sys_log(0, "RESULT_LOGIN: no account : WRONG_PASSWD");
			peer->Packet(TDGHeader::LOGIN_WRONG_PASSWD, info->dwHandle);
			delete info;
		}
		else
		{
			++info->account_index;

			char queryStr[512];
			int len;
			len = snprintf(queryStr, sizeof(queryStr),
				"SELECT id, name, job, level, playtime, st, ht, dx, iq, part_main, part_main_sub, part_hair, x, y, map_index, skill_group, change_name");
#ifdef __HAIR_SELECTOR__
			len += snprintf(queryStr + len, sizeof(queryStr) - len,
				", part_hair_base");
#endif
#ifdef __ACCE_COSTUME__
			len += snprintf(queryStr + len, sizeof(queryStr) - len,
				", part_acce");
#endif
#ifdef __PRESTIGE__
			len += snprintf(queryStr + len, sizeof(queryStr) - len,
				", prestige");
#endif
			len += snprintf(queryStr + len, sizeof(queryStr) - len,
				", UNIX_TIMESTAMP(last_play) FROM player WHERE account_id=%u",
				info->pAccountTable->id());

			CDBManager::instance().ReturnQuery(queryStr, QID_LOGIN, peer->GetHandle(), info);
		}
		return;
	}
	else
	{
		if (!info->pAccountTable) // 이럴리는 없겠지만;;
		{
			peer->Packet(TDGHeader::LOGIN_WRONG_PASSWD, info->dwHandle);
			delete info;
			return;
		}

		// 다른 컨넥션이 이미 로그인 해버렸다면.. 이미 접속했다고 보내야 한다.
		if (!InsertLogonAccount(info->pAccountTable->login().c_str(), peer->GetHandle(), info->ip))
		{
			sys_log(0, "RESULT_LOGIN: already logon %s", info->pAccountTable->login().c_str());

			DGOutputPacket<DGLoginAlreadyPacket> p;
			p->set_login(info->pAccountTable->login());
			peer->Packet(p, info->dwHandle);
		}
		else
		{
			sys_log(0, "RESULT_LOGIN: login success %s rows: %lu", info->pAccountTable->login().c_str(), msg->Get()->uiNumRows);

			if (msg->Get()->uiNumRows > 0)
				CreateAccountPlayerDataFromRes(msg->Get()->pSQLResult, info->pAccountTable);

			//PREVENT_COPY_ITEM
			CLoginData * p = GetLoginDataByLogin(info->pAccountTable->login().c_str());
			p->GetAccountRef() = *info->pAccountTable;

			//END_PREVENT_COPY_ITEM
			DGOutputPacket<DGLoginSuccessPacket> pack;
			*pack->mutable_account_info() = *info->pAccountTable;
			peer->Packet(pack, info->dwHandle);
		}

		delete info->pAccountTable;
		info->pAccountTable = NULL;
		delete info;
	}
}

void CClientManager::QUERY_LOGOUT(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDLogoutPacket> p)
{
	if (!*p->login().c_str())
		return;

	CLoginData * pLoginData = GetLoginDataByLogin(p->login().c_str());

	if (pLoginData == NULL)
		return;

	int pid[PLAYER_PER_ACCOUNT];

	for (int n = 0; n < PLAYER_PER_ACCOUNT; ++n)
	{
		if (pLoginData->GetAccountRef().players_size() <= n)
			break;

		if (pLoginData->GetAccountRef().players(n).id() == 0)
		{
			if (g_test_server)
				sys_log(0, "LOGOUT %s %d", p->login().c_str(), pLoginData->GetAccountRef().players(n).id());
			continue;
		}
		
		pid[n] = pLoginData->GetAccountRef().players(n).id();

		if (g_log)
			sys_log(0, "LOGOUT InsertLogoutPlayer %s %d", p->login().c_str(), pid[n]);

		InsertLogoutPlayer(pid[n]);
	}
	
	if (DeleteLogonAccount(p->login().c_str(), peer->GetHandle()))
	{
		if (g_log)
			sys_log(0, "LOGOUT %s ", p->login().c_str());
	}
}

void CClientManager::QUERY_CHANGE_NAME(CPeer * peer, DWORD dwHandle, std::unique_ptr<GDChangeNamePacket> p)
{
	char queryStr[QUERY_MAX_LEN];

	snprintf(queryStr, sizeof(queryStr),
		"SELECT COUNT(*) as count FROM player WHERE name='%s' AND id <> %u", p->name().c_str(), p->pid());

	std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(queryStr, SQL_PLAYER));

	if (pMsg->Get()->uiNumRows)
	{
		if (!pMsg->Get()->pSQLResult)
		{
			peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		if (*row[0] != '0')
		{
			peer->Packet(TDGHeader::PLAYER_CREATE_ALREADY, dwHandle);
			return;
		}
	}   
	else
	{
		peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
		return;
	}

	snprintf(queryStr, sizeof(queryStr),
			"UPDATE player SET name='%s',change_name=0 WHERE id=%u", p->name().c_str(), p->pid());

	std::auto_ptr<SQLMsg> pMsg0(CDBManager::instance().DirectQuery(queryStr, SQL_PLAYER));

	DGOutputPacket<DGChangeNamePacket> pack;
	pack->set_name(p->name());
	pack->set_pid(p->pid());
	peer->Packet(pack, dwHandle);
}

