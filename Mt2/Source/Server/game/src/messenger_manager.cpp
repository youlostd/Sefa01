#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "messenger_manager.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "log.h"
#include "config.h"
#include "p2p.h"
#include "crc32.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"

MessengerManager::MessengerManager()
{
}

MessengerManager::~MessengerManager()
{
}

void MessengerManager::Initialize()
{
}

void MessengerManager::Destroy()
{
}

void MessengerManager::P2PLogin(MessengerManager::keyA account)
{
	Login(account, true);
}

void MessengerManager::P2PLogout(MessengerManager::keyA account)
{
	Logout(account);
}

void MessengerManager::Login(MessengerManager::keyA account, bool p2p)
{
	if (m_set_loginAccount.find(account) != m_set_loginAccount.end())
		return;

#ifdef FIX_MESSENGER_QUERY_SPAM
	if (!p2p)
	{
#endif
		DBManager::instance().ReturnQuery(QID_MESSENGER_LIST_LOAD, 0, NULL, "SELECT account, companion FROM messenger_list WHERE account='%s'", account.c_str());
		/*DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&MessengerManager::LoadList), this),
				"SELECT account, companion FROM messenger_list WHERE account='%s'", account.c_str());*/
#ifdef ENABLE_MESSENGER_BLOCK
		DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&MessengerManager::LoadBlockList), this),
				"SELECT account, companion FROM messenger_block_list WHERE account='%s' OR companion = '%s'", account.c_str(), account.c_str());
#endif
#ifdef FIX_MESSENGER_QUERY_SPAM
	}
	else
	{
		std::set<MessengerManager::keyT>::iterator it;
		for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
			SendLogin(*it, account);
#ifdef ENABLE_MESSENGER_BLOCK
		std::set<MessengerManager::keyT>::iterator it2;
		for (it2 = m_InverseBlockRelation[account].begin(); it2 != m_InverseBlockRelation[account].end(); ++it2)
			SendBlockLogin(*it2, account);
#endif
	}
#endif

	m_set_loginAccount.insert(account);
}

void MessengerManager::LoadList(SQLMsg * msg)
{
	if (NULL == msg)
		return;

	if (NULL == msg->Get())
		return;

	if (msg->Get()->uiNumRows == 0)
		return;

	std::string account;

	sys_log(1, "Messenger::LoadList");

	for (uint i = 0; i < msg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && row[1])
		{
			if (account.length() == 0)
				account = row[0];

			m_Relation[row[0]].insert(row[1]);
			m_InverseRelation[row[1]].insert(row[0]);
		}
	}

	SendList(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
		SendLogin(*it, account);
}

void MessengerManager::Logout(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) == m_set_loginAccount.end())
		return;

	m_set_loginAccount.erase(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
	{
		SendLogout(*it, account);
	}

	std::map<keyT, std::set<keyT> >::iterator it2 = m_Relation.begin();

	while (it2 != m_Relation.end())
	{
		it2->second.erase(account);
		++it2;
	}

#ifdef ENABLE_MESSENGER_BLOCK
	std::set<MessengerManager::keyBL>::iterator it61;
	
	for (it61 = m_InverseBlockRelation[account].begin(); it61 != m_InverseBlockRelation[account].end(); ++it61)
	{
		SendBlockLogout(*it61, account);
	}

	std::map<keyBL, std::set<keyBL> >::iterator it3 = m_BlockRelation.begin();

	while (it3 != m_BlockRelation.end())
	{
		it3->second.erase(account);
		++it3;
	}
	m_BlockRelation.erase(account);
#endif

	m_Relation.erase(account);
	//m_map_stMobile.erase(account);
}

#ifdef ENABLE_MESSENGER_BLOCK
bool MessengerManager::CheckMessengerList(std::string ch, std::string tch, BYTE type)
{
	if (type == SYST_FRIEND)
		return IsInList(ch, tch);
	else
		return IsInBlockList(ch, tch);
	//const char* check = type == SYST_BLOCK ? "messenger_block_list" : "messenger_list";
	//// std::unique_ptr<SQLMsg> msg(DBManager::Instance().DirectQuery("SELECT * FROM player.%s", check));
	//std::auto_ptr<SQLMsg> msg(DBManager::Instance().DirectQuery("SELECT * FROM player.%s", check));
	//if (!msg->Get()->uiNumRows)
	//	return false;
	//
	//for (int i = 0; i < (int)msg->Get()->uiNumRows; ++i) {
	//	MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);
	//	if ((row[0] == ch && row[1] == tch) || (row[1] == ch && row[0] == tch))
	//		return true;
	//}
	//return false;
}

bool MessengerManager::IsInBlockList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	auto it = m_BlockRelation.find(account);
	auto it2 = m_BlockRelation.find(companion);
	if (it != m_BlockRelation.end())
		if (it->second.find(companion) != it->second.end())
			return true;

	if (it2 != m_BlockRelation.end())
		if (it2->second.find(account) != it2->second.end())
			return true;

	return false;
}

void MessengerManager::LoadBlockList(SQLMsg * msg)
{
	if (NULL == msg || NULL == msg->Get() || msg->Get()->uiNumRows == 0)
		return;
	
	std::string account;

	for (uint i = 0; i < msg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && row[1])
		{
			if (account.length() == 0)
				account = row[0];

			m_BlockRelation[row[0]].insert(row[1]);
			m_InverseBlockRelation[row[1]].insert(row[0]);
		}
	}

	SendBlockList(account);

	std::set<MessengerManager::keyBL>::iterator it;

	for (it = m_InverseBlockRelation[account].begin(); it != m_InverseBlockRelation[account].end(); ++it)
		SendBlockLogin(*it, account);
}
void MessengerManager::SendBlockList(MessengerManager::keyA account)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());

	if (!ch)
		return;

	LPDESC d = ch->GetDesc();

	// if (!d or m_BlockRelation.find(account) == m_BlockRelation.end() or m_BlockRelation[account].empty())
	if (!d)
		return;

	network::GCOutputPacket<network::GCMessengerBlockListPacket> pack;

	itertype(m_BlockRelation[account]) it = m_BlockRelation[account].begin(), eit = m_BlockRelation[account].end();

	while (it != eit)
	{
		auto elem = pack->add_players();
		elem->set_name(*it);
		elem->set_connected(m_set_loginAccount.find(*it) != m_set_loginAccount.end());

		++it;
	}

	d->Packet(pack);
}
void MessengerManager::SendBlockLogin(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	network::GCOutputPacket<network::GCMessengerBlockLoginPacket> pack;
	pack->set_name(companion);
	d->Packet(pack);
}

void MessengerManager::SendBlockLogout(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (!companion.size())
		return;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	BYTE bLen = companion.size();

	network::GCOutputPacket<network::GCMessengerBlockLogoutPacket> pack;
	pack->set_name(companion);
	d->Packet(pack);
}
///not compleated

void MessengerManager::AddToBlockList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	if (m_BlockRelation[account].find(companion) != m_BlockRelation[account].end())
		return;

	char escapeAccount[CHARACTER_NAME_MAX_LEN * 2 + 1];
	if ((size_t)-1 == DBManager::instance().EscapeString(escapeAccount, sizeof(escapeAccount), account.c_str(), account.length()))
	{
		sys_err("failed to escape account %s", account.c_str());
		return;
	}

	char escapeCompanion[CHARACTER_NAME_MAX_LEN * 2 + 1];
	if ((size_t)-1 == DBManager::instance().EscapeString(escapeCompanion, sizeof(escapeCompanion), companion.c_str(), companion.length()))
	{
		sys_err("failed to escape companion %s", companion.c_str());
		return;
	}

	// sys_log(0, "Messenger Add %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("INSERT INTO messenger_block_list VALUES ('%s', '%s', NOW())", 
		escapeAccount, escapeCompanion);

	__AddToBlockList(account, companion);

	network::GGOutputPacket<network::GGMessengerBlockAddPacket> p2ppck;

	p2ppck->set_account(account.c_str());
	p2ppck->set_companion(companion.c_str());
	P2P_MANAGER::instance().Send(p2ppck);
}
void MessengerManager::__AddToBlockList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	//yeni eklerken
	m_BlockRelation[account].insert(companion);
	m_InverseBlockRelation[companion].insert(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s has been blocked."), companion.c_str());
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(companion.c_str());

	if (tch)
		SendBlockLogin(account, companion);
	else
		SendBlockLogout(account, companion);
}
void MessengerManager::RemoveFromBlockList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	char escapeAccount[CHARACTER_NAME_MAX_LEN * 2 + 1];
	if ((size_t)-1 == DBManager::instance().EscapeString(escapeAccount, sizeof(escapeAccount), account.c_str(), account.length()))
	{
		sys_err("failed to escape account %s", account.c_str());
		return;
	}

	char escapeCompanion[CHARACTER_NAME_MAX_LEN * 2 + 1];
	if ((size_t)-1 == DBManager::instance().EscapeString(escapeCompanion, sizeof(escapeCompanion), companion.c_str(), companion.length()))
	{
		sys_err("failed to escape companion %s", companion.c_str());
		return;
	}

	// sys_log(1, "Messenger Remove %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("DELETE FROM messenger_block_list WHERE account='%s' AND companion = '%s'",
		escapeAccount, escapeCompanion);
			
	__RemoveFromBlockList(account, companion);

	network::GGOutputPacket<network::GGMessengerBlockRemovePacket> p2ppck;

	p2ppck->set_account(account.c_str());
	p2ppck->set_companion(companion.c_str());
	P2P_MANAGER::instance().Send(p2ppck);
}
void MessengerManager::__RemoveFromBlockList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_BlockRelation[account].erase(companion);
	m_InverseBlockRelation[companion].erase(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<메신져> %s 님을 메신저에서 삭제하였습니다."), companion.c_str());
}
void MessengerManager::RemoveAllBlockList(keyA account)
{
	std::set<keyBL>	company(m_BlockRelation[account]);

	char escapeAccount[CHARACTER_NAME_MAX_LEN * 2 + 1];
	if ((size_t)-1 == DBManager::instance().EscapeString(escapeAccount, sizeof(escapeAccount), account.c_str(), account.length()))
	{
		sys_err("failed to escape account %s", account.c_str());
		return;
	}
	DBManager::instance().Query("DELETE FROM messenger_block_list WHERE account='%s' OR companion='%s'",
		escapeAccount, escapeAccount);

	for (std::set<keyT>::iterator iter = company.begin(); iter != company.end(); iter++)
	{
		this->RemoveFromList(account, *iter);
	}

	for (std::set<keyBL>::iterator iter = company.begin(); iter != company.end();)
	{
		company.erase(iter++);
	}

	company.clear();
}
#endif

void MessengerManager::RequestToAdd(LPCHARACTER ch, const char* pszTargetName)
{
	if (!ch || !ch->IsPC())
		return;

	CCI* pCCI = P2P_MANAGER::instance().Find(pszTargetName);
	if (!(pCCI && pCCI->pkDesc))
	{
		LPCHARACTER target = CHARACTER_MANAGER::instance().FindPC(pszTargetName);
		if (target)
			RequestToAdd(ch, target);
		return;
	}

	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»ó´ë¹æÀÌ Ä£±¸ Ãß°¡¸¦ ¹ÞÀ» ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return;
	}

	network::GGOutputPacket<network::GGMessengerRequestPacket> pack;
	pack->set_requestor(ch->GetName());
	pack->set_target_pid(pCCI->dwPID);
	pCCI->pkDesc->Packet(pack);
}

void MessengerManager::RequestToAdd(const char* pszName, LPCHARACTER target)
{
	if (!target || !target->IsPC())
		return;

	CCI* pCCI = P2P_MANAGER::instance().Find(pszName);
	if (!(pCCI && pCCI->pkDesc))
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(pszName);
		if (ch)
			RequestToAdd(ch, target);
		return;
	}

	if (quest::CQuestManager::instance().GetPCForce(target->GetPlayerID())->IsRunning() == true)
	{
		target->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(target, "»ó´ë¹æÀÌ Ä£±¸ Ãß°¡¸¦ ¹ÞÀ» ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return;
	}

	DWORD dw1 = GetCRC32(pszName, strlen(pszName));
	DWORD dw2 = GetCRC32(target->GetName(), strlen(target->GetName()));

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	m_set_requestToAdd.insert(dwComplex);

	target->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", pszName);
}

void MessengerManager::RequestToAdd(LPCHARACTER ch, LPCHARACTER target)
{
	if (!ch->IsPC() || !target->IsPC())
		return;
	
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»ó´ë¹æÀÌ Ä£±¸ Ãß°¡¸¦ ¹ÞÀ» ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return;
	}

	if (IsInList(ch->GetName(), target->GetName()))
		return;

	if (quest::CQuestManager::instance().GetPCForce(target->GetPlayerID())->IsRunning() == true)
		return;

	DWORD dw1 = GetCRC32(ch->GetName(), strlen(ch->GetName()));
	DWORD dw2 = GetCRC32(target->GetName(), strlen(target->GetName()));

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	m_set_requestToAdd.insert(dwComplex);

	target->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", ch->GetName());
}

void MessengerManager::AuthToAdd(MessengerManager::keyA account, MessengerManager::keyA companion, bool bDeny)
{
	DWORD dw1 = GetCRC32(companion.c_str(), companion.length());
	DWORD dw2 = GetCRC32(account.c_str(), account.length());

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	if (m_set_requestToAdd.find(dwComplex) == m_set_requestToAdd.end())
	{
		sys_log(0, "MessengerManager::AuthToAdd : request not exist %s -> %s", companion.c_str(), account.c_str());
		return;
	}

	m_set_requestToAdd.erase(dwComplex);

	if (!bDeny)
	{
		AddToList(companion, account);
		AddToList(account, companion);
	}
}

void MessengerManager::__AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_Relation[account].insert(companion);
	m_InverseRelation[companion].insert(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<¸Þ½ÅÁ®> %s ´ÔÀ» Ä£±¸·Î Ãß°¡ÇÏ¿´½À´Ï´Ù."), companion.c_str());

		quest::CQuestManager::instance().SetAddedFriendName(companion);
		quest::CQuestManager::instance().OnAddFriend(ch->GetPlayerID());
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(companion.c_str());

	if (tch || P2P_MANAGER::instance().Find(companion.c_str()))
		SendLogin(account, companion);
	else
		SendLogout(account, companion);
}

void MessengerManager::AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	if (m_Relation[account].find(companion) != m_Relation[account].end())
		return;

	if (!check_name(account.c_str()) || !check_name(companion.c_str()))
	{
		sys_err("%s failed %s %s", __FUNCTION__, account.c_str(), companion.c_str());
		return;
	}
	
	sys_log(0, "Messenger Add %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("INSERT IGNORE INTO messenger_list VALUES ('%s', '%s')", 
			account.c_str(), companion.c_str());

	__AddToList(account, companion);

	network::GGOutputPacket<network::GGMessengerAddPacket> p2ppck;

	p2ppck->set_account(account.c_str());
	p2ppck->set_companion(companion.c_str());
	P2P_MANAGER::instance().Send(p2ppck);
}

void MessengerManager::__RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_Relation[account].erase(companion);
	m_InverseRelation[companion].erase(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<¸Þ½ÅÁ®> %s ´ÔÀ» ¸Þ½ÅÀú¿¡¼­ »èÁ¦ÇÏ¿´½À´Ï´Ù."), companion.c_str());
}

void MessengerManager::RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	if (!check_name(account.c_str()) || !check_name(companion.c_str()))
	{
		sys_err("%s failed %s %s", __FUNCTION__, account.c_str(), companion.c_str());
		return;
	}
	
	if (!IsInList(account, companion))
	{
		if (test_server)
			sys_err("not in list [%s deletes %s]", account.c_str(), companion.c_str());
		return;
	}

	sys_log(1, "Messenger Remove %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("DELETE FROM messenger_list WHERE account='%s' AND companion = '%s'",
			account.c_str(), companion.c_str());

	__RemoveFromList(account, companion);

	network::GGOutputPacket<network::GGMessengerRemovePacket> p2ppck;

	p2ppck->set_account(account.c_str());
	p2ppck->set_companion(companion.c_str());
	P2P_MANAGER::instance().Send(p2ppck);
}

void MessengerManager::RemoveAllList(keyA account)
{
	std::set<keyT>	company(m_Relation[account]);

	/* SQL Data »èÁ¦ */
	DBManager::instance().Query("DELETE FROM messenger_list WHERE account='%s' OR companion='%s'",
			account.c_str(), account.c_str());

	/* ³»°¡ °¡Áö°íÀÖ´Â ¸®½ºÆ® »èÁ¦ */
	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			iter++ )
	{
		this->RemoveFromList(account, *iter);
	}

	/* º¹»çÇÑ µ¥ÀÌÅ¸ »èÁ¦ */
	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			)
	{
		company.erase(iter++);
	}

	company.clear();
}

bool MessengerManager::IsInList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	auto it = m_Relation.find(account);
	if (it == m_Relation.end())
	{
		it = m_InverseRelation.find(account);
		if (it == m_InverseRelation.end())
			return false;
	}

	if (it->second.empty())
		return false;

	return it->second.find(companion) != it->second.end();
}

void MessengerManager::SendList(MessengerManager::keyA account)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());

	if (!ch)
		return;

	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	if (m_Relation.find(account) == m_Relation.end())
		return;

	if (m_Relation[account].empty())
		return;

	network::GCOutputPacket<network::GCMessengerListPacket> pack;

	itertype(m_Relation[account]) it = m_Relation[account].begin(), eit = m_Relation[account].end();

	while (it != eit)
	{
		auto elem = pack->add_players();
		elem->set_name(*it);
		elem->set_connected(m_set_loginAccount.find(*it) != m_set_loginAccount.end());

		++it;
	}

	d->Packet(pack);
}

void MessengerManager::SendLogin(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	if (ch->GetGMLevel() == GM_PLAYER && GM::get_level(companion.c_str()) != GM_PLAYER)
		return;

	BYTE bLen = companion.size();

	network::GCOutputPacket<network::GCMessengerLoginPacket> pack;
	pack->set_name(companion);
	d->Packet(pack);
}

void MessengerManager::SendLogout(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (!companion.size())
		return;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	BYTE bLen = companion.size();

	network::GCOutputPacket<network::GCMessengerLogoutPacket> pack;
	pack->set_name(companion);;
	d->Packet(pack);
}

