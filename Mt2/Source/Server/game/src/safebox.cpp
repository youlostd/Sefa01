#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "safebox.h"
#include "packet.h"
#include "char.h"
#include "desc_client.h"
#include "item.h"
#include "item_manager.h"

CSafebox::CSafebox(LPCHARACTER pkChrOwner, int iSize, DWORD dwGold) : m_pkChrOwner(pkChrOwner), m_iSize(MIN(iSize, SAFEBOX_MAX_NUM / 5)), m_lGold(dwGold)
{
	assert(m_pkChrOwner != NULL);
	memset(m_pkItems, 0, sizeof(m_pkItems));

	if (m_iSize)
		m_pkGrid = M2_NEW CGrid(5, m_iSize);
	else
		m_pkGrid = NULL;

	m_bWindowMode = SAFEBOX;
}

CSafebox::~CSafebox()
{
	__Destroy();
}

void CSafebox::SetWindowMode(BYTE bMode)
{
	m_bWindowMode = bMode;
}

void CSafebox::__Destroy()
{
	for (int i = 0; i < SAFEBOX_MAX_NUM; ++i)
	{
		if (m_pkItems[i])
		{
			m_pkItems[i]->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(m_pkItems[i]);

			M2_DESTROY_ITEM(m_pkItems[i]->RemoveFromCharacter());
			m_pkItems[i] = NULL;
		}
	}

	if (m_pkGrid)
	{
		M2_DELETE(m_pkGrid);
		m_pkGrid = NULL;
	}
}

void CSafebox::SendSetPacket(DWORD dwPos)
{
	LPITEM pkItem;
	if (!(pkItem = GetItem(dwPos)))
		return;

	network::GCOutputPacket<network::GCItemSetPacket> pack;
	
	ITEM_MANAGER::Instance().GetPlayerItem(pkItem, pack->mutable_data());
	*pack->mutable_data()->mutable_cell() = TItemPos(m_bWindowMode, dwPos);

	m_pkChrOwner->GetDesc()->Packet(pack);
}

bool CSafebox::Add(DWORD dwPos, LPITEM pkItem)
{
	if (!IsValidPosition(dwPos))
	{
		sys_err("SAFEBOX: item on wrong position at %d (size of grid = %d)", dwPos, m_pkGrid->GetSize());
		return false;
	}

	// if (pkItem->IsCooltime())
	// {
		// m_pkChrOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkChrOwner, "You need to wait %d Minutes until you can move the recently received item."), 8);
		// return false;
	// }
	
	pkItem->SetWindow(m_bWindowMode);
	pkItem->SetCell(m_pkChrOwner, dwPos);
	pkItem->Save(); // 강제로 Save를 불러줘야 한다.
	ITEM_MANAGER::instance().FlushDelayedSave(pkItem);

	m_pkGrid->Put(dwPos, 1, pkItem->GetSize());
	m_pkItems[dwPos] = pkItem;

	SendSetPacket(dwPos);
	sys_log(1, "SAFEBOX: ADD %s %s count %d", m_pkChrOwner->GetName(), pkItem->GetName(), pkItem->GetCount());
	return true;
}

LPITEM CSafebox::Get(DWORD dwPos)
{
	if (!m_pkGrid || dwPos >= m_pkGrid->GetSize())
		return NULL;

	return m_pkItems[dwPos];
}

LPITEM CSafebox::Remove(DWORD dwPos)
{
	LPITEM pkItem = Get(dwPos);

	if (!pkItem)
		return NULL;

	if (!m_pkGrid)
		sys_err("Safebox::Remove : nil grid");
	else
		m_pkGrid->Get(dwPos, 1, pkItem->GetSize());

	pkItem->RemoveFromCharacter();

	m_pkItems[dwPos] = NULL;

	network::GCOutputPacket<network::GCItemSetPacket> pack;
	*pack->mutable_data()->mutable_cell() = TItemPos(m_bWindowMode, dwPos);

	m_pkChrOwner->GetDesc()->Packet(pack);
	sys_log(1, "SAFEBOX: REMOVE %s %s count %d", m_pkChrOwner->GetName(), pkItem->GetName(), pkItem->GetCount());
	return pkItem;
}

void CSafebox::Save()
{
	if (!m_pkChrOwner || !m_pkChrOwner->GetDesc())
		return;
	
	network::GDOutputPacket<network::GDSafeboxSavePacket> p;
	p->set_account_id(m_pkChrOwner->GetAID());
	p->set_gold(m_lGold);

	db_clientdesc->DBPacket(p);
	sys_log(1, "SAFEBOX: SAVE %s", m_pkChrOwner->GetName());
}

bool CSafebox::IsEmpty(DWORD dwPos, BYTE bSize)
{
	if (!m_pkGrid)
		return false;

	return m_pkGrid->IsEmpty(dwPos, 1, bSize);
}

void CSafebox::ChangeSize(int iSize)
{
	// 현재 사이즈가 인자보다 크면 사이즈를 가만 둔다.
	if (m_iSize >= iSize)
		return;

	if (iSize * 5 > SAFEBOX_MAX_NUM)
	{
		sys_err("requested safebox chagnge size is too big (%d)", iSize);
		return;
	}

	m_iSize = iSize;

	CGrid * pkOldGrid = m_pkGrid;

	if (pkOldGrid)
	{
		m_pkGrid = M2_NEW CGrid(pkOldGrid, 5, m_iSize);
		M2_DELETE(pkOldGrid);
	}
	else
		m_pkGrid = M2_NEW CGrid(5, m_iSize);
}

LPITEM CSafebox::GetItem(BYTE bCell)
{
	if (bCell >= 5 * m_iSize)
	{
		sys_err("CHARACTER::GetItem: invalid item cell %d", bCell);
		return NULL;
	}

	return m_pkItems[bCell];
}

#ifdef INCREASE_ITEM_STACK
bool CSafebox::MoveItem(BYTE bCell, BYTE bDestCell, WORD count)
#else
bool CSafebox::MoveItem(BYTE bCell, BYTE bDestCell, BYTE count)
#endif
{
	LPITEM item;
	int max_position = 5 * m_iSize;

	if (bCell >= max_position || bDestCell >= max_position)
		return false;

	if (!(item = GetItem(bCell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	{
		LPITEM item2;

		if ((item2 = GetItem(bDestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum()) // 합칠 수 있는 아이템의 경우
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

#ifdef INCREASE_ITEM_STACK
			count = MIN(ITEM_MAX_COUNT - item2->GetCount(), count);
#else
			count = MIN(200 - item2->GetCount(), count);
#endif

			if (item->GetCount() >= count)
				Remove(bCell);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);

			sys_log(1, "SAFEBOX: STACK %s %d -> %d %s count %d", m_pkChrOwner->GetName(), bCell, bDestCell, item2->GetName(), item2->GetCount());
			return true;
		}

		if (!IsEmpty(bDestCell, item->GetSize()))
			return false;

		m_pkGrid->Get(bCell, 1, item->GetSize());

		if (!m_pkGrid->Put(bDestCell, 1, item->GetSize()))
		{
			m_pkGrid->Put(bCell, 1, item->GetSize());
			return false;
		}
		else
		{
			m_pkGrid->Get(bDestCell, 1, item->GetSize());
			m_pkGrid->Put(bCell, 1, item->GetSize());
		}

		sys_log(1, "SAFEBOX: MOVE %s %d -> %d %s count %d", m_pkChrOwner->GetName(), bCell, bDestCell, item->GetName(), item->GetCount());

		Remove(bCell);
		Add(bDestCell, item);
	}

	return true;
}

bool CSafebox::IsValidPosition(DWORD dwPos)
{
	if (!m_pkGrid)
		return false;

	if (dwPos >= m_pkGrid->GetSize())
		return false;

	return true;
}

