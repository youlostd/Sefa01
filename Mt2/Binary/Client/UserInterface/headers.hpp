#pragma once

#include "headers_basic.hpp"
#include "protobuf_cg_packets.h"
#include "protobuf_gc_packets.h"

#include <exception>

namespace network
{
#pragma pack(1)
	struct TPacketHeader
	{
		uint16_t header;
		uint32_t size;

		TPacketHeader() :
			header(0),
			size(0)
		{
		}
	};

	template <typename T>
	struct OutputPacket {
		virtual ~OutputPacket() = default;

		TPacketHeader header;
		T data;

		T* operator->() noexcept { return &data; }
		const T* operator->() const noexcept { return &data; }
		T& operator*() noexcept { return data; }
		const T& operator*() const noexcept { return data; }

		uint16_t get_header() const noexcept { return header.header; }
	};
#pragma pack()

    template <typename T>
    struct CGOutputPacket : OutputPacket<T> {
        CGOutputPacket()
        {
            throw std::invalid_argument("not handled packet type");
        }
    };
    template <>
    struct CGOutputPacket<CGHandshakePacket> : OutputPacket<CGHandshakePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::HANDSHAKE);
        }
    };
    template <>
    struct CGOutputPacket<CGKeyAgreementPacket> : OutputPacket<CGKeyAgreementPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::KEY_AGREEMENT);
        }
    };
    template <>
    struct CGOutputPacket<CGClientVersionPacket> : OutputPacket<CGClientVersionPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::CLIENT_VERSION);
        }
    };
    template <>
    struct CGOutputPacket<CGHackPacket> : OutputPacket<CGHackPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::HACK);
        }
    };
    template <>
    struct CGOutputPacket<CGXTRAPAckPacket> : OutputPacket<CGXTRAPAckPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::XTRAP_ACK);
        }
    };
    template <>
    struct CGOutputPacket<CGLoginVersionCheckPacket> : OutputPacket<CGLoginVersionCheckPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::LOGIN_VERSION_CHECK);
        }
    };
    template <>
    struct CGOutputPacket<CGAuthLoginPacket> : OutputPacket<CGAuthLoginPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUTH_LOGIN);
        }
    };
    template <>
    struct CGOutputPacket<CGAuthOpenIDLoginPacket> : OutputPacket<CGAuthOpenIDLoginPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUTH_OPENID_LOGIN);
        }
    };
    template <>
    struct CGOutputPacket<CGAuthPasspodAnswerPacket> : OutputPacket<CGAuthPasspodAnswerPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUTH_PASSPOD_ANSWER);
        }
    };
    template <>
    struct CGOutputPacket<CGLoginByKeyPacket> : OutputPacket<CGLoginByKeyPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::LOGIN_BY_KEY);
        }
    };
    template <>
    struct CGOutputPacket<CGEmpirePacket> : OutputPacket<CGEmpirePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EMPIRE);
        }
    };
    template <>
    struct CGOutputPacket<CGPlayerSelectPacket> : OutputPacket<CGPlayerSelectPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PLAYER_SELECT);
        }
    };
    template <>
    struct CGOutputPacket<CGPlayerCreatePacket> : OutputPacket<CGPlayerCreatePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PLAYER_CREATE);
        }
    };
    template <>
    struct CGOutputPacket<CGPlayerDeletePacket> : OutputPacket<CGPlayerDeletePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PLAYER_DELETE);
        }
    };
    template <>
    struct CGOutputPacket<CGPlayerChangeNamePacket> : OutputPacket<CGPlayerChangeNamePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PLAYER_CHANGE_NAME);
        }
    };
    template <>
    struct CGOutputPacket<CGPlayerHairSelectPacket> : OutputPacket<CGPlayerHairSelectPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PLAYER_SELECT_HAIR);
        }
    };
    template <>
    struct CGOutputPacket<CGScriptAnswerPacket> : OutputPacket<CGScriptAnswerPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SCRIPT_ANSWER);
        }
    };
    template <>
    struct CGOutputPacket<CGScriptButtonPacket> : OutputPacket<CGScriptButtonPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SCRIPT_BUTTON);
        }
    };
    template <>
    struct CGOutputPacket<CGScriptSelectItemPacket> : OutputPacket<CGScriptSelectItemPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SCRIPT_SELECT_ITEM);
        }
    };
    template <>
    struct CGOutputPacket<CGQuestInputStringPacket> : OutputPacket<CGQuestInputStringPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::QUEST_INPUT_STRING);
        }
    };
    template <>
    struct CGOutputPacket<CGQuestConfirmPacket> : OutputPacket<CGQuestConfirmPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::QUEST_CONFIRM);
        }
    };
    template <>
    struct CGOutputPacket<CGItemUsePacket> : OutputPacket<CGItemUsePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_USE);
        }
    };
    template <>
    struct CGOutputPacket<CGItemUseToItemPacket> : OutputPacket<CGItemUseToItemPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_USE_TO_ITEM);
        }
    };
    template <>
    struct CGOutputPacket<CGItemDropPacket> : OutputPacket<CGItemDropPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_DROP);
        }
    };
    template <>
    struct CGOutputPacket<CGItemDestroyPacket> : OutputPacket<CGItemDestroyPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_DESTROY);
        }
    };
    template <>
    struct CGOutputPacket<CGItemMovePacket> : OutputPacket<CGItemMovePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_MOVE);
        }
    };
    template <>
    struct CGOutputPacket<CGItemPickupPacket> : OutputPacket<CGItemPickupPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_PICKUP);
        }
    };
    template <>
    struct CGOutputPacket<CGSafeboxCheckinPacket> : OutputPacket<CGSafeboxCheckinPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SAFEBOX_CHECKIN);
        }
    };
    template <>
    struct CGOutputPacket<CGSafeboxCheckoutPacket> : OutputPacket<CGSafeboxCheckoutPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SAFEBOX_CHECKOUT);
        }
    };
    template <>
    struct CGOutputPacket<CGSafeboxItemMovePacket> : OutputPacket<CGSafeboxItemMovePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SAFEBOX_ITEM_MOVE);
        }
    };
    template <>
    struct CGOutputPacket<CGAcceRefineCheckinPacket> : OutputPacket<CGAcceRefineCheckinPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ACCE_REFINE_CHECKIN);
        }
    };
    template <>
    struct CGOutputPacket<CGAcceRefineCheckoutPacket> : OutputPacket<CGAcceRefineCheckoutPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ACCE_REFINE_CHECKOUT);
        }
    };
    template <>
    struct CGOutputPacket<CGAcceRefineAcceptPacket> : OutputPacket<CGAcceRefineAcceptPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ACCE_REFINE_ACCEPT);
        }
    };
    template <>
    struct CGOutputPacket<CGQuickslotAddPacket> : OutputPacket<CGQuickslotAddPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::QUICKSLOT_ADD);
        }
    };
    template <>
    struct CGOutputPacket<CGQuickslotDeletePacket> : OutputPacket<CGQuickslotDeletePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::QUICKSLOT_DELETE);
        }
    };
    template <>
    struct CGOutputPacket<CGQuickslotSwapPacket> : OutputPacket<CGQuickslotSwapPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::QUICKSLOT_SWAP);
        }
    };
    template <>
    struct CGOutputPacket<CGShopBuyPacket> : OutputPacket<CGShopBuyPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SHOP_BUY);
        }
    };
    template <>
    struct CGOutputPacket<CGShopSellPacket> : OutputPacket<CGShopSellPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SHOP_SELL);
        }
    };
    template <>
    struct CGOutputPacket<CGExchangeStartPacket> : OutputPacket<CGExchangeStartPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EXCHANGE_START);
        }
    };
    template <>
    struct CGOutputPacket<CGExchangeItemAddPacket> : OutputPacket<CGExchangeItemAddPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EXCHANGE_ITEM_ADD);
        }
    };
    template <>
    struct CGOutputPacket<CGExchangeItemDelPacket> : OutputPacket<CGExchangeItemDelPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EXCHANGE_ITEM_DEL);
        }
    };
    template <>
    struct CGOutputPacket<CGExchangeGoldAddPacket> : OutputPacket<CGExchangeGoldAddPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EXCHANGE_GOLD_ADD);
        }
    };
    template <>
    struct CGOutputPacket<CGMessengerAddByVIDPacket> : OutputPacket<CGMessengerAddByVIDPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MESSENGER_ADD_BY_VID);
        }
    };
    template <>
    struct CGOutputPacket<CGMessengerAddByNamePacket> : OutputPacket<CGMessengerAddByNamePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MESSENGER_ADD_BY_NAME);
        }
    };
    template <>
    struct CGOutputPacket<CGMessengerRemovePacket> : OutputPacket<CGMessengerRemovePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MESSENGER_REMOVE);
        }
    };
    template <>
    struct CGOutputPacket<CGMessengerAddBlockByVIDPacket> : OutputPacket<CGMessengerAddBlockByVIDPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MESSENGER_ADD_BLOCK_BY_VID);
        }
    };
    template <>
    struct CGOutputPacket<CGMessengerAddBlockByNamePacket> : OutputPacket<CGMessengerAddBlockByNamePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MESSENGER_ADD_BLOCK_BY_NAME);
        }
    };
    template <>
    struct CGOutputPacket<CGMessengerRemoveBlockPacket> : OutputPacket<CGMessengerRemoveBlockPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MESSENGER_REMOVE_BLOCK);
        }
    };
    template <>
    struct CGOutputPacket<CGPartyInvitePacket> : OutputPacket<CGPartyInvitePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PARTY_INVITE);
        }
    };
    template <>
    struct CGOutputPacket<CGPartyInviteAnswerPacket> : OutputPacket<CGPartyInviteAnswerPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PARTY_INVITE_ANSWER);
        }
    };
    template <>
    struct CGOutputPacket<CGPartyRemovePacket> : OutputPacket<CGPartyRemovePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PARTY_REMOVE);
        }
    };
    template <>
    struct CGOutputPacket<CGPartySetStatePacket> : OutputPacket<CGPartySetStatePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PARTY_SET_STATE);
        }
    };
    template <>
    struct CGOutputPacket<CGPartyUseSkillPacket> : OutputPacket<CGPartyUseSkillPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PARTY_USE_SKILL);
        }
    };
    template <>
    struct CGOutputPacket<CGPartyParameterPacket> : OutputPacket<CGPartyParameterPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PARTY_PARAMETER);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildDepositMoneyPacket> : OutputPacket<CGGuildDepositMoneyPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_DEPOSIT_MONEY);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildWithdrawMoneyPacket> : OutputPacket<CGGuildWithdrawMoneyPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_WITHDRAW_MONEY);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildAddMemberPacket> : OutputPacket<CGGuildAddMemberPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_ADD_MEMBER);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildRemoveMemberPacket> : OutputPacket<CGGuildRemoveMemberPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_REMOVE_MEMBER);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildChangeGradeNamePacket> : OutputPacket<CGGuildChangeGradeNamePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_CHANGE_GRADE_NAME);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildChangeGradeAuthorityPacket> : OutputPacket<CGGuildChangeGradeAuthorityPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_CHANGE_GRADE_AUTHORITY);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildOfferExpPacket> : OutputPacket<CGGuildOfferExpPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_OFFER_EXP);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildChargeGSPPacket> : OutputPacket<CGGuildChargeGSPPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_CHARGE_GSP);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildPostCommentPacket> : OutputPacket<CGGuildPostCommentPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_POST_COMMENT);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildDeleteCommentPacket> : OutputPacket<CGGuildDeleteCommentPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_DELETE_COMMENT);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildChangeMemberGradePacket> : OutputPacket<CGGuildChangeMemberGradePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_CHANGE_MEMBER_GRADE);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildUseSkillPacket> : OutputPacket<CGGuildUseSkillPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_USE_SKILL);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildChangeMemberGeneralPacket> : OutputPacket<CGGuildChangeMemberGeneralPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_CHANGE_MEMBER_GENERAL);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildInviteAnswerPacket> : OutputPacket<CGGuildInviteAnswerPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_INVITE_ANSWER);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildAnswerMakePacket> : OutputPacket<CGGuildAnswerMakePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_ANSWER_MAKE);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildRequestListPacket> : OutputPacket<CGGuildRequestListPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_REQUEST_LIST);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSearchPacket> : OutputPacket<CGGuildSearchPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SEARCH);
        }
    };
    template <>
    struct CGOutputPacket<CGMarkCRCListPacket> : OutputPacket<CGMarkCRCListPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MARK_CRC_LIST);
        }
    };
    template <>
    struct CGOutputPacket<CGMarkUploadPacket> : OutputPacket<CGMarkUploadPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MARK_UPLOAD);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSymbolUploadPacket> : OutputPacket<CGGuildSymbolUploadPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SYMBOL_UPLOAD);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSymbolCRCPacket> : OutputPacket<CGGuildSymbolCRCPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SYMBOL_CRC);
        }
    };
    template <>
    struct CGOutputPacket<CGMovePacket> : OutputPacket<CGMovePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MOVE);
        }
    };
    template <>
    struct CGOutputPacket<CGChatPacket> : OutputPacket<CGChatPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::CHAT);
        }
    };
    template <>
    struct CGOutputPacket<CGAttackPacket> : OutputPacket<CGAttackPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ATTACK);
        }
    };
    template <>
    struct CGOutputPacket<CGShootPacket> : OutputPacket<CGShootPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SHOOT);
        }
    };
    template <>
    struct CGOutputPacket<CGWhisperPacket> : OutputPacket<CGWhisperPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::WHISPER);
        }
    };
    template <>
    struct CGOutputPacket<CGOnClickPacket> : OutputPacket<CGOnClickPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ON_CLICK);
        }
    };
    template <>
    struct CGOutputPacket<CGPositionPacket> : OutputPacket<CGPositionPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::POSITION);
        }
    };
    template <>
    struct CGOutputPacket<CGNextSkillUsePacket> : OutputPacket<CGNextSkillUsePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::NEXT_SKILL_USE);
        }
    };
    template <>
    struct CGOutputPacket<CGSyncPositionPacket> : OutputPacket<CGSyncPositionPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SYNC_POSITION);
        }
    };
    template <>
    struct CGOutputPacket<CGFlyTargetPacket> : OutputPacket<CGFlyTargetPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::FLY_TARGET);
        }
    };
    template <>
    struct CGOutputPacket<CGAddFlyTargetPacket> : OutputPacket<CGAddFlyTargetPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ADD_FLY_TARGET);
        }
    };
    template <>
    struct CGOutputPacket<CGUseSkillPacket> : OutputPacket<CGUseSkillPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::USE_SKILL);
        }
    };
    template <>
    struct CGOutputPacket<CGTargetPacket> : OutputPacket<CGTargetPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::TARGET);
        }
    };
    template <>
    struct CGOutputPacket<CGTargetMonsterDropInfoPacket> : OutputPacket<CGTargetMonsterDropInfoPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::TARGET_MONSTER_DROP_INFO);
        }
    };
    template <>
    struct CGOutputPacket<CGCostumeVisibilityPacket> : OutputPacket<CGCostumeVisibilityPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::COSTUME_VISIBILITY);
        }
    };
    template <>
    struct CGOutputPacket<CGFishingPacket> : OutputPacket<CGFishingPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::FISHING);
        }
    };
    template <>
    struct CGOutputPacket<CGGiveItemPacket> : OutputPacket<CGGiveItemPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GIVE_ITEM);
        }
    };
    template <>
    struct CGOutputPacket<CGMyShopPacket> : OutputPacket<CGMyShopPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::MYSHOP);
        }
    };
    template <>
    struct CGOutputPacket<CGRefinePacket> : OutputPacket<CGRefinePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::REFINE);
        }
    };
    template <>
    struct CGOutputPacket<CGPlayerLanguageInformationPacket> : OutputPacket<CGPlayerLanguageInformationPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PLAYER_LANGUAGE_INFORMATION);
        }
    };
    template <>
    struct CGOutputPacket<CGReportPacket> : OutputPacket<CGReportPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::REPORT);
        }
    };
    template <>
    struct CGOutputPacket<CGDragonSoulRefinePacket> : OutputPacket<CGDragonSoulRefinePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::DRAGON_SOUL_REFINE);
        }
    };
    template <>
    struct CGOutputPacket<CGOnQuestTriggerPacket> : OutputPacket<CGOnQuestTriggerPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ON_QUEST_TRIGGER);
        }
    };
    template <>
    struct CGOutputPacket<CGItemMultiUsePacket> : OutputPacket<CGItemMultiUsePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::ITEM_MULTI_USE);
        }
    };
    template <>
    struct CGOutputPacket<CGBotReportLogPacket> : OutputPacket<CGBotReportLogPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::BOT_REPORT_LOG);
        }
    };
    template <>
    struct CGOutputPacket<CGForcedRewarpPacket> : OutputPacket<CGForcedRewarpPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::FORCED_REWARP);
        }
    };
    template <>
    struct CGOutputPacket<CGUseDetachmentSinglePacket> : OutputPacket<CGUseDetachmentSinglePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::USE_DETACHMENT_SINGLE);
        }
    };
    template <>
    struct CGOutputPacket<CGEventRequestAnswerPacket> : OutputPacket<CGEventRequestAnswerPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EVENT_REQUEST_ANSWER);
        }
    };
    template <>
    struct CGOutputPacket<CGCostumeBonusTransferPacket> : OutputPacket<CGCostumeBonusTransferPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::COSTUME_BONUS_TRANSFER);
        }
    };
    template <>
    struct CGOutputPacket<CGRunePagePacket> : OutputPacket<CGRunePagePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::RUNE_PAGE);
        }
    };
    template <>
    struct CGOutputPacket<CGRecvWikiPacket> : OutputPacket<CGRecvWikiPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::RECV_WIKI);
        }
    };
    template <>
    struct CGOutputPacket<CGEquipmentPageAddPacket> : OutputPacket<CGEquipmentPageAddPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EQUIPMENT_PAGE_ADD);
        }
    };
    template <>
    struct CGOutputPacket<CGEquipmentPageDeletePacket> : OutputPacket<CGEquipmentPageDeletePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EQUIPMENT_PAGE_DELETE);
        }
    };
    template <>
    struct CGOutputPacket<CGEquipmentPageSelectPacket> : OutputPacket<CGEquipmentPageSelectPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::EQUIPMENT_PAGE_SELECT);
        }
    };
    template <>
    struct CGOutputPacket<CGCombatZoneRequestActionPacket> : OutputPacket<CGCombatZoneRequestActionPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::COMBAT_ZONE_REQUEST_ACTION);
        }
    };
    template <>
    struct CGOutputPacket<CGSetSkillColorPacket> : OutputPacket<CGSetSkillColorPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::SET_SKILL_COLOR);
        }
    };
    template <>
    struct CGOutputPacket<CGCrystalRefinePacket> : OutputPacket<CGCrystalRefinePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::CRYSTAL_REFINE);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSafeboxCheckinPacket> : OutputPacket<CGGuildSafeboxCheckinPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SAFEBOX_CHECKIN);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSafeboxCheckoutPacket> : OutputPacket<CGGuildSafeboxCheckoutPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SAFEBOX_CHECKOUT);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSafeboxItemMovePacket> : OutputPacket<CGGuildSafeboxItemMovePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SAFEBOX_ITEM_MOVE);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSafeboxGiveGoldPacket> : OutputPacket<CGGuildSafeboxGiveGoldPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SAFEBOX_GIVE_GOLD);
        }
    };
    template <>
    struct CGOutputPacket<CGGuildSafeboxGetGoldPacket> : OutputPacket<CGGuildSafeboxGetGoldPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::GUILD_SAFEBOX_GET_GOLD);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionInsertItemPacket> : OutputPacket<CGAuctionInsertItemPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_INSERT_ITEM);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionTakeItemPacket> : OutputPacket<CGAuctionTakeItemPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_TAKE_ITEM);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionBuyItemPacket> : OutputPacket<CGAuctionBuyItemPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_BUY_ITEM);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionTakeGoldPacket> : OutputPacket<CGAuctionTakeGoldPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_TAKE_GOLD);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionSearchItemsPacket> : OutputPacket<CGAuctionSearchItemsPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_SEARCH_ITEMS);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionExtendedSearchItemsPacket> : OutputPacket<CGAuctionExtendedSearchItemsPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_EXTENDED_SEARCH_ITEMS);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionMarkShopPacket> : OutputPacket<CGAuctionMarkShopPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_MARK_SHOP);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionShopOpenPacket> : OutputPacket<CGAuctionShopOpenPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_SHOP_OPEN);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionShopTakeGoldPacket> : OutputPacket<CGAuctionShopTakeGoldPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_SHOP_TAKE_GOLD);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionShopClosePacket> : OutputPacket<CGAuctionShopClosePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_SHOP_CLOSE);
        }
    };
    template <>
    struct CGOutputPacket<CGAuctionRequestAveragePricePacket> : OutputPacket<CGAuctionRequestAveragePricePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::AUCTION_REQUEST_AVG_PRICE);
        }
    };
    template <>
    struct CGOutputPacket<CGPetUseEggPacket> : OutputPacket<CGPetUseEggPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PET_USE_EGG);
        }
    };
    template <>
    struct CGOutputPacket<CGPetResetSkillPacket> : OutputPacket<CGPetResetSkillPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PET_RESET_SKILL);
        }
    };
    template <>
    struct CGOutputPacket<CGPetAttrRefineInfoPacket> : OutputPacket<CGPetAttrRefineInfoPacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PET_ATTR_REFINE_INFO);
        }
    };
    template <>
    struct CGOutputPacket<CGPetAttrRefinePacket> : OutputPacket<CGPetAttrRefinePacket> {
        CGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TCGHeader::PET_ATTR_REFINE);
        }
    };

    template <typename T>
    struct GCOutputPacket : OutputPacket<T> {
        GCOutputPacket()
        {
            throw std::invalid_argument("not handled packet type");
        }
    };
    template <>
    struct GCOutputPacket<GCSetVerifyKeyPacket> : OutputPacket<GCSetVerifyKeyPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SET_VERIFY_KEY);
        }
    };
    template <>
    struct GCOutputPacket<GCLoginFailurePacket> : OutputPacket<GCLoginFailurePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::LOGIN_FAILURE);
        }
    };
    template <>
    struct GCOutputPacket<GCCreateFailurePacket> : OutputPacket<GCCreateFailurePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CREATE_FAILURE);
        }
    };
    template <>
    struct GCOutputPacket<GCDeleteSuccessPacket> : OutputPacket<GCDeleteSuccessPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PLAYER_DELETE_SUCCESS);
        }
    };
    template <>
    struct GCOutputPacket<GCPlayerCreateSuccessPacket> : OutputPacket<GCPlayerCreateSuccessPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PLAYER_CREATE_SUCCESS);
        }
    };
    template <>
    struct GCOutputPacket<GCEmpirePacket> : OutputPacket<GCEmpirePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EMPIRE);
        }
    };
    template <>
    struct GCOutputPacket<GCLoginSuccessPacket> : OutputPacket<GCLoginSuccessPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::LOGIN_SUCCESS);
        }
    };
    template <>
    struct GCOutputPacket<GCAuthSuccessPacket> : OutputPacket<GCAuthSuccessPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUTH_SUCCESS);
        }
    };
    template <>
    struct GCOutputPacket<GCAuthSuccessOpenIDPacket> : OutputPacket<GCAuthSuccessOpenIDPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUTH_SUCCESS_OPEN_ID);
        }
    };
    template <>
    struct GCOutputPacket<GCChangeNamePacket> : OutputPacket<GCChangeNamePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHANGE_NAME);
        }
    };
    template <>
    struct GCOutputPacket<GCHybridCryptSDBPacket> : OutputPacket<GCHybridCryptSDBPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::HYBRIDCRYPT_SDB);
        }
    };
    template <>
    struct GCOutputPacket<GCHybridCryptKeysPacket> : OutputPacket<GCHybridCryptKeysPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::HYBRIDCRYPT_KEYS);
        }
    };
    template <>
    struct GCOutputPacket<GCRespondChannelStatusPacket> : OutputPacket<GCRespondChannelStatusPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::RESPOND_CHANNEL_STATUS);
        }
    };
    template <>
    struct GCOutputPacket<GCPhasePacket> : OutputPacket<GCPhasePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PHASE);
        }
    };
    template <>
    struct GCOutputPacket<GCHandshakePacket> : OutputPacket<GCHandshakePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::HANDSHAKE);
        }
    };
    template <>
    struct GCOutputPacket<GCKeyAgreementPacket> : OutputPacket<GCKeyAgreementPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::KEY_AGREEMENT);
        }
    };
    template <>
    struct GCOutputPacket<GCXTrapCS1RequestPacket> : OutputPacket<GCXTrapCS1RequestPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::XTRAP_CS1_REQUEST);
        }
    };
    template <>
    struct GCOutputPacket<GCPanamaPackPacket> : OutputPacket<GCPanamaPackPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PANAMA_PACK);
        }
    };
    template <>
    struct GCOutputPacket<GCLoginVersionAnswerPacket> : OutputPacket<GCLoginVersionAnswerPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::LOGIN_VERSION_ANSWER);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildSymbolDataPacket> : OutputPacket<GCGuildSymbolDataPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_SYMBOL_DATA);
        }
    };
    template <>
    struct GCOutputPacket<GCMarkIDXListPacket> : OutputPacket<GCMarkIDXListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MARK_IDX_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCMarkBlockPacket> : OutputPacket<GCMarkBlockPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MARK_BLOCK);
        }
    };
    template <>
    struct GCOutputPacket<GCTimePacket> : OutputPacket<GCTimePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TIME);
        }
    };
    template <>
    struct GCOutputPacket<GCChannelPacket> : OutputPacket<GCChannelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHANNEL);
        }
    };
    template <>
    struct GCOutputPacket<GCWhisperPacket> : OutputPacket<GCWhisperPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::WHISPER);
        }
    };
    template <>
    struct GCOutputPacket<GCChatPacket> : OutputPacket<GCChatPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHAT);
        }
    };
    template <>
    struct GCOutputPacket<GCMovePacket> : OutputPacket<GCMovePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MOVE);
        }
    };
    template <>
    struct GCOutputPacket<GCSyncPositionPacket> : OutputPacket<GCSyncPositionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SYNC_POSITION);
        }
    };
    template <>
    struct GCOutputPacket<GCFlyTargetingPacket> : OutputPacket<GCFlyTargetingPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FLY_TARGETING);
        }
    };
    template <>
    struct GCOutputPacket<GCAddFlyTargetingPacket> : OutputPacket<GCAddFlyTargetingPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ADD_FLY_TARGETING);
        }
    };
    template <>
    struct GCOutputPacket<GCTargetPacket> : OutputPacket<GCTargetPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TARGET);
        }
    };
    template <>
    struct GCOutputPacket<GCTargetMonsterInfoPacket> : OutputPacket<GCTargetMonsterInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TARGET_MONSTER_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCPlayerOnlineInformationPacket> : OutputPacket<GCPlayerOnlineInformationPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PLAYER_ONLINE_INFORMATION);
        }
    };
    template <>
    struct GCOutputPacket<GCUpdateCharacterScalePacket> : OutputPacket<GCUpdateCharacterScalePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::UPDATE_CHARACTER_SCALE);
        }
    };
    template <>
    struct GCOutputPacket<GCMaintenanceInfoPacket> : OutputPacket<GCMaintenanceInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MAINTENANCE_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCWarpPacket> : OutputPacket<GCWarpPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::WARP);
        }
    };
    template <>
    struct GCOutputPacket<GCPVPPacket> : OutputPacket<GCPVPPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PVP);
        }
    };
    template <>
    struct GCOutputPacket<GCDuelStartPacket> : OutputPacket<GCDuelStartPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DUEL_START);
        }
    };
    template <>
    struct GCOutputPacket<GCOwnershipPacket> : OutputPacket<GCOwnershipPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::OWNERSHIP);
        }
    };
    template <>
    struct GCOutputPacket<GCPositionPacket> : OutputPacket<GCPositionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::POSITION);
        }
    };
    template <>
    struct GCOutputPacket<GCStunPacket> : OutputPacket<GCStunPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::STUN);
        }
    };
    template <>
    struct GCOutputPacket<GCDeadPacket> : OutputPacket<GCDeadPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DEAD);
        }
    };
    template <>
    struct GCOutputPacket<GCPointsPacket> : OutputPacket<GCPointsPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::POINTS);
        }
    };
    template <>
    struct GCOutputPacket<GCPointChangePacket> : OutputPacket<GCPointChangePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::POINT_CHANGE);
        }
    };
    template <>
    struct GCOutputPacket<GCRealPointSetPacket> : OutputPacket<GCRealPointSetPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::REAL_POINT_SET);
        }
    };
    template <>
    struct GCOutputPacket<GCMotionPacket> : OutputPacket<GCMotionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MOTION);
        }
    };
    template <>
    struct GCOutputPacket<GCDamageInfoPacket> : OutputPacket<GCDamageInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DAMAGE_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCCreateFlyPacket> : OutputPacket<GCCreateFlyPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CREATE_FLY);
        }
    };
    template <>
    struct GCOutputPacket<GCDungeonDestinationPositionPacket> : OutputPacket<GCDungeonDestinationPositionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DUNGEON_DESTINATION_POSITION);
        }
    };
    template <>
    struct GCOutputPacket<GCSkillLevelPacket> : OutputPacket<GCSkillLevelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SKILL_LEVEL);
        }
    };
    template <>
    struct GCOutputPacket<GCWalkModePacket> : OutputPacket<GCWalkModePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::WALK_MODE);
        }
    };
    template <>
    struct GCOutputPacket<GCChangeSkillGroupPacket> : OutputPacket<GCChangeSkillGroupPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHANGE_SKILL_GROUP);
        }
    };
    template <>
    struct GCOutputPacket<GCRefineInformationPacket> : OutputPacket<GCRefineInformationPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::REFINE_INFORMATION);
        }
    };
    template <>
    struct GCOutputPacket<GCSpecialEffectPacket> : OutputPacket<GCSpecialEffectPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SPECIAL_EFFECT);
        }
    };
    template <>
    struct GCOutputPacket<GCNPCListPacket> : OutputPacket<GCNPCListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::NPC_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCViewEquipPacket> : OutputPacket<GCViewEquipPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::VIEW_EQUIP);
        }
    };
    template <>
    struct GCOutputPacket<GCLandListPacket> : OutputPacket<GCLandListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::LAND_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCTargetCreatePacket> : OutputPacket<GCTargetCreatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TARGET_CREATE);
        }
    };
    template <>
    struct GCOutputPacket<GCTargetUpdatePacket> : OutputPacket<GCTargetUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TARGET_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCTargetDeletePacket> : OutputPacket<GCTargetDeletePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TARGET_DELETE);
        }
    };
    template <>
    struct GCOutputPacket<GCAffectAddPacket> : OutputPacket<GCAffectAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AFFECT_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCAffectRemovePacket> : OutputPacket<GCAffectRemovePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AFFECT_REMOVE);
        }
    };
    template <>
    struct GCOutputPacket<GCLoverInfoPacket> : OutputPacket<GCLoverInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::LOVER_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCLoverPointUpdatePacket> : OutputPacket<GCLoverPointUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::LOVER_POINT_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCDigMotionPacket> : OutputPacket<GCDigMotionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DIG_MOTION);
        }
    };
    template <>
    struct GCOutputPacket<GCSpecificEffectPacket> : OutputPacket<GCSpecificEffectPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SPECIFIC_EFFECT);
        }
    };
    template <>
    struct GCOutputPacket<GCDragonSoulRefinePacket> : OutputPacket<GCDragonSoulRefinePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DRAGON_SOUL_REFINE);
        }
    };
    template <>
    struct GCOutputPacket<GCTeamlerStatusPacket> : OutputPacket<GCTeamlerStatusPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TEAMLER_STATUS);
        }
    };
    template <>
    struct GCOutputPacket<GCTeamlerShowPacket> : OutputPacket<GCTeamlerShowPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::TEAMLER_SHOW);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyPositionPacket> : OutputPacket<GCPartyPositionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_POSITION);
        }
    };
    template <>
    struct GCOutputPacket<GCWikiPacket> : OutputPacket<GCWikiPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::WIKI);
        }
    };
    template <>
    struct GCOutputPacket<GCWikiMobPacket> : OutputPacket<GCWikiMobPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::WIKI_MOB);
        }
    };
    template <>
    struct GCOutputPacket<GCSendCombatZonePacket> : OutputPacket<GCSendCombatZonePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SEND_COMBAT_ZONE);
        }
    };
    template <>
    struct GCOutputPacket<GCCombatZoneRankingDataPacket> : OutputPacket<GCCombatZoneRankingDataPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::COMBAT_ZONE_RANKING_DATA);
        }
    };
    template <>
    struct GCOutputPacket<GCPVPTeamPacket> : OutputPacket<GCPVPTeamPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PVP_TEAM);
        }
    };
    template <>
    struct GCOutputPacket<GCInventoryMaxNumPacket> : OutputPacket<GCInventoryMaxNumPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::INVENTORY_MAX_NUM);
        }
    };
    template <>
    struct GCOutputPacket<GCAttributesToClientPacket> : OutputPacket<GCAttributesToClientPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ATTRIBUTES_TO_CLIENT);
        }
    };
    template <>
    struct GCOutputPacket<GCAttrtreeLevelPacket> : OutputPacket<GCAttrtreeLevelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ATTRTREE_LEVEL);
        }
    };
    template <>
    struct GCOutputPacket<GCAttrtreeRefinePacket> : OutputPacket<GCAttrtreeRefinePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ATTRTREE_REFINE);
        }
    };
    template <>
    struct GCOutputPacket<GCRunePacket> : OutputPacket<GCRunePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::RUNE);
        }
    };
    template <>
    struct GCOutputPacket<GCRuneRefinePacket> : OutputPacket<GCRuneRefinePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::RUNE_REFINE);
        }
    };
    template <>
    struct GCOutputPacket<GCRunePagePacket> : OutputPacket<GCRunePagePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::RUNE_PAGE);
        }
    };
    template <>
    struct GCOutputPacket<GCRuneLevelupPacket> : OutputPacket<GCRuneLevelupPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::RUNE_LEVELUP);
        }
    };
    template <>
    struct GCOutputPacket<GCEquipmentPageLoadPacket> : OutputPacket<GCEquipmentPageLoadPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EQUIPMENT_PAGE_LOAD);
        }
    };
    template <>
    struct GCOutputPacket<GCDmgMeterPacket> : OutputPacket<GCDmgMeterPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::DMG_METER_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCSkillMotionPacket> : OutputPacket<GCSkillMotionPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SKILL_MOTION);
        }
    };
    template <>
    struct GCOutputPacket<GCFakeBuffSkillPacket> : OutputPacket<GCFakeBuffSkillPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FAKE_BUFF_SKILL);
        }
    };
    template <>
    struct GCOutputPacket<GCCBTItemSetPacket> : OutputPacket<GCCBTItemSetPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CBT_ITEM_SET);
        }
    };
    template <>
    struct GCOutputPacket<GCSoulRefineInfoPacket> : OutputPacket<GCSoulRefineInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SOUL_REFINE_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCHorseRefineInfoPacket> : OutputPacket<GCHorseRefineInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::HORSE_REFINE_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCHorseRefineResultPacket> : OutputPacket<GCHorseRefineResultPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::HORSE_REFINE_RESULT);
        }
    };
    template <>
    struct GCOutputPacket<GCGayaShopOpenPacket> : OutputPacket<GCGayaShopOpenPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GAYA_SHOP_OPEN);
        }
    };
    template <>
    struct GCOutputPacket<GCBattlepassDataPacket> : OutputPacket<GCBattlepassDataPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::BATTLEPASS_DATA);
        }
    };
    template <>
    struct GCOutputPacket<GCCrystalRefinePacket> : OutputPacket<GCCrystalRefinePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CRYSTAL_REFINE);
        }
    };
    template <>
    struct GCOutputPacket<GCCrystalUsingSlotPacket> : OutputPacket<GCCrystalUsingSlotPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CRYSTAL_USING_SLOT);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionOwnedGoldPacket> : OutputPacket<GCAuctionOwnedGoldPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_OWNED_GOLD);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionOwnedItemPacket> : OutputPacket<GCAuctionOwnedItemPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_OWNED_ITEM);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionSearchResultPacket> : OutputPacket<GCAuctionSearchResultPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SEARCH_RESULT);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionMessagePacket> : OutputPacket<GCAuctionMessagePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_MESSAGE);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopOwnedPacket> : OutputPacket<GCAuctionShopOwnedPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP_OWNED);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopPacket> : OutputPacket<GCAuctionShopPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopGoldPacket> : OutputPacket<GCAuctionShopGoldPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP_GOLD);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopTimeoutPacket> : OutputPacket<GCAuctionShopTimeoutPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP_TIMEOUT);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopHistoryPacket> : OutputPacket<GCAuctionShopHistoryPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP_HISTORY);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopGuestOpenPacket> : OutputPacket<GCAuctionShopGuestOpenPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP_GUEST_OPEN);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionShopGuestUpdatePacket> : OutputPacket<GCAuctionShopGuestUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_SHOP_GUEST_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCAuctionAveragePricePacket> : OutputPacket<GCAuctionAveragePricePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::AUCTION_AVERAGE_PRICE);
        }
    };
    template <>
    struct GCOutputPacket<GCEventRequestPacket> : OutputPacket<GCEventRequestPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EVENT_REQUEST);
        }
    };
    template <>
    struct GCOutputPacket<GCEventCancelPacket> : OutputPacket<GCEventCancelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EVENT_CANCEL);
        }
    };
    template <>
    struct GCOutputPacket<GCEventEmpireWarLoadPacket> : OutputPacket<GCEventEmpireWarLoadPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EVENT_EMPIRE_WAR_LOAD);
        }
    };
    template <>
    struct GCOutputPacket<GCEventEmpireWarUpdatePacket> : OutputPacket<GCEventEmpireWarUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EVENT_EMPIRE_WAR_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCSafeboxSizePacket> : OutputPacket<GCSafeboxSizePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SAFEBOX_SIZE);
        }
    };
    template <>
    struct GCOutputPacket<GCSafeboxMoneyChangePacket> : OutputPacket<GCSafeboxMoneyChangePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SAFEBOX_MONEY_CHANGE);
        }
    };
    template <>
    struct GCOutputPacket<GCMallOpenPacket> : OutputPacket<GCMallOpenPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MALL_OPEN);
        }
    };
    template <>
    struct GCOutputPacket<GCObserverAddPacket> : OutputPacket<GCObserverAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::OBSERVER_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCObserverRemovePacket> : OutputPacket<GCObserverRemovePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::OBSERVER_REMOVE);
        }
    };
    template <>
    struct GCOutputPacket<GCObserverMovePacket> : OutputPacket<GCObserverMovePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::OBSERVER_MOVE);
        }
    };
    template <>
    struct GCOutputPacket<GCQuickslotAddPacket> : OutputPacket<GCQuickslotAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::QUICKSLOT_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCQuickslotDelPacket> : OutputPacket<GCQuickslotDelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::QUICKSLOT_DEL);
        }
    };
    template <>
    struct GCOutputPacket<GCQuickslotSwapPacket> : OutputPacket<GCQuickslotSwapPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::QUICKSLOT_SWAP);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerListPacket> : OutputPacket<GCMessengerListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerLoginPacket> : OutputPacket<GCMessengerLoginPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_LOGIN);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerLogoutPacket> : OutputPacket<GCMessengerLogoutPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_LOGOUT);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerBlockListPacket> : OutputPacket<GCMessengerBlockListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_BLOCK_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerBlockLoginPacket> : OutputPacket<GCMessengerBlockLoginPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_BLOCK_LOGIN);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerBlockLogoutPacket> : OutputPacket<GCMessengerBlockLogoutPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_BLOCK_LOGOUT);
        }
    };
    template <>
    struct GCOutputPacket<GCMessengerMobilePacket> : OutputPacket<GCMessengerMobilePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MESSENGER_MOBILE);
        }
    };
    template <>
    struct GCOutputPacket<GCMainCharacterPacket> : OutputPacket<GCMainCharacterPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::MAIN_CHARACTER);
        }
    };
    template <>
    struct GCOutputPacket<GCCharacterAddPacket> : OutputPacket<GCCharacterAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHARACTER_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCCharacterAdditionalInfoPacket> : OutputPacket<GCCharacterAdditionalInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHARACTER_ADDITIONAL_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCCharacterUpdatePacket> : OutputPacket<GCCharacterUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHARACTER_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCCharacterShiningPacket> : OutputPacket<GCCharacterShiningPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHARACTER_SHINING);
        }
    };
    template <>
    struct GCOutputPacket<GCCharacterDeletePacket> : OutputPacket<GCCharacterDeletePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::CHARACTER_DELETE);
        }
    };
    template <>
    struct GCOutputPacket<GCFishingStartPacket> : OutputPacket<GCFishingStartPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FISHING_START);
        }
    };
    template <>
    struct GCOutputPacket<GCFishingStopPacket> : OutputPacket<GCFishingStopPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FISHING_STOP);
        }
    };
    template <>
    struct GCOutputPacket<GCFishingReactPacket> : OutputPacket<GCFishingReactPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FISHING_REACT);
        }
    };
    template <>
    struct GCOutputPacket<GCFishingSuccessPacket> : OutputPacket<GCFishingSuccessPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FISHING_SUCCESS);
        }
    };
    template <>
    struct GCOutputPacket<GCFishingFailPacket> : OutputPacket<GCFishingFailPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FISHING_FAIL);
        }
    };
    template <>
    struct GCOutputPacket<GCFishingFishInfoPacket> : OutputPacket<GCFishingFishInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::FISHING_FISH_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCItemSetPacket> : OutputPacket<GCItemSetPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ITEM_SET);
        }
    };
    template <>
    struct GCOutputPacket<GCItemUpdatePacket> : OutputPacket<GCItemUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ITEM_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCItemGroundAddPacket> : OutputPacket<GCItemGroundAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ITEM_GROUND_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCItemGroundDeletePacket> : OutputPacket<GCItemGroundDeletePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ITEM_GROUND_DELETE);
        }
    };
    template <>
    struct GCOutputPacket<GCItemOwnershipPacket> : OutputPacket<GCItemOwnershipPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::ITEM_OWNERSHIP);
        }
    };
    template <>
    struct GCOutputPacket<GCExchangeStartPacket> : OutputPacket<GCExchangeStartPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EXCHANGE_START);
        }
    };
    template <>
    struct GCOutputPacket<GCExchangeItemAddPacket> : OutputPacket<GCExchangeItemAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EXCHANGE_ITEM_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCExchangeItemDelPacket> : OutputPacket<GCExchangeItemDelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EXCHANGE_ITEM_DEL);
        }
    };
    template <>
    struct GCOutputPacket<GCExchangeGoldAddPacket> : OutputPacket<GCExchangeGoldAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EXCHANGE_GOLD_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCExchangeAcceptPacket> : OutputPacket<GCExchangeAcceptPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::EXCHANGE_ACCEPT);
        }
    };
    template <>
    struct GCOutputPacket<GCShopStartPacket> : OutputPacket<GCShopStartPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SHOP_START);
        }
    };
    template <>
    struct GCOutputPacket<GCShopExStartPacket> : OutputPacket<GCShopExStartPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SHOP_EX_START);
        }
    };
    template <>
    struct GCOutputPacket<GCShopUpdateItemPacket> : OutputPacket<GCShopUpdateItemPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SHOP_UPDATE_ITEM);
        }
    };
    template <>
    struct GCOutputPacket<GCShopSignPacket> : OutputPacket<GCShopSignPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SHOP_SIGN);
        }
    };
    template <>
    struct GCOutputPacket<GCQuestInfoPacket> : OutputPacket<GCQuestInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::QUEST_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCScriptPacket> : OutputPacket<GCScriptPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::SCRIPT);
        }
    };
    template <>
    struct GCOutputPacket<GCQuestConfirmPacket> : OutputPacket<GCQuestConfirmPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::QUEST_CONFIRM);
        }
    };
    template <>
    struct GCOutputPacket<GCQuestCooldownPacket> : OutputPacket<GCQuestCooldownPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::QUEST_COOLDOWN);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyInvitePacket> : OutputPacket<GCPartyInvitePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_INVITE);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyAddPacket> : OutputPacket<GCPartyAddPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_ADD);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyUpdatePacket> : OutputPacket<GCPartyUpdatePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_UPDATE);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyRemovePacket> : OutputPacket<GCPartyRemovePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_REMOVE);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyLinkPacket> : OutputPacket<GCPartyLinkPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_LINK);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyUnlinkPacket> : OutputPacket<GCPartyUnlinkPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_UNLINK);
        }
    };
    template <>
    struct GCOutputPacket<GCPartyParameterPacket> : OutputPacket<GCPartyParameterPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PARTY_PARAMETER);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildLoginPacket> : OutputPacket<GCGuildLoginPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_LOGIN);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildLogoutPacket> : OutputPacket<GCGuildLogoutPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_LOGOUT);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildRemovePacket> : OutputPacket<GCGuildRemovePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_REMOVE);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildMemberListPacket> : OutputPacket<GCGuildMemberListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_MEMBER_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildGradePacket> : OutputPacket<GCGuildGradePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_GRADE);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildGradeNamePacket> : OutputPacket<GCGuildGradeNamePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_GRADE_NAME);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildGradeAuthPacket> : OutputPacket<GCGuildGradeAuthPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_GRADE_AUTH);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildInfoPacket> : OutputPacket<GCGuildInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildCommentsPacket> : OutputPacket<GCGuildCommentsPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_COMMENTS);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildChangeExpPacket> : OutputPacket<GCGuildChangeExpPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_CHANGE_EXP);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildChangeMemberGradePacket> : OutputPacket<GCGuildChangeMemberGradePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_CHANGE_MEMBER_GRADE);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildSkillInfoPacket> : OutputPacket<GCGuildSkillInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_SKILL_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildChangeMemberGeneralPacket> : OutputPacket<GCGuildChangeMemberGeneralPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_CHANGE_MEMBER_GENERAL);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildInvitePacket> : OutputPacket<GCGuildInvitePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_INVITE);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildMemberLastPlayedPacket> : OutputPacket<GCGuildMemberLastPlayedPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_MEMBER_LAST_PLAYED);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildBattleStatsPacket> : OutputPacket<GCGuildBattleStatsPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_BATTLE_STATS);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildWarPacket> : OutputPacket<GCGuildWarPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_WAR);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildNamePacket> : OutputPacket<GCGuildNamePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_NAME);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildWarListPacket> : OutputPacket<GCGuildWarListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_WAR_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildWarEndListPacket> : OutputPacket<GCGuildWarEndListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_WAR_END_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildWarPointPacket> : OutputPacket<GCGuildWarPointPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_WAR_POINT);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildMoneyChangePacket> : OutputPacket<GCGuildMoneyChangePacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_MONEY_CHANGE);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildLadderListPacket> : OutputPacket<GCGuildLadderListPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_LADDER_LIST);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildLadderSearchResultPacket> : OutputPacket<GCGuildLadderSearchResultPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_LADDER_SEARCH_RESULT);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildRankAndPointPacket> : OutputPacket<GCGuildRankAndPointPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_RANK_AND_POINT);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildSafeboxOpenPacket> : OutputPacket<GCGuildSafeboxOpenPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_SAFEBOX_OPEN);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildSafeboxGoldPacket> : OutputPacket<GCGuildSafeboxGoldPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_SAFEBOX_GOLD);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildSafeboxLoadLogPacket> : OutputPacket<GCGuildSafeboxLoadLogPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_SAFEBOX_LOAD_LOG);
        }
    };
    template <>
    struct GCOutputPacket<GCGuildSafeboxAppendLogPacket> : OutputPacket<GCGuildSafeboxAppendLogPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::GUILD_SAFEBOX_APPEND_LOG);
        }
    };
    template <>
    struct GCOutputPacket<GCPetSummonPacket> : OutputPacket<GCPetSummonPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_SUMMON);
        }
    };
    template <>
    struct GCOutputPacket<GCPetUpdateExpPacket> : OutputPacket<GCPetUpdateExpPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_UPDATE_EXP);
        }
    };
    template <>
    struct GCOutputPacket<GCPetUpdateLevelPacket> : OutputPacket<GCPetUpdateLevelPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_UPDATE_LEVEL);
        }
    };
    template <>
    struct GCOutputPacket<GCPetUpdateSkillPacket> : OutputPacket<GCPetUpdateSkillPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_UPDATE_SKILL);
        }
    };
    template <>
    struct GCOutputPacket<GCPetUpdateAttrPacket> : OutputPacket<GCPetUpdateAttrPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_UPDATE_ATTR);
        }
    };
    template <>
    struct GCOutputPacket<GCPetUpdateSkillpowerPacket> : OutputPacket<GCPetUpdateSkillpowerPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_UPDATE_SKILLPOWER);
        }
    };
    template <>
    struct GCOutputPacket<GCPetEvolutionInfoPacket> : OutputPacket<GCPetEvolutionInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_EVOLUTION_INFO);
        }
    };
    template <>
    struct GCOutputPacket<GCPetEvolveResultPacket> : OutputPacket<GCPetEvolveResultPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_EVOLVE_RESULT);
        }
    };
    template <>
    struct GCOutputPacket<GCPetAttrRefineInfoPacket> : OutputPacket<GCPetAttrRefineInfoPacket> {
        GCOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGCHeader::PET_ATTR_REFINE_INFO);
        }
    };
}
