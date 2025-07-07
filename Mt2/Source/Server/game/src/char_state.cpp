#include "stdafx.h"
#include "config.h"
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "battle.h"
#include "char_manager.h"
#include "packet.h"
#include "motion.h"
#include "party.h"
#include "affect.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "p2p.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "exchange.h"
#include "sectree_manager.h"
#include "xmas_event.h"
#include "guild_manager.h"
#include "war_map.h"
#include "BlueDragon.h"
#include "item.h"

#include "../../common/VnumHelper.h"

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

BOOL g_test_server;
extern LPCHARACTER FindMobVictim(LPCHARACTER pkChr, int iMaxDistance);
#ifdef __FAKE_PC__
extern LPCHARACTER FindFakePCVictim(LPCHARACTER pkChr);
#endif

namespace
{
	class FuncFindChrForFlag
	{
		public:
			FuncFindChrForFlag(LPCHARACTER pkChr) :
				m_pkChr(pkChr), m_pkChrFind(NULL), m_iMinDistance(INT_MAX)
				{
				}

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				if (ent->IsObserverMode())
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				if (!pkChr->GetGuild())
					return;

				if (pkChr->IsDead())
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX()-m_pkChr->GetX(), pkChr->GetY()-m_pkChr->GetY());

				if (iDist <= 500 && m_iMinDistance > iDist &&
						!pkChr->IsAffectFlag(AFF_WAR_FLAG1) &&
						!pkChr->IsAffectFlag(AFF_WAR_FLAG2) &&
						!pkChr->IsAffectFlag(AFF_WAR_FLAG3))
				{
					// ¿ì¸®Æí ±ê¹ßÀÏ °æ¿ì
					if ((DWORD) m_pkChr->GetPoint(POINT_STAT) == pkChr->GetGuild()->GetID())
					{
						CWarMap * pMap = pkChr->GetWarMap();
						BYTE idx;

						if (!pMap || !pMap->GetTeamIndex(pkChr->GetGuild()->GetID(), idx))
							return;

						// ¿ì¸®Æí ±âÁö¿¡ ±ê¹ßÀÌ ¾øÀ» ¶§¸¸ ±ê¹ßÀ» »Ì´Â´Ù. ¾È±×·¯¸é ±âÁö¿¡ ÀÖ´Â ±ê¹ßÀ»
						// °¡¸¸È÷ µÎ°í ½ÍÀºµ¥µµ »ÌÈú¼ö°¡ ÀÖÀ¸¹Ç·Î..
						if (!pMap->IsFlagOnBase(idx))
						{
							m_pkChrFind = pkChr;
							m_iMinDistance = iDist;
						}
					}
					else
					{
						// »ó´ëÆí ±ê¹ßÀÎ °æ¿ì ¹«Á¶°Ç »Ì´Â´Ù.
						m_pkChrFind = pkChr;
						m_iMinDistance = iDist;
					}
				}
			}

			LPCHARACTER	m_pkChr;
			LPCHARACTER m_pkChrFind;
			int		m_iMinDistance;
	};

	class FuncFindChrForFlagBase
	{
		public:
			FuncFindChrForFlagBase(LPCHARACTER pkChr) : m_pkChr(pkChr)
			{
			}

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				if (ent->IsObserverMode())
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				CGuild * pkGuild = pkChr->GetGuild();

				if (!pkGuild)
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX()-m_pkChr->GetX(), pkChr->GetY()-m_pkChr->GetY());

				if (iDist <= 500 &&
						(pkChr->IsAffectFlag(AFF_WAR_FLAG1) || 
						 pkChr->IsAffectFlag(AFF_WAR_FLAG2) ||
						 pkChr->IsAffectFlag(AFF_WAR_FLAG3)))
				{
					CAffect * pkAff = pkChr->FindAffect(AFFECT_WAR_FLAG);

					sys_log(0, "FlagBase %s dist %d aff %p flag gid %d chr gid %u",
							pkChr->GetName(), iDist, pkAff, m_pkChr->GetPoint(POINT_STAT),
							pkChr->GetGuild()->GetID());

					if (pkAff)
					{
						if ((DWORD) m_pkChr->GetPoint(POINT_STAT) == pkGuild->GetID() &&
								m_pkChr->GetPoint(POINT_STAT) != pkAff->lApplyValue)
						{
							CWarMap * pMap = pkChr->GetWarMap();
							BYTE idx;

							if (!pMap || !pMap->GetTeamIndex(pkGuild->GetID(), idx))
								return;

							//if (pMap->IsFlagOnBase(idx))
							{
								BYTE idx_opp = idx == 0 ? 1 : 0;

								SendGuildWarScore(m_pkChr->GetPoint(POINT_STAT), pkAff->lApplyValue, 1);
								//SendGuildWarScore(pkAff->lApplyValue, m_pkChr->GetPoint(POINT_STAT), -1);

								pMap->ResetFlag();
								//pMap->AddFlag(idx_opp);
								//pkChr->RemoveAffect(AFFECT_WAR_FLAG);

								char buf[256];
								snprintf(buf, sizeof(buf), "[%s] ±æµå°¡ [%s] ±æµåÀÇ ±ê¹ßÀ» »©¾Ñ¾Ò½À´Ï´Ù!", pMap->GetGuild(idx)->GetName(), pMap->GetGuild(idx_opp)->GetName());
								pMap->Notice(buf);
							}
						}
					}
				}
			}

			LPCHARACTER m_pkChr;
	};

	class FuncFindGuardVictim
	{
		public:
			FuncFindGuardVictim(LPCHARACTER pkChr, int iMaxDistance) :
				m_pkChr(pkChr),
			m_iMinDistance(INT_MAX),
			m_iMaxDistance(iMaxDistance),
			m_lx(pkChr->GetX()),
			m_ly(pkChr->GetY()),
			m_pkChrVictim(NULL)
			{
			};

			void operator () (LPENTITY ent)
			{
				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				// ÀÏ´Ü PC °ø°Ý¾ÈÇÔ
				if (pkChr->IsPC())
					return;


				if (pkChr->IsNPC() && !pkChr->IsMonster())
					return;

				if (pkChr->IsDead())
					return;

				if (pkChr->IsAffectFlag(AFF_EUNHYUNG) || 
						pkChr->IsAffectFlag(AFF_INVISIBILITY) ||
						pkChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
					return;

				// ¿Ö±¸´Â ÆÐ½º
#ifdef ENABLE_ZODIAC_TEMPLE
				// Nu va ataca urmatorul mob
				// Daca mobul nu este in lista si este atacat, acesta va deveni agro pe acel npc ?? wtf
				// if (pkChr->GetRaceNum() == 20432 || pkChr->GetRaceNum() == 20433 || pkChr->GetRaceNum() == 20434 || pkChr->GetRaceNum() == 20435 || pkChr->GetRaceNum() == 20436 || pkChr->GetRaceNum() == 20437 || pkChr->GetRaceNum() == 20438 || pkChr->GetRaceNum() == 20439 || pkChr->GetRaceNum() == 20440 || pkChr->GetRaceNum() == 20441 || pkChr->GetRaceNum() == 20442 || pkChr->GetRaceNum() == 20443 || pkChr->GetRaceNum() == 20444 || pkChr->GetRaceNum() == 20446 || pkChr->GetRaceNum() == 20447 || pkChr->GetRaceNum() == 20448 || pkChr->GetRaceNum() == 20449 || pkChr->GetRaceNum() == 20450 || pkChr->GetRaceNum() == 20451 || pkChr->GetRaceNum() == 20452 || pkChr->GetRaceNum() == 20453 || pkChr->GetRaceNum() == 20454 || pkChr->GetRaceNum() == 20455 || pkChr->GetRaceNum() == 20456 || pkChr->GetRaceNum() == 20457 || pkChr->GetRaceNum() == 20458 || pkChr->GetRaceNum() == 20459 || pkChr->GetRaceNum() == 20460 || pkChr->GetRaceNum() == 20461 || pkChr->GetRaceNum() == 20462)
				if ((pkChr->GetRaceNum() >= 6520) && (pkChr->GetRaceNum() <= 6528))
					return;
#endif

#ifdef __FAKE_PC__
				if (pkChr->FakePC_Check())
					return;
#endif

				int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

				if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
				{
					m_pkChrVictim = pkChr;
					m_iMinDistance = iDistance;
				}
			}

			LPCHARACTER GetVictim()
			{
				return (m_pkChrVictim);
			}

		private:
			LPCHARACTER	m_pkChr;

			int		m_iMinDistance;
			int		m_iMaxDistance;
			long	m_lx;
			long	m_ly;

			LPCHARACTER	m_pkChrVictim;
	};

	class FuncFindPlayerVictim
	{
	public:
		FuncFindPlayerVictim(long lBaseX, long lBaseY, int iMaxDistance) :
			m_iMinDistance(INT_MAX),
			m_iMaxDistance(iMaxDistance),
			m_lx(lBaseX),
			m_ly(lBaseY),
			m_pkChrVictim(NULL)
		{
		};

		void operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return;

			LPCHARACTER pkChr = (LPCHARACTER)ent;

			if (!pkChr->IsPC())
				return;

			if (pkChr->IsDead())
				return;

			if (pkChr->IsAffectFlag(AFF_EUNHYUNG) ||
				pkChr->IsAffectFlag(AFF_INVISIBILITY) ||
				pkChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
				return;

			int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

			if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
			{
				m_pkChrVictim = pkChr;
				m_iMinDistance = iDistance;
			}
		}

		LPCHARACTER GetVictim()
		{
			return (m_pkChrVictim);
		}

	private:
		int		m_iMinDistance;
		int		m_iMaxDistance;
		long	m_lx;
		long	m_ly;

		LPCHARACTER	m_pkChrVictim;
	};

}

bool CHARACTER::IsAggressive() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE);
}

void CHARACTER::SetAggressive()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_AGGRESSIVE);
}

bool CHARACTER::IsCoward() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_COWARD);
}

void CHARACTER::SetCoward()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_COWARD);
}

bool CHARACTER::IsBerserker() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_BERSERK);
}

bool CHARACTER::IsStoneSkinner() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_STONESKIN);
}

bool CHARACTER::IsGodSpeeder() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_GODSPEED);
}

bool CHARACTER::IsDeathBlower() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_DEATHBLOW);
}

bool CHARACTER::IsReviver() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_REVIVE);
}

void CHARACTER::CowardEscape()
{
	if (test_server)
		sys_log(0, "COWARD_ESCAPE : %u %s", (DWORD)GetVID(), GetName());

	int iDist[4] = {500, 1000, 3000, 5000};

	for (int iDistIdx = 2; iDistIdx >= 0; --iDistIdx)
		for (int iTryCount = 0; iTryCount < 8; ++iTryCount)
		{
			SetRotation(random_number(0, 359));		// ¹æÇâÀº ·£´ýÀ¸·Î ¼³Á¤

			float fx, fy;
			float fDist = random_number(iDist[iDistIdx], iDist[iDistIdx+1]);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			bool bIsWayBlocked = false;
			for (int j = 1; j <= 100; ++j)
			{
				if (!SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx*j/100, GetY() + (int) fy*j/100))
				{
					bIsWayBlocked = true;
					break;
				}
			}

			if (bIsWayBlocked)
				continue;

			m_dwStateDuration = PASSES_PER_SEC(1);

			int iDestX = GetX() + (int) fx;
			int iDestY = GetY() + (int) fy;

			if (Goto(iDestX, iDestY))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			sys_log(0, "WAEGU move to %d %d (far)", iDestX, iDestY);
			return;
		}
}

void  CHARACTER::SetNoAttackShinsu()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKSHINSU);
}
bool CHARACTER::IsNoAttackShinsu() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKSHINSU);
}

void CHARACTER::SetNoAttackChunjo()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKCHUNJO);
}

bool CHARACTER::IsNoAttackChunjo() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKCHUNJO);
}

void CHARACTER::SetNoAttackJinno()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKJINNO);
}

bool CHARACTER::IsNoAttackJinno() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOATTACKJINNO);
}

void CHARACTER::SetAttackMob()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_ATTACKMOB);
}

bool CHARACTER::IsAttackMob() const
{
	return IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_ATTACKMOB);
}

void CHARACTER::SetNoMove()
{
	SET_BIT(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE);
}

bool CHARACTER::CanNPCFollowTarget(LPCHARACTER pkTarget)
{
	if (!IsNPC() && !IsMonster())
		return true;

	if (IS_SET(GetAIFlag(), AIFLAG_NOMOVE))
	{
		if (!IS_SET(GetAIFlag(), AIFLAG_AGGRESSIVE))
			return false;

		if (DISTANCE_APPROX(pkTarget->GetX() - m_posExit.x, pkTarget->GetY() - m_posExit.y) > GetMobTable().aggressive_sight())
			return false;
	}

	return true;
}

// STATE_IDLE_REFACTORING
void CHARACTER::StateIdle()
{
	if (IsStone())
	{
		__StateIdle_Stone();
		return;
	}
	else if (IsWarp() || IsGoto())
	{
		// ¿öÇÁ´Â ÀÌº¥Æ®·Î Ã³¸®
		m_dwStateDuration = 60 * passes_per_sec;
		return;
	}

#ifdef ENABLE_COMPANION_NAME
	if (IsPC())
	{
		if (m_petNameTimeLeft <= get_global_time() && !m_stPetName.empty())
			SetPetName("");

		if (m_mountNameTimeLeft <= get_global_time() && !m_stMountName.empty())
			SetMountName("");

		return;
	}
#else
	if (IsPC())
		return;
#endif

	// NPC Ã³¸®
	if (!IsMonster())
	{
		__StateIdle_NPC();
		return;
	}

	__StateIdle_Monster();
}

void CHARACTER::__StateIdle_Stone()
{
	m_dwStateDuration = PASSES_PER_SEC(1);

	int iPercent = (GetHP() * 100) / GetMaxHP();
	DWORD dwVnum = random_number(MIN(GetMobTable().attack_speed(), GetMobTable().moving_speed() ), MAX(GetMobTable().attack_speed(), GetMobTable().moving_speed()));

	if (iPercent <= 10 && GetMaxSP() < 10)
	{
		SetMaxSP(10);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 20 && GetMaxSP() < 9)
	{
		SetMaxSP(9);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1500, GetY() - 1500, GetX() + 1500, GetY() + 1500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 30 && GetMaxSP() < 8)
	{
		SetMaxSP(8);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 40 && GetMaxSP() < 7)
	{
		SetMaxSP(7);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 50 && GetMaxSP() < 6)
	{
		SetMaxSP(6);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 60 && GetMaxSP() < 5)
	{
		SetMaxSP(5);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 70 && GetMaxSP() < 4)
	{
		SetMaxSP(4);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 80 && GetMaxSP() < 3)
	{
		SetMaxSP(3);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 90 && GetMaxSP() < 2)
	{
		SetMaxSP(2);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 500, GetY() - 500, GetX() + 500, GetY() + 500);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else if (iPercent <= 99 && GetMaxSP() < 1)
	{
		SetMaxSP(1);
		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0);

		CHARACTER_MANAGER::instance().SelectStone(this);
		CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, GetMapIndex(), GetX() - 1000, GetY() - 1000, GetX() + 1000, GetY() + 1000);
		CHARACTER_MANAGER::instance().SelectStone(NULL);
	}
	else
		return;

	UpdatePacket();
	return;
}

void CHARACTER::__StateIdle_NPC()
{
	MonsterChat(MONSTER_CHAT_WAIT);
	m_dwStateDuration = PASSES_PER_SEC(5);

	// Æê ½Ã½ºÅÛÀÇ Idle Ã³¸®´Â ±âÁ¸ °ÅÀÇ ¸ðµç Á¾·ùÀÇ Ä³¸¯ÅÍµéÀÌ °øÀ¯ÇØ¼­ »ç¿ëÇÏ´Â »óÅÂ¸Ó½ÅÀÌ ¾Æ´Ñ CPetActor::Update¿¡¼­ Ã³¸®ÇÔ.
	if (IsGuardNPC())
	{
		// GUARDIAN_NO_WALK
		return;
		// GUARDIAN_NO_WALK
		if (!quest::CQuestManager::instance().GetEventFlag("noguard"))
		{
			FuncFindGuardVictim f(this, 50000);

			if (GetSectree())
				GetSectree()->ForEachAround(f);

			LPCHARACTER victim = f.GetVictim();

			if (victim)
			{
				m_dwStateDuration = passes_per_sec/2;

				if (CanBeginFight())
					BeginFight(victim);
			}
		}
	}
	else
	{
		if (!IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		{
			LPCHARACTER pkChrProtege = GetProtege();

			if (pkChrProtege)
			{
				if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 500)
				{
					if (Follow(pkChrProtege, random_number(100, 300)))
						return;
				}
			}

			if (!random_number(0, 6))
			{
				SetRotation(random_number(0, 359));		// ¹æÇâÀº ·£´ýÀ¸·Î ¼³Á¤

				float fx, fy;
				float fDist = random_number(200, 400);

				GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

				// ´À½¼ÇÑ ¸ø°¨ ¼Ó¼º Ã¼Å©; ÃÖÁ¾ À§Ä¡¿Í Áß°£ À§Ä¡°¡ °¥¼ö¾ø´Ù¸é °¡Áö ¾Ê´Â´Ù.
				if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx, GetY() + (int) fy) 
					&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx / 2, GetY() + (int) fy / 2)))
					return;

				SetNowWalking(true);

				if (Goto(GetX() + (int) fx, GetY() + (int) fy))
					SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

				return;
			}
		}
		else if (IS_SET(GetAIFlag(), AIFLAG_AGGRESSIVE))
		{
			FuncFindPlayerVictim f(m_posExit.x, m_posExit.y, GetMobTable().aggressive_sight());

			if (GetSectree())
				GetSectree()->ForEachAround(f);

			LPCHARACTER victim = f.GetVictim();

			if (victim)
			{
				m_dwStateDuration = passes_per_sec / 2;

				if (CanBeginFight())
					BeginFight(victim);
			}
		}
	}
}

void CHARACTER::__StateIdle_Monster()
{
	if (IsStun())
		return;

	if (!CanMove())
		return;

	if (IsCoward())
	{
		// °ÌÀïÀÌ ¸ó½ºÅÍ´Â µµ¸Á¸¸ ´Ù´Õ´Ï´Ù.
		if (!IsDead())
			CowardEscape();

		return;
	}

	if (IsBerserker())
		if (IsBerserk())
			SetBerserk(false);

	if (IsGodSpeeder())
		if (IsGodSpeed())
			SetGodSpeed(false);

#ifdef __FAKE_PC__
	if (FakePC_Check() && FakePC_UseSkill())
		return;
#endif

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check() && FakeBuff_UseSkill())
		return;
#endif

	LPCHARACTER victim = GetVictim();

	if (!victim || victim->IsDead())
	{
		SetVictim(NULL);
		victim = NULL;
		m_dwStateDuration = PASSES_PER_SEC(1);
	}

	if (!victim || victim->IsBuilding())
	{
		// µ¹ º¸È£ Ã³¸®
		if (m_pkChrStone)
		{
			victim = m_pkChrStone->GetNearestVictim(m_pkChrStone);
		}
		// ¼±°ø ¸ó½ºÅÍ Ã³¸®
		else if (!no_wander && IsAggressive())
		{
			if (GetMapIndex() == 61 && quest::CQuestManager::instance().GetEventFlag("xmas_tree"));
			// 서한산에서 나무가 있으면 선공하지않는다.
			else
			{
				victim = FindMobVictim(this, m_pkMobData->m_table.aggressive_sight());
#ifdef __MELEY_LAIR_DUNGEON__
				if ((!victim) && (GetRaceNum() == (WORD)(MeleyLair::BOSS_VNUM)))
					victim = FindMobVictim(this, 40000);
#endif
			}
		}
#ifdef __FAKE_PC__
		else if (FakePC_IsSupporter() && FakePC_CanAttack())
		{
			victim = FindFakePCVictim(this);
		}
#endif

		if (test_server && IsAggressive())
			sys_log(0, "FindVictim -> %p (%s)", victim, victim ? victim->GetName() : "<none>");
	}

	if (victim && !victim->IsDead())
	{
		if (CanBeginFight())
			BeginFight(victim);
		if (test_server && IsAggressive())
			sys_log(0, "BeginFight");

		return;
	}

#ifdef __FAKE_PC__
	if (FakePC_IsSupporter())
		m_dwStateDuration = PASSES_PER_SEC(1);
	else
#endif
	if (IsAggressive() && !victim)
		m_dwStateDuration = PASSES_PER_SEC(random_number(1, 3));
	else
		m_dwStateDuration = PASSES_PER_SEC(random_number(3, 5));

	LPCHARACTER pkChrProtege = GetProtege();

	// º¸È£ÇÒ °Í(µ¹, ÆÄÆ¼Àå)¿¡°Ô·Î ºÎÅÍ ¸Ö´Ù¸é µû¶ó°£´Ù.
	if (pkChrProtege)
	{
#ifdef __FAKE_BUFF__
		if (FakeBuff_Check() && DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
		{
			int x = pkChrProtege->GetX() + random_number(50, 150) * (random_number(0, 1) ? -1 : 1);
			int y = pkChrProtege->GetY() + random_number(50, 150) * (random_number(0, 1) ? -1 : 1);
			Show(GetMapIndex(), x, y);
			return;
		}
#endif

		if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
		{
#ifdef __FAKE_PC__
			if (Follow(pkChrProtege, FakePC_IsSupporter() ? random_number(150, 200) : random_number(150, 300)))
#else
			if (Follow(pkChrProtege, random_number(150, 300)))
#endif
			{
				MonsterLog("[IDLE] ¸®´õ·ÎºÎÅÍ ³Ê¹« ¸Ö¸® ¶³¾îÁ³´Ù! º¹±ÍÇÑ´Ù.");
				return;
			}
		}
	}

	//
	// ±×³É ¿Ô´Ù¸® °¬´Ù¸® ÇÑ´Ù.
	//
	if (!no_wander && !IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (!random_number(0, 6))
		{
			SetRotation(random_number(0, 359));		// ¹æÇâÀº ·£´ýÀ¸·Î ¼³Á¤

			float fx, fy;
			float fDist = random_number(300, 700);

			GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

			// ´À½¼ÇÑ ¸ø°¨ ¼Ó¼º Ã¼Å©; ÃÖÁ¾ À§Ä¡¿Í Áß°£ À§Ä¡°¡ °¥¼ö¾ø´Ù¸é °¡Áö ¾Ê´Â´Ù.
			if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx, GetY() + (int) fy) 
						&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx/2, GetY() + (int) fy/2)))
				return;

			// NOTE: ¸ó½ºÅÍ°¡ IDLE »óÅÂ¿¡¼­ ÁÖº¯À» ¼­¼º°Å¸± ¶§, ÇöÀç ¹«Á¶°Ç ¶Ù¾î°¡°Ô µÇ¾î ÀÖÀ½. (Àý´ë·Î °ÈÁö ¾ÊÀ½)
			// ±×·¡ÇÈ ÆÀ¿¡¼­ ¸ó½ºÅÍ°¡ °È´Â ¸ð½Àµµ º¸°í½Í´Ù°í ÇØ¼­ ÀÓ½Ã·Î Æ¯Á¤È®·ü·Î °È°Å³ª ¶Ù°Ô ÇÔ. (°ÔÀÓÀÇ Àü¹ÝÀûÀÎ ´À³¦ÀÌ Æ²·ÁÁö±â ¶§¹®¿¡ ÀÏ´Ü Å×½ºÆ® ¸ðµå¿¡¼­¸¸ ÀÛµ¿)
			if (g_test_server)
			{
				if (random_number(0, 100) < 60)
					SetNowWalking(false);
				else
					SetNowWalking(true);
			}

			if (Goto(GetX() + (int) fx, GetY() + (int) fy))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			return;
		}
	}

	MonsterChat(MONSTER_CHAT_WAIT);
}
// END_OF_STATE_IDLE_REFACTORING

bool __CHARACTER_GotoNearTarget(LPCHARACTER self, LPCHARACTER victim)
{
	if (!self->CanNPCFollowTarget(victim))
		return false;

	switch (self->GetMobBattleType())
	{
		case BATTLE_TYPE_RANGE:
		case BATTLE_TYPE_MAGIC:
			// ¸¶¹ý»ç³ª ±Ã¼ö´Â °ø°Ý °Å¸®ÀÇ 80%±îÁö °¡¼­ °ø°ÝÀ» ½ÃÀÛÇÑ´Ù.
			if (self->Follow(victim, self->GetMobAttackRange() * 8 / 10))
				return true;
			break;

		default:
			// ³ª¸ÓÁö´Â 90%?
			if (self->Follow(victim, self->GetMobAttackRange() * 9 / 10))
				return true;
	}

	return false;
}

void CHARACTER::StateMove()
{
	DWORD dwElapsedTime = get_dword_time() - m_dwMoveStartTime;
	float fRate = (float) dwElapsedTime / (float) m_dwMoveDuration;

	if (fRate > 1.0f)
		fRate = 1.0f;

	int x = (int) ((float) (m_posDest.x - m_posStart.x) * fRate + m_posStart.x);
	int y = (int) ((float) (m_posDest.y - m_posStart.y) * fRate + m_posStart.y);

	Move(x, y);

	if (IsPC() && (thecore_pulse() & 15) == 0)
	{
		UpdateSectree();

		if (GetExchange())
		{
			LPCHARACTER victim = GetExchange()->GetCompany()->GetOwner();
			int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

			// °Å¸® Ã¼Å©
			if (iDist >= EXCHANGE_MAX_DISTANCE)
			{
				GetExchange()->Cancel();
			}
		}
	}

	// ½ºÅ×¹Ì³ª°¡ 0 ÀÌ»óÀÌ¾î¾ß ÇÑ´Ù.
	if (IsPC())
	{
		if (IsWalking() && GetStamina() < GetMaxStamina())
		{
			// 5ÃÊ ÈÄ ºÎÅÍ ½ºÅ×¹Ì³Ê Áõ°¡
			if (get_dword_time() - GetWalkStartTime() > 5000)
				PointChange(POINT_STAMINA, GetMaxStamina() / 1);
		}

		// ÀüÅõ ÁßÀÌ¸é¼­ ¶Ù´Â ÁßÀÌ¸é
		if (!IsWalking() && !IsRiding())
			if ((get_dword_time() - GetLastAttackTime()) < 20000)
			{
				StartAffectEvent();

				if (IsStaminaHalfConsume())
				{
					if (thecore_pulse()&1)
						PointChange(POINT_STAMINA, -STAMINA_PER_STEP);
				}
				else
					PointChange(POINT_STAMINA, -STAMINA_PER_STEP);

				StartStaminaConsume();

				if (GetStamina() <= 0)
				{
					// ½ºÅ×¹Ì³ª°¡ ¸ðÀÚ¶ó °É¾î¾ßÇÔ
					SetStamina(0);
					SetNowWalking(false);
					StopStaminaConsume();
				}
			}
			else if (IsStaminaConsume())
			{
				StopStaminaConsume();
			}
	}
	else
	{
		// XXX AGGRO 
		if (IsMonster() && GetVictim())
		{
			LPCHARACTER victim = GetVictim();
			UpdateAggrPoint(victim, DAMAGE_TYPE_NORMAL, -(victim->GetLevel() / 3 + 1));

			if (g_test_server)
			{
				// ¸ó½ºÅÍ°¡ ÀûÀ» ÂÑ¾Æ°¡´Â °ÍÀÌ¸é ¹«Á¶°Ç ¶Ù¾î°£´Ù.
				SetNowWalking(false);
			}
		}

		if (IsMonster() && GetMobRank() >= MOB_RANK_BOSS && GetVictim())
		{
			LPCHARACTER victim = GetVictim();

			// °Å´ë °ÅºÏ
			if (GetRaceNum() == 2191 && random_number(1, 20) == 1 && get_dword_time() - m_pkMobInst->m_dwLastWarpTime > 1000)
			{
				// ¿öÇÁ Å×½ºÆ®
				float fx, fy;
				GetDeltaByDegree(victim->GetRotation(), 400, &fx, &fy);
				long new_x = victim->GetX() + (long)fx;
				long new_y = victim->GetY() + (long)fy;
				SetRotation(GetDegreeFromPositionXY(new_x, new_y, victim->GetX(), victim->GetY()));
				Show(victim->GetMapIndex(), new_x, new_y, 0, true);
				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
				ResetMobSkillCooltime();
				m_pkMobInst->m_dwLastWarpTime = get_dword_time();
				return;
			}

			// TODO ¹æÇâÀüÈ¯À» ÇØ¼­ ´ú ¹Ùº¸°¡ µÇÀÚ!
			if (random_number(0, 3) == 0)
			{
				if (__CHARACTER_GotoNearTarget(this, victim))
					return;
			}
		}
	}

	if (fRate >= 0.93f && DoMovingWay())
		return;

	if (1.0f == fRate)
	{
		if (IsPC())
		{
			sys_log(1, "µµÂø %s %d %d", GetName(), x, y);
			GotoState(m_stateIdle);
			StopStaminaConsume();
		}
		else
		{
			if (GetVictim() && !IsCoward())
			{
				if (!IsState(m_stateBattle))
					MonsterLog("[BATTLE] ±ÙÃ³¿¡ ¿ÔÀ¸´Ï °ø°Ý½ÃÀÛ %s", GetVictim()->GetName());

				GotoState(m_stateBattle);
				m_dwStateDuration = 1;
			}
			else
			{
				if (!IsState(m_stateIdle))
					MonsterLog("[IDLE] ´ë»óÀÌ ¾øÀ¸´Ï ½¬ÀÚ");

				GotoState(m_stateIdle);

				LPCHARACTER rider = GetRider();

				m_dwStateDuration = PASSES_PER_SEC(random_number(1, 3));
			}
		}
	}
}

void CHARACTER::StateBattle()
{
	if (IsStone())
	{
		sys_err("Stone must not use battle state (name %s)", GetName());
		return;
	}

	if (IsPC())
		return; 

	if (!CanMove())
		return;

	if (IsStun())
		return;

	LPCHARACTER victim = GetVictim();

	if (IsCoward())
	{
		if (IsDead())
			return;

		SetVictim(NULL);

		if (random_number(1, 50) != 1)
		{
			GotoState(m_stateIdle);
			m_dwStateDuration = 1;
		}
		else
			CowardEscape();

		return;
	}

	if (!victim || (victim->IsStun() && IsGuardNPC()) || victim->IsDead())
	{
#ifdef __FAKE_PC__
		if (victim && victim->IsDead() &&
			!no_wander && IsAggressive() && (!GetParty() || GetParty()->GetLeader() == this)
			&& !FakePC_IsSupporter())
#else
		if (victim && victim->IsDead() &&
				!no_wander && IsAggressive() && (!GetParty() || GetParty()->GetLeader() == this))
#endif
		{
			LPCHARACTER new_victim = FindMobVictim(this, m_pkMobData->m_table.aggressive_sight());
#ifdef __MELEY_LAIR_DUNGEON__
			if ((!new_victim) && (GetRaceNum() == (WORD)(MeleyLair::BOSS_VNUM)))
				new_victim = FindMobVictim(this, 40000);
#endif
			SetVictim(new_victim);
			m_dwStateDuration = PASSES_PER_SEC(1);

			if (!new_victim)
			{
				if (IsMonster())
				{
					switch (GetMobBattleType())
					{
						case BATTLE_TYPE_MELEE:
						case BATTLE_TYPE_SUPER_POWER:
						case BATTLE_TYPE_SUPER_TANKER:
						case BATTLE_TYPE_POWER:
						case BATTLE_TYPE_TANKER:
							{
								float fx, fy;
								float fDist = random_number(400, 1500);

								GetDeltaByDegree(random_number(0, 359), fDist, &fx, &fy);

								if (SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(),
											victim->GetX() + (int) fx, 
											victim->GetY() + (int) fy) && 
										SECTREE_MANAGER::instance().IsMovablePosition(victim->GetMapIndex(),
											victim->GetX() + (int) fx/2,
											victim->GetY() + (int) fy/2))
								{
									float dx = victim->GetX() + fx;
									float dy = victim->GetY() + fy;

									SetRotation(GetDegreeFromPosition(dx, dy));

									if (Goto((long) dx, (long) dy))
									{
										sys_log(0, "KILL_AND_GO: %s distance %.1f", GetName(), fDist);
										SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
									}
								}
							}
					}
				}
			}
			return;
		}
#ifdef __FAKE_PC__
		else if (FakePC_IsSupporter())
		{
			LPCHARACTER new_victim = FindFakePCVictim(this);

			SetVictim(new_victim);
			m_dwStateDuration = PASSES_PER_SEC(1);

			return;
		}
#endif

		SetVictim(NULL);

		if (IsGuardNPC())
			Return();

		if (IS_SET(GetAIFlag(), AIFLAG_NOMOVE) && IS_SET(GetAIFlag(), AIFLAG_AGGRESSIVE))
		{
			if (GetX() != m_posExit.x || GetY() != m_posExit.y)
			{
				SetRotationToXY(m_posExit.x, m_posExit.y);
				if (Goto(m_posExit.x, m_posExit.y))
					SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
			}
		}

		m_dwStateDuration = PASSES_PER_SEC(1);
		return;
	}

	if (IsSummonMonster() && !IsDead() && !IsStun())
	{
		if (!GetParty())
		{
			// ¼­¸óÇØ¼­ Ã¤¿öµÑ ÆÄÆ¼¸¦ ¸¸µé¾î µÓ´Ï´Ù.
			CPartyManager::instance().CreateParty(this);
		}

		LPPARTY pParty = GetParty();
		bool bPct = !random_number(0, 3);

		if (bPct && pParty->CountMemberByVnum(GetSummonVnum()) < SUMMON_MONSTER_COUNT)
		{
			MonsterLog("ºÎÇÏ ¸ó½ºÅÍ ¼ÒÈ¯!");
			// ¸ðÀÚ¶ó´Â ³à¼®À» ºÒ·¯³» Ã¤¿ó½Ã´Ù.
			int sx = GetX() - 300;
			int sy = GetY() - 300;
			int ex = GetX() + 300;
			int ey = GetY() + 300;

			LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(GetSummonVnum(), GetMapIndex(), sx, sy, ex, ey, true, true);

			if (tch)
			{
				pParty->Join(tch->GetVID());
				pParty->Link(tch);
				if (!tch->GetParty())
					sys_err("failed to link mob party mapIndex: %d | leaderVnum: %d | current mob vnum: %d", GetMapIndex(), GetRaceNum(), tch->GetRaceNum());
			}
		}
	}

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (IS_SET(GetAIFlag(), AIFLAG_NOMOVE) && IS_SET(GetAIFlag(), AIFLAG_AGGRESSIVE))
	{
		float fDistFromBase = DISTANCE_APPROX(GetX() - m_posExit.x, GetY() - m_posExit.y);

		if (fDistFromBase > GetMobTable().aggressive_sight() || fDist >= 4000.0f)
		{
			SetVictim(NULL);

			SetRotation(GetDegreeFromPositionXY(GetX(), GetY(), m_posExit.x, m_posExit.y));
			if (Goto(m_posExit.x, m_posExit.y))
				SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

			return;
		}
	}

	if (fDist >= 4000.0f)
	{
#ifdef __MELEY_LAIR_DUNGEON__
		bool bPass = true;
		if (GetRaceNum() == (WORD)(MeleyLair::BOSS_VNUM) && (fDist < 16000.0f))
			bPass = false;

		if (bPass)
		{
			MonsterLog("타겟이 멀어서 포기");
			SetVictim(NULL);
			LPCHARACTER pkChrProtege = GetProtege();
			if (pkChrProtege)
				if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
					Follow(pkChrProtege, random_number(150, 400));

			return;
		}
#else
		MonsterLog("타겟이 멀어서 포기");
		SetVictim(NULL);

		LPCHARACTER pkChrProtege = GetProtege();

		if (pkChrProtege)
			if (DISTANCE_APPROX(GetX() - pkChrProtege->GetX(), GetY() - pkChrProtege->GetY()) > 1000)
				Follow(pkChrProtege, random_number(150, 400));

		return;
#endif
	}

#ifdef __FAKE_PC__
	if (FakePC_IsSupporter())
	{
		float fDistToMain = DISTANCE_APPROX(GetProtege()->GetX() - victim->GetX(), GetProtege()->GetY() - victim->GetY());

		if (fDistToMain > 2500)
		{
			SetVictim(NULL);

			StateBattle();
			return;
		}
	}
#endif

	DWORD dwCurTime = get_dword_time();
	DWORD dwDuration;

#ifdef __FAKE_PC__
	if (FakePC_Check())
	{
		DWORD dwLastActionTime = m_dwLastAttackTime;

		// normal attack
		DWORD dwMotionMode = GetMotionModeBySubType(GetWear(WEAR_WEAPON) ? GetWear(WEAR_WEAPON)->GetSubType() : UCHAR_MAX);
		float fAttackMotionDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(),
			MAKE_MOTION_KEY(dwMotionMode, MOTION_COMBO_ATTACK_1 + FakePC_ComputeComboIndex()));
		dwDuration = CalculateDuration(GET_ATTACK_SPEED(FakePC_GetOwner()), fAttackMotionDuration * 1000.0f) * (IsRiding() ? 6.3f/*(float)(quest::CQuestManager::instance().GetEventFlag("FakePCSDELAY")/10)*/ : 6.5f) / 10.0f;

		// skill
		if (m_dwLastSkillVnum != 0 && m_dwLastSkillTime > m_dwLastAttackTime)
		{
			CSkillProto* pkSkill = CSkillManager::Instance().Get(m_dwLastSkillVnum);
			if (pkSkill)
			{
				DWORD dwSkillIndex = 0;
				const DWORD* dwSkillList = GetUsableSkillList();
				if (dwSkillList)
				{
					for (int i = 0; i < CHARACTER_SKILL_COUNT; ++i)
					{
						if (dwSkillList[i] == m_dwLastSkillVnum)
						{
							dwSkillIndex = i;
							break;
						}
					}
				}

				DWORD dwDurationMotionKey = MAKE_MOTION_KEY(MOTION_MODE_GENERAL,
					MOTION_SPECIAL_1 + (GetSkillGroup() - 1) * CHARACTER_SKILL_COUNT + dwSkillIndex);

				dwLastActionTime = m_dwLastSkillTime;
				dwDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), dwDurationMotionKey) * 1000.0f * 9.0f / 10.0f;
			}
		}

		if ((dwCurTime - dwLastActionTime) < dwDuration) // 2ÃÊ ¸¶´Ù °ø°ÝÇØ¾ß ÇÑ´Ù.
		{
			m_dwStateDuration = MAX(1, PASSES_PER_SEC(dwDuration - (dwCurTime - dwLastActionTime)) / 1000);
			return;
		}
	}
	else
#endif
	{
		dwDuration = CalculateDuration(GetLimitPoint(POINT_ATT_SPEED), 2000);

		if ((dwCurTime - m_dwLastAttackTime) < dwDuration) // 2ÃÊ ¸¶´Ù °ø°ÝÇØ¾ß ÇÑ´Ù.
		{
			m_dwStateDuration = MAX(1, PASSES_PER_SEC(dwDuration - (dwCurTime - m_dwLastAttackTime)) / 1000);
			return;
		}
	}

	// FAKE PC SKILL
#ifdef __FAKE_PC__
	if (FakePC_Check() && FakePC_UseSkill(victim))
		return;
#endif
	// FAKE PC SKILL [END]

	if (fDist >= GetMobAttackRange() * 1.15)
	{
		__CHARACTER_GotoNearTarget(this, victim);
		return;
	}

	if (m_pkParty)
		m_pkParty->SendMessage(this, PM_ATTACKED_BY, 0, 0);

	if (2493 == m_pkMobData->m_table.vnum())
	{
		// ¼ö·æ(2493) Æ¯¼ö Ã³¸®
		m_dwStateDuration = BlueDragon_StateBattle(this);
		return;
	}

	if (IsBerserker() == true)
		if (GetHPPct() < m_pkMobData->m_table.berserk_point())
			if (IsBerserk() != true)
				SetBerserk(true);

	if (IsGodSpeeder() == true)
		if (GetHPPct() < m_pkMobData->m_table.god_speed_point())
			if (IsGodSpeed() != true)
				SetGodSpeed(true);

	//
	// ¸÷ ½ºÅ³ Ã³¸®
	//
	if (HasMobSkill())
	{
		for (unsigned int iSkillIdx = 0; iSkillIdx < MOB_SKILL_MAX_NUM; ++iSkillIdx)
		{
			if (CanUseMobSkill(iSkillIdx))
			{
				SetRotationToXY(victim->GetX(), victim->GetY());

#ifdef __MELEY_LAIR_DUNGEON__
				if ((GetRaceNum() == (WORD)(MeleyLair::BOSS_VNUM)) && (MeleyLair::CMgr::instance().IsMeleyMap(victim->GetMapIndex())))
				{
					PIXEL_POSITION pos = MeleyLair::CMgr::instance().GetXYZ();
					if (pos.x)
						SetRotationToXY(pos.x, pos.y);
				}
#endif
				if (UseMobSkill(iSkillIdx))
				{
					SendMovePacket(FUNC_MOB_SKILL, iSkillIdx, GetX(), GetY(), 0, dwCurTime);

					float fDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_SPECIAL_1 + iSkillIdx));
					m_dwStateDuration = (DWORD) (fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));

					if (test_server)
						sys_log(0, "USE_MOB_SKILL: %s idx %u motion %u duration %.0f", GetName(), iSkillIdx, MOTION_SPECIAL_1 + iSkillIdx, fDuration);

					return;
				}
			}
		}
	}

	if (!Attack(victim))	// °ø°Ý ½ÇÆÐ¶ó¸é? ¿Ö ½ÇÆÐÇßÁö? TODO
		m_dwStateDuration = passes_per_sec / 2;
	else
	{
		// ÀûÀ» ¹Ù¶óº¸°Ô ¸¸µç´Ù.
		SetRotationToXY(victim->GetX(), victim->GetY());

		SendMovePacket(FUNC_ATTACK, 0, GetX(), GetY(), 0, dwCurTime);

		float fDuration;
#ifdef __FAKE_PC__
		if (FakePC_Check())
		{
			fDuration = (float)dwDuration / 1000.0f;
		}
		else
#endif
		{
			DWORD dwMotionKey = MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK);
			fDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), dwMotionKey);
		}

		m_dwStateDuration = (DWORD) (fDuration == 0.0f ? PASSES_PER_SEC(2) : PASSES_PER_SEC(fDuration));
	}
}

void CHARACTER::StateFlag()
{
	m_dwStateDuration = (DWORD) PASSES_PER_SEC(0.5);

	CWarMap * pMap = GetWarMap();

	if (!pMap)
		return;

	FuncFindChrForFlag f(this);
	GetSectree()->ForEachAround(f);

	if (!f.m_pkChrFind)
		return;

	if (NULL == f.m_pkChrFind->GetGuild())
		return;

	char buf[256];
	BYTE idx;

	if (!pMap->GetTeamIndex(GetPoint(POINT_STAT), idx))
		return;

	f.m_pkChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_NONE, GetPoint(POINT_STAT), idx == 0 ? AFF_WAR_FLAG1 : AFF_WAR_FLAG2, INFINITE_AFFECT_DURATION, 0, false);
	f.m_pkChrFind->AddAffect(AFFECT_WAR_FLAG, POINT_MOV_SPEED, 50 - f.m_pkChrFind->GetPoint(POINT_MOV_SPEED), 0, INFINITE_AFFECT_DURATION, 0, false);

	pMap->RemoveFlag(idx);

	snprintf(buf, sizeof(buf), "[%s] ±æµåÀÇ ±ê¹ßÀ» [%s] ´ÔÀÌ È¹µæÇÏ¿´½À´Ï´Ù.", pMap->GetGuild(idx)->GetName(), f.m_pkChrFind->GetName());
	pMap->Notice(buf);
}

void CHARACTER::StateFlagBase()
{
	m_dwStateDuration = (DWORD) PASSES_PER_SEC(0.5);

	FuncFindChrForFlagBase f(this);
	GetSectree()->ForEachAround(f);
}

void CHARACTER::StateHorse()
{
	float	START_FOLLOW_DISTANCE = 400.0f;		// ÀÌ °Å¸® ÀÌ»ó ¶³¾îÁö¸é ÂÑ¾Æ°¡±â ½ÃÀÛÇÔ
	float	START_RUN_DISTANCE = 700.0f;		// ÀÌ °Å¸® ÀÌ»ó ¶³¾îÁö¸é ¶Ù¾î¼­ ÂÑ¾Æ°¨.
	int		MIN_APPROACH = 150;					// ÃÖ¼Ò Á¢±Ù °Å¸®
	int		MAX_APPROACH = 300;					// ÃÖ´ë Á¢±Ù °Å¸®	

	DWORD	STATE_DURATION = (DWORD)PASSES_PER_SEC(0.5);	// »óÅÂ Áö¼Ó ½Ã°£

	bool bDoMoveAlone = true;					// Ä³¸¯ÅÍ¿Í °¡±îÀÌ ÀÖÀ» ¶§ È¥ÀÚ ¿©±âÀú±â ¿òÁ÷ÀÏ°ÇÁö ¿©ºÎ -_-;
	bool bRun = true;							// ¶Ù¾î¾ß ÇÏ³ª?

	if (IsDead())
		return;

	m_dwStateDuration = STATE_DURATION;

	LPCHARACTER victim = GetRider();

	// ! ¾Æ´Ô // ´ë»óÀÌ ¾ø´Â °æ¿ì ¼ÒÈ¯ÀÚ°¡ Á÷Á¢ ³ª¸¦ Å¬¸®¾îÇÒ °ÍÀÓ
	if (!victim)
	{
		M2_DESTROY_CHARACTER(this);
		return;
	}

	m_pkMobInst->m_posLastAttacked = GetXYZ();

	float fDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if (fDist > START_RUN_DISTANCE)
			SetNowWalking(!bRun);		// NOTE: ÇÔ¼ö ÀÌ¸§º¸°í ¸ØÃß´Â°ÇÁÙ ¾Ë¾Ò´Âµ¥ SetNowWalking(false) ÇÏ¸é ¶Ù´Â°ÅÀÓ.. -_-;

		Follow(victim, random_number(MIN_APPROACH, MAX_APPROACH));

		m_dwStateDuration = STATE_DURATION;
	}
	else if (bDoMoveAlone && (get_dword_time() > m_dwLastAttackTime))
	{
		// wondering-.-
		m_dwLastAttackTime = get_dword_time() + random_number(5000, 12000);

		SetRotation(random_number(0, 359));		// ¹æÇâÀº ·£´ýÀ¸·Î ¼³Á¤

		float fx, fy;
		float fDist = random_number(200, 400);

		GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);

		// ´À½¼ÇÑ ¸ø°¨ ¼Ó¼º Ã¼Å©; ÃÖÁ¾ À§Ä¡¿Í Áß°£ À§Ä¡°¡ °¥¼ö¾ø´Ù¸é °¡Áö ¾Ê´Â´Ù.
		if (!(SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx, GetY() + (int) fy) 
					&& SECTREE_MANAGER::instance().IsMovablePosition(GetMapIndex(), GetX() + (int) fx/2, GetY() + (int) fy/2)))
			return;

		SetNowWalking(true);

		if (Goto(GetX() + (int) fx, GetY() + (int) fy))
			SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	}
}

