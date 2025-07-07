#pragma once

#include "stdafx.h"

#ifdef __PET_ADVANCED__
#include "../../common/tables.h"
#include "../../libexpreval/expreval.h"
#include "headers.hpp"
#include "utils.h"

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

class CPetSkillProto : public CPetProto<CPetSkillProto>
{
protected:
	CPetSkillProto(const network::TPetAdvancedSkillProto& skillProto);

public:
	static void Create(const network::TPetAdvancedSkillProto& skillProto)
	{
		new CPetSkillProto(skillProto);
	}

	DWORD	GetVnum() const { return m_applyType; }

	void	UpdateValues(const network::TPetAdvancedTable* petData);
	BYTE	GetApply() const { return GetVnum(); }
	double	GetValue();
	bool	IsHeroic() const { return m_isHeroic; }

private:
	void	__InitExpr(ExprEval::Expression& expr, const std::string& evalData);
	void	__SetVal(const std::string& name, double value);
	double	__GetVal(ExprEval::Expression& expr);

private:
	BYTE					m_applyType;
	ExprEval::Expression	m_valueExpr;
	bool					m_isHeroic;

	ExprEval::ValueList		m_valueList;
	ExprEval::FunctionList	m_functionList;
};

class CPetEvolveProto : public CPetProto<CPetEvolveProto>
{
private:
	CPetEvolveProto(const network::TPetAdvancedEvolveProto& evolveProto);

public:
	static void Create(const network::TPetAdvancedEvolveProto& evolveProto)
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
	bool				CanSkillpowerReroll() const { return m_can_skillpower_reroll; }

private:
	WORD		m_level;
	std::string	m_name;
	DWORD		m_refine_id;
	float		m_scale;
	BYTE		m_normal_skill_count;
	BYTE		m_heroic_skill_count;
	BYTE		m_skillpower;
	bool		m_can_skillpower_reroll;
};

class CPetAttrProto : public CPetProto<CPetAttrProto>
{
private:
	CPetAttrProto(const network::TPetAdvancedAttrProto& attrProto);

public:
	static void Create(const network::TPetAdvancedAttrProto& attrProto)
	{
		new CPetAttrProto(attrProto);
	}

	static DWORD GetKey(BYTE apply_type, BYTE attr_level)
	{
		return (static_cast<uint32_t>(apply_type) * 1000) + attr_level;
	}

	BYTE		GetType() const { return m_type; }
	float		GetValue() const { return m_value; }
	DWORD		GetRefineID() const { return m_refine_id; }

private:
	BYTE		m_type;
	float		m_value;
	DWORD		m_refine_id;
};

class CPetAdvanced
{
public:
	EVENTINFO(event_info)
	{
		CPetAdvanced* pet;
	};

public:
	CPetAdvanced(LPITEM owner);
	~CPetAdvanced();

	void		Initialize(const std::string& name, bool can_override = false);
	void		Initialize(const network::TPetAdvancedTable* data);

	// data [get] functions
public:
	LPCHARACTER	GetOwner() const;
	LPITEM		GetItem() const { return m_ownerItem; }

	DWORD		GetItemID() const { return m_data.item_id(); }
	const char* GetName() const { return m_data.name().c_str(); }
	
	WORD		GetMaxLevel() const;
	WORD		GetLevel() const { return m_data.level(); }
	void		SetLevel(WORD level);
	long long	GetExp() const { return m_data.exp(); }
	void		GiveExp(long long exp);
	long long	GetNextExp() const;

	const network::TPetAdvancedSkillData* GetSkillInfo(BYTE index) const { return index < PET_SKILL_MAX_NUM ? &m_data.skills(index) : NULL; }
	CPetSkillProto* GetSkillProto(BYTE index) const;

	bool		IsLoaded() const { return m_isLoaded; }

	int			GetAttrIndexByType(BYTE attr_type) const;
	BYTE		GetAttrType(BYTE index) const;
	BYTE		GetAttrLevel(BYTE index) const;
	int			GetAttrValue(BYTE index) const;

	// save functions
public:
	void		Save();
	void		SaveDirect();
	void		CopyPetTable(network::TPetAdvancedTable* dest);

	// general functions
public:
	bool		IsSummoned() const { return m_pet != NULL; }
	LPCHARACTER	GetSummoned() const { return m_pet; }
	bool		CanSummon(bool message = true) const;
	LPCHARACTER	Summon();
	void		SummonOnLoad() { m_isSummonOnLoad = true; }
	void		CheckSummonOnLoad();
	void		Unsummon();
	void		OnDestroyPet();

	void		Update();

	void		ApplyBuff(bool add);

	void		ChangeName(const char* c_pszNewName);

	const CPetEvolveProto*	GetEvolveData() const;
	const CPetEvolveProto*	GetNextEvolveData() const;
	bool		CanEvolve() const;
	void		ShowEvolutionInfo();
	void		Evolve();

	void		RerollAttr();
	bool		CanUpgradeAttr(BYTE index) const;
	bool		UpgradeAttr(BYTE index);
	void		ComputeAttr(bool add = true, int index = -1);
	void		ShowAttrRefineInfo(BYTE index);

	void		RerollSkillpower();
	BYTE		GetSkillpower() const;

private:
	void		__UpdateFollowAI();
	void		__Follow(int32_t minDistance);

	// skill functions
private:
	bool		__SetSkill(WORD index, DWORD vnum, WORD level);
	void		__ComputeSkills(bool add);

public:
	int			GetSkillIndexByVnum(DWORD vnum) const;
	bool		HasSkill(DWORD vnum) const;

	void		ClearSkill(WORD index);
	void		ResetSkills();

	bool		CanSkillLevelUp(BYTE index);
	bool		SkillLevelUp(BYTE index);
	bool		CanLearnSkill(DWORD skill_vnum);
	bool		LearnSkill(DWORD skill_vnum);

	bool		SetSkillLevel(BYTE index, DWORD skill_vnum, WORD skill_level);

	// packet functions
private:
	template <typename T>
	void		__SendPacket(network::GCOutputPacket<T>& packet);
	void		__SendPacket(network::TGCHeader header);
	void		__SendSummonPacket();
	void		__SendUpdateExpPacket();
	void		__SendUpdateLevelPacket();
	void		__SendUpdateSkillPacket(BYTE index);
	void		__SendUpdateAttrPacket(BYTE index);
	void		__SendUpdateSkillpowerPacket();
	void		__SendEvolutionInfoPacket();
	void		__SendAttrRefineInfoPacket(BYTE index);
	void		__SendUnsummonPacket();

private:
	LPCHARACTER			m_pet;
	LPITEM				m_ownerItem;

	network::TPetAdvancedTable	m_data;
	bool				m_isLoaded;
	bool				m_isSummonOnLoad;

	LPEVENT				m_updateEvent;
};
#endif
