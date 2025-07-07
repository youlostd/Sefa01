#ifndef	__HEADER_PET_SYSTEM__
#define	__HEADER_PET_SYSTEM__


class CHARACTER;

// TODO: ÆêÀ¸·Î¼­ÀÇ ´É·ÂÄ¡? ¶ó´ø°¡ Ä£¹Ðµµ, ¹è°íÇÄ ±âÅ¸µîµî... ¼öÄ¡
struct SPetAbility
{
};

/**
*/
class CPetActor //: public CHARACTER
{
public:
	enum EPetOptions
	{
		EPetOption_Followable		= 1 << 0,
		EPetOption_Mountable		= 1 << 1,
		EPetOption_Summonable		= 1 << 2,
		EPetOption_Combatable		= 1 << 3,		
	};


protected:
	friend class CPetSystem;

	CPetActor(LPCHARACTER owner, DWORD vnum, DWORD options = EPetOption_Followable | EPetOption_Summonable);
//	CPetActor(LPCHARACTER owner, DWORD vnum, const SPetAbility& petAbility, DWORD options = EPetOption_Followable | EPetOption_Summonable);

	virtual ~CPetActor();

	virtual bool	Update(DWORD deltaTime);

protected:
	virtual bool	_UpdateFollowAI();				///< ÁÖÀÎÀ» µû¶ó´Ù´Ï´Â AI Ã³¸®
	virtual bool	_UpdatAloneActionAI(float fMinDist, float fMaxDist);			///< ÁÖÀÎ ±ÙÃ³¿¡¼­ È¥ÀÚ ³ë´Â AI Ã³¸®

	/// @TODO
	//virtual bool	_UpdateCombatAI();

private:
	bool Follow(float fMinDistance = 50.f);

public:
	LPCHARACTER		GetCharacter()	const					{ return m_pkChar; }
	LPCHARACTER		GetOwner()	const						{ return m_pkOwner; }
	DWORD			GetVID() const							{ return m_dwVID; }
	DWORD			GetVnum() const							{ return m_dwVnum; }

	bool			HasOption(EPetOptions option) const		{ return m_dwOptions & option; }

	void			SetName(const char* petName);

	DWORD			Summon(const char* petName, LPITEM pSummonItem, bool bSpawnFar = false);
	void			Unsummon();
	void			OnUnsummon();

	bool			IsSummoned() const			{ return 0 != m_pkChar; }
	void			SetSummonItem (LPITEM pItem);
	DWORD			GetSummonItemVID () { return m_dwSummonItemVID; }
	// ¹öÇÁ ÁÖ´Â ÇÔ¼ö¿Í °ÅµÎ´Â ÇÔ¼ö.
	// ÀÌ°Ô Á» ±«¶öÇÑ°Ô, ¼­¹ö°¡ ¤´¶ó¼­, 
	// POINT_MOV_SPEED, POINT_ATT_SPEED, POINT_CAST_SPEED´Â PointChange()¶õ ÇÔ¼ö¸¸ ½á¼­ º¯°æÇØ ºÁ¾ß ¼Ò¿ëÀÌ ¾ø´Â°Ô,
	// PointChange() ÀÌÈÄ¿¡ ¾îµð¼±°¡ ComputePoints()¸¦ ÇÏ¸é ½Ï´Ù ÃÊ±âÈ­µÇ°í, 
	// ´õ ¿ô±ä°Ç, ComputePoints()¸¦ ºÎ¸£Áö ¾ÊÀ¸¸é Å¬¶óÀÇ POINT´Â ÀüÇô º¯ÇÏÁö ¾Ê´Â´Ù´Â °Å´Ù.
	// ±×·¡¼­ ¹öÇÁ¸¦ ÁÖ´Â °ÍÀº ComputePoints() ³»ºÎ¿¡¼­ petsystem->RefreshBuff()¸¦ ºÎ¸£µµ·Ï ÇÏ¿´°í,
	// ¹öÇÁ¸¦ »©´Â °ÍÀº ClearBuff()¸¦ ºÎ¸£°í, ComputePoints¸¦ ÇÏ´Â °ÍÀ¸·Î ÇÑ´Ù.
	void			GiveBuff(LPCHARACTER pkTarget = NULL);
	void			ClearBuff();

private:
	DWORD			m_dwVnum;
	DWORD			m_dwVID;
	DWORD			m_dwOptions;
	DWORD			m_dwLastActionTime;
	DWORD			m_dwSummonItemVID;
	DWORD			m_dwSummonItemVnum;

	short			m_originalMoveSpeed;

	std::string		m_name;

	LPCHARACTER		m_pkChar;					// Instance of pet(CHARACTER)
	LPCHARACTER		m_pkOwner;

//	SPetAbility		m_petAbility;				// ´É·ÂÄ¡
};

/**
*/
class CPetSystem
{
public:
	typedef	std::unordered_map<DWORD,	CPetActor*>		TPetActorMap;		/// <VNUM, PetActor> map. (ÇÑ Ä³¸¯ÅÍ°¡ °°Àº vnumÀÇ ÆêÀ» ¿©·¯°³ °¡Áú ÀÏÀÌ ÀÖÀ»±î..??)

public:
	CPetSystem(LPCHARACTER owner);
	virtual ~CPetSystem();

	CPetActor*	GetByVID(DWORD vid) const;
	CPetActor*	GetByVnum(DWORD vnum) const;

	bool		Update(DWORD deltaTime);
	void		Destroy();

	size_t		CountSummoned() const;			///< ÇöÀç ¼ÒÈ¯µÈ(½ÇÃ¼È­ µÈ Ä³¸¯ÅÍ°¡ ÀÖ´Â) ÆêÀÇ °³¼ö
	CPetActor*	GetSummoned() const;
	void		ShowPets(long lMapIndex, long x, long y, long z, bool bShowSpawnMotion);

public:
	void		SetUpdatePeriod(DWORD ms);

	CPetActor*	Summon(DWORD mobVnum, LPITEM pSummonItem, const char* petName, bool bSpawnFar, DWORD options = CPetActor::EPetOption_Followable | CPetActor::EPetOption_Summonable);

	void		Unsummon(DWORD mobVnum, bool bDeleteFromList = false);
	void		Unsummon(CPetActor* petActor, bool bDeleteFromList = false);

	// TODO: ÁøÂ¥ Æê ½Ã½ºÅÛÀÌ µé¾î°¥ ¶§ ±¸Çö. (Ä³¸¯ÅÍ°¡ º¸À¯ÇÑ ÆêÀÇ Á¤º¸¸¦ Ãß°¡ÇÒ ¶§ ¶ó´ø°¡...)
	CPetActor*	AddPet(DWORD mobVnum, const char* petName, const SPetAbility& ability, DWORD options = CPetActor::EPetOption_Followable | CPetActor::EPetOption_Summonable | CPetActor::EPetOption_Combatable);

	void		DeletePet(DWORD mobVnum);
	void		DeletePet(CPetActor* petActor);
	void		RefreshBuff(LPCHARACTER pkTarget = NULL);

#ifdef ENABLE_COMPANION_NAME
	void		OnNameChange();
#endif

private:
	TPetActorMap	m_petActorMap;
	LPCHARACTER		m_pkOwner;					///< Æê ½Ã½ºÅÛÀÇ Owner
	DWORD			m_dwUpdatePeriod;			///< ¾÷µ¥ÀÌÆ® ÁÖ±â (ms´ÜÀ§)
	DWORD			m_dwLastUpdateTime;
	LPEVENT			m_pkPetSystemUpdateEvent;
};

/**
// Summon Pet
CPetSystem* petSystem = mainChar->GetPetSystem();
CPetActor* petActor = petSystem->Summon(~~~);

DWORD petVID = petActor->GetVID();
if (0 == petActor)
{
	ERROR_LOG(...)
};


// Unsummon Pet
petSystem->Unsummon(petVID);

// Mount Pet
petActor->Mount()..


CPetActor::Update(...)
{
	// AI : Follow, actions, etc...
}

*/



#endif	//__HEADER_PET_SYSTEM__