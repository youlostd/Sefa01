﻿
#include "stdafx.h"

#include <sstream>

#include "questmanager.h"
#include "questlua.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "buffer_manager.h"
#include "db.h"
#include "xmas_event.h"
#include "regen.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "sectree_manager.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest
{
	using namespace std;

	string ScriptToString(const string& str)
	{
		lua_State* L = CQuestManager::instance().GetLuaState();
		int x = lua_gettop(L);

		int errcode = lua_dobuffer(L, ("return "+str).c_str(), str.size()+7, "ScriptToString");
		string retstr;
		if (!errcode)
		{
			if (lua_isstring(L,-1))
				retstr = lua_tostring(L, -1);
		}
		else
		{
			sys_err("LUA ScriptRunError (code:%d src:[%s])", errcode, str.c_str());
		}
		lua_settop(L,x);
		return retstr;
	}

	void FSetWarpLocation::operator() (LPCHARACTER ch)
	{
		if (ch->IsPC())
		{
			ch->SetWarpLocation (map_index, x, y);
		}
	}

	void FSetQuestFlag::operator() (LPCHARACTER ch)
	{
		if (!ch->IsPC())
			return;

		PC * pPC = CQuestManager::instance().GetPCForce(ch->GetPlayerID());

		if (pPC)
			pPC->SetFlag(flagname, value);
	}

	bool FPartyCheckFlagLt::operator() (LPCHARACTER ch)
	{
		if (!ch->IsPC())
			return false;

		PC * pPC = CQuestManager::instance().GetPCForce(ch->GetPlayerID());
		bool returnBool = false;
		if (pPC)
		{
			int flagValue = pPC->GetFlag(flagname);
			if (value > flagValue)
				returnBool = true;
			else
				returnBool = false;
		}

		return returnBool;
	}

	FPartyChat::FPartyChat(int ChatType, const char* str) : iChatType(ChatType), str(str)
	{
	}

	void FPartyChat::operator() (LPCHARACTER ch)
	{
		ch->ChatPacket(iChatType, "%s", str);
	}

	void FPartyClearReady::operator() (LPCHARACTER ch)
	{
		ch->RemoveAffect(AFFECT_DUNGEON_READY);
	}

	void FWarpEmpire::operator() (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC() && ch->GetEmpire() == m_bEmpire)
			{
				ch->WarpSet(m_x, m_y, m_lMapIndexTo);
			}
		}
	}

	FBuildLuaGuildWarList::FBuildLuaGuildWarList(lua_State * lua_state) : L(lua_state), m_count(1)
	{
		lua_newtable(lua_state);
	}

	void FBuildLuaGuildWarList::operator() (DWORD g1, DWORD g2)
	{
		CGuild* g = CGuildManager::instance().FindGuild(g1);

		if (!g)
			return;

		if (g->GetGuildWarType(g2) == GUILD_WAR_TYPE_FIELD)
			return;

		if (g->GetGuildWarState(g2) != GUILD_WAR_ON_WAR)
			return;

		lua_newtable(L);
		lua_pushnumber(L, g1);
		lua_rawseti(L, -2, 1);
		lua_pushnumber(L, g2);
		lua_rawseti(L, -2, 2);
		lua_rawseti(L, -2, m_count++);
	}

	bool IsScriptTrue(const char* code, int size)
	{
		if (size==0)
			return true;

		lua_State* L = CQuestManager::instance().GetLuaState();
		int x = lua_gettop(L);
		int errcode = lua_dobuffer(L, code, size, "IsScriptTrue");
		int bStart = lua_toboolean(L, -1);
		if (errcode)
		{
			char buf[100];
			snprintf(buf, sizeof(buf), "LUA ScriptRunError (code:%%d src:[%%%ds])", size);
			sys_err(buf, errcode, code);
		}
		lua_settop(L,x);
		return bStart != 0;
	}

	void combine_lua_string(lua_State * L, ostringstream & s)
	{
		char buf[32];

		int n = lua_gettop(L);
		int i;

		for (i = 1; i <= n; ++i)
		{
			if (lua_isstring(L,i))
				//printf("%s\n",lua_tostring(L,i));
				s << lua_tostring(L, i);
			else if (lua_isnumber(L, i))
			{
				snprintf(buf, sizeof(buf), "%.14g\n", lua_tonumber(L,i));
				s << buf;
			}
		}
	}

	// 
	// "member" Lua functions
	//
	// int member_select_player_id(lua_State* L)
	// {															//  \|/ not working anyway, i removed it... 
		// LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindByPID(pChar->GetPlayerID());
		// CQuestManager::instance().SetCurrentPartyMember(pChar);
		// return 0;
	// }

	int member_get_player_id(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentPartyMember();
		lua_pushnumber(L, ch ? ch->GetPlayerID() : 0);
		return 1;
	}

	int member_is_me(lua_State* L)
	{
		lua_pushboolean(L, CQuestManager::instance().GetCurrentPartyMember() == CQuestManager::instance().GetCurrentCharacterPtr());
		return 1;
	}

	int member_chat(lua_State* L)
	{
		ostringstream s;
		combine_lua_string(L, s);
		CQuestManager::Instance().GetCurrentPartyMember()->ChatPacket(CHAT_TYPE_TALKING, "%s", s.str().c_str());
		return 0;
	}

	int member_clear_ready(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentPartyMember();
		ch->RemoveAffect(AFFECT_DUNGEON_READY);
		return 0;
	}

	int member_set_ready(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentPartyMember();
		ch->AddAffect(AFFECT_DUNGEON_READY, POINT_NONE, 0, AFF_DUNGEON_READY, 65535, 0, true);
		return 0;
	}

	int mob_spawn(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD mob_vnum = (DWORD)lua_tonumber(L, 1);
		long local_x = (long) lua_tonumber(L, 2)*100;
		long local_y = (long) lua_tonumber(L, 3)*100;
		float radius = (float) lua_tonumber(L, 4)*100;
		bool bAggressive = lua_toboolean(L, 5);
		DWORD count = (lua_isnumber(L, 6))?(DWORD) lua_tonumber(L, 6):1;

		if (count == 0)
			count = 1;
		else if (count > 10)
		{
			sys_err("count bigger than 10");
			count = 10;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
		if (pMap == NULL) {
			return 0;
		}
		DWORD dwQuestIdx = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		bool ret = false;
		LPCHARACTER mob = NULL;

		while (count--)
		{
			for (int loop = 0; loop < 8; ++loop)
			{
				float angle = random_number(0, 999) * M_PI * 2 / 1000;
				float r = random_number(0, 999) * radius / 1000;

				long x = local_x + pMap->m_setting.iBaseX + (long)(r * cos(angle));
				long y = local_y + pMap->m_setting.iBaseY + (long)(r * sin(angle));

				mob = CHARACTER_MANAGER::instance().SpawnMob(mob_vnum, ch->GetMapIndex(), x, y, 0);

				if (mob)
					break;
			}

			if (mob)
			{
				if (bAggressive)
					mob->SetAggressive();

				mob->SetQuestBy(dwQuestIdx);

				if (!ret)
				{
					ret = true;
					lua_pushnumber(L, (DWORD) mob->GetVID());
				}
			}
		}

		if (!ret)
			lua_pushnumber(L, 0);

		return 1;
	}

	int mob_spawn_group(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 6))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		DWORD group_vnum = (DWORD)lua_tonumber(L, 1);
		long local_x = (long) lua_tonumber(L, 2) * 100;
		long local_y = (long) lua_tonumber(L, 3) * 100;
		float radius = (float) lua_tonumber(L, 4) * 100;
		bool bAggressive = lua_toboolean(L, 5);
		DWORD count = (DWORD) lua_tonumber(L, 6);

		if (count == 0)
			count = 1;
		else if (count > 10)
		{
			sys_err("count bigger than 10");
			count = 10;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
		if (pMap == NULL) {
			lua_pushnumber(L, 0);
			return 1;
		}
		DWORD dwQuestIdx = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		bool ret = false;
		LPCHARACTER mob = NULL;

		while (count--)
		{
			for (int loop = 0; loop < 8; ++loop)
			{
				float angle = random_number(0, 999) * M_PI * 2 / 1000;
				float r = random_number(0, 999)*radius/1000;

				long x = local_x + pMap->m_setting.iBaseX + (long)(r * cos(angle));
				long y = local_y + pMap->m_setting.iBaseY + (long)(r * sin(angle));

				mob = CHARACTER_MANAGER::instance().SpawnGroup(group_vnum, ch->GetMapIndex(), x, y, x, y, NULL, bAggressive);

				if (mob)
					break;
			}

			if (mob)
			{
				mob->SetQuestBy(dwQuestIdx);

				if (!ret)
				{
					ret = true;
					lua_pushnumber(L, (DWORD) mob->GetVID());
				}
			}
		}

		if (!ret)
			lua_pushnumber(L, 0);

		return 1;
	}

	//
	// global Lua functions
	//
	//
	// Registers Lua function table
	//
	void CQuestManager::AddLuaFunctionTable(const char * c_pszName, luaL_reg * preg)
	{
		lua_newtable(L);

		while ((preg->name))
		{
			lua_pushstring(L, preg->name);
			lua_pushcfunction(L, preg->func);
			lua_rawset(L, -3);
			preg++;
		}

		lua_setglobal(L, c_pszName);
	}

	void CQuestManager::BuildStateIndexToName(const char* questName)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, questName);

		if (lua_isnil(L,-1))
		{
			sys_err("QUEST wrong quest state file for quest %s",questName);
			lua_settop(L,x);
			return;
		}

		for (lua_pushnil(L); lua_next(L, -2);)
		{
			if (lua_isstring(L, -2) && lua_isnumber(L, -1))
			{
				lua_pushvalue(L, -2);
				lua_rawset(L, -4);
			}
			else
			{
				lua_pop(L, 1);
			}
		}

		lua_settop(L, x);
	}

	/**
	 * @version 05/06/08	Bang2ni - __get_guildid_byname ½ºÅ©¸³Æ® ÇÔ¼ö µî·Ï
	 */
	bool CQuestManager::InitializeLua()
	{
		L = lua_open();

		luaopen_base(L);
		luaopen_table(L);
		luaopen_string(L);
		luaopen_math(L);
		//TEMP
		luaopen_io(L);
		luaopen_debug(L);

		RegisterAffectFunctionTable();
		RegisterBuildingFunctionTable();
		RegisterDungeonFunctionTable();
		RegisterGameFunctionTable();
		RegisterGuildFunctionTable();
		RegisterMountFunctionTable();
#ifdef __PET_SYSTEM__
		RegisterPetFunctionTable();
#endif
		RegisterITEMFunctionTable();
		RegisterMarriageFunctionTable();
		RegisterNPCFunctionTable();
		RegisterPartyFunctionTable();
		RegisterPCFunctionTable();
		RegisterQuestFunctionTable();
		RegisterTargetFunctionTable();
		RegisterArenaFunctionTable();
		RegisterOXEventFunctionTable();
		// RegisterDanceEventFunctionTable();
		// RegisterDragonLairFunctionTable();
		// RegisterSpeedServerFunctionTable();
#ifdef __VOTE4BUFF__
		RegisterV4BFunctionTable();
#endif
#ifdef __DRAGONSOUL__
		RegisterDragonSoulFunctionTable();
#endif
#ifdef __EVENT_MANAGER__
		RegisterEventFunctionTable();
#endif
#ifdef __MELEY_LAIR_DUNGEON__
		RegisterMeleyLairFunctionTable();
#endif

		{
			luaL_reg member_functions[] = 
			{
				{ "chat",			member_chat		},
				{ "set_ready",			member_set_ready	},
				{ "clear_ready",		member_clear_ready	},
				{ NULL,				NULL			}
			};

			AddLuaFunctionTable("member", member_functions);
		}

		{
			luaL_reg mob_functions[] =
			{
				{ "spawn",			mob_spawn		},
				{ "spawn_group",		mob_spawn_group		},
				{ NULL,				NULL			}
			};

			AddLuaFunctionTable("mob", mob_functions);
		}

		//
		// global namespace functions
		//
		RegisterGlobalFunctionTable(L);

		// LUA_INIT_ERROR_MESSAGE
		{
			char settingsFileName[256];
			snprintf(settingsFileName, sizeof(settingsFileName), "%s/settings.lua", Locale_GetQuestPath().c_str());

			int settingsLoadingResult = lua_dofile(L, settingsFileName);
			sys_log(0, "LoadSettings(%s), returns %d", settingsFileName, settingsLoadingResult);
			if (settingsLoadingResult != 0)
			{
				sys_err("LOAD_SETTINS_FAILURE(%s)", settingsFileName);
				return false;
			}
		}

		{
			char questlibFileName[256];
			snprintf(questlibFileName, sizeof(questlibFileName), "%s/lib/questlib.lua", Locale_GetQuestPath().c_str());

			int questlibLoadingResult = lua_dofile(L, questlibFileName);
			sys_log(0, "LoadQuestlib(%s), returns %d", questlibFileName, questlibLoadingResult);
			if (questlibLoadingResult != 0)
			{
				sys_err("LOAD_QUESTLIB_FAILURE(%s)", questlibFileName);
				return false;
			}
		}

		char buf[PATH_MAX];
		snprintf(buf, sizeof(buf), "%s/state/", Locale_GetQuestObjectPath().c_str());
		DIR * pdir = opendir(buf);
		int iQuestIdx = 0;

		if (pdir)
		{
			dirent * pde;

			while ((pde = readdir(pdir)))
			{
				if (pde->d_name[0] == '.')
					continue;

				snprintf(buf + 11, sizeof(buf) - 11, "%s", pde->d_name);

				RegisterQuest(pde->d_name, ++iQuestIdx);
				int ret = lua_dofile(L, (Locale_GetQuestObjectPath() + "/state/" + pde->d_name).c_str());
				sys_log(!test_server, "QUEST: loading %s, returns %d", (Locale_GetQuestObjectPath() + "/state/" + pde->d_name).c_str(), ret);

				BuildStateIndexToName(pde->d_name);
			}

			closedir(pdir);
		}

		lua_setgcthreshold(L, 0);

		lua_newtable(L);
		lua_setglobal(L, "__codecache");
		return true;
	}

	void CQuestManager::GotoSelectState(QuestState& qs)
	{
		lua_checkstack(qs.co, 1);

		//int n = lua_gettop(L);
		int n = luaL_getn(qs.co, -1);
		qs.args = n;
		//cout << "select here (1-" << qs.args << ")" << endl;
		//

		ostringstream os;
		os << "{QUESTION ";

		for (int i=1; i<=n; i++)
		{
			lua_rawgeti(qs.co,-1,i);
			if (lua_isstring(qs.co,-1))
			{
				//printf("%d\t%s\n",i,lua_tostring(qs.co,-1));
				if (i != 1)
					os << "|";
				os << i << ";" << LC_TEXT_TYPE_EX(CLocaleManager::LANG_TYPE_QUEST, GetCurrentCharacterPtr(), lua_tostring(qs.co, -1));
			}
			else
			{
				sys_err("SELECT wrong data %s", lua_typename(qs.co, -1));
				sys_err("here");
			}
			lua_pop(qs.co,1);
		}
		os << "}";


		AddScript(os.str());
		qs.suspend_state = SUSPEND_STATE_SELECT;
		if ( test_server )
			sys_log( 0, "%s", m_strScript.c_str() );
		SendScript();
	}

	EVENTINFO(confirm_timeout_event_info)
	{
		DWORD dwWaitPID;
		DWORD dwReplyPID;

		confirm_timeout_event_info()
		: dwWaitPID( 0 )
		, dwReplyPID( 0 )
		{
		}
	};

	EVENTFUNC(confirm_timeout_event)
	{
		confirm_timeout_event_info * info = dynamic_cast<confirm_timeout_event_info *>(event->info);

		if ( info == NULL )
		{
			sys_err( "confirm_timeout_event> <Factor> Null pointer" );
			return 0;
		}

		LPCHARACTER chWait = CHARACTER_MANAGER::instance().FindByPID(info->dwWaitPID);
		LPCHARACTER chReply = NULL; //CHARACTER_MANAGER::info().FindByPID(info->dwReplyPID);

		if (chReply)
		{
			// ½Ã°£ Áö³ª¸é ¾Ë¾Æ¼­ ´ÝÈû
		}

		if (chWait)
		{
			CQuestManager::instance().Confirm(info->dwWaitPID, CONFIRM_TIMEOUT);
		}

		return 0;
	}

	void CQuestManager::GotoConfirmState(QuestState & qs)
	{
		qs.suspend_state = SUSPEND_STATE_CONFIRM;
		DWORD dwVID = (DWORD) lua_tonumber(qs.co, -3);
		const char* szMsg = lua_tostring(qs.co, -2);
		int iTimeout = (int) lua_tonumber(qs.co, -1);

		sys_log(0, "GotoConfirmState vid %u msg '%s', timeout %d", dwVID, szMsg, iTimeout);

		// 1. »ó´ë¹æ¿¡°Ô È®ÀÎÃ¢ ¶ç¿ò
		// 2. ³ª¿¡°Ô È®ÀÎ ±â´Ù¸°´Ù°í Ç¥½ÃÇÏ´Â Ã¢ ¶ç¿ò
		// 3. Å¸ÀÓ¾Æ¿ô ¼³Á¤ (Å¸ÀÓ¾Æ¿ô µÇ¸é »ó´ë¹æ Ã¢ ´Ý°í ³ª¿¡°Ôµµ Ã¢ ´ÝÀ¸¶ó°í º¸³¿)

		// 1
		// »ó´ë¹æÀÌ ¾ø´Â °æ¿ì´Â ±×³É »ó´ë¹æ¿¡°Ô º¸³»Áö ¾Ê´Â´Ù. Å¸ÀÓ¾Æ¿ô¿¡ ÀÇÇØ¼­ ³Ñ¾î°¡°ÔµÊ
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(dwVID);
		if (ch && ch->IsPC())
		{
			ch->ConfirmWithMsg(LC_TEXT_TYPE_EX(CLocaleManager::LANG_TYPE_QUEST, ch->GetLanguageID(), szMsg), iTimeout, GetCurrentCharacterPtr()->GetPlayerID());
		}

		// 2
		GetCurrentPC()->SetConfirmWait((ch && ch->IsPC())?ch->GetPlayerID():0);
		ostringstream os;
		os << "{CONFIRM_WAIT timeout;" << iTimeout << "}";
		AddScript(os.str());
		SendScript();

		// 3
		confirm_timeout_event_info* info = AllocEventInfo<confirm_timeout_event_info>();

		info->dwWaitPID = GetCurrentCharacterPtr()->GetPlayerID();
		info->dwReplyPID = (ch && ch->IsPC()) ? ch->GetPlayerID() : 0;

		event_create(confirm_timeout_event, info, PASSES_PER_SEC(iTimeout));
	}

	void CQuestManager::GotoSelectItemState(QuestState& qs)
	{
		qs.suspend_state = SUSPEND_STATE_SELECT_ITEM;
		AddScript("{SELECT_ITEM}");
		SendScript();
	}

	void CQuestManager::GotoInputState(QuestState & qs)
	{
		qs.suspend_state = SUSPEND_STATE_INPUT;
		AddScript("{INPUT}");
		SendScript();

		// ½Ã°£ Á¦ÇÑÀ» °Ë
		//event_create(input_timeout_event, dwEI, PASSES_PER_SEC(iTimeout));
	}

	void CQuestManager::GotoPauseState(QuestState & qs)
	{
		qs.suspend_state = SUSPEND_STATE_PAUSE;
		AddScript("{NEXT}");
		SendScript();
	}

	void CQuestManager::GotoEndState(QuestState & qs)
	{
		AddScript("{DONE}");
		SendScript();
	}

	//
	// * OpenState
	//
	// The beginning of script
	// 

	QuestState CQuestManager::OpenState(const string& quest_name, int state_index)
	{
		QuestState qs;
		qs.args=0;
		qs.st = state_index;
		qs.co = lua_newthread(L);
		qs.ico = lua_ref(L, 1/*qs.co*/);
		return qs;
	}

	//
	// * RunState
	// 
	// decides script to wait for user input, or finish
	// 
	bool CQuestManager::RunState(QuestState & qs)
	{
		ClearError();

		m_CurrentRunningState = &qs;
		int ret = lua_resume(qs.co, qs.args);

		if (ret == 0)
		{
			if (lua_gettop(qs.co) == 0)
			{
				// end of quest
				GotoEndState(qs);
				return false;
			}

			if (!lua_isstring(qs.co, 1))
			{
				sys_err("invalid return of lua [%d]", lua_type(qs.co, 1));
				GotoEndState(qs);
				return false;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "select"))
			{
				GotoSelectState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "wait"))
			{
				GotoPauseState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "input"))
			{
				GotoInputState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "confirm"))
			{
				GotoConfirmState(qs);
				return true;
			}

			if (!strcmp(lua_tostring(qs.co, 1), "select_item"))
			{
				GotoSelectItemState(qs);
				return true;
			}
		}
		else
		{
			sys_err("LUA_ERROR: %s", lua_tostring(qs.co, 1));
		}

		WriteRunningStateToSyserr();
		SetError();

		GotoEndState(qs);
		return false;
	}

	//
	// * CloseState
	//
	// makes script end
	//
	void CQuestManager::CloseState(QuestState& qs)
	{
		if (qs.co)
		{
			//cerr << "ICO "<<qs.ico <<endl;
			lua_unref(L, qs.ico);
			qs.co = 0;
		}
	}
}
