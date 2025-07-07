#ifndef __INC_METIN_II_GAME_SHOP_H__
#define __INC_METIN_II_GAME_SHOP_H__

#include "char.h"
#include "desc.h"

#include "protobuf_data.h"
#include "headers.hpp"
#include <google/protobuf/repeated_field.h>

enum
{
	SHOP_MAX_DISTANCE = 1000
};

class CGrid;

/* ---------------------------------------------------------------------------------- */
class CShop
{
	public:
		typedef struct shop_item
		{
			DWORD	vnum;		// 아이템 번호
			long	price;		// 가격
			DWORD	price_vnum;
#ifdef INCREASE_ITEM_STACK
			WORD	count;		// 아이템 개수
#else
			BYTE	count;		// 아이템 개수
#endif

			LPITEM	pkItem;
			DWORD	itemid;		// 아이템 고유아이디

#ifdef SECOND_ITEM_PRICE
			DWORD	price_vnum2;
			WORD	price2;
#endif

			shop_item()
			{
				vnum = 0;
				price = 0;
				price_vnum = 0;
				count = 0;
				itemid = 0;
				pkItem = NULL;
#ifdef SECOND_ITEM_PRICE
				price_vnum2 = 0;
				price2 = 0;
#endif
			}
		} SHOP_ITEM;

		CShop();
		~CShop();

		void	Close(DWORD dwExceptionPID = 0);
		
		bool	Create(DWORD dwVnum, DWORD dwNPCVnum, const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& items);
		void	SetShopItems(const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& items, BYTE bItemCount);

		virtual void	SetPCShop(LPCHARACTER ch);
		virtual bool	IsPCShop()	{ return m_pkPC ? true : false; }

		// 게스트 추가/삭제
		virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
		void	RemoveGuest(LPCHARACTER ch);
		void	RemoveAllGuests();

		// 물건 구입
		virtual network::TGCHeader Buy(LPCHARACTER ch, BYTE pos);

		// 게스트에게 패킷을 보냄
		void	BroadcastUpdateItem(BYTE pos);

		// 판매중인 아이템의 갯수를 알려준다.
		int		GetNumberByVnum(DWORD dwVnum);

		// 아이템이 상점에 등록되어 있는지 알려준다.
		virtual bool	IsSellingItem(DWORD itemID);

		DWORD	GetVnum() { return m_dwVnum; }
		DWORD	GetNPCVnum() { return m_dwNPCVnum; }

		network::TGCHeader	RemoveFromShop(BYTE pos);
		int		GetPosByItemID(DWORD itemID);
		LPITEM	GetItemAtPos(BYTE bPos);

	protected:
		template <typename T>
		void	Broadcast(network::GCOutputPacket<T>& packet)
		{
			for (auto& it : m_map_guest)
			{
				if (LPDESC desc = it.first->GetDesc())
					desc->Packet(packet);
			}
		}

	protected:
		DWORD				m_dwVnum;
		DWORD				m_dwNPCVnum;

		typedef TR1_NS::unordered_map<LPCHARACTER, bool> GuestMapType;
		GuestMapType m_map_guest;
		std::vector<SHOP_ITEM>		m_itemVector;	// 이 상점에서 취급하는 물건들

		LPCHARACTER			m_pkPC;
};

#endif 
