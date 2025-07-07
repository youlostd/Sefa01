#include "stdafx.h"
#include "mount_system.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "char_manager.h"
#include "guild.h"
#include "questmanager.h"
#include "refine.h"
#include "desc.h"

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

extern int test_server;
extern int passes_per_sec;

EVENTFUNC(mount_check_item_event)
{
	mount_system_event_info* info = dynamic_cast<mount_system_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("mount_check_item_event <Factor> Null pointer");
		return 0;
	}

	CMountSystem* pkMountSystem = info->pkMountSystem;
	if (pkMountSystem == NULL) { // <Factor>
		return 0;
	}

	if (!pkMountSystem->IsSummoned() && !pkMountSystem->IsRiding())
	{
		pkMountSystem->SetCheckItemEvent(NULL);
		return 0;
	}

	DWORD dwItemID = pkMountSystem->GetSummonItemID();
	if (!dwItemID)
	{
		sys_err("no item id found for player %d %s", pkMountSystem->GetOwner()->GetPlayerID(), pkMountSystem->GetOwner()->GetName());
		pkMountSystem->SetCheckItemEvent(NULL);
		if (pkMountSystem->IsRiding())
			pkMountSystem->StopRiding();
		if (pkMountSystem->IsSummoned())
			pkMountSystem->Unsummon();
		return 0;
	}
		
	LPITEM pkSummonItem = ITEM_MANAGER::instance().Find(dwItemID);
	if (!pkSummonItem || pkSummonItem->GetOwner() != pkMountSystem->GetOwner())
	{
		DWORD dwNewOwnerPID = (pkSummonItem && pkSummonItem->GetOwner()) ? pkSummonItem->GetOwner()->GetPlayerID() : 0;
		const char * szNewOwnerName = (pkSummonItem && pkSummonItem->GetOwner()) ? pkSummonItem->GetOwner()->GetName() : "<NONE>";
		sys_err("cannot find summon item or owner changed [itemID %u, owner %d %s, new owner %d %s]",
				pkSummonItem ? pkSummonItem->GetID() : 0, pkMountSystem->GetOwner()->GetPlayerID(), pkMountSystem->GetOwner()->GetName(),
				dwNewOwnerPID, szNewOwnerName);
		pkMountSystem->SetCheckItemEvent(NULL);
		if (pkMountSystem->IsRiding())
			pkMountSystem->StopRiding();
		pkMountSystem->Unsummon();
		return 0;
	}

	quest::PC* pPC = quest::CQuestManager::Instance().GetPC(pkMountSystem->GetOwner()->GetPlayerID());
	if (pPC && !pPC->IsRunning())
	{
		pkMountSystem->GetOwner()->SetQuestItemPtr(pkSummonItem);
		quest::CQuestManager::instance().OnMountRiding(pPC->GetID());
	}

	return PASSES_PER_SEC(15);
}

CMountSystem::CMountSystem(LPCHARACTER pkOwner)
{
	Initialize();

	m_pkOwner = pkOwner;
}

CMountSystem::~CMountSystem()
{
	if (IsRiding())
		StopRiding();
	if (IsSummoned())
		Unsummon();
}

void CMountSystem::Initialize()
{
	m_pkOwner = NULL;
	m_pkMount = NULL;
	m_pkCheckItemEvent = NULL;

	m_dwItemID = 0;
	m_bState = MOUNT_NONE;

	m_bIsStartRiding = false;

	m_fMountBuffBonus = 0.0f;
	m_dwMeltMountVnum = 0;

	memset(m_abBonusLevel, 0, sizeof(m_abBonusLevel));
}

void CMountSystem::StartCheckItemEvent()
{
	if (m_pkCheckItemEvent)
		return;

	mount_system_event_info* info = AllocEventInfo<mount_system_event_info>();

	info->pkMountSystem = this;

	SetCheckItemEvent(event_create(mount_check_item_event, info, PASSES_PER_SEC(15)));
}

void CMountSystem::StopCheckItemEvent()
{
	event_cancel(&m_pkCheckItemEvent);
	SetCheckItemEvent(NULL);
}

bool CMountSystem::IsNoSummonMap() const
{
	int lRealMapIndex = m_pkOwner->GetMapIndex() >= 10000 ? m_pkOwner->GetMapIndex() / 10000 : m_pkOwner->GetMapIndex();
#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(lRealMapIndex))
		return true;
#endif

	switch (lRealMapIndex)
	{
		case OXEVENT_MAP_INDEX:
		case GUILD_WAR_MAP_INDEX:
		case EMPIREWAR_MAP_INDEX:
		case EVENT_LABYRINTH_MAP_INDEX:
		case PVP_TOURNAMENT_MAP_INDEX:
#ifdef ENABLE_HYDRA_DUNGEON
		case HYDRA_RUN_MAP_INDEX:
#endif
#ifdef ENABLE_REACT_EVENT
		case REACT_EVENT_MAP:
#endif
			return true;
		case NEW_ENCHANTED_FOREST:
			if (m_pkOwner->GetMapIndex() != NEW_ENCHANTED_FOREST)
				return true;
			return false;
		default:
			return false;
	}
}

bool CMountSystem::IsSummoned() const
{
	return m_bState == MOUNT_SUMMONED && m_pkMount != NULL;
}

bool CMountSystem::IsRiding() const
{
	return m_bState == MOUNT_RIDING && m_pkOwner->GetMountVnum() != 0;
}

bool CMountSystem::Summon(LPITEM pkItem)
{
	if (!pkItem || pkItem->GetType() != ITEM_MOUNT)
		return false;

	if (pkItem->GetOwner() != GetOwner())
	{
		sys_err("invalid owner");
		return false;
	}
	
	if (IsNoSummonMap())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You cannot summon your mount in this map."));
		return false;
	}

	if (GetOwner()->IsDead())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You cannot do this while you are dead."));
		return false;
	}

	if (IsSummoned() || IsRiding() || m_pkOwner->GetMountVnum() != 0)
	{
		if (GetSummonItemID() != pkItem->GetID())
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You have already summoned a mount."));
		return false;
	}

	DWORD dwMountRace = pkItem->GetValue(0);
	bool bMountSkin = false;

#ifdef __SKIN_SYSTEM__
	if(m_pkOwner && m_pkOwner->GetWear(SKINSYSTEM_SLOT_MOUNT))
	{
		LPITEM pMountSkin = m_pkOwner->GetWear(SKINSYSTEM_SLOT_MOUNT);
		DWORD skinVnum = pMountSkin->GetValue(0);

		if(skinVnum)
		{
			dwMountRace = skinVnum;
			bMountSkin = true;
		}
	}
#endif

	if (!dwMountRace)
		return false;

	// value1 > 0 -> value1 == HORSE GRADE -> IS HORSE
	if (pkItem->GetValue(1) > 0)
	{
		if (m_pkOwner->GetGuild() && !bMountSkin)
		{
			dwMountRace++;
			if (m_pkOwner->GetGuild()->GetMasterPID() == m_pkOwner->GetPlayerID())
				dwMountRace++;
		}

		if (m_pkOwner->GetHorseGrade() != pkItem->GetValue(1))
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You need a different item to summon your horse."));
			return false;
		}

		if (m_pkOwner->IsHorseDead())
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "Your horse is dead. You need to revive it before summoning it."));
			return false;
		}

		if (GetOwner()->IsHorsesHoeTimeout())
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You need to give a horsehoe to the Stable Boy to summon your king horse."));
			return false;
		}
	}

	const CMob* pkMob = CMobManager::instance().Get(dwMountRace);
	if (!pkMob || pkMob->m_table.type() != CHAR_TYPE_MOUNT)
	{
		sys_err("unkown mount [itemVnum %d, mobVnum %d, mobType %d (need %d), owner %d %s]",
				pkItem->GetVnum(), pkItem->GetValue(0), pkMob ? pkMob->m_table.type() : -1, CHAR_TYPE_MOUNT, m_pkOwner->GetPlayerID(),
				m_pkOwner->GetName());
		return false;
	}

	m_pkMount = CHARACTER_MANAGER::instance().SpawnMob(dwMountRace, m_pkOwner->GetMapIndex(), m_pkOwner->GetX(), m_pkOwner->GetY(), 0,
			false, -1, false);
	if (!m_pkMount)
	{
		sys_err("cannot spawn mount[%d] for player %d %s (item %u %s)",
				dwMountRace, m_pkOwner->GetPlayerID(), m_pkOwner->GetName(), pkItem->GetID(), pkItem->GetName());
		return false;
	}
	
	m_pkMount->SetName(GetDefaultName(m_pkOwner->GetName()));
	m_pkMount->SetRider(m_pkOwner);
	
	if (!m_pkMount->Show(m_pkOwner->GetMapIndex(), m_pkOwner->GetX() + random_number(-200, 200), m_pkOwner->GetY() + random_number(-200, 200), m_pkOwner->GetZ()))
	{
		M2_DESTROY_CHARACTER(m_pkMount);
		m_pkMount = NULL;
		sys_err("cannot show mount for player %d %s", m_pkOwner->GetPlayerID(), m_pkOwner->GetName());
		return false;
	}

	m_dwItemID = pkItem->GetID();
	m_bState = MOUNT_SUMMONED;

	StartCheckItemEvent();
	pkItem->StartTimerBasedOnWearExpireEvent();

	if (pkItem->GetValue(1) > 0)
	{
		m_pkOwner->StartHorseDeadEvent();

		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "horse_state %d %d %d", m_pkOwner->GetHorseGrade(), m_pkOwner->GetHorseElapsedTime(),
			m_pkOwner->GetHorseMaxLifeTime());

		RefreshBonusLevel(pkItem);
	}
	else
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "hide_horse_state");

	pkItem->SetSocket(2, true);
	
#ifdef ENABLE_COMPANION_NAME
	if (!m_pkOwner->GetMountName().empty())
	{
		
		DWORD vid = m_pkMount->GetVID();
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "SetOwnedMountVID %u", vid);
	}
#endif

	return true;
}

void CMountSystem::Unsummon(bool bDisable)
{
	if (IsRiding())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You have to dismount first."));
		return;
	}
	
	if (!IsSummoned())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "Your mount is already unsummoned."));
		return;
	}

	LPITEM pkItem = ITEM_MANAGER::instance().Find(m_dwItemID);
	if (pkItem)
	{
		pkItem->StopTimerBasedOnWearExpireEvent();
		if (bDisable)
			pkItem->SetSocket(2, false);
	}
	
	M2_DESTROY_CHARACTER(m_pkMount);
}

void CMountSystem::OnDestroySummoned()
{
	if (!m_bIsStartRiding)
	{
		m_pkOwner->StopHorseDeadEvent();
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "hide_horse_state");
	}

	StopCheckItemEvent();

	m_pkMount = NULL;
	m_bState = MOUNT_NONE;
}

bool CMountSystem::StartRiding()
{
	if (GetOwner()->IsDead())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You cannot do this while you are dead."));
		return false;
	}

	if (IsRiding())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You are already riding."));
		return false;
	}

	if (!IsSummoned())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You have to summon your mount first."));
		return false;
	}

	if (m_pkOwner->IsPolymorphed())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "º¯½Å »óÅÂ¿¡¼­´Â ¸»¿¡ Å» ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (LPITEM pkArmor = m_pkOwner->GetWear(WEAR_BODY))
	{
		if (pkArmor->GetVnum() >= 11901 && pkArmor->GetVnum() <= 11904)
		{
			m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "¿¹º¹À» ÀÔÀº »óÅÂ¿¡¼­ ¸»À» Å» ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

#ifdef NO_MOUNTING_IN_PVP
	if (m_pkOwner->IsPVPFighting(NO_MOUNTING_IN_PVP))
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You can't ride your mount %d seconds after pvp damage."), 5);
		return false;
	}
#endif

#ifdef ENABLE_RUNE_SYSTEM
	if(m_pkOwner->FindAffect(AFFECT_RUNE_MOUNT_PARALYZE))
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You can't ride your mount now."));
		return false;
	}
#endif

	m_bIsStartRiding = true;

	DWORD dwRaceNum = m_pkMount->GetRaceNum();
	Unsummon(false);

	m_pkOwner->MountVnum(dwRaceNum);
	m_bState = MOUNT_RIDING;

	LPITEM pkItem = ITEM_MANAGER::instance().Find(m_dwItemID);
	if (pkItem)
	{
		pkItem->StartTimerBasedOnWearExpireEvent();

		m_dwMeltMountVnum = 0;
		if (pkItem->GetValue(1) == CHARACTER::HORSE_MAX_GRADE)
		{
			if (pkItem->GetSocket(1) != 0 && (pkItem->GetSocket(2) == 0 || pkItem->GetSocket(2) > get_global_time()))
				m_dwMeltMountVnum = pkItem->GetSocket(1);
		}
	}

	StartCheckItemEvent();
	GiveBuff();

	if (m_pkOwner->IsHorseSummoned())
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "horse_state %d %d %d", m_pkOwner->GetHorseGrade(), m_pkOwner->GetHorseElapsedTime(), 
			m_pkOwner->GetHorseMaxLifeTime());
	else
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "hide_horse_state");

	quest::CQuestManager::instance().Mount(m_pkOwner->GetPlayerID());

// #ifdef __PET_SYSTEM__
	// if (m_pkOwner->GetPetSystem())
		// m_pkOwner->GetPetSystem()->OnMount_Mount();
// #endif

// dwRaceNum
	


	m_bIsStartRiding = false;

	return true;
}

void CMountSystem::StopRiding(bool bSummon)
{
	if (!IsRiding())
	{
		m_pkOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You are not riding."));
		return;
	}

	StopCheckItemEvent();
	
	m_pkOwner->MountVnum(0);
	m_bState = MOUNT_NONE;

// #ifdef __PET_SYSTEM__
	// if (m_pkOwner->GetPetSystem())
		// m_pkOwner->GetPetSystem()->OnMount_Unmount();
// #endif

	m_pkOwner->ComputePoints();
	m_pkOwner->PointChange(POINT_HT, 0);
	m_pkOwner->PointChange(POINT_ST, 0);
	m_pkOwner->PointChange(POINT_DX, 0);
	m_pkOwner->PointChange(POINT_IQ, 0);

	LPITEM pkItem = ITEM_MANAGER::instance().Find(m_dwItemID);
	if (bSummon && pkItem && pkItem->GetOwner() == m_pkOwner && !m_pkOwner->IsDead() && (!m_pkOwner->IsHorseSummoned() || !m_pkOwner->IsHorsesHoeTimeout()))
		Summon(pkItem);
	else {
		if (pkItem)
			pkItem->StopTimerBasedOnWearExpireEvent();
		m_pkOwner->StopHorseDeadEvent();
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "hide_horse_state");
	}

	quest::CQuestManager::instance().Unmount(m_pkOwner->GetPlayerID());
}

void CMountSystem::GiveBuff(bool bAdd, LPCHARACTER pkChr, bool bIsDoubled)
{
	if (!IsRiding() && bAdd)
		return;

	if (!pkChr)
		pkChr = m_pkOwner;

	if (bAdd)
	{
		int iMountST = 53, iMountDX = 71, iMountHT = 36, iMountIQ = 18;
		if (iMountST > pkChr->GetPoint(POINT_ST))
			pkChr->ApplyPoint(APPLY_STR, iMountST - pkChr->GetPoint(POINT_ST));

		if (iMountDX > pkChr->GetPoint(POINT_DX))
			pkChr->ApplyPoint(APPLY_DEX, iMountDX - pkChr->GetPoint(POINT_DX));

		if (iMountHT > pkChr->GetPoint(POINT_HT))
			pkChr->ApplyPoint(APPLY_CON, iMountHT - pkChr->GetPoint(POINT_HT));

		if (iMountIQ > pkChr->GetPoint(POINT_IQ))
			pkChr->ApplyPoint(APPLY_INT, iMountIQ - pkChr->GetPoint(POINT_IQ));
	}

	// name applies 35 defense if you're riding a horse
	if (m_pkOwner->IsHorseSummoned() && *GetName())
		pkChr->ApplyPoint(APPLY_DEF_GRADE, bAdd ? 35 : -35);

	// apply boni from melt mount item
	if (m_dwMeltMountVnum && !bIsDoubled)
	{
		auto pTable = ITEM_MANAGER::instance().GetTable(m_dwMeltMountVnum);
		if (pTable)
		{
			for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
			{
				if (pTable->applies(i).type() != APPLY_NONE && pTable->applies(i).value() != 0)
				{
					if (test_server)
						pkChr->ChatPacket(CHAT_TYPE_INFO, "<MountSystem> ADD %d Give melt buff type %d value %.2f",
							bAdd, pTable->applies(i).type(), pTable->applies(i).value() * (100.0f + m_fMountBuffBonus) / 100.0f);

					pkChr->ApplyPointF(pTable->applies(i).type(), (bAdd ? pTable->applies(i).value() : -pTable->applies(i).value()) * (100.0f + m_fMountBuffBonus) / 100.0f);
				}
			}
		}
	}

	LPITEM pkSummonItem = ITEM_MANAGER::instance().Find(m_dwItemID);

	if (GetOwner()->IsHorseSummoned() && GetOwner()->GetHorseGrade() >= CHARACTER::HORSE_MAX_GRADE)
	{
		for (int i = 0; i < HORSE_BONUS_MAX_COUNT; ++i)
		{
			if (m_abBonusLevel[i] == 0)
				continue;

			auto pProto = CHARACTER_MANAGER::instance().GetHorseBonus(m_abBonusLevel[i] - 1);
			if (!pProto)
				continue;

			BYTE bBonusApply = APPLY_NONE;
			int iBonusValue;
			switch (i)
			{
			case 0:
				bBonusApply = APPLY_MAX_HP;
				iBonusValue = pProto->max_hp();
				break;
			case 1:
				bBonusApply = APPLY_DEF_GRADE_BONUS;
				iBonusValue = pProto->armor_pct();
				break;
			case 2:
				bBonusApply = APPLY_ATTBONUS_MONSTER;
				iBonusValue = pProto->monster_pct();
				break;
			}

			if (bBonusApply == APPLY_NONE)
			{
				sys_err("invalid bonus index %d", i);
				continue;
			}

			GetOwner()->ApplyPointF(bBonusApply, (bAdd ? iBonusValue : -iBonusValue) * (100.0f + m_fMountBuffBonus) / 100.0f);
		}
	}
	else
	{
		if (pkSummonItem && pkSummonItem->GetOwner() == m_pkOwner)
			pkSummonItem->ModifyPoints(bAdd, pkChr, (100.0f + m_fMountBuffBonus) / 100.0f);
		else if (!bAdd)
		{
			pkChr->ComputePoints();
			return;
		}
	}

#ifdef __MOUNT_EXTRA_SPEED__
	pkChr->tchat("FoundItem %p type %u subType %u", pkSummonItem, pkSummonItem ? pkSummonItem->GetType() : 0, pkSummonItem ? pkSummonItem->GetSubType() : 0);
	if (pkSummonItem && pkSummonItem->GetType() == ITEM_MOUNT && pkSummonItem->GetSubType() == MOUNT_SUB_SUMMON)
	{
		
#ifdef __SKIN_SYSTEM__
		if(m_pkOwner && m_pkOwner->GetWear(SKINSYSTEM_SLOT_MOUNT))
		{
			LPITEM pMountSkin = m_pkOwner->GetWear(SKINSYSTEM_SLOT_MOUNT);
			if (pMountSkin && pMountSkin->GetValue(2))
			{
				pkChr->ApplyPoint(APPLY_MOV_SPEED, bAdd ? pMountSkin->GetValue(2) : -pMountSkin->GetValue(2));
				pkChr->UpdatePacket();
			}
		}
		else
#endif

		if (pkSummonItem->GetValue(2))
		{
			pkChr->ApplyPoint(APPLY_MOV_SPEED, bAdd ? pkSummonItem->GetValue(2) : -pkSummonItem->GetValue(2));
			pkChr->UpdatePacket();
		}
	}
	else if (!bAdd)
		pkChr->ComputePoints();
#endif

	if (!bIsDoubled && m_pkOwner->IsHorseSummoned() && m_pkOwner->IsHorseRage())
		GiveBuff(bAdd, pkChr, true); // double buff in rage mode on horse
}

const char* CMountSystem::GetDefaultName(const char* szPlayerName) const
{
	static std::string stName;
	stName = szPlayerName ? szPlayerName : m_pkOwner->GetName();
	// stName += "'s ";
	// stName += "Reittier";
#ifdef ENABLE_COMPANION_NAME
	if (!m_pkOwner->GetMountName().empty())
	{
		stName = m_pkOwner->GetMountName();
		m_pkMount->SetCompanionHasName(true);
	}
#endif
	return stName.c_str();
}

const char* CMountSystem::GetName() const
{
	if (m_stName.empty())
		return "";

	return m_stName.c_str();
}

void CMountSystem::RefreshMountBuffBonus()
{
	m_fMountBuffBonus = GetOwner()->GetPointF(POINT_MOUNT_BUFF_BONUS);
}

void CMountSystem::RefreshBonusLevel(LPITEM pkItem)
{
	if (IsRiding())
		GiveBuff(false);

	memset(m_abBonusLevel, 0, sizeof(m_abBonusLevel));
	if (pkItem->GetValue(1) == CHARACTER::HORSE_MAX_GRADE)
	{
		for (int i = 0; i < HORSE_BONUS_MAX_COUNT; ++i)
			m_abBonusLevel[i] = pkItem->GetHorseBonusLevel(i);
	}

	if (IsRiding())
		GiveBuff(true);
}

bool CMountSystem::CanRefine(BYTE bRefineIndex)
{
	if (bRefineIndex >= HORSE_REFINE_MAX_NUM)
		return false;

	if (!(IsRiding() || IsSummoned()) || !GetOwner()->IsHorseSummoned() || GetOwner()->GetHorseGrade() < CHARACTER::HORSE_MAX_GRADE)
		return false;

	BYTE bLevel;
	if (bRefineIndex == HORSE_REFINE_RAGE)
	{
		bLevel = GetOwner()->GetHorseRageLevel();
		if (bLevel >= HORSE_RAGE_MAX_LEVEL)
			return false;
	}
	else
	{
		BYTE bBonusIndex = bRefineIndex - HORSE_REFINE_BONUS;
		if (bBonusIndex >= HORSE_BONUS_MAX_COUNT)
			return false;

		if (m_abBonusLevel[bBonusIndex] >= HORSE_MAX_BONUS_LEVEL)
			return false;

		LPITEM pkSummonItem = GetOwner()->FindItemByID(GetSummonItemID());
		if (!pkSummonItem)
			return false;

		if (pkSummonItem->GetHorseBonusLevel(bBonusIndex) != m_abBonusLevel[bBonusIndex])
			return false;

		auto pProto = CHARACTER_MANAGER::instance().GetHorseBonus(m_abBonusLevel[bBonusIndex]);
		if (pkSummonItem->GetHorseUsedBottles(bBonusIndex) < pProto->item_count())
			return false;

		bLevel = m_abBonusLevel[bBonusIndex];
	}

	auto pUpgradeProto = CHARACTER_MANAGER::instance().GetHorseUpgrade(1 + bRefineIndex, bLevel + 1);
	if (!pUpgradeProto || GetOwner()->GetLevel() < pUpgradeProto->level_limit())
		return false;

	return true;
}

void CMountSystem::SendRefineInfo(BYTE bRefineIndex)
{
	if (!CanRefine(bRefineIndex))
	{
		sys_err("cannot refine %d", bRefineIndex);
		return;
	}

	network::GCOutputPacket<network::GCHorseRefineInfoPacket> pack;
	pack->set_refine_index(bRefineIndex);

	if (bRefineIndex == HORSE_REFINE_RAGE)
		pack->set_current_level(GetOwner()->GetHorseRageLevel());
	else
		pack->set_current_level(m_abBonusLevel[bRefineIndex - HORSE_REFINE_BONUS]);
	
	auto pProto = CHARACTER_MANAGER::instance().GetHorseUpgrade(1 + bRefineIndex, pack->current_level() + 1);
	auto pRefineTab = CRefineManager::instance().GetRefineRecipe(pProto->refine_id());

	if (!pRefineTab)
	{
		sys_err("cannot send refine info by refine %d level %d (cannot get refine by refineID %u)", 1 + bRefineIndex, pack->current_level() + 1, pProto->refine_id());
		return;
	}

	*pack->mutable_refine() = *pRefineTab;
	GetOwner()->GetDesc()->Packet(pack);
}

void CMountSystem::DoRefine(BYTE bRefineIndex)
{
	if (!CanRefine(bRefineIndex))
		return;

	BYTE bLevel;
	if (bRefineIndex == HORSE_REFINE_RAGE)
		bLevel = GetOwner()->GetHorseRageLevel();
	else
		bLevel = m_abBonusLevel[bRefineIndex - HORSE_REFINE_BONUS];

	auto pProto = CHARACTER_MANAGER::instance().GetHorseUpgrade(1 + bRefineIndex, bLevel + 1);
	auto pRefineTab = CRefineManager::instance().GetRefineRecipe(pProto->refine_id());

	if (!pRefineTab)
		return;

	if (GetOwner()->GetGold() < pRefineTab->cost())
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "You have not enough money."));
		return;
	}

	for (int i = 0; i < pRefineTab->material_count(); ++i)
	{
		if (GetOwner()->CountSpecifyItem(pRefineTab->materials(i).vnum()) < pRefineTab->materials(i).count())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "You have not all materials."));
			return;
		}
	}

	LPITEM pkSummonItem = GetOwner()->FindItemByID(GetSummonItemID());
	if (!pkSummonItem)
	{
		sys_err("cannot get summon item");
		return;
	}

	if (pRefineTab->cost() > 0)
		GetOwner()->PointChange(POINT_GOLD, -pRefineTab->cost());
	for (int i = 0; i < pRefineTab->material_count(); ++i)
		GetOwner()->RemoveSpecifyItem(pRefineTab->materials(i).vnum(), pRefineTab->materials(i).count());

	network::GCOutputPacket<network::GCHorseRefineResultPacket> pack;
	pack->set_success(pRefineTab->prob() >= random_number(1, 100));

	if (pack->success())
	{
		if (bRefineIndex == HORSE_REFINE_RAGE)
		{
			GetOwner()->SetHorseRageLevel(bLevel + 1);
		}
		else
		{
			pkSummonItem->SetHorseBonusLevel(bRefineIndex - HORSE_REFINE_BONUS, bLevel + 1);
			pkSummonItem->SetHorseUsedBottles(bRefineIndex - HORSE_REFINE_BONUS, 0);

			RefreshBonusLevel(pkSummonItem);
		}
	}

	GetOwner()->GetDesc()->Packet(pack);
}
