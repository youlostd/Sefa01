#include "StdAfx.h"
#include "NetworkActorManager.h"
#include "PythonCharacterManager.h"
#include "PythonItem.h"

#include "AbstractPlayer.h"

#ifdef ENABLE_AUCTION
#include "PythonAuction.h"
#endif

#ifdef HIDE_NPC_OPTION
#include "PythonSystem.h"

extern DWORD g_dwOwnedPetVID;
extern DWORD g_dwOwnedMountVID;
extern DWORD g_dwOwnedFakebuffVID;

enum EHideNPCTypes
{
#ifdef ENABLE_FAKEBUFF
	HIDE_NPC_TYPE_BUFFIS = 0,
#endif

	HIDE_NPC_TYPE_MOUNTS = 1,
	HIDE_NPC_TYPE_PETS = 2,
	HIDE_NPC_TYPE_SHOPS = 3,

	HIDE_NPC_TYPE_MAX_NUM
};
#endif

#ifdef ENABLE_SHOP_RANGE
DWORD nearShopCount = 0;
#endif

void SNetworkActorData::UpdatePosition()
{
	DWORD dwClientCurTime=ELTimer_GetMSec();
	DWORD dwElapsedTime=dwClientCurTime-m_dwClientSrcTime;

	if (dwElapsedTime<m_dwDuration)
	{
		float fRate=float(dwElapsedTime)/float(m_dwDuration);
		m_lCurX=long((m_lDstX-m_lSrcX)*fRate+m_lSrcX);
		m_lCurY=long((m_lDstY-m_lSrcY)*fRate+m_lSrcY);
	}
	else
	{
		m_lCurX=m_lDstX;
		m_lCurY=m_lDstY;
	}
}

void SNetworkActorData::SetDstPosition(DWORD dwServerTime, long lDstX, long lDstY, DWORD dwDuration)
{
	m_lSrcX=m_lCurX;
	m_lSrcY=m_lCurY;
	m_lDstX=lDstX;
	m_lDstY=lDstY;

	m_dwDuration=dwDuration;
	m_dwServerSrcTime=dwServerTime;
	m_dwClientSrcTime=ELTimer_GetMSec();		
}

void SNetworkActorData::SetPosition(long lPosX, long lPosY)
{
	m_lDstX=m_lSrcX=m_lCurX=lPosX;
	m_lDstY=m_lSrcY=m_lCurY=lPosY;
}

// NETWORK_ACTOR_DATA_COPY
SNetworkActorData::SNetworkActorData(const SNetworkActorData& src)
{
	__copy__(src);
}

void SNetworkActorData::operator=(const SNetworkActorData& src)
{
	__copy__(src);
}

void SNetworkActorData::__copy__(const SNetworkActorData& src)
{
	m_bType = src.m_bType;
	m_dwVID = src.m_dwVID;
	m_dwStateFlags = src.m_dwStateFlags;
	m_dwEmpireID = src.m_dwEmpireID;
	m_dwRace = src.m_dwRace;
	m_dwMovSpd = src.m_dwMovSpd;
	m_dwAtkSpd = src.m_dwAtkSpd;
	m_fRot = src.m_fRot;
	m_lCurX = src.m_lCurX;
	m_lCurY = src.m_lCurY;
	m_lSrcX = src.m_lSrcX;
	m_lSrcY = src.m_lSrcY;
	m_lDstX = src.m_lDstX;
	m_lDstY = src.m_lDstY;
	m_kAffectFlags.CopyInstance(src.m_kAffectFlags);
	
	m_dwServerSrcTime = src.m_dwServerSrcTime;
	m_dwClientSrcTime = src.m_dwClientSrcTime;
	m_dwDuration = src.m_dwDuration;

	m_dwArmor = src.m_dwArmor;
	m_dwWeapon = src.m_dwWeapon;
	m_dwHair = src.m_dwHair;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_dwAcce = src.m_dwAcce;
#endif
#ifdef ENABLE_ALPHA_EQUIP
	m_iWeaponAlphaEquip = src.m_iWeaponAlphaEquip;
#endif

	m_dwOwnerVID = src.m_dwOwnerVID;

	m_iAlignment = src.m_iAlignment;
	m_byPKMode = src.m_byPKMode;
	m_dwMountVnum = src.m_dwMountVnum;

	m_dwGuildID = src.m_dwGuildID;
	m_dwLevel = src.m_dwLevel;
	m_stName = src.m_stName;

#ifdef COMBAT_ZONE
	combat_zone_rank = src.combat_zone_rank;
	combat_zone_points = src.combat_zone_points;
#endif

#ifdef CHANGE_SKILL_COLOR
	memcpy(m_dwSkillColor, src.m_dwSkillColor, sizeof(m_dwSkillColor));
#endif
#ifdef __PRESTIGE__
	m_bPrestigeLevel = src.m_bPrestigeLevel;
#endif
}
// END_OF_NETWORK_ACTOR_DATA_COPY
	
SNetworkActorData::SNetworkActorData()
{
	SetPosition(0, 0);

	m_bType=0;
	m_dwVID=0;
	m_dwStateFlags=0;
	m_dwRace=0;
	m_dwMovSpd=0;
	m_dwAtkSpd=0;
	m_fRot=0.0f;
	m_dwArmor=0;
	m_dwWeapon=0;	
	m_dwHair=0;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_dwAcce=0;
#endif
	m_dwEmpireID=0;

	m_dwOwnerVID=0;

	m_dwDuration=0;
	m_dwClientSrcTime=0;
	m_dwServerSrcTime=0;

	m_iAlignment=0;
	m_byPKMode=0;
	m_dwMountVnum=0;

	m_stName="";

#ifdef COMBAT_ZONE
	combat_zone_rank = 0;
#endif

#ifdef CHANGE_SKILL_COLOR
	memset(m_dwSkillColor, 0, sizeof(m_dwSkillColor));
#endif
#ifdef __PRESTIGE__
	m_bPrestigeLevel=0;
#endif

	m_kAffectFlags.Clear();
}

////////////////////////////////////////////////////////////////////////////////

CNetworkActorManager::CNetworkActorManager()
{
	m_dwMainVID=0;
}

CNetworkActorManager::~CNetworkActorManager()
{
}

void CNetworkActorManager::Destroy()
{
	m_kNetActorDict.clear();
#ifdef ENABLE_SHOP_RANGE
	nearShopCount = 0;
#endif

	m_dwMainVID=0;
	m_lMainPosX=0;
	m_lMainPosY=0;
}

void CNetworkActorManager::SetMainActorVID(DWORD dwVID)
{
	m_dwMainVID=dwVID;
	m_lMainPosX=0;
	m_lMainPosY=0;

#ifdef ENABLE_SHOP_RANGE
	nearShopCount = 0;
#endif
	m_kNetActorDict.clear();
}

void CNetworkActorManager::Update()
{
	__UpdateMainActor();

	CPythonCharacterManager& rkChrMgr=__GetCharacterManager();

	std::map<DWORD, SNetworkActorData>::iterator i;
	for (i=m_kNetActorDict.begin(); i!=m_kNetActorDict.end(); ++i)
	{
		SNetworkActorData& rkNetActorData=i->second;
		rkNetActorData.UpdatePosition();

		CInstanceBase * pkInstFind = rkChrMgr.GetInstancePtr(rkNetActorData.m_dwVID);
		if (!pkInstFind)
		{
			if (__IsVisibleActor(rkNetActorData))
				__AppendCharacterManagerActor(rkNetActorData);
		}
#ifdef ENABLE_SHOP_RANGE
#ifdef ENABLE_AUCTION
		else if ((CPythonAuction::instance().is_shop_race(rkNetActorData.m_dwRace) || rkNetActorData.m_bType == CActorInstance::TYPE_PET || rkNetActorData.m_bType == CActorInstance::TYPE_MOUNT
#ifdef HIDE_NPC_OPTION
			|| (rkNetActorData.m_bType == CActorInstance::TYPE_FAKEBUFF)
#endif
			) && !__IsVisibleActor(rkNetActorData))
#else
		else if ((rkNetActorData.m_dwRace == 30000 || rkNetActorData.m_bType == CActorInstance::TYPE_PET || rkNetActorData.m_bType == CActorInstance::TYPE_MOUNT
#ifdef HIDE_NPC_OPTION
			|| (rkNetActorData.m_bType == CActorInstance::TYPE_FAKEBUFF)
#endif
			) && !__IsVisibleActor(rkNetActorData))
#endif
			__RemoveCharacterManagerActor(rkNetActorData);
#endif
	}
}

CInstanceBase* CNetworkActorManager::__FindActor(SNetworkActorData& rkNetActorData, long lDstX, long lDstY)
{
	CPythonCharacterManager& rkChrMgr=__GetCharacterManager();
	CInstanceBase * pkInstFind = rkChrMgr.GetInstancePtr(rkNetActorData.m_dwVID);
	if (!pkInstFind)
	{
		if (__IsVisiblePos(lDstX, lDstY))
			return __AppendCharacterManagerActor(rkNetActorData);

		return NULL;
	}

	return pkInstFind;
}

CInstanceBase* CNetworkActorManager::__FindActor(SNetworkActorData& rkNetActorData)
{
	CPythonCharacterManager& rkChrMgr=__GetCharacterManager();
	CInstanceBase * pkInstFind = rkChrMgr.GetInstancePtr(rkNetActorData.m_dwVID);
	if (!pkInstFind)
	{
		if (__IsVisibleActor(rkNetActorData))
			return __AppendCharacterManagerActor(rkNetActorData);

		return NULL;
	}

	return pkInstFind;
}

void CNetworkActorManager::__RemoveAllGroundItems()
{
	CPythonItem& rkItemMgr=CPythonItem::Instance();
	rkItemMgr.DeleteAllItems();
}

void CNetworkActorManager::__RemoveAllActors()
{
	m_kNetActorDict.clear();
#ifdef ENABLE_SHOP_RANGE
	nearShopCount = 0;
#endif

	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();
	rkChrMgr.DeleteAllInstances();
}

void CNetworkActorManager::__RemoveDynamicActors()
{
	//copy it
	std::vector<DWORD> dwCharacterVIDList;

	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	for(CPythonCharacterManager::CharacterIterator i = rkChrMgr.CharacterInstanceBegin(); i!=rkChrMgr.CharacterInstanceEnd(); ++i)
	{
		dwCharacterVIDList.push_back( (*i)->GetVirtualID() );
	}

	for( int i = 0; i < dwCharacterVIDList.size(); ++i )
	{
		CInstanceBase*  pkInstEach = rkChrMgr.GetInstancePtr( dwCharacterVIDList[i] );
		if( !pkInstEach )
			continue;
		
		CActorInstance* rkActorEach=pkInstEach->GetGraphicThingInstancePtr();
		if(rkActorEach->IsPC() || rkActorEach->IsNPC() || rkActorEach->IsEnemy()
			|| rkActorEach->IsStone()
#ifdef ENABLE_FAKEBUFF
			|| rkActorEach->IsFakeBuff()
#endif
		)
		{
			rkChrMgr.DeleteInstance(dwCharacterVIDList[i]);
			std::map<DWORD, SNetworkActorData>::iterator it =m_kNetActorDict.find(dwCharacterVIDList[i]);
			if (it != m_kNetActorDict.end())
			{
#ifdef ENABLE_SHOP_RANGE
#ifdef ENABLE_AUCTION
				if (CPythonAuction::instance().is_shop_race(it->second.m_dwRace))
#else
				if (it->second.m_dwRace == 30000)
#endif
					--nearShopCount;
#endif
				m_kNetActorDict.erase(it);
			}
		}
	}
						
	rkChrMgr.DestroyDeadInstanceList();
}

void CNetworkActorManager::__UpdateMainActor()
{
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();	
	CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
	if (!pkInstMain)
		return;

	TPixelPosition kPPosMain;
	pkInstMain->NEW_GetPixelPosition(&kPPosMain);

	m_lMainPosX=long(kPPosMain.x);
	m_lMainPosY=long(kPPosMain.y);
}

extern bool IsWall(unsigned race);

bool CNetworkActorManager::__IsVisibleActor(const SNetworkActorData& c_rkNetActorData)
{
	if (__IsMainActorVID(c_rkNetActorData.m_dwVID))
		return true;

	// 2004.08.02.myevan.항상 보이기 플래그
	if (c_rkNetActorData.m_kAffectFlags.IsSet(CInstanceBase::AFFECT_SHOW_ALWAYS))
		return true;

#ifdef HIDE_NPC_OPTION
	for (int i = 0; i < HIDE_NPC_TYPE_MAX_NUM; ++i)
	{
		bool hide = CPythonSystem::Instance().IsHiddenNPC(i);

		if (!hide)
			continue;

		CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();
		CInstanceBase* pkInstMain = rkChrMgr.GetMainInstancePtr();
		if (!pkInstMain)
			continue;

		std::string mainName = pkInstMain->GetNameString();

		switch (i)
		{
#ifdef ENABLE_FAKEBUFF
		case HIDE_NPC_TYPE_BUFFIS:
			if (c_rkNetActorData.m_bType == CActorInstance::TYPE_FAKEBUFF && c_rkNetActorData.m_stName.find(mainName) == string::npos && c_rkNetActorData.m_dwVID != g_dwOwnedFakebuffVID)
				return false;
			break;
#endif
		case HIDE_NPC_TYPE_MOUNTS:
			if (c_rkNetActorData.m_bType == CActorInstance::TYPE_MOUNT && c_rkNetActorData.m_stName.find(mainName) == string::npos && c_rkNetActorData.m_dwVID != g_dwOwnedMountVID)
				return false;
			break;
		case HIDE_NPC_TYPE_PETS:
			if (c_rkNetActorData.m_bType == CActorInstance::TYPE_PET && c_rkNetActorData.m_stName.find(mainName) == string::npos && c_rkNetActorData.m_dwVID != g_dwOwnedPetVID)
				return false;
			break;
		case HIDE_NPC_TYPE_SHOPS:
			if (CPythonAuction::instance().is_shop_race(c_rkNetActorData.m_bType) && c_rkNetActorData.m_stName.find(mainName) == string::npos)
				return false;
			break;
		}
	}
#endif

#ifdef ENABLE_SHOP_RANGE
#ifdef ENABLE_AUCTION
	if (CPythonAuction::instance().is_shop_race(c_rkNetActorData.m_dwRace) || c_rkNetActorData.m_bType == CActorInstance::TYPE_PET || c_rkNetActorData.m_bType == CActorInstance::TYPE_MOUNT)
	{
		if (CPythonAuction::instance().is_shop_race(c_rkNetActorData.m_dwRace) && nearShopCount <= 30)
			return true;
#else
	if (c_rkNetActorData.m_dwRace == 30000 || c_rkNetActorData.m_bType == CActorInstance::TYPE_PET || c_rkNetActorData.m_bType == CActorInstance::TYPE_MOUNT)
	{
		if (c_rkNetActorData.m_dwRace == 30000 && nearShopCount <= 30)
			return true;
#endif

		long dx = c_rkNetActorData.m_lCurX - m_lMainPosX;
		long dy = c_rkNetActorData.m_lCurY - m_lMainPosY;
		long len = (long)sqrt(double(dx * dx + dy * dy));
		extern long SHOP_SHOW_RANGE;
		
		if (!SHOP_SHOW_RANGE || len <= SHOP_SHOW_RANGE)
			return true;
	}
	else
#endif
		if (__IsVisiblePos(c_rkNetActorData.m_lCurX, c_rkNetActorData.m_lCurY))
			return true;

	if (IsWall(c_rkNetActorData.m_dwRace))
		return true;

 	return false;
}

bool CNetworkActorManager::__IsVisiblePos(long lPosX, long lPosY)
{
 	long dx = lPosX-m_lMainPosX;
	long dy = lPosY-m_lMainPosY;
	long len = (long)sqrt(double(dx * dx + dy * dy));

	extern int CHAR_STAGE_VIEW_BOUND;
	if (len < CHAR_STAGE_VIEW_BOUND) // 거리제한 cm
		return true;

	return false;
}

bool CNetworkActorManager::__IsMainActorVID(DWORD dwVID)
{
	if (dwVID == m_dwMainVID)
		return true;

	return false;
}

CPythonCharacterManager& CNetworkActorManager::__GetCharacterManager()
{
	return CPythonCharacterManager::Instance();
}

void CNetworkActorManager::__RemoveCharacterManagerActor(SNetworkActorData& rkNetActorData)
{
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();	
#ifdef ENABLE_AUCTION
	if (__IsMainActorVID(rkNetActorData.m_dwVID) || CPythonAuction::instance().is_shop_race(rkNetActorData.m_dwRace))
#else
	if (__IsMainActorVID(rkNetActorData.m_dwVID) || rkNetActorData.m_dwRace == 30000)
#endif
	{
		rkChrMgr.DeleteInstance(rkNetActorData.m_dwVID);
	}
	else
	{
		rkChrMgr.DeleteInstanceByFade(rkNetActorData.m_dwVID);	
	}	
}

CInstanceBase* CNetworkActorManager::__AppendCharacterManagerActor(SNetworkActorData& rkNetActorData)
{
#ifdef HIDE_NPC_OPTION
	for (int i = 0; i < HIDE_NPC_TYPE_MAX_NUM; ++i)
	{
		bool hide = CPythonSystem::Instance().IsHiddenNPC(i);

		if (!hide)
			continue;

		CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();
		CInstanceBase* pkInstMain = rkChrMgr.GetMainInstancePtr();
		if (!pkInstMain)
			continue;

		std::string mainName = pkInstMain->GetNameString();

		switch (i)
		{
#ifdef ENABLE_FAKEBUFF
		case HIDE_NPC_TYPE_BUFFIS:
			if (rkNetActorData.m_bType == CActorInstance::TYPE_FAKEBUFF && rkNetActorData.m_stName.find(mainName) == string::npos && rkNetActorData.m_dwVID != g_dwOwnedFakebuffVID)
				return NULL;
			break;
#endif
		case HIDE_NPC_TYPE_MOUNTS:
			if (rkNetActorData.m_bType == CActorInstance::TYPE_MOUNT && rkNetActorData.m_stName.find(mainName) == string::npos && rkNetActorData.m_dwVID != g_dwOwnedMountVID)
				return NULL;
			break;
		case HIDE_NPC_TYPE_PETS:
			if (rkNetActorData.m_bType == CActorInstance::TYPE_PET && rkNetActorData.m_stName.find(mainName) == string::npos && rkNetActorData.m_dwVID != g_dwOwnedPetVID)
				return NULL;
			break;
		case HIDE_NPC_TYPE_SHOPS:
			if (CPythonAuction::instance().is_shop_race(rkNetActorData.m_dwRace) && rkNetActorData.m_stName.find(mainName) == string::npos)
				return NULL;
			break;
		}
	}
#endif

	Lognf(0, "CNetworkActorManager::__AppendCharacterManagerActor(%d, %s)", rkNetActorData.m_dwVID, rkNetActorData.m_stName.c_str());

	DWORD dwVID=rkNetActorData.m_dwVID;
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();
	CInstanceBase::SCreateData kCreateData;
	kCreateData.m_bType=rkNetActorData.m_bType;
	kCreateData.m_dwLevel=rkNetActorData.m_dwLevel;
	kCreateData.m_dwGuildID=rkNetActorData.m_dwGuildID;
	kCreateData.m_dwEmpireID=rkNetActorData.m_dwEmpireID;
	kCreateData.m_dwVID=rkNetActorData.m_dwVID;
	kCreateData.m_dwMountVnum=rkNetActorData.m_dwMountVnum;
	kCreateData.m_dwRace=rkNetActorData.m_dwRace;
	kCreateData.m_fRot=rkNetActorData.m_fRot;
 	kCreateData.m_lPosX=rkNetActorData.m_lCurX;
	kCreateData.m_lPosY=rkNetActorData.m_lCurY;
	kCreateData.m_stName=rkNetActorData.m_stName;
	kCreateData.m_dwStateFlags=rkNetActorData.m_dwStateFlags;
	kCreateData.m_dwMovSpd=rkNetActorData.m_dwMovSpd;
	kCreateData.m_dwAtkSpd=rkNetActorData.m_dwAtkSpd;
	kCreateData.m_iAlignment=rkNetActorData.m_iAlignment;
	kCreateData.m_byPKMode=rkNetActorData.m_byPKMode;
	kCreateData.m_kAffectFlags=rkNetActorData.m_kAffectFlags;
	kCreateData.m_dwArmor=rkNetActorData.m_dwArmor;
	kCreateData.m_dwWeapon=rkNetActorData.m_dwWeapon;
	kCreateData.m_dwHair=rkNetActorData.m_dwHair;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	kCreateData.m_dwAcce=rkNetActorData.m_dwAcce;
#endif
#ifdef ENABLE_ALPHA_EQUIP
	kCreateData.m_iWeaponAlphaEquip=rkNetActorData.m_iWeaponAlphaEquip;
#endif
	kCreateData.m_isMain=__IsMainActorVID(dwVID);

#ifdef COMBAT_ZONE
	kCreateData.combat_zone_rank = rkNetActorData.combat_zone_rank;
	kCreateData.combat_zone_points = rkNetActorData.combat_zone_points;
#endif

#ifdef CHANGE_SKILL_COLOR
	memcpy(kCreateData.m_dwSkillColor, rkNetActorData.m_dwSkillColor, sizeof(kCreateData.m_dwSkillColor));
#endif

#ifdef __PRESTIGE__
	kCreateData.m_bPrestigeLevel = rkNetActorData.m_bPrestigeLevel;
#endif

	CInstanceBase* pOldInstance = rkChrMgr.GetInstancePtr(dwVID);
	if (pOldInstance)
	{
		TPixelPosition oldPos;
		pOldInstance->GetGraphicThingInstancePtr()->GetPixelPosition( &oldPos );

		bool bIsMountingHorse = pOldInstance->IsMountingHorse();
		if( (bIsMountingHorse && kCreateData.m_dwMountVnum == 0) ||  
			(!bIsMountingHorse && kCreateData.m_dwMountVnum != 0) )
		{
			//Override Pos
			kCreateData.m_lPosX = oldPos.x;
			kCreateData.m_lPosY = -oldPos.y;
		}

		rkChrMgr.DeleteInstance(dwVID);
	}

	CInstanceBase* pNewInstance = rkChrMgr.CreateInstance(kCreateData);
	if (!pNewInstance)
		return NULL;

	if (kCreateData.m_isMain)
	{
		IAbstractPlayer & rkPlayer = IAbstractPlayer::GetSingleton();
		rkPlayer.SetMainCharacterIndex(dwVID);
		m_lMainPosX=rkNetActorData.m_lCurX;
		m_lMainPosY=rkNetActorData.m_lCurY;
	}

	DWORD dwClientCurTime=ELTimer_GetMSec();
	DWORD dwElapsedTime=dwClientCurTime-rkNetActorData.m_dwClientSrcTime;

	if (dwElapsedTime<rkNetActorData.m_dwDuration)
	{
		TPixelPosition kPPosDst;
		kPPosDst.x=float(rkNetActorData.m_lDstX);
		kPPosDst.y=float(rkNetActorData.m_lDstY);
		kPPosDst.z=0;
		pNewInstance->PushTCPState(rkNetActorData.m_dwServerSrcTime+dwElapsedTime, kPPosDst, rkNetActorData.m_fRot, CInstanceBase::FUNC_MOVE, 0);		
	}
	return pNewInstance;
}

void CNetworkActorManager::AppendActor(const SNetworkActorData& c_rkNetActorData)
{
	if (__IsMainActorVID(c_rkNetActorData.m_dwVID))
	{
		bool bChangeMountStatus = false;

		CInstanceBase* pMainInstance = CPythonCharacterManager::Instance().GetInstancePtr(c_rkNetActorData.m_dwVID);
		if( pMainInstance )
		{
			bool bIsMountingHorse = pMainInstance->IsMountingHorse();
			bChangeMountStatus = (bIsMountingHorse && c_rkNetActorData.m_dwMountVnum == 0) || (!bIsMountingHorse && c_rkNetActorData.m_dwMountVnum != 0);
#ifdef CHANGE_SKILL_COLOR
			pMainInstance->ChangeSkillColor(*c_rkNetActorData.m_dwSkillColor);
#endif
		}

		if( !bChangeMountStatus )
		{
			//__RemoveAllActors();
			__RemoveDynamicActors();
			__RemoveAllGroundItems();
		}

	}

	SNetworkActorData& rkNetActorData=m_kNetActorDict[c_rkNetActorData.m_dwVID];
	rkNetActorData=c_rkNetActorData;
#ifdef ENABLE_SHOP_RANGE
#ifdef ENABLE_AUCTION
	if (CPythonAuction::instance().is_shop_race(c_rkNetActorData.m_dwRace))
#else
	if (c_rkNetActorData.m_dwRace == 30000)
#endif
		++nearShopCount;
#endif

	if (__IsVisibleActor(rkNetActorData))
	{
		if (!__AppendCharacterManagerActor(rkNetActorData))
		{
#ifdef ENABLE_SHOP_RANGE
#ifdef ENABLE_AUCTION
			if (CPythonAuction::instance().is_shop_race(c_rkNetActorData.m_dwRace))
#else
			if (c_rkNetActorData.m_dwRace == 30000)
#endif
				--nearShopCount;
#endif
			m_kNetActorDict.erase(c_rkNetActorData.m_dwVID);
		}
	}
}

void CNetworkActorManager::RemoveActor(DWORD dwVID)
{
	std::map<DWORD, SNetworkActorData>::iterator f=m_kNetActorDict.find(dwVID);
	if (m_kNetActorDict.end()==f)
	{
#ifdef _DEBUG		
		TraceError("CNetworkActorManager::RemoveActor(dwVID=%d) - NOT EXIST VID", dwVID);
#endif
		return;
	}

	SNetworkActorData& rkNetActorData=f->second;
	__RemoveCharacterManagerActor(rkNetActorData);

#ifdef ENABLE_SHOP_RANGE
#ifdef ENABLE_AUCTION
	if (CPythonAuction::instance().is_shop_race(f->second.m_dwRace))
#else
	if (f->second.m_dwRace == 30000)
#endif
		--nearShopCount;
#endif

	m_kNetActorDict.erase(f);
}

void CNetworkActorManager::UpdateActor(const SNetworkUpdateActorData& c_rkNetUpdateActorData)
{
	std::map<DWORD, SNetworkActorData>::iterator f=m_kNetActorDict.find(c_rkNetUpdateActorData.m_dwVID);
	if (m_kNetActorDict.end()==f)
	{
#ifdef _DEBUG
		TraceError("CNetworkActorManager::UpdateActor(dwVID=%d) - NOT EXIST VID", c_rkNetUpdateActorData.m_dwVID);
#endif
		return;
	}

	SNetworkActorData& rkNetActorData=f->second;
	CInstanceBase* pkInstFind=__FindActor(rkNetActorData);
	if (pkInstFind)
	{
		pkInstFind->ChangeArmor(c_rkNetUpdateActorData.m_dwArmor);		
#ifdef ENABLE_ALPHA_EQUIP
		pkInstFind->ChangeWeapon(c_rkNetUpdateActorData.m_dwWeapon, c_rkNetUpdateActorData.m_iWeaponAlphaEquip);
#else
		pkInstFind->ChangeWeapon(c_rkNetUpdateActorData.m_dwWeapon);
#endif
		pkInstFind->ChangeHair(c_rkNetUpdateActorData.m_dwHair);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		pkInstFind->ChangeAcce(c_rkNetUpdateActorData.m_dwAcce);
#endif
		pkInstFind->ChangeGuild(c_rkNetUpdateActorData.m_dwGuildID);
		pkInstFind->SetAffectFlagContainer(c_rkNetUpdateActorData.m_kAffectFlags);
		pkInstFind->SetMoveSpeed(c_rkNetUpdateActorData.m_dwMovSpd);
		pkInstFind->SetAttackSpeed(c_rkNetUpdateActorData.m_dwAtkSpd);
		pkInstFind->SetAlignment(c_rkNetUpdateActorData.m_iAlignment);
		pkInstFind->SetPKMode(c_rkNetUpdateActorData.m_byPKMode);
		pkInstFind->SetStateFlags(c_rkNetUpdateActorData.m_dwStateFlags);
#ifdef COMBAT_ZONE
		pkInstFind->SetCombatZonePoints(c_rkNetUpdateActorData.combat_zone_points);
#endif
#ifdef CHANGE_SKILL_COLOR
		pkInstFind->ChangeSkillColor(*c_rkNetUpdateActorData.m_dwSkillColor);
#endif

#ifdef __PRESTIGE__
		pkInstFind->SetPrestigeLevel(c_rkNetUpdateActorData.m_bPrestigeLevel);
#endif
		
		//if( c_rkNetUpdateActorData.m_dwMountVnum != 0 )
		//{
		//	pkInstFind->MountHorse(c_rkNetUpdateActorData.m_dwMountVnum);
		//}
		//else
		//{
		//	if( pkInstFind->IsMountingHorse() )
		//	{
		//		pkInstFind->DismountHorse();
		//	}
		//}
	}

	rkNetActorData.m_kAffectFlags=c_rkNetUpdateActorData.m_kAffectFlags;
	rkNetActorData.m_dwGuildID=c_rkNetUpdateActorData.m_dwGuildID;
	//rkNetActorData.m_dwLevel=c_rkNetUpdateActorData.m_dwLevel;
	rkNetActorData.m_dwMovSpd=c_rkNetUpdateActorData.m_dwMovSpd;
	rkNetActorData.m_dwAtkSpd=c_rkNetUpdateActorData.m_dwAtkSpd;
	rkNetActorData.m_dwArmor=c_rkNetUpdateActorData.m_dwArmor;
	rkNetActorData.m_dwWeapon=c_rkNetUpdateActorData.m_dwWeapon;
	rkNetActorData.m_dwHair=c_rkNetUpdateActorData.m_dwHair;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	rkNetActorData.m_dwAcce=c_rkNetUpdateActorData.m_dwAcce;
#endif
#ifdef ENABLE_ALPHA_EQUIP
	rkNetActorData.m_iWeaponAlphaEquip=c_rkNetUpdateActorData.m_iWeaponAlphaEquip;
#endif
	rkNetActorData.m_iAlignment=c_rkNetUpdateActorData.m_iAlignment;
	rkNetActorData.m_byPKMode=c_rkNetUpdateActorData.m_byPKMode;
#ifdef COMBAT_ZONE
	rkNetActorData.combat_zone_points = c_rkNetUpdateActorData.combat_zone_points;
#endif
#ifdef CHANGE_SKILL_COLOR
	memcpy(rkNetActorData.m_dwSkillColor, c_rkNetUpdateActorData.m_dwSkillColor, sizeof(rkNetActorData.m_dwSkillColor));
#endif
#ifdef __PRESTIGE__
	rkNetActorData.m_bPrestigeLevel=c_rkNetUpdateActorData.m_bPrestigeLevel;
#endif
}

void CNetworkActorManager::MoveActor(const SNetworkMoveActorData& c_rkNetMoveActorData)
{
	std::map<DWORD, SNetworkActorData>::iterator f=m_kNetActorDict.find(c_rkNetMoveActorData.m_dwVID);
	if (m_kNetActorDict.end()==f)
	{
#ifdef _DEBUG
		TraceError("CNetworkActorManager::MoveActor(dwVID=%d) - NOT EXIST VID", c_rkNetMoveActorData.m_dwVID);
#endif
		return;
	}

	SNetworkActorData& rkNetActorData=f->second;
	CInstanceBase* pkInstFind=__FindActor(rkNetActorData, c_rkNetMoveActorData.m_lPosX, c_rkNetMoveActorData.m_lPosY);
	if (pkInstFind)
	{
		TPixelPosition kPPosDst;
		kPPosDst.x=float(c_rkNetMoveActorData.m_lPosX);
		kPPosDst.y=float(c_rkNetMoveActorData.m_lPosY);
		kPPosDst.z=0.0f;

		pkInstFind->PushTCPState(c_rkNetMoveActorData.m_dwTime, kPPosDst, 
			c_rkNetMoveActorData.m_fRot, c_rkNetMoveActorData.m_dwFunc, c_rkNetMoveActorData.m_dwArg);	
	}

	rkNetActorData.SetDstPosition(c_rkNetMoveActorData.m_dwTime,
		c_rkNetMoveActorData.m_lPosX, c_rkNetMoveActorData.m_lPosY, c_rkNetMoveActorData.m_dwDuration);
	rkNetActorData.m_fRot=c_rkNetMoveActorData.m_fRot;		
}

void CNetworkActorManager::SyncActor(DWORD dwVID, long lPosX, long lPosY)
{
	std::map<DWORD, SNetworkActorData>::iterator f=m_kNetActorDict.find(dwVID);
	if (m_kNetActorDict.end()==f)
	{
#ifdef _DEBUG
		TraceError("CNetworkActorManager::SyncActor(dwVID=%d) - NOT EXIST VID", dwVID);
#endif
		return;
	}

	SNetworkActorData& rkNetActorData=f->second;
	CInstanceBase* pkInstFind=__FindActor(rkNetActorData);
	if (pkInstFind)
	{
		pkInstFind->NEW_SyncPixelPosition(lPosX, lPosY);			
	}
	
	rkNetActorData.SetPosition(lPosX, lPosY);	
}

void CNetworkActorManager::SetActorOwner(DWORD dwOwnerVID, DWORD dwVictimVID)
{
	std::map<DWORD, SNetworkActorData>::iterator f=m_kNetActorDict.find(dwVictimVID);
	if (m_kNetActorDict.end()==f)
	{
#ifdef _DEBUG
		TraceError("CNetworkActorManager::SetActorOwner(dwOwnerVID=%d, dwVictimVID=%d) - NOT EXIST VID", dwOwnerVID, dwVictimVID);
#endif
		return;
	}

	SNetworkActorData& rkNetActorData=f->second;
	rkNetActorData.m_dwOwnerVID=dwOwnerVID;	

	CInstanceBase* pkInstFind=__FindActor(rkNetActorData);
	if (pkInstFind)
	{
		pkInstFind->NEW_SetOwner(rkNetActorData.m_dwOwnerVID);	
	}
		
	
}
