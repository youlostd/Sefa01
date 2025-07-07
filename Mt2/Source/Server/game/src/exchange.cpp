#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "utils.h"
#include "desc.h"
#include "desc_client.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "../../common/length.h"
#include "exchange.h"
#include "gm.h"

#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_MESSENGER_BLOCK
#include "messenger_manager.h"
#endif

bool CHARACTER::ExchangeStart(LPCHARACTER victim)
{
	if (this == victim)	// ÀÚ±â ÀÚ½Å°ú´Â ±³È¯À» ¸øÇÑ´Ù.
		return false;

	if (IsObserverMode())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°üÀü »óÅÂ¿¡¼­´Â ±³È¯À» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (victim->IsNPC())
		return false;

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()) || CCombatZoneManager::Instance().IsCombatZoneMap(victim->GetMapIndex()))
		return false;
#endif

#ifdef ENABLE_MESSENGER_BLOCK
	if (this && victim && MessengerManager::instance().CheckMessengerList(GetName(), victim->GetName(), SYST_BLOCK))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't use this action because you have blocked %s. "), victim->GetName());
		return false;
	}
#endif

	if ((!GM::check_allow(GetGMLevel(), GM_ALLOW_EXCHANGE_TO_GM) && victim->IsGM()) ||
		(!GM::check_allow(GetGMLevel(), GM_ALLOW_EXCHANGE_TO_PLAYER) && !victim->IsGM()))
	{
		if (!GM::check_allow(GetGMLevel(), GM_ALLOW_EXCHANGE_TO_GM) && victim->IsGM())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You are not allowed to trade to gamemasters."));
		else if (!GM::check_allow(GetGMLevel(), GM_ALLOW_EXCHANGE_TO_PLAYER) && !victim->IsGM())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You are not allowed to trade to players."));
		return false;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (GetDesc()->IsTradeblocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return false;
	}
#endif

	//PREVENT_TRADE_WINDOW
	if (!CanShopNow() || GetShop() || GetMyShop())
	{
		ChatPacket( CHAT_TYPE_INFO, LC_TEXT(this, "´Ù¸¥ °Å·¡Ã¢ÀÌ ¿­·ÁÀÖÀ»°æ¿ì °Å·¡¸¦ ÇÒ¼ö ¾ø½À´Ï´Ù." ) );
		return false;
	}

	if (!victim->CanShopNow() || victim->GetShop() || victim->GetMyShop())
	{
		ChatPacket( CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ ´Ù¸¥ °Å·¡ÁßÀÌ¶ó °Å·¡¸¦ ÇÒ¼ö ¾ø½À´Ï´Ù." ) );
		return false;
	}
	//END_PREVENT_TRADE_WINDOW
	int iDist = DISTANCE_APPROX(GetX() - victim->GetX(), GetY() - victim->GetY());

	// °Å¸® Ã¼Å©
	if (iDist >= EXCHANGE_MAX_DISTANCE)
		return false;

	if (GetExchange())
		return false;

	if (victim->GetExchange())
	{
		GetDesc()->Packet(network::TGCHeader::EXCHANGE_ALREADY);
		return false;
	}

	if (victim->IsBlockMode(BLOCK_EXCHANGE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ ±³È¯ °ÅºÎ »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	SetExchange(M2_NEW CExchange(this));
	victim->SetExchange(M2_NEW CExchange(victim));

	victim->GetExchange()->SetCompany(GetExchange());
	GetExchange()->SetCompany(victim->GetExchange());

	//
	SetExchangeTime();
	victim->SetExchangeTime();

	network::GCOutputPacket<network::GCExchangeStartPacket> pack;

	pack->set_target_vid(GetVID());
	victim->GetDesc()->Packet(pack);

	pack->set_target_vid(victim->GetVID());
	GetDesc()->Packet(pack);

	return true;
}

CExchange::CExchange(LPCHARACTER pOwner)
{
	m_pCompany = NULL;

	m_bAccept = false;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		m_apItems[i] = NULL;
		m_aItemPos[i] = NPOS;
		m_abItemDisplayPos[i] = 0;
	}

	m_llGold = 0;

	m_pOwner = pOwner;
	pOwner->SetExchange(this);

	m_pGrid = M2_NEW CGrid(4,6);
}

CExchange::~CExchange()
{
	M2_DELETE(m_pGrid);
}

bool CExchange::AddItem(TItemPos item_pos, BYTE display_pos)
{
	assert(m_pOwner != NULL && GetCompany());
	
	if (!item_pos.IsValidItemPosition())
		return false;
	
	// Àåºñ´Â ±³È¯ÇÒ ¼ö ¾øÀ½
	if (item_pos.IsEquipPosition())
		return false;
	
	LPITEM item;

	if (!(item = m_pOwner->GetItem(item_pos)))
		return false;
	
	LPCHARACTER pkVictim = GetCompany()->GetOwner();
	if (!pkVictim)
		return false;
	
#ifdef __TRADE_BLOCK_SYSTEM__
	if (m_pOwner->IsTradeBlocked())
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "You can't do this now. Please write COMA/GA/DEV/SA a message on Discord or Forum.");
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "This is a security mesurement against payment fraud. Don't worry it will resolve shortly.");
	}
#endif

	if (item->IsGMOwner())
	{
		if (pkVictim->IsGM() && !GM::check_allow(m_pOwner->GetGMLevel(), GM_ALLOW_EXCHANGE_GM_ITEM_TO_GM))
		{
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "You may not exchange a gm owned item to a gm."));
			return false;
		}
		else if (!pkVictim->IsGM() && !GM::check_allow(m_pOwner->GetGMLevel(), GM_ALLOW_EXCHANGE_GM_ITEM_TO_PLAYER))
		{
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "You may not exchange a gm owned item to a player."));
			return false;
		}
	}
	else
	{
		if (pkVictim->IsGM() && !GM::check_allow(m_pOwner->GetGMLevel(), GM_ALLOW_EXCHANGE_PLAYER_ITEM_TO_GM))
		{
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "You may not exchange a player owned item to a gm."));
			return false;
		}
		else if (!pkVictim->IsGM() && !GM::check_allow(m_pOwner->GetGMLevel(), GM_ALLOW_EXCHANGE_PLAYER_ITEM_TO_PLAYER))
		{
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "You may not exchange a player owned item to a player."));
			return false;
		}
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (m_pOwner->GetDesc()->IsTradeblocked())
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return false;
	}
#endif

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE))
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "¾ÆÀÌÅÛÀ» °Ç³×ÁÙ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (true == item->isLocked())
	{
		return false;
	}

	// ÀÌ¹Ì ±³È¯Ã¢¿¡ Ãß°¡µÈ ¾ÆÀÌÅÛÀÎ°¡?
	if (item->IsExchanging())
	{
		sys_log(0, "EXCHANGE under exchanging");
		return false;
	}

	if (!m_pGrid->IsEmpty(display_pos, 1, item->GetSize()))
	{
		sys_log(0, "EXCHANGE not empty item_pos %d %d %d", display_pos, 1, item->GetSize());
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			continue;

		m_apItems[i]		= item;
		m_aItemPos[i]		= item_pos;
		m_abItemDisplayPos[i]	= display_pos;
		m_pGrid->Put(display_pos, 1, item->GetSize());

		item->SetExchanging(true);

		network::GCOutputPacket<network::GCExchangeItemAddPacket> pack;
		ITEM_MANAGER::Instance().GetPlayerItem(item, pack->mutable_data());
		pack->set_display_pos(display_pos);

		pack->set_is_me(true);
		m_pOwner->GetDesc()->Packet(pack);

		pack->set_is_me(false);
		GetCompany()->GetOwner()->GetDesc()->Packet(pack);

		sys_log(0, "EXCHANGE AddItem success %s pos(%d, %d) %d", item->GetName(), item_pos.window_type, item_pos.cell, display_pos);

		return true;
	}

	// Ãß°¡ÇÒ °ø°£ÀÌ ¾øÀ½
	return false;
}

bool CExchange::RemoveItem(BYTE pos)
{
	return false;
	
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return false;

	if (!m_apItems[pos])
		return false;

	TItemPos PosOfInventory = m_aItemPos[pos];
	m_apItems[pos]->SetExchanging(false);

	m_pGrid->Get(m_abItemDisplayPos[pos], 1, m_apItems[pos]->GetSize());

	network::GCOutputPacket<network::GCExchangeItemDelPacket> pack;
	*pack->mutable_inventory_pos() = TItemPos(m_apItems[pos]->GetWindow(), m_apItems[pos]->GetCell());
	pack->set_display_pos(pos);

	pack->set_is_me(true);
	GetOwner()->GetDesc()->Packet(pack);

	pack->set_is_me(false);
	GetCompany()->GetOwner()->GetDesc()->Packet(pack);

	Accept(false);
	GetCompany()->Accept(false);

	m_apItems[pos]		= NULL;
	m_aItemPos[pos]		= NPOS;
	m_abItemDisplayPos[pos] = 0;
	return true;
}

bool CExchange::AddGold(long long gold)
{
	if (gold <= 0)
		return false;

#ifdef ACCOUNT_TRADE_BLOCK
	if (GetOwner()->GetDesc()->IsTradeblocked())
		return false;
#endif

	if (GetOwner()->GetGold() < gold)
	{
		GetOwner()->GetDesc()->Packet(network::TGCHeader::EXCHANGE_LESS_GOLD);
		return false;
	}

	if ( m_llGold > 0 )
	{
		return false;
	}

	Accept(false);
	GetCompany()->Accept(false);

	m_llGold = gold;

	network::GCOutputPacket<network::GCExchangeGoldAddPacket> pack;
	pack->set_gold(m_llGold);

	pack->set_is_me(true);
	GetOwner()->GetDesc()->Packet(pack);

	pack->set_is_me(false);
	GetCompany()->GetOwner()->GetDesc()->Packet(pack);

	return true;
}

// µ·ÀÌ ÃæºÐÈ÷ ÀÖ´ÂÁö, ±³È¯ÇÏ·Á´Â ¾ÆÀÌÅÛÀÌ ½ÇÁ¦·Î ÀÖ´ÂÁö È®ÀÎ ÇÑ´Ù.
bool CExchange::Check(int * piItemCount)
{
	if (GetOwner()->GetGold() < m_llGold)
		return false;

	if (GetCompany()->GetOwner()->GetGold() + m_llGold > GOLD_MAX)
		return false;

	int item_count = 0;

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!m_apItems[i])
			continue;

		if (!m_aItemPos[i].IsValidItemPosition())
			return false;

		if (m_apItems[i] != GetOwner()->GetItem(m_aItemPos[i]))
			return false;

		++item_count;
	}

	*piItemCount = item_count;
	return true;
}

typedef std::vector<CGrid*> TGridVec;
bool CExchange::CheckSpace()
{
#ifdef __DRAGONSOUL__
	static std::vector <WORD> s_vDSGrid(DRAGON_SOUL_INVENTORY_MAX_NUM);
#endif

	std::vector<BYTE> vecWindows;
	vecWindows.push_back(INVENTORY);
#ifdef __SKILLBOOK_INVENTORY__
	vecWindows.push_back(SKILLBOOK_INVENTORY);
#endif
	vecWindows.push_back(UPPITEM_INVENTORY);
	vecWindows.push_back(STONE_INVENTORY);
	vecWindows.push_back(ENCHANT_INVENTORY);
#ifdef __COSTUME_INVENTORY__
	vecWindows.push_back(COSTUME_INVENTORY);
#endif

	LPCHARACTER	victim = GetCompany()->GetOwner();
	LPITEM item;

	TGridVec* pGridVec = new TGridVec[vecWindows.size()];
	if (vecWindows.size() > 0)
	{
		for (int i = 0; i < vecWindows.size(); ++i)
		{
			int iInvPageSize = ITEM_MANAGER::instance().GetInventoryPageSize(vecWindows[i]);
			int iInvMaxNum = ITEM_MANAGER::instance().GetInventoryMaxNum(vecWindows[i], victim);

			for (int iPage = 0; iInvMaxNum > 0; ++iPage, iInvMaxNum -= iInvPageSize)
			{
				CGrid* pGrid = new CGrid(5, MIN(iInvMaxNum, iInvPageSize) / 5);
				pGrid->Clear();

				pGridVec[i].push_back(pGrid);
			}
		}
	}

	int iWndIndex = 0;
	int iSlotIndex = -1;
	for (int iWndIndex = 0; iWndIndex < vecWindows.size(); ++iWndIndex)
	{
		BYTE bWindow = vecWindows[iWndIndex];
		int iInvPageSize = ITEM_MANAGER::instance().GetInventoryPageSize(bWindow);
		int iInvMaxNum = ITEM_MANAGER::instance().GetInventoryMaxNum(bWindow, victim);
		int iInvStart = ITEM_MANAGER::instance().GetInventoryStart(bWindow);

		int iRealSlot = iInvStart;
		for (int iPage = 0, iTmpInvMaxNum = iInvMaxNum; iTmpInvMaxNum > 0; ++iPage, iTmpInvMaxNum -= iInvPageSize)
		{
			for (int iSlot = 0; iSlot < MIN(iTmpInvMaxNum, iInvPageSize); ++iSlot, ++iRealSlot)
			{
				if (!(item = victim->GetInventoryItem(iRealSlot)))
					continue;

				pGridVec[iWndIndex][iPage]->Put(iSlot, 1, item->GetSize());
			}
		}

		iSlotIndex += iInvPageSize;
		if (iSlotIndex >= iInvStart + iInvMaxNum)
		{
			++iWndIndex;
			iSlotIndex = -1;
		}
	}

#ifdef __DRAGONSOUL__
	bool bDSInitialized = false;
#endif
	bool bFree = true;
	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
		{
			if (!victim->DragonSoul_IsQualified())
			{
				return false;
			}

			if (!bDSInitialized)
			{
				bDSInitialized = true;
				victim->CopyDragonSoulItemGrid(s_vDSGrid);
			}

			bool bExistEmptySpace = false;
			WORD wBasePos = DSManager::instance().GetBasePosition(item);
			if (wBasePos >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; i++)
			{
				WORD wPos = wBasePos + i;
				if (0 == s_vDSGrid[wPos])
				{
					bool bEmpty = true;
					for (int j = 1; j < item->GetSize(); j++)
					{
						if (s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM])
						{
							bEmpty = false;
							break;
						}
					}
					if (bEmpty)
					{
						for (int j = 0; j < item->GetSize(); j++)
						{
							s_vDSGrid[wPos + j * DRAGON_SOUL_BOX_COLUMN_NUM] = wPos + 1;
						}
						bExistEmptySpace = true;
						break;
					}
				}
				if (bExistEmptySpace)
					break;
			}
			if (!bExistEmptySpace)
				return false;

			continue;
		}
#endif

		BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);

		bFree = false;
		TGridVec* pCurGrid = NULL;
		for (int i = 0; i < vecWindows.size(); ++i)
		{
			if (vecWindows[i] == bWindow)
			{
				pCurGrid = &pGridVec[i];
				break;
			}
		}

		if (!pCurGrid)
		{
			sys_err("invalid window %u", bWindow);
			break;
		}

		for (int iPage = 0; iPage < pCurGrid->size(); ++iPage)
		{
			int iPos = (*pCurGrid)[iPage]->FindBlank(1, item->GetSize());
			if (iPos >= 0)
			{
				(*pCurGrid)[iPage]->Put(iPos, 1, item->GetSize());
				bFree = true;
				break;
			}
		}

		if (!bFree)
			break;
	}

	for (int i = 0; i < vecWindows.size(); ++i)
	{
		for (int iPage = 0; iPage < pGridVec[i].size(); ++iPage)
			M2_DELETE(pGridVec[i][iPage]);
	}
	delete[] pGridVec;

	return bFree;
}

// ±³È¯ ³¡ (¾ÆÀÌÅÛ°ú µ· µîÀ» ½ÇÁ¦·Î ¿Å±ä´Ù)
bool CExchange::Done()
{
	int		empty_pos, i;
	LPITEM	item;

	long long ull_golditems = 0;
	DWORD dwGoldbars = 0;

	LPCHARACTER	victim = GetCompany()->GetOwner();

	std::deque<LPITEM> dq_TradeItems{ };

	// Push items to deque (sort by target window)
	for (i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (!(item = m_apItems[i]))
			continue;

		// Pass items with target window == INVENTORY first
		if (ITEM_MANAGER::instance().GetTargetWindow(item) == INVENTORY)
			dq_TradeItems.push_front(item);
		else
			dq_TradeItems.push_back(item);

		m_apItems[i] = NULL;
	}

	for (auto & item : dq_TradeItems)
	{
		BYTE bTargetWindow = INVENTORY;

		// Find empty position in inventory
#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
			empty_pos = victim->GetEmptyDragonSoulInventory(item);
		else 
#endif
			empty_pos = victim->GetEmptyInventory(item->GetSize());
		
		// If no empty space in INVENTORY, add the item to it's storage instead!
		if ( empty_pos == -1
#ifdef __DRAGONSOUL__ 
			&& !item->IsDragonSoul()
#endif	
			)
		{
			bTargetWindow = ITEM_MANAGER::instance().GetTargetWindow(item);
			empty_pos = victim->GetEmptySlotInWindow(bTargetWindow, item->GetSize());
		}

		if (empty_pos == -1)
		{
			sys_err("Exchange::Done: Cannot find blank position in inventory %s <-> %s item %s", m_pOwner->GetName(), victim->GetName(), item->GetName());
			continue;
		}

		assert(empty_pos >= 0);

		m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);
		item->RemoveFromCharacter();

#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
			item->AddToCharacter(victim, TItemPos(DRAGON_SOUL_INVENTORY, empty_pos));
		else
#endif
			item->AddToCharacter(victim, TItemPos(bTargetWindow, empty_pos));

		ITEM_MANAGER::instance().FlushDelayedSave(item);

		item->SetExchanging(false);
		{
			char exchange_buf[ 51 ];

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), GetOwner()->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(victim, item, "EXCHANGE_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%s %u %u", item->GetName(), victim->GetPlayerID(), item->GetCount());
			LogManager::instance().ItemLog(GetOwner(), item, "EXCHANGE_GIVE", exchange_buf);
		}

		// Suspect trade log
		ull_golditems += ( long long ) item->GetGold() * ( long long ) item->GetCount();

		if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007 || item->GetVnum() == 94355)
			dwGoldbars += item->GetCount();
	}

	// Gold part
	if (m_llGold)
	{
		GetOwner()->PointChange(POINT_GOLD, -m_llGold, true);
		victim->PointChange(POINT_GOLD, m_llGold, true);

		if (m_llGold > 1000)
		{
			char exchange_buf[51];
			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s %lld", GetOwner()->GetPlayerID(), GetOwner()->GetName(), m_llGold);
			LogManager::instance().CharLog(victim, m_llGold, "EXCHANGE_GOLD_TAKE", exchange_buf);

			snprintf(exchange_buf, sizeof(exchange_buf), "%u %s %lld", victim->GetPlayerID(), victim->GetName(), m_llGold);
			LogManager::instance().CharLog(GetOwner(), m_llGold, "EXCHANGE_GOLD_GIVE", exchange_buf);
		}
	}
	
	long long limit = 2000000000;
#ifdef ELONIA
	limit = 200000000;
#endif

	if ((long long) m_llGold + (long long) ull_golditems >= limit) // 2kkk
		LogManager::instance().SuspectTradeLog(GetOwner(), victim, (long long) m_llGold + (long long) ull_golditems, dwGoldbars);

	m_pGrid->Clear();
	return true;
}

// ±³È¯À» µ¿ÀÇ
bool CExchange::Accept(bool bAccept)
{
	if (m_bAccept == bAccept)
		return true;

	m_bAccept = bAccept;

	// µÑ ´Ù µ¿ÀÇ ÇßÀ¸¹Ç·Î ±³È¯ ¼º¸³
	if (m_bAccept && GetCompany()->m_bAccept)
	{
		int	iItemCount;

		LPCHARACTER victim = GetCompany()->GetOwner();

		//PREVENT_PORTAL_AFTER_EXCHANGE
		GetOwner()->SetExchangeTime();
		victim->SetExchangeTime();		
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

		// exchange_check ¿¡¼­´Â ±³È¯ÇÒ ¾ÆÀÌÅÛµéÀÌ Á¦ÀÚ¸®¿¡ ÀÖ³ª È®ÀÎÇÏ°í,
		// ¿¤Å©µµ ÃæºÐÈ÷ ÀÖ³ª È®ÀÎÇÑ´Ù, µÎ¹øÂ° ÀÎÀÚ·Î ±³È¯ÇÒ ¾ÆÀÌÅÛ °³¼ö
		// ¸¦ ¸®ÅÏÇÑ´Ù.
		if (!Check(&iItemCount))
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "µ·ÀÌ ºÎÁ·ÇÏ°Å³ª ¾ÆÀÌÅÛÀÌ Á¦ÀÚ¸®¿¡ ¾ø½À´Ï´Ù."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(victim, "»ó´ë¹æÀÇ µ·ÀÌ ºÎÁ·ÇÏ°Å³ª ¾ÆÀÌÅÛÀÌ Á¦ÀÚ¸®¿¡ ¾ø½À´Ï´Ù."));
			goto EXCHANGE_END;
		}

		// ¸®ÅÏ ¹ÞÀº ¾ÆÀÌÅÛ °³¼ö·Î »ó´ë¹æÀÇ ¼ÒÁöÇ°¿¡ ³²Àº ÀÚ¸®°¡ ÀÖ³ª È®ÀÎÇÑ´Ù.
		if (!CheckSpace())
		{
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "»ó´ë¹æÀÇ ¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(victim, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			goto EXCHANGE_END;
		}

		// »ó´ë¹æµµ ¸¶Âù°¡Áö·Î..
		if (!GetCompany()->Check(&iItemCount))
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(victim, "µ·ÀÌ ºÎÁ·ÇÏ°Å³ª ¾ÆÀÌÅÛÀÌ Á¦ÀÚ¸®¿¡ ¾ø½À´Ï´Ù."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "»ó´ë¹æÀÇ µ·ÀÌ ºÎÁ·ÇÏ°Å³ª ¾ÆÀÌÅÛÀÌ Á¦ÀÚ¸®¿¡ ¾ø½À´Ï´Ù."));
			goto EXCHANGE_END;
		}

		if (!GetCompany()->CheckSpace())
		{
			victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(victim, "»ó´ë¹æÀÇ ¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			goto EXCHANGE_END;
		}

		if (db_clientdesc->GetSocket() == INVALID_SOCKET)
		{
			sys_err("Cannot use exchange feature while DB cache connection is dead.");
			victim->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Unknown error");
			goto EXCHANGE_END;
		}

		if (Done())
		{
			if (m_llGold) // µ·ÀÌ ÀÖÀ» ‹š¸¸ ÀúÀå
				GetOwner()->Save();

			if (GetCompany()->Done())
			{
				if (GetCompany()->m_llGold) // µ·ÀÌ ÀÖÀ» ¶§¸¸ ÀúÀå
					victim->Save();

				// INTERNATIONAL_VERSION
				GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "%s ´Ô°úÀÇ ±³È¯ÀÌ ¼º»ç µÇ¾ú½À´Ï´Ù."), victim->GetName());
				victim->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(victim, "%s ´Ô°úÀÇ ±³È¯ÀÌ ¼º»ç µÇ¾ú½À´Ï´Ù."), GetOwner()->GetName());
				// END_OF_INTERNATIONAL_VERSION
			}
		}

EXCHANGE_END:
		Cancel();
		return false;
	}
	else
	{
		network::GCOutputPacket<network::GCExchangeAcceptPacket> pack;
		pack->set_accept(m_bAccept);

		pack->set_is_me(true);
		GetOwner()->GetDesc()->Packet(pack);

		pack->set_is_me(false);
		GetCompany()->GetOwner()->GetDesc()->Packet(pack);
		return true;
	}
}

// ±³È¯ Ãë¼Ò
void CExchange::Cancel()
{
	GetOwner()->GetDesc()->Packet(network::TGCHeader::EXCHANGE_CANCEL);
	GetOwner()->SetExchange(NULL);

	for (int i = 0; i < EXCHANGE_ITEM_MAX_NUM; ++i)
	{
		if (m_apItems[i])
			m_apItems[i]->SetExchanging(false);
	}

	if (GetCompany())
	{
		GetCompany()->SetCompany(NULL);
		GetCompany()->Cancel();
	}

	M2_DELETE_ONLY(this);
}

