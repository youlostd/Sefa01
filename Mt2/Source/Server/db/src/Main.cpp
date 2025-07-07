#include "stdafx.h"
#include "Config.h"
#include "Peer.h"
#include "DBManager.h"
#include "ClientManager.h"
#include "GuildManager.h"
#include "ItemAwardManager.h"
#include "PrivManager.h"
#include "Marriage.h"
#include "ItemIDRangeManager.h"
#include "Main.h"
#include <signal.h>

#ifdef __GUILD_SAFEBOX__
#include "GuildSafeboxManager.h"
#endif

void SetPlayerDBName(const char* c_pszPlayerDBName);
int Start();

std::string g_stPlayerDBName = "";

BOOL g_test_server = false;

//단위 초
int g_iPlayerCacheFlushSeconds = 60*13;
int g_iItemCacheFlushSeconds = 60*12;
int g_iQuestCacheFlushSeconds = 60*11;

#ifdef CHANGE_SKILL_COLOR
int g_iSkillColorCacheFlushSeconds = 60 * 10;
#endif
#ifdef __EQUIPMENT_CHANGER__
int g_iEquipmentPageCacheFlushSeconds = 60 * 11;
#endif

//g_iLogoutSeconds 수치는 g_iPlayerCacheFlushSeconds 와 g_iItemCacheFlushSeconds 보다 길어야 한다.
int g_iLogoutSeconds = 60*6;

int g_log = 1;

BYTE g_bProtoLoadingMethod = PROTO_LOADING_TEXTFILE;
int g_protoSaveEnable = 0;

// MYSHOP_PRICE_LIST
int g_iItemPriceListTableCacheFlushSeconds = 540;
// END_OF_MYSHOP_PRICE_LIST

std::string astLocaleStringNames[LANGUAGE_MAX_NUM];

#ifndef __WIN32__
/*const char* kAsanDefaultOptions = "symbolize=1 log_path=stderr external_symbolizer_path=\"/usr/local/bin/llvm-symbolizer\" halt_on_error=1";

extern "C"
__attribute__((no_sanitize_address))
const char *__asan_default_options() {
	// CHECK: Available flags for AddressSanitizer:
	return kAsanDefaultOptions;
}*/
#endif

void emergency_sig(int sig)
{
	if (sig == SIGSEGV)
		sys_log(0, "SIGNAL: SIGSEGV");
	else if (sig == SIGUSR1)
		sys_log(0, "SIGNAL: SIGUSR1");

	if (sig == SIGSEGV)
		abort();
}

int main()
{
	CConfig					Config;
	CDBManager				DBManager; 
	CClientManager			ClientManager;
	CGuildManager			GuildManager;
	CPrivManager			PrivManager;
	ItemAwardManager		ItemAwardManager;
	marriage::CManager		MarriageManager;
	CItemIDRangeManager		ItemIDRangeManager;
#ifdef __GUILD_SAFEBOX__
	CGuildSafeboxManager	GuildSafeboxManager;
#endif

	char* testPtr = new char[100];
	testPtr = NULL;

	if (!Start())
		return 1;

	GuildManager.Initialize();
	MarriageManager.Initialize();
#ifdef __GUILD_SAFEBOX__
	GuildSafeboxManager.Initialize();
#endif
	ItemIDRangeManager.Build();
	sys_log(0, "Metin2DBCacheServer Start\n");

	CClientManager::instance().MainLoop();

	signal_timer_disable();

	DWORD iCount;

	while (1)
	{
		iCount = 0;

		for (size_t i = 0; i < SQL_MAX_NUM; ++i)
		{
			iCount += CDBManager::instance().CountReturnQuery(i);
			iCount += CDBManager::instance().CountAsyncQuery(i);
		}

		if (iCount == 0)
			break;
		
		usleep(1000);
		sys_log(0, "WAITING_QUERY_COUNT %d", iCount);
	}
	DBManager.Quit();

	ClientManager.Destroy();
	CNetBase::Destroy();

	return 1;
}

void emptybeat(LPHEART heart, int pulse)
{
	if (!(pulse % heart->passes_per_sec))	// 1초에 한번
	{
	}
}

//
// @version	05/06/13 Bang2ni - 아이템 가격정보 캐시 flush timeout 설정 추가.
//
int Start()
{
	if (!CConfig::instance().LoadFile("conf.txt"))
	{
		fprintf(stderr, "Loading conf.txt failed.\n");
		return false;
	}

	if (!CConfig::instance().GetValue("TEST_SERVER", &g_test_server))
		g_test_server = 0;

	if (g_test_server)
		fprintf(stderr, "Test Server\n");
	else
		fprintf(stderr, "Real Server\n");

	if (!CConfig::instance().GetValue("LOG", &g_log))
		g_log= 0;

	if (g_log)
		fprintf(stderr, "Log On\n");
	else
		fprintf(stderr, "Log Off\n");

	char szProtoLoadingMethod[30];
	if (CConfig::instance().GetValue("PROTO_LOADING_METHOD", szProtoLoadingMethod, sizeof(szProtoLoadingMethod)))
	{
		if (!strcmp(szProtoLoadingMethod, "LOAD_FROM_FILE"))
			g_bProtoLoadingMethod = PROTO_LOADING_TEXTFILE;
		else if (!strcmp(szProtoLoadingMethod, "LOAD_FROM_DATABASE"))
			g_bProtoLoadingMethod = PROTO_LOADING_DATABASE;
		else
			sys_err("Config: unkown PROTO_LOADING_METHOD selected [use \"LOAD_FROM_FILE\" or \"LOAD_FROM_DATABASE\"]");
	}

	if (CConfig::instance().GetValue("PROTO_SAVE_ENABLE", &g_protoSaveEnable))
		fprintf(stderr, "ProtoSaveEnable: %s\n", g_protoSaveEnable ? "On" : "Off");
	
	int tmpValue;

	int heart_beat = 50;
	if (!CConfig::instance().GetValue("CLIENT_HEART_FPS", &heart_beat))
	{
		fprintf(stderr, "Cannot find CLIENT_HEART_FPS configuration.\n");
		return false;
	}
	fprintf(stderr, "HeartFPS: %d\n", heart_beat);

	log_set_expiration_days(3);

	if (CConfig::instance().GetValue("LOG_KEEP_DAYS", &tmpValue))
	{
		tmpValue = MINMAX(3, tmpValue, 30);
		log_set_expiration_days(tmpValue);
		fprintf(stderr, "Setting log keeping days to %d\n", tmpValue);
	}

	thecore_init(heart_beat, emptybeat);
	signal_timer_enable(60);

	char szBuf[256+1];

	if (CConfig::instance().GetValue("PLAYER_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iPlayerCacheFlushSeconds, szBuf);
		sys_log(0, "PLAYER_CACHE_FLUSH_SECONDS: %d", g_iPlayerCacheFlushSeconds);
	}

	if (CConfig::instance().GetValue("ITEM_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iItemCacheFlushSeconds, szBuf);
		sys_log(0, "ITEM_CACHE_FLUSH_SECONDS: %d", g_iItemCacheFlushSeconds);
	}

	if (CConfig::instance().GetValue("QUEST_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iQuestCacheFlushSeconds, szBuf);
		sys_log(0, "QUEST_CACHE_FLUSH_SECONDS: %d", g_iQuestCacheFlushSeconds);
	}

	//EQ-CHANGER
#ifdef __EQUIPMENT_CHANGER__
	if (CConfig::instance().GetValue("EQUIPMENT_PAGE_CACHE_FLUSH_SECONDS", szBuf, 256))
	{
		str_to_number(g_iEquipmentPageCacheFlushSeconds, szBuf);
		sys_log(0, "EQUIPMENT_PAGE_CACHE_FLUSH_SECONDS: %d", g_iEquipmentPageCacheFlushSeconds);
	}
#endif

	// MYSHOP_PRICE_LIST
	if (CConfig::instance().GetValue("ITEM_PRICELIST_CACHE_FLUSH_SECONDS", szBuf, 256)) 
	{
		str_to_number(g_iItemPriceListTableCacheFlushSeconds, szBuf);
		sys_log(0, "ITEM_PRICELIST_CACHE_FLUSH_SECONDS: %d", g_iItemPriceListTableCacheFlushSeconds);
	}
	// END_OF_MYSHOP_PRICE_LIST
	//
	if (CConfig::instance().GetValue("CACHE_FLUSH_LIMIT_PER_SECOND", szBuf, 256))
	{
		DWORD dwVal = 0; str_to_number(dwVal, szBuf);
		CClientManager::instance().SetCacheFlushCountLimit(dwVal);
	}

	int iIDStart;
	if (!CConfig::instance().GetValue("PLAYER_ID_START", &iIDStart))
	{
		sys_err("PLAYER_ID_START not configured");
		return false;
	}

	CClientManager::instance().SetPlayerIDStart(iIDStart);

	char szAddr[64], szDB[64], szUser[64], szPassword[64];
	int iPort;
	char line[256+1];

	if (CConfig::instance().GetValue("SQL_PLAYER", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (player)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_PLAYER, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);

		iRetry = 5;
		do
		{
			if (CDBManager::instance().Connect(SQL_PLAYER_STUFF, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);


		do
		{
			if (CDBManager::instance().Connect(SQL_PLAYER_STUFF_LOAD, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);

		fprintf(stderr, "Success PLAYER\n");
		SetPlayerDBName(szDB);
	}
	else
	{
		sys_err("SQL_PLAYER not configured");
		return false;
	}

	if (CConfig::instance().GetValue("SQL_ACCOUNT", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (account)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_ACCOUNT, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		fprintf(stderr, "Success ACCOUNT\n");
	}
	else
	{
		sys_err("SQL_ACCOUNT not configured");
		return false;
	}

	if (CConfig::instance().GetValue("SQL_COMMON", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (common)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_COMMON, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		} while (iRetry--);
		fprintf(stderr, "Success COMMON\n");
	}
	else
	{
		sys_err("SQL_COMMON not configured");
		return false;
	}

	/*if (CConfig::instance().GetValue("SQL_HOTBACKUP", line, 256))
	{
		sscanf(line, " %s %s %s %s %d ", szAddr, szDB, szUser, szPassword, &iPort);
		sys_log(0, "connecting to MySQL server (hotbackup)");

		int iRetry = 5;

		do
		{
			if (CDBManager::instance().Connect(SQL_HOTBACKUP, szAddr, iPort, szDB, szUser, szPassword))
			{
				sys_log(0, "   OK");
				break;
			}

			sys_log(0, "   failed, retrying in 5 seconds");
			fprintf(stderr, "   failed, retrying in 5 seconds");
			sleep(5);
		}
		while (iRetry--);

		fprintf(stderr, "Success HOTBACKUP\n");
	}
	else
	{
		sys_err("SQL_HOTBACKUP not configured");
		return false;
	}*/
	
	if (!CNetBase::Create())
	{
		sys_err("Cannot create network poller");
		return false;
	}

	sys_log(0, "ClientManager initialization.. ");

	if (!CClientManager::instance().Initialize())
	{
		sys_log(0, "   failed"); 
		return false;
	}

	sys_log(0, "   OK");
	fprintf(stderr, "Start OK\n");

#ifndef __WIN32__
	signal(SIGUSR1, emergency_sig);
#endif
	signal(SIGSEGV, emergency_sig);
	return true;
}

void SetPlayerDBName(const char* c_pszPlayerDBName)
{
	if (! c_pszPlayerDBName || ! *c_pszPlayerDBName)
		g_stPlayerDBName = "";
	else
	{
		g_stPlayerDBName = c_pszPlayerDBName;
		g_stPlayerDBName += ".";
	}
}

const char * GetPlayerDBName()
{
	return g_stPlayerDBName.c_str();
}

