#include "stdafx.h"

#ifdef __GUILD_SAFEBOX__
#include "GuildSafeboxManager.h"
#include "DBManager.h"
#include "QID.h"
#include "Peer.h"
#include "ClientManager.h"
#include "Cache.h"

extern BOOL g_test_server;

/////////////////////////////////////////
// CGuildSafebox
/////////////////////////////////////////
CGuildSafebox::CGuildSafebox(DWORD dwGuildID)
{
	m_dwGuildID = dwGuildID;
	m_bSize = 0;
	m_szPassword[0] = '\0';
	m_ullGold = 0;

	memset(m_bItemGrid, 0, sizeof(m_bItemGrid));
}
CGuildSafebox::~CGuildSafebox()
{
}

void CGuildSafebox::ChangeSize(BYTE bNewSize, CPeer* pkPeer)
{
	SetSize(bNewSize);

	network::DGOutputPacket<network::DGGuildSafeboxPacket> p;
	p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_SIZE);
	p->set_guild_id(GetGuildID());
	p->set_size(bNewSize);
	CClientManager::Instance().ForwardPacket(p, 0, pkPeer);

	CGuildSafeboxManager::Instance().SaveSafebox(this);
}

void CGuildSafebox::ChangeGold(LONGLONG llChange)
{
	SetGold(GetGold() + llChange);

	network::DGOutputPacket<network::DGGuildSafeboxPacket> p;
	p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_GOLD);
	p->set_guild_id(GetGuildID());
	p->set_gold(GetGold());
	ForwardPacket(p);

	if (llChange)
		CGuildSafeboxManager::Instance().SaveSafebox(this);
}

void CGuildSafebox::LoadItems(SQLMsg* pMsg)
{
	int iNumRows = pMsg->Get()->uiNumRows;

	if (iNumRows)
	{
		network::TItemData item;

		item.set_owner(GetGuildID());
		item.mutable_cell()->set_window_type(GUILD_SAFEBOX);

		for (int i = 0; i < iNumRows; ++i)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			CreateItemTableFromRow(row, &item);
			
			if (item.cell().cell() >= GUILD_SAFEBOX_MAX_NUM)
			{
				sys_err("cannot load item by ID %u for guild %u (cell out of range)", item.id(), GetGuildID());
				continue;
			}

			auto pProto = CClientManager::Instance().GetItemTable(item.vnum());
			if (!pProto)
			{
				sys_err("cannot load item by ID %u for guild %u (wrong vnum %u)", item.id(), GetGuildID(), item.vnum());
				continue;
			}

			m_pItems[item.cell().cell()].reset(new network::TItemData(item));
			for (int i = 0; i < pProto->size(); ++i)
				m_bItemGrid[item.cell().cell() + 5 * i] = true;

			if (g_test_server)
				sys_log(0, "CGuildSafebox::LoadItems %u: LoadItem %u vnum %u %s %dx",
					GetGuildID(), item.id(), item.vnum(), pProto->locale_name(LANGUAGE_DEFAULT).c_str(), item.count());
		}
	}
}

void CGuildSafebox::DeleteItems()
{
	char szDeleteQuery[256];
	snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM item WHERE window = %u AND owner_id = %u", GUILD_SAFEBOX, GetGuildID());
	CDBManager::Instance().AsyncQuery(szDeleteQuery);

	for (int i = 0; i < GUILD_SAFEBOX_MAX_NUM; ++i)
	{
		if (GetItem(i))
		{
			CGuildSafeboxManager::Instance().FlushItem(m_pItems[i].get(), false);
			m_pItems[i].reset();
		}
	}
	memset(m_bItemGrid, 0, sizeof(m_bItemGrid));
}

bool CGuildSafebox::IsValidCell(BYTE bCell, BYTE bSize) const
{
	// pos out of max window size
	if (bCell >= GUILD_SAFEBOX_MAX_NUM - GUILD_SAFEBOX_ITEM_WIDTH * (bSize - 1))
		return false;

	// all good
	return true;
}

bool CGuildSafebox::IsFree(BYTE bPos, BYTE bSize) const
{
	if (!IsValidCell(bPos, bSize))
	{
		sys_err("IsFree: invalid cell %u", bPos);
		return false;
	}

	// item cannot split over two pages
	const BYTE bPageSize = GUILD_SAFEBOX_ITEM_WIDTH * GUILD_SAFEBOX_ITEM_HEIGHT;
	if (bPos / bPageSize != (bPos + GUILD_SAFEBOX_ITEM_WIDTH * (bSize - 1)) / bPageSize)
	{
		sys_err("IsFree: different pages");
		return false;
	}

	// not enough pages owned
	if (bPos / bPageSize >= m_bSize)
	{
		sys_err("not enough pages (page %u requested, owned %u)", bPos / bPageSize + 1, m_bSize);
		return false;
	}

	// there is already an item on this slot
	for (int i = 0; i < bSize; ++i)
	{
		if (m_bItemGrid[bPos + i * GUILD_SAFEBOX_ITEM_WIDTH])
			return false;
	}

	// all good
	return true;
}

void CGuildSafebox::RequestAddItem(CPeer* pkPeer, DWORD dwPID, const char* c_pszPlayerName, DWORD dwHandle, const network::TItemData* pItem)
{
	auto pProto = CClientManager::instance().GetItemTable(pItem->vnum());
	if (!pProto)
	{
		sys_err("cannot get proto %u", pItem->vnum());
		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE id = %u", pItem->id());
		CDBManager::instance().AsyncQuery(szQuery);
		GiveItemToPlayer(pkPeer, dwHandle, pItem);
		return;
	}

	if (!IsFree(pItem->cell().cell(), pProto->size()))
	{
		sys_err("no free space at pos %u size %u", pItem->cell().cell(), pProto->size());
		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE id = %u", pItem->id());
		CDBManager::instance().AsyncQuery(szQuery);
		GiveItemToPlayer(pkPeer, dwHandle, pItem);
		return;
	}

	sys_log(0, "CGuildSafebox::RequestAddItem: item %u %-20s to %u (guild %u)",
		pItem->id(), pProto->locale_name(LANGUAGE_DEFAULT).c_str(), pItem->cell().cell(), GetGuildID());

	m_pItems[pItem->cell().cell()].reset(new network::TItemData(*pItem));
	m_pItems[pItem->cell().cell()]->set_owner(GetGuildID());
	m_pItems[pItem->cell().cell()]->mutable_cell()->set_window_type(GUILD_SAFEBOX);
	for (int i = 0; i < pProto->size(); ++i)
		m_bItemGrid[pItem->cell().cell() + i * GUILD_SAFEBOX_ITEM_WIDTH] = true;

	CGuildSafeboxManager::Instance().SaveItem(m_pItems[pItem->cell().cell()].get());

	SendItemPacket(pItem->cell().cell());

	AddGuildSafeboxLog(GUILD_SAFEBOX_LOG_ITEM_GIVE, dwPID, c_pszPlayerName, pItem, 0);
}

void CGuildSafebox::RequestMoveItem(BYTE bSrcSlot, BYTE bTargetSlot)
{
	if (!IsValidCell(bSrcSlot))
		return;

	auto pItem = m_pItems[bSrcSlot].get();
	if (!pItem)
		return;

	auto pProto = CClientManager::instance().GetItemTable(pItem->vnum());
	if (!pProto)
		return;

	if (!IsFree(bTargetSlot, pProto->size()))
		return;

	sys_log(0, "CGuildSafebox::RequestMoveItem: item %u %-20s from %u to %u (guild %u)",
		pItem->id(), pProto->locale_name(LANGUAGE_DEFAULT).c_str(), bSrcSlot, bTargetSlot, GetGuildID());

	m_pItems[bSrcSlot].release();
	for (int i = 0; i < pProto->size(); ++i)
		m_bItemGrid[bSrcSlot + i * GUILD_SAFEBOX_ITEM_WIDTH] = false;

	pItem->mutable_cell()->set_cell(bTargetSlot);
	m_pItems[bTargetSlot].reset(pItem);
	for (int i = 0; i < pProto->size(); ++i)
		m_bItemGrid[bTargetSlot + i * GUILD_SAFEBOX_ITEM_WIDTH] = true;

	CGuildSafeboxManager::Instance().SaveItem(pItem);

	SendItemPacket(bSrcSlot);
	SendItemPacket(bTargetSlot);
}

void CGuildSafebox::RequestTakeItem(CPeer* pkPeer, DWORD dwPID, const char* c_pszPlayerName, DWORD dwHandle, BYTE bSlot, BYTE bTargetWindow, WORD wTargetSlot)
{
	if (!IsValidCell(bSlot))
		return;

	auto pItem = m_pItems[bSlot].get();
	if (!pItem)
		return;

	auto pProto = CClientManager::Instance().GetItemTable(pItem->vnum());
	if (!pProto)
		return;

	m_pItems[bSlot].release();
	for (int i = 0; i < pProto->size(); ++i)
		m_bItemGrid[bSlot + i * GUILD_SAFEBOX_ITEM_WIDTH] = false;

	SendItemPacket(bSlot);

	AddGuildSafeboxLog(GUILD_SAFEBOX_LOG_ITEM_TAKE, dwPID, c_pszPlayerName, pItem, 0);

	pItem->mutable_cell()->set_window_type(bTargetWindow);
	pItem->mutable_cell()->set_cell(wTargetSlot);
	GiveItemToPlayer(pkPeer, dwHandle, pItem);

	CGuildSafeboxManager::Instance().FlushItem(pItem, false);
	delete pItem;
}

void CGuildSafebox::GiveItemToPlayer(CPeer* pkPeer, DWORD dwHandle, const network::TItemData* pItem)
{
	sys_log(0, "CGuildSafebox::GiveItemToPlayer: id %u vnum %u socket %u %u %u", pItem->id(), pItem->vnum(),
		pItem->sockets(0), pItem->sockets(1), pItem->sockets(2));

	network::DGOutputPacket<network::DGGuildSafeboxPacket> p;
	p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_GIVE);
	*p->mutable_item() = *pItem;
	pkPeer->Packet(p, dwHandle);
}

void CGuildSafebox::SendItemPacket(BYTE bCell)
{
	network::DGOutputPacket<network::DGGuildSafeboxPacket> p;
	
	if (auto pItem = m_pItems[bCell].get())
	{
		p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_SET);
		*p->mutable_item() = *pItem;
	}
	else
	{
		p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_DEL);
		p->mutable_item()->mutable_cell()->set_cell(bCell);
	}

	ForwardPacket(p);
}

void CGuildSafebox::LoadItems(CPeer* pkPeer, DWORD dwHandle)
{
	network::DGOutputPacket<network::DGGuildSafeboxPacket> p;
	p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_LOAD);
	p->set_guild_id(GetGuildID());
	p->set_gold(GetGold());

	if (m_set_ForwardPeer.find(pkPeer) != m_set_ForwardPeer.end())
	{
		sys_err("already loaded items for channel %u %s", pkPeer->GetChannel(), pkPeer->GetHost());
		pkPeer->Packet(p, dwHandle);
		return;
	}

	for (int i = 0; i < GUILD_SAFEBOX_MAX_NUM; ++i)
	{
		if (auto pkItem = GetItem(i))
			*p->add_items() = *pkItem;
	}

	sys_log(0, "CGuildSafebox::LoadItems [%u] for channel %u %s [Load %d logs] guild %d", p->items_size(), pkPeer->GetChannel(), pkPeer->GetHost(), m_vec_GuildSafeboxLog.size(), GetGuildID());
	pkPeer->Packet(p, dwHandle);

	for (int i = 0; i < m_vec_GuildSafeboxLog.size(); ++i)
	{
		sys_log(0, "CGuildSafebox::LoadItems for(%i)", i);
		p->Clear();
		p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_LOG);
		p->set_guild_id(GetGuildID());
		*p->mutable_added_log() = m_vec_GuildSafeboxLog[i];

		pkPeer->Packet(p);
	}

	AddPeer(pkPeer);
}

void CGuildSafebox::AddGuildSafeboxLog(BYTE bType, DWORD dwPID, const char* c_pszPlayerName, const network::TItemData* pPlayerItem, LONGLONG llGold)
{
	network::TGuildSafeboxLogTable kTable;

	kTable.set_type(bType);
	kTable.set_pid(dwPID);
	kTable.set_player_name(c_pszPlayerName);
	if (pPlayerItem)
		*kTable.mutable_item() = *pPlayerItem;
	kTable.set_gold(llGold);
	kTable.set_time(time(0));

	AddGuildSafeboxLog(kTable);

	// save
	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "INSERT INTO guild_safebox_log (guild_id, `type`, pid, item_id, item_vnum, item_count, time) VALUES (%u, %u, %u, %u, %llu, %u, %u)",
		GetGuildID(), bType, dwPID, kTable.item().id(), pPlayerItem ? (long long)pPlayerItem->vnum() : llGold, kTable.item().count(), kTable.time());
	CDBManager::instance().AsyncQuery(szQuery);

	// clear too many logs
	if (m_vec_GuildSafeboxLog.size() > GUILD_SAFEBOX_LOG_MAX_COUNT)
	{
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM guild_safebox_log WHERE guild_id = %u ORDER BY time ASC LIMIT 1", GetGuildID());
		CDBManager::Instance().AsyncQuery(szQuery);
	}
}

void CGuildSafebox::AddGuildSafeboxLog(const network::TGuildSafeboxLogTable& rkSafeboxLog)
{
	m_vec_GuildSafeboxLog.push_back(rkSafeboxLog);

	network::DGOutputPacket<network::DGGuildSafeboxPacket> p;
	p->set_sub_header(HEADER_DG_GUILD_SAFEBOX_LOG);
	*p->mutable_added_log() = rkSafeboxLog;
	ForwardPacket(p);
}

void CGuildSafebox::AddPeer(CPeer* pkPeer)
{
	if (m_set_ForwardPeer.find(pkPeer) == m_set_ForwardPeer.end())
		m_set_ForwardPeer.insert(pkPeer);
}

void CGuildSafebox::ErasePeer(CPeer* pkPeer)
{
	m_set_ForwardPeer.erase(pkPeer);
}

void CGuildSafebox::ForwardPacket(network::DGOutputPacket<network::DGGuildSafeboxPacket>& pack, CPeer* pkExceptPeer)
{
	for (CPeer* peer : m_set_ForwardPeer)
	{
		if (peer == pkExceptPeer)
			continue;

		pack->set_guild_id(GetGuildID());
		peer->Packet(pack);
	}
}

/////////////////////////////////////////
// CGuildSafeboxManager
/////////////////////////////////////////
CGuildSafeboxManager::CGuildSafeboxManager()
{
	m_dwLastFlushItemTime = time(0);
	m_dwLastFlushSafeboxTime = time(0);
}
CGuildSafeboxManager::~CGuildSafeboxManager()
{
	m_map_GuildSafebox.clear();
	m_map_DelayedItemSave.clear();
}

void CGuildSafeboxManager::Initialize()
{
	std::auto_ptr<SQLMsg> pMsg(CDBManager::Instance().DirectQuery("SELECT guild_id, size, password, gold FROM guild_safebox ORDER BY guild_id ASC"));

	char szLogQuery[2048];
	snprintf(szLogQuery, sizeof(szLogQuery), "SELECT guild_safebox_log.guild_id, guild_safebox_log.`type`, guild_safebox_log.pid, player.name, guild_safebox_log.item_id, "
		"guild_safebox_log.item_vnum, guild_safebox_log.item_count, guild_safebox_log.time, %s "
		"FROM guild_safebox_log "
		"LEFT JOIN item ON item.id = guild_safebox_log.item_id "
		"LEFT JOIN player ON player.id = guild_safebox_log.pid "
		"ORDER BY guild_safebox_log.guild_id, guild_safebox_log.time ASC", GetItemQueryKeyPart(true));
	std::auto_ptr<SQLMsg> pMsgLog(CDBManager::Instance().DirectQuery(szLogQuery));
	
	MYSQL_ROW row_log = mysql_fetch_row(pMsgLog->Get()->pSQLResult);

	uint32_t uiNumRows = pMsg->Get()->uiNumRows;
	if (uiNumRows)
	{
		for (int i = 0; i < uiNumRows; ++i)
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			int col = 0;

			// id
			DWORD dwGuildID;
			str_to_number(dwGuildID, row[col++]);
			// size
			BYTE bSize;
			str_to_number(bSize, row[col++]);
			// passwd
			const char* szPassword = row[col++];
			// gold
			ULONGLONG ullGold;
			str_to_number(ullGold, row[col++]);

			// create class
			CGuildSafebox* pGuildSafebox = new CGuildSafebox(dwGuildID);
			pGuildSafebox->SetSize(bSize);
			pGuildSafebox->SetPassword(szPassword);
			pGuildSafebox->SetGold(ullGold);
			m_map_GuildSafebox.insert(std::pair<DWORD, std::unique_ptr<CGuildSafebox>>(dwGuildID, std::unique_ptr<CGuildSafebox>(pGuildSafebox)));

			while (row_log)
			{
				int col_log = 0;

				DWORD dwLogGuildID;
				str_to_number(dwLogGuildID, row_log[col_log++]);

				while (dwLogGuildID < dwGuildID)
				{
					if (!(row_log = mysql_fetch_row(pMsgLog->Get()->pSQLResult)))
					{
						if (g_test_server)
							sys_log(0, "CGuildSafeboxManager: GuildID %d < %d", dwLogGuildID, dwGuildID);
						break;
					}

					str_to_number(dwLogGuildID, row_log[0]);
				}

				if (dwLogGuildID != dwGuildID)
				{
					if (g_test_server)
						sys_log(0, "CGuildSafeboxManager: GuildID %d != %d", dwLogGuildID, dwGuildID);
					break;
				}

				network::TGuildSafeboxLogTable kLogTable;

				kLogTable.set_type(std::stoul(row_log[col_log++]));
				kLogTable.set_pid(std::stoul(row_log[col_log++]));
				if (row_log[col_log])
					kLogTable.set_player_name(row_log[col_log]);
				++col_log;
				kLogTable.mutable_item()->set_id(std::stoul(row_log[col_log++]));
				if (kLogTable.type() == GUILD_SAFEBOX_LOG_ITEM_GIVE || kLogTable.type() == GUILD_SAFEBOX_LOG_ITEM_TAKE)
					kLogTable.mutable_item()->set_vnum(std::stoul(row_log[col_log++]));
				else
					kLogTable.set_gold(std::stoull(row_log[col_log++]));
				kLogTable.mutable_item()->set_count(std::stoul(row_log[col_log++]));
				kLogTable.set_time(std::stoul(row_log[col_log++]));

				if (row_log[col_log])
				{
					DWORD dwCheckVnum;
					str_to_number(dwCheckVnum, row_log[col_log]);
					
					if (dwCheckVnum == kLogTable.item().vnum())
						col_log = CreateItemTableFromRow(row_log, kLogTable.mutable_item(), col_log);
				}

				pGuildSafebox->AddGuildSafeboxLog(kLogTable);

				row_log = mysql_fetch_row(pMsgLog->Get()->pSQLResult);
			}

			// load items
			char szItemQuery[512];
			snprintf(szItemQuery, sizeof(szItemQuery), "SELECT %s FROM item WHERE window = %u AND owner_id = %u",
				GetItemQueryKeyPart(true), GUILD_SAFEBOX, dwGuildID);
			CDBManager::Instance().ReturnQuery(szItemQuery, QID_GUILD_SAFEBOX_ITEM_LOAD, 0, new GuildHandleInfo(dwGuildID));

			// log
			sys_log(0, "CGuildSafeboxManager::Initialize: Load Guildsafebox: %u (size %d)", dwGuildID, bSize);
		}
	}
}

void CGuildSafeboxManager::Destroy()
{
	FlushItems(true);
	FlushSafeboxes(true);
}

CGuildSafebox* CGuildSafeboxManager::GetSafebox(DWORD dwGuildID)
{
	auto it = m_map_GuildSafebox.find(dwGuildID);
	if (it == m_map_GuildSafebox.end())
		return NULL;
	return it->second.get();
}

void CGuildSafeboxManager::DestroySafebox(DWORD dwGuildID)
{
	CGuildSafebox* pSafebox;
	if (!(pSafebox = GetSafebox(dwGuildID)))
		return;

	pSafebox->DeleteItems();

	char szDeleteQuery[256];
	snprintf(szDeleteQuery, sizeof(szDeleteQuery), "DELETE FROM guild_safebox WHERE guild_id = %u", dwGuildID);
	CDBManager::Instance().AsyncQuery(szDeleteQuery);

	delete pSafebox;
	m_map_GuildSafebox.erase(dwGuildID);
}

void CGuildSafeboxManager::SaveItem(network::TItemData* pItem)
{
	if (m_map_DelayedItemSave.find(pItem) == m_map_DelayedItemSave.end())
		m_map_DelayedItemSave.insert(pItem);
}

void CGuildSafeboxManager::FlushItem(network::TItemData* pItem, bool bSave)
{
	if (m_map_DelayedItemSave.find(pItem) != m_map_DelayedItemSave.end())
	{
		if (bSave)
			SaveSingleItem(pItem);
		m_map_DelayedItemSave.erase(pItem);
	}
}

DWORD CGuildSafeboxManager::FlushItems(bool bForce, DWORD maxCnt)
{
	if (!bForce && time(0) - m_dwLastFlushItemTime <= 10 * 60)
		return 0;
	DWORD retCnt = 0;
	m_dwLastFlushItemTime = time(0);

	sys_log(0, "CGuildSafeboxManager::FlushItems: flush %u", m_map_DelayedItemSave.size());

	for (auto it = m_map_DelayedItemSave.begin(); it != m_map_DelayedItemSave.end();)
	{
		SaveSingleItem(*it);
		++retCnt;
		if (maxCnt)
		{
			it = m_map_DelayedItemSave.erase(it);
			if (retCnt >= maxCnt)
				break;
		}
		else
			++it;
	}

	if (!maxCnt)
		m_map_DelayedItemSave.clear();
	return retCnt;
}

void CGuildSafeboxManager::SaveSingleItem(network::TItemData* pItem)
{
	char szSaveQuery[QUERY_MAX_LEN];

	if (!pItem->owner())
	{
		snprintf(szSaveQuery, sizeof(szSaveQuery), "DELETE FROM player.item WHERE id = %u AND window = %u", pItem->id(), GUILD_SAFEBOX);
		delete pItem;
	}
	else
	{
		snprintf(szSaveQuery, sizeof(szSaveQuery), "REPLACE INTO player.item (owner_id, %s) VALUES (%u, %s)", GetItemQueryKeyPart(false), pItem->owner(), GetItemQueryValuePart(pItem));
	}

	if (g_test_server)
		sys_log(0, "CGuildSafeboxManager::SaveSingleItem: %s", szSaveQuery);

	CDBManager::Instance().AsyncQuery(szSaveQuery);
}

void CGuildSafeboxManager::SaveSafebox(CGuildSafebox* pSafebox)
{
	if (m_map_DelayedSafeboxSave.find(pSafebox) == m_map_DelayedSafeboxSave.end())
		m_map_DelayedSafeboxSave.insert(pSafebox);
}

void CGuildSafeboxManager::FlushSafebox(CGuildSafebox* pSafebox, bool bSave)
{
	if (m_map_DelayedSafeboxSave.find(pSafebox) != m_map_DelayedSafeboxSave.end())
	{
		if (bSave)
			SaveSingleSafebox(pSafebox);
		m_map_DelayedSafeboxSave.erase(pSafebox);
	}
}

DWORD CGuildSafeboxManager::FlushSafeboxes(bool bForce, DWORD maxCnt)
{
	if (!bForce && time(0) - m_dwLastFlushSafeboxTime <= 10 * 60)
		return 0;

	m_dwLastFlushSafeboxTime = time(0);
	DWORD retCnt = 0;
	sys_log(0, "CGuildSafeboxManager::FlushSafeboxes: flush %u", m_map_DelayedSafeboxSave.size());

	for (auto it = m_map_DelayedSafeboxSave.begin(); it != m_map_DelayedSafeboxSave.end();)
	{
		SaveSingleSafebox(*it);
		++retCnt;
		if (maxCnt)
		{
			it = m_map_DelayedSafeboxSave.erase(it);
			if (retCnt >= maxCnt)
				break;
		}
		else
			++it;
	}

	if (!maxCnt)
		m_map_DelayedSafeboxSave.clear();
	return retCnt;
}

void CGuildSafeboxManager::SaveSingleSafebox(CGuildSafebox* pSafebox)
{
	char szSaveQuery[QUERY_MAX_LEN];

	snprintf(szSaveQuery, sizeof(szSaveQuery), "UPDATE player.guild_safebox SET size = %u, gold = %llu WHERE guild_id = %u",
		pSafebox->GetSize(), pSafebox->GetGold(), pSafebox->GetGuildID());

	if (g_test_server)
		sys_log(0, "CGuildSafeboxManager::SaveSingleSafebox: %s", szSaveQuery);

	CDBManager::Instance().AsyncQuery(szSaveQuery);
}

void CGuildSafeboxManager::QueryResult(CPeer* pkPeer, SQLMsg* pMsg, int iQIDNum)
{
	std::auto_ptr<GuildHandleInfo> pInfo((GuildHandleInfo*)((CQueryInfo*)pMsg->pvUserData)->pvData);
	CGuildSafebox* pSafebox = GetSafebox(pInfo->dwGuildID);

	if (!pSafebox)
	{
		sys_err("safebox of guild %u does not exist anymore", pInfo->dwGuildID);
		return;
	}

	switch (iQIDNum)
	{
		case QID_GUILD_SAFEBOX_ITEM_LOAD:
			pSafebox->LoadItems(pMsg);
			break;

		default:
			sys_err("unkown qid %u", iQIDNum);
			break;
	}
}

void CGuildSafeboxManager::ProcessPacket(CPeer* pkPeer, DWORD dwHandle, const network::InputPacket& packet)
{
	if (g_test_server)
		sys_log(0, "CGuildSafeboxManager::ProcessPacket: %u", packet.get_header());

	switch (packet.get_header<network::TGDHeader>())
	{
		case network::TGDHeader::GUILD_SAFEBOX_ADD:
			{
				auto p = packet.get<network::GDGuildSafeboxAddPacket>();

				CClientManager::instance().EraseItemCache(p->item().id());

				CGuildSafebox* pSafebox = GetSafebox(p->item().owner());
				if (pSafebox)
				{
					pSafebox->RequestAddItem(pkPeer, p->pid(), p->name().c_str(), dwHandle, &p->item());
				}
				else
				{
					char szQuery[512];
					snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE id = %u", p->item().id());
					CDBManager::instance().AsyncQuery(szQuery);

					network::DGOutputPacket<network::DGGuildSafeboxPacket> pdg;
					pdg->set_sub_header(HEADER_DG_GUILD_SAFEBOX_GIVE);
					*pdg->mutable_item() = p->item();
					pkPeer->Packet(pdg, dwHandle);
				}
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_MOVE:
			{
				auto p = packet.get<network::GDGuildSafeboxMovePacket>();
				CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				if (pSafebox)
				{
					pSafebox->RequestMoveItem(p->source_slot(), p->target_slot());
				}
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_TAKE:
			{
				auto p = packet.get<network::GDGuildSafeboxTakePacket>();

				CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				if (pSafebox)
				{
					pSafebox->RequestTakeItem(pkPeer, p->pid(), p->player_name().c_str(), dwHandle, p->source_pos(), p->target_pos().window_type(), p->target_pos().cell());
				}
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_GIVE_GOLD:
			{
				auto p = packet.get<network::GDGuildSafeboxGiveGoldPacket>();

				CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				if (pSafebox)
				{
					pSafebox->ChangeGold(p->gold());
					pSafebox->AddGuildSafeboxLog(GUILD_SAFEBOX_LOG_GOLD_GIVE, p->pid(), p->name().c_str(), nullptr, p->gold());
				}
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_GET_GOLD:
			{
				auto p = packet.get<network::GDGuildSafeboxGetGoldPacket>();

				CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				if (pSafebox)
				{
					network::DGOutputPacket<network::DGGuildSafeboxPacket> pdg;
					pdg->set_sub_header(HEADER_DG_GUILD_SAFEBOX_GOLD);
					pdg->set_guild_id(p->guild_id());

					if (pSafebox->GetGold() >= p->gold())
					{
						pdg->set_gold(p->gold());

						pSafebox->ChangeGold(-(LONGLONG)p->gold());
						pSafebox->AddGuildSafeboxLog(GUILD_SAFEBOX_LOG_GOLD_TAKE, p->pid(), p->name().c_str(), nullptr, p->gold());
					}
					else
					{
						sys_err("not enough gold in safebox");
					}

					pkPeer->Packet(pdg, dwHandle);
				}
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_CREATE:
			{
				auto p = packet.get<network::GDGuildSafeboxCreatePacket>();

				CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				if (!pSafebox)
				{
					sys_log(0, "CreateGuildSafebox: %u", p->guild_id());

					pSafebox = new CGuildSafebox(p->guild_id());
					pSafebox->SetSize(p->size());
					m_map_GuildSafebox.insert(std::pair<DWORD, std::unique_ptr<CGuildSafebox>>(p->guild_id(), std::unique_ptr<CGuildSafebox>(pSafebox)));

					char szQuery[256];
					snprintf(szQuery, sizeof(szQuery), "INSERT INTO guild_safebox (guild_id, size, password, gold) VALUES "
						"(%u, %u, '', 0)", p->guild_id(), p->size());
					CDBManager::Instance().AsyncQuery(szQuery);

					network::DGOutputPacket<network::DGGuildSafeboxPacket> pdg;
					pdg->set_sub_header(HEADER_DG_GUILD_SAFEBOX_CREATE);
					pdg->set_guild_id(p->guild_id());
					pdg->set_size(p->size());
					CClientManager::Instance().ForwardPacket(pdg, 0, pkPeer);

					pSafebox->AddGuildSafeboxLog(GUILD_SAFEBOX_LOG_CREATE, p->pid(), p->name().c_str(), nullptr, 0);
				}
				else
					sys_err("safebox already created for guild %u", p->guild_id());
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_SIZE:
			{
				auto p = packet.get<network::GDGuildSafeboxSizePacket>();

				CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				if (pSafebox)
				{
					pSafebox->ChangeSize(p->size(), pkPeer);
					pSafebox->AddGuildSafeboxLog(GUILD_SAFEBOX_LOG_SIZE, p->pid(), p->name().c_str(), nullptr, p->size());
				}
			}
				break;

		case network::TGDHeader::GUILD_SAFEBOX_LOAD:
			{
				 auto p = packet.get<network::GDGuildSafeboxLoadPacket>();
				 CGuildSafebox* pSafebox = GetSafebox(p->guild_id());
				 if (pSafebox)
				 {
					 pSafebox->LoadItems(pkPeer, dwHandle);
				 }
			}
				break;

		default:
			sys_err("unkown packet header %u", packet.get_header());
			break;
	}
}

void CGuildSafeboxManager::InitSafeboxCore(google::protobuf::RepeatedPtrField<network::TGuildSafeboxInitial>* data)
{
	for (auto& it : m_map_GuildSafebox)
	{
		auto safebox = it.second.get();

		auto init = data->Add();
		init->set_guild_id(safebox->GetGuildID());
		init->set_size(safebox->GetSize());
		init->set_password(safebox->GetPassword());
		init->set_gold(safebox->GetGold());
	}
}

void CGuildSafeboxManager::Update(DWORD& currCnt, DWORD maxCnt)
{
	if (currCnt < maxCnt)
		currCnt += FlushItems(false, maxCnt - currCnt);
	if (currCnt < maxCnt)
		currCnt += FlushSafeboxes(false, maxCnt - currCnt);
}

void CGuildSafeboxManager::DisconnectPeer(CPeer* pkPeer)
{
	for (auto& it : m_map_GuildSafebox)
		it.second->ErasePeer(pkPeer);
}
#endif
