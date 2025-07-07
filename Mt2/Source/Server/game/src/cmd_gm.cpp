#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "packet.h"
#include "cmd.h"
#include "regen.h"
#include "guild.h"
#include "guild_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "fishing.h"
#include "mining.h"
#include "questmanager.h"
#include "vector.h"
#include "affect.h"
#include "db.h"
#include "priv_manager.h"
#include "building.h"
#include "battle.h"
#include "arena.h"
#include "start_position.h"
#include "party.h"
#include "xmas_event.h"
#include "log.h"
#include "unique_item.h"
#include "map_location.h"
#include "gm.h"
#include "mount_system.h"
#include "item_manager_private_types.h"
#include "SpamFilter.h"

#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif
#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
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

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

extern bool DropEvent_RefineBox_SetValue(const std::string& name, int value);

#ifdef __ANTI_BRUTEFORCE__
extern std::map<std::string, std::pair<DWORD, DWORD> > m_mapWrongLoginByHWID;
extern std::map<std::string, std::pair<DWORD, DWORD> > m_mapWrongLoginByIP;
#endif

// ADD_COMMAND_SLOW_STUN
enum
{
	COMMANDAFFECT_STUN,
	COMMANDAFFECT_SLOW,
};

void Command_ApplyAffect(LPCHARACTER ch, const char* argument, const char* affectName, int cmdAffect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	sys_log(0, arg1);

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: %s <name>", affectName);
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "%s is not in same map", arg1);
		return;
	}

	switch (cmdAffect)
	{
		case COMMANDAFFECT_STUN:
			SkillAttackAffect(tch, 1000, tch->IsPC() && tch->IsRiding() ? 0 : IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, 30, "GM_STUN");
			break;
		case COMMANDAFFECT_SLOW:
			SkillAttackAffect(tch, 1000, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 30, "GM_SLOW");
			break;
	}

	sys_log(0, "%s %s", arg1, affectName);

	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s %s", arg1, affectName);
}
// END_OF_ADD_COMMAND_SLOW_STUN

ACMD(do_rewarp)
{
	if (!ch)
		return;

	ch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
}

ACMD(do_stun)
{
	Command_ApplyAffect(ch, argument, "stun", COMMANDAFFECT_STUN);
}

ACMD(do_slow)
{
	Command_ApplyAffect(ch, argument, "slow", COMMANDAFFECT_SLOW);
}

#ifdef ENABLE_RUNE_SYSTEM
ACMD(do_rune)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: rune <vnum>");
		return;
	}

	int iRuneID;
	str_to_number(iRuneID, arg1);

	if (!ch->IsRuneOwned(iRuneID))
	{
		auto pProto = CRuneManager::instance().GetProto(iRuneID);
		if (!pProto)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no rune with ID #%u", iRuneID);
			return;
		}

		ch->SetRuneOwned(iRuneID);
		ch->ChatPacket(CHAT_TYPE_INFO, "Rune received : %s [%u]", pProto->name().c_str(), pProto->vnum());
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Already owned rune #%u", iRuneID);
}
#endif

ACMD(do_transfer)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: transfer <name>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1); 
	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			/*if (pkCCI->bChannel != g_bChannel && g_bChannel != 99 && pkCCI->bChannel != 99)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
				return;
			}*/

			network::GGOutputPacket<network::GGTransferPacket> pgg;
			pgg->set_name(arg1);
			pgg->set_x(ch->GetX());
			pgg->set_y(ch->GetY());
			P2P_MANAGER::instance().Send(pgg);
			ch->ChatPacket(CHAT_TYPE_INFO, "Transfer requested.");
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no character(%s) by that name", arg1);
			sys_log(0, "There is no character(%s) by that name", arg1);
		}

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Transfer me?!?");
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
	ch->ChatPacket(CHAT_TYPE_INFO, "Transfer requested.");
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

static std::vector<GotoInfo> gs_vec_gotoInfo;

void CHARACTER_AddGotoInfo(const std::string& c_st_name, BYTE empire, int mapIndex, DWORD x, DWORD y)
{
	GotoInfo newGotoInfo;
	newGotoInfo.st_name = c_st_name;
	newGotoInfo.empire = empire;
	newGotoInfo.mapIndex = mapIndex;
	newGotoInfo.x = x;
	newGotoInfo.y = y;
	gs_vec_gotoInfo.push_back(newGotoInfo);
	if(test_server)
		sys_log(0, "AddGotoInfo(name=%s, empire=%d, mapIndex=%d, pos=(%d, %d))", c_st_name.c_str(), empire, mapIndex, x, y);
}

bool FindInString(const char * c_pszFind, const char * c_pszIn)
{
	const char * c = c_pszIn;
	const char * p;

	p = strchr(c, '|');

	if (!p)
		return (0 == strncasecmp(c_pszFind, c_pszIn, strlen(c_pszFind)));
	else
	{
		char sz[64 + 1];

		do
		{
			strlcpy(sz, c, MIN(sizeof(sz), (p - c) + 1));

			if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
				return true;

			c = p + 1;
		} while ((p = strchr(c, '|')));

		strlcpy(sz, c, sizeof(sz));

		if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
			return true;
	}

	return false;
}

bool CHARACTER_GoToName(LPCHARACTER ch, BYTE empire, int mapIndex, const char* gotoName)
{
	std::vector<GotoInfo>::iterator i;
	for (i = gs_vec_gotoInfo.begin(); i != gs_vec_gotoInfo.end(); ++i)
	{
		const GotoInfo& c_eachGotoInfo = *i;

		if (mapIndex != 0)
		{
			if (mapIndex != c_eachGotoInfo.mapIndex)
				continue;
		}
		else if (!FindInString(gotoName, c_eachGotoInfo.st_name.c_str()))
			continue;

		if (c_eachGotoInfo.empire == 0 || c_eachGotoInfo.empire == empire)
		{
			int x, y;

			if (c_eachGotoInfo.x == 0 && c_eachGotoInfo.y == 0)
			{
				PIXEL_POSITION pos;
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(c_eachGotoInfo.mapIndex, empire, pos);
				x = pos.x;
				y = pos.y;
			}
			else
			{
				PIXEL_POSITION pos;
				SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(c_eachGotoInfo.mapIndex, pos);
				x = c_eachGotoInfo.x * 100 + pos.x;
				y = c_eachGotoInfo.y * 100 + pos.y;
			}

			ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
			ch->WarpSet(x, y);
			ch->Stop();
			return true;
		}
	}
	return false;
}

// END_OF_LUA_ADD_GOTO_INFO

ACMD(do_goto)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	int x = 0, y = 0, z = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 && !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: goto <x meter> <y meter>");
		return;
	}

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);

		PIXEL_POSITION p;

		if (SECTREE_MANAGER::instance().GetMapBasePosition(ch->GetX(), ch->GetY(), p))
		{
			x += p.x / 100;
			y += p.y / 100;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "You goto ( %d, %d )", x, y);
	}
	else
	{
		int mapIndex = 0;
		BYTE empire = 0;

		if (*arg1 == '#')
			str_to_number(mapIndex,  (arg1 + 1));

		if (*arg2 && isnhdigit(*arg2))
		{
			str_to_number(empire, arg2);
			empire = MINMAX(1, empire, 3);
		}
		else
			empire = ch->GetEmpire();

		if (!CHARACTER_GoToName(ch, empire, mapIndex, arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map command syntax: /goto <mapname> [empire]");
			return;
		}

		return;
	}

	x *= 100;
	y *= 100;

	ch->Show(ch->GetMapIndex(), x, y, z);
	ch->Stop();
}

ACMD(do_warp)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: warp <character name> | <x meter> <y meter>");
		return;
	}

	int x = 0, y = 0;
	long map_index = 0;

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);
	}
	else
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (NULL == tch)
		{
			const CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

			if (NULL != pkCCI)
			{
				if (pkCCI->bChannel != g_bChannel && g_bChannel != 99 && pkCCI->bChannel != 99)
				{
					/*ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
					return;*/
				}

				if (*arg2 && !strcmp(arg2, "INVISIBLE"))
					ch->SetGMInvisible(true);
				ch->WarpToPID( pkCCI->dwPID );
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
			}

			return;
		}
		else
		{
			x = tch->GetX() / 100;
			y = tch->GetY() / 100;
			map_index = tch->GetMapIndex();
		}

		if (*arg2 && !strcmp(arg2, "INVISIBLE"))
			ch->SetGMInvisible(true);
	}

	x *= 100;
	y *= 100;

	ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
	ch->WarpSet(x, y, map_index);
	ch->Stop();
}

ACMD(do_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item <item vnum>");
		return;
	}

	int iCount = 1;

	if (*arg2)
	{
		str_to_number(iCount, arg2);
		iCount = MINMAX(1, iCount, ITEM_MAX_COUNT);
	}

	DWORD dwVnum;

	if (isnhdigit(*arg1))
		str_to_number(dwVnum, arg1);
	else
	{
		if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%u item not exist by that vnum.", dwVnum);
			return;
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);

	if (item)
	{
#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
		{
			int iEmptyPos = ch->GetEmptyDragonSoulInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				M2_DESTROY_ITEM(item);
				if (!ch->DragonSoul_IsQualified())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "ÀÎº¥ÀÌ È°¼ºÈ­ µÇÁö ¾ÊÀ½.");
				}
				else
					ch->ChatPacket(CHAT_TYPE_INFO, "Not enough inventory space.");
			}
		}
		else
#endif
		{
			if (ch->TryStackItem(item, &item))
			{
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				BYTE bTargetWindow = ITEM_MANAGER::instance().GetTargetWindow(item);
				int iEmptyPos = ch->GetEmptyInventoryNew(bTargetWindow, item->GetSize());

				if (iEmptyPos != -1)
				{
					item->AddToCharacter(ch, TItemPos(bTargetWindow, iEmptyPos));
					LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
				}
				else
				{
					M2_DESTROY_ITEM(item);
					ch->ChatPacket(CHAT_TYPE_INFO, "Not enough inventory space.");
				}
			}
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "#%u item not exist by that vnum.", dwVnum);
	}
}

ACMD(do_group_random)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: grrandom <group vnum>");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);
	CHARACTER_MANAGER::instance().SpawnGroupGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_group)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: group <group vnum>");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);

	if (test_server)
		sys_log(0, "COMMAND GROUP SPAWN %u at %u %u %u", dwVnum, ch->GetMapIndex(), ch->GetX(), ch->GetY());

	CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_mob_coward)
{
	if (!ch)
		return;

	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mc <vnum>");
		return;
	}

	const CMob * pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.vnum();
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	iCount = MIN(20, iCount);

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
				ch->GetMapIndex(),
				ch->GetX() - random_number(200, 750), 
				ch->GetY() - random_number(200, 750), 
				ch->GetX() + random_number(200, 750), 
				ch->GetY() + random_number(200, 750), 
				true,
				pkMob->m_table.type() == CHAR_TYPE_STONE);
		if (tch)
			tch->SetCoward();
	}
}

ACMD(do_mob_map)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: mm <vnum>");
		return;
	}

	DWORD vnum = 0;
	str_to_number(vnum, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRandomPosition(vnum, ch->GetMapIndex());

	if (tch)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s spawned in %dx%d", tch->GetName(), tch->GetX(), tch->GetY());
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Spawn failed.");
}

ACMD(do_mob_aggresive)
{
	if (!ch)
		return;

	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob * pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.vnum();
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	iCount = MIN(20, iCount);

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
				ch->GetMapIndex(),
				ch->GetX() - random_number(200, 750), 
				ch->GetY() - random_number(200, 750), 
				ch->GetX() + random_number(200, 750), 
				ch->GetY() + random_number(200, 750), 
				true,
				pkMob->m_table.type() == CHAR_TYPE_STONE);
		if (tch)
			tch->SetAggressive();
	}
}

ACMD(do_mob)
{
	if (!ch)
		return;

	char	arg1[256], arg2[256];
	DWORD	vnum = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.vnum();
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	if (test_server)
		iCount = MIN(1000, iCount);
	else
		iCount = MIN(20, iCount);

	while (iCount--)
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
				ch->GetMapIndex(),
				ch->GetX() - random_number(200, 750), 
				ch->GetY() - random_number(200, 750), 
				ch->GetX() + random_number(200, 750), 
				ch->GetY() + random_number(200, 750), 
				true,
				pkMob->m_table.type() == CHAR_TYPE_STONE);

		if (tch && ch && ch->GetDungeon())
			tch->SetDungeon(ch->GetDungeon());

		if (!tch && ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Could not spawn monster.");
	}
}

ACMD(do_mob_ld)
{
	if (!ch)
		return;

	char	arg1[256], arg2[256], arg3[256], arg4[256];
	DWORD	vnum = 0;

	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.vnum();
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int dir = 1;
	long x=0,y=0;

	if (*arg2)
		str_to_number(x, arg2);
	if (*arg3)
		str_to_number(y, arg3);
	if (*arg4)
		str_to_number(dir, arg4);


	CHARACTER_MANAGER::instance().SpawnMob(vnum, 
		ch->GetMapIndex(),
		x*100, 
		y*100, 
		ch->GetZ(),
		pkMob->m_table.type() == CHAR_TYPE_STONE,
		dir);
}

struct FuncPurge
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncPurge(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)	// 10¹ÌÅÍ ÀÌ»ó¿¡ ÀÖ´Â °ÍµéÀº purge ÇÏÁö ¾Ê´Â´Ù.
			return;

		sys_log(0, "PURGE: %s %d", pkChr->GetName(), iDist);

		if (pkChr->IsPurgeable())
		{
			M2_DESTROY_CHARACTER(pkChr);
		}
	}
};

ACMD(do_purge)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncPurge func(ch);
	
	bool bMap = false;
	if (*arg1 && (!strcmp(arg1, "all") || !strcmp(arg1, "map") || !strcmp(arg1, "vid")))
	{
		
		func.m_bAll = true;
		if (!strcmp(arg1, "map"))
			bMap = true;
		else if (!strcmp(arg1, "vid"))
		{
			LPCHARACTER pkChr = ch->GetTarget();
			if(pkChr && pkChr->IsPurgeable())
				M2_DESTROY_CHARACTER(pkChr);
			return;
		}
	}

	if (!bMap)
	{
		LPSECTREE sectree = ch->GetSectree();
		if (sectree) // #431
			sectree->ForEachAround(func);
		else
			sys_err("PURGE_ERROR.NULL_SECTREE(mapIndex=%d, pos=(%d, %d))", ch->GetMapIndex(), ch->GetX(), ch->GetY());
	}
	else
	{
		LPSECTREE_MAP sectree_map = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
		if (sectree_map)
			sectree_map->for_each(func);
		else
			sys_err("PURGE_ERROR.NULL_MAP(mapIndex=%d)", ch->GetMapIndex());
	}
}

ACMD(do_item_purge)
{
	if (!ch)
		return;

	int		 i;
	LPITEM	  item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = ch->GetInventoryItem(i)))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}

#ifdef __DRAGONSOUL__
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
		}
	}
#endif
}

ACMD(do_state)
{
	if (!ch)
		return;

	char arg1[256];
	LPCHARACTER tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		if (arg1[0] == '#')
		{
			tch = CHARACTER_MANAGER::instance().Find(strtoul(arg1+1, NULL, 10));
		}
		else
		{
			LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);

			if (!d)
				tch = NULL;
			else
				tch = d->GetCharacter();
		}
	}
	else
		tch = ch;

	if (!tch)
	{
#ifdef __FAKE_PC__
		tch = ch->FakePC_Owner_GetSupporter();
		if (!tch)
			return;
#else
		return;
#endif
	}

	char buf[256];
	snprintf(buf, sizeof(buf), "%s's State: ", tch->GetName());

	if (tch->IsPosition(POS_FIGHTING))
		strlcat(buf, "Battle", sizeof(buf));
	else if (tch->IsPosition(POS_DEAD))
		strlcat(buf, "Dead", sizeof(buf));
	else
		strlcat(buf, "Standing", sizeof(buf));

	if (ch->GetShop())
		strlcat(buf, ", Shop", sizeof(buf));

	if (ch->GetExchange())
		strlcat(buf, ", Exchange", sizeof(buf));

	ch->ChatPacket(CHAT_TYPE_INFO, "%s - %s", buf, g_stHostname.c_str());

	int len;
	len = snprintf(buf, sizeof(buf), "Coordinate %ldx%ld (%ldx%ld) (rotation %.2f)", 
			tch->GetX(), tch->GetY(), tch->GetX() / 100, tch->GetY() / 100, tch->GetRotation());

	if (len < 0 || len >= (int) sizeof(buf))
		len = sizeof(buf) - 1;

	LPSECTREE pSec = SECTREE_MANAGER::instance().Get(tch->GetMapIndex(), tch->GetX(), tch->GetY());

	if (pSec)
	{
		TMapSetting& map_setting = SECTREE_MANAGER::instance().GetMap(tch->GetMapIndex())->m_setting;
		snprintf(buf + len, sizeof(buf) - len, " MapIndex %ld Attribute %08X Local Position (%ld x %ld)", 
			tch->GetMapIndex(), pSec->GetAttribute(tch->GetX(), tch->GetY()), (tch->GetX() - map_setting.iBaseX)/100, (tch->GetY() - map_setting.iBaseY)/100);
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);

	ch->ChatPacket(CHAT_TYPE_INFO, "LEV %d", tch->GetLevel());
	
#ifdef __PRESTIGE__
	ch->ChatPacket(CHAT_TYPE_INFO, "PRESTIGE %d", tch->Prestige_GetLevel());
#endif
	ch->ChatPacket(CHAT_TYPE_INFO, "HP %d/%d", tch->GetHP(), tch->GetMaxHP());
	ch->ChatPacket(CHAT_TYPE_INFO, "SP %d/%d", tch->GetSP(), tch->GetMaxSP());
	ch->ChatPacket(CHAT_TYPE_INFO, "ATT %d MAGIC_ATT %d SPD %d CAST_SPD %d CRIT %d%% PENE %d%% ATT_BONUS %d%%",
			tch->GetPoint(POINT_ATT_GRADE),
			tch->GetPoint(POINT_MAGIC_ATT_GRADE),
			tch->GetPoint(POINT_ATT_SPEED),
			tch->GetPoint(POINT_CASTING_SPEED),
			tch->GetPoint(POINT_CRITICAL_PCT),
			tch->GetPoint(POINT_PENETRATE_PCT),
			tch->GetPoint(POINT_ATT_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "DEF %d MAGIC_DEF %d BLOCK %d%% BLOCK_PEN %d%% DODGE %d%% DEF_BONUS %d%%", 
			tch->GetPoint(POINT_DEF_GRADE),
			tch->GetPoint(POINT_MAGIC_DEF_GRADE),
			tch->GetPoint(POINT_BLOCK),
			tch->GetPoint(POINT_BLOCK_IGNORE_BONUS),
			tch->GetPoint(POINT_DODGE),
			tch->GetPoint(POINT_DEF_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "RESISTANCES:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%% HUMANATT:%3d%% BOSS:%3d%%",
			tch->GetPoint(POINT_RESIST_WARRIOR),
			tch->GetPoint(POINT_RESIST_ASSASSIN),
			tch->GetPoint(POINT_RESIST_SURA),
			tch->GetPoint(POINT_RESIST_SHAMAN),
			tch->GetPoint(POINT_RESIST_ATTBONUS_HUMAN),
			tch->GetPoint(POINT_RESIST_BOSS));
	ch->ChatPacket(CHAT_TYPE_INFO, "   SWORD:%3d%% THSWORD:%3d%% DAGGER:%3d%% BELL:%3d%% FAN:%3d%% BOW:%3d%%",
			tch->GetPoint(POINT_RESIST_SWORD),
			tch->GetPoint(POINT_RESIST_TWOHAND),
			tch->GetPoint(POINT_RESIST_DAGGER),
			tch->GetPoint(POINT_RESIST_BELL),
			tch->GetPoint(POINT_RESIST_FAN),
			tch->GetPoint(POINT_RESIST_BOW));
	ch->ChatPacket(CHAT_TYPE_INFO, "   FIRE:%3d%% ELEC:%3d%% MAGIC:%3d%% WIND:%3d%% CRIT:%3d%% PENE:%3d%%",
			tch->GetPoint(POINT_RESIST_FIRE),
			tch->GetPoint(POINT_RESIST_ELEC),
			tch->GetPoint(POINT_RESIST_MAGIC),
			tch->GetPoint(POINT_RESIST_WIND),
			tch->GetPoint(POINT_RESIST_CRITICAL),
			tch->GetPoint(POINT_RESIST_PENETRATE));
	ch->ChatPacket(CHAT_TYPE_INFO, "   ICE:%3d%% EARTH:%3d%% DARK:%3d%%",
			tch->GetPoint(POINT_RESIST_ICE),
			tch->GetPoint(POINT_RESIST_EARTH),
			tch->GetPoint(POINT_RESIST_DARK));

	ch->ChatPacket(CHAT_TYPE_INFO, "MALL:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   ATT:%3d%% DEF:%3d%% EXP:%3d%% ITEMx%d GOLDx%d",
			tch->GetPoint(POINT_MALL_ATTBONUS),
			tch->GetPoint(POINT_MALL_DEFBONUS),
			tch->GetPoint(POINT_MALL_EXPBONUS),
			tch->GetPoint(POINT_MALL_ITEMBONUS) / 10,
			tch->GetPoint(POINT_MALL_GOLDBONUS) / 10);

	ch->ChatPacket(CHAT_TYPE_INFO, "BONUS:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   SKILL:%3d%% NORMAL:%3d%% SKILL_DEF:%3d%% NORMAL_DEF:%3d%%",
			tch->GetPoint(POINT_SKILL_DAMAGE_BONUS),
			tch->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS),
			tch->GetPoint(POINT_SKILL_DEFEND_BONUS),
			tch->GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS));

	ch->ChatPacket(CHAT_TYPE_INFO, "   HUMAN:%3d%% ANIMAL:%3d%% ORC:%3d%% MILGYO:%3d%% UNDEAD:%3d%% ELEM:%3d%% ZODIAC:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_HUMAN),
			tch->GetPoint(POINT_ATTBONUS_ANIMAL),
			tch->GetPoint(POINT_ATTBONUS_ORC),
			tch->GetPoint(POINT_ATTBONUS_MILGYO),
			tch->GetPoint(POINT_ATTBONUS_UNDEAD),
			tch->GetPoint(POINT_ATTBONUS_ALL_ELEMENTS),
#ifdef ENABLE_ZODIAC_TEMPLE
			tch->GetPoint(POINT_ATTBONUS_ZODIAC)
#else
			0
#endif
			);

	ch->ChatPacket(CHAT_TYPE_INFO, "   DEVIL:%3d%% INSECT:%3d%% FIRE:%3d%% ICE:%3d%% DESERT:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_DEVIL),
			tch->GetPoint(POINT_ATTBONUS_INSECT),
			tch->GetPoint(POINT_ATTBONUS_FIRE),
			tch->GetPoint(POINT_ATTBONUS_ICE),
			tch->GetPoint(POINT_ATTBONUS_DESERT));

	ch->ChatPacket(CHAT_TYPE_INFO, "   TREE:%3d%% MONSTER:%3d%% MONSTER_DIV10:%.1f%% STONE:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_TREE),
			tch->GetPoint(POINT_ATTBONUS_MONSTER),
			tch->GetPoint(POINT_ATTBONUS_MONSTER_DIV10) / 10.0f,
			tch->GetPoint(POINT_ATTBONUS_METIN));

	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_WARRIOR),
			tch->GetPoint(POINT_ATTBONUS_ASSASSIN),
			tch->GetPoint(POINT_ATTBONUS_SURA),
			tch->GetPoint(POINT_ATTBONUS_SHAMAN));

	ch->ChatPacket(CHAT_TYPE_INFO, "   SWORD:%3d%% 2HAND:%3d%% DAGGER:%3d%% BELL:%3d%% FAN:%3d%% BOW:%3d%%",
		tch->GetPoint(POINT_RESIST_SWORD_PEN),
		tch->GetPoint(POINT_RESIST_TWOHAND_PEN),
		tch->GetPoint(POINT_RESIST_DAGGER_PEN),
		tch->GetPoint(POINT_RESIST_BELL_PEN),
		tch->GetPoint(POINT_RESIST_FAN_PEN),
		tch->GetPoint(POINT_RESIST_BOW_PEN));


	for (int i = 0; i < MAX_PRIV_NUM; ++i)
		if (CPrivManager::instance().GetPriv(tch, i))
		{
			int iByEmpire = CPrivManager::instance().GetPrivByEmpire(tch->GetEmpire(), i);
			int iByGuild = 0;

			if (tch->GetGuild())
				iByGuild = CPrivManager::instance().GetPrivByGuild(tch->GetGuild()->GetID(), i);

			int iByPlayer = CPrivManager::instance().GetPrivByCharacter(tch->GetPlayerID(), i);

			if (iByEmpire)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for empire : %d", LC_TEXT(ch, c_apszPrivNames[i]), iByEmpire);

			if (iByGuild)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for guild : %d", LC_TEXT(ch, c_apszPrivNames[i]), iByGuild);

			if (iByPlayer)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for player : %d", LC_TEXT(ch, c_apszPrivNames[i]), iByPlayer);
		}
}

struct notice_packet_func
{
	const char * m_str;
	int m_lang;
	BYTE m_empire;
	bool m_isbig;

	notice_packet_func(const char * str, int lang, BYTE empire, bool is_big) : m_str(str), m_lang(lang), m_empire(empire), m_isbig(is_big)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter() || (m_empire && d->GetEmpire() != m_empire))
			return;

		if (m_lang == -1)
			d->GetCharacter()->ChatPacket(m_isbig ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", LC_TEXT(d->GetCharacter(), m_str));
		else if (m_lang == d->GetCharacter()->GetLanguageID())
			d->GetCharacter()->ChatPacket(m_isbig ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

struct success_notice_packet_func
{
	const char * m_str;
	int m_iLang;

	success_notice_packet_func(const char * str, int iLang) : m_str(str), m_iLang(iLang)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (m_iLang == -1)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "|cFF00CCFF|h%s", LC_TEXT(d->GetCharacter(), m_str));
		else if (d->GetCharacter()->GetLanguageID() == m_iLang)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "|cFF00CCFF|h%s", m_str);
	}
};

void SendNotice(const char * c_pszBuf, int iLangID)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_packet_func(c_pszBuf, iLangID, 0, false));
}

void SendBigNotice(const char * c_pszBuf, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_packet_func(c_pszBuf, -1, bEmpire, true));
}

void SendSuccessNotice(const char * c_pszBuf, int iLang)
{
	char szBuf[CHAT_MAX_LEN + 1];
	snprintf(szBuf, sizeof(szBuf), "%s", c_pszBuf);

	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), success_notice_packet_func(c_pszBuf, iLang));
}

struct notice_map_packet_func
{
	const char* m_str;
	int m_mapIndex;
	bool m_bigFont;
	int m_lang;

	notice_map_packet_func(const char* str, int idx, bool bigFont, int lang) : m_str(str), m_mapIndex(idx), m_bigFont(bigFont), m_lang(lang)
	{
	}

	void operator() (LPDESC d)
	{
		if (d->GetCharacter() == NULL) return;
		if (d->GetCharacter()->GetMapIndex() != m_mapIndex) return;

		if (m_lang == -1)
			d->GetCharacter()->ChatPacket(m_bigFont == true ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", LC_TEXT(d->GetCharacter(), m_str));
		else if (m_lang == d->GetCharacter()->GetLanguageID())
			d->GetCharacter()->ChatPacket(m_bigFont == true ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

void SendNoticeMap(const char* c_pszBuf, int nMapIndex, bool bBigFont, int iLangID)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_map_packet_func(c_pszBuf, nMapIndex, bBigFont, iLangID));
}

struct log_packet_func
{
	const char * m_str;

	log_packet_func(const char * str) : m_str(str)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetGMLevel() > GM_PLAYER)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
	}
};


void SendLog(const char * c_pszBuf)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), log_packet_func(c_pszBuf));
}

void BroadcastNotice(const char * c_pszBuf, int iLangID, BYTE bChannel)
{
	network::GGOutputPacket<network::GGNoticePacket> p;
	p->set_lang_id(iLangID);
	p->set_channel(bChannel);
	p->set_message(c_pszBuf);
	P2P_MANAGER::instance().Send(p); // HEADER_GG_NOTICE

	if (!bChannel || bChannel == g_bChannel)
		SendNotice(c_pszBuf, iLangID);
}

void BroadcastBigNotice(const char * c_pszBuf, BYTE bEmpire)
{
	network::GGOutputPacket<network::GGNoticePacket> p;
	p->set_empire(bEmpire);
	p->set_big_font(true);
	p->set_message(c_pszBuf);
	P2P_MANAGER::instance().Send(p); // HEADER_GG_NOTICE

	SendBigNotice(c_pszBuf, bEmpire);
}
void BroadcastSuccessNotice(const char * c_pszBuf, int iLang)
{
	network::GGOutputPacket<network::GGSuccessNoticePacket> p;
	p->set_lang_id(iLang);
	p->set_message(c_pszBuf);
	P2P_MANAGER::instance().Send(p); // HEADER_GG_NOTICE

	SendSuccessNotice(c_pszBuf, iLang);
}

ACMD(do_notice)
{
	if (test_server)
		sys_log(0, "notice %s", argument);

	char arg1[256];
	const char* szBaseArg = argument;
	argument = one_argument(argument, arg1, sizeof(arg1));

	int iLangID;
	if (!*argument || !str_is_number(arg1) || !(str_to_number(iLangID, arg1)))
	{
		iLangID = ch ? ch->GetLanguageID() : -1;
		argument = szBaseArg + 1;

		if (!*szBaseArg || !*argument)
		{
			sys_err("invalid call of notice [ no argument ]");
			return;
		}
	}

	if (iLangID < 0 || iLangID >= LANGUAGE_MAX_NUM)
		iLangID = -1;

	char szBuf[CHAT_MAX_LEN + 1];
	if (ch)
	{
		snprintf(szBuf, sizeof(szBuf), "|cFFFFE100|h%s : %s", ch->GetName(), argument);

		if (iLangID == -1)
			BroadcastNotice(CLocaleManager::Instance().GetIgnoreTranslationString(szBuf).c_str());
		else
			BroadcastNotice(szBuf, iLangID);
	}
	else
	{
		snprintf(szBuf, sizeof(szBuf), "|cFFFFE100|h%s", argument);

		if (iLangID == -1)
			SendNotice(CLocaleManager::Instance().GetIgnoreTranslationString(argument).c_str());
		else
			SendNotice(argument, iLangID);
	}
}

ACMD(do_map_notice)
{
	if (!ch)
		return;

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	int iLangID;
	if (!str_is_number(arg1) || !(str_to_number(iLangID, arg1)))
		return;

	if (iLangID < 0 || iLangID >= LANGUAGE_MAX_NUM)
		iLangID = -1;

	if (iLangID == -1)
		SendNoticeMap(CLocaleManager::Instance().GetIgnoreTranslationString(argument).c_str(), ch->GetMapIndex(), false);
	else
		SendNoticeMap(argument, ch->GetMapIndex(), false, iLangID);
}

ACMD(do_big_notice)
{
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", argument);
}

#ifdef ENABLE_ZODIAC_TEMPLE
ACMD(do_zodiac_notice)
{
	ch->ChatPacket(CHAT_TYPE_ZODIAC_NOTICE, "%s", argument);
}
#endif

class user_func
{
	public:
		LPCHARACTER	m_ch;
		static int count;
		static char str[128];
		static int str_len;

		user_func()
			: m_ch(NULL)
		{}

		void initialize(LPCHARACTER ch)
		{
			m_ch = ch;
			str_len = 0;
			count = 0;
			str[0] = '\0';
		}

		void operator () (LPDESC d)
		{
			if (!d->GetCharacter())
				return;

			int len = snprintf(str + str_len, sizeof(str) - str_len, "%-16s ", d->GetCharacter()->GetName());

			if (len < 0 || len >= (int) sizeof(str) - str_len)
				len = (sizeof(str) - str_len) - 1;

			str_len += len;
			++count;

			if (!(count % 4))
			{
				m_ch->ChatPacket(CHAT_TYPE_INFO, str);

				str[0] = '\0';
				str_len = 0;
			}
		}
};

int	user_func::count = 0;
char user_func::str[128] = { 0, };
int	user_func::str_len = 0;

ACMD(do_user)
{
	if (!ch)
		return;

	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	user_func func;

	func.initialize(ch);
	std::for_each(c_ref_set.begin(), c_ref_set.end(), func);

	if (func.count % 4)
		ch->ChatPacket(CHAT_TYPE_INFO, func.str);

	ch->ChatPacket(CHAT_TYPE_INFO, "Total %d", func.count);
}

ACMD(do_disconnect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "ex) /dc <player name>");
		return;
	}

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER	tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player.", arg1);
		return;
	}
	
	if (tch == ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You cannot disconnect yourself.");
		return;
	}

	DESC_MANAGER::instance().DestroyDesc(d);
}

struct FuncKill
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncKill(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;
		if (!pkChr || (!pkChr->IsMonster() && !pkChr->IsStone()) || (!m_bAll && 2000 <= DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY())))
			return;

		sys_log(0, "KillAll: %s ", pkChr->GetName());
		pkChr->Dead(m_pkGM);
	}
};

ACMD(do_killall)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncKill func(ch);

	bool bMap = false;
	if (*arg1 && (!strcmp(arg1, "all") || !strcmp(arg1, "map")))
	{
		func.m_bAll = true;
		if (!strcmp(arg1, "map"))
			bMap = true;
	}

	if (!bMap)
	{
		LPSECTREE sectree = ch->GetSectree();
		if (sectree)
			sectree->ForEachAround(func);
		else
			sys_err("KILLALL_ERROR.NULL_SECTREE(mapIndex=%d, pos=(%d, %d))", ch->GetMapIndex(), ch->GetX(), ch->GetY());
	}
	else
	{
		LPSECTREE_MAP sectree_map = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
		if (sectree_map)
			sectree_map->for_each(func);
		else
			sys_err("KILLALL_ERROR.NULL_MAP(mapIndex=%d)", ch->GetMapIndex());
	}
}

ACMD(do_kill)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
		{
			LPCHARACTER pkChr = ch->GetTarget();
			if (pkChr && ch->GetGMLevel() == GM_IMPLEMENTOR)
				pkChr->Dead(ch);
			else
				ch->Dead();
		}
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player", arg1);
		return;
	}

	tch->Dead();
}

#define MISC	0
#define BINARY  1
#define NUMBER  2

const struct set_struct 
{
	const char *cmd;
	const char type;
} set_fields[] = {
	{ "gold",		NUMBER	},
	{ "race",		BINARY	},
	{ "sex",		BINARY	},
	{ "exp",		NUMBER	},
	{ "max_hp",		NUMBER	},
	{ "max_sp",		NUMBER	},
	{ "job",		NUMBER	},
	{ "skill",		NUMBER	},
	{ "alignment",	NUMBER	},
	{ "align",		NUMBER	},
#ifdef __ANIMAL_SYSTEM__
	{ "mount_exp",	NUMBER	},
#ifdef __PET_SYSTEM__
	{ "pet_exp",	NUMBER	},
#endif
#endif
#ifdef __GAYA_SYSTEM__
	{ "gaya",		NUMBER	},
#endif
	{ "\n",			MISC	}
};

enum ESetData {
	SET_IDX_GOLD,
	SET_IDX_RACE,
	SET_IDX_SEX,
	SET_IDX_EXP,
	SET_IDX_MAX_HP,
	SET_IDX_MAX_SP,
	SET_IDX_JOB,
	SET_IDX_SKILL,
	SET_IDX_ALIGNMENT,
	SET_IDX_ALIGN,
#ifdef __ANIMAL_SYSTEM__
	SET_IDX_MOUNT_EXP,
#ifdef __PET_SYSTEM__
	SET_IDX_PET_EXP,
#endif
#endif
#ifdef __GAYA_SYSTEM__
	SET_IDX_GAYA,
#endif
};

ACMD(do_set)
{
	if (!ch)
		return;

	//if (!test_server)
	//	return;

	char arg1[256], arg2[256], arg3[256];

	LPCHARACTER tch = NULL;

	int i, len;
	const char* line;

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: set <name> <field> <value>");
		return;
	}

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s not exist", arg1);
		return;
	}

	len = strlen(arg2);

	for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
		if (!strncmp(arg2, set_fields[i].cmd, len))
			break;

	switch (i)
	{
		case SET_IDX_GOLD:
			{
				long long gold = 0;
				str_to_number(gold, arg3);
				LogManager::instance().MoneyLog(MONEY_LOG_MISC, 3, gold);
				long long before_gold = tch->GetGold();
				if (before_gold + gold >= GOLD_MAX)
					gold = GOLD_MAX - before_gold;
				tch->PointChange(POINT_GOLD, gold, true);
				long long after_gold = tch->GetGold();
				if (0 == after_gold && 0 != before_gold)
				{
					LogManager::instance().CharLog(tch, gold, "ZERO_GOLD", "GM");
				}
			}
			break;

		case SET_IDX_RACE: // race
			break;

		case SET_IDX_SEX: // sex
			break;

		case SET_IDX_EXP: // exp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_EXP, amount, true);
			}
			break;

		case SET_IDX_MAX_HP: // max_hp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_MAX_HP, amount, true);
			}
			break;

		case SET_IDX_MAX_SP: // max_sp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_MAX_SP, amount, true);
			}
			break;

		case SET_IDX_JOB: // set player job
			{
				int amount = 0;
				str_to_number(amount, arg3);
				amount = MINMAX(0, amount, 2);
				if (amount != tch->GetSkillGroup())
				{
					tch->ClearSkill();
					tch->SetSkillGroup(amount);
				}
			}
			break;

		case SET_IDX_SKILL: // active skill point
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_SKILL, amount, true);
			}
			break;

		case SET_IDX_ALIGNMENT: // alignment
		case SET_IDX_ALIGN: // alignment
			{
				int	amount = 0;
				str_to_number(amount, arg3);
				tch->UpdateAlignment(amount - ch->GetRealAlignment());
			}
			break;

#ifdef __ANIMAL_SYSTEM__
		case SET_IDX_MOUNT_EXP:
		{
			long amount = 0;
			str_to_number(amount, arg3);

			if (!tch->GetMountSystem())
				return;

			if (tch->GetMountSystem()->IsSummoned() || tch->GetMountSystem()->IsRiding())
			{
				LPITEM pkItem = tch->FindItemByID(tch->GetMountSystem()->GetSummonItemID());
				if (!pkItem)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Cannot set exp of mount of %s.", tch->GetName());
					return;
				}

				pkItem->Animal_GiveEXP(amount);
			}
			}
			break;

#ifdef __PET_SYSTEM__
		case SET_IDX_PET_EXP:
			{
				long amount = 0;
				str_to_number(amount, arg3);

				if (!tch->GetPetSystem())
					return;

				if (CPetActor* pPetActor = tch->GetPetSystem()->GetSummoned())
				{
					LPITEM pkItem = tch->FindItemByVID(pPetActor->GetSummonItemVID());
					if (!pkItem)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, "Cannot set exp of pet of %s.", tch->GetName());
						return;
					}

					pkItem->Animal_GiveEXP(amount);
				}
			}
			break;
#endif
#endif

#ifdef __GAYA_SYSTEM__
		case SET_IDX_GAYA:
			{
				int gaya = 0;
				str_to_number(gaya, arg3);
				tch->ChangeGaya(gaya);
			}
			break;
#endif
	}

	if (set_fields[i].type == NUMBER)
	{
		long long amount = 0;
		str_to_number(amount, arg3);
		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s set to [%lld]", tch->GetName(), set_fields[i].cmd, amount);
	}
}

ACMD(do_reset)
{
	if (!ch)
		return;

	ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
	ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
	ch->Save();
}

ACMD(do_advance)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: advance <name> <level>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s not exist", arg1);
		return;
	}

	int level = 0;
	str_to_number(level, arg2);

	tch->ResetPoint(MINMAX(0, level, PLAYER_MAX_LEVEL_CONST));
}

ACMD(do_respawn)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1 && !strcasecmp(arg1, "all"))
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Respaw everywhere");
		regen_reset(0, 0);
	}
	else if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Respaw around");
		regen_reset(ch->GetX(), ch->GetY());
	}
}

ACMD(do_safebox_size)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int size = 0;

	if (*arg1)
		str_to_number(size, arg1);

	if (size > 3 || size < 0)
		size = 0;

	ch->ChatPacket(CHAT_TYPE_INFO, "Safebox size set to %d", size);
	ch->ChangeSafeboxSize(size);
}

ACMD(do_makeguild)
{
	if (!ch)
		return;

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, arg1, sizeof(cp.name));

	if (!check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀûÇÕÇÏÁö ¾ÊÀº ±æµå ÀÌ¸§ ÀÔ´Ï´Ù."));
		return;
	}

	gm.CreateGuild(cp);
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "(%s) ±æµå°¡ »ý¼ºµÇ¾ú½À´Ï´Ù. [ÀÓ½Ã]"), cp.name);
}

ACMD(do_deleteguild)
{
	if (!ch)
		return;

	if (ch->GetGuild())
		ch->GetGuild()->RequestDisband(ch->GetPlayerID());
}

ACMD(do_greset)
{
	if (!ch)
		return;

	if (ch->GetGuild())
		ch->GetGuild()->Reset();
}

// REFINE_ROD_HACK_BUG_FIX
ACMD(do_refine_rod)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
		fishing::RealRefineRod(ch, item);
}
// END_OF_REFINE_ROD_HACK_BUG_FIX

// REFINE_PICK
ACMD(do_refine_pick)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
		mining::RealRefinePick(ch, item);
	}
}

ACMD(do_max_pick)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
	}
}
// END_OF_REFINE_PICK


ACMD(do_fishing_simul)
{
	if (!ch)
		return;

	char arg1[256];
	char arg2[256];
	char arg3[256];
	argument = one_argument(argument, arg1, sizeof(arg1));
	two_arguments(argument, arg2, sizeof(arg2), arg3, sizeof(arg3));

	int count = 1000;
	int prob_idx = 0;
	int level = 100;

	ch->ChatPacket(CHAT_TYPE_INFO, "Usage: fishing_simul <level> <prob index> <count>");

	if (*arg1)
		str_to_number(level, arg1);

	if (*arg2)
		str_to_number(prob_idx, arg2);

	if (*arg3)
		str_to_number(count, arg3);

	fishing::Simulation(level, count, prob_idx, ch);
}

ACMD(do_invisibility)
{
	if (!ch)
		return;

	ch->SetGMInvisible(!ch->IsGMInvisible());
}

ACMD(do_event_flag)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!(*arg1) || !(*arg2))
		return;

	int value = 0;
	str_to_number(value, arg2);

	if (!strcmp(arg1, "mob_item") || 
			!strcmp(arg1, "mob_exp") ||
			!strcmp(arg1, "mob_gold") ||
			!strcmp(arg1, "mob_dam") ||
			!strcmp(arg1, "mob_gold_pct") ||
			!strcmp(arg1, "mob_item_buyer") || 
			!strcmp(arg1, "mob_exp_buyer") ||
			!strcmp(arg1, "mob_gold_buyer") ||
			!strcmp(arg1, "mob_gold_pct_buyer")
	   )
		value = MINMAX(0, value, 1000);

	//quest::CQuestManager::instance().SetEventFlag(arg1, atoi(arg2));
	quest::CQuestManager::instance().RequestSetEventFlag(arg1, value);
	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "RequestSetEventFlag %s %d", arg1, value);
	sys_log(0, "RequestSetEventFlag %s %d", arg1, value);
}

ACMD(do_get_event_flag)
{
	if (!ch)
		return;

	quest::CQuestManager::instance().SendEventFlagList(ch);
}

ACMD(do_private)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: private <map index>");
		return;
	}

	long lMapIndex;
	long map_index = 0;
	str_to_number(map_index, arg1);
	if ((lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(map_index)))
	{
		ch->SaveExitLocation();

		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
		ch->WarpSet(pkSectreeMap->m_setting.posSpawn.x, pkSectreeMap->m_setting.posSpawn.y, lMapIndex); 
	}
	else
	{
		long lAddr;
		WORD wPort;
		if (CMapLocation::instance().Get(map_index, lAddr, wPort))
		{
			network::GGOutputPacket<network::GGRequestDungeonWarpPacket> packet;
			packet->set_type(P2P_DUNGEON_WARP_PLAYER);
			packet->set_player_id(ch->GetPlayerID());
			packet->set_map_index(map_index);
			if (P2P_MANAGER::instance().Send(packet, wPort))
				ch->ChatPacket(CHAT_TYPE_INFO, "Request for dungeon of map index %d sent.", map_index);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find p2p-Connection by port %u for map index %d", wPort, map_index);
		}
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Can't find map by index %d", map_index);
	}
}

ACMD(do_qf)
{
	if (!ch)
		return;

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	std::string questname = pPC->GetCurrentQuestName();

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, arg1);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it!= pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag failed");
	}
}

LPCHARACTER chHori, chForge, chLib, chTemple, chTraining, chTree, chPortal, chBall;

ACMD(do_b1)
{
	//È£¸®º´ 478 579
	chHori = CHARACTER_MANAGER::instance().SpawnMobRange(14017, ch->GetMapIndex(), 304222, 742858, 304222, 742858, true, false);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_CONSTRUCTION_SMALL, 65535, 0, true);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = random_number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(random_number(701, 706), 
				ch->GetMapIndex(),
				304222 + (int)fx,
				742858 + (int)fy,
				304222 + (int)fx,
				742858 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = random_number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009, 
				ch->GetMapIndex(),
				304222 + (int)fx,
				742858 + (int)fy,
				304222 + (int)fx,
				742858 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}
}

ACMD(do_b2)
{
	chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
}

ACMD(do_b3)
{
	// Æ÷Áö 492 547
	chForge = CHARACTER_MANAGER::instance().SpawnMobRange(14003, ch->GetMapIndex(), 307500, 746300, 307500, 746300, true, false);
	chForge->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//³ôÀºÅ¾ 509 589 -> µµ¼­°ü
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14007, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//¿åÁ¶ 513 606 -> ÈûÀÇ½ÅÀü
	chTemple = CHARACTER_MANAGER::instance().SpawnMobRange(14004, ch->GetMapIndex(), 307700, 741600, 307700, 741600, true, false);
	chTemple->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//±ÇÅõÀå 490 625
	chTraining= CHARACTER_MANAGER::instance().SpawnMobRange(14010, ch->GetMapIndex(), 307100, 739500, 307100, 739500, true, false);
	chTraining->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//³ª¹« 466 614
	chTree= CHARACTER_MANAGER::instance().SpawnMobRange(14013, ch->GetMapIndex(), 300800, 741600, 300800, 741600, true, false);
	chTree->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//Æ÷Å» 439 615
	chPortal= CHARACTER_MANAGER::instance().SpawnMobRange(14001, ch->GetMapIndex(), 300900, 744500, 300900, 744500, true, false);
	chPortal->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	// ±¸½½ 436 600
	chBall = CHARACTER_MANAGER::instance().SpawnMobRange(14012, ch->GetMapIndex(), 302500, 746600, 302500, 746600, true, false);
	chBall->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b4)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = random_number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(random_number(701, 706), 
				ch->GetMapIndex(),
				307900 + (int)fx,
				744500 + (int)fy,
				307900 + (int)fx,
				744500 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = random_number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009, 
				ch->GetMapIndex(),
				307900 + (int)fx,
				744500 + (int)fy,
				307900 + (int)fx,
				744500 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

}

ACMD(do_b5)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14008, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b6)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);
}
ACMD(do_b7)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14009, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_book)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	CSkillProto * pkProto;

	if (isnhdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pkProto = CSkillManager::instance().Get(vnum);
	}
	else 
		pkProto = CSkillManager::instance().Get(arg1);

	if (!pkProto)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such a skill.");
		return;
	}

	LPITEM item = ch->AutoGiveItem(50300);
	item->SetSocket(0, pkProto->dwVnum);
}

ACMD(do_setskillother)
{
	if (ch)
		return;

	char arg1[256], arg2[256], arg3[256];
	argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(argument, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3 || !isdigit(*arg3))
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskillother <target> <skillname> <lev>");
		return;
	}

	LPCHARACTER tch;

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg2))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg2);
		pk = CSkillManager::instance().Get(vnum);
	}
	else
		pk = CSkillManager::instance().Get(arg2);

	if (!pk)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg3);
	tch->SetSkillLevel(pk->dwVnum, level);
	tch->ComputePoints();
	tch->SkillLevelPacket();
}

ACMD(do_setskill)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskill <name> <lev>");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pk = CSkillManager::instance().Get(vnum);
	}

	else
		pk = CSkillManager::instance().Get(arg1);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg2);
	ch->SetSkillLevel(pk->dwVnum, level);
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD(do_set_skill_point)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_point = 0;
	if (*arg1)
		str_to_number(skill_point, arg1);

	ch->SetRealPoint(POINT_SKILL, skill_point);
	ch->SetPoint(POINT_SKILL, ch->GetRealPoint(POINT_SKILL));
	ch->PointChange(POINT_SKILL, 0);
}

ACMD(do_set_skill_group)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_group = 0;
	if (*arg1)
		str_to_number(skill_group, arg1);

	ch->SetSkillGroup(skill_group);
	
	ch->ClearSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "skill group to %d.", skill_group);
}

ACMD(do_reload)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	SendLog("CAUTION:::::Reloading.....");
	
	network::GGOutputPacket<network::GGReloadCommandPacket> p2p_packet;
	p2p_packet->set_argument(arg1);
	bool bSendP2P = false;

	if (*arg1)
	{
		switch (LOWER(*arg1))
		{
			case 'l':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading language.");
				CLocaleManager::instance().Initialize();
				bSendP2P = true;
				break;

			case 'b':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading banned words.");
				CSpamFilter::Instance().InitializeSpamFilterTable();
				bSendP2P = true;
				break;

			case 'i':
				ITEM_MANAGER::instance().LoadDisabledItemList();
				bSendP2P = true;
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Disabled Item List");
				break;

			case 'u':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading Uppitem-Table.");
				ITEM_MANAGER::instance().LoadUppItemList();
				bSendP2P = true;
				break;

			case 'p':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading prototype tables,");
				db_clientdesc->DBPacket(network::TGDHeader::RELOAD_PROTO);
				break;

			case 'q':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading quest.");
				quest::CQuestManager::instance().Reload();
				bSendP2P = true;
				break;

			case 'f':
				fishing::Initialize();
				bSendP2P = true;
				break;
				
			case 'a':
				{
					if (ch)
						ch->ChatPacket(CHAT_TYPE_INFO, "Reloading Admin infomation.");

					network::GDOutputPacket<network::GDReloadAdminPacket> pack;
					db_clientdesc->DBPacket(pack);
					sys_log(0, "Reloading admin infomation.");
				}
				break;
				
			case 'c':
				Cube_init ();
				bSendP2P = true;
				break;
				
			case 'm':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading mob_proto...");
				db_clientdesc->DBPacket(network::TGDHeader::RELOAD_MOB_PROTO);
				break;

			case 's':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading shop table...");
				db_clientdesc->DBPacket(network::TGDHeader::RELOAD_SHOP);
				break;
				
			case 'd':
				{
					if (ch)
						ch->ChatPacket(CHAT_TYPE_INFO, "Reloading mob_drop, common_drop, special_item ...");

					for (int i = 0; i < MOB_RANK_MAX_NUM; ++i)
						g_vec_pkCommonDropItem[i].clear();
					ITEM_MANAGER::Instance().GetMobItemGroupMap().clear();
					ITEM_MANAGER::Instance().GetDropItemGroupMap().clear();
					ITEM_MANAGER::Instance().GetLevelItemGroupMap().clear();
					ITEM_MANAGER::Instance().GetSpecialItemGroupMap().clear();
				
					const int FILE_NAME_LEN = 256;
					char szCommonDropItemFileName[FILE_NAME_LEN];
					char szMOBDropItemFileName[FILE_NAME_LEN];
					char szSpecialItemGroupFileName[FILE_NAME_LEN];
				
					snprintf(szCommonDropItemFileName, sizeof(szCommonDropItemFileName),
							"%s/common_drop_item.txt", Locale_GetBasePath().c_str());
					snprintf(szMOBDropItemFileName, sizeof(szMOBDropItemFileName),
							"%s/mob_drop_item.txt", Locale_GetBasePath().c_str());
					snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
							"%s/special_item_group.txt", Locale_GetBasePath().c_str());

					sys_log(0, "LoadLocaleFile: CommonDropItem: %s", szCommonDropItemFileName);
					if (!ITEM_MANAGER::instance().ReadCommonDropItemFile(szCommonDropItemFileName))
					{
						sys_err("cannot load CommonDropItem: %s", szCommonDropItemFileName);
						if (ch)
							ch->ChatPacket(CHAT_TYPE_INFO, "ERROR: cannot load %s", szCommonDropItemFileName);
						return;
					}
					sys_log(0, "LoadLocaleFile: MOBDropItemFile: %s", szMOBDropItemFileName);
					if (!ITEM_MANAGER::instance().ReadMonsterDropItemGroup(szMOBDropItemFileName))
					{
						sys_err("cannot load MOBDropItemFile: %s", szMOBDropItemFileName);
						if (ch)
							ch->ChatPacket(CHAT_TYPE_INFO, "ERROR: cannot load %s", szMOBDropItemFileName);
						return;
					}
					sys_log(0, "LoadLocaleFile: SpecialItemGroup: %s", szSpecialItemGroupFileName);
					if (!ITEM_MANAGER::instance().ReadSpecialDropItemFile(szSpecialItemGroupFileName))
					{
						sys_err("cannot load SpecialItemGroup: %s", szSpecialItemGroupFileName);
						if (ch)
							ch->ChatPacket(CHAT_TYPE_INFO, "ERROR: cannot load %s", szSpecialItemGroupFileName);
						return;
					}
					p2p_packet->set_argument("d");
					bSendP2P = true;
					break;
				}
				
			case '%':
				{
					if (ch)
						ch->ChatPacket(CHAT_TYPE_INFO, "Reloading mob_drop ...");


					ITEM_MANAGER::Instance().GetMobItemGroupMap().clear();
					ITEM_MANAGER::Instance().GetDropItemGroupMap().clear();
					ITEM_MANAGER::Instance().GetLevelItemGroupMap().clear();
				
					const int FILE_NAME_LEN = 256;
					char szMOBDropItemFileName[FILE_NAME_LEN];

					snprintf(szMOBDropItemFileName, sizeof(szMOBDropItemFileName),
							"%s/mob_drop_item.txt", Locale_GetBasePath().c_str());


					sys_log(0, "LoadLocaleFile: MOBDropItemFile: %s", szMOBDropItemFileName);
					if (!ITEM_MANAGER::instance().ReadMonsterDropItemGroup(szMOBDropItemFileName))
					{
						sys_err("cannot load MOBDropItemFile: %s", szMOBDropItemFileName);
						if (ch)
							ch->ChatPacket(CHAT_TYPE_INFO, "ERROR: cannot load %s", szMOBDropItemFileName);
						return;
					}
					p2p_packet->set_argument("%");
					bSendP2P = true;
					break;
				}
				
			case '8':
				{
					if (ch)
						ch->ChatPacket(CHAT_TYPE_INFO, "Reloading  special_item ...");

					ITEM_MANAGER::Instance().GetSpecialItemGroupMap().clear();
				
					const int FILE_NAME_LEN = 256;
					char szSpecialItemGroupFileName[FILE_NAME_LEN];

					snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
							"%s/special_item_group.txt", Locale_GetBasePath().c_str());

					sys_log(0, "LoadLocaleFile: SpecialItemGroup: %s", szSpecialItemGroupFileName);
					if (!ITEM_MANAGER::instance().ReadSpecialDropItemFile(szSpecialItemGroupFileName))
					{
						sys_err("cannot load SpecialItemGroup: %s", szSpecialItemGroupFileName);
						if (ch)
							ch->ChatPacket(CHAT_TYPE_INFO, "ERROR: cannot load %s", szSpecialItemGroupFileName);
						return;
					}
					p2p_packet->set_argument("8");
					bSendP2P = true;
					break;
				}
				
			case '9':
				{
					if (ch)
						ch->ChatPacket(CHAT_TYPE_INFO, "Reloading common_drop ...");

					for (int i = 0; i < MOB_RANK_MAX_NUM; ++i)
						g_vec_pkCommonDropItem[i].clear();
					
				
					const int FILE_NAME_LEN = 256;
					char szCommonDropItemFileName[FILE_NAME_LEN];
				
					snprintf(szCommonDropItemFileName, sizeof(szCommonDropItemFileName),
							"%s/common_drop_item.txt", Locale_GetBasePath().c_str());

					sys_log(0, "LoadLocaleFile: CommonDropItem: %s", szCommonDropItemFileName);
					if (!ITEM_MANAGER::instance().ReadCommonDropItemFile(szCommonDropItemFileName))
					{
						sys_err("cannot load CommonDropItem: %s", szCommonDropItemFileName);
						if (ch)
							ch->ChatPacket(CHAT_TYPE_INFO, "ERROR: cannot load %s", szCommonDropItemFileName);
						return;
					}
					p2p_packet->set_argument("9");
					bSendP2P = true;
					break;
				}
#ifdef ENABLE_XMAS_EVENT
			case 'x':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading xmas_rewards table");

				db_clientdesc->DBPacket(network::TGDHeader::RELOAD_XMAS);
				break;
#endif
#ifdef COMBAT_ZONE
			case 'z':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading combat_zone ranking");

				CCombatZoneManager::instance().InitializeRanking();
				bSendP2P = true;
				break;
#endif
#ifdef DMG_RANKING
			case 'r':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading dmg rankings");
				CDmgRankingManager::instance().initDmgRankings();
				bSendP2P = true;
#endif
#ifdef HALLOWEEN_MINIGAME
			case 'h':
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "Reloading halloween rewards");
				CEventManager::instance().HalloweenMinigame_Initialize();
				bSendP2P = true;
#endif
		}
	}
	else if(test_server)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Reloading prototype tables,");
		db_clientdesc->DBPacket(network::TGDHeader::RELOAD_PROTO);
	}

	// if ch == NULL it's called from p2p/hp command
	if (ch && bSendP2P)
	{
		P2P_MANAGER::instance().Send(p2p_packet);
		ch->ChatPacket(CHAT_TYPE_INFO, "Reloading other cores / channels.");
	}
}

ACMD(do_cooltime)
{
	if (!ch)
		return;

	ch->DisableCooltime();
}

ACMD(do_level)
{
	if (!ch)
		return;

	char arg2[256];
	one_argument(argument, arg2, sizeof(arg2));

	if (!*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: level <level>");
		return;
	}

	int	level = 0;
	str_to_number(level, arg2);
	ch->ResetPoint(MINMAX(1, level, PLAYER_MAX_LEVEL_CONST));

	ch->ClearSkill();
	ch->ClearSubSkill();
}

ACMD(do_gwlist)
{
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(ch, "ÇöÀç ÀüÀïÁßÀÎ ±æµå ÀÔ´Ï´Ù"));
	CGuildManager::instance().ShowGuildWarList(ch);
}

ACMD(do_stop_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	int id1 = 0, id2 = 0;

	str_to_number(id1, arg1);
	str_to_number(id2, arg2);
	
	if (!id1 || !id2)
		return;

	if (id1 > id2)
	{
		std::swap(id1, id2);
	}

	if (ch)
		ch->ChatPacket(CHAT_TYPE_TALKING, "%d %d", id1, id2);
	CGuildManager::instance().RequestEndWar(id1, id2);
}

ACMD(do_cancel_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int id1 = 0, id2 = 0;
	str_to_number(id1, arg1);
	str_to_number(id2, arg2);

	if (id1 > id2)
		std::swap(id1, id2);

	CGuildManager::instance().RequestCancelWar(id1, id2);
}

ACMD(do_guild_state)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CGuild* pGuild = CGuildManager::instance().FindGuildByName(arg1);
	if (pGuild != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "GuildID: %d", pGuild->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "GuildMasterPID: %d", pGuild->GetMasterPID());
		ch->ChatPacket(CHAT_TYPE_INFO, "IsInWar: %d", pGuild->UnderAnyWar());
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s: Á¸ÀçÇÏÁö ¾Ê´Â ±æµå ÀÔ´Ï´Ù."), arg1);
	}
}

struct FuncWeaken
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncWeaken(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)	// 10¹ÌÅÍ ÀÌ»ó¿¡ ÀÖ´Â °ÍµéÀº purge ÇÏÁö ¾Ê´Â´Ù.
			return;

		if (pkChr->IsNPC())
			pkChr->PointChange(POINT_HP, (10 - pkChr->GetHP()));
	}
};

ACMD(do_weaken)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncWeaken func(ch);

	if (*arg1 && !strcmp(arg1, "all"))
		func.m_bAll = true;

	ch->GetSectree()->ForEachAround(func);
}

ACMD(do_getqf)
{
	if (!ch)
		return;

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER tch;

	if (!*arg1)
		tch = ch;
	else
	{
		tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
			return;
		}
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
		pPC->SendFlagList(ch);
}

ACMD(do_set_state)
{
	char arg1[256];
	char arg2[256];

	//argument = one_argument(argument, arg1, sizeof(arg1));
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	std::string questname = arg1;
	std::string statename = arg2;

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, statename);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it!= pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag failed");
	}
}

ACMD(do_setqf)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setqf <flagname> <value> [<character name>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg3)
		tch = CHARACTER_MANAGER::instance().FindPC(arg3);

	if (!tch)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		int value = 0;
		str_to_number(value, arg2);
		pPC->SetFlag(arg1, value);
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Quest flag set: %s %d", arg1, value);
	}
}

ACMD(do_delqf)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: delqf <flagname> [<character name>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg2)
		tch = CHARACTER_MANAGER::instance().FindPC(arg2);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		if (pPC->DeleteFlag(arg1))
			ch->ChatPacket(CHAT_TYPE_INFO, "Delete success.");
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Delete failed. Quest flag does not exist.");
	}
}

ACMD(do_forgetme)
{
	if (!ch)
		return;

	ch->ForgetMyAttacker();
}

ACMD(do_aggregate)
{
	if (!ch)
		return;

	ch->AggregateMonster();
}

ACMD(do_attract_ranger)
{
	if (!ch)
		return;

	ch->AttractRanger();
}

ACMD(do_pull_monster)
{
	if (!ch)
		return;

	ch->PullMonster();
}

ACMD(do_polymorph)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);
		bool bMaintainStat = false;
		if (*arg2)
		{
			int value = 0;
			str_to_number(value, arg2);
			bMaintainStat = (value>0);
		}

		ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_polymorph_item)
{
	if (!ch)
		return;

	if (!test_server)
		return;

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		LPITEM item = ITEM_MANAGER::instance().CreateItem(70104, 1, 0, true);
		if (item)
		{
			item->SetSocket(0, dwVnum);
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "Not enough inventory space.");
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d item not exist by that vnum.", 70103);
		}
		//ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_priv_empire)
{
	char arg1[256] = {0};
	char arg2[256] = {0};
	char arg3[256] = {0};
	char arg4[256] = {0};
	int empire = 0;
	int type = 0;
	int value = 0;
	int duration = 0;

	const char* line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		goto USAGE;

	if (!line)
		goto USAGE;

	two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg3 || !*arg4)
		goto USAGE;

	str_to_number(empire, arg1);
	str_to_number(type,	arg2);
	str_to_number(value,	arg3);
	value = MINMAX(0, value, 1000);
	str_to_number(duration, arg4);

	if (empire < 0 || 3 < empire)
		goto USAGE;

	if (type < 1 || 4 < type)
		goto USAGE;

	if (value < 0)
		goto USAGE;

	if (duration < 0)
		goto USAGE;

	// ½Ã°£ ´ÜÀ§·Î º¯°æ
	duration = duration * (60*60);

	sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, duration=%d) by command", 
			empire, type, value, duration);
	CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, duration);
	return;

USAGE:
	if (!ch)
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, "usage : priv_empire <empire> <type> <value> <duration>");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <empire>	0 - 3 (0==all)");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <type>	  1:item_drop, 2:gold_drop, 3:gold10_drop, 4:exp");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <value>	 percent");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <duration>  hour");
}

/**
 * @version 05/06/08	Bang2ni - ±æµå º¸³Ê½º Äù½ºÆ® ÁøÇà ¾ÈµÇ´Â ¹®Á¦ ¼öÁ¤.(½ºÅ©¸³Æ®°¡ ÀÛ¼º¾ÈµÊ.)
 * 					  quest/priv_guild.quest ·Î ºÎÅÍ ½ºÅ©¸³Æ® ÀÐ¾î¿À°Ô ¼öÁ¤µÊ
 */
ACMD(do_priv_guild)
{
	if (!ch)
		return;

	static const char msg[] = { '\0' };

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		CGuild * g = CGuildManager::instance().FindGuildByName(arg1);

		if (!g)
		{
			DWORD guild_id = 0;
			str_to_number(guild_id, arg1);
			g = CGuildManager::instance().FindGuild(guild_id);
		}

		if (!g)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±×·± ÀÌ¸§ ¶Ç´Â ¹øÈ£ÀÇ ±æµå°¡ ¾ø½À´Ï´Ù."));
		else
		{
			char buf[1024+1];
			snprintf(buf, sizeof(buf), msg, g->GetID());

			using namespace quest;
			PC * pc = CQuestManager::instance().GetPC(ch->GetPlayerID());
			QuestState qs = CQuestManager::instance().OpenState("ADMIN_QUEST", QUEST_FISH_REFINE_STATE_INDEX);
			luaL_loadbuffer(qs.co, buf, strlen(buf), "ADMIN_QUEST");
			pc->SetQuest("ADMIN_QUEST", qs);

			QuestState & rqs = *pc->GetRunningQuestState();

			if (!CQuestManager::instance().RunState(rqs))
			{
				CQuestManager::instance().CloseState(rqs);
				pc->EndRunning();
				return;
			}
		}
	}
}

ACMD(do_mount_test)
{
	if (!ch)
		return;

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		ch->MountVnum(vnum);
	}
}

ACMD(do_observer)
{
	if (!ch)
		return;

	ch->SetObserverMode(!ch->IsObserverMode());
}

ACMD(do_socket_item)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);
	
		int iSocketCount = 0;
		str_to_number(iSocketCount, arg2);
	
		if (!iSocketCount || iSocketCount >= ITEM_SOCKET_MAX_NUM)
			iSocketCount = 3;
	
		if (!dwVnum)
		{
			if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "#%d item not exist by that vnum.", dwVnum);
				return;
			}
		}

		LPITEM item = ch->AutoGiveItem(dwVnum);
	
		if (item)
		{
			for (int i = 0; i < iSocketCount; ++i)
				item->SetSocket(i, 1);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d cannot create item.", dwVnum);
		}
	}
}

ACMD(do_xmas)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int flag = 0;

	if (*arg1)
		str_to_number(flag, arg1);

	switch (subcmd)
	{
		case SCMD_XMAS_SNOW:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_snow", flag);
			break;

		case SCMD_XMAS_BOOM:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_boom", flag);
			break;

		case SCMD_XMAS_SANTA:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_santa", flag);
			break;
	}
}


// BLOCK_CHAT
ACMD(do_block_chat_list)
{
	if (!ch || ch->GetGMLevel() < GM_HIGH_WIZARD)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±×·± ¸í·É¾î´Â ¾ø½À´Ï´Ù"));
		return;
	}

	DBManager::instance().ReturnQuery(QID_BLOCK_CHAT_LIST, ch->GetPlayerID(), NULL, 
			"SELECT p.name, a.lDuration FROM affect as a, player as p WHERE a.bType = %d AND a.dwPID = p.id",
			AFFECT_BLOCK_CHAT);
}

ACMD(do_block_chat)
{
	// GMÀÌ ¾Æ´Ï°Å³ª block_chat_privilege°¡ ¾ø´Â »ç¶÷Àº ¸í·É¾î »ç¿ë ºÒ°¡
	if (ch && ch->GetGMLevel() < GM_HIGH_WIZARD)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±×·± ¸í·É¾î´Â ¾ø½À´Ï´Ù"));
		return;
	}

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: block_chat <name> <time> (0 to off)");

		return;
	}

	const char* name = arg1;
	long lBlockDuration = parse_time_str(argument);

	if (lBlockDuration < 0)
	{
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Àß¸øµÈ Çü½ÄÀÇ ½Ã°£ÀÔ´Ï´Ù. h, m, s¸¦ ºÙ¿©¼­ ÁöÁ¤ÇØ ÁÖ½Ê½Ã¿À.");
			ch->ChatPacket(CHAT_TYPE_INFO, "¿¹) 10s, 10m, 1m 30s");
		}
		return;
	}

	sys_log(0, "BLOCK CHAT %s %d", name, lBlockDuration);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			network::GGOutputPacket<network::GGBlockChatPacket> p;
			p->set_name(name);
			p->set_block_duration(lBlockDuration);
			P2P_MANAGER::instance().Send(p);
		}
		else
		{
			network::GDOutputPacket<network::GDBlockChatPacket> p;
			p->set_name(name);
			p->set_duration(lBlockDuration);
			db_clientdesc->DBPacket(p, ch ? ch->GetDesc()->GetHandle() : 0);
		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Chat block requested.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}
// END_OF_BLOCK_CHAT

// BUILD_BUILDING
ACMD(do_build)
{
	using namespace building;

	char arg1[256], arg2[256], arg3[256], arg4[256];
	const char * line = one_argument(argument, arg1, sizeof(arg1));
	BYTE GMLevel = ch->GetGMLevel();

	CLand * pkLand = CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	// NOTE: Á¶°Ç Ã¼Å©µéÀº Å¬¶óÀÌ¾ðÆ®¿Í ¼­¹ö°¡ ÇÔ²² ÇÏ±â ¶§¹®¿¡ ¹®Á¦°¡ ÀÖÀ» ¶§´Â
	//	   ¸Þ¼¼Áö¸¦ Àü¼ÛÇÏÁö ¾Ê°í ¿¡·¯¸¦ Ãâ·ÂÇÑ´Ù.
	if (!pkLand)
	{
		sys_err("%s trying to build on not buildable area.", ch->GetName());
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax: no command");
		return;
	}

	// °Ç¼³ ±ÇÇÑ Ã¼Å©
	if (GMLevel == GM_PLAYER)
	{
		// ÇÃ·¹ÀÌ¾î°¡ ÁýÀ» ÁöÀ» ¶§´Â ¶¥ÀÌ ³»²«Áö È®ÀÎÇØ¾ß ÇÑ´Ù.
		if ((!ch->GetGuild() || ch->GetGuild()->GetID() != pkLand->GetOwner()))
		{
			sys_err("%s trying to build on not owned land.", ch->GetName());
			return;
		}

		// ³»°¡ ±æ¸¶ÀÎ°¡?
		if (ch->GetGuild()->GetMasterPID() != ch->GetPlayerID())
		{
			sys_err("%s trying to build while not the guild master.", ch->GetName());
			return;
		}
	}

	switch (LOWER(*arg1))
	{
		case 'c':
			{
				// /build c vnum x y x_rot y_rot z_rot
				char arg5[256], arg6[256];
				line = one_argument(two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3)); // vnum x y
				one_argument(two_arguments(line, arg4, sizeof(arg4), arg5, sizeof(arg5)), arg6, sizeof(arg6)); // x_rot y_rot z_rot

				if (!*arg1 || !*arg2 || !*arg3 || !*arg4 || !*arg5 || !*arg6)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax");
					return;
				}

				DWORD dwVnum = 0;
				str_to_number(dwVnum,  arg1);

				using namespace building;

				auto t = CManager::instance().GetObjectProto(dwVnum);
				if (!t)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¸ÀçÇÏÁö ¾Ê´Â °Ç¹°ÀÔ´Ï´Ù."));
					return;
				}

				const DWORD BUILDING_MAX_PRICE = 100000000;

				if (t->group_vnum())
				{
					if (pkLand->FindObjectByGroup(t->group_vnum()))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°°ÀÌ ÁöÀ» ¼ö ¾ø´Â Á¾·ùÀÇ °Ç¹°ÀÌ Áö¾îÁ® ÀÖ½À´Ï´Ù."));
						return;
					}
				}

				// °Ç¹° Á¾¼Ó¼º Ã¼Å© (ÀÌ °Ç¹°ÀÌ Áö¾îÁ® ÀÖ¾î¾ßÇÔ)
				if (t->depend_on_group_vnum())
				{
					//		const TObjectProto * dependent = CManager::instance().GetObjectProto(dwVnum);
					//		if (dependent)
					{
						// Áö¾îÁ®ÀÖ´Â°¡?
						if (!pkLand->FindObjectByGroup(t->depend_on_group_vnum()))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°Ç¼³¿¡ ÇÊ¿äÇÑ °Ç¹°ÀÌ Áö¾îÁ® ÀÖÁö ¾Ê½À´Ï´Ù."));
							return;
						}
					}
				}

				if (test_server || GMLevel == GM_PLAYER)
				{
					// GMÀÌ ¾Æ´Ò°æ¿ì¸¸ (Å×¼·¿¡¼­´Â GMµµ ¼Ò¸ð)
					// °Ç¼³ ºñ¿ë Ã¼Å©
					if (t->price() > BUILDING_MAX_PRICE)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°Ç¹° ºñ¿ë Á¤º¸ ÀÌ»óÀ¸·Î °Ç¼³ ÀÛ¾÷¿¡ ½ÇÆÐÇß½À´Ï´Ù."));
						return;
					}

					if (ch->GetGold() < t->price())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°Ç¼³ ºñ¿ëÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
						return;
					}

					// ¾ÆÀÌÅÛ ÀÚÀç °³¼ö Ã¼Å©

					int i;
					for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
					{
						DWORD dwItemVnum = t->materials(i).item_vnum();
						DWORD dwItemCount = t->materials(i).count();

						if (dwItemVnum == 0)
							break;

						if ((int) dwItemCount > ch->CountSpecifyItem(dwItemVnum))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÚÀç°¡ ºÎÁ·ÇÏ¿© °Ç¼³ÇÒ ¼ö ¾ø½À´Ï´Ù."));
							return;
						}
					}
				}

				float x_rot = atof(arg4);
				float y_rot = atof(arg5);
				float z_rot = atof(arg6);
				// 20050811.myevan.°Ç¹° È¸Àü ±â´É ºÀÀÎ ÇØÁ¦
				/*
				   if (x_rot != 0.0f || y_rot != 0.0f || z_rot != 0.0f)
				   {
				   ch->ChatPacket(CHAT_TYPE_INFO, "°Ç¹° È¸Àü ±â´ÉÀº ¾ÆÁ÷ Á¦°øµÇÁö ¾Ê½À´Ï´Ù");
				   return;
				   }
				 */

				long map_x = 0;
				str_to_number(map_x, arg2);
				long map_y = 0;
				str_to_number(map_y, arg3);

				bool isSuccess = pkLand->RequestCreateObject(dwVnum, 
						ch->GetMapIndex(),
						map_x,
						map_y,
						x_rot,
						y_rot,
						z_rot, true);

				if (!isSuccess)
				{
					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°Ç¹°À» ÁöÀ» ¼ö ¾ø´Â À§Ä¡ÀÔ´Ï´Ù."));
					return;
				}

				if (test_server || GMLevel == GM_PLAYER)
					// °Ç¼³ Àç·á ¼Ò¸ðÇÏ±â (Å×¼·¿¡¼­´Â GMµµ ¼Ò¸ð)
				{
					// °Ç¼³ ºñ¿ë ¼Ò¸ð
					ch->PointChange(POINT_GOLD, -t->price());

					// ¾ÆÀÌÅÛ ÀÚÀç »ç¿ëÇÏ±â 
					{
						int i;
						for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
						{
							DWORD dwItemVnum = t->materials(i).item_vnum();
							DWORD dwItemCount = t->materials(i).count();

							if (dwItemVnum == 0)
								break;

							sys_log(0, "BUILD: material %d %u %u", i, dwItemVnum, dwItemCount);
							ch->RemoveSpecifyItem(dwItemVnum, dwItemCount);
						}
					}
				}
			}
			break;

		case 'd' :
			// build (d)elete ObjectID
			{
				one_argument(line, arg1, sizeof(arg1));

				if (!*arg1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax");
					return;
				}

				DWORD vid = 0;
				str_to_number(vid, arg1);
				pkLand->RequestDeleteObjectByVID(vid);
			}
			break;

			// BUILD_WALL	

			// build w n/e/w/s
		case 'w' :
			if (GMLevel > GM_PLAYER) 
			{
				int mapIndex = ch->GetMapIndex();

				one_argument(line, arg1, sizeof(arg1));
				
				sys_log(0, "guild.wall.build map[%d] direction[%s]", mapIndex, arg1);

				switch (arg1[0])
				{
					case 's':
						pkLand->RequestCreateWall(mapIndex,   0.0f);
						break;
					case 'n':
						pkLand->RequestCreateWall(mapIndex, 180.0f);
						break;
					case 'e':
						pkLand->RequestCreateWall(mapIndex,  90.0f);
						break;
					case 'w':
						pkLand->RequestCreateWall(mapIndex, 270.0f);
						break;
					default:
						ch->ChatPacket(CHAT_TYPE_INFO, "guild.wall.build unknown_direction[%s]", arg1);
						sys_err("guild.wall.build unknown_direction[%s]", arg1);
						break;
				}

			}
			break;

		case 'e':
			if (GMLevel > GM_PLAYER)
			{
				pkLand->RequestDeleteWall();
			}
			break;

		case 'W' :
			// ´ãÀå ¼¼¿ì±â
			// build (w)all ´ãÀå¹øÈ£ ´ãÀåÅ©±â ´ë¹®µ¿ ´ë¹®¼­ ´ë¹®³² ´ë¹®ºÏ

			if (GMLevel >  GM_PLAYER) 
			{
				int setID = 0, wallSize = 0;
				char arg5[256], arg6[256];
				line = two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2));
				line = two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));
				two_arguments(line, arg5, sizeof(arg5), arg6, sizeof(arg6));

				str_to_number(setID, arg1);
				str_to_number(wallSize, arg2);

				if (setID != 14105 && setID != 14115 && setID != 14125)
				{
					sys_log(0, "BUILD_WALL: wrong wall set id %d", setID);
					break;
				}
				else 
				{
					bool door_east = false;
					str_to_number(door_east, arg3);
					bool door_west = false;
					str_to_number(door_west, arg4);
					bool door_south = false;
					str_to_number(door_south, arg5);
					bool door_north = false;
					str_to_number(door_north, arg6);
					pkLand->RequestCreateWallBlocks(setID, ch->GetMapIndex(), wallSize, door_east, door_west, door_south, door_north);
				}
			}
			break;

		case 'E' :
			// ´ãÀå Áö¿ì±â
			// build (e)rase ´ãÀå¼ÂID
			if (GMLevel > GM_PLAYER) 
			{
				one_argument(line, arg1, sizeof(arg1));
				DWORD id = 0;
				str_to_number(id, arg1);
				pkLand->RequestDeleteWallBlocks(id);
			}
			break;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, "Invalid command %s", arg1);
			break;
	}
}
// END_OF_BUILD_BUILDING

ACMD(do_clear_quest)
{
	if (!ch)
		return;

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	pPC->ClearQuest(arg1);
}

ACMD(do_save_attribute_to_image) // command "/saveati" for alias
{
	char szFileName[256];
	char szMapIndex[256];

	two_arguments(argument, szMapIndex, sizeof(szMapIndex), szFileName, sizeof(szFileName));

	if (!*szMapIndex || !*szFileName)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /saveati <map_index> <filename>");
		return;
	}

	long lMapIndex = 0;
	str_to_number(lMapIndex, szMapIndex);

	if (SECTREE_MANAGER::instance().SaveAttributeToImage(lMapIndex, szFileName))
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Save done.");
	}
	else if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "Save failed.");
}

ACMD(do_affect_remove)
{
	if (!ch)
		return;

	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /affect_remove <player name>");
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /affect_remove <type> <point>");

		LPCHARACTER tch = ch;

		if (*arg1)
			if (!(tch = CHARACTER_MANAGER::instance().FindPC(arg1)))
				tch = ch;

		ch->ChatPacket(CHAT_TYPE_INFO, "-- Affect List of %s -------------------------------", tch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "Type Point Modif Duration Flag");

		const std::list<CAffect *> & cont = tch->GetAffectContainer();

		itertype(cont) it = cont.begin();

		while (it != cont.end())
		{
			CAffect * pkAff = *it++;

			ch->ChatPacket(CHAT_TYPE_INFO, "%4d %5d %5d %8d %u", 
					pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration, pkAff->dwFlag);
		}
		return;
	}

	bool removed = false;

	CAffect * af;

	DWORD	type = 0;
	str_to_number(type, arg1);
	BYTE	point = 0;
	str_to_number(point, arg2);
	while ((af = ch->FindAffect(type, point)))
	{
		ch->RemoveAffect(af);
		removed = true;
	}

	if (removed)
		ch->ChatPacket(CHAT_TYPE_INFO, "Affect successfully removed.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Not affected by that type and point.");
}

ACMD(do_change_attr)
{
	if (!ch)
		return;

	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->ChangeAttribute();
}

ACMD(do_add_attr)
{
	if (!ch)
		return;

	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddAttribute();
}

ACMD(do_add_socket)
{
	if (!ch)
		return;

	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddSocket();
}

ACMD(do_show_arena_list)
{
	CArenaManager::instance().SendArenaMapListTo(ch);
}

ACMD(do_end_all_duel)
{
	CArenaManager::instance().EndAllDuel();
}

ACMD(do_end_duel)
{
	char szName[256];

	one_argument(argument, szName, sizeof(szName));

	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindPC(szName);
	if (pChar == NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¸ÀçÇÏÁö ¾Ê´Â Ä³¸¯ÅÍ ÀÔ´Ï´Ù."));
		return;
	}

	if (CArenaManager::instance().EndDuel(pChar->GetPlayerID()) == false)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·Ã °­Á¦ Á¾·á ½ÇÆÐ"));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·Ã °­Á¦ Á¾·á ¼º°ø"));
	}
}

ACMD(do_duel)
{
	char szName1[256];
	char szName2[256];
	char szSet[256];
	char szMinute[256];
	int set = 0;
	int minute = 0;

	argument = two_arguments(argument, szName1, sizeof(szName1), szName2, sizeof(szName2));
	two_arguments(argument, szSet, sizeof(szSet), szMinute, sizeof(szMinute));

	str_to_number(set, szSet);
	
	if (set < 0) set = 1;
	if (set > 5) set = 5;

	if (!str_to_number(minute, szMinute))
	{
		minute = 5;
	}
	if (minute < 5)
		minute = 5;

	LPCHARACTER pChar1 = CHARACTER_MANAGER::instance().FindPC(szName1);
	LPCHARACTER pChar2 = CHARACTER_MANAGER::instance().FindPC(szName2);

	if (pChar1 != NULL && pChar2 != NULL)
	{
		pChar1->RemoveGoodAffect();
		pChar2->RemoveGoodAffect();

		pChar1->RemoveBadAffect();
		pChar2->RemoveBadAffect();

		LPPARTY pParty = pChar1->GetParty();
		if (pParty != NULL)
		{
			if (pParty->IsInDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this while someone in %s's party is inside a dungeon."), pChar1->GetName());
				return;
			}
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar1->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pChar1, "<ÆÄÆ¼> ÆÄÆ¼¿¡¼­ ³ª°¡¼Ì½À´Ï´Ù."));
				pParty->Quit(pChar1->GetPlayerID());
			}
		}
	
		pParty = pChar2->GetParty();
		if (pParty != NULL)
		{
			if (pParty->IsInDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this while someone in %s's party is inside a dungeon."), pChar2->GetName());
				return;
			}
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar2->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pChar2, "<ÆÄÆ¼> ÆÄÆ¼¿¡¼­ ³ª°¡¼Ì½À´Ï´Ù."));
				pParty->Quit(pChar2->GetPlayerID());
			}
		}
		
		if (CArenaManager::instance().StartDuel(pChar1, pChar2, set, minute) == true)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀÌ ¼º°øÀûÀ¸·Î ½ÃÀÛ µÇ¾ú½À´Ï´Ù."));
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·Ã ½ÃÀÛ¿¡ ¹®Á¦°¡ ÀÖ½À´Ï´Ù."));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀÚ°¡ ¾ø½À´Ï´Ù."));
	}
}

ACMD(do_stat_plus_amount)
{
	char szPoint[256];

	one_argument(argument, szPoint, sizeof(szPoint));

	if (*szPoint == '\0')
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	int nRemainPoint = ch->GetPoint(POINT_STAT);

	if (nRemainPoint <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³²Àº ½ºÅÈ Æ÷ÀÎÆ®°¡ ¾ø½À´Ï´Ù."));
		return;
	}

	int nPoint = 0;
	str_to_number(nPoint, szPoint);

	if (nRemainPoint < nPoint)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³²Àº ½ºÅÈ Æ÷ÀÎÆ®°¡ Àû½À´Ï´Ù."));
		return;
	}

	if (nPoint < 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°ªÀ» Àß¸ø ÀÔ·ÂÇÏ¿´½À´Ï´Ù."));
		return;
	}
	
	switch (subcmd)
	{
		case POINT_HT : // Ã¼·Â
			if (nPoint + ch->GetPoint(POINT_HT) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_HT);
			}
			break;

		case POINT_IQ : // Áö´É
			if (nPoint + ch->GetPoint(POINT_IQ) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_IQ);
			}
			break;
			
		case POINT_ST : // ±Ù·Â
			if (nPoint + ch->GetPoint(POINT_ST) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_ST);
			}
			break;
			
		case POINT_DX : // ¹ÎÃ¸
			if (nPoint + ch->GetPoint(POINT_DX) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_DX);
			}
			break;

		default :
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸í·É¾îÀÇ ¼­ºê Ä¿¸Çµå°¡ Àß¸ø µÇ¾ú½À´Ï´Ù."));
			return;
			break;
	}

	if (nPoint != 0)
	{
		ch->SetRealPoint(subcmd, ch->GetRealPoint(subcmd) + nPoint);
		ch->SetPoint(subcmd, ch->GetPoint(subcmd) + nPoint);
		ch->ComputePoints();
		ch->PointChange(subcmd, 0);

		ch->PointChange(POINT_STAT, -nPoint);
		ch->ComputePoints();
	}
}

struct tTwoPID
{
	int pid1;
	int pid2;
};

ACMD(do_break_marriage)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	tTwoPID pids = { 0, 0 };

	str_to_number(pids.pid1, arg1);
	str_to_number(pids.pid2, arg2);
	
	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÇÃ·¹ÀÌ¾î %d ¿Í ÇÃ·¹ÀÌ¾î  %d¸¦ ÆÄÈ¥½ÃÅµ´Ï´Ù.."), pids.pid1, pids.pid2);

	network::GDOutputPacket<network::GDMarriageBreakPacket> pack;
	pack->set_pid1(pids.pid1);
	pack->set_pid2(pids.pid2);
	db_clientdesc->DBPacket(pack);
}

ACMD(do_effect)
{
	if (!ch)
		return;

	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	int	effect_type = 0;
	str_to_number(effect_type, arg1);
	ch->EffectPacket(effect_type);
}

ACMD(do_reset_subskill)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));
	
	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: reset_subskill <name>");
		return;
	}
	
	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	
	if (tch == NULL)
		return;

	tch->ClearSubSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "Subskill of [%s] was reset", tch->GetName());
}

ACMD(do_flush)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));
	
	DWORD pid = 0;
	
	if (!*arg1)
	{
		if (ch)
			pid = ch->GetPlayerID();
	}
	else
	{
		if (!str_is_number(arg1))
		{
			if (LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1))
			{
				pid = tch->GetPlayerID();
			}
			else if (CCI* pkCCI = P2P_MANAGER::instance().Find(arg1))
			{
				pid = pkCCI->dwPID;
			}
			else
			{
				if (strlen(arg1) > CHARACTER_NAME_MAX_LEN)
					return;

				char szEscapedName[CHARACTER_NAME_MAX_LEN * 2 + 1];
				DBManager::instance().EscapeString(szEscapedName, sizeof(szEscapedName), arg1, strlen(arg1));

				std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT id FROM player WHERE name LIKE '%s'", szEscapedName));
				if (pMsg->Get()->uiNumRows == 0)
				{
					if (ch)
						ch->ChatPacket(CHAT_TYPE_INFO, "Player not found by name %s.", arg1);
					return;
				}

				str_to_number(pid, mysql_fetch_row(pMsg->Get()->pSQLResult)[0]);
			}
		}
		else
			str_to_number(pid, arg1);
	}
	
	
	if (pid <= 0)
	{
		if(ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "not online or wrong usage, usage : /flush <pid or name>");
		return;
	}

	if (LPCHARACTER tch = CHARACTER_MANAGER::instance().FindByPID(pid))
	{
		tch->Save();
		CHARACTER_MANAGER::instance().FlushDelayedSave(tch);
	}
	else if (CCI* pkCCI = P2P_MANAGER::instance().FindByPID(pid))
	{
		network::GGOutputPacket<network::GGFlushPlayerPacket> pack;
		pack->set_pid(pkCCI->dwPID);
		pkCCI->pkDesc->Packet(pack);
		return;
	}

	network::GDOutputPacket<network::GDFlushCachePacket> pack;
	pack->set_pid(pid);
	db_clientdesc->DBPacket(pack);

	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "Flush sent for pid %u.", pid);
}

ACMD(do_save)
{
	if (!ch)
		return;
	
	ch->SetSkipSave(false);
	ch->SaveReal();
	ch->tchat("Save..");
}

ACMD(do_eclipse)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (strtol(arg1, NULL, 10) == 1)
	{
		quest::CQuestManager::instance().RequestSetEventFlag("eclipse", 1);
	}
	else
	{
		quest::CQuestManager::instance().RequestSetEventFlag("eclipse", 0);
	}
}

struct FMobCounter
{
	int nCount;

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (pChar->IsMonster() == true || pChar->IsStone())
			{
				nCount++;
			}
		}
	}
};

ACMD(do_get_mob_count)
{
	if (!ch)
		return;

	LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());

	if (pSectree == NULL)
		return;

	FMobCounter f;
	f.nCount = 0;

	pSectree->for_each(f);

	ch->ChatPacket(CHAT_TYPE_INFO, "MapIndex: %d MobCount %d", ch->GetMapIndex(), f.nCount);
}

ACMD(do_clear_land)
{
	const building::CLand* pLand = building::CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	if( NULL == pLand )
	{
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Guild Land(%d) Cleared", pLand->GetID());

	building::CManager::instance().ClearLand(pLand->GetID());
}

ACMD(do_special_item)
{
	ITEM_MANAGER::instance().ConvSpecialDropItemFile();
}

ACMD(do_set_stat)
{
	char szName [256];
	char szChangeAmount[256];

	two_arguments (argument, szName, sizeof (szName), szChangeAmount, sizeof(szChangeAmount));

	if (*szName == '\0' || *szChangeAmount == '\0')
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "Invalid argument.");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(szName);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(szName);

		if (pkCCI)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "Cannot find player(%s). %s is not in your game server.", szName, szName);
			return;
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "Cannot find player(%s). Perhaps %s doesn't login or exist.", szName, szName);
			return;
		}
	}
	else
	{
		if (tch->IsPolymorphed())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
			return;
		}

		if (subcmd != POINT_HT && subcmd != POINT_IQ && subcmd != POINT_ST && subcmd != POINT_DX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸í·É¾îÀÇ ¼­ºê Ä¿¸Çµå°¡ Àß¸ø µÇ¾ú½À´Ï´Ù."));
			return;
		}
		int nRemainPoint = tch->GetPoint(POINT_STAT);
		int nCurPoint = tch->GetRealPoint(subcmd);
		int nChangeAmount = 0;
		str_to_number(nChangeAmount, szChangeAmount);
		int nPoint = nCurPoint + nChangeAmount;
		
		int n;
		switch (subcmd)
		{
		case POINT_HT:
			if (nPoint < JobInitialPoints[tch->GetJob()].ht)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Cannot set stat under initial stat."));
				return;
			}
			n = 0;
			break;
		case POINT_IQ:
			if (nPoint < JobInitialPoints[tch->GetJob()].iq)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Cannot set stat under initial stat."));
				return;
			}
			n = 1;
			break;
		case POINT_ST:
			if (nPoint < JobInitialPoints[tch->GetJob()].st)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Cannot set stat under initial stat."));
				return;
			}
			n = 2;
			break;
		case POINT_DX:
			if (nPoint < JobInitialPoints[tch->GetJob()].dx)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Cannot set stat under initial stat."));
				return;
			}
			n = 3;
			break;
		}

		if (nPoint > 90)
		{
			nChangeAmount -= nPoint - 90;
			nPoint = 90;
		}

		if (nRemainPoint < nChangeAmount)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³²Àº ½ºÅÈ Æ÷ÀÎÆ®°¡ Àû½À´Ï´Ù."));
			return;
		}

		tch->SetRealPoint(subcmd, nPoint);
		tch->SetPoint(subcmd, tch->GetPoint(subcmd) + nChangeAmount);
		tch->ComputePoints();
		tch->PointChange(subcmd, 0);

		tch->PointChange(POINT_STAT, -nChangeAmount);
		tch->ComputePoints();

		const char* stat_name[4] = {"con", "int", "str", "dex"};

		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s change %d to %d", szName, stat_name[n], nCurPoint, nPoint);
	}
}

ACMD(do_get_item_id_list)
{
	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}

ACMD(do_set_socket)
{
	char arg1 [256];
	char arg2 [256];
	char arg3 [256];

	one_argument (two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof(arg2)), arg3, sizeof (arg3));
	
	int item_id, socket_num, value;
	if (!str_to_number (item_id, arg1) || !str_to_number (socket_num, arg2) || !str_to_number (value, arg3))
		return;
	
	LPITEM item = ITEM_MANAGER::instance().Find (item_id);
	if (item)
		item->SetSocket (socket_num, value);
}

ACMD (do_can_dead)
{
	if (subcmd)
		ch->SetArmada();
	else
		ch->ResetArmada();
}

ACMD (do_full_set)
{
	extern void do_all_skill_master(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	do_all_skill_master(ch, NULL, 0, 0);
	extern void do_item_full_set(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	do_item_full_set(ch, NULL, 0, 0);
	extern void do_attr_full_set(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	do_attr_full_set(ch, NULL, 0, 0);

}

ACMD(do_horse_grade)
{
	char arg1[256] = { 0 };
	char arg2[256] = { 0 };
	LPCHARACTER victim;
	int	level = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "usage : /horse_grade <name> <level>");
		return;
	}

	victim = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (NULL == victim)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¸ÀçÇÏÁö ¾Ê´Â Ä³¸¯ÅÍ ÀÔ´Ï´Ù."));
		return;
	}

	str_to_number(level, arg2);
	level = MINMAX(0, level, CHARACTER::HORSE_MAX_GRADE);

	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "horse grade set (%s: %d)", victim->GetName(), level);

	victim->SetHorseGrade(level);
	victim->SetHorseElapsedTime(0);
	victim->ComputePoints();
	victim->SkillLevelPacket();
}

ACMD (do_all_skill_master)
{
	ch->SetHorseGrade(CHARACTER::HORSE_MAX_GRADE);
	ch->SetHorseElapsedTime(0);
	for (int i = 0; i < SKILL_MAX_NUM; i++)
	{
		if (true == ch->CanUseSkill(i))
		{
			ch->SetSkillLevel(i, SKILL_MAX_LEVEL);
		}
		else
		{
			switch(i)
			{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				ch->SetSkillLevel(i, SKILL_MAX_LEVEL);
				break;
			}
		}
	}
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD (do_item_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;
	for (int i = 0; i < 6; i++)
	{
		item = ch->GetWear(i);
		if (item != NULL)
			ch->UnequipItem(item);
	}
	item = ch->GetWear(WEAR_SHIELD);
	if (item != NULL)
		ch->UnequipItem(item);

	switch (job)
	{
	case JOB_SURA:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11699);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12529 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_WARRIOR:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11299);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(3159 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12249 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_SHAMAN:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11899);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(7159 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12669 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_ASSASSIN:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11499);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(1139 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12389 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	}
}

ACMD (do_attr_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;

	switch (job)
	{
	case JOB_WARRIOR:
	case JOB_ASSASSIN:
	case JOB_SURA:
	case JOB_SHAMAN:
		{
			// ¹«»ç ¸ö»§ ¼ÂÆÃ.
			// ÀÌ°Í¸¸ ³ª¿Í ÀÖ¾î¼­ ÀÓ½Ã·Î ¸ðµç Á÷±º ´Ù ÀÌ·± ¼Ó¼º µû¸§.
			item = ch->GetWear(WEAR_HEAD);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_ATT_SPEED, 8);
				item->SetForceAttribute( 1, APPLY_HP_REGEN, 30);
				item->SetForceAttribute( 2, APPLY_SP_REGEN, 30);
				item->SetForceAttribute( 3, APPLY_DODGE, 15);
				item->SetForceAttribute( 4, APPLY_STEAL_SP, 10);
			}

			item = ch->GetWear(WEAR_WEAPON);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_CAST_SPEED, 20);
				item->SetForceAttribute( 1, APPLY_CRITICAL_PCT, 10);
				item->SetForceAttribute( 2, APPLY_PENETRATE_PCT, 10);
				item->SetForceAttribute( 3, APPLY_ATTBONUS_DEVIL, 20);
				item->SetForceAttribute( 4, APPLY_STR, 12);
			}

			item = ch->GetWear(WEAR_SHIELD);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_CON, 12);
				item->SetForceAttribute( 1, APPLY_BLOCK, 15);
				item->SetForceAttribute( 2, APPLY_REFLECT_MELEE, 10);
				item->SetForceAttribute( 3, APPLY_IMMUNE_STUN, 1);
				item->SetForceAttribute( 4, APPLY_IMMUNE_SLOW, 1);
			}

			item = ch->GetWear(WEAR_BODY);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_CAST_SPEED, 20);
				item->SetForceAttribute( 2, APPLY_STEAL_HP, 10);
				item->SetForceAttribute( 3, APPLY_REFLECT_MELEE, 10);
				item->SetForceAttribute( 4, APPLY_ATT_GRADE_BONUS, 50);
			}

			item = ch->GetWear(WEAR_FOOTS);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_MAX_SP, 80);
				item->SetForceAttribute( 2, APPLY_MOV_SPEED, 8);
				item->SetForceAttribute( 3, APPLY_ATT_SPEED, 8);
				item->SetForceAttribute( 4, APPLY_CRITICAL_PCT, 10);
			}

			item = ch->GetWear(WEAR_WRIST);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_MAX_SP, 80);
				item->SetForceAttribute( 2, APPLY_PENETRATE_PCT, 10);
				item->SetForceAttribute( 3, APPLY_STEAL_HP, 10);
				item->SetForceAttribute( 4, APPLY_MANA_BURN_PCT, 10);
			}
			item = ch->GetWear(WEAR_NECK);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_MAX_SP, 80);
				item->SetForceAttribute( 2, APPLY_CRITICAL_PCT, 10);
				item->SetForceAttribute( 3, APPLY_PENETRATE_PCT, 10);
				item->SetForceAttribute( 4, APPLY_STEAL_SP, 10);
			}
			item = ch->GetWear(WEAR_EAR);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MOV_SPEED, 20);
				item->SetForceAttribute( 1, APPLY_MANA_BURN_PCT, 10);
				item->SetForceAttribute( 2, APPLY_POISON_REDUCE, 5);
				item->SetForceAttribute( 3, APPLY_ATTBONUS_DEVIL, 20);
				item->SetForceAttribute( 4, APPLY_ATTBONUS_UNDEAD, 20);
			}
		}
		break;
	}
}

ACMD (do_use_item)
{
	char arg1 [256];

	one_argument (argument, arg1, sizeof (arg1));

	int cell;
	str_to_number(cell, arg1);
	
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		ch->UseItem(TItemPos (INVENTORY, cell));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "¾ÆÀÌÅÛÀÌ ¾ø¾î¼­ Âø¿ëÇÒ ¼ö ¾ø¾î.");
	}
}

ACMD (do_clear_affect)
{
	if (!ch)
		return;

	ch->ClearAffect(true);
}

ACMD (do_remove_rights)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "usage: remove_rights <player name>");
		return;
	}

	if (ch && !strcasecmp(ch->GetName(), arg1))
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "You cannot remove your own rights.");
		return;
	}

	if (GM::get_level(arg1) == GM_PLAYER)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "No player %s with GM-Rights has been found.", arg1);
		return;
	}

	GM::remove(arg1);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (tch)
		tch->SetGMLevel();

	network::GGOutputPacket<network::GGUpdateRightsPacket> packet;
	packet->set_name(arg1);
	packet->set_gm_level(GM_PLAYER);
	P2P_MANAGER::instance().Send(packet);

	char szPlayerName[CHARACTER_NAME_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szPlayerName, sizeof(szPlayerName), arg1, strlen(arg1));
	DBManager::instance().DirectQuery("DELETE FROM common.`gmlist` WHERE (`mName`='%s')", szPlayerName);

	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "The rights of %s has been removed (new status: GM_PLAYER).", arg1);
}

ACMD (do_give_rights)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "usage: give_rights <right name[\"LOW_WIZARD\",\"WIZARD\",\"HIGH_WIZARD\",\"GOD\",\"IMPLEMENTOR\"]/id[1-5]) <player name>");
		return;
	}

	BYTE bAuthority;
	if (str_is_number(arg1))
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Please write the right name in words");
		return;
		/*		str_to_number(bAuthority, arg1);
		if (bAuthority <= GM_PLAYER || bAuthority > GM_IMPLEMENTOR)
		{
		ch->ChatPacket(CHAT_TYPE_INFO, "Unkown right-ID %u [expected number between 1 and 5]", bAuthority);
		return;
		}*/
	}
	else
	{
		if (!strcasecmp(arg1, "LOW_WIZARD"))
			bAuthority = GM_LOW_WIZARD;
		else if (!strcasecmp(arg1, "WIZARD"))
			bAuthority = GM_WIZARD;
		else if (!strcasecmp(arg1, "HIGH_WIZARD"))
			bAuthority = GM_HIGH_WIZARD;
		else if (!strcasecmp(arg1, "GOD"))
			bAuthority = GM_GOD;
		else if (!strcasecmp(arg1, "IMPLEMENTOR"))
			bAuthority = GM_IMPLEMENTOR;
		else
		{
			if (ch)
				ch->ChatPacket(CHAT_TYPE_INFO, "Unkown right-name %s [expected \"LOW_WIZARD\", \"WIZARD\", \"HIGH_WIZARD\", \"GOD\" or \"IMPLEMENTOR\"]", arg1);
			return;
		}
	}

	if (ch && !strcasecmp(ch->GetName(), arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You cannot give rights to yourself.");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg2);
	if (!tch)
	{
		CCI* p2pCCI = P2P_MANAGER::instance().Find(arg2);
		if (!p2pCCI)
		{
			char szPlayerName[CHARACTER_NAME_MAX_LEN * 2 + 1];
			DBManager::instance().EscapeString(szPlayerName, sizeof(szPlayerName), arg2, strlen(arg2));
			std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT name FROM player WHERE name LIKE '%s'", szPlayerName));
			if (pMsg->Get()->uiNumRows == 0)
			{
				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, "The player %s does not exist.", arg2);
				return;
			}

			strlcpy(arg2, *mysql_fetch_row(pMsg->Get()->pSQLResult), sizeof(arg2));
		}
		else
			strlcpy(arg2, p2pCCI->szName, sizeof(arg2));
	}
	else
		strlcpy(arg2, tch->GetName(), sizeof(arg2));

	network::TAdminInfo info;
	info.set_authority(bAuthority);
	info.set_name(arg2);
	info.set_account("[ALL]");
	GM::insert(info);

	if (tch)
		tch->SetGMLevel();

	network::GGOutputPacket<network::GGUpdateRightsPacket> packet;
	packet->set_name(arg2);
	packet->set_gm_level(bAuthority);
	P2P_MANAGER::instance().Send(packet);

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT login FROM player join account on account.id = account_id WHERE name = '%s'", info.name().c_str()));
	if (pMsg->Get()->uiNumRows == 0)
		return;
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

	DBManager::instance().DirectQuery("DELETE FROM common.gmlist WHERE mAccount='%s'", row[0]);
	DBManager::instance().DirectQuery("REPLACE INTO common.gmlist (`mAccount`, `mName`, `mAuthority`) VALUES ('%s', '%s', '%s')", row[0], info.name().c_str(), arg1);

	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, "The rights of %s has been changed to %s.", arg2, arg1);
}

ACMD (do_get_distance)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (!*arg1 || !str_is_number(arg1) || !*arg2 || !str_is_number(arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: get_distance <local_x> <local_y>");
		return;
	}

	long lX, lY;
	str_to_number(lX, arg1);
	str_to_number(lY, arg2);

	PIXEL_POSITION basePos;
	if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(ch->GetMapIndex(), basePos))
		return;

	int iDistance = DISTANCE_APPROX(ch->GetX() - (lX + basePos.x), ch->GetY() - (lY + basePos.y));
	ch->ChatPacket(CHAT_TYPE_INFO, "Distance to (%ld, %ld) is %d.", lX, lY, iDistance);
}

#ifdef __UNIMPLEMENT__
ACMD (do_get_average_damage)
{
	char arg1[256], arg2[256], arg3[256], arg4[256];
	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1 || !str_is_number(arg1) || !*arg2 || !str_is_number(arg2) || !*arg3 || !str_is_number(arg3) || !*arg4 || !str_is_number(arg4))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: get_average_damage <job> <pvm/pvp [0/1]> <meele/ranged [0/1]> <level>");
		return;
	}

	BYTE bJob;
	str_to_number(bJob, arg1);
	bJob = MIN(bJob, JOB_MAX_NUM - 1);

	bool bIsPVP;
	str_to_number(bIsPVP, arg2);

	bool bIsRanged;
	str_to_number(bIsRanged, arg3);

	BYTE bLevel;
	str_to_number(bLevel, arg4);

	WORD wDamage = CHARACTER_MANAGER::instance().GetAverageDamage(bJob, bIsPVP, bIsRanged ? DAMAGE_TYPE_NORMAL_RANGE : DAMAGE_TYPE_NORMAL, bLevel);
	ch->ChatPacket(CHAT_TYPE_INFO, "The average damage for job[%d] pvp[%d] ranged[%d] level[%d] is %u.", bJob, bIsPVP, bIsRanged, bLevel, wDamage);
}
#endif

// new commands
#ifdef __ANTI_BRUTEFORCE__
ACMD(do_flush_bruteforce_data)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		if (m_mapWrongLoginByHWID.find(arg1) == m_mapWrongLoginByHWID.end())
		{
			if (ch)
				ch->ChatPacket(CHAT_TYPE_INFO, "no bruteforce data by hwid [%s]", arg1);
			sys_log(0, "flush_bruteforce_data: NO DATA FOUND by HWID [%s]", arg1);
		}

		m_mapWrongLoginByHWID.erase(arg1);
		sys_log(0, "flush_bruteforce_data: FLUSH by HWID [%s]", arg1);
		
		if (m_mapWrongLoginByIP.find(arg1) == m_mapWrongLoginByIP.end())
		{
			if (ch)
				ch->ChatPacket(CHAT_TYPE_INFO, "no bruteforce data by IP [%s]", arg1);
			sys_log(0, "flush_bruteforce_data: NO DATA FOUND by IP [%s]", arg1);
		}

		m_mapWrongLoginByIP.erase(arg1);
		sys_log(0, "flush_bruteforce_data: FLUSH by IP [%s]", arg1);
	}
	else
	{
		sys_log(0, "flush_bruteforce_data: FLUSHED %d HWIDs %d IP entries", m_mapWrongLoginByHWID.size(), m_mapWrongLoginByIP.size());
		m_mapWrongLoginByHWID.clear();
		m_mapWrongLoginByIP.clear();
	}
}
#endif


ACMD(do_give_apply_point)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	BYTE bApplyType;
	int iApplyValue;
	if (!*arg1 || !str_is_number(arg1) || !*arg2 || !str_is_number(arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: give_apply_point <apply_type> <apply_value>");
		return;
	}

	str_to_number(bApplyType, arg1);
	str_to_number(iApplyValue, arg2);

	if (bApplyType >= MAX_APPLY_NUM)
		return;

	BYTE bPointType = aApplyInfo[bApplyType].bPointType;
	int iOldValue = ch->GetPoint(bPointType);

	ch->ApplyPoint(bApplyType, iApplyValue);

	ch->ChatPacket(CHAT_TYPE_INFO, "Changed Apply[%d] Point[%d] from %d -> %d", bApplyType, bPointType, iOldValue, ch->GetPoint(bPointType));
}

ACMD(do_get_apply_point)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE bApplyType;
	if (!*arg1 || !str_is_number(arg1))
		return;

	str_to_number(bApplyType, arg1);

	if (bApplyType >= MAX_APPLY_NUM)
		return;

	ch->ChatPacket(CHAT_TYPE_INFO, "Apply[%d] Point[%d] = %f", bApplyType, aApplyInfo[bApplyType].bPointType, ch->GetPointF(aApplyInfo[bApplyType].bPointType));
}

ACMD(do_get_point)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE bPointType;
	if (!*arg1 || !str_is_number(arg1))
		return;

	str_to_number(bPointType, arg1);

	ch->ChatPacket(CHAT_TYPE_INFO, "Point[%d] = %f", bPointType, ch->GetPointF(bPointType));
}

ACMD(do_set_is_show_teamler)
{
	if (!ch)
		return;

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	if (*arg1 == '0')
		ch->SetIsShowTeamler(false);
	else
		ch->SetIsShowTeamler(true);
}

ACMD(do_force_item_delete)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD dwItemID;
	str_to_number(dwItemID, arg1);

	if (LPITEM pkItem = ITEM_MANAGER::instance().Find(dwItemID))
	{
		pkItem->SetSkipSave(false);
		ITEM_MANAGER::Instance().RemoveItem(pkItem, "FORCE_ITEM_DELETE");
	}
	else
	{
		network::GGOutputPacket<network::GGForceItemDeletePacket> pack;
		pack->set_item_id(dwItemID);
		P2P_MANAGER::instance().Send(pack);

		network::GDOutputPacket<network::GDForceItemDeletePacket> pdb;
		pdb->set_id(dwItemID);
		db_clientdesc->DBPacket(pdb);
	}
}

#ifdef __HOMEPAGE_COMMAND__
extern unsigned int g_iHomepageCommandSleepTime;
ACMD(do_homepage_process_sleep)
{
	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This command is only available for the homepage.");
		return;
	}

	if (g_iHomepageCommandSleepTime)
	{
		sys_err("invalid call of homepage_process_sleep (already sleeping: %u (get_dword_time() = %u)", g_iHomepageCommandSleepTime, get_dword_time());
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int iSleepTime;
	str_to_number(iSleepTime, arg1);

	if (iSleepTime <= 0)
	{
		sys_err("invalid call of homepage_process_sleep (invalid time %d)", iSleepTime);
		return;
	}

	g_iHomepageCommandSleepTime = get_dword_time() + iSleepTime * 1000;
}
#endif

#ifdef __INVENTORY_SORT__
#ifdef SORT_AND_STACK_ITEMS
struct SItem
{
	LPITEM m_item;
	BYTE m_window;

	SItem(LPITEM item, BYTE window)
	{
		m_item = item;
		m_window = window;
	}
};

inline bool IsEquipableItem(BYTE type)
{
	switch (type)
	{
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_HAIR:
		case ITEM_TOTEM:
		case ITEM_COSTUME:
			return true;
	}

	return false;
}

bool sort_inventory_func(SItem &a, SItem &b) {
	if (IsEquipableItem(a.m_item->GetType()) && IsEquipableItem(b.m_item->GetType()))
		return a.m_item->GetType() > b.m_item->GetType();
	else if (IsEquipableItem(a.m_item->GetType()) && !IsEquipableItem(b.m_item->GetType()))
		return false;
	else if (!IsEquipableItem(a.m_item->GetType()) && IsEquipableItem(b.m_item->GetType()))
		return true;
	else
		return a.m_item->GetVnum() < b.m_item->GetVnum();
}
#else
bool sort_inventory_func(CItem* a, CItem* b) {
	return a->GetVnum() < b->GetVnum();
}
#endif

ACMD(do_sort_inventory)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_inv_sort"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This function doesn't work temporarily.");
		return;
	}

	if (thecore_pulse() - ch->GetSortingPulse() < PASSES_PER_SEC(3))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait 3 seconds."));
		return;
	}

	ch->SetSortingPulse();

#ifdef SORT_AND_STACK_ITEMS // not neccessary I guess
	if (ch->IsDead() || ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShop() || ch->GetMyShop() || ch->IsCubeOpen() || ch->IsAcceWindowOpen() || !ch->CanHandleItem())
		return;
#else
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->GetMyShop() || ch->IsCubeOpen() || ch->IsAcceWindowOpen())
		return;
#endif
	
	std::vector<WORD> oldCells;
	int totalSize = 0;
#ifdef SORT_AND_STACK_ITEMS
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
		return;

	BYTE window = 0;
	str_to_number(window, arg1);

	switch (window)
	{
		case SAFEBOX:
		case MALL:
#ifdef __COSTUME_INVENTORY__
		case COSTUME_INVENTORY:
#endif
#ifdef __DRAGONSOUL__
		case DRAGON_SOUL_INVENTORY:
#endif
			return;
	}

	WORD wSlotStart = 0, wSlotEnd = 0;
	ch->GetInventorySlotRange(window, wSlotStart, wSlotEnd);

	if (!wSlotEnd)
		return;

	std::vector<SItem> collectItems;

	for (auto i = wSlotStart; i < wSlotEnd; ++i) {
#else
	std::vector<CItem*> collectItems;
	for (auto i = 0; i < ch->GetInventoryMaxNum(); ++i) {
#endif
		auto item = ch->GetInventoryItem(i);
		if (item) {
			totalSize += item->GetSize();
			oldCells.push_back(item->GetCell());
#ifdef SORT_AND_STACK_ITEMS
			collectItems.push_back(SItem(item, item->GetWindow()));
#else
			collectItems.push_back(item);
#endif
		}
	}
#ifdef SORT_AND_STACK_ITEMS
	if (totalSize - 3 >= wSlotEnd) {
#else
	if (totalSize - 3 >= ch->GetInventoryMaxNum()) {
#endif
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "INVENTORY_FULL_CANNOT_SORT"));
		return;
	}

	DWORD adwQuickslotItemID[QUICKSLOT_MAX_NUM];
	memset(adwQuickslotItemID, 0, sizeof(adwQuickslotItemID));

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		TQuickslot* p;
		if (ch->GetQuickslot(i, &p) && p->type() == QUICKSLOT_TYPE_ITEM)
		{
			LPITEM pkItem = ch->GetInventoryItem(p->pos());
			if (pkItem)
				adwQuickslotItemID[i] = pkItem->GetID();
		}
	}

	for (auto& item : collectItems) {
#ifdef SORT_AND_STACK_ITEMS
		item.m_item->RemoveFromCharacter();
#else
		item->RemoveFromCharacter();
#endif
	}

	std::sort(collectItems.begin(), collectItems.end(), sort_inventory_func);

	for (auto& sortedItem : collectItems) {
#ifdef SORT_AND_STACK_ITEMS
		int cell = ch->GetEmptyInventoryNew(sortedItem.m_window, sortedItem.m_item->GetSize());

		if (cell == -1)
			ch->AutoGiveItem(sortedItem.m_item, true);
		else
			sortedItem.m_item->AddToCharacter(ch, TItemPos(INVENTORY, cell));
#else
		int cell = ch->GetEmptyInventory(sortedItem->GetSize());

		if (cell == -1)
			ch->AutoGiveItem(sortedItem, true);
		else
			sortedItem->AddToCharacter(ch, TItemPos(INVENTORY, cell));
#endif
	}

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		if (adwQuickslotItemID[i])
		{
			if (LPITEM pkItem = ch->FindItemByID(adwQuickslotItemID[i]))
			{
				TQuickslot p;
				p.set_type(QUICKSLOT_TYPE_ITEM);
				p.set_pos(pkItem->GetCell());
				ch->SetQuickslot(i, p);
			}
			else
			{
				ch->DelQuickslot(i);
			}
		}
	}
}
#endif

ACMD(do_offline_messages_pull)
{
	if (ch && !test_server)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This command is only available for the homepage.");
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		sys_err("invalid call of offline_messages_pull");
		return;
	}

	int pid;
	str_to_number(pid, arg1);

	if (ch = CHARACTER_MANAGER::instance().FindByPID(pid))
		DBManager::instance().ReturnQuery(QID_PULL_OFFLINE_MESSAGES, pid, NULL, "SELECT sender, message, is_gm, language FROM offline_messages WHERE pid=%d ORDER BY date ASC", pid);
	else if (CCI* pkCCI = P2P_MANAGER::Instance().FindByPID(pid))
	{
		network::GGOutputPacket<network::GGPullOfflineMessagesPacket> packet;
		packet->set_pid(pid);

		pkCCI->pkDesc->Packet(packet);
	}
}

#ifdef __ITEM_REFUND__
ACMD(do_request_item_refund)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		sys_err("invalid call of request_item_refund");
		return;
	}

	int pid;
	str_to_number(pid, arg1);

	if (ch = CHARACTER_MANAGER::instance().FindByPID(pid))
	{
		network::GDOutputPacket<network::GDLoadItemRefundPacket> pdb;
		pdb->set_pid(pid);
		db_clientdesc->DBPacket(pdb, ch->GetDesc()->GetHandle());
	}
}
#endif

ACMD(do_set_inventory_max_num)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: set_inventory_max_num <max_num[%d-%d]>", INVENTORY_MAX_NUM_MIN, INVENTORY_MAX_NUM);
		return;
	}

	WORD wMaxNum;
	str_to_number(wMaxNum, arg1);

	ch->SetInventoryMaxNum(wMaxNum, INVENTORY_SIZE_TYPE_NORMAL);
	ch->ChatPacket(CHAT_TYPE_INFO, "Set inventory max num to %d.", ch->GetInventoryMaxNum());
}

ACMD(do_set_uppitem_inv_max_num)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: set_uppitem_inv_max_num <max_num[%d-%d]>", UPPITEM_INV_MIN_NUM, UPPITEM_INV_MAX_NUM);
		return;
	}

	WORD wMaxNum;
	str_to_number(wMaxNum, arg1);

	ch->SetInventoryMaxNum(wMaxNum, INVENTORY_SIZE_TYPE_UPPITEM);
	ch->ChatPacket(CHAT_TYPE_INFO, "Set uppitem inventory max num to %d.", ch->GetUppitemInventoryMaxNum());
}

ACMD(do_set_skillbook_inv_max_num)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: set_skillbook_inv_max_num <max_num[%d-%d]>", SKILLBOOK_INV_MIN_NUM, SKILLBOOK_INV_MAX_NUM);
		return;
	}

	WORD wMaxNum;
	str_to_number(wMaxNum, arg1);

	ch->SetInventoryMaxNum(wMaxNum, INVENTORY_SIZE_TYPE_SKILLBOOK);
	ch->ChatPacket(CHAT_TYPE_INFO, "Set SKILLBOOK inventory max num to %d.", ch->GetSkillbookInventoryMaxNum());
}

ACMD(do_set_stone_inv_max_num)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: set_stone_inv_max_num <max_num[%d-%d]>", STONE_INV_MIN_NUM, STONE_INV_MAX_NUM);
		return;
	}

	WORD wMaxNum;
	str_to_number(wMaxNum, arg1);

	ch->SetInventoryMaxNum(wMaxNum, INVENTORY_SIZE_TYPE_STONE);
	ch->ChatPacket(CHAT_TYPE_INFO, "Set STONE inventory max num to %d.", ch->GetStoneInventoryMaxNum());
}

ACMD(do_set_enchant_inv_max_num)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: set_enchant_inv_max_num <max_num[%d-%d]>", ENCHANT_INV_MIN_NUM, ENCHANT_INV_MAX_NUM);
		return;
	}

	WORD wMaxNum;
	str_to_number(wMaxNum, arg1);

	ch->SetInventoryMaxNum(wMaxNum, INVENTORY_SIZE_TYPE_ENCHANT);
	ch->ChatPacket(CHAT_TYPE_INFO, "Set ENCHANT inventory max num to %d.", ch->GetEnchantInventoryMaxNum());
}

#ifdef __PRESTIGE__
ACMD(do_prestige_level)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1 || !str_is_number(arg1))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: prestige_level <level>");
		return;
	}

	BYTE bLevel;
	str_to_number(bLevel, arg1);

	ch->SetPrestigeLevel(bLevel);
}
#endif

#ifdef __DRAGONSOUL__
ACMD (do_dragon_soul)
{
	char arg1[512];
	const char* rest = one_argument (argument, arg1, sizeof(arg1));
	switch (arg1[0])
	{
	case 'a':
		{
			one_argument (rest, arg1, sizeof(arg1));
			int deck_idx;
			if (str_to_number(deck_idx, arg1) == false)
			{
				return;
			}
			ch->DragonSoul_ActivateDeck(deck_idx);
		}
		break;
	case 'd':
		{
			ch->DragonSoul_DeactivateAll();
		}
		break;
	}
}

ACMD (do_ds_list)
{
	for (int i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; i++)
	{
		TItemPos cell(DRAGON_SOUL_INVENTORY, i);
		
		LPITEM item = ch->GetItem(cell);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}

#endif

ACMD(do_horse_rage)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !str_is_number(arg1) || !arg2 || !str_is_number(arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: horse_rage <level> <pct (0-%d)>", HORSE_MAX_RAGE);
		return;
	}

	BYTE bRageLevel;
	WORD wRagePct;
	str_to_number(bRageLevel, arg1);
	str_to_number(wRagePct, arg2);

	ch->SetHorseRageLevel(MIN(bRageLevel, HORSE_RAGE_MAX_LEVEL));
	ch->SetHorseRagePct(MIN(wRagePct, HORSE_MAX_RAGE));
}

ACMD(do_test_pvp_char)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!test_server)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "only available on test server");
		return;
	}

	char szQuery[2048];
	{
		const char* c_pszCommandName = "default";
		char szCommandNameBuf[256 * 2 + 1] = "";
		if (*arg1)
		{
			DBManager::instance().EscapeString(szCommandNameBuf, sizeof(szCommandNameBuf), arg1, strlen(arg1));
			c_pszCommandName = szCommandNameBuf;
		}

		int len = snprintf(szQuery, sizeof(szQuery), "SELECT vnum, count");
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
			len += snprintf(szQuery + len, sizeof(szQuery) - len, ", attrtype%d+0, attrvalue%d", i, i);
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			len += snprintf(szQuery + len, sizeof(szQuery) - len, ", socket%d", i);
		len += snprintf(szQuery + len, sizeof(szQuery) - len, ", extra_upgrade_level, jewelry_addon, jewelry_subaddon1, jewelry_subaddon2, jewelry_subaddon2_index, jewelry_addon_time_left "
			"FROM test_pvp_item "
			"WHERE (job = 'ALL' OR job-1 = %u) AND (skillgroup = 0 OR skillgroup = %u) AND (sex = 'ALL' OR sex-1 = %u) AND (`command` LIKE '%s') "
			"ORDER BY job ASC, skillgroup ASC, vnum ASC, attrtype4 ASC", ch->GetJob(), ch->GetSkillGroup(), GET_SEX(ch), c_pszCommandName);
	}

	for (int i = 0; i < SKILL_MAX_NUM; i++)
	{
		if (true == ch->CanUseSkill(i))
		{
			CSkillProto* pkSk = CSkillManager::instance().Get(i);
			ch->SetSkillLevel(i, pkSk && !pkSk->dwType ? pkSk->bMaxLevel : SKILL_MAX_LEVEL);
		}
		else
		{
			switch (i)
			{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				ch->SetSkillLevel(i, SKILL_MAX_LEVEL);
				break;
			}
		}
	}
	ch->ComputePoints();
	ch->SkillLevelPacket();

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("%s", szQuery));
	MYSQL_RES * pRes = pMsg->Get()->pSQLResult;

	sys_log(0, "test_pvp_char: %s", szQuery);

	if (pRes)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Fetched %d items.", mysql_num_rows(pRes));

		while (MYSQL_ROW row = mysql_fetch_row(pRes))
		{
			int col = 0;

			network::TItemData kCurItem;

			kCurItem.set_vnum(std::atoi(row[col++]));
			kCurItem.set_count(std::atoi(row[col++]));
			for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
			{
				auto attr = kCurItem.add_attributes();
				attr->set_type(std::atoi(row[col++]));
				attr->set_value(std::atoi(row[col++]));
			}
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				kCurItem.add_sockets(std::atoi(row[col++]));
			}

			LPITEM pNewItem = ITEM_MANAGER::instance().CreateItem(&kCurItem);
			if (!pNewItem)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "cannot create item %u", kCurItem.vnum());
				continue;
			}

			ch->AutoGiveItem(pNewItem);
		}
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "ERROR IN SQL");
}

#ifdef __FAKE_BUFF__
ACMD(do_setskillfake)
{
	if (!ch)
		return;

	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskillfake <name> <lev>");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pk = CSkillManager::instance().Get(vnum);
	}

	else
		pk = CSkillManager::instance().Get(arg1);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	if (ch->GetFakeBuffSkillIdx(pk->dwVnum) == FAKE_BUFF_SKILL_COUNT)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "skill is no fake buff skill");
		return;
	}

	unsigned char level = 0;
	str_to_number(level, arg2);

	ch->SetFakeBuffSkillLevel(pk->dwVnum, level);
}
#endif

#ifdef __QUEST_PENETRATE_TEST__
ACMD(do_quest_penetrate_test)
{
	if (!test_server)
		return;
	
	int iMaxPenetrate = 1000;
	
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1 && isdigit(*arg1))
		str_to_number(iMaxPenetrate, arg1);
	
	if (iMaxPenetrate == 1) 
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(19, 1, 0, true);
		quest::CQuestManager::instance().CatchFish(ch->GetPlayerID(), item);
		return;
	}
	else if(iMaxPenetrate == 2)
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(19, 1, 0, true);
		quest::CQuestManager::instance().MineOre(ch->GetPlayerID(), item);
		return;
	}
	
	else if(iMaxPenetrate == 3)
	{
		quest::CQuestManager::instance().DungeonComplete(ch->GetPlayerID(), 7);
		return;
	}
	
	if (iMaxPenetrate == 1999997)
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(19, 1, 0, true);
		item->SetCount(-1);
		ch->tchat("%d", item->GetCount());
		return;
	}
	
	// Levelup Trigger no problem
/* 	for (int i=0; i < iMaxPenetrate; i++)
	{
		// sys_log(0, "%s:%d [%i]", __FILE__, __LINE__, i);
		
		if (ch->GetLevel() >= 105)
			ch->SetLevel(1);
		
		ch->PointChange(POINT_LEVEL, 1, 0, 0);
		// quest::CQuestManager::instance().LevelUp(ch->GetPlayerID());...
	} */
	
	// Kill Trigger					
 	for (int i=0; i < iMaxPenetrate; i++)
	{
		// sys_log(0, "%s:%d [%i]", __FILE__, __LINE__, i);
		DWORD dwVnum = random_number(1, 100000);
		
		LPCHARACTER pSpawnMonster = CHARACTER_MANAGER::instance().SpawnMobRange( dwVnum,
							ch->GetMapIndex(),
							ch->GetX() - random_number(200, 750),
							ch->GetY() - random_number(200, 750),
							ch->GetX() + random_number(200, 750),
							ch->GetY() + random_number(200, 750),
							true,
							0,
							0 );
							
		if (!pSpawnMonster)
			continue;
							
		ch->SetQuestNPCID(pSpawnMonster->GetVID());
		quest::CQuestManager::instance().Kill(ch->GetPlayerID(), dwVnum);
		
		LPCHARACTER ch = quest::CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER npc = quest::CQuestManager::instance().GetCurrentNPCCharacterPtr();

		ch->SetQuestNPCID(0);
		if (pSpawnMonster)
		{
			pSpawnMonster->Dead();
		}
	}	
}
#endif

#ifdef __EVENT_MANAGER__
ACMD(do_event_open)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	DWORD dwIndex = 0;

	if (!*arg1 || !str_is_number(arg1) || !str_to_number(dwIndex, arg1) || dwIndex == EVENT_NONE || dwIndex >= EVENT_MAX_NUM)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: event_open <index[%d-%d]>", EVENT_NONE + 1, EVENT_MAX_NUM - 1);
		return;
	}

	CEventManager::instance().OpenEventRegistration(dwIndex);
}

ACMD(do_event_close)
{
	CEventManager::instance().CloseEventRegistration();
}
#endif

struct cmd_map_packet_func
{
	const char* m_str;
	int m_level;
	int m_lang;

	cmd_map_packet_func(const char* str, int level, int lang) : m_str(str), m_level(level), m_lang(lang)
	{
	}

	void operator() (LPDESC d)
	{
		if (d->GetCharacter() == NULL) return;
		if (d->GetCharacter()->GetLevel() < m_level) return;

		if (m_lang == -1 || m_lang == d->GetCharacter()->GetLanguageID())
			d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "%s", m_str);
	}
};

ACMD(do_send_cmd)
{
	char arg1[CHAT_MAX_LEN+1], arg2[256], arg3[256];
	int iLangID = -1, iMinLevel = 0;
	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1)
		return;
	
	if (*arg2 && str_is_number(arg2))
		str_to_number(iLangID, arg2);
	
	if (*arg3 && str_is_number(arg3))
		str_to_number(iMinLevel, arg3);
	
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), cmd_map_packet_func(arg1, iMinLevel, iLangID));
}

#ifdef __EVENT_MANAGER__
ACMD(do_event_announcement)
{
	char arg1[ 256 ], arg2[ 256 ];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	DWORD dwType = 0;
	DWORD dwSeconds = 0;

	time_t tmStamp = 0;

	if(!*arg1 || !str_is_number(arg1) || !str_to_number(dwType, arg1) || !*arg2 || !str_is_number(arg2) || !str_to_number(dwSeconds, arg2) )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: event_announcement <event id> <seconds>");
		return;
	}

	tmStamp = time(0) + dwSeconds;
	CEventManager::instance().OpenEventAnnouncement(dwType, tmStamp);
}
#endif

struct sys_announce_map_packet_func
{
	const char* m_str;

	sys_announce_map_packet_func(const char* str) : m_str(str)
	{
	}

	void operator() (LPDESC d)
	{
		if (d->GetCharacter() == NULL) return;
		d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "SYSANNOUNCE %s", m_str);
		d->GetCharacter()->tchat("m_str %s", m_str);
	}
};

ACMD(do_system_annoucement)
{
	char arg1[CHAT_MAX_LEN+1];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;
	
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), sys_announce_map_packet_func(arg1));
}

ACMD(do_enable_packet_logging)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int iEnabled = 0;

	if (!*arg1 || !*arg2 || !str_is_number(arg2) || !str_to_number(iEnabled, arg2))
	{
		if(ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "usage: enable_logging <name> <0/1>");
		return;
	}

	LPCHARACTER pChr = CHARACTER_MANAGER::Instance().FindPC(arg1);

	if (!pChr)
	{
		if(ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Player %s not found.", arg1);
		return;
	}
	
	pChr->SetQuestFlag("security.logging", iEnabled);
}

ACMD(do_test)
{
	if (!ch)
		return;
	
	ch->ItemDropPenalty(NULL);
}

#ifdef __PET_ADVANCED__
ACMD(do_pet_level)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: pet_level <level>");
		return;
	}

	WORD level = static_cast<WORD>(atoi(arg1));

	if (!ch->GetPetAdvanced())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Summon your advanced pet before using pet_level command.");
		return;
	}

	ch->GetPetAdvanced()->SetLevel(level);
}

ACMD(do_pet_skill_level)
{
	char arg1[256], arg2[256], arg3[256];
	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage: pet_skill_level <slot_index[0-%d]> <skill_vnum> <skill_level>", PET_SKILL_MAX_NUM - 1);
		return;
	}

	BYTE index = static_cast<BYTE>(atoi(arg1));
	DWORD skill_vnum = static_cast<DWORD>(atoi(arg2));
	WORD skill_level = static_cast<WORD>(atoi(arg3));

	if (index >= PET_SKILL_MAX_NUM)
		return;

	if (!ch->GetPetAdvanced())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Summon your advanced pet before using pet_skill_level command.");
		return;
	}

	if (ch->GetPetAdvanced()->SetSkillLevel(index, skill_vnum, skill_level))
		ch->ChatPacket(CHAT_TYPE_INFO, "Skill on slot %d has now skill %u with level %d.", index, skill_vnum, skill_level);
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Failed to set skill level.");
}
#endif
