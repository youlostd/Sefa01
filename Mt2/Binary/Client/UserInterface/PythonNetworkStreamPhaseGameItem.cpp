#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "PythonItem.h"
#include "PythonShop.h"
#include "PythonExchange.h"
#include "PythonSafeBox.h"
#include "PythonCharacterManager.h"

#include "AbstractPlayer.h"
#include "PythonPlayer.h"

#include "../GameLib/ItemManager.h"

using namespace network;

//////////////////////////////////////////////////////////////////////////
// SafeBox

bool CPythonNetworkStream::SendSafeBoxMoneyPacket(BYTE byState, DWORD dwMoney)
{
	assert(!"CPythonNetworkStream::SendSafeBoxMoneyPacket - 사용하지 않는 함수");
	return false;

//	TPacketCGSafeboxMoney kSafeboxMoney;
//	kSafeboxMoney.bHeader = HEADER_CG_SAFEBOX_MONEY;
//	kSafeboxMoney.bState = byState;
//	kSafeboxMoney.dwMoney = dwMoney;
//	if (!Send(kSafeboxMoney))
//		return false;
//
//	return true;
}

bool CPythonNetworkStream::SendSafeBoxCheckinPacket(::TItemPos InventoryPos, BYTE bySafeBoxPos)
{
	__PlayInventoryItemDropSound(InventoryPos);

	CGOutputPacket<CGSafeboxCheckinPacket> pack;
	*pack->mutable_inventory_pos() = InventoryPos;
	pack->set_safebox_pos(bySafeBoxPos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendSafeBoxCheckoutPacket(BYTE bySafeBoxPos, ::TItemPos InventoryPos)
{
	__PlaySafeBoxItemDropSound(bySafeBoxPos);

	CGOutputPacket<CGSafeboxCheckoutPacket> pack;
	pack->set_is_mall(false);
	pack->set_safebox_pos(bySafeBoxPos);
	*pack->mutable_inventory_pos() = InventoryPos;

	if (!Send(pack))
		return false;

	return true;
}

#ifdef INCREASE_ITEM_STACK
bool CPythonNetworkStream::SendSafeBoxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, WORD byCount)
#else
bool CPythonNetworkStream::SendSafeBoxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, BYTE byCount)
#endif
{
	__PlaySafeBoxItemDropSound(bySourcePos);

	CGOutputPacket<CGSafeboxItemMovePacket> pack;
	pack->set_source_pos(bySourcePos);
	pack->set_target_pos(byTargetPos);
	pack->set_count(byCount);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvSafeBoxWrongPasswordPacket()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnSafeBoxError", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvSafeBoxSizePacket(std::unique_ptr<GCSafeboxSizePacket> pack)
{
	CPythonSafeBox::Instance().OpenSafeBox(pack->size());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenSafeboxWindow", Py_BuildValue("(i)", pack->size()));

	return true;
}

// SafeBox
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Mall
bool CPythonNetworkStream::SendMallCheckoutPacket(BYTE byMallPos, ::TItemPos InventoryPos)
{
	__PlayMallItemDropSound(byMallPos);

	CGOutputPacket<CGSafeboxCheckoutPacket> pack;
	pack->set_is_mall(true);
	pack->set_safebox_pos(byMallPos);
	*pack->mutable_inventory_pos() = InventoryPos;

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvMallOpenPacket(std::unique_ptr<GCMallOpenPacket> pack)
{
	CPythonSafeBox::Instance().OpenMall(pack->size());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenMallWindow", Py_BuildValue("(i)", pack->size()));

	return true;
}
// Mall
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// GuildSafeBox
#ifdef ENABLE_GUILD_SAFEBOX
bool CPythonNetworkStream::SendGuildSafeboxOpenPacket()
{
	if (!Send(TCGHeader::GUILD_SAFEBOX_OPEN))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGuildSafeboxCheckinPacket(::TItemPos InventoryPos, BYTE byGuildSafeboxPos)
{
	__PlayInventoryItemDropSound(InventoryPos);

	CGOutputPacket<CGGuildSafeboxCheckinPacket> pack;
	*pack->mutable_item_pos() = InventoryPos;
	pack->set_safebox_pos(byGuildSafeboxPos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGuildSafeboxCheckoutPacket(BYTE byGuildSafeboxPos, ::TItemPos InventoryPos)
{
	__PlayGuildSafeboxItemDropSound(byGuildSafeboxPos);

	CGOutputPacket<CGGuildSafeboxCheckoutPacket> pack;
	pack->set_safebox_pos(byGuildSafeboxPos);
	*pack->mutable_item_pos() = InventoryPos;

	if (!Send(pack))
		return false;

	return true;
}

#ifdef INCREASE_ITEM_STACK
bool CPythonNetworkStream::SendGuildSafeboxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, WORD byCount)
#else
bool CPythonNetworkStream::SendGuildSafeboxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, BYTE byCount)
#endif
{
	__PlayGuildSafeboxItemDropSound(bySourcePos);

	CGOutputPacket<CGGuildSafeboxItemMovePacket> pack;
	pack->set_source_pos(bySourcePos);
	pack->set_target_pos(byTargetPos);
	pack->set_count(byCount);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGuildSafeboxGiveGoldPacket(ULONGLONG ullGold)
{
	CGOutputPacket<CGGuildSafeboxGiveGoldPacket> pack;
	pack->set_gold(ullGold);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGuildSafeboxTakeGoldPacket(ULONGLONG ullGold)
{
	CGOutputPacket<CGGuildSafeboxGetGoldPacket> pack;
	pack->set_gold(ullGold);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvGuildSafeboxPacket(const InputPacket& packet)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::GUILD_SAFEBOX_GOLD:
		{
			auto pack = packet.get<GCGuildSafeboxGoldPacket>();
			CPythonSafeBox::Instance().SetGuildMoney(pack->gold());

			__RefreshGuildSafeboxWindow();
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_OPEN:
		{
			auto pack = packet.get<GCGuildSafeboxOpenPacket>();
			CPythonSafeBox::Instance().OpenGuild(pack->size());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenGuildSafeboxWindow",
				Py_BuildValue("(i)", CPythonSafeBox::SAFEBOX_SLOT_Y_COUNT * pack->size()));
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_CLOSE:
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseGuildSafeboxWindow",
				Py_BuildValue("()"));
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_ENABLE:
		{
			// TraceError("Recieve the guild safebox enable header");
			CPythonSafeBox::Instance().SetGuildEnable(true);
			// PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSafeboxEnable", Py_BuildValue("()"));
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_DISABLE:
		{
			// TraceError("Recieve the guild safebox disable header");
			CPythonSafeBox::Instance().SetGuildEnable(false);
			// PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSafeboxEnable", Py_BuildValue("()"));
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_LOAD_LOG_START:
		{
			CPythonSafeBox::Instance().ClearGuildLogInfo();
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_LOAD_LOG:
		{
			auto pack = packet.get<GCGuildSafeboxLoadLogPacket>();
			for (auto& log : pack->logs())
				CPythonSafeBox::Instance().AddGuildLogInfo(log);
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_LOAD_LOG_DONE:
		{
			CPythonSafeBox::Instance().SortGuildLogInfo();
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSafeboxLog",
				Py_BuildValue("()"));
		}
		break;

		case TGCHeader::GUILD_SAFEBOX_APPEND_LOG:
		{
			auto pack = packet.get<GCGuildSafeboxAppendLogPacket>();

			CPythonSafeBox::Instance().AddGuildLogInfo(pack->log());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "AppendGuildSafeboxLog",
				Py_BuildValue("()"));
		}
		break;
	}

	return true;
}
#endif
// GuildSafeBox
//////////////////////////////////////////////////////////////////////////

#ifdef ENABLE_ALPHA_EQUIP
void __SetWeaponPower(IAbstractPlayer& rkPlayer, DWORD dwWeaponID, int iAlphaEquipVal)
#else
void __SetWeaponPower(IAbstractPlayer& rkPlayer, DWORD dwWeaponID)
#endif
{
	DWORD minPower = 0;
	DWORD maxPower = 0;
	DWORD minMagicPower = 0;
	DWORD maxMagicPower = 0;
	DWORD addPower = 0;

	CItemData* pkWeapon;
	if (CItemManager::Instance().GetItemDataPointer(dwWeaponID, &pkWeapon))
	{
		if (pkWeapon->GetType() == CItemData::ITEM_TYPE_WEAPON)
		{
			minPower = pkWeapon->GetValue(3);
			maxPower = pkWeapon->GetValue(4);
			minMagicPower = pkWeapon->GetValue(1);
			maxMagicPower = pkWeapon->GetValue(2);
			addPower = pkWeapon->GetValue(5);

#ifdef ENABLE_ALPHA_EQUIP
			int iDamVal = Item_ExtractAlphaEquipValue(iAlphaEquipVal);
			minPower += iDamVal;
			maxPower += iDamVal;
			minMagicPower += iDamVal;
			maxMagicPower += iDamVal;
#endif
		}
	}

	rkPlayer.SetWeaponPower(minPower, maxPower, minMagicPower, maxMagicPower, addPower);
}

// Item
// Recieve
bool CPythonNetworkStream::RecvItemSetPacket(std::unique_ptr<GCItemSetPacket> pack)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();

	auto& data = pack->data();
	::TItemPos cell = pack->data().cell();

	if (cell.window_type == SAFEBOX)
	{
		if (data.vnum() != 0)
			CPythonSafeBox::Instance().SetItemData(cell.cell, data);
		else
			CPythonSafeBox::Instance().DelItemData(cell.cell);

		__RefreshSafeboxWindow();
	}
	else if (cell.window_type == MALL)
	{
		if (data.vnum() != 0)
			CPythonSafeBox::Instance().SetMallItemData(cell.cell, data);
		else
			CPythonSafeBox::Instance().DelMallItemData(cell.cell);

		__RefreshMallWindow();
	}
#ifdef ENABLE_GUILD_SAFEBOX
	else if (cell.window_type == GUILD_SAFEBOX)
	{
		if (data.vnum() != 0)
			CPythonSafeBox::Instance().SetGuildItemData(cell.cell, data);
		else
			CPythonSafeBox::Instance().DelGuildItemData(cell.cell);

		__RefreshGuildSafeboxWindow();
	}
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	else if (cell.window_type == ACCEREFINE)
	{
		if (data.vnum() != 0)
			CPythonPlayer::Instance().SetAcceItemData(cell.cell, data);
		else
			CPythonPlayer::Instance().DelAcceItemData(cell.cell);

		__RefreshAcceWindow();
		__RefreshInventoryWindow();
	}
#endif
	else
	{
		rkPlayer.SetItemData(cell, data);

#ifdef ENABLE_MARK_NEW_ITEM_SYSTEM
		if (pack->highlight())
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Highlight_Item",
				Py_BuildValue("(ii)", cell.window_type, cell.cell));
#endif

		__RefreshInventoryWindow();

		if (cell.IsEquipCell() && cell.cell == c_Equipment_Weapon)
		{
#ifdef ENABLE_ALPHA_EQUIP
			__SetWeaponPower(rkPlayer, data.vnum(), data.alpha_equip());
#else
			__SetWeaponPower(rkPlayer, data.vnum());
#endif
		}
	}

	return true;
}

bool CPythonNetworkStream::RecvItemUpdatePacket(std::unique_ptr<GCItemUpdatePacket> pack)
{
	auto& rkPlayer=CPythonPlayer::GetSingleton();
	rkPlayer.SetItemData(pack->data().cell(), pack->data());

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshInventorySlot", Py_BuildValue("(ii)", pack->data().cell().window_type(), pack->data().cell().cell()));
	__RefreshInventoryWindow();
	return true;
}

bool CPythonNetworkStream::RecvItemGroundAddPacket(std::unique_ptr<GCItemGroundAddPacket> pack)
{
	long lX = pack->x(), lY = pack->y();
	__GlobalPositionToLocalPosition(lX, lY);
	
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	long alSockets[ITEM_SOCKET_SLOT_MAX_NUM];
	
	for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
	{
		alSockets[i] = pack->sockets(i);
	}

	TItemAttribute attrs[ITEM_ATTRIBUTE_SLOT_MAX_NUM];
	for (int i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
	{
		attrs[i] = pack->attributes(i);
	}

	CPythonItem::Instance().CreateItem(pack->vid(), pack->vnum(), lX, lY, pack->z(), true, alSockets, attrs);
#else
	CPythonItem::Instance().CreateItem(pack->vid(), pack->vnum(), lX, lY, pack->z());
#endif
	return true;
}


bool CPythonNetworkStream::RecvItemOwnership(std::unique_ptr<GCItemOwnershipPacket> pack)
{
	CPythonItem::Instance().SetOwnership(pack->vid(), pack->name().c_str());
	return true;
}

bool CPythonNetworkStream::RecvItemGroundDelPacket(std::unique_ptr<GCItemGroundDeletePacket> pack)
{
	CPythonItem::Instance().DeleteItem(pack->vid());
	return true;
}

bool CPythonNetworkStream::RecvQuickSlotAddPacket(std::unique_ptr<GCQuickslotAddPacket> pack)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	rkPlayer.AddQuickSlot(pack->pos(), pack->slot().type(), pack->slot().pos());

	__RefreshInventoryWindow();

	return true;
}

bool CPythonNetworkStream::RecvQuickSlotDelPacket(std::unique_ptr<GCQuickslotDelPacket> pack)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	rkPlayer.DeleteQuickSlot(pack->pos());

	__RefreshInventoryWindow();

	return true;
}

bool CPythonNetworkStream::RecvQuickSlotMovePacket(std::unique_ptr<GCQuickslotSwapPacket> pack)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	rkPlayer.MoveQuickSlot(pack->pos(), pack->change_pos());

	__RefreshInventoryWindow();

	return true;
}



bool CPythonNetworkStream::SendShopEndPacket()
{
	if (!__CanActMainInstance())
		return true;

	if (!Send(TCGHeader::SHOP_END))
	{
		Tracef("SendShopEndPacket Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendShopBuyPacket(BYTE bPos)
{
	if (!__CanActMainInstance())
		return true;
	
	CGOutputPacket<CGShopBuyPacket> pack;
	pack->set_pos(bPos);

	if (!Send(pack))
	{
		Tracef("SendShopBuyPacket Error\n");
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendShopSellPacket(WORD wSlot)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGShopSellPacket> pack;
	*pack->mutable_cell() = ::TItemPos(INVENTORY, wSlot);
	pack->set_count(1);

	if (!Send(pack))
		return false;

	return true;
}

#ifdef INCREASE_ITEM_STACK
bool CPythonNetworkStream::SendShopSellPacketNew(WORD wSlot, WORD byCount)
#else
bool CPythonNetworkStream::SendShopSellPacketNew(WORD wSlot, BYTE byCount)
#endif
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGShopSellPacket> pack;
	*pack->mutable_cell() = ::TItemPos(INVENTORY, wSlot);
	pack->set_count(byCount);

	if (!Send(pack))
		return false;

	Tracef(" SendShopSellPacketNew(wSlot=%d, byCount=%d)\n", wSlot, byCount);

	return true;
}

// Send
bool CPythonNetworkStream::SendItemUsePacket(::TItemPos pos)
{
#ifdef ENABLE_ZODIAC
	bool skipIsDeadItem = false;
	IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();
	DWORD dwItemID = rkPlayer.GetItemIndex(pos);
	if (dwItemID == 33025)
		skipIsDeadItem = true;


	if (!__CanActMainInstance(skipIsDeadItem))
#else
	if (!__CanActMainInstance())
#endif
		return true;

	if (__IsEquipItemInSlot(pos))
	{
		if (CPythonExchange::Instance().isTrading())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_EXCHANGE"));
			return true;
		}

		if (CPythonShop::Instance().IsOpen())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_SHOP"));
			return true;
		}

		if (__IsPlayerAttacking())
			return true;
	}

	__PlayInventoryItemUseSound(pos);

	CGOutputPacket<CGItemUsePacket> pack;
	*pack->mutable_cell() = pos;
	pack->set_count(1);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemUsePacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendItemUseToItemPacket(::TItemPos source_pos, ::TItemPos target_pos)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGItemUseToItemPacket> pack;
	*pack->mutable_cell() = source_pos;
	*pack->mutable_target_cell() = target_pos;

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemUseToItemPacket Error");
#endif
		return false;
	}

#ifdef _DEBUG
	Tracef(" << SendItemUseToItemPacket(src=%d, dst=%d)\n", source_pos, target_pos);
#endif

	return true;
}

bool CPythonNetworkStream::SendItemDropPacket(::TItemPos pos, DWORD elk)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGItemDropPacket> pack;
	*pack->mutable_cell() = pos;
	pack->set_gold(elk);
	pack->set_count(1);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemDropPacket Error");
#endif
		return false;
	}

	return true;
}

#ifdef INGAME_WIKI
bool CPythonNetworkStream::SendWikiRequestInfo(unsigned long long retID, DWORD vnum, bool isMob)
{
	CGOutputPacket<CGRecvWikiPacket> pack;
	pack->set_ret_id(retID);
	pack->set_vnum(vnum);
	pack->set_is_mob(isMob);

	if (!Send(pack))
		return false;

	return true;
}
#endif

bool CPythonNetworkStream::SendItemDropPacketNew(::TItemPos pos, DWORD elk, DWORD count)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGItemDropPacket> pack;
	*pack->mutable_cell() = pos;
	pack->set_gold(elk);
	pack->set_count(count);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemDropPacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::__IsEquipItemInSlot(::TItemPos uSlotPos)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	return rkPlayer.IsEquipItemInSlot(uSlotPos);
}

void CPythonNetworkStream::__PlayInventoryItemUseSound(::TItemPos uSlotPos)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	DWORD dwItemID=rkPlayer.GetItemIndex(uSlotPos);

	CPythonItem& rkItem=CPythonItem::Instance();
	rkItem.PlayUseSound(dwItemID);
}

void CPythonNetworkStream::__PlayInventoryItemDropSound(::TItemPos uSlotPos)
{
	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	DWORD dwItemID=rkPlayer.GetItemIndex(uSlotPos);

	CPythonItem& rkItem=CPythonItem::Instance();
	rkItem.PlayDropSound(dwItemID);
}

//void CPythonNetworkStream::__PlayShopItemDropSound(UINT uSlotPos)
//{
//	DWORD dwItemID;
//	CPythonShop& rkShop=CPythonShop::Instance();
//	if (!rkShop.GetSlotItemID(uSlotPos, &dwItemID))
//		return;
//	
//	CPythonItem& rkItem=CPythonItem::Instance();
//	rkItem.PlayDropSound(dwItemID);
//}

void CPythonNetworkStream::__PlaySafeBoxItemDropSound(UINT uSlotPos)
{
	DWORD dwItemID;
	CPythonSafeBox& rkSafeBox=CPythonSafeBox::Instance();
	if (!rkSafeBox.GetSlotItemID(uSlotPos, &dwItemID))
		return;

	CPythonItem& rkItem=CPythonItem::Instance();
	rkItem.PlayDropSound(dwItemID);
}

void CPythonNetworkStream::__PlayMallItemDropSound(UINT uSlotPos)
{
	DWORD dwItemID;
	CPythonSafeBox& rkSafeBox=CPythonSafeBox::Instance();
	if (!rkSafeBox.GetSlotMallItemID(uSlotPos, &dwItemID))
		return;

	CPythonItem& rkItem=CPythonItem::Instance();
	rkItem.PlayDropSound(dwItemID);
}

#ifdef ENABLE_GUILD_SAFEBOX
void CPythonNetworkStream::__PlayGuildSafeboxItemDropSound(UINT uSlotPos)
{
	DWORD dwItemID;
	CPythonSafeBox& rkSafeBox = CPythonSafeBox::Instance();
	if (!rkSafeBox.GetSlotGuildItemID(uSlotPos, &dwItemID))
		return;

	CPythonItem& rkItem = CPythonItem::Instance();
	rkItem.PlayDropSound(dwItemID);
}
#endif

#ifdef INCREASE_ITEM_STACK
bool CPythonNetworkStream::SendItemMovePacket(::TItemPos pos, ::TItemPos change_pos, WORD num)
#else
bool CPythonNetworkStream::SendItemMovePacket(::TItemPos pos, ::TItemPos change_pos, BYTE num)
#endif
{	
	if (!__CanActMainInstance())
		return true;
	
	if (__IsEquipItemInSlot(pos))
	{
		if (CPythonExchange::Instance().isTrading())
		{
			if (pos.IsEquipCell() || change_pos.IsEquipCell())
			{
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_EXCHANGE"));
				return true;
			}
		}

		if (CPythonShop::Instance().IsOpen())
		{
			if (pos.IsEquipCell() || change_pos.IsEquipCell())
			{
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_SHOP"));
				return true;
			}
		}

		if (__IsPlayerAttacking())
			return true;
	}

	__PlayInventoryItemDropSound(pos);

	CGOutputPacket<CGItemMovePacket> pack;
	*pack->mutable_cell() = pos;
	*pack->mutable_cell_to() = change_pos;
	pack->set_count(num);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemMovePacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendItemPickUpPacket(DWORD vid)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGItemPickupPacket> pack;
	pack->set_vid(vid);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemPickUpPacket Error");
#endif
		return false;
	}

	return true;
}


bool CPythonNetworkStream::SendQuickSlotAddPacket(BYTE wpos, BYTE type, BYTE pos)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGQuickslotAddPacket> pack;
	pack->set_pos(wpos);
	pack->mutable_slot()->set_type(type);
	pack->mutable_slot()->set_pos(pos);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendQuickSlotAddPacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendQuickSlotDelPacket(BYTE pos)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGQuickslotDeletePacket> pack;
	pack->set_pos(pos);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendQuickSlotDelPacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendQuickSlotMovePacket(BYTE pos, BYTE change_pos)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGQuickslotSwapPacket> pack;
	pack->set_pos(pos);
	pack->set_change_pos(change_pos);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendQuickSlotSwapPacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvSpecialEffect(std::unique_ptr<GCSpecialEffectPacket> pack)
{
	DWORD effect = -1;
	bool bPlayPotionSound = false;	//포션을 먹을 경우는 포션 사운드를 출력하자.!!
	bool bAttachEffect = true;		//캐리터에 붙는 어태치 이펙트와 일반 이펙트 구분.!!
	switch (pack->type())
	{
		case SE_HPUP_RED:
			effect = CInstanceBase::EFFECT_HPUP_RED;
			bPlayPotionSound = true;
			break;
		case SE_SPUP_BLUE:
			effect = CInstanceBase::EFFECT_SPUP_BLUE;
			bPlayPotionSound = true;
			break;
		case SE_SPEEDUP_GREEN:
			effect = CInstanceBase::EFFECT_SPEEDUP_GREEN;
			bPlayPotionSound = true;
			break;
		case SE_DXUP_PURPLE:
			effect = CInstanceBase::EFFECT_DXUP_PURPLE;
			bPlayPotionSound = true;
			break;
		case SE_CRITICAL:
			effect = CInstanceBase::EFFECT_CRITICAL;
			break;
		case SE_PENETRATE:
			effect = CInstanceBase::EFFECT_PENETRATE;
			break;
		case SE_BLOCK:
			effect = CInstanceBase::EFFECT_BLOCK;
			break;
		case SE_DODGE:
			effect = CInstanceBase::EFFECT_DODGE;
			break;
		case SE_CHINA_FIREWORK:
			effect = CInstanceBase::EFFECT_FIRECRACKER;
			bAttachEffect = false;
			break;
		case SE_SPIN_TOP:
			effect = CInstanceBase::EFFECT_SPIN_TOP;
			bAttachEffect = false;
			break;
		case SE_SUCCESS :
			effect = CInstanceBase::EFFECT_SUCCESS ;
			bAttachEffect = false ;
			break ;
		case SE_FAIL :
			effect = CInstanceBase::EFFECT_FAIL ;
			break ;
		case SE_FR_SUCCESS:
			effect = CInstanceBase::EFFECT_FR_SUCCESS;
			bAttachEffect = false ;
			break;
		case SE_LEVELUP_ON_14_FOR_GERMANY:	//레벨업 14일때 ( 독일전용 )
			effect = CInstanceBase::EFFECT_LEVELUP_ON_14_FOR_GERMANY;
			bAttachEffect = false ;
			break;
		case SE_LEVELUP_UNDER_15_FOR_GERMANY: //레벨업 15일때 ( 독일전용 )
			effect = CInstanceBase::EFFECT_LEVELUP_UNDER_15_FOR_GERMANY;
			bAttachEffect = false ;
			break;
		case SE_PERCENT_DAMAGE1:
			effect = CInstanceBase::EFFECT_PERCENT_DAMAGE1;
			break;
		case SE_PERCENT_DAMAGE2:
			effect = CInstanceBase::EFFECT_PERCENT_DAMAGE2;
			break;
		case SE_PERCENT_DAMAGE3:
			effect = CInstanceBase::EFFECT_PERCENT_DAMAGE3;
			break;
		case SE_AUTO_HPUP:
			effect = CInstanceBase::EFFECT_AUTO_HPUP;
			break;
		case SE_AUTO_SPUP:
			effect = CInstanceBase::EFFECT_AUTO_SPUP;
			break;
		case SE_EQUIP_RAMADAN_RING:
			effect = CInstanceBase::EFFECT_RAMADAN_RING_EQUIP;
			break;
		case SE_EQUIP_HALLOWEEN_CANDY:
			effect = CInstanceBase::EFFECT_HALLOWEEN_CANDY_EQUIP;
			break;
		case SE_EQUIP_HAPPINESS_RING:
 			effect = CInstanceBase::EFFECT_HAPPINESS_RING_EQUIP;
			break;
		case SE_EQUIP_LOVE_PENDANT:
			effect = CInstanceBase::EFFECT_LOVE_PENDANT_EQUIP;
			break;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		case SE_ACCE_SUCESS_ABSORB:
			effect = CInstanceBase::EFFECT_ACCE_SUCESS_ABSORB;
			break;
		case SE_ACCE_EQUIP:
			effect = CInstanceBase::EFFECT_ACCE_EQUIP;
			break;
		case SE_ACCE_BACK:
			effect = CInstanceBase::EFFECT_ACCE_BACK;
			break;
#endif
#ifdef ENABLE_AGGREGATE_MONSTER_EFFECT
		case SE_AGGREGATE_MONSTER_EFFECT:
			effect = CInstanceBase::EFFECT_AGGREGATE_MONSTER;
			break;
#endif
		case SE_EFFECT_HEALER:
			effect = CInstanceBase::EFFECT_HEALER;
			break;
#ifdef COMBAT_ZONE
		case SE_COMBAT_ZONE_POTION:
			effect = CInstanceBase::EFFECT_COMBAT_ZONE_POTION;
			break;
#endif
		default:
			TraceError("%d 는 없는 스페셜 이펙트 번호입니다.TPacketGCSpecialEffect",pack->type());
			break;
	}

	if (bPlayPotionSound)
	{		
		IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
		if(rkPlayer.IsMainCharacterIndex(pack->vid()))
		{
			CPythonItem& rkItem=CPythonItem::Instance();
			rkItem.PlayUsePotionSound();
		}
	}

	if (-1 != effect)
	{
		CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
		if (pInstance)
		{
			if(bAttachEffect)
				pInstance->AttachSpecialEffect(effect);
			else
				pInstance->CreateSpecialEffect(effect);
		}
	}

	return true;
}

bool CPythonNetworkStream::RecvSpecificEffect(std::unique_ptr<GCSpecificEffectPacket> pack)
{
	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	//EFFECT_TEMP
	if (pInstance)
	{
		CInstanceBase::RegisterEffect(CInstanceBase::EFFECT_TEMP, "", pack->effect_file().c_str(), false);
		pInstance->AttachSpecialEffect(CInstanceBase::EFFECT_TEMP);
	}

	return true;
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
bool CPythonNetworkStream::SendAcceRefineCheckinPacket(::TItemPos InventoryPos, BYTE byAccePos)
{
	__PlayInventoryItemDropSound(InventoryPos);

	CGOutputPacket<CGAcceRefineCheckinPacket> pack;
	*pack->mutable_item_cell() = InventoryPos;
	pack->set_acce_pos(byAccePos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAcceRefineCheckoutPacket(BYTE byAccePos)
{
	if (byAccePos > 0)
		__PlaySafeBoxItemDropSound(byAccePos);

	CGOutputPacket<CGAcceRefineCheckoutPacket> pack;
	pack->set_acce_pos(byAccePos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAcceRefineAcceptPacket(BYTE windowType)
{
	CGOutputPacket<CGAcceRefineAcceptPacket> pack;
	pack->set_window_type(windowType);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAcceRefineCancelPacket()
{
	if (!Send(TCGHeader::ACCE_REFINE_CANCEL))
		return false;

	return true;
}
#endif

bool CPythonNetworkStream::SendItemDestroyPacket(::TItemPos pos, DWORD count)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGItemDestroyPacket> pack;
	*pack->mutable_cell() = pos;
	pack->set_num(count);

	if (!Send(pack))
		return false;

	return true;
}

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
bool CPythonNetworkStream::RecvAttributesToClient(std::unique_ptr<GCAttributesToClientPacket> pack)
{
	CPythonItem::Instance().ClearAttributeData(pack->item_type(), pack->item_sub_type());

	for (auto& attr : pack->attrs())
		CPythonItem::Instance().AddAttributeData(pack->item_type(), pack->item_sub_type(), attr.type(), attr.value());

	return true;
}
#endif

#ifdef INCREASE_ITEM_STACK
bool CPythonNetworkStream::SendItemMultiUsePacket(::TItemPos pos, WORD bCount)
#else
bool CPythonNetworkStream::SendItemMultiUsePacket(::TItemPos pos, BYTE bCount)
#endif
{
	if (!__CanActMainInstance())
		return true;

	if (__IsEquipItemInSlot(pos))
	{
		if (CPythonExchange::Instance().isTrading())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_EXCHANGE"));
			return true;
		}

		if (CPythonShop::Instance().IsOpen())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AppendNotifyMessage", Py_BuildValue("(s)", "CANNOT_EQUIP_SHOP"));
			return true;
		}

		if (__IsPlayerAttacking())
			return true;
	}

	__PlayInventoryItemUseSound(pos);

	CGOutputPacket<CGItemMultiUsePacket> pack;
	*pack->mutable_cell() = pos;
	pack->set_count(bCount);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendItemMultiUsePacket Error");
#endif
		return false;
	}

	return true;
}

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
bool CPythonNetworkStream::RecvCostumeBonusTransferPacket(const InputPacket& packet)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::CBT_OPEN:
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CostumeBonusTransferWindowOpen", Py_BuildValue("()"));
			break;
		}
		case TGCHeader::CBT_CLOSE:
		{
			CPythonPlayer::instance().SetCostumeBonusTransferWindowClose();
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CostumeBonusTransferWindowClose", Py_BuildValue("()"));
			break;
		}
		case TGCHeader::CBT_ITEM_SET:
		{
			auto pack = packet.get<GCCBTItemSetPacket>();

			if (!pack->has_data() && pack->data().vnum() != 0)
				CPythonPlayer::instance().DelCostumeBonusTransferItemData(pack->cbt_pos());
			else
			{
				CPythonPlayer::instance().SetCostumeBonusTransferItemData(pack->cbt_pos(), pack->data());

				if (pack->cbt_pos() == CBT_SLOT_MATERIAL)
				{
					DWORD TargetVnum;
					if (CPythonPlayer::instance().GetCostumeBonusTransferSlotItemID(CBT_SLOT_TARGET, &TargetVnum))
					{
						TItemData* kTargetItemData;
						TItemData pkTargetItemData;
						if (CPythonPlayer::instance().GetCostumeBonusTransferItemDataPtr(CBT_SLOT_TARGET, &kTargetItemData))
						{
							pkTargetItemData = *kTargetItemData;
							*pkTargetItemData.mutable_attributes() = pack->data().attributes();
							CPythonPlayer::instance().SetCostumeBonusTransferItemData(CBT_SLOT_RESULT, pkTargetItemData);
						}
					}
				}
				else if (pack->cbt_pos() == CBT_SLOT_TARGET)
				{
					DWORD MaterialVnum;
					if (CPythonPlayer::instance().GetCostumeBonusTransferSlotItemID(CBT_SLOT_MATERIAL, &MaterialVnum))
					{
						TItemData* kTargetItemData;
						if (CPythonPlayer::instance().GetCostumeBonusTransferItemDataPtr(CBT_SLOT_MATERIAL, &kTargetItemData))
						{
							for (int iAttr = 0; iAttr < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++iAttr)
								*pack->mutable_data()->mutable_attributes(iAttr) = kTargetItemData->attributes(iAttr);
							CPythonPlayer::instance().SetCostumeBonusTransferItemData(CBT_SLOT_RESULT, pack->data());
						}
					}
				}
			}
			break;
		}
		case TGCHeader::CBT_CLEAR:
		{
			CPythonPlayer::instance().DelCostumeBonusTransferItemData(CBT_SLOT_MEDIUM);
			CPythonPlayer::instance().DelCostumeBonusTransferItemData(CBT_SLOT_MATERIAL);
			CPythonPlayer::instance().DelCostumeBonusTransferItemData(CBT_SLOT_TARGET);
			break;
		}
	}

	__RefreshInventoryWindow();

	return true;
}

bool CPythonNetworkStream::SendCostumeBonusTransferCheckIn(::TItemPos InventoryCell, BYTE byCBTPos)
{
	__PlayInventoryItemDropSound(InventoryCell);

	CGOutputPacket<CGCostumeBonusTransferPacket> pack;
	pack->set_sub_header(CBT_SUBHEADER_CG_CHECKIN);
	*pack->mutable_item_cell() = InventoryCell;
	pack->set_pos(byCBTPos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendCostumeBonusTransferCheckOut(BYTE byCBTPos)
{
	CGOutputPacket<CGCostumeBonusTransferPacket> pack;
	pack->set_sub_header(CBT_SUBHEADER_CG_CHECKOUT);
	pack->set_pos(byCBTPos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendCostumeBonusTransferAccept()
{
	CGOutputPacket<CGCostumeBonusTransferPacket> pack;
	pack->set_sub_header(CBT_SUBHEADER_CG_ACCEPT);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendCostumeBonusTransferCancel()
{
	CGOutputPacket<CGCostumeBonusTransferPacket> pack;
	pack->set_sub_header(CBT_SUBHEADER_CG_CANCEL);

	if (!Send(pack))
		return false;

	return true;
}
#endif
