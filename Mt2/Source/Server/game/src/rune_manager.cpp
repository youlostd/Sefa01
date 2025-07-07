#include "stdafx.h"

#ifdef ENABLE_RUNE_SYSTEM
#include "char.h"
#include "skill.h"
#include "item.h"
#include "party.h"
#include "rune_manager.h"

CRuneManager::CRuneManager()
{
}

CRuneManager::~CRuneManager()
{
}

void CRuneManager::Initialize(const ::google::protobuf::RepeatedPtrField<network::TRuneProtoTable>& table)
{
	if (test_server)
		sys_log(0, "CRuneManager::Initialize");

	for (auto& t : table)
	{
		sys_log(0, "RuneManager::Init: Load Rune %u %s", t.vnum(), t.name().c_str());
		m_map_Proto[t.vnum()] = t;
	}
}

void CRuneManager::Initialize(const ::google::protobuf::RepeatedPtrField<network::TRunePointProtoTable>& table)
{
	if (test_server)
		sys_log(0, "CRuneManager::Initialize points");

	m_map_pointProto.clear();
	for (auto& t : table)
	{
		sys_log(0, "RuneManager::Init: Load Rune Point %u %u", t.point(), t.refine_proto());
		m_map_pointProto[t.point()] = t.refine_proto();
	}
}

DWORD CRuneManager::FindPointProto(WORD point)
{
	auto it = m_map_pointProto.find(point);
	if (it == m_map_pointProto.end())
		return 0;

	return it->second;
}

void CRuneManager::Destroy()
{
}

network::TRuneProtoTable* CRuneManager::GetProto(DWORD dwVnum)
{
	auto it = m_map_Proto.find(dwVnum);
	if (it == m_map_Proto.end())
		return NULL;

	return &it->second;
}

#ifdef ENABLE_LEVEL2_RUNES
DWORD CRuneManager::GetIDByIndex(BYTE bGroup, BYTE bIndex)
{
	for (auto &proto : m_map_Proto)
	{
		if (bGroup == proto.second.group())
		{
			if (bIndex > 0)
			{
				--bIndex;
				continue;
			}

			return proto.second.vnum();
		}
	}

	return 0;
}
#endif
#endif
