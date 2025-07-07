#include "stdafx.h"
#include "Peer.h"
#include "ItemIDRangeManager.h"
#include "ClientManager.h"
#include "Cache.h"

CPeer::CPeer()
{
	m_state = 0;
	m_bChannel = 0;
	m_dwHandle = 0;
	m_wListenPort = 0;
	m_wP2PPort = 0;
#ifdef PROCESSOR_CORE
	m_bProcessorCore = false;
#endif

	memset(m_alMaps, 0, sizeof(m_alMaps));

	m_pkVecLogonPID = NULL;
}

CPeer::~CPeer()
{
	Close();
}

void CPeer::OnAccept()
{
	m_state = STATE_PLAYING;

	static DWORD current_handle = 0;
	m_dwHandle = ++current_handle;

	sys_log(0, "Connection accepted. (host: %s handle: %u fd: %d)", m_host, m_dwHandle, m_fd);
}

void CPeer::OnConnect()
{
	sys_log(0, "Connection established. (host: %s handle: %u fd: %d)", m_host, m_dwHandle, m_fd);
	m_state = STATE_PLAYING;
}

void CPeer::OnClose()
{
	m_state = STATE_CLOSE;

	sys_log(0, "Connection closed. (host: %s)", m_host);
	sys_log(0, "ItemIDRange: returned. %u ~ %u [spare %u ~ %u]", m_itemRange.min_id(), m_itemRange.max_id(), m_itemSpareRange.min_id(), m_itemSpareRange.max_id());

	DWORD dwMaxItemID = 0;
	DWORD dwMaxSpareItemID = 0;

	if (m_pkVecLogonPID)
	{
		for (DWORD dwAID : *m_pkVecLogonPID)
		{
			CClientManager::TPlayerCache* pCache = CClientManager::Instance().GetPlayerCache(dwAID, false);
			if (pCache)
			{
				for (CItemCache* pItemCache : pCache->setItems)
				{
					DWORD dwCurItemID = pItemCache->Get(false)->id();
					if (dwCurItemID >= m_itemRange.min_id() && dwCurItemID <= m_itemRange.max_id())
					{
						if (dwCurItemID > dwMaxItemID)
							dwMaxItemID = dwCurItemID;
					}
					else if (dwCurItemID >= m_itemSpareRange.min_id() && dwCurItemID <= m_itemSpareRange.max_id())
					{
						if (dwCurItemID > dwMaxSpareItemID)
							dwMaxSpareItemID = dwCurItemID;
					}
				}
			}
		}

		m_pkVecLogonPID = NULL;

		sys_log(0, "CPeer::OnClose: ITEM_ID_RANGE maxItemID %u maxSpareItemID %u", dwMaxItemID, dwMaxSpareItemID);
	}

	CItemIDRangeManager::Instance().AppendRange(m_itemSpareRange.min_id(), m_itemSpareRange.max_id(), dwMaxSpareItemID, true);
	CItemIDRangeManager::Instance().AppendRange(m_itemRange.min_id(), m_itemRange.max_id(), dwMaxItemID, true);

	m_itemRange.Clear();
	m_itemSpareRange.Clear();
}

DWORD CPeer::GetHandle()
{
	return m_dwHandle;
}

bool CPeer::PeekPacket(int & bytes_received, network::InputPacket & packet, DWORD & handle)
{
	if (GetRecvLength() < bytes_received + sizeof(network::TPacketHeader))
		return false;

	const char * c_pData = ((const char*) GetRecvBuffer()) + bytes_received;

	packet.header = *(network::TPacketHeader*) c_pData;
	if (packet.header.size < sizeof(network::TPacketHeader) + sizeof(uint32_t) || GetRecvLength() < bytes_received + packet.header.size)
		return false;

	c_pData += sizeof(network::TPacketHeader);

	handle = *(uint32_t*) c_pData;
	c_pData += sizeof(uint32_t);

	if (packet.header.header)
	{
		packet.content = c_pData;
		packet.content_size = packet.header.size - sizeof(network::TPacketHeader) - sizeof(uint32_t);
	}

	bytes_received += packet.header.size;
	return true;
}

int CPeer::Send()
{
	if (m_state == STATE_CLOSE)
		return -1;

	return (CPeerBase::Send());
}

void CPeer::SetP2PPort(WORD wPort)
{
	m_wP2PPort = wPort;
}

void CPeer::SetMaps(const long * pl)
{
	thecore_memcpy(m_alMaps, pl, sizeof(m_alMaps));
}

void CPeer::SetMaps(const google::protobuf::RepeatedField<google::protobuf::uint32>& pl)
{
	memset(m_alMaps, 0, sizeof(m_alMaps));
	for (int i = 0; i < MAP_ALLOW_LIMIT && i < pl.size(); ++i)
		m_alMaps[i] = pl[i];
}

void CPeer::SendSpareItemIDRange()
{
	network::DGOutputPacket<network::DGSpareItemIDRangePacket> pack;

	if (m_itemSpareRange.min_id() != 0 && m_itemSpareRange.max_id() != 0 && m_itemSpareRange.usable_item_id_min() != 0)
	{
		SetItemIDRange(m_itemSpareRange);

		if (SetSpareItemIDRange(CItemIDRangeManager::instance().GetRange()) == false)
		{
			sys_log(0, "ItemIDRange: spare range set error");
			m_itemSpareRange.set_min_id(0);
			m_itemSpareRange.set_max_id(0);
			m_itemSpareRange.set_usable_item_id_min(0);
		}
	}

	*pack->mutable_data() = m_itemSpareRange;
	Packet(pack);
}

bool CPeer::SetItemIDRange(network::TItemIDRangeTable itemRange)
{
	if (itemRange.min_id() == 0 || itemRange.max_id() == 0 || itemRange.usable_item_id_min() == 0) return false;

	m_itemRange = itemRange;
	sys_log(0, "ItemIDRange: SET %s %u ~ %u start: %u", GetPublicIP(), m_itemRange.min_id(), m_itemRange.max_id(), m_itemRange.usable_item_id_min());

	return true;
}

bool CPeer::SetSpareItemIDRange(network::TItemIDRangeTable itemRange)
{
	if (itemRange.min_id() == 0 || itemRange.max_id() == 0 || itemRange.usable_item_id_min() == 0) return false;

	m_itemSpareRange = itemRange;
	sys_log(0, "ItemIDRange: SPARE SET %s %u ~ %u start: %u", GetPublicIP(), m_itemSpareRange.min_id(), m_itemSpareRange.max_id(),
			m_itemSpareRange.usable_item_id_min());

	return true;
}

bool CPeer::CheckItemIDRangeCollision(const network::TItemIDRangeTable& itemRange)
{
	if (m_itemRange.min_id() < itemRange.max_id() && m_itemRange.max_id() > itemRange.min_id())
	{
		sys_err("ItemIDRange: Collision!! this %u ~ %u check %u ~ %u",
				m_itemRange.min_id(), m_itemRange.max_id(), itemRange.min_id(), itemRange.max_id());
		return false;
	}

	if (m_itemSpareRange.min_id() < itemRange.max_id() && m_itemSpareRange.max_id() > itemRange.min_id())
	{
		sys_err("ItemIDRange: Collision with spare range this %u ~ %u check %u ~ %u",
				m_itemSpareRange.min_id(), m_itemSpareRange.max_id(), itemRange.min_id(), itemRange.max_id());
		return false;
	}

	return true;
}


