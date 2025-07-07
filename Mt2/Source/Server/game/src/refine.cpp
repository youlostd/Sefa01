#include "stdafx.h"
#include "refine.h"

CRefineManager::CRefineManager()
{
}

CRefineManager::~CRefineManager()
{
}

bool CRefineManager::Initialize(const ::google::protobuf::RepeatedPtrField<network::TRefineTable>& table)
{
	for (auto& t : table)
	{
		sys_log(0, "REFINE %d prob %d cost %lld", t.id(), t.prob(), t.cost());
		m_map_RefineRecipe.insert(std::make_pair(t.id(), t));
	}

	sys_log(0, "REFINE: COUNT %d", m_map_RefineRecipe.size());
	return true;
}

const network::TRefineTable* CRefineManager::GetRefineRecipe(DWORD vnum)
{
	if (vnum == 0)
		return NULL;

	itertype(m_map_RefineRecipe) it = m_map_RefineRecipe.find(vnum);
	sys_log(0, "REFINE: FIND %u %s", vnum, it == m_map_RefineRecipe.end() ? "FALSE" : "TRUE");

	if (it == m_map_RefineRecipe.end())
	{
		return NULL;
	}

	return &it->second;
}
