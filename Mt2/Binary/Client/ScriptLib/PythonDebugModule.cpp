#include "StdAfx.h"

extern IPythonExceptionSender * g_pkExceptionSender;
bool g_bDebugEnabled = false;

PyObject* dbgSetEnabled(PyObject* poSelf, PyObject* poArgs)
{	
	PyTuple_GetBoolean(poArgs, 0, &g_bDebugEnabled);
	return Py_BuildNone();
}

PyObject* dbgIsEnabled(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", g_bDebugEnabled);
}

PyObject* dbgLogBox(PyObject* poSelf, PyObject* poArgs)
{	
	char* szMsg;
	char* szCaption;
	if (!PyTuple_GetString(poArgs, 0, &szMsg))
		return Py_BuildException();
	if (!PyTuple_GetString(poArgs, 1, &szCaption))
	{
		LogBox(szMsg);
	}
	else
	{
		LogBox(szMsg,szCaption);
	}
	return Py_BuildNone();	
}

PyObject* dbgTrace(PyObject* poSelf, PyObject* poArgs)
{
	char* szMsg;
	if (!PyTuple_GetString(poArgs, 0, &szMsg))
		return Py_BuildException();

	Trace(szMsg);
	return Py_BuildNone();
}

PyObject* dbgTracen(PyObject* poSelf, PyObject* poArgs)
{
	char* szMsg;
	if (!PyTuple_GetString(poArgs, 0, &szMsg)) 
		return Py_BuildException();

	Tracen(szMsg);
	return Py_BuildNone();
}

PyObject* dbgTraceError(PyObject* poSelf, PyObject* poArgs)
{
	char* szMsg;
	if (!PyTuple_GetString(poArgs, 0, &szMsg)) 
		return Py_BuildException();
	bool bEnabled = true;
	PyTuple_GetBoolean(poArgs, 1, &bEnabled);

	if (bEnabled)
		TraceError( "%s", szMsg );
	return Py_BuildNone();
}

PyObject* dbgRegisterExceptionString(PyObject* poSelf, PyObject* poArgs)
{
	char* szMsg;
	if (!PyTuple_GetString(poArgs, 0, &szMsg)) 
		return Py_BuildException();

	if (g_pkExceptionSender)
		g_pkExceptionSender->RegisterExceptionString(szMsg);

	return Py_BuildNone();
}

#ifdef EXT_PACKET_ERROR_DUMP
PyObject* dbgExtendedPacketErrorDump(PyObject* poSelf, PyObject* poArgs)
{
	ExtendedPacketErrorDump();
	return Py_BuildNone();
}
#endif

void initdbg()
{
	static PyMethodDef s_methods[] =
	{
		{ "SetEnabled",					dbgSetEnabled,				METH_VARARGS },
		{ "IsEnabled",					dbgIsEnabled,				METH_VARARGS },
		{ "LogBox",						dbgLogBox,					METH_VARARGS },
		{ "Trace",						dbgTrace,					METH_VARARGS },
		{ "Tracen",						dbgTracen,					METH_VARARGS },
		{ "TraceError",					dbgTraceError,				METH_VARARGS },
		{ "RegisterExceptionString",	dbgRegisterExceptionString,	METH_VARARGS },
#ifdef EXT_PACKET_ERROR_DUMP
		{ "ExtendedPacketErrorDump",	dbgExtendedPacketErrorDump,	METH_VARARGS },
#endif
		{ NULL, NULL},
	};	

	Py_InitModule("dbg", s_methods);
}