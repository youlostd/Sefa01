#pragma once

#include "../eterLib/StdAfx.h"
#include "../eterGrnLib/StdAfx.h"

//#include <crtdbg.h>
#include <Python27/python.h>
#include <Python27/node.h>
#include <Python27/grammar.h>
// #include <Python27/token.h>
#include <Python27/parsetok.h>
#include <Python27/errcode.h>
#include <Python27/compile.h>
typedef struct _mod *mod_ty;
#include <Python27/symtable.h>
#include <Python27/eval.h>
#include <Python27/marshal.h>

#include "PythonUtils.h"
#include "PythonLauncher.h"
#include "PythonMarshal.h"
#include "Resource.h"

void initdbg();

// PYTHON_EXCEPTION_SENDER
class IPythonExceptionSender
{
	public:
		void Clear()
		{
			m_strExceptionString = "";
		}

		void RegisterExceptionString(const char * c_szString)
		{
			m_strExceptionString += c_szString;
		}

		virtual void Send() = 0;

	protected:
		std::string m_strExceptionString;
};

extern IPythonExceptionSender * g_pkExceptionSender;

void SetExceptionSender(IPythonExceptionSender * pkExceptionSender);
// END_OF_PYTHON_EXCEPTION_SENDER
