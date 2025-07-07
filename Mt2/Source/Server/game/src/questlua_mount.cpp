#include "stdafx.h"

#include "questlua.h"
#include "questmanager.h"
#include "char.h"
#include "affect.h"
#include "config.h"
#include "utils.h"
#include "mount_system.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest
{
	//
	// "mount" Lua functions
	//
	int mount_is_mine(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER pkNPCChr = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (!pkChr || !pkNPCChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkNPCChr->GetRider() == pkChr);
		return 1;
	}

	int mount_is_riding(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr || !pkChr->GetMountSystem())
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->GetMountSystem()->IsRiding());
		return 1;
	}

	int mount_is_summoned(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->GetMountSystem()->IsSummoned());
		return 1;
	}

	int mount_is_horse_riding(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->GetMountSystem()->IsRiding() && pkChr->IsHorseSummoned());
		return 1;
	}

	int mount_is_horse_summoned(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->GetMountSystem()->IsSummoned() && pkChr->IsHorseSummoned());
		return 1;
	}

	int mount_summon(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM pkItem = CQuestManager::instance().GetCurrentItem();

		if (!pkChr || !pkItem)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->GetMountSystem()->Summon(pkItem));
		return 1;
	}

	int mount_unsummon(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (pkChr->GetMountSystem()->IsRiding())
			pkChr->GetMountSystem()->StopRiding();
		if (pkChr->GetMountSystem()->IsSummoned())
			pkChr->GetMountSystem()->Unsummon();

		if (pkChr->GetMountVnum())
			pkChr->MountVnum(0);

		lua_pushboolean(L, 1);
		return 1;
	}

	int mount_ride(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->GetMountSystem()->StartRiding());
		return 1;
	}

	int mount_unride(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		pkChr->GetMountSystem()->StopRiding();

		lua_pushboolean(L, 1);
		return 1;
	}

	int mount_set_name(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		const char* c_pszName = lua_tostring(L, 1);
		if (!check_name(c_pszName))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		pkChr->SetHorseName(c_pszName);

		lua_pushboolean(L, 1);
		return 1;
	}

	int mount_get_name(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushstring(L, "");
			return 1;
		}

		lua_pushstring(L, pkChr->GetHorseName().c_str());
		return 1;
	}

	int mount_get_horse_grade(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, pkChr->GetHorseGrade());
		return 1;
	}

	int mount_get_horse_time_left(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, (int)pkChr->GetHorseMaxLifeTime() - (int)pkChr->GetHorseElapsedTime());
		return 1;
	}

	int mount_get_horse_time_max(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, pkChr->GetHorseMaxLifeTime());
		return 1;
	}

	int mount_is_horse_dead(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->IsHorseDead());
		return 1;
	}

	int mount_upgrade_horse(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->UpgradeHorseGrade());
		return 1;
	}

	int mount_revive_horse(lua_State* L)
	{
		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->HorseRevive());
		return 1;
	}

	int mount_feed_horse(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid arguments (expected 2 numbers)");
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		int iPct = lua_tonumber(L, 1);
		int iRagePct = lua_tonumber(L, 2);
		if (iPct <= 0 || iPct > 100 || iRagePct < 0 || iRagePct > HORSE_MAX_RAGE)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, pkChr->HorseFeed(iPct, iRagePct));
		return 1;
	}

	int mount_set_grade(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER pkChr = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!pkChr)
			return 0;

		int grade = lua_tonumber(L, 1);

		pkChr->SetHorseGrade(grade);
		pkChr->SetHorseElapsedTime(0);
		pkChr->ComputePoints();
		pkChr->SkillLevelPacket();

		return 1;
	}

	void RegisterMountFunctionTable()
	{
		luaL_reg mount_functions[] =
		{
			{ "is_mine",				mount_is_mine				},
			{ "is_riding",				mount_is_riding				},
			{ "is_summoned",			mount_is_summoned			},
			{ "is_horse_riding",		mount_is_horse_riding		},
			{ "is_horse_summoned",		mount_is_horse_summoned		},
			{ "summon",					mount_summon				},
			{ "unsummon",				mount_unsummon				},
			{ "ride",					mount_ride					},
			{ "unride",					mount_unride				},
			{ "set_name",				mount_set_name				},
			{ "get_name",				mount_get_name				},
			{ "get_horse_grade",		mount_get_horse_grade		},
			{ "get_horse_time_left",	mount_get_horse_time_left	},
			{ "get_horse_time_max",		mount_get_horse_time_max	},
			{ "is_horse_dead",			mount_is_horse_dead			},
			{ "upgrade_horse",			mount_upgrade_horse			},
			{ "revive_horse",			mount_revive_horse			},
			{ "feed_horse",				mount_feed_horse			},
			{ "set_grade",				mount_set_grade				},

			{ NULL,						NULL						}
		};

		CQuestManager::instance().AddLuaFunctionTable("mount", mount_functions);
	}
}
