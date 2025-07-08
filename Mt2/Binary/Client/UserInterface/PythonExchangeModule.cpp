#include "stdafx.h"
#include "PythonExchange.h"

PyObject * exchangeInitTrading(PyObject * poSelf, PyObject * poArgs)
{
	CPythonExchange::Instance().End();
	return Py_BuildNone();
}

PyObject * exchangeisTrading(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonExchange::Instance().isTrading());
}

PyObject * exchangeGetElkFromSelf(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("L", CPythonExchange::Instance().GetElkFromSelf());
}

PyObject * exchangeGetElkFromTarget(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("L", CPythonExchange::Instance().GetElkFromTarget());
}

PyObject * exchangeGetAcceptFromSelf(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonExchange::Instance().GetAcceptFromSelf());
}

PyObject * exchangeGetAcceptFromTarget(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonExchange::Instance().GetAcceptFromTarget());
}

PyObject* exchangeGetItemDataPtrFromSelf(PyObject* poSelf, PyObject* poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromSelf(pos);
	return Py_BuildValue("i", item);
}

PyObject* exchangeGetItemDataPtrFromTarget(PyObject* poSelf, PyObject* poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromTarget(pos);
	return Py_BuildValue("i", item);
}

PyObject * exchangeGetItemVnumFromSelf(PyObject * poSelf, PyObject * poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromSelf(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->vnum());
}

PyObject * exchangeGetItemVnumFromTarget(PyObject * poTarget, PyObject * poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromTarget(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->vnum());
}

PyObject * exchangeGetItemCountFromSelf(PyObject * poSelf, PyObject * poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromSelf(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->count());
}

PyObject * exchangeGetItemCountFromTarget(PyObject * poTarget, PyObject * poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromTarget(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->count());
}

#ifdef ENABLE_ALPHA_EQUIP
PyObject * exchangeGetItemAlphaEquipFromSelf(PyObject * poSelf, PyObject * poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromSelf(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->alpha_equip());
}

PyObject * exchangeGetItemAlphaEquipFromTarget(PyObject * poTarget, PyObject * poArgs)
{
	int pos;

	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromTarget(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->alpha_equip());
}
#endif

PyObject * exchangeGetNameFromSelf(PyObject * poTarget, PyObject * poArgs)
{
	return Py_BuildValue("s", CPythonExchange::Instance().GetNameFromSelf());
}

PyObject * exchangeGetNameFromTarget(PyObject * poTarget, PyObject * poArgs)
{
	return Py_BuildValue("s", CPythonExchange::Instance().GetNameFromTarget());
}

PyObject * exchangeGetItemMetinSocketFromTarget(PyObject * poTarget, PyObject * poArgs)
{
	int pos;
	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();
	int iMetinSocketPos;
	if (!PyTuple_GetInteger(poArgs, 1, &iMetinSocketPos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromTarget(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->sockets_size() > iMetinSocketPos ? item->sockets(iMetinSocketPos) : 0);
}

PyObject * exchangeGetItemMetinSocketFromSelf(PyObject * poTarget, PyObject * poArgs)
{
	int pos;
	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();
	int iMetinSocketPos;
	if (!PyTuple_GetInteger(poArgs, 1, &iMetinSocketPos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromSelf(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", item->sockets_size() > iMetinSocketPos ? item->sockets(iMetinSocketPos) : 0);
}

PyObject * exchangeGetItemAttributeFromTarget(PyObject * poTarget, PyObject * poArgs)
{
	int pos;
	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();
	int iAttrSlotPos;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttrSlotPos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromTarget(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("ii",
		item->attributes_size() > iAttrSlotPos ? item->attributes(iAttrSlotPos).type() : 0,
		item->attributes_size() > iAttrSlotPos ? item->attributes(iAttrSlotPos).value() : 0);
}

PyObject * exchangeGetItemAttributeFromSelf(PyObject * poTarget, PyObject * poArgs)
{
	int pos;
	if (!PyTuple_GetInteger(poArgs, 0, &pos))
		return Py_BadArgument();
	int iAttrSlotPos;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttrSlotPos))
		return Py_BadArgument();

	auto item = CPythonExchange::Instance().GetItemFromSelf(pos);
	if (!item)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("ii",
		item->attributes_size() > iAttrSlotPos ? item->attributes(iAttrSlotPos).type() : 0,
		item->attributes_size() > iAttrSlotPos ? item->attributes(iAttrSlotPos).value() : 0);
}

PyObject * exchangeGetElkMode(PyObject * poTarget, PyObject * poArgs)
{
	return Py_BuildValue("b", CPythonExchange::Instance().GetElkMode());
}

PyObject * exchangeSetElkMode(PyObject * poTarget, PyObject * poArgs)
{
	int elk_mode;

	if (!PyTuple_GetInteger(poArgs, 0, &elk_mode))
		return Py_BuildException();

	CPythonExchange::Instance().SetElkMode(elk_mode ? true : false);
	return Py_BuildNone();
}

void initTrade()
{
	static PyMethodDef s_methods[] = 
	{
		{"InitTrading",					exchangeInitTrading,				METH_VARARGS},
		{"isTrading",					exchangeisTrading,					METH_VARARGS},

		{"GetElkFromSelf",				exchangeGetElkFromSelf,				METH_VARARGS},
		{"GetElkFromTarget",			exchangeGetElkFromTarget,			METH_VARARGS},

		{"GetItemDataPtrFromSelf",		exchangeGetItemDataPtrFromSelf,		METH_VARARGS},
		{"GetItemDataPtrFromTarget",	exchangeGetItemDataPtrFromTarget,	METH_VARARGS},

		{"GetItemVnumFromSelf",			exchangeGetItemVnumFromSelf,		METH_VARARGS},
		{"GetItemVnumFromTarget",		exchangeGetItemVnumFromTarget,		METH_VARARGS},

		{"GetItemCountFromSelf",		exchangeGetItemCountFromSelf,		METH_VARARGS},
		{"GetItemCountFromTarget",		exchangeGetItemCountFromTarget,		METH_VARARGS},

#ifdef ENABLE_ALPHA_EQUIP
		{"GetItemAlphaEquipFromSelf",	exchangeGetItemAlphaEquipFromSelf,		METH_VARARGS},
		{"GetItemAlphaEquipFromTarget",	exchangeGetItemAlphaEquipFromTarget,	METH_VARARGS},
#endif

		{"GetAcceptFromSelf",			exchangeGetAcceptFromSelf,			METH_VARARGS},
		{"GetAcceptFromTarget",			exchangeGetAcceptFromTarget,		METH_VARARGS},

		{"GetNameFromSelf",				exchangeGetNameFromSelf,			METH_VARARGS},
		{"GetNameFromTarget",			exchangeGetNameFromTarget,			METH_VARARGS},

		{"GetItemMetinSocketFromTarget",	exchangeGetItemMetinSocketFromTarget,	METH_VARARGS},
		{"GetItemMetinSocketFromSelf",		exchangeGetItemMetinSocketFromSelf,		METH_VARARGS},

		{"GetItemAttributeFromTarget",	exchangeGetItemAttributeFromTarget,	METH_VARARGS},
		{"GetItemAttributeFromSelf",	exchangeGetItemAttributeFromSelf,	METH_VARARGS},

		{"GetElkMode",					exchangeGetElkMode,					METH_VARARGS},
		{"SetElkMode",					exchangeSetElkMode,					METH_VARARGS},

		{NULL, NULL},
	};

	PyObject * poModule = Py_InitModule("exchange", s_methods);
	PyModule_AddIntConstant(poModule, "EXCHANGE_ITEM_MAX_NUM", CPythonExchange::EXCHANGE_ITEM_MAX_NUM);
}