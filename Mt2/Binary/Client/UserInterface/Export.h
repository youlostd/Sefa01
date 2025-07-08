#pragma once

#include "Locale_inc.h"
#include <string>

#ifdef ENABLE_MULTI_DESIGN
extern const std::string& CPythonApplication_GetSelectedDesignName();
extern const std::string& CPythonApplication_GetDefaultDesignName();
#endif

extern void TestChat(const char* c_szString);

extern bool g_bDebugEnabled;
