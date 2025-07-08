#include "StdAfx.h"

#ifdef ENABLE_RUNE_SYSTEM
#include "PythonRune.h"
#include "PythonNetworkStream.h"

// proto
PyObject * runeGetProto(PyObject * poSelf, PyObject * poArgs)
{
	int iID;
	if (!PyTuple_GetInteger(poArgs, 0, &iID))
		return Py_BadArgument();

	CPythonRune::TRuneProto* pTable = CPythonRune::Instance().GetProto(iID);
	if (!pTable)
		return Py_BuildValue("iiisi", 0, 0, 0, "", 0);

	return Py_BuildValue("iiisi", pTable->group, pTable->sub_group, pTable->sub_group_index, pTable->name.c_str(), pTable->apply_type);
}

PyObject * runeGetProtoName(PyObject * poSelf, PyObject * poArgs)
{
	int iID;
	if (!PyTuple_GetInteger(poArgs, 0, &iID))
		return Py_BadArgument();

	CPythonRune::TRuneProto* pTable = CPythonRune::Instance().GetProto(iID);
	if (!pTable)
		return Py_BuildValue("s", "");

	return Py_BuildValue("s", pTable->name.c_str());
}

PyObject * runeGetProtoDesc(PyObject * poSelf, PyObject * poArgs)
{
	int iID;
	if (!PyTuple_GetInteger(poArgs, 0, &iID))
		return Py_BadArgument();

	CPythonRune::TRuneProto* pTable = CPythonRune::Instance().GetProto(iID);
	if (!pTable)
		return Py_BuildValue("s", "");

	return Py_BuildValue("s", pTable->desc.c_str());
}

PyObject * runeGetProtoApplyValue(PyObject * poSelf, PyObject * poArgs)
{
	int iID;
	if (!PyTuple_GetInteger(poArgs, 0, &iID))
		return Py_BadArgument();

	CPythonRune::TRuneProto* pTable = CPythonRune::Instance().GetProto(iID);
	if (!pTable)
		return Py_BuildValue("f", 0.0f);

	return Py_BuildValue("f", CPythonRune::Instance().EvalApplyValue(pTable->apply_eval));
}

PyObject * runeGetIDByIndex(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bGroup;
	if (!PyTuple_GetInteger(poArgs, 0, &bGroup))
		return Py_BadArgument();
	BYTE bSubGroup;
	if (!PyTuple_GetInteger(poArgs, 1, &bSubGroup))
		return Py_BadArgument();
	BYTE bIndex;
	if (!PyTuple_GetInteger(poArgs, 2, &bIndex))
		return Py_BadArgument();

	CPythonRune::TRuneProto* pTable = CPythonRune::Instance().FindProtoByIndex(bGroup, bSubGroup, bIndex);
	return Py_BuildValue("i", pTable ? pTable->id : 0);
}

PyObject * runeHasRune(PyObject * poSelf, PyObject * poArgs)
{
	int iVnum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVnum))
		return Py_BadArgument();

	return Py_BuildValue("b", CPythonRune::Instance().HasRune(iVnum));
}

PyObject * runePageGetType(PyObject * poSelf, PyObject * poArgs)
{
	bool bIsNew = false;
	PyTuple_GetBoolean(poArgs, 0, &bIsNew);

	CPythonRune& rkRune = CPythonRune::Instance();
	const TRunePageData& c_rkData = bIsNew ? rkRune.GetNewRunePage() : rkRune.GetRunePage();

	return Py_BuildValue("i", c_rkData.main_group());
}

PyObject * runePageGetVnum(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	bool bIsNew = false;
	PyTuple_GetBoolean(poArgs, 1, &bIsNew);

	CPythonRune& rkRune = CPythonRune::Instance();
	const TRunePageData& c_rkData = bIsNew ? rkRune.GetNewRunePage() : rkRune.GetRunePage();

	if (iIndex < 0 || iIndex >= RUNE_MAIN_COUNT)
		return Py_BuildException("invalid rune main index %d", iIndex);

	return Py_BuildValue("i", c_rkData.main_vnum_size() > iIndex ? c_rkData.main_vnum(iIndex) : 0);
}

PyObject * runePageGetSubType(PyObject * poSelf, PyObject * poArgs)
{
	bool bIsNew = false;
	PyTuple_GetBoolean(poArgs, 0, &bIsNew);

	CPythonRune& rkRune = CPythonRune::Instance();
	const TRunePageData& c_rkData = bIsNew ? rkRune.GetNewRunePage() : rkRune.GetRunePage();

	return Py_BuildValue("i", c_rkData.sub_group());
}

PyObject * runePageGetSubVnum(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	bool bIsNew = false;
	PyTuple_GetBoolean(poArgs, 1, &bIsNew);

	CPythonRune& rkRune = CPythonRune::Instance();
	const TRunePageData& c_rkData = bIsNew ? rkRune.GetNewRunePage() : rkRune.GetRunePage();

	if (iIndex < 0 || iIndex >= RUNE_SUB_COUNT)
		return Py_BuildException("invalid rune sub index %d", iIndex);

	return Py_BuildValue("i", c_rkData.sub_vnum_size() > iIndex ? c_rkData.sub_vnum(iIndex) : 0);
}

PyObject * runePageSetType(PyObject * poSelf, PyObject * poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BadArgument();

	TRunePageData& rkData = CPythonRune::Instance().GetNewRunePage();
	rkData.set_main_group(iType);

	rkData.clear_main_vnum();
	rkData.set_sub_group(-1);
	rkData.clear_sub_vnum();

	return Py_BuildNone();
}

PyObject * runePageSetVnum(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	int iVnum;
	if (!PyTuple_GetInteger(poArgs, 1, &iVnum))
		return Py_BadArgument();

	if (iIndex < 0 || iIndex >= RUNE_MAIN_COUNT)
		return Py_BuildException("invalid main rune index for set cmd %d", iIndex);

	TRunePageData& c_rkData = CPythonRune::Instance().GetNewRunePage();
	while (iIndex >= c_rkData.main_vnum_size())
		c_rkData.add_main_vnum(0);
	c_rkData.set_main_vnum(iIndex, iVnum);

	return Py_BuildNone();
}

PyObject * runePageSetSubType(PyObject * poSelf, PyObject * poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BadArgument();

	TRunePageData& rkData = CPythonRune::Instance().GetNewRunePage();
	rkData.set_sub_group(iType);

	rkData.clear_sub_vnum();

	return Py_BuildNone();
}

PyObject * runePageSetSubVnum(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	int iVnum;
	if (!PyTuple_GetInteger(poArgs, 1, &iVnum))
		return Py_BadArgument();

	if (iIndex < 0 || iIndex >= RUNE_SUB_COUNT)
		return Py_BuildException("invalid sub rune index for set cmd %d", iIndex);

	TRunePageData& c_rkData = CPythonRune::Instance().GetNewRunePage();
	while (iIndex >= c_rkData.sub_vnum_size())
		c_rkData.add_sub_vnum(0);
	c_rkData.set_sub_vnum(iIndex, iVnum);

	return Py_BuildNone();
}

PyObject * runePageIsNewChanged(PyObject * poSelf, PyObject * poArgs)
{
	const TRunePageData& c_rkData = CPythonRune::Instance().GetRunePage();
	TRunePageData& c_rkDataNew = CPythonRune::Instance().GetNewRunePage();

	return Py_BuildValue("b", c_rkData.SerializeAsString() != c_rkDataNew.SerializeAsString());
}

PyObject * runePageResetNew(PyObject * poSelf, PyObject * poArgs)
{
	const TRunePageData& c_rkData = CPythonRune::Instance().GetRunePage();
	TRunePageData& c_rkDataNew = CPythonRune::Instance().GetNewRunePage();
	c_rkDataNew = c_rkData;
	
	return Py_BuildNone();
}

PyObject * runePageSaveNew(PyObject * poSelf, PyObject * poArgs)
{
	TRunePageData& c_rkDataNew = CPythonRune::Instance().GetNewRunePage();

	network::CGOutputPacket<network::CGRunePagePacket> pack;
	*pack->mutable_data() = c_rkDataNew;

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.Send(pack);

	return Py_BuildNone();
}

void initRune()
{
	static PyMethodDef s_methods[] =
	{
		// proto
		{ "GetProto",					runeGetProto,				METH_VARARGS },
		{ "GetProtoName",				runeGetProtoName,			METH_VARARGS },
		{ "GetProtoDesc",				runeGetProtoDesc,			METH_VARARGS },
		{ "GetProtoApplyValue",			runeGetProtoApplyValue,		METH_VARARGS },

		{ "GetIDByIndex",				runeGetIDByIndex,			METH_VARARGS },

		// runes
		{ "HasRune",					runeHasRune,				METH_VARARGS },

		// pages
		{ "PageGetType",				runePageGetType,			METH_VARARGS },
		{ "PageGetVnum",				runePageGetVnum,			METH_VARARGS },
		{ "PageGetSubType",				runePageGetSubType,			METH_VARARGS },
		{ "PageGetSubVnum",				runePageGetSubVnum,			METH_VARARGS },

		{ "PageSetType",				runePageSetType,			METH_VARARGS },
		{ "PageSetVnum",				runePageSetVnum,			METH_VARARGS },
		{ "PageSetSubType",				runePageSetSubType,			METH_VARARGS },
		{ "PageSetSubVnum",				runePageSetSubVnum,			METH_VARARGS },

		{ "PageIsNewChanged",			runePageIsNewChanged,		METH_VARARGS },
		{ "PageResetNew",				runePageResetNew,			METH_VARARGS },
		{ "PageSaveNew",				runePageSaveNew,			METH_VARARGS },

		{ NULL, NULL, NULL },
	};

	PyObject * poModule = Py_InitModule("rune", s_methods);

	PyModule_AddIntConstant(poModule, "GROUP_PRECISION",		CPythonRune::GROUP_PRECISION);
	PyModule_AddIntConstant(poModule, "GROUP_DOMINATION",		CPythonRune::GROUP_DOMINATION);
	PyModule_AddIntConstant(poModule, "GROUP_SORCERY",			CPythonRune::GROUP_SORCERY);
	PyModule_AddIntConstant(poModule, "GROUP_RESOLVE",			CPythonRune::GROUP_RESOLVE);
	PyModule_AddIntConstant(poModule, "GROUP_INSPIRATION",		CPythonRune::GROUP_INSPIRATION);
	PyModule_AddIntConstant(poModule, "GROUP_MAX_NUM",			CPythonRune::GROUP_MAX_NUM);
	
	PyModule_AddIntConstant(poModule, "SUBGROUP_PRIMARY",		CPythonRune::SUBGROUP_PRIMARY);
	PyModule_AddIntConstant(poModule, "SUBGROUP_SECONDARY1",	CPythonRune::SUBGROUP_SECONDARY1);
	PyModule_AddIntConstant(poModule, "SUBGROUP_SECONDARY2",	CPythonRune::SUBGROUP_SECONDARY2);
	PyModule_AddIntConstant(poModule, "SUBGROUP_SECONDARY3",	CPythonRune::SUBGROUP_SECONDARY3);
	PyModule_AddIntConstant(poModule, "SUBGROUP_MAX_NUM",		CPythonRune::SUBGROUP_MAX_NUM);
	PyModule_AddIntConstant(poModule, "SUBGROUP_SECONDARY_MAX", CPythonRune::SUBGROUP_SECONDARY_MAX);
	
	PyModule_AddIntConstant(poModule, "RUNE_MAIN_COUNT",		RUNE_MAIN_COUNT);
	PyModule_AddIntConstant(poModule, "RUNE_SUB_COUNT",			RUNE_SUB_COUNT);
};
#endif
