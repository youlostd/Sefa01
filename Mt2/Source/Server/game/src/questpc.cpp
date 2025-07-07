﻿#include "stdafx.h"
#include "constants.h"
#include "questmanager.h"
#include "packet.h"
#include "buffer_manager.h"
#include "char.h"
#include "desc_client.h"
#include "questevent.h"

extern int test_server;
extern bool pvp_server;

const std::string biologNames[] = {
	"collect_quest_lv30.duration",
	"collect_quest_lv40.duration",
	"collect_quest_lv50.duration",
	"collect_quest_lv60.duration",
	"collect_quest_lv70.duration",
	"collect_quest_lv80.duration",
	"collect_quest_lv85.duration",
	"collect_quest_lv90.duration",
	"collect_quest_lv92.duration",
	"collect_quest_lv94.duration",
	"collect_quest_lv108.duration",
	"collect_quest_lv112.duration",
};

const std::string dungeonNames[] = {
	"orkmaze.next_entry",
	"spider_dungeon.next_entry",
	"devilcatacomb_zone.next_entry",
	"dragon_dungeon.next_entry",
	"snow_dungeon.next_entry",
	"flame_dungeon.next_entry",
	"hydra_dungeon.next_entry",
	"enchanted_forest_zone.next_entry",
	"crystal_dungeon.next_entry",
	"meleylair_zone.next_entry",
	"thraduils.next_entry",
	// "zodiac.placeholder",
	"apedungeon.next_entry"
};
namespace quest
{
	PC::PC() :
		m_bIsGivenReward(false),
		m_bShouldSendDone(false),
		m_dwID(0),
		m_RunningQuestState(0),
		m_iSendToClient(0),
		m_bLoaded(false),
		m_iLastState(0),
		m_dwWaitConfirmFromPID(0),
		m_bConfirmWait(false)
	{
	}

	PC::~PC()
	{
		Destroy();
	}

	void PC::Destroy()
	{
		ClearTimer();
	}

	void PC::SetID(DWORD dwID)
	{
		m_dwID = dwID;
		m_bShouldSendDone = false;
	}

	const string & PC::GetCurrentQuestName() const
	{
		return m_stCurQuest;
	}

	int	PC::GetCurrentQuestIndex()
	{
		return CQuestManager::instance().GetQuestIndexByName(GetCurrentQuestName());
	}

	void PC::SetFlag(const string& name, int value, bool bSkipSave)
	{
		sys_log(!test_server, "QUEST Setting flag %s %d", name.c_str(),value);

		if (value == 0)
		{
			DeleteFlag(name);
			return;
		}

		LPCHARACTER currPC = CHARACTER_MANAGER::instance().FindByPID(m_dwID);
		if (currPC && currPC->GetPoint(POINT_LOWER_BIOLOG_CD))
		{
			for (size_t it = 0; it < sizeof(biologNames) / sizeof(std::string); ++it)
				if (name == biologNames[it])
				{
					value = ((value - get_global_time()) * MINMAX(0, 100 - currPC->GetPoint(POINT_LOWER_BIOLOG_CD), 100)) / 100 + get_global_time();
					break;
				}
		}
		else if (currPC && currPC->GetPoint(POINT_LOWER_DUNGEON_CD))
		{
			for (size_t it = 0; it < sizeof(dungeonNames) / sizeof(std::string); ++it)
				if (name == dungeonNames[it])
				{
					value = ((value - get_global_time()) * MINMAX(0, 100 - currPC->GetPoint(POINT_LOWER_DUNGEON_CD), 100)) / 100 + get_global_time();
					break;
				}
		}

		TFlagMap::iterator it = m_FlagMap.find(name);

		if (it == m_FlagMap.end())
			m_FlagMap.insert(make_pair(name, value));
		else if (it->second != value)
			it->second = value;
		else
			bSkipSave = true;
		
		// if (test_server  || !bSkipSave)
		// 	sys_log(1, "quest::PC::SetFlag owner %u name %s value %d skipSave %d", GetID(), name.c_str(), value, bSkipSave);

		if (!bSkipSave)
			SaveFlag(name, value);
	}

	bool PC::DeleteFlag(const string & name)
	{
		TFlagMap::iterator it = m_FlagMap.find(name);

		if (it != m_FlagMap.end())
		{
			m_FlagMap.erase(it);
			SaveFlag(name, 0);
			return true;
		}

		return false;
	}

	int PC::DeleteFlagsByQuest(const string & questname)
	{
		std::string stSearch = questname + ".";

		TFlagMap::iterator it = m_FlagMap.begin();

		std::vector<std::string> vecEraseFlag;

		while (it != m_FlagMap.end())
		{
			TFlagMap::iterator it_cur = it++;
			if (it_cur->second == 0)
				continue;

			if (!it_cur->first.compare(0, stSearch.length(), stSearch.c_str(), stSearch.length()))
			{
				vecEraseFlag.push_back(it_cur->first);
				SaveFlag(it_cur->first, 0);
			}
		}

		for (std::string stFlagName : vecEraseFlag)
			m_FlagMap.erase(stFlagName);

		return vecEraseFlag.size();
	}

	int PC::GetFlag(const string & name)
	{
		TFlagMap::iterator it = m_FlagMap.find(name);

		if (it != m_FlagMap.end())
		{
			sys_log(1, "QUEST getting flag %s %d", name.c_str(),it->second);
			return it->second;
		}
		return 0;
	}

	void PC::SaveFlag(const string & name, int value)
	{
		TFlagMap::iterator it = m_FlagSaveMap.find(name);

		if (it == m_FlagSaveMap.end())
			m_FlagSaveMap.insert(make_pair(name, value));
		else if (it->second != value)
			it->second = value;
	}

	// only from lua call
	void PC::SetCurrentQuestStateName(const string& state_name) 
	{
		SetFlag(m_stCurQuest + ".__status", CQuestManager::Instance().GetQuestStateIndex(m_stCurQuest,state_name));
	}

	void PC::SetQuestState(const string& quest_name, const string& state_name)
	{
		SetQuestState(quest_name, CQuestManager::Instance().GetQuestStateIndex(quest_name, state_name));
	}

	void PC::SetQuestState(const string& quest_name, int new_state_index)
	{
		int iNowState = GetFlag(quest_name + ".__status");

		if (iNowState != new_state_index)
			AddQuestStateChange(quest_name, iNowState, new_state_index);
	}

	void PC::AddQuestStateChange(const string& quest_name, int prev_state, int next_state)
	{
		DWORD dwQuestIndex = CQuestManager::instance().GetQuestIndexByName(quest_name);
		sys_log(0, "QUEST reserve Quest State Change quest %s[%u] from %d to %d", quest_name.c_str(), dwQuestIndex, prev_state, next_state);
		m_QuestStateChange.push_back(TQuestStateChangeInfo(dwQuestIndex, prev_state, next_state));
	}

	void PC::SetQuest(const string& quest_name, QuestState& qs)
	{
		//sys_log(0, "PC SetQuest %s", quest_name.c_str());
		unsigned int qi = CQuestManager::instance().GetQuestIndexByName(quest_name);
		QuestInfo::iterator it = m_QuestInfo.find(qi);

		if (it == m_QuestInfo.end())
			m_QuestInfo.insert(make_pair(qi, qs));
		else
			it->second = qs;

		m_stCurQuest = quest_name;
		m_RunningQuestState = &m_QuestInfo[qi];
		m_iSendToClient = 0;

		m_iLastState = qs.st;
		SetFlag(quest_name + ".__status", qs.st);

		//m_RunningQuestState->iIndex = GetCurrentQuestBeginFlag();
		m_RunningQuestState->iIndex = qi;
		m_bShouldSendDone = false;
		//if (GetCurrentQuestBeginFlag())
		//{
		//m_bSendToClient = true;
		//}
	}

	void PC::AddTimer(const string & name, LPEVENT pEvent)
	{
		RemoveTimer(name);
		m_TimerMap.insert(make_pair(name, pEvent));
		sys_log(!test_server, "QUEST add timer %p %d", get_pointer(pEvent), m_TimerMap.size());
	}

	void PC::RemoveTimerNotCancel(const string & name)
	{
		TTimerMap::iterator it = m_TimerMap.find(name);

		if (it != m_TimerMap.end())
		{
			sys_log(0, "QUEST remove with no cancel %p", get_pointer(it->second));
			m_TimerMap.erase(it);
		}

		sys_log(1, "QUEST timer map size %d by RemoveTimerNotCancel", m_TimerMap.size());
	}

	void PC::RemoveTimer(const string & name)
	{
		TTimerMap::iterator it = m_TimerMap.find(name);

		if (it != m_TimerMap.end())
		{
			sys_log(!test_server, "QUEST remove timer %p", get_pointer(it->second));
			CancelTimerEvent(&it->second);
			m_TimerMap.erase(it);
		}

		sys_log(1, "QUEST timer map size %d by RemoveTimer", m_TimerMap.size());
	}

	void PC::ClearTimer()
	{
		sys_log(!test_server, "QUEST clear timer %d", m_TimerMap.size());
		TTimerMap::iterator it = m_TimerMap.begin();

		while (it != m_TimerMap.end())
		{
			CancelTimerEvent(&it->second);
			++it;
		}

		m_TimerMap.clear();
	}

	void PC::SetCurrentQuestStartFlag()
	{
		if (!GetCurrentQuestBeginFlag())
		{
			SetCurrentQuestBeginFlag();
		}
	}

	void PC::SetCurrentQuestDoneFlag()
	{
		if (GetCurrentQuestBeginFlag())
		{
			ClearCurrentQuestBeginFlag();
		}
	}

	void PC::SendQuestInfoPakcet()
	{
		assert(m_iSendToClient);
		assert(m_RunningQuestState);

		// sys_log(0, "PC::SendQuestInfoPacket: %s", CQuestManager::instance().GetCurrentCharacterPtr()->GetName());
		network::GCOutputPacket<network::GCQuestInfoPacket> qi;

		qi->set_index(m_RunningQuestState->iIndex);
		qi->set_flag(m_iSendToClient);

		TEMP_BUFFER buf;

		if (m_iSendToClient & QUEST_SEND_ISBEGIN)
			qi->set_is_begin(m_RunningQuestState->bStart ? 1 : 0);
		if (m_iSendToClient & QUEST_SEND_TITLE)
			qi->set_title(m_RunningQuestState->_title);
#ifdef __QUEST_CATEGORIES__
		if (m_iSendToClient & QUEST_SEND_CATEGORY_ID)
			qi->set_cat_id(m_RunningQuestState->_category_id);
#endif
		if (m_iSendToClient & QUEST_SEND_CLOCK_NAME)
			qi->set_clock_name(m_RunningQuestState->_clock_name);
		if (m_iSendToClient & QUEST_SEND_CLOCK_VALUE)
			qi->set_clock_value(m_RunningQuestState->_clock_value);
		if (m_iSendToClient & QUEST_SEND_COUNTER)
		{
			qi->set_counter_name(m_RunningQuestState->_counter_name);
			qi->set_counter_value(m_RunningQuestState->_counter_value);
		}
		if (m_iSendToClient & QUEST_SEND_ICON_FILE)
			qi->set_icon_file_name(m_RunningQuestState->_icon_file);

		CQuestManager::instance().GetCurrentCharacterPtr()->GetDesc()->Packet(qi);

		m_iSendToClient = 0;
	}

	void PC::EndRunning()
	{
		// unlocked locked npc
		{
			LPCHARACTER npc = CQuestManager::instance().GetCurrentNPCCharacterPtr();
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			// npc ÀÖ¾ú´ø °æ¿ì
			if (npc && !npc->IsPC())
			{
				// ±× ¿£ÇÇ¾¾°¡ ³ª¿¡°Ô ¶ôÀÎ °æ¿ì
				if (ch && ch->GetPlayerID() == npc->GetQuestNPCID())
				{
					ch->SetQuestNPCID(0);
					sys_log(0, "QUEST NPC lock isn't unlocked : pid %u", ch->GetPlayerID());
					CQuestManager::instance().WriteRunningStateToSyserr();
				}
			}
		}

		// commit data
		if (HasReward())
		{
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			if (ch != NULL) {
				Save(ch);

				Reward(ch);
				ch->Save();
			}
		}
		m_bIsGivenReward = false;

		if (m_iSendToClient)
		{
			sys_log(1, "QUEST end running %d", m_iSendToClient);
			SendQuestInfoPakcet();
		}

		if (m_RunningQuestState == NULL) {
			sys_log(0, "Entered PC::EndRunning() with invalid running quest state");
			return;
		}
		QuestState * pOldState = m_RunningQuestState;
		int iNowState = m_RunningQuestState->st;

		m_RunningQuestState = 0;

		if (m_iLastState != iNowState)
		{
			LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
			DWORD dwQuestIndex = CQuestManager::instance().GetQuestIndexByName(m_stCurQuest);
			if (ch)
			{
				SetFlag(m_stCurQuest + ".__status", m_iLastState);
				CQuestManager::instance().LeaveState(ch->GetPlayerID(), dwQuestIndex, m_iLastState);
				pOldState->st = iNowState;
				SetFlag(m_stCurQuest + ".__status", iNowState);
				CQuestManager::instance().EnterState(ch->GetPlayerID(), dwQuestIndex, iNowState);
				if (GetFlag(m_stCurQuest + ".__status") == iNowState)
					CQuestManager::instance().Letter(ch->GetPlayerID(), dwQuestIndex, iNowState);
			}
		}


		DoQuestStateChange();
	}

	void PC::DoQuestStateChange()
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		std::vector<TQuestStateChangeInfo> vecQuestStateChange;
		m_QuestStateChange.swap(vecQuestStateChange);

		for (DWORD i=0; i<vecQuestStateChange.size(); ++i)
		{
			const TQuestStateChangeInfo& rInfo = vecQuestStateChange[i];
			if (rInfo.quest_idx == 0)
				continue;

			DWORD dwQuestIdx = rInfo.quest_idx;
			QuestInfoIterator it = quest_find(dwQuestIdx);
			const string stQuestName = CQuestManager::instance().GetQuestNameByIndex(dwQuestIdx);

			if (it == quest_end())
			{
				QuestState qs;
				qs.st = 0;

				m_QuestInfo.insert(make_pair(dwQuestIdx, qs));
				SetFlag(stQuestName + ".__status", 0);

				it = quest_find(dwQuestIdx);
			}

			sys_log(0, "QUEST change reserved Quest State Change quest %u from %d to %d (%d %d)", dwQuestIdx, rInfo.prev_state, rInfo.next_state, it->second.st, rInfo.prev_state );

			assert(it->second.st == rInfo.prev_state);

			CQuestManager::instance().LeaveState(ch->GetPlayerID(), dwQuestIdx, rInfo.prev_state);
			it->second.st = rInfo.next_state;
			SetFlag(stQuestName + ".__status", rInfo.next_state);

			CQuestManager::instance().EnterState(ch->GetPlayerID(), dwQuestIdx, rInfo.next_state);

			if (GetFlag(stQuestName + ".__status")==rInfo.next_state)
				CQuestManager::instance().Letter(ch->GetPlayerID(), dwQuestIdx, rInfo.next_state);
		}
	}

	void PC::CancelRunning()
	{
		sys_log(!test_server, "PC::CancelRunning");

		// cancel data
		m_RunningQuestState = 0;
		m_iSendToClient = 0;
		m_bShouldSendDone = false;
	}

	void PC::SetSendFlag(int idx)
	{
		m_iSendToClient |= idx;
	}

	void PC::ClearCurrentQuestBeginFlag()
	{
		//cerr << "iIndex " << m_RunningQuestState->iIndex << endl;
		SetSendFlag(QUEST_SEND_ISBEGIN);
		m_RunningQuestState->bStart = false;
		//SetFlag(m_stCurQuest+".__isbegin", 0);
	}

	void PC::SetCurrentQuestBeginFlag()
	{
		CQuestManager& q = CQuestManager::instance();
		int iQuestIndex = q.GetQuestIndexByName(m_stCurQuest);
		m_RunningQuestState->bStart = true;
		m_RunningQuestState->iIndex = iQuestIndex;

		SetSendFlag(QUEST_SEND_ISBEGIN);
		//SetFlag(m_stCurQuest+".__isbegin", iQuestIndex);
	}

	int PC::GetCurrentQuestBeginFlag()
	{
		return m_RunningQuestState?m_RunningQuestState->iIndex:0;
		//return GetFlag(m_stCurQuest+".__isbegin");
	}

	void PC::SetCurrentQuestTitle(const string& title)
	{
		SetSendFlag(QUEST_SEND_TITLE);
		m_RunningQuestState->_title = title;
	}

	void PC::SetQuestTitle(const string& quest, const string& title)
	{
		//SetSendFlag(QUEST_SEND_TITLE);
		QuestInfo::iterator it = m_QuestInfo.find(CQuestManager::instance().GetQuestIndexByName(quest));

		if (it != m_QuestInfo.end()) 
		{
			//(*it)->_title = title;
			QuestState* old = m_RunningQuestState;
			int old2 = m_iSendToClient;
			std::string oldquestname = m_stCurQuest;
			m_stCurQuest = quest;
			m_RunningQuestState = &it->second;
			m_iSendToClient = QUEST_SEND_TITLE;
			m_RunningQuestState->iIndex = GetCurrentQuestBeginFlag();

			SetCurrentQuestTitle(title);

			SendQuestInfoPakcet();

			m_stCurQuest = oldquestname;
			m_RunningQuestState = old;
			m_iSendToClient = old2;
		}
	}

#ifdef __QUEST_CATEGORIES__
	void PC::SetCurrentQuestCategoryID(int value)
	{
		SetSendFlag(QUEST_SEND_CATEGORY_ID);
		m_RunningQuestState->_category_id = value;
	}
#endif

	void PC::SetCurrentQuestClockName(const string& name)
	{
		SetSendFlag(QUEST_SEND_CLOCK_NAME);
		m_RunningQuestState->_clock_name = name;
	}

	void PC::SetCurrentQuestClockValue(int value)
	{
		SetSendFlag(QUEST_SEND_CLOCK_VALUE);
		m_RunningQuestState->_clock_value = value;
	}

	void PC::SetCurrentQuestCounterName(const string& name)
	{
		SetSendFlag(QUEST_SEND_COUNTER);
		m_RunningQuestState->_counter_name = name;
	}

	void PC::SetCurrentQuestCounterValue(int value)
	{
		// SetSendFlag(QUEST_SEND_COUNTER_VALUE);
		m_RunningQuestState->_counter_value = value;
	}

	void PC::SetCurrentQuestIconFile(const string& icon_file)
	{
		SetSendFlag(QUEST_SEND_ICON_FILE);
		m_RunningQuestState->_icon_file = icon_file;
	}

	void PC::Save(LPCHARACTER ch)
	{
		// Kill statistics save
		if (ch->IsKillcountChanged())
		{
			if (ch->GetRealPoint(POINT_EMPIRE_A_KILLED)) {
				m_FlagSaveMap.insert(make_pair("killcounter.empire_0_kill", ch->GetRealPoint(POINT_EMPIRE_A_KILLED)));
			}
			if (ch->GetRealPoint(POINT_EMPIRE_B_KILLED)) {
				m_FlagSaveMap.insert(make_pair("killcounter.empire_1_kill", ch->GetRealPoint(POINT_EMPIRE_B_KILLED)));
			}
			if (ch->GetRealPoint(POINT_EMPIRE_C_KILLED)) {
				m_FlagSaveMap.insert(make_pair("killcounter.empire_2_kill", ch->GetRealPoint(POINT_EMPIRE_C_KILLED)));
			}
			if (ch->GetRealPoint(POINT_DUELS_WON)) {
				m_FlagSaveMap.insert(make_pair("killcounter.duel_won", ch->GetRealPoint(POINT_DUELS_WON)));
			}
			if (ch->GetRealPoint(POINT_DUELS_LOST)) {
				m_FlagSaveMap.insert(make_pair("killcounter.duel_lost", ch->GetRealPoint(POINT_DUELS_LOST)));
			}
			if (ch && ch->GetRealPoint(POINT_MONSTERS_KILLED)) {
				m_FlagSaveMap.insert(make_pair("killcounter.monster_kill", ch->GetRealPoint(POINT_MONSTERS_KILLED)));
			}
			if (ch->GetRealPoint(POINT_BOSSES_KILLED)) {
				m_FlagSaveMap.insert(make_pair("killcounter.boss_kill", ch->GetRealPoint(POINT_BOSSES_KILLED)));
			}
			if (ch->GetRealPoint(POINT_STONES_DESTROYED)) {
				m_FlagSaveMap.insert(make_pair("killcounter.stone_kill", ch->GetRealPoint(POINT_STONES_DESTROYED)));
			}
		}
		
		if (m_FlagSaveMap.empty())
			return;

		TFlagMap::iterator it = m_FlagSaveMap.begin();

		network::GDOutputPacket<network::GDQuestSavePacket> pack;

		while (it != m_FlagSaveMap.end())
		{
			const std::string & stComp = it->first;
			long lValue = it->second;

			++it;

			int iPos = stComp.find(".");

			if (iPos < 0)
			{
				sys_err("quest::PC::Save : cannot find . in FlagMap");
				continue;
			}

			string stName;
			stName.assign(stComp, 0, iPos);

			string stState;
			stState.assign(stComp, iPos + 1, stComp.length());

			if (stName.length() == 0 || stState.length() == 0)
			{
				sys_err("quest::PC::Save : invalid quest data: %s", stComp.c_str());
				continue;
			}

			sys_log(1, "QUEST Save Flag %s, %s %d", stName.c_str(), stState.c_str(), lValue);

			if (stName.length() >= QUEST_NAME_MAX_LEN)
			{
				sys_err("quest::PC::Save : quest name overflow");
				continue;
			}

			if (stState.length() >= QUEST_STATE_MAX_LEN)
			{
				sys_err("quest::PC::Save : quest state overflow");
				continue;
			}

			auto& r = *pack->add_datas();

			r.set_pid(m_dwID);
			r.set_name(stName);
			r.set_state(stState);
			r.set_value(lValue);
		}

		if (pack->datas_size() > 0)
		{
			sys_log(1, "QuestPC::Save %d", pack->datas_size());
			db_clientdesc->DBPacket(pack);
		}

		m_FlagSaveMap.clear();
	}

	bool PC::HasQuest(const string & quest_name)
	{
		unsigned int qi = CQuestManager::instance().GetQuestIndexByName(quest_name);
		return m_QuestInfo.find(qi)!=m_QuestInfo.end();
	}

	QuestState & PC::GetQuest(const string & quest_name)
	{
		unsigned int qi = CQuestManager::instance().GetQuestIndexByName(quest_name);
		return m_QuestInfo[qi];
	}

	void PC::GiveItem(const string& label, DWORD dwVnum, int count)
	{
		sys_log(1, "QUEST GiveItem %s %d %d", label.c_str(),dwVnum,count);
		if (!GetFlag(m_stCurQuest+"."+label))
		{
			m_vRewardData.push_back(RewardData(RewardData::REWARD_TYPE_ITEM, dwVnum, count));
			//SetFlag(m_stCurQuest+"."+label,1);
		}
		else
			m_bIsGivenReward = true;
	}

	void PC::GiveExp(const string& label, DWORD exp)
	{
		sys_log(1, "QUEST GiveExp %s %d", label.c_str(),exp);

		if (!GetFlag(m_stCurQuest+"."+label))
		{
			m_vRewardData.push_back(RewardData(RewardData::REWARD_TYPE_EXP, exp));
			//SetFlag(m_stCurQuest+"."+label,1);
		}
		else
			m_bIsGivenReward = true;
	}

	void PC::Reward(LPCHARACTER ch)
	{
		if (m_bIsGivenReward)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Äù½ºÆ®> ÀÌÀü¿¡ °°Àº º¸»óÀ» ¹ÞÀº ÀûÀÌ ÀÖ¾î ´Ù½Ã ¹ÞÁö ¾Ê½À´Ï´Ù."));
			m_bIsGivenReward = false;
		}

		for (vector<RewardData>::iterator it = m_vRewardData.begin(); it != m_vRewardData.end(); ++it)
		{
			switch (it->type)
			{
				case RewardData::REWARD_TYPE_EXP:
					sys_log(0, "EXP cur %d add %d next %d",ch->GetExp(), it->value1, ch->GetNextExp());

					if (ch->GetExp() + it->value1 > ch->GetNextExp())
						ch->PointChange(POINT_EXP, ch->GetNextExp() - 1 - ch->GetExp());
					else
						ch->PointChange(POINT_EXP, it->value1);

					break;

				case RewardData::REWARD_TYPE_ITEM:
					ch->AutoGiveItem(it->value1, it->value2);
					break;

				case RewardData::REWARD_TYPE_NONE:
				default:
					sys_err("Invalid RewardData type");
					break;
			}
		}

		m_vRewardData.clear();
	}

	void PC::Build()
	{
		itertype(m_FlagMap) it;
		for (it = m_FlagMap.begin(); it != m_FlagMap.end(); ++it)
		{
			if (it->first.size()>9 && it->first.compare(it->first.size()-9,9, ".__status") == 0)
			{
				DWORD dwQuestIndex = CQuestManager::instance().GetQuestIndexByName(it->first.substr(0, it->first.size()-9));
				int state = it->second;
				QuestState qs;
				qs.st = state;

				m_QuestInfo.insert(make_pair(dwQuestIndex, qs));
			}
		}
	}

	void PC::ClearQuest(const string& quest_name)
	{
		string quest_name_with_dot = quest_name + '.';
		for (itertype(m_FlagMap) it = m_FlagMap.begin(); it!= m_FlagMap.end();)
		{
			itertype(m_FlagMap) itNow = it++;
			if (itNow->second != 0 && itNow->first.compare(0, quest_name_with_dot.size(), quest_name_with_dot) == 0)
			{
				//m_FlagMap.erase(itNow);
				SetFlag(itNow->first, 0);
			}
		}

		ClearTimer();

		quest::PC::QuestInfoIterator it = quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(quest_name);

		while (it!= quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = 0;
				break;
			}

			++it;
		}
	}

	void PC::SendFlagList(LPCHARACTER ch)
	{
		for (itertype(m_FlagMap) it = m_FlagMap.begin(); it!= m_FlagMap.end(); ++it)
		{
			if (it->first.size()>9 && it->first.compare(it->first.size()-9,9, ".__status") == 0)
			{
				const string quest_name = it->first.substr(0, it->first.size()-9);
				const char* state_name = CQuestManager::instance().GetQuestStateName(quest_name, it->second);
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %s (%d)", quest_name.c_str(), state_name, it->second);
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "%s %d", it->first.c_str(), it->second);
			}
		}
	}
}

