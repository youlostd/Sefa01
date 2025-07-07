#include "stdafx.h"

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#include "item.h"
#include "desc_client.h"
#include "utils.h"
#include "affect.h"
#include "char.h"
#include "char_manager.h"
#include "vector.h"
#include "pvp.h"
#include "party.h"
#include "sectree.h"
#include "battle.h"
#include "config.h"
#include "refine.h"
#include <Eigen/Core>

template <class Cls>
std::map<DWORD, std::unique_ptr<CPetProto<Cls>>> CPetProto<Cls>::s_map_Proto;

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPetSkillProto - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPetSkillProto::CPetSkillProto(const network::TPetAdvancedSkillProto& skillProto) :
	CPetProto(skillProto.apply())
{
	// init this skill
	m_applyType = skillProto.apply();
	m_isHeroic = skillProto.is_heroic();

	UpdateValues(nullptr);
	m_functionList.AddDefaultFunctions();

	try
	{
		__InitExpr(m_valueExpr, skillProto.value_expr());
	}
	catch (ExprEval::EmptyExpressionException e)
	{
		sys_err("Cannot load pet skill %u: empty expression: %s", m_applyType, e.GetValue().c_str());
	}
	catch (ExprEval::UnmatchedParenthesisException e)
	{
		sys_err("Cannot load pet skill %u: unmatched paranthesis: %s", m_applyType, e.GetValue().c_str());
	}
	catch (ExprEval::Exception e)
	{
		sys_err("Cannot load pet skill %u: invalid parsing: %s [%u - %u]", m_applyType, e.GetValue().c_str(), e.GetStart(), e.GetEnd());
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
			if (petInfo->skills(i).vnum() == m_applyType)
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

double CPetSkillProto::GetValue()
{
	return __GetVal(m_valueExpr);
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
		sys_err("Cannot eval pet skill %u: divide by zero: %s", m_applyType, e.GetValue().c_str());
	}
	catch (ExprEval::Exception e)
	{
		sys_err("Cannot eval pet skill %u: %s", m_applyType, e.GetValue().c_str());
	}

	return 0.0;
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPetEvolveProto - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPetEvolveProto::CPetEvolveProto(const network::TPetAdvancedEvolveProto& evolveProto) :
	CPetProto(evolveProto.level())
{
	m_level = evolveProto.level();
	m_name = evolveProto.name();
	m_refine_id = evolveProto.refine_id();
	m_scale = evolveProto.scale();
	m_normal_skill_count = evolveProto.normal_skill_count();
	m_heroic_skill_count = evolveProto.heroic_skill_count();
	m_skillpower = evolveProto.skillpower();
	m_can_skillpower_reroll = evolveProto.skillpower_rerollable();
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPetAttrProto - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPetAttrProto::CPetAttrProto(const network::TPetAdvancedAttrProto& attrProto) :
	CPetProto(GetKey(attrProto.apply_type(), attrProto.apply_level()))
{
	m_type = attrProto.apply_type();
	m_value = attrProto.value();
	m_refine_id = attrProto.refine_id();
}

/*******************************************************************\
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
|||| CPetAdvanced - CLASS
|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
\*******************************************************************/

EVENTFUNC(petadvanced_update_event)
{
	auto info = static_cast<CPetAdvanced::event_info*>(event->info);
	if (!info) {
		sys_err("petsystem_update_event> <Factor> Null pointer");
		return 0;
	}

	auto pet = info->pet;
	if (!pet)
		return 0;

	pet->Update();
	return 3;
}

/*******************************************************************\
| [PUBLIC] (De-)Initialize Functions
\*******************************************************************/

CPetAdvanced::CPetAdvanced(LPITEM owner)
{
	m_pet = nullptr;
	m_ownerItem = owner;

	m_isLoaded = false;
	m_isSummonOnLoad = false;

	m_updateEvent = nullptr;
}

CPetAdvanced::~CPetAdvanced()
{
	Unsummon();

	if (m_updateEvent)
		event_cancel(&m_updateEvent);
}

void CPetAdvanced::Initialize(const std::string& name, bool can_override)
{
	if (m_isLoaded && !can_override)
	{
		sys_err("PetAdvanced: already initialized?? item_id %u", m_data.item_id());
		return;
	}

	m_data.clear_skills();
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		m_data.add_skills();

	m_data.clear_attr_type();
	m_data.clear_attr_level();
	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
	{
		m_data.add_attr_type(0);
		m_data.add_attr_level(0);
	}

	m_data.set_item_id(m_ownerItem->GetID());
	m_data.set_level(1);
	m_data.set_name(name);

	RerollAttr();

	if (GetEvolveData())
		m_data.set_skillpower(GetEvolveData()->GetSkillpower());

	m_isLoaded = true;
}

void CPetAdvanced::Initialize(const network::TPetAdvancedTable* data)
{
	if (!data)
	{
		sys_err("PetAdvanced: cannot initialize with nullptr data");
		return;
	}

	if (m_isLoaded)
	{
		sys_err("PetAdvanced: already initialized?? item_id %u", m_data.item_id());
		return;
	}

	m_data = *data;

	// reset changed, database won't overwrite changed on save but matches it with the database state if changed is false
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		m_data.mutable_skills(i)->set_changed(false);

	m_isLoaded = true;

	CheckSummonOnLoad();
}

/*******************************************************************\
| [PUBLIC] Data Functions
\*******************************************************************/
WORD CPetAdvanced::GetMaxLevel() const
{
	auto evolve = GetNextEvolveData();
	if (evolve)
		return evolve->GetLevel();

	return PET_MAX_LEVEL;
}

LPCHARACTER CPetAdvanced::GetOwner() const
{
	return GetItem()->GetOwner();
}

void CPetAdvanced::SetLevel(WORD level)
{
	level = MINMAX(1, level, GetMaxLevel());
	if (level == GetLevel())
		return;

	if (level == GetMaxLevel())
	{
		m_data.set_exp(0);
		__SendUpdateExpPacket();
	}
	
	m_data.set_level(level);
	m_ownerItem->UpdatePacket();

	__SendUpdateLevelPacket();

	if (IsSummoned())
		GetSummoned()->PointChange(POINT_LEVEL, static_cast<int>(GetLevel()) - GetSummoned()->GetLevel(), false, true);

	Save();
}

void CPetAdvanced::GiveExp(long long exp)
{
	WORD level = GetLevel();
	if (level >= GetMaxLevel())
		exp = 0;
	if (exp < 0 && GetExp() < -exp)
		exp = -GetExp();

	if (exp == 0)
		return;

	exp = GetExp() + exp;

	while (exp >= GetNextExp() && level < GetMaxLevel())
	{
		exp -= GetNextExp();
		level++;
	}

	m_data.set_exp(exp);

	SetLevel(level);
	__SendUpdateExpPacket();

	Save();
}

long long CPetAdvanced::GetNextExp() const
{
	if (GetLevel() >= PET_MAX_LEVEL)
		return 0;

	return pet_exp_table[GetLevel()];
}

CPetSkillProto* CPetAdvanced::GetSkillProto(BYTE index) const
{
	auto info = GetSkillInfo(index);
	if (info == nullptr)
		return nullptr;

	return CPetSkillProto::Get(info->vnum());
}

int CPetAdvanced::GetAttrIndexByType(BYTE attr_type) const
{
	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
	{
		if (GetAttrType(i) == attr_type)
			return i;
	}

	return -1;
}

BYTE CPetAdvanced::GetAttrType(BYTE index) const
{
	if (index >= PET_ATTR_MAX_NUM)
		return APPLY_NONE;

	return m_data.attr_type(index);
}

BYTE CPetAdvanced::GetAttrLevel(BYTE index) const
{
	if (index >= PET_ATTR_MAX_NUM)
		return 0;

	return m_data.attr_level(index);
}

int CPetAdvanced::GetAttrValue(BYTE index) const
{
	auto proto = CPetAttrProto::Get(CPetAttrProto::GetKey(GetAttrType(index), GetAttrLevel(index)));
	if (!proto)
		return 0;

	return static_cast<int>(ceilf(proto->GetValue()));
}

/*******************************************************************\
| [PUBLIC] Save Functions
\*******************************************************************/
void CPetAdvanced::Save()
{
	GetItem()->Save();
}

void CPetAdvanced::SaveDirect()
{
	if (!IsLoaded())
		return;

	network::GDOutputPacket<network::GDPetSavePacket> pack;
	*pack->mutable_data() = m_data;
	db_clientdesc->DBPacket(pack);

	// reset changed state because the database will store the changed state and won't overwrite it if it's not yet saved into the mysql db
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		m_data.mutable_skills(i)->set_changed(false);
}

void CPetAdvanced::CopyPetTable(network::TPetAdvancedTable* dest)
{
	*dest = m_data;
}

/*******************************************************************\
| [PUBLIC] General Functions
\*******************************************************************/
bool CPetAdvanced::CanSummon(bool message) const
{
	if (!IsLoaded())
	{
		sys_err("cannot summon advanced pet while not loaded");
		return false;
	}

	if (!GetOwner())
		return false;

	return true;
}

LPCHARACTER CPetAdvanced::Summon()
{
	if (!CanSummon())
		return nullptr;

	LPCHARACTER owner = GetOwner();

	if (owner->GetPetAdvanced())
	{
		if (owner->GetPetAdvanced() != this)
			owner->GetPetAdvanced()->Unsummon();
	}

	int32_t x = owner->GetX();
	int32_t y = owner->GetY();
	int32_t z = owner->GetZ();

	if (!owner->GetDungeon() && owner->GetMapIndex() < 10000) {
		x += random_number(-100, 100);
		y += random_number(-100, 100);
	}

	if (IsSummoned())
	{
		m_pet->Show(owner->GetMapIndex(), x, y);
		return m_pet;
	}

	m_pet = CHARACTER_MANAGER::instance().SpawnMob(GetItem()->GetValue(PET_ITEM_VALUE_VNUM),
		owner->GetMapIndex(),
		x, y, z,
		false,
		(int)(owner->GetRotation() + 180),
		false);

	if (!m_pet)
	{
		sys_err("CPetAdvanced: Failed to summon the pet. (vnum: %u)", GetItem()->GetValue(PET_ITEM_VALUE_VNUM));
		return nullptr;
	}

	GetItem()->Lock(true);
	ApplyBuff(true);

	owner->SetPetAdvanced(this);
	m_pet->SetPetAdvanced(this);

	m_pet->SetEmpire(owner->GetEmpire());
	m_pet->SetLevel(GetLevel());
	m_pet->SetName(GetName());
	m_pet->Show(owner->GetMapIndex(), x, y, z);

	if (m_updateEvent)
		event_cancel(&m_updateEvent);

	event_info* info = AllocEventInfo<event_info>();
	info->pet = this;
	m_updateEvent = event_create(petadvanced_update_event, info, 3);

	__SendSummonPacket();

	return m_pet;
}

void CPetAdvanced::CheckSummonOnLoad()
{
	if (m_isSummonOnLoad && IsLoaded() && GetOwner() && GetOwner()->GetDesc()->IsPhase(PHASE_GAME))
	{
		Summon();
		m_isSummonOnLoad = false;
	}
}

void CPetAdvanced::Unsummon()
{
	if (!IsSummoned())
		return;

	M2_DESTROY_CHARACTER(m_pet);
}

void CPetAdvanced::OnDestroyPet()
{
	__SendUnsummonPacket();

	Save();

	if (GetOwner())
		GetOwner()->SetPetAdvanced(nullptr);

	m_pet = nullptr;

	ApplyBuff(false);
	GetItem()->Lock(false);

	event_cancel(&m_updateEvent);
}

void CPetAdvanced::Update()
{
	if (!IsSummoned())
		return;

	__UpdateFollowAI();
}

void CPetAdvanced::ApplyBuff(bool add)
{
	__ComputeSkills(add);
	ComputeAttr(add);
}

void CPetAdvanced::ChangeName(const char* c_pszNewName)
{
	m_data.set_name(c_pszNewName);
	m_ownerItem->UpdatePacket();

	if (IsSummoned())
	{
		Unsummon();
		if (GetOwner())
			Summon();
	}
}

const CPetEvolveProto* CPetAdvanced::GetEvolveData() const
{
	CPetEvolveProto* ret = NULL;

	auto& map = CPetEvolveProto::GetMap();
	for (auto& pair : map)
	{
		auto proto = dynamic_cast<CPetEvolveProto*>(pair.second.get());
		if (proto->GetLevel() > GetLevel() || (proto->GetLevel() == GetLevel() && GetExp() == 0))
			break;

		ret = proto;
	}

	return ret;
}

const CPetEvolveProto* CPetAdvanced::GetNextEvolveData() const
{
	CPetEvolveProto* ret = NULL;

	auto& map = CPetEvolveProto::GetMap();
	for (auto& pair : map)
	{
		ret = dynamic_cast<CPetEvolveProto*>(pair.second.get());

		if (ret->GetLevel() > GetLevel() || (ret->GetLevel() == GetLevel() && GetExp() == 0))
			return ret;
	}

	return nullptr;
}

bool CPetAdvanced::CanEvolve() const
{
	return GetLevel() == GetMaxLevel() && GetExp() == 0 && GetNextEvolveData() != nullptr;
}

void CPetAdvanced::ShowEvolutionInfo()
{
	__SendEvolutionInfoPacket();
}

void CPetAdvanced::Evolve()
{
	network::GCOutputPacket<network::GCPetEvolveResultPacket> pack;
	pack->set_result(false);

	if (!CanEvolve())
	{
		__SendPacket(pack);
		return;
	}

	if (!GetOwner())
	{
		__SendPacket(pack);
		return;
	}

	// check materials
	auto evolve = GetNextEvolveData();
	auto refine = CRefineManager::Instance().GetRefineRecipe(evolve->GetRefineID());
	if (!refine)
	{
		__SendPacket(pack);
		sys_err("cannot evolve pet on level %d (no refine recipe found for id %u)", GetLevel(), evolve->GetRefineID());
		return;
	}

	if (GetOwner()->GetGold() < refine->cost())
	{
		__SendPacket(pack);
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Not enough money to evolve your pet.");
		return;
	}
	
	for (int i = 0; i < refine->material_count(); ++i)
	{
		if (GetOwner()->CountSpecifyItem(refine->materials(i).vnum()) < refine->materials(i).count())
		{
			__SendPacket(pack);
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, "Not enough material.");
			return;
		}
	}

	if (random_number(1, 100) <= refine->prob())
	{
		pack->set_result(true);

		// remove materials
		GetOwner()->PointChange(POINT_GOLD, -refine->cost());
		for (int i = 0; i < refine->material_count(); ++i)
			GetOwner()->RemoveSpecifyItem(refine->materials(i).vnum(), refine->materials(i).count());

		// evolve
		m_data.set_exp(1);
		m_data.set_skillpower(evolve->GetSkillpower());
		m_ownerItem->UpdatePacket();
		__SendUpdateExpPacket();
		__SendUpdateSkillpowerPacket();

		if (IsSummoned())
		{
			network::GCOutputPacket<network::GCUpdateCharacterScalePacket> pack;
			pack->set_vid(GetSummoned()->GetVID());
			pack->set_scale(evolve->GetScale());

			GetSummoned()->PacketAround(pack);
		}

		Save();
	}

	__SendPacket(pack);
}

void CPetAdvanced::RerollAttr()
{
	if (IsSummoned())
		ComputeAttr(false);

	std::vector<BYTE> attr_types;

	auto& map = CPetAttrProto::GetMap();
	for (auto& pair : map)
		attr_types.push_back(pair.first);
	
	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
	{
		if (attr_types.size() > 0)
		{
			auto rand_index = random_number(0, attr_types.size() - 1);
			m_data.set_attr_type(i, attr_types[rand_index]);
			m_data.set_attr_level(i, 0);

			attr_types.erase(attr_types.cbegin() + rand_index);
		}
		else
		{
			m_data.set_attr_type(i, APPLY_NONE);
			m_data.set_attr_level(i, 0);
		}

		m_ownerItem->UpdatePacket();
		__SendUpdateAttrPacket(i);
	}

	if (IsSummoned())
		ComputeAttr(true);

	Save();
}

bool CPetAdvanced::CanUpgradeAttr(BYTE index) const
{
	if (index >= PET_ATTR_MAX_NUM)
		return false;

	if (GetAttrType(index) == APPLY_NONE || CPetAttrProto::Get(CPetAttrProto::GetKey(GetAttrType(index), GetAttrLevel(index) + 1)) == nullptr)
		return false;

	return true;
}

bool CPetAdvanced::UpgradeAttr(BYTE index)
{
	if (!CanUpgradeAttr(index))
		return false;

	if (IsSummoned())
		ComputeAttr(false, index);
	m_data.set_attr_level(index, m_data.attr_level(index) + 1);
	m_ownerItem->UpdatePacket();
	if (IsSummoned())
		ComputeAttr(true, index);

	__SendUpdateAttrPacket(index);
	Save();

	return true;
}

void CPetAdvanced::ComputeAttr(bool add, int index)
{
	if (index == -1)
	{
		for (index = 0; index < PET_ATTR_MAX_NUM; ++index)
			ComputeAttr(add, index);
	}
	else
	{
		if (index < 0 || index >= PET_ATTR_MAX_NUM)
			return;

		auto attr_type = GetAttrType(index);
		auto attr_value = GetAttrValue(index);

		if (!attr_type || !attr_value)
			return;

		if (!GetOwner())
			return;

		GetOwner()->ApplyPoint(attr_type, add ? attr_value : -attr_value);
	}
}

void CPetAdvanced::ShowAttrRefineInfo(BYTE index)
{
	__SendAttrRefineInfoPacket(index);
}

void CPetAdvanced::RerollSkillpower()
{
	auto evolve = GetEvolveData();
	if (!evolve)
		return;

	auto new_skillpower = random_number(evolve->GetSkillpower(), PET_SKILLPOWER_MAX);
	if (new_skillpower == m_data.skillpower())
		return;

	m_data.set_skillpower(new_skillpower);
	__SendUpdateSkillpowerPacket();
}

BYTE CPetAdvanced::GetSkillpower() const
{
	return m_data.skillpower();
}

/*******************************************************************\
| [PUBLIC] Private Functions
\*******************************************************************/
void CPetAdvanced::__UpdateFollowAI()
{
	LPCHARACTER owner = GetOwner();
	if (!owner)
		return;

	int START_FOLLOW_DISTANCE = 400;
	int START_RUN_DISTANCE = 750;
	int RESPAWN_DISTANCE = 4500;
	int APPROACH = 250;

	bool bRun = false;

	uint32_t currentTime = get_dword_time();

	int32_t ownerX = owner->GetX();
	int32_t ownerY = owner->GetY();

	int32_t charX = m_pet->GetX();
	int32_t charY = m_pet->GetY();

	const auto dist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (dist >= RESPAWN_DISTANCE)
	{
		float fx, fy;
		GetDeltaByDegree(owner->GetRotation(), -APPROACH, &fx, &fy);

		if (m_pet->Show(owner->GetMapIndex(), ownerX + fx, ownerY + fy))
			return;
	}

	if (dist >= START_FOLLOW_DISTANCE && !m_pet->IsStateMove())
	{
		if (dist >= START_RUN_DISTANCE)
			bRun = true;

		m_pet->SetNowWalking(!bRun);

		__Follow(APPROACH);

		m_pet->SetLastAttacked(currentTime);
	}
}

void CPetAdvanced::__Follow(int32_t minDistance)
{
	LPCHARACTER owner = GetOwner();
	if (!owner)
		return;

	int32_t ownerX = owner->GetX();
	int32_t ownerY = owner->GetY();

	int32_t petX = m_pet->GetX();
	int32_t petY = m_pet->GetY();

	auto dist = DISTANCE_APPROX(ownerX - petX, ownerY - petY);
	if (dist <= minDistance)
		return;

	Eigen::Vector2f ownerPos(ownerX + random_number(-500, 500), ownerY + random_number(-500, 500));
	Eigen::Vector2f petPos(petX, petY);

	Eigen::Vector2f pos = petPos + (ownerPos - petPos).normalized() * (dist - minDistance);
	if (!m_pet->Goto(pos.x() + 0.5f, pos.y() + 0.5f))
		return;

	m_pet->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
}

/*******************************************************************\
| [PRIVATE] Skill Functions
\*******************************************************************/
bool CPetAdvanced::__SetSkill(WORD index, DWORD vnum, WORD level)
{
	if (index >= PET_SKILL_MAX_NUM)
		return false;

	auto& skill = *m_data.mutable_skills(index);
	if (skill.vnum() == vnum && skill.level() == level)
		return true;

	// every skill vnum has to be unique on the pet
	if (vnum != 0 && skill.vnum() != vnum)
	{
		for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		{
			if (i == index)
				continue;

			if (GetSkillInfo(i)->vnum() == vnum)
				return false;
		}
	}

	CPetSkillProto* proto;
	bool refreshApply = false;
	if (IsSummoned())
	{
		if (skill.vnum() != vnum)
		{
			if (skill.vnum() && (proto = dynamic_cast<CPetSkillProto*>(CPetSkillProto::Get(skill.vnum()))))
			{
				refreshApply = true;
			}
		}
		if (vnum && (proto = dynamic_cast<CPetSkillProto*>(CPetSkillProto::Get(vnum))) && (vnum != skill.vnum() || level != skill.level()))
		{
			refreshApply = true;
		}
	}

	if (refreshApply)
		__ComputeSkills(false);

	skill.set_vnum(vnum);
	skill.set_level(level);
	skill.set_changed(true);

	m_ownerItem->UpdatePacket();
	__SendUpdateSkillPacket(index);

	Save();

	if (refreshApply)
		__ComputeSkills(true);

	return true;
}

void CPetAdvanced::__ComputeSkills(bool add)
{
	if (!GetOwner())
		return;

	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
	{
		auto* info = GetSkillInfo(i);
		if (!info || info->vnum() == 0 || info->level() == 0)
			continue;

		auto* proto = dynamic_cast<CPetSkillProto*>(CPetSkillProto::Get(info->vnum()));
		if (!proto)
			continue;

		proto->UpdateValues(&m_data);

		if (proto->GetApply() != APPLY_NONE)
		{
			int value = static_cast<int>(ceilf(proto->GetValue()));
			if (value)
				GetOwner()->ApplyPoint(proto->GetApply(), add ? value : -value);
		}
	}
}

/*******************************************************************\
| [PUBLIC] Skill Functions
\*******************************************************************/
int CPetAdvanced::GetSkillIndexByVnum(DWORD vnum) const
{
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
	{
		if (GetSkillInfo(i)->vnum() == vnum)
			return i;
	}

	return -1;
}

bool CPetAdvanced::HasSkill(DWORD vnum) const
{
	return GetSkillIndexByVnum(vnum) != -1;
}

void CPetAdvanced::ClearSkill(WORD index)
{
	if (index >= PET_SKILL_MAX_NUM)
		return;

	__SetSkill(index, 0, 0);
}

void CPetAdvanced::ResetSkills()
{
	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		ClearSkill(i);
}

bool CPetAdvanced::CanSkillLevelUp(BYTE index)
{
	auto info = GetSkillInfo(index);
	if (!info)
		return false;

	if (info->vnum() == 0)
		return false;

	if (info->level() >= PET_SKILL_MAX_LEVEL)
		return false;

	return true;
}

bool CPetAdvanced::SkillLevelUp(BYTE index)
{
	if (!CanSkillLevelUp(index))
		return false;

	auto info = GetSkillInfo(index);
	__SetSkill(index, info->vnum(), info->level() + 1);

	Save();
	return true;
}

bool CPetAdvanced::CanLearnSkill(DWORD skill_vnum)
{
	if (GetSkillIndexByVnum(skill_vnum) >= 0)
		return false;

	auto evolve = GetEvolveData();
	if (!evolve)
		return false;

	auto skill = dynamic_cast<CPetSkillProto*>(CPetSkillProto::Get(skill_vnum));
	if (!skill)
		return false;

	if (!skill->IsHeroic())
	{
		DWORD cnt = 0;
		for (auto i = PET_SKILL_NORMAL_START; i < PET_SKILL_HEROIC_START; ++i)
		{
			if (GetSkillInfo(i)->vnum() != 0)
				cnt++;
		}

		if (cnt >= evolve->GetNormalSkillCount())
			return false;
	}
	else
	{
		DWORD cnt = 0;
		for (auto i = PET_SKILL_HEROIC_START; i < PET_SKILL_MAX_NUM; ++i)
		{
			if (GetSkillInfo(i)->vnum() != 0)
				cnt++;
		}

		if (cnt >= evolve->GetHeroicSkillCount())
			return false;
	}

	return true;
}

bool CPetAdvanced::LearnSkill(DWORD skill_vnum)
{
	if (!CanLearnSkill(skill_vnum))
		return false;

	int empty_idx = -1;
	auto skill = dynamic_cast<CPetSkillProto*>(CPetSkillProto::Get(skill_vnum));
	if (!skill->IsHeroic())
	{
		for (auto i = PET_SKILL_NORMAL_START; i < PET_SKILL_HEROIC_START; ++i)
		{
			if (GetSkillInfo(i)->vnum() == 0)
			{
				empty_idx = i;
				break;
			}
		}
	}
	else
	{
		for (auto i = PET_SKILL_HEROIC_START; i < PET_SKILL_MAX_NUM; ++i)
		{
			if (GetSkillInfo(i)->vnum() == 0)
			{
				empty_idx = i;
				break;
			}
		}
	}

	if (empty_idx == -1)
		return false;

	return SetSkillLevel(empty_idx, skill_vnum, 1);
}

bool CPetAdvanced::SetSkillLevel(BYTE index, DWORD skill_vnum, WORD skill_level)
{
	auto info = GetSkillInfo(index);
	if (info == NULL)
		return false;

	if (skill_vnum != skill_level && (skill_vnum == 0 || skill_level == 0))
		return false;

	if (skill_vnum && CPetSkillProto::Get(skill_vnum) == NULL)
		return false;

	if (!__SetSkill(index, skill_vnum, skill_level))
		return false;

	Save();
	return true;
}

/*******************************************************************\
| [PRIVATE] Packet Functions
\*******************************************************************/
template <typename T>
void CPetAdvanced::__SendPacket(network::GCOutputPacket<T>& packet)
{
	if (!IsSummoned() || !GetOwner())
		return;

	GetOwner()->GetDesc()->Packet(packet);
}

void CPetAdvanced::__SendPacket(network::TGCHeader header)
{
	if (!IsSummoned() || !GetOwner())
		return;

	GetOwner()->GetDesc()->Packet(header);
}

void CPetAdvanced::__SendSummonPacket()
{
	network::GCOutputPacket<network::GCPetSummonPacket> pack;
	pack->set_vid(m_pet->GetVID());
	pack->set_item_vnum(m_ownerItem->GetVnum());

	*pack->mutable_pet() = m_data;
	pack->set_next_exp(GetNextExp());

	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
		pack->mutable_pet()->add_attr_can_upgrade(CanUpgradeAttr(i));

	__SendPacket(pack);

	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
		__SendUpdateAttrPacket(i);
}

void CPetAdvanced::__SendUpdateExpPacket()
{
	network::GCOutputPacket<network::GCPetUpdateExpPacket> pack;
	pack->set_exp(GetExp());
	__SendPacket(pack);
}

void CPetAdvanced::__SendUpdateLevelPacket()
{
	network::GCOutputPacket<network::GCPetUpdateLevelPacket> pack;
	pack->set_level(GetLevel());
	pack->set_next_exp(GetNextExp());
	__SendPacket(pack);
}

void CPetAdvanced::__SendUpdateSkillPacket(BYTE index)
{
	auto info = GetSkillInfo(index);
	if (info == nullptr)
		return;

	network::GCOutputPacket<network::GCPetUpdateSkillPacket> pack;
	pack->set_index(index);
	*pack->mutable_skill() = *info;

	__SendPacket(pack);
}

void CPetAdvanced::__SendUpdateAttrPacket(BYTE index)
{
	network::GCOutputPacket<network::GCPetUpdateAttrPacket> pack;
	pack->set_index(index);
	pack->set_type(GetAttrType(index));
	pack->set_level(GetAttrLevel(index));
	pack->set_value(GetAttrValue(index));
	pack->set_can_upgrade(CanUpgradeAttr(index));

	__SendPacket(pack);
}

void CPetAdvanced::__SendUpdateSkillpowerPacket()
{
	network::GCOutputPacket<network::GCPetUpdateSkillpowerPacket> pack;
	pack->set_power(GetSkillpower());

	__SendPacket(pack);
}

void CPetAdvanced::__SendEvolutionInfoPacket()
{
	auto evolve = GetNextEvolveData();
	if (!evolve)
		return;

	auto refine = CRefineManager::Instance().GetRefineRecipe(evolve->GetRefineID());
	if (!refine)
		return;

	network::GCOutputPacket<network::GCPetEvolutionInfoPacket> pack;
	pack->set_cost(refine->cost());
	pack->set_prob(refine->prob());
	for (int i = 0; i < refine->material_count(); ++i)
		*pack->add_materials() = refine->materials(i);

	__SendPacket(pack);
}

void CPetAdvanced::__SendAttrRefineInfoPacket(BYTE index)
{
	if (!CanUpgradeAttr(index))
		return;

	auto attr = CPetAttrProto::Get(CPetAttrProto::GetKey(GetAttrType(index), GetAttrLevel(index) + 1));
	auto refine = CRefineManager::Instance().GetRefineRecipe(attr->GetRefineID());
	if (!refine)
		return;

	network::GCOutputPacket<network::GCPetAttrRefineInfoPacket> pack;
	pack->set_index(index);
	pack->set_cost(refine->cost());
	for (int i = 0; i < refine->material_count(); ++i)
		*pack->add_materials() = refine->materials(i);

	__SendPacket(pack);
}

void CPetAdvanced::__SendUnsummonPacket()
{
	__SendPacket(network::TGCHeader::PET_UNSUMMON);
}

#endif
