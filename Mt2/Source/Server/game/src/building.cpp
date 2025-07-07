#include "stdafx.h"
#include "constants.h"
#include "sectree_manager.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "config.h"
#include "packet.h"
#include "char.h"
#include "char_manager.h"
#include "guild.h"
#include "guild_manager.h"
#include "desc.h"
#include "desc_manager.h"
#include "desc_client.h"
#include "questmanager.h"
#include "building.h"

enum
{
	// ADD_SUPPLY_BUILDING
	BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL = 14061,
	BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM = 14062,
	BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE = 14063,
	// END_OF_ADD_SUPPLY_BUILDING

	FLAG_VNUM = 14200,
	WALL_DOOR_VNUM	= 14201,
	WALL_BACK_VNUM	= 14202,
	WALL_LEFT_VNUM	= 14203,
	WALL_RIGHT_VNUM	= 14204,
};

using namespace building;

CObject::CObject(const network::TBuildingObject* pData, network::TBuildingObjectProto* pProto)
	: m_pProto(pProto), m_dwVID(0), m_chNPC(NULL)
{
	CEntity::Initialize(ENTITY_OBJECT);

	m_data = *pData;
}

CObject::~CObject()
{
	Destroy();
}

void CObject::Destroy()
{
	if (m_pProto)
	{
		SECTREE_MANAGER::instance().ForAttrRegion(GetMapIndex(),
				GetX() + m_pProto->region(0),
				GetY() + m_pProto->region(1),
				GetX() + m_pProto->region(2),
				GetY() + m_pProto->region(3),
				(long)m_data.z_rot(), // ADD_BUILDING_ROTATION
				ATTR_OBJECT,
				ATTR_REGION_MODE_REMOVE);
	}

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

	// <Factor> NPC should be destroyed in CHARACTER_MANAGER
	// BUILDING_NPC
	/*
	if (m_chNPC) {
		M2_DESTROY_CHARACTER(m_chNPC);
	}
	*/

	RemoveSpecialEffect();
	// END_OF_BUILDING_NPC
}

// BUILDING_NPC
void CObject::Reconstruct(DWORD dwVnum)
{
	const TMapRegion * r = SECTREE_MANAGER::instance().GetMapRegion(m_data.map_index());
	if (!r)
		return;

	CLand* pLand = GetLand();
	pLand->RequestDeleteObject(GetID());
	pLand->RequestCreateObject(dwVnum, m_data.map_index(), m_data.x() - r->sx, m_data.y() - r->sy,
		m_data.x_rot(), m_data.y_rot(), m_data.z_rot(), false);
}
// END_OF_BUILDING_NPC

void CObject::EncodeInsertPacket(LPENTITY entity)
{
	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;
	
	sys_log(0, "ObjectInsertPacket vid %u vnum %u rot %f %f %f",
			m_dwVID, m_data.vnum(), m_data.x_rot(), m_data.y_rot(), m_data.z_rot());

	network::GCOutputPacket<network::GCCharacterAddPacket> pack;

	pack->set_vid(m_dwVID);
	pack->set_type(CHAR_TYPE_BUILDING);
	pack->set_angle(m_data.z_rot());
	pack->set_x(GetX());
	pack->set_y(GetY());
	pack->set_race_num(m_data.vnum());

	// ºôµù È¸Àü Á¤º¸(º®ÀÏ¶§´Â ¹® À§Ä¡)¸¦ º¯È¯
	pack->add_affect_flags(unsigned(m_data.x_rot()));
	pack->add_affect_flags(unsigned(m_data.y_rot()));


	if (GetLand())
	{
		// pack->set_guild(GetLand()->GetOwner());
	}

	d->Packet(pack);
}

void CObject::EncodeRemovePacket(LPENTITY entity)
{
	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	sys_log(!test_server, "ObjectRemovePacket vid %u", m_dwVID);

	network::GCOutputPacket<network::GCCharacterDeletePacket> pack;
	pack->set_vid(m_dwVID);

	d->Packet(pack);
}

void CObject::SetVID(DWORD dwVID)
{
	m_dwVID = dwVID;
}

bool CObject::Show(long lMapIndex, long x, long y)
{
	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d mapindex %d id %u vnum %u", x, y, lMapIndex, GetID(), GetVnum());
		return false;
	}

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
	}

	m_data.set_map_index(lMapIndex);
	m_data.set_x(x);
	m_data.set_y(y);

	Save();

	SetMapIndex(lMapIndex);
	SetXYZ(x, y, 0);

	tree->InsertEntity(this);
	UpdateSectree();

	SECTREE_MANAGER::instance().ForAttrRegion(lMapIndex,
			x + m_pProto->region(0),
			y + m_pProto->region(1),
			x + m_pProto->region(2),
			y + m_pProto->region(3),
			(long)m_data.z_rot(),
			ATTR_OBJECT,
			ATTR_REGION_MODE_SET);

	return true;
}

void CObject::Save()
{
}

void CObject::ApplySpecialEffect()
{
	if (m_pProto)
	{
		// ADD_SUPPLY_BUILDING
		if (m_pProto->vnum() == BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL ||
			m_pProto->vnum() == BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM ||
			m_pProto->vnum() == BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE)
		{
			CLand* pLand = GetLand();
			DWORD guild_id = 0;
			if (pLand)
				guild_id = pLand->GetOwner();
			CGuild* pGuild = CGuildManager::instance().FindGuild(guild_id);
			if (pGuild)
			{
				switch (m_pProto->vnum())
				{
					case BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL:
						pGuild->SetMemberCountBonus(6);
						break;
					case BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM:
						pGuild->SetMemberCountBonus(12);
						break;
					case BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE:
						pGuild->SetMemberCountBonus(18);
						break;
				}
				if (map_allow_find(pLand->GetMapIndex()))
				{
					pGuild->BroadcastMemberCountBonus();
				}
			}
		}
		// END_OF_ADD_SUPPLY_BUILDING
	}
}

void CObject::RemoveSpecialEffect()
{
	if (m_pProto)
	{
		// ADD_SUPPLY_BUILDING
		if (m_pProto->vnum() == BUILDING_INCREASE_GUILD_MEMBER_COUNT_SMALL ||
			m_pProto->vnum() == BUILDING_INCREASE_GUILD_MEMBER_COUNT_MEDIUM ||
			m_pProto->vnum() == BUILDING_INCREASE_GUILD_MEMBER_COUNT_LARGE)
		{
			CLand* pLand = GetLand();
			DWORD guild_id = 0;
			if (pLand)
				guild_id = pLand->GetOwner();
			CGuild* pGuild = CGuildManager::instance().FindGuild(guild_id);
			if (pGuild)
			{
				pGuild->SetMemberCountBonus(0);
				if (map_allow_find(pLand->GetMapIndex()))
					pGuild->BroadcastMemberCountBonus();
			}
		}
		// END_OF_ADD_SUPPLY_BUILDING
	}
}

// BUILDING_NPC
void CObject::RegenNPC()
{
	if (!m_pProto)
		return;

	if (!m_pProto->npc_vnum())
		return;

	if (!m_pkLand)
		return;

	DWORD dwGuildID = m_pkLand->GetOwner();
	CGuild* pGuild = CGuildManager::instance().FindGuild(dwGuildID);

	if (!pGuild)
		return;

	int x = m_pProto->npc_x();
	int y = m_pProto->npc_y();
	int newX, newY;

	float rot = m_data.z_rot() * 2.0f * M_PI / 360.0f;

	newX = (int)(( x * cosf(rot)) + ( y * sinf(rot)));
	newY = (int)(( y * cosf(rot)) - ( x * sinf(rot)));
	
	m_chNPC = CHARACTER_MANAGER::instance().SpawnMob(m_pProto->npc_vnum(),
			GetMapIndex(),
			GetX() + newX,
			GetY() + newY,
			GetZ(),
			false,
			(int)m_data.z_rot());


	if (!m_chNPC)
	{
		sys_err("Cannot create guild npc (%d, %i, %i)", GetMapIndex(), GetX() + newX, GetY() + newY);
		return;
	}

	m_chNPC->SetGuild(pGuild);

	// ÈûÀÇ ½ÅÀüÀÏ °æ¿ì ±æµå ·¹º§À» ±æ¸¶¿¡°Ô ÀúÀåÇØ³õ´Â´Ù
	if (m_pProto->vnum() == 14061 || m_pProto->vnum() == 14062 || m_pProto->vnum() == 14063)
	{
		quest::PC* pPC = quest::CQuestManager::instance().GetPC(pGuild->GetMasterPID());

		if ( pPC != NULL )
		{
			pPC->SetFlag("alter_of_power.build_level", pGuild->GetLevel());
		}
	}
}
// END_OF_BUILDING_NPC

////////////////////////////////////////////////////////////////////////////////////

CLand::CLand(const network::TBuildingLand* pData)
{
	m_data = *pData;
}

CLand::~CLand()
{
	Destroy();
}

void CLand::Destroy()
{
	itertype(m_map_pkObject) it = m_map_pkObject.begin();

	while (it != m_map_pkObject.end())
	{
		LPOBJECT pkObj = (it++)->second;
		CManager::instance().UnregisterObject(pkObj);
		M2_DELETE(pkObj);
	}

	m_map_pkObject.clear();
	m_map_pkObjectByVID.clear();
}

const network::TBuildingLand& CLand::GetData()
{
	return m_data;
}

void CLand::PutData(const network::TBuildingLand* data)
{
	m_data = *data;

	if (m_data.guild_id())
	{
		const TMapRegion* r = SECTREE_MANAGER::instance().GetMapRegion(m_data.map_index());

		if (r)
		{
			CharacterVectorInteractor i;

			if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(20040, i))
			{
				CharacterVectorInteractor::iterator it = i.begin();

				while (it != i.end())
				{
					LPCHARACTER ch = *(it++);

					if (ch->GetMapIndex() != m_data.map_index())
						continue;

					int x = ch->GetX() - r->sx;
					int y = ch->GetY() - r->sy;

					if (x > m_data.x() + m_data.width() || x < m_data.x())
						continue;

					if (y > m_data.y() + m_data.height() || y < m_data.y())
						continue;

					M2_DESTROY_CHARACTER(ch);
				}
			}
		}
	}
}

void CLand::InsertObject(LPOBJECT pkObj)
{
	m_map_pkObject.insert(std::make_pair(pkObj->GetID(), pkObj));
	m_map_pkObjectByVID.insert(std::make_pair(pkObj->GetVID(), pkObj));

	pkObj->SetLand(this);
}

LPOBJECT CLand::FindObject(DWORD dwID)
{
	std::map<DWORD, LPOBJECT>::iterator it = m_map_pkObject.find(dwID);

	if (it == m_map_pkObject.end())
		return NULL;

	return it->second;
}

LPOBJECT CLand::FindObjectByGroup(DWORD dwGroupVnum)
{
	std::map<DWORD, LPOBJECT>::iterator it;
	for (it = m_map_pkObject.begin(); it != m_map_pkObject.end(); ++it)
	{
		LPOBJECT pObj = it->second;
		if (pObj->GetGroup() == dwGroupVnum)
			return pObj;
	}

	return NULL;
}

LPOBJECT CLand::FindObjectByVnum(DWORD dwVnum)
{
	std::map<DWORD, LPOBJECT>::iterator it;
	for (it = m_map_pkObject.begin(); it != m_map_pkObject.end(); ++it)
	{
		LPOBJECT pObj = it->second;
		if (pObj->GetVnum() == dwVnum)
			return pObj;
	}

	return NULL;
}

// BUILDING_NPC
LPOBJECT CLand::FindObjectByNPC(LPCHARACTER npc)
{
	if (!npc)
		return NULL;

	std::map<DWORD, LPOBJECT>::iterator it;
	for (it = m_map_pkObject.begin(); it != m_map_pkObject.end(); ++it)
	{
		LPOBJECT pObj = it->second;
		if (pObj->GetNPC() == npc)
			return pObj;
	}

	return NULL;
}
// END_OF_BUILDING_NPC

LPOBJECT CLand::FindObjectByVID(DWORD dwVID)
{
	std::map<DWORD, LPOBJECT>::iterator it = m_map_pkObjectByVID.find(dwVID);

	if (it == m_map_pkObjectByVID.end())
		return NULL;

	return it->second;
}

void CLand::DeleteObject(DWORD dwID)
{
	LPOBJECT pkObj;

	if (!(pkObj = FindObject(dwID)))
		return;

	sys_log(0, "Land::DeleteObject %u", dwID);
	CManager::instance().UnregisterObject(pkObj);
	M2_DESTROY_CHARACTER (pkObj->GetNPC());

	m_map_pkObject.erase(dwID);
	m_map_pkObjectByVID.erase(dwID);

	M2_DELETE(pkObj);
}

struct FIsIn
{
	long sx, sy;
	long ex, ey;
	
	bool bIn;
	FIsIn (	long sx_, long sy_, long ex_, long ey_)
		: sx(sx_), sy(sy_), ex(ex_), ey(ey_), bIn(false) 
	{}

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch->IsMonster())
			{
				return;
			}
			if (sx <= ch->GetX() && ch->GetX() <= ex
				&& sy <= ch->GetY() && ch->GetY() <= ey)
			{
				bIn = true;
			}
		}
	}
};

bool CLand::RequestCreateObject(DWORD dwVnum, long lMapIndex, long x, long y, float xRot, float yRot, float zRot, bool checkAnother)
{
	SECTREE_MANAGER& rkSecTreeMgr = SECTREE_MANAGER::instance();
	auto* pkProto = CManager::instance().GetObjectProto(dwVnum);

	if (!pkProto)
	{
		sys_err("Invalid Object vnum %u", dwVnum);
		return false;
	}
	const TMapRegion * r = rkSecTreeMgr.GetMapRegion(lMapIndex);
	if (!r)
		return false;

	sys_log(0, "RequestCreateObject(vnum=%u, map=%d, pos=(%d,%d), rot=(%.1f,%.1f,%.1f) region(%d,%d ~ %d,%d)", 
			dwVnum, lMapIndex, x, y, xRot, yRot, zRot, r->sx, r->sy, r->ex, r->ey);

	x += r->sx;
	y += r->sy;

	int sx = r->sx + m_data.x();
	int ex = sx + m_data.width();
	int sy = r->sy + m_data.y();
	int ey = sy + m_data.height();

	int osx = x + pkProto->region(0);
	int osy = y + pkProto->region(1);
	int oex = x + pkProto->region(2);
	int oey = y + pkProto->region(3);

	float rad = zRot * 2.0f * M_PI / 360.0f;

	int tsx = (int)(pkProto->region(0) * cosf(rad) + pkProto->region(1) * sinf(rad) + x);
	int tsy = (int)(pkProto->region(0) * -sinf(rad) + pkProto->region(1) * cosf(rad) + y);

	int tex = (int)(pkProto->region(2) * cosf(rad) + pkProto->region(3) * sinf(rad) + x);
	int tey = (int)(pkProto->region(2) * -sinf(rad) + pkProto->region(3) * cosf(rad) + y);

	if (tsx < sx || tex > ex || tsy < sy || tey > ey)
	{
		sys_err("invalid position: object is outside of land region\nLAND: %d %d ~ %d %d\nOBJ: %d %d ~ %d %d", sx, sy, ex, ey, osx, osy, oex, oey);
		return false;
	}

	// ADD_BUILDING_ROTATION
	if ( checkAnother )
	{
		if (rkSecTreeMgr.ForAttrRegion(lMapIndex, osx, osy, oex, oey, (long)zRot, ATTR_OBJECT, ATTR_REGION_MODE_CHECK))
		{
			sys_err("another object already exist");
			return false;
		}
		FIsIn f (osx, osy, oex, oey);
		rkSecTreeMgr.GetMap(lMapIndex)->for_each(f);
		
		if (f.bIn)
		{
			sys_err("another object already exist");
			return false;
		}
	}
	// END_OF_BUILDING_NPC

	network::GDOutputPacket<network::GDCreateObjectPacket> p;
	p->set_vnum(dwVnum);
	p->set_land_id(m_data.id());
	p->set_map_index(lMapIndex);
	p->set_x(x);
	p->set_y(y);
	p->set_rot_x(xRot);
	p->set_rot_y(yRot);
	p->set_rot_z(zRot);

	db_clientdesc->DBPacket(p);
	return true;
}

void CLand::RequestDeleteObject(DWORD dwID)
{
	if (!FindObject(dwID))
	{
		sys_err("no object by id %u", dwID);
		return;
	}

	network::GDOutputPacket<network::GDDeleteObjectPacket> p;
	p->set_id(dwID);
	db_clientdesc->DBPacket(p);
	sys_log(0, "RequestDeleteObject id %u", dwID);
}

void CLand::RequestDeleteObjectByVID(DWORD dwVID)
{
	LPOBJECT pkObj;

	if (!(pkObj = FindObjectByVID(dwVID)))
	{
		sys_err("no object by vid %u", dwVID);
		return;
	}

	network::GDOutputPacket<network::GDDeleteObjectPacket> p;
	p->set_id(pkObj->GetID());
	db_clientdesc->DBPacket(p);
	sys_log(0, "RequestDeleteObject vid %u id %u", dwVID, p->id());
}

void CLand::SetOwner(DWORD dwGuild)
{
	if (m_data.guild_id() != dwGuild)
	{
		m_data.set_guild_id(dwGuild);
		RequestUpdate(dwGuild);
	}
}

void CLand::RequestUpdate(DWORD dwGuild)
{
	network::GDOutputPacket<network::GDUpdateLandPacket> p;
	p->set_guild_id(dwGuild);
	p->set_land_id(GetID());

	db_clientdesc->DBPacket(p);
	sys_log(0, "RequestUpdate id %u guild %u", GetID(), dwGuild);
}

////////////////////////////////////////////////////////////////////////////////////

CManager::CManager()
{
}

CManager::~CManager()
{
	Destroy();
}

void CManager::Destroy()
{
	itertype(m_map_pkLand) it = m_map_pkLand.begin();
	for ( ; it != m_map_pkLand.end(); ++it) {
		M2_DELETE(it->second);
	}
	m_map_pkLand.clear();
}

bool CManager::LoadObjectProto(const ::google::protobuf::RepeatedPtrField<network::TBuildingObjectProto>& table) // from DB
{
	m_vec_kObjectProto.reserve(table.size());
	for (auto& t : table)
		m_vec_kObjectProto.push_back(t);

	for (auto& r : m_vec_kObjectProto)
	{
		// BUILDING_NPC
		sys_log(0, "ObjectProto %u price %lld upgrade %u upg_limit %u life %d NPC %u",
				r.vnum(), r.price(), r.upgrade_vnum(), r.upgrade_limit_time(), r.life(), r.npc_vnum());
		// END_OF_BUILDING_NPC

		for (int j = 0; j < OBJECT_MATERIAL_MAX_NUM; ++j)
		{
			if (r.materials_size() < j)
				r.add_materials();

			if (!r.materials(j).item_vnum())
				break;

			if (NULL == ITEM_MANAGER::instance().GetTable(r.materials(j).item_vnum()))
			{
				sys_err("          mat: ERROR!! no item by vnum %u", r.materials(j).item_vnum());
				return false;
			}

			sys_log(0, "          mat: %u %u", r.materials(j).item_vnum(), r.materials(j).count());
		}

		m_map_pkObjectProto.insert(std::make_pair(r.vnum(), &r));
	}

	return true;
}

network::TBuildingObjectProto* CManager::GetObjectProto(DWORD dwVnum)
{
	itertype(m_map_pkObjectProto) it = m_map_pkObjectProto.find(dwVnum);

	if (it == m_map_pkObjectProto.end())
		return NULL;

	return it->second;
}

bool CManager::LoadLand(const network::TBuildingLand * pTable) // from DB
{
	// MapAllow¿¡ ¾ø´Â ¸ÊÀÇ ¶¥ÀÏÁö¶óµµ load¸¦ ÇØ¾ßÇÑ´Ù.
	//	°Ç¹°(object)ÀÌ ¾î´À ±æµå¿¡ ¼ÓÇØ ÀÖ´ÂÁö ¾Ë±â À§ÇØ¼­´Â °Ç¹°ÀÌ ¼¼À§Áø ¶¥ÀÌ ¾î´À ±æµå ¼Ò¼ÓÀÎÁö ¾Ë¾ÆÇÑ´Ù.
	//	¸¸¾à ¶¥À» loadÇØ ³õÁö ¾ÊÀ¸¸é ±æµå °Ç¹°ÀÌ ¾î´À ±æµå¿¡ ¼Ò¼ÓµÈ °ÇÁö ¾ËÁö ¸øÇØ¼­
	//	±æµå °Ç¹°¿¡ ÀÇÇÑ ±æµå ¹öÇÁ¸¦ ¹ÞÁö ¸øÇÑ´Ù.
	//if (!map_allow_find(pTable->lMapIndex))
	//	return false;

	CLand * pkLand = M2_NEW CLand(pTable);
	m_map_pkLand.insert(std::make_pair(pkLand->GetID(), pkLand));

	sys_log(0, "LAND: %u map %d %dx%d w %u h %u",
			pTable->id(), pTable->map_index(), pTable->x(), pTable->y(), pTable->width(), pTable->height());

	return true;
}

void CManager::UpdateLand(const network::TBuildingLand* pTable)
{
	CLand* pkLand = FindLand(pTable->id());

	if (!pkLand)
	{
		sys_err("cannot find land by id %u", pTable->id());
		return;
	}

	pkLand->PutData(pTable);

	const DESC_MANAGER::DESC_SET & cont = DESC_MANAGER::instance().GetClientSet();

	itertype(cont) it = cont.begin();

	network::GCOutputPacket<network::GCLandListPacket> p;
	*p->add_lands() = *pTable;

	sys_log(0, "BUILDING: UpdateLand %u pos %dx%d guild %u", pTable->id(), pTable->x(), pTable->y(), pTable->guild_id());

	CGuild* guild = CGuildManager::instance().FindGuild(pTable->guild_id());
	while (it != cont.end())
	{
		LPDESC d = *(it++);

		if (d->GetCharacter() && d->GetCharacter()->GetMapIndex() == pTable->map_index())
		{
			// we must send the guild name first
			d->GetCharacter()->SendGuildName(guild);
			d->Packet(p);
		}
	}
}

CLand * CManager::FindLand(DWORD dwID)
{
	std::map<DWORD, CLand *>::iterator it = m_map_pkLand.find(dwID);

	if (it == m_map_pkLand.end())
		return NULL;

	return it->second;
}

CLand * CManager::FindLand(long lMapIndex, long x, long y)
{
	sys_log(0, "BUILDING: FindLand %d %d %d", lMapIndex, x, y);

	const TMapRegion * r = SECTREE_MANAGER::instance().GetMapRegion(lMapIndex);

	if (!r)
		return NULL;

	x -= r->sx;
	y -= r->sy;

	itertype(m_map_pkLand) it = m_map_pkLand.begin();

	while (it != m_map_pkLand.end())
	{
		CLand * pkLand = (it++)->second;
		auto& r = pkLand->GetData();

		if (r.map_index() != lMapIndex)
			continue;

		if (x < r.x() || y < r.y())
			continue;

		if (x > r.x() + r.width() || y > r.y() + r.height())
			continue;

		return pkLand;
	}

	return NULL;
}

CLand * CManager::FindLandByGuild(DWORD GID)
{
	itertype(m_map_pkLand) it = m_map_pkLand.begin();

	while (it != m_map_pkLand.end())
	{
		CLand * pkLand = (it++)->second;

		if (pkLand->GetData().guild_id() == GID)
			return pkLand;
	}

	return NULL;
}

bool CManager::LoadObject(const network::TBuildingObject* pTable, bool isBoot) // from DB
{
	CLand * pkLand = FindLand(pTable->land_id());

	if (!pkLand)
	{
		sys_log(0, "Cannot find land by id %u", pTable->land_id());
		return false;
	}

	auto pkProto = GetObjectProto(pTable->vnum());

	if (!pkProto)
	{
		sys_err("Cannot find object %u in prototype (id %u)", pTable->vnum(), pTable->id());
		return false;
	}

	sys_log(0, "OBJ: id %u vnum %u map %d pos %dx%d", pTable->id(), pTable->vnum(), pTable->map_index(), pTable->x(), pTable->y());

	LPOBJECT pkObj = M2_NEW CObject(pTable, pkProto);

	DWORD dwVID = CHARACTER_MANAGER::instance().AllocVID();
	pkObj->SetVID(dwVID);

	m_map_pkObjByVID.insert(std::make_pair(dwVID, pkObj));
	m_map_pkObjByID.insert(std::make_pair(pTable->id(), pkObj));

	pkLand->InsertObject(pkObj);

	if (!isBoot)
		pkObj->Show(pTable->map_index(), pTable->x(), pTable->y());
	else
	{
		pkObj->SetMapIndex(pTable->map_index());
		pkObj->SetXYZ(pTable->x(), pTable->y(), 0);
	}

	// BUILDING_NPC
	if (!isBoot)
	{
		if (pkProto->npc_vnum())
			pkObj->RegenNPC();

		pkObj->ApplySpecialEffect();
	}
	// END_OF_BUILDING_NPC

	return true;
}

void CManager::FinalizeBoot()
{
	itertype(m_map_pkObjByID) it = m_map_pkObjByID.begin();

	while (it != m_map_pkObjByID.end())
	{
		LPOBJECT pkObj = (it++)->second;

		pkObj->Show(pkObj->GetMapIndex(), pkObj->GetX(), pkObj->GetY());
		// BUILDING_NPC
		pkObj->RegenNPC();
		pkObj->ApplySpecialEffect();
		// END_OF_BUILDING_NPC
	}

	// BUILDING_NPC
	sys_log(0, "FinalizeBoot");
	// END_OF_BUILDING_NPC

	itertype(m_map_pkLand) it2 = m_map_pkLand.begin();

	while (it2 != m_map_pkLand.end())
	{
		CLand * pkLand = (it2++)->second;

		auto& r = pkLand->GetData();

		// LAND_MASTER_LOG
		sys_log(0, "LandMaster map_index=%d pos=(%d, %d)", r.map_index(), r.x(), r.y());
		// END_OF_LAND_MASTER_LOG

		if (r.guild_id() != 0)
			continue;

		if (!map_allow_find(r.map_index()))
			continue;

		const TMapRegion * region = SECTREE_MANAGER::instance().GetMapRegion(r.map_index());
		if (!region)
			continue;

		CHARACTER_MANAGER::instance().SpawnMob(20040, r.map_index(), region->sx + r.x() + (r.width() / 2), region->sy + r.y() + (r.height() / 2), 0);
	}
}

void CManager::DeleteObject(DWORD dwID) // from DB
{
	sys_log(0, "OBJ_DEL: %u", dwID);

	itertype(m_map_pkObjByID) it = m_map_pkObjByID.find(dwID);

	if (it == m_map_pkObjByID.end())
		return;

	it->second->GetLand()->DeleteObject(dwID);
}

LPOBJECT CManager::FindObjectByVID(DWORD dwVID)
{
	itertype(m_map_pkObjByVID) it = m_map_pkObjByVID.find(dwVID);

	if (it == m_map_pkObjByVID.end())
		return NULL;

	return it->second;
}

void CManager::UnregisterObject(LPOBJECT pkObj)
{
	m_map_pkObjByID.erase(pkObj->GetID());
	m_map_pkObjByVID.erase(pkObj->GetVID());
}

void CManager::SendLandList(LPDESC d, long lMapIndex)
{
	auto it = m_map_pkLand.begin();

	network::GCOutputPacket<network::GCLandListPacket> p;
	while (it != m_map_pkLand.end())
	{
		CLand * pkLand = (it++)->second;
		auto& r = pkLand->GetData();

		if (r.map_index() != lMapIndex)
			continue;

		//
		LPCHARACTER ch  = d->GetCharacter();
		if (ch)
		{
			CGuild *guild = CGuildManager::instance().FindGuild(r.guild_id());
			ch->SendGuildName(guild);
		}
		//

		*p->add_lands() = r;
	}

	sys_log(0, "SendLandList map %d elem_size: %d", lMapIndex, p->lands_size());

	if (p->lands_size() != 0)
		d->Packet(p);
}

// LAND_CLEAR
void CManager::ClearLand(DWORD dwLandID)
{
	CLand* pLand = FindLand(dwLandID);

	if ( pLand == NULL )
	{
		sys_log(0, "LAND_CLEAR: there is no LAND id like %d", dwLandID);
		return;
	}

	pLand->ClearLand();

	sys_log(0, "LAND_CLEAR: request Land Clear. LandID: %d", pLand->GetID());
}

void CManager::ClearLandByGuildID(DWORD dwGuildID)
{
	CLand* pLand = FindLandByGuild(dwGuildID);

	if ( pLand == NULL )
	{
		sys_log(0, "LAND_CLEAR: there is no GUILD id like %d", dwGuildID);
		return;
	}

	pLand->ClearLand();

	sys_log(0, "LAND_CLEAR: request Land Clear. LandID: %d", pLand->GetID());
}

void CLand::ClearLand()
{
	itertype(m_map_pkObject) iter = m_map_pkObject.begin();

	while ( iter != m_map_pkObject.end() )
	{
		RequestDeleteObject(iter->second->GetID());
		iter++;
	}

	SetOwner(0);

	auto& r = GetData();
	const TMapRegion* region = SECTREE_MANAGER::instance().GetMapRegion(r.map_index());

	CHARACTER_MANAGER::instance().SpawnMob(20040, r.map_index(), region->sx + r.x() + (r.width() / 2), region->sy + r.y() + (r.height() / 2), 0);
}
// END_LAND_CLEAR

// BUILD_WALL
void CLand::DrawWall(DWORD dwVnum, long nMapIndex, long& x, long& y, char length, float zRot)
{
	int rot = (int)zRot;
	rot = ((rot%360) / 90) * 90;

	int dx=0, dy=0;

	switch ( rot )
	{
		case 0 :
			dx = -500;
			dy = 0;
			break;

		case 90 :
			dx = 0;
			dy = 500;
			break;

		case 180 :
			dx = 500;
			dy = 0;
			break;

		case 270 :
			dx = 0;
			dy = -500;
			break;
	}

	for ( int i=0; i < length; i++ )
	{
		this->RequestCreateObject(dwVnum, nMapIndex, x, y, 0, 0, rot, false);
		x += dx;
		y += dy;
	}
}


bool CLand::RequestCreateWall(long nMapIndex, float rot)
{
	const bool 	WALL_ANOTHER_CHECKING_ENABLE = false;

	auto& land = GetData();

	int center_x = land.x() + land.width()  / 2;
	int center_y = land.y() + land.height() / 2;

	int wall_x = center_x;
	int wall_y = center_y;
	int wall_half_w = 1000;
	int wall_half_h = 1362;

	if (rot == 0.0f) 		// ³²ÂÊ ¹®
	{
		int door_x = wall_x;
		int door_y = wall_y + wall_half_h;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x, wall_y + wall_half_h, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x, wall_y - wall_half_h, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x - wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x + wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}
	else if (rot == 180.0f)		// ºÏÂÊ ¹®
	{
		int door_x = wall_x;
		int door_y = wall_y - wall_half_h;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x, wall_y - wall_half_h, door_x, door_y, 180.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x, wall_y + wall_half_h, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x - wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x + wall_half_w, wall_y, door_x, door_y,   0.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}
	else if (rot == 90.0f)		// µ¿ÂÊ ¹®
	{
		int door_x = wall_x + wall_half_h;
		int door_y = wall_y;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x + wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x - wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x, wall_y - wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x, wall_y + wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}
	else if (rot == 270.0f)		// ¼­ÂÊ ¹®
	{
		int door_x = wall_x - wall_half_h;
		int door_y = wall_y;
		RequestCreateObject(WALL_DOOR_VNUM,	nMapIndex, wall_x - wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_BACK_VNUM,	nMapIndex, wall_x + wall_half_h, wall_y, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_LEFT_VNUM,	nMapIndex, wall_x, wall_y - wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(WALL_RIGHT_VNUM,	nMapIndex, wall_x, wall_y + wall_half_w, door_x, door_y,  90.0f, WALL_ANOTHER_CHECKING_ENABLE);
	}

	if (test_server)
	{
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x() + 50, 				land.y() + 50, 0, 0, 0.0, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x() + land.width() - 50,	land.y() + 50, 0, 0, 90.0, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x() + land.width() - 50,	land.y() + land.height() - 50, 0, 0, 180.0, WALL_ANOTHER_CHECKING_ENABLE);
		RequestCreateObject(FLAG_VNUM, nMapIndex, land.x() + 50, 				land.y() + land.height() - 50, 0, 0, 270.0, WALL_ANOTHER_CHECKING_ENABLE);
	}
	return true;
}

void CLand::RequestDeleteWall()
{
	itertype(m_map_pkObject) iter = m_map_pkObject.begin();

	while (iter != m_map_pkObject.end())
	{
		unsigned id   = iter->second->GetID();
		unsigned vnum = iter->second->GetVnum();

		switch (vnum)
		{
			case WALL_DOOR_VNUM:
			case WALL_BACK_VNUM:
			case WALL_LEFT_VNUM:
			case WALL_RIGHT_VNUM:
				RequestDeleteObject(id);
				break;
		}


		if (test_server)
		{
			if (FLAG_VNUM == vnum)
				RequestDeleteObject(id);

		}

		iter++;
	}
}

bool CLand::RequestCreateWallBlocks(DWORD dwVnum, long nMapIndex, char wallSize, bool doorEast, bool doorWest, bool doorSouth, bool doorNorth)
{
	auto& r = GetData();

	long startX = r.x() + (r.width() / 2) - (1300 + wallSize * 500);
	long startY = r.y() + (r.height() / 2) + (1300 + wallSize * 500);

	DWORD corner = dwVnum - 4;
	DWORD wall   = dwVnum - 3;
	DWORD door   = dwVnum - 1;

	bool checkAnother = false;
	long* ptr = NULL;
	int delta = 1;
	int rot = 270;

	bool doorOpen[4];
	doorOpen[0] = doorWest;
	doorOpen[1] = doorNorth;
	doorOpen[2] = doorEast;
	doorOpen[3] = doorSouth;

	if ( wallSize > 3 ) wallSize = 3;
	else if ( wallSize < 0 ) wallSize = 0;

	for ( int i=0; i < 4; i++, rot -= 90 )
	{
		switch ( i )
		{
			case 0 :
				delta = -1;
				ptr = &startY;
				break;
			case 1 :
				delta = 1;
				ptr = &startX;
				break;
			case 2 :
				ptr = &startY;
				delta = 1;
				break;
			case 3 :
				ptr = &startX;
				delta = -1;
				break;
		}

		this->RequestCreateObject(corner, nMapIndex, startX, startY, 0, 0, rot, checkAnother);

		*ptr = *ptr + ( 700 * delta );

		if ( doorOpen[i] )
		{
			this->DrawWall(wall, nMapIndex, startX, startY, wallSize, rot);

			*ptr = *ptr + ( 700 * delta );

			this->RequestCreateObject(door, nMapIndex, startX, startY, 0, 0, rot, checkAnother);

			*ptr = *ptr + ( 1300 * delta );

			this->DrawWall(wall, nMapIndex, startX, startY, wallSize, rot);
		}
		else
		{
			this->DrawWall(wall, nMapIndex, startX, startY, wallSize*2 + 4, rot);
		}

		*ptr = *ptr + ( 100 * delta );
	}

	return true;
}

void CLand::RequestDeleteWallBlocks(DWORD dwID)
{
	itertype(m_map_pkObject) iter = m_map_pkObject.begin();

	DWORD corner = dwID - 4;
	DWORD wall = dwID - 3;
	DWORD door = dwID - 1;
	DWORD dwVnum = 0;

	while ( iter != m_map_pkObject.end() )
	{
		dwVnum = iter->second->GetVnum();

		if ( dwVnum == corner || dwVnum == wall || dwVnum == door )
		{
			RequestDeleteObject(iter->second->GetID());
		}
		iter++;
	}
}
// END_BUILD_WALL

