#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "input.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "cmd.h"
#include "buffer_manager.h"
#include "protocol.h"
#include "pvp.h"
#include "start_position.h"
#include "messenger_manager.h"
#include "guild_manager.h"
#include "party.h"
#include "dungeon.h"
#include "war_map.h"
#include "questmanager.h"
#include "building.h"
#include "wedding.h"
#include "affect.h"
#include "arena.h"
#include "OXEvent.h"
#include "priv_manager.h"
#include "dev_log.h"
#include "log.h"
#include "MarkManager.h"
#include "gm.h"

#include "item.h"
#include "rune_manager.h"

#ifdef __PET_SYSTEM__
#include "item_manager.h"
#include "item.h"
#include "PetSystem.h"
#endif

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraDungeon.h"
#endif

#ifdef ENABLE_SPAM_FILTER
#include "SpamFilter.h"
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

using namespace network;

static void _send_bonus_info(LPCHARACTER ch)
{
	int	item_drop_bonus = 0;
	int gold_drop_bonus = 0;
	int gold10_drop_bonus	= 0;
	int exp_bonus		= 0;
	int mob_bonus		= 0;
	int stone_bonus		= 0;
	int boss_bonus		= 0;

	item_drop_bonus		= CPrivManager::instance().GetPriv(ch, PRIV_ITEM_DROP);
	gold_drop_bonus		= CPrivManager::instance().GetPriv(ch, PRIV_GOLD_DROP);
	gold10_drop_bonus	= CPrivManager::instance().GetPriv(ch, PRIV_GOLD10_DROP);
	exp_bonus			= CPrivManager::instance().GetPriv(ch, PRIV_EXP_PCT);
	mob_bonus			= CPrivManager::instance().GetPriv(ch, PRIV_MOB_PCT);
	stone_bonus			= CPrivManager::instance().GetPriv(ch, PRIV_STONE_PCT);
	boss_bonus			= CPrivManager::instance().GetPriv(ch, PRIV_BOSS_PCT);

	if (item_drop_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, 
				LC_TEXT(ch, "¾ÆÀÌÅÛ µå·Ó·ü  %d%% Ãß°¡ ÀÌº¥Æ® ÁßÀÔ´Ï´Ù."), item_drop_bonus);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 6");
	}
	if (gold_drop_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, 
				LC_TEXT(ch, "°ñµå µå·Ó·ü %d%% Ãß°¡ ÀÌº¥Æ® ÁßÀÔ´Ï´Ù."), gold_drop_bonus);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 8");
	}
	if (gold10_drop_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, 
				LC_TEXT(ch, "´ë¹Ú°ñµå µå·Ó·ü %d%% Ãß°¡ ÀÌº¥Æ® ÁßÀÔ´Ï´Ù."), gold10_drop_bonus);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 8");
	}
	if (exp_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, 
				LC_TEXT(ch, "°æÇèÄ¡ %d%% Ãß°¡ È¹µæ ÀÌº¥Æ® ÁßÀÔ´Ï´Ù."), exp_bonus);

		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 7");
	}
	if (mob_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(ch, "Your empire got +%d%% Strong against Monster bonus!"), mob_bonus);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 15");
	}
	if (stone_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(ch, "Your empire got +%d%% Strong against Metin bonus!"), stone_bonus);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 16");
	}
	if (boss_bonus)
	{
		ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(ch, "Your empire got +%d%% Strong against Boss bonus!"), boss_bonus);
		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 17");
	}
	if (quest::CQuestManager::instance().GetEventFlag("event_easter_running"))
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 18");
	}
	if (quest::CQuestManager::instance().GetEventFlag("event_ball_running"))
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "open_event_icon 13");
	}
}

static bool FN_is_battle_zone(LPCHARACTER ch)
{
	switch (ch->GetMapIndex())
	{
		case HOME_MAP_INDEX_RED_1:		 // ½Å¼ö 1Â÷ ¸¶À»
		case HOME_MAP_INDEX_RED_2:		 // ½Å¼ö 2Â÷ ¸¶À»
		case HOME_MAP_INDEX_YELLOW_1:		// ÃµÁ¶ 1Â÷ ¸¶À»
		case HOME_MAP_INDEX_YELLOW_2:		// ÃµÁ¶ 2Â÷ ¸¶À»
		case HOME_MAP_INDEX_BLUE_1:		// Áø³ë 1Â÷ ¸¶À»
		case HOME_MAP_INDEX_BLUE_2:		// Áø³ë 2Â÷ ¸¶À»
		case OXEVENT_MAP_INDEX:	   // OX ¸Ê
			return false;
	}

	return true;
}

#ifdef CHANGE_SKILL_COLOR
static inline void sendCustomizeUnlockSkills(LPCHARACTER ch)
{
	for (int i = 0; i < ESkillColorLength::MAX_SKILL_COUNT; ++i)
	{
		if (ch->GetQuestFlag("skill_color" + std::to_string(i) + ".unlocked"))
			ch->ChatPacket(CHAT_TYPE_COMMAND, "SkillColorUnlocked %d", i);
	}
}
#endif

void CInputLogin::LoginByKey(LPDESC d, std::unique_ptr<CGLoginByKeyPacket> pinfo)
{
	if (pinfo->client_key_size() != PACK_CLIENT_KEY_COUNT)
		return;

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(pinfo->login().c_str(), login, sizeof(login));

	if (g_bNoMoreClient)
	{
		network::GCOutputPacket<network::GCLoginFailurePacket> failurePacket;

		failurePacket->set_status("SHUTDOWN");
		d->Packet(failurePacket);
		return;
	}

	// limit pvp server
	/*if (pvp_server)
	{
		int iTotal;
		int * paiEmpireUserCount;
		int iLocal;
		int max = quest::CQuestManager::instance().GetEventFlag("pvp_server_max");

		DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

		if (MAX(max, 100) <= iTotal)
		{
			network::GCOutputPacket<network::GCLoginFailurePacket> failurePacket;

			failurePacket->set_status("FULL");

			d->Packet(failurePacket);
			return;
		}
	}*/

	sys_log(0, "LOGIN_BY_KEY: %s key %u", login, pinfo->login_key());

	d->SetLoginKey(pinfo->login_key());
//#ifndef _IMPROVED_PACKET_ENCRYPTION_
//	d->SetSecurityKey(pinfo->adw_client_key());
//#endif

	network::GDOutputPacket<network::GDLoginByKeyPacket> ptod;

	ptod->set_login(login);
	ptod->set_login_key(pinfo->login_key());
	for (auto& client_key : pinfo->client_key())
		ptod->add_client_key(client_key);
	ptod->set_ip(d->GetHostName());

	db_clientdesc->DBPacket(ptod, d->GetHandle());
}

void CInputLogin::ChangeName(LPDESC d, std::unique_ptr<CGPlayerChangeNamePacket> p)
{
	const TAccountTable & c_r = d->GetAccountTable();

	if (!c_r.id())
	{
		sys_err("no account table");
		return;
	}

	if (!c_r.players(p->index()).change_name())
		return;

	if (!check_name(p->name().c_str()))
	{
		network::GCOutputPacket<network::GCCreateFailurePacket> pack;
		pack->set_type(0);
		d->Packet(pack);
		return;
	}

	network::GDOutputPacket<network::GDChangeNamePacket> pdb;

	pdb->set_pid(c_r.players(p->index()).id());
	pdb->set_name(p->name());
	db_clientdesc->DBPacket(pdb, d->GetHandle());
}

void CInputLogin::CharacterSelect(LPDESC d, std::unique_ptr<CGPlayerSelectPacket> pinfo)
{
	const TAccountTable & c_r = d->GetAccountTable();

	sys_log(0, "player_select: login: %s index: %d", c_r.login().c_str(), pinfo->index());

	if (!c_r.id())
	{
		sys_err("no account table");
		return;
	}

	if (pinfo->index() >= PLAYER_PER_ACCOUNT)
	{
		sys_err("index overflow %d, login: %s", pinfo->index(), c_r.login().c_str());
		return;
	}

	if (c_r.players(pinfo->index()).change_name())
	{
		sys_err("name must be changed idx %d, login %s, name %s", 
				pinfo->index(), c_r.login().c_str(), c_r.players(pinfo->index()).name().c_str());
		return;
	}

#ifdef __HAIR_SELECTOR__
	d->SaveHairBases();
#endif

	GDOutputPacket<GDPlayerLoadPacket> player_load_packet;
	player_load_packet->set_account_id(c_r.id());
	player_load_packet->set_player_id(c_r.players(pinfo->index()).id());
	player_load_packet->set_account_index(pinfo->index());

	db_clientdesc->DBPacket(player_load_packet, d->GetHandle());
}

bool RaceToJob(unsigned race, unsigned* ret_job)
{
	*ret_job = 0;

	if (race >= MAIN_RACE_MAX_NUM)
		return false;

	switch (race)
	{
		case MAIN_RACE_WARRIOR_M:
			*ret_job = JOB_WARRIOR;
			break;

		case MAIN_RACE_WARRIOR_W:
			*ret_job = JOB_WARRIOR;
			break;

		case MAIN_RACE_ASSASSIN_M:
			*ret_job = JOB_ASSASSIN;
			break;

		case MAIN_RACE_ASSASSIN_W:
			*ret_job = JOB_ASSASSIN;
			break;

		case MAIN_RACE_SURA_M:
			*ret_job = JOB_SURA;
			break;

		case MAIN_RACE_SURA_W:
			*ret_job = JOB_SURA;
			break;

		case MAIN_RACE_SHAMAN_M:
			*ret_job = JOB_SHAMAN;
			break;

		case MAIN_RACE_SHAMAN_W:
			*ret_job = JOB_SHAMAN;
			break;

#ifdef __WOLFMAN__
		case MAIN_RACE_WOLFMAN_M:
			*ret_job = JOB_WOLFMAN;
			break;
#endif

		default:
			return false;
			break;
	}
	return true;
}

#ifdef __HAIR_SELECTOR__
void CInputLogin::PlayerHairSelect(LPDESC d, std::unique_ptr<CGPlayerHairSelectPacket> pinfo)
{
	TAccountTable & r = d->GetAccountTable();

	sys_log(0, "player_select_hair: login: %s index: %d hair %u", r.login().c_str(), pinfo->index(), pinfo->hair_vnum());

	if (!r.id())
	{
		sys_err("no account table");
		return;
	}

	if (pinfo->index() >= PLAYER_PER_ACCOUNT)
	{
		sys_err("index overflow %d, login: %s", pinfo->index(), r.login().c_str());
		return;
	}

	const DWORD c_adwHairAllow[JOB_MAX_NUM][2] = {
		{73001, 73012},
		{73251, 73262},
		{73501, 73512},
		{73751, 73762},
#ifdef __WOLFMAN__
		{0, 0},
#endif
	};

	if (r.players(pinfo->index()).id() == 0)
	{
		sys_err("invalid player %u (no id)", pinfo->index());
		return;
	}

	unsigned int Job;
	RaceToJob(r.players(pinfo->index()).job(), &Job);

	if (pinfo->hair_vnum())
	{
		if (pinfo->hair_vnum() < c_adwHairAllow[Job][0] || pinfo->hair_vnum() > c_adwHairAllow[Job][1])
		{
			sys_err("invalid hair %u for job %u (race %d)", pinfo->hair_vnum(), Job, r.players(pinfo->index()).job());
			return;
		}
	}

	d->SaveOldHairBase(pinfo->index());
	r.mutable_players(pinfo->index())->set_hair_base_part(pinfo->hair_vnum());

	const TItemTable* c_pItemTab = pinfo->hair_vnum() ? ITEM_MANAGER::instance().GetTable(pinfo->hair_vnum()) : NULL;

	network::GDOutputPacket<network::GDSelectUpdateHairPacket> pack;
	pack->set_pid(r.players(pinfo->index()).id());
	pack->set_hair_base_part(pinfo->hair_vnum());
	pack->set_hair_part(c_pItemTab ? c_pItemTab->values(3) : 0);

	DWORD dwOldHair = d->GetOldHairBase(pinfo->index());
	c_pItemTab = dwOldHair ? ITEM_MANAGER::instance().GetTable(dwOldHair) : NULL;
	if (r.players(pinfo->index()).hair_part() != 0 && (!c_pItemTab || c_pItemTab->values(3) != r.players(pinfo->index()).hair_part()))
		pack->set_hair_part(r.players(pinfo->index()).hair_part());

	db_clientdesc->DBPacket(pack);

	if (test_server)
		sys_log(0, "hair set : changed %d old %u new %u", d->IsHairBaseChanged(pinfo->index()), d->GetOldHairBase(pinfo->index()), pinfo->hair_vnum());
}
#endif

bool NewPlayerTable(TPlayerTable * table,
		const char * name,
		BYTE race,
		BYTE shape,
		BYTE bEmpire)
{
	if (race >= MAIN_RACE_MAX_NUM)
		return false;

	unsigned job;

	if (!RaceToJob(race, &job))
	{
		sys_err("NewPlayerTable.RACE_TO_JOB_ERROR(%d)\n", race);
		return false;
	}

	table->Clear();
	table->set_name(name);

	table->set_level(1);
	table->set_job(race);	// Á÷¾÷´ë½Å Á¾Á·À» ³Ö´Â´Ù
	table->set_part_base(shape);

	table->set_st(JobInitialPoints[job].st);
	table->set_dx(JobInitialPoints[job].dx);
	table->set_ht(JobInitialPoints[job].ht);
	table->set_iq(JobInitialPoints[job].iq);

	table->set_hp(JobInitialPoints[job].max_hp + table->ht() * JobInitialPoints[job].hp_per_ht);
	table->set_sp(JobInitialPoints[job].max_sp + table->iq() * JobInitialPoints[job].sp_per_iq);

	table->set_x(CREATE_START_X(bEmpire) + random_number(-300, 300));
	table->set_y(CREATE_START_Y(bEmpire) + random_number(-300, 300));

	return true;
}

void CInputLogin::CharacterCreate(LPDESC d, std::unique_ptr<CGPlayerCreatePacket> pinfo)
{
	GDOutputPacket<GDPlayerCreatePacket> player_create_packet;
	
	sys_log(0, "PlayerCreate: name %s pos %d job %d shape %d",
			pinfo->name().c_str(), 
			pinfo->index(), 
			pinfo->job(), 
			pinfo->shape());

	network::GCOutputPacket<network::GCCreateFailurePacket> packFailure;

	if (!GM::check_account_allow(d->GetAccountTable().login(), GM_ALLOW_CREATE_PLAYER))
	{
		d->Packet(packFailure);
		return;
	}

	// »ç¿ëÇÒ ¼ö ¾ø´Â ÀÌ¸§ÀÌ°Å³ª, Àß¸øµÈ Æò»óº¹ÀÌ¸é »ý¼³ ½ÇÆÐ
	if (!check_name(pinfo->name().c_str()) || pinfo->shape() > 1
#ifdef ENABLE_SPAM_FILTER
		|| CSpamFilter::Instance().IsBannedWord(pinfo->name())
#endif
		)
	{
		d->Packet(packFailure);
		return;
	}

	const TAccountTable & c_rAccountTable = d->GetAccountTable();

	if (0 == strcmp(c_rAccountTable.login().c_str(), pinfo->name().c_str()))
	{
		packFailure->set_type(1);

		d->Packet(packFailure);
		return;
	}

	if (!NewPlayerTable(player_create_packet->mutable_player_table(), pinfo->name().c_str(), pinfo->job(), pinfo->shape(), d->GetEmpire()))
	{
		sys_err("player_prototype error: job %d face %d ", pinfo->job());
		d->Packet(packFailure);
		return;
	}

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_rAccountTable.login().c_str(), login, sizeof(login));
	player_create_packet->set_login(login);
	player_create_packet->set_passwd(c_rAccountTable.passwd());

	player_create_packet->set_account_id(c_rAccountTable.id());
	player_create_packet->set_account_index(pinfo->index());

	sys_log(0, "PlayerCreate: name %s account_id %d, Packet->Gold %ld",
			pinfo->name().c_str(), 
			pinfo->index(), 
			player_create_packet->player_table().gold());

	db_clientdesc->DBPacket(player_create_packet, d->GetHandle());
}

void CInputLogin::CharacterDelete(LPDESC d, std::unique_ptr<CGPlayerDeletePacket> pinfo)
{
	if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);
	const TAccountTable & c_rAccountTable = d->GetAccountTable();

	if (!c_rAccountTable.id())
	{
		sys_err("PlayerDelete: no login data");
		return;
	}
	if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);

	sys_log(0, "PlayerDelete: login: %s index: %d, social_id %s", c_rAccountTable.login().c_str(), pinfo->index(), pinfo->private_code().c_str());

	if (pinfo->index() >= PLAYER_PER_ACCOUNT)
	{
		if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);
		sys_err("PlayerDelete: index overflow %d, login: %s", pinfo->index(), c_rAccountTable.login().c_str());
		return;
	}
	if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);

	if (!c_rAccountTable.players(pinfo->index()).id())
	{
		if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);
		sys_err("PlayerDelete: Wrong Social ID index %d, login: %s", pinfo->index(), c_rAccountTable.login().c_str());
		d->Packet(TGCHeader::DELETE_FAILURE);
		return;
	}
	if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);

	if (!GM::check_allow(GM::get_level(c_rAccountTable.players(pinfo->index()).name().c_str(), c_rAccountTable.login().c_str(), true), GM_ALLOW_DELETE_PLAYER))
	{
		if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);
		sys_err("PlayerDelete: cannot delete gm character [%s]", c_rAccountTable.players(pinfo->index()).name().c_str());
		d->Packet(TGCHeader::DELETE_FAILURE);
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (d->IsTradeblocked())
	{
		sys_err("PlayerDelete: cannot delete tradeblocked character [%s]", c_rAccountTable.players(pinfo->index()).name().c_str());
		d->Packet(TGCHeader::DELETE_FAILURE);
		return;
	}
#endif

	if (test_server)	sys_err("__ %s:%d %s", __FILE__, __LINE__, __FUNCTION__);
	GDOutputPacket<GDPlayerDeletePacket> player_delete_packet;

	char login[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_rAccountTable.login().c_str(), login, sizeof(login));
	player_delete_packet->set_login(login);
	player_delete_packet->set_player_id(c_rAccountTable.players(pinfo->index()).id());
	player_delete_packet->set_account_index(pinfo->index());
	player_delete_packet->set_private_code(pinfo->private_code());

	db_clientdesc->DBPacket(player_delete_packet, d->GetHandle());
}

#pragma pack(1)
typedef struct SPacketGTLogin
{
	BYTE header;
	WORD empty;
	DWORD id;
} TPacketGTLogin;
#pragma pack()

extern void __SendMaintenancePacketToPlayer(LPCHARACTER pkChr);

void CInputLogin::Entergame(LPDESC d)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		d->SetPhase(PHASE_CLOSE);
		return;
	}
#ifdef ENABLE_ZODIAC_TEMPLE
	bool bIsPremium = ch->GetQuestFlag("auction_premium.premium_active") != 0;
	int iTimePerAnimasphere = bIsPremium ? 1800 : 3600;

	int ANIMASPHERE_CURENT = get_global_time();
	int MYDIFFERENCE = ANIMASPHERE_CURENT - ch->GetQuestFlag("zodiac_animasphere.animasphere_start");
	int ANIMASPHERE_ROUND = abs(MYDIFFERENCE /iTimePerAnimasphere);
	int UPDATE_TIME_LEFT = MYDIFFERENCE - (ANIMASPHERE_ROUND * iTimePerAnimasphere);
	int CHECK_ANIMASPHERE_COUNT = ch->GetAnimasphere() + ANIMASPHERE_ROUND;
	int DIFFERENCE_TO_UPDATE_START = ANIMASPHERE_CURENT - UPDATE_TIME_LEFT;

	if (ch->GetQuestFlag("zodiac_animasphere.animasphere") == 1)
	{
		if (ch->GetAnimasphere() >= 36)
		{
			ch->SetQuestFlag("zodiac_animasphere.animasphere_start", get_global_time());
		}
		else
		{
			if (MYDIFFERENCE >= iTimePerAnimasphere)
			{
				if (CHECK_ANIMASPHERE_COUNT >= 36)
				{
					int NEEDED = 36 - ch->GetAnimasphere();
					ch->PointChange(POINT_ANIMASPHERE, NEEDED, true);
					ch->SetQuestFlag("zodiac_animasphere.animasphere_start", get_global_time());
				}
				else if (CHECK_ANIMASPHERE_COUNT <= 35)
				{
					ch->PointChange(POINT_ANIMASPHERE, ANIMASPHERE_ROUND, true);
					ch->SetQuestFlag("zodiac_animasphere.animasphere_start", DIFFERENCE_TO_UPDATE_START);
				}
			}
		}
	}
	for (int i = 1; i < 24; i++)
		ch->SetZodiacBadges(0, i);
	ch->SetQuestFlag("zodiac.PrismOfRevival", 0);
#endif
	//ch->PointChange(POINT_ANIMASPHERE, 0, true)
	PIXEL_POSITION pos = ch->GetXYZ();

	if (!SECTREE_MANAGER::instance().GetMovablePosition(ch->GetMapIndex(), pos.x, pos.y, pos))
	{
		PIXEL_POSITION pos2;
		SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos2);

		sys_err("!GetMovablePosition (name %s %dx%d map %d changed to %dx%d)", 
				ch->GetName(),
				pos.x, pos.y,
				ch->GetMapIndex(),
				pos2.x, pos2.y);
		pos = pos2;
	}

	CGuildManager::instance().LoginMember(ch);

	ch->Show(ch->GetMapIndex(), pos.x, pos.y, pos.z);

	SECTREE_MANAGER::instance().SendNPCPosition(ch);
	if (!ch->IsGMInvisible())
		ch->ReviveInvisible(5);
	else
		ch->ReviveInvisible(INFINITE_AFFECT_DURATION);

	d->SetPhase(PHASE_GAME);

#ifdef __PET_ADVANCED__
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (LPITEM item = ch->GetInventoryItem(i))
		{
			if (item->GetAdvancedPet())
				item->GetAdvancedPet()->CheckSummonOnLoad();
		}
	}
#endif

#ifdef CRYSTAL_SYSTEM
	ch->check_active_crystal();
#endif

#ifdef __ATTRTREE__
	ch->SendAttrTree();
#endif

	sys_log(0, "ENTERGAME: %s %dx%dx%d %s map_index %d",
		ch->GetName(), ch->GetX(), ch->GetY(), ch->GetZ(), d->GetHostName(), ch->GetMapIndex());

	if(ch->GetItemAward_cmd())																		//°ÔÀÓÆäÀÌÁî µé¾î°¡¸é
		quest::CQuestManager::instance().ItemInformer(ch->GetPlayerID(),ch->GetItemAward_vnum());	//questmanager È£Ãâ

	const TAccountTable& r = ch->GetDesc()->GetAccountTable();

	CHARACTER_MANAGER::instance().SendOnlineTeamlerList(ch->GetDesc(), ch->GetLanguageID());
	if (ch->IsGM())
	{
		if (ch->GetGMLevel() <= GM_HIGH_WIZARD)
		{
			ch->SetIsShowTeamler(true);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You are shown in the teamler list."));
		}
	}

	ch->FinishMountLoading();

#ifdef __ATTRIBUTES_TO_CLIENT__
	ch->SendAttributesToClient();
#endif

#ifdef __FAKE_BUFF__
	ch->SendFakeBuffSkills();
#endif

	ch->PointChange(POINT_ANTI_EXP, 0);
	ch->ResetPlayTime();
	ch->StartSaveEvent();
	ch->StartRecoveryEvent();
	ch->StartUpdateAuraEvent();
	ch->StartCheckSpeedHackEvent();

	CPVPManager::instance().Connect(ch);
	CPVPManager::instance().SendList(d);

#ifdef COMBAT_ZONE
	CCombatZoneManager::instance().OnLogin(ch);
#endif

	MessengerManager::instance().Login(ch->GetName());

	CPartyManager::instance().SetParty(ch);
	CGuildManager::instance().SendGuildWar(ch);

	building::CManager::instance().SendLandList(d, ch->GetMapIndex());

	marriage::CManager::instance().Login(ch);

	ch->SendOfflineMessages();

#ifdef __ITEM_REFUND__
	ch->SendItemRefundCommands();
#endif

	ch->SetRandom(random_number(0, 15));
	
	network::GCOutputPacket<network::GCTimePacket> p;
	p->set_time(get_global_time());
	p->set_random(ch->GetRandom());
	// BYTE marginPercent = quest::CQuestManager::instance().GetEventFlag("shop_margin");
	// p->set_sell_margin(marginPercent ? marginPercent : 20);
	// BYTE marginPercentPositive = quest::CQuestManager::instance().GetEventFlag("shop_margin_positive");
	// p->set_sell_margin_positive(marginPercentPositive ? marginPercentPositive : 20);
#ifdef COMBAT_ZONE
	p->set_combatzone(quest::CQuestManager::instance().GetEventFlag("combat_zone_event"));
#endif
	p->set_channel(g_bChannel);
	p->set_test_server(test_server);
	p->set_map_index(ch->GetMapIndex());
	p->set_coins(r.coins() || quest::CQuestManager::instance().GetEventFlag("preload_browser_all"));
	d->Packet(p);

	_send_bonus_info(ch);
	
	for (int i = 0; i <= PREMIUM_MAX_NUM; ++i)
	{
		int remain = ch->GetPremiumRemainSeconds(i);

		if (remain <= 0)
			continue;
#ifdef ELONIA
		if (i == PREMIUM_EXP)
			continue;
#endif
		ch->AddAffect(AFFECT_PREMIUM_START + i, POINT_NONE, 0, 0, remain, 0, true);
		sys_log(!test_server, "PREMIUM: %s type %d %dmin", ch->GetName(), i, remain);
	}

#ifdef __EVENT_MANAGER__
	CEventManager::instance().OnPlayerLogin(ch);
#endif

#ifdef ENABLE_HYDRA_DUNGEON
	CHydraDungeonManager::instance().OnLogin(ch);
#endif

/*	if (quest::CQuestManager::instance().GetEventFlag("event_anniversary_running"))
	{
		std::string sendMsg = "ANGELDEMONEVENTSTATUS";
		for (BYTE i = 0; i < 7; ++i)
			sendMsg += " " + std::to_string(quest::CQuestManager::instance().GetEventFlag("event_anniversary_winner_d" + std::to_string(i)));
		d->ChatPacket(CHAT_TYPE_COMMAND, sendMsg.c_str());
	}

	for (BYTE it = 0; it < 7; ++it)
	{
		int bonusEnd = quest::CQuestManager::instance().GetEventFlag("event_anniversary_boniend_d" + std::to_string(it));
		int winner = quest::CQuestManager::instance().GetEventFlag("event_anniversary_winner_d" + std::to_string(it));

		if (bonusEnd > get_global_time() && winner && winner == ch->GetQuestFlag("anniversary_event.selected_fraction"))
		{
			if (it == 0)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_BOSS);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_BOSS, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
			else if (it == 1)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_DUNGEON_CD);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_DUNGEON_CD, 25, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
			else if (it == 2)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_METIN);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_METIN, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
			else if (it == 3)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_BIOLOG_CD);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_LOWER_BIOLOG_CD, 50, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
			else if (it == 4)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_MONSTER);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_MONSTER, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
			else if (it == 5)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_BONUS_UPGRADE_CHANCE);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_BONUS_UPGRADE_CHANCE, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
			else if (it == 6)
			{
				CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_DOUBLE_ITEM_DROP_BONUS);
				if (!selAffect)
					ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_DOUBLE_ITEM_DROP_BONUS, 5, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
		}
	}
*/

/*	if (!quest::CQuestManager::instance().GetEventFlag("event_anniversary_week"))
	{
		int bonusEnd = quest::CQuestManager::instance().GetEventFlag("event_anniversary_weekboniend");
		int winner = quest::CQuestManager::instance().GetEventFlag("event_anniversary_weekwinner");

		if (bonusEnd > get_global_time() && winner && winner == ch->GetQuestFlag("anniversary_event.selected_fraction"))
		{
			CAffect* selAffect = ch->FindAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_EXPBONUS);
			if (!selAffect)
			{
				ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_EXPBONUS, 100, AFF_NONE, bonusEnd - get_global_time(), 0, false);
				ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_ITEMBONUS, 50, AFF_NONE, bonusEnd - get_global_time(), 0, false);
				ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_MALL_GOLDBONUS, 50, AFF_NONE, bonusEnd - get_global_time(), 0, false);
				ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_BOSS, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
				ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_METIN, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
				ch->AddAffect(AFFECT_ANNIVERSARY_EVENT, POINT_ATTBONUS_MONSTER, 10, AFF_NONE, bonusEnd - get_global_time(), 0, false);
			}
		}
	}*/

/*	if (quest::CQuestManager::instance().GetEventFlag("anniversary_disable_pvp_boss"))
		ch->SetBlockPKMode(PK_MODE_PROTECT, true);*/

#ifdef AHMET_FISH_EVENT_SYSTEM
	ch->FishEventGeneralInfo();
#endif

	if (!d->GetClientVersion())
		d->DelayedDisconnect(10);
	else
	{
		if (atoi(g_stClientVersion.c_str()) > atoi(d->GetClientVersion()))
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT(ch, "Å¬¶óÀÌ¾ðÆ® ¹öÀüÀÌ Æ²·Á ·Î±×¾Æ¿ô µË´Ï´Ù. Á¤»óÀûÀ¸·Î ÆÐÄ¡ ÈÄ Á¢¼ÓÇÏ¼¼¿ä."));
			d->DelayedDisconnect(10);
			LogManager::instance().HackLog("VERSION_CONFLICT", ch);

			sys_log(!test_server, "VERSION : WRONG VERSION USER : account:%s name:%s hostName:%s server_version:%s client_version:%s",
					d->GetAccountTable().login().c_str(),
					ch->GetName(),
					d->GetHostName(),
					g_stClientVersion.c_str(),
					d->GetClientVersion());
		}
	}

	if (ch->GetMapIndex() >= 10000)
	{
		if (CWarMapManager::instance().IsWarMap(ch->GetMapIndex()))
			ch->SetWarMap(CWarMapManager::instance().Find(ch->GetMapIndex()));
		else if (marriage::WeddingManager::instance().IsWeddingMap(ch->GetMapIndex()))
			ch->SetWeddingMap(marriage::WeddingManager::instance().Find(ch->GetMapIndex()));
#ifdef __MELEY_LAIR_DUNGEON__
		else if (MeleyLair::CMgr::instance().IsMeleyMap(ch->GetMapIndex()))
			MeleyLair::CMgr::instance().Leave(ch->GetGuild(), ch, true);
#endif
		else {
			ch->SetDungeon(CDungeonManager::instance().FindByMapIndex(ch->GetMapIndex()));
		}
	}

#ifdef __PET_SYSTEM__
	CPetSystem* petSystem;
	if (!(petSystem = ch->GetPetSystem()))
		sys_err("not pet system found");
	else
	{
		LPITEM petItem = ITEM_MANAGER::instance().Find(ch->GetQuestFlag("pet_system.id"));
		if (petItem && petItem->GetOwner() == ch)
			petSystem->Summon(petItem->GetValue(0), petItem, 0, false);
		else
			ch->SetQuestFlag("pet_system.id", 0);
	}
#endif

#ifdef __FAKE_BUFF__
	if (!ch->FakeBuff_Owner_GetSpawn() && ch->GetMapIndex() != EMPIREWAR_MAP_INDEX && !ch->IsPrivateMap(EVENT_LABYRINTH_MAP_INDEX))
	{
		LPITEM buffiItem = ITEM_MANAGER::instance().Find(ch->GetQuestFlag("fake_buff.id"));
		if (buffiItem && buffiItem->GetOwner() == ch)
			ch->FakeBuff_Owner_Spawn(ch->GetX(), ch->GetY(), buffiItem);
		else
			ch->SetQuestFlag("fake_buff.id", 0);
	}
#endif

	if (quest::CQuestManager::instance().GetEventFlag("enable_bosshunt_event"))
	{
		std::string currHuntID = "event_boss_hunt.points" + std::to_string(quest::CQuestManager::instance().GetEventFlag("bosshunt_event_id"));
		ch->ChatPacket(CHAT_TYPE_COMMAND, "BossHuntPoints %d", ch->GetQuestFlag(currHuntID));
	}

#ifdef ENABLE_RUNE_AFFECT_ICONS
	auto currHarvestedSouls = ch->GetQuestFlag("rune_manager.harvested_souls");
	ch->ChatPacket(CHAT_TYPE_COMMAND, "HarvestSould %d", currHarvestedSouls);
#endif
	
#ifdef ENABLE_RUNE_SYSTEM	
	ch->ChatPacket(CHAT_TYPE_COMMAND, "rune_points %u", ch->GetQuestFlag("rune_manager.points_unspent"));
	ch->StartRuneEvent(true);
#endif
	if (quest::CQuestManager::instance().GetEventFlag("disable_dungeon_reconnect") == 0 && CDungeonManager::instance().HasPlayerInfo(ch->GetPlayerID()))
	{
		TDungeonPlayerInfo s = CDungeonManager::instance().GetPlayerInfo(ch->GetPlayerID());
		if (s.map != ch->GetMapIndex() || s.x != ch->GetX())
		{
			int iDistance = DISTANCE_APPROX(s.x - ch->GetX(), s.y - ch->GetY());
			ch->tchat("Distance Login/Logout Point = %i", iDistance);
			if (s.map != ch->GetMapIndex() || iDistance > 8000)
				ch->ChatPacket(CHAT_TYPE_COMMAND, "ASK_DUNGEON_RECONNECT %i %i %i", s.map, s.x, s.y);
		}
	}

#ifdef CHANGE_SKILL_COLOR
	sendCustomizeUnlockSkills(ch);
#endif

	LogManager::instance().ConnectLog(true, ch);

	__SendMaintenancePacketToPlayer(ch);

#ifdef ENABLE_COMPANION_NAME
	ch->SendCompanionNameInfo();
#endif
#ifdef LEADERSHIP_EXTENSION
	if (!ch->GetParty())
		ch->SetLeadershipState(ch->GetQuestFlag("leadershipState.value"));
#endif

#ifdef ACCOUNT_TRADE_BLOCK
	if (ch->GetDesc()->IsHWID2Banned())
		LogManager::instance().HackDetectionLog(ch, "HWID2BANNED", r.hwid2().c_str());
	if (test_server && ch->GetDesc()->IsTradeblocked())
		ch->tchat("trade blocked for %d seconds", ch->GetDesc()->GetAccountTable().tradeblock() - get_global_time());
#endif

#ifdef __EQUIPMENT_CHANGER__
	ch->SendEquipmentChangerLoadPacket(false);
#endif

	// Hotfix genderchange costume bug
	LPITEM bodyCostume = ch->GetWear(WEAR_COSTUME_BODY);
	if (bodyCostume && !ch->check_item_sex(ch, bodyCostume))
	{
		ch->UnequipItem(bodyCostume);
		ch->tchat("unequip body costume");
	}

	LPITEM hairCostume = ch->GetWear(WEAR_COSTUME_HAIR);
	if (hairCostume && !ch->check_item_sex(ch, hairCostume))
	{
		ch->UnequipItem(hairCostume);
		ch->tchat("unequip hair costume");
	}
}

void CInputLogin::Empire(LPDESC d, std::unique_ptr<CGEmpirePacket> p)
{
	if (EMPIRE_MAX_NUM <= p->empire())
	{
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	const TAccountTable& r = d->GetAccountTable();

	if (r.empire() != 0)
	{
		for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		{
			if (0 != r.players(i).id())
			{
				sys_err("EmpireSelectFailed %d", r.players(i).id());
				return;
			}
		}
	}

	GDOutputPacket<GDEmpireSelectPacket> pd;

	pd->set_account_id(r.id());
	pd->set_empire(p->empire());

	db_clientdesc->DBPacket(pd, d->GetHandle());
}

void CInputLogin::GuildSymbolUpload(LPDESC d, std::unique_ptr<CGGuildSymbolUploadPacket> p)
{
	sys_log(0, "GuildSymbolUpload uiBytes %u", p->ByteSize());

	int iSymbolSize = p->image().length();

	if (iSymbolSize <= 0 || iSymbolSize > 64 * 1024)
	{
		// 64k º¸´Ù Å« ±æµå ½Éº¼Àº ¿Ã¸±¼ö¾ø´Ù
		// Á¢¼ÓÀ» ²÷°í ¹«½Ã
		d->SetPhase(PHASE_CLOSE);
		return;
	}

	// ¶¥À» ¼ÒÀ¯ÇÏÁö ¾ÊÀº ±æµåÀÎ °æ¿ì.
	if (!test_server)
		if (!building::CManager::instance().FindLandByGuild(p->guild_id()))
		{
			d->SetPhase(PHASE_CLOSE);
			return;
		}

	CGuildMarkManager::instance().UploadSymbol(p->guild_id(), iSymbolSize, (const BYTE*)(p->image().c_str()));
	CGuildMarkManager::instance().SaveSymbol(GUILD_SYMBOL_FILENAME);
	return;
}

void CInputLogin::GuildSymbolCRC(LPDESC d, std::unique_ptr<CGGuildSymbolCRCPacket> data)
{
	sys_log(0, "GuildSymbolCRC %u %u %u", data->guild_id(), data->crc(), data->symbol_size());

	const CGuildMarkManager::TGuildSymbol * pkGS = CGuildMarkManager::instance().GetGuildSymbol(data->guild_id());

	if (!pkGS)
		return;

	sys_log(0, "  Server %u %u", pkGS->crc, pkGS->raw.size());

	if (pkGS->raw.size() != data->symbol_size() || pkGS->crc != data->crc())
	{
		GCOutputPacket<GCGuildSymbolDataPacket> GCPacket;
		GCPacket->set_guild_id(data->guild_id());
		GCPacket->set_image(&pkGS->raw[0], pkGS->raw.size());

		d->Packet(GCPacket);

		sys_log(0, "SendGuildSymbolHead %02X%02X%02X%02X Size %d",
				pkGS->raw[0], pkGS->raw[1], pkGS->raw[2], pkGS->raw[3], pkGS->raw.size());
	}
}

void CInputLogin::GuildMarkUpload(LPDESC d, std::unique_ptr<CGMarkUploadPacket> data)
{
	CGuildManager& rkGuildMgr = CGuildManager::instance();
	CGuild * pkGuild;

	if (!(pkGuild = rkGuildMgr.FindGuild(data->guild_id())))
	{
		sys_err("MARK_SERVER: GuildMarkUpload: no guild. gid %u", data->guild_id());
		return;
	}

	if (pkGuild->GetLevel() < guild_mark_min_level)
	{
		sys_log(0, "MARK_SERVER: GuildMarkUpload: level < %u (%u)", guild_mark_min_level, pkGuild->GetLevel());
		return;
	}

	CGuildMarkManager & rkMarkMgr = CGuildMarkManager::instance();

	sys_log(0, "MARK_SERVER: GuildMarkUpload: gid %u", data->guild_id());

	bool isEmpty = true;

	if (data->image().length() != 16 * 12 * 4)
		return;

	for (DWORD iPixel = 0; iPixel < SGuildMark::SIZE; ++iPixel)
		if (*((DWORD *) data->image().c_str() + iPixel) != 0x00000000)
			isEmpty = false;

	if (isEmpty)
		rkMarkMgr.DeleteMark(data->guild_id());
	else
		rkMarkMgr.SaveMark(data->guild_id(), (BYTE*) data->image().c_str());
}

void CInputLogin::GuildMarkIDXList(LPDESC d)
{
	CGuildMarkManager & rkMarkMgr = CGuildMarkManager::instance();

	GCOutputPacket<GCMarkIDXListPacket> p;
	for (auto& elem : rkMarkMgr.GetMarkMap())
	{
		auto cur = p->add_elems();
		cur->set_guild_id(elem.first);
		cur->set_mark_id(elem.second);
	}

	d->LargePacket(p);

	sys_log(0, "MARK_SERVER: GuildMarkIDXList %d bytes sent.", p->ByteSize());
}

#include "lzo_manager.h"
void CInputLogin::GuildMarkCRCList(LPDESC d, std::unique_ptr<CGMarkCRCListPacket> data)
{
	if (data->crclist_size() != 80)
		return;

	std::map<BYTE, const SGuildMarkBlock *> mapDiffBlocks;
	CGuildMarkManager::instance().GetDiffBlocks(data->image_index(), data->crclist(), mapDiffBlocks);

	DWORD blockCount = 0;
	TEMP_BUFFER buf(1024 * 1024); // 1M ¹öÆÛ

	for (auto it = mapDiffBlocks.begin(); it != mapDiffBlocks.end(); ++it)
	{
		BYTE posBlock = it->first;
		const SGuildMarkBlock & rkBlock = *it->second;

		buf.write(&posBlock, sizeof(BYTE));
		buf.write(&rkBlock.m_sizeCompBuf, sizeof(DWORD));
		buf.write(&rkBlock.m_abCompBuf[0], rkBlock.m_sizeCompBuf);


		Pixel apxBuf[SGuildMarkBlock::SIZE];
		lzo_uint sizeBuf = sizeof(apxBuf);
		bool ret = lzo1x_decompress_safe(&rkBlock.m_abCompBuf[0], rkBlock.m_sizeCompBuf, (BYTE*)apxBuf, &sizeBuf, LZOManager::instance().GetWorkMemory());
		sys_log(0, "BLOCK pos %d size_comp %u ret %u decompressed size %u max size %u", posBlock, rkBlock.m_sizeCompBuf, ret, sizeBuf, sizeof(apxBuf));

		++blockCount;
	}

	GCOutputPacket<GCMarkBlockPacket> pGC;

	pGC->set_image_index(data->image_index());
	pGC->set_block_count(blockCount);
	pGC->set_image(buf.read_peek(), buf.size());

	sys_log(0, "MARK_SERVER: Sending blocks. (imgIdx %u diff %u size %u)", data->image_index(), mapDiffBlocks.size(), pGC->ByteSize());

	d->LargePacket(pGC);
}

bool CInputLogin::Analyze(LPDESC d, const InputPacket& packet)
{
	if (test_server)
		sys_log(0, "Login[%p]: Analyze Packet %u", d, packet.get_header());

	switch (packet.get_header<TCGHeader>())
	{
		case TCGHeader::PONG:
			Pong(d);
			break;

		case TCGHeader::HANDSHAKE:
			Handshake(d, packet.get<CGHandshakePacket>());
			break;

		case TCGHeader::LOGIN_BY_KEY:
			LoginByKey(d, packet.get<CGLoginByKeyPacket>());
			break;

		case TCGHeader::PLAYER_SELECT:
			CharacterSelect(d, packet.get<CGPlayerSelectPacket>());
			break;

		case TCGHeader::PLAYER_CREATE:
			CharacterCreate(d, packet.get<CGPlayerCreatePacket>());
			break;

		case TCGHeader::PLAYER_DELETE:
			CharacterDelete(d, packet.get<CGPlayerDeletePacket>());
			break;

		case TCGHeader::ENTERGAME:
			Entergame(d);
			break;

		case TCGHeader::EMPIRE:
			Empire(d, packet.get<CGEmpirePacket>());
			break;

			///////////////////////////////////////
			// Guild Mark
			/////////////////////////////////////
		case TCGHeader::MARK_CRC_LIST:
			GuildMarkCRCList(d, packet.get<CGMarkCRCListPacket>());
			break;

		case TCGHeader::MARK_IDX_LIST:
			GuildMarkIDXList(d);
			break;

		case TCGHeader::MARK_UPLOAD:
			GuildMarkUpload(d, packet.get<CGMarkUploadPacket>());
			break;

			//////////////////////////////////////
			// Guild Symbol
			/////////////////////////////////////
		case TCGHeader::GUILD_SYMBOL_UPLOAD:
			GuildSymbolUpload(d, packet.get<CGGuildSymbolUploadPacket>());
			break;

		case TCGHeader::GUILD_SYMBOL_CRC:
			GuildSymbolCRC(d, packet.get<CGGuildSymbolCRCPacket>());
			break;
			/////////////////////////////////////

		case TCGHeader::HACK:
			break;

		case TCGHeader::PLAYER_CHANGE_NAME:
			ChangeName(d, packet.get<CGPlayerChangeNamePacket>());
			break;

		case TCGHeader::CLIENT_VERSION:
			Version(d->GetCharacter() , packet.get<CGClientVersionPacket>());
			break;

#ifdef __IPV6_FIX__
		case TCGHeader::IPV6_FIX_ENABLE:
			if (d->GetCharacter() )
				d->GetCharacter()->SetIPV6FixEnabled( );
			break;
#endif

#ifdef __HAIR_SELECTOR__
		case TCGHeader::PLAYER_SELECT_HAIR:
			PlayerHairSelect(d, packet.get<CGPlayerHairSelectPacket>());
			break;
#endif

		default:
			sys_err("login phase does not handle this packet! header %d" , packet.get_header());
			return false;
	}

	return true;
}

