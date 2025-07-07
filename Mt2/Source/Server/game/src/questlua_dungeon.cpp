#include "stdafx.h"
#include "constants.h"
#include "questmanager.h"
#include "questlua.h"
#include "dungeon.h"
#include "char.h"
#include "party.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "map_location.h"
#include "p2p.h"

#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraDungeon.h"
#endif


#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

template <class Func> Func CDungeon::ForEachMember(Func &f)
{
	itertype(m_set_pkCharacter) it;

	for (it = m_set_pkCharacter.begin(); it != m_set_pkCharacter.end(); ++it)
	{
		sys_log(0, "Dungeon ForEachMember %s", (*it)->GetName());
		f(*it);
	}
	return f;
}

namespace quest
{
	//
	// "dungeon" lua functions
	//
	int dungeon_notice(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Notice(lua_tostring(L, 1));
		return 0;
	}
#ifdef ENABLE_ZODIAC_TEMPLE
	int dungeon_notice_zodiac(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Notice_Zodiac(lua_tostring(L, 1));
		return 0;
	}
#endif
	int dungeon_set_quest_flag(lua_State* L)
	{
		CQuestManager & q = CQuestManager::instance();

		FSetQuestFlag f;

		f.flagname = q.GetCurrentPC()->GetCurrentQuestName() + "." + lua_tostring(L, 1);
		f.value = (int) rint(lua_tonumber(L, 2));

		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ForEachMember(f);

		return 0;
	}

	int dungeon_set_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			sys_err("wrong set flag");
		}
		else
		{
			CQuestManager& q = CQuestManager::instance();
			LPDUNGEON pDungeon = q.GetCurrentDungeon();

			if (pDungeon)
			{
				const char* sz = lua_tostring(L,1);
				int value = int(lua_tonumber(L, 2));
				pDungeon->SetFlag(sz, value);
			}
			else
			{
				sys_err("no dungeon !!!");
			}
		}
		return 0;
	}

	int dungeon_get_flag(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			sys_err("wrong get flag");
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			const char* sz = lua_tostring(L,1);
			lua_pushnumber(L, pDungeon->GetFlag(sz));
		}
		else
		{
			sys_err("no dungeon !!!");
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int dungeon_get_flag_from_map_index(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			sys_err("wrong get flag");
		}

		DWORD dwMapIndex = (DWORD) lua_tonumber(L, 2);
		if (dwMapIndex)
		{
			LPDUNGEON pDungeon = CDungeonManager::instance().FindByMapIndex(dwMapIndex);
			if (pDungeon)
			{
				const char* sz = lua_tostring(L,1);
				lua_pushnumber(L, pDungeon->GetFlag(sz));
			}
			else
			{
				sys_err("no dungeon !!!");
				lua_pushnumber(L, 0);
			}
		}
		else
		{
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int dungeon_get_map_index(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			sys_log(0, "Dungeon GetMapIndex %d",pDungeon->GetMapIndex());
			lua_pushnumber(L, pDungeon->GetMapIndex());
		}
		else
		{
			sys_err("no dungeon !!!");
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int dungeon_regen_file(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			sys_err("wrong filename");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnRegen(lua_tostring(L,1));

		return 0;
	}

	int dungeon_set_regen_file(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			sys_err("wrong filename");
			return 0;
		}
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnRegen(lua_tostring(L,1), false);
		return 0;
	}

	int dungeon_clear_regen(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();
		if (pDungeon)
			pDungeon->ClearRegen();
		return 0;
	}

	int dungeon_check_eliminated(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();
		if (pDungeon)
			pDungeon->CheckEliminated();
		return 0;
	}

	int dungeon_set_exit_all_at_eliminate(lua_State* L)
	{
		if (!lua_isnumber(L,1))
		{
			sys_err("wrong time");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SetExitAllAtEliminate((long)lua_tonumber(L, 1));

		return 0;
	}

	int dungeon_set_warp_at_eliminate(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("wrong time");
			return 0;
		}

		if (!lua_isnumber(L, 2))
		{
			sys_err("wrong map index");
			return 0;
		}

		if (!lua_isnumber(L, 3)) 
		{
			sys_err("wrong X");
			return 0;
		}

		if (!lua_isnumber(L, 4))
		{
			sys_err("wrong Y");
			return 0;
		}

		const char * c_pszRegenFile = NULL;

		if (lua_gettop(L) >= 5)
			c_pszRegenFile = lua_tostring(L,5);

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			pDungeon->SetWarpAtEliminate((long)lua_tonumber(L,1),
										 (long)lua_tonumber(L,2),
										 (long)lua_tonumber(L,3),
										 (long)lua_tonumber(L,4),
										 c_pszRegenFile);
		}
		else
			sys_err("cannot find dungeon");

		return 0;
	}

	int dungeon_new_jump(lua_State* L)
	{
		if (lua_gettop(L) < 3)
		{
			sys_err("not enough argument");
			return 0;
		}

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("wrong argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		long lMapIndex = (long)lua_tonumber(L, 1);
		long lX = lua_tonumber(L, 2);
		long lY = lua_tonumber(L, 3);
		bool bIsLocal = lua_isboolean(L, 4) && lua_toboolean(L, 4);

		if (bIsLocal)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, basePos))
			{
				sys_err("cannot get base pos %d", lMapIndex);
				return 0;
			}

			lX += basePos.x / 100;
			lY += basePos.y / 100;
		}

		LPDUNGEON pDungeon = CDungeonManager::instance().Create(lMapIndex);

		if (!pDungeon)
		{
			long lAddr;
			WORD wPort;
			if (CMapLocation::instance().Get(lMapIndex, lAddr, wPort))
			{
				network::GGOutputPacket<network::GGRequestDungeonWarpPacket> packet;
				packet->set_type(P2P_DUNGEON_WARP_PLAYER);
				packet->set_player_id(ch->GetPlayerID());
				packet->set_map_index(lMapIndex);
				if (P2P_MANAGER::instance().Send(packet, wPort))
					return 0;
			}

			sys_err("cannot create dungeon %d", lMapIndex);
			return 0;
		}

		//ch->WarpSet(pDungeon->GetMapIndex(), (int) lua_tonumber(L, 2), (int)lua_tonumber(L, 3));
		ch->WarpSet(lX * 100, lY * 100, pDungeon->GetMapIndex());
		return 0;
	}

	int dungeon_new_jump_all(lua_State* L)
	{
		if (lua_gettop(L)<3 || !lua_isnumber(L,1) || !lua_isnumber(L, 2) || !lua_isnumber(L,3))
		{
			sys_err("not enough argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		int lMapIndex = (int)lua_tonumber(L,1);
		int lX = (int)lua_tonumber(L,2);
		int lY = (int)lua_tonumber(L,3);
		bool bIsLocal = lua_isboolean(L, 5) && lua_toboolean(L, 5);

		if (bIsLocal)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, basePos))
			{
				sys_err("cannot get base pos %d", lMapIndex);
				return 0;
			}

			lX += basePos.x / 100;
			lY += basePos.y / 100;
		}

		LPDUNGEON pDungeon = CDungeonManager::instance().Create(lMapIndex);

		if (!pDungeon)
		{
			long lAddr;
			WORD wPort;
			if (CMapLocation::instance().Get(lMapIndex, lAddr, wPort))
			{
				network::GGOutputPacket<network::GGRequestDungeonWarpPacket> packet;
				packet->set_type(P2P_DUNGEON_WARP_MAP);
				packet->set_player_id(ch->GetPlayerID());
				packet->set_map_index(lMapIndex);
				packet->set_dest_x(lX * 100);
				packet->set_dest_y(lY * 100);
				if (P2P_MANAGER::instance().Send(packet, wPort))
					return 0;
			}

			sys_err("cannot create dungeon %d", lMapIndex);
			return 0;
		}

		CQuestManager::instance().SelectDungeon(pDungeon);
		pDungeon->JumpAll(lua_isnumber(L, 4) ? lua_tonumber(L, 4) : ch->GetMapIndex(), lX, lY, false);

		return 0;
	}

	int dungeon_new_jump_party(lua_State* L)
	{
		if (lua_gettop(L)<3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("not enough argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch->GetParty() == NULL)
		{
			sys_err("cannot go to dungeon alone.");
			return 0;
		}

		int lMapIndex = (int)lua_tonumber(L, 1);
		int lX = (int)lua_tonumber(L, 2);
		int lY = (int)lua_tonumber(L, 3);
		bool bIsLocal = lua_isboolean(L, 4) && lua_toboolean(L, 4);

		if (bIsLocal)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, basePos))
			{
				sys_err("cannot get base pos %d", lMapIndex);
				return 0;
			}

			lX += basePos.x / 100;
			lY += basePos.y / 100;
		}

		LPDUNGEON pDungeon = CDungeonManager::instance().Create(lMapIndex);

		if (!pDungeon)
		{
			long lAddr;
			WORD wPort;
			if (CMapLocation::instance().Get(lMapIndex, lAddr, wPort))
			{
				network::GGOutputPacket<network::GGRequestDungeonWarpPacket> packet;
				packet->set_type(P2P_DUNGEON_WARP_GROUP);
				packet->set_player_id(ch->GetPlayerID());
				packet->set_map_index(lMapIndex);
				packet->set_dest_x(lX * 100);
				packet->set_dest_y(lY * 100);
				if (P2P_MANAGER::instance().Send(packet, wPort))
					return 0;
			}

			sys_err("cannot create dungeon %d", lMapIndex);
			return 0;
		}

		ch->GetParty()->SetFlag("dungeon_index", pDungeon->GetMapIndex());
		pDungeon->JumpParty(ch->GetParty(), ch->GetMapIndex(), lX, lY);

		return 0;
	}

	int dungeon_jump_all(lua_State* L)
	{
		if (lua_gettop(L)<2 || !lua_isnumber(L, 1) || !lua_isnumber(L,2))
			return 0;

		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (!pDungeon)
			return 0;

		pDungeon->JumpAll(pDungeon->GetMapIndex(), (int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2), !lua_isboolean(L, 3) || lua_toboolean(L, 3));
		return 0;
	}

	int dungeon_jump_all_local(lua_State* L)
	{
		if (lua_gettop(L)<2 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			return 0;

		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (!pDungeon)
			return 0;

		PIXEL_POSITION basePos;
		if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(pDungeon->GetMapIndex(), basePos))
			return 0;

		pDungeon->JumpAll(pDungeon->GetMapIndex(), (int)lua_tonumber(L, 1) + basePos.x / 100, (int)lua_tonumber(L, 2) + basePos.y / 100,
			!lua_isboolean(L, 3) || lua_toboolean(L, 3));
		return 0;
	}

	int dungeon_warp_all(lua_State* L)
	{
		if (lua_gettop(L)<2 || !lua_isnumber(L, 1) || !lua_isnumber(L,2))
			return 0;

		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (!pDungeon)
			return 0;

		pDungeon->WarpAll(pDungeon->GetMapIndex(), (int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2));
		return 0;
	}

	int dungeon_get_kill_stone_count(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L, 0);
			return 1;
		}


		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetKillStoneCount());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int dungeon_get_kill_mob_count(lua_State * L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetKillMobCount());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	int dungeon_is_use_potion(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, 1);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushboolean(L, pDungeon->IsUsePotion());
			return 1;
		}

		lua_pushboolean(L, 1);
		return 1;
	}

	int dungeon_revived(lua_State* L) 
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			lua_pushboolean(L, 1);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushboolean(L, pDungeon->IsUseRevive());
			return 1;
		}

		lua_pushboolean(L, 1);
		return 1;
	}

	int dungeon_set_dest(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		LPPARTY pParty = ch->GetParty();
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon && pParty)
			pDungeon->SendDestPositionToParty(pParty, (int)lua_tonumber(L,1), (int)lua_tonumber(L,2));

		return 0;
	}

	int dungeon_unique_set_maxhp(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->UniqueSetMaxHP(lua_tostring(L,1), (int)lua_tonumber(L,2));

		return 0;
	}

	int dungeon_unique_set_hp(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->UniqueSetHP(lua_tostring(L,1), (int)lua_tonumber(L,2));

		return 0;
	}

	int dungeon_unique_set_def_grade(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->UniqueSetDefGrade(lua_tostring(L,1), (int)lua_tonumber(L,2));

		return 0;
	}

	int dungeon_unique_get_hp_perc(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			lua_pushnumber(L,0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetUniqueHpPerc(lua_tostring(L,1)));
			return 1;
		}

		lua_pushnumber(L,0);
		return 1;
	}

	int dungeon_is_unique_dead(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushboolean(L, pDungeon->IsUniqueDead(lua_tostring(L,1))?1:0);
			return 1;
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int dungeon_purge_unique(lua_State* L)
	{
		if (!lua_isstring(L,1))
			return 0;
		sys_log(0,"QUEST_DUNGEON_PURGE_UNIQUE %s", lua_tostring(L,1));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->PurgeUnique(lua_tostring(L,1));

		return 0;
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
					
				if (pChar->IsPurgeable() && (true == pChar->IsMonster() || true == pChar->IsStone()) && !pChar->IsDead())
				{
					if (x1 <= pChar->GetX() && pChar->GetX() <= x2 && y1 <= pChar->GetY() && pChar->GetY() <= y2)
					{
						M2_DESTROY_CHARACTER(pChar);
					}
				}
			}
		}
	};
	
	int dungeon_purge_area(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2) || !lua_isnumber(L,3) || !lua_isnumber(L,4))
			return 0;
		sys_log(0,"QUEST_DUNGEON_PURGE_AREA");

		int x1 = lua_tonumber(L, 1);
		int y1 = lua_tonumber(L, 2);
		int x2 = lua_tonumber(L, 3);
		int y2 = lua_tonumber(L, 4);
		bool bIsLocal = lua_isboolean(L, 5) ? lua_toboolean(L, 5) : false;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		const int mapIndex = pDungeon->GetMapIndex();

		if (0 == mapIndex)
		{
			sys_err("_purge_area: cannot get a map index with (%u, %u)", x1, y1);
			return 0;
		}

		LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(mapIndex);

		if (NULL != pSectree)
		{
			if (bIsLocal)
			{
				PIXEL_POSITION kBasePos;
				if (SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(mapIndex, kBasePos))
				{
					x1 = x1 * 100 + kBasePos.x;
					x2 = x2 * 100 + kBasePos.x;
					y1 = y1 * 100 + kBasePos.y;
					y2 = y2 * 100 + kBasePos.y;
				}
				else
					sys_err("cannot get map base position by index %d", mapIndex);
			}

			FPurgeArea func(x1, y1, x2, y2, CQuestManager::instance().GetCurrentNPCCharacterPtr());

			pSectree->for_each(func);
		}

		return 0;
	}

	int dungeon_kill_unique(lua_State* L)
	{
		if (!lua_isstring(L,1))
			return 0;
		sys_log(0,"QUEST_DUNGEON_KILL_UNIQUE %s", lua_tostring(L,1));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->KillUnique(lua_tostring(L,1));

		return 0;
	}

	int dungeon_spawn_stone_door(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
			return 0;
		sys_log(0,"QUEST_DUNGEON_SPAWN_STONE_DOOR %s %s", lua_tostring(L,1), lua_tostring(L,2));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnStoneDoor(lua_tostring(L,1), lua_tostring(L,2));

		return 0;
	}

	int dungeon_spawn_wooden_door(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
			return 0;
		sys_log(0,"QUEST_DUNGEON_SPAWN_WOODEN_DOOR %s %s", lua_tostring(L,1), lua_tostring(L,2));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnWoodenDoor(lua_tostring(L,1), lua_tostring(L,2));

		return 0;
	}

	int dungeon_spawn_move_group(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isstring(L,2) || !lua_isstring(L,3))
			return 0;
		sys_log(0,"QUEST_DUNGEON_SPAWN_MOVE_GROUP %d %s %s", (int)lua_tonumber(L,1), lua_tostring(L,2), lua_tostring(L,3));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnMoveGroup((DWORD)lua_tonumber(L,1), lua_tostring(L,2), lua_tostring(L,3));

		return 0;
	}

	int dungeon_spawn_move_unique(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2) || !lua_isstring(L,3) || !lua_isstring(L,4))
			return 0;
		sys_log(0,"QUEST_DUNGEON_SPAWN_MOVE_UNIQUE %s %d %s %s", lua_tostring(L,1), (int)lua_tonumber(L,2), lua_tostring(L,3), lua_tostring(L,4));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnMoveUnique(lua_tostring(L,1), (DWORD)lua_tonumber(L,2), lua_tostring(L,3), lua_tostring(L,4));

		return 0;
	}

	int dungeon_spawn_unique(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2) || !lua_isstring(L,3))
			return 0;
		sys_log(0,"QUEST_DUNGEON_SPAWN_UNIQUE %s %d %s", lua_tostring(L,1), (int)lua_tonumber(L,2), lua_tostring(L,3));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnUnique(lua_tostring(L,1), (DWORD)lua_tonumber(L,2), lua_tostring(L,3));

		return 0;
	}

	int dungeon_spawn(lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isstring(L,2))
			return 0;
		sys_log(0,"QUEST_DUNGEON_SPAWN %d %s", (int)lua_tonumber(L,1), lua_tostring(L,2));

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Spawn((DWORD)lua_tonumber(L,1), lua_tostring(L,2));

		return 0;
	}

	int dungeon_set_unique(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		DWORD vid = (DWORD) lua_tonumber(L, 2);

		if (pDungeon)
			pDungeon->SetUnique(lua_tostring(L, 1), vid);
		return 0;
	}

	int dungeon_get_unique_vid(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			lua_pushnumber(L,0);
			return 1;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			lua_pushnumber(L, pDungeon->GetUniqueVid(lua_tostring(L,1)));
			return 1;
		}

		lua_pushnumber(L,0);
		return 1;
	}

	int dungeon_spawn_mob(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		DWORD vid = 0;

		if (pDungeon)
		{
			DWORD dwVnum = (DWORD) lua_tonumber(L, 1);
			long x = (long) lua_tonumber(L, 2);
			long y = (long) lua_tonumber(L, 3);
			float radius = lua_isnumber(L, 4) ? (float) lua_tonumber(L, 4) : 0;
			DWORD count = (lua_isnumber(L, 5)) ? (DWORD) lua_tonumber(L, 5) : 1;

			sys_log(0, "dungeon_spawn_mob %u %d %d", dwVnum, x, y);

			if (count == 0)
				count = 1;

			while (count --)
			{
				if (radius<1)
				{
					LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, x, y);
					if (ch && !vid)
						vid = ch->GetVID();
				}
				else
				{
					float angle = random_number(0, 999) * M_PI * 2 / 1000;
					float r = random_number(0, 999) * radius / 1000;

					long nx = x + (long)(r * cos(angle));
					long ny = y + (long)(r * sin(angle));

					LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, nx, ny);
					if (ch && !vid)
						vid = ch->GetVID();
				}
			}
		}

		lua_pushnumber(L, vid);
		return 1;
	}

	int dungeon_spawn_mob_dir(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		DWORD vid = 0;

		if (pDungeon)
		{
			DWORD dwVnum = (DWORD) lua_tonumber(L, 1);
			long x = (long) lua_tonumber(L, 2);
			long y = (long) lua_tonumber(L, 3);
			BYTE dir = (int) lua_tonumber(L, 4);
			
			LPCHARACTER ch = pDungeon->SpawnMob(dwVnum, x, y, dir);
			if (ch && !vid)
				vid = ch->GetVID();
		}
		lua_pushnumber(L, vid);
		return 1;
	}
	
	int dungeon_spawn_mob_ac_dir(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
		{
			sys_err("invalid argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		DWORD vid = 0;

		if (pDungeon)
		{
			DWORD dwVnum = (DWORD) lua_tonumber(L, 1);
			long x = (long) lua_tonumber(L, 2);
			long y = (long) lua_tonumber(L, 3);
			BYTE dir = (int) lua_tonumber(L, 4);
			
			LPCHARACTER ch = pDungeon->SpawnMob_ac_dir(dwVnum, x, y, dir);
			if (ch && !vid)
				vid = ch->GetVID();
		}
		lua_pushnumber(L, vid);
		return 1;
	}

	int dungeon_spawn_goto_mob(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
			return 0;

		long lFromX = (long)lua_tonumber(L, 1);
		long lFromY = (long)lua_tonumber(L, 2);
		long lToX   = (long)lua_tonumber(L, 3);
		long lToY   = (long)lua_tonumber(L, 4);

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->SpawnGotoMob(lFromX, lFromY, lToX, lToY);

		return 0;
	}

	int dungeon_spawn_name_mob(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isstring(L, 4))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			DWORD dwVnum = (DWORD) lua_tonumber(L, 1);
			long x = (long)lua_tonumber(L, 2);
			long y = (long)lua_tonumber(L, 3);
			pDungeon->SpawnNameMob(dwVnum, x, y, lua_tostring(L, 4));
		}
		return 0;
	}

	int dungeon_spawn_group(lua_State* L)
	{
		//
		// argument: vnum,x,y,radius,aggressive,count
		//
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4) || !lua_isnumber(L, 6))
		{
			sys_err("invalid argument");
			return 0;
		}

		DWORD vid = 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			DWORD group_vnum = (DWORD)lua_tonumber(L, 1);
			long local_x = (long) lua_tonumber(L, 2) * 100;
			long local_y = (long) lua_tonumber(L, 3) * 100;
			float radius = (float) lua_tonumber(L, 4) * 100;
			bool bAggressive = lua_toboolean(L, 5);
			DWORD count = (DWORD) lua_tonumber(L, 6);

			LPCHARACTER chRet = pDungeon->SpawnGroup(group_vnum, local_x, local_y, radius, bAggressive, count);
			if (chRet)
				vid = chRet->GetVID();
		}

		lua_pushnumber(L, vid);
		return 1;
	}

	int dungeon_join(lua_State* L)
	{
		if (lua_gettop(L) < 1 || !lua_isnumber(L, 1))
			return 0;

		long lMapIndex = (long)lua_tonumber(L, 1);
		LPDUNGEON pDungeon = CDungeonManager::instance().Create(lMapIndex);

		if (!pDungeon)
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->GetParty() && ch->GetParty()->GetLeaderPID() == ch->GetPlayerID())
			pDungeon->JoinParty(ch->GetParty());
		else if (!ch->GetParty())
			pDungeon->Join(ch);

		return 0;
	}

	int dungeon_exit(lua_State* L) // ´øÀü¿¡ µé¾î¿À±â Àü À§Ä¡·Î º¸³¿
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		ch->ExitToSavedLocation();
		return 0;
	}

	int dungeon_exit_all(lua_State* L) // ´øÀü¿¡ ÀÖ´Â ¸ðµç »ç¶÷À» ´øÀü¿¡ µé¾î¿À±â Àü À§Ä¡·Î º¸³¿
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ExitAll();

		return 0;
	}

	struct FSayDungeonByItemGroup
	{
		const CDungeon::ItemGroup* item_group;
		std::string can_enter_ment;
		std::string cant_enter_ment;
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch->IsPC())
				{
					network::GCOutputPacket<network::GCScriptPacket> packet_script;
					packet_script->set_skin(quest::CQuestManager::QUEST_SKIN_NORMAL);
					
					for (CDungeon::ItemGroup::const_iterator it = item_group->begin(); it != item_group->end(); it++)
					{
						if(ch->CountSpecifyItem(it->first) >= it->second)
						{
							packet_script->set_script(can_enter_ment);
							ch->GetDesc()->Packet(packet_script);
							return;
						}
					}

					packet_script->set_script(cant_enter_ment);
					ch->GetDesc()->Packet(packet_script);
				}
			}
		}
	};


	int dungeon_say_diff_by_item_group(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isstring(L, 3))
		{
			sys_log(0, "QUEST wrong set flag");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		CDungeon * pDungeon = q.GetCurrentDungeon();

		if(!pDungeon)
		{
			sys_err("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP * pMap = SECTREE_MANAGER::instance().GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			sys_err("cannot find map by index %d", pDungeon->GetMapIndex());
			return 0;
		}
		FSayDungeonByItemGroup f;
		sys_log (0,"diff_by_item");
	
		std::string group_name (lua_tostring (L, 1));
		f.item_group = pDungeon->GetItemGroup (group_name);

		if (f.item_group == NULL)
		{
			sys_err ("invalid item group");
			return 0;
		}

		f.can_enter_ment = lua_tostring(L, 2);
		f.can_enter_ment+= "{ENTER}{ENTER}{ENTER}{ENTER}{DONE}";
		f.cant_enter_ment = lua_tostring(L, 3);
		f.cant_enter_ment+= "{ENTER}{ENTER}{ENTER}{ENTER}{DONE}";

		pMap -> for_each( f );

		return 0;
	}
	
	struct FExitDungeonByItemGroup
	{
		const CDungeon::ItemGroup* item_group;
		
		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch->IsPC())
				{
					for (CDungeon::ItemGroup::const_iterator it = item_group->begin(); it != item_group->end(); it++)
					{
						if(ch->CountSpecifyItem(it->first) >= it->second)
						{
							return;
						}
					}
					LPDUNGEON dungeon = ch->GetDungeonForce();
					if (dungeon)
					{
						dungeon->SetFlag(ch->GetName(), 1);
						dungeon->SkipPlayerSaveDungeonOnce();
					}
					ch->ExitToSavedLocation();
				}
			}
		}
	};
	
	int dungeon_exit_all_by_item_group (lua_State* L) // Æ¯Á¤ ¾ÆÀÌÅÛ ±×·ì¿¡ ¼ÓÇÑ ¾ÆÀÌÅÛÀÌ ¾ø´Â»ç¶÷Àº °­Åð
	{
		if (!lua_isstring(L, 1))
		{
			sys_log(0, "QUEST wrong set flag");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		CDungeon * pDungeon = q.GetCurrentDungeon();

		if(!pDungeon)
		{
			sys_err("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP * pMap = SECTREE_MANAGER::instance().GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			sys_err("cannot find map by index %d", pDungeon->GetMapIndex());
			return 0;
		}
		FExitDungeonByItemGroup f;

		std::string group_name (lua_tostring (L, 1));
		f.item_group = pDungeon->GetItemGroup (group_name);
	
		if (f.item_group == NULL)
		{
			sys_err ("invalid item group");
			return 0;
		}
		
		pMap -> for_each( f );
		
		return 0;
	}

	struct FDeleteItemInItemGroup
	{
		const CDungeon::ItemGroup* item_group;

		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (ch->IsPC())
				{
					for (CDungeon::ItemGroup::const_iterator it = item_group->begin(); it != item_group->end(); it++)
					{
						if(ch->CountSpecifyItem(it->first) >= it->second)
						{
							ch->RemoveSpecifyItem (it->first, it->second);
							return;
						}
					}
				}
			}
		}
	};
	
	int dungeon_delete_item_in_item_group_from_all(lua_State* L) // Æ¯Á¤ ¾ÆÀÌÅÛÀ» ´øÀü ³» pc¿¡°Ô¼­ »èÁ¦.
	{
		if (!lua_isstring(L, 1))
		{
			sys_log(0, "QUEST wrong set flag");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		CDungeon * pDungeon = q.GetCurrentDungeon();

		if(!pDungeon)
		{
			sys_err("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP * pMap = SECTREE_MANAGER::instance().GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			sys_err("cannot find map by index %d", pDungeon->GetMapIndex());
			return 0;
		}
		FDeleteItemInItemGroup f;

		std::string group_name (lua_tostring (L, 1));
		f.item_group = pDungeon->GetItemGroup (group_name);

		if (f.item_group == NULL)
		{
			sys_err ("invalid item group");
			return 0;
		}

		pMap -> for_each( f );
		
		return 0;
	}


	int dungeon_kill_all(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->KillAll(lua_isboolean(L, 1) ? lua_toboolean(L, 1) : false);

		return 0;
	}

	int dungeon_purge(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->Purge();

		return 0;
	}

	int dungeon_exit_all_to_start_position(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ExitAllToStartPosition();

		return 0;
	}

	int dungeon_count_monster(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			lua_pushnumber(L, pDungeon->CountMonster());
		else
		{
			sys_err("not in a dungeon");
			lua_pushnumber(L, LONG_MAX);
		}

		return 1;
	}

	int dungeon_count_alive_monster(lua_State * L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			lua_pushnumber(L, pDungeon->CountAliveMonster());
		else
		{
			sys_err("not in a dungeon");
			lua_pushnumber(L, LONG_MAX);
		}

		return 1;
	}

	int dungeon_select(lua_State* L)
	{
		DWORD dwMapIndex = (DWORD) lua_tonumber(L, 1);
		if (dwMapIndex)
		{
			LPDUNGEON pDungeon = CDungeonManager::instance().FindByMapIndex(dwMapIndex);
			if (pDungeon)
			{
				CQuestManager::instance().SelectDungeon(pDungeon);
				lua_pushboolean(L, 1);
			}
			else
			{
				CQuestManager::instance().SelectDungeon(NULL);
				lua_pushboolean(L, 0);
			}
		}
		else
		{
			CQuestManager::instance().SelectDungeon(NULL);
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int dungeon_find(lua_State* L)
	{
		DWORD dwMapIndex = (DWORD) lua_tonumber(L, 1);
		if (dwMapIndex)
		{
			LPDUNGEON pDungeon = CDungeonManager::instance().FindByMapIndex(dwMapIndex);
			if (pDungeon)
			{
				lua_pushboolean(L, 1);
			}
			else
			{
				lua_pushboolean(L, 0);
			}
		}
		else
		{
			lua_pushboolean(L, 0);
		}
		return 1;
	}

	int dungeon_all_near_to( lua_State* L)
	{
		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (pDungeon != NULL)
		{
			lua_pushboolean(L, pDungeon->IsAllPCNearTo( (int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2), 30));
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int dungeon_set_warp_location (lua_State* L)
	{
		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (pDungeon == NULL)
		{
			return 0;
		}
		
		if (lua_gettop(L)<3 || !lua_isnumber(L, 1) || !lua_isnumber(L,2) || !lua_isnumber(L, 3))
		{
			return 0;
		}

		int lMapIndex = (int)lua_tonumber(L, 1);
		int lX = (int)lua_tonumber(L, 2);
		int lY = (int)lua_tonumber(L, 3);
		bool bIsLocal = lua_isboolean(L, 4) && lua_toboolean(L, 4);

		if (bIsLocal)
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, basePos))
			{
				sys_err("cannot get base pos %d", lMapIndex);
				return 0;
			}

			lX += basePos.x / 100;
			lY += basePos.y / 100;
		}

		FSetWarpLocation f (lMapIndex, lX, lY);
		pDungeon->ForEachMember (f);

		return 0;
	}

	int dungeon_set_warp_location_local(lua_State* L)
	{
		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (pDungeon == NULL)
		{
			return 0;
		}

		if (lua_gettop(L)<3 || !lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			return 0;
		}

		PIXEL_POSITION basePos;
		SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lua_tonumber(L, 1), basePos);

		FSetWarpLocation f((int)lua_tonumber(L, 1), basePos.x / 100 + (int)lua_tonumber(L, 2), basePos.y / 100 + (int)lua_tonumber(L, 3));
		pDungeon->ForEachMember(f);

		return 0;
	}

	int dungeon_set_item_group (lua_State* L)
	{
		if (!lua_isstring (L, 1) || !lua_isnumber (L, 2))
		{
			return 0;
		}
		std::string group_name (lua_tostring (L, 1));
		int size = lua_tonumber (L, 2);

		CDungeon::ItemGroup item_group;
		
		for (int i = 0; i < size; i++)
		{
			if (!lua_isnumber (L, i * 2 + 3) || !lua_isnumber (L, i * 2 + 4))
			{
				return 0;
			}
			item_group.push_back (std::pair <DWORD, int> (lua_tonumber (L, i * 2 + 3), lua_tonumber (L, i * 2 + 4)));
		}
		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();

		if (pDungeon == NULL)
		{
			return 0;
		}
		
		pDungeon->CreateItemGroup (group_name, item_group);
		return 0;
	}

	int dungeon_set_quest_flag2(lua_State* L)
	{
		CQuestManager & q = CQuestManager::instance();

		FSetQuestFlag f;

		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("Invalid Argument");
		}

		f.flagname = string (lua_tostring(L, 1)) + "." + lua_tostring(L, 2);
		f.value = (int) rint(lua_tonumber(L, 3));

		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->ForEachMember(f);

		return 0;
	}

	struct FDungeonPIDCollector
	{
		std::vector <DWORD> vecPIDs;
		FDungeonPIDCollector()
		{
		}
		void operator () (LPCHARACTER ch)
		{
			vecPIDs.push_back(ch->GetPlayerID());
		}
	};
	int dungeon_get_pids(lua_State* L)
	{
		CQuestManager & q = CQuestManager::instance();
		LPDUNGEON pkDungeon = q.GetCurrentDungeon();
		if (!pkDungeon)
			return 0;

		FDungeonPIDCollector f;
		pkDungeon->ForEachMember(f);

		for (std::vector <DWORD>::iterator it = f.vecPIDs.begin(); it != f.vecPIDs.end(); it++)
		{
			lua_pushnumber(L, *it);
		}

		return f.vecPIDs.size();
	}

#ifdef __RIFT_SYSTEM__
	int dungeon_set_enemy_hp_factor(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		CQuestManager & q = CQuestManager::instance();
		LPDUNGEON pkDungeon = q.GetCurrentDungeon();
		if (!pkDungeon)
			return 0;

		pkDungeon->SetEnemyHPFactor(lua_tonumber(L, 1));
		return 0;
	}

	int dungeon_set_enemy_dam_factor(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		CQuestManager & q = CQuestManager::instance();
		LPDUNGEON pkDungeon = q.GetCurrentDungeon();
		if (!pkDungeon)
			return 0;

		pkDungeon->SetEnemyDamFactor(lua_tonumber(L, 1));
		return 0;
	}
#endif

	struct FExitDungeonByLevel
	{
		BYTE bMinLevel;
		const char* szMessage;

		FExitDungeonByLevel()
		{
			bMinLevel = 0;
			szMessage = NULL;
		}

		void operator()(LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER)ent;

				if (ch->IsPC())
				{
					if (ch->GetLevel() >= bMinLevel)
						return;
					if (szMessage)
						ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(ch, szMessage));
					ch->ExitToSavedLocation();
				}
			}
		}
	};

	int dungeon_exit_all_by_level(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		CDungeon * pDungeon = q.GetCurrentDungeon();

		if (!pDungeon)
		{
			sys_err("QUEST : no dungeon");
			return 0;
		}

		SECTREE_MAP * pMap = SECTREE_MANAGER::instance().GetMap(pDungeon->GetMapIndex());

		if (!pMap)
		{
			sys_err("cannot find map by index %d", pDungeon->GetMapIndex());
			return 0;
		}
		FExitDungeonByLevel f;
		
		f.bMinLevel = lua_tonumber(L, 1);
		if (lua_isstring(L, 2))
			f.szMessage = lua_tostring(L, 2);
		pMap->for_each(f);

		return 0;
	}

	int dungeon_cmdchat(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
			pDungeon->CmdChat(lua_tostring(L, 1));
		return 0;
	}

	int dungeon_move_all_monster_to_player(lua_State* L)
	{
		CQuestManager & q = CQuestManager::instance();
		LPDUNGEON pkDungeon = q.GetCurrentDungeon();
		if (!pkDungeon)
			return 0;

		pkDungeon->MoveAllMonsterToPlayer();

		return 0;
	}
	
	int dungeon_set_string_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
		{
			sys_err("wrong set flag");
		}
		else
		{
			CQuestManager& q = CQuestManager::instance();
			LPDUNGEON pDungeon = q.GetCurrentDungeon();

			if (pDungeon)
			{
				const char* sz = lua_tostring(L,1);
				const char* value = lua_tostring(L,2);
				pDungeon->SetStringFlag(sz, value);
			}
			else
			{
				sys_err("no dungeon !!!");
			}
		}
		return 0;
	}

	int dungeon_get_string_flag(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			sys_err("wrong get flag");
		}

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			const char* sz = lua_tostring(L,1);
			lua_pushstring(L, pDungeon->GetStringFlag(sz));
		}
		else
		{
			sys_err("no dungeon !!!");
			lua_pushstring(L, "");
		}

		return 1;
	}

	int dungeon_get_monster_count(lua_State* L)
	{
		LPDUNGEON pDungeon = CQuestManager::instance().GetCurrentDungeon();
		if (!pDungeon)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		int iMonsterVnum = lua_tonumber(L, 1);
		int iCount = SECTREE_MANAGER::instance().GetMonsterCountInMap(pDungeon->GetMapIndex(), iMonsterVnum, true);

		lua_pushnumber(L, iCount);
		return 1;
	}

	int dungeon_unique_has_aff(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			LPCHARACTER pkChr = pDungeon->UniqueGet(lua_tostring(L, 1));
			if (pkChr)
			{
				lua_pushboolean(L, pkChr->IsAffectFlag(lua_tonumber(L, 2)));
				return 1;
			}
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int dungeon_unique_add_affect(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3) || !lua_isnumber(L, 4))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			LPCHARACTER pkChr = pDungeon->UniqueGet(lua_tostring(L, 1));
			if (pkChr)
			{
				pkChr->AddAffect(lua_tonumber(L, 2), POINT_NONE, 0, lua_tonumber(L, 3), lua_tonumber(L, 4), 0, true);
			}
		}

		return 0;
	}

	int dungeon_unique_remove_affect(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			LPCHARACTER pkChr = pDungeon->UniqueGet(lua_tostring(L, 1));
			if (pkChr)
			{
				pkChr->RemoveAffect(lua_tonumber(L, 2));
			}
		}

		return 0;
	}

	int dungeon_unique_set_rotation_to(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			LPCHARACTER pkChr = pDungeon->UniqueGet(lua_tostring(L, 1));
			if (pkChr)
			{
				PIXEL_POSITION pxBase;
				SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(pkChr->GetMapIndex(), pxBase);

				pkChr->SetRotationToXY(pxBase.x + lua_tonumber(L, 2) * 100, pxBase.y + lua_tonumber(L, 3) * 100);
			}
		}

		return 0;
	}

	int dungeon_unique_send_skill(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			LPCHARACTER pkChr = pDungeon->UniqueGet(lua_tostring(L, 1));
			if (pkChr)
			{
				pkChr->SendMovePacket(FUNC_MOB_SKILL, 0, pkChr->GetX(), pkChr->GetY(), 0, get_dword_time());
			}
		}

		return 0;
	}

	int dungeon_unique_set_armada(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();

		if (pDungeon)
		{
			LPCHARACTER pkChr = pDungeon->UniqueGet(lua_tostring(L, 1));
			if (pkChr)
			{
				pkChr->SetArmada();
			}
		}

		return 0;
	}

#ifdef ENABLE_HYDRA_DUNGEON
	int dungeon_create_hydra(lua_State* L)
	{
		lua_pushboolean(L, CHydraDungeonManager::instance().CreateDungeon(CQuestManager::instance().GetCurrentCharacterPtr()));
		return 1;
	}
#endif

	int dungeon_completed(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPDUNGEON pDungeon = q.GetCurrentDungeon();
		if (pDungeon)
			pDungeon->Completed();
		return 1;
	}

	void RegisterDungeonFunctionTable() 
	{
		luaL_reg dungeon_functions[] = 
		{
			{ "join",			dungeon_join		},
			{ "exit",			dungeon_exit		},
			{ "exit_all",		dungeon_exit_all	},
			{ "set_item_group",	dungeon_set_item_group	},
			{ "exit_all_by_item_group",	dungeon_exit_all_by_item_group},
			{ "say_diff_by_item_group",	dungeon_say_diff_by_item_group},
			{ "delete_item_in_item_group_from_all", dungeon_delete_item_in_item_group_from_all},
			{ "purge",			dungeon_purge		},
			{ "kill_all",		dungeon_kill_all	},
			{ "spawn",			dungeon_spawn		},
			{ "spawn_mob",		dungeon_spawn_mob	},
			{ "spawn_mob_dir",	dungeon_spawn_mob_dir	},
			{ "spawn_mob_ac_dir",	dungeon_spawn_mob_ac_dir	},
			{ "spawn_name_mob",	dungeon_spawn_name_mob	},
			{ "spawn_goto_mob",		dungeon_spawn_goto_mob	},
			{ "spawn_group",		dungeon_spawn_group	},
			{ "spawn_unique",		dungeon_spawn_unique	},
			{ "spawn_move_unique",		dungeon_spawn_move_unique},
			{ "spawn_move_group",		dungeon_spawn_move_group},
			{ "spawn_stone_door",		dungeon_spawn_stone_door},
			{ "spawn_wooden_door",		dungeon_spawn_wooden_door},
			{ "purge_unique",		dungeon_purge_unique	},
			{ "purge_area",			dungeon_purge_area	},
			{ "kill_unique",		dungeon_kill_unique	},
			{ "is_unique_dead",		dungeon_is_unique_dead	},
			{ "unique_get_hp_perc",		dungeon_unique_get_hp_perc},
			{ "unique_set_def_grade",	dungeon_unique_set_def_grade},
			{ "unique_set_hp",		dungeon_unique_set_hp	},
			{ "unique_set_maxhp",		dungeon_unique_set_maxhp},
			{ "get_unique_vid",		dungeon_get_unique_vid},
			{ "get_kill_stone_count",	dungeon_get_kill_stone_count},
			{ "get_kill_mob_count",		dungeon_get_kill_mob_count},
			{ "is_use_potion",		dungeon_is_use_potion	},
			{ "revived",			dungeon_revived		},
			{ "set_dest",			dungeon_set_dest	},
			{ "jump_all",			dungeon_jump_all	},
			{ "jump_all_local",		dungeon_jump_all_local	},
			{ "warp_all",		dungeon_warp_all	},
			{ "new_jump_all",		dungeon_new_jump_all	},
			{ "new_jump_party",		dungeon_new_jump_party	},
			{ "new_jump",			dungeon_new_jump	},
			{ "regen_file",			dungeon_regen_file	},
			{ "set_regen_file",		dungeon_set_regen_file	},
			{ "clear_regen",		dungeon_clear_regen	},
			{ "set_exit_all_at_eliminate",	dungeon_set_exit_all_at_eliminate},
			{ "set_warp_at_eliminate",	dungeon_set_warp_at_eliminate},
			{ "get_map_index",		dungeon_get_map_index	},
			{ "check_eliminated",		dungeon_check_eliminated},
			{ "exit_all_to_start_position",	dungeon_exit_all_to_start_position },
			{ "count_monster",		dungeon_count_monster	},
			{ "count_alive_monster",	dungeon_count_alive_monster	},
			{ "setf",					dungeon_set_flag	},
			{ "getf",					dungeon_get_flag	},
			{ "getf_from_map_index",	dungeon_get_flag_from_map_index	},
			{ "set_unique",			dungeon_set_unique	},
			{ "select",			dungeon_select		},
			{ "find",			dungeon_find		},
			{ "notice",			dungeon_notice		},
#ifdef ENABLE_ZODIAC_TEMPLE
			{ "notice_zodiac",			dungeon_notice_zodiac		},
#endif
			{ "setqf",			dungeon_set_quest_flag	},
			{ "all_near_to",	dungeon_all_near_to	},
			{ "set_warp_location",	dungeon_set_warp_location	},
			{ "set_warp_location_local",	dungeon_set_warp_location_local	},
			{ "setqf2",			dungeon_set_quest_flag2	},
			{ "get_pids",		dungeon_get_pids	},

#ifdef __RIFT_SYSTEM__
			{ "set_enemy_hp_factor",	dungeon_set_enemy_hp_factor		},
			{ "set_enemy_dam_factor",	dungeon_set_enemy_dam_factor	},
#endif
			{ "exit_all_by_level",	dungeon_exit_all_by_level	},
			{ "move_all_monster_to_player",	dungeon_move_all_monster_to_player },
			{ "cmdchat",		dungeon_cmdchat		},
			{ "setsf",					dungeon_set_string_flag	},
			{ "getsf",					dungeon_get_string_flag	},

			{ "get_monster_count",		dungeon_get_monster_count	},
			{ "unique_add_affect",		dungeon_unique_add_affect	},
			{ "unique_has_aff",			dungeon_unique_has_aff	},
			{ "unique_remove_affect",	dungeon_unique_remove_affect	},
			{ "unique_send_skill",		dungeon_unique_send_skill	},
			{ "unique_set_armada",		dungeon_unique_set_armada	},
			{ "unique_set_rotation_to",	dungeon_unique_set_rotation_to	},

#ifdef ENABLE_HYDRA_DUNGEON
			{ "create_hydra",			dungeon_create_hydra },
#endif
			{ "dungeon_completed",				dungeon_completed },
			
			{ NULL,				NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("d", dungeon_functions);
	}
}
