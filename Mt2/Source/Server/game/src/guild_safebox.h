#ifndef __HEADER_GUILD_SAFEBOX__
#define __HEADER_GUILD_SAFEBOX__

#include "stdafx.h"

#ifdef __GUILD_SAFEBOX__
#include <google/protobuf/repeated_field.h>
#include "protobuf_data_item.h"
#include "headers.hpp"

#include "char.h"
#include "desc.h"

struct SQLMsg;

class CGuild;

class CGuildSafeBox
{
public:
	CGuildSafeBox(CGuild* pOwnerGuild);
	~CGuildSafeBox();

	BYTE	GetSize() const	{ return m_bSize; }
	ULONGLONG	GetGold() const { return m_ullGold; }

	void	Load(BYTE bSize, const char* szPassword, ULONGLONG ullGold);
	void	LoadItem(const ::google::protobuf::RepeatedPtrField<network::TItemData>& items);

	bool	IsValidPosition(WORD wPos) const;
	bool	IsEmpty(WORD wPos, BYTE bSize) const;
	bool	CanAddItem(LPITEM pkItem, WORD wPos) const;

	bool	HasSafebox() const { return m_bSize > 0; }
	void	GiveSafebox(LPCHARACTER pkChr, BYTE bSize = GUILD_SAFEBOX_DEFAULT_SIZE);
	void	ChangeSafeboxSize(LPCHARACTER pkChr, BYTE bNewSize);
	void	OpenSafebox(LPCHARACTER ch);
	void	CheckInItem(LPCHARACTER ch, LPITEM pkItem, int iDestCell);
	void	CheckOutItem(LPCHARACTER ch, int iSourcePos, BYTE bTargetWindow, int iTargetPos);
#ifdef INCREASE_ITEM_STACK
	void	MoveItem(LPCHARACTER ch, int iSourcePos, int iTargetPos, WORD bCount);
#else
	void	MoveItem(LPCHARACTER ch, int iSourcePos, int iTargetPos, BYTE bCount);
#endif
	void	GiveGold(LPCHARACTER ch, ULONGLONG ullGold);
	void	TakeGold(LPCHARACTER ch, ULONGLONG ullGold);
	void	CloseSafebox(LPCHARACTER ch);

	void	DB_SetItem(const network::TItemData* pkItem);
	void	DB_DelItem(BYTE bSlot);
	void	DB_SetGold(ULONGLONG ullGold);
	void	DB_SetOwned(BYTE bSize);

	void	SendEnableInformation(LPCHARACTER pkChr);

	void	OpenLog(LPCHARACTER ch);
	void	AppendLog(const network::TGuildSafeboxLogTable* pLogTable);

private:
	bool			__AddItem(LPITEM pkItem, WORD wPos, bool bIsLoading = false);
	void			__AddItem(const network::TItemData& rItem, WORD wPos);
	void			__AddItem(DWORD dwID, WORD wPos, DWORD dwVnum, BYTE bCount, BYTE bSpecialLevel,
						const long* alSockets, const network::TItemAttribute* aAttr);

	void			__RemoveItem(LPITEM pkItem);
	network::TItemData*	__GetItem(WORD wPos);

	bool	__IsViewer(LPCHARACTER ch);
	void	__AddViewer(LPCHARACTER ch);
	void	__RemoveViewer(LPCHARACTER ch);
	template <typename T>
	void	__ViewerPacket(network::GCOutputPacket<T>& pack)
	{
		for (LPCHARACTER ch : m_set_pkCurrentViewer)
			ch->GetDesc()->Packet(pack);
	}

	bool	__IsLogViewer(LPCHARACTER ch);
	void	__AddLogViewer(LPCHARACTER ch);
	void	__RemoveLogViewer(LPCHARACTER ch);
	template <typename T>
	void	__ViewerLogPacket(network::GCOutputPacket<T>& pack)
	{
		for (LPCHARACTER ch : m_set_pkCurrentLogViewer)
			ch->GetDesc()->Packet(pack);
	}

private:
	CGuild*					m_pkOwnerGuild;

	BYTE					m_bSize;
	char					m_szPassword[GUILD_SAFEBOX_PASSWORD_MAX_LEN + 1];
	ULONGLONG				m_ullGold;

	bool					m_bItemLoaded;
	std::unique_ptr<network::TItemData>	m_pkItems[GUILD_SAFEBOX_MAX_NUM];
	BYTE					m_bItemGrid[GUILD_SAFEBOX_MAX_NUM];

	std::vector<network::TGuildSafeboxLogTable>	m_vec_GuildSafeboxLog;

	std::set<LPCHARACTER>	m_set_pkCurrentViewer;
	std::set<LPCHARACTER>	m_set_pkCurrentLogViewer;
};
#endif

#endif
