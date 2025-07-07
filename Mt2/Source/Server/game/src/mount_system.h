#ifndef __MOUNT_SYSTEM_H__
#define __MOUNT_SYSTEM_H__

#include "stdafx.h"

class CMountSystem;

EVENTINFO(mount_system_event_info)
{
	CMountSystem* pkMountSystem;
};

enum EMountState
{
	MOUNT_NONE,
	MOUNT_SUMMONED,
	MOUNT_RIDING,
};

class CMountSystem
{
public:
	CMountSystem(LPCHARACTER pkOwner);
	~CMountSystem();
	void Initialize();

	void		StartCheckItemEvent();
	void		StopCheckItemEvent();
	void		SetCheckItemEvent(LPEVENT pkEvent) { m_pkCheckItemEvent = pkEvent; }

	bool		IsNoSummonMap() const;
	bool		IsSummoned() const;
	bool		Summon(LPITEM pkItem);
	void		Unsummon(bool bDisable = true);
	void		OnDestroySummoned();

	bool		IsRiding() const;
	bool		StartRiding();
	void		StopRiding(bool bSummon = true);

	void		GiveBuff(bool bAdd = true, LPCHARACTER pkChr = NULL, bool bIsDoubled = false);

	LPCHARACTER	GetOwner() const			{ return m_pkOwner; }
	LPCHARACTER	GetMount() const			{ return m_pkMount; }

	void		SetSummonItemID(DWORD dwID)	{ m_dwItemID = dwID; }
	DWORD		GetSummonItemID() const		{ return m_dwItemID; }
	BYTE		GetState() const			{ return m_bState; }

	void		SetName(const char* szName)	{ m_stName = szName; }
	const char*	GetDefaultName(const char* szPlayerName = NULL) const;
	const char*	GetName() const;

	void		RefreshMountBuffBonus();
	void		RefreshBonusLevel(LPITEM pkItem);

	bool		CanRefine(BYTE bRefineIndex);
	void		SendRefineInfo(BYTE bRefineIndex);
	void		DoRefine(BYTE bRefineIndex);

private:
	LPCHARACTER		m_pkOwner;
	LPCHARACTER		m_pkMount;
	LPEVENT			m_pkCheckItemEvent;

	DWORD			m_dwItemID;
	BYTE			m_bState;
	std::string		m_stName;

	bool			m_bIsStartRiding;

	float			m_fMountBuffBonus;
	DWORD			m_dwMeltMountVnum;

	BYTE			m_abBonusLevel[HORSE_BONUS_MAX_COUNT];
};

#endif
