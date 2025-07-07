
#include "stdafx.h"

#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "item.h"
#include "item_manager.h"
#include "guild_manager.h"
#include "war_map.h"
#include "start_position.h"
#include "marriage.h"
#include "mining.h"
#include "p2p.h"
#include "polymorph.h"
#include "desc_client.h"
#include "messenger_manager.h"
#include "log.h"
#include "utils.h"
#include "unique_item.h"
#include "mob_manager.h"
#include "gm.h"

#include "vector.h"

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif
#include "shop_manager.h"

#include <cctype>
#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

extern LPITEM FindItemVictim(LPCHARACTER pkChr, int iMaxDistance);

extern int g_nPortalLimitTime;
extern LPCLIENT_DESC db_clientdesc;

extern void RefreshTimerCDRs_G(LPCHARACTER ch);

namespace quest 
{
	//
	// "pc" Lua functions
	//
	int pc_has_master_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		bool bHasMasterSkill = false;
		for (int i=0; i< SKILL_MAX_NUM; i++)
			if (ch->GetSkillMasterType(i) >= SKILL_MASTER && ch->GetSkillLevel(i) >= 21)
			{
				bHasMasterSkill = true;
				break;
			}

		lua_pushboolean(L, bHasMasterSkill);
		return 1;
	}

	int pc_remove_skill_book_no_delay(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
		return 0;
	}

	int pc_remove_skill_book_no_delay2(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY2);
		return 0;
	}

	int pc_is_skill_book_no_delay(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		lua_pushboolean(L, ch->FindAffect(AFFECT_SKILL_NO_BOOK_DELAY) ? true : false);
		return 1;
	}

	int pc_is_skill_book_no_delay2(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		lua_pushboolean(L, ch->FindAffect(AFFECT_SKILL_NO_BOOK_DELAY2) ? true : false);
		return 1;
	}
#ifdef ENABLE_ZODIAC_TEMPLE
	int pc_change_animasphere(lua_State* L)
	{
		int animasphere = (int)lua_tonumber(L, -1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (animasphere + ch->GetAnimasphere() < 0)
			sys_err("QUEST wrong ChangeGem %d (now %d)", animasphere, ch->GetAnimasphere());
		else
		{
			//DBManager::instance().SendMoneyLog(MONEY_LOG_QUEST, ch->GetPlayerID(), animasphere);
			ch->PointChange(POINT_ANIMASPHERE, animasphere, true);
		}

		return 0;
	}
	
	int pc_get_animasphere(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetAnimasphere());
		return 1;
	}
#endif
	int pc_refresh_timer_cdrs(lua_State* L)
	{
		if (LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr())
			RefreshTimerCDRs_G(ch);
		return 0;
	}

	int pc_update_dungeon_rank(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		if (!lua_isnumber(L, 1))
		{
			sys_err("wrong dungeonIndex");
			return 0;
		}

		if (!lua_isnumber(L, 2))
		{
			sys_err("wrong completeTime");
			return 0;
		}
		
		if ((DWORD)lua_tonumber(L, 2) < 15 || (DWORD)lua_tonumber(L, 2) > 3600)
		{
			sys_err("time bad %d %d", ch->GetPlayerID(), (DWORD)lua_tonumber(L, 2));
			return 0;
		}
		
		LogManager::instance().DungeonLog(ch, (BYTE)lua_tonumber(L, 1), (DWORD)lua_tonumber(L, 2));

		ch->ChatPacket(CHAT_TYPE_INFO, "You've completed the dungeon in %d seconds.", (DWORD)lua_tonumber(L, 2));
		return 0;
	}

	int pc_buffi_gender(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushnumber(L, 1);
			return 1;
		}

		if (ch->GetWear(SKINSYSTEM_SLOT_BUFFI_HAIR))
		{
			if (ch->GetQuestFlag("fake_buff.gender") && IS_SET(ch->GetWear(SKINSYSTEM_SLOT_BUFFI_HAIR)->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE) ||
				!ch->GetQuestFlag("fake_buff.gender") && IS_SET(ch->GetWear(SKINSYSTEM_SLOT_BUFFI_HAIR)->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
			{
				lua_pushnumber(L, 2);
				return 1;
			}
		}

		bool needRespawn = false;
		if (ch->FakeBuff_Owner_Despawn())
			needRespawn = true;

		int currGender = ch->GetQuestFlag("fake_buff.gender");
		ch->SetQuestFlag("fake_buff.gender", currGender ^ 1);

		LPITEM buffiItem = ITEM_MANAGER::instance().Find(ch->GetQuestFlag("fake_buff.id"));
		if (needRespawn && buffiItem)
			ch->FakeBuff_Owner_Spawn(ch->GetX(), ch->GetY(), buffiItem);

		lua_pushnumber(L, 0);
		return 1;
	}

	int pc_learn_grand_master_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			sys_err("wrong skill index");
			return 0;
		}

#ifdef __FAKE_BUFF__
		lua_pushboolean(L, ch->LearnGrandMasterSkill((long)lua_tonumber(L, 1), lua_isboolean(L, 2) && lua_toboolean(L, 2)));
#else
		lua_pushboolean(L, ch->LearnGrandMasterSkill((long)lua_tonumber(L, 1)));
#endif
		return 1;
	}
	
#ifdef ENABLE_MESSENGER_BLOCK
	int pc_is_blocked(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			sys_err("%s ch null ptr", __FUNCTION__);
			lua_pushnumber(L, 0);
			return 0;
		}

		if (!lua_isnumber(L, 1))
		{
			sys_err("wrong::pc_is_blocked");
			return 0;
		}

		const char * arg1 = lua_tostring(L, 1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
		if(!tch || !tch->IsPC())
		{
			sys_err("wrong::pc_is_blocked-2");
			return 0;
		}

		lua_pushboolean(L, MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_BLOCK));
		return 1;
	}
	int pc_is_friend(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			sys_err("%s ch null ptr", __FUNCTION__);
			lua_pushnumber(L, 0);
			return 0;
		}

		if (!lua_isnumber(L, 1))
		{
			sys_err("wrong::pc_is_blocked");
			return 0;
		}
		
		const char * arg1 = lua_tostring(L, 1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
		if(!tch || !tch->IsPC())
		{
			sys_err("wrong::pc_is_blocked-2");
			return 0;
		}

		lua_pushboolean(L, MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_FRIEND));
		return 1;
	}
#endif

#ifdef __LEGENDARY_SKILL__
	int pc_learn_legendary_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			sys_err("wrong skill index");
			return 0;
		}

		lua_pushboolean(L, ch->LearnLegendarySkill((long)lua_tonumber(L, 1)));
		return 1;
	}
#endif

	int pc_set_warp_location(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			sys_err("wrong map index");
			return 0;
		}

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("wrong coodinate");
			return 0;
		}

		ch->SetWarpLocation((long)lua_tonumber(L,1), (long)lua_tonumber(L,2), (long)lua_tonumber(L,3));
		return 0;
	}

	int pc_set_warp_location_local(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			sys_err("wrong map index");
			return 0;
		}

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("wrong coodinate");
			return 0;
		}

		long lMapIndex = (long) lua_tonumber(L, 1);
		const TMapRegion * region = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex >= 10000 ? lMapIndex / 10000 : lMapIndex);

		if (!region)
		{
			sys_err("invalid map index %d", lMapIndex);
			return 0;
		}

		int x = (int) lua_tonumber(L, 2);
		int y = (int) lua_tonumber(L, 3);

		if (x > region->ex - region->sx)
		{
			sys_err("x coordinate overflow max: %d input: %d", region->ex - region->sx, x);
			return 0;
		}

		if (y > region->ey - region->sy)
		{
			sys_err("y coordinate overflow max: %d input: %d", region->ey - region->sy, y);
			return 0;
		}

		ch->SetWarpLocation(lMapIndex, x + region->sx / 100, y + region->sy / 100);
		return 0;
	}

	int pc_get_start_location(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		lua_pushnumber(L, EMPIRE_START_MAP(ch->GetEmpire()));
		lua_pushnumber(L, EMPIRE_START_X(ch->GetEmpire()) / 100);
		lua_pushnumber(L, EMPIRE_START_Y(ch->GetEmpire()) / 100);
		return 3;
	}

	int pc_warp(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			lua_pushboolean(L, false);
			return 1;
		}

		long map_index = 0;

		if (lua_isnumber(L, 3))
			map_index = (int) lua_tonumber(L,3);

		//PREVENT_HACK
		if ( ch->IsHack() )
		{
			lua_pushboolean(L, false);
			return 1;
		}
		//END_PREVENT_HACK
	
		if ( test_server )
			ch->ChatPacket( CHAT_TYPE_INFO, "pc_warp %d %d %d",(int)lua_tonumber(L,1),
					(int)lua_tonumber(L,2),map_index );
		ch->WarpSet((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2), map_index);
		
		lua_pushboolean(L, true);

		return 1;
	}

	int pc_warp_base(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("no map index argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		PIXEL_POSITION pos;
		if (!SECTREE_MANAGER::instance().GetRecallPositionByEmpire(lua_tonumber(L, 1), ch->GetEmpire(), pos))
		{
			sys_err("cannot get base of map %d", lua_tonumber(L, 1));
			lua_pushboolean(L, 0);
			return 1;
		}

		lua_pushboolean(L, ch->WarpSet(pos.x, pos.y, lua_tonumber(L, 1)));
		return 1;
	}

	int pc_direction_effect(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("no coodinate argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::Instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		int mIndex = ch->GetMapIndex();
		if (mIndex >= 10000)
			mIndex /= 10000;

		const TMapRegion * region = SECTREE_MANAGER::instance().GetMapRegion(mIndex);

		if (!region)
			return 0;

		int x = (int)lua_tonumber(L, 1) * 100;
		int y = (int)lua_tonumber(L, 2) * 100;

		ch->ChatPacket(CHAT_TYPE_COMMAND, "DrawDirection %d %d %d", x, y, (int)GetDegreeFromPositionXY(ch->GetX(), y + region->sy, x + region->sx, ch->GetY()));
		return 0;
	}

	int pc_warp_local(lua_State * L)
	{
		if (!lua_isnumber(L, 1)) 
		{
			sys_err("no map index argument");
			return 0;
		}

		if (!lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("no coodinate argument");
			return 0;
		}

		int lMapIndex = (int) lua_tonumber(L, 1);
		const TMapRegion * region = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex);

		if (!region)
		{
			sys_err("invalid map index %d", lMapIndex);
			return 0;
		}

		int x = (int) lua_tonumber(L, 2) * 100;
		int y = (int) lua_tonumber(L, 3) * 100;

		if (x > region->ex - region->sx)
		{
			sys_err("x coordinate overflow max: %d input: %d", region->ex - region->sx, x);
			return 0;
		}

		if (y > region->ey - region->sy)
		{
			sys_err("y coordinate overflow max: %d input: %d", region->ey - region->sy, y);
			return 0;
		}

		bool bRet = CQuestManager::instance().GetCurrentCharacterPtr()->WarpSet(region->sx + x, region->sy + y, lua_isnumber(L, 4) ? lua_tonumber(L, 4) : 0);
		lua_pushboolean(L, bRet);
		return bRet;
	}

	int pc_warp_exit(lua_State * L)
	{
		CQuestManager::instance().GetCurrentCharacterPtr()->ExitToSavedLocation();
		return 0;
	}

	int pc_in_dungeon(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetDungeon()?1:0);
		return 1;
	}

	int pc_hasguild(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetGuild() ? 1 : 0);
		return 1;
	}

	int pc_getguild(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetGuild() ? ch->GetGuild()->GetID() : 0);
		return 1;
	}

	int pc_isguildmaster(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CGuild * g = ch->GetGuild();

		if (g)
			lua_pushboolean(L, (ch->GetPlayerID() == g->GetMasterPID()));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int pc_destroy_guild(lua_State * L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CGuild * g = ch->GetGuild();

		lua_pushboolean(L, g && g->RequestDisband(ch->GetPlayerID()));
		return 1;
	}

	int pc_remove_from_guild(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		CGuild * g = ch->GetGuild();

		if (g)
			g->RequestRemoveMember(ch->GetPlayerID());

		return 0;
	}

	int pc_give_gold(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
		{
			sys_err("QUEST : wrong argument");
			return 0;
		}

		int iAmount = (int) lua_tonumber(L, 1);

		if (iAmount <= 0)
		{
			sys_err("QUEST : gold amount less then zero");
			return 0;
		}

		LogManager::instance().MoneyLog(MONEY_LOG_QUEST, ch->GetPlayerID(), iAmount);
		ch->PointChange(POINT_GOLD, iAmount, true);
		return 0;
	}

	int pc_warp_to_guild_war_observer_position(lua_State* L)
	{
		luaL_checknumber(L, 1);
		luaL_checknumber(L, 2);

		DWORD gid1 = (DWORD)lua_tonumber(L, 1);
		DWORD gid2 = (DWORD)lua_tonumber(L, 2);

		CGuild* g1 = CGuildManager::instance().FindGuild(gid1);
		CGuild* g2 = CGuildManager::instance().FindGuild(gid2);

		if (!g1 || !g2)
		{
			luaL_error(L, "no such guild with id %d %d", gid1, gid2);
		}

		PIXEL_POSITION pos;

		DWORD dwMapIndex = g1->GetGuildWarMapIndex(gid2);

		if (!CWarMapManager::instance().GetStartPosition(dwMapIndex, 2, pos))
		{
			luaL_error(L, "not under warp guild war between guild %d %d", gid1, gid2);
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		//PREVENT_HACK
		if ( ch->IsHack() )
			return 0;
		//END_PREVENT_HACK

		ch->SetQuestFlag("war.is_war_member", 0);
		ch->SaveExitLocation();
		ch->WarpSet(pos.x, pos.y, dwMapIndex);
		return 0;
	}

	int pc_give_item_from_special_item_group(lua_State* L)
	{
		luaL_checknumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		DWORD dwGroupVnum = (DWORD) lua_tonumber(L,1);

		std::vector <DWORD> dwVnums;
		std::vector <DWORD> dwCounts;
		std::vector <LPITEM> item_gets;
		int count = 0;

		ch->GiveItemFromSpecialItemGroup(dwGroupVnum, dwVnums, dwCounts, item_gets, count);
		
		for (int i = 0; i < count; i++)
		{
			if (!item_gets[i])
			{
				if (dwVnums[i] == 1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
				}
				else if (dwVnums[i] == 2)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³ª¹«¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
				}
			}
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(ch->GetLanguageID()));
		}
		return 0;
	}

	int pc_enough_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		DWORD item_vnum = (DWORD)lua_tonumber(L, 1);
		auto pTable = ITEM_MANAGER::instance().GetTable(item_vnum);
		if (!pTable)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		bool bEnoughInventoryForItem = ch->GetEmptyInventory(pTable->size()) != -1;
		lua_pushboolean(L, bEnoughInventoryForItem);
		return 1;
	}

	int pc_give_item(lua_State* L)
	{
		PC* pPC = CQuestManager::instance().GetCurrentPC();
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isstring(L, 1) || !(lua_isstring(L, 2)||lua_isnumber(L, 2)))
		{
			sys_err("QUEST : wrong argument");
			return 0;
		}

		DWORD dwVnum;

		if (lua_isnumber(L,2)) // ¹øÈ£ÀÎ°æ¿ì ¹øÈ£·Î ÁØ´Ù.
			dwVnum = (int) lua_tonumber(L, 2);
		else if (!ITEM_MANAGER::instance().GetVnum(lua_tostring(L, 2), dwVnum))
		{
			sys_err("QUEST Make item call error : wrong item name : %s", lua_tostring(L,1));
			return 0;
		}

		int icount = 1;

		if (lua_isnumber(L, 3) && lua_tonumber(L, 3) > 0)
		{
			icount = (int)rint(lua_tonumber(L, 3));

			if (icount <= 0) 
			{
				sys_err("QUEST Make item call error : wrong item count : %g", lua_tonumber(L, 2));
				return 0;
			}
		}

		pPC->GiveItem(lua_tostring(L, 1), dwVnum, icount);

		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), dwVnum, icount);
		return 0;
	}

	int pc_give_or_drop_item(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isstring(L, 1) && !lua_isnumber(L, 1))
		{
			sys_err("QUEST Make item call error : wrong argument");
			lua_pushnumber (L, 0);
			return 1;
		}

		DWORD dwVnum;

		if (lua_isnumber(L, 1)) // ¹øÈ£ÀÎ°æ¿ì ¹øÈ£·Î ÁØ´Ù.
		{
			dwVnum = (int) lua_tonumber(L, 1);
		}
		else if (!ITEM_MANAGER::instance().GetVnum(lua_tostring(L, 1), dwVnum))
		{
			sys_err("QUEST Make item call error : wrong item name : %s", lua_tostring(L,1));
			lua_pushnumber (L, 0);

			return 1;
		}

		int icount = 1;
		if (lua_isnumber(L,2) && lua_tonumber(L,2)>0)
		{
			icount = (int)rint(lua_tonumber(L,2));
			if (icount<=0) 
			{
				sys_err("QUEST Make item call error : wrong item count : %g", lua_tonumber(L,2));
				lua_pushnumber (L, 0);
				return 1;
			}
		}
		sys_log(0, "QUEST [REWARD] item %s to %s", lua_tostring(L, 1), ch->GetName());

		PC* pPC = CQuestManager::instance().GetCurrentPC();

		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), dwVnum, icount);

		LPITEM item = ch->AutoGiveItem(dwVnum, icount);

		if (NULL != item)
		{
			if (lua_isboolean(L, 3)) // Is_GM_ITEM
			{
				if (lua_toboolean(L, 3))
				{					
					ch->tchat("%s GMITEM 1", __FILE__);
					item->SetGMOwner(true);
				}
				else
					ch->tchat("%s GMITEM 0", __FILE__);
			}
			
			lua_pushnumber(L, item->GetID());
			LogManager::instance().ItemLog(ch, item, "CREATE_QUEST_REWARD", "PC_GIVE_ITEM2");
		}
		else
			lua_pushnumber (L, 0);
		return 1;
	}

	int pc_give_or_drop_item_and_select(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isstring(L, 1) && !lua_isnumber(L, 1))
		{
			sys_err("QUEST Make item call error : wrong argument");
			return 0;
		}

		DWORD dwVnum;

		if (lua_isnumber(L, 1)) // ¹øÈ£ÀÎ°æ¿ì ¹øÈ£·Î ÁØ´Ù.
		{
			dwVnum = (int) lua_tonumber(L, 1);
		}
		else if (!ITEM_MANAGER::instance().GetVnum(lua_tostring(L, 1), dwVnum))
		{
			sys_err("QUEST Make item call error : wrong item name : %s", lua_tostring(L,1));
			return 0;
		}

		int icount = 1;
		if (lua_isnumber(L,2) && lua_tonumber(L,2)>0)
		{
			icount = (int)rint(lua_tonumber(L,2));
			if (icount<=0) 
			{
				sys_err("QUEST Make item call error : wrong item count : %g", lua_tonumber(L,2));
				return 0;
			}
		}

		bool bCanStack = true;
		if (lua_isboolean(L, 3))
			bCanStack = lua_toboolean(L, 3);

		sys_log(0, "QUEST [REWARD] item %s to %s", lua_tostring(L, 1), ch->GetName());

		PC* pPC = CQuestManager::instance().GetCurrentPC();

		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), dwVnum, icount);

		LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, icount);
		if (item)
		{
			if (bCanStack)
				ch->AutoGiveItem(item);
			else
			{
				int iPos = -1;
				BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);
				if (bWindow == INVENTORY)
					iPos = ch->GetEmptyInventory(item->GetSize());
				else
					iPos = ch->GetEmptyNewInventory(bWindow);

				if (iPos != -1)
					item->AddToCharacter(ch, TItemPos(bWindow, iPos));
				else
				{
					item->AddToGround(ch->GetMapIndex(), ch->GetXYZ());
					item->StartDestroyEvent();
					item->SetOwnership(ch, 300);

					LogManager::instance().ItemLog(ch, item, "SYSTEM_DROP", item->GetName());
				}
			}
		}

		if (NULL != item)
			CQuestManager::Instance().SetCurrentItem(item);
		
		return 0;
	}

	int pc_get_current_map_index(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetMapIndex());
		return 1;
	}

	int pc_get_x(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetX()/100);
		return 1;
	}

	int pc_get_y(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetY()/100);
		return 1;
	}

	int pc_get_local_x(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());

		if (pMap)
			lua_pushnumber(L, (ch->GetX() - pMap->m_setting.iBaseX) / 100);
		else
			lua_pushnumber(L, ch->GetX() / 100);

		return 1;
	}

	int pc_get_local_y(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());

		if (pMap)
			lua_pushnumber(L, (ch->GetY() - pMap->m_setting.iBaseY) / 100);
		else
			lua_pushnumber(L, ch->GetY() / 100);

		return 1;
	}

	int pc_count_item(lua_State* L)
	{
		if (lua_isnumber(L, -1))
			lua_pushnumber(L,CQuestManager::instance().GetCurrentCharacterPtr()->CountSpecifyItem((DWORD)lua_tonumber(L, -1)));
		else if (lua_isstring(L, -1))
		{
			DWORD item_vnum;

			if (!ITEM_MANAGER::instance().GetVnum(lua_tostring(L,1), item_vnum))
			{
				sys_err("QUEST count_item call error : wrong item name : %s", lua_tostring(L,1));
				lua_pushnumber(L, 0);
			}
			else
			{
				lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->CountSpecifyItem(item_vnum));
			}
		}
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int pc_remove_item(lua_State* L)
	{
		if (lua_gettop(L) == 1)
		{
			DWORD item_vnum;

			if (lua_isnumber(L,1))
			{
				item_vnum = (DWORD)lua_tonumber(L, 1);
			}
			else if (lua_isstring(L,1))
			{
				if (!ITEM_MANAGER::instance().GetVnum(lua_tostring(L,1), item_vnum))
				{
					sys_err("QUEST remove_item call error : wrong item name : %s", lua_tostring(L,1));
					return 0;
				}
			}
			else
			{
				sys_err("QUEST remove_item wrong argument");
				return 0;
			}

			sys_log(0,"QUEST remove a item vnum %d of %s[%d]", item_vnum, CQuestManager::instance().GetCurrentCharacterPtr()->GetName(), CQuestManager::instance().GetCurrentCharacterPtr()->GetPlayerID());
			lua_pushboolean(L, CQuestManager::instance().GetCurrentCharacterPtr()->RemoveSpecifyItem(item_vnum));
		}
		else if (lua_gettop(L) == 2)
		{
			DWORD item_vnum;

			if (lua_isnumber(L, 1))
			{
				item_vnum = (DWORD)lua_tonumber(L, 1);
			}
			else if (lua_isstring(L, 1))
			{
				if (!ITEM_MANAGER::instance().GetVnum(lua_tostring(L,1), item_vnum))
				{
					sys_err("QUEST remove_item call error : wrong item name : %s", lua_tostring(L,1));
					return 0;
				}
			}
			else
			{
				sys_err("QUEST remove_item wrong argument");
				return 0;
			}

			DWORD item_count = (DWORD) lua_tonumber(L, 2);
			sys_log(0, "QUEST remove items(vnum %d) count %d of %s[%d]",
					item_vnum,
					item_count,
					CQuestManager::instance().GetCurrentCharacterPtr()->GetName(),
					CQuestManager::instance().GetCurrentCharacterPtr()->GetPlayerID());

			lua_pushboolean(L, CQuestManager::instance().GetCurrentCharacterPtr()->RemoveSpecifyItem(item_vnum, item_count));
		}
		return 1;
	}

	int pc_get_leadership(lua_State * L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetLeadershipSkillLevel());
		return 1;
	}

	int pc_reset_point(lua_State * L)
	{
		CQuestManager::instance().GetCurrentCharacterPtr()->ResetPoint(CQuestManager::instance().GetCurrentCharacterPtr()->GetLevel());
		return 0;
	}

	int pc_get_playtime(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetRealPoint(POINT_PLAYTIME));
		return 1;
	}

	int pc_get_vid(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetVID());
		return 1;
	}
	int pc_get_name(lua_State* L)
	{
		LPCHARACTER pc = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!pc)
			lua_pushstring(L, "");
		else if (lua_isboolean(L, 1) && !lua_toboolean(L, 1))
			lua_pushstring(L, pc->GetName());
		else
			lua_pushstring(L, CLocaleManager::instance().StringToArgument((pc->GetGMLevel() && !test_server) ? "GameMaster" : pc->GetName()));
		return 1;
	}

	int pc_get_next_exp(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetNextExp());
		return 1;
	}

	int pc_get_exp(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetExp());
		return 1;
	}

	int pc_get_race(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetRaceNum());
		return 1;
	}

	int pc_change_sex(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->ChangeSex());
		return 1;
	}

	int pc_get_job(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetJob());
		return 1;
	}

	int pc_get_max_sp(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetMaxSP());
		return 1;
	}

	int pc_get_sp(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetSP());
		return 1;
	}

	int pc_change_sp(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		long val = (long) lua_tonumber(L, 1);

		if (val == 0)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (val > 0) // Áõ°¡½ÃÅ°´Â °ÍÀÌ¹Ç·Î ¹«Á¶°Ç ¼º°ø ¸®ÅÏ
			ch->PointChange(POINT_SP, val);
		else if (val < 0)
		{
			if (ch->GetSP() < -val)
			{
				lua_pushboolean(L, 0);
				return 1;
			}

			ch->PointChange(POINT_SP, val);
		}

		lua_pushboolean(L, 1);
		return 1;
	}

	int pc_get_max_hp(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetMaxHP());
		return 1;
	}

	int pc_get_hp(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetHP());
		return 1;
	}

	int pc_get_level(lua_State * L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetLevel());
		return 1;
	}

	int pc_get_max_level(lua_State * L)
	{
#ifdef __PRESTIGE__
		int iPrestigeLevel = lua_isnumber(L, 1) ? lua_tonumber(L, 1) : CQuestManager::instance().GetCurrentCharacterPtr()->GetPrestigeLevel();
		lua_pushnumber(L, gPlayerMaxLevel[MINMAX(0, iPrestigeLevel, gPrestigeMaxLevel - 1)]);
#else
		lua_pushnumber(L, gPlayerMaxLevel);
#endif
		return 1;
	}

	int pc_set_level(lua_State * L)  
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			return 0;
		}
		else
		{
			// int newLevel = lua_tonumber(L, 1);
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

#ifdef __PRESTIGE__
			int newLevel = MINMAX(1, lua_tonumber(L, 1), gPrestigePlayerMaxLevel[ch->Prestige_GetLevel()]);
#else
			int newLevel = MINMAX(1, lua_tonumber(L, 1), gPlayerMaxLevel);
#endif

			sys_log(0,"QUEST [LEVEL] %s jumpint to level %d", ch->GetName(), (int)rint(lua_tonumber(L,1)));

			PC* pPC = CQuestManager::instance().GetCurrentPC();
			LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), newLevel, 0);
			
			//Æ÷ÀÎÆ® : ½ºÅ³, ¼­ºê½ºÅ³, ½ºÅÈ
			ch->PointChange(POINT_SKILL, newLevel - ch->GetLevel());
			ch->PointChange(POINT_SUB_SKILL, newLevel < 10 ? 0 : newLevel - MAX(ch->GetLevel(), 9));
			ch->PointChange(POINT_STAT, ((MINMAX(1, newLevel, 90) - ch->GetLevel()) * 3) + ch->GetPoint(POINT_LEVEL_STEP));
			//·¹º§
			ch->PointChange(POINT_LEVEL, newLevel - ch->GetLevel(), false, true);
	
			// È¸º¹
			ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
			ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
			
			ch->ComputePoints();
			ch->PointsPacket();
			ch->SkillLevelPacket();

			return 0;
		}
	}

	int pc_get_weapon(lua_State * L)
	{
		LPITEM item = CQuestManager::instance().GetCurrentCharacterPtr()->GetWear(WEAR_WEAPON);

		if (!item)
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, item->GetVnum());

		return 1;
	}

	int pc_get_armor(lua_State * L)
	{
		LPITEM item = CQuestManager::instance().GetCurrentCharacterPtr()->GetWear(WEAR_BODY);

		if (!item)
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, item->GetVnum());

		return 1;
	}

	int pc_get_wear(lua_State * L)
	{		
		if (!lua_isnumber(L, 1))
		{
			sys_err("QUEST wrong set flag");
			return 0;
		}

		unsigned char bCell = (unsigned char)lua_tonumber(L, 1);

		LPITEM item = CQuestManager::instance().GetCurrentCharacterPtr()->GetWear(bCell);


		if (!item)
			lua_pushnumber(L, 0);
		else
			lua_pushnumber(L, item->GetVnum());

		return 1;
	}

	int pc_get_money(lua_State * L)
	{ 
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetGold());
		return 1;
	}

	// 20050725.myevan.ÀºµÐÀÇ ¸ÁÅä »ç¿ëÁß È¥¼® ¼ö·Ã½Ã ¼±¾ÇÄ¡°¡ µÎ¹è ¼Ò¸ðµÇ´Â ¹ö±×°¡ ¹ß»ýÇØ
	// ½ÇÁ¦ ¼±¾ÇÄ¡¸¦ ÀÌ¿ëÇØ °è»êÀ» ÇÏ°Ô ÇÑ´Ù.
	int pc_get_real_alignment(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetRealAlignment()/10);
		return 1;
	}

	int pc_get_alignment(lua_State* L)
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetAlignment()/10);
		return 1;
	}

	int pc_change_alignment(lua_State * L)
	{
		int alignment = (int)(lua_tonumber(L, 1)*10);
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		ch->UpdateAlignment(alignment);
		return 0;
	}

	int pc_change_money(lua_State * L)
	{
		long long gold = lua_tonumber(L, -1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (gold + ch->GetGold() < 0)
			sys_err("QUEST wrong ChangeGold %d (now %lld)", gold, ch->GetGold());
		else
		{
			LogManager::instance().MoneyLog(MONEY_LOG_QUEST, ch->GetPlayerID(), gold);
			ch->PointChange(POINT_GOLD, gold, true);
		}

		return 0;
	}

	int pc_use_gold_item(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM pItem = CQuestManager::Instance().GetCurrentItem();
		if (!ch || !pItem)
			return 0;

		long long recv_gold = pItem->GetGold();
		long long sum_gold = recv_gold + ch->GetGold();

		if (sum_gold >= GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot use that because it would exceed your gold limit."));
			return 0;
		}

		ch->PointChange(POINT_GOLD, recv_gold, true);
		if (pItem->GetCount() == 1)
			ITEM_MANAGER::instance().RemoveItem(pItem, "REMOVE_GOLD_ITEM");
		else
			pItem->SetCount(pItem->GetCount() - 1);

		ch->SetGoldTime();

		return 0;
	}

	int pc_set_another_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L, 1) || !lua_isstring(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("QUEST wrong set flag");
			return 0;
		}
		else
		{
			const char * sz = lua_tostring(L, 1);
			const char * sz2 = lua_tostring(L, 2);
			CQuestManager & q = CQuestManager::Instance();
			PC * pPC = q.GetCurrentPC();
			pPC->SetFlag(string(sz)+"."+sz2, int(rint(lua_tonumber(L,3))));
			return 0;
		}
	}

	int pc_get_another_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isstring(L,2))
		{
			sys_err("QUEST wrong get flag");
			return 0;
		}

		const char* sz = lua_tostring(L,1);
		const char* sz2 = lua_tostring(L,2);
		CQuestManager& q = CQuestManager::Instance();
		PC* pPC = q.GetCurrentPC();
		if (!pPC)
		{
			return 0;
		}
		lua_pushnumber(L,pPC->GetFlag(string(sz)+"."+sz2));
		return 1;		
	}

	int pc_get_flag(lua_State* L)
	{
		if (!lua_isstring(L,-1))
		{
			sys_err("QUEST wrong get flag");
			return 0;
		}
		else
		{
			const char* sz = lua_tostring(L,-1);
			CQuestManager& q = CQuestManager::Instance();
			PC* pPC = q.GetCurrentPC();
			lua_pushnumber(L,pPC->GetFlag(sz));
			return 1;
		}
	}

	int pc_get_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,-1))
		{
			sys_err("QUEST wrong get flag");
			return 0;
		}
		else
		{
			const char* sz = lua_tostring(L,-1);
			CQuestManager& q = CQuestManager::Instance();
			PC* pPC = q.GetCurrentPC();
			lua_pushnumber(L, pPC->GetFlag(q.GetCurrentRootPC()->GetCurrentQuestName() + "." + sz));
			if ( test_server )
				sys_log( 0 ,"GetQF ( %s . %s )", pPC->GetCurrentQuestName().c_str(), sz );
		}
		return 1;
	}

	int pc_set_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			sys_err("QUEST wrong set flag");
		}
		else
		{
			const char* sz = lua_tostring(L,1);
			CQuestManager& q = CQuestManager::Instance();
			PC* pPC = q.GetCurrentPC();
			pPC->SetFlag(sz, int(rint(lua_tonumber(L,2))));
		}
		return 0;
	}

	int pc_set_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
		{
			sys_err("QUEST wrong set flag");
		}
		else
		{
			const char* sz = lua_tostring(L,1);
			CQuestManager& q = CQuestManager::Instance();
			PC* pPC = q.GetCurrentPC();
			pPC->SetFlag(q.GetCurrentRootPC()->GetCurrentQuestName() + "." + sz, int(rint(lua_tonumber(L, 2))));
		}
		return 0;
	}

	int pc_inc_quest_flag(lua_State* L)
	{
		if (!lua_isstring(L,1))
		{
			sys_err("QUEST wrong set flag");
			return 0;
		}
		const char* sz = lua_tostring(L,1);
		CQuestManager& q = CQuestManager::Instance();
		PC* pPC = q.GetCurrentPC();			
		pPC->SetFlag(q.GetCurrentRootPC()->GetCurrentQuestName() + "." + sz, pPC->GetFlag(q.GetCurrentRootPC()->GetCurrentQuestName() + "." + sz) + 1);
		lua_pushnumber(L, pPC->GetFlag(q.GetCurrentRootPC()->GetCurrentQuestName() + "." + sz));
		return 1;
	}

	int pc_del_quest_flag(lua_State *L)
	{
		if (!lua_isstring(L, 1))
		{
			sys_err("argument error");
			return 0;
		}

		const char * sz = lua_tostring(L, 1);
		PC * pPC = CQuestManager::instance().GetCurrentPC();
		pPC->DeleteFlag(CQuestManager::instance().GetCurrentRootPC()->GetCurrentQuestName() + "." + sz);
		return 0;
	}

	int pc_give_exp2(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!lua_isnumber(L,1))
			return 0;

		bool bForce = lua_isboolean(L, 2) && lua_toboolean(L, 2);
		bool bDisableExp = false;

		sys_log(0,"QUEST [REWARD] %s give exp2 %d", ch->GetName(), (int)rint(lua_tonumber(L,1)));

		DWORD exp = (DWORD)rint(lua_tonumber(L,1));

		PC* pPC = CQuestManager::instance().GetCurrentPC();
		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), exp, 0);
		if (bForce)
		{
			if (ch->IsEXPDisabled())
			{
				ch->SetEXPEnabled();
				bDisableExp = true;
			}
		}
		ch->PointChange(POINT_EXP, exp);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have received %u exp."), exp);
		if (bDisableExp)
			ch->SetEXPDisabled();

		return 0;
	}

	int pc_give_exp(lua_State* L)
	{
		if (!lua_isstring(L,1) || !lua_isnumber(L,2))
			return 0;

		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		sys_log(0,"QUEST [REWARD] %s give exp %s %d", ch->GetName(), lua_tostring(L,1), (int)rint(lua_tonumber(L,2)));

		DWORD exp = (DWORD)rint(lua_tonumber(L,2));

		PC* pPC = CQuestManager::instance().GetCurrentPC();

		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), exp, 0);

		pPC->GiveExp(lua_tostring(L,1), exp);
		return 0;
	}

	int pc_give_exp_perc(lua_State* L)
	{
		CQuestManager & q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (!ch || !lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
			return 0;

		int lev = (int)rint(lua_tonumber(L,2));
		double proc = (lua_tonumber(L,3));

		sys_log(0, "QUEST [REWARD] %s give exp %s lev %d percent %g%%", ch->GetName(), lua_tostring(L, 1), lev, proc);

		DWORD exp = (DWORD)((exp_table[MINMAX(0, lev, PLAYER_EXP_TABLE_MAX)] * proc) / 100);
		PC * pPC = CQuestManager::instance().GetCurrentPC();
		
		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), exp, 0);

		pPC->GiveExp(lua_tostring(L, 1), exp);
		return 0;
	}

	int pc_get_empire(lua_State* L)  
	{
		CQuestManager & q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (!ch)
			return 0;
		
		lua_pushnumber(L, ch->GetEmpire());
		return 1;
	}

	int pc_get_part(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!lua_isnumber(L,1))
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		int part_idx = (int)lua_tonumber(L, 1);
		lua_pushnumber(L, ch->GetPart(part_idx));
		return 1;
	}

	int pc_set_part(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			return 0;
		}
		int part_idx = (int)lua_tonumber(L, 1);
		int part_value = (int)lua_tonumber(L, 2);
		ch->SetPart(part_idx, part_value);
		ch->UpdatePacket();
		return 0;
	}

	int pc_get_skillgroup(lua_State* L)  
	{
		lua_pushnumber(L, CQuestManager::instance().GetCurrentCharacterPtr()->GetSkillGroup());
		return 1;
	}

	int pc_set_skillgroup(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			sys_err("QUEST wrong skillgroup number");
		else
		{
			CQuestManager & q = CQuestManager::Instance();
			LPCHARACTER ch = q.GetCurrentCharacterPtr();

			ch->SetSkillGroup((unsigned char) rint(lua_tonumber(L, 1)));
		}
		return 0;
	}

	int pc_is_polymorphed(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->IsPolymorphed());
		return 1;
	}

	int pc_remove_polymorph(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->RemoveAffect(AFFECT_POLYMORPH);
		ch->SetPolymorph(0);
		return 0;
	}

	int pc_polymorph(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		DWORD dwVnum = (DWORD) lua_tonumber(L, 1);
		int iDuration = (int) lua_tonumber(L, 2);
		ch->AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
		return 0;
	}

	int pc_have_map_scroll(lua_State* L)
	{
		if (!lua_isstring(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		const char * szMapName = lua_tostring(L, 1);
		const TMapRegion * region = SECTREE_MANAGER::instance().FindRegionByPartialName(szMapName);

		if (!region)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		bool bFind = false;
		for (int iCell = 0; iCell < ch->GetInventoryMaxNum(); iCell++)
		{
			LPITEM item = ch->GetInventoryItem(iCell);
			if (!item)
				continue;

			if (item->GetType() == ITEM_USE && 
					item->GetSubType() == USE_TALISMAN && 
					(item->GetValue(0) == 1 || item->GetValue(0) == 2))
			{
				int x = item->GetSocket(0);
				int y = item->GetSocket(1);
				//if ((x-item_x)*(x-item_x)+(y-item_y)*(y-item_y)<r*r)
				if (region->sx <=x && region->sy <= y && x <= region->ex && y <= region->ey)
				{
					bFind = true;
					break;
				}
			}
		}

		lua_pushboolean(L, bFind);
		return 1;
	}

	int pc_get_war_map(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetWarMap() ? ch->GetWarMap()->GetMapIndex() : 0);
		return 1;
	}

	int pc_have_pos_scroll(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			sys_err("invalid x y position");
			lua_pushboolean(L, 0);
			return 1;
		}

		if (!lua_isnumber(L,2))
		{
			sys_err("invalid radius");
			lua_pushboolean(L, 0);
			return 1;
		}

		int x = (int)lua_tonumber(L, 1);
		int y = (int)lua_tonumber(L, 2);
		float r = (float)lua_tonumber(L, 3);

		bool bFind = false;
		for (int iCell = 0; iCell < ch->GetInventoryMaxNum(); iCell++)
		{
			LPITEM item = ch->GetInventoryItem(iCell);
			if (!item)
				continue;

			if (item->GetType() == ITEM_USE && 
					item->GetSubType() == USE_TALISMAN && 
					(item->GetValue(0) == 1 || item->GetValue(0) == 2))
			{
				int item_x = item->GetSocket(0);
				int item_y = item->GetSocket(1);
				if ((x-item_x)*(x-item_x)+(y-item_y)*(y-item_y)<r*r)
				{
					bFind = true;
					break;
				}
			}
		}

		lua_pushboolean(L, bFind);
		return 1;
	}

	int pc_get_equip_refine_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int cell = (int) lua_tonumber(L, 1);
		if (cell < 0 || cell >= WEAR_MAX_NUM)
		{
			sys_err("invalid wear position %d", cell);
			lua_pushnumber(L, 0);
			return 1;
		}

		LPITEM item = ch->GetWear(cell);
		if (!item)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, item->GetRefineLevel());
		return 1;
	}

	int pc_refine_equip(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid argument");
			lua_pushboolean(L, 0);
			return 1;
		}

		int cell = (int) lua_tonumber(L, 1);
		int level_limit = (int) lua_tonumber(L, 2);
		int pct = lua_isnumber(L, 3) ? (int)lua_tonumber(L, 3) : 100;

		LPITEM item = ch->GetWear(cell);
		if (!item)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (item->GetRefinedVnum() == 0)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (item->GetRefineLevel()>level_limit)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		if (pct == 100 || random_number(1, 100) <= pct)
		{
			// °³·® ¼º°ø
			lua_pushboolean(L, 1);

			LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(item->GetRefinedVnum(), 1, 0, false);

			if (pkNewItem)
			{
				for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
					if (!item->GetSocket(i))
						break;
					else
						pkNewItem->SetSocket(i, 1);

				int set = 0;
				for (int i=0; i<ITEM_SOCKET_MAX_NUM; i++)
				{
					long socket = item->GetSocket(i);
					if (socket > 2 && socket != 28960)
					{
						pkNewItem->SetSocket(set++, socket);
					}
				}

				item->CopyAttributeTo(pkNewItem);

				ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

				// some tuits need here -_- pkNewItem->AddToCharacter(this, bCell); 
				pkNewItem->EquipTo(ch, cell);

				ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

				LogManager::instance().ItemLog(ch, pkNewItem, "REFINE SUCCESS (QUEST)", pkNewItem->GetName());
			}
		}
		else
		{
			// °³·® ½ÇÆÐ
			lua_pushboolean(L, 0);
		}

		return 1;
	}

	int pc_get_skill_level(lua_State * L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		DWORD dwVnum = (DWORD) lua_tonumber(L, 1);
		lua_pushnumber(L, ch->GetSkillLevel(dwVnum));

		return 1;
	}

	int pc_set_skill_level(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid argument");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			sys_err("pc_set_skill_level no current char ptr");
			return 0;
		}

		DWORD dwVnum = (DWORD)lua_tonumber(L, 1);
		BYTE bLevel = (BYTE)lua_tonumber(L, 2);
		BYTE bOldLevel = ch->GetSkillLevel(dwVnum);
		ch->SetSkillLevel(dwVnum, bLevel);
		char szSkillUp[256];
		snprintf(szSkillUp, sizeof(szSkillUp), "oldlv %u newlv %u", bOldLevel, bLevel);
		LogManager::instance().CharLog(ch, dwVnum, "SKILLSET", szSkillUp);
		ch->SkillLevelPacket();
		return 0;
	}

	int pc_aggregate_monster(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->AggregateMonster();
		return 0;
	}

	int pc_forget_my_attacker(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->ForgetMyAttacker();
		return 0;
	}

	int pc_attract_ranger(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->AttractRanger();
		return 0;
	}

	int pc_select_pid(lua_State* L)
	{
		DWORD pid = (DWORD) lua_tonumber(L, 1);

		if (pid == 0)
		{
			CQuestManager::Instance().GetPCForce(0);
			lua_pushnumber(L, 0);
		}
		else
		{
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			LPCHARACTER new_ch = CHARACTER_MANAGER::instance().FindByPID(pid);

			if (new_ch)
			{
				CQuestManager::instance().GetPC(new_ch->GetPlayerID());

				lua_pushnumber(L, ch ? ch->GetPlayerID() : 0);
			}
			else
				lua_pushnumber(L, 0);
		}

		return 1;
	}

	int pc_select_vid(lua_State* L)
	{
		DWORD vid = (DWORD) lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER new_ch = CHARACTER_MANAGER::instance().Find(vid);

		if (new_ch)
		{
			CQuestManager::instance().GetPC(new_ch->GetPlayerID());

			lua_pushnumber(L, ch ? (DWORD)ch->GetVID() : 0);
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int pc_get_sex(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, GET_SEX(ch)); /* 0==MALE, 1==FEMALE */
		return 1;
	}

	int pc_is_engaged(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, marriage::CManager::instance().IsEngaged(ch->GetPlayerID()));
		return 1;
	}

	int pc_is_married(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, marriage::CManager::instance().IsMarried(ch->GetPlayerID()));
		return 1;
	}

	int pc_is_engaged_or_married(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, marriage::CManager::instance().IsEngagedOrMarried(ch->GetPlayerID()));
		return 1;
	}

	int pc_is_gm(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetGMLevel() >= GM_HIGH_WIZARD);
		return 1;
	}

	int pc_get_gm_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetGMLevel());
		return 1;
	}

	int pc_mining(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		ch->mining(npc);
		return 0;
	}

	int pc_diamond_refine(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		int cost = (int) lua_tonumber(L, 1);
		int pct = (int)lua_tonumber(L, 2);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		LPITEM item = CQuestManager::instance().GetCurrentItem();

		if (item)
			lua_pushboolean(L, mining::OreRefine(ch, npc, item, cost, pct, NULL));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int pc_ore_refine(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		int cost = (int) lua_tonumber(L, 1);
		int pct = (int)lua_tonumber(L, 2);
		int metinstone_cell = (int)lua_tonumber(L, 3);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
		LPITEM item = CQuestManager::instance().GetCurrentItem();

		LPITEM metinstone_item = ch->GetInventoryItem(metinstone_cell);

		if (item && metinstone_item)
			lua_pushboolean(L, mining::OreRefine(ch, npc, item, cost, pct, metinstone_item));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

	int pc_clear_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if ( ch == NULL ) return 0;

		ch->ClearSkill();

		return 0;
	}

	int pc_clear_sub_skill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if ( ch == NULL ) return 0;

		ch->ClearSubSkill();

		return 0;
	}

	int pc_set_skill_point(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int newPoint = (int) lua_tonumber(L, 1);

		ch->SetRealPoint(POINT_SKILL, newPoint);
		ch->SetPoint(POINT_SKILL, ch->GetRealPoint(POINT_SKILL));
		ch->PointChange(POINT_SKILL, 0);
		ch->ComputePoints();
		ch->PointsPacket();

		return 0;
	}

	// RESET_ONE_SKILL	
	int pc_clear_one_skill(lua_State* L)
	{
		int vnum = (int)lua_tonumber(L, 1);
		sys_log(0, "%d skill clear", vnum);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if ( ch == NULL )
		{
			sys_log(0, "skill clear fail");
			lua_pushnumber(L, 0);
			return 1;
		}

		sys_log(0, "%d skill clear", vnum);

		ch->ResetOneSkill(vnum);

		return 0;
	}
	// END_RESET_ONE_SKILL

	int pc_save_exit_location(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		ch->SaveExitLocation();

		return 0;
	}

	//ÅÚ·¹Æ÷Æ® 
	int pc_teleport ( lua_State * L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int x=0,y=0;	
		if ( lua_isnumber(L, 1) )
		{
			// Áö¿ª¸í ¿öÇÁ
			const int TOWN_NUM = 10;
			struct warp_by_town_name
			{
				const char* name;
				DWORD x;
				DWORD y;
			} ws[TOWN_NUM] = 
			{
				{"¿µ¾ÈÀ¾¼º",	4743,	9548},
				{"ÀÓÁö°î",		3235,	9086},
				{"ÀÚ¾çÇö",		3531,	8829},
				{"Á¶¾ÈÀ¾¼º",	638,	1664},
				{"½Â·æ°î",		1745,	1909},
				{"º¹Á¤Çö",		1455,	2400},
				{"Æò¹«À¾¼º",	9599,	2692},
				{"¹æ»ê°î",		8036,	2984},
				{"¹Ú¶óÇö",		8639,	2460},
				{"¼­ÇÑ»ê",		4350,	2143},
			};
			int idx  = (int)lua_tonumber(L, 1);

			x = ws[idx].x;
			y = ws[idx].y;
			goto teleport_area;
		}

		else
		{
			const char * arg1 = lua_tostring(L, 1);

			LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

			if (!tch)
			{
				const CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

				if (pkCCI)
				{
					if (pkCCI->bChannel != g_bChannel)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
					}
					else
					{

						PIXEL_POSITION pos;

						if (!SECTREE_MANAGER::instance().GetCenterPositionOfMap(pkCCI->lMapIndex, pos))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map (index %d)", pkCCI->lMapIndex);
						}
						else
						{
							ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", pos.x, pos.y);
							ch->WarpSet(pos.x, pos.y);
							lua_pushnumber(L, 1 );
						}
					}
				}
				else if (NULL == CHARACTER_MANAGER::instance().FindPC(arg1))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
				}

				lua_pushnumber(L, 0 );

				return 1;
			}
			else
			{
				x = tch->GetX() / 100;
				y = tch->GetY() / 100;
			}
		}

teleport_area:

		x *= 100;
		y *= 100;

		ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
		ch->WarpSet(x,y);
		ch->Stop();
		lua_pushnumber(L, 1 );
		return 1;
	}

	int pc_give_polymorph_book(lua_State* L)
	{
		if ( lua_isnumber(L, 1) != true && lua_isnumber(L, 2) != true && lua_isnumber(L, 3) != true && lua_isnumber(L, 4) != true )
		{
			sys_err("Wrong Quest Function Arguments: pc_give_polymorph_book");
			return 0;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		CPolymorphUtils::instance().GiveBook(ch, (DWORD)lua_tonumber(L, 1), (DWORD)lua_tonumber(L, 2), (unsigned char)lua_tonumber(L, 3), (unsigned char)lua_tonumber(L, 4));

		return 0;
	}

	int pc_upgrade_polymorph_book(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM pItem = CQuestManager::instance().GetCurrentItem();

		bool ret = CPolymorphUtils::instance().BookUpgrade(ch, pItem);

		lua_pushboolean(L, ret);

		return 1;
	}

	int pc_get_premium_remain_sec(lua_State* L)
	{
		int	remain_seconds	= 0;
		int	premium_type	= 0;
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1)) 
		{
			sys_err("wrong premium index (is not number)");
			return 0;
		}

		premium_type = (int)lua_tonumber(L,1);
		switch (premium_type)
		{
			case PREMIUM_EXP:
			case PREMIUM_ITEM:
			case PREMIUM_SAFEBOX:
			case PREMIUM_AUTOLOOT:
			case PREMIUM_FISH_MIND:
			case PREMIUM_MARRIAGE_FAST:
			case PREMIUM_GOLD:
				break;

			default:
				sys_err("wrong premium index %d", premium_type);
				return 0;
		}

		remain_seconds = ch->GetPremiumRemainSeconds(premium_type);

		lua_pushnumber(L, remain_seconds);
		return 1;
	}	

	int pc_send_block_mode(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		ch->SetBlockModeForce((unsigned char)lua_tonumber(L, 1));

		return 0;
	}

	int pc_change_empire(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		lua_pushnumber(L, ch->ChangeEmpire((unsigned char)lua_tonumber(L, 1)));

		return 1;
	}
	
	int pc_change_name(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

/* 		if ( ch->GetNewName().size() != 0 )
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if ( lua_isstring(L, 1) != true )
		{
			lua_pushnumber(L, 1);
			return 1;
		}

		const char * szName = lua_tostring(L, 1);

		if ( check_name(szName) == false )
		{
			lua_pushnumber(L, 2);
			return 1;
		}

		char szQuery[1024];
		snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM player WHERE name='%s'", szName);
		std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery(szQuery));

		if ( pmsg->Get()->uiNumRows > 0 )
		{
			MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

			int	count = 0;
			str_to_number(count, row[0]);

			if ( count != 0 )
			{
				lua_pushnumber(L, 3);
				return 1;
			}
		}

		DWORD pid = ch->GetPlayerID();
		db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(DWORD));
		db_clientdesc->Packet(&pid, sizeof(DWORD));


		MessengerManager::instance().RemoveAllList(ch->GetName());

#ifdef ENABLE_MESSENGER_BLOCK
		MessengerManager::instance().RemoveAllBlockList(ch->GetName());
#endif

		snprintf(szQuery, sizeof(szQuery), "UPDATE player SET name='%s' WHERE id=%u", szName, pid);
		SQLMsg * msg = DBManager::instance().DirectQuery(szQuery);
		M2_DELETE(msg);

		DWORD dwPID = ch->GetPlayerID();
		const char* szOldName = ch->GetName();
		const char* szNewName = szName;

		LogManager::instance().ChangeNameLog(ch, dwPID, szOldName, szNewName);
		LogManager::instance().CharLog(ch, 0, "CHANGE_NAME", "");

		ch->SetNewName(szName);	
		lua_pushnumber(L, 4); */
		
		
		// Hotfix prevent item copy
		if (ch)
		{
			char szQuery[1024];
			snprintf(szQuery, sizeof(szQuery), "UPDATE player SET change_name=1 WHERE id=%u", ch->GetPlayerID());
			DBManager::instance().DirectQuery(szQuery);
	
			ch->Save();
			CHARACTER_MANAGER::instance().FlushDelayedSave(ch);
			
			network::GDOutputPacket<network::GDFlushCachePacket> pdb;
			pdb->set_pid(ch->GetPlayerID());
			db_clientdesc->DBPacket(pdb);
			
			ch->GetDesc()->DelayedDisconnect(3);
			
			lua_pushnumber(L, 1);
			return 1;
		}
		
		lua_pushnumber(L, 0);
		return 0;
	}

	int pc_is_dead(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			lua_pushboolean(L, ch->IsDead());
			return 1;
		}

		lua_pushboolean(L, true);

		return 1;
	}

	int pc_reset_status( lua_State* L )
	{
		if ( lua_isnumber(L, 1) == true )
		{
			int idx = (int)lua_tonumber(L, 1);

			if ( idx >= 0 && idx < 4 )
			{
				LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
				int point = POINT_NONE;
				char buf[128];

				switch ( idx )
				{
					case 0 : point = POINT_HT; break;
					case 1 : point = POINT_IQ; break;
					case 2 : point = POINT_ST; break;
					case 3 : point = POINT_DX; break;
					default : lua_pushboolean(L, false); return 1;
				}

				int old_val = ch->GetRealPoint(point);
				int old_stat = ch->GetRealPoint(POINT_STAT);

				ch->SetRealPoint(point, 1);
				ch->SetPoint(point, ch->GetRealPoint(point));

				ch->PointChange(POINT_STAT, old_val-1);

				ch->ComputePoints();
				ch->PointsPacket();

				if ( point == POINT_HT )
				{
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
				}
				else if ( point == POINT_IQ )
				{
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				}

				switch ( idx )
				{
					case 0 :
						snprintf(buf, sizeof(buf), "reset ht(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
					case 1 :
						snprintf(buf, sizeof(buf), "reset iq(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
					case 2 :
						snprintf(buf, sizeof(buf), "reset st(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
					case 3 :
						snprintf(buf, sizeof(buf), "reset dx(%d)->1 stat_point(%d)->(%d)", old_val, old_stat, ch->GetRealPoint(POINT_STAT));
						break;
				}

				LogManager::instance().CharLog(ch, 0, "RESET_ONE_STATUS", buf);

				lua_pushboolean(L, true);
				return 1;
			}
		}

		lua_pushboolean(L, false);
		return 1;
	}

	int pc_get_ht( lua_State* L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_HT));
		return 1;
	}

	int pc_set_ht( lua_State* L )
	{
		if ( lua_isnumber(L, 1) == false )
			return 1;

		int newPoint = (int)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int usedPoint = newPoint - ch->GetRealPoint(POINT_HT);
		ch->SetRealPoint(POINT_HT, newPoint);
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}

	int pc_get_iq( lua_State* L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_IQ));
		return 1;
	}

	int pc_set_iq( lua_State* L )
	{
		if ( lua_isnumber(L, 1) == false )
			return 1;

		int newPoint = (int)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int usedPoint = newPoint - ch->GetRealPoint(POINT_IQ);
		ch->SetRealPoint(POINT_IQ, newPoint);
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}
	
	int pc_get_st( lua_State* L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_ST));
		return 1;
	}

	int pc_set_st( lua_State* L )
	{
		if ( lua_isnumber(L, 1) == false )
			return 1;

		int newPoint = (int)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int usedPoint = newPoint - ch->GetRealPoint(POINT_ST);
		ch->SetRealPoint(POINT_ST, newPoint);
		ch->PointChange(POINT_ST, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}
	
	int pc_get_dx( lua_State* L )
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetRealPoint(POINT_DX));
		return 1;
	}

	int pc_set_dx( lua_State* L )
	{
		if ( lua_isnumber(L, 1) == false )
			return 1;

		int newPoint = (int)lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int usedPoint = newPoint - ch->GetRealPoint(POINT_DX);
		ch->SetRealPoint(POINT_DX, newPoint);
		ch->PointChange(POINT_DX, 0);
		ch->PointChange(POINT_STAT, -usedPoint);
		ch->ComputePoints();
		ch->PointsPacket();
		return 1;
	}

	int pc_is_near_vid( lua_State* L )
	{
		if ( lua_isnumber(L, 1) != true || lua_isnumber(L, 2) != true )
		{
			lua_pushboolean(L, false);
		}
		else
		{
			LPCHARACTER pMe = CQuestManager::instance().GetCurrentCharacterPtr();
			LPCHARACTER pOther = CHARACTER_MANAGER::instance().Find( (DWORD)lua_tonumber(L, 1) );

			if ( pMe != NULL && pOther != NULL )
			{
				lua_pushboolean(L, (DISTANCE_APPROX(pMe->GetX() - pOther->GetX(), pMe->GetY() - pOther->GetY()) < (int)lua_tonumber(L, 2)*100));
			}
			else
			{
				lua_pushboolean(L, false);
			}
		}

		return 1;
	}

	int pc_get_socket_items( lua_State* L )
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		lua_newtable( L );

		if ( pChar == NULL ) return 1;

		int idx = 1;

		// ¿ëÈ¥¼® ½½·ÔÀº ÇÒ ÇÊ¿ä ¾øÀ» µí.
		// ÀÌ ÇÔ¼ö´Â Å»¼®¼­¿ë ÇÔ¼öÀÎ µí ÇÏ´Ù.
		for ( int i=0; i < INVENTORY_AND_EQUIP_SLOT_MAX; i++ )
		{
			LPITEM pItem = pChar->GetInventoryItem(i);

			if ( pItem != NULL )
			{
				if ( pItem->IsEquipped() == false )
				{
					int j = 0;
					for (; j < ITEM_SOCKET_MAX_NUM; j++ )
					{
						long socket = pItem->GetSocket(j);

						if ( socket > 2 && socket != ITEM_BROKEN_METIN_VNUM )
						{
							auto pItemInfo = ITEM_MANAGER::instance().GetTable( socket );
							if ( pItemInfo != NULL )
							{
								if ( pItemInfo->type() == ITEM_METIN ) break;
							}
						}
					}

					if ( j >= ITEM_SOCKET_MAX_NUM ) continue;

					lua_newtable( L );

					{
						lua_pushstring( L, CLocaleManager::instance().StringToArgument(pItem->GetName()));
						lua_rawseti( L, -2, 1 );

						lua_pushnumber( L, i );
						lua_rawseti( L, -2, 2 );
					}

					lua_rawseti( L, -2, idx++ );
				}
			}
		}

		return 1;
	}

	int pc_get_empty_inventory_count( lua_State* L )
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( pChar != NULL )
		{
			lua_pushnumber(L, pChar->CountEmptyInventory());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int pc_get_logoff_interval( lua_State* L )
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( pChar != NULL )
		{
			lua_pushnumber(L, pChar->GetLogOffInterval());
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int pc_get_player_id( lua_State* L )
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( pChar != NULL )
		{
			lua_pushnumber( L, pChar->GetPlayerID() );
		}
		else
		{
			lua_pushnumber( L, 0 );
		}

		return 1;
	}

	int pc_get_account_id( lua_State* L )
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( pChar != NULL )
		{
			if ( pChar->GetDesc() != NULL )
			{
				lua_pushnumber( L, pChar->GetAccountTable().id() );
				return 1;
			}
		}

		lua_pushnumber( L, 0 );
		return 1;
	}

	int pc_get_account( lua_State* L )
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if( NULL != pChar )
		{
			if( NULL != pChar->GetDesc() )
			{
				lua_pushstring( L, pChar->GetAccountTable().login().c_str() );
				return 1;
			}
		}

		lua_pushstring( L, "" );
		return 1;
	}

	int pc_is_riding(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if( NULL != pChar )
		{
			bool is_riding = pChar->IsRiding();

			lua_pushboolean(L, is_riding);

			return 1;
		}

		lua_pushboolean(L, false);
		return 1;
	}

	int pc_get_special_ride_vnum(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			LPITEM Unique1 = pChar->GetWear(WEAR_UNIQUE1);
			LPITEM Unique2 = pChar->GetWear(WEAR_UNIQUE2);

			if (NULL != Unique1)
			{
				if (UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup())
				{
					lua_pushnumber(L, Unique1->GetVnum());
					lua_pushnumber(L, Unique1->GetSocket(2));
					return 2;
				}
			}

			if (NULL != Unique2)
			{
				if (UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup())
				{
					lua_pushnumber(L, Unique2->GetVnum());
					lua_pushnumber(L, Unique2->GetSocket(2));
					return 2;
				}
			}
		}

		lua_pushnumber(L, 0);
		lua_pushnumber(L, 0);

		return 2;
	}

	int pc_can_warp(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			lua_pushboolean(L, pChar->CanWarp());
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int pc_dec_skill_point(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			pChar->PointChange(POINT_SKILL, -1);
		}

		return 0;
	}

	int pc_get_skill_point(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL != pChar)
		{
			lua_pushnumber(L, pChar->GetPoint(POINT_SKILL));
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int pc_get_channel_id(lua_State* L)
	{
		lua_pushnumber(L, g_bChannel);

		return 1;
	}

	int pc_give_poly_marble(lua_State* L)
	{
		const int dwVnum = lua_tonumber(L, 1);

		const CMob* MobInfo = CMobManager::instance().Get(dwVnum);

		if (NULL == MobInfo)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		if (0 == MobInfo->m_table.polymorph_item_vnum())
		{
			lua_pushboolean(L, false);
			return 1;
		}

		LPITEM item = ITEM_MANAGER::instance().CreateItem( MobInfo->m_table.polymorph_item_vnum());

		if (NULL == item)
		{
			lua_pushboolean(L, false);
			return 1;
		}

		item->SetSocket(0, dwVnum);

		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		int iEmptyCell = ch->GetEmptyInventory(item->GetSize());

		if (-1 == iEmptyCell)
		{
			M2_DESTROY_ITEM(item);
			lua_pushboolean(L, false);
			return 1;
		}

		item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyCell));

		const PC* pPC = CQuestManager::instance().GetCurrentPC();

		LogManager::instance().QuestRewardLog(pPC->GetCurrentQuestName().c_str(), ch->GetPlayerID(), ch->GetLevel(), MobInfo->m_table.polymorph_item_vnum(), dwVnum);

		lua_pushboolean(L, true);

		return 1;
	}

	int pc_get_sig_items (lua_State* L)
	{
		DWORD group_vnum = (DWORD)lua_tonumber (L, 1);
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int count = 0;
		for (int i = 0; i < ch->GetInventoryMaxNum(); ++i)
		{
			if (ch->GetInventoryItem(i) != NULL && ch->GetInventoryItem(i)->GetSIGVnum() == group_vnum)
			{
				lua_pushnumber(L, ch->GetInventoryItem(i)->GetID());
				count++;
			}
		}

		return count;
	}		

	int pc_give_award(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isstring(L, 3) )
		{
			sys_err("QUEST give award call error : wrong argument");
			lua_pushnumber (L, 0);
			return 1;
		}

		DWORD dwVnum = (int) lua_tonumber(L, 1);

		int icount = (int) lua_tonumber(L, 2);

		sys_log(0, "QUEST [award] item %d to login %s", dwVnum, ch->GetAccountTable().login().c_str());

		DBManager::instance().Query("INSERT INTO item_award (login, vnum, count, given_time, why, mall)select '%s', %d, %d, now(), '%s', 1 from DUAL where not exists (select login, why from item_award where login = '%s' and why  = '%s') ;", 
			ch->GetAccountTable().login().c_str(),
			dwVnum, 
			icount, 
			lua_tostring(L,3),
			ch->GetAccountTable().login().c_str(),
			lua_tostring(L,3));

		lua_pushnumber (L, 0);
		return 1;
	}
	int pc_give_award_socket(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isstring(L, 3) || !lua_isstring(L, 4) || !lua_isstring(L, 5) || !lua_isstring(L, 6) )
		{
			sys_err("QUEST give award call error : wrong argument");
			lua_pushnumber (L, 0);
			return 1;
		}

		DWORD dwVnum = (int) lua_tonumber(L, 1);

		int icount = (int) lua_tonumber(L, 2);

		sys_log(0, "QUEST [award] item %d to login %s", dwVnum, ch->GetAccountTable().login().c_str());

		DBManager::instance().Query("INSERT INTO item_award (login, vnum, count, given_time, why, mall, socket0, socket1, socket2)select '%s', %d, %d, now(), '%s', 1, %s, %s, %s from DUAL where not exists (select login, why from item_award where login = '%s' and why  = '%s') ;", 
			ch->GetAccountTable().login().c_str(),
			dwVnum, 
			icount, 
			lua_tostring(L,3),
			lua_tostring(L,4),
			lua_tostring(L,5),
			lua_tostring(L,6),
			ch->GetAccountTable().login().c_str(),
			lua_tostring(L,3));

		lua_pushnumber (L, 0);
		return 1;
	}

	int pc_get_informer_type(lua_State* L)	//µ¶ÀÏ ¼±¹° ±â´É
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if( pChar != NULL )
		{
			//sys_err("quest cmd test %s", pChar->GetItemAward_cmd() );
			lua_pushstring(L, pChar->GetItemAward_cmd() );
		}
		else
			lua_pushstring(L, "" );

		return 1;
	}

	int pc_get_informer_item(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();

		if( pChar != NULL )
		{
			lua_pushnumber(L, pChar->GetItemAward_vnum() );
		}
		else
			lua_pushnumber(L,0);

		return 1;
	}

	int pc_get_killee_drop_pct(lua_State* L)
	{
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER pKillee = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		int iDeltaPercent, iRandRange;
		if (NULL == pKillee)
		{
			sys_err("killee is null");
			lua_pushnumber(L, -1);
			lua_pushnumber(L, -1);

			return 2;
		}

		if (!ITEM_MANAGER::instance().GetDropPct(pKillee, pChar, iDeltaPercent, iRandRange))
		{
			lua_pushnumber(L, -1);
			lua_pushnumber(L, 1);

			return 2;
		}

		lua_pushnumber(L, iDeltaPercent);
		lua_pushnumber(L, iRandRange);
		
		return 2;
	}

	int pc_find_near_item(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("unkown argument");
			lua_pushnumber(L, 0);
			return 1;
		}
		
		LPCHARACTER pChar = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM pItem = FindItemVictim(pChar, lua_tonumber(L, 1));

		lua_pushnumber(L, pItem ? pItem->GetID() : 0);
		return 1;
	}

	int pc_set_exp_disabled(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (!lua_isboolean(L, 1) || lua_toboolean(L, 1))
				ch->SetEXPDisabled();
			else
				ch->SetEXPEnabled();
		}

		return 0;
	}

	int pc_is_exp_disabled(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
			lua_pushboolean(L, ch->IsEXPDisabled());
		else
			lua_pushboolean(L, false);

		return 1;
	}

	int pc_give_item_and_equip(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(lua_tonumber(L, 1));
			if (!pkItem)
				return 0;

			int iWearCell = pkItem->FindEquipCell(ch);
			if (iWearCell == -1)
			{
				if (!lua_isboolean(L, 2) || !lua_toboolean(L, 2))
				{
					sys_err("pc_give_and_equip: cannot find wear cell for item %u", pkItem->GetVnum());
					M2_DESTROY_ITEM(pkItem);
					return 0;
				}

				ch->AutoGiveItem(pkItem, true);
				if (lua_isboolean(L, 3) && lua_toboolean(L, 3) && pkItem)
					CQuestManager::Instance().SetCurrentItem(pkItem);
				return 0;
			}

			if (LPITEM pkCurItem = ch->GetWear(iWearCell))
				ch->UnequipItem(pkCurItem);

			if (!ch->EquipItem(pkItem, iWearCell, true))
				ch->AutoGiveItem(pkItem, true);

			if (lua_isboolean(L, 3) && lua_toboolean(L, 3) && pkItem)
				CQuestManager::Instance().SetCurrentItem(pkItem);
		}

		return 0;
	}

	int pc_set_nopacket_mode_pvp(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch && lua_isboolean(L, 1))
		{
			ch->SetPKMode(PK_MODE_FREE);
			ch->tchat("PKMODE");
			if (ch->GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
				ch->RemoveBadAffect();
			ch->SetNoPVPPacketMode(lua_toboolean(L, 1));
		}

		return 0;
	}

	int pc_is_private(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
		{
			if (lua_isnumber(L, 1))
				lua_pushboolean(L, ch->IsPrivateMap(lua_tonumber(L, 1)));
			else
				lua_pushboolean(L, ch->IsPrivateMap());
			return 1;
		}

		lua_pushboolean(L, 0);
		return 1;
	}

	int pc_can_open_safebox(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
			lua_pushboolean(L, GM::check_allow(ch->GetGMLevel(), GM_ALLOW_USE_SAFEBOX));
		else
			lua_pushboolean(L, 0);

		return 1;
	}

#ifdef __ACCE_COSTUME__
	int pc_acce_open_combine(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch) {
			sys_err("No character on a pc function!");
			return 0;
		}

		if (!ch->IsAcceWindowOpen())
		{
			ch->SetAcceWindowType(COMBINE);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "acce open combine");
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ACCE_WINDOW_ALREADY_OPEN"));
		}
		return 0;
	}

	int pc_acce_open_absorb(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch) {
			sys_err("No character on a pc function!");
			return 0;
		}

		if (!ch->IsAcceWindowOpen())
		{
			ch->SetAcceWindowType(ABSORB);
			ch->ChatPacket(CHAT_TYPE_COMMAND, "acce open absorb");
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ACCE_WINDOW_ALREADY_OPEN"));
		}

		return 0;
	}
#endif

#ifdef __PRESTIGE__
	int pc_get_max_prestige(lua_State* L)
	{
		lua_pushnumber(L, gPrestigeMaxLevel - 1);
		return 1;
	}

	int pc_get_prestige(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch ? ch->GetPrestigeLevel() : 0);
		return 1;
	}

	int pc_set_prestige(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->SetPrestigeLevel(lua_tonumber(L, 1));
		return 0;
	}
#endif

	int pc_get_max_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetInventoryMaxNum());
		return 1;
	}

	int pc_set_max_inventory(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->SetInventoryMaxNum(lua_tonumber(L, 1), INVENTORY_SIZE_TYPE_NORMAL);
		return 0;
	}

	int pc_can_unlock_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetInventoryMaxNum() < INVENTORY_MAX_NUM);
		return 1;
	}

	int pc_get_max_uppitem_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetUppitemInventoryMaxNum());
		return 1;
	}

	int pc_get_max_skillbook_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetSkillbookInventoryMaxNum());
		return 1;
	}

	int pc_get_max_stone_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetStoneInventoryMaxNum());
		return 1;
	}

	int pc_get_max_enchant_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetEnchantInventoryMaxNum());
		return 1;
	}

	int pc_set_max_uppitem_inventory(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->SetInventoryMaxNum(lua_tonumber(L, 1), INVENTORY_SIZE_TYPE_UPPITEM);
		return 0;
	}

	int pc_set_max_skillbook_inventory(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->SetInventoryMaxNum(lua_tonumber(L, 1), INVENTORY_SIZE_TYPE_SKILLBOOK);
		return 0;
	}

	int pc_set_max_stone_inventory(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->SetInventoryMaxNum(lua_tonumber(L, 1), INVENTORY_SIZE_TYPE_STONE);
		return 0;
	}

	int pc_set_max_enchant_inventory(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->SetInventoryMaxNum(lua_tonumber(L, 1), INVENTORY_SIZE_TYPE_ENCHANT);
		return 0;
	}

	int pc_can_unlock_uppitem_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetUppitemInventoryMaxNum() < UPPITEM_INV_MAX_NUM);
		return 1;
	}

	int pc_can_unlock_skillbook_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetSkillbookInventoryMaxNum() < SKILLBOOK_INV_MAX_NUM);
		return 1;
	}

	int pc_can_unlock_stone_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetStoneInventoryMaxNum() < STONE_INV_MAX_NUM);
		return 1;
	}

	int pc_can_unlock_enchant_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetEnchantInventoryMaxNum() < ENCHANT_INV_MAX_NUM);
		return 1;
	}

	int pc_get_freeable_inventory(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, INVENTORY_MAX_NUM - ch->GetInventoryMaxNum());
		return 1;
	}

#ifdef __FAKE_PC__
	int pc_spawn_copy(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		ch->SetQuestNPCID(0);

		int lMapIndex = ch->GetMapIndex();
		int lX = ch->GetX() + random_number(-300, 300);
		int lY = ch->GetY() + random_number(-300, 300);

		if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(lMapIndex, basePos))
				return 0;
			lX = basePos.x + lua_tonumber(L, 1) * 100;
			lY = basePos.y + lua_tonumber(L, 2) * 100;
		}

		LPCHARACTER npc = ch->FakePC_Owner_Spawn(lX, lY, NULL, lua_isboolean(L, 3) ? lua_toboolean(L, 3) : false, lua_isboolean(L, 4) ? lua_toboolean(L, 4) : true);
		if (!npc)
			return 0;

		ch->SetQuestNPCID(npc->GetVID());

		return 0;
	}
#endif

#ifdef __GAYA_SYSTEM__
	int pc_change_gaya(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		if (!lua_isnumber(L, 1))
			return 0;

		int iChange = lua_tonumber(L, 1);
		ch->ChangeGaya(iChange);

		return 0;
	}
#endif

	int pc_big_notice(lua_State* L)
	{
		if (!lua_isstring(L, 1))
			return 0;

		LPCHARACTER ch = CQuestManager::Instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		const char* c_pszMsg = lua_tostring(L, 1);
		ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", LC_QUEST_TEXT(ch, c_pszMsg));
		return 0;
	}

	int pc_is_fakebuff_enabled(lua_State* L)
	{
#ifdef __FAKE_BUFF__
		lua_pushboolean(L, true);
#else
		lua_pushboolean(L, false);
#endif
		return 1;
	}

#ifdef __FAKE_BUFF__
	int pc_has_fakebuff_skill(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid arguments");
			return 0;
		}

		int iSkillVnum = lua_tonumber(L, 1);
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch)
			lua_pushboolean(L, ch->GetFakeBuffSkillIdx(iSkillVnum) != FAKE_BUFF_SKILL_COUNT);
		else
			lua_pushboolean(L, false);
		return 1;
	}

	int pc_get_fakebuff_skill(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid arguments");
			return 0;
		}

		int iSkillVnum = lua_tonumber(L, 1);
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if (ch)
			lua_pushnumber(L, ch->GetFakeBuffSkillLevel(iSkillVnum));
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	int pc_set_fakebuff_skill(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid arguments");
			return 0;
		}

		int iSkillVnum = lua_tonumber(L, 1);
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
			ch->SetFakeBuffSkillLevel(lua_tonumber(L, 1), lua_tonumber(L, 2));

		return 0;
	}
#endif

	int pc_get_hwid(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushstring(L, ch ? ch->GetAccountTable().hwid().c_str() : "");
		return 1;
	}
	
	int pc_complete_dungeon(lua_State* L)
	{		
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid arguments");
			return 0;
		}
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		
		if (!ch)
			return 0;
		
		int iDungeonNum = lua_tonumber(L, 1);
		
		ch->SetDungeonComplete(iDungeonNum);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "DUNGEON_COMPLETE");
		
		return 1;
	}
	
	int pc_quest_item_receive(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid arguments");
			return 0;
		}

		int iVnum = lua_tonumber(L, 1);		
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;
		
		ch->SetItemDropQuest(iVnum);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ITEM_QUEST_DROP");

		return 1;
	}
	
	int pc_get_last_quest_item(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch ? ch->GetLastItemDropQuest() : 0);
		return 1;
	}

	int pc_get_added_friend_name(lua_State* L)
	{
		lua_pushstring(L, CQuestManager::Instance().GetAddedFriendName().c_str());
		return 1;
	}
	
#ifdef __DAMAGE_QUEST_TRIGGER__
	int pc_get_last_damage(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;
		
		lua_pushnumber(L, ch->GetQuestDamage());
		return 1;
	}
#endif

	int pc_del_quest_flags(lua_State* L)
	{
		std::string stQuestName = lua_isstring(L, 1) ? lua_tostring(L, 1) : CQuestManager::instance().GetCurrentRootPC()->GetCurrentQuestName();

		PC * pPC = CQuestManager::instance().GetCurrentPC();
		pPC->DeleteFlagsByQuest(stQuestName);
		return 0;
	}
	
#ifdef __COSTUME_BONUS_TRANSFER__
	int pc_cbt_open(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			sys_err("character pointer is NULL. Warum?");
			return 0;
		}

		ch->CBT_WindowOpen(CQuestManager::instance().GetCurrentNPCCharacterPtr());
		return 0;
	}
#endif
	int pc_reset_life(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			sys_err("character pointer is NULL. Warum?");
			return 0;
		}

		ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
		ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
		return 0;
	}

	int pc_get_last_quest_argument(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch ? ch->GetQuestArgument() : 0);
		return 1;
	}

	int pc_warp_to_target_pid(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->WarpToPID((DWORD)lua_tonumber(L, 1));
		return 1;
	}
	
	int pc_get_quest_item(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch ? ch->GetItemDropQuest() : 0);
		return 1;
	}

	int pc_get_distance(lua_State* L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
			return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		int lX = lua_tonumber(L, 1);
		int lY = lua_tonumber(L, 2);

		if (lua_isboolean(L, 3) && lua_toboolean(L, 3))
		{
			PIXEL_POSITION basePos;
			if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(ch->GetMapIndex(), basePos))
				return 0;

			lX = basePos.x + lX * 100;
			lY = basePos.y + lY * 100;
		}

		lua_pushnumber(L, DISTANCE_APPROX(ch->GetX() - lX, ch->GetY() - lY) / 100);
		return 1;
	}

	int pc_regenerate(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
		ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());

		if (lua_isboolean(L, 1) && lua_toboolean(L, 1))
		{
			ch->RemoveBadAffect();
		}

		ch->Save();	
		return 1;
	}
	
	int pc_kill(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		ch->Dead();
		return 1;
	}
	
#ifdef ENABLE_RUNE_SYSTEM
	int pc_check_rune_integrity(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		int pointsInRunes = 0;
		for (int i = 0; i < 61; ++i)
		{
			if (ch->IsRuneOwned(i))
				pointsInRunes += 12;
		}

		int currPoints = ch->GetQuestFlag("rune_manager.points_unspent");
		int shouldHave = ch->GetQuestFlag("rune_manager.points_bought");

		if (pointsInRunes + currPoints < shouldHave)
		{
			if (lua_isboolean(L, 1) && lua_toboolean(L, 1) && ch->ResetRunes())
			{
				WORD pointsBought = ch->GetQuestFlag("rune_manager.points_bought");
				ch->SetQuestFlag("rune_manager.points_unspent", pointsBought);
				ch->ChatPacket(CHAT_TYPE_COMMAND, "rune_points %u", pointsBought);
				char buf[256];
				snprintf(buf, sizeof(buf), "currPoints[%d]pointsInRunes[%d]shouldHave[%d]", currPoints, pointsInRunes, shouldHave);
				LogManager::instance().CharLog(ch, 0, "RESET_RUNES", buf);
			}
			lua_pushboolean(L, false);
		}
		else
			lua_pushboolean(L, true);

		return 1;
	}
#endif

#ifdef ENABLE_BLOCK_PKMODE
	int pc_set_block_pkmode(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
			ch->SetBlockPKMode(lua_tonumber(L, 1), lua_toboolean(L, 2));
		return 0;
	}
#endif

	int pc_open_shop(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch)
			CShopManager::instance().StartShopping(ch, lua_tonumber(L, 1));
		return 0;
	}

#ifdef ENABLE_COMPANION_NAME
	int pc_name_pet(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		const char *name = lua_tostring(L, 1);
			
		if (!check_name(name))
			return 0;

		size_t len = strlen(name);
		if (len < 2 || len > 12)
			return 0;

		ch->SetPetName(name);

		return 0;
	}

	int pc_name_mount(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		const char *name = lua_tostring(L, 1);

		if (!check_name(name))
			return 0;

		size_t len = strlen(name);
		if (len < 2 || len > 12)
			return 0;

		ch->SetMountName(name);

		return 0;
	}

	int pc_name_fakebuff(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		const char *name = lua_tostring(L, 1);

		if (!check_name(name))
			return 0;

		size_t len = strlen(name);
		if (len < 2 || len > 12)
			return 0;

		ch->SetFakebuffName(name);

		return 0;
	}

	int pc_companion_has_name(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushboolean(L, true);
			return 0;
		}

		int mode = lua_tonumber(L, 1);

		if (mode == 0)
			lua_pushboolean(L, !ch->GetPetName().empty());
		else if (mode == 1)
			lua_pushboolean(L, !ch->GetMountName().empty());
		else if (mode == 2)
			lua_pushboolean(L, (strlen(ch->FakeBuff_Owner_GetName()) > 1) ? true : false);
		else
			return 0;

		return 1;
	}
#endif

#ifdef BATTLEPASS_EXTENSION
	int pc_set_battlepass_data(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
			return 0;

		int index = lua_tonumber(L, 1) - 1; // seems to start from 1 in quests (battlepass_starter_n1 BATTLEPASS_INDEX 1)

		if (index < 7 && quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->GetCurrentQuestName().find("_l") != std::string::npos)
			index += 7;

		auto p = ch->m_battlepassData[index];
		p.set_progress(lua_tonumber(L, 2));

		p.set_name(LC_TEXT(ch, lua_tostring(L, 3)));
		p.set_task(LC_TEXT(ch, lua_tostring(L, 4)));

		p.set_reward_vnum(lua_tonumber(L, 5));
		p.set_reward_count(lua_tonumber(L, 6));

		ch->SendBattlepassData(index);

		return 0;
	}
#else
	int pc_set_battlepass_data(lua_State* L)
	{
		return 1;
	}
#endif

	int pc_security_check(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (!ch)
		{
			lua_pushboolean(L, true);
			return 1;
		}

		if (!ch->CanShopNow() || ch->GetShop() || ch->GetMyShop())
		{
			lua_pushboolean(L, true);
			return 1;
		}

		lua_pushboolean(L, false);
		return 1;
	}
	void RegisterPCFunctionTable()
	{
		luaL_reg pc_functions[] = 
		{
			{ "get_wear",		pc_get_wear			},
			{ "get_player_id",	pc_get_player_id	},
			{ "get_account_id", pc_get_account_id	},
			{ "get_account",	pc_get_account		},
			{ "get_level",		pc_get_level		},
			{ "get_max_level",	pc_get_max_level	},
			{ "set_level",		pc_set_level		},
			{ "get_next_exp",		pc_get_next_exp		},
			{ "get_exp",		pc_get_exp		},
			{ "get_job",		pc_get_job		},
			{ "get_race",		pc_get_race		},
			{ "change_sex",		pc_change_sex	},
			{ "gethp",			pc_get_hp		},
			{ "get_hp",			pc_get_hp		},
			{ "getmaxhp",		pc_get_max_hp		},
			{ "get_max_hp",		pc_get_max_hp		},
			{ "getsp",			pc_get_sp		},
			{ "get_sp",			pc_get_sp		},
			{ "getmaxsp",		pc_get_max_sp		},
			{ "get_max_sp",		pc_get_max_sp		},
			{ "change_sp",		pc_change_sp		},
			{ "getmoney",		pc_get_money		},
			{ "get_money",		pc_get_money		},
			{ "get_real_alignment",	pc_get_real_alignment	},
			{ "get_alignment",		pc_get_alignment	},
			{ "getweapon",		pc_get_weapon		},
			{ "get_weapon",		pc_get_weapon		},
			{ "getarmor",		pc_get_armor		},
			{ "get_armor",		pc_get_armor		},
			{ "getgold",		pc_get_money		},
			{ "get_gold",		pc_get_money		},
			{ "changegold",		pc_change_money		},
			{ "changemoney",		pc_change_money		},
			{ "changealignment",	pc_change_alignment	},
			{ "change_gold",		pc_change_money		},
			{ "change_money",		pc_change_money		},
			{ "change_alignment",	pc_change_alignment	},
			{ "getname",		pc_get_name		},
			{ "get_name",		pc_get_name		},
			{ "get_vid",		pc_get_vid		},
			{ "getplaytime",		pc_get_playtime		},
			{ "get_playtime",		pc_get_playtime		},
			{ "getleadership",		pc_get_leadership	},
			{ "get_leadership",		pc_get_leadership	},
			{ "getqf",			pc_get_quest_flag	},
			{ "setqf",			pc_set_quest_flag	},
			{ "incqf",			pc_inc_quest_flag	},
			{ "delqf",			pc_del_quest_flag	},
			{ "resetqf",		pc_del_quest_flags	},
			{ "getf",			pc_get_another_quest_flag},
			{ "setf",			pc_set_another_quest_flag},
			{ "use_gold_item",	pc_use_gold_item	},
			{ "get_x",			pc_get_x		},
			{ "get_y",			pc_get_y		},
			{ "getx",			pc_get_x		},
			{ "gety",			pc_get_y		},
			{ "get_local_x",		pc_get_local_x		},
			{ "get_local_y",		pc_get_local_y		},
			{ "getcurrentmapindex",	pc_get_current_map_index},
			{ "get_map_index",		pc_get_current_map_index},
			{ "give_exp",		pc_give_exp		},
			{ "give_exp_perc",		pc_give_exp_perc	},
			{ "give_exp2",		pc_give_exp2		},
			{ "give_item",		pc_give_item		},
			{ "give_item2",		pc_give_or_drop_item	},
			{ "give_item2_select",		pc_give_or_drop_item_and_select	},
			{ "give_gold",		pc_give_gold		},
			{ "count_item",		pc_count_item		},
			{ "remove_item",		pc_remove_item		},
			{ "countitem",		pc_count_item		},
			{ "removeitem",		pc_remove_item		},
			{ "reset_point",		pc_reset_point		},
			{ "has_guild",		pc_hasguild		},
			{ "hasguild",		pc_hasguild		},
			{ "get_guild",		pc_getguild		},
			{ "getguild",		pc_getguild		},
			{ "isguildmaster",		pc_isguildmaster	},
			{ "is_guild_master",	pc_isguildmaster	},
			{ "destroy_guild",		pc_destroy_guild	},
			{ "remove_from_guild",	pc_remove_from_guild	},
			{ "in_dungeon",		pc_in_dungeon		},
			{ "getempire",		pc_get_empire		},
			{ "get_empire",		pc_get_empire		},
			{ "get_skill_group",	pc_get_skillgroup	},
			{ "set_skill_group",	pc_set_skillgroup	},
			{ "warp",			pc_warp			},
			{ "warp_base",		pc_warp_base	},
			{ "warp_local",		pc_warp_local		},
			{ "warp_exit",		pc_warp_exit		},
			{ "set_warp_location",	pc_set_warp_location	},
			{ "set_warp_location_local",pc_set_warp_location_local },
			{ "get_start_location",	pc_get_start_location	},
			{ "has_master_skill",	pc_has_master_skill	},
			{ "set_part",		pc_set_part		},
			{ "get_part",		pc_get_part		},
			{ "is_polymorphed",		pc_is_polymorphed	},
			{ "remove_polymorph",	pc_remove_polymorph	},
			{ "polymorph",		pc_polymorph		},
			{ "warp_to_guild_war_observer_position", pc_warp_to_guild_war_observer_position	},
			{ "give_item_from_special_item_group", pc_give_item_from_special_item_group	},
			{ "learn_grand_master_skill", pc_learn_grand_master_skill	},
#ifdef __LEGENDARY_SKILL__
			{ "learn_legendary_skill", pc_learn_legendary_skill	},
#endif
			{ "is_skill_book_no_delay",	pc_is_skill_book_no_delay}, 
			{ "remove_skill_book_no_delay",	pc_remove_skill_book_no_delay}, 

			{ "is_skill_book_no_delay2",	pc_is_skill_book_no_delay2 },
			{ "remove_skill_book_no_delay2",	pc_remove_skill_book_no_delay2 },

			{ "enough_inventory",	pc_enough_inventory	},
			{ "have_pos_scroll",	pc_have_pos_scroll	},
			{ "have_map_scroll",	pc_have_map_scroll	},
			{ "get_war_map",		pc_get_war_map		},
			{ "get_equip_refine_level",	pc_get_equip_refine_level },
			{ "refine_equip",		pc_refine_equip		},
			{ "get_skill_level",	pc_get_skill_level	},
			{ "aggregate_monster",	pc_aggregate_monster	},
			{ "forget_my_attacker",	pc_forget_my_attacker	},
			{ "pc_attract_ranger",	pc_attract_ranger	},
			{ "select",			pc_select_vid		},
			{ "select_pid",		pc_select_pid		},
			{ "get_sex",		pc_get_sex		},
			{ "is_married",		pc_is_married		},
			{ "is_engaged",		pc_is_engaged		},
			{ "is_engaged_or_married",	pc_is_engaged_or_married},
			{ "is_gm",			pc_is_gm		},
			{ "get_gm_level",		pc_get_gm_level		},
			{ "mining",			pc_mining		},
			{ "ore_refine",		pc_ore_refine		},
			{ "diamond_refine",		pc_diamond_refine	},
#ifdef ENABLE_ZODIAC_TEMPLE
			{ "get_animasphere",			pc_get_animasphere	},
			{ "change_animasphere",			pc_change_animasphere	},
#endif
			// RESET_ONE_SKILL
			{ "clear_one_skill",		pc_clear_one_skill	  },
			// END_RESET_ONE_SKILL

			{ "clear_skill",				pc_clear_skill		  },
			{ "clear_sub_skill",	pc_clear_sub_skill	  },
			{ "set_skill_point",	pc_set_skill_point	  },

			{ "save_exit_location",		pc_save_exit_location		},
			{ "teleport",				pc_teleport },

			{ "set_skill_level",		pc_set_skill_level	  },

			{ "give_polymorph_book",	pc_give_polymorph_book  },
			{ "upgrade_polymorph_book", pc_upgrade_polymorph_book },
			{ "get_premium_remain_sec", pc_get_premium_remain_sec },
   
			{ "send_block_mode",		pc_send_block_mode	},
			
			{ "change_empire",			pc_change_empire	},

			{ "change_name",			pc_change_name },
			#ifdef ENABLE_MESSENGER_BLOCK
			{ "is_blocked",			pc_is_blocked },
			{ "is_friend",			pc_is_friend },
			#endif

			{ "is_dead",				pc_is_dead	},

			{ "reset_status",		pc_reset_status	},
			{ "get_ht",				pc_get_ht	},
			{ "set_ht",				pc_set_ht	},
			{ "get_iq",				pc_get_iq	},
			{ "set_iq",				pc_set_iq	},
			{ "get_st",				pc_get_st	},
			{ "set_st",				pc_set_st	},
			{ "get_dx",				pc_get_dx	},
			{ "set_dx",				pc_set_dx	},

			{ "is_near_vid",		pc_is_near_vid	},

			{ "get_socket_items",	pc_get_socket_items	},
			{ "get_empty_inventory_count",	pc_get_empty_inventory_count	},

			{ "get_logoff_interval",	pc_get_logoff_interval	},

			{ "is_riding",			pc_is_riding	},
			{ "get_special_ride_vnum",	pc_get_special_ride_vnum	},

			{ "can_warp",			pc_can_warp		},

			{ "dec_skill_point",	pc_dec_skill_point	},
			{ "get_skill_point",	pc_get_skill_point	},

			{ "get_channel_id",		pc_get_channel_id	},

			{ "give_poly_marble",	pc_give_poly_marble	},
			{ "get_sig_items",		pc_get_sig_items	},
			
			{ "get_informer_type",	pc_get_informer_type	},	//µ¶ÀÏ ¼±¹° ±â´É
			{ "get_informer_item",  pc_get_informer_item	},

			{ "give_award",			pc_give_award			},	//ÀÏº» °èÁ¤´ç ÇÑ¹ø¾¿ ±Ý±« Áö±Þ
			{ "give_award_socket",	pc_give_award_socket	},	//¸ô ÀÎº¥Åä¸®¿¡ ¾ÆÀÌÅÛ Áö±Þ. ¼ÒÄÏ ¼³Á¤À» À§ÇÑ ÇÔ¼ö.

			{ "get_killee_drop_pct",	pc_get_killee_drop_pct	}, /* mob_vnum.kill ÀÌº¥Æ®¿¡¼­ killee¿Í pc¿ÍÀÇ level Â÷ÀÌ, pcÀÇ ÇÁ¸®¹Ì¾ö µå¶ø·ü µîµîÀ» °í·ÁÇÑ ¾ÆÀÌÅÛ µå¶ø È®·ü.
																	* return °ªÀº (ºÐÀÚ, ºÐ¸ð).
																	* (¸»ÀÌ º¹ÀâÇÑµ¥, CreateDropItemÀÇ GetDropPctÀÇ iDeltaPercent, iRandRange¸¦ returnÇÑ´Ù°í º¸¸é µÊ.)
																	* (ÀÌ ¸»ÀÌ ´õ ¾î·Á¿ï¶ó³ª ¤Ð¤Ð)
																	* ÁÖÀÇ»çÇ× : kill event¿¡¼­¸¸ »ç¿ëÇÒ °Í!
																	*/

			{ "find_near_item",		pc_find_near_item		},
			{ "set_exp_disabled",	pc_set_exp_disabled		},
			{ "is_exp_disabled",	pc_is_exp_disabled		},

			{ "give_and_equip",		pc_give_item_and_equip	},
			{ "is_private",			pc_is_private			},

			{ "can_open_safebox",	pc_can_open_safebox		},

#ifdef __ACCE_COSTUME__
			{ "acce_open_combine",	pc_acce_open_combine	},
			{ "acce_open_absorb",	pc_acce_open_absorb		},
#endif

#ifdef __PRESTIGE__
			{ "get_max_prestige",	pc_get_max_prestige		},
			{ "get_prestige",		pc_get_prestige			},
			{ "set_prestige",		pc_set_prestige			},
#endif

			{ "get_max_inventory",	pc_get_max_inventory	},
			{ "set_max_inventory",	pc_set_max_inventory	},
			{ "can_unlock_inventory",	pc_can_unlock_inventory	},
			{ "get_freeable_inventory",	pc_get_freeable_inventory	},

#ifdef __FAKE_PC__
			{ "spawn_copy",			pc_spawn_copy			},
#endif

#ifdef __GAYA_SYSTEM__
			{ "change_gaya",		pc_change_gaya	},
#endif
			{ "big_notice",				pc_big_notice				},

			{ "is_fakebuff_enabled",	pc_is_fakebuff_enabled	},
#ifdef __FAKE_BUFF__
			{ "has_fakebuff_skill",		pc_has_fakebuff_skill	},
			{ "get_fakebuff_skill",		pc_get_fakebuff_skill	},
			{ "set_fakebuff_skill",		pc_set_fakebuff_skill	},
#endif

			{ "get_hwid",				pc_get_hwid				},
			{ "complete_dungeon",		pc_complete_dungeon		},
			{ "quest_item_receive",		pc_quest_item_receive	},
			{ "get_last_quest_item",	pc_get_last_quest_item	},

			{ "get_added_friend_name",	pc_get_added_friend_name	},
#ifdef __DAMAGE_QUEST_TRIGGER__
			{ "get_last_damage",		pc_get_last_damage		},
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
			{ "cbt_open",				pc_cbt_open },
#endif
			{ "reset_life",				pc_reset_life },

			{ "get_max_uppitem_inventory", pc_get_max_uppitem_inventory },
			{ "set_max_uppitem_inventory", pc_set_max_uppitem_inventory }, 
			{ "can_unlock_uppitem_inventory", pc_can_unlock_uppitem_inventory },

			{ "get_max_skillbook_inventory", pc_get_max_skillbook_inventory },
			{ "set_max_skillbook_inventory", pc_set_max_skillbook_inventory }, 
			{ "can_unlock_skillbook_inventory", pc_can_unlock_skillbook_inventory },

			{ "get_max_stone_inventory", pc_get_max_stone_inventory },
			{ "set_max_stone_inventory", pc_set_max_stone_inventory }, 
			{ "can_unlock_stone_inventory", pc_can_unlock_stone_inventory },

			{ "get_max_enchant_inventory", pc_get_max_enchant_inventory },
			{ "set_max_enchant_inventory", pc_set_max_enchant_inventory }, 
			{ "can_unlock_enchant_inventory", pc_can_unlock_enchant_inventory },

			{ "refresh_timer_cdrs", pc_refresh_timer_cdrs },

			{ "set_nopacket_mode_pvp", pc_set_nopacket_mode_pvp },

			{ "get_last_quest_argument", pc_get_last_quest_argument },

			{ "warp_to_target_pid",			pc_warp_to_target_pid },
			
			{ "get_quest_item",			pc_get_quest_item },
			{ "get_distance",			pc_get_distance },
			{ "regenerate",				pc_regenerate },
			{ "direction_effect",				pc_direction_effect },
			{ "kill",				pc_kill },
#ifdef ENABLE_RUNE_SYSTEM
			{ "check_rune_integrity",	pc_check_rune_integrity },
#endif
#ifdef ENABLE_BLOCK_PKMODE
			{ "set_block_pkmode",	pc_set_block_pkmode },
#endif
#ifdef ENABLE_COMPANION_NAME
			{ "name_pet",	pc_name_pet },
			{ "name_mount",	pc_name_mount },
			{ "name_fakebuff",	pc_name_fakebuff },
			{ "companion_has_name", pc_companion_has_name },
#endif

			{ "buffi_gender", pc_buffi_gender },
			{ "SetBattlepassData",	pc_set_battlepass_data },
			{ "update_dungeon_rank", pc_update_dungeon_rank },
			{ "open_shop", pc_open_shop },
			{ "IsUnsecure", pc_security_check },

			{ NULL,			NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("pc", pc_functions);
	}
};
