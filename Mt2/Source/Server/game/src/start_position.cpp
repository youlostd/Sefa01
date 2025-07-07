#include "stdafx.h"
#include "start_position.h"
#include "constants.h"

char g_nation_name[4][32] =
{
	"",
	"신수국",
	"천조국",
	"진노국",
};

long start_private::g_start_map[4] =
{
	0,	// reserved
	HOME_MAP_INDEX_RED_1,	// 신수국
	HOME_MAP_INDEX_YELLOW_1,	// 천조국
	HOME_MAP_INDEX_BLUE_1	// 진노국
};

DWORD start_private::g_start_position[4][2] =
{
	{ 		0,		0 },	// reserved
	{	  500,	  574 },	// 신수국
	{	  531,	  642 },	// 천조국
	{	  357,	  507 },
};


DWORD arena_return_position[4][2] =
{
	{	   0,  0	   },
	{   347600, 882700  }, // 자양현
	{   138600, 236600  }, // 복정현
	{   857200, 251800  }  // 박라현
};


DWORD start_private::g_create_position[4][2] =
{
	{	 0,		  0 },
	{  497,		579 },
	{  640,		500 },
	{  358,		505 },
};
