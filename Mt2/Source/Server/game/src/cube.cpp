
/*********************************************************************
 * date		: 2006.11.20
 * file		: cube.cpp
 * author	  : mhh
 * description : 
 */

#define _cube_cpp_

#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "log.h"
#include "char.h"
#include "dev_log.h"
#include "item.h"
#include "item_manager.h"

#include <sstream>

extern int test_server;


#define RETURN_IF_CUBE_IS_NOT_OPENED(ch) if (!(ch)->IsCubeOpen()) return


/*--------------------------------------------------------*/
/*				   GLOBAL VARIABLES					 */
/*--------------------------------------------------------*/
static std::vector<CUBE_DATA*>	s_cube_proto;
static bool s_isInitializedCubeMaterialInformation = false;

/*--------------------------------------------------------*/
/*			   Cube Material Information				*/ 
/*--------------------------------------------------------*/
enum ECubeResultCategory
{
	CUBE_CATEGORY_POTION,				// ¾àÃÊ, Áø¾× µîµî..  (Æ÷¼ÇÀ¸·Î Æ¯Á¤ÇÒ ¼ö ¾øÀ¸´Ï »ç¿ë ¾È ÇÔ. ¾àÃÊ°°Àº°Ç ´Ù °Á ±âÅ¸)
	CUBE_CATEGORY_WEAPON,				// ¹«±â
	CUBE_CATEGORY_ARMOR,				// ¹æ¾î±¸
	CUBE_CATEGORY_ACCESSORY,			// Àå½Å±¸
	CUBE_CATEGORY_ETC,				// ±âÅ¸ µîµî...
};

typedef std::vector<CUBE_VALUE>	TCubeValueVector;

struct SCubeMaterialInfo
{
	SCubeMaterialInfo()
	{
		bHaveComplicateMaterial = false;
	};

	CUBE_VALUE			reward;							// º¸»óÀÌ ¹¹³Ä
	TCubeValueVector	material;						// Àç·áµéÀº ¹¹³Ä
	DWORD				gold;							// µ·Àº ¾ó¸¶µå³Ä
	int					chance;
	TCubeValueVector	complicateMaterial;				// º¹ÀâÇÑ-_- Àç·áµé

	// .. Å¬¶óÀÌ¾ðÆ®¿¡¼­ Àç·á¸¦ º¸¿©ÁÖ±â À§ÇÏ¿© ¾à¼ÓÇÑ Æ÷¸Ë
	// 72723,1&72724,2&72730,1
	// 52001,1|52002,1|52003,1&72723,1&72724,5
	//	=> ( 52001,1 or 52002,1 or 52003,1 ) and 72723,1 and 72724,5
	std::string			infoText;		
	bool				bHaveComplicateMaterial;		//
};

struct SItemNameAndLevel
{
	SItemNameAndLevel() { level = 0; }

	std::string		name;
	int				level;
};


// ÀÚ·á±¸Á¶³ª ÀÌ·±°Å º´½ÅÀÎ°Ç ÀÌÇØÁ»... ´©±¸¶«¿¡ ¿µÈ¥ÀÌ ¾ø´Â »óÅÂ¿¡¼­ ¸¸µé¾ú¾¸
typedef std::vector<SCubeMaterialInfo>								TCubeResultList;
typedef TR1_NS::unordered_map<DWORD, TCubeResultList>				TCubeMapByNPC;				// °¢°¢ÀÇ NPCº°·Î ¾î¶² °É ¸¸µé ¼ö ÀÖ°í Àç·á°¡ ¹ºÁö...
typedef TR1_NS::unordered_map<DWORD, std::string>					TCubeResultInfoTextByNPC;	// °¢°¢ÀÇ NPCº°·Î ¸¸µé ¼ö ÀÖ´Â ¸ñ·ÏÀ» Á¤ÇØÁø Æ÷¸ËÀ¸·Î Á¤¸®ÇÑ Á¤º¸

TCubeMapByNPC cube_info_map;
TCubeResultInfoTextByNPC cube_result_info_map_by_npc;				// ³×ÀÌ¹Ö Á¸³ª º´½Å°°´Ù ¤»¤»¤»

class CCubeMaterialInfoHelper
{
public:
public:
};

/*--------------------------------------------------------*/
/*				  STATIC FUNCTIONS					  */ 
/*--------------------------------------------------------*/
 // ÇÊ¿äÇÑ ¾ÆÀÌÅÛ °³¼ö¸¦ °¡Áö°íÀÖ´Â°¡?
static bool FN_check_item_count (LPITEM *items, DWORD item_vnum_start, DWORD item_vnum_end, int need_count)
{
	int	count = 0;

	// for all cube
	for (int i=0; i<CUBE_MAX_NUM; ++i)
	{
		if (NULL==items[i])	continue;

		if (items[i]->GetVnum()>=item_vnum_start&&items[i]->GetVnum()<=item_vnum_end)
		{
			count += items[i]->GetCount();
		}
	}

	return (count>=need_count);
}

// Å¥ºê³»ÀÇ Àç·á¸¦ Áö¿î´Ù.
// returns true if a removed material was a gm item
static bool FN_remove_material (LPITEM *items, DWORD item_vnum_start, DWORD item_vnum_end, int need_count, network::TItemAttribute* pResAttrs = NULL)
{
	bool bAnyGMRemoved = false;

	int		count	= 0;
	LPITEM	item	= NULL;

	// for all cube
	for (int i=0; i<CUBE_MAX_NUM; ++i)
	{
		if (NULL==items[i])	continue;

		item = items[i];
		if (item->GetVnum()>=item_vnum_start&&item->GetVnum()<=item_vnum_end)
		{
			count += item->GetCount();
			if (item->IsGMOwner())
				bAnyGMRemoved = true;

			if (pResAttrs)
			{
				if (item->GetType() == ITEM_ARMOR || item->GetType() == ITEM_WEAPON)
				{
					for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
						pResAttrs[i] = item->GetAttribute(i);
				}
			}

			if (count>need_count)
			{
				item->SetCount(count-need_count);
				return bAnyGMRemoved;
			}
			else
			{
				item->SetCount(0);
				items[i] = NULL;
			}
		}
	}

	return bAnyGMRemoved;
}

void Cube_Make_Item_List(LPCHARACTER ch, LPITEM* pkItemList)
{
	DWORD* adwItemIDs = ch->GetCubeItemID();
	for (int i = 0; i < CUBE_MAX_NUM; ++i)
	{
		if (adwItemIDs[i] == 0)
			pkItemList[i] = NULL;
		else
		{
			pkItemList[i] = ch->FindItemByID(adwItemIDs[i]);
			if (test_server)
				ch->ChatPacket(CHAT_TYPE_INFO, "adwItemIDs[%d] = %u (%p)",
					i, adwItemIDs[i], pkItemList[i]);
		}
	}
}

static CUBE_DATA* FN_find_cube (LPITEM *items, WORD npc_vnum)
{
	DWORD	i, end_index;

	if (0==npc_vnum)	return NULL;

	// FOR ALL CUBE_PROTO
	end_index = s_cube_proto.size();
	for (i=0; i<end_index; ++i)
	{
		if ( s_cube_proto[i]->can_make_item(items, npc_vnum) )
			return s_cube_proto[i];
	}

	return NULL;
}

static bool FN_check_valid_npc( WORD vnum )
{
	for ( std::vector<CUBE_DATA*>::iterator iter = s_cube_proto.begin(); iter != s_cube_proto.end(); iter++ )
	{
		if ( std::find((*iter)->npc_vnum.begin(), (*iter)->npc_vnum.end(), vnum) != (*iter)->npc_vnum.end() )
			return true;
	}

	return false;
}

// Å¥ºêµ¥ÀÌÅ¸°¡ ¿Ã¹Ù¸£°Ô ÃÊ±âÈ­ µÇ¾ú´ÂÁö Ã¼Å©ÇÑ´Ù.
static bool FN_check_cube_data (CUBE_DATA *cube_data)
{
	DWORD	i = 0;
	DWORD	end_index = 0;

	end_index = cube_data->npc_vnum.size();
	for (i=0; i<end_index; ++i)
	{
		if ( cube_data->npc_vnum[i] == 0 )	return false;
	}

	end_index = cube_data->item.size();
	for (i=0; i<end_index; ++i)
	{
		if ( cube_data->item[i].vnum_start == 0 || cube_data->item[i].vnum_end == 0 )		return false;
		if ( cube_data->item[i].count == 0 )	return false;
	}

	end_index = cube_data->reward.size();
	for (i=0; i<end_index; ++i)
	{
		if (cube_data->item[i].vnum_start == 0 || cube_data->item[i].vnum_end == 0)		return false;
		if ( cube_data->reward[i].count == 0 )	return false;
	}
	return true;
}

CUBE_DATA::CUBE_DATA()
{
	this->percent = 0;
	this->gold = 0;
}

// ÇÊ¿äÇÑ Àç·áÀÇ ¼ö·®À» ¸¸Á·ÇÏ´ÂÁö Ã¼Å©ÇÑ´Ù.
bool CUBE_DATA::can_make_item (LPITEM *items, WORD npc_vnum)
{
	// ÇÊ¿äÇÑ Àç·á, ¼ö·®À» ¸¸Á·ÇÏ´ÂÁö Ã¼Å©ÇÑ´Ù.
	DWORD	i, end_index;
	DWORD	need_vnum_start, need_vnum_end;
	int		need_count;
	int		found_npc = false;

	// check npc_vnum
	end_index = this->npc_vnum.size();
	for (i=0; i<end_index; ++i)
	{
		if (npc_vnum == this->npc_vnum[i])
			found_npc = true;
	}
	if (false==found_npc)	return false;

	end_index = this->item.size();
	for (i=0; i<end_index; ++i)
	{
		need_vnum_start	= this->item[i].vnum_start;
		need_vnum_end	= this->item[i].vnum_end;
		need_count	= this->item[i].count;

		if ( false==FN_check_item_count(items, need_vnum_start, need_vnum_end, need_count) )
			return false;
	}

	return true;
}

// Å¥ºê¸¦ µ¹·ÈÀ»¶§ ³ª¿À´Â ¾ÆÀÌÅÛÀÇ Á¾·ù¸¦ °áÁ¤ÇÔ
CUBE_VALUE* CUBE_DATA::reward_value ()
{
	int		end_index		= 0;
	DWORD	reward_index	= 0;

	end_index = this->reward.size();
	reward_index = random_number(0, end_index);
	reward_index = random_number(0, end_index-1);

	return &this->reward[reward_index];
}

// Å¥ºê¿¡ µé¾îÀÖ´Â Àç·á¸¦ Áö¿î´Ù
// returns true if any material that was removed was a gm item
bool CUBE_DATA::remove_material(LPCHARACTER ch, network::TItemAttribute* pResAttrs)
{
	bool	bAnyGMRemoved = false;
	DWORD	i, end_index;
	DWORD	need_vnum_start, need_vnum_end;
	int		need_count;
	LPITEM	items[CUBE_MAX_NUM];
	Cube_Make_Item_List(ch, items);

	end_index = this->item.size();
	for (i=0; i<end_index; ++i)
	{
		need_vnum_start	= this->item[i].vnum_start;
		need_vnum_end	= this->item[i].vnum_end;
		need_count	= this->item[i].count;

		if (FN_remove_material(items, need_vnum_start, need_vnum_end, need_count, pResAttrs))
			bAnyGMRemoved = true;
	}

	return bAnyGMRemoved;
}

void Cube_clean_item (LPCHARACTER ch)
{
	DWORD	*cube_item;

	cube_item = ch->GetCubeItemID();

	for (int i=0; i<CUBE_MAX_NUM; ++i)
		cube_item[i] = 0;
}

// Å¥ºêÃ¢ ¿­±â
void Cube_open(LPCHARACTER ch, const char* szTitle)
{
	if (false == s_isInitializedCubeMaterialInformation)
	{
		Cube_InformationInitialize();
	}

	if (NULL == ch)
		return;

	LPCHARACTER	npc;
	npc = ch->GetQuestNPC();
	if (NULL==npc)
	{
		if (test_server)
			sys_err("cube_npc is NULL");
		return;
	}

	if ( FN_check_valid_npc(npc->GetRaceNum()) == false )
	{
		if ( test_server == true )
		{
			sys_err("cube not valid NPC");
		}
		return;
	}

	if (ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ¹Ì Á¦Á¶Ã¢ÀÌ ¿­·ÁÀÖ½À´Ï´Ù."));
		return;
	}
	if ( ch->GetExchange() || ch->GetMyShop() || ch->GetShop() || ch->IsOpenSafebox() || ch->IsCubeOpen() )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	long distance = DISTANCE_APPROX(ch->GetX() - npc->GetX(), ch->GetY() - npc->GetY());

	if (distance >= CUBE_MAX_DISTANCE)
	{
		if (test_server)
			sys_log(0, "CUBE: TOO_FAR: %s distance %d", ch->GetName(), distance);
		return;
	}


	Cube_clean_item(ch);
	ch->SetCubeNpc(npc);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "cube open %d %s", npc->GetRaceNum(), szTitle);
}

// Å¥ºê Äµ½½
void Cube_close (LPCHARACTER ch)
{
	RETURN_IF_CUBE_IS_NOT_OPENED(ch);
	Cube_clean_item(ch);
	ch->SetCubeNpc(NULL);
	ch->ChatPacket(CHAT_TYPE_COMMAND, "cube close");
	dev_log(LOG_DEB0, "<CUBE> close (%s)", ch->GetName());
}

void Cube_init()
{
	CUBE_DATA * p_cube = NULL;
	std::vector<CUBE_DATA*>::iterator iter;

	char file_name[256+1];
	snprintf(file_name, sizeof(file_name), "%s/cube.txt", Locale_GetBasePath().c_str());

	sys_log(0, "Cube_Init %s", file_name);

	for (iter = s_cube_proto.begin(); iter!=s_cube_proto.end(); iter++)
	{
		p_cube = *iter;
		M2_DELETE(p_cube);
	}

	s_cube_proto.clear();

	if (false == Cube_load(file_name))
		sys_err("Cube_Init failed");
}

bool Cube_load (const char *file)
{
	FILE	*fp;
	char	one_line[256];
	int		value1_1, value1_2, value2, value3, value4;
	const char	*delim = " \t\r\n";
	char	*v, *token_string;
	CUBE_DATA	*cube_data = NULL;
	CUBE_VALUE	cube_value = {0,0};

	if (0 == file || 0 == file[0])
		return false;

	if ((fp = fopen(file, "r")) == 0)
		return false;

	while (fgets(one_line, 256, fp))
	{
		value1_1 = value1_2 = value2 = value3 = value4 = 0;

		if (one_line[0] == '#')
			continue;

		token_string = strtok(one_line, delim);

		if (NULL == token_string)
			continue;

		// set value1, value2
		if ((v = strtok(NULL, delim)))
		{
			str_to_number(value1_1, v);
			value1_2 = value1_1;

			char* v2 = strchr(v, '~');
			if (v2 != NULL)
				str_to_number(value1_2, v2 + 1);
		}

		if ((v = strtok(NULL, delim)))
			str_to_number(value2, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value3, v);

		if ((v = strtok(NULL, delim)))
			str_to_number(value4, v);

		TOKEN("section")
		{
			cube_data = M2_NEW CUBE_DATA;
		}
		else TOKEN("npc")
		{
			cube_data->npc_vnum.push_back((WORD)value1_1);
		}
		else TOKEN("item")
		{
			cube_value.vnum_start	= value1_1;
			cube_value.vnum_end		= value1_2;
			cube_value.count		= value2;

			cube_data->item.push_back(cube_value);
		}
		else TOKEN("reward")
		{
			cube_value.vnum_start	= value1_1;
			cube_value.vnum_end		= value1_1;
			cube_value.count		= value2;

			cube_data->reward.push_back(cube_value);
		}
		else TOKEN("percent")
		{
			cube_data->percent = value1_1;
		}
		else TOKEN("gold")
		{
			// Á¦Á¶¿¡ ÇÊ¿äÇÑ ±Ý¾×
			cube_data->gold = value1_1;
		}
		else TOKEN("end")
		{
			// TODO : check cube data
			if (false == FN_check_cube_data(cube_data))
			{
				sys_err("something wrong cube (npc %u)", cube_data->npc_vnum.front());
				M2_DELETE(cube_data);
				continue;
			}
			s_cube_proto.push_back(cube_data);

			sys_log(!test_server, "CUBE_LOAD: npc %u reward %ux %u", cube_data->npc_vnum.front(), cube_data->reward.front().count, cube_data->reward.front().vnum_start);
		}
	}

	fclose(fp);
	return true;
}

static void FN_cube_print (CUBE_DATA *data, DWORD index)
{
	DWORD	i;
	dev_log(LOG_DEB0, "--------------------------------");
	dev_log(LOG_DEB0, "CUBE_DATA[%d]", index);

	for (i=0; i<data->npc_vnum.size(); ++i)
	{
		dev_log(LOG_DEB0, "\tNPC_VNUM[%d] = %d", i, data->npc_vnum[i]);
	}
	for (i=0; i<data->item.size(); ++i)
	{
		dev_log(LOG_DEB0, "\tITEM[%d]   = (%d~%d, %d)", i, data->item[i].vnum_start, data->item[i].vnum_end, data->item[i].count);
	}
	for (i=0; i<data->reward.size(); ++i)
	{
		dev_log(LOG_DEB0, "\tREWARD[%d] = (%d, %d)", i, data->reward[i].vnum_start, data->reward[i].count);
	}
	dev_log(LOG_DEB0, "\tPERCENT = %d", data->percent);
	dev_log(LOG_DEB0, "--------------------------------");
}

void Cube_print ()
{
	for (DWORD i=0; i<s_cube_proto.size(); ++i)
	{
		FN_cube_print(s_cube_proto[i], i);
	}
}

static bool FN_update_cube_status(LPCHARACTER ch)
{
	if (NULL == ch)
		return false;

	if (!ch->IsCubeOpen())
		return false;

	LPCHARACTER	npc = ch->GetQuestNPC();
	if (NULL == npc)
		return false;
	
	LPITEM pkItemList[CUBE_MAX_NUM];
	Cube_Make_Item_List(ch, pkItemList);

	CUBE_DATA* cube = FN_find_cube(pkItemList, npc->GetRaceNum());

	if (NULL == cube)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "cube info 0 0 0");
		return false;
	}

	ch->ChatPacket(CHAT_TYPE_COMMAND, "cube info %d %d %d", cube->gold, 0, 0);
	return true;
}

// return new item
bool Cube_make (LPCHARACTER ch)
{
	// ÁÖ¾îÁø ¾ÆÀÌÅÛÀ» ÇÊ¿ä·ÎÇÏ´Â Á¶ÇÕÀ» Ã£´Â´Ù. (Å¥ºêµ¥ÀÌÅ¸·Î ÄªÇÔ)
	// Å¥ºê µ¥ÀÌÅ¸°¡ ÀÖ´Ù¸é ¾ÆÀÌÅÛÀÇ Àç·á¸¦ Ã¼Å©ÇÑ´Ù.
	// »õ·Î¿î ¾ÆÀÌÅÛÀ» ¸¸µç´Ù.
	// »õ·Î¿î ¾ÆÀÌÅÛ Áö±Þ

	LPCHARACTER	npc;
	int			percent_number = 0;
	CUBE_DATA	*cube_proto;
	LPITEM	items[CUBE_MAX_NUM];
	LPITEM	new_item;

	Cube_Make_Item_List(ch, items);

	if (!(ch)->IsCubeOpen())
	{
		(ch)->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¦Á¶Ã¢ÀÌ ¿­·ÁÀÖÁö ¾Ê½À´Ï´Ù"));
		return false;
	}

	npc = ch->GetQuestNPC();
	if (NULL == npc)
	{
		return false;
	}

	cube_proto = FN_find_cube(items, npc->GetRaceNum());

	if (NULL == cube_proto)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¦Á¶ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù"));
		return false;
	}

	if (ch->GetGold() < cube_proto->gold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "µ·ÀÌ ºÎÁ·ÇÏ°Å³ª ¾ÆÀÌÅÛÀÌ Á¦ÀÚ¸®¿¡ ¾ø½À´Ï´Ù."));	// ÀÌ ÅØ½ºÆ®´Â ÀÌ¹Ì ³Î¸® ¾²ÀÌ´Â°Å¶ó Ãß°¡¹ø¿ª ÇÊ¿ä ¾øÀ½
		return false;
	}

	CUBE_VALUE	*reward_value = cube_proto->reward_value();

	network::TItemAttribute kAttrs[ITEM_ATTRIBUTE_MAX_NUM];
	memset(&kAttrs, 0, sizeof(kAttrs));
	bool bCopyAttr = false;

	if (npc->GetRaceNum() == 20091)
		bCopyAttr = true;

	bool bAnyGMRemoved;
	if (bCopyAttr)
		bAnyGMRemoved = cube_proto->remove_material(ch, kAttrs);
	else
		bAnyGMRemoved = cube_proto->remove_material(ch);
	
	if (0 < cube_proto->gold)
		ch->PointChange(POINT_GOLD, -(long long)(cube_proto->gold), false);

	int iExtraPercent = 0;

	percent_number = random_number(1,100);
	if ( percent_number<=cube_proto->percent+iExtraPercent)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "cube success %d %d", reward_value->vnum_start, reward_value->count);

		new_item = ITEM_MANAGER::instance().CreateItem(reward_value->vnum_start, reward_value->count);
		if (!new_item)
		{
			sys_err("cannot create item %u count %u for player %u %s", reward_value->vnum_start, reward_value->count, ch->GetPlayerID(), ch->GetName());
			return true;
		}

		if (bAnyGMRemoved)
		{
			new_item->SetGMOwner(true);
			ch->ChatPacket(CHAT_TYPE_INFO, "Your Item is not tradeable, because something of your material items was not tradeable.");
		}

		if (bCopyAttr)
		{
			for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
				new_item->SetForceAttribute(i, kAttrs[i].type(), kAttrs[i].value());
		}

		LogManager::instance().CubeLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(),
			reward_value->vnum_start, new_item->GetID(), reward_value->count, 1);

		ch->AutoGiveItem(new_item);
		return true;
	}
	else
	{
		// ½ÇÆÐ
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Á¦Á¶¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));	// 2012.11.12 »õ·Î Ãß°¡µÈ ¸Þ¼¼Áö (locale_string.txt ¿¡ Ãß°¡ÇØ¾ß ÇÔ)
		ch->ChatPacket(CHAT_TYPE_COMMAND, "cube fail");
		LogManager::instance().CubeLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(),
				reward_value->vnum_start, 0, 0, 0);
		return false;
	}

	return false;
}


// Å¥ºê¿¡ ÀÖ´Â ¾ÆÀÌÅÛµéÀ» Ç¥½Ã
void Cube_show_list (LPCHARACTER ch)
{
	LPITEM	cube_item[CUBE_MAX_NUM];
	LPITEM	item;

	RETURN_IF_CUBE_IS_NOT_OPENED(ch);

	Cube_Make_Item_List(ch, cube_item);

	for (int i=0; i<CUBE_MAX_NUM; ++i)
	{
		item = cube_item[i];
		if (NULL==item)	continue;

		ch->ChatPacket(CHAT_TYPE_INFO, "cube[%d]: inventory[%d]: %s",
				i, item->GetCell(), item->GetName(ch->GetLanguageID()));
	}
}


// ÀÎº¥Åä¸®¿¡ ÀÖ´Â ¾ÆÀÌÅÛÀ» Å¥ºê¿¡ µî·Ï
void Cube_add_item (LPCHARACTER ch, int cube_index, int inven_index)
{
	// ¾ÆÀÌÅÛÀÌ ÀÖ´Â°¡?
	// Å¥ºê³»ÀÇ ºóÀÚ¸® Ã£±â
	// Å¥ºê¼¼ÆÃ
	// ¸Þ½ÃÁö Àü¼Û
	LPITEM	item;
	DWORD	*cube_item_id;

	RETURN_IF_CUBE_IS_NOT_OPENED(ch);

	BYTE bSrcWnd = ITEM_MANAGER::instance().GetWindowBySlot(inven_index);

	if (inven_index<0 || (ch->GetInventoryMaxNum() <= inven_index && !ITEM_MANAGER::instance().IsNewWindow(bSrcWnd)))
		return;
	if (bSrcWnd == UPPITEM_INVENTORY && ch->GetUppitemInventoryMaxNum() + UPPITEM_INV_SLOT_START <= inven_index)
		return;
	if (cube_index<0 || CUBE_MAX_NUM<=cube_index)
		return;

	item = ch->GetInventoryItem(inven_index);

	if (NULL==item)	return;

	cube_item_id = ch->GetCubeItemID();

	// ÀÌ¹Ì ´Ù¸¥À§Ä¡¿¡ µî·ÏµÇ¾ú´ø ¾ÆÀÌÅÛÀÌ¸é ±âÁ¸ indext»èÁ¦
	for (int i=0; i<CUBE_MAX_NUM; ++i)
	{
		if (item->GetID() == cube_item_id[i])
		{
			cube_item_id[i] = 0;
			break;
		}
	}

	cube_item_id[cube_index] = item->GetID();

	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "cube[%d]: inventory[%d]: %s added [id %u]",
									cube_index, inven_index, item->GetName(), cube_item_id[cube_index]);

	// ÇöÀç »óÀÚ¿¡ ¿Ã¶ó¿Â ¾ÆÀÌÅÛµé·Î ¹«¾ùÀ» ¸¸µé ¼ö ÀÖ´ÂÁö Å¬¶óÀÌ¾ðÆ®¿¡ Á¤º¸ Àü´Þ
	// À» ÇÏ°í½Í¾úÀ¸³ª ±×³É ÇÊ¿äÇÑ °ñµå°¡ ¾ó¸¶ÀÎÁö Àü´Þ
	FN_update_cube_status(ch);

	return;
}

// Å¥ºê¿¡ÀÖ´Â ¾ÆÀÌÅÛÀ» Á¦°Å
void Cube_delete_item (LPCHARACTER ch, int cube_index)
{
	LPITEM	item;
	DWORD	*cube_item_id;

	RETURN_IF_CUBE_IS_NOT_OPENED(ch);

	if (cube_index<0 || CUBE_MAX_NUM<=cube_index)	return;

	cube_item_id = ch->GetCubeItemID();

	if (0 == cube_item_id[cube_index])	return;

	item = ch->FindItemByID(cube_item_id[cube_index]);
	cube_item_id[cube_index] = 0;

	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "cube[%d]: cube[%d]: %s deleted",
				cube_index, item ? item->GetCell() : -1, item ? item->GetName() : "<none>");

	// ÇöÀç »óÀÚ¿¡ ¿Ã¶ó¿Â ¾ÆÀÌÅÛµé·Î ¹«¾ùÀ» ¸¸µé ¼ö ÀÖ´ÂÁö Å¬¶óÀÌ¾ðÆ®¿¡ Á¤º¸ Àü´Þ
	// À» ÇÏ°í½Í¾úÀ¸³ª ±×³É ÇÊ¿äÇÑ °ñµå°¡ ¾ó¸¶ÀÎÁö Àü´Þ
	FN_update_cube_status(ch);

	return;
}

// ¾ÆÀÌÅÛ ÀÌ¸§À» ÅëÇØ¼­ ¼ø¼ö ÀÌ¸§°ú °­È­·¹º§À» ºÐ¸®ÇÏ´Â ÇÔ¼ö (¹«½Ö°Ë+5 -> ¹«½Ö°Ë, 5)
SItemNameAndLevel SplitItemNameAndLevelFromName(const std::string& name)
{
	int level = 0;
	SItemNameAndLevel info;
	info.name = name;

	size_t pos = name.find("+");
	
	if (std::string::npos != pos)
	{
		const std::string levelStr = name.substr(pos + 1, name.size() - pos - 1);
		str_to_number(level, levelStr.c_str());

		info.name = name.substr(0, pos);
	}

	info.level = level;

	return info;
};

bool FIsLessCubeValue(const CUBE_VALUE& a, const CUBE_VALUE& b)
{
	return a.vnum_start < b.vnum_start;
}

void Cube_MakeCubeInformationText()
{
	// ÀÌÁ¦ Á¤¸®µÈ Å¥ºê °á°ú ¹× Àç·áµéÀÇ Á¤º¸·Î Å¬¶óÀÌ¾ðÆ®¿¡ º¸³» ÁÙ Á¤º¸·Î º¯È¯ÇÔ.
	for (TCubeMapByNPC::iterator iter = cube_info_map.begin(); cube_info_map.end() != iter; ++iter)
	{
		const DWORD& npcVNUM = iter->first;
		TCubeResultList& resultList = iter->second;

		for (TCubeResultList::iterator resultIter = resultList.begin(); resultList.end() != resultIter; ++resultIter)
		{
			SCubeMaterialInfo& materialInfo = *resultIter;
			std::string& infoText = materialInfo.infoText;

			
			// ÀÌ³ðÀÌ ³ª»Û³ðÀÌ¾ß
			if (0 < materialInfo.complicateMaterial.size())
			{
				std::sort(materialInfo.complicateMaterial.begin(), materialInfo.complicateMaterial.end(), FIsLessCubeValue);
				std::sort(materialInfo.material.begin(), materialInfo.material.end(), FIsLessCubeValue);

				//// Áßº¹µÇ´Â Àç·áµéÀ» Áö¿ò
				for (TCubeValueVector::iterator iter = materialInfo.complicateMaterial.begin(); materialInfo.complicateMaterial.end() != iter; ++iter)
				{
					for (TCubeValueVector::iterator targetIter = materialInfo.material.begin(); materialInfo.material.end() != targetIter; ++targetIter)
					{
						if (*targetIter == *iter)
						{
							targetIter = materialInfo.material.erase(targetIter);
						}
					}
				}

				// 72723,1 or 72725,1 or ... ÀÌ·± ½ÄÀÇ ¾à¼ÓµÈ Æ÷¸ËÀ» ÁöÅ°´Â ÅØ½ºÆ®¸¦ »ý¼º
				for (TCubeValueVector::iterator iter = materialInfo.complicateMaterial.begin(); materialInfo.complicateMaterial.end() != iter; ++iter)
				{
					char tempBuffer[128];
					sprintf(tempBuffer, "%d,%d,%d|", iter->vnum_start, iter->vnum_end, iter->count);
					
					infoText += std::string(tempBuffer);
				}

				infoText.erase(infoText.size() - 1);

				if (0 < materialInfo.material.size())
					infoText.push_back('&');
			}

			// Áßº¹µÇÁö ¾Ê´Â ÀÏ¹Ý Àç·áµéµµ Æ÷¸Ë »ý¼º
			for (TCubeValueVector::iterator iter = materialInfo.material.begin(); materialInfo.material.end() != iter; ++iter)
			{
				char tempBuffer[128];
				sprintf(tempBuffer, "%d,%d,%d&", iter->vnum_start, iter->vnum_end, iter->count);
				infoText += std::string(tempBuffer);
			}

			infoText.erase(infoText.size() - 1);

			// ¸¸µé ¶§ °ñµå°¡ ÇÊ¿äÇÏ´Ù¸é °ñµåÁ¤º¸ Ãß°¡
			if (0 < materialInfo.gold)
			{
				char temp[128];
				sprintf(temp, "%d", materialInfo.gold);
				infoText += std::string("/") + temp;
			}

			//sys_err("\t\tNPC: %d, Reward: %d(%s)\n\t\t\tInfo: %s", npcVNUM, materialInfo.reward.vnum, ITEM_MANAGER::Instance().GetTable(materialInfo.reward.vnum)->szName, materialInfo.infoText.c_str());
		} // for resultList
	} // for npc
}

bool Cube_InformationInitialize()
{
	for (int i = 0; i < s_cube_proto.size(); ++i)
	{
		CUBE_DATA* cubeData = s_cube_proto[i];

		if (test_server)
			sys_log(0, "Cube Init i:%i id: %d", i, cubeData->npc_vnum.at(0));
		const std::vector<CUBE_VALUE>& rewards = cubeData->reward;

		// ÇÏµåÄÚµù ¤¸¤µ
		if (1 != rewards.size())
		{
			sys_err("[CubeInfo] WARNING! Does not support multiple rewards (count: %d)", rewards.size());			
			continue;
		}
		//if (1 != cubeData->npc_vnum.size())
		//{
		//	sys_err("[CubeInfo] WARNING! Does not support multiple NPC (count: %d)", cubeData->npc_vnum.size());			
		//	continue;
		//}

		const CUBE_VALUE& reward = rewards.at(0);
		const WORD& npcVNUM = cubeData->npc_vnum.at(0);
		bool bComplicate = false;
		
		TCubeMapByNPC& cubeMap = cube_info_map;
		TCubeResultList& resultList = cubeMap[npcVNUM];
		SCubeMaterialInfo materialInfo;

		materialInfo.reward = reward;
		materialInfo.gold = cubeData->gold;
		materialInfo.material = cubeData->item;
		materialInfo.chance = cubeData->percent;

		for (TCubeResultList::iterator iter = resultList.begin(); resultList.end() != iter; ++iter)
		{
			SCubeMaterialInfo& existInfo = *iter;

			// ÀÌ¹Ì Áßº¹µÇ´Â º¸»óÀÌ µî·ÏµÇ¾î ÀÖ´Ù¸é ¾Æ¿¹ ´Ù¸¥ Á¶ÇÕÀ¸·Î ¸¸µå´Â °ÍÀÎÁö, 
			// °ÅÀÇ °°Àº Á¶ÇÕÀÎµ¥ Æ¯Á¤ ºÎºÐ¸¸ Æ²¸° °ÍÀÎÁö ±¸ºÐÇÔ.
			// ¿¹¸¦µé¸é Æ¯Á¤ ºÎºÐ¸¸ Æ²¸° ¾ÆÀÌÅÛµéÀº ¾Æ·¡Ã³·³ ÇÏ³ª·Î ¹­¾î¼­ ÇÏ³ªÀÇ °á°ú·Î º¸¿©ÁÖ±â À§ÇÔÀÓ:
			// ¿ë½ÅÁö°Ë:
			//		¹«½Ö°Ë+5 ~ +9 x 1
			//		ºÓÀº Ä®ÀÚ·ç Á¶°¢ x1
			//		³ì»ö °ËÀå½Ä Á¶°¢ x1
			if (reward.vnum_start == existInfo.reward.vnum_start)
			{
				for (TCubeValueVector::iterator existMaterialIter = existInfo.material.begin(); existInfo.material.end() != existMaterialIter; ++existMaterialIter)
				{
					auto existMaterialProto = ITEM_MANAGER::Instance().GetTable(existMaterialIter->vnum_start);
					if (NULL == existMaterialProto)
					{
						sys_err("There is no item(%u)", existMaterialIter->vnum_start);
						return false;
					}
					SItemNameAndLevel existItemInfo = SplitItemNameAndLevelFromName(existMaterialProto->name());

					if (0 < existItemInfo.level)
					{
						// Áö±Ý Ãß°¡ÇÏ´Â Å¥ºê °á°ú¹°ÀÇ Àç·á¿Í, ±âÁ¸¿¡ µî·ÏµÇ¾îÀÖ´ø Å¥ºê °á°ú¹°ÀÇ Àç·á Áß 
						// Áßº¹µÇ´Â ºÎºÐÀÌ ÀÖ´ÂÁö °Ë»öÇÑ´Ù
						for (TCubeValueVector::iterator currentMaterialIter = materialInfo.material.begin(); materialInfo.material.end() != currentMaterialIter; ++currentMaterialIter)
						{
							auto currentMaterialProto = ITEM_MANAGER::Instance().GetTable(currentMaterialIter->vnum_start);
							SItemNameAndLevel currentItemInfo = SplitItemNameAndLevelFromName(currentMaterialProto->name());

							if (currentItemInfo.name == existItemInfo.name)
							{
								bComplicate = true;
								existInfo.complicateMaterial.push_back(*currentMaterialIter);

								if (std::find(existInfo.complicateMaterial.begin(), existInfo.complicateMaterial.end(), *existMaterialIter) == existInfo.complicateMaterial.end())
									existInfo.complicateMaterial.push_back(*existMaterialIter);

								//currentMaterialIter = materialInfo.material.erase(currentMaterialIter);

								// TODO: Áßº¹µÇ´Â ¾ÆÀÌÅÛ µÎ °³ ÀÌ»ó °ËÃâÇØ¾ß µÉ ¼öµµ ÀÖÀ½
								break;
							}
						} // for currentMaterialIter
					}	// if level
				}	// for existMaterialInfo
			}	// if (reward.vnum == existInfo.reward.vnum)

		}	// for resultList

		if (false == bComplicate)
			resultList.push_back(materialInfo);
	}

	Cube_MakeCubeInformationText();

	s_isInitializedCubeMaterialInformation = true;
	return true;
}

// Å¬¶óÀÌ¾ðÆ®¿¡¼­ ¼­¹ö·Î : ÇöÀç NPC°¡ ¸¸µé ¼ö ÀÖ´Â ¾ÆÀÌÅÛµéÀÇ Á¤º¸(¸ñ·Ï)¸¦ ¿äÃ»
void Cube_request_result_list(LPCHARACTER ch)
{
	RETURN_IF_CUBE_IS_NOT_OPENED(ch);

	LPCHARACTER	npc = ch->GetQuestNPC();
	if (NULL == npc)
		return;

	DWORD npcVNUM = npc->GetRaceNum();
	size_t resultCount = 0;

	//std::string& resultText = cube_result_info_map_by_npc[npcVNUM];
	std::string resultText = "";

	// ÇØ´ç NPC°¡ ¸¸µé ¼ö ÀÖ´Â ¸ñ·ÏÀÌ Á¤¸®µÈ °Ô ¾ø´Ù¸é Ä³½Ã¸¦ »ý¼º
	//if (resultText.length() == 0)

	{
	//	resultText.clear();

		const TCubeResultList& resultList = cube_info_map[npcVNUM];
		resultCount = resultList.size();
		if (resultCount < 1)
			sys_err("[CubeInfo] Result list empty. Invalid npcVNUM: %d", npcVNUM);
	
		for (TCubeResultList::const_iterator iter = resultList.begin(); resultList.end() != iter; ++iter)
		{
			const SCubeMaterialInfo& materialInfo = *iter;

			char temp[128];
			sprintf(temp, "%d,%d,%d", materialInfo.reward.vnum_start, materialInfo.reward.count, materialInfo.chance);

			resultText += std::string(temp) + "/";
		}

		if (resultCount == 0)
			return;

		resultText.erase(resultText.size() - 1);

		// Ã¤ÆÃ ÆÐÅ¶ÀÇ ÇÑ°è¸¦ ³Ñ¾î°¡¸é ¿¡·¯ ³²±è... ±âÈ¹ÀÚ ºÐµé ²² Á¶Á¤ÇØ´Þ¶ó°í ¿äÃ»ÇÏ°Å³ª, ³ªÁß¿¡ ´Ù¸¥ ¹æ½ÄÀ¸·Î ¹Ù²Ù°Å³ª...
		if (resultText.size() >= CHAT_MAX_LEN - 20)
		{
			sys_err("[CubeInfo] Too long cube result list text. (NPC: %d, length: %d)", npcVNUM, resultText.size());
			resultText.clear();
			resultCount = 0;
		}

	}

	// ÇöÀç NPC°¡ ¸¸µé ¼ö ÀÖ´Â ¾ÆÀÌÅÛµéÀÇ ¸ñ·ÏÀ» ¾Æ·¡ Æ÷¸ËÀ¸·Î Àü¼ÛÇÑ´Ù.
	// (Server -> Client) /cube r_list npcVNUM resultCount vnum1,count1/vnum2,count2,/vnum3,count3/...
	// (Server -> Client) /cube r_list 20383 4 123,1/125,1/128,1/130,5
	
	ch->ChatPacket(CHAT_TYPE_COMMAND, "cube r_list %d %d %s", npcVNUM, resultCount, resultText.c_str());
}

// 
void Cube_request_material_info(LPCHARACTER ch, int requestStartIndex, int requestCount)
{
	RETURN_IF_CUBE_IS_NOT_OPENED(ch);

	LPCHARACTER	npc = ch->GetQuestNPC();
	if (NULL == npc)
		return;

	DWORD npcVNUM = npc->GetRaceNum();
	std::string materialInfoText = "";

	int index = 0;
	bool bCatchInfo = false;

	const TCubeResultList& resultList = cube_info_map[npcVNUM];
	for (TCubeResultList::const_iterator iter = resultList.begin(); resultList.end() != iter; ++iter)
	{
		const SCubeMaterialInfo& materialInfo = *iter;

		if (index++ == requestStartIndex)
		{
			bCatchInfo = true;
		}
		
		if (bCatchInfo)
		{
			materialInfoText += materialInfo.infoText + "@";
		}

		if (index >= requestStartIndex + requestCount)
			break;
	}

	if (!bCatchInfo || materialInfoText.size() == 0)
	{
		sys_err("[CubeInfo] Can't find matched material info (NPC: %d, index: %d, request count: %d)", npcVNUM, requestStartIndex, requestCount);
		return;
	}

	materialInfoText.erase(materialInfoText.size() - 1);

	// 
	// (Server -> Client) /cube m_info start_index count 125,1|126,2|127,2|123,5&555,5&555,4/120000

	DWORD dwSentCount = MAX(0, (materialInfoText.size() - 1)) / (CHAT_MAX_LEN - 30) + 1;
	/*if (materialInfoText.size() >= CHAT_MAX_LEN - 30)
	{
		sys_err("[CubeInfo] Too long material info. (NPC: %d, requestStart: %d, requestCount: %d, length: %d)", npcVNUM, requestStartIndex, requestCount, materialInfoText.size());
		ch->ChatPacket(CHAT_TYPE_COMMAND, "cube m_info %d %d %s", requestStartIndex, requestCount, materialInfoText.c_str());
	}
	*/

	char szInfoTxt[4096];
	snprintf(szInfoTxt, sizeof(szInfoTxt), "%s", materialInfoText.c_str());

	for (int i = 0; i < dwSentCount; ++i)
	{
		if (materialInfoText.length() < i * (CHAT_MAX_LEN - 30))
		{
			sys_err("!!! FATAL ERROR: cube [%u] dwSentCount[%d] text[%s]", npcVNUM, dwSentCount, materialInfoText.c_str());
			break;
		}

		int iEndTxt = MIN((i + 1) * (CHAT_MAX_LEN - 30), materialInfoText.length());
		char tmp = szInfoTxt[iEndTxt];
		szInfoTxt[iEndTxt] = '\0';

		ch->ChatPacket(CHAT_TYPE_COMMAND, "cube m_info %d %d %d %d %s", i, dwSentCount, requestStartIndex, requestCount, szInfoTxt + (i * (CHAT_MAX_LEN - 30)));

		szInfoTxt[iEndTxt] = tmp;
	}
}
