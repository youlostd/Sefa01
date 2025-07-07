#include "stdafx.h"
#include "constants.h"
#include "char.h"
#include "desc.h"
#include "desc_manager.h"
#include "packet.h"
#include "item.h"

/////////////////////////////////////////////////////////////////////////////
// QUICKSLOT HANDLING
/////////////////////////////////////////////////////////////////////////////
void CHARACTER::LoadQuickslot(const ::google::protobuf::RepeatedPtrField<TQuickslot>& data)
{
	for (auto i = 0; i < data.size(); ++i)
	{
		TQuickslot* tmp;
		if (GetQuickslot(i, &tmp))
		{
			if (tmp->type() == data[i].type() && tmp->pos() == data[i].pos())
				continue;
		}

		SetQuickslot(i, data[i]);
	}
}

void CHARACTER::SyncQuickslot(BYTE bType, WORD wOldPos, WORD wNewPos) // bNewPos == 255 ¸é DELETE
{
	if (wOldPos == wNewPos)
		return;

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		if (m_quickslot[i].type() == bType && m_quickslot[i].pos() == wOldPos)
		{
			if (wNewPos == 255)
				DelQuickslot(i);
			else
			{
				TQuickslot slot;

				slot.set_type(bType);
				slot.set_pos(wNewPos);

				SetQuickslot(i, slot);
			}
		}
	}
}

#ifdef __ITEM_SWAP_SYSTEM__
void CHARACTER::SyncSwapQuickslot(BYTE a, BYTE b)
{
	if (a == b)
		return;

	int idx1 = -1, idx2 = -1;

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		if (m_quickslot[i].type() == QUICKSLOT_TYPE_ITEM)
		{
			if (m_quickslot[i].pos() == a)
				idx1 = i;
			else if (m_quickslot[i].pos() == b)
				idx2 = i;
			else
				continue;

			if (idx1 != -1 && idx2 != -1)
				break;
		}
	}

	if (idx1 != -1)
	{
		TQuickslot slot;

		slot.set_type(QUICKSLOT_TYPE_ITEM);
		slot.set_pos(b);

		SetQuickslot(idx1, slot);
	}

	if (idx2 != -1)
	{
		TQuickslot slot;

		slot.set_type(QUICKSLOT_TYPE_ITEM);
		slot.set_pos(a);

		SetQuickslot(idx2, slot);
	}
}
#endif

bool CHARACTER::GetQuickslot(BYTE pos, TQuickslot ** ppSlot)
{
	if (pos >= QUICKSLOT_MAX_NUM)
		return false;

	*ppSlot = &m_quickslot[pos];
	return true;
}

bool CHARACTER::SetQuickslot(BYTE pos, const TQuickslot & rSlot)
{
	network::GCOutputPacket<network::GCQuickslotAddPacket> pack_quickslot_add;

	if (pos >= QUICKSLOT_MAX_NUM)
		return false;

	if (rSlot.type() >= QUICKSLOT_TYPE_MAX_NUM)
		return false;

	if (IsPC())
	{
		if (test_server && !m_abPlayerDataChanged[PC_TAB_CHANGED_QUICKSLOT])
			sys_log(0, "PlayerDataChanged[%s][QUICKSLOT_SET] => True (%u, rslot %u %u)", GetName(), pos, rSlot.type(), rSlot.pos());

		m_abPlayerDataChanged[PC_TAB_CHANGED_QUICKSLOT] = true;
	}

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		if (rSlot.type() == 0)
			continue;
		else if (m_quickslot[i].type() == rSlot.type() && m_quickslot[i].pos() == rSlot.pos())
			DelQuickslot(i);
	}

	TItemPos srcCell(INVENTORY, rSlot.pos());

	switch (rSlot.type())
	{
		case QUICKSLOT_TYPE_ITEM:
			if (false == srcCell.IsDefaultInventoryPosition())
				return false;

			break;

		case QUICKSLOT_TYPE_SKILL:
			if (rSlot.pos() >= SKILL_MAX_NUM)
				return false;

			break;

		case QUICKSLOT_TYPE_COMMAND:
			break;

		default:
			return false;
	}

	m_quickslot[pos] = rSlot;

	if (GetDesc())
	{
		pack_quickslot_add->set_pos(pos);
		*pack_quickslot_add->mutable_slot() = m_quickslot[pos];

		GetDesc()->Packet(pack_quickslot_add);
	}

	return true;
}

bool CHARACTER::DelQuickslot(BYTE pos)
{
	network::GCOutputPacket<network::GCQuickslotDelPacket> pack_quickslot_del;

	if (pos >= QUICKSLOT_MAX_NUM)
		return false;

	if (IsPC())
	{
		if (test_server && !m_abPlayerDataChanged[PC_TAB_CHANGED_QUICKSLOT])
			sys_log(0, "PlayerDataChanged[%s][QUICKSLOT_DEL] => True (%u)", GetName(), pos);

		m_abPlayerDataChanged[PC_TAB_CHANGED_QUICKSLOT] = true;
	}

	m_quickslot[pos].Clear();
	pack_quickslot_del->set_pos(pos);

	GetDesc()->Packet(pack_quickslot_del);
	return true;
}

bool CHARACTER::SwapQuickslot(BYTE a, BYTE b)
{
	network::GCOutputPacket<network::GCQuickslotSwapPacket> pack_quickslot_swap;
	TQuickslot quickslot;

	if (a >= QUICKSLOT_MAX_NUM || b >= QUICKSLOT_MAX_NUM)
		return false;

	if (IsPC())
	{
		if (test_server && !m_abPlayerDataChanged[PC_TAB_CHANGED_QUICKSLOT])
			sys_log(0, "PlayerDataChanged[%s][QUICKSLOT_SWAP] => True (%u, %u)", GetName(), a, b);

		m_abPlayerDataChanged[PC_TAB_CHANGED_QUICKSLOT] = true;
	}

	std::swap(m_quickslot[a], m_quickslot[b]);

	pack_quickslot_swap->set_pos(a);
	pack_quickslot_swap->set_change_pos(b);

	GetDesc()->Packet(pack_quickslot_swap);
	return true;
}

void CHARACTER::ChainQuickslotItem(LPITEM pItem, BYTE bType, BYTE bOldPos)
{
#ifdef __DRAGONSOUL__
	if (pItem->IsDragonSoul())
		return;
#endif

	for ( int i=0; i < QUICKSLOT_MAX_NUM; ++i )
	{
		if ( m_quickslot[i].type() == bType && m_quickslot[i].pos() == bOldPos )
		{
			TQuickslot slot;
			slot.set_type(bType);
			slot.set_pos(pItem->GetCell());

			SetQuickslot(i, slot);

			break;
		}
	}
}

