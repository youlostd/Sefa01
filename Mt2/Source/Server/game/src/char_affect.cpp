
#include "stdafx.h"

#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "affect.h"
#include "packet.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "battle.h"
#include "guild.h"
#include "utils.h"
#include "lua_incl.h"
#include "arena.h"
#include "item.h"
#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif
#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_RUNE_AFFECT_ICONS
#define IS_NO_SAVE_AFFECT(type) ((type) == AFFECT_WAR_FLAG || (type) == AFFECT_REVIVE_INVISIBLE || ((type) >= AFFECT_PREMIUM_START && (type) <= AFFECT_PREMIUM_END) || ((type) >= AFFECT_TEMPORARY_START && (type) <= AFFECT_TEMPORARY_END) || ((type) >= AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_1 && (type) <= AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_51))
#else
#define IS_NO_SAVE_AFFECT(type) ((type) == AFFECT_WAR_FLAG || (type) == AFFECT_REVIVE_INVISIBLE || ((type) >= AFFECT_PREMIUM_START && (type) <= AFFECT_PREMIUM_END) || ((type) >= AFFECT_TEMPORARY_START && (type) <= AFFECT_TEMPORARY_END))
#endif
#define IS_NO_CLEAR_ON_DEATH_AFFECT(type) ((type) == AFFECT_BLOCK_CHAT || ((type) >= 500 && (type) < 600) || ((type) >= AFFECT_TEMPORARY_START && (type) <= AFFECT_TEMPORARY_END) || (type >= AFFECT_BIOLOGIST_START && type <= AFFECT_BIOLOGIST_END) || (type >= AFFECT_EVENT_START && type <= AFFECT_EVENT_END))

void SendAffectRemovePacket(LPDESC d, DWORD pid, DWORD type, BYTE point, DWORD flag)
{
	network::GCOutputPacket<network::GCAffectRemovePacket> ptoc;
	ptoc->set_type(type);
	ptoc->set_apply_on(point);
	ptoc->set_flag(flag);
	d->Packet(ptoc);

	network::GDOutputPacket<network::GDRemoveAffectPacket> ptod;
	ptod->set_pid(pid);
	ptod->set_type(type);
	ptod->set_apply_on(point);
	db_clientdesc->DBPacket(ptod);
}

void SendAffectAddPacket(LPDESC d, CAffect * pkAff)
{
	network::GCOutputPacket<network::GCAffectAddPacket> ptoc;
	auto elem = ptoc->mutable_elem();
	elem->set_type(pkAff->dwType);
	elem->set_apply_on(pkAff->bApplyOn);
	elem->set_apply_value(pkAff->lApplyValue);
	elem->set_flag(pkAff->dwFlag);
	elem->set_duration(pkAff->lDuration);
	elem->set_sp_cost(pkAff->lSPCost);
	d->Packet(ptoc);
}
////////////////////////////////////////////////////////////////////
// Affect
CAffect * CHARACTER::FindAffect(DWORD dwType, BYTE bApply) const
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAffect = *it++;

		if (pkAffect->dwType == dwType && (bApply == APPLY_NONE || bApply == pkAffect->bApplyOn))
			return pkAffect;
	}

	return NULL;
}

EVENTFUNC(affect_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "affect_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->UpdateAffect())
		return 0;
	else
		return passes_per_sec; // 1ÃÊ
}

bool CHARACTER::UpdateAffect()
{
	// affect_event ¿¡¼­ Ã³¸®ÇÒ ÀÏÀº ¾Æ´ÏÁö¸¸, 1ÃÊÂ¥¸® ÀÌº¥Æ®¿¡¼­ Ã³¸®ÇÏ´Â °ÍÀÌ
	// ÀÌ°Í »ÓÀÌ¶ó ¿©±â¼­ ¹°¾à Ã³¸®¸¦ ÇÑ´Ù.
#ifdef __FAKE_PC__
	if (!FakePC_Check() && GetPoint(POINT_HP_RECOVERY) > 0)
#else
	if (GetPoint(POINT_HP_RECOVERY) > 0)
#endif
	{
		if (GetMaxHP() <= GetHP())
		{
			PointChange(POINT_HP_RECOVERY, -GetPoint(POINT_HP_RECOVERY));
		}
		else
		{
			int iMaxVal = GetMaxHP();
			int iVal = MIN(GetPoint(POINT_HP_RECOVERY), MIN(iMaxVal, (int)(iMaxVal * 7.0f / 100.0f * (100 + GetPoint(POINT_HEAL_EFFECT_BONUS)) / 100.0f + 0.5f)));

#ifdef __FAKE_PC__
			if (FakePC_IsSupporter() && iVal < iMaxVal)
			{
				PointChange(POINT_HP_RECOVERY, 1200 - iVal + iMaxVal); // using big potions (27003)
				iVal = iMaxVal;

				EffectPacket(SE_HPUP_RED);
			}
#endif

			PointChange(POINT_HP, iVal);
			PointChange(POINT_HP_RECOVERY, -iVal);
		}
	}

#ifdef __FAKE_PC__
	if (GetPoint(POINT_SP_RECOVERY) > 0 || FakePC_Check())
#else
	if (GetPoint(POINT_SP_RECOVERY) > 0)
#endif
	{
		if (GetMaxSP() <= GetSP())
			PointChange(POINT_SP_RECOVERY, -GetPoint(POINT_SP_RECOVERY));
		else 
		{
			int iMaxVal = GetMaxSP() * 7 / 100;
			int iVal = MIN(GetPoint(POINT_SP_RECOVERY), iMaxVal);

#ifdef __FAKE_PC__
			if (FakePC_Check() && iVal < iMaxVal)
			{
				PointChange(POINT_SP_RECOVERY, 400 - iVal + iMaxVal); // using big potions (27006)
				iVal = iMaxVal;

				EffectPacket(SE_SPUP_BLUE);
			}
#endif

			PointChange(POINT_SP, iVal);
			PointChange(POINT_SP_RECOVERY, -iVal);
		}
	}

	if (GetPoint(POINT_HP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_HP, GetPoint(POINT_HP_RECOVER_CONTINUE));
	}

	if (GetPoint(POINT_SP_RECOVER_CONTINUE) > 0)
	{
		PointChange(POINT_SP, GetPoint(POINT_SP_RECOVER_CONTINUE));
	}

	AutoRecoveryItemProcess( AFFECT_AUTO_HP_RECOVERY );
	AutoRecoveryItemProcess( AFFECT_AUTO_SP_RECOVERY );

	// ½ºÅ×¹Ì³ª È¸º¹
	if (GetMaxStamina() > GetStamina())
	{
		int iSec = (get_dword_time() - GetStopTime()) / 3000;
		if (iSec)
			PointChange(POINT_STAMINA, GetMaxStamina()/1);	
	}


	// ProcessAffect´Â affect°¡ ¾øÀ¸¸é true¸¦ ¸®ÅÏÇÑ´Ù.
#ifdef __FAKE_PC__
	if (ProcessAffect() && !FakePC_Check())
#else
	if (ProcessAffect())
#endif
		if (GetPoint(POINT_HP_RECOVERY) == 0 && GetPoint(POINT_SP_RECOVERY) == 0 && GetStamina() == GetMaxStamina())
		{
			m_pkAffectEvent = NULL;
			return false;
		}

	return true;
}

void CHARACTER::StartAffectEvent()
{
	if (m_pkAffectEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkAffectEvent = event_create(affect_event, info, passes_per_sec);
	sys_log(1, "StartAffectEvent %s %p %p", GetName(), this, get_pointer(m_pkAffectEvent));
}

#ifdef SKILL_AFFECT_DEATH_REMAIN
void CHARACTER::ClearAffect(bool bSave, bool isExceptGood)
#else
void CHARACTER::ClearAffect(bool bSave)
#endif
{
	TAffectFlag afOld = m_afAffectFlag;
	WORD	wMovSpd = GetPoint(POINT_MOV_SPEED);
	WORD	wAttSpd = GetPoint(POINT_ATT_SPEED);

	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it;

#ifdef SKILL_AFFECT_DEATH_REMAIN
		if (isExceptGood && IsGoodAffectSkill(pkAff->dwType)) // 12 noiembrie 2018
		{
			++it;
			continue;
		}
#endif

		if (bSave)
		{
			if ( IS_NO_CLEAR_ON_DEATH_AFFECT(pkAff->dwType) || IS_NO_SAVE_AFFECT(pkAff->dwType) )
			{
				++it;
				continue;
			}

			if (IsPC())
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn, pkAff->dwFlag);
		}

		ComputeAffect(pkAff, false);

		it = m_list_pkAffect.erase(it);
#ifdef __FAKE_PC__
		if (FakePC_Check())
		{
			itertype(m_map_pkFakePCAffects) it = m_map_pkFakePCAffects.find(pkAff);
			if (it != m_map_pkFakePCAffects.end())
			{
				m_map_pkFakePCAffects.erase(it->second);
				m_map_pkFakePCAffects.erase(it);
			}
		}
#endif

		CAffect::Release(pkAff);
	}

	if (afOld != m_afAffectFlag ||
			wMovSpd != GetPoint(POINT_MOV_SPEED) ||
			wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	CheckMaximumPoints();
#ifdef __FAKE_PC__
	FakePC_Owner_ExecFunc(&CHARACTER::CheckMaximumPoints);
#endif

	if (m_list_pkAffect.empty())
		event_cancel(&m_pkAffectEvent);
}

int CHARACTER::ProcessAffect()
{
	bool	bDiff	= false;
	bool bNeedRestore = false;
	CAffect	*pkAff	= NULL;

	//
	// ÇÁ¸®¹Ì¾ö Ã³¸®
	//
	for (int i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int aff_idx = i + AFFECT_PREMIUM_START;

		pkAff = FindAffect(aff_idx);

		if (!pkAff)
			continue;

		int remain = GetPremiumRemainSeconds(i);

		if (remain < 0)
		{
			RemoveAffect(aff_idx);
			bDiff = true;
		}
		else
			pkAff->lDuration = remain + 1;
	}

	TAffectFlag afOld = m_afAffectFlag;
	long lMovSpd = GetPoint(POINT_MOV_SPEED);
	long lAttSpd = GetPoint(POINT_ATT_SPEED);

	itertype(m_list_pkAffect) it;

	it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		pkAff = *it;

		bool bEnd = false;

		if (pkAff->dwType >= GUILD_SKILL_START && pkAff->dwType <= GUILD_SKILL_END)
		{
			if (!GetGuild() || !GetGuild()->UnderAnyWar())
				bEnd = true;
		}

		if (pkAff->lSPCost > 0)
		{
			if (GetSP() < pkAff->lSPCost)
				bEnd = true;
			else
				PointChange(POINT_SP, -pkAff->lSPCost);
		}

		// AFFECT_DURATION_BUG_FIX
		// ¹«ÇÑ È¿°ú ¾ÆÀÌÅÛµµ ½Ã°£À» ÁÙÀÎ´Ù.
		// ½Ã°£À» ¸Å¿ì Å©°Ô Àâ±â ¶§¹®¿¡ »ó°ü ¾øÀ» °ÍÀÌ¶ó »ý°¢µÊ.
		if ( --pkAff->lDuration <= 0 )
		{
			bEnd = true;
		}
		// END_AFFECT_DURATION_BUG_FIX

		if (bEnd)
		{
			it = m_list_pkAffect.erase(it);
#ifdef __FAKE_PC__
			if (FakePC_Check())
			{
				itertype(m_map_pkFakePCAffects) it = m_map_pkFakePCAffects.find(pkAff);
				if (it != m_map_pkFakePCAffects.end())
				{
					m_map_pkFakePCAffects.erase(it->second);
					m_map_pkFakePCAffects.erase(it);
				}
			}
#endif
			ComputeAffect(pkAff, false);
			bDiff = true;
			if (IsPC())
			{
				SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn, pkAff->dwFlag);
			}

			if (pkAff->dwFlag == AFF_PABEOP)
				bNeedRestore = true;

			CAffect::Release(pkAff);

			continue;
		}

		++it;
	}

	if (bNeedRestore)
		RestoreSavedAffects();

	if (bDiff)
	{
		if (afOld != m_afAffectFlag ||
				lMovSpd != GetPoint(POINT_MOV_SPEED) ||
				lAttSpd != GetPoint(POINT_ATT_SPEED))
		{
			UpdatePacket();
		}

		CheckMaximumPoints();
	}

	if (m_list_pkAffect.empty())
		return true;

	return false;
}

void CHARACTER::SaveAffect()
{
	network::GDOutputPacket<network::GDAddAffectPacket> p;

	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it++;

		if (IS_NO_SAVE_AFFECT(pkAff->dwType))
			continue;

		sys_log(1, "AFFECT_SAVE: %u %u %d %d", pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration);

		auto elem = p->mutable_elem();
		p->set_pid(GetPlayerID());
		elem->set_type(pkAff->dwType);
		elem->set_apply_on(pkAff->bApplyOn);
		elem->set_apply_value(pkAff->lApplyValue);
		elem->set_flag(pkAff->dwFlag);
		elem->set_duration(pkAff->lDuration);
		elem->set_sp_cost(pkAff->lSPCost);
		db_clientdesc->DBPacket(p);
	}
}

EVENTINFO(load_affect_login_event_info)
{
	DWORD pid;
	::google::protobuf::RepeatedPtrField<TPacketAffectElement> affects;

	load_affect_login_event_info()
	: pid( 0 )
	{
	}
};

EVENTFUNC(load_affect_login_event)
{
	load_affect_login_event_info* info = dynamic_cast<load_affect_login_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "load_affect_login_event_info> <Factor> Null pointer" );
		return 0;
	}

	DWORD dwPID = info->pid;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

	if (!ch)
	{
		return 0;
	}

	LPDESC d = ch->GetDesc();

	if (!d)
	{
		return 0;
	}

	if (d->IsPhase(PHASE_HANDSHAKE) ||
			d->IsPhase(PHASE_LOGIN) ||
			d->IsPhase(PHASE_SELECT) ||
			d->IsPhase(PHASE_DEAD) ||
			d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		sys_log(1, "Affect Load by Event");
		ch->LoadAffect(info->affects);
		return 0;
	}
	else
	{
		sys_err("input_db.cpp:quest_login_event INVALID PHASE pid %d", ch->GetPlayerID());
		return 0;
	}
}

void CHARACTER::LoadAffect(const ::google::protobuf::RepeatedPtrField<TPacketAffectElement>& elements)
{
	m_bIsLoadedAffect = false;

	if (!GetDesc()->IsPhase(PHASE_GAME))
	{
		if (test_server)
			sys_log(0, "LOAD_AFFECT: Creating Event", GetName(), elements.size());

		load_affect_login_event_info* info = AllocEventInfo<load_affect_login_event_info>();

		info->pid = GetPlayerID();
		info->affects = elements;
		
		event_create(load_affect_login_event, info, PASSES_PER_SEC(1));

		return;
	}
	
	// Fixed double affects, it was true, but why..
	ClearAffect(false);

	if (IsGMInvisible())
		AddAffect(AFFECT_REVIVE_INVISIBLE, POINT_NONE, 0, AFF_REVIVE_INVISIBLE, INFINITE_AFFECT_DURATION, 0, true);

	if (test_server)
		sys_log(0, "LOAD_AFFECT: %s count %d", GetName(), elements.size());

	TAffectFlag afOld = m_afAffectFlag;

	long lMovSpd = GetPoint(POINT_MOV_SPEED);
	long lAttSpd = GetPoint(POINT_ATT_SPEED);

	for (auto& aff : elements)
	{
		if (aff.type() == SKILL_MUYEONG)
			continue;

		if (aff.duration() == 0)
			continue;

		if (AFFECT_AUTO_HP_RECOVERY == aff.type() || AFFECT_AUTO_SP_RECOVERY == aff.type())
		{
			LPITEM item = FindItemByID( aff.flag() );

			if (NULL == item)
				continue;

#ifdef __EVENT_MANAGER__
			if (!CEventManager::instance().CanUseItem(this, item))
			{
				item->SetSocket(0, 0);
				continue;
			}
#endif

			item->Lock(true);
		}

		if (aff.apply_on() >= POINT_MAX_NUM)
		{
			sys_err("invalid affect data %s ApplyOn %u ApplyValue %d",
					GetName(), aff.apply_on(), aff.apply_value());
			continue;
		}

		if (test_server)
		{
			sys_log(0, "Load Affect : Affect %s %d %d", GetName(), aff.type(), aff.apply_on());
		}

		CAffect* pkAff = CAffect::Acquire();
		m_list_pkAffect.push_back(pkAff);

		pkAff->dwType		= aff.type();
		pkAff->bApplyOn		= aff.apply_on();
		pkAff->lApplyValue	= aff.apply_value();
		pkAff->dwFlag		= aff.flag();
		pkAff->lDuration	= aff.duration();
		pkAff->lSPCost		= aff.sp_cost();

		SendAffectAddPacket(GetDesc(), pkAff);

		ComputeAffect(pkAff, true);
	}

	if ( CArenaManager::instance().IsArenaMap(GetMapIndex()) == true )
	{
		RemoveGoodAffect();
	}
	
	if (GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
	{
		RemoveGoodAffect();
	}
	
	if (afOld != m_afAffectFlag || lMovSpd != GetPoint(POINT_MOV_SPEED) || lAttSpd != GetPoint(POINT_ATT_SPEED))
	{
		UpdatePacket();
	}

	StartAffectEvent();

	m_bIsLoadedAffect = true;

#ifdef __VOTE4BUFF__
	V4B_AddAffect();
#endif

#ifdef __DRAGONSOUL__
	DragonSoul_Initialize();
#endif

#ifdef __EVENT_MANAGER__
	CEventManager::instance().OnPlayerLoadAffect(this);
#endif

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::instance().IsCombatZoneMap(GetMapIndex()))
		RemoveGoodAffect();
#endif

	CheckForDisabledItems();
}

bool CHARACTER::AddAffect(DWORD dwType, BYTE bApplyOn, long lApplyValue, DWORD dwFlag, long lDuration, long lSPCost, bool bOverride, bool IsCube )
{
	//tchat("AddAffect (dwType: %d, bApplyOn: %d, lApplyValue: %d, lDuration: %d", dwType, bApplyOn, lApplyValue, lDuration);

	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT && lDuration > 1)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿î¿µÀÚ Á¦Á¦·Î Ã¤ÆÃÀÌ ±ÝÁö µÇ¾ú½À´Ï´Ù."));
	}
	// END_OF_CHAT_BLOCK

	if (lDuration == 0)
	{
		sys_err("Character::AddAffect lDuration == 0 type %d", lDuration, dwType);
		lDuration = 1;
	}
	
#if defined(__IGNORE_LOWER_BUFFS__)
	switch (dwType)
	{
		case SKILL_HOSIN:
		case SKILL_REFLECT:
		case SKILL_GICHEON:
		case SKILL_JEONGEOP:
		case SKILL_KWAESOK:
		case SKILL_JEUNGRYEOK:
#if defined(__WOLFMAN__)
		case SKILL_CHEONGRANG:
#endif
		{
			const CAffect * pkAffect = FindAffect(dwType);
			if (!pkAffect)
				break;
				
			if (lApplyValue < pkAffect->lApplyValue)
			{
				ChatPacket(CHAT_TYPE_INFO, "<AddAffect> has blocked receiving skill (%s) because power is (%ld%%) more small then current one (%ld%%).", CSkillManager::instance().Get(dwType)->szName, lApplyValue, pkAffect->lApplyValue);
				return false;
			}
		}
		break;
		default:
			break;
	}
#endif
	CAffect * pkAff = NULL;

	if (IsCube)
		pkAff = FindAffect(dwType,bApplyOn);
	else
		pkAff = FindAffect(dwType);

	if (dwFlag == AFF_STUN)
	{
		if (m_posDest.x != GetX() || m_posDest.y != GetY())
		{
			m_posDest.x = m_posStart.x = GetX();
			m_posDest.y = m_posStart.y = GetY();
			battle_end(this);

			SyncPacket();
		}
	}

	// ÀÌ¹Ì ÀÖ´Â È¿°ú¸¦ µ¤¾î ¾²´Â Ã³¸®
	if (pkAff && bOverride)
	{
		ComputeAffect(pkAff, false); // ÀÏ´Ü È¿°ú¸¦ »èÁ¦ÇÏ°í

		if (GetDesc())
			SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn, pkAff->dwFlag);
	}
	else
	{
		//
		// »õ ¿¡Æå¸¦ Ãß°¡
		//
		// NOTE: µû¶ó¼­ °°Àº type À¸·Îµµ ¿©·¯ ¿¡ÆåÆ®¸¦ ºÙÀ» ¼ö ÀÖ´Ù.
		// 
		pkAff = CAffect::Acquire();
		m_list_pkAffect.push_back(pkAff);

	}

	if (IsPC())
		sys_log(0, "AddAffect %s type %d apply %d %d flag %u duration %d", GetName(), dwType, bApplyOn, lApplyValue, dwFlag, lDuration);

	pkAff->dwType	= dwType;
	pkAff->bApplyOn	= bApplyOn;
	pkAff->lApplyValue	= lApplyValue;
	pkAff->dwFlag	= dwFlag;
	pkAff->lDuration	= lDuration;
	pkAff->lSPCost	= lSPCost;

	WORD wMovSpd = GetPoint(POINT_MOV_SPEED);
	WORD wAttSpd = GetPoint(POINT_ATT_SPEED);

	ComputeAffect(pkAff, true);

	if (pkAff->dwFlag || wMovSpd != GetPoint(POINT_MOV_SPEED) || wAttSpd != GetPoint(POINT_ATT_SPEED))
		UpdatePacket();

	StartAffectEvent();

	if (IsPC())
	{
#ifdef __FAKE_PC__
		FakePC_Owner_AddAffect(pkAff);
#endif

		SendAffectAddPacket(GetDesc(), pkAff);

		if (IS_NO_SAVE_AFFECT(pkAff->dwType))
			return true;

		network::GDOutputPacket<network::GDAddAffectPacket> p;
		p->set_pid(GetPlayerID());
		
		auto elem = p->mutable_elem();
		elem->set_type(pkAff->dwType);
		elem->set_apply_on(pkAff->bApplyOn);
		elem->set_apply_value(pkAff->lApplyValue);
		elem->set_flag(pkAff->dwFlag);
		elem->set_duration(pkAff->lDuration);
		elem->set_sp_cost(pkAff->lSPCost);

		db_clientdesc->DBPacket(p);
	}

	return true;
}

void CHARACTER::RefreshAffect()
{
	itertype(m_list_pkAffect) it = m_list_pkAffect.begin();

	while (it != m_list_pkAffect.end())
	{
		CAffect * pkAff = *it++;
		ComputeAffect(pkAff, true);
	}
}

void CHARACTER::ComputeAffect(CAffect * pkAff, bool bAdd)
{
	if (bAdd && pkAff->dwType >= GUILD_SKILL_START && pkAff->dwType <= GUILD_SKILL_END)	
	{
		if (!GetGuild())
			return;

		if (!GetGuild()->UnderAnyWar())
			return;
	}

	if (pkAff->dwFlag)
	{
		if (!bAdd)
			m_afAffectFlag.Reset(pkAff->dwFlag);
		else
			m_afAffectFlag.Set(pkAff->dwFlag);
	}

	if (bAdd)
		PointChange(pkAff->bApplyOn, pkAff->lApplyValue);
	else
		PointChange(pkAff->bApplyOn, -pkAff->lApplyValue);

	switch (pkAff->bApplyOn)
	{
	case POINT_ATT_GRADE_BONUS:
	case POINT_DEF_GRADE_BONUS:
	case POINT_MAGIC_ATT_GRADE_BONUS:
	case POINT_MAGIC_DEF_GRADE_BONUS:
	case POINT_DEF_GRADE:
		ComputeBattlePoints();
		PointChange(POINT_ATT_GRADE, 0);
		PointChange(POINT_DEF_GRADE, 0);
		PointChange(POINT_CLIENT_DEF_GRADE, 0);
		PointChange(POINT_MAGIC_ATT_GRADE, 0);
		PointChange(POINT_MAGIC_DEF_GRADE, 0);
		break;
	}

	if (pkAff->dwType == SKILL_MUYEONG)
	{
		if (bAdd)
			StartMuyeongEvent();
		else
			StopMuyeongEvent();
	}
}

void CHARACTER::RestoreSavedAffects()
{
	for (auto restoreIT : m_list_pkAffectSave)
	{
		m_list_pkAffect.push_back(restoreIT);
		ComputeAffect(restoreIT, true);

		if (IsPC())
		{
#ifdef __FAKE_PC__
			FakePC_Owner_AddAffect(restoreIT);
#endif

			SendAffectAddPacket(GetDesc(), restoreIT);

			if (!IS_NO_SAVE_AFFECT(restoreIT->dwType))
			{
				network::GDOutputPacket<network::GDAddAffectPacket> p;
				p->set_pid(GetPlayerID());

				auto elem = p->mutable_elem();
				elem->set_type(restoreIT->dwType);
				elem->set_apply_on(restoreIT->bApplyOn);
				elem->set_apply_value(restoreIT->lApplyValue);
				elem->set_flag(restoreIT->dwFlag);
				elem->set_duration(restoreIT->lDuration);
				elem->set_sp_cost(restoreIT->lSPCost);

				db_clientdesc->DBPacket(p);
			}
		}
	}
	m_list_pkAffectSave.clear();
}

bool CHARACTER::RemoveAffect(CAffect * pkAff, bool useCompute, bool onlySave)
{
	if (!pkAff)
		return false;

	// AFFECT_BUF_FIX
	m_list_pkAffect.remove(pkAff);
	// END_OF_AFFECT_BUF_FIX

#ifdef __FAKE_PC__
	if (FakePC_Check())
	{
		itertype(m_map_pkFakePCAffects) it = m_map_pkFakePCAffects.find(pkAff);
		if (it != m_map_pkFakePCAffects.end())
		{
			m_map_pkFakePCAffects.erase(it->second);
			m_map_pkFakePCAffects.erase(it);
		}
	}
#endif

	ComputeAffect(pkAff, false);

	// ¹é±â ¹ö±× ¼öÁ¤.
	// ¹é±â ¹ö±×´Â ¹öÇÁ ½ºÅ³ ½ÃÀü->µÐ°©->¹é±â »ç¿ë(AFFECT_REVIVE_INVISIBLE) ÈÄ ¹Ù·Î °ø°Ý ÇÒ °æ¿ì¿¡ ¹ß»ýÇÑ´Ù.
	// ¿øÀÎÀº µÐ°©À» ½ÃÀüÇÏ´Â ½ÃÁ¡¿¡, ¹öÇÁ ½ºÅ³ È¿°ú¸¦ ¹«½ÃÇÏ°í µÐ°© È¿°ú¸¸ Àû¿ëµÇ°Ô µÇ¾îÀÖ´Âµ¥,
	// ¹é±â »ç¿ë ÈÄ ¹Ù·Î °ø°ÝÇÏ¸é RemoveAffect°¡ ºÒ¸®°Ô µÇ°í, ComputePointsÇÏ¸é¼­ µÐ°© È¿°ú + ¹öÇÁ ½ºÅ³ È¿°ú°¡ µÈ´Ù.
	// ComputePoints¿¡¼­ µÐ°© »óÅÂ¸é ¹öÇÁ ½ºÅ³ È¿°ú ¾È ¸ÔÈ÷µµ·Ï ÇÏ¸é µÇ±ä ÇÏ´Âµ¥,
	// ComputePoints´Â ±¤¹üÀ§ÇÏ°Ô »ç¿ëµÇ°í ÀÖ¾î¼­ Å« º¯È­¸¦ ÁÖ´Â °ÍÀÌ ²¨·ÁÁø´Ù.(¾î¶² side effect°¡ ¹ß»ýÇÒÁö ¾Ë±â Èûµé´Ù.)
	// µû¶ó¼­ AFFECT_REVIVE_INVISIBLE°¡ RemoveAffect·Î »èÁ¦µÇ´Â °æ¿ì¸¸ ¼öÁ¤ÇÑ´Ù.
	// ½Ã°£ÀÌ ´Ù µÇ¾î ¹é±â È¿°ú°¡ Ç®¸®´Â °æ¿ì´Â ¹ö±×°¡ ¹ß»ýÇÏÁö ¾ÊÀ¸¹Ç·Î ±×¿Í ¶È°°ÀÌ ÇÔ.
	//		(ProcessAffect¸¦ º¸¸é ½Ã°£ÀÌ ´Ù µÇ¾î¼­ Affect°¡ »èÁ¦µÇ´Â °æ¿ì, ComputePoints¸¦ ºÎ¸£Áö ¾Ê´Â´Ù.)
	if (useCompute)
	{
		if (AFFECT_REVIVE_INVISIBLE != pkAff->dwType)
			ComputePoints();
		else
			UpdatePacket();
	}
	CheckMaximumPoints();

	if (test_server)
		sys_log(0, "AFFECT_REMOVE: %s %u (flag %u apply: %u)", GetName(), pkAff->dwType, pkAff->dwFlag, pkAff->bApplyOn);

	if (IsPC())
	{
#ifdef __FAKE_PC__
		FakePC_Owner_RemoveAffect(pkAff);
#endif

		SendAffectRemovePacket(GetDesc(), GetPlayerID(), pkAff->dwType, pkAff->bApplyOn, pkAff->dwFlag);
	}

	if (pkAff->dwFlag == AFF_PABEOP)
	{
		TAffectFlag afOld = m_afAffectFlag;
		long lMovSpd = GetPoint(POINT_MOV_SPEED);
		long lAttSpd = GetPoint(POINT_ATT_SPEED);
		RestoreSavedAffects();

		if (afOld != m_afAffectFlag ||
			lMovSpd != GetPoint(POINT_MOV_SPEED) ||
			lAttSpd != GetPoint(POINT_ATT_SPEED))
		{
			UpdatePacket();
		}

		CheckMaximumPoints();
	}

	if (onlySave)
		m_list_pkAffectSave.push_back(pkAff);
	else
		CAffect::Release(pkAff);

	return true;
}

bool CHARACTER::RemoveAffect(DWORD dwType, bool useCompute, bool onlySave)
{
	// CHAT_BLOCK
	if (dwType == AFFECT_BLOCK_CHAT)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¤ÆÃ ±ÝÁö°¡ Ç®·È½À´Ï´Ù."));
	}
	// END_OF_CHAT_BLOCK

	bool flag = false;

	CAffect * pkAff;

	while ((pkAff = FindAffect(dwType)))
	{
		RemoveAffect(pkAff, useCompute, onlySave);
		flag = true;
	}

	return flag;
}

bool CHARACTER::IsAffectFlag(DWORD dwAff) const
{
	return m_afAffectFlag.IsSet(dwAff);
}

const std::vector<WORD> GoodAffects =
{
	AFFECT_MOV_SPEED,
	AFFECT_ATT_SPEED,

	AFFECT_STR,
	AFFECT_DEX,
	AFFECT_INT,
	AFFECT_CON,

	AFFECT_CHINA_FIREWORK,

	// Body Warrior
	SKILL_JEONGWI, // 3 (Berserk)
	SKILL_GEOMKYUNG, // 4 (Aura of the Sword)

	// Mental Warrior
	SKILL_CHUNKEON, // 19 (Strong Body)

	// Blade-Fight Ninja
	SKILL_EUNHYUNG, // 34 (Stealth)

	// Archery Ninja
	SKILL_GYEONGGONG, // 49 (Feather Walk)

	// Weaponry Sura
	SKILL_GWIGEOM, // 63 (Enchanted Blade)
	SKILL_TERROR, // 64 (Fear)
	SKILL_JUMAGAP, // 65 (Enchanted Armour)

	// Black Magic Sura
	SKILL_MANASHILED, // 79 (Dark Protection)

	// Dragon Force Shaman
	SKILL_HOSIN, // 94 (Blessing)
	SKILL_REFLECT, // 95 (Reflection)
	SKILL_GICHEON, // 96 (Dragon's Strength)

	// Healing Force Shaman
	SKILL_KWAESOK, // 110 (Swiftness)
	SKILL_JEUNGRYEOK, // 111 (Attack Up)
	
	SKILL_GICHEON,
#ifdef __WOLFMAN__
	// Instinct Lykan
	SKILL_JEOKRANG, // 174 (Crimson Wolf Soul)
	SKILL_CHEONGRANG, // 175 (Indigo Wolf Soul)
#endif
};

void CHARACTER::RemoveGoodAffect(bool bSave)
{
//	RemoveAffect(AFFECT_MOV_SPEED);
//	RemoveAffect(AFFECT_ATT_SPEED);
//	RemoveAffect(AFFECT_STR);
//	RemoveAffect(AFFECT_DEX);
//	RemoveAffect(AFFECT_INT);
//	RemoveAffect(AFFECT_CON);
//	RemoveAffect(AFFECT_CHINA_FIREWORK);
//
//	RemoveAffect(SKILL_JEONGWI);
//	RemoveAffect(SKILL_GEOMKYUNG);
//	RemoveAffect(SKILL_CHUNKEON);
//	RemoveAffect(SKILL_EUNHYUNG);
//	RemoveAffect(SKILL_GYEONGGONG);
//	RemoveAffect(SKILL_GWIGEOM);
//	RemoveAffect(SKILL_TERROR);
//	RemoveAffect(SKILL_JUMAGAP);
//	RemoveAffect(SKILL_MANASHILED);
//	RemoveAffect(SKILL_HOSIN);
//	RemoveAffect(SKILL_REFLECT);
//	RemoveAffect(SKILL_KWAESOK);
//	RemoveAffect(SKILL_JEUNGRYEOK);
//	RemoveAffect(SKILL_GICHEON);
//
//#ifdef __WOLFMAN__
//	RemoveAffect(SKILL_JEOKRANG);
//	RemoveAffect(SKILL_CHEONGRANG);
//#endif

	for (auto it : GoodAffects)
		RemoveAffect(it, false, bSave);

	ComputePoints(); 
}

#ifdef SKILL_AFFECT_DEATH_REMAIN
bool CHARACTER::IsGoodAffect(BYTE bAffectType)
#else
bool CHARACTER::IsGoodAffect(BYTE bAffectType) const
#endif
{
	switch (bAffectType)
	{
		case (AFFECT_MOV_SPEED):
		case (AFFECT_ATT_SPEED):
		case (AFFECT_STR):
		case (AFFECT_DEX):
		case (AFFECT_INT):
		case (AFFECT_CON):
		case (AFFECT_CHINA_FIREWORK):

		case (SKILL_JEONGWI):
		case (SKILL_GEOMKYUNG):
		case (SKILL_CHUNKEON):
		case (SKILL_EUNHYUNG):
		case (SKILL_GYEONGGONG):
		case (SKILL_GWIGEOM):
		case (SKILL_TERROR):
		case (SKILL_JUMAGAP):
		case (SKILL_MANASHILED):
		case (SKILL_HOSIN):
		case (SKILL_REFLECT):
		case (SKILL_KWAESOK):
		case (SKILL_JEUNGRYEOK):
		case (SKILL_GICHEON) :

#ifdef __WOLFMAN__
		case (SKILL_JEOKRANG) :
		case (SKILL_CHEONGRANG) :
#endif
			return true;
	}
	return false;
}

bool CHARACTER::IsGoodAffectSkill(BYTE bAffectType)
{
	switch (bAffectType)
	{
	case SKILL_JEONGWI:
	case SKILL_GEOMKYUNG:
	case SKILL_CHUNKEON:
	case SKILL_EUNHYUNG:
	case SKILL_GYEONGGONG:
	case SKILL_GWIGEOM:
	case SKILL_TERROR:
	case SKILL_JUMAGAP:
	case SKILL_MANASHILED:
	case SKILL_HOSIN:
	case SKILL_REFLECT:
	case SKILL_KWAESOK:
	case SKILL_JEUNGRYEOK:
	case SKILL_GICHEON:
		return true;
	}
	return false;
}

const std::vector<WORD> BadAffects =
{
	AFFECT_FIRE,
	AFFECT_POISON,

	AFFECT_STUN,
	AFFECT_SLOW,

	SKILL_TUSOK,
	SKILL_PABEOB,
};

void CHARACTER::RemoveBadAffect()
{
	sys_log(0, "RemoveBadAffect %s", GetName());
	// µ¶
	//RemovePoison();
	//RemoveFire();

	//// ½ºÅÏ		   : Value%·Î »ó´ë¹æÀ» 5ÃÊ°£ ¸Ó¸® À§¿¡ º°ÀÌ µ¹¾Æ°£´Ù. (¶§¸®¸é 1/2 È®·ü·Î Ç®¸²)			   AFF_STUN
	//RemoveAffect(AFFECT_STUN);

	//// ½½·Î¿ì		 : Value%·Î »ó´ë¹æÀÇ °ø¼Ó/ÀÌ¼Ó ¸ðµÎ ´À·ÁÁø´Ù. ¼ö·Ãµµ¿¡ µû¶ó ´Þ¶óÁü ±â¼ú·Î »ç¿ë ÇÑ °æ¿ì¿¡   AFF_SLOW
	//RemoveAffect(AFFECT_SLOW);

	//// Åõ¼Ó¸¶·É
	//RemoveAffect(SKILL_TUSOK);

	//RemoveAffect(SKILL_PABEOB);
	CAffect * pkAff;
	for (auto it : BadAffects)
	{
		pkAff = FindAffect(it);
		if (pkAff)
		{
			RemoveAffect(pkAff, false);

			switch (it)
			{
			case AFFECT_FIRE:
				if (m_pkFireEvent)
					event_cancel(&m_pkFireEvent);
				break;
			case AFFECT_POISON:
				if (m_pkPoisonEvent)
					event_cancel(&m_pkPoisonEvent);
				break;
			}
		}
	}

	ComputePoints();
}
