#include "stdafx.h"
#include "general_manager.h"

CGeneralManager::CGeneralManager()
{
#ifdef __GAYA_SYSTEM__
	for (int i = 0; i < GAYA_SHOP_MAX_NUM; ++i)
		m_akGayaShop[i].set_pos(i);
#endif
}

CGeneralManager::~CGeneralManager()
{
}

#ifdef __GAYA_SYSTEM__
void CGeneralManager::InitializeGayaShop(const ::google::protobuf::RepeatedPtrField<network::TGayaShopData>& table)
{
	for (auto& t : table)
	{
		sys_log(0, "GayaShop: Load %d [vnum %u count %u price %u]", t.pos(), t.vnum(), t.count(), t.price());
		m_akGayaShop[t.pos()] = t;
	}
}
#endif

#ifdef CRYSTAL_SYSTEM
void CGeneralManager::initialize_crystal_proto(const ::google::protobuf::RepeatedPtrField<network::TCrystalProto>& table)
{
	_crystal_proto.clear();
	_crystal_proto_by_clarity.clear();

	for (auto& t : table)
	{
		auto proto = std::make_shared<network::TCrystalProto>(t);

		_crystal_proto[t.process_level()] = proto;
		_crystal_proto_by_clarity[get_clarity_proto_key(t.clarity_type(), t.clarity_level())] = proto;
	}
}

const std::shared_ptr<network::TCrystalProto>& CGeneralManager::get_crystal_proto(BYTE clarity_type, BYTE clarity_level, bool next) const
{
	auto it = _crystal_proto_by_clarity.find(get_clarity_proto_key(clarity_type, clarity_level));
	if (it == _crystal_proto_by_clarity.end())
		return nullptr;

	if (next)
	{
		auto it_next = _crystal_proto.find(it->second->process_level() + 1);
		if (it_next == _crystal_proto.end())
			return nullptr;

		return it_next->second;
	}

	return it->second;
}
#endif
