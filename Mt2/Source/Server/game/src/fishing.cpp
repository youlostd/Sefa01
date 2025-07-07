//#define __FISHING_MAIN__
#include "stdafx.h"
#include "constants.h"
#include "fishing.h"
#include "sectree.h"

#ifndef __FISHING_MAIN__
#include "item_manager.h"

#include "config.h"
#include "packet.h"

#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"

#include "log.h"

#include "questmanager.h"
#include "buffer_manager.h"
#include "desc_client.h"

#include "affect.h"
#include "unique_item.h"
#endif

namespace fishing
{
	enum
	{
		MAX_FISH = 37,
		NUM_USE_RESULT_COUNT = 10, // 1 : DEAD 2 : BONE 3 ~ 12 : rest
		FISH_BONE_VNUM = 27799,
		SHELLFISH_VNUM = 27987,
		EARTHWORM_VNUM = 27801,
		WATER_STONE_VNUM_BEGIN = 28030,
		WATER_STONE_VNUM_END = 28043,
		FISH_NAME_MAX_LEN = 64,
		MAX_PROB = 3,
	};

	enum
	{
		USED_NONE,
		USED_SHELLFISH,
		USED_WATER_STONE,
		USED_TREASURE_MAP,
		USED_EARTHWARM,
		MAX_USED_FISH
	};

	enum
	{
		FISHING_TIME_NORMAL,
		FISHING_TIME_SLOW,
		FISHING_TIME_QUICK,
		FISHING_TIME_ALL,
		FISHING_TIME_EASY,

		FISHING_TIME_COUNT,

		MAX_FISHING_TIME_COUNT = 31,
	};

	enum
	{
		FISHING_LIMIT_NONE,
		FISHING_LIMIT_APPEAR,
	};

	int aFishingTime[FISHING_TIME_COUNT][MAX_FISHING_TIME_COUNT] =
	{
		{   0,   0,   0,   0,   0,   2,   4,   8,  12,  16,  20,  22,  25,  30,  50,  80,  50,  30,  25,  22,  20,  16,  12,   8,   4,   2,   0,   0,   0,   0,   0 },
		{   0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   4,   8,  12,  16,  20,  22,  25,  30,  50,  80,  50,  30,  25,  22,  20 },
		{  20,  22,  25,  30,  50,  80,  50,  30,  25,  22,  20,  16,  12,   8,   4,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 },
		{ 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
		{  20,  20,  20,  20,  20,  22,  24,  28,  32,  36,  40,  42,  45,  50,  70, 100,  70,  50,  45,  42,  40,  36,  32,  28,  24,  22,  20,  20,  20,  20,  20 },
	};

	struct SFishInfo
	{
		char name[FISH_NAME_MAX_LEN];

		DWORD vnum;
		DWORD dead_vnum;
		DWORD grill_vnum;

		int prob[MAX_PROB];
		int difficulty;

		int time_type;

		int used_table[NUM_USE_RESULT_COUNT];
		// 6000 2000 1000 500 300 100 50 30 10 5 4 1
	};

	bool operator < ( const SFishInfo& lhs, const SFishInfo& rhs )
	{
		return lhs.vnum < rhs.vnum;
	}

	int g_prob_sum[MAX_PROB];
	int g_prob_accumulate[MAX_PROB][MAX_FISH];

	SFishInfo fish_info[MAX_FISH] = { { "\0", }, };

void Initialize()
{
	if (g_bAuthServer)
	{
		sys_log(0, "	fishing::Initialize: No need for auth server");
		return;
	}

	SFishInfo fish_info_bak[MAX_FISH];
	thecore_memcpy(fish_info_bak, fish_info, sizeof(fish_info));

	memset(fish_info, 0, sizeof(fish_info));


	// LOCALE_SERVICE
	const int FILE_NAME_LEN = 256;
	char szFishingFileName[FILE_NAME_LEN+1];
	snprintf(szFishingFileName, sizeof(szFishingFileName),
			"%s/fishing.txt", Locale_GetBasePath().c_str());
	FILE * fp = fopen(szFishingFileName, "r");
	// END_OF_LOCALE_SERVICE

	if (*fish_info_bak[0].name)
		SendLog("Reloading fish table.");

	if (!fp)
	{
		SendLog("error! cannot open fishing.txt");

		// ¹é¾÷¿¡ ÀÌ¸§ÀÌ ÀÖÀ¸¸é ¸®½ºÅä¾î ÇÑ´Ù.
		if (*fish_info_bak[0].name)
		{
			thecore_memcpy(fish_info, fish_info_bak, sizeof(fish_info));
			SendLog("  restoring to backup");
		}
		return;
	}

	memset(fish_info, 0, sizeof(fish_info));

	char buf[512];
	int idx = 0;

	while (fgets(buf, 512, fp))
	{
		if (*buf == '#')
			continue;

		char * p = strrchr(buf, '\n');
		*p = '\0';

		const char * start = buf;
		const char * tab = strchr(start, '\t');

		if (!tab)
		{
			printf("Tab error on line: %s\n", buf);
			SendLog("error! parsing fishing.txt");

			if (*fish_info_bak[0].name)
			{
				thecore_memcpy(fish_info, fish_info_bak, sizeof(fish_info));
				SendLog("  restoring to backup");
			}
			break;
		}

		char szCol[256], szCol2[256];
		int iColCount = 0;

		do
		{
			strlcpy(szCol2, start, MIN(sizeof(szCol2), (tab - start) + 1));
			szCol2[tab-start] = '\0';

			trim_and_lower(szCol2, szCol, sizeof(szCol));

			if (!*szCol || *szCol == '\t')
				iColCount++;
			else 
			{
				switch (iColCount++)
				{
					case 0: strlcpy(fish_info[idx].name, szCol, sizeof(fish_info[idx].name)); break;
					case 1: str_to_number(fish_info[idx].vnum, szCol); break;
					case 2: str_to_number(fish_info[idx].dead_vnum, szCol); break;
					case 3: str_to_number(fish_info[idx].grill_vnum, szCol); break;
					case 4: str_to_number(fish_info[idx].prob[0], szCol); break;
					case 5: str_to_number(fish_info[idx].prob[1], szCol); break;
					case 6: str_to_number(fish_info[idx].prob[2], szCol); break;
					case 7: str_to_number(fish_info[idx].difficulty, szCol); break;
					case 8: str_to_number(fish_info[idx].time_type, szCol); break;
					case 9: // 0
					case 10: // 1
					case 11: // 2
					case 12: // 3
					case 13: // 4
					case 14: // 5
					case 15: // 6
					case 16: // 7
					case 17: // 8
					case 18: // 9
							 str_to_number(fish_info[idx].used_table[iColCount-1-8], szCol);
							 break;
				}
			}

			start = tab + 1;
			tab = strchr(start, '\t');
		} while (tab);

		idx++;

		if (idx == MAX_FISH)
			break;
	}

	fclose(fp);

	for (int i = 0; i < MAX_FISH; ++i)
	{
		sys_log(!test_server, "FISH: %-24s vnum %5lu prob %4d %4d %4d", 
				fish_info[i].name,
				fish_info[i].vnum,
				fish_info[i].prob[0],
				fish_info[i].prob[1],
				fish_info[i].prob[2]);
	}

	// È®·ü °è»ê
	for (int j = 0; j < MAX_PROB; ++j)
	{
		g_prob_accumulate[j][0] = fish_info[0].prob[j];

		for (int i = 1; i < MAX_FISH; ++i)
			g_prob_accumulate[j][i] = fish_info[i].prob[j] + g_prob_accumulate[j][i - 1];

		g_prob_sum[j] = g_prob_accumulate[j][MAX_FISH - 1];
		sys_log(!test_server, "FISH: prob table %d %d", j, g_prob_sum[j]);
	}
}

int DetermineFishByProbIndex(int prob_idx)
{
	int rv = random_number(1, g_prob_sum[prob_idx]);
	int * p = std::lower_bound(g_prob_accumulate[prob_idx], g_prob_accumulate[prob_idx]+ MAX_FISH, rv);
	int fish_idx = p - g_prob_accumulate[prob_idx];
	return fish_idx;
}

int GetProbIndexByMapIndex(int index)
{
	switch (index)
	{
		case HOME_MAP_INDEX_RED_1:
		case HOME_MAP_INDEX_YELLOW_1:
		case HOME_MAP_INDEX_BLUE_1:
			return 0;

		case HOME_MAP_INDEX_RED_2:
		case HOME_MAP_INDEX_YELLOW_2:
		case HOME_MAP_INDEX_BLUE_2:
			return 1;

		case FLAME_MAP_INDEX:
		case SNOW_MAP_INDEX:
		case DESERT_MAP_INDEX:
			return 2;

		case MILGYO_MAP_INDEX:
			return 3;
	}

	return -1;
}

#ifndef __FISHING_MAIN__
int DetermineFish(LPCHARACTER ch)
{
	int map_idx = ch->GetMapIndex();
	int prob_idx = GetProbIndexByMapIndex(map_idx);

	if (prob_idx < 0) 
		return 0;

	int rv = random_number(1, g_prob_sum[prob_idx]);

	int * p = std::lower_bound(g_prob_accumulate[prob_idx], g_prob_accumulate[prob_idx] + MAX_FISH, rv);
	int fish_idx = p - g_prob_accumulate[prob_idx];

	DWORD vnum = fish_info[fish_idx].vnum;

	if (vnum == 50008 || vnum == 50009 || vnum == 80008) 
		return 0;

	return (fish_idx);
}

void FishingReact(LPCHARACTER ch)
{
	network::GCOutputPacket<network::GCFishingReactPacket> p;
	p->set_vid(ch->GetVID());
	ch->PacketAround(p);
}

void FishingSuccess(LPCHARACTER ch)
{
	network::GCOutputPacket<network::GCFishingSuccessPacket> p;
	p->set_vid(ch->GetVID());
	ch->PacketAround(p);
}

void FishingFail(LPCHARACTER ch)
{
	network::GCOutputPacket<network::GCFishingFailPacket> p;
	p->set_vid(ch->GetVID());
	ch->PacketAround(p);
}

void FishingPractice(LPCHARACTER ch)
{
	LPITEM rod = ch->GetWear(WEAR_WEAPON);
	if (rod && rod->GetType() == ITEM_ROD)
	{
		// ÃÖ´ë ¼ö·Ãµµ°¡ ¾Æ´Ñ °æ¿ì ³¬½Ã´ë ¼ö·Ã
		if ( rod->GetRefinedVnum()>0 && rod->GetSocket(0) < rod->GetValue(2) && random_number(1,rod->GetValue(1))==1 )
		{
			rod->SetSocket(0, rod->GetSocket(0) + 1);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³¬½Ã´ëÀÇ ¼ö·Ãµµ°¡ Áõ°¡ÇÏ¿´½À´Ï´Ù! (%d/%d)"),rod->GetSocket(0), rod->GetValue(2));
			if (rod->GetSocket(0) == rod->GetValue(2))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³¬½Ã´ë°¡ ÃÖ´ë ¼ö·Ãµµ¿¡ µµ´ÞÇÏ¿´½À´Ï´Ù."));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾îºÎ¸¦ ÅëÇØ ´ÙÀ½ ·¹º§ÀÇ ³¬½Ã´ë·Î ¾÷±×·¹ÀÌµå ÇÒ ¼ö ÀÖ½À´Ï´Ù."));
			}
		}
	}
	// ¹Ì³¢¸¦ »«´Ù
	rod->SetSocket(2, 0);
}

bool PredictFish(LPCHARACTER ch)
{
	// ADD_PREMIUM
	// ¾î½ÉÈ¯
	if (ch->FindAffect(AFFECT_FISH_MIND_PILL) || 
			ch->GetPremiumRemainSeconds(PREMIUM_FISH_MIND) > 0)
		return true;
	// END_OF_ADD_PREMIUM

	return false;
}

EVENTFUNC(fishing_event)
{
	fishing_event_info * info = dynamic_cast<fishing_event_info *>( event->info );

	if ( info == NULL )
	{
		sys_err( "fishing_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->pid);

	if (!ch)
		return 0;

	LPITEM rod = ch->GetWear(WEAR_WEAPON);

	if (!(rod && rod->GetType() == ITEM_ROD))
	{
		ch->m_pkFishingEvent = NULL;
		return 0;
	}

	switch (info->step)
	{
		case 0:	// Èçµé¸®±â ¶Ç´Â ¶±¹ä¸¸ ³¯¾Æ°¨
			++info->step;

			//info->ch->Motion(MOTION_FISHING_SIGN);
			info->hang_time = get_dword_time();
			info->fish_id = DetermineFish(ch);
			FishingReact(ch);

			if (fishing::PredictFish(ch))
			{
				network::GCOutputPacket<network::GCFishingFishInfoPacket> p;
				p->set_info(fish_info[info->fish_id].vnum);
				ch->GetDesc()->Packet(p);
			}
			return (PASSES_PER_SEC(6));

		default:
			++info->step;

			if (info->step > 5)
				info->step = 5;

			ch->m_pkFishingEvent = NULL;
			FishingFail(ch);
			rod->SetSocket(2, 0);
			return 0;
	}
}

LPEVENT CreateFishingEvent(LPCHARACTER ch)
{
	fishing_event_info* info = AllocEventInfo<fishing_event_info>();
	info->pid	= ch->GetPlayerID();
	info->step	= 0;
	info->hang_time	= 0;

	int time = random_number(10, 40);

	network::GCOutputPacket<network::GCFishingStartPacket> p;
	p->set_vid(ch->GetVID());
	p->set_dir(ch->GetRotation() / 5);
	ch->PacketAround(p);

	return event_create(fishing_event, info, PASSES_PER_SEC(time));
}

int GetFishingLevel(LPCHARACTER ch)
{
	LPITEM rod = ch->GetWear(WEAR_WEAPON);

	if (!rod || rod->GetType()!= ITEM_ROD)
		return 0;

	return rod->GetSocket(2) + rod->GetValue(0);
}

int Compute(DWORD fish_id, DWORD ms, DWORD* item, int level)
{
	if (fish_id == 0)
		return -2;

	if (fish_id >= MAX_FISH)
	{
		sys_err("Wrong FISH ID : %d", fish_id);
		return -2; 
	}

	if (ms > 6000)
		return -1;

	int time_step = MINMAX(0,((ms + 99) / 200), MAX_FISHING_TIME_COUNT - 1);

	if (random_number(1, 100) <= aFishingTime[fish_info[fish_id].time_type][time_step])
	{
		if (random_number(1, fish_info[fish_id].difficulty) <= level)
		{
			*item = fish_info[fish_id].vnum;
			return 0;
		}
		return -3;
	}

	return -1;
}

void Take(fishing_event_info* info, LPCHARACTER ch)
{
	if (quest::CQuestManager::instance().GetEventFlag("disable_fishdetect") == 0)
	{
		if (ch->GetLastSpacebarTime() + 41 > get_global_time())
		{
			ch->tchat("HitSpacebar check OK");
		}
		else
		{
			LogManager::instance().HackDetectionLog(ch, "FISHBOT", "NO SPACEKEY");
			if (quest::CQuestManager::instance().GetEventFlag("disable_fishbots"))
			{
				FishingFail(ch);
				return;
			}
			ch->tchat("HitSpacebar check FAILED BUSTED");
		}
	}
	
	if (!ch->IsNearWater())
	{
		sys_err("try to fish without being in the near of water (player %d %s pos %d, %d mapIndex %d)",
				ch->GetPlayerID(), ch->GetName(), ch->GetX(), ch->GetY(), ch->GetMapIndex());
		FishingFail(ch);
		return;
	}
	else if (!ch->GetWear(WEAR_WEAPON) || ch->GetWear(WEAR_WEAPON)->GetType() != ITEM_ROD)
	{
		sys_err("try to fish without equipped rod (player %d %s weapon %d)",
				ch->GetPlayerID(), ch->GetName(), ch->GetWear(WEAR_WEAPON) ? ch->GetWear(WEAR_WEAPON)->GetID() : 0);
		FishingFail(ch);
		return;
	}

	if (info->step == 1)	// °í±â°¡ °É¸° »óÅÂ¸é..
	{
		long ms = (long) ((get_dword_time() - info->hang_time));
		DWORD item_vnum = 0;
		int ret = Compute(info->fish_id, ms, &item_vnum, fishing::GetFishingLevel(ch));

		//if (test_server)
		//ch->ChatPacket(CHAT_TYPE_INFO, "%d ms", ms);

		switch (ret)
		{
			case -2: // ÀâÈ÷Áö ¾ÊÀº °æ¿ì
			case -3: // ³­ÀÌµµ ¶§¹®¿¡ ½ÇÆÐ
			case -1: // ½Ã°£ È®·ü ¶§¹®¿¡ ½ÇÆÐ
				//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(//ch, "°í±â°¡ ¹Ì³¢¸¸ »©¸Ô°í Àì½Î°Ô µµ¸ÁÄ¨´Ï´Ù."));
				{
					int map_idx = ch->GetMapIndex();
					int prob_idx = GetProbIndexByMapIndex(map_idx);

					LogManager::instance().FishLog(
							ch->GetPlayerID(),
							prob_idx,
							info->fish_id,
							fishing::GetFishingLevel(ch),
							ms);
				}
				FishingFail(ch);
				break;

			case 0:
				//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(//ch, "°í±â°¡ ÀâÇû½À´Ï´Ù! (%s)"), fish_info[info->fish_id].name);
				if (item_vnum)
				{
					FishingSuccess(ch);

#ifdef AHMET_FISH_EVENT_SYSTEM
					if(quest::CQuestManager::instance().GetEventFlag("enable_fish_event"))
					{
						if(ch->IsEquipUniqueItem(UNIQUE_ITEM_FISH_MIND_1) || ch->IsEquipUniqueItem(UNIQUE_ITEM_FISH_MIND_2) || ch->IsEquipUniqueItem(UNIQUE_ITEM_FISH_MIND_3))
						{
							int iChanceForSpecial = quest::CQuestManager::instance().GetEventFlag("event_fish_drop");

							if(test_server)
								iChanceForSpecial = 85;

							int r = random_number(1, 100);

							if(r <= iChanceForSpecial)
								ch->AutoGiveItem(ITEM_FISH_EVENT_BOX_SPECIAL, 1, -1, false);
							else
								ch->AutoGiveItem(ITEM_FISH_EVENT_BOX, 5, -1, false);
						}
						else
						{
							ch->AutoGiveItem(ITEM_FISH_EVENT_BOX, 5, -1, false);
						}
					}
#endif

					network::GCOutputPacket<network::GCFishingFishInfoPacket> p;
					p->set_info(item_vnum);
					ch->GetDesc()->Packet(p);

					LPITEM item = ch->AutoGiveItem(item_vnum, 1, -1, false);

					int map_idx = ch->GetMapIndex();
					int prob_idx = GetProbIndexByMapIndex(map_idx);

					LogManager::instance().FishLog(
							ch->GetPlayerID(),
							prob_idx,
							info->fish_id,
							fishing::GetFishingLevel(ch),
							ms,
							true,
							item ? item->GetSocket(0) : 0);

						quest::CQuestManager::instance().CatchFish(ch->GetPlayerID(),item);
				}
				else
				{
					int map_idx = ch->GetMapIndex();
					int prob_idx = GetProbIndexByMapIndex(map_idx);

					LogManager::instance().FishLog(
							ch->GetPlayerID(),
							prob_idx,
							info->fish_id,
							fishing::GetFishingLevel(ch),
							ms);
					FishingFail(ch);
				}
				break;
		}
	}
	else if (info->step > 1)
	{
		int map_idx = ch->GetMapIndex();
		int prob_idx = GetProbIndexByMapIndex(map_idx);

		LogManager::instance().FishLog(
				ch->GetPlayerID(),
				prob_idx,
				info->fish_id,
				fishing::GetFishingLevel(ch),
				7000);
		//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(//ch, "°í±â°¡ ¹Ì³¢¸¸ »©¸Ô°í Àì½Î°Ô µµ¸ÁÄ¨´Ï´Ù."));
		FishingFail(ch);
	}
	else
	{
		network::GCOutputPacket<network::GCFishingStopPacket> p;
		p->set_vid(ch->GetVID());
		ch->PacketAround(p);
	}

	if (info->step)
	{
		FishingPractice(ch);
	}
	//Motion(MOTION_FISHING_PULL);
}

void Simulation(int level, int count, int prob_idx, LPCHARACTER ch)
{
	std::map<std::string, int> fished;
	int total_count = 0;

	for (int i = 0; i < count; ++i)
	{
		int fish_id = DetermineFishByProbIndex(prob_idx);
		DWORD item = 0;
		Compute(fish_id, (random_number(2000, 4000) + random_number(2000,4000)) / 2, &item, level);

		if (item)
		{
			fished[fish_info[fish_id].name]++;
			total_count ++;
		}
	}

	for (std::map<std::string,int>::iterator it = fished.begin(); it != fished.end(); ++it)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s : %d ¸¶¸®"), it->first.c_str(), it->second);

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%d Á¾·ù %d ¸¶¸® ³¬À½"), fished.size(), total_count);
}

void UseFish(LPCHARACTER ch, LPITEM item)
{
	/*
	int idx = 0;
	for (int i = 1; i < MAX_FISH; ++i)
	{
		if (fish_info[i].vnum == item->GetVnum())
		{
			idx = i;
			break;
		}
	}

	// ÇÇ¶ó¹Ì »ç¿ëºÒ°¡, »ì¾ÆÀÖ´Â°Ô ¾Æ´Ñ°Ç »ç¿ëºÒ°¡

	if (idx<=1 || idx >= MAX_FISH)
		return;

	int r = number(1, 10000);

	int changeProb = 0;
	if (ch->GetMapIndex() == SNOW_MAP_INDEX)
		changeProb = 1000;
	else if (ch->GetMapIndex() == FLAME_MAP_INDEX || ch->GetMapIndex() == DESERT_MAP_INDEX)
		changeProb = -1000;
	if (ch->IsNearWater())
		changeProb += 1000;

	item->SetCount(item->GetCount()-1);

	if (r >= 4001 + changeProb)
	{
		// Á×Àº ¹°°í±â
		ch->AutoGiveItem(fish_info[idx].dead_vnum);
	}
	else if (r >= 2001 + changeProb)
	{
		// »ý¼±»À
		ch->AutoGiveItem(FISH_BONE_VNUM);
	}
	else
	{
		// 1000 500 300 100 50 30 10 5 4 1
		static int s_acc_prob[NUM_USE_RESULT_COUNT] = { 1000, 1500, 1800, 1900, 1950, 1980, 1990, 1995, 1999, 2000 };
		int u_index = std::lower_bound(s_acc_prob, s_acc_prob + NUM_USE_RESULT_COUNT, r) - s_acc_prob;

		switch (fish_info[idx].used_table[u_index])
		{
			case USED_TREASURE_MAP:	// 3
			case USED_NONE:		// 0
			case USED_WATER_STONE:	// 2
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°í±â°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
				break;

			case USED_SHELLFISH:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¹è ¼Ó¿¡¼­ Á¶°³°¡ ³ª¿Ô½À´Ï´Ù."));
				ch->AutoGiveItem(SHELLFISH_VNUM);
				break;

			case USED_EARTHWARM:	// 4
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¹è ¼Ó¿¡¼­ Áö··ÀÌ°¡ ³ª¿Ô½À´Ï´Ù."));
				ch->AutoGiveItem(EARTHWORM_VNUM);
				break;

			default:
				ch->AutoGiveItem(fish_info[idx].used_table[u_index]);
				break;
		}
	}
	*/
}

void Grill(LPCHARACTER ch, LPITEM item)
{
	/*if (item->GetVnum() < fish_info[3].vnum)
	  return;
	  int idx = item->GetVnum()-fish_info[3].vnum+3;
	  if (idx>=MAX_FISH)
	  idx = item->GetVnum()-fish_info[3].dead_vnum+3;
	  if (idx>=MAX_FISH)
	  return;*/
	int idx = -1;
	DWORD vnum = item->GetVnum();

	for (int i = 0; i < MAX_FISH; ++i)
	{
		if (fish_info[i].vnum == vnum || fish_info[i].dead_vnum == vnum)
		{
			idx = i;
			break;
		}
	}
	
	if (idx == -1)
		return;

	int count = item->GetCount();

	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s¸¦ ±¸¿ü½À´Ï´Ù."), item->GetName(ch->GetLanguageID()));
	item->SetCount(0);
	ch->AutoGiveItem(fish_info[idx].grill_vnum, count);
}

bool RefinableRod(LPITEM rod)
{
	if (rod->GetType() != ITEM_ROD)
		return false;

	if (rod->IsEquipped())
		return false;

	return (rod->GetSocket(0) == rod->GetValue(2));
}

int RealRefineRod(LPCHARACTER ch, LPITEM item)
{
	if (!ch || !item)
		return 2;

	// REFINE_ROD_HACK_BUG_FIX
	if (!RefinableRod(item))
	{
		sys_err("REFINE_ROD_HACK pid(%u) item(%s:%d)", ch->GetPlayerID(), item->GetName(), item->GetID());

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), -1, 1, "ROD_HACK");

		return 2;
	}
	// END_OF_REFINE_ROD_HACK_BUG_FIX

	LPITEM rod = item;	

	int iAdv = rod->GetValue(0) / 10;

	if (random_number(1,100) <= rod->GetValue(3))
	{
		LogManager::instance().RefineLog(ch->GetPlayerID(), rod->GetName(), rod->GetID(), iAdv, 1, "ROD");

		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(rod->GetRefinedVnum(), 1);

		if (pkNewItem)
		{
			WORD wCell = rod->GetCell();
			// ³¬½Ã´ë °³·® ¼º°ø
			ITEM_MANAGER::instance().RemoveItem(rod, "REMOVE (REFINE FISH_ROD)");
			pkNewItem->AddToCharacter(ch, TItemPos (INVENTORY, wCell)); 
			LogManager::instance().ItemLog(ch, pkNewItem, "REFINE FISH_ROD SUCCESS", pkNewItem->GetName());
			return 1;
		}

		// ³¬½Ã´ë °³·® ½ÇÆÐ
		return 2;
	}
	else
	{
		LogManager::instance().RefineLog(ch->GetPlayerID(), rod->GetName(), rod->GetID(), iAdv, 0, "ROD");

		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(rod->GetValue(4), 1);
		if (pkNewItem)
		{
			WORD wCell = rod->GetCell();
			// ³¬½Ã´ë °³·®¿¡ ¼º°ø
			ITEM_MANAGER::instance().RemoveItem(rod, "REMOVE (REFINE FISH_ROD)");
			pkNewItem->AddToCharacter(ch, TItemPos(INVENTORY, wCell)); 
			LogManager::instance().ItemLog(ch, pkNewItem, "REFINE FISH_ROD FAIL", pkNewItem->GetName());
			return 0;
		}

		// ³¬½Ã´ë °³·® ½ÇÆÐ
		return 2;
	}
}
#endif
}

#ifdef __FISHING_MAIN__
int main(int argc, char **argv)
{
	//srandom(time(0) + getpid());
	srandomdev();
	/*
	   struct SFishInfo
	   {
	   const char* name;

	   DWORD vnum;
	   DWORD dead_vnum;
	   DWORD grill_vnum;

	   int prob[3];
	   int difficulty;

	   int limit_type;
	   int limits[3];

	   int time_type;
	   int length_range[3]; // MIN MAX EXTRA_MAX : 99% MIN~MAX, 1% MAX~EXTRA_MAX

	   int used_table[NUM_USE_RESULT_COUNT];
	// 6000 2000 1000 500 300 100 50 30 10 5 4 1
	};
	 */
	using namespace fishing;

	Initialize();

	for (int i = 0; i < MAX_FISH; ++i)
	{
		printf("%s\t%u\t%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
				fish_info[i].name,
				fish_info[i].vnum,
				fish_info[i].dead_vnum,
				fish_info[i].grill_vnum,
				fish_info[i].prob[0], 
				fish_info[i].prob[1], 
				fish_info[i].prob[2],
				fish_info[i].difficulty,
				fish_info[i].time_type,
				fish_info[i].length_range[0],
				fish_info[i].length_range[1],
				fish_info[i].length_range[2]);

		for (int j = 0; j < NUM_USE_RESULT_COUNT; ++j)
			printf("\t%d", fish_info[i].used_table[j]);

		puts("");
	}

	return 1;
}

#endif
