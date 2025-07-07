#pragma once

#include "stdafx.h"

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

class CGeneralManager : public singleton<CGeneralManager>
{
public:
	CGeneralManager();
	~CGeneralManager();

#ifdef __GAYA_SYSTEM__
	void	InitializeGayaShop(const ::google::protobuf::RepeatedPtrField<network::TGayaShopData>& table);
	const network::TGayaShopData*	GetGayaShop() const { return m_akGayaShop; }
#endif

#ifdef CRYSTAL_SYSTEM
public:
	void	initialize_crystal_proto(const ::google::protobuf::RepeatedPtrField<network::TCrystalProto>& table);
	const std::shared_ptr<network::TCrystalProto>&	get_crystal_proto(BYTE clarity_type, BYTE clarity_level, bool next = false) const;

private:
	WORD	get_clarity_proto_key(BYTE type, BYTE level) const noexcept { return static_cast<WORD>(type) + (static_cast<WORD>(level) << 8); }
#endif

private:
#ifdef __GAYA_SYSTEM__
	network::TGayaShopData	m_akGayaShop[GAYA_SHOP_MAX_NUM];
#endif
#ifdef CRYSTAL_SYSTEM
	std::unordered_map<BYTE, std::shared_ptr<network::TCrystalProto>>	_crystal_proto;
	std::unordered_map<WORD, std::shared_ptr<network::TCrystalProto>>	_crystal_proto_by_clarity;
#endif
};
