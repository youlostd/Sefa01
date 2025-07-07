#include "stdafx.h"
#include <sstream>
#ifndef __WIN32__
#include <ifaddrs.h>
#endif

#include "constants.h"
#include "utils.h"
#include "log.h"
#include "desc.h"
#include "desc_manager.h"
#include "item_manager.h"
#include "p2p.h"
#include "char.h"
#include "war_map.h"
#include "config.h"
#include "dev_log.h"
#include "db.h"
#include "skill_power.h"
#include "version.h"

#include "protocol.h"

using std::string;

BYTE	g_bChannel = 0;
WORD	mother_port = 50080;
int		passes_per_sec = 25;
WORD	db_port = 0;
WORD	p2p_port = 50900;
char	db_addr[ADDRESS_MAX_LEN + 1];
int		save_event_second_cycle = passes_per_sec * 120;	// 3ºÐ
int		ping_event_second_cycle = passes_per_sec * 60;
bool	g_bNoMoreClient = false;
bool	g_bNoRegen = false;

bool		pvp_server = 0;
int			test_server = 0;
bool		guild_mark_server = false;
BYTE		guild_mark_min_level = 3;
bool		no_wander = false;

char		g_szPublicIP[16] = "0";
char		g_szInternalIP[16] = "0";
bool		g_bEmpireWhisper = true;
unsigned char		g_bAuthServer = false;

#ifdef ELONIA
string	g_stClientVersion = VERSION_ELONIA;
#else
string	g_stClientVersion = VERSION_AELDRA;
#endif

#ifdef __DEPRECATED_BILLING__
BYTE		g_bBilling = false;
#endif

BYTE		g_bPKProtectLevel = 15;

string	g_stAuthMasterIP;
WORD		g_wAuthMasterPort = 0;

string g_stHostname = "";

int SPEEDHACK_LIMIT_COUNT   = 50;
int SPEEDHACK_LIMIT_BONUS   = 80;
int g_iSyncHackLimitCount = 10;

//½Ã¾ß = VIEW_RANGE + VIEW_BONUS_RANGE
//VIEW_BONUSE_RANGE : Å¬¶óÀÌ¾ðÆ®¿Í ½Ã¾ß Ã³¸®¿¡¼­³Ê¹« µü ¶³¾îÁú°æ¿ì ¹®Á¦°¡ ¹ß»ýÇÒ¼öÀÖ¾î 500CMÀÇ ¿©ºÐÀ» Ç×»óÁØ´Ù.
int VIEW_RANGE = 5000;
int VIEW_BONUS_RANGE = 500;

int g_server_id = 0;
string g_strWebMallURL = "www.metin2.de";

bool		g_bCheckMultiHack = true;

bool			g_protectNormalPlayer   = false;		// ¹ü¹ýÀÚ°¡ "ÆòÈ­¸ðµå" ÀÎ ÀÏ¹ÝÀ¯Àú¸¦ °ø°ÝÇÏÁö ¸øÇÔ
bool			g_noticeBattleZone	  = false;		// Áß¸³Áö´ë¿¡ ÀÔÀåÇÏ¸é ¾È³»¸Þ¼¼Áö¸¦ ¾Ë·ÁÁÜ

#ifdef __PRESTIGE__
int gPrestigePlayerMaxLevel[PRESTIGE_MAX_LEVEL] = { 115, };
int gPlayerMaxLevel[PRESTIGE_MAX_LEVEL] = { 115, };
int gPrestigeMaxLevel = 1;
#else
int gPlayerMaxLevel = 115;
#endif
#ifdef __ANIMAL_SYSTEM__
int gAnimalMaxLevel = 1;
#endif

bool g_BlockCharCreation = false;

bool g_bCreatePublicFiles = false;

bool g_bEmpireChat = true;

#ifdef PROCESSOR_CORE
bool g_isProcessorCore = false;
#endif

// locale
bool g_bLoadLocaleStringFromFile = true;
bool g_bSaveLocaleStringToDatabase = false;

#ifdef __HOMEPAGE_COMMAND__
bool g_bProcessHomepageCommands = false;
#endif

#ifdef __P2P_ONLINECOUNT__
bool g_bWriteStats = false;
#endif

#ifdef DMG_RANKING
bool g_isDmgRanksProcess = false;
#endif

int g_iClientOutputBufferStartingSize = DEFAULT_PACKET_BUFFER_SIZE * 4 * 2;

bool is_string_true(const char * string)
{
	bool	result = 0;
	if (isnhdigit(*string))
	{
		str_to_number(result, string);
		return result > 0 ? true : false;
	}
	else if (LOWER(*string) == 't')
		return true;
	else
		return false;
}

static std::set<int> s_set_map_allows;

bool map_allow_find(int index)
{
	if (g_bAuthServer)
		return false;

	if (s_set_map_allows.find(index) == s_set_map_allows.end())
		return false;

	return true;
}

void map_allow_log()
{
	std::set<int>::iterator i;

	for (i = s_set_map_allows.begin(); i != s_set_map_allows.end(); ++i)
		sys_log(0, "MAP_ALLOW: %d", *i);
}

void map_allow_add(int index)
{
	if (map_allow_find(index) == true)
	{
		fprintf(stdout, "!!! FATAL ERROR !!! multiple MAP_ALLOW setting!!\n");
		exit(1);
	}
#ifdef PROCESSOR_CORE
	if (g_isProcessorCore)
	{
		fprintf(stdout, "!!! FATAL ERROR !!! don't allow map_allow on processor core!!\n");
		exit(1);
	}
#endif
	if (test_server)
		fprintf(stdout, "MAP ALLOW %d\n", index);
	s_set_map_allows.insert(index);
}

void map_allow_copy(google::protobuf::RepeatedField<google::protobuf::uint32>* target)
{
	std::set<int>::iterator it = s_set_map_allows.begin();

	while (it != s_set_map_allows.end())
	{
		int i = *(it++);
		target->Add(i);
	}
}

void map_allow_copy(long* pl, int size)
{
	int iCount = 0;
	std::set<int>::iterator it = s_set_map_allows.begin();

	while (it != s_set_map_allows.end())
	{
		int i = *(it++);
		*(pl++) = i;

		if (++iCount > size)
			break;
	}
}

bool GetIPInfo()
{
#ifndef __WIN32__
	struct ifaddrs* ifaddrp = NULL;

	if (0 != getifaddrs(&ifaddrp))
		return false;

	for( struct ifaddrs* ifap=ifaddrp ; NULL != ifap ; ifap = ifap->ifa_next )
	{
		struct sockaddr_in * sai = (struct sockaddr_in *) ifap->ifa_addr;

		if (!ifap->ifa_netmask ||  // ignore if no netmask
				sai->sin_addr.s_addr == 0 || // ignore if address is 0.0.0.0
				sai->sin_addr.s_addr == 16777343) // ignore if address is 127.0.0.1
			continue;
#else
	WSADATA wsa_data;
	char host_name[100];
	HOSTENT* host_ent;
	int n = 0;

	if (WSAStartup(0x0101, &wsa_data)) {
		return false;
	}

	gethostname(host_name, sizeof(host_name));
	host_ent = gethostbyname(host_name);
	if (host_ent == NULL) {
		return false;
	}
	for (; host_ent->h_addr_list[n] != NULL; ++n) {
		struct sockaddr_in addr;
		struct sockaddr_in* sai = &addr;
		thecore_memcpy(&sai->sin_addr.s_addr, host_ent->h_addr_list[n], host_ent->h_length);
#endif

		char* netip = inet_ntoa(sai->sin_addr);

		if (!strncmp(netip, "192.168", 7)) // ignore if address is starting with 192
		{
			strlcpy(g_szInternalIP, netip, sizeof(g_szInternalIP));
#ifndef __WIN32__
			fprintf(stderr, "INTERNAL_IP: %s interface %s\n", netip, ifap->ifa_name);
#else
			fprintf(stderr, "INTERNAL_IP: %s\n", netip);
#endif
		}
		else if (!strncmp(netip, "10.", 3))
		{
			strlcpy(g_szInternalIP, netip, sizeof(g_szInternalIP));
#ifndef __WIN32__
			fprintf(stderr, "INTERNAL_IP: %s interface %s\n", netip, ifap->ifa_name);
#else
			fprintf(stderr, "INTERNAL_IP: %s\n", netip);
#endif
		}
		else if (g_szPublicIP[0] == '0')
		{
			strlcpy(g_szPublicIP, netip, sizeof(g_szPublicIP));
#ifndef __WIN32__
			fprintf(stderr, "PUBLIC_IP: %s interface %s\n", netip, ifap->ifa_name);
#else
			fprintf(stderr, "PUBLIC_IP: %s\n", netip);
#endif
		}
	}

#ifndef __WIN32__
	freeifaddrs(ifaddrp);
#else
	WSACleanup();
#endif

	if (g_szPublicIP[0] != '0')
		return true;
	else
	{
#ifdef ENABLE_AUTODETECT_INTERNAL_IP
		if (g_szInternalIP[0] == '0')
			return false;
		else
		{
			strlcpy(g_szPublicIP, g_szInternalIP, sizeof(g_szPublicIP));
			fprintf(stderr, "INTERNAL_IP -> PUBLIC_IP: %s\n", g_szPublicIP);
			return true;
		}
#else
		return false;
#endif
	}
}

void __init_check_only_for_db(const char* token_string, char* value_string,
	char db_host[2][64], char db_user[2][64], char db_pwd[2][64], char db_db[2][64], int mysql_db_port[2],
	char log_host[64], char log_user[64], char log_pwd[64], char log_db[64], int* log_port,
	bool* pIsPlayerSQL, bool* pIsCommonSQL)
{
	TOKEN("hostname")
	{
		g_stHostname = value_string;
		fprintf(stdout, "HOSTNAME: %s\n", g_stHostname.c_str());
		return;
	}

	TOKEN("channel")
	{
		str_to_number(g_bChannel, value_string);
		// if (g_bChannel == 5)
			// g_bUserMinimumReached = false;
		return;
	}

	TOKEN("player_sql")
	{
		const char * line = two_arguments(value_string, db_host[0], sizeof(db_host[0]), db_user[0], sizeof(db_user[0]));
		line = two_arguments(line, db_pwd[0], sizeof(db_pwd[0]), db_db[0], sizeof(db_db[0]));

		if ('\0' != line[0])
		{
			char buf[256];
			one_argument(line, buf, sizeof(buf));
			str_to_number(mysql_db_port[0], buf);
		}

		if (!*db_host[0] || !*db_user[0] || !*db_pwd[0] || !*db_db[0])
		{
			fprintf(stderr, "PLAYER_SQL syntax: logsql <host user password db>\n");
			exit(1);
		}

		char buf[1024];
		snprintf(buf, sizeof(buf), "PLAYER_SQL: %s %s %s %s %d", db_host[0], db_user[0], db_pwd[0], db_db[0], mysql_db_port[0]);
		fprintf(stdout, buf);
		*pIsPlayerSQL = true;
		return;
	}

	TOKEN("common_sql")
	{
		const char * line = two_arguments(value_string, db_host[1], sizeof(db_host[1]), db_user[1], sizeof(db_user[1]));
		line = two_arguments(line, db_pwd[1], sizeof(db_pwd[1]), db_db[1], sizeof(db_db[1]));

		if ('\0' != line[0])
		{
			char buf[256];
			one_argument(line, buf, sizeof(buf));
			str_to_number(mysql_db_port[1], buf);
		}

		if (!*db_host[1] || !*db_user[1] || !*db_pwd[1] || !*db_db[1])
		{
			fprintf(stderr, "COMMON_SQL syntax: logsql <host user password db>\n");
			exit(1);
		}

		char buf[1024];
		snprintf(buf, sizeof(buf), "COMMON_SQL: %s %s %s %s %d\n", db_host[1], db_user[1], db_pwd[1], db_db[1], mysql_db_port[1]);
		fprintf(stdout, buf);
		*pIsCommonSQL = true;
		return;
	}

	TOKEN("log_sql")
	{
		const char * line = two_arguments(value_string, log_host, 64, log_user, 64);
		line = two_arguments(line, log_pwd, 64, log_db, 64);

		if ('\0' != line[0])
		{
			char buf[256];
			one_argument(line, buf, sizeof(buf));
			str_to_number(*log_port, buf);
		}

		if (!*log_host || !*log_user || !*log_pwd || !*log_db)
		{
			fprintf(stderr, "LOG_SQL syntax: logsql <host user password db>\n");
			exit(1);
		}

		char buf[1024];
		snprintf(buf, sizeof(buf), "LOG_SQL: %s %s %s %s %d\n", log_host, log_user, log_pwd, log_db, log_port);
		fprintf(stdout, buf);
		return;
	}
}

void __init_check_main(const char* token_string, char* value_string)
{
	TOKEN("empire_whisper")
	{
		bool b_value = 0;
		str_to_number(b_value, value_string);
		g_bEmpireWhisper = b_value;
		return;
	}

	TOKEN("mark_server")
	{
		guild_mark_server = is_string_true(value_string);
		return;
	}

	TOKEN("mark_min_level")
	{
		str_to_number(guild_mark_min_level, value_string);
		guild_mark_min_level = MINMAX(0, guild_mark_min_level, GUILD_MAX_LEVEL);
		return;
	}

	TOKEN("port")
	{
		str_to_number(mother_port, value_string);
		return;
	}

	TOKEN("max_log_level")
	{
		int tempVal;
		str_to_number(tempVal, value_string);
		for (int i = 1; i <= tempVal; ++i)
			log_set_level(i);
		return;
	}

	TOKEN("log_keep_days")
	{
		int i = 0;
		str_to_number(i, value_string);
		log_set_expiration_days(MINMAX(1, i, 90));
		return;
	}

	TOKEN("passes_per_sec")
	{
		str_to_number(passes_per_sec, value_string);
		return;
	}

	TOKEN("p2p_port")
	{
		str_to_number(p2p_port, value_string);
		return;
	}

	TOKEN("db_port")
	{
		str_to_number(db_port, value_string);
		return;
	}

	TOKEN("db_addr")
	{
		strlcpy(db_addr, value_string, sizeof(db_addr));

		for (int n = 0; n < ADDRESS_MAX_LEN; ++n)
		{
			if (db_addr[n] == ' ')
				db_addr[n] = '\0';
		}

		return;
	}

	TOKEN("save_event_second_cycle")
	{
		int	cycle = 0;
		str_to_number(cycle, value_string);
		save_event_second_cycle = cycle * passes_per_sec;
		return;
	}

	TOKEN("ping_event_second_cycle")
	{
		int	cycle = 0;
		str_to_number(cycle, value_string);
		ping_event_second_cycle = cycle * passes_per_sec;
		return;
	}

	TOKEN("test_server")
	{
		str_to_number(test_server, value_string);
		if(test_server)
		{
			printf("-----------------------------------------------\n");
			printf("TEST_SERVER\n");
			printf("-----------------------------------------------\n");
		}
		return;
	}

	TOKEN("pvp_server")
	{
		str_to_number(pvp_server, value_string);
		if(pvp_server)
		{
			printf("-----------------------------------------------\n");
			printf("PVP_SERVER\n");
			printf("-----------------------------------------------\n");
		}
		return;
	}

	TOKEN("shutdowned")
	{
		g_bNoMoreClient = true;
		return;
	}

	TOKEN("no_regen")
	{
		g_bNoRegen = true;
		return;
	}

	TOKEN("map_allow")
	{
		char * p = value_string;
		string stNum;

		for (; *p; p++)
		{
			if (isnhspace(*p))
			{
				if (stNum.length())
				{
					int	index = 0;
					str_to_number(index, stNum.c_str());
					map_allow_add(index);
					stNum.clear();
				}
			}
			else
				stNum += *p;
		}

		if (stNum.length())
		{
			int	index = 0;
			str_to_number(index, stNum.c_str());
			map_allow_add(index);
		}

		return;
	}

	TOKEN("no_wander")
	{
		no_wander = true;
		return;
	}

	TOKEN("auth_server")
	{
		char szIP[32];
		char szPort[32];

		two_arguments(value_string, szIP, sizeof(szIP), szPort, sizeof(szPort));

		if (!*szIP || (!*szPort && strcasecmp(szIP, "master")))
		{
			fprintf(stderr, "AUTH_SERVER: syntax error: <ip|master> <port>\n");
			exit(1);
		}

		g_bAuthServer = true;

		if (!strcasecmp(szIP, "master"))
			fprintf(stdout, "AUTH_SERVER: I am the master\n");
		else
		{
			g_stAuthMasterIP = szIP;
			str_to_number(g_wAuthMasterPort, szPort);

			fprintf(stdout, "AUTH_SERVER: master %s %u\n", g_stAuthMasterIP.c_str(), g_wAuthMasterPort);
		}
		return;
	}

#ifdef __DEPRECATED_BILLING__
	TOKEN("billing")
	{
		g_bBilling = true;
		return;
	}
#endif

	TOKEN("synchack_limit_count")
	{
		str_to_number(g_iSyncHackLimitCount, value_string);
		return;
	}

#ifdef PROCESSOR_CORE
	TOKEN("processor_core")
	{
		g_isProcessorCore = is_string_true(value_string);
		if (g_isProcessorCore)
		{
			if (s_set_map_allows.size() > 0)
			{
				fprintf(stdout, "!!! FATAL ERROR !!! don't allow processor core with maps!!\n");
				exit(1);
			}

			fprintf(stdout, "Booting processor core.\n");
		}
		return;
	}
#endif

	TOKEN("speedhack_limit_count")
	{
		str_to_number(SPEEDHACK_LIMIT_COUNT, value_string);
		return;
	}

	TOKEN("speedhack_limit_bonus")
	{
		str_to_number(SPEEDHACK_LIMIT_BONUS, value_string);
		return;
	}

	TOKEN("server_id")
	{
		str_to_number(g_server_id, value_string);
		return;
	}

	TOKEN("mall_url")
	{
		g_strWebMallURL = value_string;
		return;
	}

	TOKEN("bind_ip")
	{
		strlcpy(g_szPublicIP, value_string, sizeof(g_szPublicIP));
		return;
	}

	TOKEN("view_range")
	{
		str_to_number(VIEW_RANGE, value_string);
		return;
	}

	TOKEN("check_multihack")
	{
		str_to_number(g_bCheckMultiHack, value_string);
		return;
	}

	TOKEN("protect_normal_player")
	{
		str_to_number(g_protectNormalPlayer, value_string);
		return;
	}
	TOKEN("notice_battle_zone")
	{
		str_to_number(g_noticeBattleZone, value_string);
		return;
	}

	TOKEN("pk_protect_level")
	{
		str_to_number(g_bPKProtectLevel, value_string);
		fprintf(stderr, "PK_PROTECT_LEVEL: %d\n", g_bPKProtectLevel);
		return;
	}

	TOKEN("client_output_buffer_starting_size")
	{
		str_to_number(g_iClientOutputBufferStartingSize, value_string);
		fprintf(stderr, "CLIENT_OPBUFF_START_SIZE: %d\n", g_iClientOutputBufferStartingSize);
		return;
	}

	TOKEN("max_level")
	{
#ifdef __PRESTIGE__
		char * p = value_string;
		string stNum;

		int iPrestigeIndex = 0;

		for (; *p && iPrestigeIndex < PRESTIGE_MAX_LEVEL; p++)
		{
			if (isnhspace(*p))
			{
				if (stNum.length())
				{
					int	index = 0;
					str_to_number(index, stNum.c_str());
					gPlayerMaxLevel[iPrestigeIndex++] = MINMAX(1, index, PLAYER_MAX_LEVEL_CONST);
					stNum.clear();
				}
			}
			else
				stNum += *p;
		}

		if (stNum.length() && iPrestigeIndex < PRESTIGE_MAX_LEVEL)
		{
			int	index = 0;
			str_to_number(index, stNum.c_str());
			gPlayerMaxLevel[iPrestigeIndex++] = MINMAX(1, index, PLAYER_MAX_LEVEL_CONST);
		}

		for (int i = iPrestigeIndex; i < PRESTIGE_MAX_LEVEL; ++i)
			gPlayerMaxLevel[i] = gPlayerMaxLevel[MAX(0, iPrestigeIndex - 1)];

		for (int i = 0; i < iPrestigeIndex; ++i)
		{
			fprintf(stderr, "PLAYER_MAX_LEVEL: %d (prestigeLv %d)\n", gPlayerMaxLevel[i], i + 1);
		}

		return;
#else
		str_to_number(gPlayerMaxLevel, value_string);

		gPlayerMaxLevel = MINMAX(1, gPlayerMaxLevel, PLAYER_MAX_LEVEL_CONST);

		fprintf(stderr, "PLAYER_MAX_LEVEL: %d\n", gPlayerMaxLevel);
#endif
		return;
	}

#ifdef __PRESTIGE__
	TOKEN("prestige_max_level")
	{
		char * p = value_string;
		string stNum;

		int iPrestigeIndex = 1;

		for (; *p && iPrestigeIndex < PRESTIGE_MAX_LEVEL; p++)
		{
			if (isnhspace(*p))
			{
				if (stNum.length())
				{
					int	index = 0;
					str_to_number(index, stNum.c_str());
					gPrestigePlayerMaxLevel[iPrestigeIndex++] = index;
					stNum.clear();
				}
			}
			else
				stNum += *p;
		}

		if (stNum.length() && iPrestigeIndex < PRESTIGE_MAX_LEVEL)
		{
			int	index = 0;
			str_to_number(index, stNum.c_str());
			gPrestigePlayerMaxLevel[iPrestigeIndex++] = index;
		}

		for (int i = iPrestigeIndex; i < PRESTIGE_MAX_LEVEL; ++i)
			gPrestigePlayerMaxLevel[i] = gPrestigePlayerMaxLevel[MAX(0, iPrestigeIndex - 1)];

		return;
	}
#endif

#ifdef __ANIMAL_SYSTEM__
	TOKEN("animal_max_level")
	{
		str_to_number(gAnimalMaxLevel, value_string);
		fprintf(stderr, "ANIMAL_MAX_LEVEL: %d\n", gAnimalMaxLevel);
		return;
	}
#endif

	TOKEN("create_public_files")
	{
		int tmp = 0;
		str_to_number(tmp, value_string);

		if (0 == tmp || test_server)
			g_bCreatePublicFiles = false;
		else
			g_bCreatePublicFiles = true;
		return;
	}

	TOKEN("global_chat")
	{
		int tmp = 0;
		str_to_number(tmp, value_string);

		if (0 == tmp)
			g_bEmpireChat = true;
		else
			g_bEmpireChat = false;
		return;
	}

	TOKEN("LOCALE_LOAD_MODE")
	{
		if (!strcasecmp(value_string, "DATABASE"))
			g_bLoadLocaleStringFromFile = false;
		else
			g_bLoadLocaleStringFromFile = true;
		return;
	}

	TOKEN("LOCALE_SAVE")
	{
		g_bSaveLocaleStringToDatabase = is_string_true(value_string);
		return;
	}

#ifdef __HOMEPAGE_COMMAND__
	TOKEN("process_homepage_commands")
	{
		g_bProcessHomepageCommands = is_string_true(value_string);
		return;
	}
#endif
	
#ifdef __P2P_ONLINECOUNT__
	TOKEN("write_stats")
	{
		g_bWriteStats = is_string_true(value_string);
		return;
	}
#endif

#ifdef DMG_RANKING
	TOKEN("dmg_ranks_process")
	{
		g_isDmgRanksProcess = is_string_true(value_string);
		return;
	}
#endif
}

void config_init()
{
	FILE	*fp, *fp_main;

	char	buf[256];
	char	token_string[256];
	char	value_string[256];

	string	st_configFileName = "CONFIG";
	string	st_configGeneralFileName = "CONFIG_MAIN";

	if (!(fp = fopen(st_configFileName.c_str(), "r")))
	{
		fprintf(stderr, "Can not open [%s]\n", st_configFileName.c_str());
		exit(1);
	}

	if (!(fp_main = fopen(st_configGeneralFileName.c_str(), "r")))
	{
		fprintf(stderr, "Can not open [%s] - ignore\n", st_configGeneralFileName.c_str());
	}

	if (!GetIPInfo())
	{
		fprintf(stderr, "Can not get public ip address\n");
		exit(1);
	}

	char db_host[2][64], db_user[2][64], db_pwd[2][64], db_db[2][64];
	// ... ¾Æ... db_port´Â ÀÌ¹Ì ÀÖ´Âµ¥... ³×ÀÌ¹Ö ¾îÂîÇØ¾ßÇÔ...
	int mysql_db_port[2];

	for (int n = 0; n < 2; ++n)
	{
		*db_host[n]	= '\0';
		*db_user[n] = '\0';
		*db_pwd[n]= '\0';
		*db_db[n]= '\0';
		mysql_db_port[n] = 0;
	}

	char log_host[64], log_user[64], log_pwd[64], log_db[64];
	int log_port = 0;

	*log_host = '\0';
	*log_user = '\0';
	*log_pwd = '\0';
	*log_db = '\0';


	// DB¿¡¼­ ·ÎÄÉÀÏÁ¤º¸¸¦ ¼¼ÆÃÇÏ±âÀ§ÇØ¼­´Â ´Ù¸¥ ¼¼ÆÃ°ªº¸´Ù ¼±ÇàµÇ¾î¼­
	// DBÁ¤º¸¸¸ ÀÐ¾î¿Í ·ÎÄÉÀÏ ¼¼ÆÃÀ» ÇÑÈÄ ´Ù¸¥ ¼¼ÆÃÀ» Àû¿ë½ÃÄÑ¾ßÇÑ´Ù.
	// ÀÌÀ¯´Â ·ÎÄÉÀÏ°ü·ÃµÈ ÃÊ±âÈ­ ·çÆ¾ÀÌ °÷°÷¿¡ Á¸ÀçÇÏ±â ¶§¹®.

	bool isCommonSQL = false;	
	bool isPlayerSQL = false;

	FILE* fpOnlyForDB;
	if (!(fpOnlyForDB = fopen(st_configFileName.c_str(), "r")))
	{
		fprintf(stderr, "Can not open [%s]\n", st_configFileName.c_str());
		exit(1);
	}
	while (fgets(buf, 256, fpOnlyForDB))
	{
		parse_token(buf, token_string, value_string);

		__init_check_only_for_db(token_string, value_string,
			db_host, db_user, db_pwd, db_db, mysql_db_port,
			log_host, log_user, log_pwd, log_db, &log_port,
			&isPlayerSQL, &isCommonSQL);
	}
	fclose(fpOnlyForDB);
	if (!(fpOnlyForDB = fopen(st_configGeneralFileName.c_str(), "r")))
	{
		fprintf(stderr, "Can not open [%s] - ignore\n", st_configGeneralFileName.c_str());
	}
	else
	{
		while (fgets(buf, 256, fpOnlyForDB))
		{
			parse_token(buf, token_string, value_string);

			__init_check_only_for_db(token_string, value_string,
				db_host, db_user, db_pwd, db_db, mysql_db_port,
				log_host, log_user, log_pwd, log_db, &log_port,
				&isPlayerSQL, &isCommonSQL);
		}
		fclose(fpOnlyForDB);
	}

	// CONFIG_SQL_INFO_ERROR
	if (!isCommonSQL)
	{
		puts("LOAD_COMMON_SQL_INFO_FAILURE:");
		puts("");
		puts("CONFIG:");
		puts("------------------------------------------------");
		puts("COMMON_SQL: HOST USER PASSWORD DATABASE");
		puts("");
		exit(1);
	}

	if (!isPlayerSQL)
	{
		puts("LOAD_PLAYER_SQL_INFO_FAILURE:");
		puts("");
		puts("CONFIG:");
		puts("------------------------------------------------");
		puts("PLAYER_SQL: HOST USER PASSWORD DATABASE");
		puts("");
		exit(1);
	}

	// Common DB °¡ Locale Á¤º¸¸¦ °¡Áö°í ÀÖ±â ¶§¹®¿¡ °¡Àå ¸ÕÀú Á¢¼ÓÇØ¾ß ÇÑ´Ù.
	AccountDB::instance().Connect(db_host[1], mysql_db_port[1], db_user[1], db_pwd[1], db_db[1]);

	if (false == AccountDB::instance().IsConnected())
	{
		fprintf(stderr, "cannot start server while no common sql connected\n");
		exit(1);
	}

	fprintf(stdout, "CommonSQL connected\n");

	fprintf(stdout, "Setting DB to locale %s\n", Locale_GetLocale().c_str());
	AccountDB::instance().SetLocale(Locale_GetLocale());

	// Account DB
	AccountDB::instance().ConnectAsync(db_host[1], mysql_db_port[1], db_user[1], db_pwd[1], db_db[1], Locale_GetLocale().c_str());

	// Player DB
	DBManager::instance().Connect(db_host[0], mysql_db_port[0], db_user[0], db_pwd[0], db_db[0]);

	if (!DBManager::instance().IsConnected())
	{
		fprintf(stderr, "PlayerSQL.ConnectError\n");
		exit(1);
	}

	fprintf(stdout, "PlayerSQL connected\n");

	if (false == g_bAuthServer) // ÀÎÁõ ¼­¹ö°¡ ¾Æ´Ò °æ¿ì
	{
		// Log DB Á¢¼Ó
		LogManager::instance().Connect(log_host, log_port, log_user, log_pwd, log_db);

		if (!LogManager::instance().IsConnected())
		{
			fprintf(stderr, "LogSQL.ConnectError\n");
			exit(1);
		}

		fprintf(stdout, "LogSQL connected\n");

		LogManager::instance().BootLog(g_stHostname.c_str(), g_bChannel);
	}

	// SKILL_POWER_BY_LEVEL
	// ½ºÆ®¸µ ºñ±³ÀÇ ¹®Á¦·Î ÀÎÇØ¼­ AccountDB::instance().SetLocale(Locale_GetLocale()) ÈÄºÎÅÍ ÇÑ´Ù.
	// ¹°·Ð ±¹³»´Â º°·Î ¹®Á¦°¡ ¾ÈµÈ´Ù(ÇØ¿Ü°¡ ¹®Á¦)
	{
		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT mValue FROM locale WHERE mKey='SKILL_POWER_BY_LEVEL'");
		std::auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));

		if (pMsg->Get()->uiNumRows == 0)
		{
			fprintf(stderr, "[SKILL_PERCENT] Query failed: %s", szQuery);
			exit(1);
		}

		MYSQL_ROW row; 

		row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		const char * p = row[0];
		int cnt = 0;
		char num[128];
		int aiBaseSkillPowerByLevelTable[SKILL_MAX_LEVEL+1];

		fprintf(stdout, "SKILL_POWER_BY_LEVEL %s\n", p);
		while (*p != '\0' && cnt < (SKILL_MAX_LEVEL + 1))
		{
			p = one_argument(p, num, sizeof(num));
			aiBaseSkillPowerByLevelTable[cnt++] = atoi(num);

			//fprintf(stdout, "%d %d\n", cnt - 1, aiBaseSkillPowerByLevelTable[cnt - 1]);
			if (*p == '\0')
			{
				if (cnt != (SKILL_MAX_LEVEL + 1))
				{
					fprintf(stderr, "[SKILL_PERCENT] locale table has not enough skill information! (count: %d query: %s)", cnt, szQuery);
					exit(1);
				}

				fprintf(stdout, "SKILL_POWER_BY_LEVEL: Done! (count %d)\n", cnt);
				break;
			}
		}

		// Á¾Á·º° ½ºÅ³ ¼¼ÆÃ
		for (int job = 0; job < JOB_MAX_NUM * 2; ++job)
		{
			snprintf(szQuery, sizeof(szQuery), "SELECT mValue from locale where mKey='SKILL_POWER_BY_LEVEL_TYPE%d' ORDER BY CAST(mValue AS unsigned)", job);
			std::auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));

			// ¼¼ÆÃÀÌ ¾ÈµÇ¾îÀÖÀ¸¸é ±âº»Å×ÀÌºíÀ» »ç¿ëÇÑ´Ù.
			if (pMsg->Get()->uiNumRows == 0)
			{
				CTableBySkill::instance().SetSkillPowerByLevelFromType(job, aiBaseSkillPowerByLevelTable);
				continue;
			}

			row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			cnt = 0;
			p = row[0];
			int aiSkillTable[SKILL_MAX_LEVEL + 1];

			if (test_server)
				fprintf(stdout, "SKILL_POWER_BY_JOB %d %s\n", job, p);
			while (*p != '\0' && cnt < (SKILL_MAX_LEVEL + 1))
			{			
				p = one_argument(p, num, sizeof(num));
				aiSkillTable[cnt++] = atoi(num);

				//fprintf(stdout, "%d %d\n", cnt - 1, aiBaseSkillPowerByLevelTable[cnt - 1]);
				if (*p == '\0')
				{
					if (cnt != (SKILL_MAX_LEVEL + 1))
					{
						fprintf(stderr, "[SKILL_PERCENT] locale table has not enough skill information! (count: %d query: %s)", cnt, szQuery);
						exit(1);
					}
					if (test_server)
						fprintf(stdout, "SKILL_POWER_BY_JOB: Done! (job: %d count: %d)\n", job, cnt);
					break;
				}
			}

			CTableBySkill::instance().SetSkillPowerByLevelFromType(job, aiSkillTable);
		}		
	}
	// END_SKILL_POWER_BY_LEVEL

	// LOG_KEEP_DAYS_EXTEND
	log_set_expiration_days(7);
	// END_OF_LOG_KEEP_DAYS_EXTEND

	while (fgets(buf, 256, fp))
	{
		parse_token(buf, token_string, value_string);

		__init_check_main(token_string, value_string);
	}
	if (fp_main)
	{
		while (fgets(buf, 256, fp_main))
		{
			parse_token(buf, token_string, value_string);

			__init_check_main(token_string, value_string);
		}
	}

	if (0 == db_port)
	{
		fprintf(stderr, "DB_PORT not configured\n");
		exit(1);
	}

	if (0 == g_bChannel)
	{
		fprintf(stderr, "CHANNEL not configured\n");
		exit(1);
	}

	if (g_stHostname.empty())
	{
		fprintf(stderr, "HOSTNAME must be configured.\n");
		exit(1);
	}

	CLocaleManager::Instance().Initialize();

	fclose(fp);

#ifdef USE_CMD_CONFIG
	if ((fp = fopen("CMD", "r")))
	{
		while (fgets(buf, 256, fp))
		{
			char cmd[32], levelname[32];
			int level;

			two_arguments(buf, cmd, sizeof(cmd), levelname, sizeof(levelname));

			if (!*cmd || !*levelname)
			{
				fprintf(stderr, "CMD syntax error: <cmd> <DISABLE | LOW_WIZARD | WIZARD | HIGH_WIZARD | GOD>\n");
				exit(1);
			}

			if (!strcasecmp(levelname, "LOW_WIZARD"))
				level = GM_LOW_WIZARD;
			else if (!strcasecmp(levelname, "WIZARD"))
				level = GM_WIZARD;
			else if (!strcasecmp(levelname, "HIGH_WIZARD"))
				level = GM_HIGH_WIZARD;
			else if (!strcasecmp(levelname, "GOD"))
				level = GM_GOD;
			else if (!strcasecmp(levelname, "IMPLEMENTOR"))
				level = GM_IMPLEMENTOR;
			else if (!strcasecmp(levelname, "DISABLE"))
				level = GM_IMPLEMENTOR + 1;
			else
			{
				fprintf(stderr, "CMD syntax error: <cmd> <DISABLE | LOW_WIZARD | WIZARD | HIGH_WIZARD | GOD>\n");
				exit(1);
			}

			interpreter_set_privilege(cmd, level);
		}

		fclose(fp);
	}
#endif
	
	CWarMapManager::instance().LoadWarMapInfo(NULL);
}
