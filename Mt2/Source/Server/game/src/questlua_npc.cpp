#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "questmanager.h"
#include "char.h"
#include "party.h"
#include "xmas_event.h"
#include "char_manager.h"
#include "shop_manager.h"
#include "guild.h"
#include "packet.h"
#include "general_manager.h"
#include "desc.h"
#include "sectree_manager.h"
#include "mob_manager.h"

namespace quest
{
	//
	// "npc" lua functions
	//
	int npc_open_shop(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();

		int iShopVnum = 0;

		if (lua_gettop(L) == 1)
		{
			if (lua_isnumber(L, 1))
				iShopVnum = (int) lua_tonumber(L, 1);
		}

		if (q.GetCurrentCharacterPtr() && q.GetCurrentNPCCharacterPtr())
			CShopManager::instance().StartShopping(q.GetCurrentCharacterPtr(), q.GetCurrentNPCCharacterPtr(), iShopVnum);

		return 0;
	}

#ifdef __GAYA_SYSTEM__
	int npc_open_gaya_shop(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		auto pShop = CGeneralManager::instance().GetGayaShop();

		network::GCOutputPacket<network::GCGayaShopOpenPacket> pack;
		for (int i = 0; i < GAYA_SHOP_MAX_NUM; ++i)
			*pack->add_datas() = pShop[i];

		if (ch && ch->GetDesc())
			ch->GetDesc()->Packet(pack);
		return 0;
	}
#endif

	int npc_is_pc(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		lua_pushboolean(L, npc && npc->IsPC());
		return 1;
	}

	int npc_get_hwid(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushstring(L, (npc && npc->IsPC()) ? npc->GetAccountTable().hwid().c_str() : "");
		return 1;
	}
	
	int npc_get_empire(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		lua_pushnumber(L, npc ? npc->GetEmpire() : 0);

		return 1;
	}

	int npc_get_race(lua_State * L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentNPCRace());
		return 1;
	}

	int npc_get_guild(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		CGuild* pGuild = npc ? npc->GetGuild() : NULL;
		lua_pushnumber(L, pGuild ? pGuild->GetID() : 0);
		return 1;
	}

	int npc_is_quest(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (npc)
		{
			const std::string & r_st = q.GetCurrentQuestName();

			if (q.GetQuestIndexByName(r_st) == npc->GetQuestBy())
			{
				lua_pushboolean(L, 1);
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int npc_kill(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		ch->SetQuestNPCID(0);
		if (npc)
		{
			npc->Dead();
		}
		return 0;
	}

	int npc_purge(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (ch)
			ch->SetQuestNPCID(0);
		else
			q.SetCurrentSelectedNPCCharacterPtr(NULL);

		if (npc)
		{
			M2_DESTROY_CHARACTER(npc);
		}
		return 0;
	}

	int npc_is_near(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_Number dist = 10;

		if (lua_isnumber(L, 1))
			dist = lua_tonumber(L, 1);

		lua_pushboolean(L, ch && npc && DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) < dist * 100);

		return 1;
	}

	int npc_is_near_vid(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid vid");
			lua_pushboolean(L, 0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find((DWORD)lua_tonumber(L, 1));
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_Number dist = 10;

		if (lua_isnumber(L, 2))
			dist = lua_tonumber(L, 2);

		lua_pushboolean(L, ch && npc && DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY()) < dist * 100);

		return 1;
	}

	int npc_unlock(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if ( npc != NULL )
		{
			if (npc->IsPC())
				return 0;

			if (npc->GetQuestNPCID() == ch->GetPlayerID())
			{
				npc->SetQuestNPCID(0);
			}
		}
		return 0;
	}

	int npc_lock(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		if (!npc || npc->IsPC())
		{
			lua_pushboolean(L, TRUE);
			return 1;
		}

		if (npc->GetQuestNPCID() == 0 || npc->GetQuestNPCID() == ch->GetPlayerID())
		{
			npc->SetQuestNPCID(ch->GetPlayerID());
			lua_pushboolean(L, TRUE);
		}
		else
		{
			lua_pushboolean(L, FALSE);
		}

		return 1;
	}

	int npc_get_leader_vid(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		LPPARTY party = npc ? npc->GetParty() : NULL;
		LPCHARACTER leader = party ? party->GetLeader() : NULL;

		lua_pushnumber(L, leader ? leader->GetVID() : 0);


		return 1;
	}

	int npc_get_vid(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		
		lua_pushnumber(L, npc ? npc->GetVID() : 0);


		return 1;
	}

	int npc_get_level(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();

		lua_pushnumber(L, npc ? npc->GetLevel() : 0);


		return 1;
	}

	int npc_get_name(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		LPCHARACTER pc = q.GetCurrentCharacterPtr();

		lua_pushstring(L, npc ? CLocaleManager::instance().StringToArgument(npc->GetName(pc ? pc->GetLanguageID() : LANGUAGE_DEFAULT)) : "");

		return 1;
	}

	int npc_select_target(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (!ch)
		{
			quest::CQuestManager::instance().SetCurrentSelectedNPCCharacterPtr(NULL);
			lua_pushnumber(L, 0);
			return 1;
		}

		ch->SetQuestNPCID(0);
		if (ch->GetTarget())
			ch->SetQuestNPCID(ch->GetTarget()->GetVID());

		lua_pushnumber(L, ch->GetQuestNPCID());
		return 1;
	}

	int npc_select_vid(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("unkown argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPCHARACTER pkOldNPC = q.GetCurrentSelectedNPCCharacterPtr();

		if (ch)
		{
			if (ch->GetQuestNPCID() == lua_tonumber(L, 1))
			{
				lua_pushnumber(L, ch->GetQuestNPCID());
				return 1;
			}
		}
		else
		{
			if (pkOldNPC && pkOldNPC->GetVID() == lua_tonumber(L, 1))
			{
				lua_pushnumber(L, pkOldNPC->GetVID());
				return 1;
			}
		}

		LPCHARACTER pkNPC = CHARACTER_MANAGER::Instance().Find(lua_tonumber(L, 1));
		if (pkNPC != NULL)
		{
			if (ch)
			{
				lua_pushnumber(L, ch->GetQuestNPCID());
				ch->SetQuestNPCID(0); // reset (else there will be syserr that you cannot reset quest npc id!)
				ch->SetQuestNPCID(lua_tonumber(L, 1));
			}
			else
			{
				lua_pushnumber(L, pkOldNPC ? pkOldNPC->GetVID() : 0);
				q.SetCurrentSelectedNPCCharacterPtr(pkNPC);
			}
		}
		else
		{
			if (ch)
				ch->SetQuestNPCID(0);
			else
				q.SetCurrentSelectedNPCCharacterPtr(NULL);
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int npc_set_position(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("unkown argument");
			return 0;
		}

		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (npc)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(npc->GetMapIndex(), basePos))
				return 0;
		
			npc->Show(npc->GetMapIndex(), basePos.x + (lua_tonumber(L, 1)*100), basePos.y + (lua_tonumber(L, 2)*100), 0);
		}

		return 0;
	}

	int npc_get_vid_attack_mul(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("unkown argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		lua_pushnumber(L, targetChar ? targetChar->GetAttMul() : 0.0f);


		return 1;
	}
	
	int npc_set_vid_attack_mul(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("unkown argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		lua_Number attack_mul = lua_tonumber(L, 2);

		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			targetChar->SetAttMul(attack_mul);

		return 0;
	}

	int npc_get_vid_damage_mul(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("unkown argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		lua_pushnumber(L, targetChar ? targetChar->GetDamMul() : 0.0f);


		return 1;
	}
	
	int npc_set_vid_damage_mul(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("unkown argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();

		lua_Number vid = lua_tonumber(L, 1);
		lua_Number damage_mul = lua_tonumber(L, 2);

		LPCHARACTER targetChar = CHARACTER_MANAGER::instance().Find(vid);

		if (targetChar)
			targetChar->SetDamMul(damage_mul);

		return 0;
	}

	int npc_get_hp(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (npc)
			lua_pushnumber(L, npc->GetHP());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int npc_set_hp(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (npc)
			npc->PointChange(POINT_HP, lua_tonumber(L, 1) - npc->GetHP());

		return 0;
	}

	int npc_get_max_sp(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (npc)
			lua_pushnumber(L, npc->GetMaxSP());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int npc_set_max_sp(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (npc)
			npc->SetMaxSP(lua_tonumber(L, 1));

		return 0;
	}

	int npc_get_max_hp(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (npc)
			lua_pushnumber(L, npc->GetMaxHP());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int npc_set_aggro(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (npc)
			npc->SetAggressive();

		return 0;
	}

	int npc_is_metin(lua_State* L)
	{
		if (lua_isnumber(L, 1))
		{
			DWORD vnum = lua_tonumber(L, 1);
			const CMob* pMob = CMobManager::instance().Get(vnum);
			if (pMob)
				lua_pushboolean(L, pMob->m_table.type() == CHAR_TYPE_STONE);
			else
				lua_pushboolean(L, 0);
		}
		else
		{
			LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
			if (npc)
				lua_pushboolean(L, npc->GetCharType() == CHAR_TYPE_STONE);
			else
				lua_pushboolean(L, 0);
		}

		return 1;
	}
	
	int npc_is_boss(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (npc)
			lua_pushboolean(L, npc->GetMobRank() >= MOB_RANK_BOSS);
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int npc_get_local_x(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (npc)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(npc->GetMapIndex(), basePos))
			{
				lua_pushnumber(L, 0);
				return 1;
			}

			lua_pushnumber(L, (npc->GetX() - basePos.x) / 100);
		}
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int npc_get_local_y(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		if (npc)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(npc->GetMapIndex(), basePos))
			{
				lua_pushnumber(L, 0);
				return 1;
			}

			lua_pushnumber(L, (npc->GetY() - basePos.y) / 100);
		}
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int npc_get_apply(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || !lua_isnumber(L, 1))
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		BYTE bApplyType = lua_tonumber(L, 1);
		if (bApplyType >= MAX_APPLY_NUM)
		{
			sys_err("npc_get_apply: invalid apply %d", bApplyType);
			lua_pushnumber(L, 0);
			return 1;
		}

		BYTE bPointType = aApplyInfo[bApplyType].bPointType;
		lua_pushnumber(L, npc->GetPoint(bPointType));
		return 1;
	}

	int npc_change_apply(lua_State* L)
	{
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		if (!npc || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushnumber(L, 0);
			return 0;
		}

		BYTE bApplyType = lua_tonumber(L, 1);
		if (bApplyType >= MAX_APPLY_NUM)
		{
			sys_err("npc_get_apply: invalid apply %d", bApplyType);
			return 0;
		}

		BYTE bPointType = aApplyInfo[bApplyType].bPointType;
		int iChange = lua_tonumber(L, 2);
		npc->PointChange(bPointType, iChange);

		return 0;
	}

	void RegisterNPCFunctionTable()
	{
		luaL_reg npc_functions[] = 
		{
			{ "getrace",			npc_get_race			},
			{ "get_race",			npc_get_race			},
			{ "open_shop",			npc_open_shop			},
#ifdef __GAYA_SYSTEM__
			{ "open_gaya_shop",		npc_open_gaya_shop		},
#endif
			{ "get_empire",			npc_get_empire			},
			{ "is_pc",				npc_is_pc			},
			{ "get_hwid",			npc_get_hwid			},
			{ "is_quest",			npc_is_quest			},
			{ "kill",				npc_kill			},
			{ "purge",				npc_purge			},
			{ "is_near",			npc_is_near			},
			{ "is_near_vid",			npc_is_near_vid			},
			{ "lock",				npc_lock			},
			{ "unlock",				npc_unlock			},
			{ "get_guild",			npc_get_guild			},
			{ "get_leader_vid",		npc_get_leader_vid	},
			{ "get_vid",			npc_get_vid		},
			{ "get_level",			npc_get_level	},
			{ "get_name",			npc_get_name	},
			{ "select",				npc_select_vid	},
			{ "select_target",		npc_select_target	},
			{ "get_vid_attack_mul",		npc_get_vid_attack_mul	},
			{ "set_vid_attack_mul",		npc_set_vid_attack_mul	},
			{ "get_vid_damage_mul",		npc_get_vid_damage_mul	},
			{ "set_vid_damage_mul",		npc_set_vid_damage_mul	},

			{ "get_hp",				npc_get_hp			},
			{ "set_hp",				npc_set_hp			},
			{ "get_max_sp",			npc_get_max_sp		},
			{ "set_max_sp",			npc_set_max_sp		},
			{ "get_max_hp",			npc_get_max_hp		},
			{ "set_aggro",			npc_set_aggro		},
			{ "is_metin",			npc_is_metin		},
			{ "is_boss",			npc_is_boss			},

			{ "get_local_x",		npc_get_local_x	},
			{ "get_local_y",		npc_get_local_y	},

			{ "get_apply",			npc_get_apply	},
			{ "change_apply",		npc_change_apply	},
			{ "set_position",		npc_set_position	},
			
			{ NULL,				NULL					}
		};

		CQuestManager::instance().AddLuaFunctionTable("npc", npc_functions);
	}
};
