#include "stdafx.h"
#include <fstream>
#include "constants.h"
#include "buffer_manager.h"
#include "packet.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"
#include "lzo_manager.h"
#include "item.h"
#include "config.h"
#include "xmas_event.h"
#include "target.h"
#include "party.h"

#include "dungeon.h"

DWORD g_GoldDropTimeLimitValue = 0;
extern bool DropEvent_CharStone_SetValue(const std::string& name, int value);
extern bool DropEvent_RefineBox_SetValue (const std::string& name, int value);

#ifdef ENABLE_BLOCK_PKMODE
struct FSetPKModeInMap
{
	BYTE pkMode;
	bool mode;

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER)ent;

			if (ch->IsPC())
				ch->SetBlockPKMode(pkMode, mode);
		}
	}
};
#endif

LPEVENT bossHuntEvent = NULL;
LPEVENT bossHuntSpawnEvent = NULL;

EVENTINFO(bosshunt_event_info)
{
	std::map<long, std::vector<DWORD>> bossVids;
	std::map<long, std::queue<time_t>> nextSpawn;
	std::map<DWORD, size_t> usedCoords;
	bool setEnd = false;
#ifdef BOSSHUNT_EVENT_UPDATE
	std::map<long, std::vector<Coord>> allowedCoords;
#endif
};

void SpawnBossHuntMobs(long mapIndex = 0)
{
	bosshunt_event_info* info = bossHuntEvent ? dynamic_cast<bosshunt_event_info*>(bossHuntEvent->info) : NULL;
	if (!info)
		return;

	for (std::vector<BossHuntSpawnPos>::const_iterator it = g_bossHuntSpawnPos.cbegin(); it != g_bossHuntSpawnPos.cend(); ++it)
	{
		if (!mapIndex || it->lMapIndex == mapIndex)
		{
			if (info->bossVids[it->lMapIndex].size() >= 2)
				continue;

			if (info->nextSpawn[it->lMapIndex].size() >= 2 && info->nextSpawn[it->lMapIndex].front() > get_global_time())
				continue;

			LPSECTREE_MAP map = SECTREE_MANAGER::instance().GetMap(it->lMapIndex);

			if (!map)
				continue;
#ifdef BOSSHUNT_EVENT_UPDATE
			if (info->allowedCoords[it->lMapIndex].empty())
				info->allowedCoords[it->lMapIndex] = it->vCoords;

			size_t selectCoord = random_number(0, info->allowedCoords[it->lMapIndex].size() - 1);
			info->allowedCoords[it->lMapIndex].erase(info->allowedCoords[it->lMapIndex].begin() + selectCoord);
#else
			size_t selectCoord;
			bool isOK = info->usedCoords.empty();
			do
			{
				selectCoord = random_number(0, it->vCoords.size() - 1);
				for (itertype(info->usedCoords) it2 = info->usedCoords.begin(); it2 != info->usedCoords.end(); ++it2)
				{
					if (it2->second == selectCoord)
					{
						isOK = false;
						break;
					}
					else
						isOK = true;
				}
			} while (!isOK);
#endif

			const Coord &crd = it->vCoords[selectCoord];
#ifdef BOSSHUNT_EVENT_UPDATE
			DWORD vnum = BOSSHUNT_BOSS_VNUM;

			if (it->lMapIndex >= DRAGON_CAPE_MAP_INDEX)
				vnum = BOSSHUNT_BOSS_VNUM2;

			LPCHARACTER mob = CHARACTER_MANAGER::instance().SpawnMob(vnum, it->lMapIndex,
				map->m_setting.iBaseX + crd.x * 100, map->m_setting.iBaseY + crd.y * 100, 0);
#else
			LPCHARACTER mob = CHARACTER_MANAGER::instance().SpawnMob(BOSSHUNT_BOSS_VNUM, it->lMapIndex,
				map->m_setting.iBaseX + crd.x * 100, map->m_setting.iBaseY + crd.y * 100, 0);
#endif
			if (mob)
			{
				info->bossVids[it->lMapIndex].push_back(mob->GetVID());
				if (info->nextSpawn[it->lMapIndex].size() >= 2)
					info->nextSpawn[it->lMapIndex].pop();
				
				info->nextSpawn[it->lMapIndex].push(test_server ? get_global_time() + 30 : get_global_time() + 3600);
				info->usedCoords[mob->GetVID()] = selectCoord;

				if (test_server)
				{
					char notice[64];
					sprintf(notice, "boss spawn at %d x %d", mob->GetX(), mob->GetY());
					SendNotice(notice, LANGUAGE_GERMAN);
				}
			}
		}
	}
}

void ClearBossHuntMobs()
{
	if (!bossHuntEvent)
		return;

	bosshunt_event_info* info = dynamic_cast<bosshunt_event_info*>(bossHuntEvent->info);
	if (!info)
		return;

	std::vector<LPCHARACTER> deleteVec; //prevent DelBossHuntMob respawn

	for (itertype(info->bossVids) it2 = info->bossVids.begin(); it2 != info->bossVids.end(); ++it2)
		for (std::vector<DWORD>::iterator it = it2->second.begin(); it != it2->second.end(); ++it)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(*it);
			if (ch)
				deleteVec.push_back(ch);
				//M2_DESTROY_CHARACTER(ch);
		}

	info->bossVids.clear();
	info->nextSpawn.clear();
	info->usedCoords.clear();
#ifdef BOSSHUNT_EVENT_UPDATE
	info->allowedCoords.clear();
#endif

	for (itertype(deleteVec) it = deleteVec.begin(); it != deleteVec.end(); ++it)
		M2_DESTROY_CHARACTER(*it);
}

void DelBossHuntMob(DWORD vid, long mapIndex)
{
	if (!bossHuntEvent)
		return;

	bosshunt_event_info* info = dynamic_cast<bosshunt_event_info*>(bossHuntEvent->info);
	if (!info)
		return;

	std::vector<DWORD>::iterator f = std::find(info->bossVids[mapIndex].begin(), info->bossVids[mapIndex].end(), vid);
	if (f != info->bossVids[mapIndex].end())
	{
		info->bossVids[mapIndex].erase(f);
		info->usedCoords.erase(vid);
		SpawnBossHuntMobs(mapIndex);
	}
}

EVENTFUNC(bosshunt_event)
{
	if (event == NULL)
		return 0;

	if (event->info == NULL)
		return 0;

	bosshunt_event_info* info = dynamic_cast<bosshunt_event_info*>(bossHuntEvent->info);
	if (!info)
		return 0;

	if (!info->setEnd)
		return 0;

	if (quest::CQuestManager::instance().GetEventFlag("enable_bosshunt_event"))
		quest::CQuestManager::instance().RequestSetEventFlag("enable_bosshunt_event", 0);

	ClearBossHuntMobs();
	bossHuntEvent = NULL;
	return 0;
}

EVENTINFO(bosshunt_event_respawn_info)
{
	BYTE _dummy;
};

EVENTFUNC(bosshunt_event_respawn_timer)
{
	SpawnBossHuntMobs();
	return PASSES_PER_SEC(5);
}

namespace quest
{
	using namespace std;

	CQuestManager::CQuestManager()
		: m_pSelectedDungeon(NULL), m_dwServerTimerArg(0), m_iRunningEventIndex(0), L(NULL), m_bNoSend (false),
		m_CurrentRunningState(NULL), m_pCurrentCharacter(NULL), m_pCurrentNPCCharacter(NULL), m_pCurrentPartyMember(NULL),
		m_pCurrentPC(NULL),  m_iCurrentSkin(0), m_bError(false), m_pOtherPCBlockRootPC(NULL)
	{
	}

	CQuestManager::~CQuestManager()
	{
		Destroy();
	}

	void CQuestManager::Destroy()
	{
		if (L)
		{
			lua_close(L);
			L = NULL;
		}
	}	

	bool CQuestManager::Initialize()
	{
		if (g_bAuthServer)
			return true;

		if (!InitializeLua())
			return false;

		m_pSelectedDungeon = NULL;

		m_mapEventName.insert(TEventNameMap::value_type("click", QUEST_CLICK_EVENT));		// NPC¸¦ Å¬¸¯
		m_mapEventName.insert(TEventNameMap::value_type("kill", QUEST_KILL_EVENT));		// MobÀ» »ç³É
		m_mapEventName.insert(TEventNameMap::value_type("timer", QUEST_TIMER_EVENT));		// ¹Ì¸® ÁöÁ¤ÇØµÐ ½Ã°£ÀÌ Áö³²
		m_mapEventName.insert(TEventNameMap::value_type("levelup", QUEST_LEVELUP_EVENT));	// ·¹º§¾÷À» ÇÔ
		m_mapEventName.insert(TEventNameMap::value_type("login", QUEST_LOGIN_EVENT));		// ·Î±×ÀÎ ½Ã
		m_mapEventName.insert(TEventNameMap::value_type("logout", QUEST_LOGOUT_EVENT));		// ·Î±×¾Æ¿ô ½Ã
		m_mapEventName.insert(TEventNameMap::value_type("button", QUEST_BUTTON_EVENT));		// Äù½ºÆ® ¹öÆ°À» ´©¸§
		m_mapEventName.insert(TEventNameMap::value_type("info", QUEST_INFO_EVENT));		// Äù½ºÆ® Á¤º¸Ã¢À» ¿°
		m_mapEventName.insert(TEventNameMap::value_type("chat", QUEST_CHAT_EVENT));		// Æ¯Á¤ Å°¿öµå·Î ´ëÈ­¸¦ ÇÔ
		m_mapEventName.insert(TEventNameMap::value_type("in", QUEST_ATTR_IN_EVENT));		// ¸ÊÀÇ Æ¯Á¤ ¼Ó¼º¿¡ µé¾î°¨
		m_mapEventName.insert(TEventNameMap::value_type("out", QUEST_ATTR_OUT_EVENT));		// ¸ÊÀÇ Æ¯Á¤ ¼Ó¼º¿¡¼­ ³ª¿È
		m_mapEventName.insert(TEventNameMap::value_type("use", QUEST_ITEM_USE_EVENT));		// Äù½ºÆ® ¾ÆÀÌÅÛÀ» »ç¿ë
		m_mapEventName.insert(TEventNameMap::value_type("server_timer", QUEST_SERVER_TIMER_EVENT));	// ¼­¹ö Å¸ÀÌ¸Ó (¾ÆÁ÷ Å×½ºÆ® ¾ÈµÆÀ½)
		m_mapEventName.insert(TEventNameMap::value_type("enter", QUEST_ENTER_STATE_EVENT));	// ÇöÀç ½ºÅ×ÀÌÆ®°¡ µÊ
		m_mapEventName.insert(TEventNameMap::value_type("leave", QUEST_LEAVE_STATE_EVENT));	// ÇöÀç ½ºÅ×ÀÌÆ®¿¡¼­ ´Ù¸¥ ½ºÅ×ÀÌÆ®·Î ¹Ù²ñ
		m_mapEventName.insert(TEventNameMap::value_type("letter", QUEST_LETTER_EVENT));		// ·Î±ä ÇÏ°Å³ª ½ºÅ×ÀÌÆ®°¡ ¹Ù²¸ »õ·Î Á¤º¸¸¦ ¼¼ÆÃÇØÁà¾ßÇÔ
		m_mapEventName.insert(TEventNameMap::value_type("take", QUEST_ITEM_TAKE_EVENT));	// ¾ÆÀÌÅÛÀ» ¹ÞÀ½
		m_mapEventName.insert(TEventNameMap::value_type("target", QUEST_TARGET_EVENT));		// Å¸°Ù
		m_mapEventName.insert(TEventNameMap::value_type("party_kill", QUEST_PARTY_KILL_EVENT));	// ÆÄÆ¼ ¸â¹ö°¡ ¸ó½ºÅÍ¸¦ »ç³É (¸®´õ¿¡°Ô ¿È)
		m_mapEventName.insert(TEventNameMap::value_type("mount", QUEST_MOUNT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("unmount", QUEST_UNMOUNT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("pick", QUEST_ITEM_PICK_EVENT));	// ¶³¾îÁ®ÀÖ´Â ¾ÆÀÌÅÛÀ» ½ÀµæÇÔ.
		m_mapEventName.insert(TEventNameMap::value_type("sig_use", QUEST_SIG_USE_EVENT));		// Special item group¿¡ ¼ÓÇÑ ¾ÆÀÌÅÛÀ» »ç¿ëÇÔ.
		m_mapEventName.insert(TEventNameMap::value_type("item_informer", QUEST_ITEM_INFORMER_EVENT));	// µ¶ÀÏ¼±¹°±â´ÉÅ×½ºÆ®
		m_mapEventName.insert(TEventNameMap::value_type("catch_fish", QUEST_CATCH_FISH));
		m_mapEventName.insert(TEventNameMap::value_type("mine_ore", QUEST_MINE_ORE));
		m_mapEventName.insert(TEventNameMap::value_type("dungeon_complete", QUEST_DUNGEON_COMPLETE));
		m_mapEventName.insert(TEventNameMap::value_type("drop_quest_item", QUEST_DROP_QUEST_ITEM));
		m_mapEventName.insert(TEventNameMap::value_type("send_shout", QUEST_SEND_SHOUT_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("add_friend", QUEST_ADD_FRIEND_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("sell_item", QUEST_SELL_ITEM_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("item_used", QUEST_ITEM_USED_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("mount_riding", QUEST_MOUNT_RIDING_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("complete_missionbook", QUEST_COMPLETE_MISSIONBOOKS_EVENT));
#ifdef __DAMAGE_QUEST_TRIGGER__
		m_mapEventName.insert(TEventNameMap::value_type("damage", QUEST_DAMAGE_EVENT));
#endif
		m_mapEventName.insert(TEventNameMap::value_type("collect_yang_from_monster", QUEST_COLLECT_YANG_FROM_MOB_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("spend_souls", QUEST_SPEND_SOULS_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("craft_gaya", QUEST_CRAFT_GAYA_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("kill_enemy_fraction", QUEST_KILL_ENEMY_FRACTION_EVENT));

// XMAS Battlepass Triggers
		m_mapEventName.insert(TEventNameMap::value_type("xmas_open_door", QUEST_XMAS_OPEN_DOOR_EVENT));
		m_mapEventName.insert(TEventNameMap::value_type("xmas_santa_sock", QUEST_XMAS_SANTA_SOCKS_EVENT));

		m_mapEventName.insert(TEventNameMap::value_type("die", QUEST_DEAD_EVENT));

#ifdef DUNGEON_REPAIR_TRIGGER
		m_mapEventName.insert(TEventNameMap::value_type("dungeon_repair", QUEST_DUNGEON_REPAIR_EVENT));
#endif

		m_bNoSend = false;

		m_iCurrentSkin = QUEST_SKIN_NORMAL;

		{
			ifstream inf((Locale_GetQuestPath() + "/questnpc.txt").c_str());
			int line = 0;

			if (!inf.is_open())
				sys_err( "QUEST Cannot open 'questnpc.txt'");
			else
				sys_log(0, "QUEST can open 'questnpc.txt' (%s)", Locale_GetQuestPath().c_str());

			while (1)
			{
				unsigned int vnum;

				inf >> vnum;

				line++;

				if (inf.fail())
					break;

				string s;
				getline(inf, s);
				unsigned int li = 0, ri = s.size()-1;
				while (li < s.size() && isspace(s[li])) li++;
				while (ri > 0 && isspace(s[ri])) ri--;

				if (ri < li) 
				{
					sys_err("QUEST questnpc.txt:%d:npc name error",line);
					continue;
				}

				s = s.substr(li, ri-li+1);

				int	n = 0;
				str_to_number(n, s.c_str());
				if (n)
					continue;

				//cout << '-' << s << '-' << endl;
				if ( test_server )
					sys_log(0, "QUEST reading script of %s(%d)", s.c_str(), vnum);
				m_mapNPC[vnum].Set(vnum, s);
				m_mapNPCNameID[s] = vnum;
			}

			// notarget quest
			m_mapNPC[0].Set(0, "notarget");
		}

		SetEventFlag("guild_withdraw_delay", 1);
		SetEventFlag("guild_disband_delay", 1);
		return true;
	}

	unsigned int CQuestManager::FindNPCIDByName(const string& name)
	{
		map<string, unsigned int>::iterator it = m_mapNPCNameID.find(name);
		return it != m_mapNPCNameID.end() ? it->second : 0;
	}

	void CQuestManager::SelectItem(unsigned int pc, unsigned int selection)
	{
		PC* pPC = GetPC(pc);
		if (pPC && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_SELECT_ITEM)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args=1;
			lua_pushnumber(pPC->GetRunningQuestState()->co,selection);

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
	}

	void CQuestManager::Confirm(unsigned int pc, EQuestConfirmType confirm, unsigned int pc2)
	{
		PC* pPC = GetPC(pc);

		if (!pPC->IsRunning())
		{
			sys_err("no quest running for pc, cannot process input : %u", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_CONFIRM)
		{
			sys_err("not wait for a confirm : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		if (pc2 && !pPC->IsConfirmWait(pc2))
		{
			sys_err("not wait for a confirm : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->ClearConfirmWait();

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args=1;
		lua_pushnumber(pPC->GetRunningQuestState()->co, confirm);

		AddScript("{END_CONFIRM_WAIT}");
		SetSkinStyle(QUEST_SKIN_NOWINDOW);
		SendScript();

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}

	}

	void CQuestManager::Input(unsigned int pc, const char* msg)
	{
		PC* pPC = GetPC(pc);
		if (!pPC)
		{
			sys_err("no pc! : %u",pc);
			return;
		}

		if (!pPC->IsRunning())
		{
			sys_err("no quest running for pc, cannot process input : %u", pc);
			return;
		}

		if (pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_INPUT)
		{
			sys_err("not wait for a input : %u %d", pc, pPC->GetRunningQuestState()->suspend_state);
			return;
		}

		pPC->SetSendDoneFlag();

		pPC->GetRunningQuestState()->args=1;
		lua_pushstring(pPC->GetRunningQuestState()->co,msg);

		if (!RunState(*pPC->GetRunningQuestState()))
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->EndRunning();
		}
	}

	void CQuestManager::Select(unsigned int pc, unsigned int selection)
	{
		PC* pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state==SUSPEND_STATE_SELECT)
		{
			pPC->SetSendDoneFlag();

			if (!pPC->GetRunningQuestState()->chat_scripts.empty())
			{
				// Ã¤ÆÃ ÀÌº¥Æ®ÀÎ °æ¿ì
				// ÇöÀç Äù½ºÆ®´Â ¾î´À Äù½ºÆ®¸¦ ½ÇÇàÇÒ °ÍÀÎ°¡¸¦ °í¸£´Â Äù½ºÆ® ÀÌ¹Ç·Î
				// ³¡³»°í ¼±ÅÃµÈ Äù½ºÆ®¸¦ ½ÇÇàÇÑ´Ù.
				QuestState& old_qs = *pPC->GetRunningQuestState();
				CloseState(old_qs);

				if (selection >= pPC->GetRunningQuestState()->chat_scripts.size())
				{
					pPC->SetSendDoneFlag();
					GotoEndState(old_qs);
					pPC->EndRunning();
				}
				else
				{
					AArgScript* pas = pPC->GetRunningQuestState()->chat_scripts[selection];
					ExecuteQuestScript(*pPC, pas->quest_index, pas->state_index, pas->script.GetCode(), pas->script.GetSize());
				}
			}
			else
			{
				// on default 
				pPC->GetRunningQuestState()->args=1;
				lua_pushnumber(pPC->GetRunningQuestState()->co,selection+1);

				if (!RunState(*pPC->GetRunningQuestState()))
				{
					CloseState(*pPC->GetRunningQuestState());
					pPC->EndRunning();
				}
			}
		}
		else
		{
			sys_err("wrong QUEST_SELECT request! : %d", pc);

			if (test_server)
				sys_err("PC: %u running %d runningQuestState %p questIdx %d suspended_state %d",
					pPC ? pPC->GetID() : 0, pPC ? pPC->IsRunning() : false, pPC ? pPC->GetRunningQuestState() : NULL,
					(pPC && pPC->GetRunningQuestState()) ? pPC->GetRunningQuestState()->iIndex : -1,
					(pPC && pPC->GetRunningQuestState()) ? pPC->GetRunningQuestState()->suspend_state : -1);
		}
	}

	void CQuestManager::Resume(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state == SUSPEND_STATE_PAUSE)
		{
			pPC->SetSendDoneFlag();
			pPC->GetRunningQuestState()->args = 0;

			if (!RunState(*pPC->GetRunningQuestState()))
			{
				CloseState(*pPC->GetRunningQuestState());
				pPC->EndRunning();
			}
		}
		else
		{
			QuestState* pState = pPC->GetRunningQuestState();
			//cerr << pPC << endl;
			//cerr << pPC->IsRunning() << endl;
			//cerr << pPC->GetRunningQuestState()->suspend_state;
			//cerr << SUSPEND_STATE_WAIT << endl;
			//cerr << "wrong QUEST_WAIT request! : " << pc << endl;
			sys_err("wrong QUEST_WAIT request [runningQS index %d st %d suspend_state %u (isRunning %d)]! : %d", pState?pState->iIndex:0, pState?pState->st:0, pState?pState->suspend_state:0, pState!=NULL, pc);
		}
	}

	void CQuestManager::EnterState(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnEnterState(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::LeaveState(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLeaveState(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::Letter(DWORD pc, DWORD quest_index, int state)
	{
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnLetter(*pPC, quest_index, state);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::LogoutPC(LPCHARACTER ch)
	{
		PC * pPC = GetPC(ch->GetPlayerID());

		if (pPC && pPC->IsRunning())
		{
			CloseState(*pPC->GetRunningQuestState());
			pPC->CancelRunning();
		}

		// Áö¿ì±â Àü¿¡ ·Î±×¾Æ¿ô ÇÑ´Ù.
		Logout(ch->GetPlayerID());

		if (ch == m_pCurrentCharacter)
		{
			m_pCurrentCharacter = NULL;
			m_pCurrentPC = NULL;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	//
	// Quest Event °ü·Ã
	//
	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::Login(unsigned int pc, const char * c_pszQuest)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnLogin(*pPC, c_pszQuest);
		}
		else
		{
			sys_err("QUEST no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	void CQuestManager::Logout(unsigned int pc)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnLogout(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	struct FOnKillEvent
	{
	public:
		FOnKillEvent(unsigned int npc, unsigned int npc_vid, long lMapIndex) : m_npc(npc), m_npc_vid(npc_vid), m_lMapIndex(lMapIndex)
		{
		}

		void operator()(LPCHARACTER pChar)
		{
			if (m_lMapIndex == pChar->GetMapIndex())
			{
				pChar->SetQuestNPCID(m_npc_vid);
				CQuestManager::instance().PartyKill(pChar->GetPlayerID(), m_npc);
			}
		}

	private:
		unsigned int	m_npc;
		unsigned int	m_npc_vid;
		unsigned int	m_lMapIndex;
	};

	void CQuestManager::Kill(unsigned int pc, unsigned int npc)
	{
		PC * pPC;

		//if (test_server || (GetCurrentNPCCharacterPtr() && GetCurrentNPCCharacterPtr()->GetLevel() > 105))
			sys_log(0, "CQuestManager::Kill QUEST_KILL_EVENT (pc=%d, npc=%d)", pc, npc);

#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

#ifdef __QUEST_SAFE__
			SaveCurrentSelection();
#endif

			// call script
			m_mapNPC[npc].OnKill(*pPC);
			if (npc != QUEST_NO_NPC)
				m_mapNPC[QUEST_NO_NPC].OnKill(*pPC);

#ifdef __QUEST_SAFE__
			ResetCurrentSelection();
#endif
			//if (test_server)
				sys_log(!test_server, "End 1111Kill[%u] with CurPC [%u %s] CurNPC [%u]", npc, GetCurrentCharacterPtr()->GetPlayerID(), GetCurrentCharacterPtr()->GetName(), GetCurrentNPCCharacterPtr() ? (DWORD)GetCurrentNPCCharacterPtr()->GetVID() : 0);

			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				m_pCurrentPartyMember = ch;
				LPPARTY pParty = ch->GetParty();
				if (pParty)
				{
					unsigned int npc_vid = 0;
					if (GetCurrentNPCCharacterPtr())
						npc_vid = GetCurrentNPCCharacterPtr()->GetVID();

					//if (test_server)
						sys_log(0, "Start PARTY KiLL[%u] with CurPC [%u %s] VID %u", npc, GetCurrentCharacterPtr()->GetPlayerID(), GetCurrentCharacterPtr()->GetName(), npc_vid);

					FOnKillEvent killEvent(npc, npc_vid, ch->GetMapIndex());
					pParty->ForEachOnlineMember(killEvent);

					//if (test_server)
						sys_log(0, "End PARTY KiLL with CurPC [%u %s]", GetCurrentCharacterPtr()->GetPlayerID(), GetCurrentCharacterPtr()->GetName());
				}
			}
		}
		else
			sys_err("QUEST: no such pc id : %d", pc);

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
		//if (test_server)
			sys_log(!test_server, "End 2222Kill[%u] with CurPC [%u %s] CurNPC [%u]", npc, GetCurrentCharacterPtr()->GetPlayerID(), GetCurrentCharacterPtr()->GetName(), GetCurrentNPCCharacterPtr() ? (DWORD)GetCurrentNPCCharacterPtr()->GetVID() : 0);

	}

	void CQuestManager::PartyKill(unsigned int pc, unsigned int npc)
	{
		PC * pPC;
		//if (test_server)
			sys_log(0, "CQuestManager::PartyKill QUEST_PARTY_KILL_EVENT (pc=%d, npc=%d)", pc, npc);

#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			m_mapNPC[npc].OnPartyKill(*pPC);
			if (npc != QUEST_NO_NPC)
				m_mapNPC[QUEST_NO_NPC].OnPartyKill(*pPC);
		}
#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	bool CQuestManager::ServerTimer(unsigned int npc, unsigned int arg)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		SetServerTimerArg(arg);
		sys_log(!test_server, "XXX ServerTimer Call NPC %p", GetPCForce(0));
		m_pCurrentPC = GetPCForce(0);
		m_pCurrentCharacter = NULL;
		m_pSelectedDungeon = NULL;
		bool bCall = m_mapNPC[npc].OnServerTimer(*m_pCurrentPC);
#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
		return bCall;
	}

	bool CQuestManager::Timer(unsigned int pc, unsigned int npc)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		PC* pPC;

		bool bResult = false;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return false;
			}
			// call script
			return m_mapNPC[npc].OnTimer(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST TIMER_EVENT no such pc id : %d", pc);
			return false;
		}
		//cerr << "QUEST TIMER" << endl;

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
		return bResult;
	}

	void CQuestManager::LevelUp(unsigned int pc)
	{
		PC * pPC;

#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnLevelUp(*pPC);
		}
		else
		{
			sys_err("QUEST LEVELUP_EVENT no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	void CQuestManager::AttrIn(unsigned int pc, LPCHARACTER ch, int attr)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif

		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			// call script
			m_mapNPC[attr+QUEST_ATTR_NPC_START].OnAttrIn(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	void CQuestManager::AttrOut(unsigned int pc, LPCHARACTER ch, int attr)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif

		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			//m_pCurrentCharacter = ch;
			m_pCurrentPartyMember = ch;
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			// call script
			m_mapNPC[attr+QUEST_ATTR_NPC_START].OnAttrOut(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	bool CQuestManager::Target(unsigned int pc, DWORD dwQuestIndex, const char * c_pszTargetName, const char * c_pszVerb)
	{
		PC * pPC;

#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif

		bool bResult = false;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return false;
			}

			bool bRet;
			bResult = m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, dwQuestIndex, c_pszTargetName, c_pszVerb, bRet);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
		return bResult;
	}

	void CQuestManager::QuestInfo(unsigned int pc, unsigned int quest_index)
	{
		PC* pPC;

#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif
		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);

				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));

#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnInfo(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST INFO_EVENT no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	void CQuestManager::QuestButton(unsigned int pc, unsigned int quest_index)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif

		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			// call script
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));
				}
#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return;
			}

			m_mapNPC[QUEST_NO_NPC].OnButton(*pPC, quest_index);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST CLICK_EVENT no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
	}

	bool CQuestManager::TakeItem(unsigned int pc, unsigned int npc, LPITEM item)
	{
#ifdef __QUEST_SAFE__
		SaveCurrentSelection();
#endif

		//m_CurrentNPCRace = npc;
		PC* pPC;

		bool bResult = false;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));
				}

#ifdef __QUEST_SAFE__
				ResetCurrentSelection();
#endif
				return false;
			}
			// call script
			SetCurrentItem(item);
			bResult = m_mapNPC[npc].OnTakeItem(*pPC);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
		}

#ifdef __QUEST_SAFE__
		ResetCurrentSelection();
#endif
		return bResult;
	}

	bool CQuestManager::UseItem(unsigned int pc, LPITEM item, bool bReceiveAll)
	{
		if (test_server)
			sys_log( 0, "questmanager::UseItem Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));
				}

				return false;
			}
			// call script
			SetCurrentItem(item);
			/*
			if (test_server)
			{
				sys_log( 0, "Quest UseItem Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
				itertype(m_mapNPC) it = m_mapNPC.begin();
				itertype(m_mapNPC) end = m_mapNPC.end();
				for( ; it != end ; ++it)
				{
					sys_log( 0, "Quest UseItem : vnum : %d item Vnum : %d", it->first, item->GetOriginalVnum());
				}
			}
			if(test_server)
			sys_log( 0, "questmanager:useItem: mapNPCVnum : %d\n", m_mapNPC[item->GetVnum()].GetVnum());
			*/

			return m_mapNPC[item->GetVnum()].OnUseItem(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	// Speical Item Group¿¡ Á¤ÀÇµÈ Group Use
	bool CQuestManager::SIGUse(unsigned int pc, DWORD sig_vnum, LPITEM item, bool bReceiveAll)
	{
		if (test_server)
			sys_log( 0, "questmanager::SIGUse Start : itemVnum : %d PC : %d", item->GetOriginalVnum(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);

			return m_mapNPC[sig_vnum].OnSIGUse(*pPC, bReceiveAll);
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST USE_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}

	bool CQuestManager::GiveItemToPC(unsigned int pc, LPCHARACTER pkChr)
	{
		if (!pkChr->IsPC())
			return false;

		PC * pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
				return false;

			TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(pc, TARGET_TYPE_VID, pkChr->GetVID());

			if (pInfo)
			{
				bool bRet;

				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return true;
			}
		}

		return false;
	}

	bool CQuestManager::Click(unsigned int pc, LPCHARACTER pkChrTarget)
	{
		PC * pPC = GetPC(pc);

		if (pPC)
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);

				if (ch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));

				return false;
			}

			TargetInfo * pInfo = CTargetManager::instance().GetTargetInfo(pc, TARGET_TYPE_VID, pkChrTarget->GetVID());
			if (test_server)
			{
				sys_log(0, "CQuestManager::Click(pid=%d, npc_name=%s) - target_info(%x)", pc, pkChrTarget->GetName(), pInfo);
			}

			if (pInfo)
			{				
				bool bRet;
				if (m_mapNPC[QUEST_NO_NPC].OnTarget(*pPC, pInfo->dwQuestIndex, pInfo->szTargetName, "click", bRet))
					return bRet;
			}

			DWORD dwCurrentNPCRace = pkChrTarget->GetRaceNum();

			if (pkChrTarget->IsNPC())
			{
				map<unsigned int, NPC>::iterator it = m_mapNPC.find(dwCurrentNPCRace);

				if (it == m_mapNPC.end())
				{
					if(!pkChrTarget->IsMount() 
#ifdef __PET_SYSTEM__
						&& !pkChrTarget->IsPet()
#endif
						)
						if (test_server)
							sys_err("CQuestManager::Click(pid=%d, target_npc_name=%s) - NOT EXIST NPC RACE VNUM[%d]", pc, pkChrTarget->GetName(), dwCurrentNPCRace);
					return false;
				}

				// call script
				if (it->second.HasChat())
				{
					// if have chat, give chat
					if (test_server)
						sys_log(0, "CQuestManager::Click->OnChat");

					if (!it->second.OnChat(*pPC))
					{
						if (test_server)
							sys_log(0, "CQuestManager::Click->OnChat Failed");

						return it->second.OnClick(*pPC);
					}

					return true;
				}
				else
				{
					// else click
					return it->second.OnClick(*pPC);
				}
			}
			return false;
		}
		else
		{
			//cout << "no such pc id : " << pc;
			sys_err("QUEST CLICK_EVENT no such pc id : %d", pc);
			return false;
		}
		//cerr << "QUEST CLICk" << endl;
	}

	void CQuestManager::Mount(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnMount(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}

	void CQuestManager::Unmount(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnUnmount(*pPC);
		}
		else if(test_server)
			sys_err("QUEST no such pc id : %d", pc);
	}
	//µ¶ÀÏ ¼±¹° ±â´É Å×½ºÆ®
	void CQuestManager::ItemInformer(unsigned int pc,unsigned int vnum)
	{
		
		PC* pPC;
		pPC = GetPC(pc);
		
		m_mapNPC[QUEST_NO_NPC].OnItemInformer(*pPC,vnum);
	}

	void CQuestManager::OnSendShout(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnSendShout(*pPC);
		}
	}

	void CQuestManager::OnAddFriend(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnAddFriend(*pPC);
		}
	}

	void CQuestManager::OnSellItem(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnSellItem(*pPC);
		}
	}

	void CQuestManager::OnItemUsed(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnItemUsed(*pPC);
		}
	}

	void CQuestManager::OnMountRiding(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnMountRiding(*pPC);
		}
	}

	void CQuestManager::OnCompleteMissionbook(unsigned int pc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnCompleteMissionbook(*pPC);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////
	// END OF Äù½ºÆ® ÀÌº¥Æ® Ã³¸®
	///////////////////////////////////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////////////////////////////////
	void CQuestManager::LoadStartQuest(const string& quest_name, unsigned int idx)
	{
		string full_name = Locale_GetQuestObjectPath() + "/begin_condition/" + quest_name;
		ifstream inf(full_name.c_str());

		if (inf.is_open())
		{
			sys_log(0, "QUEST loading begin condition for %s", quest_name.c_str());

			istreambuf_iterator<char> ib(inf), ie;
			copy(ib, ie, back_inserter(m_hmQuestStartScript[idx]));
		}
	}

	bool CQuestManager::CanStartQuest(unsigned int quest_index, const PC& pc)
	{
		return CanStartQuest(quest_index);
	}

	bool CQuestManager::CanStartQuest(unsigned int quest_index)
	{
		THashMapQuestStartScript::iterator it;

		if ((it = m_hmQuestStartScript.find(quest_index)) == m_hmQuestStartScript.end())
			return true;
		else
		{
			int x = lua_gettop(L);
			lua_dobuffer(L, &(it->second[0]), it->second.size(), "StartScript");
			int bStart = lua_toboolean(L, -1);
			lua_settop(L, x);
			return bStart != 0;
		}
	}

	bool CQuestManager::CanEndQuestAtState(const string& quest_name, const string& state_name)
	{
		return false;
	}

	void CQuestManager::DisconnectPC(LPCHARACTER ch)
	{
		m_mapPC.erase(ch->GetPlayerID());
	}

	PC * CQuestManager::GetPCForce(unsigned int pc)
	{
		PCMap::iterator it;

		if ((it = m_mapPC.find(pc)) == m_mapPC.end())
		{
			PC * pPC = &m_mapPC[pc];
			pPC->SetID(pc);
			return pPC;
		}

		return &it->second;
	}

	PC * CQuestManager::GetPC(unsigned int pc)
	{
		PCMap::iterator it;

		LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(pc);

		if (!pkChr)
			return NULL;

		if (!pkChr->GetDesc())
			return NULL;

		m_pCurrentPC = GetPCForce(pc);
		m_pCurrentCharacter = pkChr;
		m_pSelectedDungeon = NULL;
		return (m_pCurrentPC);
	}

	void CQuestManager::ClearScript()
	{
		m_strScript.clear();
		m_iCurrentSkin = QUEST_SKIN_NORMAL;
	}

	void CQuestManager::AddScript(const string& str)
	{
		m_strScript+=str;
	}

	void CQuestManager::SendScript()
	{
		if (m_bNoSend)
		{
			m_bNoSend = false;
			ClearScript();
			return;
		}

		if (m_strScript.length() > 0)
		{
			bool bAddEnter = false;
			int iAddEnterPos;

			bool bIsCommand = false;
			for (int i = m_strScript.length() - 1; i >= 0; --i)
			{
				if (!bIsCommand && m_strScript[i] == '}')
					bIsCommand = true;
				else if (bIsCommand && m_strScript[i] == '{')
				{
					bIsCommand = false;
				//	if (i + 7 <= m_strScript.length() && !strncmp(m_strScript.c_str() + i, "{ENTER}", 7))
				//		break;
				}
				else if (!bIsCommand)
				{
					iAddEnterPos = i + 1;
					bAddEnter = true;
					break;
				}
			}

			if (bAddEnter)
			{
				std::string stTempScript = m_strScript;
				stTempScript.erase(iAddEnterPos, stTempScript.length());
				stTempScript += "{ENTER}";
				stTempScript += m_strScript.c_str() + iAddEnterPos;

				m_strScript = stTempScript;
			}
		}

		if (m_strScript=="{DONE}" || m_strScript == "{NEXT}")
		{
			if (m_pCurrentPC && !m_pCurrentPC->GetAndResetDoneFlag() && m_strScript=="{DONE}" && m_iCurrentSkin == QUEST_SKIN_NORMAL && !IsError())
			{
				ClearScript();
				return;
			}
			m_iCurrentSkin = QUEST_SKIN_NOWINDOW;
		}

		m_strScript = LC_QUEST_TEXT(GetCurrentCharacterPtr(), m_strScript.c_str());

		//sys_log(0, "Send Quest Script to %s", GetCurrentCharacterPtr()->GetName());
		//send -_-!
		network::GCOutputPacket<network::GCScriptPacket> packet_script;

		packet_script->set_skin(m_iCurrentSkin);
		packet_script->set_script(m_strScript);
		GetCurrentCharacterPtr()->GetDesc()->Packet(packet_script);

		if (test_server)
			sys_log(0, "m_strScript %s size %d", m_strScript.c_str(), packet_script->ByteSize());

		ClearScript();
	}

	const char* CQuestManager::GetQuestStateName(const string& quest_name, const int state_index)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L,-1))
		{
			sys_err("QUEST wrong quest state file %s.%d", quest_name.c_str(), state_index);
			lua_settop(L,x);
			return "";
		}
		lua_pushnumber(L, state_index);
		lua_gettable(L, -2);

		const char* str = lua_tostring(L, -1);
		if (test_server)
			sys_log(0, "STR: %s", str);
		lua_settop(L, x);
		return str;
	}

	int CQuestManager::GetQuestStateIndex(const string& quest_name, const string& state_name)
	{
		int x = lua_gettop(L);
		lua_getglobal(L, quest_name.c_str());
		if (lua_isnil(L,-1))
		{
			sys_err("QUEST wrong quest state file %s.%s",quest_name.c_str(),state_name.c_str()  );
			lua_settop(L,x);
			return 0;
		}
		lua_pushstring(L, state_name.c_str());
		lua_gettable(L, -2);

		int v = (int)rint(lua_tonumber(L,-1));
		lua_settop(L, x);
		if ( test_server )
			sys_log( 0,"[QUESTMANAGER] GetQuestStateIndex x(%d) v(%d) %s %s", v,x, quest_name.c_str(), state_name.c_str() );
		return v;
	}

	void CQuestManager::SetSkinStyle(int iStyle)
	{
		if (iStyle<0 || iStyle >= QUEST_SKIN_COUNT)
		{
			m_iCurrentSkin = QUEST_SKIN_NORMAL;
		}
		else
			m_iCurrentSkin = iStyle;
	}

	unsigned int CQuestManager::LoadTimerScript(const string& name)
	{
		map<string, unsigned int>::iterator it;
		if ((it = m_mapTimerID.find(name)) != m_mapTimerID.end())
		{
			return it->second;
		}
		else
		{
			unsigned int new_id = UINT_MAX - m_mapTimerID.size();

			m_mapNPC[new_id].Set(new_id, name);
			m_mapTimerID.insert(make_pair(name, new_id));

			return new_id;
		}
	}

	unsigned int CQuestManager::GetCurrentNPCRace()
	{
		return GetCurrentNPCCharacterPtr() ? GetCurrentNPCCharacterPtr()->GetRaceNum() : 0;
	}

	LPITEM CQuestManager::GetCurrentItem()
	{
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestItemPtr() : NULL; 
	}

	void CQuestManager::ClearCurrentItem()
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->ClearQuestItemPtr();
	}

	void CQuestManager::SetCurrentItem(LPITEM item)
	{
		if (GetCurrentCharacterPtr())
			GetCurrentCharacterPtr()->SetQuestItemPtr(item);
	}

	LPCHARACTER CQuestManager::GetCurrentNPCCharacterPtr()
	{ 
		return GetCurrentCharacterPtr() ? GetCurrentCharacterPtr()->GetQuestNPC() : GetCurrentSelectedNPCCharacterPtr();
	}

	const string & CQuestManager::GetCurrentQuestName()
	{
		return GetCurrentPC()->GetCurrentQuestName();
	}

	PC * CQuestManager::GetCurrentRootPC()
	{
		if (m_vecPCStack.empty() || !GetOtherPCBlockRootPC())
			return GetCurrentPC();

		return GetOtherPCBlockRootPC();
	}

	void CQuestManager::CatchFish(unsigned int pc, LPITEM item)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;
			
			if (!item)
				return;
			
			SetCurrentItem(item);
			m_mapNPC[QUEST_NO_NPC].OnCatchFish(*pPC);
		}
		else
		{
			sys_err("QUEST CATCH_FISH_EVENT no such pc id : %d", pc);
		}
	}
	void CQuestManager::MineOre(unsigned int pc, LPITEM item)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			if(!item)
				return;
			
			SetCurrentItem(item);
			m_mapNPC[QUEST_NO_NPC].OnMineOre(*pPC);
		}
		else
		{
			sys_err("QUEST MINE_ORE_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::DungeonComplete(unsigned int pc, BYTE dungeon)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnDungeonComplete(*pPC);
		}
		else
		{
			sys_err("QUEST DUNGEON_COMPLETE no such pc id : %d", pc);
		}
	}
	
	LPDUNGEON CQuestManager:: GetCurrentDungeon()
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();

		if (!ch)
		{
			if (m_pSelectedDungeon)
				return m_pSelectedDungeon;
			return NULL;
		}

		return ch->GetDungeonForce();
	}

	void CQuestManager::DropQuestItem(unsigned int pc, int item)
	{
		PC * pPC;

		if (( pPC = GetPC(pc) ))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			if (!item)
				return;

			// SetCurrentItem(item);
			m_mapNPC[ QUEST_NO_NPC ].OnDropQuestItem(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_DROP_QUEST_ITEM no such pc id : %d", pc);
		}
	}

	void CQuestManager::CollectYangFromMonster(unsigned int pc, int iYangAmount)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;
			
			if (GetCurrentCharacterPtr())
				GetCurrentCharacterPtr()->SetQuestArgument(iYangAmount);

			m_mapNPC[QUEST_NO_NPC].OnCollectYangFromMonster(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_COLLECT_YANG_FROM_MOB_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::CraftGaya(unsigned int pc, int iGayaAmount)
	{
		PC * pPC;

		if (( pPC = GetPC(pc) ))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			if (GetCurrentCharacterPtr())
				GetCurrentCharacterPtr()->SetQuestArgument(iGayaAmount);

			m_mapNPC[ QUEST_NO_NPC ].OnCraftGaya(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_CRAFT_GAYA_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::SpendSouls(unsigned int pc, int iSoulsAmount)
	{
		PC * pPC;

		if (( pPC = GetPC(pc) ))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			if (GetCurrentCharacterPtr())
				GetCurrentCharacterPtr()->SetQuestArgument(iSoulsAmount);

			m_mapNPC[ QUEST_NO_NPC ].OnSpendSouls(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_SPEND_SOULS_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::KillEnemyFraction(unsigned int pc)
	{
		PC * pPC;

		if (( pPC = GetPC(pc) ))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[ QUEST_NO_NPC ].OnKillEnemyFraction(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_KILL_ENEMY_FRACTION_EVENT no such pc id : %d", pc);
		}
	}
	
// XMAS Event
	void CQuestManager::OpenXmasDoor(unsigned int pc)
	{
		PC * pPC;

		if (( pPC = GetPC(pc) ))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[ QUEST_NO_NPC ].OnXMASOpenDoor(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_XMAS_OPEN_DOOR_EVENT no such pc id : %d", pc);
		}
	}

	void CQuestManager::SpendXmasSocks(unsigned int pc, int iAmount)
	{
		PC * pPC;

		if (( pPC = GetPC(pc) ))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			if (GetCurrentCharacterPtr())
				GetCurrentCharacterPtr()->SetQuestArgument(iAmount);

			m_mapNPC[ QUEST_NO_NPC ].OnXMASSantaSocks(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_XMAS_SANTA_SOCKS_EVENT no such pc id : %d", pc);
		}
	}
	
#ifdef __DAMAGE_QUEST_TRIGGER__			
	void CQuestManager::QuestDamage(unsigned int pc, unsigned int npc)
	{
		PC * pPC;

		if ((pPC = GetPC(pc)))
		{			
			if (!CheckQuestLoaded(pPC))
				return;
			
			m_mapNPC[npc].OnQuestDamage(*pPC);
			if (npc != QUEST_NO_NPC)
				m_mapNPC[QUEST_NO_NPC].OnQuestDamage(*pPC);
		}
		else
		{
			sys_err("QUEST QUEST_DAMAGE_EVENT no such pc id : %d npc: %d", pc, npc);
		}
	}
#endif

	void CQuestManager::Dead(unsigned int pc)
	{
		PC * pPC;
	    if ((pPC = GetPC(pc)))
	    {
	        if (!CheckQuestLoaded(pPC))
	            return;

	        m_mapNPC[QUEST_NO_NPC].OnDead(*pPC);
	    }
	    else
	        sys_err("QUEST no such pc id : %d", pc);
	}

#ifdef DUNGEON_REPAIR_TRIGGER
	void CQuestManager::DungeonRepair(unsigned int pc)
	{
		PC * pPC = nullptr;

		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
				return;

			m_mapNPC[QUEST_NO_NPC].OnDungeonRepair(*pPC);
		}
		else
			sys_err("QUEST no such pc id : %d", pc);
	}
#endif

	void CQuestManager::RegisterQuest(const string & stQuestName, unsigned int idx)
	{
		assert(idx > 0);

		itertype(m_hmQuestName) it;

		if ((it = m_hmQuestName.find(stQuestName)) != m_hmQuestName.end())
			return;

		m_hmQuestName.insert(make_pair(stQuestName, idx));
		LoadStartQuest(stQuestName, idx);
		m_mapQuestNameByIndex.insert(make_pair(idx, stQuestName));

		sys_log(!test_server, "QUEST: Register %4u %s", idx, stQuestName.c_str());
	}

	unsigned int CQuestManager::GetQuestIndexByName(const string& name)
	{
		THashMapQuestName::iterator it = m_hmQuestName.find(name);

		if (it == m_hmQuestName.end())
			return 0; // RESERVED

		return it->second;
	}

	const string & CQuestManager::GetQuestNameByIndex(unsigned int idx)
	{
		itertype(m_mapQuestNameByIndex) it;

		if ((it = m_mapQuestNameByIndex.find(idx)) == m_mapQuestNameByIndex.end())
		{
			sys_err("cannot find quest name by index %u", idx);
			assert(!"cannot find quest name by index");

			static std::string st = "";
			return st;
		}

		return it->second;
	}

	void CQuestManager::SendEventFlagList(LPCHARACTER ch)
	{
		itertype(m_mapEventFlag) it;
		for (it = m_mapEventFlag.begin(); it != m_mapEventFlag.end(); ++it)
		{
			const string& flagname = it->first;
			int value = it->second;

			if (!test_server && value == 1 && flagname == "valentine_drop")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 800", flagname.c_str(), value);
			else if (!test_server && value == 1 && flagname == "newyear_wonso")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 500", flagname.c_str(), value);
			else if (!test_server && value == 1 && flagname == "newyear_fire")
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d prob 1000", flagname.c_str(), value);
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d", flagname.c_str(), value);
		}
	}

	void CQuestManager::RequestSetEventFlag(const string& name, int value, bool isAdd)
	{
		network::GDOutputPacket<network::GDSetEventFlagPacket> p;
		p->set_flag_name(name);
		p->set_value(value);
		p->set_is_add(isAdd);
		db_clientdesc->DBPacket(p);
	}

	void CQuestManager::SetEventFlag(const string& name, int value, bool bIsAdd)
	{
		static const char*	DROPEVENT_CHARTONE_NAME		= "drop_char_stone";
		static const int	DROPEVENT_CHARTONE_NAME_LEN = strlen(DROPEVENT_CHARTONE_NAME);

		int prev_value = m_mapEventFlag[name];

		sys_log(0, "QUEST eventflag %s %d prev_value %d", name.c_str(), value, m_mapEventFlag[name]);
		if (bIsAdd)
		{
			m_mapEventFlag[name] += value;
			value = m_mapEventFlag[name];
		}
		else
			m_mapEventFlag[name] = value;

		if (name == "mob_item")
		{
			CHARACTER_MANAGER::instance().SetMobItemRate(value);
		}
		else if (name == "mob_dam")
		{
			CHARACTER_MANAGER::instance().SetMobDamageRate(value);
		}
		else if (name == "mob_gold")
		{
			CHARACTER_MANAGER::instance().SetMobGoldAmountRate(value);
		}
		else if (name == "mob_gold_pct")
		{
			CHARACTER_MANAGER::instance().SetMobGoldDropRate(value);
		}
		else if (name == "user_dam")
		{
			sys_log(0, "SetUserDamRate (user_dam) value %d", value);
			CHARACTER_MANAGER::instance().SetUserDamageRate(value);
		}
		else if (name == "user_dam_buyer")
		{
			sys_log(0, "SetUserDamRatePremium (user_dam_buyer) value %d", value);
			CHARACTER_MANAGER::instance().SetUserDamageRatePremium(value);
		}
		else if (name == "mob_exp")
		{
			CHARACTER_MANAGER::instance().SetMobExpRate(value);
		}
		else if (name == "mob_item_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobItemRatePremium(value);
		}
		else if (name == "mob_exp_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobExpRatePremium(value);
		}
		else if (name == "mob_gold_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobGoldAmountRatePremium(value);
		}
		else if (name == "mob_gold_pct_buyer")
		{
			CHARACTER_MANAGER::instance().SetMobGoldDropRatePremium(value);
		}
		else if (name == "crcdisconnect")
		{
			DESC_MANAGER::instance().SetDisconnectInvalidCRCMode(value != 0);
		}
		else if (!name.compare(0,5,"xmas_"))
		{
			xmas::ProcessEventFlag(name, prev_value, value);
		}
		else if (name == "newyear_boom")
		{
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();

				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "newyear_boom %d", value);
			}
		}
		else if ( name == "eclipse" )
		{
			std::string mode("");

			if ( value == 1 )
			{
				mode = "dark";
			}
			else
			{
				mode = "light";
			}
			
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode %s", mode.c_str());
			}
		}
		else if (name == "event_anniversary_day")
		{
			if (!GetEventFlag("event_anniversary_running"))
				return;

			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
			std::string sendMsg = "ANGELDEMONEVENTSTATUS";
			for (BYTE i = 0; i < 7; ++i)
				sendMsg += " " + std::to_string(GetEventFlag("event_anniversary_winner_d" + std::to_string(i)));

			int bonusEnd = GetEventFlag("event_anniversary_boniend_d" + std::to_string(prev_value)); 
			int winner = GetEventFlag("event_anniversary_winner_d" + std::to_string(prev_value));
			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, sendMsg.c_str());
				if (test_server)
					ch->tchat("fraction: %d winner: %d remainingBonusTime: %d", ch->GetQuestFlag("anniversary_event.selected_fraction"), winner, bonusEnd - get_global_time());
				if (bonusEnd > get_global_time() && winner && winner == ch->GetQuestFlag("anniversary_event.selected_fraction"))
				{
					if (prev_value == 0)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_BOSS);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_BOSS, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
					else if (prev_value == 1)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_DUNGEON_CD);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_DUNGEON_CD, 25, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
					else if (prev_value == 2)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_METIN);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_METIN, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
					else if (prev_value == 3)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_BIOLOG_CD);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_BIOLOG_CD, 50, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
					else if (prev_value == 4)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_MONSTER);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_MONSTER, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
					else if (prev_value == 5)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_BONUS_UPGRADE_CHANCE);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_BONUS_UPGRADE_CHANCE, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
					else if (prev_value == 6)
					{
						CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_DOUBLE_ITEM_DROP_BONUS);
						if (!selAffect)
							ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_DOUBLE_ITEM_DROP_BONUS, 5, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
				}
			}
		}
		else if (name == "event_anniversary_weekangel_cnt" || name == "event_anniversary_weekdemon_cnt")
		{
			if (prev_value == value)
				return;
			if (!GetEventFlag("event_anniversary_running") || !GetEventFlag("event_anniversary_week"))
				return;

			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "UpdateAnniversaryWeek %d %d", GetEventFlag("event_anniversary_weekangel_cnt"), GetEventFlag("event_anniversary_weekdemon_cnt"));
			}
		}
		else if (name == "event_anniversary_week")
		{
			if (!GetEventFlag("event_anniversary_running") || !prev_value)
				return;

			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			int bonusEnd = GetEventFlag("event_anniversary_weekboniend");
			int winner = GetEventFlag("event_anniversary_weekwinner");
			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				if (test_server)
					ch->tchat("fraction: %d winner: %d remainingBonusTime: %d", ch->GetQuestFlag("anniversary_event.selected_fraction"), winner, bonusEnd - get_global_time());
				if (bonusEnd > get_global_time() && winner && winner == ch->GetQuestFlag("anniversary_event.selected_fraction"))
				{
					CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_EXPBONUS);
					if (!selAffect)
					{
						ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_EXPBONUS, 100, AFF_NONE, bonusEnd - get_global_time(), 0, false);
						ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_ITEMBONUS, 50, AFF_NONE, bonusEnd - get_global_time(), 0, false);
						ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_GOLDBONUS, 50, AFF_NONE, bonusEnd - get_global_time(), 0, false);
						ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_BOSS, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
						ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_METIN, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
						ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_MONSTER, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
					}
				}
			}
		}
		else if (name == "day")
		{
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;
				if (value)
				{
					// ¹ã
					ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
				}
				else
				{
					// ³·
					ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode light");
				}
			}

			if (value && !prev_value)
			{
				// ¾øÀ¸¸é ¸¸µé¾îÁØ´Ù
				struct SNPCSellFireworkPosition
				{
					long lMapIndex;
					int x;
					int y;
				} positions[] = {
					{ HOME_MAP_INDEX_RED_1,	615,	618 },
					{ HOME_MAP_INDEX_RED_2,	500,	625 },
					{ HOME_MAP_INDEX_YELLOW_1,	598,	665 },
					{ HOME_MAP_INDEX_YELLOW_2,	476,	360 },
					{ HOME_MAP_INDEX_BLUE_1,	318,	629 },
					{ HOME_MAP_INDEX_BLUE_2,	478,	375 },
					{	0,	0,	0   },
				};

				SNPCSellFireworkPosition* p = positions;
				while (p->lMapIndex)
				{
					if (map_allow_find(p->lMapIndex))
					{
						PIXEL_POSITION posBase;
						if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(p->lMapIndex, posBase))
						{
							sys_err("cannot get map base position %d", p->lMapIndex);
							++p;
							continue;
						}

						CHARACTER_MANAGER::instance().SpawnMob(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM, p->lMapIndex, posBase.x + p->x * 100, posBase.y + p->y * 100, 0, false, -1);
					}
					p++;
				}
			}
			else if (!value && prev_value)
			{
				// ÀÖÀ¸¸é Áö¿öÁØ´Ù
				CharacterVectorInteractor i;

				if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(xmas::MOB_XMAS_FIRWORK_SELLER_VNUM, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end()) {
						M2_DESTROY_CHARACTER(*it++);
					}
				}
			}
		}
		else if (name == "pre_event_hc")
		{
			const DWORD EventNPC = 20090;

			struct SEventNPCPosition
			{
				long lMapIndex;
				int x;
				int y;
			} positions[] = {
				{ HOME_MAP_INDEX_RED_2, 588, 617 },
				{ HOME_MAP_INDEX_YELLOW_2, 397, 250 },
				{ HOME_MAP_INDEX_BLUE_2, 567, 426 },
				{ 0, 0, 0 },
			};

			if (value && !prev_value)
			{
				SEventNPCPosition* pPosition = positions;

				while (pPosition->lMapIndex)
				{
					if (map_allow_find(pPosition->lMapIndex))
					{
						PIXEL_POSITION pos;

						if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(pPosition->lMapIndex, pos))
						{
							sys_err("cannot get map base position %d", pPosition->lMapIndex);
							++pPosition;
							continue;
						}

						CHARACTER_MANAGER::instance().SpawnMob(EventNPC, pPosition->lMapIndex, pos.x+pPosition->x*100, pos.y+pPosition->y*100, 0, false, -1);
					}
					pPosition++;
				}
			}
			else if (!value && prev_value)
			{
				CharacterVectorInteractor i;

				if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(EventNPC, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end())
					{
						LPCHARACTER ch = *it++;

						switch (ch->GetMapIndex())
						{
							case 3:
							case 23:
							case 43:
								M2_DESTROY_CHARACTER(ch);
								break;
						}
					}
				}
			}
		}
		else if (name.compare(0, DROPEVENT_CHARTONE_NAME_LEN, DROPEVENT_CHARTONE_NAME)== 0)
		{
			DropEvent_CharStone_SetValue(name, value);
		}
		else if (name.compare(0, strlen("refine_box"), "refine_box")== 0)
		{
			DropEvent_RefineBox_SetValue(name, value);
		}
		else if (name == "gold_drop_limit_time")
		{
			g_GoldDropTimeLimitValue = value * 1000;
		}
		else if (name == "anniversary_disable_pvp_boss")
		{
			if (value == prev_value)
				return;

#ifdef ENABLE_BLOCK_PKMODE
			FSetPKModeInMap f;
			f.pkMode = PK_MODE_PROTECT;
			f.mode = true;
			if (!value)
				f.mode = false;

			for (int i = 10; i <= 11; ++i)
			{
				LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(i);

				if (pSecMap)
					pSecMap->for_each(f);
			}
#endif
		}
		else if (name == "enable_bosshunt_event")
		{
			if (value == prev_value) // prevent problems from event messages from other cores
				return;

			if (bossHuntSpawnEvent)
				event_cancel(&bossHuntSpawnEvent);

			ClearBossHuntMobs(); // before bossHuntEvent cancel, cous info needed for clearing boss
			if (bossHuntEvent)
				event_cancel(&bossHuntEvent);

			if (value)
			{
				bossHuntEvent = event_create(bosshunt_event, AllocEventInfo<bosshunt_event_info>(), PASSES_PER_SEC(MAX(1, GetEventFlag("bosshunt_event_end") - get_global_time())));
				bosshunt_event_info* info = bossHuntEvent ? dynamic_cast<bosshunt_event_info*>(bossHuntEvent->info) : NULL;
				if (info)
					info->setEnd = false;

				// the first mob will respawn in 60 mins anyway, no need to check every time till first spawn
				bossHuntSpawnEvent = event_create(bosshunt_event_respawn_timer, AllocEventInfo<bosshunt_event_respawn_info>(), test_server ? PASSES_PER_SEC(5) : PASSES_PER_SEC(3600));

				// 2 boss / map
				SpawnBossHuntMobs();
				SpawnBossHuntMobs();

				SendNotice("<BossHunt> The event is now active.");
#ifdef ENABLE_BLOCK_PKMODE
				for (int i = 10; i <= 13; ++i)
				{
					LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(i);

					FSetPKModeInMap f;
					f.pkMode = PK_MODE_PROTECT;
					f.mode = true;

					if (pSecMap)
						pSecMap->for_each(f);
				}
#endif
			}
			else
#ifdef ENABLE_BLOCK_PKMODE
			{
				SendNotice("<BossHunt> The event has been ended.");

				for (int i = 10; i <= 13; ++i)
				{
					LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(i);

					FSetPKModeInMap f;
					f.pkMode = PK_MODE_PEACE;
					f.mode = false;

					if (pSecMap)
						pSecMap->for_each(f);
				}
			}
#else
				SendNotice("<BossHunt> The event has been ended.");
#endif

			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
			std::string currHuntID = "event_boss_hunt.points" + std::to_string(GetEventFlag("bosshunt_event_id"));
			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();
				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "BossHuntPoints %d", value == 0 ? -1 : ch->GetQuestFlag(currHuntID));
			}
		}
		else if (name == "bosshunt_event_end")
		{
			if (bossHuntEvent)
				event_cancel(&bossHuntEvent);

			value -= get_global_time();

			bossHuntEvent = event_create(bosshunt_event, AllocEventInfo<bosshunt_event_info>(), PASSES_PER_SEC(MAX(1, value)));
			bosshunt_event_info* info = bossHuntEvent ? dynamic_cast<bosshunt_event_info*>(bossHuntEvent->info) : NULL;
			if (info)
				info->setEnd = true;
		}
		else if (name == "new_xmas_event")
		{
			// 20126 new»êÅ¸.
			static DWORD new_santa = 20126;
			if (value != 0)
			{
				CharacterVectorInteractor i;
				bool map1_santa_exist = false;
				bool map21_santa_exist = false;
				bool map41_santa_exist = false;
				
				if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(new_santa, i))
				{
					CharacterVectorInteractor::iterator it = i.begin();

					while (it != i.end())
					{
						LPCHARACTER tch = *(it++);

						if (tch->GetMapIndex() == HOME_MAP_INDEX_RED_1)
						{
							map1_santa_exist = true;
						}
						else if (tch->GetMapIndex() == HOME_MAP_INDEX_YELLOW_1)
						{
							map21_santa_exist = true;
						}
						else if (tch->GetMapIndex() == HOME_MAP_INDEX_BLUE_1)
						{
							map41_santa_exist = true;
						}
					}
				}

				if (map_allow_find(HOME_MAP_INDEX_RED_1) && !map1_santa_exist)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(HOME_MAP_INDEX_RED_1);
					CHARACTER_MANAGER::instance().SpawnMob(new_santa, HOME_MAP_INDEX_RED_1, pkSectreeMap->m_setting.iBaseX + 60800, pkSectreeMap->m_setting.iBaseY + 61700, 0, false, 90, true);
				}
				if (map_allow_find(HOME_MAP_INDEX_YELLOW_1) && !map21_santa_exist)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(HOME_MAP_INDEX_YELLOW_1);
					CHARACTER_MANAGER::instance().SpawnMob(new_santa, HOME_MAP_INDEX_YELLOW_1, pkSectreeMap->m_setting.iBaseX + 59600, pkSectreeMap->m_setting.iBaseY + 61000, 0, false, 110, true);
				}
				if (map_allow_find(HOME_MAP_INDEX_BLUE_1) && !map41_santa_exist)
				{
					LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(HOME_MAP_INDEX_BLUE_1);
					CHARACTER_MANAGER::instance().SpawnMob(new_santa, HOME_MAP_INDEX_BLUE_1, pkSectreeMap->m_setting.iBaseX + 35700, pkSectreeMap->m_setting.iBaseY + 74300, 0, false, 140, true);
				}
			}
			else
			{
				CharacterVectorInteractor i;
				CHARACTER_MANAGER::instance().GetCharactersByRaceNum(new_santa, i);
				
				for (CharacterVectorInteractor::iterator it = i.begin(); it != i.end(); it++)
				{
					M2_DESTROY_CHARACTER(*it);
				}
			}
		}
	}

	int	CQuestManager::GetEventFlag(const string& name)
	{
		map<string,int>::iterator it = m_mapEventFlag.find(name);

		if (it == m_mapEventFlag.end())
			return 0;

		return it->second;
	}

	void CQuestManager::BroadcastEventFlagOnLogin(LPCHARACTER ch)
	{
		int iEventFlagValue;

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("xmas_snow")))
		{	
			if (iEventFlagValue)
				ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_snow %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("xmas_boom")))
		{
			if (iEventFlagValue)
				ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_boom %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("xmas_tree")))
		{
			if (iEventFlagValue)
				ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_tree %d", iEventFlagValue);
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("day")))
		{
			if (iEventFlagValue)
				ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
		}

		if ((iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("newyear_boom")))
		{
			if (iEventFlagValue)
				ch->ChatPacket(CHAT_TYPE_COMMAND, "newyear_boom %d", iEventFlagValue);
		}

		if ( (iEventFlagValue = quest::CQuestManager::instance().GetEventFlag("eclipse")) )
		{
			std::string mode;

			if ( iEventFlagValue == 1 ) mode = "dark";
			else mode = "light";

			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode %s", mode.c_str());
		}
	}

	void CQuestManager::Reload()
	{
		lua_close(L);
		m_mapNPC.clear();
		m_mapNPCNameID.clear();
		m_hmQuestName.clear();
		m_mapTimerID.clear();
		m_hmQuestStartScript.clear();
		m_mapEventName.clear();
		L = NULL;
		Initialize();

		for (itertype(m_registeredNPCVnum) it = m_registeredNPCVnum.begin(); it != m_registeredNPCVnum.end(); ++it)
		{
			char buf[256];
			DWORD dwVnum = *it;
			snprintf(buf, sizeof(buf), "%u", dwVnum);
			m_mapNPC[dwVnum].Set(dwVnum, buf);
		}
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, DWORD quest_index, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		return ExecuteQuestScript(pc, CQuestManager::instance().GetQuestNameByIndex(quest_index), state, code, code_size, pChatScripts, bUseCache);
	}

	bool CQuestManager::ExecuteQuestScript(PC& pc, const string& quest_name, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts, bool bUseCache)
	{
		/*if (pc.IsRunning() && pc.GetRunningQuestState()->iIndex && pc.GetRunningQuestState()->iIndex != CQuestManager::instance().GetQuestIndexByName(quest_name))
		{
			sys_log(0, "ExecuteQuestScript(%u, %s:%u) already running state quest %d", pc.GetID(), quest_name.c_str(), state, pc.GetRunningQuestState()->iIndex);
			return false;
		}*/

		// ½ÇÇà°ø°£À» »ý¼º
		QuestState qs = CQuestManager::instance().OpenState(quest_name, state);
		if (pChatScripts)
			qs.chat_scripts.swap(*pChatScripts);

		// ÄÚµå¸¦ ÀÐ¾îµéÀÓ
		if (bUseCache)
		{
			lua_getglobal(qs.co, "__codecache");
			// stack : __codecache
			lua_pushnumber(qs.co, (long)code);
			// stack : __codecache (codeptr)
			lua_rawget(qs.co, -2);
			// stack : __codecache (compiled-code)
			if (lua_isnil(qs.co, -1))
			{
				// cache miss

				// load code to lua,
				// save it to cache
				// and only function remain in stack
				lua_pop(qs.co, 1);
				// stack : __codecache
				luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());
				// stack : __codecache (compiled-code)
				lua_pushnumber(qs.co, (long)code);
				// stack : __codecache (compiled-code) (codeptr)
				lua_pushvalue(qs.co, -2);
				// stack : __codecache (compiled-code) (codeptr) (compiled_code)
				lua_rawset(qs.co, -4);
				// stack : __codecache (compiled-code)
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
			else
			{
				// cache hit
				lua_remove(qs.co, -2);
				// stack : (compiled-code)
			}
		}
		else
			luaL_loadbuffer(qs.co, code, code_size, quest_name.c_str());

		// ÇÃ·¹ÀÌ¾î¿Í ¿¬°á
		pc.SetQuest(quest_name, qs);

		// ½ÇÇà
		QuestState& rqs = *pc.GetRunningQuestState();
		if (!CQuestManager::instance().RunState(rqs))
		{
			if (test_server)
				sys_log(0, "RunState FALSE (%u, index %d st %d suspend_state %d)", pc.GetID(), rqs.iIndex, rqs.st, rqs.suspend_state);
			CQuestManager::instance().CloseState(rqs);
			pc.EndRunning();
			return false;
		}
		if (test_server)
			sys_log(0, "RunState (%u, index %d st %d suspend_state %d)", pc.GetID(), rqs.iIndex, rqs.st, rqs.suspend_state);

		return true;
	}

	void CQuestManager::RegisterNPCVnum(DWORD dwVnum)
	{
		if (m_registeredNPCVnum.find(dwVnum) != m_registeredNPCVnum.end())
			return;

		m_registeredNPCVnum.insert(dwVnum);

		char buf[256];
		DIR* dir;

		snprintf(buf, sizeof(buf), "%s/%u", Locale_GetQuestObjectPath().c_str(), dwVnum);
		sys_log(!test_server, "%s", buf);

		if ((dir = opendir(buf)))
		{
			closedir(dir);
			snprintf(buf, sizeof(buf), "%u", dwVnum);
			sys_log(!test_server, "%s", buf);

			m_mapNPC[dwVnum].Set(dwVnum, buf);
		}
	}

#ifdef __QUEST_SAFE__
	void CQuestManager::SaveCurrentSelection()
	{
		sys_log(!test_server, "SaveCurrentSelection level %d VID %u", m_pSelectedDungeon_Save.size() + 1, GetCurrentNPCCharacterPtr() ? (DWORD)GetCurrentNPCCharacterPtr()->GetVID() : 0);

		m_pSelectedDungeon_Save.push_back(m_pSelectedDungeon);
		m_pCurrentCharacter_Save.push_back(m_pCurrentCharacter);
		m_dwCurrentNPCCharacterVID_Save.push_back(GetCurrentNPCCharacterPtr() ? GetCurrentNPCCharacterPtr()->GetVID() : 0);
		m_pCurrentPartyMember_Save.push_back(m_pCurrentPartyMember);
		m_pCurrentPC_Save.push_back(m_pCurrentPC);
	}

	void CQuestManager::ResetCurrentSelection()
	{
		if (m_pSelectedDungeon_Save.size() == 0)
		{
			sys_err("ResetCurrentSelection: there is no data to reset selection");
			return;
		}

		sys_log(!test_server, "ResetCurrentSelection level %d lastVID %u", m_pSelectedDungeon_Save.size(), m_dwCurrentNPCCharacterVID_Save.back());

		m_pSelectedDungeon = m_pSelectedDungeon_Save.back();
		m_pCurrentCharacter = m_pCurrentCharacter_Save.back();
		LPCHARACTER pkNPCChar = m_dwCurrentNPCCharacterVID_Save.back() ? CHARACTER_MANAGER::instance().Find(m_dwCurrentNPCCharacterVID_Save.back()) : NULL;
		if (m_pCurrentCharacter)
			m_pCurrentCharacter->SetQuestNPCID(pkNPCChar ? pkNPCChar->GetVID() : 0);
		else
			m_pCurrentNPCCharacter = pkNPCChar;
		m_pCurrentPartyMember = m_pCurrentPartyMember_Save.back();
		m_pCurrentPC = m_pCurrentPC_Save.back();

		m_pSelectedDungeon_Save.pop_back();
		m_pCurrentCharacter_Save.pop_back();
		m_dwCurrentNPCCharacterVID_Save.pop_back();
		m_pCurrentPartyMember_Save.pop_back();
		m_pCurrentPC_Save.pop_back();
	}
#endif

	void CQuestManager::WriteRunningStateToSyserr()
	{
		const char * state_name = GetQuestStateName(GetCurrentQuestName(), GetCurrentState()->st);

		string event_index_name = "";
		for (itertype(m_mapEventName) it = m_mapEventName.begin(); it != m_mapEventName.end(); ++it)
		{
			if (it->second == m_iRunningEventIndex)
			{
				event_index_name = it->first;
				break;
			}
		}

		sys_err("LUA_ERROR: quest %s.%s %s", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str() );
		if (GetCurrentCharacterPtr() && test_server)
			GetCurrentCharacterPtr()->ChatPacket(CHAT_TYPE_PARTY, "LUA_ERROR: quest %s.%s %s", GetCurrentQuestName().c_str(), state_name, event_index_name.c_str() );
	}

#ifndef __WIN32__
	void CQuestManager::QuestError(const char* func, int line, const char* fmt, ...)
	{
		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		_sys_err(func, line, "QUEST[%s] %s [%s:%d]", GetCurrentQuestName().c_str(), szMsg, func, line);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d] qname %s", func, line, GetCurrentQuestName().c_str());
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#else
	void CQuestManager::QuestError(const char* func, int line, const char* fmt, ...)
	{

		char szMsg[4096];
		va_list args;

		va_start(args, fmt);
		vsnprintf(szMsg, sizeof(szMsg), fmt, args);
		va_end(args);

		_sys_err(func, line, "QUEST[%s] %s [%s:%d]", GetCurrentQuestName().c_str(), szMsg, func, line);
		if (test_server)
		{
			LPCHARACTER ch = GetCurrentCharacterPtr();
			if (ch)
			{
				ch->ChatPacket(CHAT_TYPE_PARTY, "error occurred on [%s:%d] qname %s", func, line, GetCurrentQuestName().c_str());
				ch->ChatPacket(CHAT_TYPE_PARTY, "%s", szMsg);
			}
		}
	}
#endif

	void CQuestManager::AddServerTimer(const std::string& name, DWORD arg, LPEVENT event)
	{
		sys_log(!test_server, "XXX AddServerTimer %s %d %p", name.c_str(), arg, get_pointer(event));
		if (m_mapServerTimer.find(make_pair(name, arg)) != m_mapServerTimer.end())
		{
			sys_err("already registered server timer name:%s arg:%u", name.c_str(), arg);
			return;
		}
		m_mapServerTimer.insert(make_pair(make_pair(name, arg), event));
	}

	void CQuestManager::ClearServerTimerNotCancel(const std::string& name, DWORD arg)
	{
		m_mapServerTimer.erase(make_pair(name, arg));
	}

	void CQuestManager::ClearServerTimer(const std::string& name, DWORD arg)
	{
		itertype(m_mapServerTimer) it = m_mapServerTimer.find(make_pair(name, arg));
		if (it != m_mapServerTimer.end())
		{
			LPEVENT event = it->second;
			event_cancel(&event);
			m_mapServerTimer.erase(it);
		}
	}

	void CQuestManager::CancelServerTimers(DWORD arg)
	{
		vector<pair<string, DWORD> > tmpEraseKeys;
		itertype(m_mapServerTimer) it = m_mapServerTimer.begin();
		for ( ; it != m_mapServerTimer.end(); ++it) {
			if (it->first.second == arg) {
				LPEVENT event = it->second;
				event_cancel(&event);
				tmpEraseKeys.push_back(it->first);
			}
		}

		for (auto key : tmpEraseKeys)
			m_mapServerTimer.erase(key);
	}

	void CQuestManager::SetServerTimerArg(DWORD dwArg)
	{
		m_dwServerTimerArg = dwArg;
	}

	DWORD CQuestManager::GetServerTimerArg()
	{
		return m_dwServerTimerArg;
	}

	void CQuestManager::SelectDungeon(LPDUNGEON pDungeon)
	{
		m_pSelectedDungeon = pDungeon;
	}
	
	bool CQuestManager::PickupItem(unsigned int pc, LPITEM item)
	{
		sys_log(!test_server, "questmanager::PickupItem Start : item %s itemVnum : %d itemCount: %d PC : %d", item->GetName(), item->GetOriginalVnum(), item->GetCount(), pc);
		PC* pPC;
		if ((pPC = GetPC(pc)))
		{
			if (!CheckQuestLoaded(pPC))
			{
				LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc);
				if (ch)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Äù½ºÆ®¸¦ ·ÎµåÇÏ´Â ÁßÀÔ´Ï´Ù. Àá½Ã¸¸ ±â´Ù·Á ÁÖ½Ê½Ã¿À."));
				}
				return false;
			}
			// call script
			SetCurrentItem(item);

			return m_mapNPC[item->GetVnum()].OnPickupItem(*pPC);
		}
		else
		{
			sys_err("QUEST PICK_ITEM_EVENT no such pc id : %d", pc);
			return false;
		}
	}
	void CQuestManager::BeginOtherPCBlock(DWORD pid)
	{
		LPCHARACTER ch = GetCurrentCharacterPtr();
		if (NULL == ch)
		{
			sys_err("NULL?");
			return;
		}
		/*
		# 1. current pid = pid0 <- It will be m_pOtherPCBlockRootPC.
		begin_other_pc_block(pid1)
			# 2. current pid = pid1
			begin_other_pc_block(pid2)
				# 3. current_pid = pid2
			end_other_pc_block()
		end_other_pc_block()
		*/
		// when begin_other_pc_block(pid1)
		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = GetCurrentPC();
		}
		m_vecPCStack.push_back(GetCurrentCharacterPtr()->GetPlayerID());
		GetPC(pid);
	}

	void CQuestManager::EndOtherPCBlock()
	{
		if (m_vecPCStack.size() == 0)
		{
			sys_err("m_vecPCStack is alread empty. CurrentQuest{Name(%s), State(%s)}", GetCurrentQuestName().c_str(), GetCurrentState()->_title.c_str());
			return;
		}
		DWORD pc = m_vecPCStack.back();
		m_vecPCStack.pop_back();
		GetPC(pc);

		if (m_vecPCStack.empty())
		{
			m_pOtherPCBlockRootPC = NULL;
		}
	}

	bool CQuestManager::IsInOtherPCBlock()
	{
		return !m_vecPCStack.empty();
	}

	PC*	CQuestManager::GetOtherPCBlockRootPC()
	{
		return m_pOtherPCBlockRootPC;
	}

	bool CQuestManager::IsSuspended(unsigned int pc)
	{
		PC * pPC;
		if ((pPC = GetPC(pc)) && pPC->IsRunning() && pPC->GetRunningQuestState()->suspend_state != SUSPEND_STATE_NONE)
			return true;

		return false;
	}
}

