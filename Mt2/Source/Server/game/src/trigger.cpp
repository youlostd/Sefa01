#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "sectree_manager.h"
#include "battle.h"
#include "affect.h"
#include "shop_manager.h"
#include "item.h"

int	OnClickShop(TRIGGERPARAM);
int	OnClickTalk(TRIGGERPARAM);

int	OnIdleDefault(TRIGGERPARAM);
int	OnAttackDefault(TRIGGERPARAM);

typedef struct STriggerFunction
{
	int (*func) (TRIGGERPARAM);
} TTriggerFunction;

TTriggerFunction OnClickTriggers[ON_CLICK_MAX_NUM] =
{
	{ NULL,		  	},	// ON_CLICK_NONE,
	{ OnClickShop,	},	// ON_CLICK_SHOP,
};

void CHARACTER::AssignTriggers(const network::TMobTable * table)
{
	if (table->on_click_type() >= ON_CLICK_MAX_NUM)
	{
		sys_err("%s has invalid OnClick value %d", GetName(), table->on_click_type());
		abort();
	}

	m_triggerOnClick.bType = table->on_click_type();
	m_triggerOnClick.pFunc = OnClickTriggers[table->on_click_type()].func;
}

/*
 * ON_CLICK
 */
int OnClickShop(TRIGGERPARAM)
{
	CShopManager::instance().StartShopping(causer, ch);
	return 1;
}

/*
 * 몬스터 AI 함수들을 BattleAI 클래스로 수정
 */
int OnIdleDefault(TRIGGERPARAM)
{
	if (ch->OnIdle())
		return PASSES_PER_SEC(1);

	return PASSES_PER_SEC(1);
}

class FuncFindMobVictim
{
	public:
		FuncFindMobVictim(LPCHARACTER pkChr, int iMaxDistance) :
			m_pkChr(pkChr),
			m_iMinDistance(~(1L << 31)),
			m_iMaxDistance(iMaxDistance),
			m_lx(pkChr->GetX()),
			m_ly(pkChr->GetY()),
			m_pkChrVictim(NULL),
			m_pkChrBuilding(NULL)
	{
			if (IS_SET(m_pkChr->GetAIFlag(), AIFLAG_NOMOVE) && IS_SET(m_pkChr->GetAIFlag(), AIFLAG_AGGRESSIVE))
			{
				long lMapIndex;
				m_pkChr->GetExitLocation(lMapIndex, m_lx, m_ly);
			}
	};

		bool operator () (LPENTITY ent)
		{
			if (!ent->IsType(ENTITY_CHARACTER))
				return false;

			LPCHARACTER pkChr = (LPCHARACTER) ent;

			if (pkChr->IsBuilding() && 
				(pkChr->IsAffectFlag(AFF_BUILDING_CONSTRUCTION_SMALL) ||
				 pkChr->IsAffectFlag(AFF_BUILDING_CONSTRUCTION_LARGE) ||
				 pkChr->IsAffectFlag(AFF_BUILDING_UPGRADE)))
			{
				m_pkChrBuilding = pkChr;
			}

			if (pkChr->IsNPC())
			{
				if ( !pkChr->IsMonster() || !m_pkChr->IsAttackMob() || m_pkChr->IsAggressive()  )
					return false;

#ifdef __FAKE_PC__
				if (pkChr->FakePC_Check())
				{
					if (!m_pkChr->FakePC_Check())
						return false;

					if (m_pkChr->GetPVPTeam() == pkChr->GetPVPTeam())
						return false;
				}
#endif
					
			}

#ifdef __FAKE_PC__
			if ((pkChr->FakePC_IsSupporter() && !pkChr->FakePC_CanAttack()) || (m_pkChr->FakePC_IsSupporter() && !m_pkChr->FakePC_CanAttack()))
				return false;

			if (pkChr->IsPC() && m_pkChr->FakePC_Check() && m_pkChr->FakePC_GetOwner() == pkChr)
				return false;
#endif

			if (pkChr->IsDead())
				return false;

			if (pkChr->IsAffectFlag(AFF_EUNHYUNG) || 
					pkChr->IsAffectFlag(AFF_INVISIBILITY) ||
					pkChr->IsAffectFlag(AFF_REVIVE_INVISIBLE))
				return false;

			if ((pkChr->IsAffectFlag(AFF_TERROR) || pkChr->IsAffectFlag(AFF_TERROR_PERFECT)) && m_pkChr->IsImmune(IMMUNE_TERROR) == false )	// 공포 처리
			{
				if ( pkChr->GetLevel() >= m_pkChr->GetLevel() )
					return false;
			}

		 	if ( m_pkChr->IsNoAttackShinsu() )
			{
				if ( pkChr->GetEmpire() == 1 )
					return false;
			}

			if ( m_pkChr->IsNoAttackChunjo() )
			{
				if ( pkChr->GetEmpire() == 2 )
					return false;
			}
			

			if ( m_pkChr->IsNoAttackJinno() )
			{
				if ( pkChr->GetEmpire() == 3 )
					return false;
			}

			int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

			if (iDistance < m_iMinDistance && iDistance <= m_iMaxDistance)
			{
				m_pkChrVictim = pkChr;
				m_iMinDistance = iDistance;
			}
			return true;
		}

		LPCHARACTER GetVictim()
		{
			// 근처에 건물이 있고 피가 많이 있다면 건물을 공격한다. 건물만 있어도 건물을 공격
			if (m_pkChrBuilding && m_pkChr->GetHP() * 2 > m_pkChr->GetMaxHP() || !m_pkChrVictim)
			{
				return m_pkChrBuilding;
			}

			return (m_pkChrVictim);
		}

	private:
		LPCHARACTER	m_pkChr;

		int		m_iMinDistance;
		int		m_iMaxDistance;
		long		m_lx;
		long		m_ly;

		LPCHARACTER	m_pkChrVictim;
		LPCHARACTER	m_pkChrBuilding;
};

LPCHARACTER FindMobVictim(LPCHARACTER pkChr, int iMaxDistance)
{
	FuncFindMobVictim f(pkChr, iMaxDistance);
	if (pkChr->GetSectree() != NULL) {
		pkChr->GetSectree()->ForEachAround(f);	
	}
	return f.GetVictim();
}

#ifdef __FAKE_PC__
class FuncFindFakePCVictim
{
public:
	FuncFindFakePCVictim(LPCHARACTER pkChr, LPCHARACTER pkMainPC) :
		m_pkChr(pkChr),
		m_pkMainPC(pkMainPC),
		m_lx(pkChr->GetX()),
		m_ly(pkChr->GetY()),
		m_lmainx(pkMainPC->GetX()),
		m_lmainy(pkMainPC->GetY()),
		m_lDistance(-1),
		m_pkChrVictim(NULL)
	{
	}

	bool operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return false;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		if (!pkChr->IsMonster())
			return false;

		if (pkChr->IsDead())
			return false;

		if (pkChr->GetVictim() != m_pkMainPC && pkChr->GetVictim() != m_pkChr)
			return false;

		// focus range / magic types
		if (m_pkChrVictim)
		{
			if (m_pkChrVictim->GetVictim() == m_pkMainPC && pkChr->GetVictim() == m_pkChr)
				return false;

			if (m_pkChrVictim->GetVictim() == pkChr->GetVictim())
			{
				if ((m_pkChrVictim->GetMobBattleType() == BATTLE_TYPE_RANGE || m_pkChrVictim->GetMobBattleType() == BATTLE_TYPE_MAGIC) &&
					(pkChr->GetMobBattleType() != BATTLE_TYPE_RANGE && pkChr->GetMobBattleType() != BATTLE_TYPE_MAGIC))
					return false;
			}
		}

		int iDistanceToPlayer = DISTANCE_APPROX(m_lmainx - pkChr->GetX(), m_lmainy - pkChr->GetY());
		if (iDistanceToPlayer > 2500)
			return false;

		int iDistance = DISTANCE_APPROX(m_lx - pkChr->GetX(), m_ly - pkChr->GetY());

		if (m_lDistance < 0 || iDistance < m_lDistance)
		{
			m_lDistance = iDistance;
			m_pkChrVictim = pkChr;
		}

		return true;
	}

	LPCHARACTER GetVictim()
	{
		return (m_pkChrVictim);
	}

private:
	LPCHARACTER	m_pkChr;
	LPCHARACTER	m_pkMainPC;

	int		m_lx;
	int		m_ly;
	int		m_lmainx;
	int		m_lmainy;
	int		m_lDistance;

	LPCHARACTER	m_pkChrVictim;
};

LPCHARACTER FindFakePCVictim(LPCHARACTER pkChr)
{
	if (!pkChr->FakePC_Check())
	{
		sys_err("%u %s is no fake pc [char type %u]", pkChr->GetPlayerID(), pkChr->GetName(), pkChr->GetCharType());
		return NULL;
	}

	LPCHARACTER pkOwner = pkChr->FakePC_GetOwner();
	if (!pkOwner)
	{
		sys_err("cannot get spawner for fake pc %s", pkChr->GetName());
		return NULL;
	}

	FuncFindFakePCVictim f(pkChr, pkOwner);
	if (pkChr->GetSectree() != NULL) {
		pkChr->GetSectree()->ForEachAround(f);
	}
	return f.GetVictim();
}
#endif

class FuncFindItemVictim
{
public:
	FuncFindItemVictim(LPCHARACTER pkChr, int iMaxDistance, bool bNeedOwner = false) :
		m_pkChr(pkChr),
		m_iMinItemDistance(~(1L << 31)),
		m_iMinGoldDistance(~(1L << 31)),
		m_iMaxDistance(iMaxDistance),
		m_lx(pkChr->GetX()),
		m_ly(pkChr->GetY()),
		m_bNeedOwner(bNeedOwner),
		m_pkItemVictim(NULL),
		m_pkGoldVictim(NULL)
	{
	};

	bool operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_ITEM))
			return false;

		LPITEM pkItem = (LPITEM)ent;

		if (pkItem->GetWindow() != GROUND)
			return false;

		if (m_bNeedOwner && pkItem->GetOwner() == NULL)
			return false;

		if (pkItem->GetOwner()->GetPlayerID() != m_pkChr->GetPlayerID())
			return false;

		int iDistance = DISTANCE_APPROX(m_lx - pkItem->GetX(), m_ly - pkItem->GetY());

		int* iMinDistance;
		LPITEM* pkItemVictim;

		if (pkItem->GetType() == ITEM_ELK)
		{
			iMinDistance = &m_iMinGoldDistance;
			pkItemVictim = &m_pkGoldVictim;
		}
		else
		{
			iMinDistance = &m_iMinItemDistance;
			pkItemVictim = &m_pkItemVictim;
		}

		if (iDistance < *iMinDistance && iDistance <= m_iMaxDistance)
		{
			*iMinDistance = iDistance;
			*pkItemVictim = pkItem;
		}

		return true;
	}

	LPITEM GetVictim()
	{
		return (m_pkItemVictim ? m_pkItemVictim : m_pkGoldVictim);
	}

private:
	LPCHARACTER	m_pkChr;

	int		m_iMinItemDistance;
	int		m_iMinGoldDistance;
	int		m_iMaxDistance;
	long		m_lx;
	long		m_ly;

	bool	m_bNeedOwner;

	LPITEM	m_pkItemVictim;
	LPITEM	m_pkGoldVictim;
};

LPITEM FindItemVictim(LPCHARACTER pkChr, int iMaxDistance, bool bNeedOwner)
{
	FuncFindItemVictim f(pkChr, iMaxDistance, bNeedOwner);
	if (pkChr->GetSectree() != NULL) {
		pkChr->GetSectree()->ForEachAround(f);
	}
	return f.GetVictim();
}

LPITEM FindItemVictim(LPCHARACTER pkChr, int iMaxDistance)
{
	FuncFindItemVictim f(pkChr, iMaxDistance);
	if (pkChr->GetSectree() != NULL) {
		pkChr->GetSectree()->ForEachAround(f);
	}
	return f.GetVictim();
}

