
#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "char_manager.h"
#include "OXEvent.h"
#include "config.h"
#include "db.h"

namespace quest
{
	int oxevent_get_status(lua_State* L)
	{
		OXEventStatus ret = COXEventManager::instance().GetStatus();

		lua_pushnumber(L, (int)ret);

		return 1;
	}

	int oxevent_open(lua_State* L)
	{
		COXEventManager::instance().ClearQuiz();

		if (CQuestManager::instance().GetEventFlag("oxquiz_file") == 0)
		{
			char szQuery[1024];
			int iQueryLen = snprintf(szQuery, sizeof(szQuery), "SELECT stage, answer-1");
			for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
				iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen, ", lang_%s", CLocaleManager::instance().GetLanguageName(i));
			std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("%s FROM common.ox_quiz WHERE enabled = 'TRUE'", szQuery));

			if (!pMsg->Get() || !pMsg->Get()->pSQLResult)
			{
				lua_pushnumber(L, 0);
				return 1;
			}

			int i = 0;
			while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
			{
				int col = 0;

				int iStage;
				bool bAnswer;
				const char* szQuestion[LANGUAGE_MAX_NUM];
				std::string stQuestions[LANGUAGE_MAX_NUM];

				str_to_number(iStage, row[col++]);
				str_to_number(bAnswer, row[col++]);
				for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
				{
					if (row[col])
						stQuestions[i] = row[col];
					else if (i != LANGUAGE_ENGLISH)
						stQuestions[i] = stQuestions[LANGUAGE_ENGLISH];
					++col;

					szQuestion[i] = stQuestions[i].c_str();
				}

				COXEventManager::instance().AddQuiz(iStage, szQuestion, bAnswer);
			}
		}
		else
		{
			char script[256];
			snprintf(script, sizeof(script), "%s/oxquiz.lua", Locale_GetBasePath().c_str());
			int result = lua_dofile(quest::CQuestManager::instance().GetLuaState(), script);

			if (result != 0)
			{
				lua_pushnumber(L, 0);
				return 1;
			}
		}

		COXEventManager::instance().SetStatus(OXEVENT_OPEN);

		lua_pushnumber(L, 1);
		return 1;
	}
	
	int oxevent_close(lua_State* L)
	{
		COXEventManager::instance().SetStatus(OXEVENT_CLOSE);
		
		return 0;
	}
	
	int oxevent_quiz(lua_State* L)
	{
		if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
		{
			bool ret = COXEventManager::instance().Quiz((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2));

			if (ret == false)
			{
				lua_pushnumber(L, 0);
			}
			else
			{
				lua_pushnumber(L, 1);
			}
		}

		return 1;
	}
	
	int oxevent_get_attender(lua_State* L)
	{
		lua_pushnumber(L, (int)COXEventManager::instance().GetAttenderCount());
		return 1;
	}

	EVENTINFO(end_oxevent_info)
	{
		int empty;

		end_oxevent_info() 
		: empty( 0 )
		{
		}
	};

	EVENTFUNC(end_oxevent)
	{
		COXEventManager::instance().CloseEvent();
		return 0;
	}

	int oxevent_end_event(lua_State* L)
	{
		COXEventManager::instance().SetStatus(OXEVENT_FINISH);

		end_oxevent_info* info = AllocEventInfo<end_oxevent_info>();
		event_create(end_oxevent, info, PASSES_PER_SEC(5));

		return 0;
	}

	int oxevent_end_event_force(lua_State* L)
	{
		COXEventManager::instance().CloseEvent();
		COXEventManager::instance().SetStatus(OXEVENT_FINISH);

		return 0;
	}

	int oxevent_give_item(lua_State* L)
	{
		if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
		{
			COXEventManager::instance().GiveItemToAttender((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2));
		}

		return 0;
	}
	
	void RegisterOXEventFunctionTable()
	{
		luaL_reg oxevent_functions[] = 
		{
			{	"get_status",	oxevent_get_status	},
			{	"open",			oxevent_open		},
			{	"close",		oxevent_close		},
			{	"quiz",			oxevent_quiz		},
			{	"get_attender",	oxevent_get_attender},
			{	"end_event",	oxevent_end_event	},
			{	"end_event_force",	oxevent_end_event_force	},
			{	"give_item",	oxevent_give_item	},

			{ NULL, NULL}
		};

		CQuestManager::instance().AddLuaFunctionTable("oxevent", oxevent_functions);
	}
}

