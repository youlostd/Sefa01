﻿#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "item.h"
#include "item_manager.h"
#include "over9refine.h"
#include "log.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest
{
	//
	// "item" Lua functions
	//

	int item_get_cell(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		if (q.GetCurrentItem())
		{
			lua_pushnumber(L, q.GetCurrentItem()->GetCell());
		}
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	int item_select_cell(lua_State* L)
	{
		lua_pushboolean(L, 0);
		if (!lua_isnumber(L, 1))
		{
			return 1;
		}
		DWORD cell = (DWORD) lua_tonumber(L, 1);

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM item = ch ? ch->GetInventoryItem(cell) : NULL;

		if (!item)
		{
			return 1;
		}

		CQuestManager::instance().SetCurrentItem(item);
		lua_pushboolean(L, 1);

		return 1;
	}

	int item_select(lua_State* L)
	{
		lua_pushboolean(L, 0);
		if (!lua_isnumber(L, 1))
		{
			return 1;
		}
		DWORD id = (DWORD) lua_tonumber(L, 1);
		LPITEM item = ITEM_MANAGER::instance().Find(id);

		if (!item)
		{
			return 1;
		}

		CQuestManager::instance().SetCurrentItem(item);
		lua_pushboolean(L, 1);

		return 1;
	}

	int item_equip(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();
		if (!item)
			return 0;

		LPCHARACTER ch = q.GetCurrentCharacterPtr();

		if (ch && item->GetOwner() == ch)
		{
			int iWearCell = item->FindEquipCell(ch);
			if (iWearCell == -1)
				return 0;

			if (LPITEM pkCurItem = ch->GetWear(iWearCell))
				ch->UnequipItem(pkCurItem);

			ch->EquipItem(item, iWearCell, true);
		}

		return 0;
	}

	int item_get_id(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		if (q.GetCurrentItem())
		{
			lua_pushnumber(L, q.GetCurrentItem()->GetID());
		}
		else
			lua_pushnumber(L, 0);
		return 1;
	}

	int item_remove(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();
		if (item != NULL) {
			if (q.GetCurrentCharacterPtr() == item->GetOwner()) {
				ITEM_MANAGER::instance().RemoveItem(item);
			} else {
				sys_err("Tried to remove invalid item %p", get_pointer(item));
			}
			q.ClearCurrentItem();
		}

		return 0;
	}

	int item_get_socket(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		if (q.GetCurrentItem() && lua_isnumber(L, 1))
		{
			int idx = (int) lua_tonumber(L, 1);
			if (idx < 0 || idx >= ITEM_SOCKET_MAX_NUM)
				lua_pushnumber(L,0);
			else
				lua_pushnumber(L, q.GetCurrentItem()->GetSocket(idx));
		}
		else
		{
			lua_pushnumber(L,0);
		}
		return 1;
	}

	int item_set_socket(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		if (q.GetCurrentItem() && lua_isnumber(L,1) && lua_isnumber(L,2))
		{
			int idx = (int) lua_tonumber(L, 1);
			int value = (int) lua_tonumber(L, 2);
			if (idx >=0 && idx < ITEM_SOCKET_MAX_NUM)
				q.GetCurrentItem()->SetSocket(idx, value);
		}
		return 0;
	}

	int item_get_vnum(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item) 
			lua_pushnumber(L, item->GetVnum());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_is_metin(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushboolean(L, item->GetType() == ITEM_METIN);
		else
			lua_pushboolean(L, false);

		return 1;
	}

	int item_has_flag(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (!lua_isnumber(L, 1))
		{
			sys_err("flag is not a number.");
			lua_pushboolean(L, 0);
			return 1;
		}

		if (!item)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		long lCheckFlag = (long) lua_tonumber(L, 1);	
		lua_pushboolean(L, IS_SET(item->GetFlag(), lCheckFlag));

		return 1;
	}

	int item_get_value(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (!item)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (!lua_isnumber(L, 1))
		{
			sys_err("index is not a number");
			lua_pushnumber(L, 0);
			return 1;
		}

		int index = (int) lua_tonumber(L, 1);

		if (index < 0 || index >= ITEM_VALUES_MAX_NUM)
		{
			sys_err("index(%d) is out of range (0..%d)", index, ITEM_VALUES_MAX_NUM);
			lua_pushnumber(L, 0);
		}
		else
			lua_pushnumber(L, item->GetValue(index));

		return 1;
	}

	int item_set_attr(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (!item)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		if (false == (lua_isnumber(L, 1) && lua_isnumber(L, 2) && lua_isnumber(L, 3)))
		{
			sys_err("index is not a number");
			lua_pushnumber(L, 0);
			return 1;
		}

		item->SetForceAttribute(
			lua_tonumber(L, 1),		// index
			lua_tonumber(L, 2),		// apply type
			lua_tonumber(L, 3)		// apply value
			);

		return 0;
	}

	int item_get_attr(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (!item)
		{
			lua_pushnumber(L, 0);
			lua_pushnumber(L, 0);
			return 2;
		}

		if (false == lua_isnumber(L, 1) || lua_tonumber(L, 1) < 0 || lua_tonumber(L, 1) >= ITEM_ATTRIBUTE_MAX_NUM)
		{
			sys_err("index is not a number or out of range");
			lua_pushnumber(L, 0);
			lua_pushnumber(L, 0);
			return 2;
		}

		lua_pushnumber(L, item->GetAttribute(lua_tonumber(L, 1)).type());
		lua_pushnumber(L, item->GetAttribute(lua_tonumber(L, 1)).value());

		return 2;
	}

	int item_get_name(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER pkChr = q.GetCurrentCharacterPtr();
		LPITEM item = q.GetCurrentItem();

		if (item)
		{
			if (!lua_isboolean(L, 1) || lua_toboolean(L, 1))
				lua_pushstring(L, CLocaleManager::instance().StringToArgument(item->GetName(pkChr ? pkChr->GetLanguageID() : 0)));
			else
				lua_pushstring(L, item->GetName(pkChr ? pkChr->GetLanguageID() : 0));
		}
		else
			lua_pushstring(L, "");

		return 1;
	}

	int item_get_size(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetSize());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_get_count(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetCount());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_set_count(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("unkown argument");
			return 0;
		}

		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			item->SetCount(lua_tonumber(L, 1));

		return 0;
	}

	int item_get_type(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetType());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_get_sub_type(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetSubType());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_get_refine_vnum(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetRefinedVnum());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_next_refine_vnum(lua_State* L)
	{
		DWORD vnum = 0;
		if (lua_isnumber(L, 1))
			vnum = (DWORD) lua_tonumber(L, 1);

		auto pTable = ITEM_MANAGER::instance().GetTable(vnum);
		if (pTable)
		{
			lua_pushnumber(L, pTable->refined_vnum());
		}
		else
		{
			sys_err("Cannot find item table of vnum %u", vnum);
			lua_pushnumber(L, 0);
		}
		return 1;
	}

#ifdef SUNDAE_EVENT
	int item_find(lua_State* L)
	{
		if (!lua_isnumber(L, 1))
		{
			sys_err("wrong argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!ch)
		{
			lua_pushnumber(L, 0);
			return 1;
		}

		DWORD vnum = (DWORD)lua_tonumber(L, 1);
		LPITEM findItem = ch->FindSpecifyItem(vnum);

		if (findItem)
		{
			lua_pushnumber(L, findItem->GetID());
			return 1;
		}

		lua_pushnumber(L, 0);
		return 1;
	}
#endif

	int item_get_level(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetRefineLevel());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_can_over9refine(lua_State* L)
	{
		LPITEM item = CQuestManager::instance().GetCurrentItem();

		if ( item == NULL ) return 0;

		lua_pushnumber(L, COver9RefineManager::instance().canOver9Refine(item->GetVnum()));

		return 1;
	}

	int item_change_to_over9(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM item = CQuestManager::instance().GetCurrentItem();

		if ( ch == NULL || item == NULL ) return 0;

		lua_pushboolean(L, COver9RefineManager::instance().Change9ToOver9(ch, item));
		
		return 1;
	}
	
	int item_over9refine(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPITEM item = CQuestManager::instance().GetCurrentItem();

		if ( ch == NULL || item == NULL ) return 0;

		lua_pushboolean(L, COver9RefineManager::instance().Over9Refine(ch, item));
		
		return 1;
	}

	int item_get_over9_material_vnum(lua_State* L)
	{
		if ( lua_isnumber(L, 1) == true )
		{
			lua_pushnumber(L, COver9RefineManager::instance().GetMaterialVnum((DWORD)lua_tonumber(L, 1)));
			return 1;
		}

		return 0;
	}

	int item_get_level_limit (lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();

		if (q.GetCurrentItem())
		{
			if (q.GetCurrentItem()->GetType() != ITEM_WEAPON && q.GetCurrentItem()->GetType() != ITEM_ARMOR)
			{
				return 0;
			}
			lua_pushnumber(L, q.GetCurrentItem() -> GetLevelLimit());
			return 1;
		}
		return 0;
	}

	int item_start_realtime_expire(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM pItem = q.GetCurrentItem();

		if (pItem)
		{
			pItem->StartRealTimeExpireEvent();
			return 1;
		}

		return 0;
	}
	
	int item_copy_and_give_before_remove(lua_State* L)
	{
		lua_pushboolean(L, 0);
		if (!lua_isnumber(L, 1))
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		DWORD vnum = (DWORD)lua_tonumber(L, 1);

		CQuestManager& q = CQuestManager::instance();
		LPITEM pItem = q.GetCurrentItem();
		LPCHARACTER pChar = q.GetCurrentCharacterPtr();
		if (!pItem || !pChar)
		{
			lua_pushboolean(L, 0);
			return 1;
		}

		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(pItem, pkNewItem);
			LogManager::instance().ItemLog(pChar, pkNewItem, "COPY SUCCESS", pkNewItem->GetName());

			WORD wCell = pItem->GetCell();

			ITEM_MANAGER::instance().RemoveItem(pItem, "REMOVE (COPY SUCCESS)");

			pkNewItem->AddToCharacter(pChar, TItemPos(INVENTORY, wCell)); 
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();

			// ¼º°ø!
			lua_pushboolean(L, 1);			
		}

		return 1;
	}

	int item_get_refine_level(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushnumber(L, item->GetRefineLevel());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int item_check_owner(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPCHARACTER ch = q.GetCurrentCharacterPtr();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushboolean(L, ch && item->GetOwner() == ch);
		else
			lua_pushboolean(L, false);

		return 1;
	}


	int item_is_gm_item(lua_State* L)
	{
		CQuestManager& q = CQuestManager::instance();
		LPITEM item = q.GetCurrentItem();

		if (item)
			lua_pushboolean(L, item->IsGMOwner());
		else
			lua_pushboolean(L, false);

		return 1;
	}

	void RegisterITEMFunctionTable()
	{

		luaL_reg item_functions[] =
		{
			{ "get_id",		item_get_id		},
			{ "get_cell",		item_get_cell		},
			{ "select",		item_select		},
			{ "select_cell",	item_select_cell	},
			{ "remove",		item_remove		},
			{ "get_socket",		item_get_socket		},
			{ "set_socket",		item_set_socket		},
			{ "get_vnum",		item_get_vnum		},
			{ "is_metin",		item_is_metin		},
			{ "has_flag",		item_has_flag		},
			{ "get_value",		item_get_value		},
			{ "set_attr",		item_set_attr		},
			{ "get_attr",		item_get_attr		},
			{ "get_name",		item_get_name		},
			{ "get_size",		item_get_size		},
			{ "get_count",		item_get_count		},
			{ "set_count",		item_set_count		},
			{ "get_type",		item_get_type		},
			{ "get_sub_type",	item_get_sub_type	},
			{ "get_refine_vnum",	item_get_refine_vnum	},
			{ "get_level",		item_get_level		},
			{ "next_refine_vnum",	item_next_refine_vnum	},
			{ "can_over9refine",	item_can_over9refine	},
			{ "change_to_over9",		item_change_to_over9	},
			{ "over9refine",		item_over9refine	},
			{ "get_over9_material_vnum",		item_get_over9_material_vnum	},
			{ "get_level_limit", 				item_get_level_limit },
			{ "start_realtime_expire", 			item_start_realtime_expire },
			{ "copy_and_give_before_remove",	item_copy_and_give_before_remove},
			{ "get_refine_level",	item_get_refine_level	},
			{ "check_owner",	item_check_owner	},
			{ "is_gm_item",		item_is_gm_item		},
			{ "equip",			item_equip },
#ifdef SUNDAE_EVENT
			{ "find",			item_find },
#endif

			{ NULL,			NULL			}
		};
		CQuestManager::instance().AddLuaFunctionTable("item", item_functions);
	}
}
