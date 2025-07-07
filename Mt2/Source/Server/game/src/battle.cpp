#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "vector.h"
#include "packet.h"
#include "pvp.h"
#include "profiler.h"
#include "guild.h"
#include "affect.h"
#include "unique_item.h"
#include "lua_incl.h"
#include "arena.h"
#include "sectree.h"
#include "ani.h"
#include "priv_manager.h"
#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraDungeon.h"
#endif
#include "dungeon.h"

int battle_hit(LPCHARACTER ch, LPCHARACTER victim, int & iRetDam);

bool battle_distance_valid_by_xy(long x, long y, long tx, long ty)
{
	long distance = DISTANCE_APPROX(x - tx, y - ty);

	if (distance > 170)
		return false;

	return true;
}

bool battle_distance_valid(LPCHARACTER ch, LPCHARACTER victim)
{
	return battle_distance_valid_by_xy(ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY());
}

bool timed_event_cancel(LPCHARACTER ch)
{
	if (ch->m_pkChannelSwitchEvent)
	{
		event_cancel(&ch->m_pkChannelSwitchEvent);
		return true;
	}

	if (ch->m_pkTimedEvent)
	{
		event_cancel(&ch->m_pkTimedEvent);
		return true;
	}

	/* RECALL_DELAY
	   Â÷ÈÄ ÀüÅõ·Î ÀÎÇØ ±ÍÈ¯ºÎ µô·¹ÀÌ°¡ Ãë¼Ò µÇ¾î¾ß ÇÒ °æ¿ì ÁÖ¼® ÇØÁ¦
	   if (ch->m_pk_RecallEvent)
	   {
	   event_cancel(&ch->m_pkRecallEvent);
	   return true;
	   }
	   END_OF_RECALL_DELAY */

	return false;
}

bool battle_is_attackable(LPCHARACTER ch, LPCHARACTER victim)
{
	// »ó´ë¹æÀÌ Á×¾úÀ¸¸é Áß´ÜÇÑ´Ù.
	if (victim->IsDead())
		return false;

#ifdef ELONIA
	if (ch->GetMapIndex() != EMPIREWAR_MAP_INDEX)
#endif
	{
		SECTREE	*sectree = NULL;

		sectree	= ch->GetSectree();
		if (sectree && sectree->IsAttr(ch->GetX(), ch->GetY(), ATTR_BANPK))
			return false;

		sectree = victim->GetSectree();
		if (sectree && sectree->IsAttr(victim->GetX(), victim->GetY(), ATTR_BANPK))
			return false;
	}
	

	// ³»°¡ Á×¾úÀ¸¸é Áß´ÜÇÑ´Ù.
	if (ch->IsStun() || ch->IsDead())
		return false;

	if (ch->GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
		return true;

	if (ch->IsPC() && victim->IsPC())
	{
		CGuild* g1 = ch->GetGuild();
		CGuild* g2 = victim->GetGuild();

		if (g1 && g2)
		{
			if (g1->UnderWar(g2->GetID()))
				return true;
		}
	}

	if (victim->IsPet() || victim->IsMount())
		return false;

	if (CArenaManager::instance().CanAttack(ch, victim) == true)
		return true;

#ifdef ENABLE_HYDRA_DUNGEON
	if (CHydraDungeonManager::instance().CanAttack(ch, victim))
		return true;
#endif

	if (victim->IsStone() && ch->IsPC() && ch->IsPrivateMap(DUNGEON_RAZADOR_MAP_INDEX) && ch->GetDungeon() && ch->GetDungeon()->GetFlag("block_stone_dmg") == 1)
		return false;

	return CPVPManager::instance().CanAttack(ch, victim);
}

int battle_melee_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	if (!victim || ch == victim)
		return BATTLE_NONE;

	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	if (!battle_is_attackable(ch, victim))
		return BATTLE_NONE;
	
	if (test_server&&ch->IsPC())
		sys_log(0, "battle_melee_attack : [%s] attack to [%s]", ch->GetName(), victim->GetName());

	// °Å¸® Ã¼Å©
	int distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

	if (!victim->IsBuilding())
	{
		int max = 300;
	
		if (false == ch->IsPC())
		{
			// ¸ó½ºÅÍÀÇ °æ¿ì ¸ó½ºÅÍ °ø°Ý °Å¸®¸¦ »ç¿ë
			max = (int) (ch->GetMobAttackRange() * 1.15f);
		}
		else
		{
			// PCÀÏ °æ¿ì »ó´ë°¡ melee ¸÷ÀÏ °æ¿ì ¸÷ÀÇ °ø°Ý °Å¸®°¡ ÃÖ´ë °ø°Ý °Å¸®
			if (false == victim->IsPC() && BATTLE_TYPE_MELEE == victim->GetMobBattleType())
				max = MAX(300, (int) (victim->GetMobAttackRange() * 1.15f));
		}
#ifdef ENABLE_HYDRA_DUNGEON
		if (victim->GetRaceNum() == HYDRA_BOSS_VNUM)
			max = 1000;
#endif
		if (distance > max)
		{
			if (test_server)
				sys_log(0, "VICTIM_FAR: %s distance: %d max: %d", ch->GetName(), distance, max);

			return BATTLE_NONE;
		}
	}

	if (timed_event_cancel(ch))
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀüÅõ°¡ ½ÃÀÛ µÇ¾î Ãë¼Ò µÇ¾ú½À´Ï´Ù."));

#ifdef ENABLE_FASTER_LOGOUT
	if (ch->IsPC() && timed_event_cancel(victim))
#else
	if (timed_event_cancel(victim))
#endif
		victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(victim, "ÀüÅõ°¡ ½ÃÀÛ µÇ¾î Ãë¼Ò µÇ¾ú½À´Ï´Ù."));

	ch->SetPosition(POS_FIGHTING);
	ch->SetVictim(victim);

	const PIXEL_POSITION & vpos = victim->GetXYZ();
	ch->SetRotationToXY(vpos.x, vpos.y);

	int dam;
	int ret = battle_hit(ch, victim, dam);
	return (ret);
}

// ½ÇÁ¦ GET_BATTLE_VICTIMÀ» NULL·Î ¸¸µé°í ÀÌº¥Æ®¸¦ Äµ½½ ½ÃÅ²´Ù.
void battle_end_ex(LPCHARACTER ch)
{
	if (ch->IsPosition(POS_FIGHTING))
		ch->SetPosition(POS_STANDING);
}

void battle_end(LPCHARACTER ch)
{
	battle_end_ex(ch);
}

// AG = Attack Grade
// AL = Attack Limit
int CalcBattleDamage(int iDam, int iAttackerLev, int iVictimLev)
{
	if (iDam < 3)
		iDam = random_number(1, 5); 

	//return CALCULATE_DAMAGE_LVDELTA(iAttackerLev, iVictimLev, iDam);
	return iDam;
}

int CalcMagicDamageWithValue(int iDam, LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

int CalcMagicDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	int iDam = 0;

	if (pkAttacker->IsNPC())
	{
		iDam = CalcMeleeDamage(pkAttacker, pkVictim, false, false);	
	}

	iDam += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);

	return CalcMagicDamageWithValue(iDam, pkAttacker, pkVictim);
}

float CalcAttackRating(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreTargetRating)
{
	int iARSrc;
	int iERSrc;

	int attacker_dx = pkAttacker->GetPolymorphPoint(POINT_DX);
	int attacker_lv = pkAttacker->GetLevel();

	int victim_dx = pkVictim->GetPolymorphPoint(POINT_DX);
	int victim_lv = pkAttacker->GetLevel();

	iARSrc = MIN(90, (attacker_dx * 4	+ attacker_lv * 2) / 6);
	iERSrc = MIN(90, (victim_dx	  * 4	+ victim_lv   * 2) / 6);

	float fAR = ((float) iARSrc + 210.0f) / 300.0f; // fAR = 0.7 ~ 1.0
	
	// if(test_server && pkAttacker && pkAttacker->IsPC())	pkAttacker->tchat("fAR: %.2f", fAR);
	
	if (bIgnoreTargetRating)
		return fAR;

	// ((Edx * 2 + 20) / (Edx + 110)) * 0.3
	float fER = ((float) (iERSrc * 2 + 5) / (iERSrc + 95)) * 3.0f / 10.0f;

	// if(test_server && pkAttacker && pkAttacker->IsPC())	pkAttacker->tchat("fAR CAUTION: %.2f", fAR - fER);
	return fAR - fER;
}

int CalcAttBonus(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int iAtk)
{
	// PvP¿¡´Â Àû¿ëÇÏÁö¾ÊÀ½
	if (!pkVictim->IsPC())
		iAtk += pkAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_ATTACK_BONUS);

	// PvP¿¡´Â Àû¿ëÇÏÁö¾ÊÀ½
#ifdef __FAKE_PC__
	if (!pkAttacker->IsPC() && !pkAttacker->FakePC_Check())
#else
	if (!pkAttacker->IsPC())
#endif
	{
		int iReduceDamagePct = pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_TRANSFER_DAMAGE);
		iAtk = iAtk * (100 + iReduceDamagePct) / 100;
	}

#ifdef __FAKE_PC__
	if (pkAttacker->IsNPC() && !pkAttacker->FakePC_Check() && pkVictim->IsPC())
#else
	if (pkAttacker->IsNPC() && pkVictim->IsPC())
#endif
	{
		iAtk = (iAtk * CHARACTER_MANAGER::instance().GetMobDamageRate(pkAttacker)) / 100;
	}

#ifdef __FAKE_PC__
	if (pkVictim->IsNPC() && !pkVictim->FakePC_Check())
#else
	if (pkVictim->IsNPC())
#endif
	{
		if (pkVictim->IsRaceFlag(RACE_FLAG_ANIMAL))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ANIMAL)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_UNDEAD))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_UNDEAD)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_DEVIL))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DEVIL)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_HUMAN))
			iAtk += (iAtk * MAX(0, pkAttacker->GetPoint(POINT_ATTBONUS_HUMAN) - pkVictim->GetPoint(POINT_RESIST_ATTBONUS_HUMAN))) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_ORC))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ORC)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_MILGYO))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MILGYO)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_INSECT))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_INSECT)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_FIRE))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_FIRE)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_ICE))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ICE)) / 100;
#ifdef __ELEMENT_SYSTEM__
		if (pkVictim->IsRaceFlag(RACE_FLAG_ELEC) ||
			pkVictim->IsRaceFlag(RACE_FLAG_WIND) ||
			pkVictim->IsRaceFlag(RACE_FLAG_FIRE) ||
			pkVictim->IsRaceFlag(RACE_FLAG_ICE) ||
			pkVictim->IsRaceFlag(RACE_FLAG_EARTH) ||
			pkVictim->IsRaceFlag(RACE_FLAG_DARK))
		{
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ALL_ELEMENTS)) / 100;
		}

		if (pkVictim->IsRaceFlag(RACE_FLAG_ELEC))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ELEC)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_WIND))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WIND)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_EARTH))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_EARTH)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_DARK))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DARK)) / 100;
#endif
		if (pkVictim->IsRaceFlag(RACE_FLAG_DESERT))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_DESERT)) / 100;
		if (pkVictim->IsRaceFlag(RACE_FLAG_TREE))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_TREE)) / 100;
#ifndef PROMETA
		if (pkVictim->IsStone() || pkVictim->GetMobRank() >= MOB_RANK_BOSS)
		{
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER)) / 200;
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER_DIV10)) / 2000;
		}
		else
#endif
		{
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER)) / 100;
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_MONSTER_DIV10)) / 1000;
		}

		if (!pkVictim->IsStone() && pkVictim->GetMobRank() >= MOB_RANK_BOSS)
			iAtk += (iAtk * CPrivManager::instance().GetPriv(pkAttacker, PRIV_BOSS_PCT)) / 100;
		else if(pkVictim->IsStone())
			iAtk += (iAtk * CPrivManager::instance().GetPriv(pkAttacker, PRIV_STONE_PCT)) / 100;
		else if(!pkVictim->IsStone() && pkVictim->GetMobRank() < MOB_RANK_BOSS)
			iAtk += (iAtk * CPrivManager::instance().GetPriv(pkAttacker, PRIV_MOB_PCT)) / 100;

		if (pkVictim->IsStone())
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_METIN)) / 100;
		
		if (!pkVictim->IsStone() && pkVictim->GetMobRank() >= MOB_RANK_BOSS)
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_BOSS)) / 100;
		
#ifdef ENABLE_ZODIAC_TEMPLE
		if (pkVictim->IsRaceFlag(RACE_FLAG_ZODIAC))
			iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ZODIAC)) / 100;
#endif
	}
#ifdef __FAKE_PC__
	else if (pkVictim->IsPC() || pkVictim->FakePC_Check())
#else
	else if (pkVictim->IsPC())
#endif
	{
		iAtk += (iAtk * MAX(0, pkAttacker->GetPoint(POINT_ATTBONUS_HUMAN) - pkVictim->GetPoint(POINT_RESIST_ATTBONUS_HUMAN))) / 100;

		switch (pkVictim->GetJob())
		{
			case JOB_WARRIOR:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WARRIOR)) / 100;
				break;

			case JOB_ASSASSIN:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_ASSASSIN)) / 100;
				break;

			case JOB_SURA:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_SURA)) / 100;
				break;

			case JOB_SHAMAN:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_SHAMAN)) / 100;
				break;

#ifdef __WOLFMAN__
			case JOB_WOLFMAN:
				iAtk += (iAtk * pkAttacker->GetPoint(POINT_ATTBONUS_WOLFMAN)) / 100;
				break;
#endif
		}
	}

#ifdef __FAKE_PC__
	if (pkAttacker->IsPC() || pkAttacker->FakePC_Check())
#else
	if (pkAttacker->IsPC())
#endif
	{
		iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_HUMAN)) / 100;

		switch (pkAttacker->GetJob())
		{
			case JOB_WARRIOR:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WARRIOR)) / 100;
				break;
				
			case JOB_ASSASSIN:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_ASSASSIN)) / 100;
				break;
				
			case JOB_SURA:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_SURA)) / 100;
				break;

			case JOB_SHAMAN:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_SHAMAN)) / 100;
				break;

#ifdef __WOLFMAN__
			case JOB_WOLFMAN:
				iAtk -= (iAtk * pkVictim->GetPoint(POINT_RESIST_WOLFMAN)) / 100;
				break;
#endif
		}
	}

#ifdef __FAKE_PC__
	if (pkAttacker->IsNPC() && !pkAttacker->FakePC_Check() && pkVictim->IsPC())
#else
	if (pkAttacker->IsNPC() && pkVictim->IsPC())
#endif
	{
		iAtk -= iAtk * pkVictim->GetPoint(POINT_RESIST_MONSTER) / 100;

		if (pkAttacker->IsRaceFlag(RACE_FLAG_ELEC))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_ELEC))		/ 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_FIRE))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_FIRE))		/ 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_ICE))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_ICE))		/ 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_WIND))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_WIND))		/ 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_EARTH))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_EARTH))	/ 10000;
		else if (pkAttacker->IsRaceFlag(RACE_FLAG_DARK))
			iAtk -= (iAtk * 30 * pkVictim->GetPoint(POINT_RESIST_DARK))		/ 10000;
	}
		
	
	return iAtk;
}

void Item_GetDamage(LPITEM pkItem, int* pdamMin, int* pdamMax)
{
	*pdamMin = 0;
	*pdamMax = 1;

	if (!pkItem)
		return;

	switch (pkItem->GetType())
	{
		case ITEM_ROD:
		case ITEM_PICK:
			return;
	}

	if (pkItem->GetType() != ITEM_WEAPON)
		sys_err("Item_GetDamage - !ITEM_WEAPON vnum=%d, type=%d", pkItem->GetOriginalVnum(), pkItem->GetType());

	*pdamMin = pkItem->GetValue(3);
	*pdamMax = pkItem->GetValue(4);

#ifdef __ALPHA_EQUIP__
	*pdamMin += pkItem->GetAlphaEquipValue();
	*pdamMax += pkItem->GetAlphaEquipValue();
#endif
}

void Item_GetMagicDamage(LPITEM pkItem, int* pmagicDamMin, int* pmagicDamMax)
{
	*pmagicDamMin = 0;
	*pmagicDamMax = 1;

	if (!pkItem)
		return;

	switch (pkItem->GetType())
	{
	case ITEM_ROD:
	case ITEM_PICK:
		return;
	}

	if (pkItem->GetType() != ITEM_WEAPON)
		sys_err("Item_GetMagicDamage - !ITEM_WEAPON vnum=%d, type=%d", pkItem->GetOriginalVnum(), pkItem->GetType());

	*pmagicDamMin = pkItem->GetValue(1);
	*pmagicDamMax = pkItem->GetValue(2);

#ifdef __ALPHA_EQUIP__
	*pmagicDamMin += pkItem->GetAlphaEquipValue();
	*pmagicDamMax += pkItem->GetAlphaEquipValue();
#endif
}

int CalcMeleeDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, bool bIgnoreDefense, bool bIgnoreTargetRating)
{
	LPITEM pWeapon = pkAttacker->GetWear(WEAR_WEAPON);
#ifdef __ACCE_COSTUME__
	LPITEM pAcce = pkAttacker->GetWear(WEAR_ACCE);
#endif
	bool bPolymorphed = pkAttacker->IsPolymorphed();

	if (pWeapon && !(bPolymorphed && !pkAttacker->IsPolyMaintainStat()))
	{
		if (pWeapon->GetType() != ITEM_WEAPON)
			return 0;

		switch (pWeapon->GetSubType())
		{
			case WEAPON_SWORD:
			case WEAPON_DAGGER:
			case WEAPON_TWO_HANDED:
			case WEAPON_BELL:
			case WEAPON_FAN:
			case WEAPON_MOUNT_SPEAR:
#ifdef __WOLFMAN__
			case WEAPON_CLAW:
#endif
				break;

			case WEAPON_BOW:
				sys_err("CalcMeleeDamage should not handle bows (name: %s)", pkAttacker->GetName());
				return 0;

			default:
				return 0;
		}
	}

	int iDam = 0;
	float fAR = CalcAttackRating(pkAttacker, pkVictim, bIgnoreTargetRating);
	int iDamMin = 0, iDamMax = 0;

	// TESTSERVER_SHOW_ATTACKINFO
	int DEBUG_iDamCur = 0;
	int DEBUG_iDamBonus = 0;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO

	if (bPolymorphed && !pkAttacker->IsPolyMaintainStat())
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX

		DWORD dwMobVnum = pkAttacker->GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);

		if (pMob)
		{
			int iPower = pkAttacker->GetPolymorphPower();
			iDamMin += pMob->m_table.damage_min() * iPower / 100;
			iDamMax += pMob->m_table.damage_max() * iPower / 100;
		}
	}
	else if (pWeapon)
	{
		// MONKEY_ROD_ATTACK_BUG_FIX
		Item_GetDamage(pWeapon, &iDamMin, &iDamMax);
		// END_OF_MONKEY_ROD_ATTACK_BUG_FIX
	}
#ifdef __FAKE_PC__
	else if (pkAttacker->IsNPC() && !pkAttacker->FakePC_Check())
#else
	else if (pkAttacker->IsNPC())
#endif
	{
		iDamMin = pkAttacker->GetMobDamageMin();
		iDamMax = pkAttacker->GetMobDamageMax();
	}

#ifdef __ACCE_COSTUME__
	if (pAcce && pAcce->GetType() == ITEM_COSTUME && pAcce->GetSubType() == COSTUME_ACCE && pAcce->GetSocket(1) != 0)
	{
		auto p = ITEM_MANAGER::instance().GetTable(pAcce->GetSocket(1));
		if (p)
		{
#ifdef __ALPHA_EQUIP__
			iDamMin += float(p->values(3) + pAcce->GetAlphaEquipValue()) / 100.0f * float(pAcce->GetSocket(0));
			iDamMax += float(p->values(4) + pAcce->GetAlphaEquipValue()) / 100.0f * float(pAcce->GetSocket(0));
#else
			iDamMin += float(p->values(3)) / 100.0f * float(pAcce->GetSocket(0));
			iDamMax += float(p->values(4)) / 100.0f * float(pAcce->GetSocket(0));
#endif
		}
	}
#endif

	iDam = random_number(iDamMin, iDamMax) * 2;

	// TESTSERVER_SHOW_ATTACKINFO
	DEBUG_iDamCur = iDam;
	// END_OF_TESTSERVER_SHOW_ATTACKINFO
	//
	int iAtk = 0;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttacker->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttacker->GetLevel() * 2);
#ifdef ENABLE_RUNE_SYSTEM
	if (!pkVictim->IsPC())
		iAtk += pkAttacker->GetRuneData().soulsHarvested;
#endif
	iAtk = (int) (iAtk * fAR);
	iAtk += pkAttacker->GetLevel() * 2; // and add again

	if (pWeapon)
	{
		iAtk += pWeapon->GetValue(5) * 2;

		// 2004.11.12.myevan.TESTSERVER_SHOW_ATTACKINFO
		DEBUG_iDamBonus = pWeapon->GetValue(5) * 2;
		///////////////////////////////////////////////
	}

#ifdef __ACCE_COSTUME__
	if (pAcce && pAcce->GetType() == ITEM_COSTUME && pAcce->GetSubType() == COSTUME_ACCE && pAcce->GetSocket(1) != 0)
	{
		auto p = ITEM_MANAGER::instance().GetTable(pAcce->GetSocket(1));
		if (p)
		{
			iAtk += (float(p->values(5)) / 100.0f * float(pAcce->GetSocket(0))) * 2;
		}
	}
#endif

	iAtk += pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS); // party attacker role bonus
	iAtk += (int)(pkAttacker->GetMaxSP() * pkAttacker->GetPoint(POINT_DAMAGE_BY_SP_BONUS) / 100);
	iAtk = (int) (iAtk * (100 + (pkAttacker->GetPoint(POINT_ATT_BONUS) + pkAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pkAttacker, pkVictim, iAtk);

	int iDef = 0;

	if (!bIgnoreDefense)
	{
		iDef = (pkVictim->GetPoint(POINT_DEF_GRADE) * (100 + pkVictim->GetPoint(POINT_DEF_BONUS)) / 100);

		if (!pkAttacker->IsPC())
			iDef += pkVictim->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_DEFENSE_BONUS);
	}

#ifdef __FAKE_PC__
	if (pkAttacker->IsNPC() && !pkAttacker->FakePC_Check())
#else
	if (pkAttacker->IsNPC())
#endif
		iAtk = (int) (iAtk * pkAttacker->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);
/*
	if (test_server)
	{
		int DEBUG_iLV = pkAttacker->GetLevel()*2;
		int DEBUG_iST = int((pkAttacker->GetPoint(POINT_ATT_GRADE) - DEBUG_iLV) * fAR);
		int DEBUG_iPT = pkAttacker->GetPoint(POINT_PARTY_ATTACKER_BONUS);
		int DEBUG_iWP = 0;
		int DEBUG_iPureAtk = 0;
		int DEBUG_iPureDam = 0;
		char szRB[32] = "";
		char szGradeAtkBonus[32] = "";

		DEBUG_iWP = int(DEBUG_iDamCur * fAR);
		DEBUG_iPureAtk = DEBUG_iLV + DEBUG_iST + DEBUG_iWP+DEBUG_iDamBonus;
		DEBUG_iPureDam = iAtk - iDef;

		if (pkAttacker->IsNPC())
		{
			snprintf(szGradeAtkBonus, sizeof(szGradeAtkBonus), "=%d*%.1f", DEBUG_iPureAtk, pkAttacker->GetMobDamageMultiply());
			DEBUG_iPureAtk = int(DEBUG_iPureAtk * pkAttacker->GetMobDamageMultiply());
		}

		if (DEBUG_iDamBonus != 0)
			snprintf(szRB, sizeof(szRB), "+RB(%d)", DEBUG_iDamBonus);

		char szPT[32] = "";

		if (DEBUG_iPT != 0)
			snprintf(szPT, sizeof(szPT), ", PT=%d", DEBUG_iPT);

		char szUnknownAtk[32] = "";

		if (iAtk != DEBUG_iPureAtk)
			snprintf(szUnknownAtk, sizeof(szUnknownAtk), "+?(%d)", iAtk-DEBUG_iPureAtk);

		char szUnknownDam[32] = "";

		if (iDam != DEBUG_iPureDam)
			snprintf(szUnknownDam, sizeof(szUnknownDam), "+?(%d)", iDam-DEBUG_iPureDam);

		char szMeleeAttack[128];

		snprintf(szMeleeAttack, sizeof(szMeleeAttack), 
				"%s(%d)-%s(%d)=%d%s, ATK=LV(%d)+ST(%d)+WP(%d)%s%s%s, AR=%.3g%s", 
				pkAttacker->GetName(),
				iAtk,
				pkVictim->GetName(),
				iDef,
				iDam,
				szUnknownDam,
				DEBUG_iLV, 
				DEBUG_iST,
				DEBUG_iWP, 
				szRB,
				szUnknownAtk,
				szGradeAtkBonus,
				fAR,
				szPT);

		pkAttacker->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
		pkVictim->ChatPacket(CHAT_TYPE_TALKING, "%s", szMeleeAttack);
	}*/

	return CalcBattleDamage(iDam, pkAttacker->GetLevel(), pkVictim->GetLevel());
}

int CalcArrowDamage(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, LPITEM pkBow, LPITEM pkArrow, bool bIgnoreDefense)
{
	if (!pkBow || pkBow->GetType() != ITEM_WEAPON || pkBow->GetSubType() != WEAPON_BOW)
		return 0;

	if (!pkArrow)
		return 0;

	LPCHARACTER pkAttackerDmg = pkAttacker;
#ifdef __FAKE_PC__
	if (pkAttacker->FakePC_Check())
		pkAttackerDmg = pkAttacker->FakePC_GetOwner();
#endif

	// Å¸°ÝÄ¡ °è»êºÎ
	int iDist = (int) (DISTANCE_SQRT(pkAttacker->GetX() - pkVictim->GetX(), pkAttacker->GetY() - pkVictim->GetY()));
	if (pkArrow->GetSubType() == WEAPON_QUIVER)
		iDist = 200;

	//int iGap = (iDist / 100) - 5 - pkBow->GetValue(5) - pkAttacker->GetPoint(POINT_BOW_DISTANCE);
	int iGap = (iDist / 100) - 5 - pkAttackerDmg->GetPoint(POINT_BOW_DISTANCE);
	int iPercent = 100 - (iGap * 5);

	if (iPercent <= 0)
		return 0;
	else if (iPercent > 100)
		iPercent = 100;

	int iDam = 0;

	// get arrow damage
	int iArrowDamage;
	if (pkArrow->GetSubType() == WEAPON_ARROW)
		iArrowDamage = pkArrow->GetValue(3);
	else
	{
		if (pkArrow->GetSubType() != WEAPON_QUIVER)
		{
			sys_err("invalid arrow %u (sub type %u)", pkArrow->GetVnum(), pkArrow->GetSubType());
			iArrowDamage = 0;
		}
		else
		{
			auto pTab = ITEM_MANAGER::instance().GetTable(pkArrow->GetSocket(0));
			if (!pTab)
			{
				sys_err("cannot get arrow damage by quiver");
				iArrowDamage = 0;
			}
			else
				iArrowDamage = pTab->values(3);
		}
	}

	float fAR = CalcAttackRating(pkAttackerDmg, pkVictim, false);
	int iMinBowDam, iMaxBowDam;
	Item_GetDamage(pkBow, &iMinBowDam, &iMaxBowDam);
	iDam = random_number(iMinBowDam, iMaxBowDam) * 2 + iArrowDamage;
	int iAtk;

	// level must be ignored when multiply by fAR, so subtract it before calculation.
	iAtk = pkAttackerDmg->GetPoint(POINT_ATT_GRADE) + iDam - (pkAttackerDmg->GetLevel() * 2);
#ifdef ENABLE_RUNE_SYSTEM
	if (!pkVictim->IsPC())
		iAtk += pkAttacker->GetRuneData().soulsHarvested;
#endif
	iAtk = (int) (iAtk * fAR);
	iAtk += pkAttackerDmg->GetLevel() * 2; // and add again

	// Refine Grade
	iAtk += pkBow->GetValue(5) * 2;

	iAtk += pkAttackerDmg->GetPoint(POINT_PARTY_ATTACKER_BONUS);
	iAtk += (int)(pkAttacker->GetMaxSP() * pkAttacker->GetPoint(POINT_DAMAGE_BY_SP_BONUS) / 100);
	iAtk = (int)(iAtk * (100 + (pkAttackerDmg->GetPoint(POINT_ATT_BONUS) + pkAttackerDmg->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100);

	iAtk = CalcAttBonus(pkAttackerDmg, pkVictim, iAtk);

	int iDef = 0;

	if (!bIgnoreDefense)
		iDef = (pkVictim->GetPoint(POINT_DEF_GRADE) * (100 + pkAttackerDmg->GetPoint(POINT_DEF_BONUS)) / 100);

	if (pkAttackerDmg->IsNPC())
		iAtk = (int)(iAtk * pkAttackerDmg->GetMobDamageMultiply());

	iDam = MAX(0, iAtk - iDef);

	int iPureDam = iDam;

	iPureDam = (iPureDam * iPercent) / 100;

/*	if (test_server)
	{
		pkAttackerDmg->ChatPacket(CHAT_TYPE_INFO, "ARROW %s -> %s, DAM %d DIST %d GAP %d %% %d",
				pkAttacker->GetName(), 
				pkVictim->GetName(), 
				iPureDam, 
				iDist, iGap, iPercent);
	}*/

	return iPureDam;
	//return iDam;
}


void NormalAttackAffect(LPCHARACTER pkAttacker, LPCHARACTER pkVictim)
{
	// µ¶ °ø°ÝÀº Æ¯ÀÌÇÏ¹Ç·Î Æ¯¼ö Ã³¸®
	if (pkAttacker->GetPoint(POINT_POISON_PCT) && !pkVictim->IsAffectFlag(AFF_POISON))
	{
		if (random_number(1, 100) <= pkAttacker->GetPoint(POINT_POISON_PCT))
			pkVictim->AttackedByPoison(pkAttacker);
	}

	int iStunDuration = 2;
	if (pkAttacker->IsPC() && !pkVictim->IsPC())
		iStunDuration = 4;

	int iImmuneStunFlag = IMMUNE_STUN;
	//if (pkAttacker->IsPC() && pkVictim->IsPC() && pkVictim->IsRiding())
	//	iImmuneStunFlag = 0;

	AttackAffect(pkAttacker, pkVictim, POINT_STUN_PCT, iImmuneStunFlag,  AFFECT_STUN, POINT_NONE,		0, AFF_STUN, iStunDuration, "STUN");
	AttackAffect(pkAttacker, pkVictim, POINT_SLOW_PCT, IMMUNE_SLOW,  AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 20,		"SLOW");
}

int battle_hit(LPCHARACTER pkAttacker, LPCHARACTER pkVictim, int & iRetDam)
{
	//PROF_UNIT puHit("Hit");
	if (test_server)
		sys_log(0, "battle_hit : [%s] attack to [%s] : dam :%d", pkAttacker->GetName(), pkVictim->GetName(), iRetDam);

	int iDam = CalcMeleeDamage(pkAttacker, pkVictim);

	if (iDam <= 0)
		return (BATTLE_DAMAGE);

	NormalAttackAffect(pkAttacker, pkVictim);

	// µ¥¹ÌÁö °è»ê
	//iDam = iDam * (100 - pkVictim->GetPoint(POINT_RESIST)) / 100;
	LPITEM pkWeapon = pkAttacker->GetWear(WEAR_WEAPON);

	if (pkWeapon)
		switch (pkWeapon->GetSubType())
		{
			case WEAPON_SWORD:
				iDam = iDam * (100 - MINMAX(0, pkVictim->GetPoint(POINT_RESIST_SWORD) - pkAttacker->GetPoint(POINT_RESIST_SWORD_PEN), 100)) / 100;
				break;

			case WEAPON_TWO_HANDED:
				iDam = iDam * (100 - MINMAX(0, pkVictim->GetPoint(POINT_RESIST_TWOHAND) - pkAttacker->GetPoint(POINT_RESIST_TWOHAND_PEN), 100)) / 100;
				break;

			case WEAPON_DAGGER:
				iDam = iDam * (100 - MINMAX(0, pkVictim->GetPoint(POINT_RESIST_DAGGER) - pkAttacker->GetPoint(POINT_RESIST_DAGGER_PEN), 100)) / 100;
				break;

			case WEAPON_BELL:
				iDam = iDam * (100 - MINMAX(0, pkVictim->GetPoint(POINT_RESIST_BELL) - pkAttacker->GetPoint(POINT_RESIST_BELL_PEN), 100)) / 100;
				break;

			case WEAPON_FAN:
				iDam = iDam * (100 - MINMAX(0, pkVictim->GetPoint(POINT_RESIST_FAN) - pkAttacker->GetPoint(POINT_RESIST_FAN_PEN), 100)) / 100;
				break;

			case WEAPON_BOW:
				iDam = iDam * (100 - MINMAX(0, pkVictim->GetPoint(POINT_RESIST_BOW) - pkAttacker->GetPoint(POINT_RESIST_BOW_PEN), 100)) / 100;
				break;

#ifdef __WOLFMAN__
			case WEAPON_CLAW:
				iDam = iDam * (100 - pkVictim->GetPoint(POINT_RESIST_CLAW)) / 100;
				break;
#endif
		}


	//ÃÖÁ¾ÀûÀÎ µ¥¹ÌÁö º¸Á¤. (2011³â 2¿ù ÇöÀç ´ë¿Õ°Å¹Ì¿¡°Ô¸¸ Àû¿ë.)
	float attMul = pkAttacker->GetAttMul();
	float tempIDam = iDam;
	iDam = attMul * tempIDam + 0.5f;

	iRetDam = iDam;

	//PROF_UNIT puDam("Dam");
	if (pkVictim->Damage(pkAttacker, iDam, DAMAGE_TYPE_NORMAL))
		return (BATTLE_DEAD);

	return (BATTLE_DAMAGE);
}

DWORD GET_ATTACK_SPEED(LPCHARACTER ch)
{
	if (NULL == ch)
		return 1000;

	LPITEM item = ch->GetWear(WEAR_WEAPON);
	DWORD default_bonus = SPEEDHACK_LIMIT_BONUS;	// À¯µÎ¸® °ø¼Ó(±âº» 80)
	DWORD riding_bonus = 0;

	if (ch->IsRiding())
	{
		// ¹º°¡¸¦ ÅÀÀ¸¸é Ãß°¡°ø¼Ó 50
		riding_bonus = 50;
	}

	DWORD ani_speed = ani_attack_speed(ch);
	DWORD real_speed = (ani_speed * 100) / (default_bonus + ch->GetPoint(POINT_ATT_SPEED) + riding_bonus);

	// ´Ü°ËÀÇ °æ¿ì °ø¼Ó 2¹è
	if (item && item->GetSubType() == WEAPON_DAGGER)
		real_speed /= 2;

	return real_speed;

}

void SET_ATTACK_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	ch->m_kAttackLog.dwVID = victim->GetVID();
	ch->m_kAttackLog.dwTime = current_time;
}

void SET_ATTACKED_TIME(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (NULL == ch || NULL == victim)
		return;

	if (!ch->IsPC())
		return;

	victim->m_AttackedLog.dwPID			= ch->GetPlayerID();
	victim->m_AttackedLog.dwAttackedTime= current_time;
}

bool IS_SPEED_HACK(LPCHARACTER ch, LPCHARACTER victim, DWORD current_time)
{
	if (ch->m_kAttackLog.dwVID == victim->GetVID())
	{
		if (current_time - ch->m_kAttackLog.dwTime < GET_ATTACK_SPEED(ch))
		{
			INCREASE_SPEED_HACK_COUNT(ch);

			if (test_server)
			{
				sys_log(0, "%s attack hack! time (delta, limit)=(%u, %u) hack_count %d",
						ch->GetName(),
						current_time - ch->m_kAttackLog.dwTime,
						GET_ATTACK_SPEED(ch),
						ch->m_speed_hack_count);

				ch->ChatPacket(CHAT_TYPE_INFO, "%s attack hack! time (delta, limit)=(%u, %u) hack_count %d",
						ch->GetName(),
						current_time - ch->m_kAttackLog.dwTime,
						GET_ATTACK_SPEED(ch),
						ch->m_speed_hack_count);
			}

			SET_ATTACK_TIME(ch, victim, current_time);
			SET_ATTACKED_TIME(ch, victim, current_time);
			return true;
		}
	}

	SET_ATTACK_TIME(ch, victim, current_time);

	if (victim->m_AttackedLog.dwPID == ch->GetPlayerID())
	{
		if (current_time - victim->m_AttackedLog.dwAttackedTime < GET_ATTACK_SPEED(ch))
		{
			INCREASE_SPEED_HACK_COUNT(ch);

			if (test_server)
			{
				sys_log(0, "%s Attack Speed HACK! time (delta, limit)=(%u, %u), hack_count = %d",
						ch->GetName(),
						current_time - victim->m_AttackedLog.dwAttackedTime,
						GET_ATTACK_SPEED(ch),
						ch->m_speed_hack_count);

				ch->ChatPacket(CHAT_TYPE_INFO, "Attack Speed Hack(%s), (delta, limit)=(%u, %u)",
						ch->GetName(),
						current_time - victim->m_AttackedLog.dwAttackedTime,
						GET_ATTACK_SPEED(ch));
			}

			SET_ATTACKED_TIME(ch, victim, current_time);
			return true;
		}
	}

	SET_ATTACKED_TIME(ch, victim, current_time);
	return false;
}


