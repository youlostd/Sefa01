#include "stdafx.h"

#include "questlua.h"
#include "questmanager.h"
#include "desc_client.h"
#include "char.h"
#include "char_manager.h"
#include "utils.h"
#include "guild.h"
#include "guild_manager.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest
{
	//
	// "guild" Lua functions
	//
	int guild_around_ranking_string(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch->GetGuild())
			lua_pushstring(L,"");
		else
		{
			char szBuf[4096+1];
			CGuildManager::instance().GetAroundRankString(ch->GetGuild()->GetID(), szBuf, sizeof(szBuf));
			lua_pushstring(L, szBuf);
		}
		return 1;
	}

	int guild_high_ranking_string(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		DWORD dwMyGuild = 0;
		if (ch->GetGuild())
			dwMyGuild = ch->GetGuild()->GetID();

		char szBuf[4096+1];
		CGuildManager::instance().GetHighRankString(dwMyGuild, szBuf, sizeof(szBuf));
		lua_pushstring(L, szBuf);
		return 1;
	}

	int guild_get_ladder_point(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!ch->GetGuild())
		{
			lua_pushnumber(L, -1);
		}
		else
		{
			lua_pushnumber(L, ch->GetGuild()->GetLadderPoint());
		}
		return 1;
	}

	int guild_get_rank(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (!ch->GetGuild())
		{
			lua_pushnumber(L, -1);
		}
		else
		{
			lua_pushnumber(L, CGuildManager::instance().GetRank(ch->GetGuild()));
		}
		return 1;
	}

	int guild_is_war(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->GetGuild() && ch->GetGuild()->UnderWar((DWORD) lua_tonumber(L, 1)))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);

		return 1;
	}

	int guild_name(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		CGuild * pkGuild = CGuildManager::instance().FindGuild((DWORD)lua_tonumber(L, 1));

		if (pkGuild)
			lua_pushstring(L, CLocaleManager::instance().StringToArgument(pkGuild->GetName()));
		else
			lua_pushstring(L, "");

		return 1;
	}

	int guild_level(lua_State* L)
	{
		luaL_checknumber(L, 1);

		CGuild * pkGuild = CGuildManager::instance().FindGuild((DWORD) lua_tonumber(L, 1));

		if (pkGuild)
			lua_pushnumber(L, pkGuild->GetLevel());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int guild_war_enter(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (ch->GetGuild())
			ch->GetGuild()->GuildWarEntryAccept((DWORD) lua_tonumber(L, 1), ch);

		return 0;
	}

	int guild_get_any_war(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->GetGuild())
			lua_pushnumber(L, ch->GetGuild()->UnderAnyWar());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int guild_get_name(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushstring(L,  "");
			return 1;
		}

		CGuild * pkGuild = CGuildManager::instance().FindGuild((DWORD) lua_tonumber(L, 1));

		if (pkGuild)
			lua_pushstring(L, CLocaleManager::instance().StringToArgument(pkGuild->GetName()));
		else
			lua_pushstring(L, "");

		return 1;
	}

	int guild_war_bet(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		network::GDOutputPacket<network::GDGuildWarBetPacket> p;

		p->set_war_id((DWORD) lua_tonumber(L, 1));
		p->set_login(ch->GetDesc()->GetAccountTable().login());
		p->set_guild_id((DWORD) lua_tonumber(L, 2));
		p->set_gold((DWORD) lua_tonumber(L, 3));

		sys_log(0, "GUILD_WAR_BET: %s login %s war_id %u guild %u gold %u", 
				ch->GetName(), p->login().c_str(), p->war_id(), p->guild_id(), p->gold());

		db_clientdesc->DBPacket(p);
		return 0;
	}

	int guild_is_bet(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			lua_pushboolean(L, true);
			return 1;
		}

		bool bBet = CGuildManager::instance().IsBet((DWORD) lua_tonumber(L, 1),
				CQuestManager::instance().GetCurrentCharacterPtr()->GetDesc()->GetAccountTable().login().c_str());

		lua_pushboolean(L, bBet);
		return 1;
	}

	int guild_get_warp_war_list(lua_State* L)
	{
		FBuildLuaGuildWarList f(L);
		CGuildManager::instance().for_each_war(f);
		return 1;
	}

	int guild_get_reserve_war_table(lua_State * L)
	{
		std::vector<CGuildWarReserveForGame *> & con = CGuildManager::instance().GetReserveWarRef();

		int i = 0;
		std::vector<CGuildWarReserveForGame *>::iterator it = con.begin();

		sys_log(0, "con.size(): %d", con.size());

		// stack : table1
		lua_newtable(L);

		while (it != con.end())
		{
			TGuildWarReserve * p = &(*(it++))->data;

			if (p->bType != GUILD_WAR_TYPE_BATTLE)
				continue;

			lua_newtable(L);

			sys_log(0, "con.size(): %u %u %u handi %d", p->dwID, p->dwGuildFrom, p->dwGuildTo, p->lHandicap);

			// stack : table1 table2
			lua_pushnumber(L, p->dwID);
			// stack : table1 table2 dwID
			lua_rawseti(L, -2, 1);

			// stack : table1 table2
			if (p->lPowerFrom > p->lPowerTo)
				lua_pushnumber(L, p->dwGuildFrom);
			else
				lua_pushnumber(L, p->dwGuildTo);
			// stack : table1 table2 guildfrom
			lua_rawseti(L, -2, 2);

			// stack : table1 table2
			if (p->lPowerFrom > p->lPowerTo)
				lua_pushnumber(L, p->dwGuildTo);
			else
				lua_pushnumber(L, p->dwGuildFrom);
			// stack : table1 table2 guildto
			lua_rawseti(L, -2, 3);

			lua_pushnumber(L, p->lHandicap);
			lua_rawseti(L, -2, 4);

			// stack : table1 table2
			lua_rawseti(L, -2, ++i);
		}

		return 1;
	}

	int guild_get_member_count(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if ( ch == NULL )
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		CGuild* pGuild = ch->GetGuild();

		if ( pGuild == NULL )
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, pGuild->GetMemberCount());

		return 1;
	}

	int guild_change_master(lua_State* L)
	{
		// ¸®ÅÏ°ª
		//	0 : ÀÔ·ÂÇÑ ÀÌ¸§ÀÌ Àß¸øµÇ¾úÀ½ ( ¹®ÀÚ¿­ÀÌ ¾Æ´Ô )
		//	1 : ±æµåÀåÀÌ ¾Æ´Ô
		//	2 : ÁöÁ¤ÇÑ ÀÌ¸§ÀÇ ±æµå¿øÀÌ ¾øÀ½
		//	3 : ¿äÃ» ¼º°ø
		//	4 : ±æµå°¡ ¾øÀ½

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		CGuild* pGuild = ch->GetGuild();

		if ( pGuild != NULL )
		{
			if ( pGuild->GetMasterPID() == ch->GetPlayerID() )
			{
				if ( lua_isstring(L, 1) == false )
				{
					lua_pushnumber(L, 0);
				}
				else
				{
					bool ret = pGuild->ChangeMasterTo(pGuild->GetMemberPID(lua_tostring(L, 1)));

					lua_pushnumber(L, ret == false ? 2 : 3 );
				}
			}
			else
			{
				lua_pushnumber(L, 1);
			}
		}
		else
		{
			lua_pushnumber(L, 4);
		}

		return 1;
	}

	int guild_change_master_with_limit(lua_State* L)
	{
		// ÀÎÀÚ
		//  arg0 : »õ ±æµåÀå ÀÌ¸§
		//  arg1 : »õ ±æµåÀå ·¹º§ Á¦ÇÑ
		//  arg2 : resign_limit Á¦ÇÑ ½Ã°£
		//  arg3 : be_other_leader Á¦ÇÑ ½Ã°£
		//  arg4 : be_other_member Á¦ÇÑ ½Ã°£
		//  arg5 : Ä³½ÃÅÛÀÎ°¡ ¾Æ´Ñ°¡
		//
		// ¸®ÅÏ°ª
		//	0 : ÀÔ·ÂÇÑ ÀÌ¸§ÀÌ Àß¸øµÇ¾úÀ½ ( ¹®ÀÚ¿­ÀÌ ¾Æ´Ô )
		//	1 : ±æµåÀåÀÌ ¾Æ´Ô
		//	2 : ÁöÁ¤ÇÑ ÀÌ¸§ÀÇ ±æµå¿øÀÌ ¾øÀ½
		//	3 : ¿äÃ» ¼º°ø
		//	4 : ±æµå°¡ ¾øÀ½
		//	5 : ÁöÁ¤ÇÑ ÀÌ¸§ÀÌ ¿Â¶óÀÎÀÌ ¾Æ´Ô
		//	6 : ÁöÁ¤ÇÑ Ä³¸¯ÅÍ ·¹º§ÀÌ ±âÁØ·¹º§º¸´Ù ³·À½
		//	7 : »õ ±æµåÀåÀÌ be_other_leader Á¦ÇÑ¿¡ °É¸²

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		CGuild* pGuild = ch->GetGuild();

		if ( pGuild != NULL )
		{
			if ( pGuild->GetMasterPID() == ch->GetPlayerID() )
			{
				if ( lua_isstring(L, 1) == false )
				{
					lua_pushnumber(L, 0);
				}
				else
				{
					LPCHARACTER pNewMaster = CHARACTER_MANAGER::instance().FindPC( lua_tostring(L,1) );

					if ( pNewMaster != NULL )
					{
						if ( pNewMaster->GetLevel() < lua_tonumber(L, 2) )
						{
							lua_pushnumber(L, 6);
						}
						else
						{
							int nBeOtherLeader = pNewMaster->GetQuestFlag("change_guild_master.be_other_leader");
							CQuestManager::instance().GetPC( ch->GetPlayerID() );

							if ( lua_toboolean(L, 6) == true ) nBeOtherLeader = 0;

							if ( nBeOtherLeader > get_global_time() )
							{
								lua_pushnumber(L, 7);
							}
							else
							{
								bool ret = pGuild->ChangeMasterTo(pGuild->GetMemberPID(lua_tostring(L, 1)));

								if ( ret == false )
								{
									lua_pushnumber(L, 2);
								}
								else
								{
									lua_pushnumber(L, 3);

									pNewMaster->SetQuestFlag("change_guild_master.be_other_leader", 0);
									pNewMaster->SetQuestFlag("change_guild_master.be_other_member", 0);
									pNewMaster->SetQuestFlag("change_guild_master.resign_limit", (int)lua_tonumber(L, 3));

									ch->SetQuestFlag("change_guild_master.be_other_leader", (int)lua_tonumber(L, 4));
									ch->SetQuestFlag("change_guild_master.be_other_member", (int)lua_tonumber(L, 5));
									ch->SetQuestFlag("change_guild_master.resign_limit", 0);
								}
							}
						}
					}
					else
					{
						lua_pushnumber(L, 5);
					}
				}
			}
			else
			{
				lua_pushnumber(L, 1);
			}
		}
		else
		{
			lua_pushnumber(L, 4);
		}

		return 1;
	}

#ifdef __GUILD_SAFEBOX__
	int guild_has_safebox(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (ch->GetGuild())
			{
				lua_pushboolean(L, ch->GetGuild()->GetSafeBox().HasSafebox());
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int guild_give_safebox(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (ch->GetGuild())
			{
				CGuildSafeBox& rkSafeBox = ch->GetGuild()->GetSafeBox();
				if (!rkSafeBox.HasSafebox())
				{
					if (lua_isnumber(L, 1))
						rkSafeBox.GiveSafebox(ch, lua_tonumber(L, 1));
					else
						rkSafeBox.GiveSafebox(ch);
					lua_pushboolean(L, 1);
					return 1;
				}
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int guild_open_safebox(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (ch->GetGuild())
			{
				CGuildSafeBox& rkSafeBox = ch->GetGuild()->GetSafeBox();
				if (rkSafeBox.HasSafebox())
				{
					rkSafeBox.OpenSafebox(ch);
					lua_pushboolean(L, 1);
					return 1;
				}
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int guild_get_safebox_size(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (ch->GetGuild())
			{
				CGuildSafeBox& rkSafeBox = ch->GetGuild()->GetSafeBox();
				if (rkSafeBox.HasSafebox())
				{
					lua_pushnumber(L, rkSafeBox.GetSize());
					return 1;
				}
			}
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int guild_change_safebox_size(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (ch->GetGuild())
			{
				CGuildSafeBox& rkSafeBox = ch->GetGuild()->GetSafeBox();
				if (rkSafeBox.HasSafebox())
				{
					rkSafeBox.ChangeSafeboxSize(ch, lua_tonumber(L, 1));
					lua_pushboolean(L, 1);
					return 1;
				}
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}
#endif

	void RegisterGuildFunctionTable()
	{
		luaL_reg guild_functions[] =
		{
			{ "get_rank",				guild_get_rank				},
			{ "get_ladder_point",		guild_get_ladder_point		},
			{ "high_ranking_string",	guild_high_ranking_string	},
			{ "around_ranking_string",	guild_around_ranking_string	},
			{ "name",					guild_name					},
			{ "level",					guild_level					},
			{ "is_war",					guild_is_war				},
			{ "war_enter",				guild_war_enter				},
			{ "get_any_war",			guild_get_any_war			},
			{ "get_reserve_war_table",	guild_get_reserve_war_table	},
			{ "get_name",				guild_get_name				},
			{ "war_bet",				guild_war_bet				},
			{ "is_bet",					guild_is_bet				},
			{ "get_warp_war_list",		guild_get_warp_war_list		},
			{ "get_member_count",		guild_get_member_count		},
			{ "change_master",			guild_change_master			},
			{ "change_master_with_limit",			guild_change_master_with_limit			},
			
#ifdef __GUILD_SAFEBOX__
			{ "has_safebox",			guild_has_safebox			},
			{ "give_safebox",			guild_give_safebox			},
			{ "open_safebox",			guild_open_safebox			},
			{ "get_safebox_size",		guild_get_safebox_size		},
			{ "change_safebox_size",	guild_change_safebox_size	},
#endif

			{ NULL,						NULL						}
		};

		CQuestManager::instance().AddLuaFunctionTable("guild", guild_functions);
	}
}

