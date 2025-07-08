#include "StdAfx.h"
#include "PythonSafeBox.h"

void CPythonSafeBox::OpenSafeBox(int iSize)
{
	m_dwMoney = 0;
	m_ItemInstanceVector.clear();
	m_ItemInstanceVector.resize(SAFEBOX_SLOT_X_COUNT * iSize);

	for (DWORD i = 0; i < m_ItemInstanceVector.size(); ++i)
	{
		auto& rInstance = m_ItemInstanceVector[i];
		rInstance.Clear();
	}
}

void CPythonSafeBox::SetItemData(DWORD dwSlotIndex, const network::TItemData & rItemInstance)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::SetItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}

	m_ItemInstanceVector[dwSlotIndex] = rItemInstance;
}

void CPythonSafeBox::DelItemData(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::DelItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}

	auto& rInstance = m_ItemInstanceVector[dwSlotIndex];
	rInstance.Clear();
}

void CPythonSafeBox::SetMoney(DWORD dwMoney)
{
	m_dwMoney = dwMoney;
}

DWORD CPythonSafeBox::GetMoney()
{
	return m_dwMoney;
}

int CPythonSafeBox::GetCurrentSafeBoxSize()
{
	return m_ItemInstanceVector.size();
}

BOOL CPythonSafeBox::GetSlotItemID(DWORD dwSlotIndex, DWORD* pdwItemID)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::GetSlotItemID(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*pdwItemID=m_ItemInstanceVector[dwSlotIndex].vnum();

	return TRUE;
}

BOOL CPythonSafeBox::GetItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance)
{
	if (dwSlotIndex >= m_ItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::GetItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*ppInstance = &m_ItemInstanceVector[dwSlotIndex];

	return TRUE;
}

void CPythonSafeBox::OpenMall(int iSize)
{
	m_MallItemInstanceVector.clear();
	m_MallItemInstanceVector.resize(SAFEBOX_SLOT_X_COUNT * iSize);

	for (DWORD i = 0; i < m_MallItemInstanceVector.size(); ++i)
	{
		auto& rInstance = m_MallItemInstanceVector[i];
		rInstance.Clear();
	}
}

void CPythonSafeBox::SetMallItemData(DWORD dwSlotIndex, const network::TItemData & rItemData)
{
	if (dwSlotIndex >= m_MallItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::SetMallItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}

	m_MallItemInstanceVector[dwSlotIndex] = rItemData;
}

void CPythonSafeBox::DelMallItemData(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= m_MallItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::DelMallItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}

	auto& rInstance = m_MallItemInstanceVector[dwSlotIndex];
	rInstance.Clear();
}

BOOL CPythonSafeBox::GetMallItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance)
{
	if (dwSlotIndex >= m_MallItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::GetMallSlotItemID(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*ppInstance = &m_MallItemInstanceVector[dwSlotIndex];

	return TRUE;
}

BOOL CPythonSafeBox::GetSlotMallItemID(DWORD dwSlotIndex, DWORD * pdwItemID)
{
	if (dwSlotIndex >= m_MallItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::GetMallSlotItemID(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*pdwItemID = m_MallItemInstanceVector[dwSlotIndex].vnum();

	return TRUE;
}

DWORD CPythonSafeBox::GetMallSize()
{
	return m_MallItemInstanceVector.size();
}

#ifdef ENABLE_GUILD_SAFEBOX
void CPythonSafeBox::SetGuildEnable(bool bIsEnabled)
{
	m_bGuildSafeboxEnabled = bIsEnabled;
}

void CPythonSafeBox::OpenGuild(int iSize)
{
	m_ullGuildMoney = 0;
	m_GuildItemInstanceVector.clear();
	m_GuildItemInstanceVector.resize(SAFEBOX_SLOT_X_COUNT * SAFEBOX_SLOT_Y_COUNT * iSize);

	for (DWORD i = 0; i < m_GuildItemInstanceVector.size(); ++i)
	{
		auto& rInstance = m_GuildItemInstanceVector[i];
		rInstance.Clear();
	}
}

void CPythonSafeBox::SetGuildItemData(DWORD dwSlotIndex, const network::TItemData & rItemData)
{
	if (dwSlotIndex >= m_GuildItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::SetGuildItemData(dwSlotIndex=%d) - Strange slot index (maxIndex %d)",
			dwSlotIndex, m_GuildItemInstanceVector.size());
		return;
	}

	m_GuildItemInstanceVector[dwSlotIndex] = rItemData;
}

void CPythonSafeBox::DelGuildItemData(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= m_GuildItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::DelGuildItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}

	auto& rInstance = m_GuildItemInstanceVector[dwSlotIndex];
	rInstance.Clear();
}

BOOL CPythonSafeBox::GetGuildItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance)
{
	if (dwSlotIndex >= m_GuildItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::GetGuildSlotItemID(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*ppInstance = &m_GuildItemInstanceVector[dwSlotIndex];

	return TRUE;
}

BOOL CPythonSafeBox::GetSlotGuildItemID(DWORD dwSlotIndex, DWORD * pdwItemID)
{
	if (dwSlotIndex >= m_GuildItemInstanceVector.size())
	{
		TraceError("CPythonSafeBox::GetGuildSlotItemID(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*pdwItemID = m_GuildItemInstanceVector[dwSlotIndex].vnum();

	return TRUE;
}

void CPythonSafeBox::SetGuildMoney(ULONGLONG ullMoney)
{
	m_ullGuildMoney = ullMoney;
}

ULONGLONG CPythonSafeBox::GetGuildMoney()
{
	return m_ullGuildMoney;
}

DWORD CPythonSafeBox::GetGuildSize()
{
	return m_GuildItemInstanceVector.size();
}

void CPythonSafeBox::ClearGuildLogInfo()
{
	m_GuildLogInfo.clear();
}

void CPythonSafeBox::AddGuildLogInfo(const network::TGuildSafeboxLogTable& rkLogInfo)
{
	m_GuildLogInfo.insert(m_GuildLogInfo.end(), rkLogInfo)->set_time(time(0) - rkLogInfo.time());
}

void CPythonSafeBox::SortGuildLogInfo()
{
	auto kInfo = &m_GuildLogInfo[0];
	std::qsort(&m_GuildLogInfo[0], m_GuildLogInfo.size(), sizeof(network::TGuildSafeboxLogTable), [](const void* pFirst, const void* pSecond)
	{
		auto pLogInfo1 = (network::TGuildSafeboxLogTable*) pFirst;
		auto pLogInfo2 = (network::TGuildSafeboxLogTable*) pSecond;

		if (pLogInfo1->time() < pLogInfo2->time()) return 1;
		if (pLogInfo1->time() > pLogInfo2->time()) return -1;
		return 0;
	});
}

DWORD CPythonSafeBox::GetGuildLogInfoCount() const
{
	return m_GuildLogInfo.size();
}

network::TGuildSafeboxLogTable* CPythonSafeBox::GetGuildLogInfo(DWORD dwIndex)
{
	if (dwIndex >= m_GuildLogInfo.size())
		return NULL;

	return &m_GuildLogInfo[dwIndex];
}
#endif

CPythonSafeBox::CPythonSafeBox()
{
	m_dwMoney = 0;
#ifdef ENABLE_GUILD_SAFEBOX
	m_bGuildSafeboxEnabled = false;
#endif
}

CPythonSafeBox::~CPythonSafeBox()
{
}

PyObject * safeboxGetCurrentSafeboxSize(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonSafeBox::Instance().GetCurrentSafeBoxSize());
}

PyObject * safeboxGetItemID(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData * pInstance;
	if (!CPythonSafeBox::Instance().GetItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->vnum());
}

PyObject * safeboxGetItemCount(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetItemDataPtr(ipos, &pInstance)) // GetItemDataPtr is already writing on syserr
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pInstance->count());
}

#ifdef ENABLE_ALPHA_EQUIP
PyObject * safeboxGetItemAlphaEquip(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->alpha_equip());
}
#endif

PyObject * safeboxGetItemMetinSocket(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BadArgument();
	int iSocketIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSocketIndex))
		return Py_BadArgument();

	if (iSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return Py_BuildException();

	network::TItemData* pItemData;
	if (!CPythonSafeBox::Instance().GetItemDataPtr(iSlotIndex, &pItemData))
		return Py_BuildException();

	return Py_BuildValue("i", pItemData->sockets(iSocketIndex));
}

PyObject * safeboxGetItemAttribute(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();
	int iAttrSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttrSlotIndex))
		return Py_BuildException();

	if (iAttrSlotIndex >= 0 && iAttrSlotIndex < ITEM_ATTRIBUTE_SLOT_MAX_NUM)
	{
		network::TItemData* pItemData;
		if (CPythonSafeBox::Instance().GetItemDataPtr(iSlotIndex, &pItemData))
			return Py_BuildValue("ii", pItemData->attributes(iAttrSlotIndex).type(), pItemData->attributes(iAttrSlotIndex).value());
	}

	return Py_BuildValue("ii", 0, 0);
}

PyObject * safeboxGetMoney(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonSafeBox::Instance().GetMoney());
}

PyObject * safeboxGetMallItemID(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetMallItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->vnum());
}

PyObject * safeboxGetMallItemCount(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetMallItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->count());
}

#ifdef ENABLE_ALPHA_EQUIP
PyObject * safeboxGetMallItemAlphaEquip(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetMallItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->alpha_equip());
}
#endif

PyObject * safeboxGetMallItemMetinSocket(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BadArgument();
	int iSocketIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSocketIndex))
		return Py_BadArgument();

	if (iSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return Py_BuildException();

	network::TItemData* pItemData;
	if (!CPythonSafeBox::Instance().GetMallItemDataPtr(iSlotIndex, &pItemData))
		return Py_BuildException();

	return Py_BuildValue("i", pItemData->sockets(iSocketIndex));
}

PyObject * safeboxGetMallItemAttribute(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();
	int iAttrSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttrSlotIndex))
		return Py_BuildException();

	if (iAttrSlotIndex >= 0 && iAttrSlotIndex < ITEM_ATTRIBUTE_SLOT_MAX_NUM)
	{
		network::TItemData* pItemData;
		if (CPythonSafeBox::Instance().GetMallItemDataPtr(iSlotIndex, &pItemData))
			return Py_BuildValue("ii", pItemData->attributes(iAttrSlotIndex).type(), pItemData->attributes(iAttrSlotIndex).value());
	}

	return Py_BuildValue("ii", 0, 0);
}

PyObject * safeboxGetMallSize(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonSafeBox::Instance().GetMallSize());
}

#ifdef ENABLE_GUILD_SAFEBOX
PyObject * safeboxIsGuildEnabled(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("b", CPythonSafeBox::Instance().IsGuildEnabled());
}

PyObject * safeboxGetGuildItemID(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetGuildItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->vnum());
}

PyObject * safeboxGetGuildItemCount(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetGuildItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->count());
}

#ifdef ENABLE_ALPHA_EQUIP
PyObject * safeboxGetGuildItemAlphaEquip(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonSafeBox::Instance().GetGuildItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->alpha_equip());
}
#endif

PyObject * safeboxGetGuildItemMetinSocket(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BadArgument();
	int iSocketIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSocketIndex))
		return Py_BadArgument();

	if (iSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return Py_BuildException();

	network::TItemData* pItemData;
	if (!CPythonSafeBox::Instance().GetGuildItemDataPtr(iSlotIndex, &pItemData))
		return Py_BuildException();

	return Py_BuildValue("i", pItemData->sockets(iSocketIndex));
}

PyObject * safeboxGetGuildItemAttribute(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();
	int iAttrSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttrSlotIndex))
		return Py_BuildException();

	if (iAttrSlotIndex >= 0 && iAttrSlotIndex < ITEM_ATTRIBUTE_SLOT_MAX_NUM)
	{
		network::TItemData* pItemData;
		if (CPythonSafeBox::Instance().GetGuildItemDataPtr(iSlotIndex, &pItemData))
			return Py_BuildValue("ii", pItemData->attributes(iAttrSlotIndex).type(), pItemData->attributes(iAttrSlotIndex).value());
	}

	return Py_BuildValue("ii", 0, 0);
}

PyObject * safeboxGetGuildSize(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonSafeBox::Instance().GetGuildSize());
}

PyObject * safeboxGetGuildMoney(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("L", CPythonSafeBox::Instance().GetGuildMoney());
}

PyObject * safeboxGetGuildLogCount(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonSafeBox::Instance().GetGuildLogInfoCount());
}

PyObject * safeboxGetGuildLogType(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pTab->type());
}

PyObject * safeboxGetGuildLogPlayerName(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("s", "");

	return Py_BuildValue("s", pTab->player_name().c_str());
}

PyObject * safeboxGetGuildLogItemVnum(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pTab->item().vnum());
}

PyObject * safeboxGetGuildLogItemInfo(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("ii", 0, 0);

	return Py_BuildValue("ii", pTab->item().vnum(), pTab->item().count());
}

PyObject * safeboxGetGuildLogItemSocket(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	int iSubIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSubIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pTab->item().sockets_size() > iSubIndex ? (int)pTab->item().sockets(iSubIndex) : 0);
}

PyObject * safeboxGetGuildLogItemAttribute(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	int iSubIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSubIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("ii", 0, 0);

	return Py_BuildValue("ii",
		pTab->item().attributes_size() > iSubIndex ? pTab->item().attributes(iSubIndex).type() : 0,
		pTab->item().attributes_size() > iSubIndex ? pTab->item().attributes(iSubIndex).value() : 0);
}

PyObject * safeboxGetGuildLogGold(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("L", (long long) 0);

	return Py_BuildValue("L", (long long) pTab->gold());
}

PyObject * safeboxGetGuildLogTime(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pTab = CPythonSafeBox::Instance().GetGuildLogInfo(iIndex);
	if (!pTab)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pTab->time());
}
#endif

void initsafebox()
{
	static PyMethodDef s_methods[] =
	{
		// SafeBox
		{ "GetCurrentSafeboxSize",		safeboxGetCurrentSafeboxSize,			METH_VARARGS },
		{ "GetItemID",					safeboxGetItemID,						METH_VARARGS },
		{ "GetItemCount",				safeboxGetItemCount,					METH_VARARGS },
#ifdef ENABLE_ALPHA_EQUIP
		{ "GetItemAlphaEquip",			safeboxGetItemAlphaEquip,				METH_VARARGS },
#endif
		{ "GetItemMetinSocket",			safeboxGetItemMetinSocket,				METH_VARARGS },
		{ "GetItemAttribute",			safeboxGetItemAttribute,				METH_VARARGS },
		{ "GetMoney",					safeboxGetMoney,						METH_VARARGS },

		// Mall
		{ "GetMallItemID",				safeboxGetMallItemID,					METH_VARARGS },
		{ "GetMallItemCount",			safeboxGetMallItemCount,				METH_VARARGS },
#ifdef ENABLE_ALPHA_EQUIP
		{ "GetMallItemAlphaEquip",		safeboxGetMallItemAlphaEquip,			METH_VARARGS },
#endif
		{ "GetMallItemMetinSocket",		safeboxGetMallItemMetinSocket,			METH_VARARGS },
		{ "GetMallItemAttribute",		safeboxGetMallItemAttribute,			METH_VARARGS },
		{ "GetMallSize",				safeboxGetMallSize,						METH_VARARGS },
		
#ifdef ENABLE_GUILD_SAFEBOX
		{ "IsGuildEnabled",				safeboxIsGuildEnabled,					METH_VARARGS },
		{ "GetGuildItemID",				safeboxGetGuildItemID,					METH_VARARGS },
		{ "GetGuildItemCount",			safeboxGetGuildItemCount,				METH_VARARGS },
#ifdef ENABLE_ALPHA_EQUIP
		{ "GetGuildItemAlphaEquip",		safeboxGetGuildItemAlphaEquip,			METH_VARARGS },
#endif
		{ "GetGuildItemMetinSocket",	safeboxGetGuildItemMetinSocket,			METH_VARARGS },
		{ "GetGuildItemAttribute",		safeboxGetGuildItemAttribute,			METH_VARARGS },
		{ "GetGuildSize",				safeboxGetGuildSize,					METH_VARARGS },
		{ "GetGuildMoney",				safeboxGetGuildMoney,					METH_VARARGS },

		{ "GetGuildLogCount",			safeboxGetGuildLogCount,				METH_VARARGS },
		{ "GetGuildLogType",			safeboxGetGuildLogType,					METH_VARARGS },
		{ "GetGuildLogPlayerName",		safeboxGetGuildLogPlayerName,			METH_VARARGS },
		{ "GetGuildLogItemVnum",		safeboxGetGuildLogItemVnum,				METH_VARARGS },
		{ "GetGuildLogItemInfo",		safeboxGetGuildLogItemInfo,				METH_VARARGS },
		{ "GetGuildLogItemSocket",		safeboxGetGuildLogItemSocket,			METH_VARARGS },
		{ "GetGuildLogItemAttribute",	safeboxGetGuildLogItemAttribute,		METH_VARARGS },
		{ "GetGuildLogGold",			safeboxGetGuildLogGold,					METH_VARARGS },
		{ "GetGuildLogTime",			safeboxGetGuildLogTime,					METH_VARARGS },
#endif

		{ NULL,							NULL,									NULL },
	};

	PyObject * poModule = Py_InitModule("safebox", s_methods);
	PyModule_AddIntConstant(poModule, "SAFEBOX_SLOT_X_COUNT", CPythonSafeBox::SAFEBOX_SLOT_X_COUNT);
	PyModule_AddIntConstant(poModule, "SAFEBOX_SLOT_Y_COUNT", CPythonSafeBox::SAFEBOX_SLOT_Y_COUNT);
	PyModule_AddIntConstant(poModule, "SAFEBOX_PAGE_SIZE", CPythonSafeBox::SAFEBOX_PAGE_SIZE);

#ifdef ENABLE_GUILD_SAFEBOX
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX_LOG_CREATE", GUILD_SAFEBOX_LOG_CREATE);
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX_LOG_ITEM_GIVE", GUILD_SAFEBOX_LOG_ITEM_GIVE);
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX_LOG_ITEM_TAKE", GUILD_SAFEBOX_LOG_ITEM_TAKE);
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX_LOG_GOLD_GIVE", GUILD_SAFEBOX_LOG_GOLD_GIVE);
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX_LOG_GOLD_TAKE", GUILD_SAFEBOX_LOG_GOLD_TAKE);
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX_LOG_SIZE", GUILD_SAFEBOX_LOG_SIZE);
#endif
}
