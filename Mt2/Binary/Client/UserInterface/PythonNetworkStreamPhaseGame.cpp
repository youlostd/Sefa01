#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "Packet.h"

#include "NetworkActorManager.h"

#include "PythonGuild.h"
#include "PythonCharacterManager.h"
#include "PythonPlayer.h"
#include "PythonBackground.h"
#include "PythonMiniMap.h"
#include "PythonTextTail.h"
#include "PythonItem.h"
#include "PythonChat.h"
#include "PythonShop.h"
#include "PythonExchange.h"
#include "PythonQuest.h"
#include "PythonEventManager.h"
#include "PythonMessenger.h"
#include "PythonApplication.h"

#include "../EterPack/EterPackManager.h"
#include "../gamelib/ItemManager.h"

#include "AbstractApplication.h"
#include "AbstractCharacterManager.h"
#include "InstanceBase.h"
#include "test.h"

#ifdef COMBAT_ZONE
#include "PythonCombatZone.h"
#endif

#include "PythonWhisperManager.h"

using namespace network;

extern const char * g_szCurrentClientVersion;

BOOL gs_bEmpireLanuageEnable = TRUE;

void CPythonNetworkStream::__RefreshAlignmentWindow()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshAlignment", Py_BuildValue("()"));
}

void CPythonNetworkStream::__RefreshTargetBoardByVID(DWORD dwVID)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshTargetBoardByVID", Py_BuildValue("(i)", dwVID));
}

void CPythonNetworkStream::__RefreshGuildRankingWindow(bool isSearch)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildRankingList", Py_BuildValue("(O)", isSearch ? Py_True : Py_False));
}

void CPythonNetworkStream::__RefreshTargetBoardByName(const char * c_szName)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshTargetBoardByName", Py_BuildValue("(s)", c_szName));
}

void CPythonNetworkStream::__RefreshTargetBoard()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshTargetBoard", Py_BuildValue("()"));
}

void CPythonNetworkStream::__RefreshGuildWindowGradePage()
{
	m_isRefreshGuildWndGradePage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowSkillPage()
{
	m_isRefreshGuildWndSkillPage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowMemberPageGradeComboBox()
{
	m_isRefreshGuildWndMemberPageGradeComboBox=true;
}

void CPythonNetworkStream::__RefreshGuildWindowMemberPageLastPlayed()
{
	m_isRefreshGuildWndMemberPageLastPlayed=true;
}

void CPythonNetworkStream::__RefreshGuildWindowMemberPage()
{
	m_isRefreshGuildWndMemberPage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowBoardPage()
{
	m_isRefreshGuildWndBoardPage=true;
}

void CPythonNetworkStream::__RefreshGuildWindowInfoPage()
{
	m_isRefreshGuildWndInfoPage=true;
}

void CPythonNetworkStream::__RefreshMessengerWindow()
{
	m_isRefreshMessengerWnd=true;
}

void CPythonNetworkStream::__RefreshSafeboxWindow()
{
	m_isRefreshSafeboxWnd=true;
}

void CPythonNetworkStream::__RefreshMallWindow()
{
	m_isRefreshMallWnd=true;
}

void CPythonNetworkStream::__RefreshSkillWindow()
{
	m_isRefreshSkillWnd=true;
}

void CPythonNetworkStream::__RefreshExchangeWindow()
{
	m_isRefreshExchangeWnd=true;
}

void CPythonNetworkStream::__RefreshStatus()
{
	m_isRefreshStatus=true;
}

void CPythonNetworkStream::__RefreshCharacterWindow()
{
	m_isRefreshCharacterWnd=true;
}

void CPythonNetworkStream::__RefreshInventoryWindow()
{
	m_isRefreshInventoryWnd=true;
}

void CPythonNetworkStream::__RefreshEquipmentWindow()
{
	m_isRefreshEquipmentWnd=true;
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CPythonNetworkStream::__RefreshAcceWindow()
{
	m_isRefreshAcceWnd=true;
}
#endif

#ifdef ENABLE_GUILD_SAFEBOX
void CPythonNetworkStream::__RefreshGuildSafeboxWindow()
{
	m_isRefreshGuildSafeboxWnd = true;
}
#endif

#ifdef ENABLE_ANIMAL_SYSTEM
void CPythonNetworkStream::__RefreshAnimalWindow(BYTE bType)
{
	m_isRefreshAnimalWnd[bType] = true;
}
#endif

#ifdef ENABLE_PET_ADVANCED
void CPythonNetworkStream::__RefreshPetStatusWindow()
{
	m_isRefreshPetStatusWnd = true;
}

void CPythonNetworkStream::__RefreshPetSkillWindow()
{
	m_isRefreshPetSkillWnd = true;
}
#endif

void CPythonNetworkStream::__SetGuildID(DWORD id)
{
	if (m_dwGuildID != id)
	{
		m_dwGuildID = id;
		IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();

		for (int i = 0; i < PLAYER_PER_ACCOUNT4; ++i)
			if (m_akSimplePlayerInfo[i].name() == rkPlayer.GetName())
			{
				m_akSimplePlayerInfo[i].set_guild_id(id);

				std::string  guildName;
				if (CPythonGuild::Instance().GetGuildName(id, &guildName))
				{
					m_akSimplePlayerInfo[i].set_guild_name(guildName);
				}
				else
				{
					m_akSimplePlayerInfo[i].clear_guild_name();
				}
			}
	}
}
/*
bool CPythonNetworkStream::SendKingMountMeltPacket(WORD wHorseCell, WORD wMountCell, WORD wStoneCell)
{
	TPacketCGKingMountMelt packet;
	packet.bHeader = HEADER_CG_KING_MOUNT_MELT;
	packet.wCellHorse = wHorseCell;
	packet.wCellMount = wMountCell;
	packet.wCellStone = wStoneCell;

	if (!Send(packet))
		return false;

	return true;
}*/

bool CPythonNetworkStream::RecvHorseRefineInfo(std::unique_ptr<GCHorseRefineInfoPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_HorseRefine", Py_BuildValue("(iiL)", pack->refine_index(), pack->current_level(), (long long) pack->refine().cost()));
	for (int i = 0; i < pack->refine().material_count(); ++i)
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_HorseRefineAdd", Py_BuildValue("(ii)", pack->refine().materials(i).vnum(), pack->refine().materials(i).count()));
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_HorseRefineOpen", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvHorseRefineResult(std::unique_ptr<GCHorseRefineResultPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_HorseRefineResult", Py_BuildValue("(b)", pack->success()));

	return true;
}

#ifdef __PERFORMANCE_CHECK__

class PERF_PacketTimeAnalyzer
{
	public:
		~PERF_PacketTimeAnalyzer()
		{
			FILE* fp=fopen("perf_dispatch_packet_result.txt", "w");		

			for (std::map<DWORD, PERF_PacketInfo>::iterator i=m_kMap_kPacketInfo.begin(); i!=m_kMap_kPacketInfo.end(); ++i)
			{
				if (i->second.dwTime>0)
					fprintf(fp, "header %d: count %d, time %d, tpc %d\n", i->first, i->second.dwCount, i->second.dwTime, i->second.dwTime/i->second.dwCount);
			}
			fclose(fp);
		}

	public:
		std::map<DWORD, PERF_PacketInfo> m_kMap_kPacketInfo;
};

PERF_PacketTimeAnalyzer gs_kPacketTimeAnalyzer;

#endif

// Game Phase ---------------------------------------------------------------------------
void CPythonNetworkStream::GamePhasePacket(InputPacket& packet, bool& ret)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::SET_VERIFY_KEY:
			SetPacketVerifyKey(packet.get<GCSetVerifyKeyPacket>()->verify_key());
			ret = true;
			break;
		case TGCHeader::OBSERVER_ADD:
			ret = RecvObserverAddPacket(packet.get<GCObserverAddPacket>());
			break;
		case TGCHeader::OBSERVER_REMOVE:
			ret = RecvObserverRemovePacket(packet.get<GCObserverRemovePacket>());
			break;
		case TGCHeader::OBSERVER_MOVE:
			ret = RecvObserverMovePacket(packet.get<GCObserverMovePacket>());
			break;
		case TGCHeader::WARP:
			ret = RecvWarpPacket(packet.get<GCWarpPacket>());
			break;

		case TGCHeader::PHASE:
			ret = RecvPhasePacket(packet.get<GCPhasePacket>());
			break;

		case TGCHeader::PVP:
			ret = RecvPVPPacket(packet.get<GCPVPPacket>());
			break;

		case TGCHeader::DUEL_START:
			ret = RecvDuelStartPacket(packet.get<GCDuelStartPacket>());
			break;

		case TGCHeader::CHARACTER_ADD:
 			ret = RecvCharacterAppendPacket(packet.get<GCCharacterAddPacket>());
			break;

		case TGCHeader::CHARACTER_ADDITIONAL_INFO:
			ret = RecvCharacterAdditionalInfo(packet.get<GCCharacterAdditionalInfoPacket>());
			break;

		case TGCHeader::CHARACTER_UPDATE:
			ret = RecvCharacterUpdatePacket(packet.get<GCCharacterUpdatePacket>());
			break;

		case TGCHeader::CHARACTER_DELETE:
			ret = RecvCharacterDeletePacket(packet.get<GCCharacterDeletePacket>());
			break;

		case TGCHeader::CHAT:
			ret = RecvChatPacket(packet.get<GCChatPacket>());
			break;

		case TGCHeader::SYNC_POSITION:
			ret = RecvSyncPositionPacket(packet.get<GCSyncPositionPacket>());
			break;

		case TGCHeader::OWNERSHIP:
			ret = RecvOwnerShipPacket(packet.get<GCOwnershipPacket>());
			break;

		case TGCHeader::WHISPER:
			ret = RecvWhisperPacket(packet.get<GCWhisperPacket>());
			break;

		case TGCHeader::MOVE:
			ret = RecvCharacterMovePacket(packet.get<GCMovePacket>());
			break;

		// Battle Packet
		case TGCHeader::STUN:
			ret = RecvStunPacket(packet.get<GCStunPacket>());
			break;

		case TGCHeader::DEAD:
			ret = RecvDeadPacket(packet.get<GCDeadPacket>());
			break;

		case TGCHeader::POINT_CHANGE:
			ret = RecvPointChange(packet.get<GCPointChangePacket>());
			break;

		case TGCHeader::REAL_POINT_SET:
			ret = RecvRealPointSet(packet.get<GCRealPointSetPacket>());
			break;

		// item packet.
		case TGCHeader::ITEM_SET:
			ret = RecvItemSetPacket(packet.get<GCItemSetPacket>());
			break;

		case TGCHeader::ITEM_UPDATE:
			ret = RecvItemUpdatePacket(packet.get<GCItemUpdatePacket>());
			break;

		case TGCHeader::ITEM_GROUND_ADD:
			ret = RecvItemGroundAddPacket(packet.get<GCItemGroundAddPacket>());
			break;

		case TGCHeader::ITEM_GROUND_DELETE:
			ret = RecvItemGroundDelPacket(packet.get<GCItemGroundDeletePacket>());
			break;

		case TGCHeader::ITEM_OWNERSHIP:
			ret = RecvItemOwnership(packet.get<GCItemOwnershipPacket>());
			break;

		case TGCHeader::QUICKSLOT_ADD:
			ret = RecvQuickSlotAddPacket(packet.get<GCQuickslotAddPacket>());
			break;

		case TGCHeader::QUICKSLOT_DEL:
			ret = RecvQuickSlotDelPacket(packet.get<GCQuickslotDelPacket>());
			break;

		case TGCHeader::QUICKSLOT_SWAP:
			ret = RecvQuickSlotMovePacket(packet.get<GCQuickslotSwapPacket>());
			break;

		case TGCHeader::MOTION:
			ret = RecvMotionPacket(packet.get<GCMotionPacket>());
			break;

		case TGCHeader::SHOP_START:
		case TGCHeader::SHOP_EX_START:
		case TGCHeader::SHOP_UPDATE_ITEM:
		case TGCHeader::SHOP_END:
		case TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY:
		case TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY_EX:
		case TGCHeader::SHOP_ERROR_SOLDOUT:
		case TGCHeader::SHOP_ERROR_INVENTORY_FULL:
		case TGCHeader::SHOP_ERROR_INVALID_POS:
		case TGCHeader::SHOP_ERROR_ZODIAC_SHOP:
		case TGCHeader::SHOP_ERROR_MAX_LIMIT_POINTS:
		case TGCHeader::SHOP_ERROR_NOT_ENOUGH_POINTS:
		case TGCHeader::SHOP_ERROR_OVERFLOW_LIMIT_POINTS:
			ret = RecvShopPacket(packet);
			break;
				
		case TGCHeader::SHOP_SIGN:
			ret = RecvShopSignPacket(packet.get<GCShopSignPacket>());
			break;

		case TGCHeader::EXCHANGE_START:
		case TGCHeader::EXCHANGE_ITEM_ADD:
		case TGCHeader::EXCHANGE_ITEM_DEL:
		case TGCHeader::EXCHANGE_GOLD_ADD:
		case TGCHeader::EXCHANGE_ACCEPT:
		case TGCHeader::EXCHANGE_CANCEL:
			ret = RecvExchangePacket(packet);
			break;

		case TGCHeader::QUEST_INFO:
			ret = RecvQuestInfoPacket(packet.get<GCQuestInfoPacket>());
			break;

		case TGCHeader::GUILD_REQUEST_MAKE:
			ret = RecvRequestMakeGuild();
			break;

		case TGCHeader::PING:
			ret = RecvPingPacket();
			break;

		case TGCHeader::SCRIPT:
			ret = RecvScriptPacket(packet.get<GCScriptPacket>());
			break;

		case TGCHeader::QUEST_CONFIRM:
			ret = RecvQuestConfirmPacket(packet.get<GCQuestConfirmPacket>());
			break;

//		case TGCHeader::QUEST_DELETE:
//			ret = RecvQuestDeletePacket();
//			break;

		case TGCHeader::TARGET:
			ret = RecvTargetPacket(packet.get<GCTargetPacket>());
			break;

		case TGCHeader::DAMAGE_INFO:
			ret = RecvDamageInfoPacket(packet.get<GCDamageInfoPacket>());
			break;

		case TGCHeader::POINTS:
			ret = __RecvPlayerPoints(packet.get<GCPointsPacket>());
			break;

		case TGCHeader::CREATE_FLY:
			ret = RecvCreateFlyPacket(packet.get<GCCreateFlyPacket>());
			break;

		case TGCHeader::FLY_TARGETING:
			ret = RecvFlyTargetingPacket(packet.get<GCFlyTargetingPacket>());
			break;

		case TGCHeader::ADD_FLY_TARGETING:
			ret = RecvAddFlyTargetingPacket(packet.get<GCAddFlyTargetingPacket>());
			break;

		case TGCHeader::SKILL_LEVEL:
			ret = RecvSkillLevel(packet.get<GCSkillLevelPacket>());
			break;

		case TGCHeader::MESSENGER_LIST:
		case TGCHeader::MESSENGER_LOGIN:
		case TGCHeader::MESSENGER_LOGOUT:
		case TGCHeader::MESSENGER_BLOCK_LIST:
		case TGCHeader::MESSENGER_BLOCK_LOGIN:
		case TGCHeader::MESSENGER_BLOCK_LOGOUT:
		case TGCHeader::MESSENGER_MOBILE:
			ret = RecvMessenger(packet);
			break;

		case TGCHeader::GUILD_LOGIN:
		case TGCHeader::GUILD_LOGOUT:
		case TGCHeader::GUILD_REMOVE:
		case TGCHeader::GUILD_MEMBER_LIST:
		case TGCHeader::GUILD_GRADE:
		case TGCHeader::GUILD_GRADE_NAME:
		case TGCHeader::GUILD_GRADE_AUTH:
		case TGCHeader::GUILD_INFO:
		case TGCHeader::GUILD_COMMENTS:
		case TGCHeader::GUILD_CHANGE_EXP:
		case TGCHeader::GUILD_CHANGE_MEMBER_GRADE:
		case TGCHeader::GUILD_SKILL_INFO:
		case TGCHeader::GUILD_CHANGE_MEMBER_GENERAL:
		case TGCHeader::GUILD_INVITE:
		case TGCHeader::GUILD_MEMBER_LAST_PLAYED:
		case TGCHeader::GUILD_BATTLE_STATS:
		case TGCHeader::GUILD_WAR:
		case TGCHeader::GUILD_NAME:
		case TGCHeader::GUILD_WAR_LIST:
		case TGCHeader::GUILD_WAR_END_LIST:
		case TGCHeader::GUILD_WAR_POINT:
		case TGCHeader::GUILD_MONEY_CHANGE:
		case TGCHeader::GUILD_LADDER_LIST:
		case TGCHeader::GUILD_LADDER_SEARCH_RESULT:
		case TGCHeader::GUILD_RANK_AND_POINT:
			ret = RecvGuild(packet);
			break;

		case TGCHeader::PARTY_INVITE:
			ret = RecvPartyInvite(packet.get<GCPartyInvitePacket>());
			break;

		case TGCHeader::PARTY_ADD:
			ret = RecvPartyAdd(packet.get<GCPartyAddPacket>());
			break;

		case TGCHeader::PARTY_UPDATE:
			ret = RecvPartyUpdate(packet.get<GCPartyUpdatePacket>());
			break;

		case TGCHeader::PARTY_REMOVE:
			ret = RecvPartyRemove(packet.get<GCPartyRemovePacket>());
			break;

		case TGCHeader::PARTY_LINK:
			ret = RecvPartyLink(packet.get<GCPartyLinkPacket>());
			break;

		case TGCHeader::PARTY_UNLINK:
			ret = RecvPartyUnlink(packet.get<GCPartyUnlinkPacket>());
			break;

		case TGCHeader::PARTY_PARAMETER:
			ret = RecvPartyParameter(packet.get<GCPartyParameterPacket>());
			break;

		case TGCHeader::SAFEBOX_WRONG_PASSWORD:
			ret = RecvSafeBoxWrongPasswordPacket();
			break;

		case TGCHeader::SAFEBOX_SIZE:
			ret = RecvSafeBoxSizePacket(packet.get<GCSafeboxSizePacket>());
			break;

		case TGCHeader::FISHING_START:
		case TGCHeader::FISHING_STOP:
		case TGCHeader::FISHING_REACT:
		case TGCHeader::FISHING_SUCCESS:
		case TGCHeader::FISHING_FAIL:
		case TGCHeader::FISHING_FISH_INFO:
			ret = RecvFishing(packet);
			break;

		case TGCHeader::DUNGEON_DESTINATION_POSITION:
			ret = RecvDungeon(packet.get<GCDungeonDestinationPositionPacket>());
			break;

		case TGCHeader::TIME:
			ret = RecvTimePacket(packet.get<GCTimePacket>());
			break;

		case TGCHeader::WALK_MODE:
			ret = RecvWalkModePacket(packet.get<GCWalkModePacket>());
			break;

		case TGCHeader::CHANGE_SKILL_GROUP:
			ret = RecvChangeSkillGroupPacket(packet.get<GCChangeSkillGroupPacket>());
			break;

		case TGCHeader::REFINE_INFORMATION:
			ret = RecvRefineInformationPacket(packet.get<GCRefineInformationPacket>());
			break;

		case TGCHeader::SPECIAL_EFFECT:
			ret = RecvSpecialEffect(packet.get<GCSpecialEffectPacket>());
			break;

		case TGCHeader::NPC_LIST:
			ret = RecvNPCList(packet.get<GCNPCListPacket>());
			break;

		case TGCHeader::VIEW_EQUIP:
			ret = RecvViewEquipPacket(packet.get<GCViewEquipPacket>());
			break;

		case TGCHeader::LAND_LIST:
			ret = RecvLandPacket(packet.get<GCLandListPacket>());
			break;

		case TGCHeader::TARGET_CREATE:
			ret = RecvTargetCreatePacket(packet.get<GCTargetCreatePacket>());
			break;

		case TGCHeader::TARGET_UPDATE:
			ret = RecvTargetUpdatePacket(packet.get<GCTargetUpdatePacket>());
			break;

		case TGCHeader::TARGET_DELETE:
			ret = RecvTargetDeletePacket(packet.get<GCTargetDeletePacket>());
			break;

		case TGCHeader::HORSE_REFINE_INFO:
			ret = RecvHorseRefineInfo(packet.get<GCHorseRefineInfoPacket>());
			break;

		case TGCHeader::HORSE_REFINE_RESULT:
			ret = RecvHorseRefineResult(packet.get<GCHorseRefineResultPacket>());
			break;

		case TGCHeader::AFFECT_ADD:
			ret = RecvAffectAddPacket(packet.get<GCAffectAddPacket>());
			break;

		case TGCHeader::AFFECT_REMOVE:
			ret = RecvAffectRemovePacket(packet.get<GCAffectRemovePacket>());
			break;

		case TGCHeader::MALL_OPEN:
			ret = RecvMallOpenPacket(packet.get<GCMallOpenPacket>());
			break;

		case TGCHeader::LOVER_INFO:
			ret = RecvLoverInfoPacket(packet.get<GCLoverInfoPacket>());
			break;

		case TGCHeader::LOVER_POINT_UPDATE:
			ret = RecvLovePointUpdatePacket(packet.get<GCLoverPointUpdatePacket>());
			break;

		case TGCHeader::DIG_MOTION:
			ret = RecvDigMotionPacket(packet.get<GCDigMotionPacket>());
			break;

		case TGCHeader::HANDSHAKE:
			RecvHandshakePacket(packet.get<GCHandshakePacket>());
			break;

		case TGCHeader::HANDSHAKE_OK:
			RecvHandshakeOKPacket();
			break;

#ifdef _IMPROVED_PACKET_ENCRYPTION_
		case TGCHeader::KEY_AGREEMENT:
			RecvKeyAgreementPacket(packet.get<GCKeyAgreementPacket>());
			break;

		case TGCHeader::KEY_AGREEMENT_COMPLETED:
			RecvKeyAgreementCompletedPacket();
			break;
#endif
		case TGCHeader::SPECIFIC_EFFECT:
			ret = RecvSpecificEffect(packet.get<GCSpecificEffectPacket>());
			break;

		case TGCHeader::PVP_TEAM:
			ret = RecvPVPTeam(packet.get<GCPVPTeamPacket>());
			break;

#ifdef ENABLE_PET_ADVANCED
		case TGCHeader::PET_SUMMON:
		case TGCHeader::PET_UPDATE_EXP:
		case TGCHeader::PET_UPDATE_LEVEL:
		case TGCHeader::PET_UPDATE_SKILL:
		case TGCHeader::PET_ATTR_REFINE_INFO:
		case TGCHeader::PET_UPDATE_ATTR:
		case TGCHeader::PET_UPDATE_SKILLPOWER:
		case TGCHeader::PET_EVOLUTION_INFO:
		case TGCHeader::PET_EVOLVE_RESULT:
		case TGCHeader::PET_UNSUMMON:
			ret = RecvPetAdvanced(packet);
			break;
#endif

		case TGCHeader::UPDATE_CHARACTER_SCALE:
			ret = RecvUpdateCharacterScale(packet.get<GCUpdateCharacterScalePacket>());
			break;

#ifdef ENABLE_MAINTENANCE
		case TGCHeader::MAINTENANCE_INFO:
			ret = RecvMaintenancePacket(packet.get<GCMaintenanceInfoPacket>());
			break;
#endif

#ifdef ENABLE_GUILD_SAFEBOX
		case TGCHeader::GUILD_SAFEBOX_OPEN:
		case TGCHeader::GUILD_SAFEBOX_GOLD:
		case TGCHeader::GUILD_SAFEBOX_ENABLE:
		case TGCHeader::GUILD_SAFEBOX_DISABLE:
		case TGCHeader::GUILD_SAFEBOX_LOAD_LOG_START:
		case TGCHeader::GUILD_SAFEBOX_LOAD_LOG:
		case TGCHeader::GUILD_SAFEBOX_LOAD_LOG_DONE:
		case TGCHeader::GUILD_SAFEBOX_APPEND_LOG:
		case TGCHeader::GUILD_SAFEBOX_CLOSE:
			ret = RecvGuildSafeboxPacket(packet);
			break;
#endif

		case TGCHeader::INVENTORY_MAX_NUM:
			ret = RecvInventoryMaxNum(packet.get<GCInventoryMaxNumPacket>());
			break;

#ifdef ENABLE_ANIMAL_SYSTEM
		case TGCHeader::ANIMAL_SUMMON:
			ret = RecvAnimalSummon();
			break;

		case TGCHeader::ANIMAL_UPDATE_LEVEL:
			ret = RecvAnimalUpdateLevel();
			break;

		case TGCHeader::ANIMAL_UPDATE_EXP:
			ret = RecvAnimalUpdateExp();
			break;

		case TGCHeader::ANIMAL_UPDATE_STATS:
			ret = RecvAnimalUpdateStats();
			break;

		case TGCHeader::ANIMAL_UNSUMMON:
			ret = RecvAnimalUnsummon();
			break;
#endif

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
		case TGCHeader::ATTRIBUTES_TO_CLIENT:
			ret = RecvAttributesToClient(packet.get<GCAttributesToClientPacket>());
			break;
#endif

		case TGCHeader::SKILL_MOTION:
			ret = RecvSkillMotion(packet.get<GCSkillMotionPacket>());
			break;

#ifdef ENABLE_DRAGONSOUL
		case TGCHeader::DRAGON_SOUL_REFINE:
			ret = RecvDragonSoulRefine(packet.get<GCDragonSoulRefinePacket>());
			break;
#endif

#ifdef ENABLE_GAYA_SYSTEM
		case TGCHeader::GAYA_SHOP_OPEN:
			ret = RecvGayaShopOpen(packet.get<GCGayaShopOpenPacket>());
			break;
#endif

		case TGCHeader::TARGET_MONSTER_INFO:
			ret = RecvTargetMonsterDropInfo(packet.get<GCTargetMonsterInfoPacket>());
			break;

#ifdef ENABLE_AUCTION
		case TGCHeader::AUCTION_OWNED_GOLD:
			ret = RecvAuctionOwnedGold(packet.get<GCAuctionOwnedGoldPacket>());
			break;

		case TGCHeader::AUCTION_OWNED_ITEM:
			ret = RecvAuctionOwnedItem(packet.get<GCAuctionOwnedItemPacket>());
			break;

		case TGCHeader::AUCTION_SEARCH_RESULT:
			ret = RecvAuctionSearchResult(packet.get<GCAuctionSearchResultPacket>());
			break;

		case TGCHeader::AUCTION_MESSAGE:
			ret = RecvAuctionMessage(packet.get<GCAuctionMessagePacket>());
			break;

		case TGCHeader::AUCTION_SHOP_OWNED:
			ret = RecvAuctionShopOwned(packet.get<GCAuctionShopOwnedPacket>());
			break;

		case TGCHeader::AUCTION_SHOP:
			ret = RecvAuctionShop(packet.get<GCAuctionShopPacket>());
			break;

		case TGCHeader::AUCTION_SHOP_GOLD:
			ret = RecvAuctionShopGold(packet.get<GCAuctionShopGoldPacket>());
			break;

		case TGCHeader::AUCTION_SHOP_TIMEOUT:
			ret = RecvAuctionShopTimeout(packet.get<GCAuctionShopTimeoutPacket>());
			break;

		case TGCHeader::AUCTION_SHOP_GUEST_OPEN:
			ret = RecvAuctionShopGuestOpen(packet.get<GCAuctionShopGuestOpenPacket>());
			break;

		case TGCHeader::AUCTION_SHOP_GUEST_UPDATE:
			ret = RecvAuctionShopGuestUpdate(packet.get<GCAuctionShopGuestUpdatePacket>());
			break;

		case TGCHeader::AUCTION_SHOP_GUEST_CLOSE:
			ret = RecvAuctionShopGuestClose();
			break;

		case TGCHeader::AUCTION_SHOP_HISTORY:
			ret = RecvAuctionShopHistory(packet.get<GCAuctionShopHistoryPacket>());
			break;

		case TGCHeader::AUCTION_AVERAGE_PRICE:
			ret = RecvAuctionAveragePrice(packet.get<GCAuctionAveragePricePacket>());
			break;
#endif

		case TGCHeader::TEAMLER_SHOW:
			ret = RecvShowTeamler(packet.get<GCTeamlerShowPacket>());
			break;

		case TGCHeader::TEAMLER_STATUS:
			ret = RecvTeamlerStatus(packet.get<GCTeamlerStatusPacket>());
			break;
			
		case TGCHeader::PLAYER_ONLINE_INFORMATION:
			ret = RecvPlayerOnlineInformation(packet.get<GCPlayerOnlineInformationPacket>());
			break;

#ifdef ENABLE_FAKEBUFF
		case TGCHeader::FAKE_BUFF_SKILL:
			ret = RecvFakeBuffSkill(packet.get<GCFakeBuffSkillPacket>());
			break;
#endif

#ifdef ENABLE_ATTRTREE
		case TGCHeader::ATTRTREE_LEVEL:
			ret = RecvAttrtreeLevel(packet.get<GCAttrtreeLevelPacket>());
			break;

		case TGCHeader::ATTRTREE_REFINE:
			ret = RecvAttrtreeRefine(packet.get<GCAttrtreeRefinePacket>());
			break;
#endif

#ifdef ENABLE_EVENT_SYSTEM
		case TGCHeader::EVENT_REQUEST:
			ret = RecvEventRequestPacket(packet.get<GCEventRequestPacket>());
			break;

		case TGCHeader::EVENT_CANCEL:
			ret = RecvEventCancelPacket(packet.get<GCEventCancelPacket>());
			break;

		case TGCHeader::EVENT_EMPIRE_WAR_LOAD:
			ret = RecvEventEmpireWarLoadPacket(packet.get<GCEventEmpireWarLoadPacket>());
			break;

		case TGCHeader::EVENT_EMPIRE_WAR_UPDATE:
			ret = RecvEventEmpireWarUpdatePacket(packet.get<GCEventEmpireWarUpdatePacket>());
			break;

		case TGCHeader::EVENT_EMPIRE_WAR_FINISH:
			ret = RecvEventEmpireWarFinishPacket();
			break;
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
		case TGCHeader::CBT_OPEN:
		case TGCHeader::CBT_ITEM_SET:
		case TGCHeader::CBT_CLEAR:
		case TGCHeader::CBT_CLOSE:
			ret = RecvCostumeBonusTransferPacket(packet);
			break;
#endif

#ifdef COMBAT_ZONE
		case TGCHeader::COMBAT_ZONE_RANKING_DATA:
			ret = __RecvCombatZoneRankingPacket(packet.get<GCCombatZoneRankingDataPacket>());
			break;

		case TGCHeader::SEND_COMBAT_ZONE:
			ret = __RecvCombatZonePacket(packet.get<GCSendCombatZonePacket>());
			break;
#endif

		case TGCHeader::CHARACTER_SHINING:
			ret = RecvShiningPacket(packet.get<GCCharacterShiningPacket>());
			break;

#ifdef ENABLE_RUNE_SYSTEM
		case TGCHeader::RUNE:
			ret = RecvRune(packet.get<GCRunePacket>());
			break;

		case TGCHeader::RUNE_PAGE:
			ret = RecvRunePage(packet.get<GCRunePagePacket>());
			break;

		case TGCHeader::RUNE_REFINE:
			ret = RecvRuneRefine(packet.get<GCRuneRefinePacket>());
			break;

		case TGCHeader::RUNE_RESET_OWNED:
			ret = RecvRuneResetOwned();
			break;

#ifdef ENABLE_LEVEL2_RUNES
		case TGCHeader::RUNE_LEVELUP:
			ret = RecvRuneLevelUp(packet.get<GCRuneLevelupPacket>());
			break;
#endif
#endif

		case TGCHeader::SOUL_REFINE_INFO:
			ret = RecvSoulRefineInfo(packet.get<GCSoulRefineInfoPacket>());
			break;

#ifdef INGAME_WIKI
		case TGCHeader::WIKI:
			ret = RecvWikiPacket(packet.get<GCWikiPacket>());
			break;

		case TGCHeader::WIKI_MOB:
			ret = RecvWikiMobPacket(packet.get<GCWikiMobPacket>());
			break;
#endif

#ifdef BATTLEPASS_EXTENSION
		case TGCHeader::BATTLEPASS_DATA:
			ret = RecvBattlepassData(packet.get<GCBattlepassDataPacket>());
			break;
#endif
				
#ifdef ENABLE_EQUIPMENT_CHANGER
		case TGCHeader::EQUIPMENT_PAGE_LOAD:
			ret = RecvEquipmentPageLoadPacket(packet.get<GCEquipmentPageLoadPacket>());
			break;
#endif

#ifdef DMG_METER
		case TGCHeader::DMG_METER:
			ret = RecvDmgMeterPacket(packet.get<GCDmgMeterPacket>());
			break;
#endif

#ifdef CRYSTAL_SYSTEM
		case TGCHeader::CRYSTAL_REFINE:
			ret = RecvCrystalRefine(packet.get<GCCrystalRefinePacket>());
			break;

		case TGCHeader::CRYSTAL_REFINE_SUCCESS:
			ret = RecvCrystalRefineSuccess();
			break;

		case TGCHeader::CRYSTAL_USING_SLOT:
			ret = RecvCrystalUsingSlot(packet.get<GCCrystalUsingSlotPacket>());
			break;
#endif

		default:
			ret = RecvErrorPacket(packet);
			break;
	}
}

void CPythonNetworkStream::GamePhase()
{
	if (!m_kQue_stHack.empty())
	{
		__SendHack(m_kQue_stHack.front().c_str());
		m_kQue_stHack.pop_front();
	}

	InputPacket packet;
	bool ret = true;

#ifdef __PERFORMANCE_CHECK__
	DWORD timeBeginDispatch=timeGetTime();

	static std::map<DWORD, PERF_PacketInfo> kMap_kPacketInfo;
	kMap_kPacketInfo.clear();
#endif

#ifdef ENABLE_PYTHON_CONFIG
	const DWORD MAX_RECV_COUNT = 75;//CPythonConfig::Instance().GetInteger(CPythonConfig::CLASS_OPTION, "loading_recv_count", 75);
#else
	const DWORD MAX_RECV_COUNT = 4;
#endif
	const DWORD SAFE_RECV_BUFSIZE = 8192;
	DWORD dwRecvCount = 0;

    while (ret)
	{
		if(dwRecvCount++ >= MAX_RECV_COUNT-1 && GetRecvBufferSize() < SAFE_RECV_BUFSIZE
			&& m_strPhase == "Game") //phase_game ÀÌ ¾Æ´Ï¾îµµ ¿©±â·Î µé¾î¿À´Â °æ¿ì°¡ ÀÖ´Ù.
			break;

		if (!Recv(packet))
			break;

#ifdef __PERFORMANCE_CHECK__
		DWORD timeBeginPacket=timeGetTime();
#endif

		GamePhasePacket(packet, ret);

#ifdef __PERFORMANCE_CHECK__
		DWORD timeEndPacket=timeGetTime();

		{
			PERF_PacketInfo& rkPacketInfo=kMap_kPacketInfo[header];
			rkPacketInfo.dwCount++;
			rkPacketInfo.dwTime+=timeEndPacket-timeBeginPacket;			
		}

		{
			PERF_PacketInfo& rkPacketInfo=gs_kPacketTimeAnalyzer.m_kMap_kPacketInfo[header];
			rkPacketInfo.dwCount++;
			rkPacketInfo.dwTime+=timeEndPacket-timeBeginPacket;			
		}
#endif

		if (!ret)
			RecvErrorPacket(packet);

		if (packet.get_header<TGCHeader>() == TGCHeader::PHASE)
			break;
	}

#ifdef __PERFORMANCE_CHECK__
	DWORD timeEndDispatch=timeGetTime();
	
	if (timeEndDispatch-timeBeginDispatch>2)
	{
		static FILE* fp=fopen("perf_dispatch_packet.txt", "w");		

		fprintf(fp, "delay %d\n", timeEndDispatch-timeBeginDispatch);
		for (std::map<DWORD, PERF_PacketInfo>::iterator i=kMap_kPacketInfo.begin(); i!=kMap_kPacketInfo.end(); ++i)
		{
			if (i->second.dwTime>0)
				fprintf(fp, "header %d: count %d, time %d\n", i->first, i->second.dwCount, i->second.dwTime);
		}
		fputs("=====================================================\n", fp);
		fflush(fp);
	}
#endif

	static DWORD s_nextRefreshTime = ELTimer_GetMSec();

	DWORD curTime = ELTimer_GetMSec();
	if (s_nextRefreshTime > curTime)
		return;	

	if (m_isRefreshCharacterWnd)
	{
		m_isRefreshCharacterWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshCharacter", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshEquipmentWnd)
	{
		m_isRefreshEquipmentWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshEquipment", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshInventoryWnd)
	{
		m_isRefreshInventoryWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshInventory", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshExchangeWnd)
	{
		m_isRefreshExchangeWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshExchange", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshSkillWnd)
	{
		m_isRefreshSkillWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshSkill", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshSafeboxWnd)
	{
		m_isRefreshSafeboxWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshSafebox", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshMallWnd)
	{
		m_isRefreshMallWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshMall", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshStatus)
	{
		m_isRefreshStatus=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshStatus", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshMessengerWnd)
	{
		m_isRefreshMessengerWnd=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshMessenger", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndInfoPage)
	{
		m_isRefreshGuildWndInfoPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildInfoPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndBoardPage)
	{
		m_isRefreshGuildWndBoardPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildBoardPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndMemberPage)
	{
		m_isRefreshGuildWndMemberPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildMemberPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndMemberPageGradeComboBox)
	{
		m_isRefreshGuildWndMemberPageGradeComboBox=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildMemberPageGradeComboBox", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndMemberPageLastPlayed)
	{
		m_isRefreshGuildWndMemberPageLastPlayed=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildMemberPageLastPlayed", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 500;
	}

	if (m_isRefreshGuildWndSkillPage)
	{
		m_isRefreshGuildWndSkillPage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSkillPage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshGuildWndGradePage)
	{
		m_isRefreshGuildWndGradePage=false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGradePage", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	if (m_isRefreshAcceWnd)
	{
		m_isRefreshAcceWnd = false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshAcce", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}
#endif

#ifdef ENABLE_GUILD_SAFEBOX
	if (m_isRefreshGuildSafeboxWnd)
	{
		m_isRefreshGuildSafeboxWnd = false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSafebox", Py_BuildValue("()"));
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildSafeboxMoney", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}
#endif

#ifdef ENABLE_ANIMAL_SYSTEM
	for (int i = 0; i < ANIMAL_TYPE_MAX_NUM; ++i)
	{
		if (m_isRefreshAnimalWnd[i])
		{
			m_isRefreshAnimalWnd[i] = false;
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshAnimalWindow", Py_BuildValue("(i)", i));
			s_nextRefreshTime = curTime + 300;
		}
	}
#endif

#ifdef ENABLE_PET_ADVANCED
	if (m_isRefreshPetStatusWnd)
	{
		m_isRefreshPetStatusWnd = false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetRefreshStatus", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}

	if (m_isRefreshPetSkillWnd)
	{
		m_isRefreshPetSkillWnd = false;
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetRefreshSkill", Py_BuildValue("()"));
		s_nextRefreshTime = curTime + 300;
	}
#endif
}

void CPythonNetworkStream::__InitializeGamePhase()
{
	__ServerTimeSync_Initialize();

	m_isRefreshStatus=false;
	m_isRefreshCharacterWnd=false;
	m_isRefreshEquipmentWnd=false;
	m_isRefreshInventoryWnd=false;
	m_isRefreshExchangeWnd=false;
	m_isRefreshSkillWnd=false;
	m_isRefreshSafeboxWnd=false;
	m_isRefreshMallWnd=false;
	m_isRefreshMessengerWnd=false;
	m_isRefreshGuildWndInfoPage=false;
	m_isRefreshGuildWndBoardPage=false;
	m_isRefreshGuildWndMemberPage=false;
	m_isRefreshGuildWndMemberPageGradeComboBox=false;
	m_isRefreshGuildWndMemberPageLastPlayed=false;
	m_isRefreshGuildWndSkillPage=false;
	m_isRefreshGuildWndGradePage=false;
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	m_isRefreshAcceWnd=false;
#endif
#ifdef ENABLE_GUILD_SAFEBOX
	m_isRefreshGuildSafeboxWnd=false;
#endif
#ifdef ENABLE_ANIMAL_SYSTEM
	ZeroMemory(m_isRefreshAnimalWnd, sizeof(m_isRefreshAnimalWnd));
#endif
#ifdef ENABLE_PET_ADVANCED
	m_isRefreshPetStatusWnd = false;
	m_isRefreshPetSkillWnd = false;
#endif

	m_EmoticonStringVector.clear();

	m_pInstTarget = NULL;
}

void CPythonNetworkStream::Warp(long lGlobalX, long lGlobalY)
{
#ifdef _USE_LOG_FILE
	Tracenf("WarpTo %ld %ld", lGlobalX, lGlobalY);
#endif

	CPythonBackground& rkBgMgr=CPythonBackground::Instance();
	rkBgMgr.Destroy();
	rkBgMgr.Create();
	rkBgMgr.Warp(lGlobalX, lGlobalY);
	//rkBgMgr.SetShadowLevel(CPythonBackground::SHADOW_ALL);
	rkBgMgr.RefreshShadowLevel();

	// NOTE : Warp ÇßÀ»¶§ CenterPositionÀÇ Height°¡ 0ÀÌ±â ¶§¹®¿¡ Ä«¸Þ¶ó°¡ ¶¥¹Ù´Ú¿¡ ¹ÚÇôÀÖ°Ô µÊ
	//        ¿òÁ÷ÀÏ¶§¸¶´Ù Height°¡ °»½Å µÇ±â ¶§¹®ÀÌ¹Ç·Î ¸ÊÀ» ÀÌµ¿ÇÏ¸é PositionÀ» °­Á¦·Î ÇÑ¹ø
	//        ¼ÂÆÃÇØÁØ´Ù - [levites]
	long lLocalX = lGlobalX;
	long lLocalY = lGlobalY;
	__GlobalPositionToLocalPosition(lLocalX, lLocalY);
	float fHeight = CPythonBackground::Instance().GetHeight(float(lLocalX), float(lLocalY));

	IAbstractApplication& rkApp=IAbstractApplication::GetSingleton();
	rkApp.SetCenterPosition(float(lLocalX), float(lLocalY), fHeight);

	__ShowMapName(lLocalX, lLocalY);

#ifdef ENABLE_NEW_WEBBROWSER
#ifndef WEBBROWSER_OFFSCREEN
	CPythonNewWeb::instance().Hide(true);
#endif
#endif
}

void CPythonNetworkStream::__ShowMapName(long lLocalX, long lLocalY)
{
	const std::string & c_rstrMapFileName = CPythonBackground::Instance().GetWarpMapName();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "ShowMapName", Py_BuildValue("(sii)", c_rstrMapFileName.c_str(), lLocalX, lLocalY));
}

void CPythonNetworkStream::__LeaveGamePhase()
{
	CInstanceBase::ClearPVPKeySystem();

	__ClearNetworkActorManager();

	m_bComboSkillFlag = FALSE;

	IAbstractCharacterManager& rkChrMgr=IAbstractCharacterManager::GetSingleton();
	rkChrMgr.Destroy();

	CPythonItem& rkItemMgr=CPythonItem::Instance();
	rkItemMgr.Destroy();

	CPythonPlayer& rkPlayerMgr = CPythonPlayer::Instance();
	rkPlayerMgr.LeaveGamePhase();

#ifdef ENABLE_PET_ADVANCED
	CPythonPetAdvanced::Instance().Clear();
#endif

	CPythonApplication::Instance().ClearDynamicImagePtr(); // clear memory
}

void CPythonNetworkStream::SetGamePhase()
{
	if ("Game"!=m_strPhase)
		m_phaseLeaveFunc.Run();

#ifdef _USE_LOG_FILE
	Tracen("");
	Tracen("## Network - Game Phase ##");
	Tracen("");
#endif

	m_strPhase = "Game";

	m_dwChangingPhaseTime = ELTimer_GetMSec();
	m_phaseProcessFunc.Set(this, &CPythonNetworkStream::GamePhase);
	m_phaseLeaveFunc.Set(this, &CPythonNetworkStream::__LeaveGamePhase);

	// Main Character µî·ÏO

	IAbstractPlayer & rkPlayer = IAbstractPlayer::GetSingleton();
	rkPlayer.SetMainCharacterIndex(GetMainActorVID());

	__RefreshStatus();
}

bool CPythonNetworkStream::RecvObserverAddPacket(std::unique_ptr<GCObserverAddPacket> pack)
{
	CPythonMiniMap::Instance().AddObserver(
		pack->vid(), 
		pack->x()*100.0f, 
		pack->y()*100.0f);

	return true;
}

bool CPythonNetworkStream::RecvObserverRemovePacket(std::unique_ptr<GCObserverRemovePacket> pack)
{
	CPythonMiniMap::Instance().RemoveObserver(
		pack->vid()
	);

	return true;
}

bool CPythonNetworkStream::RecvObserverMovePacket(std::unique_ptr<GCObserverMovePacket> pack)
{
	CPythonMiniMap::Instance().MoveObserver(
		pack->vid(), 
		pack->x()*100.0f, 
		pack->y()*100.0f);

	return true;
}


bool CPythonNetworkStream::RecvWarpPacket(std::unique_ptr<GCWarpPacket> pack)
{
	m_dwLastWarpTime = time(0);

	__DirectEnterMode_Set(m_dwSelectedCharacterIndex);
	
	CNetworkStream::Connect((DWORD)pack->addr(), pack->port());

	return true;
}

bool CPythonNetworkStream::RecvDuelStartPacket(std::unique_ptr<GCDuelStartPacket> pack)
{
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();

	CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
	if (!pkInstMain)
	{
#ifdef _USE_LOG_FILE
		TraceError("CPythonNetworkStream::RecvDuelStartPacket - MainCharacter is NULL");
#endif
		return false;
	}
	DWORD dwVIDSrc = pkInstMain->GetVirtualID();

	for (DWORD dwVIDDest : pack->vids())
	{
		CInstanceBase::InsertDUELKey(dwVIDSrc, dwVIDDest);
	}
	
	if(pack->vids_size() == 0)
		pkInstMain->SetDuelMode(CInstanceBase::DUEL_CANNOTATTACK);
	else
		pkInstMain->SetDuelMode(CInstanceBase::DUEL_START);
	
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoard", Py_BuildValue("()"));
	
	rkChrMgr.RefreshAllPCTextTail();

	return true;
}

bool CPythonNetworkStream::RecvPVPPacket(std::unique_ptr<GCPVPPacket> pack)
{
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();
	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();

	switch (pack->mode())
	{
		case PVP_MODE_AGREE:
			rkChrMgr.RemovePVPKey(pack->vid_src(), pack->vid_dst());

			// »ó´ë°¡ ³ª(Dst)¿¡°Ô µ¿ÀÇ¸¦ ±¸ÇßÀ»¶§
			if (rkPlayer.IsMainCharacterIndex(pack->vid_dst()))
				rkPlayer.RememberChallengeInstance(pack->vid_src());

			// »ó´ë¿¡°Ô µ¿ÀÇ¸¦ ±¸ÇÑ µ¿¾È¿¡´Â ´ë°á ºÒ´É
			if (rkPlayer.IsMainCharacterIndex(pack->vid_src()))
				rkPlayer.RememberCantFightInstance(pack->vid_dst());
			break;
		case PVP_MODE_REVENGE:
		{
			rkChrMgr.RemovePVPKey(pack->vid_src(), pack->vid_dst());

			DWORD dwKiller = pack->vid_src();
			DWORD dwVictim = pack->vid_dst();

			// ³»(victim)°¡ »ó´ë¿¡°Ô º¹¼öÇÒ ¼ö ÀÖÀ»¶§
			if (rkPlayer.IsMainCharacterIndex(dwVictim))
				rkPlayer.RememberRevengeInstance(dwKiller);

			// »ó´ë(victim)°¡ ³ª¿¡°Ô º¹¼öÇÏ´Â µ¿¾È¿¡´Â ´ë°á ºÒ´É
			if (rkPlayer.IsMainCharacterIndex(dwKiller))
				rkPlayer.RememberCantFightInstance(dwVictim);
			break;
		}

		case PVP_MODE_FIGHT:
			rkChrMgr.InsertPVPKey(pack->vid_src(), pack->vid_dst());
			rkPlayer.ForgetInstance(pack->vid_src());
			rkPlayer.ForgetInstance(pack->vid_dst());
			break;
		case PVP_MODE_NONE:
			rkChrMgr.RemovePVPKey(pack->vid_src(), pack->vid_dst());
			rkPlayer.ForgetInstance(pack->vid_src());
			rkPlayer.ForgetInstance(pack->vid_dst());
			break;
	}

	// NOTE : PVP Åä±Û½Ã TargetBoard ¸¦ ¾÷µ¥ÀÌÆ® ÇÕ´Ï´Ù.
	__RefreshTargetBoardByVID(pack->vid_src());
	__RefreshTargetBoardByVID(pack->vid_dst());

	return true;
}

// DELETEME
/*
void CPythonNetworkStream::__SendWarpPacket()
{
	TPacketCGWarp kWarpPacket;
	kWarpPacket.bHeader=HEADER_GC_WARP;
	if (!Send(kWarpPacket))
	{
		return;
	}
}
*/
void CPythonNetworkStream::NotifyHack(const char* c_szMsg)
{
	if (!m_kQue_stHack.empty())
		if (c_szMsg==m_kQue_stHack.back())
			return;

	m_kQue_stHack.push_back(c_szMsg);	
}

bool CPythonNetworkStream::__SendHack(const char* c_szMsg)
{
#ifdef _USE_LOG_FILE
	Tracen(c_szMsg);
#endif
	
	CGOutputPacket<CGHackPacket> pack;
	pack->set_buf(c_szMsg);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendMessengerAddByVIDPacket(DWORD vid)
{
	CGOutputPacket<CGMessengerAddByVIDPacket> pack;
	pack->set_vid(vid);

	if (!Send(pack))
		return false;
	return true;
}

bool CPythonNetworkStream::SendMessengerAddByNamePacket(const char * c_szName)
{
	CGOutputPacket<CGMessengerAddByNamePacket> pack;
	pack->set_name(c_szName);
	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendMessengerRemovePacket(const char * c_szKey, const char * c_szName)
{
	CGOutputPacket<CGMessengerRemovePacket> pack;
	pack->set_name(c_szKey);
	if (!Send(pack))
		return false;

	__RefreshTargetBoardByName(c_szName);
	return true;
}

#ifdef INGAME_WIKI
extern PyObject* wikiModule;
bool CPythonNetworkStream::RecvWikiPacket(std::unique_ptr<GCWikiPacket> pack)
{
	CItemData* pData;
	if (!CItemManager::instance().GetItemDataPointer(pack->vnum(), &pData))
		return false;

	auto& recv_wiki = pack->wiki_info();

	CItemData::TWikiItemInfo* wikiInfo = pData->GetWikiTable();
	wikiInfo->isSet = true;
	wikiInfo->hasData = recv_wiki.refine_infos_size() > 0 || recv_wiki.chest_infos_size() > 0 || pack->origin_infos_size() > 0;
	wikiInfo->bIsCommon = recv_wiki.is_common();
	wikiInfo->dwOrigin = recv_wiki.origin_vnum();
	wikiInfo->pRefineData = recv_wiki.refine_infos();
	wikiInfo->pChestInfo = recv_wiki.chest_infos();
	wikiInfo->pOriginInfo = pack->origin_infos();

	PyCallClassMemberFunc(wikiModule, "BINARY_LoadInfo", Py_BuildValue("(Li)", (long long) pack->ret_id(), pack->vnum()));
	return true;
}

bool CPythonNetworkStream::RecvWikiMobPacket(std::unique_ptr<GCWikiMobPacket> pack)
{
	CPythonNonPlayer::TWikiInfoTable* mobData = CPythonNonPlayer::instance().GetWikiTable(pack->vnum());
	if (mobData)
	{
		mobData->isSet = true;
		for (DWORD mob : pack->mobs())
			mobData->dropList.push_back(mob);
	}

	PyCallClassMemberFunc(wikiModule, "BINARY_LoadInfo", Py_BuildValue("(Li)", (long long) pack->ret_id(), pack->vnum()));
	return true;
}
#endif

bool CPythonNetworkStream::RecvSoulRefineInfo(std::unique_ptr<GCSoulRefineInfoPacket> pack)
{
	PyObject* matTuple = PyTuple_New(pack->refine().material_count());
	for (int i = 0; i < pack->refine().material_count(); ++i)
		PyTuple_SetItem(matTuple, i, Py_BuildValue("(ii)", pack->refine().materials(i).vnum(), pack->refine().materials(i).count()));

	PyObject* applyValTuple = PyTuple_New(pack->apply_values_size());
	for (int i = 0; i < pack->apply_values_size(); ++i)
		PyTuple_SetItem(applyValTuple, i, Py_BuildValue("i", pack->apply_values(i)));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SoulRefineInfo",
		Py_BuildValue("(iiiOOL)", pack->vnum(), pack->type(), pack->apply_type(), applyValTuple, matTuple, (long long) pack->refine().cost()));
	return true;
}

#ifdef ENABLE_RUNE_SYSTEM
bool CPythonNetworkStream::RecvRune(std::unique_ptr<GCRunePacket> pack)
{
	CPythonRune::Instance().AddRune(pack->vnum());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RuneRefresh", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvRuneResetOwned()
{
	CPythonRune::Instance().ClearOwned();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RuneRefresh", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::RecvRunePage(std::unique_ptr<GCRunePagePacket> pack)
{
	CPythonRune::Instance().SetRunePage(&pack->data());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RuneRefresh", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvRuneRefine(std::unique_ptr<GCRuneRefinePacket> pack)
{
	PyObject* tuple = PyTuple_New(pack->refine_table().material_count());
	
	for (int i = 0; i < pack->refine_table().material_count(); ++i)
		PyTuple_SetItem(tuple, i, Py_BuildValue("(ii)", pack->refine_table().materials(i).vnum(), pack->refine_table().materials(i).count()));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RuneOpenRefine", Py_BuildValue("(OL)", tuple, (long long) pack->refine_table().cost()));

	return true;
}
#ifdef ENABLE_LEVEL2_RUNES
bool CPythonNetworkStream::RecvRuneLevelUp(std::unique_ptr<GCRuneLevelupPacket> pack)
{
	PyObject* tuple = PyTuple_New(pack->refine_table().material_count());

	for (int i = 0; i < pack->refine_table().material_count(); ++i)
		PyTuple_SetItem(tuple, i, Py_BuildValue("(ii)", pack->refine_table().materials(i).vnum(), pack->refine_table().materials(i).count()));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RuneOpenLevel", Py_BuildValue("(OLi)", tuple, (long long) pack->refine_table().cost(), pack->pos()));

	return true;
}
#endif
#endif

#ifdef ENABLE_MESSENGER_BLOCK
bool CPythonNetworkStream::SendMessengerAddBlockByVIDPacket(DWORD vid)
{
	CGOutputPacket<CGMessengerAddBlockByVIDPacket> pack;
	pack->set_vid(vid);
	if (!Send(pack))
		return false;
	return true;
}

bool CPythonNetworkStream::SendMessengerAddBlockByNamePacket(const char * c_szName)
{
	CGOutputPacket<CGMessengerAddBlockByNamePacket> pack;
	pack->set_name(c_szName);
	if (!Send(pack))
		return false;
	return true;
}

bool CPythonNetworkStream::SendMessengerRemoveBlockPacket(const char * c_szKey, const char * c_szName)
{
	CGOutputPacket<CGMessengerRemoveBlockPacket> pack;
	pack->set_name(c_szKey);
	if (!Send(pack))
		return false;
	__RefreshTargetBoardByName(c_szName);
	return true;
}
#endif

bool CPythonNetworkStream::SendCharacterStatePacket(const TPixelPosition& c_rkPPosDst, float fDstRot, UINT eFunc, UINT uArg)
{
	NANOBEGIN
	if (!__CanActMainInstance())
		return true;

	if (fDstRot < 0.0f)
		fDstRot = 360 + fDstRot;
	else if (fDstRot > 360.0f)
		fDstRot = fmodf(fDstRot, 360.0f);

	CGOutputPacket<CGMovePacket> pack;
	pack->set_func(eFunc);
	pack->set_arg(uArg);
	pack->set_rot(fDstRot / 5.0f);
	pack->set_x(c_rkPPosDst.x);
	pack->set_y(c_rkPPosDst.y);
	pack->set_time(ELTimer_GetServerMSec());
	
	assert(pack->x() >= 0 && pack->x() < 204800);

	long lX = pack->x(), lY = pack->y();
	__LocalPositionToGlobalPosition(lX, lY);
	pack->set_x(lX), pack->set_y(lY);

	if (!Send(pack))
		return false;

	NANOEND
	return true;
}

// NOTE : SlotIndex´Â ÀÓ½Ã
bool CPythonNetworkStream::SendUseSkillPacket(DWORD dwSkillIndex, DWORD dwTargetVID)
{
	CGOutputPacket<CGUseSkillPacket> pack;
	pack->set_vnum(dwSkillIndex);
	pack->set_vid(dwTargetVID);
	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendChatPacket(const char * c_szChat, BYTE byType)
{
	if (strlen(c_szChat) == 0)
		return true;

	if (strlen(c_szChat) >= 512)
		return true;

	if (c_szChat[0] == '/')
	{
		if (1 == strlen(c_szChat))
		{
			if (!m_strLastCommand.empty())
				c_szChat = m_strLastCommand.c_str();
		}
		else
		{
			m_strLastCommand = c_szChat;
		}
	}

	if (ClientCommand(c_szChat))
		return true;

	CGOutputPacket<CGChatPacket> pack;
	pack->set_type(byType);
	pack->set_message(c_szChat);
	if (!Send(pack))
		return false;

	return true;
}

//////////////////////////////////////////////////////////////////////////
// Emoticon
void CPythonNetworkStream::RegisterEmoticonString(const char * pcEmoticonString)
{
	if (m_EmoticonStringVector.size() >= CInstanceBase::EMOTICON_NUM)
	{
#ifdef _USE_LOG_FILE
		TraceError("Can't register emoticon string... vector is full (size:%d)", m_EmoticonStringVector.size() );
#endif
		return;
	}
	m_EmoticonStringVector.push_back(pcEmoticonString);
}

bool CPythonNetworkStream::ParseEmoticon(const char * pChatMsg, DWORD * pdwEmoticon)
{
	const char* c_pszStartStr = "|h|r : ";
	const char* c_pszStartPos = strstr(pChatMsg, c_pszStartStr);
	if (c_pszStartPos)
		pChatMsg = c_pszStartPos + strlen(c_pszStartStr);

	for (DWORD dwEmoticonIndex = 0; dwEmoticonIndex < m_EmoticonStringVector.size() ; ++dwEmoticonIndex)
	{
		if (strlen(pChatMsg) > m_EmoticonStringVector[dwEmoticonIndex].size())
			continue;

		const char * pcFind = strstr(pChatMsg, m_EmoticonStringVector[dwEmoticonIndex].c_str());

		if (pcFind != pChatMsg)
			continue;

		*pdwEmoticon = dwEmoticonIndex;

		return true;
	}

	return false;
}
// Emoticon
//////////////////////////////////////////////////////////////////////////

void CPythonNetworkStream::__ConvertEmpireText(DWORD dwEmpireID, char* szText)
{

}

bool CPythonNetworkStream::RecvChatPacket(std::unique_ptr<GCChatPacket> pack)
{
    char buf[1024 + 1];
	char line[1024 + 1];
	
	strcpy_s(buf, pack->message().c_str());
	
	// À¯·´ ¾Æ¶ø ¹öÀü Ã³¸®
	// "ÀÌ¸§: ³»¿ë" ÀÔ·ÂÀ» "³»¿ë: ÀÌ¸§" ¼ø¼­·Î Ãâ·ÂÇÏ±â À§ÇØ ÅÇ(0x08)À» ³ÖÀ½
	// ÅÇÀ» ¾Æ¶ø¾î ±âÈ£·Î Ã³¸®ÇØ (¿µ¾î1) : (¿µ¾î2) ·Î ÀÔ·ÂµÇ¾îµµ (¿µ¾î2) : (¿µ¾î1) ·Î Ãâ·ÂÇÏ°Ô ¸¸µç´Ù
	if (GetDefaultCodePage() == CP_ARABIC)
	{
		char * p = strchr(buf, ':'); 
		if (p && p[1] == ' ')
			p[1] = 0x08;
	}

	if (pack->type() >= CHAT_TYPE_MAX_NUM)
		return true;

	if (CHAT_TYPE_COMMAND == pack->type())
	{
		ServerCommand(buf);
		return true;
	}
#ifdef ENABLE_ZODIAC
	if (CHAT_TYPE_ZODIAC_NOTICE == pack->type())
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SetZodiacMessage", Py_BuildValue("(s)", buf));
		return true;
	}
#endif
	if (pack->id() != 0)
	{
		CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
		CInstanceBase * pkInstChatter = rkChrMgr.GetInstancePtr(pack->id());
		if (NULL == pkInstChatter)
			return true;
		
		switch (pack->type())
		{
		case CHAT_TYPE_TALKING:  /* ±×³É Ã¤ÆÃ */
		case CHAT_TYPE_PARTY:    /* ÆÄÆ¼¸» */
		case CHAT_TYPE_GUILD:    /* ±æµå¸» */
		case CHAT_TYPE_SHOUT:	/* ¿ÜÄ¡±â */
		case CHAT_TYPE_TEAM:
		case CHAT_TYPE_WHISPER:	// ¼­¹ö¿Í´Â ¿¬µ¿µÇÁö ¾Ê´Â Only Client Enum
			{
				char * p = strchr(buf, ':');

				if (p)
					p += 2;
				else
					p = buf;

				DWORD dwEmoticon;

				if (ParseEmoticon(p, &dwEmoticon))
				{
					pkInstChatter->SetEmoticon(dwEmoticon);
					return true;
				}
				else
				{
					if (gs_bEmpireLanuageEnable)
					{
						CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
						if (pkInstMain)
							if (!pkInstMain->IsSameEmpire(*pkInstChatter))
								__ConvertEmpireText(pkInstChatter->GetEmpireID(), p);
					}
					_snprintf(line, sizeof(line), "%s", p);
				}
			}
			break;
		case CHAT_TYPE_COMMAND:	/* ¸í·É */
		case CHAT_TYPE_INFO:     /* Á¤º¸ (¾ÆÀÌÅÛÀ» Áý¾ú´Ù, °æÇèÄ¡¸¦ ¾ò¾ú´Ù. µî) */
		case CHAT_TYPE_NOTICE:   /* °øÁö»çÇ× */
		case CHAT_TYPE_BIG_NOTICE:
		case CHAT_TYPE_MAX_NUM:
		default:
			_snprintf(line, sizeof(line), "%s", buf);
			break;
		}

		// replace item names by current selected language item names for alle sent item hyperlinks
		if (CHAT_TYPE_SHOUT != pack->type())
		{
			std::string str = line;
			std::size_t pos_0 = str.find(" : ");
			if (pos_0 != std::string::npos)
				str = str.substr(pos_0 + 3, str.length() - pos_0 - 3);

			std::string findl1 = "|Hitem:";
			std::size_t find1 = 0;

			std::size_t pos1 = 0;
			for (int i = 0; i < str.length(); i++)
			{
				find1 = str.find(findl1, i);;
				if (find1 != std::string::npos)
				{
					pos1 = find1 + findl1.length();
					i = find1 + findl1.length();
					std::string item_vnumabc = str.substr(pos1, str.length() - pos1);
					std::size_t pos2 = item_vnumabc.find(":");
					std::string item_vnum = item_vnumabc.substr(0, pos2);
					char* p, * s0;
					long n = strtol(item_vnum.c_str(), &p, 16);

					std::string item_flag = item_vnumabc.substr(pos2 + 1, strlen(item_vnumabc.c_str()));
					std::string item_socket = item_flag.substr(item_flag.find(":") + 1, strlen(item_flag.c_str()));
					item_socket = item_socket.substr(0, item_socket.find(":"));

					long socket0 = strtol(item_socket.c_str(), &s0, 16);

					if (*p != 0)
					{
#ifdef _USE_LOG_FILE
						TraceError("%s a number %s", item_vnum, str);
#endif
					}
					else
					{
						std::size_t pos_1 = item_vnumabc.find("|h[");
						std::size_t pos_2 = item_vnumabc.find("]|h");
						std::string item_name = item_vnumabc.substr(pos_1 + 3, pos_2 - pos_1 - 3);

						CItemData * pItemData = NULL;
						if (CItemManager::Instance().GetItemDataPointer(n, &pItemData))
						{
							if (strcmp(item_name.c_str(), pItemData->GetName()) != 0)
							{
								if (socket0 && n == 50300)
								{
									CPythonSkill::SSkillData* c_pSkillData;
									const char* szSkillName = pItemData->GetName();
									if (CPythonSkill::Instance().GetSkillData(socket0, &c_pSkillData))
										szSkillName = c_pSkillData->strName.c_str();
									str = str.replace(pos1 + pos_1 + 3, item_name.length(), szSkillName);
								}
								else
								{
									str = str.replace(pos1 + pos_1 + 3, item_name.length(), pItemData->GetName());
								}
							}
						}
					}
				}
			}
			
			CPythonTextTail::Instance().RegisterChatTail(pack->id(), str.c_str());
		}

		if (pkInstChatter->IsPC())
			CPythonChat::Instance().AppendChat(pack->type(), buf);
	}
	else
	{
		if (CHAT_TYPE_NOTICE == pack->type())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SetTipMessage", Py_BuildValue("(s)", buf));
		}
		else if (CHAT_TYPE_BIG_NOTICE == pack->type())
		{
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SetBigMessage", Py_BuildValue("(s)", buf));
		}
		CPythonChat::Instance().AppendChat(pack->type(), buf);
	}
	return true;
}

bool CPythonNetworkStream::RecvWhisperPacket(std::unique_ptr<GCWhisperPacket> pack)
{
    char buf[512 + 1];
	strcpy_s(buf, pack->message().c_str());

	static char line[256];
	if (CPythonChat::WHISPER_TYPE_NORMAL == pack->type() || CPythonChat::WHISPER_TYPE_GM == pack->type())
	{
		CPythonWhisperManager::Instance().OnRecvWhisper(pack->type(), pack->name_from().c_str(), buf);

		_snprintf(line, sizeof(line), "%s : %s", pack->name_from().c_str(), buf);
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnRecvWhisper", Py_BuildValue("(iss)", pack->type(), pack->name_from().c_str(), line));
	}
	else if (CPythonChat::WHISPER_TYPE_SYSTEM == pack->type() || CPythonChat::WHISPER_TYPE_ERROR == pack->type())
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnRecvWhisperSystemMessage",
			Py_BuildValue("(iss)", pack->type(), pack->name_from().c_str(), buf));
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnRecvWhisperError",
			Py_BuildValue("(iss)", pack->type(), pack->name_from().c_str(), buf));
	}

	return true;
}

bool CPythonNetworkStream::SendWhisperPacket(const char * name, const char * c_szChat, bool bSendOffline)
{
	if (strlen(c_szChat) >= 255)
		return true;

	CGOutputPacket<CGWhisperPacket> pack;
	pack->set_name_to(name);
	pack->set_message(c_szChat);
	pack->set_send_offline(bSendOffline);

	if (!Send(pack))
		return false;

	CPythonWhisperManager::Instance().OnSendWhisper(name, c_szChat);

	return true;
}

bool CPythonNetworkStream::RecvPointChange(std::unique_ptr<GCPointChangePacket> pack)
{
	CPythonCharacterManager& rkChrMgr = CPythonCharacterManager::Instance();
	rkChrMgr.ShowPointEffect(pack->type(), pack->vid());

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetMainInstancePtr();

	// ÀÚ½ÅÀÇ Point°¡ º¯°æµÇ¾úÀ» °æ¿ì..
	if (pInstance)
	{
		if (pack->vid() == pInstance->GetVirtualID())
		{
			CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
			rkPlayer.SetStatus(pack->type(), pack->value());

			switch (pack->type())
			{
			case POINT_STAT_RESET_COUNT:
				__RefreshStatus();
				break;
			case POINT_PLAYTIME:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_play_minutes(pack->value());
				break;
			case POINT_LEVEL:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_level(pack->value());
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_ST:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_st(pack->value());
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_DX:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_dx(pack->value());
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_HT:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_ht(pack->value());
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_IQ:
				m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_iq(pack->value());
				__RefreshStatus();
				__RefreshSkillWindow();
				break;
			case POINT_SKILL:
			case POINT_SUB_SKILL:
			case POINT_HORSE_SKILL:
				__RefreshSkillWindow();
				break;
			case POINT_ENERGY:
				if (pack->value() == 0)
				{
					rkPlayer.SetStatus(POINT_ENERGY_END_TIME, 0);
				}
				__RefreshStatus();
				break;
			default:
				__RefreshStatus();
				break;
			}

			if (POINT_GOLD == pack->type())
			{
				if (pack->amount() > 0)
				{
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnPickMoney", Py_BuildValue("(L)", (long long) pack->amount()));
				}
			}

#ifdef ENABLE_ERROR_DETECTION
			if (pack->type() == POINT_HP || pack->type() == POINT_MAX_HP)
			{
				long long llHP = rkPlayer.GetStatus(POINT_HP);
				long long llMaxHP = rkPlayer.GetStatus(POINT_MAX_HP);
				if (llHP < -9000000000 && llMaxHP <= 0)
				{
					CGOutputPacket<CGForcedRewarpPacket> pack_send;
					pack_send->set_checkval(404);
					pack_send->set_detail_log(std::string("invalid HP ") + std::to_string(llHP) + " MaxHP " + std::to_string(llMaxHP));
					if (!Send(pack_send))
						return false;
				}
			}
#endif
		}
	}

	if ((pInstance && pInstance->GetVirtualID() == pack->vid()) || (pInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid())))
	{
		switch (pack->type())
		{
		case POINT_LEVEL:
			pInstance->UpdateTextTailLevel(pack->value());
			break;
		}

#ifdef ENABLE_ERROR_DETECTION
		if (pack->type() == POINT_LEVEL && pack->value() <= 0)
		{
			CGOutputPacket<CGForcedRewarpPacket> pack_send;
			pack_send->set_checkval(404);
			pack_send->set_detail_log(std::string("invalid Level ") + std::to_string(pack->value()));
			if (!Send(pack_send))
				return false;
		}
#endif
	}

	return true;
}

bool CPythonNetworkStream::RecvRealPointSet(std::unique_ptr<GCRealPointSetPacket> pack)
{
	CPythonPlayer::Instance().SetRealStatus(pack->type(), pack->value());
	__RefreshStatus();

	return true;
}

bool CPythonNetworkStream::RecvStunPacket(std::unique_ptr<GCStunPacket> pack)
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase * pkInstSel = rkChrMgr.GetInstancePtr(pack->vid());

	if (pkInstSel)
	{
		if (CPythonCharacterManager::Instance().GetMainInstancePtr()==pkInstSel)
			pkInstSel->Die();
		else
			pkInstSel->Stun();
	}

	return true;
}

bool CPythonNetworkStream::RecvDeadPacket(std::unique_ptr<GCDeadPacket> pack)
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase * pkChrInstSel = rkChrMgr.GetInstancePtr(pack->vid());
	if (pkChrInstSel)
	{
		CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
		if (pkInstMain==pkChrInstSel)
		{

#ifdef SKILL_AFFECT_DEATH_REMAIN
			if (pack->killer_is_pc())
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SetKeepSkillAffectIcons", Py_BuildValue("()"));
#endif
			if (!pkInstMain->GetDuelMode())
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnGameOver", Py_BuildValue("()"));

			CPythonPlayer::Instance().NotifyDeadMainCharacter();
		}

		pkChrInstSel->Die();
	}

	return true;
}

bool CPythonNetworkStream::SendOnClickPacket(DWORD vid)
{
	CGOutputPacket<CGOnClickPacket> pack;
	pack->set_vid(vid);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvMotionPacket(std::unique_ptr<GCMotionPacket> pack)
{
	CInstanceBase * pMainInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	CInstanceBase * pVictimInstance = NULL;

	if (0 != pack->victim_vid())
		pVictimInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->victim_vid());

	if (!pMainInstance)
		return false;

	return true;
}

bool CPythonNetworkStream::RecvShopPacket(const InputPacket& packet)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::SHOP_START:
			{
				auto pack = packet.get<GCShopStartPacket>();

				CPythonShop::Instance().Clear();

				for (auto i = 0; i < pack->items_size(); ++i)
					CPythonShop::Instance().SetItemData(pack->items(i).display_pos(), pack->items(i));

#ifdef COMBAT_ZONE
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartShop", Py_BuildValue("(iiii)", pack->vid(), 0, 0, 0));
#else
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartShop", Py_BuildValue("(i)", pack->vid()));
#endif
			}
			break;

		case TGCHeader::SHOP_EX_START:
			{
				auto pack = packet.get<GCShopExStartPacket>();

				CPythonShop::Instance().Clear();

				CPythonShop::instance().SetTabCount(pack->tabs_size());

				for (auto i = 0; i < pack->tabs_size(); ++i)
				{
					auto& tab = pack->tabs(i);

					CPythonShop::instance().SetTabCoinType(i, tab.coin_type());
					CPythonShop::instance().SetTabName(i, tab.name().c_str());

					for (auto j = 0; j < tab.items_size(); ++j)
						CPythonShop::Instance().SetItemData(i, j, tab.items(j));
				}

#ifdef COMBAT_ZONE
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartShop",
					Py_BuildValue("(i)", pack->vid(), pack->points(), pack->cur_limit(), pack->max_limit()));
#else
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartShop", Py_BuildValue("(i)", pack->vid()));
#endif
			}
			break;


		case TGCHeader::SHOP_END:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "EndShop", Py_BuildValue("()"));
			break;

		case TGCHeader::SHOP_UPDATE_ITEM:
			{
				auto pack = packet.get<GCShopUpdateItemPacket>();
				CPythonShop::Instance().SetItemData(pack->item().display_pos(), pack->item());
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshShop", Py_BuildValue("()"));
			}
			break;

		case TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY"));
			break;

		case TGCHeader::SHOP_ERROR_NOT_ENOUGH_MONEY_EX:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_MONEY_EX"));
			break;

		case TGCHeader::SHOP_ERROR_SOLDOUT:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "SOLDOUT"));
			break;

		case TGCHeader::SHOP_ERROR_INVENTORY_FULL:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "INVENTORY_FULL"));
			break;

		case TGCHeader::SHOP_ERROR_INVALID_POS:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "INVALID_POS"));
			break;

#ifdef COMBAT_ZONE
		case TGCHeader::SHOP_ERROR_NOT_ENOUGH_POINTS:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "NOT_ENOUGH_POINTS"));
			break;

		case TGCHeader::SHOP_ERROR_MAX_LIMIT_POINTS:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "MAX_LIMIT_POINTS"));
			break;

		case TGCHeader::SHOP_ERROR_OVERFLOW_LIMIT_POINTS:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "OVERFLOW_LIMIT_POINTS"));
			break;
#endif
#ifdef ENABLE_ZODIAC
		case TGCHeader::SHOP_ERROR_ZODIAC_SHOP:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnShopError", Py_BuildValue("(s)", "ZODIAC_SHOP"));
			break;
#endif

		default:
			TraceError("CPythonNetworkStream::RecvShopPacket: Unknown subheader\n");
			break;
	}

	return true;
}

bool CPythonNetworkStream::RecvExchangePacket(const InputPacket& packet)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::EXCHANGE_START:
			{
				auto pack = packet.get<GCExchangeStartPacket>();

				CPythonExchange::Instance().Clear();
				CPythonExchange::Instance().Start();
				CPythonExchange::Instance().SetSelfName(CPythonPlayer::Instance().GetName());

				{
					CInstanceBase * pCharacterInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->target_vid());

					if (pCharacterInstance)
					{
						CPythonExchange::Instance().SetTargetName(pCharacterInstance->GetNameString());
					}
				}

				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartExchange", Py_BuildValue("()"));
			}
			break;

		case TGCHeader::EXCHANGE_ITEM_ADD:
			{
				auto pack = packet.get<GCExchangeItemAddPacket>();

				if (pack->is_me())
				{
					CPythonExchange::Instance().SetItemToSelf(pack->display_pos(), pack->data());

					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnExchangeAddItem",
						Py_BuildValue("(ii)", pack->data().cell().window_type(), pack->data().cell().cell()));
				}
				else
				{
					CPythonExchange::Instance().SetItemToTarget(pack->display_pos(), pack->data());
				}

				__RefreshExchangeWindow();
				__RefreshInventoryWindow();
			}
			break;

		case TGCHeader::EXCHANGE_ITEM_DEL:
			{
				auto pack = packet.get<GCExchangeItemDelPacket>();

				if (pack->is_me())
				{
					CPythonExchange::Instance().DelItemOfSelf(pack->display_pos());

					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnExchangeDelItem",
						Py_BuildValue("(ii)", pack->inventory_pos().window_type(), pack->inventory_pos().cell()));
				}
				else
				{
					CPythonExchange::Instance().DelItemOfTarget(pack->display_pos());
				}
				__RefreshExchangeWindow();
				__RefreshInventoryWindow();
			}
			break;

		case TGCHeader::EXCHANGE_GOLD_ADD:
			{
				auto pack = packet.get<GCExchangeGoldAddPacket>();
			
				if (pack->is_me())
					CPythonExchange::Instance().SetElkToSelf(pack->gold());
				else
					CPythonExchange::Instance().SetElkToTarget(pack->gold());

				__RefreshExchangeWindow();
			}
			break;

		case TGCHeader::EXCHANGE_ACCEPT:
			{
				auto pack = packet.get<GCExchangeAcceptPacket>();

				if (pack->is_me())
				{
					CPythonExchange::Instance().SetAcceptToSelf(pack->accept());
				}
				else
				{
					CPythonExchange::Instance().SetAcceptToTarget(pack->accept());
				}
				__RefreshExchangeWindow();
			}
			break;

		case TGCHeader::EXCHANGE_CANCEL:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "EndExchange", Py_BuildValue("()"));
			__RefreshInventoryWindow();
			CPythonExchange::Instance().End();
			break;
	};

	return true;
}

bool CPythonNetworkStream::RecvQuestInfoPacket(std::unique_ptr<GCQuestInfoPacket> pack)
{
	auto flag = pack->flag();

	enum
	{
		QUEST_PACKET_TYPE_NONE,
		QUEST_PACKET_TYPE_BEGIN,
		QUEST_PACKET_TYPE_UPDATE,
		QUEST_PACKET_TYPE_END,
	};

	BYTE byQuestPacketType = QUEST_PACKET_TYPE_NONE;

	if (0 != (flag & QUEST_SEND_IS_BEGIN))
	{
		if (pack->is_begin())
			byQuestPacketType = QUEST_PACKET_TYPE_BEGIN;
		else
			byQuestPacketType = QUEST_PACKET_TYPE_END;
	}
	else
	{
		byQuestPacketType = QUEST_PACKET_TYPE_UPDATE;
	}

	CPythonQuest& rkQuest=CPythonQuest::Instance();

	bool bRefreshCategories = false;

	// Process Start
	if (QUEST_PACKET_TYPE_END == byQuestPacketType)
	{
		bRefreshCategories = true;
		rkQuest.DeleteQuestInstance(pack->index());
	}
	else if (QUEST_PACKET_TYPE_UPDATE == byQuestPacketType)
	{
		if (!rkQuest.IsQuest(pack->index()))
		{
			bRefreshCategories = true;
			rkQuest.MakeQuest(pack->index());
		}

		if (IS_SET(flag, QUEST_SEND_TITLE))
			rkQuest.SetQuestTitle(pack->index(), pack->title().c_str());
#ifdef ENABLE_QUEST_CATEGORIES
		if (IS_SET(flag, QUEST_SEND_CATEGORY_ID))
			rkQuest.SetQuestCatId(pack->index(), pack->cat_id());
#endif
		if (IS_SET(flag, QUEST_SEND_CLOCK_NAME))
			rkQuest.SetQuestClockName(pack->index(), pack->clock_name().c_str());
		if (IS_SET(flag, QUEST_SEND_CLOCK_VALUE))
			rkQuest.SetQuestClockValue(pack->index(), pack->clock_value());
		if (IS_SET(flag, QUEST_SEND_COUNTER))
			rkQuest.SetQuestCounter(pack->index(), 0, pack->counter_name().c_str(), pack->counter_value());
		if (IS_SET(flag, QUEST_SEND_ICON_FILE))
			rkQuest.SetQuestIconFileName(pack->index(), pack->icon_file_name().c_str());
	}
	else if (QUEST_PACKET_TYPE_BEGIN == byQuestPacketType)
	{
		bRefreshCategories = true;

		CPythonQuest::SQuestInstance QuestInstance;
		QuestInstance.dwIndex = pack->index();
		QuestInstance.strTitle = pack->title();
#ifdef ENABLE_QUEST_CATEGORIES
		QuestInstance.iQuestCatId = pack->cat_id();
#endif
		QuestInstance.strClockName = pack->clock_name();
		QuestInstance.iClockValue = pack->clock_value();
		if (!pack->counter_name().empty())
		{
			QuestInstance.map_CounterName[0] = pack->counter_name();
			QuestInstance.map_CounterValue[0] = pack->counter_value();
		}
		QuestInstance.strIconFileName = pack->icon_file_name();

		CPythonQuest::Instance().RegisterQuestInstance(QuestInstance);
	}
	// Process Start End

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshQuest", Py_BuildValue("(b)", bRefreshCategories));
	return true;
}

bool CPythonNetworkStream::RecvQuestConfirmPacket(std::unique_ptr<GCQuestConfirmPacket> pack)
{
	PyObject * poArg = Py_BuildValue("(sii)", pack->message().c_str(), pack->timeout(), pack->request_pid());
 	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_OnQuestConfirm", poArg);
	return true;
}

/*
bool CPythonNetworkStream::RecvQuestDeletePacket()
{
	TPacketGCQuestDelete packet;
	if (!Recv(&packet))
		return false;

	CPythonQuest::Instance().DeleteQuestInstance(packet.quest_index);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshQuest", Py_BuildValue("(b)", true));

	CPythonEventManager::Instance().HideQuestLetter(packet.quest_index);
	return true;
}
*/

bool CPythonNetworkStream::RecvRequestMakeGuild()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "AskGuildName", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::SendExchangeStartPacket(DWORD vid)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGExchangeStartPacket> pack;
	pack->set_other_vid(vid);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracef("send_trade_start_packet Error\n");
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	// Tracef("send_trade_start_packet   vid %d \n", vid);
#endif
	return true;
}

bool CPythonNetworkStream::SendExchangeElkAddPacket(long long elk)
{
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGExchangeGoldAddPacket> pack;
	pack->set_gold(elk);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracef("send_trade_elk_add_packet Error\n");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeItemAddPacket(::TItemPos ItemPos, BYTE byDisplayPos)
{
	// TraceError("ExchangeItemAddPacket Start");
	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGExchangeItemAddPacket> pack;
	*pack->mutable_cell() = ItemPos;
	pack->set_display_pos(byDisplayPos);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracef("send_trade_item_add_packet Error\n");
#endif
		return false;
	}
	// TraceError("Sent");

	return true;
}

bool CPythonNetworkStream::SendExchangeItemDelPacket(BYTE pos)
{
	assert(!"Can't be called function - CPythonNetworkStream::SendExchangeItemDelPacket");
	return true;

	if (!__CanActMainInstance())
		return true;

	CGOutputPacket<CGExchangeItemDelPacket> pack;
	pack->set_display_pos(pos);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracef("send_trade_item_del_packet Error\n");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeAcceptPacket()
{
	if (!__CanActMainInstance())
		return true;
	
	if (!Send(TCGHeader::EXCHANGE_ACCEPT))
	{
#ifdef _USE_LOG_FILE
		Tracef("send_trade_accept_packet Error\n");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendExchangeExitPacket()
{
	if (!__CanActMainInstance())
		return true;

	if (!Send(TCGHeader::EXCHANGE_CANCEL))
	{
#ifdef _USE_LOG_FILE
		Tracef("send_trade_exit_packet Error\n");
#endif
		return false;
	}

	return true;
}

// PointReset °³ÀÓ½Ã
bool CPythonNetworkStream::SendPointResetPacket()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "StartPointReset", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::__IsPlayerAttacking()
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
	if (!pkInstMain)
		return false;

	if (!pkInstMain->IsAttacking())
		return false;

	return true;
}

bool CPythonNetworkStream::RecvScriptPacket(std::unique_ptr<GCScriptPacket> pack)
{
	if (pack->script().compare(0, 13, "[DESTROY_ALL]") == 0)
	{
		CPythonNetworkStream::Instance().HideQuestWindows();
		return true;
	}
	
	int iIndex = CPythonEventManager::Instance().RegisterEventSetFromString(pack->script());

	if (-1 != iIndex)
	{
		CPythonEventManager::Instance().SetVisibleLineCount(iIndex, 30);
		CPythonNetworkStream::Instance().OnScriptEventStart(pack->skin(), iIndex);
	}

	return true;
}

bool CPythonNetworkStream::SendScriptAnswerPacket(int iAnswer)
{
	CGOutputPacket<CGScriptAnswerPacket> pack;
	pack->set_answer(iAnswer);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("Send Script Answer Packet Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendScriptButtonPacket(unsigned int iIndex)
{
	CGOutputPacket<CGScriptButtonPacket> pack;
	pack->set_index(iIndex);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("Send Script Button Packet Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendAnswerMakeGuildPacket(const char * c_szName)
{
	CGOutputPacket<CGGuildAnswerMakePacket> pack;
	pack->set_guild_name(c_szName);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendAnswerMakeGuild Packet Error");
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
// 	Tracef(" SendAnswerMakeGuildPacket : %s", c_szName);
#endif
	return true;
}

bool CPythonNetworkStream::SendQuestInputStringPacket(const char * c_szString)
{
	CGOutputPacket<CGQuestInputStringPacket> pack;
	pack->set_message(c_szString);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendQuestInputStringPacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendQuestConfirmPacket(BYTE byAnswer, DWORD dwPID)
{
	CGOutputPacket<CGQuestConfirmPacket> pack;
	pack->set_answer(byAnswer);
	pack->set_request_pid(dwPID);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendQuestConfirmPacket Error");
#endif
		return false;
	}

	Tracenf(" SendQuestConfirmPacket : %d, %d", byAnswer, dwPID);
	return true;
}

bool CPythonNetworkStream::RecvSkillLevel(std::unique_ptr<GCSkillLevelPacket> pack)
{
	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();

	rkPlayer.SetSkill(7, 0);
	rkPlayer.SetSkill(8, 0);

	for (int i = 0; i < MIN(SKILL_MAX_NUM, pack->levels_size()); ++i)
	{
		auto& rPlayerSkill = pack->levels(i);

		if (i >= 112 && i <= 115 && rPlayerSkill.level())
			rkPlayer.SetSkill(7, i);

		if (i >= 116 && i <= 119 && rPlayerSkill.level())
			rkPlayer.SetSkill(8, i);

		rkPlayer.SetSkillLevel_(i, rPlayerSkill.master_type(), rPlayerSkill.level());
	}

	__RefreshSkillWindow();
	__RefreshStatus();
#ifdef _USE_LOG_FILE
	//Tracef(" >> RecvSkillLevelNew\n");
#endif
	return true;
}

bool CPythonNetworkStream::RecvDamageInfoPacket(std::unique_ptr<GCDamageInfoPacket> pack)
{
	CInstanceBase * pInstTarget = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	bool bSelf = (pInstTarget == CPythonCharacterManager::Instance().GetMainInstancePtr());
	bool bTarget = (pInstTarget==m_pInstTarget);
	if (pInstTarget)
	{
		if(pack->damage() >= 0)
			pInstTarget->AddDamageEffect(pack->damage(),pack->flag(),bSelf,bTarget);
		else
			TraceError("Damage is equal or below 0.");
	}

#ifdef TARGET_DMG_VID
	CInstanceBase * pInstAttacker = CPythonCharacterManager::Instance().GetInstancePtr(pack->target_vid());
	if (!pInstAttacker && bSelf)
	{
		static float lastTimeCommandSent = 0;

		if (lastTimeCommandSent < CPythonApplication::Instance().GetGlobalTime())
		{
			lastTimeCommandSent = CPythonApplication::Instance().GetGlobalTime() + 5;
			SendChatPacket("/reload_environment");
		}
	}
#endif

	return true;
}

bool CPythonNetworkStream::RecvTargetPacket(std::unique_ptr<GCTargetPacket> pack)
{
	CInstanceBase * pInstPlayer = CPythonCharacterManager::Instance().GetMainInstancePtr();
	CInstanceBase * pInstTarget = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	if (pInstPlayer && pInstTarget)
	{
		if (!pInstTarget->IsDead())
		{
#if defined(COMBAT_ZONE_HIDE_INFO_USER)
			if (pInstPlayer->IsCombatZoneMap() || pInstTarget->IsCombatZoneMap())
			{
				m_pInstTarget = pInstTarget;
				return true;
			}
#endif
#ifdef NEW_TARGET_UI
			if (pInstTarget->IsPC() || pInstTarget->IsBuilding())
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenTargetNew",
					Py_BuildValue("(iiiii)", pack->vid(), pack->hppercent(), pInstTarget->IsPC(), pack->cur_hp(), pack->max_hp()));
#else
			if (pInstTarget->IsPC() && !pInstPlayer->CanViewTargetHP(*pInstTarget) || pInstTarget->IsBuilding())
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoardIfDifferent", Py_BuildValue("(i)", pack->vid()));
#endif
			else if (pInstPlayer->CanViewTargetHP(*pInstTarget))
#ifdef __ELEMENT_SYSTEM__
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetHPTargetBoard",
					Py_BuildValue("(iii)", pack->vid(), pack->hppercent(), pack->element()));
#else
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetHPTargetBoard",
					Py_BuildValue("(ii)", pack->vid(), pack->hppercent()));
#endif
			else
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoard", Py_BuildValue("()"));	
			m_pInstTarget = pInstTarget;
		}
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "CloseTargetBoard", Py_BuildValue("()"));
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Recv

bool CPythonNetworkStream::SendHitSpacebarPacket()
{
/*	TPacketCGHitSpacebar kPacketHitSpacebar;
	kPacketHitSpacebar.bHeader=HEADER_CG_HIT_SPACEBAR;
	kPacketHitSpacebar.sLevel=__GetLevel() * 3;
	kPacketHitSpacebar.dSpam=__GetLevel();

	if (!Send(kPacketHitSpacebar))
		return false;*/

	if (!Send(TCGHeader::ON_HIT_SPACEBAR))
		return false;

	return true;
}

bool CPythonNetworkStream::SendQuestTriggerPacket(DWORD dwIndex, DWORD dwArg)
{
	CGOutputPacket<CGOnQuestTriggerPacket> pack;
	pack->set_index(dwIndex);
	pack->set_arg(dwArg);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAttackPacket(UINT uMotAttack, DWORD dwVIDVictim)
{
	if (!__CanActMainInstance())
		return true;

#ifdef ATTACK_TIME_LOG
	static DWORD prevTime = timeGetTime();
	DWORD curTime = timeGetTime();
	TraceError("TIME: %.4f(%.4f) ATTACK_PACKET: %d TARGET: %d", curTime / 1000.0f, (curTime - prevTime) / 1000.0f, uMotAttack, dwVIDVictim);
	prevTime = curTime;
#endif

#ifdef ENABLE_HIDDEN_ATTACK_REPORT
	// Anti cheat https://metin2zone.net/index.php?/topic/21951-c-anti-hack-mob-y-waithack/
	if (!__IsPlayerAttacking())
	{
		CPythonNetworkStream::instance().SendOnClickPacketNew(CPythonNetworkStream::ON_CLICK_ERROR_ID_HIDDEN_ATTACK);
		closeF = random_range(100, 240);
		return true;
	}
#endif

	CGOutputPacket<CGAttackPacket> pack;
	pack->set_type(uMotAttack);
	pack->set_random(CPythonApplication::Instance().GetRandom() + 4);
	pack->set_vid(dwVIDVictim);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("Send Battle Attack Packet Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvAddFlyTargetingPacket(std::unique_ptr<GCAddFlyTargetingPacket> pack)
{
	long lX = pack->x(), lY = pack->y();
	__GlobalPositionToLocalPosition(lX, lY);
	pack->set_x(lX);
	pack->set_y(lY);

#ifdef _USE_LOG_FILE
	Tracef("VID [%d]°¡ Å¸°ÙÀ» Ãß°¡ ¼³Á¤\n",pack->shooter_vid());
#endif

	CPythonCharacterManager & rpcm = CPythonCharacterManager::Instance();

	CInstanceBase * pShooter = rpcm.GetInstancePtr(pack->shooter_vid());

	if (!pShooter)
	{
#ifdef _USE_LOG_FILE
		TraceError("CPythonNetworkStream::RecvFlyTargetingPacket() - dwShooterVID[%d] NOT EXIST", pack->shooter_vid());
#endif
		return true;
	}

	CInstanceBase * pTarget = rpcm.GetInstancePtr(pack->target_vid());

	if (pack->target_vid() && pTarget)
	{
		pShooter->GetGraphicThingInstancePtr()->AddFlyTarget(pTarget->GetGraphicThingInstancePtr());
	}
	else
	{
		float h = CPythonBackground::Instance().GetHeight(pack->x(),pack->y()) + 60.0f; // TEMPORARY HEIGHT
		pShooter->GetGraphicThingInstancePtr()->AddFlyTarget(D3DXVECTOR3(pack->x(),pack->y(),h));
		//pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(kPacket.kPPosTarget.x,kPacket.kPPosTarget.y,);
	}

	return true;
}

bool CPythonNetworkStream::RecvFlyTargetingPacket(std::unique_ptr<GCFlyTargetingPacket> pack)
{
	long lX = pack->x(), lY = pack->y();
	__GlobalPositionToLocalPosition(lX, lY);
	pack->set_x(lX);
	pack->set_y(lY);

#ifdef _USE_LOG_FILE
	//Tracef("CPythonNetworkStream::RecvFlyTargetingPacket - VID [%d]\n",kPacket.dwShooterVID);
#endif

	CPythonCharacterManager & rpcm = CPythonCharacterManager::Instance();

	CInstanceBase * pShooter = rpcm.GetInstancePtr(pack->shooter_vid());

	if (!pShooter)
	{
#ifdef _USE_LOG_FILE
		TraceError("CPythonNetworkStream::RecvFlyTargetingPacket() - dwShooterVID[%d] NOT EXIST", pack->shooter_vid());
#endif
		return true;
	}

	CInstanceBase * pTarget = rpcm.GetInstancePtr(pack->target_vid());

	if (pack->target_vid() && pTarget)
	{
		pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(pTarget->GetGraphicThingInstancePtr());
	}
	else
	{
		float h = CPythonBackground::Instance().GetHeight(pack->x(), pack->y()) + 60.0f; // TEMPORARY HEIGHT
		pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(D3DXVECTOR3(pack->x(),pack->y(),h));
		//pShooter->GetGraphicThingInstancePtr()->SetFlyTarget(kPacket.kPPosTarget.x,kPacket.kPPosTarget.y,);
	}

	return true;
}

bool CPythonNetworkStream::SendShootPacket(UINT uSkill)
{
	CGOutputPacket<CGShootPacket> pack;
	pack->set_type(uSkill);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("SendShootPacket Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendAddFlyTargetingPacket(DWORD dwTargetVID, const TPixelPosition & kPPosTarget)
{
	CGOutputPacket<CGAddFlyTargetPacket> pack;

	pack->set_target_vid(dwTargetVID);

	long lX = kPPosTarget.x, lY = kPPosTarget.y;
	__LocalPositionToGlobalPosition(lX, lY);
	pack->set_x(lX);
	pack->set_y(lY);
	
	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("Send FlyTargeting Packet Error");
#endif
		return false;
	}

	return true;
}


bool CPythonNetworkStream::SendFlyTargetingPacket(DWORD dwTargetVID, const TPixelPosition & kPPosTarget)
{
	CGOutputPacket<CGFlyTargetPacket> pack;

	pack->set_target_vid(dwTargetVID);

	long lX = kPPosTarget.x, lY = kPPosTarget.y;
	__LocalPositionToGlobalPosition(lX, lY);
	pack->set_x(lX);
	pack->set_y(lY);
	
	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("Send FlyTargeting Packet Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvCreateFlyPacket(std::unique_ptr<GCCreateFlyPacket> pack)
{
	CFlyingManager& rkFlyMgr = CFlyingManager::Instance();
	CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();

	CInstanceBase * pkStartInst = rkChrMgr.GetInstancePtr(pack->start_vid());
	CInstanceBase * pkEndInst = rkChrMgr.GetInstancePtr(pack->end_vid());
	if (!pkStartInst || !pkEndInst)
		return true;

	rkFlyMgr.CreateIndexedFly(pack->type(), pkStartInst->GetGraphicThingInstancePtr(), pkEndInst->GetGraphicThingInstancePtr());

	return true;
}

bool CPythonNetworkStream::SendTargetPacket(DWORD dwVID)
{
	CGOutputPacket<CGTargetPacket> pack;
	pack->set_vid(dwVID);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("Send Target Packet Error");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::SendSyncPositionElementPacket(DWORD dwVictimVID, DWORD dwVictimX, DWORD dwVictimY)
{
	CGOutputPacket<CGSyncPositionPacket> pack;
	auto elem = pack->add_elements();

	elem->set_vid(dwVictimVID);

	long lX = dwVictimX, lY = dwVictimY;
	__LocalPositionToGlobalPosition(lX, lY);
	elem->set_x(lX);
	elem->set_y(lY);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("CPythonNetworkStream::SendSyncPositionElementPacket - ERROR");
#endif
		return false;
	}

	return true;
}

bool CPythonNetworkStream::RecvMessenger(const InputPacket& packet)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::MESSENGER_LIST:
		{
			auto pack = packet.get<GCMessengerListPacket>();
			for (auto& elem : pack->players())
			{
				if (elem.connected() & MESSENGER_CONNECTED_STATE_ONLINE)
					CPythonMessenger::Instance().OnFriendLogin(elem.name().c_str());
				else
					CPythonMessenger::Instance().OnFriendLogout(elem.name().c_str());

				if (elem.connected() & MESSENGER_CONNECTED_STATE_MOBILE)
					CPythonMessenger::Instance().SetMobile(elem.name().c_str(), TRUE);
			}
			break;
		}

		#ifdef ENABLE_MESSENGER_BLOCK
		case TGCHeader::MESSENGER_BLOCK_LIST:
		{
			auto pack = packet.get<GCMessengerBlockListPacket>();
			for (auto& elem : pack->players())
			{
				if (elem.connected() & MESSENGER_CONNECTED_STATE_ONLINE)
					CPythonMessenger::Instance().OnBlockLogin(elem.name().c_str());
				else
					CPythonMessenger::Instance().OnBlockLogout(elem.name().c_str());
			}
			break;
		}
		
		case TGCHeader::MESSENGER_BLOCK_LOGIN:
		{
			auto pack = packet.get<GCMessengerBlockLoginPacket>();
			CPythonMessenger::Instance().OnBlockLogin(pack->name().c_str());
			__RefreshTargetBoardByName(pack->name().c_str());
			break;
		}

		case TGCHeader::MESSENGER_BLOCK_LOGOUT:
		{
			auto pack = packet.get<GCMessengerLogoutPacket>();
			CPythonMessenger::Instance().OnBlockLogout(pack->name().c_str());
			break;
		}
		#endif
		case TGCHeader::MESSENGER_LOGIN:
		{
			auto pack = packet.get<GCMessengerLoginPacket>();
			CPythonMessenger::Instance().OnFriendLogin(pack->name().c_str());
			__RefreshTargetBoardByName(pack->name().c_str());
			break;
		}

		case TGCHeader::MESSENGER_LOGOUT:
		{
			auto pack = packet.get<GCMessengerLogoutPacket>();
			CPythonMessenger::Instance().OnFriendLogout(pack->name().c_str());
			break;
		}

		case TGCHeader::MESSENGER_MOBILE:
		{
			auto pack = packet.get<GCMessengerMobilePacket>();
			CPythonMessenger::Instance().SetMobile(pack->name().c_str(), pack->state());
			break;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// Party

bool CPythonNetworkStream::SendPartyInvitePacket(DWORD dwVID)
{
	CGOutputPacket<CGPartyInvitePacket> pack;
	pack->set_vid(dwVID);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracenf("CPythonNetworkStream::SendPartyInvitePacket [%ud] - PACKET SEND ERROR", dwVID);
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracef(" << SendPartyInvitePacket : %d\n", dwVID);
#endif
	return true;
}

bool CPythonNetworkStream::SendPartyInviteAnswerPacket(DWORD dwLeaderVID, BYTE byAnswer)
{
	CGOutputPacket<CGPartyInviteAnswerPacket> pack;
	pack->set_leader_vid(dwLeaderVID);
	pack->set_accept(byAnswer);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracenf("CPythonNetworkStream::SendPartyInviteAnswerPacket [%ud %ud] - PACKET SEND ERROR", dwLeaderVID, byAnswer);
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracef(" << SendPartyInviteAnswerPacket : %d, %d\n", dwLeaderVID, byAnswer);
#endif
	return true;
}

bool CPythonNetworkStream::SendPartyRemovePacket(DWORD dwPID)
{
	CGOutputPacket<CGPartyRemovePacket> pack;
	pack->set_pid(dwPID);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracenf("CPythonNetworkStream::SendPartyRemovePacket [%ud] - PACKET SEND ERROR", dwPID);
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracef(" << SendPartyRemovePacket : %d\n", dwPID);
#endif
	return true;
}

bool CPythonNetworkStream::SendPartySetStatePacket(DWORD dwVID, BYTE byState, BYTE byFlag)
{
	CGOutputPacket<CGPartySetStatePacket> pack;
	pack->set_pid(dwVID);
	pack->set_role(byState);
	pack->set_flag(byFlag);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracenf("CPythonNetworkStream::SendPartySetStatePacket(%ud, %ud) - PACKET SEND ERROR", dwVID, byState);
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracef(" << SendPartySetStatePacket : %d, %d, %d\n", dwVID, byState, byFlag);
#endif
	return true;
}

bool CPythonNetworkStream::SendPartyUseSkillPacket(BYTE bySkillIndex, DWORD dwVID)
{
	CGOutputPacket<CGPartyUseSkillPacket> pack;
	pack->set_skill_index(bySkillIndex);
	pack->set_vid(dwVID);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracenf("CPythonNetworkStream::SendPartyUseSkillPacket(%ud, %ud) - PACKET SEND ERROR", bySkillIndex, dwVID);
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracef(" << SendPartyUseSkillPacket : %d, %d\n", bySkillIndex, dwVID);
#endif
	return true;
}

bool CPythonNetworkStream::SendPartyParameterPacket(BYTE byDistributeMode)
{
	CGOutputPacket<CGPartyParameterPacket> pack;
	pack->set_distribute_mode(byDistributeMode);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracenf("CPythonNetworkStream::SendPartyParameterPacket(%d) - PACKET SEND ERROR", byDistributeMode);
#endif
		return false;
	}

#ifdef _USE_LOG_FILE
	Tracef(" << SendPartyParameterPacket : %d\n", byDistributeMode);
#endif
	return true;
}

bool CPythonNetworkStream::RecvPartyInvite(std::unique_ptr<GCPartyInvitePacket> pack)
{
	CInstanceBase* pInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->leader_vid());
	if (!pInstance)
	{
#ifdef _USE_LOG_FILE
		TraceError(" CPythonNetworkStream::RecvPartyInvite - Failed to find leader instance [%d]\n", pack->leader_vid());
#endif
		return true;
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RecvPartyInviteQuestion",
		Py_BuildValue("(is)", pack->leader_vid(), pInstance->GetNameString()));
#ifdef _USE_LOG_FILE
	Tracef(" >> RecvPartyInvite : %d, %s\n", pack->leader_vid(), pInstance->GetNameString());
#endif

	return true;
}

bool CPythonNetworkStream::RecvPartyAdd(std::unique_ptr<GCPartyAddPacket> pack)
{
	CPythonPlayer::Instance().AppendPartyMember(pack->pid(), pack->name().c_str());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "AddPartyMember", Py_BuildValue("(is)", pack->pid(), pack->name().c_str()));
#ifdef _USE_LOG_FILE
	Tracef(" >> RecvPartyAdd : %d, %s\n", pack->pid(), pack->name().c_str());
#endif

	return true;
}

bool CPythonNetworkStream::RecvPartyUpdate(std::unique_ptr<GCPartyUpdatePacket> pack)
{
	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(pack->pid(), &pPartyMemberInfo))
		return true;

	BYTE byOldState = pPartyMemberInfo->byState;

	CPythonPlayer::Instance().UpdatePartyMemberInfo(pack->pid(), pack->leader(), pack->role(), pack->percent_hp());
	for (int i = 0; i < PARTY_AFFECT_SLOT_MAX_NUM; ++i)
		CPythonPlayer::Instance().UpdatePartyMemberAffect(pack->pid(), i, pack->affects_size() > i ? pack->affects(i) : 0);

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UpdatePartyMemberInfo", Py_BuildValue("(i)", pack->pid()));

	// ¸¸¾à ¸®´õ°¡ ¹Ù²î¾ú´Ù¸é, TargetBoard ÀÇ ¹öÆ°À» ¾÷µ¥ÀÌÆ® ÇÑ´Ù.
	DWORD dwVID;
	if (CPythonPlayer::Instance().PartyMemberPIDToVID(pack->pid(), &dwVID))
	if (byOldState != pack->role())
	{
		__RefreshTargetBoardByVID(dwVID);
	}

#ifdef _USE_LOG_FILE
// 	Tracef(" >> RecvPartyUpdate : %d, %d, %d\n", kPartyUpdatePacket.pid, kPartyUpdatePacket.state, kPartyUpdatePacket.percent_hp);
#endif

	return true;
}

bool CPythonNetworkStream::RecvPartyRemove(std::unique_ptr<GCPartyRemovePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RemovePartyMember", Py_BuildValue("(i)", pack->pid()));
#ifdef _USE_LOG_FILE
	Tracef(" >> RecvPartyRemove : %d\n", pack->pid());
#endif

	return true;
}

bool CPythonNetworkStream::RecvPartyLink(std::unique_ptr<GCPartyLinkPacket> pack)
{
	CPythonPlayer::Instance().LinkPartyMember(pack->pid(), pack->vid());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "LinkPartyMember", Py_BuildValue("(ii)", pack->pid(), pack->vid()));
#ifdef _USE_LOG_FILE
	Tracef(" >> RecvPartyLink : %d, %d\n", pack->pid(), pack->vid());
#endif

	return true;
}

bool CPythonNetworkStream::RecvPartyUnlink(std::unique_ptr<GCPartyUnlinkPacket> pack)
{
	CPythonPlayer::Instance().UnlinkPartyMember(pack->pid());

	if (CPythonPlayer::Instance().IsMainCharacterIndex(pack->vid()))
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UnlinkAllPartyMember", Py_BuildValue("()"));
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "UnlinkPartyMember", Py_BuildValue("(i)", pack->pid()));
	}

#ifdef _USE_LOG_FILE
	Tracef(" >> RecvPartyUnlink : %d, %d\n", pack->pid(), pack->vid());
#endif

	return true;
}

bool CPythonNetworkStream::RecvPartyParameter(std::unique_ptr<GCPartyParameterPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "ChangePartyParameter", Py_BuildValue("(i)", pack->distribute_mode()));
#ifdef _USE_LOG_FILE
	Tracef(" >> RecvPartyParameter : %d\n", pack->distribute_mode());
#endif
	return true;
}

#ifdef ENABLE_HAIR_SELECTOR
bool CPythonNetworkStream::SendSelectHairPacket(BYTE index, DWORD dwHairVnum)
{
	CGOutputPacket<CGPlayerHairSelectPacket> pack;
	pack->set_index(index);
	pack->set_hair_vnum(dwHairVnum);

	if (!Send(pack))
		return false;

	return true;
}
#endif

bool CPythonNetworkStream::SendGuildAddMemberPacket(DWORD dwVID)
{
	CGOutputPacket<CGGuildAddMemberPacket> pack;
	pack->set_vid(dwVID);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildAddMemberPacket\n", dwVID);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildRemoveMemberPacket(DWORD dwPID)
{
	CGOutputPacket<CGGuildRemoveMemberPacket> pack;
	pack->set_pid(dwPID);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildRemoveMemberPacket %d\n", dwPID);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildChangeGradeNamePacket(BYTE byGradeNumber, const char * c_szName)
{
	CGOutputPacket<CGGuildChangeGradeNamePacket> pack;
	pack->set_grade(byGradeNumber);
	pack->set_gradename(c_szName);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildChangeGradeNamePacket %d, %s\n", byGradeNumber, c_szName);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildChangeGradeAuthorityPacket(BYTE byGradeNumber, WORD wAuthority)
{
	CGOutputPacket<CGGuildChangeGradeAuthorityPacket> pack;
	pack->set_grade(byGradeNumber);
	pack->set_authority(wAuthority);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildChangeGradeAuthorityPacket %d, %d\n", byGradeNumber, wAuthority);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildOfferPacket(DWORD dwExperience)
{
	CGOutputPacket<CGGuildOfferExpPacket> pack;
	pack->set_exp(dwExperience);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildOfferPacket %d\n", dwExperience);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildPostCommentPacket(const char * c_szMessage)
{
	CGOutputPacket<CGGuildPostCommentPacket> pack;
	pack->set_message(c_szMessage);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildPostCommentPacket %d, %s\n", pack->ByteSize(), c_szMessage);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildDeleteCommentPacket(DWORD dwIndex)
{
	CGOutputPacket<CGGuildDeleteCommentPacket> pack;
	pack->set_comment_id(dwIndex);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildDeleteCommentPacket %d\n", dwIndex);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildRefreshCommentsPacket(DWORD dwHighestIndex)
{
	static DWORD s_LastTime = timeGetTime() - 1001;

	if (timeGetTime() - s_LastTime < 1000)
		return true;
	s_LastTime = timeGetTime();

	if (!Send(TCGHeader::GUILD_REFRESH_COMMENT))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildRefreshCommentPacket %d\n", dwHighestIndex);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildChangeMemberGradePacket(DWORD dwPID, BYTE byGrade)
{
	CGOutputPacket<CGGuildChangeMemberGradePacket> pack;
	pack->set_pid(dwPID);
	pack->set_grade(byGrade);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildChangeMemberGradePacket %d, %d\n", dwPID, byGrade);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildUseSkillPacket(DWORD dwSkillID, DWORD dwTargetVID)
{
	CGOutputPacket<CGGuildUseSkillPacket> pack;
	pack->set_vnum(dwSkillID);
	pack->set_pid(dwTargetVID);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildUseSkillPacket %d, %d\n", dwSkillID, dwTargetVID);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildChangeMemberGeneralPacket(DWORD dwPID, BYTE byFlag)
{
	CGOutputPacket<CGGuildChangeMemberGeneralPacket> pack;
	pack->set_pid(dwPID);
	pack->set_is_general(byFlag);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildChangeMemberGeneralFlagPacket %d, %d\n", dwPID, byFlag);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildInviteAnswerPacket(DWORD dwGuildID, BYTE byAnswer)
{
	CGOutputPacket<CGGuildInviteAnswerPacket> pack;
	pack->set_guild_id(dwGuildID);
	pack->set_accept(byAnswer);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildInviteAnswerPacket %d, %d\n", dwGuildID, byAnswer);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildChargeGSPPacket(DWORD dwMoney)
{
	CGOutputPacket<CGGuildChargeGSPPacket> pack;
	pack->set_amount(dwMoney);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildChargeGSPPacket %d\n", dwMoney);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildDepositMoneyPacket(DWORD dwMoney)
{
	CGOutputPacket<CGGuildDepositMoneyPacket> pack;
	pack->set_gold(dwMoney);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildDepositMoneyPacket %d\n", dwMoney);
#endif
	return true;
}

bool CPythonNetworkStream::SendGuildWithdrawMoneyPacket(DWORD dwMoney)
{
	CGOutputPacket<CGGuildWithdrawMoneyPacket> pack;
	pack->set_gold(dwMoney);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendGuildWithdrawMoneyPacket %d\n", dwMoney);
#endif
	return true;
}

bool CPythonNetworkStream::SendRequestGuildList(DWORD pageNumber, BYTE pageType, BYTE empire)
{
	CPythonGuild::instance().SetLadder(NULL, 0, 0, 0);
	__RefreshGuildRankingWindow(false);

	CGOutputPacket<CGGuildRequestListPacket> pack;
	pack->set_page_number(pageNumber);
	pack->set_page_type(pageType);
	pack->set_empire(empire);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef(" SendRequestGuildList %d %d %d\n", pageNumber, pageType, empire);
#endif
	return true;
}

bool CPythonNetworkStream::SendRequestSearchGuild(const char* guildName, BYTE pageType, BYTE empire)
{
	CPythonGuild::instance().SetLadder(NULL, 0, 0, 0);
	__RefreshGuildRankingWindow(true);

	CGOutputPacket<CGGuildSearchPacket> pack;
	pack->set_search_name(guildName);
	pack->set_page_type(pageType);
	pack->set_empire(empire);

	if (!Send(pack))
		return false;

#ifdef _USE_LOG_FILE
	Tracef("SendRequestSearchGuild %s %d %d\n", guildName, pageType, empire);
#endif
	return true;
}

bool CPythonNetworkStream::RecvGuild(const InputPacket& packet)
{
	switch(packet.get_header<TGCHeader>())
	{
		case TGCHeader::GUILD_LOGIN:
		{
			auto pack = packet.get<GCGuildLoginPacket>();

			// Messenger
			CPythonGuild::TGuildMemberData * pGuildMemberData;
			if (CPythonGuild::Instance().GetMemberDataPtrByPID(pack->pid(), &pGuildMemberData))
				if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
					CPythonMessenger::Instance().LoginGuildMember(pGuildMemberData->strName.c_str());

			//Tracef(" <Login> %d\n", dwPID);
			break;
		}
		case TGCHeader::GUILD_LOGOUT:
		{
			auto pack = packet.get<GCGuildLogoutPacket>();

			// Messenger
			CPythonGuild::TGuildMemberData * pGuildMemberData;
			if (CPythonGuild::Instance().GetMemberDataPtrByPID(pack->pid(), &pGuildMemberData))
				if (0 != pGuildMemberData->strName.compare(CPythonPlayer::Instance().GetName()))
					CPythonMessenger::Instance().LogoutGuildMember(pGuildMemberData->strName.c_str());

			//Tracef(" <Logout> %d\n", dwPID);
			break;
		}
		case TGCHeader::GUILD_REMOVE:
		{
			auto pack = packet.get<GCGuildLogoutPacket>();

			if (CPythonGuild::Instance().IsMainPlayer(pack->pid()))
			{
				CPythonGuild::Instance().Destroy();
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "DeleteGuild", Py_BuildValue("()"));
				CPythonMessenger::Instance().RemoveAllGuildMember();
				__SetGuildID(0);
				__RefreshMessengerWindow();
				__RefreshTargetBoard();
				__RefreshCharacterWindow();
			}
			else
			{
				// Get Member Name
				std::string strMemberName = "";
				CPythonGuild::TGuildMemberData * pData;
				if (CPythonGuild::Instance().GetMemberDataPtrByPID(pack->pid(), &pData))
				{
					strMemberName = pData->strName;
					CPythonMessenger::Instance().RemoveGuildMember(pData->strName.c_str());
				}

				CPythonGuild::Instance().RemoveMember(pack->pid());

				// Refresh
				__RefreshTargetBoardByName(strMemberName.c_str());
				__RefreshGuildWindowMemberPage();
			}

			Tracef(" <Remove> %d\n", pack->pid());
			break;
		}
		case TGCHeader::GUILD_MEMBER_LIST:
		{
			auto pack = packet.get<GCGuildMemberListPacket>();

			for (auto& member : pack->members())
			{
				std::string member_name = member.name();
				if (member_name.empty())
				{
					CPythonGuild::TGuildMemberData * pMemberData;
					if (CPythonGuild::Instance().GetMemberDataPtrByPID(member.pid(), &pMemberData))
					{
						member_name = pMemberData->strName;
					}
				}

				//Tracef(" <List> %d : %s, %d (%d, %d, %d)\n", memberPacket.pid, szName, memberPacket.byGrade, memberPacket.byJob, memberPacket.byLevel, memberPacket.dwOffer);

				CPythonGuild::SGuildMemberData GuildMemberData;
				GuildMemberData.dwPID = member.pid();
				GuildMemberData.byGrade = member.grade();
				GuildMemberData.strName = member_name;
				GuildMemberData.byJob = member.job();
				GuildMemberData.byLevel = member.level();
				GuildMemberData.dwOffer = member.offer();
				GuildMemberData.byGeneralFlag = member.is_general();
				CPythonGuild::Instance().RegisterMember(GuildMemberData);

				// Messenger
				if (strcmp(member_name.c_str(), CPythonPlayer::Instance().GetName()))
					CPythonMessenger::Instance().AppendGuildMember(member_name.c_str());

				__RefreshTargetBoardByName(member_name.c_str());
			}

			__RefreshGuildWindowInfoPage();
			__RefreshGuildWindowMemberPage();
			__RefreshMessengerWindow();
			__RefreshCharacterWindow();
			break;
		}
		case TGCHeader::GUILD_GRADE:
		{
			auto pack = packet.get<GCGuildGradePacket>();

			for (auto& grade : pack->grades())
			{
				CPythonGuild::Instance().SetGradeData(grade.index(), CPythonGuild::SGuildGradeData(grade.auth_flag(), grade.name().c_str()));
			}
			__RefreshGuildWindowGradePage();
			__RefreshGuildWindowMemberPageGradeComboBox();
			break;
		}
		case TGCHeader::GUILD_GRADE_NAME:
		{
			auto pack = packet.get<GCGuildGradeNamePacket>();

			CPythonGuild::Instance().SetGradeName(pack->index(), pack->name().c_str());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGrade", Py_BuildValue("()"));

			Tracef(" <Change Grade Name> %d, %s\n", pack->index(), pack->name().c_str());
			__RefreshGuildWindowGradePage();
			__RefreshGuildWindowMemberPageGradeComboBox();
			break;
		}
		case TGCHeader::GUILD_GRADE_AUTH:
		{
			auto pack = packet.get<GCGuildGradeAuthPacket>();

			CPythonGuild::Instance().SetGradeAuthority(pack->index(), pack->auth_flag());
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshGuildGrade", Py_BuildValue("()"));

			Tracef(" <Change Grade Authority> %d, %d\n", pack->index(), pack->auth_flag());
			__RefreshGuildWindowGradePage();
			break;
		}
		case TGCHeader::GUILD_INFO:
		{
			auto pack = packet.get<GCGuildInfoPacket>();

			CPythonGuild::Instance().EnableGuild();
			CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
			strncpy(rGuildInfo.szGuildName, pack->name().c_str(), GUILD_NAME_MAX_LEN);
			rGuildInfo.szGuildName[GUILD_NAME_MAX_LEN] = '\0';

			rGuildInfo.dwGuildID = pack->guild_id();
			rGuildInfo.dwMasterPID = pack->master_pid();
			rGuildInfo.dwGuildLevel = pack->level();
			rGuildInfo.dwCurrentExperience = pack->exp();
			rGuildInfo.dwCurrentMemberCount = pack->member_count();
			rGuildInfo.dwMaxMemberCount = pack->max_member_count();
			rGuildInfo.dwGuildMoney = pack->gold();
			rGuildInfo.bHasLand = pack->has_land();

			rGuildInfo.dwGuildPoint = pack->guild_point();
			rGuildInfo.dwGuildRank = pack->guild_rank();
			for (int i = 0; i < MIN(3, pack->wins_size()); ++i)
				rGuildInfo.dwWin[i] = pack->wins(i);
			for (int i = 0; i < MIN(3, pack->draws_size()); ++i)
				rGuildInfo.dwDraw[i] = pack->wins(i);
			for (int i = 0; i < MIN(3, pack->losses_size()); ++i)
				rGuildInfo.dwLoss[i] = pack->wins(i);

			//Tracef(" <Info> %s, %d, %d : %d\n", GuildInfo.name, GuildInfo.master_pid, GuildInfo.level, rGuildInfo.bHasLand);
			__RefreshGuildWindowInfoPage();
			break;
		}
		case TGCHeader::GUILD_COMMENTS:
		{
			auto pack = packet.get<GCGuildCommentsPacket>();

			CPythonGuild::Instance().ClearComment();
			//Tracef(" >>> Comments Count : %d\n", byCount);

			for (auto& comment : pack->comments())
			{
				CPythonGuild::Instance().RegisterComment(comment.id(), comment.name().c_str(), comment.message().c_str());
			}

			__RefreshGuildWindowBoardPage();
			break;
		}
		case TGCHeader::GUILD_CHANGE_EXP:
		{
			auto pack = packet.get<GCGuildChangeExpPacket>();

			CPythonGuild::Instance().SetGuildEXP(pack->level(), pack->exp());
			Tracef(" <ChangeEXP> %d, %d\n", pack->level(), pack->exp());
			__RefreshGuildWindowInfoPage();
			break;
		}
		case TGCHeader::GUILD_CHANGE_MEMBER_GRADE:
		{
			auto pack = packet.get<GCGuildChangeMemberGradePacket>();

			CPythonGuild::Instance().ChangeGuildMemberGrade(pack->pid(), pack->grade());
			Tracef(" <ChangeMemberGrade> %d, %d\n", pack->pid(), pack->grade());
			__RefreshGuildWindowMemberPage();
			break;
		}
		case TGCHeader::GUILD_SKILL_INFO:
		{
			auto pack = packet.get<GCGuildSkillInfoPacket>();

			CPythonGuild::TGuildSkillData & rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
			rSkillData.bySkillPoint = pack->skill_point();
			for (int i = 0; i < MIN(CPythonGuild::GUILD_SKILL_MAX_NUM, pack->skill_levels_size()); ++i)
				rSkillData.bySkillLevel[i] = pack->skill_levels(i);
			rSkillData.wGuildPoint = pack->guild_point();
			rSkillData.wMaxGuildPoint = pack->max_guild_point();

			Tracef(" <SkillInfo> %d / %d, %d\n", rSkillData.bySkillPoint, rSkillData.wGuildPoint, rSkillData.wMaxGuildPoint);
			__RefreshGuildWindowSkillPage();
			break;
		}
		case TGCHeader::GUILD_CHANGE_MEMBER_GENERAL:
		{
			auto pack = packet.get<GCGuildChangeMemberGeneralPacket>();

			CPythonGuild::Instance().ChangeGuildMemberGeneralFlag(pack->pid(), pack->flag());
			Tracef(" <ChangeMemberGeneralFlag> %d, %d\n", pack->pid(), pack->flag());
			__RefreshGuildWindowMemberPage();
			break;
		}
		case TGCHeader::GUILD_INVITE:
		{
			auto pack = packet.get<GCGuildInvitePacket>();

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RecvGuildInviteQuestion",
				Py_BuildValue("(is)", pack->guild_id(), pack->guild_name().c_str()));
			Tracef(" <Guild Invite> %d, %s\n", pack->guild_id(), pack->guild_name().c_str());
			break;
		}
		case TGCHeader::GUILD_WAR:
		{
			auto pack = packet.get<GCGuildWarPacket>();

			switch (pack->war_state())
			{
				case GUILD_WAR_SEND_DECLARE:
					Tracef(" >> GUILD_SUBHEADER_GC_WAR : GUILD_WAR_SEND_DECLARE\n");
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
						"BINARY_GuildWar_OnSendDeclare",
						Py_BuildValue("(i)", pack->guild_opponent())
					);
					break;
				case GUILD_WAR_RECV_DECLARE:
					Tracef(" >> GUILD_SUBHEADER_GC_WAR : GUILD_WAR_RECV_DECLARE\n");
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
						"BINARY_GuildWar_OnRecvDeclare",
						Py_BuildValue("(ii)", pack->guild_opponent(), pack->type())
					);
					break;
				case GUILD_WAR_ON_WAR:
					Tracef(" >> GUILD_SUBHEADER_GC_WAR : GUILD_WAR_ON_WAR : %d, %d\n", pack->guild_self(), pack->guild_opponent());
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
						"BINARY_GuildWar_OnStart",
						Py_BuildValue("(ii)", pack->guild_self(), pack->guild_opponent())
					);
					CPythonGuild::Instance().StartGuildWar(pack->guild_opponent());
					break;
				case GUILD_WAR_END:
					Tracef(" >> GUILD_SUBHEADER_GC_WAR : GUILD_WAR_END\n");
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
						"BINARY_GuildWar_OnEnd",
						Py_BuildValue("(ii)", pack->guild_self(), pack->guild_opponent())
					);
					CPythonGuild::Instance().EndGuildWar(pack->guild_opponent());
					break;
			}
			break;
		}
		case TGCHeader::GUILD_NAME:
		{
			auto pack = packet.get<GCGuildNamePacket>();

			for (auto& elem : pack->names())
				CPythonGuild::Instance().RegisterGuildName(elem.guild_id(), elem.name().c_str());

			break;
		}
		case TGCHeader::GUILD_WAR_LIST:
		{
			auto pack = packet.get<GCGuildWarListPacket>();

			for (auto& elem : pack->wars())
			{
				Tracef(" >> GulidWarList [%d vs %d]\n", elem.src_guild_id(), elem.dst_guild_id());
				CInstanceBase::InsertGVGKey(elem.src_guild_id(), elem.dst_guild_id());
				CPythonCharacterManager::Instance().ChangeGVG(elem.src_guild_id(), elem.dst_guild_id());
			}
			break;
		}
		case TGCHeader::GUILD_WAR_END_LIST:
		{
			auto pack = packet.get<GCGuildWarEndListPacket>();

			for (auto& elem : pack->wars())
			{
				Tracef(" >> GulidWarEndList [%d vs %d]\n", elem.src_guild_id(), elem.dst_guild_id());
				CInstanceBase::RemoveGVGKey(elem.src_guild_id(), elem.dst_guild_id());
				CPythonCharacterManager::Instance().ChangeGVG(elem.src_guild_id(), elem.dst_guild_id());
			}
			break;
		}
		case TGCHeader::GUILD_WAR_POINT:
		{
			auto pack = packet.get<GCGuildWarPointPacket>();

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
				"BINARY_GuildWar_OnRecvPoint",
				Py_BuildValue("(iii)", pack->gain_guild_id(), pack->opponent_guild_id(), pack->point())
			);
			break;
		}
		case TGCHeader::GUILD_MONEY_CHANGE:
		{
			auto pack = packet.get<GCGuildMoneyChangePacket>();

			CPythonGuild::Instance().SetGuildMoney(pack->gold());

			__RefreshGuildWindowInfoPage();
			Tracef(" >> Guild Money Change : %d\n", pack->gold());
			break;
		}
		case TGCHeader::GUILD_RANK_AND_POINT:
		{
			auto pack = packet.get<GCGuildRankAndPointPacket>();

			CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();

			rGuildInfo.dwGuildPoint = pack->point();
			rGuildInfo.dwGuildRank = pack->rank();

			__RefreshGuildWindowSkillPage();
#ifdef _USE_LOG_FILE
			Tracef(" >> Guild Point: %u Guild Rank: %u", rGuildInfo.dwGuildPoint, rGuildInfo.dwGuildRank);
#endif
			break;
		}
		case TGCHeader::GUILD_BATTLE_STATS:
		{
			auto pack = packet.get<GCGuildBattleStatsPacket>();

			CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();

			for (int i = 0; i < MIN(3, pack->wins_size()); ++i)
				rGuildInfo.dwWin[i] = pack->wins(i);
			for (int i = 0; i < MIN(3, pack->draws_size()); ++i)
				rGuildInfo.dwDraw[i] = pack->wins(i);
			for (int i = 0; i < MIN(3, pack->losses_size()); ++i)
				rGuildInfo.dwLoss[i] = pack->wins(i);

			__RefreshGuildWindowSkillPage();
#ifdef _USE_LOG_FILE
			Tracef(" >> Guild war info: w %u %u %u d %u %u %u l %u %u %u", 
				rGuildInfo.dwWin[0], rGuildInfo.dwWin[1], rGuildInfo.dwWin[3],
				rGuildInfo.dwDraw[0], rGuildInfo.dwDraw[1], rGuildInfo.dwDraw[2], 
				rGuildInfo.dwLoss[0], rGuildInfo.dwLoss[1], rGuildInfo.dwLoss[2]);
#endif
			break;
		}
		case TGCHeader::GUILD_LADDER_LIST:
		{
			auto pack = packet.get<GCGuildLadderListPacket>();
			
			std::vector<guildLadder> ladders;
			ladders.reserve(pack->ladders_size());
			for (auto& elem : pack->ladders())
			{
				guildLadder ladder;
				strcpy_s(ladder.name, elem.name().c_str());
				ladder.level = elem.level();
				ladder.ladderPoints = elem.ladder_points();
				ladder.minMember = elem.min_member();
				ladder.maxMember = elem.max_member();

				ladders.push_back(std::move(ladder));
			}

			CPythonGuild::instance().SetLadder(ladders.size() > 0 ? &ladders[0] : nullptr, pack->ladders_size(), pack->page_number(), pack->total_pages());
			__RefreshGuildRankingWindow(false);
			break;
		}

		case TGCHeader::GUILD_LADDER_SEARCH_RESULT:
		{
			auto pack = packet.get<GCGuildLadderSearchResultPacket>();

			auto& recv_ladder = pack->ladder();
			guildLadder ladder;
			strcpy_s(ladder.name, recv_ladder.name().c_str());
			ladder.level = recv_ladder.level();
			ladder.ladderPoints = recv_ladder.ladder_points();
			ladder.minMember = recv_ladder.min_member();
			ladder.maxMember = recv_ladder.max_member();

			CPythonGuild::instance().SetLadder(&ladder, 1, 0, 0, pack->rank());
			__RefreshGuildRankingWindow(true);
			break;
		}
		case TGCHeader::GUILD_MEMBER_LAST_PLAYED:
		{
			auto pack = packet.get<GCGuildMemberLastPlayedPacket>();

			for (auto& elem : pack->members())
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_GuildReceiveLastplayed", Py_BuildValue("(ii)", elem.pid(), elem.timestamp()));

			__RefreshGuildWindowMemberPageLastPlayed();
			break;
		}
	}

	return true;
}

// Guild
//////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////
// Fishing

bool CPythonNetworkStream::SendFishingPacket(int iRotation)
{
	CGOutputPacket<CGFishingPacket> pack;
	pack->set_dir(iRotation);
	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendGiveItemPacket(DWORD dwTargetVID, ::TItemPos ItemPos, int iItemCount)
{
	CGOutputPacket<CGGiveItemPacket> pack;
	pack->set_target_vid(dwTargetVID);
	*pack->mutable_cell() = ItemPos;
	pack->set_item_count(iItemCount);

	if (!Send(pack))
		return false;

	return true;
}

#ifdef COMBAT_ZONE
bool CPythonNetworkStream::__RecvCombatZoneRankingPacket(std::unique_ptr<GCCombatZoneRankingDataPacket> pack)
{
	CPythonCombatZone::instance()->Initialize(*pack);
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CombatZone_Manager", Py_BuildValue("(s)", "RegisterRank"));
	return true;
}

bool CPythonNetworkStream::SendCombatZoneRequestActionPacket(int iAction, int iValue)
{
	CGOutputPacket<CGCombatZoneRequestActionPacket> p;
	p->set_action(iAction);
	p->set_value(iValue);

	if (!Send(p))
		return false;

	return true;
}

bool CPythonNetworkStream::__RecvCombatZonePacket(std::unique_ptr<GCSendCombatZonePacket> p)
{
	switch (p->sub_header())
	{
		/*
		case COMBAT_ZONE_SUB_HEADER_ADD_LEAVING_TARGET:
			CPythonBackground::Instance().CreateCombatZoneTargetsLeaving(p.m_pInfoData[0], p.m_pInfoData[1]);
			break;

		case COMBAT_ZONE_SUB_HEADER_REMOVE_LEAVING_TARGET:
			CPythonBackground::Instance().DeleteCombatZoneTargetsLeaving(0);
			break;
		*/

		case COMBAT_ZONE_SUB_HEADER_FLASH_ON_MINIMAP:
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CombatZone_Manager", Py_BuildValue("(s)", "StartFlashing"));
			break;

		case COMBAT_ZONE_SUB_HEADER_OPEN_RANKING:
			{
				DWORD dwPoints = p->data_infos(0);
				DWORD dwTimeRemaining = p->data_infos(1);
				DWORD dwCurMobsKills = p->data_infos(2);
				DWORD dwMaxMobsKills = p->data_infos(3);

				CPythonCombatZone::instance()->SendDataDays(p->data_days(), p->is_running());
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CombatZone_Manager", Py_BuildValue("(siiii)", "OpenWindow", dwPoints, dwTimeRemaining, dwCurMobsKills, dwMaxMobsKills));
			}
			break;

		case COMBAT_ZONE_SUB_HEADER_REFRESH_SHOP:
			{
				DWORD dwPoints = p->data_infos(0);
				DWORD dwCurLimit = p->data_infos(1);
				DWORD dwMaxLimit = p->data_infos(2);
				PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CombatZone_Manager", Py_BuildValue("(siii)", "RefreshShop", dwPoints, dwCurLimit, dwMaxLimit));
			}
			break;
		default:
			return false;
	}
	return true;
}
#endif

bool CPythonNetworkStream::RecvFishing(const InputPacket& packet)
{
	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::FISHING_START:
			{
				auto pack = packet.get<GCFishingStartPacket>();
				CInstanceBase * pFishingInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
				if (pFishingInstance)
					pFishingInstance->StartFishing(float(pack->dir()) * 5.0f);
			}
			break;

		case TGCHeader::FISHING_STOP:
			{
				auto pack = packet.get<GCFishingStopPacket>();
				CInstanceBase * pFishingInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
				if (pFishingInstance && pFishingInstance->IsFishing())
					pFishingInstance->StopFishing();
			}
			break;

		case TGCHeader::FISHING_REACT:
			{
				auto pack = packet.get<GCFishingReactPacket>();
				CInstanceBase * pFishingInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
				if (pFishingInstance && pFishingInstance->IsFishing())
				{
					pFishingInstance->SetFishEmoticon(); // Fish Emoticon
					pFishingInstance->ReactFishing();
				}
			}
			break;

		case TGCHeader::FISHING_SUCCESS:
			{
				auto pack = packet.get<GCFishingSuccessPacket>();
				CInstanceBase * pFishingInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
				if (pFishingInstance)
					pFishingInstance->CatchSuccess();
			}
			break;

		case TGCHeader::FISHING_FAIL:
			{
				auto pack = packet.get<GCFishingSuccessPacket>();
				CInstanceBase * pFishingInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
				if (pFishingInstance)
				{
					pFishingInstance->CatchFail();
					if (pFishingInstance == CPythonCharacterManager::Instance().GetMainInstancePtr())
					{
						PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnFishingFailure", Py_BuildValue("()"));
					}
				}
			}
			break;

		case TGCHeader::FISHING_FISH_INFO:
			{
				auto pack = packet.get<GCFishingFishInfoPacket>();

				if (0 == pack->info())
				{
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnFishingNotifyUnknown", Py_BuildValue("()"));
					return true;
				}

				CItemData * pItemData;
				if (!CItemManager::Instance().GetItemDataPointer(pack->info(), &pItemData))
					return true;

				CInstanceBase * pMainInstance = CPythonCharacterManager::Instance().GetMainInstancePtr();
				if (!pMainInstance)
					return true;

				if (pMainInstance->IsFishing())
				{
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnFishingNotify",
						Py_BuildValue("(is)", CItemData::ITEM_TYPE_FISH == pItemData->GetType(), pItemData->GetName()));
				}
				else
				{
					PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OnFishingSuccess",
						Py_BuildValue("(is)", CItemData::ITEM_TYPE_FISH == pItemData->GetType(), pItemData->GetName()));
				}
			}
			break;

		default:
			return false;
	}

	return true;
}
// Fishing
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// Dungeon
bool CPythonNetworkStream::RecvDungeon(std::unique_ptr<GCDungeonDestinationPositionPacket> pack)
{
	CPythonPlayer::Instance().SetDungeonDestinationPosition(pack->x(), pack->y());

	return true;
}
// Dungeon
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////
// MyShop
bool CPythonNetworkStream::SendBuildPrivateShopPacket(const char * c_szName, const std::vector<TShopItemTable> & c_rSellingItemStock)
{
	CGOutputPacket<CGMyShopPacket> pack;
	pack->set_sign(c_szName);

	for (auto elem : c_rSellingItemStock)
	{
		*pack->add_items() = elem;
	}

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvShopSignPacket(std::unique_ptr<GCShopSignPacket> pack)
{
	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	
	if (pack->sign().empty())
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], 
			"BINARY_PrivateShop_Disappear", 
			Py_BuildValue("(i)", pack->vid())
		);

		if (rkPlayer.IsMainCharacterIndex(pack->vid()))
			rkPlayer.ClosePrivateShop();
	}
	else
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
			"BINARY_PrivateShop_Appear",
#ifdef ENABLE_AUCTION
			Py_BuildValue("(isfffi)", pack->vid(), pack->sign().c_str(), pack->red(), pack->green(), pack->blue(), pack->style())
#else
			Py_BuildValue("(isfff)", pack->vid(), pack->sign().c_str(), pack->red(), pack->green(), pack->blue())
#endif
		);

		if (rkPlayer.IsMainCharacterIndex(pack->vid()))
			rkPlayer.OpenPrivateShop();
	}

	return true;
}
/////////////////////////////////////////////////////////////////////////

bool CPythonNetworkStream::RecvTimePacket(std::unique_ptr<GCTimePacket> pack)
{
	IAbstractApplication& rkApp=IAbstractApplication::GetSingleton();
	rkApp.SetServerTime(pack->time());
	CPythonApplication::Instance().SetRandom(pack->random());
	//CPythonShop::Instance().SetSellMarginPercent(TimePacket.sellMargin, TimePacket.sellMarginPositive);

#ifdef COMBAT_ZONE
	if (pack->combatzone())
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CombatZone_Flash", Py_BuildValue("(i)", pack->combatzone()));
#endif

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SetChannelInfo", Py_BuildValue("(i)", pack->channel()));

	if (pack->test_server())
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EnableTestServerFlag", Py_BuildValue("()"));

	if (pack->coins())
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_Web_PreLoad", Py_BuildValue("()"));

	SetCurrentMapIndex(pack->map_index());
	
	return true;
}

bool CPythonNetworkStream::RecvWalkModePacket(std::unique_ptr<GCWalkModePacket> pack)
{
	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	if (pInstance)
	{
		if (WALKMODE_RUN == pack->mode())
		{
			pInstance->SetRunMode();
		}
		else
		{
			pInstance->SetWalkMode();
		}
	}

	return true;
}

bool CPythonNetworkStream::RecvChangeSkillGroupPacket(std::unique_ptr<GCChangeSkillGroupPacket> pack)
{
	m_dwMainActorSkillGroup = pack->skill_group();

	CPythonPlayer::Instance().NEW_ClearSkillData();
	__RefreshCharacterWindow();
	return true;
}

void CPythonNetworkStream::__TEST_SetSkillGroupFake(int iIndex)
{
	m_dwMainActorSkillGroup = DWORD(iIndex);

	CPythonPlayer::Instance().NEW_ClearSkillData();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "RefreshCharacter", Py_BuildValue("()"));
}

bool CPythonNetworkStream::SendRefinePacket(BYTE byPos, BYTE byType, bool bFastRefine)
{
	CGOutputPacket<CGRefinePacket> pack;
	*pack->mutable_cell() = ::TItemPos(INVENTORY, byPos);
	pack->set_type(byType);
	pack->set_fast_refine(bFastRefine);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendSelectItemPacket(DWORD dwItemPos)
{
	CGOutputPacket<CGScriptSelectItemPacket> pack;
	pack->set_selection(dwItemPos);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvRefineInformationPacket(std::unique_ptr<GCRefineInformationPacket> pack)
{
	//TRefineTable & rkRefineTable = kRefineInfoPacket.refine_table;
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME],
		"OpenRefineDialog",
		Py_BuildValue("(iiLiib)",
		pack->pos().cell(),
		pack->result_vnum(),
		(long long) pack->cost(),
		pack->prob(),
		pack->type(),
		pack->can_fast_refine()
		)
		);

	for (int i = 0; i < pack->refine_table().material_count(); ++i)
	{
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "AppendMaterialToRefineDialog",
			Py_BuildValue("(ii)", pack->refine_table().materials(i).vnum(), pack->refine_table().materials(i).count()));
	}

#ifdef _DEBUG
#ifdef _USE_LOG_FILE
	Tracef(" >> RecvRefineInformationPacket(pos=%d, result_vnum=%d, cost=%d, prob=%d, type=%d)\n",
		pack->pos(),
		pack->result_vnum(),
		pack->cost(),
		pack->prob(),
		pack->type());
#endif
#endif

	return true;
}

bool CPythonNetworkStream::RecvNPCList(std::unique_ptr<GCNPCListPacket> pack)
{
	CPythonMiniMap::Instance().ClearAtlasMarkInfo();

	CRaceData* pRaceData;
	for (auto& elem : pack->positions())
	{
		CPythonMiniMap::Instance().RegisterAtlasMark(elem.type(), elem.name().c_str(), elem.x(), elem.y());

		if (CRaceManager::instance().GetRaceDataPointer(elem.race(), &pRaceData))
			pRaceData->LoadMotions();
	}

	return true;
}

bool CPythonNetworkStream::SendClientVersionPacket()
{
	std::string filename;
	GetExcutedFileName(filename);
	filename = CFileNameHelper::NoPath(filename);
	CFileNameHelper::ChangeDosPath(filename);

	CGOutputPacket<CGClientVersionPacket> pack;
	pack->set_filename(filename);
	pack->set_timestamp(g_szCurrentClientVersion);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvAffectAddPacket(std::unique_ptr<GCAffectAddPacket> pack)
{
	auto& rkElement = pack->elem();
	if (rkElement.apply_on() == POINT_ENERGY)
	{
		CPythonPlayer::instance().SetStatus(POINT_ENERGY_END_TIME, CPythonApplication::Instance().GetServerTimeStamp() + rkElement.duration());
		__RefreshStatus();
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_NEW_AddAffect",
		Py_BuildValue("(iiiii)", rkElement.type(), rkElement.apply_on(), rkElement.apply_value(), rkElement.flag(), rkElement.duration()));

	return true;
}

bool CPythonNetworkStream::RecvAffectRemovePacket(std::unique_ptr<GCAffectRemovePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_NEW_RemoveAffect",
		Py_BuildValue("(iii)", pack->type(), pack->apply_on(), pack->flag()));

	return true;
}

bool CPythonNetworkStream::RecvViewEquipPacket(std::unique_ptr<GCViewEquipPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "OpenEquipmentDialog", Py_BuildValue("(i)", pack->vid()));

	for (int i = 0; i < WEAR_MAX_NUM; ++i)
	{
		auto& rItemSet = pack->equips(i);
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetEquipmentDialogItem",
			Py_BuildValue("(iiii)", pack->vid(), i, rItemSet.vnum(), rItemSet.count()));

		for (int j = 0; j < ITEM_SOCKET_SLOT_MAX_NUM; ++j)
		{
			long lSocket = rItemSet.sockets(j);
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetEquipmentDialogSocket",
				Py_BuildValue("(iiii)", pack->vid(), i, j, lSocket));
		}

		for (int k = 0; k < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++k)
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "SetEquipmentDialogAttr",
				Py_BuildValue("(iiiii)", pack->vid(), i, k, rItemSet.attributes(k).type(), rItemSet.attributes(k).value()));
	}

	return true;
}

bool CPythonNetworkStream::RecvLandPacket(std::unique_ptr<GCLandListPacket> pack)
{
	std::vector<DWORD> kVec_dwGuildID;

	CPythonMiniMap & rkMiniMap = CPythonMiniMap::Instance();
	CPythonBackground & rkBG = CPythonBackground::Instance();
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();

	rkMiniMap.ClearGuildArea();
	rkBG.ClearGuildArea();

	for (auto& elem : pack->lands())
	{
		rkMiniMap.RegisterGuildArea(elem.id(),
									elem.guild_id(),
									elem.x(),
									elem.y(),
									elem.width(),
									elem.height());

		if (pMainInstance)
		if (elem.guild_id() == pMainInstance->GetGuildID())
		{
			rkBG.RegisterGuildArea(elem.x(),
								   elem.y(),
								   elem.x()+elem.width(),
								   elem.y()+elem.height());
		}

		if (0 != elem.guild_id())
			kVec_dwGuildID.push_back(elem.guild_id());
	}

	__DownloadSymbol(kVec_dwGuildID);

	return true;
}

bool CPythonNetworkStream::RecvTargetCreatePacket(std::unique_ptr<GCTargetCreatePacket> pack)
{
	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	CPythonBackground & rkpyBG = CPythonBackground::Instance();
	if (CREATE_TARGET_TYPE_LOCATION == pack->type())
	{
		rkpyMiniMap.CreateTarget(pack->id(), pack->name().c_str());
	}
#ifdef COMBAT_ZONE
	else if (CREATE_TARGET_TYPE_COMBAT_ZONE == pack->type())
	{
		rkpyBG.CreateCombatZoneTargetsLeaving(pack->id(), pack->vid());
		rkpyMiniMap.CreateTarget(pack->id(),
#if defined(COMBAT_ZONE_HIDE_INFO_USER)
			""
#else
			pack->name().c_str()
#endif
			, pack->vid());
	}
#endif
	else
	{
		rkpyMiniMap.CreateTarget(pack->id(), pack->name().c_str(), pack->vid());
		rkpyBG.CreateTargetEffect(pack->id(), pack->vid());
	}

//#ifdef _DEBUG
//	char szBuf[256+1];
//	_snprintf(szBuf, sizeof(szBuf), "Ä³¸¯ÅÍ Å¸°ÙÀÌ »ý¼º µÇ¾ú½À´Ï´Ù [%d:%s:%d]", kTargetCreate.lID, kTargetCreate.szTargetName, kTargetCreate.dwVID);
//	CPythonChat::Instance().AppendChat(CHAT_TYPE_NOTICE, szBuf);
//	Tracef(" >> RecvTargetCreatePacketNew %d : %d/%d\n", kTargetCreate.lID, kTargetCreate.byType, kTargetCreate.dwVID);
//#endif

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_OpenAtlasWindow", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::RecvTargetUpdatePacket(std::unique_ptr<GCTargetUpdatePacket> pack)
{
	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	rkpyMiniMap.UpdateTarget(pack->id(), pack->x(), pack->y());

	CPythonBackground & rkpyBG = CPythonBackground::Instance();
	rkpyBG.CreateTargetEffect(pack->id(), pack->x(), pack->y());

//#ifdef _DEBUG
//	char szBuf[256+1];
//	_snprintf(szBuf, sizeof(szBuf), "Å¸°ÙÀÇ À§Ä¡°¡ °»½Å µÇ¾ú½À´Ï´Ù [%d:%d/%d]", kTargetUpdate.lID, kTargetUpdate.lX, kTargetUpdate.lY);
//	CPythonChat::Instance().AppendChat(CHAT_TYPE_NOTICE, szBuf);
//	Tracef(" >> RecvTargetUpdatePacket %d : %d, %d\n", kTargetUpdate.lID, kTargetUpdate.lX, kTargetUpdate.lY);
//#endif

	return true;
}

bool CPythonNetworkStream::RecvTargetDeletePacket(std::unique_ptr<GCTargetDeletePacket> pack)
{
	CPythonMiniMap & rkpyMiniMap = CPythonMiniMap::Instance();
	rkpyMiniMap.DeleteTarget(pack->id());

	CPythonBackground & rkpyBG = CPythonBackground::Instance();
	rkpyBG.DeleteTargetEffect(pack->id());

//#ifdef _DEBUG
//	Tracef(" >> RecvTargetDeletePacket %d\n", kTargetDelete.lID);
//#endif

	return true;
}

bool CPythonNetworkStream::RecvLoverInfoPacket(std::unique_ptr<GCLoverInfoPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_LoverInfo", Py_BuildValue("(si)", pack->name().c_str(), pack->love_point()));
#ifdef _DEBUG
	Tracef("RECV LOVER INFO : %s, %d\n", kLoverInfo.szName, kLoverInfo.byLovePoint);
#endif
	return true;
}

bool CPythonNetworkStream::RecvLovePointUpdatePacket(std::unique_ptr<GCLoverPointUpdatePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_UpdateLovePoint", Py_BuildValue("(i)", pack->love_point()));
#ifdef _DEBUG
	Tracef("RECV LOVE POINT UPDATE : %d\n", kLovePointUpdate.byLovePoint);
#endif
	return true;
}

bool CPythonNetworkStream::RecvDigMotionPacket(std::unique_ptr<GCDigMotionPacket> pack)
{
#ifdef _DEBUG
	Tracef(" Dig Motion [%d/%d]\n", kDigMotion.vid, kDigMotion.count);
#endif

	IAbstractCharacterManager& rkChrMgr=IAbstractCharacterManager::GetSingleton();
	CInstanceBase * pkInstMain = rkChrMgr.GetInstancePtr(pack->vid());
	CInstanceBase * pkInstTarget = rkChrMgr.GetInstancePtr(pack->target_vid());
	if (NULL == pkInstMain)
		return true;

	if (pkInstTarget)
		pkInstMain->NEW_LookAtDestInstance(*pkInstTarget);

	for (int i = 0; i < pack->count(); ++i)
		pkInstMain->PushOnceMotion(CRaceMotionData::NAME_DIG);

	return true;
}

bool CPythonNetworkStream::RecvPVPTeam(std::unique_ptr<GCPVPTeamPacket> pack)
{
	if (CPythonNetworkStream::Instance().GetMainActorVID() == pack->vid())
	{
		CPythonPlayer::Instance().SetPVPTeam(pack->team());
	}
	else
	{
		CPythonCharacterManager::Instance().SetPVPTeam(pack->vid(), pack->team());
	}

	return true;
}

#ifdef ENABLE_MAINTENANCE
bool CPythonNetworkStream::RecvMaintenancePacket(std::unique_ptr<GCMaintenanceInfoPacket> pack)
{
	if (pack->duration() > 0)
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_ShowMaintenanceSign", Py_BuildValue("(ii)", pack->remaining_time(), pack->duration()));
	else
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_HideMaintenanceSign", Py_BuildValue("()"));

	return true;
}
#endif

bool CPythonNetworkStream::RecvInventoryMaxNum(std::unique_ptr<GCInventoryMaxNumPacket> pack)
{
	if (pack->inv_type() == INVENTORY_SIZE_TYPE_NORMAL)
		CPythonPlayer::Instance().SetInventoryMaxNum(pack->max_num());
	else if (pack->inv_type() == INVENTORY_SIZE_TYPE_UPPITEM)
		CPythonPlayer::Instance().SetUppitemInventoryMaxNum(pack->max_num());
	else if (pack->inv_type() == INVENTORY_SIZE_TYPE_SKILLBOOK)
		CPythonPlayer::Instance().SetSkillbookInventoryMaxNum(pack->max_num());
	else if (pack->inv_type() == INVENTORY_SIZE_TYPE_STONE)
		CPythonPlayer::Instance().SetStoneInventoryMaxNum(pack->max_num());
	else if (pack->inv_type() == INVENTORY_SIZE_TYPE_ENCHANT)
		CPythonPlayer::Instance().SetEnchantInventoryMaxNum(pack->max_num());
	
	if (m_apoPhaseWnd[PHASE_WINDOW_GAME])
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshInventoryMax", Py_BuildValue("(i)", pack->inv_type()));
	
	return true;
}

#ifdef ENABLE_ANIMAL_SYSTEM
bool CPythonNetworkStream::RecvAnimalSummon()
{
	TPacketGCAnimalSummon packet;
	if (!Recv(&packet))
		return false;

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SelectAnimalType(packet.type);
	rkPlayer.SetAnimalSummoned(true);
	rkPlayer.SetAnimalName(packet.name);
	rkPlayer.SetAnimalLevel(packet.level);
	rkPlayer.SetAnimalEXP(packet.exp);
	rkPlayer.SetAnimalMaxEXP(packet.max_exp);
	rkPlayer.SetAnimalStatPoints(packet.stat_points);
	rkPlayer.SetAnimalStats(packet.stats);

	__RefreshAnimalWindow(packet.type);
	return true;
}

bool CPythonNetworkStream::RecvAnimalUpdateLevel()
{
	TPacketGCAnimalUpdateLevel packet;
	if (!Recv(&packet))
		return false;

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SelectAnimalType(packet.type);
	rkPlayer.SetAnimalLevel(packet.level);
	rkPlayer.SetAnimalMaxEXP(packet.max_exp);
	rkPlayer.SetAnimalStatPoints(packet.stat_points);
	rkPlayer.SetAnimalStats(packet.stats);

	__RefreshAnimalWindow(packet.type);
	return true;
}

bool CPythonNetworkStream::RecvAnimalUpdateExp()
{
	TPacketGCAnimalUpdateExp packet;
	if (!Recv(&packet))
		return false;

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SelectAnimalType(packet.type);
	rkPlayer.SetAnimalEXP(packet.exp);

	__RefreshAnimalWindow(packet.type);
	return true;
}

bool CPythonNetworkStream::RecvAnimalUpdateStats()
{
	TPacketGCAnimalUpdateStats packet;
	if (!Recv(&packet))
		return false;

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SelectAnimalType(packet.type);
	rkPlayer.SetAnimalSummoned(true);
	rkPlayer.SetAnimalStatPoints(packet.stat_points);
	rkPlayer.SetAnimalStats(packet.stats);

	__RefreshAnimalWindow(packet.type);
	return true;
}

bool CPythonNetworkStream::RecvAnimalUnsummon()
{
	TPacketGCAnimalUnsummon packet;
	if (!Recv(&packet))
		return false;

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SelectAnimalType(packet.type);
	rkPlayer.SetAnimalSummoned(false);

	__RefreshAnimalWindow(packet.type);
	return true;
}
#endif

bool CPythonNetworkStream::RecvSkillMotion(std::unique_ptr<GCSkillMotionPacket> pack)
{
	long lX = pack->x(), lY = pack->y();
	__GlobalPositionToLocalPosition(lX, lY);

	// Get Skill-Data
	CPythonSkill::TSkillData * pSkillData;
	if (!CPythonSkill::Instance().GetSkillData(pack->skill_vnum(), &pSkillData))
	{
		TraceError("RecvSkillMotion: cannot use skill motion %u", pack->skill_vnum());
		return false;
	}

	if (pSkillData->IsNoMotion())
		return true;

	DWORD dwMotionIndex = pSkillData->GetSkillMotionIndex(pack->skill_grade());
	DWORD dwLoopCount = pSkillData->GetMotionLoopCount(CLocaleManager::instance().GetSkillPower(pack->skill_level()) / 100.0f);
	if (pSkillData->IsMovingSkill())
		dwLoopCount |= 1 << 4;

	SNetworkMoveActorData kNetMoveActorData;
	kNetMoveActorData.m_dwArg = dwLoopCount | (dwMotionIndex << 8);
	kNetMoveActorData.m_dwFunc = CInstanceBase::FUNC_SKILL;
	kNetMoveActorData.m_dwTime = pack->time();
	kNetMoveActorData.m_dwVID = pack->vid();
	kNetMoveActorData.m_fRot = pack->rotation() * 5.0f;
	kNetMoveActorData.m_lPosX = lX;
	kNetMoveActorData.m_lPosY = lY;
	kNetMoveActorData.m_dwDuration = 0;

	// TraceError("MoveActor");
	m_rokNetActorMgr->MoveActor(kNetMoveActorData);
	// TraceError("MoveActor END");

	return true;
}

#ifdef ENABLE_DRAGONSOUL
bool CPythonNetworkStream::RecvDragonSoulRefine(std::unique_ptr<GCDragonSoulRefinePacket> pack)
{
	switch (pack->sub_type())
	{
	case DS_SUB_HEADER_OPEN:
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_Open", Py_BuildValue("()"));
		break;
	case DS_SUB_HEADER_REFINE_FAIL:
	case DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE:
	case DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL:
	case DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY:
	case DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL:
	case DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL:
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_RefineFail", Py_BuildValue("(iii)",
			pack->sub_type(), pack->cell().window_type(), pack->cell().cell()));
		break;
	case DS_SUB_HEADER_REFINE_SUCCEED:
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DragonSoulRefineWindow_RefineSucceed",
			Py_BuildValue("(ii)", pack->cell().window_type(), pack->cell().cell()));
		break;
	}

	return true;
}

bool CPythonNetworkStream::SendDragonSoulRefinePacket(BYTE bRefineType, ::TItemPos* pos)
{
	CGOutputPacket<CGDragonSoulRefinePacket> pack;
	pack->set_sub_type(bRefineType);
	for (int i = 0; i < DS_REFINE_WINDOW_MAX_NUM; ++i)
		*pack->add_item_grid() = pos[i];

	if (!Send(pack))
		return false;

	return true;
}
#endif

#ifdef ENABLE_GAYA_SYSTEM
bool CPythonNetworkStream::RecvGayaShopOpen(std::unique_ptr<GCGayaShopOpenPacket> pack)
{
	CPythonShop::Instance().SetGayaShopData(pack->datas());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_GayaShopOpen", Py_BuildValue("()"));

	return true;
}
#endif

bool CPythonNetworkStream::SendTargetMonsterDropInfo(DWORD dwRaceNum)
{
	CGOutputPacket<CGTargetMonsterDropInfoPacket> pack;
	pack->set_race_num(dwRaceNum);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvTargetMonsterDropInfo(std::unique_ptr<GCTargetMonsterInfoPacket> pack)
{
	for (auto& elem : pack->drops())
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AddTargetMonsterDropInfo",
			Py_BuildValue("(iiii)", pack->race_num(), elem.level_limit(), elem.item_vnum(), elem.item_count()));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshTargetMonsterDropInfo", Py_BuildValue("(i)", pack->race_num()));

	return true;
}

#ifdef ENABLE_AUCTION
bool CPythonNetworkStream::SendAuctionInsertItemPacket(::TItemPos cell, ::TItemPos target_cell, uint64_t price)
{
	CGOutputPacket<CGAuctionInsertItemPacket> pack;
	*pack->mutable_cell() = cell;
	*pack->mutable_target_cell() = target_cell;
	pack->set_price(price);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionTakeItemPacket(DWORD item_id, WORD target_inven_pos)
{
	CGOutputPacket<CGAuctionTakeItemPacket> pack;
	pack->set_item_id(item_id);
	pack->set_inventory_pos(target_inven_pos);
	
	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionBuyItemPacket(DWORD item_id, uint64_t price)
{
	CGOutputPacket<CGAuctionBuyItemPacket> pack;
	pack->set_item_id(item_id);
	pack->set_price(price);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionTakeGoldPacket(uint64_t gold)
{
	CGOutputPacket<CGAuctionTakeGoldPacket> pack;
	pack->set_gold(gold);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionSearchItemsPacket(WORD page, const network::TDataAuctionSearch& options)
{
	CGOutputPacket<CGAuctionSearchItemsPacket> pack;
	pack->set_page(page);
	*pack->mutable_options() = options;

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionExtendedSearchItemsPacket(WORD page, const network::TExtendedDataAuctionSearch& options)
{
	CGOutputPacket<CGAuctionExtendedSearchItemsPacket> pack;
	pack->set_page(page);
	*pack->mutable_options() = options;

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionMarkShopPacket(DWORD item_id)
{
	CGOutputPacket<CGAuctionMarkShopPacket> pack;
	pack->set_item_id(item_id);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionRequestShopView()
{
	if (!Send(TCGHeader::AUCTION_SHOP_REQUEST_SHOW))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionOpenShopPacket(const std::string& name, float red, float green, float blue, std::vector<network::TShopItemTable>&& items, BYTE model, BYTE style)
{
	TPixelPosition kPixelPosition;
	CPythonPlayer::Instance().NEW_GetMainActorPosition(&kPixelPosition);

	CGOutputPacket<CGAuctionShopOpenPacket> pack;

	pack->set_name(name);
	pack->set_style(style);
	pack->set_model(model);

	pack->set_color_red(red);
	pack->set_color_green(green);
	pack->set_color_blue(blue);

	pack->set_x(kPixelPosition.x);
	pack->set_y(kPixelPosition.y);

	for (auto& item : items)
		*pack->add_items() = std::move(item);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionTakeShopGoldPacket(uint64_t gold)
{
	CGOutputPacket<CGAuctionShopTakeGoldPacket> pack;
	pack->set_gold(gold);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionRenewShopPacket()
{
	if (!Send(TCGHeader::AUCTION_SHOP_RENEW))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionCloseShopPacket(bool has_items)
{
	network::CGOutputPacket<network::CGAuctionShopClosePacket> pack;
	pack->set_has_items(has_items);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionShopGuestCancelPacket()
{
	if (!Send(TCGHeader::AUCTION_SHOP_GUEST_CANCEL))
		return false;

	return true;
}

bool CPythonNetworkStream::SendAuctionRequestShopHistoryPacket()
{
	if (!Send(TCGHeader::AUCTION_SHOP_REQUEST_HISTORY))
		return false;

	return true;
}

bool CPythonNetworkStream::SendRequestAveragePricePacket(BYTE requestor, DWORD vnum, DWORD count)
{
	network::CGOutputPacket<network::CGAuctionRequestAveragePricePacket> pack;
	pack->set_requestor(requestor);
	pack->set_vnum(vnum);
	pack->set_count(count);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvAuctionOwnedGold(std::unique_ptr<network::GCAuctionOwnedGoldPacket> pack)
{
	CPythonAuction::instance().set_owned_gold(pack->gold());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionGold", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionOwnedItem(std::unique_ptr<network::GCAuctionOwnedItemPacket> pack)
{
	auto& shop_item = *pack->mutable_item();

	BYTE container_type;
	if (CPythonAuction::TYPE_SHOP == shop_item.auction_type())
		container_type = CPythonAuction::ITEM_CONTAINER_OWNED_SHOP;
	else
		container_type = CPythonAuction::ITEM_CONTAINER_OWNED;

	if (shop_item.item().vnum())
		CPythonAuction::instance().append_item(container_type, std::move(shop_item));
	else
		CPythonAuction::instance().remove_item(container_type, shop_item.item().id());

	if (CPythonAuction::ITEM_CONTAINER_OWNED_SHOP == container_type)
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionOwnedShop", Py_BuildValue("()"));
	else
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionOwned", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionSearchResult(std::unique_ptr<network::GCAuctionSearchResultPacket> pack)
{
	CPythonAuction::instance().clear_items(CPythonAuction::ITEM_CONTAINER_SEARCH);

	for (auto& item : *pack->mutable_items())
		CPythonAuction::instance().append_item(CPythonAuction::ITEM_CONTAINER_SEARCH, std::move(item));

	if (pack->max_page() >= 0)
		CPythonAuction::instance().set_max_page_count(pack->max_page());

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionSearch", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::RecvAuctionMessage(std::unique_ptr<network::GCAuctionMessagePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RecvAuctionMessage", Py_BuildValue("(s)", pack->message().c_str()));
	return true;
}

bool CPythonNetworkStream::RecvAuctionShopOwned(std::unique_ptr<network::GCAuctionShopOwnedPacket> pack)
{
	CPythonAuction::instance().set_owned_shop(pack->owned());

	if (!pack->owned())
		CPythonAuction::instance().clear_owned_shop();

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionOwnedShop", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionShop(std::unique_ptr<network::GCAuctionShopPacket> pack)
{
	auto& auction = CPythonAuction::instance();

	if (pack->name().empty())
	{
		auction.clear_owned_shop();
	}
	else
	{
		auction.set_owned_shop(true);
		auction.load_owned_shop(pack->name(), pack->timeout(), pack->gold());

		auction.clear_items(CPythonAuction::ITEM_CONTAINER_OWNED_SHOP);
		for (auto& item : *pack->mutable_items())
		{
			TShopItemTable shop_item;
			*shop_item.mutable_item() = std::move(item);

			auction.append_item(CPythonAuction::ITEM_CONTAINER_OWNED_SHOP, std::move(shop_item));
		}
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionOwnedShop", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionShopGold(std::unique_ptr<network::GCAuctionShopGoldPacket> pack)
{
	CPythonAuction::instance().set_owned_shop_gold(pack->gold());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionOwnedShopGold", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionShopTimeout(std::unique_ptr<network::GCAuctionShopTimeoutPacket> pack)
{
	CPythonAuction::instance().set_owned_shop_timeout(pack->timeout());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionOwnedShopTimeout", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionShopHistory(std::unique_ptr<network::GCAuctionShopHistoryPacket> pack)
{
	CPythonAuction::instance().clear_shop_history();
	for (auto& elem : *pack->mutable_elems())
		CPythonAuction::instance().append_shop_history(std::move(elem));

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionShopHistory", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionAveragePrice(std::unique_ptr<network::GCAuctionAveragePricePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RecvAuctionAveragePrice", Py_BuildValue("(iL)", pack->requestor(), static_cast<long long>(pack->price())));
	return true;
}

bool CPythonNetworkStream::RecvAuctionShopGuestOpen(std::unique_ptr<network::GCAuctionShopGuestOpenPacket> pack)
{
	auto& auction = CPythonAuction::instance();
	auction.open_guest_shop(pack->name());

	auction.clear_items(CPythonAuction::ITEM_CONTAINER_SHOP);
	for (auto& item : *pack->mutable_items())
	{
		TShopItemTable shop_item;
		*shop_item.mutable_item() = std::move(item);

		auction.append_item(CPythonAuction::ITEM_CONTAINER_SHOP, std::move(shop_item));
	}

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_OpenAuctionGuestShop", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionShopGuestUpdate(std::unique_ptr<network::GCAuctionShopGuestUpdatePacket> pack)
{
	TShopItemTable shop_item;
	*shop_item.mutable_item() = std::move(*pack->mutable_item());

	if (shop_item.item().vnum())
		CPythonAuction::instance().append_item(CPythonAuction::ITEM_CONTAINER_SHOP, std::move(shop_item));
	else
		CPythonAuction::instance().remove_item(CPythonAuction::ITEM_CONTAINER_SHOP, shop_item.item().id());

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RefreshAuctionGuestShop", Py_BuildValue("()"));

	return true;
}

bool CPythonNetworkStream::RecvAuctionShopGuestClose()
{
	CPythonAuction::instance().close_guest_shop();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CloseAuctionGuestShop", Py_BuildValue("()"));

	return true;
}
#endif

bool CPythonNetworkStream::RecvShowTeamler(std::unique_ptr<GCTeamlerShowPacket> pack)
{
	CPythonPlayer::Instance().SetIsShowTeamler(pack->is_show());
	CPythonMessenger::Instance().RefreshTeamlerState();

	return true;
}

bool CPythonNetworkStream::RecvTeamlerStatus(std::unique_ptr<GCTeamlerStatusPacket> pack)
{
	if (pack->is_online())
		CPythonMessenger::Instance().OnTeamLogin(pack->name().c_str());
	else
		CPythonMessenger::Instance().OnTeamLogout(pack->name().c_str());

	return true;
}

bool CPythonNetworkStream::SendRequestOnlineInformation(const char* c_pszPlayerName)
{
	CGOutputPacket<CGPlayerLanguageInformationPacket> pack;
	pack->set_player_name(c_pszPlayerName);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvPlayerOnlineInformation(std::unique_ptr<GCPlayerOnlineInformationPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_RecvPlayerInfo", Py_BuildValue("(si)", pack->player_name().c_str(), pack->language_id()));

	return true;
}

#ifdef ENABLE_PYTHON_REPORT_PACKET
bool CPythonNetworkStream::SendHackReportPacket(const char* szTitle, const char* szDescription)
{
	static DWORD s_dwLastSend = 0;
	if (s_dwLastSend != 0 && time(0) - s_dwLastSend < 10)
		return false;

	s_dwLastSend = time(0);

	CGOutputPacket<CGBotReportLogPacket> pack;
	pack->set_type(szTitle);
	pack->set_detail(szDescription);

	if (!Send(pack))
		return false;

	return true;
}
#endif

#ifdef ENABLE_FAKEBUFF
bool CPythonNetworkStream::RecvFakeBuffSkill(std::unique_ptr<GCFakeBuffSkillPacket> pack)
{
	CPythonPlayer::Instance().SetFakeBuffSkill(pack->skill_vnum(), pack->level());
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_FakeBuffSkillRefresh", Py_BuildValue("(i)", pack->skill_vnum()));

	return true;
}
#endif

#ifdef ENABLE_ATTRTREE
bool CPythonNetworkStream::RecvAttrtreeLevel()
{
	TPacketGCAttrtreeLevel pack;
	if (!Recv(&pack))
		return false;

	BYTE row, col;
	CPythonPlayer::Instance().AttrtreeIDToCell(pack.id, row, col);
	CPythonPlayer::Instance().SetAttrtreeLevel(row, col, pack.level);

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AttrtreeRefresh", Py_BuildValue("(ii)", row, col));
	return true;
}

bool CPythonNetworkStream::RecvAttrtreeRefine()
{
	TPacketGCRefineInformation pack;
	if (!Recv(&pack))
		return false;

	BYTE row, col;
	CPythonPlayer::Instance().AttrtreeIDToCell(pack.pos, row, col);

	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AttrtreeRefine", Py_BuildValue("(iii)", row, col, pack.cost));
	for (int i = 0; i < pack.material_count; ++i)
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AttrtreeRefineMaterial", Py_BuildValue("(ii)", pack.materials[i].vnum, pack.materials[i].count));

	return true;
}
#endif

#ifdef ENABLE_EVENT_SYSTEM
bool CPythonNetworkStream::SendEventRequestAnswerPacket(DWORD dwIndex, bool bAccept)
{
	CGOutputPacket<CGEventRequestAnswerPacket> pack;
	pack->set_event_index(dwIndex);
	pack->set_accept(bAccept);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvEventRequestPacket(std::unique_ptr<GCEventRequestPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EventRequest",
						  Py_BuildValue("(iss)", pack->event_index(), pack->name().c_str(), pack->desc().c_str()));

	return true;
}

bool CPythonNetworkStream::RecvEventCancelPacket(std::unique_ptr<GCEventCancelPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EventCancel",
						  Py_BuildValue("(i)", pack->event_index()));

	return true;
}

bool CPythonNetworkStream::RecvEventEmpireWarLoadPacket(std::unique_ptr<GCEventEmpireWarLoadPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EventEmpireWarLoad",
						  Py_BuildValue("(iiiiiii)", pack->time_left(),
							  pack->kills(0), pack->deaths(0),
							  pack->kills(1), pack->deaths(1),
							  pack->kills(2), pack->deaths(2)));

	return true;
}

bool CPythonNetworkStream::RecvEventEmpireWarUpdatePacket(std::unique_ptr<GCEventEmpireWarUpdatePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EventEmpireWarUpdate",
						  Py_BuildValue("(iii)", pack->empire(), pack->kills(), pack->deaths()));

	return true;
}

bool CPythonNetworkStream::RecvEventEmpireWarFinishPacket()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EventEmpireWarFinish", Py_BuildValue("()"));

	return true;
}
#endif

#ifdef CHANGE_SKILL_COLOR
bool CPythonNetworkStream::SendSkillColorPacket(BYTE skill, DWORD col1, DWORD col2, DWORD col3, DWORD col4, DWORD col5)
{
	CGOutputPacket<CGSetSkillColorPacket> pack;
	pack->set_skill(skill);
	pack->set_col1(col1);
	pack->set_col2(col2);
	pack->set_col3(col3);
	pack->set_col4(col4);
	pack->set_col5(col5);

	if (!Send(pack))
	{
#ifdef _USE_LOG_FILE
		Tracen("CPythonNetworkStream::SendSkillColorPacket - ERROR");
#endif
		return false;
	}

	return true;
}
#endif

#ifdef BATTLEPASS_EXTENSION
bool CPythonNetworkStream::RecvBattlepassData(std::unique_ptr<GCBattlepassDataPacket> pack)
{
	auto& p = pack->data();
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_SetBattlePassData",
		Py_BuildValue("(IissII)", pack->index(), p.progress(), p.name().c_str(), p.task().c_str(), p.reward_vnum(), p.reward_count()));

	return true;
}
#endif

#ifdef CRYSTAL_SYSTEM
bool CPythonNetworkStream::RecvCrystalRefine(std::unique_ptr<network::GCCrystalRefinePacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_OpenCrystalRefine", Py_BuildValue("((ii)(ii)iii)",
		pack->crystal_cell().window_type(), pack->crystal_cell().cell(),
		pack->scroll_cell().window_type(), pack->scroll_cell().cell(),
		pack->next_clarity_type(), pack->next_clarity_level(), pack->required_fragments()));

	for (auto& attr : pack->next_attributes())
		PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_AddCrystalRefineAttr", Py_BuildValue("(ii)",
			attr.type(), attr.value()));

	return true;
}

bool CPythonNetworkStream::RecvCrystalRefineSuccess()
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CrystalRefineSuccess", Py_BuildValue("()"));
	return true;
}

bool CPythonNetworkStream::RecvCrystalUsingSlot(std::unique_ptr<network::GCCrystalUsingSlotPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_CrystalUsingSlot", Py_BuildValue("(iib)",
		pack->cell().window_type(), pack->cell().cell(), pack->active()));
	return true;
}
#endif

#ifdef ENABLE_EQUIPMENT_CHANGER
bool CPythonNetworkStream::SendEquipmentPageAddPacket(const char* c_pszPageName)
{
	CGOutputPacket<CGEquipmentPageAddPacket> pack;
	pack->set_name(c_pszPageName);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendEquipmentPageDeletePacket(DWORD dwIndex)
{
	CGOutputPacket<CGEquipmentPageDeletePacket> pack;
	pack->set_index(dwIndex);

	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::SendEquipmentPageSelectPacket(DWORD dwIndex)
{
	CGOutputPacket<CGEquipmentPageSelectPacket> pack;
	pack->set_index(dwIndex);
	
	if (!Send(pack))
		return false;

	return true;
}

bool CPythonNetworkStream::RecvEquipmentPageLoadPacket(std::unique_ptr<GCEquipmentPageLoadPacket> pack)
{
	CPythonPlayer::Instance().ClearEquipmentPages();
	CPythonPlayer::Instance().SetSelectedEquipmentPage(pack->selected_index());

	for (auto& page : pack->pages())
		CPythonPlayer::Instance().AddEquipmentPage(page);
	
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_EquipmentPageLoad", Py_BuildValue("()"));
	return true;
}
#endif

#ifdef DMG_METER
bool CPythonNetworkStream::RecvDmgMeterPacket(std::unique_ptr<GCDmgMeterPacket> pack)
{
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_DmgMeter", Py_BuildValue("(ii)", pack->dmg(), pack->vid()));

}
#endif

#ifdef ENABLE_PET_ADVANCED
bool CPythonNetworkStream::RecvPetAdvanced(InputPacket& packet)
{
	CPythonPetAdvanced& rkPet = CPythonPetAdvanced::Instance();

	switch (packet.get_header<TGCHeader>())
	{
		case TGCHeader::PET_SUMMON:
		{
			auto pack = packet.get<GCPetSummonPacket>();

			rkPet.SetSummonVID(pack->vid());
			rkPet.SetItemVnum(pack->item_vnum());
			rkPet.SetNextExp(pack->next_exp());

			auto& info = rkPet.GetPetTable();
			info = pack->pet();

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetSummon", Py_BuildValue("()"));
		}
		break;

		case TGCHeader::PET_UPDATE_EXP:
		{
			auto pack = packet.get<GCPetUpdateExpPacket>();

			rkPet.GetPetTable().set_exp(pack->exp());

			__RefreshPetStatusWindow();
		}
		break;

		case TGCHeader::PET_UPDATE_LEVEL:
		{
			auto pack = packet.get<GCPetUpdateLevelPacket>();

			rkPet.GetPetTable().set_level(pack->level());
			rkPet.SetNextExp(pack->next_exp());

			__RefreshPetStatusWindow();
		}
		break;

		case TGCHeader::PET_UPDATE_SKILL:
		{
			auto pack = packet.get<GCPetUpdateSkillPacket>();

			if (pack->index() >= PET_SKILL_MAX_NUM)
			{
				TraceError("RecvPetAdvanced: UPDATE_SKILL: invalid skill index %d (exceeds maximum of %d)", pack->index(), PET_SKILL_MAX_NUM);
				return false;
			}

			auto skill = rkPet.GetPetTable().mutable_skills(pack->index());
			*skill = pack->skill();

			__RefreshPetSkillWindow();
		}
		break;

		case TGCHeader::PET_UPDATE_ATTR:
		{
			auto pack = packet.get<GCPetUpdateAttrPacket>();

			rkPet.GetPetTable().set_attr_type(pack->index(), pack->type());
			rkPet.GetPetTable().set_attr_level(pack->index(), pack->level());
			rkPet.SetAttrValue(pack->index(), pack->value());

			__RefreshPetStatusWindow();
		}
		break;

		case TGCHeader::PET_UPDATE_SKILLPOWER:
		{
			auto pack = packet.get<GCPetUpdateSkillpowerPacket>();

			rkPet.GetPetTable().set_skillpower(pack->power());

			__RefreshPetStatusWindow();
		}
		break;

		case TGCHeader::PET_ATTR_REFINE_INFO:
		{
			auto pack = packet.get<GCPetAttrRefineInfoPacket>();

			network::TRefineTable refineInfo;
			refineInfo.set_cost(pack->cost());
			refineInfo.set_material_count(pack->materials_size());
			for (auto& material : pack->materials())
				*refineInfo.add_materials() = material;

			rkPet.SetAttrRefine(refineInfo);
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetAttrRefineInfo", Py_BuildValue("()"));
		}

		case TGCHeader::PET_EVOLUTION_INFO:
		{
			auto pack = packet.get<GCPetEvolutionInfoPacket>();

			network::TRefineTable refineInfo;
			refineInfo.set_cost(pack->cost());
			refineInfo.set_material_count(pack->materials_size());
			refineInfo.set_prob(pack->prob());
			for (auto& material : pack->materials())
				*refineInfo.add_materials() = material;

			rkPet.SetEvolutionRefine(refineInfo);
			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetEvolutionInfo", Py_BuildValue("()"));
		}
		break;

		case TGCHeader::PET_EVOLVE_RESULT:
		{
			auto pack = packet.get<GCPetEvolveResultPacket>();

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetEvolveResult", Py_BuildValue("(b)", pack->result()));

			if (pack->result())
				__RefreshPetSkillWindow();
		}
		break;

		case TGCHeader::PET_UNSUMMON:
		{
			rkPet.Clear();

			PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], "BINARY_PetUnsummon", Py_BuildValue("()"));
		}
		break;

		default:
			TraceError("RecvPetAdvanced: invalid subheader %u", packet.get_header());
			return false;
	}

	return true;
}

bool CPythonNetworkStream::SendPetUseEggPacket(::TItemPos egg_position, const std::string& name)
{
	CGOutputPacket<CGPetUseEggPacket> pack;
	pack->mutable_egg_cell()->set_window_type(egg_position.window_type);
	pack->mutable_egg_cell()->set_cell(egg_position.cell);
	pack->set_pet_name(name);

	return Send(pack);
}

bool CPythonNetworkStream::SendPetAttrRefineInfoPacket(BYTE index)
{
	CGOutputPacket<CGPetAttrRefineInfoPacket> pack;
	pack->set_index(index);

	return Send(pack);
}

bool CPythonNetworkStream::SendPetEvolutionInfoPacket()
{
	return Send(TCGHeader::PET_EVOLUTION_INFO);
}

bool CPythonNetworkStream::SendPetEvolvePacket()
{
	return Send(TCGHeader::PET_EVOLVE);
}

bool CPythonNetworkStream::SendPetResetSkillPacket(::TItemPos reset_item_position, BYTE skill_index)
{
	CGOutputPacket<CGPetResetSkillPacket> pack;
	pack->mutable_reset_cell()->set_window_type(reset_item_position.window_type);
	pack->mutable_reset_cell()->set_cell(reset_item_position.cell);
	pack->set_skill_index(skill_index);

	return Send(pack);
}
#endif

bool CPythonNetworkStream::RecvUpdateCharacterScale(std::unique_ptr<GCUpdateCharacterScalePacket> pack)
{
	CInstanceBase* pInst = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	if (pInst)
		pInst->GetGraphicThingInstancePtr()->Scale(pack->scale(), pack->scale(), pack->scale());

	return true;
}
