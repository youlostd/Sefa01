#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "polymorph.h"
#include "blend_item.h"
#include "arena.h"
#include "dev_log.h"
#include "gm.h"
#include "mount_system.h"
#include "affect.h"
#include "general_manager.h"

#include "safebox.h"
#include "shop.h"

#include "../../common/VnumHelper.h"
#include "buff_on_attributes.h"

#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif

#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif

#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

extern bool CanPutIntoV(LPITEM item, DWORD vnum, BYTE bSubType);

extern bool CAN_ENTER_ZONE_CHECKLEVEL(const LPCHARACTER& ch, int map_index, bool bSendMessage);

// CHANGE_ITEM_ATTRIBUTES
const DWORD CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};


//±ÍÈ¯ºÎ, ±ÍÈ¯±â¾ïºÎ, °áÈ¥¹ÝÁö
static bool IS_SUMMON_ITEM(int vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}

static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case MONKEY_EASY_MAP_INDEX_1:
		case MONKEY_EASY_MAP_INDEX_2:
		case MONKEY_EASY_MAP_INDEX_3:
		case MONKEY_MEDIUM_MAP_INDEX:
		case MONKEY_EXPERT_MAP_INDEX:
			return true;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int map_index)
{
	switch (map_index)
	{
		case DEVILTOWER_MAP_INDEX:
		case DEVILSCATACOMB_MAP_INDEX:
		// case SPIDER_MAP_INDEX_2:
		case OXEVENT_MAP_INDEX:
		// case WEDDING_MAP_INDEX:
			return false;
	}

	if (map_index > 10000)
		return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int nMapIndex)
{
	return true;
}

// item socket ÀÌ ÇÁ·ÎÅäÅ¸ÀÔ°ú °°ÀºÁö Ã¼Å© -- by mhh
static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->sockets(i))
			return false;
	}

	return true;
}

// item socket º¹»ç -- by mhh
static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}

bool CHARACTER::check_item_sex(LPCHARACTER ch, LPITEM item)
{
	// ³²ÀÚ ±ÝÁö
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}
	// ¿©ÀÚ±ÝÁö
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE)) 
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen())
		return false;

	if (IsWarping())
		return false;

#ifdef __ACCE_COSTUME__
	if (IsAcceWindowOpen())
		return false;
#endif

#ifdef __DRAGONSOUL__
	if (DragonSoul_RefineWindow_GetOpener())
		return false;
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
	if (CBT_GetWindowOpener())
		return false;
#endif

	return true;
}

void CHARACTER::EncodeItemPacket(LPITEM item, network::GCOutputPacket<network::GCItemSetPacket>& pack) const
{
	auto& data = *pack->mutable_data();
	ITEM_MANAGER::Instance().GetPlayerItem(item, &data);

	data.set_vnum(item->GetVnum());
}

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}
LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return NULL;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
#endif
	case STONE_INVENTORY:
	case UPPITEM_INVENTORY:
	case ENCHANT_INVENTORY:
#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
#endif
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pItems[wCell];

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pDSItems[wCell];
#endif

	default:
		return NULL;
	}
	return NULL;
}

#ifndef __MARK_NEW_ITEM_SYSTEM__
void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem)
#else
void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem, bool bWereMine)
#endif
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}

#ifdef __FAKE_BUFF__
	if (GetItem(Cell) && CItemVnumHelper::IsFakeBuffSpawn(GetItem(Cell)->GetVnum()))
		FakeBuff_Owner_Despawn();
#endif

	// ±âº» ÀÎº¥Åä¸®
	switch(window_type)
	{
	case INVENTORY:
#ifdef __SKILLBOOK_INVENTORY__
	case SKILLBOOK_INVENTORY:
#endif
	case STONE_INVENTORY:
	case UPPITEM_INVENTORY:
	case ENCHANT_INVENTORY:
#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
#endif
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pItems[wCell];

			if (pOld)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
							continue;

						m_pointsInstant.wItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 ·Î ÇÏ´Â °ÍÀº ºó°÷À» Ã¼Å©ÇÒ ¶§ °°Àº
						// ¾ÆÀÌÅÛÀº ¿¹¿ÜÃ³¸®ÇÏ±â À§ÇÔ
						m_pointsInstant.wItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pItems[wCell] = pItem;
		}
		break;

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pDSItems[wCell];

			if (pOld)
			{
				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
							continue;

						m_pointsInstant.wDSItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
					return;
				}

				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 ·Î ÇÏ´Â °ÍÀº ºó°÷À» Ã¼Å©ÇÒ ¶§ °°Àº
						// ¾ÆÀÌÅÛÀº ¿¹¿ÜÃ³¸®ÇÏ±â À§ÇÔ
						m_pointsInstant.wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pDSItems[wCell] = pItem;
		}
		break;
#endif
	default:
		sys_err ("Invalid Inventory type %d", window_type);
		return;
	}

#ifdef __DRAGONSOUL__
	if (Cell.window_type != DRAGON_SOUL_INVENTORY)
#endif
	{
		Cell.window_type = EQUIPMENT;
		for (int i = 0; i < WINDOW_MAX_NUM; ++i)
		{
			if (i != INVENTORY && !ITEM_MANAGER::instance().IsNewWindow(i))
				continue;

			WORD wSlotStart, wSlotEnd;
			if (!ITEM_MANAGER::instance().GetInventorySlotRange(i, wSlotStart, wSlotEnd, this))
				continue;

			if (wCell >= wSlotStart && wCell < wSlotEnd)
			{
				Cell.window_type = i;
				break;
			}
		}
	}

	if (GetDesc())
	{
		network::GCOutputPacket<network::GCItemSetPacket> pack;

		auto data = pack->mutable_data();

		if (pItem)
		{
			EncodeItemPacket(pItem, pack);

#if defined(__MARK_NEW_ITEM_SYSTEM__) || defined(__DRAGONSOUL__)
			pack->set_highlight(false);
#ifdef __MARK_NEW_ITEM_SYSTEM__
			pack->set_highlight(pack->highlight() || !bWereMine);
#endif
#ifdef __DRAGONSOUL__
			pack->set_highlight(pack->highlight() || (Cell.window_type == DRAGON_SOUL_INVENTORY));
#endif
#endif
		}

		*data->mutable_cell() = Cell;
		GetDesc()->Packet(pack);
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		pItem->SetWindow(Cell.window_type);
	}
// #ifdef __EQUIPMENT_CHANGER__
// 		if (wCell >= INVENTORY_MAX_NUM && wCell < INVENTORY_AND_EQUIP_SLOT_MAX)
// 			UpdateEquipmentChangerItem(wCell - INVENTORY_MAX_NUM);
// #endif
}

LPITEM CHARACTER::GetWear(BYTE bCell) const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetWear(bCell);
#endif

	// > WEAR_MAX_NUM : ¿ëÈ¥¼® ½½·Ôµé.
#ifdef __DRAGONSOUL__
	if (bCell >= WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_MAX_NUM

#ifdef __SKIN_SYSTEM__
		+ SHINING_RESERVED + SKINSYSTEM_MAX_NUM
#endif

		)
#else
	if (bCell >= WEAR_MAX_NUM + SHINING_MAX_NUM

#ifdef __SKIN_SYSTEM__
		+ SHINING_RESERVED + SKINSYSTEM_MAX_NUM
#endif
		)
#endif
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[EQUIPMENT_SLOT_START + (WORD)bCell];
}

void CHARACTER::SetWear(BYTE bCell, LPITEM item)
{
	// > WEAR_MAX_NUM : ¿ëÈ¥¼® ½½·Ôµé.
#ifdef __DRAGONSOUL__
	if (bCell >= WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_MAX_NUM

#ifdef __SKIN_SYSTEM__
		+ SHINING_RESERVED + SKINSYSTEM_MAX_NUM
#endif
		)
#else
	if (bCell >= WEAR_MAX_NUM + SHINING_MAX_NUM
	
#ifdef __SKIN_SYSTEM__
		+ SHINING_RESERVED + SKINSYSTEM_MAX_NUM
#endif
		)
#endif
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

	SetItem(TItemPos (INVENTORY, EQUIPMENT_SLOT_START + (WORD)bCell), item);

	if (!item && bCell == WEAR_WEAPON)
	{
#ifdef KEEP_SKILL_AFFECTS
		if (m_isDestroyed)
			return;
#endif
		// ±Í°Ë »ç¿ë ½Ã ¹þ´Â °ÍÀÌ¶ó¸é È¿°ú¸¦ ¾ø¾Ö¾ß ÇÑ´Ù.
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

#ifdef __LEGENDARY_SKILL__
		if (IsAffectFlag(AFF_GWIGUM_PERFECT))
			RemoveAffect(SKILL_GWIGEOM);
#endif

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);

#ifdef __LEGENDARY_SKILL__
		if (IsAffectFlag(AFF_GEOMGYEONG_PERFECT))
			RemoveAffect(SKILL_GEOMKYUNG);
#endif
	}
}

void CHARACTER::ClearItem()
{
#ifdef KEEP_SKILL_AFFECTS
	m_isDestroyed = true;
#endif

	int		i;
	LPITEM	item;
	
	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}

#ifdef __DRAGONSOUL__
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
#endif
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	switch (Cell.window_type)
	{
	case UPPITEM_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (m_pointsInstant.wItemGrid[wCell] || wCell >= GetUppitemInventoryMaxNum() + UPPITEM_INV_SLOT_START)
				return false;

			if (bSize <= 1)
				return true;
			else
			{
				int j = 1;
				WORD wPage = (wCell - UPPITEM_INV_SLOT_START) / UPPITEM_INV_PAGE_SIZE;

				do
				{
					WORD p = wCell + (5 * j);

					if (p >= GetUppitemInventoryMaxNum() + UPPITEM_INV_SLOT_START)
						return false;

					if ((p - UPPITEM_INV_PAGE_SIZE) / UPPITEM_INV_PAGE_SIZE != wPage)
						return false;

				} while (++j < bSize);

				return true;
			}
			return true;
		}
#ifdef __COSTUME_INVENTORY__
	case COSTUME_INVENTORY:
	{
		WORD wCell = Cell.cell;

		if(wCell >= COSTUME_INV_SLOT_END)
			return false;

		if(m_pointsInstant.wItemGrid[ wCell ])
			return false;
		else
		{
			int j = 1;

			// Opened page
			WORD wPage = ( wCell - COSTUME_INV_SLOT_START ) / COSTUME_INV_PAGE_SIZE;
			WORD wPageStart = COSTUME_INV_SLOT_START + ( wPage * COSTUME_INV_PAGE_SIZE );

			WORD wSlot = 0;

			// Check previous slots
			constexpr int iMaxItemHeight = 3;
			int iDiffCounter = 1;

			WORD wCheckGrid = wCell;

			while(wCheckGrid - wPageStart >= 5)
			{
				wCheckGrid -= 5;

				LPITEM pItem = m_pointsInstant.pItems[ wCheckGrid ];

				if(pItem && pItem->GetSize() > iDiffCounter)
					return false;

				if(iDiffCounter == iMaxItemHeight)
					break;

				++iDiffCounter;
			}

			if(bSize == 1)
				return true;

			// Check further slots
			do
			{
				// Calc next slot
				wSlot = wCell + ( 5 * j );

				// Not in limit
				if(wSlot >= COSTUME_INV_SLOT_END)
					return false;

				if(( wSlot - COSTUME_INV_SLOT_START ) / COSTUME_INV_PAGE_SIZE != wPage)
					return false;

				if(m_pointsInstant.wItemGrid[ wSlot ])
					return false;
			}
			while(++j < bSize);

			return true;
		}

		return true;
	}
	break;
#endif
	case SKILLBOOK_INVENTORY:
	case STONE_INVENTORY:
	case ENCHANT_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (m_pointsInstant.wItemGrid[wCell])
				return false;

			return true;
		}
		break;

	case INVENTORY:
		{
			WORD wCell = Cell.cell;

			// bItemCellÀº 0ÀÌ falseÀÓÀ» ³ªÅ¸³»±â À§ÇØ + 1 ÇØ¼­ Ã³¸®ÇÑ´Ù.
			// µû¶ó¼­ iExceptionCell¿¡ 1À» ´õÇØ ºñ±³ÇÑ´Ù.
			++iExceptionCell;

			if (wCell >= GetInventoryMaxNum())
				return false;

			if (m_pointsInstant.wItemGrid[wCell])
			{
				if (m_pointsInstant.wItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = wCell / INVENTORY_PAGE_SIZE;

					do
					{
						WORD p = wCell + (5 * j);

						if (p >= GetInventoryMaxNum())
							return false;

						if (p / INVENTORY_PAGE_SIZE != bPage)
							return false;

						if (m_pointsInstant.wItemGrid[p])
							if (m_pointsInstant.wItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// Å©±â°¡ 1ÀÌ¸é ÇÑÄ­À» Â÷ÁöÇÏ´Â °ÍÀÌ¹Ç·Î ±×³É ¸®ÅÏ
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				WORD wPage = wCell / INVENTORY_PAGE_SIZE;

				do
				{
					WORD p = wCell + (5 * j);

					if (p >= GetInventoryMaxNum())
						return false;

					if (p / INVENTORY_PAGE_SIZE != wPage)
						return false;

					if (m_pointsInstant.wItemGrid[p])
						if (m_pointsInstant.wItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			// bItemCellÀº 0ÀÌ falseÀÓÀ» ³ªÅ¸³»±â À§ÇØ + 1 ÇØ¼­ Ã³¸®ÇÑ´Ù.
			// µû¶ó¼­ iExceptionCell¿¡ 1À» ´õÇØ ºñ±³ÇÑ´Ù.
			iExceptionCell++;

			if (m_pointsInstant.wDSItemGrid[wCell])
			{
				if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						BYTE p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.wDSItemGrid[p])
							if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// Å©±â°¡ 1ÀÌ¸é ÇÑÄ­À» Â÷ÁöÇÏ´Â °ÍÀÌ¹Ç·Î ±×³É ¸®ÅÏ
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					BYTE p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.wItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
#endif
	}

	return false;
}

int CHARACTER::GetEmptySlotInWindow(BYTE bWindow, BYTE bSize) const
{
	WORD wSlotStart, wSlotEnd;
	ITEM_MANAGER::instance().GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, (const LPCHARACTER) this);

	for(int i = wSlotStart; i < wSlotEnd; ++i)
	{
		if(IsEmptyItemGrid(TItemPos(bWindow, i), bSize))
			return i;
	}

	return -1;
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	// NOTE: ÇöÀç ÀÌ ÇÔ¼ö´Â ¾ÆÀÌÅÛ Áö±Þ, È¹µæ µîÀÇ ÇàÀ§¸¦ ÇÒ ¶§ ÀÎº¥Åä¸®ÀÇ ºó Ä­À» Ã£±â À§ÇØ »ç¿ëµÇ°í ÀÖ´Âµ¥,
	//		º§Æ® ÀÎº¥Åä¸®´Â Æ¯¼ö ÀÎº¥Åä¸®ÀÌ¹Ç·Î °Ë»çÇÏÁö ¾Êµµ·Ï ÇÑ´Ù. (±âº» ÀÎº¥Åä¸®: INVENTORY_MAX_NUM ±îÁö¸¸ °Ë»ç)
	for (int i = 0; i < GetInventoryMaxNum(); ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}

bool CHARACTER::GetInventorySlotRange(BYTE bWindow, WORD& wSlotStart, WORD& wSlotEnd) const
{
	return ITEM_MANAGER::instance().GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, (const LPCHARACTER)this);
}

int CHARACTER::GetEmptyInventoryNew(BYTE window, BYTE size) const
{
	if (window == INVENTORY)
		return GetEmptyInventory(size);

	if(window == COSTUME_INVENTORY)
	{
		/* 
			int iFreeSlot = GetEmptySlotInWindow(COSTUME_INVENTORY, size);
			
			if(iFreeSlot != -1)
				return iFreeSlot;

			else return GetEmptyInventory(size);
		*/
		return GetEmptyInventory(size);
	}

	WORD wSlotStart, wSlotEnd;
	ITEM_MANAGER::instance().GetInventorySlotRange(window, wSlotStart, wSlotEnd, (const LPCHARACTER)this);

	for (int i = wSlotStart; i < wSlotEnd; ++i)
	{
		if (!GetInventoryItem(i))
			return i;
	}

	if (window == ENCHANT_INVENTORY)
		return GetEmptyNewInventory(ENCHANT_INVENTORY);

	if (window == SKILLBOOK_INVENTORY || window == STONE_INVENTORY)
		return GetEmptyInventory(size);

	return -1;
}

int CHARACTER::GetEmptyNewInventory(BYTE bWindow) const
{
	WORD wSlotStart, wSlotEnd;
	if (!GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd))
		return -1;

	for (int i = wSlotStart; i < wSlotEnd; ++i)
	{
		if (GetItem(TItemPos(bWindow, i)) == NULL)
			return i;
	}

	return -1;
}

#ifdef __DRAGONSOUL__
int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}
#endif

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;

	for (int i = 0; i < GetInventoryMaxNum(); ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (GetInventoryMaxNum() - count);
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
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

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}

	}

	// ¸ÅÁ÷ ¾ÆÀÌÅÛ ¼³Á¤
	pkOldItem->CopyAttributeTo(pkNewItem);
}

void LogRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), 1, way);
	}
}

void NotifyRefineSuccess(LPCHARACTER ch)
{
	if (NULL != ch)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");
	}
}

void LogRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), success, way);
	}
}

void NotifyRefineFail(LPCHARACTER ch)
{
	if (NULL != ch)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if ( ch != NULL )
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

LPCHARACTER CHARACTER::GetRefineNPC() const
{
	if (m_dwRefineNPCVID)
		return CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);
	return NULL;
}

void CHARACTER::SetRefinedByCount(DWORD dwVID, DWORD dwCount)
{
	itertype(m_map_refinedByCount) it = m_map_refinedByCount.find(dwVID);
	if (it == m_map_refinedByCount.end())
		m_map_refinedByCount.insert(std::pair<DWORD, DWORD>(dwVID, dwCount));
	else
		it->second = dwCount;
}

DWORD CHARACTER::GetRefinedByCount(DWORD dwVID) const
{
	itertype(m_map_refinedByCount) it = m_map_refinedByCount.find(dwVID);
	if (it == m_map_refinedByCount.end())
		return 0;
	return it->second;
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly, bool bFastRefine)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	if (!item)
		return false;

	if (!CanRefineBySmith(item, !bMoneyOnly ? REFINE_TYPE_NORMAL : REFINE_TYPE_MONEY_ONLY, NULL, GetRefineNPC(), true))
		return false;

	auto prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	DWORD result_vnum = item->GetRefinedVnum();

	// REFINE_COST
	int cost = ComputeRefineFee(prt->cost());

	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	if (!bMoneyOnly)
	{
		for (int i = 0; i < prt->material_count(); ++i)
		{
			if (CountSpecifyItem(prt->materials(i).vnum()) < prt->materials(i).count())
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d",
						prt->materials(i).vnum(), CountSpecifyItem(prt->materials(i).vnum()), prt->materials(i).count());
				}
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
				return false;
			}
		}

		for (int i = 0; i < prt->material_count(); ++i)
			RemoveSpecifyItem(prt->materials(i).vnum(), prt->materials(i).count());
	}

	// END_OF_REFINE_COST
	
	if (bMoneyOnly && GetRefineNPC())
		GetRefineNPC()->SetRefinedByCount(GetPlayerID(), GetRefineNPC()->GetRefinedByCount(GetPlayerID()) + 1);

	int rand = random_number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		rand -= 10;

#ifdef ENABLE_UPGRADE_STONE
	if (GetQuestFlag("upgrade_stone.use"))
	{
		rand -= 5;
		SetQuestFlag("upgrade_stone.use", 0);
		ChatPacket(CHAT_TYPE_COMMAND, "SetUpgradeBonus 0");
	}
#endif

	if (prt->prob() + (prt->prob() * GetPoint(POINT_BONUS_UPGRADE_CHANCE)) / 100 >= rand)
	{
		// ¼º°ø! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁö°í, °°Àº ¼Ó¼ºÀÇ ´Ù¸¥ ¾ÆÀÌÅÛ È¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			WORD wCell = item->GetCell();

			// DETAIL_REFINE_LOG
			LogRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			LogManager::instance().MoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, wCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			if (bFastRefine)
			{
				RefineInformation(wCell, !bMoneyOnly ? REFINE_TYPE_NORMAL : REFINE_TYPE_MONEY_ONLY);
			}
			NotifyRefineSuccess(this);

			sys_log(0, "Refine Success %d", cost);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -cost);
			sys_log(0, "PayPee %d", cost);
			PayRefineFee(cost);
			sys_log(0, "PayPee End %d", cost);
		}
		else
		{
			// DETAIL_REFINE_LOG
			// ¾ÆÀÌÅÛ »ý¼º¿¡ ½ÇÆÐ -> °³·® ½ÇÆÐ·Î °£ÁÖ
			sys_err("cannot create item %u", result_vnum);
			LogRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			NotifyRefineFail(this);
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		// ½ÇÆÐ! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁü.
		LogManager::instance().MoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
		LogRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		NotifyRefineFail(this);
		item->AttrLog();
		ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

bool CHARACTER::DoRefineWithScroll(LPITEM item, bool bFastRefine)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	if (m_iRefineAdditionalCell < 0)
		return false;

	LPITEM pkItemScroll;
	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetScrollRefineType() < 0)
		return false;

	if (!CanRefineBySmith(item, pkItemScroll->GetScrollRefineType(), pkItemScroll, GetRefineNPC(), true))
		return false;

#ifdef ELONIA
	if (pkItemScroll->GetVnum() == 54016 && item->GetLimitType(0) == LIMIT_LEVEL && item->GetLimitValue(0) > 40)
		return false;
#endif

	auto prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (GetGold() < prt->cost())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	for (int i = 0; i < prt->material_count(); ++i)
	{
		if (CountSpecifyItem(prt->materials(i).vnum(), item) < prt->materials(i).count())
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d",
					prt->materials(i).vnum(), CountSpecifyItem(prt->materials(i).vnum()), prt->materials(i).count());
			}
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
			return false;
		}
	}


	for (int i = 0; i < prt->material_count(); ++i)
		RemoveSpecifyItem(prt->materials(i).vnum(), prt->materials(i).count(), item);

	int prob = random_number(1, 100);
	int success_prob = prt->prob() + (prt->prob() * GetPoint(POINT_BONUS_UPGRADE_CHANCE)) / 100;
	bool bDowngradeWhenFail = true;

	const char* szRefineType = "SCROLL";

	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN ||
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL ||
		pkItemScroll->GetValue(0) == YAGONG_SCROLL ||
		pkItemScroll->GetValue(0) == YAGONG_SCROLL_KEEPLEVEL) // ÇöÃ¶, ¿ë½ÅÀÇ Ãàº¹¼­, ¾ß°øÀÇ ºñÀü¼­  Ã³¸®
	{
		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
#ifdef ELONIA
			success_prob += 5;
#else
			success_prob += 10;
#endif
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
#ifdef ELONIA
			success_prob += 10;
#else
			success_prob += 20;
#endif
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL_KEEPLEVEL)
		{
			success_prob += 10;
			bDowngradeWhenFail = false;
		}

		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) // ÇöÃ¶Àº ¾ÆÀÌÅÛÀÌ ºÎ¼­Á®¾ß ÇÑ´Ù.
			bDowngradeWhenFail = false;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL_KEEPLEVEL)
			szRefineType = "YAGONG_SCROLL_KEEPLEVEL";
		// END_OF_DETAIL_REFINE_LOG
	}

#ifdef ENABLE_UPGRADE_STONE
	if (GetQuestFlag("upgrade_stone.use"))
	{
		success_prob += 5;
		SetQuestFlag("upgrade_stone.use", 0);
		ChatPacket(CHAT_TYPE_COMMAND, "SetUpgradeBonus 0");
	}
#endif

	tchat("Success_Prob %d, RefineLevel %d szRefineType %s", success_prob, item->GetRefineLevel(), szRefineType);
	tchat("bDowngradeWhenFail %d pkItemScroll->GetValue(0) %d result_fail_vnum %d", bDowngradeWhenFail, pkItemScroll->GetValue(0), result_fail_vnum);

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL) // ¹«½ÅÀÇ Ãàº¹¼­´Â 100% ¼º°ø (+4±îÁö¸¸)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
	//	success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

	BYTE bRefineType = pkItemScroll->GetScrollRefineType();
	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);

	if (prob <= success_prob)
	{
		// ¼º°ø! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁö°í, °°Àº ¼Ó¼ºÀÇ ´Ù¸¥ ¾ÆÀÌÅÛ È¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			WORD wCell = item->GetCell();

			LogRefineSuccess(this, item, szRefineType);
			LogManager::instance().MoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost());
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, wCell));
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();

			if (bFastRefine)
			{
				if (test_server)
					sys_log(0, "DoRefineWithScroll - refineInfo (%u, %u, %d)", wCell, bRefineType, m_iRefineAdditionalCell);
				RefineInformation(wCell, bRefineType, m_iRefineAdditionalCell);
			}
			NotifyRefineSuccess(this);

			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost());
		}
		else
		{
			// ¾ÆÀÌÅÛ »ý¼º¿¡ ½ÇÆÐ -> °³·® ½ÇÆÐ·Î °£ÁÖ
			sys_err("cannot create item %u", result_vnum);
			LogRefineFail(this, item, szRefineType);
			NotifyRefineFail(this);
		}
	}
	else if (bDowngradeWhenFail && result_fail_vnum)
	{
		// ½ÇÆÐ! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁö°í, °°Àº ¼Ó¼ºÀÇ ³·Àº µî±ÞÀÇ ¾ÆÀÌÅÛ È¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE FAIL", pkNewItem->GetName());

			WORD wCell = item->GetCell();

			LogManager::instance().MoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost());
			LogRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, wCell)); 
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			pkNewItem->AttrLog();

			if (bFastRefine)
				RefineInformation(wCell, bRefineType, m_iRefineAdditionalCell);
			NotifyRefineFail(this);

			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost());
		}
		else
		{
			// ¾ÆÀÌÅÛ »ý¼º¿¡ ½ÇÆÐ -> °³·® ½ÇÆÐ·Î °£ÁÖ
			sys_err("cannot create item %u", result_fail_vnum);
			LogRefineFail(this, item, szRefineType);
			NotifyRefineFail(this);
		}
	}
	else
	{
		LogRefineFail(this, item, szRefineType); // °³·®½Ã ¾ÆÀÌÅÛ »ç¶óÁöÁö ¾ÊÀ½
		NotifyRefineFail(this);
		
		PayRefineFee(prt->cost());
	}

	return true;
}

bool CHARACTER::CanRefineBySmith(LPITEM pkItem, BYTE bType, LPITEM pkItemScroll, LPCHARACTER pkSmith, bool bMessage)
{
	if (!pkSmith)
		pkSmith = GetRefineNPC();

	// general checks
	if (!pkItem->GetRefinedVnum() || !ITEM_MANAGER::instance().GetTable(pkItem->GetRefinedVnum()))
	{
		if (bMessage)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		if (test_server)
			sys_err("cannot refine item (no refined vnum) %s %u", pkItem->GetName(), pkItem->GetVnum());
		return false;
	}

	if (!pkItem->GetRefineSet() || !CRefineManager::instance().GetRefineRecipe(pkItem->GetRefineSet()))
	{
		if (bMessage)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		sys_err("cannot get refine set for item %s %u (refine_set %u)", pkItem->GetName(), pkItem->GetVnum(), pkItem->GetRefineSet());
		return false;
	}

	if (pkSmith && pkItemScroll)
	{
		sys_err("refine: cannot refine: pkSmith != NULL and pkItemScroll != NULL");
		return false;
	}
	// general checks [end]

	// upgrade other items than weapons / armors (and stones for bluedragon scroll)
	if (pkItem->GetType() != ITEM_WEAPON && pkItem->GetType() != ITEM_ARMOR && pkItem->GetType() != ITEM_TOTEM
#ifdef __BELT_SYSTEM__
		&& pkItem->GetType() != ITEM_BELT
#endif
		)
	{
		if (pkItem->GetType() == ITEM_METIN)
		{
			if (pkItem->GetRefineLevel() >= 4)
			{
				if (!pkItemScroll || pkItemScroll->GetValue(0) != STONE_LV5_SCROLL)
				{
					if (bMessage)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can upgrade this metin stone only with %s."), ITEM_MANAGER::instance().GetItemLink(ITEM_STONE_SCROLL_VNUM));
					return false;
				}
			}
			else
			{
				if (!pkSmith || pkSmith->GetRaceNum() != BLACKSMITH_STONE_MOB)
				{
					if (bMessage)
					{
						const CMob* pkMob = CMobManager::instance().Get(BLACKSMITH_STONE_MOB);
						if (pkMob)
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can upgrade stones only at %s."), pkMob->m_table.locale_name(GetLanguageID()).c_str());
					}
					return false;
				}
			}
		}
		else
		{
			if (bMessage)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			sys_err("unkown refine item type %u vnum %u", pkItem->GetType(), pkItem->GetVnum());
			return false;
		}
	}
	// upgrade other items than weapons / armors [end]

	// upgrade item
	if (pkItem->IsRefinedOtherItem())
	{
		if (bType != REFINE_TYPE_NORMAL || pkItemScroll || !pkSmith || pkSmith->GetRaceNum() != BLACKSMITH2_MOB)
		{
			const char* szName = "[not_found_npc_20091]"; // err message if cannot find npc by race 20091
			const CMob* pkMob = CMobManager::instance().Get(BLACKSMITH2_MOB);
			if (pkMob)
				szName = pkMob->m_table.locale_name(GetLanguageID()).c_str();

			if (bMessage)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can refine this item only at %s."), szName);
			sys_err("cannot upgrade item %u %s (not at %s)", pkItem->GetVnum(), pkItem->GetName(), szName);
			return false;
		}
		else
		{
			return true;
		}
	}
	// upgrade item [end]

	// check scroll
	if (bType != REFINE_TYPE_NORMAL && bType != REFINE_TYPE_MONEY_ONLY && !pkItemScroll)
	{
		sys_err("ERROR: no scroll given when type is %u", bType);
		return false;
	}

	if (pkItemScroll)
	{
		if (pkItemScroll->GetScrollRefineType() != bType)
		{
			sys_err("ERROR: scroll %u %s scrollRefineType[%d] != refine_type[%u] !!",
				pkItemScroll->GetVnum(), pkItemScroll->GetName(), pkItemScroll->GetScrollRefineType(), bType);
			return false;
		}
		else if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
		{
			if (pkItem->GetRefineLevel() >= 4)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ °³·®¼­·Î ´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
		}
		else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
		{
			if (pkItem->GetRefineLevel() != pkItemScroll->GetValue(1))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ °³·®¼­·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
		}
		else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkItem->GetType() != ITEM_METIN || pkItem->GetRefineLevel() != 4)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
		}
		else if (pkItemScroll->GetValue(0) == STONE_LV5_SCROLL)
		{
			if (pkItem->GetType() != ITEM_METIN || pkItem->GetRefineLevel() != 4)
			{
				if (bMessage)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only upgrade stones with this scroll."));
				return false;
			}
		}
	}
	// check scroll [end]

	switch (bType)
	{
	case REFINE_TYPE_NORMAL:
	case REFINE_TYPE_SCROLL:
	case REFINE_TYPE_HYUNIRON:
	case REFINE_TYPE_MUSIN:
	case REFINE_TYPE_BDRAGON:
		return true;
		break;

	case REFINE_TYPE_MONEY_ONLY:
		{
			if (pkSmith && 
				(pkSmith->GetRaceNum() == DEVILTOWER_BLACKSMITH_WEAPON_MOB ||
				pkSmith->GetRaceNum() == DEVILTOWER_BLACKSMITH_ARMOR_MOB ||
				pkSmith->GetRaceNum() == DEVILTOWER_BLACKSMITH_ACCESSORY_MOB))
			{
				if (pkSmith->GetRefinedByCount(GetPlayerID()) > 0)
				{
					if (bMessage)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only refine once at this smith."));
					return false;
				}

				return true;
			}

			if (bMessage)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can refine with money-only only in the deviltower."));
			return false;
		}
		break;
	}

	return false;
}

bool CHARACTER::RefineInformation(WORD wCell, BYTE bType, int iAdditionalCell)
{
	if (wCell > GetInventoryMaxNum())
		return false;

	LPITEM item = GetInventoryItem(wCell);

	if (!item)
		return false;

	LPITEM itemScroll = NULL;
	if (iAdditionalCell != -1)
		itemScroll = GetInventoryItem(iAdditionalCell);

	if (!CanRefineBySmith(item, bType, itemScroll, GetRefineNPC(), true))
		return false;

	network::GCOutputPacket<network::GCRefineInformationPacket> p;

	p->mutable_pos()->set_cell(wCell);
	p->set_src_vnum(item->GetVnum());
	p->set_result_vnum(item->GetRefinedVnum());
	p->set_type(bType);
	p->set_can_fast_refine(!GetRefineNPC() || GetRefineNPC()->GetRaceNum() != BLACKSMITH2_MOB);

	CRefineManager & rm = CRefineManager::instance();
	auto prt = rm.GetRefineRecipe(item->GetRefineSet());

	// REFINE_COST
	p->set_cost(ComputeRefineFee(prt->cost()));
	
	//END_MAIN_QUEST_LV7
	p->set_prob(prt->prob() + (prt->prob() * GetPoint(POINT_BONUS_UPGRADE_CHANCE)) / 100);
	tchat("current default refine prob: %d", p->prob());
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p->mutable_refine_table()->set_material_count(0);
		p->mutable_refine_table()->clear_materials();
	}
	else
	{
		p->mutable_refine_table()->set_material_count(prt->material_count());
		*p->mutable_refine_table()->mutable_materials() = prt->materials();
	}
	// END_OF_REFINE_COST

	GetDesc()->Packet(p);

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// XXX ¼º´É, ¼ÒÄÏ °³·®¼­´Â »ç¶óÁ³½À´Ï´Ù...
		// XXX ¼º´É°³·®¼­´Â Ãàº¹ÀÇ ¼­°¡ µÇ¾ú´Ù!
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell());
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell());
		}
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT", pkTarget->GetName());

		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::instance().GetTable(pkTarget->GetSocket(i));
					//pkTarget->SetSocket(i, pTable->alValues[2]);
					// ±úÁøµ¹·Î ´ëÃ¼ÇØÁØ´Ù
					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»©³¾ ¼ö ÀÖ´Â ¸ÞÆ¾¼®ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "kill_campfire_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "±â¾ïÇØ µÑ ¼ö ¾ø´Â À§Ä¡ ÀÔ´Ï´Ù."));
		return false;
	}

	int pos;

	if (item->GetCount() == 1)	// ¾ÆÀÌÅÛÀÌ ÇÏ³ª¶ó¸é ±×³É ¼ÂÆÃ.
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1) // ±×·¸Áö ¾Ê´Ù¸é ´Ù¸¥ ÀÎº¥Åä¸® ½½·ÔÀ» Ã£´Â´Ù.
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	if (!CAN_ENTER_ZONE_CHECKLEVEL(this, idx, true))
	{
		return;
	}

	sys_log(1, "Recall: %s %d %d -> %d %d", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
	WarpSet(item->GetSocket(0), item->GetSocket(1));
	item->SetCount(item->GetCount() - 1);
}

bool CHARACTER::__CanMakePrivateShop()
{
	return true;
}

void CHARACTER::__OpenPrivateShop()
{
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
		case 0:
		case 1:
		case 2:
			ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
			break;
		default:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°©¿ÊÀ» ¹þ¾î¾ß °³ÀÎ »óÁ¡À» ¿­ ¼ö ÀÖ½À´Ï´Ù."));
			break;
	}
}

// MYSHOP_PRICE_LIST
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}

//
// DB Ä³½Ã·Î ºÎÅÍ ¹ÞÀº ¸®½ºÆ®¸¦ User ¿¡°Ô Àü¼ÛÇÏ°í »óÁ¡À» ¿­¶ó´Â Ä¿¸Çµå¸¦ º¸³½´Ù.
//
void CHARACTER::UseSilkBotaryReal(const google::protobuf::RepeatedPtrField<network::TItemPriceInfo>& p)
{
	if (!p.size())
		// °¡°Ý ¸®½ºÆ®°¡ ¾ø´Ù. dummy µ¥ÀÌÅÍ¸¦ ³ÖÀº Ä¿¸Çµå¸¦ º¸³»ÁØ´Ù.
		SendMyShopPriceListCmd(1, 0);
	else {
		for (int idx = 0; idx < p.size(); idx++)
			SendMyShopPriceListCmd(p[ idx ].vnum(), p[ idx ].price());
	}

	__OpenPrivateShop();
}

//
// ÀÌ¹ø Á¢¼Ó ÈÄ Ã³À½ »óÁ¡À» Open ÇÏ´Â °æ¿ì ¸®½ºÆ®¸¦ Load ÇÏ±â À§ÇØ DB Ä³½Ã¿¡ °¡°ÝÁ¤º¸ ¸®½ºÆ® ¿äÃ» ÆÐÅ¶À» º¸³½´Ù.
// ÀÌÈÄºÎÅÍ´Â ¹Ù·Î »óÁ¡À» ¿­¶ó´Â ÀÀ´äÀ» º¸³½´Ù.
//
void CHARACTER::UseSilkBotary(void)
{
	if (!__CanMakePrivateShop())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot open a shop on this map."));
		return;
	}

	if (m_bNoOpenedShop) {
		network::GDOutputPacket<network::GDMyShopPricelistRequestPacket> pdb;
		pdb->set_pid(GetPlayerID());
		db_clientdesc->DBPacket(pdb, GetDesc()->GetHandle());
		m_bNoOpenedShop = false;
	} else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST

int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT	= 30;
	static const int WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int curLife		= ch->GetHP();
		const int needPercent	= WARP_NEED_LIFE_PERCENT;
		const int needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³²Àº »ý¸í·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return -1;
		}

		consumeLife = needLife;


		// CheckMinLifeForWarp: µ¶¿¡ ÀÇÇØ¼­ Á×À¸¸é ¾ÈµÇ¹Ç·Î »ý¸í·Â ÃÖ¼Ò·®´Â ³²°ÜÁØ´Ù
		const int minPercent	= WARP_MIN_LIFE_PERCENT;
		const int minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(lpChar, "³²Àº Á¤½Å·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return -1;
	}

	return needSP;
}

bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	//this->tchat("CHARACTER::UseItemEx(%s, %d)", item->GetName(), DestCell.cell);

	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->limits(i).value();

		switch (item->GetProto()->limits(i).type())
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case LIMIT_TIMER_BASED_ON_WEAR:
				iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		sys_log(0, "USE_ITEM %s, Inven %d, Cell %d, ItemType %d, SubType %d", item->GetName(), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

	if ( CArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
		return false;
	}

	// ¾ÆÀÌÅÛ ÃÖÃÊ »ç¿ë ÀÌÈÄºÎÅÍ´Â »ç¿ëÇÏÁö ¾Ê¾Æµµ ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä Ã³¸®. 
	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		// ÇÑ ¹øÀÌ¶óµµ »ç¿ëÇÑ ¾ÆÀÌÅÛÀÎÁö ¿©ºÎ´Â Socket1À» º¸°í ÆÇ´ÜÇÑ´Ù. (Socket1¿¡ »ç¿ëÈ½¼ö ±â·Ï)
		if (0 == item->GetSocket(1))
		{
			// »ç¿ë°¡´É½Ã°£Àº Default °ªÀ¸·Î Limit Value °ªÀ» »ç¿ëÇÏµÇ, Socket0¿¡ °ªÀÌ ÀÖÀ¸¸é ±× °ªÀ» »ç¿ëÇÏµµ·Ï ÇÑ´Ù. (´ÜÀ§´Â ÃÊ)
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->limits(iLimitRealtimeStartFirstUseFlagIndex).value() + time(0);

			if (0 == duration)
				duration = 60 * 60 * 24 * 7 + time(0);

			item->SetSocket(0, duration);
			item->StartRealTimeExpireEvent();
		}	

		if (false == item->IsEquipped())
		{
			bool bChangeSocket = true;
#ifdef __FAKE_BUFF__
			if (CItemVnumHelper::IsFakeBuffSpawn(item->GetVnum()))
				bChangeSocket = false;
#endif

			if (bChangeSocket)
				item->SetSocket(1, item->GetSocket(1) + 1);
		}
	}

#ifdef EL_COSTUME_ATTR
	switch (item->GetVnum())
	{
		// Costume Bonus Adder
		case 54000:
		case 54002:
		case 54004:
		case 54006:
		case 54008:
		case 54010:
		case 54012:
		case 54014:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
					return false;

				if (item2->IsExchanging() == true)
					return false;

				// if (item2->GetAttributeSetIndex() == -1)
				// {
				// 	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
				// 	return false;
				// }

				BYTE bEmptyIndex = 0;
				for (; bEmptyIndex < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++bEmptyIndex)
				{
					if (item2->GetAttributeType(bEmptyIndex) == APPLY_NONE || item2->GetAttributeValue(bEmptyIndex) == 0)
						break;
				}

				if (bEmptyIndex == ITEM_MANAGER::MAX_NORM_ATTR_NUM)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
					tchat("max attr num");
					return false;
				}

				if (item2->GetType() != ITEM_COSTUME || (	
						(!( bEmptyIndex <= 2 &&
							(
								(item->GetVnum() == 54000 && item2->GetSubType() == COSTUME_WEAPON) ||
								(item->GetVnum() == 54002 && item2->GetSubType() == COSTUME_BODY) ||
								(item->GetVnum() == 54004 && item2->GetSubType() == COSTUME_HAIR)
							)
						) && item->GetVnum() != 54012) // allrounder
						&&
						(!( (bEmptyIndex >= 3 && bEmptyIndex <= 4) &&
							(
								(item->GetVnum() == 54006 && item2->GetSubType() == COSTUME_WEAPON) ||
								(item->GetVnum() == 54008 && item2->GetSubType() == COSTUME_BODY) ||
								(item->GetVnum() == 54010 && item2->GetSubType() == COSTUME_HAIR)
							)
						) && item->GetVnum() != 54014) // allrounder
					))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
					return false;
				}


				if (item2->GetLimitType(0) == LIMIT_REAL_TIME_START_FIRST_USE || item2->GetLimitType(0) == LIMIT_REAL_TIME || item2->GetLimitType(0) == LIMIT_TIMER_BASED_ON_WEAR)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't add a bonus if the costume has an active expire."));
					return false;
				}
				// else
				tchat("^---%d , %d", item2->GetLimitType(0), item2->GetSocket(0));

				// if (random_number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
				// {

				int iAttributeSet = item2->GetAttributeSetIndex();

				// if (iAttributeSet < 0)
				// 	return false;

				WORD total = 0;
				std::vector<network::TItemAttrTable> avail;
				for (const auto& it : g_map_itemCostumeAttr)
				{
					auto& v = it.second;

					bool bHasAttr = false;
					for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
					{
						if (item2->GetAttributeType(i) == v.apply_index())
						{
							bHasAttr = true;
							break;
						}
					}

					if (v.max_level_by_set(iAttributeSet) && !bHasAttr && 
						((bEmptyIndex <= 2 && v.values(3)) || (bEmptyIndex >= 3 && v.values(4))))
					{
						tchat("{%d:%d} (%d,%d,%d, %d,%d)", v.apply_index(), v.values(2), v.max_level_by_set(iAttributeSet),
							item2->HasAttr(v.apply_index()),bEmptyIndex , v.values(3), v.values(4));
						avail.push_back(v);
						total += v.prob();
					}
				}

				if (avail.size() < 3)
				{
					ChatPacket(CHAT_TYPE_INFO, "ERROR111, iAttributeSet %i", iAttributeSet);
					return false;
				}

				int i = bEmptyIndex;

				DWORD prob = random_number(1, total);
				int attr_idx = -1;

				for (DWORD i = 0; i < avail.size(); ++i)
				{
					if (prob <= avail[i].prob())
					{
						attr_idx = i;
						break;
					}

					prob -= avail[i].prob();
				}

				BYTE switcherType = 1;
				if (item->GetVnum() <= 54005 || item->GetVnum() == 54013)
					switcherType = 0;

				if ((item->GetVnum() == 54012 && bEmptyIndex > 2) || (item->GetVnum() == 54014 && bEmptyIndex < 3))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
					tchat("Allround switcher but wrong index");
					return false;
				}

				BYTE bLevel = random_number(0, avail[attr_idx].max_level_by_set(iAttributeSet) - 1);
				tchat("attr_idx:%i / %d / %d || iAttributeSet[%d] bLevel[%d] emptyidx[%d]", attr_idx, avail.size(), g_map_itemCostumeAttr.size(), iAttributeSet, bLevel, bEmptyIndex);

				if (attr_idx < 0)
				{
					tchat("Cannot put item attribute %d %d", iAttributeSet, bLevel);
					sys_err("Cannot put item attribute %d %d", iAttributeSet, bLevel);
					return false;;
				}

				if (item2->GetAttributeType(i) == APPLY_NONE)
				{
					item2->SetAttribute(i, avail[attr_idx].apply_index(), avail[attr_idx].values(bLevel));
					total -= avail[attr_idx].prob();
					avail.erase(avail.begin() + attr_idx);
				}
	
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

				LogManager::instance().ItemLog(
						GetPlayerID(), 
						item2->GetID(),
						"ADD_COSTUME_ATTRIBUTE",
						"",
						GetDesc()->GetHostName(),
						item->GetOriginalVnum());

				if (item->IsGMOwner())
					item2->SetGMOwner(true);

				SetQuestItemPtr(item);
				quest::CQuestManager::instance().OnItemUsed(GetPlayerID());


				item->SetCount(item->GetCount() - 1);
				return true;

			}
			break;

		// Costume Bonus Changer
		case 54001:
		case 54003:
		case 54005:
		case 54007:
		case 54009:
		case 54011:
		case 54013:
		case 54015:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging() == true)
					return false;


				int iAttributeSet = item2->GetAttributeSetIndex();

				if (iAttributeSet < 0)
					return false;

				BYTE bEmptyIndex = 0;
				for (; bEmptyIndex < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++bEmptyIndex)
				{
					if (item2->GetAttributeType(bEmptyIndex) == APPLY_NONE || item2->GetAttributeValue(bEmptyIndex) == 0)
						break;
				}
				
				if ( bEmptyIndex == 0 || item2->GetAttributeSetIndex() == -1)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
					return false;
				}

				if (item2->GetType() != ITEM_COSTUME ||
						!(
							(item->GetVnum() == 54001 && item2->GetSubType() == COSTUME_WEAPON) ||
							(item->GetVnum() == 54007 && item2->GetSubType() == COSTUME_WEAPON) ||
							(item->GetVnum() == 54003 && item2->GetSubType() == COSTUME_BODY) ||
							(item->GetVnum() == 54009 && item2->GetSubType() == COSTUME_BODY) ||
							(item->GetVnum() == 54005 && item2->GetSubType() == COSTUME_HAIR) ||
							(item->GetVnum() == 54011 && item2->GetSubType() == COSTUME_HAIR) ||
							(item->GetVnum() == 54013 || item->GetVnum() == 54015) // allrounder
						)
					)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
					return false;
				}

				BYTE switcherType = 1;
				if (item->GetVnum() <= 54005 || item->GetVnum() == 54013)
					switcherType = 0;

				WORD total = 0;
				std::vector<network::TItemAttrTable> avail;
				for (const auto& it : g_map_itemCostumeAttr)
				{
					auto& v = it.second;
					if (v.max_level_by_set(iAttributeSet) && !item2->HasAttr(v.apply_index()) && 
						((!switcherType && v.values(3)) || (switcherType && v.values(4))))
					{
						avail.push_back(v);
						total += v.prob();
					}
				}

				BYTE start = (!switcherType ? 0 : 3);
				BYTE end = (!switcherType ? 3 : 5);
				for (int i = start; i < end; i++)
				{
					tchat("I:%i // Total: %d", i, total);
					DWORD prob = random_number(1, total);
					int attr_idx = -1;

					for (DWORD i = 0; i < avail.size(); ++i)
					{
						if (prob <= avail[i].prob())
						{
							attr_idx = i;
							break;
						}

						prob -= avail[i].prob();
					}

					BYTE bLevel = random_number(0, avail[attr_idx].max_level_by_set(iAttributeSet) - 1);
					tchat("attr_idx:%i / %d / %d || iAttributeSet[%d] bLevel[%d] emptyidx[%d]", attr_idx, avail.size(), g_map_itemCostumeAttr.size(), iAttributeSet, bLevel, bEmptyIndex);

					if (attr_idx < 0)
					{
						tchat("Cannot put item attribute %d %d", iAttributeSet, bLevel);
						sys_err("Cannot put item attribute %d %d", iAttributeSet, bLevel);
						return false;;
					}

					if (item2->GetAttributeType(i) != APPLY_NONE)
					{
						item2->SetAttribute(i, avail[attr_idx].apply_index(), avail[attr_idx].values(bLevel));
						total -= avail[attr_idx].prob();
						avail.erase(avail.begin() + attr_idx);
					}
				}

				if (item->IsGMOwner())
					item2->SetGMOwner(true);
				
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
				LogManager::instance().ItemLog(
						GetPlayerID(), 
						item2->GetID(),
						"CHANGE_COSTUME_ATTRIBUTE",
						"",
						GetDesc()->GetHostName(),
						item->GetOriginalVnum());

				SetQuestItemPtr(item);
				quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

				item->SetCount(item->GetCount() - 1);
				return true;
			}
			break;
	}
#endif

	switch (item->GetType())
	{
#ifdef CRYSTAL_SYSTEM
		case ITEM_CRYSTAL:
		{
			auto crystal_sub_type = static_cast<ECrystalItem>(item->GetSubType());
			switch (crystal_sub_type)
			{
				case ECrystalItem::CRYSTAL:
					if (item->is_crystal_using())
						item->stop_crystal_use();
					else
					{
						if (get_active_crystal_id())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You are already using a crystal."));
							return false;
						}

						if (item->GetSocket(CItem::CRYSTAL_TIME_SOCKET) <= 0)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This crystal has no energy left. You have to restore it with a time elixir."));
							return false;
						}

						item->start_crystal_use();
					}
					break;

				case ECrystalItem::TIME_ELIXIR:
				case ECrystalItem::UPGRADE_SCROLL:
				{
					LPITEM item2 = GetItem(DestCell);
					if (!item2)
						return false;

					if (item2->IsExchanging())
						return false;

					if (item2->GetType() != ITEM_CRYSTAL || item2->GetSubType() != static_cast<uint32_t>(ECrystalItem::CRYSTAL))
						return false;

					if (crystal_sub_type == ECrystalItem::TIME_ELIXIR)
					{
						auto duration = item->GetValue(0);

						item2->restore_crystal_energy(duration);
						item->SetCount(item->GetCount() - 1);
					}
					else if (crystal_sub_type == ECrystalItem::UPGRADE_SCROLL)
					{
						auto clarity_type = item2->GetSocket(CItem::CRYSTAL_CLARITY_TYPE_SOCKET);
						auto clarity_level = item2->GetSocket(CItem::CRYSTAL_CLARITY_LEVEL_SOCKET);

						auto next_proto = CGeneralManager::instance().get_crystal_proto(clarity_type, clarity_level, true);

						if (!next_proto)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This crystal already reached the maximum level."));
							return false;
						}

						network::GCOutputPacket<network::GCCrystalRefinePacket> pack;
						*pack->mutable_crystal_cell() = DestCell;
						*pack->mutable_scroll_cell() = ::TItemPos(item->GetWindow(), item->GetCell());
						pack->set_next_clarity_type(next_proto->clarity_type());
						pack->set_next_clarity_level(next_proto->clarity_level());
						pack->set_required_fragments(next_proto->required_fragments());

						for (auto& attr : next_proto->applies())
						{
							if (attr.type() != APPLY_NONE && attr.value() != 0)
								*pack->add_next_attributes() = attr;
						}

						GetDesc()->Packet(pack);
					}
				}
				break;
			}
		}
		break;
#endif

#ifdef __PET_ADVANCED__
		case ITEM_PET_ADVANCED:
			switch (static_cast<EPetAdvancedItem>(item->GetSubType()))
			{
				case EPetAdvancedItem::SUMMON:
					{
						auto pet = item->GetAdvancedPet();
						if (pet->IsSummoned())
						{
							pet->Unsummon();
						}
						else
						{
							if (!item->GetAdvancedPet()->CanSummon())
								return false;

							if (!item->GetAdvancedPet()->Summon())
								return false;
						}

						CheckMaximumPoints();

						return true;
					}
					break;

				case EPetAdvancedItem::SKILL_BOOK:
				case EPetAdvancedItem::HEROIC_SKILL_BOOK:
				case EPetAdvancedItem::SKILLPOWER_REROLL:
					{
						CPetAdvanced* pet = GetPetAdvanced();

						LPITEM item2;
						if (item2 = GetItem(DestCell))
						{
							if (item2->IsExchanging())
								return false;

							if (item2->GetType() != ITEM_PET_ADVANCED || item2->GetSubType() != static_cast<uint32_t>(EPetAdvancedItem::SUMMON))
								return false;

							if (!item2->GetAdvancedPet())
								return false;

							pet = item2->GetAdvancedPet();
						}

						if (!pet)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to summon your pet or drag this item on your pet item."));
							return false;
						}

						switch (static_cast<EPetAdvancedItem>(item->GetSubType()))
						{
							case EPetAdvancedItem::SKILLPOWER_REROLL:
								{
									if (!pet->GetEvolveData() || !pet->GetEvolveData()->CanSkillpowerReroll())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your pet needs to ber heroic to reroll skillpower."));
										return false;
									}

									auto old_power = pet->GetSkillpower();
									pet->RerollSkillpower();

									char szHint[256];
									snprintf(szHint, sizeof(szHint), "REROLL from %d to %d", old_power, pet->GetSkillpower());
									LogManager::Instance().ItemLog(this, item, "PET_SKILLPOWER", szHint);

									item->SetCount(item->GetCount() - 1);

									return true;
								}

							case EPetAdvancedItem::HEROIC_SKILL_BOOK:
								if (!pet->GetEvolveData() || pet->GetEvolveData()->GetName() != PET_HEROIC_GRADE_NAME)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your pet needs to be heroic to learn this skill."));
									return false;
								}

							case EPetAdvancedItem::SKILL_BOOK:
								{
									DWORD skill_vnum = item->GetSocket(0);
									if (skill_vnum == 0)
										skill_vnum = item->GetValue(0);

									if (!skill_vnum)
									{
										ChatPacket(CHAT_TYPE_INFO, "Failed to use skill book.");
										return false;
									}

									auto idx = pet->GetSkillIndexByVnum(skill_vnum);
									if (idx == -1)
									{
										if (!pet->LearnSkill(skill_vnum))
										{
											ChatPacket(CHAT_TYPE_INFO, "Your pet cannot learn that skill now.");
											return false;
										}

										idx = pet->GetSkillIndexByVnum(skill_vnum);

										char szHint[256];
										snprintf(szHint, sizeof(szHint), "SkillLearn %d", skill_vnum);
										LogManager::Instance().ItemLog(this, item, "PET_SKILL", szHint);

										ChatPacket(CHAT_TYPE_INFO, "Your pet has learned a new skill!");

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										if (!pet->CanSkillLevelUp(idx))
										{
											ChatPacket(CHAT_TYPE_INFO, "Your pet cannot advance that skill anymore.");
											return false;
										}

										char szHint[256];
										snprintf(szHint, sizeof(szHint), "SkillLevelUp %d %d",
											skill_vnum, pet->GetSkillInfo(idx)->level());

										pet->SkillLevelUp(idx);
										ChatPacket(CHAT_TYPE_INFO, "The skill was successfully improved!");

										LogManager::Instance().ItemLog(this, item, "PET_SKILL", szHint);

										item->SetCount(item->GetCount() - 1);
									}

									return true;
								}
								break;
						}
					}
					break;
			}
			break;
#endif
		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM_QUEST:
		{
			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
					return false;
				}
			}

			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
		}
		break;
			
#ifdef __PET_SYSTEM__
		case ITEM_PET:	
			if (item->GetValue(0))
			{
				CPetSystem* petSystem;
				if (!(petSystem = GetPetSystem()))
				{
					tchat("ITEM_PET no pet system found");
					sys_err("not pet system found");
					return false;
				}
				
				if (petSystem->GetSummoned())
				{
					CPetActor* pet = petSystem->GetSummoned();
					DWORD itemVID = pet->GetSummonItemVID();
					pet->Unsummon();
					
					if (ITEM_MANAGER::instance().FindByVID(itemVID) == item)
					{
						SetQuestFlag("pet_system.id", 0);
						return false;
					}
				}

				SetQuestFlag("pet_system.id", item->GetID());
				petSystem->Summon(item->GetValue(0), item, 0, false);
			}
			break;
#endif

		case ITEM_CAMPFIRE:
			{
				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy));

				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø´Â ÁöÁ¡ÀÔ´Ï´Ù."));
					return false;
				}

				if (tree->IsAttr((long)(GetX()+fx), (long)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹° ¼Ó¿¡ ¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy), 0, false, random_number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();

				info->ch = campfire;

				campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);
			}
			break;

		case ITEM_UNIQUE:
			{
				switch (item->GetSubType())
				{
					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_STR:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_RESIST_MAGIC:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS, 
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						SetQuestItemPtr(item);
						quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

						item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
							if (item->GetSubType() == USE_SPECIAL)
							{
								sys_log(0, "ITEM_UNIQUE: USE_SPECIAL %u", item->GetVnum());

								switch (item->GetVnum())
								{
									case 71049: // ºñ´Üº¸µû¸®
										UseSilkBotary();
										break;

									case 71221:
									{
										ChatPacket(CHAT_TYPE_INFO, "Premium Activated!");
										SetQuestFlag("auction_premium.premium_active", 1);
										item->SetCount(item->GetCount() - 1);
									}
									break;
								}

							}
							else
							{
								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

								if (!item->IsEquipped())
								{
									if (item->GetCount() > 1)
									{
										LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum());
										if (!item2)
											return false;

										ITEM_MANAGER::instance().CopyAllAttrTo(item, item2);
										
										// Copy position too if item is stackable
										if(item->GetType() == ITEM_UNIQUE && item->IsStackable())
										{
											if(GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE2))
											{
												WORD oldCell = item->GetCell();
												item2->SetCell(this, oldCell);
											}
										}
										
										if (EquipItem(item2))
											item->SetCount(item->GetCount() - 1);
										else
											ITEM_MANAGER::instance().DestroyItem(item2);
									}
									else
										EquipItem(item);
								}
								else
									UnequipItem(item);
							}
						}
						break;
				}
			}
			break;

		case ITEM_COSTUME:
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:
		case ITEM_PICK:
#ifdef __BELT_SYSTEM__
		case ITEM_BELT:
#endif
		case ITEM_TOTEM:
		case ITEM_SHINING:
			if (item->GetType() == ITEM_WEAPON && (item->GetSubType() == WEAPON_ARROW || item->GetSubType() == WEAPON_QUIVER))
			{
				LPITEM item2;

				if ((DestCell.IsDefaultInventoryPosition() || DestCell.IsEquipPosition()) && (item2 = GetItem(DestCell)))
				{
					if (item2->IsExchanging())
						return false;

					if (item2->GetType() != ITEM_WEAPON || item2->GetSubType() != WEAPON_QUIVER)
						return false;

					if (item->GetSubType() == WEAPON_ARROW)
					{
						// check if it's the right arrow
						if (item2->GetSocket(0) != 0 && item2->GetSocket(0) != item->GetVnum())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only put one arrow type in one time in a quiver."));
							return false;
						}

						// check if already max count reached
						if (item2->GetSocket(1) >= item2->GetValue(0))
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only put %d arrows in this quiver."), item2->GetValue(0));
							return false;
						}

						int iCount = MIN(item2->GetValue(0) - item2->GetSocket(1), item->GetCount());

						// add arrows
						if (item2->GetSocket(0) == 0)
							item2->SetSocket(0, item->GetVnum());
						item2->SetSocket(1, item2->GetSocket(1) + iCount);

						// set count
						item->SetCount(item->GetCount() - iCount);

						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The quiver received %d arrows."), iCount);
						return true;
					}
					else if (item->GetSubType() == WEAPON_QUIVER)
					{
						if (item != item2)
							return false;

						if (item->GetSocket(1) == 0)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have no arrows in your quiver."));
							return false;
						}

						int iCount = item->GetSocket(1);
						while (iCount > 0)
						{
#ifdef INCREASE_ITEM_STACK
							int iCurrentCount = MIN(iCount, ITEM_MAX_COUNT);
#else
							int iCurrentCount = MIN(iCount, 200);
#endif

							if (!AutoGiveItem(item->GetSocket(0), iCurrentCount))
							{
								ChatPacket(CHAT_TYPE_INFO, "Error.");
								item->SetSocket(1, iCount);
								return false;
							}

							iCount -= iCurrentCount;
						}

						item->SetSocket(0, 0);
						item->SetSocket(1, 0);

						return true;
					}
				}
			}

			SetQuestItemPtr(item);
			quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

			// END_OF_MINING
			if(!item->IsEquipped())
			{
				EquipItem(item);
			}
			else
				UnequipItem(item);
			break;
		
		case ITEM_FISH:
			{
				if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
					return false;
				}

				if (item->GetSubType() == FISH_ALIVE)
				{
					SetQuestItemPtr(item);
					quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

					fishing::UseFish(this, item);
				}
			}
			break;

		case ITEM_TREASURE_BOX:
			{
				return false;
				//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­¼è·Î Àá°Ü ÀÖ¾î¼­ ¿­¸®Áö ¾Ê´Â°Í °°´Ù. ¿­¼è¸¦ ±¸ÇØº¸ÀÚ."));
			}
			break;

		case ITEM_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging())
					return false;

				if (item2->GetType() != ITEM_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­¼è·Î ¿©´Â ¹°°ÇÀÌ ¾Æ´Ñ°Í °°´Ù."));
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­¼è´Â ¸ÂÀ¸³ª ¾ÆÀÌÅÛ ÁÖ´Â ºÎºÐ ±¸ÇöÀÌ ¾ÈµÇ¾ú½À´Ï´Ù."));
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets;
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count, item->IsGMOwner() || item2->IsGMOwner()))
					{
						SetQuestItemPtr(item);
						quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
						SetQuestItemPtr(item2);
						quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

						item->SetCount(item->GetCount()-1);
						item2->SetCount(item2->GetCount()-1);
						// ITEM_MANAGER::instance().RemoveItem(item);
						// ITEM_MANAGER::instance().RemoveItem(item2);
						
						for (int i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::EXP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
									break;
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::POLY_MARBLE:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "There was a poly marble in the chest!"));
									break;
								default:
									if (item_gets[i])
									{
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(GetLanguageID()), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(GetLanguageID()));

									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
					return false;
				}
			}
			break;

		case ITEM_GIFTBOX:
			{
				DWORD dwBoxVnum = item->GetVnum();
				if (IsPrivateMap())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot do this in a dungeon."));
					return false;
				}

				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				std::vector <LPITEM> item_gets;
				int count = 0;
				
#ifdef __DRAGONSOUL__
				if ((dwBoxVnum > 51500 && dwBoxVnum < 52000) || (dwBoxVnum >= 50255 && dwBoxVnum <= 50260))	// ¿ëÈ¥¿ø¼®µé
				{
					if (!(this->DragonSoul_IsQualified()))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸ÕÀú ¿ëÈ¥¼® Äù½ºÆ®¸¦ ¿Ï·áÇÏ¼Å¾ß ÇÕ´Ï´Ù."));
						return false;
					}
				}
#endif
				if (item && item->GetCount() && dwBoxVnum >= 92867 && dwBoxVnum <= 92869 || dwBoxVnum == 93006)
				{
					if (item->GetItemCooltime() > 0)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetLanguageID(), "You have to wait until you can use it again."));
						return false;
					}
				}
				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count, item->IsGMOwner()))
				{
					if (item && item->GetCount() && dwBoxVnum >= 92867 && dwBoxVnum <= 92869 || dwBoxVnum == 93006)
					{
						item->SetItemCooltime(60*3);
					}
				
					SetQuestItemPtr(item);
					quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

					item->SetCount(item->GetCount()-1);

					for (int i = 0; i < count; i++){
						switch (dwVnums[i])
						{
						case CSpecialItemGroup::GOLD:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
							break;
						case CSpecialItemGroup::EXP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
							break;
						case CSpecialItemGroup::MOB:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::SLOW:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::DRAIN_HP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
							break;
						case CSpecialItemGroup::POISON:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
							break;
						case CSpecialItemGroup::MOB_GROUP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::POLY_MARBLE:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "There was a poly marble in the chest!"));
							break;
						default:
							if (item_gets[i])
							{
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(GetLanguageID()), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(GetLanguageID()));
							}
						}
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
					return false;
				}
			}
			break;

		case ITEM_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return false;
				}

				DWORD dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					SetQuestItemPtr(item);
					quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

					ITEM_MANAGER::instance().RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "½ºÅ³ ·¹º§À» ³»¸®´Âµ¥ ¼º°øÇÏ¿´½À´Ï´Ù."));
				}
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "½ºÅ³ ·¹º§À» ³»¸± ¼ö ¾ø½À´Ï´Ù."));
			}
			break;

		case ITEM_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				DWORD dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{
					// »õ·Î¿î ¼ö·Ã¼­´Â value 0 ¿¡ ½ºÅ³ ¹øÈ£°¡ ÀÖÀ¸¹Ç·Î ±×°ÍÀ» »ç¿ë.
					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::instance().RemoveItem(item);

					return false;
				}

				if (true == LearnSkillByBook(dwVnum, 0, item->GetSubType()))
				{
					SetQuestItemPtr(item);
					quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

					item->SetCount(item->GetCount() - 1);

					int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM_USE:
			{
				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						sys_log (0, "ADD addtional effect : vnum(%d) subtype(%d)", item->GetOriginalVnum(), item->GetSubType());

					int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int apply_value = item->GetValue(2);
					int apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, item->GetValue(5) == 1 ? item->GetVnum() : AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, item->GetValue(5) == 1 ? item->GetVnum() : AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_STR:
									case APPLY_DEX:
									case APPLY_CON:
									case APPLY_INT:
									case APPLY_CAST_SPEED:
									case APPLY_RESIST_MAGIC:
									case APPLY_ATT_GRADE_BONUS:
									case APPLY_DEF_GRADE_BONUS:
									case APPLY_MAGIC_ATT_GRADE:
										AddAffect(affect_type, apply_type, apply_value, item->GetValue(5) == 1 ? item->GetVnum() : 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							SetQuestItemPtr(item);
							quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

							item->SetCount(item->GetCount() - 1);
							break;

					case USE_AFFECT :
						{
							if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							}
							else
							{
								AddAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);

								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
										break;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0) // HP Àý´ë°ª È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, (int)(item->GetValue(0) * (100.0f + GetPoint(POINT_HEAL_EFFECT_BONUS)) / 100.0f + 0.5f));
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP Àý´ë°ª È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									//PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									PointChange(POINT_SP, item->GetValue(1));
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿ùº´ ¶Ç´Â Á¾ÀÚ ¸¦ »ç¿ëÇÏ¿´½À´Ï´Ù"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}


				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}
				}

				if (test_server)
				{
					 sys_log (0, "USE_ITEM %s Type %d SubType %d vnum %d", item->GetName(), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
#ifdef __DRAGONSOUL__
					case USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// ¿ì¼± ¿ëÈ¥¼®¿¡ °üÇØ¼­¸¸ ÇÏµµ·Ï ÇÑ´Ù.
							if (pDestItem->IsDragonSoul())
							{
								int ret;
								char buf[128];
								if (item->GetVnum() == DRAGON_HEART_VNUM)
								{
#ifdef DS_TIME_ELIXIR_FIX
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX), item->GetSocket(ITEM_SOCKET_REMAIN_SEC));
#else
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
#endif
								}
								else
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								}
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%d}", ret, item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%d}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%dÃÊ ¸¸Å­ ÃæÀüµÇ¾ú½À´Ï´Ù."), ret);
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%d}", item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%d}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÃæÀüÇÒ ¼ö ¾ø½À´Ï´Ù."));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// ¿ì¼± ¿ëÈ¥¼®¿¡ °üÇØ¼­¸¸ ÇÏµµ·Ï ÇÑ´Ù.
							if (pDestItem->IsDragonSoul())
							{
								int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%dÃÊ ¸¸Å­ ÃæÀüµÇ¾ú½À´Ï´Ù."), ret);
									sprintf(buf, "Increase %ds by item{VN:%d VAL%d:%d}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÃæÀüÇÒ ¼ö ¾ø½À´Ï´Ù."));
									sprintf(buf, "No change by item{VN:%d VAL%d:%d}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
#endif
					case USE_SPECIAL:
						
						switch (item->GetVnum())
						{
							case GUILD_RENAME_ITEM_VNUM:
								{
									GetDesc()->Packet(network::TGCHeader::GUILD_REQUEST_MAKE);

									SetUsedRenameGuildItem(item->GetCell());
								}
								break;
							
							case 92895:
								{
									if (FindAffect(AFFECT_RAMADAN1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN1, POINT_MAX_HP, moveSpeedPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN1, POINT_ATTBONUS_MONSTER, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN1, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
									break;
								}
								
							case 92896:
								{
									if (FindAffect(AFFECT_RAMADAN2))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN2, POINT_ATTBONUS_METIN, moveSpeedPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN2, POINT_MAX_HP, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN2, POINT_MALL_ITEMBONUS, expPer, AFF_NONE, time, 0, true, true);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
									break;
								}
								
								
							
							case 92865:
							{
								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
								item->SetCount(item->GetCount() - 1);
								char szQuery[1024];
								snprintf(szQuery, sizeof(szQuery), "UPDATE account SET recover_points=recover_points+1 WHERE id=%d", GetAID());
								DBManager::instance().DirectQuery(szQuery);
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "A recovery point has been added to your account. (Usage on the webiste)"));
								break;
							}

							case 92879:
							{
								LPITEM item2 = GetItem(DestCell);
								if (!item2 || item2->IsExchanging() || item2->IsEquipped() || item2->isLocked())
									return false;
								if (item2->GetType() != ITEM_COSTUME || item2->GetSubType() != COSTUME_ACCE)
									return false;

								for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
									item2->SetForceAttribute(i, APPLY_NONE, 0);
								item2->SetSocket(1, 0);

								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

								item->SetCount(item->GetCount() - 1);

								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The absorbed item has been removed from the sash."));
							}
							break;

#ifdef AELDRA
							case ITEM_NOG_POCKET:
								{
									/*
									¶õÁÖ´É·ÂÄ¡ : item_proto value ÀÇ¹Ì
										ÀÌµ¿¼Óµµ  value 1
										°ø°Ý·Â	  value 2
										°æÇèÄ¡	value 3
										Áö¼Ó½Ã°£  value 0 (´ÜÀ§ ÃÊ)

									*/
									if (FindAffect(AFFECT_EVENT_START+15))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_EVENT_START+15, POINT_ATTBONUS_MONSTER, moveSpeedPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_EVENT_START+15, POINT_ATTBONUS_BOSS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_EVENT_START+15, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case 77100: // Plätzchen
								{
									/*
									¶õÁÖ´É·ÂÄ¡ : item_proto value ÀÇ¹Ì
										ÀÌµ¿¼Óµµ  value 1
										°ø°Ý·Â	  value 2
										°æÇèÄ¡	value 3
										Áö¼Ó½Ã°£  value 0 (´ÜÀ§ ÃÊ)

									*/
									if (FindAffect(AFFECT_EVENT_START+16))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_EVENT_START+16, POINT_ATTBONUS_MONSTER, moveSpeedPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_EVENT_START+16, POINT_ATTBONUS_METIN, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_EVENT_START+16, POINT_MALL_ITEMBONUS, expPer, AFF_NONE, time, 0, true, true);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
								}
								break;
#else
							case ITEM_NOG_POCKET:
								{
									/*
									¶õÁÖ´É·ÂÄ¡ : item_proto value ÀÇ¹Ì
										ÀÌµ¿¼Óµµ  value 1
										°ø°Ý·Â	  value 2
										°æÇèÄ¡	value 3
										Áö¼Ó½Ã°£  value 0 (´ÜÀ§ ÃÊ)

									*/
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
								}
								break;
#endif
							//¶ó¸¶´Ü¿ë »çÅÁ
							case ITEM_RAMADAN_CANDY:
								{
									/*
									»çÅÁ´É·ÂÄ¡ : item_proto value ÀÇ¹Ì
										ÀÌµ¿¼Óµµ  value 1
										°ø°Ý·Â	  value 2
										°æÇèÄ¡	value 3
										Áö¼Ó½Ã°£  value 0 (´ÜÀ§ ÃÊ)

									*/
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case ITEM_MARRIAGE_RING:
								{
									marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());

									if (IsPVPFighting())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to wait 3 seconds before to teleport."));
										break;
									}

									if (pMarriage)
									{
										if (pMarriage->ch1 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
												break;
											}

#ifdef COMBAT_ZONE
											if (CCombatZoneManager::Instance().IsCombatZoneMap(pMarriage->ch1->GetMapIndex()))
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do this on this map."));
												break;
											}
#endif
										}

										if (pMarriage->ch2 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
												break;
											}
#ifdef COMBAT_ZONE
											if (CCombatZoneManager::Instance().IsCombatZoneMap(pMarriage->ch2->GetMapIndex()))
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do this on this map."));
												break;
											}
#endif
										}

										int consumeSP = CalculateConsumeSP(this);

										if (consumeSP < 0)
											return false;

										PointChange(POINT_SP, -consumeSP, false);

										WarpToPID(pMarriage->GetOther(GetPlayerID()));

										SetQuestItemPtr(item);
										quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									}
									else
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°áÈ¥ »óÅÂ°¡ ¾Æ´Ï¸é °áÈ¥¹ÝÁö¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
								}
								break;

#ifdef __COSTUME_ACCE__
							case ACCE_ITEM_REMOVE_ABSORBATION:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->IsEquipped())
										return false;

									if (item2->GetType() != ITEM_COSTUME || item2->GetSubType() != COSTUME_ACCE)
										return false;

									if (item2->GetSocket(1) == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "There is no absorbed item for removing."));
										return false;
									}

									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

									item->SetCount(item->GetCount() - 1);

									item2->SetSocket(1, 0);
									item2->ClearAttribute();

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The absorbed item has been removed from the sash."));
								}
								break;
#endif

								//±âÁ¸ ¿ë±âÀÇ ¸ÁÅä
							case UNIQUE_ITEM_CAPE_OF_COURAGE:
								//¶ó¸¶´Ü º¸»ó¿ë ¿ë±âÀÇ ¸ÁÅä
							case 70057:
							case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
							case UNIQUE_ITEM_CAPE_OF_COURAGE_ALL:
#ifdef BRAVERY_CAPE_STORE
							{
								LPITEM item2 = NULL;

								if (IsValidItemPosition(DestCell) && (item2 = GetItem(DestCell)))
								{
									if(UNIQUE_ITEM_CAPE_OF_COURAGE != item->GetVnum())
									{
										ChatPacket(CHAT_TYPE_INFO, "You can't store here.");
										break;
									}
									
									if (item2->GetVnum() != 93359)
										break;

									if (item2->IsExchanging())
										break;

									item2->SetSocket(0, item2->GetSocket(0) + item->GetCount());
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								else
								{
									int iBraveCapeLastPulse = GetBraveCapeLastPulse();
									int iCurrentPulse = thecore_pulse();
									
									if (iBraveCapeLastPulse > iCurrentPulse)
									{
										break;
									}

									SetBraveCapeLastPulse(iCurrentPulse + (PASSES_PER_SEC(1)/4));

									AggregateMonster(item->GetVnum() == UNIQUE_ITEM_CAPE_OF_COURAGE_ALL);
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
									break;
								}

								break;
							}
							case 93359:
							{
								int iBraveCapeLastPulse = GetBraveCapeLastPulse();
								int iCurrentPulse = thecore_pulse();
								
								if (iBraveCapeLastPulse > iCurrentPulse)
								{
									break;
								}

								SetBraveCapeLastPulse(iCurrentPulse + (PASSES_PER_SEC(1)/4));

								if (item->GetSocket(0) < 1)
								{
									ChatPacket(CHAT_TYPE_INFO, "You don't have other capes stored.");
									break;
								}
								AggregateMonster(true);
								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
								item->SetSocket(0, item->GetSocket(0) - 1);
								break;
							}
#else
							{
								AggregateMonster(item->GetVnum() == UNIQUE_ITEM_CAPE_OF_COURAGE_ALL);
								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
								item->SetCount(item->GetCount() - 1);
								break;
							}
#endif
#ifdef PROMETA
							case UNIQUE_ITEM_CAPE_OF_COURAGE_GOLD:
								int iCost;
								if (GetLevel() <= 55)
									iCost = 25000;
								else
									iCost = 50000;
								
								if (GetGold() < iCost)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have not enough yang to use this item."));
									break;
								}

								PointChange(POINT_GOLD, -iCost);
								AggregateMonster();

								break;

#else
							case UNIQUE_ITEM_CAPE_OF_COURAGE_PERMANENT:
								int iCost;
								if (GetLevel() <= 55)
									iCost = 1000;
								else if (GetLevel() <= 80)
									iCost = 5000;
								else
									iCost = 10000;
								if (GetGold() < iCost)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have not enough yang to use this item."));
									break;
								}

								PointChange(POINT_GOLD, -iCost);
								AggregateMonster();

								break;
#endif
							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								SetQuestItemPtr(item);
								quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							case 30093:
							case 30094:
							case 30095:
							case 30096:
								// º¹ÁÖ¸Ó´Ï
								{
									const int MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										DWORD count;
										int prob;
										DWORD vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									struct LuckyBagInfo b2[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27002 },
										{ 10,	100,	27005 },
										{ 10,	50,	27005 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};
	
									LuckyBagInfo * bi = NULL;
									bi = b1;

									int pct = random_number(1, 1000);

									int i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										// ½ºÅ³¼ö·Ã¼­´Â Æ¯¼öÇÏ°Ô ÁØ´Ù.
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										PointChange(POINT_GOLD, 1000, true);
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 50004: // ÀÌº¥Æ®¿ë °¨Áö±â
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{
										// Ã³À½ »ç¿ë½Ã
										int iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::instance().GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ °÷¿¡¼± ÀÌº¥Æ®¿ë °¨Áö±â°¡ µ¿ÀÛÇÏÁö ¾Ê´Â°Í °°½À´Ï´Ù."));
											return false;
										}
									}

									int dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{
										// ¹ß°ß!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌº¥Æ®¿ë °¨Áö±â°¡ ½Åºñ·Î¿î ºûÀ» ³»¸ç »ç¶óÁý´Ï´Ù."));

										// »ç¿ëÈ½¼ö¿¡ µû¶ó ÁÖ´Â ¾ÆÀÌÅÛÀ» ´Ù¸£°Ô ÇÑ´Ù.
										struct TEventStoneInfo
										{
											DWORD dwVnum;
											int count;
											int prob;
										};
										const int EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int prob = random_number(1, 100);

											for (int i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													AutoGiveItem(info[i].dwVnum, info[i].count);

													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (DWORD)GetVID());

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ¹®ÀÚ±îÁö º¸³»±â

										network::GCOutputPacket<network::GCChatPacket> pack_chat;
										pack_chat->set_type(CHAT_TYPE_COMMAND);
										pack_chat->set_id(0);
										pack_chat->set_empire(GetDesc()->GetEmpire());
										pack_chat->set_message(chatbuf);

										PacketAround(pack_chat);

										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;

									// ¸¹ÀÌ »ç¿ëÇßÀ¸¸é »ç¶óÁø´Ù.
									const int STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌº¥Æ®¿ë °¨Áö±â°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(DWORD)GetVID(), dist, (int)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ¹®ÀÚ±îÁö º¸³»±â

										network::GCOutputPacket<network::GCChatPacket> pack_chat;
										pack_chat->set_type(CHAT_TYPE_COMMAND);
										pack_chat->set_id(0);
										pack_chat->set_empire(GetDesc()->GetEmpire());
										pack_chat->set_message(chatbuf);

										PacketAround(pack_chat);
									}

								}
								break;

							case 27989: // ¿µ¼®°¨Áö±â
							case 76006: // ¼±¹°¿ë ¿µ¼®°¨Áö±â
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

									if (pMap != NULL)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											DWORD max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												DWORD dist = (DWORD)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												stone++;
											}

											if (pTarget != NULL)
											{
												int val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
														(int)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
											ITEM_MANAGER::instance().RemoveItem(item);
										}
									}
									break;
								}
								break;

							case 27996: // µ¶º´
								item->SetCount(item->GetCount() - 1);
								/*if (GetSkillLevel(SKILL_CREATE_POISON))
								  AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE, 3, AFF_DRINK_POISON, 15*60, 0, true);
								  else
								  {
								// µ¶´Ù·ç±â°¡ ¾øÀ¸¸é 50% Áï»ç 50% °ø°Ý·Â +2
								if (random_number(0, 1))
								{
								if (GetHP() > 100)
								PointChange(POINT_HP, -(GetHP() - 1));
								else
								Dead();
								}
								else
								AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE, 2, AFF_DRINK_POISON, 15*60, 0, true);
								}*/
								break;

							case 27987: // Á¶°³
								// 50  µ¹Á¶°¢ 47990
								// 30  ²Î
								// 10  ¹éÁøÁÖ 47992
								// 7   Ã»ÁøÁÖ 47993
								// 3   ÇÇÁøÁÖ 47994
								{
									item->SetCount(item->GetCount() - 1);

									int r = random_number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Á¶°³¿¡¼­ µ¹Á¶°¢ÀÌ ³ª¿Ô½À´Ï´Ù."));
										AutoGiveItem(27990);
									}
									else
									{
										const int prob_table[] =
										{
											94, 97, 99
										};

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Á¶°³°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Á¶°³¿¡¼­ ¹éÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Á¶°³¿¡¼­ Ã»ÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Á¶°³¿¡¼­ ÇÇÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27994);
										}
									}
								}
								break;

							case 71013: // ÃàÁ¦¿ëÆøÁ×
								CreateFly(random_number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100: // ÆøÁ×
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50200: 
								__OpenPrivateShop();
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50301: // Åë¼Ö·Â ¼ö·Ã¼­
							case 50302:
							case 50303:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥Àº ¾Æ¹«¸® ºÁµµ µµ¿òÀÌ µÉ °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									const BYTE abProbs[] = { 50, 40, 25 };
									if (LearnSkillByBook(SKILL_LEADERSHIP, abProbs[item->GetVnum() - 50301]))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = LEADERSHIP_DELAY;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;
#ifdef LEADERSHIP_EXTENSION
							case 94155:
							{
								if (IsPolymorphed() == true)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								int lv = GetSkillLevel(SKILL_LEADERSHIP);

								if (lv < 40)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
									return false;
								}

								if (lv >= 50)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥Àº ¾Æ¹«¸® ºÁµµ µµ¿òÀÌ µÉ °Í °°Áö ¾Ê½À´Ï´Ù."));
									return false;
								}

								if (LearnSkillByBook(SKILL_LEADERSHIP, 25))
								{
									item->SetCount(item->GetCount() - 1);

									int iReadDelay = LEADERSHIP_DELAY;

									SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
								}
							}
							break;
#endif
							case 50304: // ¿¬°è±â ¼ö·Ã¼­
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "·¹º§ 30ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "·¹º§ 50ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿¬°è±â´Â ´õÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;
							case 50311: // ¾ð¾î ¼ö·Ã¼­
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì ¿Ïº®ÇÏ°Ô ¾Ë¾ÆµéÀ» ¼ö ÀÖ´Â ¾ð¾îÀÌ´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 : // ÀÏº» ¸» ¼ÒÈ¯ ½ºÅ³ ¼ö·Ã¼­
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316: // º¯½Å ¼ö·Ã¼­
							case 50323: case 50324: // ÁõÇ÷ ¼ö·Ã¼­
							case 50325: case 50326: // Ã¶Åë ¼ö·Ã¼­
							case 92206: case 92207: case 92208: // passive resist crit
							case 92209: case 92210: case 92211: // passive resist db
#ifdef STANDARD_SKILL_DURATION
							case 93261: case 93262: case 93263: // pasive skill duration
#endif
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}
									
									int iSkillLevelLowLimit = item->GetValue(0);
									int iSkillLevelHighLimit = item->GetValue(1);
									int iPct = MINMAX(0, item->GetValue(2), 100);
									int iLevelLimit = item->GetValue(3);
									DWORD dwSkillVnum = 0;
									
									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										case 92206: case 92207: case 92208:
											dwSkillVnum = SKILL_PASSIVE_RESIST_CRIT;
											break;

										case 92209: case 92210: case 92211:
											dwSkillVnum = SKILL_PASSIVE_RESIST_PENE;
											break;

#ifdef STANDARD_SKILL_DURATION
										case 93261: case 93262: case 93263:
											dwSkillVnum = SKILL_PASSIVE_SKILL_DURATION;
											break;
#endif

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥À» ÀÐÀ¸·Á¸é ·¹º§À» ´õ ¿Ã·Á¾ß ÇÕ´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ Ã¥À¸·Î´Â ´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = SKILL_FISHING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server) 
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server) 
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = SKILL_MINING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										item->SetCount(item->GetCount() - 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = SKILL_HORSE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÁ÷ ½Â¸¶ ½ºÅ³À» ¼ö·ÃÇÒ ¼ö ÀÖ´Â ·¹º§ÀÌ ¾Æ´Õ´Ï´Ù."));
										return false;
									}
									
									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{
											// ÁÖ¾È¼ú¼­ »ç¿ëÁß¿¡´Â ½Ã°£ Á¦ÇÑ ¹«½Ã
											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÁÖ¾È¼ú¼­¸¦ ÅëÇØ ÁÖÈ­ÀÔ¸¶¿¡¼­ ºüÁ®³ª¿Ô½À´Ï´Ù."));
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 || 
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐÀ» ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (random_number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐ¾î ½Â¸¶ ½ºÅ³ Æ÷ÀÎÆ®¸¦ ¾ò¾ú½À´Ï´Ù."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾òÀº Æ÷ÀÎÆ®·Î´Â ½Â¸¶ ½ºÅ³ÀÇ ·¹º§À» ¿Ã¸± ¼ö ÀÖ½À´Ï´Ù."));
										PointChange(POINT_HORSE_SKILL, 1);

										int iReadDelay = random_number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "½Â¸¶ ¼ö·Ã¼­ ÀÌÇØ¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
									}

									item->SetCount(item->GetCount() - 1);
								}
								break;

							case 70102: // ¼±µÎ
							case 70103: // ¼±µÎ
#ifdef UPDATE_3_0_0
							case 95013: 
#endif
								{
									if (GetAlignment() >= 0)
										return false;

									int delta = MIN(-GetAlignment(), item->GetValue(0));

									sys_log(0, "%s ALIGNMENT ITEM %d", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¸¶À½ÀÌ ¸¼¾ÆÁö´Â±º. °¡½¿À» Áþ´©¸£´ø ¹«¾ð°¡°¡ Á» °¡º­¿öÁø ´À³¦ÀÌ¾ß."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼±¾ÇÄ¡°¡ %d Áõ°¡ÇÏ¿´½À´Ï´Ù."), delta/10);
									}
								}
								break;

							case 71107: // Ãµµµº¹¼þ¾Æ
								{
									int val = item->GetValue(0);
									int interval = item->GetValue(1);
									quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());
									int last_use_time = pPC->GetFlag("mythical_peach.last_use_time");

									if (get_global_time() - last_use_time < interval * 60 * 60)
									{
										if (test_server == false)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÁ÷ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
											return false;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Å×½ºÆ® ¼­¹ö ½Ã°£Á¦ÇÑ Åë°ú"));
										}
									}
									
									if (GetAlignment() == 200000)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼±¾ÇÄ¡¸¦ ´õ ÀÌ»ó ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}
									
									if (200000 - GetAlignment() < val * 10)
									{
										val = (200000 - GetAlignment()) / 10;
									}
									
									int old_alignment = GetAlignment() / 10;

									UpdateAlignment(val*10);
									
									item->SetCount(item->GetCount()-1);
									pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

									ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¸¶À½ÀÌ ¸¼¾ÆÁö´Â±º. °¡½¿À» Áþ´©¸£´ø ¹«¾ð°¡°¡ Á» °¡º­¿öÁø ´À³¦ÀÌ¾ß."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼±¾ÇÄ¡°¡ %d Áõ°¡ÇÏ¿´½À´Ï´Ù."), val);

									char buf[256 + 1];
									snprintf(buf, sizeof(buf), "%d %d", old_alignment, GetAlignment() / 10);
									LogManager::instance().CharLog(this, val, "MYTHICAL_PEACH", buf);
								}
								break;

							case 71109: // Å»¼®¼­
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetSocketCount() == 0)
										return false;

									switch( item2->GetType() )
									{
										case ITEM_WEAPON:
											break;
										case ITEM_ARMOR:
											switch (item2->GetSubType())
											{
											case ARMOR_EAR:
											case ARMOR_WRIST:
											case ARMOR_NECK:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
												return false;
											}
											break;

										default:
											return false;
									}

									std::stack<long> socket;

									for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										socket.push(item2->GetSocket(i));

									int idx = ITEM_SOCKET_MAX_NUM - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != NULL)
									{
										item2->SetSocket(idx, 1);

										char buf[256+1];
										snprintf(buf, sizeof(buf), "%s(%u) %s(%u)", 
												item2->GetName(), item2->GetID(), pItemReward->GetName(), pItemReward->GetID());
										LogManager::instance().ItemLog(this, item, "USE_DETACHMENT_ONE", buf);

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:   // Å»»öÁ¦
							case 70202:   // ¿°»ö¾à(Èò»ö)
							case 70203:   // ¿°»ö¾à(±Ý»ö)
							case 70204:   // ¿°»ö¾à(»¡°£»ö)
							case 70205:   // ¿°»ö¾à(°¥»ö)
							case 70206:   // ¿°»ö¾à(°ËÀº»ö)
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÇöÀç Çì¾î½ºÅ¸ÀÏ¿¡¼­´Â ¿°»ö°ú Å»»öÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										quest::CQuestManager& q = quest::CQuestManager::instance();
										quest::PC* pPC = q.GetPC(GetPlayerID());

										if (pPC)
										{
											int last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%d ·¹º§ÀÌ µÇ¾î¾ß ´Ù½Ã ¿°»öÇÏ½Ç ¼ö ÀÖ½À´Ï´Ù."), last_dye_level+3);
											}
										}
									}
								}
								break;

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
									DWORD dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count, item->IsGMOwner()))
									{
										for (int i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
										}

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;


									if (item->GetVnum() == ITEM_VALENTINE_ROSE && SEX_MALE==GET_SEX(this) ||
										item->GetVnum() == ITEM_VALENTINE_CHOCOLATE && SEX_FEMALE==GET_SEX(this))
									{
										// ¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ¾µ ¼ö ¾ø´Ù.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count, item->IsGMOwner()))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;


									if (item->GetVnum() == ITEM_WHITEDAY_CANDY && SEX_MALE==GET_SEX(this) ||
										item->GetVnum() == ITEM_WHITEDAY_ROSE && SEX_FEMALE==GET_SEX(this))
									{
										// ¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ¾µ ¼ö ¾ø´Ù.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count, item->IsGMOwner()))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case 50011: // ¿ù±¤º¸ÇÕ
								{
									DWORD dwBoxVnum = 50011;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count, item->IsGMOwner()))
									{
										for (int i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);
											LogManager::instance().ItemLog(this, item, "MOONLIGHT_GET", buf);

											//ITEM_MANAGER::instance().RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
												break;

											case CSpecialItemGroup::EXP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
												break;

											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::POLY_MARBLE:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "There was a poly marble in the chest!"));
												break;

											default:
												if (item_gets[i])
												{
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(GetLanguageID()), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(GetLanguageID()));
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
										return false;
									}
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									//PointChange(POINT_GOLD, -iCost);
									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50107:
								{
									EffectPacket(SE_CHINA_FIREWORK);
									// ½ºÅÏ °ø°ÝÀ» ¿Ã·ÁÁØ´Ù
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}

									EffectPacket(SE_SPIN_TOP);
									// ½ºÅÏ °ø°ÝÀ» ¿Ã·ÁÁØ´Ù
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_ELK_VNUM: // µ·²Ù·¯¹Ì
								{
									int iGold = item->GetSocket(0);
									ITEM_MANAGER::instance().RemoveItem(item);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), iGold);
									PointChange(POINT_GOLD, iGold);
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 : // º¯½Å ÇØÃ¼ºÎ ÀÓ½Ã
								{
									if (m_pkChrTarget != NULL)
									{
										if (m_pkChrTarget->IsPolymorphed())
										{
											m_pkChrTarget->SetPolymorph(0);
											m_pkChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;

#ifdef ITEM_RARE_ATTR
							case 71051 : // ÁøÀç°¡
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->AddRareAttribute() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼º°øÀûÀ¸·Î ¼Ó¼ºÀÌ Ãß°¡ µÇ¾ú½À´Ï´Ù"));

										int iAddedIdx = item2->GetRareAttrCount() + 4;
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

									/*	LogManager::instance().ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_RARE_ATTR",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum()); */
												
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù"));
									}
								}
								break;

							case 71052 : // ÁøÀç°æ
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->ChangeRareAttribute() == true)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_RARE_ATTR", buf);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯°æ ½ÃÅ³ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù"));
									}
								}
								break;
#endif
							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:
							// ¹«½Ã¹«½ÃÇÏÁö¸¸ ÀÌÀü¿¡ ÇÏ´ø °É °íÄ¡±â´Â ¹«¼·°í...
							// ±×·¡¼­ ±×³É ÇÏµå ÄÚµù. ¼±¹° »óÀÚ¿ë ÀÚµ¿¹°¾à ¾ÆÀÌÅÛµé.
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
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
#ifdef ENABLE_PERMANENT_POTIONS
										case ITEM_AUTO_HP_RECOVERY_PERMANENT:
#endif
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
#ifdef ENABLE_PERMANENT_POTIONS
										case ITEM_AUTO_SP_RECOVERY_PERMANENT:
#endif
											type = AFFECT_AUTO_SP_RECOVERY;
											break;
									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int pos = GetEmptyInventory(item->GetSize());

										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
											break;
										}

										item->SetCount( item->GetCount() - 1 );

										LPITEM item2 = ITEM_MANAGER::instance().CreateItem( item->GetVnum(), 1 );
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect( type );

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (true == isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess( type );
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect( pAffect );

											item->Lock(false);
											item->SetSocket(0, false);
										}
										else
										{
											LPITEM old = FindItemByID( pAffect->dwFlag );

											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect( pAffect );

											EPointTypes bonus = POINT_NONE;

											if (true == isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess( type );
										}
									}
								}
								break;

							default:
#ifdef __FAKE_BUFF__
								sys_log(0, "IsFakeBuffSpawn: vnum %u", item->GetVnum());
								if (CItemVnumHelper::IsFakeBuffSpawn(item->GetVnum()))
								{
									sys_log(0, "IsSpawn TRUE %d", FakeBuff_Owner_GetSpawn());
									if (FakeBuff_Owner_GetSpawn())
									{
										if (FakeBuff_GetItem() == item)
										{
											FakeBuff_Owner_Despawn();
											SetQuestFlag("fake_buff.id", 0);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetLanguageID(), "You have already summoned a fake buff."));
											return false;
										}
									}
									else
									{
										if (GetLevel() < 5)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetLanguageID(), "You need at least level 5 to summon a fake buff."));
											return false;
										}

										if (item->GetItemCooltime() > 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetLanguageID(), "You have to wait until you can use it again."));
											return false;
										}

										if (GetMapIndex() == EMPIREWAR_MAP_INDEX || IsPrivateMap(EVENT_LABYRINTH_MAP_INDEX))
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetLanguageID(), "You can't summon your fake buff on this map."));
											return false;
										}

										bool bSuccess = false;
										for (int i = 0; i < 25 && !bSuccess; ++i)
											bSuccess = FakeBuff_Owner_Spawn(GetX() + random_number(-200, 200), GetY() + random_number(-200, 200), item);

										if (!bSuccess)
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetLanguageID(), "You cannot spawn a fake buff here."));
										else
										{
											item->SetItemCooltime(10);
											SetQuestFlag("fake_buff.id", item->GetID());
										}
									}
								}
#endif
								break;
						}
						break;

					case USE_CLEAR:
						{
							switch (item->GetVnum())
							{
#ifdef ENABLE_ZODIAC_TEMPLE
								case 33025 : // 진재경
									if (GetMapIndex() >= 690000 && GetMapIndex() < 699999)
									{
										if (false == IsDead())
										{
											ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You are already alive and cannot revive using Prism of Revival."));
											return false;
										}

										
										int ZODIAC_REVIVE_ITEM = CountSpecifyItem(33025);
										int REVIVE_PRICE = GetQuestFlag("zodiac.PrismOfRevival") + 1;
										// if (REVIVE_PRICE > 128)
										// {
											// ChatPacket(CHAT_TYPE_INFO, "You exceeded the limit of revive inside Zodiac Temple.");
											// return false;
										// }
										if (ZODIAC_REVIVE_ITEM < GetQuestFlag("zodiac.PrismOfRevival"))
										{
											ChatPacket(CHAT_TYPE_INFO, "You dont have enough Prism of Revival. You need %d.", GetQuestFlag("zodiac.PrismOfRevival"));
											return false;
										}

										ChatPacket(CHAT_TYPE_COMMAND, "CloseRestartWindow");
										GetDesc()->SetPhase(PHASE_GAME);
										SetPosition(POS_STANDING);
										StartRecoveryEvent();
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have been revived using Prism of Revival."));
										RestartAtSamePos();
										// PointChange(POINT_HP, GetHP() - 50);
										PointChange(POINT_HP, GetMaxHP() - GetHP());
										DeathPenalty(0);
										ReviveInvisible(5);
										// Revive price

										if (GetQuestFlag("zodiac.PrismOfRevival") == 5)
										{
											RemoveSpecifyItem(33025, 5);
										}
										else
										{
											RemoveSpecifyItem(33025, REVIVE_PRICE);
											SetQuestFlag("zodiac.PrismOfRevival", REVIVE_PRICE);
										}
									}
									else
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Prism of Revival can be used only inside of Zodiac Temple."));
									break;
#endif
								default:
									RemoveBadAffect();
									item->SetCount(item->GetCount() - 1);
									break;
							}
						}
						break;

					case USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								quest::CQuestManager& q = quest::CQuestManager::instance();
								quest::PC* pPC = q.GetPC(GetPlayerID());

								if (pPC != NULL)
								{
									int last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÁ÷ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0) // HP Àý´ë°ª È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, (int)(item->GetValue(0) * (100.0f + GetPoint(POINT_HEAL_EFFECT_BONUS)) / 100.0f + 0.5f));
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP Àý´ë°ª È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									//PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									PointChange(POINT_SP, item->GetValue(1));
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿ùº´ ¶Ç´Â Á¾ÀÚ ¸¦ »ç¿ëÇÏ¿´½À´Ï´Ù"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case USE_POTION:
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}
						
							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
							}
						}
						
						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							//PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							PointChange(POINT_SP_RECOVERY, item->GetValue(1));
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							float fHealBonus = (100.0f + GetPoint(POINT_HEAL_EFFECT_BONUS));
							if (fHealBonus > 200.0f)
								fHealBonus = 200.0f;
							PointChange(POINT_HP_RECOVERY, (int)(item->GetValue(0) * fHealBonus / 100.0f + 0.5f));
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_ABILITY_UP:
						{
							int iEffectPacket = -1;
							DWORD dwAffType = 0;
							WORD wPointType = 0;
							DWORD dwAffFlag = 0;

							GetDataByAbilityUp(item->GetVnum(), iEffectPacket, dwAffType, wPointType, dwAffFlag);
							tchat("USE_ABILITY_UP %d , %d, %d, %d, %d", item->GetVnum(), iEffectPacket, dwAffType, wPointType, dwAffFlag);

							if (!dwAffType)
							{
								ChatPacket(CHAT_TYPE_INFO, "An error occured. Please inform a team member.");
								return false;
							}

							if (FindAffect(dwAffType))
							{
#ifdef INFINITY_ITEMS
								if (item->IsInfinityItem() && item->GetSocket(2))
								{
									item->SetSocket(2, 0);
									item->Lock(false);
									RemoveAffect(dwAffType);
								}
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
#endif
								return false;
							}

							if (iEffectPacket != -1)
								EffectPacket(iEffectPacket);

#ifdef INFINITY_ITEMS
							if (item->IsInfinityItem())
								AddAffect(dwAffType, wPointType, item->GetValue(2), item->GetValue(5) == 1 ? item->GetVnum() : 0, INFINITE_AFFECT_DURATION, 0, true);
							else
								AddAffect(dwAffType, wPointType, item->GetValue(2), item->GetValue(5) == 1 ? item->GetVnum() : 0, item->GetValue(1), 0, true);
#else
							AddAffect(dwAffType, wPointType,
								item->GetValue(2), item->GetValue(5) == 1 ? item->GetVnum() : 0, item->GetValue(1), 0, true);
#endif

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

#ifdef INFINITY_ITEMS
							if (item->IsInfinityItem())
							{
								item->SetSocket(2, 1);
								item->Lock(true);
							}
							else
								item->SetCount(item->GetCount() - 1);
#else
							item->SetCount(item->GetCount() - 1);
#endif
						}
						break;

					case USE_TALISMAN:
						{
							const int TOWN_PORTAL	= 1;
							const int MEMORY_PORTAL = 2;


							// gm_guild_build, oxevent ¸Ê¿¡¼­ ±ÍÈ¯ºÎ ±ÍÈ¯±â¾ïºÎ ¸¦ »ç¿ë¸øÇÏ°Ô ¸·À½
							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÇöÀç À§Ä¡¿¡¼­ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
								return false;
							}

							if (m_pkWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌµ¿ÇÒ ÁØºñ°¡ µÇ¾îÀÖÀ½À¸·Î ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù"));
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL) // ±ÍÈ¯ºÎ
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿ø·¡ À§Ä¡·Î º¹±Í"));	

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL) // ±ÍÈ¯±â¾ïºÎ
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´øÀü ¾È¿¡¼­´Â %s%s »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."),
												item->GetName(), "");
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case USE_TUNING:
					case USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging())
								return false;
	
							if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343) // ¿µ¼®+3
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "+3 ¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
								return false;
							}
							
							if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)  // ¿µ¼®+4
							{
								if (item->GetVnum() == ITEM_STONE_SCROLL_VNUM) // Ã»·æÀÇ¼û°á
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
								}
							}
							else
							{
								RefineItem(item, item2);
							}
						}
						break;

						//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
					case USE_PUT_INTO_RING_SOCKET:
					case USE_PUT_INTO_ACCESSORY_SOCKET:
					case USE_ADD_ACCESSORY_SOCKET:
					case USE_CLEAN_SOCKET:
					case USE_CHANGE_ATTRIBUTE:
					case USE_CHANGE_ATTRIBUTE2:
					case USE_ADD_ATTRIBUTE:
					case USE_ADD_ATTRIBUTE2:
					case USE_ADD_SPECIFIC_ATTRIBUTE:
#ifdef __DRAGONSOUL__
					case USE_DS_CHANGE_ATTR:
#endif
					case USE_PUT_INTO_ACCESSORY_SOCKET_PERMA:
					case USE_CHANGE_SASH_COSTUME_ATTR:
					case USE_DEL_LAST_PERM_ORE:
						{
							if (GetExchange())
								return false;
							
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped()) // please use goto resetBuffAndReturnFalse instead of return false after this
								BuffOnAttr_RemoveBuffsFromItem(item2);
							
							if (IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_APPLY))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't add or change the bonus of this item."));
								goto resetBuffAndReturnFalse;
							}
							
							// [NOTE] ÄÚ½ºÆ¬ ¾ÆÀÌÅÛ¿¡´Â ¾ÆÀÌÅÛ ÃÖÃÊ »ý¼º½Ã ·£´ý ¼Ó¼ºÀ» ºÎ¿©ÇÏµÇ, Àç°æÀç°¡ µîµîÀº ¸·¾Æ´Þ¶ó´Â ¿äÃ»ÀÌ ÀÖ¾úÀ½.
							// ¿ø·¡ ANTI_CHANGE_ATTRIBUTE °°Àº ¾ÆÀÌÅÛ Flag¸¦ Ãß°¡ÇÏ¿© ±âÈ¹ ·¹º§¿¡¼­ À¯¿¬ÇÏ°Ô ÄÁÆ®·Ñ ÇÒ ¼ö ÀÖµµ·Ï ÇÒ ¿¹Á¤ÀÌ¾úÀ¸³ª
							// ±×µý°Å ÇÊ¿ä¾øÀ¸´Ï ´ÚÄ¡°í »¡¸® ÇØ´Þ·¡¼­ ±×³É ¿©±â¼­ ¸·À½... -_-

							if (item2->IsExchanging())
								goto resetBuffAndReturnFalse;

							if (ITEM_COSTUME == item2->GetType() && item2->GetSubType() != COSTUME_ACCE_COSTUME)
							{
								BYTE socketCount = 0;
								if (item->GetProto())
									socketCount = item->GetProto()->gain_socket_pct();
								if (socketCount > ADDON_COSTUME_NONE && item->GetSubType() == USE_ADD_SPECIFIC_ATTRIBUTE)
								{
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									bool isOk = false;
									switch (socketCount)
									{
									case ADDON_COSTUME_WEAPON:
										if (item2->GetSubType() == COSTUME_WEAPON)
											isOk = true;
										break;
									case ADDON_COSTUME_ARMOR:
										if (item2->GetSubType() == COSTUME_BODY)
											isOk = true;
										break;
									case ADDON_COSTUME_HAIR:
										if (item2->GetSubType() == COSTUME_HAIR)
											isOk = true;
										break;
									}

									if (!isOk)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't use it on this type of item"));
										tchat("socket count: %d", socketCount);
										goto resetBuffAndReturnFalse;
									}

									BYTE bAddAttrType = item->GetAttributeType(0);
									short sAddAttrValue = item->GetAttributeValue(0);

									if (!bAddAttrType)
									{
										ChatPacket(CHAT_TYPE_INFO, "Error. Costume, Contact a team member.");
										goto resetBuffAndReturnFalse;
									}

									/*if (g_map_itemAttr[bAddAttrType].bMaxLevelBySet[item2->GetAttributeSetIndex()] == 0)
									{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot apply this attribute to that item type."));
									return false;
									}*/

									BYTE bEmptyIndex = ITEM_MANAGER::MAX_COSTUME_ATTR_NUM;
									for (BYTE i = 0; i < ITEM_MANAGER::MAX_COSTUME_ATTR_NUM; ++i)
										if (item2->GetAttributeType(i) == APPLY_NONE || item2->GetAttributeValue(i) == 0)
										{
											bEmptyIndex = i;
											break;
										}

									if (bEmptyIndex == ITEM_MANAGER::MAX_COSTUME_ATTR_NUM && item->GetValue(3) == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't add more bonus to this costume with this item."));
										goto resetBuffAndReturnFalse;
									}

									if (item->GetValue(3) == 1)
									{
										bEmptyIndex = ITEM_MANAGER::MAX_COSTUME_ATTR_NUM_TOTAL;
										for (BYTE i = ITEM_MANAGER::MAX_COSTUME_ATTR_NUM; i < ITEM_MANAGER::MAX_COSTUME_ATTR_NUM_TOTAL; ++i)
											if (item2->GetAttributeType(i) == APPLY_NONE || item2->GetAttributeValue(i) == 0)
											{
												bEmptyIndex = i;
												break;
											}

										if (bEmptyIndex == ITEM_MANAGER::MAX_COSTUME_ATTR_NUM_TOTAL)
										{
											ChatPacket(CHAT_TYPE_INFO, "You can't add more bonus to this costume with this item.");
											goto resetBuffAndReturnFalse;
										}
									}

									for (BYTE i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
										if (item2->GetAttributeType(i) == bAddAttrType && item2->GetAttributeValue(i) != 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This bonus is already applied to that item."));
											goto resetBuffAndReturnFalse;
										}

									if (item->IsGMOwner())
										item2->SetGMOwner(true);
										
									item2->SetForceAttribute(bEmptyIndex, bAddAttrType, sAddAttrValue);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

									item->SetCount(item->GetCount() - 1);

									if (item2->IsEquipped())
										BuffOnAttr_AddBuffsFromItem(item2);
									break;
								}

								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								goto resetBuffAndReturnFalse;
							}

							switch (item->GetSubType())
							{
								case USE_CLEAN_SOCKET:
									{
										int i;
										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												break;
										}

										if (i == ITEM_SOCKET_MAX_NUM)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã»¼ÒÇÒ ¼®ÀÌ ¹ÚÇôÀÖÁö ¾Ê½À´Ï´Ù."));
											goto resetBuffAndReturnFalse;
										}

										int j = 0;

										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
												item2->SetSocket(j++, item2->GetSocket(i));
										}

										for (; j < ITEM_SOCKET_MAX_NUM; ++j)
										{
											if (item2->GetSocket(j) > 0)
												item2->SetSocket(j, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CLEAN_SOCKET", buf);
										}

										SetQuestItemPtr(item);
										quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

										item->SetCount(item->GetCount() - 1);

									}
									break;

								case USE_CHANGE_ATTRIBUTE:
								{
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										goto resetBuffAndReturnFalse;
									}

									if (item2->GetAttributeCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
										goto resetBuffAndReturnFalse;
									}

#ifdef EL_COSTUME_ATTR
									if (item2->GetType() == ITEM_COSTUME)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
										goto resetBuffAndReturnFalse;
									}
#endif

									DWORD Zodiacs[] = { 310, 1180, 2200, 3220, 5160, 7300 };
									DWORD targetItemVnum = item2->GetVnum();
									for (int i = 0; i < sizeof(Zodiacs) / sizeof(Zodiacs[0]); ++i)
										if (targetItemVnum >= Zodiacs[i] && targetItemVnum < Zodiacs[i] + 10)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot change this item."));
											goto resetBuffAndReturnFalse;
										}

									if (item->GetSubType() == USE_CHANGE_ATTRIBUTE2)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 0, 30, 40, 3
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									else if (item->GetVnum() == 76014)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] =
										{
											0, 10, 50, 39, 1
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									else
									{
										// ¿¬Àç°æ Æ¯¼öÃ³¸®
										// Àý´ë·Î ¿¬Àç°¡ Ãß°¡ ¾ÈµÉ°Å¶ó ÇÏ¿© ÇÏµå ÄÚµùÇÔ.
										if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
										{
											//if ((item2->GetType() == ITEM_WEAPON)
											//	|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
													break;
												}
											}
											/*else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
												break;
											}*/
										}
										item2->ChangeAttribute();
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
									}

									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

									if (item->GetVnum() != 92870)
									{
										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										item2->SetGMOwner(true);
									}
									break;
								}

#ifdef __DRAGONSOUL__
								case USE_DS_CHANGE_ATTR:
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									if (!item2->IsDragonSoul())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This item is no dragon soul item."));
										goto resetBuffAndReturnFalse;
									}

									if (!DSManager::instance().RerollAttributes(item2))
										goto resetBuffAndReturnFalse;

									LogManager::instance().ItemLog(this, item, "DS_CHANGE_ATTR", "");
									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
									item->SetCount(item->GetCount() - 1);
									break;
#endif

								case USE_CHANGE_SASH_COSTUME_ATTR:
								{
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									if (item2->GetAttributeCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
										goto resetBuffAndReturnFalse;
									}

									DWORD zodiacArm[] = { 310, 1180, 2200, 3220, 5160, 7300 };
									DWORD targetVnum = item2->GetVnum();
									bool isZodiac = false;
									for (int i = 0; i < sizeof(zodiacArm) / sizeof(zodiacArm[0]); ++i)
										if (targetVnum >= zodiacArm[i] && targetVnum < zodiacArm[i] + 10)
										{
											isZodiac = true;
											break;
										}

									if (isZodiac)
									{
										item2->ChangeAttribute();

										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
									}
									else
									{
										if (!ITEM_MANAGER::Instance().ChangeSashAttr(item2))
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The attributes of the selected item could not be changed."));
											goto resetBuffAndReturnFalse;
										}
									}

									SetQuestItemPtr(item);
									quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

									if (item->IsGMOwner())
										item2->SetGMOwner(true);
									item->SetCount(item->GetCount() - 1);
									break;
								}

								case USE_ADD_SPECIFIC_ATTRIBUTE:
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										goto resetBuffAndReturnFalse;
									}

#ifdef EL_COSTUME_ATTR
									if (item2->GetType() == ITEM_COSTUME)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
										goto resetBuffAndReturnFalse;
									}
#endif

									if (!item->GetProto() || item->GetProto()->gain_socket_pct() != ADDON_COSTUME_NONE)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't use it on this type of item"));
										tchat("getsocketcount()=%d", item->GetSocketCount());
										goto resetBuffAndReturnFalse;
									}

									if (item2->GetAttributeCount() < item2->GetType() == ITEM_TOTEM ? ITEM_MANAGER::MAX_TOTEM_NORM_ATTR_NUM : ITEM_MANAGER::MAX_NORM_ATTR_NUM)
									{
										BYTE bAddAttrType = item->GetAttributeType(0);
										short sAddAttrValue = item->GetAttributeValue(0);

										if (!bAddAttrType || g_map_itemAttr.find(bAddAttrType) == g_map_itemAttr.end())
										{
											ChatPacket(CHAT_TYPE_INFO, "Error. Contact a team member.");
											goto resetBuffAndReturnFalse;
										}

										if (g_map_itemAttr[bAddAttrType].max_level_by_set(item2->GetAttributeSetIndex()) == 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot apply this attribute to that item type."));
											goto resetBuffAndReturnFalse;
										}

										BYTE bEmptyIndex = 0;
										for (; bEmptyIndex < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++bEmptyIndex)
										{
											if (item2->GetAttributeType(bEmptyIndex) == APPLY_NONE || item2->GetAttributeValue(bEmptyIndex) == 0)
												break;
										}

										if (bEmptyIndex == ITEM_MANAGER::MAX_NORM_ATTR_NUM)
										{
											goto resetBuffAndReturnFalse;
										}

										// check if existing
										for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
										{
											if (item2->GetAttributeType(i) == bAddAttrType)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This bonus is already applied to that item."));
												goto resetBuffAndReturnFalse;
											}
										}

										if (item2->FindApplyValue(bAddAttrType))
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This bonus is already applied to that item."));
											goto resetBuffAndReturnFalse;
										}

										item2->SetForceAttribute(bEmptyIndex, bAddAttrType, sAddAttrValue);
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

										SetQuestItemPtr(item);
										quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									break;

								case USE_ADD_ATTRIBUTE:
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										goto resetBuffAndReturnFalse;
									}

#ifdef EL_COSTUME_ATTR
									if (item2->GetType() == ITEM_COSTUME)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
										goto resetBuffAndReturnFalse;
									}
#endif

									if (item2->GetAttributeCount() < (item2->GetType() == ITEM_TOTEM ? 3 : 4))
									{
										// ¿¬Àç°¡ Æ¯¼öÃ³¸®
										// Àý´ë·Î ¿¬Àç°¡ Ãß°¡ ¾ÈµÉ°Å¶ó ÇÏ¿© ÇÏµå ÄÚµùÇÔ.
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
											//if ((item2->GetType() == ITEM_WEAPON)
											//	|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
													break;
												}
											}
											/*else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
												break;
											}*/
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (random_number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

											/* int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(
													GetPlayerID(), 
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(), 
													"ADD_ATTRIBUTE_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum()); */
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE_FAIL", buf);
										}

										SetQuestItemPtr(item);
										quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									break;

								case USE_ADD_ATTRIBUTE2:
									if (item2->IsEquipped())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use that on equipped items."));
										goto resetBuffAndReturnFalse;
									}

									// Ãàº¹ÀÇ ±¸½½ 
									// Àç°¡ºñ¼­¸¦ ÅëÇØ ¼Ó¼ºÀ» 4°³ Ãß°¡ ½ÃÅ² ¾ÆÀÌÅÛ¿¡ ´ëÇØ¼­ ÇÏ³ªÀÇ ¼Ó¼ºÀ» ´õ ºÙ¿©ÁØ´Ù.
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										goto resetBuffAndReturnFalse;
									}

#ifdef EL_COSTUME_ATTR
									if (item2->GetType() == ITEM_COSTUME)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do it with this item."));
										goto resetBuffAndReturnFalse;
									}
#endif
									
									// ¼Ó¼ºÀÌ ÀÌ¹Ì 4°³ Ãß°¡ µÇ¾úÀ» ¶§¸¸ ¼Ó¼ºÀ» Ãß°¡ °¡´ÉÇÏ´Ù.
									if (item2->GetAttributeCount() == 4 && item2->GetType() != ITEM_TOTEM)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (random_number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
										/*	LogManager::instance().ItemLog(
													GetPlayerID(),
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(),
													"ADD_ATTRIBUTE2_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum()); */
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE2_FAIL", buf);
										}

										SetQuestItemPtr(item);
										quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

										item->SetCount(item->GetCount() - 1);
									}
									else if (item2->GetAttributeCount() == 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									else if (item2->GetAttributeCount() < 4)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸ÕÀú Àç°¡ºñ¼­¸¦ ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡½ÃÄÑ ÁÖ¼¼¿ä."));
									}
									else
									{
										// wtf ?!
										sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d)", item2->GetAttributeCount());
									}
									break;

								case USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												if (random_number(1, 100) <= 50)
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÄÏÀÌ ¼º°øÀûÀ¸·Î Ãß°¡µÇ¾ú½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÄÏ Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_FAIL", buf);
												}

												SetQuestItemPtr(item);
												quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ¼ÒÄÏÀ» Ãß°¡ÇÒ °ø°£ÀÌ ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼ÒÄÏÀ» Ãß°¡ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										}
									}
									break;

								case USE_DEL_LAST_PERM_ORE:
									{
										BYTE currPerma = item2->GetAccessorySocketGrade(true);
										if (currPerma == 0)
										{
											currPerma = item2->GetAccessorySocketGrade(false);
											if (!currPerma)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "There is no ore in this item."));
												return false;
											}

											item2->SetAccessorySocketGrade(currPerma - 1, false);
											item->SetCount(item->GetCount() - 1);
											break;
										}

										static DWORD s_dwRemoveablePermaOresArray[] = { 93042, 93043, 93044, 93045, 93046, 93047, 93048, 93049, 93050, 93051, 93052, 93053, 93054, 93055, 93056, 93057 };
										DWORD removed = 0;
										for (int i = 0; i < sizeof(s_dwRemoveablePermaOresArray) / sizeof(s_dwRemoveablePermaOresArray[0]); ++i)
										{
											if (CanPutIntoV(item2, s_dwRemoveablePermaOresArray[i], USE_PUT_INTO_ACCESSORY_SOCKET_PERMA))
											{
												item2->SetAccessorySocketGrade(currPerma - 1, true);
												item->SetCount(item->GetCount() - 1);
												removed = s_dwRemoveablePermaOresArray[i];
												break;
											}
										}

										if (!removed)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't remove this permanent ore from this item."));
											return false;
										}
										else
											AutoGiveItem(removed);
									}
									break;

								case USE_PUT_INTO_ACCESSORY_SOCKET_PERMA:
								case USE_PUT_INTO_ACCESSORY_SOCKET:
									if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										if (item2->GetAccessorySocketGradeTotal() < item2->GetAccessorySocketMaxGrade())
										{
											int successRate;
											bool isPerma;
											if (item->GetSubType() == USE_PUT_INTO_ACCESSORY_SOCKET_PERMA)
											{
												successRate = aiAccessorySocketPutPermaPct[item2->GetAccessorySocketGrade(true)];
												isPerma = true;
											}
											else
											{
												successRate = aiAccessorySocketPutPct[item2->GetAccessorySocketGradeTotal()];
												isPerma = false;
											}

											if (random_number(1, 100) <= successRate)
											{
												item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade(isPerma) + 1, isPerma);
												item2->SetAccessorySocketType(item->GetValue(3));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀåÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀåÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_FAIL", buf);
											}

											SetQuestItemPtr(item);
											quest::CQuestManager::instance().OnItemUsed(GetPlayerID());

											item->SetCount(item->GetCount() - 1);
										}
										else
										{
											if (item2->GetAccessorySocketMaxGrade() == 0)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸ÕÀú ´ÙÀÌ¾Æ¸óµå·Î ¾Ç¼¼¼­¸®¿¡ ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
											else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ÀåÂøÇÒ ¼ÒÄÏÀÌ ¾ø½À´Ï´Ù."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´ÙÀÌ¾Æ¸óµå·Î ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
											}
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó º¸¼®À» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¾ÆÀÌÅÛÀ» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									break;
							}

							if (item2->IsEquipped())
								BuffOnAttr_AddBuffsFromItem(item2);
							break;
resetBuffAndReturnFalse:
							if (item2->IsEquipped())
								BuffOnAttr_AddBuffsFromItem(item2);
							return false;
						}
						break;
						//  END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case USE_BAIT:
						{

							if (m_pkFishingEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "³¬½Ã Áß¿¡ ¹Ì³¢¸¦ °¥¾Æ³¢¿ï ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì ²ÈÇôÀÖ´ø ¹Ì³¢¸¦ »©°í %s¸¦ ³¢¿ó´Ï´Ù."), item->GetName(GetLanguageID()));
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "³¬½Ã´ë¿¡ %s¸¦ ¹Ì³¢·Î ³¢¿ó´Ï´Ù."), item->GetName(GetLanguageID()));
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_MOVE:
					case USE_TREASURE_BOX:
					case USE_MONEYBAG:
						break;

					case USE_AFFECT :
						{
							if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
							{
#ifdef INFINITY_ITEMS
								if (item->IsInfinityItem() && item->GetSocket(2))
								{
									item->SetSocket(2, 0);
									item->Lock(false);
									RemoveAffect(FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType));
								}
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
#else
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
#endif
							}
							else
							{
#ifdef INFINITY_ITEMS
								if (item->IsInfinityItem())
								{
									AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), AFF_ATT_SPEED_POTION, INFINITE_AFFECT_DURATION, 0, false);
									item->SetSocket(2, 1);
									item->Lock(true);
								}
								else
								{
									AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), item->GetValue(5) == 1 ? item->GetVnum() : 0, item->GetValue(3), 0, false);
									item->SetCount(item->GetCount() - 1);
								}
#else
								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
								item->SetCount(item->GetCount() - 1);
#endif
							}
						}
						break;

					case USE_CREATE_STONE:
						AutoGiveItem(random_number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					// ¹°¾à Á¦Á¶ ½ºÅ³¿ë ·¹½ÃÇÇ Ã³¸®	
					case USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							DWORD dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							DWORD dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹°¾à Á¶ÇÕÀ» À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹°¾à Á¶ÇÕÀ» À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
									return false;
								}
							}

							if (pSource1 != NULL)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Àç·á(%s)°¡ ºÎÁ·ÇÕ´Ï´Ù."), pSource1->GetName(GetLanguageID()));
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != NULL)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Àç·á(%s)°¡ ºÎÁ·ÇÕ´Ï´Ù."), pSource2->GetName(GetLanguageID()));
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ºó º´ÀÌ ¸ðÀÚ¸¨´Ï´Ù."));
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (random_number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹°¾à Á¦Á¶¿¡ ½ÇÆÐÇß½À´Ï´Ù."));
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging())
					return false;

				if (item2->GetType() == ITEM_PICK) return false;
				if (item2->GetType() == ITEM_ROD) return false;

				if (item2->IsEquipped())
					return false;
				
				int i;

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					DWORD dwVnum;   

					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					auto p = ITEM_MANAGER::instance().GetTable(dwVnum);

					if (!p)
						continue;

					if (item->GetValue(5) == p->values(5))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°°Àº Á¾·ùÀÇ ¸ÞÆ¾¼®Àº ¿©·¯°³ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}

				if (item2->GetType() == ITEM_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¸ÞÆ¾¼®Àº Àåºñ¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}
				else if (item2->GetType() == ITEM_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ¸ÞÆ¾¼®Àº ¹«±â¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}
#ifdef PROMETA
				else if (item2->GetType() == ITEM_COSTUME && item2->GetSubType() == COSTUME_ACCE)
				{
					if (item->GetSubType() != METIN_ACCE)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This stone cannot be added to a sash."));
						return false;
					}
				}
#endif
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
					// disabled ymir stone categories and let only a check triggered by AlterToSocketItem
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
						//if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
						// ¼® È®·ü
						if (random_number(1, 100) <= 30)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸ÞÆ¾¼® ºÎÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
							item2->SetSocket(i, item->GetVnum());
							tchat("ADD-%d", item2->GetSocket(i));
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸ÞÆ¾¼® ºÎÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						LogManager::instance().ItemLog(this, item2, "SOCKET", item->GetName());
						item->SetCount(item->GetCount() - 1);
						// ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (METIN)");

						break;
					}
					else
						tchat("failed to add stone on item2->GetSocket(%d) = %d, item->GetValue(2) = %d", i, item2->GetSocket(i), item->GetValue(2));

				if (i == ITEM_SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
			}
			break;

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
#ifdef SUNDAE_EVENT
		{
			int iceCreamFlavourStart = 95300;
			int	iceCreamFlavourEnd = 95309;
			int iceCreamCup = 95310;

			if (item->GetVnum() >= iceCreamFlavourStart && item->GetVnum() <= iceCreamFlavourEnd)
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (pDestItem && pDestItem->GetVnum() == iceCreamCup)
				{
					int emptySocketIndex = 0;

					for (emptySocketIndex = 0; emptySocketIndex < ITEM_SOCKET_MAX_NUM; ++emptySocketIndex)
					{
						long curSocketValue = pDestItem->GetSocket(emptySocketIndex);

						if (curSocketValue == item->GetVnum())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The Ice Cream cup has already this flavour."));
							return false;
						}

						if (!(curSocketValue >= iceCreamFlavourStart && curSocketValue <= iceCreamFlavourEnd))
						{
							pDestItem->SetSocket(emptySocketIndex, item->GetVnum());
							item->SetCount(item->GetCount() - 1);
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The Ice Cream flavour has been added to the cup."));
							return true;
						}
					}
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The Ice Cream Cup is full."));
					return false;
				}
			}
		}
#endif
			break;
		case ITEM_SPECIAL:
		case ITEM_TOOL:
		case ITEM_LOTTERY:
			break;

		case ITEM_BLEND:
		{
			if (Blend_Item_find(item->GetVnum()))
			{
				int affect_type = AFFECT_BLEND;

				if (item->GetVnum() == 93273)
					affect_type += 1;
				else if (item->GetVnum() == 95219)
					affect_type += 2;
				else if (item->GetVnum() == 95220)
					affect_type += 3;

				if (item->GetSocket(0) >= _countof(aApplyInfo))
				{
					sys_err ("INVALID BLEND ITEM(id : %u, vnum : %d). APPLY TYPE IS %d.", item->GetID(), item->GetVnum(), item->GetSocket(0));
					return false;
				}

				int apply_type = aApplyInfo[item->GetSocket(0)].bPointType;
				int apply_value = item->GetSocket(1);
				int apply_duration = item->GetSocket(2);
				
				tchat("affect_type %d", affect_type);
				tchat("apply_type %d", apply_type);
				tchat("apply_value %d", apply_value);
				tchat("apply_duration %d", apply_duration);
				
				if (apply_type != POINT_POTION_BONUS)
					apply_value = apply_value * (100.0f + GetPointF(POINT_POTION_BONUS)) / 100.0f;

				if (FindAffect(affect_type, apply_type))
				{
#ifdef INFINITY_ITEMS
					if (item->IsInfinityItem() && item->GetSocket(2))
					{
						item->SetSocket(2, 0);
						item->Lock(false);
						RemoveAffect(FindAffect(affect_type, apply_type));
					}
					else if(item->IsInfinityItem() && !item->GetSocket(2))
					{
						RemoveAffect(FindAffect(affect_type, apply_type));
					}
					else
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
#else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
#endif
				}
				else
				{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
					}
					else
					{
#ifdef INFINITY_ITEMS
						if (item->IsInfinityItem())
						{
							AddAffect(affect_type, apply_type, apply_value, 0, INFINITE_AFFECT_DURATION, 0, false);
							item->SetSocket(2, 1);
							item->Lock(true);
						}
						else
						{
							AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
							item->SetCount(item->GetCount() - 1);
						}
#else
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
#endif
					}
				}
			}
		}
		break;

			case ITEM_MOUNT:
			{
				sys_log(!test_server, "%s use mount item %d %s", GetName(), item->GetVnum(), item->GetName());

				CMountSystem* pkMountSystem = GetMountSystem();
				if (!pkMountSystem)
					return false;

				switch (item->GetSubType())
				{
					case MOUNT_SUB_SUMMON:
					{
						if (pkMountSystem->IsRiding())
							pkMountSystem->StopRiding();
						if (pkMountSystem->IsSummoned())
						{
							pkMountSystem->Unsummon();
							if (pkMountSystem->GetSummonItemID() != item->GetID())
								pkMountSystem->Summon(item);
						}
						else
						{
							if (GetSP() < 200)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need %d mana to summon your mount."), 200);
								return false;
							}
						
							if (pkMountSystem->Summon(item))
								PointChange(POINT_SP, -200);
						}
					}
					break;

		case ITEM_ANIMAL_BOTTLE:
		{

			if (true)
			{
				if (IsHorseSummoned() && GetHorseGrade() >= HORSE_MAX_GRADE)

				{
					LPITEM pkSummonItem = FindItemByID(GetMountSystem()->GetSummonItemID());
					if (!pkSummonItem)
					{
						ChatPacket(CHAT_TYPE_INFO, "An error occured.");
						return false;
					}

					BYTE bBonusID = item->GetValue(0);
					auto pProto = CHARACTER_MANAGER::Instance().GetHorseBonus(pkSummonItem->GetHorseBonusLevel(bBonusID));
					if (!pProto)
					{
						sys_err("cannot get proto by horse bonus level %d", pkSummonItem->GetHorseBonusLevel(bBonusID));
						return false;
					}
					else if (pkSummonItem->GetHorseBonusLevel(bBonusID) >= HORSE_MAX_BONUS_LEVEL)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have maxed this bonus already."));
						return false;
					}
					else if (pkSummonItem->GetHorseUsedBottles(bBonusID) >= pProto->item_count())
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have already used enough bottles to enable the upgrade of your horse bonus."));
						return false;
					}

					int iItemVnumNeed;
					switch (bBonusID)
					{
					case 0:
						iItemVnumNeed = pProto->max_hp_item();
						break;
					case 1:
						iItemVnumNeed = pProto->armor_item();
						break;
					case 2:
						iItemVnumNeed = pProto->monster_item();
						break;
					default:
						sys_err("invalid bonus ID %d", bBonusID);
						return false;
					}

					if (iItemVnumNeed != item->GetVnum() && (iItemVnumNeed != 55720 || item->GetVnum() != 92206) && (iItemVnumNeed != 55723 || item->GetVnum() != 92207)
						&& (iItemVnumNeed != 55726 || item->GetVnum() != 92208))
					{
						ChatPacket(CHAT_TYPE_INFO, "You need %s to skill up this bonus.", ITEM_MANAGER::instance().GetItemLink(iItemVnumNeed));
						return false;
					}

					pkSummonItem->SetHorseUsedBottles(bBonusID, pkSummonItem->GetHorseUsedBottles(bBonusID) + 1);
					item->SetCount(item->GetCount() - 1);
				}

			}
		}
		break;

		
				case MOUNT_SUB_FOOD:
				{
					if (item->GetValue(1) != GetHorseGrade() && GetHorseGrade() < HORSE_MAX_GRADE)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need a different food item for your horse."));
						return false;
					}

					if (IsHorseDead())
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your cannot feed a dead horse."));
						return false;
					}

					if (IsHorseRage())
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to wait until the rage mode of your horse ends."));
						return false;
					}

					int iFeedPct = item->GetValue(0);
					int iFeedRagePct = item->GetValue(2);
					if (GetHorseGrade() == HORSE_MAX_GRADE)
						iFeedPct = 0;
					else
						iFeedRagePct = 0;

					if (HorseFeed(iFeedPct, iFeedRagePct))
					{
						item->SetCount(item->GetCount() - 1);
						if (iFeedPct == 0)
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have raised the rage of your horse."));
						else
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have fed your horse."));
					}
				}
				break;

					case MOUNT_SUB_REVIVE:
					{
						if (item->GetValue(1) != GetHorseGrade())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need a different revival item for your horse."));
							return false;
						}

						if (!IsHorseDead())
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your horse is not dead."));
							return false;
						}

						if (HorseRevive())
							item->SetCount(item->GetCount() - 1);
					}
					break;
					
					
					
					
					
				}
			}
			break;

#ifdef __DRAGONSOUL__
		case ITEM_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (NULL == pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;
#endif

		case ITEM_SOUL:
		{
			if (GetExchange())
				return false;

			LPITEM item2;
			if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
				return false;

			if (item2->IsEquipped())
				BuffOnAttr_RemoveBuffsFromItem(item2);

			if (IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_APPLY))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't add or change the bonus of this item."));
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			if (item2->IsExchanging())
			{
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			if (!(ITEM_COSTUME == item2->GetType() && item2->GetSubType() == COSTUME_ACCE_COSTUME))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only use this item on a sash costume!"));
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			if (item2->GetSocket(2) && item2->GetSocket(2) != item->GetSubType())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't use it on this item because other type of bonus is already applied to this item."));
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			if (item->GetValue(3) > item2->GetValue(3) && item->GetValue(3) < 5 || item->GetValue(3) >= 5 && item2->GetValue(3) != 4)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The quality of this sash is too low for this soul."));
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			if (item2->GetAttributeCount() < item->GetValue(3) - 1)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You must add a lower level soul to this item before you can add this."));
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			BYTE bAddAttrType = item->GetAttributeType(0);
			short sAddAttrValue = item->GetAttributeValue(0);

			if (!bAddAttrType)
			{
				ChatPacket(CHAT_TYPE_INFO, "Error. Costume ace, Contact a team member.");
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			BYTE bEmptyIndex = ITEM_MANAGER::MAX_NORM_ATTR_NUM;
			for (BYTE i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
				if (item2->GetAttributeType(i) == APPLY_NONE || item2->GetAttributeValue(i) == 0)
				{
					bEmptyIndex = i;
					break;
				}

			if (bEmptyIndex == ITEM_MANAGER::MAX_NORM_ATTR_NUM)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't add more bonus to this sash with this item."));
				if (item2->IsEquipped())
					BuffOnAttr_AddBuffsFromItem(item2);
				return false;
			}

			for (BYTE i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
				if (item2->GetAttributeType(i) == bAddAttrType && item2->GetAttributeValue(i) != 0)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This bonus is already applied to that item."));
					if (item2->IsEquipped())
						BuffOnAttr_AddBuffsFromItem(item2);
					return false;
				}

			if (item->IsGMOwner())
				item2->SetGMOwner(true);

			if (item2->GetSocket(2) == SOUL_NONE)
				item2->SetSocket(2, item->GetSubType());

			item2->SetForceAttribute(bEmptyIndex, bAddAttrType, sAddAttrValue);
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

			item->SetCount(item->GetCount() - 1);

			if (item2->IsEquipped())
				BuffOnAttr_AddBuffsFromItem(item2);
			break;
		}

		case ITEM_NONE:
			sys_err("Item type NONE %s", item->GetName());
			break;

		default:
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
	}

	return true;
}

int g_nPortalLimitTime = 10;
int g_nPortalGoldLimitTime = 10;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

#ifdef __SKIN_SYSTEM__
	bool bSkipCheckCostume = ( item->GetType() == ITEM_COSTUME && ( item->GetSubType() == COSTUME_WEAPON || item->GetSubType() == COSTUME_BODY  || item->GetSubType() == COSTUME_HAIR ) );
#endif

	sys_log(0, "USE_ITEM (%s) %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

#ifdef __SKIN_SYSTEM__
if(!bSkipCheckCostume)
#endif
	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "±ºÁ÷ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsStun())
		return false;

#ifdef __SKIN_SYSTEM__
	if(!bSkipCheckCostume)
#endif
	if (false == check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

#ifdef COMBAT_ZONE
	if (!CCombatZoneManager::instance().CanUseItem(this, item))
		return false;
#endif

#ifdef __EVENT_MANAGER__
	if (!CEventManager::instance().CanUseItem(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot use this here."));
		return false;
	}
#endif

	if (ITEM_MANAGER::instance().IsDisabledItem(item->GetVnum(), GetMapIndex()) && !item->IsEquipped())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This item can not be used on this map."));
		return false;
	}
	
	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		int iPulse = thecore_pulse();

		//Ã¢°í ¿¬ÈÄ Ã¼Å©
		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¢°í¸¦ ¿¬ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false; 
		}

		//°Å·¡°ü·Ã Ã¢ Ã¼Å©
		if (!CanShopNow() || GetMyShop() || GetShop())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		//PREVENT_REFINE_HACK
		//°³·®ÈÄ ½Ã°£Ã¼Å© 
		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ °³·®ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK
		

		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°³ÀÎ»óÁ¡ »ç¿ëÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
				return false;
			}
			
		}
		//END_PREVENT_ITEM_COPY
		

		//±ÍÈ¯ºÎ °Å¸®Ã¼Å©
		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;
			//±ÍÈ¯±â¾ïºÎ 
			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}
			//±ÍÈ¯ºÎ
			else if (item->GetVnum() == 22000) 
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌµ¿ µÇ¾îÁú À§Ä¡¿Í ³Ê¹« °¡±î¿ö ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));				
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist); 
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE
		//±³È¯ ÈÄ ½Ã°£Ã¼Å©
		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡ ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎµîÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	//º¸µû¸® ºñ´Ü »ç¿ë½Ã °Å·¡Ã¢ Á¦ÇÑ Ã¼Å© 
	if (item->GetVnum() == 50200 | item->GetVnum() == 71049)
	{
		if (!CanShopNow() || GetMyShop() || GetShop())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â º¸µû¸®,ºñ´Üº¸µû¸®¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

	}

	//END_PREVENT_TRADE_WINDOW

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG)) // »ç¿ë ·Î±×¸¦ ³²±â´Â ¾ÆÀÌÅÛ Ã³¸®
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 32 + 1];
		int len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, DestCell);

		if (NULL == ITEM_MANAGER::instance().FindByVID(vid)) // UseItemEx¿¡¼­ ¾ÆÀÌÅÛÀÌ »èÁ¦ µÇ¾ú´Ù. »èÁ¦ ·Î±×¸¦ ³²±è
		{
			LogManager::instance().ItemLog(this, vid, vnum, "REMOVE", hint);
		}
		else if (oldCount != item->GetCount())
		{
			snprintf(hint + len, sizeof(hint) - len, " %u", oldCount - 1);
			LogManager::instance().ItemLog(this, vid, vnum, "USE_ITEM", hint);
		}
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

#ifdef INCREASE_ITEM_STACK
bool CHARACTER::DropItem(TItemPos Cell, WORD bCount)
#else
bool CHARACTER::DropItem(TItemPos Cell, BYTE bCount)
#endif
{
	LPITEM item = NULL; 

	if (!GM::check_allow(GetGMLevel(), GM_ALLOW_DROP_ITEM))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot do this with this gamemaster rank."));
		return false;
	}
	
#ifdef ACCOUNT_TRADE_BLOCK
	if (GetDesc()->IsTradeblocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return false;
	}
#endif

	if (!CanHandleItem())
	{
#ifdef __DRAGONSOUL__
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
#endif
		return false;
	}

	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsGMOwner() && !test_server)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot drop this item."));
		return false;
	}

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹ö¸± ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		return false;
	}

#ifdef __TRADE_BLOCK_SYSTEM__
	if (IsTradeBlocked())
	{
		ChatPacket(CHAT_TYPE_INFO, "You can't do this now. Please write COMA/GA/DEV/SA a message on Discord or Forum.");
		ChatPacket(CHAT_TYPE_INFO, "This is a security mesurement against payment fraud. Don't worry it will resolve shortly.");
		return false;
	}
#endif

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);	// Quickslot ¿¡¼­ Áö¿ò

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				sys_log(0, "[DROP_ITEM] drop item count == 0");
			return false;
		}

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		FN_copy_item_socket(pkItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %i", pkItemToDrop->GetID(), pkItemToDrop->GetCount());
		LogManager::instance().ItemLog(this, item, "ITEM_SPLIT_DROP", szBuf);
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->GetCount() <= 0 || pkItemToDrop->GetCount() > ITEM_MAX_COUNT)
	{
		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %i %i", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "BUG_ABUSE1", szHint);
		return false;
	}
	
	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¶³¾îÁø ¾ÆÀÌÅÛÀº 3ºÐ ÈÄ »ç¶óÁý´Ï´Ù."));
		pkItemToDrop->StartDestroyEvent();

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);
		
		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %i %i", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "DROP", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}

bool CHARACTER::DropGold(long long gold)
{
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot drop gold."));
	return false;
}

#ifdef INCREASE_ITEM_STACK
bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, WORD count)
#else
bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, BYTE count)
#endif
{
	if (test_server)
		sys_log(0, "[%u %s] MoveItem: %u[wnd %u] -> %u[wnd %u] (count %d)",
		GetPlayerID(), GetName(), Cell.cell, Cell.window_type, DestCell.cell, DestCell.window_type, count);

	LPITEM item = NULL;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count && item->GetVnum() != 93359)
		return false;

	if (item->GetSocket(0) != 0 && item->GetSocket(0) < count && item->GetVnum() == 93359)
		return false;

	if (Cell.IsEquipPosition() && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
		return false;

	if (!CanHandleItem())
	{
#ifdef __DRAGONSOUL__
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
#endif
		return false;
	}

	if (ITEM_MANAGER::instance().IsNewWindow(DestCell.window_type) || ITEM_MANAGER::instance().IsNewWindow(Cell.window_type))
	{
		if (DestCell.window_type != Cell.window_type)
		{
			bool bAllow = false;
			if (DestCell.window_type == INVENTORY || Cell.window_type == INVENTORY)
			{
				BYTE bWindow = DestCell.window_type == INVENTORY ? Cell.window_type : DestCell.window_type;
				switch (bWindow)
				{
				case UPPITEM_INVENTORY:
				case STONE_INVENTORY:
				case SKILLBOOK_INVENTORY:
				case ENCHANT_INVENTORY:
#ifdef __COSTUME_INVENTORY__
				case COSTUME_INVENTORY:
#endif
					if (Cell.window_type == bWindow || ITEM_MANAGER::instance().GetTargetWindow(item) == bWindow)
						bAllow = true;
					break;
				}
			}

			if (!bAllow)
				return false;
		}
	}

	if (Cell.IsEquipPosition() && !CanUnequipNow(item))
		return false;

	if (DestCell.IsEquipPosition())
	{
		if (GetItem(DestCell))	// ÀåºñÀÏ °æ¿ì ÇÑ °÷¸¸ °Ë»çÇØµµ µÈ´Ù.
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì Àåºñ¸¦ Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù."));
			
			return false;
		}
		EquipItem(item, DestCell.cell - EQUIPMENT_SLOT_START);
	}
	else
	{
#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					if (DestCell.IsDefaultInventoryPosition())
					{
						if (!GetItem(DestCell))
						{
							BYTE bType, bGrade, bStep, bRefine;
							DSManager::instance().GetDragonSoulInfo(item->GetVnum(), bType, bGrade, bStep, bRefine);

							if (bGrade < DRAGON_SOUL_GRADE_ANCIENT)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only put dragon onyxs with grade ancient or higher into your inventory."));
								return false;
							}

							item->RemoveFromCharacter();
							SetItem(DestCell, item, true);

							return true;
						}
					}

					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}
		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;
#endif

		LPITEM item2;
		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum()) // ÇÕÄ¥ ¼ö ÀÖ´Â ¾ÆÀÌÅÛÀÇ °æ¿ì
		{
			if (!item->CanStackWith(item2))
				return false;

			if (count == 0)
				count = item->GetCount();

			sys_log(0, "%s: ITEM_STACK %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

#ifdef INCREASE_ITEM_STACK
			count = MIN(ITEM_MAX_COUNT - item2->GetCount(), count);
#else
			count = MIN(200 - item2->GetCount(), count);
#endif

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);
			return true;
		}
		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.window_type == DestCell.window_type ? Cell.cell : -1))
#ifdef __ITEM_SWAP_SYSTEM__
		{

#ifdef __COSTUME_INVENTORY__
			if(DestCell.window_type == COSTUME_INVENTORY || Cell.window_type == COSTUME_INVENTORY || item->GetWindow() == COSTUME_INVENTORY)
			{
				tchat("Cant swap [DestCell %d] [Cell %d] [item %d] [COSTUME_INVENTORY %d]", DestCell.window_type, Cell.window_type, item->GetWindow(), COSTUME_INVENTORY);
				return false;
			}
#endif

			if (!Cell.IsEquipPosition() && !DestCell.IsEquipPosition() &&
				(ITEM_MANAGER::instance().IsNewWindow(Cell.window_type) || Cell.window_type == INVENTORY) &&
				Cell.window_type == DestCell.window_type)
			{
				/*if (Cell.window_type != INVENTORY &&
#ifdef __SKILLBOOK_INVENTORY__
					Cell.window_type != SKILLBOOK_INVENTORY &&
#endif
					Cell.window_type != STONE_INVENTORY && Cell.window_type != UPPITEM_INVENTORY)
					return false;*/

				item2 = GetItem(DestCell);

				if (!item2)
					return false;

				if (item2->IsExchanging())
					return false;

				if (true == item2->isLocked())
					return false;

				// if equal size
				if (item->GetSize() == item2->GetSize())
				{
					item->RemoveFromCharacter();
					item2->RemoveFromCharacter();

					SetItem(DestCell, item, true);
					SetItem(Cell, item2, true);

					SyncSwapQuickslot(Cell.cell, DestCell.cell);

					return true;
				}

				//when swaping items have diferent size
				if (item->GetSize() > 1)
				{
					int j = 0;

					int destination_size = 0;
					LPITEM check_item = NULL;

					do
					{
						WORD slot = DestCell.cell + (5 * j);

						//if slot is empty
						if (!(check_item = GetItem(TItemPos(INVENTORY, slot))))
						{
							if ((item->GetSize() - 1) == j)
							{
								if ((DestCell.cell / INVENTORY_PAGE_SIZE) != ((DestCell.cell + (5 * j)) / INVENTORY_PAGE_SIZE))
									return false;
							}
						}
						else{
							check_item = GetItem(TItemPos(INVENTORY, slot));
							destination_size += check_item->GetSize();

							if ((item->GetSize() - 1) == j)
							{
								//last item must have size 1 to swap
								if (check_item->GetSize() > 1)
									return false;

								if ((DestCell.cell / INVENTORY_PAGE_SIZE) != ((DestCell.cell + (5 * j)) / INVENTORY_PAGE_SIZE))
									return false;
							}
						}
					} while (++j < item->GetSize());

					if(item->GetSize() != destination_size)
					{
						int itemSizeMultiplier = (item->GetSize() == 1) ? 1 : item->GetSize() - 1;
						if(DestCell.cell + (5*itemSizeMultiplier) > GetInventoryMaxNum())
						{
							tchat("swap item error DestCell.cell : %d DestCell.cell + (5*item->GetSize()) %d", DestCell.cell, DestCell.cell + (5*item->GetSize()));
							return false;
						}
					}
					
					if ((Cell.cell + 5) == DestCell.cell)
						return false;

					if (item->GetSize() < destination_size)
						return false;

					item->RemoveFromCharacter();

					int k = 0;
					check_item = NULL;

					do
					{
						BYTE slot = DestCell.cell + (5 * k);
						BYTE slot2 = Cell.cell + (5 * k);

						if (!(check_item = GetItem(TItemPos(INVENTORY, slot))))
							continue;
						else
						{
							check_item->RemoveFromCharacter();
							SetItem(TItemPos(INVENTORY, slot2), check_item, true);
						}
					} while (++k < item->GetSize());
					SetItem(DestCell, item, true);

					SyncSwapQuickslot(Cell.cell, DestCell.cell);
					return true;
				}
				else
					return false;
			}
			else
				return false;
		}
#else
			return false;
#endif
		else if (ITEM_MANAGER::instance().IsNewWindow(Cell.window_type))
		{
			if (item2 = GetItem(DestCell))
				return false;
		}
		
		if (count > 1 && item->GetVnum() == 93359 && count <= item->GetSocket(0) && count <= ITEM_MAX_COUNT)
		{
			if (quest::CQuestManager::instance().GetEventFlag("disable_item_split"))
			{
				ChatPacket(CHAT_TYPE_INFO, "This function doesn't work temporarily.");
				return false;
			}
			
			if (item->GetSocket(0) <= 0 || item->GetSocket(0) > ITEM_MAX_COUNT || count <= 0 || count > ITEM_MAX_COUNT)
			{
				char szHint[32 + 1];
				snprintf(szHint, sizeof(szHint), "%s %i %i --- %i", item->GetName(), item->GetSocket(0), item->GetOriginalVnum(), count);
				LogManager::instance().ItemLog(this, item, "BUG_ABUSE6", szHint);
				return false;
			}
			
			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

			item->SetSocket(0, item->GetSocket(0) - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(70038, count);
			item2->SetGMOwner(item->IsGMOwner());

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			item2->AddToCharacter(this, DestCell);

			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
			
			return true;
		}
		else if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			sys_log(0, "%s: ITEM_MOVE %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);
			
			item->RemoveFromCharacter();
#ifndef __MARK_NEW_ITEM_SYSTEM__
			SetItem(DestCell, item);
#else
			SetItem(DestCell, item, true);
#endif

			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			if (quest::CQuestManager::instance().GetEventFlag("disable_item_split"))
			{
				ChatPacket(CHAT_TYPE_INFO, "This function doesn't work temporarily.");
				return false;
			}
			
			if (item->GetCount() <= 0 || item->GetCount() > ITEM_MAX_COUNT || count <= 0 || count > ITEM_MAX_COUNT)
			{
				char szHint[32 + 1];
				snprintf(szHint, sizeof(szHint), "%s %i %i --- %i", item->GetName(), item->GetCount(), item->GetOriginalVnum(), count);
				LogManager::instance().ItemLog(this, item, "BUG_ABUSE6", szHint);
				return false;
			}
			
			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);
			item2->SetGMOwner(item->IsGMOwner());

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			item2->AddToCharacter(this, DestCell);

			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
		}
	}

	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item) 
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center )
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		long long	total;
		LPCHARACTER	c;
		int		x, y;
		long long	iMoney;

		FMoneyDistributor(LPCHARACTER center, long long iMoney) 
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney) 
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);

					if (iMoney > 1000000) // Ãµ¿ø ÀÌ»ó¸¸ ±â·ÏÇÑ´Ù.
						LogManager::instance().CharLog(ch, iMoney, "GET_GOLD", "");
				}
		}
	};
}

void CHARACTER::GiveGold(long long iAmount)
{
	if (iAmount <= 0)
		return;
	
	if (test_server)
		sys_log(0, "GIVE_GOLD: %s %lld", GetName(), iAmount);

	if (GetParty())
	{
		LPPARTY pParty = GetParty();
		long long llTotal = iAmount;
		long long llMyAmount = llTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			long long llShare = llTotal / funcCountNearMember.total;
			llMyAmount -= llShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, llShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, llMyAmount, true);

		if (llMyAmount > 1000000)
			LogManager::instance().CharLog(this, llMyAmount, "GET_GOLD", "");
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);
		if (iAmount > 1000000)
			LogManager::instance().CharLog(this, iAmount, "GET_GOLD", "");
	}
}

bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (GetMyShop())
	{
		sys_err("cannot pickup item %u %s as shop player %u %s", item->GetID(), item->GetName(), GetPlayerID(), GetName());
		return false;
	}

	if (item->DistanceValid(this))
	{
		if (item->IsOwnership(this))
		{
#ifdef BRAVERY_CAPE_STORE
			switch (item->GetVnum())
			{
				case UNIQUE_ITEM_CAPE_OF_COURAGE:
#ifndef ELONIA
				case 70057:
				case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
				case UNIQUE_ITEM_CAPE_OF_COURAGE_ALL:
#endif
				{
					LPITEM item2 = FindSpecifyItem(93359);

					if (item2)
					{
						item2->SetSocket(0, item2->GetSocket(0) + item->GetCount());
						item->RemoveFromGround();
						// M2_DESTROY_ITEM(item);
						ITEM_MANAGER::instance().RemoveItem(item);

						return true;
					}
				}
			}
#endif
			// ¸¸¾à ÁÖÀ¸·Á ÇÏ´Â ¾ÆÀÌÅÛÀÌ ¿¤Å©¶ó¸é
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				if (item->GetCount() > 100000)
					Save();

				M2_DESTROY_ITEM(item);
			}
			// Æò¹üÇÑ ¾ÆÀÌÅÛÀÌ¶ó¸é
			else
			{
				if (item->IsOwnership(NULL) && item->IsGMOwner() && !test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot pick up this item because it's not tradable."));
					return false;
				}

				BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);

#ifdef INCREASE_ITEM_STACK
				WORD bRealCount = item->GetCount();
#else
				BYTE bRealCount = item->GetCount();
#endif
				if (StackFillItem(item, &item))
				{
					if (item->GetCount() <= 0 || item->GetCount() > ITEM_MAX_COUNT)
					{
						char szHint[32 + 1];
						snprintf(szHint, sizeof(szHint), "%s %i %i", item->GetName(), item->GetCount(), item->GetOriginalVnum());
						LogManager::instance().ItemLog(this, item, "BUG_ABUSE5", szHint);
						return false;
					}
					
					// dont log yang coupon drops
					if (!(item->GetVnum() >= 93396 && item->GetVnum() <= 93402))
					{
						char szHint[32 + 1];
						snprintf(szHint, sizeof(szHint), "%s %i", item->GetName(), bRealCount);
						LogManager::instance().ItemLog(this, item, "GET1", szHint);
					}

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));

					if (item->GetType() == ITEM_QUEST)
						quest::CQuestManager::instance().PickupItem(GetPlayerID(), item);
					return true;
				}

				int iEmptyCell;
#ifdef __DRAGONSOUL__
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						sys_log(0, "No empty ds inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else
#endif
				if ((iEmptyCell = GetEmptyInventoryNew(bWindow, item->GetSize())) == -1)
				{
					sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
					return false;
				}

				item->RemoveFromGround();
				if (item->GetCount() <= 0 || item->GetCount() > ITEM_MAX_COUNT)
				{
					char szHint[32 + 1];
					snprintf(szHint, sizeof(szHint), "%s %i %i", item->GetName(), item->GetCount(), item->GetOriginalVnum());
					LogManager::instance().ItemLog(this, item, "BUG_ABUSE4", szHint);
					return false;
				}
				
				item->AddToCharacter(this, TItemPos(bWindow, iEmptyCell));

				char szHint[80+1];
				snprintf(szHint, sizeof(szHint), "%s %i %i realCount %i", item->GetName(), item->GetCount(), item->GetOriginalVnum(), bRealCount);
				LogManager::instance().ItemLog(this, item, "GET2", szHint);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));

				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty() && !item->IsGMOwner())
		{
			// ´Ù¸¥ ÆÄÆ¼¿ø ¼ÒÀ¯±Ç ¾ÆÀÌÅÛÀ» ÁÖÀ¸·Á°í ÇÑ´Ù¸é
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;
			
			// Pickup Steal Fix
			if (!owner)
				return false;  

#ifdef BRAVERY_CAPE_STORE
			switch (item->GetVnum())
			{
				case UNIQUE_ITEM_CAPE_OF_COURAGE:
#ifndef ELONIA
				case 70057:
				case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
				case UNIQUE_ITEM_CAPE_OF_COURAGE_ALL:
#endif
				{
					LPITEM item2 = FindSpecifyItem(93359);

					if (item2)
					{
						item2->SetSocket(0, item2->GetSocket(0) + item->GetCount());
						item->RemoveFromGround();
						// M2_DESTROY_ITEM(item);
						ITEM_MANAGER::instance().RemoveItem(item);

						return true;
					}
				}
			}
#endif

			if (PickupItem_PartyStack(owner, item))
				return true;

			BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);
			int iEmptyCell;

#ifdef __DRAGONSOUL__
			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(owner, "¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else
#endif
			if (!(owner && (iEmptyCell = owner->GetEmptyInventoryNew(bWindow, item->GetSize())) != -1))
			{
				owner = this;

				if (PickupItem_PartyStack(owner, item))
					return true;

				if ((iEmptyCell = GetEmptyInventoryNew(bWindow, item->GetSize())) == -1)
				{
					owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(owner, "¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
					return false;
				}
			}

			item->RemoveFromGround();

			if (item->GetCount() <= 0 || item->GetCount() > ITEM_MAX_COUNT)
			{
				char szHint[32 + 1];
				snprintf(szHint, sizeof(szHint), "%s %i %i", item->GetName(), item->GetCount(), item->GetOriginalVnum());
				LogManager::instance().ItemLog(this, item, "BUG_ABUSE3", szHint);
				return false;
			}
			
			item->AddToCharacter(owner, TItemPos(bWindow, iEmptyCell));

			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %i %i", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(owner, item, "GET3", szHint);

			if (owner == this)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(owner, "¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s"), item->GetName(GetLanguageID()), GetName());
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName(GetLanguageID()));
			}

			if (item->GetType() == ITEM_QUEST)
				quest::CQuestManager::instance().PickupItem (owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::PickupItem_PartyStack(LPCHARACTER owner, LPITEM item)
{
#ifdef INCREASE_ITEM_STACK
	WORD bRealCount = item->GetCount();
#else
	BYTE bRealCount = item->GetCount();
#endif
	if (owner && owner->StackFillItem(item, &item))
	{
		if (item->GetCount() <= 0 || item->GetCount() > ITEM_MAX_COUNT)
		{
			char szHint[32 + 1];
			snprintf(szHint, sizeof(szHint), "%s %i %i", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(this, item, "BUG_ABUSE2", szHint);
			return false;
		}
#ifdef __ENABLE_FULL_LOGS__
		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %i %i realCount %i", item->GetName(), item->GetCount(), item->GetOriginalVnum(), bRealCount);
		LogManager::instance().ItemLog(owner, item, "GET4", szHint);
#endif
		if (owner == this)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));
		else
		{
			owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(owner, "¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s"), item->GetName(GetLanguageID()), GetName());
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName(GetLanguageID()));
		}

		if (item->GetType() == ITEM_QUEST)
			quest::CQuestManager::instance().PickupItem(owner->GetPlayerID(), item);

		return true;
	}

	return false;
}

bool CHARACTER::SwapItem(WORD wCell, WORD wDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(INVENTORY, wCell), destCell(INVENTORY, wDestCell);

#ifdef __DRAGONSOUL__
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;
#endif

	if (srcCell.IsShiningEquipPosition())
		return false;

#ifdef __SKIN_SYSTEM__
	//if(srcCell.IsSkinSystemEquipPosition() || destCell.IsSkinSystemEquipPosition())
	//	return false;
#endif

	// °°Àº CELL ÀÎÁö °Ë»ç
	if (wCell == wDestCell)
		return false;

	// µÑ ´Ù ÀåºñÃ¢ À§Ä¡¸é Swap ÇÒ ¼ö ¾ø´Ù.
	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	// item2°¡ ÀåºñÃ¢¿¡ ÀÖ´Â °ÍÀÌ µÇµµ·Ï.
	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(wDestCell);
		item2 = GetInventoryItem(wCell);
	}
	else
	{
		item1 = GetInventoryItem(wCell);
		item2 = GetInventoryItem(wDestCell);
	}

	if (!item1 || !item2)
		return false;
	
	if (item1 == item2)
	{
		sys_log(0, "[WARNING][WARNING][HACK USER!] : %s %d %d", m_stName.c_str(), wCell, wDestCell);
		return false;
	}

	int iInvenPos = item1->GetCell();

	if (!IsEmptyItemGrid(TItemPos(INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
	{
		iInvenPos = GetEmptyInventory(item2->GetSize());
		if (iInvenPos == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	if(item1->GetCount() > 1)
	{
		iInvenPos = GetEmptyInventory(item2->GetSize());

		if(iInvenPos == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	// ¹Ù²Ü ¾ÆÀÌÅÛÀÌ ÀåºñÃ¢¿¡ ÀÖÀ¸¸é
	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - EQUIPMENT_SLOT_START;

		// Âø¿ëÁßÀÎ ¾ÆÀÌÅÛÀ» ¹þÀ» ¼ö ÀÖ°í, Âø¿ë ¿¹Á¤ ¾ÆÀÌÅÛÀÌ Âø¿ë °¡´ÉÇÑ »óÅÂ¿©¾ß¸¸ ÁøÇà
		if (IS_SET(item2->GetFlag(), ITEM_FLAG_IRREMOVABLE) || IS_SET(item1->GetFlag(), ITEM_FLAG_IRREMOVABLE))
			return false;

		if (bEquipCell != item1->FindEquipCell(this)) // °°Àº À§Ä¡ÀÏ¶§¸¸ Çã¿ë
			return false;

		if (item1->GetType() == ITEM_WEAPON && item1->GetSubType() != item2->GetSubType() && GetWear(WEAR_COSTUME_WEAPON))
		{
			LPITEM pkWearCostume = GetWear(WEAR_COSTUME_WEAPON);

			int iCostumePos = -1;
			for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
				if (IsEmptyItemGrid(TItemPos(INVENTORY, i), pkWearCostume->GetSize(), item1->GetCell()))
				{
					bool bAvailPos = true;
					for (int j = iInvenPos; j < iInvenPos + 5 * item1->GetSize(); iInvenPos += 5)
					{
						if (j == i)
						{
							bAvailPos = false;
							break;
						}
					}

					if (bAvailPos)
					{
						iCostumePos = i;
						break;
					}
				}

			if (iCostumePos == -1)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have no empty inventory for your weapon costume."));
				return false;
			}

			pkWearCostume->RemoveFromCharacter();
			pkWearCostume->AddToCharacter(this, TItemPos(INVENTORY, iCostumePos));
		}

		item2->RemoveFromCharacter();

		if(item1->GetCount() > 1)
		{
			LPITEM tempItem = ITEM_MANAGER::instance().CreateItem(item1->GetVnum());

			if(!tempItem)
				return false;

			ITEM_MANAGER::instance().CopyAllAttrTo(item1, tempItem);

			if(tempItem->EquipTo(this, bEquipCell))
				item2->AddToCharacter(this, TItemPos(INVENTORY, iInvenPos));
			else
			{
				sys_err("SwapItem (count>1) cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
				ITEM_MANAGER::instance().DestroyItem(tempItem);
			}
		}
		else
		{
			if(item1->EquipTo(this, bEquipCell))
				item2->AddToCharacter(this, TItemPos(INVENTORY, iInvenPos));
			else
				sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
		}
	}
	else
	{
		WORD wCell1 = item1->GetCell();
		WORD wCell2 = item2->GetCell();
		
		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

		item1->AddToCharacter(this, TItemPos(INVENTORY, wCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, wCell1));
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
	int pos;

	if (false == CanUnequipNow(item))
		return false;

	BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);

#ifdef __DRAGONSOUL__
	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
#endif
		pos = GetEmptyInventoryNew(bWindow, item->GetSize());
	if (pos == -1)
		return false;

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	item->AddToCharacter(this, TItemPos(bWindow, pos));

#ifdef __SKIN_SYSTEM__
	if(item->GetType() == ITEM_COSTUME)
	{
		if(item->GetSubType() == COSTUME_PET)
			ResummonPet();

		if(item->GetSubType() == COSTUME_MOUNT)
			ResummonMount();
	}
#endif

	CheckMaximumPoints();
#ifdef __FAKE_PC__
	FakePC_Owner_ExecFunc(&CHARACTER::CheckMaximumPoints);
#endif

	return true;
}

#ifdef __SKIN_SYSTEM__
void CHARACTER::ResummonPet()
{
	if(GetPetSystem())
	{
		auto pPetSystem = GetPetSystem();

		CPetActor* pet = pPetSystem->GetSummoned();

		if(!pet || !pet->IsSummoned())
			return;

		DWORD itemVID = pet->GetSummonItemVID();

		if(!itemVID)
			return;

		if(ITEM_MANAGER::instance().FindByVID(itemVID))
		{
			LPITEM item = ITEM_MANAGER::instance().FindByVID(itemVID);
			pet->Unsummon();
			pPetSystem->Summon(item->GetValue(0), item, 0, false);
		}

	}
}

void CHARACTER::ResummonMount()
{
	if(m_pkMountSystem && m_pkMountSystem->IsSummoned())
	{
		DWORD itemVnum = m_pkMountSystem->GetSummonItemID();

		if(ITEM_MANAGER::instance().Find(itemVnum))
		{
			LPITEM item = ITEM_MANAGER::instance().Find(itemVnum);

			if(!item)
				return;

			m_pkMountSystem->Unsummon();
			m_pkMountSystem->Summon(item);
		}
	}
}
#endif

//
// @version	05/07/05 Bang2ni - Skill »ç¿ëÈÄ 1.5 ÃÊ ÀÌ³»¿¡ Àåºñ Âø¿ë ±ÝÁö
//
bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell, bool bCheckNecessaryOnly, bool bCheckLimits, bool force)
{
	if (test_server)
		sys_log(0, "EquipItem %u %s to %s", item->GetID(), item->GetName(), GetName());
	
	if (item->IsExchanging())
		return false;

	if (!CanShopNow() || GetMyShop())
	{
		ChatPacket(CHAT_TYPE_INFO, "Close the shop to equip an item.");
		return false;
	}

	if (false == item->IsEquipable())
		return false;

	int iWearCell = item->FindEquipCell(this, iCandidateCell);

	this->tchat("CHARACTER::EquipItem | Name: %s, Candidate cell: %d | FindEquipCell: %d", item->GetName(), iCandidateCell, iWearCell);

#ifdef __SKIN_SYSTEM__
	bool bCanSkipChecks = ( iWearCell == SKINSYSTEM_SLOT_BUFFI_BODY || iWearCell == SKINSYSTEM_SLOT_BUFFI_WEAPON  || iWearCell == SKINSYSTEM_SLOT_BUFFI_HAIR);
#endif

	if(iWearCell < 0)
		return false;

#ifdef __SKIN_SYSTEM__
	if(!bCanSkipChecks)
#endif

	if (!CanEquipNow(item, NPOS, NPOS, force))
		return false;

	// ¹«¾ð°¡¸¦ Åº »óÅÂ¿¡¼­ ÅÎ½Ãµµ ÀÔ±â ±ÝÁö
	if (!bCheckNecessaryOnly && iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸»À» Åº »óÅÂ¿¡¼­ ¿¹º¹À» ÀÔÀ» ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (!bCheckNecessaryOnly && iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µÐ°© Áß¿¡´Â Âø¿ëÁßÀÎ Àåºñ¸¦ º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

#ifdef __SKIN_SYSTEM__
	if(!bCanSkipChecks)
#endif
	if (check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	//½Å±Ô Å»°Í »ç¿ë½Ã ±âÁ¸ ¸» »ç¿ë¿©ºÎ Ã¼Å©
	if(item->IsRideItem() && IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì Å»°ÍÀ» ÀÌ¿ëÁßÀÔ´Ï´Ù."));
		return false;
	}

#ifdef __SKIN_SYSTEM__
	if(item->GetType() == ITEM_COSTUME)
	{
		if(item->GetSubType() == COSTUME_MOUNT && IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cant equip this item while riding."));
			return false;
		}

		// buffi skin
		if(bCanSkipChecks && FakeBuff_Owner_GetSpawn())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Please unsummon your buffi to equip this item."));
			return false;
		}
	}
#endif

	if (item->GetType() == ITEM_SHINING)
	{
		if (HasShining(item->GetValue(0)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This shining is already equiped."));
			return false;
		}

		BYTE shiningIndex;
		if (item->GetSubType() == SHINING_BODY)
			shiningIndex = 0;
		else
			shiningIndex = 2;

		BYTE usedCount = 0;
		LPITEM currItem = NULL;
		for (BYTE i = shiningIndex; i < shiningIndex + 2; ++i)
			if (currItem = GetItem(TItemPos(EQUIPMENT, SHINING_EQUIP_SLOT_START + i)))
				usedCount += currItem->GetSize();

		if (usedCount + item->GetSize() > 2)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You don't have enough space to equip this shining."));
			return false;
		}
	}

#ifndef EQUIP_WHILE_ATTACKING
	// È­»ì ÀÌ¿Ü¿¡´Â ¸¶Áö¸· °ø°Ý ½Ã°£ ¶Ç´Â ½ºÅ³ »ç¿ë 1.5 ÈÄ¿¡ Àåºñ ±³Ã¼°¡ °¡´É
	DWORD dwCurTime = get_dword_time();

	if (!bCheckNecessaryOnly
		&& iWearCell != WEAR_ARROW
		&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°¡¸¸È÷ ÀÖÀ» ¶§¸¸ Âø¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return false;
	}
#endif

#ifdef CHECK_TIME_AFTER_PVP
	if (IsPVPFighting())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to wait three seconds before you can change equipment."));
		return false;
	}
#endif
	
#ifdef __DRAGONSOUL__
	if (item->IsDragonSoul())
	{
		// °°Àº Å¸ÀÔÀÇ ¿ëÈ¥¼®ÀÌ ÀÌ¹Ì µé¾î°¡ ÀÖ´Ù¸é Âø¿ëÇÒ ¼ö ¾ø´Ù.
		// ¿ëÈ¥¼®Àº swapÀ» Áö¿øÇÏ¸é ¾ÈµÊ.
		if (GetWear(iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì °°Àº Á¾·ùÀÇ ¿ëÈ¥¼®À» Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù."));
			return false;
		}

		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	// ¿ëÈ¥¼®ÀÌ ¾Æ´Ô.
	else
	{
#endif
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			// ÀÌ ¾ÆÀÌÅÛÀº ÇÑ¹ø ¹ÚÈ÷¸é º¯°æ ºÒ°¡. swap ¿ª½Ã ¿ÏÀü ºÒ°¡
			if (item->GetWearFlag() == WEARABLE_ABILITY)
				return false;

			if (false == SwapItem(item->GetCell(), EQUIPMENT_SLOT_START + iWearCell))
			{
				return false;
			}
		}
		else
		{
			WORD wOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, wOldCell, iWearCell);
			}
		}
#ifdef __DRAGONSOUL__
	}
#endif

	if (true == item->IsEquipped())
	{
		// ¾ÆÀÌÅÛ ÃÖÃÊ »ç¿ë ÀÌÈÄºÎÅÍ´Â »ç¿ëÇÏÁö ¾Ê¾Æµµ ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä Ã³¸®. 
		if (-1 != item->GetProto()->limit_real_time_first_use_index())
		{
			// ÇÑ ¹øÀÌ¶óµµ »ç¿ëÇÑ ¾ÆÀÌÅÛÀÎÁö ¿©ºÎ´Â Socket1À» º¸°í ÆÇ´ÜÇÑ´Ù. (Socket1¿¡ »ç¿ëÈ½¼ö ±â·Ï)
			if (0 == item->GetSocket(1))
			{
				// »ç¿ë°¡´É½Ã°£Àº Default °ªÀ¸·Î Limit Value °ªÀ» »ç¿ëÇÏµÇ, Socket0¿¡ °ªÀÌ ÀÖÀ¸¸é ±× °ªÀ» »ç¿ëÇÏµµ·Ï ÇÑ´Ù. (´ÜÀ§´Â ÃÊ)
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->limits(item->GetProto()->limit_real_time_first_use_index()).value() + time(0);

				if (0 == duration)
					duration = 60 * 60 * 24 * 7 + time(0);

				item->SetSocket(0, duration);
				item->StartRealTimeExpireEvent();
				
				char szHint[256];
				snprintf(szHint, sizeof(szHint), "duration %d timeout %u");
				LogManager::instance().ItemLog(this, item, "FIRST_USE_EXPIRE", szHint);
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const DWORD& dwVnum = item->GetVnum();

		// ÇÒ·ÎÀ© »çÅÁ(71136) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		if (true == CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}
		// Çàº¹ÀÇ ¹ÝÁö(71143) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		else if (true == CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}
		// »ç¶ûÀÇ ÆÒ´øÆ®(71145) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		else if (true == CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
#ifdef __ACCE_COSTUME__
		else if (true == CItemVnumHelper::IsAcceItem(dwVnum))
		{
			this->EffectPacket(SE_ACCE_EQUIP);
		}
#endif
		// ITEM_UNIQUEÀÇ °æ¿ì, SpecialItemGroup¿¡ Á¤ÀÇµÇ¾î ÀÖ°í, (item->GetSIGVnum() != NULL)
		// 
		else if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}

		if (UNIQUE_SPECIAL_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE))
		{
			quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
		}

		// if (ITEM_SHINING == item->GetType() ||
			// ITEM_COSTUME == item->GetType() && (item->GetSubType() == COSTUME_BODY || item->GetSubType() == COSTUME_HAIR || item->GetSubType() == COSTUME_WEAPON))
			// ChatPacket(CHAT_TYPE_COMMAND, "OpenCostumeWindow");

#ifdef __SKIN_SYSTEM__
		if(item->GetType() == ITEM_COSTUME)
		{
			// if(item->GetSubType() == COSTUME_PET || item->GetSubType() == COSTUME_MOUNT)
				// ChatPacket(CHAT_TYPE_COMMAND, "OpenCostumeWindow");

			if(item->GetSubType() == COSTUME_PET)
				ResummonPet();

			if(item->GetSubType() == COSTUME_MOUNT)
				ResummonMount();
		}
#endif
	}

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (int i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (int i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if(0 == bOldValue)
	{
		CBuffOnAttributes* pBuff;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
				{
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			case POINT_COSTUME_ATTR_BONUS:
				{
					static BYTE abSlot[] = { WEAR_COSTUME_BODY, WEAR_COSTUME_HAIR, WEAR_COSTUME_WEAPON };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));

		}
		else
			pBuff = it->second;
			
		pBuff->On(bNewValue);
	}
	else
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->ChangeBuffValue(bNewValue);
	}
}


LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
	for (int i = 0; i < EQUIPMENT_SLOT_START; ++i)
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);

	return NULL;
}

#ifdef __EQUIPMENT_CHANGER__
LPITEM CHARACTER::FindItemByID(DWORD id, bool search_equip) const
{
	WORD wMaxItemNum = search_equip ? INVENTORY_AND_EQUIP_SLOT_MAX : EQUIPMENT_SLOT_START;
	for (int i = 0; i < wMaxItemNum; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}
#else
LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	for (int i = 0; i < EQUIPMENT_SLOT_START; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}
#endif

LPITEM CHARACTER::FindItemByVID(DWORD vid) const
{
	for (int i = 0; i < EQUIPMENT_SLOT_START; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetVID() == vid)
			return GetInventoryItem(i);
	}

	return NULL;
}

int CHARACTER::CountSpecifyItem(DWORD vnum, LPITEM except) const
{
	int	count = 0;
	LPITEM item;

	for (int i = 0; i < EQUIPMENT_SLOT_START; ++i)
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum && item != except)
		{
			// °³ÀÎ »óÁ¡¿¡ µî·ÏµÈ ¹°°ÇÀÌ¸é ³Ñ¾î°£´Ù.
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
				continue;
			else
				count += item->GetCount();
		}
	}

	return count;
}

bool CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count, LPITEM except)
{
	bool bAnyGMItemRemoved = false;

	if (0 == count)
		return bAnyGMItemRemoved;

	for (UINT i = 0; i < EQUIPMENT_SLOT_START; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum || GetInventoryItem(i) == except)
			continue;

		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (GetInventoryItem(i)->IsGMOwner())
			bAnyGMItemRemoved = true;

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return bAnyGMItemRemoved;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return bAnyGMItemRemoved;
		}
	}

	// ¿¹¿ÜÃ³¸®°¡ ¾àÇÏ´Ù.
	if (count)
		sys_log(0, "CHARACTER::RemoveSpecifyItem cannot remove enough item vnum %u, still remain %d", vnum, count);

	return bAnyGMItemRemoved;
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;

	for (int i = 0; i < EQUIPMENT_SLOT_START; ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < EQUIPMENT_SLOT_START; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		//°³ÀÎ »óÁ¡¿¡ µî·ÏµÈ ¹°°ÇÀÌ¸é ³Ñ¾î°£´Ù. (°³ÀÎ »óÁ¡¿¡¼­ ÆÇ¸ÅµÉ¶§ ÀÌ ºÎºÐÀ¸·Î µé¾î¿Ã °æ¿ì ¹®Á¦!)
		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
}

LPITEM CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return NULL;
	}
	if (item->GetOwner())
	{
		sys_err ("item %u 's owner exists!",item->GetID());
		return item;
	}

	// try to stack the item
	if (TryStackItem(item, &item))
		return item;

	// add item into inventory if could not stack
	BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item->GetVnum());

	WORD wSlotStart, wSlotEnd;
	ITEM_MANAGER::instance().GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, true, this);

	int cell;
#ifdef __DRAGONSOUL__
	if (item->IsDragonSoul())
		cell = GetEmptyDragonSoulInventory(item);
	else
#endif
		cell = GetEmptyInventoryNew(bWindow, item->GetSize());
	if (cell != -1)
	{
		item->AddToCharacter(this, TItemPos(bWindow, cell));

		LogManager::instance().ItemLog(this, item, "SYSTEM1", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type() == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.set_type(QUICKSLOT_TYPE_ITEM);
				slot.set_pos(cell);
				SetQuickslot(0, slot);
			}
		}
	}
	// add item to ground if could not add to inventory
	else
	{
		item->AddToGround (GetMapIndex(), GetXYZ());
		item->StartDestroyEvent();

		if (longOwnerShip)
			item->SetOwnership (this, 300);
		else
			item->SetOwnership (this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}

	return item;
}

bool CHARACTER::CanStackItem(LPITEM item, LPITEM* ppNewItem, bool bCheckFullCount)
{
	BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item->GetVnum());
	tchat("Window: %d", bWindow);
	// sys_err("Window: %d", bWindow);

#ifdef __DRAGONSOUL__
	if (bWindow == DRAGON_SOUL_INVENTORY)
	{
		if (ppNewItem)
			*ppNewItem = NULL;
		return false;
	}
#endif

	WORD wSlotStart, wSlotEnd;
	ITEM_MANAGER::instance().GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, true, this);
	tchat("Slots %d %d", wSlotStart, wSlotEnd);
	// sys_err("Slots %d %d", wSlotStart, wSlotEnd);
	if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
	{
		for (int i = wSlotStart; i < wSlotEnd; ++i)
		{
			if (LPITEM pCurItem = GetInventoryItem(i))
			{
				if (pCurItem->CanStackWith(item) && pCurItem->GetCount() < ITEM_MAX_COUNT && (!bCheckFullCount || pCurItem->GetCount() + item->GetCount() <= ITEM_MAX_COUNT))
				{
					if (ppNewItem)
						*ppNewItem = pCurItem;

					return true;
				}
			}
		}
	}
	tchat("% sreturn false;", __FUNCTION__);
	// sys_err("% sreturn false;", __FUNCTION__);
	if (ppNewItem)
		*ppNewItem = NULL;
	return false;
}

bool CHARACTER::TryStackItem(LPITEM item, LPITEM* ppNewItem)
{
	LPITEM pkNewItem;
	if (CanStackItem(item, &pkNewItem))
	{
		DoStackItem(item, pkNewItem);
		if (ppNewItem)
			*ppNewItem = pkNewItem;

		return true;
	}

	return false;
}

void CHARACTER::DoStackItem(LPITEM item, LPITEM pkNewItem, bool removeOld)
{
	pkNewItem->SetCount(item->GetCount() + pkNewItem->GetCount());

#ifdef __ENABLE_FULL_LOGS__
	char szLogBuf[256];
	snprintf(szLogBuf, sizeof(szLogBuf), "[old_ID %u] add_count %u => %u", item->GetID(), pkNewItem->GetCount() - item->GetCount(), pkNewItem->GetCount());
	LogManager::instance().ItemLog(this, pkNewItem, "INC_COUNT", szLogBuf);
#endif

	if (removeOld)
		M2_DESTROY_ITEM(item);
}

bool CHARACTER::StackFillItem(LPITEM item, LPITEM* ppNewItem)
{
	LPITEM pOldItem = item;
	LPITEM pkNewItem;
	while (CanStackItem(pOldItem, &pkNewItem, false))
	{
#ifdef INCREASE_ITEM_STACK
		WORD bNewCount = MAX(0, static_cast<int>(pOldItem->GetCount()) - static_cast<int>(ITEM_MAX_COUNT - pkNewItem->GetCount()));
#else
		BYTE bNewCount = MAX(0, pOldItem->GetCount() - (ITEM_MAX_COUNT - pkNewItem->GetCount()));
#endif
		DoStackItem(pOldItem, pkNewItem, false);
		if (ppNewItem)
			*ppNewItem = pkNewItem;
		if (bNewCount == 0)
			ITEM_MANAGER::instance().RemoveItem(pOldItem);
		else
			pOldItem->SetCount(bNewCount);

		if (bNewCount == 0)
			return true;
	}

	return false;
}

LPITEM CHARACTER::AutoGiveItem(const network::TItemData* pTable, bool bUseCell)
{
	LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(pTable);
	if (pkItem)
	{
		if (bUseCell)
		{
			BYTE bWindow = ITEM_MANAGER::Instance().GetTargetWindow(pTable->vnum());

			if (bWindow == INVENTORY && pTable->cell().cell() < GetInventoryMaxNum())
			{
				TItemPos itemPos(INVENTORY, pTable->cell().cell());
				if (itemPos.IsValidItemPosition() && pTable->cell().cell() + 5 * (pkItem->GetProto()->size() - 1) < GetInventoryMaxNum())
				{
					if (IsEmptyItemGrid(itemPos, pkItem->GetProto()->size()))
						pkItem->AddToCharacter(this, itemPos);
				}
			}
#ifdef __DRAGONSOUL__
			else if (bWindow == DRAGON_SOUL_INVENTORY)
			{
				if (DragonSoul_IsQualified())
				{
					WORD wBaseCell = DSManager::instance().GetBasePosition(pkItem);
					if (WORD_MAX != wBaseCell)
					{
						TItemPos itemPos(bWindow, pTable->cell().cell());

						for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
						{
							if (i + wBaseCell == pTable->cell().cell() && IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), pkItem->GetSize()))
								pkItem->AddToCharacter(this, itemPos);
						}
					}
				}
			}
#endif
			else if (bWindow != INVENTORY)
			{
				WORD wSlotStart, wSlotEnd;
				if (ITEM_MANAGER::instance().GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, this))
				{
					TItemPos itemPos(bWindow, pTable->cell().cell());
					if (itemPos.IsValidItemPosition() && pTable->cell().cell() >= wSlotStart && pTable->cell().cell() < wSlotEnd && GetItem(TItemPos(bWindow, pTable->cell().cell())) == NULL)
						pkItem->AddToCharacter(this, itemPos);
				}
			}
		}

		if (!pkItem->GetOwner())
			AutoGiveItem(pkItem);
	}

	return pkItem;
}

#ifdef INCREASE_ITEM_STACK
LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum, WORD bCount, int iRarePct, bool bMsg)
#else
LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum, BYTE bCount, int iRarePct, bool bMsg)
#endif
{
	auto p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	LogManager::instance().MoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->flags() & ITEM_FLAG_STACKABLE && p->type() != ITEM_BLEND) 
	{
		for (int i = 0; i < EQUIPMENT_SLOT_START; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->flags(), ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->values(1))
						bCount = p->values(1);
				}
#ifdef INCREASE_ITEM_STACK
				WORD bCount2 = MIN(ITEM_MAX_COUNT - item->GetCount(), bCount);
#else
				BYTE bCount2 = MIN(200 - item->GetCount(), bCount);
#endif
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));

					return item;
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (TryStackItem(item, &item))
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));

		return item;
	}

	BYTE bWindow = ITEM_MANAGER::instance().GetTargetWindow(item);

	WORD wSlotStart, wSlotEnd;
	ITEM_MANAGER::instance().GetInventorySlotRange(bWindow, wSlotStart, wSlotEnd, true, this);

	int iEmptyCell;

#ifdef __DRAGONSOUL__
	if (item->IsDragonSoul())
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	else
#endif
		iEmptyCell = GetEmptyInventoryNew(bWindow, item->GetSize());
	if (iEmptyCell != -1)
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName(GetLanguageID()));

		item->AddToCharacter(this, TItemPos(bWindow, iEmptyCell));
		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type() == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.set_type(QUICKSLOT_TYPE_ITEM);
				slot.set_pos(iEmptyCell);
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
		item->StartDestroyEvent();
		// ¾ÈÆ¼ µå¶ø flag°¡ °É·ÁÀÖ´Â ¾ÆÀÌÅÛÀÇ °æ¿ì, 
		// ÀÎº¥¿¡ ºó °ø°£ÀÌ ¾ø¾î¼­ ¾îÂ¿ ¼ö ¾øÀÌ ¶³¾îÆ®¸®°Ô µÇ¸é,
		// ownershipÀ» ¾ÆÀÌÅÛÀÌ »ç¶óÁú ¶§±îÁö(300ÃÊ) À¯ÁöÇÑ´Ù.
		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}

	sys_log(!test_server, "AutoGiveItem: %d %d", dwItemVnum, bCount);
	return item;
}

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
#ifdef __MELEY_LAIR_DUNGEON__
		case MeleyLair::STATUE_VNUM:
			return MeleyLair::CMgr::instance().IsMeleyMap(from->GetMapIndex());
#endif
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && 
					(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
			if (item->GetType() == ITEM_WEAPON && 
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
			if (item->GetType() == ITEM_ARMOR && 
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:
			if (item->GetRefineSet() >= 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH_STONE_MOB:
			if (item->GetType() == ITEM_METIN)
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
			if (item->GetRefinedVnum())
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:
			// ÃÊ±Þ ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:
			// Áß±Þ ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:
			// °í±Þ ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
#ifdef __MELEY_LAIR_DUNGEON__
		case MeleyLair::STATUE_VNUM:
			if (MeleyLair::CMgr::instance().IsMeleyMap(from->GetMapIndex()))
				MeleyLair::CMgr::instance().OnKillStatue(item, from, this, from->GetGuild());
			break;
#endif
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

			// DEVILTOWER_NPC 
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			}
			break;
			// END_OF_DEVILTOWER_NPC

		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
		case BLACKSMITH_STONE_MOB:
			if (item->GetRefinedVnum())
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
		case 20119:
			if (item->GetType() == ITEM_MOUNT && item->GetSubType() == MOUNT_SUB_REVIVE)
			{
				if (item->GetValue(1) != GetHorseGrade())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need a different revival item for your horse."));
					break;
				}

				if (!IsHorseDead())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your horse is not dead."));
					break;
				}

				if (from->HorseRevive())
				{
					item->SetCount(item->GetCount() - 1);
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "¸»¿¡°Ô ¼±ÃÊ¸¦ ÁÖ¾ú½À´Ï´Ù."));
				}
			}
			else if (item->GetType() == ITEM_MOUNT && item->GetSubType() == MOUNT_SUB_FOOD)
			{
				if (item->GetValue(1) != from->GetHorseGrade() && (from->GetHorseGrade() < CHARACTER::HORSE_MAX_GRADE))
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "You need a different feed item for your horse."));
					break;
				}

				if (from->IsHorseDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "Your cannot feed a dead horse."));
					break;
				}

				if (from->IsHorseRage())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(from, "You have to wait until the rage mode of your horse ends."));
					return;
				}

				int iFeedPct = item->GetValue(0);
				int iFeedRagePct = item->GetValue(2);
				if (GetHorseGrade() == HORSE_MAX_GRADE && item->GetValue(1) != from->GetHorseGrade())
					iFeedPct = 0;
				else
					iFeedRagePct = 0;

				if (from->HorseFeed(iFeedPct, iFeedRagePct))
				{
					if (iFeedPct == 0)
						from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have raised the rage of your horse."));
					else
						from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have fed your horse."));
					item->SetCount(item->GetCount() - 1);
					EffectPacket(SE_HPUP_RED);
				}
			}
			break;

		default:
			sys_log(0, "TakeItem %s %d %s", from->GetName(), GetRaceNum(), item->GetName());
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	// ¾ð¾î¹ÝÁöÀÎ °æ¿ì ¾ð¾î¹ÝÁö(°ßº») ÀÎÁöµµ Ã¼Å©ÇÑ´Ù.
	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC( NULL );
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD> &dwItemVnums, 
											std::vector<DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count, bool bIsGMOwner)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess = false;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				PointChange(POINT_GOLD, dwCount);
				LogManager::instance().CharLog(this, dwCount, "TREASURE_GOLD", "");

				bSuccess = true;
				break;
			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);
					LogManager::instance().CharLog(this, dwCount, "TREASURE_EXP", "");

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					sys_log(0, "CSpecialItemGroup::MOB %d", dwCount);
					int x = GetX() + random_number(-500, 500);
					int y = GetY() + random_number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
					{
						bSuccess = true;
						ch->SetAggressive();
					}
				}
				break;
			case CSpecialItemGroup::SLOW:
				{
					sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP()*dwCount/100;
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(NULL);
					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB_GROUP:
				{
					int sx = GetX() - random_number(300, 500);
					int sy = GetY() - random_number(300, 500);
					int ex = GetX() + random_number(300, 500);
					int ey = GetY() + random_number(300, 500);
					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);
					if (ch)
					{
						bSuccess = true;
					}
				}
				break;

			case CSpecialItemGroup::POLY_MARBLE:
				{
					item_get = ITEM_MANAGER::instance().CreateItem(70104, 1);

					if (item_get)
					{
						if (bIsGMOwner)
							item_get->SetGMOwner(bIsGMOwner);
						item_get->SetSocket(0, dwCount, false);

						item_get = AutoGiveItem(item_get, true);
						if (item_get)
							bSuccess = true;
					}
				}
				break;
			default:
				{
					item_get = ITEM_MANAGER::instance().CreateItem(dwVnum, dwCount, 0, false, iRarePct);

					if (item_get)
					{
						if (bIsGMOwner)
							item_get->SetGMOwner(bIsGMOwner);
						item_get = AutoGiveItem(item_get, true);
						bSuccess = true;
					}
				}
				break;
		}
	
		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;

		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		// ·¹º§ Á¦ÇÑ¿¡ °É¸²
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÁ÷ ÀÌ ¸Ó¸®¸¦ »ç¿ëÇÒ ¼ö ¾ø´Â ·¹º§ÀÔ´Ï´Ù."));
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000; // 73001 - 72000 = 1001 ºÎÅÍ Çì¾î ¹øÈ£ ½ÃÀÛ
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;

#ifdef __WOLFMAN__
		case JOB_WOLFMAN:
			break;
#endif

		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µ¿ÀÏÇÑ ¸Ó¸® ½ºÅ¸ÀÏ·Î´Â ±³Ã¼ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't do this on this map."));
		return false;
	}
#endif
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ¹Ì µÐ°©ÁßÀÎ »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	if (true == IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µÐ°©ÇÒ ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{
				// µÐ°©±¸ Ã³¸®
				sys_log(0, "USE_POLYMORPH_BALL PID(%d) vnum(%d)", GetPlayerID(), dwVnum);

				// ·¹º§ Á¦ÇÑ Ã¼Å©
				int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.level() >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "³ªº¸´Ù ³Ê¹« ³ôÀº ·¹º§ÀÇ ¸ó½ºÅÍ·Î´Â º¯½Å ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				DWORD dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID)/40) * 100;

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);
				
				item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				// º¸·ù

				// µÐ°©¼­ Ã³¸®
				// ¼ÒÄÏ0				¼ÒÄÏ1		   ¼ÒÄÏ2   
				// µÐ°©ÇÒ ¸ó½ºÅÍ ¹øÈ£   ¼ö·ÃÁ¤µµ		µÐ°©¼­ ·¹º§
				sys_log(0, "USE_POLYMORPH_BOOK: %s(%u) vnum(%u)", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
				{
					CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;

	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	LPITEM Unique1 = GetWear(WEAR_UNIQUE1);
	LPITEM Unique2 = GetWear(WEAR_UNIQUE2);

	if( NULL != Unique1 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup() )
		{
			return UnequipItem(Unique1);
		}
	}

	if( NULL != Unique2 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup() )
		{
			return UnequipItem(Unique2);
		}
	}

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(DWORD) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (false == CArenaManager::instance().IsArenaMap(GetMapIndex()))
			{
				const long amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);
				
				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;
				int32_t amount_need = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				float fHealEffectBonus = GetPoint(POINT_HEAL_EFFECT_BONUS);
				if (fHealEffectBonus > 100.0f)
					fHealEffectBonus = 100.0f;
				amount_need = (int)(amount * (100.0f - fHealEffectBonus) / 100.0f + 0.5f);

				if (amount_need > 0)
				{
					if (avail > amount_need)
					{
						const int pct_of_used = amount_of_used * 100 / amount_of_full;
						const int pct_of_will_used = (amount_of_used + amount_need) * 100 / amount_of_full;

						bool bLog = false;
						// »ç¿ë·®ÀÇ 10% ´ÜÀ§·Î ·Î±×¸¦ ³²±è
						// (»ç¿ë·®ÀÇ %¿¡¼­, ½ÊÀÇ ÀÚ¸®°¡ ¹Ù²ð ¶§¸¶´Ù ·Î±×¸¦ ³²±è.)
						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;

#ifdef ENABLE_PERMANENT_POTIONS
						if (pItem->GetVnum() != ITEM_AUTO_HP_RECOVERY_PERMANENT && pItem->GetVnum() != ITEM_AUTO_SP_RECOVERY_PERMANENT)
							pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount_need, bLog);
#else
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount_need, bLog);
#endif
					}
					else
					{
						amount = (int)(avail * (100.0f + GetPoint(POINT_HEAL_EFFECT_BONUS)) / 100.0f + 0.5f);

						ITEM_MANAGER::instance().RemoveItem(pItem);
					}

					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange(POINT_HP_RECOVERY, amount);
						//EffectPacket( SE_AUTO_HPUP );
						EffectPacket(SE_HPUP_RED);
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange(POINT_SP_RECOVERY, amount);
						//EffectPacket( SE_AUTO_SPUP );
						EffectPacket(SE_SPUP_BLUE);
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}
		else
		{
			RemoveAffect( const_cast<CAffect*>(pAffect) );
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;

	if (ITEM_MANAGER::instance().IsNewWindow(window_type) || window_type == INVENTORY)
	{
		if (window_type == INVENTORY)
		{
			for (int iWnd = 0; iWnd < WINDOW_MAX_NUM; ++iWnd)
			{
				if (ITEM_MANAGER::instance().IsNewWindow(iWnd))
				{
					WORD wSlotStart, wSlotEnd;
					if (!ITEM_MANAGER::instance().GetInventorySlotRange(iWnd, wSlotStart, wSlotEnd, (const LPCHARACTER)this))
						continue;

					if (cell >= wSlotStart && cell < wSlotEnd)
						return true;
				}
			}
		}
		else
		{
			WORD wSlotStart, wSlotEnd;
			if (!ITEM_MANAGER::instance().GetInventorySlotRange(window_type, wSlotStart, wSlotEnd, (const LPCHARACTER)this))
				return false;

			return cell >= wSlotStart && cell < wSlotEnd;
		}
	}
	
	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		if ((cell >= INVENTORY_SLOT_START && cell < INVENTORY_SLOT_START + GetInventoryMaxNum()) || (cell >= EQUIPMENT_SLOT_START && cell < EQUIPMENT_SLOT_END))
			return true;

#ifdef __DRAGONSOUL__
		if (cell >= DRAGON_SOUL_EQUIP_SLOT_START && cell < DRAGON_SOUL_EQUIP_RESERVED_SLOT_END)
			return true;
#endif
		if (cell >= SHINING_EQUIP_SLOT_START && cell < SHINING_EQUIP_RESERVED_SLOT_END)
			return true;

#ifdef __SKIN_SYSTEM__
		if(cell >= SKINSYSTEM_EQUIP_SLOT_START && cell < SKINSYSTEM_EQUIP_SLOT_END)
			return true;
#endif

		return false;

#ifdef __DRAGONSOUL__
	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);
#endif

	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;
	default:
		return false;
	}
}


// ±ÍÂú¾Æ¼­ ¸¸µç ¸ÅÅ©·Î.. exp°¡ true¸é msg¸¦ Ãâ·ÂÇÏ°í return false ÇÏ´Â ¸ÅÅ©·Î (ÀÏ¹ÝÀûÀÎ verify ¿ëµµ¶ûÀº return ¶§¹®¿¡ ¾à°£ ¹Ý´ë¶ó ÀÌ¸§¶§¹®¿¡ Çò°¥¸± ¼öµµ ÀÖ°Ú´Ù..)
#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, msg)); \
			return false; \
	}

		
/// ÇöÀç Ä³¸¯ÅÍÀÇ »óÅÂ¸¦ ¹ÙÅÁÀ¸·Î ÁÖ¾îÁø itemÀ» Âø¿ëÇÒ ¼ö ÀÖ´Â Áö È®ÀÎÇÏ°í, ºÒ°¡´É ÇÏ´Ù¸é Ä³¸¯ÅÍ¿¡°Ô ÀÌÀ¯¸¦ ¾Ë·ÁÁÖ´Â ÇÔ¼ö
bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell,  bool force) /*const*/
{
	auto itemTable = item->GetProto();
	BYTE itemType = item->GetType();
	BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;

#ifdef __WOLFMAN__
		case JOB_WOLFMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
			break;
#endif
	}

	if (test_server)
	{
		for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		{
			if (itemTable->applies(i).type())
				tchat("item: [%d][%d] ", itemTable->applies(i).type(), itemTable->applies(i).value());
		}
	}
	
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->limits(i).value();
		switch (itemTable->limits(i).type())
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "·¹º§ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

#ifdef ENABLE_LEVEL_LIMIT_MAX
			case LEVEL_LIMIT_MAX:
				if (GetLevel() >= limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your level exceeds the item's max level."));
					return false;
				}
				break;
#endif

			case LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "±Ù·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Áö´ÉÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹ÎÃ¸ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¼·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°°Àº Á¾·ùÀÇ À¯´ÏÅ© ¾ÆÀÌÅÛ µÎ °³¸¦ µ¿½Ã¿¡ ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) && 
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°áÈ¥ÇÏÁö ¾ÊÀº »óÅÂ¿¡¼­ ¿¹¹°À» Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

	}
	
	if (ITEM_MANAGER::instance().IsDisabledItem(item->GetVnum(), GetMapIndex()))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This item can not be equipped on this map."));
		return false;
	}

	if (!force && item->GetType() == ITEM_COSTUME)
	{
		if (item->GetSubType() == COSTUME_WEAPON)
		{
			LPITEM pkWeapon = GetWear(WEAR_WEAPON);
			if (!pkWeapon || pkWeapon->GetSubType() != item->GetValue(0) || pkWeapon->GetType() != ITEM_WEAPON)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to wear the same weapon type as your costume weapon as real weapon."));
				tchat("pkWeapon->GetSubType()[%d] != item->GetValue(0)[%d] || pkWeapon->GetType()[%d] != ITEM_WEAPON[%d]", pkWeapon?pkWeapon->GetSubType():-1, item->GetValue(0), pkWeapon?pkWeapon->GetType():-1, ITEM_WEAPON);
				return false;
			}
		}
		else if (item->GetSubType() == COSTUME_ACCE_COSTUME && !GetWear(WEAR_ACCE))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to equip a real sash before equiping a costume for it."));
			return false;
		}
	}

	if (!force && item->FindEquipCell(this) == WEAR_WEAPON)
	{
		if (GetWear(WEAR_COSTUME_WEAPON) && (GetWear(WEAR_COSTUME_WEAPON)->GetValue(0) != item->GetSubType() || item->GetType() != ITEM_WEAPON))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You are wearing a different costume weapon type!"));
			return false;
		}
	}

	return true;
}

/// ÇöÀç Ä³¸¯ÅÍÀÇ »óÅÂ¸¦ ¹ÙÅÁÀ¸·Î Âø¿ë ÁßÀÎ itemÀ» ¹þÀ» ¼ö ÀÖ´Â Áö È®ÀÎÇÏ°í, ºÒ°¡´É ÇÏ´Ù¸é Ä³¸¯ÅÍ¿¡°Ô ÀÌÀ¯¸¦ ¾Ë·ÁÁÖ´Â ÇÔ¼ö
bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{	
	// ¿µ¿øÈ÷ ÇØÁ¦ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛ
	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	// ¾ÆÀÌÅÛ unequip½Ã ÀÎº¥Åä¸®·Î ¿Å±æ ¶§ ºó ÀÚ¸®°¡ ÀÖ´Â Áö È®ÀÎ
	{
		int pos;
#ifdef __DRAGONSOUL__
		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
#endif
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG( -1 == pos, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù." );
	}

	if (item->GetType() == ITEM_WEAPON && GetWear(WEAR_COSTUME_WEAPON))
	{
		BYTE bSize = item->GetSize();
		int iPos1 = -1;
		int iPos2 = -1;

		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
			if (IsEmptyItemGrid(TItemPos(INVENTORY, i), bSize))
			{
				if (iPos1 != -1)
				{
					bool bPosEnable = true;
					for (int j = iPos1; j < iPos1 + bSize * 5; j += 5)
					{
						if (j == i)
						{
							bPosEnable = false;
							break;
						}
					}

					if (bPosEnable)
					{
						iPos2 = i;
						break;
					}
				}
				else
					iPos1 = i;
			}

		if (iPos1 == -1 || iPos2 == -1)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have no empty inventory to unequip your weapon or weapon costume."));
			return false;
		}
	}

	if (item->GetType() == ITEM_WEAPON)
	{
		if (LPITEM pkCostume = GetWear(WEAR_COSTUME_WEAPON))
		{
			if (!UnequipItem(pkCostume))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to unequip your costume weapon before unequipping your real weapon."));
				return false;
			}
		}
	}

	if (item->GetType() == ITEM_COSTUME && item->GetSubType() == COSTUME_ACCE)
	{
		if (LPITEM pkCostume = GetWear(WEAR_COSTUME_ACCE))
		{
			if (!UnequipItem(pkCostume))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to unequip your sash costume before unequipping your real sash."));
				return false;
			}
		}
	}

#ifdef __SKIN_SYSTEM__
	if(item->GetType() == ITEM_COSTUME)
	{
		if(item->GetSubType() == COSTUME_MOUNT && IsRiding())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cant unequip this item while riding."));
			return false;
		}

		if(item->IsEquippedInBuffiSkinCell() && FakeBuff_Owner_GetSpawn())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Please unsummon your buffi to unequip this item."));
			return false;
		}
	}
#endif

	return true;
}

#ifdef __ACCE_COSTUME__
int GetAcceRefineGrade(DWORD vnum)
{
	if (vnum >= 85001 && vnum <= 85004)
		return vnum - 85000;
	else if (vnum >= 85005 && vnum <= 85008)
		return vnum - 85004;
	else if (vnum >= 85011 && vnum <= 85014)
		return vnum - 85010;
	else if (vnum >= 85015 && vnum <= 85018)
		return vnum - 85014;
	else if (vnum >= 85021 && vnum <= 85024)
		return vnum - 85020;
	else if (vnum >= 93331 && vnum <= 93334)
		return vnum - 93330;
	else
		return 0;
}

void CHARACTER::AcceRefineCheckin(BYTE acceSlot, TItemPos currentCell)
{
	if (GetAcceWindowType() >= 2)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "INVALID_ACCE_WINDOW_HACKER_QST"));
		return;
	}

	if (IsOpenSafebox() || IsCubeOpen())
		return;

	if (!IsValidItemPosition(currentCell))
		return;

	if (IsOpenSafebox())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_PLEASE_CLOSE_SAFEBOX"));
		return;
	}

	if (m_pointsInstant.pAcceSlots[acceSlot] != WORD_MAX)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_SLOT_ALREADY_IN_USE"));
		return;
	}

	LPITEM pkItem = GetItem(currentCell);

	if (pkItem == NULL)
		return;

	if (ABSORB == GetAcceWindowType())
	{
		//ChatInfoCond(test_server, "AcceRefineCheckin Window Type Absorb %d", acceSlot);
		if (acceSlot == ACCE_SLOT_RIGHT)
		{
			LPITEM pkAcceItem = GetInventoryItem(m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT]);
			if (!pkAcceItem)
				return;

			switch (pkAcceItem->GetValue(1))
			{
#ifdef ELONIA
			case 1:
			case 2:
			case 3:
			case 4:
#endif
			case 0:
				if (pkItem->GetType() != ITEM_WEAPON || (pkItem->GetSubType() >= WEAPON_ARROW
#ifdef __WOLFMAN__
				&& pkItem->GetSubType() != WEAPON_CLAW
#endif
				))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only absorb weapons into that sash."));
					return;
				}
				break;
#ifndef ELONIA
			case 1:
				if (pkItem->GetType() != ITEM_ARMOR || (pkItem->GetSubType() != ARMOR_BODY && pkItem->GetSubType() != ARMOR_HEAD))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only absorb armors and helmets into that sash."));
					return;
				}
				break;

			case 2:
				if (pkItem->GetType() != ITEM_ARMOR || (pkItem->GetSubType() != ARMOR_WRIST && pkItem->GetSubType() != ARMOR_EAR && pkItem->GetSubType() != ARMOR_NECK))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only absorb bracelets, earrings and necklaces into that sash."));
					return;
				}
				break;

			case 3:
				if (pkItem->GetType() != ITEM_ARMOR || (pkItem->GetSubType() != ARMOR_FOOTS && pkItem->GetSubType() != ARMOR_SHIELD))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only absorb shoes and shields into that sash."));
					return;
				}
				break;

			case 4:
				if ((pkItem->GetType() != ITEM_ARMOR || pkItem->GetSubType() != ARMOR_BODY) && (pkItem->GetType() != ITEM_WEAPON || (pkItem->GetSubType() >= WEAPON_ARROW
#ifdef __WOLFMAN__
																																	   && pkItem->GetSubType() != WEAPON_CLAW
#endif
																																	   )))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can only absorb weapons and armors into that sash."));
					return;
				}
				break;
#endif//ELONIA
			}
			/*if (pkItem->GetType() != ITEM_WEAPON)
			{
			//ChatInfoCond(test_server, "AcceRefineCheckin Window Type Absorb %d No Weapon", acceSlot);
			return;
			}*/

			/*			if (pkItem->GetType() == ITEM_WEAPON && pkItem->GetSubType() == WEAPON_ARROW)
			return;
			*/
			//if (pkItem->GetType() == ITEM_WEAPON && pkItem->GetSubType() == WEAPON_QUIVER)
			//return;
		}
		else if (acceSlot == ACCE_SLOT_LEFT)
		{
			if (pkItem->GetType() != ITEM_COSTUME)
				return;

			if (pkItem->GetSubType() != COSTUME_ACCE)
				return;

			if (!CItemVnumHelper::IsAcceItem(pkItem->GetVnum()))
			{
				return;
			}

			if (pkItem->GetSocket(1) != 0 && ABSORB == GetAcceWindowType())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_ALREADY_ABSORBED_BONUS"));
				return;
			}

		}

	}
	else if (COMBINE == GetAcceWindowType())
	{
		if (pkItem->GetType() != ITEM_COSTUME && pkItem->GetSubType() != COSTUME_ACCE && pkItem->GetSubType() != COSTUME_ACCE_COSTUME)
			return;
	}



	if (pkItem->GetCell() >= INVENTORY_MAX_NUM && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<Ã¢°í> Ã¢°í·Î ¿Å±æ ¼ö ¾ø´Â ¾ÆÀÌÅÛ ÀÔ´Ï´Ù."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù."));
		return;
	}


	if (COMBINE == GetAcceWindowType())
	{
		if (pkItem->GetType() != ITEM_COSTUME)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_NO_ACCE_ITEM"));
			return;
		}


		if (pkItem->GetSubType() != COSTUME_ACCE && pkItem->GetSubType() != COSTUME_ACCE_COSTUME)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_NO_ACCE_ITEM"));
			return;
		}

		if (!CItemVnumHelper::IsAcceItem(pkItem->GetVnum()) && pkItem->GetSubType() != COSTUME_ACCE_COSTUME)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_NO_ACCE_ITEM"));
			return;
		}


		if (acceSlot == ACCE_SLOT_LEFT && pkItem->GetSocket(0) == ITEM_MAX_ACCEDRAIN)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_ALREADY_MAX"));
			return;
		}

		if (acceSlot == ACCE_SLOT_RIGHT && pkItem->GetSocket(0) == ITEM_MAX_ACCEDRAIN)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_ALREADY_MAX"));
			return;
		}

		LPITEM pkAcceChosen = NULL;
		if (acceSlot == ACCE_SLOT_LEFT)
			pkAcceChosen = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT]));
		else if (acceSlot == ACCE_SLOT_RIGHT)
			pkAcceChosen = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT]));

		DWORD gradeOfChosen = 0;
		DWORD gradeOfNew = pkItem->GetSubType() == COSTUME_ACCE_COSTUME ? pkItem->GetValue(3) : GetAcceRefineGrade(pkItem->GetVnum());
		if (gradeOfNew == 4 && pkItem->GetSubType() == COSTUME_ACCE_COSTUME)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This costume sash is already on maximum grade."));
			return;
		}
		if (pkAcceChosen != NULL)
		{
			if (pkAcceChosen->GetSubType() != pkItem->GetSubType())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't combine a costume sash with a normal sash!"));
				return;
			}

			gradeOfChosen = pkAcceChosen->GetSubType() == COSTUME_ACCE_COSTUME ? pkAcceChosen->GetValue(3) : GetAcceRefineGrade(pkAcceChosen->GetVnum());

			if (gradeOfChosen != 0 && gradeOfNew != 0)
			{
				if (gradeOfNew != gradeOfChosen)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_CANNOT_COMBINE_DIFFERENT_GRADE"));
					return;
				}
			}
			else
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_CANNOT_COMBINE_DIFFERENT_GRADE"));
				return;
			}

		/*	if (pkAcceChosen->GetVnum() != pkItem->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot combine two different acce items."));
				return;
			}*/
		}
	}

	if (m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] != currentCell.cell && m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT] != currentCell.cell) {
		m_pointsInstant.pAcceSlots[acceSlot] = currentCell.cell;
	}
	else {
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_SLOT_ALREADY_IN_USE"));
		return;
	}

	if (ABSORB == GetAcceWindowType())
	{

		if (m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT] != WORD_MAX && m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] != WORD_MAX)
		{
			LPITEM pkAcce = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT]));
			LPITEM pkWeaponToAbsorb = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT]));
			if (!pkAcce || !pkWeaponToAbsorb)
			{
				sys_err("ACCE_SLOT_LEFT or ACCE_SLOT_RIGHT not found");
				return;
			}
			LPITEM pShowItem = ITEM_MANAGER::instance().CreateItem(pkAcce->GetVnum(), 1, 0, false, -1, true);
			if (!pShowItem)
			{
				sys_err("couldnt create acce for absorb preview");
				return;
			}
			pkWeaponToAbsorb->CopyAttributeTo(pShowItem);
			pShowItem->SetSocket(0, pkAcce->GetSocket(0));
			pShowItem->SetSocket(1, pkWeaponToAbsorb->GetVnum());

			network::GCOutputPacket<network::GCItemSetPacket> pack;
			EncodeItemPacket(pShowItem, pack);
			*pack->mutable_data()->mutable_cell() = TItemPos(ACCEREFINE, (WORD)ACCE_SLOT_RESULT);
			GetDesc()->Packet(pack);

			M2_DESTROY_ITEM(pShowItem);
		}

	}

	if (COMBINE == GetAcceWindowType())
	{
		if (m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] != WORD_MAX)
		{
			LPITEM pkAcce = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT]));
			if (!pkAcce)
			{
				sys_err("no ACCE_SLOT_LEFT found");
				return;
			}
			LPITEM pkNewAcce = ITEM_MANAGER::instance().CreateItem(pkAcce->GetRefinedVnum() ? pkAcce->GetRefinedVnum() : pkAcce->GetVnum(), 1, 0, false, -1, true);
			if (!pkNewAcce)
			{
				sys_err("couldnt create acce for refine preview");
				return;
			}

			if (pkAcce->GetSocket(1) > 0) {
				pkNewAcce->SetSocket(1, pkAcce->GetSocket(1));

				if (pkAcce->GetAttributeCount() > 0)
					pkAcce->CopyAttributeTo(pkNewAcce);
			}

			network::GCOutputPacket<network::GCItemSetPacket> pack;
			EncodeItemPacket(pkNewAcce, pack);
			*pack->mutable_data()->mutable_cell() = TItemPos(ACCEREFINE, (WORD)ACCE_SLOT_RESULT);
			GetDesc()->Packet(pack);

			M2_DESTROY_ITEM(pkNewAcce);
		}
	}

	network::GCOutputPacket<network::GCItemSetPacket> pack;
	EncodeItemPacket(pkItem, pack);
	*pack->mutable_data()->mutable_cell() = TItemPos(ACCEREFINE, (WORD)acceSlot);
	GetDesc()->Packet(pack);
}

void CHARACTER::AcceRefineCheckout(BYTE acceSlot)
{

	if (m_pointsInstant.pAcceSlots[acceSlot] == WORD_MAX)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_SLOT_ALREADY_EMPTY"));
		return;
	}

	m_pointsInstant.pAcceSlots[acceSlot] = WORD_MAX;

	LPDESC d = GetDesc();

	struct SPacketGCClearAcceSlot
	{
		WORD cell;

	};

	network::GCOutputPacket<network::GCItemSetPacket> p;
	*p->mutable_data()->mutable_cell() = TItemPos(ACCEREFINE, acceSlot);
	d->Packet(p);
}

void CHARACTER::AcceRefineAccept(int windowType)
{
	bool uppMax = false;
	if (ABSORB == windowType)
	{

		if (m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] == WORD_MAX)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_NO_ACCE_ITEM_TO_ABSORB"));
			return;
		}


		if (m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT] == WORD_MAX)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_NO_ACCE_ITEM_TO_ABSORB_FROM"));
			return;
		}

		LPITEM pAcce = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT]));
		LPITEM pAbsorbItem = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT]));

		if (pAcce && pAbsorbItem)
		{
			if (pAcce->IsEquipped() || pAbsorbItem->IsEquipped())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_CANT_ABSORB_WHILE_EQUIPPED"));
				return;
			}

			if (!CItemVnumHelper::IsAcceItem(pAcce->GetVnum()))
			{
				AcceRefineCheckout(ACCE_SLOT_LEFT);
				return;
			}

			if (pAcce->GetSocket(1) != 0)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_ABSORB_ERROR_ALREADY"));
				return;
			}

			WORD TargetCell = pAcce->GetCell();
			pAcce->RemoveFromCharacter();

			pAbsorbItem->CopyAttributeTo(pAcce);

			// move speed is not allowed; negative boni are not allowed
			for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
			{
				if (pAcce->GetAttributeType(i) == APPLY_MOV_SPEED || pAcce->GetAttributeValue(i) <= 0)
					pAcce->SetForceAttribute(i, APPLY_NONE, 0);
			}

			pAcce->SetSocket(1, pAbsorbItem->GetVnum());
#ifdef __ALPHA_EQUIP__
			pAcce->LoadAlphaEquipValue(pAbsorbItem->GetRealAlphaEquipValue());
#endif

			pAcce->AddToCharacter(this, TItemPos(INVENTORY, TargetCell));
			pAbsorbItem->RemoveFromCharacter();
			ITEM_MANAGER::instance().RemoveItem(pAbsorbItem, "ACCE_ABSORBED");
			AcceRefineCancel();

			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_SUCCESS"));
		}

	}
	else if (COMBINE == windowType)
	{

		if (m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] == WORD_MAX)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_NO_ACCE_ITEM_TO_ABSORB"));
			return;
		}
		if (m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT] == WORD_MAX)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_NO_ACCE_ITEM_TO_ABSORB_FROM"));
			return;
		}

		LPITEM pBaseItem = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT]));
		LPITEM pMaterialItem = GetItem(TItemPos(INVENTORY, m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT]));

		if (pBaseItem == pMaterialItem)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You inserted the same item on both slots."));
			return;
		}

		if (pBaseItem && pMaterialItem)
		{
			if (pBaseItem->IsEquipped() || pMaterialItem->IsEquipped())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't combine while one of the sashes is equipped!"));
				return;
			}

			if (pBaseItem->GetSubType() != COSTUME_ACCE_COSTUME || pMaterialItem->GetSubType() != COSTUME_ACCE_COSTUME)
			{
				if (!CItemVnumHelper::IsAcceItem(pBaseItem->GetVnum()))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_CANNOT_COMBINE_NON_ACCE_ITEMS"));
					return;
				}

				if (!CItemVnumHelper::IsAcceItem(pMaterialItem->GetVnum()))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_CANNOT_COMBINE_NON_ACCE_ITEMS"));
					return;
				}
			}
			
			BYTE baseRefineGrade = pBaseItem->GetSubType() == COSTUME_ACCE_COSTUME ? pBaseItem->GetValue(3) : GetAcceRefineGrade(pBaseItem->GetVnum());
			BYTE materialRefineGrade = pMaterialItem->GetSubType() == COSTUME_ACCE_COSTUME ? pMaterialItem->GetValue(3) : GetAcceRefineGrade(pMaterialItem->GetVnum());
			if (baseRefineGrade != materialRefineGrade)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_CANNOT_COMBINE_DIFFERENT_GRADE"));
				return;
			}

		/*	if (pBaseItem->GetVnum() != pMaterialItem->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot combine two different acce items."));
				return;
			}*/

			DWORD result_vnum = 0;

			if (pBaseItem->GetRefinedVnum() == 0 && pBaseItem->GetSocket(0) < ITEM_MAX_ACCEDRAIN && baseRefineGrade == 4)
			{
/*				if (pBaseItem->GetSocket(0) < ITEM_MIN_ACCEDRAIN) {
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_COMBINE_ABSORB_TO_LOW"));
					return;
				} */
				uppMax = true;
				result_vnum = pBaseItem->GetVnum();
			}
			else {
				result_vnum = pBaseItem->GetRefinedVnum();
			}

			auto prt = CRefineManager::instance().GetRefineRecipe(pBaseItem->GetRefineSet());

			if (!prt)
				return;

			DWORD cost = pBaseItem->GetGold();

			if (GetGold() < static_cast<unsigned long long>(cost))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
				return;
			}

			int prob = random_number(1, 100);

			if (uppMax)
			{
				if (pBaseItem->GetSocket(0) - ITEM_MIN_ACCEDRAIN <= 0) {
					prob += 0;
				}
				else {
					prob -= (pBaseItem->GetSocket(0) - ITEM_MIN_ACCEDRAIN) * 2;
					if (prob <= 0)
						prob = 1;
				}
#ifdef ACCE_COMBINE_CHANGES // if grade is max 100% change of success
				prob = 0;
#endif
			}

			if (prob <= prt->prob())
			{

				LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);
				if (pkNewItem)
				{

					pMaterialItem->RemoveFromCharacter();

					WORD Cell = pBaseItem->GetCell();
					BYTE Window = pBaseItem->GetWindow();

					pBaseItem->CopyAttributeTo(pkNewItem);
					pkNewItem->SetSocket(1, pBaseItem->GetSocket(1));

					if (pBaseItem->IsGMOwner() || pMaterialItem->IsGMOwner())
						pkNewItem->SetGMOwner(true);

					if (!pkNewItem->GetRefinedVnum() && pkNewItem->GetSubType() != COSTUME_ACCE_COSTUME)
					{
						if (uppMax) {
#ifdef ACCE_COMBINE_CHANGES
							int itemDrain = pBaseItem->GetSocket(0) + 1;
							pkNewItem->SetSocket(0, itemDrain);
#else
							int num = random_number(pBaseItem->GetSocket(0) + 1, MIN(pBaseItem->GetSocket(0) + 5, ITEM_MAX_ACCEDRAIN));
							pkNewItem->SetSocket(0, num);//num <= ITEM_MIN_ACCEDRAIN ? ITEM_MIN_ACCEDRAIN : num);
#endif
						}
						else {
							if (GetAcceRefineGrade(result_vnum) == 4)
#ifdef ACCE_COMBINE_CHANGES
								pkNewItem->SetSocket(0, pkNewItem->GetProto()->applies(0).value());
#else
								pkNewItem->SetSocket(0, random_number(pkNewItem->GetProto()->applies(0).value(), ITEM_MAX_ACCEDRAIN));
#endif
							else
								pkNewItem->SetSocket(0, pkNewItem->GetProto()->applies(0).value());
							//random_number(pBaseItem->GetProto()->aApplies[0].lValue + 1, pkNewItem->GetProto()->aApplies[0].lValue - 1));
						}
					}
					ITEM_MANAGER::instance().RemoveItem(pMaterialItem, "ACCE_MAT");
					ITEM_MANAGER::instance().RemoveItem(pBaseItem, "ACCE_BASE");

					pkNewItem->AddToCharacter(this, TItemPos(Window, Cell));
					ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

					LogManager::instance().ItemLog(this, pkNewItem, "ACCE COMBINE SUCCESS", pkNewItem->GetName());

					PayRefineFee(cost);
					AcceRefineCancel();

					EffectPacket(SE_ACCE_SUCESS);

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_SUCCESS"));
				}
				else
				{
					sys_err("cannot create item %u", result_vnum);
					AcceRefineCancel();
				}

			}
			else
			{

				ITEM_MANAGER::instance().RemoveItem(pMaterialItem, "ACCE_FAIL");
				PayRefineFee(cost);
				AcceRefineCancel();

				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ACCE_FAILED"));
			}
		}
	}
}

void CHARACTER::AcceRefineCancel()
{
	m_pointsInstant.pAcceSlots[ACCE_SLOT_RESULT] = WORD_MAX;
	m_pointsInstant.pAcceSlots[ACCE_SLOT_LEFT] = WORD_MAX;
	m_pointsInstant.pAcceSlots[ACCE_SLOT_RIGHT] = WORD_MAX;

	network::GCOutputPacket<network::GCItemSetPacket> p;
	for (int i = 0; i < ACCE_SLOT_MAX_NUM; ++i)
	{
		*p->mutable_data()->mutable_cell() = TItemPos(ACCEREFINE, i);
		GetDesc()->Packet(p);
	}
}

void CHARACTER::AcceClose()
{
	//AcceRefineCancel();
	ChatPacket(CHAT_TYPE_COMMAND, "acce close");
}
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
void CHARACTER::CBT_WindowOpen(LPENTITY pEntity)
{
	LPDESC desc = GetDesc();
	if (!desc)
	{
		sys_err("User(%s)'s DESC is NULL POINTER. WARUM?", GetName());
		return;
	}

	if (NULL == m_pointsInstant.m_pCBTWindowOpener)
		m_pointsInstant.m_pCBTWindowOpener = pEntity;

	m_bIsOpenedCostumeBonusTransferWindow = true;

	desc->Packet(network::TGCHeader::CBT_OPEN);
}

void CHARACTER::CBT_WindowClose()
{
	LPDESC desc = GetDesc();
	if (!desc)
	{
		sys_err("User(%s)'s DESC is NULL POINTER. WARUM?", GetName());
		return;
	}

	m_pointsInstant.m_pCBTWindowOpener = NULL;
	m_bIsOpenedCostumeBonusTransferWindow = false;
	for (BYTE i = 0; i < CBT_SLOT_MAX; i++)
	{
		if (!(m_pCostumeBonusTransferWindowItemCell[i] == NPOS))
		{
			LPITEM pItem = GetItem(m_pCostumeBonusTransferWindowItemCell[i]);
			if (pItem)
				pItem->Lock(false);
		}
		m_pCostumeBonusTransferWindowItemCell[i] = NPOS;
	}

	desc->Packet(network::TGCHeader::CBT_CLOSE);
}

bool CHARACTER::CBT_CanAct()
{
	if ((GetExchange() || IsOpenSafebox() || GetShop()) || !CanHandleItem())
		if (NULL != m_pointsInstant.m_pCBTWindowOpener && m_bIsOpenedCostumeBonusTransferWindow)
			return true;

	return false;
}

void CHARACTER::CBT_CheckIn(BYTE CBTPos, TItemPos ItemCell)
{
	if (IsDead())
		return;

	if (CBTPos < CBT_SLOT_MEDIUM || CBTPos >= CBT_SLOT_RESULT)
		return;

	if (!CBT_CanAct())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You are not able to transfer the bonuses during you are doing something else."));
		return;
	}

	if (!ItemCell.IsValidItemPosition())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You cannot use this item."));
		return;
	}

	LPITEM pItem = GetItem(ItemCell);
	if (!pItem)
		return;

	if (CBTPos == CBT_SLOT_MEDIUM && pItem->GetVnum() != CBT_MEDIUM_ITEM_VNUM)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You need the magical item to transfer the bonuses."));
		return;
	}
	if (CBTPos == CBT_SLOT_MATERIAL || CBTPos == CBT_SLOT_TARGET)
	{
		if (m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MEDIUM] == NPOS)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> First of all put the transfer item into the slot at the top of the window."));
			return;
		}

		if (!(pItem->GetType() == ITEM_COSTUME && (pItem->GetSubType() == COSTUME_BODY || pItem->GetSubType() == COSTUME_HAIR || pItem->GetSubType() == COSTUME_WEAPON || pItem->GetSubType() == COSTUME_ACCE_COSTUME)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You can transfer the bonuses from costumes only."));
			return;
		}

		if (pItem->IsEquipped())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You cannot transfer bonuses of an equipped item."));
			return;
		}

		if (CBTPos == CBT_SLOT_MATERIAL)
		{
			if (pItem->GetAttributeCount() == 0)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> This item has no bonus to transfer."));
				return;
			}
			else if (!(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_TARGET] == NPOS))
			{
				LPITEM pTargetItem = GetItem(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_TARGET]);
				if (!pTargetItem)
				{
					ChatPacket(CHAT_TYPE_INFO, "[CBT] ERROR 0x991A");
					return;
				}

				if (pTargetItem->GetSubType() != pItem->GetSubType())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You cannot attach different type costumes to transferring the bonuses."));
					return;
				}

				if (pTargetItem->GetAttributeCount() > pItem->GetAttributeCount())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> The target item has more bonuses then what you would like to transfer."));
				}
			}
		}
		if (CBTPos == CBT_SLOT_TARGET)
		{
			if (pItem->GetAttributeCount() > 0)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> This item has bonus(es) already, if you continue you gonna lose them."));
			}

			if (!(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MATERIAL] == NPOS))
			{
				LPITEM pMaterialItem = GetItem(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MATERIAL]);
				if (!pMaterialItem)
				{
					ChatPacket(CHAT_TYPE_INFO, "[CBT] ERROR 0x991B");
					return;
				}
				if (pMaterialItem->GetSubType() != pItem->GetSubType())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You cannot attach different type costumes to transferring the bonuses."));
					return;
				}
				if (pItem->GetAttributeCount() > pMaterialItem->GetAttributeCount())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> The target item has more bonuses then what you would like to transfer."));
				}
			}
		}
	}

	if (!(m_pCostumeBonusTransferWindowItemCell[CBTPos] == NPOS))
		return;

	for (BYTE i = 0; i < CBT_SLOT_MAX; i++)
	{
		if (m_pCostumeBonusTransferWindowItemCell[i] == ItemCell)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You cannot attach the same item twice."));
			return;
		}
	}

	pItem->Lock(true);
	m_pCostumeBonusTransferWindowItemCell[CBTPos] = ItemCell;

	network::GCOutputPacket<network::GCCBTItemSetPacket> pack;
	ITEM_MANAGER::Instance().GetPlayerItem(pItem, pack->mutable_data());
	pack->mutable_data()->set_vnum(pItem->GetVnum());
	*pack->mutable_data()->mutable_cell() = ItemCell;
	pack->set_cbt_pos(CBTPos);

	LPDESC desc = GetDesc();
	if (!desc)
	{
		sys_err("User(%s)'s DESC is NULL POINTER.", GetName());
		return;
	}

	desc->Packet(pack);
}

void CHARACTER::CBT_CheckOut(BYTE CBTPos)
{
	if (CBTPos < CBT_SLOT_MEDIUM || CBTPos >= CBT_SLOT_RESULT)
		return;

	if (!CBT_CanAct())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You are not able to transfer the bonuses during you are doing something else."));
		return;
	}

	if (m_pCostumeBonusTransferWindowItemCell[CBTPos] == NPOS)
		return;

	LPITEM pItem = GetItem(m_pCostumeBonusTransferWindowItemCell[CBTPos]);
	if (!pItem)
		return;

	if (CBTPos == CBT_SLOT_MEDIUM)
	{
		for (BYTE i = 0; i < CBT_SLOT_MAX; i++)
		{
			if (!(m_pCostumeBonusTransferWindowItemCell[i] == NPOS))
			{
				LPITEM pItem = GetItem(m_pCostumeBonusTransferWindowItemCell[i]);
				if (pItem)
					pItem->Lock(false);
			}
			m_pCostumeBonusTransferWindowItemCell[i] = NPOS;
		}

		LPDESC desc = GetDesc();
		if (!desc)
		{
			sys_err("User(%s)'s DESC is NULL POINTER. WARUM?", GetName());
			return;
		}

		desc->Packet(network::TGCHeader::CBT_CLEAR);

	}
	else
	{
		pItem->Lock(false);
		m_pCostumeBonusTransferWindowItemCell[CBTPos] = NPOS;

		network::GCOutputPacket<network::GCCBTItemSetPacket> pack;
		pack->set_cbt_pos(CBTPos);

		LPDESC desc = GetDesc();
		if (!desc)
		{
			sys_err("User(%s)'s DESC is NULL POINT.", GetName());
			return;
		}

		desc->Packet(pack);
	}
}

void CHARACTER::CBT_Accept()
{
	if (!CBT_CanAct())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You are not able to transfer the bonuses during you are doing something else."));
		return;
	}
	if (m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MEDIUM] == NPOS || m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MATERIAL] == NPOS || m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_TARGET] == NPOS)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> Pull the items into the window."));
		return;
	}

	LPITEM pMediumItem = GetItem(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MEDIUM]);
	if (!pMediumItem || !IsValidItemPosition(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MEDIUM]) || pMediumItem->IsExchanging() || pMediumItem->IsEquipped())
		return;

	if (pMediumItem->GetVnum() != CBT_MEDIUM_ITEM_VNUM)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You will need an item: %s"), ITEM_MANAGER::instance().GetTable(CBT_MEDIUM_ITEM_VNUM)->locale_name(GetLanguageID()).c_str());
		return;
	}

	LPITEM pMaterialItem = GetItem(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MATERIAL]);
	if (!pMaterialItem || !IsValidItemPosition(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_MATERIAL]) || pMaterialItem->IsExchanging() || pMaterialItem->IsEquipped())
		return;

	if (!(pMaterialItem->GetType() == ITEM_COSTUME && (pMaterialItem->GetSubType() == COSTUME_BODY || pMaterialItem->GetSubType() == COSTUME_HAIR || pMaterialItem->GetSubType() == COSTUME_WEAPON || pMaterialItem->GetSubType() == COSTUME_ACCE_COSTUME)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You can transfer the bonuses from costumes only."));
		return;
	}

	LPITEM pTargetItem = GetItem(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_TARGET]);
	if (!pTargetItem || !IsValidItemPosition(m_pCostumeBonusTransferWindowItemCell[CBT_SLOT_TARGET]) || pTargetItem->IsExchanging() || pTargetItem->IsEquipped())
		return;

	if (!(pTargetItem->GetType() == ITEM_COSTUME && (pTargetItem->GetSubType() == COSTUME_BODY || pTargetItem->GetSubType() == COSTUME_HAIR || pTargetItem->GetSubType() == COSTUME_WEAPON || pTargetItem->GetSubType() == COSTUME_ACCE_COSTUME)))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You can transfer the bonuses from costumes only."));
		return;
	}

	if (pMaterialItem->GetSubType() == COSTUME_ACCE_COSTUME && pTargetItem->GetSubType() == COSTUME_ACCE_COSTUME
		&& pMaterialItem->GetValue(3) != pTargetItem->GetValue(3)
	)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> Sashes must have the same level."));
		return;
	}

	if (pMaterialItem->GetSubType() != pTargetItem->GetSubType())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You cannot transfer bonuses from different type of costumes."));
		return;
	}

	if (IS_SET(pMediumItem->GetFlag(), ITEM_FLAG_STACKABLE) && !IS_SET(pMediumItem->GetAntiFlag(), ITEM_ANTIFLAG_STACK) && pMediumItem->GetCount() > 1)
		pMediumItem->SetCount(pMediumItem->GetCount() - 1);
	else
		ITEM_MANAGER::instance().RemoveItem(pMediumItem, "[CBT] MEDIUM ITEM REMOVE");

	pTargetItem->ClearAttribute();
	pMaterialItem->CopyAttributeTo(pTargetItem);
	pMaterialItem->ClearAttribute();
	pMaterialItem->Save();
	pMaterialItem->UpdatePacket();
	pTargetItem->UpdatePacket();

	for (BYTE i = 0; i < CBT_SLOT_MAX; i++)
	{
		if (!(m_pCostumeBonusTransferWindowItemCell[i] == NPOS))
		{
			LPITEM pItem = GetItem(m_pCostumeBonusTransferWindowItemCell[i]);
			if (pItem)
				pItem->Lock(false);
		}
		m_pCostumeBonusTransferWindowItemCell[i] = NPOS;
	}

	LPDESC desc = GetDesc();
	if (!desc)
	{
		sys_err("User(%s)'s DESC is NULL POINTER. WARUM?", GetName());
		return;
	}

	desc->Packet(network::TGCHeader::CBT_CLEAR);

	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<CBT> You have successfully transferred the bonuses."));
}
#endif
