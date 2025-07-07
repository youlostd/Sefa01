#include "stdafx.h"
#ifdef DMG_RANKING

#include <sstream>
#include "../../common/tables.h"
#include "dmg_ranking.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "locale.hpp"
#include "p2p.h"
#include "log.h"
#include "questmanager.h"
#include "db.h"

void CDmgRankingManager::initDmgRankings()
{
	if (g_bAuthServer)
		return;
	
	for (int i = 0; i < TYPE_DMG_MAX_NUM; ++i)
	{
		m_vecDmgRankings[i].clear();
		m_vecDmgRankings[i].reserve(10);
	}

	auto getRanksType = [this](const int &type) {
		std::auto_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery("SELECT name, damage FROM damage_ranking WHERE type = %d ORDER BY damage DESC LIMIT 10", type + 1));
		SQLResult *pRes = pkMsg->Get();

		if (!pRes->uiNumRows)
			return;

		MYSQL_ROW data = NULL;
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			int dmg = 0;
			str_to_number(dmg, data[1]);
			m_vecDmgRankings[type].push_back(TRankDamageEntry(data[0], dmg));
		}
	};

	for (int i = 0; i < TYPE_DMG_MAX_NUM; ++i)
		getRanksType(i);
}

void CDmgRankingManager::registerToDmgRanks(LPCHARACTER ch, TypeDmg type, const int &dmg)
{
	auto &dmgVec = m_vecDmgRankings[type];

	// remove from ranks if it has entry
	for (auto it = dmgVec.begin(); it != dmgVec.end(); ++it)
	{
		if (strcmp(ch->GetName(), it->name) == 0)
		{
			if (dmg <= it->dmg)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You didn't ranked with %d damage. (Rank1: %d, Rank10: %d)"), dmg, dmgVec[0].dmg, dmgVec[9].dmg);
				return;
			}

			dmgVec.erase(it);
			break;
		}
	}

	bool added = false;

	TRankDamageEntry entry(ch->GetName(), dmg);

	// add in ranks
	auto it = dmgVec.begin();
	BYTE rank = 1;
	for (; it != dmgVec.end(); ++it)
	{
		if (dmg >= it->dmg)
		{
			added = true;
			dmgVec.insert(it, entry);
			break;
		}
		rank += 1;
	}

	if (dmgVec.size() < 10 && it == dmgVec.end())
	{
		dmgVec.push_back(entry);
		added = true;
	}

	// remove the 11th position
	if (dmgVec.size() > 10)
		dmgVec.pop_back();

	if (added)
	{
		sendP2PDmgRanking(type, entry);

		char szBuf[512+1];
		sprintf(szBuf, "You placed #[%d] with [%u] dmg on the [%s] ranking!", rank, dmg, LC_TEXT(ch, type == TYPE_DMG_SKILL ? "skill-dmg" : "hit-dmg"));
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, szBuf));

		if (!quest::CQuestManager::instance().GetEventFlag("dmg_ranking_notifyall_disable"))
		{
			char szBuf[512 + 1];
			sprintf(szBuf, 
				"[%s] placed #[%d] with [%u] dmg on the [%s] ranking!", ch->GetName(), rank, dmg, LC_TEXT(LANGUAGE_ENGLISH, (type == TYPE_DMG_SKILL ? "skill-dmg" : "10hits-dmg")));
			SendSuccessNotice(szBuf);
		}
	}
	else if (dmgVec.size() >= 10)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You didn't ranked with %d damage. (Rank1: %d, Rank10: %d)"), dmg, dmgVec[0].dmg, dmgVec[9].dmg);
	}
}

void CDmgRankingManager::registerToDmgRanks(const char * name, TypeDmg type, const int &dmg)
{
	auto &dmgVec = m_vecDmgRankings[type];

	// remove from ranks if it has entry
	for (auto it = dmgVec.begin(); it != dmgVec.end(); ++it)
	{
		if (strcmp(name, it->name) == 0)
		{
			if (dmg <= it->dmg)
				return;

			dmgVec.erase(it);
			break;
		}
	}

	TRankDamageEntry entry(name, dmg);

	// add in ranks
	auto it = dmgVec.begin();
	BYTE rank = 1;
	for (; it != dmgVec.end(); ++it)
	{
		if (dmg >= it->dmg)
		{
			dmgVec.insert(it, entry);
			break;
		}
		rank += 1;
	}

	if (dmgVec.size() < 10 && it == dmgVec.end())
		dmgVec.push_back(entry);

	// remove the 11th position
	if (dmgVec.size() > 10)
		dmgVec.pop_back();
}

void CDmgRankingManager::saveDmgRankings()
{
	if (!g_isDmgRanksProcess)
		return;

	DBManager::instance().Query("TRUNCATE TABLE damage_ranking");

	auto saveRanksType = [this](const int &type) {
		for (const auto &it : m_vecDmgRankings[type])
			DBManager::instance().Query("INSERT INTO damage_ranking (name, type, damage) VALUES ('%s', %d, %d)", it.name, type + 1, it.dmg);
	};

	for (int i = 0; i < TYPE_DMG_MAX_NUM; ++i)
		saveRanksType(i);
}

void CDmgRankingManager::updateDmgRankings(TypeDmg type, const TRankDamageEntry &entry)
{
	registerToDmgRanks(entry.name, type, entry.dmg);
}

void CDmgRankingManager::sendP2PDmgRanking(TypeDmg type, const TRankDamageEntry &entry)
{
	network::GGOutputPacket<network::GGDmgRankingUpdatePacket> pack;
	pack->set_type(type);
	pack->mutable_data()->set_dmg(entry.dmg);
	pack->mutable_data()->set_name(entry.name);

	P2P_MANAGER::instance().Send(pack);
}

void CDmgRankingManager::ShowRanks(LPCHARACTER ch)
{
	ch->tchat("RANKINGS NORMAL:");

	for (const auto &it : m_vecDmgRankings[0])
	{
		ch->tchat("%s : %d", it.name, it.dmg);
	}

	ch->tchat("RANKINGS SKILL:");

	for (const auto &it : m_vecDmgRankings[1])
	{
		ch->tchat("%s : %d", it.name, it.dmg);
	}
}
#endif
