#ifndef __INC_METiN_II_DBSERV_STDAFX_H__
#define __INC_METiN_II_DBSERV_STDAFX_H__

#include "../../libthecore/include/stdafx.h"
#include <string>

#ifndef __WIN32__
#include <semaphore.h>
#else
#define isdigit iswdigit
#define isspace iswspace
#endif

#include <vector>
#include "../../common/service.h"
#include "../../common/length.h"
#include "../../common/tables.h"
#include "../../common/singleton.h"
#include "../../common/utils.h"
#include "../../common/stl.h"

#endif
