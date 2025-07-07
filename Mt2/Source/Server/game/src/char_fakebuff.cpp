#include "stdafx.h"

#ifdef __FAKE_BUFF__
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "skill.h"
#include "motion.h"
#include "sectree.h"
#include "party.h"
#include "config.h"
#include "desc.h"
#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

struct FFindFakeBuffSkillVictim
{
	FFindFakeBuffSkillVictim(LPCHARACTER pkFakeBuff, std::vector<CSkillProto*>& rVecUsableSkillList) :
		m_pkChr(pkFakeBuff), m_rvec_UsableSkillList(rVecUsableSkillList)
	{
		m_pkOwner = m_pkChr->FakeBuff_GetOwner();
		m_pkVictim = nullptr;
		m_pkNeedSkill = nullptr;
	}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		if (m_pkVictim && m_pkVictim == m_pkOwner)
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		if (pkChr->IsDead())
			return;

		if (pkChr->IsPC())
		{
			if (m_pkOwner != pkChr)
			{
#ifdef ELONIA
				return;
#else
				if (!m_pkOwner->GetParty() || !m_pkOwner->GetParty()->IsMember(pkChr->GetPlayerID()))
					return;
#endif
			}
		}
		else
			return;

		int iDist = DISTANCE_SQRT(m_pkChr->GetX() - pkChr->GetX(), m_pkChr->GetY() - pkChr->GetY());

		for (auto skillProto : m_rvec_UsableSkillList)
		{
			//m_pkOwner->tchat("FakeBuff : CheckSkill [%d] dist [%d] maxDist[%d] isNeeded[%d]",
			//	skillProto->dwVnum, iDist, skillProto->dwTargetRange, pkChr->FakeBuff_IsSkillNeeded(skillProto));

			if (iDist > skillProto->dwTargetRange)
				continue;

			if (pkChr->FakeBuff_IsSkillNeeded(skillProto))
			{
				m_pkVictim = pkChr;
				m_pkNeedSkill = skillProto;
				break;
			}
		}
	}

	std::vector<CSkillProto*>&	m_rvec_UsableSkillList;
	LPCHARACTER				m_pkChr;
	LPCHARACTER				m_pkOwner;
	LPCHARACTER				m_pkVictim;
	CSkillProto*			m_pkNeedSkill;
};

void CHARACTER::FakeBuff_Load(LPCHARACTER pkOwner, LPITEM pkSpawnItem)
{
	if (m_pkFakeBuffOwner)
		return;

	if (test_server)
		sys_log(0, "FAKEBUFF: FakeBuff_Load(%p [%u %s], %p [%u %s])", pkOwner, pkOwner->GetPlayerID(), pkOwner->GetName(), pkSpawnItem, pkSpawnItem->GetID(), pkSpawnItem->GetName());

	// init owner
	pkOwner->FakeBuff_Owner_SetSpawn(this);
	pkOwner->FakeBuff_SetItem(pkSpawnItem);

	// init this
	m_pkFakeBuffOwner = pkOwner;
	m_pkFakeBuffItem = pkSpawnItem;

	// set item info
	if (m_pkFakeBuffItem)
	{
	//	m_pkFakeBuffItem->SetItemActive(true);
		m_pkFakeBuffItem->StartTimerBasedOnWearExpireEvent();
	}

	// set general
	m_bPKMode = PK_MODE_PROTECT;
	SetLevel(pkOwner->GetLevel());
	SetEmpire(pkOwner->GetEmpire());

	// skillgroup
	BYTE bSkillGroup = random_number(1, 2);
	if (pkSpawnItem->GetValue(0) == 1 || pkSpawnItem->GetValue(0) == 2)
		bSkillGroup = pkSpawnItem->GetValue(0);
	SetSkillGroup(bSkillGroup);
	
	// set name
	const char* szName = pkOwner->FakeBuff_Owner_GetName();
	// if (*szName)
	// 	SetName(((std::string)szName) + " (" + (bSkillGroup == 1 ? LC_TEXT(GetLanguageID(), "Buffi") : LC_TEXT(GetLanguageID(), "Heilerin")) + ")");
	if(*szName)
	{
		SetName(((std::string)szName));// + " (" + (bSkillGroup == 1 ? LC_TEXT(GetLanguageID(), "Buffi") : LC_TEXT(GetLanguageID(), "Heilerin")) + ")");
	}
	else
		SetName(((std::string)pkOwner->GetName()) + LC_TEXT(GetLanguageID(), "'s ") + (bSkillGroup == 1 ? LC_TEXT(GetLanguageID(), "Buffi") : LC_TEXT(GetLanguageID(), "Heilerin")));

	DWORD vid = GetVID();
	FakeBuff_GetOwner()->ChatPacket(CHAT_TYPE_COMMAND, "SetOwnedFakeBuffVID %u", vid);

	// set int
	/*int iInt = GetLevel();
	if (m_pkFakeBuffItem)
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (m_pkFakeBuffItem->GetAttributeValue(i) != 0)
				iInt += m_pkFakeBuffItem->GetAttributeValue(i);
		}
	}*/
	int iInt = 110;
	if (pkSpawnItem->GetValue(1) != 0)
		iInt = pkSpawnItem->GetValue(1);
	SetRealPoint(POINT_IQ, iInt);
	PointChange(POINT_IQ, iInt - GetPoint(POINT_IQ));

	// load parts
	for (int i = 0; i < PART_MAX_NUM; ++i)
	{
		SetPart(i, FakeBuff_GetPart(i));
		if (test_server)
			FakeBuff_GetOwner()->ChatPacket(CHAT_TYPE_INFO, "FakeBuff part %d : %u", i, GetPart(i));
	}
}

void CHARACTER::FakeBuff_Destroy()
{
	if (!FakeBuff_Check())
		return;

	if (test_server)
		sys_log(0, "FAKEBUFF: FakeBuff_Destroy()");

	// deinit owner
	m_pkFakeBuffOwner->FakeBuff_Owner_SetSpawn(NULL);
	m_pkFakeBuffOwner->FakeBuff_SetItem(NULL);

	// set item info
	if (m_pkFakeBuffItem)
	{
	//	m_pkFakeBuffItem->SetItemActive(false);
		m_pkFakeBuffItem->StopTimerBasedOnWearExpireEvent();
	}

	// clear
	m_pkFakeBuffOwner = NULL;
	m_pkFakeBuffItem = NULL;
}

DWORD CHARACTER::FakeBuff_GetPart(BYTE bPart) const
{
#ifdef __SKIN_SYSTEM__
	LPCHARACTER pOwner = FakeBuff_GetOwner();

	if(pOwner)
	{
		if(bPart == PART_MAIN)
		{
			if(pOwner->GetWear(SKINSYSTEM_SLOT_BUFFI_BODY))
				return pOwner->GetWear(SKINSYSTEM_SLOT_BUFFI_BODY)->GetVnum();
		}
		else if(bPart == PART_WEAPON)
		{
			if(pOwner->GetWear(SKINSYSTEM_SLOT_BUFFI_WEAPON))
				return pOwner->GetWear(SKINSYSTEM_SLOT_BUFFI_WEAPON)->GetVnum();
		}
		else if (bPart == PART_HAIR)
		{
			if (pOwner->GetWear(SKINSYSTEM_SLOT_BUFFI_HAIR))
				return pOwner->GetWear(SKINSYSTEM_SLOT_BUFFI_HAIR)->GetValue(3);
		}
	}
#endif

	const DWORD* c_sdwArmors = adwArmorCollection[JOB_SHAMAN];
	const static DWORD c_sdwWeapons[] = {7009, 7019, 7029, 7039, 7049, 7059, 7069, 7079, 7089, 7099, 7109, 7119, 7129, 7149, 7159};

	const DWORD* pdwList = NULL;
	int iListSize = 0;

	switch (bPart)
	{
	case PART_MAIN:
		pdwList = c_sdwArmors;
		iListSize = ARMOR_COLLECTION_MAX_COUNT;
		break;

	case PART_WEAPON:
		pdwList = c_sdwWeapons;
		iListSize = sizeof(c_sdwWeapons) / sizeof(DWORD);
		break;
	}

	if (!pdwList)
		return GetOriginalPart(bPart);

	for (int i = iListSize - 1; i >= 0; --i)
	{
		DWORD dwVnum = pdwList[i];
		dwVnum += 9 - (dwVnum % 10); // make sure it's +9

		auto pTab = ITEM_MANAGER::instance().GetTable(dwVnum);
		if (!pTab)
			continue;

		int iLevelLimitIndex = -1;
		for (int iLimitIndex = 0; iLimitIndex < ITEM_LIMIT_MAX_NUM; ++iLimitIndex)
		{
			if (pTab->limits(iLimitIndex).type() == LIMIT_LEVEL)
			{
				iLevelLimitIndex = iLimitIndex;
				break;
			}
		}
		if (iLevelLimitIndex != -1)
		{
			if (pTab->limits(iLevelLimitIndex).value() > GetLevel())
				continue;
		}

		return dwVnum;
	}

	return 0;
}

LPCHARACTER CHARACTER::FakeBuff_Owner_Spawn(int lX, int lY, LPITEM pkItem)
{
	if (!IsPC())
	{
		sys_err("cannot spawn fake pc for mob %u", GetRaceNum());
		return NULL;
	}

	DWORD dwRaceNum = MAIN_RACE_MOB_SHAMAN_W;
	if (GetQuestFlag("fake_buff.gender"))
		dwRaceNum = MAIN_RACE_MOB_SHAMAN_M;
	int lMapIndex = GetMapIndex();

	// Dont allow Buffi on Tagteam Event Map
	if (IsPrivateMap(EVENT_LABYRINTH_MAP_INDEX))
		return NULL;

#ifdef COMBAT_ZONE
	else if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return NULL;
#endif

#ifdef ENABLE_REACT_EVENT
	if (lMapIndex == REACT_EVENT_MAP)
		return NULL;
#endif

	if (lMapIndex == PVP_TOURNAMENT_MAP_INDEX)
		return NULL;
	
	LPCHARACTER pkFakeBuff = CHARACTER_MANAGER::instance().SpawnMob(dwRaceNum, lMapIndex, lX, lY, GetZ(), false, -1, false);
	if (!pkFakeBuff)
	{
		sys_err("cannot spawn fake pc %u for player %u %s", dwRaceNum, GetPlayerID(), GetName());
		return NULL;
	}

	pkFakeBuff->FakeBuff_Load(this, pkItem);
	pkFakeBuff->Show(lMapIndex, lX, lY);

	return pkFakeBuff;
}

bool CHARACTER::FakeBuff_Owner_Despawn()
{
	if (!m_pkFakeBuffSpawn)
		return false;

	M2_DESTROY_CHARACTER(m_pkFakeBuffSpawn);
	return true;
}

void CHARACTER::FakeBuff_Local_Warp(long lMapIndex, int lX, int lY)
{
	if (!m_pkFakeBuffSpawn)
		return;

	m_pkFakeBuffSpawn->Show(lMapIndex, lX, lY, 0);
}

bool CHARACTER::FakeBuff_IsSkillNeeded(CSkillProto* pkSkill)
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

const DWORD* CHARACTER::FakeBuff_GetUsableSkillList() const
{
#ifdef AELDRA
	static const DWORD BuffiSkillList[2][CHARACTER_SKILL_COUNT] =
	{
		 { 91, 92, 93, 94, 95, 96 }, { 91, 92, 93, 94, 111, 96 }
	};
	return BuffiSkillList[(strlen(FakeBuff_GetOwner()->FakeBuff_Owner_GetName()) > 1) ? 1 : 0];
#else
	static const DWORD BuffiSkillList[CHARACTER_SKILL_COUNT] = { 91, 92, 93, 94, 95, 96 };
	return BuffiSkillList;
#endif
}

bool CHARACTER::FakeBuff_UseSkill(LPCHARACTER pkTarget)
{
	if (!FakeBuff_Check())
	{
		sys_err("cannot use fake buff skills on non-fake char %s [pid %u vid %u]",
			GetName(), GetPlayerID(), (DWORD)GetVID());
		return false;
	}

	if (!GetSectree())
		return false;

	//if (test_server)
	//	FakeBuff_GetOwner()->ChatPacket(CHAT_TYPE_INFO, "FakeBuff_UseSkill(%p)", pkTarget);

	// check animation time
	if (m_dwLastSkillVnum != 0)
	{
		DWORD dwTimeDif = get_dword_time() - m_dwLastSkillTime;
		if (dwTimeDif < 750)
			return false;

		CSkillProto* pkSkill = CSkillManager::Instance().Get(m_dwLastSkillVnum);
		if (pkSkill)
		{
			DWORD dwSkillIndex = 0;
			const DWORD* dwSkillList = FakeBuff_GetUsableSkillList();
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

	std::vector<CSkillProto*> vec_UsableSkillList;

	const DWORD* pSkills = FakeBuff_GetUsableSkillList();
	if (pSkills)
	{
		// SUPPORT SKILLS
		for (int i = 0; i < CHARACTER_SKILL_COUNT; ++i)
		{
			// get skill vnum
			DWORD dwVnum = pSkills[i];
			if (!dwVnum)
				continue;

			// ignore specific skill
			if (dwVnum == SKILL_REFLECT)
				continue;

			// check if skill is support skill
			CSkillProto* pkProto = CSkillManager::instance().Get(dwVnum);
			if (!pkProto || IS_SET(pkProto->dwFlag, SKILL_FLAG_ATTACK))
				continue;

			// check level
			if (GetSkillLevel(dwVnum) <= 0)
				continue;

			// skills with only-display effects aren't used by npcs
			if (pkProto->bPointOn == POINT_NONE)
				continue;

			// selfonly skills can't be used
			if (IS_SET(pkProto->dwFlag, SKILL_FLAG_SELFONLY))
				continue;

			// check if the skill is usable (cooltime)
			if (!m_SkillUseInfo[dwVnum].IsCooltimeOver())
				continue;

			vec_UsableSkillList.push_back(pkProto);
		}
	}

	FFindFakeBuffSkillVictim f(this, vec_UsableSkillList);
	GetSectree()->ForEachAround(f);

	//if (test_server)
	//	FakeBuff_GetOwner()->ChatPacket(CHAT_TYPE_INFO, "FakeBuff_UseSkill count %d victim %p need skill %u", vec_UsableSkillList.size(), f.m_pkVictim, f.m_pkVictim ? f.m_pkNeedSkill->dwVnum : 0);

	if (f.m_pkVictim != NULL)
	{
#ifdef STANDARD_SKILL_DURATION // here instead of like when spawn because the character may increase the skill level
		SetPoint(POINT_SKILL_DURATION, f.m_pkVictim->GetPoint(POINT_SKILL_DURATION));
#endif
		if (UseSkill(f.m_pkNeedSkill->dwVnum, f.m_pkVictim))
		{
			SetRotationToXY(f.m_pkVictim->GetX(), f.m_pkVictim->GetY());
			FakeBuff_SendSkillPacket(f.m_pkNeedSkill);
			return true;
		}
	}

	return false;
}

void CHARACTER::FakeBuff_SendSkillPacket(CSkillProto* pkSkill)
{
	network::GCOutputPacket<network::GCSkillMotionPacket> packet;
	packet->set_vid(GetVID());
	packet->set_x(GetX());
	packet->set_y(GetY());
	packet->set_rotation(GetRotation() / 5.0f);
	packet->set_time(get_dword_time());
	packet->set_skill_vnum(pkSkill->dwVnum);
	packet->set_skill_level(GetSkillLevel(pkSkill->dwVnum));
	packet->set_skill_grade(GetSkillMasterType(pkSkill->dwVnum));
	PacketView(packet);
}

DWORD CHARACTER::GetFakeBuffSkillVnum(BYTE bIndex) const
{
	if (test_server)
		sys_log(0, "FAKEBUFF: %s: GetFakeBuffSkillVnum(%u)", GetName(), bIndex);

	if (bIndex >= FAKE_BUFF_SKILL_COUNT)
	{
		sys_err("invalid fake buff skill index");
		return 0;
	}

	DWORD dwVnum = adwFakeBuffSkillVnums[bIndex];
	if (test_server)
		sys_log(0, "__FAKEBUFF: return vnum %u", dwVnum);
	return dwVnum;
}

BYTE CHARACTER::GetFakeBuffSkillIdx(DWORD dwSkillVnum) const
{
#ifdef __NEW_SHAMAN_SKILL__
	if (dwSkillVnum == 95)
		return FAKE_BUFF_SKILL_COUNT;
#endif

	if (this && FakeBuff_GetOwner() && strlen(FakeBuff_GetOwner()->FakeBuff_Owner_GetName()) <= 1 && dwSkillVnum == 111) // companion system skill 111 allowed
		return 0;

	for (int i = 0; i < FAKE_BUFF_SKILL_COUNT; ++i)
	{
		DWORD dwVnum = adwFakeBuffSkillVnums[i];
		if (dwVnum == dwSkillVnum)
			return i;
	}

	return FAKE_BUFF_SKILL_COUNT;
}

void CHARACTER::SetFakeBuffSkillLevel(DWORD dwSkillVnum, BYTE bLevel)
{
	if (test_server)
		sys_log(0, "FAKEBUFF: %s: SetFakeBuffSkillLevel %u %u", GetName(), dwSkillVnum, bLevel);

	BYTE bIndex = GetFakeBuffSkillIdx(dwSkillVnum);
	if (bIndex == FAKE_BUFF_SKILL_COUNT)
	{
		sys_err("invalid skill vnum for fake buff %u", dwSkillVnum);
		return;
	}

	m_abFakeBuffSkillLevel[bIndex] = bLevel;
	SendFakeBuffSkills();
}

BYTE CHARACTER::GetFakeBuffSkillLevel(DWORD dwSkillVnum) const
{
	//if (test_server)

	BYTE bIndex = GetFakeBuffSkillIdx(dwSkillVnum);
	if (bIndex == FAKE_BUFF_SKILL_COUNT)
	{
	//	sys_err("invalid skill vnum for fake buff %u", dwSkillVnum);
		return 20;
	}

	return MINMAX(20, m_abFakeBuffSkillLevel[bIndex], SKILL_MAX_LEVEL);
}

bool CHARACTER::CanFakeBuffSkillUp(DWORD dwSkillVnum, bool bUseBook)
{
	if (test_server)
		sys_log(0, "FAKEBUFF: %s: CanFakeBuffSkillUp(%u, %u)", GetName(), dwSkillVnum, bUseBook);

	if (bUseBook && !FakeBuff_Owner_GetSpawn())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Summon your buffi to skill up its skills."));
		return false;
	}

	if (GetFakeBuffSkillIdx(dwSkillVnum) == FAKE_BUFF_SKILL_COUNT)
		return false;

	if (!(bUseBook == (GetFakeBuffSkillLevel(dwSkillVnum) < 30)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot skill up this skill with that item."));
		return false;
	}
	
#ifdef __NEW_SHAMAN_SKILL__
	if (dwSkillVnum == 95)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot skill up this skill."));
		return false;
	}
#endif

	return true;
}

void CHARACTER::SendFakeBuffSkills(DWORD dwSkillVnum)
{
#ifdef __NEW_SHAMAN_SKILL__
	if (dwSkillVnum == 95)
		return;
#endif
	if (dwSkillVnum)
	{
		BYTE bIndex = GetFakeBuffSkillIdx(dwSkillVnum);
		if (bIndex == FAKE_BUFF_SKILL_COUNT)
		{
			sys_err("invalid skill vnum for fake buff %u", dwSkillVnum);
			return;
		}

		network::GCOutputPacket<network::GCFakeBuffSkillPacket> pack;
		pack->set_skill_vnum(dwSkillVnum);

#ifdef AELDRA
		if (dwSkillVnum != 111 || strlen(FakeBuff_Owner_GetName()) > 1)
			pack->set_level(GetFakeBuffSkillLevel(dwSkillVnum));
		else
			pack->set_level(0);
#else
		pack->set_level(GetFakeBuffSkillLevel(dwSkillVnum));
#endif
		
		GetDesc()->Packet(pack);
	}
	else
	{
		for (int i = 0; i < FAKE_BUFF_SKILL_COUNT; ++i)
		{
			DWORD dwVnum = adwFakeBuffSkillVnums[i];
			SendFakeBuffSkills(dwVnum);
		}
	}
}
#endif
