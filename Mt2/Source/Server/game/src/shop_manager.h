#ifndef __INC_METIN_II_GAME_SHOP_MANAGER_H__
#define __INC_METIN_II_GAME_SHOP_MANAGER_H__

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

class CShop;
typedef class CShop * LPSHOP;

class CShopManager : public singleton<CShopManager>
{
public:
	typedef std::map<DWORD, CShop *> TShopMap;

public:
	CShopManager();
	virtual ~CShopManager();

	bool	Initialize(const ::google::protobuf::RepeatedPtrField<network::TShopTable>& table);
	void	Destroy();

	LPSHOP	Get(DWORD dwVnum);
	LPSHOP	GetByNPCVnum(DWORD dwVnum);

	bool	StartShopping(LPCHARACTER pkChr, LPCHARACTER pkShopKeeper, int iShopVnum = 0);
	bool	StartShopping(LPCHARACTER pkChr, int iShopVnum = 0);
	void	StopShopping(LPCHARACTER ch);

	void	Buy(LPCHARACTER ch, BYTE pos);
#ifdef INCREASE_ITEM_STACK
	void	Sell(LPCHARACTER ch, WORD wCell, WORD bCount = 0);
#else
	void	Sell(LPCHARACTER ch, WORD wCell, BYTE bCount=0);
#endif

	LPSHOP	CreatePCShop(LPCHARACTER ch, const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& table);
	LPSHOP	FindPCShop(DWORD dwVID);
	void	DestroyPCShop(LPCHARACTER ch);

private:
	TShopMap	m_map_pkShop;
	TShopMap	m_map_pkShopByNPCVnum;
	TShopMap	m_map_pkShopByPC;

	bool	ReadShopTableEx(const char* stFileName);
};

#endif