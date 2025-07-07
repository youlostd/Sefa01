#ifndef __EVENT_TEAMTAG_HEADER__
#define __EVENT_TEAMTAG_HEADER__

#include "stdafx.h"

#ifdef __EVENT_MANAGER__
#include "event.h"

class CEventTagTeam
{
	// [PUBLIC] Static Values
public:
	enum EStaticValues {
		ATTENDER_TIMEOUT_TIME = 60, // time after a disconnected attender will be kicked from the game
		ATTENDER_EXIT_TIME = 6, // time after a attender will be warped out after dying
		DESTROY_EVENT_TIME = 60, // destroy time (starts after finishing the game)
		MAX_TEAM_COUNT = 7,
	};

	// [PUBLIC] Typedefs
public:
	EVENTINFO(tag_team_event_info)
	{
		CEventTagTeam*	pkEventTagTeam;
	};
	EVENTINFO(attender_event_info)
	{
		CEventTagTeam*	pkEventTagTeam;
		DWORD			dwAttenderPID;
	};
	typedef struct SPlayerInfo
	{
		SPlayerInfo() {}
		SPlayerInfo(DWORD dwPID) : dwPID(dwPID) {}

		DWORD	dwPID;
		char	szName[CHARACTER_NAME_MAX_LEN + 1];
		BYTE	bTeamIndex;
		LPEVENT	pkTimeoutEvent;
		LPEVENT	pkExitEvent;

		bool operator<(const SPlayerInfo& other) const { return dwPID < other.dwPID; }
	} TPlayerInfo;
	typedef struct STagTeamName
	{
		char	szName1[CHARACTER_NAME_MAX_LEN + 1];
		char	szName2[CHARACTER_NAME_MAX_LEN + 1];
	} TTagTeamName;
	typedef std::set<TPlayerInfo> TAttenderSet;
	typedef std::map<BYTE, TTagTeamName> TTeamNameMap;

	typedef struct SRewardInfo
	{
		DWORD	dwItemVnum;
		BYTE	bItemCount;
		DWORD	dwChance;
	} TRewardInfo;

	// [PUBLIC] (De-)Initialize Functions
public:
	CEventTagTeam();
	~CEventTagTeam();

	void				Destroy();

	// [PUBLIC] Main Functions
public:
	void				JoinTagTeam(DWORD dwPID1, const char* szName1, DWORD dwPID2, const char* szName2);

	// [PUBLIC] Trigger Functions
public:
	void				OnPlayerOnline(LPCHARACTER pkChr);
	void				OnPlayerOffline(LPCHARACTER pkChr);
	void				OnPlayerLoadAffect(LPCHARACTER pkChr);
	void				OnPlayerDead(LPCHARACTER pkChr);

	// [PUBLIC] Event Functions
public:
	void				EventAttenderTimeout(DWORD dwPID);
	void				EventAttenderExit(DWORD dwPID);

	// [PUBLIC] Data Functions
public:
	BYTE				GetAttenderCount() const;
	long				GetMapIndex() { return m_dwMapIndex; }

	// [PRIVATE] Data Functions
private:
	void				AddAttender(DWORD dwPID, const char* szName, BYTE bTeamIndex, BYTE bPositionIndex);
	void				RemoveAttender(DWORD dwPID);
	void				ExitAttender(DWORD dwPID);
	TPlayerInfo*		GetAttenderInfo(DWORD dwPID);

	// [PRIVATE] Compute Functions
private:
	void				CheckMatch();
	void				GiveReward(int iTeamIndex);

	// [PRIVATE] Variables
private:
	DWORD				m_dwMapIndex;

	BYTE				m_bTagTeamCount;
	BYTE				m_bAttenderCount;

	bool				m_bIsOver;
	LPEVENT				m_pkDestroyEvent;

	std::vector<BYTE>	m_vec_PositionIndexes;
	TAttenderSet		m_set_Attender;
	TTeamNameMap		m_map_TeamName;
};
#endif

#endif
