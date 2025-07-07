#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "log.h"
#include "desc.h"

// player commands

#ifdef HALLOWEEN_MINIGAME
ACMD(do_halloween_minigame);
#endif
#ifdef __EQUIPMENT_CHANGER__
ACMD(do_equipment_changer_load);
#endif
ACMD(do_user_warp);
#ifdef BLACKJACK
ACMD(do_blackjack);
#endif
#ifdef ACCOUNT_TRADE_BLOCK
ACMD(do_verify_hwid);
#endif
ACMD(do_guild_members_lastplayed);
ACMD(do_character_bug_report);
#ifdef USER_ATK_SPEED_LIMIT
ACMD(do_attackspeed_limit);
#endif
#ifdef PACKET_ERROR_DUMP
ACMD(do_syserr_log);
#endif
ACMD(do_dungeon_debug_info);
ACMD(do_war);
ACMD(do_nowar);
ACMD(do_stat);
ACMD(do_stat_minus);
ACMD(do_mount);
ACMD(do_restart);
ACMD(do_inputall);
ACMD(do_cmd);
ACMD(do_skillup);
ACMD(do_guildskillup);
ACMD(do_pvp);
// SAFEBOX/MALL
ACMD(do_safebox_close);
ACMD(do_safebox_password);
ACMD(do_safebox_change_password);
ACMD(do_mall_password);
ACMD(do_mall_open);
ACMD(do_mall_close);
ACMD(do_in_game_mall);
ACMD(do_click_mall);
// END OF SAFEBOX/MALL
ACMD(do_ungroup);
ACMD(do_close_shop);
ACMD(do_set_walk_mode);
ACMD(do_set_run_mode);
ACMD(do_pkmode);
ACMD(do_messenger_auth);
ACMD(do_setblockmode);
ACMD(do_unmount);
ACMD(do_party_request);
ACMD(do_party_request_accept);
ACMD(do_party_request_deny);
ACMD(do_observer_exit);
ACMD(do_block_chat);
ACMD(do_block_chat_list);
ACMD(do_build);
// EMOTION
ACMD(do_emotion_allow);
ACMD(do_emotion);
// END OF EMOTION
ACMD(do_hair);
ACMD(do_cube);
ACMD(do_gift);
ACMD(do_dice);
ACMD(do_costume);
ACMD(do_party_invite);
#ifdef __INVENTORY_SORT__
ACMD(do_sort_inventory);
#endif

// trial gamemaster commands
ACMD(do_rewarp);
ACMD(do_warp);
ACMD(do_notice);
ACMD(do_map_notice);
ACMD(do_transfer);
ACMD(do_goto);
ACMD(do_level);
ACMD(do_reset);
ACMD(do_state);
ACMD(do_refine_rod);
ACMD(do_refine_pick);
ACMD(do_max_pick);
ACMD(do_invisibility);
ACMD(do_set_skill_group);
ACMD(do_setskill);
ACMD(do_cooltime);
ACMD(do_getqf);
ACMD(do_set_state);
ACMD(do_view_equip);
ACMD(do_affect_remove);
ACMD(do_stat_plus_amount);
ACMD(do_oxevent_show_quiz);
ACMD(do_oxevent_log);
ACMD(do_oxevent_get_attender);
ACMD(do_effect);
ACMD(do_get_item_id_list);
ACMD(do_get_event_flag);
ACMD(do_console);
ACMD(do_set_stat);
ACMD(do_can_dead);
ACMD(do_full_set);
ACMD(do_item_full_set);
ACMD(do_attr_full_set);
ACMD(do_all_skill_master);
ACMD(do_use_item);
ACMD(do_clear_affect);
ACMD(do_get_distance);
ACMD(do_give_apply_point);
ACMD(do_get_apply_point);
ACMD(do_get_point);
ACMD(do_set_is_show_teamler);
ACMD(do_premium_color);

// gamemaster commands
ACMD(do_get_mob_count);
ACMD(do_inventory);
ACMD(do_polymorph);
ACMD(do_polymorph_item);
ACMD(do_stat_reset);
ACMD(do_stat_reset);
ACMD(do_user);
ACMD(do_big_notice);
#ifdef ENABLE_ZODIAC_TEMPLE
ACMD(do_zodiac_notice);
#endif
ACMD(do_purge);
ACMD(do_item);
ACMD(do_item_purge);
ACMD(do_kill);
ACMD(do_disconnect);
ACMD(do_book);
ACMD(do_stun);
ACMD(do_slow);
ACMD(do_detaillog);
ACMD(do_monsterlog);
ACMD(do_gwlist);
ACMD(do_stop_guild_war);
ACMD(do_cancel_guild_war);
ACMD(do_guild_state);
ACMD(do_forgetme);
ACMD(do_aggregate);
ACMD(do_attract_ranger);
ACMD(do_pull_monster);
ACMD(do_show_arena_list);
ACMD(do_end_all_duel);
ACMD(do_end_duel);
ACMD(do_duel);
ACMD(do_eclipse);

// super gamemaster commands
ACMD(do_break_marriage);
ACMD(do_priv_empire);
ACMD(do_priv_guild);
ACMD(do_event_flag);
ACMD(do_mob);
ACMD(do_mob_ld);
ACMD(do_mob_aggresive);
ACMD(do_mob_coward);
ACMD(do_mob_map);
ACMD(do_group);
ACMD(do_group_random);
ACMD(do_greset);
ACMD(do_respawn);
ACMD(do_clear_quest);
ACMD(do_makeguild);
ACMD(do_deleteguild);
ACMD(do_setqf);
ACMD(do_delqf);
ACMD(do_xmas);
ACMD(do_clear_land);
ACMD(do_setskillother);
ACMD(do_reset_subskill);
ACMD(do_safebox_size);
ACMD(do_weaken);
ACMD(do_advance);
ACMD(do_flush);
ACMD(do_save);
ACMD(do_mount_test);
ACMD(do_private);
ACMD(do_observer);
ACMD(do_set);
ACMD(do_shutdown);
ACMD(do_fishing_simul);
ACMD(do_qf);
ACMD(do_set_skill_point);
ACMD(do_reload);
ACMD(do_save_attribute_to_image);
ACMD(do_set_socket);
ACMD(do_socket_item);
ACMD(do_change_attr);
ACMD(do_add_attr);
ACMD(do_add_socket);
ACMD(do_remove_rights);
ACMD(do_give_rights);
#ifdef __EVENT_MANAGER__
ACMD(do_event_open);
ACMD(do_event_close);
#endif

#ifdef ENABLE_RUNE_SYSTEM
ACMD(do_rune);
ACMD(do_open_rune_points_buy);
ACMD(do_use_buy_rune);
ACMD(do_accept_rune_points_buy);
ACMD(do_reset_runes);
#endif
ACMD(do_remove_affect);

// new commands
#ifdef __ANTI_BRUTEFORCE__
ACMD(do_flush_bruteforce_data);
#endif
ACMD(do_force_item_delete);
#ifdef __HOMEPAGE_COMMAND__
ACMD(do_homepage_process_sleep);
#endif
#ifdef __MAINTENANCE__
ACMD(do_maintenance);
#endif
#ifdef __GUILD_SAFEBOX__
ACMD(do_guild_safebox_open);
ACMD(do_guild_safebox_open_log);
ACMD(do_guild_safebox_close);
#endif
#ifdef __ANIMAL_SYSTEM__
ACMD(do_animal_stat_up);
#endif
ACMD(do_offline_messages_pull);
#ifdef __ITEM_REFUND__
ACMD(do_request_item_refund);
#endif
ACMD(do_fast_change_channel);
ACMD(do_set_inventory_max_num);
ACMD(do_set_uppitem_inv_max_num);
ACMD(do_set_skillbook_inv_max_num);
ACMD(do_set_stone_inv_max_num);
ACMD(do_set_enchant_inv_max_num);
ACMD(do_horse_grade);
ACMD(do_user_mount_set);
ACMD(do_user_mount_action);
ACMD(do_safebox_open);
ACMD(do_killall);
#ifdef __SWITCHBOT__
ACMD(do_switchbot_change_speed);
ACMD(do_switchbot_start);
ACMD(do_switchbot_start_premium);
ACMD(do_switchbot_stop);
#endif
ACMD(do_anti_exp);
#ifdef __PRESTIGE__
ACMD(do_prestige_level);
#endif
#ifdef __DRAGONSOUL__
ACMD(do_dragon_soul);
ACMD(do_ds_list);
#endif
ACMD(do_horse_rage);

ACMD(do_horse_rage_mode);
ACMD(do_horse_refine);

#ifdef __GAYA_SYSTEM__
ACMD(do_gaya_shop_buy);
#endif
ACMD(do_cards);
ACMD(do_test_pvp_char);

#ifdef __FAKE_BUFF__
ACMD(do_setskillfake);
#endif

#ifdef __ATTRTREE__
ACMD(do_attrtree_level_info);
ACMD(do_attrtree_levelup);
#endif

ACMD(do_reload_entitys);
ACMD(do_load_timers);

#ifdef __TIMER_BIO_DUNGEON__
ACMD(do_get_timer_cdrs);
ACMD(do_timer_warp);
#endif

#ifdef __QUEST_PENETRATE_TEST__
ACMD(do_quest_penetrate_test);
#endif

ACMD(do_dungeon_complete_delayed);
ACMD(do_quest_complete_missionbook_delayed);
ACMD(do_quest_drop_delayed);
ACMD(do_set_hide_costumes);
ACMD(do_send_cmd);
ACMD(do_vm_detected);
ACMD(do_hwid_change_detected);

#ifdef __EVENT_MANAGER__
ACMD(do_event_announcement);
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
ACMD(do_SendFishBoxUse);
ACMD(do_SendFishShapeAdd);
ACMD(do_SendFishShapeAdd_Specific);
#endif
ACMD(do_system_annoucement);
ACMD(do_select_angelsdemons_fraction);

ACMD(do_enable_packet_logging);
ACMD(do_test);

#ifdef ENABLE_XMAS_EVENT
ACMD(do_xmas_reward);
#endif

#ifdef ENABLE_WARP_BIND_RING
ACMD(do_request_warp_bind);
#endif

ACMD(do_server_boot_time);
ACMD(do_dungeon_reconnect);

#ifdef ENABLE_RUNE_PAGES
ACMD(do_select_rune);
ACMD(do_get_rune_page);
ACMD(do_reset_rune_page);
ACMD(do_get_selected_page);
#endif

#ifdef SORT_AND_STACK_ITEMS
ACMD(do_stack_items);
#endif

#ifdef SOUL_SYSTEM
ACMD(do_refine_soul_item_info);
ACMD(do_refine_soul_item);
#endif

#if defined(ENABLE_LEVEL2_RUNES) && defined(ENABLE_RUNE_SYSTEM)
ACMD(do_level_rune);
ACMD(do_open_rune_level);
#endif

#ifdef LEADERSHIP_EXTENSION
ACMD(do_leadership_state);
#endif

#ifdef BATTLEPASS_EXTENSION
ACMD(do_battlepass_data);
ACMD(do_battlepass_shop);
#endif

#ifdef DMG_RANKING
ACMD(do_get_dmg_ranks);
#endif

#ifdef DUNGEON_REPAIR_TRIGGER
ACMD(do_dungeon_repair);
#endif

#ifdef ENABLE_REACT_EVENT
ACMD(do_react_event);
#endif

#ifdef __PET_ADVANCED__
ACMD(do_pet_level);
ACMD(do_pet_skill_level);
#endif

#ifdef DS_ALCHEMY_SHOP_BUTTON
ACMD(do_open_alchemy_shop);
#endif

struct command_info cmd_info[] =
{
	{ "!RESERVED!",					NULL,						0,					POS_DEAD,		GM_IMPLEMENTOR,	0	},

	// implementor commands
	{ "flush",						do_flush,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "save",						do_save,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "mount_test",					do_mount_test,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "private",					do_private,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "observer",					do_observer,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "shutdow",					do_inputall,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "shutdown",					do_shutdown,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "fish_simul",					do_fishing_simul,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "qf",							do_qf,						0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "setskillpoint",				do_set_skill_point,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "reload",						do_reload,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "saveati",					do_save_attribute_to_image,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "set_socket",					do_set_socket,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "remove_rights",				do_remove_rights,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "give_rights",				do_give_rights,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "force_item_delete",			do_force_item_delete,		0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#ifdef __ANTI_BRUTEFORCE__
	{ "flush_bruteforce_data",		do_flush_bruteforce_data,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#endif
#ifdef __HOMEPAGE_COMMAND__
	{ "homepage_process_sleep",		do_homepage_process_sleep,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#endif
#ifdef __MAINTENANCE__
	{ "maintenance",				do_maintenance,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#endif
	{ "offline_messages_pull",		do_offline_messages_pull,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#ifdef __ITEM_REFUND__
	{ "request_item_refund",		do_request_item_refund,		0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "pull_item_refund",			do_request_item_refund,		0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#endif
	{ "set_inventory_max_num",		do_set_inventory_max_num,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "set_uppitem_inv_max_num",	do_set_uppitem_inv_max_num, 0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "set_skillbook_inv_max_num",	do_set_skillbook_inv_max_num,0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "set_stone_inv_max_num",		do_set_stone_inv_max_num, 	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "set_enchant_inv_max_num",	do_set_enchant_inv_max_num, 0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "horse_grade",				do_horse_grade,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "killall",					do_killall,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#ifdef __PRESTIGE__
	{ "prestige_level",				do_prestige_level,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#endif
	{ "test_pvp_char",				do_test_pvp_char,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#ifdef __FAKE_BUFF__
	{ "setskillfake",				do_setskillfake,			0,					POS_DEAD,		GM_GOD,			1	},
#endif
	{ "send_cmd",					do_send_cmd,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},

	// GM_GOD -> unused. can be useful through gmconfig (common db)

	// super gamemaster commands
	{ "break_marriage",				do_break_marriage,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "priv_empire",				do_priv_empire,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "set",						do_set,						0,					POS_DEAD,		GM_GOD,	1	},
	{ "priv_guild",					do_priv_guild,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "eventflag",					do_event_flag,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "mob",						do_mob,						0,					POS_DEAD,		GM_GOD,	1	},
	{ "mob_ld",						do_mob_ld,					0,					POS_DEAD,		GM_GOD,	2	},
	{ "ma",							do_mob_aggresive,			0,					POS_DEAD,		GM_GOD,	2	},
	{ "mc",							do_mob_coward,				0,					POS_DEAD,		GM_GOD,	2	},
	{ "mm",							do_mob_map,					0,					POS_DEAD,		GM_GOD,	2	},
	{ "group",						do_group,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "grrandom",					do_group_random,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "greset",						do_greset,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "respawn",					do_respawn,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "clear_quest",				do_clear_quest,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "makeguild",					do_makeguild,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "deleteguild",				do_deleteguild,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "setqf",						do_setqf,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "delqf",						do_delqf,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "xmas_boom",					do_xmas,					SCMD_XMAS_BOOM,		POS_DEAD,		GM_GOD,	1	},
	{ "xmas_snow",					do_xmas,					SCMD_XMAS_SNOW,		POS_DEAD,		GM_GOD,	1	},
	{ "xmas_santa",					do_xmas,					SCMD_XMAS_SANTA,	POS_DEAD,		GM_GOD,	1	},
	{ "block_chat",					do_block_chat,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "clear_land",					do_clear_land,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "setskillother",				do_setskillother,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "reset_subskill",				do_reset_subskill,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "safebox",					do_safebox_size,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "weaken",						do_weaken,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "advance",					do_advance,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "socket_item",				do_socket_item,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "change_attr",				do_change_attr,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "add_attr",					do_add_attr,				0,					POS_DEAD,		GM_GOD,	1	},

#ifdef __EVENT_MANAGER__
	{ "event_open",					do_event_open,				0,					POS_DEAD,		GM_HIGH_WIZARD,	1	},
	{ "event_close",				do_event_close,				0,					POS_DEAD,		GM_HIGH_WIZARD,	1	},
#endif

	// gamemaster commands
	{ "get_mob_count",				do_get_mob_count,			0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "inventory",					do_inventory,				0,					POS_DEAD,		GM_WIZARD,		2	},
	{ "polymorph",					do_polymorph,				0,					POS_DEAD,		GM_WIZARD,		2	},
	{ "polyitem",					do_polymorph_item,			0,					POS_DEAD,		GM_IMPLEMENTOR,		3	},
	{ "stat_reset",					do_stat_reset,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "user",						do_user,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "big_notice",					do_big_notice,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
#ifdef ENABLE_ZODIAC_TEMPLE
	{ "zodiac_notice",	do_zodiac_notice,	0,	POS_DEAD,	GM_HIGH_WIZARD	},
#endif
	{ "purge",						do_purge,					0,					POS_DEAD,		GM_WIZARD,		1	},
	{ "item",						do_item,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "ipurge",						do_item_purge,				0,					POS_DEAD,		GM_WIZARD,		1	},
	{ "kill",						do_kill,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "dc",							do_disconnect,				0,					POS_DEAD,		GM_WIZARD,		1	},
	{ "book",						do_book,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "stun",						do_stun,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "slow",						do_slow,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "detaillog",					do_detaillog,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "monsterlog",					do_monsterlog,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "gwlist",						do_gwlist,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "gwstop",						do_stop_guild_war,			0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "gwcancel",					do_cancel_guild_war,		0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "gstate",						do_guild_state,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "forgetme",					do_forgetme,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "aggregate",					do_aggregate,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "attract_ranger",				do_attract_ranger,			0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "pull_monster",				do_pull_monster,			0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "show_arena_list",			do_show_arena_list,			0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "end_all_duel",				do_end_all_duel,			0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "end_duel",					do_end_duel,				0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "duel",						do_duel,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},
	{ "eclipse",					do_eclipse,					0,					POS_DEAD,		GM_IMPLEMENTOR,		1	},

	// trial gamemaster commands
	{ "warp",						do_warp,					0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "notice",						do_notice,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "notice_map",					do_map_notice,				0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "transfer",					do_transfer,				0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "goto",						do_goto,					0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "level",						do_level,					0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "reset",						do_reset,					0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "state",						do_state,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "refine_rod",					do_refine_rod,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "refine_pick",				do_refine_pick,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "max_pick",					do_max_pick,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "invisible",					do_invisibility,			0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "setjob",						do_set_skill_group,			0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "setskill",					do_setskill,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "cooltime",					do_cooltime,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "getqf",						do_getqf,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "set_state",					do_set_state,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "view_equip",					do_view_equip,				0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "affect_remove",				do_affect_remove,			0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "con+",						do_stat_plus_amount,		POINT_HT,			POS_DEAD,		GM_GOD,	3	},
	{ "int+",						do_stat_plus_amount,		POINT_IQ,			POS_DEAD,		GM_GOD,	3	},
	{ "str+",						do_stat_plus_amount,		POINT_ST,			POS_DEAD,		GM_GOD,	3	},
	{ "dex+",						do_stat_plus_amount,		POINT_DX,			POS_DEAD,		GM_GOD,	3	},
	{ "show_quiz",					do_oxevent_show_quiz,		0,					POS_DEAD,		GM_GOD,	1	},
	{ "log_oxevent",				do_oxevent_log,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "get_oxevent_att",			do_oxevent_get_attender,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "effect",						do_effect,					0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "item_id_list",				do_get_item_id_list,		0,					POS_DEAD,		GM_GOD,	1	},
	{ "geteventflag",				do_get_event_flag,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "console",					do_console,					0,					POS_DEAD,		GM_GOD,	1	},
	{ "tcon",						do_set_stat,				POINT_HT,			POS_DEAD,		GM_GOD,	2	},
	{ "tint",						do_set_stat,				POINT_IQ,			POS_DEAD,		GM_GOD,	2	},
	{ "tstr",						do_set_stat,				POINT_ST,			POS_DEAD,		GM_GOD,	2	},
	{ "tdex",						do_set_stat,				POINT_DX,			POS_DEAD,		GM_GOD,	2	},
	{ "cannot_dead",				do_can_dead,				1,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "can_dead",					do_can_dead,				0,					POS_DEAD,		GM_GOD,	1	},
	{ "full_set",					do_full_set,				0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "item_full_set",				do_item_full_set,			0,					POS_DEAD,		GM_GOD,	2	},
	{ "attr_full_set",				do_attr_full_set,			0,					POS_DEAD,		GM_GOD,	1	},
	{ "all_skill_master",			do_all_skill_master,		0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "use_item",					do_use_item,				0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "clear_affect",				do_clear_affect,		 	0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "get_distance",				do_get_distance,			0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "rewarp",						do_rewarp,			0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "set_is_show_teamler",		do_set_is_show_teamler,		0,					POS_DEAD,		GM_LOW_WIZARD,	1	},
	{ "give_apply_point",			do_give_apply_point,		0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "get_apply_point",			do_get_apply_point,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "get_point",					do_get_point,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},


	// player commands
	{ "fast_change_channel",		do_fast_change_channel,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "ch",							do_fast_change_channel,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "channel",					do_fast_change_channel,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "dungeon_debug_info",			do_dungeon_debug_info,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "war",						do_war,						0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "nowar",						do_nowar,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "stat",						do_stat,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "stat-",						do_stat_minus,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "restart_here",				do_restart,					SCMD_RESTART_HERE,	POS_DEAD,		GM_PLAYER,		1	},
	{ "restart_town",				do_restart,					SCMD_RESTART_TOWN,	POS_DEAD,		GM_PLAYER,		1	},
	{ "restart_base",				do_restart,					SCMD_RESTART_BASE,	POS_DEAD,		GM_PLAYER,		1	},
#ifdef COMBAT_ZONE
	{ "restart_combat_zone",		do_restart,					SCMD_RESTART_COMBAT_ZONE,POS_DEAD,	GM_PLAYER,		1	},
#endif
	{ "phase_selec",				do_inputall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "phase_select",				do_cmd,						SCMD_PHASE_SELECT,	POS_DEAD,		GM_PLAYER,		1	},
	{ "qui",						do_inputall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "quit",						do_cmd,						SCMD_QUIT,			POS_DEAD,		GM_PLAYER,		1	},
	{ "logou",						do_inputall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "logout",						do_cmd,						SCMD_LOGOUT,		POS_DEAD,		GM_PLAYER,		1	},
	{ "skillup",					do_skillup,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "gskillup",					do_guildskillup,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "pvp",						do_pvp,						0,					POS_DEAD,		GM_PLAYER,		2	},
	// SAFEBOX/MALL
	{ "safebox_close",				do_safebox_close,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "safebox_passwor",			do_inputall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "safebox_password",			do_safebox_password,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "safebox_change_passwor",		do_inputall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "safebox_change_password",	do_safebox_change_password,	0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "mall_passwor",				do_inputall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "mall_password",				do_mall_password,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "mall_open",					do_mall_open,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "mall_close",					do_mall_close,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "in_game_mall",				do_in_game_mall,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "click_mall",					do_click_mall,				0,					POS_DEAD,		GM_PLAYER,		1	},
	// END OF SAFEBOX/MALL
	{ "ungroup",					do_ungroup,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "close_shop",					do_close_shop,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "set_walk_mode",				do_set_walk_mode,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "set_run_mode",				do_set_run_mode,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "pkmode",						do_pkmode,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "messenger_auth",				do_messenger_auth,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "setblockmode",				do_setblockmode,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "unmount",					do_unmount,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "party_request",				do_party_request,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "party_request_accept",		do_party_request_accept,	0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "party_request_deny",			do_party_request_deny,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "observer_exit",				do_observer_exit,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "block_chat",					do_block_chat,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "block_chat_list",			do_block_chat_list,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "build",						do_build,					0,					POS_DEAD,		GM_PLAYER,		1	},
	// EMOTION
	{ "emotion_allow",				do_emotion_allow,			0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "kiss",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "slap",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "french_kiss",				do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "clap",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "cheer1",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "cheer2",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "dance1",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "dance2",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "dance3",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "dance4",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "dance5",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "dance6",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "congratulation",				do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "forgive",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "angry",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "attractive",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "sad",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "shy",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "cheerup",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "banter",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "joy",						do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
#ifdef ENABLE_NEW_EMOTES
	{ "new_emote1",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote2",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote3",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote4",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote5",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote6",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote7",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote8",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote9",					do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote10",				do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote11",				do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
	{ "new_emote12",				do_emotion,					0,					POS_FIGHTING,	GM_PLAYER,		1	},
#endif
	// END OF EMOTION
	{ "hair",						do_hair,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "cube",						do_cube,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "gift",						do_gift,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "dice",						do_dice,					0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "costume",					do_costume, 				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "party_invity",				do_party_invite,			0,					POS_DEAD,		GM_PLAYER,		1	},
#ifdef __INVENTORY_SORT__
	{ "sort_inventory",				do_sort_inventory,			0,					POS_DEAD,		GM_PLAYER,		1	},
#endif
#ifdef __GUILD_SAFEBOX__
	{ "guild_safebox_open",			do_guild_safebox_open,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "guild_safebox_open_log",		do_guild_safebox_open_log,	0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "guild_safebox_close",		do_guild_safebox_close,		0,					POS_DEAD,		GM_PLAYER,		1	},
#endif
#ifdef __ANIMAL_SYSTEM__
	{ "animal_stat_up",				do_animal_stat_up,			0,					POS_DEAD,		GM_PLAYER,		1	},
#endif
	{ "user_mount_set",				do_user_mount_set,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "user_mount_action",			do_user_mount_action,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "safebox_open",				do_safebox_open,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "anti_exp",					do_anti_exp,				0,					POS_DEAD,		GM_PLAYER,		1	},

#ifdef __SWITCHBOT__
	{ "switchbot_change_speed",		do_switchbot_change_speed,	0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "switchbot_start",			do_switchbot_start,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "switchbot_start_premium",	do_switchbot_start_premium,	0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "switchbot_stop",				do_switchbot_stop,			0,					POS_DEAD,		GM_PLAYER,		1	},
#endif

#ifdef __DRAGONSOUL__
	{ "dragon_soul",				do_dragon_soul,				0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "ds_list",					do_ds_list,					0,					POS_DEAD,		GM_PLAYER,		1	},
#endif
	{ "horse_rage",					do_horse_rage,				0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	{ "horse_rage_mode",			do_horse_rage_mode,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "horse_refine",				do_horse_refine,			0,					POS_DEAD,		GM_PLAYER,		1	},

#ifdef __GAYA_SYSTEM__
	{ "gaya_shop_buy",				do_gaya_shop_buy,			0,					POS_DEAD,		GM_PLAYER,		1	},
#endif

	{ "cards",                		do_cards,                	0,    				POS_DEAD,    	GM_PLAYER,		1   },

#ifdef __ATTRTREE__
	{ "attrtree_level_info",		do_attrtree_level_info,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "attrtree_levelup",			do_attrtree_levelup,		0,					POS_DEAD,		GM_PLAYER,		1	},
#endif
	{ "reload_environment",			do_reload_entitys,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "load_timers",				do_load_timers,				0,					POS_DEAD,		GM_PLAYER,		1	},

#ifdef __TIMER_BIO_DUNGEON__
	{ "get_timer_cdrs",				do_get_timer_cdrs,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "timer_warp",					do_timer_warp,				0,					POS_DEAD,		GM_PLAYER,		1	},
#endif
	
#ifdef __QUEST_PENETRATE_TEST__
	{ "quest_penetration_test",		do_quest_penetrate_test,	0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
#endif
	{ "test",						do_test,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1	},
	
	{ "dungeon_complete_delayed",	do_dungeon_complete_delayed,0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "quest_complete_missionbook_delayed",	do_quest_complete_missionbook_delayed,0,POS_DEAD,		GM_PLAYER,		1 },
	{ "quest_drop_item_delayed",	do_quest_drop_delayed,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "premium_color",				do_premium_color,			0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "channel_change",				do_fast_change_channel,		0,					POS_DEAD,		GM_PLAYER,		1	},
	{ "set_hide_costumes",			do_set_hide_costumes,		0,					POS_DEAD,		GM_PLAYER,		1	},
	
#ifdef ENABLE_RUNE_SYSTEM
	{ "rune",						do_rune,					0,					POS_DEAD,		GM_IMPLEMENTOR,	1 },
	{ "open_rune_points_buy",		do_open_rune_points_buy,	0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "use_buy_rune",				do_use_buy_rune,			0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "accept_rune_points_buy",		do_accept_rune_points_buy,	0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "reset_runes",				do_reset_runes,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
	{ "remove_affect",				do_remove_affect,			0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "virtualmachine",				do_vm_detected,				0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "changed_hwid",				do_hwid_change_detected,	0,					POS_DEAD,		GM_PLAYER,		1 },

#ifdef __EVENT_MANAGER__
	{ "event_announcement",			do_event_announcement,		0,					POS_DEAD,		GM_IMPLEMENTOR,	1 },
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
	{ "sendfishboxuse",				do_SendFishBoxUse,			0,					POS_DEAD,		GM_PLAYER,	1 },
	{ "sendfishshapeadd",			do_SendFishShapeAdd,		0,					POS_DEAD,		GM_PLAYER,	1 },
	{ "addfish",					do_SendFishShapeAdd_Specific,0,					POS_DEAD,		GM_IMPLEMENTOR,	1 },
#endif
	{ "system_announcement",		do_system_annoucement,		0,					POS_DEAD,		GM_IMPLEMENTOR,	1 },

	{ "angelsdemons_select_fraction",	do_select_angelsdemons_fraction, 0,			POS_DEAD,		GM_PLAYER,		1 },

	{ "enable_logging",				do_enable_packet_logging,	0,					POS_DEAD,		GM_IMPLEMENTOR, 1 },

#ifdef ENABLE_XMAS_EVENT
	{ "xmas_reward",				do_xmas_reward,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif

#ifdef ENABLE_WARP_BIND_RING
	{ "request_warp_bind",			do_request_warp_bind,		0,					POS_DEAD,		GM_PLAYER,		1 },
#endif

	{ "server_boot_time",			do_server_boot_time,		0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "dungeon_reconnect",			do_dungeon_reconnect,		0,					POS_DEAD,		GM_PLAYER,		1 },

#ifdef ENABLE_RUNE_PAGES
	{ "select_rune",				do_select_rune,				0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "get_rune_page",				do_get_rune_page,			0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "reset_rune_page",			do_reset_rune_page,			0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "get_selected_page",			do_get_selected_page,		0,					POS_DEAD,		GM_PLAYER,		1 },
#endif

#ifdef SORT_AND_STACK_ITEMS
	{ "stack_items",				do_stack_items,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif

#ifdef SOUL_SYSTEM
	{ "refine_soul_item",			do_refine_soul_item,		0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "refine_soul_item_info",		do_refine_soul_item_info,	0,					POS_DEAD,		GM_PLAYER,		1 },
#endif

#if defined(ENABLE_LEVEL2_RUNES) && defined(ENABLE_RUNE_SYSTEM)
	{ "level_rune",					do_level_rune,				0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "open_rune_level",			do_open_rune_level,			0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef LEADERSHIP_EXTENSION
	{ "leadership_state",			do_leadership_state,		0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef PACKET_ERROR_DUMP
	{ "syserr_log",					do_syserr_log,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef BATTLEPASS_EXTENSION
	{ "battlepass_data",			do_battlepass_data,			0,					POS_DEAD,		GM_PLAYER,		1 },
	{ "battlepass_shop",			do_battlepass_shop,			0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef USER_ATK_SPEED_LIMIT
	{ "attackspeed_limit",			do_attackspeed_limit,		0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef CHARACTER_BUG_REPORT
	{ "character_bug_report",		do_character_bug_report,	0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
	{ "guild_info",					do_guild_members_lastplayed,	0,				POS_DEAD,		GM_PLAYER,		1 },

#ifdef ACCOUNT_TRADE_BLOCK
	{ "verify_hwid",				do_verify_hwid,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef BLACKJACK
	{ "blackjack",					do_blackjack,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef DMG_RANKING
	{ "get_dmg_ranks",				do_get_dmg_ranks,			0,					POS_DEAD,		GM_IMPLEMENTOR,	1 },
#endif
	{ "user_warp",					do_user_warp,				0,					POS_DEAD,		GM_PLAYER,		1 },
#ifdef __EQUIPMENT_CHANGER__
	{ "equipment_changer_load",		do_equipment_changer_load,	0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef DUNGEON_REPAIR_TRIGGER
	{ "dungeon_repair",				do_dungeon_repair,			0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef ENABLE_REACT_EVENT
	{ "react_event",				do_react_event,				0,					POS_DEAD,		GM_PLAYER,		1 },
#endif
#ifdef HALLOWEEN_MINIGAME
	{ "halloween_minigame",			do_halloween_minigame,		0,					POS_DEAD,		GM_PLAYER,		1 },
#endif

#ifdef __PET_ADVANCED__
	{ "pet_level", do_pet_level, 0, POS_DEAD, GM_IMPLEMENTOR, 1 },
	{ "pet_skill_level", do_pet_skill_level, 0, POS_DEAD, GM_IMPLEMENTOR, 1 },
#endif

#ifdef DS_ALCHEMY_SHOP_BUTTON
	{ "open_alchemy_shop", do_open_alchemy_shop, 0, POS_DEAD, GM_PLAYER, 1 },
#endif

	{ "\n",							NULL,						0,					POS_DEAD,		GM_IMPLEMENTOR,	0 }
};

void interpreter_set_privilege(const char *cmd, int lvl)
{
	int i;

	for (i = 0; *cmd_info[i].command != '\n'; ++i)
	{
		if (!str_cmp(cmd, cmd_info[i].command))
		{
			cmd_info[i].gm_level = lvl;
			sys_log(0, "Setting command privilege: %s -> %d", cmd, lvl);
			break;
		}
	}
}

void double_dollar(const char *src, size_t src_len, char *dest, size_t dest_len)
{	   
	const char * tmp = src;
	size_t cur_len = 0;

	// \0 ³ÖÀ» ÀÚ¸® È®º¸
	dest_len -= 1;

	while (src_len-- && *tmp)
	{
		if (*tmp == '$')
		{
			if (cur_len + 1 >= dest_len)
				break;

			*(dest++) = '$';
			*(dest++) = *(tmp++);
			cur_len += 2;
		}
		else
		{
			if (cur_len >= dest_len)
				break;

			*(dest++) = *(tmp++);
			cur_len += 1;
		}
	}

	*dest = '\0';
}

void interpret_command(LPCHARACTER ch, const char * argument, size_t len)
{
	if (NULL == ch)
	{
		sys_err ("NULL CHRACTER");
		return ;
	}

	char cmd[128 + 1];  // buffer overflow ¹®Á¦°¡ »ý±âÁö ¾Êµµ·Ï ÀÏºÎ·¯ ±æÀÌ¸¦ Âª°Ô ÀâÀ½
	char new_line[256 + 1];
	const char * line;
	int icmd;
	int isavecmd;

	if (len == 0 || !*argument)
		return;

	double_dollar(argument, len, new_line, sizeof(new_line));

	size_t cmdlen;
	line = first_cmd(new_line, cmd, sizeof(cmd), &cmdlen);

	for (icmd = 1, isavecmd = -1; *cmd_info[icmd].command != '\n'; ++icmd)
	{
		if (cmd_info[icmd].command_pointer == do_cmd)
		{
			if (!strcmp(cmd_info[icmd].command, cmd)) // do_cmd´Â ¸ðµç ¸í·É¾î¸¦ ÃÄ¾ß ÇÒ ¼ö ÀÖ´Ù.
				break;
		}
		else if (!strncmp(cmd_info[icmd].command, cmd, cmdlen) && cmd_info[icmd].gm_level <= ch->GetGMLevel())
		{
			if (strlen(cmd_info[icmd].command) == cmdlen)
				break;
			else if (isavecmd == -1 ||
				((cmd_info[icmd].gm_level != GM_PLAYER || cmd_info[isavecmd].gm_level == GM_PLAYER) &&
				((cmd_info[isavecmd].level == 0 || cmd_info[isavecmd].level > cmd_info[icmd].level) ||
					(
					 cmd_info[isavecmd].level == cmd_info[icmd].level &&
					 abs((int)(cmdlen - strlen(cmd_info[isavecmd].command))) > abs((int)(cmdlen - strlen(cmd_info[icmd].command)))
					)
				)))
			{
				isavecmd = icmd;
			}
		}
	}

	if (*cmd_info[icmd].command == '\n' && isavecmd != -1)
		icmd = isavecmd;

	if (ch->GetPosition() < cmd_info[icmd].minimum_position)
	{
		switch (ch->GetPosition())
		{
			case POS_MOUNTING:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Åº »óÅÂ¿¡¼­´Â ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				break;

			case POS_DEAD:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾²·¯Áø »óÅÂ¿¡¼­´Â ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				break;

			case POS_SLEEPING:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "²Þ¼Ó¿¡¼­ ¾î¶»°Ô¿ä?"));
				break;

			case POS_RESTING:
			case POS_SITTING:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸ÕÀú ÀÏ¾î ³ª¼¼¿ä."));
				break;
				/*
				   case POS_FIGHTING:
				   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸ñ¼ûÀ» °É°í ÀüÅõ Áß ÀÔ´Ï´Ù. ÁýÁß ÇÏ¼¼¿ä."));
				   break;
				 */
			default:
				sys_err("unknown position %d", ch->GetPosition());
				break;
		}

		return;
	}

	if (cmd_info[icmd].level == 0)
	{   
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±×·± ¸í·É¾î´Â ¾ø½À´Ï´Ù"));
		return;
	}

	if (cmd_info[icmd].gm_level && cmd_info[icmd].gm_level > ch->GetGMLevel())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±×·± ¸í·É¾î´Â ¾ø½À´Ï´Ù"));
		return;
	}

	if (strncmp("phase", cmd_info[icmd].command, 5) != 0) // È÷µç ¸í·É¾î Ã³¸® 
		sys_log(0, "COMMAND: %s: %s", ch->GetName(), cmd_info[icmd].command);

	if (ch->GetGMLevel() >= GM_LOW_WIZARD)
	{
		if (cmd_info[icmd].gm_level >= GM_LOW_WIZARD)
		{
			char buf[1024];
			snprintf(buf, sizeof(buf), "%s", argument);

			LogManager::instance().GMCommandLog(ch->GetPlayerID(), ch->GetName(), ch->GetDesc()->GetHostName(), g_bChannel, buf);
		}
	}

	((*cmd_info[icmd].command_pointer) (ch, line, icmd, cmd_info[icmd].subcmd));
}

void direct_interpret_command(const char * argument, LPCHARACTER ch)
{

	char cmd[128 + 1];  // buffer overflow ¹®Á¦°¡ »ý±âÁö ¾Êµµ·Ï ÀÏºÎ·¯ ±æÀÌ¸¦ Âª°Ô ÀâÀ½
	char new_line[256 + 1];
	const char * line;
	int icmd;
	int isavecmd;

	if (!*argument)
		return;

	double_dollar(argument, strlen(argument), new_line, sizeof(new_line));

	size_t cmdlen;
	line = first_cmd(new_line, cmd, sizeof(cmd), &cmdlen);

	for (icmd = 1, isavecmd = -1; *cmd_info[icmd].command != '\n'; ++icmd)
	{
		if (cmd_info[icmd].command_pointer == do_cmd)
		{
			if (!strcmp(cmd_info[icmd].command, cmd)) // do_cmd´Â ¸ðµç ¸í·É¾î¸¦ ÃÄ¾ß ÇÒ ¼ö ÀÖ´Ù.
				break;
		}
		else if (!strcmp(cmd_info[icmd].command, cmd))
			break;
	}

	if (cmd_info[icmd].level == 0)
	{
		sys_err("invalid gm command [real %s, read %s] %s", cmd, cmd_info[icmd].command, argument);
		return;
	}

	((*cmd_info[icmd].command_pointer) (ch, line, icmd, cmd_info[icmd].subcmd));
}

