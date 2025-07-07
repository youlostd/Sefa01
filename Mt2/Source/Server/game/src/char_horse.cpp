#include "stdafx.h"

#include "../../common/VnumHelper.h"

#include "char.h"
#include "mount_system.h"
#include "item_manager.h"
#include "item.h"
#include "skill.h"

bool CHARACTER::IsRiding() const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->IsRiding();
#endif

	return (m_pkMountSystem && m_pkMountSystem->IsRiding()) || GetMountVnum();
}

EVENTFUNC(horse_dead_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("horse_dead_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->OnHorseDeadEvent();
	return 0;
}

void CHARACTER::FinishMountLoading()
{
	if (m_pkMountSystem && m_bTempMountState != MOUNT_NONE)
	{
		LPITEM pkItem = ITEM_MANAGER::instance().Find(m_pkMountSystem->GetSummonItemID());
		if (!pkItem || pkItem->GetOwner() != this)
		{
			sys_err("LoadMountState: cannot find item by ID %d [player %d %s]", m_pkMountSystem->GetSummonItemID(), GetPlayerID(), GetName());
			m_pkMountSystem->SetSummonItemID(0);
		}
		else if (!m_pkMountSystem->IsNoSummonMap())
		{
			bool bSummon = m_pkMountSystem->Summon(pkItem);
			if (bSummon && m_bTempMountState == MOUNT_RIDING)
				m_pkMountSystem->StartRiding();
		}
	}

	m_bTempMountState = MOUNT_NONE;
}

void CHARACTER::SetHorseGrade(BYTE bGrade, bool bPacket)
{
	m_bHorseGrade = bGrade;

	SetSkillLevel(SKILL_HORSE, bGrade);
	if (bPacket)
		SkillLevelPacket();

	if (IsHorseSummoned() && GetMountSystem()->IsRiding())
		ChatPacket(CHAT_TYPE_COMMAND, "horse_state %d %d %d", GetHorseGrade(), GetHorseElapsedTime(), GetHorseMaxLifeTime());
}

void CHARACTER::SetHorseRageLevel(BYTE bLevel, bool bPacket)
{
	m_bHorseRageLevel = MINMAX(0, bLevel, HORSE_RAGE_MAX_LEVEL);
	if (bPacket)
		SendHorseRagePacket();
}

void CHARACTER::SetHorseRagePct(short sPct, bool bPacket)
{
	if (sPct > m_sHorseRagePct && IsHorseRage())
		return;

	m_sHorseRagePct = MINMAX(0, sPct, HORSE_MAX_RAGE);
	if (bPacket)
		SendHorseRagePacket();
}

EVENTFUNC(horse_rage_timeout_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("horse_rage_timeout_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->StopHorseRage();
	return 0;
}

EVENTFUNC(horse_rage_dec_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("horse_rage_dec_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->SetHorseRagePct(ch->GetHorseRagePct() - HORSE_DEC_RAGE_PER_TIME);
	return ch->GetNextHorseRageDecTime();
}

DWORD CHARACTER::GetNextHorseRageDecTime()
{
	if (!IsHorseRage())
		return 0;

	int iDecCountLeft = (int)ceilf((float)GetHorseRagePct() / (float)HORSE_DEC_RAGE_PER_TIME);
	if (iDecCountLeft == 0)
		return 0;

	DWORD dwTimeLeft = m_dwHorseRageTimeout - get_global_time();
	return (passes_per_sec * dwTimeLeft) / iDecCountLeft;
}

void CHARACTER::SetHorseRageTimeout(DWORD dwTimeout, bool bPacket)
{
	if (dwTimeout && get_global_time() >= dwTimeout)
		dwTimeout = 0;

	m_dwHorseRageTimeout = dwTimeout;
	event_cancel(&m_pkHorseRageTimeoutEvent);
	event_cancel(&m_pkHorseRageDecEvent);
	if (dwTimeout)
	{
		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;
		m_pkHorseRageTimeoutEvent = event_create(horse_rage_timeout_event, info, PASSES_PER_SEC(dwTimeout - get_global_time()));

		DWORD dwDecEventTime = GetNextHorseRageDecTime();
		if (dwDecEventTime)
		{
			info = AllocEventInfo<char_event_info>();
			info->ch = this;
			m_pkHorseRageDecEvent = event_create(horse_rage_dec_event, info, dwDecEventTime);
		}
	}

	if (bPacket)
		SendHorseRagePacket();
}

void CHARACTER::StartHorseRage()
{
	if (IsHorseRage())
	{
		sys_err("already in rage mode");
		return;
	}

	if (!IsHorseSummoned() || !IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only use your horse rage mode while riding your horse."));
		return;
	}

	if (GetHorseRagePct() < HORSE_MAX_RAGE)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your rage have to be full to use the rage mode of your horse."));
		return;
	}

	GetMountSystem()->GiveBuff(false);
	SetHorseRageTimeout(get_global_time() + adwHorseRageTime[GetHorseRageLevel()]);
	GetMountSystem()->GiveBuff(true);
}

void CHARACTER::StopHorseRage()
{
	if (!IsHorseRage())
	{
		sys_err("not in rage mode");
		return;
	}

	SetHorseRagePct(0, false);

	if (IsHorseSummoned() && IsRiding())
		GetMountSystem()->GiveBuff(false);
	SetHorseRageTimeout(0);
	if (IsHorseSummoned() && IsRiding())
		GetMountSystem()->GiveBuff(true);
}

void CHARACTER::SendHorseRagePacket()
{
	ChatPacket(CHAT_TYPE_COMMAND, "horse_rage %d %d %d", m_bHorseRageLevel, m_sHorseRagePct, IsHorseRage() ? m_dwHorseRageTimeout - get_global_time() : 0);
}

bool CHARACTER::UpgradeHorseGrade()
{
	if (GetHorseGrade() >= HORSE_MAX_GRADE)
		return false;

	BYTE bState = MOUNT_NONE;
	LPITEM pkItem = NULL;
	if (m_pkHorseDeadEvent)
	{
		bState = GetMountSystem()->GetState();
		pkItem = ITEM_MANAGER::instance().Find(GetMountSystem()->GetSummonItemID());
		if (GetMountSystem()->IsRiding())
			GetMountSystem()->StopRiding();
		if (GetMountSystem()->IsSummoned())
			GetMountSystem()->Unsummon();
	}

	int iPct = 0;
	if (GetHorseMaxLifeTime())
		iPct = GetHorseElapsedTime() * 10000 / GetHorseMaxLifeTime();
	SetHorseGrade(GetHorseGrade() + 1);
	SetHorseElapsedTime(GetHorseMaxLifeTime() * iPct / 10000);

	if (GetHorseGrade() == HORSE_MAX_GRADE )
		SetHorsesHoeTimeout(HORSES_HOE_TIMEOUT_TIME);

	if (bState > MOUNT_NONE)
	{
		if (pkItem)
		{
			GetMountSystem()->Summon(pkItem);
			if (bState == MOUNT_RIDING)
				GetMountSystem()->StartRiding();
		}
	}

	return true;
}

void CHARACTER::SetHorsesHoeTimeout(DWORD dwHorsesHoeTimeout, bool bAddGlobalTime)
{
	sys_log(0, "SetHorsesHoeTimeout[%s] %u timeout %u addGlobal %d", GetName(), GetPlayerID(), dwHorsesHoeTimeout, bAddGlobalTime);

	if (!dwHorsesHoeTimeout)
		m_dwHorsesHoeTimeout = 0;
	else
		m_dwHorsesHoeTimeout = bAddGlobalTime ? get_global_time() + dwHorsesHoeTimeout : dwHorsesHoeTimeout;

	StartHorsesHoeTimeoutEvent();
}

EVENTFUNC(horse_hoe_timeout_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("horse_hoe_timeout_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}
	
	DWORD dwTimeout = ch->GetHorsesHoeTimeout();
	if (dwTimeout > get_global_time())
	{
		DWORD dwTimeLeft = dwTimeout - get_global_time();
		sys_err("INVALID timeout[%s] event call -restart with time left %u (timeout %u globalTime %u)", ch->GetName(), dwTimeLeft, dwTimeout, get_global_time());
		return PASSES_PER_SEC(dwTimeLeft);
	}

	ch->OnHorsesHoeTimeout();
	return 0;
}

void CHARACTER::StartHorsesHoeTimeoutEvent()
{

	event_cancel(&m_pkHorsesHoeTimeoutEvent);

	if (!NeedHorsesHoe())
		return;

	if (!m_dwHorsesHoeTimeout || get_global_time() >= m_dwHorsesHoeTimeout)
	{
		sys_log(0, "StartHorsesHoeTimeout[%s] => timeout (timeout %u globalTime %u)", GetName(), m_dwHorsesHoeTimeout, get_global_time());
		OnHorsesHoeTimeout();
	}
	else
	{
		sys_log(0, "StartHorsesHoeTimeout[%s] => time left %u (timeout %u globalTime %u)", GetName(), m_dwHorsesHoeTimeout - get_global_time(), m_dwHorsesHoeTimeout, get_global_time());
		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;
		m_pkHorsesHoeTimeoutEvent = event_create(horse_hoe_timeout_event, info, PASSES_PER_SEC(m_dwHorsesHoeTimeout - get_global_time()));
	}
}

void CHARACTER::OnHorsesHoeTimeout()
{
	sys_log(0, "OnHOrsesHoeTimeout[%s] (timeout %u globalTime %u)", GetName(), m_dwHorsesHoeTimeout, get_global_time());

	m_pkHorsesHoeTimeoutEvent = NULL;
	bool bMessage = false;
	if (m_dwHorsesHoeTimeout)
	{
		m_dwHorsesHoeTimeout = 0;
		bMessage = true;
	}

	if (IsHorseSummoned())
	{
		if (IsRiding())
			GetMountSystem()->StopRiding();
		else
			GetMountSystem()->Unsummon();
	}

	if (bMessage)
		ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(this, "You have to give a horsehoe to the Stable Boy for using your king horse."));
}

bool CHARACTER::NeedHorsesHoe() const
{
	return false; // deactivated

	if (GetHorseGrade() < HORSE_MAX_GRADE)
		return false;

	return true;
}

bool CHARACTER::IsHorsesHoeTimeout() const
{
	if (!NeedHorsesHoe())
		return false;

	return m_dwHorsesHoeTimeout == 0;
}

void CHARACTER::SetHorseName(const std::string& c_rstName)
{
	if (m_stHorseName == c_rstName)
		return;

	m_stHorseName = c_rstName;

	GetMountSystem()->SetName(m_stHorseName.c_str());
	if (GetMountSystem()->IsSummoned())
	{
		GetMountSystem()->GetMount()->SetName(m_stHorseName);
		GetMountSystem()->GetMount()->ViewReencode();
	}
}

bool CHARACTER::IsHorseDead() const
{
	return GetHorseElapsedTime() >= GetHorseMaxLifeTime();
}

void CHARACTER::SetHorseElapsedTime(DWORD dwTime)
{
	bool bEventRunning = m_pkHorseDeadEvent != NULL;
	if (bEventRunning)
		event_cancel(&m_pkHorseDeadEvent);

	if (dwTime > GetHorseMaxLifeTime())
		dwTime = GetHorseMaxLifeTime();
	m_dwHorseElapsedLifeTime = dwTime;
	if (bEventRunning)
		StartHorseDeadEvent();

	if (IsHorseSummoned())
		ChatPacket(CHAT_TYPE_COMMAND, "horse_state %d %d %d", GetHorseGrade(), GetHorseElapsedTime(), GetHorseMaxLifeTime());
}

DWORD CHARACTER::GetHorseElapsedTime() const
{
	DWORD dwTime = m_dwHorseElapsedLifeTime;
	if (m_pkHorseDeadEvent)
		dwTime += event_processing_time(m_pkHorseDeadEvent) / passes_per_sec;
	return MIN(GetHorseMaxLifeTime(), dwTime);
}

DWORD CHARACTER::GetHorseMaxLifeTime() const
{
	static const DWORD sc_adwMaxLifeTime[HORSE_MAX_GRADE + 1] = { 0, 60*60*24*3, 60*60*24*2+60*60*12, 60*60*24*2 };
	return sc_adwMaxLifeTime[GetHorseGrade()];
}

void CHARACTER::StartHorseDeadEvent()
{
	if (m_pkHorseDeadEvent)
		event_cancel(&m_pkHorseDeadEvent);

	int iTimeLeft = (int)GetHorseMaxLifeTime() - (int)GetHorseElapsedTime();
	if (iTimeLeft <= 0)
		iTimeLeft = 1;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkHorseDeadEvent = event_create(horse_dead_event, info, PASSES_PER_SEC(iTimeLeft));
}

void CHARACTER::StopHorseDeadEvent()
{
	SetHorseElapsedTime(GetHorseElapsedTime());
	event_cancel(&m_pkHorseDeadEvent);
}

void CHARACTER::OnHorseDeadEvent()
{
	m_pkHorseDeadEvent = NULL;

	SetHorseElapsedTime(GetHorseMaxLifeTime());
	if (GetMountSystem()->IsRiding())
		GetMountSystem()->StopRiding();
	if (GetMountSystem()->IsSummoned())
		GetMountSystem()->Unsummon();

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your horse died. Revive it with a revival item."));
}

bool CHARACTER::HorseRevive()
{
	if (!IsHorseDead())
	{
		sys_err("cannot revive non dead horse");
		return false;
	}

	SetHorseElapsedTime(0);

	return true;
}

bool CHARACTER::HorseFeed(int iPct, int iRagePct)
{
	if ((GetHorseElapsedTime() == 0 || !iPct) && (GetHorseRagePct() >= HORSE_MAX_RAGE || !iRagePct))
	{
		if (iPct)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your horse is not hungry."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The rage of your horse is already full."));
		return false;
	}

	if (iRagePct && GetHorseRagePct() < HORSE_MAX_RAGE)
	{
		int iRageNew = GetHorseRagePct() + iRagePct;
		iRageNew = MIN(iRageNew, HORSE_MAX_RAGE);

		SetHorseRagePct(iRageNew);
	}

	int iChange = GetHorseMaxLifeTime() * iPct / 100;
	if (iChange > GetHorseElapsedTime())
		SetHorseElapsedTime(0);
	else
		SetHorseElapsedTime(GetHorseElapsedTime() - iChange);

	return true;
}

bool CHARACTER::CanUseHorseSkill() const
{
	if (GetHorseGrade() < 2)
		return false;

	if (GetMountVnum() == 0)
		return false;

	return true;
}

