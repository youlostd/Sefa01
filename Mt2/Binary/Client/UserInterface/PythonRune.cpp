#include "StdAfx.h"

#ifdef ENABLE_RUNE_SYSTEM
#include "PythonRune.h"
#include "../EterPack/EterPackManager.h"
#include "../GameLib/ItemManager.h"
#include "PythonPlayer.h"

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPythonRune - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPythonRune::CPythonRune()
{
	Initialize();
	Destroy();
	Clear();
}

CPythonRune::~CPythonRune()
{
	Destroy();
}

void CPythonRune::Initialize()
{
	BuildPolyStates();
}

void CPythonRune::Destroy()
{
	m_map_Proto.clear();
	m_vec_Proto.clear();
}

void CPythonRune::Clear()
{
	m_set_Runes.clear();
	m_kPageData.Clear();
	m_kNewPageData.Clear();
}

/*******************************************************************\
| [PUBLIC] Proto Functions
\*******************************************************************/

bool CPythonRune::LoadProto(const char* c_pszFileName)
{
	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, c_pszFileName, &pvData))
	{
		Tracenf("CPythonRune::LoadProto(c_szFileName=%s) - Load Error", c_pszFileName);
		return false;
	}

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(kFile.Size(), pvData);

	CTokenVector kTokenVector;
	for (DWORD i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &kTokenVector))
			continue;

		if (kTokenVector.size() != COL_MAX_NUM)
		{
			TraceError("CPythonRune::LoadProto: StrangeLine in %d (tokenCount %d expected %d)", i + 1, kTokenVector.size(), COL_MAX_NUM);
			continue;
		}

		std::string& rstID = kTokenVector[COL_ID];
		std::string& rstName = kTokenVector[COL_NAME];
		std::string& rstGroup = kTokenVector[COL_GROUP];
		std::string& rstSubGroup = kTokenVector[COL_SUBGROUP];
		std::string& rstApplyType = kTokenVector[COL_APPLY_TYPE];
		std::string& rstApplyEval = kTokenVector[COL_APPLY_EVAL];

		DWORD dwID = atoi(rstID.c_str());
		if (m_map_Proto.find(dwID) != m_map_Proto.end())
		{
			TraceError("CPythonRune::LoadProto: StrangeLine in %d (id %u already exists)", i + 1, dwID);
			continue;
		}

		auto pProto = std::make_unique<CPythonRune::TRuneProto>();

		pProto->id = dwID;
		pProto->name = rstName;

		if (rstGroup == "PRECISION") pProto->group = GROUP_PRECISION;
		else if (rstGroup == "DOMINATION") pProto->group = GROUP_DOMINATION;
		else if (rstGroup == "SORCERY") pProto->group = GROUP_SORCERY;
		else if (rstGroup == "RESOLVE") pProto->group = GROUP_RESOLVE;
		else if (rstGroup == "INSPIRATION") pProto->group = GROUP_INSPIRATION;
		else
		{
			TraceError("CPythonRune::LoadProto: StrangeLine in %d (id %u invalid group %s)", i + 1, dwID, rstGroup.c_str());
			continue;
		}

		if (rstSubGroup == "PRIMARY") pProto->sub_group = SUBGROUP_PRIMARY;
		else if (rstSubGroup == "SECONDARY1") pProto->sub_group = SUBGROUP_SECONDARY1;
		else if (rstSubGroup == "SECONDARY2") pProto->sub_group = SUBGROUP_SECONDARY2;
		else if (rstSubGroup == "SECONDARY3") pProto->sub_group = SUBGROUP_SECONDARY3;
		else
		{
			TraceError("CPythonRune::LoadProto: StrangeLine in %d (id %u invalid subgroup %s)", i + 1, dwID, rstSubGroup.c_str());
			continue;
		}

		pProto->sub_group_index = 0;
		for (auto& vecProto : m_vec_Proto)
		{
			if (pProto->group == vecProto->group && pProto->sub_group == vecProto->sub_group)
				++pProto->sub_group_index;
		}

		pProto->apply_type = CItemManager::instance().GetAttrTypeByName(rstApplyType);
		pProto->apply_eval = rstApplyEval;

		m_map_Proto[pProto->id] = pProto.get();
		m_vec_Proto.push_back(std::move(pProto));
	}

	return true;
}

bool CPythonRune::LoadDesc(const char* c_pszFileName)
{
	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, c_pszFileName, &pvData))
	{
		Tracenf("CPythonRune::LoadDesc(c_szFileName=%s) - Load Error", c_pszFileName);
		return false;
	}

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(kFile.Size(), pvData);

	CTokenVector kTokenVector;
	for (DWORD i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &kTokenVector))
			continue;

		if (kTokenVector.size() != 2 && kTokenVector.size() != 3)
		{
			TraceError("CPythonRune::LoadDesc: StrangeLine in %d (tokenCount %d)", i + 1, kTokenVector.size());
			continue;
		}

		const std::string& c_rstID = kTokenVector[0];
		const std::string& c_rstName = kTokenVector[1];
		const std::string* c_pstDesc = kTokenVector.size() > 2 ? &kTokenVector[2] : NULL;

		DWORD dwID = atoi(c_rstID.c_str());
		if (m_map_Proto.find(dwID) == m_map_Proto.end())
		{
			TraceError("CPythonRune::LoadDesc: StrangeLine in %d (id %u not exists)", i + 1, dwID);
			continue;
		}

		auto pProto = m_map_Proto[dwID];
		pProto->name = c_rstName;

		pProto->desc.clear();

		if (c_pstDesc)
			pProto->desc = *c_pstDesc;
	}

	return true;
}

CPythonRune::TRuneProto* CPythonRune::GetProto(DWORD dwID)
{
	auto it = m_map_Proto.find(dwID);
	if (it == m_map_Proto.end())
		return NULL;

	return it->second;
}

CPythonRune::TRuneProto* CPythonRune::FindProtoByIndex(BYTE bGroup, BYTE bSubGroup, BYTE bIndex)
{
	for (auto& pProto : m_vec_Proto)
	{
		if (bGroup == pProto->group && bSubGroup == pProto->sub_group)
		{
			if (bIndex > 0)
			{
				--bIndex;
				continue;
			}

			return pProto.get();
		}
	}

	return NULL;
}

/*******************************************************************\
| [PUBLIC] Rune Functions
\*******************************************************************/

void CPythonRune::AddRune(DWORD dwVnum)
{
	m_set_Runes.insert(dwVnum);
}

bool CPythonRune::HasRune(DWORD dwVnum) const
{
	return m_set_Runes.find(dwVnum) != m_set_Runes.end();
}

void CPythonRune::SetRunePage(const TRunePageData* c_pData)
{
	m_kPageData = *c_pData;
	m_kNewPageData = *c_pData;
}

const TRunePageData& CPythonRune::GetRunePage() const
{
	return m_kPageData;
}

TRunePageData& CPythonRune::GetNewRunePage()
{
	return m_kNewPageData;
}

/*******************************************************************\
| Poly Functions
\*******************************************************************/

void CPythonRune::BuildPolyStates()
{
	m_map_PolyStates.clear();

	m_map_PolyStates["LV"] = POINT_LEVEL;
	m_map_PolyStates["Level"] = POINT_LEVEL;
}

float CPythonRune::ProcessFormula(CPoly* pPoly)
{
	if (pPoly->Analyze())
	{
		for (DWORD i = 0; i < pPoly->GetVarCount(); ++i)
		{
			const char * c_szVarName = pPoly->GetVarName(i);
			int iState = 0;

			auto it = m_map_PolyStates.find(c_szVarName);
			if (it == m_map_PolyStates.end())
			{
				TraceError("runeProcessFormula - unknown poly variable [%s]", c_szVarName);
				return 0.0f;
			}

			float fState = float(CPythonPlayer::Instance().GetStatus(it->second));
			pPoly->SetVar(c_szVarName, fState);
		}
	}
	else
	{
		TraceError("runeProcessFormula - Strange Formula");
		return 0.0f;
	}

	return pPoly->Eval();
}

float CPythonRune::EvalApplyValue(const std::string& c_rstApplyValue)
{
	CPoly kPoly;
	kPoly.SetStr(c_rstApplyValue);

	return ProcessFormula(&kPoly);
}
#endif
