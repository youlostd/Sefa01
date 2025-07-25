﻿/*********************************************************************
 * date		: 2006.11.20
 * file		: cube.h
 * author	  : mhh
 * description : Å¥ºê½Ã½ºÅÛ
 */

#ifndef _cube_h_
#define _cube_h_

#include "protobuf_data_item.h"

#define CUBE_MAX_NUM	24	// OLD:INVENTORY_MAX_NUM
#define CUBE_MAX_DISTANCE	1000

struct CUBE_VALUE
{
	DWORD	vnum_start;
	DWORD	vnum_end;
	int		count;

	bool operator == (const CUBE_VALUE& b)
	{
		return (this->count == b.count) && (this->vnum_start == b.vnum_start) && (this->vnum_end == b.vnum_end);
	}
};

struct CUBE_DATA
{
	std::vector<WORD>		npc_vnum;
	std::vector<CUBE_VALUE>	item;
	std::vector<CUBE_VALUE>	reward;
	int						percent;
	unsigned int			gold;		// Á¦Á¶½Ã ÇÊ¿äÇÑ ±Ý¾×

	CUBE_DATA();

	bool		can_make_item (LPITEM *items, WORD npc_vnum);
	CUBE_VALUE*	reward_value ();
	bool		remove_material (LPCHARACTER ch, network::TItemAttribute* pResAttrs = NULL);
}; 


void Cube_init ();
bool Cube_load (const char *file);

bool Cube_make (LPCHARACTER ch);

void Cube_clean_item (LPCHARACTER ch);
void Cube_open (LPCHARACTER ch, const char* szTitle = "");
void Cube_close (LPCHARACTER ch);

void Cube_show_list (LPCHARACTER ch);
void Cube_add_item (LPCHARACTER ch, int cube_index, int inven_index);
void Cube_delete_item (LPCHARACTER ch, int cube_index);

void Cube_request_result_list(LPCHARACTER ch);
void Cube_request_material_info(LPCHARACTER ch, int request_start_index, int request_count = 1);

// test print code
void Cube_print();

bool Cube_InformationInitialize();

#endif	/* _cube_h_ */

