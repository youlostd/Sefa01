#include "stdafx.h"
#include "constants.h"
#include "pvp.h"
#include "crc32.h"
#include "packet.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "config.h"
#include "sectree_manager.h"
#include "buffer_manager.h"

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

using namespace std;

CPVP::CPVP(DWORD dwPID1, DWORD dwPID2)
{
	if (dwPID1 > dwPID2)
	{
		m_players[0].dwPID = dwPID1;
		m_players[1].dwPID = dwPID2;
		m_players[0].bAgree = true;
	}
	else
	{
		m_players[0].dwPID = dwPID2;
		m_players[1].dwPID = dwPID1;
		m_players[1].bAgree = true;
	}

	DWORD adwID[2];
	adwID[0] = m_players[0].dwPID;
	adwID[1] = m_players[1].dwPID;
	m_dwCRC = GetFastHash((const char *) &adwID, 8);
	m_bRevenge = false;

	SetLastFightTime();
}

CPVP::CPVP(CPVP & k)
{
	m_players[0] = k.m_players[0];
	m_players[1] = k.m_players[1];

	m_dwCRC = k.m_dwCRC;
	m_bRevenge = k.m_bRevenge;

	SetLastFightTime();
}

CPVP::~CPVP()
{
}

void CPVP::Packet(bool bDelete)
{
	if (!m_players[0].dwVID || !m_players[1].dwVID)
	{
		if (bDelete)
			sys_err("null vid when removing %u %u", m_players[0].dwVID, m_players[0].dwVID);

		return;
	}

	network::GCOutputPacket<network::GCPVPPacket> pack;


	if (bDelete)
	{
		pack->set_mode(PVP_MODE_NONE);
		pack->set_vid_src(m_players[0].dwVID);
		pack->set_vid_dst(m_players[1].dwVID);
	}
	else if (IsFight())
	{
		pack->set_mode(PVP_MODE_FIGHT);
		pack->set_vid_src(m_players[0].dwVID);
		pack->set_vid_dst(m_players[1].dwVID);
	}
	else
	{
		pack->set_mode(m_bRevenge ? PVP_MODE_REVENGE : PVP_MODE_AGREE);

		if (m_players[0].bAgree)
		{
			pack->set_vid_src(m_players[0].dwVID);
			pack->set_vid_dst(m_players[1].dwVID);
		}
		else
		{
			pack->set_vid_src(m_players[1].dwVID);
			pack->set_vid_dst(m_players[0].dwVID);
		}
	}

	const DESC_MANAGER::DESC_SET & c_rSet = DESC_MANAGER::instance().GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = c_rSet.begin();

	while (it != c_rSet.end())
	{
		LPDESC d = *it++;

		if (d->IsPhase(PHASE_GAME) || d->IsPhase(PHASE_DEAD))
			d->Packet(pack);
	}

	if (IsFight())
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(pack->vid_src());
		if (ch)
			ch->BroadcastTargetPacket();

		ch = CHARACTER_MANAGER::instance().Find(pack->vid_dst());
		if (ch)
			ch->BroadcastTargetPacket();
	}
}

bool CPVP::Agree(DWORD dwPID)
{
	m_players[m_players[0].dwPID != dwPID ? 1 : 0].bAgree = true;

	if (IsFight())
	{
		Packet();
		return true;
	}

	return false;
}

bool CPVP::IsFight()
{
	return (m_players[0].bAgree == m_players[1].bAgree) && m_players[0].bAgree;
}

void CPVP::Win(DWORD dwPID)
{
	int iSlot = m_players[0].dwPID != dwPID ? 1 : 0;

	m_bRevenge = true;

	m_players[iSlot].bAgree = true; // ÀÚµ¿À¸·Î µ¿ÀÇ
	m_players[!iSlot].bCanRevenge = true;
	m_players[!iSlot].bAgree = false;

	LPCHARACTER winner = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (winner)
	{
		/*int val = winner->GetQuestFlag("killcounter.duel_won");
		winner->SetQuestFlag("killcounter.duel_won", ++val);
		winner->ChatPacket(CHAT_TYPE_COMMAND, "killstat 4 %d", val);*/
		winner->SetRealPoint(POINT_DUELS_WON, winner->GetRealPoint(POINT_DUELS_WON) + 1);
	}

	LPCHARACTER looser = CHARACTER_MANAGER::instance().FindByPID(m_players[iSlot ^ 1].dwPID);
	if (looser)
	{
		/*int val = looser->GetQuestFlag("killcounter.duel_lost");
		looser->SetQuestFlag("killcounter.duel_lost", ++val);
		looser->ChatPacket(CHAT_TYPE_COMMAND, "killstat 5 %d", val);*/
		looser->SetRealPoint(POINT_DUELS_LOST, winner->GetRealPoint(POINT_DUELS_LOST) + 1);
	}

	Packet();
}

bool CPVP::CanRevenge(DWORD dwPID)
{
	return m_players[m_players[0].dwPID != dwPID ? 1 : 0].bCanRevenge;
}

void CPVP::SetVID(DWORD dwPID, DWORD dwVID)
{
	if (m_players[0].dwPID == dwPID)
		m_players[0].dwVID = dwVID;
	else
		m_players[1].dwVID = dwVID;
}

void CPVP::SetLastFightTime()
{
	m_dwLastFightTime = get_dword_time();
}

DWORD CPVP::GetLastFightTime()
{
	return m_dwLastFightTime;
}

CPVPManager::CPVPManager()
{
}

CPVPManager::~CPVPManager()
{
}

void CPVPManager::Insert(LPCHARACTER pkChr, LPCHARACTER pkVictim)
{
	if (pkChr->IsDead() || pkVictim->IsDead())
		return;

	CPVP kPVP(pkChr->GetPlayerID(), pkVictim->GetPlayerID());

	CPVP * pkPVP;

	if ((pkPVP = Find(kPVP.m_dwCRC)))
	{
		// º¹¼öÇÒ ¼ö ÀÖÀ¸¸é ¹Ù·Î ½Î¿ò!
		if (pkPVP->Agree(pkChr->GetPlayerID()))
		{
			pkVictim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkVictim, "%s´Ô°úÀÇ ´ë°á ½ÃÀÛ!"), pkChr->GetName());
			pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChr, "%s´Ô°úÀÇ ´ë°á ½ÃÀÛ!"), pkVictim->GetName());
		}
		return;
	}

	pkPVP = M2_NEW CPVP(kPVP);

	pkPVP->SetVID(pkChr->GetPlayerID(), pkChr->GetVID());
	pkPVP->SetVID(pkVictim->GetPlayerID(), pkVictim->GetVID());

	m_map_pkPVP.insert(map<DWORD, CPVP *>::value_type(pkPVP->m_dwCRC, pkPVP));

	m_map_pkPVPSetByID[pkChr->GetPlayerID()].insert(pkPVP);
	m_map_pkPVPSetByID[pkVictim->GetPlayerID()].insert(pkPVP);

	pkPVP->Packet();

	char msg[CHAT_MAX_LEN + 1];
	snprintf(msg, sizeof(msg), LC_TEXT(pkVictim, "%s´ÔÀÌ ´ë°á½ÅÃ»À» Çß½À´Ï´Ù. ½Â³«ÇÏ·Á¸é ´ë°áµ¿ÀÇ¸¦ ÇÏ¼¼¿ä."), pkChr->GetName());

	pkVictim->ChatPacket(CHAT_TYPE_INFO, msg);
	pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChr, "%s¿¡°Ô ´ë°á½ÅÃ»À» Çß½À´Ï´Ù."), pkVictim->GetName());

	// NOTIFY_PVP_MESSAGE
	LPDESC pkVictimDesc = pkVictim->GetDesc();
	if (pkVictimDesc)
	{
		network::GCOutputPacket<network::GCWhisperPacket> pack;

		pack->set_type(WHISPER_TYPE_SYSTEM);
		pack->set_name_from(pkChr->GetName());
		pack->set_message(msg);

		pkVictimDesc->Packet(pack);
	}	
	// END_OF_NOTIFY_PVP_MESSAGE
}

void CPVPManager::ConnectEx(LPCHARACTER pkChr, bool bDisconnect)
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return;

	DWORD dwVID = bDisconnect ? 0 : pkChr->GetVID();

	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP * pkPVP = *it2++;
		pkPVP->SetVID(pkChr->GetPlayerID(), dwVID);
	}
}

void CPVPManager::Connect(LPCHARACTER pkChr)
{
	ConnectEx(pkChr, false);
}

void CPVPManager::Disconnect(LPCHARACTER pkChr)
{
	//ConnectEx(pkChr, true);
}

void CPVPManager::GiveUp(LPCHARACTER pkChr, DWORD dwKillerPID) // This method is calling from no where yet.
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return;

	sys_log(1, "PVPManager::Dead %d", pkChr->GetPlayerID());
	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP * pkPVP = *it2++;

		DWORD dwCompanionPID;

		if (pkPVP->m_players[0].dwPID == pkChr->GetPlayerID())
			dwCompanionPID = pkPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pkPVP->m_players[0].dwPID;

		if (dwCompanionPID != dwKillerPID)
			continue;

		pkPVP->SetVID(pkChr->GetPlayerID(), 0);

		m_map_pkPVPSetByID.erase(dwCompanionPID);

		it->second.erase(pkPVP);

		if (it->second.empty())
			m_map_pkPVPSetByID.erase(it);

		m_map_pkPVP.erase(pkPVP->m_dwCRC);

		pkPVP->Packet(true);
		M2_DELETE(pkPVP);
		break;
	}
}

// ¸®ÅÏ°ª: 0 = PK, 1 = PVP
// PVP¸¦ ¸®ÅÏÇÏ¸é °æÇèÄ¡³ª ¾ÆÀÌÅÛÀ» ¶³±¸°í PK¸é ¶³±¸Áö ¾Ê´Â´Ù.
bool CPVPManager::Dead(LPCHARACTER pkChr, DWORD dwKillerPID)
{
	CPVPSetMap::iterator it = m_map_pkPVPSetByID.find(pkChr->GetPlayerID());

	if (it == m_map_pkPVPSetByID.end())
		return false;

	bool found = false;

	sys_log(1, "PVPManager::Dead %d", pkChr->GetPlayerID());
	TR1_NS::unordered_set<CPVP*>::iterator it2 = it->second.begin();

	while (it2 != it->second.end())
	{
		CPVP * pkPVP = *it2++;

		DWORD dwCompanionPID;

		if (pkPVP->m_players[0].dwPID == pkChr->GetPlayerID())
			dwCompanionPID = pkPVP->m_players[1].dwPID;
		else
			dwCompanionPID = pkPVP->m_players[0].dwPID;

		if (dwCompanionPID == dwKillerPID)
		{
			if (pkPVP->IsFight())
			{
				pkPVP->SetLastFightTime();
				pkPVP->Win(dwKillerPID);
				found = true;
				break;
			}
			else if (get_dword_time() - pkPVP->GetLastFightTime() <= 15000)
			{
				found = true;
				break;
			}
		}
	}

	return found;
}

bool CPVPManager::CanAttack(LPCHARACTER pkChr, LPCHARACTER pkVictim)
{
	switch (pkVictim->GetCharType())
	{
		case CHAR_TYPE_NPC:
		case CHAR_TYPE_WARP:
		case CHAR_TYPE_GOTO:
			return false;
	}

	if (pkChr == pkVictim)  // ³»°¡ ³¯ Ä¥¶ó°í ÇÏ³× -_-
		return false;

#ifdef __FAKE_BUFF__
	if (pkChr->FakeBuff_Check() || pkVictim->FakeBuff_Check())
		return false;
#endif
	
// GUARDIAN_NO_WALK
	if (pkChr->IsGuardNPC() ||pkVictim->IsGuardNPC())
		return false;
// GUARDIAN_NO_WALK

#ifdef __FAKE_PC__
	if (pkVictim->IsNPC() && pkChr->IsNPC() && !pkChr->IsGuardNPC() && !pkChr->FakePC_Check() && !pkVictim->FakePC_Check())
#else
	if (pkVictim->IsNPC() && pkChr->IsNPC() && !pkChr->IsGuardNPC())
#endif
		return false;

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(pkChr->GetMapIndex()) || CCombatZoneManager::Instance().IsCombatZoneMap(pkVictim->GetMapIndex()))
		return true;
#endif

#ifdef __FAKE_PC__
	if (pkChr->FakePC_Check() && pkVictim->FakePC_Check())
	{
		if (pkChr->FakePC_IsSupporter() == pkVictim->FakePC_IsSupporter() && pkChr->GetPVPTeam() == pkVictim->GetPVPTeam())
			return false;
	}

	if ((pkChr->FakePC_IsSupporter() && !pkChr->FakePC_CanAttack()) || (pkVictim->FakePC_IsSupporter() && !pkVictim->FakePC_CanAttack()))
		return false;

	if ((((pkChr->FakePC_IsSupporter() && pkVictim->IsPC()) || (pkChr->IsPC() && pkVictim->FakePC_IsSupporter())) && pkChr->GetPVPTeam() == pkVictim->GetPVPTeam()) ||
		(pkChr->FakePC_Check() && !pkChr->FakePC_IsSupporter() && !pkVictim->IsPC()) ||
		(!pkChr->IsPC() && pkVictim->FakePC_Check() && !pkVictim->FakePC_IsSupporter()))
		return false;
#endif

	if( true == pkChr->IsHorseSummoned() && pkChr->IsRiding() )
	{
		if( 1 == pkChr->GetHorseGrade() ) 
			return false;
	}

	if (pkVictim->IsNPC() || pkChr->IsNPC())
	{
		return true;
	}

	if (pkVictim->IsObserverMode() || pkChr->IsObserverMode())
		return false;

	if (pkChr->GetPVPTeam() != -1 && pkVictim->GetPVPTeam() != -1)
		return pkChr->GetPVPTeam() != pkVictim->GetPVPTeam();

	{
		BYTE bMapEmpire = SECTREE_MANAGER::instance().GetEmpireFromMapIndex(pkChr->GetMapIndex());

		if ( pkChr->GetPKMode() == PK_MODE_PROTECT && pkChr->GetEmpire() == bMapEmpire ||
				pkVictim->GetPKMode() == PK_MODE_PROTECT && pkVictim->GetEmpire() == bMapEmpire )
		{
			return false;
		}
	}

#ifdef ENABLE_BLOCK_PKMODE
	if (pkChr->GetEmpire() != pkVictim->GetEmpire())
	{
		if (pkChr->GetPKMode() == PK_MODE_PROTECT || pkVictim->GetPKMode() == PK_MODE_PROTECT)
			return false;

		return true;
	}
#else
	if (pkChr->GetEmpire() != pkVictim->GetEmpire())
		return true;
#endif

	bool beKillerMode = false;

	if (pkVictim->GetParty() && pkVictim->GetParty() == pkChr->GetParty())
	{
		return false;
		// Cannot attack same party on any pvp model
	}
	else
	{
		if (pkVictim->IsKillerMode())
		{
			return true;
		}

		if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() >= 0)
		{
			if (g_protectNormalPlayer)
			{
			// ¹ü¹ýÀÚ´Â ÆòÈ­¸ðµåÀÎ ÂøÇÑ»ç¶÷À» °ø°ÝÇÒ ¼ö ¾ø´Ù.
			if (PK_MODE_PEACE == pkVictim->GetPKMode())
				return false;
			}
		}


		switch (pkChr->GetPKMode())
		{
			case PK_MODE_PEACE:
			case PK_MODE_REVENGE:
				// Cannot attack same guild
				if (pkVictim->GetGuild() && pkVictim->GetGuild() == pkChr->GetGuild())
					break;

				if (pkChr->GetPKMode() == PK_MODE_REVENGE)
				{
					if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() >= 0)
					{
						pkChr->SetKillerMode(true);
						return true;
					}
					else if (pkChr->GetAlignment() >= 0 && pkVictim->GetAlignment() < 0)
						return true;
				}
				break;

			case PK_MODE_GUILD:
				// Same implementation from PK_MODE_FREE except for attacking same guild
				if (!pkChr->GetGuild() || (pkVictim->GetGuild() != pkChr->GetGuild()))
				{
					if (pkVictim->GetAlignment() >= 0)
						pkChr->SetKillerMode(true);
					else if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() < 0)
						pkChr->SetKillerMode(true);

					return true;
				}
				break;

			case PK_MODE_FREE:
				{
					if (pkVictim->GetAlignment() >= 0)
						pkChr->SetKillerMode(true);
					else if (pkChr->GetAlignment() < 0 && pkVictim->GetAlignment() < 0)
						pkChr->SetKillerMode(true);

					return true;
				}
				break;
		}
	}

	CPVP kPVP(pkChr->GetPlayerID(), pkVictim->GetPlayerID());
	CPVP * pkPVP = Find(kPVP.m_dwCRC);

	if (!pkPVP || !pkPVP->IsFight())
	{
		if (beKillerMode)
			pkChr->SetKillerMode(true);

		return (beKillerMode);
	}

	pkPVP->SetLastFightTime();
	return true;
}

CPVP * CPVPManager::Find(DWORD dwCRC)
{
	map<DWORD, CPVP *>::iterator it = m_map_pkPVP.find(dwCRC);

	if (it == m_map_pkPVP.end())
		return NULL;

	return it->second;
}

void CPVPManager::Delete(CPVP * pkPVP)
{
	map<DWORD, CPVP *>::iterator it = m_map_pkPVP.find(pkPVP->m_dwCRC);

	if (it == m_map_pkPVP.end())
		return;

	m_map_pkPVP.erase(it);
	m_map_pkPVPSetByID[pkPVP->m_players[0].dwPID].erase(pkPVP);
	m_map_pkPVPSetByID[pkPVP->m_players[1].dwPID].erase(pkPVP);

	M2_DELETE(pkPVP);
}

void CPVPManager::SendList(LPDESC d)
{
	map<DWORD, CPVP *>::iterator it = m_map_pkPVP.begin();

	DWORD dwVID = d->GetCharacter()->GetVID();

	network::GCOutputPacket<network::GCPVPPacket> pack;


	while (it != m_map_pkPVP.end())
	{
		CPVP * pkPVP = (it++)->second;

		if (!pkPVP->m_players[0].dwVID || !pkPVP->m_players[1].dwVID)
			continue;

		// VID°¡ µÑ´Ù ÀÖÀ» °æ¿ì¿¡¸¸ º¸³½´Ù.
		if (pkPVP->IsFight())
		{
			pack->set_mode(PVP_MODE_FIGHT);
			pack->set_vid_src(pkPVP->m_players[0].dwVID);
			pack->set_vid_dst(pkPVP->m_players[1].dwVID);
		}
		else
		{
			pack->set_mode(pkPVP->m_bRevenge ? PVP_MODE_REVENGE : PVP_MODE_AGREE);

			if (pkPVP->m_players[0].bAgree)
			{
				pack->set_vid_src(pkPVP->m_players[0].dwVID);
				pack->set_vid_dst(pkPVP->m_players[1].dwVID);
			}
			else
			{
				pack->set_vid_src(pkPVP->m_players[1].dwVID);
				pack->set_vid_dst(pkPVP->m_players[0].dwVID);
			}
		}

		d->Packet(pack);
		sys_log(1, "PVPManager::SendList %d %d", pack->vid_src(), pack->vid_dst());

		if (pkPVP->m_players[0].dwVID == dwVID)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(pkPVP->m_players[1].dwVID);
			if (ch && ch->GetDesc())
			{
				LPDESC d = ch->GetDesc();
				d->Packet(pack);
			}
		}
		else if (pkPVP->m_players[1].dwVID == dwVID)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(pkPVP->m_players[0].dwVID);
			if (ch && ch->GetDesc())
			{
				LPDESC d = ch->GetDesc();
				d->Packet(pack);
			}
		}
	}
}

void CPVPManager::Process()
{
	map<DWORD, CPVP *>::iterator it = m_map_pkPVP.begin();

	while (it != m_map_pkPVP.end())
	{
		CPVP * pvp = (it++)->second;

		if (get_dword_time() - pvp->GetLastFightTime() > 600000) // 10ºÐ ÀÌ»ó ½Î¿òÀÌ ¾ø¾úÀ¸¸é
		{
			pvp->Packet(true);
			Delete(pvp);
		}
	}
}

