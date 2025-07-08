#pragma once

#pragma warning(disable:4702)
#pragma warning(disable:4100)
#pragma warning(disable:4201)
#pragma warning(disable:4511)
#pragma warning(disable:4663)
#pragma warning(disable:4018)
#pragma warning(disable:4245)
#pragma warning(disable:4099)

// Define _STATIC_CPPLIB, otherwise you will get "unresoved linker error '__int64 const std::_BADOFF'" in VC2012
#define _STATIC_CPPLIB
#define _DISABLE_DEPRECATE_STATIC_CPPLIB  // disable warnings about defining _STATIC_CPPLIB

#if _MSC_VER >= 1400
//if don't use below, time_t is 64bit
#define _USE_32BIT_TIME_T
#endif

#include <locale.h>
#include "../eterLib/StdAfx.h"
#include "../eterPythonLib/StdAfx.h"
#include "../gameLib/StdAfx.h"
#include "../scriptLib/StdAfx.h"
#include "../milesLib/StdAfx.h"
#include "../EffectLib/StdAfx.h"
#include "../PRTerrainLib/StdAfx.h"
#include "../SpeedTreeLib/StdAfx.h"

#ifndef __D3DRM_H__
#define __D3DRM_H__
#endif

#include <dshow.h>
#include <qedit.h>

#include "Locale.h"
#include "Export.h"

#include "GameType.h"
extern DWORD __DEFAULT_CODE_PAGE__;

#define APP_NAME	"Metin 2"

#ifdef ENABLE_PYTHON_CONFIG
#define WM_NEW_RELOAD_BLOCKLIST 0x1000
#endif

enum
{
	POINT_MAX_NUM = 255,	
	CHARACTER_NAME_MAX_LEN = 35,
	CHAT_MAX_LEN = 512,
};

#ifdef COMBAT_ZONE
enum
{
	COMBAT_ZONE_MAX_ROWS_RANKING = 10,
	COMBAT_ZONE_EMPTY_DATA = 999,
	COMBAT_ZONE_ACTION_OPEN_RANKING = 1,
	COMBAT_ZONE_ACTION_CHANGE_PAGE_RANKING = 2,
	COMBAT_ZONE_ACTION_PARTICIPATE = 3,
	COMBAT_ZONE_ACTION_LEAVE = 4,
	COMBAT_ZONE_ACTION_REQUEST_POTION = 5,
	COMBAT_ZONE_TYPE_RANKING_WEEKLY = 1,
	COMBAT_ZONE_TYPE_RANKING_ALL = 2,
};
#endif

void initapp();
void initime();
void initsystem();
void initchr();
void initchrmgr();
void initChat();
void initTextTail();
void initime();
void initItem();
#ifdef ENABLE_PET_ADVANCED
void initItemPtr();
#endif
void initNonPlayer();
void initnet();
void initPlayer();
void initSectionDisplayer();
void initTrade();
void initMiniMap();
void initEvent();
void initeffect();
void initsnd();
void initeventmgr();
void initBackground();
void initwndMgr();
void initshop();
void initpack();
void initskill();
void initfly();
void initquest();
void initsafebox();
void initguild();
void initMessenger();
#ifdef ENABLE_PYTHON_CONFIG
void initcfg();
#endif
#ifdef ENABLE_ACCOUNT_MANAGER
void initAccountManager();
#endif
#ifdef ENABLE_AUCTION
void initAuction();
#endif
#ifdef COMBAT_ZONE
void initCombatZoneSystem();
#endif
#ifdef ENABLE_RUNE_SYSTEM
void initRune();
#endif
#ifdef INGAME_WIKI
void initWiki();
#endif
void initwhispermgr();
#ifdef ENABLE_PET_ADVANCED
void initPetAdvanced();
#endif

extern const std::string& ApplicationStringTable_GetString(DWORD dwID);
extern const std::string& ApplicationStringTable_GetString(DWORD dwID, LPCSTR szKey);

extern const char* ApplicationStringTable_GetStringz(DWORD dwID);
extern const char* ApplicationStringTable_GetStringz(DWORD dwID, LPCSTR szKey);

extern void ApplicationSetErrorString(const char* szErrorString);

#include "Packet.h"
