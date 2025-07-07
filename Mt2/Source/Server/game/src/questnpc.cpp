﻿#include "stdafx.h"
#include <fstream>
#include <sstream>
#include "questmanager.h"
#include "profiler.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"

// questpc.h: PC::typedef Quest
// questpc.h: PC::typedef map<unsigned long, QuestState> QuestInfo;
// typedef 


namespace quest
{
	NPC::NPC()
	{
		m_vnum = 0;
	}

	NPC::~NPC()
	{
	}

	void NPC::Set(unsigned int vnum, const string & script_name)
	{
		m_vnum = vnum;

		char buf[PATH_MAX];

		CQuestManager::TEventNameMap::iterator itEventName = CQuestManager::instance().m_mapEventName.begin();

		while (itEventName != CQuestManager::instance().m_mapEventName.end())
		{
			typeof(itEventName) it = itEventName;
			++itEventName;

			int is = snprintf(buf, sizeof(buf), "%s/%s/%s/", Locale_GetQuestObjectPath().c_str(), script_name.c_str(), it->first.c_str());

			if (is < 0 || is >= (int) sizeof(buf))
				is = sizeof(buf) - 1;

			//sys_log(0, "XXX %s", buf);
			int event_index = it->second;

			DIR * pdir = opendir(buf);

			if (!pdir)
				continue;

			dirent * pde;

			while ((pde = readdir(pdir)))
			{
				if (pde->d_name[0] == '.')
					continue;

				if (!strncasecmp(pde->d_name, "CVS", 3))
					continue;

				sys_log(1, "QUEST reading %s", pde->d_name);
				strlcpy(buf + is, pde->d_name, sizeof(buf) - is);
				LoadStateScript(event_index, buf, pde->d_name);
			}

			closedir(pdir);
		}
	}

	void NPC::LoadStateScript(int event_index, const char* filename, const char* script_name)
	{
		ifstream inf(filename);
		const string s(script_name);

		size_t i = s.find('.');

		CQuestManager & q = CQuestManager::instance();

		//
		// script_name examples:
		//   christmas_tree.start -> argument not exist
		//
		//   guild_manage.start.0.script -> argument exist
		//   guild_manage.start.0.when
		//   guild_manage.start.0.arg

		///////////////////////////////////////////////////////////////////////////
		// Quest name
		const string stQuestName = s.substr(0, i); 

		int quest_index = q.GetQuestIndexByName(stQuestName);

		if (quest_index == 0)
		{
			fprintf(stderr, "cannot find quest index for %s\n", stQuestName.c_str());
			assert(!"cannot find quest index");
			return;
		}

		///////////////////////////////////////////////////////////////////////////
		// State name
		string stStateName;

		size_t j = i;
		i = s.find('.', i + 1);

		if (i == s.npos)
			stStateName = s.substr(j + 1, s.npos);
		else
			stStateName = s.substr(j + 1, i - j - 1);

		int state_index = q.GetQuestStateIndex(stQuestName, stStateName);
		///////////////////////////////////////////////////////////////////////////

		sys_log(!test_server, "QUEST loading %s : %s [STATE] %s", 
				filename, stQuestName.c_str(), stStateName.c_str());

		if (i == s.npos)
		{
			// like in example: christmas_tree.start
			istreambuf_iterator<char> ib(inf), ie;
			copy(ib, ie, back_inserter(m_mapOwnQuest[event_index][quest_index][q.GetQuestStateIndex(stQuestName, stStateName)].m_code));
		}
		else
		{
			//
			// like in example: guild_manage.start.0.blah
			// NOTE : currently, only CHAT script uses argument
			//

			///////////////////////////////////////////////////////////////////////////
			// ¼ø¼­ Index (¿©·¯°³ ÀÖÀ» ¼ö ÀÖÀ¸¹Ç·Î ÀÖ´Â °ÍÀÓ, ½ÇÁ¦ index °ªÀº ¾²Áö ¾ÊÀ½)
			j = i;
			i = s.find('.', i + 1);

			if (i == s.npos)
			{
				sys_err("invalid QUEST STATE index [%s] [%s]",filename, script_name);
				return;
			}

			const int index = strtol(s.substr(j + 1, i - j - 1).c_str(), NULL, 10); 
			///////////////////////////////////////////////////////////////////////////
			// Type name
			j = i;
			i = s.find('.', i + 1);

			if (i != s.npos)
			{
				sys_err("invalid QUEST STATE name [%s] [%s]",filename, script_name);
				return;
			}

			const string type_name = s.substr(j + 1, i - j - 1);
			///////////////////////////////////////////////////////////////////////////

			istreambuf_iterator<char> ib(inf), ie;

			m_mapOwnArgQuest[event_index][quest_index][state_index].resize(MAX(index + 1, m_mapOwnArgQuest[event_index][quest_index][state_index].size()));

			if (type_name == "when")
			{
				copy(ib, ie, back_inserter(m_mapOwnArgQuest[event_index][quest_index][state_index][index].when_condition));
			}
			else if (type_name == "arg")
			{
				string s;
				getline(inf, s);
				m_mapOwnArgQuest[event_index][quest_index][state_index][index].arg.clear();

				for (string::iterator it = s.begin(); it != s.end(); ++it)
				{
					m_mapOwnArgQuest[event_index][quest_index][state_index][index].arg+=*it;
				}
			}
			else if (type_name == "script")
			{
				copy(ib, ie, back_inserter(m_mapOwnArgQuest[event_index][quest_index][state_index][index].script.m_code));
				m_mapOwnArgQuest[event_index][quest_index][state_index][index].quest_index = quest_index;
				m_mapOwnArgQuest[event_index][quest_index][state_index][index].state_index = state_index;
			}
		}
	}

	bool NPC::OnEnterState(PC& pc, DWORD quest_index, int state)
	{
		return ExecuteEventScript(pc, QUEST_ENTER_STATE_EVENT, quest_index, state);
	}

	bool NPC::OnLeaveState(PC& pc, DWORD quest_index, int state)
	{
		return ExecuteEventScript(pc, QUEST_LEAVE_STATE_EVENT, quest_index, state);
	}

	bool NPC::OnLetter(PC& pc, DWORD quest_index, int state)
	{
		return ExecuteEventScript(pc, QUEST_LETTER_EVENT, quest_index, state);
	}

	bool NPC::OnTarget(PC & pc, DWORD dwQuestIndex, const char * c_pszTargetName, const char * c_pszVerb, bool & bRet)
	{
		sys_log(1, "OnTarget begin %s verb %s qi %u", c_pszTargetName, c_pszVerb, dwQuestIndex);

		bRet = false;

		PC::QuestInfoIterator itPCQuest = pc.quest_find(dwQuestIndex);

		if (itPCQuest == pc.quest_end())
		{
			sys_log(1, "no quest");
			return false;
		}

		int iState = itPCQuest->second.st;

		AArgQuestScriptType & r = m_mapOwnArgQuest[QUEST_TARGET_EVENT][dwQuestIndex];
		AArgQuestScriptType::iterator it = r.find(iState);

		if (it == r.end())
		{
			sys_log(1, "no target event, state %d", iState);
			return false;
		}

		vector<AArgScript>::iterator it_vec = it->second.begin();

		int iTargetLen = strlen(c_pszTargetName);

		while (it_vec != it->second.end())
		{
			AArgScript & argScript = *(it_vec++);
			const char * c_pszArg = argScript.arg.c_str();

			sys_log(1, "OnTarget compare %s %d", c_pszArg, argScript.arg.length());

			if (strncmp(c_pszArg, c_pszTargetName, iTargetLen))
				continue;

			const char * c_pszArgVerb = strchr(c_pszArg, '.');

			if (!c_pszArgVerb)
				continue;

			if (strcmp(++c_pszArgVerb, c_pszVerb))
				continue;

			if (argScript.when_condition.size() > 0)
				sys_log(1, "OnTarget when %s size %d", &argScript.when_condition[0], argScript.when_condition.size());
	
			if (argScript.when_condition.size() != 0 && !IsScriptTrue(&argScript.when_condition[0], argScript.when_condition.size()))
				continue;

			sys_log(1, "OnTarget execute qi %u st %d code %s", dwQuestIndex, iState, (const char *) argScript.script.GetCode());
			bRet = CQuestManager::ExecuteQuestScript(pc, dwQuestIndex, iState, argScript.script.GetCode(), argScript.script.GetSize());
			bRet = true;
			return true;
		}

		return false;
	}

	bool NPC::OnAttrIn(PC& pc)
	{
		return HandleEvent(pc, QUEST_ATTR_IN_EVENT);
	}

	bool NPC::OnAttrOut(PC& pc)
	{
		return HandleEvent(pc, QUEST_ATTR_OUT_EVENT);
	}

	bool NPC::OnTakeItem(PC& pc)
	{
		return HandleEvent(pc, QUEST_ITEM_TAKE_EVENT);
	}

	bool NPC::OnUseItem(PC& pc, bool bReceiveAll)
	{
		if (bReceiveAll)
			return HandleReceiveAllEvent(pc, QUEST_ITEM_USE_EVENT);
		else
			return HandleEvent(pc, QUEST_ITEM_USE_EVENT);
	}

	bool NPC::OnSIGUse(PC& pc, bool bReceiveAll)
	{
		if (bReceiveAll)
			return HandleReceiveAllEvent(pc, QUEST_SIG_USE_EVENT);
		else
			return HandleEvent(pc, QUEST_SIG_USE_EVENT);
	}

	bool NPC::OnClick(PC& pc)
	{
		return HandleEvent(pc, QUEST_CLICK_EVENT);
	}

	bool NPC::OnServerTimer(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_SERVER_TIMER_EVENT);
	}

	bool NPC::OnTimer(PC& pc)
	{
		return HandleEvent(pc, QUEST_TIMER_EVENT);
	}

	bool NPC::OnKill(PC & pc)
	{
		//PROF_UNIT puOnKill("quest::NPC::OnKill");
		if (m_vnum)
		{
			//PROF_UNIT puOnKill1("onk1");
			return HandleEvent(pc, QUEST_KILL_EVENT);
		}
		else
		{
			//PROF_UNIT puOnKill2("onk2");
			return HandleReceiveAllEvent(pc, QUEST_KILL_EVENT);
		}
	}

	bool NPC::OnPartyKill(PC & pc)
	{
		if (m_vnum)
		{
			return HandleEvent(pc, QUEST_PARTY_KILL_EVENT);
		}
		else
		{
			return HandleReceiveAllEvent(pc, QUEST_PARTY_KILL_EVENT);
		}
	}

	bool NPC::OnLevelUp(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_LEVELUP_EVENT);
	}

	bool NPC::OnLogin(PC& pc, const char * c_pszQuestName)
	{
		/*
		   if (c_pszQuestName)
		   {
		   DWORD dwQI = CQuestManager::instance().GetQuestIndexByName(c_pszQuestName);

		   if (dwQI)
		   {
		   std::string stQuestName(c_pszQuestName);

		   CQuestManager & q = CQuestManager::instance();

		   QuestMapType::iterator qmit = m_mapOwnQuest[QUEST_LOGIN_EVENT].begin();

		   while (qmit != m_mapOwnQuest[QUEST_LOGIN_EVENT].end())
		   {
		   if (qmit->first != dwQI)
		   {
		   ++qmit;
		   continue;
		   }

		   int iState = pc.GetFlag(stQuestName + "__status");

		   AQuestScriptType::iterator qsit;

		   if ((qsit = qmit->second.find(iState)) != qmit->second.end())
		   {
		   return q.ExecuteQuestScript(pc, stQuestName, iState, qsit->second.GetCode(), qsit->second.GetSize(), NULL, true);
		   }

		   ++qmit;
		   }

		   sys_err("Cannot find any code for %s", c_pszQuestName);
		   }
		   else
		   sys_err("Cannot find quest index by %s", c_pszQuestName);
		   }
		 */
		bool bRet = HandleReceiveAllNoWaitEvent(pc, QUEST_LOGIN_EVENT);
		HandleReceiveAllEvent(pc, QUEST_LETTER_EVENT);
		return bRet;
	}

	bool NPC::OnLogout(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_LOGOUT_EVENT);
	}

	bool NPC::OnMount(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_MOUNT_EVENT);
	}
	
	bool NPC::OnUnmount(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_UNMOUNT_EVENT);
	}

	bool NPC::OnSendShout(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_SEND_SHOUT_EVENT);
	}

	bool NPC::OnAddFriend(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_ADD_FRIEND_EVENT);
	}

	bool NPC::OnSellItem(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_SELL_ITEM_EVENT);
	}

	bool NPC::OnItemUsed(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_ITEM_USED_EVENT);
	}

	bool NPC::OnMountRiding(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_MOUNT_RIDING_EVENT);
	}

	bool NPC::OnCompleteMissionbook(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_COMPLETE_MISSIONBOOKS_EVENT);
	}

	struct FuncMissHandleEvent
	{
		std::vector <DWORD> vdwNewStartQuestIndices;
		int size;

		FuncMissHandleEvent() : vdwNewStartQuestIndices(0), size(0)
		{}

		bool Matched()
		{
			return vdwNewStartQuestIndices.size() != 0;
		}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			// ¾øÀ¸´Ï »õ·Î ½ÃÀÛ
			DWORD dwQuestIndex = itQuestMap->first;

			if (NPC::HasStartState(itQuestMap->second) && CQuestManager::instance().CanStartQuest(dwQuestIndex))
			{
				size++;
				vdwNewStartQuestIndices.push_back(dwQuestIndex);
			}
		}
	};

	struct FuncMatchHandleEvent
	{
		bool bMatched;

		std::vector <DWORD> vdwQuesIndices;
		std::vector <int> viPCStates;
		std::vector <const char*> vcodes;
		std::vector <int> vcode_sizes;
		int size;

		//DWORD dwQuestIndex;
		//int iPCState;
		//const char* code;
		//int code_size;

		FuncMatchHandleEvent()
			: bMatched(false), vdwQuesIndices(0), viPCStates(0), vcodes(0), vcode_sizes(0), size(0)
		{}

		bool Matched()
		{
			return bMatched;
		}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			NPC::AQuestScriptType::iterator itQuestScript;

			int iState = itPCQuest->second.st;
			if ((itQuestScript = itQuestMap->second.find(iState)) != itQuestMap->second.end())
			{
				bMatched = true;
				size++;
				vdwQuesIndices.push_back(itQuestMap->first);
				viPCStates.push_back(iState);
				vcodes.push_back(itQuestScript->second.GetCode());
				vcode_sizes.push_back(itQuestScript->second.GetSize());
			}
		}
	};

	bool NPC::HandleEvent(PC& pc, int EventIndex)
	{
		if (EventIndex < 0 || EventIndex >= QUEST_EVENT_COUNT)
		{
			sys_err("QUEST invalid EventIndex : %d", EventIndex);
			return false;
		}

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				CQuestManager & mgr = CQuestManager::instance();

				sys_err("QUEST There's suspended quest state, can't run new quest state (quest: %s pc: %s)",
						pc.GetCurrentQuestName().c_str(),
						mgr.GetCurrentCharacterPtr() ? mgr.GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}
		sys_log(!test_server, "NPC::HandleEvent PC %u eventIdx %d", pc.GetID(), EventIndex);

		FuncMissHandleEvent fMiss;
		FuncMatchHandleEvent fMatch;
		MatchingQuest(pc, m_mapOwnQuest[EventIndex], fMatch, fMiss);

		bool r = false;
		if (fMatch.Matched())
		{
			for (int i = 0; i < fMatch.size; i++)
			{
				if ( i != 0 ) {
					//2012.05.14 <±è¿ë¿í> : Äù½ºÆ® ¸Å´ÏÀúÀÇ m_pCurrentPC°¡ ¹Ù²î´Â °æ¿ì°¡ ¹ß»ýÇÏ¿©,
					//µÎ°³ ÀÌ»óÀÇ ½ºÅ©¸³Æ®¸¦ ½ÇÇà½Ã, µÎ¹øÂ° ºÎÅÍ´Â Äù½ºÆ® ¸Å´ÏÀúÀÇ PC °ªÀ» »õ·Î ¼ÂÆÃÇÑ´Ù.
					PC * pPC = CQuestManager::instance().GetPC(pc.GetID());

					if (pc.IsRunning())
					{
						char* szCurQuestScript = new char[fMatch.vcode_sizes[i] + 1];
						strncpy(szCurQuestScript, fMatch.vcodes[i], fMatch.vcode_sizes[i]);
						szCurQuestScript[fMatch.vcode_sizes[i]] = 0;
						sys_err("already running %s cannot execute quest script of quest [%u]: %s", pc.GetCurrentQuestName().c_str(), fMatch.vdwQuesIndices[i], szCurQuestScript);
						delete[] szCurQuestScript;
						continue;
					}
				}
				
				CQuestManager::ExecuteQuestScript(pc, fMatch.vdwQuesIndices[i], fMatch.viPCStates[i],
					fMatch.vcodes[i], fMatch.vcode_sizes[i]);
			}
			r = true;
		}
		if (fMiss.Matched())
		{
			QuestMapType& rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
			
			for (int i = 0; i < fMiss.size; i++)
			{
				AStateScriptType& script = rmapEventOwnQuest[fMiss.vdwNewStartQuestIndices[i]][0];

				if (pc.IsRunning())
				{
					char* szCurQuestScript = new char[script.GetSize() + 1];
					strncpy(szCurQuestScript, script.GetCode(), script.GetSize());
					szCurQuestScript[script.GetSize()] = 0;
					sys_err("already running %s cannot execute quest script of quest [%u]: %s", pc.GetCurrentQuestName().c_str(), fMiss.vdwNewStartQuestIndices[i], szCurQuestScript);
					delete[] szCurQuestScript;
					continue;
				}

				CQuestManager::ExecuteQuestScript(pc, fMiss.vdwNewStartQuestIndices[i], 0, script.GetCode(), script.GetSize());
			}
			r = true;
		}
		else
		{
			return r;
		}
		return true;
	}

	struct FuncMissHandleReceiveAllEvent
	{
		bool bHandled;

		FuncMissHandleReceiveAllEvent()
		{
			bHandled = false;
		}

		void operator() (PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			DWORD dwQuestIndex = itQuestMap->first;

			if (NPC::HasStartState(itQuestMap->second) && CQuestManager::instance().CanStartQuest(dwQuestIndex))
			{
				const NPC::AQuestScriptType & QuestScript = itQuestMap->second;
				itertype(QuestScript) it = QuestScript.find(QUEST_START_STATE_INDEX);

				if (it != QuestScript.end())
				{
					bHandled = true;
					CQuestManager::ExecuteQuestScript(
							*CQuestManager::instance().GetCurrentPC(), 
							dwQuestIndex,
							QUEST_START_STATE_INDEX, 
							it->second.GetCode(), 
							it->second.GetSize());
				}
			}
		}
	};

	struct FuncMatchHandleReceiveAllEvent
	{
		bool bHandled;

		FuncMatchHandleReceiveAllEvent()
		{
			bHandled = false;
		}

		void operator() (PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			const NPC::AQuestScriptType& QuestScript = itQuestMap->second;
			int iPCState = itPCQuest->second.st;
			itertype(QuestScript) itQuestScript = QuestScript.find(iPCState);

			if (itQuestScript != QuestScript.end())
			{
				bHandled = true;

				CQuestManager::ExecuteQuestScript(
						*CQuestManager::instance().GetCurrentPC(), 
						itQuestMap->first, 
						iPCState, 
						itQuestScript->second.GetCode(), 
						itQuestScript->second.GetSize());
			}
		}
	};

	bool NPC::HandleReceiveAllEvent(PC& pc, int EventIndex)
	{
		if (EventIndex < 0 || EventIndex >= QUEST_EVENT_COUNT)
		{
			sys_err("QUEST invalid EventIndex : %d", EventIndex);
			return false;
		}

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				CQuestManager & mgr = CQuestManager::instance();

				sys_err("QUEST There's suspended quest state, can't run new quest state (quest: %s pc: %s)",
						pc.GetCurrentQuestName().c_str(),
						mgr.GetCurrentCharacterPtr() ? mgr.GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}
		sys_log(!test_server, "NPC::HandleEventReceiveAll PC %u eventIdx %d", pc.GetID(), EventIndex);

		FuncMissHandleReceiveAllEvent fMiss;
		FuncMatchHandleReceiveAllEvent fMatch;

		MatchingQuest(pc, m_mapOwnQuest[EventIndex], fMatch, fMiss);
		return fMiss.bHandled || fMatch.bHandled;
	}

	struct FuncDoNothing
	{
		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
		}
	};

	struct FuncMissHandleReceiveAllNoWaitEvent
	{
		bool bHandled;

		FuncMissHandleReceiveAllNoWaitEvent()
		{
			bHandled = false;
		}


		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::QuestMapType::iterator& itQuestMap)
		{
			DWORD dwQuestIndex = itQuestMap->first;

			if (NPC::HasStartState(itQuestMap->second) && CQuestManager::instance().CanStartQuest(dwQuestIndex))
			{
				const NPC::AQuestScriptType& QuestScript = itQuestMap->second;
				itertype(QuestScript) it = QuestScript.find(QUEST_START_STATE_INDEX);
				if (it != QuestScript.end())
				{
					bHandled = true;
					PC* pPC = CQuestManager::instance().GetCurrentPC();
					if (CQuestManager::ExecuteQuestScript(
								*pPC,
								dwQuestIndex,
								QUEST_START_STATE_INDEX, 
								it->second.GetCode(), 
								it->second.GetSize()))
					{
						sys_err("QUEST NOT END RUNNING on Login/Logout - %s", 
								CQuestManager::instance().GetQuestNameByIndex(itQuestMap->first).c_str());

						QuestState& rqs = *pPC->GetRunningQuestState();
						CQuestManager::instance().CloseState(rqs);
						pPC->EndRunning();
					}
				}
			}
		}
	};

	struct FuncMatchHandleReceiveAllNoWaitEvent
	{
		bool bHandled;

		FuncMatchHandleReceiveAllNoWaitEvent()
		{
			bHandled = false;
		}

		void operator()(PC::QuestInfoIterator & itPCQuest, NPC::QuestMapType::iterator & itQuestMap)
		{
			const NPC::AQuestScriptType & QuestScript = itQuestMap->second;
			int iPCState = itPCQuest->second.st;
			itertype(QuestScript) itQuestScript = QuestScript.find(iPCState);

			if (itQuestScript != QuestScript.end())
			{
				PC * pPC = CQuestManager::instance().GetCurrentPC();

				if (CQuestManager::ExecuteQuestScript(
							*pPC,
							itQuestMap->first, 
							iPCState, 
							itQuestScript->second.GetCode(), 
							itQuestScript->second.GetSize()))
				{
					sys_err("QUEST NOT END RUNNING on Login/Logout - %s", 
							CQuestManager::instance().GetQuestNameByIndex(itQuestMap->first).c_str());

					QuestState& rqs = *pPC->GetRunningQuestState();
					CQuestManager::instance().CloseState(rqs);
					pPC->EndRunning();
				}
				bHandled = true;
			}
		}
	};

	bool NPC::HandleReceiveAllNoWaitEvent(PC& pc, int EventIndex)
	{
		//cerr << EventIndex << endl;
		if (EventIndex<0 || EventIndex>=QUEST_EVENT_COUNT)
		{
			sys_err("QUEST invalid EventIndex : %d", EventIndex);
			return false;
		}

		/*
		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				CQuestManager & mgr = CQuestManager::instance();

				sys_err("QUEST There's suspended quest state, can't run new quest state (quest: %s pc: %s)",
						pc.GetCurrentQuestName().c_str(),
						mgr.GetCurrentCharacterPtr() ? mgr.GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}
		*/

		//FuncDoNothing fMiss;
		FuncMissHandleReceiveAllNoWaitEvent fMiss;
		FuncMatchHandleReceiveAllNoWaitEvent fMatch;

		QuestMapType& rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
		MatchingQuest(pc, rmapEventOwnQuest, fMatch, fMiss);

		return fMatch.bHandled || fMiss.bHandled;
	}

	bool NPC::OnInfo(PC & pc, unsigned int quest_index)
	{
		const int EventIndex = QUEST_INFO_EVENT;

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				CQuestManager & mgr = CQuestManager::instance();

				sys_err("QUEST There's suspended quest state, can't run new quest state (quest: %s pc: %s)",
						pc.GetCurrentQuestName().c_str(),
						mgr.GetCurrentCharacterPtr() ? mgr.GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		PC::QuestInfoIterator itPCQuest = pc.quest_find(quest_index);

		if (pc.quest_end() == itPCQuest)
		{
			sys_err("QUEST no quest by (quest %u)", quest_index);
			return false;
		}

		QuestMapType & rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
		QuestMapType::iterator itQuestMap = rmapEventOwnQuest.find(quest_index);

		const char * questName = CQuestManager::instance().GetQuestNameByIndex(quest_index).c_str();

		if (itQuestMap == rmapEventOwnQuest.end())
		{
			sys_err("QUEST no info event (quest %s)", questName);
			return false;
		}

		AQuestScriptType::iterator itQuestScript = itQuestMap->second.find(itPCQuest->second.st);

		if (itQuestScript == itQuestMap->second.end())
		{
			sys_err("QUEST no info script by state %d (quest %s)", itPCQuest->second.st, questName);
			return false;
		}

		CQuestManager::ExecuteQuestScript(pc, quest_index, itPCQuest->second.st, itQuestScript->second.GetCode(), itQuestScript->second.GetSize());
		return true;
	}

	bool NPC::OnButton(PC & pc, unsigned int quest_index)
	{
		const int EventIndex = QUEST_BUTTON_EVENT;

		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				CQuestManager & mgr = CQuestManager::instance();

				sys_err("QUEST There's suspended quest state, can't run new quest state (quest: %s pc: %s)",
						pc.GetCurrentQuestName().c_str(),
						mgr.GetCurrentCharacterPtr() ? mgr.GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		PC::QuestInfoIterator itPCQuest = pc.quest_find(quest_index);

		QuestMapType & rmapEventOwnQuest = m_mapOwnQuest[EventIndex];
		QuestMapType::iterator itQuestMap = rmapEventOwnQuest.find(quest_index);

		// ±×·± Äù½ºÆ®°¡ ¾øÀ½
		if (itQuestMap == rmapEventOwnQuest.end())
			return false;

		int iState = 0;

		if (itPCQuest != pc.quest_end())
		{
			iState = itPCQuest->second.st;
		}
		else
		{
			// »õ·Î ½ÃÀÛÇÒ±î¿ä?
			if (CQuestManager::instance().CanStartQuest(itQuestMap->first, pc) && HasStartState(itQuestMap->second))
				iState = 0;
			else
				return false;
		}

		AQuestScriptType::iterator itQuestScript=itQuestMap->second.find(iState);

		if (itQuestScript==itQuestMap->second.end())
			return false;

		CQuestManager::ExecuteQuestScript(pc, quest_index, iState, itQuestScript->second.GetCode(), itQuestScript->second.GetSize());
		return true;
	}

	struct FuncMissChatEvent
	{
		FuncMissChatEvent(vector<AArgScript*>& rAvailScript)
			: rAvailScript(rAvailScript)
			{}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::ArgQuestMapType::iterator& itQuestMap)
		{
			if (CQuestManager::instance().CanStartQuest(itQuestMap->first) && NPC::HasStartState(itQuestMap->second))
			{
				size_t i;
				for (i = 0; i < itQuestMap->second[QUEST_START_STATE_INDEX].size(); ++i)
				{
					if (itQuestMap->second[QUEST_START_STATE_INDEX][i].when_condition.size() == 0 || 
							IsScriptTrue(&itQuestMap->second[QUEST_START_STATE_INDEX][i].when_condition[0], itQuestMap->second[QUEST_START_STATE_INDEX][i].when_condition.size()))
						rAvailScript.push_back(&itQuestMap->second[QUEST_START_STATE_INDEX][i]);
				}
			}
		}

		vector<AArgScript*>& rAvailScript;
	};

	struct FuncMatchChatEvent
	{
		FuncMatchChatEvent(vector<AArgScript*>& rAvailScript)
			: rAvailScript(rAvailScript)
			{}

		void operator()(PC::QuestInfoIterator& itPCQuest, NPC::ArgQuestMapType::iterator& itQuestMap)
		{
			int iState = itPCQuest->second.st;
			map<int,vector<AArgScript> >::iterator itQuestScript = itQuestMap->second.find(iState);
			if (itQuestScript != itQuestMap->second.end())
			{
				size_t i;
				for (i = 0; i < itQuestMap->second[iState].size(); i++)
				{
					if ( itQuestMap->second[iState][i].when_condition.size() == 0 ||
							IsScriptTrue(&itQuestMap->second[iState][i].when_condition[0], itQuestMap->second[iState][i].when_condition.size()))
						rAvailScript.push_back(&itQuestMap->second[iState][i]);
				}
			}
		}

		vector<AArgScript*>& rAvailScript;
	};

	bool NPC::OnChat(PC& pc)
	{
		if (pc.IsRunning()) 
		{
			if (test_server)
			{
				CQuestManager & mgr = CQuestManager::instance();

				sys_err("QUEST There's suspended quest state, can't run new quest state (quest: %s pc: %s)",
						pc.GetCurrentQuestName().c_str(),
						mgr.GetCurrentCharacterPtr() ? mgr.GetCurrentCharacterPtr()->GetName() : "<none>");
			}

			return false;
		}

		const int EventIndex = QUEST_CHAT_EVENT;
		vector<AArgScript*> AvailScript;

		FuncMatchChatEvent fMatch(AvailScript);
		FuncMissChatEvent fMiss(AvailScript);
		MatchingQuest(pc, m_mapOwnArgQuest[EventIndex], fMatch, fMiss);


		if (AvailScript.empty())
			return false;

		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pc.GetID());

			ostringstream os;
			os << "select(";
			os << '"' << ScriptToString(AvailScript[0]->arg.c_str()).c_str() << '"';
			for (size_t i = 1; i < AvailScript.size(); i++)
			{
				os << ",\"" << ScriptToString(AvailScript[i]->arg.c_str()).c_str() << '"';
			}
			os << ", '"<< "´Ý±â" <<"'";
			os << ")";

			CQuestManager::ExecuteQuestScript(pc, "QUEST_CHAT_TEMP_QUEST", 0, os.str().c_str(), os.str().size(), &AvailScript, false);
		}

		return true;
	}
	
	bool NPC::OnDead(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_DEAD_EVENT);
	}

#ifdef DUNGEON_REPAIR_TRIGGER
	bool NPC::OnDungeonRepair(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_DUNGEON_REPAIR_EVENT);
	}
#endif

	bool NPC::HasChat()
	{
		return !m_mapOwnArgQuest[QUEST_CHAT_EVENT].empty();
	}

	bool NPC::ExecuteEventScript(PC& pc, int EventIndex, DWORD dwQuestIndex, int iState)
	{
		QuestMapType& rQuest = m_mapOwnQuest[EventIndex];

		itertype(rQuest) itQuest = rQuest.find(dwQuestIndex);
		if (itQuest == rQuest.end())
		{
			sys_log(0, "ExecuteEventScript ei %d qi %u is %d - NO QUEST", EventIndex, dwQuestIndex, iState);
			return false;
		}

		AQuestScriptType& rScript = itQuest->second;
		itertype(itQuest->second) itState = rScript.find(iState);
		if (itState == rScript.end())
		{
			sys_log(0, "ExecuteEventScript ei %d qi %u is %d - NO STATE", EventIndex, dwQuestIndex, iState);
			return false;
		}

		sys_log(0, "ExecuteEventScript ei %d qi %u is %d", EventIndex, dwQuestIndex, iState);
		CQuestManager::instance().SetCurrentEventIndex(EventIndex);
		return CQuestManager::ExecuteQuestScript(pc, dwQuestIndex, iState, itState->second.GetCode(), itState->second.GetSize());
	}

	bool NPC::OnPickupItem(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_ITEM_PICK_EVENT);
		else
			return HandleEvent(pc, QUEST_ITEM_PICK_EVENT);
	}
	
	bool NPC::OnItemInformer(PC& pc, unsigned int vnum)
	{
		return HandleEvent(pc, QUEST_ITEM_INFORMER_EVENT);
	}
	
	bool NPC::OnCatchFish(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_CATCH_FISH);
		else
			return HandleEvent(pc, QUEST_CATCH_FISH);	
	}
	
	bool NPC::OnMineOre(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_MINE_ORE);
		else
			return HandleEvent(pc, QUEST_MINE_ORE);	
	}
	
	bool NPC::OnDungeonComplete(PC& pc)
	{
		return HandleReceiveAllEvent(pc, QUEST_DUNGEON_COMPLETE);
	}
	
	bool NPC::OnDropQuestItem(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_DROP_QUEST_ITEM);
		else
			return HandleEvent(pc, QUEST_DROP_QUEST_ITEM);	
	}
	
#ifdef __DAMAGE_QUEST_TRIGGER__
	bool NPC::OnQuestDamage(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_DAMAGE_EVENT);
		else
			return HandleEvent(pc, QUEST_DAMAGE_EVENT);	
	}
#endif

	bool NPC::OnCollectYangFromMonster(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_COLLECT_YANG_FROM_MOB_EVENT);
		else
			return HandleEvent(pc, QUEST_COLLECT_YANG_FROM_MOB_EVENT);
	}

	bool NPC::OnCraftGaya(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_CRAFT_GAYA_EVENT);
		else
			return HandleEvent(pc, QUEST_CRAFT_GAYA_EVENT);
	}

	bool NPC::OnKillEnemyFraction(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_KILL_ENEMY_FRACTION_EVENT);
		else
			return HandleEvent(pc, QUEST_KILL_ENEMY_FRACTION_EVENT);
	}

	bool NPC::OnSpendSouls(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_SPEND_SOULS_EVENT);
		else
			return HandleEvent(pc, QUEST_SPEND_SOULS_EVENT);
	}

	bool NPC::OnXMASOpenDoor(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_XMAS_OPEN_DOOR_EVENT);
		else
			return HandleEvent(pc, QUEST_XMAS_OPEN_DOOR_EVENT);
	}

	bool NPC::OnXMASSantaSocks(PC& pc)
	{
		if (m_vnum == 0)
			return HandleReceiveAllEvent(pc, QUEST_XMAS_SANTA_SOCKS_EVENT);
		else
			return HandleEvent(pc, QUEST_XMAS_SANTA_SOCKS_EVENT);
	}

}
