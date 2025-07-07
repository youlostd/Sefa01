#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "shop.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "mob_manager.h"
#include "gm.h"
#include "p2p.h"
#include "desc_client.h"

CShop::CShop()
	: m_dwVnum(0), m_dwNPCVnum(0), m_pkPC(NULL)
{
}

CShop::~CShop()
{
	Close();
}

void CShop::Close(DWORD dwExceptionPID)
{
	GuestMapType::iterator it;

	it = m_map_guest.begin();

	LPCHARACTER pkExceptionChr = NULL;

	while (it != m_map_guest.end())
	{
		LPCHARACTER ch = it->first;
		if (ch->GetPlayerID() != dwExceptionPID)
		{
			if (ch->GetDesc())
				ch->GetDesc()->Packet(network::TGCHeader::SHOP_END);
			ch->SetShop(NULL);
		}
		else
			pkExceptionChr = ch;
		++it;
	}

	m_map_guest.clear();

	if (pkExceptionChr)
		m_map_guest[pkExceptionChr] = false;
}

void CShop::SetPCShop(LPCHARACTER ch)
{
	m_pkPC = ch;
}

bool CShop::Create(DWORD dwVnum, DWORD dwNPCVnum, const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& items)
{
	/*
	   if (NULL == CMobManager::instance().Get(dwNPCVnum))
	   {
	   sys_err("No such a npc by vnum %d", dwNPCVnum);
	   return false;
	   }
	 */
	sys_log(!test_server, "SHOP #%d (Shopkeeper %d)", dwVnum, dwNPCVnum);

	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;

	BYTE bItemCount;

	for (bItemCount = 0; bItemCount < SHOP_HOST_ALL_ITEM_MAX_NUM && bItemCount < items.size(); ++bItemCount)
		if (0 == items[bItemCount].item().vnum())
			break;

	SetShopItems(items, bItemCount);
	return true;
}

void CShop::SetShopItems(const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& items, BYTE bItemCount)
{
	if (bItemCount > SHOP_HOST_ALL_ITEM_MAX_NUM)
		return;
	
	CGrid kGrid(5, SHOP_HOST_ITEM_MAX_NUM / 5);
	kGrid.Clear();

	int iPage = 0;

	m_itemVector.resize(SHOP_HOST_ALL_ITEM_MAX_NUM);
	memset(&m_itemVector[0], 0, sizeof(SHOP_ITEM) * m_itemVector.size());

	for (int i = 0; i < bItemCount && i < items.size(); ++i)
	{
		LPITEM pkItem = NULL;
		const network::TItemTable * item_table;
		auto& shop_item = items[i].item();

		if (m_pkPC)
		{
			pkItem = m_pkPC->GetItem(shop_item.cell());

			if (!pkItem)
			{
				sys_err("cannot find item on pos (window %d cell %d) (name: %s)", shop_item.cell().window_type(), shop_item.cell().cell(), m_pkPC->GetName());
				continue;
			}

			item_table = pkItem->GetProto();
		}
		else
		{
			if (!shop_item.vnum())
				continue;

			item_table = ITEM_MANAGER::instance().GetTable(shop_item.vnum());
		}

		if (!item_table)
		{
			sys_err("Shop: no item table by item vnum #%d", shop_item.vnum());
			continue;
		}

		int iPos;

		if (IsPCShop())
		{
			sys_log(0, "MyShop: use position %d", items[i].display_pos());
			iPos = items[i].display_pos();
		}
		else
			iPos = kGrid.FindBlank(1, item_table->size());

		if (iPos < 0)
		{
			if (++iPage >= SHOP_TAB_COUNT_MAX)
			{
				sys_err("not enough shop window");
				continue;
			}

			kGrid.Clear();
			iPos = 0;
		}

		if (!kGrid.IsEmpty(iPos, 1, item_table->size()))
		{
			if (IsPCShop())
			{
				sys_err("not empty position for pc shop %s[%d]", m_pkPC->GetName(), m_pkPC->GetPlayerID());
			}
			else
			{
				sys_err("not empty position for npc shop");
			}
			continue;
		}

		kGrid.Put(iPos, 1, item_table->size());

		SHOP_ITEM & item = m_itemVector[iPage * SHOP_HOST_ITEM_MAX_NUM + iPos];

		item.pkItem = pkItem;
		item.itemid = 0;

		if (item.pkItem)
		{
			item.vnum = pkItem->GetVnum();
			item.count = pkItem->GetCount(); // PC 샵의 경우 아이템 개수는 진짜 아이템의 개수여야 한다.
			item.price = shop_item.price(); // 가격도 사용자가 정한대로..
			item.itemid	= pkItem->GetID();
		}
		else
		{
			item.vnum = shop_item.vnum();
			item.count = shop_item.count();

			if (items[i].price_item_vnum())
			{
				item.price = shop_item.price();
				item.price_vnum = items[i].price_item_vnum();
#ifdef SECOND_ITEM_PRICE
				if (items[i].price_item_vnum2())
				{
					item.price2 = items[i].price2();
					item.price_vnum2 = items[i].price_item_vnum2();
				}
#endif
			}
			else
			{
				if (IS_SET(item_table->flags(), ITEM_FLAG_COUNT_PER_1GOLD))
				{
					if (item_table->gold() == 0)
						item.price = item.count;
					else
						item.price = item.count / item_table->gold();
				}
				else
					item.price = item_table->gold() * item.count;
			}
		}

		char name[36];
		snprintf(name, sizeof(name), "%-20s(#%-5d) (x %d)", item_table->locale_name(LANGUAGE_DEFAULT).c_str(), (int) item.vnum, item.count);

		sys_log(!test_server, "SHOP_ITEM: %-36s PRICE %-5lld (PRICE_VNUM %-5d)", name, item.price, item.price_vnum);
	}
}

network::TGCHeader CShop::Buy(LPCHARACTER ch, BYTE pos)
{
	if (pos >= m_itemVector.size())
	{
		sys_log(0, "Shop::Buy : invalid position %d : %s", pos, ch->GetName());
		return network::TGCHeader::SHOP_ERROR_INVALID_POS;
	}

	bool bIsPCShop = IsPCShop();
	if (bIsPCShop && !GM::check_allow(ch->GetGMLevel(), GM_ALLOW_BUY_PRIVATE_ITEM))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot do this with this gamemaster rank."));
		sys_log(0, "Shop::Buy : cannot buy as gamemaster player %u %s", ch->GetPlayerID(), ch->GetName());
		return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY;
	}

	sys_log(0, "Shop::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return network::TGCHeader::SHOP_END;

	SHOP_ITEM& r_item = m_itemVector[pos];

	if (test_server)
		sys_log(0, "Shop::Buy : itemID %u pkItem %p", r_item.itemid, r_item.pkItem);

	if (r_item.price <= 0)
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY;
	}

	LPITEM pkSelectedItem = ITEM_MANAGER::instance().Find(r_item.itemid);

	if (bIsPCShop)
	{
		if (!pkSelectedItem)
		{
			sys_log(0, "Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item : BuyerPID:%d SellerPID:%d",
				ch->GetPlayerID(),
				m_pkPC ? m_pkPC->GetPlayerID() : 0);

			return network::TGCHeader::SHOP_ERROR_SOLDOUT;
		}

		if ((pkSelectedItem->GetOwner() != m_pkPC))
		{
			sys_log(0, "Shop::Buy : Critical: This user seems to be a hacker : invalid pcshop item : BuyerPID:%d SellerPID:%d",
				ch->GetPlayerID(),
				m_pkPC ? m_pkPC->GetPlayerID() : 0);

			return network::TGCHeader::SHOP_ERROR_SOLDOUT;
		}
	}

	DWORD dwPriceVnum = r_item.price_vnum;
	DWORD dwPrice = r_item.price;

	if (dwPriceVnum != 0)
	{
		if (ch->CountSpecifyItem(dwPriceVnum) < (int)dwPrice)
		{
			if (test_server)
				sys_log(0, "Shop::Buy : Not enough item : %s has %d, price %d priceVnum %d", ch->GetName(), ch->CountSpecifyItem(dwPriceVnum), dwPrice, dwPriceVnum);
			return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY_EX;
		}

#ifdef SECOND_ITEM_PRICE
		if (r_item.price_vnum2 && ch->CountSpecifyItem(r_item.price_vnum2) < (int)r_item.price2)
		{
			if (test_server)
				sys_log(0, "Shop::Buy : Not enough item : %s has %d, price %d priceVnum %d", ch->GetName(), ch->CountSpecifyItem(r_item.price_vnum2), r_item.price2, r_item.price_vnum2);
			return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY_EX;
		}
#endif
	}
	else if (ch->GetGold() < (int)dwPrice)
	{
		sys_log(1, "Shop::Buy : Not enough money : %s has %d, price %d", ch->GetName(), ch->GetGold(), dwPrice);
		return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY;
	}

	LPITEM item;

	if (bIsPCShop)
		item = r_item.pkItem;
	else
		item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);
	if (!item)
		return network::TGCHeader::SHOP_ERROR_SOLDOUT;
#ifdef ENABLE_ZODIAC_TEMPLE
	int Badges[] = { 33001, 33002, 33003, 33004, 33005, 33006, 33007, 33008, 33009, 33010, 33011, 33012, 33013, 33014, 33015, 33016, 33017, 33018, 33019, 33020, 33021, 33022, 33025 };
	for (int i = 0; i < 23; ++i)
	{
		if (ch->GetZodiacBadges(i + 1) == 1 && item->GetVnum() == Badges[i])
			return network::TGCHeader::SHOP_ERROR_ZODIAC_SHOP;
	}
	for (int i = 0; i < 23; i++)
	{
		if (item->GetVnum() == Badges[i])
			ch->SetZodiacBadges(1, i + 1);
	}
#endif

	BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);

	// if (bWindow == DRAGON_SOUL_INVENTORY && quest::CQuestManager::instance().GetEventFlag("disable_ds_shopbuy"))
	// return SHOP_SUBHEADER_GC_SOLD_OUT;

	// Avoid buy & no inventory -> drop to ground -> can't pickup
	if (bWindow == DRAGON_SOUL_INVENTORY && !ch->DragonSoul_IsQualified())
		return network::TGCHeader::SHOP_ERROR_INVENTORY_FULL;

	int iEmptyPos;

	// iHotfixEmptyPos = ch->GetEmptyInventory(item->GetSize());

	LPITEM pkStackItem;
	if (!ch->CanStackItem(item, &pkStackItem))
	{
#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
			iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
		else
#endif
			iEmptyPos = ch->GetEmptyInventoryNew(bWindow, item->GetSize());

		if (iEmptyPos < 0)
		{
			if (bIsPCShop)
			{
				sys_log(1, "Shop::Buy at PC Shop : Inventory full : %s size %d", ch->GetName(), item->GetSize());
				return network::TGCHeader::SHOP_ERROR_INVENTORY_FULL;
			}
			else
			{
				sys_log(1, "Shop::Buy : Inventory full : %s size %d", ch->GetName(), item->GetSize());
				M2_DESTROY_ITEM(item);
				return network::TGCHeader::SHOP_ERROR_INVENTORY_FULL;
			}
		}
	}

	bool bIsGMItem = false;

	if (dwPriceVnum != 0)
#ifdef SECOND_ITEM_PRICE
	{
		bIsGMItem = ch->RemoveSpecifyItem(dwPriceVnum, dwPrice);

		if (ch->RemoveSpecifyItem(r_item.price_vnum2, r_item.price2))
			bIsGMItem = true;
	}
#else
		bIsGMItem = ch->RemoveSpecifyItem(dwPriceVnum, dwPrice);
#endif
	else
		ch->PointChange(POINT_GOLD, -(int)dwPrice, false);
	
	if (bIsGMItem)
		item->SetGMOwner(true);
	
	DWORD dwTax = 3;
	DWORD dwRewardGold = (DWORD)(((ULONGLONG)dwPrice) * (100ULL - (ULONGLONG)dwTax) / 100ULL);

	WORD wItemCell = item->GetCell();
	if (item->GetOwner())
		item->RemoveFromCharacter();
	
	ITEM_MANAGER::instance().FlushDelayedSave(item);
	if (bIsPCShop)
	{
		if (m_pkPC)
			m_pkPC->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wItemCell, 255);
			
		char buf[1024];
		snprintf(buf, sizeof(buf), "%s %u(%s) %u %u [%s]", item->GetName(), m_pkPC->GetPlayerID(), m_pkPC->GetName(), dwPrice, item->GetCount(), g_stHostname.c_str());
		LogManager::instance().ItemLog(ch, item, "SHOP_BUY", buf);

		snprintf(buf, sizeof(buf), "%s %u(%s) %u->%u %u", item->GetName(), ch->GetPlayerID(), ch->GetName(), dwPrice, m_pkPC->GetGold() + dwPrice, item->GetCount());
		LogManager::instance().ItemLog(m_pkPC, item, "SHOP_SELL", buf);

		r_item.pkItem = NULL;
		BroadcastUpdateItem(pos);

		m_pkPC->PointChange(POINT_GOLD, dwPrice, false);
		if (dwTax > 0 && m_pkPC)
			m_pkPC->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkPC, "ÆÇ¸Å±Ý¾×ÀÇ %d %% °¡ ¼¼±ÝÀ¸·Î ³ª°¡°ÔµË´Ï´Ù"), dwTax);
	}
	else
	{
		LogManager::instance().ItemLog(ch, item, "BUY", item->GetName());
		LogManager::instance().MoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -(int)dwPrice);
	}
	
	
	
	
	/* 
	
	// Dupe fix
	LPITEM newItem = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);
	if (!newItem)
	{
		sys_err("cannot4 create new item %d %u (%ux) for player %u %s", r_item.itemid, r_item.vnum, r_item.count, ch->GetPlayerID(), ch->GetName());
		return SHOP_SUBHEADER_GC_SOLD_OUT;
	}

	newItem->SetSockets(item->GetSockets());
	newItem->SetAttributes(item->GetAttributes());
	newItem->SetGMOwner(item->IsGMOwner());

	///////////////////// ch->AutoGiveItem(newItem, true);
	ch->SetGoldTime();
	
	// Log for old/new id
	char buff[255];
	sprintf(buff, "NewItemID: %u OldItemID: %u", newItem->GetID(), r_item.itemid);
	ch->tchat("HandlePlayerGiveItem insert || %s", buff);
	LogManager::instance().ItemLog(ch->GetPlayerID(), r_item.itemid, "DELETE_AND_COPY_TO_ID_4", buff, ch->GetDesc()->GetHostName(), r_item.vnum);

	// Force Delete old item ID on all cores + db
	ITEM_MANAGER::Instance().RemoveItem(item, "DEL_TEMP_ITEM4");
	// GG
	network::GGOutputPacket<network::GGForceItemDeletePacket> pack2;
	pack2->set_item_id(r_item.itemid);
	P2P_MANAGER::instance().Send(pack2);
	// GD
	db_clientdesc->DBPacket(HEADER_GD_FORCE_ITEM_DELETE, 0, &r_item.itemid, sizeof(DWORD));
	// Dupe fix

		
	
	item = newItem;
	 */
	
	
	LPITEM oldItem = NULL;
	if (pkStackItem)
	{
		ch->DoStackItem(item, pkStackItem);
		item = pkStackItem;
	}
	else
	{
		item->AddToCharacter(ch, TItemPos(bWindow, iEmptyPos));
	}
	
	if (oldItem) {
		M2_DESTROY_ITEM(oldItem);
	}
	
	// item->SetItemCooltime();
	ch->Save();
	return network::TGCHeader::SHOP_END;
}

bool CShop::AddGuest(LPCHARACTER ch, DWORD owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.insert(GuestMapType::value_type(ch, bOtherEmpire));

	network::GCOutputPacket<network::GCShopStartPacket> pack;

	pack->set_vid(owner_vid);

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ALL_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM& item = m_itemVector[i];

		if (m_pkPC && !item.pkItem)
			continue;

		auto cur = pack->add_items();
		auto cur_item = cur->mutable_item();
		cur->set_display_pos(i);

		if (item.pkItem)
		{
			ITEM_MANAGER::Instance().GetPlayerItem(item.pkItem, cur_item);
		}
		else
		{
			cur_item->set_vnum(item.vnum);
			cur_item->set_count(item.count);
		}

		cur_item->set_price(item.price);
		cur->set_price_item_vnum(item.price_vnum);

#ifdef SECOND_ITEM_PRICE
		cur->set_price2(item.price2);
		cur->set_price_item_vnum2(item.price_vnum2);
#endif
	}

	ch->GetDesc()->Packet(pack);
	return true;
}

void CShop::RemoveGuest(LPCHARACTER ch)
{
	if (ch->GetShop() != this)
		return;

	m_map_guest.erase(ch);
	ch->SetShop(NULL);

	ch->GetDesc()->Packet(network::TGCHeader::SHOP_END);
}

void CShop::RemoveAllGuests()
{
	for (GuestMapType::iterator it = m_map_guest.begin(); it != m_map_guest.end(); ++it)
	{
		LPCHARACTER ch = it->first;
		if (ch && ch->GetDesc() && ch->GetShop() == this)
		{
			ch->SetShop(NULL);
			ch->GetDesc()->Packet(network::TGCHeader::SHOP_END);
		}
	}
	m_map_guest.clear();
}

void CShop::BroadcastUpdateItem(BYTE pos)
{
	network::GCOutputPacket<network::GCShopUpdateItemPacket> pack;

	auto shop_item = pack->mutable_item();
	shop_item->set_display_pos(pos);

	auto item = shop_item->mutable_item();

	if (!m_pkPC || m_itemVector[pos].pkItem)
	{
		if (m_itemVector[pos].pkItem)
		{
			ITEM_MANAGER::Instance().GetPlayerItem(m_itemVector[pos].pkItem, item);
		}
		else
		{
			item->set_vnum(m_itemVector[pos].vnum);
			item->set_count(m_itemVector[pos].count);
		}
	}

	item->set_price(m_itemVector[pos].price);
	shop_item->set_price_item_vnum(m_itemVector[pos].price_vnum);

#ifdef SECOND_ITEM_PRICE
	shop_item->set_price2(m_itemVector[pos].price2);
	shop_item->set_price_item_vnum2(m_itemVector[pos].price_vnum2);
#endif

	Broadcast(pack);
}

int CShop::GetNumberByVnum(DWORD dwVnum)
{
	int itemNumber = 0;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ALL_ITEM_MAX_NUM; ++i)
	{
		const SHOP_ITEM & item = m_itemVector[i];

		if (item.vnum == dwVnum)
		{
			itemNumber += item.count;
		}
	}

	return itemNumber;
}

bool CShop::IsSellingItem(DWORD itemID)
{
	bool isSelling = false;

	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ALL_ITEM_MAX_NUM; ++i)
	{
		if (m_itemVector[i].itemid == itemID)
		{
			isSelling = true;
			break;
		}
	}

	return isSelling;
}

network::TGCHeader CShop::RemoveFromShop(BYTE pos)
{
	bool bIsPCShop = IsPCShop();

	if (!bIsPCShop)
		return network::TGCHeader::SHOP_ERROR_INVALID_POS;

	if (pos >= m_itemVector.size())
	{
		sys_log(0, "Shop::RemoveFromShop : invalid position %d : %s", pos, m_pkPC ? m_pkPC->GetName() : "unknown");
		return network::TGCHeader::SHOP_ERROR_INVALID_POS;
	}

	SHOP_ITEM& r_item = m_itemVector[pos];
	LPITEM pkSelectedItem = ITEM_MANAGER::instance().Find(r_item.itemid);

	if (!pkSelectedItem)
	{
		sys_log(0, "Shop::RemoveFromShop : Critical: This user seems to be a hacker : invalid pcshop item (not exist) : TakePID:%d ItemID %u",
			m_pkPC ? m_pkPC->GetPlayerID() : 0, r_item.itemid);

		return network::TGCHeader::SHOP_ERROR_INVALID_POS;
	}

	if ((pkSelectedItem->GetOwner() != m_pkPC))
	{
		sys_log(0, "Shop::RemoveFromShop : Critical: This user seems to be a hacker : invalid pcshop item (wrong owner) : TakePID:%d ItemID %u owner %p %u",
			m_pkPC ? m_pkPC->GetPlayerID() : 0, r_item.itemid, pkSelectedItem->GetOwner(), pkSelectedItem->GetOwner() ? pkSelectedItem->GetOwner()->GetPlayerID() : 0);

		return network::TGCHeader::SHOP_ERROR_INVALID_POS;
	}

	if (!r_item.pkItem)
		return network::TGCHeader::SHOP_ERROR_SOLDOUT;

	r_item.pkItem = NULL;
	BroadcastUpdateItem(pos);

	return network::TGCHeader::SHOP_END;
}

int CShop::GetPosByItemID(DWORD itemID)
{
	for (DWORD i = 0; i < m_itemVector.size() && i < SHOP_HOST_ALL_ITEM_MAX_NUM; ++i)
	{
		if (m_itemVector[i].itemid == itemID)
		{
			return i;
		}
	}

	return -1;
}

LPITEM CShop::GetItemAtPos(BYTE bPos)
{
	if (bPos >= m_itemVector.size())
	{
		sys_log(0, "Shop::IsItemAtPos : invalid position %d : %s", bPos, m_pkPC ? m_pkPC->GetName() : "unknown");
		return NULL;
	}

	SHOP_ITEM& r_item = m_itemVector[bPos];
	return r_item.pkItem;
}