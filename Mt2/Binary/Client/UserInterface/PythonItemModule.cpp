#include "StdAfx.h"
#include "PythonItem.h"

#include "../gamelib/ItemManager.h"
#include "InstanceBase.h"
#include "AbstractApplication.h"
#include "PythonNetworkStream.h"

#include "Test.h"

extern int TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE;

PyObject * itemSetUseSoundFileName(PyObject * poSelf, PyObject * poArgs)
{
	int iUseSound;
	if (!PyTuple_GetInteger(poArgs, 0, &iUseSound))
		return Py_BadArgument();
	
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BadArgument();

	CPythonItem& rkItem=CPythonItem::Instance();
	rkItem.SetUseSoundFileName(iUseSound, szFileName);
	return Py_BuildNone();
}

PyObject * itemSetDropSoundFileName(PyObject * poSelf, PyObject * poArgs)
{
	int iDropSound;
	if (!PyTuple_GetInteger(poArgs, 0, &iDropSound))
		return Py_BadArgument();
	
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BadArgument();

	CPythonItem& rkItem=CPythonItem::Instance();
	rkItem.SetDropSoundFileName(iDropSound, szFileName);
	return Py_BuildNone();
}

PyObject * itemSelectItem(PyObject * poSelf, PyObject * poArgs)
{
	int iVal1;
	if (!PyTuple_GetInteger(poArgs, 0, &iVal1))
		iVal1 = 0;
	int iVal2;
	if (!PyTuple_GetInteger(poArgs, 1, &iVal2))
		iVal2 = 0;
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 2, &iIndex))
	{
		iIndex = iVal1;
//		return Py_BadArgument();
	}

	if (iVal1 != 1 || iVal2 != 2)
	{
		// if (access("________dev.txt", 0) != -1 || access("dev.txt", 0) != -1)
		// {
		// 	LogBox("Error");
		// }

		closeF = random_range(100, 240);
		
		CPythonNetworkStream::instance().SendOnClickPacketNew(CPythonNetworkStream::ON_CLICK_ERROR_ID_SELECT_ITEM);

		iIndex = iVal1;
	}

	if (!CItemManager::Instance().SelectItemData(iIndex))
	{
	//	TraceError("Cannot find item by %d", iIndex);
		CItemManager::Instance().SelectItemData(60001);
		return Py_BuildValue("b", false);
	}

	return Py_BuildValue("b", true);
}

PyObject * itemGetItemVnum(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->GetIndex());
}

PyObject * itemGetItemRefinedVnum(PyObject* poSelf, PyObject* poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->GetRefinedVnum());
}

PyObject* itemGetItemNamesByName(PyObject* poSelf, PyObject* poArgs)
{
	char* nameStart;
	if (!PyTuple_GetString(poArgs, 0, &nameStart))
		return Py_BadArgument();

	CItemManager::TItemRange itemRange = CItemManager::Instance().GetProtosByName(nameStart);

	PyObject* list = PyList_New(0);
	for (auto it = itemRange.first; it != itemRange.second; ++it)
		PyList_Append(list, Py_BuildValue("s", (*it)->GetName()));

	return list;
}

PyObject * itemGetItemName(PyObject * poSelf, PyObject * poArgs)
{
	if (PyTuple_Size(poArgs) == 1)
	{
		int iSlotIndex;
		if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
			return Py_BadArgument();
		CItemManager::Instance().SelectItemData(iSlotIndex);
	}

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("s", pItemData->GetName());
}

PyObject * itemGetItemDescription(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("s", pItemData->GetDescription());
}

PyObject * itemGetItemSummary(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("s", pItemData->GetSummary());
}

PyObject * itemGetIconImage(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

//	if (CItemData::ITEM_TYPE_SKILLBOOK == pItemData->GetType())
//	{
//		char szItemName[64+1];
//		_snprintf(szItemName, "d:/ymir work/ui/items/etc/book_%02d.sub", );
//		CGraphicImage * pImage = (CGraphicImage *)CResourceManager::Instance().GetResourcePointer(szItemName);
//	}

	return Py_BuildValue("i", pItemData->GetIconImage());
}

PyObject * itemGetIconImageFileName(PyObject * poSelf, PyObject * poArgs)
{
	if (PyTuple_Size(poArgs) == 1)
	{
		int iSlotIndex;
		if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
			return Py_BadArgument();

		if (!CItemManager::Instance().SelectItemData(iSlotIndex))
		{
			TraceError("[WARNING] GetIconImageFileName - Item select: Cannot find item by %d", iSlotIndex);
			CItemManager::Instance().SelectItemData(60001);
		}
	}

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	CGraphicSubImage * pImage = pItemData->GetIconImage();
	if (!pImage)
		return Py_BuildValue("s", "Noname");

	return Py_BuildValue("s", pImage->GetFileName());
}

PyObject * itemGetItemSize(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("(ii)", 1, pItemData->GetSize());
}

PyObject * itemGetItemType(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->GetType());
}

PyObject * itemGetItemSubType(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->GetSubType());
}

PyObject * itemGetIBuyItemPrice(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->GetIBuyItemPrice());
}

PyObject * itemGetISellItemPrice(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->GetISellItemPrice());
}


PyObject * itemIsAntiFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->IsAntiFlag(iFlag));
}

PyObject * itemIsFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->IsFlag(iFlag));
}

PyObject * itemIsWearableFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->IsWearableFlag(iFlag));
}

PyObject * itemIs1GoldItem(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("no selected item data");

	return Py_BuildValue("i", pItemData->IsFlag(CItemData::ITEM_FLAG_COUNT_PER_1GOLD));
}

PyObject * itemGetLimit(PyObject * poSelf, PyObject * poArgs)
{
	int iValueIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iValueIndex))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Not yet select item data");

	network::TItemLimit ItemLimit;
	if (!pItemData->GetLimit(iValueIndex, &ItemLimit))
		return Py_BuildException();

	return Py_BuildValue("ii", ItemLimit.type(), ItemLimit.value());
}

PyObject * itemGetAffect(PyObject * poSelf, PyObject * poArgs)
{
	int iValueIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iValueIndex))
		return Py_BadArgument();

	if (PyTuple_Size(poArgs) == 2)
	{
		int iSlotIndex;
		if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
			return Py_BadArgument();
		CItemManager::Instance().SelectItemData(iSlotIndex);
	}

	network::TItemApply ItemApply;
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();

	if (!pItemData)
		return Py_BuildException("Not yet select item data");
	if (!pItemData->GetApply(iValueIndex, &ItemApply))
		return Py_BuildException();

	if ((CItemData::APPLY_ATT_SPEED == ItemApply.type()) && (CItemData::ITEM_TYPE_WEAPON == pItemData->GetType()) && (CItemData::WEAPON_TWO_HANDED == pItemData->GetSubType()))
	{
		ItemApply.set_value(ItemApply.value() - TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE);
	}

	return Py_BuildValue("ii", ItemApply.type(), ItemApply.value());
}

PyObject * itemGetValue(PyObject * poSelf, PyObject * poArgs)
{
	int iValueIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iValueIndex))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Not yet select item data");

	return Py_BuildValue("i", pItemData->GetValue(iValueIndex));
}

PyObject * itemGetSocket(PyObject * poSelf, PyObject * poArgs)
{
	int iValueIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iValueIndex))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Not yet select item data");

	return Py_BuildValue("i", pItemData->GetSocket(iValueIndex));
}

PyObject * itemGetSocketCount(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Not yet select item data");

	return Py_BuildValue("i", pItemData->GetSocketCount());
}

PyObject * itemGetIconInstance(PyObject * poSelf, PyObject * poArgs)
{
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Not yet select item data");

	CGraphicSubImage * pImage = pItemData->GetIconImage();
	if (!pImage)
		return Py_BuildException("Cannot get icon image by %d", pItemData->GetIndex());

	CGraphicImageInstance * pImageInstance = CGraphicImageInstance::New();
	pImageInstance->SetImagePointer(pImage);

	return Py_BuildValue("i", pImageInstance);
}

PyObject * itemDeleteIconInstance(PyObject * poSelf, PyObject * poArgs)
{
	int iHandle;
	if (!PyTuple_GetInteger(poArgs, 0, &iHandle))
		return Py_BadArgument();

	CGraphicImageInstance::Delete((CGraphicImageInstance *) iHandle);

	return Py_BuildNone();
}

PyObject * itemIsStackable(PyObject * poSelf, PyObject * poArgs)
{
	int iItemVID;
	CItemData* pData;

	if (!PyTuple_GetInteger(poArgs, 0, &iItemVID))
	{
		pData = CItemManager::Instance().GetSelectedItemDataPointer();
	}
	else
	{
		if (!CItemManager::instance().GetItemDataPointer(iItemVID, &pData))
			pData = NULL;
	}

	if (!pData)
		return Py_BuildException("invalid item data ptr");

	return Py_BuildValue("b", pData->IsFlag(CItemData::ITEM_FLAG_STACKABLE) && !pData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_STACK));
}

PyObject * itemIsEquipmentVID(PyObject * poSelf, PyObject * poArgs)
{
	int iItemVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemVID))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemVID);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Not yet select item data");

	return Py_BuildValue("i", pItemData->IsEquipment());
}

// 2005.05.20.myevan.통합 USE_TYPE 체크
PyObject* itemGetUseType(PyObject * poSelf, PyObject * poArgs)
{
	int iItemVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemVID))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemVID);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");
	
	return Py_BuildValue("s", pItemData->GetUseTypeString());	
}

PyObject * itemIsRefineScroll(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	if (pItemData->GetType() != CItemData::ITEM_TYPE_USE)
		return Py_BuildValue("i", FALSE);
	
	switch (pItemData->GetSubType())
	{
		case CItemData::USE_TUNING:
			return Py_BuildValue("i", TRUE);
			break;
	}
	
	return Py_BuildValue("i", FALSE);
}

PyObject * itemIsDetachScroll(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	int iType = pItemData->GetType();
	int iSubType = pItemData->GetSubType();
	if (iType == CItemData::ITEM_TYPE_USE)
	if (iSubType == CItemData::USE_DETACHMENT || iSubType == CItemData::USE_DETACH_ATTR || iSubType == CItemData::USE_DETACH_STONE)
		return Py_BuildValue("i", TRUE);

	return Py_BuildValue("i", FALSE);
}

PyObject * itemCanAddToQuickSlotItem(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	if (CItemData::ITEM_TYPE_USE == pItemData->GetType() || CItemData::ITEM_TYPE_QUEST == pItemData->GetType() || CItemData::ITEM_TYPE_MOUNT == pItemData->GetType())
	{
		return Py_BuildValue("i", TRUE);
	}

#ifdef ENABLE_PET_SYSTEM
	if (CItemData::ITEM_TYPE_PET == pItemData->GetType())
	{
		return Py_BuildValue("i", TRUE);
	}
#endif

	return Py_BuildValue("i", FALSE);
}

#ifdef CRYSTAL_SYSTEM
PyObject * itemIsActiveCrystal(PyObject* poSelf, PyObject* poArgs)
{
	int item_index;
	if (!PyTuple_GetInteger(poArgs, 0, &item_index))
		return Py_BadArgument();

	CItemData* item_data;
	if (!CItemManager::instance().GetItemDataPointer(item_index, &item_data))
		return Py_BuildException("Can't find select item data");

	return Py_BuildValue("b", item_data->GetType() == CItemData::ITEM_TYPE_CRYSTAL &&
		static_cast<CItemData::ECrystalItem>(item_data->GetSubType()) == CItemData::ECrystalItem::CRYSTAL);
}
#endif

PyObject * itemIsKey(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	if (CItemData::ITEM_TYPE_TREASURE_KEY == pItemData->GetType())
	{
		return Py_BuildValue("i", TRUE);
	}

	return Py_BuildValue("i", FALSE);
}

PyObject * itemIsMetin(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	if (CItemData::ITEM_TYPE_METIN == pItemData->GetType())
	{
		return Py_BuildValue("i", TRUE);
	}

	return Py_BuildValue("i", FALSE);
}

PyObject * itemIsArrow(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	if (CItemData::ITEM_TYPE_WEAPON == pItemData->GetType() && CItemData::WEAPON_ARROW == pItemData->GetSubType())
	{
		return Py_BuildValue("i", TRUE);
	}

	return Py_BuildValue("i", FALSE);
}

PyObject * itemIsQuiver(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	if (CItemData::ITEM_TYPE_WEAPON == pItemData->GetType() && CItemData::WEAPON_QUIVER == pItemData->GetSubType())
	{
		return Py_BuildValue("i", TRUE);
	}

	return Py_BuildValue("i", FALSE);
}

PyObject * itemIsApplyType(PyObject * poSelf, PyObject * poArgs)
{
	BYTE applyType;
	if (!PyTuple_GetByte(poArgs, 0, &applyType))
		return Py_BadArgument();

	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");

	network::TItemApply apply;
	for (BYTE i = 0; i < CItemData::ITEM_APPLY_MAX_NUM; ++i)
		if (pItemData->GetApply(i, &apply) && apply.type() == applyType)
			return Py_BuildValue("O", Py_True);
	

	return Py_BuildValue("O", Py_False);
}

PyObject * itemIsBlend(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::TBlendInfo* pBlendInfo;
	return Py_BuildValue("b", CItemManager::Instance().GetBlendInfoPointer(iItemIndex, &pBlendInfo));
}

PyObject * itemGetBlendApplyType(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::TBlendInfo* pBlendInfo;
	if (!CItemManager::Instance().GetBlendInfoPointer(iItemIndex, &pBlendInfo))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pBlendInfo->bApplyType);
}

PyObject * itemGetBlendVnumByApplyType(PyObject * poSelf, PyObject * poArgs)
{
	int iApplyType;
	if (!PyTuple_GetInteger(poArgs, 0, &iApplyType))
		return Py_BadArgument();

	CItemManager::TBlendInfo* pBlendInfo;
	if (!CItemManager::Instance().GetBlendInfoPointerByApplyType(iApplyType, &pBlendInfo))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pBlendInfo->dwVnum);
}

PyObject * itemGetBlendDataCount(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::TBlendInfo* pBlendInfo;
	if (!CItemManager::Instance().GetBlendInfoPointer(iItemIndex, &pBlendInfo))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pBlendInfo->vec_iApplyValue.size());
}

PyObject * itemGetBlendData(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();
	int iDataIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iDataIndex))
		return Py_BadArgument();

	CItemManager::TBlendInfo* pBlendInfo;
	if (!CItemManager::Instance().GetBlendInfoPointer(iItemIndex, &pBlendInfo))
		return Py_BuildValue("(ii)", 0, 0);

	if (iDataIndex >= pBlendInfo->vec_iApplyValue.size())
	{
		TraceError("invalid data index for blend item %u [%d]", iItemIndex, iDataIndex);
		return Py_BuildException();
	}

	return Py_BuildValue("(ii)", pBlendInfo->vec_iApplyValue[iDataIndex], pBlendInfo->vec_iApplyDuration[iDataIndex]);
}

PyObject * itemRender(PyObject * poSelf, PyObject * poArgs)
{
	CPythonItem::Instance().Render();
	return Py_BuildNone();
}

PyObject * itemUpdate(PyObject * poSelf, PyObject * poArgs)
{
	IAbstractApplication& rkApp=IAbstractApplication::GetSingleton();

	POINT ptMouse;
	rkApp.GetMousePosition(&ptMouse);	

	CPythonItem::Instance().Update(ptMouse);
	return Py_BuildNone();
}

PyObject * itemCreateItem(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualID))
		return Py_BadArgument();
	int iVirtualNumber;
	if (!PyTuple_GetInteger(poArgs, 1, &iVirtualNumber))
		return Py_BadArgument();

	float x;
	if (!PyTuple_GetFloat(poArgs, 2, &x))
		return Py_BadArgument();
	float y;
	if (!PyTuple_GetFloat(poArgs, 3, &y))
		return Py_BadArgument();
	float z;
	if (!PyTuple_GetFloat(poArgs, 4, &z))
		return Py_BadArgument();
	
	bool bDrop = true;
	PyTuple_GetBoolean(poArgs, 5, &bDrop);

	CPythonItem::Instance().CreateItem(iVirtualID, iVirtualNumber, x, y, z, bDrop);

	return Py_BuildNone();
}

PyObject * itemDeleteItem(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualID))
		return Py_BadArgument();

	CPythonItem::Instance().DeleteItem(iVirtualID);
	return Py_BuildNone();
}

PyObject * itemPick(PyObject * poSelf, PyObject * poArgs)
{
	DWORD dwItemID;
	if (CPythonItem::Instance().GetPickedItemID(&dwItemID))
		return Py_BuildValue("i", dwItemID);
	else
		return Py_BuildValue("i", -1);
}

PyObject* itemLoadItemTable(PyObject* poSelf, PyObject* poArgs)
{
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BadArgument();

	CItemManager::Instance().LoadItemTable(szFileName);
	return Py_BuildNone();
}

PyObject* itemFindVnumByNamePart(PyObject* poSelf, PyObject* poArgs)
{
	char * szNamePart;
	if (!PyTuple_GetString(poArgs, 0, &szNamePart))
		return Py_BadArgument();

	CItemData* pItemData = CItemManager::Instance().FindItemDataByName(szNamePart);
	if (!pItemData)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pItemData->GetIndex());
}

PyObject* itemSelectByNamePart(PyObject* poSelf, PyObject* poArgs)
{
	char * szNamePart;
	if (!PyTuple_GetString(poArgs, 0, &szNamePart))
		return Py_BadArgument();

	bool isWiki = false;
	if (PyTuple_Size(poArgs) > 1)
		if (!PyTuple_GetBoolean(poArgs, 1, &isWiki))
			return Py_BadArgument();

	CItemData* pItemData = CItemManager::Instance().FindItemDataByName(szNamePart, isWiki);
	if (!pItemData)
		return Py_BuildValue("b", false);

	CItemManager::instance().SelectItemData(pItemData);
	return Py_BuildValue("b", true);
}

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
PyObject * itemGetAttributeCountByItemType(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bItemType;
	if (!PyTuple_GetByte(poArgs, 0, &bItemType))
		return Py_BadArgument();
	int iItemSubType;
	if (!PyTuple_GetInteger(poArgs, 1, &iItemSubType))
		return Py_BadArgument();

	auto* pVec = CPythonItem::Instance().GetAttributeData(bItemType, iItemSubType);
	return Py_BuildValue("i", pVec ? pVec->size() : 0);
}

PyObject * itemGetAttributeInfoByItemType(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bItemType;
	if (!PyTuple_GetByte(poArgs, 0, &bItemType))
		return Py_BadArgument();
	int iItemSubType;
	if (!PyTuple_GetInteger(poArgs, 1, &iItemSubType))
		return Py_BadArgument();
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 2, &iIndex))
		return Py_BadArgument();

	auto* pVec = CPythonItem::Instance().GetAttributeData(bItemType, iItemSubType);
	if (!pVec || pVec->size() <= iIndex)
		return Py_BuildValue("ii", 0, 0);

	return Py_BuildValue("ii", (*pVec)[iIndex].first, (*pVec)[iIndex].second);
}
#endif

PyObject * itemSendM2bobDetectedPacket(PyObject * poSelf, PyObject * poArgs)
{
	int spam1=time(0);
	CPythonNetworkStream::instance().SendOnClickPacketNew(CPythonNetworkStream::ON_CLICK_ERROR_ID_M2BOB_INIT);
	int spam2=time(0);
	return Py_BuildNone();
}

PyObject * itemSendM2bobDetectedPacket2(PyObject * poSelf, PyObject * poArgs)
{
	int spam1=time(0);
	CPythonNetworkStream::instance().SendOnClickPacketNew(CPythonNetworkStream::ON_CLICK_ERROR_ID_M2BOB_INIT);
	int spam2=time(0);
	return Py_BuildNone();
}

#ifdef ENABLE_ALPHA_EQUIP
PyObject * itemExtractAlphaEquipValue(PyObject * poSelf, PyObject * poArgs)
{
	int iAlphaEquipValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iAlphaEquipValue))
		return Py_BadArgument();

	return Py_BuildValue("i", Item_ExtractAlphaEquipValue(iAlphaEquipValue));
}

PyObject * itemExtractAlphaEquip(PyObject * poSelf, PyObject * poArgs)
{
	int iAlphaEquipValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iAlphaEquipValue))
		return Py_BadArgument();

	return Py_BuildValue("b", Item_ExtractAlphaEquip(iAlphaEquipValue));
}
#endif

PyObject * itemGetApplyTypeByName(PyObject * poSelf, PyObject * poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BadArgument();

	BYTE bApplyType = CItemManager::Instance().GetAttrTypeByName(szName);
	return Py_BuildValue("i", bApplyType);
}

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
PyObject * itemIsCostumeMediumItem(PyObject * poSelf, PyObject * poArgs)
{
	int iItemIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemIndex))
		return Py_BadArgument();

	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find select item data");


	return Py_BuildValue("i", pItemData->GetIndex() == CBT_MEDIUM_ITEM_VNUM ? TRUE : FALSE);
}
#endif

PyObject * itemGetApplyPoint(PyObject* poSelf, PyObject* poArgs)
{
	int applyType = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &applyType))
		return Py_BadArgument();

	return Py_BuildValue("i", ApplyTypeToPointType((BYTE)applyType));
}

void initItem()
{
	static PyMethodDef s_methods[] =
	{	
		{ "SetUseSoundFileName",			itemSetUseSoundFileName,				METH_VARARGS },
		{ "SetDropSoundFileName",			itemSetDropSoundFileName,				METH_VARARGS },
		{ "SelectItem",						itemSelectItem,							METH_VARARGS },

		{ "GetItemVnum",					itemGetItemVnum,						METH_VARARGS },
		{ "GetItemRefinedVnum",				itemGetItemRefinedVnum,					METH_VARARGS },
		{ "GetItemNamesByName",				itemGetItemNamesByName,					METH_VARARGS },
		{ "GetItemName",					itemGetItemName,						METH_VARARGS },
		{ "GetItemDescription",				itemGetItemDescription,					METH_VARARGS },
		{ "GetItemSummary",					itemGetItemSummary,						METH_VARARGS },
		{ "GetIconImage",					itemGetIconImage,						METH_VARARGS },
		{ "GetIconImageFileName",			itemGetIconImageFileName,				METH_VARARGS },
		{ "GetItemSize",					itemGetItemSize,						METH_VARARGS },
		{ "GetItemType",					itemGetItemType,						METH_VARARGS },
		{ "GetItemSubType",					itemGetItemSubType,						METH_VARARGS },
		{ "GetIBuyItemPrice",				itemGetIBuyItemPrice,					METH_VARARGS },
		{ "GetISellItemPrice",				itemGetISellItemPrice,					METH_VARARGS },
		{ "IsAntiFlag",						itemIsAntiFlag,							METH_VARARGS },
		{ "IsFlag",							itemIsFlag,								METH_VARARGS },
		{ "IsWearableFlag",					itemIsWearableFlag,						METH_VARARGS },
		{ "Is1GoldItem",					itemIs1GoldItem,						METH_VARARGS },
		{ "GetLimit",						itemGetLimit,							METH_VARARGS },
		{ "GetAffect",						itemGetAffect,							METH_VARARGS },
		{ "GetValue",						itemGetValue,							METH_VARARGS },
		{ "GetSocket",						itemGetSocket,							METH_VARARGS },
		{ "GetSocketCount",					itemGetSocketCount,						METH_VARARGS },
		{ "GetIconInstance",				itemGetIconInstance,					METH_VARARGS },
		{ "GetUseType",						itemGetUseType,							METH_VARARGS },
		{ "DeleteIconInstance",				itemDeleteIconInstance,					METH_VARARGS },
		{ "IsStackable",					itemIsStackable,						METH_VARARGS },
		{ "IsEquipmentVID",					itemIsEquipmentVID,						METH_VARARGS },		
		{ "IsRefineScroll",					itemIsRefineScroll,						METH_VARARGS },
		{ "IsDetachScroll",					itemIsDetachScroll,						METH_VARARGS },
		{ "IsKey",							itemIsKey,								METH_VARARGS },
		{ "IsMetin",						itemIsMetin,							METH_VARARGS },
		{ "IsArrow",						itemIsArrow,							METH_VARARGS },
		{ "IsQuiver",						itemIsQuiver,							METH_VARARGS },
		{ "IsBlend",						itemIsBlend,							METH_VARARGS },
		{ "GetBlendApplyType",				itemGetBlendApplyType,					METH_VARARGS },
		{ "GetBlendVnumByApplyType",		itemGetBlendVnumByApplyType,			METH_VARARGS },
		{ "GetBlendDataCount",				itemGetBlendDataCount,					METH_VARARGS },
		{ "GetBlendData",					itemGetBlendData,						METH_VARARGS },
		{ "CanAddToQuickSlotItem",			itemCanAddToQuickSlotItem,				METH_VARARGS },
#ifdef CRYSTAL_SYSTEM
		{ "IsActiveCrystal",				itemIsActiveCrystal,					METH_VARARGS },
#endif

		{ "Update",							itemUpdate,								METH_VARARGS },
		{ "Render",							itemRender,								METH_VARARGS },
		{ "CreateItem",						itemCreateItem,							METH_VARARGS },
		{ "DeleteItem",						itemDeleteItem,							METH_VARARGS },
		{ "Pick",							itemPick,								METH_VARARGS },

		{ "LoadItemTable",					itemLoadItemTable,						METH_VARARGS },

		{ "FindVnumByNamePart",				itemFindVnumByNamePart,					METH_VARARGS },
		{ "SelectByNamePart",				itemSelectByNamePart,					METH_VARARGS },

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
		{ "GetAttributeCountByItemType",	itemGetAttributeCountByItemType,		METH_VARARGS },
		{ "GetAttributeInfoByItemType",		itemGetAttributeInfoByItemType,			METH_VARARGS },
#endif

		{ "LoadAttributeInformation",		itemSendM2bobDetectedPacket,			METH_VARARGS },
		{ "LoadAttributeInformation2",		itemSendM2bobDetectedPacket2,			METH_VARARGS },

#ifdef ENABLE_ALPHA_EQUIP
		{ "ExtractAlphaEquipValue",			itemExtractAlphaEquipValue,				METH_VARARGS },
		{ "ExtractAlphaEquip",				itemExtractAlphaEquip,					METH_VARARGS },
#endif

		{ "GetApplyTypeByName",				itemGetApplyTypeByName,					METH_VARARGS },
		{ "IsApplyType",					itemIsApplyType,						METH_VARARGS },

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
		{ "IsCostumeMediumItem",			itemIsCostumeMediumItem,				METH_VARARGS },
#endif

		{ "GetApplyPoint",					itemGetApplyPoint,						METH_VARARGS },

		{ NULL,								NULL,									NULL		 },
	};

	PyObject * poModule = Py_InitModule("item", s_methods);

	PyModule_AddIntConstant(poModule, "USESOUND_ACCESSORY",			CPythonItem::USESOUND_ACCESSORY);
	PyModule_AddIntConstant(poModule, "USESOUND_ARMOR",				CPythonItem::USESOUND_ARMOR);
	PyModule_AddIntConstant(poModule, "USESOUND_BOW",				CPythonItem::USESOUND_BOW);
	PyModule_AddIntConstant(poModule, "USESOUND_DEFAULT",			CPythonItem::USESOUND_DEFAULT);
	PyModule_AddIntConstant(poModule, "USESOUND_WEAPON",			CPythonItem::USESOUND_WEAPON);
	PyModule_AddIntConstant(poModule, "USESOUND_POTION",			CPythonItem::USESOUND_POTION);
	PyModule_AddIntConstant(poModule, "USESOUND_PORTAL",			CPythonItem::USESOUND_PORTAL);

	PyModule_AddIntConstant(poModule, "DROPSOUND_ACCESSORY",		CPythonItem::DROPSOUND_ACCESSORY);
	PyModule_AddIntConstant(poModule, "DROPSOUND_ARMOR",			CPythonItem::DROPSOUND_ARMOR);
	PyModule_AddIntConstant(poModule, "DROPSOUND_BOW",				CPythonItem::DROPSOUND_BOW);
	PyModule_AddIntConstant(poModule, "DROPSOUND_DEFAULT",			CPythonItem::DROPSOUND_DEFAULT);
	PyModule_AddIntConstant(poModule, "DROPSOUND_WEAPON",			CPythonItem::DROPSOUND_WEAPON);

	PyModule_AddIntConstant(poModule, "EQUIPMENT_COUNT",			c_Equipment_Count);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_HEAD",				c_Equipment_Head);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_BODY",				c_Equipment_Body);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_WEAPON",			c_Equipment_Weapon);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_WRIST",			c_Equipment_Wrist);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_SHOES",			c_Equipment_Shoes);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_NECK",				c_Equipment_Neck);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_EAR",				c_Equipment_Ear);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_UNIQUE1",			c_Equipment_Unique1);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_UNIQUE2",			c_Equipment_Unique2);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_UNIQUE3",			c_Equipment_Unique3);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_UNIQUE4",			c_Equipment_Unique4);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_ARROW",			c_Equipment_Arrow);
#ifdef ENABLE_BELT_SYSTEM
	PyModule_AddIntConstant(poModule, "EQUIPMENT_BELT",				c_Equipment_Belt);
#endif

	PyModule_AddIntConstant(poModule, "ITEM_TYPE_NONE",				CItemData::ITEM_TYPE_NONE);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_WEAPON",			CItemData::ITEM_TYPE_WEAPON);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_ARMOR",			CItemData::ITEM_TYPE_ARMOR);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_USE",				CItemData::ITEM_TYPE_USE);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_AUTOUSE",			CItemData::ITEM_TYPE_AUTOUSE);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_MATERIAL",			CItemData::ITEM_TYPE_MATERIAL);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_SPECIAL",			CItemData::ITEM_TYPE_SPECIAL);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_TOOL",				CItemData::ITEM_TYPE_TOOL);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_LOTTERY",			CItemData::ITEM_TYPE_LOTTERY);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_ELK",				CItemData::ITEM_TYPE_ELK);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_METIN",			CItemData::ITEM_TYPE_METIN);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_CONTAINER",		CItemData::ITEM_TYPE_CONTAINER);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_FISH",				CItemData::ITEM_TYPE_FISH);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_ROD",				CItemData::ITEM_TYPE_ROD);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_RESOURCE",			CItemData::ITEM_TYPE_RESOURCE);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_CAMPFIRE",			CItemData::ITEM_TYPE_CAMPFIRE);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_UNIQUE",			CItemData::ITEM_TYPE_UNIQUE);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_SKILLBOOK",		CItemData::ITEM_TYPE_SKILLBOOK);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_QUEST",			CItemData::ITEM_TYPE_QUEST);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_POLYMORPH",		CItemData::ITEM_TYPE_POLYMORPH);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_TREASURE_BOX",		CItemData::ITEM_TYPE_TREASURE_BOX);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_TREASURE_KEY",		CItemData::ITEM_TYPE_TREASURE_KEY);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_SKILLFORGET",		CItemData::ITEM_TYPE_SKILLFORGET);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_PICK",				CItemData::ITEM_TYPE_PICK);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_BLEND",			CItemData::ITEM_TYPE_BLEND);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_RING",				CItemData::ITEM_TYPE_RING);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_TOTEM",			CItemData::ITEM_TYPE_TOTEM);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_GIFTBOX",			CItemData::ITEM_TYPE_GIFTBOX);
#ifdef ENABLE_PET_SYSTEM
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_PET",				CItemData::ITEM_TYPE_PET);
#endif
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_MOUNT",			CItemData::ITEM_TYPE_MOUNT);
#ifdef ENABLE_BELT_SYSTEM
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_BELT",				CItemData::ITEM_TYPE_BELT);
#endif
#ifdef ENABLE_DRAGONSOUL
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_DS",				CItemData::ITEM_TYPE_DS);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_SPECIAL_DS",		CItemData::ITEM_TYPE_SPECIAL_DS);
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_EXTRACT",			CItemData::ITEM_TYPE_EXTRACT);
	
	PyModule_AddIntConstant(poModule, "MATERIAL_DS_REFINE_NORMAL",	CItemData::MATERIAL_DS_REFINE_NORMAL);
	PyModule_AddIntConstant(poModule, "MATERIAL_DS_REFINE_BLESSED",	CItemData::MATERIAL_DS_REFINE_BLESSED);
	PyModule_AddIntConstant(poModule, "MATERIAL_DS_REFINE_HOLLY",	CItemData::MATERIAL_DS_REFINE_HOLLY);
#endif
#ifdef ENABLE_PET_ADVANCED
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_PET_ADVANCED",		CItemData::ITEM_TYPE_PET_ADVANCED);
#endif
#ifdef CRYSTAL_SYSTEM
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_CRYSTAL",			CItemData::ITEM_TYPE_CRYSTAL);
#endif

	PyModule_AddIntConstant(poModule, "ITEM_TYPE_SOUL",				CItemData::ITEM_TYPE_SOUL);
	PyModule_AddIntConstant(poModule, "SOUL_NONE",					CItemData::SOUL_NONE);
	PyModule_AddIntConstant(poModule, "SOUL_DREAM",					CItemData::SOUL_DREAM);
	PyModule_AddIntConstant(poModule, "SOUL_HEAVEN",				CItemData::SOUL_HEAVEN);

#ifdef ENABLE_COSTUME_SYSTEM
	PyModule_AddIntConstant(poModule, "ITEM_TYPE_COSTUME",			CItemData::ITEM_TYPE_COSTUME);

	// Item Sub Type
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_BODY",			CItemData::COSTUME_BODY);
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_HAIR",			CItemData::COSTUME_HAIR);
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_WEAPON",		CItemData::COSTUME_WEAPON);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_ACCE",			CItemData::COSTUME_ACCE);
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_ACCE",			c_Equipment_Acce);
#endif
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_MOUNT",			CItemData::COSTUME_MOUNT);
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_PET",			CItemData::COSTUME_PET);
	PyModule_AddIntConstant(poModule, "COSTUME_TYPE_ACCE_COSTUME",	CItemData::COSTUME_ACCE_COSTUME);
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_ACCE_CUSTOME",	c_Equipment_AcceCostume);

	PyModule_AddIntConstant(poModule, "ADDON_COSTUME_NONE",			CItemData::ADDON_COSTUME_NONE);
	PyModule_AddIntConstant(poModule, "ADDON_COSTUME_WEAPON",		CItemData::ADDON_COSTUME_WEAPON);
	PyModule_AddIntConstant(poModule, "ADDON_COSTUME_ARMOR",		CItemData::ADDON_COSTUME_ARMOR);
	PyModule_AddIntConstant(poModule, "ADDON_COSTUME_HAIR",			CItemData::ADDON_COSTUME_HAIR);

	// 인벤토리 및 장비창에서의 슬롯 번호
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_START",			c_Costume_Slot_Start);
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_COUNT",			c_Costume_Slot_Count);
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_BODY",			c_Costume_Slot_Body);
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_HAIR",			c_Costume_Slot_Hair);
	PyModule_AddIntConstant(poModule, "COSTUME_SLOT_END",			c_Costume_Slot_End);
#endif

	PyModule_AddIntConstant(poModule, "WEAPON_SWORD",				CItemData::WEAPON_SWORD);
	PyModule_AddIntConstant(poModule, "WEAPON_DAGGER",				CItemData::WEAPON_DAGGER);
	PyModule_AddIntConstant(poModule, "WEAPON_BOW",					CItemData::WEAPON_BOW);
	PyModule_AddIntConstant(poModule, "WEAPON_TWO_HANDED",			CItemData::WEAPON_TWO_HANDED);
	PyModule_AddIntConstant(poModule, "WEAPON_BELL",				CItemData::WEAPON_BELL);
	PyModule_AddIntConstant(poModule, "WEAPON_FAN",					CItemData::WEAPON_FAN);
	PyModule_AddIntConstant(poModule, "WEAPON_ARROW",				CItemData::WEAPON_ARROW);
	PyModule_AddIntConstant(poModule, "WEAPON_QUIVER",				CItemData::WEAPON_QUIVER);
	PyModule_AddIntConstant(poModule, "WEAPON_NUM_TYPES",			CItemData::WEAPON_NUM_TYPES);

	PyModule_AddIntConstant(poModule, "USE_POTION",					CItemData::USE_POTION);
	PyModule_AddIntConstant(poModule, "USE_TALISMAN",				CItemData::USE_TALISMAN);
	PyModule_AddIntConstant(poModule, "USE_TUNING",					CItemData::USE_TUNING);
	PyModule_AddIntConstant(poModule, "USE_MOVE",					CItemData::USE_MOVE);
	PyModule_AddIntConstant(poModule, "USE_TREASURE_BOX",			CItemData::USE_TREASURE_BOX);
	PyModule_AddIntConstant(poModule, "USE_MONEYBAG",				CItemData::USE_MONEYBAG);
	PyModule_AddIntConstant(poModule, "USE_BAIT",					CItemData::USE_BAIT);
	PyModule_AddIntConstant(poModule, "USE_ABILITY_UP",				CItemData::USE_ABILITY_UP);
	PyModule_AddIntConstant(poModule, "USE_AFFECT",					CItemData::USE_AFFECT);
	PyModule_AddIntConstant(poModule, "USE_CREATE_STONE",			CItemData::USE_CREATE_STONE);
	PyModule_AddIntConstant(poModule, "USE_SPECIAL",				CItemData::USE_SPECIAL);
	PyModule_AddIntConstant(poModule, "USE_POTION_NODELAY",			CItemData::USE_POTION_NODELAY);
	PyModule_AddIntConstant(poModule, "USE_CLEAR",					CItemData::USE_CLEAR);
	PyModule_AddIntConstant(poModule, "USE_INVISIBILITY",			CItemData::USE_INVISIBILITY);
	PyModule_AddIntConstant(poModule, "USE_DETACHMENT",				CItemData::USE_DETACHMENT);
	PyModule_AddIntConstant(poModule, "USE_TIME_CHARGE_PER",		CItemData::USE_TIME_CHARGE_PER);
	PyModule_AddIntConstant(poModule, "USE_TIME_CHARGE_FIX",		CItemData::USE_TIME_CHARGE_FIX);
	PyModule_AddIntConstant(poModule, "USE_ADD_SPECIFIC_ATTRIBUTE",	CItemData::USE_ADD_SPECIFIC_ATTRIBUTE);
	PyModule_AddIntConstant(poModule, "USE_DETACH_STONE",			CItemData::USE_DETACH_STONE);
	PyModule_AddIntConstant(poModule, "USE_DETACH_ATTR",			CItemData::USE_DETACH_ATTR);
#ifdef ENABLE_DRAGONSOUL
	PyModule_AddIntConstant(poModule, "USE_DS_CHANGE_ATTR",			CItemData::USE_DS_CHANGE_ATTR);
#endif
	PyModule_AddIntConstant(poModule, "USE_CHANGE_SASH_COSTUME_ATTR",CItemData::USE_CHANGE_SASH_COSTUME_ATTR);
	PyModule_AddIntConstant(poModule, "USE_DEL_LAST_PERM_ORE",		CItemData::USE_DEL_LAST_PERM_ORE);

	PyModule_AddIntConstant(poModule, "USE_PUT_INTO_ACCESSORY_SOCKET_PERMA",		CItemData::USE_PUT_INTO_ACCESSORY_SOCKET_PERMA);
	PyModule_AddIntConstant(poModule, "USE_PUT_INTO_ACCESSORY_SOCKET", CItemData::USE_PUT_INTO_ACCESSORY_SOCKET);

	PyModule_AddIntConstant(poModule, "METIN_NORMAL",				CItemData::METIN_NORMAL);
	PyModule_AddIntConstant(poModule, "METIN_GOLD",					CItemData::METIN_GOLD);

#ifdef PROMETA
	PyModule_AddIntConstant(poModule, "METIN_ACCE", CItemData::METIN_ACCE);
#endif

	PyModule_AddIntConstant(poModule, "MOUNT_SUB_SUMMON",			CItemData::MOUNT_SUB_SUMMON);
	PyModule_AddIntConstant(poModule, "MOUNT_SUB_FOOD",				CItemData::MOUNT_SUB_FOOD);
	PyModule_AddIntConstant(poModule, "MOUNT_SUB_REVIVE",			CItemData::MOUNT_SUB_REVIVE);

	PyModule_AddIntConstant(poModule, "LIMIT_NONE",					CItemData::LIMIT_NONE);
	PyModule_AddIntConstant(poModule, "LIMIT_LEVEL",				CItemData::LIMIT_LEVEL);
	PyModule_AddIntConstant(poModule, "LIMIT_STR",					CItemData::LIMIT_STR);
	PyModule_AddIntConstant(poModule, "LIMIT_DEX",					CItemData::LIMIT_DEX);
	PyModule_AddIntConstant(poModule, "LIMIT_INT",					CItemData::LIMIT_INT);
	PyModule_AddIntConstant(poModule, "LIMIT_CON",					CItemData::LIMIT_CON);
	PyModule_AddIntConstant(poModule, "LIMIT_REAL_TIME",			CItemData::LIMIT_REAL_TIME);
	PyModule_AddIntConstant(poModule, "LIMIT_REAL_TIME_START_FIRST_USE",	CItemData::LIMIT_REAL_TIME_START_FIRST_USE);
	PyModule_AddIntConstant(poModule, "LIMIT_TIMER_BASED_ON_WEAR",	CItemData::LIMIT_TIMER_BASED_ON_WEAR);
#ifdef NEW_OFFICIAL
	PyModule_AddIntConstant(poModule, "LIMIT_PC_BANG",				CItemData::PC_BANG);
#endif
	PyModule_AddIntConstant(poModule, "LIMIT_TYPE_MAX_NUM",			CItemData::LIMIT_MAX_NUM);
#ifdef ENABLE_LEVEL_LIMIT_MAX
	PyModule_AddIntConstant(poModule, "LIMIT_LEVEL_MAX",			CItemData::LIMIT_LEVEL_MAX);
#endif
	PyModule_AddIntConstant(poModule, "LIMIT_MAX_NUM",				CItemData::ITEM_LIMIT_MAX_NUM);

	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_FEMALE",		CItemData::ITEM_ANTIFLAG_FEMALE);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_MALE",			CItemData::ITEM_ANTIFLAG_MALE);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_WARRIOR",		CItemData::ITEM_ANTIFLAG_WARRIOR);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_ASSASSIN",		CItemData::ITEM_ANTIFLAG_ASSASSIN);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_SURA",			CItemData::ITEM_ANTIFLAG_SURA);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_SHAMAN",		CItemData::ITEM_ANTIFLAG_SHAMAN);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_GET",			CItemData::ITEM_ANTIFLAG_GET);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_DROP",			CItemData::ITEM_ANTIFLAG_DROP);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_SELL",			CItemData::ITEM_ANTIFLAG_SELL);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_EMPIRE_A",		CItemData::ITEM_ANTIFLAG_EMPIRE_A);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_EMPIRE_B",		CItemData::ITEM_ANTIFLAG_EMPIRE_B);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_EMPIRE_R",		CItemData::ITEM_ANTIFLAG_EMPIRE_R);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_SAVE",			CItemData::ITEM_ANTIFLAG_SAVE);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_GIVE",			CItemData::ITEM_ANTIFLAG_GIVE);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_PKDROP",		CItemData::ITEM_ANTIFLAG_PKDROP);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_STACK",		CItemData::ITEM_ANTIFLAG_STACK);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_MYSHOP",		CItemData::ITEM_ANTIFLAG_MYSHOP);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_DESTROY",		CItemData::ITEM_ANTIFLAG_DESTROY);
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_APPLY",		CItemData::ITEM_ANTIFLAG_APPLY);

	PyModule_AddIntConstant(poModule, "ITEM_FLAG_RARE",				CItemData::ITEM_FLAG_RARE);
	PyModule_AddIntConstant(poModule, "ITEM_FLAG_UNIQUE",			CItemData::ITEM_FLAG_UNIQUE);
	PyModule_AddIntConstant(poModule, "ITEM_FLAG_CONFIRM_WHEN_USE",	CItemData::ITEM_FLAG_CONFIRM_WHEN_USE);
	PyModule_AddIntConstant(poModule, "ITEM_FLAG_CONFIRM_GM_ITEM",	CItemData::ITEM_FLAG_CONFIRM_GM_ITEM);

	PyModule_AddIntConstant(poModule, "ANTIFLAG_FEMALE",			CItemData::ITEM_ANTIFLAG_FEMALE);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_MALE",				CItemData::ITEM_ANTIFLAG_MALE);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_WARRIOR",			CItemData::ITEM_ANTIFLAG_WARRIOR);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_ASSASSIN",			CItemData::ITEM_ANTIFLAG_ASSASSIN);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_SURA",				CItemData::ITEM_ANTIFLAG_SURA);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_SHAMAN",			CItemData::ITEM_ANTIFLAG_SHAMAN);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_GET",				CItemData::ITEM_ANTIFLAG_GET);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_DROP",				CItemData::ITEM_ANTIFLAG_DROP);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_SELL",				CItemData::ITEM_ANTIFLAG_SELL);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_EMPIRE_A",			CItemData::ITEM_ANTIFLAG_EMPIRE_A);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_EMPIRE_B",			CItemData::ITEM_ANTIFLAG_EMPIRE_B);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_EMPIRE_R",			CItemData::ITEM_ANTIFLAG_EMPIRE_R);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_SAVE",				CItemData::ITEM_ANTIFLAG_SAVE);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_GIVE",				CItemData::ITEM_ANTIFLAG_GIVE);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_PKDROP",			CItemData::ITEM_ANTIFLAG_PKDROP);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_STACK",				CItemData::ITEM_ANTIFLAG_STACK);
	PyModule_AddIntConstant(poModule, "ANTIFLAG_MYSHOP",			CItemData::ITEM_ANTIFLAG_MYSHOP);

	PyModule_AddIntConstant(poModule, "WEARABLE_BODY",				CItemData::WEARABLE_BODY);
	PyModule_AddIntConstant(poModule, "WEARABLE_HEAD",				CItemData::WEARABLE_HEAD);
	PyModule_AddIntConstant(poModule, "WEARABLE_FOOTS",				CItemData::WEARABLE_FOOTS);
	PyModule_AddIntConstant(poModule, "WEARABLE_WRIST",				CItemData::WEARABLE_WRIST);
	PyModule_AddIntConstant(poModule, "WEARABLE_WEAPON",			CItemData::WEARABLE_WEAPON);
	PyModule_AddIntConstant(poModule, "WEARABLE_NECK",				CItemData::WEARABLE_NECK);
	PyModule_AddIntConstant(poModule, "WEARABLE_EAR",				CItemData::WEARABLE_EAR);
	PyModule_AddIntConstant(poModule, "WEARABLE_UNIQUE",			CItemData::WEARABLE_UNIQUE);
	PyModule_AddIntConstant(poModule, "WEARABLE_SHIELD",			CItemData::WEARABLE_SHIELD);
	PyModule_AddIntConstant(poModule, "WEARABLE_ARROW",				CItemData::WEARABLE_ARROW);
	PyModule_AddIntConstant(poModule, "WEARABLE_TOTEM",				CItemData::WEARABLE_TOTEM);

	PyModule_AddIntConstant(poModule, "ARMOR_BODY",					CItemData::ARMOR_BODY);
	PyModule_AddIntConstant(poModule, "ARMOR_HEAD",					CItemData::ARMOR_HEAD);
	PyModule_AddIntConstant(poModule, "ARMOR_SHIELD",				CItemData::ARMOR_SHIELD);
	PyModule_AddIntConstant(poModule, "ARMOR_WRIST",				CItemData::ARMOR_WRIST);
	PyModule_AddIntConstant(poModule, "ARMOR_FOOTS",				CItemData::ARMOR_FOOTS);
	PyModule_AddIntConstant(poModule, "ARMOR_NECK",					CItemData::ARMOR_NECK);
	PyModule_AddIntConstant(poModule, "ARMOR_EAR",					CItemData::ARMOR_EAR);

	PyModule_AddIntConstant(poModule, "FISH_ALIVE",					CItemData::FISH_ALIVE);
	PyModule_AddIntConstant(poModule, "FISH_DEAD",					CItemData::FISH_DEAD);

#ifdef ENABLE_PET_ADVANCED
	PyModule_AddIntConstant(poModule, "PET_EGG",					static_cast<long>(CItemData::EPetAdvanced::EGG));
	PyModule_AddIntConstant(poModule, "PET_SUMMON",					static_cast<long>(CItemData::EPetAdvanced::SUMMON));
	PyModule_AddIntConstant(poModule, "PET_SKILL_BOOK",				static_cast<long>(CItemData::EPetAdvanced::SKILL_BOOK));
	PyModule_AddIntConstant(poModule, "PET_HEROIC_SKILL_BOOK",		static_cast<long>(CItemData::EPetAdvanced::HEROIC_SKILL_BOOK));
	PyModule_AddIntConstant(poModule, "PET_SKILL_REVERT",			static_cast<long>(CItemData::EPetAdvanced::SKILL_REVERT));
	PyModule_AddIntConstant(poModule, "PET_SKILLPOWER_REROLL",		static_cast<long>(CItemData::EPetAdvanced::SKILLPOWER_REROLL));
#endif
	
#ifdef CRYSTAL_SYSTEM
	PyModule_AddIntConstant(poModule, "CRYSTAL_ACTIVE",				static_cast<long>(CItemData::ECrystalItem::CRYSTAL));
	PyModule_AddIntConstant(poModule, "CRYSTAL_FRAGMENT",			static_cast<long>(CItemData::ECrystalItem::FRAGMENT));
	PyModule_AddIntConstant(poModule, "CRYSTAL_UPGRADE_SCROLL",		static_cast<long>(CItemData::ECrystalItem::UPGRADE_SCROLL));
	PyModule_AddIntConstant(poModule, "CRYSTAL_TIME_ELIXIR",		static_cast<long>(CItemData::ECrystalItem::TIME_ELIXIR));
#endif

	PyModule_AddIntConstant(poModule, "ITEM_APPLY_MAX_NUM",			CItemData::ITEM_APPLY_MAX_NUM);
	PyModule_AddIntConstant(poModule, "ITEM_SOCKET_MAX_NUM",		CItemData::ITEM_SOCKET_MAX_NUM);

#ifdef PROMETA
	PyModule_AddIntConstant(poModule, "ACCE_SOCKET_MAX_NUM", CItemData::ACCE_SOCKET_MAX_NUM);
#endif

	PyModule_AddIntConstant(poModule, "APPLY_NONE",					CItemData::APPLY_NONE);
	PyModule_AddIntConstant(poModule, "APPLY_STR",					CItemData::APPLY_STR);
	PyModule_AddIntConstant(poModule, "APPLY_DEX",					CItemData::APPLY_DEX);
	PyModule_AddIntConstant(poModule, "APPLY_CON",					CItemData::APPLY_CON);
	PyModule_AddIntConstant(poModule, "APPLY_INT",					CItemData::APPLY_INT);
	PyModule_AddIntConstant(poModule, "APPLY_MAX_HP",				CItemData::APPLY_MAX_HP);
	PyModule_AddIntConstant(poModule, "APPLY_MAX_SP",				CItemData::APPLY_MAX_SP);
	PyModule_AddIntConstant(poModule, "APPLY_HP_REGEN",				CItemData::APPLY_HP_REGEN);
	PyModule_AddIntConstant(poModule, "APPLY_SP_REGEN",				CItemData::APPLY_SP_REGEN);
	PyModule_AddIntConstant(poModule, "APPLY_DEF_GRADE_BONUS",		CItemData::APPLY_DEF_GRADE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_ATT_GRADE_BONUS",		CItemData::APPLY_ATT_GRADE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_ATT_SPEED",			CItemData::APPLY_ATT_SPEED);
	PyModule_AddIntConstant(poModule, "APPLY_MOV_SPEED",			CItemData::APPLY_MOV_SPEED);
	PyModule_AddIntConstant(poModule, "APPLY_CAST_SPEED",			CItemData::APPLY_CAST_SPEED);
	PyModule_AddIntConstant(poModule, "APPLY_MAGIC_ATT_GRADE",		CItemData::APPLY_MAGIC_ATT_GRADE);
	PyModule_AddIntConstant(poModule, "APPLY_MAGIC_DEF_GRADE",		CItemData::APPLY_MAGIC_DEF_GRADE);
	PyModule_AddIntConstant(poModule, "APPLY_SKILL",				CItemData::APPLY_SKILL);
    PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ANIMAL",		CItemData::APPLY_ATTBONUS_ANIMAL);
    PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_UNDEAD",		CItemData::APPLY_ATTBONUS_UNDEAD);
    PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_DEVIL", 		CItemData::APPLY_ATTBONUS_DEVIL);
    PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_HUMAN",		CItemData::APPLY_ATTBONUS_HUMAN);
    PyModule_AddIntConstant(poModule, "APPLY_BOW_DISTANCE", 		CItemData::APPLY_BOW_DISTANCE);
    PyModule_AddIntConstant(poModule, "APPLY_RESIST_BOW", 			CItemData::APPLY_RESIST_BOW);
    PyModule_AddIntConstant(poModule, "APPLY_RESIST_FIRE", 			CItemData::APPLY_RESIST_FIRE);
    PyModule_AddIntConstant(poModule, "APPLY_RESIST_ELEC", 			CItemData::APPLY_RESIST_ELEC);
    PyModule_AddIntConstant(poModule, "APPLY_RESIST_MAGIC", 		CItemData::APPLY_RESIST_MAGIC);
    PyModule_AddIntConstant(poModule, "APPLY_POISON_PCT",			CItemData::APPLY_POISON_PCT);
    PyModule_AddIntConstant(poModule, "APPLY_SLOW_PCT", 			CItemData::APPLY_SLOW_PCT);
    PyModule_AddIntConstant(poModule, "APPLY_STUN_PCT", 			CItemData::APPLY_STUN_PCT);
	PyModule_AddIntConstant(poModule, "APPLY_CRITICAL_PCT",			CItemData::APPLY_CRITICAL_PCT);			// n% 확률로 두배 타격
	PyModule_AddIntConstant(poModule, "APPLY_PENETRATE_PCT",		CItemData::APPLY_PENETRATE_PCT);		// n% 확률로 적의 방어력 무시
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ORC",			CItemData::APPLY_ATTBONUS_ORC);			// 웅귀에게 n% 추가 데미지
#ifdef ENABLE_ZODIAC
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ZODIAC",		CItemData::APPLY_ATTBONUS_ZODIAC);			// 웅귀에게 n% 추가 데미지
#else
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ZODIAC",		CItemData::APPLY_NONE);			// 웅귀에게 n% 추가 데미지
#endif
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_MILGYO",		CItemData::APPLY_ATTBONUS_MILGYO);		// 밀교에게 n% 추가 데미지
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_UNDEAD",		CItemData::APPLY_ATTBONUS_UNDEAD);		// 시체에게 n% 추가 데미지
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_DEVIL",		CItemData::APPLY_ATTBONUS_DEVIL);		// 악마에게 n% 추가 데미지
	PyModule_AddIntConstant(poModule, "APPLY_STEAL_HP",				CItemData::APPLY_STEAL_HP);				// n% 확률로 타격의 10% 를 생명력으로 흡수
	PyModule_AddIntConstant(poModule, "APPLY_STEAL_SP",				CItemData::APPLY_STEAL_SP);				// n% 확률로 타격의 10% 를 정신력으로 흡수
	PyModule_AddIntConstant(poModule, "APPLY_MANA_BURN_PCT",		CItemData::APPLY_MANA_BURN_PCT);		// n% 확률로 상대의 마나를 깎는다
	PyModule_AddIntConstant(poModule, "APPLY_DAMAGE_SP_RECOVER",	CItemData::APPLY_DAMAGE_SP_RECOVER);	// n% 확률로 정신력 2 회복
	PyModule_AddIntConstant(poModule, "APPLY_BLOCK",				CItemData::APPLY_BLOCK);				// n% 확률로 물리공격 완벽 방어
	PyModule_AddIntConstant(poModule, "APPLY_DODGE",				CItemData::APPLY_DODGE);				// n% 확률로 물리공격 완벽 회피
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_SWORD",			CItemData::APPLY_RESIST_SWORD);			// 한손검에 의한 피해를 n% 감소
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_TWOHAND",		CItemData::APPLY_RESIST_TWOHAND);		// 양손검에 의한 피해를 n% 감소
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_DAGGER",		CItemData::APPLY_RESIST_DAGGER);		// 단도에 의한 피해를 n% 감소
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_BELL",			CItemData::APPLY_RESIST_BELL);			// 방울에 의한 피해를 n% 감소
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_FAN",			CItemData::APPLY_RESIST_FAN);			// 부채에 의한 피해를 n% 감소
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_WIND",			CItemData::APPLY_RESIST_WIND);			// 바람에 의한 피해를 n% 감소
	PyModule_AddIntConstant(poModule, "APPLY_REFLECT_MELEE",		CItemData::APPLY_REFLECT_MELEE);		// 근접 타격 n% 를 적에게 되돌린다
	PyModule_AddIntConstant(poModule, "APPLY_REFLECT_CURSE",		CItemData::APPLY_REFLECT_CURSE);		// 적이 나에게 저주 사용시 n% 확률로 되돌린다
	PyModule_AddIntConstant(poModule, "APPLY_POISON_REDUCE",		CItemData::APPLY_POISON_REDUCE);		// 독에 의한 데미지 감소
	PyModule_AddIntConstant(poModule, "APPLY_KILL_SP_RECOVER",		CItemData::APPLY_KILL_SP_RECOVER);		// 적을 죽였을때 n% 확률로 정신력 10 회복
	PyModule_AddIntConstant(poModule, "APPLY_EXP_DOUBLE_BONUS",		CItemData::APPLY_EXP_DOUBLE_BONUS);		// n% 확률로 경험치 획득량 2배
	PyModule_AddIntConstant(poModule, "APPLY_EXP_REAL_BONUS",		CItemData::APPLY_EXP_REAL_BONUS);		// n% 확률로 경험치 획득량 2배
	PyModule_AddIntConstant(poModule, "APPLY_GOLD_DOUBLE_BONUS",	CItemData::APPLY_GOLD_DOUBLE_BONUS);	// n% 확률로 돈 획득량 2배
	PyModule_AddIntConstant(poModule, "APPLY_ITEM_DROP_BONUS",		CItemData::APPLY_ITEM_DROP_BONUS);		// n% 확률로 아이템 획득량 2배
	PyModule_AddIntConstant(poModule, "APPLY_POTION_BONUS",			CItemData::APPLY_POTION_BONUS);			// 물약 복용시 n% 만큼 성능 증대
	PyModule_AddIntConstant(poModule, "APPLY_KILL_HP_RECOVER",		CItemData::APPLY_KILL_HP_RECOVER);		// 죽일때마다 생명력 회복 
	PyModule_AddIntConstant(poModule, "APPLY_IMMUNE_STUN",			CItemData::APPLY_IMMUNE_STUN);			// 기절 하지 않는다
	PyModule_AddIntConstant(poModule, "APPLY_IMMUNE_SLOW",			CItemData::APPLY_IMMUNE_SLOW);			// 느려지지 않는다
	PyModule_AddIntConstant(poModule, "APPLY_IMMUNE_FALL",			CItemData::APPLY_IMMUNE_FALL);			// 넘어지지 않는다
	PyModule_AddIntConstant(poModule, "APPLY_MAX_STAMINA",			CItemData::APPLY_MAX_STAMINA);			// 최대 스테미너 증가
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_WARRIOR",		CItemData::APPLY_ATT_BONUS_TO_WARRIOR);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ASSASSIN",	CItemData::APPLY_ATT_BONUS_TO_ASSASSIN);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_SURA",		CItemData::APPLY_ATT_BONUS_TO_SURA);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_SHAMAN",		CItemData::APPLY_ATT_BONUS_TO_SHAMAN);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_MONSTER",		CItemData::APPLY_ATT_BONUS_TO_MONSTER);
	PyModule_AddIntConstant(poModule, "APPLY_MALL_ATTBONUS",		CItemData::APPLY_MALL_ATTBONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MALL_DEFBONUS",		CItemData::APPLY_MALL_DEFBONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MALL_EXPBONUS",		CItemData::APPLY_MALL_EXPBONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MALL_ITEMBONUS",		CItemData::APPLY_MALL_ITEMBONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MALL_GOLDBONUS",		CItemData::APPLY_MALL_GOLDBONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MAX_HP_PCT",			CItemData::APPLY_MAX_HP_PCT);
	PyModule_AddIntConstant(poModule, "APPLY_MAX_SP_PCT",			CItemData::APPLY_MAX_SP_PCT);
	PyModule_AddIntConstant(poModule, "APPLY_SKILL_DAMAGE_BONUS",		CItemData::APPLY_SKILL_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_NORMAL_HIT_DAMAGE_BONUS",	CItemData::APPLY_NORMAL_HIT_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_SKILL_DEFEND_BONUS",		CItemData::APPLY_SKILL_DEFEND_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_NORMAL_HIT_DEFEND_BONUS",	CItemData::APPLY_NORMAL_HIT_DEFEND_BONUS);

	PyModule_AddIntConstant(poModule, "APPLY_RESIST_WARRIOR",	CItemData::APPLY_RESIST_WARRIOR );
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_ASSASSIN",	CItemData::APPLY_RESIST_ASSASSIN );
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_SURA",		CItemData::APPLY_RESIST_SURA );
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_SHAMAN",	CItemData::APPLY_RESIST_SHAMAN );
	PyModule_AddIntConstant(poModule, "APPLY_ENERGY",	CItemData::APPLY_ENERGY );		// 기력
	PyModule_AddIntConstant(poModule, "APPLY_COSTUME_ATTR_BONUS",	CItemData::APPLY_COSTUME_ATTR_BONUS );		

	PyModule_AddIntConstant(poModule, "APPLY_MAGIC_ATTBONUS_PER",	CItemData::APPLY_MAGIC_ATTBONUS_PER );		
	PyModule_AddIntConstant(poModule, "APPLY_MELEE_MAGIC_ATTBONUS_PER",	CItemData::APPLY_MELEE_MAGIC_ATTBONUS_PER );		
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_ICE",	CItemData::APPLY_RESIST_ICE );		
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_EARTH",	CItemData::APPLY_RESIST_EARTH );		
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_DARK",	CItemData::APPLY_RESIST_DARK );		
	PyModule_AddIntConstant(poModule, "APPLY_ANTI_CRITICAL_PCT",	CItemData::APPLY_ANTI_CRITICAL_PCT );		
	PyModule_AddIntConstant(poModule, "APPLY_ANTI_PENETRATE_PCT",	CItemData::APPLY_ANTI_PENETRATE_PCT );		

#ifdef ENABLE_WOLFMAN
	PyModule_AddIntConstant(poModule, "ITEM_ANTIFLAG_WOLFMAN", CItemData::ITEM_ANTIFLAG_WOLFMAN);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_WOLFMAN", CItemData::APPLY_ATTBONUS_WOLFMAN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_WOLFMAN", CItemData::APPLY_RESIST_WOLFMAN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_CLAW", CItemData::APPLY_RESIST_CLAW);
	PyModule_AddIntConstant(poModule, "APPLY_BLEEDING_PCT", CItemData::APPLY_BLEEDING_PCT);
	PyModule_AddIntConstant(poModule, "APPLY_BLEEDING_REDUCE", CItemData::APPLY_BLEEDING_REDUCE);
#endif

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	PyModule_AddIntConstant(poModule, "APPLY_ACCEDRAIN_RATE", CItemData::APPLY_ACCEDRAIN_RATE);
#endif

#ifdef ENABLE_ANIMAL_SYSTEM
#ifdef ENABLE_PET_SYSTEM
	PyModule_AddIntConstant(poModule, "APPLY_PET_EXP_BONUS", CItemData::APPLY_PET_EXP_BONUS);
#endif
	PyModule_AddIntConstant(poModule, "APPLY_MOUNT_EXP_BONUS", CItemData::APPLY_MOUNT_EXP_BONUS);
#endif
	PyModule_AddIntConstant(poModule, "APPLY_MOUNT_BUFF_BONUS",				CItemData::APPLY_MOUNT_BUFF_BONUS);
	

	PyModule_AddIntConstant(poModule, "APPLY_RESIST_BOSS", CItemData::APPLY_RESIST_BOSS);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_MONSTER", CItemData::APPLY_RESIST_MONSTER);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_METIN", CItemData::APPLY_ATTBONUS_METIN);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_BOSS", CItemData::APPLY_ATTBONUS_BOSS);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_HUMAN", CItemData::APPLY_RESIST_HUMAN);

	PyModule_AddIntConstant(poModule, "APPLY_RESIST_SWORD_PEN", CItemData::APPLY_RESIST_SWORD_PEN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_TWOHAND_PEN", CItemData::APPLY_RESIST_TWOHAND_PEN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_DAGGER_PEN", CItemData::APPLY_RESIST_DAGGER_PEN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_BELL_PEN", CItemData::APPLY_RESIST_BELL_PEN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_FAN_PEN", CItemData::APPLY_RESIST_FAN_PEN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_BOW_PEN", CItemData::APPLY_RESIST_BOW_PEN);
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_ATTBONUS_HUMAN", CItemData::APPLY_RESIST_ATTBONUS_HUMAN);

#ifdef __ELEMENT_SYSTEM__
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ELEC", CItemData::APPLY_ATTBONUS_ELEC);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_FIRE", CItemData::APPLY_ATTBONUS_FIRE);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ICE", CItemData::APPLY_ATTBONUS_ICE);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_WIND", CItemData::APPLY_ATTBONUS_WIND);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_EARTH", CItemData::APPLY_ATTBONUS_EARTH);
	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_DARK", CItemData::APPLY_ATTBONUS_DARK);
#endif

	PyModule_AddIntConstant(poModule, "APPLY_DEFENSE_BONUS", CItemData::APPLY_DEFENSE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_ANTI_RESIST_MAGIC", CItemData::APPLY_ANTI_RESIST_MAGIC);
	PyModule_AddIntConstant(poModule, "APPLY_BLOCK_IGNORE_BONUS", CItemData::APPLY_BLOCK_IGNORE_BONUS);

	PyModule_AddIntConstant(poModule, "APPLY_HEAL_EFFECT_BONUS", CItemData::APPLY_HEAL_EFFECT_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_CRITICAL_DAMAGE_BONUS", CItemData::APPLY_CRITICAL_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_DOUBLE_ITEM_DROP_BONUS", CItemData::APPLY_DOUBLE_ITEM_DROP_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_DAMAGE_BY_SP_BONUS", CItemData::APPLY_DAMAGE_BY_SP_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_SINGLETARGET_SKILL_DAMAGE_BONUS", CItemData::APPLY_SINGLETARGET_SKILL_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MULTITARGET_SKILL_DAMAGE_BONUS", CItemData::APPLY_MULTITARGET_SKILL_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_MIXED_DEFEND_BONUS", CItemData::APPLY_MIXED_DEFEND_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_EQUIP_SKILL_BONUS", CItemData::APPLY_EQUIP_SKILL_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_AURA_HEAL_EFFECT_BONUS", CItemData::APPLY_AURA_HEAL_EFFECT_BONUS);
	PyModule_AddIntConstant(poModule, "APPLY_AURA_EQUIP_SKILL_BONUS", CItemData::APPLY_AURA_EQUIP_SKILL_BONUS);

	PyModule_AddIntConstant(poModule, "APPLY_ATTBONUS_ALL_ELEMENTS", CItemData::APPLY_ATTBONUS_ALL_ELEMENTS);

#ifdef ENABLE_SKIN_SYSTEM
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_SLOT_PET", c_SkinSystem_Slot_Pet);
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_SLOT_MOUNT", c_SkinSystem_Slot_Mount);
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_SLOT_BUFFI_BODY", c_SkinSystem_Slot_BuffiBody);
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_SLOT_BUFFI_WEAPON", c_SkinSystem_Slot_BuffiWeapon);
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_SLOT_BUFFI_HAIR", c_SkinSystem_Slot_BuffiHair);
#endif
	PyModule_AddIntConstant(poModule, "APPLY_RESIST_MAGIC_REDUCTION", CItemData::APPLY_RESIST_MAGIC_REDUCTION);
#ifdef STANDARD_SKILL_DURATION
	PyModule_AddIntConstant(poModule, "APPLY_SKILL_DURATION", CItemData::APPLY_SKILL_DURATION);
#else
	PyModule_AddIntConstant(poModule, "APPLY_SKILL_DURATION", CItemData::APPLY_NONE);
#endif
#ifdef ENABLE_RUNE_SYSTEM
	PyModule_AddIntConstant(poModule, "APPLY_RUNE_MOUNT_PARALYZE", CItemData::APPLY_RUNE_MOUNT_PARALYZE);
#else
	PyModule_AddIntConstant(poModule, "APPLY_RUNE_MOUNT_PARALYZE", CItemData::APPLY_NONE);
#endif
}
