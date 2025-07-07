#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "desc.h"
#include "sectree_manager.h"
#include "packet.h"
#include "protocol.h"
#include "log.h"
#include "skill.h"
#include "unique_item.h"
#include "profiler.h"
#include "marriage.h"
#include "item_addon.h"
#include "dev_log.h"
#include "item.h"
#include "item_manager.h"
#include "affect.h"
#include "buff_on_attributes.h"
#include "../../common/VnumHelper.h"
#include "shop.h"

#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

#include "questmanager.h"

#include "safebox.h"
#include "mob_manager.h"

CItem::CItem(DWORD dwVnum)
	: m_dwVnum(dwVnum), m_bWindow(0), m_dwID(0), m_bEquipped(false), m_dwVID(0), m_wCell(0), m_dwCount(0), m_lFlag(0), m_dwLastOwnerPID(0),
	m_bExchanging(false), m_pkDestroyEvent(NULL), m_pkUniqueExpireEvent(NULL), m_pkTimerBasedOnWearExpireEvent(NULL), m_pkRealTimeExpireEvent(NULL),
	m_pkExpireEvent(NULL),
   	m_pkAccessorySocketExpireEvent(NULL), m_pkOwnershipEvent(NULL), m_dwOwnershipPID(0), m_bSkipSave(false), m_isLocked(false),
	m_dwMaskVnum(0), m_dwSIGVnum(0), m_bIsGMOwner(false), m_dwRealOwnerPID(0), m_iData(0), m_bIsDisabledItem(false)
#ifdef __ALPHA_EQUIP__
	, m_iAlphaEquipValue(0)
#endif
#ifdef __PET_ADVANCED__
	, m_petAdvanced(NULL)
#endif
#ifdef CRYSTAL_SYSTEM
	, _crystal_timeout_event(nullptr), _crystal_event_char(nullptr), _crystal_event_handle(0), _crystal_last_use_time(0)
#endif
	// , m_dwCooltime(0)
{
	memset( m_alSockets, 0, sizeof(m_alSockets) );
	memset( m_aAttr, 0, sizeof(m_aAttr) );
}

CItem::~CItem()
{
	Destroy();
}

void CItem::Initialize()
{
	CEntity::Initialize(ENTITY_ITEM);

	m_bWindow = RESERVED_WINDOW;
	m_pOwner = NULL;
	m_dwID = 0;
	m_bEquipped = false;
	m_dwVID = m_wCell = m_dwCount = m_lFlag = 0;
	m_pProto = NULL;
	m_bExchanging = false;
	memset(&m_alSockets, 0, sizeof(m_alSockets));
	memset(&m_aAttr, 0, sizeof(m_aAttr));

	m_pkDestroyEvent = NULL;
	m_pkOwnershipEvent = NULL;
	m_dwOwnershipPID = 0;
	m_pkUniqueExpireEvent = NULL;
	m_pkTimerBasedOnWearExpireEvent = NULL;
	m_pkRealTimeExpireEvent = NULL;

	m_pkAccessorySocketExpireEvent = NULL;

	m_bSkipSave = false;
	m_dwLastOwnerPID = 0;

	m_bIsGMOwner = false;
	m_iData = 0;

#ifdef __ALPHA_EQUIP__
	m_iAlphaEquipValue = 0;
#endif
	// m_dwCooltime = 0;
}

void CItem::Destroy()
{
	event_cancel(&m_pkDestroyEvent);
	event_cancel(&m_pkOwnershipEvent);
	event_cancel(&m_pkUniqueExpireEvent);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);
	event_cancel(&m_pkRealTimeExpireEvent);
	event_cancel(&m_pkAccessorySocketExpireEvent);
#ifdef CRYSTAL_SYSTEM
	event_cancel(&_crystal_timeout_event);
	clear_crystal_char_events();
#endif

	if (quest::CQuestManager::instance().GetCurrentItem() == this)
		quest::CQuestManager::instance().SetCurrentItem(NULL);

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

#ifdef __PET_ADVANCED__
	if (m_petAdvanced)
	{
		M2_DELETE(m_petAdvanced);
		m_petAdvanced = NULL;
	}
#endif
}

EVENTFUNC(item_destroy_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "item_destroy_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	if (pkItem->GetOwner())
		sys_err("item_destroy_event: Owner exist. (item %s owner %s)", pkItem->GetName(), pkItem->GetOwner()->GetName());

	if (pkItem->GetType() != ITEM_ELK && (pkItem->GetOwner() || pkItem->GetLastOwnerPID()))
	{
		if (test_server)
			sys_log(0, "item_destroy_event [info %p item %p]", info, pkItem);

		DWORD dwRealOwnerPID = pkItem->GetRealOwnerPID();
		pkItem->SetRealOwnerPID(pkItem->GetLastOwnerPID());
		LogManager::instance().ItemDestroyLog(LogManager::ITEM_DESTROY_GROUND_REMOVE, pkItem);
		pkItem->SetRealOwnerPID(dwRealOwnerPID);
	}

	pkItem->SetDestroyEvent(NULL);
	M2_DESTROY_ITEM(pkItem);
	return 0;
}

void CItem::SetDestroyEvent(LPEVENT pkEvent)
{
	m_pkDestroyEvent = pkEvent;
}

void CItem::StartDestroyEvent(int iSec)
{
	if (m_pkDestroyEvent)
		return;

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;
	if (test_server)
		sys_log(0, "StartDestroyEvent[info %p item %p %u %s]", info, info->item, GetID(), GetName());

	SetDestroyEvent(event_create(item_destroy_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::EncodeInsertPacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	const PIXEL_POSITION & c_pos = GetXYZ();

	network::GCOutputPacket<network::GCItemGroundAddPacket> pack;

	pack->set_x(c_pos.x);
	pack->set_y(c_pos.y);
	pack->set_z(c_pos.z);
	pack->set_vnum(GetVnum());
	pack->set_vid(m_dwVID);
#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	for (size_t i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		pack->add_sockets(GetSocket(i));

	for (size_t i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		*pack->add_attributes() = GetAttribute(i);
#endif

	d->Packet(pack);

	if (m_pkOwnershipEvent != NULL)
	{
		item_event_info * info = dynamic_cast<item_event_info *>(m_pkOwnershipEvent->info);

		if ( info == NULL )
		{
			sys_err( "CItem::EncodeInsertPacket> <Factor> Null pointer" );
			return;
		}

		network::GCOutputPacket<network::GCItemOwnershipPacket> p;

		p->set_vid(m_dwVID);
		p->set_name(info->szOwnerName);

		d->Packet(p);
	}
}

void CItem::EncodeRemovePacket(LPENTITY ent)
{
	LPDESC d;

	if (!(d = ent->GetDesc()))
		return;

	network::GCOutputPacket<network::GCItemGroundDeletePacket> pack;
	pack->set_vid(m_dwVID);
	d->Packet(pack);
	sys_log(2, "Item::EncodeRemovePacket %s to %s", GetName(), ((LPCHARACTER) ent)->GetName());
}

void CItem::SetProto(const network::TItemTable * table)
{
	assert(table != NULL);
	m_pProto = table;
	SetFlag(m_pProto->flags());

#ifdef __PET_ADVANCED__
	if (IsAdvancedPet())
	{
		if (!m_petAdvanced)
			m_petAdvanced = M2_NEW CPetAdvanced(this);
	}
	else
	{
		if (m_petAdvanced)
		{
			M2_DELETE(m_petAdvanced);
			m_petAdvanced = NULL;
		}
	}
#endif
}

void CItem::RemoveFlag(long bit)
{
	REMOVE_BIT(m_lFlag, bit);
}

void CItem::AddFlag(long bit)
{
	SET_BIT(m_lFlag, bit);
}

void CItem::UpdatePacket()
{
	if (!m_pOwner || !m_pOwner->GetDesc())
		return;

	network::GCOutputPacket<network::GCItemUpdatePacket> pack;
	ITEM_MANAGER::Instance().GetPlayerItem(this, pack->mutable_data());

	sys_log(2, "UpdatePacket %s -> %s", GetName(), m_pOwner->GetName());
	m_pOwner->GetDesc()->Packet(pack);
}

DWORD CItem::GetCount()
{
	if (GetType() == ITEM_ELK) return MIN(m_dwCount, INT_MAX);
	else
	{
		return MIN(m_dwCount, ITEM_MAX_COUNT);
	}
}

bool CItem::SetCount(DWORD count)
{
	if (GetType() == ITEM_ELK)
	{
		m_dwCount = MIN(count, INT_MAX);
	}
	else
	{
		m_dwCount = MIN(count, ITEM_MAX_COUNT);
	}

	if (count == 0 && m_pOwner)
	{
		//if (GetSubType() == USE_ABILITY_UP || GetSubType() == USE_POTION || GetVnum() == 70020)
		{
			LPCHARACTER pOwner = GetOwner();
			WORD wCell = GetCell();

			RemoveFromCharacter();

#ifdef __DRAGONSOUL__
			if (!IsDragonSoul())
#endif
			{
				LPITEM pItem = pOwner->FindSpecifyItem(GetVnum());

				if (NULL != pItem)
				{
					pOwner->ChainQuickslotItem(pItem, QUICKSLOT_TYPE_ITEM, wCell);
				}
				else
				{
					pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, wCell, 255);
				}
			}

			M2_DESTROY_ITEM(this);
		}
		/*else
		{
#ifdef __DRAGONSOUL__
			if (!IsDragonSoul())
#endif
				m_pOwner->SyncQuickslot(QUICKSLOT_TYPE_ITEM, m_wCell, 255);

			M2_DESTROY_ITEM(RemoveFromCharacter());
		}*/

		return false;
	}

	UpdatePacket();

	Save();
	return true;
}

LPITEM CItem::RemoveFromCharacter()
{
	if (!m_pOwner)
	{
		sys_err("Item::RemoveFromCharacter owner null");
		return (this);
	}

	LPCHARACTER pOwner = m_pOwner;

	if (pOwner->GetMyShop())
	{
		int iItemPos = pOwner->GetMyShop()->GetPosByItemID(GetID());
		if (iItemPos != -1)
			pOwner->GetMyShop()->RemoveFromShop(iItemPos);
	}

#ifdef __PET_ADVANCED__
	if (m_petAdvanced)
		m_petAdvanced->Unsummon();
#endif

#ifdef CRYSTAL_SYSTEM
	if (GetType() == ITEM_CRYSTAL && static_cast<ECrystalItem>(GetSubType()) == ECrystalItem::CRYSTAL)
		stop_crystal_use();
#endif

	if (m_bEquipped)	// ÀåÂøµÇ¾ú´Â°¡?
	{
		Unequip();
		//pOwner->UpdatePacket();

#ifdef ENABLE_DS_SET_BONUS
		if (IsDragonSoul() && pOwner)
		{
			pOwner->ComputePoints();
			pOwner->PointsPacket();
		}
#endif

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
	else
	{
		if (GetWindow() != SAFEBOX && GetWindow() != MALL)
		{
#ifdef __DRAGONSOUL__
			if (IsDragonSoul())
			{
				if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
					sys_err("CItem::RemoveFromCharacter: pos >= DRAGON_SOUL_INVENTORY_MAX_NUM");
				else
					pOwner->SetItem(TItemPos(m_bWindow, m_wCell), NULL);
			}
			else
#endif
			{
				TItemPos cell(INVENTORY, m_wCell);

				if (false == cell.IsDefaultInventoryPosition() && false == ITEM_MANAGER::instance().IsNewWindow(GetWindow())) // ¾Æ´Ï¸é ¼ÒÁöÇ°¿¡?
					sys_err("CItem::RemoveFromCharacter: Invalid Item Position (window %d cell %d)", GetWindow(), GetCell());
				else
				{
					pOwner->SetItem(cell, NULL);
				}
			}
		}

		m_pOwner = NULL;
		m_wCell = 0;

		SetWindow(RESERVED_WINDOW);
		Save();
		return (this);
	}
}

bool CItem::AddToCharacter(LPCHARACTER ch, TItemPos Cell)
{
	assert(GetSectree() == NULL);
	assert(m_pOwner == NULL);
	WORD pos = Cell.cell;
	BYTE window_type = Cell.window_type;
	
	if (INVENTORY == window_type)
	{
		if (m_wCell >= ch->GetInventoryMaxNum())
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d maxCell %d", m_pProto->name().c_str(), ch->GetName(), m_wCell, ch->GetInventoryMaxNum());
			return false;
		}
	}
	else if (window_type == UPPITEM_INVENTORY && m_wCell >= ch->GetUppitemInventoryMaxNum() + UPPITEM_INV_SLOT_START)
	{
		sys_err("CItem::AddToCharacter: uppitem cell overflow: %s to %s cell %d maxCell %d", m_pProto->name().c_str(), ch->GetName(), m_wCell, ch->GetUppitemInventoryMaxNum());
		return false;
	}

	// IDK if correct..., cuz was no case before in here...
	else if (window_type == SKILLBOOK_INVENTORY && m_wCell >= ch->GetSkillbookInventoryMaxNum() + SKILLBOOK_INV_SLOT_START)
	{
		sys_err("CItem::AddToCharacter: Skillbook cell overflow: %s to %s cell %d maxCell %d", m_pProto->name().c_str(), ch->GetName(), m_wCell, ch->GetSkillbookInventoryMaxNum());
		return false;
	}
	else if (window_type == STONE_INVENTORY && m_wCell >= ch->GetStoneInventoryMaxNum() + STONE_INV_SLOT_START)
	{
		sys_err("CItem::AddToCharacter: Stone cell overflow: %s to %s cell %d maxCell %d", m_pProto->name().c_str(), ch->GetName(), m_wCell, ch->GetStoneInventoryMaxNum());
		return false;
	}
	else if (window_type == ENCHANT_INVENTORY && m_wCell >= ch->GetEnchantInventoryMaxNum() + ENCHANT_INV_SLOT_START)
	{
		sys_err("CItem::AddToCharacter: Enchant cell overflow: %s to %s cell %d maxCell %d", m_pProto->name().c_str(), ch->GetName(), m_wCell, ch->GetEnchantInventoryMaxNum());
		return false;
	}
#ifdef __DRAGONSOUL__
	else if (DRAGON_SOUL_INVENTORY == window_type)
	{
		if (m_wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CItem::AddToCharacter: cell overflow: %s to %s cell %d", m_pProto->name().c_str(), ch->GetName(), m_wCell);
			return false;
		}
	}
#endif

#ifdef __MARK_NEW_ITEM_SYSTEM__
	bool bWereMine = this->GetLastOwnerPID() == ch->GetPlayerID();
#endif

	if (ch->GetDesc())
		m_dwLastOwnerPID = ch->GetPlayerID();

	event_cancel(&m_pkDestroyEvent);

#ifndef __MARK_NEW_ITEM_SYSTEM__
	ch->SetItem(TItemPos(window_type, pos), this);
#else
	ch->SetItem(TItemPos(window_type, pos), this, bWereMine);
#endif
	m_pOwner = ch;
	m_dwRealOwnerPID = 0;

	if (m_bIsGMOwner == GM_OWNER_UNSET)
		m_bIsGMOwner = ch->IsGM() ? GM_OWNER_GM : GM_OWNER_PLAYER;

	Save();
	return true;
}

bool CItem::CanStackWith(LPITEM pOtherItem, bool bCheckSocket)
{
	if (!IsStackable() || IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		return false;

	if (GetVnum() != pOtherItem->GetVnum())
		return false;

	if (IsGMOwner() != pOtherItem->IsGMOwner() && !test_server)
		return false;

	if (bCheckSocket)
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (GetSocket(i) != pOtherItem->GetSocket(i))
				return false;
		}
	}

	return true;
}

LPITEM CItem::RemoveFromGround()
{
	if (GetSectree())
	{
		SetOwnership(NULL);
		
		GetSectree()->RemoveEntity(this);
		
		ViewCleanup();

		Save();
	}

	return (this);
}

bool CItem::AddToGround(long lMapIndex, const PIXEL_POSITION & pos, bool skipOwnerCheck)
{
	if (0 == lMapIndex)
	{
		sys_err("wrong map index argument: %d", lMapIndex);
		return false;
	}

	if (GetSectree())
	{
		sys_err("sectree already assigned");
		return false;
	}

	if (!skipOwnerCheck && m_pOwner)
	{
		sys_err("owner pointer not null");
		return false;
	}

	LPSECTREE tree = SECTREE_MANAGER::instance().Get(lMapIndex, pos.x, pos.y);

	if (!tree)
	{
		sys_err("cannot find sectree by %dx%d", pos.x, pos.y);
		return false;
	}

	//tree->Touch();

	SetWindow(GROUND);
	SetXYZ(pos.x, pos.y, pos.z);
	tree->InsertEntity(this);
	UpdateSectree();
	Save();
	return true;
}

bool CItem::DistanceValid(LPCHARACTER ch)
{
	if (!GetSectree())
		return false;

	int iDist = DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY());

	if (iDist > 500*1.5)
		return false;

	return true;
}

int CItem::GetScrollRefineType() const
{
	if (GetType() != ITEM_USE || GetSubType() != USE_TUNING)
		return -1;

	switch (GetValue(0))
	{
	case CHUKBOK_SCROLL:
	case YONGSIN_SCROLL:
	case YAGONG_SCROLL:
	case MEMO_SCROLL:
	case STONE_LV5_SCROLL:
	case YAGONG_SCROLL_KEEPLEVEL:
		return REFINE_TYPE_SCROLL;
		break;

	case HYUNIRON_CHN:
		return REFINE_TYPE_HYUNIRON;
		break;

	case MUSIN_SCROLL:
		return REFINE_TYPE_MUSIN;
		break;

	case BDRAGON_SCROLL:
		return REFINE_TYPE_BDRAGON;
		break;
	}

	return -1;
}

bool CItem::CanUsedBy(LPCHARACTER ch)
{
	// Anti flag check
	switch (ch->GetJob())
	{
		case JOB_WARRIOR:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;

#ifdef __WOLFMAN__
		case JOB_WOLFMAN:
			if (GetAntiFlag() & ITEM_ANTIFLAG_WOLFMAN)
				return false;
#endif
	}

	return true;
}

int CItem::FindEquipCell(LPCHARACTER ch, int iCandidateCell)
{
	bool bCanWearExtra = false;
#ifdef __BELT_SYSTEM__
	if (GetType() == ITEM_BELT)
		bCanWearExtra = true;
#endif
#ifdef __DRAGONSOUL__
	if (GetType() == ITEM_DS || GetType() == ITEM_SPECIAL_DS)
		bCanWearExtra = true;
#endif
	if (GetType() == ITEM_SHINING)
		bCanWearExtra = true;

	if (0 == GetWearFlag() && ITEM_COSTUME != GetType() && ITEM_RING != GetType() && !bCanWearExtra)
		return -1;

#ifdef __DRAGONSOUL__
	if (GetType() == ITEM_DS || GetType() == ITEM_SPECIAL_DS)
	{
		if (iCandidateCell < 0)
		{
			return WEAR_MAX_NUM + GetSubType();
		}
		else
		{
			for (int i = 0; i < DRAGON_SOUL_DECK_MAX_NUM; i++)
			{
				if (WEAR_MAX_NUM + i * DS_SLOT_MAX + GetSubType() == iCandidateCell)
				{
					return iCandidateCell;
				}
			}
			return -1;
		}
	}
#endif
	
	if (GetType() == ITEM_COSTUME)
	{
#ifdef __SKIN_SYSTEM__
		if(GetSubType() == COSTUME_BODY || GetSubType() == COSTUME_WEAPON || GetSubType() == COSTUME_HAIR)
		{
			auto CanBeUsedByPlayerSex = [ & ](LPCHARACTER ch) -> bool
			{
				if(IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_MALE))
					if(SEX_MALE == GET_SEX(ch))
						return false;

				if(IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_FEMALE))
					if(SEX_FEMALE == GET_SEX(ch))
						return false;

				return true;
			};

			BYTE normalCell = 0;
			BYTE buffiCell = 0;

			if(GetSubType() == COSTUME_BODY)
			{
				normalCell = WEAR_COSTUME_BODY;
				buffiCell = SKINSYSTEM_SLOT_BUFFI_BODY;
			}
			else if (GetSubType() == COSTUME_HAIR)
			{
				normalCell = WEAR_COSTUME_HAIR;
				buffiCell = SKINSYSTEM_SLOT_BUFFI_HAIR;
			}
			else
			{
				normalCell = WEAR_COSTUME_WEAPON;
				buffiCell = SKINSYSTEM_SLOT_BUFFI_WEAPON;
			}

			LPITEM normalCostume = ch->GetWear(normalCell);
			LPITEM buffiCostume = ch->GetWear(buffiCell);

			bool bIsBuffiCostume = !IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_SHAMAN);
			if (ch->GetQuestFlag("fake_buff.gender"))
				bIsBuffiCostume = bIsBuffiCostume && (!IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_MALE));
			else
				bIsBuffiCostume = bIsBuffiCostume && (!IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_FEMALE));
			bool bCanWear = ( CanUsedBy(ch) && CanBeUsedByPlayerSex(ch) );

			if(bIsBuffiCostume)
			{
				ch->tchat("bCanWear %d normalCostume %d normalCell %d iCandidateCell %d buffiCell %d buffiCostume %d", bCanWear, normalCostume, normalCell, iCandidateCell, buffiCell, buffiCostume);
				// If used by shaman female
				if(bCanWear)
				{
					if(normalCostume && buffiCostume)
					{
						if(iCandidateCell == buffiCell)
							return buffiCell;

						return normalCell;
					}
					
					if(normalCostume && !buffiCostume)
					{
						if(iCandidateCell == normalCell)
							return normalCell;

						return buffiCell;
					}

					if(!normalCostume && buffiCostume)
					{
						if(iCandidateCell == buffiCell)
							return buffiCell;

						return normalCell;
					}
				}
				else
					return buffiCell;
			}

			return normalCell;
		}
#else
		if(GetSubType() == COSTUME_BODY)
			return WEAR_COSTUME_BODY;
		else if(GetSubType() == COSTUME_WEAPON)
			return WEAR_COSTUME_WEAPON;
#endif
		else if(GetSubType() == COSTUME_HAIR)
			return WEAR_COSTUME_HAIR;
#ifdef __ACCE_COSTUME__
		else if(GetSubType() == COSTUME_ACCE)
			return WEAR_ACCE;
#endif
		else if(GetSubType() == COSTUME_ACCE_COSTUME)
			return WEAR_COSTUME_ACCE;

#ifdef __SKIN_SYSTEM__
		else if(GetSubType() == COSTUME_PET)
			return SKINSYSTEM_SLOT_PET;

		else if(GetSubType() == COSTUME_MOUNT)
			return SKINSYSTEM_SLOT_MOUNT;
#endif
	}
	else if (GetType() == ITEM_RING)
	{
		if (ch->GetWear(WEAR_RING1))
			return WEAR_RING2;
		else
			return WEAR_RING1;
	}
	else if (GetType() == ITEM_SHINING)
	{
		switch (GetSubType())
		{
			case SHINING_BODY:
				if (iCandidateCell < 0)
				{
					for (BYTE i = 0; i < SHINING_BODY_MAX; ++i)
						if (!ch->GetWear(i + WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM))
							return i + WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM;
				}
				else if (WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM <= iCandidateCell &&
					WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_BODY_MAX > iCandidateCell)
					return iCandidateCell;

				return -1;

			case SHINING_WEAPON:
				if (iCandidateCell < 0)
				{
					for (BYTE i = SHINING_BODY_MAX; i < SHINING_MAX_NUM; ++i)
						if (!ch->GetWear(i + WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM))
							return i + WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM;
				}
				else if (WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_BODY_MAX <= iCandidateCell &&
					WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_MAX_NUM > iCandidateCell)
					return iCandidateCell;

				return -1;
		}
	}
#ifdef __BELT_SYSTEM__
	else if (GetType() == ITEM_BELT)
	{
		return WEAR_BELT;
	}
#endif
	else if (GetWearFlag() & WEARABLE_BODY)
		return WEAR_BODY;
	else if (GetWearFlag() & WEARABLE_HEAD)
		return WEAR_HEAD;
	else if (GetWearFlag() & WEARABLE_FOOTS)
		return WEAR_FOOTS;
	else if (GetWearFlag() & WEARABLE_WRIST)
		return WEAR_WRIST;
	else if (GetWearFlag() & WEARABLE_WEAPON)
		return WEAR_WEAPON;
	else if (GetWearFlag() & WEARABLE_SHIELD)
		return WEAR_SHIELD;
	else if (GetWearFlag() & WEARABLE_NECK)
		return WEAR_NECK;
	else if (GetWearFlag() & WEARABLE_EAR)
		return WEAR_EAR;
	else if (GetWearFlag() & WEARABLE_ARROW)
		return WEAR_ARROW;
	else if (GetWearFlag() & WEARABLE_TOTEM)
		return WEAR_TOTEM;
	else if (GetWearFlag() & WEARABLE_UNIQUE)
	{
		if (ch->GetWear(WEAR_UNIQUE1))
			return WEAR_UNIQUE2;
		else
			return WEAR_UNIQUE1;		
	}
	else if (GetWearFlag() && WEARABLE_COSTUME_UNIQUE)
	{
		if (ch->GetWear(WEAR_UNIQUE3))
			return WEAR_UNIQUE4;
		else
			return WEAR_UNIQUE3;
	}

	// ¼öÁý Äù½ºÆ®¸¦ À§ÇÑ ¾ÆÀÌÅÛÀÌ ¹ÚÈ÷´Â°÷À¸·Î ÇÑ¹ø ¹ÚÈ÷¸é Àý´ë –E¼ö ¾ø´Ù.
	else if (GetWearFlag() & WEARABLE_ABILITY)
	{
		if (!ch->GetWear(WEAR_ABILITY1))
		{
			return WEAR_ABILITY1;
		}
		else if (!ch->GetWear(WEAR_ABILITY2))
		{
			return WEAR_ABILITY2;
		}
		else if (!ch->GetWear(WEAR_ABILITY3))
		{
			return WEAR_ABILITY3;
		}
		else if (!ch->GetWear(WEAR_ABILITY4))
		{
			return WEAR_ABILITY4;
		}
		else if (!ch->GetWear(WEAR_ABILITY5))
		{
			return WEAR_ABILITY5;
		}
		else if (!ch->GetWear(WEAR_ABILITY6))
		{
			return WEAR_ABILITY6;
		}
		else if (!ch->GetWear(WEAR_ABILITY7))
		{
			return WEAR_ABILITY7;
		}
		else if (!ch->GetWear(WEAR_ABILITY8))
		{
			return WEAR_ABILITY8;
		}
		else
		{
			return -1;
		}
	}
	return -1;
}

void CItem::ModifyPoints(bool bAdd, LPCHARACTER pkChr, float fFactor)
{
	if (!pkChr)
		pkChr = m_pOwner;

	if (!pkChr)
	{
		sys_err("modify_points no owner");
		return;
	}

#ifdef __PET_ADVANCED__
	if (GetAdvancedPet())
	{
		GetAdvancedPet()->ApplyBuff(bAdd);
		return;
}
#endif

	//pkChr->tchat("ModifyPoints %d", bAdd);

#ifdef __HEAVENSTONE__
	if (GetType() == ITEM_HEAVENSTONE && !pkChr->IsHeavenstoneActive())
		return;
#endif

	// if (test_server)
		// pkChr->ChatPacket(CHAT_TYPE_INFO, "ModifyPoints[%s|%d] add %d fFactor %f", GetName(pkChr->GetLanguageID()), GetID(), bAdd, fFactor);

	int accessoryGrade;
	BYTE socketType = 0;

	if (false == IsAccessoryForSocket())
	{
		if ((m_pProto->type() == ITEM_WEAPON && m_pProto->sub_type() != WEAPON_QUIVER) || m_pProto->type() == ITEM_ARMOR)
		{
			// ¼ÒÄÏÀÌ ¼Ó¼º°­È­¿¡ »ç¿ëµÇ´Â °æ¿ì Àû¿ëÇÏÁö ¾Ê´Â´Ù (ARMOR_WRIST ARMOR_NECK ARMOR_EAR)
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				if (i == 0 && IsRealTimeItem())
					continue;

				DWORD dwVnum;

				if ((dwVnum = GetSocket(i)) <= 2)
					continue;

				auto p = ITEM_MANAGER::instance().GetTable(dwVnum);

				if (!p)
				{
					sys_err("cannot find table by vnum %u", dwVnum);
					continue;
				}

				if (ITEM_METIN == p->type())
				{
					//pkChr->ApplyPoint(p->alValues[0], bAdd ? p->alValues[1] : -p->alValues[1]);
					for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
					{
						if (p->applies(i).type() == APPLY_NONE)
							continue;

						if (p->applies(i).type() == APPLY_SKILL)
							pkChr->ApplyPoint(p->applies(i).type(), bAdd ? p->applies(i).value() : p->applies(i).value() ^ 0x00800000);
						else
						{
							long lValue = p->applies(i).value() * fFactor;

							pkChr->ApplyPoint(p->applies(i).type(), bAdd ? lValue : -lValue);
						}
					}
				}
			}
		}

		accessoryGrade = 0;
	}
	else
	{
		accessoryGrade = MIN(GetAccessorySocketGradeTotal(), ITEM_ACCESSORY_SOCKET_MAX_NUM);
		socketType = GetAccessorySocketType();
		if (!GetAccessorySocketByType(socketType, false))
			socketType = 0;
	}

	if (GetType() == ITEM_BELT && GetAccessorySocketGrade(true))
	{
		auto jewTbl = ITEM_MANAGER::Instance().GetTable(GetAccessorySocketByType(GetAccessorySocketType(), true));
		if (jewTbl)
			for (BYTE i = 0; i < GetAccessorySocketGrade(true); ++i)
				if (jewTbl->applies(i).type())
					pkChr->ApplyPoint(jewTbl->applies(i).type(), bAdd ? jewTbl->applies(i).value() : -jewTbl->applies(i).value());
	}
	else if (socketType && accessoryGrade)
	{
		auto jewTbl =  ITEM_MANAGER::Instance().GetTable(GetAccessorySocketByType(socketType, false));
		if (jewTbl && GetAccessorySocketGrade(false) > 0 && jewTbl->applies(0).type())
			pkChr->ApplyPoint(jewTbl->applies(0).type(), bAdd ? (jewTbl->applies(0).value() * GetAccessorySocketGrade(false)) * fFactor : -(jewTbl->applies(0).value() * GetAccessorySocketGrade(false)) * fFactor);

		jewTbl = ITEM_MANAGER::Instance().GetTable(GetAccessorySocketByType(socketType, true));
		if (jewTbl && GetAccessorySocketGrade(true) > 0 && jewTbl->applies(0).type())
			pkChr->ApplyPoint(jewTbl->applies(0).type(), bAdd ? (jewTbl->applies(0).value() * GetAccessorySocketGrade(true)) * fFactor : -(jewTbl->applies(0).value() * GetAccessorySocketGrade(true)) * fFactor);
	}

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->applies(i).type() == APPLY_NONE)
			continue;

		long value = m_pProto->applies(i).value();

		if (m_pProto->applies(i).type() == APPLY_SKILL)
		{
			pkChr->ApplyPoint(m_pProto->applies(i).type(), bAdd ? value : value ^ 0x00800000);
		}
		else
		{
			if (0 != accessoryGrade && socketType == 0)
				value += MAX(accessoryGrade, value * aiAccessorySocketEffectivePct[accessoryGrade] / 100);

#ifdef __ALPHA_EQUIP__
			if (IsAlphaEquip())
				value += m_pProto->aApplies[i].lValue;
#endif

// #ifdef __PET_SYSTEM__
			// float fPetExtraVal;
			// if (pkChr->GetPetSystem() && (fPetExtraVal = pkChr->GetPetSystem()->GetSkillEquipBuffValue(GetCell() - EQUIPMENT_SLOT_START)) != 0.0f)
				// value += (int)((float)value * fPetExtraVal / 100.0f + 0.5f);
// #endif

			pkChr->ApplyPoint(m_pProto->applies(i).type(), bAdd ? value * fFactor : -value * fFactor);
			/*if (GetType() == ITEM_TOTEM)
				pkChr->tchat("Modify apply%d : type: %d val: %d", i, m_pProto->aApplies[i].bType, int(value*fFactor))*/;
		}
	}

#ifdef __COSTUME_ACCE__
	if (ITEM_COSTUME == GetType() && COSTUME_ACCE == GetSubType())
	{
		if (GetSocket(1) != 0)
		{
			auto c_pAbsorbItem = ITEM_MANAGER::instance().GetTable(GetSocket(1));
			if (c_pAbsorbItem)
			{
				for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
				{
					BYTE bType = c_pAbsorbItem->applies(i).type();
					long lValue = c_pAbsorbItem->applies(i).value();

					if (bType != APPLY_NONE && lValue > 0)
					{
						lValue = lValue * AcceCostumeGetReceptiveGrade() / 100;
						if (lValue <= 0)
							lValue = 1;

						pkChr->ApplyPoint(bType, bAdd ? lValue : -lValue);
					}
				}
			}
		}
	}
#endif


	// ÃÊ½Â´ÞÀÇ ¹ÝÁö, ÇÒ·ÎÀ© »çÅÁ, Çàº¹ÀÇ ¹ÝÁö, ¿µ¿øÇÑ »ç¶ûÀÇ Ææ´øÆ®ÀÇ °æ¿ì
	// ±âÁ¸ÀÇ ÇÏµå ÄÚµùÀ¸·Î °­Á¦·Î ¼Ó¼ºÀ» ºÎ¿©ÇßÁö¸¸,
	// ±× ºÎºÐÀ» Á¦°ÅÇÏ°í special item group Å×ÀÌºí¿¡¼­ ¼Ó¼ºÀ» ºÎ¿©ÇÏµµ·Ï º¯°æÇÏ¿´´Ù.
	// ÇÏÁö¸¸ ÇÏµå ÄÚµùµÇ¾îÀÖÀ» ¶§ »ý¼ºµÈ ¾ÆÀÌÅÛÀÌ ³²¾ÆÀÖÀ» ¼öµµ ÀÖ¾î¼­ Æ¯¼öÃ³¸® ÇØ³õ´Â´Ù.
	// ÀÌ ¾ÆÀÌÅÛµéÀÇ °æ¿ì, ¹Ø¿¡ ITEM_UNIQUEÀÏ ¶§ÀÇ Ã³¸®·Î ¼Ó¼ºÀÌ ºÎ¿©µÇ±â ¶§¹®¿¡,
	// ¾ÆÀÌÅÛ¿¡ ¹ÚÇôÀÖ´Â attribute´Â Àû¿ëÇÏÁö ¾Ê°í ³Ñ¾î°£´Ù.
	if (true == CItemVnumHelper::IsHalloweenCandy(GetVnum())
		|| true == CItemVnumHelper::IsHappinessRing(GetVnum()) || true == CItemVnumHelper::IsLovePendant(GetVnum()))
	{
		// Do not anything.
	}
	else
	{
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			if (GetAttributeType(i))
			{
				auto& ia = GetAttribute(i);
				float attrVal = ia.value();

#ifdef __COSTUME_ACCE__
				if (ITEM_COSTUME == GetType() && COSTUME_ACCE == GetSubType())
				{
					attrVal = float(ia.value()) / 100.0f * float(AcceCostumeGetReceptiveGrade());

					if (attrVal < 1.0)
						attrVal = 1.0;
					else
						attrVal = (int)attrVal;
				}
#endif

				if (ia.type() == APPLY_SKILL)
					pkChr->ApplyPoint(ia.type(), bAdd ? ia.value() : ia.value() ^ 0x00800000);
				else
					pkChr->ApplyPointF(ia.type(), bAdd ? attrVal * fFactor : -attrVal * fFactor);
			}
		}
	}

#ifdef __HEAVENSTONE__
	if (GetType() == ITEM_HEAVENSTONE)
	{
		const THeavenStoneProto* pHeavenStoneProto = ITEM_MANAGER::instance().GetHeavenStoneProto(GetVnum());
		if (pHeavenStoneProto)
		{
			int iLevel = GetHeavenStoneLevel();
			if (iLevel >= 0 && iLevel <= HEAVENSTONE_LEVEL_MAX)
				pkChr->ApplyPointF(pHeavenStoneProto->bApplyType, bAdd ? pHeavenStoneProto->iApplyValue[iLevel] * fFactor : -pHeavenStoneProto->iApplyValue[iLevel] * fFactor);
		}
	}
#endif

	// if (!bAdd)
		// pkChr->RefreshDefBuffBoni(this);
	// else
		// pkChr->RefreshDefBuffBoni();

	switch (m_pProto->type())
	{
		case ITEM_PICK:
		case ITEM_ROD:
			{
				if (bAdd)
				{
					if (m_wCell == EQUIPMENT_SLOT_START + WEAR_WEAPON)
						pkChr->SetPart(PART_WEAPON, GetVnum());
				}
				else
				{
					if (m_wCell == EQUIPMENT_SLOT_START + WEAR_WEAPON)
						pkChr->SetPart(PART_WEAPON, pkChr->GetOriginalPart(PART_WEAPON));
				}
			}
			break;

		case ITEM_WEAPON:
			{
				if (0 != pkChr->GetWear(WEAR_COSTUME_WEAPON))
					break;

				if (bAdd)
				{
					if (m_wCell == EQUIPMENT_SLOT_START + WEAR_WEAPON)
						pkChr->SetPart(PART_WEAPON, GetDisplayVnum());
				}
				else
				{
					if (m_wCell == EQUIPMENT_SLOT_START + WEAR_WEAPON)
						pkChr->SetPart(PART_WEAPON, pkChr->GetOriginalPart(PART_WEAPON));
				}
			}
			break;

		case ITEM_ARMOR:
			{
				// ÄÚ½ºÃõ body¸¦ ÀÔ°íÀÖ´Ù¸é armor´Â ¹þ´ø ÀÔ´ø »ó°ü ¾øÀÌ ºñÁÖ¾ó¿¡ ¿µÇâÀ» ÁÖ¸é ¾È µÊ.
				if (0 != pkChr->GetWear(WEAR_COSTUME_BODY) && !(GetVnum() >= 11901 && GetVnum() <= 11904))
					break;

				if (GetSubType() == ARMOR_BODY || GetSubType() == ARMOR_HEAD || GetSubType() == ARMOR_FOOTS || GetSubType() == ARMOR_SHIELD)
				{
					if (bAdd)
					{
						if (GetProto()->sub_type() == ARMOR_BODY)
							pkChr->SetPart(PART_MAIN, GetDisplayVnum());
					}
					else
					{
						if (GetProto()->sub_type() == ARMOR_BODY)
						{
							if (pkChr->GetWear(WEAR_COSTUME_BODY))
								pkChr->SetPart(PART_MAIN, pkChr->GetWear(WEAR_COSTUME_BODY)->GetDisplayVnum());
							else
								pkChr->SetPart(PART_MAIN, pkChr->GetOriginalPart(PART_MAIN));
						}
					}
				}
			}
			break;

		// ÄÚ½ºÃõ ¾ÆÀÌÅÛ ÀÔ¾úÀ» ¶§ Ä³¸¯ÅÍ parts Á¤º¸ ¼¼ÆÃ. ±âÁ¸ ½ºÅ¸ÀÏ´ë·Î Ãß°¡ÇÔ..
		case ITEM_COSTUME:
			{
				DWORD toSetValue = this->GetVnum();
				EParts toSetPart = PART_MAX_NUM;

				// °©¿Ê ÄÚ½ºÃõ
				if (GetSubType() == COSTUME_BODY)
				{
					toSetPart = PART_MAIN;

					if (pkChr->GetWear(WEAR_BODY) && pkChr->GetWear(WEAR_BODY)->GetVnum() >= 11901 && pkChr->GetWear(WEAR_BODY)->GetVnum() <= 11904)
						return;

					if (false == bAdd)
					{
						// ÄÚ½ºÃõ °©¿ÊÀ» ¹þ¾úÀ» ¶§ ¿ø·¡ °©¿ÊÀ» ÀÔ°í ÀÖ¾ú´Ù¸é ±× °©¿ÊÀ¸·Î look ¼¼ÆÃ, ÀÔÁö ¾Ê¾Ò´Ù¸é default look
						const CItem* pArmor = pkChr->GetWear(WEAR_BODY);
						toSetValue = (NULL != pArmor) ? pArmor->GetDisplayVnum() : pkChr->GetOriginalPart(PART_MAIN);
					}
					
				}

				// Çì¾î ÄÚ½ºÃõ
				else if (GetSubType() == COSTUME_HAIR)
				{
					toSetPart = PART_HAIR;

					// ÄÚ½ºÃõ Çì¾î´Â shape°ªÀ» item protoÀÇ value3¿¡ ¼¼ÆÃÇÏµµ·Ï ÇÔ. Æ¯º°ÇÑ ÀÌÀ¯´Â ¾ø°í ±âÁ¸ °©¿Ê(ARMOR_BODY)ÀÇ shape°ªÀÌ ÇÁ·ÎÅäÀÇ value3¿¡ ÀÖ¾î¼­ Çì¾îµµ °°ÀÌ value3À¸·Î ÇÔ.
					// [NOTE] °©¿ÊÀº ¾ÆÀÌÅÛ vnumÀ» º¸³»°í Çì¾î´Â shape(value3)°ªÀ» º¸³»´Â ÀÌÀ¯´Â.. ±âÁ¸ ½Ã½ºÅÛÀÌ ±×·¸°Ô µÇ¾îÀÖÀ½...
					toSetValue = (true == bAdd) ? this->GetValue(3) : GetOwner()->GetCurrentHair();
				}

#ifdef __COSTUME_ACCE__
				else if (GetSubType() == COSTUME_ACCE || GetSubType() == COSTUME_ACCE_COSTUME)
				{
					toSetPart = PART_ACCE;
					if (GetSubType() == COSTUME_ACCE && pkChr->GetWear(WEAR_COSTUME_ACCE))
						toSetValue = pkChr->GetWear(WEAR_COSTUME_ACCE)->GetVnum();
					else if (GetSubType() == COSTUME_ACCE_COSTUME && pkChr->GetWear(WEAR_ACCE) && !bAdd)
						toSetValue = pkChr->GetWear(WEAR_ACCE)->GetVnum();
					else
						toSetValue = (true == bAdd) ? this->GetVnum() : 0;
				}
#endif

				else if (GetSubType() == COSTUME_WEAPON)
				{
					toSetPart = PART_WEAPON;

					if (false == bAdd)
					{
						// ÄÚ½ºÃõ °©¿ÊÀ» ¹þ¾úÀ» ¶§ ¿ø·¡ °©¿ÊÀ» ÀÔ°í ÀÖ¾ú´Ù¸é ±× °©¿ÊÀ¸·Î look ¼¼ÆÃ, ÀÔÁö ¾Ê¾Ò´Ù¸é default look
						const CItem* pWeapon = pkChr->GetWear(WEAR_WEAPON);
						toSetValue = (NULL != pWeapon) ? pWeapon->GetDisplayVnum() : pkChr->GetOriginalPart(PART_WEAPON);
					}
				}

				if (PART_MAX_NUM != toSetPart)
				{
					// pkChr->SetPart((unsigned char)toSetPart, toSetValue);
					pkChr->SetPart((BYTE)toSetPart, toSetValue);
//					if (!IsNoPacketMode())
//						pkChr->UpdatePacket();
				}
			}
			break;
		case ITEM_UNIQUE:
			{
				if (0 != GetSIGVnum())
				{
					const CSpecialItemGroup* pItemGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(GetSIGVnum());
					if (NULL == pItemGroup)
						break;
					DWORD dwAttrVnum = pItemGroup->GetAttrVnum(GetVnum());
					const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(dwAttrVnum);
					if (NULL == pAttrGroup)
						break;
					for (itertype (pAttrGroup->m_vecAttrs) it = pAttrGroup->m_vecAttrs.begin(); it != pAttrGroup->m_vecAttrs.end(); it++)
					{
						pkChr->ApplyPoint(it->apply_type, bAdd ? it->apply_value : -it->apply_value);
					}
				}
			}
			break;
	}
}

#ifdef __SKIN_SYSTEM__
bool CItem::IsEquippedInBuffiSkinCell()
{
	WORD baseCell = GetCell() - EQUIPMENT_SLOT_START;

	bool bIsBuffiSkinCell = ( baseCell == SKINSYSTEM_SLOT_BUFFI_BODY || baseCell == SKINSYSTEM_SLOT_BUFFI_WEAPON || baseCell == SKINSYSTEM_SLOT_BUFFI_HAIR);

	//if(m_pOwner)
	//{
	//	m_pOwner->tchat("IsEquippedInBuffiSkinCell() ITEM CELL: %d", baseCell);
	//	m_pOwner->tchat("IsEquippedInBuffiSkinCell(): %d", bIsBuffiSkinCell);
	//}

	return bIsBuffiSkinCell;
}

bool CItem::IsBuffiSkinCell(WORD cell)
{
	//if(m_pOwner)
	//	m_pOwner->tchat("IsBuffiSkinCell(%d): %d", cell, ( cell == SKINSYSTEM_SLOT_BUFFI_BODY || cell == SKINSYSTEM_SLOT_BUFFI_WEAPON ));

	return ( cell == SKINSYSTEM_SLOT_BUFFI_BODY || cell == SKINSYSTEM_SLOT_BUFFI_WEAPON || cell == SKINSYSTEM_SLOT_BUFFI_HAIR);
}
#endif

bool CItem::IsEquipable() const
{
	switch (this->GetType())
	{
	case ITEM_COSTUME:
	case ITEM_ARMOR:
	case ITEM_WEAPON:
	case ITEM_ROD:
	case ITEM_PICK:
	case ITEM_UNIQUE:
	case ITEM_RING:
#ifdef __DRAGONSOUL__
	case ITEM_DS:
#endif
#ifdef __BELT_SYSTEM__
	case ITEM_BELT:
#endif
	case ITEM_TOTEM:
	case ITEM_SHINING:
		return true;
	}

	return false;
}

// return false on error state
bool CItem::EquipTo(LPCHARACTER ch, BYTE bWearCell)
{
	if (!ch)
	{
		sys_err("EquipTo: nil character");
		return false;
	}
	
	if(GetOwner())
		GetOwner()->tchat("bWearCell %d", bWearCell);
	
#ifdef __DRAGONSOUL__
	if (IsDragonSoul())
	{
		if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
		{
			sys_err("EquipTo: invalid dragon soul cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM);
			return false;
		}
	}
	else
#endif
	{
		if (GetType() == ITEM_SHINING)
		{
			if (bWearCell < WEAR_MAX_NUM || bWearCell >= WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_MAX_NUM)
			{
				sys_err("EquipTo: invalid shining cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetSubType(), bWearCell - WEAR_MAX_NUM - DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM - DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM);
				return false;
			}

			if (ch->HasShining(GetValue(0)))
				return false;
		}
#ifdef __SKIN_SYSTEM__
		else if(GetType() == ITEM_COSTUME)
		{
			if(GetSubType() == COSTUME_PET && bWearCell != SKINSYSTEM_SLOT_PET)
				return false;

			if(GetSubType() == COSTUME_MOUNT && bWearCell != SKINSYSTEM_SLOT_MOUNT)
				return false;
		}

		else if( bWearCell >= WEAR_MAX_NUM && !IsBuffiSkinCell(bWearCell) )
		{
			sys_err("EquipTo: invalid wear cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
#else
		else if(bWearCell >= WEAR_MAX_NUM)
		{
			sys_err("EquipTo: invalid wear cell (this: #%d %s wearflag: %d cell: %d)", GetOriginalVnum(), GetName(), GetWearFlag(), bWearCell);
			return false;
		}
#endif
	}

	if (ch->GetWear(bWearCell))
	{
		sys_err("EquipTo: item already exist (this: #%d %s cell: %d %s)", GetOriginalVnum(), GetName(), bWearCell, ch->GetWear(bWearCell)->GetName());
		return false;
	}

	if (GetOwner())
		RemoveFromCharacter();

#ifdef __SKIN_SYSTEM__
	bool bBuffiSkin = IsBuffiSkinCell(bWearCell);

	// Do only important things
	if(bBuffiSkin)
	{
		ch->SetWear(bWearCell, this);
		m_pOwner = ch;
		m_bEquipped = true;
		m_wCell = EQUIPMENT_SLOT_START + bWearCell;

		StartUniqueExpireEvent();

		if(-1 != GetProto()->limit_timer_based_on_wear_index())
			StartTimerBasedOnWearExpireEvent();

		Save();
		return true;
	}
#endif

	ch->SetWear(bWearCell, this); // ¿©±â¼­ ÆÐÅ¶ ³ª°¨

	m_pOwner = ch;
	m_bEquipped = true;
	m_wCell	= EQUIPMENT_SLOT_START + bWearCell;

	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		if (LPITEM pItem = m_pOwner->GetWear(i))
			SET_BIT(dwImmuneFlag, pItem->GetRealImmuneFlag());
	}

	m_pOwner->SetImmuneFlag(dwImmuneFlag);

#ifdef __DRAGONSOUL__
	if (IsDragonSoul())
	{
		DSManager::instance().ActivateDragonSoul(this);
	}
	else
#endif
	{
		if (GetType() == ITEM_SHINING)
#ifdef __DRAGONSOUL__
			m_pOwner->SetShining(bWearCell - (WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM), GetValue(0));
#else
			m_pOwner->SetShining(bWearCell - WEAR_MAX_NUM, GetValue(0));
#endif

		ModifyPoints(true);

#ifdef __FAKE_PC__
		m_pOwner->FakePC_Owner_ItemPoints(this, true);
#endif

		StartUniqueExpireEvent();
		if (-1 != GetProto()->limit_timer_based_on_wear_index())
			StartTimerBasedOnWearExpireEvent();

		// ACCESSORY_REFINE
		StartAccessorySocketExpireEvent();
		// END_OF_ACCESSORY_REFINE
	}

	ch->BuffOnAttr_AddBuffsFromItem(this);

	m_pOwner->ComputeBattlePoints();
#ifdef __FAKE_PC__
	m_pOwner->FakePC_Owner_ExecFunc(&CHARACTER::ComputeBattlePoints);
#endif

	m_pOwner->UpdatePacket();
#ifdef __FAKE_PC__
	m_pOwner->FakePC_Owner_ExecFunc(&CHARACTER::UpdatePacket);
#endif

	Save();

	return (true);
}

bool CItem::Unequip()
{

	if (!m_pOwner || GetCell() < EQUIPMENT_SLOT_START)
	{
		// ITEM_OWNER_INVALID_PTR_BUG
		sys_err("%s %u m_pOwner %p, GetCell %d", 
				GetName(), GetID(), get_pointer(m_pOwner), GetCell());
		// END_OF_ITEM_OWNER_INVALID_PTR_BUG
		return false;
	}

	if (this != m_pOwner->GetWear(GetCell() - EQUIPMENT_SLOT_START))
	{
		sys_err("m_pOwner->GetWear() != this");
		return false;
	}

#ifdef __PET_ADVANCED__
	if (m_petAdvanced)
		m_petAdvanced->Unsummon();
#endif

#ifdef __SKIN_SYSTEM__
	if(IsEquippedInBuffiSkinCell())
	{
		StopUniqueExpireEvent();

		if(-1 != GetProto()->limit_timer_based_on_wear_index())
			StopTimerBasedOnWearExpireEvent();

		m_pOwner->SetWear(GetCell() - EQUIPMENT_SLOT_START, NULL);

		m_pOwner = NULL;
		m_wCell = 0;
		m_bEquipped = false;

		return true;
	}
#endif

	//½Å±Ô ¸» ¾ÆÀÌÅÛ Á¦°Å½Ã Ã³¸®
	if (IsRideItem())
		ClearMountAttributeAndAffect();

#ifdef __DRAGONSOUL__
	if (IsDragonSoul())
	{
		DSManager::instance().DeactivateDragonSoul(this);
	}
	else
#endif
	{
		ModifyPoints(false);
#ifdef __FAKE_PC__
		m_pOwner->FakePC_Owner_ItemPoints(this, false);
#endif
	}

	StopUniqueExpireEvent();

	if (-1 != GetProto()->limit_timer_based_on_wear_index())
		StopTimerBasedOnWearExpireEvent();

	// ACCESSORY_REFINE
	StopAccessorySocketExpireEvent();
	// END_OF_ACCESSORY_REFINE


	m_pOwner->BuffOnAttr_RemoveBuffsFromItem(this);

	m_pOwner->SetWear(GetCell() - EQUIPMENT_SLOT_START, NULL);

	if (GetType() == ITEM_SHINING)
#ifdef __DRAGONSOUL__
		m_pOwner->SetShining(GetCell() - EQUIPMENT_SLOT_START - (WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM), 0);
#else
		m_pOwner->SetShining(bWearCell - GetCell() - EQUIPMENT_SLOT_START - WEAR_MAX_NUM, 0);
#endif

	DWORD dwImmuneFlag = 0;

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
		if (LPITEM pItem = m_pOwner->GetWear(i))
			SET_BIT(dwImmuneFlag, pItem->GetRealImmuneFlag());


	m_pOwner->SetImmuneFlag(dwImmuneFlag);
	m_pOwner->ComputeBattlePoints();
	m_pOwner->UpdatePacket();

	m_pOwner = NULL;
	m_wCell = 0;
	m_bEquipped	= false;

	return true;
}

long CItem::GetValue(DWORD idx) const
{
	assert(idx < ITEM_VALUES_MAX_NUM);
	return GetProto()->values(idx);
}

void CItem::SetExchanging(bool bOn)
{
	m_bExchanging = bOn;
}

void CItem::Save()
{
	if (m_bSkipSave)
		return;

	ITEM_MANAGER::instance().DelayedSave(this);
}

bool CItem::CreateSocket(BYTE bSlot, BYTE bGold)
{
	assert(bSlot < ITEM_SOCKET_MAX_NUM);

	if (m_alSockets[bSlot] != 0)
	{
		sys_err("Item::CreateSocket : socket already exist %s %d", GetName(), bSlot);
		return false;
	}

	if (bGold)
		m_alSockets[bSlot] = 2;
	else
		m_alSockets[bSlot] = 1;

	UpdatePacket();

	Save();
	return true;
}

void CItem::SetSockets(const long * c_al)
{
	thecore_memcpy(m_alSockets, c_al, sizeof(m_alSockets));
	Save();
}

void CItem::SetSockets(const ::google::protobuf::RepeatedField<::google::protobuf::int32> sockets)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM && i < sockets.size(); ++i)
		m_alSockets[i] = sockets[i];
	Save();
}

void CItem::SetSocket(int i, long v, bool bLog)
{
	assert(i < ITEM_SOCKET_MAX_NUM);

	if (m_alSockets[i] == v)
		return;

	m_alSockets[i] = v;
	UpdatePacket();
	Save();

/* 	if (bLog)
	{
		char szHint[256];
		snprintf(szHint, sizeof(szHint), "i=%d|v=%ld", i, v);
		LogManager::instance().ItemLog(i, GetID(), "SET_SOCKET", szHint, "", GetOriginalVnum());
	} */
}

int CItem::GetGold()
{
	if (IS_SET(GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (GetProto()->gold() == 0)
			return GetCount();
		else
			return GetCount() / GetProto()->gold();
	}
	else
		return GetProto()->gold();
}

int CItem::GetShopBuyPrice()
{
	return GetProto()->shop_buy_price();
}

bool CItem::IsOwnership(LPCHARACTER ch)
{
	if (!m_pkOwnershipEvent)
		return true;

	if (!ch)
		return false;

	return m_dwOwnershipPID == ch->GetPlayerID() ? true : false;
}

EVENTFUNC(ownership_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "ownership_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;

	pkItem->SetOwnershipEvent(NULL);

	network::GCOutputPacket<network::GCItemOwnershipPacket> p;
	p->set_vid(pkItem->GetVID());
	pkItem->PacketAround(p);
	return 0;
}

void CItem::SetOwnershipEvent(LPEVENT pkEvent)
{
	m_pkOwnershipEvent = pkEvent;
}

void CItem::SetOwnership(LPCHARACTER ch, int iSec)
{
	if (!ch)
	{
		if (m_pkOwnershipEvent)
		{
			event_cancel(&m_pkOwnershipEvent);
			m_dwOwnershipPID = 0;

			network::GCOutputPacket<network::GCItemOwnershipPacket> p;
			p->set_vid(m_dwVID);
			PacketAround(p);
		}
		return;
	}

	if (m_pkOwnershipEvent)
		return;

	if (iSec <= 10)
		iSec = 30;

	m_dwOwnershipPID = ch->GetPlayerID();

	item_event_info* info = AllocEventInfo<item_event_info>();
	strlcpy(info->szOwnerName, ch->GetName(), sizeof(info->szOwnerName));
	info->item = this;

	SetOwnershipEvent(event_create(ownership_event, info, PASSES_PER_SEC(iSec)));

	network::GCOutputPacket<network::GCItemOwnershipPacket> p;

	p->set_vid(m_dwVID);
	p->set_name(ch->GetName());

	PacketAround(p);
}

int CItem::GetSocketCount()
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
	{
		if (GetSocket(i) == 0)
			return i;
	}
	return ITEM_SOCKET_MAX_NUM;
}

bool CItem::AddSocket()
{
	int count = GetSocketCount();
	if (count == ITEM_SOCKET_MAX_NUM)
		return false;
	m_alSockets[count] = 1;
	return true;
}

void CItem::AlterToSocketItem(int iSocketCount)
{
	if (iSocketCount >= ITEM_SOCKET_MAX_NUM)
	{
		sys_log(!test_server, "Invalid Socket Count %d, set to maximum", ITEM_SOCKET_MAX_NUM);
		iSocketCount = ITEM_SOCKET_MAX_NUM;
	}

	for (int i = 0; i < iSocketCount; ++i)
	{
		if (IsRealTimeItem() && i <= 1)
			continue;

		SetSocket(i, 1);
	}
}

void CItem::AlterToMagicItem()
{
	int idx = GetAttributeSetIndex();

	if (idx < 0 || GetType() == ITEM_TOTEM)
		return;

	//	  Appeariance Second Third
	// Weapon 50		20	 5
	// Armor  30		10	 2
	// Acc	20		10	 1

	int iSecondPct=0;
	int iThirdPct=0;

	switch (GetType())
	{
		case ITEM_WEAPON:
			iSecondPct = 20;
			iThirdPct = 5;
			break;

		case ITEM_ARMOR:
			if (GetSubType() == ARMOR_BODY)
			{
				iSecondPct = 10;
				iThirdPct = 2;
			}
			else
			{
				iSecondPct = 10;
				iThirdPct = 1;
			}
			break;

		case ITEM_TOTEM:
			iSecondPct = 10;
			iThirdPct = 1;
			break;

		case ITEM_COSTUME:
			if (GetSubType() == COSTUME_BODY)
			{
				iSecondPct = 10;
				iThirdPct = 2;
			}
			else if (GetSubType() == COSTUME_HAIR)
			{
				iSecondPct = 10;
				iThirdPct = 1;
			}
			else if (GetSubType() == COSTUME_WEAPON)
			{
				iSecondPct = 20;
				iThirdPct = 5;
			}
			break;

		default:
			return;
	}

	// 100% È®·ü·Î ÁÁÀº ¼Ó¼º ÇÏ³ª
	PutAttribute(aiItemMagicAttributePercentHigh);

	if (random_number(1, 100) <= iSecondPct)
		PutAttribute(aiItemMagicAttributePercentLow);

	if (random_number(1, 100) <= iThirdPct)
		PutAttribute(aiItemMagicAttributePercentLow);
}

DWORD CItem::GetRefineFromVnum()
{
	if (GetRefineLevel() == 0)
		return 0;

	return ITEM_MANAGER::instance().GetRefineFromVnum(GetVnum());
}

int CItem::GetRefineLevel() const
{
	const char* name = GetBaseName();
	char* p = const_cast<char*>(strrchr(name, '+'));

	if (!p)
		return 0;

	int	rtn = 0;
	str_to_number(rtn, p+1);

	const char* locale_name = GetName();
	p = const_cast<char*>(strrchr(locale_name, '+'));

	if (p)
	{
		int	locale_rtn = 0;
		str_to_number(locale_rtn, p+1);
		if (locale_rtn != rtn)
		{
			sys_err("refine_level_based_on_NAME(%d) is not equal to refine_level_based_on_LOCALE_NAME(%d).", rtn, locale_rtn);
		}
	}

	return rtn;
}

bool CItem::IsPolymorphItem()
{
	return GetType() == ITEM_POLYMORPH;
}

EVENTFUNC(unique_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "unique_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;
	
	if (test_server)
		sys_err("%s:%d %p val0=%d val1=%d val2=%d socket=%d", __FILE__, __LINE__, pkItem->GetValue(0), pkItem->GetValue(1), pkItem->GetValue(2), pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME));
	
	if (pkItem->GetValue(2) == 0)
	{
		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= 1)
		{
			sys_log(0, "UNIQUE_ITEM: expire %s %u", pkItem->GetName(), pkItem->GetID());
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			pkItem->SetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME, pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - 1);
			return PASSES_PER_SEC(60);
		}
	}
	else
	{
		time_t cur = get_global_time();
		
		if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) <= cur)
		{
			pkItem->SetUniqueExpireEvent(NULL);
			ITEM_MANAGER::instance().RemoveItem(pkItem, "UNIQUE_EXPIRE");
			return 0;
		}
		else
		{
			// °ÔÀÓ ³»¿¡ ½Ã°£Á¦ ¾ÆÀÌÅÛµéÀÌ ºü¸´ºü¸´ÇÏ°Ô »ç¶óÁöÁö ¾Ê´Â ¹ö±×°¡ ÀÖ¾î
			// ¼öÁ¤
			// by rtsummit
			if (pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur < 600)
				return PASSES_PER_SEC(pkItem->GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) - cur);
			else
				return PASSES_PER_SEC(600);
		}
	}
}

// ½Ã°£ ÈÄºÒÁ¦
// timer¸¦ ½ÃÀÛÇÒ ¶§¿¡ ½Ã°£ Â÷°¨ÇÏ´Â °ÍÀÌ ¾Æ´Ï¶ó, 
// timer°¡ ¹ßÈ­ÇÒ ¶§¿¡ timer°¡ µ¿ÀÛÇÑ ½Ã°£ ¸¸Å­ ½Ã°£ Â÷°¨À» ÇÑ´Ù.
EVENTFUNC(timer_based_on_wear_expire_event)
{
	item_event_info* info = dynamic_cast<item_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "expire_event <Factor> Null pointer" );
		return 0;
	}

	LPITEM pkItem = info->item;
	int remain_time = pkItem->GetSocket(ITEM_SOCKET_REMAIN_SEC) - processing_time/passes_per_sec;
	if (remain_time <= 0)
	{
		sys_log(0, "ITEM EXPIRED : expired %s %u", pkItem->GetName(), pkItem->GetID());
		pkItem->SetTimerBasedOnWearExpireEvent(NULL);
		pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, 0);

#ifdef __DRAGONSOUL__
		if (pkItem->IsDragonSoul())
			DSManager::instance().DeactivateDragonSoul(pkItem);
		else
#endif
			ITEM_MANAGER::instance().RemoveItem(pkItem, "TIMER_BASED_ON_WEAR_EXPIRE");
		return 0;
	}
	pkItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	return PASSES_PER_SEC (MIN (60, remain_time));
}

void CItem::SetUniqueExpireEvent(LPEVENT pkEvent)
{
	m_pkUniqueExpireEvent = pkEvent;
}

void CItem::SetTimerBasedOnWearExpireEvent(LPEVENT pkEvent)
{
	m_pkTimerBasedOnWearExpireEvent = pkEvent;
}

EVENTFUNC(real_time_expire_event)
{
	const item_vid_event_info* info = reinterpret_cast<const item_vid_event_info*>(event->info);

	if (NULL == info)
		return 0;

	const LPITEM item = ITEM_MANAGER::instance().FindByVID( info->item_vid );

	if (NULL == item)
		return 0;

	const time_t current = get_global_time();

	if (current > item->GetSocket(0))
	{
		if(item->IsNewMountItem())
		{
			if (item->GetSocket(2) != 0)
				item->ClearMountAttributeAndAffect();
		}

		ITEM_MANAGER::instance().RemoveItem(item, "REAL_TIME_EXPIRE");

		return 0;
	}

	return PASSES_PER_SEC(1);
}

void CItem::StartRealTimeExpireEvent()
{
	if (m_pkRealTimeExpireEvent)
		return;
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->limits(i).type() || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->limits(i).type())
		{
			item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
			info->item_vid = GetVID();

			m_pkRealTimeExpireEvent = event_create( real_time_expire_event, info, PASSES_PER_SEC(1));

			// sys_log(0, "REAL_TIME_EXPIRE: StartRealTimeExpireEvent");

			return;
		}
	}
}

bool CItem::IsRealTimeItem()
{
	if(!GetProto())
		return false;
	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->limits(i).type() || LIMIT_REAL_TIME_START_FIRST_USE == GetProto()->limits(i).type())
			return true;
	}
	return false;
}

bool CItem::IsRefinedOtherItem() const
{
	if (GetRefinedVnum() == 0 || GetType() == ITEM_TOTEM)
		return false;

	return GetRefineLevel() == 9;
}

void CItem::StartUniqueExpireEvent()
{
	if (GetType() != ITEM_UNIQUE)
		return;

	if (m_pkUniqueExpireEvent)
		return;

	//±â°£Á¦ ¾ÆÀÌÅÛÀÏ °æ¿ì ½Ã°£Á¦ ¾ÆÀÌÅÛÀº µ¿ÀÛÇÏÁö ¾Ê´Â´Ù
	if (IsRealTimeItem())
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(false);

	int iSec = GetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME);

	if (iSec == 0)
		iSec = 60;
	else
		iSec = MIN(iSec, 60);

	if (test_server)
		sys_err("%s:%d START %p iSec=%d [%d,%d,%d] ", __FILE__, __LINE__, this, iSec, GetSocket(0), GetSocket(1), GetSocket(2));
	
	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, 0);

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetUniqueExpireEvent(event_create(unique_expire_event, info, PASSES_PER_SEC(iSec)));
}

// ½Ã°£ ÈÄºÒÁ¦
// timer_based_on_wear_expire_event ¼³¸í ÂüÁ¶
void CItem::StartTimerBasedOnWearExpireEvent()
{
	if (m_pkTimerBasedOnWearExpireEvent)
		return;

	//±â°£Á¦ ¾ÆÀÌÅÛÀÏ °æ¿ì ½Ã°£Á¦ ¾ÆÀÌÅÛÀº µ¿ÀÛÇÏÁö ¾Ê´Â´Ù
	if (IsRealTimeItem())
		return;

	if (-1 == GetProto()->limit_timer_based_on_wear_index())
		return;

	int iSec = GetSocket(0);
	
	// ³²Àº ½Ã°£À» ºÐ´ÜÀ§·Î ²÷±â À§ÇØ...
	if (0 != iSec)
	{
		iSec %= 60;
		if (0 == iSec)
			iSec = 60;
	}

	item_event_info* info = AllocEventInfo<item_event_info>();
	info->item = this;

	SetTimerBasedOnWearExpireEvent(event_create(timer_based_on_wear_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopUniqueExpireEvent()
{
	if (!m_pkUniqueExpireEvent)
		return;

	if (GetValue(2) != 0) // °ÔÀÓ½Ã°£Á¦ ÀÌ¿ÜÀÇ ¾ÆÀÌÅÛÀº UniqueExpireEvent¸¦ Áß´ÜÇÒ ¼ö ¾ø´Ù.
		return;

	// HARD CODING
	if (GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		m_pOwner->ShowAlignment(true);

	SetSocket(ITEM_SOCKET_UNIQUE_SAVE_TIME, event_time(m_pkUniqueExpireEvent) / passes_per_sec);
	event_cancel(&m_pkUniqueExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::StopTimerBasedOnWearExpireEvent()
{
	if (!m_pkTimerBasedOnWearExpireEvent)
		return;

	int remain_time = GetSocket(ITEM_SOCKET_REMAIN_SEC) - event_processing_time(m_pkTimerBasedOnWearExpireEvent) / passes_per_sec;

	SetSocket(ITEM_SOCKET_REMAIN_SEC, remain_time);
	event_cancel(&m_pkTimerBasedOnWearExpireEvent);

	ITEM_MANAGER::instance().SaveSingleItem(this);
}

void CItem::ApplyAddon(int iAddonType)
{
	CItemAddonManager::instance().ApplyAddonTo(iAddonType, this);
}

int CItem::GetSpecialGroup() const
{ 
	return ITEM_MANAGER::instance().GetSpecialGroupFromItem(GetVnum()); 
}

//
// ¾Ç¼¼¼­¸® ¼ÒÄÏ Ã³¸®.
//
bool CItem::IsAccessoryForSocket()
{
	if (m_pProto->type() == ITEM_BELT)
		return GetValue(0) == 1;

	return m_pProto->type() == ITEM_ARMOR && (m_pProto->sub_type() == ARMOR_WRIST || m_pProto->sub_type() == ARMOR_NECK || m_pProto->sub_type() == ARMOR_EAR);
}

BYTE CItem::GetAccessorySocketType()
{
	return BYTE(GetSocket(0) >> 16);
}

DWORD CItem::GetAccessorySocketByType(BYTE type, bool isPerma)
{
	DWORD dwVnum = GetVnum();
	if (dwVnum >= 17580 && dwVnum < 17580 + 10 ||
		dwVnum >= 14580 && dwVnum < 14580 + 10 ||
		dwVnum >= 16580 && dwVnum < 16580 + 10)
	{
		switch (type)
		{
		case 1:
			if (isPerma)
				return 93056;
			else
				return 50638;
		case 2:
			if (isPerma)
				return 93054;
			else
				return 50636;
		case 3:
			if (isPerma)
				return 93055;
			else
				return 50637;
		case 4:
			if (isPerma)
				return 93053;
			else
				return 50635;
		}
	}
	if (GetType() == ITEM_BELT && isPerma)
	{
		switch (type)
		{
		case 1:
			return 94340;
		case 2:
			return 94341;
		}
	}
	return 0;
}

void CItem::SetAccessorySocketType(BYTE bType)
{
	long currSocket = GetSocket(0);
	BYTE currNormal = BYTE(currSocket);
	BYTE currPerma = BYTE(currSocket >> 8);

	currSocket = (bType << 16) + (currPerma << 8) + currNormal;
	SetSocket(0, currSocket);
}

void CItem::SetAccessorySocketGrade(BYTE iGrade, bool setPerma) 
{ 
	// pack out
	long currSocket = GetSocket(0);
	BYTE currNormal = BYTE(currSocket);
	BYTE currPerma = BYTE(currSocket >> 8);
	BYTE jewType = BYTE(currSocket >> 16);

	if (setPerma)
		currPerma = iGrade;
	else
		currNormal = iGrade;

	// pack back
	currSocket = (jewType << 16) + (currPerma << 8) + currNormal;
	SetSocket(0, currSocket);

	int iDownTime = aiAccessorySocketDegradeTime[MIN(currNormal + currPerma, ITEM_ACCESSORY_SOCKET_MAX_NUM)];

	//if (test_server)
	//	iDownTime /= 60;

	SetAccessorySocketDownGradeTime(iDownTime);
}

void CItem::SetAccessorySocketMaxGrade(int iMaxGrade) 
{ 
	SetSocket(1, MINMAX(0, iMaxGrade, ITEM_ACCESSORY_SOCKET_MAX_NUM)); 
}

void CItem::SetAccessorySocketDownGradeTime(DWORD time) 
{ 
	SetSocket(2, time); 

	if (test_server && GetOwner())
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "%s¿¡¼­ ¼ÒÄÏ ºüÁú¶§±îÁö ³²Àº ½Ã°£ %d"), GetName(), time);
}

EVENTFUNC(accessory_socket_expire_event)
{
	item_vid_event_info* info = dynamic_cast<item_vid_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "accessory_socket_expire_event> <Factor> Null pointer" );
		return 0;
	}

	LPITEM item = ITEM_MANAGER::instance().FindByVID(info->item_vid);

	if (item->GetAccessorySocketDownGradeTime() <= 1)
	{
degrade:
		item->SetAccessorySocketExpireEvent(NULL);
		item->AccessorySocketDegrade();
		return 0;
	}
	else
	{
		int iTime = item->GetAccessorySocketDownGradeTime() - 60;

		if (iTime <= 1)
			goto degrade;

		item->SetAccessorySocketDownGradeTime(iTime);

		if (iTime > 60)
			return PASSES_PER_SEC(60);
		else
			return PASSES_PER_SEC(iTime);
	}
}

void CItem::StartAccessorySocketExpireEvent()
{
	if (!IsAccessoryForSocket())
		return;

	if (m_pkAccessorySocketExpireEvent)
		return;

	if (GetAccessorySocketMaxGrade() == 0)
		return;

	if (GetAccessorySocketGrade(false) == 0)
		return;

	int iSec = GetAccessorySocketDownGradeTime();
	SetAccessorySocketExpireEvent(NULL);

	if (iSec <= 1)
		iSec = 5;
	else
		iSec = MIN(iSec, 60);

	item_vid_event_info* info = AllocEventInfo<item_vid_event_info>();
	info->item_vid = GetVID();

	SetAccessorySocketExpireEvent(event_create(accessory_socket_expire_event, info, PASSES_PER_SEC(iSec)));
}

void CItem::StopAccessorySocketExpireEvent()
{
	if (!m_pkAccessorySocketExpireEvent)
		return;

	if (!IsAccessoryForSocket())
		return;

	int new_time = GetAccessorySocketDownGradeTime() - (60 - event_time(m_pkAccessorySocketExpireEvent) / passes_per_sec);

	event_cancel(&m_pkAccessorySocketExpireEvent);

	if (new_time <= 1)
	{
		AccessorySocketDegrade();
	}
	else
	{
		SetAccessorySocketDownGradeTime(new_time);
	}
}
		
bool CItem::IsRideItem()
{
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_RIDE == GetSubType())
		return true;
	if (ITEM_UNIQUE == GetType() && UNIQUE_SPECIAL_MOUNT_RIDE == GetSubType())
		return true;
	return false;
}

void CItem::ClearMountAttributeAndAffect()
{
	LPCHARACTER ch = GetOwner();

	ch->RemoveAffect(AFFECT_MOUNT);
	ch->RemoveAffect(AFFECT_MOUNT_BONUS);

	ch->MountVnum(0);

	ch->PointChange(POINT_ST, 0);
	ch->PointChange(POINT_DX, 0);
	ch->PointChange(POINT_HT, 0);
	ch->PointChange(POINT_IQ, 0);
}

// fixme
// ÀÌ°Å Áö±ÝÀº ¾È¾´µ¥... ±Ùµ¥ È¤½Ã³ª ½Í¾î¼­ ³²°ÜµÒ.
// by rtsummit
bool CItem::IsNewMountItem()
{
	switch(GetVnum())
	{
		case 76000: case 76001: case 76002: case 76003: 
		case 76004: case 76005: case 76006: case 76007:
		case 76008: case 76009: case 76010: case 76011: 
		case 76012: case 76013: case 76014:
			return true;
	}
	return false;
}

bool CItem::IsNormalEquipItem() const
{
	if (GetType() == ITEM_WEAPON)
		return true;

	if (GetType() == ITEM_ARMOR)
	{
		switch (GetSubType())
		{
		case ARMOR_BODY:
		case ARMOR_EAR:
		case ARMOR_FOOTS:
		case ARMOR_HEAD:
		case ARMOR_NECK:
		case ARMOR_SHIELD:
		case ARMOR_WRIST:
			return true;
			break;
		}
	}

	return false;
}

DWORD CItem::GetRealImmuneFlag()
{
	DWORD dwImmuneFlag = GetImmuneFlag();

	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
	{
		if (!GetAttribute(i).value())
			continue;

		if (GetAttribute(i).type() == APPLY_IMMUNE_STUN)
			SET_BIT(dwImmuneFlag, IMMUNE_STUN);
		else if (GetAttribute(i).type() == APPLY_IMMUNE_SLOW)
			SET_BIT(dwImmuneFlag, IMMUNE_SLOW);
		else if (GetAttribute(i).type() == APPLY_IMMUNE_FALL)
			SET_BIT(dwImmuneFlag, IMMUNE_FALL);
	}

	return dwImmuneFlag;
}

void CItem::SetAccessorySocketExpireEvent(LPEVENT pkEvent)
{
	m_pkAccessorySocketExpireEvent = pkEvent;
}

void CItem::AccessorySocketDegrade()
{
	if (GetAccessorySocketGrade(false) > 0)
	{
		LPCHARACTER ch = GetOwner();

		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s¿¡ ¹ÚÇôÀÖ´ø º¸¼®ÀÌ »ç¶óÁý´Ï´Ù."), GetName(ch->GetLanguageID()));
		}

		ModifyPoints(false);
#ifdef __FAKE_PC__
		m_pOwner->FakePC_Owner_ItemPoints(this, false);
#endif
		SetAccessorySocketGrade(GetAccessorySocketGrade(false)-1, false);
		ModifyPoints(true);
#ifdef __FAKE_PC__
		m_pOwner->FakePC_Owner_ItemPoints(this, true);
#endif

		GetOwner()->ComputeBattlePoints();
#ifdef __FAKE_PC__
		GetOwner()->FakePC_Owner_ExecFunc(&CHARACTER::ComputeBattlePoints);
#endif

		int iDownTime = GetAccessorySocketDownGradeTime();

		if (test_server)
			iDownTime /= 60;

		SetAccessorySocketDownGradeTime(iDownTime);

		if (iDownTime)
			StartAccessorySocketExpireEvent();
	}
}

// ring¿¡ itemÀ» ¹ÚÀ» ¼ö ÀÖ´ÂÁö ¿©ºÎ¸¦ Ã¼Å©ÇØ¼­ ¸®ÅÏ
static const bool CanPutIntoRing(LPITEM ring, LPITEM item)
{
	const DWORD vnum = item->GetVnum();
	return false;
}

static const bool CanPutIntoBelt(LPITEM belt, LPITEM item)
{
	const DWORD vnum = item->GetVnum();
	switch (vnum)
	{
		case 18900:
		case 94340:
		case 94341:
			if (belt->GetAccessorySocketGradeTotal() == 0 || belt->GetAccessorySocketType() == item->GetValue(3))
				return true;
	}

	return false;
}

bool CanPutIntoV(LPITEM item, DWORD jewelVnum, BYTE bSubType)
{
	DWORD vnum = item->GetVnum();

	struct JewelAccessoryInfo
	{
		DWORD jewel;
		DWORD wrist;
		DWORD neck;
		DWORD ear;
	};
	const static JewelAccessoryInfo infos[] = { 
		{ 50633, 19180, 19170, 19160 },
		{ 50634, 14220, 16220, 17220 },
		{ 50634, 19240, 19230, 19220 },
		{ 50635, 14500, 16500, 17500 }, 
		{ 50636, 14520, 16520, 17520 }, 
		{ 50637, 14540, 16540, 17540 }, 
		{ 50638, 14560, 16560, 17560 }, 
		{ 50639, 14570, 16570, 17570 },
	};

	const static JewelAccessoryInfo infosPerma[] = {
		{ 93042, 14020, 16020, 17020 },
		{ 93043, 14040, 16040, 17040 },
		{ 93044, 14060, 16060, 17060 },
		{ 93045, 14080, 16080, 17080 },
		{ 93046, 14100, 16100, 17100 },
		{ 93047, 14120, 16120, 17120 },
		{ 93048, 14140, 16140, 17140 },
		{ 93049, 14160, 16160, 17160 },
		{ 93050, 14180, 16180, 17180 },
		{ 93051, 14200, 16200, 17200 },
		{ 93052, 14220, 16220, 17220 },
		{ 93053, 14500, 16500, 17500 },
		{ 93054, 14520, 16520, 17520 },
		{ 93055, 14540, 16540, 17540 },
		{ 93056, 14560, 16560, 17560 },
		{ 93057, 14570, 16570, 17570 },
		{ 93051, 19180, 19170, 19160 },
		{ 93052, 19240, 19230, 19220 },
	};
	
	DWORD item_type = (item->GetVnum() / 10) * 10;

	if (item_type == 17580 || item_type == 14580 || item_type == 16580)
	{
		auto pTable = ITEM_MANAGER::Instance().GetTable(jewelVnum);
		if (!pTable)
			return false;

		if (item->GetAccessorySocketType() == 0 || item->GetAccessorySocketGradeTotal() == 0)
		{
			if (jewelVnum <= 93056 && jewelVnum >= 93053 ||
				jewelVnum <= 50638 && jewelVnum >= 50635)
				return true;
		}
		else if (item->GetAccessorySocketType() == pTable->values(3))
			return true;
		return false;
	}

	if (bSubType == USE_PUT_INTO_ACCESSORY_SOCKET_PERMA)
	{
		for (int i = 0; i < sizeof(infosPerma) / sizeof(infosPerma[0]); ++i)
		{
			const JewelAccessoryInfo& info = infosPerma[i];
			switch (item->GetSubType())
			{
			case ARMOR_WRIST:
				if (info.wrist == item_type)
				{
					if (info.jewel == jewelVnum)
						return true;

					return false;
				}
				break;
			case ARMOR_NECK:
				if (info.neck == item_type)
				{
					if (info.jewel == jewelVnum)
						return true;
					return false;
				}
				break;
			case ARMOR_EAR:
				if (info.ear == item_type)
				{
					if (info.jewel == jewelVnum)
						return true;
					return false;
				}
				break;
			}
		}
		return false;
	}
	
	for (int i = 0; i < sizeof(infos) / sizeof(infos[0]); i++)
	{
		const JewelAccessoryInfo& info = infos[i];
		switch(item->GetSubType())
		{
		case ARMOR_WRIST:
			if (info.wrist == item_type)
			{
				if (info.jewel == jewelVnum)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_NECK:
			if (info.neck == item_type)
			{
				if (info.jewel == jewelVnum)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		case ARMOR_EAR:
			if (info.ear == item_type)
			{
				if (info.jewel == jewelVnum)
				{
					return true;
				}
				else
				{
					return false;
				}
			}
			break;
		}
	}
	if (item->GetSubType() == ARMOR_WRIST)
		vnum -= 14000;
	else if (item->GetSubType() == ARMOR_NECK)
		vnum -= 16000;
	else if (item->GetSubType() == ARMOR_EAR)
		vnum -= 17000;
	else
		return false;

	DWORD type = vnum / 20;

	if (type < 0 || type > 11)
	{
		type = (vnum - 170) / 20;

		if (50623 + type != jewelVnum)
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16210 && item->GetVnum() <= 16219)
	{
		if (50625 != jewelVnum)
			return false;
		else
			return true;
	}
	else if (item->GetVnum() >= 16230 && item->GetVnum() <= 16239)
	{
		if (50626 != jewelVnum)
			return false;
		else
			return true;
	}

	return 50623 + type == jewelVnum;
}

bool CItem::CanPutInto(LPITEM item)
{
	if (item->GetType() == ITEM_RING)
		return CanPutIntoRing(item, this);

	else if (item->GetType() == ITEM_BELT)
		return CanPutIntoBelt(item, this);

	else if (item->GetType() != ITEM_ARMOR)
		return false;

	return CanPutIntoV(item, GetVnum(), GetSubType());
}

bool CItem::CheckItemUseLevel(int nLevel)
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->limits(i).type() == LIMIT_LEVEL)
		{
			if (this->m_pProto->limits(i).value() > nLevel) return false;
			else return true;
		}
	}
	return true;
}

long CItem::FindApplyValue(BYTE bApplyType)
{
	if (m_pProto == NULL)
		return 0;

	for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
	{
		if (m_pProto->applies(i).type() == bApplyType)
			return m_pProto->applies(i).value();
	}

	return 0;
}

void CItem::CopySocketTo(LPITEM pItem)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		pItem->m_alSockets[i] = m_alSockets[i];
	}
}
BYTE CItem::GetAccessorySocketGrade(bool countPerma)
{
	BYTE val;
	if (countPerma)
		val = BYTE(GetSocket(0) >> 8);
	else
		val = (BYTE)GetSocket(0);

	return MINMAX(0, val, GetAccessorySocketMaxGrade());
}

BYTE CItem::GetAccessorySocketGradeTotal()
{
	long socket = GetSocket(0);
   	return MINMAX(0, (BYTE)socket + BYTE(socket >> 8), GetAccessorySocketMaxGrade());
}

BYTE CItem::GetAccessorySocketMaxGrade()
{
   	return MINMAX(0, GetSocket(1), ITEM_ACCESSORY_SOCKET_MAX_NUM);
}

int CItem::GetAccessorySocketDownGradeTime()
{
	return MINMAX(0, GetSocket(2), aiAccessorySocketDegradeTime[GetAccessorySocketGradeTotal()]);
}

void CItem::AttrLog()
{
	const char * pszIP = NULL;

	if (GetOwner() && GetOwner()->GetDesc())
		pszIP = GetOwner()->GetDesc()->GetHostName();

	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (m_alSockets[i])
		{
		//	LogManager::instance().ItemLog(i, GetID(), "INFO_SOCKET", "", pszIP ? pszIP : "", GetOriginalVnum());
		}
	}

	for (int i = 0; i<ITEM_ATTRIBUTE_MAX_NUM; ++i)
	{
		int	type	= m_aAttr[i].type();
		int value	= m_aAttr[i].value();

		//if (type)
		//	LogManager::instance().ItemLog(i, GetID(), "INFO_ATTR", "", pszIP ? pszIP : "", GetOriginalVnum());
	}
}

int CItem::GetLevelLimit()
{
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		if (this->m_pProto->limits(i).type() == LIMIT_LEVEL)
		{
			return this->m_pProto->limits(i).value();
		}
	}
	return 0;
}

bool CItem::OnAfterCreatedItem()
{
	// ¾ÆÀÌÅÛÀ» ÇÑ ¹øÀÌ¶óµµ »ç¿ëÇß´Ù¸é, ±× ÀÌÈÄ¿£ »ç¿ë ÁßÀÌÁö ¾Ê¾Æµµ ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä
	if (-1 != this->GetProto()->limit_real_time_first_use_index())
	{
		// Socket1¿¡ ¾ÆÀÌÅÛÀÇ »ç¿ë È½¼ö°¡ ±â·ÏµÇ¾î ÀÖÀ¸´Ï, ÇÑ ¹øÀÌ¶óµµ »ç¿ëÇÑ ¾ÆÀÌÅÛÀº Å¸ÀÌ¸Ó¸¦ ½ÃÀÛÇÑ´Ù.
		if (0 != GetSocket(1))
		{
			StartRealTimeExpireEvent();
		}
	}

	return true;
}

#ifdef __DRAGONSOUL__
bool CItem::IsDragonSoul()
{
	return GetType() == ITEM_DS;
}

#ifdef DS_TIME_ELIXIR_FIX
int CItem::GiveMoreTime_Per(float fPercent, int time)
#else
int CItem::GiveMoreTime_Per(float fPercent)
#endif
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		int remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		int given_time = fPercent * duration / 100;

#ifdef DS_TIME_ELIXIR_FIX
		if (time > 0)
		{
			given_time = time;
		}
		else if (time == 0) // set defaults if socket 0 has value 0
		{
			switch ((int)fPercent)
			{
			case 5:
				given_time = 4320;
				break;
			case 15:
				given_time = 12960;
				break;
			case 30:
				given_time = 25920;
				break;
			case 50:
				given_time = 43200;
				break;
			default: // ?
				given_time = 0;
			}
		}
#endif

		if (remain_sec == duration)
			return false;
		if ((given_time + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, given_time + remain_sec);
			return given_time;
		}
	}
	// ¿ì¼± ¿ëÈ¥¼®¿¡ °üÇØ¼­¸¸ ÇÏµµ·Ï ÇÑ´Ù.
	else
		return 0;
}

int CItem::GiveMoreTime_Fix(DWORD dwTime)
{
	if (IsDragonSoul())
	{
		DWORD duration = DSManager::instance().GetDuration(this);
		int remain_sec = GetSocket(ITEM_SOCKET_REMAIN_SEC);
		if (remain_sec == duration)
			return false;
		if ((dwTime + remain_sec) >= duration)
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, duration);
			return duration - remain_sec;
		}
		else
		{
			SetSocket(ITEM_SOCKET_REMAIN_SEC, dwTime + remain_sec);
			return dwTime;
		}
	}
	// ¿ì¼± ¿ëÈ¥¼®¿¡ °üÇØ¼­¸¸ ÇÏµµ·Ï ÇÑ´Ù.
	else
		return 0;
}
#endif

#ifdef CRYSTAL_SYSTEM
EVENTFUNC(crystal_timeout_event)
{
	auto info = dynamic_cast<item_event_info*>(event->info);
	if (!info)
		return 0;

	info->item->on_crystal_timeout_event();
	return 0;
}

void CItem::start_crystal_use(bool set_last_use_time)
{
	if (GetSocket(CRYSTAL_TIME_SOCKET) == 0)
		return;

	if (is_crystal_using())
		return;

	if (set_last_use_time)
		_crystal_last_use_time = get_dword_time();

	clear_crystal_char_events();
	start_crystal_timeout_event();

	ModifyPoints(true);
	Lock(true);

	network::GCOutputPacket<network::GCCrystalUsingSlotPacket> pack;
	*pack->mutable_cell() = ::TItemPos(GetWindow(), GetCell());
	pack->set_active(true);
	if (GetOwner() && GetOwner()->GetDesc())
		GetOwner()->GetDesc()->Packet(pack);

	if (GetOwner())
	{
		_crystal_event_char = GetOwner();
		_crystal_event_handle = GetOwner()->add_event(CHARACTER::EEventTypes::COMPUTE_POINTS, [this](LPCHARACTER ch) {
			ModifyPoints(true, ch);
		});
		_crystal_event_destroy_handle = GetOwner()->add_event(CHARACTER::EEventTypes::DESTROY, [this](LPCHARACTER ch) {
			_crystal_event_char = nullptr;
			_crystal_event_handle = 0;
			_crystal_event_destroy_handle = 0;
		});

		GetOwner()->set_active_crystal_id(GetID());
	}
}

void CItem::stop_crystal_use()
{
	if (!is_crystal_using())
		return;

	if (_crystal_last_use_time != 0 && get_dword_time() - _crystal_last_use_time < CItem::CRYSTAL_MIN_ACTIVE_DURATION * 1000)
	{
		if (GetOwner())
			GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(GetOwner(), "The crystal must be active for at least one minute before deactivating."));
		return;
	}

	SetSocket(CRYSTAL_TIME_SOCKET, get_crystal_duration());
	event_cancel(&_crystal_timeout_event);

	ModifyPoints(false);
	Lock(false);

	network::GCOutputPacket<network::GCCrystalUsingSlotPacket> pack;
	*pack->mutable_cell() = ::TItemPos(GetWindow(), GetCell());
	pack->set_active(false);
	if (GetOwner() && GetOwner()->GetDesc())
		GetOwner()->GetDesc()->Packet(pack);

	clear_crystal_char_events();

	if (!GetSkipSave() && GetOwner() && GetOwner()->get_active_crystal_id() == GetID())
		GetOwner()->set_active_crystal_id(0);

	if (GetOwner())
		GetOwner()->CheckMaximumPoints();
}

DWORD CItem::get_crystal_duration() const
{
	auto duration = GetSocket(CRYSTAL_TIME_SOCKET);

	if (is_crystal_using())
		duration -= event_processing_time(_crystal_timeout_event) / passes_per_sec;

	return MINMAX(0, duration, CRYSTAL_MAX_DURATION);
}

void CItem::restore_crystal_energy(DWORD restore_duration)
{
	auto current_duration = get_crystal_duration();

	bool is_using = is_crystal_using();
	if (is_using)
		event_cancel(&_crystal_timeout_event);
	
	SetSocket(CRYSTAL_TIME_SOCKET, MIN(current_duration + restore_duration, CRYSTAL_MAX_DURATION));

	if (is_using)
		start_crystal_timeout_event();
}

void CItem::set_crystal_grade(const std::shared_ptr<network::TCrystalProto>& proto)
{
	if (!proto)
		return;

	if (is_crystal_using() && GetOwner())
		ModifyPoints(false);

	SetSocket(CRYSTAL_CLARITY_TYPE_SOCKET, proto->clarity_type());
	SetSocket(CRYSTAL_CLARITY_LEVEL_SOCKET, proto->clarity_level());

	ClearAllAttribute();

	for (auto& attr : proto->applies())
	{
		if (attr.type() == 0 || attr.value() == 0)
			continue;

		AddAttribute(attr.type(), attr.value());
	}

	if (is_crystal_using() && GetOwner())
		ModifyPoints(true);
}

void CItem::on_crystal_timeout_event()
{
	auto duration = MAX(0, GetSocket(CRYSTAL_TIME_SOCKET) - 60);
	if (duration == 0)
		stop_crystal_use();
	else
		start_crystal_timeout_event();

	SetSocket(CRYSTAL_TIME_SOCKET, duration);
}

void CItem::start_crystal_timeout_event()
{
	event_cancel(&_crystal_timeout_event);

	auto info = AllocEventInfo<item_event_info>();
	info->item = this;
	_crystal_timeout_event = event_create(crystal_timeout_event, info, PASSES_PER_SEC(MIN(60, GetSocket(CRYSTAL_TIME_SOCKET))));
}

void CItem::clear_crystal_char_events()
{
	if (_crystal_event_char)
	{
		_crystal_event_char->remove_event(CHARACTER::EEventTypes::COMPUTE_POINTS, _crystal_event_handle);
		_crystal_event_char->remove_event(CHARACTER::EEventTypes::DESTROY, _crystal_event_destroy_handle);
		_crystal_event_handle = 0;
		_crystal_event_destroy_handle = 0;
		_crystal_event_char = nullptr;
	}
}
#endif

int	CItem::GetDuration()
{
	if(!GetProto())
		return -1;

	for (int i=0 ; i < ITEM_LIMIT_MAX_NUM ; i++)
	{
		if (LIMIT_REAL_TIME == GetProto()->limits(i).type())
			return GetProto()->limits(i).value();
	}
	
	if (-1 != GetProto()->limit_timer_based_on_wear_index())
		return GetProto()->limits(GetProto()->limit_timer_based_on_wear_index()).value();	

	return -1;
}

bool CItem::IsSameSpecialGroup(const LPITEM item) const
{
	// ¼­·Î VNUMÀÌ °°´Ù¸é °°Àº ±×·ìÀÎ °ÍÀ¸·Î °£ÁÖ
	if (this->GetVnum() == item->GetVnum())
		return true;

	if (GetSpecialGroup() && (item->GetSpecialGroup() == GetSpecialGroup()))
		return true;

	return false;
}

#ifdef __ANIMAL_SYSTEM__
const TAnimalLevelData* CItem::Animal_GetLevelData()
{
	const TAnimalLevelData* pData = NULL;

	switch (GetType())
	{
#ifdef __PET_SYSTEM__
	case ITEM_PET:
		pData = &PetAnimalData;
		break;
#endif

	case ITEM_MOUNT:
		pData = &MountAnimalData;
		break;
	}

	if (!pData)
	{
		sys_err("Animal_GiveBuff [vnum %u id %u]: cannot give buff due to invalid type (%u)", GetVnum(), GetID(), GetType());
		return NULL;
	}

	return pData;
}

bool CItem::Animal_IsAnimal() const
{
	return Animal_GetType() != ANIMAL_TYPE_UNKNOWN;
}

void CItem::Animal_SetLevel(BYTE bLevel)
{
	SetSocket(1, bLevel);
}

BYTE CItem::Animal_GetLevel()
{
	return MINMAX(1, GetSocket(1), gAnimalMaxLevel);
}

void CItem::Animal_GiveEXP(long lExp)
{
#ifdef __ANIMAL_SYSTEM__
#ifdef __PET_SYSTEM__
	if (GetOwner() && Animal_GetType() == ANIMAL_TYPE_PET)
		lExp += lExp * GetOwner()->GetPoint(POINT_PET_EXP_BONUS) / 100;
#endif
	if (GetOwner() && Animal_GetType() == ANIMAL_TYPE_MOUNT)
		lExp += lExp * GetOwner()->GetPoint(POINT_MOUNT_EXP_BONUS) / 100;
#endif

	if (Animal_GetLevel() < gAnimalMaxLevel)
	{
		Animal_SetEXP(Animal_GetEXP() + MIN(lExp, Animal_GetMaxEXP() / 10));

		bool bLevelChange = false;
		while (Animal_GetEXP() >= Animal_GetMaxEXP())
		{
			bLevelChange = true;
			Animal_SetEXP(Animal_GetEXP() - Animal_GetMaxEXP());
			Animal_SetLevel(Animal_GetLevel() + 1);
			Animal_SetStatusPoints(Animal_GetStatusPoints() + 1);
			Animal_IncreaseStatus(ANIMAL_STATUS_MAIN);

			if (Animal_GetLevel() >= gAnimalMaxLevel)
			{
				Animal_SetStatusPoints(Animal_GetStatusPoints() + 1);
				Animal_SetEXP(0);
				break;
			}
		}

		Animal_ExpPacket();
		if (bLevelChange)
			Animal_LevelPacket();
	}
}

void CItem::Animal_SetEXP(long lExp)
{
	SetSocket(2, lExp);
}

long CItem::Animal_GetEXP()
{
	return GetSocket(2);
}

long CItem::Animal_GetMaxEXP()
{
	if (Animal_GetLevel() >= gAnimalMaxLevel)
		return 0;

	switch (Animal_GetType())
	{
		case ANIMAL_TYPE_MOUNT:
			return mount_exp_table[Animal_GetLevel()];

#ifdef __PET_SYSTEM__
		case ANIMAL_TYPE_PET:
			return pet_exp_table[Animal_GetLevel()];
#endif
	}

	return 0;
}

void CItem::Animal_SetStatusPoints(int iPoints)
{
	SetAttribute(0, iPoints, GetAttributeValue(0));
}

int CItem::Animal_GetStatusPoints()
{
	return GetAttributeType(0);
}

void CItem::Animal_SetStatus(BYTE bStatusID, short sValue)
{
	SetAttribute(bStatusID, GetAttributeType(bStatusID), sValue);
}

short CItem::Animal_GetStatus(BYTE bStatusID)
{
	return GetAttributeValue(bStatusID);
}

short CItem::Animal_GetApplyStatus(BYTE bStatusID, BYTE* pbRetType)
{
	const TAnimalLevelData* pData = Animal_GetLevelData();
	if (!pData)
		return 0;

	if (bStatusID == ANIMAL_STATUS_MAIN)
	{
		if (pbRetType)
			*pbRetType = pData->kMainStat.bType;
		return pData->kMainStat.sValueBase + Animal_GetStatus(bStatusID) * pData->kMainStat.sValuePerLevel;
	}
	else
	{
		if (pbRetType)
			*pbRetType = pData->kStat[bStatusID - ANIMAL_STATUS1].bType;
		return GetValue(ITEM_VALUES_MAX_NUM - ANIMAL_STATUS_COUNT + (bStatusID - ANIMAL_STATUS1)) + Animal_GetStatus(bStatusID) * pData->kStat[bStatusID - ANIMAL_STATUS1].sValuePerLevel;
	}
}

void CItem::Animal_GiveBuff(bool bAdd, LPCHARACTER pkTarget)
{
	for (int i = 0; i < 5; ++i)
	{
		Animal_GiveBuff(i, bAdd, pkTarget);
	}
}

void CItem::Animal_GiveBuff(BYTE bStatusID, bool bAdd, LPCHARACTER pkTarget)
{
	if (!pkTarget)
		pkTarget = GetOwner();

	BYTE bApplyType;
	short sApplyVal = Animal_GetApplyStatus(bStatusID, &bApplyType);
	if (bApplyType && sApplyVal)
		pkTarget->ApplyPoint(bApplyType, bAdd ? sApplyVal : -sApplyVal);
}

void CItem::Animal_IncreaseStatus(BYTE bStatusID)
{
	Animal_SetStatus(bStatusID, Animal_GetStatus(bStatusID) + 1);

	const TAnimalLevelData* pData = Animal_GetLevelData();
	if (!pData)
		return;

	if (bStatusID == ANIMAL_STATUS_MAIN)
		GetOwner()->ApplyPoint(pData->kMainStat.bType, pData->kMainStat.sValuePerLevel);
	else
		GetOwner()->ApplyPoint(pData->kStat[bStatusID - ANIMAL_STATUS1].bType, pData->kStat[bStatusID - ANIMAL_STATUS1].sValuePerLevel);
}

BYTE CItem::Animal_GetType() const
{
	switch (GetType())
	{
		case ITEM_MOUNT:
			if (GetValue(1) > CHARACTER::HORSE_MAX_GRADE)
				return ANIMAL_TYPE_MOUNT;
			break;

#ifdef __PET_SYSTEM__
		case ITEM_PET:
			if (GetValue(1) == 1)
				return ANIMAL_TYPE_PET;
			break;
#endif
	}

	return ANIMAL_TYPE_UNKNOWN;
}

void CItem::Animal_SummonPacket(const char* c_pszName)
{
	if (!GetOwner())
		return;

	if (!Animal_IsAnimal())
		return;

	network::GCOutputPacket<network::GCAnimalSummonPacket> pack;
	pack->set_type(Animal_GetType());
	strcpy(pack->name(), c_pszName);
	pack->set_level(Animal_GetLevel());
	pack->set_exp(Animal_GetEXP());
	pack->set_max_exp(Animal_GetMaxEXP());
	pack->set_stat_points(Animal_GetStatusPoints());
	pack->stats()[ANIMAL_STATUS_MAIN] = Animal_GetApplyStatus(ANIMAL_STATUS_MAIN);
	for (int i = 0; i < ANIMAL_STATUS_COUNT; ++i)
		pack->stats()[ANIMAL_STATUS1 + i] = Animal_GetApplyStatus(ANIMAL_STATUS1 + i);

	GetOwner()->GetDesc()->Packet(pack);
}

void CItem::Animal_LevelPacket()
{
	if (!GetOwner())
		return;

	if (!Animal_IsAnimal())
		return;

	network::GCOutputPacket<network::GCAnimalUpdateLevelPacket> pack;
	pack->set_type(Animal_GetType());
	pack->set_level(Animal_GetLevel());
	pack->set_max_exp(Animal_GetMaxEXP());
	pack->set_stat_points(Animal_GetStatusPoints());
	pack->stats()[ANIMAL_STATUS_MAIN] = Animal_GetApplyStatus(ANIMAL_STATUS_MAIN);
	for (int i = 0; i < ANIMAL_STATUS_COUNT; ++i)
		pack->stats()[ANIMAL_STATUS1 + i] = Animal_GetApplyStatus(ANIMAL_STATUS1 + i);

	GetOwner()->GetDesc()->Packet(pack);
}

void CItem::Animal_ExpPacket()
{
	if (!GetOwner())
		return;

	if (!Animal_IsAnimal())
		return;

	network::GCOutputPacket<network::GCAnimalUpdateExpPacket> pack;
	pack->set_type(Animal_GetType());
	pack->set_exp(Animal_GetEXP());

	GetOwner()->GetDesc()->Packet(pack);
}

void CItem::Animal_StatsPacket()
{
	if (!GetOwner())
		return;

	if (!Animal_IsAnimal())
		return;

	network::GCOutputPacket<network::GCAnimalUpdateStatsPacket> pack;
	pack->set_type(Animal_GetType());
	pack->set_stat_points(Animal_GetStatusPoints());
	pack->stats()[ANIMAL_STATUS_MAIN] = Animal_GetApplyStatus(ANIMAL_STATUS_MAIN);
	for (int i = 0; i < ANIMAL_STATUS_COUNT; ++i)
		pack->stats()[ANIMAL_STATUS1 + i] = Animal_GetApplyStatus(ANIMAL_STATUS1 + i);

	GetOwner()->GetDesc()->Packet(pack);
}

void CItem::Animal_UnsummonPacket()
{
	if (!GetOwner())
		return;

	if (!Animal_IsAnimal())
		return;

	network::GCOutputPacket<network::GCAnimalUnsummonPacket> pack;
	pack->set_type(Animal_GetType());

	GetOwner()->GetDesc()->Packet(pack);
}
#endif

bool CItem::CanPutItemIntoShop(bool bMessage)
{
	if (!m_pOwner)
		return false;

	if (IsGMOwner() && !test_server)
	{
		if (bMessage)
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "The item %s is not tradeable."), GetName(m_pOwner->GetLanguageID()));
		return false;
	}

#ifdef __TRADE_BLOCK_SYSTEM__
	if (m_pOwner->IsTradeBlocked())
	{
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "You can't do this now. Please write COMA/GA/DEV/SA a message on Discord or Forum.");
		m_pOwner->ChatPacket(CHAT_TYPE_INFO, "This is a security mesurement against payment fraud. Don't worry it will resolve shortly.");
	}
#endif

	if (IS_SET(GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_MYSHOP))
	{
		if (bMessage)
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "À¯·áÈ­ ¾ÆÀÌÅÛÀº °³ÀÎ»óÁ¡¿¡¼­ ÆÇ¸ÅÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsEquipped() == true)
	{
		if (bMessage)
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "ÀåºñÁßÀÎ ¾ÆÀÌÅÛÀº °³ÀÎ»óÁ¡¿¡¼­ ÆÇ¸ÅÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (true == isLocked())
	{
		if (bMessage)
			m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "»ç¿ëÁßÀÎ ¾ÆÀÌÅÛÀº °³ÀÎ»óÁ¡¿¡¼­ ÆÇ¸ÅÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (GetType() == ITEM_COSTUME)
	{
		if (GetLimitType(0) == LIMIT_REAL_TIME_START_FIRST_USE || GetLimitType(0) == LIMIT_REAL_TIME || GetLimitType(0) == LIMIT_TIMER_BASED_ON_WEAR)
		{
			if (GetSocket(0) > 0 && GetSocket(0)-get_global_time() < GetLimitValue(0))
			{
				if (bMessage)
					m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "This item has a too small amount of wear time left to insert it into the shop."));
				return false;
			}
			else
				m_pOwner->tchat("	if !!(GetSocket(%d) > 0 && GetSocket(%d) < GetLimitValue(%d))", GetSocket(0) ,GetSocket(0), GetLimitValue(0));
		}
	}

	if (GetVnum() == 70043 || GetVnum() == 92196 || GetVnum() == 92197 || (GetVnum() >= 72703 && GetVnum() <= 72714))
	{
		if (GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) < 60)
		{
			if (bMessage)
				m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "This item has a too small amount of wear time left to insert it into the shop."));
			return false;
		}
	}
	if (GetVnum() == 70005 || GetVnum() == UNIQUE_ITEM_75EXP_NEW)
	{
		if (GetSocket(ITEM_SOCKET_UNIQUE_REMAIN_TIME) < 30)
		{
			if (bMessage)
				m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "This item has a too small amount of wear time left to insert it into the shop."));
			return false;
		}
	}

	return true;
}

bool CItem::IsPotionItem(bool bOnlyHP, bool bCheckSpecialPotion) const
{
	if (GetType() == ITEM_USE)
	{
		switch (GetSubType())
		{
		case USE_POTION:
		case USE_POTION_CONTINUE:
		case USE_POTION_NODELAY:
			if (!bOnlyHP || GetValue(0) != 0)
				return true;
			break;
		}
	}

	return false;
}

bool CItem::IsAutoPotionItem(bool bOnlyHP) const
{
	if (GetType() == ITEM_USE)
	{
		if (GetSubType() == USE_SPECIAL)
		{
			switch (GetVnum())
			{
			case ITEM_AUTO_HP_RECOVERY_S:
			case ITEM_AUTO_HP_RECOVERY_M:
			case ITEM_AUTO_HP_RECOVERY_L:
			case ITEM_AUTO_HP_RECOVERY_X:
			case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
#ifdef ENABLE_PERMANENT_POTIONS
			case ITEM_AUTO_HP_RECOVERY_PERMANENT:
#endif
				return true;
				break;

			case ITEM_AUTO_SP_RECOVERY_S:
			case ITEM_AUTO_SP_RECOVERY_M:
			case ITEM_AUTO_SP_RECOVERY_L:
			case ITEM_AUTO_SP_RECOVERY_X:
			case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
#ifdef ENABLE_PERMANENT_POTIONS
			case ITEM_AUTO_SP_RECOVERY_PERMANENT:
#endif
				if (!bOnlyHP)
					return true;
				break;
			}
		}
	}

	return false;
}

#ifdef INFINITY_ITEMS
bool CItem::IsInfinityItem() const
{
	switch (GetVnum())
	{
		case 93366:
		case 93367:
		case 93368:
		case 93369:
		case 93370:
		case 93371:
		case 93360:
		case 93361:
		case 93362:
		case 93363:
		case 93364:
		case 93365:
		case 93273:
		case 50828:
		case 95219:
		case 95220:
		case 95381:
			return true;
	}

	return false;
}
#endif

WORD CItem::GetHorseUsedBottles(BYTE bBonusID)
{
	return GetAttributeValue(bBonusID);
}

void CItem::SetHorseUsedBottles(BYTE bBonusID, WORD wCount)
{
	SetAttribute(bBonusID, GetHorseBonusLevel(bBonusID), wCount);
}

BYTE CItem::GetHorseBonusLevel(BYTE bBonusID)
{
	return GetAttributeType(bBonusID);
}

void CItem::SetHorseBonusLevel(BYTE bBonusID, BYTE bLevel)
{
	SetAttribute(bBonusID, bLevel, GetHorseUsedBottles(bBonusID));
}

BYTE CItem::AcceCostumeGetReceptiveGrade()
{
	return GetSocket(0);
}

#ifdef __ALPHA_EQUIP__
int CItem::GetAlphaEquipMinValue()
{
	return 0;
}

int CItem::GetAlphaEquipMaxValue()
{
	return 0;
}

void CItem::RerollAlphaEquipValue()
{
	int minVal = GetAlphaEquipMinValue();
	int maxVal = GetAlphaEquipMaxValue();
	if (minVal == maxVal)
	{
		SetAlphaEquipValue(minVal);
		return;
	}
	
	if (maxVal < minVal)
		std::swap(minVal, maxVal);

	SetAlphaEquipValue(random_number(minVal, maxVal));
}

bool CItem::UpgradeAlphaEquipValue()
{
	int val = GetAlphaEquipValue();
	int maxVal = GetAlphaEquipMaxValue();

	if (val >= maxVal)
		return false;

	maxVal = MIN(val + 50, maxVal);
	SetAlphaEquipValue(random_number(val + 1, maxVal));

	return true;
}

const int c_iAlphaEquipBit = 31; // 31. bit -> use it as is_alpha_weapon bool; 32. bit is +/- and all bits before the 31. are used for the AlphaEquipValue

void CItem::SetAlphaEquipValue(int iValue)
{
	if (IsAlphaEquip())
		SET_BIT(iValue, c_iAlphaEquipBit);
	else
		REMOVE_BIT(iValue, c_iAlphaEquipBit);

	m_iAlphaEquipValue = iValue;
}

int CItem::GetAlphaEquipValue() const
{
	static const int cs_iRemoveBitFlag = ~(1 << (c_iAlphaEquipBit - 1));
	return m_iAlphaEquipValue & cs_iRemoveBitFlag;
}

void CItem::SetAlphaEquip(bool bIsAlpha)
{
	if (bIsAlpha)
		SET_BIT(m_iAlphaEquipValue, c_iAlphaEquipBit);
	else
		REMOVE_BIT(m_iAlphaEquipValue, c_iAlphaEquipBit);
}

bool CItem::IsAlphaEquip() const
{
	return IS_SET(m_iAlphaEquipValue, c_iAlphaEquipBit);
}
#endif

void CItem::DisableItem()
{
	if (m_bIsDisabledItem)
		return;

	if (IsEquipped())
		ModifyPoints(false);
	m_bIsDisabledItem = true;
}

void CItem::EnableItem()
{
	if (!m_bIsDisabledItem)
		return;

	m_bIsDisabledItem = false;
	if (IsEquipped())
		ModifyPoints(true);
}

bool CItem::IsItemDisabled() const
{
	return m_bIsDisabledItem;
}

bool CItem::IsHelperSpawnItem() const
{
	if (GetType() == ITEM_MOUNT)
		return true;
#ifdef __PET_SYSTEM__
	if (GetType() == ITEM_PET)
		return true;
#endif
#ifdef __FAKE_BUFF__
	if (CItemVnumHelper::IsFakeBuffSpawn(GetVnum()))
		return true;
#endif

	return false;
}

void CItem::SetItemCooltime(int lCooltime)
{
	if (lCooltime)
		SetSocket(1, get_global_time() + lCooltime);
	// else
		// m_dwCooltime = get_global_time();
}

int CItem::GetItemCooltime()
{
	time_t now = get_global_time();
	if (!GetSocket(1) || now >= GetSocket(1))
		return 0;

	return GetSocket(1) - now;
}

/* bool CItem::IsCooltime()
{
	return false;
	// return m_dwCooltime + (60*8) > get_global_time();
} */

bool CItem::IsGMOwner() const
{
	if (test_server)
		return false;
// #ifdef ACCOUNT_TRADE_BLOCK
// 	if (m_pOwner && m_pOwner->GetDesc() && m_pOwner->GetDesc()->IsTradeblocked())
// 	{
// 		m_pOwner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pOwner, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
// 		return true;
// 	}
// #endif
	return m_bIsGMOwner == GM_OWNER_GM;
}

#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
const char * CItem::GetName(BYTE bLanguageID) const
{
	static char szItemName[128];
	*szItemName = '\0';

	if (GetProto())
	{
		int len = 0;
		switch (GetType())
		{
			case ITEM_POLYMORPH:
			{
				const long x = GetSocket(0);
				const CMob* pMob = CMobManager::instance().Get(x);
				if (pMob)
					len = snprintf(szItemName, sizeof(szItemName), "%s", pMob ? pMob->m_table.locale_name(bLanguageID).c_str() : "");

				break;
			}
			case ITEM_SKILLBOOK:
			case ITEM_SKILLFORGET:
			{
				DWORD dwSkillVnum = (GetVnum() == ITEM_SKILLBOOK_VNUM || GetVnum() == ITEM_SKILLFORGET_VNUM) ? GetSocket(0) : 0;
				const CSkillProto* pSkill = (dwSkillVnum != 0) ? CSkillManager::instance().Get(dwSkillVnum) : NULL;
				if (pSkill)
					len = snprintf(szItemName, sizeof(szItemName), "%s", pSkill->szLocaleName[bLanguageID]);

				break;
			}
		}
		len += snprintf(szItemName + len, sizeof(szItemName) - len, (len>0)?" %s":"%s", GetProto() ? GetProto()->locale_name(bLanguageID).c_str() : "");
	}

	return szItemName;
}
#endif
