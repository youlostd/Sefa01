#include "stdafx.h"

#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "utils.h"
#include "config.h"
#include "p2p.h"
#include "db.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest
{
	int event_open(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		int iEventIndex = lua_tonumber(L, 1);
		CEventManager::instance().OpenEventRegistration(iEventIndex);

		return 0;
	}

	int event_close(lua_State* L)
	{
		bool bOnlyRegistration = false;
		if (lua_isboolean(L, 1))
			bOnlyRegistration = lua_toboolean(L, 1);

		CEventManager::instance().CloseEventRegistration(!bOnlyRegistration);

		return 0;
	}

	int event_over(lua_State* L)
	{
		CEventManager::instance().OnEventOver();

		return 0;
	}

	int event_is_running(lua_State* L)
	{
		if (lua_isnumber(L, 1))
			return CEventManager::instance().GetRunningEventIndex() == lua_tonumber(L, 1);
		else
			return CEventManager::instance().IsRunningEvent();
	}

	int event_get_name(lua_State* L)
	{
		if (CEventManager::instance().IsRunningEvent())
			lua_pushstring(L, CEventManager::instance().GetRunningEventData()->name.c_str());
		else
			lua_pushstring(L, "");
		return 1;
	}

	int event_is_player_ignored(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChr && CEventManager::instance().IsIgnorePlayer(pkChr->GetPlayerID()))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
		return 1;
	}

	int event_can_request_join(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		bool bCanJoin = CEventManager::instance().IsRunningEvent() && pkChr && CEventManager::instance().CanSendRequest(pkChr);

		lua_pushboolean(L, bCanJoin);
		return 1;
	}

	int event_request_join(lua_State* L)
	{
		if (!CEventManager::instance().IsRunningEvent())
			return 0;

		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!pkChr)
			return 0;

		CEventManager::instance().SendRequest(pkChr);
		return 0;
	}

	int event_get_rewards(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		DWORD dwEventIndex = lua_tonumber(L, 1);

		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT vnum,count,chance FROM event_reward WHERE event_index = %u", dwEventIndex));

		lua_newtable(L);

		int i = 0;
		while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
		{
			int col = 0;

			CEventTagTeam::TRewardInfo kInfo;
			str_to_number(kInfo.dwItemVnum, row[col++]);
			str_to_number(kInfo.bItemCount, row[col++]);
			str_to_number(kInfo.dwChance, row[col++]);

			lua_newtable(L);
			lua_pushnumber(L, kInfo.dwItemVnum);
			lua_rawseti(L, -2, 1);
			lua_pushnumber(L, kInfo.bItemCount);
			lua_rawseti(L, -2, 2);
			lua_pushnumber(L, kInfo.dwChance);
			lua_rawseti(L, -2, 3);

			lua_rawseti(L, -2, ++i);
		}

		return 1;
	}

	int event_add_reward(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		DWORD dwEventIndex = lua_tonumber(L, 1);
		DWORD dwItemVnum = lua_tonumber(L, 2);
		BYTE bItemCount = lua_tonumber(L, 3);
		DWORD dwChance = lua_tonumber(L, 4);

		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("INSERT INTO event_reward (event_index, vnum, count, chance) "
			"VALUES (%u, %u, %u, %u)", dwEventIndex, dwItemVnum, bItemCount, dwChance));

		lua_pushboolean(L, pMsg->Get()->uiAffectedRows > 0);
		return 1;
	}

	int event_remove_reward(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		DWORD dwEventIndex = lua_tonumber(L, 1);
		DWORD dwItemVnum = lua_tonumber(L, 2);
		BYTE bItemCount = lua_tonumber(L, 3);

		std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("DELETE FROM event_reward WHERE event_index = %u AND vnum = %u AND count = %u",
			dwEventIndex, dwItemVnum, bItemCount));

		lua_pushboolean(L, pMsg->Get()->uiAffectedRows > 0);
		return 1;
	}

	int event_has_tag_team(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		if (lua_isnumber(L, 1))
			pkChr = CHARACTER_MANAGER::instance().Find(lua_tonumber(L, 1));

		if (pkChr)
		{
			lua_pushboolean(L, CEventManager::instance().TagTeam_IsRegistered(pkChr));
			return 1;
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int event_register_tag_team(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChr)
		{
			LPCHARACTER pkOther = CHARACTER_MANAGER::instance().Find(lua_tonumber(L, 1));
			if (pkOther && pkOther->GetMapIndex() == pkChr->GetMapIndex() && pkOther != pkChr)
			{
				CEventManager::instance().TagTeam_Register(pkChr, pkOther);
				lua_pushboolean(L, 1);
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int event_unregister_tag_team(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		if (pkChr)
		{
			CEventManager::instance().TagTeam_Unregister(pkChr);
			lua_pushboolean(L, 1);
			return 1;
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int event_get_registrations(lua_State* L)
	{
		BYTE bGroupIdx;
		if (!lua_isnumber(L, 1))
			bGroupIdx = 2;
		else
			bGroupIdx = lua_tonumber(L, 1);

		std::vector<CEventManager::TTagTeam> vecTagTeams;

		for (int i = 0; i < 2; ++i)
		{
			if (bGroupIdx >= 0 && bGroupIdx < 2 && i != bGroupIdx)
				continue;

			const std::map<DWORD, DWORD>& rkMap = CEventManager::instance().TagTeam_GetRegistrationMap(i);

			if (rkMap.size())
			{
				std::set<DWORD> set_Used;

				int iIndex = 0;

				for (itertype(rkMap) it = rkMap.begin(); it != rkMap.end(); ++it)
				{
					if (set_Used.find(it->first) != set_Used.end())
						continue;

					CEventManager::TTagTeam kCurTagTeam;

					DWORD dwPID1 = kCurTagTeam.pid1();
					auto& name1 = kCurTagTeam.name1();
					DWORD dwPID2 = kCurTagTeam.pid2();
					auto& name2 = kCurTagTeam.name2();

					LPCHARACTER pkChr;

					// player 1
					pkChr = CHARACTER_MANAGER::instance().FindByPID(it->first);
					if (pkChr)
					{
						kCurTagTeam.set_pid1(pkChr->GetPlayerID());
						kCurTagTeam.set_name1(pkChr->GetName());
					}
					else
					{
						CCI* pkCCI = P2P_MANAGER::instance().FindByPID(it->first);
						if (pkCCI)
						{
							kCurTagTeam.set_pid1(pkCCI->dwPID);
							kCurTagTeam.set_name1(pkCCI->szName);
						}
					}
					// player 2
					pkChr = CHARACTER_MANAGER::instance().FindByPID(it->second);
					if (pkChr)
					{
						kCurTagTeam.set_pid2(pkChr->GetPlayerID());

						kCurTagTeam.set_pid2(pkChr->GetPlayerID());
						kCurTagTeam.set_name2(pkChr->GetName());
					}
					else
					{
						CCI* pkCCI = P2P_MANAGER::instance().FindByPID(it->second);
						if (pkCCI)
						{
							kCurTagTeam.set_pid2(pkCCI->dwPID);
							kCurTagTeam.set_name2(pkCCI->szName);
						}
					}

					++iIndex;

					vecTagTeams.push_back(kCurTagTeam);
					set_Used.insert(it->second);
				}
			}
		}

		lua_newtable(L);

		for (int i = 0; i < vecTagTeams.size(); ++i)
		{
			lua_newtable(L);

			lua_pushnumber(L, vecTagTeams[i].pid1());
			lua_rawseti(L, -2, 1);

			lua_pushstring(L, CLocaleManager::instance().StringToArgument(vecTagTeams[i].name1().c_str()));
			lua_rawseti(L, -2, 2);

			lua_pushnumber(L, vecTagTeams[i].pid2());
			lua_rawseti(L, -2, 3);

			lua_pushstring(L, CLocaleManager::instance().StringToArgument(vecTagTeams[i].name2().c_str()));
			lua_rawseti(L, -2, 4);

			lua_rawseti(L, -2, i + 1);
		}

		return 1;
	}

	int event_start_tag_team(lua_State* L)
	{
		BYTE bGroupIdx;
		if (!lua_isnumber(L, 1))
			bGroupIdx = 2;
		else
			bGroupIdx = lua_tonumber(L, 1);

		const std::map<DWORD, DWORD>* pkMap[2] = { &CEventManager::instance().TagTeam_GetRegistrationMap(0), &CEventManager::instance().TagTeam_GetRegistrationMap(1) };

		google::protobuf::RepeatedPtrField<CEventManager::TTagTeam> akTagTeams;

		for (int i = 0; i < (test_server ? 2 : CEventTagTeam::MAX_TEAM_COUNT); ++i)
		{
			if (!lua_isnumber(L, i + 1 + 1))
			{
				sys_err("no number as argument %d", i + 1 + 1);
				lua_pushboolean(L, 0);
				return 1;
			}

			akTagTeams.Add();

			std::map<DWORD, DWORD>::const_iterator it;
			for (int j = 0; j < 2; ++j)
			{
				it = pkMap[j]->find(lua_tonumber(L, i + 1 + 1));
				if (it != pkMap[j]->end())
					break;

				if (j == 1)
				{
					sys_err("cannot find pid %u (%d)", lua_tonumber(L, i + 1 + 1), i + 1 + 1);
					lua_pushboolean(L, 0);
					return 1;
				}
			}

			DWORD dwPID1 = akTagTeams[i].pid1();
			auto name1 = akTagTeams[i].name1();
			DWORD dwPID2 = akTagTeams[i].pid2();
			auto name2 = akTagTeams[i].name2();

			LPCHARACTER pkChr;
			
			// player 1
				// player 1
			pkChr = CHARACTER_MANAGER::instance().FindByPID(it->first);
			if (pkChr)
			{
				akTagTeams[i].set_pid1(pkChr->GetPlayerID());
				akTagTeams[i].set_name1(pkChr->GetName());
			}
			else
			{
				CCI* pkCCI = P2P_MANAGER::instance().FindByPID(it->first);
				if (pkCCI)
				{
					akTagTeams[i].set_pid1(pkCCI->dwPID);
					akTagTeams[i].set_name1(pkCCI->szName);
				}
				else
				{
					sys_err("cannot find player 1 (%u)", it->first);
					lua_pushboolean(L, 0);
					return 1;
				}
			}
			// player 2
			pkChr = CHARACTER_MANAGER::instance().FindByPID(it->second);
			if (pkChr)
			{
				akTagTeams[i].set_pid2(pkChr->GetPlayerID());
				akTagTeams[i].set_name2(pkChr->GetName());
			}
			else
			{
				CCI* pkCCI = P2P_MANAGER::instance().FindByPID(it->second);
				if (pkCCI)
				{
					akTagTeams[i].set_pid2(pkCCI->dwPID);
					akTagTeams[i].set_name2(pkCCI->szName);
				}
				else
				{
					sys_err("cannot find player 2 (%u)", it->second);
					lua_pushboolean(L, 0);
					return 1;
				}
			}
		}

		CEventManager::instance().TagTeam_Create(akTagTeams);

		lua_pushboolean(L, 1);
		return 1;
	}

	int event_update_empirewar(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		BYTE bEmpire = lua_tonumber(L, 1);
		WORD wKills = lua_tonumber(L, 2);
		WORD wDeads = lua_tonumber(L, 3);

		CEventManager::instance().EmpireWar_SendUpdatePacket(bEmpire, wKills, wDeads);

		return 0;
	}

#ifdef ENABLE_REACT_EVENT
	int event_react(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		lua_pushnumber(L, CEventManager::instance().React_Manager(lua_tonumber(L, 1)));
		return 1;
	}
#endif

	int event_warp_home(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD dwMapIndex = lua_tonumber(L, 1);

		const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
		for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
		{
			LPCHARACTER pkChr = it->second;
			if (!pkChr->GetDesc())
				continue;

			if (pkChr->GetMapIndex() == dwMapIndex && !pkChr->GetGMLevel())
				pkChr->GoHome();
		}

		return 0;
	}

	void RegisterEventFunctionTable()
	{
		luaL_reg event_functions[] =
		{
			{ "open",					event_open						},
			{ "close",					event_close						},
			{ "over",					event_over						},
			{ "is_running",				event_is_running				},
			{ "get_name",				event_get_name					},

			{ "is_player_ignored",		event_is_player_ignored			},
			{ "can_request_join",		event_can_request_join			},
			{ "request_join",			event_request_join				},

			{ "get_rewards",			event_get_rewards				},
			{ "remove_reward",			event_remove_reward				},
			{ "add_reward",				event_add_reward				},
			
			{ "has_tag_team",			event_has_tag_team				},
			{ "register_tag_team",		event_register_tag_team			},
			{ "unregister_tag_team",	event_unregister_tag_team		},
			{ "get_registrations",		event_get_registrations			},
			{ "start_tag_team",			event_start_tag_team			},

			{ "update_empirewar",		event_update_empirewar			},
#ifdef ENABLE_REACT_EVENT
			{ "react",					event_react						},
#endif
			{ "warp_home",				event_warp_home					},
			
			{ NULL, NULL }
		};
		CQuestManager::instance().AddLuaFunctionTable("event", event_functions);
	}
}
#endif
