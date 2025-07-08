#pragma once

#include "../eterLib/FuncObject.h"
#include "../eterlib/NetStream.h"

#include "packet.h"
#include "protobuf_gc_packets.h"

class CInstanceBase;
class CNetworkActorManager;
struct SNetworkActorData;
struct SNetworkUpdateActorData;

class CPythonNetworkStream : public CNetworkStream, public CSingleton<CPythonNetworkStream>
{
	public:
		enum
		{
			SERVER_COMMAND_LOG_OUT = 0,
			SERVER_COMMAND_RETURN_TO_SELECT_CHARACTER = 1,
			SERVER_COMMAND_QUIT = 2,

			MAX_ACCOUNT_PLAYER
		};
		
		enum
		{
			ERROR_NONE,
			ERROR_UNKNOWN,
			ERROR_CONNECT_MARK_SERVER,			
			ERROR_LOAD_MARK,
			ERROR_MARK_WIDTH,
			ERROR_MARK_HEIGHT,

			// MARK_BUG_FIX
			ERROR_MARK_UPLOAD_NEED_RECONNECT,
			ERROR_MARK_CHECK_NEED_RECONNECT,
			// END_OF_MARK_BUG_FIX
		};

		enum
		{
			ACCOUNT_CHARACTER_SLOT_ID,
			ACCOUNT_CHARACTER_SLOT_NAME,
			ACCOUNT_CHARACTER_SLOT_RACE,
			ACCOUNT_CHARACTER_SLOT_LEVEL,
			ACCOUNT_CHARACTER_SLOT_STR,
			ACCOUNT_CHARACTER_SLOT_DEX,
			ACCOUNT_CHARACTER_SLOT_HTH,
			ACCOUNT_CHARACTER_SLOT_INT,
			ACCOUNT_CHARACTER_SLOT_PLAYTIME,
			ACCOUNT_CHARACTER_SLOT_FORM,
			ACCOUNT_CHARACTER_SLOT_ADDR,
			ACCOUNT_CHARACTER_SLOT_PORT,
			ACCOUNT_CHARACTER_SLOT_GUILD_ID,
			ACCOUNT_CHARACTER_SLOT_GUILD_NAME,
			ACCOUNT_CHARACTER_SLOT_CHANGE_NAME_FLAG,
			ACCOUNT_CHARACTER_SLOT_HAIR,
#ifdef ENABLE_HAIR_SELECTOR
			ACCOUNT_CHARACTER_SLOT_HAIR_BASE,
#endif
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			ACCOUNT_CHARACTER_SLOT_ACCE,
#endif
			ACCOUNT_CHARACTER_SLOT_LAST_PLAYTIME,
#ifdef __PRESTIGE__
			ACCOUNT_CHARACTER_SLOT_PRESTIGE,
#endif
		};

		enum
		{
			PHASE_WINDOW_LOGO,
			PHASE_WINDOW_LOGIN,
			PHASE_WINDOW_SELECT,
			PHASE_WINDOW_CREATE,
			PHASE_WINDOW_LOAD,
			PHASE_WINDOW_GAME,
			PHASE_WINDOW_EMPIRE,
			PHASE_WINDOW_NUM,
		};

	public:
		CPythonNetworkStream();
		virtual ~CPythonNetworkStream();
		
		void StartGame();
		void Warp(long lGlobalX, long lGlobalY);
		
		void NotifyHack(const char* c_szMsg);		
		void SetWaitFlag();

		void SendEmoticon(UINT eEmoticon);

		void ExitApplication();
		void ExitGame();
		void LogOutGame();
		void AbsoluteExitGame();
		void AbsoluteExitApplication();
		bool SendAttackPacket(UINT uMotAttack, DWORD dwVIDVictim);
		DWORD GetGuildID();

		UINT UploadMark(const char* c_szImageFileName);
		UINT UploadSymbol(const char* c_szImageFileName);

		bool LoadConvertTable(DWORD dwEmpireID, const char* c_szFileName);

		UINT		GetAccountCharacterSlotDatau(UINT iSlot, UINT eType);
		const char* GetAccountCharacterSlotDataz(UINT iSlot, UINT eType);

		void SendUseDetachmentSinglePacket(WORD scrollItemPos, WORD curItemPos, BYTE slotIndex);

		const char*		GetFieldMusicFileName();
		float			GetFieldMusicVolume();

		bool IsSelectedEmpire();

		void SetMarkServer(const char* c_szAddr, UINT uPort);
		void ConnectLoginServer(const char* c_szAddr, UINT uPort);
		void ConnectGameServer(UINT iChrSlot);

		void SetLoginInfo(const char* c_szID, const char* c_szPassword);
		void SetLoginKey(DWORD dwLoginKey);
		void ClearLoginInfo( void );

		void SetHandler(PyObject* poHandler);
		void SetPhaseWindow(UINT ePhaseWnd, PyObject* poPhaseWnd);
		void ClearPhaseWindow(UINT ePhaseWnd, PyObject* poPhaseWnd);
		PyObject* GetPhaseWindow(UINT ePhaseWnd);
		void SetServerCommandParserWindow(PyObject* poPhaseWnd);

		bool SendSyncPositionElementPacket(DWORD dwVictimVID, DWORD dwVictimX, DWORD dwVictimY);

		bool SendCharacterStatePacket(const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg);
		bool SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID=0);
		bool SendTargetPacket(DWORD dwVID);

#ifdef CHANGE_SKILL_COLOR
		bool SendSkillColorPacket(BYTE skill, DWORD col1, DWORD col2, DWORD col3, DWORD col4, DWORD col5);
#endif

		bool SendItemUsePacket(::TItemPos pos);
#ifdef INCREASE_ITEM_STACK
		bool SendItemMultiUsePacket(::TItemPos pos, WORD bCount);
#else
		bool SendItemMultiUsePacket(::TItemPos pos, BYTE bCount);
#endif
		bool SendItemUseToItemPacket(::TItemPos source_pos, ::TItemPos target_pos);
		bool SendItemDropPacket(::TItemPos pos, DWORD elk);
		bool SendItemDropPacketNew(::TItemPos pos, DWORD elk, DWORD count);
#ifdef INCREASE_ITEM_STACK
		bool SendItemMovePacket(::TItemPos pos, ::TItemPos change_pos, WORD num);
#else
		bool SendItemMovePacket(::TItemPos pos, ::TItemPos change_pos, BYTE num);
#endif
#ifdef INGAME_WIKI
		bool SendWikiRequestInfo(unsigned long long retID, DWORD vnum, bool isMob);
#endif
		bool SendItemPickUpPacket(DWORD vid);

		bool SendQuickSlotAddPacket(BYTE wpos, BYTE type, BYTE pos);
		bool SendQuickSlotDelPacket(BYTE wpos);
		bool SendQuickSlotMovePacket(BYTE wpos, BYTE change_pos);

		// PointReset 개 임시
		bool SendPointResetPacket();

		// Shop
		bool SendShopEndPacket();
		bool SendShopBuyPacket(BYTE byCount);
		bool SendShopSellPacket(WORD wSlot);
#ifdef INCREASE_ITEM_STACK
		bool SendShopSellPacketNew(WORD wSlot, WORD byCount);
#else
		bool SendShopSellPacketNew(WORD wSlot, BYTE byCount);
#endif

		// Exchange
		bool SendExchangeStartPacket(DWORD vid);
		bool SendExchangeItemAddPacket(::TItemPos ItemPos, BYTE byDisplayPos);
		bool SendExchangeElkAddPacket(long long elk);
		bool SendExchangeItemDelPacket(BYTE pos);
		bool SendExchangeAcceptPacket();
		bool SendExchangeExitPacket();

		// Quest
		bool SendScriptAnswerPacket(int iAnswer);
		bool SendScriptButtonPacket(unsigned int iIndex);
		bool SendAnswerMakeGuildPacket(const char * c_szName);
		bool SendQuestInputStringPacket(const char * c_szString);
		bool SendQuestConfirmPacket(BYTE byAnswer, DWORD dwPID);

		// Event
		bool SendOnClickPacket(DWORD vid);
		bool SendOnClickPacketNew(DWORD errID);
		enum EOnClickErrorID {
			ON_CLICK_ERROR_ID_SELECT_ITEM = 0,
			ON_CLICK_ERROR_ID_GET_ITEM_SIZE = 1,
			ON_CLICK_ERROR_ID_EMOTICON_CHECK = 2,
			ON_CLICK_ERROR_ID_M2BOB_INIT = 3,
			ON_CLICK_ERROR_ID_HIDDEN_ATTACK = 4,
			ON_CLICK_ERROR_ID_NET_MODULE = 5,
			ON_CLICK_ERROR_ID_PIXEL_POSITION = 6,
			ON_CLICK_ERROR_ID_ITEM_MODULE = 7,
		};
		bool SendHitSpacebarPacket();

		bool SendQuestTriggerPacket(DWORD dwIndex, DWORD dwArg);

		// Fly
		bool SendFlyTargetingPacket(DWORD dwTargetVID, const TPixelPosition& kPPosTarget);
		bool SendAddFlyTargetingPacket(DWORD dwTargetVID, const TPixelPosition& kPPosTarget);
		bool SendShootPacket(UINT uSkill);

		// Command
		bool ClientCommand(const char * c_szCommand);
		void ServerCommand(char * c_szCommand);

		// Emoticon
		void RegisterEmoticonString(const char * pcEmoticonString);

		// Party
		bool SendPartyInvitePacket(DWORD dwVID);
		bool SendPartyInviteAnswerPacket(DWORD dwLeaderVID, BYTE byAccept);
		bool SendPartyRemovePacket(DWORD dwPID);
		bool SendPartySetStatePacket(DWORD dwVID, BYTE byState, BYTE byFlag);
		bool SendPartyUseSkillPacket(BYTE bySkillIndex, DWORD dwVID);
		bool SendPartyParameterPacket(BYTE byDistributeMode);

		// SafeBox
		bool SendSafeBoxMoneyPacket(BYTE byState, DWORD dwMoney);
		bool SendSafeBoxCheckinPacket(::TItemPos InventoryPos, BYTE bySafeBoxPos);
		bool SendSafeBoxCheckoutPacket(BYTE bySafeBoxPos, ::TItemPos InventoryPos);
#ifdef INCREASE_ITEM_STACK
		bool SendSafeBoxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, WORD byCount);
#else
		bool SendSafeBoxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, BYTE byCount);
#endif

		// Mall
		bool SendMallCheckoutPacket(BYTE byMallPos, ::TItemPos InventoryPos);

		// Guild
		bool SendGuildAddMemberPacket(DWORD dwVID);
		bool SendGuildRemoveMemberPacket(DWORD dwPID);
		bool SendGuildChangeGradeNamePacket(BYTE byGradeNumber, const char * c_szName);
		bool SendGuildChangeGradeAuthorityPacket(BYTE byGradeNumber, WORD wAuthority);
		bool SendGuildOfferPacket(DWORD dwExperience);
		bool SendGuildPostCommentPacket(const char * c_szMessage);
		bool SendGuildDeleteCommentPacket(DWORD dwIndex);
		bool SendGuildRefreshCommentsPacket(DWORD dwHighestIndex);
		bool SendGuildChangeMemberGradePacket(DWORD dwPID, BYTE byGrade);
		bool SendGuildUseSkillPacket(DWORD dwSkillID, DWORD dwTargetVID);
		bool SendGuildChangeMemberGeneralPacket(DWORD dwPID, BYTE byFlag);
		bool SendGuildInvitePacket(DWORD dwVID);
		bool SendGuildInviteAnswerPacket(DWORD dwGuildID, BYTE byAnswer);
		bool SendGuildChargeGSPPacket(DWORD dwMoney);
		bool SendGuildDepositMoneyPacket(DWORD dwMoney);
		bool SendGuildWithdrawMoneyPacket(DWORD dwMoney);
		bool SendRequestGuildList(DWORD pageNumber, BYTE pageType, BYTE empire);
		bool SendRequestSearchGuild(const char* guildName, BYTE pageType, BYTE empire);

		// Mall
		bool RecvMallOpenPacket(std::unique_ptr<network::GCMallOpenPacket> pack);

		// Lover
		bool RecvLoverInfoPacket(std::unique_ptr<network::GCLoverInfoPacket> pack);
		bool RecvLovePointUpdatePacket(std::unique_ptr<network::GCLoverPointUpdatePacket> pack);

		// Dig
		bool RecvDigMotionPacket(std::unique_ptr<network::GCDigMotionPacket> pack);

		// Fishing
		bool SendFishingPacket(int iRotation);
		bool SendGiveItemPacket(DWORD dwTargetVID, ::TItemPos ItemPos, int iItemCount);

		// Private Shop
		bool SendBuildPrivateShopPacket(const char * c_szName, const std::vector<network::TShopItemTable> & c_rSellingItemStock);

		// Refine
		bool SendRefinePacket(BYTE byPos, BYTE byType, bool bFastRefine);
		bool SendSelectItemPacket(DWORD dwItemPos);

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		//Acce
		bool SendAcceRefineCheckinPacket(::TItemPos InventoryPos, BYTE byAccePos);
		bool SendAcceRefineCheckoutPacket(BYTE byAccePos);
		bool SendAcceRefineAcceptPacket(BYTE windowType);
		bool SendAcceRefineCancelPacket();
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
		bool SendFishBoxUse(BYTE bWindow, WORD wCell);
		bool SendFishShapeAdd(BYTE bPos);
#endif

		// Item Destroy
		bool SendItemDestroyPacket(::TItemPos pos, DWORD count);

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
	protected:
		bool RecvAttributesToClient(std::unique_ptr<network::GCAttributesToClientPacket> pack);
#endif

#ifdef ENABLE_EQUIPMENT_CHANGER
	public:
		bool SendEquipmentPageAddPacket(const char* c_pszPageName);
		bool SendEquipmentPageDeletePacket(DWORD dwIndex);
		bool SendEquipmentPageSelectPacket(DWORD dwIndex);
	protected:
		bool RecvEquipmentPageLoadPacket(std::unique_ptr<network::GCEquipmentPageLoadPacket> pack);
#endif
#ifdef DMG_METER
		bool RecvDmgMeterPacket(std::unique_ptr<network::GCDmgMeterPacket> pack);
#endif

	protected:
		bool RecvHorseRefineInfo(std::unique_ptr<network::GCHorseRefineInfoPacket> pack);
		bool RecvHorseRefineResult(std::unique_ptr<network::GCHorseRefineResultPacket> pack);

	//public:
	//	bool SendKingMountMeltPacket(WORD wHorseCell, WORD wMountCell, WORD wStoneCell);

	protected:
		bool RecvSkillMotion(std::unique_ptr<network::GCSkillMotionPacket> pack);

#ifdef ENABLE_DRAGONSOUL
	protected:
		bool RecvDragonSoulRefine(std::unique_ptr<network::GCDragonSoulRefinePacket> pack);
	public:
		bool SendDragonSoulRefinePacket(BYTE bRefineType, ::TItemPos* pos);
#endif

#ifdef ENABLE_GAYA_SYSTEM
	protected:
		bool RecvGayaShopOpen(std::unique_ptr<network::GCGayaShopOpenPacket> pack);
#endif

		// Event System
#ifdef ENABLE_EVENT_SYSTEM
	public:
		bool SendEventRequestAnswerPacket(DWORD dwIndex, bool bAccept);
	protected:
		bool RecvEventRequestPacket(std::unique_ptr<network::GCEventRequestPacket> pack);
		bool RecvEventCancelPacket(std::unique_ptr<network::GCEventCancelPacket> pack);
		bool RecvEventEmpireWarLoadPacket(std::unique_ptr<network::GCEventEmpireWarLoadPacket> pack);
		bool RecvEventEmpireWarUpdatePacket(std::unique_ptr<network::GCEventEmpireWarUpdatePacket> pack);
		bool RecvEventEmpireWarFinishPacket();
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
	public:
		bool SendCostumeBonusTransferCheckIn(::TItemPos InventoryCell, BYTE byCBTPos);
		bool SendCostumeBonusTransferCheckOut(BYTE byCBTPos);
		bool SendCostumeBonusTransferAccept();
		bool SendCostumeBonusTransferCancel();
	protected:
		bool RecvCostumeBonusTransferPacket(const network::InputPacket& packet);
#endif

#ifdef ENABLE_RUNE_SYSTEM
	protected:
		bool	RecvRune(std::unique_ptr<network::GCRunePacket> pack);
		bool	RecvRunePage(std::unique_ptr<network::GCRunePagePacket> pack);
		bool	RecvRuneRefine(std::unique_ptr<network::GCRuneRefinePacket> pack);
		bool	RecvRuneResetOwned();
#ifdef ENABLE_LEVEL2_RUNES
		bool	RecvRuneLevelUp(std::unique_ptr<network::GCRuneLevelupPacket> pack);
#endif
#endif

	public:
		bool SendTargetMonsterDropInfo(DWORD dwRaceNum);
	protected:
		bool RecvTargetMonsterDropInfo(std::unique_ptr<network::GCTargetMonsterInfoPacket> pack);

		// Client Version
	public:
		bool SendClientVersionPacket();

		// Handshake
		bool RecvHandshakePacket(std::unique_ptr<network::GCHandshakePacket> pack);
		bool RecvHandshakeOKPacket();

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		bool RecvKeyAgreementPacket(std::unique_ptr<network::GCKeyAgreementPacket> pack);
		bool RecvKeyAgreementCompletedPacket();
#endif

		// ETC
		DWORD GetMainActorVID();
		DWORD GetMainActorRace();
		DWORD GetMainActorEmpire();
		DWORD GetMainActorSkillGroup();
		void SetEmpireID(DWORD dwEmpireID);
		DWORD GetEmpireID();
		void __TEST_SetSkillGroupFake(int iIndex);

	//////////////////////////////////////////////////////////////////////////
	// Phase 관련
	//////////////////////////////////////////////////////////////////////////
	public:
		void SetOffLinePhase();
		void SetHandShakePhase();
		void SetLoginPhase();
		void SetSelectPhase();
		void SetLoadingPhase();
		void SetGamePhase();
		void ClosePhase();

		const std::string& GetPhaseName() const { return m_strPhase; }

		// Login Phase
		bool SendLoginPacketNew(const char * c_szName, const char * c_szPassword);
		
		bool SendEnterGame();

		// Select Phase
		bool SendSelectEmpirePacket(DWORD dwEmpireID);
		bool SendSelectCharacterPacket(BYTE account_Index);
		bool SendChangeNamePacket(BYTE index, const char *name);
		bool SendCreateCharacterPacket(BYTE index, const char *name, BYTE job, BYTE shape, BYTE byStat1, BYTE byStat2, BYTE byStat3, BYTE byStat4);
		bool SendDestroyCharacterPacket(BYTE index, const char * szPrivateCode);
#ifdef ENABLE_HAIR_SELECTOR
		bool SendSelectHairPacket(BYTE index, DWORD dwHairVnum);
#endif

#ifdef COMBAT_ZONE
		bool SendCombatZoneRequestActionPacket(int action, int value);
#endif

		// Main Game Phase
		bool SendC2CPacket(DWORD dwSize, void * pData);
		bool SendChatPacket(const char * c_szChat, BYTE byType = CHAT_TYPE_TALKING);
		bool SendWhisperPacket(const char * name, const char * c_szChat, bool bSendOffline);
		bool SendMessengerAddByVIDPacket(DWORD vid);
		bool SendMessengerAddByNamePacket(const char * c_szName);
		bool SendMessengerRemovePacket(const char * c_szKey, const char * c_szName);
		bool SendRequestOnlineInformation(const char* c_pszPlayerName);
		bool RecvPlayerOnlineInformation(std::unique_ptr<network::GCPlayerOnlineInformationPacket> pack);
		#ifdef ENABLE_MESSENGER_BLOCK
		bool SendMessengerAddBlockByVIDPacket(DWORD vid);
		bool SendMessengerAddBlockByNamePacket(const char * c_szName);
		bool SendMessengerRemoveBlockPacket(const char * c_szKey, const char * c_szName);
		#endif

	protected:
		bool OnProcess();	// State들을 실제로 실행한다.
		void OffLinePhase();
		void HandShakePhase();
		void LoginPhase();
		void SelectPhase();
		void LoadingPhase();
		void GamePhase();
		void GamePhasePacket(network::InputPacket& packet, bool& ret);

		bool __IsNotPing();

		void __DownloadMark();
		void __DownloadSymbol(const std::vector<DWORD> & c_rkVec_dwGuildID);

		void __PlayInventoryItemUseSound(::TItemPos uSlotPos);
		void __PlayInventoryItemDropSound(::TItemPos uSlotPos);
		//void __PlayShopItemDropSound(UINT uSlotPos);
		void __PlaySafeBoxItemDropSound(UINT uSlotPos);
		void __PlayMallItemDropSound(UINT uSlotPos);
#ifdef ENABLE_GUILD_SAFEBOX
		void __PlayGuildSafeboxItemDropSound(UINT uSlotPos);
#endif

		short __GetLevel();

		enum REFRESH_WINDOW_TYPE
		{
			RefreshStatus = (1 << 0),
			RefreshAlignmentWindow = (1 << 1),
			RefreshCharacterWindow = (1 << 2),
			RefreshEquipmentWindow = (1 << 3), 
			RefreshInventoryWindow = (1 << 4),
			RefreshExchangeWindow = (1 << 5),
			RefreshSkillWindow = (1 << 6),
			RefreshSafeboxWindow  = (1 << 7),
			RefreshMessengerWindow = (1 << 8),
			RefreshGuildWindowInfoPage = (1 << 9),
			RefreshGuildWindowBoardPage = (1 << 10),
			RefreshGuildWindowMemberPage = (1 << 11), 
			RefreshGuildWindowMemberPageGradeComboBox = (1 << 12),
			RefreshGuildWindowSkillPage = (1 << 13),
			RefreshGuildWindowGradePage = (1 << 14),
			RefreshTargetBoard = (1 << 15),
			RefreshMallWindow = (1 << 16),
#ifdef ENABLE_GUILD_SAFEBOX
			RefreshGuildSafeboxWindow = (1 << 17),
#endif
		};

		void __RefreshStatus();
		void __RefreshAlignmentWindow();
		void __RefreshCharacterWindow();
		void __RefreshEquipmentWindow();
		void __RefreshInventoryWindow();
		void __RefreshExchangeWindow();
		void __RefreshSkillWindow();
		void __RefreshSafeboxWindow();
		void __RefreshMessengerWindow();
		void __RefreshGuildWindowInfoPage();
		void __RefreshGuildWindowBoardPage();
		void __RefreshGuildWindowMemberPage();
		void __RefreshGuildWindowMemberPageGradeComboBox();
		void __RefreshGuildWindowMemberPageLastPlayed();
		void __RefreshGuildWindowSkillPage();
		void __RefreshGuildWindowGradePage();
		void __RefreshTargetBoardByVID(DWORD dwVID);
		void __RefreshTargetBoardByName(const char * c_szName);
		void __RefreshTargetBoard();
		void __RefreshMallWindow();
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		void __RefreshAcceWindow();
#endif
#ifdef ENABLE_GUILD_SAFEBOX
		void __RefreshGuildSafeboxWindow();
#endif
#ifdef ENABLE_ANIMAL_SYSTEM
		void __RefreshAnimalWindow(BYTE bType);
#endif

		void __RefreshGuildRankingWindow(bool isSearch);
#ifdef ENABLE_PET_ADVANCED
		void __RefreshPetStatusWindow();
		void __RefreshPetSkillWindow();
#endif

		bool __SendHack(const char* c_szMsg);
	public:
#ifdef ENABLE_ZODIAC
		bool __CanActMainInstance(bool skipIsDeadItem = false);
#else
		bool __CanActMainInstance();
#endif
		bool __IsPlayerAttacking();

	protected:
		bool RecvObserverAddPacket(std::unique_ptr<network::GCObserverAddPacket> pack);
		bool RecvObserverRemovePacket(std::unique_ptr<network::GCObserverRemovePacket> pack);
		bool RecvObserverMovePacket(std::unique_ptr<network::GCObserverMovePacket> pack);

		// Common
		bool RecvErrorPacket(network::InputPacket& pack);
		bool RecvPingPacket();
		bool RecvPhasePacket(std::unique_ptr<network::GCPhasePacket> pack);

		// Login Phase
		bool __RecvLoginSuccessPacket(std::unique_ptr<network::GCLoginSuccessPacket> pack);
		bool __RecvLoginFailurePacket(std::unique_ptr<network::GCLoginFailurePacket> pack);
		bool __RecvEmpirePacket(std::unique_ptr<network::GCEmpirePacket> pack);

		// Select Phase
		bool __RecvPlayerCreateSuccessPacket(std::unique_ptr<network::GCPlayerCreateSuccessPacket> pack);
		bool __RecvPlayerCreateFailurePacket(std::unique_ptr<network::GCCreateFailurePacket> pack);
		bool __RecvPlayerDestroySuccessPacket(std::unique_ptr<network::GCDeleteSuccessPacket> pack);
		bool __RecvPlayerDestroyFailurePacket();
		bool __RecvPlayerPoints(std::unique_ptr<network::GCPointsPacket> pack);
		bool __RecvChangeName(std::unique_ptr<network::GCChangeNamePacket> pack);

#ifdef COMBAT_ZONE
		bool __RecvCombatZoneRankingPacket(std::unique_ptr<network::GCCombatZoneRankingDataPacket> pack);
		bool __RecvCombatZonePacket(std::unique_ptr<network::GCSendCombatZonePacket> pack);
#endif

		// Loading Phase
		bool RecvMainCharacter(std::unique_ptr<network::GCMainCharacterPacket> pack);

		void __SetFieldMusicFileName(const char* musicName);
		void __SetFieldMusicFileInfo(const char* musicName, float vol);
		// END_OF_SUPPORT_BGM

		// Main Game Phase
		bool RecvWarpPacket(std::unique_ptr<network::GCWarpPacket> pack);
		bool RecvPVPPacket(std::unique_ptr<network::GCPVPPacket> pack);
		bool RecvDuelStartPacket(std::unique_ptr<network::GCDuelStartPacket> pack);
		bool RecvCharacterAppendPacket(std::unique_ptr<network::GCCharacterAddPacket> pack);
		bool RecvCharacterAdditionalInfo(std::unique_ptr<network::GCCharacterAdditionalInfoPacket> pack);
		bool RecvCharacterUpdatePacket(std::unique_ptr<network::GCCharacterUpdatePacket> pack);
		bool RecvCharacterDeletePacket(std::unique_ptr<network::GCCharacterDeletePacket> pack);
		bool RecvChatPacket(std::unique_ptr<network::GCChatPacket> pack);
		bool RecvOwnerShipPacket(std::unique_ptr<network::GCOwnershipPacket> pack);
		bool RecvSyncPositionPacket(std::unique_ptr<network::GCSyncPositionPacket> pack);
		bool RecvWhisperPacket(std::unique_ptr<network::GCWhisperPacket> pack);
		bool RecvPointChange(std::unique_ptr<network::GCPointChangePacket> pack);					// Alarm to python
		bool RecvRealPointSet(std::unique_ptr<network::GCRealPointSetPacket> pack);

		bool RecvStunPacket(std::unique_ptr<network::GCStunPacket> pack);
		bool RecvDeadPacket(std::unique_ptr<network::GCDeadPacket> pack);
		bool RecvCharacterMovePacket(std::unique_ptr<network::GCMovePacket> pack);

		bool RecvItemSetPacket(std::unique_ptr<network::GCItemSetPacket> pack);
		bool RecvItemUpdatePacket(std::unique_ptr<network::GCItemUpdatePacket> pack);

		// Alarm to python
		bool RecvItemGroundAddPacket(std::unique_ptr<network::GCItemGroundAddPacket> pack);
		bool RecvItemGroundDelPacket(std::unique_ptr<network::GCItemGroundDeletePacket> pack);
		bool RecvItemOwnership(std::unique_ptr<network::GCItemOwnershipPacket> pack);

		bool RecvQuickSlotAddPacket(std::unique_ptr<network::GCQuickslotAddPacket> pack);				// Alarm to python
		bool RecvQuickSlotDelPacket(std::unique_ptr<network::GCQuickslotDelPacket> pack);				// Alarm to python
		bool RecvQuickSlotMovePacket(std::unique_ptr<network::GCQuickslotSwapPacket> pack);				// Alarm to python

		bool RecvCharacterPositionPacket(std::unique_ptr<network::GCPositionPacket> pack);
		bool RecvMotionPacket(std::unique_ptr<network::GCMotionPacket> pack);

		bool RecvShopPacket(const network::InputPacket& packet);
		bool RecvShopSignPacket(std::unique_ptr<network::GCShopSignPacket> pack);
		bool RecvExchangePacket(const network::InputPacket& packet);

		// Quest
		bool RecvScriptPacket(std::unique_ptr<network::GCScriptPacket> pack);
		bool RecvQuestInfoPacket(std::unique_ptr<network::GCQuestInfoPacket> pack);
		bool RecvQuestConfirmPacket(std::unique_ptr<network::GCQuestConfirmPacket> pack);
		//bool RecvQuestDeletePacket();
		bool RecvRequestMakeGuild();

		// Skill
		bool RecvSkillLevel(std::unique_ptr<network::GCSkillLevelPacket> pack);

		// Target
		bool RecvTargetPacket(std::unique_ptr<network::GCTargetPacket> pack);
		bool RecvViewEquipPacket(std::unique_ptr<network::GCViewEquipPacket> pack);
		bool RecvDamageInfoPacket(std::unique_ptr<network::GCDamageInfoPacket> pack);

		// Fly
		bool RecvCreateFlyPacket(std::unique_ptr<network::GCCreateFlyPacket> pack);
		bool RecvFlyTargetingPacket(std::unique_ptr<network::GCFlyTargetingPacket> pack);
		bool RecvAddFlyTargetingPacket(std::unique_ptr<network::GCAddFlyTargetingPacket> pack);

		// Messenger
		bool RecvMessenger(const network::InputPacket& packet);

		// Guild
		bool RecvGuild(const network::InputPacket& packet);

		// Party
		bool RecvPartyInvite(std::unique_ptr<network::GCPartyInvitePacket> pack);
		bool RecvPartyAdd(std::unique_ptr<network::GCPartyAddPacket> pack);
		bool RecvPartyUpdate(std::unique_ptr<network::GCPartyUpdatePacket> pack);
		bool RecvPartyRemove(std::unique_ptr<network::GCPartyRemovePacket> pack);
		bool RecvPartyLink(std::unique_ptr<network::GCPartyLinkPacket> pack);
		bool RecvPartyUnlink(std::unique_ptr<network::GCPartyUnlinkPacket> pack);
		bool RecvPartyParameter(std::unique_ptr<network::GCPartyParameterPacket> pack);

		// SafeBox
		bool RecvSafeBoxWrongPasswordPacket();
		bool RecvSafeBoxSizePacket(std::unique_ptr<network::GCSafeboxSizePacket> pack);

		// Fishing
		bool RecvFishing(const network::InputPacket& packet);

		// Dungeon
		bool RecvDungeon(std::unique_ptr<network::GCDungeonDestinationPositionPacket> pack);

		// Time
		bool RecvTimePacket(std::unique_ptr<network::GCTimePacket> pack);

		// WalkMode
		bool RecvWalkModePacket(std::unique_ptr<network::GCWalkModePacket> pack);

		// ChangeSkillGroup
		bool RecvChangeSkillGroupPacket(std::unique_ptr<network::GCChangeSkillGroupPacket> pack);

		// Refine
		bool RecvRefineInformationPacket(std::unique_ptr<network::GCRefineInformationPacket> pack);

		// Use Potion
		bool RecvSpecialEffect(std::unique_ptr<network::GCSpecialEffectPacket> pack);

		// 서버에서 지정한 이팩트 발동 패킷.
		bool RecvSpecificEffect(std::unique_ptr<network::GCSpecificEffectPacket> pack);

		// PVP Team
		bool RecvPVPTeam(std::unique_ptr<network::GCPVPTeamPacket> pack);

		// Teamler
		bool RecvShowTeamler(std::unique_ptr<network::GCTeamlerShowPacket> pack);
		bool RecvTeamlerStatus(std::unique_ptr<network::GCTeamlerStatusPacket> pack);
		
		// MiniMap Info
		bool RecvNPCList(std::unique_ptr<network::GCNPCListPacket> pack);
		bool RecvLandPacket(std::unique_ptr<network::GCLandListPacket> pack);
		bool RecvTargetCreatePacket(std::unique_ptr<network::GCTargetCreatePacket> pack);
		bool RecvTargetUpdatePacket(std::unique_ptr<network::GCTargetUpdatePacket> pack);
		bool RecvTargetDeletePacket(std::unique_ptr<network::GCTargetDeletePacket> pack);

		// Affect
		bool RecvAffectAddPacket(std::unique_ptr<network::GCAffectAddPacket> pack);
		bool RecvAffectRemovePacket(std::unique_ptr<network::GCAffectRemovePacket> pack);

		bool RecvShiningPacket(std::unique_ptr<network::GCCharacterShiningPacket> pack);
		bool RecvSoulRefineInfo(std::unique_ptr<network::GCSoulRefineInfoPacket> pack);
#ifdef INGAME_WIKI
		bool RecvWikiPacket(std::unique_ptr<network::GCWikiPacket> pack);
		bool RecvWikiMobPacket(std::unique_ptr<network::GCWikiMobPacket> pack);
#endif

#ifdef ENABLE_PET_ADVANCED
	protected:
		bool RecvPetAdvanced(network::InputPacket& packet);

	public:
		bool SendPetUseEggPacket(::TItemPos egg_position, const std::string& name);
		bool SendPetAttrRefineInfoPacket(BYTE index);
		bool SendPetEvolutionInfoPacket();
		bool SendPetEvolvePacket();
		bool SendPetResetSkillPacket(::TItemPos reset_item_position, BYTE skill_index);
#endif

	protected:
		bool RecvUpdateCharacterScale(std::unique_ptr<network::GCUpdateCharacterScalePacket> pack);

#ifdef ENABLE_MAINTENANCE
		// Maintenance
	protected:
		bool RecvMaintenancePacket(std::unique_ptr<network::GCMaintenanceInfoPacket> pack);
#endif

#ifdef ENABLE_GUILD_SAFEBOX
	public:
		bool SendGuildSafeboxOpenPacket();
		bool SendGuildSafeboxCheckinPacket(::TItemPos InventoryPos, BYTE byGuildSafeboxPos);
		bool SendGuildSafeboxCheckoutPacket(BYTE byGuildSafeboxPos, ::TItemPos InventoryPos);
#ifdef INCREASE_ITEM_STACK
		bool SendGuildSafeboxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, WORD byCount);
#else
		bool SendGuildSafeboxItemMovePacket(BYTE bySourcePos, BYTE byTargetPos, BYTE byCount);
#endif
		bool SendGuildSafeboxGiveGoldPacket(ULONGLONG ullGold);
		bool SendGuildSafeboxTakeGoldPacket(ULONGLONG ullGold);
	protected:
		bool RecvGuildSafeboxPacket(const network::InputPacket& packet);
#endif

	public:
		bool RecvInventoryMaxNum(std::unique_ptr<network::GCInventoryMaxNumPacket> pack);

#ifdef ENABLE_FAKEBUFF
	protected:
		bool RecvFakeBuffSkill(std::unique_ptr<network::GCFakeBuffSkillPacket> pack);
#endif

#ifdef ENABLE_ATTRTREE
	protected:
		bool RecvAttrtreeLevel(std::unique_ptr<network::GCAttrtreeLevelPacket> pack);
		bool RecvAttrtreeRefine(std::unique_ptr<network::GCAttrtreeRefinePacket> pack);
#endif

		// Auction
#ifdef ENABLE_AUCTION
	public:
		bool SendAuctionInsertItemPacket(::TItemPos cell, ::TItemPos target_cell, uint64_t price);
		bool SendAuctionTakeItemPacket(DWORD item_id, WORD target_inven_pos);
		bool SendAuctionBuyItemPacket(DWORD item_id, uint64_t price);
		bool SendAuctionTakeGoldPacket(uint64_t gold);
		bool SendAuctionSearchItemsPacket(WORD page, const network::TDataAuctionSearch& options);
		bool SendAuctionExtendedSearchItemsPacket(WORD page, const network::TExtendedDataAuctionSearch& options);
		bool SendAuctionMarkShopPacket(DWORD item_id);
		bool SendAuctionRequestShopView();
		bool SendAuctionOpenShopPacket(const std::string& name, float red, float green, float blue, std::vector<network::TShopItemTable>&& items, BYTE model, BYTE style);
		bool SendAuctionTakeShopGoldPacket(uint64_t gold);
		bool SendAuctionRenewShopPacket();
		bool SendAuctionCloseShopPacket(bool has_items);
		bool SendAuctionShopGuestCancelPacket();
		bool SendAuctionRequestShopHistoryPacket();
		bool SendRequestAveragePricePacket(BYTE requestor, DWORD vnum, DWORD count);

	protected:
		bool RecvAuctionOwnedGold(std::unique_ptr<network::GCAuctionOwnedGoldPacket> pack);
		bool RecvAuctionOwnedItem(std::unique_ptr<network::GCAuctionOwnedItemPacket> pack);
		bool RecvAuctionSearchResult(std::unique_ptr<network::GCAuctionSearchResultPacket> pack);
		bool RecvAuctionMessage(std::unique_ptr<network::GCAuctionMessagePacket> pack);
		bool RecvAuctionShopOwned(std::unique_ptr<network::GCAuctionShopOwnedPacket> pack);
		bool RecvAuctionShop(std::unique_ptr<network::GCAuctionShopPacket> pack);
		bool RecvAuctionShopGold(std::unique_ptr<network::GCAuctionShopGoldPacket> pack);
		bool RecvAuctionShopTimeout(std::unique_ptr<network::GCAuctionShopTimeoutPacket> pack);
		bool RecvAuctionShopGuestOpen(std::unique_ptr<network::GCAuctionShopGuestOpenPacket> pack);
		bool RecvAuctionShopGuestUpdate(std::unique_ptr<network::GCAuctionShopGuestUpdatePacket> pack);
		bool RecvAuctionShopGuestClose();
		bool RecvAuctionShopHistory(std::unique_ptr<network::GCAuctionShopHistoryPacket> pack);
		bool RecvAuctionAveragePrice(std::unique_ptr<network::GCAuctionAveragePricePacket> pack);
#endif

#ifdef ENABLE_ANIMAL_SYSTEM
	protected:
		bool RecvAnimalSummon();
		bool RecvAnimalUpdateLevel();
		bool RecvAnimalUpdateExp();
		bool RecvAnimalUpdateStats();
		bool RecvAnimalUnsummon();
#endif

#ifdef BATTLEPASS_EXTENSION
		bool RecvBattlepassData(std::unique_ptr<network::GCBattlepassDataPacket> pack);
#endif

#ifdef CRYSTAL_SYSTEM
		bool RecvCrystalRefine(std::unique_ptr<network::GCCrystalRefinePacket> pack);
		bool RecvCrystalRefineSuccess();
		bool RecvCrystalUsingSlot(std::unique_ptr<network::GCCrystalUsingSlotPacket> pack);
#endif

#ifdef ENABLE_PYTHON_REPORT_PACKET
	public:
		bool SendHackReportPacket(const char* szTitle, const char* szDescription);
#endif

	protected:
		// 이모티콘
		bool ParseEmoticon(const char * pChatMsg, DWORD * pdwEmoticon);

		// 파이썬으로 보내는 콜들
		void OnConnectFailure();
		void OnScriptEventStart(int iSkin, int iIndex);
		
		void HideQuestWindows();

		void OnRemoteDisconnect();
		void OnDisconnect();

		void SetGameOnline();
		void SetGameOffline();
		BOOL IsGameOnline();

	protected:
		void __InitializeGamePhase();
		void __InitializeMarkAuth();
		void __GlobalPositionToLocalPosition(long& rGlobalX, long& rGlobalY);
		void __LocalPositionToGlobalPosition(long& rLocalX, long& rLocalY);

		bool __IsEquipItemInSlot(::TItemPos Cell);

		void __ShowMapName(long lLocalX, long lLocalY);

		void __LeaveOfflinePhase() {}
		void __LeaveHandshakePhase() {}
		void __LeaveLoginPhase() {}
		void __LeaveSelectPhase() {}
		void __LeaveLoadingPhase() {}
		void __LeaveGamePhase();

		void __ClearNetworkActorManager();

		void __ClearSelectCharacterData();

		// DELETEME
		//void __SendWarpPacket();

		void __ConvertEmpireText(DWORD dwEmpireID, char* szText);

		void __RecvCharacterAppendPacket(SNetworkActorData * pkNetActorData);
		void __RecvCharacterUpdatePacket(SNetworkUpdateActorData * pkNetUpdateActorData);

		void __SetGuildID(DWORD id);

	protected:
		network::GCHandshakePacket m_HandshakeData;
		DWORD m_dwChangingPhaseTime;
		DWORD m_dwBindupRetryCount;
		DWORD m_dwMainActorVID;
		DWORD m_dwMainActorRace;
		DWORD m_dwMainActorEmpire;
		DWORD m_dwMainActorSkillGroup;
		BOOL m_isGameOnline;
		BOOL m_isStartGame;

		DWORD m_dwGuildID;
		DWORD m_dwEmpireID;
		
		struct SServerTimeSync
		{
			DWORD m_dwChangeServerTime;
			DWORD m_dwChangeClientTime;
		} m_kServerTimeSync;

		void __ServerTimeSync_Initialize();
		//DWORD m_dwBaseServerTime;
		//DWORD m_dwBaseClientTime;

		DWORD m_dwLastGamePingTime;

		std::string	m_stID;
		std::string	m_stPassword;
		std::string	m_strLastCommand;
		std::string	m_strPhase;
		DWORD m_dwLoginKey;
		BOOL m_isWaitLoginKey;

		std::string m_stMarkIP;

		CFuncObject<CPythonNetworkStream>	m_phaseProcessFunc;
		CFuncObject<CPythonNetworkStream>	m_phaseLeaveFunc;

		PyObject*							m_poHandler;
		PyObject*							m_apoPhaseWnd[PHASE_WINDOW_NUM];
		PyObject*							m_poSerCommandParserWnd;

		network::TSimplePlayer				m_akSimplePlayerInfo[PLAYER_PER_ACCOUNT4];
		bool m_bSimplePlayerInfo;

		CRef<CNetworkActorManager>			m_rokNetActorMgr;

		bool m_isRefreshStatus;
		bool m_isRefreshCharacterWnd;
		bool m_isRefreshEquipmentWnd;
		bool m_isRefreshInventoryWnd;
		bool m_isRefreshExchangeWnd;
		bool m_isRefreshSkillWnd;
		bool m_isRefreshSafeboxWnd;
		bool m_isRefreshMallWnd;
		bool m_isRefreshMessengerWnd;
		bool m_isRefreshGuildWndInfoPage;
		bool m_isRefreshGuildWndBoardPage;
		bool m_isRefreshGuildWndMemberPage;
		bool m_isRefreshGuildWndMemberPageGradeComboBox;
		bool m_isRefreshGuildWndMemberPageLastPlayed;
		bool m_isRefreshGuildWndSkillPage;
		bool m_isRefreshGuildWndGradePage;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		bool m_isRefreshAcceWnd;
#endif
#ifdef ENABLE_GUILD_SAFEBOX
		bool m_isRefreshGuildSafeboxWnd;
#endif
#ifdef ENABLE_ANIMAL_SYSTEM
		bool m_isRefreshAnimalWnd[ANIMAL_TYPE_MAX_NUM];
#endif
#ifdef ENABLE_PET_ADVANCED
		bool m_isRefreshPetStatusWnd;
		bool m_isRefreshPetSkillWnd;
#endif

		// Emoticon
		std::vector<std::string> m_EmoticonStringVector;

		struct STextConvertTable 
		{
			char acUpper[26];
			char acLower[26];
			BYTE aacHan[5000][2];
		} m_aTextConvTable[3];



		struct SMarkAuth
		{
			CNetworkAddress m_kNetAddr;
			DWORD m_dwHandle;
			DWORD m_dwRandomKey;
		} m_kMarkAuth;



		DWORD m_dwSelectedCharacterIndex;

		bool m_bComboSkillFlag;

		std::deque<std::string> m_kQue_stHack;

	private:
		struct SDirectEnterMode
		{
			bool m_isSet;
			DWORD m_dwChrSlotIndex;
		} m_kDirectEnterMode;

		void __DirectEnterMode_Initialize();
		void __DirectEnterMode_Set(UINT uChrSlotIndex);
		bool __DirectEnterMode_IsSet();

	public:
		DWORD EXPORT_GetBettingGuildWarValue(const char* c_szValueName);

	private:
		struct SBettingGuildWar
		{
			DWORD m_dwBettingMoney;
			DWORD m_dwObserverCount;
		} m_kBettingGuildWar;

		CInstanceBase * m_pInstTarget;

		void __BettingGuildWar_Initialize();
		void __BettingGuildWar_SetObserverCount(UINT uObserverCount);
		void __BettingGuildWar_SetBettingMoney(UINT uBettingMoney);

	public:
		DWORD	GetLastWarpTime() const { return m_dwLastWarpTime; }
	private:
		DWORD	m_dwLastWarpTime;
	
	public:
		void	SetCurrentMapIndex(DWORD idx) { m_dwCurrentMapIndex = idx; };
		DWORD	GetCurrentMapIndex() { return m_dwCurrentMapIndex; };
		bool	IsPrivateMap() { return m_dwCurrentMapIndex >= 10000; };
	private:
		DWORD m_dwCurrentMapIndex;
};

inline bool CPythonNetworkStream::SendOnClickPacketNew(DWORD errID)
{
	network::CGOutputPacket<network::CGOnClickPacket> OnClickPacket;
	OnClickPacket->set_vid(UINT_MAX - errID);
	
	if (!Send(OnClickPacket))
	{
		return false;
	}

	return true;
}
