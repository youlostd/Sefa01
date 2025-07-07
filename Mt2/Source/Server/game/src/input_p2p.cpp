#include "stdafx.h" 
#ifdef __DEPRECATED_BILLING__
#include "../../common/billing.h"
#endif
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "party.h"
#include "messenger_manager.h"
#include "empire_text_convert.h"
#include "unique_item.h"
#include "xmas_event.h"
#include "affect.h"
#include "dev_log.h"
#include "questmanager.h"
#include "skill.h"
#include "dungeon.h"
#include "gm.h"
#include "item_manager.h"
#include "item.h"
#include "log.h"

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif
#ifdef DMG_RANKING
#include "dmg_ranking.h"
#endif
#ifdef LOCALE_SAVE_LAST_USAGE
#include "locale.h"
#endif
#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#endif

using namespace network;

extern void __StartNewShutdown(int iStartSec, bool bIsMaintenance, int iMaintenanceDuration, bool bSendP2P);
extern void __StopCurrentShutdown(bool bSendP2P);

ACMD(do_reload);
ACMD(do_flush);

////////////////////////////////////////////////////////////////////////////////
// Input Processor
void CInputP2P::Login(LPDESC d, std::unique_ptr<GGLoginPacket> p)
{
	P2P_MANAGER::instance().Login(d, std::move(p));
}

void CInputP2P::Logout(LPDESC d, std::unique_ptr<GGLogoutPacket> p)
{
	P2P_MANAGER::instance().Logout(p->name().c_str());
}

void CInputP2P::Relay(LPDESC d, std::unique_ptr<GGRelayPacket> p)
{
	sys_log(0, "InputP2P::Relay : %s %u relay_header %u size %d", p->name().c_str(), p->pid(), p->relay_header(), p->ByteSize());

	LPCHARACTER pkChr = p->name().empty() ? CHARACTER_MANAGER::instance().FindByPID(p->pid()) : CHARACTER_MANAGER::instance().FindPC(p->name().c_str());
	if (!pkChr)
		return;

	auto header = static_cast<TGCHeader>(p->relay_header());

	if (header == TGCHeader::WHISPER)
	{
		GCOutputPacket<GCWhisperPacket> send_pack;
		send_pack->ParseFromArray(p->relay().data(), p->relay().size());

		if (test_server)
			sys_log(0, "Relay Whisper from %s to %s type %d", send_pack->name_from().c_str(), p->name().c_str(), send_pack->type());

		if (pkChr->IsBlockMode(BLOCK_WHISPER) && send_pack->type() != WHISPER_TYPE_TARGET_BLOCKED)
		{
			d->SetRelay(send_pack->name_from().c_str());

			send_pack->set_type(WHISPER_TYPE_TARGET_BLOCKED);
			send_pack->set_name_from(p->name());
			d->Packet(send_pack);

			return;
		}

		char buf[1024];
		strcpy(buf, send_pack->message().c_str());

		BYTE bType = send_pack->type();
		BYTE bToEmpire = (bType >> 4);
		bType = bType & 0x0F;
		if(bType == 0x0F) {
			bType = WHISPER_TYPE_SYSTEM;
		}
		else
		{
			if (!pkChr->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE) && g_bEmpireWhisper)
				if (bToEmpire >= 1 && bToEmpire <= 3 && pkChr->GetEmpire() != bToEmpire)
				{
					ConvertEmpireText(bToEmpire,
							buf, 
							send_pack->message().length(),
							10+2*pkChr->GetSkillPower(SKILL_LANGUAGE1 + bToEmpire - 1));

					send_pack->set_message(buf);
				}
		}

		send_pack->set_type(bType);

		pkChr->GetDesc()->Packet(send_pack);
	}
	else
		pkChr->GetDesc()->DirectPacket(header, p->relay().data(), p->relay().size());
}

void CInputP2P::Notice(LPDESC d, std::unique_ptr<GGNoticePacket> p)
{
	if (p->big_font())
		SendBigNotice(p->message().c_str(), p->empire());
	else if (!p->channel() || p->channel() == g_bChannel)
		SendNotice(p->message().c_str(), p->lang_id());
}

void CInputP2P::SuccessNotice(LPDESC d, std::unique_ptr<GGSuccessNoticePacket> p)
{
	SendSuccessNotice(p->message().c_str(), p->lang_id());
}

void CInputP2P::Guild(LPDESC d, const InputPacket& packet)
{
	switch (packet.get_header<TGGHeader>())
	{
		case TGGHeader::GUILD_CHAT:
			{
				auto p = packet.get<GGGuildChatPacket>();

				CGuild* g = CGuildManager::instance().FindGuild(p->guild_id());
				if (g)
					g->P2PChat(p->message().c_str());

				return;
			}
			
		case TGGHeader::GUILD_SET_MEMBER_COUNT_BONUS:
			{
				auto p = packet.get<GGGuildSetMemberCountBonusPacket>();

				CGuild* pGuild = CGuildManager::instance().FindGuild(p->guild_id());
				if (pGuild)
				{
					pGuild->SetMemberCountBonus(p->bonus());
				}
				return;
			}

		case TGGHeader::GUILD_CHANGE_NAME:
			{
				auto p = packet.get<GGGuildChangeNamePacket>();

				CGuild* pGuild = CGuildManager::instance().FindGuild(p->guild_id());
				if (pGuild)
					pGuild->P2P_ChangeName(p->name().c_str());

				return;
			}

		default:
			sys_err ("UNKNOWN GUILD SUB PACKET");
			break;
	}
}

struct FuncShout
{
	const char * m_str;
	BYTE m_bEmpire;

	FuncShout(const char * str, BYTE bEmpire) : m_str(str), m_bEmpire(bEmpire)
	{
	}   

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter() || (d->GetCharacter()->GetGMLevel() == GM_PLAYER && m_bEmpire != 0 && d->GetEmpire() != m_bEmpire))
			return;

		d->GetCharacter()->ChatPacket(CHAT_TYPE_SHOUT, "%s", m_str);
	}
};

void SendShout(const char * szText, BYTE bEmpire)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), FuncShout(szText, bEmpire));
}

void CInputP2P::RecvShutdown(LPDESC d, std::unique_ptr<GGRecvShutdownPacket> p)
{
	if (p->start_sec() < 0)
	{
		// TODO implement advanced maintenance		
		__StopCurrentShutdown(false);
	}
	else
	{
		sys_err("Accept shutdown p2p command from %s.", d->GetHostName());
		// TODO implement advanced maintenance
		__StartNewShutdown(p->start_sec(), p->maintenance(), p->maintenance_duration(), false);
	}
}

void CInputP2P::Shout(std::unique_ptr<GGShoutPacket> p)
{
	SendShout(p->text().c_str(), p->empire());
}

void CInputP2P::Disconnect(std::unique_ptr<GGDisconnectPacket> p)
{
	LPDESC d = DESC_MANAGER::instance().FindByLoginName(p->login().c_str());

	if (!d)
		return;

	if (!d->GetCharacter())
	{
		d->SetPhase(PHASE_CLOSE);
	}
	else
		d->DisconnectOfSameLogin();
}

void CInputP2P::Setup(LPDESC d, std::unique_ptr<GGSetupPacket> p)
{
	sys_log(0, "P2P: Setup %s:%d", d->GetHostName(), p->port());
	d->SetP2P(d->GetHostName(), p->port(), p->channel());
	d->SetListenPort(p->listen_port());

#ifdef PROCESSOR_CORE
	if (p->processor_core())
		P2P_MANAGER::Instance().SetProcessorCore(d);
#endif

#ifdef AUCTION_SYSTEM
	AuctionManager::instance().on_connect_peer(d);
#endif
}

void CInputP2P::MessengerAdd(std::unique_ptr<GGMessengerAddPacket> p)
{
	sys_log(0, "P2P: Messenger Add %s %s", p->account().c_str(), p->companion().c_str());
	MessengerManager::instance().__AddToList(p->account().c_str(), p->companion().c_str());
}

void CInputP2P::MessengerRemove(std::unique_ptr<GGMessengerRemovePacket> p)
{
	sys_log(0, "P2P: Messenger Remove %s %s", p->account().c_str(), p->companion().c_str());
	MessengerManager::instance().__RemoveFromList(p->account().c_str(), p->companion().c_str());
}

#ifdef ENABLE_MESSENGER_BLOCK
void CInputP2P::MessengerBlockAdd(std::unique_ptr<GGMessengerBlockAddPacket> p)
{
	MessengerManager::instance().__AddToBlockList(p->account().c_str(), p->companion().c_str());
}

void CInputP2P::MessengerBlockRemove(std::unique_ptr<GGMessengerBlockRemovePacket> p)
{
	MessengerManager::instance().__RemoveFromBlockList(p->account().c_str(), p->companion().c_str());
}
#endif

void CInputP2P::FindPosition(LPDESC d, std::unique_ptr<GGFindPositionPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->target_pid());
	if (ch && (ch->GetMapIndex() < 10000 || p->is_gm()))
	{
		network::GGOutputPacket<network::GGWarpCharacterPacket> pw;
		pw->set_pid(p->from_pid());
		pw->set_target_pid(p->target_pid());
		pw->set_x(ch->GetX());
		pw->set_y(ch->GetY());
		pw->set_map_index(0);
		d->Packet(pw);
	}
}

void CInputP2P::WarpCharacter(std::unique_ptr<GGWarpCharacterPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid());
	if (ch)
	{
		ch->WarpSet(p->x(), p->y(), p->map_index(), p->target_pid());
	}
}

void CInputP2P::GuildWarZoneMapIndex(std::unique_ptr<GGGuildWarZoneMapIndexPacket> p)
{
	CGuildManager & gm = CGuildManager::instance();

	sys_log(0, "P2P: GuildWarZoneMapIndex g1(%u) vs g2(%u), mapIndex(%d)", p->guild_id1(), p->guild_id2(), p->map_index());

	CGuild * g1 = gm.FindGuild(p->guild_id1());
	CGuild * g2 = gm.FindGuild(p->guild_id2());

	if (g1 && g2)
	{
		g1->SetGuildWarMapIndex(p->guild_id2(), p->map_index());
		g2->SetGuildWarMapIndex(p->guild_id1(), p->map_index());
	}
}

void CInputP2P::Transfer(std::unique_ptr<GGTransferPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->name().c_str());

	if (ch)
		ch->WarpSet(p->x(), p->y());
}

void CInputP2P::LoginPing(LPDESC d, std::unique_ptr<GGLoginPingPacket> p)
{
	if (strlen(p->login().c_str()) < 3) // P2P Crack spam...
	{
		sys_err("%s:%d p->login().c_str() : '%s' strlen() DONT BROADCAST PACKET", __FILE__, __LINE__, p->login().c_str(), strlen(p->login().c_str()));
		return;
	}
	else if (!check_name(p->login().c_str())) // P2P Crack spam...
	{
		sys_err("%s:%d check_name(p->login().c_str()) : '%s' failed..  DONT BROADCAST PACKET", __FILE__, __LINE__, p->login().c_str(), strlen(p->login().c_str()));
		return;
	}

#ifdef __DEPRECATED_BILLING__
	SendBillingExpire(p->login().c_str(), BILLING_FREE, 0, NULL, 3);
#endif

	if (!g_pkAuthMasterDesc) // If I am master, I have to broadcast
	{
		network::GGOutputPacket<network::GGLoginPingPacket> pout;
		pout.data = *p;

		P2P_MANAGER::instance().Send(pout, d);
	}
}

// BLOCK_CHAT
void CInputP2P::BlockChat(std::unique_ptr<GGBlockChatPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->name().c_str());

	if (ch)
	{
		sys_log(0, "BLOCK CHAT apply name %s dur %d", p->name().c_str(), p->block_duration());
		ch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, p->block_duration(), 0, true);
	}
	else
	{
		sys_log(0, "BLOCK CHAT fail name %s dur %d", p->name().c_str(), p->block_duration());
	}
}
// END_OF_BLOCK_CHAT
//

void CInputP2P::IamAwake(LPDESC d)
{
	std::string hostNames;
	P2P_MANAGER::instance().GetP2PHostNames(hostNames);
	sys_log(0, "P2P Awakeness check from %s. My P2P connection number is %d. and details...\n%s", d->GetHostName(), P2P_MANAGER::instance().GetDescCount(), hostNames.c_str());
}

void CInputP2P::RequestDungeonWarp(LPDESC d, std::unique_ptr<GGRequestDungeonWarpPacket> p)
{
	LPDUNGEON pkDungeon = CDungeonManager::instance().Create(p->map_index());
	if (!pkDungeon)
	{
		sys_err("P2P::CreateDungeon: cannot create dungeon on map %u", p->map_index());
		return;
	}

	network::GGOutputPacket<network::GGAnswerDungeonWarpPacket> packet;
	packet->set_type(p->type());
	packet->set_player_id(p->player_id());
	packet->set_map_index(pkDungeon->GetMapIndex());
	packet->set_dest_x(p->dest_x());
	packet->set_dest_y(p->dest_y());

	if (packet->dest_x() == 0 && packet->dest_y() == 0)
	{
		PIXEL_POSITION px;
		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(packet->map_index(), 0, px))
		{
			packet->set_dest_x(px.x);
			packet->set_dest_y(px.y);
		}
	}

	d->Packet(packet);
}

struct FWarpToPosition
{
	long lMapIndex;
	long x;
	long y;
	FWarpToPosition(long lMapIndex, long x, long y)
		: lMapIndex(lMapIndex), x(x), y(y)
	{}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER)) {
			return;
		}
		LPCHARACTER ch = (LPCHARACTER)ent;
		if (!ch->IsPC()) {
			return;
		}

		if (ch->GetMapIndex() == lMapIndex)
		{
			ch->Show(lMapIndex, x, y, 0);
			ch->Stop();
		}
		else
		{
			ch->WarpSet(x, y, lMapIndex);
		}
	}
};

void CInputP2P::AnswerDungeonWarp(LPDESC d, std::unique_ptr<GGAnswerDungeonWarpPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->player_id());

	switch (p->type())
	{
	case P2P_DUNGEON_WARP_PLAYER:
		if (ch)
			ch->WarpSet(p->dest_x(), p->dest_y(), p->map_index());
		break;

	case P2P_DUNGEON_WARP_GROUP:
		{
			if (!ch || !ch->GetParty())
			{
				sys_err("cannot warp without group");
				return;
			}
			FWarpToPosition f(p->map_index(), p->dest_x(), p->dest_y());
			ch->GetParty()->SetFlag("dungeon_index", p->map_index());
			ch->GetParty()->ForEachOnMapMember(f, ch->GetMapIndex());
		}
		break;

	case P2P_DUNGEON_WARP_MAP:
		{
			if (!ch)
			{
				sys_err("cannot warp without group");
				return;
			}
			FWarpToPosition f(p->map_index(), p->dest_x(), p->dest_y());
			LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());
			if (pMap)
				pMap->for_each(f);
		}
		break;
	}
}

void CInputP2P::UpdateRights(std::unique_ptr<GGUpdateRightsPacket> p)
{
	if (p->gm_level() > GM_PLAYER)
	{
		TAdminInfo info;
		info.set_authority(p->gm_level());
		info.set_name(p->name());
		info.set_account("[ALL]");
		GM::insert(info);
	}
	else
	{
		GM::remove(p->name().c_str());
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(p->name().c_str());
	if (tch)
		tch->SetGMLevel();

	sys_log(0, "P2P::UpdateRights: update rights to %u of %s", p->gm_level(), p->name().c_str());
}

void CInputP2P::TeamChat(std::unique_ptr<GGTeamChatPacket> p)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

	for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
	{
		if ((*it)->GetCharacter() && (*it)->GetCharacter()->IsGM())
		{
			(*it)->ChatPacket(CHAT_TYPE_TEAM, "%s", p->text().c_str());
		}
	}
}

void CInputP2P::MessengerRequest(std::unique_ptr<GGMessengerRequestPacket> p)
{
	sys_log(0, "P2P: Messenger Request %s %d", p->requestor().c_str(), p->target_pid());
	MessengerManager::instance().RequestToAdd(p->requestor().c_str(), CHARACTER_MANAGER::instance().FindByPID(p->target_pid()));
}

void CInputP2P::MessengerRequestFail(std::unique_ptr<GGMessengerRequestFailPacket> p)
{
	sys_log(0, "P2P: Messenger Request Fail %s %d", p->requestor().c_str(), p->target_pid());
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->requestor().c_str());
	CCI* pCCI = P2P_MANAGER::instance().FindByPID(p->target_pid());
	if (ch && pCCI)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s ´ÔÀ¸·Î ºÎÅÍ Ä£±¸ µî·ÏÀ» °ÅºÎ ´çÇß½À´Ï´Ù."), pCCI->szName);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "messenger_add_fail %s", pCCI->szName);
	}
}

void CInputP2P::PlayerPacket(std::unique_ptr<GGPlayerPacket> p)
{
	const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
	for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
	{
		if (it->second->GetDesc())
		{
			if (it->second->GetDesc()->IsPhase(PHASE_GAME) || it->second->GetDesc()->IsPhase(PHASE_DEAD))
			{
				if ((p->language() == -1 || p->language() == it->second->GetLanguageID()) && (p->empire() == 0 || p->empire() == it->second->GetEmpire()))
				{
					if (test_server)
						sys_log(0, "SendPlayerPacket to [%u] %s: language %d checkLang %d", it->second->GetPlayerID(), it->second->GetName(), it->second->GetLanguageID(), p->language());

					if (it->second->GetDesc())
						it->second->GetDesc()->DirectPacket(static_cast<TGCHeader>(p->relay_header()), p->relay().data(), p->relay().size());
#ifdef __APP_SERVER_V2__
					else if (it->second->GetAppDesc())
						it->second->GetAppDesc()->P2P_OnRecvMessage(*(BYTE*) c_pData, c_pData, p->size());
#endif
				}
			}
		}
	}

	return;
}

void CInputP2P::TeamlerStatus(std::unique_ptr<GGTeamlerStatusPacket> p)
{
	if (p->is_online())
		CHARACTER_MANAGER::instance().AddOnlineTeamler(p->name().c_str(), p->language());
	else
		CHARACTER_MANAGER::instance().RemoveOnlineTeamler(p->name().c_str(), p->language());
}

void CInputP2P::FlushPlayer(std::unique_ptr<GGFlushPlayerPacket> p)
{
	char szArg[256];
	snprintf(szArg, sizeof(szArg), "%d", p->pid());
	do_flush(NULL, szArg, 0, 0);
}

#ifdef __HOMEPAGE_COMMAND__
void CInputP2P::HomepageCommand(std::unique_ptr<GGHomepageCommandPacket> p)
{
	direct_interpret_command(p->command().c_str());
}
#endif

void CInputP2P::PullOfflineMessages(std::unique_ptr<GGPullOfflineMessagesPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid());
	if (ch)
		DBManager::instance().ReturnQuery(QID_PULL_OFFLINE_MESSAGES, p->pid(), NULL, "SELECT sender, message, is_gm, language FROM offline_messages WHERE pid%d ORDER BY date ASC", p->pid());
}

#ifdef __EVENT_MANAGER__
void CInputP2P::EventManagerOpenRegistration(std::unique_ptr<GGEventManagerOpenRegistrationPacket> p)
{
	if (p->event_index() == EVENT_NONE || p->event_index() > EVENT_MAX_NUM)
		return;

	CEventManager::instance().P2P_OpenEventRegistration(p->event_index());
}

void CInputP2P::EventManagerCloseRegistration(std::unique_ptr<GGEventManagerCloseRegistrationPacket> p)
{
	CEventManager::instance().P2P_CloseEventRegistration(p->clear_event_index());
}

void CInputP2P::EventManagerOver()
{
	CEventManager::instance().P2P_OnEventOver();
}

void CInputP2P::EventManagerIgnorePlayer(std::unique_ptr<GGEventManagerIgnorePlayerPacket> p)
{
	CEventManager::instance().P2P_IgnorePlayer(p->pid());
}

void CInputP2P::EventManagerOpenAnnouncement(std::unique_ptr<GGEventManagerOpenAnnouncementPacket> p)
{
	CEventManager::instance().P2P_OpenEventAnnouncement(p->type(), p->tm_stamp());
}

void CInputP2P::EventManagerTagTeamRegister(std::unique_ptr<GGEventManagerTagTeamRegisterPacket> p)
{
	CEventManager::instance().P2P_TagTeam_Register(p->pid1(), p->pid2(), p->groupidx());
}

void CInputP2P::EventManagerTagTeamUnregister(std::unique_ptr<GGEventManagerTagTeamUnregisterPacket> p)
{
	CEventManager::instance().P2P_TagTeam_RemoveRegistration(p->pid1(), p->pid2(), p->groupidx());
}

void CInputP2P::EventManagerTagTeamCreate(std::unique_ptr<GGEventManagerTagTeamCreatePacket> p)
{
	CEventManager::instance().TagTeam_Create(p->teams());
}
#endif

#ifdef COMBAT_ZONE
void CInputP2P::CombatZoneRanking(std::unique_ptr<GGCombatZoneRankingPacket> p)
{
	CCombatZoneManager::instance().InitializeRanking(p->weekly(), p->general());
}
#endif

#ifdef DMG_RANKING
void CInputP2P::DmgRankingUpdate(std::unique_ptr<GGDmgRankingUpdatePacket> p)
{
	CDmgRankingManager::instance().updateDmgRankings(static_cast<TypeDmg>(p->type()), TRankDamageEntry(p->data().name().c_str(), p->data().dmg()));
}
#endif

#ifdef LOCALE_SAVE_LAST_USAGE
void CInputP2P::LocaleUpdateLastUsage(std::unique_ptr<GGLocaleUpdateLastUsagePacket> p)
{
	CLocaleManager::instance().UpdateLastUsage(p->lang_base().c_str());	
}
#endif

void CInputP2P::GiveItem(std::unique_ptr<network::GGGiveItemPacket> p)
{
	std::ostringstream hint;
	hint << "vnum " << p->item().vnum() << " count " << p->item().count();

	if (LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(p->item().owner()))
	{
		if (LPITEM item = pkChr->AutoGiveItem(&p->item()))
		{
			LogManager::instance().ItemLog(pkChr, item, "P2P_GIVE_ITEM", hint.str().c_str());
		}
		else
		{
			LogManager::instance().ItemLog(pkChr, p->item().id(), p->item().vnum(), "P2P_GIVE_ITEM_REFUND", hint.str().c_str());
			ITEM_MANAGER::instance().GiveItemRefund(&p->item());
		}
	}
	else
	{
		if (!p->no_refund())
		{
			LogManager::instance().ItemLog(p->item().owner(), p->item().id(), "P2P_GIVE_ITEM_REFUND", hint.str().c_str(), "unknown", p->item().vnum());
			ITEM_MANAGER::instance().GiveItemRefund(&p->item());
		}
	}
}

void CInputP2P::GiveGold(std::unique_ptr<network::GGGiveGoldPacket> p)
{
	std::ostringstream hint;

	if (LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindByPID(p->pid()))
	{
		hint << "from " << pkChr->GetGold();
		pkChr->PointChange(POINT_GOLD, p->gold());
		hint << " to " << pkChr->GetGold() << " (amount " << p->gold() << ")";

		LogManager::instance().CharLog(pkChr, 0, "P2P_GIVE_GOLD", hint.str().c_str());
	}
	else
	{
		LogManager::instance().CharLog(p->pid(), 0, 0, 0, "P2P_GIVE_GOLD_REFUND", hint.str().c_str(), "unknown");
		ITEM_MANAGER::Instance().GiveGoldRefund(p->pid(), p->gold());
	}
}

#ifdef AUCTION_SYSTEM
void CInputP2P::AuctionInsertItem(LPDESC d, std::unique_ptr<network::GGAuctionInsertItemPacket> p)
{
	auto channel = p->item().channel();
	auto map_index = p->item().map_index();
	AuctionManager::instance().insert_item(d, std::move(*p->mutable_item()), channel, map_index);
}

void CInputP2P::AuctionTakeItem(LPDESC d, std::unique_ptr<network::GGAuctionTakeItemPacket> p)
{
	AuctionManager::instance().take_item(d, p->owner_id(), p->item_id(), p->inventory_pos());
}

void CInputP2P::AuctionBuyItem(LPDESC d, std::unique_ptr<network::GGAuctionBuyItemPacket> p)
{
	if (!AuctionManager::instance().buy_item(d, p->pid(), p->player_name(), p->item_id(), p->paid_gold()))
	{
		network::GGOutputPacket<network::GGGiveGoldPacket> pack;
		pack->set_pid(p->pid());
		pack->set_gold(p->paid_gold());
		d->Packet(pack);
	}
}

void CInputP2P::AuctionTakeGold(LPDESC d, std::unique_ptr<network::GGAuctionTakeGoldPacket> p)
{
	AuctionManager::instance().take_gold(d, p->owner_id(), p->gold());
}

void CInputP2P::AuctionSearchItems(LPDESC d, std::unique_ptr<network::GGAuctionSearchItemsPacket> p)
{
	AuctionManager::instance().search_items(d, p->pid(), p->page(), p->language(), p->options());
}

void CInputP2P::AuctionExtendedSearchItems(LPDESC d, std::unique_ptr<network::GGAuctionExtendedSearchItemsPacket> p)
{
	AuctionManager::instance().search_items(d, p->pid(), p->page(), p->language(), p->options(), p->channel(), p->map_index());
}

void CInputP2P::AuctionMarkShop(LPDESC d, std::unique_ptr<network::GGAuctionMarkShopPacket> p)
{
	AuctionManager::instance().shop_mark(d, p->pid(), p->item_id());
}

void CInputP2P::AuctionAnswerMarkShop(LPDESC d, std::unique_ptr<network::GGAuctionAnswerMarkShopPacket> p)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->pid());
	if (!ch)
		return;

	network::GCOutputPacket<network::GCAuctionMessagePacket> pack;
	if (AuctionShopManager::instance().mark_shop(ch, p->owner_id()))
		pack->set_message("MARK_DONE");
	else
		pack->set_message("MARK_NOT_FOUND");

	ch->GetDesc()->Packet(pack);
}

void CInputP2P::AuctionShopRequestShow(LPDESC d, std::unique_ptr<GGAuctionShopRequestShowPacket> p)
{
	AuctionManager::instance().shop_request_show(p->pid());
}

void CInputP2P::AuctionShopOpen(LPDESC d, std::unique_ptr<network::GGAuctionShopOpenPacket> p)
{
	AuctionManager::instance().shop_open(d, std::move(*p));
}

void CInputP2P::AuctionShopTakeGold(LPDESC d, std::unique_ptr<network::GGAuctionShopTakeGoldPacket> p)
{
	AuctionManager::instance().shop_take_gold(d, p->owner_id(), p->gold());
}

void CInputP2P::AuctionShopSpawn(LPDESC d, std::unique_ptr<network::GGAuctionShopSpawnPacket> p)
{
	AuctionShopManager::instance().spawn_shop(*p);
}

void CInputP2P::AuctionShopDespawn(LPDESC d, std::unique_ptr<network::GGAuctionShopDespawnPacket> p)
{
	AuctionShopManager::instance().despawn_shop(p->owner_id());
}

void CInputP2P::AuctionShopView(LPDESC d, std::unique_ptr<network::GGAuctionShopViewPacket> p)
{
	AuctionManager::instance().shop_view(d, p->player_id(), p->owner_id());
}

void CInputP2P::AuctionShopViewCancel(LPDESC d, std::unique_ptr<network::GGAuctionShopViewCancelPacket> p)
{
	AuctionManager::instance().shop_view_cancel(p->player_id());
}

void CInputP2P::AuctionShopRenew(LPDESC d, std::unique_ptr<network::GGAuctionShopRenewPacket> p)
{
	AuctionManager::instance().shop_reset_timeout(d, p->player_id(), p->timeout());
}

void CInputP2P::AuctionShopClose(LPDESC d, std::unique_ptr<network::GGAuctionShopClosePacket> p)
{
	AuctionManager::instance().shop_close(d, p->player_id(), p->channel(), p->map_index());
}

void CInputP2P::AuctionShopRequestHistory(LPDESC d, std::unique_ptr<network::GGAuctionShopRequestHistoryPacket> p)
{
	AuctionManager::instance().shop_request_history(d, p->player_id());
}

void CInputP2P::AuctionRequestAveragePrice(LPDESC d, std::unique_ptr<network::GGAuctionRequestAveragePricePacket> p)
{
	AuctionManager::instance().request_average_price(d, p->player_id(), p->requestor(), p->vnum(), p->count());
}
#endif

bool CInputP2P::Analyze(LPDESC d, const InputPacket& packet)
{
	if (test_server)
		sys_log(0, "CInputP2P::Anlayze[Header %d]", packet.get_header());

	switch (packet.get_header<TGGHeader>())
	{
		case TGGHeader::SETUP:
			Setup(d, packet.get<GGSetupPacket>());
			break;

		case TGGHeader::LOGIN:
			Login(d, packet.get<GGLoginPacket>());
			break;

		case TGGHeader::LOGOUT:
			Logout(d, packet.get<GGLogoutPacket>());
			break;

		case TGGHeader::RELAY:
			Relay(d, packet.get<GGRelayPacket>());
			break;

		case TGGHeader::NOTICE:
			Notice(d, packet.get<GGNoticePacket>());
			break;

		case TGGHeader::SUCCESS_NOTICE:
			SuccessNotice(d, packet.get<GGSuccessNoticePacket>());
			break;

		case TGGHeader::SHUTDOWN:
			RecvShutdown(d, packet.get<GGRecvShutdownPacket>());
			break;

		case TGGHeader::GUILD_CHAT:
		case TGGHeader::GUILD_SET_MEMBER_COUNT_BONUS:
		case TGGHeader::GUILD_CHANGE_NAME:
			Guild(d, packet);
			break;

		case TGGHeader::SHOUT:
			Shout(packet.get<GGShoutPacket>());
			break;

		case TGGHeader::DISCONNECT:
			Disconnect(packet.get<GGDisconnectPacket>());
			break;

		case TGGHeader::MESSENGER_ADD:
			MessengerAdd(packet.get<GGMessengerAddPacket>());
			break;

		case TGGHeader::MESSENGER_REMOVE:
			MessengerRemove(packet.get<GGMessengerRemovePacket>());
			break;

#ifdef ENABLE_MESSENGER_BLOCK
		case TGGHeader::MESSENGER_BLOCK_ADD:
			MessengerBlockAdd(packet.get<GGMessengerBlockAddPacket>());
			break;

		case TGGHeader::MESSENGER_BLOCK_REMOVE:
			MessengerBlockRemove(packet.get<GGMessengerBlockRemovePacket>());
			break;
#endif

		case TGGHeader::FIND_POSITION:
			FindPosition(d, packet.get<GGFindPositionPacket>());
			break;

		case TGGHeader::WARP_CHARACTER:
			WarpCharacter(packet.get<GGWarpCharacterPacket>());
			break;

		case TGGHeader::GUILD_WAR_ZONE_MAP_INDEX:
			GuildWarZoneMapIndex(packet.get<GGGuildWarZoneMapIndexPacket>());
			break;

		case TGGHeader::TRANSFER:
			Transfer(packet.get<GGTransferPacket>());
			break;

		case TGGHeader::LOGIN_PING:
			LoginPing(d, packet.get<GGLoginPingPacket>());
			break;

		case TGGHeader::BLOCK_CHAT:
			BlockChat(packet.get<GGBlockChatPacket>());
			break;

		case TGGHeader::AWAKENESS:
			IamAwake(d);
			break;

		case TGGHeader::REQUEST_DUNGEON_WARP:
			RequestDungeonWarp(d, packet.get<GGRequestDungeonWarpPacket>());
			break;

		case TGGHeader::ANSWER_DUNGEON_WARP:
			AnswerDungeonWarp(d, packet.get<GGAnswerDungeonWarpPacket>());
			break;

		case TGGHeader::UPDATE_RIGHTS:
			UpdateRights(packet.get<GGUpdateRightsPacket>());
			break;

		case TGGHeader::RELOAD_COMMAND:
			{
				auto reload_pack = packet.get<GGReloadCommandPacket>();
				do_reload(NULL, reload_pack->argument().c_str(), 0, 0);
			}
			break;

		case TGGHeader::TEAM_CHAT:
			TeamChat(packet.get<GGTeamChatPacket>());
			break;

		case TGGHeader::MESSENGER_REQUEST:
			MessengerRequest(packet.get<GGMessengerRequestPacket>());
			break;

		case TGGHeader::MESSENGER_REQUEST_FAIL:
			MessengerRequestFail(packet.get<GGMessengerRequestFailPacket>());
			break;

		case TGGHeader::PLAYER_PACKET:
			PlayerPacket(packet.get<GGPlayerPacket>());
			break;

		case TGGHeader::FORCE_ITEM_DELETE:
		{
			auto item_del_pack = packet.get<GGForceItemDeletePacket>();
			LPITEM pkItem = ITEM_MANAGER::instance().Find(item_del_pack->item_id());
			if (pkItem)
			{
				pkItem->SetSkipSave(false);
				ITEM_MANAGER::instance().RemoveItem( pkItem, "FORCE_ITEM_DELETE" );
			}
		}
		break;
		
		case TGGHeader::TEAMLER_STATUS:
			TeamlerStatus(packet.get<GGTeamlerStatusPacket>());
			break;

		case TGGHeader::FLUSH_PLAYER:
			FlushPlayer(packet.get<GGFlushPlayerPacket>());
			break;

#ifdef __HOMEPAGE_COMMAND__
		case TGGHeader::HOMEPAGE_COMMAND:
			HomepageCommand(packet.get<GGHomepageCommandPacket>());
			break;
#endif

		case TGGHeader::PULL_OFFLINE_MESSAGES:
			PullOfflineMessages(packet.get<GGPullOfflineMessagesPacket>());
			break;

#ifdef __EVENT_MANAGER__
		case TGGHeader::EVENT_MANAGER_OPEN_REGISTRATION:
			EventManagerOpenRegistration(packet.get<GGEventManagerOpenRegistrationPacket>());
			break;

		case TGGHeader::EVENT_MANAGER_CLOSE_REGISTRATION:
			EventManagerCloseRegistration(packet.get<GGEventManagerCloseRegistrationPacket>());
			break;

		case TGGHeader::EVENT_MANAGER_OVER:
			EventManagerOver();
			break;

		case TGGHeader::EVENT_MANAGER_IGNORE_PLAYER:
			EventManagerIgnorePlayer(packet.get<GGEventManagerIgnorePlayerPacket>());
			break;

		case TGGHeader::EVENT_MANAGER_OPEN_ANNOUNCEMENT:
			EventManagerOpenAnnouncement(packet.get<GGEventManagerOpenAnnouncementPacket>());
			break;
			
		case TGGHeader::EVENT_MANAGER_TAG_TEAM_REGISTER:
			EventManagerTagTeamRegister(packet.get<GGEventManagerTagTeamRegisterPacket>());
			break;

		case TGGHeader::EVENT_MANAGER_TAG_TEAM_UNREGISTER:
			EventManagerTagTeamUnregister(packet.get<GGEventManagerTagTeamUnregisterPacket>());
			break;

		case TGGHeader::EVENT_MANAGER_TAG_TEAM_CREATE:
			EventManagerTagTeamCreate(packet.get<GGEventManagerTagTeamCreatePacket>());
			break;
#endif

#ifdef COMBAT_ZONE
		case TGGHeader::COMBAT_ZONE_RANKING:
			CombatZoneRanking(packet.get<GGCombatZoneRankingPacket>());
			break;
#endif

#ifdef DMG_RANKING
		case TGGHeader::DMG_RANKING_UPDATE:
			DmgRankingUpdate(packet.get<GGDmgRankingUpdatePacket>());
			break;
#endif

#ifdef LOCALE_SAVE_LAST_USAGE
		case TGGHeader::LOCALE_UPDATE_LAST_USAGE:
			LocaleUpdateLastUsage(packet.get<GGLocaleUpdateLastUsagePacket>());
			break;
#endif

		case TGGHeader::GIVE_ITEM:
			GiveItem(packet.get<GGGiveItemPacket>());
			break;

		case TGGHeader::GIVE_GOLD:
			GiveGold(packet.get<GGGiveGoldPacket>());
			break;

#ifdef AUCTION_SYSTEM
		case TGGHeader::AUCTION_INSERT_ITEM:
			AuctionInsertItem(d, packet.get<GGAuctionInsertItemPacket>());
			break;

		case TGGHeader::AUCTION_TAKE_ITEM:
			AuctionTakeItem(d, packet.get<GGAuctionTakeItemPacket>());
			break;

		case TGGHeader::AUCTION_BUY_ITEM:
			AuctionBuyItem(d, packet.get<GGAuctionBuyItemPacket>());
			break;

		case TGGHeader::AUCTION_TAKE_GOLD:
			AuctionTakeGold(d, packet.get<GGAuctionTakeGoldPacket>());
			break;

		case TGGHeader::AUCTION_SEARCH_ITEMS:
			AuctionSearchItems(d, packet.get<GGAuctionSearchItemsPacket>());
			break;

		case TGGHeader::AUCTION_EXTENDED_SEARCH_ITEMS:
			AuctionExtendedSearchItems(d, packet.get<GGAuctionExtendedSearchItemsPacket>());
			break;

		case TGGHeader::AUCTION_MARK_SHOP:
			AuctionMarkShop(d, packet.get<GGAuctionMarkShopPacket>());
			break;

		case TGGHeader::AUCTION_ANSWER_MARK_SHOP:
			AuctionAnswerMarkShop(d, packet.get<GGAuctionAnswerMarkShopPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_REQUEST_SHOW:
			AuctionShopRequestShow(d, packet.get<GGAuctionShopRequestShowPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_OPEN:
			AuctionShopOpen(d, packet.get<GGAuctionShopOpenPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_TAKE_GOLD:
			AuctionShopTakeGold(d, packet.get<GGAuctionShopTakeGoldPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_SPAWN:
			AuctionShopSpawn(d, packet.get<GGAuctionShopSpawnPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_DESPAWN:
			AuctionShopDespawn(d, packet.get<GGAuctionShopDespawnPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_VIEW:
			AuctionShopView(d, packet.get<GGAuctionShopViewPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_VIEW_CANCEL:
			AuctionShopViewCancel(d, packet.get<GGAuctionShopViewCancelPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_RENEW:
			AuctionShopRenew(d, packet.get<GGAuctionShopRenewPacket>());
			break;

		case TGGHeader::AUCTION_SHOP_CLOSE:
			AuctionShopClose(d, packet.get<GGAuctionShopClosePacket>());
			break;

		case TGGHeader::AUCTION_SHOP_REQUEST_HISTORY:
			AuctionShopRequestHistory(d, packet.get<GGAuctionShopRequestHistoryPacket>());
			break;

		case TGGHeader::AUCTION_REQUEST_AVG_PRICE:
			AuctionRequestAveragePrice(d, packet.get<GGAuctionRequestAveragePricePacket>());
			break;
#endif

		default:
			sys_err("invalid header on p2p phase %u", packet.get_header());
			return false;
	}

	return true;
}

