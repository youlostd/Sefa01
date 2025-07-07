#ifndef __HEADER_GUILD_SAFEBOX_MANAGER__
#define __HEADER_GUILD_SAFEBOX_MANAGER__

#include "stdafx.h"

#ifdef __GUILD_SAFEBOX__
#include <unordered_map>
#include <unordered_set>

#include <map>
#include "DBManager.h"
#include "Peer.h"

#include <google/protobuf/repeated_field.h>
#include "headers.hpp"
#include "protobuf_data.h"
#include "protobuf_data_item.h"
#include "protobuf_dg_packets.h"

class CPeer;

typedef struct SGuildHandleInfo
{
	DWORD	dwGuildID;

	SGuildHandleInfo(DWORD dwGuildID) : dwGuildID(dwGuildID)
	{
	}
} GuildHandleInfo;

class CGuildSafebox
{
public:
	CGuildSafebox(DWORD dwGuildID);
	~CGuildSafebox();

	DWORD	GetGuildID() const								{ return m_dwGuildID; }

	void	SetSize(BYTE bSize)								{ m_bSize = bSize; }
	void	ChangeSize(BYTE bNewSize, CPeer* pkPeer = NULL);
	BYTE	GetSize() const									{ return m_bSize; }

	void	SetPassword(const char* szPassword)				{ strlcpy(m_szPassword, szPassword, sizeof(m_szPassword)); }
	const char* GetPassword()								{ return m_szPassword; }
	bool	CheckPassword(const char* szPassword) const		{ return !strcmp(m_szPassword, szPassword); }

	void	SetGold(ULONGLONG ullGold)						{ m_ullGold = ullGold; }
	void	ChangeGold(LONGLONG llChange);
	ULONGLONG	GetGold() const								{ return m_ullGold; }

	void	LoadItems(SQLMsg* pMsg);
	void	DeleteItems();

	bool	IsValidCell(BYTE bCell, BYTE bSize = 1) const;
	bool	IsFree(BYTE bPos, BYTE bSize) const;

	void	RequestAddItem(CPeer* pkPeer, DWORD dwPID, const char* c_pszPlayerName, DWORD dwHandle, const network::TItemData* pItem);
	void	RequestMoveItem(BYTE bSrcSlot, BYTE bTargetSlot);
	void	RequestTakeItem(CPeer* pkPeer, DWORD dwPID, const char* c_pszPlayerName, DWORD dwHandle, BYTE bSlot, BYTE bTargetWindow, WORD wTargetSlot);

	void	GiveItemToPlayer(CPeer* pkPeer, DWORD dwHandle, const network::TItemData* pItem);
	void	SendItemPacket(BYTE bCell);

	void	LoadItems(CPeer* pkPeer, DWORD dwHandle = 0);

	const network::TItemData*	GetItem(BYTE bCell) const { return m_pItems[bCell].get(); }

	void	AddGuildSafeboxLog(BYTE bType, DWORD dwPID, const char* c_pszPlayerName, const network::TItemData* pPlayerItem, LONGLONG ullGold);
	void	AddGuildSafeboxLog(const network::TGuildSafeboxLogTable& rkSafeboxLog);

	void	AddPeer(CPeer* pkPeer);
	void	ErasePeer(CPeer* pkPeer);
	void	ForwardPacket(network::DGOutputPacket<network::DGGuildSafeboxPacket>& pack, CPeer* pkExceptPeer = NULL);

private:
	DWORD			m_dwGuildID;
	BYTE			m_bSize;
	char			m_szPassword[GUILD_SAFEBOX_PASSWORD_MAX_LEN + 1];
	ULONGLONG		m_ullGold;

	std::unique_ptr<network::TItemData>	m_pItems[GUILD_SAFEBOX_MAX_NUM];
	bool			m_bItemGrid[GUILD_SAFEBOX_MAX_NUM];

	std::vector<network::TGuildSafeboxLogTable>	m_vec_GuildSafeboxLog;

	std::unordered_set<CPeer*>	m_set_ForwardPeer;
};

class CGuildSafeboxManager : public singleton<CGuildSafeboxManager>
{
public:
	CGuildSafeboxManager();
	~CGuildSafeboxManager();
	void Initialize();
	void Destroy();

	CGuildSafebox*	GetSafebox(DWORD dwGuildID);
	void			DestroySafebox(DWORD dwGuildID);

	void			SaveItem(network::TItemData* pItem);
	void			FlushItem(network::TItemData* pItem, bool bSave = true);
	DWORD			FlushItems(bool bForce = false, DWORD maxCnt = 0);
	void			SaveSingleItem(network::TItemData* pItem);

	void			SaveSafebox(CGuildSafebox* pSafebox);
	void			FlushSafebox(CGuildSafebox* pSafebox, bool bSave = true);
	DWORD			FlushSafeboxes(bool bForce = false, DWORD maxCnt = 0);
	void			SaveSingleSafebox(CGuildSafebox* pSafebox);

	void			QueryResult(CPeer* pkPeer, SQLMsg* pMsg, int iQIDNum);
	void			ProcessPacket(CPeer* pkPeer, DWORD dwHandle, const network::InputPacket& packet);

	void			InitSafeboxCore(google::protobuf::RepeatedPtrField<network::TGuildSafeboxInitial>* data);
	WORD			GetSafeboxCount() const { return m_map_GuildSafebox.size(); }

	void			Update(DWORD& currCnt, DWORD maxCnt);

	void			DisconnectPeer(CPeer* pkPeer);

private:
	std::map<DWORD, std::unique_ptr<CGuildSafebox>>	m_map_GuildSafebox;
	std::unordered_set<network::TItemData*>		m_map_DelayedItemSave;
	std::unordered_set<CGuildSafebox*>	m_map_DelayedSafeboxSave;

	DWORD									m_dwLastFlushItemTime;
	DWORD									m_dwLastFlushSafeboxTime;
};

#endif // __GUILD_SAFEBOX__
#endif // __HEADER_GUILD_SAFEBOX_MANAGER__
