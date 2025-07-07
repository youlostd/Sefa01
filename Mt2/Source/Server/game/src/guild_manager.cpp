#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "log.h"
#include "char.h"
#include "db.h"
#include "lzo_manager.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "packet.h"
#include "war_map.h"
#include "questmanager.h"
#include "guild_manager.h"
#include "MarkManager.h"

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

SGuildMember::SGuildMember(LPCHARACTER ch, BYTE grade, DWORD offer_exp)
	: pid(ch->GetPlayerID()), grade(grade), is_general(0), job(ch->GetJob()), level(ch->GetLevel()), offer_exp(offer_exp), name(ch->GetName())
{}
SGuildMember::SGuildMember(DWORD pid, BYTE grade, BYTE is_general, BYTE job, BYTE level, DWORD offer_exp, const char* name)
	: pid(pid), grade(grade), is_general(is_general), job(job), level(level), offer_exp(offer_exp), name(name)
{}

CGuildManager::CGuildManager() : m_dwLastRankUpdate(0)
{
}

CGuildManager::~CGuildManager()
{
	for( TGuildMap::const_iterator iter = m_mapGuild.begin() ; iter != m_mapGuild.end() ; ++iter )
	{
		M2_DELETE_ONLY(iter->second);
	}

	for (itertype(m_vecRanking[0]) it = m_vecRanking[0].begin(); it != m_vecRanking[0].end(); ++it)
		delete (*it);

	for (BYTE i = 0; i < EMPIRE_MAX_NUM; ++i)
		m_vecRanking[i].clear();

	m_mapGuild.clear();
}

int CGuildManager::GetDisbandDelay()
{
	return quest::CQuestManager::instance().GetEventFlag("guild_disband_delay") * (test_server ? 60 : 86400);
}

int CGuildManager::GetWithdrawDelay()
{
	return quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay") * (test_server ? 60 : 86400);
}

const char* CGuildManager::EscapedGuildName(const char* c_pszGuildName)
{
	static char s_szEscapedName[GUILD_NAME_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(s_szEscapedName, sizeof(s_szEscapedName), c_pszGuildName, strlen(c_pszGuildName));

	return s_szEscapedName;
}

DWORD CGuildManager::CreateGuild(TGuildCreateParameter& gcp)
{
	if (!gcp.master)
		return 0;

	if (!check_name(gcp.name, true))
	{
		gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(gcp.master, "<±æµå> ±æµå ÀÌ¸§ÀÌ ÀûÇÕÇÏÁö ¾Ê½À´Ï´Ù."));
		return 0;
	}

	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM guild WHERE name = '%s'",
																 EscapedGuildName(gcp.name)));

	if (pmsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

		if (!(row[0] && row[0][0] == '0'))
		{
			gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(gcp.master, "<±æµå> ÀÌ¹Ì °°Àº ÀÌ¸§ÀÇ ±æµå°¡ ÀÖ½À´Ï´Ù."));
			return 0;
		}
	}
	else
	{
		gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(gcp.master, "<±æµå> ±æµå¸¦ »ý¼ºÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return 0;
	}

	// new CGuild(gcp) queries guild tables and tell dbcache to notice other game servers.
	// other game server calls CGuildManager::LoadGuild to load guild.
	CGuild * pg = M2_NEW CGuild(gcp);
	m_mapGuild.insert(std::make_pair(pg->GetID(), pg));
	return pg->GetID();
}

void CGuildManager::Unlink(DWORD pid)
{
	m_map_pkGuildByPID.erase(pid);
}

CGuild * CGuildManager::GetLinkedGuild(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it == m_map_pkGuildByPID.end())
		return NULL;

	return it->second; 
}

void CGuildManager::Link(DWORD pid, CGuild* guild)
{
	if (m_map_pkGuildByPID.find(pid) == m_map_pkGuildByPID.end())
		m_map_pkGuildByPID.insert(std::make_pair(pid,guild));
}

void CGuildManager::P2PLogoutMember(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->P2PLogoutMember(pid);
	}
}

void CGuildManager::P2PLoginMember(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->P2PLoginMember(pid);
	}
}

void CGuildManager::LoginMember(LPCHARACTER ch)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(ch->GetPlayerID());

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->LoginMember(ch);
	}
}


CGuild* CGuildManager::TouchGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
	{
		m_mapGuild.insert(std::make_pair(guild_id, M2_NEW CGuild(guild_id)));
		it = m_mapGuild.find(guild_id);

		network::GCOutputPacket<network::GCGuildNamePacket> p;
		auto name_info = p->add_names();
		name_info->set_guild_id(guild_id);
		name_info->set_name(it->second->GetName());

		CHARACTER_MANAGER::instance().for_each_pc([&p](LPCHARACTER ch) {
			ch->PacketAround(p);
		});
	}

	return it->second;
}

CGuild* CGuildManager::FindGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);
	if (it == m_mapGuild.end())
	{
		return NULL;
	}
	return it->second;
}

CGuild*	CGuildManager::FindGuildByName(const std::string guild_name)
{
	itertype(m_mapGuild) it;
	for (it = m_mapGuild.begin(); it!=m_mapGuild.end(); ++it)
	{
		if (it->second->GetName()==guild_name)
			return it->second;
	}
	return NULL;
}

void CGuildManager::Initialize()
{
	sys_log(0, "Initializing Guild");

	if (g_bAuthServer)
	{
		sys_log(0, "	No need for auth server");
		return;
	}

	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT id FROM guild"));

	std::vector<DWORD> vecGuildID;
	vecGuildID.reserve(pmsg->Get()->uiNumRows);

	for (uint i = 0; i < pmsg->Get()->uiNumRows; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		DWORD guild_id = strtoul(row[0], (char**) NULL, 10);
		LoadGuild(guild_id);

		vecGuildID.push_back(guild_id);
	}

	CGuildMarkManager & rkMarkMgr = CGuildMarkManager::instance();

	rkMarkMgr.SetMarkPathPrefix("mark");

	extern bool GuildMarkConvert(const std::vector<DWORD> & vecGuildID);
	GuildMarkConvert(vecGuildID);

	rkMarkMgr.LoadMarkIndex();
	rkMarkMgr.LoadMarkImages();
	rkMarkMgr.LoadSymbol(GUILD_SYMBOL_FILENAME);
}

void CGuildManager::LoadGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
	{
		m_mapGuild.insert(std::make_pair(guild_id, M2_NEW CGuild(guild_id)));
	}
	else
	{
		//it->second->Load(guild_id);
	}
}

void CGuildManager::DisbandGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
		return;

#ifdef __MELEY_LAIR_DUNGEON__
	MeleyLair::CMgr::instance().GuildRemoved(it->second);
#endif

	it->second->Disband();

	M2_DELETE(it->second);
	m_mapGuild.erase(it);

	CGuildMarkManager::instance().DeleteMark(guild_id);
}

void CGuildManager::SkillRecharge()
{
	for (TGuildMap::iterator it = m_mapGuild.begin(); it!=m_mapGuild.end();++it)
	{
		it->second->SkillRecharge();
	}
}

int CGuildManager::GetRank(CGuild* g)
{
	int point = g->GetLadderPoint();
	int rank = 1;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end();++it)
	{
		CGuild * pg = it->second;

		if (pg->GetLadderPoint() > point)
			rank++;
	}

	return rank;
}

struct FGuildCompare : public std::binary_function<CGuild*, CGuild*, bool>
{
	bool operator () (CGuild* g1, CGuild* g2) const
	{
		if (g1->GetLadderPoint() < g2->GetLadderPoint())
			return true;
		if (g1->GetLadderPoint() > g2->GetLadderPoint())
			return false;
		if (g1->GetGuildWarWinCount(GUILD_WAR_TYPE_MAX_NUM) < g2->GetGuildWarWinCount(GUILD_WAR_TYPE_MAX_NUM))
			return true;
		if (g1->GetGuildWarWinCount(GUILD_WAR_TYPE_MAX_NUM) > g2->GetGuildWarWinCount(GUILD_WAR_TYPE_MAX_NUM))
			return false;
		if (g1->GetGuildWarLossCount(GUILD_WAR_TYPE_MAX_NUM) < g2->GetGuildWarLossCount(GUILD_WAR_TYPE_MAX_NUM))
			return true;
		if (g1->GetGuildWarLossCount(GUILD_WAR_TYPE_MAX_NUM) > g2->GetGuildWarLossCount(GUILD_WAR_TYPE_MAX_NUM))
			return false;
		int c = strcmp(g1->GetName(), g2->GetName());
		if (c>0) 
			return true;
		return false;
	}
};

struct FGuildRankCompare : public std::binary_function<const network::TGuildLadderInfo*, const network::TGuildLadderInfo*, bool>
{
	bool operator () (const network::TGuildLadderInfo* g1, const network::TGuildLadderInfo* g2) const
	{
		if (g1->ladder_points() > g2->ladder_points())
			return true;
		if (g1->ladder_points() < g2->ladder_points())
			return false;
		if (g1->level() > g2->level())
			return true;
		if (g1->level() < g2->level())
			return false;
		int c = strcmp(g1->name().c_str(), g2->name().c_str());
		if (c>0)
			return true;
		return false;
	}
};

void CGuildManager::SendSpecificGuildRank(LPCHARACTER ch, const char* searchName, BYTE pageType, BYTE empire)
{
	ch->tchat("rankRequestSearch: %s %d %d", searchName, pageType, empire);
	if (!ch->GetDesc() || empire >= EMPIRE_MAX_NUM)
		return;

	CGuild* guild = FindGuildByName(searchName);
	if (!guild)
		return;

	if (empire && guild->GetEmpire() != empire)
		return;

	network::GCOutputPacket<network::GCGuildLadderSearchResultPacket> p;

	auto ladder = p->mutable_ladder();
	ladder->set_name(searchName);
	ladder->set_level(guild->GetLevel());
	ladder->set_ladder_points(guild->GetLadderPoint());
	ladder->set_min_member(guild->GetMemberCount());
	ladder->set_max_member(guild->GetMaxMemberCount());
	p->set_rank(GetRank(guild));

	ch->GetDesc()->Packet(p);
}

void CGuildManager::SendGuildList(LPCHARACTER ch, DWORD pageNumber, BYTE pageType, BYTE empire)
{
	ch->tchat("rankRequest: %d %d %d", pageNumber, pageType, empire);
	if (!ch->GetDesc() || empire >= EMPIRE_MAX_NUM)
		return;

	if (m_dwLastRankUpdate < get_global_time() + GUILD_LADDER_UPDATE_INTERVAL)
	{
		for (itertype(m_vecRanking[0]) it = m_vecRanking[0].begin(); it != m_vecRanking[0].end(); ++it)
			M2_DELETE(*it);

		for (BYTE i = 0; i < EMPIRE_MAX_NUM; ++i)
			m_vecRanking[i].clear();

		for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
		{
			if (it->second)
			{
				auto ladder = M2_NEW network::TGuildLadderInfo();
				ladder->set_name(it->second->GetName());
				ladder->set_level(it->second->GetLevel());
				ladder->set_ladder_points(it->second->GetLadderPoint());
				ladder->set_min_member(it->second->GetMemberCount());
				ladder->set_max_member(it->second->GetMaxMemberCount());

				m_vecRanking[0].insert(std::upper_bound(m_vecRanking[0].begin(), m_vecRanking[0].end(), ladder, FGuildRankCompare()), ladder);
				BYTE currEmpire = it->second->GetEmpire();
				if (currEmpire && currEmpire < EMPIRE_MAX_NUM)
					m_vecRanking[currEmpire].insert(std::upper_bound(m_vecRanking[currEmpire].begin(), m_vecRanking[currEmpire].end(), ladder, FGuildRankCompare()), ladder);
			}
		}

		m_dwLastRankUpdate = get_global_time();
	}

	network::GCOutputPacket<network::GCGuildLadderListPacket> p;
	p->set_page_number(pageNumber);
	p->set_total_pages(m_vecRanking[empire].size() / GUILD_RANKING_RESULT_PER_PAGE + 1);

	for (size_t i = pageNumber * GUILD_RANKING_RESULT_PER_PAGE; i < (pageNumber + 1) * GUILD_RANKING_RESULT_PER_PAGE && i < m_vecRanking[empire].size(); ++i)
		*p->add_ladders() = *m_vecRanking[empire][i];

	ch->GetDesc()->Packet(p);
}

void CGuildManager::GetHighRankString(DWORD dwMyGuild, char * buffer, size_t buflen)
{
	using namespace std;
	vector<CGuild*> v;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second)
			v.push_back(it->second);
	}

	std::sort(v.begin(), v.end(), FGuildCompare());
	int n = v.size();
	int len = 0, len2;
	*buffer = '\0';

	for (int i = 0; i < 8; ++i)
	{
		if (n - i - 1 < 0)
			break;

		CGuild * g = v[n - i - 1];

		if (!g)
			continue;

		if (g->GetLadderPoint() <= 0)
			break;

		if (dwMyGuild == g->GetID())
		{
			len2 = snprintf(buffer + len, buflen - len, "[COLOR r;255|g;255|b;0]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		if (i)
		{
			len2 = snprintf(buffer + len, buflen - len, "{ENTER}");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		len2 = snprintf(buffer + len, buflen - len, "%3d | %-12s | %-8d | %4d | %4d | %4d", 
				GetRank(g),
				g->GetName(),
				g->GetLadderPoint(),
				g->GetGuildWarWinCount(GUILD_WAR_TYPE_MAX_NUM),
				g->GetGuildWarDrawCount(GUILD_WAR_TYPE_MAX_NUM),
				g->GetGuildWarLossCount(GUILD_WAR_TYPE_MAX_NUM));

		if (len2 < 0 || len2 >= (int) buflen - len)
			len += (buflen - len) - 1;
		else
			len += len2;

		if (g->GetID() == dwMyGuild)
		{
			len2 = snprintf(buffer + len, buflen - len, "[/COLOR]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}
	}
}

void CGuildManager::GetAroundRankString(DWORD dwMyGuild, char * buffer, size_t buflen)
{
	using namespace std;
	vector<CGuild*> v;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second)
			v.push_back(it->second);
	}

	std::sort(v.begin(), v.end(), FGuildCompare());
	int n = v.size();
	int idx;

	for (idx = 0; idx < (int) v.size(); ++idx)
		if (v[idx]->GetID() == dwMyGuild)
			break;

	int len = 0, len2;
	int count = 0;
	*buffer = '\0';

	for (int i = -3; i <= 3; ++i)
	{
		if (idx - i < 0)
			continue;

		if (idx - i >= n)
			continue;

		CGuild * g = v[idx - i];

		if (!g)
			continue;

		if (dwMyGuild == g->GetID())
		{
			len2 = snprintf(buffer + len, buflen - len, "[COLOR r;255|g;255|b;0]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		if (count)
		{
			len2 = snprintf(buffer + len, buflen - len, "{ENTER}");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		len2 = snprintf(buffer + len, buflen - len, "%3d | %-12s | %-8d | %4d | %4d | %4d", 
				GetRank(g),
				g->GetName(),
				g->GetLadderPoint(),
				g->GetGuildWarWinCount(GUILD_WAR_TYPE_MAX_NUM),
				g->GetGuildWarDrawCount(GUILD_WAR_TYPE_MAX_NUM),
				g->GetGuildWarLossCount(GUILD_WAR_TYPE_MAX_NUM));

		if (len2 < 0 || len2 >= (int) buflen - len)
			len += (buflen - len) - 1;
		else
			len += len2;

		++count;

		if (g->GetID() == dwMyGuild)
		{
			len2 = snprintf(buffer + len, buflen - len, "[/COLOR]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Guild War
/////////////////////////////////////////////////////////////////////
void CGuildManager::RequestCancelWar(DWORD guild_id1, DWORD guild_id2)
{
	sys_log(0, "RequestCancelWar %d %d", guild_id1, guild_id2);

	network::GDOutputPacket<network::GDGuildWarPacket> p;
	p->set_war(GUILD_WAR_CANCEL);
	p->set_guild_from(guild_id1);
	p->set_guild_to(guild_id2);
	db_clientdesc->DBPacket(p);
}

void CGuildManager::RequestEndWar(DWORD guild_id1, DWORD guild_id2)
{
	sys_log(0, "RequestEndWar %d %d", guild_id1, guild_id2);

	network::GDOutputPacket<network::GDGuildWarPacket> p;
	p->set_war(GUILD_WAR_END);
	p->set_guild_from(guild_id1);
	p->set_guild_to(guild_id2);
	db_clientdesc->DBPacket(p);
}

void CGuildManager::RequestWarOver(DWORD dwGuild1, DWORD dwGuild2, DWORD dwGuildWinner, long lReward)
{
	CGuild * g1 = TouchGuild(dwGuild1);
	CGuild * g2 = TouchGuild(dwGuild2);

	if (g1->GetGuildWarState(g2->GetID()) != GUILD_WAR_ON_WAR)
	{
		sys_log(0, "RequestWarOver : both guild was not in war %u %u", dwGuild1, dwGuild2);
		RequestEndWar(dwGuild1, dwGuild2);
		return;
	}

	network::GDOutputPacket<network::GDGuildWarPacket> p;

	p->set_war(GUILD_WAR_OVER);
	p->set_type(dwGuildWinner == 0 ? 1 : 0); // type == 1 means draw for this packet.

	if (dwGuildWinner == 0)
	{
		p->set_guild_from(dwGuild1);
		p->set_guild_to(dwGuild2);
	}
	else
	{
		p->set_guild_from(dwGuildWinner);
		p->set_guild_to(dwGuildWinner == dwGuild1 ? dwGuild2 : dwGuild1);
	}

	db_clientdesc->DBPacket(p);
	sys_log(0, "RequestWarOver : winner %u loser %u draw %u betprice %d", p->guild_from(), p->guild_to(), p->type(), p->war_price());
}

void CGuildManager::DeclareWar(DWORD guild_id1, DWORD guild_id2, BYTE bType)
{
	if (guild_id1 == guild_id2)
		return;

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
		return;

	if (g1->DeclareWar(guild_id2, bType, GUILD_WAR_SEND_DECLARE))
		g2->DeclareWar(guild_id1, bType, GUILD_WAR_RECV_DECLARE);
}

void CGuildManager::RefuseWar(DWORD guild_id1, DWORD guild_id2)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (g1 && g2)
	{
		if (g2->GetMasterCharacter())
			g2->GetMasterCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(g2->GetMasterCharacter(), "<±æµå> %s ±æµå°¡ ±æµåÀüÀ» °ÅºÎÇÏ¿´½À´Ï´Ù."), g1->GetName());
	}

	if ( g1 != NULL )
		g1->RefuseWar(guild_id2);

	if ( g2 != NULL && g1 != NULL )
		g2->RefuseWar(g1->GetID());
}

void CGuildManager::WaitStartWar(DWORD guild_id1, DWORD guild_id2)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
	{
		sys_log(0, "GuildWar: CGuildManager::WaitStartWar(%d,%d) - something wrong in arg. there is no guild like that.", guild_id1, guild_id2);
		return;
	}

	if (g1->WaitStartWar(guild_id2) || g2->WaitStartWar(guild_id1) )
	{
		char buf[256];
		snprintf(buf, sizeof(buf), "[%s] ±æµå¿Í [%s] ±æµå°¡ Àá½Ã ÈÄ ÀüÀïÀ» ½ÃÀÛÇÕ´Ï´Ù!", g1->GetName(), g2->GetName());
		SendNotice(buf);
	}
}

void CGuildManager::StartWar(DWORD guild_id1, DWORD guild_id2)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
		return;

	if (!g1->CheckStartWar(guild_id2) || !g2->CheckStartWar(guild_id1))
		return;

	g1->StartWar(guild_id2);
	g2->StartWar(guild_id1);

	char buf[256];
	snprintf(buf, sizeof(buf), "[%s] ±æµå¿Í [%s] ±æµå°¡ ÀüÀïÀ» ½ÃÀÛÇÏ¿´½À´Ï´Ù!", g1->GetName(), g2->GetName());
	SendNotice(buf);

	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);
	
	CHARACTER_MANAGER::instance().for_each_pc([guild_id1, guild_id2](LPCHARACTER ch) {
		network::GCOutputPacket<network::GCGuildWarListPacket> pack;
		auto war = pack->add_wars();
		war->set_src_guild_id(guild_id1);
		war->set_dst_guild_id(guild_id2);

		ch->GetDesc()->Packet(pack);
	});
	m_GuildWar.insert(std::make_pair(guild_id1, guild_id2));
}

void SendGuildWarOverNotice(CGuild* g1, CGuild* g2, bool bDraw)
{
	if (g1 && g2)
	{
		char buf[256];

		if (bDraw)
		{
			snprintf(buf, sizeof(buf), "[%s] ±æµå¿Í [%s] ±æµå »çÀÌÀÇ ÀüÀïÀÌ ¹«½ÂºÎ·Î ³¡³µ½À´Ï´Ù.", g1->GetName(), g2->GetName());
		}
		else
		{
			if ( g1->GetWarScoreAgainstTo( g2->GetID() ) > g2->GetWarScoreAgainstTo( g1->GetID() ) )
			{
				snprintf(buf, sizeof(buf), "[%s] ±æµå°¡ [%s] ±æµå¿ÍÀÇ ÀüÀï¿¡¼­ ½Â¸® Çß½À´Ï´Ù.", g1->GetName(), g2->GetName());
			}
			else
			{
				snprintf(buf, sizeof(buf), "[%s] ±æµå°¡ [%s] ±æµå¿ÍÀÇ ÀüÀï¿¡¼­ ½Â¸® Çß½À´Ï´Ù.", g2->GetName(), g1->GetName());
			}
		}

		SendNotice(buf);
	}
}

bool CGuildManager::EndWar(DWORD guild_id1, DWORD guild_id2)
{
	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	std::pair<DWORD, DWORD> k = std::make_pair(guild_id1, guild_id2);

	TGuildWarContainer::iterator it = m_GuildWar.find(k);

	if (m_GuildWar.end() == it)
	{
		sys_log(0, "EndWar(%d,%d) - EndWar request but guild is not in list", guild_id1, guild_id2);
		return false;
	}

	if ( g1 && g2 )
	{
		if (g1->GetGuildWarType(guild_id2) == GUILD_WAR_TYPE_FIELD)
		{
			SendGuildWarOverNotice(g1, g2, g1->GetWarScoreAgainstTo(guild_id2) == g2->GetWarScoreAgainstTo(guild_id1));
		}
	}
	else
	{
		return false;
	}

	if (g1)
		g1->EndWar(guild_id2);

	if (g2)
		g2->EndWar(guild_id1);

	m_GuildWarEndTime[k] = get_global_time();
	CHARACTER_MANAGER::instance().for_each_pc([guild_id1, guild_id2](LPCHARACTER ch) {
		network::GCOutputPacket<network::GCGuildWarEndListPacket> pack;
		auto war = pack->add_wars();
		war->set_src_guild_id(guild_id1);
		war->set_dst_guild_id(guild_id2);

		ch->GetDesc()->Packet(pack);
	});
	m_GuildWar.erase(it);

	return true;
}

void CGuildManager::WarOver(DWORD guild_id1, DWORD guild_id2, bool bDraw)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	std::pair<DWORD, DWORD> k = std::make_pair(guild_id1, guild_id2);

	TGuildWarContainer::iterator it = m_GuildWar.find(k);

	if (m_GuildWar.end() == it)
		return;

	SendGuildWarOverNotice(g1, g2, bDraw);

	EndWar(guild_id1, guild_id2);
}

void CGuildManager::CancelWar(DWORD guild_id1, DWORD guild_id2)
{
	if (!EndWar(guild_id1, guild_id2))
		return;

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (g1)
	{
		LPCHARACTER master1 = g1->GetMasterCharacter();

		if (master1)
			master1->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(master1, "<±æµå> ±æµåÀüÀÌ Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
	}

	if (g2)
	{
		LPCHARACTER master2 = g2->GetMasterCharacter();

		if (master2)
			master2->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(master2, "<±æµå> ±æµåÀüÀÌ Ãë¼Ò µÇ¾ú½À´Ï´Ù."));
	}

	if (g1 && g2)
	{
		char buf[256+1];
		snprintf(buf, sizeof(buf), "[%s] ±æµå¿Í [%s] ±æµå »çÀÌÀÇ ÀüÀïÀÌ Ãë¼ÒµÇ¾ú½À´Ï´Ù.", g1->GetName(), g2->GetName());
		SendNotice(buf);
	}
}

void CGuildManager::ReserveWar(DWORD dwGuild1, DWORD dwGuild2, BYTE bType) // from DB
{
	sys_log(0, "GuildManager::ReserveWar %u %u", dwGuild1, dwGuild2);

	CGuild * g1 = FindGuild(dwGuild1);
	CGuild * g2 = FindGuild(dwGuild2);

	if (!g1 || !g2)
		return;

	g1->ReserveWar(dwGuild2, bType);
	g2->ReserveWar(dwGuild1, bType);
}

void CGuildManager::ShowGuildWarList(LPCHARACTER ch)
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		CGuild * A = TouchGuild(it->first);
		CGuild * B = TouchGuild(it->second);

		if (A && B)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s[%d] vs %s[%d] time %u sec.",
					A->GetName(), A->GetID(),
					B->GetName(), B->GetID(),
					get_global_time() - A->GetWarStartTime(B->GetID()));
		}
	}
}

void CGuildManager::SendGuildWar(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	network::GCOutputPacket<network::GCGuildWarListPacket> p;

	for (auto it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		auto war = p->add_wars();
		war->set_src_guild_id(it->first);
		war->set_dst_guild_id(it->second);
	}

	ch->GetDesc()->Packet(p);
}

void SendGuildWarScore(DWORD dwGuild, DWORD dwGuildOpp, int iDelta, int iBetScoreDelta)
{
	network::GDOutputPacket<network::GDGuildWarScorePacket> p;

	p->set_guild_gain_point(dwGuild);
	p->set_guild_opponent(dwGuildOpp);
	p->set_score(iDelta);
	p->set_bet_score(iBetScoreDelta);

	db_clientdesc->DBPacket(p);
	sys_log(0, "SendGuildWarScore %u %u %d", dwGuild, dwGuildOpp, iDelta);
}

void CGuildManager::Kill(LPCHARACTER killer, LPCHARACTER victim)
{
	if (!killer->IsPC())
		return;

	if (!victim->IsPC())
		return;

	if (killer->GetWarMap())
	{
		killer->GetWarMap()->OnKill(killer, victim);
		return;
	}

	CGuild * gAttack = killer->GetGuild();
	CGuild * gDefend = victim->GetGuild();

	if (!gAttack || !gDefend)
		return;

	if (gAttack->GetGuildWarType(gDefend->GetID()) != GUILD_WAR_TYPE_FIELD)
		return;

	if (!gAttack->UnderWar(gDefend->GetID()))
		return;

	SendGuildWarScore(gAttack->GetID(), gDefend->GetID(), victim->GetLevel());
}

void CGuildManager::StopAllGuildWar()
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		CGuild * g = CGuildManager::instance().TouchGuild(it->first);
		CGuild * pg = CGuildManager::instance().TouchGuild(it->second);
		g->EndWar(it->second);
		pg->EndWar(it->first);
	}

	m_GuildWar.clear();
}

void CGuildManager::ReserveWarAdd(const TGuildWarReserve& data)
{
	auto it = m_map_kReserveWar.find(data.dwID);

	CGuildWarReserveForGame * pkReserve;

	if (it != m_map_kReserveWar.end())
		pkReserve = it->second;
	else
	{
		pkReserve = M2_NEW CGuildWarReserveForGame;

		m_map_kReserveWar.insert(std::make_pair(data.dwID, pkReserve));
		m_vec_kReserveWar.push_back(pkReserve);
	}

	thecore_memcpy(&pkReserve->data, &data, sizeof(TGuildWarReserve));

	sys_log(0, "ReserveWarAdd %u gid1 %u power %d gid2 %u power %d handicap %d",
			pkReserve->data.dwID, data.dwGuildFrom, data.lPowerFrom, data.dwGuildTo, data.lPowerTo, data.lHandicap);
}

void CGuildManager::ReserveWarBet(DWORD war_id, const std::string& login, DWORD gold, DWORD guild_id)
{
	auto it = m_map_kReserveWar.find(war_id);

	if (it == m_map_kReserveWar.end())
		return;

	it->second->mapBet.insert(std::make_pair(login, std::make_pair(guild_id, gold)));
}

bool CGuildManager::IsBet(DWORD dwID, const char * c_pszLogin)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(dwID);

	if (it == m_map_kReserveWar.end())
		return true;

	return it->second->mapBet.end() != it->second->mapBet.find(c_pszLogin);
}

void CGuildManager::ReserveWarDelete(DWORD dwID)
{
	sys_log(0, "ReserveWarDelete %u", dwID);
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(dwID);

	if (it == m_map_kReserveWar.end())
		return;

	itertype(m_vec_kReserveWar) it_vec = m_vec_kReserveWar.begin();

	while (it_vec != m_vec_kReserveWar.end())
	{
		if (*it_vec == it->second)
		{
			it_vec = m_vec_kReserveWar.erase(it_vec);
			break;
		}
		else
			++it_vec;
	}

	M2_DELETE(it->second);
	m_map_kReserveWar.erase(it);
}

std::vector<CGuildWarReserveForGame *> & CGuildManager::GetReserveWarRef()
{
	return m_vec_kReserveWar;
}

//
// End of Guild War
//

void CGuildManager::ChangeMaster(DWORD dwGID)
{
	TGuildMap::iterator iter = m_mapGuild.find(dwGID);

	if ( iter != m_mapGuild.end() )
	{
		iter->second->Load(dwGID);
	}

	// ¾÷µ¥ÀÌÆ®µÈ Á¤º¸ º¸³»ÁÖ±â
	DBManager::instance().ReturnQuery(QID_GUILD_CHANGE_LEADER, dwGID, NULL,
		"SELECT 1");
	/*DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::SendGuildDataUpdateToAllMember), iter->second), 
			"SELECT 1");*/

}

