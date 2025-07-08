#include "StdAfx.h"

#ifdef ENABLE_PET_ADVANCED
#include "PythonPetAdvanced.h"
#include "PythonNetworkStream.h"
#include "InstanceBase.h"
#include "../EterPack/EterPackManager.h"
#include "../GameLib/ItemManager.h"
#include <boost/algorithm/string.hpp>

template <typename Cls>
std::map<DWORD, std::unique_ptr<CPetProto<Cls>>> CPetProto<Cls>::s_map_Proto;

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPetSkill - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

// RandomMin
//--------------------------------------------------------------------------
class random_min_FunctionNode : public ExprEval::FunctionNode
{
public:
	random_min_FunctionNode(ExprEval::Expression* expr) : FunctionNode(expr)
	{
		SetArgumentCount(2, 2, 0, 1);
	}

	double DoEvaluate()
	{
		double a = m_nodes[0]->Evaluate();
		double b = m_nodes[1]->Evaluate();

		return a;
	}
};

class random_min_FunctionFactory : public ExprEval::FunctionFactory
{
public:
	string GetName() const
	{
		return "random";
	}

	ExprEval::FunctionNode* DoCreate(ExprEval::Expression* expr)
	{
		return new random_min_FunctionNode(expr);
	}
};

// RandomMax
//--------------------------------------------------------------------------
class random_max_FunctionNode : public ExprEval::FunctionNode
{
public:
	random_max_FunctionNode(ExprEval::Expression* expr) : FunctionNode(expr)
	{
		SetArgumentCount(2, 2, 0, 1);
	}

	double DoEvaluate()
	{
		double a = m_nodes[0]->Evaluate();
		double b = m_nodes[1]->Evaluate();

		return b;
	}
};

class random_max_FunctionFactory : public ExprEval::FunctionFactory
{
public:
	string GetName() const
	{
		return "random";
	}

	ExprEval::FunctionNode* DoCreate(ExprEval::Expression* expr)
	{
		return new random_max_FunctionNode(expr);
	}
};

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPetSkillProto::CPetSkillProto(const TPetAdvancedSkillProto& skillProto) :
	CPetProto(skillProto.apply)
{
	// init this skill
	m_data = skillProto;

	UpdateValues(NULL);
	m_functionList.AddDefaultFunctions();

	try
	{
		__InitExpr(m_valueExpr, skillProto.value_expr);
	}
	catch (ExprEval::EmptyExpressionException e)
	{
		TraceError("Cannot load pet skill %u: empty expression: %s", GetVnum(), e.GetValue().c_str());
	}
	catch (ExprEval::UnmatchedParenthesisException e)
	{
		TraceError("Cannot load pet skill %u: unmatched paranthesis: %s", GetVnum(), e.GetValue().c_str());
	}
	catch (ExprEval::Exception e)
	{
		TraceError("Cannot load pet skill %u: invalid parsing: %s [%u - %u]", GetVnum(), e.GetValue().c_str(), e.GetStart(), e.GetEnd());
	}
}

/*******************************************************************\
| [PUBLIC] General Functions
\*******************************************************************/

void CPetSkillProto::UpdateValues(const network::TPetAdvancedTable* petInfo)
{
	__SetVal("lv", petInfo ? petInfo->level() : 0);

	int sklv = 0;
	if (petInfo)
	{
		for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		{
			if (petInfo->skills(i).vnum() == GetVnum())
			{
				sklv = petInfo->skills(i).level();
				break;
			}
		}
	}
	__SetVal("sklv", sklv);
	__SetVal("maxsklv", PET_SKILL_MAX_LEVEL);
	__SetVal("skpct", MAX(0, (sklv - 1)) / double(PET_SKILL_MAX_LEVEL - 1));
}

BYTE CPetSkillProto::GetApply() const
{
	return m_data.apply;
}

bool CPetSkillProto::IsHeroic() const
{
	return m_data.is_heroic;
}

double CPetSkillProto::GetValue()
{
	return __GetVal(m_valueExpr);
}

std::string CPetSkillProto::GetValueString(bool as_int)
{
	return __GetValString(m_data.value_expr, [=]() { return GetValue(); }, as_int);
}

CGraphicImage* CPetSkillProto::GetIconImage()
{
	return m_data.image;
}

const std::string& CPetSkillProto::GetIconPath()
{
	return m_data.image_path;
}

std::string& CPetSkillProto::GetLocaleName()
{
	return m_data.locale_name;
}

/*******************************************************************\
| [PRIVATE] Helper Functions
\*******************************************************************/

void CPetSkillProto::__InitExpr(ExprEval::Expression& expr, const std::string& evalData)
{
	expr.SetValueList(&m_valueList);
	expr.SetFunctionList(&m_functionList);
	expr.Parse(evalData);
}

void CPetSkillProto::__SetVal(const std::string& name, double value)
{
	double* valPtr = m_valueList.GetAddress(name);
	if (valPtr)
		*valPtr = value;
	else
		m_valueList.Add(name, value);
}

double CPetSkillProto::__GetVal(ExprEval::Expression& expr)
{
	try
	{
		return expr.Evaluate();
	}
	catch (ExprEval::DivideByZeroException e)
	{
		TraceError("Cannot eval pet skill %u: divide by zero: %s", GetVnum(), e.GetValue());
	}
	catch (ExprEval::Exception e)
	{
		TraceError("Cannot eval pet skill %u: %s", GetVnum(), e.GetValue());
	}

	return 0;
}

double CPetSkillProto::__GetMinVal(const std::string& evalData)
{
	ExprEval::Expression tmpExpr;

	// replace random function with minimum function
	ExprEval::FunctionList funcList;
	funcList.AddDefaultFunctions();
	funcList.Remove("random");
	funcList.Add(new random_min_FunctionFactory());

	// parse expression
	tmpExpr.SetValueList(&m_valueList);
	tmpExpr.SetFunctionList(&funcList);
	tmpExpr.Parse(evalData);

	// eval expression
	return tmpExpr.Evaluate();
}

double CPetSkillProto::__GetMaxVal(const std::string& evalData)
{
	ExprEval::Expression tmpExpr;

	// replace random function with minimum function
	ExprEval::FunctionList funcList;
	funcList.AddDefaultFunctions();
	funcList.Remove("random");
	funcList.Add(new random_max_FunctionFactory());

	// parse expression
	tmpExpr.SetValueList(&m_valueList);
	tmpExpr.SetFunctionList(&funcList);
	tmpExpr.Parse(evalData);

	// eval expression
	return tmpExpr.Evaluate();
}

std::string CPetSkillProto::__GetValString(const std::string& evalData, std::function<double()> defaultFunc, bool as_int)
{
	if (evalData.find("random(") != std::string::npos)
	{
		float minVal = __GetMinVal(evalData);
		float maxVal = __GetMaxVal(evalData);
		
		if (as_int)
		{
			int iMinVal = (int) minVal;
			int iMaxVal = (int) maxVal;
			if (iMinVal != iMaxVal)
				return std::to_string(iMinVal) + " - " + std::to_string(iMaxVal);
		}
		else
		{
			char minValStr[20];
			snprintf(minValStr, sizeof(minValStr), "%.3g", minVal);
			char maxValStr[20];
			snprintf(maxValStr, sizeof(maxValStr), "%.3g", maxVal);

			if (strcmp(minValStr, maxValStr))
				return std::string(minValStr) + " - " + std::string(maxValStr);
		}
	}

	if (as_int)
	{
		return std::to_string((int) defaultFunc());
	}
	else
	{
		char valStr[20];
		snprintf(valStr, sizeof(valStr), "%.3g", defaultFunc());
		return valStr;
	}
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPythonPetAdvanced - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPythonPetAdvanced::CPythonPetAdvanced()
{
	Clear();
}

CPythonPetAdvanced::~CPythonPetAdvanced()
{
	CPetSkillProto::Destroy();
}

void CPythonPetAdvanced::Clear()
{
	m_summonVID = 0;
	m_itemVnum = 0;

	m_nextExp = 0;

	m_data.Clear();
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		m_data.add_skills();
	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
	{
		m_data.add_attr_type(0);
		m_data.add_attr_level(0);
	}

	ZeroMemory(m_attrValue, sizeof(m_attrValue));

	m_evolutionRefine.Clear();
	m_attrRefine.Clear();
}

/*******************************************************************\
| [PUBLIC] Proto Functions
\*******************************************************************/

bool CPythonPetAdvanced::LoadSkillProto(const std::string& fileName)
{
	enum ETableTokens {
		TOKEN_APPLY,
		TOKEN_VALUE,
		TOKEN_IS_HEROIC,
		TOKEN_MAX_NUM,
	};

	CPetSkillProto::Destroy();

	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, fileName.c_str(), &pvData))
		return false;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(kFile.Size(), pvData);

	CTokenVector TokenVector;
	for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		if (TokenVector.size() != TOKEN_MAX_NUM)
		{
			TraceError("CPythonPetAdvanced::LoadSkillProto(%s): invalid token size %d on line %d",
				fileName.c_str(), TokenVector.size(), i + 1);
			continue;
		}

		TPetAdvancedSkillProto proto;

		// skill name
		if (!(proto.apply = CItemManager::Instance().GetAttrTypeByName(TokenVector[TOKEN_APPLY])))
		{
			TraceError("CPythonPetAdvanced::LoadSkillProto(%s): invalid apply name [%s] on line %d",
				fileName.c_str(), TokenVector[TOKEN_APPLY].c_str(), i + 1);
			continue;
		}

		const std::string iconImagePath = g_strImagePath + "game/pet/skill/" + TokenVector[TOKEN_APPLY] + ".tga";
		proto.image_path = iconImagePath;
		proto.image = (CGraphicImage *) CResourceManager::Instance().GetResourcePointer(iconImagePath.c_str());

		proto.value_expr = TokenVector[TOKEN_VALUE];
		proto.is_heroic = TokenVector[TOKEN_IS_HEROIC] == "YES";

		// append
		CPetSkillProto::Create(proto);
	}

	return true;
}

bool CPythonPetAdvanced::LoadSkillDesc(const std::string& fileName)
{
	enum EDescTokens {
		TOKEN_APPLY_NAME,
		TOKEN_LOCALE_NAME,
		TOKEN_MAX_NUM,
	};

	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, fileName.c_str(), &pvData))
		return false;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(kFile.Size(), pvData);

	CTokenVector TokenVector;
	for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		if (TokenVector.size() != TOKEN_MAX_NUM)
		{
			TraceError("CPythonPetAdvanced::LoadSkillDesc(%s): invalid token size %d on line %d", fileName.c_str(), TokenVector.size(), i + 1);
			continue;
		}
		else if (TokenVector.size() > TOKEN_MAX_NUM)
		{
			TraceError("CPythonPetAdvanced::LoadSkillDesc(%s): invalid token size %d on line %d (max token size %d)", fileName.c_str(), TokenVector.size(), i + 1, TOKEN_MAX_NUM);
			continue;
		}

		DWORD applyType;
		if (!(applyType = CItemManager::Instance().GetAttrTypeByName(TokenVector[TOKEN_APPLY_NAME])))
		{
			TraceError("CPythonPetAdvanced::LoadSkillDesc(%s): invalid skill name [%s] on line %d", fileName.c_str(), TokenVector[TOKEN_APPLY_NAME].c_str(), i + 1);
			continue;
		}

		auto proto = CPetSkillProto::Get(applyType);
		if (proto == nullptr)
		{
			TraceError("CPythonPetAdvanced::LoadSkillDesc(%s): proto not found for skill %s (id %d) on line %d", fileName.c_str(), TokenVector[TOKEN_APPLY_NAME].c_str(), applyType, i + 1);
			continue;
		}

		proto->GetLocaleName() = TokenVector[TOKEN_LOCALE_NAME];
	}

	return true;
}

bool CPythonPetAdvanced::LoadEvolveProto(const std::string& fileName)
{
	enum EEvolveTokens {
		TOKEN_LEVEL,
		TOKEN_NAME,
		TOKEN_REFINE_ID,
		TOKEN_SCALE,
		TOKEN_NORMAL_SKILL_COUNT,
		TOKEN_HEROIC_SKILL_COUNT,
		TOKEN_SKILLPOWER,
		TOKEN_SKILLPOWER_REROLLABLE,
		TOKEN_MAX_NUM,
	};

	CPetEvolveProto::Destroy();

	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, fileName.c_str(), &pvData))
		return false;

	CMemoryTextFileLoader textFileLoader;
	textFileLoader.Bind(kFile.Size(), pvData);

	CTokenVector TokenVector;
	for (DWORD i = 0; i < textFileLoader.GetLineCount(); ++i)
	{
		if (!textFileLoader.SplitLineByTab(i, &TokenVector))
			continue;

		if (TokenVector.size() < TOKEN_MAX_NUM)
		{
			TraceError("CPythonPetAdvanced::LoadStaminaProto(%s): invalid token size %d on line %d (min token size %d)", fileName.c_str(), TokenVector.size(), i + 1, TOKEN_MAX_NUM);
			continue;
		}
		else if (TokenVector.size() > TOKEN_MAX_NUM)
		{
			TraceError("CPythonPetAdvanced::LoadStaminaProto(%s): invalid token size %d on line %d (max token size %d)", fileName.c_str(), TokenVector.size(), i + 1, TOKEN_MAX_NUM);
			continue;
		}

		TPetAdvancedEvolveProto proto;
		proto.level = std::stoi(TokenVector[TOKEN_LEVEL]);
		proto.name = TokenVector[TOKEN_NAME];
		proto.refine_id = std::stoi(TokenVector[TOKEN_REFINE_ID]);
		proto.scale = std::stoi(TokenVector[TOKEN_SCALE]);
		proto.normal_skill_count = std::stoi(TokenVector[TOKEN_NORMAL_SKILL_COUNT]);
		proto.heroic_skill_count = std::stoi(TokenVector[TOKEN_HEROIC_SKILL_COUNT]);
		proto.skillpower = std::stoi(TokenVector[TOKEN_SKILLPOWER]);
		proto.skillpower_rerollable = std::stoi(TokenVector[TOKEN_SKILLPOWER_REROLLABLE]);

		CPetEvolveProto::Create(proto);
	}

	return true;
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| Python interface
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| General Functions
\*******************************************************************/

PyObject* petIsSummoned(PyObject* self, PyObject* args)
{
	bool isSummoned = CPythonPetAdvanced::Instance().IsSummoned();
	return Py_BuildValue("b", isSummoned);
}

PyObject* petGetSummonVID(PyObject* self, PyObject* args)
{
	DWORD vid = CPythonPetAdvanced::Instance().GetSummonVID();
	return Py_BuildValue("i", vid);
}

PyObject* petGetItemVnum(PyObject* self, PyObject* args)
{
	DWORD vnum = CPythonPetAdvanced::Instance().GetItemVnum();
	return Py_BuildValue("i", vnum);
}

PyObject* petGetName(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("s", info.name().c_str());
}

PyObject* petGetLevel(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("i", info.level());
}

PyObject* petGetExp(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("L", info.exp());
}

PyObject* petGetNextExp(PyObject* self, PyObject* args)
{
	LONGLONG nextExp = CPythonPetAdvanced::Instance().GetNextExp();
	return Py_BuildValue("L", nextExp);
}

PyObject* petGetAttrType(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	auto& info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("i", index <= PET_ATTR_MAX_NUM ? info.attr_type(index) : 0);
}

PyObject* petGetAttrLevel(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	auto& info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("i", index <= PET_ATTR_MAX_NUM ? info.attr_level(index) : 0);
}

PyObject* petGetAttrValue(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	return Py_BuildValue("i", CPythonPetAdvanced::Instance().GetAttrValue(index));
}

PyObject* petGetSkillpower(PyObject* self, PyObject* args)
{
	return Py_BuildValue("i", CPythonPetAdvanced::Instance().GetPetTable().skillpower());
}

PyObject* petGetAttrRefineCost(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetAttrRefine();
	return Py_BuildValue("L", (long long)info.cost());
}

PyObject* petGetAttrRefineMaterialCount(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetAttrRefine();
	return Py_BuildValue("i", info.material_count());
}

PyObject* petGetAttrRefineMaterial(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	auto& info = CPythonPetAdvanced::Instance().GetAttrRefine();
	return Py_BuildValue("ii", info.materials(index).vnum(), info.materials(index).count());
}

PyObject* petGetEvolveCost(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetEvolutionRefine();
	return Py_BuildValue("L", (long long)info.cost());
}

PyObject* petGetEvolveMaterialCount(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetEvolutionRefine();
	return Py_BuildValue("i", info.material_count());
}

PyObject* petGetEvolveMaterial(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	auto& info = CPythonPetAdvanced::Instance().GetEvolutionRefine();
	return Py_BuildValue("ii", info.materials(index).vnum(), info.materials(index).count());
}

PyObject* petGetEvolveProb(PyObject* self, PyObject* args)
{
	auto& info = CPythonPetAdvanced::Instance().GetEvolutionRefine();
	return Py_BuildValue("i", info.prob());
}

/*******************************************************************\
| Skill Functions
\*******************************************************************/

PyObject* petGetSkillSlotByVnum(PyObject* self, PyObject* args)
{
	int vnum;
	if (!PyTuple_GetInteger(args, 0, &vnum))
		return Py_BadArgument();

	auto & info = CPythonPetAdvanced::Instance().GetPetTable();
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
	{
		if (info.skills(i).vnum() == vnum)
			return Py_BuildValue("i", i);
	}

	return Py_BuildValue("i", -1);
}

PyObject* petGetSkillID(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	if (index >= PET_SKILL_MAX_NUM)
		return Py_BuildException("pet skill index %d exceeds maximum of %d", index, PET_SKILL_MAX_NUM);

	auto& info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("i", info.skills(index).vnum());
}

PyObject* petGetSkillLevel(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	if (index >= PET_SKILL_MAX_NUM)
		return Py_BuildException("pet skill index %d exceeds maximum of %d", index, PET_SKILL_MAX_NUM);

	auto & info = CPythonPetAdvanced::Instance().GetPetTable();
	return Py_BuildValue("i", info.skills(index).level());
}

PyObject* petGetSkillLevelByVnum(PyObject* self, PyObject* args)
{
	int vnum;
	if (!PyTuple_GetInteger(args, 0, &vnum))
		return Py_BadArgument();

	auto & info = CPythonPetAdvanced::Instance().GetPetTable();
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
	{
		if (info.skills(i).vnum() == vnum)
			return Py_BuildValue("i", info.skills(i).level());
	}

	return Py_BuildValue("i", 0);
}

/*******************************************************************\
| SkillProto Functions
\*******************************************************************/

PyObject* petUpdateSkillProtoExpr(PyObject* self, PyObject* args)
{
	int skillIdx = -1;
	PyTuple_GetInteger(args, 0, &skillIdx);

	auto& info = CPythonPetAdvanced::Instance().GetPetTable();

	if (skillIdx == -1)
	{
		auto& protoMap = CPetSkillProto::GetMap();
		for (auto& it : protoMap)
		{
			auto skill = dynamic_cast<CPetSkillProto*>(it.second.get());
			skill->UpdateValues(&info);
		}
	}
	else
	{
		auto skill = CPetSkillProto::Get(skillIdx);
		if (skill)
			skill->UpdateValues(&info);
	}

	return Py_BuildNone();
}

PyObject* petSetSkillProtoExprLevel(PyObject* self, PyObject* args)
{
	int skillIdx;
	if (!PyTuple_GetInteger(args, 0, &skillIdx))
		return Py_BadArgument();
	int skillLevel;
	if (!PyTuple_GetInteger(args, 1, &skillLevel))
		return Py_BadArgument();

	auto info_copy = CPythonPetAdvanced::Instance().GetPetTable();
	auto skill = CPetSkillProto::Get(skillIdx);

	if (skill)
	{
		info_copy.mutable_skills(0)->set_vnum(skillIdx);
		info_copy.mutable_skills(0)->set_level(skillLevel);

		skill->UpdateValues(&info_copy);
	}

	return Py_BuildNone();
}

PyObject* petGetSkillProtoApply(PyObject* self, PyObject* args)
{
	int id;
	if (!PyTuple_GetInteger(args, 0, &id))
		return Py_BadArgument();

	auto info = CPetSkillProto::Get(id);
	return Py_BuildValue("i", info ? info->GetApply() : 0);
}

PyObject* petGetSkillProtoValue(PyObject* self, PyObject* args)
{
	int id;
	if (!PyTuple_GetInteger(args, 0, &id))
		return Py_BadArgument();
	bool as_int;
	if (!PyTuple_GetBoolean(args, 2, &as_int))
		as_int = false;

	auto info = CPetSkillProto::Get(id);
	return Py_BuildValue("s", info ? info->GetValueString(as_int).c_str() : "0");
}

PyObject* petGetSkillProtoIconImage(PyObject* self, PyObject* args)
{
	int id;
	if (!PyTuple_GetInteger(args, 0, &id))
		return Py_BadArgument();

	auto info = CPetSkillProto::Get(id);
	return Py_BuildValue("i", info ? info->GetIconImage() : 0);
}

PyObject* petGetSkillProtoIconPath(PyObject* self, PyObject* args)
{
	int id;
	if (!PyTuple_GetInteger(args, 0, &id))
		return Py_BadArgument();

	auto info = CPetSkillProto::Get(id);
	return Py_BuildValue("s", info ? info->GetIconPath().c_str() : "");
}

PyObject* petGetSkillProtoName(PyObject* self, PyObject* args)
{
	int id;
	if (!PyTuple_GetInteger(args, 0, &id))
		return Py_BadArgument();

	auto info = CPetSkillProto::Get(id);
	return Py_BuildValue("s", info ? info->GetLocaleName().c_str() : "");
}

PyObject* petGetEvolveProtoLevel(PyObject* self, PyObject* args)
{
	WORD index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	auto& map = CPetEvolveProto::GetMap();
	if (index >= map.size())
		return Py_BuildValue("i", -1);

	auto it = map.begin();
	while (index > 0)
		it++, index--;

	return Py_BuildValue("i", it->first);
}

PyObject* petGetEvolveProtoName(PyObject* self, PyObject* args)
{
	WORD level;
	if (!PyTuple_GetInteger(args, 0, &level))
		return Py_BadArgument();

	auto proto = CPetEvolveProto::Get(level);
	return Py_BuildValue("s", proto ? proto->GetName().c_str() : "");
}

PyObject* petGetEvolveProtoScale(PyObject* self, PyObject* args)
{
	WORD level;
	if (!PyTuple_GetInteger(args, 0, &level))
		return Py_BadArgument();

	auto proto = CPetEvolveProto::Get(level);
	return Py_BuildValue("f", proto ? proto->GetScale() : 0.0f);
}

PyObject* petGetEvolveProtoSkillpower(PyObject* self, PyObject* args)
{
	WORD level;
	if (!PyTuple_GetInteger(args, 0, &level))
		return Py_BadArgument();

	auto proto = CPetEvolveProto::Get(level);
	return Py_BuildValue("i", proto ? proto->GetSkillpower() : 0);
}

PyObject* petCanEvolveProtoSkillpowerReroll(PyObject* self, PyObject* args)
{
	WORD level;
	if (!PyTuple_GetInteger(args, 0, &level))
		return Py_BadArgument();

	auto proto = CPetEvolveProto::Get(level);
	return Py_BuildValue("b", proto ? proto->IsSkillpowerRerollable() : false);
}

PyObject* petGetEvolveProtoNormalSkillCount(PyObject* self, PyObject* args)
{
	WORD level;
	if (!PyTuple_GetInteger(args, 0, &level))
		return Py_BadArgument();

	auto proto = CPetEvolveProto::Get(level);
	return Py_BuildValue("i", proto ? proto->GetNormalSkillCount() : 0);
}

PyObject* petGetHeroicProtoPassiveSkillCount(PyObject* self, PyObject* args)
{
	WORD level;
	if (!PyTuple_GetInteger(args, 0, &level))
		return Py_BadArgument();

	auto proto = CPetEvolveProto::Get(level);
	return Py_BuildValue("i", proto ? proto->GetHeroicSkillCount() : 0);
}

/*******************************************************************\
| Network Functions
\*******************************************************************/

PyObject* petUseEgg(PyObject* self, PyObject* args)
{
	TItemPos egg_position;

	int argi = 0;
	if (PyTuple_Size(args) >= 3)
	{
		if (!PyTuple_GetInteger(args, argi++, &egg_position.window_type))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(args, argi++, &egg_position.cell))
			return Py_BadArgument();
	}
	else
	{
		egg_position.window_type = INVENTORY;
		if (!PyTuple_GetInteger(args, argi++, &egg_position.cell))
			return Py_BadArgument();
	}

	char* name;
	if (!PyTuple_GetString(args, argi++, &name))
		return Py_BadArgument();

	CPythonNetworkStream::Instance().SendPetUseEggPacket(egg_position, name);
	return Py_BuildNone();
}

PyObject* petRequestAttrRefineInfo(PyObject* self, PyObject* args)
{
	BYTE index;
	if (!PyTuple_GetInteger(args, 0, &index))
		return Py_BadArgument();

	CPythonNetworkStream::Instance().SendPetAttrRefineInfoPacket(index);
	return Py_BuildNone();
}

PyObject* petRequestEvolutionInfo(PyObject* self, PyObject* args)
{
	CPythonNetworkStream::Instance().SendPetEvolutionInfoPacket();
	return Py_BuildNone();
}

PyObject* petEvolve(PyObject* self, PyObject* args)
{
	CPythonNetworkStream::Instance().SendPetEvolvePacket();
	return Py_BuildNone();
}

PyObject* petUseResetSkill(PyObject* self, PyObject* args)
{
	TItemPos reset_item_position;

	int argi = 0;
	if (PyTuple_Size(args) >= 3)
	{
		if (!PyTuple_GetInteger(args, argi++, &reset_item_position.window_type))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(args, argi++, &reset_item_position.cell))
			return Py_BadArgument();
	}
	else
	{
		reset_item_position.window_type = INVENTORY;
		if (!PyTuple_GetInteger(args, argi++, &reset_item_position.cell))
			return Py_BadArgument();
	}

	BYTE skill_index;
	if (!PyTuple_GetInteger(args, argi++, &skill_index))
		return Py_BadArgument();

	CPythonNetworkStream::Instance().SendPetResetSkillPacket(reset_item_position, skill_index);
	return Py_BuildNone();
}

void initPetAdvanced()
{
	static PyMethodDef s_methods[] =
	{
		{ "IsSummoned",						petIsSummoned,						METH_VARARGS },
		{ "GetSummonVID",					petGetSummonVID,					METH_VARARGS },
		{ "GetItemVnum",					petGetItemVnum,						METH_VARARGS },

		{ "GetName",						petGetName,							METH_VARARGS },
		{ "GetLevel",						petGetLevel,						METH_VARARGS },
		{ "GetExp",							petGetExp,							METH_VARARGS },
		{ "GetNextExp",						petGetNextExp,						METH_VARARGS },
		{ "GetAttrType",					petGetAttrType,						METH_VARARGS },
		{ "GetAttrLevel",					petGetAttrLevel,					METH_VARARGS },
		{ "GetAttrValue",					petGetAttrValue,					METH_VARARGS },
		{ "GetSkillpower",					petGetSkillpower,					METH_VARARGS },

		{ "GetAttrRefineCost",				petGetAttrRefineCost,				METH_VARARGS },
		{ "GetAttrRefineMaterialCount",		petGetAttrRefineMaterialCount,		METH_VARARGS },
		{ "GetAttrRefineMaterial",			petGetAttrRefineMaterial,			METH_VARARGS },

		{ "GetEvolveCost",					petGetEvolveCost,					METH_VARARGS },
		{ "GetEvolveMaterialCount",			petGetEvolveMaterialCount,			METH_VARARGS },
		{ "GetEvolveMaterial",				petGetEvolveMaterial,				METH_VARARGS },
		{ "GetEvolveProb",					petGetEvolveProb,					METH_VARARGS },

		{ "GetSkillID",						petGetSkillID,						METH_VARARGS },
		{ "GetSkillSlotByVnum",				petGetSkillSlotByVnum,				METH_VARARGS },
		{ "GetSkillLevel",					petGetSkillLevel,					METH_VARARGS },
		{ "GetSkillLevelByVnum",			petGetSkillLevelByVnum,				METH_VARARGS },

		{ "UpdateSkillProtoExpr",			petUpdateSkillProtoExpr,			METH_VARARGS },
		{ "SetSkillProtoExprLevel",			petSetSkillProtoExprLevel,			METH_VARARGS },
		{ "GetSkillProtoApply",				petGetSkillProtoApply,				METH_VARARGS },
		{ "GetSkillProtoValue",				petGetSkillProtoValue,				METH_VARARGS },
		{ "GetSkillProtoIconImage",			petGetSkillProtoIconImage,			METH_VARARGS },
		{ "GetSkillProtoIconPath",			petGetSkillProtoIconPath,			METH_VARARGS },

		{ "GetSkillProtoName",				petGetSkillProtoName,				METH_VARARGS },

		{ "GetEvolveProtoLevel",				petGetEvolveProtoLevel,					METH_VARARGS },
		{ "GetEvolveProtoName",					petGetEvolveProtoName,					METH_VARARGS },
		{ "GetEvolveProtoScale",				petGetEvolveProtoScale,					METH_VARARGS },
		{ "GetEvolveProtoSkillpower",			petGetEvolveProtoSkillpower,			METH_VARARGS },
		{ "CanEvolveProtoSkillpowerReroll",		petCanEvolveProtoSkillpowerReroll,		METH_VARARGS },

		{ "UseEgg",							petUseEgg,							METH_VARARGS },
		{ "RequestAttrRefineInfo",			petRequestAttrRefineInfo,			METH_VARARGS },
		{ "RequestEvolutionInfo",			petRequestEvolutionInfo,			METH_VARARGS },
		{ "Evolve",							petEvolve,							METH_VARARGS },
		{ "UseResetSkill",					petUseResetSkill,					METH_VARARGS },

		{ NULL,								NULL,								NULL },
	};

	PyObject* poModule = Py_InitModule("pet", s_methods);
	
	PyModule_AddIntConstant(poModule, "SKILL_NORMAL_START",			PET_SKILL_NORMAL_START);
	PyModule_AddIntConstant(poModule, "SKILL_HEROIC_START",			PET_SKILL_HEROIC_START);
	PyModule_AddIntConstant(poModule, "SKILL_MAX_LEVEL",			PET_SKILL_MAX_LEVEL);
	PyModule_AddIntConstant(poModule, "SKILL_HEROIC_MAX_LEVEL",		PET_SKILL_HEROIC_MAX_LEVEL);
	PyModule_AddIntConstant(poModule, "SKILL_MAX_NUM",				PET_SKILL_MAX_NUM);
	
	PyModule_AddIntConstant(poModule, "MAX_LEVEL",					PET_MAX_LEVEL);

	PyModule_AddIntConstant(poModule, "ATTR_MAX_NUM",				PET_ATTR_MAX_NUM);
}
#endif
