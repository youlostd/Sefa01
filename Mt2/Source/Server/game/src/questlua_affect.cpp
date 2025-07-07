#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "affect.h"
#include "db.h"

namespace quest
{
	//
	// "affect" Lua functions
	//
	int affect_add(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		BYTE applyOn = (BYTE) lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		if (ch->FindAffect(AFFECT_QUEST_START_IDX, applyOn)) // Äù½ºÆ®·Î ÀÎÇØ °°Àº °÷¿¡ È¿°ú°¡ °É·ÁÀÖÀ¸¸é ½ºÅµ
			return 0;

		long value = (long) lua_tonumber(L, 2);
		long duration = (long) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_QUEST_START_IDX, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int affect_add_biolog_bonus(lua_State * L)
	{
		// Index, Bonus, Value, Expire
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		BYTE bIndex = (BYTE) lua_tonumber(L, 1);
		BYTE bApply = (BYTE) lua_tonumber(L, 2);

		if (bApply >= MAX_APPLY_NUM || bApply < 1)
		{
			sys_err("apply is out of range : %d", bApply);
			return 0;
		}

		BYTE bPoint = aApplyInfo[bApply].bPointType;

		DWORD dwType = AFFECT_BIOLOGIST_START + bIndex;

		if (dwType < AFFECT_BIOLOGIST_START || dwType > AFFECT_BIOLOGIST_END)
		{
			sys_err("affect is out of range : %d", dwType);
			return 0;
		}

		long value = (long) lua_tonumber(L, 3);
		long duration = (long) lua_tonumber(L, 4);

		if (ch->FindAffect(dwType, bPoint))
			return 0;

		ch->AddAffect(dwType, bPoint, value, 0, duration, 0, false);

		return 0;
	}

	int affect_add_event_bonus(lua_State * L)
	{
		// Index, Bonus, Value, Expire
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		BYTE bIndex = (BYTE) lua_tonumber(L, 1);
		BYTE bApply = (BYTE) lua_tonumber(L, 2);

		if (bApply >= MAX_APPLY_NUM || bApply < 1)
		{
			sys_err("apply is out of range : %d", bApply);
			return 0;
		}

		BYTE bPoint = aApplyInfo[bApply].bPointType;

		DWORD dwType = AFFECT_EVENT_START + bIndex;

		if (dwType < AFFECT_EVENT_START || dwType > AFFECT_EVENT_END)
		{
			sys_err("affect is out of range : %d", dwType);
			return 0;
		}

		long value = (long) lua_tonumber(L, 3);
		long duration = (long) lua_tonumber(L, 4);

		if (ch->FindAffect(dwType, bPoint))
			return 0;

		ch->AddAffect(dwType, bPoint, value, 0, duration, 0, false);

		return 0;
	}

	int affect_has_event_bonus(lua_State * L)
	{
		// Index, Bonus, Value, Expire
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		BYTE bIndex = (BYTE) lua_tonumber(L, 1);
		BYTE bApply = (BYTE) lua_tonumber(L, 2);

		if (bApply >= MAX_APPLY_NUM || bApply < 1)
		{
			sys_err("apply is out of range : %d", bApply);
			lua_pushnumber(L, 0);
			return 1;
		}

		BYTE bPoint = aApplyInfo[bApply].bPointType;

		DWORD dwType = AFFECT_EVENT_START + bIndex;

		if (dwType < AFFECT_EVENT_START || dwType > AFFECT_EVENT_END)
		{
			sys_err("affect is out of range : %d", dwType);
			lua_pushnumber(L, 0);
			return 1;
		}

		CAffect * pkAffect = ch->FindAffect(dwType, bPoint);

		if (pkAffect)
		{
			lua_pushnumber(L, pkAffect->lDuration);
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int affect_add_biolog_point(lua_State * L)
	{
		// Index, Bonus, Value, Expire
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		BYTE bIndex = (BYTE) lua_tonumber(L, 1);
		BYTE bPointType = (BYTE) lua_tonumber(L, 2);

		if (bPointType >= POINT_MAX_NUM || bPointType < 1)
		{
			sys_err("point is out of range : %d", bPointType);
			return 0;
		}

		DWORD dwType = AFFECT_BIOLOGIST_START + bIndex;

		if (dwType < AFFECT_BIOLOGIST_START || dwType > AFFECT_BIOLOGIST_END)
		{
			sys_err("affect is out of range : %d", dwType);
			return 0;
		}

		long value = (long) lua_tonumber(L, 3);
		long duration = (long) lua_tonumber(L, 4);

		if (ch->FindAffect(dwType, bPointType))
			return 0;

		ch->AddAffect(dwType, bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int affect_remove_biolog_bonus(lua_State * L)
	{
		// Quest index
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		BYTE bIndex = (BYTE) lua_tonumber(L, 1);

		DWORD dwType = AFFECT_BIOLOGIST_START + bIndex;

		if (dwType < AFFECT_BIOLOGIST_START || dwType > AFFECT_BIOLOGIST_END)
		{
			sys_err("affect is out of range : %d", dwType);
			return 0;
		}

		bool bRemoved = ch->RemoveAffect(dwType);

		if (!bRemoved)
			sys_err("affect for remove not found : %d", dwType);

		return 0;
	}

	int affect_add_event_point(lua_State * L)
	{
		// Index, Bonus, Value, Expire
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		BYTE bIndex = (BYTE) lua_tonumber(L, 1);
		BYTE bPointType = (BYTE) lua_tonumber(L, 2);

		if (bPointType >= POINT_MAX_NUM || bPointType < 1)
		{
			sys_err("point is out of range : %d", bPointType);
			return 0;
		}

		DWORD dwType = AFFECT_EVENT_START + bIndex;

		if (dwType < AFFECT_EVENT_START || dwType > AFFECT_EVENT_END)
		{
			sys_err("affect is out of range : %d", dwType);
			return 0;
		}

		long value = (long) lua_tonumber(L, 3);
		long duration = (long) lua_tonumber(L, 4);

		if (ch->FindAffect(dwType, bPointType))
			return 0;

		ch->AddAffect(dwType, bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int affect_add_new(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		DWORD dwAffType = (DWORD) lua_tonumber(L, 1);
		BYTE applyOn = (BYTE) lua_tonumber(L, 2);
		DWORD dwAffFlag = lua_isnumber(L, 5) ? (DWORD) lua_tonumber(L, 5) : AFF_NONE;

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		if (ch->FindAffect(dwAffType, applyOn)) // Äù½ºÆ®·Î ÀÎÇØ °°Àº °÷¿¡ È¿°ú°¡ °É·ÁÀÖÀ¸¸é ½ºÅµ
			return 0;

		long value = (long) lua_tonumber(L, 3);
		long duration = (long) lua_tonumber(L, 4);

		ch->AddAffect(dwAffType, aApplyInfo[applyOn].bPointType, value, dwAffFlag, duration, 0, false);

		return 0;
	}

	int affect_remove(lua_State * L)
	{
		CQuestManager & q = CQuestManager::instance();
		int iType;

		if (lua_isnumber(L, 1))
		{
			iType = (int) lua_tonumber(L, 1);

			if (iType == 0)
				iType = q.GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;
		}
		else
			iType = q.GetCurrentPC()->GetCurrentQuestIndex() + AFFECT_QUEST_START_IDX;

		q.GetCurrentCharacterPtr()->RemoveAffect(iType);

		return 0;
	}

	int affect_remove_bad(lua_State * L) // ³ª»Û È¿°ú¸¦ ¾ø¾Ú
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveBadAffect();
		return 0;
	}

	int affect_remove_good(lua_State * L) // ÁÁÀº È¿°ú¸¦ ¾ø¾Ú
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveGoodAffect();
		return 0;
	}

	int affect_add_hair(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		BYTE applyOn = (BYTE) lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		long value = (long) lua_tonumber(L, 2);
		long duration = (long) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_HAIR, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int affect_remove_hair(lua_State * L) // Çì¾î È¿°ú¸¦ ¾ø¾Ø´Ù.
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		CAffect* pkAff = ch->FindAffect( AFFECT_HAIR );

		if ( pkAff != NULL )
		{
			lua_pushnumber(L, pkAff->lDuration);
			ch->RemoveAffect( pkAff );
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}
	
	// ÇöÀç Ä³¸¯ÅÍ°¡ AFFECT_TYPE affect¸¦ °®°íÀÖÀ¸¸é bApplyOn °ªÀ» ¹ÝÈ¯ÇÏ°í ¾øÀ¸¸é nilÀ» ¹ÝÈ¯ÇÏ´Â ÇÔ¼ö.
	// usage :	applyOn = affect.get_apply(AFFECT_TYPE) 
	int affect_get_apply_on(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD affectType = lua_tonumber(L, 1);

		CAffect* pkAff = ch->FindAffect(affectType);

		if ( pkAff != NULL )
			lua_pushnumber(L, pkAff->bApplyOn);
		else
			lua_pushnil(L);

		return 1;

	}

	int affect_add_collect(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		BYTE applyOn = (BYTE) lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (applyOn >= MAX_APPLY_NUM || applyOn < 1)
		{
			sys_err("apply is out of range : %d", applyOn);
			return 0;
		}

		long value = (long) lua_tonumber(L, 2);
		long duration = (long) lua_tonumber(L, 3);
		bool overRide = false;
		if (lua_isboolean(L, 4))
			overRide = (bool) lua_toboolean(L, 4);

		if (overRide)
		{
			const std::list<CAffect*>& rList = ch->GetAffectContainer();
			const CAffect* pAffect = NULL;

			for (std::list<CAffect*>::const_iterator iter = rList.begin(); iter != rList.end(); ++iter)
			{
				pAffect = *iter;
				if (pAffect->dwType == AFFECT_COLLECT)
					if (pAffect->bApplyOn == aApplyInfo[applyOn].bPointType)
						break;

				pAffect = NULL;
			}

			if (pAffect)
			{
				value += pAffect->lApplyValue;
				ch->RemoveAffect(const_cast<CAffect*>(pAffect));
			}
		}
		ch->AddAffect(AFFECT_COLLECT, aApplyInfo[applyOn].bPointType, value, 0, duration, 0, false);

		return 0;
	}

	int affect_add_collect_point(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager & q = CQuestManager::instance();

		BYTE point_type = (BYTE) lua_tonumber(L, 1);

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (point_type >= POINT_MAX_NUM || point_type < 1)
		{
			sys_err("point is out of range : %d", point_type);
			return 0;
		}

		long value = (long) lua_tonumber(L, 2);
		long duration = (long) lua_tonumber(L, 3);

		ch->AddAffect(AFFECT_COLLECT, point_type, value, 0, duration, 0, false);

		return 0;
	}

	int affect_remove_collect(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			BYTE bApply = (BYTE)lua_tonumber(L, 1);

			if ( bApply >= MAX_APPLY_NUM ) return 0;

			bApply = aApplyInfo[bApply].bPointType;
			long value = (long)lua_tonumber(L, 2);

			const std::list<CAffect*>& rList = ch->GetAffectContainer();
			const CAffect* pAffect = NULL;

			for ( std::list<CAffect*>::const_iterator iter = rList.begin(); iter != rList.end(); ++iter )
			{
				pAffect = *iter;

				if ( pAffect->dwType == AFFECT_COLLECT )
				{
					if ( pAffect->bApplyOn == bApply && pAffect->lApplyValue == value )
					{
						break;
					}
				}

				pAffect = NULL;
			}

			if ( pAffect != NULL )
			{
				ch->RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}

		return 0;
	}

	int affect_remove_all_collect( lua_State* L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			ch->RemoveAffect(AFFECT_COLLECT);
		}

		return 0;
	}

	void RegisterAffectFunctionTable()
	{
		luaL_reg affect_functions[] =
		{
			{ "add",		affect_add		},
			{ "add_new",		affect_add_new		},
			{ "remove",		affect_remove		},
			{ "remove_bad",	affect_remove_bad	},
			{ "remove_good",	affect_remove_good	},
			{ "add_hair",		affect_add_hair		},
			{ "remove_hair",	affect_remove_hair		},
			{ "add_collect",		affect_add_collect		},
			{ "add_collect_point",		affect_add_collect_point		},
			{ "remove_collect",		affect_remove_collect	},
			{ "remove_all_collect",	affect_remove_all_collect	},
			{ "get_apply_on",	affect_get_apply_on },
			{ "add_event_bonus", affect_add_event_bonus },
			{ "add_biolog_bonus", affect_add_biolog_bonus },
			{ "add_event_point", affect_add_event_point },
			{ "add_biolog_point", affect_add_biolog_point },
			{ "remove_biolog_bonus", affect_remove_biolog_bonus },
			{ "has_event_bonus", affect_has_event_bonus },

			{ NULL,		NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("affect", affect_functions);
	}
};
