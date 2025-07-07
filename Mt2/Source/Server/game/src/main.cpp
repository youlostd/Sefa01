#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "desc_client.h"
#include "event.h"
#include "minilzo.h"
#include "packet.h"
#include "desc_manager.h"
#include "item_manager.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "motion.h"
#include "sectree_manager.h"
#include "shop_manager.h"
#include "regen.h"
#include "text_file_loader.h"
#include "skill.h"
#include "pvp.h"
#include "party.h"
#include "questmanager.h"
#include "profiler.h"
#include "lzo_manager.h"
#include "messenger_manager.h"
#include "db.h"
#include "log.h"
#include "p2p.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "cmd.h"
#include "refine.h"
#include "priv_manager.h"
#include "war_map.h"
#include "building.h"
#include "target.h"
#include "marriage.h"
#include "wedding.h"
#include "fishing.h"
#include "item_addon.h"
#include "arena.h"
#include "OXEvent.h"
#include "polymorph.h"
#include "blend_item.h"
#include "ani.h"
#include "over9refine.h"
#include "MarkManager.h"
#include "DragonLair.h"
#include "skill_power.h"
#include "SpamFilter.h"

#include "general_manager.h"
#include <boost/bind.hpp>

#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif
#ifdef __ATTRTREE__
#include "attrtree_manager.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif

#ifdef USE_STACKTRACE
#include <execinfo.h>
#endif

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraDungeon.h"
#endif

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif

#ifdef DMG_RANKING
#include "dmg_ranking.h"
#endif

#ifdef __P2P_ONLINECOUNT__
#include <fstream>
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#endif

#include <chrono>

#ifndef __WIN32__
/*const char* kAsanDefaultOptions = "external_symbolizer_path=\"/usr/local/bin/llvm-symbolizer\":verbosity=1:help=true:symbolize=true:detect_stack_use_after_return=true:log_path=sanitizer:debug=true:alloc_dealloc_mismatch=true:new_delete_type_mismatch=true:detect_invalid_pointer_pairs=3:detect_container_overflow=false:allocator_may_return_null=false";
//"symbolize=1 log_path=asan.log external_symbolizer_path=\"/usr/local/bin/llvm-symbolizer\" halt_on_error=1";

extern "C"
__attribute__(
(no_sanitize_address)
)

const char *__asan_default_options() {
	return kAsanDefaultOptions;
}*/
#endif

volatile int	num_events_called = 0;
BYTE		g_bLogLevel = 0;

socket_t	tcp_socket = 0;
socket_t	udp_socket = 0;
socket_t	p2p_socket = 0;

LPFDWATCH	main_fdw = NULL;

int		io_loop(LPFDWATCH fdw);

int		start(int argc, char **argv);
int		idle();
void	destroy();

enum EProfile
{
	PROF_EVENT,
	PROF_CHR_UPDATE,
	PROF_IO,
	PROF_HEARTBEAT,
	PROF_LOG_ROTATE,
	PROF_IDLE,
	PROF_HB_SAVE_FLUSH,
	PROF_HB_DELAYED_SAVE,
	PROF_HB_HOMEPAGE_COMMAND,
	PROF_HB_ITEM_MANAGER_UPDATE,
	PROF_HB_OFFSHOP_HOMEPAGE_UPDATE,
	PROF_HB_QUERY_POOLING,
	PROF_HB_CH_USERCOUNT_UPDATE,
	PROF_HB_ONLINECOUNT_UPDATE,
	PROF_HB_PROCESS,
	PROF_MAX_NUM
};

static long long s_dwProfiler[PROF_MAX_NUM];

int g_shutdown_disconnect_pulse;
int g_shutdown_disconnect_force_pulse;
int g_shutdown_core_pulse;
bool g_bShutdown=false;

void ShutdownOnFatalError()
{
	if (!g_bShutdown)
	{
		sys_err("ShutdownOnFatalError!!!!!!!!!!");
		
		{
			char buf[256];

			strlcpy(buf, "¼­¹ö¿¡ Ä¡¸íÀûÀÎ ¿À·ù°¡ ¹ß»ýÇÏ¿© ÀÚµ¿À¸·Î ÀçºÎÆÃµË´Ï´Ù.", sizeof(buf));
			SendNotice(buf);
			strlcpy(buf, "10ÃÊÈÄ ÀÚµ¿À¸·Î Á¢¼ÓÀÌ Á¾·áµÇ¸ç,", sizeof(buf));
			SendNotice(buf);
			strlcpy(buf, "5ºÐ ÈÄ¿¡ Á¤»óÀûÀ¸·Î Á¢¼ÓÇÏ½Ç¼ö ÀÖ½À´Ï´Ù.", sizeof(buf));
			SendNotice(buf);
		}

		g_bShutdown = true;
		g_bNoMoreClient = true;

		g_shutdown_disconnect_pulse = thecore_pulse() + PASSES_PER_SEC(10);
		g_shutdown_disconnect_force_pulse = thecore_pulse() + PASSES_PER_SEC(20);
		g_shutdown_core_pulse = thecore_pulse() + PASSES_PER_SEC(30);
	}
}

extern std::vector<TPlayerTable> g_vec_save;
unsigned int save_idx = 0;
#ifdef __HOMEPAGE_COMMAND__
unsigned int g_iHomepageCommandSleepTime = 0;
std::queue<std::pair<std::string, bool> > g_queueSleepHomepageCommands;

void process_sleeping_homepage_commands()
{
	if (g_iHomepageCommandSleepTime)
	{
		if (g_iHomepageCommandSleepTime > get_dword_time())
			return;
	}

	g_iHomepageCommandSleepTime = 0;

	while (!g_queueSleepHomepageCommands.empty() && !g_iHomepageCommandSleepTime)
	{
		auto& rkCur = g_queueSleepHomepageCommands.front();

		if (rkCur.second)
		{
			if (test_server)
				sys_log(0, "HomepageCommand FORWARDING command [%s] (isForwarding %d)", rkCur.first.c_str(), rkCur.second);

			network::GGOutputPacket<network::GGHomepageCommandPacket> commandPacket;
			commandPacket->set_command(rkCur.first);
			P2P_MANAGER::instance().Send(commandPacket);
		}

		direct_interpret_command(rkCur.first.c_str());

		g_queueSleepHomepageCommands.pop();
	}
}

void process_homepage_commands()
{
	DBManager::instance().ReturnQuery(QID_PROCESS_HOMEPAGE_COMMANDS, 0, NULL, "SELECT command, date, forwarding FROM homepage_command WHERE executed = 0 ORDER BY id ASC");
}
#endif

void heartbeat(LPHEART ht, int pulse) 
{
#ifdef ENABLE_CORE_FPS_CHECK
	auto testTime = std::chrono::steady_clock::now();
#endif
	num_events_called += event_process(pulse);
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_EVENT] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif

	// 1ÃÊ¸¶´Ù
	if (!(pulse % ht->passes_per_sec))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif
		if (g_bAuthServer)
		{
			DESC_MANAGER::instance().ProcessExpiredLoginKey();
#ifdef __DEPRECATED_BILLING__
			DBManager::instance().FlushBilling();
#endif
		}

		{
			int count = 0;

			if (save_idx < g_vec_save.size())
			{
				count = MIN(100, g_vec_save.size() - save_idx);

				for (int i = 0; i < count; ++i, ++save_idx)
				{
					network::GDOutputPacket<network::GDPlayerSavePacket> pkg;
					pkg->set_allocated_data(&g_vec_save[save_idx]);
					db_clientdesc->DBPacket(pkg);
					pkg->release_data();
				}

				sys_log(0, "SAVE_FLUSH %d", count);
			}
		}
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_SAVE_FLUSH] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}

	//
	// 25 PPS(Pulse per second) ¶ó°í °¡Á¤ÇÒ ¶§
	//

	if (!(pulse % (passes_per_sec + 4)))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif
		CHARACTER_MANAGER::instance().ProcessDelayedSave();
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_DELAYED_SAVE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}


#ifdef __HOMEPAGE_COMMAND__
#ifdef ENABLE_CORE_FPS_CHECK
	testTime = std::chrono::steady_clock::now();
#endif
	process_sleeping_homepage_commands();
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_HB_HOMEPAGE_COMMAND] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
#endif

	if (!(pulse % (passes_per_sec * 6)))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif
		ITEM_MANAGER::instance().Update();
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_ITEM_MANAGER_UPDATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}

	if (!(pulse % (passes_per_sec * 5)))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif

#ifdef __HOMEPAGE_COMMAND__
		if (g_bProcessHomepageCommands && !g_iHomepageCommandSleepTime)
			process_homepage_commands();
#endif
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_OFFSHOP_HOMEPAGE_UPDATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}

#ifdef QUERY_POOLING
	if (!(pulse % (passes_per_sec * 3)))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif
		LogManager::instance().InsertItemQueryLogs();
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_QUERY_POOLING] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}
#endif
	
#ifdef __HOMEPAGE_COMMAND__	
	if (test_server && !(pulse % (passes_per_sec * 1 + 2)))
	{
		if (g_bProcessHomepageCommands && !g_iHomepageCommandSleepTime)
			process_homepage_commands();
	}
#endif

	if (!(pulse % (passes_per_sec * 60)))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif

#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_CH_USERCOUNT_UPDATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}

	if (!(pulse % (passes_per_sec * 60 * 14)))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif

#ifdef DMG_RANKING
		CDmgRankingManager::instance().saveDmgRankings();
#endif
		
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_CH_USERCOUNT_UPDATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}

#ifdef __P2P_ONLINECOUNT__
	if( g_bWriteStats && !( pulse % ( passes_per_sec * 1 ) ))
	{
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif
		static std::vector<network::TOnlinePlayerInfo> v_onlinePlayers{ };

		auto GetChannelOnline = [ & ](BYTE channel) -> DWORD
		{
			return std::count_if(v_onlinePlayers.begin(), v_onlinePlayers.end(), [ & ](const network::TOnlinePlayerInfo& player) {
				return channel == player.channel();
			});
		};

		// Get online player list
		DWORD totalOnline = P2P_MANAGER::instance().GetOnlinePlayerInfo(v_onlinePlayers);	
		
		DWORD ch1	= GetChannelOnline(1);
		DWORD ch2	= GetChannelOnline(2);
		DWORD ch3	= GetChannelOnline(3);
		DWORD ch4	= GetChannelOnline(4);
		DWORD ch5	= GetChannelOnline(5);
		DWORD main	= GetChannelOnline(99);

		std::ofstream statsFile("server_stats.txt", std::ios::out | std::ios::trunc);

		statsFile 
			<< totalOnline << "\t"
			<< main << "\t"
			<< ch1 << "\t"
			<< ch2 << "\t"
			<< ch3 << "\t"
			<< ch4 << "\t"
			<< ch5 << "\tEND";

		statsFile.close();

		// Clear vec to use it again
		v_onlinePlayers.clear();
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HB_ONLINECOUNT_UPDATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
	}
#endif

#ifdef ENABLE_CORE_FPS_CHECK
	testTime = std::chrono::steady_clock::now();
#endif
	DBManager::instance().Process();
	AccountDB::instance().Process();
	CPVPManager::instance().Process();
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_HB_PROCESS] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif
}

SECTREE_MANAGER*	sectree_manager = new SECTREE_MANAGER;
CHARACTER_MANAGER*	char_manager = new CHARACTER_MANAGER;
ITEM_MANAGER*	item_manager = new ITEM_MANAGER;
CShopManager*	shop_manager = new CShopManager;
CMobManager*		mob_manager = new CMobManager;
CMotionManager*	motion_manager = new CMotionManager;
CPartyManager*	party_manager = new CPartyManager;
CSkillManager*	skill_manager = new CSkillManager;
CPVPManager*		pvp_manager = new CPVPManager;
LZOManager*		lzo_manager = new LZOManager;
DBManager*		db_manager = new DBManager;
AccountDB* 		account_db = new AccountDB;

LogManager*		log_manager = new LogManager;
MessengerManager*	messenger_manager = new MessengerManager;
P2P_MANAGER*		p2p_manager = new P2P_MANAGER;
CGuildManager*	guild_manager = new CGuildManager;
CGuildMarkManager* mark_manager = new CGuildMarkManager;
CDungeonManager*	dungeon_manager = new CDungeonManager;
CRefineManager*	refine_manager = new CRefineManager;
CPrivManager*	priv_manager = new CPrivManager;
CWarMapManager*	war_map_manager = new CWarMapManager;
building::CManager*	building_manager = new building::CManager;
CTargetManager*	target_manager = new CTargetManager;
marriage::CManager*	marriage_manager = new marriage::CManager;
marriage::WeddingManager* wedding_manager = new marriage::WeddingManager;
CItemAddonManager*	item_addon_manager = new CItemAddonManager;
CArenaManager* arena_manager = new CArenaManager;
COXEventManager* OXEvent_manager = new COXEventManager;
#ifdef ENABLE_SPAM_FILTER
CSpamFilter* spam_manager = new CSpamFilter;
#endif
#ifdef COMBAT_ZONE
CCombatZoneManager* CombatZone_manager = new CCombatZoneManager;
#endif

DESC_MANAGER*	desc_manager = new DESC_MANAGER;

CTableBySkill* SkillPowerByLevel = new CTableBySkill;
CPolymorphUtils* polymorph_utils = new CPolymorphUtils;
CProfiler*		profiler = new CProfiler;
COver9RefineManager*	o9r = new COver9RefineManager;
CDragonLairManager*	dl_manager = new CDragonLairManager;

// CSpeedServerManager* SSManage = new CSpeedServerManager;

CLocaleManager*	localeManager = new CLocaleManager;

CGeneralManager*	generalManager = new CGeneralManager;

quest::CQuestManager* quest_manager = new quest::CQuestManager;

#ifdef __DRAGONSOUL__
DSManager* dragonsoul_manager = new DSManager;
#endif

#ifdef __MELEY_LAIR_DUNGEON__
MeleyLair::CMgr* meley_manager = new MeleyLair::CMgr;
#endif

#ifdef __ATTRTREE__
CAttrtreeManager* attrtree_manager = new CAttrtreeManager;
#endif

#ifdef __EVENT_MANAGER__
CEventManager* event_manager = new CEventManager;
#endif

#ifdef ENABLE_HYDRA_DUNGEON
CHydraDungeonManager* hydra_manager = new CHydraDungeonManager;
#endif

#ifdef ENABLE_RUNE_SYSTEM
CRuneManager*		rune_manager = new CRuneManager;
#endif

#ifdef DMG_RANKING
CDmgRankingManager* dmgranking_manager = new CDmgRankingManager;
#endif

#ifdef AUCTION_SYSTEM
AuctionManager* auction_manager = new AuctionManager;
AuctionShopManager* auction_shop_manager = new AuctionShopManager;
#endif

#define destroy_manager(manager_name) sys_log(0, "DESTROY MANAGER: %s [%p]\n", #manager_name, manager_name); if (manager_name) delete(manager_name); manager_name = NULL; sys_log(0, "Destroyed.\n")

void destroy_all_manager()
{
	sys_log(0, "Destroy all managers...\n\n");

#ifdef __MELEY_LAIR_DUNGEON__
	destroy_manager(meley_manager);
#endif

#ifdef ENABLE_HYDRA_DUNGEON
	destroy_manager(hydra_manager);
#endif

	destroy_manager(sectree_manager);
	destroy_manager(char_manager);
	destroy_manager(item_manager);
	destroy_manager(shop_manager);
	destroy_manager(mob_manager);
	destroy_manager(motion_manager);
	destroy_manager(party_manager);
	destroy_manager(skill_manager);
	destroy_manager(pvp_manager);
	destroy_manager(lzo_manager);
	destroy_manager(db_manager);
	destroy_manager(account_db);

	destroy_manager(log_manager);
	destroy_manager(messenger_manager);
	destroy_manager(p2p_manager);
	destroy_manager(guild_manager);
	destroy_manager(mark_manager);
	destroy_manager(dungeon_manager);
	destroy_manager(refine_manager);
	destroy_manager(priv_manager);
	destroy_manager(war_map_manager);
	destroy_manager(building_manager);
	destroy_manager(target_manager);
	destroy_manager(marriage_manager);
	destroy_manager(wedding_manager);
	destroy_manager(item_addon_manager);
	destroy_manager(arena_manager);
	destroy_manager(OXEvent_manager);
#ifdef ENABLE_SPAM_FILTER
	destroy_manager(spam_manager);
#endif

#ifdef COMBAT_ZONE
	destroy_manager(CombatZone_manager);
#endif

	destroy_manager(desc_manager);

	destroy_manager(SkillPowerByLevel);
	destroy_manager(polymorph_utils);
	destroy_manager(profiler);
	destroy_manager(o9r);
	destroy_manager(dl_manager);

	destroy_manager(localeManager);
	
	destroy_manager(generalManager);
	
	destroy_manager(quest_manager);
	
#ifdef __DRAGONSOUL__
	destroy_manager(dragonsoul_manager);
#endif

#ifdef __ATTRTREE__
	destroy_manager(attrtree_manager);
#endif

#ifdef __EVENT_MANAGER__
	destroy_manager(event_manager);
#endif

#ifdef ENABLE_RUNE_SYSTEM
	destroy_manager(rune_manager);
#endif

#ifdef DMG_RANKING
	destroy_manager(dmgranking_manager);
#endif

#ifdef AUCTION_SYSTEM
	destroy_manager(auction_manager);
	destroy_manager(auction_shop_manager);
#endif
}

int main(int argc, char **argv)
{
	ilInit(); // DevIL Initialize

	if (!start(argc, argv)) {
		destroy_all_manager();
		return 0;
	}


	if (!g_bAuthServer)
	{
		CGuildManager::instance().Initialize();

		if (!guild_mark_server)
		{
			if (!quest_manager->Initialize()) {
				destroy_all_manager();
				return 0;
			}

			MessengerManager::instance().Initialize();
			fishing::Initialize();
			OXEvent_manager->Initialize();
#ifdef COMBAT_ZONE
			CCombatZoneManager::instance().Initialize();
#endif

#ifdef DMG_RANKING
			CDmgRankingManager::instance().initDmgRankings();
#endif

			Cube_init();
			Blend_Item_init();
			ani_init();
		}
	}

	memset(s_dwProfiler, 0, sizeof(s_dwProfiler));

	while (idle());

	sys_log(0, "<shutdown> Starting...");
	g_bShutdown = true;
	g_bNoMoreClient = true;

	if (g_bAuthServer)
	{
#ifdef __DEPRECATED_BILLING__
		DBManager::instance().FlushBilling(true);
#endif

		int iLimit = DBManager::instance().CountQuery() / 50;
		int i = 0;

		do
		{
			DWORD dwCount = DBManager::instance().CountQuery();
			sys_log(0, "Queries %u", dwCount);

			if (dwCount == 0)
				break;

			usleep(500000);

			if (++i >= iLimit)
				if (dwCount == DBManager::instance().CountQuery())
					break;
		} while (1);
	}
	
#ifdef LOCALE_SAVE_LAST_USAGE
	CLocaleManager::instance().SaveLastUsage();
#endif
	
	sys_log(0, "<shutdown> Destroying CArenaManager...");
	arena_manager->Destroy();
#ifdef __EVENT_MANAGER__
	sys_log(0, "<shutdown> Shutting down CEventManager...");
	event_manager->Shutdown();
#endif
	sys_log(0, "<shutdown> Destroying COXEventManager...");
	OXEvent_manager->Destroy();

#ifdef COMBAT_ZONE
	sys_log(0, "<shutdown> Destroying CombatZone_manager...");
	CombatZone_manager->Destroy();
#endif

	sys_log(0, "<shutdown> Disabling signal timer...");
	signal_timer_disable();

	sys_log(0, "<shutdown> Shutting down CHARACTER_MANAGER...");
	char_manager->GracefulShutdown();
	sys_log(0, "<shutdown> Shutting down ITEM_MANAGER...");
	item_manager->GracefulShutdown();

	sys_log(0, "<shutdown> Flushing db_clientdesc...");
	db_clientdesc->FlushOutput();
	sys_log(0, "<shutdown> Flushing p2p_manager...");
	p2p_manager->FlushOutput();

	sys_log(0, "<shutdown> Destroying CShopManager...");
	shop_manager->Destroy();
	sys_log(0, "<shutdown> Destroying CHARACTER_MANAGER...");
	char_manager->Destroy();
	if (quest::CQuestManager::instance().GetCurrentCharacterPtr())
	{
		sys_err("<factor> quest char ptr is pointing to an invalid instance");
		quest::CQuestManager::instance().SetCurrentCharacterPtr(NULL);
	}
	sys_log(0, "<shutdown> Destroying ITEM_MANAGER...");
	item_manager->Destroy();
	sys_log(0, "<shutdown> Destroying DESC_MANAGER...");
	desc_manager->Destroy();
	sys_log(0, "<shutdown> Destroying quest::CQuestManager...");
	quest_manager->Destroy();
	sys_log(0, "<shutdown> Destroying building::CManager...");
	building_manager->Destroy();

#ifdef __PET_ADVANCED__
	CPetSkillProto::Destroy();
	CPetEvolveProto::Destroy();
	CPetAttrProto::Destroy();
#endif

#ifdef ENABLE_HYDRA_DUNGEON
	sys_log(0, "<shutdown> Destroying CHydraManager...");
	hydra_manager->Destroy();
#endif

#ifdef QUERY_POOLING
	LogManager::instance().InsertItemQueryLogs(true);
#endif

#ifdef DMG_RANKING
	CDmgRankingManager::instance().saveDmgRankings();
#endif

	destroy();
	destroy_all_manager();

	sys_log(0, "<shutdown> thecore_destroy()...");
	thecore_destroy();

	return 1;
}

int start(int argc, char **argv)
{
	bool bVerbose = false;

	config_init();

	char ch;
	while ((ch = getopt(argc, argv, "npverltI")) != -1)
	{
		char* ep = NULL;

		switch (ch)
		{
		case 'p': // port
			mother_port = (WORD)strtol(argv[optind], &ep, 10);
			p2p_port = mother_port + 1;
			printf("port %d\n", mother_port);

			optind++;
			optreset = 1;
			break;
		}
	}

#ifdef __WIN32__
	bVerbose = true;
#endif
	if (!bVerbose)
		freopen("stdout", "a", stdout);

	bool is_thecore_initialized = thecore_init(25, heartbeat);
	if (!is_thecore_initialized)
	{
		fprintf(stderr, "Could not initialize thecore, check owner of pid, syslog\n");
		exit(0);
	}

	signal_timer_disable();
	
	main_fdw = fdwatch_new(4096);

	if ((tcp_socket = socket_tcp_bind(g_szPublicIP, mother_port)) == INVALID_SOCKET)
	{
		perror("socket_tcp_bind: tcp_socket");
		return 0;
	}

	// if internal ip exists, p2p socket uses internal ip, if not use public ip
	//if ((p2p_socket = socket_tcp_bind(*g_szInternalIP ? g_szInternalIP : g_szPublicIP, p2p_port)) == INVALID_SOCKET)
	if ((p2p_socket = socket_tcp_bind(g_szPublicIP, p2p_port)) == INVALID_SOCKET)
	{
		perror("socket_tcp_bind: p2p_socket");
		return 0;
	}

	fdwatch_add_fd(main_fdw, tcp_socket, NULL, FDW_READ, false);
	fdwatch_add_fd(main_fdw, p2p_socket, NULL, FDW_READ, false);
	db_clientdesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, db_addr, db_port, PHASE_DBCLIENT, true);
	
	if (g_bAuthServer)
	{
		if (g_stAuthMasterIP.length() != 0)
		{
			fprintf(stderr, "SlaveAuth");
			g_pkAuthMasterDesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, g_stAuthMasterIP.c_str(), g_wAuthMasterPort, PHASE_P2P, true); 
			P2P_MANAGER::instance().RegisterConnector(g_pkAuthMasterDesc);
			g_pkAuthMasterDesc->SetP2P(g_stAuthMasterIP.c_str(), g_wAuthMasterPort, g_bChannel);

		}
		else
			fprintf(stderr, "MasterAuth");
	}

	signal_timer_enable(30);
	return 1;
}

void destroy()
{
	sys_log(0, "<shutdown> regen_free()...");
	regen_free();

	sys_log(0, "<shutdown> Closing sockets...");
	socket_close(tcp_socket);
	socket_close(p2p_socket);

	sys_log(0, "<shutdown> fdwatch_delete()...");
	fdwatch_delete(main_fdw);

	sys_log(0, "<shutdown> event_destroy()...");
	event_destroy();

	sys_log(0, "<shutdown> CTextFileLoader::DestroySystem()...");
	CTextFileLoader::DestroySystem();
}

int idle()
{
#ifdef ENABLE_CORE_FPS_CHECK
	static auto	pta = std::chrono::steady_clock::now();
	static auto testTime = std::chrono::steady_clock::now();
	static int			process_time_count = 0;
	static int heartFPS = 0;
#endif

	static int passed_pulses;

#ifdef ENABLE_CORE_FPS_CHECK
	testTime = std::chrono::steady_clock::now();
#endif
	if (!(passed_pulses = thecore_idle()))
		return 0;
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_IDLE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif

	assert(passed_pulses > 0);

	while (passed_pulses--) {
#ifdef ENABLE_CORE_FPS_CHECK
		testTime = std::chrono::steady_clock::now();
#endif
		heartbeat(thecore_heart, ++thecore_heart->pulse);
#ifdef ENABLE_CORE_FPS_CHECK
		s_dwProfiler[PROF_HEARTBEAT] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();

		// To reduce the possibility of abort() in checkpointing
		++heartFPS;
#endif
		thecore_tick();
	}

#ifdef ENABLE_CORE_FPS_CHECK
	testTime = std::chrono::steady_clock::now();
#endif
	CHARACTER_MANAGER::instance().Update(thecore_heart->pulse);
	db_clientdesc->Update(0);
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_CHR_UPDATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif

#ifdef ENABLE_CORE_FPS_CHECK
	testTime = std::chrono::steady_clock::now();
#endif
	if (!io_loop(main_fdw)) return 0;
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_IO] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();
#endif

#ifdef ENABLE_CORE_FPS_CHECK
	testTime = std::chrono::steady_clock::now();
#endif
	log_rotate();
#ifdef ENABLE_CORE_FPS_CHECK
	s_dwProfiler[PROF_LOG_ROTATE] += std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - testTime).count();


	++process_time_count;

	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - pta).count();
	if (duration >= 1000000 || heartFPS >= thecore_heart->passes_per_sec || process_time_count >= thecore_heart->passes_per_sec)
	{
		auto processingTime = duration - s_dwProfiler[PROF_IDLE];
		double utilizationRate = double(processingTime * 100) / double(duration);
		if (heartFPS != thecore_heart->passes_per_sec || process_time_count != thecore_heart->passes_per_sec || utilizationRate >= 90.0)
		{
			time_t ct = time(0);
			char *time_s = asctime(localtime(&ct));
			time_s[strlen(time_s) - 1] = '\0';
			struct timeval tv;
			gettimeofday(&tv, NULL);

			pt_log("%-15.15s.%d :: [HFPS: %d, FPS: %d] Monitored time: %lldus | Usefull processing time: %lldus | Idle time: %lldus | Utilization rate: %.3f%%\n"
					"\t[EVENT_STAT] Events processed: %d | Total events: %d | Computing time: %lldus (AVG: %lldus) | %.3f%% of total processing time\n"
					"\t[CHR_UPDATE_STAT] Computing time: %lldus (AVG: %lldus) | %.3f%% of total processing time\n"
					"\t[IO_STAT] Computing time: %lldus (AVG: %lldus) | %.3f%% of total processing time\n"
					"\t[LOG_ROTATE_STAT] Computing time: %lldus (AVG: %lldus) | %.3f%% of total processing time\n"
					"\t[HEARTBEAT_STAT] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total processing time\n"
					"\t\t[HB_SAVE_FLUSH] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_DELAYED_SAVE] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_HOMEPAGE_COMMAND] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_ITEM_MANAGER_UPDATE] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_OFFSHOP_HOMEPAGE_UPDATE] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_QUERY_POOLING] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_CH_USERCOUNT_UPDATE] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_ONLINECOUNT_UPDATE] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n"
					"\t\t[HB_PROCESS] Total computing time: %lldus (AVG: %lldus) | %.3f%% of total HB processing time\n",
				time_s + 4, tv.tv_usec, heartFPS, process_time_count, duration, processingTime, s_dwProfiler[PROF_IDLE], utilizationRate,
				num_events_called, event_count(), s_dwProfiler[PROF_EVENT], s_dwProfiler[PROF_EVENT] / heartFPS, double(s_dwProfiler[PROF_EVENT] * 100) / double(processingTime),
				s_dwProfiler[PROF_CHR_UPDATE], s_dwProfiler[PROF_CHR_UPDATE] / process_time_count, double(s_dwProfiler[PROF_CHR_UPDATE] * 100) / double(processingTime),
				s_dwProfiler[PROF_IO], s_dwProfiler[PROF_IO] / process_time_count, double(s_dwProfiler[PROF_IO] * 100) / double(processingTime),
				s_dwProfiler[PROF_LOG_ROTATE], s_dwProfiler[PROF_LOG_ROTATE] / process_time_count, double(s_dwProfiler[PROF_LOG_ROTATE] * 100) / double(processingTime),
				s_dwProfiler[PROF_HEARTBEAT], s_dwProfiler[PROF_HEARTBEAT] / heartFPS, double(s_dwProfiler[PROF_HEARTBEAT] * 100) / double(processingTime),
				s_dwProfiler[PROF_HB_SAVE_FLUSH], s_dwProfiler[PROF_HB_SAVE_FLUSH] / heartFPS, double(s_dwProfiler[PROF_HB_SAVE_FLUSH] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_DELAYED_SAVE], s_dwProfiler[PROF_HB_DELAYED_SAVE] / heartFPS, double(s_dwProfiler[PROF_HB_DELAYED_SAVE] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_HOMEPAGE_COMMAND], s_dwProfiler[PROF_HB_HOMEPAGE_COMMAND] / heartFPS, double(s_dwProfiler[PROF_HB_HOMEPAGE_COMMAND] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_ITEM_MANAGER_UPDATE], s_dwProfiler[PROF_HB_ITEM_MANAGER_UPDATE] / heartFPS, double(s_dwProfiler[PROF_HB_ITEM_MANAGER_UPDATE] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_OFFSHOP_HOMEPAGE_UPDATE], s_dwProfiler[PROF_HB_OFFSHOP_HOMEPAGE_UPDATE] / heartFPS, double(s_dwProfiler[PROF_HB_OFFSHOP_HOMEPAGE_UPDATE] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_QUERY_POOLING], s_dwProfiler[PROF_HB_QUERY_POOLING] / heartFPS, double(s_dwProfiler[PROF_HB_QUERY_POOLING] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_CH_USERCOUNT_UPDATE], s_dwProfiler[PROF_HB_CH_USERCOUNT_UPDATE] / heartFPS, double(s_dwProfiler[PROF_HB_CH_USERCOUNT_UPDATE] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_ONLINECOUNT_UPDATE], s_dwProfiler[PROF_HB_ONLINECOUNT_UPDATE] / heartFPS, double(s_dwProfiler[PROF_HB_ONLINECOUNT_UPDATE] * 100) / double(s_dwProfiler[PROF_HEARTBEAT]),
				s_dwProfiler[PROF_HB_PROCESS], s_dwProfiler[PROF_HB_PROCESS] / heartFPS, double(s_dwProfiler[PROF_HB_PROCESS] * 100) / double(s_dwProfiler[PROF_HEARTBEAT])
			);
		}
		/*pt_log("[%3d] event %5d/%-5d idle %-4ld event %-4ld heartbeat %-4ld I/O %-4ld chrUpate %-4ld | WRITE: %-7d | PULSE: %d",
				process_time_count,
				num_events_called,
				event_count(),
				thecore_profiler[PF_IDLE],
				s_dwProfiler[PROF_EVENT],
				s_dwProfiler[PROF_HEARTBEAT],
				s_dwProfiler[PROF_IO],
				s_dwProfiler[PROF_CHR_UPDATE],
				current_bytes_written,
				thecore_pulse());*/
		//pt_log("steady_clock num: %d den: %d", std::chrono::steady_clock::period::num, std::chrono::steady_clock::period::den);
		//pt_log("hr_clock num: %d den: %d steady: %d", std::chrono::high_resolution_clock::period::num, std::chrono::high_resolution_clock::period::den, std::chrono::high_resolution_clock::is_steady ? 1 : 0);
		num_events_called = 0;

		process_time_count = 0; 
		heartFPS = 0;

		memset(&s_dwProfiler[0], 0, sizeof(s_dwProfiler));
		pta = std::chrono::steady_clock::now();
	}
#endif

#ifdef __WIN32__
	if (_kbhit()) {
		int c = _getch();
		switch (c) {
			case 0x1b: // Esc
				return 0; // shutdown
				break;
			default:
				break;
		}
	}
#endif

	return 1;
}

int io_loop(LPFDWATCH fdw)
{
	LPDESC	d;
	int		num_events, event_idx;

	DESC_MANAGER::instance().DestroyClosed(); // PHASE_CLOSEÀÎ Á¢¼ÓµéÀ» ²÷¾îÁØ´Ù.
	DESC_MANAGER::instance().TryConnect();

	if ((num_events = fdwatch(fdw, 0)) < 0)
		return 0;

	for (event_idx = 0; event_idx < num_events; ++event_idx)
	{
		d = (LPDESC) fdwatch_get_client_data(fdw, event_idx);

		if (!d)
		{
			if (FDW_READ == fdwatch_check_event(fdw, tcp_socket, event_idx))
			{
				DESC_MANAGER::instance().AcceptDesc(fdw, tcp_socket);
				fdwatch_clear_event(fdw, tcp_socket, event_idx);
			}
			else if (FDW_READ == fdwatch_check_event(fdw, p2p_socket, event_idx))
			{
				DESC_MANAGER::instance().AcceptP2PDesc(fdw, p2p_socket);
				fdwatch_clear_event(fdw, p2p_socket, event_idx);
			}
			continue; 
		}

		int iRet = fdwatch_check_event(fdw, d->GetSocket(), event_idx);

		switch (iRet)
		{
			case FDW_READ:
				if (db_clientdesc == d)
				{
					int size = d->ProcessInput();

					if (size)
						sys_log(1, "DB_BYTES_READ: %d", size);

					if (size < 0)
					{
						d->SetPhase(PHASE_CLOSE);
					}
				}
				else if (d->ProcessInput() < 0)
				{
					d->SetPhase(PHASE_CLOSE);
				}
				break;

			case FDW_WRITE:
				if (db_clientdesc == d)
				{
					int buf_size = buffer_size(d->GetOutputBuffer());
					int sock_buf_size = fdwatch_get_buffer_size(fdw, d->GetSocket());

					int ret = d->ProcessOutput();

					if (ret < 0)
					{
						d->SetPhase(PHASE_CLOSE);
					}

					if (buf_size)
						sys_log(1, "DB_BYTES_WRITE: size %d sock_buf %d ret %d", buf_size, sock_buf_size, ret);
				}
				else if (d->ProcessOutput() < 0)
				{
					d->SetPhase(PHASE_CLOSE);
				}
				break;

			case FDW_EOF:
				{
					d->SetPhase(PHASE_CLOSE);
				}
				break;

			default:
				sys_err("fdwatch_check_event returned unknown %d", iRet);
				d->SetPhase(PHASE_CLOSE);
				break;
		}
	}

	return 1;
}

