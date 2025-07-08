#include "StdAfx.h"

bool PyTuple_GetItemPtr(PyObject * poArgs, int pos, network::TItemData** ptr)
{
	int tmp;
	if (!PyTuple_GetInteger(poArgs, pos, &tmp))
		return false;

	if (tmp == 0)
		return false;

	*ptr = (network::TItemData*) tmp;
	return true;
}

PyObject * itemPtrNewCopy(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("i", new network::TItemData(*item));
}

PyObject * itemPtrDeleteCopy(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	delete item;

	return Py_BuildNone();
}

PyObject * itemPtrGetVnum(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("i", item->vnum());
}

PyObject * itemPtrGetCount(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("i", item->count());
}

PyObject * itemPtrGetSpecialFlag(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("i", item->special_flag());
}

PyObject * itemPtrGetPrice(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("L", (long long) item->price());
}

PyObject * itemPtrGetSocket(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();
	BYTE index;
	if (!PyTuple_GetInteger(poArgs, 1, &index))
		return Py_BadArgument();

	return Py_BuildValue("i", item->sockets_size() > index ? item->sockets(index) : 0);
}

PyObject * itemPtrGetAttribute(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();
	BYTE index;
	if (!PyTuple_GetInteger(poArgs, 1, &index))
		return Py_BadArgument();

	return Py_BuildValue("ii",
		item->attributes_size() > index ? item->attributes(index).type() : 0,
		item->attributes_size() > index ? item->attributes(index).value() : 0);
}

#ifdef ENABLE_ITEM_NICKNAME
PyObject * itemPtrGetNicknameVnum(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("i", item->nickname_vnum());
}

PyObject * itemPtrGetNicknameTimeout(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	return Py_BuildValue("i", item->nickname_timeout());
}
#endif

#ifdef ENABLE_PET_ADVANCED
PyObject * itemPtrGetPetName(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	auto& pet = item->pet_info();
	return Py_BuildValue("s", pet.name().c_str());
}

PyObject * itemPtrGetPetLevel(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();

	auto& pet = item->pet_info();
	return Py_BuildValue("i", pet.level());
}

PyObject * itemPtrGetPetSkillVnum(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();
	BYTE index;
	if (!PyTuple_GetInteger(poArgs, 1, &index))
		return Py_BadArgument();

	auto& pet = item->pet_info();
	return Py_BuildValue("i", pet.skills_size() > index ? pet.skills(index).vnum() : 0);
}

PyObject * itemPtrGetPetSkillLevel(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();
	BYTE index;
	if (!PyTuple_GetInteger(poArgs, 1, &index))
		return Py_BadArgument();

	auto& pet = item->pet_info();
	return Py_BuildValue("i", pet.skills_size() > index ? pet.skills(index).vnum() : 0);
}
#endif

PyObject * itemPtrSetSocket(PyObject * poSelf, PyObject * poArgs)
{
	network::TItemData* item;
	if (!PyTuple_GetItemPtr(poArgs, 0, &item))
		return Py_BadArgument();
	BYTE index;
	if (!PyTuple_GetInteger(poArgs, 1, &index))
		return Py_BadArgument();
	int value;
	if (!PyTuple_GetInteger(poArgs, 2, &value))
		return Py_BadArgument();

	while (item->sockets_size() <= index)
		item->add_sockets(0);

	item->set_sockets(index, value);

	return Py_BuildNone();
}

void initItemPtr()
{
	static PyMethodDef s_methods[] =
	{
		{ "NewCopy",				itemPtrNewCopy,				METH_VARARGS },
		{ "DeleteCopy",				itemPtrDeleteCopy,			METH_VARARGS },

		{ "GetVnum",				itemPtrGetVnum,				METH_VARARGS },
		{ "GetCount",				itemPtrGetCount,			METH_VARARGS },
		{ "GetSpecialFlag",			itemPtrGetSpecialFlag,		METH_VARARGS },
		{ "GetPrice",				itemPtrGetPrice,			METH_VARARGS },
		{ "GetSocket",				itemPtrGetSocket,			METH_VARARGS },
		{ "GetAttribute",			itemPtrGetAttribute,		METH_VARARGS },
#ifdef ENABLE_ITEM_NICKNAME
		{ "GetNicknameVnum",		itemPtrGetNicknameVnum,		METH_VARARGS },
		{ "GetNicknameTimeout",		itemPtrGetNicknameTimeout,	METH_VARARGS },
#endif
#ifdef ENABLE_PET_ADVANCED
		{ "GetPetName",				itemPtrGetPetName,			METH_VARARGS },
		{ "GetPetLevel",			itemPtrGetPetLevel,			METH_VARARGS },
		{ "GetPetSkillVnum",		itemPtrGetPetSkillVnum,		METH_VARARGS },
		{ "GetPetSkillLevel",		itemPtrGetPetSkillLevel,	METH_VARARGS },
#endif

		{ "SetSocket",				itemPtrSetSocket,			METH_VARARGS },

		{ NULL,						NULL,						NULL		 },
	};

	PyObject * poModule = Py_InitModule("itemPtr", s_methods);
}
