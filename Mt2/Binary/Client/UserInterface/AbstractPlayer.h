#pragma once

#include "AbstractSingleton.h"
#include "protobuf_data_item.h"

class CInstanceBase;

class IAbstractPlayer : public TAbstractSingleton<IAbstractPlayer>
{
	public:
		IAbstractPlayer() {}
		virtual ~IAbstractPlayer() {}

		virtual DWORD	GetMainCharacterIndex() = 0;
		virtual void	SetMainCharacterIndex(int iIndex) = 0;
		virtual bool	IsMainCharacterIndex(DWORD dwIndex) = 0;

		virtual long long	GetStatus(DWORD dwType) = 0;

		virtual const char *	GetName() = 0;

		virtual void	SetRace(DWORD dwRace) = 0;

		virtual void	StartStaminaConsume(DWORD dwConsumePerSec, DWORD dwCurrentStamina) = 0;
		virtual void	StopStaminaConsume(DWORD dwCurrentStamina) = 0;		

		virtual bool	IsPartyMemberByVID(DWORD dwVID) = 0;		
		virtual bool	PartyMemberVIDToPID(DWORD dwVID, DWORD * pdwPID) = 0;
		virtual bool	IsSamePartyMember(DWORD dwVID1, DWORD dwVID2) = 0;

		virtual void	SetItemData(::TItemPos itemPos, const network::TItemData& c_rkItemInst) = 0;
#ifdef INCREASE_ITEM_STACK
		virtual void	SetItemCount(::TItemPos itemPos, WORD byCount) = 0;
#else
		virtual void	SetItemCount(::TItemPos itemPos, BYTE byCount) = 0;
#endif
		virtual void	SetItemMetinSocket(::TItemPos itemPos, DWORD dwMetinSocketIndex, DWORD dwMetinNumber) = 0;
		virtual void	SetItemAttribute(::TItemPos itemPos, DWORD dwAttrIndex, BYTE byType, short sValue) = 0;
#if defined(GAIDEN)
		virtual void	SetItemUnbindTime(DWORD dwItemSlotIndex, DWORD dwUnbindSecondsLeft) = 0;
#endif

		virtual const network::TItemData* GetItemData(::TItemPos Cell) const = 0;
		virtual DWORD	GetItemIndex(::TItemPos itemPos) = 0;
		virtual DWORD	GetItemCount(::TItemPos itemPos) = 0;

		virtual bool	IsEquipItemInSlot(::TItemPos itemPos) = 0;

		virtual void	AddQuickSlot(int QuickslotIndex, char IconType, WORD IconPosition) = 0;
		virtual void	DeleteQuickSlot(int QuickslotIndex) = 0;
		virtual void	MoveQuickSlot(int Source, int Target) = 0;

		virtual void	SetWeaponPower(DWORD dwMinPower, DWORD dwMaxPower, DWORD dwMinMagicPower, DWORD dwMaxMagicPower, DWORD dwAddPower) = 0;

		virtual void	SetTarget(DWORD dwVID, BOOL bForceChange = TRUE) = 0;
		virtual void	NotifyCharacterUpdate(DWORD dwVID) = 0;		
		virtual void	NotifyCharacterDead(DWORD dwVID) = 0;
		virtual void	NotifyDeletingCharacterInstance(DWORD dwVID) = 0;
		virtual void	NotifyChangePKMode() = 0;
				
		virtual void	SetObserverMode(bool isEnable) = 0;
		virtual void	SetMobileFlag(BOOL bFlag) = 0;
		virtual void	SetComboSkillFlag(BOOL bFlag) = 0;

		virtual void	StartEmotionProcess() = 0;
		virtual void	EndEmotionProcess() = 0;

		virtual CInstanceBase* NEW_GetMainActorPtr() = 0;
};

