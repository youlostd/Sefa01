#pragma once

#include "AbstractPlayer.h"
#include "Packet.h"
#include "PythonSkill.h"
#include "protobuf_data_player.h"

class CInstanceBase;

/*
 *	메인 캐릭터 (자신이 조정하는 캐릭터) 가 가진 정보들을 관리한다.
 *
 * 2003-01-12 Levites	본래는 CPythonCharacter가 가지고 있었지만 규모가 너무 커져 버린데다
 *						위치도 애매해서 따로 분리
 * 2003-07-19 Levites	메인 캐릭터의 이동 처리 CharacterInstance에서 떼어다 붙임
 *						기존의 데이타 보존의 역할에서 완벽한 메인 플레이어 제어 클래스로
 *						탈바꿈 함.
 */

enum
{
	MAIN_RACE_WARRIOR_M,
	MAIN_RACE_ASSASSIN_W,
	MAIN_RACE_SURA_M,
	MAIN_RACE_SHAMAN_W,
	MAIN_RACE_WARRIOR_W,
	MAIN_RACE_ASSASSIN_M,
	MAIN_RACE_SURA_W,
	MAIN_RACE_SHAMAN_M,
#ifdef ENABLE_WOLFMAN
	MAIN_RACE_WOLFMAN_M,
#endif
	MAIN_RACE_MAX_NUM,
};

class CPythonPlayer : public CSingleton<CPythonPlayer>, public IAbstractPlayer
{
	public:
		enum
		{
			CATEGORY_NONE		= 0,
			CATEGORY_ACTIVE		= 1,
			CATEGORY_PASSIVE	= 2,
			CATEGORY_MAX_NUM	= 3,

			STATUS_INDEX_ST = 1,
			STATUS_INDEX_DX = 2,
			STATUS_INDEX_IQ = 3,
			STATUS_INDEX_HT = 4,
		};

		enum
		{
			MBT_LEFT,
			MBT_RIGHT,
			MBT_MIDDLE,
			MBT_NUM,
		};

		enum
		{
			MBF_SMART,
			MBF_MOVE,
			MBF_CAMERA,
			MBF_ATTACK,
			MBF_SKILL,
			MBF_AUTO,
		};

		enum
		{
			MBS_CLICK,
			MBS_PRESS,
			MBS_PRESS_SHOPDECO,
		};

		enum EMode
		{
			MODE_NONE,
			MODE_CLICK_POSITION,
			MODE_CLICK_ITEM,
			MODE_CLICK_ACTOR,
			MODE_USE_SKILL,
		};

		enum EEffect
		{
			EFFECT_PICK,
			EFFECT_NUM,
		};

		enum EMetinSocketType
		{
			METIN_SOCKET_TYPE_NONE,
			METIN_SOCKET_TYPE_SILVER,
			METIN_SOCKET_TYPE_GOLD,
#ifdef PROMETA
			METIN_SOCKET_TYPE_ACCE,
#endif
		};

		typedef struct SSkillInstance
		{
			DWORD dwIndex;
			int iType;
			int iGrade;
			int iLevel;
			float fcurEfficientPercentage;
			float fnextEfficientPercentage;
			BOOL isCoolTime;

			float fCoolTime;			// NOTE : 쿨타임 중인 스킬 슬롯을
			float fLastUsedTime;		//        퀵창에 등록할 때 사용하는 변수
			BOOL bActive;
		} TSkillInstance;

		enum EKeyBoard_UD
		{
			KEYBOARD_UD_NONE,
			KEYBOARD_UD_UP,
			KEYBOARD_UD_DOWN,
		};

		enum EKeyBoard_LR
		{
			KEYBOARD_LR_NONE,
			KEYBOARD_LR_LEFT,
			KEYBOARD_LR_RIGHT,
		};

		enum
		{
			DIR_UP,
			DIR_DOWN,
			DIR_LEFT,
			DIR_RIGHT,
		};

		typedef struct SPlayerStatus
		{
			void Clear()
			{
				for (auto& item : aItem)
					item.Clear();
#ifdef ENABLE_DRAGONSOUL
				for (auto& item : aDSItem)
					item.Clear();
#endif
				for (auto& qslot : aQuickSlot)
					qslot.Clear();

				ZeroMemory(aSkill, sizeof(aSkill));
				ZeroMemory(m_allPoint, sizeof(m_allPoint));
				ZeroMemory(m_allRealPoint, sizeof(m_allRealPoint));
				lQuickPageIndex = 0;
			}

			network::TItemData	aItem[c_Inventory_Count];
#ifdef ENABLE_DRAGONSOUL
			network::TItemData	aDSItem[c_DragonSoul_Inventory_Count];
#endif
			TQuickslot			aQuickSlot[QUICKSLOT_MAX_NUM];
			TSkillInstance		aSkill[SKILL_MAX_NUM];
			LONGLONG			m_allPoint[POINT_MAX_NUM];
			LONGLONG			m_allRealPoint[POINT_MAX_NUM];
			long				lQuickPageIndex;

			void SetPoint(UINT ePoint, LONGLONG lPoint);
			LONGLONG GetPoint(UINT ePoint);
			void SetRealPoint(UINT ePoint, LONGLONG lPoint);
			LONGLONG GetRealPoint(UINT ePoint);
		} TPlayerStatus;

		typedef struct SPartyMemberInfo
		{
			SPartyMemberInfo(DWORD _dwPID, const char * c_szName) : dwPID(_dwPID), strName(c_szName), dwVID(0) {}

			DWORD dwVID;
			DWORD dwPID;
			std::string strName;
			bool bLeader;
			BYTE byState;
			BYTE byHPPercentage;
			short sAffects[PARTY_AFFECT_SLOT_MAX_NUM];
		} TPartyMemberInfo;

		enum EPartyRole
		{
			PARTY_ROLE_NORMAL,
			PARTY_ROLE_ATTACKER,
			PARTY_ROLE_TANKER,
			PARTY_ROLE_BUFFER,
			PARTY_ROLE_SKILL_MASTER,
			PARTY_ROLE_BERSERKER,
			PARTY_ROLE_DEFENDER,
			PARTY_ROLE_MAX_NUM,
		};

		enum
		{
			SKILL_NORMAL,
			SKILL_MASTER,
			SKILL_GRAND_MASTER,
			SKILL_PERFECT_MASTER,
			SKILL_LEGENDARY_MASTER,
		};

		// 자동물약 상태 관련 특화 구조체.. 이런식의 특화 처리 작업을 안 하려고 최대한 노력했지만 실패하고 결국 특화처리.
		struct SAutoPotionInfo
		{
			SAutoPotionInfo() : bActivated(false), totalAmount(0), currentAmount(0) {}

			bool bActivated;					// 활성화 되었는가?			
			long currentAmount;					// 현재 남은 양
			long totalAmount;					// 전체 양
			long inventorySlotIndex;			// 사용중인 아이템의 인벤토리상 슬롯 인덱스
		};

		enum EAutoPotionType
		{
			AUTO_POTION_TYPE_HP = 0,
			AUTO_POTION_TYPE_SP = 1,
			AUTO_POTION_TYPE_NUM
		};

	public:
		CPythonPlayer(void);
		virtual ~CPythonPlayer(void);

		void	PickCloseMoney();
		void	PickCloseItem();

		void	SetGameWindow(PyObject * ppyObject);

		void	SetObserverMode(bool isEnable);
		bool	IsObserverMode();

		void	SetQuickCameraMode(bool isEnable);

		void	SetAttackKeyState(bool isPress);

		void	NEW_GetMainActorPosition(TPixelPosition* pkPPosActor);

		bool	RegisterEffect(DWORD dwEID, const char* c_szEftFileName, bool isCache);

		bool	NEW_SetMouseState(int eMBType, int eMBState);
		bool	NEW_SetMouseFunc(int eMBType, int eMBFunc);
		int		NEW_GetMouseFunc(int eMBT);
		void	NEW_SetMouseMiddleButtonState(int eMBState);

		void	NEW_SetAutoCameraRotationSpeed(float fRotSpd);
		void	NEW_ResetCameraRotation();

		void	NEW_SetSingleDirKeyState(int eDirKey, bool isPress);
		void	NEW_SetSingleDIKKeyState(int eDIKKey, bool isPress);
		void	NEW_SetMultiDirKeyState(bool isLeft, bool isRight, bool isUp, bool isDown);

		void	NEW_Attack();
		void	NEW_Fishing();
		bool	NEW_CancelFishing();

		void	NEW_LookAtFocusActor();
		bool	NEW_IsAttackableDistanceFocusActor();


		bool	NEW_MoveToDestPixelPositionDirection(const TPixelPosition& c_rkPPosDst);
		bool	NEW_MoveToMousePickedDirection();
		bool	NEW_MoveToMouseScreenDirection();
		bool	NEW_MoveToDirection(float fDirRot);
		void	NEW_Stop();


		// Reserved
		bool	NEW_IsEmptyReservedDelayTime(float fElapsedtime);	// 네이밍 교정 논의 필요 - [levites]


		// Dungeon
		void	SetDungeonDestinationPosition(int ix, int iy);
		void	AlarmHaveToGo();


		CInstanceBase* NEW_FindActorPtr(DWORD dwVID);
		CInstanceBase* NEW_GetMainActorPtr();

		// flying target set
		void	Clear();
		void	ClearSkillDict(); // 없어지거나 ClearGame 쪽으로 포함될 함수
		void	NEW_ClearSkillData(bool bAll = false);

		void	Update();


		// Play Time
		DWORD	GetPlayTime();
		void	SetPlayTime(DWORD dwPlayTime);


		// System
		void	SetMainCharacterIndex(int iIndex);

		DWORD	GetMainCharacterIndex();
		bool	IsMainCharacterIndex(DWORD dwIndex);
		DWORD	GetGuildID();
		void	NotifyDeletingCharacterInstance(DWORD dwVID);
		void	NotifyCharacterDead(DWORD dwVID);
		void	NotifyCharacterUpdate(DWORD dwVID);
		void	NotifyDeadMainCharacter();
		void	NotifyChangePKMode();


		// Player Status
		const char *	GetName();
		void	SetName(const char *name);
		
		void	SetRace(DWORD dwRace);
		DWORD	GetRace();

		void	SetWeaponPower(DWORD dwMinPower, DWORD dwMaxPower, DWORD dwMinMagicPower, DWORD dwMaxMagicPower, DWORD dwAddPower);
		void	SetStatus(DWORD dwType, LONGLONG lValue);
		LONGLONG	GetStatus(DWORD dwType);
		void		SetRealStatus(DWORD dwType, LONGLONG llValue);
		LONGLONG	GetRealStatus(DWORD dwType);


		// Item
		void	MoveItemData(::TItemPos SrcCell, ::TItemPos DstCell);
		void	SetItemData(::TItemPos Cell, const network::TItemData & c_rkItemInst);
		const network::TItemData * GetItemData(::TItemPos Cell) const;
#ifdef INCREASE_ITEM_STACK
		void	SetItemCount(::TItemPos Cell, WORD byCount);
#else
		void	SetItemCount(::TItemPos Cell, BYTE byCount);
#endif
		void	SetItemMetinSocket(::TItemPos Cell, DWORD dwMetinSocketIndex, DWORD dwMetinNumber);
		void	SetItemAttribute(::TItemPos Cell, DWORD dwAttrIndex, BYTE byType, short sValue);
		DWORD	GetItemIndex(::TItemPos Cell);
		DWORD	GetItemCount(::TItemPos Cell);
		DWORD	GetItemCountByVnum(DWORD dwVnum);
		DWORD	GetItemCountByVnumRange(DWORD dwVnumStart, DWORD dwVnumEnd);
#ifdef ENABLE_ALPHA_EQUIP
		int		GetItemAlphaEquipValue(::TItemPos Cell);
#endif
		DWORD	GetItemMetinSocket(::TItemPos Cell, DWORD dwMetinSocketIndex);
		void	GetItemAttribute(::TItemPos Cell, DWORD dwAttrSlotIndex, BYTE * pbyType, short * psValue);
		void	SendClickItemPacket(DWORD dwIID, bool bIgnorePickupTime = false);

		void	RequestAddLocalQuickSlot(DWORD dwLocalSlotIndex, DWORD dwWndType, DWORD dwWndItemPos);
		void	RequestAddToEmptyLocalQuickSlot(DWORD dwWndType, DWORD dwWndItemPos);
		void	RequestMoveGlobalQuickSlotToLocalQuickSlot(DWORD dwGlobalSrcSlotIndex, DWORD dwLocalDstSlotIndex);
		void	RequestDeleteGlobalQuickSlot(DWORD dwGlobalSlotIndex);
		void	RequestUseLocalQuickSlot(DWORD dwLocalSlotIndex);
		DWORD	LocalQuickSlotIndexToGlobalQuickSlotIndex(DWORD dwLocalSlotIndex);

		void	GetGlobalQuickSlotData(DWORD dwGlobalSlotIndex, DWORD* pdwWndType, DWORD* pdwWndItemPos);
		void	GetLocalQuickSlotData(DWORD dwSlotPos, DWORD* pdwWndType, DWORD* pdwWndItemPos);
		void	RemoveQuickSlotByValue(int iType, int iPosition);

		char	IsItem(::TItemPos SlotIndex);

		bool	IsInventorySlot(::TItemPos SlotIndex);
		bool	IsEquipmentSlot(::TItemPos SlotIndex);

		bool	IsEquipItemInSlot(::TItemPos iSlotIndex);


		// Quickslot
		int		GetQuickPage();
		void	SetQuickPage(int nPageIndex);
		void	AddQuickSlot(int QuickslotIndex, char IconType, WORD IconPosition);
		void	DeleteQuickSlot(int QuickslotIndex);
		void	MoveQuickSlot(int Source, int Target);


		// Skill
		void	SetSkill(DWORD dwSlotIndex, DWORD dwSkillIndex);
		bool	GetSkillSlotIndex(DWORD dwSkillIndex, DWORD* pdwSlotIndex);
		int		GetSkillIndex(DWORD dwSlotIndex);
		int		GetSkillGrade(DWORD dwSlotIndex);
		int		GetSkillLevel(DWORD dwSlotIndex);
		float	GetSkillCurrentEfficientPercentage(DWORD dwSlotIndex);
		float	GetSkillNextEfficientPercentage(DWORD dwSlotIndex);
		void	SetSkillLevel(DWORD dwSlotIndex, DWORD dwSkillLevel);
		void	SetSkillLevel_(DWORD dwSkillIndex, DWORD dwSkillGrade, DWORD dwSkillLevel);
		BOOL	IsToggleSkill(DWORD dwSlotIndex);
		void	ClickSkillSlot(DWORD dwSlotIndex);
		void	ChangeCurrentSkillNumberOnly(DWORD dwSlotIndex);
		bool	FindSkillSlotIndexBySkillIndex(DWORD dwSkillIndex, DWORD * pdwSkillSlotIndex);

		void	SetSkillCoolTime(DWORD dwSkillIndex);
		void	EndSkillCoolTime(DWORD dwSkillIndex);

		float	GetSkillCoolTime(DWORD dwSlotIndex);
		float	GetSkillElapsedCoolTime(DWORD dwSlotIndex);
		BOOL	IsSkillActive(DWORD dwSlotIndex);
		BOOL	IsSkillCoolTime(DWORD dwSlotIndex);
		void	UseGuildSkill(DWORD dwSkillSlotIndex);
		bool	AffectIndexToSkillSlotIndex(UINT uAffect, DWORD* pdwSkillSlotIndex);
		bool	AffectIndexToSkillIndex(DWORD dwAffectIndex, DWORD * pdwSkillIndex);
		bool	SkillIndexToAffectIndex(DWORD dwSkillIndex, DWORD * pdwAffectIndex, int iIgnoreCount = 0);

		void	SetAffect(UINT uAffect);
		void	ResetAffect(UINT uAffect);
		void	ClearAffects();


		// Target
		void	SetTarget(DWORD dwVID, BOOL bForceChange = TRUE);
		void	OpenCharacterMenu(DWORD dwVictimActorID);
		DWORD	GetTargetVID();


		// Party
		void	ExitParty();
		void	AppendPartyMember(DWORD dwPID, const char * c_szName);
		void	LinkPartyMember(DWORD dwPID, DWORD dwVID);
		void	UnlinkPartyMember(DWORD dwPID);
		void	UpdatePartyMemberInfo(DWORD dwPID, bool bLeader, BYTE byState, BYTE byHPPercentage);
		void	UpdatePartyMemberAffect(DWORD dwPID, BYTE byAffectSlotIndex, short sAffectNumber);
		void	RemovePartyMember(DWORD dwPID);
		bool	IsPartyMemberByVID(DWORD dwVID);
		bool	IsPartyMemberByName(const char * c_szName);
		bool	GetPartyMemberPtr(DWORD dwPID, TPartyMemberInfo ** ppPartyMemberInfo);
		bool	PartyMemberPIDToVID(DWORD dwPID, DWORD * pdwVID);
		bool	PartyMemberVIDToPID(DWORD dwVID, DWORD * pdwPID);
		bool	IsSamePartyMember(DWORD dwVID1, DWORD dwVID2);


		// Fight
		void	RememberChallengeInstance(DWORD dwVID);
		void	RememberRevengeInstance(DWORD dwVID);
		void	RememberCantFightInstance(DWORD dwVID);
		void	ForgetInstance(DWORD dwVID);
		bool	IsChallengeInstance(DWORD dwVID);
		bool	IsRevengeInstance(DWORD dwVID);
		bool	IsCantFightInstance(DWORD dwVID);


		// Private Shop
		void	OpenPrivateShop();
		void	ClosePrivateShop();
		bool	IsOpenPrivateShop();



		// Stamina
		void	StartStaminaConsume(DWORD dwConsumePerSec, DWORD dwCurrentStamina);
		void	StopStaminaConsume(DWORD dwCurrentStamina);


		// PK Mode
		DWORD	GetPKMode();


		// Mobile
		void	SetMobileFlag(BOOL bFlag);
		BOOL	HasMobilePhoneNumber();


		// Combo
		void	SetComboSkillFlag(BOOL bFlag);


		// System
		void	SetMovableGroundDistance(float fDistance);


		// Emotion
		void	ActEmotion(DWORD dwEmotionID);
		void	StartEmotionProcess();
		void	EndEmotionProcess();


		// Function Only For Console System
		BOOL	__ToggleCoolTime();
		BOOL	__ToggleLevelLimit();
#ifdef ENABLE_RUNE_SYSTEM
		void	ResetCooltime(DWORD dwSkillVnum);
#endif

		__inline const	SAutoPotionInfo& GetAutoPotionInfo(int type) const	{ return m_kAutoPotionInfo[type]; }
		__inline		SAutoPotionInfo& GetAutoPotionInfo(int type)		{ return m_kAutoPotionInfo[type]; }
		__inline void					 SetAutoPotionInfo(int type, const SAutoPotionInfo& info)	{ m_kAutoPotionInfo[type] = info; }		

	protected:
		TQuickslot &	__RefLocalQuickSlot(int SlotIndex);
		TQuickslot &	__RefGlobalQuickSlot(int SlotIndex);


		DWORD	__GetLevelAtk();
		DWORD	__GetStatAtk();
		DWORD	__GetWeaponAtk(DWORD dwWeaponPower);		
		DWORD	__GetTotalAtk(DWORD dwWeaponPower, DWORD dwRefineBonus);
		DWORD	__GetRaceStat();		
		DWORD	__GetHitRate();
		DWORD	__GetEvadeRate();

		void	__UpdateBattleStatus();

		void	__DeactivateSkillSlot(DWORD dwSlotIndex);
		void	__ActivateSkillSlot(DWORD dwSlotIndex);

		void	__OnPressSmart(CInstanceBase& rkInstMain, bool isAuto);
		void	__OnClickSmart(CInstanceBase& rkInstMain, bool isAuto);

		void	__OnPressItem(CInstanceBase& rkInstMain, DWORD dwPickedItemID);
		void	__OnPressActor(CInstanceBase& rkInstMain, DWORD dwPickedActorID, bool isAuto);
		void	__OnPressGround(CInstanceBase& rkInstMain, const TPixelPosition& c_rkPPosPickedGround);
		void	__OnPressScreen(CInstanceBase& rkInstMain);

		void	__OnClickActor(CInstanceBase& rkInstMain, DWORD dwPickedActorID, bool isAuto);
		void	__OnClickItem(CInstanceBase& rkInstMain, DWORD dwPickedItemID);
		void	__OnClickGround(CInstanceBase& rkInstMain, const TPixelPosition& c_rkPPosPickedGround);

		bool	__IsMovableGroundDistance(CInstanceBase& rkInstMain, const TPixelPosition& c_rkPPosPickedGround);

		bool	__GetPickedActorPtr(CInstanceBase** pkInstPicked);

		bool	__GetPickedActorID(DWORD* pdwActorID);
		bool	__GetPickedItemID(DWORD* pdwItemID);
		bool	__GetPickedGroundPos(TPixelPosition* pkPPosPicked);

		void	__ClearReservedAction();
		void	__ReserveClickItem(DWORD dwItemID);
		void	__ReserveClickActor(DWORD dwActorID);
		void	__ReserveClickGround(const TPixelPosition& c_rkPPosPickedGround);
		void	__ReserveUseSkill(DWORD dwActorID, DWORD dwSkillSlotIndex, DWORD dwRange);

		void	__ReserveProcess_ClickActor();

		void	__ShowPickedEffect(const TPixelPosition& c_rkPPosPickedGround);
		void	__SendClickActorPacket(CInstanceBase& rkInstVictim);

		void	__ClearAutoAttackTargetActorID();
		void	__SetAutoAttackTargetActorID(DWORD dwActorID);

		void	NEW_ShowEffect(int dwEID, TPixelPosition kPPosDst);

		void	NEW_SetMouseSmartState(int eMBS, bool isAuto);
		void	NEW_SetMouseMoveState(int eMBS);
		void	NEW_SetMouseCameraState(int eMBS);
		void	NEW_GetMouseDirRotation(float fScrX, float fScrY, float* pfDirRot);
		void	NEW_GetMultiKeyDirRotation(bool isLeft, bool isRight, bool isUp, bool isDown, float* pfDirRot);

		float	GetDegreeFromDirection(int iUD, int iLR);
		float	GetDegreeFromPosition(int ix, int iy, int iHalfWidth, int iHalfHeight);

		bool	CheckCategory(int iCategory);
		bool	CheckAbilitySlot(int iSlotIndex);

		void	RefreshKeyWalkingDirection();
		void	NEW_RefreshMouseWalkingDirection();


		// Instances
		void	RefreshInstances();

		bool	__CanShot(CInstanceBase& rkInstMain, CInstanceBase& rkInstTarget);
		bool	__CanUseSkill();

		bool	__CanMove();
		
		bool	__CanAttack();
		bool	__CanChangeTarget();

		bool	__CheckSkillUsable(DWORD dwSlotIndex);
		void	__UseCurrentSkill();
		void	__UseChargeSkill(DWORD dwSkillSlotIndex);
		bool	__UseSkill(DWORD dwSlotIndex);
		bool	__CheckSpecialSkill(DWORD dwSkillIndex);

		bool	__CheckRestSkillCoolTime(DWORD dwSkillSlotIndex);
		bool	__CheckShortLife(TSkillInstance & rkSkillInst, CPythonSkill::TSkillData& rkSkillData);
		bool	__CheckShortMana(TSkillInstance & rkSkillInst, CPythonSkill::TSkillData& rkSkillData);
		bool	__CheckShortArrow(TSkillInstance & rkSkillInst, CPythonSkill::TSkillData& rkSkillData);
		bool	__CheckDashAffect(CInstanceBase& rkInstMain);

		void	__SendUseSkill(DWORD dwSkillSlotIndex, DWORD dwTargetVID);
		void	__RunCoolTime(DWORD dwSkillSlotIndex);

		BYTE	__GetSkillType(DWORD dwSkillSlotIndex);

		bool	__IsReservedUseSkill(DWORD dwSkillSlotIndex);
		bool	__IsMeleeSkill(CPythonSkill::TSkillData& rkSkillData);
		bool	__IsChargeSkill(CPythonSkill::TSkillData& rkSkillData);
#ifdef BOW_RANGE_ONLY_FOR_NINJA
		DWORD	__GetSkillTargetRange(CInstanceBase *rkInstMain, CPythonSkill::TSkillData& rkSkillData);
#else
		DWORD	__GetSkillTargetRange(CPythonSkill::TSkillData& rkSkillData);
#endif
		bool	__SearchNearTarget();
		bool	__IsUsingChargeSkill();

		bool	__ProcessEnemySkillTargetRange(CInstanceBase& rkInstMain, CInstanceBase& rkInstTarget, CPythonSkill::TSkillData& rkSkillData, DWORD dwSkillSlotIndex);


		// Item
		bool	__HasEnoughArrow();
		bool	__HasItem(DWORD dwItemID);
		DWORD	__GetPickableDistance();


		// Target
		CInstanceBase*		__GetTargetActorPtr();
		void				__ClearTarget();
		DWORD				__GetTargetVID();
		void				__SetTargetVID(DWORD dwVID);
		bool				__IsSameTargetVID(DWORD dwVID);
		bool				__IsTarget();
		bool				__ChangeTargetToPickedInstance();

		CInstanceBase *		__GetSkillTargetInstancePtr(CPythonSkill::TSkillData& rkSkillData);
		CInstanceBase *		__GetAliveTargetInstancePtr();
		CInstanceBase *		__GetDeadTargetInstancePtr();

		BOOL				__IsRightButtonSkillMode();


		// Update
		void				__Update_AutoAttack();
		void				__Update_NotifyGuildAreaEvent();

		

		// Emotion
		BOOL				__IsProcessingEmotion();


	protected:
		PyObject *				m_ppyGameWindow;

		// Client Player Data
		std::map<DWORD, DWORD>	m_skillSlotDict;
		std::string				m_stName;
		DWORD					m_dwMainCharacterIndex;		
		DWORD					m_dwRace;
		DWORD					m_dwWeaponMinPower;
		DWORD					m_dwWeaponMaxPower;
		DWORD					m_dwWeaponMinMagicPower;
		DWORD					m_dwWeaponMaxMagicPower;
		DWORD					m_dwWeaponAddPower;

		// Todo
		DWORD					m_dwSendingTargetVID;
		float					m_fTargetUpdateTime;

		// Attack
		DWORD					m_dwAutoAttackTargetVID;

		// NEW_Move
		EMode					m_eReservedMode;
		float					m_fReservedDelayTime;

		float					m_fMovDirRot;

		bool					m_isUp;
		bool					m_isDown;
		bool					m_isLeft;
		bool					m_isRight;
		bool					m_isAtkKey;
		bool					m_isDirKey;
		bool					m_isCmrRot;
		bool					m_isSmtMov;
		bool					m_isDirMov;

		float					m_fCmrRotSpd;

		TPlayerStatus			m_playerStatus;

		UINT					m_iComboOld;
		DWORD					m_dwVIDReserved;
		DWORD					m_dwIIDReserved;

		DWORD					m_dwcurSkillSlotIndex;
		DWORD					m_dwSkillSlotIndexReserved;
		DWORD					m_dwSkillRangeReserved;

		TPixelPosition			m_kPPosInstPrev;
		TPixelPosition			m_kPPosReserved;

		// Emotion
		BOOL					m_bisProcessingEmotion;

		// Dungeon
		BOOL					m_isDestPosition;
		int						m_ixDestPos;
		int						m_iyDestPos;
		int						m_iLastAlarmTime;

		// Party
		std::map<DWORD, TPartyMemberInfo>	m_PartyMemberMap;

		// PVP
		std::set<DWORD>			m_ChallengeInstanceSet;
		std::set<DWORD>			m_RevengeInstanceSet;
		std::set<DWORD>			m_CantFightInstanceSet;

		// Private Shop
		bool					m_isOpenPrivateShop;
		bool					m_isObserverMode;

		// Stamina
		BOOL					m_isConsumingStamina;
		float					m_fCurrentStamina;
		float					m_fConsumeStaminaPerSec;

		// Guild
		DWORD					m_inGuildAreaID;

		// Mobile
		BOOL					m_bMobileFlag;

		// System
		BOOL					m_sysIsCoolTime;
		BOOL					m_sysIsLevelLimit;

	protected:
		// Game Cursor Data
		TPixelPosition			m_MovingCursorPosition;
		float					m_fMovingCursorSettingTime;
		DWORD					m_adwEffect[EFFECT_NUM];

		DWORD					m_dwVIDPicked;
		DWORD					m_dwIIDPicked;
		int						m_aeMBFButton[MBT_NUM];

		DWORD					m_dwTargetVID;
		DWORD					m_dwTargetEndTime;
		DWORD					m_dwPlayTime;

		SAutoPotionInfo			m_kAutoPotionInfo[AUTO_POTION_TYPE_NUM];

	protected:
		float					MOVABLE_GROUND_DISTANCE;

	private:
		std::map<DWORD, DWORD> m_kMap_dwAffectIndexToSkillIndex;

	public:
		void					SetPVPTeam(int iTeam) { m_iPVPTeam = iTeam; }
		int						GetPVPTeam() const { return m_iPVPTeam; }
	private:
		int						m_iPVPTeam;

	public:
		void					LeaveGamePhase();

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	// Acce
	public:
		void	SetAcceItemData(DWORD dwSlotIndex, const network::TItemData & rItemInstance);
		void	DelAcceItemData(DWORD dwSlotIndex);
		int		GetCurrentAcceSize();
		BOOL	GetAcceSlotItemID(DWORD dwSlotIndex, DWORD* pdwItemID);
		BOOL	GetAcceItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance);
		BOOL	IsEmtpyAcceWindow();
		int		GetCurrentAcceItemCount();
		void	SetAcceRefineWindowOpen(int type);
		void	SetActivedAcceSlot(int acceSlot, int itemPos);
		int		FindActivedSlot(int acceSlot);
		int		FindUsingSlot(int acceSlot);
		void	SetHideSashes(bool value);
		bool	IsHideSashes();

		BYTE					m_acceRefineWindowIsOpen;
		DWORD					m_acceRefineWindowType;
		signed int				m_acceRefineActivedSlot[3];
		bool					m_bHideSashes;

	private:
		typedef std::vector<network::TItemData> TItemAcceInstanceVector;
		TItemAcceInstanceVector m_ItemAcceInstanceVector;
#endif

	public:
		void	SetInventoryMaxNum(WORD wMaxNum) { m_wInventoryMaxNum = wMaxNum; }
		WORD	GetInventoryMaxNum() const { return m_wInventoryMaxNum; }
		void	SetUppitemInventoryMaxNum(WORD wMaxNum) { m_wUppitemInventoryMaxNum = wMaxNum; }
		WORD	GetUppitemInventoryMaxNum() const { return m_wUppitemInventoryMaxNum; }
		void	SetSkillbookInventoryMaxNum(WORD wMaxNum) { m_wSkillbookInventoryMaxNum = wMaxNum; }
		WORD	GetSkillbookInventoryMaxNum() const { return m_wSkillbookInventoryMaxNum; }
		void	SetStoneInventoryMaxNum(WORD wMaxNum) { m_wStoneInventoryMaxNum = wMaxNum; }
		WORD	GetStoneInventoryMaxNum() const { return m_wStoneInventoryMaxNum; }
		void	SetEnchantInventoryMaxNum(WORD wMaxNum) { m_wEnchantInventoryMaxNum = wMaxNum; }
		WORD	GetEnchantInventoryMaxNum() const { return m_wEnchantInventoryMaxNum; }

	private:
		WORD	m_wInventoryMaxNum;
		WORD	m_wUppitemInventoryMaxNum;
		WORD	m_wSkillbookInventoryMaxNum;
		WORD	m_wStoneInventoryMaxNum;
		WORD	m_wEnchantInventoryMaxNum;

	public:
		enum EHorseData {
			HORSE_MAX_BONUS_LEVEL = 6,
			HORSE_BONUS_COUNT = 3,
		};

		typedef struct SHorseBonusProtoTable {
			BYTE	bApplyType[HORSE_BONUS_COUNT];
			DWORD	dwApplyValue[HORSE_BONUS_COUNT];
			DWORD	dwItemVnum[HORSE_BONUS_COUNT];
			BYTE	bItemCount; // INCREASE_ITEM_STACK
		} THorseBonusProtoTable;

		bool					LoadHorseBonusProto(const char* c_pszFileName);
		THorseBonusProtoTable*	GetHorseBonusProto(BYTE bLevel);

	private:
		THorseBonusProtoTable	m_aBonusProto[HORSE_MAX_BONUS_LEVEL];

#ifdef ENABLE_ANIMAL_SYSTEM
	public:
		enum EAnimalStats {
			ANIMAL_STAT_MAIN,
			ANIMAL_STAT_1,
			ANIMAL_STAT_2,
			ANIMAL_STAT_3,
			ANIMAL_STAT_4,

			ANIMAL_STAT_COUNT = 4,
		};

		void		SelectAnimalType(BYTE bType)			{ m_bSelectedAnimalType = bType; }
		BYTE		GetAnimalType() const					{ return m_bSelectedAnimalType; }
		void		SetAnimalSummoned(bool bSummoned)		{ m_bAnimalSummoned[GetAnimalType()] = bSummoned; }
		bool		IsAnimalSummoned() const				{ return m_bAnimalSummoned[GetAnimalType()]; }
		void		SetAnimalName(const char* c_pszName)	{ m_stAnimalName[GetAnimalType()] = c_pszName; };
		const char*	GetAnimalName() const					{ return m_stAnimalName[GetAnimalType()].c_str(); }
		void		SetAnimalLevel(BYTE bLevel)				{ m_bAnimalLevel[GetAnimalType()] = bLevel; }
		BYTE		GetAnimalLevel() const					{ return m_bAnimalLevel[GetAnimalType()]; }
		void		SetAnimalEXP(LONGLONG llExp)			{ m_llAnimalEXP[GetAnimalType()] = llExp; }
		LONGLONG	GetAnimalEXP() const					{ return m_llAnimalEXP[GetAnimalType()]; }
		void		SetAnimalMaxEXP(LONGLONG llExp)			{ m_llAnimalMaxEXP[GetAnimalType()] = llExp; }
		LONGLONG	GetAnimalMaxEXP() const					{ return m_llAnimalMaxEXP[GetAnimalType()]; }
		void		SetAnimalStatPoints(BYTE bPoints)		{ m_bAnimalStatPoints[GetAnimalType()] = bPoints; }
		BYTE		GetAnimalStatPoints() const				{ return m_bAnimalStatPoints[GetAnimalType()]; }
		void		SetAnimalStats(short* abStats)			{ memcpy(m_sAnimalStats[GetAnimalType()], abStats, sizeof(m_sAnimalStats[GetAnimalType()])); }
		short		GetAnimalStat(BYTE bIndex) const		{ return m_sAnimalStats[GetAnimalType()][bIndex]; }

	private:
		BYTE		m_bSelectedAnimalType;
		bool		m_bAnimalSummoned[ANIMAL_TYPE_MAX_NUM];
		std::string	m_stAnimalName[ANIMAL_TYPE_MAX_NUM];
		BYTE		m_bAnimalLevel[ANIMAL_TYPE_MAX_NUM];
		LONGLONG	m_llAnimalEXP[ANIMAL_TYPE_MAX_NUM];
		LONGLONG	m_llAnimalMaxEXP[ANIMAL_TYPE_MAX_NUM];
		BYTE		m_bAnimalStatPoints[ANIMAL_TYPE_MAX_NUM];
		short		m_sAnimalStats[ANIMAL_TYPE_MAX_NUM][5];
#endif

	public:
		void					SetIsShowTeamler(bool bIsShowTeamler)	{ m_bIsShowTeamler = bIsShowTeamler; }
		bool					IsShowTeamler() const					{ return m_bIsShowTeamler; }
	private:
		bool					m_bIsShowTeamler;

#ifdef ENABLE_FAKEBUFF
	public:
		void	SetFakeBuffSkill(DWORD dwSkillVnum, BYTE bLevel);
		BYTE	GetFakeBuffSkillLevel(DWORD dwSkillVnum);

	private:
		std::map<DWORD, BYTE>	m_map_FakeBuffSkill;
#endif

#ifdef ENABLE_ATTRTREE
	public:
		enum EAttrtreeData {
			ATTRTREE_ROW_NUM = 6,
			ATTRTREE_COL_NUM = 5,
			ATTRTREE_LEVEL_NUM = 5,
		};

		void	ClearAttrtreeLevel();
		void	SetAttrtreeLevel(BYTE row, BYTE col, BYTE level);
		BYTE	GetAttrtreeLevel(BYTE row, BYTE col) const;

		BYTE	AttrtreeCellToID(BYTE row, BYTE col) const;
		void	AttrtreeIDToCell(BYTE id, BYTE& row, BYTE& col) const;

	private:
		BYTE	m_aAttrtreeLevel[ATTRTREE_ROW_NUM][ATTRTREE_COL_NUM];
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
	//Costume Bonus Transfer
	public:
		// Window
		void		SetCostumeBonusTransferWindowOpen() { m_bCostumeBonusTransferWindowIsOpen = true; }
		bool		GetCostumeBonusTransferWindowOpen() const { return m_bCostumeBonusTransferWindowIsOpen; }
		void		SetCostumeBonusTransferWindowClose();

		// Items
		void		SetCostumeBonusTransferItemData(BYTE bySlotIndex, const network::TItemData & rItemInstance);
		void		DelCostumeBonusTransferItemData(BYTE bySlotIndex);

		size_t		GetCostumeBonusTransferSize();
		bool		GetCostumeBonusTransferSlotItemID(BYTE bySlotIndex, DWORD* pdwItemID);
		bool		GetCostumeBonusTransferItemDataPtr(BYTE bySlotIndex, network::TItemData ** ppInstance);
		bool		IsEmtpyCostumeBonusTransferWindow();
		BYTE		GetCurrentCostumeBonusTransferItemCount();

	private:
		typedef std::vector<network::TItemData> TCostumeBonusTransferItemInstanceVector;
		TCostumeBonusTransferItemInstanceVector m_CostumeBonusTransferItemInstanceVector;
	protected:
		bool		m_bCostumeBonusTransferWindowIsOpen;
#endif

#ifdef ENABLE_EQUIPMENT_CHANGER
	public:
		void				ClearEquipmentPages();
		void				SetSelectedEquipmentPage(DWORD dwIndex)	{ m_dwEquipmentPageSelected = dwIndex; }
		DWORD				GetSelectedEquipmentPage() const		{ return m_dwEquipmentPageSelected; }
		void				AddEquipmentPage(const network::TEquipmentPageInfo& c_rkInfo);
		void				RemoveEquipmentPage(DWORD dwIndex);
		network::TEquipmentPageInfo*	GetEquipmentPageInfo(DWORD dwIndex);
		DWORD				GetEquipmentPageCount() const;
	private:
		DWORD										m_dwEquipmentPageSelected;
		std::vector<network::TEquipmentPageInfo>	m_vec_EquipmentPageInfo;
#endif
};

extern const int c_iFastestSendingCount;
extern const int c_iSlowestSendingCount;
extern const float c_fFastestSendingDelay;
extern const float c_fSlowestSendingDelay;
extern const float c_fRotatingStepTime;

extern const float c_fComboDistance;
extern const float c_fPickupDistance;
extern const float c_fClickDistance;
