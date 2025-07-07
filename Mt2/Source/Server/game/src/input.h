#ifndef __INC_METIN_II_GAME_INPUT_PROCESSOR__
#define __INC_METIN_II_GAME_INPUT_PROCESSOR__

#include "headers.hpp"
#include "protobuf_cg_packets.h"
#include "protobuf_dg_packets.h"
#include "protobuf_gg_packets.h"
#include <google/protobuf/port_def.inc>

#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif

enum
{
	INPROC_CLOSE,
	INPROC_HANDSHAKE,
	INPROC_LOGIN,
	INPROC_MAIN,
	INPROC_DEAD,
	INPROC_DB,
	INPROC_UDP,
	INPROC_P2P,
	INPROC_AUTH,
	INPROC_TEEN,
};

void LoginFailure(LPDESC d, const char * c_pszStatus, int iData = 0);

namespace network
{
	struct InputPacket
	{
		TPacketHeader header;
		const uint8_t* content;
		uint32_t content_size;

		InputPacket()
		{
			content_size = 0;
		}

		uint16_t get_header() const noexcept
		{
			return header.header;
		}
		template <typename HeaderType>
		typename std::enable_if<std::is_enum<HeaderType>::value, HeaderType>::type get_header() const noexcept
		{
			return static_cast<HeaderType>(header.header);
		}

		template <typename T>
		std::unique_ptr<T> get() const
		{
			T* packet = new T();

			auto packet_base = static_cast<::PROTOBUF_NAMESPACE_ID::Message*>(packet);
			if (content_size > 0)
			{
				bool ret = packet_base->ParseFromArray(content, content_size);
				if (!ret || packet_base->ByteSize() != content_size)
					sys_err("invalid packet with header %u content_size %u real_size %u ret %d",
						get_header(), content_size, packet_base->ByteSize(), ret);
			}

			return std::unique_ptr<T>(packet);
		}
	};
}

class CInputProcessor
{
	public:
		CInputProcessor();
		virtual ~CInputProcessor() {};
		
		virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
		virtual BYTE GetType() = 0;

		void Pong(LPDESC d);
		void Handshake(LPDESC d, std::unique_ptr<network::CGHandshakePacket> pack);
		void Version(LPCHARACTER ch, std::unique_ptr<network::CGClientVersionPacket> pack);

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet) = 0;

		int	m_iBufferLeft;
};

class CInputClose : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_CLOSE; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet) { return false; }
};

class CInputHandshake : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_HANDSHAKE; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);
};

class CInputLogin : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_LOGIN; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);

	protected:
		void		LoginByKey(LPDESC d, std::unique_ptr<network::CGLoginByKeyPacket> pinfo);

		void		CharacterSelect(LPDESC d, std::unique_ptr<network::CGPlayerSelectPacket> pinfo);
#ifdef __HAIR_SELECTOR__
		void		PlayerHairSelect(LPDESC d, std::unique_ptr<network::CGPlayerHairSelectPacket> pinfo);
#endif
		void		CharacterCreate(LPDESC d, std::unique_ptr<network::CGPlayerCreatePacket> pinfo);
		void		CharacterDelete(LPDESC d, std::unique_ptr<network::CGPlayerDeletePacket> pinfo);
		void		Entergame(LPDESC d);
		void		Empire(LPDESC d, std::unique_ptr<network::CGEmpirePacket> p);
		void		GuildMarkCRCList(LPDESC d, std::unique_ptr<network::CGMarkCRCListPacket> pCG);
		// MARK_BUG_FIX
		void		GuildMarkIDXList(LPDESC d);
		// END_OF_MARK_BUG_FIX
		void		GuildMarkUpload(LPDESC d, std::unique_ptr<network::CGMarkUploadPacket> p);
		void			GuildSymbolUpload(LPDESC d, std::unique_ptr<network::CGGuildSymbolUploadPacket> p);
		void		GuildSymbolCRC(LPDESC d, std::unique_ptr<network::CGGuildSymbolCRCPacket> p);
		void		ChangeName(LPDESC d, std::unique_ptr<network::CGPlayerChangeNamePacket> p);
};

class CInputMain : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_MAIN; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);

	protected:
		void		Attack(LPCHARACTER ch, const network::InputPacket& data);
		void		Whisper(LPCHARACTER ch, std::unique_ptr<network::CGWhisperPacket> pinfo);
		void		Chat(LPCHARACTER ch, std::unique_ptr<network::CGChatPacket> pinfo);
		void		ItemUse(LPCHARACTER ch, std::unique_ptr<network::CGItemUsePacket> pack);
		void		ItemMultiUse(LPCHARACTER ch, std::unique_ptr<network::CGItemMultiUsePacket> p);
		void		ItemDrop(LPCHARACTER ch, std::unique_ptr<network::CGItemDropPacket> pack);
		void		ItemMove(LPCHARACTER ch, std::unique_ptr<network::CGItemMovePacket> pack);
		void		ItemPickup(LPCHARACTER ch, std::unique_ptr<network::CGItemPickupPacket> pack);
		void		ItemToItem(LPCHARACTER ch, std::unique_ptr<network::CGItemUseToItemPacket> p);
		void		ItemGive(LPCHARACTER ch, std::unique_ptr<network::CGGiveItemPacket> p);
		void		QuickslotAdd(LPCHARACTER ch, std::unique_ptr<network::CGQuickslotAddPacket> pack);
		void		QuickslotDelete(LPCHARACTER ch, std::unique_ptr<network::CGQuickslotDeletePacket> pack);
		void		QuickslotSwap(LPCHARACTER ch, std::unique_ptr<network::CGQuickslotSwapPacket> pack);
		void		Shop(LPCHARACTER ch, const network::InputPacket& p);
		void		OnClick(LPCHARACTER ch, std::unique_ptr<network::CGOnClickPacket> pack);
		void		OnHitSpacebar(LPCHARACTER ch);
		void		OnQuestTrigger(LPCHARACTER ch, std::unique_ptr<network::CGOnQuestTriggerPacket> p);
		void		Exchange(LPCHARACTER ch, const network::InputPacket& p);
		void		Move(LPCHARACTER ch, std::unique_ptr<network::CGMovePacket> pack);
		void		SyncPosition(LPCHARACTER ch, std::unique_ptr<network::CGSyncPositionPacket> pinfo);
		void		FlyTarget(LPCHARACTER ch, std::unique_ptr<network::CGFlyTargetPacket> p);
		void		AddFlyTarget(LPCHARACTER ch, std::unique_ptr<network::CGAddFlyTargetPacket> p);
		void		UseSkill(LPCHARACTER ch, std::unique_ptr<network::CGUseSkillPacket> p);
		
		void		ScriptAnswer(LPCHARACTER ch, std::unique_ptr<network::CGScriptAnswerPacket> p);
		void		ScriptButton(LPCHARACTER ch, std::unique_ptr<network::CGScriptButtonPacket> p);
		void		ScriptSelectItem(LPCHARACTER ch, std::unique_ptr<network::CGScriptSelectItemPacket> p);

		void		QuestInputString(LPCHARACTER ch, std::unique_ptr<network::CGQuestInputStringPacket> p);
		void		QuestConfirm(LPCHARACTER ch, std::unique_ptr<network::CGQuestConfirmPacket> p);
		void		Target(LPCHARACTER ch, std::unique_ptr<network::CGTargetPacket> p);
		void		Warp(LPCHARACTER ch);
		void		SafeboxCheckin(LPCHARACTER ch, std::unique_ptr<network::CGSafeboxCheckinPacket> p);
		void		SafeboxCheckout(LPCHARACTER ch, std::unique_ptr<network::CGSafeboxCheckoutPacket> p);
		void		SafeboxItemMove(LPCHARACTER ch, std::unique_ptr<network::CGSafeboxItemMovePacket> p);
		void		Messenger(LPCHARACTER ch, const network::InputPacket& p);

		void 		PartyInvite(LPCHARACTER ch, std::unique_ptr<network::CGPartyInvitePacket> p);
		void 		PartyInviteAnswer(LPCHARACTER ch, std::unique_ptr<network::CGPartyInviteAnswerPacket> p);
		void		PartyRemove(LPCHARACTER ch, std::unique_ptr<network::CGPartyRemovePacket> p);
		void		PartySetState(LPCHARACTER ch, std::unique_ptr<network::CGPartySetStatePacket> p);
		void		PartyUseSkill(LPCHARACTER ch, std::unique_ptr<network::CGPartyUseSkillPacket> p);
		void		PartyParameter(LPCHARACTER ch, std::unique_ptr<network::CGPartyParameterPacket> p);

		void		Guild(LPCHARACTER ch, const network::InputPacket& data);
		void		AnswerMakeGuild(LPCHARACTER ch, std::unique_ptr<network::CGGuildAnswerMakePacket> p);

		void		Fishing(LPCHARACTER ch, std::unique_ptr<network::CGFishingPacket> p);
		void		Hack(LPCHARACTER ch, std::unique_ptr<network::CGHackPacket> p);
		void			MyShop(LPCHARACTER ch, std::unique_ptr<network::CGMyShopPacket> p);

		void		Refine(LPCHARACTER ch, std::unique_ptr<network::CGRefinePacket> p);

		void		Roulette(LPCHARACTER ch, const char* c_pData);

#ifdef __ACCE_COSTUME__
		void			AcceRefine(LPCHARACTER ch, const network::InputPacket& data);
#endif

#ifdef __GUILD_SAFEBOX__
		void		GuildSafebox(LPCHARACTER ch, const network::InputPacket& data);
#endif

		void		ItemDestroy(LPCHARACTER ch, std::unique_ptr<network::CGItemDestroyPacket> pack);
		void		TargetMonsterDropInfo(LPCHARACTER ch, std::unique_ptr<network::CGTargetMonsterDropInfoPacket> p);
		void		PlayerLanguageInformation(LPCHARACTER ch, std::unique_ptr<network::CGPlayerLanguageInformationPacket> p);

#ifdef __PYTHON_REPORT_PACKET__
		void		BotReportLog(LPCHARACTER ch, std::unique_ptr<network::CGBotReportLogPacket> p);
#endif

		void		ForcedRewarp(LPCHARACTER ch, std::unique_ptr<network::CGForcedRewarpPacket> p);
		void		UseDetachmentSingle(LPCHARACTER ch, std::unique_ptr<network::CGUseDetachmentSinglePacket> p);

#ifdef __EVENT_MANAGER__
		void		EventRequestAnswer(LPCHARACTER ch, std::unique_ptr<network::CGEventRequestAnswerPacket> p);
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
		void			CostumeBonusTransfer(LPCHARACTER ch, std::unique_ptr<network::CGCostumeBonusTransferPacket> p);
#endif

#ifdef ENABLE_RUNE_SYSTEM
		void		RunePage(LPCHARACTER ch, std::unique_ptr<network::CGRunePagePacket> p);
#endif
#ifdef CHANGE_SKILL_COLOR
		void		SetSkillColor(LPCHARACTER ch, std::unique_ptr<network::CGSetSkillColorPacket> p);
#endif
#ifdef INGAME_WIKI
		void		RecvWikiPacket(LPCHARACTER ch, std::unique_ptr<network::CGRecvWikiPacket> p);
#endif
#ifdef __EQUIPMENT_CHANGER__
		void		EquipmentPageAdd(LPCHARACTER ch, std::unique_ptr<network::CGEquipmentPageAddPacket> p);
		void		EquipmentPageDelete(LPCHARACTER ch, std::unique_ptr<network::CGEquipmentPageDeletePacket> p);
		void		EquipmentPageSelect(LPCHARACTER ch, std::unique_ptr<network::CGEquipmentPageSelectPacket> p);
		void		EquipmentPageSetName(LPCHARACTER ch, const char * c_pData);
#endif

#ifdef __PET_ADVANCED__
		void		PetAdvanced(LPCHARACTER ch, network::TCGHeader header, const network::InputPacket& data);
#endif

#ifdef AUCTION_SYSTEM
		void		AuctionInsertItem(LPCHARACTER ch, std::unique_ptr<network::CGAuctionInsertItemPacket> p);
		void		AuctionTakeItem(LPCHARACTER ch, std::unique_ptr<network::CGAuctionTakeItemPacket> p);
		void		AuctionBuyItem(LPCHARACTER ch, std::unique_ptr<network::CGAuctionBuyItemPacket> p);
		void		AuctionTakeGold(LPCHARACTER ch, std::unique_ptr<network::CGAuctionTakeGoldPacket> p);
		void		AuctionSearchItems(LPCHARACTER ch, std::unique_ptr<network::CGAuctionSearchItemsPacket> p);
		void		AuctionExtendedSearchItems(LPCHARACTER ch, std::unique_ptr<network::CGAuctionExtendedSearchItemsPacket> p);
		void		AuctionMarkShop(LPCHARACTER ch, std::unique_ptr<network::CGAuctionMarkShopPacket> p);

		void		AuctionShopRequestShow(LPCHARACTER ch);
		void		AuctionShopOpen(LPCHARACTER ch, std::unique_ptr<network::CGAuctionShopOpenPacket> p);
		void		AuctionShopTakeGold(LPCHARACTER ch, std::unique_ptr<network::CGAuctionShopTakeGoldPacket> p);
		void		AuctionShopGuestCancel(LPCHARACTER ch);
		void		AuctionShopRenew(LPCHARACTER ch);
		void		AuctionShopClose(LPCHARACTER ch, std::unique_ptr<network::CGAuctionShopClosePacket> p);
		void		AuctionShopRequestHistory(LPCHARACTER ch);
		void		AuctionRequestAveragePrice(LPCHARACTER ch, std::unique_ptr<network::CGAuctionRequestAveragePricePacket> p);
#endif

#ifdef CRYSTAL_SYSTEM
		void		CrystalRefine(LPCHARACTER ch, std::unique_ptr<network::CGCrystalRefinePacket> p);
#endif
};

class CInputDead : public CInputMain
{
	public:
		virtual BYTE	GetType() { return INPROC_DEAD; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);
};

class CInputDB : public CInputProcessor
{
public:
	virtual bool Process(LPDESC d, const void * c_pvOrig, int iBytes, int & r_iBytesProceed);
	virtual BYTE GetType() { return INPROC_DB; }

protected:
	virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);

protected:
	void		MapLocations(std::unique_ptr<network::DGMapLocationsPacket> data);
	void		LoginSuccess(DWORD dwHandle, std::unique_ptr<network::DGLoginSuccessPacket> data);
	void		PlayerCreateFailure(LPDESC d, BYTE bType);	// 0 = ÀÏ¹Ý ½ÇÆÐ 1 = ÀÌ¹Ì ÀÖÀ½
	void		PlayerDeleteSuccess(LPDESC d, std::unique_ptr<network::DGPlayerDeleteSuccessPacket> pack);
	void		PlayerDeleteFail(LPDESC d);
	void		PlayerLoad(LPDESC d, std::unique_ptr<network::DGPlayerLoadPacket> pack);
	void		PlayerCreateSuccess(LPDESC d, std::unique_ptr<network::DGPlayerCreateSuccessPacket> pPacketDB);
	void		Boot(std::unique_ptr<network::DGBootPacket> pack);
	void		QuestLoad(LPDESC d, std::unique_ptr<network::DGQuestLoadPacket> data);
	void		SafeboxLoad(LPDESC d, std::unique_ptr<network::DGSafeboxLoadPacket> data);
	void		SafeboxChangeSize(LPDESC d, std::unique_ptr<network::DGSafeboxChangeSizePacket> data);
	void		SafeboxWrongPassword(LPDESC d);
	void		SafeboxChangePasswordAnswer(LPDESC d, std::unique_ptr<network::DGSafeboxChangePasswordAnswerPacket> data);
	void		EmpireSelect(LPDESC d, std::unique_ptr<network::DGEmpireSelectPacket> data);
	void		P2P(std::unique_ptr<network::DGP2PInfoPacket> p);
	void		ItemLoad(LPDESC d, std::unique_ptr<network::DGItemLoadPacket> data);
	void		AffectLoad(LPDESC d, std::unique_ptr<network::DGAffectLoadPacket> data);
	void		OfflineMessagesLoad(LPDESC d, std::unique_ptr<network::DGOfflineMessagesLoadPacket> data);
#ifdef __ITEM_REFUND__
	void		ItemRefundLoad(LPDESC d, std::unique_ptr<network::DGItemRefundLoadPacket> data);
#endif

	void		GuildLoad(std::unique_ptr<network::DGGuildLoadPacket> data);
	void		GuildSkillUpdate(std::unique_ptr<network::DGGuildSkillUpdatePacket> data);
	void		GuildSkillRecharge();
	void		GuildExpUpdate(std::unique_ptr<network::DGGuildExpUpdatePacket> data);
	void		GuildAddMember(std::unique_ptr<network::DGGuildAddMemberPacket> data);
	void		GuildRemoveMember(std::unique_ptr<network::DGGuildRemoveMemberPacket> data);
	void		GuildChangeGrade(std::unique_ptr<network::DGGuildChangeGradePacket> data);
	void		GuildChangeMemberData(std::unique_ptr<network::DGGuildChangeMemberDataPacket> data);
	void		GuildDisband(std::unique_ptr<network::DGGuildDisbandPacket> data);
	void		GuildLadder(std::unique_ptr<network::DGGuildLadderPacket> data);
	void		GuildWar(std::unique_ptr<network::DGGuildWarPacket> data);
	void		GuildWarScore(std::unique_ptr<network::DGGuildWarScorePacket> data);
	void		GuildSkillUsableChange(std::unique_ptr<network::DGGuildSkillUsableChangePacket> data);
	void		GuildMoneyChange(std::unique_ptr<network::DGGuildMoneyChangePacket> data);
	void		GuildMoneyWithdraw(std::unique_ptr<network::DGGuildMoneyWithdrawPacket> data);
	void		GuildWarReserveAdd(std::unique_ptr<network::DGGuildWarReserveAddPacket> data);
	void		GuildWarReserveDelete(std::unique_ptr<network::DGGuildWarReserveDeletePacket> data);
	void		GuildWarBet(std::unique_ptr<network::DGGuildWarBetPacket> data);
	void		GuildChangeMaster(std::unique_ptr<network::DGGuildChangeMasterPacket> data);

	void		LoginAlready(LPDESC d, std::unique_ptr<network::DGLoginAlreadyPacket> p);

	void		PartyCreate(std::unique_ptr<network::DGPartyCreatePacket> data);
	void		PartyDelete(std::unique_ptr<network::DGPartyDeletePacket> data);
	void		PartyAdd(std::unique_ptr<network::DGPartyAddPacket> data);
	void		PartyRemove(std::unique_ptr<network::DGPartyRemovePacket> data);
	void		PartyStateChange(std::unique_ptr<network::DGPartyStateChangePacket> data);
	void		PartySetMemberLevel(std::unique_ptr<network::DGPartySetMemberLevelPacket> data);

	void		Time(std::unique_ptr<network::DGTimePacket> data);

	void		ReloadProto(std::unique_ptr<network::DGReloadProtoPacket> data);
	void		ReloadShopTable(std::unique_ptr<network::DGReloadShopTablePacket> data);
	void		ReloadMobProto(std::unique_ptr<network::DGReloadMobProtoPacket> data);
	void		ChangeName(LPDESC d, std::unique_ptr<network::DGChangeNamePacket> p);

	void		AuthLogin(LPDESC d, std::unique_ptr<network::DGAuthLoginPacket> data);
	void		ItemAward(const char * c_pData);

	void		ChangeEmpirePriv(std::unique_ptr<network::DGChangeEmpirePrivPacket> p);
	void		ChangeGuildPriv(std::unique_ptr<network::DGChangeGuildPrivPacket> p);
	void		ChangeCharacterPriv(std::unique_ptr<network::DGChangeCharacterPrivPacket> p);

	void		SetEventFlag(std::unique_ptr<network::DGSetEventFlagPacket> data);

#ifdef __DEPRECATED_BILLING__
	void		BillingRepair(std::unique_ptr<network::DGBillingRepairPacket> data);
	void		BillingExpire(std::unique_ptr<network::DGBillingExpirePacket> data);
	void		BillingLogin(std::unique_ptr<network::DGBillingLoginPacket> data);
	void		BillingCheck(std::unique_ptr<network::DGBillingCheckPacket> data);
#endif

	void		CreateObject(std::unique_ptr<network::DGCreateObjectPacket> data);
	void		DeleteObject(std::unique_ptr<network::DGDeleteObjectPacket> data);
	void		UpdateLand(std::unique_ptr<network::DGUpdateLandPacket> data);

	void		Notice(std::unique_ptr<network::DGNoticePacket> data);

	void		MarriageAdd(std::unique_ptr<network::DGMarriageAddPacket> data);
	void		MarriageUpdate(std::unique_ptr<network::DGMarriageUpdatePacket> data);
	void		MarriageRemove(std::unique_ptr<network::DGMarriageRemovePacket> data);

	void		WeddingRequest(std::unique_ptr<network::DGWeddingRequestPacket> data);
	void		WeddingReady(std::unique_ptr<network::DGWeddingReadyPacket> data);
	void		WeddingStart(std::unique_ptr<network::DGWeddingStartPacket> data);
	void		WeddingEnd(std::unique_ptr<network::DGWeddingEndPacket> data);

#ifdef __MAINTENANCE__
	void		RecvShutdown(std::unique_ptr<network::DGMaintenancePacket> pack);
#endif

#ifdef __CHECK_P2P_BROKEN__
	void		RecvP2PCheck(std::unique_ptr<network::DGRecvP2PCheckPacket> p);
#endif

	// MYSHOP_PRICE_LIST
	/// ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® ¿äÃ»¿¡ ´ëÇÑ ÀÀ´ä ÆÐÅ¶(HEADER_DG_MYSHOP_PRICELIST_RES) Ã³¸®ÇÔ¼ö
	/**
	* @param	d ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ®¸¦ ¿äÃ»ÇÑ ÇÃ·¹ÀÌ¾îÀÇ descriptor
	* @param	p ÆÐÅ¶µ¥ÀÌÅÍÀÇ Æ÷ÀÎÅÍ
	*/
	void		MyshopPricelistRes( LPDESC d, std::unique_ptr<network::DGMyShopPricelistPacket> p );
	// END_OF_MYSHOP_PRICE_LIST
	//
	//RELOAD_ADMIN
	void ReloadAdmin(std::unique_ptr<network::DGReloadAdminPacket> data);
	//END_RELOAD_ADMIN

	void		DetailLog(std::unique_ptr<network::DGDetailLogPacket> data);
	// µ¶ÀÏ ¼±¹° ±â´É Å×½ºÆ®
	void		ItemAwardInformer(std::unique_ptr<network::DGItemAwardInformerPacket> data);

#ifdef __GUILD_SAFEBOX__
	void		GuildSafebox(DWORD dwHandle, std::unique_ptr<network::DGGuildSafeboxPacket> data);
#endif

#ifdef __DUNGEON_FOR_GUILD__
	void		GuildDungeon(std::unique_ptr<network::DGGuildDungeonPacket> sPacket);
	void		GuildDungeonCD(std::unique_ptr<network::DGGuildDungeonCDPacket> sPacket);
#endif

	void		WhisperPlayerExistResult(LPDESC desc, std::unique_ptr<network::DGWhisperPlayerExistResultPacket> p);
	void		WhisperPlayerMessageOffline(LPDESC desc, std::unique_ptr<network::DGWhisperPlayerMessageOfflinePacket> p);

#ifdef ENABLE_XMAS_EVENT
	void		ReloadXmasRewards(std::unique_ptr<network::DGReloadXmasRewardsPacket> data);
#endif

#ifdef CHANGE_SKILL_COLOR
	void		SkillColorLoad(LPDESC d, std::unique_ptr<network::DGSkillColorLoadPacket> data);
#endif
#ifdef __EQUIPMENT_CHANGER__
	void		EquipmentPageLoad(LPDESC d, std::unique_ptr<network::DGEquipmentPageLoadPacket> data);
#endif

#ifdef __PET_ADVANCED__
	void		PetLoad(std::unique_ptr<network::DGPetLoadPacket> data);
#endif

#ifdef AUCTION_SYSTEM
	void		AuctionDeletePlayer(std::unique_ptr<network::DGAuctionDeletePlayer> p);
#endif

	protected:
		DWORD		m_dwHandle;
};

class CInputP2P : public CInputProcessor
{
	public:
		virtual BYTE	GetType() { return INPROC_P2P; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);

	public:
		void		Setup(LPDESC d, std::unique_ptr<network::GGSetupPacket> p);
		void		Login(LPDESC d, std::unique_ptr<network::GGLoginPacket> p);
		void		Logout(LPDESC d, std::unique_ptr<network::GGLogoutPacket> p);
		void		Relay(LPDESC d, std::unique_ptr<network::GGRelayPacket> p);
		void		Notice(LPDESC d, std::unique_ptr<network::GGNoticePacket> p);
		void		SuccessNotice(LPDESC d, std::unique_ptr<network::GGSuccessNoticePacket> p);
		void		Guild(LPDESC d, const network::InputPacket& packet);
		void		RecvShutdown(LPDESC d, std::unique_ptr<network::GGRecvShutdownPacket> p);
		void		Shout(std::unique_ptr<network::GGShoutPacket> p);
		void		Disconnect(std::unique_ptr<network::GGDisconnectPacket> p);
		void		MessengerAdd(std::unique_ptr<network::GGMessengerAddPacket> p);
		void		MessengerRemove(std::unique_ptr<network::GGMessengerRemovePacket> p);
#ifdef ENABLE_MESSENGER_BLOCK
		void		MessengerBlockAdd(std::unique_ptr<network::GGMessengerBlockAddPacket> p);
		void		MessengerBlockRemove(std::unique_ptr<network::GGMessengerBlockRemovePacket> p);
#endif
		void		MessengerMobile(const char * c_pData);
		void		FindPosition(LPDESC d, std::unique_ptr<network::GGFindPositionPacket> p);
		void		WarpCharacter(std::unique_ptr<network::GGWarpCharacterPacket> p);
		void		GuildWarZoneMapIndex(std::unique_ptr<network::GGGuildWarZoneMapIndexPacket> p);
		void		Transfer(std::unique_ptr<network::GGTransferPacket> p);
		void		LoginPing(LPDESC d, std::unique_ptr<network::GGLoginPingPacket> p);
		void		BlockChat(std::unique_ptr<network::GGBlockChatPacket> p);
		void		IamAwake(LPDESC d);

		void		RequestDungeonWarp(LPDESC d, std::unique_ptr<network::GGRequestDungeonWarpPacket> p);
		void		AnswerDungeonWarp(LPDESC d, std::unique_ptr<network::GGAnswerDungeonWarpPacket> p);

		void		UpdateRights(std::unique_ptr<network::GGUpdateRightsPacket> p);
		void		TeamChat(std::unique_ptr<network::GGTeamChatPacket> p);

		void		MessengerRequest(std::unique_ptr<network::GGMessengerRequestPacket> p);
		void		MessengerRequestFail(std::unique_ptr<network::GGMessengerRequestFailPacket> p);

		void		PlayerPacket(std::unique_ptr<network::GGPlayerPacket> p);

		void		TeamlerStatus(std::unique_ptr<network::GGTeamlerStatusPacket> p);
		
		void		FlushPlayer(std::unique_ptr<network::GGFlushPlayerPacket> p);

#ifdef __HOMEPAGE_COMMAND__
		void		HomepageCommand(std::unique_ptr<network::GGHomepageCommandPacket> p);
#endif

		void		PullOfflineMessages(std::unique_ptr<network::GGPullOfflineMessagesPacket> p);

#ifdef __EVENT_MANAGER__
		void		EventManagerOpenRegistration(std::unique_ptr<network::GGEventManagerOpenRegistrationPacket> p);
		void		EventManagerCloseRegistration(std::unique_ptr<network::GGEventManagerCloseRegistrationPacket> p);
		void		EventManagerOver();
		void		EventManagerIgnorePlayer(std::unique_ptr<network::GGEventManagerIgnorePlayerPacket> p);
		void		EventManagerOpenAnnouncement(std::unique_ptr<network::GGEventManagerOpenAnnouncementPacket> p);
		void		EventManagerTagTeamRegister(std::unique_ptr<network::GGEventManagerTagTeamRegisterPacket> p);
		void		EventManagerTagTeamUnregister(std::unique_ptr<network::GGEventManagerTagTeamUnregisterPacket> p);
		void		EventManagerTagTeamCreate(std::unique_ptr<network::GGEventManagerTagTeamCreatePacket> p);
#endif

#ifdef COMBAT_ZONE
		void		CombatZoneRanking(std::unique_ptr<network::GGCombatZoneRankingPacket> p);
#endif

#ifdef DMG_RANKING
	public:
		void		DmgRankingUpdate(std::unique_ptr<network::GGDmgRankingUpdatePacket> p);
#endif
		
#ifdef LOCALE_SAVE_LAST_USAGE
	public:
		void		LocaleUpdateLastUsage(std::unique_ptr<network::GGLocaleUpdateLastUsagePacket> p);
#endif

		void		GiveItem(std::unique_ptr<network::GGGiveItemPacket> p);
		void		GiveGold(std::unique_ptr<network::GGGiveGoldPacket> p);

#ifdef AUCTION_SYSTEM
		void		AuctionInsertItem(LPDESC d, std::unique_ptr<network::GGAuctionInsertItemPacket> p);
		void		AuctionTakeItem(LPDESC d, std::unique_ptr<network::GGAuctionTakeItemPacket> p);
		void		AuctionBuyItem(LPDESC d, std::unique_ptr<network::GGAuctionBuyItemPacket> p);
		void		AuctionTakeGold(LPDESC d, std::unique_ptr<network::GGAuctionTakeGoldPacket> p);
		void		AuctionSearchItems(LPDESC d, std::unique_ptr<network::GGAuctionSearchItemsPacket> p);
		void		AuctionExtendedSearchItems(LPDESC d, std::unique_ptr<network::GGAuctionExtendedSearchItemsPacket> p);
		void		AuctionMarkShop(LPDESC d, std::unique_ptr<network::GGAuctionMarkShopPacket> p);
		void		AuctionAnswerMarkShop(LPDESC d, std::unique_ptr<network::GGAuctionAnswerMarkShopPacket> p);

		void		AuctionShopRequestShow(LPDESC d, std::unique_ptr<network::GGAuctionShopRequestShowPacket> p);
		void		AuctionShopOpen(LPDESC d, std::unique_ptr<network::GGAuctionShopOpenPacket> p);
		void		AuctionShopTakeGold(LPDESC d, std::unique_ptr<network::GGAuctionShopTakeGoldPacket> p);
		void		AuctionShopSpawn(LPDESC d, std::unique_ptr<network::GGAuctionShopSpawnPacket> p);
		void		AuctionShopDespawn(LPDESC d, std::unique_ptr<network::GGAuctionShopDespawnPacket> p);
		void		AuctionShopView(LPDESC d, std::unique_ptr<network::GGAuctionShopViewPacket> p);
		void		AuctionShopViewCancel(LPDESC d, std::unique_ptr<network::GGAuctionShopViewCancelPacket> p);
		void		AuctionShopRenew(LPDESC d, std::unique_ptr<network::GGAuctionShopRenewPacket> p);
		void		AuctionShopClose(LPDESC d, std::unique_ptr<network::GGAuctionShopClosePacket> p);
		void		AuctionShopRequestHistory(LPDESC d, std::unique_ptr<network::GGAuctionShopRequestHistoryPacket> p);
		void		AuctionRequestAveragePrice(LPDESC d, std::unique_ptr<network::GGAuctionRequestAveragePricePacket> p);
#endif
};

class CInputAuth : public CInputProcessor
{
	public:
		virtual BYTE GetType() { return INPROC_AUTH; }

	protected:
		virtual bool	Analyze(LPDESC d, const network::InputPacket& packet);

	public:
		void		LoginVersionCheck(LPDESC d, std::unique_ptr<network::CGLoginVersionCheckPacket> p);
		void		Login(LPDESC d, std::unique_ptr<network::CGAuthLoginPacket> pinfo);
};

#include <google/protobuf/port_undef.inc>

#endif