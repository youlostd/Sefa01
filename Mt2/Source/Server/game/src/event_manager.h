#ifndef __EVENT_MANAGER_HEADER__
#define __EVENT_MANAGER_HEADER__

#include "stdafx.h"

#ifdef __EVENT_MANAGER__
#include "packet.h"
#include "event_tagteam.h"

#include <google/protobuf/repeated_field.h>
#include "headers.hpp"
#include "protobuf_data.h"

class CEventPrison;

// event list
enum EEventList
{
	EVENT_NONE,
	EVENT_EMPIREWAR,
	EVENT_PVP_TOURNAMENT,
	EVENT_TAG_TEAM_BATTLE,
#ifdef __ANGELSDEMON_EVENT2__
	EVENT_ANGELDEAMON,
#endif
#ifdef ENABLE_REACT_EVENT
	EVENT_REACT,
#endif
	EVENT_MAX_NUM,
};

class CEventManager : public singleton<CEventManager>
{
	// [PUBLIC] Typedefs
public:
	typedef struct SEventData {
		SEventData()
		{
			bLoaded = false;
			dwIndex = 0;
			bLevelLimit = 0;
			dwMapIndex = 0;
			dwX = 0;
			dwY = 0;
			dwX2 = 0;
			dwY2 = 0;
			bSpawnRecall = false;
			bIsNotice = false;
		}

		bool	bLoaded;
		DWORD	dwIndex;
		std::string name;
		BYTE	bLevelLimit;
		DWORD	dwMapIndex;
		DWORD	dwX;
		DWORD	dwY;
		DWORD	dwX2;
		DWORD	dwY2;
		BOOL	bSpawnRecall;
		std::string	description;
		BOOL	bIsNotice;
	} TEventData;
	// Tag Team
	typedef network::TEventManagerTagTeam TTagTeam;

#ifdef ENABLE_REACT_EVENT
	struct SReactMember
	{
		SReactMember()
			: reactTime(0)
		{
		}

		DWORD	reactTime;
	};

	enum EReactTaskList
	{
		REACT_TYPE_START,
		REACT_TYPE_EMOTION,
		// REACT_TYPE_WORD,
		REACT_TYPE_NUMBER,
		REACT_TYPE_SYMBOL,
		REACT_TYPE_END,
	};
#endif

	// [PUBLIC] (De-)Initialize Functions
public:
	CEventManager();
	~CEventManager();

	bool				Initialize(const char* c_pszEventDataFileName);
	void				Shutdown();

	// [PUBLIC] Data Functions
public:
	BYTE				GetRunningEventIndex() const				{ return m_dwRunningEventIndex; }
	bool				IsRunningEvent() const						{ return m_dwRunningEventIndex != 0; }

	const TEventData*	GetRunningEventData() const					{ return GetEventData(GetRunningEventIndex()); }
	// [PRIVATE] Data Functions
private:
	void				SetRunningEventIndex(DWORD dwEventIndex)	{ m_dwRunningEventIndex = dwEventIndex; }

	const TEventData*	GetEventData(DWORD dwIndex) const;

	// [PUBLIC] Main Functions
public:
	void				OpenEventRegistration(DWORD dwEventIndex = 0);
	void				P2P_OpenEventRegistration(DWORD dwEventIndex);
	void				CloseEventRegistration(bool bClearEventIndex = true);
	void				P2P_CloseEventRegistration(bool bClearEventIndex);
	void				OnEventOver();
	void				P2P_OnEventOver();
	bool				IsIgnorePlayer(DWORD dwPID);
	void				P2P_IgnorePlayer(DWORD dwPID);
	bool				CanSendRequest(LPCHARACTER pkChr);
	void				SendRequest(LPCHARACTER pkChr);

	void				OpenEventAnnouncement(DWORD dwType, time_t tmStamp);
	void				P2P_OpenEventAnnouncement(DWORD dwType, time_t tmStamp);
	void				EventAnnouncement_OnPlayerLogin(LPCHARACTER pkChr);

	// [PUBLIC] Trigger Functions
public:
	void				OnPlayerLogin(LPCHARACTER pkChr);
	void				OnPlayerLogout(LPCHARACTER pkChr);
	void				OnPlayerLoadAffect(LPCHARACTER pkChr);
	void				OnPlayerDead(LPCHARACTER pkChr);
	void				OnPlayerAnswer(LPCHARACTER pkChr, bool bAccept);

public:
	// [PUBLIC] Check Functions
	bool				CanUseItem(LPCHARACTER pkChr, LPITEM pkItem);
	bool				PVPTournament_CanUseItem(LPCHARACTER pkChr, LPITEM pkItem);

	// [PRIVATE] Packet Functions
private:
	void				EncodeEventRequestPacket(network::GCOutputPacket<network::GCEventRequestPacket>& rkPacket, LPCHARACTER pkChr);
	void				EncodeEventCancelPacket(network::GCOutputPacket<network::GCEventCancelPacket>& rkPacket);

	// [PRIVATE] Variables
private:
	DWORD					m_dwRunningEventIndex;
	TEventData				m_akEventData[EVENT_MAX_NUM];
	std::set<LPCHARACTER>	m_set_WFRPlayer; // wait for reply players
	std::set<DWORD>			m_set_IgnorePlayer;

	bool					m_bIsAnnouncementRunning;
	DWORD					m_dwEventAnnouncementType;
	time_t					m_tmEventAnnouncementStamp;

	// [PUBLIC] Event: EmpireWar Functions
public:
	void				EmpireWar_EncodeLoadPacket(network::GCOutputPacket<network::GCEventEmpireWarLoadPacket>& rkPacket, bool bIsNowStart = false);
	void				EmpireWar_SendUpdatePacket(BYTE bEmpire, WORD wKills, WORD wDeads);
	void				EmpireWar_SendFinishPacket();

	void				EmpireWar_OnPlayerLogin(LPCHARACTER pkChr);

	bool				EmpireWar_CanUseItem(LPITEM pkItem);
	
	
	
	// [PUBLIC] Event: Tag Team Functions
public:
	void				TagTeam_Register(LPCHARACTER pkChr1, LPCHARACTER pkChr2);
	void				P2P_TagTeam_Register(DWORD dwPID1, DWORD dwPID2, BYTE bGroupIdx);
	void				TagTeam_Unregister(LPCHARACTER pkChr);
	bool				TagTeam_IsRegistered(LPCHARACTER pkChr);
	bool				TagTeam_RemoveRegistration(LPCHARACTER pkChr, const char* szMessage = NULL);
	void				P2P_TagTeam_RemoveRegistration(DWORD dwPID1, DWORD dwPID2, BYTE bGroupIdx);

	const std::map<DWORD, DWORD>&	TagTeam_GetRegistrationMap(BYTE bGroupIdx) const { return m_map_TagTeamRegistrations[bGroupIdx]; }

	void				TagTeam_Create(const ::google::protobuf::RepeatedPtrField<TTagTeam>& teams);
	void				TagTeam_Destroy(CEventTagTeam* pkEventTagTeam);
	void				TagTeam_Shutdown();

	void				TagTeam_RemovePID(DWORD dwPID);

	void				TagTeam_OnPlayerLogin(LPCHARACTER pkChr);
	void				TagTeam_OnPlayerLogout(LPCHARACTER pkChr);
	void				TagTeam_OnPlayerLoadAffect(LPCHARACTER pkChr);
	void				TagTeam_OnPlayerDead(LPCHARACTER pkChr);

	bool				TagTeam_CanUseItem(CEventTagTeam* pkTagTeam, LPITEM pkItem);

	// [PRIVATE] Event: Tag Team Variables
private:
		std::map<DWORD, DWORD>			m_map_TagTeamRegistrations[2];

		std::set<CEventTagTeam*>		m_set_EventTagTeam;
		std::map<DWORD, CEventTagTeam*>	m_map_EventTagTeamByPID;


#ifdef ENABLE_REACT_EVENT
	// [PUBLIC] Event: React
public:
	int					React_Manager(BYTE idx);
	void				React_OnPlayerLogin(LPCHARACTER ch);
	void				React_OnPlayerLogout(LPCHARACTER ch);
	void				React_OnPlayerReact(LPCHARACTER ch, std::string answer);
	void				React_WarpLosers();
	bool				React_ResolveTask();
	void				React_SetTask();
	DWORD				React_GetParticipants() { return m_map_EventReactMembers.size(); };

	// [PRIVATE] Event: React
private:
	std::string							m_strAnswer;
	std::map<LPCHARACTER, SReactMember>	m_map_EventReactMembers;
	std::vector<LPCHARACTER>			m_vec_EventReactLosers;
#endif

#ifdef HALLOWEEN_MINIGAME
public:
	void				HalloweenMinigame_Initialize();
	void				HalloweenMinigame_StartRound(LPCHARACTER ch);
	typedef struct SHalloweenRewardData {
		DWORD	vnum;
		DWORD	count;
		BYTE	type;
		BYTE	chance;
	} THalloweenRewardData;

private:
	bool 		m_bHalloweenLoaded;
#endif


};
#endif

#endif

