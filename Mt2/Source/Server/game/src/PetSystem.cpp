#include "stdafx.h"

#ifdef __PET_SYSTEM__
#include "utils.h"
#include "vector.h"
#include "char.h"
#include "sectree_manager.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "PetSystem.h"
#include "../../common/VnumHelper.h"
#include "packet.h"
#include "item_manager.h"
#include "item.h"
#include "desc.h"

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

extern int passes_per_sec;
EVENTINFO(petsystem_event_info)
{
	CPetSystem* pPetSystem;
};

// PetSystemÀ» update ÇØÁÖ´Â event.
// PetSystemÀº CHRACTER_MANAGER¿¡¼­ ±âÁ¸ FSMÀ¸·Î update ÇØÁÖ´Â ±âÁ¸ chracters¿Í ´Þ¸®,
// OwnerÀÇ STATE¸¦ update ÇÒ ¶§ _UpdateFollowAI ÇÔ¼ö·Î update ÇØÁØ´Ù.
// ±×·±µ¥ ownerÀÇ state¸¦ update¸¦ CHRACTER_MANAGER¿¡¼­ ÇØÁÖ±â ¶§¹®¿¡,
// petsystemÀ» updateÇÏ´Ù°¡ petÀ» unsummonÇÏ´Â ºÎºÐ¿¡¼­ ¹®Á¦°¡ »ý°å´Ù.
// (CHRACTER_MANAGER¿¡¼­ update ÇÏ¸é chracter destroy°¡ pendingµÇ¾î, CPetSystem¿¡¼­´Â dangling Æ÷ÀÎÅÍ¸¦ °¡Áö°í ÀÖ°Ô µÈ´Ù.)
// µû¶ó¼­ PetSystem¸¸ ¾÷µ¥ÀÌÆ® ÇØÁÖ´Â event¸¦ ¹ß»ý½ÃÅ´.
EVENTFUNC(petsystem_update_event)
{
	petsystem_event_info* info = dynamic_cast<petsystem_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "check_speedhack_event> <Factor> Null pointer" );
		return 0;
	}

	CPetSystem*	pPetSystem = info->pPetSystem;

	if (NULL == pPetSystem)
		return 0;

	
	pPetSystem->Update(0);
	// 0.25ÃÊ¸¶´Ù °»½Å.
	return PASSES_PER_SEC(1) / 4;
}

/// NOTE: 1Ä³¸¯ÅÍ°¡ ¸î°³ÀÇ ÆêÀ» °¡Áú ¼ö ÀÖ´ÂÁö Á¦ÇÑ... Ä³¸¯ÅÍ¸¶´Ù °³¼ö¸¦ ´Ù¸£°Ô ÇÒ°Å¶ó¸é º¯¼ö·Î ³Öµî°¡... À½..
/// °¡Áú ¼ö ÀÖ´Â °³¼ö¿Í µ¿½Ã¿¡ ¼ÒÈ¯ÇÒ ¼ö ÀÖ´Â °³¼ö°¡ Æ²¸± ¼ö ÀÖ´Âµ¥ ÀÌ·±°Ç ±âÈ¹ ¾øÀ¸´Ï ÀÏ´Ü ¹«½Ã
const float PET_COUNT_LIMIT = 3;

///////////////////////////////////////////////////////////////////////////////////////
//  CPetActor
///////////////////////////////////////////////////////////////////////////////////////

CPetActor::CPetActor(LPCHARACTER owner, DWORD vnum, DWORD options)
{
	owner->tchat("pet vnum: %d", vnum);

	m_dwVnum = vnum;
	m_dwVID = 0;
	m_dwOptions = options;
	m_dwLastActionTime = 0;

	m_pkChar = 0;
	m_pkOwner = owner;

	m_originalMoveSpeed = 0;
	
	m_dwSummonItemVID = 0;
	m_dwSummonItemVnum = 0;
}

CPetActor::~CPetActor()
{
	this->Unsummon();

	m_pkOwner = 0;
}

void CPetActor::SetName(const char* name)
{

	std::string petName = m_pkOwner->GetName();

/* 	if (0 != m_pkOwner && 
		0 == name && 
		0 != m_pkOwner->GetName())
	{
		petName += LC_TEXT(m_pkOwner, "'s Pet");
	}
	else
	{
		petName += "'s ";
		petName += name;
	} */

#ifdef ENABLE_COMPANION_NAME
	std::string customName = m_pkOwner->GetPetName();
	if (IsSummoned())
	{
		m_pkChar->SetName(!customName.empty() ? customName : petName);
		m_pkChar->SetCompanionHasName(!customName.empty());
	}
	m_name = !customName.empty() ? customName : petName;
#else
	if (true == IsSummoned())
		m_pkChar->SetName(petName);

	m_name = petName;
#endif
}

void CPetActor::OnUnsummon()
{
	if (true == this->IsSummoned())
	{
		this->ClearBuff();
		this->SetSummonItem(NULL);

		if (m_pkOwner)
			m_pkOwner->CheckMaximumPoints();
	}

	m_pkChar = 0;
	m_dwVID = 0;
}

void CPetActor::Unsummon()
{
	if (true == this->IsSummoned())
	{
#ifdef __ANIMAL_SYSTEM__
		LPITEM pkSummonItem = ITEM_MANAGER::instance().FindByVID(GetSummonItemVID());
		if (pkSummonItem)
			pkSummonItem->Animal_UnsummonPacket();
		else
		{
			network::GCOutputPacket<network::GCAnimalUnsummonPacket> pack;
			pack->set_type(ANIMAL_TYPE_PET);
			GetOwner()->GetDesc()->Packet(pack);
		}
#endif

		if (NULL != m_pkChar)
			M2_DESTROY_CHARACTER(m_pkChar);
	}

#ifdef ENABLE_COMPANION_NAME
	if (m_pkOwner)
		m_pkOwner->ComputePoints();
#endif
}

DWORD CPetActor::Summon(const char* petName, LPITEM pSummonItem, bool bSpawnFar)
{
	if (test_server)
		sys_log(0, "PetActor::Summon(%s) owner %s", petName ? petName : "<no-name>", GetOwner()->GetName());

	long x = m_pkOwner->GetX();
	long y = m_pkOwner->GetY();
	long z = m_pkOwner->GetZ();

	if (GetOwner()->GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You cannot summon your mount in this map."));
		return false;
	}

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetOwner()->GetMapIndex()))
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You cannot summon your mount in this map."));
		return false;
	}
#endif

#ifdef ENABLE_REACT_EVENT
	if (GetOwner()->GetMapIndex() == REACT_EVENT_MAP)
	{
		GetOwner()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(m_pkOwner, "You cannot summon your mount in this map."));
		return false;
	}
#endif
	
	if (true == bSpawnFar)
	{
		x += (random_number(0, 1) * 2 - 1) * random_number(2000, 2500);
		y += (random_number(0, 1) * 2 - 1) * random_number(2000, 2500);
	}
	else
	{
		x += random_number(-100, 100);
		y += random_number(-100, 100);
	}

	if (0 != m_pkChar)
	{
		m_pkChar->Show (m_pkOwner->GetMapIndex(), x, y);
		m_dwVID = m_pkChar->GetVID();

		return m_dwVID;
	}
	
	m_pkChar = CHARACTER_MANAGER::instance().SpawnMob(
				m_dwVnum, 
				m_pkOwner->GetMapIndex(), 
				x, y, z,
				false, (int)(m_pkOwner->GetRotation()+180), false);

	if (0 == m_pkChar)
	{
		sys_err("[CPetSystem::Summon] Failed to summon the pet. (vnum: %d)", m_dwVnum);
		return 0;
	}

	m_pkChar->SetPet(this);

//	m_pkOwner->DetailLog();
//	m_pkChar->DetailLog();

	//ÆêÀÇ ±¹°¡¸¦ ÁÖÀÎÀÇ ±¹°¡·Î ¼³Á¤ÇÔ.
	m_pkChar->SetEmpire(m_pkOwner->GetEmpire());

	m_dwVID = m_pkChar->GetVID();

	this->SetName(petName);

	// SetSummonItem(pSummonItem)¸¦ ºÎ¸¥ ÈÄ¿¡ ComputePoints¸¦ ºÎ¸£¸é ¹öÇÁ Àû¿ëµÊ.
	this->SetSummonItem(pSummonItem);
	//m_pkOwner->ComputePoints();
	m_pkChar->Show(m_pkOwner->GetMapIndex(), x, y, z);

#ifdef __ANIMAL_SYSTEM__
	pSummonItem->Animal_SummonPacket(petName);
#endif

	GiveBuff();

#ifdef ENABLE_COMPANION_NAME
	m_pkOwner->ComputePoints();
	if (!m_pkOwner->GetPetName().empty())
		m_pkOwner->ChatPacket(CHAT_TYPE_COMMAND, "SetOwnedPetVID %u", m_dwVID);
#endif

	return m_dwVID;
}

bool CPetActor::_UpdatAloneActionAI(float fMinDist, float fMaxDist)
{
	float fDist = random_number(fMinDist, fMaxDist);
	float r = (float)random_number (0, 359);
	float dest_x = GetOwner()->GetX() + fDist * cos(r);
	float dest_y = GetOwner()->GetY() + fDist * sin(r);

	//m_pkChar->SetRotation(random_number(0, 359));		// ¹æÇâÀº ·£´ýÀ¸·Î ¼³Á¤

	//GetDeltaByDegree(m_pkChar->GetRotation(), fDist, &fx, &fy);

	// ´À½¼ÇÑ ¸ø°¨ ¼Ó¼º Ã¼Å©; ÃÖÁ¾ À§Ä¡¿Í Áß°£ À§Ä¡°¡ °¥¼ö¾ø´Ù¸é °¡Áö ¾Ê´Â´Ù.
	//if (!(SECTREE_MANAGER::instance().IsMovablePosition(m_pkChar->GetMapIndex(), m_pkChar->GetX() + (int) fx, m_pkChar->GetY() + (int) fy) 
	//			&& SECTREE_MANAGER::instance().IsMovablePosition(m_pkChar->GetMapIndex(), m_pkChar->GetX() + (int) fx/2, m_pkChar->GetY() + (int) fy/2)))
	//	return true;

	m_pkChar->SetNowWalking(true);

	//if (m_pkChar->Goto(m_pkChar->GetX() + (int) fx, m_pkChar->GetY() + (int) fy))
	//	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	if (!m_pkChar->IsStateMove() && m_pkChar->Goto(dest_x, dest_y))
		m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	m_dwLastActionTime = get_dword_time();

	return true;
}

// char_state.cpp StateHorseÇÔ¼ö ±×³É C&P -_-;
bool CPetActor::_UpdateFollowAI()
{
	if (0 == m_pkChar->m_pkMobData)
	{
		sys_err("[CPetActor::_UpdateFollowAI] m_pkChar->m_pkMobData is NULL");
		return false;
	}
	
	// NOTE: Ä³¸¯ÅÍ(Æê)ÀÇ ¿ø·¡ ÀÌµ¿ ¼Óµµ¸¦ ¾Ë¾Æ¾ß ÇÏ´Âµ¥, ÇØ´ç °ª(m_pkChar->m_pkMobData->m_table.sMovingSpeed)À» Á÷Á¢ÀûÀ¸·Î Á¢±ÙÇØ¼­ ¾Ë¾Æ³¾ ¼öµµ ÀÖÁö¸¸
	// m_pkChar->m_pkMobData °ªÀÌ invalidÇÑ °æ¿ì°¡ ÀÚÁÖ ¹ß»ýÇÔ. ÇöÀç ½Ã°£°ü°è»ó ¿øÀÎÀº ´ÙÀ½¿¡ ÆÄ¾ÇÇÏ°í ÀÏ´ÜÀº m_pkChar->m_pkMobData °ªÀ» ¾Æ¿¹ »ç¿ëÇÏÁö ¾Êµµ·Ï ÇÔ.
	// ¿©±â¼­ ¸Å¹ø °Ë»çÇÏ´Â ÀÌÀ¯´Â ÃÖÃÊ ÃÊ±âÈ­ ÇÒ ¶§ Á¤»ó °ªÀ» Á¦´ë·Î ¸ø¾ò¾î¿À´Â °æ¿ìµµ ÀÖÀ½.. -_-;; ¤Ð¤Ð¤Ð¤Ð¤Ð¤Ð¤Ð¤Ð¤Ð
	if (0 == m_originalMoveSpeed)
	{
		const CMob* mobData = CMobManager::Instance().Get(m_dwVnum);

		if (0 != mobData)
			m_originalMoveSpeed = mobData->m_table.moving_speed();
	}
	float	START_FOLLOW_DISTANCE = 300.0f;		// ÀÌ °Å¸® ÀÌ»ó ¶³¾îÁö¸é ÂÑ¾Æ°¡±â ½ÃÀÛÇÔ
	float	START_RUN_DISTANCE = 900.0f;		// ÀÌ °Å¸® ÀÌ»ó ¶³¾îÁö¸é ¶Ù¾î¼­ ÂÑ¾Æ°¨.

	float	RESPAWN_DISTANCE = 4500.f;			// ÀÌ °Å¸® ÀÌ»ó ¸Ö¾îÁö¸é ÁÖÀÎ ¿·À¸·Î ¼ÒÈ¯ÇÔ.
	int		APPROACH = 200;						// Á¢±Ù °Å¸®

	bool bDoMoveAlone = true;					// Ä³¸¯ÅÍ¿Í °¡±îÀÌ ÀÖÀ» ¶§ È¥ÀÚ ¿©±âÀú±â ¿òÁ÷ÀÏ°ÇÁö ¿©ºÎ -_-;
	bool bRun = false;							// ¶Ù¾î¾ß ÇÏ³ª?

	DWORD currentTime = get_dword_time();

	long ownerX = m_pkOwner->GetX();		long ownerY = m_pkOwner->GetY();
	long charX = m_pkChar->GetX();			long charY = m_pkChar->GetY();

	float fDist = DISTANCE_APPROX(charX - ownerX, charY - ownerY);

	if (fDist >= RESPAWN_DISTANCE)
	{
		float fOwnerRot = m_pkOwner->GetRotation() * 3.141592f / 180.f;
		float fx = -APPROACH * cos(fOwnerRot);
		float fy = -APPROACH * sin(fOwnerRot);
		if (m_pkChar->Show(m_pkOwner->GetMapIndex(), ownerX + fx, ownerY + fy))
		{
			return true;
		}
	}
	
	
	if (fDist >= START_FOLLOW_DISTANCE)
	{
		if( fDist >= START_RUN_DISTANCE)
		{
			bRun = true;
		}

		m_pkChar->SetNowWalking(!bRun);		// NOTE: ÇÔ¼ö ÀÌ¸§º¸°í ¸ØÃß´Â°ÇÁÙ ¾Ë¾Ò´Âµ¥ SetNowWalking(false) ÇÏ¸é ¶Ù´Â°ÅÀÓ.. -_-;
		
		Follow(APPROACH);

		m_pkChar->SetLastAttacked(currentTime);
		m_dwLastActionTime = currentTime;
	}
	else
	{
		if (fabs(m_pkChar->GetRotation() - GetDegreeFromPositionXY(charX, charY, ownerX, ownerX)) > 10.f || fabs(m_pkChar->GetRotation() - GetDegreeFromPositionXY(charX, charY, ownerX, ownerX)) < 350.f)
		{
			m_pkChar->Follow(m_pkOwner, APPROACH);
			m_pkChar->SetLastAttacked(currentTime);
			m_dwLastActionTime = currentTime;
		}
		else if (currentTime - m_dwLastActionTime > random_number(5000, 12000))
		{
			this->_UpdatAloneActionAI(START_FOLLOW_DISTANCE / 2, START_FOLLOW_DISTANCE);
		}
	}
	// Follow ÁßÀÌÁö¸¸ ÁÖÀÎ°ú ÀÏÁ¤ °Å¸® ÀÌ³»·Î °¡±î¿öÁ³´Ù¸é ¸ØÃã
	//else 
	//	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	return true;
}

bool CPetActor::Update(DWORD deltaTime)
{
	bool bResult = true;

	// Æê ÁÖÀÎÀÌ Á×¾ú°Å³ª, ¼ÒÈ¯µÈ ÆêÀÇ »óÅÂ°¡ ÀÌ»óÇÏ´Ù¸é ÆêÀ» ¾ø¾Ú. (NOTE: °¡²û°¡´Ù ÀÌ·± Àú·± ÀÌÀ¯·Î ¼ÒÈ¯µÈ ÆêÀÌ DEAD »óÅÂ¿¡ ºüÁö´Â °æ¿ì°¡ ÀÖÀ½-_-;)
	// ÆêÀ» ¼ÒÈ¯ÇÑ ¾ÆÀÌÅÛÀÌ ¾ø°Å³ª, ³»°¡ °¡Áø »óÅÂ°¡ ¾Æ´Ï¶ó¸é ÆêÀ» ¾ø¾Ú.
	if (/*m_pkOwner->IsDead() ||*/ (IsSummoned() && m_pkChar->IsDead()) 
		|| NULL == ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())
		|| ITEM_MANAGER::instance().FindByVID(this->GetSummonItemVID())->GetOwner() != this->GetOwner()
		)
	{
		this->Unsummon();
		return true;
	}

	if (this->IsSummoned() && HasOption(EPetOption_Followable))
		bResult = bResult && this->_UpdateFollowAI();

	return bResult;
}

//NOTE : ÁÖÀÇ!!! MinDistance¸¦ Å©°Ô ÀâÀ¸¸é ±× º¯À§¸¸Å­ÀÇ º¯È­µ¿¾ÈÀº followÇÏÁö ¾Ê´Â´Ù,
bool CPetActor::Follow(float fMinDistance)
{
	// °¡·Á´Â À§Ä¡¸¦ ¹Ù¶óºÁ¾ß ÇÑ´Ù.
	if( !m_pkOwner || !m_pkChar) 
		return false;

	
	float fOwnerX = m_pkOwner->GetX();
	float fOwnerY = m_pkOwner->GetY();

	float fPetX = m_pkChar->GetX();
	float fPetY = m_pkChar->GetY();

	float fDist = DISTANCE_SQRT(fOwnerX - fPetX, fOwnerY - fPetY);
	// m_pkOwner->tchat("Follow... fMinDistance %f  fDist %f", fMinDistance, fDist);
	
	if (fDist <= fMinDistance)
		return false;

	m_pkChar->SetRotationToXY(fOwnerX, fOwnerY);

	float fx, fy;

	float fDistToGo = fDist - fMinDistance;
	GetDeltaByDegree(m_pkChar->GetRotation(), fDistToGo, &fx, &fy);
	
	// m_pkOwner->tchat("Follow... fMinDistance %f  fDist %f fDistToGo %f", fMinDistance, fDist, fDistToGo);
	if (!m_pkChar->Goto((int)(fPetX+fx+0.5f), (int)(fPetY+fy+0.5f)) )
		return false;

	// m_pkOwner->tchat("Follow... fMinDistance %f  fDist %f fDistToGo %f DONE", fMinDistance, fDist, fDistToGo);
	
	m_pkChar->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0, 0);

	return true;
}

void CPetActor::SetSummonItem(LPITEM pItem)
{
	if (NULL == pItem)
	{
		LPITEM pSummonItem = ITEM_MANAGER::instance().FindByVID(m_dwSummonItemVID);
		if (NULL != pSummonItem)
			pSummonItem->SetSocket(1, false);
		
		m_dwSummonItemVID = 0;
		m_dwSummonItemVnum = 0;
		return;
	}

	pItem->SetSocket(1, true);
	m_dwSummonItemVID = pItem->GetVID();
	m_dwSummonItemVnum = pItem->GetVnum();
}

void CPetActor::GiveBuff(LPCHARACTER pkTarget)
{
	if (!pkTarget)
		pkTarget = m_pkOwner;

	LPITEM item = ITEM_MANAGER::instance().FindByVID(m_dwSummonItemVID);
	if (NULL != item && item->GetOwner() == GetOwner())
		item->ModifyPoints(true);
	return ;
}

void CPetActor::ClearBuff()
{
	if (NULL == m_pkOwner)
		return ;
	LPITEM pkItem = ITEM_MANAGER::instance().FindByVID(m_dwSummonItemVID);
	if (NULL == pkItem || pkItem->GetOwner() != GetOwner())
	{
		GetOwner()->ComputePoints();
		return;
	}
	pkItem->ModifyPoints(false);

	return ;
}

///////////////////////////////////////////////////////////////////////////////////////
//  CPetSystem
///////////////////////////////////////////////////////////////////////////////////////

CPetSystem::CPetSystem(LPCHARACTER owner)
{
//	assert(0 != owner && "[CPetSystem::CPetSystem] Invalid owner");

	m_pkOwner = owner;
	m_dwUpdatePeriod = 400;

	m_dwLastUpdateTime = 0;
}

CPetSystem::~CPetSystem()
{
	Destroy();
}

void CPetSystem::Destroy()
{
	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (petActor)
			delete petActor;
	}

	if (m_pkPetSystemUpdateEvent)
	{
		event_cancel(&m_pkPetSystemUpdateEvent); 
		m_pkPetSystemUpdateEvent = NULL;
	}

	m_petActorMap.clear();
}

/// Æê ½Ã½ºÅÛ ¾÷µ¥ÀÌÆ®. µî·ÏµÈ ÆêµéÀÇ AI Ã³¸® µîÀ» ÇÔ.
bool CPetSystem::Update(DWORD deltaTime)
{
	bool bResult = true;

	DWORD currentTime = get_dword_time();

	// CHARACTER_MANAGER¿¡¼­ Ä³¸¯ÅÍ·ù UpdateÇÒ ¶§ ¸Å°³º¯¼ö·Î ÁÖ´Â (Pulse¶ó°í µÇ¾îÀÖ´Â)°ªÀÌ ÀÌÀü ÇÁ·¹ÀÓ°úÀÇ ½Ã°£Â÷ÀÌÀÎÁÙ ¾Ë¾Ò´Âµ¥
	// ÀüÇô ´Ù¸¥ °ªÀÌ¶ó¼­-_-; ¿©±â¿¡ ÀÔ·ÂÀ¸·Î µé¾î¿À´Â deltaTimeÀº ÀÇ¹Ì°¡ ¾øÀ½¤Ð¤Ð	
	
	if (m_dwUpdatePeriod > currentTime - m_dwLastUpdateTime)
		return true;
	
	std::vector <CPetActor*> v_garbageActor;

	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor && petActor->IsSummoned())
		{
			LPCHARACTER pPet = petActor->GetCharacter();
			
			if (NULL == CHARACTER_MANAGER::instance().Find(pPet->GetVID()))
			{
				v_garbageActor.push_back(petActor);
			}
			else
			{
				bResult = bResult && petActor->Update(deltaTime);
			}
		}
	}
	for (std::vector<CPetActor*>::iterator it = v_garbageActor.begin(); it != v_garbageActor.end(); it++)
		DeletePet(*it);

	m_dwLastUpdateTime = currentTime;

	return bResult;
}

/// °ü¸® ¸ñ·Ï¿¡¼­ ÆêÀ» Áö¿ò
void CPetSystem::DeletePet(DWORD mobVnum)
{
	TPetActorMap::iterator iter = m_petActorMap.find(mobVnum);

	if (m_petActorMap.end() == iter)
	{
		sys_err("[CPetSystem::DeletePet] Can't find pet on my list (VNUM: %d)", mobVnum);
		return;
	}

	CPetActor* petActor = iter->second;

	if (0 == petActor)
		sys_err("[CPetSystem::DeletePet] Null Pointer (petActor)");
	else
		delete petActor;

	m_petActorMap.erase(iter);	
}

/// °ü¸® ¸ñ·Ï¿¡¼­ ÆêÀ» Áö¿ò
void CPetSystem::DeletePet(CPetActor* petActor)
{
	for (TPetActorMap::iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		if (iter->second == petActor)
		{
			delete petActor;
			m_petActorMap.erase(iter);

			return;
		}
	}

	sys_err("[CPetSystem::DeletePet] Can't find petActor(0x%x) on my list(size: %d) ", petActor, m_petActorMap.size());
}

void CPetSystem::Unsummon(DWORD vnum, bool bDeleteFromList)
{
	CPetActor* actor = this->GetByVnum(vnum);

	if (0 == actor)
	{
		sys_err("[CPetSystem::GetByVnum(%d)] Null Pointer (petActor)", vnum);
		return;
	}
	actor->Unsummon();

	if (true == bDeleteFromList)
		this->DeletePet(actor);

	bool bActive = false;
	for (TPetActorMap::iterator it = m_petActorMap.begin(); it != m_petActorMap.end(); it++)
	{
		bActive |= it->second->IsSummoned();
	}
	if (false == bActive && m_pkPetSystemUpdateEvent)
	{
		event_cancel(&m_pkPetSystemUpdateEvent);
		m_pkPetSystemUpdateEvent = NULL;
	}
}


CPetActor* CPetSystem::Summon(DWORD mobVnum, LPITEM pSummonItem, const char* petName, bool bSpawnFar, DWORD options)
{
#ifdef __SKIN_SYSTEM__
	if(m_pkOwner && m_pkOwner->GetWear(SKINSYSTEM_SLOT_PET))
	{
		LPITEM pPetSkin = m_pkOwner->GetWear(SKINSYSTEM_SLOT_PET);
		DWORD skinVnum = pPetSkin->GetValue(0);
		
		if(skinVnum)
			mobVnum = skinVnum;
	}
#endif

	CPetActor* petActor = this->GetByVnum(mobVnum);

	// µî·ÏµÈ ÆêÀÌ ¾Æ´Ï¶ó¸é »õ·Î »ý¼º ÈÄ °ü¸® ¸ñ·Ï¿¡ µî·ÏÇÔ.
	if (0 == petActor)
	{
		petActor = M2_NEW CPetActor(m_pkOwner, mobVnum, options);
		m_petActorMap.insert(std::make_pair(mobVnum, petActor));
	}

	DWORD petVID = petActor->Summon(petName, pSummonItem, bSpawnFar);

	if (NULL == m_pkPetSystemUpdateEvent)
	{
		petsystem_event_info* info = AllocEventInfo<petsystem_event_info>();

		info->pPetSystem = this;

		m_pkPetSystemUpdateEvent = event_create(petsystem_update_event, info, PASSES_PER_SEC(1) / 4);	// 0.25ÃÊ	
	}
	
	return petActor;
}


CPetActor* CPetSystem::GetByVID(DWORD vid) const
{
	CPetActor* petActor = 0;

	bool bFound = false;

	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		petActor = iter->second;

		if (0 == petActor)
		{
			sys_err("[CPetSystem::GetByVID(%d)] Null Pointer (petActor)", vid);
			continue;
		}

		bFound = petActor->GetVID() == vid;

		if (true == bFound)
			break;
	}

	return bFound ? petActor : 0;
}

/// µî·Ï µÈ Æê Áß¿¡¼­ ÁÖ¾îÁø ¸÷ VNUMÀ» °¡Áø ¾×ÅÍ¸¦ ¹ÝÈ¯ÇÏ´Â ÇÔ¼ö.
CPetActor* CPetSystem::GetByVnum(DWORD vnum) const
{
	CPetActor* petActor = 0;

	TPetActorMap::const_iterator iter = m_petActorMap.find(vnum);

	if (m_petActorMap.end() != iter)
		petActor = iter->second;

	return petActor;
}

size_t CPetSystem::CountSummoned() const
{
	size_t count = 0;

	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
				++count;
		}
	}

	return count;
}

void CPetSystem::ShowPets(long lMapIndex, long x, long y, long z, bool bShowSpawnMotion)
{
	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned() && petActor->GetCharacter())
				petActor->GetCharacter()->Show(lMapIndex, x, y, z, bShowSpawnMotion);
		}
	}
}

CPetActor* CPetSystem::GetSummoned() const
{
	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
				return petActor;
		}
	}

	return NULL;
}

void CPetSystem::RefreshBuff(LPCHARACTER pkTarget)
{
	for (TPetActorMap::const_iterator iter = m_petActorMap.begin(); iter != m_petActorMap.end(); ++iter)
	{
		CPetActor* petActor = iter->second;

		if (0 != petActor)
		{
			if (petActor->IsSummoned())
			{
				petActor->GiveBuff(pkTarget);
			}
		}
	}
}

#ifdef ENABLE_COMPANION_NAME
void CPetSystem::OnNameChange()
{
	for (auto &it : m_petActorMap)
	{
		if (!it.second)
			continue;

		LPCHARACTER owner = it.second->GetOwner();
		if (owner)
			it.second->SetName(owner->GetPetName().c_str());



		LPCHARACTER ch = it.second->GetCharacter();
		if (!ch)
			return;

		ch->ViewReencode();
	}
}
#endif

#endif
