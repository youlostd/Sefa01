#include "StdAfx.h"
#include "PythonPlayerEventHandler.h"
#include "PythonApplication.h"
#include "PythonItem.h"
#include "../eterbase/Timer.h"
#include "../eterbase/Timer.h"
#include "../EterPack/EterPackManager.h"
#include "test.h"
#include "AbstractPlayer.h"

const DWORD POINT_MAGIC_NUMBER = 0xe73ac1da;

bool CPythonPlayer::LoadHorseBonusProto(const char* c_pszFileName)
{
	const VOID* pvData;
	CMappedFile kFile;
	if (!CEterPackManager::Instance().Get(kFile, c_pszFileName, &pvData))
	{
		Tracenf("CPythonPet::LoadBonusProto(c_szFileName=%s) - Load Error", c_pszFileName);
		return false;
	}

	CMemoryTextFileLoader kTextFileLoader;
	kTextFileLoader.Bind(kFile.Size(), pvData);

	CTokenVector kTokenVector;
	for (DWORD i = 0; i < kTextFileLoader.GetLineCount(); ++i)
	{
		if (!kTextFileLoader.SplitLineByTab(i, &kTokenVector))
			continue;

		if (kTokenVector.size() != 8)
		{
			TraceError("CPythonPet::LoadBonusProto: StrangeLine in %d (tokenCount %d)", i + 1, kTokenVector.size());
			continue;
		}

		const std::string& c_rstLevel = kTokenVector[0];
		const std::string& c_rstApplyValue1 = kTokenVector[1];
		const std::string& c_rstItemVnum1 = kTokenVector[2];
		const std::string& c_rstApplyValue2 = kTokenVector[3];
		const std::string& c_rstItemVnum2 = kTokenVector[4];
		const std::string& c_rstApplyValue3 = kTokenVector[5];
		const std::string& c_rstItemVnum3 = kTokenVector[6];
		const std::string& c_rstItemCount = kTokenVector[7];

		BYTE bLevel = atoi(c_rstLevel.c_str());
		if (!bLevel || bLevel > HORSE_MAX_BONUS_LEVEL)
		{
			TraceError("CPythonPet::LoadBonusProto: StrangeLine in %d (level %u invalid)", i + 1, bLevel);
			continue;
		}

		THorseBonusProtoTable& rkTab = m_aBonusProto[bLevel - 1];
		rkTab.bApplyType[0] = CItemData::APPLY_MAX_HP;
		rkTab.dwApplyValue[0] = atoi(c_rstApplyValue1.c_str());
		rkTab.dwItemVnum[0] = atoi(c_rstItemVnum1.c_str());
		rkTab.bApplyType[1] = CItemData::APPLY_DEF_GRADE_BONUS;
		rkTab.dwApplyValue[1] = atoi(c_rstApplyValue2.c_str());
		rkTab.dwItemVnum[1] = atoi(c_rstItemVnum2.c_str());
		rkTab.bApplyType[2] = CItemData::APPLY_ATT_BONUS_TO_MONSTER;
		rkTab.dwApplyValue[2] = atoi(c_rstApplyValue3.c_str());
		rkTab.dwItemVnum[2] = atoi(c_rstItemVnum3.c_str());
		rkTab.bItemCount = atoi(c_rstItemCount.c_str());
	}

	return true;
}

CPythonPlayer::THorseBonusProtoTable* CPythonPlayer::GetHorseBonusProto(BYTE bLevel)
{
	if (bLevel >= HORSE_MAX_BONUS_LEVEL)
		return NULL;
	
	return &m_aBonusProto[bLevel];
}

void CPythonPlayer::SPlayerStatus::SetPoint(UINT ePoint, LONGLONG lPoint)
{
	m_allPoint[ePoint]=lPoint ^ POINT_MAGIC_NUMBER;
}

LONGLONG CPythonPlayer::SPlayerStatus::GetPoint(UINT ePoint)
{
	return m_allPoint[ePoint] ^ POINT_MAGIC_NUMBER;
}

void CPythonPlayer::SPlayerStatus::SetRealPoint(UINT ePoint, LONGLONG llPoint)
{
	m_allRealPoint[ePoint] = llPoint;// ^ POINT_MAGIC_NUMBER;
}

LONGLONG CPythonPlayer::SPlayerStatus::GetRealPoint(UINT ePoint)
{
	return m_allRealPoint[ePoint];// ^ POINT_MAGIC_NUMBER;
}

bool CPythonPlayer::AffectIndexToSkillIndex(DWORD dwAffectIndex, DWORD * pdwSkillIndex)
{
	if (m_kMap_dwAffectIndexToSkillIndex.end() == m_kMap_dwAffectIndexToSkillIndex.find(dwAffectIndex))
		return false;

	*pdwSkillIndex = m_kMap_dwAffectIndexToSkillIndex[dwAffectIndex];
	return true;
}

bool CPythonPlayer::SkillIndexToAffectIndex(DWORD dwSkillIndex, DWORD * pdwAffectIndex, int iIgnoreCount)
{
	int i = 0;
	for (auto it = m_kMap_dwAffectIndexToSkillIndex.begin(); it != m_kMap_dwAffectIndexToSkillIndex.end(); ++it)
	{
		if (it->second == dwSkillIndex)
		{
			if (iIgnoreCount > i++)
				continue;

			*pdwAffectIndex = it->first;
			return true;
		}
	}

	return false;
}

bool CPythonPlayer::AffectIndexToSkillSlotIndex(UINT uAffect, DWORD* pdwSkillSlotIndex)
{
	DWORD dwSkillIndex=m_kMap_dwAffectIndexToSkillIndex[uAffect];

	return GetSkillSlotIndex(dwSkillIndex, pdwSkillSlotIndex);
}

bool CPythonPlayer::__GetPickedActorPtr(CInstanceBase** ppkInstPicked)
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase* pkInstPicked=rkChrMgr.OLD_GetPickedInstancePtr();
	if (!pkInstPicked)
		return false;

	*ppkInstPicked=pkInstPicked;
	return true;
}

bool CPythonPlayer::__GetPickedActorID(DWORD* pdwActorID)
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	return rkChrMgr.OLD_GetPickedInstanceVID(pdwActorID);
}

bool CPythonPlayer::__GetPickedItemID(DWORD* pdwItemID)
{
	CPythonItem& rkItemMgr=CPythonItem::Instance();
	return rkItemMgr.GetPickedItemID(pdwItemID);
}

bool CPythonPlayer::__GetPickedGroundPos(TPixelPosition* pkPPosPicked)
{
	CPythonBackground& rkBG=CPythonBackground::Instance();

	TPixelPosition kPPosPicked;
	if (rkBG.GetPickingPoint(pkPPosPicked))
	{
		pkPPosPicked->y=-pkPPosPicked->y;
		return true;
	}

	return false;
}

void CPythonPlayer::NEW_GetMainActorPosition(TPixelPosition* pkPPosActor)
{
	TPixelPosition kPPosMainActor;

	IAbstractPlayer& rkPlayer=IAbstractPlayer::GetSingleton();
	CInstanceBase * pInstance = rkPlayer.NEW_GetMainActorPtr();
	if (pInstance)
	{
		pInstance->NEW_GetPixelPosition(pkPPosActor);
	}
	else
	{
		CPythonApplication::Instance().GetCenterPosition(pkPPosActor);
	}
}



bool CPythonPlayer::RegisterEffect(DWORD dwEID, const char* c_szFileName, bool isCache)
{
	if (dwEID>=EFFECT_NUM)
		return false;

	CEffectManager& rkEftMgr=CEffectManager::Instance();
	rkEftMgr.RegisterEffect2(c_szFileName, &m_adwEffect[dwEID], isCache);
	return true;
}

void CPythonPlayer::NEW_ShowEffect(int dwEID, TPixelPosition kPPosDst)
{
	if (dwEID>=EFFECT_NUM)
		return;

	D3DXVECTOR3 kD3DVt3Pos(kPPosDst.x, -kPPosDst.y, kPPosDst.z);
	D3DXVECTOR3 kD3DVt3Dir(0.0f, 0.0f, 1.0f);

	CEffectManager& rkEftMgr=CEffectManager::Instance();
	rkEftMgr.CreateEffect(m_adwEffect[dwEID], kD3DVt3Pos, kD3DVt3Dir);
}

CInstanceBase* CPythonPlayer::NEW_FindActorPtr(DWORD dwVID)
{
	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	return rkChrMgr.GetInstancePtr(dwVID);
}

CInstanceBase* CPythonPlayer::NEW_GetMainActorPtr()
{
	return NEW_FindActorPtr(m_dwMainCharacterIndex);
}

///////////////////////////////////////////////////////////////////////////////////////////

void CPythonPlayer::Update()
{
	NEW_RefreshMouseWalkingDirection();

	CPythonPlayerEventHandler& rkPlayerEventHandler=CPythonPlayerEventHandler::GetSingleton();
	rkPlayerEventHandler.FlushVictimList();

	if (m_isDestPosition)
	{
		CInstanceBase * pInstance = NEW_GetMainActorPtr();
		if (pInstance)
		{
			TPixelPosition PixelPosition;
			pInstance->NEW_GetPixelPosition(&PixelPosition);

			if (abs(int(PixelPosition.x) - m_ixDestPos) + abs(int(PixelPosition.y) - m_iyDestPos) < 10000)
			{
				m_isDestPosition = FALSE;
			}
			else
			{
				if (CTimer::Instance().GetCurrentMillisecond() - m_iLastAlarmTime > 20000)
				{
					AlarmHaveToGo();
				}
			}
		}
	}

	if (m_isConsumingStamina)
	{
		float fElapsedTime = CTimer::Instance().GetElapsedSecond();
		m_fCurrentStamina -= (fElapsedTime * m_fConsumeStaminaPerSec);

		SetStatus(POINT_STAMINA, DWORD(m_fCurrentStamina));

		PyCallClassMemberFunc(m_ppyGameWindow, "RefreshStamina", Py_BuildValue("()"));
	}

	__Update_AutoAttack();
	__Update_NotifyGuildAreaEvent();
}

bool CPythonPlayer::__IsUsingChargeSkill()
{
	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain)
		return false;

	if (__CheckDashAffect(*pkInstMain))
		return true;

	if (MODE_USE_SKILL != m_eReservedMode)
		return false;

	if (m_dwSkillSlotIndexReserved >= SKILL_MAX_NUM)
		return false;

	TSkillInstance & rkSkillInst = m_playerStatus.aSkill[m_dwSkillSlotIndexReserved];

	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::Instance().GetSkillData(rkSkillInst.dwIndex, &pSkillData))
		return false;

	return pSkillData->IsChargeSkill() ? true : false;
}

void CPythonPlayer::__Update_AutoAttack()
{
	if (0 == m_dwAutoAttackTargetVID)
		return;

	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (!pkInstMain)
		return;

	// ÅºÈ¯°Ý ¾²°í ´Þ·Á°¡´Â µµÁß¿¡´Â ½ºÅµ
	if (__IsUsingChargeSkill())
		return;

	CInstanceBase* pkInstVictim=NEW_FindActorPtr(m_dwAutoAttackTargetVID);
	if (!pkInstVictim)
	{
		__ClearAutoAttackTargetActorID();
	}
	else
	{
		if (pkInstVictim->IsDead())
		{
			__ClearAutoAttackTargetActorID();
		}
		else if (pkInstMain->IsMountingHorse() && !pkInstMain->CanAttackHorseLevel())
		{
			__ClearAutoAttackTargetActorID();
		}
		else if (pkInstMain->IsAttackableInstance(*pkInstVictim))
		{
			if (pkInstMain->IsSleep())
			{
				//TraceError("SKIP_AUTO_ATTACK_IN_SLEEPING");
			}
			else
			{
				__ReserveClickActor(m_dwAutoAttackTargetVID);
			}
		}
	}
}

void CPythonPlayer::__Update_NotifyGuildAreaEvent()
{
	CInstanceBase * pkInstMain = NEW_GetMainActorPtr();
	if (pkInstMain)
	{
		TPixelPosition kPixelPosition;
		pkInstMain->NEW_GetPixelPosition(&kPixelPosition);

		DWORD dwAreaID = CPythonMiniMap::Instance().GetGuildAreaID(
			ULONG(kPixelPosition.x), ULONG(kPixelPosition.y));

		if (dwAreaID != m_inGuildAreaID)
		{
			if (0xffffffff != dwAreaID)
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_Guild_EnterGuildArea", Py_BuildValue("(i)", dwAreaID));
			}
			else
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_Guild_ExitGuildArea", Py_BuildValue("(i)", dwAreaID));
			}

			m_inGuildAreaID = dwAreaID;
		}
	}
}

void CPythonPlayer::SetMainCharacterIndex(int iIndex)
{
	m_dwMainCharacterIndex = iIndex;

	CInstanceBase* pkInstMain=NEW_GetMainActorPtr();
	if (pkInstMain)
	{
		CPythonPlayerEventHandler& rkPlayerEventHandler=CPythonPlayerEventHandler::GetSingleton();
		pkInstMain->SetEventHandler(&rkPlayerEventHandler);
	}
}

DWORD CPythonPlayer::GetMainCharacterIndex()
{
	return m_dwMainCharacterIndex;
}

bool CPythonPlayer::IsMainCharacterIndex(DWORD dwIndex)
{
	return (m_dwMainCharacterIndex == dwIndex);
}

DWORD CPythonPlayer::GetGuildID()
{
	CInstanceBase* pkInstMain=NEW_GetMainActorPtr();
	if (!pkInstMain)
		return 0xffffffff;

	return pkInstMain->GetGuildID();
}

void CPythonPlayer::SetWeaponPower(DWORD dwMinPower, DWORD dwMaxPower, DWORD dwMinMagicPower, DWORD dwMaxMagicPower, DWORD dwAddPower)
{
	m_dwWeaponMinPower=dwMinPower;
	m_dwWeaponMaxPower=dwMaxPower;
	m_dwWeaponMinMagicPower=dwMinMagicPower;
	m_dwWeaponMaxMagicPower=dwMaxMagicPower;
	m_dwWeaponAddPower=dwAddPower;

	__UpdateBattleStatus();	
}

void CPythonPlayer::SetRace(DWORD dwRace)
{
	m_dwRace=dwRace;
}

DWORD CPythonPlayer::GetRace()
{
	return m_dwRace;
}

DWORD CPythonPlayer::__GetRaceStat()
{
	switch (GetRace())
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_WARRIOR_W:
			return GetStatus(POINT_ST);
			break;
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_ASSASSIN_W:
			return GetStatus(POINT_DX);
			break;
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_SURA_W:
			return GetStatus(POINT_ST);
			break;
		case MAIN_RACE_SHAMAN_M:
		case MAIN_RACE_SHAMAN_W:
			return GetStatus(POINT_IQ);
			break;
#ifdef ENABLE_WOLFMAN
		case MAIN_RACE_WOLFMAN_M:
			return GetStatus(POINT_DX);
			break;
#endif
	}	
	return GetStatus(POINT_ST);
}

DWORD CPythonPlayer::__GetLevelAtk()
{
	return 2*GetStatus(POINT_LEVEL);
}

DWORD CPythonPlayer::__GetStatAtk()
{
#ifdef ENABLE_WOLFMAN
	if (GetRace() == MAIN_RACE_WOLFMAN_M)
		return (2 * GetStatus(POINT_HT) + 4 * __GetRaceStat()) / 3;
	else
		return (4 * GetStatus(POINT_ST) + 2 * __GetRaceStat()) / 3;
#else
	return (4*GetStatus(POINT_ST)+2*__GetRaceStat())/3;
#endif
}

DWORD CPythonPlayer::__GetWeaponAtk(DWORD dwWeaponPower)
{
	return 2*dwWeaponPower;
}

DWORD CPythonPlayer::__GetTotalAtk(DWORD dwWeaponPower, DWORD dwRefineBonus)
{
	DWORD dwLvAtk=__GetLevelAtk();
	DWORD dwStAtk=__GetStatAtk();

	/////

	DWORD dwWepAtk;
	DWORD dwTotalAtk;	

	int hr = __GetHitRate();
	dwWepAtk = __GetWeaponAtk(dwWeaponPower+dwRefineBonus);
	dwTotalAtk = dwLvAtk + (dwStAtk + dwWepAtk)*hr / 100;

#ifdef ENABLE_WOLFMAN
	if (GetRace() == MAIN_RACE_WOLFMAN_M) //Cheating in Vision
	{
		if (dwWepAtk <= 0)
			dwTotalAtk += 4;
		else
			dwTotalAtk += 5;
	}
#endif	

	return dwTotalAtk;
}

DWORD CPythonPlayer::__GetHitRate()
{
	int src = (GetStatus(POINT_DX) * 4 + GetStatus(POINT_LEVEL) * 2)/6;

	return 100*(min(90, src)+210)/300;
}

DWORD CPythonPlayer::__GetEvadeRate()
{
	return 30*(2*GetStatus(POINT_DX)+5)/(GetStatus(POINT_DX)+95);
} 

void CPythonPlayer::__UpdateBattleStatus()
{
	m_playerStatus.SetPoint(POINT_NONE, 0);
	m_playerStatus.SetPoint(POINT_EVADE_RATE, __GetEvadeRate());
	m_playerStatus.SetPoint(POINT_HIT_RATE, __GetHitRate());
	m_playerStatus.SetPoint(POINT_MIN_WEP, m_dwWeaponMinPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MAX_WEP, m_dwWeaponMaxPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MIN_MAGIC_WEP, m_dwWeaponMinMagicPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MAX_MAGIC_WEP, m_dwWeaponMaxMagicPower+m_dwWeaponAddPower);
	m_playerStatus.SetPoint(POINT_MIN_ATK, __GetTotalAtk(m_dwWeaponMinPower, m_dwWeaponAddPower));
	m_playerStatus.SetPoint(POINT_MAX_ATK, __GetTotalAtk(m_dwWeaponMaxPower, m_dwWeaponAddPower));	
}

void CPythonPlayer::SetStatus(DWORD dwType, LONGLONG lValue)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::SetStatus - Strange Status Type!");
		Tracef("CPythonPlayer::SetStatus - Set Status Type Error\n");
		return;
	}

	if (dwType == POINT_LEVEL)
	{
		CInstanceBase* pkPlayer = NEW_GetMainActorPtr();

		if (pkPlayer)
			pkPlayer->UpdateTextTailLevel(lValue);
	}

	switch (dwType)
	{
		case POINT_MIN_WEP:
		case POINT_MAX_WEP:
		case POINT_MIN_ATK:
		case POINT_MAX_ATK:
		case POINT_HIT_RATE:
		case POINT_EVADE_RATE:
		case POINT_LEVEL:
		case POINT_ST:
		case POINT_DX:
		case POINT_IQ:
			m_playerStatus.SetPoint(dwType, lValue);
			__UpdateBattleStatus();
			break;
		default:
			m_playerStatus.SetPoint(dwType, lValue);
			break;
	}		
}

LONGLONG CPythonPlayer::GetStatus(DWORD dwType)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::GetStatus - Strange Status Type!");
		Tracef("CPythonPlayer::GetStatus - Get Status Type Error\n");
		return 0;
	}

	return m_playerStatus.GetPoint(dwType);
}

void CPythonPlayer::SetRealStatus(DWORD dwType, LONGLONG llValue)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::SetStatus - Strange Status Type!");
		Tracef("CPythonPlayer::SetStatus - Set Status Type Error\n");
		return;
	}

	m_playerStatus.SetRealPoint(dwType, llValue);
}

long long CPythonPlayer::GetRealStatus(DWORD dwType)
{
	if (dwType >= POINT_MAX_NUM)
	{
		assert(!" CPythonPlayer::GetStatus - Strange Status Type!");
		Tracef("CPythonPlayer::GetStatus - Get Status Type Error\n");
		return 0;
	}

	return m_playerStatus.GetRealPoint(dwType);
}

const char* CPythonPlayer::GetName()
{
	return m_stName.c_str();
}

void CPythonPlayer::SetName(const char *name)
{
	m_stName = name;
}

void CPythonPlayer::NotifyDeletingCharacterInstance(DWORD dwVID)
{
	if (m_dwMainCharacterIndex == dwVID)
		m_dwMainCharacterIndex = 0;
}

void CPythonPlayer::NotifyCharacterDead(DWORD dwVID)
{
	if (__IsSameTargetVID(dwVID))
	{
		SetTarget(0);
	}
}

void CPythonPlayer::NotifyCharacterUpdate(DWORD dwVID)
{
	if (__IsSameTargetVID(dwVID))
	{
		CInstanceBase * pMainInstance = NEW_GetMainActorPtr();
		CInstanceBase * pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(dwVID);
		if (pMainInstance && pTargetInstance)
		{
			if (!pMainInstance->IsTargetableInstance(*pTargetInstance))
			{
				SetTarget(0);
				PyCallClassMemberFunc(m_ppyGameWindow, "CloseTargetBoard", Py_BuildValue("()"));
			}
			else
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "RefreshTargetBoardByVID", Py_BuildValue("(i)", dwVID));
			}
		}
	}
}

void CPythonPlayer::NotifyDeadMainCharacter()
{
	__ClearAutoAttackTargetActorID();
}

void CPythonPlayer::NotifyChangePKMode()
{
	PyCallClassMemberFunc(m_ppyGameWindow, "OnChangePKMode", Py_BuildValue("()"));
}


void CPythonPlayer::MoveItemData(TItemPos SrcCell, TItemPos DstCell)
{
	if (!SrcCell.IsValidCell() || !DstCell.IsValidCell())
		return;

	network::TItemData src_item(*GetItemData(SrcCell));
	network::TItemData dst_item(*GetItemData(DstCell));
	SetItemData(DstCell, src_item);
	SetItemData(SrcCell, dst_item);
}

const network::TItemData * CPythonPlayer::GetItemData(TItemPos Cell) const
{
	if (!Cell.IsValidCell())
		return NULL;

	switch (Cell.window_type)
	{
	case INVENTORY:
#ifdef ENABLE_SKILL_INVENTORY
	case SKILLBOOK_INVENTORY:
#endif
	case UPPITEM_INVENTORY:
	case STONE_INVENTORY:
	case ENCHANT_INVENTORY:
#ifdef ENABLE_COSTUME_INVENTORY
	case COSTUME_INVENTORY:
#endif
	case EQUIPMENT:
		return &m_playerStatus.aItem[Cell.cell];
#ifdef ENABLE_DRAGONSOUL
	case DRAGON_SOUL_INVENTORY:
		return &m_playerStatus.aDSItem[Cell.cell];
#endif
	default:
		return NULL;
	}
}

void CPythonPlayer::SetItemData(TItemPos Cell, const network::TItemData & c_rkItemInst)
{
	if (!Cell.IsValidCell())
		return;

	if (c_rkItemInst.vnum() != 0)
	{
		CItemData * pItemData;
		if (!CItemManager::Instance().GetItemDataPointer(c_rkItemInst.vnum(), &pItemData))
		{
			TraceError("CPythonPlayer::SetItemData(window_type : %d, dwSlotIndex=%d, itemIndex=%d) - Failed to item data\n", Cell.window_type, Cell.cell, c_rkItemInst.vnum());
			return;
		}
	}

	switch (Cell.window_type)
	{
	case INVENTORY:
#ifdef ENABLE_SKILL_INVENTORY
	case SKILLBOOK_INVENTORY:
#endif
	case UPPITEM_INVENTORY:
	case STONE_INVENTORY:
	case ENCHANT_INVENTORY:
#ifdef ENABLE_COSTUME_INVENTORY
	case COSTUME_INVENTORY:
#endif
	case EQUIPMENT:
		m_playerStatus.aItem[Cell.cell] = c_rkItemInst;
		break;
#ifdef ENABLE_DRAGONSOUL
	case DRAGON_SOUL_INVENTORY:
		m_playerStatus.aDSItem[Cell.cell] = c_rkItemInst;
		break;
#endif
	}
}

DWORD CPythonPlayer::GetItemIndex(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;

	return GetItemData(Cell)->vnum();
}

DWORD CPythonPlayer::GetItemCount(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;
	auto pItem = GetItemData(Cell);
	if (pItem == NULL)
		return 0;
	else
		return pItem->count();
}

DWORD CPythonPlayer::GetItemCountByVnum(DWORD dwVnum)
{
	DWORD dwCount = 0;

	for (int i = 0; i < c_Inventory_Count; ++i)
	{
		auto& c_rItemData = m_playerStatus.aItem[i];
		if (c_rItemData.vnum() == dwVnum)
		{
			dwCount += c_rItemData.count();
		}
	}

	return dwCount;
}

DWORD CPythonPlayer::GetItemCountByVnumRange(DWORD dwVnumStart, DWORD dwVnumEnd)
{
	DWORD dwCount = 0;

	for (int i = 0; i < c_Inventory_Count; ++i)
	{
		auto& c_rItemData = m_playerStatus.aItem[i];
		if (c_rItemData.vnum() >= dwVnumStart && c_rItemData.vnum() <= dwVnumEnd)
		{
			dwCount += c_rItemData.count();
		}
	}

	return dwCount;
}

#ifdef ENABLE_ALPHA_EQUIP
int CPythonPlayer::GetItemAlphaEquipValue(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;
	auto pItem = GetItemData(Cell);
	if (pItem == NULL)
		return 0;
	else
		return pItem->alpha_equip();
}
#endif

DWORD CPythonPlayer::GetItemMetinSocket(TItemPos Cell, DWORD dwMetinSocketIndex)
{
	if (!Cell.IsValidCell())
		return 0;

	if (dwMetinSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return 0;

	if (GetItemData(Cell)->sockets_size() <= dwMetinSocketIndex)
		return 0;

	return GetItemData(Cell)->sockets(dwMetinSocketIndex);
}

void CPythonPlayer::GetItemAttribute(TItemPos Cell, DWORD dwAttrSlotIndex, BYTE * pbyType, short * psValue)
{
	*pbyType = 0;
	*psValue = 0;

	if (!Cell.IsValidCell())
		return;

	if (dwAttrSlotIndex >= ITEM_ATTRIBUTE_SLOT_MAX_NUM)
		return;

	*pbyType = GetItemData(Cell)->attributes(dwAttrSlotIndex).type();
	*psValue = GetItemData(Cell)->attributes(dwAttrSlotIndex).value();
}

#ifdef INCREASE_ITEM_STACK
void CPythonPlayer::SetItemCount(TItemPos Cell, WORD byCount)
#else
void CPythonPlayer::SetItemCount(TItemPos Cell, BYTE byCount)
#endif
{
	if (!Cell.IsValidCell())
		return;

	(const_cast <network::TItemData *>(GetItemData(Cell)))->set_count(byCount);
	PyCallClassMemberFunc(m_ppyGameWindow, "RefreshInventory", Py_BuildValue("()"));	
}

void CPythonPlayer::SetItemMetinSocket(TItemPos Cell, DWORD dwMetinSocketIndex, DWORD dwMetinNumber)
{
	if (!Cell.IsValidCell())
		return;
	if (dwMetinSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return;

	(const_cast <network::TItemData*>(GetItemData(Cell)))->set_sockets(dwMetinSocketIndex, dwMetinNumber);
}

void CPythonPlayer::SetItemAttribute(TItemPos Cell, DWORD dwAttrIndex, BYTE byType, short sValue)
{
	if (!Cell.IsValidCell())
		return;
	if (dwAttrIndex >= ITEM_ATTRIBUTE_SLOT_MAX_NUM)
		return;

	auto attr = (const_cast <network::TItemData*>(GetItemData(Cell)))->mutable_attributes(dwAttrIndex);
	attr->set_type(byType);
	attr->set_value(sValue);
}

int CPythonPlayer::GetQuickPage()
{
	return m_playerStatus.lQuickPageIndex;
}

void CPythonPlayer::SetQuickPage(int nQuickPageIndex)
{
	if (nQuickPageIndex<0)
		m_playerStatus.lQuickPageIndex=QUICKSLOT_MAX_LINE+nQuickPageIndex;	
	else if (nQuickPageIndex>=QUICKSLOT_MAX_LINE)
		m_playerStatus.lQuickPageIndex=nQuickPageIndex%QUICKSLOT_MAX_LINE;	
	else
		m_playerStatus.lQuickPageIndex=nQuickPageIndex;	

	PyCallClassMemberFunc(m_ppyGameWindow, "RefreshInventory", Py_BuildValue("()"));
}

DWORD	CPythonPlayer::LocalQuickSlotIndexToGlobalQuickSlotIndex(DWORD dwLocalSlotIndex)
{
	return m_playerStatus.lQuickPageIndex*QUICKSLOT_MAX_COUNT_PER_LINE+dwLocalSlotIndex;	
}

void	CPythonPlayer::GetGlobalQuickSlotData(DWORD dwGlobalSlotIndex, DWORD* pdwWndType, DWORD* pdwWndItemPos)
{
	auto& rkQuickSlot=__RefGlobalQuickSlot(dwGlobalSlotIndex);
	*pdwWndType=rkQuickSlot.type();
	*pdwWndItemPos=rkQuickSlot.pos();
}

void	CPythonPlayer::GetLocalQuickSlotData(DWORD dwSlotPos, DWORD* pdwWndType, DWORD* pdwWndItemPos)
{
	auto& rkQuickSlot=__RefLocalQuickSlot(dwSlotPos);
	*pdwWndType=rkQuickSlot.type();
	*pdwWndItemPos=rkQuickSlot.pos();
}

TQuickslot & CPythonPlayer::__RefLocalQuickSlot(int SlotIndex)
{
	return __RefGlobalQuickSlot(LocalQuickSlotIndexToGlobalQuickSlotIndex(SlotIndex));
}

TQuickslot & CPythonPlayer::__RefGlobalQuickSlot(int SlotIndex)
{
	if (SlotIndex < 0 || SlotIndex >= QUICKSLOT_MAX_NUM)
	{
		static TQuickslot s_kQuickSlot;
		s_kQuickSlot.Clear();
		return s_kQuickSlot;
	}

	return m_playerStatus.aQuickSlot[SlotIndex];
}

void CPythonPlayer::RemoveQuickSlotByValue(int iType, int iPosition)
{
	for (BYTE i = 0; i < QUICKSLOT_MAX_NUM; ++i)
	{
		if (iType == m_playerStatus.aQuickSlot[i].type())
			if (iPosition == m_playerStatus.aQuickSlot[i].pos())
				CPythonNetworkStream::Instance().SendQuickSlotDelPacket(i);
	}
}

char CPythonPlayer::IsItem(TItemPos Cell)
{
	if (!Cell.IsValidCell())
		return 0;

	return 0 != GetItemData(Cell)->vnum();
}

void CPythonPlayer::RequestMoveGlobalQuickSlotToLocalQuickSlot(DWORD dwGlobalSrcSlotIndex, DWORD dwLocalDstSlotIndex)
{
	//DWORD dwGlobalSrcSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSrcSlotIndex);
	DWORD dwGlobalDstSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalDstSlotIndex);

	CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
	rkNetStream.SendQuickSlotMovePacket((BYTE) dwGlobalSrcSlotIndex, (BYTE)dwGlobalDstSlotIndex);
}

void CPythonPlayer::RequestAddLocalQuickSlot(DWORD dwLocalSlotIndex, DWORD dwWndType, DWORD dwWndItemPos)
{
	if (dwLocalSlotIndex>=QUICKSLOT_MAX_COUNT_PER_LINE)
		return;

	DWORD dwGlobalSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSlotIndex);

	CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
	rkNetStream.SendQuickSlotAddPacket((BYTE)dwGlobalSlotIndex, (BYTE)dwWndType, (BYTE)dwWndItemPos);
}

void CPythonPlayer::RequestAddToEmptyLocalQuickSlot(DWORD dwWndType, DWORD dwWndItemPos)
{
    for (int i = 0; i < QUICKSLOT_MAX_COUNT_PER_LINE; ++i)
    {
        auto& rkQuickSlot=__RefLocalQuickSlot(i);

        if (0 == rkQuickSlot.type())
        {
            DWORD dwGlobalQuickSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(i);
            CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
            rkNetStream.SendQuickSlotAddPacket((BYTE)dwGlobalQuickSlotIndex, (BYTE)dwWndType, (BYTE)dwWndItemPos);
            return;
        }
    }

}

void CPythonPlayer::RequestDeleteGlobalQuickSlot(DWORD dwGlobalSlotIndex)
{
	if (dwGlobalSlotIndex>=QUICKSLOT_MAX_COUNT)
		return;

	//if (dwLocalSlotIndex>=QUICKSLOT_MAX_SLOT_PER_LINE)
	//	return;

	//DWORD dwGlobalSlotIndex=LocalQuickSlotIndexToGlobalQuickSlotIndex(dwLocalSlotIndex);

	CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
	rkNetStream.SendQuickSlotDelPacket((BYTE)dwGlobalSlotIndex);
}

void CPythonPlayer::RequestUseLocalQuickSlot(DWORD dwLocalSlotIndex)
{
	if (dwLocalSlotIndex>=QUICKSLOT_MAX_COUNT_PER_LINE)
		return;

	DWORD dwRegisteredType;
	DWORD dwRegisteredItemPos;
	GetLocalQuickSlotData(dwLocalSlotIndex, &dwRegisteredType, &dwRegisteredItemPos);

	switch (dwRegisteredType)
	{
		case SLOT_TYPE_INVENTORY:
		{
			CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
			rkNetStream.SendItemUsePacket(TItemPos(INVENTORY, (WORD)dwRegisteredItemPos));
			break;
		}
		case SLOT_TYPE_SKILL:
		{
			ClickSkillSlot(dwRegisteredItemPos);
			break;
		}
		case SLOT_TYPE_EMOTION:
		{
			PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_ActEmotion", Py_BuildValue("(i)", dwRegisteredItemPos));
			break;
		}
	}
}

void CPythonPlayer::AddQuickSlot(int QuickSlotIndex, char IconType, WORD IconPosition)
{
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QUICKSLOT_MAX_NUM)
		return;

	m_playerStatus.aQuickSlot[QuickSlotIndex].set_type(IconType);
	m_playerStatus.aQuickSlot[QuickSlotIndex].set_pos(IconPosition);
}

void CPythonPlayer::DeleteQuickSlot(int QuickSlotIndex)
{
	if (QuickSlotIndex < 0 || QuickSlotIndex >= QUICKSLOT_MAX_NUM)
		return;

	m_playerStatus.aQuickSlot[QuickSlotIndex].Clear();
}

void CPythonPlayer::MoveQuickSlot(int Source, int Target)
{
	if (Source < 0 || Source >= QUICKSLOT_MAX_NUM)
		return;

	if (Target < 0 || Target >= QUICKSLOT_MAX_NUM)
		return;

	auto& rkSrcSlot=__RefGlobalQuickSlot(Source);
	auto& rkDstSlot=__RefGlobalQuickSlot(Target);

	std::swap(rkSrcSlot, rkDstSlot);
}

bool CPythonPlayer::IsInventorySlot(TItemPos Cell)
{
	return !Cell.IsEquipCell() && Cell.IsValidCell();
}

bool CPythonPlayer::IsEquipmentSlot(TItemPos Cell)
{
	return Cell.IsEquipCell();
}

bool CPythonPlayer::IsEquipItemInSlot(TItemPos Cell)
{
	if (!Cell.IsEquipCell())
	{
		return false;
	}

	auto pData = GetItemData(Cell);
	
	if (NULL == pData)
	{
		return false;
	}

	DWORD dwItemIndex = pData->vnum();

	CItemManager::Instance().SelectItemData(dwItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
	{
		TraceError("Failed to find ItemData - CPythonPlayer::IsEquipItem(window_type=%d, iSlotindex=%d)\n", Cell.window_type, Cell.cell);
		return false;
	}

	return pItemData->IsEquipment() ? true : false;
}


void CPythonPlayer::SetSkill(DWORD dwSlotIndex, DWORD dwSkillIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	m_playerStatus.aSkill[dwSlotIndex].dwIndex = dwSkillIndex;
	m_skillSlotDict[dwSkillIndex] = dwSlotIndex;
}

int CPythonPlayer::GetSkillIndex(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].dwIndex;
}

bool CPythonPlayer::GetSkillSlotIndex(DWORD dwSkillIndex, DWORD* pdwSlotIndex)
{
	std::map<DWORD, DWORD>::iterator f=m_skillSlotDict.find(dwSkillIndex);
	if (m_skillSlotDict.end()==f)
	{
		return false;
	}

	*pdwSlotIndex=f->second;

	return true;
}

int CPythonPlayer::GetSkillGrade(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].iGrade;
}

int CPythonPlayer::GetSkillLevel(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].iLevel;
}

float CPythonPlayer::GetSkillCurrentEfficientPercentage(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage;
}

float CPythonPlayer::GetSkillNextEfficientPercentage(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0;

	return m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage;
}

void CPythonPlayer::SetSkillLevel(DWORD dwSlotIndex, DWORD dwSkillLevel)
{
	assert(!"CPythonPlayer::SetSkillLevel - »ç¿ëÇÏÁö ¾Ê´Â ÇÔ¼ö");
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	m_playerStatus.aSkill[dwSlotIndex].iGrade = -1;
	m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel;
}

void CPythonPlayer::SetSkillLevel_(DWORD dwSkillIndex, DWORD dwSkillGrade, DWORD dwSkillLevel)
{
	DWORD dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
		return;

	if (dwSlotIndex >= SKILL_MAX_NUM)
		return;

	switch (dwSkillGrade)
	{
		case 0:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel;
			break;
		case 1:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-20+1;
			break;
		case 2:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-30+1;
			break;
		case 3:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-40+1;
			break;
		case 4:
			m_playerStatus.aSkill[dwSlotIndex].iGrade = dwSkillGrade;
			m_playerStatus.aSkill[dwSlotIndex].iLevel = dwSkillLevel-50+1;
			break;
	}

	const DWORD SKILL_MAX_LEVEL = 50;

	if (dwSkillLevel>SKILL_MAX_LEVEL)
	{
		m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage = 0.0f;
		m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage = 0.0f;

		TraceError("CPythonPlayer::SetSkillLevel(SlotIndex=%d, SkillLevel=%d)", dwSlotIndex, dwSkillLevel);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].fcurEfficientPercentage = CLocaleManager::instance().GetSkillPower(dwSkillLevel) / 100.0f;
	m_playerStatus.aSkill[dwSlotIndex].fnextEfficientPercentage = CLocaleManager::instance().GetSkillPower(dwSkillLevel + 1) / 100.0f;

}

void CPythonPlayer::SetSkillCoolTime(DWORD dwSkillIndex)
{
	DWORD dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
	{
		Tracenf("CPythonPlayer::SetSkillCoolTime(dwSkillIndex=%d) - FIND SLOT ERROR", dwSkillIndex);
		return;
	}

	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::SetSkillCoolTime(dwSkillIndex=%d) - dwSlotIndex=%d/%d OUT OF RANGE", dwSkillIndex, dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].isCoolTime=true;
}

void CPythonPlayer::EndSkillCoolTime(DWORD dwSkillIndex)
{
	DWORD dwSlotIndex;
	if (!GetSkillSlotIndex(dwSkillIndex, &dwSlotIndex))
	{
		Tracenf("CPythonPlayer::EndSkillCoolTime(dwSkillIndex=%d) - FIND SLOT ERROR", dwSkillIndex);
		return;
	}

	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::EndSkillCoolTime(dwSkillIndex=%d) - dwSlotIndex=%d/%d OUT OF RANGE", dwSkillIndex, dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].isCoolTime=false;
}

float CPythonPlayer::GetSkillCoolTime(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0.0f;

	return m_playerStatus.aSkill[dwSlotIndex].fCoolTime;
}

float CPythonPlayer::GetSkillElapsedCoolTime(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return 0.0f;

	return CTimer::Instance().GetCurrentSecond() - m_playerStatus.aSkill[dwSlotIndex].fLastUsedTime;
}

void CPythonPlayer::__ActivateSkillSlot(DWORD dwSlotIndex)
{
	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::ActivavteSkill(dwSlotIndex=%d/%d) - OUT OF RANGE", dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].bActive = TRUE;
	PyCallClassMemberFunc(m_ppyGameWindow, "ActivateSkillSlot", Py_BuildValue("(i)", dwSlotIndex));
}

void CPythonPlayer::__DeactivateSkillSlot(DWORD dwSlotIndex)
{
	if (dwSlotIndex>=SKILL_MAX_NUM)
	{
		Tracenf("CPythonPlayer::DeactivavteSkill(dwSlotIndex=%d/%d) - OUT OF RANGE", dwSlotIndex, SKILL_MAX_NUM);
		return;
	}

	m_playerStatus.aSkill[dwSlotIndex].bActive = FALSE;
	PyCallClassMemberFunc(m_ppyGameWindow, "DeactivateSkillSlot", Py_BuildValue("(i)", dwSlotIndex));
}

BOOL CPythonPlayer::IsSkillCoolTime(DWORD dwSlotIndex)
{
	if (!__CheckRestSkillCoolTime(dwSlotIndex))
		return FALSE;

	return TRUE;
}

BOOL CPythonPlayer::IsSkillActive(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return FALSE;

	return m_playerStatus.aSkill[dwSlotIndex].bActive;
}

BOOL CPythonPlayer::IsToggleSkill(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= SKILL_MAX_NUM)
		return FALSE;

	DWORD dwSkillIndex = m_playerStatus.aSkill[dwSlotIndex].dwIndex;

	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::Instance().GetSkillData(dwSkillIndex, &pSkillData))
		return FALSE;

	return pSkillData->IsToggleSkill();
}

void CPythonPlayer::SetPlayTime(DWORD dwPlayTime)
{
	m_dwPlayTime = dwPlayTime;
}

DWORD CPythonPlayer::GetPlayTime()
{
	return m_dwPlayTime;
}

void CPythonPlayer::SendClickItemPacket(DWORD dwIID, bool bIgnorePickupTime)
{
	if (IsObserverMode())
		return;

	static DWORD s_dwNextTCPTime = 0;

	DWORD dwCurTime=ELTimer_GetMSec();

	if (dwCurTime >= s_dwNextTCPTime || bIgnorePickupTime)
	{
		s_dwNextTCPTime=dwCurTime + 50;

		const char * c_szOwnerName;
		if (!CPythonItem::Instance().GetOwnership(dwIID, &c_szOwnerName))
			return;

		if (strlen(c_szOwnerName) > 0)
		if (0 != strcmp(c_szOwnerName, GetName()))
		{
			CItemData * pItemData;
			if (!CItemManager::Instance().GetItemDataPointer(CPythonItem::Instance().GetVirtualNumberOfGroundItem(dwIID), &pItemData))
			{
				Tracenf("CPythonPlayer::SendClickItemPacket(dwIID=%d) : Non-exist item.", dwIID);
				return;
			}
			if (!IsPartyMemberByName(c_szOwnerName) || pItemData->IsAntiFlag(CItemData::ITEM_ANTIFLAG_DROP | CItemData::ITEM_ANTIFLAG_GIVE))
			{
				PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotPickItem", Py_BuildValue("()"));
				return;
			}
		}

		CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();
		rkNetStream.SendItemPickUpPacket(dwIID);
	}
}

void CPythonPlayer::__SendClickActorPacket(CInstanceBase& rkInstVictim)
{
#ifdef ENABLE_FAKEBUFF
	if (rkInstVictim.IsMyFakeBuff())
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_FakeBuffOpenSkills", Py_BuildValue("()"));
		return;
	}
#endif

	// ¸»À» Å¸°í ±¤»êÀ» Ä³´Â °Í¿¡ ´ëÇÑ ¿¹¿Ü Ã³¸®
	CInstanceBase* pkInstMain=NEW_GetMainActorPtr();
	if (pkInstMain)
	if (pkInstMain->IsHoldingPickAxe())
	if (pkInstMain->IsMountingHorse())
	if (rkInstVictim.IsResource())
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotMining", Py_BuildValue("()"));
		return;
	}

	static DWORD s_dwNextTCPTime = 0;

	DWORD dwCurTime=ELTimer_GetMSec();

	if (dwCurTime >= s_dwNextTCPTime)
	{
		s_dwNextTCPTime=dwCurTime+1000;

		CPythonNetworkStream& rkNetStream=CPythonNetworkStream::Instance();

		DWORD dwVictimVID=rkInstVictim.GetVirtualID();
		rkNetStream.SendOnClickPacket(dwVictimVID);
	}
}

void CPythonPlayer::ActEmotion(DWORD dwEmotionID)
{
	CInstanceBase * pkInstTarget = __GetAliveTargetInstancePtr();
	if (!pkInstTarget)
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "OnCannotShotError", Py_BuildValue("(is)", GetMainCharacterIndex(), "NEED_TARGET"));
		return;
	}

	CPythonNetworkStream::Instance().SendChatPacket(_getf("/kiss %s", pkInstTarget->GetNameString()));
}

void CPythonPlayer::StartEmotionProcess()
{
	__ClearReservedAction();
	__ClearAutoAttackTargetActorID();

	m_bisProcessingEmotion = TRUE;
}

void CPythonPlayer::EndEmotionProcess()
{
	m_bisProcessingEmotion = FALSE;
}

BOOL CPythonPlayer::__IsProcessingEmotion()
{
	return m_bisProcessingEmotion;
}

// Dungeon
void CPythonPlayer::SetDungeonDestinationPosition(int ix, int iy)
{
	m_isDestPosition = TRUE;
	m_ixDestPos = ix;
	m_iyDestPos = iy;

	AlarmHaveToGo();
}

void CPythonPlayer::AlarmHaveToGo()
{
	m_iLastAlarmTime = CTimer::Instance().GetCurrentMillisecond();

	/////

	CInstanceBase * pInstance = NEW_GetMainActorPtr();
	if (!pInstance)
		return;

	TPixelPosition PixelPosition;
	pInstance->NEW_GetPixelPosition(&PixelPosition);

	float fAngle = GetDegreeFromPosition2(PixelPosition.x, PixelPosition.y, float(m_ixDestPos), float(m_iyDestPos));
	fAngle = fmod(540.0f - fAngle, 360.0f);
	D3DXVECTOR3 v3Rotation(0.0f, 0.0f, fAngle);

	PixelPosition.y *= -1.0f;

	CEffectManager::Instance().RegisterEffect("d:/ymir work/effect/etc/compass/appear_middle.mse");
	CEffectManager::Instance().CreateEffect("d:/ymir work/effect/etc/compass/appear_middle.mse", PixelPosition, v3Rotation);
}

// Party
void CPythonPlayer::ExitParty()
{
	m_PartyMemberMap.clear();

	CPythonCharacterManager::Instance().RefreshAllPCTextTail();
}

void CPythonPlayer::AppendPartyMember(DWORD dwPID, const char * c_szName)
{
	m_PartyMemberMap.insert(make_pair(dwPID, TPartyMemberInfo(dwPID, c_szName)));
}

void CPythonPlayer::LinkPartyMember(DWORD dwPID, DWORD dwVID)
{
	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceError(" CPythonPlayer::LinkPartyMember(dwPID=%d, dwVID=%d) - Failed to find party member", dwPID, dwVID);
		return;
	}

	pPartyMemberInfo->dwVID = dwVID;

	CInstanceBase * pInstance = NEW_FindActorPtr(dwVID);
	if (pInstance)
		pInstance->RefreshTextTail();
}

void CPythonPlayer::UnlinkPartyMember(DWORD dwPID)
{
	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceError(" CPythonPlayer::UnlinkPartyMember(dwPID=%d) - Failed to find party member", dwPID);
		return;
	}

	pPartyMemberInfo->dwVID = 0;
}

void CPythonPlayer::UpdatePartyMemberInfo(DWORD dwPID, bool bLeader, BYTE byState, BYTE byHPPercentage)
{
	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceError(" CPythonPlayer::UpdatePartyMemberInfo(dwPID=%d, byState=%d, byHPPercentage=%d) - Failed to find character", dwPID, byState, byHPPercentage);
		return;
	}

	pPartyMemberInfo->bLeader = bLeader;
	pPartyMemberInfo->byState = byState;
	pPartyMemberInfo->byHPPercentage = byHPPercentage;
}

void CPythonPlayer::UpdatePartyMemberAffect(DWORD dwPID, BYTE byAffectSlotIndex, short sAffectNumber)
{
	if (byAffectSlotIndex >= PARTY_AFFECT_SLOT_MAX_NUM)
	{
		TraceError(" CPythonPlayer::UpdatePartyMemberAffect(dwPID=%d, byAffectSlotIndex=%d, sAffectNumber=%d) - Strange affect slot index", dwPID, byAffectSlotIndex, sAffectNumber);
		return;
	}

	TPartyMemberInfo * pPartyMemberInfo;
	if (!GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		TraceError(" CPythonPlayer::UpdatePartyMemberAffect(dwPID=%d, byAffectSlotIndex=%d, sAffectNumber=%d) - Failed to find character", dwPID, byAffectSlotIndex, sAffectNumber);
		return;
	}

	pPartyMemberInfo->sAffects[byAffectSlotIndex] = sAffectNumber;
}

void CPythonPlayer::RemovePartyMember(DWORD dwPID)
{
	DWORD dwVID = 0;
	TPartyMemberInfo * pPartyMemberInfo;
	if (GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
	{
		dwVID = pPartyMemberInfo->dwVID;
	}

	m_PartyMemberMap.erase(dwPID);

	if (dwVID > 0)
	{
		CInstanceBase * pInstance = NEW_FindActorPtr(dwVID);
		if (pInstance)
			pInstance->RefreshTextTail();
	}
}

bool CPythonPlayer::IsPartyMemberByVID(DWORD dwVID)
{
	std::map<DWORD, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.begin();
	for (; itor != m_PartyMemberMap.end(); ++itor)
	{
		TPartyMemberInfo & rPartyMemberInfo = itor->second;
		if (dwVID == rPartyMemberInfo.dwVID)
			return true;
	}

	return false;
}

bool CPythonPlayer::IsPartyMemberByName(const char * c_szName)
{
	std::map<DWORD, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.begin();
	for (; itor != m_PartyMemberMap.end(); ++itor)
	{
		TPartyMemberInfo & rPartyMemberInfo = itor->second;
		if (0 == rPartyMemberInfo.strName.compare(c_szName))
			return true;
	}

	return false;
}

bool CPythonPlayer::GetPartyMemberPtr(DWORD dwPID, TPartyMemberInfo ** ppPartyMemberInfo)
{
	std::map<DWORD, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.find(dwPID);

	if (m_PartyMemberMap.end() == itor)
		return false;

	*ppPartyMemberInfo = &(itor->second);

	return true;
}

bool CPythonPlayer::PartyMemberPIDToVID(DWORD dwPID, DWORD * pdwVID)
{
	std::map<DWORD, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.find(dwPID);

	if (m_PartyMemberMap.end() == itor)
		return false;

	const TPartyMemberInfo & c_rPartyMemberInfo = itor->second;
	*pdwVID = c_rPartyMemberInfo.dwVID;

	return true;
}

bool CPythonPlayer::PartyMemberVIDToPID(DWORD dwVID, DWORD * pdwPID)
{
	std::map<DWORD, TPartyMemberInfo>::iterator itor = m_PartyMemberMap.begin();
	for (; itor != m_PartyMemberMap.end(); ++itor)
	{
		TPartyMemberInfo & rPartyMemberInfo = itor->second;
		if (dwVID == rPartyMemberInfo.dwVID)
		{
			*pdwPID = rPartyMemberInfo.dwPID;
			return true;
		}
	}

	return false;
}

bool CPythonPlayer::IsSamePartyMember(DWORD dwVID1, DWORD dwVID2)
{
	return (IsPartyMemberByVID(dwVID1) && IsPartyMemberByVID(dwVID2));
}

// PVP
void CPythonPlayer::RememberChallengeInstance(DWORD dwVID)
{
	m_RevengeInstanceSet.erase(dwVID);
	m_ChallengeInstanceSet.insert(dwVID);
}
void CPythonPlayer::RememberRevengeInstance(DWORD dwVID)
{
	m_ChallengeInstanceSet.erase(dwVID);
	m_RevengeInstanceSet.insert(dwVID);
}
void CPythonPlayer::RememberCantFightInstance(DWORD dwVID)
{
	m_CantFightInstanceSet.insert(dwVID);
}
void CPythonPlayer::ForgetInstance(DWORD dwVID)
{
	m_ChallengeInstanceSet.erase(dwVID);
	m_RevengeInstanceSet.erase(dwVID);
	m_CantFightInstanceSet.erase(dwVID);
}

bool CPythonPlayer::IsChallengeInstance(DWORD dwVID)
{
	return m_ChallengeInstanceSet.end() != m_ChallengeInstanceSet.find(dwVID);
}
bool CPythonPlayer::IsRevengeInstance(DWORD dwVID)
{
	return m_RevengeInstanceSet.end() != m_RevengeInstanceSet.find(dwVID);
}
bool CPythonPlayer::IsCantFightInstance(DWORD dwVID)
{
	return m_CantFightInstanceSet.end() != m_CantFightInstanceSet.find(dwVID);
}

void CPythonPlayer::OpenPrivateShop()
{
	m_isOpenPrivateShop = TRUE;
}
void CPythonPlayer::ClosePrivateShop()
{
	m_isOpenPrivateShop = FALSE;
}

bool CPythonPlayer::IsOpenPrivateShop()
{
	return m_isOpenPrivateShop;
}

void CPythonPlayer::SetObserverMode(bool isEnable)
{
	m_isObserverMode=isEnable;
}

bool CPythonPlayer::IsObserverMode()
{
	return m_isObserverMode;
}


BOOL CPythonPlayer::__ToggleCoolTime()
{
	m_sysIsCoolTime = 1 - m_sysIsCoolTime;
	return m_sysIsCoolTime;
}

BOOL CPythonPlayer::__ToggleLevelLimit()
{
	m_sysIsLevelLimit = 1 - m_sysIsLevelLimit;
	return m_sysIsLevelLimit;
}

void CPythonPlayer::StartStaminaConsume(DWORD dwConsumePerSec, DWORD dwCurrentStamina)
{
	m_isConsumingStamina = TRUE;
	m_fConsumeStaminaPerSec = float(dwConsumePerSec);
	m_fCurrentStamina = float(dwCurrentStamina);

	SetStatus(POINT_STAMINA, dwCurrentStamina);
}

void CPythonPlayer::StopStaminaConsume(DWORD dwCurrentStamina)
{
	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = float(dwCurrentStamina);

	SetStatus(POINT_STAMINA, dwCurrentStamina);
}

DWORD CPythonPlayer::GetPKMode()
{
	CInstanceBase * pInstance = NEW_GetMainActorPtr();
	if (!pInstance)
		return 0;

	return pInstance->GetPKMode();
}

void CPythonPlayer::SetMobileFlag(BOOL bFlag)
{
	m_bMobileFlag = bFlag;
	PyCallClassMemberFunc(m_ppyGameWindow, "RefreshMobile", Py_BuildValue("()"));
}

BOOL CPythonPlayer::HasMobilePhoneNumber()
{
	return m_bMobileFlag;
}

void CPythonPlayer::SetGameWindow(PyObject * ppyObject)
{
	m_ppyGameWindow = ppyObject;
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CPythonPlayer::SetAcceItemData(DWORD dwSlotIndex, const network::TItemData & rItemInstance)
{
	if (dwSlotIndex >= m_ItemAcceInstanceVector.size())
	{
		TraceError("CPythonSafeBox::SetAcceItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}

	if (dwSlotIndex != 2)
	{
		PyCallClassMemberFunc(m_ppyGameWindow, "ActivateAcceSlot", Py_BuildValue("(i)", m_acceRefineActivedSlot[dwSlotIndex]));
	}

	m_ItemAcceInstanceVector[dwSlotIndex] = rItemInstance;
}

enum eAcceSlots {
	ACCE_SLOT_LEFT,
	ACCE_SLOT_RIGHT,
	ACCE_SLOT_RESULT,
	ACCE_SLOT_NUM,
};

void CPythonPlayer::DelAcceItemData(DWORD dwSlotIndex)
{
	if (dwSlotIndex >= m_ItemAcceInstanceVector.size())
	{
		TraceError("CPythonPlayer::DelAcceItemData(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return;
	}
	auto& rInstance = m_ItemAcceInstanceVector[dwSlotIndex];

	CItemData * pItemData = nullptr;
	if (CItemManager::instance().GetItemDataPointer(rInstance.vnum(), &pItemData))
	{
		if (dwSlotIndex == ACCE_SLOT_LEFT)
			DelAcceItemData(ACCE_SLOT_RESULT);

		if (pItemData->GetType() == CItemData::ITEM_TYPE_WEAPON || pItemData->GetType() == CItemData::ITEM_TYPE_ARMOR)
			DelAcceItemData(ACCE_SLOT_RESULT);

		if (dwSlotIndex != ACCE_SLOT_RESULT)
		{
			PyCallClassMemberFunc(m_ppyGameWindow, "DeactivateAcceSlot", Py_BuildValue("(i)", m_acceRefineActivedSlot[dwSlotIndex]));
			m_acceRefineActivedSlot[dwSlotIndex] = -1;
		}

		rInstance.Clear();
	}
}

int CPythonPlayer::GetCurrentAcceSize()
{
	return m_ItemAcceInstanceVector.size();
}

BOOL CPythonPlayer::GetAcceSlotItemID(DWORD dwSlotIndex, DWORD* pdwItemID)
{
	if (dwSlotIndex >= m_ItemAcceInstanceVector.size())
	{
		TraceError("CPythonPlayer::GetAcceSlotItemID(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*pdwItemID = m_ItemAcceInstanceVector[dwSlotIndex].vnum();

	return TRUE;
}

BOOL CPythonPlayer::GetAcceItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance)
{
	if (dwSlotIndex >= m_ItemAcceInstanceVector.size())
	{
		TraceError("CPythonPlayer::GetAcceItemDataPtr(dwSlotIndex=%d) - Strange slot index", dwSlotIndex);
		return FALSE;
	}

	*ppInstance = &m_ItemAcceInstanceVector[dwSlotIndex];

	return TRUE;
}


BOOL CPythonPlayer::IsEmtpyAcceWindow()
{
	for (auto &i : m_ItemAcceInstanceVector) {

		if (i.vnum())
			break;

		return false;
	}

	return true;
}
int CPythonPlayer::GetCurrentAcceItemCount()
{
	int itemCount = 0;
	for (auto &i : m_ItemAcceInstanceVector) {

		if (i.vnum())
			++itemCount;
	}

	return itemCount;
}

void CPythonPlayer::SetAcceRefineWindowOpen(int type)
{
	bool isOpen = m_acceRefineWindowIsOpen == 0;
	m_acceRefineWindowType = type;
	m_acceRefineWindowIsOpen = isOpen;
}

void CPythonPlayer::SetActivedAcceSlot(int acceSlot, int itemPos)
{
	m_acceRefineActivedSlot[acceSlot] = itemPos;
}

int CPythonPlayer::FindActivedSlot(int acceSlot)
{
	for (int i = 0; i < 3; ++i)
	{
		if (m_acceRefineActivedSlot[i] == acceSlot)
			return i;
	}

	return 3;
}

int CPythonPlayer::FindUsingSlot(int acceSlot)
{
	if (acceSlot == 3)
		return -1;
	return m_acceRefineActivedSlot[acceSlot];
}

void CPythonPlayer::SetHideSashes(bool value)
{
	m_bHideSashes = value;
}

bool CPythonPlayer::IsHideSashes()
{
	return m_bHideSashes;
}

#endif

#ifdef ENABLE_FAKEBUFF
void CPythonPlayer::SetFakeBuffSkill(DWORD dwSkillVnum, BYTE bLevel)
{
	m_map_FakeBuffSkill[dwSkillVnum] = bLevel;
}

BYTE CPythonPlayer::GetFakeBuffSkillLevel(DWORD dwSkillVnum)
{
	auto it = m_map_FakeBuffSkill.find(dwSkillVnum);
	if (it == m_map_FakeBuffSkill.end())
		return 0;

	return it->second;
}
#endif

void CPythonPlayer::NEW_ClearSkillData(bool bAll)
{
	std::map<DWORD, DWORD>::iterator it;

	for (it = m_skillSlotDict.begin(); it != m_skillSlotDict.end();)
	{
		if (bAll || __GetSkillType(it->first) == CPythonSkill::SKILL_TYPE_ACTIVE)
			it = m_skillSlotDict.erase(it);
		else
			++it;
	}

	for (int i = 0; i < SKILL_MAX_NUM; ++i)
	{
		ZeroMemory(&m_playerStatus.aSkill[i], sizeof(TSkillInstance));
	}

	for (int j = 0; j < SKILL_MAX_NUM; ++j)
	{
		// 2004.09.30.myevan.½ºÅ³°»½Å½Ã ½ºÅ³ Æ÷ÀÎÆ®¾÷[+] ¹öÆ°ÀÌ ¾È³ª¿Í Ã³¸®
		m_playerStatus.aSkill[j].iGrade = 0;
		m_playerStatus.aSkill[j].fcurEfficientPercentage=0.0f;
		m_playerStatus.aSkill[j].fnextEfficientPercentage=0.05f;
	}

	if (m_ppyGameWindow)
		PyCallClassMemberFunc(m_ppyGameWindow, "BINARY_CheckGameButton", Py_BuildNone());
}

#ifdef ENABLE_ATTRTREE
void CPythonPlayer::ClearAttrtreeLevel()
{
	ZeroMemory(m_aAttrtreeLevel, sizeof(m_aAttrtreeLevel));
}

void CPythonPlayer::SetAttrtreeLevel(BYTE row, BYTE col, BYTE level)
{
	if (row >= ATTRTREE_ROW_NUM || col >= ATTRTREE_COL_NUM)
	{
		TraceError("SetAttrtreeLevel: invalid cell (%u, %u)", row, col);
		return;
	}

	m_aAttrtreeLevel[row][col] = level;
}

BYTE CPythonPlayer::GetAttrtreeLevel(BYTE row, BYTE col) const
{
	if (row >= ATTRTREE_ROW_NUM || col >= ATTRTREE_COL_NUM)
	{
		TraceError("GetAttrtreeLevel: invalid cell (%u, %u)", row, col);
		return 0;
	}

	return m_aAttrtreeLevel[row][col];
}

BYTE CPythonPlayer::AttrtreeCellToID(BYTE row, BYTE col) const
{
	return row * ATTRTREE_COL_NUM + col;
}

void CPythonPlayer::AttrtreeIDToCell(BYTE id, BYTE& row, BYTE& col) const
{
	row = id / ATTRTREE_COL_NUM;
	col = id % ATTRTREE_COL_NUM;
}
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
size_t CPythonPlayer::GetCostumeBonusTransferSize()
{
	return m_CostumeBonusTransferItemInstanceVector.size();
}

bool CPythonPlayer::GetCostumeBonusTransferSlotItemID(BYTE bySlotIndex, DWORD * pdwItemID)
{
	if (bySlotIndex >= m_CostumeBonusTransferItemInstanceVector.size())
	{
		TraceError("CPythonPlayer::GetCostumeBonusTransferSlotItemID(bySlotIndex=%u) - Strange slot index", bySlotIndex);
		return false;
	}

	*pdwItemID = m_CostumeBonusTransferItemInstanceVector[bySlotIndex].vnum();

	return true;
}

bool CPythonPlayer::GetCostumeBonusTransferItemDataPtr(BYTE bySlotIndex, network::TItemData ** ppInstance)
{
	if (bySlotIndex >= m_CostumeBonusTransferItemInstanceVector.size())
	{
		TraceError("CPythonPlayer::GetCostumeBonusTransferItemDataPtr(bySlotIndex=%u) - Strange slot index", bySlotIndex);
		return false;
	}

	*ppInstance = &m_CostumeBonusTransferItemInstanceVector[bySlotIndex];

	return true;
}

bool CPythonPlayer::IsEmtpyCostumeBonusTransferWindow()
{
	for (size_t i = 0; i < m_CostumeBonusTransferItemInstanceVector.size(); ++i)
	{
		if (m_CostumeBonusTransferItemInstanceVector[i].vnum())
			return false;
	}

	return true;
}

BYTE CPythonPlayer::GetCurrentCostumeBonusTransferItemCount()
{
	BYTE byCurCount = 0;
	for (BYTE i = 0; i < CBT_SLOT_MAX; ++i)
	{
		if (m_CostumeBonusTransferItemInstanceVector[i].vnum())
			++byCurCount;
	}

	return byCurCount;
}

void CPythonPlayer::SetCostumeBonusTransferItemData(BYTE bySlotIndex, const network::TItemData & rItemInstance)
{
	if (bySlotIndex >= m_CostumeBonusTransferItemInstanceVector.size())
	{
		TraceError("CPythonPlayer::SetCostumeBonusTransferItemData(bySlotIndex=%u) - Strange slot index", bySlotIndex);
		return;
	}

	m_CostumeBonusTransferItemInstanceVector[bySlotIndex] = rItemInstance;
}

void CPythonPlayer::DelCostumeBonusTransferItemData(BYTE bySlotIndex)
{
	if (bySlotIndex >= m_CostumeBonusTransferItemInstanceVector.size())
	{
		TraceError("CPythonPlayer::DelCostumeBonusTransferItemData(bySlotIndex=%u) - Strange slot index", bySlotIndex);
		return;
	}

	auto & rInstance = m_CostumeBonusTransferItemInstanceVector[bySlotIndex];
	CItemData * pItemData;
	if (CItemManager::instance().GetItemDataPointer(rInstance.vnum(), &pItemData))
	{
		if (bySlotIndex == CBT_SLOT_MATERIAL || bySlotIndex == CBT_SLOT_TARGET)
			DelCostumeBonusTransferItemData(CBT_SLOT_RESULT);

		rInstance.Clear();
	}
}

void CPythonPlayer::SetCostumeBonusTransferWindowClose()
{
	m_bCostumeBonusTransferWindowIsOpen = false;
	DelCostumeBonusTransferItemData(CBT_SLOT_MEDIUM);
	DelCostumeBonusTransferItemData(CBT_SLOT_MATERIAL);
	DelCostumeBonusTransferItemData(CBT_SLOT_TARGET);
}
#endif

void CPythonPlayer::ClearSkillDict()
{
	// ClearSkillDict
	m_skillSlotDict.clear();

	// Game End - Player Data Reset
	m_isOpenPrivateShop = false;
	m_isObserverMode = false;

	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = 0.0f;

	m_bMobileFlag = FALSE;

	__ClearAutoAttackTargetActorID();
}

void CPythonPlayer::Clear()
{
	m_playerStatus.Clear();
	NEW_ClearSkillData(true);

	m_bisProcessingEmotion = FALSE;

	m_dwSendingTargetVID = 0;
	m_fTargetUpdateTime = 0.0f;

	// Test Code for Status Interface
	m_stName = "";
	m_dwMainCharacterIndex = 0;
	m_dwRace = 0;
	m_dwWeaponMinPower = 0;
	m_dwWeaponMaxPower = 0;
	m_dwWeaponMinMagicPower = 0;
	m_dwWeaponMaxMagicPower = 0;
	m_dwWeaponAddPower = 0;

	/////
	m_MovingCursorPosition = TPixelPosition(0, 0, 0);
	m_fMovingCursorSettingTime = 0.0f;

	m_eReservedMode = MODE_NONE;
	m_fReservedDelayTime = 0.0f;
	m_kPPosReserved = TPixelPosition(0, 0, 0);
	m_dwVIDReserved = 0;
	m_dwIIDReserved = 0;
	m_dwSkillSlotIndexReserved = 0;
	m_dwSkillRangeReserved = 0;

	m_isUp = false;
	m_isDown = false;
	m_isLeft = false;
	m_isRight = false;
	m_isSmtMov = false;
	m_isDirMov = false;
	m_isDirKey = false;
	m_isAtkKey = false;

	m_isCmrRot = true;
	m_fCmrRotSpd = 20.0f;

	m_iComboOld = 0;

	m_dwVIDPicked=0;
	m_dwIIDPicked=0;

	m_dwcurSkillSlotIndex = DWORD(-1);

	m_dwTargetVID = 0;
	m_dwTargetEndTime = 0;

	m_PartyMemberMap.clear();

	m_ChallengeInstanceSet.clear();
	m_RevengeInstanceSet.clear();

	m_isOpenPrivateShop = false;
	m_isObserverMode = false;

	m_isConsumingStamina = FALSE;
	m_fConsumeStaminaPerSec = 0.0f;
	m_fCurrentStamina = 0.0f;

	m_inGuildAreaID = 0xffffffff;

	m_bMobileFlag = FALSE;

	__ClearAutoAttackTargetActorID();

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_ItemAcceInstanceVector.clear();
	m_ItemAcceInstanceVector.resize(3);

	for (DWORD i = 0; i < m_ItemAcceInstanceVector.size(); ++i)
	{
		auto & rInstance = m_ItemAcceInstanceVector[i];
		rInstance.Clear();
	}
	m_acceRefineWindowIsOpen = false;
	m_acceRefineWindowType = 3;
	m_acceRefineActivedSlot[0] = -1;
	m_acceRefineActivedSlot[1] = -1;
	m_acceRefineActivedSlot[2] = -1;
#endif

#ifdef ENABLE_ANIMAL_SYSTEM
	m_bSelectedAnimalType = ANIMAL_TYPE_UNKNOWN;
	ZeroMemory(m_bAnimalSummoned, sizeof(m_bAnimalSummoned));
	ZeroMemory(m_bAnimalLevel, sizeof(m_bAnimalLevel));
	ZeroMemory(m_llAnimalEXP, sizeof(m_llAnimalEXP));
	ZeroMemory(m_llAnimalMaxEXP, sizeof(m_llAnimalMaxEXP));
	ZeroMemory(m_bAnimalStatPoints, sizeof(m_bAnimalStatPoints));
	ZeroMemory(m_sAnimalStats, sizeof(m_sAnimalStats));
#endif

#ifdef ENABLE_FAKEBUFF
	m_map_FakeBuffSkill.clear();
#endif

#ifdef ENABLE_ATTRTREE
	ClearAttrtreeLevel();
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
	m_CostumeBonusTransferItemInstanceVector.clear();
	m_CostumeBonusTransferItemInstanceVector.resize(CBT_SLOT_MAX);
	for (size_t i = 0; i < m_CostumeBonusTransferItemInstanceVector.size(); ++i)
	{
		auto &rCBTItemInstance = m_CostumeBonusTransferItemInstanceVector[i];
		rCBTItemInstance.Clear();
	}

	m_bCostumeBonusTransferWindowIsOpen = false;
#endif

	m_bIsShowTeamler = false;
}

void CPythonPlayer::LeaveGamePhase()
{
	m_iPVPTeam = -1;
	
#ifdef ENABLE_EQUIPMENT_CHANGER
	ClearEquipmentPages();
#endif
}

CPythonPlayer::CPythonPlayer(void)
{
	SetMovableGroundDistance(40.0f);

	// AffectIndex To SkillIndex
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_JEONGWI), 3));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GEOMGYEONG), 4));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GEOMGYEONG_PERFECT), 4));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_CHEONGEUN), 19));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_CHEONGEUN_PERFECT), 19));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GYEONGGONG), 49));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_EUNHYEONG), 34));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GONGPO), 64));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GONGPO_PERFECT), 64));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_JUMAGAP), 65));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_HOSIN), 94));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_HOSIN_PERFECT), 94));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_BOHO), 95));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_KWAESOK), 110));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GICHEON), 96));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GICHEON_PERFECT), 96));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_JEUNGRYEOK), 111));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_PABEOP), 66));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_FALLEN_CHEONGEUN), 19));
	/////
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GWIGEOM), 63));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_GWIGEOM_PERFECT), 63));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_MUYEONG), 78));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_HEUKSIN), 79));
	m_kMap_dwAffectIndexToSkillIndex.insert(make_pair(int(CInstanceBase::AFFECT_HEUKSIN_PERFECT), 79));

	m_ppyGameWindow = NULL;

	m_sysIsCoolTime = TRUE;
	m_sysIsLevelLimit = TRUE;
	m_dwPlayTime = 0;

	m_aeMBFButton[MBT_LEFT]=CPythonPlayer::MBF_SMART;
	m_aeMBFButton[MBT_RIGHT]=CPythonPlayer::MBF_CAMERA;
	m_aeMBFButton[MBT_MIDDLE]=CPythonPlayer::MBF_CAMERA;

	memset(m_adwEffect, 0, sizeof(m_adwEffect));

	m_isDestPosition = FALSE;
	m_ixDestPos = 0;
	m_iyDestPos = 0;
	m_iLastAlarmTime = 0;

	Clear();

	m_iPVPTeam = -1;

	m_wInventoryMaxNum = 0;
	m_wUppitemInventoryMaxNum = 0;
	m_wSkillbookInventoryMaxNum = 0;
	m_wStoneInventoryMaxNum = 0;
	m_wEnchantInventoryMaxNum = 0;

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_bHideSashes = false;//bool(CPythonConfig::Instance().GetInteger(CPythonConfig::CLASS_PLAYER, "hide_costume_acce"));
#endif
	
#ifdef ENABLE_EQUIPMENT_CHANGER
	m_dwEquipmentPageSelected = 0;
#endif
}

CPythonPlayer::~CPythonPlayer(void)
{
}

#ifdef ENABLE_EQUIPMENT_CHANGER
void CPythonPlayer::ClearEquipmentPages()
{
	m_dwEquipmentPageSelected = 0;
	m_vec_EquipmentPageInfo.clear();
}

void CPythonPlayer::AddEquipmentPage(const network::TEquipmentPageInfo& c_rkInfo)
{
	m_vec_EquipmentPageInfo.push_back(c_rkInfo);
}

void CPythonPlayer::RemoveEquipmentPage(DWORD dwIndex)
{
	if (dwIndex >= m_vec_EquipmentPageInfo.size())
		return;

	m_vec_EquipmentPageInfo.erase(m_vec_EquipmentPageInfo.begin() + dwIndex);
}

network::TEquipmentPageInfo* CPythonPlayer::GetEquipmentPageInfo(DWORD dwIndex)
{
	if (dwIndex >= m_vec_EquipmentPageInfo.size())
		return NULL;

	return &m_vec_EquipmentPageInfo[dwIndex];
}

DWORD CPythonPlayer::GetEquipmentPageCount() const
{
	return m_vec_EquipmentPageInfo.size();
}
#endif
