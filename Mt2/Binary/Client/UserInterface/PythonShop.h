#pragma once

#include "Packet.h"

/*
 *	상점 처리
 *
 *	2003-01-16 anoa	일차 완료
 *	2003-12-26 levites 수정
 *
 *	2012-10-29 rtsummit 새로운 화폐 출현 및 tab 기능 추가로 인한 shop 확장.
 *
 */
typedef enum
{
	SHOP_COIN_TYPE_GOLD, // DEFAULT VALUE
	SHOP_COIN_TYPE_SECONDARY_COIN,
	SHOP_COIN_TYPE_ITEM,
#ifdef COMBAT_ZONE
	SHOP_COIN_TYPE_COMBAT_ZONE,
#endif
} EShopCoinType;

class CPythonShop : public CSingleton<CPythonShop>
{
	public:
		CPythonShop(void);
		virtual ~CPythonShop(void);

		void Clear();

		void SetItemData(DWORD dwIndex, const network::TShopItemTable& c_rShopItemData);
		BOOL GetItemData(DWORD dwIndex, const network::TShopItemTable** c_ppItemData);

		void SetItemData(BYTE tabIdx, DWORD dwSlotPos, const network::TShopItemTable& c_rShopItemData);
		BOOL GetItemData(BYTE tabIdx, DWORD dwSlotPos, const network::TShopItemTable** c_ppItemData);
		
		void SetTabCount(BYTE bTabCount) { m_bTabCount = bTabCount; }
		BYTE GetTabCount() { return m_bTabCount; }

		void SetTabCoinType(BYTE tabIdx, BYTE coinType);
		BYTE GetTabCoinType(BYTE tabIdx);

		void SetTabName(BYTE tabIdx, const char* name);
		const char* GetTabName(BYTE tabIdx);


		//BOOL GetSlotItemID(DWORD dwSlotPos, DWORD* pdwItemID);

		void Open(BOOL isPrivateShop, BOOL isMainPrivateShop);
		void Close();
		BOOL IsOpen();
		BOOL IsPrivateShop();
		BOOL IsMainPlayerPrivateShop();

		void ClearPrivateShopStock();
		void AddPrivateShopItemStock(::TItemPos ItemPos, BYTE byDisplayPos, DWORD dwPrice);
		void DelPrivateShopItemStock(::TItemPos ItemPos);
		int GetPrivateShopItemPrice(::TItemPos ItemPos);
		void BuildPrivateShop(const char * c_szName);

	protected:
		BOOL	CheckSlotIndex(DWORD dwIndex);

	protected:
		BOOL				m_isShoping;
		BOOL				m_isPrivateShop;
		BOOL				m_isMainPlayerPrivateShop;

		struct ShopTab
		{
			ShopTab()
			{
				coinType = SHOP_COIN_TYPE_GOLD;
			}
			BYTE				coinType;
			std::string			name;
			network::TShopItemTable	items[SHOP_HOST_ITEM_MAX_NUM];
		};
		
		BYTE m_bTabCount;
		ShopTab m_aShoptabs[SHOP_TAB_COUNT_MAX];

		typedef std::map<::TItemPos, network::TShopItemTable> TPrivateShopItemStock;
		TPrivateShopItemStock	m_PrivateShopItemStock;

#ifdef ENABLE_GAYA_SYSTEM
	public:
		void	SetGayaShopData(const google::protobuf::RepeatedPtrField<network::TGayaShopData>& shop_data) {
			for (int i = 0; i < GAYA_SHOP_MAX_NUM; ++i)
			{
				if (i < shop_data.size())
					m_akGayaShopData[i] = shop_data[i];
				else
					m_akGayaShopData[i].Clear();
			}
		}
		const network::TGayaShopData*	GetGayaShopData(BYTE bIndex) const { return &m_akGayaShopData[bIndex]; }

	private:
		network::TGayaShopData	m_akGayaShopData[GAYA_SHOP_MAX_NUM];
#endif
};
