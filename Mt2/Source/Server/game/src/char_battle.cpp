#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "battle.h"
#include "pvp.h"
#include "skill.h"
#include "start_position.h"
#include "profiler.h"
#include "cmd.h"
#include "dungeon.h"
#include "log.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "db.h"
#include "vector.h"
#include "marriage.h"
#include "arena.h"
#include "regen.h"
#include "exchange.h"
#include "shop_manager.h"
#include "dev_log.h"
#include "ani.h"
#include "packet.h"
#include "party.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "questmanager.h"
#include "questlua.h"
#include "BlueDragon.h"
#include "DragonLair.h"
#include "mount_system.h"

#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraDungeon.h"
#endif

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif
#include <chrono>

#ifdef __NEW_DROP_SYSTEM__
#include <random>
#endif

DWORD AdjustExpByLevel(const LPCHARACTER ch, const DWORD exp)
{
	if (PLAYER_EXP_TABLE_MAX < ch->GetLevel())
	{
		double ret = 0.95;
		double factor = 0.1;

		for (ssize_t i=0 ; i < ch->GetLevel()-100 ; ++i)
		{
			if ( (i%10) == 0)
				factor /= 2.0;

			ret *= 1.0 - factor;
		}

		ret = ret * static_cast<double>(exp);

		if (ret < 1.0)
			return 1;

		return static_cast<DWORD>(ret);
	}

	return exp;
}

bool CHARACTER::CanBeginFight() const
{
	if (!CanMove())
		return false;

	return m_pointsInstant.position == POS_STANDING && !IsDead() && !IsStun();
}

void CHARACTER::BeginFight(LPCHARACTER pkVictim)
{
#ifdef __FAKE_PC__
	if (FakePC_Check() && !CPVPManager::instance().CanAttack(this, pkVictim))
		return;
#endif
#ifdef __FAKE_BUFF__
	if (FakeBuff_Check() && !CPVPManager::instance().CanAttack(this, pkVictim))
		return;
#endif


	SetVictim(pkVictim);
	SetPosition(POS_FIGHTING);
	SetNextStatePulse(1);
}

bool CHARACTER::CanFight() const
{
	return m_pointsInstant.position >= POS_FIGHTING ? true : false;
}

void CHARACTER::CreateFly(BYTE bType, LPCHARACTER pkVictim)
{
	network::GCOutputPacket<network::GCCreateFlyPacket> packFly;

	packFly->set_type(bType);
	packFly->set_start_vid(GetVID());
	packFly->set_end_vid(pkVictim->GetVID());

	PacketAround(packFly);
}

void CHARACTER::DistributeSP(LPCHARACTER pkKiller, int iMethod)
{
	if (pkKiller->GetSP() >= pkKiller->GetMaxSP())
		return;

	bool bAttacking = (get_dword_time() - GetLastAttackTime()) < 3000;
	bool bMoving = (get_dword_time() - GetLastMoveTime()) < 3000;

	if (iMethod == 1)
	{
		int num = random_number(0, 3);

		if (!num)
		{
			int iLvDelta = GetLevel() - pkKiller->GetLevel();
			int iAmount = 0;

			if (iLvDelta >= 5)
				iAmount = 10;
			else if (iLvDelta >= 0)
				iAmount = 6;
			else if (iLvDelta >= -3)
				iAmount = 2;

			if (iAmount != 0)
			{
				iAmount += (iAmount * pkKiller->GetPoint(POINT_SP_REGEN)) / 100;

				if (iAmount >= 11)
					CreateFly(FLY_SP_BIG, pkKiller);
				else if (iAmount >= 7)
					CreateFly(FLY_SP_MEDIUM, pkKiller);
				else
					CreateFly(FLY_SP_SMALL, pkKiller);

				pkKiller->PointChange(POINT_SP, iAmount);
			}
		}
	}
	else
	{
		if (pkKiller->GetJob() == JOB_SHAMAN || (pkKiller->GetJob() == JOB_SURA && pkKiller->GetSkillGroup() == 2))
		{
			int iAmount;

			if (bAttacking)
				iAmount = 2 + GetMaxSP() / 100;
			else if (bMoving)
				iAmount = 3 + GetMaxSP() * 2 / 100;
			else
				iAmount = 10 + GetMaxSP() * 3 / 100; // Æò»ó½Ã

			iAmount += (iAmount * pkKiller->GetPoint(POINT_SP_REGEN)) / 100;
			pkKiller->PointChange(POINT_SP, iAmount);
		}
		else
		{
			int iAmount;

			if (bAttacking)
				iAmount = 2 + pkKiller->GetMaxSP() / 200;
			else if (bMoving)
				iAmount = 2 + pkKiller->GetMaxSP() / 100;
			else
			{
				// Æò»ó½Ã
				if (pkKiller->GetHP() < pkKiller->GetMaxHP())
					iAmount = 2 + (pkKiller->GetMaxSP() / 100); // ÇÇ ´Ù ¾ÈÃ¡À»¶§
				else
					iAmount = 9 + (pkKiller->GetMaxSP() / 100); // ±âº»
			}

			iAmount += (iAmount * pkKiller->GetPoint(POINT_SP_REGEN)) / 100;
			pkKiller->PointChange(POINT_SP, iAmount);
		}
	}
}

#ifdef __FAKE_PC__
struct FCollectFakePCVictimList
{
	FCollectFakePCVictimList(LPCHARACTER pkFakePC, std::vector<LPCHARACTER>& rvec_VictimList) :
		m_pkFakePC(pkFakePC), m_rvec_VictimList(rvec_VictimList)
	{
	}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		if (pkChr == m_pkFakePC)
			return;

		float fDist = DISTANCE_APPROX(m_pkFakePC->GetX() - pkChr->GetX(), m_pkFakePC->GetY() - pkChr->GetY());
		if (fDist > m_pkFakePC->GetMobAttackRange() * 1.25f)
			return;

		const float fMaxRotationDif = 100.0f;

		float fRealRotation = m_pkFakePC->GetRotation();
		float fHitRotation = GetDegreeFromPositionXY(m_pkFakePC->GetX(), m_pkFakePC->GetY(), pkChr->GetX(), pkChr->GetY());

		if (fRealRotation > 180.0f)
			fRealRotation = 360.0f - fRealRotation;
		if (fHitRotation > 180.0f)
			fHitRotation = 360.0f - fHitRotation;

		float fDif = abs(fRealRotation - fHitRotation);
		if (fDif > fMaxRotationDif)
			return;

		if (!CPVPManager::instance().CanAttack(m_pkFakePC, pkChr))
			return;

		m_rvec_VictimList.push_back(pkChr);
	}

	LPCHARACTER					m_pkFakePC;
	std::vector<LPCHARACTER>&	m_rvec_VictimList;
};
#endif

bool CHARACTER::Attack(LPCHARACTER pkVictim, BYTE bType)
{
	if (test_server)
		sys_log(0, "[TEST_SERVER] Attack : %s type %d, MobBattleType %d", GetName(), bType, (IsPC() || !GetMobBattleType()) ? 0 : GetMobAttackRange());
	//PROF_UNIT puAttack("Attack");
	if (!CanMove())
		return false;

	if (pkVictim->IsMount() || pkVictim->IsPet())
		return false;

	if (pkVictim->IsPC() && IsPC() && quest::CQuestManager::instance().GetEventFlag("enable_bosshunt_event") && 
			(GetMapIndex()==10 || GetMapIndex()==11 || GetMapIndex()==12 || GetMapIndex()==13))
		return false;

	if (pkVictim->IsPC() && IsPC() && quest::CQuestManager::instance().GetEventFlag("anniversary_disable_pvp_boss") && 
			(GetMapIndex() == 10 || GetMapIndex() == 11))
		return false;



#ifdef __ANTI_CHEAT_FIXES__
#ifdef ELONIA
	if (GetMapIndex() != EMPIREWAR_MAP_INDEX)
#endif
	{
		SECTREE	*sectree = NULL;
		SECTREE	*vsectree = NULL;
		sectree = GetSectree();
		vsectree = pkVictim->GetSectree();

		if (sectree && vsectree) {
			if (sectree->IsAttr(GetX(), GetY(), ATTR_BANPK) || vsectree->IsAttr(pkVictim->GetX(), pkVictim->GetY(), ATTR_BANPK)) {
				if (GetDesc()) {
					// LogManager::instance().HackLog("ANTISAFEZONE", this);
					return false;
				}
			}
		}
	}
#endif

	DWORD dwCurrentTime = get_dword_time();

	if (IsPC())
	{
		if (IS_SPEED_HACK(this, pkVictim, dwCurrentTime))
			return false;

		if (bType == 0 && dwCurrentTime < GetSkipComboAttackByTime())
			return false;

#ifdef CHECK_TIME_AFTER_PVP
		if (pkVictim->IsPC())
		{
			m_dwLastAttackTimePVP = get_dword_time();
			pkVictim->m_dwLastTimeAttackedPVP = get_dword_time();
		}
#endif
	}
	else
	{
		MonsterChat(MONSTER_CHAT_ATTACK);
	}


	int iRet;

	if (bType == 0)
	{
		std::vector<LPCHARACTER> vec_pkVictimList;
		vec_pkVictimList.push_back(pkVictim);

#ifdef __FAKE_PC__
		if (FakePC_Check())
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);
			if (pkWeapon && pkWeapon->GetSubType() != WEAPON_ARROW && pkWeapon->GetSubType() != WEAPON_BOW)
			{
				FCollectFakePCVictimList f(this, vec_pkVictimList);
				GetSectree()->ForEachAround(f);
			}
		}
#endif

		//
		// ÀÏ¹Ý °ø°Ý
		//
		for (itertype(vec_pkVictimList) it = vec_pkVictimList.begin(); it != vec_pkVictimList.end(); ++it)
		{
			LPCHARACTER pkCurVictim = *it;

			switch (GetMobBattleType())
			{
				case BATTLE_TYPE_MELEE:
				case BATTLE_TYPE_POWER:
				case BATTLE_TYPE_TANKER:
				case BATTLE_TYPE_SUPER_POWER:
				case BATTLE_TYPE_SUPER_TANKER:
					if (pkCurVictim == pkVictim)
						iRet = battle_melee_attack(this, pkCurVictim);
					else
						battle_melee_attack(this, pkCurVictim);
					break;

				case BATTLE_TYPE_RANGE:
					FlyTarget(pkCurVictim->GetVID(), pkCurVictim->GetX(), pkCurVictim->GetY(), false);
					if (pkCurVictim == pkVictim)
						iRet = Shoot(0) ? BATTLE_DAMAGE : BATTLE_NONE;
					else
						Shoot(0);
					break;

				case BATTLE_TYPE_MAGIC:
					FlyTarget(pkCurVictim->GetVID(), pkCurVictim->GetX(), pkCurVictim->GetY(), false);
					if (pkCurVictim == pkVictim)
						iRet = Shoot(1) ? BATTLE_DAMAGE : BATTLE_NONE;
					else
						Shoot(1);
					break;

				default:
					sys_err("Unhandled battle type %d", GetMobBattleType());
					iRet = BATTLE_NONE;
					break;
			}
		}
	}
	else
	{
		if (IsPC() == true)
		{
			if (dwCurrentTime - m_dwLastSkillTime > 1500)
			{
				sys_log(1, "HACK: Too long skill using term. Name(%s) PID(%u) delta(%u)",
						GetName(), GetPlayerID(), (dwCurrentTime - m_dwLastSkillTime));
				return false;
			}
		}

		sys_log(1, "Attack call ComputeSkill %d %s", bType, pkVictim?pkVictim->GetName():"");
		iRet = ComputeSkill(bType, pkVictim);
	}

	if(iRet != BATTLE_NONE)
	{
		pkVictim->SetSyncOwner(this);

		if (pkVictim->CanBeginFight())
			pkVictim->BeginFight(this);
	}
	
	//if (test_server && IsPC())
	//	sys_log(0, "%s Attack %s type %u ret %d", GetName(), pkVictim->GetName(), bType, iRet);
	if (iRet == BATTLE_DAMAGE || iRet == BATTLE_DEAD)
	{
		OnMove(true, pkVictim->IsPC());
		pkVictim->OnMove(false, IsPC());

		// only pc sets victim null. For npc, state machine will reset this.
		if (BATTLE_DEAD == iRet && IsPC())
			SetVictim(NULL);
#ifdef __FAKE_PC__
		else if (IsPC() && !pkVictim->IsPC())
			FakePC_Owner_ForceFocus(pkVictim);
#endif

		return true;
	}

	return false;
}

void CHARACTER::DeathPenalty(BYTE bTown)
{
	sys_log(1, "DEATH_PERNALY_CHECK(%s) town(%d)", GetName(), bTown);

	Cube_close(this);
#ifdef __ACCE_COSTUME__
	AcceClose();
#endif

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif
	
	if (GetLevel() < 10)
	{
		sys_log(0, "NO_DEATH_PENALTY_LESS_LV10(%s)", GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿ë½ÅÀÇ °¡È£·Î °æÇèÄ¡°¡ ¶³¾îÁöÁö ¾Ê¾Ò½À´Ï´Ù."));
		return;
	}

   	if (random_number(0, 2))
	{
		sys_log(0, "NO_DEATH_PENALTY_LUCK(%s)", GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿ë½ÅÀÇ °¡È£·Î °æÇèÄ¡°¡ ¶³¾îÁöÁö ¾Ê¾Ò½À´Ï´Ù."));
		return;
	}

	if (IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY))
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);

		// NO_DEATH_PENALTY_BUG_FIX 
		if (!bTown) // ±¹Á¦ ¹öÀü¿¡¼­´Â Á¦ÀÚ¸® ºÎÈ°½Ã¸¸ ¿ë½ÅÀÇ °¡È£¸¦ »ç¿ëÇÑ´Ù. (¸¶À» º¹±Í½Ã´Â °æÇèÄ¡ ÆÐ³ÎÆ¼ ¾øÀ½)
		{
			if (FindAffect(AFFECT_NO_DEATH_PENALTY))
			{
				sys_log(0, "NO_DEATH_PENALTY_AFFECT(%s)", GetName());
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿ë½ÅÀÇ °¡È£·Î °æÇèÄ¡°¡ ¶³¾îÁöÁö ¾Ê¾Ò½À´Ï´Ù."));
				RemoveAffect(AFFECT_NO_DEATH_PENALTY);
				return;
			}
		}
		// END_OF_NO_DEATH_PENALTY_BUG_FIX

		int iLoss = ((GetNextExp() * aiExpLossPercents[MINMAX(1, GetLevel(), PLAYER_EXP_TABLE_MAX)]) / 100);
		iLoss = MIN(800000, iLoss);

		if (bTown)
			iLoss = 0;

		if (IsEquipUniqueItem(UNIQUE_ITEM_TEARDROP_OF_GODNESS))
			iLoss /= 2;

#ifdef __PRESTIGE__
		sys_log(0, "DEATH_PENALTY(%s) EXP_LOSS: %d percent %d%%", GetName(), iLoss, aiExpLossPercents[MIN(gPlayerMaxLevel[GetPrestigeLevel()], GetLevel())]);
#else
		sys_log(0, "DEATH_PENALTY(%s) EXP_LOSS: %d percent %d%%", GetName(), iLoss, aiExpLossPercents[MIN(gPlayerMaxLevel, GetLevel())]);
#endif

		PointChange(POINT_EXP, -iLoss, true);
	}
}

bool CHARACTER::IsStun() const
{
	if (IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN))
		return true;

	return false;
}

EVENTFUNC(StunEvent)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "StunEvent> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkStunEvent = NULL;
	ch->Dead();
	return 0;
}

void CHARACTER::Stun()
{
	if (IsStun())
		return;

	if (IsDead())
		return;

	if (!IsPC() && m_pkParty)
	{
		m_pkParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);
	}

	sys_log(1, "%s: Stun %p", GetName(), this);

	PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
	PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));

	CloseMyShop();

	event_cancel(&m_pkRecoveryEvent); // È¸º¹ ÀÌº¥Æ®¸¦ Á×ÀÎ´Ù.

	network::GCOutputPacket<network::GCStunPacket> pack;
	pack->set_vid(m_vid);
	PacketAround(pack);

	SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

	if (m_pkStunEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkStunEvent = event_create(StunEvent, info, PASSES_PER_SEC(3));
}

EVENTINFO(SCharDeadEventInfo)
{
	bool isPC;
	uint32_t dwID;

	SCharDeadEventInfo()
	: isPC(0)
	, dwID(0)
	{
	}
};

EVENTFUNC(dead_event)
{
	const SCharDeadEventInfo* info = dynamic_cast<SCharDeadEventInfo*>(event->info);

	if ( info == NULL )
	{
		sys_err( "dead_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = NULL;

	// if (test_server)	sys_err("%s:%d", __FILE__, __LINE__);
	
	if (true == info->isPC)
	{
		ch = CHARACTER_MANAGER::instance().FindByPID( info->dwID );
	}
	else
	{
		ch = CHARACTER_MANAGER::instance().Find( info->dwID );
	}

	if (NULL == ch)
	{
		sys_err("DEAD_EVENT: cannot find char pointer with %s id(%d)", info->isPC ? "PC" : "MOB", info->dwID );
		return 0;
	}

	ch->m_pkDeadEvent = NULL;

	// if (test_server)	sys_err("%s:%d", __FILE__, __LINE__);
	if (ch->GetDesc())
	{
		if (ch->GetMapIndex() == EVENT_LABYRINTH_MAP_INDEX || ch->IsPrivateMap(EVENT_LABYRINTH_MAP_INDEX))
			return 0;
		
		// if (test_server)	sys_err("%s:%d", __FILE__, __LINE__);
		ch->GetDesc()->SetPhase(PHASE_GAME);

		ch->SetPosition(POS_STANDING);
		// if (test_server)	sys_err("%s:%d", __FILE__, __LINE__);

		PIXEL_POSITION pos;

		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
			ch->WarpSet(pos.x, pos.y);
		else
		{
			sys_err("cannot find spawn position (name %s)", ch->GetName());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
		// if (test_server)	sys_err("%s:%d", __FILE__, __LINE__);

		ch->PointChange(POINT_HP, (ch->GetMaxHP() / 2) - ch->GetHP(), true);

		ch->DeathPenalty(0);

		ch->StartRecoveryEvent();

		ch->ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
		// if (test_server)	sys_err("%s:%d", __FILE__, __LINE__);
	}
	else
	{
		if (ch->IsMonster() == true)
		{
			if (ch->IsRevive() == false && ch->HasReviverInParty() == true)
			{
				ch->SetPosition(POS_STANDING);
				ch->SetHP(ch->GetMaxHP());

				ch->ViewReencode();

				ch->SetAggressive();
				ch->SetRevive(true);

				return 0;
			}
		}

		M2_DESTROY_CHARACTER(ch);
	}

	return 0;
}

bool CHARACTER::IsDead() const
{
	if (m_pointsInstant.position == POS_DEAD)
		return true;

	return false;
}

void CHARACTER::RewardGold(LPCHARACTER pkAttacker)
{
	// ADD_PREMIUM
#ifdef __ALL_AUTO_LOOT__
	bool isAutoLoot = true;
#else
	bool isAutoLoot = 
		(pkAttacker->GetPremiumRemainSeconds(PREMIUM_AUTOLOOT) > 0 ||
		 pkAttacker->IsEquipUniqueGroup(UNIQUE_GROUP_AUTOLOOT))
		? true : false; // Á¦3ÀÇ ¼Õ
#endif
	// END_OF_ADD_PREMIUM

	PIXEL_POSITION pos;

	if (!isAutoLoot)
		if (!SECTREE_MANAGER::instance().GetMovablePosition(GetMapIndex(), GetX(), GetY(), pos))
			return;

	int iTotalGold = 0;
	//
	// --------- µ· µå·Ó È®·ü °è»ê ----------
	//
	int iGoldPercent = MobRankStats[GetMobRank()].iGoldPercent;

	if (pkAttacker->IsPC())
#ifdef __FAKE_PRIV_BONI__
		iGoldPercent = iGoldPercent * (100 + (CPrivManager::instance().GetPriv(pkAttacker, PRIV_GOLD_DROP) / __FAKE_PRIV_BONI__)) / 100;
#else
		iGoldPercent = iGoldPercent * (100 + CPrivManager::instance().GetPriv(pkAttacker, PRIV_GOLD_DROP)) / 100;
#endif

	if (pkAttacker->GetPoint(POINT_MALL_GOLDBONUS))
		iGoldPercent += (iGoldPercent * pkAttacker->GetPoint(POINT_MALL_GOLDBONUS) / 100);

	iGoldPercent = iGoldPercent * CHARACTER_MANAGER::instance().GetMobGoldDropRate(pkAttacker) / 100;

	// ADD_PREMIUM
	if (pkAttacker->GetPremiumRemainSeconds(PREMIUM_GOLD) > 0 ||
			pkAttacker->IsEquipUniqueGroup(UNIQUE_GROUP_LUCKY_GOLD))
		iGoldPercent += iGoldPercent;
	// END_OF_ADD_PREMIUM

	if (iGoldPercent > 100) 
		iGoldPercent = 100;

	int iPercent;

	if (GetMobRank() >= MOB_RANK_BOSS)
		iPercent = ((iGoldPercent * PERCENT_LVDELTA_BOSS(pkAttacker->GetLevel(), GetLevel())) / 100);
	else
		iPercent = ((iGoldPercent * PERCENT_LVDELTA(pkAttacker->GetLevel(), GetLevel())) / 100);
	//int iPercent = CALCULATE_VALUE_LVDELTA(pkAttacker->GetLevel(), GetLevel(), iGoldPercent);

	if (random_number(1, 100) > iPercent)
		return;

	int iGoldMultipler = 1;

	if (1 == random_number(1, 50000)) // 1/50000 È®·ü·Î µ·ÀÌ 10¹è
		iGoldMultipler *= 10;
	else if (1 == random_number(1, 10000)) // 1/10000 È®·ü·Î µ·ÀÌ 5¹è
		iGoldMultipler *= 5;

	// °³ÀÎ Àû¿ë
	if (pkAttacker->GetPoint(POINT_GOLD_DOUBLE_BONUS))
		if (random_number(1, 100) <= pkAttacker->GetPoint(POINT_GOLD_DOUBLE_BONUS))
			iGoldMultipler *= 2;

	//
	// --------- µ· µå·Ó ¹è¼ö °áÁ¤ ----------
	//
	// if (test_server)
	// 	pkAttacker->ChatPacket(CHAT_TYPE_PARTY, "gold_mul %d rate %d", iGoldMultipler, CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker));

	//
	// --------- ½ÇÁ¦ µå·Ó Ã³¸® -------------
	// 
	LPITEM item;

	int iGold10DropPct = 100;
#ifdef __FAKE_PRIV_BONI__
	iGold10DropPct = (iGold10DropPct * 100) / (100 + (CPrivManager::instance().GetPriv(pkAttacker, PRIV_GOLD10_DROP) / __FAKE_PRIV_BONI__ ));
#else
	iGold10DropPct = (iGold10DropPct * 100) / (100 + CPrivManager::instance().GetPriv(pkAttacker, PRIV_GOLD10_DROP));
#endif

	// MOB_RANK°¡ BOSSº¸´Ù ³ôÀ¸¸é ¹«Á¶°Ç µ·ÆøÅº
	if (GetMobRank() >= MOB_RANK_BOSS && !IsStone() && GetMobTable().gold_max() != 0)
	{
		if (1 == random_number(1, iGold10DropPct))
			iGoldMultipler *= 10; // 1% È®·ü·Î µ· 10¹è

		int iSplitCount = random_number(25, 35);

		for (int i = 0; i < iSplitCount; ++i)
		{
			int iGold = random_number(GetMobTable().gold_min(), GetMobTable().gold_max()) / iSplitCount;
			if (test_server)
				sys_log(0, "iGold %d", iGold);
			iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker) / 100;
			iGold *= iGoldMultipler;

			if (iGold == 0)
			{
				continue ;
			}

			if (test_server)
			{
				sys_log(0, "Drop Moeny MobGoldAmountRate %d %d", CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker), iGoldMultipler);
				sys_log(0, "Drop Money gold %d GoldMin %d GoldMax %d", iGold, GetMobTable().gold_min(), GetMobTable().gold_max());
			}

			// NOTE: µ· ÆøÅºÀº Á¦ 3ÀÇ ¼Õ Ã³¸®¸¦ ÇÏÁö ¾ÊÀ½
			if ((item = ITEM_MANAGER::instance().CreateItem(1, iGold)))
			{
				pos.x = GetX() + ((random_number(-14, 14) + random_number(-14, 14)) * 23);
				pos.y = GetY() + ((random_number(-14, 14) + random_number(-14, 14)) * 23);

				item->AddToGround(GetMapIndex(), pos);
				item->StartDestroyEvent();

				iTotalGold += iGold; // Total gold
			}
		}
	}
	else if (1 == random_number(1, iGold10DropPct))
	{
		for (int i = 0; i < 10; ++i)
		{
			int iGold = random_number(GetMobTable().gold_min(), GetMobTable().gold_max());
			iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker) / 100;
			iGold *= iGoldMultipler;

			if (iGold == 0)
				continue;

			if ((item = ITEM_MANAGER::instance().CreateItem(1, iGold)))
			{
				pos.x = GetX() + (random_number(-7, 7) * 20);
				pos.y = GetY() + (random_number(-7, 7) * 20);

				item->AddToGround(GetMapIndex(), pos);
				item->StartDestroyEvent();

				iTotalGold += iGold; // Total gold
			}
		}
	}
	else
	{
		int iGold = random_number(GetMobTable().gold_min(), GetMobTable().gold_max());
		
		float fGoldFactor = 1.0f;
		#ifdef AELDRA	
		if (GetLevel() >= 1 && GetLevel() <= 30)
			fGoldFactor = 2.5;
        else if (GetLevel() >= 31 && GetLevel() <= 55)
            fGoldFactor = 1.8;
        else if (GetLevel() >= 56 && GetLevel() <= 75)
            fGoldFactor = 2.0;
		iGold = abs(iGold * fGoldFactor);
			
#elif defined(ELONIA)
		if (GetLevel() >= 1 && GetLevel() <= 105)
			fGoldFactor = 1.0;
#endif
		iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(pkAttacker) / 100;
		iGold *= iGoldMultipler;

		int iSplitCount;

		if (iGold >= 3) 
			iSplitCount = random_number(1, 3);
		else if (GetMobRank() >= MOB_RANK_BOSS)
		{
			iSplitCount = random_number(3, 10);

			if ((iGold / iSplitCount) == 0)
				iSplitCount = 1;
		}
		else
			iSplitCount = 1;

		if (iGold != 0)
		{
			iTotalGold += iGold; // Total gold

			for (int i = 0; i < iSplitCount; ++i)
			{
				if (isAutoLoot)
				{
					pkAttacker->GiveGold(iGold / iSplitCount);
				}
				else if ((item = ITEM_MANAGER::instance().CreateItem(1, iGold / iSplitCount)))
				{
					pos.x = GetX() + (random_number(-7, 7) * 20);
					pos.y = GetY() + (random_number(-7, 7) * 20);

					item->AddToGround(GetMapIndex(), pos);
					item->StartDestroyEvent();
				}
			}
		}
	}

	quest::CQuestManager::Instance().CollectYangFromMonster(pkAttacker->GetPlayerID(), iTotalGold);

	LogManager::instance().MoneyLog(MONEY_LOG_MONSTER, GetRaceNum(), iTotalGold);
}

typedef struct {
	LPITEM item;
	bool bAutoPickup;
} item_and_pickup_info;

void CHARACTER::Reward(bool bItemDrop)
{
	DWORD dwThisRaceNum = GetRaceNum();
	PIXEL_POSITION pos;
	pos.x = GetX();
	pos.y = GetY();
	pos.z = GetZ();
	long lMapIndex = GetMapIndex();
	long mX = GetX();
	long mY = GetY();

	char mName[CHARACTER_NAME_MAX_LEN + 1];
	strlcpy(mName, GetName(), sizeof(mName));
	
	if (dwThisRaceNum == 5001) // ¿Ö±¸´Â µ·À» ¹«Á¶°Ç µå·Ó
	{
		PIXEL_POSITION pos;

		if (!SECTREE_MANAGER::instance().GetMovablePosition(lMapIndex, mX, mY, pos))
			return;

		LPITEM item;
		int iGold = random_number(GetMobTable().gold_min(), GetMobTable().gold_max());
		iGold = iGold * CHARACTER_MANAGER::instance().GetMobGoldAmountRate(NULL) / 100;
		int iSplitCount = random_number(25, 35);

		sys_log(0, "WAEGU Dead gold %d split %d", iGold, iSplitCount);

		for (int i = 1; i <= iSplitCount; ++i)
		{
			if ((item = ITEM_MANAGER::instance().CreateItem(1, iGold / iSplitCount)))
			{
				if (i != 0)
				{
					pos.x = random_number(-7, 7) * 20;
					pos.y = random_number(-7, 7) * 20;

					pos.x += mX;
					pos.y += mY;
				}

				item->AddToGround(lMapIndex, pos);
				item->StartDestroyEvent();
			}
		}
		return;
	}

	if (GetDungeon())
		GetDungeon()->DecAliveMonster();

	//PROF_UNIT puReward("Reward");
   	LPCHARACTER pkAttacker = DistributeExp();

	if (!pkAttacker)
		return;


	static std::vector<LPITEM> s_vec_item;
	static std::vector<LPITEM> s_vec_item_auto_pickup;
	s_vec_item.clear();
	s_vec_item_auto_pickup.clear();
	bool bCreateDropItem = ITEM_MANAGER::instance().CreateDropItem(this, pkAttacker, s_vec_item, s_vec_item_auto_pickup);

	//PROF_UNIT pu1("r1");
	
	//pu1.Pop();

	if (!bItemDrop || !SECTREE_MANAGER::instance().GetMovablePosition(lMapIndex, pos.x, pos.y, pos))
	{
		if (pkAttacker->IsPC())
		{
			if (GetLevel() - pkAttacker->GetLevel() >= -10)
				if (pkAttacker->GetRealAlignment() < 0)
				{
					if (pkAttacker->IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_KILL))
						pkAttacker->UpdateAlignment(14);
					else
						pkAttacker->UpdateAlignment(7);
				}
				else
					pkAttacker->UpdateAlignment(2);

			pkAttacker->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().Kill(pkAttacker->GetPlayerID(), dwThisRaceNum);
			CHARACTER_MANAGER::instance().KillLog(dwThisRaceNum);

			if (!random_number(0, 9))
			{
				if (pkAttacker->GetPoint(POINT_KILL_HP_RECOVERY))
				{
					int iHP = pkAttacker->GetMaxHP() * pkAttacker->GetPoint(POINT_KILL_HP_RECOVERY) / 100;
					pkAttacker->PointChange(POINT_HP, iHP);
					CreateFly(FLY_HP_SMALL, pkAttacker);
				}

				if (pkAttacker->GetPoint(POINT_KILL_SP_RECOVER))
				{
					int iSP = pkAttacker->GetMaxSP() * pkAttacker->GetPoint(POINT_KILL_SP_RECOVER) / 100;
					pkAttacker->PointChange(POINT_SP, iSP);
					CreateFly(FLY_SP_SMALL, pkAttacker);
				}
			}
		}

		return;
	}

	// Attacker is in dungeon
	if (pkAttacker->GetDungeon() && quest::CQuestManager::Instance().GetEventFlag("kill_trigger_fix") != 0)
	{
		quest::PC* pc = quest::CQuestManager::Instance().GetPCForce(pkAttacker->GetPlayerID());

		if (!pc || !quest::CQuestManager::Instance().CheckQuestLoaded(pc))
			return;
		
		if (pc->IsRunning())
			return;
	}

	//
	// µ· µå·Ó
	//
	//PROF_UNIT pu2("r2");
	if (test_server)
		sys_log(0, "Drop money : Attacker %s", pkAttacker->GetName());
	RewardGold(pkAttacker);
	//pu2.Pop();

	//
	// ¾ÆÀÌÅÛ µå·Ó
	//
	//PROF_UNIT pu3("r3");

	// Hack Detection No Spacebars
	// if (pkAttacker->GetLastSpacebarTime() + 120 < get_global_time())
	// {
		// pkAttacker->DetectionHackLog("FISHBOT", "NO_SPACEKEY");
		// pkAttacker->tchat("HitSpacebar check FAILED BUSTED");
	// }
	
	LPITEM item;
	if (bCreateDropItem)
	{
		if (s_vec_item.size() == 0 && s_vec_item_auto_pickup.size() == 0);
		else if (s_vec_item.size() + s_vec_item_auto_pickup.size() == 1)
		{
			bool bAutoPickup = s_vec_item.size() == 0;
			if (bAutoPickup)
			{
				item = s_vec_item_auto_pickup[0];
				sys_log(0, "DROP_ITEM: %s %d %d from %s is_auto_pickup %d", item->GetName(), pos.x, pos.y, mName, bAutoPickup);
				if (item->GetVnum() == 50128)
				{
					pkAttacker->SetItemDropQuest(item->GetVnum());
					pkAttacker->ChatPacket(CHAT_TYPE_COMMAND, "ITEM_QUEST_DROP");
				}
				pkAttacker->AutoGiveItem(item, true);
			}
			else
			{
				item = s_vec_item[0];
				sys_log(0, "DROP_ITEM: %s %d %d from %s is_auto_pickup %d", item->GetName(), pos.x, pos.y, mName, bAutoPickup);
			}

			if (!bAutoPickup)
			{
				item->AddToGround(lMapIndex, pos);

				item->SetOwnership(pkAttacker);

				item->StartDestroyEvent(60);

				pos.x = random_number(-7, 7) * 20;
				pos.y = random_number(-7, 7) * 20;
				pos.x += mX;
				pos.y += mY;
			}
		}
		else
		{
#ifdef __NEW_DROP_SYSTEM__

			std::random_device random_device;
			std::mt19937 random_gen(random_device());

			auto GetRandomUint64 = [&](std::uint64_t min, std::uint64_t max)
			{
				std::uniform_int_distribution<std::uint64_t> uid(min, max);
				return uid(random_gen);
			};

			// copy all items into one vector
			static std::vector<item_and_pickup_info> s_vec_full_item;

			s_vec_full_item.clear();
			s_vec_full_item.resize(s_vec_item.size() + s_vec_item_auto_pickup.size());
			
			for(int i = 0; i < s_vec_item.size(); ++i)
			{
				s_vec_full_item[ i ].item = s_vec_item[ i ];
				s_vec_full_item[ i ].bAutoPickup = false;
			}

			for(int i = 0; i < s_vec_item_auto_pickup.size(); ++i)
			{
				s_vec_full_item[ s_vec_item.size() + i ].item = s_vec_item_auto_pickup[ i ];
				s_vec_full_item[ s_vec_item.size() + i ].bAutoPickup = true;
			}

			int iItemIdx = s_vec_full_item.size() - 1;

			std::uint64_t totalDamage = 0;
			std::uint64_t highestDamage = 0;

			std::vector<std::pair<LPCHARACTER, int>> v_players{ };

			// Add players to list
			for(auto& damageIt : m_map_kDamage)
			{
				int damageDone = damageIt.second.iTotalDamage;

#ifdef __FAKE_PC__
				playerDamage += damageIt.second.iTotalFakePCDamage;
#endif

				if(damageDone <= 0)
					continue;

				LPCHARACTER lpCharacter = CHARACTER_MANAGER::instance().Find(damageIt.first);
			
				if(!lpCharacter)
					continue;

				if(damageDone > highestDamage)
					highestDamage = damageDone;

				totalDamage += damageDone;

				v_players.push_back(std::make_pair(lpCharacter, damageDone));
			}

			std::uint64_t damageTreshold = ( highestDamage * 20 ) / 100;

			sys_log(!test_server, "----------------------------------------------------------------");

			for(auto & player : v_players)
			{
				float damagePercent = ( float( player.second ) / totalDamage ) * 100.f;
				sys_log(!test_server, "NEW_DROP_SYSTEM: Player %s done %.1f%% of damage to the mob.", player.first->GetName(), damagePercent);
			}

			// Used only for logging, can remove after
			float damageTresholdPerc = ( float( damageTreshold ) / totalDamage ) * 100.f;

			sys_log(!test_server, "NEW_DROP_SYSTEM: Minimum damage to get drop: %d (~%.2f%% of total damage)", damageTreshold, damageTresholdPerc);

			v_players.erase( std::remove_if(v_players.begin(), v_players.end(), [&](const std::pair<LPCHARACTER,int>& playerInfo)
			{
				bool remove = ( playerInfo.second < damageTreshold );

				if(remove)
				{
					sys_log(!test_server, "NEW_DROP_SYSTEM: Removed %s from getting drop, too low damage.", playerInfo.first->GetName(), playerInfo.second);
					totalDamage -= playerInfo.second;
				}

				return remove;
			}), v_players.end());

			// Sort by damage
			std::sort(v_players.begin(), v_players.end(), [](const std::pair<LPCHARACTER, int>& player_a, const std::pair<LPCHARACTER, int>& player_b)
			{
				return player_a.second > player_b.second;
			});

			// Create ticket vector
			// LPCHARACTER:range{min,max}
			std::vector<std::pair<LPCHARACTER, std::pair<std::uint64_t, std::uint64_t>>> v_tickets{ };

			std::uint64_t range_start = 1;

			for(auto & player : v_players)
			{
				std::uint64_t start = range_start;
				std::uint64_t end = range_start + player.second - 1;

				v_tickets.push_back(std::make_pair(player.first, std::make_pair(start, end)));

				range_start = end + 1;
			}

			// Used only for logging
			std::vector<std::pair<LPCHARACTER, int>> v_itemdropcount;

			if(!v_tickets.empty())
			{
				while(iItemIdx >= 0)
				{
					item_and_pickup_info& r_info = s_vec_full_item[ iItemIdx-- ];
					item = r_info.item;

					if(!item)
						continue;

					item->AddToGround(lMapIndex, pos);

					std::uint64_t winning_number = GetRandomUint64(1, totalDamage);

					auto winner = std::find_if(v_tickets.begin(), v_tickets.end(), [winning_number](const std::pair<LPCHARACTER, std::pair<std::uint64_t, std::uint64_t>>& ticket_entry)
					{
						return winning_number >= ticket_entry.second.first && winning_number <= ticket_entry.second.second;
					});

					LPCHARACTER ch = ( *winner ).first;
					 
					// LOGGING
					auto it = std::find_if(v_itemdropcount.begin(), v_itemdropcount.end(), [&](const std::pair<LPCHARACTER,int>& itemDropLog)
					{
						return itemDropLog.first == ch;
					});

					if(it != v_itemdropcount.end())
						( *it ).second++;
					else v_itemdropcount.push_back(std::make_pair(ch, 1));
					// ---

#ifndef NO_PARTY_DROP
					if(ch->GetParty())
						ch = ch->GetParty()->GetNextOwnership(ch, mX, mY);
#endif
					item->SetOwnership(ch);

					item->StartDestroyEvent();

					pos.x = random_number(-7, 7) * 20;
					pos.y = random_number(-7, 7) * 20;
					pos.x += mX;
					pos.y += mY;

					if(item->GetVnum() == 50128)
					{
						ch->SetItemDropQuest(item->GetVnum());
						ch->ChatPacket(CHAT_TYPE_COMMAND, "ITEM_QUEST_DROP");
					}
				}
			}

			for(auto & itemDropLog : v_itemdropcount)
				sys_log(!test_server, "NEW_DROP_SYSTEM: Player %s dropped %d items.", itemDropLog.first->GetName(), itemDropLog.second);

			sys_log(!test_server, "----------------------------------------------------------------");

			v_players.clear();
			v_tickets.clear();
			v_itemdropcount.clear();
#else
			// copy all items into one vector
			static std::vector<item_and_pickup_info> s_vec_full_item;
			s_vec_full_item.clear();

			s_vec_full_item.resize(s_vec_item.size() + s_vec_item_auto_pickup.size());
			for (int i = 0; i < s_vec_item.size(); ++i)
			{
				s_vec_full_item[i].item = s_vec_item[i];
				s_vec_full_item[i].bAutoPickup = false;
			}
			for (int i = 0; i < s_vec_item_auto_pickup.size(); ++i)
			{
				s_vec_full_item[s_vec_item.size() + i].item = s_vec_item_auto_pickup[i];
				s_vec_full_item[s_vec_item.size() + i].bAutoPickup = true;
			}
			
			int iItemIdx = s_vec_full_item.size() - 1;

			std::priority_queue<std::pair<int, LPCHARACTER> > pq;

			std::uint64_t total_dam = 0;

			for (TDamageMap::iterator it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
			{
				int iDamage = it->second.iTotalDamage;
#ifdef __FAKE_PC__
				iDamage += it->second.iTotalFakePCDamage;
#endif
				if (iDamage > 0)
				{
					LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(it->first);

					if (ch)
					{
						pq.push(std::make_pair(iDamage, ch));
						total_dam += iDamage;
					}
				}
			}

			std::vector<LPCHARACTER> v;

			while (!pq.empty() && pq.top().first * 10 >= total_dam)
			{
				v.push_back(pq.top().second);
				pq.pop();
			}

			if (!v.empty())
			{
				// µ¥¹ÌÁö ¸¹ÀÌ ÁØ »ç¶÷µé ³¢¸®¸¸ ¼ÒÀ¯±Ç ³ª´²°¡Áü
				std::vector<LPCHARACTER>::iterator it = v.begin();

				while (iItemIdx >= 0)
				{
					item_and_pickup_info& r_info = s_vec_full_item[iItemIdx--];
					item = r_info.item;

					if (!item)
					{
						sys_err("item null in vector idx %d", iItemIdx + 1);
						continue;
					}

					item->AddToGround(lMapIndex, pos);

					LPCHARACTER ch = *it;

#ifndef NO_PARTY_DROP
					if (ch->GetParty())
						ch = ch->GetParty()->GetNextOwnership(ch, mX, mY);
#endif
					++it;

					if (it == v.end())
						it = v.begin();

					item->SetOwnership(ch);

					item->StartDestroyEvent();

					pos.x = random_number(-7, 7) * 20;
					pos.y = random_number(-7, 7) * 20;
					pos.x += mX;
					pos.y += mY;
					
					if (item->GetVnum() == 50128)
					{
						ch->SetItemDropQuest(item->GetVnum());
						ch->ChatPacket(CHAT_TYPE_COMMAND, "ITEM_QUEST_DROP");
					}

					sys_log(0, "DROP_ITEM: %s %dx %d %d by %s", item->GetName(), item->GetCount(), pos.x, pos.y, mName);
				}
			}
#endif
		}
	}

	m_map_kDamage.clear();

	if (pkAttacker->IsPC())
	{
		if (GetLevel() - pkAttacker->GetLevel() >= -10)
			if (pkAttacker->GetRealAlignment() < 0)
			{
				if (pkAttacker->IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_KILL))
					pkAttacker->UpdateAlignment(14);
				else
					pkAttacker->UpdateAlignment(7);
			}
			else
				pkAttacker->UpdateAlignment(2);

		pkAttacker->SetQuestNPCID(GetVID());
		quest::CQuestManager::instance().Kill(pkAttacker->GetPlayerID(), dwThisRaceNum);
		CHARACTER_MANAGER::instance().KillLog(dwThisRaceNum);
		
		if (!random_number(0, 9))
		{
			if (pkAttacker->GetPoint(POINT_KILL_HP_RECOVERY))
			{
				int iHP = pkAttacker->GetMaxHP() * pkAttacker->GetPoint(POINT_KILL_HP_RECOVERY) / 100;
				pkAttacker->PointChange(POINT_HP, iHP);
				CreateFly(FLY_HP_SMALL, pkAttacker);
			}

			if (pkAttacker->GetPoint(POINT_KILL_SP_RECOVER))
			{
				int iSP = pkAttacker->GetMaxSP() * pkAttacker->GetPoint(POINT_KILL_SP_RECOVER) / 100;
				pkAttacker->PointChange(POINT_SP, iSP);
				CreateFly(FLY_SP_SMALL, pkAttacker);
			}
		}
	}
}

struct TItemDropPenalty
{
	int iInventoryPct;		// Range: 1 ~ 1000
	int iInventoryQty;		// Range: --
	int iEquipmentPct;		// Range: 1 ~ 100
	int iEquipmentQty;		// Range: --
};

TItemDropPenalty aItemDropPenalty_kor[9] =
{
	{   0,   0,  0,  0 },	// 선왕
	{   0,   0,  0,  0 },	// 영웅
	{   0,   0,  0,  0 },	// 성자
	{   0,   0,  0,  0 },	// 지인
	{   0,   0,  0,  0 },	// 양민
	{  25,   1,  5,  1 },	// 낭인
	{  50,   2, 10,  1 },	// 악인
	{  75,   4, 15,  1 },	// 마두
	{ 100,   8, 20,  1 },	// 패왕
};

void CHARACTER::ItemDropPenalty(LPCHARACTER pkKiller)
{
	// °³ÀÎ»óÁ¡À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» µå·ÓÇÏÁö ¾Ê´Â´Ù.
	if (GetMyShop())
		return;

	if (GetLevel() < 50)
		return;

	if (pvp_server)
		return;
	
	struct TItemDropPenalty * table = &aItemDropPenalty_kor[0];

	if (GetLevel() < 10)
		return;

	int iAlignIndex;

	if (GetRealAlignment() >= 120000)
		iAlignIndex = 0;
	else if (GetRealAlignment() >= 80000)
		iAlignIndex = 1;
	else if (GetRealAlignment() >= 40000)
		iAlignIndex = 2;
	else if (GetRealAlignment() >= 10000)
		iAlignIndex = 3;
	else if (GetRealAlignment() >= 0)
		iAlignIndex = 4;
	else if (GetRealAlignment() > -40000)
		iAlignIndex = 5;
	else if (GetRealAlignment() > -80000)
		iAlignIndex = 6;
	else if (GetRealAlignment() > -120000)
		iAlignIndex = 7;
	else
		iAlignIndex = 8;
	
	if (test_server)
		iAlignIndex = 8;

	std::vector<std::pair<LPITEM, int> > vec_item;
	LPITEM pkItem;
	int	i;
	bool isDropAllEquipments = false;

	TItemDropPenalty & r = table[iAlignIndex];
	sys_log(0, "%s align %d inven_pct %d equip_pct %d", GetName(), iAlignIndex, r.iInventoryPct, r.iEquipmentPct);

	bool bDropInventory = test_server || r.iInventoryPct >= random_number(1, 1000);
	bool bDropEquipment = test_server || r.iEquipmentPct >= random_number(1, 100);
	bool bDropAntiDropUniqueItem = false;

	if ((bDropInventory || bDropEquipment) && IsEquipUniqueItem(UNIQUE_ITEM_SKIP_ITEM_DROP_PENALTY) && iAlignIndex < ALIGN_CRAZY)
	{
		bDropInventory = false;
		bDropEquipment = false;
		bDropAntiDropUniqueItem = true;
	}

	if (bDropInventory) // Drop Inventory
	{
		std::vector<WORD> vec_wSlots;

		for (i = 0; i < EQUIPMENT_SLOT_START; ++i)
			if (GetInventoryItem(i))
				vec_wSlots.push_back(i);

		if (!vec_wSlots.empty())
		{
			random_shuffle(vec_wSlots.begin(), vec_wSlots.end());

			int iQty = MIN(vec_wSlots.size(), r.iInventoryQty);

			if (iQty)
				iQty = random_number(1, iQty);

			for (i = 0; i < iQty; ++i)
			{
				pkItem = GetInventoryItem(vec_wSlots[i]);

				if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_PKDROP) || pkItem->IsGMOwner())
					continue;
				
#ifdef __TRADE_BLOCK_SYSTEM__
				if (IsTradeBlocked())
					break;
#endif

				SyncQuickslot(QUICKSLOT_TYPE_ITEM, vec_wSlots[i], 255);
				vec_item.push_back(std::make_pair(pkItem->RemoveFromCharacter(), INVENTORY));
			}
		}
		else if (iAlignIndex == 8)
			isDropAllEquipments = true;
	}

	if (bDropEquipment) // Drop Equipment
	{
		std::vector<BYTE> vec_bSlots;

		for (i = 0; i < WEAR_MAX_NUM; ++i)
			if (GetWear(i))
				vec_bSlots.push_back(i);

		if (!vec_bSlots.empty())
		{
			random_shuffle(vec_bSlots.begin(), vec_bSlots.end());
			int iQty;

			if (isDropAllEquipments)
				iQty = vec_bSlots.size();
			else
				iQty = MIN(vec_bSlots.size(), random_number(1, r.iEquipmentQty));

			if (iQty)
				iQty = random_number(1, iQty);

			for (i = 0; i < iQty; ++i)
			{
				pkItem = GetWear(vec_bSlots[i]);

				if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_PKDROP) || pkItem->IsGMOwner())
					continue;

#ifdef __TRADE_BLOCK_SYSTEM__
				if (IsTradeBlocked())
					break;;
#endif
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, vec_bSlots[i], 255);
				vec_item.push_back(std::make_pair(pkItem->RemoveFromCharacter(), EQUIPMENT));
			}
		}
	}

	if (bDropAntiDropUniqueItem)
	{
		LPITEM pkItem;

		pkItem = GetWear(WEAR_UNIQUE1);

		if (pkItem && pkItem->GetVnum() == UNIQUE_ITEM_SKIP_ITEM_DROP_PENALTY)
		{
			SyncQuickslot(QUICKSLOT_TYPE_ITEM, WEAR_UNIQUE1, 255);
			vec_item.push_back(std::make_pair(pkItem->RemoveFromCharacter(), EQUIPMENT));
		}

		pkItem = GetWear(WEAR_UNIQUE2);

		if (pkItem && pkItem->GetVnum() == UNIQUE_ITEM_SKIP_ITEM_DROP_PENALTY)
		{
			SyncQuickslot(QUICKSLOT_TYPE_ITEM, WEAR_UNIQUE2, 255);
			vec_item.push_back(std::make_pair(pkItem->RemoveFromCharacter(), EQUIPMENT));
		}
	}

	{
		PIXEL_POSITION pos;
		pos.x = GetX();
		pos.y = GetY();

		unsigned int i;

		for (i = 0; i < vec_item.size(); ++i)
		{
			LPITEM item = vec_item[i].first;
			int window = vec_item[i].second;

			item->AddToGround(GetMapIndex(), pos);
			item->StartDestroyEvent();

			sys_log(0, "DROP_ITEM_PK: %s %d %d from %s", item->GetName(), pos.x, pos.y, GetName());
			LogManager::instance().ItemLog(this, item, "DEAD_DROP", (window == INVENTORY) ? "INVENTORY" : ((window == EQUIPMENT) ? "EQUIPMENT" : ""));

			pos.x = GetX() + random_number(-7, 7) * 20;
			pos.y = GetY() + random_number(-7, 7) * 20;
		}
	}
}

class FPartyAlignmentCompute
{
	public:
		FPartyAlignmentCompute(int iAmount, int x, int y)
		{
			m_iAmount = iAmount;
			m_iCount = 0;
			m_iStep = 0;
			m_iKillerX = x;
			m_iKillerY = y;
		}

		void operator () (LPCHARACTER pkChr)
		{
			if (DISTANCE_APPROX(pkChr->GetX() - m_iKillerX, pkChr->GetY() - m_iKillerY) < PARTY_DEFAULT_RANGE)
			{
				if (m_iStep == 0)
				{
					++m_iCount;
				}
				else
				{
					pkChr->UpdateAlignment(m_iAmount / m_iCount);
				}
			}
		}

		int m_iAmount;
		int m_iCount;
		int m_iStep;

		int m_iKillerX;
		int m_iKillerY;
};

void CHARACTER::UpdateStatOnKill(LPCHARACTER pkVictim)
{
	m_bKillcounterStatsChanged = true;
	if (pkVictim->IsPC())
	{
		BYTE pointID = 0;
		switch (pkVictim->GetRealEmpire())
		{
		case 1:
			pointID = POINT_EMPIRE_A_KILLED;
			break;
		case 2:
			pointID = POINT_EMPIRE_B_KILLED;
			break;
		case 3:
			pointID = POINT_EMPIRE_C_KILLED;
			break;
		}
		SetRealPoint(pointID, GetRealPoint(pointID) + 1);
	}
	else if (pkVictim->IsMonster() && pkVictim->GetMobRank() < MOB_RANK_BOSS)
	{
		SetRealPoint(POINT_MONSTERS_KILLED, GetRealPoint(POINT_MONSTERS_KILLED) + 1);
	}
	else if (pkVictim->IsMonster() && pkVictim->GetMobRank() >= MOB_RANK_BOSS)
	{
		SetRealPoint(POINT_BOSSES_KILLED, GetRealPoint(POINT_BOSSES_KILLED) + 1);
	}
	else if (pkVictim->IsStone())
	{
		SetRealPoint(POINT_STONES_DESTROYED, GetRealPoint(POINT_STONES_DESTROYED) + 1);
	}
}

void CHARACTER::Dead(LPCHARACTER pkKiller, bool bImmediateDead)
{
	if (IsDead())
		return;

	if (GetMountSystem() && GetMountSystem()->IsRiding())
		GetMountSystem()->StopRiding();
	else if (GetMountVnum())
	{
		MountVnum(0);
		UnEquipSpecialRideUniqueItem();
		UpdatePacket();
	}

	if (IsPC())
	{
		ClearGivenAuraBuffs();
		event_cancel(&m_pkAuraUpdateEvent);
		quest::CQuestManager::instance().Dead(GetPlayerID());
	}

#ifdef __FAKE_PC__
	if (FakePC_IsSupporter())
		FakePC_GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(FakePC_GetOwner(), "Your fake pc died."));
#endif

	if (!pkKiller && m_dwKillerPID)
		pkKiller = CHARACTER_MANAGER::instance().FindByPID(m_dwKillerPID);

#ifdef __FAKE_PC__
	if (pkKiller && pkKiller->FakePC_IsSupporter())
		pkKiller = FakePC_GetOwner();
#endif

	m_dwKillerPID = 0; // ¹Ýµå½Ã ÃÊ±âÈ­ ÇØ¾ßÇÔ DO NOT DELETE THIS LINE UNLESS YOU ARE 1000000% SURE

	bool isAgreedPVP = false;
	bool isUnderGuildWar = false;
	bool isDuel = false;

	if (pkKiller && pkKiller->IsPC())
	{
		if (pkKiller->GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
			pkKiller->RemoveBadAffect();

		if (pkKiller->m_pkChrTarget == this)
			pkKiller->SetTarget(NULL);

		if (!IsPC() && pkKiller->GetDungeon())
			pkKiller->GetDungeon()->IncKillCount(pkKiller, this);

		isAgreedPVP = CPVPManager::instance().Dead(this, pkKiller->GetPlayerID());
		isDuel = CArenaManager::instance().OnDead(pkKiller, this);
#ifdef COMBAT_ZONE
		CCombatZoneManager::instance().OnDead(pkKiller, this);
#endif
		if (IsPC())
		{
			
			CGuild * g1 = GetGuild();
			CGuild * g2 = pkKiller->GetGuild();

			if (g1 && g2)
				if (g1->UnderWar(g2->GetID()))
					isUnderGuildWar = true;

			pkKiller->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().Kill(pkKiller->GetPlayerID(), quest::QUEST_NO_NPC);
			// If Angels & Demons event is running
			if (quest::CQuestManager::Instance().GetEventFlag("event_anniversary_running") != 0)
			{
				quest::PC* pKillerPC = quest::CQuestManager::Instance().GetPCForce(pkKiller->GetPlayerID());
				quest::PC* pPC = quest::CQuestManager::Instance().GetPCForce(GetPlayerID());

				if (pKillerPC && pPC)
				{
					int iKillerFraction = pKillerPC->GetFlag("anniversary_event.selected_fraction");
					int iFraction = pPC->GetFlag("anniversary_event.selected_fraction");

					// Dead player and killer has fractions
					if (iFraction != 0 && iKillerFraction != 0)
					{
						// They are in different fractions
						if (iFraction != iKillerFraction)
						{
							if (!strcmp(pkKiller->GetAccountTable().hwid().c_str(), GetAccountTable().hwid().c_str()))
							{
								tchat("SAME HWID NOT COUNTING");
								pkKiller->tchat("SAME HWID NOT COUNTING");
							}
							else
								quest::CQuestManager::instance().KillEnemyFraction(pkKiller->GetPlayerID());
						}
					}
				}
			}

			CGuildManager::instance().Kill(pkKiller, this);
		}
	}
#ifdef ENABLE_HYDRA_DUNGEON
	if (pkKiller)
		CHydraDungeonManager::instance().OnKill(pkKiller, this);
#endif

/*	if (pkKiller && pkKiller->IsPC() && quest::CQuestManager::instance().GetEventFlag("event_anniversary_running"))
	{
		if (IsMonster() && GetRaceNum() == 9242 && quest::CQuestManager::instance().GetEventFlag("event_anniversary_week"))
			quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_weekangel_cnt", 1, true);
		else if (IsMonster() && GetRaceNum() == 9241 && quest::CQuestManager::instance().GetEventFlag("event_anniversary_week"))
			quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_weekdemon_cnt", 1, true);

		if (IsMonster() && GetMobRank() >= 4)
		{
			int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
			if (day == 0)
			{
				int killerFraction = pkKiller->GetQuestFlag("anniversary_event.selected_fraction");
				if (killerFraction == 1)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
				else if (killerFraction == 2)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
			}
		}
		else if (IsStone())
		{
			int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
			if (day == 2)
			{
				int killerFraction = pkKiller->GetQuestFlag("anniversary_event.selected_fraction");
				if (killerFraction == 1)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
				else if (killerFraction == 2)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
			}
		}
		else if (IsMonster())
		{
			int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
			if (day == 4)
			{
				int killerFraction = pkKiller->GetQuestFlag("anniversary_event.selected_fraction");
				if (killerFraction == 1)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
				else if (killerFraction == 2)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
			}
		}
		else if (IsPC())
		{
			int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
			if (day == 6)
			{
				int killerFraction = pkKiller->GetQuestFlag("anniversary_event.selected_fraction");
				int targetFraction = GetQuestFlag("anniversary_event.selected_fraction");
				if (killerFraction == 1 && targetFraction == 2)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
				else if (killerFraction == 2 && targetFraction == 1)
					quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
			}
		}
	}*/

#ifdef ENABLE_RUNE_SYSTEM
	if (pkKiller)
		CRuneManager::instance().OnKill(pkKiller, this);
#endif

	if (pkKiller &&
			!isAgreedPVP &&
			!isUnderGuildWar &&
			IsPC() &&
			!isDuel)
	{
		if (GetGMLevel() == GM_PLAYER || test_server)
		{
			ItemDropPenalty(pkKiller);
		}
	}

	DWORD dwMobRank = GetMobRank();
	DWORD dwRaceNum = GetRaceNum();
	DWORD dwVID = GetVID();
	
	SetPosition(POS_DEAD);

#ifdef SKILL_AFFECT_DEATH_REMAIN
	bool isExceptGood = true;
	ClearAffect(true, isExceptGood);
#else
	ClearAffect(true);
#endif

	if (pkKiller && IsPC())
	{
		if (!pkKiller->IsPC())
		{
			sys_log(1, "DEAD: %s %p WITH PENALTY", GetName(), this);
			SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
			LogManager::instance().CharLog(this, pkKiller->GetRaceNum(), "DEAD_BY_NPC", pkKiller->GetName());
		}
		else
		{
			sys_log(1, "DEAD_BY_PC: %s %p KILLER %s %p", GetName(), this, pkKiller->GetName(), get_pointer(pkKiller));
			REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);

			if (GetEmpire() != pkKiller->GetEmpire())
			{
				int iEP = MIN(GetPoint(POINT_EMPIRE_POINT), pkKiller->GetPoint(POINT_EMPIRE_POINT));

				PointChange(POINT_EMPIRE_POINT, -(iEP / 10));
				pkKiller->PointChange(POINT_EMPIRE_POINT, iEP / 5);

				char buf[256];
				snprintf(buf, sizeof(buf),
						"%d %d %d %s %d %d %d %s",
						GetEmpire(), GetAlignment(), GetPKMode(), GetName(),
						pkKiller->GetEmpire(), pkKiller->GetAlignment(), pkKiller->GetPKMode(), pkKiller->GetName());

				LogManager::instance().CharLog(this, pkKiller->GetPlayerID(), "DEAD_BY_PC", buf);
			}
			else
			{
				if (!isAgreedPVP && !isUnderGuildWar && !IsKillerMode() && GetAlignment() >= 0 && !isDuel)
				{
					int iNoPenaltyProb = 0;

					if (pkKiller->GetAlignment() >= 0)	// 1/3 percent down
						iNoPenaltyProb = 33;
					else				// 4/5 percent down
						iNoPenaltyProb = 20;

					if (random_number(1, 100) < iNoPenaltyProb)
						pkKiller->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkKiller, "¿ë½ÅÀÇ º¸È£·Î ¾ÆÀÌÅÛÀÌ ¶³¾îÁöÁö ¾Ê¾Ò½À´Ï´Ù."));
					else
					{
						if (pkKiller->GetParty())
						{
							FPartyAlignmentCompute f(-20000, pkKiller->GetX(), pkKiller->GetY());
							pkKiller->GetParty()->ForEachOnlineMember(f);

							if (f.m_iCount == 0)
								pkKiller->UpdateAlignment(-20000);
							else
							{
								sys_log(0, "ALIGNMENT PARTY count %d amount %d", f.m_iCount, f.m_iAmount);

								f.m_iStep = 1;
								pkKiller->GetParty()->ForEachOnlineMember(f);
							}
						}
						else
							pkKiller->UpdateAlignment(-20000);
					}
				}

				char buf[256];
				snprintf(buf, sizeof(buf),
						"%d %d %d %s %d %d %d %s",
						GetEmpire(), GetAlignment(), GetPKMode(), GetName(),
						pkKiller->GetEmpire(), pkKiller->GetAlignment(), pkKiller->GetPKMode(), pkKiller->GetName());

				LogManager::instance().CharLog(this, pkKiller->GetPlayerID(), "DEAD_BY_PC", buf);
			}
		}
	}
	else
	{
		sys_log(1, "DEAD: %s %p", GetName(), this);
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_DEATH_PENALTY);
	}

#ifdef __MELEY_LAIR_DUNGEON__
	if (IsStone() || IsMonster())
	{
		if (pkKiller)
		{
			if (
				(GetRaceNum() == (DWORD)(MeleyLair::MOBVNUM_RESPAWN_STONE_STEP2) || GetRaceNum() == (DWORD)(MeleyLair::MOBVNUM_RESPAWN_BOSS_STEP3))
				 && MeleyLair::CMgr::instance().IsMeleyMap(pkKiller->GetMapIndex())
				)
				MeleyLair::CMgr::instance().OnKill(GetRaceNum(), pkKiller->GetGuild());

			else if (MeleyLair::CMgr::instance().IsMeleyMap(pkKiller->GetMapIndex()))
				MeleyLair::CMgr::instance().OnKillCommon(this, pkKiller, pkKiller->GetGuild() ? pkKiller->GetGuild()->GetID() : 0);
		}
	}
#endif

	ClearSync();

	event_cancel(&m_pkStunEvent); 

	if (IsPC())
	{
		m_dwLastDeadTime = get_dword_time();
		SetKillerMode(false);
		GetDesc()->SetPhase(PHASE_DEAD);

#ifdef __EVENT_MANAGER__
		CEventManager::instance().OnPlayerDead(this);
#endif
	}
#ifdef __FAKE_PC__
	else if (!FakePC_IsSupporter())
#else
	else
#endif
	{
		if (!IS_SET(m_pointsInstant.instant_flag, INSTANT_FLAG_NO_REWARD))
		{
			if (!(pkKiller && pkKiller->IsPC() && pkKiller->GetGuild() && pkKiller->GetGuild()->UnderAnyWar(GUILD_WAR_TYPE_FIELD)))
			{
#ifdef __FAKE_PC__
				if (!FakePC_Check() && GetMobTable().dwResurrectionVnum)
#else
				if (GetMobTable().resurrection_vnum())
#endif
				{
					// DUNGEON_MONSTER_REBIRTH_BUG_FIX
					LPCHARACTER chResurrect = CHARACTER_MANAGER::instance().SpawnMob(GetMobTable().resurrection_vnum(), GetMapIndex(), GetX(), GetY(), GetZ(), true, (int) GetRotation());
					if (GetDungeon() && chResurrect)
					{
						chResurrect->SetDungeon(GetDungeon());
					}
					// END_OF_DUNGEON_MONSTER_REBIRTH_BUG_FIX

					Reward(false);
				}
				else if (IsRevive() == true)
				{
					Reward(false);
				}
				else
				{
					Reward(true); // Drops gold, item, etc..
				}
			}
			else
			{
				if (pkKiller->m_dwUnderGuildWarInfoMessageTime < get_dword_time())
				{
					pkKiller->m_dwUnderGuildWarInfoMessageTime = get_dword_time() + 60000;
					pkKiller->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkKiller, "<±æµå> ±æµåÀüÁß¿¡´Â »ç³É¿¡ µû¸¥ ÀÌÀÍÀÌ ¾ø½À´Ï´Ù."));
				}
			}
		}
	}

	if (pkKiller && pkKiller->IsPC())
		pkKiller->UpdateStatOnKill(this);

	// BOSS_KILL_LOG
	if (dwMobRank >= MOB_RANK_BOSS && pkKiller && pkKiller->IsPC())
	{
		char buf[51];
		snprintf(buf, sizeof(buf), "%d %ld", g_bChannel, pkKiller->GetMapIndex());
		if (IsStone())
			LogManager::instance().CharLog(pkKiller, dwRaceNum, "STONE_KILL", buf);
		else
			LogManager::instance().CharLog(pkKiller, dwRaceNum, "BOSS_KILL", buf);
	}
	// END_OF_BOSS_KILL_LOG

	network::GCOutputPacket<network::GCDeadPacket> pack;
	pack->set_vid(dwVID);
#ifdef SKILL_AFFECT_DEATH_REMAIN
	pack->set_killer_is_pc((pkKiller && pkKiller->IsPC()));
#endif
	PacketAround(pack);

	REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

	if (GetDesc() != NULL) {
		itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

		while (it != m_list_pkAffect.end())
			SendAffectAddPacket(GetDesc(), *it++);
	}

	if (isDuel == false)
	{
		if (m_pkDeadEvent)
		{
			sys_log(1, "DEAD_EVENT_CANCEL: %s %p %p", GetName(), this, get_pointer(m_pkDeadEvent));
			event_cancel(&m_pkDeadEvent);
		}

		if (IsStone())
			ClearStone();

		if (GetDungeon())
		{
			GetDungeon()->DeadCharacter(this);
		}

		SCharDeadEventInfo* pEventInfo = AllocEventInfo<SCharDeadEventInfo>();

		if (IsPC())
		{
			pEventInfo->isPC = true;
			pEventInfo->dwID = this->GetPlayerID();

			m_pkDeadEvent = event_create(dead_event, pEventInfo, PASSES_PER_SEC(/*test_server ? 3 :*/ 180));
		}
		else
		{
			pEventInfo->isPC = false;
			pEventInfo->dwID = this->GetVID();

			if (IsRevive() == false && HasReviverInParty() == true)
			{
				m_pkDeadEvent = event_create(dead_event, pEventInfo, bImmediateDead ? 1 : PASSES_PER_SEC(3));
			}
			else
			{
				if(GetMobRank() >= MOB_RANK_BOSS && !IsStone())
					m_pkDeadEvent = event_create(dead_event, pEventInfo, bImmediateDead ? 1 : PASSES_PER_SEC(15));
				else
					m_pkDeadEvent = event_create(dead_event, pEventInfo, bImmediateDead ? 1 : PASSES_PER_SEC(2));
			}
		}

		sys_log(1, "DEAD_EVENT_CREATE: %s %p %p", GetName(), this, get_pointer(m_pkDeadEvent));
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();

	if (IsCubeOpen())
		Cube_close(this);

#ifdef __ACCE_COSTUME__
	if (IsAcceWindowOpen())
		AcceClose();
#endif
	
	CShopManager::instance().StopShopping(this);
	CloseMyShop();
	CloseSafebox();
// Unused because it's not a guild dungeon... i guess
/* 	if (true == IsMonster() && 2493 == GetRaceNum())
	{
		if (NULL != pkKiller && NULL != pkKiller->GetGuild())
		{
			CDragonLairManager::instance().OnDragonDead( this, pkKiller->GetGuild()->GetID() );
		}
		else
		{
			sys_err("DragonLair: Dragon killed by nobody");
		}
	} */
}

struct FuncSetLastAttacked
{
	FuncSetLastAttacked(DWORD dwTime) : m_dwTime(dwTime)
	{
	}

	void operator () (LPCHARACTER ch)
	{
		ch->SetLastAttacked(m_dwTime);
	}

	DWORD m_dwTime;
};

void CHARACTER::SetLastAttacked(DWORD dwTime)
{
	assert(m_pkMobInst != NULL);

	m_pkMobInst->m_dwLastAttackedTime = dwTime;
	m_pkMobInst->m_posLastAttacked = GetXYZ();
}

#ifdef DMG_METER

class FPartyDmgCompute
{
	public:
		FPartyDmgCompute(int dmg, int vid)
		{
			m_damage = dmg;
			m_vid = vid;
		}

		void operator () (LPCHARACTER pkChr)
		{
			network::GCOutputPacket<network::GCDmgMeterPacket> p;
			p->set_dmg(m_damage);
			p->set_vid(m_vid);
			pkChr->GetDesc()->Packet(p);
		}

		int m_damage;
		int m_vid;
};
#endif

void CHARACTER::SendDamagePacket(LPCHARACTER pAttacker, int Damage, BYTE DamageFlag)
{
	if (IsPC() == true || (pAttacker->IsPC() == true && pAttacker->GetTarget() == this))
	{
		network::GCOutputPacket<network::GCDamageInfoPacket> damageInfo;
		damageInfo->set_vid((DWORD)GetVID());
		damageInfo->set_flag(DamageFlag);
		damageInfo->set_damage(Damage);
#ifdef __FAKE_PC__
		damageInfo->set_fake_pc(pAttacker->FakePC_Check());
#endif
#ifdef TARGET_DMG_VID
		damageInfo->set_target_vid(pAttacker->GetVID());
#endif

		if (GetDesc() != NULL)
		{
			GetDesc()->Packet(damageInfo);
		}

#ifdef DMG_METER
		LPPARTY party = pAttacker->GetParty();
		if (pAttacker->IsPC() && !IsPC() && party != NULL)
		{
			FPartyDmgCompute f(Damage, pAttacker->GetVID());
			party->ForEachOnlineMember(f);
		}
#endif

#ifdef __FAKE_PC__
		if (pAttacker->FakePC_Check())
		{
			if (pAttacker->FakePC_GetOwner()->GetDesc() != NULL)
			{
				pAttacker->FakePC_GetOwner()->GetDesc()->Packet(damageInfo);
			}
		}
		else
		{
			if (pAttacker->GetDesc() != NULL)
			{
				pAttacker->GetDesc()->Packet(damageInfo);
			}
		}
#else
		if (pAttacker->GetDesc() != NULL)
		{
			pAttacker->GetDesc()->Packet(damageInfo);
		}
#endif
		/*
		   if (GetArenaObserverMode() == false && GetArena() != NULL)
		   {
		   GetArena()->SendPacketToObserver(damageInfo);
		   }
		 */		
	}
}

//
// CHARACTER::Damage ¸Þ¼Òµå´Â this°¡ µ¥¹ÌÁö¸¦ ÀÔ°Ô ÇÑ´Ù.
//
// Arguments
//	pAttacker		: °ø°ÝÀÚ
//	dam		: µ¥¹ÌÁö
//	EDamageType	: ¾î¶² Çü½ÄÀÇ °ø°ÝÀÎ°¡?
//	
// Return value
//	true		: dead
//	false		: not dead yet
// 
bool CHARACTER::Damage(LPCHARACTER pAttacker, int dam, EDamageType type) // returns true if dead
{
#ifdef __FAKE_PC__
	if (pAttacker && pAttacker->FakePC_Check())
		dam = dam * pAttacker->FakePC_GetDamageFactor();
#endif

	if (DAMAGE_TYPE_MAGIC == type)
	{
		if (!pAttacker)
		{
			sys_err("no attacker given when get magic damage");
			return false;
		}

		dam = (int)((float)dam * (100 + (pAttacker->GetPoint(POINT_MAGIC_ATT_BONUS_PER) + pAttacker->GetPoint(POINT_MELEE_MAGIC_ATT_BONUS_PER))) / 100.f + 0.5f);
	}

	if (pAttacker && pAttacker->IsPC() && pAttacker->IsPrivateMap(EVENT_VALENTINE2019_DUNGEON) && GetMobRank() > 0 && GetHP() > 0)
	{
		PointChange(POINT_HP, -1);
		SendDamagePacket(pAttacker, 1, (1 << 0));
		return true;
	}

#ifdef __POLY_NO_PVP_DMG__
	if (pAttacker && pAttacker->IsPC() && IsPC() && pAttacker->IsPolymorphed())
	{
		SendDamagePacket(pAttacker, 0, (1 << 3));
		return false;
	}
#endif

	// if (pAttacker && pAttacker->IsPC() && GetMobRank()>=MOB_RANK_BOSS && 
	// 		quest::CQuestManager::instance().GetEventFlag("disable_damage_check_q_susp") == 0 && 
	// 		quest::CQuestManager::Instance().IsSuspended(pAttacker->GetPlayerID()))
	// 	return false;

	if (GetRaceNum() == 5001)
	{
		bool bDropMoney = false;
		int iPercent = (GetHP() * 100) / GetMaxHP();

		if (iPercent <= 10 && GetMaxSP() < 5)
		{
			SetMaxSP(5);
			bDropMoney = true;
		}
		else if (iPercent <= 20 && GetMaxSP() < 4)
		{
			SetMaxSP(4);
			bDropMoney = true;
		}
		else if (iPercent <= 40 && GetMaxSP() < 3)
		{
			SetMaxSP(3);
			bDropMoney = true;
		}
		else if (iPercent <= 60 && GetMaxSP() < 2)
		{
			SetMaxSP(2);
			bDropMoney = true;
		}
		else if (iPercent <= 80 && GetMaxSP() < 1)
		{
			SetMaxSP(1);
			bDropMoney = true;
		}

		if (bDropMoney)
		{
			DWORD dwGold = 1000;
			int iSplitCount = random_number(10, 13);

			sys_log(0, "WAEGU DropGoldOnHit %d times", GetMaxSP());

			for (int i = 1; i <= iSplitCount; ++i)
			{
				PIXEL_POSITION pos;
				LPITEM item;

				if ((item = ITEM_MANAGER::instance().CreateItem(1, dwGold / iSplitCount)))
				{
					if (i != 0)
					{
						pos.x = (random_number(-14, 14) + random_number(-14, 14)) * 20;
						pos.y = (random_number(-14, 14) + random_number(-14, 14)) * 20;

						pos.x += GetX();
						pos.y += GetY();
					}

					item->AddToGround(GetMapIndex(), pos);
					item->StartDestroyEvent();
				}
			}
		}
	}

	// ÆòÅ¸°¡ ¾Æ´Ò ¶§´Â °øÆ÷ Ã³¸®
#ifdef ENABLE_RUNE_SYSTEM
	if (type != DAMAGE_TYPE_RUNE)
#endif
		if (type != DAMAGE_TYPE_NORMAL && type != DAMAGE_TYPE_NORMAL_RANGE)
		{
			if (IsAffectFlag(AFF_TERROR) || IsAffectFlag(AFF_TERROR_PERFECT))
			{
				int pct = GetSkillPower(SKILL_TERROR) / 400;

				if (random_number(1, 100) <= pct)
					return false;
			}
		}

	int iCurHP = GetHP();
	int iCurSP = GetSP();

	bool IsCritical = false;
	bool IsPenetrate = false;
	bool IsDeathBlow = false;

	enum DamageFlag
	{
		DAMAGE_NORMAL	= (1 << 0),
		DAMAGE_POISON	= (1 << 1),
		DAMAGE_DODGE	= (1 << 2),
		DAMAGE_BLOCK	= (1 << 3),
		DAMAGE_PENETRATE= (1 << 4),
		DAMAGE_CRITICAL = (1 << 5),
	};

#ifdef __MELEY_LAIR_DUNGEON__
	if (pAttacker)
	{
		if ((GetRaceNum() == (WORD)(MeleyLair::STATUE_VNUM)) && (MeleyLair::CMgr::instance().IsMeleyMap(pAttacker->GetMapIndex())) && (!MeleyLair::CMgr::instance().Damage(this, pAttacker->GetGuild())))
		{
			//sys_log(0, "%s:%d %s MELEYLAIR::STATUE_VNUM  BLOCK DMG map[%d]", __FILE__, __LINE__, __FUNCTION__, pAttacker->GetMapIndex());
			SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
			return false;
		}
		else if ((GetRaceNum() == (WORD)(MeleyLair::BOSS_VNUM)) && (MeleyLair::CMgr::instance().IsMeleyMap(pAttacker->GetMapIndex())))
		{
			SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
			return false;
		}
	}
#endif

	// Attacker is in dungeon
	if (pAttacker && pAttacker->IsPC() && pAttacker->GetDungeon() && quest::CQuestManager::Instance().GetEventFlag("triggerfix1_disabled") == 0)
	{
		quest::PC* pc = quest::CQuestManager::Instance().GetPCForce(pAttacker->GetPlayerID());

		if (!pc || !quest::CQuestManager::Instance().CheckQuestLoaded(pc))
		{
			sys_err("DMG_DUNGEON NO QUESTPTR %s", pAttacker->GetName());
			return false;
		}
		
		if (pc->IsRunning())
		{
			sys_err("DMG_DUNGEON QUEST OPEN %s", pAttacker->GetName());
			return false;
		}
	}

	//PROF_UNIT puAttr("Attr");
	//
	// ¸¶¹ýÇü ½ºÅ³°ú, ·¹ÀÎÁöÇü ½ºÅ³Àº(±ÃÀÚ°´) Å©¸®Æ¼ÄÃ°ú, °üÅë°ø°Ý °è»êÀ» ÇÑ´Ù.
	// ¿ø·¡´Â ÇÏÁö ¾Ê¾Æ¾ß ÇÏ´Âµ¥ Nerf(´Ù¿î¹ë·±½º)ÆÐÄ¡¸¦ ÇÒ ¼ö ¾ø¾î¼­ Å©¸®Æ¼ÄÃ°ú
	// °üÅë°ø°ÝÀÇ ¿ø·¡ °ªÀ» ¾²Áö ¾Ê°í, /2 ÀÌ»óÇÏ¿© Àû¿ëÇÑ´Ù.
	// 
	// ¹«»ç ÀÌ¾ß±â°¡ ¸¹¾Æ¼­ ¹Ð¸® ½ºÅ³µµ Ãß°¡
	//
	// 20091109 : ¹«»ç°¡ °á°úÀûÀ¸·Î ¾öÃ»³ª°Ô °­ÇØÁø °ÍÀ¸·Î °á·Ð³², µ¶ÀÏ ±âÁØ ¹«»ç ºñÀ² 70% À°¹Ú
	//
	if (type == DAMAGE_TYPE_MELEE || type == DAMAGE_TYPE_RANGE || type == DAMAGE_TYPE_MAGIC)
	{
		if (pAttacker)
		{
			// Å©¸®Æ¼ÄÃ
			int iCriticalPct = pAttacker->GetPoint(POINT_CRITICAL_PCT);

			if (!IsPC())
				iCriticalPct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_CRITICAL_BONUS);

			if (iCriticalPct)
			{
				if (iCriticalPct >= 10) // 10º¸´Ù Å©¸é 5% + (4¸¶´Ù 1%¾¿ Áõ°¡), µû¶ó¼­ ¼öÄ¡°¡ 50ÀÌ¸é 20%
					iCriticalPct = 5 + (iCriticalPct - 10) / 4;
				else // 10º¸´Ù ÀÛÀ¸¸é ´Ü¼øÈ÷ ¹ÝÀ¸·Î ±ðÀ½, 10 = 5%
					iCriticalPct /= 2;

				//Å©¸®Æ¼ÄÃ ÀúÇ× °ª Àû¿ë.
				iCriticalPct -= GetPoint(POINT_RESIST_CRITICAL);

				if (!IsIgnorePenetrateCritical() && random_number(1, 100) <= iCriticalPct) // removed !IsPC() && 
				{
					IsCritical = true;
#ifdef RUNE_CRITICAL_POINT
					dam += dam * (100 + pAttacker->GetPoint(POINT_CRITICAL_DAMAGE_BONUS) + (!IsPC() ? pAttacker->GetPoint(POINT_RUNE_CRITICAL_PVM) : 0)) / 100;
#else
					dam += dam * (100 + pAttacker->GetPoint(POINT_CRITICAL_DAMAGE_BONUS)) / 100;
#endif
					EffectPacket(SE_CRITICAL);

					if (IsAffectFlag(AFF_MANASHIELD))
					{
						RemoveAffect(AFF_MANASHIELD);
					}

					if (IsAffectFlag(AFF_MANASHIELD_PERFECT))
						RemoveAffect(AFF_MANASHIELD_PERFECT);
				}
			}

			// °üÅë°ø°Ý
			int iPenetratePct = pAttacker->GetPoint(POINT_PENETRATE_PCT);

			if (!IsPC())
				iPenetratePct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_PENETRATE_BONUS);


			if (iPenetratePct)
			{
				{
					CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_RESIST_PENETRATE);

					if (NULL != pkSk)
					{
						pkSk->SetVar("k", 1.0f * GetSkillPower(SKILL_RESIST_PENETRATE) / 100.0f);

						iPenetratePct -= static_cast<int>(pkSk->kPointPoly.Evaluate());
					}
				}

				if (iPenetratePct >= 10)
				{
					// 10º¸´Ù Å©¸é 5% + (4¸¶´Ù 1%¾¿ Áõ°¡), µû¶ó¼­ ¼öÄ¡°¡ 50ÀÌ¸é 20%
					iPenetratePct = 5 + (iPenetratePct - 10) / 4;
				}
				else
				{
					// 10º¸´Ù ÀÛÀ¸¸é ´Ü¼øÈ÷ ¹ÝÀ¸·Î ±ðÀ½, 10 = 5%
					iPenetratePct /= 2;
				}

				//°üÅëÅ¸°Ý ÀúÇ× °ª Àû¿ë.
				iPenetratePct -= GetPoint(POINT_RESIST_PENETRATE);

				if (!IsIgnorePenetrateCritical() && random_number(1, 100) <= iPenetratePct)
				{
					IsPenetrate = true;

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°üÅë Ãß°¡ µ¥¹ÌÁö %d"), GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100);

					dam += GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100;

					if (IsAffectFlag(AFF_MANASHIELD))
					{
						RemoveAffect(AFF_MANASHIELD);
					}

					if (IsAffectFlag(AFF_MANASHIELD_PERFECT))
						RemoveAffect(AFF_MANASHIELD_PERFECT);
				}
			}
		}
	}
	// 
	// ÄÞº¸ °ø°Ý, È° °ø°Ý, Áï ÆòÅ¸ ÀÏ ¶§¸¸ ¼Ó¼º°ªµéÀ» °è»êÀ» ÇÑ´Ù.
	//
	else if (type == DAMAGE_TYPE_NORMAL || type == DAMAGE_TYPE_NORMAL_RANGE)
	{
		if (type == DAMAGE_TYPE_NORMAL)
		{
			// ±ÙÁ¢ ÆòÅ¸ÀÏ °æ¿ì ¸·À» ¼ö ÀÖÀ½

			int blockValue = MAX(0, GetPoint(POINT_BLOCK) - pAttacker->GetPoint(POINT_BLOCK_IGNORE_BONUS));
			//pAttacker->tchat("point block(target): %d point pen(attacker): %d", GetPoint(POINT_BLOCK), pAttacker->GetPoint(POINT_BLOCK_IGNORE_BONUS));
			if (blockValue && random_number(1, 100) <= blockValue)
			{
/*				if (test_server)
				{
					if (pAttacker)
						pAttacker->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pAttacker, "%s ºí·°! (%d%%)"), GetName(), blockValue);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%s ºí·°! (%d%%)"), GetName(), blockValue);
				}*/

				SendDamagePacket(pAttacker, 0, DAMAGE_BLOCK);
				return false;
			}
		}
		else if (type == DAMAGE_TYPE_NORMAL_RANGE)
		{
			// ¿ø°Å¸® ÆòÅ¸ÀÇ °æ¿ì ÇÇÇÒ ¼ö ÀÖÀ½
			if (GetPoint(POINT_DODGE) && random_number(1, 100) <= GetPoint(POINT_DODGE))
			{
/*				if (test_server)
				{
					if (pAttacker)
						pAttacker->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pAttacker, "%s È¸ÇÇ! (%d%%)"), GetName(), GetPoint(POINT_DODGE));
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%s È¸ÇÇ! (%d%%)"), GetName(), GetPoint(POINT_DODGE));
				}*/

				SendDamagePacket(pAttacker, 0, DAMAGE_DODGE);
				return false;
			}
		}

		if (IsAffectFlag(AFF_TERROR) || IsAffectFlag(AFF_TERROR_PERFECT))
			dam = (int) (dam * (95 - GetSkillPower(SKILL_TERROR) / 5) / 100);

		if (IsAffectFlag(AFF_HOSIN) || IsAffectFlag(AFF_HOSIN_PERFECT))
			dam = dam * (100 - GetPoint(POINT_RESIST_NORMAL_DAMAGE)) / 100;

		//
		// °ø°ÝÀÚ ¼Ó¼º Àû¿ë
		//
		if (pAttacker)
		{
			if (type == DAMAGE_TYPE_NORMAL)
			{
				// ¹Ý»ç
				if (GetPoint(POINT_REFLECT_MELEE))
				{
					int reflectDamage = dam * GetPoint(POINT_REFLECT_MELEE) / 100;

					// NOTE: °ø°ÝÀÚ°¡ IMMUNE_REFLECT ¼Ó¼ºÀ» °®°íÀÖ´Ù¸é ¹Ý»ç¸¦ ¾È ÇÏ´Â °Ô 
					// ¾Æ´Ï¶ó 1/3 µ¥¹ÌÁö·Î °íÁ¤ÇØ¼­ µé¾î°¡µµ·Ï ±âÈ¹¿¡¼­ ¿äÃ».
					if (pAttacker->IsImmune(IMMUNE_REFLECT))
						reflectDamage = int(reflectDamage / 3.0f + 0.5f);

					pAttacker->Damage(this, reflectDamage, DAMAGE_TYPE_SPECIAL);
				}
			}

			// Å©¸®Æ¼ÄÃ
			int iCriticalPct = pAttacker->GetPoint(POINT_CRITICAL_PCT);

			if (!IsPC())
				iCriticalPct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_CRITICAL_BONUS);

			if (iCriticalPct)
			{
				//Å©¸®Æ¼ÄÃ ÀúÇ× °ª Àû¿ë.
				iCriticalPct -= GetPoint(POINT_RESIST_CRITICAL);

				if (!IsIgnorePenetrateCritical() && random_number(1, 100) <= iCriticalPct)
				{
					IsCritical = true;
#ifdef RUNE_CRITICAL_POINT
					dam += dam * (100 + pAttacker->GetPoint(POINT_CRITICAL_DAMAGE_BONUS) + (!IsPC() ? pAttacker->GetPoint(POINT_RUNE_CRITICAL_PVM) : 0)) / 100;
#else
					dam += dam * (100 + pAttacker->GetPoint(POINT_CRITICAL_DAMAGE_BONUS)) / 100;
#endif
					EffectPacket(SE_CRITICAL);
				}
			}

			// °üÅë°ø°Ý
			int iPenetratePct = pAttacker->GetPoint(POINT_PENETRATE_PCT);

			if (!IsPC())
				iPenetratePct += pAttacker->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_PENETRATE_BONUS);

			{
				CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_RESIST_PENETRATE);

				if (NULL != pkSk)
				{
					pkSk->SetVar("k", 1.0f * GetSkillPower(SKILL_RESIST_PENETRATE) / 100.0f);

					iPenetratePct -= static_cast<int>(pkSk->kPointPoly.Evaluate());
				}
			}


			if (iPenetratePct)
			{
				
				//°üÅëÅ¸°Ý ÀúÇ× °ª Àû¿ë.
				iPenetratePct -= GetPoint(POINT_RESIST_PENETRATE);

				if (!IsIgnorePenetrateCritical() && random_number(1, 100) <= iPenetratePct)
				{
					IsPenetrate = true;

					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°üÅë Ãß°¡ µ¥¹ÌÁö %d"), GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100);
					dam += GetPoint(POINT_DEF_GRADE) * (100 + GetPoint(POINT_DEF_BONUS)) / 100;
				}
			}

			// HP ½ºÆ¿
			if (pAttacker->GetPoint(POINT_STEAL_HP) && !cannot_dead && (IsPC() || GetRaceNum() != BOSSHUNT_BOSS_VNUM
#ifdef __MELEY_LAIR_DUNGEON__
			 && GetRaceNum() != MeleyLair::STATUE_VNUM
#endif
#ifdef BOSSHUNT_EVENT_UPDATE
				&& GetRaceNum() != BOSSHUNT_BOSS_VNUM2
#endif
				))
			{
				int pct = 1;

				if (random_number(1, 10) <= pct)
				{
					int iHP = MIN(dam, MAX(0, iCurHP)) * pAttacker->GetPoint(POINT_STEAL_HP) / 100;

					if (iHP > 0 && iCurHP >= iHP)
					{
						CreateFly(FLY_HP_SMALL, pAttacker);
						pAttacker->PointChange(POINT_HP, abs(iHP));
#ifdef ENABLE_HYDRA_DUNGEON
						if (GetRaceNum() == HYDRA_BOSS_VNUM || GetRaceNum() == MAST_VNUM)
							CHydraDungeonManager::instance().NotifyHydraDmg(this, &iHP);

						if (pAttacker->IsAffectFlag(AFF_HYDRA))
						{
							CAffect* hydraAffect = pAttacker->FindAffect(AFFECT_HYDRA);
							if (hydraAffect)
								iHP = iHP * hydraAffect->lApplyValue / 100;
						}
#endif

#ifdef ENABLE_RUNE_SYSTEM
						CRuneManager::instance().CalculateRuneAffects(pAttacker, this, iHP, type);
#endif

						PointChange(POINT_HP, -iHP);
						iCurHP -= iHP;
					}
				}
			}

			// SP ½ºÆ¿
			if (pAttacker->GetPoint(POINT_STEAL_SP))
			{
				int pct = 1;

				if (random_number(1, 10) <= pct)
				{
					int iCur;

					if (IsPC())
						iCur = iCurSP;
					else
						iCur = iCurHP;

					int iSP = MIN(dam, MAX(0, iCur)) * pAttacker->GetPoint(POINT_STEAL_SP) / 100;

					if (iSP > 0 && iCur >= iSP)
					{
						CreateFly(FLY_SP_SMALL, pAttacker);
						pAttacker->PointChange(POINT_SP, iSP);

						if (IsPC())
						{
							PointChange(POINT_SP, -iSP);
							iCurSP -= iSP;
						}
					}
				}
			}

			// µ· ½ºÆ¿
			if (pAttacker->GetPoint(POINT_STEAL_GOLD))
			{
				if (random_number(1, 100) <= pAttacker->GetPoint(POINT_STEAL_GOLD))
				{
					int iAmount = random_number(1, GetLevel());
					pAttacker->PointChange(POINT_GOLD, iAmount);
					LogManager::instance().MoneyLog(MONEY_LOG_MISC, 1, iAmount);
				}
			}

			// Ä¥ ¶§¸¶´Ù HPÈ¸º¹
			if (pAttacker->GetPoint(POINT_HIT_HP_RECOVERY) && random_number(0, 4) > 0) // 80% È®·ü
			{
				int i = (int)(((int64_t)MIN(dam, iCurHP)) * ((int64_t)pAttacker->GetPoint(POINT_HIT_HP_RECOVERY)) / ((int64_t)100));

				if (i && i > 0)
				{
					CreateFly(FLY_HP_SMALL, pAttacker);
					pAttacker->PointChange(POINT_HP, i);
				}
			}

			// Ä¥ ¶§¸¶´Ù SPÈ¸º¹
			if (pAttacker->GetPoint(POINT_HIT_SP_RECOVERY) && random_number(0, 4) > 0) // 80% È®·ü
			{
				int i = MIN(dam, iCurSP) * pAttacker->GetPoint(POINT_HIT_SP_RECOVERY) / 100;

				if (i)
				{
					CreateFly(FLY_SP_SMALL, pAttacker);
					pAttacker->PointChange(POINT_SP, i);
				}
			}

			// »ó´ë¹æÀÇ ¸¶³ª¸¦ ¾ø¾Ø´Ù.
			if (pAttacker->GetPoint(POINT_MANA_BURN_PCT))
			{
				if (random_number(1, 100) <= pAttacker->GetPoint(POINT_MANA_BURN_PCT))
					PointChange(POINT_SP, -50);
			}
		}
	}

	//
	// ÆòÅ¸ ¶Ç´Â ½ºÅ³·Î ÀÎÇÑ º¸³Ê½º ÇÇÇØ/¹æ¾î °è»ê
	// 
	switch (type)
	{
		case DAMAGE_TYPE_NORMAL:
		case DAMAGE_TYPE_NORMAL_RANGE:
			if (pAttacker)
				if (pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS))
					dam = dam * (100 + pAttacker->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS)) / 100;

			// pAttacker->tchat("POINT_NORMAL_HIT_DEFEND_BONUS[%d] dam = %d", GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS), dam);
			dam = dam * (100 - MIN(99, GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS))) / 100;
			break;

		case DAMAGE_TYPE_MELEE:
		case DAMAGE_TYPE_RANGE:
		case DAMAGE_TYPE_FIRE:
		case DAMAGE_TYPE_ICE:
		case DAMAGE_TYPE_ELEC:
		case DAMAGE_TYPE_MAGIC:
			if (pAttacker)
				if (pAttacker->GetPoint(POINT_SKILL_DAMAGE_BONUS))
					dam = dam * (100 + pAttacker->GetPoint(POINT_SKILL_DAMAGE_BONUS)) / 100;

			dam = dam * (100 - MIN(99, GetPoint(POINT_SKILL_DEFEND_BONUS))) / 100;
			break;

		default:
			break;
	}

	//
	// ¸¶³ª½¯µå(Èæ½Å¼öÈ£)
	//
#ifdef ENABLE_RUNE_SYSTEM
	if (type != DAMAGE_TYPE_RUNE)
#endif
		if (IsAffectFlag(AFF_MANASHIELD) || IsAffectFlag(AFF_MANASHIELD_PERFECT))
		{
			// POINT_MANASHIELD ´Â ÀÛ¾ÆÁú¼ö·Ï ÁÁ´Ù
			int iDamageSPPart = abs(dam / 5);
			int iDamageToSP = iDamageSPPart * GetPoint(POINT_MANASHIELD) / 100;
			int iSP = GetSP();

			// SP°¡ ÀÖÀ¸¸é ¹«Á¶°Ç µ¥¹ÌÁö Àý¹Ý °¨¼Ò
			if (iDamageToSP <= iSP)
			{
				PointChange(POINT_SP, -iDamageToSP);
				dam -= iDamageSPPart;
			}
			else
			{
				// Á¤½Å·ÂÀÌ ¸ðÀÚ¶ó¼­ ÇÇ°¡ ´õ ±ï¿©¾ßÇÒ‹š
				PointChange(POINT_SP, -GetSP());
				dam -= iSP * 100 / MAX(GetPoint(POINT_MANASHIELD), 1);
			}
		}

	//
	// ÀüÃ¼ ¹æ¾î·Â »ó½Â (¸ô ¾ÆÀÌÅÛ)
	// 
#ifdef ENABLE_RUNE_SYSTEM
	if (type != DAMAGE_TYPE_RUNE)
#endif
		if (GetPoint(POINT_MALL_DEFBONUS) > 0)
		{
			int dec_dam = MIN(200, dam * GetPoint(POINT_MALL_DEFBONUS) / 100);
			dam -= dec_dam;
		}

#ifdef ENABLE_RUNE_SYSTEM
	if (type != DAMAGE_TYPE_RUNE)
#endif
		if (pAttacker)
		{
			//
			// ÀüÃ¼ °ø°Ý·Â »ó½Â (¸ô ¾ÆÀÌÅÛ)
			//
			if (pAttacker->GetPoint(POINT_MALL_ATTBONUS) > 0)
			{
				int add_dam = MIN(300, dam * pAttacker->GetLimitPoint(POINT_MALL_ATTBONUS) / 100);
				dam += add_dam;
			}

			//
			// Á¦±¹À¸·Î ÀÎÇÑ º¸³Ê½º (ÇÑ±¹ ¿Ãµå ¹öÀü¸¸ Àû¿ë)
			//
			int iEmpire = GetEmpire();
			long lMapIndex = GetMapIndex();
			int iMapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex);

			if (pAttacker->IsPC())
			{
				iEmpire = pAttacker->GetEmpire();
				lMapIndex = pAttacker->GetMapIndex();
				iMapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(lMapIndex);

				// ´Ù¸¥ Á¦±¹ »ç¶÷ÀÎ °æ¿ì µ¥¹ÌÁö 10% °¨¼Ò
				if (iEmpire && iMapEmpire && iEmpire != iMapEmpire)
				{
					int percent = 9;

					dam = dam * percent / 10;
				}

				if (!IsPC() && GetMonsterDrainSPPoint())
				{
					int iDrain = GetMonsterDrainSPPoint();

					if (iDrain <= pAttacker->GetSP())
						pAttacker->PointChange(POINT_SP, -iDrain);
					else
					{
						int iSP = pAttacker->GetSP();
						pAttacker->PointChange(POINT_SP, -iSP);
					}
				}

			}
			else if (pAttacker->IsGuardNPC())
			{
#ifdef ENABLE_ZODIAC_TEMPLE
				dam += dam / 1;
#else
				SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_NO_REWARD);
				Stun();
				return true;
#endif
			}
		}
	//puAttr.Pop();

#ifdef ELONIA
	if (GetMapIndex() != EMPIREWAR_MAP_INDEX)
#endif
	{
		if (!GetSectree() || GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK))
			return false;
	}

	if (!IsPC())
	{
		if (m_pkParty && m_pkParty->GetLeader())
			m_pkParty->GetLeader()->SetLastAttacked(get_dword_time());
		else
			SetLastAttacked(get_dword_time());

		// ¸ó½ºÅÍ ´ë»ç : ¸ÂÀ» ¶§
		MonsterChat(MONSTER_CHAT_ATTACKED);
	}

	if (IsStun())
	{
		Dead(pAttacker);
		return true;
	}

	if (IsDead())
		return true;

	// µ¶ °ø°ÝÀ¸·Î Á×Áö ¾Êµµ·Ï ÇÔ.
	if (type == DAMAGE_TYPE_POISON)
	{
		if (GetHP() - dam <= 0)
		{
			dam = GetHP() - 1;
		}
	}

	// ------------------------
	// µ¶ÀÏ ÇÁ¸®¹Ì¾ö ¸ðµå 
	// -----------------------
#ifdef ENABLE_RUNE_SYSTEM
	if (type != DAMAGE_TYPE_RUNE)
#endif
		if (pAttacker && pAttacker->IsPC())
		{
			int iDmgPct = CHARACTER_MANAGER::instance().GetUserDamageRate(pAttacker);
			dam = dam * iDmgPct / 100;
		}
	
	
	// any sense of this?
	/*float fDamageFactor = 1.0f;
	if (GetLevel() >= 1 && GetLevel() <= 35)
		fDamageFactor = 1.0;
	else if (GetLevel() >= 36 && GetLevel() <= 55)
		fDamageFactor = 1.0;
	else if (GetLevel() >= 56 && GetLevel() <= 75)
		fDamageFactor = 1.0;
	else if (GetLevel() >= 76 && GetLevel() <= 105)
		fDamageFactor = 1.0;
	dam = abs(dam * fDamageFactor);*/

	
	
	// STONE SKIN : ÇÇÇØ ¹ÝÀ¸·Î °¨¼Ò
#ifdef ENABLE_RUNE_SYSTEM
	if (type != DAMAGE_TYPE_RUNE)
#endif
		if (IsMonster() && IsStoneSkinner())
		{
			if (GetHPPct() < GetMobTable().stone_skin_point())
				dam /= 2;
		}

	//PROF_UNIT puRest1("Rest1");
	if (pAttacker)
	{
		// DEATH BLOW : È®·ü ÀûÀ¸·Î 4¹è ÇÇÇØ (!? ÇöÀç ÀÌº¥Æ®³ª °ø¼ºÀü¿ë ¸ó½ºÅÍ¸¸ »ç¿ëÇÔ)
#ifdef ENABLE_RUNE_SYSTEM
		if (type != DAMAGE_TYPE_RUNE)
#endif
			if (pAttacker->IsMonster() && pAttacker->IsDeathBlower())
			{
				if (pAttacker->IsDeathBlow())
				{
	#ifdef __WOLFMAN__
					if (random_number(0, 4) == GetJob())
	#else
					if (random_number(0, 3) == GetJob())
	#endif
					{
						IsDeathBlow = true;
						dam = dam * 3;
					}
				}
			}

#ifdef ENABLE_RUNE_SYSTEM
		if (type != DAMAGE_TYPE_RUNE)
#endif
			dam = BlueDragon_Damage(this, pAttacker, dam);

		BYTE damageFlag = 0;

		if (type == DAMAGE_TYPE_POISON)
			damageFlag = DAMAGE_POISON;
		else
			damageFlag = DAMAGE_NORMAL;

		if (IsCritical == true)
			damageFlag |= DAMAGE_CRITICAL;

		if (IsPenetrate == true)
			damageFlag |= DAMAGE_PENETRATE;


		//ÃÖÁ¾ µ¥¹ÌÁö º¸Á¤
		float damMul = this->GetDamMul();
		float tempDam = dam;
#ifdef ENABLE_RUNE_SYSTEM
		if (type != DAMAGE_TYPE_RUNE)
#endif
			dam = tempDam * damMul + 0.5f;

		if (!IsPC() && (GetRaceNum() == BOSSHUNT_BOSS_VNUM
#ifdef BOSSHUNT_EVENT_UPDATE
			|| GetRaceNum() == BOSSHUNT_BOSS_VNUM2
#endif
			))
		{
			if ((type == DAMAGE_TYPE_NORMAL || type == DAMAGE_TYPE_NORMAL_RANGE) && pAttacker->GetLevel() >= 30)
			{
				dam = 1;
				pAttacker->AddAffect(AFFECT_BOSSHUNT_ATTSPEED, POINT_ATT_SPEED, -100, 0, 10, 0, true);
				
				if (pAttacker->IsPC() && quest::CQuestManager::instance().GetEventFlag("enable_bosshunt_event"))
				{
					std::string currHuntID = "event_boss_hunt.points" + std::to_string(quest::CQuestManager::instance().GetEventFlag("bosshunt_event_id"));
					int newPoints = pAttacker->GetQuestFlag(currHuntID) + 1;
#ifdef BOSSHUNT_EVENT_UPDATE
					if (GetRaceNum() == BOSSHUNT_BOSS_VNUM2)
						++newPoints;
#endif
					pAttacker->SetQuestFlag(currHuntID, newPoints);
					pAttacker->ChatPacket(CHAT_TYPE_COMMAND, "BossHuntPoints %d", newPoints);
					if (test_server)
						pAttacker->tchat("give more points bro [now: %d]", newPoints);
				}
			}
			else
			{
				dam = 0;
				damageFlag = DAMAGE_BLOCK;
			}
		}

#ifdef ENABLE_HYDRA_DUNGEON
		if (pAttacker->IsAffectFlag(AFF_HYDRA))
		{
			CAffect* hydraAffect = pAttacker->FindAffect(AFFECT_HYDRA);
			if (hydraAffect)
				dam = dam * hydraAffect->lApplyValue / 100;
		}
#endif

#ifdef ENABLE_RUNE_SYSTEM
		if (IsPC() || (GetRaceNum() != BOSSHUNT_BOSS_VNUM
#ifdef BOSSHUNT_EVENT_UPDATE
			&& GetRaceNum() != BOSSHUNT_BOSS_VNUM2
#endif
			))
			CRuneManager::instance().CalculateRuneAffects(pAttacker, this, dam, type);
#endif

		/*if (test_server)
		{
			pAttacker->ChatPacket(CHAT_TYPE_INFO, "-> %s, DAM %d HP %d(%d%%) %s%s",
				GetName(),
				dam,
				GetHP(),
				(GetHP() * 100) / GetMaxHP(),
				IsCritical ? "crit " : "",
				IsPenetrate ? "pene " : "",
				IsDeathBlow ? "deathblow " : "");

			ChatPacket(CHAT_TYPE_PARTY, "<- %s, DAM %d HP %d(%d%%) %s%s",
					pAttacker ? pAttacker->GetName() : 0,
					dam, 
					GetHP(),
					(GetHP() * 100) / GetMaxHP(),
					IsCritical ? "crit " : "",
					IsPenetrate ? "pene " : "",
					IsDeathBlow ? "deathblow " : "");
		}*/

		if (cannot_dead)
		{
			dam = 0;
			damageFlag = DAMAGE_BLOCK;
		}

		SendDamagePacket(pAttacker, dam, damageFlag);

		if (m_bDetailLog)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%s[%d]°¡ °ø°Ý À§Ä¡: %d %d"), pAttacker->GetName(), (DWORD) pAttacker->GetVID(), pAttacker->GetX(), pAttacker->GetY());
		}
#ifdef __MELEY_LAIR_DUNGEON__
		if (pAttacker->IsPC() && GetRaceNum() == MeleyLair::STATUE_VNUM && MeleyLair::CMgr::instance().IsMeleyMap(pAttacker->GetMapIndex()) && dam >= GetHP())
			dam = GetHP() - 1;
#endif
		if (pAttacker->IsPC())
			SetLastAttackedByPC();
	}

	if (!cannot_dead)
	{
#ifdef __DAMAGE_QUEST_TRIGGER__
		if (pAttacker)
		{
			pAttacker->SetQuestDamage(dam);
			pAttacker->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().QuestDamage(pAttacker->GetPlayerID(), GetRaceNum());
		}
#endif
#ifdef ENABLE_HYDRA_DUNGEON
		if (GetRaceNum() == HYDRA_BOSS_VNUM || GetRaceNum() == MAST_VNUM)
			CHydraDungeonManager::instance().NotifyHydraDmg(this, &dam);
#endif
		PointChange(POINT_HP, -dam, false);

#ifdef __UNIMPLEMENT__
		if (pAttacker && (int)pAttacker->GetLevel() - 20 <= GetLevel() && pAttacker->IsPC())
			CHARACTER_MANAGER::instance().CharacterDamage(GetJob(), IsPC() && pAttacker->IsPC(), type, GetLevel(), dam);
#endif
	}

	//puRest1.Pop();

	//PROF_UNIT puRest2("Rest2");
	if (pAttacker && dam > 0 && IsNPC())
	{
		//PROF_UNIT puRest20("Rest20");
		TDamageMap::iterator it;

#ifdef __FAKE_PC__
		if (pAttacker->FakePC_IsSupporter())
		{
			LPCHARACTER pOwner = pAttacker->FakePC_GetOwner();

			it = m_map_kDamage.find(pOwner->GetVID());
			if (it == m_map_kDamage.end())
			{
				m_map_kDamage.insert(TDamageMap::value_type(pOwner->GetVID(), TBattleInfo(0, dam, 0)));
				it = m_map_kDamage.find(pOwner->GetVID());
			}
			else
			{
				it->second.iTotalFakePCDamage += dam;
			}
		}
#endif

		it = m_map_kDamage.find(pAttacker->GetVID());

		if (it == m_map_kDamage.end())
		{
#ifdef __FAKE_PC__
			m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(dam, 0, 0)));
#else
			m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(dam, 0)));
#endif
			it = m_map_kDamage.find(pAttacker->GetVID());
		}
		else
		{
			it->second.iTotalDamage += dam;
		}
		//puRest20.Pop();

		//PROF_UNIT puRest21("Rest21");
		StartRecoveryEvent(); // ¸ó½ºÅÍ´Â µ¥¹ÌÁö¸¦ ÀÔÀ¸¸é È¸º¹À» ½ÃÀÛÇÑ´Ù.
		//puRest21.Pop();

		//PROF_UNIT puRest22("Rest22");
		UpdateAggrPointEx(pAttacker, type, dam, it->second);
		//puRest22.Pop();
	}
	//puRest2.Pop();

	//PROF_UNIT puRest3("Rest3");
	if (GetHP() <= 0)
	{
		Stun();

		if (pAttacker && !pAttacker->IsNPC())
			m_dwKillerPID = pAttacker->GetPlayerID();
		else
			m_dwKillerPID = 0;
	}

#ifdef DMG_RANKING
	if (GetRaceNum() == 35074 && !quest::CQuestManager::instance().GetEventFlag("dmg_ranking_disabled"))
	{
		pAttacker->registerDamageToDummy(type, dam);
		// pAttacker->tchat("dmg type %d", type);
	}
		
#endif

	return false;
}

void CHARACTER::DistributeHP(LPCHARACTER pkKiller)
{
	if (pkKiller->GetDungeon()) // ´øÁ¯³»¿¡¼± ¸¸µÎ°¡³ª¿ÀÁö¾Ê´Â´Ù
		return;
}

static void GiveExp(LPCHARACTER from, LPCHARACTER to, int iExp)
{
	float fExpFactor = 1.0f;
#ifdef AELDRA	
    if (from->GetLevel() >= 1 && from->GetLevel() <= 30) 
        fExpFactor = 5.0;
	
    else if (from->GetLevel() >= 31 && from->GetLevel() <= 55) 
        fExpFactor = 2.5;
	
    else if (from->GetLevel() >= 56 && from->GetLevel() <= 74) 
        fExpFactor = 2.8;
	
	else if (from->GetLevel() >= 75 && from->GetLevel() <= 90) 
        fExpFactor = 2.0;
	
	else if (from->GetLevel() >= 91 && from->GetLevel() <= 105) 
        fExpFactor = 1.75;
#elif defined(ELONIA)
    if (from->GetLevel() >= 1 && from->GetLevel() <= 30) 
        fExpFactor = 2.5;
	
    else if (from->GetLevel() >= 31 && from->GetLevel() <= 55) 
        fExpFactor = 2.0;
	
    else if (from->GetLevel() >= 56 && from->GetLevel() <= 90) 
        fExpFactor = 1.0;
	
	else if (from->GetLevel() >= 91 && from->GetLevel() <= 99) 
        fExpFactor = 2;
	
	else if (from->GetLevel() >= 100 && from->GetLevel() <= 105) 
        fExpFactor = 2;
#endif
	iExp = abs(iExp * fExpFactor);
	
	iExp = CALCULATE_VALUE_LVDELTA(to->GetLevel(), from->GetLevel(), iExp);

	int iBaseExp = iExp;

#ifdef __FAKE_PRIV_BONI__
	iExp = iExp * (100 + (CPrivManager::instance().GetPriv(to, PRIV_EXP_PCT) / __FAKE_PRIV_BONI__)) / 100;
#else
	iExp = iExp * (100 + CPrivManager::instance().GetPriv(to, PRIV_EXP_PCT)) / 100;
#endif

#ifdef __ANIMAL_SYSTEM__
#ifdef __PET_SYSTEM__
	if (to->GetPetSystem())
	{
		if (CPetActor* pPetActor = to->GetPetSystem()->GetSummoned())
		{
			LPITEM pkSummonItem = to->FindItemByVID(pPetActor->GetSummonItemVID());
			if (pkSummonItem)
				pkSummonItem->Animal_GiveEXP(iExp);
		}
	}
#endif
	if (to->GetMountSystem() && (to->GetMountSystem()->IsRiding() || to->GetMountSystem()->IsSummoned()))
	{
		LPITEM pkItem = to->FindItemByID(to->GetMountSystem()->GetSummonItemID());
		if (pkItem && pkItem->Animal_IsAnimal())
			pkItem->Animal_GiveEXP(iExp);
	}
#endif


	{
		if (to->IsEquipUniqueItem(UNIQUE_ITEM_LARBOR_MEDAL))
			iExp += iExp * 20 /100;

		if (to->IsPrivateMap(DEVILTOWER_MAP_INDEX)) 
			iExp += iExp * 20 / 100; // 1.2¹è (20%)

		if (to->GetPoint(POINT_EXP_DOUBLE_BONUS))
			if (random_number(1, 100) <= to->GetPoint(POINT_EXP_DOUBLE_BONUS))
				iExp += iExp * 30 / 100; // 1.3¹è (30%)

		if (to->GetPoint(POINT_EXP_REAL_BONUS))
			iExp += iExp * to->GetPoint(POINT_EXP_REAL_BONUS) / 100;

		if (to->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_EXP))
			iExp += iExp * 50 / 100;

		if (to->IsEquipUniqueItem(UNIQUE_ITEM_75EXP_NEW))
			iExp += iExp * 75 / 100;

		switch (to->GetMountVnum())
		{
			case 20110:
			case 20111:
			case 20112:
			case 20113:
				if (to->IsEquipUniqueItem(71115) || to->IsEquipUniqueItem(71117) || to->IsEquipUniqueItem(71119) ||
						to->IsEquipUniqueItem(71121) )
				{
					iExp += iExp * 10 / 100;
				}
				break;

			case 20114:
			case 20120:
			case 20121:
			case 20122:
			case 20123:
			case 20124:
			case 20125:
				// ¹é»çÀÚ °æÇèÄ¡ º¸³Ê½º
				iExp += iExp * 30 / 100;
				break;
		}
	}

	// ¾ÆÀÌÅÛ ¸ô: °æÇèÄ¡ °áÁ¦
	if (to->GetPremiumRemainSeconds(PREMIUM_EXP) > 0)
	{
		iExp += (iExp * 50 / 100);
	}

	if (to->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_EXP) == true)
	{
		iExp += (iExp * 50 / 100);
	}

	// °áÈ¥ º¸³Ê½º
	iExp += iExp * to->GetMarriageBonus(UNIQUE_ITEM_MARRIAGE_EXP_BONUS) / 100;

	iExp += (iExp * to->GetPoint(POINT_RAMADAN_CANDY_BONUS_EXP)/100);
	iExp += (iExp * to->GetPoint(POINT_MALL_EXPBONUS)/100);
	iExp += (iExp * to->GetPoint(POINT_EXP)/100);

	iExp = iExp * CHARACTER_MANAGER::instance().GetMobExpRate(to) / 100;

#ifdef __EXP_DOUBLE_UNTIL30__
	if (to->GetLevel() < 30)
		iExp += iExp;
#endif

	iExp = MIN(to->GetNextExp() / 10, iExp);

	if (test_server)
	{
		if (quest::CQuestManager::instance().GetEventFlag("exp_bonus_log") && iBaseExp>0)
			to->ChatPacket(CHAT_TYPE_INFO, "exp bonus %d%%", (iExp-iBaseExp)*100/iBaseExp);
	}

	iExp = AdjustExpByLevel(to, iExp);

#ifdef __PRESTIGE__
	if (to->GetPrestigeLevel() > 0)
	{
		if (test_server)
			to->ChatPacket(CHAT_TYPE_INFO, "prestige: exp %d -> %d", iExp, iExp / 2);

		iExp = iExp / 2;
	}
#endif

	to->PointChange(POINT_EXP, iExp, true);
	from->CreateFly(FLY_EXP, to);

	{
		LPCHARACTER you = to->GetMarryPartner();
		// ºÎºÎ°¡ ¼­·Î ÆÄÆ¼ÁßÀÌ¸é ±Ý½½ÀÌ ¿À¸¥´Ù
		if (you)
		{
			// 1¾ïÀÌ 100%
			DWORD dwUpdatePoint = 2000*iExp/to->GetLevel()/to->GetLevel()/3;

			if (to->GetPremiumRemainSeconds(PREMIUM_MARRIAGE_FAST) > 0 || 
					you->GetPremiumRemainSeconds(PREMIUM_MARRIAGE_FAST) > 0)
				dwUpdatePoint = (DWORD)(dwUpdatePoint * 3);

			marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(to->GetPlayerID());

			// DIVORCE_NULL_BUG_FIX
			if (pMarriage && pMarriage->IsNear())
				pMarriage->Update(dwUpdatePoint);
			// END_OF_DIVORCE_NULL_BUG_FIX
		}
	}
}

namespace NPartyExpDistribute
{
	struct FPartyTotaler
	{
		int		total;
		int		member_count;
		int		x, y;

		FPartyTotaler(LPCHARACTER center)
			: total(0), member_count(0), x(center->GetX()), y(center->GetY())
		{};

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
			{
				total += party_exp_distribute_table[ch->GetLevel()];

				++member_count;
			}
		}
	};

	struct FPartyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		DWORD		_iExp;
		int		m_iMode;
		int		m_iMemberCount;

		FPartyDistributor(LPCHARACTER center, int member_count, int total, DWORD iExp, int iMode) 
			: total(total), c(center), x(center->GetX()), y(center->GetY()), _iExp(iExp), m_iMode(iMode), m_iMemberCount(member_count)
			{
				if (m_iMemberCount == 0)
					m_iMemberCount = 1;
			};

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
			{
				DWORD iExp2 = 0;

				switch (m_iMode)
				{
					case PARTY_EXP_DISTRIBUTION_NON_PARITY:
						iExp2 = (DWORD) (_iExp * (float) party_exp_distribute_table[ch->GetLevel()] / total);
						break;

					case PARTY_EXP_DISTRIBUTION_PARITY:
						iExp2 = _iExp / m_iMemberCount;
						break;

					default:
						sys_err("Unknown party exp distribution mode %d", m_iMode);
						return;
				}

				GiveExp(c, ch, iExp2);
			}
		}
	};
}

typedef struct SDamageInfo
{
	int iDam;
	LPCHARACTER pAttacker;
	LPPARTY pParty;

	void Clear()
	{
		pAttacker = NULL;
		pParty = NULL;
	}

	inline void Distribute(LPCHARACTER ch, int iExp)
	{
		if (pAttacker)
			GiveExp(ch, pAttacker, iExp);
		else if (pParty)
		{
			NPartyExpDistribute::FPartyTotaler f(ch);
			pParty->ForEachOnlineMember(f);

			if (pParty->IsPositionNearLeader(ch))
				iExp = iExp * (100 + pParty->GetExpBonusPercent()) / 100;

			if (test_server)
			{
				if (quest::CQuestManager::instance().GetEventFlag("exp_bonus_log") && pParty->GetExpBonusPercent())
					pParty->ChatPacketToAllMember(CHAT_TYPE_INFO, "exp party bonus %d%%", pParty->GetExpBonusPercent());
			}

			// °æÇèÄ¡ ¸ô¾ÆÁÖ±â (ÆÄÆ¼°¡ È¹µæÇÑ °æÇèÄ¡¸¦ 5% »©¼­ ¸ÕÀú ÁÜ)
			if (pParty->GetExpCentralizeCharacter())
			{
				LPCHARACTER tch = pParty->GetExpCentralizeCharacter();

				if (DISTANCE_APPROX(ch->GetX() - tch->GetX(), ch->GetY() - tch->GetY()) <= PARTY_DEFAULT_RANGE)
				{
					int iExpCenteralize = (int) (iExp * 0.05f);
					iExp -= iExpCenteralize;

					GiveExp(ch, pParty->GetExpCentralizeCharacter(), iExpCenteralize);
				}
			}

			NPartyExpDistribute::FPartyDistributor fDist(ch, f.member_count, f.total, iExp, pParty->GetExpDistributionMode());
			pParty->ForEachOnlineMember(fDist);
		}
	}
} TDamageInfo;

LPCHARACTER CHARACTER::DistributeExp()
{
	int iExpToDistribute = GetExp();

	if (iExpToDistribute <= 0)
		return NULL;

	int	iTotalDam = 0;
	LPCHARACTER pkChrMostAttacked = NULL;
	int iMostDam = 0;

	typedef std::vector<TDamageInfo> TDamageInfoTable;
	TDamageInfoTable damage_info_table;
	std::map<LPPARTY, TDamageInfo> map_party_damage;

	damage_info_table.reserve(m_map_kDamage.size());

	TDamageMap::iterator it = m_map_kDamage.begin();

	// ÀÏ´Ü ÁÖÀ§¿¡ ¾ø´Â »ç¶÷À» °É·¯ ³½´Ù. (50m)
	while (it != m_map_kDamage.end())
	{
		const VID & c_VID = it->first;
		int iDam = it->second.iTotalDamage;
#ifdef __FAKE_PC__
		iDam += it->second.iTotalFakePCDamage;
#endif

		++it;

		LPCHARACTER pAttacker = CHARACTER_MANAGER::instance().Find(c_VID);

		// NPC°¡ ¶§¸®±âµµ ÇÏ³ª? -.-;
		if (!pAttacker || pAttacker->IsNPC() || DISTANCE_APPROX(GetX()-pAttacker->GetX(), GetY()-pAttacker->GetY())>5000)
			continue;

		iTotalDam += iDam;
		if (!pkChrMostAttacked || iDam > iMostDam)
		{
			pkChrMostAttacked = pAttacker;
			iMostDam = iDam;
		}

		if (pAttacker->GetParty())
		{
			std::map<LPPARTY, TDamageInfo>::iterator it = map_party_damage.find(pAttacker->GetParty());
			if (it == map_party_damage.end())
			{
				TDamageInfo di;
				di.iDam = iDam;
				di.pAttacker = NULL;
				di.pParty = pAttacker->GetParty();
				map_party_damage.insert(std::make_pair(di.pParty, di));
			}
			else
			{
				it->second.iDam += iDam;
			}
		}
		else
		{
			TDamageInfo di;

			di.iDam = iDam;
			di.pAttacker = pAttacker;
			di.pParty = NULL;

			//sys_log(0, "__ pq_damage %s %d", pAttacker->GetName(), iDam);
			//pq_damage.push(di);
			damage_info_table.push_back(di);
		}
	}

	for (std::map<LPPARTY, TDamageInfo>::iterator it = map_party_damage.begin(); it != map_party_damage.end(); ++it)
	{
		damage_info_table.push_back(it->second);
		//sys_log(0, "__ pq_damage_party [%u] %d", it->second.pParty->GetLeaderPID(), it->second.iDam);
	}

	SetExp(0);
	//m_map_kDamage.clear();

	if (iTotalDam == 0)	// µ¥¹ÌÁö ÁØ°Ô 0ÀÌ¸é ¸®ÅÏ
		return NULL;

	if (m_pkChrStone)	// µ¹ÀÌ ÀÖÀ» °æ¿ì °æÇèÄ¡ÀÇ ¹ÝÀ» µ¹¿¡°Ô ³Ñ±ä´Ù.
	{
		//sys_log(0, "__ Give half to Stone : %d", iExpToDistribute>>1);
		int iExp = iExpToDistribute >> 1;
		m_pkChrStone->SetExp(m_pkChrStone->GetExp() + iExp);
		iExpToDistribute -= iExp;
	}

	sys_log(1, "%s total exp: %d, damage_info_table.size() == %d, TotalDam %d",
			GetName(), iExpToDistribute, damage_info_table.size(), iTotalDam);
	//sys_log(1, "%s total exp: %d, pq_damage.size() == %d, TotalDam %d",
	//GetName(), iExpToDistribute, pq_damage.size(), iTotalDam);

	if (damage_info_table.empty())
		return NULL;

	// Á¦ÀÏ µ¥¹ÌÁö¸¦ ¸¹ÀÌ ÁØ »ç¶÷ÀÌ HP È¸º¹À» ÇÑ´Ù.
	DistributeHP(pkChrMostAttacked);	// ¸¸µÎ ½Ã½ºÅÛ

	{
		// Á¦ÀÏ µ¥¹ÌÁö¸¦ ¸¹ÀÌ ÁØ »ç¶÷ÀÌ³ª ÆÄÆ¼°¡ ÃÑ °æÇèÄ¡ÀÇ 20% + ÀÚ±â°¡ ¶§¸°¸¸Å­ÀÇ °æÇèÄ¡¸¦ ¸Ô´Â´Ù.
		TDamageInfoTable::iterator di = damage_info_table.begin();
		{
			TDamageInfoTable::iterator it;

			for (it = damage_info_table.begin(); it != damage_info_table.end();++it)
			{
				if (it->iDam > di->iDam)
					di = it;
			}
		}

		int	iExp = iExpToDistribute / 5;
		iExpToDistribute -= iExp;

		float fPercent = (float) di->iDam / iTotalDam;

		if (fPercent > 1.0f)
		{
			sys_err("DistributeExp percent over 1.0 (fPercent %f name %s)", fPercent, di->pAttacker->GetName());
			fPercent = 1.0f;
		}

		iExp += (int) (iExpToDistribute * fPercent);

		//sys_log(0, "%s given exp percent %.1f + 20 dam %d", GetName(), fPercent * 100.0f, di.iDam);

		di->Distribute(this, iExp);

		// 100% ´Ù ¸Ô¾úÀ¸¸é ¸®ÅÏÇÑ´Ù.
		if (fPercent == 1.0f)
			return pkChrMostAttacked;

		di->Clear();
	}

	{
		// ³²Àº 80%ÀÇ °æÇèÄ¡¸¦ ºÐ¹èÇÑ´Ù.
		TDamageInfoTable::iterator it;

		for (it = damage_info_table.begin(); it != damage_info_table.end(); ++it)
		{
			TDamageInfo & di = *it;

			float fPercent = (float) di.iDam / iTotalDam;

			if (fPercent > 1.0f)
			{
				sys_err("DistributeExp percent over 1.0 (fPercent %f name %s)", fPercent, di.pAttacker->GetName());
				fPercent = 1.0f;
			}

			//sys_log(0, "%s given exp percent %.1f dam %d", GetName(), fPercent * 100.0f, di.iDam);
			di.Distribute(this, (int) (iExpToDistribute * fPercent));
		}
	}

	return pkChrMostAttacked;
}

// È­»ì °³¼ö¸¦ ¸®ÅÏÇØ ÁÜ
int CHARACTER::GetArrowAndBow(LPITEM * ppkBow, LPITEM * ppkArrow, int iArrowCount/* = 1 */)
{
	LPITEM pkBow;

	if (!(pkBow = GetWear(WEAR_WEAPON)) || pkBow->GetProto()->sub_type() != WEAPON_BOW)
	{
		return 0;
	}

	LPITEM pkArrow;

	if (!(pkArrow = GetWear(WEAR_ARROW)) || pkArrow->GetType() != ITEM_WEAPON ||
		(pkArrow->GetSubType() != WEAPON_ARROW && pkArrow->GetSubType() != WEAPON_QUIVER))
	{
		return 0;
	}

	if (pkArrow->GetSubType() == WEAPON_ARROW)
		iArrowCount = MIN(iArrowCount, pkArrow->GetCount());
	else
		iArrowCount = MIN(iArrowCount, pkArrow->GetSocket(1));

	*ppkBow = pkBow;
	*ppkArrow = pkArrow;

	return iArrowCount;
}

void CHARACTER::UseArrow(LPITEM pkArrow, DWORD dwArrowCount)
{
	int iCount = pkArrow->GetCount();
	DWORD dwVnum = pkArrow->GetVnum();

	if (pkArrow->GetSubType() == WEAPON_QUIVER)
	{
		dwVnum = pkArrow->GetSocket(0);
		iCount = pkArrow->GetSocket(1);
	}

	iCount = iCount - MIN(iCount, dwArrowCount);

	DWORD dwSubType = pkArrow->GetSubType();
	
	if (pkArrow->GetSubType() == WEAPON_QUIVER)
		pkArrow->SetSocket(1, iCount, false);
	else
		pkArrow->SetCount(iCount);

	if (iCount == 0)
	{
		LPITEM pkNewArrow = FindSpecifyItem(dwVnum);
		
		sys_log(0, "UseArrow : FindSpecifyItem %u %p", dwVnum, get_pointer(pkNewArrow));

		if (dwSubType == WEAPON_QUIVER)
		{
			if (pkNewArrow)
			{
				int iMaxCount = MIN(pkNewArrow->GetCount(), pkArrow->GetValue(0));
				pkNewArrow->SetCount(pkNewArrow->GetCount() - iMaxCount);
				pkArrow->SetSocket(1, iMaxCount);
			}
			else
				pkArrow->SetSocket(0, 0);
		}
		else
		{
			if (pkNewArrow)
				EquipItem(pkNewArrow);
		}
	}
}

class CFuncShoot
{
	public:
		LPCHARACTER	m_me;
		BYTE		m_bType;
		bool		m_bSucceed;

		CFuncShoot(LPCHARACTER ch, BYTE bType) : m_me(ch), m_bType(bType), m_bSucceed(FALSE)
		{
		}

		void operator () (DWORD dwTargetVID)
		{
			if (m_bType > 1)
			{
				m_me->m_SkillUseInfo[m_bType].SetMainTargetVID(dwTargetVID);
				/*if (m_bType == SKILL_BIPABU || m_bType == SKILL_KWANKYEOK)
				  m_me->m_SkillUseInfo[m_bType].ResetHitCount();*/
			}

			LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(dwTargetVID);

			if (!pkVictim)
				return;

			// °ø°Ý ºÒ°¡
			if (!battle_is_attackable(m_me, pkVictim))
				return;

			if (m_me->IsNPC())
			{
				if (DISTANCE_APPROX(m_me->GetX() - pkVictim->GetX(), m_me->GetY() - pkVictim->GetY()) > 5000)
					return;
			}

			LPITEM pkBow, pkArrow;

			switch (m_bType)
			{
				case 0: // ÀÏ¹ÝÈ°
					{
						int iDam = 0;

#ifdef __FAKE_PC__
						if (m_me->IsPC() || m_me->FakePC_Check())
#else
						if (m_me->IsPC())
#endif
						{
							if (m_me->GetJob() != JOB_ASSASSIN)
								return;

							if (0 == m_me->GetArrowAndBow(&pkBow, &pkArrow))
								return;

							if (m_me->GetSkillGroup() != 0)
								if (!m_me->IsNPC() && m_me->GetSkillGroup() != 2)
								{
									if (m_me->GetSP() < 5)
										return;

									m_me->PointChange(POINT_SP, -5);
								}

							iDam = CalcArrowDamage(m_me, pkVictim, pkBow, pkArrow);
							m_me->UseArrow(pkArrow, 1);

							// check speed hack
							DWORD	dwCurrentTime	= get_dword_time();
							if (IS_SPEED_HACK(m_me, pkVictim, dwCurrentTime))
								iDam	= 0;
						}
						else
							iDam = CalcMeleeDamage(m_me, pkVictim);

						NormalAttackAffect(m_me, pkVictim);

						// µ¥¹ÌÁö °è»ê
						iDam = iDam * (100 - pkVictim->GetPoint(POINT_RESIST_BOW)) / 100;

						//sys_log(0, "%s arrow %s dam %d", m_me->GetName(), pkVictim->GetName(), iDam);

						m_me->OnMove(true, pkVictim->IsPC());
						pkVictim->OnMove(false, m_me->IsPC());

						if (pkVictim->CanBeginFight())
							pkVictim->BeginFight(m_me);

						pkVictim->Damage(m_me, iDam, DAMAGE_TYPE_NORMAL_RANGE);
						// Å¸°ÝÄ¡ °è»êºÎ ³¡
					}
					break;

				case 1: // ÀÏ¹Ý ¸¶¹ý
					{
						int iDam;

						if (m_me->IsPC())
							return;

						iDam = CalcMagicDamage(m_me, pkVictim);

						NormalAttackAffect(m_me, pkVictim);

						// µ¥¹ÌÁö °è»ê
						iDam = iDam * (100 - (pkVictim->GetPoint(POINT_RESIST_MAGIC) - m_me->GetPoint(POINT_ANTI_RESIST_MAGIC))) / 100;

						//sys_log(0, "%s arrow %s dam %d", m_me->GetName(), pkVictim->GetName(), iDam);

						m_me->OnMove(true, pkVictim->IsPC());
						pkVictim->OnMove(false, m_me->IsPC());

						if (pkVictim->CanBeginFight())
							pkVictim->BeginFight(m_me); 

						pkVictim->Damage(m_me, iDam, DAMAGE_TYPE_MAGIC);
						// Å¸°ÝÄ¡ °è»êºÎ ³¡
					}
					break;

				case SKILL_YEONSA:	// ¿¬»ç
					{
						//int iUseArrow = 2 + (m_me->GetSkillPower(SKILL_YEONSA) *6/100);
						int iUseArrow = 1;

						// ÅäÅ»¸¸ °è»êÇÏ´Â°æ¿ì
						{
							if (iUseArrow == m_me->GetArrowAndBow(&pkBow, &pkArrow, iUseArrow))
							{
								m_me->OnMove(true, pkVictim->IsPC());
								pkVictim->OnMove(false, m_me->IsPC());

								if (pkVictim->CanBeginFight())
									pkVictim->BeginFight(m_me); 

								m_me->ComputeSkill(m_bType, pkVictim);
								m_me->UseArrow(pkArrow, iUseArrow);

								if (pkVictim->IsDead())
									break;

							}
							else
								break;
						}
					}
					break;


				case SKILL_KWANKYEOK:
					{
						int iUseArrow = 1;

						if (iUseArrow == m_me->GetArrowAndBow(&pkBow, &pkArrow, iUseArrow))
						{
							m_me->OnMove(true, pkVictim->IsPC());
							pkVictim->OnMove(false, m_me->IsPC());

							if (pkVictim->CanBeginFight())
								pkVictim->BeginFight(m_me); 

							sys_log(0, "%s kwankeyok %s", m_me->GetName(), pkVictim->GetName());
							m_me->ComputeSkill(m_bType, pkVictim);
							m_me->UseArrow(pkArrow, iUseArrow);
						}
					}
					break;

				case SKILL_GIGUNG:
					{
						int iUseArrow = 1;
						if (iUseArrow == m_me->GetArrowAndBow(&pkBow, &pkArrow, iUseArrow))
						{
							m_me->OnMove(true, pkVictim->IsPC());
							pkVictim->OnMove(false, m_me->IsPC());

							if (pkVictim->CanBeginFight())
								pkVictim->BeginFight(m_me);

							sys_log(0, "%s gigung %s", m_me->GetName(), pkVictim->GetName());
							m_me->ComputeSkill(m_bType, pkVictim);
							m_me->UseArrow(pkArrow, iUseArrow);
						}
					}

					break;
				case SKILL_HWAJO:
					{
						int iUseArrow = 1;
						if (iUseArrow == m_me->GetArrowAndBow(&pkBow, &pkArrow, iUseArrow))
						{
							m_me->OnMove(true, pkVictim->IsPC());
							pkVictim->OnMove(false, m_me->IsPC());

							if (pkVictim->CanBeginFight())
								pkVictim->BeginFight(m_me);

							sys_log(0, "%s hwajo %s", m_me->GetName(), pkVictim->GetName());
							m_me->ComputeSkill(m_bType, pkVictim);
							m_me->UseArrow(pkArrow, iUseArrow);
						}
					}

					break;

				case SKILL_HORSE_WILDATTACK_RANGE:
					{
						int iUseArrow = 1;
						if (iUseArrow == m_me->GetArrowAndBow(&pkBow, &pkArrow, iUseArrow))
						{
							m_me->OnMove(true, pkVictim->IsPC());
							pkVictim->OnMove(false, m_me->IsPC());

							if (pkVictim->CanBeginFight())
								pkVictim->BeginFight(m_me);

							sys_log(0, "%s horse_wildattack %s", m_me->GetName(), pkVictim->GetName());
							m_me->ComputeSkill(m_bType, pkVictim);
							m_me->UseArrow(pkArrow, iUseArrow);
						}
					}

					break;

				case SKILL_MARYUNG:
					//case SKILL_GUMHWAN:
				case SKILL_TUSOK:
				case SKILL_BIPABU:
				case SKILL_NOEJEON:
				case SKILL_GEOMPUNG:
				case SKILL_SANGONG:
				case SKILL_MAHWAN:
				case SKILL_PABEOB:
					//case SKILL_CURSE:
					{
						m_me->OnMove(true, pkVictim->IsPC());
						pkVictim->OnMove(false, m_me->IsPC());

						if (pkVictim->CanBeginFight())
							pkVictim->BeginFight(m_me);

						sys_log(0, "%s - Skill %d -> %s", m_me->GetName(), m_bType, pkVictim->GetName());
						m_me->ComputeSkill(m_bType, pkVictim);
					}
					break;

				case SKILL_CHAIN:
					{
						m_me->OnMove(true, pkVictim->IsPC());
						pkVictim->OnMove(false, m_me->IsPC());

						if (pkVictim->CanBeginFight())
							pkVictim->BeginFight(m_me); 

						sys_log(0, "%s - Skill %d -> %s", m_me->GetName(), m_bType, pkVictim->GetName());
						m_me->ComputeSkill(m_bType, pkVictim);

						// TODO ¿©·¯¸í¿¡°Ô ½µ ½µ ½µ ÇÏ±â
					}
					break;

				case SKILL_YONGBI:
					{
						m_me->OnMove(true);
					}
					break;

					/*case SKILL_BUDONG:
					  {
					  m_me->OnMove(true);
					  pkVictim->OnMove();

					  DWORD * pdw;
					  DWORD dwEI = AllocEventInfo(sizeof(DWORD) * 2, &pdw);
					  pdw[0] = m_me->GetVID();
					  pdw[1] = pkVictim->GetVID();

					  event_create(budong_event_func, dwEI, PASSES_PER_SEC(1));
					  }
					  break;*/

				default:
					sys_err("CFuncShoot: I don't know this type [%d] of range attack.", (int) m_bType);
					break;
			}

			m_bSucceed = TRUE;
		}
};

bool CHARACTER::Shoot(BYTE bType)
{
	sys_log(1, "Shoot %s type %u flyTargets.size %zu", GetName(), bType, m_vec_dwFlyTargets.size());

	if (!CanMove())
	{
		return false;
	}	

	CFuncShoot f(this, bType);

	if (m_dwFlyTargetID != 0)
	{
		f(m_dwFlyTargetID);
		m_dwFlyTargetID = 0;
	}

	f = std::for_each(m_vec_dwFlyTargets.begin(), m_vec_dwFlyTargets.end(), f);
	m_vec_dwFlyTargets.clear();

	return f.m_bSucceed;
}

void CHARACTER::FlyTarget(DWORD dwTargetVID, long x, long y, bool is_add)
{
	LPCHARACTER pkVictim = CHARACTER_MANAGER::instance().Find(dwTargetVID);

	network::GCOutputPacket<network::GCFlyTargetingPacket> pack;
	network::GCOutputPacket<network::GCAddFlyTargetingPacket> pack2;

	pack->set_shooter_vid(GetVID());
	pack2->set_shooter_vid(GetVID());

	if (pkVictim)
	{
		pack->set_target_vid(pkVictim->GetVID());
		pack2->set_target_vid(pkVictim->GetVID());
		pack->set_x(pkVictim->GetX());
		pack2->set_x(pkVictim->GetX());
		pack->set_y(pkVictim->GetY());
		pack2->set_y(pkVictim->GetY());

		if (!is_add)
			m_dwFlyTargetID = dwTargetVID;
		else
			m_vec_dwFlyTargets.push_back(dwTargetVID);
	}
	else
	{
		pack->set_x(x);
		pack2->set_x(x);
		pack->set_y(y);
		pack2->set_y(y);
	}

	if (!is_add)
		PacketAround(pack, this);
	else
		PacketAround(pack2, this);
}

LPCHARACTER CHARACTER::GetNearestVictim(LPCHARACTER pkChr)
{
	if (NULL == pkChr)
		pkChr = this;

	float fMinDist = 99999.0f;
	LPCHARACTER pkVictim = NULL;

	TDamageMap::iterator it = m_map_kDamage.begin();

	// ÀÏ´Ü ÁÖÀ§¿¡ ¾ø´Â »ç¶÷À» °É·¯ ³½´Ù.
	while (it != m_map_kDamage.end())
	{
		const VID & c_VID = it->first;
		++it;

		LPCHARACTER pAttacker = CHARACTER_MANAGER::instance().Find(c_VID);

		if (!pAttacker)
			continue;

		if (pAttacker->IsAffectFlag(AFF_EUNHYUNG) || 
				pAttacker->IsAffectFlag(AFF_INVISIBILITY) ||
				pAttacker->IsAffectFlag(AFF_REVIVE_INVISIBLE))
			continue;

		float fDist = DISTANCE_APPROX(pAttacker->GetX() - pkChr->GetX(), pAttacker->GetY() - pkChr->GetY());

		if (fDist < fMinDist)
		{
			pkVictim = pAttacker;
			fMinDist = fDist;
		}
	}

	return pkVictim;
}

void CHARACTER::SetVictim(LPCHARACTER pkVictim)
{
	if (!pkVictim)
	{
		if (0 != (DWORD)m_kVIDVictim)
			MonsterLog("°ø°Ý ´ë»óÀ» ÇØÁ¦");

		m_kVIDVictim.Reset();
		battle_end(this);
	}
	else
	{
#ifdef ENABLE_HYDRA_DUNGEON
		//if (m_bLockTarget && m_kVIDVictim && m_kVIDVictim != pkVictim->GetVID())
		//	return;
#endif

		if (m_kVIDVictim != pkVictim->GetVID())
			MonsterLog("°ø°Ý ´ë»óÀ» ¼³Á¤: %s", pkVictim->GetName());

		m_kVIDVictim = pkVictim->GetVID();
		m_dwLastVictimSetTime = get_dword_time();
	}
}

LPCHARACTER CHARACTER::GetVictim() const
{
	return CHARACTER_MANAGER::instance().Find(m_kVIDVictim);
}

LPCHARACTER CHARACTER::GetProtege() const // º¸È£ÇØ¾ß ÇÒ ´ë»óÀ» ¸®ÅÏ
{
	if (m_pkChrStone)
		return m_pkChrStone;

	if (m_pkParty)
		return m_pkParty->GetLeader();

#ifdef __FAKE_PC__
	if (FakePC_IsSupporter())
		return FakePC_GetOwner();
#endif

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		return FakeBuff_GetOwner();
#endif

	return NULL;
}

int CHARACTER::GetAlignment() const
{
	return m_iAlignment;
}

int CHARACTER::GetRealAlignment() const
{
	return m_iRealAlignment;
}

void CHARACTER::ShowAlignment(bool bShow)
{
	if (bShow)
	{
		if (m_iAlignment != m_iRealAlignment)
		{
			m_iAlignment = m_iRealAlignment;
			UpdatePacket();
		}
	}
	else
	{
		if (m_iAlignment != 0)
		{
			m_iAlignment = 0;
			UpdatePacket();
		}
	}
}

void CHARACTER::UpdateAlignment(int iAmount)
{
#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif
	bool bShow = false;

	if (m_iAlignment == m_iRealAlignment)
		bShow = true;

	int i = m_iAlignment / 10;

	m_iRealAlignment = MINMAX(-200000, m_iRealAlignment + iAmount, 200000);

	if (bShow)
	{
		m_iAlignment = m_iRealAlignment;

		if (i != m_iAlignment / 10)
			UpdatePacket();
	}
}

void CHARACTER::SetKillerMode(bool isOn)
{
	if ((isOn ? ADD_CHARACTER_STATE_KILLER : 0) == IS_SET(m_bAddChrState, ADD_CHARACTER_STATE_KILLER))
		return;

	if (isOn)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);
	else
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);

	m_iKillerModePulse = thecore_pulse();
	UpdatePacket();
	sys_log(0, "SetKillerMode Update %s[%d]", GetName(), GetPlayerID());
}

bool CHARACTER::IsKillerMode() const
{
	return IS_SET(m_bAddChrState, ADD_CHARACTER_STATE_KILLER);
}

void CHARACTER::UpdateKillerMode()
{
	if (!IsKillerMode())
		return;

	int iKillerSeconds = 30;

	if (thecore_pulse() - m_iKillerModePulse >= PASSES_PER_SEC(iKillerSeconds))
		SetKillerMode(false);
}

void CHARACTER::SetPKMode(BYTE bPKMode)
{
	if (bPKMode >= PK_MODE_MAX_NUM)
		return;

	if (GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
		bPKMode = PK_MODE_FREE;

#ifdef __MELEY_LAIR_DUNGEON__
	if ((MeleyLair::CMgr::instance().IsMeleyMap(GetMapIndex())) && (bPKMode != PK_MODE_GUILD))
		bPKMode = PK_MODE_GUILD;
#endif

	if (m_bPKMode == bPKMode)
		return;

	if (bPKMode == PK_MODE_GUILD && !GetGuild())
		bPKMode = PK_MODE_FREE;

	m_bPKMode = bPKMode;
	UpdatePacket();

	sys_log(0, "PK_MODE: %s %d", GetName(), m_bPKMode);
}

BYTE CHARACTER::GetPKMode() const
{
	return m_bPKMode;
}

struct FuncForgetMyAttacker
{
	LPCHARACTER m_ch;
	FuncForgetMyAttacker(LPCHARACTER ch)
	{
		m_ch = ch;
	}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (ch->m_kVIDVictim == m_ch->GetVID())
				ch->SetVictim(NULL);
		}
	}
};

struct FuncAggregateMonster
{
	LPCHARACTER m_ch;
	bool m_bAllMonster;
	FuncAggregateMonster(LPCHARACTER ch, bool bAllMonster) : m_ch(ch), m_bAllMonster(bAllMonster)
	{
	}
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			if (ch->GetVictim())
				return;

			if (m_bAllMonster || random_number(1, 100) <= 50) // ÀÓ½Ã·Î 50% È®·ü·Î ÀûÀ» ²ø¾î¿Â´Ù
				if (DISTANCE_APPROX(ch->GetX() - m_ch->GetX(), ch->GetY() - m_ch->GetY()) < 5000)
					if (ch->CanBeginFight())
					{
						ch->BeginFight(m_ch);
#ifdef AGGREGATE_MONSTER_MOVESPEED
						ch->AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, AGGREGATE_MONSTER_MOVESPEED, AFF_MOV_SPEED_POTION, 7, 0, true, true);
#endif
					}
		}
	}
};

struct FuncAttractRanger
{
	LPCHARACTER m_ch;
	FuncAttractRanger(LPCHARACTER ch)
	{
		m_ch = ch;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			if (ch->GetVictim() && ch->GetVictim() != m_ch)
				return;
			if (ch->GetMobAttackRange() > 150)
			{
				int iNewRange = 150;//(int)(ch->GetMobAttackRange() * 0.2);
				if (iNewRange < 150)
					iNewRange = 150;

				ch->AddAffect(AFFECT_BOW_DISTANCE, POINT_BOW_DISTANCE, iNewRange - ch->GetMobAttackRange(), AFF_NONE, 3*60, 0, false);
			}
		}
	}
};

struct FuncPullMonster
{
	LPCHARACTER m_ch;
	int m_iLength;
	FuncPullMonster(LPCHARACTER ch, int iLength = 300)
	{
		m_ch = ch;
		m_iLength = iLength;
	}

	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsPC())
				return;
			if (!ch->IsMonster())
				return;
			//if (ch->GetVictim() && ch->GetVictim() != m_ch)
			//return;
			float fDist = DISTANCE_APPROX(m_ch->GetX() - ch->GetX(), m_ch->GetY() - ch->GetY());
			if (fDist > 3000 || fDist < 100)
				return;

			float fNewDist = fDist - m_iLength;
			if (fNewDist < 100) 
				fNewDist = 100;

			float degree = GetDegreeFromPositionXY(ch->GetX(), ch->GetY(), m_ch->GetX(), m_ch->GetY());
			float fx;
			float fy;

			GetDeltaByDegree(degree, fDist - fNewDist, &fx, &fy);
			long tx = (long)(ch->GetX() + fx);
			long ty = (long)(ch->GetY() + fy);

			ch->Sync(tx, ty);
			ch->Goto(tx, ty);
			ch->CalculateMoveDuration();

			ch->SyncPacket();
		}
	}
};

void CHARACTER::ForgetMyAttacker()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncForgetMyAttacker f(this);
		pSec->ForEachAround(f);
	}
	ReviveInvisible(5);
}

void CHARACTER::AggregateMonster(bool allMonster)
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncAggregateMonster f(this, allMonster);
		pSec->ForEachAround(f);
#ifdef __AGGREGATE_MONSTER_EFFECT__
		EffectPacket(SE_AGGREGATE_MONSTER_EFFECT);
#endif
	}
}

void CHARACTER::AttractRanger()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncAttractRanger f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::PullMonster()
{
	LPSECTREE pSec = GetSectree();
	if (pSec)
	{
		FuncPullMonster f(this);
		pSec->ForEachAround(f);
	}
}

void CHARACTER::UpdateAggrPointEx(LPCHARACTER pAttacker, EDamageType type, int dam, CHARACTER::TBattleInfo & info)
{
	// Æ¯Á¤ °ø°ÝÅ¸ÀÔ¿¡ µû¶ó ´õ ¿Ã¶ó°£´Ù
	switch (type)
	{
		case DAMAGE_TYPE_NORMAL_RANGE:
			dam = (int) (dam*1.2f);
			break;

		case DAMAGE_TYPE_RANGE:
			dam = (int) (dam*1.5f);
			break;

		case DAMAGE_TYPE_MAGIC:
			dam = (int) (dam*1.2f);
			break;

		default:
			break;
	}

	// °ø°ÝÀÚ°¡ ÇöÀç ´ë»óÀÎ °æ¿ì º¸³Ê½º¸¦ ÁØ´Ù.
	if (pAttacker == GetVictim())
		dam = (int) (dam * 1.2f);

	info.iAggro += dam;

	if (info.iAggro < 0)
		info.iAggro = 0;

	//sys_log(0, "UpdateAggrPointEx for %s by %s dam %d total %d", GetName(), pAttacker->GetName(), dam, total);
	if (GetParty() && dam > 0 && type != DAMAGE_TYPE_SPECIAL)
	{
		LPPARTY pParty = GetParty();

		// ¸®´õÀÎ °æ¿ì ¿µÇâ·ÂÀÌ Á»´õ °­ÇÏ´Ù
		int iPartyAggroDist = dam;

		if (pParty->GetLeaderPID() == GetVID())
			iPartyAggroDist /= 2;
		else
			iPartyAggroDist /= 3;

		pParty->SendMessage(this, PM_AGGRO_INCREASE, iPartyAggroDist, pAttacker->GetVID());
	}

	ChangeVictimByAggro(info.iAggro, pAttacker);
}

void CHARACTER::UpdateAggrPoint(LPCHARACTER pAttacker, EDamageType type, int dam)
{
	if (IsDead() || IsStun())
		return;

	TDamageMap::iterator it = m_map_kDamage.find(pAttacker->GetVID());

	if (it == m_map_kDamage.end())
	{
#ifdef __FAKE_PC__
		m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(0, 0, dam)));
#else
		m_map_kDamage.insert(TDamageMap::value_type(pAttacker->GetVID(), TBattleInfo(0, dam)));
#endif
		it = m_map_kDamage.find(pAttacker->GetVID());
	}

	UpdateAggrPointEx(pAttacker, type, dam, it->second);
}

void CHARACTER::ChangeVictimByAggro(int iNewAggro, LPCHARACTER pNewVictim)
{
	if (get_dword_time() - m_dwLastVictimSetTime < 3000) // 3ÃÊ´Â ±â´Ù·Á¾ßÇÑ´Ù
		return;

	if (pNewVictim == GetVictim())
	{
		if (m_iMaxAggro < iNewAggro)
		{
			m_iMaxAggro = iNewAggro;
			return;
		}

		// Aggro°¡ °¨¼ÒÇÑ °æ¿ì
		TDamageMap::iterator it;
		TDamageMap::iterator itFind = m_map_kDamage.end();

		for (it = m_map_kDamage.begin(); it != m_map_kDamage.end(); ++it)
		{
			if (it->second.iAggro > iNewAggro)
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(it->first);

				if (ch && !ch->IsDead() && DISTANCE_APPROX(ch->GetX() - GetX(), ch->GetY() - GetY()) < 5000)
				{
					itFind = it;
					iNewAggro = it->second.iAggro;
				}
			}
		}

		if (itFind != m_map_kDamage.end())
		{
			m_iMaxAggro = iNewAggro;
			SetVictim(CHARACTER_MANAGER::instance().Find(itFind->first));
			m_dwStateDuration = 1;
		}
	}
#ifdef ENABLE_HYDRA_DUNGEON
	else if (!m_bLockTarget)
#else
	else
#endif
	{
		if (m_iMaxAggro < iNewAggro)
		{
			m_iMaxAggro = iNewAggro;
			SetVictim(pNewVictim);
			m_dwStateDuration = 1;
		}
	}
}

