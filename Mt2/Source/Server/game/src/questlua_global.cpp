﻿#include "stdafx.h"
#include <sstream>
#include "constants.h"
#include "char.h"
#include "char_manager.h"
#include "log.h"
#include "questmanager.h"
#include "questlua.h"
#include "questevent.h"
#include "config.h"
#include "mining.h"
#include "fishing.h"
#include "priv_manager.h"
#include "utils.h"
#include "p2p.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "start_position.h"
#include "over9refine.h"
#include "OXEvent.h"
#include "regen.h"
#include "cmd.h"
#include "guild.h"
#include "guild_manager.h" 
#include "sectree_manager.h"
#include "db.h"
#include <boost/algorithm/string/predicate.hpp>

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)

#endif

extern ACMD(do_block_chat);

namespace quest
{
	int _get_locale(lua_State* L)
	{
		lua_pushstring(L, CLocaleManager::instance().GetLocale().c_str());
		return 1;
	}

	int _number(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, random_number((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2)));
		return 1;
	}

	int _time_to_str(lua_State* L)
	{
		time_t curTime = (time_t)lua_tonumber(L, -1);
		lua_pushstring(L, asctime(gmtime(&curTime)));
		return 1;
	}

	int _say(lua_State* L)
	{
		ostringstream s;
		combine_lua_string(L, s);
		std::string stAddScript = s.str().c_str();
		if (!boost::algorithm::ends_with(stAddScript.c_str(), "{ENTER}") && !stAddScript.empty() && !boost::algorithm::ends_with(stAddScript.c_str(), " "))
			stAddScript += " ";
		CQuestManager::Instance().AddScript(stAddScript);
		return 0;
	}

	int _tchat(lua_State* L)
	{
		if(test_server)
		{
			ostringstream s;
			combine_lua_string(L, s);
			CQuestManager::instance().GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_INFO, s.str().c_str());
		}
		return 0;
	}

	int _chat(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		ostringstream s;
		combine_lua_string(L, s);
		
		pkChr->ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT_TYPE_EX(CLocaleManager::LANG_TYPE_QUEST, pkChr, s.str().c_str()));
		return 0;
	}

	int _cmdchat(lua_State* L)
	{
		ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::Instance().GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_COMMAND, "%s", s.str().c_str());
		return 0;
	}

	int _syschat(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		ostringstream s;
		combine_lua_string(L, s);
		pkChr->ChatPacket(CHAT_TYPE_INFO, "%s", LC_TEXT_TYPE_EX(CLocaleManager::LANG_TYPE_QUEST, pkChr, s.str().c_str()));
		return 0;
	}

	int _notice(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		ostringstream s;
		combine_lua_string(L, s);
		pkChr->ChatPacket(CHAT_TYPE_NOTICE, "%s", LC_TEXT_TYPE_EX(CLocaleManager::LANG_TYPE_QUEST, pkChr, s.str().c_str()));
		return 0;
	}

	int _left_image(lua_State* L)
	{
		if (lua_isstring(L, -1))
		{
			string s = lua_tostring(L,-1);
			CQuestManager::Instance().AddScript("{LEFTIMAGE src;"+s+"}");
		}
		return 0;
	}

	int _top_image(lua_State* L)
	{
		if (lua_isstring(L, -1))
		{
			string s = lua_tostring(L,-1);
			CQuestManager::Instance().AddScript("{TOPIMAGE src;"+s+"}");
		}
		return 0;
	}

	int _set_skin(lua_State* L) // Quest UI style
	{
		if (lua_isnumber(L, -1))
		{
			CQuestManager::Instance().SetSkinStyle((int)rint(lua_tonumber(L,-1)));
		}
		else
		{
			sys_err("QUEST wrong skin index");
		}

		return 0;
	}

	int _set_server_timer(lua_State* L)
	{
		int n = lua_gettop(L);
		if ((n != 2 || !lua_isnumber(L, 2) || !lua_isstring(L, 1)) &&
				(n != 3 || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)))
		{
			sys_err("QUEST set_server_timer argument count wrong.");
			return 0;
		}

		const char * name = lua_tostring(L, 1);
		double t = lua_tonumber(L, 2);
		DWORD arg = 0;

		CQuestManager & q = CQuestManager::instance();

		if (lua_isnumber(L, 3))
			arg = (DWORD) lua_tonumber(L, 3);

		int timernpc = q.LoadTimerScript(name);

		LPEVENT event = quest_create_server_timer_event(name, t, timernpc, false, arg);
		q.AddServerTimer(name, arg, event);
		return 0;
	}

	int _set_server_loop_timer(lua_State* L)
	{
		int n = lua_gettop(L);
		if ((n != 2 || !lua_isnumber(L, 2) || !lua_isstring(L, 1)) &&
				(n != 3 || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3)))
		{
			sys_err("QUEST set_server_timer argument count wrong.");
			return 0;
		}
		const char * name = lua_tostring(L, 1);
		double t = lua_tonumber(L, 2);
		DWORD arg = 0;
		CQuestManager & q = CQuestManager::instance();

		if (lua_isnumber(L, 3))
			arg = (DWORD) lua_tonumber(L, 3);

		int timernpc = q.LoadTimerScript(name);

		LPEVENT event = quest_create_server_timer_event(name, t, timernpc, true, arg);
		q.AddServerTimer(name, arg, event);
		return 0;
	}

	int _clear_server_timer(lua_State* L)
	{
		CQuestManager & q = CQuestManager::instance();
		const char * name = lua_tostring(L, 1);
		DWORD arg = (DWORD) lua_tonumber(L, 2);
		q.ClearServerTimer(name, arg);
		return 0;
	}

	int _set_named_loop_timer(lua_State* L)
	{
		int n = lua_gettop(L);

		if (n != 2 || !lua_isnumber(L, -1) || !lua_isstring(L, -2))
			sys_err("QUEST set_timer argument count wrong.");
		else
		{
			const char * name = lua_tostring(L, -2);
			double t = lua_tonumber(L, -1);

			CQuestManager & q = CQuestManager::instance();
			int timernpc = q.LoadTimerScript(name);
			q.GetCurrentPC()->AddTimer(name, quest_create_timer_event(name, q.GetCurrentCharacterPtr()->GetPlayerID(), t, timernpc, true));
		}

		return 0;
	}

	int _get_server_timer_arg(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetServerTimerArg());
		return 1;
	}

	int _set_timer(lua_State* L)
	{
		if (lua_gettop(L) != 1 || !lua_isnumber(L, -1))
			sys_err("QUEST invalid argument.");
		else
		{
			double t = lua_tonumber(L, -1);

			CQuestManager& q = CQuestManager::instance();
			quest_create_timer_event("", q.GetCurrentCharacterPtr()->GetPlayerID(), t);
		}

		return 0;
	}

	int _set_named_timer(lua_State* L)
	{
		int n = lua_gettop(L);

		if (n != 2 || !lua_isnumber(L, -1) || !lua_isstring(L, -2))
		{
			sys_err("QUEST set_timer argument count wrong.");
		}
		else
		{
			const char * name = lua_tostring(L,-2);
			double t = lua_tonumber(L, -1);

			CQuestManager & q = CQuestManager::instance();
			int timernpc = q.LoadTimerScript(name);
			q.GetCurrentPC()->AddTimer(name, quest_create_timer_event(name, q.GetCurrentCharacterPtr()->GetPlayerID(), t, timernpc));
		}

		return 0;
	}

	int _timer(lua_State * L)
	{
		if (lua_gettop(L) == 1)
			return _set_timer(L);
		else
			return _set_named_timer(L);
	}

	int _clear_named_timer(lua_State* L)
	{
		int n = lua_gettop(L);

		if (n != 1 || !lua_isstring(L, -1))
			sys_err("QUEST set_timer argument count wrong.");
		else
		{
			CQuestManager & q = CQuestManager::instance();
			q.GetCurrentPC()->RemoveTimer(lua_tostring(L, -1));
		}

		return 0;
	}

	int _getnpcid(lua_State * L)
	{
		const char * name = lua_tostring(L, -1);
		CQuestManager & q = CQuestManager::instance();
		lua_pushnumber(L, q.FindNPCIDByName(name));
		return 1;
	}

	int _is_test_server(lua_State * L)
	{
		lua_pushboolean(L, test_server);
		return 1;
	}

	int _is_pvp_server(lua_State * L)
	{
		lua_pushboolean(L, pvp_server);
		return 1;
	}

	int _raw_script(lua_State* L)
	{
		if ( test_server )
			sys_log ( 0, "_raw_script : %s ", lua_tostring(L,-1));
		if (lua_isstring(L, -1))
			CQuestManager::Instance().AddScript(lua_tostring(L,-1));
		else
			sys_err("QUEST wrong argument: questname: %s", CQuestManager::instance().GetCurrentQuestName().c_str());

		return 0;
	}

	int _char_log(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		DWORD what = 0;
		const char* how = "";
		const char* hint = "";

		if (lua_isnumber(L, 1)) what = (DWORD)lua_tonumber(L, 1);
		if (lua_isstring(L, 2)) how = lua_tostring(L, 2);
		if (lua_tostring(L, 3)) hint = lua_tostring(L, 3);

		LogManager::instance().CharLog(ch, what, how, hint);
		return 0;
	}
	
	int _item_log(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		DWORD dwItemID = 0;
		const char* how = "";
		const char* hint = "";

		if ( lua_isnumber(L, 1) ) dwItemID = (DWORD)lua_tonumber(L, 1);
		if ( lua_isstring(L, 2) ) how = lua_tostring(L, 2);
		if ( lua_tostring(L, 3) ) hint = lua_tostring(L, 3);

		LPITEM item = ITEM_MANAGER::instance().Find(dwItemID);

		if (item)
			LogManager::instance().ItemLog(ch, item, how, hint);

		return 0;
	}

	int _syslog(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isstring(L, 2))
			return 0;

		if (lua_tonumber(L, 1) >= 1)
		{
			if (!test_server)
				return 0;
		}

		PC* pc = CQuestManager::instance().GetCurrentPC();
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		sys_log(0, "QUEST: quest: %s player: %s : %s", pc ? pc->GetCurrentQuestName().c_str() : "<unknown>", ch ? ch->GetName() : "<unknown>", lua_tostring(L, 2));

		if (!ch)
			return 0;

		if (true == test_server)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "QUEST_SYSLOG %s", lua_tostring(L, 2));
		}

		return 0;
	}

	int _syserr(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		PC* pc = CQuestManager::instance().GetCurrentPC();

		if (!pc)
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!ch)
			return 0;

		sys_err("QUEST: quest: %s player: %s : %s", pc->GetCurrentQuestName().c_str(), ch->GetName(), lua_tostring(L, 1));
		ch->ChatPacket(CHAT_TYPE_INFO, "QUEST_SYSERR %s", lua_tostring(L, 1));
		return 0;
	}

	int _add_bgm_info(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isstring(L, 2))
			return 0;

		int mapIndex		= (int)lua_tonumber(L, 1);

		const char*	bgmName	= lua_tostring(L, 2);
		if (!bgmName)
			return 0;

		float bgmVol = lua_isnumber(L, 3) ? lua_tonumber(L, 3) : (1.0f/5.0f)*0.1f;

		CHARACTER_AddBGMInfo(mapIndex, bgmName, bgmVol);

		return 0;
	}
	// END_OF_LUA_ADD_BGM_INFO

	// LUA_ADD_GOTO_INFO
	int _add_goto_info(lua_State* L)
	{
		const char* name = lua_tostring(L, 1);

		int empire 	= (int)lua_tonumber(L, 2);
		int mapIndex 	= (int)lua_tonumber(L, 3);
		int x 		= lua_isnumber(L, 4) ? (int)lua_tonumber(L, 4) : 0;
		int y 		= lua_isnumber(L, 5) ? (int)lua_tonumber(L, 5) : 0;

		if (!name)
			return 0;

		CHARACTER_AddGotoInfo(name, empire, mapIndex, x, y);
		return 0;
	}
	// END_OF_LUA_ADD_GOTO_INFO

	int _add_warp_levellimit(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			return 0;

		int iMapIndex = lua_tonumber(L, 1);
		int iLevelLimit = lua_tonumber(L, 2);
		bool bIsLimitMin = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : true;

		CHARACTER_AddWarpLevelLimit(iMapIndex, iLevelLimit, bIsLimitMin);

		return 0;
	}

	// REFINE_PICK
	int _refine_pick(lua_State* L)
	{
		WORD wCell = (BYTE) lua_tonumber(L,-1);

		CQuestManager& q = CQuestManager::instance();

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		LPITEM item = ch->GetInventoryItem(wCell);

		int ret = mining::RealRefinePick(ch, item);
		lua_pushnumber(L, ret);
		return 1;
	}
	// END_OF_REFINE_PICK

	int _fish_real_refine_rod(lua_State* L)
	{
		WORD wCell = (BYTE) lua_tonumber(L,-1);

		CQuestManager& q = CQuestManager::instance();

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		LPITEM item = ch->GetInventoryItem(wCell);

		int ret = fishing::RealRefineRod(ch, item);
		lua_pushnumber(L, ret);
		return 1;
	}

	int _give_char_privilege(lua_State* L)
	{
		int pid = CQuestManager::instance().GetCurrentCharacterPtr()->GetPlayerID();
		int type = (int)lua_tonumber(L, 1);
		int value = (int)lua_tonumber(L, 2);

		if (MAX_PRIV_NUM <= type)
		{
			sys_err("PRIV_MANAGER: _give_char_privilege: wrong empire priv type(%u)", type);
			return 0;
		}

		CPrivManager::instance().RequestGiveCharacterPriv(pid, type, value);

		return 0;
	}

	int _give_empire_privilege(lua_State* L)
	{
		int empire = (int)lua_tonumber(L,1);
		int type = (int)lua_tonumber(L, 2);
		int value = (int)lua_tonumber(L, 3);
		int time = (int) lua_tonumber(L,4);
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (MAX_PRIV_NUM <= type)
		{
			sys_err("PRIV_MANAGER: _give_empire_privilege: wrong empire priv type(%u)", type);
			return 0;
		}

		if (ch)
			sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, time=%d), by quest, %s", 
					empire, type, value, time, ch->GetName());
		else
			sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, time=%d), by quest, NULL",
					empire, type, value, time);

		CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, time);
		return 0;
	}

	int _give_guild_privilege(lua_State* L)
	{
		int guild_id = (int)lua_tonumber(L,1);
		int type = (int)lua_tonumber(L, 2);
		int value = (int)lua_tonumber(L, 3);
		int time = (int)lua_tonumber( L, 4 );

		if (MAX_PRIV_NUM <= type)
		{
			sys_err("PRIV_MANAGER: _give_guild_privilege: wrong empire priv type(%u)", type);
			return 0;
		}

		sys_log(0, "_give_guild_privileage(empire=%d, type=%d, value=%d, time=%d)", 
				guild_id, type, value, time);

		CPrivManager::instance().RequestGiveGuildPriv(guild_id,type,value,time);

		return 0;
	}

	int _get_empire_privilege_string(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		int empire = (int) lua_tonumber(L, 1);
		ostringstream os;
		bool found = false;

		for (int type = PRIV_NONE + 1; type < MAX_PRIV_NUM; ++type)
		{
			CPrivManager::SPrivEmpireData* pkPrivEmpireData = CPrivManager::instance().GetPrivByEmpireEx(empire, type);

			if (pkPrivEmpireData && pkPrivEmpireData->m_value)
			{
				if (found)
					os << ", ";

				os << LC_TEXT(pkChr, c_apszPrivNames[type]) << " : " << 
					pkPrivEmpireData->m_value << "%" << " (" <<
					((pkPrivEmpireData->m_end_time_sec-get_global_time())/3600.0f) << " hours)" << endl;
				found = true;
			}
		}

		if (!found)
			os << "None!" << endl;

		lua_pushstring(L, os.str().c_str());
		return 1;
	}

	int _get_empire_privilege(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L, 0);
			lua_pushnumber(L, 0);
			return 2;
		}
		int empire = (int)lua_tonumber(L,1);
		int type = (int)lua_tonumber(L, 2);
		CPrivManager::SPrivEmpireData* pkPrivEmpireData = CPrivManager::instance().GetPrivByEmpireEx(empire, type);
		int value = 0;
		int timeleft = 0;
		if (pkPrivEmpireData && pkPrivEmpireData->m_value)
		{
			value = pkPrivEmpireData->m_value;
			timeleft = pkPrivEmpireData->m_end_time_sec - get_global_time();
		}
		lua_pushnumber(L, value);
		lua_pushnumber(L, timeleft);
		return 2;
	}

	int _get_guild_privilege_string(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		int guild = (int) lua_tonumber(L,1);
		ostringstream os;
		bool found = false;

		for (int type = PRIV_NONE+1; type < MAX_PRIV_NUM; ++type)
		{
			const CPrivManager::SPrivGuildData* pPrivGuildData = CPrivManager::instance().GetPrivByGuildEx( guild, type );

			if (pPrivGuildData && pPrivGuildData->value)
			{
				if (found)
					os << ", ";

				os << LC_TEXT(pkChr, c_apszPrivNames[type]) << " : " << pPrivGuildData->value << "%"
					<< " (" << ((pPrivGuildData->end_time_sec - get_global_time()) / 3600.0f) << " hours)" << endl;
				found = true;
			}
		}

		if (!found)
			os << "None!" << endl;

		lua_pushstring(L, os.str().c_str());
		return 1;
	}

	int _get_guildid_byname( lua_State* L )
	{
		if ( !lua_isstring( L, 1 ) ) {
			sys_err( "_get_guildid_byname() - invalud argument" );
			lua_pushnumber( L, 0 );
			return 1;
		}

		const char* pszGuildName = lua_tostring( L, 1 );
		CGuild* pFindGuild = CGuildManager::instance().FindGuildByName( pszGuildName );
		if ( pFindGuild )
			lua_pushnumber( L, pFindGuild->GetID() );
		else
			lua_pushnumber( L, 0 );

		return 1;
	}

	int _get_guild_privilege(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L,0);
			return 1;
		}
		int guild = (int)lua_tonumber(L,1);
		int type = (int)lua_tonumber(L,2);
		int value = CPrivManager::instance().GetPrivByGuild(guild,type);
		lua_pushnumber(L, value);
		return 1;
	}

	int _item_name(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (lua_isnumber(L, 1))
		{
			DWORD dwVnum = (DWORD)lua_tonumber(L, 1);
			auto pTable = ITEM_MANAGER::instance().GetTable(dwVnum);
			if (pTable)
			{
				if (!lua_isboolean(L, 2) || lua_toboolean(L, 2))
					lua_pushstring(L, CLocaleManager::instance().StringToArgument(pTable->locale_name(pkChr ? pkChr->GetLanguageID() : LANGUAGE_DEFAULT).c_str()));
				else
					lua_pushstring(L, pTable->locale_name(pkChr ? pkChr->GetLanguageID() : LANGUAGE_DEFAULT).c_str());
			}
			else
				lua_pushstring(L, "");
		}
		else
			lua_pushstring(L, "");
		return 1;
	}

	int _mob_name(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (lua_isnumber(L, 1))
		{
			DWORD dwVnum = (DWORD)lua_tonumber(L, 1);
			const CMob * pkMob = CMobManager::instance().Get(dwVnum);

			if (pkMob)
				lua_pushstring(L, CLocaleManager::instance().StringToArgument(pkMob->m_table.locale_name(pkChr ? pkChr->GetLanguageID() : LANGUAGE_DEFAULT).c_str()));
			else
				lua_pushstring(L, "");
		}
		else
			lua_pushstring(L, "");

		return 1;
	}

	int _mob_vnum(lua_State* L)
	{
		if (lua_isstring(L,1))
		{
			const char* str = lua_tostring(L, 1);
			const CMob* pkMob = CMobManager::instance().Get(str, false);
			if (pkMob)
				lua_pushnumber(L,pkMob->m_table.vnum());
			else
				lua_pushnumber(L,0);
		}
		else
			lua_pushnumber(L,0);

		return 1;
	}

	int _get_global_time(lua_State* L)
	{
		lua_pushnumber(L, get_global_time());
		return 1;
	}
	

	int _get_channel_id(lua_State* L)
	{
		lua_pushnumber(L, g_bChannel);

		return 1;
	}

	int _do_command(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		const char * str = lua_tostring(L, 1);
		size_t len = strlen(str);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		::interpret_command(ch, str, len);
		return 0;
	}

	int _find_pc(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		const char * name = lua_tostring(L, 1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);
		if(tch)
			lua_pushnumber(L, tch->GetVID());
		else
		{
			std::string str = name;		
			if (str.length() > 2)
				str = str.substr(1,str.length()-2);
			tch = CHARACTER_MANAGER::instance().FindPC(str.c_str());
			lua_pushnumber(L, tch ? tch->GetVID() : 0);
		}
		return 1;
	}

	int _find_pc_cond(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		int iMinLev = (int) lua_tonumber(L, 1);
		int iMaxLev = (int) lua_tonumber(L, 2);
		unsigned int uiJobFlag = (unsigned int) lua_tonumber(L, 3);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER tch;

		if (test_server)
		{
			sys_log(0, "find_pc_cond map=%d, job=%d, level=%d~%d",
					ch->GetMapIndex(),
					uiJobFlag,
					iMinLev, iMaxLev);
		}

		tch = CHARACTER_MANAGER::instance().FindSpecifyPC(uiJobFlag,
				ch->GetMapIndex(),
				ch,
				iMinLev,
				iMaxLev);

		lua_pushnumber(L, tch ? tch->GetVID() : 0);
		return 1;
	}

	int _find_npc_by_vnum(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		LPCHARACTER pkChrTemp = NULL;

		DWORD race = (DWORD) lua_tonumber(L, 1);

		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(race, i))
		{
			CharacterVectorInteractor::iterator it = i.begin();

			while (it != i.end())
			{
				LPCHARACTER tch = *(it++);

				if (tch->GetMapIndex() == CQuestManager::instance().GetCurrentCharacterPtr()->GetMapIndex())
				{
					if (tch->GetEmpire() == CQuestManager::instance().GetCurrentCharacterPtr()->GetEmpire())
					{
						lua_pushnumber(L, tch->GetVID());
						return 1;
					}
					else if (!pkChrTemp)
					{
						pkChrTemp = tch;
					}
				}
			}
		}

		lua_pushnumber(L, pkChrTemp ? pkChrTemp->GetVID() : 0);
		return 1;
	}

	// »õ·Î¿î state¸¦ ¸¸µç´Ù.
	int _set_quest_state(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		QuestState * pqs = q.GetCurrentState();
		PC* pPC = q.GetCurrentPC();
		//assert(L == pqs->co);
		if (L!=pqs->co) 
		{
			luaL_error(L, "running thread != current thread???");
			sys_log(0,"running thread != current thread???");
			return -1;
		}
		if (pPC)
		{
			//const char* szQuestName = lua_tostring(L, 1);
			//const char* szStateName = lua_tostring(L, 2);
			const string stQuestName(lua_tostring(L, 1));
			const string stStateName(lua_tostring(L, 2));
			if ( test_server )
				sys_log(0,"set_state %s %s ", stQuestName.c_str(), stStateName.c_str() );
			if (pPC->GetCurrentQuestName() == stQuestName)
			{
				pqs->st = q.GetQuestStateIndex(pPC->GetCurrentQuestName(), lua_tostring(L, -1));
				pPC->SetCurrentQuestStateName(lua_tostring(L,-1));
			}
			else
			{
				if (q.GetQuestIndexByName(stQuestName) == 0)
				{
					sys_err("questname %s not found (try to set state %s)", stQuestName.c_str(), stStateName.c_str());
					return 0;
				}

				pPC->SetQuestState(stQuestName, stStateName);
			}
		}
		return 0;
	}

	int _get_quest_state(lua_State* L)
	{
		if (!lua_isstring(L, 1) )
			return 0;

		CQuestManager& q = CQuestManager::instance();
		PC* pPC = q.GetCurrentPC();
		
		if (pPC)
		{
			std::string stQuestName	= lua_tostring(L, 1);
			stQuestName += ".__status";

			int nRet = pPC->GetFlag( stQuestName.c_str() ); 

			lua_pushnumber(L, nRet );

			if ( test_server )
				sys_log(0,"Get_quest_state name %s value %d", stQuestName.c_str(), nRet );
		}
		else
		{
			if ( test_server )
				sys_log(0,"PC == 0 ");

			lua_pushnumber(L, 0);
		}
		return 1;
	}

	int _get_quest_state_name(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		PC* pPC = q.GetCurrentPC();

		if (pPC)
		{
			std::string stQuestName = lua_tostring(L, 1);

			int nRet = pPC->GetFlag((stQuestName + ".__status").c_str());
			const char* c_pszStateName = q.GetQuestStateName(stQuestName, nRet);

			lua_pushstring(L, c_pszStateName);

			if (test_server)
				sys_log(0, "Get_quest_state name %s value %d name %s", stQuestName.c_str(), nRet, c_pszStateName);
		}
		else
		{
			if (test_server)
				sys_log(0, "PC == 0 ");

			lua_pushstring(L, "");
		}
		return 1;
	}

	int _under_han(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			lua_pushboolean(L, 0);
		else
			lua_pushboolean(L, under_han(lua_tostring(L, 1)));
		return 1;
	}

	int _notice_all( lua_State* L )
	{
		ostringstream s;
		combine_lua_string(L, s);

		network::GGOutputPacket<network::GGNoticePacket> p;
		p->set_lang_id(-1);
		p->set_message(s.str());

		P2P_MANAGER::instance().Send(p); // HEADER_GG_NOTICE

		SendNotice(s.str().c_str());
		return 1;	
	}

	EVENTINFO(warp_all_to_village_event_info)
	{
		DWORD dwWarpMapIndex;

		warp_all_to_village_event_info() 
		: dwWarpMapIndex( 0 )
		{
		}
	};

	struct FWarpAllToVillage
	{
		FWarpAllToVillage() {};
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
				if (ch->IsPC())
				{
					BYTE bEmpire =  ch->GetEmpire();
					if ( bEmpire == 0 )
					{
						sys_err( "Unkonwn Empire %s %d ", ch->GetName(), ch->GetPlayerID() );
						return;
					}

					ch->WarpSet( EMPIRE_START_X(bEmpire), EMPIRE_START_Y(bEmpire) );
				}
			}
		}
	};

	EVENTFUNC(warp_all_to_village_event)
	{
		warp_all_to_village_event_info * info = dynamic_cast<warp_all_to_village_event_info *>(event->info);

		if ( info == NULL )
		{
			sys_err( "warp_all_to_village_event> <Factor> Null pointer" );
			return 0;
		}

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap( info->dwWarpMapIndex );

		if (NULL != pSecMap)
		{
			FWarpAllToVillage f;
			pSecMap->for_each( f );
		}

		return 0;
	}

	int _warp_all_to_village( lua_State * L )
	{
		int iMapIndex 	= static_cast<int>(lua_tonumber(L, 1));
		int iSec		= static_cast<int>(lua_tonumber(L, 2));
		
		warp_all_to_village_event_info* info = AllocEventInfo<warp_all_to_village_event_info>();

		info->dwWarpMapIndex = iMapIndex;

		event_create(warp_all_to_village_event, info, PASSES_PER_SEC(iSec));

		SendNoticeMap("Àá½ÃÈÄ ¸ðµÎ ¸¶À»·Î ÀÌµ¿µË´Ï´Ù.", iMapIndex, false);

		return 0;
	}

	int _warp_to_village( lua_State * L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
	
		if (NULL != ch)
		{
			if (test_server)
				sys_err("ch->warp_to_village : %s", ch->GetName());
			BYTE bEmpire = ch->GetEmpire();	
			ch->WarpSet( EMPIRE_START_X(bEmpire), EMPIRE_START_Y(bEmpire) );
		}

		return 0;
	}

	int _say_in_map( lua_State * L )
	{
		int iMapIndex 		= static_cast<int>(lua_tonumber( L, 1 ));
		std::string Script(lua_tostring( L, 2 ));

		Script += "{ENTER}";
		Script += "{DONE}";
		
		network::GCOutputPacket<network::GCScriptPacket> packet_script;

		packet_script->set_skin(CQuestManager::QUEST_SKIN_NORMAL);
		packet_script->set_script(Script);

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap( iMapIndex );

		if ( pSecMap )
		{
			pSecMap->for_each([&packet_script](LPENTITY ent) {
				if (ent->IsType(ENTITY_CHARACTER))
				{
					LPCHARACTER ch = (LPCHARACTER)ent;
					if (ch->GetDesc())
						ch->GetDesc()->Packet(packet_script);
				}
			});
		}

		return 0;
	}

	struct FKillSectree2
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (!ch->IsPC())
					ch->Dead();
			}
		}
	};

	int _kill_all_in_map ( lua_State * L )
	{
		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap( lua_tonumber(L,1) );

		if (NULL != pSecMap)
		{
			FKillSectree2 f;
			pSecMap->for_each( f );
		}

		return 0;
	}

	//ÁÖÀÇ: ¸÷ ¸®Á¨ÀÌ ¾ÈµÇ´Â ¸Ê¿¡¼­¸¸ »ç¿ë
	int _regen_in_map( lua_State * L )
	{
		int iMapIndex = static_cast<int>(lua_tonumber(L, 1));
		std::string szFilename(lua_tostring(L, 2));

		LPSECTREE_MAP pkMap = SECTREE_MANAGER::instance().GetMap(iMapIndex);

		if (pkMap != NULL)
		{
			regen_load_in_file( szFilename.c_str(), iMapIndex, pkMap->m_setting.iBaseX ,pkMap->m_setting.iBaseY );
		}

		return 0;
	}

	int _enable_over9refine( lua_State* L )
	{
		if ( lua_isnumber(L, 1) == true && lua_isnumber(L, 2) == true )
		{
			DWORD dwVnumFrom = (DWORD)lua_tonumber(L, 1);
			DWORD dwVnumTo = (DWORD)lua_tonumber(L, 2);

			COver9RefineManager::instance().enableOver9Refine(dwVnumFrom, dwVnumTo);
		}

		return 0;
	}

	int _add_ox_quiz(lua_State* L)
	{
		int level = (int)lua_tonumber(L, 1);
		const char* quiz_langs[LANGUAGE_MAX_NUM];
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			quiz_langs[i] = lua_tostring(L, 2 + i);
		bool answer = lua_toboolean(L, 2 + LANGUAGE_MAX_NUM);

		if (COXEventManager::instance().AddQuiz(level, quiz_langs, answer) == false)
		{
			LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();
			if (pChar)
				pChar->ChatPacket(CHAT_TYPE_INFO, "Cannot add quiz. %d [de: %s] == %d", level, lua_tostring(L, 3), answer);
			sys_err("Cannot add quiz. %d [de: %s] %d", level, quiz_langs[LANGUAGE_GERMAN], answer);
		}

		return 1;
	}

	EVENTFUNC(warp_all_to_map_my_empire_event)
	{
		warp_all_to_map_my_empire_event_info * info = dynamic_cast<warp_all_to_map_my_empire_event_info *>(event->info);

		if ( info == NULL )
		{
			sys_err( "warp_all_to_map_my_empire_event> <Factor> Null pointer" );
			return 0;
		}

		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap( info->m_lMapIndexFrom );

		if (pSecMap)
		{
			FWarpEmpire f;

			f.m_lMapIndexTo = info->m_lMapIndexTo;
			f.m_x			= info->m_x;
			f.m_y			= info->m_y;
			f.m_bEmpire		= info->m_bEmpire;

			pSecMap->for_each(f);
		}

		return 0;
	}

	int _block_chat(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if (pChar != NULL)
		{
			if (lua_isstring(L, 1) != true && lua_isstring(L, 2) != true)
			{
				lua_pushboolean(L, false);
				return 1;
			}

			std::string strName(lua_tostring(L, 1));
			std::string strTime(lua_tostring(L, 2));

			std::string strArg = strName + " " + strTime;

			do_block_chat(pChar, const_cast<char*>(strArg.c_str()), 0, 0);

			lua_pushboolean(L, true);
			return 1;
		}

		lua_pushboolean(L, false);
		return 1;
	}

	int _spawn_mob(lua_State* L)
	{
		if( false == lua_isnumber(L, 1) || false == lua_isnumber(L, 2) || false == lua_isboolean(L, 3) )
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		const DWORD dwVnum = static_cast<DWORD>(lua_tonumber(L, 1));
		const size_t count = MINMAX(1, static_cast<size_t>(lua_tonumber(L, 2)), 10);
		const bool isAggresive = static_cast<bool>(lua_toboolean(L, 3));
		const long lMapIndex = lua_isnumber(L, 4) ? lua_tonumber(L, 4) : 0;
		size_t SpawnCount = 0;

		const CMob* pMonster = CMobManager::instance().Get( dwVnum );

		if( NULL != pMonster )
		{
			const LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

			for( size_t i=0 ; i < count ; ++i )
			{
				const LPCHARACTER pSpawnMonster = CHARACTER_MANAGER::instance().SpawnMobRange( dwVnum,
						lMapIndex ? lMapIndex : pChar->GetMapIndex(),
						pChar->GetX() - random_number(200, 750),
						pChar->GetY() - random_number(200, 750),
						pChar->GetX() + random_number(200, 750),
						pChar->GetY() + random_number(200, 750),
						true,
						pMonster->m_table.type() == CHAR_TYPE_STONE,
						isAggresive );

				if( NULL != pSpawnMonster )
				{
					++SpawnCount;
				}
			}

			sys_log(0, "QUEST Spawn Monstster: VNUM(%u) COUNT(%u) isAggresive(%b)", dwVnum, SpawnCount, isAggresive);
		}

		lua_pushnumber(L, SpawnCount);

		return 1;
	}

	int _notice_in_map( lua_State* L )
	{
		const LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			SendNoticeMap( lua_tostring(L,1), pChar->GetMapIndex(), lua_toboolean(L,2) );
		}

		return 0;
	}

	int _get_locale_base_path( lua_State* L )
	{
		lua_pushstring( L, Locale_GetBasePath().c_str() );

		return 1;
	}

	struct FPurgeArea
	{
		int x1, y1, x2, y2;
		LPCHARACTER ExceptChar;

		FPurgeArea(int a, int b, int c, int d, LPCHARACTER p)
			: x1(a), y1(b), x2(c), y2(d),
			ExceptChar(p)
		{}

		void operator () (LPENTITY ent)
		{
			if (true == ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

				if (pChar == ExceptChar)
					return;
					
				if (pChar->IsPurgeable() && (true == pChar->IsMonster() || true == pChar->IsStone()))
				{
					if (x1 <= pChar->GetX() && pChar->GetX() <= x2 && y1 <= pChar->GetY() && pChar->GetY() <= y2)
					{
						M2_DESTROY_CHARACTER(pChar);
					}
				}
			}
		}
	};
#ifdef ENABLE_ZODIAC_TEMPLE
	struct FPurgeAreaZodiac
	{
		int x1, y1, x2, y2;
		LPCHARACTER ExceptChar;

		FPurgeAreaZodiac(int a, int b, int c, int d, LPCHARACTER p)
			: x1(a), y1(b), x2(c), y2(d),
			ExceptChar(p)
		{}

		void operator () (LPENTITY ent)
		{
			if (true == ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

				if (pChar == ExceptChar)
					return;

				if (!pChar->IsPet() && (true == pChar->IsMonster() || true == pChar->IsStone() || true == pChar->IsNPC()))
				{
						M2_DESTROY_CHARACTER(pChar);
				}
			}
		}
	};
#endif
	int _purge_area( lua_State* L )
	{
		int x1 = lua_tonumber(L, 1);
		int y1 = lua_tonumber(L, 2);
		int x2 = lua_tonumber(L, 3);
		int y2 = lua_tonumber(L, 4);

		const int mapIndex = SECTREE_MANAGER::instance().GetMapIndex( x1, y1 );

		if (0 == mapIndex)
		{
			sys_err("_purge_area: cannot get a map index with (%u, %u)", x1, y1);
			return 0;
		}

		LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(mapIndex);

		if (NULL != pSectree)
		{
			FPurgeArea func(x1, y1, x2, y2, CQuestManager::instance().GetCurrentNPCCharacterPtr());

			pSectree->for_each(func);
		}

		return 0;
	}
#ifdef ENABLE_ZODIAC_TEMPLE
	int _purge_area_zodiac( lua_State* L )
	{
		int x1 = lua_tonumber(L, 1);
		int y1 = lua_tonumber(L, 2);
		int x2 = lua_tonumber(L, 3);
		int y2 = lua_tonumber(L, 4);

		const int mapIndex = SECTREE_MANAGER::instance().GetMapIndex( x1, y1 );

		if (0 == mapIndex)
		{
			sys_err("_purge_area_zodiac: cannot get a map index with (%u, %u)", x1, y1);
			return 0;
		}

		LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(mapIndex);

		if (NULL != pSectree)
		{
			FPurgeAreaZodiac func(x1, y1, x2, y2, CQuestManager::instance().GetCurrentNPCCharacterPtr());

			pSectree->for_each(func);
		}

		return 0;
	}
#endif
	struct FWarpAllInAreaToArea
	{
		int from_x1, from_y1, from_x2, from_y2;
		int to_x1, to_y1, to_x2, to_y2;
		size_t warpCount;

		FWarpAllInAreaToArea(int a, int b, int c, int d, int e, int f, int g, int h)
			: from_x1(a), from_y1(b), from_x2(c), from_y2(d),
			to_x1(e), to_y1(f), to_x2(g), to_y2(h),
			warpCount(0)
		{}

		void operator () (LPENTITY ent)
		{
			if (true == ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

				if (true == pChar->IsPC())
				{
					if (from_x1 <= pChar->GetX() && pChar->GetX() <= from_x2 && from_y1 <= pChar->GetY() && pChar->GetY() <= from_y2)
					{
						++warpCount;

						pChar->WarpSet( random_number(to_x1, to_x2), random_number(to_y1, to_y2) );
					}
				}
			}
		}
	};

	int _warp_all_in_area_to_area( lua_State* L )
	{
		int from_x1 = lua_tonumber(L, 1);
		int from_y1 = lua_tonumber(L, 2);
		int from_x2 = lua_tonumber(L, 3);
		int from_y2 = lua_tonumber(L, 4);

		int to_x1 = lua_tonumber(L, 5);
		int to_y1 = lua_tonumber(L, 6);
		int to_x2 = lua_tonumber(L, 7);
		int to_y2 = lua_tonumber(L, 8);

		const int mapIndex = SECTREE_MANAGER::instance().GetMapIndex( from_x1, from_y1 );

		if (0 == mapIndex)
		{
			sys_err("_warp_all_in_area_to_area: cannot get a map index with (%u, %u)", from_x1, from_y1);
			lua_pushnumber(L, 0);
			return 1;
		}

		LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(mapIndex);

		if (NULL != pSectree)
		{
			FWarpAllInAreaToArea func(from_x1, from_y1, from_x2, from_y2, to_x1, to_y1, to_x2, to_y2);

			pSectree->for_each(func);

			lua_pushnumber(L, func.warpCount);
			sys_log(0, "_warp_all_in_area_to_area: %u character warp", func.warpCount);
			return 1;
		}
		else
		{
			lua_pushnumber(L, 0);
			sys_err("_warp_all_in_area_to_area: no sectree");
			return 1;
		}
	}

	int _get_special_item_group( lua_State* L )
	{
		if (!lua_isnumber (L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup((DWORD)lua_tonumber(L, 1));

		if (!pItemGroup)
		{
			sys_err("cannot find special item group %d", (DWORD)lua_tonumber(L, 1));
			return 0;
		}

		int count = pItemGroup->GetGroupSize();
		
		for (int i = 0; i < count; i++)
		{
			lua_pushnumber(L, (int)pItemGroup->GetVnum(i));
			lua_pushnumber(L, (int)pItemGroup->GetCount(i));
		}

		return count*2;
	}

	int _mysql_query_async(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			sys_err("unkown argument (expected query)");
			lua_pushboolean(L, 0);
			return 1;
		}

		const char * szQuery = lua_tostring(L, 1);
		sys_log(0, "QUEST_QUERY: %s", szQuery);

		DBManager::instance().Query(szQuery);
	
		return 0;
	}

	int _mysql_query(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			sys_err("unkown argument (expected query)");
			lua_pushboolean(L, 0);
			return 1;
		}

		const char * szQuery = lua_tostring(L, 1);
		sys_log(0, "QUEST_QUERY: %s", szQuery);

		SQLMsg * msg = DBManager::instance().DirectQuery(szQuery);
		if (!msg)
		{
			sys_err("query failed: %s", szQuery);
			lua_pushboolean(L, 0);
			return 1;
		}
		std::auto_ptr<SQLMsg> pmsg(msg);

		int i = 0;
		int num;

		if (pmsg->Get()->uiNumRows > 0)
		{
			lua_newtable(L);
			while (MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult)) {
				while (MYSQL_FIELD * field = mysql_fetch_field(pmsg->Get()->pSQLResult)) {
					lua_pushstring(L, field->name);
					if (str_is_number(row[i]) && str_to_number(num, row[i]))
						lua_pushnumber(L, num * 1.0);
					else
						lua_pushstring(L, row[i]);
					lua_rawset(L, -3);
					i++;
				}
			}

			lua_pushboolean(L, 1);
		}
		else
		{
			lua_pushboolean(L, pmsg->Get()->uiAffectedRows > 0);
		}

		return 1;
	}

#ifdef _MSC_VER
#define INFINITY (DBL_MAX+DBL_MAX)
#define NAN (INFINITY-INFINITY)
#endif
	int _mysql_direct_query(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		int i = 0, m = 1;
		MYSQL_ROW row;
		MYSQL_FIELD * field;
		MYSQL_RES * result;

		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("%s", lua_tostring(L, 1)));
		if (pMsg.get())
		{
			// ret1 (number of affected rows)
			lua_pushnumber(L, pMsg->Get()->uiAffectedRows);
			//-1 if error such as duplicate occurs (-2147483648 via lua)
			//   if wrong syntax error occurs (4294967295 via lua)
			// ret2 (table of affected rows)
			lua_newtable(L);
			if ((result = pMsg->Get()->pSQLResult) &&
				!(pMsg->Get()->uiAffectedRows == 0 || pMsg->Get()->uiAffectedRows == (uint32_t)-1))
			{

				while ((row = mysql_fetch_row(result)))
				{
					lua_pushnumber(L, m);
					lua_newtable(L);
					int j = 1;
					while ((field = mysql_fetch_field(result)))
					{
						if (!lua_isboolean(L, 2) || !lua_toboolean(L, 1))
							lua_pushstring(L, field->name);
						else // Pass True for Numeric Table Format = { [1] = { [1] = row[0], [2] = row[1] } }
							lua_pushnumber(L, j++);
						
						if (!(field->flags & NOT_NULL_FLAG) && (row[i] == NULL))
						{
							// lua_pushstring(L, "NULL");
							lua_pushnil(L);
						}
						else if (IS_NUM(field->type))
						{
							double val = NAN;
							lua_pushnumber(L, (sscanf(row[i], "%lf", &val) == 1) ? val : NAN);
						}
						else if (field->type == MYSQL_TYPE_BLOB)
						{
							lua_newtable(L);
							for (DWORD iBlob = 0; iBlob < field->max_length; iBlob++)
							{
								lua_pushnumber(L, row[i][iBlob]);
								lua_rawseti(L, -2, iBlob + 1);
							}
						}
						else
							lua_pushstring(L, row[i]);

						lua_rawset(L, -3);
						i++;
					}
					mysql_field_seek(result, 0);
					i = 0;

					lua_rawset(L, -3);
					m++;
				}
			}
		}
		else { lua_pushnumber(L, 0); lua_newtable(L); }

		return 2;
	}

	int _check_name(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		bool bAllowSpecialChars = lua_isboolean(L, 2) && lua_toboolean(L, 2);

		const char* szName = lua_tostring(L, 1);
		lua_pushboolean(L, check_name(szName, bAllowSpecialChars) && strlen(szName) <= CHARACTER_NAME_MAX_LEN);
		return 1;
	}

	int _locale_text(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			lua_pushstring(L, "ERROR!");
			return 1;
		}

		const char* szText = lua_tostring(L, 1);

		int iLangID = 0;
		if (lua_isnumber(L, 2))
			iLangID = lua_tonumber(L, 2);
		else
			iLangID = CQuestManager::instance().GetCurrentCharacterPtr()->GetLanguageID();

		lua_pushstring(L, LC_QUEST_TEXT(iLangID, szText));
		return 1;
	}

	int _locale_text_game(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			lua_pushstring(L, "ERROR!");
			return 1;
		}

		const char* szText = lua_tostring(L, 1);

		int iLangID = 0;
		if (lua_isnumber(L, 2))
			iLangID = lua_tonumber(L, 2);
		else
			iLangID = CQuestManager::instance().GetCurrentCharacterPtr()->GetLanguageID();

		lua_pushstring(L, LC_TEXT(iLangID, szText));
		return 1;
	}

	int _get_skill_name(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushstring(L, "ERROR!");
			return 1;
		}

		int iLanguageIndex;
		if (lua_isnumber(L, 2))
			iLanguageIndex = lua_tonumber(L, 2);
		else
			iLanguageIndex = CQuestManager::instance().GetCurrentCharacterPtr()->GetLanguageID();

		int iSkillIndex = lua_tonumber(L, 1);
		CSkillProto* pSkillProto = CSkillManager::instance().Get(iSkillIndex);

		if (pSkillProto)
			lua_pushstring(L, CLocaleManager::instance().StringToArgument(pSkillProto->szLocaleName[iLanguageIndex]));
		else
			lua_pushstring(L, "");
		return 1;
	}

	int _is_vid_alive(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER pkChr = CHARACTER_MANAGER::instance().Find(lua_tonumber(L, 1));
		lua_pushboolean(L, pkChr && !pkChr->IsDead());

		return 1;
	}

	int _get_polar_x(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, 0);
			return 1;
		}
		lua_pushnumber(L, lua_tonumber(L, 1) * cos(lua_tonumber(L, 2)));
		return 1;
	}

	int _get_polar_y(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, 0);
			return 1;
		}
		lua_pushnumber(L, lua_tonumber(L, 1) * sin(lua_tonumber(L, 2)));
		return 1;
	}

	int _get_base_coord(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		int iMapIndex = lua_tonumber(L, 1);

		const TMapRegion* pMapRegion = SECTREE_MANAGER::instance().GetMapRegion(iMapIndex);
		if (!pMapRegion)
		{
			lua_pushnumber(L, 0);
			lua_pushnumber(L, 0);
			return 2;
		}

		lua_pushnumber(L, pMapRegion->sx);
		lua_pushnumber(L, pMapRegion->sy);
		return 2;
	}

	int _get_town_coord(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		int iMapIndex = lua_tonumber(L, 1);
		int iEmpire = 1;
		if (lua_isnumber(L, 2))
			iEmpire = lua_tonumber(L, 2);

		if (iEmpire < 1 || iEmpire > EMPIRE_MAX_NUM)
			return 0;

		const TMapRegion* pMapRegion = SECTREE_MANAGER::instance().GetMapRegion(iMapIndex);
		if (!pMapRegion)
		{
			lua_pushnumber(L, 0);
			lua_pushnumber(L, 0);
			return 2;
		}

		if (pMapRegion->bEmpireSpawnDifferent)
		{
			lua_pushnumber(L, pMapRegion->posEmpire[iEmpire - 1].x);
			lua_pushnumber(L, pMapRegion->posEmpire[iEmpire - 1].y);
		}
		else
		{
			lua_pushnumber(L, pMapRegion->posSpawn.x);
			lua_pushnumber(L, pMapRegion->posSpawn.y);
		}
		return 2;
	}

	int _get_map_base_path(lua_State* L)
	{
		std::string stPath = Locale_GetMapPath() + "/";

		if (lua_isnumber(L, 1))
		{
			int lMapIndex = lua_tonumber(L, 1);
			if (lMapIndex >= 10000)
				lMapIndex /= 10000;

			const TMapRegion* pkRegion = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex);
			if (pkRegion)
				stPath = stPath + pkRegion->strMapName + "/";
		}

		lua_pushstring(L, stPath.c_str());
		return 1;
	}

	int _find_item_vnum(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		const char* szInput = lua_tostring(L, 1);
		DWORD dwItemVnum = 0;

		if (str_is_number(szInput))
			str_to_number(dwItemVnum, szInput);
		else
		{
			if (!ITEM_MANAGER::instance().GetVnum(szInput, dwItemVnum, ch ? ch->GetLanguageID() : -1))
				dwItemVnum = 0;
		}

		if (!dwItemVnum || !ITEM_MANAGER::instance().GetTable(dwItemVnum))
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, dwItemVnum);
		return 1;
	}

#ifdef ENABLE_BLOCK_PKMODE
	struct FSetPKModeInMap
	{
		BYTE pkMode;
		bool mode;

		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER)ent;

				if (ch->IsPC())
					ch->SetBlockPKMode(pkMode, mode);
			}
		}
	};

	int _set_pkmode_in_map(lua_State* L)
	{
		int mapIndex = lua_tonumber(L, 1);
		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(mapIndex);

		FSetPKModeInMap f;
		f.pkMode = lua_tonumber(L, 2);
		f.mode = lua_toboolean(L, 3);

		if (pSecMap)
			pSecMap->for_each(f);

		return 0;
	}
#endif

	void RegisterGlobalFunctionTable(lua_State* L)
	{
		extern int quest_setstate(lua_State* L);

		luaL_reg global_functions[] =
		{
			{	"sys_err",					_syserr					},
			{	"sys_log",					_syslog					},
			{	"char_log",					_char_log				},
			{	"item_log",					_item_log				},
			{	"set_state",				quest_setstate			},
			{	"set_skin",					_set_skin				},
			{	"setskin",					_set_skin				},
			{	"time_to_str",				_time_to_str			},
			{	"say",						_say					},
			{	"chat",						_chat					},
			{	"cmdchat",					_cmdchat				},
			{	"syschat",					_syschat				},
			{	"get_locale",				_get_locale				},
			{	"setleftimage",				_left_image				},
			{	"settopimage",				_top_image				},
			{	"server_timer",				_set_server_timer		},
			{	"clear_server_timer",		_clear_server_timer		},
			{	"server_loop_timer",		_set_server_loop_timer	},
			{	"get_server_timer_arg",		_get_server_timer_arg	},
			{	"timer",					_timer					},
			{	"loop_timer",				_set_named_loop_timer	},
			{	"cleartimer",				_clear_named_timer		},
			{	"getnpcid",					_getnpcid				},
			{	"is_test_server",			_is_test_server			},
			{	"is_pvp_server",			_is_pvp_server			},
			{	"raw_script",				_raw_script				},
			{	"number",					_number	   				},

			{	"add_bgm_info",				_add_bgm_info			},
			{	"add_goto_info",			_add_goto_info			},
			{	"add_warp_levellimit",		_add_warp_levellimit	},
			{	"__refine_pick",			_refine_pick			},

			{	"add_ox_quiz",					_add_ox_quiz					},
			{	"__fish_real_refine_rod",		_fish_real_refine_rod			}, // XXX
			{	"__give_char_priv",				_give_char_privilege			},
			{	"__give_empire_priv",			_give_empire_privilege			},
			{	"__give_guild_priv",			_give_guild_privilege			},
			{	"__get_empire_priv_string",		_get_empire_privilege_string	},
			{	"__get_empire_priv",			_get_empire_privilege			},
			{	"__get_guild_priv_string",		_get_guild_privilege_string		},
			{	"__get_guildid_byname",			_get_guildid_byname				},
			{	"__get_guild_priv",				_get_guild_privilege			},
			{	"item_name",					_item_name						},
			{	"mob_name",						_mob_name						},
			{	"mob_vnum",						_mob_vnum						},
			{	"get_time",						_get_global_time				},
			{	"get_global_time",				_get_global_time				},
			{	"get_channel_id",				_get_channel_id					},
			{	"command",						_do_command						},
			{	"find_pc_cond",					_find_pc_cond					},
			{	"find_pc_by_name",				_find_pc						},
			{	"find_npc_by_vnum",				_find_npc_by_vnum				},
			{	"set_quest_state",				_set_quest_state				},
			{	"get_quest_state",				_get_quest_state				},
			{	"get_quest_state_name",			_get_quest_state_name			},
			{	"under_han",					_under_han						},
			{	"notice",						_notice							},
			{	"notice_all",					_notice_all						},
			{	"notice_in_map",				_notice_in_map					},
			{	"warp_all_to_village",			_warp_all_to_village			},
			{	"warp_to_village",				_warp_to_village				},	
			{	"say_in_map",					_say_in_map						},	
			{	"kill_all_in_map",				_kill_all_in_map				},
			{	"regen_in_map",					_regen_in_map					},
			{	"enable_over9refine",			_enable_over9refine				},
			{	"block_chat",					_block_chat						},
			{	"spawn_mob",					_spawn_mob						},
			{	"get_locale_base_path",			_get_locale_base_path			},
			{	"purge_area",					_purge_area						},
#ifdef ENABLE_ZODIAC_TEMPLE
			{	"purge_area_zodiac",					_purge_area_zodiac		},
#endif
			{	"warp_all_in_area_to_area",		_warp_all_in_area_to_area		},
			{	"get_special_item_group",		_get_special_item_group			},

			{	"mysql_query_async",			_mysql_query_async				},
			{	"mysql_query",					_mysql_query					},
			{	"mysql_direct_query",			_mysql_direct_query				},
			{	"check_name",					_check_name						},
			{	"locale_text",					_locale_text					},
			{	"get_skill_name",				_get_skill_name					},

			{	"is_vid_alive",					_is_vid_alive					},
			{	"get_polar_x",					_get_polar_x					},
			{	"get_polar_y",					_get_polar_y					},

			{	"get_base_coord",				_get_base_coord					},
			{	"get_town_coord",				_get_town_coord					},

			{	"get_map_base_path",			_get_map_base_path				},
			{	"tchat",						_tchat							},

			{	"find_item_vnum",				_find_item_vnum					},

#ifdef ENABLE_BLOCK_PKMODE
			{	"set_pkmode_in_map",			_set_pkmode_in_map				},
#endif

			{	NULL,	NULL	}
		};
	
		int i = 0;

		while (global_functions[i].name != NULL)
		{
			lua_register(L, global_functions[i].name, global_functions[i].func);
			++i;
		}
	}
}

