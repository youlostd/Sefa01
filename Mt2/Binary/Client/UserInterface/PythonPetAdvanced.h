#pragma once

#include "StdAfx.h"

#ifdef ENABLE_PET_ADVANCED
#include "Packet.h"
#include "expreval.h"
#include "../EterLib/GrpImage.h"
#include <functional>
#include <memory>

template <class Cls>
class CPetProto
{
public:
	using TProtoMap = std::map<DWORD, std::unique_ptr<CPetProto<Cls>>>;

	static Cls* Get(DWORD index)
	{
		auto it = s_map_Proto.find(index);
		if (it == s_map_Proto.end())
			return nullptr;

		return dynamic_cast<Cls*>(it->second.get());
	}
	static const TProtoMap& GetMap()
	{
		return s_map_Proto;
	}
	static void Destroy()
	{
		s_map_Proto.clear();
	}

	virtual ~CPetProto() = default;

protected:
	CPetProto(DWORD index)
	{
		s_map_Proto[index] = std::unique_ptr<CPetProto<Cls>>(this);
	}

private:
	static TProtoMap s_map_Proto;
};

typedef struct SPetAdvancedSkillProto {
	BYTE	apply;
	std::string	value_expr;
	bool	is_heroic;

	std::string	image_path;
	CGraphicImage*	image;

	std::string	locale_name;
} TPetAdvancedSkillProto;

class CPetSkillProto : public CPetProto<CPetSkillProto>
{
private:
	CPetSkillProto(const TPetAdvancedSkillProto& skillProto);

public:
	static void Create(const TPetAdvancedSkillProto& skillProto)
	{
		new CPetSkillProto(skillProto);
	}

	DWORD		GetVnum() const { return m_data.apply; }

	void		UpdateValues(const network::TPetAdvancedTable* petData);

	BYTE		GetApply() const;
	bool		IsHeroic() const;
	double		GetValue();
	std::string	GetValueString(bool as_int = false);

	CGraphicImage*	GetIconImage();
	const std::string&	GetIconPath();

	std::string&	GetLocaleName();

private:
	void		__InitExpr(ExprEval::Expression& expr, const std::string& evalData);
	void		__SetVal(const std::string& name, double value);
	double		__GetVal(ExprEval::Expression& expr);
	double		__GetMinVal(const std::string& evalData);
	double		__GetMaxVal(const std::string& evalData);
	std::string	__GetValString(const std::string& evalData, std::function<double()> defaultFunc, bool as_int);

private:
	TPetAdvancedSkillProto	m_data;

	ExprEval::ValueList		m_valueList;
	ExprEval::FunctionList	m_functionList;

	ExprEval::Expression	m_valueExpr;
};

typedef struct SPetAdvancedEvolveProto {
	WORD	level;
	std::string	name;
	DWORD	refine_id;
	float	scale;
	BYTE	normal_skill_count;
	BYTE	heroic_skill_count;
	BYTE	skillpower;
	bool	skillpower_rerollable;
} TPetAdvancedEvolveProto;

class CPetEvolveProto : public CPetProto<CPetEvolveProto>
{
private:
	CPetEvolveProto(const TPetAdvancedEvolveProto& evolveProto) :
		CPetProto(evolveProto.level)
	{
		m_level = evolveProto.level;
		m_name = evolveProto.name;
		m_refine_id = evolveProto.refine_id;
		m_scale = evolveProto.scale;
		m_normal_skill_count = evolveProto.normal_skill_count;
		m_heroic_skill_count = evolveProto.heroic_skill_count;
		m_skillpower = evolveProto.skillpower;
		m_skillpower_rerollable = evolveProto.skillpower_rerollable;
	}

public:
	static void Create(const TPetAdvancedEvolveProto& evolveProto)
	{
		new CPetEvolveProto(evolveProto);
	}

	WORD				GetLevel() const { return m_level; }
	const std::string&	GetName() const { return m_name; }
	DWORD				GetRefineID() const { return m_refine_id; }
	float				GetScale() const { return m_scale; }
	BYTE				GetNormalSkillCount() const { return m_normal_skill_count; }
	BYTE				GetHeroicSkillCount() const { return m_heroic_skill_count; }
	BYTE				GetSkillpower() const { return m_skillpower; }
	bool				IsSkillpowerRerollable() const { return m_skillpower_rerollable; }

private:
	WORD		m_level;
	std::string	m_name;
	DWORD		m_refine_id;
	float		m_scale;
	BYTE		m_normal_skill_count;
	BYTE		m_heroic_skill_count;
	BYTE		m_skillpower;
	bool		m_skillpower_rerollable;
};

class CPythonPetAdvanced : public singleton<CPythonPetAdvanced>
{
public:
	CPythonPetAdvanced();
	~CPythonPetAdvanced();

	void		Clear();

public:
	bool		LoadSkillProto(const std::string& fileName);
	bool		LoadSkillDesc(const std::string& fileName);
	bool		LoadEvolveProto(const std::string& fileName);

	// data
public:
	void		SetSummonVID(DWORD vid) { m_summonVID = vid; }
	DWORD		GetSummonVID() const { return m_summonVID; }
	bool		IsSummoned() const { return m_summonVID != 0; }

	void		SetItemVnum(DWORD vnum) { m_itemVnum = vnum; }
	DWORD		GetItemVnum() const { return m_itemVnum; }

	network::TPetAdvancedTable& GetPetTable() { return m_data; }

	void		SetNextExp(LONGLONG nextExp) { m_nextExp = nextExp; }
	LONGLONG	GetNextExp() const { return m_nextExp; }

	void		SetEvolutionRefine(const network::TRefineTable& refine) { m_evolutionRefine = refine; }
	const network::TRefineTable&	GetEvolutionRefine() const { return m_evolutionRefine; }

	void		SetAttrRefine(const network::TRefineTable& refine) { m_attrRefine = refine; }
	const network::TRefineTable& GetAttrRefine() const { return m_attrRefine; }

	void		SetAttrValue(BYTE index, int value) { if (index < PET_ATTR_MAX_NUM) m_attrValue[index] = value; }
	int			GetAttrValue(BYTE index) const { return index >= PET_ATTR_MAX_NUM ? 0 : m_attrValue[index]; }

private:
	DWORD				m_summonVID;
	DWORD				m_itemVnum;

	network::TPetAdvancedTable	m_data;
	LONGLONG			m_nextExp;

	network::TRefineTable		m_evolutionRefine;
	network::TRefineTable		m_attrRefine;

	int					m_attrValue[PET_ATTR_MAX_NUM];
};
#endif
