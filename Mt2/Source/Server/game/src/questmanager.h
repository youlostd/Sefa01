#ifndef __METIN2_SERVER_QUEST_MANAGER__
#define __METIN2_SERVER_QUEST_MANAGER__

#include "stdafx.h"

#include "questnpc.h"

class ITEM;
class CHARACTER;
class CDungeon;

namespace quest
{
	using namespace std;

	bool IsScriptTrue(const char* code, int size);
	string ScriptToString(const string& str);

	class CQuestManager : public singleton<CQuestManager>
	{
		public:
			enum
			{
				QUEST_SKIN_NOWINDOW,
				QUEST_SKIN_NORMAL,
				//QUEST_SKIN_CINEMATIC,
				QUEST_SKIN_SCROLL=4,
				QUEST_SKIN_CINEMATIC=5,
				QUEST_SKIN_COUNT
			};

			typedef map<string, int>		TEventNameMap;
			typedef map<unsigned int, PC>	PCMap;

		public:
			CQuestManager();
			virtual ~CQuestManager();

			bool		Initialize();
			void		Destroy();

			bool		InitializeLua();
			lua_State *		GetLuaState() { return L; }
			void		AddLuaFunctionTable(const char * c_pszName, luaL_reg * preg);

			TEventNameMap	m_mapEventName;

			QuestState		OpenState(const string& quest_name, int state_index);
			void		CloseState(QuestState& qs);
			bool		RunState(QuestState& qs);

			PC *		GetPC(unsigned int pc);
			PC *		GetPCForce(unsigned int pc);	// ÇöÀç PC¸¦ ¹Ù²ÙÁö ¾Ê°í PC Æ÷ÀÎÅÍ¸¦ °¡Á®¿Â´Ù.

			unsigned int	GetCurrentNPCRace();
			const string & 	GetCurrentQuestName();
			unsigned int	FindNPCIDByName(const string& name);

			//void		SetCurrentNPCCharacterPtr(LPCHARACTER ch) { m_pkCurrentNPC = ch; }
			LPCHARACTER		GetCurrentNPCCharacterPtr();

			void		SetCurrentEventIndex(int index) { m_iRunningEventIndex = index; }

			bool		UseItem(unsigned int pc, LPITEM item, bool bReceiveAll);
			bool		PickupItem(unsigned int pc, LPITEM item);
			bool		SIGUse(unsigned int pc, DWORD sig_vnum, LPITEM item, bool bReceiveAll);
			bool		TakeItem(unsigned int pc, unsigned int npc, LPITEM item);
			LPITEM		GetCurrentItem();
			void		ClearCurrentItem();
			void		SetCurrentItem(LPITEM item);
			void		AddServerTimer(const string& name, DWORD arg, LPEVENT event);
			void		ClearServerTimer(const string& name, DWORD arg);
			void		ClearServerTimerNotCancel(const string& name, DWORD arg);
			void		CancelServerTimers(DWORD arg);
			const map<pair<string, DWORD>, LPEVENT>&	GetServerTimerMap() { return m_mapServerTimer; }

			void		SetServerTimerArg(DWORD dwArg);
			DWORD		GetServerTimerArg();

			// event over state and stae
			bool		ServerTimer(unsigned int npc, unsigned int arg);

			void		Login(unsigned int pc, const char * c_pszQuestName = NULL);
			void		Logout(unsigned int pc);
			bool		Timer(unsigned int pc, unsigned int npc);
			bool		Click(unsigned int pc, LPCHARACTER pkNPC);
			void		Kill(unsigned int pc, unsigned int npc);
			void		PartyKill(unsigned int pc, unsigned int npc);
			void		LevelUp(unsigned int pc);
			void		AttrIn(unsigned int pc, LPCHARACTER ch, int attr);
			void		AttrOut(unsigned int pc, LPCHARACTER ch, int attr);
			bool		Target(unsigned int pc, DWORD dwQuestIndex, const char * c_pszTargetName, const char * c_pszVerb);
			bool		GiveItemToPC(unsigned int pc, LPCHARACTER pkChr);
			void		Mount(unsigned int pc);
			void		Unmount(unsigned int pc);
			void		Dead(unsigned int pc);
#ifdef DUNGEON_REPAIR_TRIGGER
			void		DungeonRepair(unsigned int pc);
#endif
			
			void		QuestButton(unsigned int pc, unsigned int quest_index);
			void		QuestInfo(unsigned int pc, unsigned int quest_index);

			void		EnterState(DWORD pc, DWORD quest_index, int state);
			void		LeaveState(DWORD pc, DWORD quest_index, int state);

			void		Letter(DWORD pc);
			void		Letter(DWORD pc, DWORD quest_index, int state);
			
			void		ItemInformer(unsigned int pc, unsigned int vnum);	//µ¶ÀÏ¼±¹°±â´É

			void		OnSendShout(unsigned int pc);
			void		OnAddFriend(unsigned int pc);
			void		OnSellItem(unsigned int pc);
			void		OnItemUsed(unsigned int pc);
			void		OnMountRiding(unsigned int pc);
			void		OnCompleteMissionbook(unsigned int pc);

			//

			bool		CheckQuestLoaded(PC* pc) { return pc && pc->IsLoaded(); }

			// event occurs in one state
			void		Select(unsigned int pc, unsigned int selection);
			void		Resume(unsigned int pc);
			void		Input(unsigned int pc, const char* msg);
			void		Confirm(unsigned int pc, EQuestConfirmType confirm, unsigned int pc2 = 0);
			void		SelectItem(unsigned int pc, unsigned int selection);

			void		LogoutPC(LPCHARACTER ch);
			void		DisconnectPC(LPCHARACTER ch);

			QuestState *	GetCurrentState()	{ return m_CurrentRunningState; }

			void 		LoadStartQuest(const string& quest_name, unsigned int idx);
			//bool		CanStartQuest(const string& quest_name, const PC& pc);
			bool		CanStartQuest(unsigned int quest_index, const PC& pc);
			bool		CanStartQuest(unsigned int quest_index);
			bool		CanEndQuestAtState(const string& quest_name, const string& state_name);

			LPCHARACTER		GetCurrentCharacterPtr() { return m_pCurrentCharacter; }
			void			SetCurrentCharacterPtr(LPCHARACTER ch) { m_pCurrentCharacter = ch; }
			LPCHARACTER		GetCurrentPartyMember() { return m_pCurrentPartyMember; }
			void			SetCurrentPartyMember(LPCHARACTER member) { m_pCurrentPartyMember = member; }
			PC *		GetCurrentPC() { return m_pCurrentPC; }
			PC *		GetCurrentRootPC();

			LPDUNGEON		GetCurrentDungeon();
			void		SelectDungeon(LPDUNGEON pDungeon);

			LPCHARACTER		GetCurrentSelectedNPCCharacterPtr()	{ return m_pCurrentNPCCharacter; }
			void			SetCurrentSelectedNPCCharacterPtr(LPCHARACTER pkChr) { m_pCurrentNPCCharacter = pkChr; }

			void		ClearScript();
			void		SendScript();
			void		AddScript(const string& str);

			void		BuildStateIndexToName(const char* questName);

			int			GetQuestStateIndex(const string& quest_name, const string& state_name);
			const char*		GetQuestStateName(const string& quest_name, const int state_index);

			void		SetSkinStyle(int iStyle);

			void		SetNoSend() { m_bNoSend = true; }

			unsigned int	LoadTimerScript(const string& name);

			//unsigned int	RegisterQuestName(const string& name);

			void		RegisterQuest(const string & name, unsigned int idx);
			unsigned int 	GetQuestIndexByName(const string& name);
			const string& 	GetQuestNameByIndex(unsigned int idx);

			void		RequestSetEventFlag(const string& name, int value, bool isAdd = false);

			void		SetEventFlag(const string& name, int value, bool bIsAdd = false);
			int			GetEventFlag(const string& name);
			void		BroadcastEventFlagOnLogin(LPCHARACTER ch);

			void		SendEventFlagList(LPCHARACTER ch);

			void		Reload();

			//void		CreateAllButton(const string& quest_name, const string& button_name);
			void		SetError() { m_bError = true; }
			void		ClearError() { m_bError = false; }
			bool		IsError() { return m_bError; }
			void		WriteRunningStateToSyserr();
#ifndef __WIN32__
			void		QuestError(const char* func, int line, const char* fmt, ...);
#else
			//void		QuestError(const char* fmt, ...);
			void		QuestError(const char* func, int line, const char* fmt, ...);
#endif

			void		RegisterNPCVnum(DWORD dwVnum);

			void		CatchFish(unsigned int pc, LPITEM item);
			void		MineOre(unsigned int pc, LPITEM item);
			void		DungeonComplete(unsigned int pc, BYTE dungeon);
			void		DropQuestItem(unsigned int pc, int item);
#ifdef __DAMAGE_QUEST_TRIGGER__
			void		QuestDamage(unsigned int pc, unsigned int npc);
#endif

			void CollectYangFromMonster(unsigned int pc, int iYangAmount);
			void CraftGaya(unsigned int pc, int iGayaAmount);
			void KillEnemyFraction(unsigned int pc);
			void SpendSouls(unsigned int pc, int iSoulsAmount);
// Xmas Event
			void OpenXmasDoor(unsigned int pc);
			void SpendXmasSocks(unsigned int pc, int iAmount);

#ifdef __QUEST_SAFE__
			void		SaveCurrentSelection();
			void		ResetCurrentSelection();
#endif

		private:
			LPDUNGEON			m_pSelectedDungeon;
#ifdef __QUEST_SAFE__
			std::vector<LPDUNGEON>	m_pSelectedDungeon_Save;
#endif
			DWORD			m_dwServerTimerArg;

			map<pair<string, DWORD>, LPEVENT>	m_mapServerTimer;

			int				m_iRunningEventIndex;

			map<string, int>		m_mapEventFlag;

			void			GotoSelectState(QuestState& qs);
			void			GotoPauseState(QuestState& qs);
			void			GotoEndState(QuestState& qs);
			void			GotoInputState(QuestState& qs);
			void			GotoConfirmState(QuestState& qs);
			void			GotoSelectItemState(QuestState& qs);

			lua_State *		L;

			bool			m_bNoSend;

			set<unsigned int>			m_registeredNPCVnum;
			map<unsigned int, NPC>		m_mapNPC;
			map<string, unsigned int>	m_mapNPCNameID;
			map<string, unsigned int>	m_mapTimerID;

			QuestState *	m_CurrentRunningState;

			PCMap			m_mapPC;

			LPCHARACTER		m_pCurrentCharacter;
			LPCHARACTER		m_pCurrentNPCCharacter;
			LPCHARACTER		m_pCurrentPartyMember;
			PC*				m_pCurrentPC;
#ifdef __QUEST_SAFE__
			std::vector<LPCHARACTER>	m_pCurrentCharacter_Save;
			std::vector<DWORD>			m_dwCurrentNPCCharacterVID_Save;
			std::vector<LPCHARACTER>	m_pCurrentPartyMember_Save;
			std::vector<PC*>			m_pCurrentPC_Save;
#endif

			string			m_strScript;

			int				m_iCurrentSkin;

			struct stringhash
			{
				size_t operator () (const string& str) const
				{
					const unsigned char * s = (const unsigned char*) str.c_str();
					const unsigned char * end = s + str.size();
					size_t h = 0;

					while (s < end)
					{
						h *= 16777619;
						h ^= (unsigned char) *(unsigned char *) (s++);
					}

					return h;

				}
			};

			typedef TR1_NS::unordered_map<string, int, stringhash> THashMapQuestName;
			typedef TR1_NS::unordered_map<unsigned int, vector<char> > THashMapQuestStartScript;

			THashMapQuestName			m_hmQuestName;
			THashMapQuestStartScript	m_hmQuestStartScript;
			map<unsigned int, string>	m_mapQuestNameByIndex;

			bool						m_bError;

		public:
			static bool ExecuteQuestScript(PC& pc, const string& quest_name, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts = NULL, bool bUseCache = true);
			static bool ExecuteQuestScript(PC& pc, DWORD quest_index, const int state, const char* code, const int code_size, vector<AArgScript*>* pChatScripts = NULL, bool bUseCache = true);
		

		// begin_other_pc_blcok, end_other_pc_blockÀ» À§ÇÑ °´Ã¼µé.
		public:
			void		BeginOtherPCBlock(DWORD pid);
			void		EndOtherPCBlock();
			bool		IsInOtherPCBlock();
			PC*			GetOtherPCBlockRootPC();
			bool		IsSuspended(unsigned int pc);
		private:
			PC*			m_pOtherPCBlockRootPC;
			std::vector <DWORD>	m_vecPCStack;

		public:
			void	SetAddedFriendName(const std::string& c_rstName) { m_stAddedFriendName = c_rstName; }
			const std::string&	GetAddedFriendName() const { return m_stAddedFriendName; }
		private:
			std::string	m_stAddedFriendName;
	};
};

#endif
