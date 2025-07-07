#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "log.h"
#include "char.h"
#include "packet.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "db.h"
#include "affect.h"
#include "p2p.h"
#include "war_map.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "guild_manager.h"

enum
{
	GUILD_WAR_WAIT_START_DURATION = 10*60
};

// 
// Packet
//
void CGuild::GuildWarPacket(DWORD dwOppGID, BYTE bWarType, BYTE bWarState)
{
	network::GCOutputPacket<network::GCGuildWarPacket> pack;
	pack->set_guild_self(GetID());
	pack->set_guild_opponent(dwOppGID);
	pack->set_war_state(bWarState);
	pack->set_type(bWarType);

	for (auto it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;

		if (bWarState == GUILD_WAR_ON_WAR)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀüÁß¿¡´Â »ç³É¿¡ µû¸¥ ÀÌÀÍÀÌ ¾ø½À´Ï´Ù."));

		LPDESC d = ch->GetDesc();

		if (d)
		{
			ch->SendGuildName(dwOppGID);
			d->Packet(pack);
		}
	}
}

void CGuild::SendEnemyGuild(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	network::GCOutputPacket<network::GCGuildWarPacket> pack;
	pack->set_guild_self(GetID());

	for (itertype(m_EnemyGuild) it = m_EnemyGuild.begin(); it != m_EnemyGuild.end(); ++it)
	{
		ch->SendGuildName( it->first );

		pack->set_guild_opponent(it->first);
		pack->set_type(it->second.type);
		pack->set_war_state(it->second.state);
		d->Packet(pack);

		if (it->second.state == GUILD_WAR_ON_WAR)
		{
			long lScore;

			lScore = GetWarScoreAgainstTo(it->first);

			network::GCOutputPacket<network::GCGuildWarPointPacket> p;
			p->set_gain_guild_id(GetID());
			p->set_opponent_guild_id(it->first);
			p->set_point(lScore);
			d->Packet(p);

			lScore = CGuildManager::instance().TouchGuild(it->first)->GetWarScoreAgainstTo(GetID());

			p->set_gain_guild_id(it->first);
			p->set_opponent_guild_id(GetID());
			p->set_point(lScore);
			d->Packet(pack);
		}
	}
}

//
// War Login
//
int CGuild::GetGuildWarState(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return GUILD_WAR_NONE;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	return (it != m_EnemyGuild.end()) ? (it->second.state) : GUILD_WAR_NONE;
} 

int CGuild::GetGuildWarType(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return GUILD_WAR_TYPE_FIELD;

	return git->second.type;
}

DWORD CGuild::GetGuildWarMapIndex(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return 0;

	return git->second.map_index;
}

bool CGuild::CanStartWar(BYTE bGuildWarType) // Å¸ÀÔ¿¡ µû¶ó ´Ù¸¥ Á¶°ÇÀÌ »ý±æ ¼öµµ ÀÖÀ½
{
	if (bGuildWarType >= GUILD_WAR_TYPE_MAX_NUM)
		return false;

	// Å×½ºÆ®½Ã¿¡´Â ÀÎ¿ø¼ö¸¦ È®ÀÎÇÏÁö ¾Ê´Â´Ù.
	if (test_server || quest::CQuestManager::instance().GetEventFlag("guild_war_test") != 0)
		return GetLadderPoint() > 0;

	return GetLadderPoint() > 0 && GetMemberCount() >= GUILD_WAR_MIN_MEMBER_COUNT;
}

bool CGuild::UnderWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return false;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	return (it != m_EnemyGuild.end()) && (it->second.IsWarBegin());
} 

DWORD CGuild::UnderAnyWar(BYTE bType)
{
	for (itertype(m_EnemyGuild) it = m_EnemyGuild.begin(); it != m_EnemyGuild.end(); ++it)
	{
		if (bType < GUILD_WAR_TYPE_MAX_NUM)
			if (it->second.type != bType)
				continue;

		if (it->second.IsWarBegin())
			return it->first;
	}

	return 0;
}

void CGuild::SetWarScoreAgainstTo(DWORD dwOppGID, int iScore)
{
	DWORD dwSelfGID = GetID();

	sys_log(0, "GuildWarScore Set %u from %u %d", dwSelfGID, dwOppGID, iScore);
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		it->second.score = iScore;

		if (it->second.type != GUILD_WAR_TYPE_FIELD)
		{
			CGuild * gOpp = CGuildManager::instance().TouchGuild(dwOppGID);
			CWarMap * pMap = CWarMapManager::instance().Find(it->second.map_index);

			if (pMap)
				pMap->UpdateScore(dwSelfGID, iScore, dwOppGID, gOpp->GetWarScoreAgainstTo(dwSelfGID));
		}
		else
		{
			network::GCOutputPacket<network::GCGuildWarPointPacket> p;
			p->set_gain_guild_id(dwSelfGID);
			p->set_opponent_guild_id(dwOppGID);
			p->set_point(iScore);

			Packet(p);

			CGuild * gOpp = CGuildManager::instance().TouchGuild(dwOppGID);

			if (gOpp)
				gOpp->Packet(p);
		}
	}
}

int CGuild::GetWarScoreAgainstTo(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		sys_log(0, "GuildWarScore Get %u from %u %d", GetID(), dwOppGID, it->second.score);
		return it->second.score;
	}

	sys_log(0, "GuildWarScore Get %u from %u No data", GetID(), dwOppGID);
	return 0;
}

DWORD CGuild::GetWarStartTime(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return 0;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return 0;

	return it->second.war_start_time;
}

const TGuildWarInfo& GuildWar_GetTypeInfo(unsigned type)
{
	return KOR_aGuildWarInfo[type];
}

unsigned GuildWar_GetTypeMapIndex(unsigned type)
{
	return GuildWar_GetTypeInfo(type).lMapIndex;
}

bool GuildWar_IsWarMap(unsigned type)
{
	if (type == GUILD_WAR_TYPE_FIELD)
		return true;

	unsigned mapIndex = GuildWar_GetTypeMapIndex(type);

	if (SECTREE_MANAGER::instance().GetMapRegion(mapIndex))
		return true;

	return false;
}

void CGuild::NotifyGuildMaster(const char* msg)
{
	LPCHARACTER ch = GetMasterCharacter();
	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, msg));
}

extern void map_allow_log();

//
// War State Relative
//
//
// A Declare -> B Refuse
//		   -> B Declare -> StartWar -> EndWar
//
// A Declare -> B Refuse
//		   -> B Declare -> WaitStart -> Fail
//									 -> StartWar -> EndWar
//
void CGuild::RequestDeclareWar(DWORD dwOppGID, BYTE type)
{
	if (dwOppGID == GetID())
	{
		sys_log(0, "GuildWar.DeclareWar.DECLARE_WAR_SELF id(%d -> %d), type(%d)", GetID(), dwOppGID, type);
		return;
	}

	if (type >= GUILD_WAR_TYPE_MAX_NUM)
	{
		sys_log(0, "GuildWar.DeclareWar.UNKNOWN_WAR_TYPE id(%d -> %d), type(%d)", GetID(), dwOppGID, type);
		return;
	}

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	if (it == m_EnemyGuild.end())
	{
		if (!GuildWar_IsWarMap(type))
		{
			sys_err("GuildWar.DeclareWar.NOT_EXIST_MAP id(%d -> %d), type(%d), map(%d)", 
					GetID(), dwOppGID, type, GuildWar_GetTypeMapIndex(type));

			map_allow_log();
			NotifyGuildMaster("ÀüÀï ¼­¹ö°¡ ¿­·ÁÀÖÁö ¾Ê¾Æ ±æµåÀüÀ» ½ÃÀÛÇÒ ¼ö ¾ø½À´Ï´Ù.");
			return;
		}

		network::GDOutputPacket<network::GDGuildWarPacket> p;
		p->set_type(type);
		p->set_war(GUILD_WAR_SEND_DECLARE);
		p->set_guild_from(GetID());
		p->set_guild_to(dwOppGID);
		db_clientdesc->DBPacket(p);
		sys_log(0, "GuildWar.DeclareWar id(%d -> %d), type(%d)", GetID(), dwOppGID, type);
		return;
	}

	switch (it->second.state)
	{	
		case GUILD_WAR_RECV_DECLARE:
			{
				const unsigned saved_type = it->second.type;

				if (saved_type == GUILD_WAR_TYPE_FIELD)
				{
					network::GDOutputPacket<network::GDGuildWarPacket> p;
					p->set_type(saved_type);
					p->set_war(GUILD_WAR_ON_WAR);
					p->set_guild_from(GetID());
					p->set_guild_to(dwOppGID);
					db_clientdesc->DBPacket(p);
					sys_log(0, "GuildWar.AcceptWar id(%d -> %d), type(%d)", GetID(), dwOppGID, saved_type);
					return;
				}

				if (!GuildWar_IsWarMap(saved_type))
				{
					sys_err("GuildWar.AcceptWar.NOT_EXIST_MAP id(%d -> %d), type(%d), map(%d)", 
							GetID(), dwOppGID, type, GuildWar_GetTypeMapIndex(type));

					map_allow_log();
					NotifyGuildMaster("ÀüÀï ¼­¹ö°¡ ¿­·ÁÀÖÁö ¾Ê¾Æ ±æµåÀüÀ» ½ÃÀÛÇÒ ¼ö ¾ø½À´Ï´Ù.");
					return;
				}

				const TGuildWarInfo& guildWarInfo = GuildWar_GetTypeInfo(saved_type);

				network::GDOutputPacket<network::GDGuildWarPacket> p;
				p->set_type(saved_type);
				p->set_war(GUILD_WAR_WAIT_START);
				p->set_guild_from(GetID());
				p->set_guild_to(dwOppGID);
				p->set_war_price(guildWarInfo.iWarPrice);
				p->set_initial_score(guildWarInfo.iInitialScore);

				if (test_server)
					p->set_initial_score(p->initial_score() / 10);

				db_clientdesc->DBPacket(p);

				sys_log(0, "GuildWar.WaitStartSendToDB id(%d vs %d), type(%d), bet(%d), map_index(%d)", 
						GetID(), dwOppGID, saved_type, guildWarInfo.iWarPrice, guildWarInfo.lMapIndex);

			}
			break;
		case GUILD_WAR_SEND_DECLARE:
			{
				NotifyGuildMaster("ÀÌ¹Ì ¼±ÀüÆ÷°í ÁßÀÎ ±æµåÀÔ´Ï´Ù.");
			}
			break;
		default:
			sys_err("GuildWar.DeclareWar.UNKNOWN_STATE[%d]: id(%d vs %d), type(%d), guild(%s:%u)", 
					it->second.state, GetID(), dwOppGID, type, GetName(), GetID());
			break;
	}
}

bool CGuild::DeclareWar(DWORD dwOppGID, BYTE type, BYTE state)
{
	if (m_EnemyGuild.find(dwOppGID) != m_EnemyGuild.end())
		return false;

	TGuildWar gw(type);
	gw.state = state;

	m_EnemyGuild.insert(std::make_pair(dwOppGID, gw));

	GuildWarPacket(dwOppGID, type, state);
	return true;
}

bool CGuild::CheckStartWar(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return false;

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_ON_WAR)
		return false;

	return true;
}

void CGuild::StartWar(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return;

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_ON_WAR)
		return;

	gw.state = GUILD_WAR_ON_WAR;
	gw.war_start_time = get_global_time();

	GuildWarPacket(dwOppGID, gw.type, GUILD_WAR_ON_WAR);

	if (gw.type != GUILD_WAR_TYPE_FIELD)
		GuildWarEntryAsk(dwOppGID);
}

bool CGuild::WaitStartWar(DWORD dwOppGID)
{
	//ÀÚ±âÀÚ½ÅÀÌ¸é 
	if (dwOppGID == GetID())
	{
		sys_log(0 ,"GuildWar.WaitStartWar.DECLARE_WAR_SELF id(%u -> %u)", GetID(), dwOppGID);
		return false;
	}

	//»ó´ë¹æ ±æµå TGuildWar ¸¦ ¾ò¾î¿Â´Ù.
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	if (it == m_EnemyGuild.end())
	{
		sys_log(0 ,"GuildWar.WaitStartWar.UNKNOWN_ENEMY id(%u -> %u)", GetID(), dwOppGID);
		return false;
	}

	//·¹ÆÛ·±½º¿¡ µî·ÏÇÏ°í
	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_WAIT_START)
	{
		sys_log(0 ,"GuildWar.WaitStartWar.UNKNOWN_STATE id(%u -> %u), state(%d)", GetID(), dwOppGID, gw.state);
		return false;
	}

	//»óÅÂ¸¦ ÀúÀåÇÑ´Ù.
	gw.state = GUILD_WAR_WAIT_START;

	//»ó´ëÆíÀÇ ±æµå Å¬·¡½º Æ÷ÀÎÅÍ¸¦ ¾ò¾î¿À°í
	CGuild* g = CGuildManager::instance().FindGuild(dwOppGID);
	if (!g)
	{
		sys_log(0 ,"GuildWar.WaitStartWar.NOT_EXIST_GUILD id(%u -> %u)", GetID(), dwOppGID);
		return false;
	}

	// GUILDWAR_INFO
	const TGuildWarInfo& rkGuildWarInfo = GuildWar_GetTypeInfo(gw.type);
	// END_OF_GUILDWAR_INFO


	// ÇÊµåÇüÀÌ¸é ¸Ê»ý¼º ¾ÈÇÔ
	if (gw.type == GUILD_WAR_TYPE_FIELD)
	{
		sys_log(0 ,"GuildWar.WaitStartWar.FIELD_TYPE id(%u -> %u)", GetID(), dwOppGID);
		return true;
	}		

	// ÀüÀï ¼­¹ö ÀÎÁö È®ÀÎ
	sys_log(0 ,"GuildWar.WaitStartWar.CheckWarServer id(%u -> %u), type(%d), map(%d)", 
			GetID(), dwOppGID, gw.type, rkGuildWarInfo.lMapIndex);

	if (!map_allow_find(rkGuildWarInfo.lMapIndex))
	{
		sys_log(0 ,"GuildWar.WaitStartWar.SKIP_WAR_MAP id(%u -> %u)", GetID(), dwOppGID);
		return true;
	}


	DWORD id1 = GetID();
	DWORD id2 = dwOppGID;

	if (id1 > id2)
		std::swap(id1, id2);

	//¿öÇÁ ¸ÊÀ» »ý¼º
	DWORD lMapIndex = CWarMapManager::instance().CreateWarMap(rkGuildWarInfo, id1, id2);
	if (!lMapIndex) 
	{
		sys_err("GuildWar.WaitStartWar.CREATE_WARMAP_ERROR id(%u vs %u), type(%u), map(%d)", id1, id2, gw.type, rkGuildWarInfo.lMapIndex);
		CGuildManager::instance().RequestEndWar(GetID(), dwOppGID);
		return false;
	}

	sys_log(0, "GuildWar.WaitStartWar.CreateMap id(%u vs %u), type(%u), map(%d) -> map_inst(%u)", id1, id2, gw.type, rkGuildWarInfo.lMapIndex, lMapIndex);

	//±æµåÀü Á¤º¸¿¡ ¸ÊÀÎµ¦½º¸¦ ¼¼ÆÃ
	gw.map_index = lMapIndex;

	//¾çÂÊ¿¡ µî·Ï(?)
	SetGuildWarMapIndex(dwOppGID, lMapIndex);
	g->SetGuildWarMapIndex(GetID(), lMapIndex);

	///////////////////////////////////////////////////////
	network::GGOutputPacket<network::GGGuildWarZoneMapIndexPacket> p;

	p->set_guild_id1(id1);
	p->set_guild_id2(id2);
	p->set_map_index(lMapIndex);

	P2P_MANAGER::instance().Send(p);
	///////////////////////////////////////////////////////

	return true;
}

void CGuild::RequestRefuseWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end() && it->second.state == GUILD_WAR_RECV_DECLARE)
	{
		network::GDOutputPacket<network::GDGuildWarPacket> p;
		p->set_war(GUILD_WAR_REFUSE);
		p->set_guild_from(GetID());
		p->set_guild_to(dwOppGID);

		db_clientdesc->DBPacket(p);
	}
}

void CGuild::RefuseWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end() && (it->second.state == GUILD_WAR_SEND_DECLARE || it->second.state == GUILD_WAR_RECV_DECLARE))
	{
		BYTE type = it->second.type;
		m_EnemyGuild.erase(dwOppGID);

		GuildWarPacket(dwOppGID, type, GUILD_WAR_END);
	}
}

void CGuild::ReserveWar(DWORD dwOppGID, BYTE type)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
	{
		TGuildWar gw(type);
		gw.state = GUILD_WAR_RESERVE;
		m_EnemyGuild.insert(std::make_pair(dwOppGID, gw));
	}
	else
		it->second.state = GUILD_WAR_RESERVE;

	sys_log(0, "Guild::ReserveWar %u", dwOppGID);
}

void CGuild::EndWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		CWarMap * pMap = CWarMapManager::instance().Find(it->second.map_index);

		if (pMap)
			pMap->SetEnded();

		GuildWarPacket(dwOppGID, it->second.type, GUILD_WAR_END);
		m_EnemyGuild.erase(it);

		if (!UnderAnyWar())
		{
			for (itertype(m_memberOnline) it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPCHARACTER ch = *it;
				ch->RemoveAffect(GUILD_SKILL_BLOOD);
				ch->RemoveAffect(GUILD_SKILL_BLESS);
				ch->RemoveAffect(GUILD_SKILL_SEONGHWI);
				ch->RemoveAffect(GUILD_SKILL_ACCEL);
				ch->RemoveAffect(GUILD_SKILL_BUNNO);
				ch->RemoveAffect(GUILD_SKILL_JUMUN);

				ch->RemoveBadAffect();
			}
		}
	}
}

void CGuild::SetGuildWarMapIndex(DWORD dwOppGID, long lMapIndex)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return;

	it->second.map_index = lMapIndex;
	sys_log(0, "GuildWar.SetGuildWarMapIndex id(%d -> %d), map(%d)", GetID(), dwOppGID, lMapIndex);
}

void CGuild::GuildWarEntryAccept(DWORD dwOppGID, LPCHARACTER ch)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return;

	TGuildWar & gw(git->second);

	if (gw.type == GUILD_WAR_TYPE_FIELD)
		return;

	if (gw.state != GUILD_WAR_ON_WAR)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ¹Ì ÀüÀïÀÌ ³¡³µ½À´Ï´Ù."));
		return;
	}

	if (!gw.map_index)
		return;

	PIXEL_POSITION pos;

	if (!CWarMapManager::instance().GetStartPosition(gw.map_index, GetID() < dwOppGID ? 0 : 1, pos))
		return;

	quest::PC * pPC = quest::CQuestManager::instance().GetPC(ch->GetPlayerID());
	pPC->SetFlag("war.is_war_member", 1);

	ch->SaveExitLocation();
	ch->WarpSet(pos.x, pos.y, gw.map_index);
}

void CGuild::GuildWarEntryAsk(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);
	if (git == m_EnemyGuild.end())
	{
		sys_err("GuildWar.GuildWarEntryAsk.UNKNOWN_ENEMY(%d)", dwOppGID);
		return;
	}

	TGuildWar & gw(git->second);

	sys_log(0, "GuildWar.GuildWarEntryAsk id(%d vs %d), map(%d)", GetID(), dwOppGID, gw.map_index);
	if (!gw.map_index)
	{
		sys_err("GuildWar.GuildWarEntryAsk.NOT_EXIST_MAP id(%d vs %d)", GetID(), dwOppGID);
		return;
	}

	PIXEL_POSITION pos;
	if (!CWarMapManager::instance().GetStartPosition(gw.map_index, GetID() < dwOppGID ? 0 : 1, pos))
	{
		sys_err("GuildWar.GuildWarEntryAsk.START_POSITION_ERROR id(%d vs %d), pos(%d, %d)", GetID(), dwOppGID, pos.x, pos.y);
		return;
	}

	sys_log(0, "GuildWar.GuildWarEntryAsk.OnlineMemberCount(%d)", m_memberOnline.size());

	itertype(m_memberOnline) it = m_memberOnline.begin();

	while (it != m_memberOnline.end())
	{
		LPCHARACTER ch = *it++;

		using namespace quest;
		unsigned int questIndex=CQuestManager::instance().GetQuestIndexByName("guild_war_join");
		if (questIndex)
		{
			sys_log(0, "GuildWar.GuildWarEntryAsk.SendLetterToMember pid(%d), qid(%d)", ch->GetPlayerID(), questIndex);
			CQuestManager::instance().Letter(ch->GetPlayerID(), questIndex, 0);
		}
		else
		{
			sys_err("GuildWar.GuildWarEntryAsk.SendLetterToMember.QUEST_ERROR pid(%d), quest_name('guild_war_join.quest')", 
					ch->GetPlayerID(), questIndex);
			break;
		}
	}
}

//
// LADDER POINT
//
void CGuild::SetLadderPoint(int point) 
{ 
	if (m_data.ladder_point != point)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "<±æµå> ·¡´õ Á¡¼ö°¡ [%d] Á¡ÀÌ µÇ¾ú½À´Ï´Ù", point);
		for (itertype(m_memberOnline) it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		{
			LPCHARACTER ch = (*it);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, buf));
		}
	}
	m_data.ladder_point = point; 

	network::GCOutputPacket<network::GCGuildRankAndPointPacket> pack;
	pack->set_point(point);
	pack->set_rank(CGuildManager::instance().GetRank(this));

	Packet(pack);
}

void CGuild::ChangeLadderPoint(int iChange)
{
	network::GDOutputPacket<network::GDGuildChangeLadderPointPacket> p;
	p->set_guild_id(GetID());
	p->set_change(iChange);
	db_clientdesc->DBPacket(p);
}
