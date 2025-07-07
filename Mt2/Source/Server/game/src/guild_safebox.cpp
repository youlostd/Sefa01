#include "stdafx.h"

#ifdef __GUILD_SAFEBOX__
#include "../../common/tables.h"
#include "guild_safebox.h"
#include "guild.h"
#include "db.h"
#include "desc_client.h"
#include "item.h"
#include "char.h"
#include "item_manager.h"
#include "desc.h"
#include "config.h"
#include "buffer_manager.h"
#include "log.h"
#include "utils.h"

struct FGuildSafeboxSendEnableInformation
{
	FGuildSafeboxSendEnableInformation(CGuildSafeBox* pSafeBox) : m_pSafeBox(pSafeBox) {}
	void operator()(LPCHARACTER pkChr)
	{
		if (test_server)
			sys_log(0, "SendGuildSafeboxEnableInfo to %u %s", pkChr->GetPlayerID(), pkChr->GetName());
		m_pSafeBox->SendEnableInformation(pkChr);
	}

	CGuildSafeBox* m_pSafeBox;
};

/********************************\
** PUBLIC LOADING
\********************************/

CGuildSafeBox::CGuildSafeBox(CGuild* pOwnerGuild) : m_pkOwnerGuild(pOwnerGuild), m_bSize(0), m_ullGold(0), m_bItemLoaded(false)
{
	memset(m_szPassword, 0, sizeof(m_szPassword));
	memset(m_bItemGrid, 0, sizeof(m_bItemGrid));
}

CGuildSafeBox::~CGuildSafeBox()
{
}

void CGuildSafeBox::Load(BYTE bSize, const char* szPassword, ULONGLONG ullGold)
{
	sys_log(0, "LoadGuildSafeBox: %u", m_pkOwnerGuild->GetID());

	m_bSize = bSize;
	strlcpy(m_szPassword, szPassword, sizeof(m_szPassword));
	m_ullGold = ullGold;
}

void CGuildSafeBox::LoadItem(const ::google::protobuf::RepeatedPtrField<network::TItemData>& items)
{
	if (m_bItemLoaded)
		return;

	for (auto& item : m_pkItems)
		item.reset();

	memset(m_bItemGrid, 0, sizeof(m_bItemGrid));
	for (auto& item : items)
	{
		auto pProto = ITEM_MANAGER::instance().GetTable(item.vnum());
		if (!pProto)
		{
			sys_err("cannot load guild item %u vnum %u for guild %u", item.id(), item.vnum(), m_pkOwnerGuild->GetID());
			continue;
		}

		if (item.cell().cell() + GUILD_SAFEBOX_ITEM_WIDTH * (pProto->size() - 1) >= GUILD_SAFEBOX_MAX_NUM)
		{
			sys_err("cannot lod guild item %u vnum %u for guild %u (out of position %u)",
				item.id(), item.vnum(), m_pkOwnerGuild->GetID(), item.cell().cell());
			continue;
		}

		m_pkItems[item.cell().cell()].reset(new network::TItemData(item));
		for (int iSize = 0; iSize < pProto->size(); ++iSize)
			m_bItemGrid[item.cell().cell() + GUILD_SAFEBOX_ITEM_WIDTH * iSize] = 1;
	}

	m_bItemLoaded = true;
}

/********************************\
** PUBLIC CHECK
\********************************/

bool CGuildSafeBox::IsValidPosition(WORD wPos) const
{
	if (!m_bSize)
		return false;

	if (wPos >= GUILD_SAFEBOX_MAX_NUM)
		return false;

	return true;
}

bool CGuildSafeBox::IsEmpty(WORD wPos, BYTE bSize) const
{
	if (!m_bSize)
		return false;

	if (wPos + 5 * (bSize - 1) >= GUILD_SAFEBOX_MAX_NUM)
		return false;

	for (int i = wPos; i < wPos + 5 * bSize; i += 5)
	{
		if (m_bItemGrid[i])
			return false;
	}

	return true;
}

bool CGuildSafeBox::CanAddItem(LPITEM pkItem, WORD wPos) const
{
	if (!IsValidPosition(wPos))
		return false;

	if (!IsEmpty(wPos, pkItem->GetSize()))
		return false;

	return true;
}

/********************************\
** PUBLIC GENERAL
\********************************/

void CGuildSafeBox::GiveSafebox(LPCHARACTER pkChr, BYTE bSize)
{
	if (HasSafebox())
		return;

	m_bSize = bSize;
	m_ullGold = 0;

	network::GDOutputPacket<network::GDGuildSafeboxCreatePacket> pdg;
	pdg->set_guild_id(m_pkOwnerGuild->GetID());
	pdg->set_pid(pkChr->GetPlayerID());
	pdg->set_name(pkChr->GetName());
	pdg->set_size(m_bSize);
	db_clientdesc->DBPacket(pdg);

	FGuildSafeboxSendEnableInformation f(this);
	if (test_server)
		sys_log(0, "SendEnableInfo to guild %p %u", m_pkOwnerGuild, m_pkOwnerGuild ? m_pkOwnerGuild->GetID() : 0);
	m_pkOwnerGuild->ForEachOnlineMember(f);
}

void CGuildSafeBox::ChangeSafeboxSize(LPCHARACTER pkChr, BYTE bNewSize)
{
	m_bSize = bNewSize;

	network::GDOutputPacket<network::GDGuildSafeboxSizePacket> pgd;
	pgd->set_guild_id(m_pkOwnerGuild->GetID());
	pgd->set_pid(pkChr->GetPlayerID());
	pgd->set_name(pkChr->GetName());
	pgd->set_size(m_bSize);
	db_clientdesc->DBPacket(pgd);
}

void CGuildSafeBox::OpenSafebox(LPCHARACTER ch)
{
	if (!HasSafebox())
		return;

#ifdef ACCOUNT_TRADE_BLOCK
	if (ch->GetDesc()->IsTradeblocked())
		return;
#endif

	if (ch->GetGuild() != m_pkOwnerGuild)
	{
		sys_err("cannot open guild safebox %d for player %d %s", m_pkOwnerGuild->GetID(), ch->GetPlayerID(), ch->GetName());
		return;
	}

	if (!m_bItemLoaded)
	{
		if (test_server)
			sys_log(0, "CGuildSafeBox::OpenSafebox: Request loading from DB");
		network::GDOutputPacket<network::GDGuildSafeboxLoadPacket> pgd;
		pgd->set_guild_id(m_pkOwnerGuild->GetID());
		db_clientdesc->DBPacket(pgd, ch->GetDesc()->GetHandle());
		return;
	}

	__AddViewer(ch);

	network::GCOutputPacket<network::GCGuildSafeboxOpenPacket> pack_open;
	pack_open->set_size(m_bSize);
	ch->GetDesc()->Packet(pack_open);

	network::GCOutputPacket<network::GCGuildSafeboxGoldPacket> pack_gold;
	pack_gold->set_gold(m_ullGold);
	ch->GetDesc()->Packet(pack_gold);

	for (int i = 0; i < GUILD_SAFEBOX_MAX_NUM; ++i)
	{
		if (auto pkItem = __GetItem(i))
		{
			network::GCOutputPacket<network::GCItemSetPacket> pack_item;
			*pack_item->mutable_data() = *pkItem;
			pack_item->mutable_data()->mutable_cell()->set_window_type(GUILD_SAFEBOX);

			ch->GetDesc()->Packet(pack_item);
		}
	}
}

void CGuildSafeBox::CheckInItem(LPCHARACTER ch, LPITEM pkItem, int iDestCell)
{
	if (!__IsViewer(ch))
	{
		sys_err("no viewer character (%d %s) try to check in item %d %s pos %d",
			ch->GetPlayerID(), ch->GetName(), pkItem->GetID(), pkItem->GetName(), iDestCell);
		return;
	}

	TGuildMember* pMember = ch->GetGuild()->GetMember(ch->GetPlayerID());
	if (!pMember || !ch->GetGuild()->HasGradeAuth(pMember->grade, GUILD_AUTH_SAFEBOX_ITEM_GIVE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Guild> You are not allowed to do that.");
		return;
	}

	if (!ch->CanHandleItem())
		return;

	if (pkItem->IsExchanging())
		return;

	if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX | ITEM_ANTIFLAG_GIVE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù. 111"));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù.222"));
		return;
	}

	if (pkItem->IsGMOwner() && !test_server)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù.333"));
		return;
	}

#ifdef __TRADE_BLOCK_SYSTEM__
	if (ch->IsTradeBlocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You can't do this now. Please write COMA/GA/DEV/SA a message on Discord or Forum.");
		ch->ChatPacket(CHAT_TYPE_INFO, "This is a security mesurement against payment fraud. Don't worry it will resolve shortly.");
	}
#endif

	if ((pkItem->GetWindow() != INVENTORY && pkItem->GetWindow() != ITEM_MANAGER::instance().GetTargetWindow(pkItem)) || pkItem->IsExchanging() || pkItem->isLocked())
	{
		sys_err("wrong item selected");
		return;
	}

	if (!CanAddItem(pkItem, iDestCell))
	{
		sys_err("cannot add item to %d", iDestCell);
		return;
	}

	// if (pkItem->IsCooltime())
	// {
		// ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to wait %d Minutes until you can move the recently received item."), 8);
		// return;
	// }
	
	char szHintBuffer[ 100 ];
	sprintf(szHintBuffer, "GNAME: %s GID: %d", m_pkOwnerGuild->GetName(), m_pkOwnerGuild->GetID());

	LogManager::instance().CharLog(ch, pkItem->GetVnum(), "GUILD_SAFEBOX_CHECK_IN", szHintBuffer);

	if (test_server)
		sys_log(0, "CGuildSafeBox::CheckInItem %u: source %u destination %u", m_pkOwnerGuild->GetID(), pkItem->GetCell(), iDestCell);

	network::GDOutputPacket<network::GDGuildSafeboxAddPacket> pack_item;
	pack_item->set_pid(ch->GetPlayerID());
	pack_item->set_name(ch->GetName());

	auto item = pack_item->mutable_item();
	ITEM_MANAGER::instance().GetPlayerItem(pkItem, item);

	item->set_owner(m_pkOwnerGuild->GetID());
	*item->mutable_cell() = TItemPos(GUILD_SAFEBOX, iDestCell);

	db_clientdesc->DBPacket(pack_item, ch->GetDesc()->GetHandle());

	// dont send destroy packet to db (guild safebox will handle this itself)
	pkItem->SetSkipSave(true);
	ITEM_MANAGER::instance().RemoveItem(pkItem, "GUILD_SAFEBOX_CHECK_IN");
	}

void CGuildSafeBox::CheckOutItem(LPCHARACTER ch, int iSourcePos, BYTE bTargetWindow, int iTargetPos)
{
	ch->tchat("%s:%d %s", __FILE__, __LINE__, __FUNCTION__);
	if (!__IsViewer(ch))
	{
		sys_err("no viewer character (%d %s) try to check out item from pos %d",
			ch->GetPlayerID(), ch->GetName(), iSourcePos);
		return;
	}

	TGuildMember* pMember = ch->GetGuild()->GetMember(ch->GetPlayerID());
	if (!pMember || !ch->GetGuild()->HasGradeAuth(pMember->grade, GUILD_AUTH_SAFEBOX_ITEM_TAKE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Guild> You are not allowed to do that.");
		return;
	}

	if (!IsValidPosition(iSourcePos))
	{
		sys_err("invalid cell %u", iSourcePos);
		return;
	}

	if (IsEmpty(iSourcePos, 1))
	{
		sys_err("empty pos %u", iSourcePos);
		return;
	}

	auto pTargetItem = __GetItem(iSourcePos);
	if (!pTargetItem)
	{
		sys_err("invalid target item pos %u", iSourcePos);
		return;
	}

	TItemPos kDestPos(bTargetWindow, iTargetPos);

	BYTE bExpectWindow = ITEM_MANAGER::instance().GetTargetWindow(pTargetItem->vnum());
	if ((bTargetWindow != bExpectWindow && bTargetWindow != INVENTORY) || 
		!kDestPos.IsValidItemPosition())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't put this item directly to this window."));
		ch->tchat("invalid target pos %u or targetWindow %u (expected window %u)", iTargetPos, bTargetWindow, bExpectWindow);
		return;
	}

	char szHintBuffer[ 100 ];
	sprintf(szHintBuffer, "GNAME: %s GID: %d", m_pkOwnerGuild->GetName(), m_pkOwnerGuild->GetID());

	LogManager::instance().CharLog(ch, pTargetItem->vnum(), "GUILD_SAFEBOX_CHECK_OUT", szHintBuffer);

	if (test_server)
		sys_log(0, "CGuildSafeBox::CheckOutItem %u: source %u destination %u", m_pkOwnerGuild->GetID(), iSourcePos, iTargetPos);

	network::GDOutputPacket<network::GDGuildSafeboxTakePacket> packet;
	packet->set_guild_id(m_pkOwnerGuild->GetID());
	packet->set_pid(ch->GetPlayerID());
	packet->set_player_name(ch->GetName());
	packet->set_source_pos(iSourcePos);
	*packet->mutable_target_pos() = TItemPos(bTargetWindow, iTargetPos);

	db_clientdesc->DBPacket(packet, ch->GetDesc()->GetHandle());
}

#ifdef INCREASE_ITEM_STACK
void CGuildSafeBox::MoveItem(LPCHARACTER ch, int iSourcePos, int iTargetPos, WORD bCount)
#else
void CGuildSafeBox::MoveItem(LPCHARACTER ch, int iSourcePos, int iTargetPos, BYTE bCount)
#endif
{
	if (!__IsViewer(ch))
	{
		sys_err("no viewer character (%d %s) try to move item from pos %d to pos %d count %d",
			ch->GetPlayerID(), ch->GetName(), iSourcePos, iTargetPos, bCount);
		return;
	}

	TGuildMember* pMember = ch->GetGuild()->GetMember(ch->GetPlayerID());
	if (!pMember || !ch->GetGuild()->HasGradeAuth(pMember->grade, GUILD_AUTH_SAFEBOX_ITEM_GIVE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Guild> You are not allowed to do that.");
		return;
	}

	if (!IsValidPosition(iSourcePos) || !IsValidPosition(iTargetPos))
	{
		sys_err("source or target not valid (src %u dst %u)", iSourcePos, iTargetPos);
		return;
	}

	if (iSourcePos < 0 || iSourcePos >= GUILD_SAFEBOX_MAX_NUM || iTargetPos < 0 || iTargetPos >= GUILD_SAFEBOX_MAX_NUM)
		return;

	if (IsEmpty(iSourcePos, 1))
	{
		sys_err("cannot move none item (cell %u)", iSourcePos);
		return;
	}

	if (test_server)
		sys_log(0, "CGuildSafeBox::MoveItem %u: source %u destination %u", m_pkOwnerGuild->GetID(), iSourcePos, iTargetPos);

	network::GDOutputPacket<network::GDGuildSafeboxMovePacket> packet;
	packet->set_guild_id(m_pkOwnerGuild->GetID());
	packet->set_source_slot(iSourcePos);
	packet->set_target_slot(iTargetPos);

	db_clientdesc->DBPacket(packet);
}

void CGuildSafeBox::GiveGold(LPCHARACTER ch, ULONGLONG ullGold)
{
	if (!__IsViewer(ch))
	{
		sys_err("no viewer character (%d %s) try to give gold %llu",
			ch->GetPlayerID(), ch->GetName(), ullGold);
		return;
	}

	TGuildMember* pMember = ch->GetGuild()->GetMember(ch->GetPlayerID());
	if (!pMember || !ch->GetGuild()->HasGradeAuth(pMember->grade, GUILD_AUTH_SAFEBOX_GOLD_GIVE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Guild> You are not allowed to do that.");
		return;
	}

	if (m_ullGold + ullGold >= GUILD_SAFEBOX_GOLD_MAX)
	{
		if (m_ullGold >= GUILD_SAFEBOX_GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot safe any more gold."));
			return;
		}

		ullGold = GUILD_SAFEBOX_GOLD_MAX - m_ullGold;
	}

	if (ch->GetGold() < ullGold)
		ullGold = ch->GetGold();

	if (!ullGold)
		return;

	if (test_server)
		sys_log(0, "CGuildSafeBox::GiveGold %u: %llu", m_pkOwnerGuild->GetID(), ullGold);

	char szHintBuffer[ 100 ];
	sprintf(szHintBuffer, "GNAME: %s GID: %d", m_pkOwnerGuild->GetName(), m_pkOwnerGuild->GetID());

	LogManager::instance().CharLog(ch, ullGold, "GUILD_SAFEBOX_GOLD_GIVE", szHintBuffer);

	ch->PointChange(POINT_GOLD, -(long long)ullGold);

	network::GDOutputPacket<network::GDGuildSafeboxGiveGoldPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	pack->set_name(ch->GetName());
	pack->set_guild_id(m_pkOwnerGuild->GetID());
	pack->set_gold(ullGold);

	db_clientdesc->DBPacket(pack);
}

void CGuildSafeBox::TakeGold(LPCHARACTER ch, ULONGLONG ullGold)
{
	if (!__IsViewer(ch))
	{
		sys_err("no viewer character (%d %s) try to give gold %llu",
			ch->GetPlayerID(), ch->GetName(), ullGold);
		return;
	}

	TGuildMember* pMember = ch->GetGuild()->GetMember(ch->GetPlayerID());
	if (!pMember || !ch->GetGuild()->HasGradeAuth(pMember->grade, GUILD_AUTH_SAFEBOX_GOLD_TAKE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "<Guild> You are not allowed to do that.");
		return;
	}

	if (m_ullGold < ullGold)
		ullGold = m_ullGold;

	if (ullGold && ch->GetGold() + ullGold > GOLD_MAX)
	{
		if (ch->GetGold() >= GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot take any more gold."));
			return;
		}

		ullGold = GOLD_MAX - ch->GetGold();
	}

	if (!ullGold)
		return;

	char szHintBuffer[100];
	sprintf(szHintBuffer, "GNAME: %s GID: %d", m_pkOwnerGuild->GetName(), m_pkOwnerGuild->GetID());

	LogManager::instance().CharLog(ch, ullGold, "GUILD_SAFEBOX_GOLD_TAKE", szHintBuffer);

	if (test_server)
		sys_log(0, "CGuildSafeBox::TakeGold %u: %llu", m_pkOwnerGuild->GetID(), ullGold);

	network::GDOutputPacket<network::GDGuildSafeboxGetGoldPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	pack->set_name(ch->GetName());
	pack->set_guild_id(m_pkOwnerGuild->GetID());
	pack->set_gold(ullGold);

	db_clientdesc->DBPacket(pack, ch->GetDesc()->GetHandle());
}

void CGuildSafeBox::CloseSafebox(LPCHARACTER ch)
{
	if (!__IsViewer(ch))
		return;

	__RemoveViewer(ch);
	__RemoveLogViewer(ch);

	ch->GetDesc()->Packet(network::TGCHeader::GUILD_SAFEBOX_CLOSE);
}

void CGuildSafeBox::DB_SetItem(const network::TItemData* pkItem)
{
	auto pProto = ITEM_MANAGER::Instance().GetTable(pkItem->vnum());
	if (!pProto)
	{
		sys_err("cannot get proto of vnum %u guild id %u", pkItem->vnum(), m_pkOwnerGuild->GetID());
		return;
	}

	m_pkItems[pkItem->cell().cell()].reset(new network::TItemData(*pkItem));
	for (int i = pkItem->cell().cell(); i < pkItem->cell().cell() + GUILD_SAFEBOX_ITEM_WIDTH * pProto->size(); i += GUILD_SAFEBOX_ITEM_WIDTH)
		m_bItemGrid[i] = 1;

	network::GCOutputPacket<network::GCItemSetPacket> pack;
	*pack->mutable_data() = *pkItem;
	pack->mutable_data()->mutable_cell()->set_window_type(GUILD_SAFEBOX);
	__ViewerPacket(pack);
}

void CGuildSafeBox::DB_DelItem(BYTE bSlot)
{
	if (!m_pkItems[bSlot])
	{
		sys_err("DB_DelItem: no item on slot %u", bSlot);
		return;
	}

	auto pProto = ITEM_MANAGER::Instance().GetTable(m_pkItems[bSlot]->vnum());
	if (!pProto)
	{
		sys_err("cannot get proto of vnum %u guild id %u", m_pkItems[bSlot]->vnum(), m_pkOwnerGuild->GetID());
		return;
	}

	m_pkItems[bSlot].reset();
	for (int i = bSlot; i < bSlot + GUILD_SAFEBOX_ITEM_WIDTH * pProto->size(); i += GUILD_SAFEBOX_ITEM_WIDTH)
		m_bItemGrid[i] = 0;

	network::GCOutputPacket<network::GCItemSetPacket> pack;
	*pack->mutable_data()->mutable_cell() = TItemPos(GUILD_SAFEBOX, bSlot);
	__ViewerPacket(pack);
}

void CGuildSafeBox::DB_SetGold(ULONGLONG ullGold)
{
	m_ullGold = ullGold;

	network::GCOutputPacket<network::GCGuildSafeboxGoldPacket> pack;
	pack->set_gold(m_ullGold);
	__ViewerPacket(pack);
}

void CGuildSafeBox::DB_SetOwned(BYTE bSize)
{
	m_bSize = bSize;

	FGuildSafeboxSendEnableInformation f(this);
	m_pkOwnerGuild->ForEachOnlineMember(f);
}

void CGuildSafeBox::SendEnableInformation(LPCHARACTER pkChr)
{
	bool bHasSafebox = HasSafebox() && pkChr->GetGuild() == m_pkOwnerGuild;
	pkChr->GetDesc()->Packet(bHasSafebox ? network::TGCHeader::GUILD_SAFEBOX_ENABLE : network::TGCHeader::GUILD_SAFEBOX_DISABLE);
}

void CGuildSafeBox::OpenLog(LPCHARACTER ch)
{
	if (!__IsViewer(ch))
	{
		sys_err("cannot open guild safebox for %u %s (no viewer)", ch->GetPlayerID(), ch->GetName());
		return;
	}

	if (__IsLogViewer(ch))
	{
		sys_log(0, "CGuildSafeBox::OpenLog: %u %s already log viewer", ch->GetPlayerID(), ch->GetName());
		return;
	}

	if (test_server)
		sys_log(0, "CGuildSafeBox::OpenLog %u %s size %u", ch->GetPlayerID(), ch->GetName(), m_vec_GuildSafeboxLog.size());

	__AddLogViewer(ch);

	ch->GetDesc()->Packet(network::TGCHeader::GUILD_SAFEBOX_LOAD_LOG_START);

	int iCurIndex = 0;
	int iSendCount = m_vec_GuildSafeboxLog.size();
	while (iSendCount > 0)
	{
		network::GCOutputPacket<network::GCGuildSafeboxLoadLogPacket> pack;

		BYTE bRealSendCount = iSendCount > GUILD_SAFEBOX_LOG_SEND_COUNT ? GUILD_SAFEBOX_LOG_SEND_COUNT : iSendCount;

		for (int i = 0; i < bRealSendCount; ++i)
		{
			auto log = pack->add_logs();
			*log = m_vec_GuildSafeboxLog[iCurIndex];
			log->set_time(get_global_time() - log->time());

			++iCurIndex;
		}

		ch->GetDesc()->Packet(pack);

		iSendCount -= GUILD_SAFEBOX_LOG_SEND_COUNT;
	}

	ch->GetDesc()->Packet(network::TGCHeader::GUILD_SAFEBOX_LOAD_LOG_DONE);
}

void CGuildSafeBox::AppendLog(const network::TGuildSafeboxLogTable* pLogTable)
{
	m_vec_GuildSafeboxLog.push_back(*pLogTable);
	if (m_vec_GuildSafeboxLog.size() > GUILD_SAFEBOX_LOG_MAX_COUNT)
		m_vec_GuildSafeboxLog.erase(m_vec_GuildSafeboxLog.begin());

	network::GCOutputPacket<network::GCGuildSafeboxAppendLogPacket> pack;
	*pack->mutable_log() = *pLogTable;
	pack->mutable_log()->set_time(get_global_time() - pack->log().time());

	__ViewerLogPacket(pack);
}

/********************************\
** PRIVATE SET/GET
\********************************/

network::TItemData* CGuildSafeBox::__GetItem(WORD wPos)
{
	if (wPos >= GUILD_SAFEBOX_MAX_NUM)
		return NULL;
	return m_pkItems[wPos].get();
}

/********************************\
** PRIVATE VIEWER
\********************************/

bool CGuildSafeBox::__IsViewer(LPCHARACTER ch)
{
	return m_set_pkCurrentViewer.find(ch) != m_set_pkCurrentViewer.end();
}
void CGuildSafeBox::__AddViewer(LPCHARACTER ch)
{
	if (!__IsViewer(ch))
		m_set_pkCurrentViewer.insert(ch);
}
void CGuildSafeBox::__RemoveViewer(LPCHARACTER ch)
{
	m_set_pkCurrentViewer.erase(ch);
}

bool CGuildSafeBox::__IsLogViewer(LPCHARACTER ch)
{
	return m_set_pkCurrentLogViewer.find(ch) != m_set_pkCurrentLogViewer.end();
}
void CGuildSafeBox::__AddLogViewer(LPCHARACTER ch)
{
	if (!__IsLogViewer(ch))
		m_set_pkCurrentLogViewer.insert(ch);
}
void CGuildSafeBox::__RemoveLogViewer(LPCHARACTER ch)
{
	m_set_pkCurrentLogViewer.erase(ch);
}
#endif
