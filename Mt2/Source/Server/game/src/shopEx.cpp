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
#include "desc_client.h"
#include "shopEx.h"
#include "group_text_parse_tree.h"

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

bool CShopEx::Create(DWORD dwVnum, DWORD dwNPCVnum)
{
	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;
	return true;
}

bool CShopEx::AddShopTable(TShopTableEx& shopTable)
{
	for (itertype(m_vec_shopTabs) it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& _shopTable = *it;
		if (0 != _shopTable.shop_info.vnum() && _shopTable.shop_info.vnum() == shopTable.shop_info.vnum())
			return false;
		if (0 != _shopTable.shop_info.npc_vnum() && _shopTable.shop_info.npc_vnum() == shopTable.shop_info.npc_vnum())
			return false;
	}
	m_vec_shopTabs.push_back(shopTable);
	return true;
}

bool CShopEx::AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.insert(GuestMapType::value_type(ch, bOtherEmpire));

	network::GCOutputPacket<network::GCShopExStartPacket> pack;
	
	pack->set_vid(owner_vid);

#ifdef COMBAT_ZONE
	DWORD dwGetFirstDayHour = CCombatZoneManager::instance().GetFirstDayHour();
	int dwLastBuyTime = ch->GetQuestFlag(COMBAT_ZONE_FLAG_BUY_LAST_TIME);

	if (dwGetFirstDayHour > static_cast<DWORD>(dwLastBuyTime))
		ch->SetQuestFlag(COMBAT_ZONE_FLAG_LIMIT_POINTS, 0);

	pack->set_points(ch->GetRealCombatZonePoints());
	pack->set_cur_limit(ch->GetQuestFlag(COMBAT_ZONE_FLAG_LIMIT_POINTS));
	pack->set_max_limit(COMBAT_ZONE_SHOP_MAX_LIMIT_POINTS);
#endif

	for (const TShopTableEx& shop_tab : m_vec_shopTabs)
	{
		auto& current_tab = *pack->add_tabs();

		current_tab.set_coin_type(shop_tab.coinType);
		current_tab.set_name(shop_tab.name);

		for (BYTE i = 0; i < SHOP_HOST_ITEM_MAX_NUM; i++)
		{
			auto & current_item = *current_tab.add_items();
			current_item = shop_tab.itemsEx[i];

			auto & current_item_data = *current_item.mutable_item();

			switch(shop_tab.coinType)
			{
				case SHOP_COIN_TYPE_GOLD:
					if (bOtherEmpire) // no empire price penalty for pc shop
						current_item_data.set_price(current_item_data.price() * 3);
					break;
			}
		}
	}

	ch->GetDesc()->Packet(pack);

	return true;
}

network::TGCHeader CShopEx::Buy(LPCHARACTER ch, WORD pos)
{
	BYTE tabIdx = pos / SHOP_HOST_ITEM_MAX_NUM;
	BYTE slotPos = pos % SHOP_HOST_ITEM_MAX_NUM;
	if (tabIdx >= GetTabCount())
	{
		sys_log(0, "ShopEx::Buy : invalid position %d : %s", pos, ch->GetName());
		return network::TGCHeader::SHOP_ERROR_INVALID_POS;
	}

	sys_log(0, "ShopEx::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return network::TGCHeader::SHOP_END;

	TShopTableEx& shopTab = m_vec_shopTabs[tabIdx];
	auto& r_item = shopTab.itemsEx[slotPos];

	auto price = r_item.item().price();
	if (price <= 0)
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY;
	}

	switch (shopTab.coinType)
	{
	case SHOP_COIN_TYPE_GOLD:
		if (it->second)	// if other empire, price is triple
			price *= 3;

		if (ch->GetGold() < price)
		{
			sys_log(1, "ShopEx::Buy : Not enough money : %s has %d, price %d", ch->GetName(), ch->GetGold(), price);
			return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY;
		}
		break;
	case SHOP_COIN_TYPE_SECONDARY_COIN:
		{
			int count = ch->CountSpecifyTypeItem(ITEM_SECONDARY_COIN);
			if (count < price)
			{
				sys_log(1, "ShopEx::Buy : Not enough myeongdojun : %s has %d, price %d", ch->GetName(), count, price);
				return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY_EX;
			}
		}
		break;
	case SHOP_COIN_TYPE_ITEM:
		{
			int count = ch->CountSpecifyItem(r_item.price_item_vnum());
			if (count < price)
			{
				return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY_EX;
			}
		}
		break;

#ifdef COMBAT_ZONE
	case SHOP_COIN_TYPE_COMBAT_ZONE:
		int iCurrentLimitPoints = ch->GetQuestFlag(COMBAT_ZONE_FLAG_LIMIT_POINTS);

		if (iCurrentLimitPoints == COMBAT_ZONE_SHOP_MAX_LIMIT_POINTS)
			return network::TGCHeader::SHOP_ERROR_MAX_LIMIT_POINTS;

		if (ch->GetRealCombatZonePoints() < price)
			return network::TGCHeader::SHOP_ERROR_NOT_ENOUGH_POINTS;

		if ((iCurrentLimitPoints + price) > COMBAT_ZONE_SHOP_MAX_LIMIT_POINTS)
			return network::TGCHeader::SHOP_ERROR_OVERFLOW_LIMIT_POINTS;
		break;
#endif
	}
	
	LPITEM item;

	item = ITEM_MANAGER::instance().CreateItem(&r_item.item());

	if (!item)
		return network::TGCHeader::SHOP_ERROR_SOLDOUT;

	BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);

	int iEmptyPos;
#ifdef __DRAGONSOUL__
	if (item->IsDragonSoul())
		iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
	else
#endif
		iEmptyPos = ch->GetEmptyInventoryNew(bWindow, item->GetSize());
	if (iEmptyPos < 0)
	{
		sys_log(1, "ShopEx::Buy : Inventory full : %s size %d", ch->GetName(), item->GetSize());
		M2_DESTROY_ITEM(item);
		return network::TGCHeader::SHOP_ERROR_INVENTORY_FULL;
	}

	switch (shopTab.coinType)
	{
	case SHOP_COIN_TYPE_GOLD:
		ch->PointChange(POINT_GOLD, -price, false);
		break;
	case SHOP_COIN_TYPE_SECONDARY_COIN:
		ch->RemoveSpecifyTypeItem(ITEM_SECONDARY_COIN, price);
		break;
	case SHOP_COIN_TYPE_ITEM:
		ch->RemoveSpecifyItem(r_item.price_item_vnum(), price);
		break;
#ifdef COMBAT_ZONE
	case SHOP_COIN_TYPE_COMBAT_ZONE:
		ch->SetRealCombatZonePoints(ch->GetRealCombatZonePoints() - price);
		ch->SetQuestFlag(COMBAT_ZONE_FLAG_LIMIT_POINTS, ch->GetQuestFlag(COMBAT_ZONE_FLAG_LIMIT_POINTS) + price);
		ch->SetQuestFlag(COMBAT_ZONE_FLAG_BUY_LAST_TIME, get_global_time());

		std::vector<DWORD> m_vec_refreshData;
		m_vec_refreshData.push_back(ch->GetRealCombatZonePoints() - price);
		m_vec_refreshData.push_back(ch->GetQuestFlag(COMBAT_ZONE_FLAG_LIMIT_POINTS));
		m_vec_refreshData.push_back(COMBAT_ZONE_SHOP_MAX_LIMIT_POINTS);
		m_vec_refreshData.push_back(0);
		CCombatZoneManager::instance().SendCombatZoneInfoPacket(ch, COMBAT_ZONE_SUB_HEADER_REFRESH_SHOP, m_vec_refreshData);
		break;
#endif
	}

	item->AddToCharacter(ch, TItemPos(bWindow, iEmptyPos));

	ITEM_MANAGER::instance().FlushDelayedSave(item);
	LogManager::instance().ItemLog(ch, item, "BUY", item->GetName());

	LogManager::instance().MoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -price);

	if (item)
		sys_log(0, "ShopEx: BUY: name %s %s(x %d):%u price %llu", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), (unsigned long long) price);

	ch->Save();

	return network::TGCHeader::SHOP_END;
}

