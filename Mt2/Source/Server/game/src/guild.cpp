#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "packet.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "db.h"
#include "guild.h"
#include "guild_manager.h"
#include "affect.h"
#include "p2p.h"
#include "questmanager.h"
#include "building.h"
#include "log.h"

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

#define LEADER_GRADE_NAME "±æµåÀå"
#define MEMBER_GRADE_NAME "±æµå¿ø"

CGuild::CGuild(TGuildCreateParameter & cp)
#ifdef __GUILD_SAFEBOX__
 : m_SafeBox(this)
#endif
{
	Initialize();

	m_general_count = 0;

	m_iMemberCountBonus = 0;
	m_data.bEmpire = cp.master->GetEmpire();
	strlcpy(m_data.name, cp.name, sizeof(m_data.name));
	m_data.master_pid = cp.master->GetPlayerID();
	strlcpy(m_data.grade_array[0].grade_name, LEADER_GRADE_NAME, sizeof(m_data.grade_array[0].grade_name));
	m_data.grade_array[0].auth_flag = GUILD_AUTH_ADD_MEMBER | GUILD_AUTH_REMOVE_MEMBER | GUILD_AUTH_NOTICE | GUILD_AUTH_USE_SKILL;
#ifdef __GUILD_SAFEBOX__
	m_data.grade_array[0].auth_flag |= GUILD_AUTH_SAFEBOX_ITEM_GIVE | GUILD_AUTH_SAFEBOX_ITEM_TAKE | GUILD_AUTH_SAFEBOX_GOLD_GIVE | GUILD_AUTH_SAFEBOX_GOLD_TAKE;
#endif
#ifdef __MELEY_LAIR_DUNGEON__
	m_data.grade_array[0].auth_flag |= GUILD_AUTH_MELEY_ALLOW;
#endif

	for (int i = 1; i < GUILD_GRADE_COUNT; ++i)
	{
		strlcpy(m_data.grade_array[i].grade_name, MEMBER_GRADE_NAME, sizeof(m_data.grade_array[i].grade_name));
		m_data.grade_array[i].auth_flag = 0;
	}

	std::auto_ptr<SQLMsg> pmsg (DBManager::instance().DirectQuery(
				"INSERT INTO guild(name, master, sp, level, exp, skill_point, skill, win, draw, loss) "
				"VALUES('%s', %u, 1000, 1, 0, 0, '\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0', "
				"'\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0', '\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0', '\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0')", 
		CGuildManager::instance().EscapedGuildName(m_data.name), m_data.master_pid));

	// TODO if error occur?
	m_data.guild_id = pmsg->Get()->uiInsertID;

	for (int i = 0; i < GUILD_GRADE_COUNT; ++i)
	{
		DBManager::instance().Query("INSERT INTO guild_grade VALUES(%u, %d, '%s', %d)",
				m_data.guild_id, 
				i + 1, 
				m_data.grade_array[i].grade_name, 
				m_data.grade_array[i].auth_flag);
	}

	ComputeGuildPoints();
	m_data.power	= m_data.max_power;
	m_data.ladder_point	= 0;

	network::GDOutputPacket<network::GDGuildCreatePacket> create_pack;
	create_pack->set_guild_id(GetID());
	db_clientdesc->DBPacket(create_pack);

	network::GDOutputPacket<network::GDGuildSkillUpdatePacket> skill_pack;
	skill_pack->set_guild_id(GetID());
	for (int i = 0; i < GUILD_SKILL_COUNT; ++i)
		skill_pack->add_skill_levels(0);

	db_clientdesc->DBPacket(skill_pack);

	{
		network::GCOutputPacket<network::GCGuildNamePacket> name_pack;
		auto name_ptr = name_pack->add_names();
		name_ptr->set_guild_id(GetID());
		name_ptr->set_name(GetName());
		CHARACTER_MANAGER::instance().for_each_pc([&name_pack](LPCHARACTER ch) {
			ch->GetDesc()->Packet(name_pack);
		});
	}

	RequestAddMember(cp.master, GUILD_LEADER_GRADE);
}

void CGuild::Initialize()
{
	memset(&m_data, 0, sizeof(m_data));
	m_data.level = 1;

	for (int i = 0; i < GUILD_SKILL_COUNT; ++i)
		abSkillUsable[i] = true;

	m_iMemberCountBonus = 0;
	m_iGuildPostCommentPulse = 0;
}

CGuild::~CGuild()
{
}

void CGuild::RequestAddMember(LPCHARACTER ch, int grade)
{
	if (ch->GetGuild())
		return;

	network::GDOutputPacket<network::GDGuildAddMemberPacket> gd;

	if (m_member.find(ch->GetPlayerID()) != m_member.end())
	{
		sys_err("Already a member in guild %s[%d]", ch->GetName(), ch->GetPlayerID());
		return;
	}

	gd->set_pid(ch->GetPlayerID());
	gd->set_guild_id(GetID());
	gd->set_grade(grade);

	db_clientdesc->DBPacket(gd);
}

void CGuild::AddMember(DWORD pid, BYTE grade, bool is_general, BYTE job, BYTE level, DWORD offer, const char* name)
{
	TGuildMemberContainer::iterator it;

	if ((it = m_member.find(pid)) == m_member.end())
		m_member.insert(std::make_pair(pid, TGuildMember(pid, grade, is_general, job, level, offer, name)));
	else
	{
		TGuildMember & r_gm = it->second;
		r_gm.pid = pid;
		r_gm.grade = grade;
		r_gm.job = job;
		r_gm.offer_exp = offer;
		r_gm.is_general = is_general;
	}

	CGuildManager::instance().Link(pid, this);

	SendListOneToAll(pid);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	sys_log(0, "GUILD: AddMember PID %u, grade %u, job %u, level %u, offer %u, name %s ptr %p",
			pid, grade, job, level, offer, name, get_pointer(ch));

	if (ch)
	{
#ifdef __MELEY_LAIR_DUNGEON__
		ch->SetQuestFlag("meleyliar_zone.next_entry", get_global_time() + (60*60*3));
#endif
		LoginMember(ch);
	}
	else
		P2PLoginMember(pid);
}

bool CGuild::RequestRemoveMember(DWORD pid)
{
	TGuildMemberContainer::iterator it;

	if ((it = m_member.find(pid)) == m_member.end())
		return false;

	if (it->second.grade == GUILD_LEADER_GRADE)
		return false;

	network::GDOutputPacket<network::GDGuildRemoveMemberPacket> gd_guild;
	gd_guild->set_guild_id(GetID());
	gd_guild->set_pid(pid);

	db_clientdesc->DBPacket(gd_guild);
	return true;
}

bool CGuild::RemoveMember(DWORD pid)
{
	sys_log(0, "Receive Guild P2P RemoveMember");
	TGuildMemberContainer::iterator it;

	if ((it = m_member.find(pid)) == m_member.end())
		return false;

	if (it->second.grade == GUILD_LEADER_GRADE)
		return false;

	if (it->second.is_general)
		m_general_count--;

	m_member.erase(it);
	SendOnlineRemoveOnePacket(pid);

	CGuildManager::instance().Unlink(pid);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (ch)
	{
		//GuildRemoveAffect(ch);
#ifdef __MELEY_LAIR_DUNGEON__
		MeleyLair::CMgr::instance().MemberRemoved(ch, this);
#endif
		m_memberOnline.erase(ch);
		ch->SetGuild(NULL);

#ifdef __GUILD_SAFEBOX__
		GetSafeBox().CloseSafebox(ch);
		GetSafeBox().SendEnableInformation(ch);
#endif

		LogManager::instance().CharLog(ch, 0, "GUILD_LEAVE", GetName());
	}

	return true;
}

void CGuild::P2PLoginMember(DWORD pid)
{
	if (m_member.find(pid) == m_member.end())
	{
		sys_err("GUILD [%d] is not a memeber of guild.", pid);
		return;
	}

	m_memberP2POnline.insert(pid);

	// Login event occur + Send List
	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		SendLoginPacket(*it, pid);
}

void CGuild::LoginMember(LPCHARACTER ch)
{
	if (m_member.find(ch->GetPlayerID()) == m_member.end())
	{
		sys_err("GUILD %s[%d] is not a memeber of guild.", ch->GetName(), ch->GetPlayerID());
		return;
	}

	ch->SetGuild(this);

	// Login event occur + Send List
	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		SendLoginPacket(*it, ch);

	m_memberOnline.insert(ch);

	SendAllGradePacket(ch);
	SendGuildInfoPacket(ch);
	SendListPacket(ch);
	SendSkillInfoPacket(ch);
	SendEnemyGuild(ch);

	//GuildUpdateAffect(ch);

#ifdef __GUILD_SAFEBOX__
	GetSafeBox().SendEnableInformation(ch);
	// ch->tchat("Guild Safebox Enabled: %d", GetSafeBox().HasSafebox());
#endif
}

void CGuild::P2PLogoutMember(DWORD pid)
{
	if (m_member.find(pid)==m_member.end())
	{
		sys_err("GUILD [%d] is not a memeber of guild.", pid);
		return;
	}

	m_memberP2POnline.erase(pid);

	// Logout event occur
	TGuildMemberOnlineContainer::iterator it;
	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
	{
		SendLogoutPacket(*it, pid);
	}
}

void CGuild::LogoutMember(LPCHARACTER ch)
{
	if (m_member.find(ch->GetPlayerID())==m_member.end())
	{
		sys_err("GUILD %s[%d] is not a memeber of guild.", ch->GetName(), ch->GetPlayerID());
		return;
	}

	//GuildRemoveAffect(ch);

	//ch->SetGuild(NULL);
	m_memberOnline.erase(ch);

	// Logout event occur
	TGuildMemberOnlineContainer::iterator it;
	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
	{
		SendLogoutPacket(*it, ch);
	}
}

void CGuild::SendOnlineRemoveOnePacket(DWORD pid)
{
	network::GCOutputPacket<network::GCGuildRemovePacket> pack;
	pack->set_pid(pid);

	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(pack);
	}
}

void CGuild::SendAllGradePacket(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();
	if (!d)
		return;

	network::GCOutputPacket<network::GCGuildGradePacket> pack;

	for (int i = 0; i < GUILD_GRADE_COUNT; i++)
	{
		auto grade = pack->add_grades();
		grade->set_index(i + 1);
		grade->set_name(m_data.grade_array[i].grade_name);
		grade->set_auth_flag(m_data.grade_array[i].auth_flag);
	}

	d->Packet(pack);
}

void CGuild::SendListOneToAll(LPCHARACTER ch)
{
	SendListOneToAll(ch->GetPlayerID());
}

void CGuild::EncodeOneMemberListPacket(network::TGuildMemberInfo* member, const TGuildMember& member_info)
{
	member->set_pid(member_info.pid);
	member->set_grade(member_info.grade);
	member->set_is_general(member_info.is_general);
	member->set_job(member_info.job);
	member->set_level(member_info.level);
	member->set_offer(member_info.offer_exp);
	member->set_name(member_info.name);
}

void CGuild::SendListOneToAll(DWORD pid)
{
	TGuildMemberContainer::iterator cit = m_member.find(pid);
	if (cit == m_member.end())
		return;

	network::GCOutputPacket<network::GCGuildMemberListPacket> pack;
	EncodeOneMemberListPacket(pack->add_members(), cit->second);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();
		if (!d)
			continue;

		d->Packet(pack);
	}
}

void CGuild::SendListPacket(LPCHARACTER ch)
{
	LPDESC d;
	if (!(d = ch->GetDesc()))
		return;

	network::GCOutputPacket<network::GCGuildMemberListPacket> pack;

	for (TGuildMemberContainer::iterator it = m_member.begin(); it != m_member.end(); ++it)
		EncodeOneMemberListPacket(pack->add_members(), it->second);

	d->Packet(pack);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		SendLoginPacket(ch, *it);
	}

	for (TGuildMemberP2POnlineContainer::iterator it = m_memberP2POnline.begin(); it != m_memberP2POnline.end(); ++it)
	{
		SendLoginPacket(ch, *it);
	}
}

void CGuild::QueryLastPlayedlist(LPCHARACTER ch)
{
	if (!ch)
		return;

	std::string query = "SELECT id,IFNULL(UNIX_TIMESTAMP(last_play),0) FROM player WHERE id in (";

	for (auto it = m_member.begin(); it != m_member.end();)
	{
		query += std::to_string(it->second.pid);

		if (++it != m_member.end())
			query += ",";
	}
	query += ")";
	DBManager::instance().ReturnQuery(QID_GUILD_LOAD_MEMBER_LASTPLAY, ch->GetPlayerID(), NULL, query.c_str());
}

void CGuild::SendLastplayedListPacket(LPCHARACTER ch, SQLMsg * pMsg)
{
	if (!ch)
		return;

	network::GCOutputPacket<network::GCGuildMemberLastPlayedPacket> p;

	for (uint i = 0; i < pMsg->Get()->uiNumRows; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		if (!str_is_number(row[0]) || !str_is_number(row[1]))
			continue;

		auto info = p->add_members();
		info->set_pid(strtoul(row[0], nullptr, 10));
		info->set_timestamp(strtoul(row[1], nullptr, 10));
	}

	if (ch->GetDesc())
		ch->GetDesc()->Packet(p);
}

void CGuild::SendLoginPacket(LPCHARACTER ch, LPCHARACTER chLogin)
{
	SendLoginPacket(ch, chLogin->GetPlayerID());
}

void CGuild::SendLoginPacket(LPCHARACTER ch, DWORD pid)
{
	if (!ch->GetDesc())
		return;

	network::GCOutputPacket<network::GCGuildLoginPacket> pack;
	pack->set_pid(pid);

	ch->GetDesc()->Packet(pack);
}

void CGuild::SendLogoutPacket(LPCHARACTER ch, LPCHARACTER chLogout)
{
	SendLogoutPacket(ch, chLogout->GetPlayerID());
}

void CGuild::SendLogoutPacket(LPCHARACTER ch, DWORD pid)
{
	if (!ch->GetDesc())
		return;

	network::GCOutputPacket<network::GCGuildLogoutPacket> pack;
	pack->set_pid(pid);

	ch->GetDesc()->Packet(pack);
}

void CGuild::LoadGuildMemberData(SQLMsg* pmsg)
{
	if (pmsg->Get()->uiNumRows == 0)
		return;

	m_general_count = 0;

	m_member.clear();

	for (uint i = 0; i < pmsg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

		DWORD pid = strtoul(row[0], (char**) NULL, 10);
		BYTE grade = (BYTE) strtoul(row[1], (char**) NULL, 10);
		BYTE is_general = 0;

		if (row[2] && *row[2] == '1')
			is_general = 1;

		DWORD offer = strtoul(row[3], (char**) NULL, 10);
		BYTE level = (BYTE)strtoul(row[4], (char**) NULL, 10);
		BYTE job = (BYTE)strtoul(row[5], (char**) NULL, 10);
		char * name = row[6];

		if (is_general)
			m_general_count++;

		m_member.insert(std::make_pair(pid, TGuildMember(pid, grade, is_general, job, level, offer, name)));
		CGuildManager::instance().Link(pid, this);
	}
}

void CGuild::LoadGuildGradeData(SQLMsg* pmsg)
{
	/*
	// 15°³ ¾Æ´Ò °¡´É¼º Á¸Àç
	if (pmsg->Get()->iNumRows != 15)
	{
		sys_err("Query failed: getting guild grade data. GuildID(%d)", GetID());
		return;
	}
	*/
	for (uint i = 0; i < pmsg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		BYTE grade = 0;
		str_to_number(grade, row[0]);
		char * name = row[1];
		DWORD auth = strtoul(row[2], NULL, 10);

		if (grade >= 1 && grade <= 15)
		{
			//sys_log(0, "GuildGradeLoad %s", name);
			strlcpy(m_data.grade_array[grade-1].grade_name, name, sizeof(m_data.grade_array[grade-1].grade_name));
			m_data.grade_array[grade-1].auth_flag = auth;
		}
	}
}
void CGuild::LoadGuildData(SQLMsg* pmsg)
{
	if (pmsg->Get()->uiNumRows == 0)
	{
		sys_err("Query failed: getting guild data %s", pmsg->stQuery.c_str());
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
	m_data.master_pid = strtoul(row[0], (char **)NULL, 10);
	m_data.level = (BYTE)strtoul(row[1], (char **)NULL, 10);
	m_data.exp = strtoul(row[2], (char **)NULL, 10);
	strlcpy(m_data.name, row[3], sizeof(m_data.name));

	m_data.skill_point = (BYTE) strtoul(row[4], (char **) NULL, 10);
	if (row[5])
		thecore_memcpy(m_data.abySkill, row[5], sizeof(BYTE) * GUILD_SKILL_COUNT);
	else
		memset(m_data.abySkill, 0, sizeof(BYTE) * GUILD_SKILL_COUNT);

	m_data.power = MAX(0, strtoul(row[6], (char **) NULL, 10));

	str_to_number(m_data.ladder_point, row[7]);

	if (m_data.ladder_point < 0)
		m_data.ladder_point = 0;

	if (row[8])
		thecore_memcpy(m_data.win, row[8], sizeof(m_data.win));
	else
		memset(m_data.win, 0, sizeof(m_data.win));

	if (row[9])
		thecore_memcpy(m_data.draw, row[9], sizeof(m_data.draw));
	else
		memset(m_data.draw, 0, sizeof(m_data.draw));

	if (row[10])
		thecore_memcpy(m_data.loss, row[10], sizeof(m_data.loss));
	else
		memset(m_data.loss, 0, sizeof(m_data.loss));

	str_to_number(m_data.gold, row[11]);
	str_to_number(m_data.bEmpire, row[12]);
#ifdef __DUNGEON_FOR_GUILD__
	str_to_number(m_data.dungeon_ch, row[13]);
	str_to_number(m_data.dungeon_map, row[14]);
	str_to_number(m_data.dungeon_cooldown, row[15]);
#endif

	ComputeGuildPoints();
}

void CGuild::Load(DWORD guild_id)
{
	Initialize();

	m_data.guild_id = guild_id;

	DBManager::instance().ReturnQuery(QID_GUILD_LOAD_DATA, m_data.guild_id, NULL, 
		"SELECT a.master, a.level, a.exp, a.name, a.skill_point, a.skill, a.sp, a.ladder_point, a.win, a.draw, a.loss, a.gold, b.empire"
#ifdef __DUNGEON_FOR_GUILD__
		", a.dungeon_ch, a.dungeon_map, a.dungeon_cooldown"
#endif
		" FROM guild a, player_index b WHERE a.id = %u and (b.pid1 = a.master or b.pid2 = a.master or b.pid3 = a.master"
		" or b.pid4 = a.master) LIMIT 1", m_data.guild_id);
	/*DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::LoadGuildData), this),
		"SELECT a.master, a.level, a.exp, a.name, a.skill_point, a.skill, a.sp, a.ladder_point, a.win, a.draw, a.loss, a.gold, b.empire"
#ifdef __DUNGEON_FOR_GUILD__
		", a.dungeon_ch, a.dungeon_map, a.dungeon_cooldown"
#endif
		" FROM guild a, player_index b WHERE a.id = %u and (b.pid1 = a.master or b.pid2 = a.master or b.pid3 = a.master"
		" or b.pid4 = a.master) LIMIT 1", m_data.guild_id);*/

	sys_log(!test_server, "GUILD: loading guild id %12s %u", m_data.name, guild_id);

	DBManager::instance().ReturnQuery(QID_GUILD_LOAD_GRADE, m_data.guild_id, NULL,
		"SELECT grade, name, auth+0 FROM guild_grade WHERE guild_id = %u", m_data.guild_id);

	DBManager::instance().ReturnQuery(QID_GUILD_LOAD_MEMBER, m_data.guild_id, NULL,
		"SELECT pid, grade, is_general, offer, level, job, name FROM guild_member, player WHERE guild_id = %u and pid = id", guild_id);

	/*DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::LoadGuildGradeData), this), 
			"SELECT grade, name, auth+0 FROM guild_grade WHERE guild_id = %u", m_data.guild_id);

	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::LoadGuildMemberData), this), 
			"SELECT pid, grade, is_general, offer, level, job, name FROM guild_member, player WHERE guild_id = %u and pid = id", guild_id);*/
}

void CGuild::SaveLevel()
{
	DBManager::instance().Query("UPDATE guild SET level=%d, exp=%u, skill_point=%d WHERE id = %u", m_data.level,m_data.exp, m_data.skill_point,m_data.guild_id);
}

void CGuild::SendDBSkillUpdate(int amount)
{
	network::GDOutputPacket<network::GDGuildSkillUpdatePacket> guild_skill;
	guild_skill->set_guild_id(GetID());
	guild_skill->set_amount(amount);
	guild_skill->set_skill_point(m_data.skill_point);

	for (int i = 0; i < GUILD_SKILL_COUNT; ++i)
		guild_skill->add_skill_levels(m_data.abySkill[i]);

	db_clientdesc->DBPacket(guild_skill);
}

void CGuild::SaveSkill()
{
	char text[GUILD_SKILL_COUNT * 2 + 1];

	DBManager::instance().EscapeString(text, sizeof(text), (const char *) m_data.abySkill, sizeof(m_data.abySkill));
	DBManager::instance().Query("UPDATE guild SET sp = %d, skill_point=%d, skill='%s' WHERE id = %u",
			m_data.power, m_data.skill_point, text, m_data.guild_id);
}

TGuildMember* CGuild::GetMember(DWORD pid)
{
	TGuildMemberContainer::iterator it = m_member.find(pid);
	if (it==m_member.end())
		return NULL;

	return &it->second;
}

DWORD CGuild::GetMemberPID(const std::string& strName)
{
	for ( TGuildMemberContainer::iterator iter = m_member.begin();
			iter != m_member.end(); iter++ )
	{
		if ( iter->second.name == strName ) return iter->first;
	}

	return 0;
}

void CGuild::P2PUpdateGrade(SQLMsg* pmsg)
{
	if (pmsg->Get()->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		
		int grade = 0;
		const char* name = row[1];
		int auth = 0;

		str_to_number(grade, row[0]);
		str_to_number(auth, row[2]);

		if (grade <= 0)
			return;

		grade--;

		if (strcmp(m_data.grade_array[grade].grade_name, name))
		{
			strlcpy(m_data.grade_array[grade].grade_name, name, sizeof(m_data.grade_array[grade].grade_name));

			network::GCOutputPacket<network::GCGuildGradeNamePacket> pack;
			pack->set_index(grade + 1);
			pack->set_name(name);

			for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPDESC d = (*it)->GetDesc();

				if (d)
				{
					if (!strcmp(name, LEADER_GRADE_NAME) || !strcmp(name, MEMBER_GRADE_NAME))
						pack->set_name(LC_TEXT(*it, name));

					d->Packet(pack);
				}
			}
		}

		if (m_data.grade_array[grade].auth_flag != auth)
		{
			m_data.grade_array[grade].auth_flag = auth;

			network::GCOutputPacket<network::GCGuildGradeAuthPacket> pack;
			pack->set_index(grade + 1);
			pack->set_auth_flag(auth);

			for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPDESC d = (*it)->GetDesc();
				if (d)
					d->Packet(pack);
			}
		}
	}
}

void CGuild::P2PChangeGrade(BYTE grade)
{
	DBManager::instance().ReturnQuery(QID_GUILD_CHANGE_GRADE, m_data.guild_id, NULL, 
		"SELECT grade, name, auth+0 FROM guild_grade WHERE guild_id = %u and grade = %d", m_data.guild_id, grade);
	/*DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::P2PUpdateGrade),this),
			"SELECT grade, name, auth+0 FROM guild_grade WHERE guild_id = %u and grade = %d", m_data.guild_id, grade);*/
}

void CGuild::ChangeGradeName(BYTE grade, const char* grade_name)
{
	if (grade == 1)
		return;

	if (grade < 1 || grade > 15)
	{
		sys_err("Wrong guild grade value %d", grade);
		return;
	}

	if (strlen(grade_name) > GUILD_GRADE_NAME_MAX_LEN)
		return;

	if (!*grade_name)
		return;

	char text[GUILD_GRADE_NAME_MAX_LEN * 2 + 1];

	DBManager::instance().EscapeString(text, sizeof(text), grade_name, strlen(grade_name));

	DWORD guild_id = GetID();
	DBManager::instance().FuncAfterQuery([guild_id, grade]() {
		network::GDOutputPacket<network::GDGuildChangeGradePacket> pack;
		pack->set_guild_id(guild_id);
		pack->set_grade(grade);
		db_clientdesc->DBPacket(pack);
	}, "UPDATE guild_grade SET name = '%s' where guild_id = %u and grade = %d", text, m_data.guild_id, grade);

	grade--;
	strlcpy(m_data.grade_array[grade].grade_name, grade_name, sizeof(m_data.grade_array[grade].grade_name));

	network::GCOutputPacket<network::GCGuildGradeNamePacket> pack;
	pack->set_index(grade + 1);
	pack->set_name(grade_name);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(pack);
	}
}

void CGuild::ChangeGradeAuth(BYTE grade, WORD auth)
{
	if (grade == 1)
		return;

	if (grade < 1 || grade > 15)
	{
		sys_err("Wrong guild grade value %d", grade);
		return;
	}
	
	DWORD guild_id = GetID();
	DBManager::instance().FuncAfterQuery([guild_id, grade]() {
		network::GDOutputPacket<network::GDGuildChangeGradePacket> pack;
		pack->set_guild_id(guild_id);
		pack->set_grade(grade);
		db_clientdesc->DBPacket(pack);
	}, "UPDATE guild_grade SET auth = %d where guild_id = %u and grade = %d", auth, m_data.guild_id, grade);

	grade--;

	m_data.grade_array[grade].auth_flag=auth;

	network::GCOutputPacket<network::GCGuildGradeAuthPacket> pack;
	pack->set_index(grade + 1);
	pack->set_auth_flag(auth);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(pack);
	}
}

void CGuild::SendGuildInfoPacket(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	network::GCOutputPacket<network::GCGuildInfoPacket> pack;

	pack->set_member_count(GetMemberCount()); 
	pack->set_max_member_count(GetMaxMemberCount());
	pack->set_guild_id(m_data.guild_id);
	pack->set_master_pid(m_data.master_pid);
	pack->set_exp(m_data.exp);
	pack->set_level(m_data.level);
	pack->set_name(m_data.name);
	pack->set_gold(m_data.gold);
	pack->set_has_land(HasLand());

	pack->set_guild_point(m_data.ladder_point);
	pack->set_guild_rank(CGuildManager::instance().GetRank(this));

	for (auto& win : m_data.win)
		pack->add_wins(win);
	for (auto& draw : m_data.draw)
		pack->add_draws(draw);
	for (auto& loss : m_data.loss)
		pack->add_losses(loss);

	sys_log(!test_server, "GMC guild_name %s", m_data.name);
	sys_log(!test_server, "GMC master %d", m_data.master_pid);

	d->Packet(pack);
}

bool CGuild::OfferExp(LPCHARACTER ch, int amount)
{
	TGuildMemberContainer::iterator cit = m_member.find(ch->GetPlayerID());

	if (cit == m_member.end())
		return false;

	if (m_data.exp+amount < m_data.exp)
		return false;

	if (amount < 0)
		return false;

	if (ch->GetExp() < (DWORD) amount)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Á¦°øÇÏ°íÀÚ ÇÏ´Â °æÇèÄ¡°¡ ³²Àº °æÇèÄ¡º¸´Ù ¸¹½À´Ï´Ù."));
		return false;
	}

	if (ch->GetExp() - (DWORD) amount > ch->GetExp())
	{
		sys_err("Wrong guild offer amount %d by %s[%u]", amount, ch->GetName(), ch->GetPlayerID());
		return false;
	}

	ch->PointChange(POINT_EXP, -amount);

	network::GDOutputPacket<network::GDGuildExpUpdatePacket> guild_exp;
	guild_exp->set_guild_id(GetID());
	guild_exp->set_amount(amount / 100);
	db_clientdesc->DBPacket(guild_exp);
	GuildPointChange(POINT_EXP, amount / 100, true);

	cit->second.offer_exp += amount / 100;

	network::GCOutputPacket<network::GCGuildMemberListPacket> pack;
	EncodeOneMemberListPacket(pack->add_members(), cit->second);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();
		if (d)
			d->Packet(pack);
	}

	SaveMember(ch->GetPlayerID());

	network::GDOutputPacket<network::GDGuildChangeMemberDataPacket> gd_guild;
	gd_guild->set_guild_id(GetID());
	gd_guild->set_pid(ch->GetPlayerID());
	gd_guild->set_offer(cit->second.offer_exp);
	gd_guild->set_level(ch->GetLevel());
	gd_guild->set_grade(cit->second.grade);

	db_clientdesc->DBPacket(gd_guild);
	return true;
}

void CGuild::Disband()
{
	sys_log(0, "GUILD: Disband %s:%u", GetName(), GetID());

	//building::CLand* pLand = building::CManager::instance().FindLandByGuild(GetID());
	//if (pLand)
	//pLand->SetOwner(0);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;
		ch->SetGuild(NULL);
		SendOnlineRemoveOnePacket(ch->GetPlayerID());
		ch->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
	}

	for (TGuildMemberContainer::iterator it = m_member.begin(); it != m_member.end(); ++it)
	{
		CGuildManager::instance().Unlink(it->first);
	}

}

bool CGuild::RequestDisband(DWORD pid)
{
	if (m_data.master_pid != pid)
		return false;
	
	network::GDOutputPacket<network::GDGuildDisbandPacket> gd_guild;
	gd_guild->set_guild_id(GetID());
	db_clientdesc->DBPacket(gd_guild);

	// LAND_CLEAR
	building::CManager::instance().ClearLandByGuildID(GetID());
	// END_LAND_CLEAR

	return true;
}

void CGuild::AddComment(LPCHARACTER ch, const std::string& str)
{
	if (str.length() > GUILD_COMMENT_MAX_LEN || str.length() == 0)
		return;

	// Compare last pulse with current pulse and notify the player
	if (m_iGuildPostCommentPulse > thecore_pulse()) {
		int deltaInSeconds = ( ( m_iGuildPostCommentPulse / PASSES_PER_SEC(1) ) - ( thecore_pulse() / PASSES_PER_SEC(1) ) );
		int minutes = deltaInSeconds / 60;
		int seconds = ( deltaInSeconds - ( minutes * 60 ) );

		ch->ChatPacket(CHAT_TYPE_INFO, "You need to wait %02d minutes and %02d seconds before adding new comment!", minutes, seconds);
		return;
	}

	// Check if limit reached
	char szQuery[ 1024 ];
	snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM guild_comment WHERE guild_id = %d", m_data.guild_id);
	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery(szQuery));
	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

	if (pMsg->Get()->uiNumRows)
	{
		// I think its better to put this into int... if some player managed to spam the comments before this exploit fix? It could overflow the BYTE
		int iCommentAmount = 0;
		str_to_number(iCommentAmount, row[0]);

		if (iCommentAmount >= GUILD_COMMENT_LIMIT)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You have reach the limit of 12 comments. Delete a comment for continue.");
			// Now, set the cooldown to some low value, so player cant spam the SELECT query
			m_iGuildPostCommentPulse = thecore_pulse() + PASSES_PER_SEC(30);
			return;
		}
	}

	// Add comment
	auto player_id = ch->GetPlayerID();
	char text[GUILD_COMMENT_MAX_LEN * 2 + 1];
	DBManager::Instance().EscapeString(text, sizeof(text), str.c_str(), str.length());
	DBManager::Instance().FuncAfterQuery(
		[this, player_id] {this->RefreshCommentForce(player_id); },
		"INSERT INTO guild_comment(guild_id, name, notice, content, time) VALUES(%u, '%s', %d, '%s', NOW())",
		m_data.guild_id,
		ch->GetName(),
		(str[0] == '!') ? 1 : 0,
		text);

	// Set cooldown
	m_iGuildPostCommentPulse = thecore_pulse() + PASSES_PER_SEC(GUILD_COMMENT_COOLDOWN);
}

void CGuild::DeleteComment(LPCHARACTER ch, DWORD comment_id)
{
	SQLMsg * pmsg;

	if (GetMember(ch->GetPlayerID())->grade == GUILD_LEADER_GRADE)
		pmsg = DBManager::instance().DirectQuery("DELETE FROM guild_comment WHERE id = %u AND guild_id = %u", comment_id, m_data.guild_id);
	else
		pmsg = DBManager::instance().DirectQuery("DELETE FROM guild_comment WHERE id = %u AND guild_id = %u AND name = '%s'", comment_id, m_data.guild_id, ch->GetName());

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »èÁ¦ÇÒ ¼ö ¾ø´Â ±ÛÀÔ´Ï´Ù."));
	else
		RefreshCommentForce(ch->GetPlayerID());

	M2_DELETE(pmsg);
}

void CGuild::RefreshComment(LPCHARACTER ch)
{
	RefreshCommentForce(ch->GetPlayerID());
}

void CGuild::RefreshCommentForce(DWORD player_id)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(player_id);
	if (ch == NULL) {
		return;
	}

	std::auto_ptr<SQLMsg> pmsg (DBManager::instance().DirectQuery("SELECT id, name, content FROM guild_comment WHERE guild_id = %u ORDER BY notice DESC, id DESC LIMIT %d", m_data.guild_id, GUILD_COMMENT_MAX_COUNT));

	BYTE count = pmsg->Get()->uiNumRows;

	LPDESC d = ch->GetDesc();

	if (!d) 
		return;

	network::GCOutputPacket<network::GCGuildCommentsPacket> pack;
	for (uint i = 0; i < pmsg->Get()->uiNumRows; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

		auto comment = pack->add_comments();
		comment->set_id(std::stoi(row[0]));
		comment->set_name(row[1]);
		comment->set_message(row[2]);
	}

	d->Packet(pack);
}

bool CGuild::ChangeMemberGeneral(DWORD pid, BYTE is_general)
{
	if (is_general && GetGeneralCount() >= GetMaxGeneralCount())
		return false;

	TGuildMemberContainer::iterator it = m_member.find(pid);
	if (it == m_member.end())
	{
		return true;
	}

	is_general = is_general?1:0;

	if (it->second.is_general == is_general)
		return true;

	if (is_general)
		++m_general_count;
	else
		--m_general_count;

	it->second.is_general = is_general;

	TGuildMemberOnlineContainer::iterator itOnline = m_memberOnline.begin();

	network::GCOutputPacket<network::GCGuildChangeMemberGeneralPacket> pack;
	pack->set_pid(pid);
	pack->set_flag(is_general);

	while (itOnline != m_memberOnline.end())
	{
		LPDESC d = (*(itOnline++))->GetDesc();

		if (!d)
			continue;

		d->Packet(pack);
	}

	SaveMember(pid);
	return true;
}

void CGuild::ChangeMemberGrade(DWORD pid, BYTE grade)
{
	if (grade == 1)
		return;

	TGuildMemberContainer::iterator it = m_member.find(pid);

	if (it == m_member.end())
		return;

	it->second.grade = grade;

	TGuildMemberOnlineContainer::iterator itOnline = m_memberOnline.begin();

	network::GCOutputPacket<network::GCGuildChangeMemberGradePacket> pack;
	pack->set_pid(pid);
	pack->set_grade(grade);

	while (itOnline != m_memberOnline.end())
	{
		LPDESC d = (*(itOnline++))->GetDesc();

		if (!d)
			continue;

		d->Packet(pack);
	}

	SaveMember(pid);

	network::GDOutputPacket<network::GDGuildChangeMemberDataPacket> gd_guild;

	gd_guild->set_guild_id(GetID());
	gd_guild->set_pid(pid);
	gd_guild->set_offer(it->second.offer_exp);
	gd_guild->set_level(it->second.level);
	gd_guild->set_grade(grade);

	db_clientdesc->DBPacket(gd_guild);
}

void CGuild::SkillLevelUp(DWORD dwVnum)
{
	DWORD dwRealVnum = dwVnum - GUILD_SKILL_START;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such guild skill by number %u", dwVnum);
		return;
	}

	if (m_data.abySkill[dwRealVnum] >= pkSk->bMaxLevel)
		return;

	if (m_data.skill_point <= 0)
		return;
	m_data.skill_point --;

	m_data.abySkill[dwRealVnum] ++;

	ComputeGuildPoints();
	SaveSkill();
	SendDBSkillUpdate();

	/*switch (dwVnum)
	  {
	  case GUILD_SKILL_GAHO:
	  {
	  TGuildMemberOnlineContainer::iterator it;

	  for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	  (*it)->PointChange(POINT_DEF_GRADE, 1);
	  }
	  break;
	  case GUILD_SKILL_HIM:
	  {
	  TGuildMemberOnlineContainer::iterator it;

	  for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	  (*it)->PointChange(POINT_ATT_GRADE, 1);
	  }
	  break;
	  }*/

	for_each(m_memberOnline.begin(), m_memberOnline.end(), std::bind1st(std::mem_fun(&CGuild::SendSkillInfoPacket),this));

	sys_log(0, "Guild SkillUp: %s %d level %d type %u", GetName(), pkSk->dwVnum, m_data.abySkill[dwRealVnum], pkSk->dwType);
}

void CGuild::UseSkill(DWORD dwVnum, LPCHARACTER ch, DWORD pid)
{
	LPCHARACTER victim = NULL;

	if (!GetMember(ch->GetPlayerID()) || !HasGradeAuth(GetMember(ch->GetPlayerID())->grade, GUILD_AUTH_USE_SKILL))
		return;

	sys_log(0,"GUILD_USE_SKILL : cname(%s), skill(%d)", ch ? ch->GetName() : "", dwVnum);

	DWORD dwRealVnum = dwVnum - GUILD_SKILL_START;

	if (!ch->CanMove())
		return;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such guild skill by number %u", dwVnum);
		return;
	}

	if (m_data.abySkill[dwRealVnum] == 0)
		return;

	if ((pkSk->dwFlag & SKILL_FLAG_SELFONLY))
	{
		// ÀÌ¹Ì °É·Á ÀÖÀ¸¹Ç·Î »ç¿ëÇÏÁö ¾ÊÀ½.
		if (ch->FindAffect(pkSk->dwVnum))
			return;

		victim = ch;
	}

	if (ch->IsAffectFlag(AFF_REVIVE_INVISIBLE))
		ch->RemoveAffect(AFFECT_REVIVE_INVISIBLE);

	if (ch->IsAffectFlag(AFF_EUNHYUNG))
		ch->RemoveAffect(SKILL_EUNHYUNG);

	double k =1.0*m_data.abySkill[dwRealVnum]/pkSk->bMaxLevel;
	pkSk->SetVar("k", k);
	int iNeededSP = (int) pkSk->kSPCostPoly.Evaluate();

	if (GetSP() < iNeededSP)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ¿ë½Å·ÂÀÌ ºÎÁ·ÇÕ´Ï´Ù. (%d, %d)"), GetSP(), iNeededSP);
		return;
	}

	pkSk->SetVar("k", k);
	int iCooltime = (int) pkSk->kCooldownPoly.Evaluate();

	if (!abSkillUsable[dwRealVnum])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ÄðÅ¸ÀÓÀÌ ³¡³ªÁö ¾Ê¾Æ ±æµå ½ºÅ³À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	{
		network::GDOutputPacket<network::GDGuildUseSkillPacket> p;
		p->set_guild_id(GetID());
		p->set_skill_vnum(pkSk->dwVnum);
		p->set_cooltime(iCooltime);
		db_clientdesc->DBPacket(p);
	}
	abSkillUsable[dwRealVnum] = false;
	//abSkillUsed[dwRealVnum] = true;
	//adwSkillNextUseTime[dwRealVnum] = get_dword_time() + iCooltime * 1000;

	//PointChange(POINT_SP, -iNeededSP);
	//GuildPointChange(POINT_SP, -iNeededSP);

	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> %d ½ºÅ³À» »ç¿ëÇÔ (%d, %d) to %u"), dwVnum, GetSP(), iNeededSP, pid);

	switch (dwVnum)
	{
		case GUILD_SKILL_TELEPORT:
			// ÇöÀç ¼­¹ö¿¡ ÀÖ´Â »ç¶÷À» ¸ÕÀú ½Ãµµ.
			SendDBSkillUpdate(-iNeededSP);
			if ((victim = (CHARACTER_MANAGER::instance().FindByPID(pid))))
				ch->WarpSet(victim->GetX(), victim->GetY());
			else
			{
				if (m_memberP2POnline.find(pid) != m_memberP2POnline.end())
				{
					// ´Ù¸¥ ¼­¹ö¿¡ ·Î±×ÀÎµÈ »ç¶÷ÀÌ ÀÖÀ½ -> ¸Þ½ÃÁö º¸³» ÁÂÇ¥¸¦ ¹Þ¾Æ¿ÀÀÚ
					// 1. A.pid, B.pid ¸¦ »Ñ¸²
					// 2. B.pid¸¦ °¡Áø ¼­¹ö°¡ »Ñ¸°¼­¹ö¿¡°Ô A.pid, ÁÂÇ¥ ¸¦ º¸³¿
					// 3. ¿öÇÁ
					CCI * pcci = P2P_MANAGER::instance().FindByPID(pid);

					if (pcci->bChannel != g_bChannel)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë°¡ %d Ã¤³Î¿¡ ÀÖ½À´Ï´Ù. (ÇöÀç Ã¤³Î %d)"), pcci->bChannel, g_bChannel);
					}
					else
					{
						network::GGOutputPacket<network::GGFindPositionPacket> p;
						p->set_from_pid(ch->GetPlayerID());
						p->set_target_pid(pid);
						p->set_is_gm(ch->IsGM());
						pcci->pkDesc->Packet(p);
						if (test_server) ch->ChatPacket(CHAT_TYPE_PARTY, "sent find position packet for guild teleport");
					}
				}
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë°¡ ¿Â¶óÀÎ »óÅÂ°¡ ¾Æ´Õ´Ï´Ù."));
			}
			break;

			/*case GUILD_SKILL_ACCEL:
			  ch->RemoveAffect(dwVnum);
			  ch->AddAffect(dwVnum, POINT_MOV_SPEED, m_data.abySkill[dwRealVnum]*3, pkSk->dwAffectFlag, (int)pkSk->kDurationPoly.Evaluate(), 0, false);
			  ch->AddAffect(dwVnum, POINT_ATT_SPEED, m_data.abySkill[dwRealVnum]*3, pkSk->dwAffectFlag, (int)pkSk->kDurationPoly.Evaluate(), 0, false);
			  break;*/

		default:
			{
				/*if (ch->GetPlayerID() != GetMasterPID())
				  {
				  ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀå¸¸ ±æµå ½ºÅ³À» »ç¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
				  return;
				  }*/

				if (!UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå ½ºÅ³Àº ±æµåÀü Áß¿¡¸¸ »ç¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
					return;
				}

				SendDBSkillUpdate(-iNeededSP);

				for (itertype(m_memberOnline) it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
				{
					LPCHARACTER victim = *it;
					victim->RemoveAffect(dwVnum);
					ch->ComputeSkill(dwVnum, victim, m_data.abySkill[dwRealVnum]);
				}
			}
			break;
			/*if (!victim)
			  return;

			  ch->ComputeSkill(dwVnum, victim, m_data.abySkill[dwRealVnum]);*/
	}
}

void CGuild::SendSkillInfoPacket(LPCHARACTER ch) const
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	network::GCOutputPacket<network::GCGuildSkillInfoPacket> pack;
	pack->set_skill_point(1);
	pack->set_guild_point(m_data.power);
	pack->set_max_guild_point(m_data.max_power);
	for (int i = 0; i < GUILD_SKILL_COUNT; ++i)
		pack->add_skill_levels(m_data.abySkill[i]);

	d->Packet(pack);
}

void CGuild::ComputeGuildPoints()
{
	m_data.max_power = GUILD_BASE_POWER + (m_data.level-1) * GUILD_POWER_PER_LEVEL;

	m_data.power = MINMAX(0, m_data.power, m_data.max_power);
}

int CGuild::GetSkillLevel(DWORD vnum)
{
	DWORD dwRealVnum = vnum - GUILD_SKILL_START;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return 0;

	return m_data.abySkill[dwRealVnum];
}

/*void CGuild::GuildUpdateAffect(LPCHARACTER ch)
  {
  if (GetSkillLevel(GUILD_SKILL_GAHO))
  ch->PointChange(POINT_DEF_GRADE, GetSkillLevel(GUILD_SKILL_GAHO));

  if (GetSkillLevel(GUILD_SKILL_HIM))
  ch->PointChange(POINT_ATT_GRADE, GetSkillLevel(GUILD_SKILL_HIM));
  }*/

/*void CGuild::GuildRemoveAffect(LPCHARACTER ch)
  {
  if (GetSkillLevel(GUILD_SKILL_GAHO))
  ch->PointChange(POINT_DEF_GRADE, -(int) GetSkillLevel(GUILD_SKILL_GAHO));

  if (GetSkillLevel(GUILD_SKILL_HIM))
  ch->PointChange(POINT_ATT_GRADE, -(int) GetSkillLevel(GUILD_SKILL_HIM));
  }*/

void CGuild::UpdateSkillLevel(BYTE skill_points, BYTE index, BYTE skill_level)
{
	m_data.skill_point = skill_points;
	m_data.abySkill[index] = skill_level;
	ComputeGuildPoints();
}

static DWORD __guild_levelup_exp(int level)
{
	return guild_exp_table2[level];
}

void CGuild::GuildPointChange(BYTE type, int amount, bool save)
{
	switch (type)
	{
		case POINT_SP:
			m_data.power += amount;

			m_data.power = MINMAX(0, m_data.power, m_data.max_power);

			if (save)
			{
				SaveSkill();
			}

			for_each(m_memberOnline.begin(), m_memberOnline.end(), std::bind1st(std::mem_fun(&CGuild::SendSkillInfoPacket),this));
			break;

		case POINT_EXP:
			if (amount < 0 && m_data.exp < (DWORD) - amount)
			{
				m_data.exp = 0;
			}
			else
			{
				m_data.exp += amount;

				while (m_data.exp >= __guild_levelup_exp(m_data.level))
				{

					if (m_data.level < GUILD_MAX_LEVEL)
					{
						m_data.exp -= __guild_levelup_exp(m_data.level);
						++m_data.level;
						++m_data.skill_point;

						if (m_data.level > GUILD_MAX_LEVEL)
							m_data.level = GUILD_MAX_LEVEL;

						ComputeGuildPoints();
						GuildPointChange(POINT_SP, m_data.max_power-m_data.power);

						if (save)
							ChangeLadderPoint(GUILD_LADDER_POINT_PER_LEVEL);

						// NOTIFY_GUILD_EXP_CHANGE
						for_each(m_memberOnline.begin(), m_memberOnline.end(), std::bind1st(std::mem_fun(&CGuild::SendGuildInfoPacket), this));
						// END_OF_NOTIFY_GUILD_EXP_CHANGE
					}

					if (m_data.level == GUILD_MAX_LEVEL)
					{
						m_data.exp = 0;
					}
				}
			}

			network::GCOutputPacket<network::GCGuildChangeExpPacket> pack;
			pack->set_level(GetLevel());
			pack->set_exp(m_data.exp);

			for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPDESC d = (*it)->GetDesc();

				if (d)
					d->Packet(pack);
			}

			if (save)
				SaveLevel();

			break;
	}
}

void CGuild::SkillRecharge()
{
	//GuildPointChange(POINT_SP, m_data.max_power / 2);
	//GuildPointChange(POINT_SP, 10);
}

void CGuild::SaveMember(DWORD pid)
{
	TGuildMemberContainer::iterator it = m_member.find(pid);

	if (it == m_member.end())
		return;

	DBManager::instance().Query(
			"UPDATE guild_member SET grade = %d, offer = %u, is_general = %d WHERE pid = %u and guild_id = %u",
			it->second.grade, it->second.offer_exp, it->second.is_general, pid, m_data.guild_id);
}

void CGuild::LevelChange(DWORD pid, BYTE level)
{
	TGuildMemberContainer::iterator cit = m_member.find(pid);

	if (cit == m_member.end())
		return;

	cit->second.level = level;

	network::GDOutputPacket<network::GDGuildChangeMemberDataPacket> gd_guild;

	gd_guild->set_guild_id(GetID());
	gd_guild->set_pid(pid);
	gd_guild->set_offer(cit->second.offer_exp);
	gd_guild->set_grade(cit->second.grade);
	gd_guild->set_level(level);

	db_clientdesc->DBPacket(gd_guild);

	network::GCOutputPacket<network::GCGuildMemberListPacket> pack;
	EncodeOneMemberListPacket(pack->add_members(), cit->second);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(pack);
	}
}

void CGuild::ChangeMemberData(DWORD pid, DWORD offer, BYTE level, BYTE grade)
{
	TGuildMemberContainer::iterator cit = m_member.find(pid);

	if (cit == m_member.end())
		return;

	cit->second.offer_exp = offer;
	cit->second.level = level;
	cit->second.grade = grade;

	network::GCOutputPacket<network::GCGuildMemberListPacket> pack;
	EncodeOneMemberListPacket(pack->add_members(), cit->second);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(pack);
	}
}

namespace
{
	struct FGuildChat
	{
		const char* c_pszText;

		FGuildChat(const char* c_pszText)
			: c_pszText(c_pszText)
			{}

		void operator()(LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_GUILD, "%s", c_pszText);
		}
	};
}

void CGuild::P2PChat(const char* c_pszText)
{
	std::for_each(m_memberOnline.begin(), m_memberOnline.end(), FGuildChat(c_pszText));
}

void CGuild::Chat(const char* c_pszText)
{
	std::for_each(m_memberOnline.begin(), m_memberOnline.end(), FGuildChat(c_pszText));

	network::GGOutputPacket<network::GGGuildChatPacket> p;
	p->set_guild_id(GetID());
	p->set_message(c_pszText);

	P2P_MANAGER::instance().Send(p);
}

LPCHARACTER CGuild::GetMasterCharacter()
{ 
	return CHARACTER_MANAGER::instance().FindByPID(GetMasterPID()); 
}

int CGuild::GetTotalLevel() const
{
	int total = 0;

	for (itertype(m_member) it = m_member.begin(); it != m_member.end(); ++it)
	{
		total += it->second.level;
	}

	return total;
}

bool CGuild::ChargeSP(LPCHARACTER ch, int iSP)
{
	int gold = iSP * 100;

	if (gold < iSP || ch->GetGold() < gold)
		return false;

	int iRemainSP = m_data.max_power - m_data.power;

	if (iSP > iRemainSP)
	{
		iSP = iRemainSP;
		gold = iSP * 100;
	}

	ch->PointChange(POINT_GOLD, -gold);
	LogManager::instance().MoneyLog(MONEY_LOG_GUILD, 1, -gold);

	SendDBSkillUpdate(iSP);
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> %uÀÇ ¿ë½Å·ÂÀ» È¸º¹ÇÏ¿´½À´Ï´Ù."), iSP);
	}
	return true;
}

void CGuild::SkillUsableChange(DWORD dwSkillVnum, bool bUsable)
{
	DWORD dwRealVnum = dwSkillVnum - GUILD_SKILL_START;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return; 

	abSkillUsable[dwRealVnum] = bUsable;

	// GUILD_SKILL_COOLTIME_BUG_FIX
	sys_log(0, "CGuild::SkillUsableChange(guild=%s, skill=%d, usable=%d)", GetName(), dwSkillVnum, bUsable);
	// END_OF_GUILD_SKILL_COOLTIME_BUG_FIX
}

// GUILD_MEMBER_COUNT_BONUS
void CGuild::SetMemberCountBonus(int iBonus)
{
	m_iMemberCountBonus = iBonus;
	sys_log(0, "GUILD_IS_FULL_BUG : Bonus set to %d(val:%d)", iBonus, m_iMemberCountBonus);
}

void CGuild::BroadcastMemberCountBonus()
{
	network::GGOutputPacket<network::GGGuildSetMemberCountBonusPacket> p;
	p->set_guild_id(GetID());
	p->set_bonus(m_iMemberCountBonus);

	P2P_MANAGER::instance().Send(p);
}

int CGuild::GetMaxMemberCount()
{
	// GUILD_IS_FULL_BUG_FIX
	if ( m_iMemberCountBonus < 0 || m_iMemberCountBonus > 18 )
		m_iMemberCountBonus = 0;
	// END_GUILD_IS_FULL_BUG_FIX

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetMasterPID());

	if ( pPC != NULL )
	{
		if ( pPC->GetFlag("guild.is_unlimit_member") == 1 )
		{
			return INT_MAX;
		}
	}

	int iMemberCountBonus = m_iMemberCountBonus;
	if (GetLevel() == GUILD_MAX_LEVEL)
		iMemberCountBonus += 4;

	return 32 + 6 * (m_data.level-1) + iMemberCountBonus;
}
// END_OF_GUILD_MEMBER_COUNT_BONUS

void CGuild::AdvanceLevel(int iLevel)
{
	if (m_data.level == iLevel)
		return;

	m_data.level = MIN(GUILD_MAX_LEVEL, iLevel);
}

void CGuild::RequestDepositMoney(LPCHARACTER ch, int iGold)
{
	if (false==ch->CanDeposit())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Àá½ÃÈÄ¿¡ ÀÌ¿ëÇØÁÖ½Ê½Ã¿À"));
		return;
	}

	if (ch->GetGold() < iGold)
		return;


	ch->PointChange(POINT_GOLD, -iGold);

	network::GDOutputPacket<network::GDGuildDepositMoneyPacket> pack;
	pack->set_guild_id(GetID());
	pack->set_gold(iGold);
	db_clientdesc->DBPacket(pack);

	char buf[64+1];
	snprintf(buf, sizeof(buf), "%u %s", GetID(), GetName());
	LogManager::instance().CharLog(ch, iGold, "GUILD_DEPOSIT", buf);

	ch->UpdateDepositPulse();
	sys_log(0, "GUILD: DEPOSIT %s:%u player %s[%u] gold %d", GetName(), GetID(), ch->GetName(), ch->GetPlayerID(), iGold);
}

void CGuild::RequestWithdrawMoney(LPCHARACTER ch, int iGold)
{
	if (false==ch->CanDeposit())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Àá½ÃÈÄ¿¡ ÀÌ¿ëÇØÁÖ½Ê½Ã¿À"));
		return;
	}

	if (ch->GetPlayerID() != GetMasterPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå ±Ý°í¿¡¼± ±æµåÀå¸¸ Ãâ±ÝÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}

	if (m_data.gold < iGold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> °¡Áö°í ÀÖ´Â µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return;
	}

	network::GDOutputPacket<network::GDGuildWithdrawMoneyPacket> p;
	p->set_guild_id(GetID());
	p->set_gold(iGold);
	db_clientdesc->DBPacket(p);

	ch->UpdateDepositPulse();
}

void CGuild::RecvMoneyChange(int iGold)
{
	m_data.gold = iGold;

	network::GCOutputPacket<network::GCGuildMoneyChangePacket> p;
	p->set_gold(iGold);

	for (auto it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;
		LPDESC d = ch->GetDesc();
		d->Packet(p);
	}
}

void CGuild::RecvWithdrawMoneyGive(int iChangeGold)
{
	LPCHARACTER ch = GetMasterCharacter();

	if (ch)
	{
		ch->PointChange(POINT_GOLD, iChangeGold);
		sys_log(0, "GUILD: WITHDRAW %s:%u player %s[%u] gold %d", GetName(), GetID(), ch->GetName(), ch->GetPlayerID(), iChangeGold);
	}

	network::GDOutputPacket<network::GDGuildWithdrawMoneyGiveReplyPacket> p;
	p->set_guild_id(GetID());
	p->set_change_gold(iChangeGold);
	p->set_give_success(ch ? 1 : 0);
	db_clientdesc->DBPacket(p);
}

bool CGuild::HasLand()
{
	return building::CManager::instance().FindLandByGuild(GetID()) != NULL;
}

// GUILD_JOIN_BUG_FIX
/// ±æµå ÃÊ´ë event Á¤º¸
EVENTINFO(TInviteGuildEventInfo)
{
	DWORD	dwInviteePID;		///< ÃÊ´ë¹ÞÀº character ÀÇ PID
	DWORD	dwGuildID;		///< ÃÊ´ëÇÑ Guild ÀÇ ID

	TInviteGuildEventInfo()
	: dwInviteePID( 0 )
	, dwGuildID( 0 )
	{
	}
};

/**
 * ±æµå ÃÊ´ë event callback ÇÔ¼ö.
 * event °¡ ¹ßµ¿ÇÏ¸é ÃÊ´ë °ÅÀý·Î Ã³¸®ÇÑ´Ù.
 */
EVENTFUNC( GuildInviteEvent )
{
	TInviteGuildEventInfo *pInfo = dynamic_cast<TInviteGuildEventInfo*>( event->info );

	if ( pInfo == NULL )
	{
		sys_err( "GuildInviteEvent> <Factor> Null pointer" );
		return 0;
	}

	CGuild* pGuild = CGuildManager::instance().FindGuild( pInfo->dwGuildID );

	if ( pGuild ) 
	{
		sys_log( 0, "GuildInviteEvent %s", pGuild->GetName() );
		pGuild->InviteDeny( pInfo->dwInviteePID );
	}

	return 0;
}

void CGuild::Invite( LPCHARACTER pchInviter, LPCHARACTER pchInvitee )
{
	if (quest::CQuestManager::instance().GetPCForce(pchInviter->GetPlayerID())->IsRunning() == true)
	{
		pchInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> »ó´ë¹æÀÌ ÃÊ´ë ½ÅÃ»À» ¹ÞÀ» ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return;
	}

	
	if (quest::CQuestManager::instance().GetPCForce(pchInvitee->GetPlayerID())->IsRunning() == true)
		return;

	if ( pchInvitee->IsBlockMode( BLOCK_GUILD_INVITE ) ) 
	{
		pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> »ó´ë¹æÀÌ ±æµå ÃÊ´ë °ÅºÎ »óÅÂÀÔ´Ï´Ù.") );
		return;
	} 
	else if ( !HasGradeAuth( GetMember( pchInviter->GetPlayerID() )->grade, GUILD_AUTH_ADD_MEMBER ) ) 
	{
		pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> ±æµå¿øÀ» ÃÊ´ëÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù.") );
		return;
	} 
	else if ( pchInvitee->GetEmpire() != pchInviter->GetEmpire() ) 
	{
		pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> ´Ù¸¥ Á¦±¹ »ç¶÷À» ±æµå¿¡ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù.") );
		return;
	}

	GuildJoinErrCode errcode = VerifyGuildJoinableCondition( pchInvitee );
	switch ( errcode ) 
	{
		case GERR_NONE: break;
		case GERR_WITHDRAWPENALTY:
						pchInviter->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT(pchInviter, "<±æµå> Å»ÅðÇÑ ÈÄ %dÀÏÀÌ Áö³ªÁö ¾ÊÀº »ç¶÷Àº ±æµå¿¡ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_withdraw_delay" ) );
						return;
		case GERR_COMMISSIONPENALTY:
						pchInviter->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT(pchInviter, "<±æµå> ±æµå¸¦ ÇØ»êÇÑ Áö %dÀÏÀÌ Áö³ªÁö ¾ÊÀº »ç¶÷Àº ±æµå¿¡ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_disband_delay") );
						return;
		case GERR_ALREADYJOIN:	pchInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> »ó´ë¹æÀÌ ÀÌ¹Ì ´Ù¸¥ ±æµå¿¡ ¼ÓÇØÀÖ½À´Ï´Ù.")); return;
		case GERR_GUILDISFULL:	pchInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> ÃÖ´ë ±æµå¿ø ¼ö¸¦ ÃÊ°úÇß½À´Ï´Ù.")); return;
		case GERR_GUILD_IS_IN_WAR : pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> ÇöÀç ±æµå°¡ ÀüÀï Áß ÀÔ´Ï´Ù.") ); return;
		case GERR_INVITE_LIMIT : pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInviter, "<±æµå> ÇöÀç ½Å±Ô °¡ÀÔ Á¦ÇÑ »óÅÂ ÀÔ´Ï´Ù.") ); return;

		default: sys_err( "ignore guild join error(%d)", errcode ); return;
	}

	if ( m_GuildInviteEventMap.end() != m_GuildInviteEventMap.find( pchInvitee->GetPlayerID() ) )
		return;

	//
	// ÀÌº¥Æ® »ý¼º
	// 
	TInviteGuildEventInfo* pInfo = AllocEventInfo<TInviteGuildEventInfo>();
	pInfo->dwInviteePID = pchInvitee->GetPlayerID();
	pInfo->dwGuildID = GetID();

	m_GuildInviteEventMap.insert(EventMap::value_type(pchInvitee->GetPlayerID(), event_create(GuildInviteEvent, pInfo, PASSES_PER_SEC(10))));

	network::GCOutputPacket<network::GCGuildInvitePacket> p;
	p->set_guild_id(GetID());
	p->set_guild_name(GetName());

	pchInvitee->GetDesc()->Packet(p);
}

void CGuild::InviteAccept( LPCHARACTER pchInvitee )
{
	EventMap::iterator itFind = m_GuildInviteEventMap.find( pchInvitee->GetPlayerID() );
	if ( itFind == m_GuildInviteEventMap.end() ) 
	{
		sys_log( 0, "GuildInviteAccept from not invited character(invite guild: %s, invitee: %s)", GetName(), pchInvitee->GetName() );
		return;
	}

	event_cancel( &itFind->second );
	m_GuildInviteEventMap.erase( itFind );

	GuildJoinErrCode errcode = VerifyGuildJoinableCondition( pchInvitee );
	switch ( errcode ) 
	{
		case GERR_NONE: break;
		case GERR_WITHDRAWPENALTY:
						pchInvitee->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT(pchInvitee, "<±æµå> Å»ÅðÇÑ ÈÄ %dÀÏÀÌ Áö³ªÁö ¾ÊÀº »ç¶÷Àº ±æµå¿¡ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_withdraw_delay" ) );
						return;
		case GERR_COMMISSIONPENALTY:
						pchInvitee->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT(pchInvitee, "<±æµå> ±æµå¸¦ ÇØ»êÇÑ Áö %dÀÏÀÌ Áö³ªÁö ¾ÊÀº »ç¶÷Àº ±æµå¿¡ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_disband_delay") );
						return;
		case GERR_ALREADYJOIN:	pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<±æµå> »ó´ë¹æÀÌ ÀÌ¹Ì ´Ù¸¥ ±æµå¿¡ ¼ÓÇØÀÖ½À´Ï´Ù.")); return;
		case GERR_GUILDISFULL:	pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<±æµå> ÃÖ´ë ±æµå¿ø ¼ö¸¦ ÃÊ°úÇß½À´Ï´Ù.")); return;
		case GERR_GUILD_IS_IN_WAR : pchInvitee->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<±æµå> ÇöÀç ±æµå°¡ ÀüÀï Áß ÀÔ´Ï´Ù.") ); return;
		case GERR_INVITE_LIMIT : pchInvitee->ChatPacket( CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<±æµå> ÇöÀç ½Å±Ô °¡ÀÔ Á¦ÇÑ »óÅÂ ÀÔ´Ï´Ù.") ); return;

		default: sys_err( "ignore guild join error(%d)", errcode ); return;
	}

	LogManager::instance().CharLog(pchInvitee, 0, "GUILD_JOIN", GetName());

	RequestAddMember( pchInvitee, 15 );
}

void CGuild::InviteDeny( DWORD dwPID )
{
	EventMap::iterator itFind = m_GuildInviteEventMap.find( dwPID );
	if ( itFind == m_GuildInviteEventMap.end() ) 
	{
		sys_log( 0, "GuildInviteDeny from not invited character(invite guild: %s, invitee PID: %d)", GetName(), dwPID );
		return;
	}

	event_cancel( &itFind->second );
	m_GuildInviteEventMap.erase( itFind );
}

void CGuild::ChangeName(LPCHARACTER ch, const char* c_pszNewName)
{
	char szHint[256];
	snprintf(szHint, sizeof(szHint), "old[%s] => new[%s]", GetName(), c_pszNewName);
	LogManager::instance().CharLog(ch, GetID(), "GUILD_NAME_CHANGE", szHint);

	DBManager::instance().Query("UPDATE guild SET name = '%s' WHERE id = %u", CGuildManager::instance().EscapedGuildName(c_pszNewName), GetID());

	P2P_ChangeName(c_pszNewName);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The guild name was changed. You have to relog to see the change."));

	network::GGOutputPacket<network::GGGuildChangeNamePacket> p;
	p->set_guild_id(GetID());
	p->set_name(c_pszNewName);

	P2P_MANAGER::instance().Send(p);
}

void CGuild::P2P_ChangeName(const char* c_pszNewName)
{
	strlcpy(m_data.name, c_pszNewName, sizeof(m_data.name));

	SendGuildDataUpdateToAllMember();
}

CGuild::GuildJoinErrCode CGuild::VerifyGuildJoinableCondition( const LPCHARACTER pchInvitee )
{
	if ( get_global_time() - pchInvitee->GetQuestFlag( "guild_manage.new_withdraw_time" )
			< CGuildManager::instance().GetWithdrawDelay() )
		return GERR_WITHDRAWPENALTY;
	else if ( get_global_time() - pchInvitee->GetQuestFlag( "guild_manage.new_disband_time" )
			< CGuildManager::instance().GetDisbandDelay() )
		return GERR_COMMISSIONPENALTY;
	else if ( pchInvitee->GetGuild() )
		return GERR_ALREADYJOIN;
	else if ( GetMemberCount() >= GetMaxMemberCount() )
	{
		sys_log(1, "GuildName = %s, GetMemberCount() = %d, GetMaxMemberCount() = %d (32 + MAX(level(%d)-10, 0) * 2 + bonus(%d)", 
				GetName(), GetMemberCount(), GetMaxMemberCount(), m_data.level, m_iMemberCountBonus);
		return GERR_GUILDISFULL;
	}
	else if ( UnderAnyWar() != 0 )
	{
		return GERR_GUILD_IS_IN_WAR;
	}

	return GERR_NONE;
}
// END_OF_GUILD_JOIN_BUG_FIX

bool CGuild::ChangeMasterTo(DWORD dwPID)
{
	if ( GetMember(dwPID) == NULL ) return false;

	network::GDOutputPacket<network::GDGuildReqChangeMasterPacket> p;
	p->set_guild_id(GetID());
	p->set_id_from(GetMasterPID());
	p->set_id_to(dwPID);
	db_clientdesc->DBPacket(p);

	return true;
}

void CGuild::SendGuildDataUpdateToAllMember(SQLMsg* pmsg)
{
	TGuildMemberOnlineContainer::iterator iter = m_memberOnline.begin();

	for (; iter != m_memberOnline.end(); iter++ )
	{
		SendGuildInfoPacket(*iter);
		SendAllGradePacket(*iter);
	}
}

#ifdef __DUNGEON_FOR_GUILD__
bool CGuild::RequestDungeon(BYTE bChannel, long lMapIndex)
{
	network::GDOutputPacket<network::GDGuildDungeonPacket> sPacket;
	sPacket->set_guild_id(GetID());
	sPacket->set_channel(bChannel);
	sPacket->set_map_index(lMapIndex);
	db_clientdesc->DBPacket(sPacket);
	return true;
}

void CGuild::RecvDungeon(BYTE bChannel, long lMapIndex)
{
	m_data.dungeon_ch = bChannel;
	m_data.dungeon_map = lMapIndex;
}

bool CGuild::SetDungeonCooldown(DWORD dwTime)
{
	network::GDOutputPacket<network::GDGuildDungeonCDPacket> sPacket;
	sPacket->set_guild_id(GetID());
	sPacket->set_time(dwTime);
	db_clientdesc->DBPacket(sPacket);
	return true;
}

void CGuild::RecvDungeonCD(DWORD dwTime)
{
	m_data.dungeon_cooldown = dwTime;
}
#endif

int CGuild::GetGuildWarWinCount(BYTE type) const
{
	int total = 0;
	if (type >= GUILD_WAR_TYPE_MAX_NUM)
		for (BYTE i = 0; i < GUILD_WAR_TYPE_MAX_NUM; ++i)
			total += m_data.win[i];
	else
		total = m_data.win[type];

	return total;
}

int CGuild::GetGuildWarDrawCount(BYTE type) const 
{
	int total = 0;
	if (type >= GUILD_WAR_TYPE_MAX_NUM)
		for (BYTE i = 0; i < GUILD_WAR_TYPE_MAX_NUM; ++i)
			total += m_data.draw[i];
	else
		total = m_data.draw[type];

	return total;
}

int CGuild::GetGuildWarLossCount(BYTE type) const 
{ 
	int total = 0;
	if (type >= GUILD_WAR_TYPE_MAX_NUM)
		for (BYTE i = 0; i < GUILD_WAR_TYPE_MAX_NUM; ++i)
			total += m_data.loss[i];
	else
		total = m_data.loss[type];

	return total;
}

void CGuild::SetWarData(
	const ::google::protobuf::RepeatedField<::google::protobuf::int32>& wins,
	const ::google::protobuf::RepeatedField<::google::protobuf::int32>& draws,
	const ::google::protobuf::RepeatedField<::google::protobuf::int32>& losses)
{
	for (int i = 0; i < GUILD_WAR_TYPE_MAX_NUM; ++i)
	{
		if (i < wins.size())
			m_data.win[i] = wins[i];
		if (i < draws.size())
			m_data.draw[i] = draws[i];
		if (i < losses.size())
			m_data.loss[i] = losses[i];
	}

	network::GCOutputPacket<network::GCGuildBattleStatsPacket> pack;
	*pack->mutable_wins() = wins;
	*pack->mutable_draws() = draws;
	*pack->mutable_losses() = losses;

	ForEachOnlineMember([&pack](LPCHARACTER ch) {
		ch->GetDesc()->Packet(pack);
	});
}

