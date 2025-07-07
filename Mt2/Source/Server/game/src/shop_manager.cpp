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
#include "shop_manager.h"
#include "group_text_parse_tree.h"
#include "shopEx.h"
#include <boost/algorithm/string/predicate.hpp>
#include "shop_manager.h"
#include <cctype>

CShopManager::CShopManager()
{
}

CShopManager::~CShopManager()
{
	Destroy();
}

bool CShopManager::Initialize(const ::google::protobuf::RepeatedPtrField<network::TShopTable>& table)
{
	for (TShopMap::iterator it = m_map_pkShop.begin(); it != m_map_pkShop.end(); ++it)
		it->second->RemoveAllGuests();

	m_map_pkShop.clear();
	m_map_pkShopByNPCVnum.clear();

	for (auto& t : table)
	{
		LPSHOP shop = M2_NEW CShop;

		if (!shop->Create(t.vnum(), t.npc_vnum(), t.items()))
		{
			M2_DELETE(shop);
			continue;
		}

		m_map_pkShop.insert(TShopMap::value_type(t.vnum(), shop));
		m_map_pkShopByNPCVnum.insert(TShopMap::value_type(t.npc_vnum(), shop));
	}

	char szShopTableExFileName[256];

	snprintf(szShopTableExFileName, sizeof(szShopTableExFileName),
		"%s/shop_table_ex.txt", Locale_GetBasePath().c_str());

	return true;//ReadShopTableEx(szShopTableExFileName);
}

void CShopManager::Destroy()
{
	TShopMap::iterator it = m_map_pkShop.begin();

	while (it != m_map_pkShop.end())
	{
		M2_DELETE(it->second);
		++it;
	}

	m_map_pkShop.clear();
}

LPSHOP CShopManager::Get(DWORD dwVnum)
{
	TShopMap::const_iterator it = m_map_pkShop.find(dwVnum);

	if (it == m_map_pkShop.end())
		return NULL;

	return (it->second);
}

LPSHOP CShopManager::GetByNPCVnum(DWORD dwVnum)
{
	TShopMap::const_iterator it = m_map_pkShopByNPCVnum.find(dwVnum);

	if (test_server)
		sys_log(0, "GetShopByNPCVnum(%u) => %p", dwVnum, it == m_map_pkShopByNPCVnum.end() ? NULL : it->second);

	if (it == m_map_pkShopByNPCVnum.end())
		return NULL;

	return (it->second);
}

/*
 * 인터페이스 함수들
 */

// 상점 거래를 시작
bool CShopManager::StartShopping(LPCHARACTER pkChr, LPCHARACTER pkChrShopKeeper, int iShopVnum)
{
	if (test_server)
		sys_log(0, "StartShopping [%s at %u %s vnum %u]", pkChr->GetName(), pkChrShopKeeper ? pkChrShopKeeper->GetRaceNum() : 0, pkChrShopKeeper ? pkChrShopKeeper->GetName() : "none", iShopVnum);

	if (pkChrShopKeeper && pkChr->GetShopOwner() == pkChrShopKeeper)
		return false;
	// this method is only for NPC
	if (pkChrShopKeeper && pkChrShopKeeper->IsPC())
		return false;
	// not if dead
	if (pkChr->IsDead())
		return false;

	//PREVENT_TRADE_WINDOW
	if (!pkChr->CanShopNow() || pkChr->GetMyShop())
	{
		pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChr, "´Ù¸¥ °Å·¡Ã¢ÀÌ ¿­¸°»óÅÂ¿¡¼­´Â »óÁ¡°Å·¡¸¦ ÇÒ¼ö °¡ ¾ø½À´Ï´Ù."));
		return false;
	}
	//END_PREVENT_TRADE_WINDOW

	if (pkChrShopKeeper)
	{
		long distance = DISTANCE_APPROX(pkChr->GetX() - pkChrShopKeeper->GetX(), pkChr->GetY() - pkChrShopKeeper->GetY());

		if (distance >= SHOP_MAX_DISTANCE)
		{
			sys_log(1, "SHOP: TOO_FAR: %s distance %d", pkChr->GetName(), distance);
			return false;
		}
	}

	LPSHOP pkShop;

	if (iShopVnum)
		pkShop = Get(iShopVnum);
	else if (pkChrShopKeeper)
		pkShop = GetByNPCVnum(pkChrShopKeeper->GetRaceNum());
	else
		return false;

	if (!pkShop)
	{
		if (test_server)
			sys_log(0, "SHOP: NO SHOP");
		return false;
	}

	bool bOtherEmpire = false;

	if (pkChrShopKeeper && pkChr->GetEmpire() != pkChrShopKeeper->GetEmpire())
		bOtherEmpire = true;

	pkShop->AddGuest(pkChr, pkChrShopKeeper ? pkChrShopKeeper->GetVID() : 0, bOtherEmpire);
	pkChr->SetShopOwner(pkChrShopKeeper);
	sys_log(0, "SHOP: START: %s", pkChr->GetName());
	return true;
}

bool CShopManager::StartShopping(LPCHARACTER pkChr, int iShopVnum)
{
	if (test_server)
		sys_log(0, "StartShopping [%s at vnum %u]", pkChr->GetName(), iShopVnum);

	// not if dead
	if (pkChr->IsDead())
		return false;

	//PREVENT_TRADE_WINDOW
	if (!pkChr->CanShopNow() || pkChr->GetMyShop())
	{
		pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChr, "´Ù¸¥ °Å·¡Ã¢ÀÌ ¿­¸°»óÅÂ¿¡¼­´Â »óÁ¡°Å·¡¸¦ ÇÒ¼ö °¡ ¾ø½À´Ï´Ù."));
		return false;
	}
	//END_PREVENT_TRADE_WINDOW

	LPSHOP pkShop;

	pkShop = Get(iShopVnum);

	if (!pkShop)
	{
		if (test_server)
			sys_log(0, "SHOP: NO SHOP");
		return false;
	}
	pkShop->AddGuest(pkChr, pkChr->GetVID(), 0);

	sys_log(0, "SHOP: START: %s", pkChr->GetName());
	return true;
}

LPSHOP CShopManager::FindPCShop(DWORD dwVID)
{
	TShopMap::iterator it = m_map_pkShopByPC.find(dwVID);

	if (it == m_map_pkShopByPC.end())
		return NULL;

	return it->second;
}

LPSHOP CShopManager::CreatePCShop(LPCHARACTER ch, const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& table)
{
	if (FindPCShop(ch->GetVID()))
		return NULL;

	LPSHOP pkShop = M2_NEW CShop;
	pkShop->SetPCShop(ch);
	pkShop->SetShopItems(table, table.size());

	m_map_pkShopByPC.insert(TShopMap::value_type(ch->GetVID(), pkShop));
	return pkShop;
}

void CShopManager::DestroyPCShop(LPCHARACTER ch)
{
	LPSHOP pkShop = FindPCShop(ch->GetVID());

	if (!pkShop)
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY
	
	m_map_pkShopByPC.erase(ch->GetVID());
	M2_DELETE(pkShop);
}

// 상점 거래를 종료
void CShopManager::StopShopping(LPCHARACTER ch)
{
	LPSHOP shop;

	if (!(shop = ch->GetShop()))
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY
	
	shop->RemoveGuest(ch);
	sys_log(0, "SHOP: END: %s", ch->GetName());
}

// 아이템 구입
void CShopManager::Buy(LPCHARACTER ch, BYTE pos)
{
	if (!ch->GetShop())
		return;

/*	if (!ch->GetShopOwner())
		return;

	if (DISTANCE_APPROX(ch->GetX() - ch->GetShopOwner()->GetX(), ch->GetY() - ch->GetShopOwner()->GetY()) > 2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»óÁ¡°úÀÇ °Å¸®°¡ ³Ê¹« ¸Ö¾î ¹°°ÇÀ» »ì ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
*/
	
	CShop* pkShop = ch->GetShop();

	//PREVENT_ITEM_COPY
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY

	auto ret = pkShop->Buy(ch, pos);

	if (network::TGCHeader::SHOP_END != ret) // ¹®Á¦°¡ ÀÖ¾úÀ¸¸é º¸³½´Ù.
	{
		ch->GetDesc()->Packet(ret);
	}
}

#ifdef INCREASE_ITEM_STACK
void CShopManager::Sell(LPCHARACTER ch, WORD wCell, WORD bCount)
#else
void CShopManager::Sell(LPCHARACTER ch, WORD wCell, BYTE bCount)
#endif
{
#ifndef SHOP_SYSTEM_SELL_WITHOUT_SHOP
	if (!ch->GetShop())
		return;

	if (!ch->GetShopOwner())
		return;
#endif

	if (!ch->CanHandleItem())
		return;

#ifndef SHOP_SYSTEM_SELL_WITHOUT_SHOP
	if (ch->GetShop()->IsPCShop())
		return;

	if (DISTANCE_APPROX(ch->GetX()-ch->GetShopOwner()->GetX(), ch->GetY()-ch->GetShopOwner()->GetY())>2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»óÁ¡°úÀÇ °Å¸®°¡ ³Ê¹« ¸Ö¾î ¹°°ÇÀ» ÆÈ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
#endif

	LPITEM item = ch->GetInventoryItem(wCell);

	if (!item)
		return;

	if (item->IsEquipped() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Âø¿ë ÁßÀÎ ¾ÆÀÌÅÛÀº ÆÇ¸ÅÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (true == item->isLocked())
	{
		return;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
		return;

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	DWORD dwPrice = item->GetShopBuyPrice();

	if (item->GetShopBuyPrice() > item->GetGold())
	{
		ch->tchat("%d %d,", item->GetShopBuyPrice() > item->GetGold());
		dwPrice = item->GetGold();
	}


	if (IS_SET(item->GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

	

		// return;

	dwPrice /= 5;
	
	ch->tchat("buy price %d sell endprice %d", item->GetShopBuyPrice(), dwPrice);
	
	//세금 계산
	DWORD dwTax = dwPrice * 3 / 100;
	dwPrice -= dwTax;

	if (test_server)
		sys_log(0, "Sell Item price id %u %s itemid %u", ch->GetPlayerID(), ch->GetName(), item->GetID());

	const int64_t nTotalMoney = static_cast<int64_t>(ch->GetGold()) + static_cast<int64_t>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] id %u name %s gold %u", ch->GetPlayerID(), ch->GetName(), ch->GetGold());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "20¾ï³ÉÀÌ ÃÊ°úÇÏ¿© ¹°Ç°À» ÆÈ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	// sys_log(0, "SHOP: SELL: %s item name: %s(x%u):%u price: %u", ch->GetName(), item->GetName(), bCount, item->GetID(), dwPrice);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÆÇ¸Å±Ý¾×ÀÇ %d %% °¡ ¼¼±ÝÀ¸·Î ³ª°¡°ÔµË´Ï´Ù"), 3); // 3 % taxes

	LogManager::instance().MoneyLog(MONEY_LOG_SHOP, item->GetVnum(), dwPrice);
	LogManager::instance().ItemDestroyLog(LogManager::ITEM_DESTROY_SELL_NPC, item, bCount);

	ch->SetQuestItemPtr(item);
	quest::CQuestManager::instance().OnSellItem(ch->GetPlayerID());

	if (bCount == item->GetCount())
	{
		ITEM_MANAGER::instance().RemoveItem(item, "SELL");
	}
	else
		item->SetCount(item->GetCount() - bCount);

	ch->PointChange(POINT_GOLD, dwPrice, false);
}

bool CompareShopItemName(const network::TShopItemTable& lhs, const network::TShopItemTable& rhs)
{
	auto lItem = ITEM_MANAGER::instance().GetTable(lhs.item().vnum());
	auto rItem = ITEM_MANAGER::instance().GetTable(rhs.item().vnum());
	if (lItem && rItem)
		return strcmp(lItem->name().c_str(), rItem->name().c_str()) < 0;
	else
		return true;
}

bool ConvertToShopItemTable(IN CGroupNode* pNode, OUT TShopTableEx& shopTable)
{
	DWORD vnum;
	if (!pNode->GetValue("vnum", 0, vnum))
	{
		sys_err("Group %s does not have vnum.", pNode->GetNodeName().c_str());
		return false;
	}
	shopTable.shop_info.set_vnum(vnum);

	if (!pNode->GetValue("name", 0, shopTable.name))
	{
		sys_err("Group %s does not have name.", pNode->GetNodeName().c_str());
		return false;
	}
	
	if (shopTable.name.length() >= SHOP_TAB_NAME_MAX)
	{
		sys_err("Shop name length must be less than %d. Error in Group %s, name %s", SHOP_TAB_NAME_MAX, pNode->GetNodeName().c_str(), shopTable.name.c_str());
		return false;
	}

	std::string stCoinType;
	if (!pNode->GetValue("cointype", 0, stCoinType))
	{
		stCoinType = "Gold";
	}
	
	if (boost::iequals(stCoinType, "Gold"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_GOLD;
	}
	else if (boost::iequals(stCoinType, "SecondaryCoin"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_SECONDARY_COIN;
	}
	else if (boost::iequals(stCoinType, "Item"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_ITEM;
	}
#ifdef COMBAT_ZONE
	else if (boost::iequals(stCoinType, "CombatZone"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_COMBAT_ZONE;
	}
#endif
	else
	{
		sys_err("Group %s has undefine cointype(%s).", pNode->GetNodeName().c_str(), stCoinType.c_str());
		return false;
	}

	CGroupNode* pItemGroup = pNode->GetChildNode("items");
	if (!pItemGroup)
	{
		sys_err("Group %s does not have 'group items'.", pNode->GetNodeName().c_str());
		return false;
	}

	int itemGroupSize = pItemGroup->GetRowCount();
	std::vector <network::TShopItemTable> shopItems(itemGroupSize);
	if (itemGroupSize >= SHOP_HOST_ITEM_MAX_NUM)
	{
		sys_err("count(%d) of rows of group items of group %s must be smaller than %d", itemGroupSize, pNode->GetNodeName().c_str(), SHOP_HOST_ITEM_MAX_NUM);
		return false;
	}

	for (int i = 0; i < itemGroupSize; i++)
	{
		DWORD vnum;
		if (!pItemGroup->GetValue(i, "vnum", vnum))
		{
			sys_err("row(%d) of group items of group %s does not have vnum column", i, pNode->GetNodeName().c_str());
			return false;
		}
		shopItems[i].mutable_item()->set_vnum(vnum);
		
		DWORD count;
		if (!pItemGroup->GetValue(i, "count", count))
		{
			sys_err("row(%d) of group items of group %s does not have count column", i, pNode->GetNodeName().c_str());
			return false;
		}
		shopItems[i].mutable_item()->set_count(count);

		ULONGLONG price;
		if (!pItemGroup->GetValue(i, "price", price))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
		shopItems[i].mutable_item()->set_price(price);

		if (shopTable.coinType == SHOP_COIN_TYPE_ITEM)
		{
			DWORD price_item_vnum;
			if (!pItemGroup->GetValue(i, "price_vnum", price_item_vnum))
			{
				sys_err("row(%d) of group items of group %s does not have price_vnum column", i, pNode->GetNodeName().c_str());
				return false;
			}

			shopItems[i].set_price_item_vnum(price_item_vnum);
		}
	}
	std::string stSort;
	if (!pNode->GetValue("sort", 0, stSort))
	{
		stSort = "None";
	}

	if (boost::iequals(stSort, "Asc"))
	{
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemName);
	}
	else if(boost::iequals(stSort, "Desc"))
	{
		std::sort(shopItems.rbegin(), shopItems.rend(), CompareShopItemName);
	}

	CGrid grid = CGrid(5, 9);
	int iPos;

	for (size_t i = 0; i < shopItems.size(); i++)
	{
		auto item_table = ITEM_MANAGER::instance().GetTable(shopItems[i].item().vnum());
		if (!item_table)
		{
			sys_err("vnum(%d) of group items of group %s does not exist", shopItems[i].item().vnum(), pNode->GetNodeName().c_str());
			return false;
		}

		iPos = grid.FindBlank(1, item_table->size());

		sys_log(0, "AddShopExItem [%d] => %u pos %d", i, shopItems[i].item().vnum(), iPos);

		grid.Put(iPos, 1, item_table->size());
		shopTable.itemsEx[iPos] = shopItems[i];
	}

	return true;
}

bool CShopManager::ReadShopTableEx(const char* stFileName)
{
	// file 유무 체크.
	// 없는 경우는 에러로 처리하지 않는다.
	FILE* fp = fopen(stFileName, "rb");
	if (NULL == fp)
		return true;
	fclose(fp);

	CGroupTextParseTreeLoader loader;
	if (!loader.Load(stFileName))
	{
		sys_err("%s Load fail.", stFileName);
		return false;
	}

	CGroupNode* pShopNPCGroup = loader.GetGroup("shopnpc");
	if (NULL == pShopNPCGroup)
	{
		sys_err("Group ShopNPC is not exist.");
		return false;
	}

	typedef std::multimap <DWORD, TShopTableEx> TMapNPCshop;
	TMapNPCshop map_npcShop;
	for (int i = 0; i < pShopNPCGroup->GetRowCount(); i++)
	{
		DWORD npcVnum;
		std::string shopName;
		if (!pShopNPCGroup->GetValue(i, "npc", npcVnum) || !pShopNPCGroup->GetValue(i, "group", shopName))
		{
			sys_err("Invalid row(%d). Group ShopNPC rows must have 'npc', 'group' columns", i);
			return false;
		}
		std::transform(shopName.begin(), shopName.end(), shopName.begin(), (int(*)(int))std::tolower);
		CGroupNode* pShopGroup = loader.GetGroup(shopName.c_str());
		if (!pShopGroup)
		{
			sys_err("Group %s is not exist.", shopName.c_str());
			return false;
		}
		TShopTableEx table;
		if (!ConvertToShopItemTable(pShopGroup, table))
		{
			sys_err("Cannot read Group %s.", shopName.c_str());
			return false;
		}
		if (m_map_pkShopByNPCVnum.find(npcVnum) != m_map_pkShopByNPCVnum.end())
		{
			sys_err("%d cannot have both original shop and extended shop", npcVnum);
			return false;
		}
		
		map_npcShop.insert(TMapNPCshop::value_type(npcVnum, table));	
	}

	for (TMapNPCshop::iterator it = map_npcShop.begin(); it != map_npcShop.end(); ++it)
	{
		DWORD npcVnum = it->first;
		TShopTableEx& table = it->second;
		if (m_map_pkShop.find(table.shop_info.vnum()) != m_map_pkShop.end())
		{
			sys_err("Shop vnum(%d) already exists", table.shop_info.vnum());
			return false;
		}
		TShopMap::iterator shop_it = m_map_pkShopByNPCVnum.find(npcVnum);
		
		LPSHOPEX pkShopEx = NULL;
		if (m_map_pkShopByNPCVnum.end() == shop_it)
		{
			pkShopEx = M2_NEW CShopEx;
			pkShopEx->Create(0, npcVnum);
			m_map_pkShopByNPCVnum.insert(TShopMap::value_type(npcVnum, pkShopEx));
			sys_log(0, "ShopEx inserted with vnum %u ptr %p (find : %d | find : %p)", npcVnum, pkShopEx, m_map_pkShopByNPCVnum.find(npcVnum) != m_map_pkShopByNPCVnum.end(), GetByNPCVnum(npcVnum));
		}
		else
		{
			pkShopEx = dynamic_cast <CShopEx*> (shop_it->second);
			if (NULL == pkShopEx)
			{
				sys_err("WTF!!! It can't be happend. NPC(%d) Shop is not extended version.", shop_it->first);
				return false;
			}
		}
		GetByNPCVnum(npcVnum);
		if (pkShopEx->GetTabCount() >= SHOP_TAB_COUNT_MAX)
		{
			sys_err("ShopEx cannot have tab more than %d", SHOP_TAB_COUNT_MAX);
			return false;
		}

		GetByNPCVnum(npcVnum);
		if (pkShopEx->GetVnum() != 0 && m_map_pkShop.find(pkShopEx->GetVnum()) != m_map_pkShop.end())
		{
			sys_err("Shop vnum(%d) already exist.", pkShopEx->GetVnum());
			return false;
		}
		GetByNPCVnum(npcVnum);
		m_map_pkShop.insert(TShopMap::value_type(pkShopEx->GetVnum(), pkShopEx));
		GetByNPCVnum(npcVnum);
		pkShopEx->AddShopTable(table);

		sys_log(0, "Add ShopEx NPC %u (findByNpcVNUM %u = %p)", npcVnum, npcVnum, GetByNPCVnum(npcVnum));
	}

	return true;
}
