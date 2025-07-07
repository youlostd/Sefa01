
#include "stdafx.h"

#include "config.h"
#include "questmanager.h"
#include "char.h"
#include "db.h"

#include <cctype>
#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

#ifdef __VOTE4BUFF__
namespace quest
{
	//
	// "v4b" Lua functions
	//
	int v4b_is_loaded(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
		{
			if (/*ch->IsLoadedAffect() && */ch->V4B_IsLoaded())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int v4b_can_vote(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
		{
			std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT last_vote FROM vote4buff_lastvote WHERE hwid = '%s'", ch->GetEscapedHWID()));
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int iLastVote = 0;
			if (row)
				str_to_number(iLastVote, row[0]);

			lua_pushboolean(L, !iLastVote || get_global_time() - iLastVote > 60 * 60 * 24);
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int v4b_has_buff(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
		{
			if (ch->IsLoadedAffect() && ch->V4B_IsLoaded() && ch->V4B_IsBuff())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int v4b_can_give_buff(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
		{
			if (ch->IsLoadedAffect() && ch->V4B_IsLoaded() && !ch->V4B_IsBuff())
			{
				std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT vote4buff FROM account WHERE id = %u", ch->GetAID()));

				if (pMsg->Get()->uiNumRows > 0)
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
					if (**row != '0')
					{
						lua_pushboolean(L, 1);
						return 1;
					}
				}
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int v4b_give_buff(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
		{
			if (ch->IsLoadedAffect() && ch->V4B_IsLoaded() && !ch->V4B_IsBuff())
			{
				BYTE bApplyType = lua_tonumber(L, 1);
				int iApplyValue = lua_tonumber(L, 2);
				DWORD dwDuration = lua_tonumber(L, 3);

				DBManager::instance().Query("UPDATE account SET vote4buff = vote4buff - 1 WHERE id = %u", ch->GetAID());
				DBManager::instance().Query("REPLACE INTO vote4buff (hwid, time_end, applytype, applyvalue) VALUES ('%s', %u, %u, %d)",
					ch->GetEscapedHWID(), get_global_time() + dwDuration, bApplyType, iApplyValue);
				ch->V4B_GiveBuff(get_global_time() + dwDuration, bApplyType, iApplyValue);
			}
		}

		return 0;
	}
	
	void RegisterV4BFunctionTable()
	{
		luaL_reg v4b_functions[] =
		{
			{ "is_loaded",		v4b_is_loaded		},
			{ "can_vote",		v4b_can_vote		},
			{ "has_buff",		v4b_has_buff		},
			{ "can_give_buff",	v4b_can_give_buff	},
			{ "give_buff",		v4b_give_buff		},
			
			{ NULL,				NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("v4b", v4b_functions);
	}
};
#endif
