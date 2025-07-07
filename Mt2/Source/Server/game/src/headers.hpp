#pragma once

#include "headers_basic.hpp"
#include "protobuf_cg_packets.h"
#include "protobuf_dg_packets.h"
#include "protobuf_gg_packets.h"
#include "protobuf_gd_packets.h"
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

    template <typename T>
    struct GGOutputPacket : OutputPacket<T> {
        GGOutputPacket()
        {
            throw std::invalid_argument("not handled packet type");
        }
    };
    template <>
    struct GGOutputPacket<GGSetupPacket> : OutputPacket<GGSetupPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::SETUP);
        }
    };
    template <>
    struct GGOutputPacket<GGLoginPacket> : OutputPacket<GGLoginPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::LOGIN);
        }
    };
    template <>
    struct GGOutputPacket<GGLogoutPacket> : OutputPacket<GGLogoutPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::LOGOUT);
        }
    };
    template <>
    struct GGOutputPacket<GGRelayPacket> : OutputPacket<GGRelayPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::RELAY);
        }
    };
    template <>
    struct GGOutputPacket<GGPlayerPacket> : OutputPacket<GGPlayerPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::PLAYER_PACKET);
        }
    };
    template <>
    struct GGOutputPacket<GGDisconnectPacket> : OutputPacket<GGDisconnectPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::DISCONNECT);
        }
    };
    template <>
    struct GGOutputPacket<GGLoginPingPacket> : OutputPacket<GGLoginPingPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::LOGIN_PING);
        }
    };
    template <>
    struct GGOutputPacket<GGGiveItemPacket> : OutputPacket<GGGiveItemPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::GIVE_ITEM);
        }
    };
    template <>
    struct GGOutputPacket<GGGiveGoldPacket> : OutputPacket<GGGiveGoldPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::GIVE_GOLD);
        }
    };
    template <>
    struct GGOutputPacket<GGShoutPacket> : OutputPacket<GGShoutPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::SHOUT);
        }
    };
    template <>
    struct GGOutputPacket<GGNoticePacket> : OutputPacket<GGNoticePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::NOTICE);
        }
    };
    template <>
    struct GGOutputPacket<GGMessengerRequestPacket> : OutputPacket<GGMessengerRequestPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::MESSENGER_REQUEST);
        }
    };
    template <>
    struct GGOutputPacket<GGMessengerRequestFailPacket> : OutputPacket<GGMessengerRequestFailPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::MESSENGER_REQUEST_FAIL);
        }
    };
    template <>
    struct GGOutputPacket<GGMessengerAddPacket> : OutputPacket<GGMessengerAddPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::MESSENGER_ADD);
        }
    };
    template <>
    struct GGOutputPacket<GGMessengerRemovePacket> : OutputPacket<GGMessengerRemovePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::MESSENGER_REMOVE);
        }
    };
    template <>
    struct GGOutputPacket<GGMessengerBlockAddPacket> : OutputPacket<GGMessengerBlockAddPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::MESSENGER_BLOCK_ADD);
        }
    };
    template <>
    struct GGOutputPacket<GGMessengerBlockRemovePacket> : OutputPacket<GGMessengerBlockRemovePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::MESSENGER_BLOCK_REMOVE);
        }
    };
    template <>
    struct GGOutputPacket<GGFindPositionPacket> : OutputPacket<GGFindPositionPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::FIND_POSITION);
        }
    };
    template <>
    struct GGOutputPacket<GGWarpCharacterPacket> : OutputPacket<GGWarpCharacterPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::WARP_CHARACTER);
        }
    };
    template <>
    struct GGOutputPacket<GGXmasWarpSantaPacket> : OutputPacket<GGXmasWarpSantaPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::XMAS_WARP_SANTA);
        }
    };
    template <>
    struct GGOutputPacket<GGXmasWarpSantaReplyPacket> : OutputPacket<GGXmasWarpSantaReplyPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::XMAS_WARP_SANTA_REPLY);
        }
    };
    template <>
    struct GGOutputPacket<GGBlockChatPacket> : OutputPacket<GGBlockChatPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::BLOCK_CHAT);
        }
    };
    template <>
    struct GGOutputPacket<GGCastleSiegePacket> : OutputPacket<GGCastleSiegePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::CASTLE_SIEGE);
        }
    };
    template <>
    struct GGOutputPacket<GGPCBangUpdatePacket> : OutputPacket<GGPCBangUpdatePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::PCBANG_UPDATE);
        }
    };
    template <>
    struct GGOutputPacket<GGWhisperManagerAddPacket> : OutputPacket<GGWhisperManagerAddPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::WHISPER_MANAGER_ADD);
        }
    };
    template <>
    struct GGOutputPacket<GGTeamlerStatusPacket> : OutputPacket<GGTeamlerStatusPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::TEAMLER_STATUS);
        }
    };
    template <>
    struct GGOutputPacket<GGRequestDungeonWarpPacket> : OutputPacket<GGRequestDungeonWarpPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::REQUEST_DUNGEON_WARP);
        }
    };
    template <>
    struct GGOutputPacket<GGAnswerDungeonWarpPacket> : OutputPacket<GGAnswerDungeonWarpPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::ANSWER_DUNGEON_WARP);
        }
    };
    template <>
    struct GGOutputPacket<GGDungeonSetFlagPacket> : OutputPacket<GGDungeonSetFlagPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::DUNGEON_SET_FLAG);
        }
    };
    template <>
    struct GGOutputPacket<GGExecReloadCommandPacket> : OutputPacket<GGExecReloadCommandPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EXEC_RELOAD_COMMAND);
        }
    };
    template <>
    struct GGOutputPacket<GGRecvShutdownPacket> : OutputPacket<GGRecvShutdownPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::RECV_SHUTDOWN);
        }
    };
    template <>
    struct GGOutputPacket<GGTransferPacket> : OutputPacket<GGTransferPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::TRANSFER);
        }
    };
    template <>
    struct GGOutputPacket<GGForceItemDeletePacket> : OutputPacket<GGForceItemDeletePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::FORCE_ITEM_DELETE);
        }
    };
    template <>
    struct GGOutputPacket<GGTeamChatPacket> : OutputPacket<GGTeamChatPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::TEAM_CHAT);
        }
    };
    template <>
    struct GGOutputPacket<GGCombatZoneRankingPacket> : OutputPacket<GGCombatZoneRankingPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::COMBAT_ZONE_RANKING);
        }
    };
    template <>
    struct GGOutputPacket<GGSuccessNoticePacket> : OutputPacket<GGSuccessNoticePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::SUCCESS_NOTICE);
        }
    };
    template <>
    struct GGOutputPacket<GGUpdateRightsPacket> : OutputPacket<GGUpdateRightsPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::UPDATE_RIGHTS);
        }
    };
    template <>
    struct GGOutputPacket<GGFlushPlayerPacket> : OutputPacket<GGFlushPlayerPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::FLUSH_PLAYER);
        }
    };
    template <>
    struct GGOutputPacket<GGHomepageCommandPacket> : OutputPacket<GGHomepageCommandPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::HOMEPAGE_COMMAND);
        }
    };
    template <>
    struct GGOutputPacket<GGPullOfflineMessagesPacket> : OutputPacket<GGPullOfflineMessagesPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::PULL_OFFLINE_MESSAGES);
        }
    };
    template <>
    struct GGOutputPacket<GGDmgRankingUpdatePacket> : OutputPacket<GGDmgRankingUpdatePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::DMG_RANKING_UPDATE);
        }
    };
    template <>
    struct GGOutputPacket<GGLocaleUpdateLastUsagePacket> : OutputPacket<GGLocaleUpdateLastUsagePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::LOCALE_UPDATE_LAST_USAGE);
        }
    };
    template <>
    struct GGOutputPacket<GGReloadCommandPacket> : OutputPacket<GGReloadCommandPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::RELOAD_COMMAND);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionInsertItemPacket> : OutputPacket<GGAuctionInsertItemPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_INSERT_ITEM);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionTakeItemPacket> : OutputPacket<GGAuctionTakeItemPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_TAKE_ITEM);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionBuyItemPacket> : OutputPacket<GGAuctionBuyItemPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_BUY_ITEM);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionTakeGoldPacket> : OutputPacket<GGAuctionTakeGoldPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_TAKE_GOLD);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionSearchItemsPacket> : OutputPacket<GGAuctionSearchItemsPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SEARCH_ITEMS);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionExtendedSearchItemsPacket> : OutputPacket<GGAuctionExtendedSearchItemsPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_EXTENDED_SEARCH_ITEMS);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionMarkShopPacket> : OutputPacket<GGAuctionMarkShopPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_MARK_SHOP);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionAnswerMarkShopPacket> : OutputPacket<GGAuctionAnswerMarkShopPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_ANSWER_MARK_SHOP);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopRequestShowPacket> : OutputPacket<GGAuctionShopRequestShowPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_REQUEST_SHOW);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopOpenPacket> : OutputPacket<GGAuctionShopOpenPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_OPEN);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopTakeGoldPacket> : OutputPacket<GGAuctionShopTakeGoldPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_TAKE_GOLD);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopSpawnPacket> : OutputPacket<GGAuctionShopSpawnPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_SPAWN);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopDespawnPacket> : OutputPacket<GGAuctionShopDespawnPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_DESPAWN);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopViewPacket> : OutputPacket<GGAuctionShopViewPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_VIEW);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopViewCancelPacket> : OutputPacket<GGAuctionShopViewCancelPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_VIEW_CANCEL);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopRequestHistoryPacket> : OutputPacket<GGAuctionShopRequestHistoryPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_REQUEST_HISTORY);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopRenewPacket> : OutputPacket<GGAuctionShopRenewPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_RENEW);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionShopClosePacket> : OutputPacket<GGAuctionShopClosePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_SHOP_CLOSE);
        }
    };
    template <>
    struct GGOutputPacket<GGAuctionRequestAveragePricePacket> : OutputPacket<GGAuctionRequestAveragePricePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::AUCTION_REQUEST_AVG_PRICE);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerOpenRegistrationPacket> : OutputPacket<GGEventManagerOpenRegistrationPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_OPEN_REGISTRATION);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerCloseRegistrationPacket> : OutputPacket<GGEventManagerCloseRegistrationPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_CLOSE_REGISTRATION);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerIgnorePlayerPacket> : OutputPacket<GGEventManagerIgnorePlayerPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_IGNORE_PLAYER);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerOpenAnnouncementPacket> : OutputPacket<GGEventManagerOpenAnnouncementPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_OPEN_ANNOUNCEMENT);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerTagTeamRegisterPacket> : OutputPacket<GGEventManagerTagTeamRegisterPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_TAG_TEAM_REGISTER);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerTagTeamUnregisterPacket> : OutputPacket<GGEventManagerTagTeamUnregisterPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_TAG_TEAM_UNREGISTER);
        }
    };
    template <>
    struct GGOutputPacket<GGEventManagerTagTeamCreatePacket> : OutputPacket<GGEventManagerTagTeamCreatePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::EVENT_MANAGER_TAG_TEAM_CREATE);
        }
    };
    template <>
    struct GGOutputPacket<GGGuildChatPacket> : OutputPacket<GGGuildChatPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::GUILD_CHAT);
        }
    };
    template <>
    struct GGOutputPacket<GGGuildSetMemberCountBonusPacket> : OutputPacket<GGGuildSetMemberCountBonusPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::GUILD_SET_MEMBER_COUNT_BONUS);
        }
    };
    template <>
    struct GGOutputPacket<GGGuildWarZoneMapIndexPacket> : OutputPacket<GGGuildWarZoneMapIndexPacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::GUILD_WAR_ZONE_MAP_INDEX);
        }
    };
    template <>
    struct GGOutputPacket<GGGuildChangeNamePacket> : OutputPacket<GGGuildChangeNamePacket> {
        GGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGGHeader::GUILD_CHANGE_NAME);
        }
    };

    template <typename T>
    struct GDOutputPacket : OutputPacket<T> {
        GDOutputPacket()
        {
            throw std::invalid_argument("not handled packet type");
        }
    };
    template <>
    struct GDOutputPacket<GDBootPacket> : OutputPacket<GDBootPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::BOOT);
        }
    };
    template <>
    struct GDOutputPacket<GDSetupPacket> : OutputPacket<GDSetupPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SETUP);
        }
    };
    template <>
    struct GDOutputPacket<GDUpdateChannelStatusPacket> : OutputPacket<GDUpdateChannelStatusPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::UPDATE_CHANNELSTATUS);
        }
    };
    template <>
    struct GDOutputPacket<GDBlockExceptionPacket> : OutputPacket<GDBlockExceptionPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::BLOCK_EXCEPTION);
        }
    };
    template <>
    struct GDOutputPacket<GDPlayerCountPacket> : OutputPacket<GDPlayerCountPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PLAYER_COUNT);
        }
    };
    template <>
    struct GDOutputPacket<GDReloadAdminPacket> : OutputPacket<GDReloadAdminPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::RELOAD_ADMIN);
        }
    };
    template <>
    struct GDOutputPacket<GDAuthLoginPacket> : OutputPacket<GDAuthLoginPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::AUTH_LOGIN);
        }
    };
    template <>
    struct GDOutputPacket<GDLoginByKeyPacket> : OutputPacket<GDLoginByKeyPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::LOGIN_BY_KEY);
        }
    };
    template <>
    struct GDOutputPacket<GDPlayerCreatePacket> : OutputPacket<GDPlayerCreatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PLAYER_CREATE);
        }
    };
    template <>
    struct GDOutputPacket<GDPlayerDeletePacket> : OutputPacket<GDPlayerDeletePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PLAYER_DELETE);
        }
    };
    template <>
    struct GDOutputPacket<GDChangeNamePacket> : OutputPacket<GDChangeNamePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::CHANGE_NAME);
        }
    };
    template <>
    struct GDOutputPacket<GDEmpireSelectPacket> : OutputPacket<GDEmpireSelectPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::EMPIRE_SELECT);
        }
    };
    template <>
    struct GDOutputPacket<GDBillingExpirePacket> : OutputPacket<GDBillingExpirePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::BILLING_EXPIRE);
        }
    };
    template <>
    struct GDOutputPacket<GDBillingCheckPacket> : OutputPacket<GDBillingCheckPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::BILLING_CHECK);
        }
    };
    template <>
    struct GDOutputPacket<GDDisconnectPacket> : OutputPacket<GDDisconnectPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::DISCONNECT);
        }
    };
    template <>
    struct GDOutputPacket<GDValidLogoutPacket> : OutputPacket<GDValidLogoutPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::VALID_LOGOUT);
        }
    };
    template <>
    struct GDOutputPacket<GDPlayerLoadPacket> : OutputPacket<GDPlayerLoadPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PLAYER_LOAD);
        }
    };
    template <>
    struct GDOutputPacket<GDSafeboxLoadPacket> : OutputPacket<GDSafeboxLoadPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SAFEBOX_LOAD);
        }
    };
    template <>
    struct GDOutputPacket<GDReqHorseNamePacket> : OutputPacket<GDReqHorseNamePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::REQ_HORSE_NAME);
        }
    };
    template <>
    struct GDOutputPacket<GDPlayerSavePacket> : OutputPacket<GDPlayerSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PLAYER_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDItemSavePacket> : OutputPacket<GDItemSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::ITEM_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDQuestSavePacket> : OutputPacket<GDQuestSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::QUEST_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDPetSavePacket> : OutputPacket<GDPetSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PET_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDSafeboxSavePacket> : OutputPacket<GDSafeboxSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SAFEBOX_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDItemDestroyPacket> : OutputPacket<GDItemDestroyPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::ITEM_DESTROY);
        }
    };
    template <>
    struct GDOutputPacket<GDSkillColorSavePacket> : OutputPacket<GDSkillColorSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SKILL_COLOR_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDPlayerRuneSavePacket> : OutputPacket<GDPlayerRuneSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PLAYER_RUNE_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDFlushCachePacket> : OutputPacket<GDFlushCachePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::FLUSH_CACHE);
        }
    };
    template <>
    struct GDOutputPacket<GDItemFlushPacket> : OutputPacket<GDItemFlushPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::ITEM_FLUSH);
        }
    };
    template <>
    struct GDOutputPacket<GDLogoutPacket> : OutputPacket<GDLogoutPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::LOGOUT);
        }
    };
    template <>
    struct GDOutputPacket<GDSafeboxChangeSizePacket> : OutputPacket<GDSafeboxChangeSizePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SAFEBOX_CHANGE_SIZE);
        }
    };
    template <>
    struct GDOutputPacket<GDSafeboxChangePasswordPacket> : OutputPacket<GDSafeboxChangePasswordPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SAFEBOX_CHANGE_PASSWORD);
        }
    };
    template <>
    struct GDOutputPacket<GDAddAffectPacket> : OutputPacket<GDAddAffectPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::ADD_AFFECT);
        }
    };
    template <>
    struct GDOutputPacket<GDRemoveAffectPacket> : OutputPacket<GDRemoveAffectPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::REMOVE_AFFECT);
        }
    };
    template <>
    struct GDOutputPacket<GDHighscoreRegisterPacket> : OutputPacket<GDHighscoreRegisterPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::HIGHSCORE_REGISTER);
        }
    };
    template <>
    struct GDOutputPacket<GDSMSPacket> : OutputPacket<GDSMSPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SMS);
        }
    };
    template <>
    struct GDOutputPacket<GDRequestGuildPrivPacket> : OutputPacket<GDRequestGuildPrivPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::REQUEST_GUILD_PRIV);
        }
    };
    template <>
    struct GDOutputPacket<GDRequestEmpirePrivPacket> : OutputPacket<GDRequestEmpirePrivPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::REQUEST_EMPIRE_PRIV);
        }
    };
    template <>
    struct GDOutputPacket<GDRequestCharacterPrivPacket> : OutputPacket<GDRequestCharacterPrivPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::REQUEST_CHARACTER_PRIV);
        }
    };
    template <>
    struct GDOutputPacket<GDMoneyLogPacket> : OutputPacket<GDMoneyLogPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MONEY_LOG);
        }
    };
    template <>
    struct GDOutputPacket<GDSetEventFlagPacket> : OutputPacket<GDSetEventFlagPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SET_EVENT_FLAG);
        }
    };
    template <>
    struct GDOutputPacket<GDCreateObjectPacket> : OutputPacket<GDCreateObjectPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::CREATE_OBJECT);
        }
    };
    template <>
    struct GDOutputPacket<GDDeleteObjectPacket> : OutputPacket<GDDeleteObjectPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::DELETE_OBJECT);
        }
    };
    template <>
    struct GDOutputPacket<GDUpdateLandPacket> : OutputPacket<GDUpdateLandPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::UPDATE_LAND);
        }
    };
    template <>
    struct GDOutputPacket<GDVCardPacket> : OutputPacket<GDVCardPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::VCARD);
        }
    };
    template <>
    struct GDOutputPacket<GDBlockChatPacket> : OutputPacket<GDBlockChatPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::BLOCK_CHAT);
        }
    };
    template <>
    struct GDOutputPacket<GDMyShopPricelistUpdatePacket> : OutputPacket<GDMyShopPricelistUpdatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MYSHOP_PRICELIST_UPDATE);
        }
    };
    template <>
    struct GDOutputPacket<GDMyShopPricelistRequestPacket> : OutputPacket<GDMyShopPricelistRequestPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MYSHOP_PRICELIST_REQUEST);
        }
    };
    template <>
    struct GDOutputPacket<GDUpdateHorseNamePacket> : OutputPacket<GDUpdateHorseNamePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::UPDATE_HORSE_NAME);
        }
    };
    template <>
    struct GDOutputPacket<GDRequestChargeCashPacket> : OutputPacket<GDRequestChargeCashPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::REQUEST_CHARGE_CASH);
        }
    };
    template <>
    struct GDOutputPacket<GDDeleteAwardIDPacket> : OutputPacket<GDDeleteAwardIDPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::DELETE_AWARDID);
        }
    };
    template <>
    struct GDOutputPacket<GDChannelSwitchPacket> : OutputPacket<GDChannelSwitchPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::CHANNEL_SWITCH);
        }
    };
    template <>
    struct GDOutputPacket<GDSpawnMobTimedPacket> : OutputPacket<GDSpawnMobTimedPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SPAWN_MOB_TIMED);
        }
    };
    template <>
    struct GDOutputPacket<GDForceItemDeletePacket> : OutputPacket<GDForceItemDeletePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::FORCE_ITEM_DELETE);
        }
    };
    template <>
    struct GDOutputPacket<GDCombatZoneSkillsCachePacket> : OutputPacket<GDCombatZoneSkillsCachePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::COMBAT_ZONE_SKILLS_CACHE);
        }
    };
    template <>
    struct GDOutputPacket<GDWhisperPlayerExistCheckPacket> : OutputPacket<GDWhisperPlayerExistCheckPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::WHISPER_PLAYER_EXIST_CHECK);
        }
    };
    template <>
    struct GDOutputPacket<GDWhisperPlayerMessageOfflinePacket> : OutputPacket<GDWhisperPlayerMessageOfflinePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::WHISPER_PLAYER_MESSAGE_OFFLINE);
        }
    };
    template <>
    struct GDOutputPacket<GDEquipmentPageDeletePacket> : OutputPacket<GDEquipmentPageDeletePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::EQUIPMENT_PAGE_DELETE);
        }
    };
    template <>
    struct GDOutputPacket<GDEquipmentPageSavePacket> : OutputPacket<GDEquipmentPageSavePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::EQUIPMENT_PAGE_SAVE);
        }
    };
    template <>
    struct GDOutputPacket<GDRecvShutdownPacket> : OutputPacket<GDRecvShutdownPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::RECV_SHUTDOWN);
        }
    };
    template <>
    struct GDOutputPacket<GDLoadItemRefundPacket> : OutputPacket<GDLoadItemRefundPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::LOAD_ITEM_REFUND);
        }
    };
    template <>
    struct GDOutputPacket<GDItemDestroyLogPacket> : OutputPacket<GDItemDestroyLogPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::ITEM_DESTROY_LOG);
        }
    };
    template <>
    struct GDOutputPacket<GDSelectUpdateHairPacket> : OutputPacket<GDSelectUpdateHairPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::SELECT_UPDATE_HAIR);
        }
    };
    template <>
    struct GDOutputPacket<GDItemTimedIgnorePacket> : OutputPacket<GDItemTimedIgnorePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::ITEM_TIMED_IGNORE);
        }
    };
    template <>
    struct GDOutputPacket<GDMarriageAddPacket> : OutputPacket<GDMarriageAddPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MARRIAGE_ADD);
        }
    };
    template <>
    struct GDOutputPacket<GDMarriageUpdatePacket> : OutputPacket<GDMarriageUpdatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MARRIAGE_UPDATE);
        }
    };
    template <>
    struct GDOutputPacket<GDMarriageRemovePacket> : OutputPacket<GDMarriageRemovePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MARRIAGE_REMOVE);
        }
    };
    template <>
    struct GDOutputPacket<GDMarriageBreakPacket> : OutputPacket<GDMarriageBreakPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::MARRIAGE_BREAK);
        }
    };
    template <>
    struct GDOutputPacket<GDWeddingRequestPacket> : OutputPacket<GDWeddingRequestPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::WEDDING_REQUEST);
        }
    };
    template <>
    struct GDOutputPacket<GDWeddingReadyPacket> : OutputPacket<GDWeddingReadyPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::WEDDING_READY);
        }
    };
    template <>
    struct GDOutputPacket<GDWeddingEndPacket> : OutputPacket<GDWeddingEndPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::WEDDING_END);
        }
    };
    template <>
    struct GDOutputPacket<GDPartyCreatePacket> : OutputPacket<GDPartyCreatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PARTY_CREATE);
        }
    };
    template <>
    struct GDOutputPacket<GDPartyDeletePacket> : OutputPacket<GDPartyDeletePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PARTY_DELETE);
        }
    };
    template <>
    struct GDOutputPacket<GDPartyAddPacket> : OutputPacket<GDPartyAddPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PARTY_ADD);
        }
    };
    template <>
    struct GDOutputPacket<GDPartyRemovePacket> : OutputPacket<GDPartyRemovePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PARTY_REMOVE);
        }
    };
    template <>
    struct GDOutputPacket<GDPartyStateChangePacket> : OutputPacket<GDPartyStateChangePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PARTY_STATE_CHANGE);
        }
    };
    template <>
    struct GDOutputPacket<GDPartySetMemberLevelPacket> : OutputPacket<GDPartySetMemberLevelPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::PARTY_SET_MEMBER_LEVEL);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildCreatePacket> : OutputPacket<GDGuildCreatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_CREATE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSkillUpdatePacket> : OutputPacket<GDGuildSkillUpdatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SKILL_UPDATE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildExpUpdatePacket> : OutputPacket<GDGuildExpUpdatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_EXP_UPDATE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildAddMemberPacket> : OutputPacket<GDGuildAddMemberPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_ADD_MEMBER);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildRemoveMemberPacket> : OutputPacket<GDGuildRemoveMemberPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_REMOVE_MEMBER);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildChangeGradePacket> : OutputPacket<GDGuildChangeGradePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_CHANGE_GRADE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildChangeMemberDataPacket> : OutputPacket<GDGuildChangeMemberDataPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_CHANGE_MEMBER_DATA);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildDisbandPacket> : OutputPacket<GDGuildDisbandPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_DISBAND);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildDungeonPacket> : OutputPacket<GDGuildDungeonPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_DUNGEON);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildDungeonCDPacket> : OutputPacket<GDGuildDungeonCDPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_DUNGEON_CD);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildWarPacket> : OutputPacket<GDGuildWarPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_WAR);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildWarScorePacket> : OutputPacket<GDGuildWarScorePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_WAR_SCORE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildChangeLadderPointPacket> : OutputPacket<GDGuildChangeLadderPointPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_CHANGE_LADDER_POINT);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildUseSkillPacket> : OutputPacket<GDGuildUseSkillPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_USE_SKILL);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildDepositMoneyPacket> : OutputPacket<GDGuildDepositMoneyPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_DEPOSIT_MONEY);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildWithdrawMoneyPacket> : OutputPacket<GDGuildWithdrawMoneyPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_WITHDRAW_MONEY);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildWithdrawMoneyGiveReplyPacket> : OutputPacket<GDGuildWithdrawMoneyGiveReplyPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_WITHDRAW_MONEY_GIVE_REPLY);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildWarBetPacket> : OutputPacket<GDGuildWarBetPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_WAR_BET);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildReqChangeMasterPacket> : OutputPacket<GDGuildReqChangeMasterPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_REQ_CHANGE_MASTER);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxLoadPacket> : OutputPacket<GDGuildSafeboxLoadPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_LOAD);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxCreatePacket> : OutputPacket<GDGuildSafeboxCreatePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_CREATE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxSizePacket> : OutputPacket<GDGuildSafeboxSizePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_SIZE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxAddPacket> : OutputPacket<GDGuildSafeboxAddPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_ADD);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxTakePacket> : OutputPacket<GDGuildSafeboxTakePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_TAKE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxMovePacket> : OutputPacket<GDGuildSafeboxMovePacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_MOVE);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxGiveGoldPacket> : OutputPacket<GDGuildSafeboxGiveGoldPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_GIVE_GOLD);
        }
    };
    template <>
    struct GDOutputPacket<GDGuildSafeboxGetGoldPacket> : OutputPacket<GDGuildSafeboxGetGoldPacket> {
        GDOutputPacket()
        {
            header.header = static_cast<uint16_t>(TGDHeader::GUILD_SAFEBOX_GET_GOLD);
        }
    };

    template <typename T>
    struct DGOutputPacket : OutputPacket<T> {
        DGOutputPacket()
        {
            throw std::invalid_argument("not handled packet type");
        }
    };
    template <>
    struct DGOutputPacket<DGBootPacket> : OutputPacket<DGBootPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::BOOT);
        }
    };
    template <>
    struct DGOutputPacket<DGMapLocationsPacket> : OutputPacket<DGMapLocationsPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MAP_LOCATIONS);
        }
    };
    template <>
    struct DGOutputPacket<DGLoginSuccessPacket> : OutputPacket<DGLoginSuccessPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::LOGIN_SUCCESS);
        }
    };
    template <>
    struct DGOutputPacket<DGPlayerCreateSuccessPacket> : OutputPacket<DGPlayerCreateSuccessPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PLAYER_CREATE_SUCCESS);
        }
    };
    template <>
    struct DGOutputPacket<DGPlayerDeleteSuccessPacket> : OutputPacket<DGPlayerDeleteSuccessPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PLAYER_DELETE_SUCCESS);
        }
    };
    template <>
    struct DGOutputPacket<DGPlayerLoadPacket> : OutputPacket<DGPlayerLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PLAYER_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGPlayerSkillLoadPacket> : OutputPacket<DGPlayerSkillLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PLAYER_SKILL_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGItemLoadPacket> : OutputPacket<DGItemLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::ITEM_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGQuestLoadPacket> : OutputPacket<DGQuestLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::QUEST_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGAffectLoadPacket> : OutputPacket<DGAffectLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::AFFECT_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGPetLoadPacket> : OutputPacket<DGPetLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PET_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGSafeboxLoadPacket> : OutputPacket<DGSafeboxLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SAFEBOX_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGSafeboxChangeSizePacket> : OutputPacket<DGSafeboxChangeSizePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SAFEBOX_CHANGE_SIZE);
        }
    };
    template <>
    struct DGOutputPacket<DGSafeboxChangePasswordAnswerPacket> : OutputPacket<DGSafeboxChangePasswordAnswerPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SAFEBOX_CHANGE_PASSWORD_ANSWER);
        }
    };
    template <>
    struct DGOutputPacket<DGEmpireSelectPacket> : OutputPacket<DGEmpireSelectPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::EMPIRE_SELECT);
        }
    };
    template <>
    struct DGOutputPacket<DGP2PInfoPacket> : OutputPacket<DGP2PInfoPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::P2P_INFO);
        }
    };
    template <>
    struct DGOutputPacket<DGLoginAlreadyPacket> : OutputPacket<DGLoginAlreadyPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::LOGIN_ALREADY);
        }
    };
    template <>
    struct DGOutputPacket<DGTimePacket> : OutputPacket<DGTimePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::TIME);
        }
    };
    template <>
    struct DGOutputPacket<DGReloadProtoPacket> : OutputPacket<DGReloadProtoPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::RELOAD_PROTO);
        }
    };
    template <>
    struct DGOutputPacket<DGReloadShopTablePacket> : OutputPacket<DGReloadShopTablePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::RELOAD_SHOP_TABLE);
        }
    };
    template <>
    struct DGOutputPacket<DGReloadMobProtoPacket> : OutputPacket<DGReloadMobProtoPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::RELOAD_MOB_PROTO);
        }
    };
    template <>
    struct DGOutputPacket<DGChangeNamePacket> : OutputPacket<DGChangeNamePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::CHANGE_NAME);
        }
    };
    template <>
    struct DGOutputPacket<DGAuthLoginPacket> : OutputPacket<DGAuthLoginPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::AUTH_LOGIN);
        }
    };
    template <>
    struct DGOutputPacket<DGChangeEmpirePrivPacket> : OutputPacket<DGChangeEmpirePrivPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::CHANGE_EMPIRE_PRIV);
        }
    };
    template <>
    struct DGOutputPacket<DGChangeGuildPrivPacket> : OutputPacket<DGChangeGuildPrivPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::CHANGE_GUILD_PRIV);
        }
    };
    template <>
    struct DGOutputPacket<DGChangeCharacterPrivPacket> : OutputPacket<DGChangeCharacterPrivPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::CHANGE_CHARACTER_PRIV);
        }
    };
    template <>
    struct DGOutputPacket<DGMoneyLogPacket> : OutputPacket<DGMoneyLogPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MONEY_LOG);
        }
    };
    template <>
    struct DGOutputPacket<DGSetEventFlagPacket> : OutputPacket<DGSetEventFlagPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SET_EVENT_FLAG);
        }
    };
    template <>
    struct DGOutputPacket<DGVCardPacket> : OutputPacket<DGVCardPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::VCARD);
        }
    };
    template <>
    struct DGOutputPacket<DGNoticePacket> : OutputPacket<DGNoticePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::NOTICE);
        }
    };
    template <>
    struct DGOutputPacket<DGAddBlockCountryIPPacket> : OutputPacket<DGAddBlockCountryIPPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::ADD_BLOCK_COUNTRY_IP);
        }
    };
    template <>
    struct DGOutputPacket<DGBlockExceptionPacket> : OutputPacket<DGBlockExceptionPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::BLOCK_EXCEPTION);
        }
    };
    template <>
    struct DGOutputPacket<DGMyShopPricelistPacket> : OutputPacket<DGMyShopPricelistPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MYSHOP_PRICELIST);
        }
    };
    template <>
    struct DGOutputPacket<DGReloadAdminPacket> : OutputPacket<DGReloadAdminPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::RELOAD_ADMIN);
        }
    };
    template <>
    struct DGOutputPacket<DGDetailLogPacket> : OutputPacket<DGDetailLogPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::DETAIL_LOG);
        }
    };
    template <>
    struct DGOutputPacket<DGItemAwardInformerPacket> : OutputPacket<DGItemAwardInformerPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::ITEM_AWARD_INFORMER);
        }
    };
    template <>
    struct DGOutputPacket<DGRespondChannelStatusPacket> : OutputPacket<DGRespondChannelStatusPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::RESPOND_CHANNEL_STATUS);
        }
    };
    template <>
    struct DGOutputPacket<DGChannelSwitchPacket> : OutputPacket<DGChannelSwitchPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::CHANNEL_SWITCH);
        }
    };
    template <>
    struct DGOutputPacket<DGSpareItemIDRangePacket> : OutputPacket<DGSpareItemIDRangePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SPARE_ITEM_ID_RANGE);
        }
    };
    template <>
    struct DGOutputPacket<DGUpdateHorseNamePacket> : OutputPacket<DGUpdateHorseNamePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::UPDATE_HORSE_NAME);
        }
    };
    template <>
    struct DGOutputPacket<DGSpawnMobTimedPacket> : OutputPacket<DGSpawnMobTimedPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SPAWN_MOB_TIMED);
        }
    };
    template <>
    struct DGOutputPacket<DGItemOfflineRestorePacket> : OutputPacket<DGItemOfflineRestorePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::ITEM_OFFLINE_RESTORE);
        }
    };
    template <>
    struct DGOutputPacket<DGOfflineMessagesLoadPacket> : OutputPacket<DGOfflineMessagesLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::OFFLINE_MESSAGES_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGItemRefundLoadPacket> : OutputPacket<DGItemRefundLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::ITEM_REFUND_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGWhisperPlayerExistResultPacket> : OutputPacket<DGWhisperPlayerExistResultPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WHISPER_PLAYER_EXIST_RESULT);
        }
    };
    template <>
    struct DGOutputPacket<DGWhisperPlayerMessageOfflinePacket> : OutputPacket<DGWhisperPlayerMessageOfflinePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WHISPER_PLAYER_MESSAGE_OFFLINE);
        }
    };
    template <>
    struct DGOutputPacket<DGReloadXmasRewardsPacket> : OutputPacket<DGReloadXmasRewardsPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::RELOAD_XMAS_REWARDS);
        }
    };
    template <>
    struct DGOutputPacket<DGSetAveragePricesPacket> : OutputPacket<DGSetAveragePricesPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SET_AVERAGE_PRICES);
        }
    };
    template <>
    struct DGOutputPacket<DGSkillColorLoadPacket> : OutputPacket<DGSkillColorLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::SKILL_COLOR_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGEquipmentPageLoadPacket> : OutputPacket<DGEquipmentPageLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::EQUIPMENT_PAGE_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildDungeonPacket> : OutputPacket<DGGuildDungeonPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_DUNGEON);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildDungeonCDPacket> : OutputPacket<DGGuildDungeonCDPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_DUNGEON_CD);
        }
    };
    template <>
    struct DGOutputPacket<DGMaintenancePacket> : OutputPacket<DGMaintenancePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MAINTENANCE);
        }
    };
    template <>
    struct DGOutputPacket<DGWhitelistIPPacket> : OutputPacket<DGWhitelistIPPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WHITELIST_IP);
        }
    };
    template <>
    struct DGOutputPacket<DGAuctionDeletePlayer> : OutputPacket<DGAuctionDeletePlayer> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::AUCTION_DELETE_PLAYER);
        }
    };
    template <>
    struct DGOutputPacket<DGCreateObjectPacket> : OutputPacket<DGCreateObjectPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::CREATE_OBJECT);
        }
    };
    template <>
    struct DGOutputPacket<DGDeleteObjectPacket> : OutputPacket<DGDeleteObjectPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::DELETE_OBJECT);
        }
    };
    template <>
    struct DGOutputPacket<DGUpdateLandPacket> : OutputPacket<DGUpdateLandPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::UPDATE_LAND);
        }
    };
    template <>
    struct DGOutputPacket<DGMarriageAddPacket> : OutputPacket<DGMarriageAddPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MARRIAGE_ADD);
        }
    };
    template <>
    struct DGOutputPacket<DGMarriageUpdatePacket> : OutputPacket<DGMarriageUpdatePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MARRIAGE_UPDATE);
        }
    };
    template <>
    struct DGOutputPacket<DGMarriageRemovePacket> : OutputPacket<DGMarriageRemovePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::MARRIAGE_REMOVE);
        }
    };
    template <>
    struct DGOutputPacket<DGWeddingRequestPacket> : OutputPacket<DGWeddingRequestPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WEDDING_REQUEST);
        }
    };
    template <>
    struct DGOutputPacket<DGWeddingReadyPacket> : OutputPacket<DGWeddingReadyPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WEDDING_READY);
        }
    };
    template <>
    struct DGOutputPacket<DGWeddingStartPacket> : OutputPacket<DGWeddingStartPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WEDDING_START);
        }
    };
    template <>
    struct DGOutputPacket<DGWeddingEndPacket> : OutputPacket<DGWeddingEndPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::WEDDING_END);
        }
    };
    template <>
    struct DGOutputPacket<DGBillingRepairPacket> : OutputPacket<DGBillingRepairPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::BILLING_REPAIR);
        }
    };
    template <>
    struct DGOutputPacket<DGBillingExpirePacket> : OutputPacket<DGBillingExpirePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::BILLING_EXPIRE);
        }
    };
    template <>
    struct DGOutputPacket<DGBillingLoginPacket> : OutputPacket<DGBillingLoginPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::BILLING_LOGIN);
        }
    };
    template <>
    struct DGOutputPacket<DGBillingCheckPacket> : OutputPacket<DGBillingCheckPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::BILLING_CHECK);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildLoadPacket> : OutputPacket<DGGuildLoadPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_LOAD);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildSkillUpdatePacket> : OutputPacket<DGGuildSkillUpdatePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_SKILL_UPDATE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildExpUpdatePacket> : OutputPacket<DGGuildExpUpdatePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_EXP_UPDATE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildAddMemberPacket> : OutputPacket<DGGuildAddMemberPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_ADD_MEMBER);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildRemoveMemberPacket> : OutputPacket<DGGuildRemoveMemberPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_REMOVE_MEMBER);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildChangeGradePacket> : OutputPacket<DGGuildChangeGradePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_CHANGE_GRADE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildChangeMemberDataPacket> : OutputPacket<DGGuildChangeMemberDataPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_CHANGE_MEMBER_DATA);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildDisbandPacket> : OutputPacket<DGGuildDisbandPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_DISBAND);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildLadderPacket> : OutputPacket<DGGuildLadderPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_LADDER);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildWarPacket> : OutputPacket<DGGuildWarPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_WAR);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildWarScorePacket> : OutputPacket<DGGuildWarScorePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_WAR_SCORE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildSkillUsableChangePacket> : OutputPacket<DGGuildSkillUsableChangePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_SKILL_USABLE_CHANGE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildMoneyChangePacket> : OutputPacket<DGGuildMoneyChangePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_MONEY_CHANGE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildMoneyWithdrawPacket> : OutputPacket<DGGuildMoneyWithdrawPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_MONEY_WITHDRAW);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildWarReserveAddPacket> : OutputPacket<DGGuildWarReserveAddPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_WAR_RESERVE_ADD);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildWarReserveDeletePacket> : OutputPacket<DGGuildWarReserveDeletePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_WAR_RESERVE_DELETE);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildWarBetPacket> : OutputPacket<DGGuildWarBetPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_WAR_BET);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildChangeMasterPacket> : OutputPacket<DGGuildChangeMasterPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_CHANGE_MASTER);
        }
    };
    template <>
    struct DGOutputPacket<DGGuildSafeboxPacket> : OutputPacket<DGGuildSafeboxPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::GUILD_SAFEBOX);
        }
    };
    template <>
    struct DGOutputPacket<DGPartyCreatePacket> : OutputPacket<DGPartyCreatePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PARTY_CREATE);
        }
    };
    template <>
    struct DGOutputPacket<DGPartyDeletePacket> : OutputPacket<DGPartyDeletePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PARTY_DELETE);
        }
    };
    template <>
    struct DGOutputPacket<DGPartyAddPacket> : OutputPacket<DGPartyAddPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PARTY_ADD);
        }
    };
    template <>
    struct DGOutputPacket<DGPartyRemovePacket> : OutputPacket<DGPartyRemovePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PARTY_REMOVE);
        }
    };
    template <>
    struct DGOutputPacket<DGPartyStateChangePacket> : OutputPacket<DGPartyStateChangePacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PARTY_STATE_CHANGE);
        }
    };
    template <>
    struct DGOutputPacket<DGPartySetMemberLevelPacket> : OutputPacket<DGPartySetMemberLevelPacket> {
        DGOutputPacket()
        {
            header.header = static_cast<uint16_t>(TDGHeader::PARTY_SET_MEMBER_LEVEL);
        }
    };
}
