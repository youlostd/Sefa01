#include "stdafx.h"
#include "SpamFilter.h"
#include "db.h"
#include "char.h"
#include "desc.h"
#include <algorithm>

#ifdef ENABLE_SPAM_FILTER
bool CSpamFilter::InitializeSpamFilterTable()
{
	char query[4096];
	snprintf(query, sizeof(query),"SELECT word_1, word_2, sanction, duration FROM banword");
	std::unique_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery(query));

	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from SpamTable");
		return false;
	}

	if (!g_vecStrBanWords.empty())
	{
		sys_log(0, "RELOAD: SpamTable");
		g_vecStrBanWords.clear();
	}

	g_vecStrBanWords.reserve(pRes->uiNumRows);

	MYSQL_ROW data;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TSpamTable t;
		memset(&t, 0, sizeof(TSpamTable));
		int col = 0;

		t.word_1 = data[col++];
		t.word_2 = data[col++];
		t.sanction = data[col++];
		str_to_number(t.duration, data[col++]);

		g_vecStrBanWords.push_back(t);
	}

	return true;
}

static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {return !std::isspace(c);}));
    return s;
}

bool CSpamFilter::IsBannedWord(std::string message, TSpamTable &out)
{
	// ERASE ALL NON ALPHANUMERIC CHARS
	message.erase(std::remove_if(message.begin(), message.end(), [](char c) { return !std::isalnum(c); } ), message.end());

	// Lower case
	std::transform(message.begin(), message.end(), message.begin(), ::tolower);

	for (int i = 0; i < g_vecStrBanWords.size(); ++i)
	{
		auto t = g_vecStrBanWords[i];
		if (message.find(t.word_1) != std::string::npos && message.find(t.word_2) != std::string::npos)
		{
			out = t;
			return true;
		}
	}

	return false;
}

bool CSpamFilter::IsBannedWord(LPCHARACTER ch, std::string message)
{
	TSpamTable t;
	if (!IsBannedWord(message, t))
		return false;

	if (message.find(t.word_1) != std::string::npos && message.find(t.word_2) != std::string::npos)
	{
		if (t.sanction.find("BLOCK_CHAT") != std::string::npos)
		{
			ApplyBlockChatSanction(ch, t.duration);
			ch->DetectionHackLog("BANWORD_BLOCK_CHAT", message.c_str());
		}
		else if (t.sanction.find("BLOCK_ACCOUNT") != std::string::npos)
		{
			ApplyBlockAccountSanction(ch, t.duration);
			ch->DetectionHackLog("BANWORD_BLOCK_ACC", message.c_str());
		}
		else if (t.sanction.find("BLOCK_HWID") != std::string::npos)
		{
			ApplyBlockAccountSanction(ch, t.duration);
			ch->DetectionHackLog("BANWORD_BLOCK_HWID", message.c_str());
		}
		else if (t.sanction.find("TEST") != std::string::npos)
			ch->DetectionHackLog("BANWORD_TEST", message.c_str());
	}
	return true;
}

bool CSpamFilter::IsBannedWord(std::string message)
{
	TSpamTable t;
	if (!IsBannedWord(message, t))
		return false;

	return true;
}

void CSpamFilter::ApplyBlockAccountSanction(LPCHARACTER ch, long duration)
{
	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "SANCTION_BLOCK_ACCOUNT_FOR_%ld_seconds"), duration);

		DBManager::Instance().Query
		(
			"UPDATE player.account SET status= 'BLOCK' WHERE id = %d", ch->GetDesc()->GetAccountTable().id()
		);
		DBManager::Instance().Query
		(
			"UPDATE player.account SET availDt = NOW() + %ld WHERE id = %d", duration, ch->GetDesc()->GetAccountTable().id()
		);

		ch->GetDesc()->DelayedDisconnect(3);
	}
}

void CSpamFilter::ApplyBlockChatSanction(LPCHARACTER ch, long duration)
{
	if (ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "SANCTION_BLOCK_CHAT_FOR_%ld_seconds"), duration);
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, duration, 0, true);
	}
}
#endif
