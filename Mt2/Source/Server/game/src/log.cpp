#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "log.h"

#include "char.h"
#include "desc.h"
#include "item.h"

#include "desc_client.h"
#include "item_manager.h"
#include "questmanager.h"

#include "p2p.h"

static char	__escape_hint[1024*3];
static char	__escape_hint2[1024];
static char	__escape_hint3[1024];
static char	__escape_hint4[1024];

#ifdef QUERY_POOLING
std::vector<std::string> g_vec_logItemQuery;
DWORD g_dwInsertItemQueryTime = 0;
#endif

constexpr auto LOG_TABLE_DEFAULT = "log";
constexpr auto LOG_TABLE_TEMP = "log_temp_login";
#define LOG_TABLE (bTempLogin ? LOG_TABLE_TEMP : LOG_TABLE_DEFAULT)

LogManager::LogManager() : m_bIsConnect(false), m_sql(new CAsyncSQL)
{
}

LogManager::~LogManager()
{
	// FILE* pf = fopen("destroy_log_mgr.txt", "w");
	// fprintf(pf, "DELETE START\n");
	// fflush(pf);

	if (m_sql)
	{
		delete(m_sql);
		m_sql = NULL;
	}

	// fprintf(pf, "DELETE END\n");
	// fflush(pf);
	// fclose(pf);
}

bool LogManager::Connect(const char * host, const int port, const char * user, const char * pwd, const char * db)
{
	if (m_sql->Setup(host, user, pwd, db, Locale_GetLocale().c_str(), false, port))
		m_bIsConnect = true;

	return m_bIsConnect;
}

void LogManager::Query(const char * c_pszFormat, ...)
{
	char szQuery[QUERY_MAX_LEN];
	va_list args;
	va_start(args, c_pszFormat);
	vsnprintf(szQuery, sizeof(szQuery), c_pszFormat, args);
	va_end(args);

	if (test_server)
	{
		sys_log(0, "LOG: %s", szQuery);
		static FILE* pfQueryLog = fopen("logquerylog.txt", "w");
		time_t know = time(NULL);
		fprintf(pfQueryLog, "%s :: %s\n", asctime(localtime(&know)), szQuery);
		fflush(pfQueryLog);
	}

	m_sql->AsyncQuery(szQuery);
}

bool LogManager::IsConnected()
{
	return m_bIsConnect;
}

void LogManager::ItemLog(DWORD dwPID, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum, bool bTempLogin)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));

#ifdef QUERY_POOLING
	if (quest::CQuestManager::instance().GetEventFlag("query_pooling_disabled") == 0)
	{
		std::string queryValue = "('ITEM', NOW(), " + std::to_string(dwPID) + ", " + std::to_string(dwItemID) + ", '" + c_pszText + "', '" + __escape_hint + "', '" + c_pszIP + "', " + std::to_string(dwVnum) + ")";
		g_vec_logItemQuery.push_back(queryValue);
		InsertItemQueryLogs();
	}
	else
#endif
	Query("INSERT INTO %s (type, time, who, what, how, hint, ip, vnum) VALUES('ITEM', NOW(), %u, %u, '%s', '%s', '%s', %u)",
		LOG_TABLE, dwPID, dwItemID, c_pszText, __escape_hint, c_pszIP, dwVnum);
}

#ifdef QUERY_POOLING
void LogManager::InsertItemQueryLogs(bool shutdown)
{
	if (g_vec_logItemQuery.size() == 0)
		return;

	if (!shutdown)
	{
		if (g_dwInsertItemQueryTime + 5000 > get_dword_time() && g_vec_logItemQuery.size() < 10)
			return;

		g_dwInsertItemQueryTime = get_dword_time();
	}

	std::string query = "INSERT INTO log(type, time, who, what, how, hint, ip, vnum) VALUES ";

	for (const auto &it : g_vec_logItemQuery)
	{
		query += it;

		// reference check
		if (&it != &g_vec_logItemQuery.back())
			query += ", ";
	}

	g_vec_logItemQuery.clear();

	m_sql->AsyncQuery(query.c_str());
}
#endif

void LogManager::ItemLog(DWORD dwPID, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum)
{
	bool bTempLogin = false;
	if (auto p2p = P2P_MANAGER::instance().FindByPID(dwPID))
		bTempLogin = p2p->bTempLogin;

	ItemLog(dwPID, dwItemID, c_pszText, c_pszHint, c_pszIP, dwVnum, bTempLogin);
}

void LogManager::ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint)
{
	if (!ch || !item)
	{
		sys_err("character or item nil (ch %p item %p text %s)", get_pointer(ch), get_pointer(item), c_pszText);
		return;
	}

	ItemLog(ch->GetPlayerID(), item->GetID(),
			NULL == c_pszText ? "" : c_pszText,
		   	c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "",
		   	item->GetOriginalVnum(), ch->is_temp_login());
}

void LogManager::ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint)
{
	ItemLog(ch->GetPlayerID(), itemID, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", itemVnum, ch->is_temp_login());
}

void LogManager::CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dwValue, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, bool bTempLogin)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHint, strlen(c_pszHint));
#ifdef QUERY_POOLING
	if (quest::CQuestManager::instance().GetEventFlag("query_pooling_disabled") == 0)
	{
		std::string queryValue = "('CHARACTER', NOW(), " + std::to_string(dwPID) + ", " + std::to_string(dwValue) + ", '" + c_pszText + "', '" + __escape_hint + "', '" + c_pszIP + "', 0)";
		g_vec_logItemQuery.push_back(queryValue);
		InsertItemQueryLogs();
	}
	else
#endif
	Query("INSERT INTO %s (type, time, who, what, how, hint, ip) VALUES('CHARACTER', NOW(), %u, %u, '%s', '%s', '%s')",
		LOG_TABLE, dwPID, dwValue, c_pszText, __escape_hint, c_pszIP);
}

void LogManager::CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dw, const char* c_pszText, const char* c_pszHint, const char* c_pszIP)
{
	bool bTempLogin = false;
	if (auto p2p = P2P_MANAGER::instance().FindByPID(dwPID))
		bTempLogin = p2p->bTempLogin;

	CharLog(dwPID, x, y, dw, c_pszText, c_pszHint, c_pszIP, bTempLogin);
}

void LogManager::CharLog(LPCHARACTER ch, DWORD dw, const char * c_pszText, const char * c_pszHint)
{
	if (ch)
		CharLog(ch->GetPlayerID(), 0, 0, dw, c_pszText, c_pszHint, ch->GetDesc() ? ch->GetDesc()->GetHostName() : "", ch->is_temp_login());
	else
		CharLog(0, 0, 0, dw, c_pszText, c_pszHint, "", false);
}

void LogManager::MoneyLog(BYTE type, DWORD vnum, int gold)
{
	return;
	if (type == MONEY_LOG_RESERVED || type >= MONEY_LOG_TYPE_MAX_NUM)
	{
		sys_err("TYPE ERROR: type %d vnum %u gold %d", type, vnum, gold);
		return;
	}

	Query("INSERT INTO money_log VALUES (NOW(), %d, %d, %d)", type, vnum, gold);
}

void LogManager::HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHackName, strlen(c_pszHackName));

	Query("INSERT INTO hack_log (time, login, name, ip, server, why) VALUES(NOW(), '%s', '%s', '%s', '%s', '%s')", c_pszLogin, c_pszName, c_pszIP, g_stHostname.c_str(), __escape_hint);
}

void LogManager::HackLog(const char * c_pszHackName, LPCHARACTER ch)
{
	if (ch->GetDesc())
	{
		HackLog(c_pszHackName, 
				ch->GetDesc()->GetAccountTable().login().c_str(),
				ch->GetName(),
				ch->GetDesc()->GetHostName());
	}
}

void LogManager::CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success)
{
	Query("INSERT INTO cube (pid, time, x, y, item_vnum, item_uid, item_count, success) "
			"VALUES(%u, NOW(), %u, %u, %u, %u, %d, %d)",
			dwPID, x, y, item_vnum, item_uid, item_count, success?1:0);
}

void LogManager::BossSpawnLog(DWORD dwVnum)
{
	Query("INSERT INTO boss_spawn_log (date, hostname, vnum) "
		  "VALUES(NOW(), '%s', %d)",
		  g_stHostname.c_str(), dwVnum);
}

void LogManager::SpeedHackLog(DWORD pid, DWORD x, DWORD y, int hack_count)
{
	Query("INSERT INTO speed_hack (pid, time, x, y, hack_count) "
			"VALUES(%u, NOW(), %u, %u, %d)",
			pid, x, y, hack_count);
}

void LogManager::GMCommandLog(DWORD dwPID, const char* szName, const char* szIP, BYTE byChannel, const char* szCommand)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), szCommand, strlen(szCommand));

	Query("INSERT INTO command_log (userid, server, ip, port, username, command, date ) "
			"VALUES(%u, 1, '%s', %u, '%s', '%s', NOW()) ",
			dwPID, szIP, byChannel, szName, __escape_hint);
}

void LogManager::RefineLog(DWORD pid, const char* item_name, DWORD item_id, int item_refine_level, int is_success, const char* how)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), item_name, strlen(item_name));

	Query("INSERT INTO refinelog (pid, item_name, item_id, step, time, is_success, setType) VALUES(%u, '%s', %u, %d, NOW(), %d, '%s')",
			pid, __escape_hint, item_id, item_refine_level, is_success, how);
}

void LogManager::ShoutLog(BYTE bChannel, BYTE bEmpire, const char * pszText)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), pszText, strlen(pszText));

	Query("INSERT INTO shout_log VALUES(NOW(), %d, %d,'%s')", bChannel, bEmpire, __escape_hint);
}

void LogManager::LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour)
{
	Query("REPLACE INTO levellog (name, level, time, account_id, pid, playtime) VALUES('%s', %u, NOW(), %u, %u, %d)",
			pChar->GetName(), level, pChar->GetAID(), pChar->GetPlayerID(), playhour);
}

void LogManager::BootLog(const char * c_pszHostName, BYTE bChannel)
{
	Query("INSERT INTO bootlog (time, hostname, channel) VALUES(NOW(), '%s', %d)",
			c_pszHostName, bChannel);
}

void LogManager::FishLog(DWORD dwPID, int prob_idx, int fish_id, int fish_level, DWORD dwMiliseconds, DWORD dwVnum, DWORD dwValue)
{
	Query("INSERT INTO fish_log VALUES(NOW(), %u, %d, %u, %d, %u, %u, %u)",
			dwPID,
			prob_idx,
			fish_id,
			fish_level,
			dwMiliseconds,
			dwVnum,
			dwValue);
}

void LogManager::QuestRewardLog(const char * c_pszQuestName, DWORD dwPID, DWORD dwLevel, int iValue1, int iValue2)
{
	Query("INSERT INTO quest_reward_log VALUES('%s',%u,%u,2,%u,%u,NOW())", 
			c_pszQuestName,
			dwPID,
			dwLevel,
			iValue1, 
			iValue2);
}

void LogManager::DetailLoginLog(bool isLogin, LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	if (isLogin)
	{
		char szEscapedHWID[HWID_MAX_LEN * 2 + 1];
		m_sql->EscapeString(szEscapedHWID, sizeof(szEscapedHWID), ch->GetAccountTable().hwid().c_str(), HWID_MAX_LEN);

		Query("INSERT INTO loginlog (type, is_gm, login_time, channel, account_id, pid, ip, hwid) "
				"VALUES('INVALID', %s, NOW(), %d, %u, %u, '%s', '%s')",
				ch->IsGM() == true ? "'Y'" : "'N'",
				g_bChannel,
				ch->GetAccountTable().id(),
				ch->GetPlayerID(),
				ch->GetDesc()->GetHostName(),
				szEscapedHWID);
	}
	else
		Query("UPDATE loginlog SET type='VALID', logout_time=NOW(), playtime=TIMEDIFF(logout_time,login_time) WHERE account_id=%u AND pid=%u ORDER BY id DESC LIMIT 1",
				ch->GetAccountTable().id(),
				ch->GetPlayerID());
}

void LogManager::ConnectLog(bool isLogin, LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	if (isLogin)
	{
		char szName[CHARACTER_NAME_MAX_LEN * 2 + 1];
		m_sql->EscapeString(szName, sizeof(szName), ch->GetName(), strlen(ch->GetName()));

		Query("INSERT INTO connect_log (pid, name, login_time, channel, map_index, account_id) "
				"VALUES(%u, '%s', NOW(), %u, %ld, %u)",
				ch->GetPlayerID(),
				szName,
				g_bChannel,
				ch->GetMapIndex(),
				ch->GetAID());
	}
	else
		Query("UPDATE connect_log SET logout_time=NOW(), playtime=TIMEDIFF(logout_time,login_time) WHERE account_id=%u AND pid=%u ORDER BY id DESC LIMIT 1",
				ch->GetDesc()->GetAccountTable().id(),
				ch->GetPlayerID());
}

void LogManager::TranslationErrorLog(BYTE bType, const char* c_pszLangBase, const char* c_pszLanguage, const char* c_pszLangString, const char* c_pszError)
{
	char szLangBase[255 * 2 + 1];
	m_sql->EscapeString(szLangBase, sizeof(szLangBase), c_pszLangBase, strlen(c_pszLangBase));

	char szLanguage[255 * 2 + 1];
	m_sql->EscapeString(szLanguage, sizeof(szLanguage), c_pszLanguage, strlen(c_pszLanguage));

	char szLangString[255 * 2 + 1];
	m_sql->EscapeString(szLangString, sizeof(szLangString), c_pszLangString, strlen(c_pszLangString));

	char szError[255 * 2 + 1];
	m_sql->EscapeString(szError, sizeof(szError), c_pszError, strlen(c_pszError));

	Query("INSERT IGNORE INTO translation_error (`type`, lang_base, language, lang_string, error) VALUES (%d, '%s', '%s', '%s', '%s')",
		bType, szLangBase, szLanguage, szLangString, szError);
}

void LogManager::WhisperLog(DWORD dwSenderPID, const char* c_pszSenderName, DWORD dwReceiverPID, const char* c_pszReceiverName, const char* c_pszText, bool bIsOfflineMessage)
{
	char szSenderName[CHARACTER_NAME_MAX_LEN * 2 + 1];
	m_sql->EscapeString(szSenderName, sizeof(szSenderName), c_pszSenderName, strlen(c_pszSenderName));

	char szReceiverName[CHARACTER_NAME_MAX_LEN * 2 + 1];
	m_sql->EscapeString(szReceiverName, sizeof(szReceiverName), c_pszReceiverName, strlen(c_pszReceiverName));

	char szText[CHAT_MAX_LEN * 2 + 1];
	m_sql->EscapeString(szText, sizeof(szText), c_pszText, strlen(c_pszText));

	Query("INSERT INTO whisper_log (sender, sender_name, receiver, receiver_name, `text`, is_offline) VALUES (%u, '%s', %u, '%s', '%s', %u)",
		dwSenderPID, szSenderName, dwReceiverPID, szReceiverName, szText, bIsOfflineMessage);
}

#ifdef INCREASE_ITEM_STACK
void LogManager::ItemDestroyLog(BYTE bType, LPITEM pkItem, WORD bCount)
#else
void LogManager::ItemDestroyLog(BYTE bType, LPITEM pkItem, BYTE bCount)
#endif
{
	if (!pkItem)
	{
		sys_err("Item NULL");
		return;
	}
	
	static DWORD s_adwIgnoreVnumArray[] = { 92998, 71085, 70038, 50139, 27003, 8000, 1105, 7145, 5105, 2145, 15122, 71004, 145, 155, 3135, 27002, 27801, 27005, 50140, 1062, 70024, 71084, 50141, 27006, 2131, 90011, 122, 71152, 76007, 7141, 5101, 2130, 1111, 2072, 161, 241, 1042, 50314, 3111, 3141, 27110, 71151, 7140, 5100, 1110, 71094, 240, 1072, 160, 1063, 11252, 2132, 50306, 112, 3140, 102, 71029, 30301, 5102, 7142, 90010, 17162, 1112, 242, 162, 3142, 76014, 50826, 71001, 1061, 11432, 1060, 50821, 100, 2091, 2090, 5051, 123, 50315, 3091, 7090, 3090, 5050, 101, 2092, 7091, 50305, 92, 7092, 17182, 50823, 15142, 50825, 71034, 50304, 27102, 19, 50060, 11852, 1113, 2133, 17009, 7143, 72728, 5103, 2110, 5070, 17163, 14160, 163, 243, 2111, 1081, 121, 5071, 120, 3110, 13009, 3143, 1080, 11681, 11841, 11281, 50822, 11481, 7110, 72724, 50138, 50603, 50615, 50617, 50614, 50606, 50616, 27800, 50601, 50605, 50618, 11882, 11482, 11682, 11282, 11881, 25040, 50607, 50604, 50602, 50609, 50611, 50824, 7133, 1103, 5093, 2143, 50107, 1102, 7132, 7131, 5092, 2142, 2141, 5091, 1101, 153, 143, 152, 3133, 30341, 151, 50725, 141, 142, 3131, 3132, 90012, 50128, 50316, 7111, 50610, 17164, 17166, 16164, 50726, 17165, 14165, 16165, 16166, 14164, 14166, 50300, 3112, 50608, 50130, 76013, 70008, 11422, 1092, 3124, 70043, 2082, 14125, 17125, 11209, 70005, 5084, 15105, 16105, 15125, 30304, 5074, 5064, 14180, 14009, 15009, 27990, 16009, 7160, 1170, 92206, 11683, 11483, 3123, 17145, 30270, 16145, 11283, 11883, 70048, 50612, 30315, 50127, 72729, 72725, 132, 50901, 92209, 50722, 30700, 17084, 50724, 50511, 3114, 7120, 2120, 1090, 5080, 3104, 2122, 7121, 50506, 130, 3094, 50507, 5081, 3076, 17183, 50721, 50508, 1091, 131, 2121, 50480, 50509, 12642, 12502, 85, 12362, 50510, 1094, 50478, 50481, 296, 13202, 50434, 14161, 12365, 16125, 12222, 50477, 1052, 2124, 1064, 14105, 2114, 1074, 50476, 1084, 50432, 12384, 50479, 50613, 1093, 2104, 5110, 50450, 50435, 3122, 50418, 2094, 50491, 50431, 14082, 14145, 17124, 50421, 50728, 12346, 12225, 70051, 12244, 50417, 50449, 50448, 50493, 50420, 70102, 3120, 76016, 14181, 3121, 28230, 50451, 50447, 50433, 28233, 28235, 12645, 11219, 28236, 50419, 28237, 70050, 17202, 28242, 28240, 11609, 50416, 50446, 5082, 28231, 50495, 12505, 28234, 50492, 28232, 12206, 50723, 7082, 11855, 11454, 28243, 50405, 11455, 11854, 28238, 28239, 30303, 50496, 50436, 28241, 2156, 7122, 50494, 92208, 92211, 28130, 12209, 86, 133, 15104, 144, 11654, 28139, 14102, 12486, 11655, 12643, 12524, 12363, 28140, 28131, 28138, 12503, 28133, 28132, 28142, 50727, 50406, 13120, 12664, 11254, 134, 28143, 14122, 28134, 28137, 28136, 154, 50401, 30704, 28141, 50402, 13203, 12382, 28135, 104, 92210, 124, 50466, 50302, 92207, 114, 12223, 50461, 7056, 12626, 3075, 50462, 17144, 15085, 11255, 4026, 1046, 7076, 15124, 3082, 50464, 50465, 30318, 13194, 11445, 13205, 39019, 17105, 11442, 17203, 28340, 28043, 50303, 14026, 11645, 14027, 28032, 12662, 28040, 28041, 28035, 59, 11619, 28031, 70049, 12242, 17046, 28037, 12522, 30300, 28036, 17047, 7134, 17104, 11245, 28033, 13006, 28034, 28038, 7144, 1104, 15046, 16026, 14200, 5094, 50301, 28333, 5104, 15067, 28042, 5052, 15066, 2144, 16046, 28332, 16047, 16027, 28030, 15047, 30311, 11844, 11239, 28039, 2076, 1114, 14066, 3134, 28336, 5044, 14204, 28339, 76026, 50463, 2134, 28341, 28335, 28334, 15144, 17026, 3144, 14142, 50255, 92240, 76034, 17027, 92239, 92237, 92238, 17204, 28343, 17066, 14067, 50034, 3049, 28342, 17085, 11845, 7009, 13029, 17067, 15162, 15084, 28331, 5009, 28338, 28330, 71045, 1115, 14046, 2185, 11446, 1125, 5062, 14201, 7093, 11846, 255, 5095, 2150, 11246, 12489, 11662, 7135, 14182, 28337, 14047, 12663, 13100, 12523, 7155, 2135, 11809, 16204, 175, 11862, 12383, 11646, 71044, 3029, 92878, 12670, 11462, 1100, 5090, 14163, 7130, 50403, 11652, 12243, 3155, 5034, 11452, 53024, 3145, 12390, 92644, 16163, 11880, 16162, 140, 11493, 16066, 71114, 11262, 50404, 2140, 1019, 150, 11480, 92642, 30330, 11893, 30006, 92646, 50053, 11680, 3130, 11229, 12530, 13044, 11444, 39, 11693, 2093, 16182, 92645, 71030, 30329, 11639, 16067, 146, 156, 2146, 5106, 3136, 1106, 7146, 93004, 1123, 253, 2183, 173, 7153, 3153, 92254, 92247, 92243, 92251, 92255, 92249, 92253, 92242, 92252, 92246, 92250, 92245, 92241, 92248, 92244, 30711, 50619, 93401, 93400, 93402, 93399, 93397, 93398, 50102, 50105, 50100, 50101, 50104, 50103, 93396,50827, 94092, 12260, 93257, 50628, 50633, 50010, 50307, 93261, 93262, 93263, 95015, 
											50711, 30302, 92236, 92234, 92233, 92232, 27987, 93035, 92235, 50256, 92231, 92225, 92227, 92228, 92229, 30520, 92226, 92222, 92221, 92230, 50818, 30518, 17142, 92224, 92223, 50817, 27864, 50308, 7123, 16160, 33011 };
	auto itLast = s_adwIgnoreVnumArray + sizeof(s_adwIgnoreVnumArray) / sizeof(DWORD);
	if (std::find(s_adwIgnoreVnumArray, itLast, pkItem->GetVnum()) != itLast)
		return;

	network::GDOutputPacket<network::GDItemDestroyLogPacket> pack;
	ITEM_MANAGER::instance().GetPlayerItem(pkItem, pack->mutable_item());
	if (bCount)
		pack->mutable_item()->set_count(bCount);
	pack->set_type(bType);

	db_clientdesc->DBPacket(pack);
}

void LogManager::PacketErrorLog(BYTE bType, BYTE bHeader, const char* c_pszHostName, BYTE bSubHeader, LPCHARACTER ch, LPDESC d, const char* c_pszLastHeader, int iPhase, const char* c_pszLastChat)
{
	if (ch)
		m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), ch->GetName(), strlen(ch->GetName()));
	
	m_sql->EscapeString(__escape_hint2, sizeof(__escape_hint2), c_pszLastChat, strlen(c_pszLastChat));

	char szChannelData[40];
	snprintf(szChannelData, sizeof(szChannelData), "%s (%s)", g_stHostname.c_str(), c_pszHostName);
	Query("INSERT INTO packet_error (`type`, header, sub_header, player, map_index, channel, last_header, date, phase, comment) VALUES (%u, %u, %u, '%s', %u, '%s', '%s', NOW(), %i, '%s')",
		bType, bHeader, bSubHeader, ch ? __escape_hint : d->GetHostName(), ch ? ch->GetMapIndex() : 0, szChannelData, c_pszLastHeader, iPhase, __escape_hint2);
}

void LogManager::OkayEventLog(int dwPID, const char * c_pszText, int points)
{
    Query("INSERT INTO okay_event (pid, name, points) VALUES(%d, '%s', %d)",
            dwPID, c_pszText, points);
}

void LogManager::ForcedRewarpLog(LPCHARACTER ch, const char* c_pszDetailLog)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), c_pszDetailLog, strlen(c_pszDetailLog));

	Query("INSERT INTO forced_rewarp_log (pid, name, map_idx, x, y, detail_log) VALUES (%u, '%s', %ld, %ld, %ld, '%s')",
		ch->GetPlayerID(), ch->GetName(), ch->GetMapIndex(), ch->GetX(), ch->GetY(), __escape_hint);
}

#ifdef __PYTHON_REPORT_PACKET__
void LogManager::HackDetectionLog(LPCHARACTER ch, const char * c_pszType, const char * c_pszDetail)
{
	char szTypeEscaped[50 * 2 + 1];
	char szDetailEscaped[200 * 2 + 1];
	char szHWIDEscaped[HWID_MAX_LEN * 2 + 1];
	char szIP[40];

	m_sql->EscapeString(szTypeEscaped, sizeof(szTypeEscaped), c_pszType, strlen(c_pszType));
	m_sql->EscapeString(szDetailEscaped, sizeof(szDetailEscaped), c_pszDetail, strlen(c_pszDetail));
	m_sql->EscapeString(szHWIDEscaped, sizeof(szHWIDEscaped), ch->GetAccountTable().hwid().c_str(), strlen(ch->GetAccountTable().hwid().c_str()));
	m_sql->EscapeString(szIP, sizeof(szIP), ch->GetDesc()->GetHostName(), strlen(ch->GetDesc()->GetHostName()));

	Query("INSERT INTO hack_detection_log SET `type` = '%s', detail = '%s', account_id = %u, player_id = %u, hwid = '%s', ip = '%s'",
		szTypeEscaped, szDetailEscaped, ch->GetAID(), ch->GetPlayerID(), szHWIDEscaped, szIP);
}
#endif

void LogManager::LoginFailLog(const char * c_pszHWID, const char * c_pszError, const char * c_pszAccount, const char * c_pszPassword, const char * ip, const char * c_pszComputername)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), c_pszHWID, strlen(c_pszHWID));
	m_sql->EscapeString(__escape_hint2, sizeof(__escape_hint2), c_pszError, strlen(c_pszError));
	m_sql->EscapeString(__escape_hint3, sizeof(__escape_hint3), c_pszAccount, strlen(c_pszAccount));
	m_sql->EscapeString(__escape_hint4, sizeof(__escape_hint4), c_pszPassword, strlen(c_pszPassword));
	Query("INSERT INTO login_fail_log (hwid,error,login,password,ip, pcname) VALUES('%s', '%s', '%s', '%s', '%s', '%s')", __escape_hint,__escape_hint2,__escape_hint3,__escape_hint4, ip, c_pszComputername);
}

#ifdef __MELEY_LAIR_DUNGEON__
void LogManager::MeleyLog(DWORD dwGuildID, DWORD dwPartecipants, DWORD dwTime)
{
	// std::auto_ptr<SQLMsg> pMsg(DirectQuery("SELECT partecipants, time FROM log.meley_dungeon WHERE guild_id=%u;", dwGuildID));
	
	// if (pMsg->Get()->uiNumRows == 0)
	Query("INSERT INTO meley_dungeon (guild_id, partecipants, time, date) VALUES(%u, %u, %u, NOW())", dwGuildID, dwPartecipants, dwTime);
	// else
	// {
	// 	DWORD dwPartecipantsR = 0, dwTimeR = 0;
	// 	MYSQL_ROW mRow;
	// 	while (NULL != (mRow = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	// 	{
	// 		int iCur = 0;
	// 		str_to_number(dwPartecipantsR, mRow[iCur++]);
	// 		str_to_number(dwTimeR, mRow[iCur]);
	// 	}

	// 	if ((dwTimeR == dwTime) && (dwPartecipantsR < dwPartecipants))
	// 		Query("UPDATE meley_dungeon SET partecipants=%u, time=%u, date=NOW() WHERE guild_id=%u;", dwPartecipants, dwTime, dwGuildID);
	// 	else if (dwTimeR > dwTime)
	// 		Query("UPDATE meley_dungeon SET partecipants=%u, time=%u, date=NOW() WHERE guild_id=%u;", dwPartecipants, dwTime, dwGuildID);
	// }
}
#endif

void LogManager::ChangeNameLog(LPDESC d, DWORD dwPID, const char* c_pszOldName, const char* c_pszNewName)
{
	char szOldNameEscaped[CHARACTER_NAME_MAX_LEN]{};
	char szNewNameEscaped[CHARACTER_NAME_MAX_LEN]{};
	char szIP[40]{};

	m_sql->EscapeString(szOldNameEscaped, sizeof(szOldNameEscaped), c_pszOldName, strlen(c_pszOldName));
	m_sql->EscapeString(szNewNameEscaped, sizeof(szNewNameEscaped), c_pszNewName, strlen(c_pszNewName));
	m_sql->EscapeString(szIP, sizeof(szIP), d->GetHostName(), strlen(d->GetHostName())); // No need to escape that..

	Query("INSERT INTO change_name (pid, old_name, new_name, time, ip) VALUES(%d, '%s', '%s', NOW(), '%s')", dwPID, szOldNameEscaped, szNewNameEscaped, szIP);
}

void LogManager::SuspectTradeLog(LPCHARACTER seller, LPCHARACTER buyer, long long gold, int bars)
{
	char szEscapedHWID[HWID_MAX_LEN * 2 + 1];
	m_sql->EscapeString(szEscapedHWID, sizeof(szEscapedHWID), seller->GetAccountTable().hwid().c_str(), HWID_MAX_LEN);
	
	char szEscapedHWID2[HWID_MAX_LEN * 2 + 1];
	m_sql->EscapeString(szEscapedHWID2, sizeof(szEscapedHWID2), buyer->GetAccountTable().hwid().c_str(), HWID_MAX_LEN);
	
	if (!strcmp(seller->GetDesc()->GetHostName(), buyer->GetDesc()->GetHostName()))
	{
		if (!test_server)
			return;
		else
			seller->tchat("Wouldn't log on live.. same ip");
	}
		
	Query("INSERT INTO suspect_trade_log (seller, buyer, seller_name, buyer_name, gold, bars, seller_level, buyer_level, seller_hwid, buyer_hwid, seller_ip, buyer_ip) VALUES (%d, %d, '%s', '%s', %lld, %i, %i, %i, '%s', '%s', '%s', '%s')", 
		seller->GetPlayerID(), buyer->GetPlayerID(), seller->GetName(), buyer->GetName(), (long long) gold, bars,
		seller->GetLevel(), buyer->GetLevel(), szEscapedHWID, szEscapedHWID2, seller->GetDesc()->GetHostName(), buyer->GetDesc()->GetHostName());
}

void LogManager::EmergencyLog(const char* c_pszHint, ...)
{
	char szHint[1024];

	va_list args;
	va_start(args, c_pszHint);
	vsnprintf(szHint, sizeof(szHint), c_pszHint, args);
	va_end(args);

	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), szHint, strlen(szHint));
	Query("INSERT INTO emergency_log (date, core, hint) VALUES(NOW(), '%s', '%s')", g_stHostname.c_str(), __escape_hint);
}

#ifdef PACKET_ERROR_DUMP
void LogManager::ClientSyserrLog(LPCHARACTER ch, const char * c_pszText)
{
	m_sql->EscapeString(__escape_hint, sizeof(__escape_hint), c_pszText, strlen(c_pszText));
	Query("INSERT INTO client_syserr_log (pid, name, text) VALUES(%d, '%s', '%s')", ch->GetPlayerID(), ch->GetName(), __escape_hint);
}
#endif

#ifdef __DUNGEON_RANKING__
void LogManager::DungeonLog(LPCHARACTER ch, BYTE bIndex, DWORD dwTime)
{
	Query("INSERT INTO dungeon_log (pid, name, dungeon, time) VALUES(%d, '%s', %d, %d)", ch->GetPlayerID(), ch->GetName(), bIndex, dwTime);
}
#endif

#ifdef AUCTION_SYSTEM
void LogManager::AuctionLog(BYTE type, const std::string& info, DWORD player_id, DWORD owner_id, DWORD val, DWORD val2, const std::string& hint)
{
	char* escaped_hint = new char[hint.length() * 2 + 1];
	DBManager::instance().EscapeString(escaped_hint, hint.length() * 2 + 1, hint.c_str(), hint.length());

	Query("INSERT INTO auction_log (`type`, info, player_id, owner_id, item_id, item_vnum, hint, date) VALUES (%u, '%s', %u, %u, %u, %u, '%s', NOW())",
		type, info.c_str(), player_id, owner_id, val, val2, escaped_hint);
}
#endif

SQLMsg* LogManager::DirectQuery(const char * c_pszFormat, ...)
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
	return m_sql->DirectQuery(szQuery);
}