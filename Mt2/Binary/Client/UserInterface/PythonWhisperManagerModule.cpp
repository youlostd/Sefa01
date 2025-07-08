#include "StdAfx.h"
#include "PythonWhisperManager.h"

PyObject * whispermgrSetGameWindow(PyObject * poSelf, PyObject * poArgs)
{
	PyObject* poGameWnd;
	if (!PyTuple_GetObject(poArgs, 0, &poGameWnd))
		return Py_BuildException();

	CPythonWhisperManager::Instance().SetGameWindow(poGameWnd);
	return Py_BuildNone();
}

PyObject * whispermgrSetFileName(PyObject * poSelf, PyObject * poArgs)
{
	char* szFileName;
	if (!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BadArgument();

	CPythonWhisperManager::Instance().SetFileName(szFileName);
	return Py_BuildNone();
}

PyObject * whispermgrGetFileName(PyObject * poSelf, PyObject * poArgs)
{
	const char* szFileName = CPythonWhisperManager::Instance().GetFileName();
	return Py_BuildValue("s", szFileName);
}

PyObject * whispermgrClear(PyObject * poSelf, PyObject * poArgs)
{
	CPythonWhisperManager::Instance().Destroy();
	return Py_BuildNone();
}

PyObject * whispermgrLoad(PyObject * poSelf, PyObject * poArgs)
{
	CPythonWhisperManager::Instance().Load();
	return Py_BuildNone();
}

PyObject * whispermgrSave(PyObject * poSelf, PyObject * poArgs)
{
	CPythonWhisperManager::Instance().Save();
	return Py_BuildNone();
}

PyObject * whispermgrOpenWhisper(PyObject * poSelf, PyObject * poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BadArgument();

	CPythonWhisperManager::instance().OpenWhisper(szName);
	return Py_BuildNone();
}

PyObject * whispermgrCloseWhisper(PyObject * poSelf, PyObject * poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BadArgument();

	CPythonWhisperManager::instance().CloseWhisper(szName);
	return Py_BuildNone();
}

PyObject * whispermgrClearWhisper(PyObject * poSelf, PyObject * poArgs)
{
	char* szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BadArgument();

	CPythonWhisperManager::instance().ClearWhisper(szName);
	return Py_BuildNone();
}

void initwhispermgr()
{
	static PyMethodDef s_methods[] =
	{
		{ "SetGameWindow",			whispermgrSetGameWindow,		METH_VARARGS	},
		{ "SetFileName",			whispermgrSetFileName,			METH_VARARGS	},
		{ "GetFileName",			whispermgrGetFileName,			METH_VARARGS	},
		
		{ "Clear",					whispermgrClear,				METH_VARARGS	},
		{ "Load",					whispermgrLoad,					METH_VARARGS	},
		{ "Save",					whispermgrSave,					METH_VARARGS	},
		
		{ "OpenWhisper",			whispermgrOpenWhisper,			METH_VARARGS	},
		{ "CloseWhisper",			whispermgrCloseWhisper,			METH_VARARGS	},
		{ "ClearWhisper",			whispermgrClearWhisper,			METH_VARARGS	},

		{ NULL,						NULL,							NULL			},
	};

	PyObject * poModule = Py_InitModule("whispermgr", s_methods);
}
