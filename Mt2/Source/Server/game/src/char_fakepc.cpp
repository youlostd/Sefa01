#include "stdafx.h"

#ifdef __FAKE_PC__
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "skill.h"
#include "motion.h"
#include "sectree.h"
#include "party.h"
#include "mount_system.h"
#include "packet.h"

extern int passes_per_sec;

struct FFindFakePCSkillVictim
{
	FFindFakePCSkillVictim(LPCHARACTER pkChr, CSkillProto* pkSkill)
	{
		m_pkChr = pkChr;
		m_pkSkill = pkSkill;
		m_pkVictim = NULL;
		m_iVictimDistance = -1;
	}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;

		if (pkChr->IsDead())
			return;

		if (!m_pkChr->FakePC_IsSupporter())
		{
			if (!pkChr->IsMonster())
				return;

			if (!pkChr->FakePC_IsSkillNeeded(m_pkSkill))
				return;

			int iDist = DISTANCE_SQRT(m_pkChr->GetX() - pkChr->GetX(), m_pkChr->GetY() - pkChr->GetY());
			if (iDist > m_pkSkill->dwTargetRange)
				return;

			if ((pkChr->FakePC_Check() && m_pkVictim && !m_pkVictim->FakePC_Check()) ||
				(m_iVictimDistance == -1 || iDist > m_iVictimDistance))
			{
				m_iVictimDistance = iDist;
				m_pkVictim = pkChr;
			}
		}
		else
		{
			if (!pkChr->IsPC() && !pkChr->FakePC_Check())
				return;

			if (!pkChr->FakePC_IsSkillNeeded(m_pkSkill))
				return;

			if (!IS_SET(m_pkSkill->dwFlag, SKILL_FLAG_ATTACK))
			{
				if (pkChr->IsPC())
				{
					LPCHARACTER pkFakePCOwner = m_pkChr->FakePC_GetOwner();
					if (pkFakePCOwner != pkChr)
					{
						if (!pkFakePCOwner->GetParty() || !pkFakePCOwner->GetParty()->IsMember(pkChr->GetPlayerID()))
							return;
					}
				}
				else
				{
					if (!pkChr->FakePC_Check() || pkChr->FakePC_GetOwner() != m_pkChr->FakePC_GetOwner())
						return;

					if (m_pkVictim && (m_pkVictim->GetJob() != JOB_SHAMAN || m_pkVictim->GetSkillGroup() != m_pkChr->GetSkillGroup()) &&
						(m_pkVictim->IsPC() || m_pkVictim->GetLevel() > pkChr->GetLevel()))
						return;
				}
			}

			int iDist = DISTANCE_SQRT(m_pkChr->GetX() - pkChr->GetX(), m_pkChr->GetY() - pkChr->GetY());
			if (iDist > m_pkSkill->dwTargetRange)
				return;

			if (m_iVictimDistance == -1)
			{
				m_iVictimDistance = iDist;
				m_pkVictim = pkChr;
			}
		}
	}

	LPCHARACTER		m_pkChr;
	CSkillProto*	m_pkSkill;
	LPCHARACTER		m_pkVictim;
	int				m_iVictimDistance;
};

EVENTFUNC(fake_pc_afk_event)
{
	if (event == NULL) 
	{
		sys_err("%s <Event> Null pointer, %s:%d", __FUNCTION__, __FILE__, __LINE__);
		return 0;
	}

	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("fake_pc_afk_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->FakePC_Owner_ClearAfkEvent();
	if (ch->FakePC_Owner_DespawnAllSupporter())
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your fake pcs are despawned because of your inactivity."));

	return 0;
}

void CHARACTER::FakePC_Load(LPCHARACTER pkOwner, LPITEM pkSpawnItem)
{
	if (m_pkFakePCOwner)
		return;

	// init owner
	pkOwner->FakePC_Owner_AddSpawned(this, pkSpawnItem);
	pkOwner->FakePC_Owner_ResetAfkEvent();

	// init this
	m_pkFakePCOwner = pkOwner;
	m_pkFakePCSpawnItem = pkSpawnItem;
	
	// set item info
	if (m_pkFakePCSpawnItem)
	{
		//m_pkFakePCSpawnItem->SetItemActive(true);
		m_pkFakePCSpawnItem->StartTimerBasedOnWearExpireEvent();
	}

	// set general
	m_bPKMode = PK_MODE_PROTECT;
	SetLevel(pkOwner->GetLevel());
	SetEmpire(pkOwner->GetEmpire());

	// set name
	const char* szName = pkOwner->FakePC_Owner_GetName();
	if (*szName)
		SetName(szName);
	else
		SetName(((std::string)pkOwner->GetName()) + "'s Ditto");

	// set damage factor
	FakePC_ComputeDamageFactor();

	// copy parts
	for (int i = 0; i < PART_MAX_NUM; ++i)
		SetPart(i, m_pkFakePCOwner->GetPart(i));

	// copy affects
	const std::list<CAffect*>& list_pkAffect = pkOwner->GetAffectContainer();
	for (itertype(list_pkAffect) it = list_pkAffect.begin(); it != list_pkAffect.end(); ++it)
		FakePC_AddAffect(*it);

	// compute points
	ComputePoints();
	ComputeBattlePoints();

	// start affect + potion event
	StartAffectEvent();
}

void CHARACTER::FakePC_Destroy()
{
	if (!FakePC_Check())
		return;

	// deinit owner
	m_pkFakePCOwner->FakePC_Owner_RemoveSpawned(this);
	m_pkFakePCOwner->FakePC_Owner_ResetAfkEvent();

	// set item info
	if (m_pkFakePCSpawnItem)
	{
		//m_pkFakePCSpawnItem->SetItemActive(false);
		m_pkFakePCSpawnItem->StopTimerBasedOnWearExpireEvent();
	}

#ifdef __ARENA_DITTO_MODE__
	CDittoArenaManager::instance().OnDittoDestroy(this);
#endif
	event_cancel(&m_pkFakePCAfkEvent);
	// clear
	m_pkFakePCOwner = NULL;
	m_pkFakePCSpawnItem = NULL;
}

void CHARACTER::FakePC_Owner_ResetAfkEvent()
{
	event_cancel(&m_pkFakePCAfkEvent);

	if (m_set_pkFakePCSpawns.size() == 0)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	event_create(fake_pc_afk_event, info, PASSES_PER_SEC(60 * 3));
}

void CHARACTER::FakePC_Owner_ClearAfkEvent()
{
	m_pkFakePCAfkEvent = NULL;
}

bool CHARACTER::FakePC_CanAddAffect(CAffect* pkAff)
{
	if (pkAff->lDuration < 60 * 10)
		return false;

	std::set<DWORD> set_dwNotAllowedTypes;
#ifdef __DOMINION_MODE__
	set_dwNotAllowedTypes.insert(AFFECT_DOMINION_CAPTURED);
#endif

	if (set_dwNotAllowedTypes.find(pkAff->dwFlag) != set_dwNotAllowedTypes.end())
		return false;

	return true;
}

void CHARACTER::FakePC_AddAffect(CAffect* pkAff)
{
	if (!FakePC_CanAddAffect(pkAff))
		return;

	CAffect* pkNewAff;

	itertype(m_map_pkFakePCAffects) it = m_map_pkFakePCAffects.find(pkAff);
	if (it == m_map_pkFakePCAffects.end())
	{
		pkNewAff = new CAffect();

		m_list_pkAffect.push_back(pkNewAff);
		m_map_pkFakePCAffects.insert(std::pair<CAffect*, CAffect*>(pkAff, pkNewAff));
		m_map_pkFakePCAffects.insert(std::pair<CAffect*, CAffect*>(pkNewAff, pkAff));
	}
	else
	{
		pkNewAff = it->second;

		ComputeAffect(pkNewAff, false);
	}

	memcpy(pkNewAff, pkAff, sizeof(CAffect));

	ComputeAffect(pkNewAff, true);
}

void CHARACTER::FakePC_RemoveAffect(CAffect* pkAff)
{
	itertype(m_map_pkFakePCAffects) it = m_map_pkFakePCAffects.find(pkAff);
	if (it == m_map_pkFakePCAffects.end())
		return;

	RemoveAffect(it->second);
}

void CHARACTER::FakePC_Owner_AddSpawned(LPCHARACTER pkFakePC, LPITEM pkSpawnItem)
{
	m_set_pkFakePCSpawns.insert(pkFakePC);

	if (pkSpawnItem)
		m_map_pkFakePCSpawnItems.insert(std::pair<LPITEM, LPCHARACTER>(pkSpawnItem, pkFakePC));
}

bool CHARACTER::FakePC_Owner_RemoveSpawned(LPCHARACTER pkFakePC)
{
	if (pkFakePC->FakePC_GetOwnerItem())
		m_map_pkFakePCSpawnItems.erase(pkFakePC->FakePC_GetOwnerItem());

	m_set_pkFakePCSpawns.erase(pkFakePC);

	return true;
}

bool CHARACTER::FakePC_Owner_RemoveSpawned(LPITEM pkSpawnItem)
{
	itertype(m_map_pkFakePCSpawnItems) it = m_map_pkFakePCSpawnItems.find(pkSpawnItem);
	if (it == m_map_pkFakePCSpawnItems.end())
		return false;

	return FakePC_Owner_RemoveSpawned(it->second);
}

LPCHARACTER CHARACTER::FakePC_Owner_GetSpawnedByItem(LPITEM pkItem)
{
	if (m_set_pkFakePCSpawns.empty())
		return NULL;

	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
	{
		if ((*it)->FakePC_GetOwnerItem() == pkItem)
			return *it;
	}

	return NULL;
}

LPCHARACTER CHARACTER::FakePC_Owner_GetSupporter()
{
	if (m_set_pkFakePCSpawns.empty())
		return NULL;

	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
	{
		if ((*it)->FakePC_IsSupporter())
			return *it;
	}

	return NULL;
}

LPCHARACTER CHARACTER::FakePC_Owner_GetSupporterByItem(LPITEM pkItem)
{
	if (m_set_pkFakePCSpawns.empty())
		return NULL;

	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
	{
		if ((*it)->FakePC_IsSupporter() && (*it)->FakePC_GetOwnerItem() == pkItem)
			return *it;
	}

	return NULL;
}

DWORD CHARACTER::FakePC_Owner_CountSummonedByItem()
{
	return m_map_pkFakePCSpawnItems.size();
}

void CHARACTER::FakePC_Owner_AddAffect(CAffect* pkAff)
{
	if (pkAff->dwType < 200)
		return;

	if (!FakePC_CanAddAffect(pkAff))
		return;

	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
		(*it)->FakePC_AddAffect(pkAff);
}

void CHARACTER::FakePC_Owner_RemoveAffect(CAffect* pkAff)
{
	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
		(*it)->FakePC_RemoveAffect(pkAff);
}

// general apply
void CHARACTER::FakePC_Owner_ApplyPoint(BYTE bType, int lValue)
{
	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
		(*it)->ApplyPoint(bType, lValue);
}

// equip / unequip items
void CHARACTER::FakePC_Owner_ItemPoints(LPITEM pkItem, bool bAdd)
{
	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
	{
		LPCHARACTER pkFakePC = (*it);

		pkItem->ModifyPoints(bAdd, pkFakePC);
		pkFakePC->SetImmuneFlag(GetImmuneFlag());
	}
}

// mount
void CHARACTER::FakePC_Owner_MountBuff(bool bAdd)
{
	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
		GetMountSystem()->GiveBuff(bAdd, (*it));
}

void CHARACTER::FakePC_ComputeDamageFactor()
{
	m_fFakePCDamageFactor = 0.5f;
	if (m_pkFakePCSpawnItem)
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (m_pkFakePCSpawnItem->GetAttributeType(i) != 0)
			{
				if (m_pkFakePCSpawnItem->GetAttributeValue(i) != 0)
				{
					if (get_global_time() >= m_pkFakePCSpawnItem->GetAttributeValue(i))
						continue;
				}

				m_fFakePCDamageFactor += ((float)m_pkFakePCSpawnItem->GetAttributeType(i) / 100.0f);
			}
		}
	}
}

LPCHARACTER CHARACTER::FakePC_Owner_Spawn(int lX, int lY, LPITEM pkItem, bool bIsEnemy, bool bIsRedPotionEnabled)
{
	if (!IsPC())
	{
		sys_err("cannot spawn fake pc for mob %u", GetRaceNum());
		return NULL;
	}

	DWORD dwRaceNum = MAIN_RACE_MOB_WARRIOR_M + GetRaceNum();
	int lMapIndex = GetMapIndex();

	LPCHARACTER pkFakePC = CHARACTER_MANAGER::instance().SpawnMob(dwRaceNum, lMapIndex, lX, lY, GetZ(), false, -1, false);
	if (!pkFakePC)
	{
		sys_err("cannot spawn fake pc %u for player %u %s", dwRaceNum, GetPlayerID(), GetName());
		return NULL;
	}

	pkFakePC->FakePC_Load(this, pkItem);
	pkFakePC->SetHP(pkFakePC->GetMaxHP());
	pkFakePC->SetSP(pkFakePC->GetMaxSP());

	if (bIsEnemy)
	{
		pkFakePC->SetPVPTeam(SHRT_MAX);
		if (LPDUNGEON pkDungeon = GetDungeon())
			pkFakePC->SetDungeon(pkDungeon);
	}
	
	//if (!bIsRedPotionEnabled)
	//	pkFakePC->DisableHealPotions();

	pkFakePC->Show(lMapIndex, lX, lY);

	return pkFakePC;
}

void CHARACTER::FakePC_Owner_DespawnAll()
{
	if (!CHARACTER_MANAGER::instance().IsPendingDestroy())
	{
		while (!m_set_pkFakePCSpawns.empty())
			M2_DESTROY_CHARACTER(*(m_set_pkFakePCSpawns.begin()));
	}
	else
	{
		for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
			M2_DESTROY_CHARACTER(*it);
	}
}

bool CHARACTER::FakePC_Owner_DespawnAllSupporter()
{
	// create temporary vector copy of all spawned fake pcs
	std::vector<LPCHARACTER> vec_pkSpawns;
	vec_pkSpawns.reserve(m_set_pkFakePCSpawns.size());

	int i = 0;
	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it, ++i)
		vec_pkSpawns[i] = *it;

	// destroy all fake pc supporter in the vector
	int iCount = 0;
	for (i = 0; i < vec_pkSpawns.size(); ++i)
	{
		LPCHARACTER ch = vec_pkSpawns[i];

		if (!ch->FakePC_IsSupporter())
			continue;

		M2_DESTROY_CHARACTER(ch);
		++iCount;
	}

	return iCount > 0;
}

bool CHARACTER::FakePC_Owner_DespawnByItem(LPITEM pkItem)
{
	itertype(m_map_pkFakePCSpawnItems) it = m_map_pkFakePCSpawnItems.find(pkItem);
	if (it == m_map_pkFakePCSpawnItems.end())
		return false;

	M2_DESTROY_CHARACTER(it->second);
	return true;
}

void CHARACTER::FakePC_Owner_ForceFocus(LPCHARACTER pkVictim)
{
	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
	{
		LPCHARACTER pkFakePC = *it;

		if (!pkFakePC->FakePC_IsSupporter() || !pkFakePC->FakePC_CanAttack())
			continue;
		
		if (pkFakePC->IsDead() || pkFakePC->IsStun())
			continue;
		
		pkFakePC->BeginFight(pkVictim);
	}
}

BYTE CHARACTER::FakePC_ComputeComboIndex()
{
	BYTE bComboSequence = 0;

	if (!GetWear(WEAR_WEAPON))
		return bComboSequence;

	if (GetComboSequence() > 0 && GetComboSequence() <= 3)
	{
		DWORD dwInterval = get_dword_time() - GetLastComboTime();
		if (dwInterval <= GetValidComboInterval())
			bComboSequence = GetComboSequence();
	}

	return bComboSequence;
}

bool CHARACTER::FakePC_IsSkillNeeded(CSkillProto* pkSkill)
{
	if (pkSkill->dwAffectFlag != 0)
	{
		if (FindAffect(pkSkill->dwVnum) != NULL)
			return false;
		else
			return !IsGoodAffect(pkSkill->dwVnum) || !IsAffectFlag(AFF_PABEOP);
	}
	else if (pkSkill->bPointOn != POINT_NONE)
	{
		if (pkSkill->bPointOn == POINT_HP)
		{
			if (GetHPPct() > 90)
				return false;
		}
	}

	return true;
}

bool CHARACTER::FakePC_IsBuffSkill(DWORD dwVnum) {
	switch (dwVnum) {
		case SKILL_HOSIN:
		case SKILL_REFLECT:
		case SKILL_GICHEON:
		case SKILL_JEONGEOP:
		case SKILL_KWAESOK:
		case SKILL_JEUNGRYEOK:
			return true;
		default:
			return false;
	}
	return false;
}

bool CHARACTER::FakePC_UseSkill(LPCHARACTER pkTarget)
{
	if (!FakePC_Check())
	{
		sys_err("cannot use fake pc skills on non-fake pc %s [pid %u vid %u]",
			GetName(), GetPlayerID(), (DWORD)GetVID());
		return false;
	}

	if (!GetSectree())
		return false;

	if (!FakePC_CanAttack())
		return false;

	// check animation time
	if (m_dwLastSkillVnum != 0)
	{
		DWORD dwTimeDif = get_dword_time() - m_dwLastSkillTime;
		if (dwTimeDif < 2000)
			return false;

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
			DWORD dwDuration = CMotionManager::instance().GetMotionDuration(GetRaceNum(), dwDurationMotionKey) * 1000.0f;

			if (dwTimeDif < dwDuration)
				return false;
		}
	}

	// get distance
	int iDistToTarget = 0;
	if (pkTarget)
		iDistToTarget = DISTANCE_APPROX(GetX() - pkTarget->GetX(), GetY() - pkTarget->GetY());

	// check if attr is BANPK
	bool bIsBanPK = false;
	if (GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK) ||
		(pkTarget && pkTarget->GetSectree() && pkTarget->GetSectree()->IsAttr(pkTarget->GetX(), pkTarget->GetY(), ATTR_BANPK)))
		bIsBanPK = true;

	const DWORD* pSkills = GetUsableSkillList();
	if (pSkills)
	{
		// SUPPORT SKILLS
		for (int i = 0; i < CHARACTER_SKILL_COUNT; ++i)
		{
			// get skill vnum
			DWORD dwVnum = pSkills[i];
			if (!dwVnum)
				continue;

			// check if skill is support skill
			CSkillProto* pkProto = CSkillManager::instance().Get(dwVnum);
			if (!pkProto || IS_SET(pkProto->dwFlag, SKILL_FLAG_ATTACK))
				continue;

			// check level
			if (GetSkillLevel(dwVnum) <= 0)
				continue;

			// check if the skill is usable (cooltime)
			if (!m_SkillUseInfo[dwVnum].IsCooltimeOver())
				continue;

			// skills with only-display effects aren't used by npcs
			if (pkProto->bPointOn == POINT_NONE)
				continue;

			// check if self needed
			bool bNeedSelf = FakePC_IsSkillNeeded(pkProto);
			if (!bNeedSelf)
			{
				if (!IS_SET(pkProto->dwFlag, SKILL_FLAG_SELFONLY))
				{
					FFindFakePCSkillVictim f(this, pkProto);
					GetSectree()->ForEachAround(f);

					if (f.m_pkVictim != NULL)
					{
						//block buffs for main char
						// if (f.m_pkVictim == FakePC_GetOwner() && FakePC_IsBuffSkill(dwVnum))
						// {
							// continue;
						// }
						if (!FakePC_IsBuffSkill(dwVnum) && UseSkill(dwVnum, f.m_pkVictim))
						{
							SetRotationToXY(f.m_pkVictim->GetX(), f.m_pkVictim->GetY());
							FakePC_SendSkillPacket(pkProto);
							return true;
						}
					}
				}

				continue;
			}


			// use skill
			if (UseSkill(dwVnum, this))
			{
				FakePC_SendSkillPacket(pkProto);
				return true;
			}
		}

		// ATTACK SKILLS
		if (pkTarget && !bIsBanPK && (GetComboSequence() != 0 || GetLastComboTime() == 0 || get_dword_time() - GetLastComboTime() >= 1000))
		{
			SetRotationToXY(pkTarget->GetX(), pkTarget->GetY());

			for (int i = 0; i < CHARACTER_SKILL_COUNT; ++i)
			{
				// get skill vnum
				DWORD dwVnum = pSkills[i];
				if (!dwVnum)
					continue;

				// check if skill is attack skill
				CSkillProto* pkProto = CSkillManager::instance().Get(dwVnum);
				if (!pkProto || !IS_SET(pkProto->dwFlag, SKILL_FLAG_ATTACK))
					continue;

				// check level
				if (GetSkillLevel(dwVnum) <= 0)
					continue;

				// check if the skill is usable (cooltime)
				if (!m_SkillUseInfo[dwVnum].IsCooltimeOver())
					continue;
				
				// check range
				if (!IS_SET(pkProto->dwFlag, SKILL_FLAG_SELFONLY) || IS_SET(pkProto->dwFlag, SKILL_FLAG_SPLASH))
				{
					DWORD dwMaxRange = pkProto->dwTargetRange;
					if (!dwMaxRange)
						dwMaxRange = pkProto->iSplashRange;
					if (!dwMaxRange)
						dwMaxRange = 250;

					if (iDistToTarget >= dwMaxRange)
						continue;
				}

				// use skill
				if (UseSkill(dwVnum, pkTarget))
				{
					FakePC_SendSkillPacket(pkProto);
					return true;
				}
			}
		}
	}

	return false;
}

void CHARACTER::FakePC_SendSkillPacket(CSkillProto* pkSkill)
{
	network::GCOutputPacket<network::GCSkillMotionPacket> packet;
	packet->set_vid(GetVID());
	packet->set_x(GetX());
	packet->set_y(GetY());
	packet->set_rot(GetRotation() / 5.0f);
	packet->set_time(get_dword_time());
	packet->set_skill_vnum(pkSkill->dwVnum);
	packet->set_skill_level(GetSkillLevel(pkSkill->dwVnum));
	packet->set_skill_grade(GetSkillMasterType(pkSkill->dwVnum));
	PacketView(packet);
}

void CHARACTER::FakePC_Owner_ExecFunc(void(CHARACTER::* func)())
{
	if (test_server && m_set_pkFakePCSpawns.size() > 0)
		sys_log(0, "FakePC_Owner_ExecFunc [%s] countOf[%d]", GetName(), m_set_pkFakePCSpawns.size());

	for (itertype(m_set_pkFakePCSpawns) it = m_set_pkFakePCSpawns.begin(); it != m_set_pkFakePCSpawns.end(); ++it)
		((*it)->*func)();
}
#endif
