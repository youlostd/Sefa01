#include "StdAfx.h"

#ifdef ENABLE_PYTHON_CONFIG
#include "PythonApplication.h"
#include "PythonConfig.h"

PyObject * cfgInit(PyObject* poSelf, PyObject* poArgs)
{
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	CPythonConfig::Instance().Initialize(szFileName);

	return Py_BuildNone();
}

PyObject * cfgSet(PyObject* poSelf, PyObject* poArgs)
{
	unsigned char bType;
	if (!PyTuple_GetByte(poArgs, 0, &bType))
		return Py_BuildException();

	char* szKey;
	if (!PyTuple_GetString(poArgs, 1, &szKey))
		return Py_BuildException();

	char* szValue;
	int iValue;
	
	CPythonConfig& rkConfig = CPythonConfig::Instance();

	if (PyTuple_GetString(poArgs, 2, &szValue))
	{
		rkConfig.Write((CPythonConfig::EClassTypes) bType, szKey, szValue);
	}
	else if (PyTuple_GetInteger(poArgs, 2, &iValue))
	{
		rkConfig.Write((CPythonConfig::EClassTypes) bType, szKey, iValue);
	}
	else
		return Py_BuildException();

	return Py_BuildNone();
}

PyObject * cfgGet(PyObject* poSelf, PyObject* poArgs)
{
	unsigned char bType;
	if (!PyTuple_GetByte(poArgs, 0, &bType))
		return Py_BuildException();

	char* szKey;
	if (!PyTuple_GetString(poArgs, 1, &szKey))
		return Py_BuildException();

	char* szDefault;
	if (!PyTuple_GetString(poArgs, 2, &szDefault))
		szDefault = "";

	std::string stRes = CPythonConfig::Instance().GetString((CPythonConfig::EClassTypes) bType, szKey, szDefault);
	const char* c_pszString = stRes.c_str();

	for (int i = 0; i < stRes.length(); ++i)
		if (c_pszString[i] < 0)
			return Py_BuildValue("s", szDefault);

	return Py_BuildValue("s", c_pszString);
}

PyObject * cfgRemove(PyObject* poSelf, PyObject* poArgs)
{
	unsigned char bType;
	if (!PyTuple_GetByte(poArgs, 0, &bType))
		return Py_BuildException();

	CPythonConfig::Instance().RemoveSection((CPythonConfig::EClassTypes) bType);

	return Py_BuildNone();
}

PyObject * cfgLoadBlockNameList(PyObject* poSelf, PyObject* poArgs)
{
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	CPythonConfig::Instance().LoadBlockNameList(szFileName);

	return Py_BuildNone();
}

PyObject * cfgGetBlockNameCount(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonConfig::Instance().GetBlockNameCount());
}

PyObject * cfgGetBlockName(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	return Py_BuildValue("s", CPythonConfig::Instance().GetBlockName(iIndex));
}

PyObject * cfgAddBlockName(PyObject* poSelf, PyObject* poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	CPythonConfig::Instance().AddBlockName(szName);
	CPythonConfig::Instance().SaveBlockNameList();
	CPythonConfig::Instance().SendReloadBlockListToOthers();

	return Py_BuildNone();
}

PyObject * cfgRemoveBlockName(PyObject* poSelf, PyObject* poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	CPythonConfig::Instance().RemoveBlockName(szName);
	CPythonConfig::Instance().SaveBlockNameList();
	CPythonConfig::Instance().SendReloadBlockListToOthers();

	return Py_BuildNone();
}

PyObject * cfgIsBlockName(PyObject* poSelf, PyObject* poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	return Py_BuildValue("b", CPythonConfig::Instance().IsBlockName(szName));
}

PyObject * cfgSaveBlockNameList(PyObject* poSelf, PyObject* poArgs)
{
	CPythonConfig::Instance().SaveBlockNameList();
	return Py_BuildNone();
}

void initcfg()
{
	static PyMethodDef s_methods[] =
	{
		{ "Init",		cfgInit,		METH_VARARGS },
		{ "Set",		cfgSet,			METH_VARARGS },
		{ "Get",		cfgGet,			METH_VARARGS },
		{ "Remove",		cfgRemove,		METH_VARARGS },

		{ "LoadBlockNameList",	cfgLoadBlockNameList,	METH_VARARGS },
		{ "GetBlockNameCount",	cfgGetBlockNameCount,	METH_VARARGS },
		{ "GetBlockName",		cfgGetBlockName,		METH_VARARGS },
		{ "AddBlockName",		cfgAddBlockName,		METH_VARARGS },
		{ "RemoveBlockName",	cfgRemoveBlockName,		METH_VARARGS },
		{ "IsBlockName",		cfgIsBlockName,			METH_VARARGS },
		{ "SaveBlockNameList",	cfgSaveBlockNameList,	METH_VARARGS },

		{ NULL,			NULL }
	};

	PyObject * poModule = Py_InitModule("cfg", s_methods);

	PyModule_AddIntConstant(poModule,	"SAVE_GENERAL",			CPythonConfig::CLASS_GENERAL);
	PyModule_AddIntConstant(poModule,	"SAVE_CHAT",			CPythonConfig::CLASS_CHAT);
	PyModule_AddIntConstant(poModule,	"SAVE_OPTION",			CPythonConfig::CLASS_OPTION);
	PyModule_AddIntConstant(poModule,	"SAVE_UNIQUE",			CPythonConfig::CLASS_UNIQUE);
#ifdef ENABLE_AUCTION
	PyModule_AddIntConstant(poModule,	"SAVE_AUCTIONHOUSE",	CPythonConfig::CLASS_AUCTIONHOUSE);
#endif
	PyModule_AddIntConstant(poModule,	"SAVE_WHISPER",			CPythonConfig::CLASS_WHISPER);
#ifdef ENABLE_PET_SYSTEM
	PyModule_AddIntConstant(poModule,	"SAVE_PET",				CPythonConfig::CLASS_PET);
#endif
	PyModule_AddIntConstant(poModule,	"SAVE_PLAYER",			CPythonConfig::CLASS_PLAYER);

	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_ARMOR",	CPythonConfig::DISABLE_PICKUP_ARMOR);
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_WEAPON",	CPythonConfig::DISABLE_PICKUP_WEAPON);
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_ETC",		CPythonConfig::DISABLE_PICKUP_ETC);
#ifdef NEW_PICKUP_FILTER
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_POTION",	CPythonConfig::DISABLE_PICKUP_POTION);
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_BOOK",	CPythonConfig::DISABLE_PICKUP_BOOK);
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_SIZE_1",	CPythonConfig::DISABLE_PICKUP_SIZE_1);
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_SIZE_2",	CPythonConfig::DISABLE_PICKUP_SIZE_2);
	PyModule_AddIntConstant(poModule, "DISABLE_PICKUP_SIZE_3",	CPythonConfig::DISABLE_PICKUP_SIZE_3);
#endif
}

#endif
