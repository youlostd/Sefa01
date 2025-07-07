#include "stdafx.h"
#include "../../common/stl.h"

#include "constants.h"
#include "skill.h"
#include "char.h"

CSkillProto::CSkillProto()
{
	// all possible values added
	_expr_value_list.Add("k");
	_expr_value_list.Add("wep");
	_expr_value_list.Add("mtk");
	_expr_value_list.Add("mwep");
	_expr_value_list.Add("lv");
	_expr_value_list.Add("iq");
	_expr_value_list.Add("str");
	_expr_value_list.Add("dex");
	_expr_value_list.Add("con");
	_expr_value_list.Add("def");
	_expr_value_list.Add("odef");
	_expr_value_list.Add("horse_level");
	_expr_value_list.Add("sklv");
	_expr_value_list.Add("isLegendary");
	_expr_value_list.Add("ar");
	_expr_value_list.Add("atk");
	_expr_value_list.Add("maxv");
	_expr_value_list.Add("maxhp");
	_expr_value_list.Add("maxsp");
	_expr_value_list.Add("chain");
	_expr_value_list.Add("ek");
	_expr_value_list.Add("v");

	// init functions
	_expr_function_list.AddDefaultFunctions();
}

void CSkillProto::SetVar(const std::string& strName, double dVar)
{
	double* valPtr = _expr_value_list.GetAddress(strName);
	if (valPtr)
		*valPtr = dVar;
	else
	{
		sys_err("unexpected CSkillProto::SetVar of new variable [name \"%s\"] - it's probably not used, consider adding it to CSkillProto constructor to make it usable");
		_expr_value_list.Add(strName, dVar);
	}
}

double CSkillProto::GetVar(const std::string& strName)
{
	double* valPtr = _expr_value_list.GetAddress(strName);
	if (valPtr)
		return *valPtr;

	return 0.0;
}

bool CSkillProto::parse(ExprEval::Expression& expr, const std::string& expr_string)
{
	try
	{
		expr.SetValueList(&_expr_value_list);
		expr.SetFunctionList(&_expr_function_list);
		expr.Parse(expr_string);

		return true;
	}
	catch (const ExprEval::EmptyExpressionException & e)
	{
		return parse(expr, "0");
//		sys_err("CSkillProto[%u]: invalid skill expression [%s]: empty expression: %s", dwVnum, expr_string.c_str(), e.GetValue().c_str());
	}
	catch (const ExprEval::UnmatchedParenthesisException & e)
	{
		sys_err("CSkillProto[%u]: invalid skill expression [%s]: unmatched paranthesis: %s", dwVnum, expr_string.c_str(), e.GetValue().c_str());
	}
	catch (const ExprEval::Exception & e)
	{
		sys_err("CSkillProto[%u]: invalid skill expression [%s]: invalid parsing: %s [%u - %u]", dwVnum, expr_string.c_str(), e.GetValue().c_str(), e.GetStart(), e.GetEnd());
	}

	return false;
}

CSkillManager::CSkillManager()
{
}

CSkillManager::~CSkillManager()
{
	itertype(m_map_pkSkillProto) it = m_map_pkSkillProto.begin();
	for ( ; it != m_map_pkSkillProto.end(); ++it) {
		M2_DELETE(it->second);
	}
}

struct SPointOnType
{
	const char * c_pszName;
	int		 iPointOn;
} kPointOnTypes[] = {
	{ "NONE",		POINT_NONE		},
	{ "MAX_HP",		POINT_MAX_HP		},
	{ "MAX_SP",		POINT_MAX_SP		},
	{ "HP_REGEN",	POINT_HP_REGEN		},
	{ "SP_REGEN",	POINT_SP_REGEN		},
	{ "BLOCK",		POINT_BLOCK		},
	{ "HP",		POINT_HP		},
	{ "SP",		POINT_SP		},
	{ "ATT_GRADE",	POINT_ATT_GRADE_BONUS	},
	{ "DEF_GRADE",	POINT_DEF_GRADE_BONUS	},
	{ "MAGIC_ATT_GRADE",POINT_MAGIC_ATT_GRADE_BONUS	},
	{ "MAGIC_DEF_GRADE",POINT_MAGIC_DEF_GRADE_BONUS	},
	{ "BOW_DISTANCE",	POINT_BOW_DISTANCE	},
	{ "MOV_SPEED",	POINT_MOV_SPEED		},
	{ "ATT_SPEED",	POINT_ATT_SPEED		},
	{ "POISON_PCT",	POINT_POISON_PCT	},
	{ "RESIST_RANGE",   POINT_RESIST_BOW	},
	//{ "RESIST_MELEE",	POINT_RESIST_MELEE	},
	{ "CASTING_SPEED",	POINT_CASTING_SPEED	},
	{ "REFLECT_MELEE",	POINT_REFLECT_MELEE	},
	{ "ATT_BONUS",	POINT_ATT_BONUS		},
	{ "DEF_BONUS",	POINT_DEF_BONUS		},
	{ "RESIST_NORMAL",	POINT_RESIST_NORMAL_DAMAGE },
	{ "DODGE",		POINT_DODGE		},
	{ "KILL_HP_RECOVER",POINT_KILL_HP_RECOVERY	},
	{ "KILL_SP_RECOVER",POINT_KILL_SP_RECOVER	},
	{ "HIT_HP_RECOVER",	POINT_HIT_HP_RECOVERY	},
	{ "HIT_SP_RECOVER",	POINT_HIT_SP_RECOVERY	},
	{ "CRITICAL",	POINT_CRITICAL_PCT	},
	{ "PENETRATE",	POINT_PENETRATE_PCT	},
	{ "RESIST_CRITICAL",	POINT_RESIST_CRITICAL	},
	{ "RESIST_PENETRATE",	POINT_RESIST_PENETRATE	},
	{ "MANASHIELD",	POINT_MANASHIELD	},
	{ "SKILL_DAMAGE_BONUS", POINT_SKILL_DAMAGE_BONUS	},
	{ "NORMAL_HIT_DAMAGE_BONUS", POINT_NORMAL_HIT_DAMAGE_BONUS	},
	{ "BLOCK_IGNORE_BONUS", POINT_BLOCK_IGNORE_BONUS },
#ifdef STANDARD_SKILL_DURATION
	{ "SKILL_DURATION",	POINT_SKILL_DURATION },
#endif
	{ "ATTBONUS_MONSTER_DIV10", POINT_ATTBONUS_MONSTER_DIV10 },

	{ "\n",		POINT_NONE		},
};

int FindPointType(const char * c_sz)
{
	for (int i = 0; *kPointOnTypes[i].c_pszName != '\n'; ++i)
	{
		if (!strcasecmp(c_sz, kPointOnTypes[i].c_pszName))
			return kPointOnTypes[i].iPointOn;
	}
	return -1;
}

bool CSkillManager::Initialize(const ::google::protobuf::RepeatedPtrField<network::TSkillTable>& table)
{
	char buf[1024];
	std::map<DWORD, CSkillProto *> map_pkSkillProto;

	bool bError = false;

	for (auto& t : table)
	{
		CSkillProto * pkProto = M2_NEW CSkillProto;

		pkProto->dwVnum = t.vnum();
		strlcpy(pkProto->szName, t.name().c_str(), sizeof(pkProto->szName));
		pkProto->dwType = t.type();
		pkProto->bMaxLevel = t.max_level();
		pkProto->dwFlag = t.flag();
		pkProto->dwAffectFlag = t.affect_flag();
#ifdef __LEGENDARY_SKILL__
		pkProto->dwAffectFlagLegendary = t.affect_flag_legendary();
#endif
		pkProto->dwAffectFlag2 = t.affect_flag2();
#ifdef __LEGENDARY_SKILL__
		pkProto->dwAffectFlag2Legendary = t.affect_flag2_legendary();
#endif

		pkProto->bLevelStep = t.level_step();
		pkProto->bLevelLimit = t.level_limit();
		pkProto->iSplashRange = t.splash_range();
		pkProto->dwTargetRange = t.target_range();
		pkProto->preSkillVnum = t.pre_skill_vnum();
		pkProto->preSkillLevel = t.pre_skill_level();

		pkProto->lMaxHit = t.max_hit();

		pkProto->bSkillAttrType = t.skill_attr_type();

		int iIdx = FindPointType(t.point_on().c_str());

		if (iIdx < 0)
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn: %s",
				t.name().c_str(), t.point_on().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		pkProto->bPointOn = iIdx;

		int iIdx2 = FindPointType(t.point_on2().c_str());

		if (iIdx2 < 0)
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn2: %s",
				t.name().c_str(), t.point_on2().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		pkProto->bPointOn2 = iIdx2;

		int iIdx3 = FindPointType(t.point_on3().c_str());

		if (iIdx3 < 0)
		{
			if (t.point_on3()[0] == 0)
			{
				iIdx3 = POINT_NONE;
			}
			else
			{
				snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn3: %s",
					t.name().c_str(), t.point_on3().c_str());
				sys_err("%s", buf);
				SendLog(buf);
				bError = true;
				M2_DELETE(pkProto);
				continue;
			}
		}

		pkProto->bPointOn3 = iIdx3;

		if (!pkProto->parse(pkProto->kSplashAroundDamageAdjustPoly, t.splash_around_damage_adjust_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szSplashAroundDamageAdjustPoly: %s",
				t.name().c_str(), t.splash_around_damage_adjust_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kPointPoly, t.point_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly: %s",
				t.name().c_str(), t.point_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kPointPoly2, t.point_poly2()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly2: %s",
				t.name().c_str(), t.point_poly2().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kPointPoly3, t.point_poly3()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly3: %s",
				t.name().c_str(), t.point_poly3().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kSPCostPoly, t.sp_cost_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szSPCostPoly: %s",
				t.name().c_str(), t.sp_cost_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kGrandMasterAddSPCostPoly, t.grand_master_add_sp_cost_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szGrandMasterAddSPCostPoly: %s",
				t.name().c_str(), t.grand_master_add_sp_cost_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kDurationPoly, t.duration_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly: %s",
				t.name().c_str(), t.duration_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kDurationPoly2, t.duration_poly2()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly2: %s",
				t.name().c_str(), t.duration_poly2().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kDurationPoly3, t.duration_poly3()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly3: %s",
				t.name().c_str(), t.duration_poly3().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kDurationSPCostPoly, t.duration_sp_cost_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationSPCostPoly: %s",
				t.name().c_str(), t.duration_sp_cost_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->parse(pkProto->kCooldownPoly, t.cooldown_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szCooldownPoly: %s",
				t.name().c_str(), t.cooldown_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		sys_log(0, "Master %s", t.master_bonus_poly().c_str());
		if (!pkProto->parse(pkProto->kMasterBonusPoly, t.master_bonus_poly()))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szMasterBonusPoly: %s",
				t.name().c_str(), t.master_bonus_poly().c_str());
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		sys_log(0, "#%-3d %-24s type %u flag %u affect %u point_poly: %s",
				pkProto->dwVnum, pkProto->szName, pkProto->dwType, pkProto->dwFlag, pkProto->dwAffectFlag, t.point_poly().c_str());

		map_pkSkillProto.insert(std::map<DWORD, CSkillProto *>::value_type(pkProto->dwVnum, pkProto));
	}

	if (!bError)
	{
		// 기존 테이블의 내용을 지운다.
		itertype(m_map_pkSkillProto) it = m_map_pkSkillProto.begin();

		while (it != m_map_pkSkillProto.end()) {
			M2_DELETE(it->second);
			++it;
		}

		m_map_pkSkillProto.clear();

		// 새로운 내용을 삽입
		it = map_pkSkillProto.begin();

		while (it != map_pkSkillProto.end())
		{
			m_map_pkSkillProto.insert(std::map<DWORD, CSkillProto *>::value_type(it->first, it->second));
			++it;
		}

		SendLog("Skill Prototype reloaded!");
	}
	else
		SendLog("There were erros when loading skill table");

	return !bError;
}

bool CSkillManager::InitializeNames(BYTE bLanguageID, const char* szSkillDescFileName)
{
	FILE* pFile = fopen(szSkillDescFileName, "r");
	if (pFile == NULL)
	{
		sys_err("cannot initialize skill names for language [%d %s]", bLanguageID, CLocaleManager::instance().GetLanguageName(bLanguageID));
		return false;
	}

	char buf[1024];
	while (fgets(buf, sizeof(buf), pFile))
	{
		int iTokenIndex = 3;

		const char* token_string = strtok(buf, "\t");

		int iSkillIndex;
		if (token_string)
			str_to_number(iSkillIndex, token_string);

		while (--iTokenIndex > 0 && token_string != NULL)
			token_string = strtok(NULL, "\t");

		if (!token_string)
			continue;

		CSkillProto* pProto = Get(iSkillIndex);
		if (pProto)
			strcpy(pProto->szLocaleName[bLanguageID], token_string);

		sys_log(1, "CSkillManager::InitializeNames [%u] %d %s", bLanguageID, iSkillIndex, token_string);
	}

	fclose(pFile);

	return true;
}

CSkillProto * CSkillManager::Get(DWORD dwVnum)
{
	std::map<DWORD, CSkillProto *>::iterator it = m_map_pkSkillProto.find(dwVnum);

	if (it == m_map_pkSkillProto.end())
		return NULL;

	return it->second;
}

CSkillProto * CSkillManager::Get(const char * c_pszSkillName)
{
	std::map<DWORD, CSkillProto *>::iterator it = m_map_pkSkillProto.begin();

	while (it != m_map_pkSkillProto.end())
	{
		if (!strcasecmp(it->second->szName, c_pszSkillName))
			return it->second;

		it++;
	}

	return NULL;
}

