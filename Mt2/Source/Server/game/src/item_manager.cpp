#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "db.h"
#include "log.h"
#include "skill.h"
#include "text_file_loader.h"
#include "priv_manager.h"
#include "questmanager.h"
#include "unique_item.h"
#include "safebox.h"
#include "blend_item.h"
#include "dev_log.h"
#include "item.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "sectree_manager.h"
#include "sectree.h"
#include "general_manager.h"

#include "../../common/VnumHelper.h"
#include "cube.h"

#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif

#ifdef INGAME_WIKI
#include "refine.h"
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

int CSpecialItemGroup::GetVnumByRange(DWORD dwStart, DWORD dwEnd) const
{
	// max 1000 tries
	for (int i = 0; i < 1000; ++i)
	{
		DWORD dwVnum = random_number(dwStart, dwEnd);
		if (ITEM_MANAGER::instance().GetTable(dwVnum))
			return dwVnum;
	}

	sys_err("cannot get vnum by special item group (vnum range %u~%u)", dwStart, dwEnd);
	return 0;
}

ITEM_MANAGER::ITEM_MANAGER()
	: m_iTopOfTable(0), m_dwVIDCount(0), m_dwCurrentID(0)
{
}

ITEM_MANAGER::~ITEM_MANAGER()
{
	Destroy();
}

void ITEM_MANAGER::Destroy()
{
	itertype(m_VIDMap) it = m_VIDMap.begin();
	for ( ; it != m_VIDMap.end(); ++it) {
		M2_DELETE(it->second);
	}

	m_VIDMap.clear();
}

void ITEM_MANAGER::GracefulShutdown()
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.begin();

	while (it != m_set_pkItemForDelayedSave.end())
		SaveSingleItem(*(it++));

	m_set_pkItemForDelayedSave.clear();
}

const network::TSoulProtoTable* ITEM_MANAGER::GetSoulProto(DWORD vnum)
{
	auto it = m_map_soulProtoTable.find(vnum);
	if (it != m_map_soulProtoTable.end())
		return &(it->second);
	return NULL;
}

void ITEM_MANAGER::InitializeSoulProto(const ::google::protobuf::RepeatedPtrField<network::TSoulProtoTable>& table)
{
	if (!m_map_soulProtoTable.empty())
		m_map_soulProtoTable.clear();

	for (auto& t : table)
	{
		if (m_map_soulProtoTable.find(t.vnum()) != m_map_soulProtoTable.end())
			sys_err("Dublicate in soul_proto table for %d", t.vnum());
		else
			m_map_soulProtoTable[t.vnum()] = t;

		sys_log(0, "soul_proto: %d loaded", m_map_soulProtoTable[t.vnum()].vnum());
	}
}

bool ITEM_MANAGER::ChangeSashAttr(LPITEM item)
{
	if (!item || item->GetType() != ITEM_COSTUME || item->GetSubType() != COSTUME_ACCE_COSTUME)
		return false;

	BYTE itemType = item->GetSocket(2);
	bool changed = false;
	for (int i = 0; i < MAX_NORM_ATTR_NUM; ++i)
	{
		auto& attr = item->GetAttribute(i);
		if (!attr.type() || !attr.value())
			continue;

		for (auto it = m_map_soulProtoTable.begin(); it != m_map_soulProtoTable.end(); ++it)
			if (it->second.soul_type() == itemType && it->second.apply_type() == attr.type())
			{
				changed = true;
				item->SetForceAttribute(i, attr.type(), it->second.apply_values(random_number(0, SOUL_TABLE_MAX_APPLY_COUNT - 1)));
			}
	}
	return changed;
}

bool ITEM_MANAGER::Initialize(const google::protobuf::RepeatedPtrField<network::TItemTable>& table)
{
	if (!m_vec_prototype.empty())
		m_vec_prototype.clear();

	m_vec_prototype.reserve(table.size());
	for (auto& t : table)
	{
		m_vec_prototype.push_back(t);

		if (0 != t.vnum_range())
		{
			m_vec_item_vnum_range_info.push_back(&m_vec_prototype[m_vec_prototype.size() - 1]);
		}
	}

	m_map_ItemRefineFrom.clear();
	for (auto& t : m_vec_prototype)
	{
		if (t.refined_vnum())
			m_map_ItemRefineFrom.insert(std::make_pair(t.refined_vnum(), t.vnum()));

		bool bIsQuestItem = t.type() == ITEM_QUEST;
#ifdef __PET_SYSTEM__
		if (ITEM_PET == t.type())
			bIsQuestItem = true;
#endif
		if (bIsQuestItem || IS_SET(t.flags(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			quest::CQuestManager::instance().RegisterNPCVnum(t.vnum());

		m_map_vid.insert( std::map<DWORD,network::TItemTable>::value_type( t.vnum(), t ) ); 
		if ( test_server )
			sys_log( 0, "ITEM_INFO %d %s ", t.vnum(), t.name().c_str() );	
	}

	int len = 0, len2;
	char buf[512];

	int i = 0;
	for (auto& t : m_vec_prototype)
	{
		len2 = snprintf(buf + len, sizeof(buf) - len, "%5u %-16s", t.vnum(), t.locale_name(LANGUAGE_DEFAULT).c_str());

		if (len2 < 0 || len2 >= (int) sizeof(buf) - len)
			len += (sizeof(buf) - len) - 1;
		else
			len += len2;

		if (!((i + 1) % 4))
		{
			if ( !test_server )
				sys_log(0, "%s", buf);
			len = 0;
		}
		else
		{
			buf[len++] = '\t';
			buf[len] = '\0';
		}

		++i;
	}

	if ((i + 1) % 4)
	{
		sys_log(!test_server, "%s", buf);
	}

	ITEM_VID_MAP::iterator it = m_VIDMap.begin();

	sys_log (1, "ITEM_VID_MAP %d", m_VIDMap.size() );

	while (it != m_VIDMap.end())
	{
		LPITEM item = it->second;
		++it;

		auto* tableInfo = GetTable(item->GetOriginalVnum());

		if (NULL == tableInfo)
		{
			sys_err("cannot reset item table");
			item->SetProto(NULL);
		}

		item->SetProto(tableInfo);
	}

	LoadUppItemList();
	LoadDisabledItemList();

	return true;
}

LPITEM ITEM_MANAGER::CreateItem(DWORD vnum, DWORD count, DWORD id, bool bTryMagic, int iRarePct, bool bSkipSave, DWORD owner_pid)
{
	if (0 == vnum)
		return NULL;

	auto* table = GetTable(vnum);

	if (NULL == table)
		return NULL;

	LPITEM item = NULL;

	//id·Î °Ë»çÇØ¼­ Á¸ÀçÇÑ´Ù¸é -- ¸®ÅÏ! 
	if (m_map_pkItemByID.find(id) != m_map_pkItemByID.end())
	{
		item = m_map_pkItemByID[id];
		if (!item)
		{
			sys_err("ITEM_ID_DUPE: %u", id);
			return NULL;
		}
		else if (owner_pid)
		{
			sys_err("ITEM_ID_DUPE: %u %s ownerPID %d window[%d] pos[%d]", id, item->GetName(), owner_pid, item->GetWindow(), item->GetCell());
			LogManager::instance().CharLog(owner_pid, item->GetWindow(), item->GetCell(), id, "ITEM_ID_DUP", item->GetName(), "");
		}
		else
		{
			LPCHARACTER owner = item->GetOwner();
			if (!owner)
			{	
				sys_err("NO OWNER FOUND!!!  ITEM_ID_DUPE: %u window[%d] pos[%d]", id, item->GetWindow(), item->GetCell());
				return NULL;
			}
			sys_err("ITEM_ID_DUPE: %u %s owner %p window[%d] pos[%d]", id, item->GetName(), get_pointer(owner), item->GetWindow(), item->GetCell());
			LogManager::instance().CharLog(owner, id, "ITEM_ID_DUP", item->GetName());
		}
		return NULL;
	}

	//¾ÆÀÌÅÛ ÇÏ³ª ÇÒ´çÇÏ°í
	item = M2_NEW CItem(vnum);

	bool bIsNewItem = (0 == id);

	//ÃÊ±âÈ­ ÇÏ°í. Å×ÀÌºí ¼ÂÇÏ°í
	item->Initialize();
	item->SetProto(table);

	if (test_server)
		sys_log(0, "[ITEM_CREATE] %p vnum %u name %s", item, item->GetVnum(), item->GetName());

	if (item->GetType() == ITEM_ELK) // µ·Àº ID°¡ ÇÊ¿ä¾ø°í ÀúÀåµµ ÇÊ¿ä¾ø´Ù.
		item->SetSkipSave(true);

	// Unique ID¸¦ ¼¼ÆÃÇÏÀÚ
	else if (!bIsNewItem)
	{
		item->SetID(id);
		item->SetSkipSave(true);
	}
	else
	{
		item->SetID(GetNewID());

#ifdef __PET_ADVANCED__
		if (item->GetType() == ITEM_PET_ADVANCED)
		{
			if (static_cast<EPetAdvancedItem>(item->GetSubType()) == EPetAdvancedItem::EGG)
				item->SetSocket(0, get_global_time() + item->GetValue(0));
			else if (static_cast<EPetAdvancedItem>(item->GetSubType()) == EPetAdvancedItem::SKILL_BOOK ||
				static_cast<EPetAdvancedItem>(item->GetSubType()) == EPetAdvancedItem::HEROIC_SKILL_BOOK)
			{
				if (item->GetValue(0) == 0)
				{
					auto& skill_map = CPetSkillProto::GetMap();
					auto it = skill_map.begin();

					while (true)
					{
						it = skill_map.begin();
						auto rand_idx = random_number(0, skill_map.size() - 1);
						while (rand_idx > 0)
							it++, rand_idx--;

						CPetSkillProto* proto = dynamic_cast<CPetSkillProto*>(it->second.get());
						if (proto && proto->IsHeroic() == (static_cast<EPetAdvancedItem>(item->GetSubType()) == EPetAdvancedItem::HEROIC_SKILL_BOOK))
							break;
					}

					item->SetSocket(0, it->first);
				}
			}
			else if (static_cast<EPetAdvancedItem>(item->GetSubType()) == EPetAdvancedItem::SUMMON)
			{
				if (item->GetAdvancedPet())
					item->GetAdvancedPet()->Initialize("Pet");
			}
		}
#endif

#ifdef __ALPHA_EQUIP__
		if (IS_SET(item->GetFlag(), ITEM_FLAG_ALPHA_EQUIP))
			item->RerollAlphaEquipValue();
#endif

		if (item->GetType() == ITEM_UNIQUE)
		{
			if (item->GetValue(2) == 0)
				item->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, item->GetValue(0)); // °ÔÀÓ ½Ã°£ À¯´ÏÅ©
			else
			{
				//int globalTime = get_global_time();
				//int lastTime = item->GetValue(0);
				//int endTime = get_global_time() + item->GetValue(0);
				item->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, get_global_time() + item->GetValue(0)); // ½Ç½Ã°£ À¯´ÏÅ©
			}
		}

#ifdef CRYSTAL_SYSTEM
		if (item->GetType() == ITEM_CRYSTAL && static_cast<ECrystalItem>(item->GetSubType()) == ECrystalItem::CRYSTAL)
		{
			auto crystal_proto = CGeneralManager::instance().get_crystal_proto(0, 0);
			item->set_crystal_grade(crystal_proto);

			item->restore_crystal_energy(CItem::CRYSTAL_MAX_DURATION);
		}
#endif

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_ADD_SPECIFIC_ATTRIBUTE)
		{
			auto& c_rkAddApply = item->GetProto()->applies(0);
			if (c_rkAddApply.type() != APPLY_NONE && c_rkAddApply.value() != 0)
			{
				item->SetForceAttribute(0, c_rkAddApply.type(), c_rkAddApply.value());
			}
			else
			{
				BYTE bAddAttrType = APPLY_NONE;
				short sAddAttrValue = 0;

				std::vector<BYTE> vecApplies;
				for (auto it : g_map_itemAttr)
				{
					auto& c_rkAttr = it.second;

					bool bAnyItemApply = false;
					for (int i = 0; i < ATTRIBUTE_SET_MAX_NUM; ++i)
					{
						if (c_rkAttr.max_level_by_set(i) > 0)
						{
							bAnyItemApply = true;
							break;
						}
					}

					if (!bAnyItemApply)
						continue;

					vecApplies.push_back(it.first);
				}

				if (vecApplies.size() > 0)
				{
					int iRand = random_number(0, vecApplies.size() - 1);
					bAddAttrType = vecApplies[iRand];

					auto& c_rkAttr = g_map_itemAttr[bAddAttrType];
					sAddAttrValue = c_rkAttr.values(random_number(0, 5 - 1));
				}

				if (bAddAttrType != APPLY_NONE && sAddAttrValue != 0)
					item->SetForceAttribute(0, bAddAttrType, sAddAttrValue);
			}
		}

		if (item->GetType() == ITEM_SOUL && m_map_soulProtoTable.find(vnum) != m_map_soulProtoTable.end())
			item->SetForceAttribute(0, m_map_soulProtoTable[vnum].apply_type(), m_map_soulProtoTable[vnum].apply_values(random_number(0, SOUL_TABLE_MAX_APPLY_COUNT - 1)));
	}


	switch (item->GetVnum())
	{
		case ITEM_AUTO_HP_RECOVERY_S:
		case ITEM_AUTO_HP_RECOVERY_M:
		case ITEM_AUTO_HP_RECOVERY_L:
		case ITEM_AUTO_HP_RECOVERY_X:
		case ITEM_AUTO_SP_RECOVERY_S:
		case ITEM_AUTO_SP_RECOVERY_M:
		case ITEM_AUTO_SP_RECOVERY_L:
		case ITEM_AUTO_SP_RECOVERY_X:
		case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
		case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
		case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
		case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
		case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
		case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
#ifdef ENABLE_PERMANENT_POTIONS
		case ITEM_AUTO_HP_RECOVERY_PERMANENT:
		case ITEM_AUTO_SP_RECOVERY_PERMANENT:
#endif
			if (bIsNewItem)
				item->SetSocket(2, item->GetValue(0), true);
			else
				item->SetSocket(2, item->GetValue(0), false);
			break;
	}

#ifdef __ACCE_COSTUME__
#ifndef PROMETA
	if (CItemVnumHelper::IsAcceItem(item->GetVnum()))
	{
		auto& aApplies = item->GetProto()->applies();
		item->SetSocket(0, (aApplies[0].value() != aApplies[1].value() && aApplies[1].value() > 0) ?
			random_number(aApplies[0].value(), aApplies[1].value()) : aApplies[0].value());
	}
#endif
#endif

	if (item->GetType() == ITEM_ELK) // µ·Àº ¾Æ¹« Ã³¸®°¡ ÇÊ¿äÇÏÁö ¾ÊÀ½
		;
	else if (item->IsStackable())  // ÇÕÄ¥ ¼ö ÀÖ´Â ¾ÆÀÌÅÛÀÇ °æ¿ì
	{
		count = MINMAX(1, count, ITEM_MAX_COUNT);

		if (bTryMagic && count <= 1 && IS_SET(item->GetFlag(), ITEM_FLAG_MAKECOUNT))
			count = item->GetValue(1);
	}
	else
		count = 1;

	item->SetVID(++m_dwVIDCount);

	if (bSkipSave == false)
		m_VIDMap.insert(ITEM_VID_MAP::value_type(item->GetVID(), item));

	if (item->GetID() != 0 && bSkipSave == false)
		m_map_pkItemByID.insert(std::map<DWORD, LPITEM>::value_type(item->GetID(), item));

	if (!item->SetCount(count))
		return NULL;

	item->SetSkipSave(false);

	if (item->GetType() == ITEM_UNIQUE && item->GetValue(2) != 0)
		item->StartUniqueExpireEvent();

	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		// ¾ÆÀÌÅÛ »ý¼º ½ÃÁ¡ºÎÅÍ »ç¿ëÇÏÁö ¾Ê¾Æµµ ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä
		if (LIMIT_REAL_TIME == item->GetLimitType(i))
		{
			if (item->GetLimitValue(i))
			{
				item->SetSocket(0, time(0) + item->GetLimitValue(i)); 
			}
			else
			{
				item->SetSocket(0, time(0) + 60*60*24*7); 
			}

			item->StartRealTimeExpireEvent();
		}

		// ±âÁ¸ À¯´ÏÅ© ¾ÆÀÌÅÛÃ³·³ Âø¿ë½Ã¿¡¸¸ »ç¿ë°¡´É ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä
		else if (LIMIT_TIMER_BASED_ON_WEAR == item->GetLimitType(i))
		{
			// ÀÌ¹Ì Âø¿ëÁßÀÎ ¾ÆÀÌÅÛÀÌ¸é Å¸ÀÌ¸Ó¸¦ ½ÃÀÛÇÏ°í, »õ·Î ¸¸µå´Â ¾ÆÀÌÅÛÀº »ç¿ë °¡´É ½Ã°£À» ¼¼ÆÃÇØÁØ´Ù. (
			// ¾ÆÀÌÅÛ¸ô·Î Áö±ÞÇÏ´Â °æ¿ì¿¡´Â ÀÌ ·ÎÁ÷¿¡ µé¾î¿À±â Àü¿¡ Socket0 °ªÀÌ ¼¼ÆÃÀÌ µÇ¾î ÀÖ¾î¾ß ÇÑ´Ù.
			if (true == item->IsEquipped())
			{
				item->StartTimerBasedOnWearExpireEvent();
			}
			else if(0 == id)
			{
				long duration = item->GetSocket(0);
				if (0 == duration)
					duration = item->GetLimitValue(i);

				if (0 == duration)
					duration = 60 * 60 * 10;	// Á¤º¸°¡ ¾Æ¹«°Íµµ ¾øÀ¸¸é µðÆúÆ®·Î 10½Ã°£ ¼¼ÆÃ

				item->SetSocket(0, duration);
			}
		}
	}

	if (id == 0) // »õ·Î ¸¸µå´Â ¾ÆÀÌÅÛÀÏ ¶§¸¸ Ã³¸®
	{
		// »õ·ÎÃß°¡µÇ´Â ¾àÃÊµéÀÏ°æ¿ì ¼º´ÉÀ» ´Ù¸£°ÔÃ³¸®
		if (ITEM_BLEND==item->GetType())
		{
			if (Blend_Item_find(item->GetVnum()))
			{
				Blend_Item_set_value(item);

#ifdef INFINITY_ITEMS
				if (item->IsInfinityItem())
					item->SetSocket(2, 0);
#endif

				return item;
			}
		}

		if (table->addon_type())
		{
			item->ApplyAddon(table->addon_type());
		}

		if (bTryMagic)
		{
			if (iRarePct == -1)
				iRarePct = table->alter_to_magic_item_pct();

			if (random_number(1, 100) <= iRarePct)
				item->AlterToMagicItem();
		}

		
#ifdef PROMETA
		if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_ACCE)
			item->AlterToSocketItem(ACCE_SOCKET_MAX_NUM);
		else
#endif
		{
			if (table->gain_socket_pct() > 0)
				item->AlterToSocketItem(table->gain_socket_pct());
		}

		// 50300 == ±â¼ú ¼ö·Ã¼­
		if (vnum == 50300 || vnum == ITEM_SKILLFORGET_VNUM)
		{
			DWORD dwSkillVnum;

			do
			{
				dwSkillVnum = random_number(1, 111);

				if (NULL != CSkillManager::instance().Get(dwSkillVnum))
					break;
			} while (true);

			item->SetSocket(0, dwSkillVnum);
		}
		else if (ITEM_SKILLFORGET2_VNUM == vnum)
		{
			DWORD dwSkillVnum;

			do
			{
				dwSkillVnum = random_number(112, 119);

				if (NULL != CSkillManager::instance().Get(dwSkillVnum))
					break;
			} while (true);

			item->SetSocket(0, dwSkillVnum);
		}
	}

	if (item->GetType() == ITEM_QUEST)
	{
		for (itertype (m_map_pkQuestItemGroup) it = m_map_pkQuestItemGroup.begin(); it != m_map_pkQuestItemGroup.end(); it++)
		{
			if (it->second->m_bType == CSpecialItemGroup::QUEST && it->second->Contains(vnum))
			{
				item->SetSIGVnum(it->first);
			}
		}
	}
	else if (item->GetType() == ITEM_UNIQUE)
	{
		for (itertype (m_map_pkSpecialItemGroup) it = m_map_pkSpecialItemGroup.begin(); it != m_map_pkSpecialItemGroup.end(); it++)
		{
			if (it->second->m_bType == CSpecialItemGroup::SPECIAL && it->second->Contains(vnum))
			{
				item->SetSIGVnum(it->first);
			}
		}
	}

#ifdef __DRAGONSOUL__
	if (item->IsDragonSoul() && 0 == id)
	{
		DSManager::instance().DragonSoulItemInitialize(item);
	}
#endif

	return item;
}

LPITEM ITEM_MANAGER::CreateItem(const network::TItemData* pTable, bool bSkipSave, bool bSetSocket)
{
	LPITEM pkItem = CreateItem(pTable->vnum(), pTable->count(), pTable->id(), false, -1, false, pTable->owner());
	if (pkItem)
	{
		pkItem->SetSkipSave(bSkipSave);
		pkItem->SetGMOwner(pTable->is_gm_owner());
		if (bSetSocket)
			pkItem->SetSockets(pTable->sockets());
		pkItem->SetAttributes(pTable->attributes());
#ifdef __ALPHA_EQUIP__
		pkItem->LoadAlphaEquipValue(pTable->alpha_equip_value());
#endif
#ifdef INFINITY_ITEMS
		if (pkItem->IsInfinityItem() && pkItem->GetSocket(2))
			pkItem->Lock(true);
#endif
	}

	return pkItem;
}

void ITEM_MANAGER::DelayedSave(LPITEM item)
{
	if (item->GetID() != 0)
		m_set_pkItemForDelayedSave.insert(item);
}

void ITEM_MANAGER::FlushDelayedSave(LPITEM item)
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.find(item);

	if (it == m_set_pkItemForDelayedSave.end())
	{
		return;
	}

	m_set_pkItemForDelayedSave.erase(it);
	SaveSingleItem(item);
}

void ITEM_MANAGER::SaveSingleItem(LPITEM item)
{
	if (!item->GetOwner())
	{
		DWORD dwID = item->GetID();
		DWORD dwOwnerID = item->GetLastOwnerPID();

		network::GDOutputPacket<network::GDItemDestroyPacket> pdb;
		pdb->set_pid(dwOwnerID);
		pdb->set_item_id(dwID);
		db_clientdesc->DBPacket(pdb);

		sys_log(1, "ITEM_DELETE %s:%u", item->GetName(), dwID);
		return;
	}

	sys_log(1, "ITEM_SAVE %s:%u in %s window %d", item->GetName(), item->GetID(), item->GetOwner() ? item->GetOwner()->GetName() : "<unknown_owner>", item->GetWindow());

	network::TItemData t;
	GetPlayerItem(item, &t);
	if (item->GetWindow() == EQUIPMENT)
		t.mutable_cell()->set_cell(item->GetCell() - EQUIPMENT_SLOT_START);

	network::GDOutputPacket<network::GDItemSavePacket> pdb;
	*pdb->mutable_data() = t;
	db_clientdesc->DBPacket(pdb);

#ifdef __PET_ADVANCED__
	if (item->GetAdvancedPet())
		item->GetAdvancedPet()->SaveDirect();
#endif
}

void ITEM_MANAGER::Update()
{
	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.begin();
	TR1_NS::unordered_set<LPITEM>::iterator this_it;

	while (it != m_set_pkItemForDelayedSave.end())
	{
		this_it = it++;
		LPITEM item = *this_it;

		// SLOW_QUERY ÇÃ·¡±×°¡ ÀÖ´Â °ÍÀº Á¾·á½Ã¿¡¸¸ ÀúÀåÇÑ´Ù.
		if (item->GetOwner() && IS_SET(item->GetFlag(), ITEM_FLAG_SLOW_QUERY))
			continue;

		SaveSingleItem(item);

		m_set_pkItemForDelayedSave.erase(this_it);
	}
}

void ITEM_MANAGER::RemoveItem(LPITEM item, const char * c_pszReason)
{
	LPCHARACTER o;

	if ((o = item->GetOwner()))
	{
#ifdef __ENABLE_FULL_LOGS__
		char szHint[64];
		snprintf(szHint, sizeof(szHint), "%s %i ", item->GetName(), item->GetCount());
		LogManager::instance().ItemLog(o, item, c_pszReason ? c_pszReason : "REMOVE", szHint);
#endif
		// SAFEBOX_TIME_LIMIT_ITEM_BUG_FIX
		if (item->GetWindow() == MALL || item->GetWindow() == SAFEBOX)
		{
			// 20050613.ipkn.½Ã°£Á¦ ¾ÆÀÌÅÛÀÌ »óÁ¡¿¡ ÀÖÀ» °æ¿ì ½Ã°£¸¸·á½Ã ¼­¹ö°¡ ´Ù¿îµÈ´Ù.
			CSafebox* pSafebox = item->GetWindow() == MALL ? o->GetMall() : o->GetSafebox();
			if (pSafebox)
			{
				pSafebox->Remove(item->GetCell());
			}
		}
		// END_OF_SAFEBOX_TIME_LIMIT_ITEM_BUG_FIX
		else
		{
			o->SyncQuickslot(QUICKSLOT_TYPE_ITEM, item->GetCell(), 255);
			item->RemoveFromCharacter();
		}
	}

	M2_DESTROY_ITEM(item);
}

void ITEM_MANAGER::RemoveItemForFurtherUse(LPITEM item)
{
	// set item cache to be ignored on loading for the next 10 seconds after storing the latest cache, in this time the next use core should be able to remove the item cache from the database if it succeeds
	ITEM_MANAGER::instance().FlushDelayedSave(item);

	network::GDOutputPacket<network::GDItemTimedIgnorePacket> pack_db;
	pack_db->set_item_id(item->GetID());
	pack_db->set_ignore_duration(10);
	db_clientdesc->DBPacket(pack_db);

	item->SetSkipSave(true);
	ITEM_MANAGER::instance().RemoveItem(item);
}

void ITEM_MANAGER::DestroyItem(LPITEM item)
{
	if (item->GetSectree())
		item->RemoveFromGround();

	if (item->GetOwner())
	{
		if (CHARACTER_MANAGER::instance().Find(item->GetOwner()->GetPlayerID()) != NULL)
		{
			sys_err("DestroyItem: GetOwner %s %s!!", item->GetName(), item->GetOwner()->GetName());
			item->RemoveFromCharacter();
		}
		else
		{
			sys_err ("WTH! Invalid item owner. owner pointer : %p", item->GetOwner());
		}
	}

	TR1_NS::unordered_set<LPITEM>::iterator it = m_set_pkItemForDelayedSave.find(item);

	if (it != m_set_pkItemForDelayedSave.end())
		m_set_pkItemForDelayedSave.erase(it);

	DWORD dwID = item->GetID();
	sys_log(2, "ITEM_DESTROY %s:%u", item->GetName(), dwID);

	if (test_server)
		sys_log(0, "[ITEM_DESTROY] %p vnum %u name %s", item, item->GetVnum(), item->GetName());

	if (!item->GetSkipSave() && dwID)
	{
		DWORD dwOwnerID = item->GetLastOwnerPID();

		network::GDOutputPacket<network::GDItemDestroyPacket> pdb;
		pdb->set_pid(dwOwnerID);
		pdb->set_item_id(dwID);
		db_clientdesc->DBPacket(pdb);
	}
	else
	{
		sys_log(2, "ITEM_DESTROY_SKIP %s:%u (skip=%d)", item->GetName(), dwID, item->GetSkipSave());
	}

	if (dwID)
		m_map_pkItemByID.erase(dwID);

	m_VIDMap.erase(item->GetVID());
	M2_DELETE(item);
}

LPITEM ITEM_MANAGER::Find(DWORD id)
{
	itertype(m_map_pkItemByID) it = m_map_pkItemByID.find(id);
	if (it == m_map_pkItemByID.end())
		return NULL;
	return it->second;
}

LPITEM ITEM_MANAGER::FindByVID(DWORD vid)
{
	ITEM_VID_MAP::iterator it = m_VIDMap.find(vid);

	if (it == m_VIDMap.end())
		return NULL;

	return (it->second);
}

network::TItemTable * ITEM_MANAGER::GetTable(DWORD vnum)
{
	int rnum = RealNumber(vnum);
	
	if (rnum < 0)
	{
		for (int i = 0; i < m_vec_item_vnum_range_info.size(); i++)
		{
			auto p = m_vec_item_vnum_range_info[i];
			if ((p->vnum() < vnum) &&
				vnum < (p->vnum() + p->vnum_range()))
			{
				return p;
			}
		}
			
		return NULL;
	}
	
	return &m_vec_prototype[rnum];
}

int ITEM_MANAGER::RealNumber(DWORD vnum)
{
	if (!m_vec_prototype.size())
		return -1;

	int bot, top, mid;

	bot = 0;
	top = m_vec_prototype.size();

	auto* pTable = &m_vec_prototype[0];

	while (1)
	{
		mid = MINMAX(0, (bot + top) >> 1, m_vec_prototype.size() - 1);

		if ((pTable + mid)->vnum() == vnum)
			return (mid);

		if (bot >= top)
			return (-1);

		if ((pTable + mid)->vnum() > vnum)
			top = mid - 1;
		else		
			bot = mid + 1;
	}
}

bool ITEM_MANAGER::GetVnum(const char * c_pszName, DWORD & r_dwVnum, int iLanguageID)
{
	if (iLanguageID < 0 || iLanguageID >= LANGUAGE_MAX_NUM)
	{
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		{
			if (GetVnum(c_pszName, r_dwVnum, i))
				return true;
		}

		return false;
	}

	int len = strlen(c_pszName);

	auto* pTable = &m_vec_prototype[0];

	for (DWORD i = 0; i < m_vec_prototype.size(); ++i, ++pTable)
	{
		if (!strncasecmp(c_pszName, pTable->locale_name(iLanguageID).c_str(), len))
		{
			r_dwVnum = pTable->vnum();
			return true;
		}
	}

	return false;
}

bool ITEM_MANAGER::GetVnumByOriginalName(const char * c_pszName, DWORD & r_dwVnum)
{
	int len = strlen(c_pszName);

	auto* pTable = &m_vec_prototype[0];

	for (DWORD i = 0; i < m_vec_prototype.size(); ++i, ++pTable)
	{
		if (!strncasecmp(c_pszName, pTable->name().c_str(), len))
		{
			r_dwVnum = pTable->vnum();
			return true;
		}
	}

	return false;
}

class CItemDropInfo
{
	public:
		CItemDropInfo(int iLevelStart, int iLevelEnd, int iPercent, DWORD dwVnumStart, DWORD dwVnumEnd, unsigned char bCount = 1, unsigned char bJob = 0, bool bAutoPickup = false) :
			m_iLevelStart(iLevelStart), m_iLevelEnd(iLevelEnd), m_iPercent(iPercent), m_dwVnumStart(dwVnumStart), m_dwVnumEnd(dwVnumEnd), m_bCount(bCount), m_bJob(bJob), m_bAutoPickup(bAutoPickup)
			{
			}

		int	m_iLevelStart;
		int	m_iLevelEnd;
		int	m_iPercent; // 1 ~ 1000
		DWORD	m_dwVnumStart;
		DWORD	m_dwVnumEnd;
		unsigned char	m_bCount;
		unsigned char	m_bJob;
		bool	m_bAutoPickup;

		friend bool operator < (const CItemDropInfo & l, const CItemDropInfo & r)
		{
			return l.m_iLevelEnd < r.m_iLevelEnd;
		}
};

extern std::vector<CItemDropInfo> g_vec_pkCommonDropItem[MOB_RANK_MAX_NUM];

// 20050503.ipkn.
// iMinimum º¸´Ù ÀÛÀ¸¸é iDefault ¼¼ÆÃ (´Ü, iMinimumÀº 0º¸´Ù Ä¿¾ßÇÔ)
// 1, 0 ½ÄÀ¸·Î ON/OFF µÇ´Â ¹æ½ÄÀ» Áö¿øÇÏ±â À§ÇØ Á¸Àç
int GetDropPerKillPct(int iMinimum, int iDefault, int iDeltaPercent, const char * c_pszFlag, float fFactor = 1)
{
	int iVal = 0;

	if ((iVal = quest::CQuestManager::instance().GetEventFlag(c_pszFlag)))
	{
		if (fFactor > 1 && !quest::CQuestManager::instance().GetEventFlag("quest_drop_factor_disabled"))
			iVal = iVal * fFactor;
			
		if (!test_server)
		{
			if (iVal < iMinimum)
				iVal = iDefault;

			if (iVal < 0)
				iVal = iDefault;
		}
	}

	if (iVal == 0)
		return 0;

	// ±âº» ¼¼ÆÃÀÏ¶§ (iDeltaPercent=100) 
	// 40000 iVal ¸¶¸®´ç ÇÏ³ª ´À³¦À» ÁÖ±â À§ÇÑ »ó¼öÀÓ
	return (40000 * iDeltaPercent / iVal);
}

// ADD_GRANDMASTER_SKILL
int GetThreeSkillLevelAdjust(int level)
{
	if (level < 40)
		return 32;
	if (level < 45)
		return 16;
	if (level < 50)
		return 8;
	if (level < 55)
		return 4;
	if (level < 60)
		return 2;
	return 1;
}
// END_OF_ADD_GRANDMASTER_SKILL

bool ITEM_MANAGER::GetDropPct(LPCHARACTER pkChr, LPCHARACTER pkKiller, OUT int& iDeltaPercent, OUT int& iRandRange)
{
 	if (NULL == pkChr || NULL == pkKiller)
		return false;

	int iLevel = pkKiller->GetLevel();
	iDeltaPercent = 100;

	if (!pkChr->IsStone() && pkChr->GetMobRank() >= MOB_RANK_BOSS)
		iDeltaPercent = PERCENT_LVDELTA_BOSS(pkKiller->GetLevel(), pkChr->GetLevel());
	else
		iDeltaPercent = PERCENT_LVDELTA(pkKiller->GetLevel(), pkChr->GetLevel());

	BYTE bRank = pkChr->GetMobRank();

	if (1 == random_number(1, 50000))
		iDeltaPercent += 1000;
	else if (1 == random_number(1, 10000))
		iDeltaPercent += 500;

	sys_log(3, "CreateDropItem for level: %d rank: %u pct: %d", iLevel, bRank, iDeltaPercent);
	iDeltaPercent = iDeltaPercent * CHARACTER_MANAGER::instance().GetMobItemRate(pkKiller) / 100;

	// ADD_PREMIUM
	if (pkKiller->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0 ||
			pkKiller->IsEquipUniqueGroup(UNIQUE_GROUP_DOUBLE_ITEM))
		iDeltaPercent += iDeltaPercent;
	// END_OF_ADD_PREMIUM

	// ADD_APPLY
	if (pkKiller->GetPoint(POINT_ITEM_DROP_BONUS) > 0)
	{
		iDeltaPercent += pkKiller->GetPoint(POINT_ITEM_DROP_BONUS);
		// if (test_server)
		// 	pkKiller->ChatPacket(CHAT_TYPE_INFO, "item drop bonus %d new perc %d", pkKiller->GetPoint(POINT_ITEM_DROP_BONUS), iDeltaPercent);
	}
	// END_OF_ADD_APPLY

	// ADD_MALL_APPLY
	if (pkKiller->GetPoint(POINT_MALL_ITEMBONUS) > 0)
	{
		iDeltaPercent += pkKiller->GetPoint(POINT_MALL_ITEMBONUS);
		// if (test_server)
		// 	pkKiller->ChatPacket(CHAT_TYPE_INFO, "mall drop bonus %d new perc %d", pkKiller->GetPoint(POINT_MALL_ITEMBONUS), iDeltaPercent);
	}
	// END_OF_ADD_MALL_APPLY

	iRandRange = 1000000;
	iRandRange = iRandRange * 100 / 
		(100 + 
#ifdef __FAKE_PRIV_BONI__
		 (CPrivManager::instance().GetPriv(pkKiller, PRIV_ITEM_DROP) / 2)
#else
		 CPrivManager::instance().GetPriv(pkKiller, PRIV_ITEM_DROP)
#endif
		 + 
		 (pkKiller->IsEquipUniqueItem(UNIQUE_ITEM_DOUBLE_ITEM)?100:0));

	return true;
}

void ITEM_MANAGER::GetDropItemList(DWORD dwNPCRace, std::vector<network::TTargetMonsterDropInfoTable>& vec_item, BYTE bLevel)
{
	const CMob* pMob = CMobManager::instance().Get(dwNPCRace);
	auto* pMobTable = pMob ? &pMob->m_table : NULL;
	if (!pMobTable)
		return;

	struct TTargetMonsterDropInfoTableWithPct {
		BYTE	bLevelLimit;
		DWORD	dwItemVnum;
#ifdef INCREASE_ITEM_STACK
		WORD	bItemCount;
#else
		BYTE	bItemCount;
#endif
		DWORD	dwPct;

		bool operator<(const TTargetMonsterDropInfoTableWithPct& o) const {
			if (dwPct != o.dwPct)
				return dwPct < o.dwPct;

			if (dwItemVnum != o.dwItemVnum)
				return dwItemVnum < o.dwItemVnum;

			return bItemCount < o.bItemCount;
		}
	};

	std::vector<TTargetMonsterDropInfoTableWithPct> kTempVec;

	TTargetMonsterDropInfoTableWithPct kTempTab;
	kTempTab.bLevelLimit = 0;

	// stones
	for (int i = 0; i < STONE_INFO_MAX_NUM; ++i)
	{
		if (aStoneDrop[i].dwMobVnum == dwNPCRace)
		{
			kTempTab.bItemCount = 1;
			for (int j = 0; j < STONE_LEVEL_MAX_NUM + 1; ++j)
			{
				if (aStoneDrop[i].iLevelPct[j] > 0)
				{
					kTempTab.dwItemVnum = 28030 + j * 100;
					kTempTab.dwPct = aStoneDrop[i].iDropPct * 10000;
					kTempVec.push_back(kTempTab);
				}
			}
		}
	}

	// soulstone
	if (pMobTable->level() >= 40 && pMobTable->rank() >= MOB_RANK_BOSS && quest::CQuestManager::instance().GetEventFlag("three_skill_item") > 0)
	{
		if (pMobTable->level() < 90)
		{
			kTempTab.dwItemVnum = 50513;
			kTempTab.bItemCount = 1;
			kTempTab.dwPct = 1000000 / quest::CQuestManager::instance().GetEventFlag("three_skill_item") / GetThreeSkillLevelAdjust(bLevel);
			kTempVec.push_back(kTempTab);
		}
	}
	
#ifdef AELDRA
	// mithril
	if (((dwNPCRace >= 8051 && dwNPCRace <= 8058) || (pMobTable->level() >= 95 && pMobTable->level() < 115)) 
		&& pMobTable->rank() >= MOB_RANK_BOSS && quest::CQuestManager::instance().GetEventFlag("mithril_drop_item") > 0)
	{
			kTempTab.dwItemVnum = 92919;
			kTempTab.bItemCount = 1;
			kTempTab.dwPct = 1000000 / quest::CQuestManager::instance().GetEventFlag("mithril_drop_item") / GetThreeSkillLevelAdjust(bLevel);
			kTempVec.push_back(kTempTab);
	}
#endif

	// level item group
	{
		itertype(m_map_pkLevelItemGroup) it;
		it = m_map_pkLevelItemGroup.find(dwNPCRace);

		if (it != m_map_pkLevelItemGroup.end())
		{
			/*if (!bLevel || it->second->GetLevelLimit() <= (DWORD)bLevel)*/
			{
				kTempTab.bLevelLimit = it->second->GetLevelLimit();
				typeof(it->second->GetVector()) v = it->second->GetVector();

				for (DWORD i = 0; i < v.size(); ++i)
				{
					kTempTab.bItemCount = v[i].iCount;

					for (DWORD j = v[i].dwVNumStart; j <= v[i].dwVNumEnd; ++j)
					{
						kTempTab.dwItemVnum = j;
						kTempTab.dwPct = v[i].dwPct;
						kTempVec.push_back(kTempTab);
					}
				}
			}
		}
	}
	kTempTab.bLevelLimit = 0;

	// drop item group
	{
		itertype(m_map_pkDropItemGroup) it;
		it = m_map_pkDropItemGroup.find(dwNPCRace);

		if (it != m_map_pkDropItemGroup.end())
		{
			typeof(it->second->GetVector()) v = it->second->GetVector();

			for (DWORD i = 0; i < v.size(); ++i)
			{
				kTempTab.bItemCount = v[i].iCount;

				for (DWORD j = v[i].dwVnumStart; j <= v[i].dwVnumEnd; ++j)
				{
					kTempTab.dwItemVnum = j;
					kTempTab.dwPct = v[i].dwPct;
					kTempVec.push_back(kTempTab);
				}
			}
		}
	}

	// mob drop item group
	{
		itertype(m_map_pkMobItemGroup) it;
		it = m_map_pkMobItemGroup.find(dwNPCRace);

		if (it != m_map_pkMobItemGroup.end())
		{
			std::vector<CMobItemGroup*>& vec_pGroups = it->second;

			for (int i = 0; i < vec_pGroups.size(); ++i)
			{
				CMobItemGroup* pGroup = vec_pGroups[i];

				if (pGroup && !pGroup->IsEmpty())
				{
					const std::vector<CMobItemGroup::SMobItemGroupInfo>& v = pGroup->GetItemVector();

					for (DWORD x = 0; x < v.size(); ++x)
					{
						kTempTab.bItemCount = v[x].iCount;

						for (DWORD j = v[x].dwItemVnumStart; j <= v[x].dwItemVnumEnd; ++j)
						{
							kTempTab.dwItemVnum = j;
							kTempTab.dwPct = 1000000 / pGroup->GetKillPerDrop();
							kTempVec.push_back(kTempTab);
						}
					}
				}
			}
		}
	}

	std::sort(kTempVec.begin(), kTempVec.end());
	for (int i = kTempVec.size() - 1; i >= 0; --i)
	{
		network::TTargetMonsterDropInfoTable kTemp;
		kTemp.set_level_limit(kTempVec[i].bLevelLimit);
		kTemp.set_item_vnum(kTempVec[i].dwItemVnum);
		kTemp.set_item_count(kTempVec[i].bItemCount);

		vec_item.push_back(kTemp);
	}
}

bool ITEM_MANAGER::CreateDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item, std::vector<LPITEM> & vec_item_auto_pickup)
{
	int iLevel = pkKiller->GetLevel();
	int iDoubleDropPct = pkKiller->GetPoint(POINT_DOUBLE_ITEM_DROP_BONUS);

#ifdef AELDRA
#ifdef __DISABLE_STONE_LV20_DROP__
	if (pkChr->IsStone() && iLevel > pkChr->GetLevel() + 20 && !quest::CQuestManager::instance().GetEventFlag("stone_drop_lv_disabled"))
		return false;
#endif

#ifdef __DISABLE_BOSS_LV25_DROP__
	if (pkChr->IsMonster() && !pkChr->IsStone() && pkChr->GetMobRank() >= MOB_RANK_BOSS && (iLevel - 25 > pkChr->GetLevel() || iLevel + 25 < pkChr->GetLevel()))
		return false;
#endif
	
#elif defined(ELONIA)
	if ((pkChr->IsStone() || pkChr->GetMobRank() >= MOB_RANK_BOSS) && iLevel > pkChr->GetLevel() + 20)
		return false;
#endif

	int iDeltaPercent, iRandRange;
	if (!GetDropPct(pkChr, pkKiller, iDeltaPercent, iRandRange))
		return false;

	if (test_server && quest::CQuestManager::instance().GetEventFlag("dmg_chat") == 1)
		pkKiller->ChatPacket(CHAT_TYPE_INFO, "DropPct [%s]: iDeltaPercent => %d iRandRange => %d", pkChr->GetName(), iDeltaPercent, iRandRange);

	if (test_server)
		sys_log(0, "CreateDropItem for %s from %s", pkKiller->GetName(), pkChr->GetName());

	BYTE bRank = pkChr->GetMobRank();
	LPITEM item = NULL;

	// Common Drop Items
	{
		for (int i = 0; i < g_vec_pkCommonDropItem[bRank].size(); ++i)
		{
			const CItemDropInfo & c_rInfo = g_vec_pkCommonDropItem[bRank][i];

			if (iLevel < c_rInfo.m_iLevelStart || (c_rInfo.m_iLevelEnd && iLevel > c_rInfo.m_iLevelEnd))
				continue;

			int iPercent = (c_rInfo.m_iPercent * iDeltaPercent) / 100;
			
			if (test_server)
				sys_log(0, "CreateCommonDropItem %d ~ %d vnum %d~%d perc %d(%d) auto_pickup %d", c_rInfo.m_iLevelStart, c_rInfo.m_iLevelEnd, c_rInfo.m_dwVnumStart, c_rInfo.m_dwVnumEnd, iPercent, c_rInfo.m_iPercent, c_rInfo.m_bAutoPickup);

			if (iPercent >= random_number(1, iRandRange))
			{
				network::TItemTable* pTable;
				DWORD dwVnum = 0;
				while (!dwVnum)
				{
					if (pTable = GetTable(random_number(c_rInfo.m_dwVnumStart, c_rInfo.m_dwVnumEnd)))
						dwVnum = pTable->vnum();
				}

				item = NULL;

				int iCount = 1;
				if (iDoubleDropPct && iDoubleDropPct >= random_number(1, 100))
					iCount = 2;

				for (int i = 0; i < iCount; ++i)
				{
					if (pTable->type() == ITEM_POLYMORPH)
					{
						item = CreateItem(dwVnum, c_rInfo.m_bCount, 0, true);

						if (item)
							item->SetSocket(0, pkChr->GetRaceNum());
					}
					else
						item = CreateItem(dwVnum, c_rInfo.m_bCount, 0, true);

					if (item)
					{
						if (c_rInfo.m_bAutoPickup)
							vec_item_auto_pickup.push_back(item);
						else
							vec_item.push_back(item);

						if (test_server)
							LogManager::instance().ItemLog(pkKiller, item, "DROP_ITEM", "common_drop_item");
					}
				}
			}
		}
	}

	// Drop Item Group
	{
		itertype(m_map_pkDropItemGroup) it;
		it = m_map_pkDropItemGroup.find(pkChr->GetRaceNum());

		if (it != m_map_pkDropItemGroup.end())
		{
			typeof(it->second->GetVector()) v = it->second->GetVector();

			for (DWORD i = 0; i < v.size(); ++i)
			{
				int iPercent = (v[i].dwPct * iDeltaPercent) / 100;

				if (iPercent >= random_number(1, iRandRange))
				{
					int iCount = 1;
					if (iDoubleDropPct && iDoubleDropPct >= random_number(1, 100))
						iCount = 2;

					for (int j = 0; j < iCount; ++j)
					{
						item = CreateItem(random_number(v[i].dwVnumStart, v[i].dwVnumEnd), v[i].iCount, 0, true);

						if (test_server)
							sys_log(0, "DropItem [\"DROP\"]: vnum %d (%d~%d) count %d (%d)", item ? item->GetVnum() : 0, v[i].dwVnumStart, v[i].dwVnumEnd, item->GetCount(), v[i].iCount);

						if (item)
						{
							if (item->GetType() == ITEM_POLYMORPH)
							{
								if (item->GetVnum() == pkChr->GetPolymorphItemVnum())
								{
									item->SetSocket(0, pkChr->GetRaceNum());
								}
							}

							vec_item.push_back(item);
							if (test_server)
								LogManager::instance().ItemLog(pkKiller, item, "DROP_ITEM", "drop item group");
						}
					}
				}
			}
		}
	}

	// MobDropItem Group
	{
		itertype(m_map_pkMobItemGroup) it;
		it = m_map_pkMobItemGroup.find(pkChr->GetRaceNum());

		if ( it != m_map_pkMobItemGroup.end() )
		{
			std::vector<CMobItemGroup*>& vec_pGroups = it->second;

			for (int i = 0; i < vec_pGroups.size(); ++i)
			{
				CMobItemGroup* pGroup = vec_pGroups[i];

				// MOB_DROP_ITEM_BUG_FIX
				// 20050805.myevan.MobDropItem ¿¡ ¾ÆÀÌÅÛÀÌ ¾øÀ» °æ¿ì CMobItemGroup::GetOne() Á¢±Ù½Ã ¹®Á¦ ¹ß»ý ¼öÁ¤
				if (pGroup && !pGroup->IsEmpty())
				{
					int iPercent = 10000 * iDeltaPercent / pGroup->GetKillPerDrop();

					if (test_server)
						sys_log(0, "DropItem [\"KILL\"]: perc %u randRange %u (killPerDrop %u)", iPercent, iRandRange, pGroup->GetKillPerDrop());

					if (iPercent >= random_number(1, iRandRange))
					{
						int iCount = 1;
						if (iDoubleDropPct && iDoubleDropPct >= random_number(1, 100))
							iCount = 2;

						for (int i = 0; i < iCount; ++i)
						{
							const CMobItemGroup::SMobItemGroupInfo& info = pGroup->GetOne();
							item = CreateItem(random_number(info.dwItemVnumStart, info.dwItemVnumEnd), info.iCount, 0, true, info.iRarePct);

							if (item)
							{
								vec_item.push_back(item);
								if (test_server)
									LogManager::instance().ItemLog(pkKiller, item, "DROP_ITEM", "mob_item_group");
							}
						}
					}
				}
				// END_OF_MOB_DROP_ITEM_BUG_FIX
			}
		}
	}

	// Level Item Group
	{
		itertype(m_map_pkLevelItemGroup) it;
		it = m_map_pkLevelItemGroup.find(pkChr->GetRaceNum());

		if ( it != m_map_pkLevelItemGroup.end() )
		{
			if ( it->second->GetLevelLimit() <= (DWORD)iLevel )
			{
				typeof(it->second->GetVector()) v = it->second->GetVector();

				for ( DWORD i=0; i < v.size(); i++ )
				{
					if ( v[i].dwPct >= (DWORD)random_number(1, 1000000/*iRandRange*/) )
					{
						int iCount = 1;
						if (iDoubleDropPct && iDoubleDropPct >= random_number(1, 100))
							iCount = 2;

						for (int j = 0; j < iCount; ++j)
						{
							DWORD dwVnum = random_number(v[i].dwVNumStart, v[i].dwVNumEnd);
							item = CreateItem(dwVnum, v[i].iCount, 0, true);
							if (item)
							{
								vec_item.push_back(item);
								if (test_server)
									LogManager::instance().ItemLog(pkKiller, item, "DROP_ITEM", "level_item_group");
							}
						}
					}
				}
			}
		}
	}
	
	// BuyerTheitGloves Item Group
	{
		if (pkKiller->GetPremiumRemainSeconds(PREMIUM_ITEM) > 0 ||
				pkKiller->IsEquipUniqueGroup(UNIQUE_GROUP_DOUBLE_ITEM))
		{
			itertype(m_map_pkGloveItemGroup) it;
			it = m_map_pkGloveItemGroup.find(pkChr->GetRaceNum());

			if (it != m_map_pkGloveItemGroup.end())
			{
				typeof(it->second->GetVector()) v = it->second->GetVector();

				for (DWORD i = 0; i < v.size(); ++i)
				{
					int iPercent = (v[i].dwPct * iDeltaPercent) / 100;

					if (iPercent >= random_number(1, iRandRange))
					{
						int iCount = 1;
						if (iDoubleDropPct && iDoubleDropPct >= random_number(1, 100))
							iCount = 2;

						for (int i = 0; i < iCount; ++i)
						{
							DWORD dwVnum = random_number(v[i].dwVnumStart, v[i].dwVnumEnd);
							item = CreateItem(dwVnum, v[i].iCount, 0, true);
							if (item)
							{
								vec_item.push_back(item);
								if (test_server)
									LogManager::instance().ItemLog(pkKiller, item, "DROP_ITEM", "buyer_theit_gloves");
							}
						}
					}
				}
			}
		}
	}
	
	// ÀâÅÛ
	if (pkChr->GetMobDropItemVnum())
	{
		itertype(m_map_dwEtcItemDropProb) it = m_map_dwEtcItemDropProb.find(pkChr->GetMobDropItemVnum());

		if (it != m_map_dwEtcItemDropProb.end())
		{
			int iPercent = (it->second * iDeltaPercent) / 100;

			if (iPercent >= random_number(1, iRandRange))
			{
				int iCount = 1;
				if (iDoubleDropPct && iDoubleDropPct >= random_number(1, 100))
					iCount = 2;

				for (int i = 0; i < iCount; ++i)
				{
					item = CreateItem(pkChr->GetMobDropItemVnum(), 1, 0, true);
					if (item)
					{
						vec_item.push_back(item);
						if (test_server)
							LogManager::instance().ItemLog(pkKiller, item, "DROP_ITEM", "mob drop item vnum");
					}
				}
			}
		}
	}

	if (pkChr->IsStone())
	{
		if (pkChr->GetDropMetinStoneVnum())
		{
			int iPercent = (pkChr->GetDropMetinStonePct() * iDeltaPercent) * 400;

			if (iPercent >= random_number(1, iRandRange))
			{
				item = CreateItem(pkChr->GetDropMetinStoneVnum(), 1, 0, true);
				if (item) vec_item.push_back(item);
			}
		}
	}

	//
	// ½ºÆä¼È µå·Ó ¾ÆÀÌÅÛ
	// 
	CreateQuestDropItem(pkChr, pkKiller, vec_item, iDeltaPercent, iRandRange);

	for (itertype(vec_item) it = vec_item.begin(); it != vec_item.end(); ++it)
	{
		LPITEM item = *it;
		LogManager::instance().MoneyLog(MONEY_LOG_DROP, item->GetVnum(), item->GetCount());
	}

	return vec_item.size() + vec_item_auto_pickup.size();
}

// DROPEVENT_CHARSTONE
// drop_char_stone 1
// drop_char_stone.percent_lv01_10 5
// drop_char_stone.percent_lv11_30 10
// drop_char_stone.percent_lv31_MX 15
// drop_char_stone.level_range	   10
static struct DropEvent_CharStone
{
	int percent_lv01_10;
	int percent_lv11_30;
	int percent_lv31_MX;
	int level_range;
	bool alive;

	DropEvent_CharStone()
	{
		percent_lv01_10 =  100;
		percent_lv11_30 =  200;
		percent_lv31_MX =  300;
		level_range = 10;
		alive = false;
	}
} gs_dropEvent_charStone;

static int __DropEvent_CharStone_GetDropPercent(int killer_level)
{
	int killer_levelStep = (killer_level-1)/10;

	switch (killer_levelStep)
	{
		case 0:
			return gs_dropEvent_charStone.percent_lv01_10;

		case 1:
		case 2:
			return gs_dropEvent_charStone.percent_lv11_30;
	}

	return gs_dropEvent_charStone.percent_lv31_MX;
}

static void __DropEvent_CharStone_DropItem(CHARACTER & killer, CHARACTER & victim, ITEM_MANAGER& itemMgr, std::vector<LPITEM>& vec_item)
{
	if (!gs_dropEvent_charStone.alive)
		return;

	int killer_level = killer.GetLevel();
	int dropPercent = __DropEvent_CharStone_GetDropPercent(killer_level);

	int MaxRange = 10000;

	if (random_number(1, MaxRange) <= dropPercent)
	{
		int log_level = (test_server || killer.GetGMLevel() >= GM_LOW_WIZARD) ? 0 : 1;
		int victim_level = victim.GetLevel();
		int level_diff = victim_level - killer_level;

		if (level_diff >= +gs_dropEvent_charStone.level_range || level_diff <= -gs_dropEvent_charStone.level_range)
		{
			sys_log(log_level, 
					"dropevent.drop_char_stone.level_range_over: killer(%s: lv%d), victim(%s: lv:%d), level_diff(%d)",
					killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), level_diff);	
			return;
		}

		static const int Stones[] = { 30210, 30211, 30212, 30213, 30214, 30215, 30216, 30217, 30218, 30219, 30258, 30259, 30260, 30261, 30262, 30263 };
		int item_vnum = Stones[random_number(0, _countof(Stones))];

		LPITEM p_item = NULL;

		if ((p_item = itemMgr.CreateItem(item_vnum, 1, 0, true)))
		{
			vec_item.push_back(p_item);

			sys_log(log_level, 
					"dropevent.drop_char_stone.item_drop: killer(%s: lv%d), victim(%s: lv:%d), item_name(%s)",
					killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), p_item->GetName());	
		}
	}
}

bool DropEvent_CharStone_SetValue(const std::string& name, int value)
{
	if (name == "drop_char_stone")
	{
		gs_dropEvent_charStone.alive = value;

		if (value)
			sys_log(0, "dropevent.drop_char_stone = on");
		else
			sys_log(0, "dropevent.drop_char_stone = off");

	}
	else if (name == "drop_char_stone.percent_lv01_10")
		gs_dropEvent_charStone.percent_lv01_10 = value;
	else if (name == "drop_char_stone.percent_lv11_30")
		gs_dropEvent_charStone.percent_lv11_30 = value;
	else if (name == "drop_char_stone.percent_lv31_MX")
		gs_dropEvent_charStone.percent_lv31_MX = value;
	else if (name == "drop_char_stone.level_range")
		gs_dropEvent_charStone.level_range = value;
	else
		return false;

	sys_log(0, "dropevent.drop_char_stone: %d", gs_dropEvent_charStone.alive ? true : false);
	sys_log(0, "dropevent.drop_char_stone.percent_lv01_10: %f", gs_dropEvent_charStone.percent_lv01_10/100.0f);
	sys_log(0, "dropevent.drop_char_stone.percent_lv11_30: %f", gs_dropEvent_charStone.percent_lv11_30/100.0f);
	sys_log(0, "dropevent.drop_char_stone.percent_lv31_MX: %f", gs_dropEvent_charStone.percent_lv31_MX/100.0f);
	sys_log(0, "dropevent.drop_char_stone.level_range: %d", gs_dropEvent_charStone.level_range);

	return true;
}

// END_OF_DROPEVENT_CHARSTONE

// fixme
// À§ÀÇ °Í°ú ÇÔ²² quest·Î »¬°Í »©º¸ÀÚ. 
// ÀÌ°Å ³Ê¹« ´õ·´ÀÝ¾Æ...
// ”?. ÇÏµåÄÚµù ½È´Ù ¤Ì¤Ð
// °è·® ¾ÆÀÌÅÛ º¸»ó ½ÃÀÛ.
// by rtsummit °íÄ¡ÀÚ ÁøÂ¥
static struct DropEvent_RefineBox
{
	int percent_low;
	int low;
	int percent_mid;
	int mid;
	int percent_high;
	//int level_range;
	bool alive;

	DropEvent_RefineBox()
	{
		percent_low =  100;
		low = 20;
		percent_mid =  100;
		mid = 45;
		percent_high =  100;
		//level_range = 10;
		alive = false;
	}
} gs_dropEvent_refineBox;

static LPITEM __DropEvent_RefineBox_GetDropItem(CHARACTER & killer, CHARACTER & victim, ITEM_MANAGER& itemMgr)
{
	static const int lowerBox[] = { 50197, 50198, 50199 };
	static const int lowerBox_range = 3;
	static const int midderBox[] = { 50203, 50204, 50205, 50206 };
	static const int midderBox_range = 4;
	static const int higherBox[] = { 50207, 50208, 50209, 50210, 50211 };
	static const int higherBox_range = 5;

	if (victim.GetMobRank() < MOB_RANK_KNIGHT)
		return NULL;

	int killer_level = killer.GetLevel();
	//int level_diff = victim_level - killer_level;

	//if (level_diff >= +gs_dropEvent_refineBox.level_range || level_diff <= -gs_dropEvent_refineBox.level_range)
	//{
	//	sys_log(log_level, 
	//		"dropevent.drop_refine_box.level_range_over: killer(%s: lv%d), victim(%s: lv:%d), level_diff(%d)",
	//		killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), level_diff);	
	//	return NULL;
	//}

	if (killer_level <= gs_dropEvent_refineBox.low)
	{
		if (random_number (1, gs_dropEvent_refineBox.percent_low) == 1)
		{
			return itemMgr.CreateItem(lowerBox [random_number (1,lowerBox_range) - 1], 1, 0, true);
		}
	}
	else if (killer_level <= gs_dropEvent_refineBox.mid)
	{
		if (random_number (1, gs_dropEvent_refineBox.percent_mid) == 1)
		{
			return itemMgr.CreateItem(midderBox [random_number (1,midderBox_range) - 1], 1, 0, true);
		}
	}
	else
	{
		if (random_number (1, gs_dropEvent_refineBox.percent_high) == 1)
		{
			return itemMgr.CreateItem(higherBox [random_number (1,higherBox_range) - 1], 1, 0, true);
		}
	}
	return NULL;
}

static void __DropEvent_RefineBox_DropItem(CHARACTER & killer, CHARACTER & victim, ITEM_MANAGER& itemMgr, std::vector<LPITEM>& vec_item)
{
	if (!gs_dropEvent_refineBox.alive)
		return;

	int log_level = (test_server || killer.GetGMLevel() >= GM_LOW_WIZARD) ? 0 : 1;

	LPITEM p_item = __DropEvent_RefineBox_GetDropItem(killer, victim, itemMgr);

	if (p_item)
	{
		vec_item.push_back(p_item);

		sys_log(log_level, 
			"dropevent.drop_refine_box.item_drop: killer(%s: lv%d), victim(%s: lv:%d), item_name(%s)",
			killer.GetName(), killer.GetLevel(), victim.GetName(), victim.GetLevel(), p_item->GetName());	
	}
}

bool DropEvent_RefineBox_SetValue(const std::string& name, int value)
{
	if (name == "refine_box_drop")
	{
		gs_dropEvent_refineBox.alive = value;

		if (value)
			sys_log(0, "refine_box_drop = on");
		else
			sys_log(0, "refine_box_drop = off");

	}
	else if (name == "refine_box_low")
		gs_dropEvent_refineBox.percent_low = value < 100 ? 100 : value;
	else if (name == "refine_box_mid")
		gs_dropEvent_refineBox.percent_mid = value < 100 ? 100 : value;
	else if (name == "refine_box_high")
		gs_dropEvent_refineBox.percent_high = value < 100 ? 100 : value;
	//else if (name == "refine_box_level_range")
	//	gs_dropEvent_refineBox.level_range = value;
	else
		return false;

	sys_log(0, "refine_box_drop: %d", gs_dropEvent_refineBox.alive ? true : false);
	sys_log(0, "refine_box_low: %d", gs_dropEvent_refineBox.percent_low);
	sys_log(0, "refine_box_mid: %d", gs_dropEvent_refineBox.percent_mid);
	sys_log(0, "refine_box_high: %d", gs_dropEvent_refineBox.percent_high);
	//sys_log(0, "refine_box_low_level_range: %d", gs_dropEvent_refineBox.level_range);

	return true;
}
// °³·® ¾ÆÀÌÅÛ º¸»ó ³¡.


void ITEM_MANAGER::CreateQuestDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item, int iDeltaPercent, int iRandRange)
{
	LPITEM item = NULL;

	if (!pkChr)
		return;

	if (!pkKiller)
		return;

	sys_log(1, "CreateQuestDropItem victim(%s), killer(%s)", pkChr->GetName(), pkKiller->GetName() );

	// DROPEVENT_CHARSTONE
	__DropEvent_CharStone_DropItem(*pkKiller, *pkChr, *this, vec_item);
	// END_OF_DROPEVENT_CHARSTONE
	__DropEvent_RefineBox_DropItem(*pkKiller, *pkChr, *this, vec_item);

	// Å©¸®½º¸¶½º ¾ç¸»
	if (quest::CQuestManager::instance().GetEventFlag("xmas_sock"))
	{
		//const DWORD SOCK_ITEM_VNUM = 50010;
		DWORD	SOCK_ITEM_VNUM	= 50010;

		int iDropPerKill[MOB_RANK_MAX_NUM] =
		{
			2000,
			1000,
			300,
			50,
			0,
			0,
		};

		if ( iDropPerKill[pkChr->GetMobRank()] != 0 )
		{
			int iPercent = 40000 * iDeltaPercent / iDropPerKill[pkChr->GetMobRank()];

			sys_log(0, "SOCK DROP %d %d", iPercent, iRandRange);
			if (iPercent >= random_number(1, iRandRange))
			{
				if ((item = CreateItem(SOCK_ITEM_VNUM, 1, 0, true)))
					vec_item.push_back(item);
			}
		}
	}

	// ¿ù±¤ º¸ÇÕ
	if (quest::CQuestManager::instance().GetEventFlag("drop_moon"))
	{
		const DWORD ITEM_VNUM = 50011;

		int iDropPerKill[MOB_RANK_MAX_NUM] =
		{
			2000,
			1200,
			800,
			300,
			0,
			0,
		};

		if (iDropPerKill[pkKiller->GetMobRank()])
		{
			int iPercent = 9000 * iDeltaPercent / iDropPerKill[pkKiller->GetMobRank()];

			if (iPercent >= random_number(1, iRandRange))
			{
				if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
					vec_item.push_back(item);
			}
		}
	}

	if (pkKiller->GetLevel() >= 15 && abs(pkKiller->GetLevel() - pkChr->GetLevel()) <= 5)
	{
		int pct = quest::CQuestManager::instance().GetEventFlag("hc_drop");

		if (pct > 0)
		{
			const DWORD ITEM_VNUM = 30178;

			if (random_number(1,100) <= pct)
			{
				if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
					vec_item.push_back(item);
			}
		}
	}

	//À°°¢º¸ÇÕ
	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "2006_drop") >= random_number(1, iRandRange))
	{
		sys_log(0, "À°°¢º¸ÇÕ DROP EVENT ");

		const static DWORD dwVnum = 50037;

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);

	}

	//À°°¢º¸ÇÕ+
	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "2007_drop") >= random_number(1, iRandRange))
	{
		sys_log(0, "À°°¢º¸ÇÕ DROP EVENT ");

		const static DWORD dwVnum = 50043;

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	// »õÇØ ÆøÁ× ÀÌº¥Æ®
	if (GetDropPerKillPct(/* minimum */ 100, /* default */ 1000, iDeltaPercent, "newyear_fire") >= random_number(1, iRandRange))
	{
		// Áß±¹Àº ÆøÁ×, ÇÑ±¹ ÆØÀÌ
		const DWORD ITEM_VNUM_FIRE = 50107;

		if ((item = CreateItem(ITEM_VNUM_FIRE, 1, 0, true)))
			vec_item.push_back(item);
	}

	// »õÇØ ´ëº¸¸§ ¿ø¼Ò ÀÌº¥Æ®
	if (GetDropPerKillPct(100, 500, iDeltaPercent, "newyear_moon") >= random_number(1, iRandRange))
	{
		sys_log(0, "EVENT NEWYEAR_MOON DROP");

		const static DWORD wonso_items[6] = { 50016, 50017, 50018, 50019, 50019, 50019, };
		DWORD dwVnum = wonso_items[random_number(0,5)];

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	// ¹ß·»Å¸ÀÎ µ¥ÀÌ ÀÌº¥Æ®. OGEÀÇ ¿ä±¸¿¡ µû¶ó event ÃÖ¼Ò°ªÀ» 1·Î º¯°æ.(´Ù¸¥ ÀÌº¥Æ®´Â ÀÏ´Ü ±×´ë·Î µÒ.)
	if (GetDropPerKillPct(1, 2000, iDeltaPercent, "valentine_drop") >= random_number(1, iRandRange))
	{
		sys_log(0, "EVENT VALENTINE_DROP");

		const static DWORD valentine_items[2] = { 50024, 50025 };
		DWORD dwVnum = valentine_items[random_number(0, 1)];

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	// ¾ÆÀÌ½ºÅ©¸² ÀÌº¥Æ®
	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "icecream_drop") >= random_number(1, iRandRange))
	{
		const static DWORD icecream = 50123;

		if ((item = CreateItem(icecream, 1, 0, true)))
			vec_item.push_back(item);
	}

	// new Å©¸®½º¸¶½º ÀÌº¥Æ®
	// 53002 : ¾Æ±â ¼ø·Ï ¼ÒÈ¯±Ç
	if ((pkKiller->CountSpecifyItem(53002) > 0) && (GetDropPerKillPct(50, 100, iDeltaPercent, "new_xmas_event") >= random_number(1, iRandRange)))
	{
		const static DWORD xmas_sock = 50010;
		pkKiller->AutoGiveItem (xmas_sock, 1);
	}

	if ((pkKiller->CountSpecifyItem(53007) > 0) && (GetDropPerKillPct(50, 100, iDeltaPercent, "new_xmas_event") >= random_number(1, iRandRange)))
	{
		const static DWORD xmas_sock = 50010;
		pkKiller->AutoGiveItem (xmas_sock, 1);
	}

	if ( GetDropPerKillPct(100, 2000, iDeltaPercent, "halloween_drop") >= random_number(1, iRandRange) )
	{
		const static DWORD halloween_item = 30321;

		if ( (item=CreateItem(halloween_item, 1, 0, true)) )
			vec_item.push_back(item);
	}
	
	if ( GetDropPerKillPct(100, 2000, iDeltaPercent, "ramadan_drop") >= random_number(1, iRandRange) )
	{
		const static DWORD ramadan_item = 30315;

		if ( (item=CreateItem(ramadan_item, 1, 0, true)) )
			vec_item.push_back(item);
	}

	if ( GetDropPerKillPct(100, 2000, iDeltaPercent, "easter_drop") >= random_number(1, iRandRange) )
	{
		const static DWORD easter_item_base = 50160;

		if ( (item=CreateItem(easter_item_base+random_number(0,19), 1, 0, true)) )
			vec_item.push_back(item);
	}

	// ¿ùµåÄÅ ÀÌº¥Æ®
	if ( GetDropPerKillPct(100, 2000, iDeltaPercent, "football_drop") >= random_number(1, iRandRange) )
	{
		const static DWORD football_item = 50096;

		if ( (item=CreateItem(football_item, 1, 0, true)) )
			vec_item.push_back(item);
	}

	// È­ÀÌÆ® µ¥ÀÌ ÀÌº¥Æ®
	if (GetDropPerKillPct(100, 2000, iDeltaPercent, "whiteday_drop") >= random_number(1, iRandRange))
	{
		sys_log(0, "EVENT WHITEDAY_DROP");
		const static DWORD whiteday_items[2] = { ITEM_WHITEDAY_ROSE, ITEM_WHITEDAY_CANDY };
		DWORD dwVnum = whiteday_items[random_number(0,1)];

		if ((item = CreateItem(dwVnum, 1, 0, true)))
			vec_item.push_back(item);
	}

	// ¾î¸°ÀÌ³¯ ¼ö¼ö²²³¢ »óÀÚ ÀÌº¥Æ®
	if (pkKiller->GetLevel()>=50)
	{
		if (GetDropPerKillPct(100, 1000, iDeltaPercent, "kids_day_drop_high") >= random_number(1, iRandRange))
		{
			DWORD ITEM_QUIZ_BOX = 50034;

			if ((item = CreateItem(ITEM_QUIZ_BOX, 1, 0, true)))
				vec_item.push_back(item);
		}
	}
	else
	{
		if (GetDropPerKillPct(100, 1000, iDeltaPercent, "kids_day_drop") >= random_number(1, iRandRange))
		{
			DWORD ITEM_QUIZ_BOX = 50034;

			if ((item = CreateItem(ITEM_QUIZ_BOX, 1, 0, true)))
				vec_item.push_back(item);
		}
	}

	// ¿Ã¸²ÇÈ µå·Ó ÀÌº¥Æ®
	if (pkChr->GetLevel() >= 30 && GetDropPerKillPct(50, 100, iDeltaPercent, "medal_part_drop") >= random_number(1, iRandRange))
	{
		const static DWORD drop_items[] = { 30265, 30266, 30267, 30268, 30269 };
		int i = random_number (0, 4);
		item = CreateItem(drop_items[i]);
		if (item != NULL)
			vec_item.push_back(item);
	}

	// ADD_GRANDMASTER_SKILL
	// È¥¼® ¾ÆÀÌÅÛ µå·Ó
	if (pkChr->GetLevel() >= 40 && pkChr->GetMobRank() >= MOB_RANK_BOSS && GetDropPerKillPct(/* minimum */ 1, /* default */ 1000, iDeltaPercent, "three_skill_item") / GetThreeSkillLevelAdjust(pkChr->GetLevel()) >= random_number(1, iRandRange))
	{
		if (pkChr->GetLevel() < 90)
		{		
			const DWORD ITEM_VNUM = 50513;

			if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
				vec_item.push_back(item);
		}
	}
	// END_OF_ADD_GRANDMASTER_SKILL

#ifdef AELDRA
	// MITHRIL
	if (((pkChr->GetRaceNum() >= 8051 && pkChr->GetRaceNum() <= 8058) || (pkChr->GetLevel() >= 95 && pkChr->GetLevel() < 115)) 
		&& pkChr->GetMobRank() >= MOB_RANK_BOSS && GetDropPerKillPct(/* minimum */ 1, /* default */ 1000, iDeltaPercent, "mithril_drop_item") / GetThreeSkillLevelAdjust(pkChr->GetLevel()) >= random_number(1, iRandRange))
	{
		const DWORD ITEM_VNUM = 92919;
		if ((item = CreateItem(ITEM_VNUM, 1, 0, true)))
			vec_item.push_back(item);
	}
	// MITHRIL
#endif
	
	// ¹«½ÅÀÇ Ãàº¹¼­¿ë ¸¸³âÇÑÃ¶ drop
	if (pkKiller->GetLevel() >= 15 && quest::CQuestManager::instance().GetEventFlag("mars_drop"))
	{
		const DWORD ITEM_HANIRON = 70035;
		int iDropMultiply[MOB_RANK_MAX_NUM] =
		{
			50,
			30,
			5,
			1,
			0,
			0,
		};

		if (iDropMultiply[pkChr->GetMobRank()] &&
				GetDropPerKillPct(1000, 1500, iDeltaPercent, "mars_drop") >= random_number(1, iRandRange) * iDropMultiply[pkChr->GetMobRank()])
		{
			if ((item = CreateItem(ITEM_HANIRON, 1, 0, true)))
				vec_item.push_back(item);
		}
	}
}

DWORD ITEM_MANAGER::GetRefineFromVnum(DWORD dwVnum)
{
	itertype(m_map_ItemRefineFrom) it = m_map_ItemRefineFrom.find(dwVnum);
	if (it != m_map_ItemRefineFrom.end())
		return it->second;
	return 0;
}

const CSpecialItemGroup* ITEM_MANAGER::GetSpecialItemGroup(DWORD dwVnum)
{
	itertype(m_map_pkSpecialItemGroup) it = m_map_pkSpecialItemGroup.find(dwVnum);
	if (it != m_map_pkSpecialItemGroup.end())
	{
		return it->second;
	}
	return NULL;
}

const CSpecialAttrGroup* ITEM_MANAGER::GetSpecialAttrGroup(DWORD dwVnum)
{
	itertype(m_map_pkSpecialAttrGroup) it = m_map_pkSpecialAttrGroup.find(dwVnum);
	if (it != m_map_pkSpecialAttrGroup.end())
	{
		return it->second;
	}
	return NULL;
}

// pkNewItemÀ¸·Î ¸ðµç ¼Ó¼º°ú ¼ÒÄÏ °ªµéÀ» ¸ñ»çÇÏ´Â ÇÔ¼ö.
// ±âÁ¸¿¡ char_item.cpp ÆÄÀÏ¿¡ ÀÖ´ø ·ÎÄÃÇÔ¼öÀÎ TransformRefineItem ±×´ë·Î º¹»çÇÔ
void ITEM_MANAGER::CopyAllAttrTo(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		// ¿©±â¼­ ±úÁø¼®ÀÌ ÀÚµ¿ÀûÀ¸·Î Ã»¼Ò µÊ
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		// ¼ÒÄÏ ¼³Á¤
		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);
			const int ITEM_BROKEN_METIN_VNUM = 28960; // ÀÌ°Ç ¹¹ ¶È°°Àº »ó¼ö°¡ 3±ºµ¥³ª ÀÖ³Ä... ÇÏ³ª·Î ÇØ³õÁö¤Ð¤Ð¤Ð ³ª´Â ÆÐ½º È«ÀÌ ÇÒ²¨ÀÓ
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}

	}

	pkNewItem->SetGMOwner(pkOldItem->IsGMOwner());

#ifdef __ALPHA_EQUIP__
	pkNewItem->LoadAlphaEquipValue(pkOldItem->GetRealAlphaEquipValue());
#endif

	// ¸ÅÁ÷ ¾ÆÀÌÅÛ ¼³Á¤
	pkOldItem->CopyAttributeTo(pkNewItem);
}

bool ITEM_MANAGER::GetVnumRangeByString(const std::string& stVnumRange, DWORD & r_dwVnumStart, DWORD & r_dwVnumEnd)
{
	int iPos;

	if ((iPos = stVnumRange.find("~")) > 0)
	{
		std::string vnum_start, vnum_end;
		vnum_start.assign(stVnumRange, 0, iPos);
		vnum_end.assign(stVnumRange, iPos + 1, stVnumRange.length());

		if (!str_is_number(vnum_start.c_str()) || !str_is_number(vnum_end.c_str()))
			return false;

		int iRange[2];

		str_to_number(iRange[0], vnum_start.c_str());
		str_to_number(iRange[1], vnum_end.c_str());
		if (iRange[0] > iRange[1])
		{
			int iFirstVnum = iRange[0];
			iRange[0] = iRange[1];
			iRange[1] = iFirstVnum;
		}

		r_dwVnumStart = iRange[0];
		r_dwVnumEnd = iRange[1];

		return true;
	}

	return false;
}

DWORD ITEM_MANAGER::GetItemRealTimeout(const network::TItemTable* proto, const google::protobuf::RepeatedField<google::protobuf::int32>& sockets)
{
	if (proto->limit_real_time_first_use_index() != -1)
		return sockets[0];

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (proto->limits(i).type() == LIMIT_REAL_TIME)
			return sockets[0];
	}

	return 0;
}

DWORD ITEM_MANAGER::GetInventoryPageSize(BYTE bWindow)
{
	switch (bWindow)
	{
	case INVENTORY:
		return INVENTORY_PAGE_SIZE;

#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
		return SKILLBOOK_INV_PAGE_SIZE;
#endif

	case UPPITEM_INVENTORY:
		return UPPITEM_INV_PAGE_SIZE;

	case STONE_INVENTORY:
		return STONE_INV_PAGE_SIZE;

	case ENCHANT_INVENTORY:
		return ENCHANT_INV_PAGE_SIZE;

#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
		return COSTUME_INV_PAGE_SIZE;
#endif

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		return DRAGON_SOUL_INVENTORY_MAX_NUM;
#endif
	}

	return 0;
}

DWORD ITEM_MANAGER::GetInventoryMaxNum(BYTE bWindow, const LPCHARACTER ch)
{
	switch (bWindow)
	{
	case INVENTORY:
		return ch ? ch->GetInventoryMaxNum() : INVENTORY_MAX_NUM;

#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
		return SKILLBOOK_INV_MAX_NUM;
#endif

	case UPPITEM_INVENTORY:
		return ch ? ch->GetUppitemInventoryMaxNum() : UPPITEM_INV_MAX_NUM;

	case STONE_INVENTORY:
		return STONE_INV_MAX_NUM;

	case ENCHANT_INVENTORY:	
		return ch ? ch->GetEnchantInventoryMaxNum() : ENCHANT_INV_MAX_NUM;

#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
		return COSTUME_INV_MAX_NUM;
#endif

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		return DRAGON_SOUL_INVENTORY_MAX_NUM;
#endif
	}

	return 0;
}

DWORD ITEM_MANAGER::GetInventoryStart(BYTE bWindow)
{
	switch (bWindow)
	{
	case INVENTORY:
		return INVENTORY_SLOT_START;

#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
		return SKILLBOOK_INV_SLOT_START;
#endif

	case UPPITEM_INVENTORY:
		return UPPITEM_INV_SLOT_START;

	case STONE_INVENTORY:
		return STONE_INV_SLOT_START;

	case ENCHANT_INVENTORY:
		return ENCHANT_INV_SLOT_START;

#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
		return COSTUME_INV_SLOT_START;
#endif

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		return 0;
#endif
	}

	return 0;
}

BYTE ITEM_MANAGER::GetTargetWindow(LPITEM pkItem)
{
	return GetTargetWindow(pkItem->GetVnum(), pkItem->GetType(), pkItem->GetSubType());
}

BYTE ITEM_MANAGER::GetTargetWindow(DWORD dwVnum)
{
	auto* pProto = GetTable(dwVnum);
	if (!pProto)
	{
		sys_err("invalid GetTargetWindow on vnum %u (no table)", dwVnum);
		return INVENTORY;
	}

	return GetTargetWindow(dwVnum, pProto->type(), pProto->sub_type());
}

BYTE ITEM_MANAGER::GetTargetWindow(DWORD dwVnum, BYTE bItemType, BYTE bItemSubType)
{
#ifdef __SKILLBOOK_INVENTORY__
	if
	(
	(bItemType == ITEM_SKILLBOOK) ||
	dwVnum == 50060 ||
	dwVnum == 71001 ||
	dwVnum == 71094 ||
	dwVnum == 50513 ||
	(dwVnum >= 50301 && dwVnum <= 50306) ||
	(dwVnum >= 50314 && dwVnum <= 50316) ||
	(dwVnum >= 92206 && dwVnum <= 92211) ||
	(dwVnum >= 93261 && dwVnum <= 93263) ||
	(dwVnum >= 54017 && dwVnum <= 54018)
#ifdef ELONIA
	 ||	dwVnum == 70102
#endif
#ifdef AELDRA
	 || (dwVnum >= 95011 && dwVnum <= 95013)
#endif
	)
		//	return bItemSubType == SKILLBOOK_NORMAL ? SKILLBOOK_INVENTORY : MASTER_SKILLBOOK_INVENTORY;
		return SKILLBOOK_INVENTORY;
#endif
	if (bItemType == ITEM_METIN)
		return STONE_INVENTORY;
#ifdef __DRAGONSOUL__
	if (bItemType == ITEM_DS)
		return DRAGON_SOUL_INVENTORY;
#endif

	if (IsUppItem(dwVnum))
		return UPPITEM_INVENTORY;
	if (dwVnum == 71084 || dwVnum == 95010 || dwVnum == 71085 || dwVnum == 76023 || dwVnum == 76024 || dwVnum == 71151 || dwVnum == 71152 || dwVnum == 76013 || dwVnum == 76014 || dwVnum == 70024 
#ifdef AELDRA
		|| (dwVnum >= 92345 && dwVnum <= 92386) || (dwVnum >= 93437 && dwVnum <= 93450) || dwVnum == 94253 || (dwVnum >= 93345 && dwVnum <= 93358) || (dwVnum >= 92800 && dwVnum <= 92841) || dwVnum == 95010 || dwVnum == 92878 || (dwVnum >= 92300 && dwVnum <= 92344) 
#endif
#ifdef ELONIA
		|| (dwVnum >= 54000 && dwVnum <= 54015)
#endif
		)
		return ENCHANT_INVENTORY;

#ifdef __COSTUME_INVENTORY__
	if(( bItemType == ITEM_COSTUME && bItemSubType != COSTUME_ACCE )|| bItemType == ITEM_SHINING)
		return COSTUME_INVENTORY;
#endif

	return INVENTORY;
}

const char* ITEM_MANAGER::GetTargetWindowName(BYTE bWindow)
{
	switch (bWindow)
	{
	case INVENTORY:
		return "inventory";
	case EQUIPMENT:
		return "equipment";
#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
		return "skillbook storage";
#endif
	case UPPITEM_INVENTORY:
		return "uppitem storage";
	case STONE_INVENTORY:
		return "stone storage";
	case ENCHANT_INVENTORY:
		return "enchant storage";
#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
		return "costume storage";
#endif

	}

	return "noname-window";
}

bool ITEM_MANAGER::GetInventorySlotRange(BYTE bWindow, WORD& wSlotStart, WORD& wSlotEnd, const LPCHARACTER ch)
{
	wSlotStart = GetInventoryStart(bWindow);
	wSlotEnd = wSlotStart + GetInventoryMaxNum(bWindow, ch);

	return wSlotEnd != wSlotStart;
}

bool ITEM_MANAGER::GetInventorySlotRange(BYTE& bWindow, WORD& wSlotStart, WORD& wSlotEnd, bool bSetInvOnErr, const LPCHARACTER ch)
{
	if (!GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, ch))
	{
		if (bSetInvOnErr)
		{
			bWindow = INVENTORY;
			GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, ch);
		}
		return false;
	}

	return true;
}

struct FRefreshDisableItemsOnMap
{
	const std::set<DWORD>* m_pDisabledList;

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER ch = (LPCHARACTER) ent;
		if (!ch->IsPC())
			return;

		ch->CheckForDisabledItems(m_pDisabledList);
	}
};

void ITEM_MANAGER::LoadDisabledItemList()
{
	std::set<long> set_MapIndexes;
	for (auto it = m_map_DisabledItemList.begin(); it != m_map_DisabledItemList.end(); ++it)
		set_MapIndexes.insert(it->first);

	m_map_DisabledItemList.clear();

	std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT map_index, item_vnum FROM disable_item_proto"));
	while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		long lMapIndex;
		DWORD dwItemVnum;
		str_to_number(lMapIndex, row[0]);
		str_to_number(dwItemVnum, row[1]);

		m_map_DisabledItemList[lMapIndex].insert(dwItemVnum);
		set_MapIndexes.insert(lMapIndex);
	}

	for (auto it = set_MapIndexes.begin(); it != set_MapIndexes.end(); ++it)
	{
		FRefreshDisableItemsOnMap f;
		f.m_pDisabledList = GetDisabledItemList(*it);

		SECTREE_MAP* pkMap = SECTREE_MANAGER::instance().GetMap(*it);
		if (pkMap)
			pkMap->for_each(f);
	}
}

const std::set<DWORD>* ITEM_MANAGER::GetDisabledItemList(long lMapIndex)
{
	if (lMapIndex >= 10000)
		lMapIndex /= 10000;

	auto it = m_map_DisabledItemList.find(lMapIndex);
	if (it == m_map_DisabledItemList.end())
		return NULL;

	return &it->second;
}

bool ITEM_MANAGER::IsDisabledItem(DWORD dwVnum, long lMapIndex)
{
	auto pkSet = GetDisabledItemList(lMapIndex);
	if (!pkSet)
		return false;

	return pkSet->find(dwVnum) != pkSet->end();
}

bool ITEM_MANAGER::IsNewWindow(BYTE bWindow)
{
	switch (bWindow)
	{
#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
#endif
	case UPPITEM_INVENTORY:
	case STONE_INVENTORY:
	case ENCHANT_INVENTORY:
#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
#endif
		return true;
	}

	return false;
}

BYTE ITEM_MANAGER::GetWindowBySlot(WORD wSlotPos)
{
	for (int i = 0; i < WINDOW_MAX_NUM; ++i)
	{
#ifdef __DRAGONSOUL__
		if (i == DRAGON_SOUL_INVENTORY)
			continue;
#endif

		WORD wStartPos, wEndPos;
		if (GetInventorySlotRange(i, wStartPos, wEndPos))
		{
			if (wSlotPos >= wStartPos && wSlotPos < wEndPos)
				return i;
		}
	}

	return INVENTORY;
}

void ITEM_MANAGER::LoadUppItemList()
{
	DBManager::Instance().ReturnQuery(QID_UPPITEM_LIST_RELOAD, 0, NULL, "SELECT vnum FROM uppitem_proto");
}

void ITEM_MANAGER::OnLoadUppItemList(MYSQL_RES* pSQSLRes)
{
	m_set_UppItemVnums.clear();

	while (MYSQL_ROW row = mysql_fetch_row(pSQSLRes))
		m_set_UppItemVnums.insert(atoi(row[0]));
}

bool ITEM_MANAGER::IsUppItem(DWORD dwVnum) const
{
	return m_set_UppItemVnums.find(dwVnum) != m_set_UppItemVnums.end();
}

const char* ITEM_MANAGER::GetItemLink(DWORD dwItemVnum)
{
	static char s_szBuf[512], s_szOutputBuf[512];
	int len = 0;

	auto* pItemTable = GetTable(dwItemVnum);
	if (!pItemTable)
		return NULL;

	len += snprintf(s_szBuf, sizeof(s_szBuf), "item:%x:%x:%x:%x:%x:%x",
		dwItemVnum, pItemTable->flags(), 0, 0, 0, 0);

	snprintf(s_szOutputBuf, sizeof(s_szOutputBuf), "|cfff1e6c0|H%s|h[%s]|h|r", s_szBuf, pItemTable->locale_name(LANGUAGE_DEFAULT).c_str());
	return s_szOutputBuf;
}

#ifdef INGAME_WIKI
network::TWikiInfoTable* ITEM_MANAGER::GetItemWikiInfo(DWORD vnum)
{
	auto it = m_wikiInfoMap.find(vnum);
	if (it != m_wikiInfoMap.end())
		return it->second.get();

	auto* tbl = GetTable(vnum);
	if (!tbl)
		return NULL;

	auto newTable = new network::TWikiInfoTable();
	newTable->set_is_common(false);
	for (int it = 0; it < MOB_RANK_MAX_NUM && !newTable->is_common(); ++it)
		for (auto it2 = g_vec_pkCommonDropItem[it].begin(); it2 != g_vec_pkCommonDropItem[it].end() && !newTable->is_common(); ++it2)
			if (it2->m_dwVnumStart >= vnum && it2->m_dwVnumEnd <= vnum)
				newTable->set_is_common(true);

	newTable->set_origin_vnum(0);
	m_wikiInfoMap.insert(std::make_pair(vnum, std::unique_ptr<network::TWikiInfoTable>(newTable)));

	if (vnum == 94326 || (tbl->type() == ITEM_WEAPON || tbl->type() == ITEM_ARMOR || tbl->type() == ITEM_BELT) && vnum % 10 == 0 && tbl->refined_vnum())
	{
		network::TWikiRefineInfo tempRef[9];
		const network::TRefineTable* refTbl;
		auto* tblTemp = tbl;
		bool success = true;
		for (BYTE i = 0; i < 9; ++i)
		{
			if (!tblTemp)
			{
				success = false;
				break;
			}

			refTbl = CRefineManager::instance().GetRefineRecipe(tblTemp->refine_set());
			if (!refTbl)
			{
				success = false;
				break;
			}

			for (auto j = 0; j < refTbl->material_count(); ++j)
				*tempRef[i].add_materials() = refTbl->materials(j);
			tempRef[i].set_price(refTbl->cost());

			tblTemp = GetTable(tblTemp->vnum() + 1);
		}

		if (success)
		{
			for (int i = 0; i < 9; ++i)
				*newTable->add_refine_infos() = tempRef[i];
		}
	}
	else if (tbl->type() == ITEM_GIFTBOX || tbl->type() == ITEM_USE && tbl->sub_type() == USE_SPECIAL)
	{
		CSpecialItemGroup * ptr = NULL;
		auto it = m_map_pkSpecialItemGroup.find(vnum);
		if (it == m_map_pkSpecialItemGroup.end())
		{
			it = m_map_pkQuestItemGroup.find(vnum);
			if (it != m_map_pkQuestItemGroup.end())
				ptr = it->second;
		}
		else
			ptr = it->second;

		if (ptr && (!ptr->m_vecItems.empty() || !ptr->m_vec100PctItems.empty()))
		{
			for (auto it2 : ptr->m_vecItems)
				if (it2.vnum_start > ptr->POLY_MARBLE)
				{
					auto t = newTable->add_chest_infos();
					t->set_vnum_start(it2.vnum_start);
					t->set_vnum_end(it2.vnum_end);
				}
				else if (it2.vnum_start == ptr->POLY_MARBLE)
				{
					auto t = newTable->add_chest_infos();
					t->set_vnum_start(it2.vnum_start);
					t->set_vnum_end(it2.count);
				}

			for (auto it2 : ptr->m_vec100PctItems)
				if (it2.vnum_start > ptr->POLY_MARBLE)
				{
					auto t = newTable->add_chest_infos();
					t->set_vnum_start(it2.vnum_start);
					t->set_vnum_end(it2.vnum_end);
				}
				else if (it2.vnum_start == ptr->POLY_MARBLE)
				{
					auto t = newTable->add_chest_infos();
					t->set_vnum_start(it2.vnum_start);
					t->set_vnum_end(it2.count);
				}
		}
	}

	return newTable;
}
#endif

void ITEM_MANAGER::GetPlayerItem(LPITEM item, network::TItemData* pRet)
{
	pRet->set_id(item->GetID());
	*pRet->mutable_cell() = TItemPos(item->GetWindow(), item->GetCell());
	pRet->set_count(item->GetCount());
	pRet->set_vnum(item->GetOriginalVnum());
	switch (pRet->cell().window_type())
	{
		case SAFEBOX:
		case MALL:
			pRet->set_owner(item->GetOwner()->GetDesc()->GetAccountTable().id());
			break;
		default:
			if (item->GetOwner())
				pRet->set_owner(item->GetOwner()->GetPlayerID());
			else
				pRet->set_owner(item->GetRealOwnerPID() ? item->GetRealOwnerPID() : item->GetLastOwnerPID());
			break;
	}

	pRet->set_is_gm_owner(item->IsGMOwner());
#ifdef __ALPHA_EQUIP__
	pRet->set_alpha_equip_value(item->GetRealAlphaEquipValue());
#endif

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pRet->add_sockets(item->GetSocket(i));
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		*pRet->add_attributes() = item->GetAttribute(i);

#ifdef CRYSTAL_SYSTEM
	if (item->GetType() == ITEM_CRYSTAL && static_cast<ECrystalItem>(item->GetSubType()) == ECrystalItem::CRYSTAL)
		pRet->set_sockets(CItem::CRYSTAL_TIME_SOCKET, item->get_crystal_duration());
#endif

#ifdef __PET_ADVANCED__
	if (item->GetAdvancedPet())
	{
		item->GetAdvancedPet()->CopyPetTable(pRet->mutable_pet_info());
	}
#endif
}

void ITEM_MANAGER::GiveItemRefund(const network::TItemData* pTab)
{
	DBManager::instance().Query("INSERT INTO item_refund (owner_id, vnum, count, "
		"socket0, socket1, socket2, socket_set, "
		"attrtype0, attrvalue0, attrtype1, attrvalue1, attrtype2, attrvalue2, attrtype3, attrvalue3, attrtype4, attrvalue4, "
		"given_time) VALUES (%u, %u, %u, "
		"%d, %d, %d, 1, "
		"%u, %d, %u, %d, %u, %d, %u, %d, %u, %d, NOW())",
		pTab->owner(), pTab->vnum(), pTab->count(),
		pTab->sockets(0), pTab->sockets(1), pTab->sockets(2),
		pTab->attributes(0).type(), pTab->attributes(0).value(),
		pTab->attributes(1).type(), pTab->attributes(1).value(),
		pTab->attributes(2).type(), pTab->attributes(2).value(),
		pTab->attributes(3).type(), pTab->attributes(3).value(),
		pTab->attributes(4).type(), pTab->attributes(4).value());
}

void ITEM_MANAGER::GiveGoldRefund(DWORD pid, long long gold)
{
	DBManager::instance().Query("INSERT INTO item_refund (owner_id, vnum, count, given_time) VALUES (%u, 1, %lld, NOW())", pid, gold);
}
