#pragma once

#include "InstanceBase.h"

struct SNetworkActorData
{
	std::string m_stName;
	
	CAffectFlagContainer	m_kAffectFlags;

	BYTE	m_bType;
	DWORD	m_dwVID;
	DWORD	m_dwStateFlags;
	DWORD	m_dwEmpireID;
	DWORD	m_dwRace;
	DWORD	m_dwMovSpd;
	DWORD	m_dwAtkSpd;
	FLOAT	m_fRot;
	long	m_lCurX;
	long	m_lCurY;
	long	m_lSrcX;
	long	m_lSrcY;
	long	m_lDstX;
	long	m_lDstY;
	

	DWORD	m_dwServerSrcTime;
	DWORD	m_dwClientSrcTime;
	DWORD	m_dwDuration;

	DWORD	m_dwArmor;
	DWORD	m_dwWeapon;
	DWORD	m_dwHair;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	DWORD	m_dwAcce;
#endif
#ifdef ENABLE_ALPHA_EQUIP
	int		m_iWeaponAlphaEquip;
#endif

	DWORD	m_dwOwnerVID;

	int		m_iAlignment;
	BYTE	m_byPKMode;
	DWORD	m_dwMountVnum;
	short	m_sPVPTeam;

	DWORD	m_dwGuildID;
	DWORD	m_dwLevel;

#ifdef COMBAT_ZONE
	BYTE 	combat_zone_rank;
	DWORD	combat_zone_points;
#endif
	
#ifdef CHANGE_SKILL_COLOR
	DWORD	m_dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif

#ifdef __PRESTIGE__
	BYTE	m_bPrestigeLevel;
#endif

	SNetworkActorData();

	void SetDstPosition(DWORD dwServerTime, long lDstX, long lDstY, DWORD dwDuration);
	void SetPosition(long lPosX, long lPosY);
	void UpdatePosition();	

	// NETWORK_ACTOR_DATA_COPY
	SNetworkActorData(const SNetworkActorData& src);
	void operator=(const SNetworkActorData& src);
	void __copy__(const SNetworkActorData& src);	
	// END_OF_NETWORK_ACTOR_DATA_COPY
};

struct SNetworkMoveActorData
{
	DWORD	m_dwVID;
	DWORD	m_dwTime;
	long	m_lPosX;
	long	m_lPosY;
	float	m_fRot;
	DWORD	m_dwFunc;
	DWORD	m_dwArg;
	DWORD	m_dwDuration;

	SNetworkMoveActorData()
	{
		m_dwVID=0;
		m_dwTime=0;
		m_fRot=0.0f;
		m_lPosX=0;
		m_lPosY=0;
		m_dwFunc=0;
		m_dwArg=0;
		m_dwDuration=0;
	}
};

struct SNetworkUpdateActorData
{
	DWORD m_dwVID;
	DWORD m_dwGuildID;
	DWORD m_dwArmor;
	DWORD m_dwWeapon;
	DWORD m_dwHair;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	DWORD m_dwAcce;
#endif
#ifdef ENABLE_ALPHA_EQUIP
	int m_iWeaponAlphaEquip;
#endif
	DWORD m_dwMovSpd;
	DWORD m_dwAtkSpd;
	int m_iAlignment;
	BYTE m_byPKMode;
	DWORD m_dwMountVnum;
	DWORD m_dwStateFlags; // 본래 Create 때만 쓰이는 변수임
	CAffectFlagContainer m_kAffectFlags;

#ifdef COMBAT_ZONE
	DWORD combat_zone_points;
#endif

#ifdef CHANGE_SKILL_COLOR
	DWORD m_dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
#endif

#ifdef __PRESTIGE__
	BYTE m_bPrestigeLevel;
#endif

	SNetworkUpdateActorData()
	{
		m_dwGuildID=0;
		m_dwVID=0;
		m_dwArmor=0;
		m_dwWeapon=0;
		m_dwHair=0;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		m_dwAcce=0;
#endif
#ifdef ENABLE_ALPHA_EQUIP
		m_iWeaponAlphaEquip=0;
#endif
		m_dwMovSpd=0;
		m_dwAtkSpd=0;
		m_iAlignment=0;
		m_byPKMode=0;
		m_dwMountVnum=0;
		m_dwStateFlags=0;
		
#ifdef COMBAT_ZONE
		combat_zone_points = 0;
#endif

#ifdef CHANGE_SKILL_COLOR
		memset(m_dwSkillColor, 0, sizeof(m_dwSkillColor));
#endif

#ifdef __PRESTIGE__
		m_bPrestigeLevel=0;
#endif

		m_kAffectFlags.Clear();
	}
};

class CPythonCharacterManager;

class CNetworkActorManager : public CReferenceObject
{
	public:
		CNetworkActorManager();
		virtual ~CNetworkActorManager();

		void Destroy();

		void SetMainActorVID(DWORD dwVID);

		void RemoveActor(DWORD dwVID);
		void AppendActor(const SNetworkActorData& c_rkNetActorData);
		void UpdateActor(const SNetworkUpdateActorData& c_rkNetUpdateActorData);
		void MoveActor(const SNetworkMoveActorData& c_rkNetMoveActorData);

		void SyncActor(DWORD dwVID, long lPosX, long lPosY);
		void SetActorOwner(DWORD dwOwnerVID, DWORD dwVictimVID);
		void Update();

	protected:
		void __UpdateMainActor();
		bool __IsVisiblePos(long lPosX, long lPosY);
		bool __IsVisibleActor(const SNetworkActorData& c_rkNetActorData);
		bool __IsMainActorVID(DWORD dwVID);

		void __RemoveAllGroundItems();
		void __RemoveAllActors();
		void __RemoveDynamicActors();
		void __RemoveCharacterManagerActor(SNetworkActorData& rkNetActorData);

		SNetworkActorData* __FindActorData(DWORD dwVID);

		CInstanceBase* __AppendCharacterManagerActor(SNetworkActorData& rkNetActorData);
		CInstanceBase* __FindActor(SNetworkActorData& rkNetActorData);
		CInstanceBase* __FindActor(SNetworkActorData& rkNetActorData, long lDstX, long lDstY);

		CPythonCharacterManager& __GetCharacterManager();

	protected:
		DWORD m_dwMainVID;

		long m_lMainPosX;
		long m_lMainPosY;

		std::map<DWORD, SNetworkActorData> m_kNetActorDict;
};
