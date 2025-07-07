// vim:ts=4 sw=4
#include "stdafx.h"
#include "ClientManager.h"
#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"

void CClientManager::LoadEventFlag()
{
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "SELECT szName, lValue FROM quest WHERE dwPID = 0");
	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	SQLResult* pRes = pmsg->Get();
	if (pRes->uiNumRows)
	{
		MYSQL_ROW row;
		while ((row = mysql_fetch_row(pRes->pSQLResult)))
		{
			network::DGOutputPacket<network::DGSetEventFlagPacket> p;
			p->set_flag_name(row[0]);
			p->set_value(atoi(row[1]));
			sys_log(0, "EventFlag Load %s %d", p->flag_name().c_str(), p->value());
			m_map_lEventFlag.insert(std::make_pair(std::string(p->flag_name()), p->value()));
			ForwardPacket(p);
		}
	}
}

void CClientManager::SetEventFlag(std::unique_ptr<network::GDSetEventFlagPacket> p)
{
	network::DGOutputPacket<network::DGSetEventFlagPacket> pack;
	pack->set_flag_name(p->flag_name());
	pack->set_value(p->value());
	ForwardPacket(pack);

	bool bChanged = false;
	long saveVal = p->is_add();
	
	typeof(m_map_lEventFlag.begin()) it = m_map_lEventFlag.find(p->flag_name());
	if (it == m_map_lEventFlag.end())
	{
		bChanged = true;
		m_map_lEventFlag.insert(std::make_pair(std::string(p->flag_name()), p->value()));
	}
	else if (it->second != p->value())
	{
		bChanged = true;
		if (p->is_add())
		{
			it->second += p->value();
			saveVal = it->second;
		}
		else
			it->second = p->value();
	}
	else if (p->is_add() && p->value())
	{
		bChanged = true;
		it->second += p->value();
		saveVal = it->second;
	}

	/*if (bChanged)
		sys_log(0, "HEADER_GD_SET_EVENT_FLAG : Changed CClientmanager::SetEventFlag(%s %d) ", p->flag_name(), saveVal);
	else
		sys_log(0, "HEADER_GD_SET_EVENT_FLAG : No Changed CClientmanager::SetEventFlag(%s %d %d) ", p->flag_name(), p->value(), p->bIsAdd ? 1 : 0);*/
}

void CClientManager::SaveEventFlags()
{
	char szQuery[1024];
	for (auto it = m_map_lEventFlag.begin(); it != m_map_lEventFlag.end(); ++it)
	{
		snprintf(szQuery, sizeof(szQuery),
			"REPLACE INTO quest (dwPID, szName, szState, lValue) VALUES(0, '%s', '', %ld)",
			it->first.c_str(), it->second);
		szQuery[1023] = '\0';

		//CDBManager::instance().ReturnQuery(szQuery, QID_QUEST_SAVE, 0, NULL);
		CDBManager::instance().AsyncQuery(szQuery);
		sys_log(0, "SAVING EVENT_FLAG : CClientManager::SaveEventFlags name: %s val: %ld", it->first.c_str(), it->second);
	}
}

void CClientManager::SendEventFlagsOnSetup(CPeer* peer)
{
	typeof(m_map_lEventFlag.begin()) it;
	for (it = m_map_lEventFlag.begin(); it != m_map_lEventFlag.end(); ++it)
	{
		network::DGOutputPacket<network::DGSetEventFlagPacket> p;
		p->set_flag_name(it->first.c_str());
		p->set_value(it->second);
		
		peer->Packet(p);
	}
}

int CClientManager::GetEventFlag(const std::string& c_rstFlagName) const
{
	auto it = m_map_lEventFlag.find(c_rstFlagName);
	if (it == m_map_lEventFlag.end())
		return 0;

	return it->second;
}

