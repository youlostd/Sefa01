#ifndef __INC_METIN_II_GAME_CONFIG_H__
#define __INC_METIN_II_GAME_CONFIG_H__

#include <google/protobuf/repeated_field.h>

enum
{
	ADDRESS_MAX_LEN = 15
};

void config_init();

extern char sql_addr[256];

extern WORD mother_port;
extern WORD p2p_port;

extern char db_addr[ADDRESS_MAX_LEN + 1];
extern WORD db_port;

extern int passes_per_sec;
extern int save_event_second_cycle;
extern int ping_event_second_cycle;
extern int test_server;
extern bool	guild_mark_server;
extern BYTE guild_mark_min_level;

extern bool	g_bNoMoreClient;
extern bool	g_bNoRegen;

extern BYTE	g_bChannel;

extern bool	map_allow_find(int index);
extern void	map_allow_copy(long * pl, int size);
extern void map_allow_copy(google::protobuf::RepeatedField<google::protobuf::uint32>* target);
extern bool	no_wander;

extern time_t	g_global_time;

extern std::string	g_stHostname;

extern char		g_szPublicIP[16];
extern char		g_szInternalIP[16];

extern int is_twobyte(const char * str);

extern bool	g_bEmpireWhisper;

extern BYTE	g_bAuthServer;
#ifdef __DEPRECATED_BILLING__
extern BYTE	g_bBilling;
#endif

extern BYTE	g_bPKProtectLevel;

extern std::string	g_stAuthMasterIP;
extern WORD		g_wAuthMasterPort;

extern std::string	g_stClientVersion;

extern int	SPEEDHACK_LIMIT_COUNT;
extern int 	SPEEDHACK_LIMIT_BONUS;

extern int g_iSyncHackLimitCount;

extern int g_server_id;
extern std::string g_strWebMallURL;

extern int VIEW_RANGE;
extern int VIEW_BONUS_RANGE;

extern bool g_bCheckMultiHack;
extern bool g_protectNormalPlayer;	  // ¹ü¹ýÀÚ°¡ "ÆòÈ­¸ðµå" ÀÎ ÀÏ¹ÝÀ¯Àú¸¦ °ø°ÝÇÏÁö ¸øÇÔ
extern bool g_noticeBattleZone;		 // Áß¸³Áö´ë¿¡ ÀÔÀåÇÏ¸é ¾È³»¸Þ¼¼Áö¸¦ ¾Ë·ÁÁÜ

extern DWORD g_GoldDropTimeLimitValue;

#ifdef __PRESTIGE__
extern int gPlayerMaxLevel[PRESTIGE_MAX_LEVEL];
extern int gPrestigeMaxLevel;
#else
extern int gPlayerMaxLevel;
#endif
#ifdef __ANIMAL_SYSTEM__
extern int gAnimalMaxLevel;
#endif

extern bool g_bCreatePublicFiles;

extern bool g_bEmpireChat;

extern int g_iClientOutputBufferStartingSize;

// locale
extern bool g_bLoadLocaleStringFromFile;
extern bool g_bSaveLocaleStringToDatabase;

#ifdef __HOMEPAGE_COMMAND__
extern bool g_bProcessHomepageCommands;
#endif

#ifdef __P2P_ONLINECOUNT__
extern bool g_bWriteStats;
#endif

#ifdef PROCESSOR_CORE
extern bool g_isProcessorCore;
#endif

#ifdef DMG_RANKING
extern bool g_isDmgRanksProcess;
#endif

extern bool pvp_server;

#ifdef __PRESTIGE__
extern int gPrestigePlayerMaxLevel[PRESTIGE_MAX_LEVEL];
#endif

#endif /* __INC_METIN_II_GAME_CONFIG_H__ */

