#include "stdafx.h"
#include "ItemIDRangeManager.h"
#include "Main.h"
#include "DBManager.h"
#include "ClientManager.h"
#include "Peer.h"

CItemIDRangeManager::CItemIDRangeManager()
{
	m_listData.clear();
}

void CItemIDRangeManager::Build()
{
	DWORD dwMin = 0;
	DWORD dwMax = 0;

	for (int i = 0; ; ++i)
	{
		dwMin = cs_dwMinimumRange * (i + 1) + 1;
		dwMax = cs_dwMinimumRange * (i + 2);

		if (dwMax == cs_dwMaxItemID)
			break;

		if (CClientManager::instance().GetItemRange().min_id() <= dwMin &&
			CClientManager::instance().GetItemRange().max_id() >= dwMax)
		{
			continue;
		}

		AppendRange(dwMin, dwMax);
	}
}

void CItemIDRangeManager::AppendRange(DWORD dwMin, DWORD dwMax, DWORD dwMinUsable, bool bFront)
{
	TItemIDRangeTable range;
	if (BuildRange(dwMin, dwMax, range, dwMinUsable) == true)
	{
		if (bFront)
			m_listData.push_front(range);
		else
			m_listData.push_back(range);
	}
}

struct FCheckCollision
{
	bool hasCollision;
	TItemIDRangeTable range;

	FCheckCollision(TItemIDRangeTable data)
	{
		hasCollision = false;
		range = data;
	}

	void operator() (CPeer* peer)
	{
		if (hasCollision == false)
		{
			hasCollision = peer->CheckItemIDRangeCollision(range);
		}
	}
};

TItemIDRangeTable CItemIDRangeManager::GetRange()
{
	TItemIDRangeTable ret;
	ret.set_min_id(0);
	ret.set_max_id(0);
	ret.set_usable_item_id_min(0);

	if (m_listData.size() > 0)
	{
		while (m_listData.size() > 0)
		{
			ret = m_listData.front();
			m_listData.pop_front();

			FCheckCollision f(ret);
			CClientManager::instance().for_each_peer(f);

			if (f.hasCollision == false) return ret;
		}
	}

	for (int i = 0; i < 10; ++i)
		sys_err("ItemIDRange: NO MORE ITEM ID RANGE");

	return ret;
}

bool CItemIDRangeManager::BuildRange(DWORD dwMin, DWORD dwMax, TItemIDRangeTable& range, DWORD dwMinUsable)
{
	char szQuery[1024];
	DWORD dwItemMaxID = 0;
	SQLMsg* pMsg = NULL;
	MYSQL_ROW row;

	snprintf(szQuery, sizeof(szQuery), "SELECT MAX(id) FROM item WHERE id >= %u and id <= %u", dwMin, dwMax);

	pMsg = CDBManager::instance().DirectQuery(szQuery);

	if (pMsg != NULL)
	{
		if (pMsg->Get()->uiNumRows > 0)
		{
			row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			str_to_number(dwItemMaxID, row[0]);
		}
		delete pMsg;
	}

	if (dwItemMaxID == 0 || (dwMinUsable && dwMinUsable > dwItemMaxID))
		dwItemMaxID = dwMinUsable ? dwMinUsable + 1 : dwMin;
	else
		dwItemMaxID++;
	
	if ((dwMax < dwItemMaxID) || (dwMax - dwItemMaxID < cs_dwMinimumRemainCount))
	{
		sys_log(0, "ItemIDRange: Build %u ~ %u start: %u\tNOT USE remain count is below %u",
			   	dwMin, dwMax, dwItemMaxID, cs_dwMinimumRemainCount);
	}
	else
	{
		range.set_min_id(dwMin);
		range.set_max_id(dwMax);
		range.set_usable_item_id_min(dwItemMaxID);

		snprintf(szQuery, sizeof(szQuery), "SELECT COUNT(*) FROM item WHERE id >= %u AND id <= %u", 
				range.usable_item_id_min(), range.max_id());

		pMsg = CDBManager::instance().DirectQuery(szQuery);

		if (pMsg != NULL)
		{
			if (pMsg->Get()->uiNumRows > 0)
			{
				DWORD count = 0;
				row = mysql_fetch_row(pMsg->Get()->pSQLResult);
				str_to_number(count, row[0]);

				if (count > 0)
				{
					sys_err("ItemIDRange: Build: %u ~ %u\thave a item", range.usable_item_id_min(), range.max_id());
					delete pMsg;
					return false;
				}
				else
				{
					sys_log(0, "ItemIDRange: Build: %u ~ %u start:%u", range.min_id(), range.max_id(), range.usable_item_id_min());
					delete pMsg;
					return true;
				}
			}

			delete pMsg;
		}
	}

	return false;
}
