#include "stdafx.h"
#ifdef __FreeBSD__
#include <md5.h>
#else
#include "../../libthecore/include/xmd5.h"
#endif

#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "affect.h"
#include "pvp.h"
#include "start_position.h"
#include "party.h"
#include "guild_manager.h"
#include "p2p.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "war_map.h"
#include "questmanager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "dev_log.h"
#include "item.h"
#include "arena.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "log.h"
#include "../../common/VnumHelper.h"
#include "mount_system.h"
#include "general_manager.h"
#include "refine.h"
#include "shop.h"

#ifdef __GUILD_SAFEBOX__
#include "guild_safebox.h"
#endif
#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif
#ifdef __ATTRTREE__
#include "attrtree_manager.h"
#endif

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef DMG_RANKING
#include "dmg_ranking.h"
#endif

#ifdef BATTLEPASS_EXTENSION
#include "shop_manager.h"
#endif

extern int g_server_id;

extern int g_nPortalLimitTime;

#ifdef __TIMER_BIO_DUNGEON__
const std::string bioTimerCdrList[] = {
	"collect_quest_lv30",
	"collect_quest_lv40",
	"collect_quest_lv50",
	"collect_quest_lv60",
	"collect_quest_lv70",
	"collect_quest_lv80",
	"collect_quest_lv85",
	"collect_quest_lv90",
	"collect_quest_lv92",
	"collect_quest_lv94",
#ifdef AELDRA
	"collect_quest_lv108",
	"collect_quest_lv112",
#endif
};

const std::string dungeonTimerCdrList[] = {
#ifdef AELDRA
	"orkmaze.next_entry",
#endif
#ifdef ELONIA
	"apedungeon.next_entry",
	"demontower.next_entry",
#endif
	"spider_dungeon.next_entry",
	"devilcatacomb_zone.next_entry",
	"dragon_dungeon.next_entry",
	"snow_dungeon.next_entry",
	"flame_dungeon.next_entry",
#ifdef AELDRA
	"hydra_dungeon.next_entry",
	"enchanted_forest_zone.next_entry",
	"crystal_dungeon.next_entry",
	"meleylair_zone.next_entry",
	"thraduils.next_entry",
	"zodiac.placeholder",
	"slime_dungeon.next_entry",
	"infected_garden.next_entry",
#endif
#ifdef ELONIA
	"apedungeon.next_entry"
#endif
};

void RefreshTimerCDRs_G(LPCHARACTER ch)
{
	quest::PC* curr = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (!curr)
		return;

	int dur = 0;
	std::string currBio;
	for (size_t i = 0; i < sizeof(bioTimerCdrList) / sizeof(std::string); ++i)
	{
		int currState = curr->GetFlag((bioTimerCdrList[i] + ".__status").c_str());
		if (currState == quest::CQuestManager::instance().GetQuestStateIndex(bioTimerCdrList[i], "go_to_disciple"))
		{
			dur = curr->GetFlag((bioTimerCdrList[i] + ".duration").c_str()) - get_global_time();
			currBio = &(bioTimerCdrList[i].c_str()[bioTimerCdrList[i].length() - 2]);
			break;
		}
	}
	ch->ChatPacket(CHAT_TYPE_COMMAND, "timer_cdr 0 %d", dur);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "curr_biolog %s", currBio.length() ? currBio.c_str() : "0");
	
	for (size_t i = 0; i < sizeof(dungeonTimerCdrList) / sizeof(std::string); ++i)
	{
		dur = curr->GetFlag(dungeonTimerCdrList[i].c_str()) - get_global_time();
		ch->ChatPacket(CHAT_TYPE_COMMAND, "timer_cdr %d %d", i+1, dur > 0 ? dur : 0);
	}

	// dur = 0;
	// if (ch->GetGuild())
		// dur = ch->GetGuild()->GetDungeonCooldown() - get_global_time();
	// ch->ChatPacket(CHAT_TYPE_COMMAND, "timer_cdr %d %d", sizeof(dungeonTimerCdrList) / sizeof(std::string) + 1, dur);
}

ACMD(do_get_timer_cdrs)
{
	RefreshTimerCDRs_G(ch);
}

ACMD(do_premium_color)
{
	BYTE iReq;
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(iReq, arg1))
	{
		ch->tchat("usage: premium_color <number>");
		return;
	}

	quest::PC* cPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (!cPC
#ifndef ELONIA
	 || !quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->GetFlag("auction_premium.premium_active")
#endif
	 )
	{
		ch->tchat("premium is not active");
		return;
	}

	cPC->SetFlag("premium.chat_color", iReq);
}

extern bool CAN_ENTER_ZONE_CHECKLEVEL(const LPCHARACTER& ch, int map_index, bool bSendMessage);

ACMD(do_timer_warp)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_timer_warp") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This doesn't work currently!");
		return;
	}
	
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int iReq;
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(iReq, arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: timer_warp <pos>");
		return;
	}
	
	int lMapIndex = 0;
	int x = 0;
	int y = 0;

	switch(iReq)
	{
#ifdef AELDRA
		// Biologist
		case 0:
		{		
			if (ch->GetEmpire() == 1)
			{
				lMapIndex = 1;
				x = 881;
				y = 610;
			}
			else if (ch->GetEmpire() == 2)
			{
				lMapIndex = 3;
				x = 885;
				y = 797;
			}
			else if (ch->GetEmpire() == 3)
			{
				lMapIndex = 5;
				x = 301;
				y = 282;
			}
			break;
		}
		
		// Orc Maze
		case 1:
		{
			lMapIndex = THREEWAY_MAP_INDEX;
			x = 283;
			y = 1448;
			break;
		}
		
		// Spider Queen
		case 2:
		{
			lMapIndex = 19;
			x = 178;
			y = 469;
			break;
		}
		
		// Azrael
		case 3:
		{
			lMapIndex = 14;
			x = 540;
			y = 488;
			break;
		}
		
		// Dragon
		case 4:
		{
			lMapIndex = 21;
			x = 278;
			y = 174;
			break;
		}
		
		// Nemere
		case 5:
		{
			lMapIndex = 10;
			x = 736;
			y = 114;
			break;
		}
		
		// Razador
		case 6:
		{
			lMapIndex = 11;
			x = 251;
			y = 926;
			break;
		}

		// Hydra
		case 7:
		{
			lMapIndex = 54;
			x = 410;
			y = 490;
			break;
		}

		// Erebos
		case 8:
		{
			lMapIndex = 26;
			x = 603;
			y = 105;
			break;
		}

		// Crystal
		case 9:
		{
			lMapIndex = 24;
			x = 645;
			y = 80;
			break;
		}
		
		// Meley
		case 10:
		{
			lMapIndex = 11;
			x = 85;
			y = 855;
			break;
		}
		
		// Thranduil
		case 11:
		{
			lMapIndex = 28;
			x = 145;
			y = 419;
			break;
		}

#ifdef ENABLE_ZODIAC_TEMPLE
		case 12:
		{
			lMapIndex = 69;
			x = 1797;
			y = 242;
			break;
		}
#endif

		// Slime
		case 13:
		{
			lMapIndex = 14;
			x = 560;
			y = 783;
			break;
		}

		// Infected Garden
		case 14:
		{
			lMapIndex = 58;
			x = 146;
			y = 185;
			break;
		}
#elif defined(ELONIA)
		// Biologist
		case 0:
		{		
			if (ch->GetEmpire() == 1)
			{
				lMapIndex = 1;
				x = 881;
				y = 610;
			}
			else if (ch->GetEmpire() == 2)
			{
				lMapIndex = 3;
				x = 885;
				y = 797;
			}
			else if (ch->GetEmpire() == 3)
			{
				lMapIndex = 5;
				x = 301;
				y = 282;
			}
			break;
		}

		// Ape
		case 1:
		{
			lMapIndex = THREEWAY_MAP_INDEX;
			x = 283;
			y = 1448;
			break;
		}
		
		// Demontower
		case 2:
		{
			lMapIndex = 14;
			x = 529;
			y = 600;
			break;
		}
		
		// Spider Queen
		case 3:
		{
			lMapIndex = 19;
			x = 178;
			y = 469;
			break;
		}
		
		// Azrael
		case 4:
		{
			lMapIndex = 14;
			x = 540;
			y = 488;
			break;
		}
		
		// Dragon
		case 5:
		{
			lMapIndex = 21;
			x = 278;
			y = 174;
			break;
		}
		
		// Nemere
		case 6:
		{
			lMapIndex = 10;
			x = 736;
			y = 114;
			break;
		}
		
		// Razador
		case 7:
		{
			lMapIndex = 11;
			x = 251;
			y = 926;
			break;
		}
#endif

		default:
		{
			sys_err("timer_warp wrong index %i", iReq);
			return;			
		}
	}
	
	if (!CAN_ENTER_ZONE_CHECKLEVEL(ch, lMapIndex, true))
	{
		return;
	}
	
	x *= 100;
	y *= 100;
	
	const TMapRegion * region = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex);

	if (!region)
	{
		sys_err("invalid map index %d", lMapIndex);
		return;
	}

	if (x > region->ex - region->sx)
	{
		sys_err("x coordinate overflow max: %d input: %d", region->ex - region->sx, x);
		return;
	}

	if (y > region->ey - region->sy)
	{
		sys_err("y coordinate overflow max: %d input: %d", region->ey - region->sy, y);
		return;
	}

	if (ch->GetGold() < 250000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You don't have 250.000 Yang."));
		return;
	}
	
	bool bRet = ch->WarpSet(region->sx + x, region->sy + y, 0);
	if (!bRet)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You warp to this location now."));
	}
	else
	{
		ch->PointChange(POINT_GOLD, -250000);
	}
}
#endif


#ifdef ENABLE_RUNE_SYSTEM
ACMD(do_open_rune_points_buy)
{
	if (!ch->GetDesc())
		return;

	WORD nextPoint = ch->GetQuestFlag("rune_manager.points_bought") + 1;
	DWORD nextProto = CRuneManager::instance().FindPointProto(nextPoint);
	if (!nextProto)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "There is no more points you can buy."));
		return;
	}

	auto pRefineTab = CRefineManager::instance().GetRefineRecipe(nextProto);
	if (!pRefineTab)
		return;

	if (test_server && ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "send refine info for id %u", nextPoint);

	network::GCOutputPacket<network::GCRuneRefinePacket> pack;
	pack->set_next_point(nextPoint);
	*pack->mutable_refine_table() = *pRefineTab;
	ch->GetDesc()->Packet(pack);
}

ACMD(do_reset_runes)
{
	if (!ch || !ch->GetDesc())
		return;

#ifdef REQ_ITEM_RESET_RUNE
	if (ch->GetExchange() || ch->GetShopOwner() || !ch->CanHandleItem())
		return;

	if (!ch->CountSpecifyItem(93267))
	{
		auto p = ITEM_MANAGER::instance().GetTable(93267);
		if (p)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need %s in order to reset the runes"), p->locale_name(ch->GetLanguageID()).c_str());
		return;
	}

#endif

#ifdef ENABLE_LEVEL2_RUNES
	if (ch->GetQuestFlag("rune_manager.points_bought") == 720)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You already have all the Runes, you don't need to Reset your Points."));
		return;
	}
#endif

	if (ch->ResetRunes())
	{
#ifdef REQ_ITEM_RESET_RUNE
		ch->RemoveSpecifyItem(93267);
#endif
		WORD pointsBought = ch->GetQuestFlag("rune_manager.points_bought");
		ch->SetQuestFlag("rune_manager.points_unspent", pointsBought);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "rune_points %u", pointsBought);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Error: Can't reset runes. Try select a rune page, logout for 10 minutes or buy a new point and retry."));
	}
}

ACMD(do_accept_rune_points_buy)
{
	if (!ch->GetDesc())
		return;

	WORD nextPoint = ch->GetQuestFlag("rune_manager.points_bought") + 1;
	DWORD nextProto = CRuneManager::instance().FindPointProto(nextPoint);
	if (!nextProto)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "There is no more points you can buy."));
		return;
	}

	auto pRefineTab = CRefineManager::instance().GetRefineRecipe(nextProto);
	if (!pRefineTab)
		return;

	if (ch->GetGold() < pRefineTab->cost())
	{
		sys_log(0, "attrtree_levelup %s : not enough gold (%lld < %d)", ch->GetName(), ch->GetGold(), pRefineTab->cost());
		return;
	}

	for (int i = 0; i < pRefineTab->material_count(); ++i)
	{
		if (ch->CountSpecifyItem(pRefineTab->materials(i).vnum()) < pRefineTab->materials(i).count())
		{
			sys_log(0, "attrtree_levelup %s : not enough item (vnum %u count %d < %d)",
				ch->GetName(), pRefineTab->materials(i).vnum(), ch->CountSpecifyItem(pRefineTab->materials(i).vnum()), pRefineTab->materials(i).count());
			return;
		}
	}

	ch->PointChange(POINT_GOLD, -pRefineTab->cost());
	for (int i = 0; i < pRefineTab->material_count(); ++i)
		ch->RemoveSpecifyItem(pRefineTab->materials(i).vnum(), pRefineTab->materials(i).count());

	int currPoints = ch->GetQuestFlag("rune_manager.points_unspent") + 1;
	ch->SetQuestFlag("rune_manager.points_bought", nextPoint);
	ch->SetQuestFlag("rune_manager.points_unspent", currPoints);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "rune_points %u", currPoints);
}

ACMD(do_use_buy_rune)
{
	if (!ch->GetDesc())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD dwVnum;
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(dwVnum, arg1))
	{
		ch->tchat("usage: use_buy_rune <vnum>");
		return;
	}

	int currPoints = ch->GetQuestFlag("rune_manager.points_unspent");
	if (currPoints < 12)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need at least 12 points to buy a rune."));
		return;
	}

	ch->SetRuneOwned(dwVnum);
	ch->SetQuestFlag("rune_manager.points_unspent", currPoints - 12);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "rune_points %u", currPoints - 12);
}
#endif

#ifdef SOUL_SYSTEM
ACMD(do_refine_soul_item_info)
{
	if (!ch->GetDesc())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	WORD pos;
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(pos, arg1))
	{
		ch->tchat("usage: refine_soul_item <pos>");
		return;
	}

	LPITEM item = ch->GetInventoryItem(pos);
	const network::TSoulProtoTable* pTable;
	if (!item || item->GetType() != ITEM_SOUL || !item->GetRefinedVnum() || !(pTable = ITEM_MANAGER::Instance().GetSoulProto(item->GetRefinedVnum())))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't upgrade this item."));
		return;
	}

	auto refTable = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());
	if (!refTable)
		return;

	network::GCOutputPacket<network::GCSoulRefineInfoPacket> pack;
	pack->set_vnum(item->GetRefinedVnum());
	pack->set_apply_type(pTable->apply_type());
	pack->set_type(pTable->soul_type());
	*pack->mutable_apply_values() = pTable->apply_values();
	*pack->mutable_refine() = *refTable;
	ch->GetDesc()->Packet(pack);
}

ACMD(do_refine_soul_item)
{
	if (!ch->GetDesc())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	WORD pos;
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(pos, arg1))
	{
		ch->tchat("usage: refine_soul_item <pos>");
		return;
	}

	LPITEM item = ch->GetInventoryItem(pos);
	const network::TSoulProtoTable* pTable;
	if (!item || item->GetType() != ITEM_SOUL || !item->GetRefinedVnum() || !(pTable = ITEM_MANAGER::Instance().GetSoulProto(item->GetRefinedVnum())))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't upgrade this item."));
		return;
	}

	auto refTable = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());
	if (!refTable)
		return;

	if (ch->GetGold() < refTable->cost())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You don't have enough money to upgrade this item."));
		return;
	}

	for (int i = 0; i < refTable->material_count(); ++i)
		if (ch->CountSpecifyItem(refTable->materials(i).vnum(), item) < refTable->materials(i).count())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You don't have the necessary materials to upgrade this item."));
			return;
		}

	ch->PointChange(POINT_GOLD, -refTable->cost());
	for (int i = 0; i < refTable->material_count(); ++i)
		ch->RemoveSpecifyItem(refTable->materials(i).vnum(), refTable->materials(i).count(), item);

#ifdef ENABLE_UPGRADE_STONE
	int add = 0;

	if (ch->GetQuestFlag("upgrade_stone.use"))
	{
		add += 5;
		ch->SetQuestFlag("upgrade_stone.use", 0);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetUpgradeBonus 0");
	}

	if (random_number(1, 100) <= refTable->prob() + add)
#else
	if (random_number(1, 100) <= refTable->prob())
#endif
	{
		LPITEM result = ITEM_MANAGER::Instance().CreateItem(item->GetRefinedVnum());
		result->SetGMOwner(item->IsGMOwner());
		item->SetCount(item->GetCount() - 1);
		result->AddToCharacter(ch, TItemPos(INVENTORY, pos));
		ch->ChatPacket(CHAT_TYPE_COMMAND, "result_soul_refine 1");
	}
	else
		ch->ChatPacket(CHAT_TYPE_COMMAND, "result_soul_refine 0");
}
#endif

ACMD(do_user_mount_set)
{
	CMountSystem* pkMountSystem = ch->GetMountSystem();
	if (!pkMountSystem)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int iCell;
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(iCell, arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: user_mount_set <cell>");
		return;
	}

	if (iCell < 0 || iCell >= ch->GetInventoryMaxNum())
		return;

	LPITEM pItem = ch->GetInventoryItem(iCell);
	if (!pItem)
		return;

	if (pItem->GetType() != ITEM_MOUNT)
	{
		sys_err("hack: cannot set no-mount-item to horse [item[%u] %d %s, owner %d %s]",
			pItem->GetID(), pItem->GetVnum(), pItem->GetName(), ch->GetPlayerID(), ch->GetName());
		return;
	}

	if (!pkMountSystem->IsSummoned() && !pkMountSystem->IsRiding())
	{
		if (ch->GetSP() < 200)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need %d mana to summon your mount."), 200);
			return;
		}

		ch->PointChange(POINT_SP, -200);
	}

	if (pkMountSystem->IsRiding())
		pkMountSystem->StopRiding();
	if (pkMountSystem->IsSummoned())
		pkMountSystem->Unsummon();
	pkMountSystem->Summon(pItem);

	ch->ChatPacket(CHAT_TYPE_COMMAND, "set_mount %d", iCell);
}

ACMD(do_user_mount_action)
{	
	CMountSystem* pkMountSystem = ch->GetMountSystem();
	if (!pkMountSystem)
		return;

	if (*argument && strcmp(argument + 1, "back") == 0)
	{
		if (pkMountSystem->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to dismount first."));
			return;
		}
		else if (!pkMountSystem->IsSummoned())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have no mount summoned."));
			return;
		}

		pkMountSystem->Unsummon();
		return;
	}
	else if (*argument && strcmp(argument + 1, "summon") == 0)
	{
		LPITEM pkItem;
		if (pkMountSystem->GetSummonItemID() == 0 || !(pkItem = ITEM_MANAGER::Instance().Find(pkMountSystem->GetSummonItemID())))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have no mount selected."));
			return;
		}
		else if (pkMountSystem->IsRiding() || pkMountSystem->IsSummoned())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your mount is already summoned."));
			return;
		}

		if (ch->GetSP() < 200)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need %d mana to summon your mount."), 200);
			return;
		}

		ch->PointChange(POINT_SP, -200);
		pkMountSystem->Summon(pkItem);
		return;
	}
	else if (*argument && strcmp(argument + 1, "feed") == 0)
	{
		if (!pkMountSystem->IsSummoned() || !ch->IsHorseSummoned())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to summon your horse to feed it."));
			return;
		}

		if (ch->IsHorseDead())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your cannot feed a dead horse."));
			return;
		}

		for (int i = 0; i < ch->GetInventoryMaxNum(); ++i)
		{
			LPITEM pkItem = ch->GetInventoryItem(i);
			if (!pkItem || pkItem->GetType() != ITEM_MOUNT || pkItem->GetSubType() != MOUNT_SUB_FOOD)
				continue;

			if (pkItem->GetValue(0) != ch->GetHorseGrade())
				continue;

			int iFeedPct = pkItem->GetValue(0);
			if (ch->HorseFeed(iFeedPct,0))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸»¿¡°Ô »ç·á¸¦ ÁÖ¾ú½À´Ï´Ù."));
				pkItem->SetCount(pkItem->GetCount() - 1);
				return;
			}
		}

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have no item to feed your horse."));
		return;
	}

	if (pkMountSystem->IsRiding())
		pkMountSystem->StopRiding();
	else if (pkMountSystem->IsSummoned())
	{
		ch->tchat("is affect flag: %d", ch->IsAffectFlag(AFF_TANHWAN_DASH));
		if(ch->IsAffectFlag(AFF_TANHWAN_DASH))
			ch->RemoveAffect(5);
		
		DWORD lastAttack = ch->GetLastAttackedByPC();
		DWORD currTime = get_dword_time();
		/*if (lastAttack + 3000 > currTime)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to wait %d seconds to ride your mount."), DWORD((lastAttack + 3000 - currTime) / 1000) + 1);
			return;
		}
		else*/
			pkMountSystem->StartRiding();
			
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to summon your mount first."));
		return;
	}
}

#define MAX_REASON_LEN		128

EVENTINFO(TimedEventInfo)
{
	DynamicCharacterPtr ch;
	int		subcmd;
	int		 	left_second;
	char		szReason[MAX_REASON_LEN];

	TimedEventInfo() 
	: ch()
	, subcmd( 0 )
	, left_second( 0 )
	{
		::memset( szReason, 0, MAX_REASON_LEN );
	}
};

struct DisconnectFunc
{
	void operator () (LPDESC d)
	{
		if (d->GetType() == DESC_TYPE_CONNECTOR)
			return;

		if (d->IsPhase(PHASE_P2P))
			return;

		if (d->GetCharacter())
			d->GetCharacter()->Disconnect("Shutdown(DisconnectFunc)");

		d->SetPhase(PHASE_CLOSE);
	}
};

EVENTINFO(shutdown_event_data)
{
	int seconds;

	shutdown_event_data()
	: seconds( 0 )
	{
	}
};

#ifdef __MAINTENANCE__
LPEVENT g_pkShutdownEvent = NULL;
bool g_bIsMaintenance = false;
int g_iMaintenanceDurSec;
bool g_bIsMaintenanceStarter = false;
EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>(event->info);

	if (info == NULL)
	{
		sys_err("shutdown_event> <Factor> Null pointer");
		return 0;
	}

	int * pSec = &(info->seconds);
	--*pSec;
	sys_log(0, "shutdown_event sec %d", *pSec);

	if (*pSec == 0)
	{
		sys_log(0, "shutdown: disconnect clients");
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
		g_bNoMoreClient = true;
		// system("killall -9 vrunner");
		return passes_per_sec;
	}
	else if (*pSec < 0)
	{
		if (*pSec <= -5)
		{
#ifdef __MAINTENANCE_DIFFERENT_SERVER__
			if (!test_server && g_bIsMaintenance)
				system("touch /game/auto_restart &");
			if (g_bIsMaintenance && g_bIsMaintenanceStarter)
				db_clientdesc->DBPacket(HEADER_GD_ON_SHUTDOWN_SERVER, 0, NULL, 0);
#else
			if (!test_server && g_bIsMaintenance && g_bIsMaintenanceStarter)
				system("touch /game/auto_restart &");
#endif
			sys_log(0, "shutdown: finish shutdown");
			g_pkShutdownEvent = NULL;
			
			thecore_shutdown();
			return 0;
		}

		return passes_per_sec;
	}
	else
	{
		system("touch /game/stop &");
		char buf[64];
		snprintf(buf, sizeof(buf), "The server will shutdown in [%d] seconds.", *pSec);
		SendNotice(buf);
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	if (g_pkShutdownEvent)
		event_cancel(&g_pkShutdownEvent);

	CWarMapManager::instance().OnShutdown();

	if (iSec <= 60 && g_bIsMaintenance)
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "There is a maintenance in [%d] seconds.", iSec);

		SendNotice(buf);
	}

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = MIN(10, iSec);

	g_pkShutdownEvent = event_create(shutdown_event, info, iSec > 10 ? PASSES_PER_SEC(iSec - 10) : passes_per_sec);
}

void __StartNewShutdown(int iStartSec, bool bIsMaintenance, int iMaintenanceDuration, bool bSendP2P);
ACMD(do_shutdown)
{
	if (NULL != ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}

	__StartNewShutdown(10, false, 0, true);
}

void __SendMaintenancePacket(DWORD dwTimeLeft, DWORD dwDuration, LPCHARACTER pkChr = NULL)
{
	network::GCOutputPacket<network::GCMaintenanceInfoPacket> packet;
	packet->set_remaining_time(dwTimeLeft);
	packet->set_duration(dwDuration);

	if (pkChr)
	{
		pkChr->GetDesc()->Packet(packet);
		return;
	}

	network::GGOutputPacket<network::GGPlayerPacket> p2p_packet;
	p2p_packet->set_language(-1);
	p2p_packet->set_relay_header(packet.get_header());

	std::vector<uint8_t> buf;
	buf.resize(packet->ByteSize());
	packet->SerializeToArray(&buf[0], buf.size());
	p2p_packet->set_relay(&buf[0], buf.size());
	P2P_MANAGER::instance().Send(p2p_packet);

	const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
	for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
		it->second->GetDesc()->Packet(packet);
}

void __StopCurrentShutdown(bool bSendP2P)
{
	event_cancel(&g_pkShutdownEvent);
	__SendMaintenancePacket(0, 0);
	g_bIsMaintenance = false;
	g_bIsMaintenanceStarter = false;

	if (bSendP2P)
	{
		// GG <=>GG
		network::GGOutputPacket<network::GGRecvShutdownPacket> p_g;
		p_g->set_start_sec(-1);
		P2P_MANAGER::instance().Send(p_g);
		
		// For security reasons if P2P Breaks also send to DB
		// GD => DG 
		network::GDOutputPacket<network::GDRecvShutdownPacket> p_d;
		p_d->set_start_sec(-1);
		db_clientdesc->DBPacket(p_d);
	}
}

void __StartNewShutdown(int iStartSec, bool bIsMaintenance, int iMaintenanceDuration, bool bSendP2P)
{
	g_bIsMaintenance = bIsMaintenance;
	g_bIsMaintenanceStarter = bSendP2P;
	g_iMaintenanceDurSec = iMaintenanceDuration;

	if (bIsMaintenance)
		__SendMaintenancePacket(iStartSec, iMaintenanceDuration);
	Shutdown(iStartSec);

	if (bSendP2P)
	{
		// GG <=>GG ()
		network::GGOutputPacket<network::GGRecvShutdownPacket> p_g;
		p_g->set_start_sec(iStartSec);
		p_g->set_maintenance(bIsMaintenance);
		p_g->set_maintenance_duration(iMaintenanceDuration);
		P2P_MANAGER::instance().Send(p_g);
		
		// For security reasons if P2P Breaks also send to DB
		// GD => DG 
		network::GDOutputPacket<network::GDRecvShutdownPacket> p_d;
		p_d->set_start_sec(iStartSec);
		p_d->set_maintenance(bIsMaintenance);
		p_d->set_maintenance_duration(iMaintenanceDuration);
		db_clientdesc->DBPacket(p_d);
	}
}

bool __IsCurrentMaintenance(int& iStartSecLeft, int& iDuration)
{
	if (!g_bIsMaintenance)
		return false;

	if (!g_pkShutdownEvent)
	{
		sys_err("no shutdown event while g_bIsMaintenance = TRUE");
		return false;
	}

	iStartSecLeft = event_time(g_pkShutdownEvent) / passes_per_sec;
	iDuration = g_iMaintenanceDurSec;

	return true;
}

void __SendMaintenancePacketToPlayer(LPCHARACTER pkChr)
{
	int iStartSecLeft, iDuration;
	if (!__IsCurrentMaintenance(iStartSecLeft, iDuration))
		return;

	__SendMaintenancePacket(iStartSecLeft, iDuration, pkChr);
}

ACMD(do_maintenance)
{
	if (ch && ch->GetGMLevel() < GM_IMPLEMENTOR)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to be a administrator."));
		return;
	}

	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!strcasecmp(arg1, "stop"))
	{
		if (!g_pkShutdownEvent)
		{
			if (ch)
				ch->ChatPacket(CHAT_TYPE_INFO, "No shutdown event running.");
			return;
		}

		__StopCurrentShutdown(true);
		return;
	}

	if (!*arg1 || !*arg2)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: maintenance <time> <duration min>(0 to off) (1m 30s)");
		return;
	}

	int StartSec = parse_time_str(arg1);
	int DurSec = parse_time_str(arg2);

	if (StartSec <= 0 && DurSec > 0)
	{
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "=> 10s, 10m, 1m 30s");
		}
		return;
	}

	if (DurSec <= 0)
	{
		if (g_pkShutdownEvent)
			__StopCurrentShutdown(true);

		return;
	}

	if (NULL != ch)
	{
		sys_err("Accept maintenance command from %s.", ch->GetName());
	}

	__StartNewShutdown(StartSec, true, DurSec, true);
}
#else
EVENTFUNC(shutdown_event)
{
	shutdown_event_data* info = dynamic_cast<shutdown_event_data*>( event->info );

	if ( info == NULL )
	{
		sys_err( "shutdown_event> <Factor> Null pointer" );
		return 0;
	}

	int * pSec = & (info->seconds);

	if (*pSec < 0)
	{
		sys_log(0, "shutdown_event sec %d", *pSec);

		if (--*pSec == -10)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), DisconnectFunc());
			return passes_per_sec;
		}
		else if (*pSec < -10)
			return 0;

		return passes_per_sec;
	}
	else if (*pSec == 0)
	{
		const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
		std::for_each(c_set_desc.begin(), c_set_desc.end(), SendDisconnectFunc());
		g_bNoMoreClient = true;
		--*pSec;
		return passes_per_sec;
	}
	else
	{
		char buf[64];
		snprintf(buf, sizeof(buf), "¼Ë´Ù¿îÀÌ [%d]ÃÊ ³²¾Ò½À´Ï´Ù.", *pSec);
		SendNotice(buf);

		--*pSec;
		return passes_per_sec;
	}
}

void Shutdown(int iSec)
{
	if (g_bNoMoreClient)
	{
		thecore_shutdown();
		return;
	}

	CWarMapManager::instance().OnShutdown();

	char buf[64];
	snprintf(buf, sizeof(buf), "[%d]ÃÊ ÈÄ °ÔÀÓÀÌ ¼Ë´Ù¿î µË´Ï´Ù.", iSec);

	SendNotice(buf);

	shutdown_event_data* info = AllocEventInfo<shutdown_event_data>();
	info->seconds = iSec;

	event_create(shutdown_event, info, 1);
}

ACMD(do_shutdown)
{
	if (NULL == ch)
	{
		sys_err("Accept shutdown command from %s.", ch->GetName());
	}
	TPacketGGShutdown p;
	p.bHeader = HEADER_GG_SHUTDOWN;
	P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGShutdown));

	Shutdown(10);
}
#endif

EVENTFUNC(timed_event)
{
	TimedEventInfo * info = dynamic_cast<TimedEventInfo *>( event->info );
	
	if ( info == NULL )
	{
		sys_err( "timed_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	LPDESC d = ch->GetDesc();

	if (info->left_second <= 0)
	{
		ch->m_pkTimedEvent = NULL;

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
			case SCMD_QUIT:
			case SCMD_PHASE_SELECT:
				{
					network::GDOutputPacket<network::GDValidLogoutPacket> acc_info;
					acc_info->set_account_id(ch->GetDesc()->GetAccountTable().id());
					db_clientdesc->DBPacket(acc_info);

					LogManager::instance().DetailLoginLog( false, ch );
				}
				break;
		}

		switch (info->subcmd)
		{
			case SCMD_LOGOUT:
				if (d)
					d->SetPhase(PHASE_CLOSE);
				break;

			case SCMD_QUIT:
				ch->ChatPacket(CHAT_TYPE_COMMAND, "quit");
				break;

			case SCMD_PHASE_SELECT:
				{
					ch->Disconnect("timed_event - SCMD_PHASE_SELECT");

					if (d)
					{
						d->SetPhase(PHASE_SELECT);
					}
				}
				break;
		}

		return 0;
	}
	else
	{
#ifdef LOGOUT_DISABLE_CLIENT_SEND_PACKETS
/*		if(info->left_second == 1 && quest::CQuestManager::instance().GetEventFlag("disable_logout_nosend") == 0)
		{
			if (ch->GetDesc())
				ch->GetDesc()->SetIgnoreClientInput(true);
		}*/
#endif
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%dÃÊ ³²¾Ò½À´Ï´Ù."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_cmd)
{
	/* RECALL_DELAY
	   if (ch->m_pkRecallEvent != NULL)
	   {
	   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
	   event_cancel(&ch->m_pkRecallEvent);
	   return;
	   }
	// END_OF_RECALL_DELAY */

	if (ch->m_pkChannelSwitchEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
		event_cancel(&ch->m_pkChannelSwitchEvent);
		return;
	}

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	switch (subcmd)
	{
		case SCMD_LOGOUT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "·Î±×ÀÎ È­¸éÀ¸·Î µ¹¾Æ °©´Ï´Ù. Àá½Ã¸¸ ±â´Ù¸®¼¼¿ä."));
			break;

		case SCMD_QUIT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°ÔÀÓÀ» Á¾·á ÇÕ´Ï´Ù. Àá½Ã¸¸ ±â´Ù¸®¼¼¿ä."));
			break;

		case SCMD_PHASE_SELECT:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ä³¸¯ÅÍ¸¦ ÀüÈ¯ ÇÕ´Ï´Ù. Àá½Ã¸¸ ±â´Ù¸®¼¼¿ä."));
			break;
	}

#ifdef ENABLE_ZODIAC_TEMPLE
	if (ch->IsPrivateMap(ZODIAC_MAP_INDEX))
	{
		if (ch->IsDead())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You are dead. Respawn in town first or use Prisme."));
			return;
		}
		ch->ChatPacket(CHAT_TYPE_INFO, "You won't be able to join this current dungeon again after a relog.");
		CDungeonManager::instance().RemovePlayerInfo(ch->GetPlayerID());	
		LPDUNGEON dungeon = ch->GetDungeonForce();
		if (dungeon)
			dungeon->SkipPlayerSaveDungeonOnce();
	}
#endif

	int nExitLimitTime = 10;

	if (ch->IsHack(false, true, nExitLimitTime) &&
	   	(!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
	{
		return;
	}
	
	ch->GetDesc()->SetIsDisconnecting(true);
	
	switch (subcmd)
	{
		case SCMD_LOGOUT:
		case SCMD_QUIT:
		case SCMD_PHASE_SELECT:
			{
				TimedEventInfo* info = AllocEventInfo<TimedEventInfo>();
				{
#ifndef ENABLE_FASTER_LOGOUT
					if (ch->IsPosition(POS_FIGHTING))
						info->left_second = 10;
					else
#endif
						info->left_second = 3;
				}

				if (ch->GetGMLevel() == GM_IMPLEMENTOR)
					info->left_second = 0;

				info->ch		= ch;
				info->subcmd		= subcmd;
				strlcpy(info->szReason, argument, sizeof(info->szReason));

				ch->m_pkTimedEvent	= event_create(timed_event, info, 1);
			}
			break;
	}
}

ACMD(do_console)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ConsoleEnable");
}

ACMD(do_restart)
{
	if (false == ch->IsDead())
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->StartRecoveryEvent();
		return;
	}

	if (NULL == ch->m_pkDeadEvent)
		return;

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(ch->GetMapIndex()))
	{
		CCombatZoneManager::Instance().OnRestart(ch, subcmd);
		return;
	}
#endif

	int iTimeToDead = (event_time(ch->m_pkDeadEvent) / passes_per_sec);

	if (iTimeToDead - (180 - g_nPortalLimitTime) < (g_nPortalLimitTime - 180 + 2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You will be ported in your city soon."));
		return;
	}

	if (subcmd == SCMD_RESTART_BASE)
		return;

	if (iTimeToDead - (180 - g_nPortalLimitTime) > 0)
	{
		if (subcmd == SCMD_RESTART_HERE && (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG))
		{
			if (!test_server)
			{
				if (ch->IsHack())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾ÆÁ÷ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%dÃÊ ³²À½)"), iTimeToDead - (180 - g_nPortalLimitTime));
					return;
				}

				if (iTimeToDead > 170)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾ÆÁ÷ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%dÃÊ ³²À½)"), iTimeToDead - 170);
					return;
				}
			}
		}

		//PREVENT_HACK
		//DESC : Ã¢°í, ±³È¯ Ã¢ ÈÄ Æ÷Å»À» »ç¿ëÇÏ´Â ¹ö±×¿¡ ÀÌ¿ëµÉ¼ö ÀÖ¾î¼­
		//		ÄðÅ¸ÀÓÀ» Ãß°¡ 
		if (subcmd == SCMD_RESTART_TOWN)
		{
			if (ch->IsHack())
			{
				//±æµå¸Ê, ¼ºÁö¸Ê¿¡¼­´Â Ã¼Å© ÇÏÁö ¾Ê´Â´Ù.
				if (!ch->GetWarMap() || ch->GetWarMap()->GetType() == GUILD_WAR_TYPE_FLAG)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾ÆÁ÷ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%dÃÊ ³²À½)"), iTimeToDead - (180 - g_nPortalLimitTime));
					return;
				}
			}
			
			if (ch->GetDungeon() && !ch->GetWarMap() && !ch->IsObserverMode())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't restart in town, if you are in a dungeon."));
				return;
			}
			
			if (iTimeToDead > 173)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾ÆÁ÷ ¸¶À»¿¡¼­ Àç½ÃÀÛ ÇÒ ¼ö ¾ø½À´Ï´Ù. (%d ÃÊ ³²À½)"), iTimeToDead - 173);
				return;
			}
		}
	//END_PREVENT_HACK
	}

	if (ch->GetMapIndex() == EVENT_LABYRINTH_MAP_INDEX || ch->IsPrivateMap(EVENT_LABYRINTH_MAP_INDEX))
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		return;
	}
	
	if (ch->GetMapIndex() == EMPIREWAR_MAP_INDEX && subcmd == SCMD_RESTART_TOWN)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this on this map."));
		return;
	}
#ifdef ENABLE_ZODIAC_TEMPLE
#else
	ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
	
	ch->GetDesc()->SetPhase(PHASE_GAME);
	ch->SetPosition(POS_STANDING);
	ch->StartRecoveryEvent();
	ch->StartUpdateAuraEvent();
#endif

	if (ch->GetDungeon())
		ch->GetDungeon()->UseRevive(ch);

	if (ch->GetWarMap() && !ch->IsObserverMode())
	{
		CWarMap * pMap = ch->GetWarMap();
		DWORD dwGuildOpponent = pMap ? pMap->GetGuildOpponent(ch) : 0;

		if (dwGuildOpponent)
		{
			switch (subcmd)
			{
				case SCMD_RESTART_TOWN:
					sys_log(0, "do_restart: restart town");
					PIXEL_POSITION pos;

					if (CWarMapManager::instance().GetStartPosition(ch->GetMapIndex(), ch->GetGuild()->GetID() < dwGuildOpponent ? 0 : 1, pos))
						ch->Show(ch->GetMapIndex(), pos.x, pos.y);
					else
						ch->ExitToSavedLocation();

					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					break;

				case SCMD_RESTART_HERE:
					sys_log(0, "do_restart: restart here");
					ch->RestartAtSamePos();
					//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
					ch->ReviveInvisible(5);
					break;
			}

			return;
		}
	}

	switch (subcmd)
	{
		case SCMD_RESTART_TOWN:
			sys_log(0, "do_restart: restart town");
			PIXEL_POSITION pos;
#ifdef ENABLE_ZODIAC_TEMPLE
			if (ch->IsPrivateMap(ZODIAC_MAP_INDEX))
			{
				LPDUNGEON pDungeon = quest::CQuestManager::instance().GetCurrentDungeon();
				if (pDungeon)
					pDungeon->Completed();
			}
#endif
			if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
				ch->WarpSet(pos.x, pos.y);
			else
				ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));

			ch->PointChange(POINT_HP, 50 - ch->GetHP());
			ch->DeathPenalty(1);
			
			break;

		case SCMD_RESTART_HERE:
#ifdef ENABLE_ZODIAC_TEMPLE
			if (ch->IsPrivateMap(ZODIAC_MAP_INDEX))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Use Prism of Revival in order to restart here."));
				return;
			}
			else
#endif
			{
#ifdef ENABLE_ZODIAC_TEMPLE
				ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");

				ch->GetDesc()->SetPhase(PHASE_GAME);
				ch->SetPosition(POS_STANDING);
				ch->StartRecoveryEvent();
#endif
				sys_log(0, "do_restart: restart here");
				ch->RestartAtSamePos();
				//ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY());
				if (ch->GetMapIndex() == EMPIREWAR_MAP_INDEX)
				{
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				}
#ifdef COMBAT_ZONE
				else if (CCombatZoneManager::Instance().IsCombatZoneMap(ch->GetMapIndex()))
				{
					ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
					ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
				}					
#endif
				else
					ch->PointChange(POINT_HP, 50 - ch->GetHP());
				ch->DeathPenalty(0);
				ch->ReviveInvisible(5);
				break;
			}
	}
}

#define MAX_STAT 90

ACMD(do_stat_reset)
{
	ch->PointChange(POINT_STAT_RESET_COUNT, 12 - ch->GetPoint(POINT_STAT_RESET_COUNT));
}

ACMD(do_stat_minus)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetPoint(POINT_STAT_RESET_COUNT) <= 0)
		return;

	if (!strcmp(arg1, "st"))
	{
		if (ch->GetRealPoint(POINT_ST) <= JobInitialPoints[ch->GetJob()].st)
			return;

		ch->SetRealPoint(POINT_ST, ch->GetRealPoint(POINT_ST) - 1);
		ch->SetPoint(POINT_ST, ch->GetPoint(POINT_ST) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_ST, 0);
	}
	else if (!strcmp(arg1, "dx"))
	{
		if (ch->GetRealPoint(POINT_DX) <= JobInitialPoints[ch->GetJob()].dx)
			return;

		ch->SetRealPoint(POINT_DX, ch->GetRealPoint(POINT_DX) - 1);
		ch->SetPoint(POINT_DX, ch->GetPoint(POINT_DX) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_DX, 0);
	}
	else if (!strcmp(arg1, "ht"))
	{
		if (ch->GetRealPoint(POINT_HT) <= JobInitialPoints[ch->GetJob()].ht)
			return;

		ch->SetRealPoint(POINT_HT, ch->GetRealPoint(POINT_HT) - 1);
		ch->SetPoint(POINT_HT, ch->GetPoint(POINT_HT) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_HT, 0);
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (!strcmp(arg1, "iq"))
	{
		if (ch->GetRealPoint(POINT_IQ) <= JobInitialPoints[ch->GetJob()].iq)
			return;

		ch->SetRealPoint(POINT_IQ, ch->GetRealPoint(POINT_IQ) - 1);
		ch->SetPoint(POINT_IQ, ch->GetPoint(POINT_IQ) - 1);
		ch->ComputePoints();
		ch->PointChange(POINT_IQ, 0);
		ch->PointChange(POINT_MAX_SP, 0);
	}
	else
		return;

	ch->PointChange(POINT_STAT, +1);
	ch->PointChange(POINT_STAT_RESET_COUNT, -1);
	ch->ComputePoints();
}

ACMD(do_stat)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetPoint(POINT_STAT) <= 0)
		return;

	BYTE bCount = 1;
	if (*arg2 && str_is_number(arg2) && str_to_number(bCount, arg2))
	{
		if (bCount < 1)
			bCount = 1;
		if (bCount > ch->GetPoint(POINT_STAT))
			bCount = ch->GetPoint(POINT_STAT);
	}

	unsigned char idx = 0;
	
	if (!strcmp(arg1, "st"))
		idx = POINT_ST;
	else if (!strcmp(arg1, "dx"))
		idx = POINT_DX;
	else if (!strcmp(arg1, "ht"))
		idx = POINT_HT;
	else if (!strcmp(arg1, "iq"))
		idx = POINT_IQ;
	else
		return;

	if (ch->GetRealPoint(idx) - 1 + bCount >= MAX_STAT)
		return;

	ch->SetRealPoint(idx, ch->GetRealPoint(idx) + bCount);
	ch->SetPoint(idx, ch->GetPoint(idx) + bCount);
	ch->ComputePoints();
	ch->PointChange(idx, 0);

	if (idx == POINT_IQ)
	{
		ch->PointChange(POINT_MAX_HP, 0);
	}
	else if (idx == POINT_HT)
	{
		ch->PointChange(POINT_MAX_SP, 0);
	}

	ch->PointChange(POINT_STAT, -(int)bCount);
	ch->ComputePoints();
}

ACMD(do_pvp)
{
#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(ch->GetMapIndex()))
		return;
#endif

	if (ch->GetArena() != NULL || CArenaManager::instance().IsArenaMap(ch->GetMapIndex()) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(vid);

	if (!pkVictim)
		return;

	if (pkVictim->IsNPC())
		return;

#ifdef ENABLE_MESSENGER_BLOCK
	if (ch && MessengerManager::instance().CheckMessengerList(ch->GetName(), pkVictim->GetName(), SYST_BLOCK))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't use this action because you have blocked %s."), pkVictim->GetName());
		return;
	}
#endif

	if (pkVictim->GetArena() != NULL)
	{
		pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkVictim, "»ó´ë¹æÀÌ ´ë·ÃÁßÀÔ´Ï´Ù."));
		return;
	}

	CPVPManager::instance().Insert(ch, pkVictim);
}

ACMD(do_guildskillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (!ch->GetGuild())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå¿¡ ¼ÓÇØÀÖÁö ¾Ê½À´Ï´Ù."));
		return;
	}

	CGuild* g = ch->GetGuild();
	TGuildMember* gm = g->GetMember(ch->GetPlayerID());
	if (gm->grade == GUILD_LEADER_GRADE)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		g->SkillLevelUp(vnum);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå ½ºÅ³ ·¹º§À» º¯°æÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
	}
}

ACMD(do_skillup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (true == ch->CanUseSkill(vnum))
	{
		ch->SkillLevelUp(vnum);
	}
	else if (vnum >= BOOST_SKILL_START && vnum < BOOST_SKILL_END)
	{
		BYTE job = ch->GetJob();
		BYTE group = ch->GetSkillGroup();
		static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM] =
		{
			{ SKILL_BOOST_SWORDSPIN,SKILL_BOOST_SPIRITSTRIKE },
			{ SKILL_BOOST_AMBUSH,SKILL_BOOST_FIREARROW },
			{ SKILL_BOOST_FINGERSTRIKE,SKILL_BOOST_DARKSTRIKE },
			{ SKILL_BOOST_SHOOTINGDRAGON,SKILL_BOOST_SUMMONLIGHTNING },
		};
		if (group > 0 && group < SKILL_GROUP_MAX_NUM + 1 && job < JOB_MAX_NUM && SkillList[job][group - 1] == vnum)
			ch->SkillLevelUp(vnum);
	}
	else
	{
		switch(vnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:

			case SKILL_7_A_ANTI_TANHWAN:
			case SKILL_7_B_ANTI_AMSEOP:
			case SKILL_7_C_ANTI_SWAERYUNG:
			case SKILL_7_D_ANTI_YONGBI:

			case SKILL_8_A_ANTI_GIGONGCHAM:
			case SKILL_8_B_ANTI_YEONSA:
			case SKILL_8_C_ANTI_MAHWAN:
			case SKILL_8_D_ANTI_BYEURAK:

			case SKILL_ADD_HP:
			case SKILL_RESIST_PENETRATE:

			case SKILL_WARD_AMBUSH:
			case SKILL_WARD_DARKSTRIKE:
			case SKILL_WARD_FINGERSTRIKE:
			case SKILL_WARD_FIREARROW:
			case SKILL_WARD_SHOOTINGDRAGON:
			case SKILL_WARD_SPIRITSTRIKE:
			case SKILL_WARD_SUMMONLIGHTNING:
			case SKILL_WARD_SWORDSPIN:
				ch->SkillLevelUp(vnum);
				break;
		}
	}
}

//
// @version	05/06/20 Bang2ni - Ä¿¸Çµå Ã³¸® Delegate to CHARACTER class
//
ACMD(do_safebox_close)
{
	ch->CloseSafebox();
}

//
// @version	05/06/20 Bang2ni - Ä¿¸Çµå Ã³¸® Delegate to CHARACTER class
//
ACMD(do_safebox_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	ch->ReqSafeboxLoad(arg1);
}

ACMD(do_safebox_change_password)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || strlen(arg1)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}

	if (!*arg2 || strlen(arg2)>6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}

	network::GDOutputPacket<network::GDSafeboxChangePasswordPacket> p;
	p->set_account_id(ch->GetAID());
	p->set_old_password(arg1);
	p->set_new_password(arg2);
	db_clientdesc->DBPacket(p, ch->GetDesc()->GetHandle());
}

ACMD(do_mall_password)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || strlen(arg1) > 6)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}

	int iPulse = thecore_pulse();

	if (ch->GetMall())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> Ã¢°í°¡ ÀÌ¹Ì ¿­·ÁÀÖ½À´Ï´Ù."));
		return;
	}

	if (iPulse - ch->GetMallLoadTime() < passes_per_sec * 10) // 10ÃÊ¿¡ ÇÑ¹ø¸¸ ¿äÃ» °¡´É
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> Ã¢°í¸¦ ´ÝÀºÁö 10ÃÊ ¾È¿¡´Â ¿­ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	ch->SetMallLoadTime(iPulse);

	network::GDOutputPacket<network::GDSafeboxLoadPacket> p;
	p->set_account_id(ch->GetAID());
	p->set_login(ch->GetAccountTable().login());
	p->set_password(arg1);
	p->set_is_mall(true);
	db_clientdesc->DBPacket(p, ch->GetDesc()->GetHandle());
}

ACMD(do_mall_open)
{
	if (ch->IsNeedSafeboxPassword())
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
	else
		do_mall_password(ch, "000000", 0, 0);
}

ACMD(do_mall_close)
{
	if (ch->GetMall())
	{
		ch->SetMallLoadTime(thecore_pulse());
		ch->CloseMall();
		ch->Save();
	}
}

ACMD(do_ungroup)
{
	if (!ch->GetParty())
		return;

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼¿¡¼­ ³ª°¥ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	LPPARTY pParty = ch->GetParty();
	if (pParty->IsInDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this while someone in your party is inside a dungeon."));
		return;
	}

	if (pParty->GetMemberCount() == 2)
	{
		// party disband
		CPartyManager::instance().DeleteParty(pParty);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ÆÄÆ¼¿¡¼­ ³ª°¡¼Ì½À´Ï´Ù."));
		//pParty->SendPartyRemoveOneToAll(ch);
		pParty->Quit(ch->GetPlayerID());
		//pParty->SendPartyRemoveAllToOne(ch);
	}
}

ACMD(do_close_shop)
{
	if (ch->GetMyShop())
	{
		ch->CloseMyShop();
		return;
	}
}

ACMD(do_set_walk_mode)
{
	ch->SetNowWalking(true);
	ch->SetWalking(true);
}

ACMD(do_set_run_mode)
{
	ch->SetNowWalking(false);
	ch->SetWalking(false);
}

ACMD(do_war)
{
	//³» ±æµå Á¤º¸¸¦ ¾ò¾î¿À°í
	CGuild * g = ch->GetGuild();

	if (!g)
		return;

	//ÀüÀïÁßÀÎÁö Ã¼Å©ÇÑ¹ø!
	if (g->UnderAnyWar())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ÀÌ¹Ì ´Ù¸¥ ÀüÀï¿¡ ÂüÀü Áß ÀÔ´Ï´Ù."));
		return;
	}

	//ÆÄ¶ó¸ÞÅÍ¸¦ µÎ¹è·Î ³ª´©°í
	char arg1[256], arg2[256];
	BYTE type = GUILD_WAR_TYPE_FIELD;
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
		return;

	if (*arg2)
	{
		str_to_number(type, arg2);

		if (type >= GUILD_WAR_TYPE_MAX_NUM)
			type = GUILD_WAR_TYPE_FIELD;
	
		if(type < 0) //war crash fix
			return;
			
	}

	//±æµåÀÇ ¸¶½ºÅÍ ¾ÆÀÌµð¸¦ ¾ò¾î¿ÂµÚ
	DWORD gm_pid = g->GetMasterPID();

	//¸¶½ºÅÍÀÎÁö Ã¼Å©(±æÀüÀº ±æµåÀå¸¸ÀÌ °¡´É)
	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀü¿¡ ´ëÇÑ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
		return;
	}

	//»ó´ë ±æµå¸¦ ¾ò¾î¿À°í
	CGuild * opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±×·± ±æµå°¡ ¾ø½À´Ï´Ù."));
		return;
	}

	//»ó´ë±æµå¿ÍÀÇ »óÅÂ Ã¼Å©
	switch (g->GetGuildWarState(opp_g->GetID()))
	{
		case GUILD_WAR_NONE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµå°¡ ÀÌ¹Ì ÀüÀï Áß ÀÔ´Ï´Ù."));
					return;
				}

				int iWarPrice = KOR_aGuildWarInfo[type].iWarPrice;

				if (g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Àüºñ°¡ ºÎÁ·ÇÏ¿© ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}

				if (opp_g->GetGuildMoney() < iWarPrice)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµåÀÇ Àüºñ°¡ ºÎÁ·ÇÏ¿© ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}
			}
			break;

		case GUILD_WAR_SEND_DECLARE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ¹Ì ¼±ÀüÆ÷°í ÁßÀÎ ±æµåÀÔ´Ï´Ù."));
				return;
			}
			break;

		case GUILD_WAR_RECV_DECLARE:
			{
				if (opp_g->UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµå°¡ ÀÌ¹Ì ÀüÀï Áß ÀÔ´Ï´Ù."));
					g->RequestRefuseWar(opp_g->GetID());
					return;
				}
			}
			break;

		case GUILD_WAR_RESERVE:
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ÀÌ¹Ì ÀüÀïÀÌ ¿¹¾àµÈ ±æµå ÀÔ´Ï´Ù."));
				return;
			}
			break;

		case GUILD_WAR_END:
			return;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ÀÌ¹Ì ÀüÀï ÁßÀÎ ±æµåÀÔ´Ï´Ù."));
			g->RequestRefuseWar(opp_g->GetID());
			return;
	}

	if (!g->CanStartWar(type))
	{
		// ±æµåÀüÀ» ÇÒ ¼ö ÀÖ´Â Á¶°ÇÀ» ¸¸Á·ÇÏÁö¾Ê´Â´Ù.
		if (g->GetLadderPoint() == 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ·¹´õ Á¡¼ö°¡ ¸ðÀÚ¶ó¼­ ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			sys_log(0, "GuildWar.StartError.NEED_LADDER_POINT");
		}
		else if (g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀüÀ» ÇÏ±â À§ÇØ¼± ÃÖ¼ÒÇÑ %d¸íÀÌ ÀÖ¾î¾ß ÇÕ´Ï´Ù."), GUILD_WAR_MIN_MEMBER_COUNT);
			sys_log(0, "GuildWar.StartError.NEED_MINIMUM_MEMBER[%d]", GUILD_WAR_MIN_MEMBER_COUNT);
		}
		else
		{
			sys_log(0, "GuildWar.StartError.UNKNOWN_ERROR");
		}
		return;
	}

	// ÇÊµåÀü Ã¼Å©¸¸ ÇÏ°í ¼¼¼¼ÇÑ Ã¼Å©´Â »ó´ë¹æÀÌ ½Â³«ÇÒ¶§ ÇÑ´Ù.
	if (!opp_g->CanStartWar(GUILD_WAR_TYPE_FIELD))
	{
		if (opp_g->GetLadderPoint() == 0)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµåÀÇ ·¹´õ Á¡¼ö°¡ ¸ðÀÚ¶ó¼­ ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		else if (opp_g->GetMemberCount() < GUILD_WAR_MIN_MEMBER_COUNT)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµåÀÇ ±æµå¿ø ¼ö°¡ ºÎÁ·ÇÏ¿© ±æµåÀüÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	do
	{
		if (g->GetMasterCharacter() != NULL)
			break;

		CCI *pCCI = P2P_MANAGER::instance().FindByPID(g->GetMasterPID());

		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµåÀÇ ±æµåÀåÀÌ Á¢¼ÓÁßÀÌ ¾Æ´Õ´Ï´Ù."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	do
	{
		if (opp_g->GetMasterCharacter() != NULL)
			break;
		
		CCI *pCCI = P2P_MANAGER::instance().FindByPID(opp_g->GetMasterPID());
		
		if (pCCI != NULL)
			break;

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æ ±æµåÀÇ ±æµåÀåÀÌ Á¢¼ÓÁßÀÌ ¾Æ´Õ´Ï´Ù."));
		g->RequestRefuseWar(opp_g->GetID());
		return;

	} while (false);

	g->RequestDeclareWar(opp_g->GetID(), type);
}

ACMD(do_nowar)
{
	CGuild* g = ch->GetGuild();
	if (!g)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD gm_pid = g->GetMasterPID();

	if (gm_pid != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀü¿¡ ´ëÇÑ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
		return;
	}

	CGuild* opp_g = CGuildManager::instance().FindGuildByName(arg1);

	if (!opp_g)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±×·± ±æµå°¡ ¾ø½À´Ï´Ù."));
		return;
	}

	g->RequestRefuseWar(opp_g->GetID());
}

ACMD(do_detaillog)
{
	ch->DetailLog();
}

ACMD(do_monsterlog)
{
	ch->ToggleMonsterLog();
}

ACMD(do_pkmode)
{
#ifdef ENABLE_BLOCK_PKMODE
	if (ch->IsPKModeBlocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't change your PK Mode."));
		return;
	}
#endif

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	BYTE mode = 0;
	str_to_number(mode, arg1);

	if (mode == PK_MODE_PROTECT)
		return;

	if (ch->GetLevel() < g_bPKProtectLevel && mode != 0)
		return;

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(ch->GetMapIndex()))
		return;
#endif

	ch->SetPKMode(mode);
}

ACMD(do_messenger_auth)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	char answer = LOWER(*arg1);

	if (answer != 'y')
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);

		if (tch)
			tch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(tch, "%s ´ÔÀ¸·Î ºÎÅÍ Ä£±¸ µî·ÏÀ» °ÅºÎ ´çÇß½À´Ï´Ù."), ch->GetName());
		else if (CCI* pCCI = P2P_MANAGER::instance().Find(arg2))
		{
			if (pCCI->pkDesc)
			{
				network::GGOutputPacket<network::GGMessengerRequestPacket> packet;
				packet->set_requestor(arg2);
				packet->set_target_pid(ch->GetPlayerID());
				pCCI->pkDesc->Packet(packet);
			}
		}
	}

	MessengerManager::instance().AuthToAdd(ch->GetName(), arg2, answer == 'y' ? false : true); // DENY
}

ACMD(do_setblockmode)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		BYTE flag = 0;
		str_to_number(flag, arg1);
		ch->SetBlockMode(flag);
	}
}

ACMD(do_unmount)
{
	if (ch->GetMountSystem() && ch->GetMountSystem()->IsRiding())
	{
		ch->GetMountSystem()->StopRiding();
	}
	else
	{
		ch->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÎº¥Åä¸®°¡ ²Ë Â÷¼­ ³»¸± ¼ö ¾ø½À´Ï´Ù."));
	}

}

ACMD(do_observer_exit)
{
	if (ch->IsObserverMode())
	{
		if (ch->GetWarMap())
			ch->SetWarMap(NULL);

		if (ch->IsPrivateMap(EVENT_LABYRINTH_MAP_INDEX))
			ch->GoHome();
		else if (ch->GetArena() != NULL || ch->GetArenaObserverMode() == true)
		{
			ch->SetArenaObserverMode(false);

			if (ch->GetArena() != NULL)
				ch->GetArena()->RemoveObserver(ch->GetPlayerID());

			ch->SetArena(NULL);
			ch->WarpSet(ARENA_RETURN_POINT_X(ch->GetEmpire()), ARENA_RETURN_POINT_Y(ch->GetEmpire()));
		}
		else
			ch->ExitToSavedLocation();
		ch->SetObserverMode(false);
	}
}

ACMD(do_view_equip)
{
	if (ch->GetGMLevel() <= GM_PLAYER)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vid = 0;
		str_to_number(vid, arg1);
		LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

		if (!tch)
			return;

		if (!tch->IsPC())
			return;
		/*
		   int iSPCost = ch->GetMaxSP() / 3;

		   if (ch->GetSP() < iSPCost)
		   {
		   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¤½Å·ÂÀÌ ºÎÁ·ÇÏ¿© ´Ù¸¥ »ç¶÷ÀÇ Àåºñ¸¦ º¼ ¼ö ¾ø½À´Ï´Ù."));
		   return;
		   }
		   ch->PointChange(POINT_SP, -iSPCost);
		 */
		tch->SendEquipment(ch);
	}
}

ACMD(do_party_request)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ¹Ì ÆÄÆ¼¿¡ ¼ÓÇØ ÀÖÀ¸¹Ç·Î °¡ÀÔ½ÅÃ»À» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch && tch != ch)
		if (!ch->RequestToParty(tch))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

ACMD(do_party_request_accept)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->AcceptToParty(tch);
}

ACMD(do_party_request_deny)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD vid = 0;
	str_to_number(vid, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(vid);

	if (tch)
		ch->DenyToParty(tch);
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}

	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}

	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

extern void BroadcastNotice(const char * c_pszBuf);

static const char* FN_point_string(int apply_number)
{
	switch (apply_number)
	{
		case POINT_MAX_HP:	return ("ÃÖ´ë »ý¸í·Â +%d");
		case POINT_MAX_SP:	return ("ÃÖ´ë Á¤½Å·Â +%d");
		case POINT_HT:		return ("Ã¼·Â +%d");
		case POINT_IQ:		return ("Áö´É +%d");
		case POINT_ST:		return ("±Ù·Â +%d");
		case POINT_DX:		return ("¹ÎÃ¸ +%d");
		case POINT_ATT_SPEED:	return ("°ø°Ý¼Óµµ +%d");
		case POINT_MOV_SPEED:	return ("ÀÌµ¿¼Óµµ %d");
		case POINT_CASTING_SPEED:	return ("ÄðÅ¸ÀÓ -%d");
		case POINT_HP_REGEN:	return ("»ý¸í·Â È¸º¹ +%d");
		case POINT_SP_REGEN:	return ("Á¤½Å·Â È¸º¹ +%d");
		case POINT_POISON_PCT:	return ("µ¶°ø°Ý %d");
		case POINT_STUN_PCT:	return ("½ºÅÏ +%d");
		case POINT_SLOW_PCT:	return ("½½·Î¿ì +%d");
		case POINT_CRITICAL_PCT:	return ("%d%% È®·ü·Î Ä¡¸íÅ¸ °ø°Ý");
		case POINT_RESIST_CRITICAL:	return ("»ó´ëÀÇ Ä¡¸íÅ¸ È®·ü %d%% °¨¼Ò");
		case POINT_PENETRATE_PCT:	return ("%d%% È®·ü·Î °üÅë °ø°Ý");
		case POINT_RESIST_PENETRATE: return ("»ó´ëÀÇ °üÅë °ø°Ý È®·ü %d%% °¨¼Ò");
		case POINT_ATTBONUS_HUMAN:	return ("ÀÎ°£·ù ¸ó½ºÅÍ Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_ANIMAL:	return ("µ¿¹°·ù ¸ó½ºÅÍ Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_ORC:	return ("¿õ±ÍÁ· Å¸°ÝÄ¡ +%d%%");
#ifdef ENABLE_ZODIAC_TEMPLE
		case POINT_ATTBONUS_ZODIAC:	return ("Strong against Zodiacmonster +%d%%");
#endif
		case POINT_ATTBONUS_MILGYO:	return ("¹Ð±³·ù Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_UNDEAD:	return ("½ÃÃ¼·ù Å¸°ÝÄ¡ +%d%%");
		case POINT_ATTBONUS_DEVIL:	return ("¾Ç¸¶·ù Å¸°ÝÄ¡ +%d%%");
		case POINT_STEAL_HP:		return ("Å¸°ÝÄ¡ %d%% ¸¦ »ý¸í·ÂÀ¸·Î Èí¼ö");
		case POINT_STEAL_SP:		return ("Å¸·ÂÄ¡ %d%% ¸¦ Á¤½Å·ÂÀ¸·Î Èí¼ö");
		case POINT_MANA_BURN_PCT:	return ("%d%% È®·ü·Î Å¸°Ý½Ã »ó´ë Àü½Å·Â ¼Ò¸ð");
		case POINT_DAMAGE_SP_RECOVER:	return ("%d%% È®·ü·Î ÇÇÇØ½Ã Á¤½Å·Â È¸º¹");
		case POINT_BLOCK:			return ("¹°¸®Å¸°Ý½Ã ºí·° È®·ü %d%%");
		case POINT_DODGE:			return ("È° °ø°Ý È¸ÇÇ È®·ü %d%%");
		case POINT_RESIST_SWORD:	return ("ÇÑ¼Õ°Ë ¹æ¾î %d%%");
		case POINT_RESIST_TWOHAND:	return ("¾ç¼Õ°Ë ¹æ¾î %d%%");
		case POINT_RESIST_DAGGER:	return ("µÎ¼Õ°Ë ¹æ¾î %d%%");
		case POINT_RESIST_BELL:		return ("¹æ¿ï ¹æ¾î %d%%");
		case POINT_RESIST_FAN:		return ("ºÎÃ¤ ¹æ¾î %d%%");
		case POINT_RESIST_BOW:		return ("È°°ø°Ý ÀúÇ× %d%%");
		case POINT_RESIST_FIRE:		return ("È­¿° ÀúÇ× %d%%");
		case POINT_RESIST_ELEC:		return ("Àü±â ÀúÇ× %d%%");
		case POINT_RESIST_MAGIC:	return ("¸¶¹ý ÀúÇ× %d%%");
		case POINT_RESIST_WIND:		return ("¹Ù¶÷ ÀúÇ× %d%%");
		case POINT_RESIST_ICE:		return ("³Ã±â ÀúÇ× %d%%");
		case POINT_RESIST_EARTH:	return ("´ëÁö ÀúÇ× %d%%");
		case POINT_RESIST_DARK:		return ("¾îµÒ ÀúÇ× %d%%");
		case POINT_REFLECT_MELEE:	return ("Á÷Á¢ Å¸°ÝÄ¡ ¹Ý»ç È®·ü : %d%%");
		case POINT_REFLECT_CURSE:	return ("ÀúÁÖ µÇµ¹¸®±â È®·ü %d%%");
		case POINT_POISON_REDUCE:	return ("µ¶ ÀúÇ× %d%%");
		case POINT_KILL_SP_RECOVER:	return ("%d%% È®·ü·Î ÀûÅðÄ¡½Ã Á¤½Å·Â È¸º¹");
		case POINT_EXP_DOUBLE_BONUS:	return ("%d%% È®·ü·Î ÀûÅðÄ¡½Ã °æÇèÄ¡ Ãß°¡ »ó½Â");
		case POINT_GOLD_DOUBLE_BONUS:	return ("%d%% È®·ü·Î ÀûÅðÄ¡½Ã µ· 2¹è µå·Ó");
		case POINT_ITEM_DROP_BONUS:	return ("%d%% È®·ü·Î ÀûÅðÄ¡½Ã ¾ÆÀÌÅÛ 2¹è µå·Ó");
		case POINT_POTION_BONUS:	return ("¹°¾à »ç¿ë½Ã %d%% ¼º´É Áõ°¡");
		case POINT_KILL_HP_RECOVERY:	return ("%d%% È®·ü·Î ÀûÅðÄ¡½Ã »ý¸í·Â È¸º¹");
//		case POINT_IMMUNE_STUN:	return ("±âÀýÇÏÁö ¾ÊÀ½ %d%%");
//		case POINT_IMMUNE_SLOW:	return ("´À·ÁÁöÁö ¾ÊÀ½ %d%%");
//		case POINT_IMMUNE_FALL:	return ("³Ñ¾îÁöÁö ¾ÊÀ½ %d%%");
//		case POINT_SKILL:	return ("");
//		case POINT_BOW_DISTANCE:	return ("");
		case POINT_ATT_GRADE_BONUS:	return ("°ø°Ý·Â +%d");
		case POINT_DEF_GRADE_BONUS:	return ("¹æ¾î·Â +%d");
		case POINT_MAGIC_ATT_GRADE:	return ("¸¶¹ý °ø°Ý·Â +%d");
		case POINT_MAGIC_DEF_GRADE:	return ("¸¶¹ý ¹æ¾î·Â +%d");
//		case POINT_CURSE_PCT:	return ("");
		case POINT_MAX_STAMINA:	return ("ÃÖ´ë Áö±¸·Â +%d");
		case POINT_ATTBONUS_WARRIOR:	return ("¹«»ç¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_ASSASSIN:	return ("ÀÚ°´¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_SURA:		return ("¼ö¶ó¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_SHAMAN:		return ("¹«´ç¿¡°Ô °­ÇÔ +%d%%");
		case POINT_ATTBONUS_MONSTER:	return ("¸ó½ºÅÍ¿¡°Ô °­ÇÔ +%d%%");
		case POINT_MALL_ATTBONUS:		return ("°ø°Ý·Â +%d%%");
		case POINT_MALL_DEFBONUS:		return ("¹æ¾î·Â +%d%%");
		case POINT_MALL_EXPBONUS:		return ("°æÇèÄ¡ %d%%");
		case POINT_MALL_ITEMBONUS:		return ("¾ÆÀÌÅÛ µå·ÓÀ² %.1f¹è");
		case POINT_MALL_GOLDBONUS:		return ("µ· µå·ÓÀ² %.1f¹è");
		case POINT_MAX_HP_PCT:			return ("ÃÖ´ë »ý¸í·Â +%d%%");
		case POINT_MAX_SP_PCT:			return ("ÃÖ´ë Á¤½Å·Â +%d%%");
		case POINT_SKILL_DAMAGE_BONUS:	return ("½ºÅ³ µ¥¹ÌÁö %d%%");
		case POINT_NORMAL_HIT_DAMAGE_BONUS:	return ("ÆòÅ¸ µ¥¹ÌÁö %d%%");
		case POINT_SKILL_DEFEND_BONUS:		return ("½ºÅ³ µ¥¹ÌÁö ÀúÇ× %d%%");
		case POINT_NORMAL_HIT_DEFEND_BONUS:	return ("ÆòÅ¸ µ¥¹ÌÁö ÀúÇ× %d%%");
//		case POINT_EXTRACT_HP_PCT:	return ("");
		case POINT_RESIST_WARRIOR:	return ("¹«»ç°ø°Ý¿¡ %d%% ÀúÇ×");
		case POINT_RESIST_ASSASSIN:	return ("ÀÚ°´°ø°Ý¿¡ %d%% ÀúÇ×");
		case POINT_RESIST_SURA:		return ("¼ö¶ó°ø°Ý¿¡ %d%% ÀúÇ×");
		case POINT_RESIST_SHAMAN:	return ("¹«´ç°ø°Ý¿¡ %d%% ÀúÇ×");
		default:					return NULL;
	}
}

static bool FN_hair_affect_string(LPCHARACTER ch, char *buf, size_t bufsiz)
{
	if (NULL == ch || NULL == buf)
		return false;

	CAffect* aff = NULL;
	time_t expire = 0;
	struct tm ltm;
	int	year, mon, day;
	int	offset = 0;

	aff = ch->FindAffect(AFFECT_HAIR);

	if (NULL == aff)
		return false;

	// set apply string
	offset = snprintf(buf, bufsiz, FN_point_string(aff->bApplyOn), aff->lApplyValue);

	if (offset < 0 || offset >= (int) bufsiz)
		offset = bufsiz - 1;

	localtime_r(&expire, &ltm);

	year	= ltm.tm_year + 1900;
	mon		= ltm.tm_mon + 1;
	day		= ltm.tm_mday;

	snprintf(buf + offset, bufsiz - offset, " (¸¸·áÀÏ : %d³â %d¿ù %dÀÏ)", year, mon, day);

	return true;
}

ACMD(do_costume)
{
	sys_err("%s do_costume abuse? (%s)", ch->GetName(), argument);
	return;

	char buf[512];
	const size_t bufferSize = sizeof(buf);

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CItem* pBody = ch->GetWear(WEAR_COSTUME_BODY);
	CItem* pHair = ch->GetWear(WEAR_COSTUME_HAIR);
	CItem* pWeapon = ch->GetWear(WEAR_COSTUME_WEAPON);

	ch->ChatPacket(CHAT_TYPE_INFO, "COSTUME status:");

	if (pHair)
	{
		const char* itemName = pHair->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  HAIR : %s", itemName);

		for (int i = 0; i < pHair->GetAttributeCount(); ++i)
		{
			auto& attr = pHair->GetAttribute(i);
			if (0 < attr.type())
			{
				snprintf(buf, bufferSize, FN_point_string(attr.type()), attr.value());
				ch->ChatPacket(CHAT_TYPE_INFO, "	 %s", buf);
			}
		}

		if (pHair->IsEquipped() && arg1[0] == 'h')
			ch->UnequipItem(pHair);
	}

	if (pBody)
	{
		const char* itemName = pBody->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  BODY : %s", itemName);

		if (pBody->IsEquipped() && arg1[0] == 'b')
			ch->UnequipItem(pBody);
	}

	if (pWeapon)
	{
		const char* itemName = pWeapon->GetName();
		ch->ChatPacket(CHAT_TYPE_INFO, "  WEAPON : %s", itemName);

		if (pWeapon->IsEquipped() && arg1[0] == 'w')
			ch->UnequipItem(pWeapon);
	}
}

ACMD(do_hair)
{
	char buf[256];

	if (false == FN_hair_affect_string(ch, buf, sizeof(buf)))
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

ACMD(do_inventory)
{
	int	index = 0;
	int	count		= 1;

	char arg1[256];
	char arg2[256];

	LPITEM	item;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: inventory <start_index> <count>");
		return;
	}

	if (!*arg2)
	{
		index = 0;
		str_to_number(count, arg1);
	}
	else
	{
		str_to_number(index, arg1); index = MIN(index, INVENTORY_MAX_NUM);
		str_to_number(count, arg2); count = MIN(count, INVENTORY_MAX_NUM);
	}

	for (int i = 0; i < count; ++i)
	{
		if (index >= INVENTORY_MAX_NUM)
			break;

		item = ch->GetInventoryItem(index);

		if (!item)
			continue;

		ch->ChatPacket(CHAT_TYPE_INFO, "inventory [%d] = %s [%u] stone %d, %d, %d attr %d %d, %d %d, %d %d, %d %d, %d %d, %d %d, %d %d",
						index, item->GetName(), item->GetID(), item->GetSocket(0), item->GetSocket(1), item->GetSocket(2),
						item->GetAttributeType(0), item->GetAttributeValue(0),
						item->GetAttributeType(1), item->GetAttributeValue(1), 
						item->GetAttributeType(2), item->GetAttributeValue(2), 
						item->GetAttributeType(3), item->GetAttributeValue(3), 
						item->GetAttributeType(4), item->GetAttributeValue(4), 
						item->GetAttributeType(5), item->GetAttributeValue(5), 
						item->GetAttributeType(6), item->GetAttributeValue(6));
		++index;
	}
}

//gift notify quest command
ACMD(do_gift)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "gift");
}

ACMD(do_cube)
{
	if (!ch->CanDoCube())
		return;

	ch->tchat(argument);

	dev_log(LOG_DEB0, "CUBE COMMAND <%s>: %s", ch->GetName(), argument);
	int cube_index = 0, inven_index = 0;
	const char *line;

	char arg1[256], arg2[256], arg3[256];

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (0 == arg1[0])
	{
		// print usage
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: cube open");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube close");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube add <inveltory_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube delete <cube_index>");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube list");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube cancel");
		ch->ChatPacket(CHAT_TYPE_INFO, "	   cube make [all]");
		return;
	}

	const std::string& strArg1 = std::string(arg1);

	// r_info (request information)
	// /cube r_info	 ==> (Client -> Server) ÇöÀç NPC°¡ ¸¸µé ¼ö ÀÖ´Â ·¹½ÃÇÇ ¿äÃ»
	//						(Server -> Client) /cube r_list npcVNUM resultCOUNT 123,1/125,1/128,1/130,5
	//
	// /cube r_info 3   ==> (Client -> Server) ÇöÀç NPC°¡ ¸¸µé¼ö ÀÖ´Â ·¹½ÃÇÇ Áß 3¹øÂ° ¾ÆÀÌÅÛÀ» ¸¸µå´Â µ¥ ÇÊ¿äÇÑ Á¤º¸¸¦ ¿äÃ»
	// /cube r_info 3 5 ==> (Client -> Server) ÇöÀç NPC°¡ ¸¸µé¼ö ÀÖ´Â ·¹½ÃÇÇ Áß 3¹øÂ° ¾ÆÀÌÅÛºÎÅÍ ÀÌÈÄ 5°³ÀÇ ¾ÆÀÌÅÛÀ» ¸¸µå´Â µ¥ ÇÊ¿äÇÑ Àç·á Á¤º¸¸¦ ¿äÃ»
	//					   (Server -> Client) /cube m_info startIndex count 125,1|126,2|127,2|123,5&555,5&555,4/120000@125,1|126,2|127,2|123,5&555,5&555,4/120000
	//
	if (strArg1 == "r_info")
	{
		if (0 == arg2[0])
			Cube_request_result_list(ch);
		else
		{
			if (isdigit(*arg2))
			{
				int listIndex = 0, requestCount = 1;
				str_to_number(listIndex, arg2);

				if (0 != arg3[0] && isdigit(*arg3))
					str_to_number(requestCount, arg3);

				Cube_request_material_info(ch, listIndex, requestCount);
			}
		}

		return;
	}

	switch (LOWER(arg1[0]))
	{
		case 'o':	// open
			Cube_open(ch, arg2);
			break;

		case 'c':	// close
			Cube_close(ch);
			break;

		case 'l':	// list
			Cube_show_list(ch);
			break;

		case 'a':	// add cue_index inven_index
			{
				if (0 == arg2[0] || !isdigit(*arg2) ||
					0 == arg3[0] || !isdigit(*arg3))
					return;

				str_to_number(cube_index, arg2);
				str_to_number(inven_index, arg3);
				Cube_add_item (ch, cube_index, inven_index);
			}
			break;

		case 'd':	// delete
			{
				if (0 == arg2[0] || !isdigit(*arg2))
					return;

				str_to_number(cube_index, arg2);
				Cube_delete_item (ch, cube_index);
			}
			break;

		case 'm':	// make
			if (0 != arg2[0])
			{
				while (true == Cube_make(ch))
					dev_log (LOG_DEB0, "cube make success");
			}
			else
				Cube_make(ch);
			break;

		default:
			return;
	}
}

ACMD(do_in_game_mall)
{
	char country_code[3] = { 'd', 'e', '\0', };

	static char buf[512 + 1];
	char sas[33];
	MD5_CTX ctx;
	const char sas_key[] = "GF9001";

	snprintf(buf, sizeof(buf), "%u%u%s", ch->GetPlayerID(), ch->GetAID(), sas_key);

	MD5Init(&ctx);
	MD5Update(&ctx, (const unsigned char *)buf, strlen(buf));
#ifdef __FreeBSD__
	MD5End(&ctx, sas);
#else
	static const char hex[] = "0123456789abcdef";
	unsigned char digest[16];
	MD5Final(digest, &ctx);
	int i;
	for (i = 0; i < 16; ++i) {
		sas[i + i] = hex[digest[i] >> 4];
		sas[i + i + 1] = hex[digest[i] & 0x0f];
	}
	sas[i + i] = '\0';
#endif

	snprintf(buf, sizeof(buf), "mall https://%s/ishop?pid=%u&sas=%s&timestamp=%d",
		g_strWebMallURL.c_str(), ch->GetPlayerID(), sas, time(0));
		
	// snprintf(buf, sizeof(buf), "mall http://%s/ishop?pid=%u&c=%s&sid=%d&sas=%s",
			// g_strWebMallURL.c_str(), ch->GetPlayerID(), country_code, g_server_id, sas);

	ch->ChatPacket(CHAT_TYPE_COMMAND, buf);
}

// ÁÖ»çÀ§
ACMD(do_dice) 
{
	if(quest::CQuestManager::instance().GetEventFlag("enable_dice") == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This feature is disabled.");
		return;
	}

	char arg1[256], arg2[256];
	int start = 1, end = 100;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		start = atoi(arg1);
		end = atoi(arg2);
	}
	else if (*arg1 && !*arg2)
	{
		start = 1;
		end = atoi(arg1);
	}

	end = MAX(start, end);
	start = MIN(start, end);

	int n = random_number(start, end);
	
	if (ch->GetParty())
		ch->GetParty()->ChatPacketToAllMemberNonTranslation(CHAT_TYPE_INFO, "Dice roll of [%s] : %d (%d-%d)", ch->GetName(), n, start, end);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ç½ÅÀÌ ÁÖ»çÀ§¸¦ ±¼·Á %d°¡ ³ª¿Ô½À´Ï´Ù. (%d-%d)"), n, start, end);
}

ACMD(do_click_mall)
{
	ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeMallPassword");
}

ACMD(do_inputall)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸í·É¾î¸¦ ¸ðµÎ ÀÔ·ÂÇÏ¼¼¿ä."));
}

ACMD (do_party_invite)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!pInvitee || !pInvitee->GetDesc() || pInvitee == ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s ´ÔÀº Á¢¼ÓµÇ ÀÖÁö ¾Ê½À´Ï´Ù."), arg1);
		return;
	}

	ch->PartyInvite(pInvitee);
}

#ifdef __GUILD_SAFEBOX__
ACMD(do_guild_safebox_open)
{
	if (ch->GetGuild())
	{
		ch->GetGuild()->GetSafeBox().OpenSafebox(ch);
	}
}

ACMD(do_guild_safebox_open_log)
{
	if (ch->GetGuild())
	{
		ch->GetGuild()->GetSafeBox().OpenLog(ch);
	}
}

ACMD(do_guild_safebox_close)
{
	if (ch->GetGuild())
	{
		ch->GetGuild()->GetSafeBox().CloseSafebox(ch);
	}
}
#endif

#ifdef __ANIMAL_SYSTEM__
ACMD(do_animal_stat_up)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !str_is_number(arg2))
	{
		if (ch->IsGM())
			ch->ChatPacket(CHAT_TYPE_INFO, "usage: animal_stat_up <type(mount|pet)> <stat_index>");
		return;
	}

	int iStatIndex;
	str_to_number(iStatIndex, arg2);

	if (iStatIndex < CItem::ANIMAL_STATUS1 || iStatIndex >= CItem::ANIMAL_STATUS1 + CItem::ANIMAL_STATUS_COUNT)
	{
		sys_err("animal_stat_up: invalid stat index %d", iStatIndex);
		return;
	}

	LPITEM pkItem = NULL;

	if (!strcmp(arg1, "mount"))
	{
		if (CMountSystem* pMountSys = ch->GetMountSystem())
		{
			if (pMountSys->IsRiding() || pMountSys->IsSummoned())
				pkItem = ch->FindItemByID(pMountSys->GetSummonItemID());
		}
	}
#ifdef __PET_SYSTEM__
	else if (!strcmp(arg1, "pet"))
	{
		if (CPetActor* pPet = ch->GetPetSystem()->GetSummoned())
			pkItem = ch->FindItemByVID(pPet->GetSummonItemVID());
		else
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Summon your pet to skill up its stat points."));
	}
#endif
	else {
		sys_err("invalid animal stat type [%s]", arg1);
	}

	if (pkItem && pkItem->Animal_IsAnimal())
	{
		if (pkItem->Animal_GetStatusPoints() <= 0)
			return;

		pkItem->Animal_IncreaseStatus(iStatIndex);
		pkItem->Animal_SetStatusPoints(pkItem->Animal_GetStatusPoints() - 1);
		pkItem->Animal_StatsPacket();
	}
}
#endif

EVENTINFO(FastChannelChangeEventInfo)
{
	DynamicCharacterPtr ch;
	int			new_channel;
	long		new_host;
	int		 	left_second;

	FastChannelChangeEventInfo()
		: ch()
		, new_channel(0)
		, new_host(0)
		, left_second(0)
	{
	}
};

EVENTFUNC(fast_channel_change_event)
{
	FastChannelChangeEventInfo * info = dynamic_cast<FastChannelChangeEventInfo *>(event->info);

	if (info == NULL)
	{
		sys_err("fast_channel_change_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (info->left_second <= 0)
	{
		if (!ch->ChangeChannel(info->new_host, info->new_channel))
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot change the channel right now."));
		else
		{
			sys_log(0, "fast_channel_change: ChangeChannel (%s) channel %u -> %ld : %u", ch->GetName(), g_bChannel, info->new_host, info->new_channel);

			ch->m_pkChannelSwitchEvent = NULL;

			network::GDOutputPacket<network::GDValidLogoutPacket> acc_info;
			acc_info->set_account_id(ch->GetAID());
			db_clientdesc->DBPacket(acc_info);

			LogManager::instance().DetailLoginLog(false, ch);
		}
		return 0;
	}
	else
	{
#ifdef LOGOUT_DISABLE_CLIENT_SEND_PACKETS
		/*if(info->left_second == 1 && quest::CQuestManager::instance().GetEventFlag("disable_logout_nosend") == 0)
		{
			if (ch->GetDesc())
				ch->GetDesc()->SetIgnoreClientInput(true);
		}*/
#endif
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%dÃÊ ³²¾Ò½À´Ï´Ù."), info->left_second);
		--info->left_second;
	}

	return PASSES_PER_SEC(1);
}

ACMD(do_fast_change_channel)
{
	char arg1[256], arg2[256];
	int iNewChannel;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	sys_log(0, "fast_change_channel [%s %s]", arg1, arg2);

	if (!*arg1 || !str_is_number(arg1) || !str_to_number(iNewChannel, arg1) || !iNewChannel || iNewChannel == g_bChannel)
		return;
	
	if (!*arg2)
		return;
		
	if (ch->m_pkChannelSwitchEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
		event_cancel(&ch->m_pkChannelSwitchEvent);
		return;
	}

	if (ch->m_pkTimedEvent)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
		event_cancel(&ch->m_pkTimedEvent);
		return;
	}

	FastChannelChangeEventInfo* info = AllocEventInfo<FastChannelChangeEventInfo>();
	info->new_host = inet_addr(arg2);
	info->new_channel = iNewChannel;

	if (ch->IsPosition(POS_FIGHTING))
		info->left_second = 10;
	else
		info->left_second = 3;
	
	if (test_server || ch->GetGMLevel() == GM_IMPLEMENTOR)
		info->left_second = 0;

	info->ch = ch;

	ch->GetDesc()->SetIsDisconnecting(true);
	
	sys_log(0, "fast_channel_change: ReqChangeChannel (%s) channel %u -> %s(%ld) : %u", ch->GetName(), g_bChannel, arg2, info->new_host, iNewChannel);
	ch->m_pkChannelSwitchEvent = event_create(fast_channel_change_event, info, 1);
}

ACMD(do_safebox_open)
{
	if (ch->IsOpenSafebox())
		return;

	if (ch->GetSafeboxSize() <= 0)
		return;

	ch->SetSafeboxOpenPosition();

	if (ch->IsNeedSafeboxPassword())
		ch->ChatPacket(CHAT_TYPE_COMMAND, "ShowMeSafeboxPassword");
	else
		ch->ReqSafeboxLoad("000000");
}

#ifdef __SWITCHBOT__
ACMD(do_switchbot_change_speed)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	DWORD dwMS;
	str_to_number(dwMS, arg1);
	ch->tchat("Switchbot change speed: %d", dwMS);
#ifndef ELONIA
	bool bIsPremium = ch->GetQuestFlag("auction_premium.premium_active") != 0;
	if (bIsPremium)
#endif
		dwMS = MINMAX(150, dwMS, 1500);
#ifndef ELONIA
	else
		dwMS = MINMAX(400, dwMS, 1500);
#endif
	ch->tchat("Switchbot change speed: %d", dwMS);
	ch->ChangeSwitchbotSpeed(dwMS);
}

ACMD(do_switchbot_start)
{
	if (quest::CQuestManager::instance().GetEventFlag("switchbot_disabled") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Disabled.");
		return;
	}

	char argIdx[256], argSlot[256], argMaxSwitcher[256];
	argument = one_argument(argument, argSlot, sizeof(argSlot));

	if (!*argSlot || !str_is_number(argSlot))
		return;

	WORD wSlot;
	str_to_number(wSlot, argSlot);
	if (wSlot >= INVENTORY_MAX_NUM)
		return;

	LPITEM pkItem = ch->GetInventoryItem(wSlot);
	if (!pkItem || (pkItem->GetType() != ITEM_WEAPON && pkItem->GetType() != ITEM_ARMOR && pkItem->GetType() != ITEM_TOTEM) || pkItem->GetAttributeSetIndex() == -1)
		return;

	network::TSwitchbotTable kSwitchbotTable;
	kSwitchbotTable.set_item_id(pkItem->GetID());
	kSwitchbotTable.set_inv_cell(pkItem->GetCell());

	for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
	{
		char argType[256], argVal[256];
		argument = two_arguments(argument, argType, sizeof(argType), argVal, sizeof(argVal));

		if (!*argType || !str_is_number(argType) || !*argVal || !str_is_number(argVal))
		{
			if (i == 0)
				return;

			break;
		}

		auto attr = kSwitchbotTable.add_attrs();
		attr->set_type(std::stoi(argType));
		attr->set_value(std::stoi(argVal));
	}

	ch->AppendSwitchbotData(&kSwitchbotTable);
}

ACMD(do_switchbot_start_premium)
{
	if (quest::CQuestManager::instance().GetEventFlag("switchbot_disabled") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Disabled.");
		return;
	}

#ifndef ELONIA
	if (!ch->GetQuestFlag("auction_premium.premium_active") && !test_server)
		return;
#endif
	
	char argIdx[256], argSlot[256], argMaxSwitcher[256];
	argument = one_argument(argument, argSlot, sizeof(argSlot));

	if (!*argSlot || !str_is_number(argSlot))
		return;

	WORD wSlot;
	str_to_number(wSlot, argSlot);
	if (wSlot >= INVENTORY_MAX_NUM)
		return;

	LPITEM pkItem = ch->GetInventoryItem(wSlot);
	if (!pkItem || (pkItem->GetType() != ITEM_WEAPON && pkItem->GetType() != ITEM_ARMOR && pkItem->GetType() != ITEM_TOTEM) || pkItem->GetAttributeSetIndex() == -1)
		return;

	network::TSwitchbotTable kSwitchbotTable;
	kSwitchbotTable.set_item_id(pkItem->GetID());
	kSwitchbotTable.set_inv_cell(pkItem->GetCell());

	for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
	{
		char argType[256], argVal[256];
		argument = two_arguments(argument, argType, sizeof(argType), argVal, sizeof(argVal));

		if (!*argType || !str_is_number(argType) || !*argVal || !str_is_number(argVal))
		{
			if (i == 0)
				return;

			break;
		}

		auto attr = kSwitchbotTable.add_premium_attrs();
		attr->set_type(std::stoi(argType));
		attr->set_value(std::stoi(argVal));
	}

	ch->AppendSwitchbotDataPremium(&kSwitchbotTable);
}

ACMD(do_switchbot_stop)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	WORD wItemCell;
	str_to_number(wItemCell, arg1);

	ch->RemoveSwitchbotDataBySlot(wItemCell);
}
#endif

ACMD(do_anti_exp)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	bool bVal;
	str_to_number(bVal, arg1);

	const char* c_pszFlagName = "game_flag.anti_exp";
	if (ch->GetQuestFlag(c_pszFlagName) == bVal)
	{
		ch->PointChange(POINT_ANTI_EXP, 0);
		return;
	}

	ch->SetQuestFlag(c_pszFlagName, bVal);
	if (bVal)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You don't receive EXP anymore."));
		ch->SetEXPDisabled();
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You will receive EXP now."));
		ch->SetEXPEnabled();
	}
}

ACMD(do_horse_rage_mode)
{
	ch->StartHorseRage();
}

ACMD(do_horse_refine)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	BYTE bRefineIndex;
	str_to_number(bRefineIndex, arg1);

	if (ch->GetMountSystem())
		ch->GetMountSystem()->DoRefine(bRefineIndex);
}

#ifdef __GAYA_SYSTEM__
ACMD(do_gaya_shop_buy)
{
	char arg1[256], arg2[256], arg3[256];
	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1 || !str_is_number(arg1) || !*arg2 || !str_is_number(arg2) || !*arg3 || !str_is_number(arg3))
		return;

	DWORD dwVnum;
	str_to_number(dwVnum, arg1);
#ifdef INCREASE_ITEM_STACK
	WORD bCount;
#else
	BYTE bCount;
#endif
	str_to_number(bCount, arg2);
	DWORD dwPrice;
	str_to_number(dwPrice, arg3);

	if (!dwVnum || !bCount)
		return;

	if (dwPrice > ch->GetGaya())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have not enough gaya."));
		return;
	}

	auto pShopData = CGeneralManager::instance().GetGayaShop();
	for (int i = 0; i <= GAYA_SHOP_MAX_NUM; ++i)
	{
		if (i == GAYA_SHOP_MAX_NUM)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Invalid purchase."));
			return;
		}

		if (pShopData->vnum() == dwVnum && pShopData->count() == bCount && pShopData->price() == dwPrice)
			break;

		++pShopData;
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, bCount);
	if (!item)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Error occurred. Contact a team member.");
		return;
	}

	ch->AutoGiveItem(item);
	ch->ChangeGaya(-(int)dwPrice);
}
#endif

ACMD(do_cards)
{
    const char *line;

    char arg1[256], arg2[256];

    line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    switch (LOWER(arg1[0]))
    {
        case 'o':    // open
            if (isdigit(*arg2))
            {
                DWORD safemode;
                str_to_number(safemode, arg2);
                ch->Cards_open(safemode);
            }
            break;
        case 'p':    // open
            ch->Cards_pullout();
            break;
        case 'e':    // open
            ch->CardsEnd();
            break;
        case 'd':    // open
            if (isdigit(*arg2))
            {
                DWORD destroy_index;
                str_to_number(destroy_index, arg2);
                ch->CardsDestroy(destroy_index);
            }
            break;
        case 'a':    // open
            if (isdigit(*arg2))
            {
                DWORD accpet_index;
                str_to_number(accpet_index, arg2);
                ch->CardsAccept(accpet_index);
            }
            break;
        case 'r':    // open
            if (isdigit(*arg2))
            {
                DWORD restore_index;
                str_to_number(restore_index, arg2);
                ch->CardsRestore(restore_index);
            }
            break;
        default:
            return;
    }
}

ACMD(do_dungeon_debug_info)
{
	if (!ch)
		return;

	if (!ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_PARTY, "You are not in a dungeon.");
		return;
	}

	time_t ct = time(0);  
	char *time_s = asctime(localtime(&ct));
	struct timeval tv;
	int nMiliSec = 0;
	gettimeofday(&tv, NULL);
	time_s[strlen(time_s) - 1] = '\0';

	ch->ChatPacket(CHAT_TYPE_PARTY, "Dungeon-Debug-Info[%d/%d]: %-15.15s.%d || %s", ch->GetPlayerID(), ch->GetMapIndex(), time_s + 4, tv.tv_usec, g_stHostname.c_str());
	auto& rkMap = ch->GetDungeon()->GetFlagMap();
	for (itertype(rkMap) it = rkMap.begin(); it != rkMap.end(); ++it)
		ch->ChatPacket(CHAT_TYPE_PARTY, "  FLAG [%s] : %d", it->first.c_str(), it->second);

	auto& rkMapServerTimer = quest::CQuestManager::instance().GetServerTimerMap();
	for (itertype(rkMapServerTimer) it = rkMapServerTimer.begin(); it != rkMapServerTimer.end(); ++it)
	{
		if (it->first.second == ch->GetMapIndex())
		{
			ch->ChatPacket(CHAT_TYPE_PARTY, "  STIM [%s] : %.2f sec", it->first.first.c_str(), (float)event_time(it->second) / (float)passes_per_sec);
		}
	}
	ch->ChatPacket(CHAT_TYPE_PARTY, "Monster[%d] Alive[%d]", ch->GetDungeon()->CountMonster(), ch->GetDungeon()->CountAliveMonster());
}

#ifdef __ATTRTREE__
ACMD(do_attrtree_level_info)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	if (!ch || !ch->CanShopNow() || ch->GetShopOwner() || ch->GetMyShop())
	{
		if (ch)
			ch->ChatPacket( CHAT_TYPE_INFO, "You can't do this now.");
		return;
	}
	
	BYTE id;
	str_to_number(id, arg1);

	if (test_server && ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "attrtree_level_info -> get refine by id %u", id);

	auto pRefineTab = ch->GetAttrTreeRefine(id);
	if (!pRefineTab)
		return;

	if (test_server && ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "send refine info for id %u", id);

	network::GCOutputPacket<network::GCAttrtreeRefinePacket> pack;
	pack->set_pos(id);
	*pack->mutable_refine_table() = *pRefineTab;

	if (ch->GetDesc())
		ch->GetDesc()->Packet(pack);
}

ACMD(do_attrtree_levelup)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	if (!ch || !ch->CanShopNow() || ch->GetShopOwner() || ch->GetMyShop())
	{
		if (ch)
			ch->ChatPacket( CHAT_TYPE_INFO, "You can't do this now.");
		return;
	}
	
	BYTE id;
	str_to_number(id, arg1);

	auto pRefineTab = ch->GetAttrTreeRefine(id);
	if (!pRefineTab)
		return;

	if (ch->GetGold() < pRefineTab->cost())
	{
		sys_log(0, "attrtree_levelup %s : not enough gold (%lld < %d)", ch->GetName(), ch->GetGold(), pRefineTab->cost());
		return;
	}

	for (int i = 0; i < pRefineTab->material_count(); ++i)
	{
		if (ch->CountSpecifyItem(pRefineTab->materials(i).vnum()) < pRefineTab->materials(i).count())
		{
			sys_log(0, "attrtree_levelup %s : not enough item (vnum %u count %d < %d)",
				ch->GetName(), pRefineTab->materials(i).vnum(), ch->CountSpecifyItem(pRefineTab->materials(i).vnum()), pRefineTab->materials(i).count());
			return;
		}
	}

	ch->PointChange(POINT_GOLD, -pRefineTab->cost());
	for (int i = 0; i < pRefineTab->material_count(); ++i)
		ch->RemoveSpecifyItem(pRefineTab->materials(i).vnum(), pRefineTab->materials(i).count());

	BYTE row, col;
	CAttrtreeManager::instance().IDToCell(id, row, col);

	if (pRefineTab->prob() >= random_number(1, 100))
	{
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "AttrTree Levelup success");

		char szHint[100];
		snprintf(szHint, sizeof(szHint), "SUCCESS: %d -> %d cost %d", ch->GetAttrTreeLevel(row, col), ch->GetAttrTreeLevel(row, col) + 1, pRefineTab->cost());
		LogManager::instance().CharLog(ch, id, "ATTRTREE_UP", szHint);

		ch->SetAttrTreeLevel(row, col, ch->GetAttrTreeLevel(row, col) + 1);
	}
	else
	{
		if (test_server)
			ch->ChatPacket(CHAT_TYPE_INFO, "AttrTree Levelup failed");

		char szHint[100];
		snprintf(szHint, sizeof(szHint), "FAILED: %d -> %d cost %d", ch->GetAttrTreeLevel(row, col), ch->GetAttrTreeLevel(row, col) + 1, pRefineTab->cost());
		LogManager::instance().CharLog(ch, id, "ATTRTREE_UP", szHint);
	}
}
#endif

ACMD(do_reload_entitys)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_reload_entitys") == 1)
		return;

#ifdef CHECK_TIME_AFTER_PVP
	if (ch->IsPVPFighting(15))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait %d seconds to do this after the last time you were attacked."), 15);
		return;
	}
#endif

	ch->ViewReencode();
	ch->ChatPacket(CHAT_TYPE_INFO, "Environment reloaded");
}

ACMD(do_load_timers)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_timer_feature") == 1)
		return;


	ch->ChatPacket(CHAT_TYPE_COMMAND, "TIMER_OPEN");
}

ACMD(do_dungeon_complete_delayed)
{
	if(!ch)
		return;
	
	ch->tchat("dungeon_complete_delayed");
	if (quest::CQuestManager::instance().GetEventFlag("event_anniversary_running"))
	{
		int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
		if (day == 1)
		{
			int currFraction = ch->GetQuestFlag("anniversary_event.selected_fraction");
			if (currFraction == 1)
				quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
			else if (currFraction == 2)
				quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
		}
	}
	if (ch->GetDungeonComplete())
	{
		quest::CQuestManager::instance().DungeonComplete(ch->GetPlayerID(), ch->GetDungeonComplete());
		ch->SetDungeonComplete(0);
	}
}

ACMD(do_quest_drop_delayed)
{
	if(!ch)
		return;
	
	ch->tchat("do_quest_drop_delayed");
	if (ch->GetItemDropQuest())
	{
		quest::CQuestManager::instance().DropQuestItem(ch->GetPlayerID(), ch->GetItemDropQuest());
		ch->SetItemDropQuest(0);
	}
}

ACMD(do_quest_complete_missionbook_delayed)
{
	if (!ch)
		return;

	ch->tchat("complete_missinbook_trigger");
	if (quest::CQuestManager::instance().GetEventFlag("event_anniversary_running"))
	{
		int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
		if (day == 5)
		{
			int currFraction = ch->GetQuestFlag("anniversary_event.selected_fraction");
			if (currFraction == 1)
				quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
			else if (currFraction == 2)
				quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
		}
	}
	quest::CQuestManager::instance().OnCompleteMissionbook(ch->GetPlayerID());
}

ACMD(do_set_hide_costumes)
{
	if(!ch)
		return;
	
	quest::PC* cPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (!cPC
#ifndef ELONIA
	 || !quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->GetFlag("auction_premium.premium_active")
#endif
	 )
	{
		ch->tchat("premium is not active");
		return;
	}
	
	// To avoid abort of attack/skill animation
	DWORD dwCurTime = get_dword_time();
	if (dwCurTime - ch->GetLastAttackTime() <= 1500/*  || dwCurTime - ch->GetLastSkillTime() <= 1500 */)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°¡¸¸È÷ ÀÖÀ» ¶§¸¸ Âø¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}
	
	ch->tchat("do_set_hide_costumes");
	
	int tmp1, tmp2, tmp3, tmp4;
	char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256];
	one_argument(two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4)), arg5, sizeof(arg5));

	if (!*arg1 || !str_is_number(arg1) || !str_to_number(tmp1, arg1) || !*arg2 || !str_is_number(arg2) || !str_to_number(tmp2, arg2) || !*arg3 || !str_is_number(arg3) || !str_to_number(tmp3, arg3) || !*arg4 || !str_is_number(arg4) || !str_to_number(tmp4, arg4))
	{
		ch->tchat("wrong arguments");
		return;
	}
	
	ch->SetCostumeHide(HIDE_COSTUME_WEAPON, tmp1);
	ch->SetCostumeHide(HIDE_COSTUME_ARMOR, tmp2);
	ch->SetCostumeHide(HIDE_COSTUME_HAIR, tmp3);
	ch->SetCostumeHide(HIDE_COSTUME_ACCE, tmp4);

	ch->UpdatePacket();
	
	if (!*arg5)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The change will take affect after the next teleport."));
}

ACMD(do_remove_affect)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));	

	if (!*arg1 || !str_is_number(arg1))
		return;		

	WORD affectIndex = 0;
	str_to_number(affectIndex, arg1);

	int apply_type = 0;
	if (*arg2)
	{
		str_to_number(apply_type, arg2);
	}

	if(affectIndex == AFFECT_BLEND)
	{
		CAffect* p = ch->FindAffect(AFFECT_BLEND, apply_type);
		
		if (p)
		{
			ch->RemoveAffect(p);
			return;
		}
	}

	if (affectIndex == AFFECT_POLYMORPH)
	{
		ch->CancelPolymorphEvent();
		ch->SetPolymorph(ch->GetJob(), true);
	}

	if (!(ch->IsGoodAffectSkill(affectIndex) || affectIndex == AFFECT_POLYMORPH))
		return;
	
	ch->tchat("Remove Affect ID: %d", affectIndex);
	ch->RemoveAffect(affectIndex);
}

ACMD(do_vm_detected)
{
	if (!ch)
		return;
	
	char buff[255];
	sprintf(buff, "%i KK Yang + %i KK in bars", ch->GetGold()/1000000, (ch->CountSpecifyItem(80007) * 250) + (ch->CountSpecifyItem(80006) * 100) + (ch->CountSpecifyItem(94355) * 1000));
	
	ch->DetectionHackLog("VirtualMachine", buff);
}

ACMD(do_hwid_change_detected)
{
	if (!ch)
		return;

	if (quest::CQuestManager::instance().GetEventFlag("disable_hwid_change_check"))
		return;
	
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;
	
	ch->DetectionHackLog("ChangedHWID", arg1);
}

#ifdef AHMET_FISH_EVENT_SYSTEM
ACMD(do_SendFishBoxUse)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int a, b;

	if (!*arg1 || !*arg2 || !str_is_number(arg1) || !str_is_number(arg2) || !str_to_number(a, arg1) || !str_to_number(b, arg2))
	{
		ch->tchat("ERR PARAMETERS");
		return;
	}
	
	ch->FishEventUseBox(TItemPos(a, b));
}

ACMD(do_SendFishShapeAdd)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int a, b;

	if (!*arg1 || !*arg2 || !str_is_number(arg1) || !str_is_number(arg2) || !str_to_number(a, arg1) || !str_to_number(b, arg2))
	{
		ch->tchat("ERR PARAMETERS");
		return;
	}
	
	ch->FishEventAddShape(a);
}

ACMD(do_SendFishShapeAdd_Specific)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int a, b;

	if (!*arg1 || !str_is_number(arg1) || !str_to_number(a, arg1))
	{
		ch->tchat("ERR PARAMETERS");
		return;
	}
	
	ch->SetFishAttachedShape(a);
	ch->FishEventIncreaseUseCount();
	ch->SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_BOX_USE, ch->GetFishAttachedShape(), ch->GetFishEventUseCount());
}
#endif

ACMD(do_select_angelsdemons_fraction)
{
	if (!ch)
		return;

	char arg1[ 256 ];
	one_argument(argument, arg1, sizeof(arg1));

	int iFractionIndex = 0;

	if (!*arg1 || !str_is_number(arg1) || !str_to_number(iFractionIndex, arg1))
	{
		ch->tchat("ERR PARAMETERS");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::Instance().GetPCForce(ch->GetPlayerID());

	if (!pPC)
		return;

	bool bEventBattlepassGiven = bool(pPC->GetFlag("anniversary_event.given_battlepass"));

	if (bEventBattlepassGiven)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, "You have already received Angels & Demons battlepass.");
		return;
	}

	if (iFractionIndex != FRACTION_ANGELS && iFractionIndex != FRACTION_DEMONS)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, "Invalid fraction selected.");
		return;
	}

	ch->AutoGiveItem(94219, 1);

	pPC->SetFlag("anniversary_event.selected_fraction", iFractionIndex);
	pPC->SetFlag("anniversary_event.given_battlepass", 1);
}

#ifdef ENABLE_XMAS_EVENT
std::vector<network::TXmasRewards> g_vec_xmasRewards;
#include <ctime>

ACMD(do_xmas_reward)
{
	if (!quest::CQuestManager::instance().GetEventFlag("xmas_event_calendar"))
		return;

	if (thecore_pulse() - ch->GetXmasEventPulse() < PASSES_PER_SEC(10))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait 10 seconds."));
		return;
	}

	ch->SetXmasEventPulse();

	if (ch->GetLevel() < 75)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The minimum required level to recive gifts is 75."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !isdigit(*arg1))
		return;

	BYTE reward = 0;
	str_to_number(reward, arg1);

	if (reward >= g_vec_xmasRewards.size())
		return;

	time_t time = get_global_time();
	std::tm *now = std::localtime(&time);

	if (!test_server && now->tm_mon != 11) // month count starts from 0
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "It's not the season yet."));
		return;
	}

	if (now->tm_mday != g_vec_xmasRewards[reward].day())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot recive this gift today."));
		return;
	}

	std::auto_ptr<SQLMsg> msg(DBManager::instance().DirectQuery("SELECT name FROM player WHERE DATE_SUB(NOW(), INTERVAL 1 WEEK) < create_date and id = %u", ch->GetPlayerID()));

	if (msg->Get()->uiNumRows)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "In order to recive this gift your caracter needs to be 7 days or older."));
		return;
	}

	std::auto_ptr<SQLMsg> msg1(DBManager::instance().DirectQuery("SELECT day, year, pid, account_id, hwid, ip FROM xmas_recived_gifts WHERE day = %u AND year = %u AND (pid = %u OR account_id = %u OR hwid = '%s' OR ip = '%s')",
		now->tm_mday, 1900 + now->tm_year, ch->GetPlayerID(), ch->GetAccountTable().id(), ch->GetEscapedHWID(), ch->GetDesc()->GetHostName()));

	if (msg1->Get()->uiNumRows)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have been already recived this gift."));
		return;
	}

	std::auto_ptr<SQLMsg> msg2(DBManager::instance().DirectQuery("INSERT INTO xmas_recived_gifts VALUES(%u, %u, %u, %u, '%s', '%s')",
		now->tm_mday, 1900 + now->tm_year, ch->GetPlayerID(), ch->GetAccountTable().id(), ch->GetEscapedHWID(), ch->GetDesc()->GetHostName()));

	if (reward == 23)
	{
		std::auto_ptr<SQLMsg> msg3(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM xmas_recived_gifts WHERE year = %u AND (pid = %u OR account_id = %u OR hwid = '%s' OR ip = '%s')",
			1900 + now->tm_year, ch->GetPlayerID(), ch->GetAccountTable().id(), ch->GetEscapedHWID(), ch->GetDesc()->GetHostName()));

		if (msg3->Get()->uiNumRows)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "To recive this gift you were required to open 15 cards."));
			return;
		}

		MYSQL_ROW row = NULL;

		if ((row = mysql_fetch_row(msg3->Get()->pSQLResult)))
		{
			BYTE count = 0;
			str_to_number(count, row[0]);

			if (count < 15)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "To recive this gift you were required to open 15 cards."));
				return;
			}
		}
	}

	quest::CQuestManager::Instance().OpenXmasDoor(ch->GetPlayerID());
	ch->AutoGiveItem(g_vec_xmasRewards[reward].vnum(), g_vec_xmasRewards[reward].count());
	ch->ChatPacket(CHAT_TYPE_COMMAND, "XmasRecivedReward %u", reward);
}
#endif

#ifdef ENABLE_WARP_BIND_RING
ACMD(do_request_warp_bind)
{
	if (thecore_pulse() - ch->GetWarpBindPulse() < PASSES_PER_SEC(10))
		return;

	ch->SetWarpBindPulse();

	std::auto_ptr<SQLMsg> msg(DBManager::instance().DirectQuery("SELECT target_pid, name FROM warp_bind WHERE pid = '%d'", ch->GetPlayerID()));

	SQLResult *pRes = msg->Get();

	if (!pRes->uiNumRows)
		return;

	MYSQL_ROW row = NULL;

	while ((row = mysql_fetch_row(pRes->pSQLResult)))
	{
		if (!MessengerManager::instance().CheckMessengerList(ch->GetName(), row[1], SYST_BLOCK))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "WarpBindRing %s \"%s\"", row[0], row[1]);
		else
			ch->ChatPacket(CHAT_TYPE_COMMAND, "WarpBindRing 0 BLOCKED");			
	}
}
#endif

ACMD(do_server_boot_time)
{
	if (!ch)
		return;
	
	ch->ChatPacket(CHAT_TYPE_COMMAND, "SERVER_BOOT_TIME %f", thecore_time());
}

ACMD(do_dungeon_reconnect)
{
	if (!ch)
		return;
	
	char arg1[ 256 ];		
	int iReconnect;
	one_argument(argument, arg1, sizeof(arg1));
	if (!*arg1 || !str_is_number(arg1) || !str_to_number(iReconnect, arg1))
	{
		ch->tchat("ERR PARAMETERS");
		return;
	}
	
	TDungeonPlayerInfo s = CDungeonManager::instance().GetPlayerInfo(ch->GetPlayerID());
	CDungeonManager::instance().RemovePlayerInfo(ch->GetPlayerID());
	
	if (!iReconnect)
		return;
	
	LPDUNGEON dungeon = ch->GetDungeonForce();
	if (dungeon)
		dungeon->SkipPlayerSaveDungeonOnce();
	
	if (s.map == 0)
		ch->ChatPacket(CHAT_TYPE_INFO, "The Dungeon timeouted meanwhile, sorry.");
	else if (s.map != ch->GetMapIndex() || s.x != ch->GetX())
		ch->WarpSet(s.x, s.y, s.map);
}

#ifdef ENABLE_RUNE_PAGES
ACMD(do_select_rune)
{
	char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256];
	one_argument(two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4)), arg5, sizeof(arg5));

	if (!*arg1 || !str_is_number(arg1) || !*arg2 || !str_is_number(arg2) || !*arg3 || !str_is_number(arg3) || !*arg4 || !str_is_number(arg4) || !*arg5 || !str_is_number(arg5))
		return;

	BYTE group = 0, page = 0, category = 0, index = 0, vnum = 0;
	str_to_number(group, arg1);
	str_to_number(page, arg2);
	str_to_number(category, arg3);
	str_to_number(index, arg4);
	str_to_number(vnum, arg5);

	if (group >= RUNE_GROUP_MAX_NUM)
		return;

	if (page >= RUNE_PAGE_COUNT)
		return;

	if (category == 0)
	{
		if (index >= RUNE_MAIN_COUNT)
			return;

		ch->m_selectedRunes[page].set_main_group(group + 1);
		while (index >= ch->m_selectedRunes[page].main_vnum_size())
			ch->m_selectedRunes[page].add_main_vnum(0);
		ch->m_selectedRunes[page].set_main_vnum(index, vnum);
	}
	else if (category == 2)
	{
		ch->m_selectedRunes[page].set_sub_group(vnum + 1);
		while (2 >= ch->m_selectedRunes[page].sub_vnum_size())
			ch->m_selectedRunes[page].add_sub_vnum(0);
		ch->m_selectedRunes[page].set_sub_vnum(0, 0);
		ch->m_selectedRunes[page].set_sub_vnum(1, 0);
	}
	else if (category == 3)
	{
		ch->m_selectedRunes[page].set_sub_group(vnum + 1);
	}
	else
	{
		if (index >= RUNE_SUB_COUNT)
			return;

		while (index >= ch->m_selectedRunes[page].sub_vnum_size())
			ch->m_selectedRunes[page].add_sub_vnum(0);
		ch->m_selectedRunes[page].set_sub_vnum(index, vnum);
	}
}

ACMD(do_get_rune_page)
{

#ifdef CHECK_TIME_AFTER_PVP
	if (ch->IsPVPFighting(5))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait %d seconds to do this after the last time you were attacked."), 5);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", 9999, 9999);
		return;
	}
#endif

	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	ch->tchat("do_get_rune_page %s", argument);

	if (!*arg1 || !str_is_number(arg1))
		return;

	short page = 0;
	str_to_number(page, arg1);

	if (page >= RUNE_PAGE_COUNT)
		return;
	if (page < 0)
		return;

	BYTE group = ch->m_selectedRunes[page].main_group();

	if (group)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", -1, group - 1);

	for (int i = 0; i < RUNE_MAIN_COUNT; ++i)
	{
		BYTE vnum = ch->m_selectedRunes[page].main_vnum_size() > i ? ch->m_selectedRunes[page].main_vnum(i) : 0;
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", i, vnum);
	}

	BYTE sub = ch->m_selectedRunes[page].sub_group();
	if (sub)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", RUNE_MAIN_COUNT + RUNE_SUB_COUNT, sub - 1);

	for (int i = 0 + RUNE_MAIN_COUNT; i < RUNE_SUB_COUNT + RUNE_MAIN_COUNT; ++i)
	{
		BYTE vnum = ch->m_selectedRunes[page].sub_vnum_size() > (i - RUNE_MAIN_COUNT) ? ch->m_selectedRunes[page].sub_vnum(i - RUNE_MAIN_COUNT) : 0;
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", i, vnum);
	}

	if (*arg2 && str_is_number(arg2))
		ch->ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", -2, 0);

	ch->SetQuestFlag("rune.current_page", page);
}

ACMD(do_reset_rune_page)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	BYTE page = 0;
	str_to_number(page, arg1);

	if (page >= RUNE_PAGE_COUNT)
		return;

	ch->m_selectedRunes[page].Clear();
}

ACMD(do_get_selected_page)
{
	char arg1[256], arg2[256], arg3[256], arg4[256], arg5[256], arg6[256], arg7[256], arg8[256];
	two_arguments(two_arguments(two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4)), arg5, sizeof(arg5), arg6, sizeof(arg6)), arg7, sizeof(arg7), arg8, sizeof(arg8));

	if (!*arg1 || !str_is_number(arg1) || !*arg2 || !str_is_number(arg2) || !*arg3 || !str_is_number(arg3) || !*arg4 || !str_is_number(arg4) || !*arg5 || !str_is_number(arg5) || !*arg6 || !str_is_number(arg6) || !*arg7 || !str_is_number(arg7) || !*arg8 || !str_is_number(arg8))
		return;

	BYTE group = 0, sub_group = 0;
	BYTE runes[RUNE_MAIN_COUNT];
	memset(runes, 0, sizeof(runes));
	BYTE subs[RUNE_SUB_COUNT];
	memset(subs, 0, sizeof(subs));
	str_to_number(group, arg1);
	str_to_number(runes[0], arg2);
	str_to_number(runes[1], arg3);
	str_to_number(runes[2], arg4);
	str_to_number(runes[3], arg5);
	str_to_number(sub_group, arg6);
	str_to_number(subs[0], arg7);
	str_to_number(subs[1], arg8);

	for (int i = 0; i < RUNE_PAGE_COUNT; ++i)
	{
		if (group + 1 != ch->m_selectedRunes[i].main_group())
			continue;

		bool found = true;
		for (int j = 0; j < RUNE_MAIN_COUNT; ++j)
		{
			if (runes[j] != (ch->m_selectedRunes[i].main_vnum_size() > j ? ch->m_selectedRunes[i].main_vnum(j) : 0))
			{
				found = false;
				break;
			}
		}
		if (!found)
			continue;

		if (sub_group + 1 != ch->m_selectedRunes[i].sub_group())
			continue;

		for (int j = 0; j < RUNE_SUB_COUNT; ++j)
		{
			if (subs[j] != (ch->m_selectedRunes[i].sub_vnum_size() > j ? ch->m_selectedRunes[i].sub_vnum(j) : 0))
			{
				found = false;
				break;
			}
		}
		if (!found)
			continue;

		ch->ChatPacket(CHAT_TYPE_COMMAND, "SelectRunePage %d", i);
		return;
	}

	ch->tchat("didnt found rune page");
}
#endif

#ifdef SORT_AND_STACK_ITEMS
ACMD(do_stack_items)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_inv_stack"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This function doesn't work temporarily.");
		return;
	}

	if (thecore_pulse() - ch->GetStackingPulse() < PASSES_PER_SEC(3))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait 3 seconds."));
		return;
	}

	ch->SetStackingPulse();

	if (ch->IsDead() || ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShop() || ch->GetMyShop() || ch->IsCubeOpen() || ch->IsAcceWindowOpen() || !ch->CanHandleItem())
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	BYTE window = 0;
	str_to_number(window, arg1);

	WORD wSlotStart = 0, wSlotEnd = 0;
	ch->GetInventorySlotRange(window, wSlotStart, wSlotEnd);

	if (window == 100)
		wSlotEnd = INVENTORY_AND_EQUIP_SLOT_MAX;

	if (!wSlotEnd)
		return;

	for (int i = wSlotStart; i < wSlotEnd; ++i)
	{
		LPITEM item = ch->GetInventoryItem(i);

		if (!item)
			continue;

		if (item->isLocked())
			continue;

		if (item->GetCount() == ITEM_MAX_COUNT)
			continue;

		if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			for (int j = i; j < wSlotEnd; ++j)
			{
				LPITEM item2 = ch->GetInventoryItem(j);

				if (!item2)
					continue;

				if (item2->isLocked())
					continue;

				if ((item->IsGMOwner() && !item2->IsGMOwner()) || (!item->IsGMOwner() && item2->IsGMOwner()))
					continue;

				if (item2->GetVnum() == item->GetVnum())
				{
					bool canStack = true;

					for (int k = 0; k < ITEM_SOCKET_MAX_NUM; ++k)
					{
						if (item2->GetSocket(k) != item->GetSocket(k))
						{
							canStack = false;
							break;
						}
					}

					if (!canStack)
						continue;

					WORD count = MIN(ITEM_MAX_COUNT - item->GetCount(), item2->GetCount());

					item->SetCount(item->GetCount() + count);
					item2->SetCount(item2->GetCount() - count);

					continue;
				}
			}
		}
	}
}
#endif

#if defined(ENABLE_LEVEL2_RUNES) && defined(ENABLE_RUNE_SYSTEM)
ACMD(do_level_rune)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (!ch->IsRuneOwned(vnum))
		return;

	DWORD newVnum = vnum + 100;

	auto pProto = CRuneManager::instance().GetProto(newVnum);

	if (!pProto)
		return;

	if (pProto->sub_group() == RUNE_SUBGROUP_PRIMARY)
		return;

	//for (int i = 0; i < 12; ++i)
	for (int i = 1; i <= 60; ++i)
	{
		//DWORD id = CRuneManager::instance().GetIDByIndex(pProto->bGroup, i);

		//if (!ch->IsRuneOwned(id) && id != newVnum)
		if (!ch->IsRuneOwned(i))
		{
			ch->tchat("you don't have this rune %d", i);
			return;
		}
	}

	DWORD refine = CRuneManager::instance().FindPointProto(1000 + vnum);

	if (!refine)
		return;

	auto pRefineTab = CRefineManager::instance().GetRefineRecipe(refine);
	if (!pRefineTab)
		return;

	if (ch->GetGold() < pRefineTab->cost())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You don't have enough gold to level up the rune!");
		return;
	}

	for (int i = 0; i < pRefineTab->material_count(); ++i)
	{
		if (ch->CountSpecifyItem(pRefineTab->materials(i).vnum()) < pRefineTab->materials(i).count())
			return;
	}

	ch->PointChange(POINT_GOLD, -pRefineTab->cost());
	for (int i = 0; i < pRefineTab->material_count(); ++i)
		ch->RemoveSpecifyItem(pRefineTab->materials(i).vnum(), pRefineTab->materials(i).count());

	ch->SetRuneOwned(newVnum);

	if (ch->GetPoint(aApplyInfo[pProto->apply_type()].bPointType))
	{
		ch->GiveRuneBuff(vnum, false);
		ch->GiveRuneBuff(vnum, true);
		ch->ComputePoints();
		ch->PointsPacket();
	}
}

ACMD(do_open_rune_level)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	DWORD vnum = 0;
	str_to_number(vnum, arg1);

	if (!ch->IsRuneOwned(vnum))
		return;

	DWORD newVnum = vnum + 100;

	if (ch->IsRuneOwned(newVnum))
		return;

	auto pProto = CRuneManager::instance().GetProto(newVnum);
	if (!pProto)
		return;

	DWORD refine = CRuneManager::instance().FindPointProto(1000 + vnum);

	if (!refine)
		return;

	auto pRefineTab = CRefineManager::instance().GetRefineRecipe(refine);
	if (!pRefineTab)
		return;

	network::GCOutputPacket<network::GCRuneLevelupPacket> pack;
	pack->set_pos(vnum);
	*pack->mutable_refine_table() = *pRefineTab;
	ch->GetDesc()->Packet(pack);
}
#endif

#ifdef LEADERSHIP_EXTENSION
ACMD(do_leadership_state)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE state = 0;
	str_to_number(state, arg1);

	if (state > 6)
		return;

	if (ch->GetSkillMasterType(121) != SKILL_LEGENDARY_MASTER)
		return;

	if (ch->GetParty())
	{
		const BYTE roles[] = {
			PARTY_ROLE_ATTACKER,
			PARTY_ROLE_HASTE,
			PARTY_ROLE_TANKER,
			PARTY_ROLE_BUFFER,
			PARTY_ROLE_SKILL_MASTER,
			PARTY_ROLE_DEFENDER,
		};

		ch->GetParty()->SetRole(ch->GetPlayerID(), ch->GetParty()->GetRole(ch->GetPlayerID()), false);
		ch->GetParty()->SetRole(ch->GetPlayerID(), roles[state], true);
		return;
	}

	ch->SetLeadershipState(state + 1);
	ch->SetQuestFlag("leadershipState.value", state + 1);
}
#endif

#ifdef PACKET_ERROR_DUMP
ACMD(do_syserr_log)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_syserr_log") == 1)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	LogManager::instance().ClientSyserrLog(ch, arg1);
}
#endif

#ifdef BATTLEPASS_EXTENSION
ACMD(do_battlepass_data)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE index = 0;
	str_to_number(index, arg1);

	if (index >= 10)
		return;

	ch->SendBattlepassData(index);
}

ACMD(do_battlepass_shop)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE index = 0;
	str_to_number(index, arg1);

	CShopManager::instance().StartShopping(ch, index);
}
#endif

#ifdef USER_ATK_SPEED_LIMIT
ACMD(do_attackspeed_limit)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE limit = 0;
	str_to_number(limit, arg1);

	if (limit && (limit < 100 || limit > 200)) // allow 0, 100-200
		return;

#ifdef CHECK_TIME_AFTER_PVP
	if (get_dword_time() - ch->GetLastAttackTimePVP() <= 3000)
		return;
#endif
	
	ch->SetQuestFlag("setting.limit_atkspd", limit);
 	ch->UpdatePacket();
}
#endif

#ifdef CHARACTER_BUG_REPORT
ACMD(do_character_bug_report)
{
	if (!ch)
		return;

	char arg1[256];
	char arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->tchat("Usage: character_bug_report <namelist> <name>");
		return;
	}

	char szEscapedNameList[CHARACTER_NAME_MAX_LEN * 8 + 1];
	DBManager::instance().EscapeString(szEscapedNameList, sizeof(szEscapedNameList), arg1, strlen(arg1));

	char szEscapedName[CHARACTER_NAME_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szEscapedName, sizeof(szEscapedName), arg2, strlen(arg2));

	char szHint[200];
	snprintf(szHint, sizeof(szHint), "(%s) '%s'", szEscapedNameList, szEscapedName);
	LogManager::instance().CharLog(ch, 0, "CHAR_BUG", szHint);

	if (quest::CQuestManager::instance().GetEventFlag("char_bug_rewarp_enabled") == 1)
		ch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
}
#endif

ACMD(do_guild_members_lastplayed)
{
	if (quest::CQuestManager::instance().GetEventFlag("guild_lastplayed_disabled") == 1)
		return;

	if (!ch || !ch->GetGuild() || (ch->GetPlayerID() != ch->GetGuild()->GetMasterPID() && !test_server))
		return;
	
	ch->tchat("Request last played data..");
	ch->GetGuild()->QueryLastPlayedlist(ch);
}

#ifdef ACCOUNT_TRADE_BLOCK
ACMD(do_verify_hwid) // <pid> <hwid>
{
	char arg1[256];
	char arg2[HWID_MAX_LEN + 1];
	int pid;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !str_is_number(arg1) || !str_to_number(pid, arg1))
		return;


	if (ch = CHARACTER_MANAGER::instance().FindByPID(pid))
	{
		if (!strcmp(arg2, ch->GetEscapedHWID()))
		{
			ch->GetDesc()->SetTradeblocked(0);
			ch->ChatPacket(CHAT_TYPE_INFO, "Computer verified!");
		}
		else
			ch->tchat("hwid wrong '%s' '%s'", arg2, ch->GetEscapedHWID());
	}
}
#endif

#ifdef DMG_RANKING
ACMD(do_get_dmg_ranks)
{
	if (!ch)
		return;

	CDmgRankingManager::instance().ShowRanks(ch);
}
#endif

#ifdef BLACKJACK
ACMD(do_blackjack)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!ch || !*arg1 || !str_is_number(arg1))
		return;


	BYTE action = 0;
	str_to_number(action, arg1);

	if (action == 10 || action == 25 || action == 50) // stake choice
		ch->BlackJack_Start(action);
	else if (action <= 1)
		ch->BlackJack_Turn(action, 0);
}
#endif

ACMD(do_user_warp)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_user_warp") == 1)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	if (!ch || !*arg1)
		return;

	if (ch->IsPVPFighting())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait %d seconds to do this after the last time you were attacked."), 5);
		return;
	}

	if (!CHARACTER_GoToName(ch, ch->GetEmpire(), 0, arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map command syntax: /goto <mapname> [empire]");
		return;
	}
}

#ifdef __EQUIPMENT_CHANGER__
ACMD(do_equipment_changer_load)
{
	if (!ch)
		return;
	ch->SendEquipmentChangerLoadPacket();
}
#endif

#ifdef DUNGEON_REPAIR_TRIGGER
ACMD(do_dungeon_repair)
{
	if (!ch)
		return;

	quest::CQuestManager::instance().DungeonRepair(ch->GetPlayerID());
}
#endif

#ifdef ENABLE_REACT_EVENT
ACMD(do_react_event)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !str_is_number(arg2))
	{
		ch->tchat("Input error!");
		return;
	}

	BYTE type = 0;
	str_to_number(type, arg2);

	if (ch->GetMapIndex() != REACT_EVENT_MAP)
		return;

	// check if event is active?

	// type holds how the user input the react, need to validate it

	CEventManager::instance().React_OnPlayerReact(ch, arg1);
}
#endif

#ifdef HALLOWEEN_MINIGAME
ACMD(do_halloween_minigame)
{
	if (!ch)
		return;

	CEventManager::instance().HalloweenMinigame_StartRound(ch);
}
#endif

#ifdef DS_ALCHEMY_SHOP_BUTTON
ACMD(do_open_alchemy_shop)
{
	constexpr size_t SHOP_NPC_VNUM = 20001;
	
	auto shop = CShopManager::instance().GetByNPCVnum(20001);
	if (!shop)
		return;

	CShopManager::instance().StartShopping(ch, nullptr, shop->GetVnum());
}
#endif
