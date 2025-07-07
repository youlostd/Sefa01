#ifndef __START_POSITION_H
#define __START_POSITION_H

#include "sectree_manager.h"

namespace start_private {
	extern DWORD g_start_position[4][2];
	extern long g_start_map[4];
	extern DWORD g_create_position[4][2];
}

extern char g_nation_name[4][32];
extern DWORD g_create_position_canada[4][2];
extern DWORD arena_return_position[4][2];

inline const char* EMPIRE_NAME( BYTE e)
{
	return g_nation_name[e];
}

inline DWORD EMPIRE_START_MAP(BYTE e)
{
	return start_private::g_start_map[e];
}

inline DWORD EMPIRE_BASE_X(BYTE e)
{
	PIXEL_POSITION basePos;
	SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(EMPIRE_START_MAP(e), basePos);
	return basePos.x;
}

inline DWORD EMPIRE_BASE_Y(BYTE e)
{
	PIXEL_POSITION basePos;
	SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(EMPIRE_START_MAP(e), basePos);
	return basePos.y;
}

inline DWORD EMPIRE_START_X(BYTE e)
{
	if (1 <= e && e <= 3)
		return EMPIRE_BASE_X(e) + start_private::g_start_position[e][0] * 100;

	return 0;
}

inline DWORD EMPIRE_START_Y(BYTE e)
{
	if (1 <= e && e <= 3)
		return EMPIRE_BASE_Y(e) + start_private::g_start_position[e][1] * 100;

	return 0;
}

inline DWORD ARENA_RETURN_POINT_X(BYTE e)
{
	if (1 <= e && e <= 3)
		return arena_return_position[e][0];

	return 0;
}

inline DWORD ARENA_RETURN_POINT_Y(BYTE e)
{
	if (1 <= e && e <= 3)
		return arena_return_position[e][1];

	return 0;
}

inline DWORD CREATE_START_X(BYTE e)
{
	if (1 <= e && e <= 3)
		return EMPIRE_BASE_X(e) + start_private::g_create_position[e][0] * 100;

	return 0;
}

inline DWORD CREATE_START_Y(BYTE e)
{
	if (1 <= e && e <= 3)
		return EMPIRE_BASE_Y(e) + start_private::g_create_position[e][1] * 100;

	return 0;
}

#endif
