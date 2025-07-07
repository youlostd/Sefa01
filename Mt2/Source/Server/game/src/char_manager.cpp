﻿#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "party.h"
#include "regen.h"
#include "p2p.h"
#include "dungeon.h"
#include "db.h"
#include "config.h"
#include "xmas_event.h"
#include "questmanager.h"
#include "questlua.h"
#include "log.h"
#include "skill.h"

#ifndef __GNUC__
#include <boost/bind.hpp>
#endif

extern void DelBossHuntMob(DWORD vid, long mapIndex);
extern LPEVENT bossHuntEvent;
extern LPEVENT bossHuntSpawnEvent;

CHARACTER_MANAGER::CHARACTER_MANAGER() :
	m_iVIDCount(0),
	m_pkChrSelectedStone(NULL),
	m_bUsePendingDestroy(false)
{
	RegisterRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM);
	RegisterRaceNum(xmas::MOB_XMAS_TREE_VNUM);

	m_iMobItemRate = 100;
	m_iMobDamageRate = 100;
	m_iMobGoldAmountRate = 100;
	m_iMobGoldDropRate = 100;
	m_iMobExpRate = 100;

	m_iMobItemRatePremium = 100;
	m_iMobGoldAmountRatePremium = 100;
	m_iMobGoldDropRatePremium = 100;
	m_iMobExpRatePremium = 100;
	
	m_iUserDamageRate = 100;
	m_iUserDamageRatePremium = 100;
}

CHARACTER_MANAGER::~CHARACTER_MANAGER()
{
	Destroy();
}

void CHARACTER_MANAGER::Destroy()
{
	if (bossHuntEvent)
		event_cancel(&bossHuntEvent);

	if (bossHuntSpawnEvent)
		event_cancel(&bossHuntSpawnEvent);

	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.begin();
	while (it != m_map_pkChrByVID.end()) {
		LPCHARACTER ch = it->second;
		M2_DESTROY_CHARACTER(ch); // m_map_pkChrByVID is changed here
		it = m_map_pkChrByVID.begin();
	}
}

void CHARACTER_MANAGER::GracefulShutdown()
{
	NAME_MAP::iterator it = m_map_pkPCChr.begin();

	while (it != m_map_pkPCChr.end())
	{
		it->second->Disconnect("GracefulShutdown");
		it = m_map_pkPCChr.begin();
	}

	// SaveAverageDamage();
}

DWORD CHARACTER_MANAGER::AllocVID()
{
	++m_iVIDCount;
	return m_iVIDCount;
}

void CHARACTER_MANAGER::RemoveFromCharMap(LPCHARACTER ptr)
{
	auto it = m_map_pkChrByVID.find(ptr->GetVID());
	if (it != m_map_pkChrByVID.end())
		m_map_pkChrByVID.erase(it);
}

LPCHARACTER CHARACTER_MANAGER::CreateCharacter(const char * name, DWORD dwPID)
{
	DWORD dwVID = AllocVID();

	LPCHARACTER ch = M2_NEW CHARACTER;

	ch->Create(name, dwVID, dwPID ? true : false);

	m_map_pkChrByVID.insert(std::make_pair(dwVID, ch));
	ch->SetHaveToRemoveFromMgr(true);

	if (dwPID)
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];
		str_lower(name, szName, sizeof(szName));

		m_map_pkPCChr.insert(NAME_MAP::value_type(szName, ch));
		m_map_pkChrByPID.insert(std::make_pair(dwPID, ch));
	}

	return (ch);
}

void CHARACTER_MANAGER::DestroyCharacter(LPCHARACTER ch)
{
	if (!ch)
		return;

	// <Factor> Check whether it has been already deleted or not.
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(ch->GetVID());
	if (it == m_map_pkChrByVID.end()) {
		sys_err("[CHARACTER_MANAGER::DestroyCharacter] <Factor> %d not found", (long)(ch->GetVID()));
		return; // prevent duplicated destrunction
	}

	// ´øÀü¿¡ ¼Ò¼ÓµÈ ¸ó½ºÅÍ´Â ´øÀü¿¡¼­µµ »èÁ¦ÇÏµµ·Ï.
#ifdef __PET_SYSTEM__
	if (ch->IsNPC() && !ch->IsPet() && ch->GetRider() == NULL)
#else
	if (ch->IsNPC() && ch->GetRider() == NULL)
#endif
	{
		if (ch->GetDungeon())
		{
			ch->GetDungeon()->DeadCharacter(ch);
		}
	}

	if (m_bUsePendingDestroy)
	{
		m_set_pkChrPendingDestroy.insert(ch);
		return;
	}

	m_map_pkChrByVID.erase(it);

	if (true == ch->IsPC())
	{
		char szName[CHARACTER_NAME_MAX_LEN + 1];

		str_lower(ch->GetName(), szName, sizeof(szName));

		NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

		if (m_map_pkPCChr.end() != it)
			m_map_pkPCChr.erase(it);
	}

	if (0 != ch->GetPlayerID())
	{
		itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(ch->GetPlayerID());

		if (m_map_pkChrByPID.end() != it)
		{
			m_map_pkChrByPID.erase(it);
		}
	}

	UnregisterRaceNumMap(ch);

	RemoveFromStateList(ch);

	if ((ch->GetRaceNum() == BOSSHUNT_BOSS_VNUM
#ifdef BOSSHUNT_EVENT_UPDATE
		|| ch->GetRaceNum() == BOSSHUNT_BOSS_VNUM2
#endif
		) && quest::CQuestManager::instance().GetEventFlag("enable_bosshunt_event"))
		DelBossHuntMob(ch->GetVID(), ch->GetMapIndex());

	ch->SetHaveToRemoveFromMgr(false);
	M2_DELETE(ch);
}

LPCHARACTER CHARACTER_MANAGER::Find(DWORD dwVID)
{
	itertype(m_map_pkChrByVID) it = m_map_pkChrByVID.find(dwVID);

	if (m_map_pkChrByVID.end() == it)
		return NULL;
	
	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwVID != (DWORD)found->GetVID()) {
		sys_err("[CHARACTER_MANAGER::Find] <Factor> %u != %u", dwVID, (DWORD)found->GetVID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::Find(const VID & vid)
{
	LPCHARACTER tch = Find((DWORD) vid);

	if (!tch || tch->GetVID() != vid)
		return NULL;

	return tch;
}

LPCHARACTER CHARACTER_MANAGER::FindByPID(DWORD dwPID)
{
	itertype(m_map_pkChrByPID) it = m_map_pkChrByPID.find(dwPID);

	if (m_map_pkChrByPID.end() == it)
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && dwPID != found->GetPlayerID()) {
		sys_err("[CHARACTER_MANAGER::FindByPID] <Factor> %u != %u", dwPID, found->GetPlayerID());
		return NULL;
	}
	return found;
}

LPCHARACTER CHARACTER_MANAGER::FindPC(const char * name)
{
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	str_lower(name, szName, sizeof(szName));
	NAME_MAP::iterator it = m_map_pkPCChr.find(szName);

	if (it == m_map_pkPCChr.end())
		return NULL;

	// <Factor> Added sanity check
	LPCHARACTER found = it->second;
	if (found != NULL && strncasecmp(szName, found->GetName(), CHARACTER_NAME_MAX_LEN) != 0) {
		sys_err("[CHARACTER_MANAGER::FindPC] <Factor> %s != %s", name, found->GetName());
		return NULL;
	}
	return found;
}

void CHARACTER_MANAGER::InitializeHorseUpgrade(const ::google::protobuf::RepeatedPtrField<network::THorseUpgradeProto>& table)
{
	m_map_HorseUpgrade.clear();

	for (auto& t : table)
	{
		auto kIndex = std::make_pair(t.upgrade_type(), t.level());
		m_map_HorseUpgrade[kIndex] = t;

		sys_log(0, "InitializeHorseUpgrade: [type %d level %d] -> levelLimit %d refineID %u",
			t.upgrade_type(), t.level(), t.level_limit(), t.refine_id());
	}
}

void CHARACTER_MANAGER::InitializeHorseBonus(const ::google::protobuf::RepeatedPtrField<network::THorseBonusProto>& table)
{
	for (auto& t : table)
	{
		if (t.level() == 0 || t.level() > HORSE_MAX_BONUS_LEVEL)
		{
			sys_err("invalid horse bonus level %d (max level %d)", t.level(), HORSE_MAX_BONUS_LEVEL);
			continue;
		}

		m_akHorseBonus[t.level() - 1] = t;
	}
}

const network::THorseUpgradeProto* CHARACTER_MANAGER::GetHorseUpgrade(BYTE bType, BYTE bLevel)
{
	auto it = m_map_HorseUpgrade.find(std::make_pair(bType, bLevel));
	if (it == m_map_HorseUpgrade.end())
		return NULL;

	return &it->second;
}

const network::THorseBonusProto* CHARACTER_MANAGER::GetHorseBonus(BYTE bLevel)
{
	if (bLevel >= HORSE_MAX_BONUS_LEVEL)
		return NULL;

	return &m_akHorseBonus[bLevel];
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex)
{
	// ¿Ö±¸ ½ºÆùÇÒÁö¸»Áö¸¦ °áÁ¤ÇÒ ¼ö ÀÖ°ÔÇÔ
	{
		if (dwVnum == 5001 && !quest::CQuestManager::instance().GetEventFlag("japan_regen"))
		{
			sys_log(1, "WAEGU[5001] regen disabled.");
			return NULL;
		}
	}

	// ÇØÅÂ¸¦ ½ºÆùÇÒÁö ¸»Áö¸¦ °áÁ¤ÇÒ ¼ö ÀÖ°Ô ÇÔ
	{
		if (dwVnum == 5002 && !quest::CQuestManager::instance().GetEventFlag("newyear_mob"))
		{
			sys_log(1, "HAETAE (new-year-mob) [5002] regen disabled.");
			return NULL;
		}
	}

	// ±¤º¹Àý ÀÌº¥Æ® 
	{
		if (dwVnum == 5004 && !quest::CQuestManager::instance().GetEventFlag("independence_day"))
		{
			sys_log(1, "INDEPENDECE DAY [5004] regen disabled.");
			return NULL;
		}
	}

	const CMob * pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
	{
		sys_err("no mob data for vnum %u", dwVnum);
		return NULL;
	}

	if (!map_allow_find(lMapIndex))
	{
		sys_err("not allowed map %u", lMapIndex);
		return NULL;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
	if (pkSectreeMap == NULL) {
		return NULL;
	}

	int i;
	long x, y;
	for (i=0; i<2000; i++)
	{
		x = random_number(1, (pkSectreeMap->m_setting.iWidth / 100)  - 1) * 100 + pkSectreeMap->m_setting.iBaseX;
		y = random_number(1, (pkSectreeMap->m_setting.iHeight / 100) - 1) * 100 + pkSectreeMap->m_setting.iBaseY;
		//LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);
		LPSECTREE tree = pkSectreeMap->Find(x, y);

		if (!tree)
			continue;

		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT))
			continue;

		if (IS_SET(dwAttr, ATTR_BANPK))
			continue;

		break;
	}

	if (i == 2000)
	{
		sys_err("cannot find valid location");
		return NULL;
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.locale_name(LANGUAGE_DEFAULT).c_str());

	if (!ch)
	{
		sys_log(0, "SpawnMobRandomPosition: cannot create new character");
		return NULL;
	}

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.type() == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(random_number(0, 360));

	if (!ch->Show(lMapIndex, x, y, 0, false))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_err(0, "SpawnMobRandomPosition: cannot show monster");
		return NULL;
	}

	char buf[512+1];
	long local_x = x - pkSectreeMap->m_setting.iBaseX;
	long local_y = y - pkSectreeMap->m_setting.iBaseY;
	snprintf(buf, sizeof(buf), "spawn %s[%d] random position at %ld %ld %ld %ld (time: %d)", ch->GetName(), dwVnum, x, y, local_x, local_y, get_global_time());
	
	if (test_server)
		SendNotice(buf);

	sys_log(0, buf);
	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion, int iRot, bool bShow)
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);
	if (!pkMob)
	{
		sys_err("SpawnMob: no mob data for vnum %u (map %u)", dwVnum, lMapIndex);
		return NULL;
	}

	bool bCheckTree = true;
	if ((pkMob->m_table.type() == CHAR_TYPE_NPC || pkMob->m_table.type() == CHAR_TYPE_WARP || pkMob->m_table.type() == CHAR_TYPE_GOTO || pkMob->m_table.type() == CHAR_TYPE_MOUNT || pkMob->m_table.type() == CHAR_TYPE_PET) && !mining::IsVeinOfOre(dwVnum))
		bCheckTree = false;
#if defined(__FAKE_PC__) || defined(__FAKE_BUFF__)
	else if (dwVnum >= MAIN_RACE_MOB_WARRIOR_M && dwVnum <= MAIN_RACE_MOB_MAX_NUM)
		bCheckTree = false;
#endif
#ifdef ENABLE_HYDRA_DUNGEON
	else if (lMapIndex == HYDRA_RUN_MAP_INDEX || lMapIndex >= HYDRA_RUN_MAP_INDEX * 10000 && lMapIndex < (HYDRA_RUN_MAP_INDEX + 1) * 10000)
		bCheckTree = false;
#endif

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!tree)
	{
		sys_log(0, "no sectree for spawn at %d %d mobvnum %d mapindex %d", x, y, dwVnum, lMapIndex);
		return NULL;
	}

	if (bCheckTree)
	{

		DWORD dwAttr = tree->GetAttribute(x, y);

		bool is_set = false;

		if ( mining::IsVeinOfOre (dwVnum) ) is_set = IS_SET(dwAttr, ATTR_BLOCK);
		else is_set = IS_SET(dwAttr, ATTR_BLOCK | ATTR_OBJECT);

		if ( is_set )
		{
			// SPAWN_BLOCK_LOG
			static bool s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");
			static DWORD s_nextTime=get_global_time()+10000;

			DWORD curTime=get_global_time();

			if (curTime>s_nextTime)
			{
				s_nextTime=curTime;
				s_isLog=quest::CQuestManager::instance().GetEventFlag("spawn_block_log");

			}

			if (s_isLog)
				sys_log(0, "SpawnMob: BLOCKED position for spawn[%s] map:%u vnum:%u at %d,%d (attr %u)", pkMob->m_table.locale_name(LANGUAGE_DEFAULT).c_str(), lMapIndex, dwVnum, x, y, dwAttr);
			// END_OF_SPAWN_BLOCK_LOG
			return NULL;
		}

		if (IS_SET(dwAttr, ATTR_BANPK))
		{
			sys_log(0, "SpawnMob: BAN_PK position for mob spawn %s %u at %d %d", pkMob->m_table.name().c_str(), dwVnum, x, y);
			return NULL;
		}
	}

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "SpawnMob: cannot create monster at non-exist sectree %d x %d (map %d)", x, y, lMapIndex);
		return NULL;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pkMob->m_table.locale_name(LANGUAGE_DEFAULT).c_str());

	if (!ch)
	{
		sys_log(0, "SpawnMob: cannot create new character");
		return NULL;
	}

	if (iRot == -1)
		iRot = random_number(0, 360);

	ch->SetProto(pkMob);

	// if mob is npc with no empire assigned, assign to empire of map
	if (pkMob->m_table.type() == CHAR_TYPE_NPC)
		if (ch->GetEmpire() == 0)
			ch->SetEmpire(SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex));

	ch->SetRotation(iRot);

	if (bShow && !ch->Show(lMapIndex, x, y, z, bSpawnMotion))
	{
		M2_DESTROY_CHARACTER(ch);
		sys_log(0, "SpawnMob: cannot show monster on map %u x %u y %u", lMapIndex, x, y);
		return NULL;
	}

	if (!bShow)
	{
		ch->SetMapIndex(lMapIndex);
		ch->SetXYZ(x, y, z);
	}

	return (ch);
}

LPCHARACTER CHARACTER_MANAGER::SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException, bool bSpawnMotion, bool bAggressive )
{
	const CMob * pkMob = CMobManager::instance().Get(dwVnum);

	if (!pkMob)
		return NULL;

	if (pkMob->m_table.type() == CHAR_TYPE_STONE)	// µ¹Àº ¹«Á¶°Ç SPAWN ¸ð¼ÇÀÌ ÀÖ´Ù.
		bSpawnMotion = true;

	int i = 16;

	while (i--)
	{
		int x = random_number(sx, ex);
		int y = random_number(sy, ey);
		/*
		   if (bIsException)
		   if (is_regen_exception(x, y))
		   continue;
		 */
		LPCHARACTER ch = SpawnMob(dwVnum, lMapIndex, x, y, 0, bSpawnMotion);

		if (ch)
		{
			sys_log(1, "MOB_SPAWN: %s(%d) %dx%d", ch->GetName(), (DWORD)ch->GetVID(), ch->GetX(), ch->GetY());
			if ( bAggressive )
				ch->SetAggressive();
			return (ch);
		}
	}

	return NULL;
}

void CHARACTER_MANAGER::SelectStone(LPCHARACTER pkChr)
{
	m_pkChrSelectedStone = pkChr;
}

bool CHARACTER_MANAGER::SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen, bool bAggressive_)
{
	if (!dwVnum)
		return false;
	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		if (dwVnum)
			sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return false;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;
		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)	// ¸ø¸¸µç ¸ó½ºÅÍ°¡ ´ëÀåÀÏ °æ¿ì¿¡´Â ±×³É ½ÇÆÐ
				return false;

			continue;
		}

		sx = tch->GetX() - random_number(300, 500);
		sy = tch->GetY() - random_number(300, 500);
		ex = tch->GetX() + random_number(300, 500);
		ey = tch->GetY() + random_number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}
		if (bAggressive)
			tch->SetAggressive();

		if (tch->Goto(tx, ty))
			tch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}

	return true;
}

bool CHARACTER_MANAGER::SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	const DWORD dwGroupID = CMobManager::Instance().GetGroupFromGroupGroup(dwVnum);

	if( dwGroupID != 0 )
	{
		return SpawnGroup(dwGroupID, lMapIndex, sx, sy, ex, ey, pkRegen, bAggressive_, pDungeon);
	}
	else
	{
		sys_err( "NOT_EXIST_GROUP_GROUP_VNUM(%u) MAP(%ld)", dwVnum, lMapIndex );
		return false;
	}
}

LPCHARACTER CHARACTER_MANAGER::SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen, bool bAggressive_, LPDUNGEON pDungeon)
{
	if (!dwVnum)
		return false;

	CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);

	if (!pkGroup)
	{
		if (dwVnum)
			sys_err("NOT_EXIST_GROUP_VNUM(%u) Map(%u) ", dwVnum, lMapIndex);
		return NULL;
	}

	LPCHARACTER pkChrMaster = NULL;
	LPPARTY pkParty = NULL;

	const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

	bool bSpawnedByStone = false;
	bool bAggressive = bAggressive_;

	if (m_pkChrSelectedStone)
	{
		bSpawnedByStone = true;

		if (m_pkChrSelectedStone->GetDungeon())
			bAggressive = true;
	}

	LPCHARACTER chLeader = NULL;

	for (DWORD i = 0; i < c_rdwMembers.size(); ++i)
	{
		LPCHARACTER tch = SpawnMobRange(c_rdwMembers[i], lMapIndex, sx, sy, ex, ey, true, bSpawnedByStone);

		if (!tch)
		{
			if (i == 0)	// ¸ø¸¸µç ¸ó½ºÅÍ°¡ ´ëÀåÀÏ °æ¿ì¿¡´Â ±×³É ½ÇÆÐ
				return NULL;

			continue;
		}

		if (i == 0)
			chLeader = tch;

		tch->SetDungeon(pDungeon);

		sx = tch->GetX() - random_number(300, 500);
		sy = tch->GetY() - random_number(300, 500);
		ex = tch->GetX() + random_number(300, 500);
		ey = tch->GetY() + random_number(300, 500);

		if (m_pkChrSelectedStone)
			tch->SetStone(m_pkChrSelectedStone);
		else if (pkParty)
		{
			pkParty->Join(tch->GetVID());
			pkParty->Link(tch);
		}
		else if (!pkChrMaster)
		{
			pkChrMaster = tch;
			pkChrMaster->SetRegen(pkRegen);

			pkParty = CPartyManager::instance().CreateParty(pkChrMaster);
		}

		if (bAggressive)
			tch->SetAggressive();
	}

	return chLeader;
}

struct FuncUpdateAndResetChatCounter
{
	void operator () (LPCHARACTER ch)
	{
		ch->ResetChatCounter();
		ch->CFSM::Update();
	}
};

void CHARACTER_MANAGER::Update(int iPulse)
{
	BeginPendingDestroy();

	bool resetChatCounter = !(iPulse % PASSES_PER_SEC(5));

	std::for_each(m_map_pkPCChr.begin(), m_map_pkPCChr.end(),
		[resetChatCounter, iPulse](const NAME_MAP::value_type& v) {
		CHARACTER* ch = v.second;

		if (resetChatCounter)
			ch->ResetChatCounter();

		ch->UpdateCharacter(iPulse);
	});

	std::for_each(m_set_pkChrState.begin(), m_set_pkChrState.end(),
		[iPulse](CHARACTER* ch) {
		ch->UpdateStateMachine(iPulse);
	});

	// 1시간에 한번씩 몹 사냥 개수 기록
	if (0 == (iPulse % PASSES_PER_SEC(3600)))
	{
		//for (auto it = m_map_dwMobKillCount.begin(); it != m_map_dwMobKillCount.end(); ++it)
		//	DBManager::instance().SendMoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);

		m_map_dwMobKillCount.clear();
	}

	// 테스트 서버에서는 60초마다 캐릭터 개수를 센다
	if (0 == (iPulse % PASSES_PER_SEC(60)))
		sys_log(1, "CHARACTER COUNT vid %d pid %d",
		m_map_pkChrByVID.size(), m_map_pkChrByPID.size());

	// 지연된 DestroyCharacter 하기
	FlushPendingDestroy();
}

void CHARACTER_MANAGER::ProcessDelayedSave()
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.begin();

	while (it != m_set_pkChrForDelayedSave.end())
	{
		LPCHARACTER pkChr = *it++;
		pkChr->SaveReal();
	}

	m_set_pkChrForDelayedSave.clear();
}

bool CHARACTER_MANAGER::AddToStateList(LPCHARACTER ch)
{
	assert(ch != NULL);

	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it == m_set_pkChrState.end())
	{
		m_set_pkChrState.insert(ch);
		return true;
	}

	return false;
}

void CHARACTER_MANAGER::RemoveFromStateList(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrState.find(ch);

	if (it != m_set_pkChrState.end())
	{
		//sys_log(0, "RemoveFromStateList %p", ch);
		m_set_pkChrState.erase(it);
	}
}

void CHARACTER_MANAGER::DelayedSave(LPCHARACTER ch)
{
	m_set_pkChrForDelayedSave.insert(ch);
}

bool CHARACTER_MANAGER::FlushDelayedSave(LPCHARACTER ch)
{
	CHARACTER_SET::iterator it = m_set_pkChrForDelayedSave.find(ch);

	if (it == m_set_pkChrForDelayedSave.end())
		return false;

	m_set_pkChrForDelayedSave.erase(it);
	ch->SaveReal();
	return true;
}

void CHARACTER_MANAGER::RegisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.insert(ch);
}

void CHARACTER_MANAGER::UnregisterForMonsterLog(LPCHARACTER ch)
{
	m_set_pkChrMonsterLog.erase(ch);
}

void CHARACTER_MANAGER::PacketMonsterLog(LPCHARACTER ch, network::GCOutputPacket<network::GCChatPacket>& chat_pack)
{
	itertype(m_set_pkChrMonsterLog) it;

	for (it = m_set_pkChrMonsterLog.begin(); it!=m_set_pkChrMonsterLog.end();++it)
	{
		LPCHARACTER c = *it;

		if (ch && DISTANCE_APPROX(c->GetX()-ch->GetX(), c->GetY()-ch->GetY())>6000)
			continue;

		LPDESC d = c->GetDesc();

		if (d)
			d->Packet(chat_pack);
	}
}

void CHARACTER_MANAGER::KillLog(DWORD dwVnum)
{
	const DWORD SEND_LIMIT = 10000;

	itertype(m_map_dwMobKillCount) it = m_map_dwMobKillCount.find(dwVnum);

	if (it == m_map_dwMobKillCount.end())
		m_map_dwMobKillCount.insert(std::make_pair(dwVnum, 1));
	else
	{
		++it->second;

		if (it->second > SEND_LIMIT)
		{
			LogManager::instance().MoneyLog(MONEY_LOG_MONSTER_KILL, it->first, it->second);
			m_map_dwMobKillCount.erase(it);
		}
	}
}

void CHARACTER_MANAGER::RegisterRaceNum(DWORD dwVnum)
{
	m_set_dwRegisteredRaceNum.insert(dwVnum);
}

void CHARACTER_MANAGER::RegisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	if (m_set_dwRegisteredRaceNum.find(dwVnum) != m_set_dwRegisteredRaceNum.end()) // µî·ÏµÈ ¹øÈ£ ÀÌ¸é
	{
		sys_log(1, "RegisterRaceNumMap %s %u", ch->GetName(), dwVnum);
		m_map_pkChrByRaceNum[dwVnum].insert(ch);
	}
}

void CHARACTER_MANAGER::UnregisterRaceNumMap(LPCHARACTER ch)
{
	DWORD dwVnum = ch->GetRaceNum();

	itertype(m_map_pkChrByRaceNum) it = m_map_pkChrByRaceNum.find(dwVnum);

	if (it != m_map_pkChrByRaceNum.end())
		it->second.erase(ch);
}

bool CHARACTER_MANAGER::GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor & i)
{
	std::map<DWORD, CHARACTER_SET>::iterator it = m_map_pkChrByRaceNum.find(dwRaceNum);

	if (it == m_map_pkChrByRaceNum.end())
		return false;

	// ÄÁÅ×ÀÌ³Ê º¹»ç
	i = it->second;
	return true;
}

#define FIND_JOB_WARRIOR_0	(1 << 3)
#define FIND_JOB_WARRIOR_1	(1 << 4)
#define FIND_JOB_WARRIOR_2	(1 << 5)
#define FIND_JOB_WARRIOR	(FIND_JOB_WARRIOR_0 | FIND_JOB_WARRIOR_1 | FIND_JOB_WARRIOR_2)
#define FIND_JOB_ASSASSIN_0	(1 << 6)
#define FIND_JOB_ASSASSIN_1	(1 << 7)
#define FIND_JOB_ASSASSIN_2	(1 << 8)
#define FIND_JOB_ASSASSIN	(FIND_JOB_ASSASSIN_0 | FIND_JOB_ASSASSIN_1 | FIND_JOB_ASSASSIN_2)
#define FIND_JOB_SURA_0		(1 << 9)
#define FIND_JOB_SURA_1		(1 << 10)
#define FIND_JOB_SURA_2		(1 << 11)
#define FIND_JOB_SURA		(FIND_JOB_SURA_0 | FIND_JOB_SURA_1 | FIND_JOB_SURA_2)
#define FIND_JOB_SHAMAN_0	(1 << 12)
#define FIND_JOB_SHAMAN_1	(1 << 13)
#define FIND_JOB_SHAMAN_2	(1 << 14)
#define FIND_JOB_SHAMAN		(FIND_JOB_SHAMAN_0 | FIND_JOB_SHAMAN_1 | FIND_JOB_SHAMAN_2)

//
// (job+1)*3+(skill_group)
//
LPCHARACTER CHARACTER_MANAGER::FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except, int iMinLevel, int iMaxLevel)
{
	LPCHARACTER chFind = NULL;
	itertype(m_map_pkChrByPID) it;
	int n = 0;

	for (it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
	{
		LPCHARACTER ch = it->second;

		if (ch == except)
			continue;

		if (ch->GetLevel() < iMinLevel)
			continue;

		if (ch->GetLevel() > iMaxLevel)
			continue;

		if (ch->GetMapIndex() != lMapIndex)
			continue;

		if (uiJobFlag)
		{
			unsigned int uiChrJob = (1 << ((ch->GetJob() + 1) * 3 + ch->GetSkillGroup()));

			if (!IS_SET(uiJobFlag, uiChrJob))
				continue;
		}

		if (!chFind || random_number(1, ++n) == 1)
			chFind = ch;
	}

	return chFind;
}

int CHARACTER_MANAGER::GetMobItemRate(LPCHARACTER ch)	
{ 
	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0)
		return m_iMobItemRatePremium;
	return m_iMobItemRate; 
}

int CHARACTER_MANAGER::GetMobDamageRate(LPCHARACTER ch)	
{ 
	return m_iMobDamageRate; 
}

int CHARACTER_MANAGER::GetMobGoldAmountRate(LPCHARACTER ch)
{ 
	if ( !ch )
		return m_iMobGoldAmountRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldAmountRatePremium;
	return m_iMobGoldAmountRate; 
}

int CHARACTER_MANAGER::GetMobGoldDropRate(LPCHARACTER ch)
{
	if ( !ch )
		return m_iMobGoldDropRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0)
		return m_iMobGoldDropRatePremium;
	return m_iMobGoldDropRate;
}

int CHARACTER_MANAGER::GetMobExpRate(LPCHARACTER ch)
{ 
	if ( !ch )
		return m_iMobExpRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iMobExpRatePremium;
	return m_iMobExpRate; 
}

int	CHARACTER_MANAGER::GetUserDamageRate(LPCHARACTER ch)
{
	if (!ch)
		return m_iUserDamageRate;

	if (ch && ch->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
		return m_iUserDamageRatePremium;

	return m_iUserDamageRate;
}

void CHARACTER_MANAGER::SendScriptToMap(long lMapIndex, const std::string & s)
{
	LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);

	if (NULL == pSecMap)
		return;

	network::GCOutputPacket<network::GCScriptPacket> p;

	p->set_skin(1);
	p->set_script(s);

	pSecMap->for_each([&p](LPENTITY ent) {
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER ch = dynamic_cast<LPCHARACTER>(ent);
		if (ch && ch->GetDesc())
			ch->GetDesc()->Packet(p);
	});
}

bool CHARACTER_MANAGER::BeginPendingDestroy()
{
	// Begin ÀÌ ÈÄ¿¡ BeginÀ» ¶Ç ÇÏ´Â °æ¿ì¿¡ Flush ÇÏÁö ¾Ê´Â ±â´É Áö¿øÀ» À§ÇØ
	// ÀÌ¹Ì ½ÃÀÛµÇ¾îÀÖÀ¸¸é false ¸®ÅÏ Ã³¸®
	if (m_bUsePendingDestroy)
		return false;

	m_bUsePendingDestroy = true;
	return true;
}

void CHARACTER_MANAGER::FlushPendingDestroy()
{
	using namespace std;

	m_bUsePendingDestroy = false; // ÇÃ·¡±×¸¦ ¸ÕÀú ¼³Á¤ÇØ¾ß ½ÇÁ¦ Destroy Ã³¸®°¡ µÊ

	if (!m_set_pkChrPendingDestroy.empty())
	{
		sys_log(0, "FlushPendingDestroy size %d", m_set_pkChrPendingDestroy.size());
		
		CHARACTER_SET::iterator it = m_set_pkChrPendingDestroy.begin(),
			end = m_set_pkChrPendingDestroy.end();
		for ( ; it != end; ++it) {
			M2_DESTROY_CHARACTER(*it);
		}

		m_set_pkChrPendingDestroy.clear();
	}
}

#ifdef __UNIMPLEMENT__
void CHARACTER_MANAGER::RefreshAverageDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevelGroup) { }

WORD CHARACTER_MANAGER::GetAverageDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevel) const { }

WORD CHARACTER_MANAGER::__GetAverageDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevelGroup) const { }

void CHARACTER_MANAGER::SaveAverageDamage() { };

void CHARACTER_MANAGER::CharacterDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevel, int iDam)
{
	if (iDam <= 5)
		return;

	if (bType == DAMAGE_TYPE_NORMAL)
		bType = 0;
	else if (bType == DAMAGE_TYPE_NORMAL_RANGE)
		bType = 1;
	else
		return;

	BYTE bLevelGroup = bLevel / 10;

	if (iDam >= WORD_MAX)
		iDam = WORD_MAX;

	BYTE& bIndex = m_bRealDamageVectorIndex[bJob][bPVP][bType][bLevelGroup];
	WORD* awData = m_awRealDamage[bJob][bPVP][bType][bLevelGroup];
	awData[bIndex++] = iDam;

	if (bIndex == 100)
	{
		DWORD dwMaxValue = 0;
		for (int i = 0; i < 100; ++i)
			dwMaxValue += awData[i];
		m_vec_wAverageDamage[bJob][bPVP][bType][bLevelGroup].push_back(dwMaxValue / 100);

		RefreshAverageDamage(bJob, bPVP, bType, bLevelGroup);
		
		bIndex = 0;
	}
}
#endif

const DWORD* CHARACTER_MANAGER::GetUsableSkillList(BYTE bJob, BYTE bSkillGroup) const
{
	if (bSkillGroup == 0)
		return NULL;

	static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM][CHARACTER_SKILL_COUNT] =
	{
		{ { 1, 2, 3, 4, 5, 6 }, { 16, 17, 18, 19, 20, 21 } },
		{ { 31, 32, 33, 34, 35, 36 }, { 46, 47, 48, 49, 50, 51 } },
		{ { 61, 62, 63, 64, 65, 66 }, { 76, 77, 78, 79, 80, 81 } },
		{ { 91, 92, 93, 94, 95, 96 }, { 106, 107, 108, 109, 110, 111 } },
#ifdef __WOLFMAN__
		{ { 170, 171, 172, 173, 174, 175 }, { 170, 171, 172, 173, 174, 175 } },
#endif
	};

	return SkillList[bJob][bSkillGroup - 1];
}

void CHARACTER_MANAGER::SendOnlineTeamlerList(LPDESC pkDesc, BYTE bLanguage)
{
	if (bLanguage != LANGUAGE_TURKISH)
		bLanguage = LANGUAGE_ENGLISH;

	network::GCOutputPacket<network::GCTeamlerStatusPacket> packet;
	packet->set_is_online(true);

	auto& rkSet = m_set_OnlineTeamler[bLanguage];
	for (itertype(rkSet) it = rkSet.begin(); it != rkSet.end(); ++it)
	{
		packet->set_name((*it).c_str());
		pkDesc->Packet(packet);
	}
}

void CHARACTER_MANAGER::AddOnlineTeamler(const char* szName, BYTE bLanguage)
{
	if (bLanguage != LANGUAGE_TURKISH)
		bLanguage = LANGUAGE_ENGLISH;

	m_set_OnlineTeamler[bLanguage].insert(szName);
}

void CHARACTER_MANAGER::RemoveOnlineTeamler(const char* szName, BYTE bLanguage)
{
	if (bLanguage != LANGUAGE_TURKISH)
		bLanguage = LANGUAGE_ENGLISH;
	
	m_set_OnlineTeamler[bLanguage].erase(szName);
}

CharacterVectorInteractor::CharacterVectorInteractor(const CHARACTER_SET & r)
{
	using namespace std;

	reserve(r.size());
	insert(end(), r.begin(), r.end());

	if (CHARACTER_MANAGER::instance().BeginPendingDestroy())
		m_bMyBegin = true;
}

CharacterVectorInteractor::~CharacterVectorInteractor()
{
	if (m_bMyBegin)
		CHARACTER_MANAGER::instance().FlushPendingDestroy();
}
