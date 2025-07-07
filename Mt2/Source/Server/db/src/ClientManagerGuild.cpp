// vim:ts=4 sw=4
#include "stdafx.h"
#include "ClientManager.h"
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "GuildManager.h"


void CClientManager::GuildCreate(CPeer * peer, std::unique_ptr<network::GDGuildCreatePacket> p)
{
	sys_log(0, "GuildCreate %u", p->guild_id());

	network::DGOutputPacket<network::DGGuildLoadPacket> pack;
	pack->set_guild_id(p->guild_id());
	ForwardPacket(pack);

	CGuildManager::instance().Load(p->guild_id());
}

void CClientManager::GuildChangeGrade(CPeer* peer, std::unique_ptr<network::GDGuildChangeGradePacket> p)
{
	sys_log(0, "GuildChangeGrade %u %u", p->guild_id(), p->grade());

	network::DGOutputPacket<network::DGGuildChangeGradePacket> pack;
	pack->set_guild_id(p->guild_id());
	pack->set_grade(p->grade());
	ForwardPacket(pack);
}

void CClientManager::GuildAddMember(CPeer* peer, std::unique_ptr<network::GDGuildAddMemberPacket> p)
{
	CGuildManager::instance().TouchGuild(p->guild_id());
	sys_log(0, "GuildAddMember %u %u", p->guild_id(), p->pid());

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery), 
			"REPLACE INTO guild_member VALUES(%u, %u, %d, 0, 0)",
			p->pid(), p->guild_id(), p->grade());

	std::auto_ptr<SQLMsg> pmsg_insert(CDBManager::instance().DirectQuery(szQuery));

	snprintf(szQuery, sizeof(szQuery), 
			"SELECT pid, grade, is_general, offer, level, job, name FROM guild_member, player WHERE guild_id = %u and pid = id and pid = %u", p->guild_id(), p->pid());

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiNumRows == 0)
	{
		sys_err("Query failed when getting guild member data %s", pmsg->stQuery.c_str());
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

	if (!row[0] || !row[1])
		return;

	network::DGOutputPacket<network::DGGuildAddMemberPacket> pack;
	pack->set_guild_id(p->guild_id());
	pack->set_pid(std::stoi(row[0]));
	pack->set_grade(std::stoi(row[1]));
	pack->set_is_general(std::stoi(row[2]));
	pack->set_offer(std::stoi(row[3]));
	pack->set_level(std::stoi(row[4]));
	pack->set_job(std::stoi(row[5]));
	pack->set_name(row[6]);

	ForwardPacket(pack);
}

void CClientManager::GuildRemoveMember(CPeer* peer, std::unique_ptr<network::GDGuildRemoveMemberPacket> p)
{
	sys_log(0, "GuildRemoveMember %u %u", p->guild_id(), p->pid());

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_member WHERE pid=%u and guild_id=%u", p->pid(), p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO quest (dwPID, szName, szState, lValue) VALUES(%u, 'guild_manage', 'withdraw_time', %u)", p->pid(), (DWORD) GetCurrentTime());
	CDBManager::instance().AsyncQuery(szQuery);

	network::DGOutputPacket<network::DGGuildRemoveMemberPacket> pack;
	pack->set_guild_id(p->guild_id());
	pack->set_pid(p->pid());
	ForwardPacket(pack);
}

void CClientManager::GuildSkillUpdate(CPeer* peer, std::unique_ptr<network::GDGuildSkillUpdatePacket> p)
{
	sys_log(0, "GuildSkillUpdate %d", p->amount());

	network::DGOutputPacket<network::DGGuildSkillUpdatePacket> pack;
	pack->set_guild_id(p->guild_id());
	pack->set_amount(p->amount());
	pack->set_skill_point(p->skill_point());
	pack->set_save(p->save());
	for (auto& skill_level : p->skill_levels())
		pack->add_skill_levels(skill_level);
	ForwardPacket(pack);
}

void CClientManager::GuildExpUpdate(CPeer* peer, std::unique_ptr<network::GDGuildExpUpdatePacket> p)
{
	sys_log(0, "GuildExpUpdate %d", p->amount());

	network::DGOutputPacket<network::DGGuildExpUpdatePacket> pack;
	pack->set_guild_id(p->guild_id());
	pack->set_amount(p->amount());
	ForwardPacket(pack, 0, peer);
}

void CClientManager::GuildChangeMemberData(CPeer* peer, std::unique_ptr<network::GDGuildChangeMemberDataPacket> p)
{
	sys_log(0, "GuildChangeMemberData %u %u %d %d", p->pid(), p->offer(), p->level(), p->grade());

	network::DGOutputPacket<network::DGGuildChangeMemberDataPacket> pack;
	pack->set_guild_id(p->guild_id());
	pack->set_pid(p->pid());
	pack->set_offer(p->offer());
	pack->set_level(p->level());
	pack->set_grade(p->grade());
	ForwardPacket(pack, 0, peer);
}

void CClientManager::GuildDisband(CPeer* peer, std::unique_ptr<network::GDGuildDisbandPacket> p)
{
	sys_log(0, "GuildDisband %u", p->guild_id());

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild WHERE id=%u", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_grade WHERE guild_id=%u", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO quest (dwPID, szName, szState, lValue) SELECT pid, 'guild_manage', 'withdraw_time', %u FROM guild_member WHERE guild_id = %u", (DWORD) GetCurrentTime(), p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_member WHERE guild_id=%u", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);
	
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_comment WHERE guild_id=%u", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);
	
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_safebox WHERE guild_id=%u", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);
	
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_safebox_log WHERE guild_id=%u", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE owner_id=%u and window='GUILD_SAFEBOX'", p->guild_id());
	CDBManager::instance().AsyncQuery(szQuery);

	network::DGOutputPacket<network::DGGuildDisbandPacket> pack;
	pack->set_guild_id(p->guild_id());
	ForwardPacket(pack);
}

const char* __GetWarType(int n)
{
	switch (n)
	{
		case 0 :
			return "패왕";
		case 1 :
			return "맹장";
		case 2 :
			return "수호";
		default :
			return "없는 번호";
	}
}

void CClientManager::GuildWar(CPeer* peer, std::unique_ptr<network::GDGuildWarPacket> p)
{
	network::DGOutputPacket<network::DGGuildWarPacket> pack;
	pack->set_guild_from(p->guild_from());
	pack->set_guild_to(p->guild_to());
	pack->set_initial_score(p->initial_score());
	pack->set_type(p->type());
	pack->set_war(p->war());
	pack->set_war_price(p->war_price());

	switch (p->war())
	{
		case GUILD_WAR_SEND_DECLARE:
			sys_log(0, "GuildWar: GUILD_WAR_SEND_DECLARE type(%s) guild(%d - %d)",  __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().AddDeclare(p->type(), p->guild_from(), p->guild_to());
			break;

		case GUILD_WAR_REFUSE:
			sys_log(0, "GuildWar: GUILD_WAR_REFUSE type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().RemoveDeclare(p->guild_from(), p->guild_to());
			break;
			/*
			   case GUILD_WAR_WAIT_START:
			   CGuildManager::instance().RemoveDeclare(p->dwGuildFrom, p->dwGuildTo);

			   if (!CGuildManager::instance().WaitStart(p))
			   p->bWar = GUILD_WAR_CANCEL;

			   break;
			   */

		case GUILD_WAR_WAIT_START:
			sys_log(0, "GuildWar: GUILD_WAR_WAIT_START type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
		case GUILD_WAR_RESERVE:	// 길드전 예약
			if (p->war() != GUILD_WAR_WAIT_START)
				sys_log(0, "GuildWar: GUILD_WAR_RESERVE type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().RemoveDeclare(p->guild_from(), p->guild_to());

			if (!CGuildManager::instance().ReserveWar(std::move(p)))
				pack->set_war(GUILD_WAR_CANCEL);
			else
				pack->set_war(GUILD_WAR_RESERVE);

			break;

		case GUILD_WAR_ON_WAR:		// 길드전을 시작 시킨다. (필드전은 바로 시작 됨)
			sys_log(0, "GuildWar: GUILD_WAR_ON_WAR type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().RemoveDeclare(p->guild_from(), p->guild_to());
			CGuildManager::instance().StartWar(p->type(), p->guild_from(), p->guild_to());
			break;

		case GUILD_WAR_OVER:		// 길드전 정상 종료
			sys_log(0, "GuildWar: GUILD_WAR_OVER type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().RecvWarOver(p->guild_from(), p->guild_to(), p->type(), p->war_price());
			break;

		case GUILD_WAR_END:		// 길드전 비정상 종료
			sys_log(0, "GuildWar: GUILD_WAR_END type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().RecvWarEnd(p->guild_from(), p->guild_to());
			return; // NOTE: RecvWarEnd에서 패킷을 보내므로 따로 브로드캐스팅 하지 않는다.

		case GUILD_WAR_CANCEL :
			sys_log(0, "GuildWar: GUILD_WAR_CANCEL type(%s) guild(%d - %d)", __GetWarType(p->type()), p->guild_from(), p->guild_to());
			CGuildManager::instance().CancelWar(p->guild_from(), p->guild_to());
			break;
	}

	ForwardPacket(pack);
}

void CClientManager::GuildWarScore(CPeer* peer, std::unique_ptr<network::GDGuildWarScorePacket> p)
{
	CGuildManager::instance().UpdateScore(p->guild_gain_point(), p->guild_opponent(), p->score(), p->bet_score());
}

void CClientManager::GuildChangeLadderPoint(std::unique_ptr<network::GDGuildChangeLadderPointPacket> p)
{
	sys_log(0, "GuildChangeLadderPoint Recv %u %d", p->guild_id(), p->change());
	CGuildManager::instance().ChangeLadderPoint(p->guild_id(), p->change());
}

void CClientManager::GuildUseSkill(std::unique_ptr<network::GDGuildUseSkillPacket> p)
{
	sys_log(0, "GuildUseSkill Recv %u %d", p->guild_id(), p->skill_vnum());
	CGuildManager::instance().UseSkill(p->guild_id(), p->skill_vnum(), p->cooltime());
	SendGuildSkillUsable(p->guild_id(), p->skill_vnum(), false);
}

void CClientManager::SendGuildSkillUsable(DWORD guild_id, DWORD dwSkillVnum, bool bUsable)
{
	sys_log(0, "SendGuildSkillUsable Send %u %d %s", guild_id, dwSkillVnum, bUsable?"true":"false");

	DGOutputPacket<DGGuildSkillUsableChangePacket> p;
	p->set_guild_id(guild_id);
	p->set_skill_vnum(dwSkillVnum);
	p->set_usable(bUsable);

	ForwardPacket(p);
}

void CClientManager::GuildChangeMaster(std::unique_ptr<network::GDGuildReqChangeMasterPacket> p)
{
	if (CGuildManager::instance().ChangeMaster(p->guild_id(), p->id_from(), p->id_to()) == true)
	{
		DGOutputPacket<DGGuildChangeMasterPacket> packet;
		packet->set_guild_id(p->guild_id());

		ForwardPacket(packet);
	}
}

