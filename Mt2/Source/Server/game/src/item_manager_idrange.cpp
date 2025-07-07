
#include "stdafx.h"
#include "desc_client.h"
#include "item_manager.h"

int touch(const char *path)
{
	FILE	*fp;

	if ( !(fp = fopen(path, "a")) )
	{
		sys_err("touch failed");
		return (-1);
	}

	fclose(fp);
	return 0;
}

DWORD ITEM_MANAGER::GetNewID()
{
	assert(m_dwCurrentID != 0);

	if ( m_dwCurrentID >= m_ItemIDRange.max_id() )
	{
		if ( m_ItemIDSpareRange.min_id() == 0 || m_ItemIDSpareRange.max_id() == 0 || m_ItemIDSpareRange.usable_item_id_min() == 0 )
		{
			for ( int i=0; i < 10; i++ ) sys_err("ItemIDRange: FATAL ERROR!!! no more item id");
			touch(".killscript");
			thecore_shutdown();
			return 0;
		}
		else
		{
			sys_log(0, "ItemIDRange: First Range is full. Change to SpareRange %u ~ %u %u",
					m_ItemIDSpareRange.min_id(), m_ItemIDSpareRange.max_id(), m_ItemIDSpareRange.usable_item_id_min());

			db_clientdesc->DBPacket(network::TGDHeader::REQ_SPARE_ITEM_ID_RANGE);

			SetMaxItemID(m_ItemIDSpareRange);

			m_ItemIDSpareRange.set_min_id(0);
			m_ItemIDSpareRange.set_max_id(0);
			m_ItemIDSpareRange.set_usable_item_id_min(0);
		}
	}

	return (m_dwCurrentID++);
}

bool ITEM_MANAGER::SetMaxItemID(network::TItemIDRangeTable range)
{
	m_ItemIDRange = range;

	if (m_ItemIDRange.min_id() == 0 || m_ItemIDRange.max_id() == 0 || m_ItemIDRange.usable_item_id_min() == 0)
	{
		for (int i = 0; i < 10; i++) sys_err("ItemIDRange: FATAL ERROR!!! ITEM ID RANGE is not set [%d~%d, start %d].",
			m_ItemIDRange.min_id(), m_ItemIDRange.max_id(), m_ItemIDRange.usable_item_id_min());
		thecore_shutdown();
		return false;
	}

	m_dwCurrentID = range.usable_item_id_min();

	sys_log(0, "ItemIDRange: %u ~ %u %u", m_ItemIDRange.min_id(), m_ItemIDRange.max_id(), m_dwCurrentID);

	return true;
}

bool ITEM_MANAGER::SetMaxSpareItemID(network::TItemIDRangeTable range)
{
	if (range.min_id() == 0 || range.max_id() == 0 || range.usable_item_id_min() == 0)
	{
		for (int i = 0; i < 10; i++) sys_err("ItemIDRange: FATAL ERROR!!! Spare ITEM ID RANGE is not set");
		return false;
	}

	m_ItemIDSpareRange = range;

	sys_log(0, "ItemIDRange: New Spare ItemID Range Recv %u ~ %u %u",
		m_ItemIDSpareRange.min_id(), m_ItemIDSpareRange.max_id(), m_ItemIDSpareRange.usable_item_id_min());

	return true;
}

