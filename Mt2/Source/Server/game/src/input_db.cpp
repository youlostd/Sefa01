#include "stdafx.h" 
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "protocol.h"
#include "mob_manager.h"
#include "shop_manager.h"
#include "sectree_manager.h"
#include "skill.h"
#include "questmanager.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "start_position.h"
#include "party.h"
#include "refine.h"
#include "priv_manager.h"
#include "db.h"
#include "building.h"
#include "wedding.h"
#include "login_data.h"
#include "unique_item.h"
#include "SpamFilter.h"
#include "desc_client.h"
#include "affect.h"
#include "motion.h"

#include "dev_log.h"

#include "log.h"

#include "gm.h"
#include "map_location.h"
#include "general_manager.h"

#include "cmd.h"

#ifdef __GUILD_SAFEBOX__
#include "guild_safebox.h"
#endif
#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif
#ifdef __ATTRTREE__
#include "attrtree_manager.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#endif

using namespace network;

extern void PUBLIC_CreateLists();

extern BYTE		g_bAuthServer;

#ifdef __MAINTENANCE__
extern void __StartNewShutdown(int iStartSec, bool bIsMaintenance, int iMaintenanceDuration, bool bSendP2P);
extern void __StopCurrentShutdown(bool bSendP2P);
#endif

#ifdef ENABLE_XMAS_EVENT
extern std::vector<TXmasRewards> g_vec_xmasRewards;
#endif

#define MAPNAME_DEFAULT	"none"

bool GetServerLocation(TAccountTable & rTab, BYTE bEmpire)
{
	bool bFound = false;

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i)
	{
		if (0 == rTab.players(i).id())
			continue;

		bFound = true;
		long lIndex = 0;

		long addr;
		WORD port;
		if (!CMapLocation::instance().Get(rTab.players(i).x(),
					rTab.players(i).y(),
					lIndex,
					addr,
					port))
		{
			sys_err("location error name %s mapindex %d %d x %d empire %d",
					rTab.players(i).name().c_str(), lIndex, rTab.players(i).x(), rTab.players(i).y(), rTab.empire());

			rTab.mutable_players(i)->set_x(EMPIRE_START_X(rTab.empire()));
			rTab.mutable_players(i)->set_y(EMPIRE_START_Y(rTab.empire()));

			lIndex = 0;

			if (!CMapLocation::instance().Get(rTab.players(i).x(), rTab.players(i).y(), lIndex, addr, port))
			{
				sys_err("cannot find server for mapindex %d %d x %d (name %s)",
						lIndex,
						rTab.players(i).x(),
						rTab.players(i).y(),
						rTab.players(i).name().c_str());

				continue;
			}
		}

		rTab.mutable_players(i)->set_addr(addr);
		rTab.mutable_players(i)->set_port(port);

		struct in_addr in;
		in.s_addr = rTab.players(i).addr();
		sys_log(0, "success to %s:%d [pos %dx%d map %d]", inet_ntoa(in), rTab.players(i).port(), rTab.players(i).x(), rTab.players(i).y(), lIndex);
	}

	return bFound;
}

void CInputDB::LoginSuccess(DWORD dwHandle, std::unique_ptr<DGLoginSuccessPacket> data)
{
	sys_log(0, "LoginSuccess");

	LPDESC d = DESC_MANAGER::instance().FindByHandle(dwHandle);

	auto pTab = data->mutable_account_info();

	if (!d)
	{
		sys_log(0, "CInputDB::LoginSuccess - cannot find handle [%s]", pTab->login().c_str());

		network::GDOutputPacket<network::GDLogoutPacket> pack;
		pack->set_login(pTab->login());
		db_clientdesc->DBPacket(pack, dwHandle);
		return;
	}

	if (strcmp(pTab->status().c_str(), "OK")) // OK°¡ ¾Æ´Ï¸é
	{
		sys_log(0, "CInputDB::LoginSuccess - status[%s] is not OK [%s]", pTab->status().c_str(), pTab->login().c_str());

		GDOutputPacket<GDLogoutPacket> pack;
		pack->set_login(pTab->login());
		db_clientdesc->DBPacket(pack, dwHandle);

		LoginFailure(d, pTab->status().c_str());
		return;
	}

	for (int i = 0; i != PLAYER_PER_ACCOUNT; ++i)
	{
		const TSimplePlayer& player = pTab->players(i);
		sys_log(0, "LoginSuccess load player(%s).job(%d)", player.name().c_str(), player.job());
	}

	bool bFound = GetServerLocation(*pTab, pTab->empire());

	d->BindAccountTable(pTab);

	if (!bFound) // Ä³¸¯ÅÍ°¡ ¾øÀ¸¸é ·£´ýÇÑ Á¦±¹À¸·Î º¸³½´Ù.. -_-
	{
		network::GCOutputPacket<network::GCEmpirePacket> pe;
		pe->set_empire(random_number(1, 3));
		d->Packet(pe);
	}
	else
	{
		network::GCOutputPacket<network::GCEmpirePacket> pe;
		pe->set_empire(d->GetEmpire());
		d->Packet(pe);
	}

	d->SetPhase(PHASE_SELECT);
	d->SendLoginSuccessPacket();

	sys_log(0, "InputDB::login_success: %s", pTab->login().c_str());
}

void CInputDB::PlayerCreateFailure(LPDESC d, BYTE bType)
{
	if (!d)
		return;

	network::GCOutputPacket<network::GCCreateFailurePacket> pack;

	pack->set_type(bType);

	d->Packet(pack);
}

void CInputDB::PlayerCreateSuccess(LPDESC d, std::unique_ptr<DGPlayerCreateSuccessPacket> pPacketDB)
{
	if (!d)
		return;

	if (pPacketDB->account_index() > PLAYER_PER_ACCOUNT)
	{
		d->Packet(TGCHeader::CREATE_FAILURE);
		return;
	}

	long lIndex = 0;

	long addr;
	WORD port;
	if (!CMapLocation::instance().Get(pPacketDB->player().x(),
		pPacketDB->player().y(),
				lIndex,
				addr,
				port))
	{
		sys_err("InputDB::PlayerCreateSuccess: cannot find server for mapindex %d %d x %d (name %s)",
				lIndex,
				pPacketDB->player().x(),
				pPacketDB->player().y(),
				pPacketDB->player().name().c_str());
	}
	else
	{
		pPacketDB->mutable_player()->set_addr(addr);
		pPacketDB->mutable_player()->set_port(port);
	}

	TAccountTable& r_Tab = d->GetAccountTable();
	*r_Tab.mutable_players(pPacketDB->account_index()) = pPacketDB->player();

	GCOutputPacket<GCPlayerCreateSuccessPacket> pack;
	pack->set_account_index(pPacketDB->account_index());
	*pack->mutable_player() = pPacketDB->player();

	d->Packet(pack);

	LogManager::instance().CharLog(pack->player().id(), 0, 0, 0, "CREATE PLAYER", "", d->GetHostName());
}

void CInputDB::PlayerDeleteSuccess(LPDESC d, std::unique_ptr<DGPlayerDeleteSuccessPacket> pack)
{
	if (!d)
		return;

	network::GCOutputPacket<network::GCDeleteSuccessPacket> pack_send;
	pack_send->set_account_index(pack->account_index());
	d->Packet(pack_send);

	d->GetAccountTable().mutable_players(pack->account_index())->set_id(0);
}

void CInputDB::PlayerDeleteFail(LPDESC d)
{
	if (!d)
		return;

	d->Packet(TGCHeader::DELETE_FAILURE);
	//d->Packet(encode_byte(account_index),			1);

	//d->GetAccountTable().players[account_index].dwID = 0;
}

void CInputDB::ChangeName(LPDESC d, std::unique_ptr<DGChangeNamePacket> p)
{
	if (!d)
		return;

	TAccountTable & r = d->GetAccountTable();

	if (!r.id())
		return;

	for (size_t i = 0; i < PLAYER_PER_ACCOUNT; ++i)
		if (r.players(i).id() == p->pid())
		{
			LogManager::instance().ChangeNameLog(d, p->pid(), r.players(i).name().c_str(), p->name().c_str());
			//FIXCHANGENAME
			char szHint[80];
			sprintf(szHint, "%s => %s", r.players(i).name().c_str(), p->name().c_str());
			LogManager::instance().CharLog(p->pid(), 0, 0, 0, "CHANGE_NAME", szHint, d->GetHostName());
			char szBuf[256];
			sprintf(szBuf, "UPDATE player.zodiac_temple_table SET player = '%s' WHERE player = '%s'",p->name().c_str(), r.players(i).name().c_str());
			DBManager::Instance().Query(szBuf);
			
			sprintf(szBuf, "UPDATE messenger_block_list SET account='%s' WHERE account='%s'",p->name().c_str(), r.players(i).name().c_str());
			DBManager::Instance().Query(szBuf);
			sprintf(szBuf, "UPDATE messenger_block_list SET companion='%s' WHERE companion='%s'",p->name().c_str(), r.players(i).name().c_str());
			DBManager::Instance().Query(szBuf);

			sprintf(szBuf, "UPDATE messenger_list SET account='%s' WHERE account='%s'",p->name().c_str(), r.players(i).name().c_str());
			DBManager::Instance().Query(szBuf);
			sprintf(szBuf, "UPDATE messenger_list SET companion='%s' WHERE companion='%s'",p->name().c_str(), r.players(i).name().c_str());
			DBManager::Instance().Query(szBuf);

			r.mutable_players(i)->set_name(p->name());
			r.mutable_players(i)->set_change_name(false);

			network::GCOutputPacket<network::GCChangeNamePacket> pgc;

			pgc->set_pid(p->pid());
			pgc->set_name(p->name());

			d->Packet(pgc);
			break;
		}
}

void CInputDB::PlayerLoad(LPDESC d, std::unique_ptr<DGPlayerLoadPacket> pack)
{
	if (!d)
		return;

	TPlayerTable* pTab = pack->mutable_player();

	long lMapIndex = pTab->map_index();
	PIXEL_POSITION pos;

	long lRealMapIndex = lMapIndex;
	if (lRealMapIndex >= 10000)
		lRealMapIndex /= 10000;

	long lMapIndexByCoord = SECTREE_MANAGER::instance().GetMapIndex(pTab->x(), pTab->y());
	if (lMapIndex == 0 || lMapIndexByCoord != lRealMapIndex)
	{
		lMapIndex = EMPIRE_START_MAP(d->GetAccountTable().empire());
		pTab->set_x(EMPIRE_START_X(d->GetAccountTable().empire()));
		pTab->set_y(EMPIRE_START_Y(d->GetAccountTable().empire()));
	}
	pTab->set_map_index(lMapIndex);

	if (!SECTREE_MANAGER::instance().GetValidLocation(pTab->map_index(), pTab->x(), pTab->y(), lMapIndex, pos, d->GetEmpire()))
	{
		sys_err("InputDB::PlayerLoad : cannot find valid location %d x %d (name: %s)", pTab->x(), pTab->y(), pTab->name().c_str());
#ifdef GOHOME_FOR_INVALID_LOCATION
		if (quest::CQuestManager::instance().GetEventFlag("disable_gohome") == 0)
		{
			lMapIndex = EMPIRE_START_MAP(d->GetAccountTable().empire());
			pos.x = EMPIRE_START_X(d->GetAccountTable().empire());
			pos.y = EMPIRE_START_Y(d->GetAccountTable().empire());
			LogManager::instance().CharLog(pTab->id(), 0, 0, 0, "GO_HOME_FIX", "", "");
		}
		else
		{
#endif
			network::GDOutputPacket<network::GDFlushCachePacket> pdb;
			pdb->set_pid(pTab->id());
			db_clientdesc->DBPacket(pdb);
			
			DBManager::instance().Query("UPDATE player SET x=%d, y=%d, map_index=%d, exit_x=%d, exit_y=%d, exit_map_index=%d WHERE id=%u", 
					EMPIRE_START_MAP(d->GetAccountTable().empire()), EMPIRE_START_X(d->GetAccountTable().empire()), EMPIRE_START_Y(d->GetAccountTable().empire()),
					EMPIRE_START_MAP(d->GetAccountTable().empire()), EMPIRE_START_X(d->GetAccountTable().empire()), EMPIRE_START_Y(d->GetAccountTable().empire()),
					pTab->id());

			d->SetPhase(PHASE_CLOSE);
			return;
#ifdef GOHOME_FOR_INVALID_LOCATION
		}
#endif
	}

	pTab->set_x(pos.x);
	pTab->set_y(pos.y);
	pTab->set_map_index(lMapIndex);

	if (d->GetCharacter() || d->IsPhase(PHASE_GAME))
	{
		LPCHARACTER p = d->GetCharacter();
		sys_err("login state already has main state (character %s %p)", p->GetName(), get_pointer(p));
		return;
	}

	if (NULL != CHARACTER_MANAGER::Instance().FindPC(pTab->name().c_str()))
	{
		sys_err("InputDB: PlayerLoad : %s already exist in game", pTab->name().c_str());
		return;
	}

	long lPublicMapIndex = lMapIndex >= 10000 ? lMapIndex / 10000 : lMapIndex;

	//if (!map_allow_find(lMapIndex >= 10000 ? lMapIndex / 10000 : lMapIndex) || !CheckEmpire(ch, lMapIndex))
	if (!map_allow_find(lPublicMapIndex))
	{
		sys_err("InputDB::PlayerLoad1 : entering %d map is not allowed here (name: %s, empire %u)", 
				lMapIndex, pTab->name().c_str(), d->GetEmpire());
		if (map_allow_find(EMPIRE_START_MAP(d->GetEmpire())))
		{
			pTab->set_x(EMPIRE_START_X(d->GetEmpire()));
			pTab->set_y(EMPIRE_START_Y(d->GetEmpire()));
			pTab->set_map_index(EMPIRE_START_MAP(d->GetEmpire()));
		}
		else
		{
			PIXEL_POSITION kSpawnPos;
			for (int i=0; i<255; i++)
			{
				if (map_allow_find(i))
				{
					if (SECTREE_MANAGER::instance().GetSpawnPositionByMapIndex(i, kSpawnPos))
					{
						pTab->set_x(kSpawnPos.x);
						pTab->set_y(kSpawnPos.y);
						pTab->set_map_index(i);
						break;
					}
				}
			}
		}
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().CreateCharacter(pTab->name().c_str(), pTab->id());

	ch->BindDesc(d);
	ch->SetPlayerProto(pTab);
	ch->SetEmpire(d->GetEmpire());

	d->BindCharacter(ch);

	if (ch->IsGM())
		ch->SetGMInvisible(true, true); // set invisible until quest flags loaded

#ifdef __VOTE4BUFF__
	ch->V4B_Initialize();
#endif
	
	{
		// P2P Login
		network::GGOutputPacket<network::GGLoginPacket> p;

		p->set_name(ch->GetName());
		p->set_pid(ch->GetPlayerID());
		p->set_empire(ch->GetEmpire());
		p->set_map_index(SECTREE_MANAGER::instance().GetMapIndex(ch->GetX(), ch->GetY()));
		p->set_is_in_dungeon(ch->GetMapIndex() >= 10000);
		p->set_channel(g_bChannel);
		p->set_race(ch->GetRaceNum());
		p->set_language(ch->GetLanguageID());
		p->set_temp_login(ch->is_temp_login());

		P2P_MANAGER::instance().Send(p);
#ifdef ENABLE_ZODIAC_TEMPLE
		char buf[51];
		snprintf(buf, sizeof(buf), "%s [%lld yang] %d %s %ld %d", 
				inet_ntoa(ch->GetDesc()->GetAddr().sin_addr), ch->GetGold(), ch->GetAnimasphere(), g_stHostname.c_str(), ch->GetMapIndex(), ch->GetAlignment());
#else
		char buf[51];
		snprintf(buf, sizeof(buf), "%s [%lld yang] %s %ld %d", 
				inet_ntoa(ch->GetDesc()->GetAddr().sin_addr), ch->GetGold(), g_stHostname.c_str(), ch->GetMapIndex(), ch->GetAlignment());
#endif
		LogManager::instance().CharLog(ch, 0, "LOGIN", buf);
	}

	d->SetPhase(PHASE_LOADING);
	ch->MainCharacterPacket();

/*	if (!map_allow_find(lPublicMapIndex) && !map_allow_find(EMPIRE_START_MAP(d->GetEmpire())))
	{
		sys_err("InputDB::PlayerLoad2 : entering %d map is not allowed here (name: %s, empire %u)", 
				lMapIndex, pTab->name, d->GetEmpire());
		ch->SetWarpLocation(EMPIRE_START_MAP(d->GetEmpire()),
				EMPIRE_START_X(d->GetEmpire()) / 100,
				EMPIRE_START_Y(d->GetEmpire()) / 100);
		d->SetPhase(PHASE_CLOSE);
		return;
	}*/

	quest::CQuestManager::instance().BroadcastEventFlagOnLogin(ch);

	for (int i = 0; i < QUICKSLOT_MAX_NUM && i < pTab->quickslots_size(); ++i)
		ch->SetQuickslot(i, pTab->quickslots(i));

	ch->PointsPacket();
	ch->SkillLevelPacket();

#ifdef ENABLE_RUNE_SYSTEM
	for (DWORD rune : pTab->runes())
	{
		if (test_server)
			sys_log(0, "  LoadRune[%u]", rune);

		ch->SetRuneOwned(rune);
	}

	ch->SetRuneLoaded();
#endif

	sys_log(0, "InputDB: player_load %s %dx%dx%d LEVEL %d MOV_SPEED %d JOB %d ATG %d DFG %d GMLv %d",
			pTab->name().c_str(), 
			ch->GetX(), ch->GetY(), ch->GetZ(),
			ch->GetLevel(),
			ch->GetPoint(POINT_MOV_SPEED),
			ch->GetJob(),
			ch->GetPoint(POINT_ATT_GRADE),
			ch->GetPoint(POINT_DEF_GRADE),
			ch->GetGMLevel());

	ch->QuerySafeboxSize();
}

void CInputDB::Boot(std::unique_ptr<DGBootPacket> data)
{
	signal_timer_disable();

	sys_log(0, "RECV_BOOT_PACKET[size %u]", data->ByteSize());

	WORD size;
	sys_log(0, "BOOT: MOB: %d", data->mobs_size());
	if (data->mobs_size() > 0)
		CMobManager::instance().Initialize(data->mobs());

	// item proto
	sys_log(0, "BOOT: ITEM: %d", data->items_size());
	if (data->items_size() > 0)
		ITEM_MANAGER::instance().Initialize(data->items());

	// shop proto
	sys_log(0, "BOOT: SHOP: %d", data->shops_size());
	if (data->shops_size() > 0)
	{
		if (!CShopManager::instance().Initialize(data->shops()))
		{
			sys_err("shop table Initialize error");
			thecore_shutdown();
			return;
		}
	}

	// skill proto
	sys_log(0, "BOOT: SKILL: %d", data->skills_size());
	if (data->skills_size() > 0)
	{
		if (!CSkillManager::instance().Initialize(data->skills()))
		{
			sys_err("cannot initialize skill table");
			thecore_shutdown();
			return;
		}
	}

	for (BYTE i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		char szSkillDescFileName[FILE_MAX_LEN];
		snprintf(szSkillDescFileName, sizeof(szSkillDescFileName), "%s/%s/skilldesc.txt", CLocaleManager::instance().GetLocalePath().c_str(), CLocaleManager::instance().GetLanguageName(i));
		CSkillManager::instance().InitializeNames(i, szSkillDescFileName);
	}

	// refine proto
	sys_log(0, "BOOT: REFINE: %d", data->refines_size());
	if (data->refines_size() > 0)
		CRefineManager::instance().Initialize(data->refines());

	// item attr table
	sys_log(0, "BOOT: ITEM_ATTR: %d", data->attrs_size());
	if (data->attrs_size() > 0)
	{
		for (int i = 0; i < data->attrs_size(); ++i)
		{
			auto& attr = data->attrs(i);
			if (attr.apply_index() >= MAX_APPLY_NUM)
				continue;

			g_map_itemAttr[attr.apply_index()] = attr;
			sys_log(0, "ITEM_ATTR[%d]: %s %u", attr.apply_index(), attr.apply().c_str(), attr.prob());
		}
	}

#ifdef EL_COSTUME_ATTR
	// item attr table
	sys_log(0, "BOOT: ITEM_ATTR_COSTUME: %d", data->costume_attrs_size());
	if (data->costume_attrs_size() > 0)
	{
		for (int i = 0; i < data->costume_attrs_size(); ++i)
		{
			auto& attr = data->costume_attrs(i);
			if (attr.apply_index() >= MAX_APPLY_NUM)
				continue;

			g_map_itemCostumeAttr[attr.apply_index()] = attr;
			sys_log(0, "ITEM_ATTR_COSTUME[%d]: %s %u", attr.apply_index(), attr.apply().c_str(), attr.prob());
		}
	}
#endif

#ifdef ITEM_ATTR_RARE
	// item attr rare table
	sys_log(0, "BOOT: ITEM_ATTR_RARE: %d", data->rare_attrs_size());
	if (data->rare_attrs_size() > 0)
	{
		for (int i = 0; i < data->rare_attrs_size(); ++i)
		{
			auto& attr = data->rare_attrs(i);
			if (attr.apply_index() >= MAX_APPLY_NUM)
				continue;

			g_map_itemRare[attr.apply_index()] = attr;
			sys_log(0, "ITEM_ATTR_RARE[%d]: %s %u", attr.apply_index(), attr.apply().c_str(), attr.prob());
		}
	}
#endif

	{
		using namespace building;

		// lands
		for (auto i = 0; i < data->lands_size(); ++i)
			CManager::instance().LoadLand(&data->lands(i));

		// object protos
		CManager::instance().LoadObjectProto(data->object_protos());

		// objects
		for (WORD i = 0; i < data->objects_size(); ++i)
			CManager::instance().LoadObject(&data->objects(i), true);
	}

	/*
	* GUILD_SAFEBOX
	*/
#ifdef __GUILD_SAFEBOX__
	for (auto& p : data->guild_safeboxes())
	{
		CGuild* pGuild = CGuildManager::Instance().FindGuild(p.guild_id());
		if (!pGuild)
		{
			// sys_err("cannot load guild safebox for guild %u (no guild)", p->dwGuildID);
			continue;
		}

		pGuild->GetSafeBox().Load(p.size(), p.password().c_str(), p.gold());
	}
#endif

	CHARACTER_MANAGER::instance().InitializeHorseUpgrade(data->horse_upgrades());
	CHARACTER_MANAGER::instance().InitializeHorseBonus(data->horse_boni());

	/*
	* Gaya Shop
	*/
#ifdef __GAYA_SYSTEM__
	CGeneralManager::instance().InitializeGayaShop(data->gaya_shops());
#endif

	/*
	* Attrtree
	*/
#ifdef __ATTRTREE__
	CAttrtreeManager::Instance().Initialize(data->attrtrees());
#endif

#ifdef ENABLE_RUNE_SYSTEM
	CRuneManager::instance().Initialize(data->runes());
	CRuneManager::instance().Initialize(data->rune_points());
#endif

#ifdef ENABLE_XMAS_EVENT
	for (auto& t : data->xmas_rewards())
		g_vec_xmasRewards.push_back(t);
#endif

#ifdef __PET_ADVANCED__
	// pet skills
	for (auto i = 0; i < data->pet_skills_size(); ++i)
		CPetSkillProto::Create(data->pet_skills(i));

	// pet evolve data
	for (auto i = 0; i < data->pet_evolves_size(); ++i)
		CPetEvolveProto::Create(data->pet_evolves(i));

	// pet attr data
	for (auto i = 0; i < data->pet_attrs_size(); ++i)
		CPetAttrProto::Create(data->pet_attrs(i));
#endif

	ITEM_MANAGER::Instance().InitializeSoulProto(data->soul_protos());

#ifdef CRYSTAL_SYSTEM
	CGeneralManager::instance().initialize_crystal_proto(data->crystal_protos());
#endif
	
	set_global_time(data->current_time());

	for (auto& t : data->admins())
		GM::insert(t);
	GM::init(data->admin_configs());

	if (!ITEM_MANAGER::instance().SetMaxItemID(data->item_id_range()))
	{
		sys_err("not enough item id contact your administrator!");
		thecore_shutdown();
		return;
	}

	if (!ITEM_MANAGER::instance().SetMaxSpareItemID(data->item_id_range_spare()))
	{
		sys_err("not enough item id for spare contact your administrator!");
		thecore_shutdown();
		return;
	}

	// LOCALE_SERVICE
	const int FILE_NAME_LEN = 256;
	char szCommonDropItemFileName[FILE_NAME_LEN];
	char szETCDropItemFileName[FILE_NAME_LEN];
	char szMOBDropItemFileName[FILE_NAME_LEN];
	char szDropItemGroupFileName[FILE_NAME_LEN];
	char szSpecialItemGroupFileName[FILE_NAME_LEN];
	char szMapIndexFileName[FILE_NAME_LEN];
#ifdef __DRAGONSOUL__
	char szDragonSoulTableFileName[FILE_NAME_LEN];
#endif
#ifdef __EVENT_MANAGER__
	char szEventManagerFileName[FILE_NAME_LEN];
#endif

	snprintf(szCommonDropItemFileName, sizeof(szCommonDropItemFileName),
			"%s/common_drop_item.txt", Locale_GetBasePath().c_str());
	snprintf(szETCDropItemFileName, sizeof(szETCDropItemFileName),
			"%s/etc_drop_item.txt", Locale_GetBasePath().c_str());
	snprintf(szMOBDropItemFileName, sizeof(szMOBDropItemFileName),
			"%s/mob_drop_item.txt", Locale_GetBasePath().c_str());
	snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
			"%s/special_item_group.txt", Locale_GetBasePath().c_str());
	snprintf(szDropItemGroupFileName, sizeof(szDropItemGroupFileName),
			"%s/drop_item_group.txt", Locale_GetBasePath().c_str());
	snprintf(szMapIndexFileName, sizeof(szMapIndexFileName),
			"%s/index", Locale_GetMapPath().c_str());
#ifdef __EVENT_MANAGER__
	snprintf(szEventManagerFileName, sizeof(szEventManagerFileName),
			"%s/event_data.txt", Locale_GetBasePath().c_str());
#endif
#ifdef __DRAGONSOUL__
	snprintf(szDragonSoulTableFileName, sizeof(szDragonSoulTableFileName),
			"%s/dragon_soul_table.txt", Locale_GetBasePath().c_str());
#endif

	sys_log(0, "Initializing Informations of Cube System");
	if (!Cube_InformationInitialize())
	{
		sys_err("cannot init cube infomation.");
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: CommonDropItem: %s", szCommonDropItemFileName);
	if (!ITEM_MANAGER::instance().ReadCommonDropItemFile(szCommonDropItemFileName))
	{
		sys_err("cannot load CommonDropItem: %s", szCommonDropItemFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: ETCDropItem: %s", szETCDropItemFileName);
	if (!ITEM_MANAGER::instance().ReadEtcDropItemFile(szETCDropItemFileName))
	{
		sys_err("cannot load ETCDropItem: %s", szETCDropItemFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: DropItemGroup: %s", szDropItemGroupFileName);
	if (!ITEM_MANAGER::instance().ReadDropItemGroup(szDropItemGroupFileName))
	{
		sys_err("cannot load DropItemGroup: %s", szDropItemGroupFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: SpecialItemGroup: %s", szSpecialItemGroupFileName);
	if (!ITEM_MANAGER::instance().ReadSpecialDropItemFile(szSpecialItemGroupFileName))
	{
		sys_err("cannot load SpecialItemGroup: %s", szSpecialItemGroupFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: MOBDropItemFile: %s", szMOBDropItemFileName);
	if (!ITEM_MANAGER::instance().ReadMonsterDropItemGroup(szMOBDropItemFileName))
	{
		sys_err("cannot load MOBDropItemFile: %s", szMOBDropItemFileName);
		thecore_shutdown();
		return;
	}

	sys_log(0, "LoadLocaleFile: MapIndex: %s", szMapIndexFileName);
	if (!SECTREE_MANAGER::instance().Build(szMapIndexFileName, Locale_GetMapPath().c_str()))
	{
		sys_err("cannot load MapIndex: %s", szMapIndexFileName);
		thecore_shutdown();
		return;
	}

#ifdef __DRAGONSOUL__
	sys_log(0, "LoadLocaleFile: DragonSoulTable: %s", szDragonSoulTableFileName);
	if (!DSManager::instance().ReadDragonSoulTableFile(szDragonSoulTableFileName))
	{
		sys_err("cannot load DragonSoulTable: %s", szDragonSoulTableFileName);
		thecore_shutdown();
		return;
	}
#endif

#ifdef __EVENT_MANAGER__
	sys_log(0, "LoadLocaleFile: EventData: %s", szEventManagerFileName);
	if (!CEventManager::instance().Initialize(szEventManagerFileName))
	{
		sys_err("cannot load EventData: %s", szEventManagerFileName);
		thecore_shutdown();
		return;
	}
#endif

	// END_OF_LOCALE_SERVICE

#ifdef ENABLE_SPAM_FILTER
	CSpamFilter::instance().InitializeSpamFilterTable();
#endif
	building::CManager::instance().FinalizeBoot();
	CMotionManager::instance().Build();
	signal_timer_enable(30);

	// auction
#ifdef AUCTION_SYSTEM
	if (g_isProcessorCore)
		AuctionManager::instance().initialize();
#endif

	// public_table
	if (g_bCreatePublicFiles)
		PUBLIC_CreateLists();
}

EVENTINFO(quest_login_event_info)
{
	DWORD dwPID;

	quest_login_event_info() 
	: dwPID( 0 )
	{
	}
};

EVENTFUNC(quest_login_event)
{
	quest_login_event_info* info = dynamic_cast<quest_login_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "quest_login_event> <Factor> Null pointer" );
		return 0;
	}

	DWORD dwPID = info->dwPID;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

	if (!ch)
		return 0;

	LPDESC d = ch->GetDesc();

	if (!d)
		return 0;

	if (d->IsPhase(PHASE_HANDSHAKE) ||
		d->IsPhase(PHASE_LOGIN) ||
		d->IsPhase(PHASE_SELECT) ||
		d->IsPhase(PHASE_DEAD) ||
		d->IsPhase(PHASE_LOADING))
	{
		return PASSES_PER_SEC(1);
	}
	else if (d->IsPhase(PHASE_CLOSE))
	{
		return 0;
	}
	else if (d->IsPhase(PHASE_GAME))
	{
		sys_log(0, "QUEST_LOAD: Login pc %d by event", ch->GetPlayerID());
		quest::CQuestManager::instance().Login(ch->GetPlayerID());
		return 0;
	}
	else
	{
		sys_err(0, "input_db.cpp:quest_login_event INVALID PHASE pid %d", ch->GetPlayerID());
		return 0;
	}
}

void CInputDB::QuestLoad(LPDESC d, std::unique_ptr<DGQuestLoadPacket> data)
{
	if (NULL == d)
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (NULL == ch)
		return;

	if (ch->GetPlayerID() != data->pid())
	{
		sys_err("PID differs %u %u", ch->GetPlayerID(), data->pid());
		return;
	}

	sys_log(0, "QUEST_LOAD: count %d", data->quests_size());

	quest::PC * pkPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());

	if (!pkPC)
	{
		sys_err("null quest::PC with id %u", data->pid());
		return;
	}

	if (pkPC->IsLoaded())
		return;

	for (auto& quest : data->quests())
	{
		std::string st(quest.name());
		if (st == "killcounter")
		{
			if (!strcmp(quest.state().c_str(), "empire_0_kill"))
				ch->SetRealPoint(POINT_EMPIRE_A_KILLED, quest.value());
			else if (!strcmp(quest.state().c_str(), "empire_1_kill"))
				ch->SetRealPoint(POINT_EMPIRE_B_KILLED, quest.value());
			else if (!strcmp(quest.state().c_str(), "empire_2_kill"))
				ch->SetRealPoint(POINT_EMPIRE_C_KILLED, quest.value());
			else if (!strcmp(quest.state().c_str(), "duel_won"))
				ch->SetRealPoint(POINT_DUELS_WON, quest.value());
			else if (!strcmp(quest.state().c_str(), "duel_lost"))
				ch->SetRealPoint(POINT_DUELS_LOST, quest.value());
			else if (!strcmp(quest.state().c_str(), "monster_kill"))
				ch->SetRealPoint(POINT_MONSTERS_KILLED, quest.value());
			else if (!strcmp(quest.state().c_str(), "boss_kill"))
				ch->SetRealPoint(POINT_BOSSES_KILLED, quest.value());
			else if (!strcmp(quest.state().c_str(), "stone_kill"))
				ch->SetRealPoint(POINT_STONES_DESTROYED, quest.value());
		}

		st += ".";
		st += quest.state();

		sys_log(!test_server, "            %s %d", st.c_str(), quest.value());
		pkPC->SetFlag(st.c_str(), quest.value(), false);

		if (st == "game_flag.anti_exp" && quest.value())
			ch->SetEXPDisabled();

#ifdef ENABLE_RUNE_SYSTEM
		if (st == "rune_manager.harvested_souls" && ch->GetPoint(POINT_RUNE_HARVEST))
			ch->GetRuneData().soulsHarvested = MIN(ch->GetPoint(POINT_RUNE_HARVEST), int(quest.value()));
#endif
	}

	pkPC->SetLoaded();
	pkPC->Build();

	if (ch->IsGM())
		ch->SetGMInvisible(ch->GetQuestFlag("gm_save.is_invisible"), false);

	if (ch->GetDesc()->IsPhase(PHASE_GAME))
	{
		sys_log(0, "QUEST_LOAD: Login pc %d", data->pid());
		quest::CQuestManager::instance().Login(data->pid());
	}
	else
	{
		quest_login_event_info* info = AllocEventInfo<quest_login_event_info>();
		info->dwPID = ch->GetPlayerID();

		event_create(quest_login_event, info, PASSES_PER_SEC(1));
	}
}

void CInputDB::SafeboxLoad(LPDESC d, std::unique_ptr<DGSafeboxLoadPacket> data)
{
	if (!d)
		return;

	if (d->GetAccountTable().id() != data->account_id())
	{
		sys_err("SafeboxLoad: safebox has different id %u != %u", d->GetAccountTable().id(), data->account_id());
		return;
	}

	if (!d->GetCharacter())
		return;

	//BYTE bSize = 1;

	LPCHARACTER ch = d->GetCharacter();

	//PREVENT_TRADE_WINDOW
	if (!ch->CanShopNow() || ch->GetMyShop())
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(d->GetCharacter(), "´Ù¸¥°Å·¡Ã¢ÀÌ ¿­¸°»óÅÂ¿¡¼­´Â Ã¢°í¸¦ ¿­¼ö°¡ ¾ø½À´Ï´Ù." ) );
		d->GetCharacter()->CancelSafeboxLoad();
		return;
	}
	//END_PREVENT_TRADE_WINDOW

	// ADD_PREMIUM
	/*if (d->GetCharacter()->GetPremiumRemainSeconds(PREMIUM_SAFEBOX) > 0 ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_LARGE_SAFEBOX))
		bSize = 3;*/
	// END_OF_ADD_PREMIUM

	//if (d->GetCharacter()->IsEquipUniqueItem(UNIQUE_ITEM_SAFEBOX_EXPAND))
	//bSize = 3; // Ã¢°íÈ®Àå±Ç

	//d->GetCharacter()->LoadSafebox(p->bSize * SAFEBOX_PAGE_SIZE, p->dwGold, p->wItemCount, (TPlayerItem *) (c_pData + sizeof(TSafeboxTable)));
	if (data->is_mall())
		d->GetCharacter()->LoadMall(data->items());
	else
		d->GetCharacter()->LoadSafebox(data->size() * SAFEBOX_PAGE_SIZE, data->gold(), data->items());
}

void CInputDB::SafeboxChangeSize(LPDESC d, std::unique_ptr<DGSafeboxChangeSizePacket> data)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	d->GetCharacter()->ChangeSafeboxSize(data->size());
}

//
// @version	05/06/20 Bang2ni - ReqSafeboxLoad ÀÇ Ãë¼Ò
//
void CInputDB::SafeboxWrongPassword(LPDESC d)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	d->Packet(TGCHeader::SAFEBOX_WRONG_PASSWORD);

	d->GetCharacter()->CancelSafeboxLoad();
}

void CInputDB::SafeboxChangePasswordAnswer(LPDESC d, std::unique_ptr<DGSafeboxChangePasswordAnswerPacket> data)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	if (data->flag())
	{
		d->GetCharacter()->SetSafeboxNeedPassword(true);
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(d->GetCharacter(), "<Ã¢°í> Ã¢°í ºñ¹Ð¹øÈ£°¡ º¯°æµÇ¾ú½À´Ï´Ù."));
	}
	else
	{
		d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(d->GetCharacter(), "<Ã¢°í> ±âÁ¸ ºñ¹Ð¹øÈ£°¡ Æ²·È½À´Ï´Ù."));
	}
}

void CInputDB::LoginAlready(LPDESC d, std::unique_ptr<DGLoginAlreadyPacket> p)
{
	if (!d)
		return;

	// INTERNATIONAL_VERSION ÀÌ¹Ì Á¢¼ÓÁßÀÌ¸é Á¢¼Ó ²÷À½
	{ 
		LPDESC d2 = DESC_MANAGER::instance().FindByLoginName(p->login().c_str());

		if (d2)
			d2->DisconnectOfSameLogin();
		else
		{
			network::GGOutputPacket<network::GGDisconnectPacket> pgg;

			pgg->set_login(p->login().c_str());

			P2P_MANAGER::instance().Send(pgg);
		}
	}
	// END_OF_INTERNATIONAL_VERSION

	LoginFailure(d, "ALREADY");
}

void CInputDB::EmpireSelect(LPDESC d, std::unique_ptr<DGEmpireSelectPacket> data)
{
	sys_log(0, "EmpireSelect %p", get_pointer(d));

	if (!d)
		return;

	TAccountTable & rTable = d->GetAccountTable();
	rTable.set_empire(data->empire());

	network::GCOutputPacket<network::GCEmpirePacket> pe;
	pe->set_empire(rTable.empire());
	d->Packet(pe);

	for (int i = 0; i < PLAYER_PER_ACCOUNT; ++i) 
		if (rTable.players(i).id())
		{
			rTable.mutable_players(i)->set_x(EMPIRE_START_X(rTable.empire()));
			rTable.mutable_players(i)->set_y(EMPIRE_START_Y(rTable.empire()));
		}

	GetServerLocation(d->GetAccountTable(), rTable.empire());

	d->SendLoginSuccessPacket();
}

void CInputDB::MapLocations(std::unique_ptr<DGMapLocationsPacket> data)
{
	sys_log(0, "InputDB::MapLocations %d", data->maps_size());

	for (int j = 0; j < data->maps_size(); ++j)
	{
		auto& loc = data->maps(j);

		for (int i = 0; i < 32; ++i)
		{
			if (0 == loc.maps(i))
				break;

			CMapLocation::instance().Insert(loc.maps(i), loc.host_name().c_str(), loc.port(), loc.channel());
		}
	}

#ifdef AUCTION_SYSTEM
	AuctionManager::instance().on_receive_map_location();
#endif
}

void CInputDB::P2P(std::unique_ptr<DGP2PInfoPacket> p)
{
	extern LPFDWATCH main_fdw;

	P2P_MANAGER& mgr = P2P_MANAGER::instance();

	if (false == DESC_MANAGER::instance().IsP2PDescExist(p->host().c_str(), p->port()))
	{
		LPCLIENT_DESC pkDesc = NULL;
		sys_log(0, "InputDB:P2P %s:%u", p->host().c_str(), p->port());
		pkDesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, p->host().c_str(), p->port(), PHASE_P2P, false);
		mgr.RegisterConnector(pkDesc);
#ifdef PROCESSOR_CORE
		if (p->processor_core())
			mgr.SetProcessorCore(pkDesc);
#endif
		pkDesc->SetP2P(p->host().c_str(), p->port(), p->channel());
		pkDesc->SetListenPort(p->listen_port());

#ifdef AUCTION_SYSTEM
		AuctionManager::instance().on_connect_peer(pkDesc);
#endif
	}
}

void CInputDB::GuildLoad(std::unique_ptr<DGGuildLoadPacket> data)
{
	CGuildManager::instance().LoadGuild(data->guild_id());
}

void CInputDB::GuildSkillUpdate(std::unique_ptr<network::DGGuildSkillUpdatePacket> data)
{
	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_id());

	if (g)
	{
		for (int i = 0; i < data->skill_levels_size(); ++i)
			g->UpdateSkillLevel(data->skill_point(), i, data->skill_levels(i));
		g->GuildPointChange(POINT_SP, data->amount(), data->save() ? true : false);
	}
}

void CInputDB::GuildWar(std::unique_ptr<network::DGGuildWarPacket> data)
{
	sys_log(0, "InputDB::GuildWar %u %u state %d", data->guild_from(), data->guild_to(), data->war());

	switch (data->war())
	{
		case GUILD_WAR_SEND_DECLARE:
		case GUILD_WAR_RECV_DECLARE:
			CGuildManager::instance().DeclareWar(data->guild_from(), data->guild_to(), data->type());
			break;

		case GUILD_WAR_REFUSE:
			CGuildManager::instance().RefuseWar(data->guild_from(), data->guild_to());
			break;

		case GUILD_WAR_WAIT_START:
			CGuildManager::instance().WaitStartWar(data->guild_from(), data->guild_to());
			break;

		case GUILD_WAR_CANCEL:
			CGuildManager::instance().CancelWar(data->guild_from(), data->guild_to());
			break;

		case GUILD_WAR_ON_WAR:
			CGuildManager::instance().StartWar(data->guild_from(), data->guild_to());
			break;

		case GUILD_WAR_END:
			CGuildManager::instance().EndWar(data->guild_from(), data->guild_to());
			break;

		case GUILD_WAR_OVER:
			CGuildManager::instance().WarOver(data->guild_from(), data->guild_to(), data->type());
			break;

		case GUILD_WAR_RESERVE:
			CGuildManager::instance().ReserveWar(data->guild_from(), data->guild_to(), data->type());
			break;

		default:
			sys_err("Unknown guild war state");
			break;
	}
}

void CInputDB::GuildWarScore(std::unique_ptr<network::DGGuildWarScorePacket> data)
{
	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_gain_point());
	g->SetWarScoreAgainstTo(data->guild_opponent(), data->score());
}

void CInputDB::GuildSkillRecharge()
{
	CGuildManager::instance().SkillRecharge();
}

void CInputDB::GuildExpUpdate(std::unique_ptr<network::DGGuildExpUpdatePacket> data)
{
	sys_log(1, "GuildExpUpdate %d", data->amount());

	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_id());

	if (g)
		g->GuildPointChange(POINT_EXP, data->amount());
}

void CInputDB::GuildAddMember(std::unique_ptr<network::DGGuildAddMemberPacket> data)
{
	CGuild * g = CGuildManager::instance().TouchGuild(data->guild_id());

	if (g)
		g->AddMember(data->pid(), data->grade(), data->is_general(), data->job(), data->level(), data->offer(), data->name().c_str());
}

void CInputDB::GuildRemoveMember(std::unique_ptr<network::DGGuildRemoveMemberPacket> data)
{
	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_id());

	if (g)
		g->RemoveMember(data->pid());
}

void CInputDB::GuildChangeGrade(std::unique_ptr<network::DGGuildChangeGradePacket> data)
{
	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_id());

	if (g)
		g->P2PChangeGrade(data->grade());
}

void CInputDB::GuildChangeMemberData(std::unique_ptr<network::DGGuildChangeMemberDataPacket> data)
{
	CGuild * g = CGuildManager::instance().TouchGuild(data->guild_id());

	if (g)
		g->ChangeMemberData(data->pid(), data->offer(), data->level(), data->grade());
}

void CInputDB::GuildDisband(std::unique_ptr<network::DGGuildDisbandPacket> data)
{
	CGuildManager::instance().DisbandGuild(data->guild_id());
}

void CInputDB::GuildLadder(std::unique_ptr<network::DGGuildLadderPacket> data)
{
	sys_log(0, "Recv GuildLadder %u %d", data->guild_id(), data->ladder_point());
	CGuild * g = CGuildManager::instance().TouchGuild(data->guild_id());

	g->SetLadderPoint(data->ladder_point());
	g->SetWarData(data->wins(), data->draws(), data->losses());
}

void CInputDB::ItemLoad(LPDESC d, std::unique_ptr<DGItemLoadPacket> data)
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()))
		return;

	if (ch->IsItemLoaded())
		return;

	sys_log(0, "ITEM_LOAD: COUNT %s %u", ch->GetName(), data->items_size());

	std::vector<LPITEM> v;
	std::vector<LPITEM> v_lostItems;

	for (auto& p : data->items())
	{
		LPITEM item = ITEM_MANAGER::instance().CreateItem(&p, true);
		if (!item)
		{
			sys_err("cannot create item by vnum %u (name %s id %u)", p.vnum(), ch->GetName(), p.id());
			continue;
		}

#ifdef __MARK_NEW_ITEM_SYSTEM__
		item->SetLastOwnerPID(p.owner());
#endif

		if ((p.cell().window_type() == INVENTORY && ch->GetInventoryItem(p.cell().cell())) ||
				(p.cell().window_type() == EQUIPMENT && ch->GetWear(p.cell().cell())))
		{
			sys_log(0, "ITEM_RESTORE: %s %s", ch->GetName(), item->GetName());
			v.push_back(item);
		}
		else if (p.cell().window_type() == RECOVERY_INVENTORY)
		{
			sys_log(0, "ITEM_RESTORE: %s %s", ch->GetName(), item->GetName());
			if(test_server)sys_err("ITEM_RECOVERY: %s %s", ch->GetName(), item->GetName());
			item->SetWindow(INVENTORY);
			LogManager::instance().ItemLog(ch, item, "ITEM_RECOVERY", "it was lost in offlineshop heaven");
			item->SetSkipSave(false);
			v.push_back(item);
			v_lostItems.push_back(item);
		}
#ifdef __SKILLBOOK_INVENTORY__
		else if (p.cell().window_type() == SKILLBOOK_INVENTORY && ch->GetInventoryItem(p.cell().cell()))
		{
			sys_log(0, "ITEM_RESTORE: %s %s", ch->GetName(), item->GetName());
			v.push_back(item);
		}
#endif
		else if ((p.cell().window_type() == UPPITEM_INVENTORY || p.cell().window_type() == STONE_INVENTORY ||
			p.cell().window_type() == ENCHANT_INVENTORY) && ch->GetInventoryItem(p.cell().cell()))
		{
			sys_log(0, "ITEM_INVENORIES..: %s %s", ch->GetName(), item->GetName());
			v.push_back(item);
		}
#ifdef __COSTUME_INVENTORY__
		else if(p.cell().window_type() == COSTUME_INVENTORY && ch->GetInventoryItem(p.cell().cell()))
		{
			sys_log(0, "ITEM_COSTUME: %s %s", ch->GetName(), item->GetName());
			v.push_back(item);
		}
#endif
		else
		{
			switch (p.cell().window_type())
			{
				case INVENTORY:
#ifdef __SKILLBOOK_INVENTORY__
				case SKILLBOOK_INVENTORY:
#endif
				case UPPITEM_INVENTORY:
				case STONE_INVENTORY:
				case ENCHANT_INVENTORY:
#ifdef __COSTUME_INVENTORY__
				case COSTUME_INVENTORY:
#endif
#ifdef __DRAGONSOUL__
				case DRAGON_SOUL_INVENTORY:
#endif
					if (test_server)
						sys_log(0, "ITEM_LOAD_INV: %s %s to pos %d %d", ch->GetName(), item->GetName(), p.cell().window_type(), p.cell().cell());
					item->AddToCharacter(ch, p.cell());
					break;

				case EQUIPMENT:
					if (item->CheckItemUseLevel(ch->GetLevel()) == true )
					{
						if (test_server)
							sys_log(0, "ITEM_LOAD_EQUIP: %s %s to pos %d %d", ch->GetName(), item->GetName(), p.cell().window_type(), p.cell().cell());
						if (item->EquipTo(ch, p.cell().cell()) == false )
						{
							sys_log(0, "ITEM_RESTORE_EQUIP: %s %s", ch->GetName(), item->GetName());
							v.push_back(item);
						}
					}
					else
					{
						sys_log(0, "ITEM_RESTORE_EQUIP: %s %s", ch->GetName(), item->GetName());
						v.push_back(item);
					}
					break;

				default:
					sys_log(0, "ITEM_RESTORE_UNK: %s %s to pos %d %d", ch->GetName(), item->GetName(), p.cell().window_type(), p.cell().cell());
					//v.push_back(item);
					break;
			}
		}

		if (false == item->OnAfterCreatedItem())
			sys_err("Failed to call ITEM::OnAfterCreatedItem (vnum: %d, id: %u)", item->GetVnum(), item->GetID());

		item->SetSkipSave(false);
	}

	itertype(v) it = v.begin();

	while (it != v.end())
	{
		LPITEM item = *(it++);

		int pos = ch->GetEmptyInventory(item->GetSize());

		if (pos < 0)
		{
			PIXEL_POSITION coord;
			coord.x = ch->GetX();
			coord.y = ch->GetY();

			item->AddToGround(ch->GetMapIndex(), coord);
			item->SetOwnership(ch, 60*10);
			item->StartDestroyEvent(60*10);
			
			ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", LC_TEXT(ch, "Your inventory is full, remaining items are 10 minutes on the ground."));
		}
		else
			item->AddToCharacter(ch, ::TItemPos(INVENTORY, pos));
	}
	
	ch->CheckMaximumPoints();
	ch->PointsPacket();

	ch->SetItemLoaded();
	
	if (v_lostItems.size())
	{
		itertype(v_lostItems) it_ = v_lostItems.begin();
		while (it_ != v_lostItems.end())
		{
			LPITEM item = *(it_++);
			if (item)
			{				
				ch->AddItemRefundCommand("ItemRefundAdd %u %u", item->GetCell(), item->GetCount());
#ifdef __MARK_NEW_ITEM_SYSTEM__
				item->SetLastOwnerPID(0);
#endif
			}
		}
		ch->AddItemRefundCommand("ItemRefundOpen 0");
	}
}

void CInputDB::AffectLoad(LPDESC d, std::unique_ptr<DGAffectLoadPacket> data)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (ch->GetPlayerID() != data->pid())
		return;

	ch->LoadAffect(data->affects());
}

void CInputDB::OfflineMessagesLoad(LPDESC d, std::unique_ptr<DGOfflineMessagesLoadPacket> data)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (ch->GetPlayerID() != data->pid())
		return;

	ch->LoadOfflineMessages(data->messages());
}

#ifdef __ITEM_REFUND__
void CInputDB::ItemRefundLoad(LPDESC d, std::unique_ptr<DGItemRefundLoadPacket> data)
{
	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (ch->GetPlayerID() != data->pid())
		return;

	static std::set<DWORD> s_set_dwUsesIDList;

	std::vector<std::pair<WORD, BYTE> > vecRefundItemData;
	ULONGLONG ullRefundedGold = 0;

	for (auto& elem : data->items())
	{
		if (s_set_dwUsesIDList.find(elem.id()) != s_set_dwUsesIDList.end())
		{
			sys_err("MULTIBLE_REFUND: cannot refund item_refund id: %u", elem.id());
			continue;
		}

		auto item = elem.item();

		const TItemTable* pProto = ITEM_MANAGER::instance().GetTable(item.vnum());
		if (!pProto)
		{
			sys_err("cannot refund item %u for player %u ID %u (no item table)", item.vnum(), data->pid(), elem.id());
			continue;
		}

		DWORD dwItemID = 1;
		if (pProto->type() != ITEM_ELK)
		{
			LPITEM pkItem = ITEM_MANAGER::instance().CreateItem(&item, false, elem.socket_set());
			if (!pkItem)
			{
				sys_err("cannot create refund item vnum %u for player %u", item.vnum(), data->pid());
				continue;
			}

			LogManager::instance().ItemLog(ch, pkItem, "CREATE_REFUND", "Refunded item per item_refund table");

			dwItemID = pkItem->GetID();
			pkItem = ch->AutoGiveItem(pkItem);
			if (pkItem)
				vecRefundItemData.push_back(std::make_pair(pkItem->GetCell(), item.count()));
		}
		else
		{
			ch->PointChange(POINT_GOLD, item.count());
			ullRefundedGold += item.count();
			ch->PointsPacket();
			LogManager::instance().CharLog(ch, item.count(), "CREATE_REFUND_GOLD", "Refunded gold per item_refund table");
		}

		s_set_dwUsesIDList.insert(elem.id());
		DBManager::instance().Query("UPDATE item_refund SET item_id = %u, received_time = NOW() WHERE id = %u", dwItemID, elem.id());
	}

	if (!vecRefundItemData.empty() || ullRefundedGold > 0)
	{
		ch->AddItemRefundCommand("ItemRefundClear");
		for (int i = 0; i < vecRefundItemData.size(); ++i)
			ch->AddItemRefundCommand("ItemRefundAdd %u %u", vecRefundItemData[i].first, vecRefundItemData[i].second);
		ch->AddItemRefundCommand("ItemRefundOpen %llu", ullRefundedGold);
	}
}
#endif

void CInputDB::PartyCreate(std::unique_ptr<network::DGPartyCreatePacket> data)
{
	CPartyManager::instance().P2PCreateParty(data->leader_pid());
}

void CInputDB::PartyDelete(std::unique_ptr<network::DGPartyDeletePacket> data)
{
	CPartyManager::instance().P2PDeleteParty(data->leader_pid());
}

void CInputDB::PartyAdd(std::unique_ptr<network::DGPartyAddPacket> data)
{
	CPartyManager::instance().P2PJoinParty(data->leader_pid(), data->pid(), data->state());
}

void CInputDB::PartyRemove(std::unique_ptr<network::DGPartyRemovePacket> data)
{
	CPartyManager::instance().P2PQuitParty(data->pid());
}

void CInputDB::PartyStateChange(std::unique_ptr<network::DGPartyStateChangePacket> data)
{
	LPPARTY pParty = CPartyManager::instance().P2PCreateParty(data->leader_pid());

	if (!pParty)
		return;

	pParty->SetRole(data->pid(), data->role(), data->flag());
}

void CInputDB::PartySetMemberLevel(std::unique_ptr<network::DGPartySetMemberLevelPacket> data)
{
	LPPARTY pParty = CPartyManager::instance().P2PCreateParty(data->leader_pid());

	if (!pParty)
		return;

	pParty->P2PSetMemberLevel(data->pid(), data->level());
}

void CInputDB::Time(std::unique_ptr<network::DGTimePacket> data)
{
	set_global_time(static_cast<time_t>(data->time()));
}

void CInputDB::ReloadShopTable(std::unique_ptr<DGReloadShopTablePacket> data)
{
	sys_log(0, "RELOAD: SHOP: %d", data->shops_size());

	if (data->shops_size())
		CShopManager::instance().Initialize(data->shops());
}

void CInputDB::ReloadProto(std::unique_ptr<network::DGReloadProtoPacket> data)
{
	/*
	 * Skill
	 */
	if (data->skills_size())
		CSkillManager::instance().Initialize(data->skills());

	/*
	 * ITEM
	 */
	sys_log(0, "RELOAD: ITEM: %d", data->items_size());
	if (data->items_size())
		ITEM_MANAGER::instance().Initialize(data->items());

	/*
	* SHOP
	*/
	sys_log(0, "RELOAD: SHOP: %d", data->shops_size());
	if (data->shops_size())
		CShopManager::instance().Initialize(data->shops());

	/*
	 * MONSTER
	 */
	sys_log(0, "RELOAD: MOB: %d", data->mobs_size());
	if (data->mobs_size())
		CMobManager::instance().Initialize(data->mobs());

	sys_log(0, "RELOAD: soul_proto: %d", data->soul_protos_size());
	if (data->soul_protos_size())
		ITEM_MANAGER::instance().InitializeSoulProto(data->soul_protos());

#ifdef __PET_ADVANCED__
	// pet skills
	for (auto i = 0; i < data->pet_skills_size(); ++i)
		CPetSkillProto::Create(data->pet_skills(i));

	// pet evolve data
	for (auto i = 0; i < data->pet_evolves_size(); ++i)
		CPetEvolveProto::Create(data->pet_evolves(i));

	// pet attr data
	for (auto i = 0; i < data->pet_attrs_size(); ++i)
		CPetAttrProto::Create(data->pet_attrs(i));
#endif

	CMotionManager::instance().Build();

	CHARACTER_MANAGER::instance().for_each_pc(std::mem_fun(&CHARACTER::ComputePoints));
}

void CInputDB::ReloadMobProto(std::unique_ptr<DGReloadMobProtoPacket> data)
{
	sys_log(0, "RELOAD: MOB: %d", data->mobs_size());

	CMobManager::instance().Initialize(data->mobs());

	SendLog("Reloaded mob proto!");
}

void CInputDB::GuildSkillUsableChange(std::unique_ptr<DGGuildSkillUsableChangePacket> data)
{
	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_id());

	g->SkillUsableChange(data->skill_vnum(), data->usable()?true:false);
}

void CInputDB::AuthLogin(LPDESC d, std::unique_ptr<DGAuthLoginPacket> data)
{
	if (!d)
		return;

	network::GCOutputPacket<network::GCAuthSuccessPacket> ptoc;
	ptoc->set_login_key(data->result() ? d->GetLoginKey() : 0);
	ptoc->set_result(data->result());

	d->Packet(ptoc);
	sys_log(0, "AuthLogin result %u key %u", data->result(), d->GetLoginKey());
}

void CInputDB::ChangeEmpirePriv(std::unique_ptr<DGChangeEmpirePrivPacket> p)
{
	// ADD_EMPIRE_PRIV_TIME
	CPrivManager::instance().GiveEmpirePriv(p->empire(), p->type(), p->value(), p->log(), p->end_time_sec());
	// END_OF_ADD_EMPIRE_PRIV_TIME
}

/**
 * @version 05/06/08	Bang2ni - Áö¼Ó½Ã°£ Ãß°¡
 */
void CInputDB::ChangeGuildPriv(std::unique_ptr<DGChangeGuildPrivPacket> p)
{
	// ADD_GUILD_PRIV_TIME
	CPrivManager::instance().GiveGuildPriv(p->guild_id(), p->type(), p->value(), p->log(), p->end_time_sec());
	// END_OF_ADD_GUILD_PRIV_TIME
}

void CInputDB::ChangeCharacterPriv(std::unique_ptr<DGChangeCharacterPrivPacket> p)
{
	CPrivManager::instance().GiveCharacterPriv(p->pid(), p->type(), p->value(), p->log());
}

void CInputDB::GuildMoneyChange(std::unique_ptr<DGGuildMoneyChangePacket> p)
{
	CGuild* g = CGuildManager::instance().TouchGuild(p->guild_id());
	if (g)
	{
		g->RecvMoneyChange(p->total_gold());
	}
}

void CInputDB::GuildMoneyWithdraw(std::unique_ptr<network::DGGuildMoneyWithdrawPacket> data)
{
	CGuild* g = CGuildManager::instance().TouchGuild(data->guild_id());
	if (g)
	{
		g->RecvWithdrawMoneyGive(data->change_gold());
	}
}

void CInputDB::SetEventFlag(std::unique_ptr<network::DGSetEventFlagPacket> data)
{
	quest::CQuestManager::instance().SetEventFlag(data->flag_name(), data->value());
}

void CInputDB::CreateObject(std::unique_ptr<network::DGCreateObjectPacket> data)
{
	using namespace building;
	CManager::instance().LoadObject(&data->object());
}

void CInputDB::DeleteObject(std::unique_ptr<network::DGDeleteObjectPacket> data)
{
	using namespace building;
	CManager::instance().DeleteObject(data->id());
}

void CInputDB::UpdateLand(std::unique_ptr<network::DGUpdateLandPacket> data)
{
	using namespace building;
	CManager::instance().UpdateLand(&data->land());
}

#ifdef __DEPRECATED_BILLING__
////////////////////////////////////////////////////////////////////
// Billing
////////////////////////////////////////////////////////////////////
void CInputDB::BillingRepair(std::unique_ptr<network::DGBillingRepairPacket> data)
{
	for (DWORD i = 0; i < data->logins_size(); ++i)
	{
		CLoginData * pkLD = M2_NEW CLoginData;

		pkLD->SetKey(data->login_keys(i));
		pkLD->SetLogin(data->logins(i).c_str());
		pkLD->SetIP(data->hosts(i).c_str());

		sys_log(0, "BILLING: REPAIR %s host %s", data->logins(i).c_str(), data->hosts(i).c_str());
	}
}

void CInputDB::BillingExpire(std::unique_ptr<network::DGBillingExpirePacket> data)
{
	LPDESC d = DESC_MANAGER::instance().FindByLoginName(data->login());

	if (!d)
		return;

	LPCHARACTER ch = d->GetCharacter();

	if (data->remain_seconds() <= 60)
	{
		int i = MAX(5, data->remain_seconds());
		sys_log(0, "BILLING_EXPIRE: %s %u", data->login().c_str(), data->remain_seconds());
		d->DelayedDisconnect(i);
	}
	else
	{
		if ((data->remain_seconds() - d->GetBillingExpireSecond()) > 60)
		{
			d->SetBillingExpireSecond(data->remain_seconds());

			if (ch)
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your Playtime runs out in %d Minutes."), (data->remain_seconds() / 60));
		}
	}
}

void CInputDB::BillingLogin(std::unique_ptr<network::DGBillingLoginPacket> data)
{
	for (DWORD i = 0; i < data->logins_size(); ++i)
	{
		DBManager::instance().SetBilling(data->login_keys(i), data->logins(i));
	}
}

void CInputDB::BillingCheck(std::unique_ptr<network::DGBillingCheckPacket> data)
{
	for (DWORD i = 0; i < data->keys_size(); ++i)
	{
		sys_log(0, "BILLING: NOT_LOGIN %u", data->keys(i));
		DBManager::instance().SetBilling(data->keys(i), 0, true);
	}
}
#endif

void CInputDB::Notice(std::unique_ptr<network::DGNoticePacket> data)
{
	sys_log(0, "InputDB:: Notice: %s", data->notice().c_str());

	//SendNotice(LC_TEXT(szBuf));
	SendNotice(data->notice().c_str());
}

void CInputDB::GuildWarReserveAdd(std::unique_ptr<network::DGGuildWarReserveAddPacket> data)
{
	CGuildManager::instance().ReserveWarAdd(TGuildWarReserve(
		data->id(), data->guild_from(), data->guild_to(), data->time(), data->type(), data->war_price(), data->initial_score(),
		data->started(), data->bet_from(), data->bet_to(), data->power_from(), data->power_to(), data->handicap()));
}

void CInputDB::GuildWarReserveDelete(std::unique_ptr<network::DGGuildWarReserveDeletePacket> data)
{
	CGuildManager::instance().ReserveWarDelete(data->id());
}

void CInputDB::GuildWarBet(std::unique_ptr<network::DGGuildWarBetPacket> data)
{
	CGuildManager::instance().ReserveWarBet(data->id(), data->login(), data->gold(), data->guild_id());
}

void CInputDB::MarriageAdd(std::unique_ptr<network::DGMarriageAddPacket> data)
{
	sys_log(0, "MarriageAdd %u %u %u %s %s", data->pid1(), data->pid2(), data->marry_time(), data->name1().c_str(), data->name2().c_str());
	marriage::CManager::instance().Add(data->pid1(), data->pid2(), data->marry_time(), data->name1().c_str(), data->name2().c_str());
}

void CInputDB::MarriageUpdate(std::unique_ptr<network::DGMarriageUpdatePacket> data)
{
	sys_log(0, "MarriageUpdate %u %u %d %d", data->pid1(), data->pid2(), data->love_point(), data->married());
	marriage::CManager::instance().Update(data->pid1(), data->pid2(), data->love_point(), data->married());
}

void CInputDB::MarriageRemove(std::unique_ptr<network::DGMarriageRemovePacket> data)
{
	sys_log(0, "MarriageRemove %u %u", data->pid1(), data->pid2());
	marriage::CManager::instance().Remove(data->pid1(), data->pid2());
}

void CInputDB::WeddingRequest(std::unique_ptr<network::DGWeddingRequestPacket> data)
{
	marriage::WeddingManager::instance().Request(data->pid1(), data->pid2());
}

void CInputDB::WeddingReady(std::unique_ptr<network::DGWeddingReadyPacket> data)
{
	sys_log(0, "WeddingReady %u %u %u", data->pid1(), data->pid2(), data->map_index());
	marriage::CManager::instance().WeddingReady(data->pid1(), data->pid2(), data->map_index());
}

void CInputDB::WeddingStart(std::unique_ptr<network::DGWeddingStartPacket> data)
{
	sys_log(0, "WeddingStart %u %u", data->pid1(), data->pid2());
	marriage::CManager::instance().WeddingStart(data->pid1(), data->pid2());
}

void CInputDB::WeddingEnd(std::unique_ptr<network::DGWeddingEndPacket> data)
{
	sys_log(0, "WeddingEnd %u %u", data->pid1(), data->pid2());
	marriage::CManager::instance().WeddingEnd(data->pid1(), data->pid2());
}

// MYSHOP_PRICE_LIST
void CInputDB::MyshopPricelistRes(LPDESC d, std::unique_ptr<DGMyShopPricelistPacket> pack)
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()) )
		return;

	sys_log(0, "RecvMyshopPricelistRes name[%s]", ch->GetName());
	ch->UseSilkBotaryReal(pack->price_info());

}
// END_OF_MYSHOP_PRICE_LIST


//RELOAD_ADMIN
void CInputDB::ReloadAdmin(std::unique_ptr<DGReloadAdminPacket> data)
{
	GM::clear();
	
	for (auto& admin : data->admins())
	{
		GM::insert(admin);

		LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindPC(admin.name().c_str());
		if (pChar)
		{
			pChar->SetGMLevel();
		}
	}

	GM::init(data->admin_configs());
}
//END_RELOAD_ADMIN

void CInputDB::GuildChangeMaster(std::unique_ptr<network::DGGuildChangeMasterPacket> data)
{
	CGuildManager::instance().ChangeMaster(data->guild_id());
}

void CInputDB::DetailLog(std::unique_ptr<network::DGDetailLogPacket> data)
{
	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindByPID(data->pid());

	if (NULL != pChar)
	{
		LogManager::instance().DetailLoginLog(true, pChar);
	}
}

void CInputDB::ItemAwardInformer(std::unique_ptr<network::DGItemAwardInformerPacket> data)
{
	LPDESC d = DESC_MANAGER::instance().FindByLoginName(data->login());	//loginÁ¤º¸

	if (d == NULL)
		return;
	else
	{
		if (d->GetCharacter())
		{
			LPCHARACTER ch = d->GetCharacter();
			ch->SetItemAward_vnum(data->vnum());	// ch ¿¡ ÀÓ½Ã ÀúÀåÇØ³ù´Ù°¡ QuestLoad ÇÔ¼ö¿¡¼­ Ã³¸®
			ch->SetItemAward_cmd(data->command().c_str());

			if (d->IsPhase(PHASE_GAME))			//°ÔÀÓÆäÀÌÁîÀÏ¶§
			{
				quest::CQuestManager::instance().ItemInformer(ch->GetPlayerID(), ch->GetItemAward_vnum());	//questmanager È£Ãâ
			}
		}
	}
}

#ifdef __GUILD_SAFEBOX__
void CInputDB::GuildSafebox(DWORD dwHandle, std::unique_ptr<DGGuildSafeboxPacket> data)
{
	switch (data->sub_header())
	{
	case HEADER_DG_GUILD_SAFEBOX_SET:
		{
			CGuild* pGuild = CGuildManager::Instance().FindGuild(data->guild_id());
			if (pGuild)
			{
				pGuild->GetSafeBox().DB_SetItem(&data->item());
			}
		}
			break;

	case HEADER_DG_GUILD_SAFEBOX_DEL:
		{
			CGuild* pGuild = CGuildManager::Instance().FindGuild(data->guild_id());
			if (pGuild)
			{
				pGuild->GetSafeBox().DB_DelItem(data->item().cell().cell());
			}
		}
			break;

	case HEADER_DG_GUILD_SAFEBOX_GIVE:
		{
			LPDESC d = DESC_MANAGER::Instance().FindByHandle(dwHandle);
			if (d && d->GetCharacter())
			{
				LPCHARACTER pkChr = d->GetCharacter();
				LPITEM pkItem = pkChr->AutoGiveItem(&data->item());
				// pkItem->SetItemCooltime();
			}
			else
			{
				auto& item = data->item();

				char szLogQuery[512];
				snprintf(szLogQuery, sizeof(szLogQuery), "INSERT INTO log.gsafebox_err_log (owner_id, item_id, item_vnum, item_count,"
					"is_gm_owner, socket0, socket1, socket2, attrtype0, attrvalue0, attrtype1, attrvalue1, attrtype2, attrvalue2, attrtype3, attrvalue3, "
					"attrtype4, attrvalue4, attrtype5, attrvalue5, attrtype6, attrvalue6) "
					"VALUES ("
					"%u, %u, %u, %u"
					", %u, %d, %d, %d, %u, %d, %u, %d, %u, %d, %u, %d"
					", %u, %d, %u, %d, %u, %d)",
					item.owner(), item.id(), item.vnum(), item.count(),
					item.is_gm_owner(), item.sockets(0), item.sockets(1), item.sockets(2),
					item.attributes(0).type(), item.attributes(0).value(), item.attributes(1).type(), item.attributes(1).value(), item.attributes(2).type(),
					item.attributes(2).value(), item.attributes(3).type(), item.attributes(3).value(), item.attributes(4).type(), item.attributes(4).value(),
					item.attributes(5).type(), item.attributes(5).value(), item.attributes(6).type(), item.attributes(6).value());
				sys_err(szLogQuery);
				DBManager::Instance().Query(szLogQuery);
			}
		}
			break;

	case HEADER_DG_GUILD_SAFEBOX_GOLD:
		{
			if (dwHandle)
			{
				LPDESC d = DESC_MANAGER::Instance().FindByHandle(dwHandle);
				if (d && d->GetCharacter())
				{
					CGuild* pkGuild = d->GetCharacter()->GetGuild();
					if (pkGuild && pkGuild->GetID() == data->guild_id())
					{
						if (data->gold())
							d->GetCharacter()->PointChange(POINT_GOLD, data->gold());
						else
							d->GetCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(d->GetCharacter(), "There is not enough money in the guild safebox."));
						return;
					}
				}

				// character no longer exists
				GDOutputPacket<GDGuildSafeboxGiveGoldPacket> packet;
				packet->set_guild_id(data->guild_id());
				packet->set_gold(data->gold());
				db_clientdesc->DBPacket(packet);
			}
			else
			{
				CGuild* pkGuild = CGuildManager::instance().FindGuild(data->guild_id());
				if (pkGuild)
				{
					pkGuild->GetSafeBox().DB_SetGold(data->gold());
				}
			}
		}
			break;

	case HEADER_DG_GUILD_SAFEBOX_CREATE:
	case HEADER_DG_GUILD_SAFEBOX_SIZE:
		{
			CGuild* pGuild = CGuildManager::Instance().FindGuild(data->guild_id());
			if (pGuild)
				pGuild->GetSafeBox().DB_SetOwned(data->size());
		}
			break;

	case HEADER_DG_GUILD_SAFEBOX_LOAD:
		{
			CGuild* pGuild = CGuildManager::Instance().FindGuild(data->guild_id());
			if (pGuild)
			{
				pGuild->GetSafeBox().DB_SetGold(data->gold());
				pGuild->GetSafeBox().LoadItem(data->items());

				LPDESC d = NULL;
				if (dwHandle && (d = DESC_MANAGER::Instance().FindByHandle(dwHandle)) && d->GetCharacter())
					pGuild->GetSafeBox().OpenSafebox(d->GetCharacter());
			}
		}
			break;

	case HEADER_DG_GUILD_SAFEBOX_LOG:
		{
			CGuild* pGuild = CGuildManager::Instance().FindGuild(data->guild_id());
			if (pGuild)
				pGuild->GetSafeBox().AppendLog(&data->added_log());
		}
			break;
	}
}
#endif

void CInputDB::WhisperPlayerExistResult(LPDESC desc, std::unique_ptr<DGWhisperPlayerExistResultPacket> p)
{
	sys_log(1, "WhisperPlayerExistResult %s %d %p", p->target_name().c_str(), p->is_exist(), desc);

	if (!desc)
		return;

	if (!desc->GetCharacter() || desc->GetCharacter()->GetPlayerID() != p->pid())
		return;

	if (p->return_money())
		desc->GetCharacter()->PointChange(POINT_GOLD, 500000);

	network::GCOutputPacket<network::GCWhisperPacket> pack;
	
	pack->set_type(p->is_exist() ? (p->is_blocked() ? WHISPER_TYPE_TARGET_BLOCKED : WHISPER_TYPE_NOT_LOGIN) : WHISPER_TYPE_NOT_EXIST);
	pack->set_name_from(p->target_name().c_str());
	pack->set_message(p->message());
	desc->Packet(pack);
	sys_log(0, "WHISPER: no player (%d)", p->is_exist());
}

void CInputDB::WhisperPlayerMessageOffline(LPDESC desc, std::unique_ptr<DGWhisperPlayerMessageOfflinePacket> p)
{
	if (!desc)
		return;

	LPCHARACTER ch;
	if (!(ch = desc->GetCharacter()) || ch->GetPlayerID() != p->pid())
		return;

	LogManager::Instance().WhisperLog(p->pid(), ch->GetName(), p->target_pid(), p->target_name().c_str(), p->message().c_str(), true);
}

#ifdef ENABLE_XMAS_EVENT
void CInputDB::ReloadXmasRewards(std::unique_ptr<DGReloadXmasRewardsPacket> data)
{
	g_vec_xmasRewards.clear();

	for (auto& elem : data->rewards())
		g_vec_xmasRewards.push_back(elem);
}
#endif

#ifdef CHANGE_SKILL_COLOR
void CInputDB::SkillColorLoad(LPDESC d, std::unique_ptr<DGSkillColorLoadPacket> data)
{
	LPCHARACTER ch;

	if (!d || !(ch = d->GetCharacter()))
		return;

	::google::protobuf::RepeatedPtrField<::google::protobuf::RepeatedField<google::protobuf::uint32>> tmp;
	for (auto& clr : data->colors())
		*tmp.Add() = clr.colors();

	ch->SetSkillColor(tmp);
}
#endif

#ifdef __EQUIPMENT_CHANGER__
void CInputDB::EquipmentPageLoad(LPDESC d, std::unique_ptr<DGEquipmentPageLoadPacket> data)
{
	if (!d)
	{
		sys_err("no desc");
		return;
	}

	if (!d->GetCharacter())
	{
		sys_err("no char");
		return;
	}

	LPCHARACTER ch = d->GetCharacter();

	if (ch->GetPlayerID() != data->pid())
	{
		sys_err("wrong pid (expected %u got %u)", data->pid(), ch->GetPlayerID());
		return;
	}

	ch->LoadEquipmentChanger(data->equipments());
}
#endif

#ifdef __PET_ADVANCED__
void CInputDB::PetLoad(std::unique_ptr<network::DGPetLoadPacket> data)
{
	for (auto& t : data->pets())
	{
		if (LPITEM item = ITEM_MANAGER::instance().Find(t.item_id()))
		{
			if (CPetAdvanced* pet = item->GetAdvancedPet())
				pet->Initialize(&t);
		}
	}
}
#endif

#ifdef AUCTION_SYSTEM
void CInputDB::AuctionDeletePlayer(std::unique_ptr<network::DGAuctionDeletePlayer> p)
{
	AuctionManager::instance().on_player_delete(p->pid());
}
#endif

////////////////////////////////////////////////////////////////////
// Analyze
// @version	05/06/10 Bang2ni - ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® ÆÐÅ¶(TDGHeader::MYSHOP_PRICELIST_RES) Ã³¸®·çÆ¾ Ãß°¡.
////////////////////////////////////////////////////////////////////
bool CInputDB::Analyze(LPDESC d, const network::InputPacket& packet)
{
	switch (packet.get_header<TDGHeader>())
	{
	case TDGHeader::BOOT:
		Boot(packet.get<DGBootPacket>());
		break;

	case TDGHeader::LOGIN_SUCCESS:
		LoginSuccess(m_dwHandle, packet.get<DGLoginSuccessPacket>());
		break;

	case TDGHeader::LOGIN_NOT_EXIST:
		LoginFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , "NOID" );
		break;

	case TDGHeader::LOGIN_WRONG_PASSWD:
		LoginFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , "NOID" ); // Do not tell em
		// LoginFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , "WRONGPWD" );
		break;

	case TDGHeader::LOGIN_ALREADY:
		LoginAlready(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGLoginAlreadyPacket>());
		break;

	case TDGHeader::PLAYER_LOAD:
		PlayerLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGPlayerLoadPacket>());
		break;

	case TDGHeader::PLAYER_CREATE_SUCCESS:
		PlayerCreateSuccess(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGPlayerCreateSuccessPacket>());
		break;

	case TDGHeader::PLAYER_CREATE_FAILURE:
		PlayerCreateFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , 0);
		break;

	case TDGHeader::PLAYER_CREATE_ALREADY:
		PlayerCreateFailure(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , 1);
		break;

	case TDGHeader::PLAYER_DELETE_SUCCESS:
		PlayerDeleteSuccess(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGPlayerDeleteSuccessPacket>());
		break;

	case TDGHeader::PLAYER_DELETE_FAILURE:
		//sys_log(0, "PLAYER_DELETE_FAILED" );
		PlayerDeleteFail(DESC_MANAGER::instance().FindByHandle(m_dwHandle) );
		break;

	case TDGHeader::ITEM_LOAD:
		ItemLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGItemLoadPacket>());
		break;

	case TDGHeader::QUEST_LOAD:
		QuestLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGQuestLoadPacket>());
		break;

	case TDGHeader::AFFECT_LOAD:
		AffectLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGAffectLoadPacket>());
		break;

	case TDGHeader::SAFEBOX_LOAD:
		SafeboxLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGSafeboxLoadPacket>());
		break;

	case TDGHeader::SAFEBOX_CHANGE_SIZE:
		SafeboxChangeSize(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGSafeboxChangeSizePacket>());
		break;

	case TDGHeader::SAFEBOX_WRONG_PASSWORD:
		SafeboxWrongPassword(DESC_MANAGER::instance().FindByHandle(m_dwHandle) );
		break;

	case TDGHeader::SAFEBOX_CHANGE_PASSWORD_ANSWER:
		SafeboxChangePasswordAnswer(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGSafeboxChangePasswordAnswerPacket>());
		break;

	case TDGHeader::EMPIRE_SELECT:
		EmpireSelect(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGEmpireSelectPacket>());
		break;

	case TDGHeader::MAP_LOCATIONS:
		MapLocations(packet.get<DGMapLocationsPacket>());
		break;

	case TDGHeader::P2P_INFO:
		P2P(packet.get<DGP2PInfoPacket>());
		break;

	case TDGHeader::GUILD_SKILL_UPDATE:
		GuildSkillUpdate(packet.get<DGGuildSkillUpdatePacket>());
		break;

	case TDGHeader::GUILD_LOAD:
		GuildLoad(packet.get<DGGuildLoadPacket>());
		break;

	case TDGHeader::GUILD_SKILL_RECHARGE:
		GuildSkillRecharge();
		break;

	case TDGHeader::GUILD_EXP_UPDATE:
		GuildExpUpdate(packet.get<DGGuildExpUpdatePacket>());
		break;

	case TDGHeader::PARTY_CREATE:
		PartyCreate(packet.get<DGPartyCreatePacket>());
		break;

	case TDGHeader::PARTY_DELETE:
		PartyDelete(packet.get<DGPartyDeletePacket>());
		break;

	case TDGHeader::PARTY_ADD:
		PartyAdd(packet.get<DGPartyAddPacket>());
		break;

	case TDGHeader::PARTY_REMOVE:
		PartyRemove(packet.get<DGPartyRemovePacket>());
		break;

	case TDGHeader::PARTY_STATE_CHANGE:
		PartyStateChange(packet.get<DGPartyStateChangePacket>());
		break;

	case TDGHeader::PARTY_SET_MEMBER_LEVEL:
		PartySetMemberLevel(packet.get<DGPartySetMemberLevelPacket>());	
		break;

	case TDGHeader::TIME:
		Time(packet.get<DGTimePacket>());
		break;

	case TDGHeader::GUILD_ADD_MEMBER:
		GuildAddMember(packet.get<DGGuildAddMemberPacket>());
		break;

	case TDGHeader::GUILD_REMOVE_MEMBER:
		GuildRemoveMember(packet.get<DGGuildRemoveMemberPacket>());
		break;

	case TDGHeader::GUILD_CHANGE_GRADE:
		GuildChangeGrade(packet.get<DGGuildChangeGradePacket>());
		break;

	case TDGHeader::GUILD_CHANGE_MEMBER_DATA:
		GuildChangeMemberData(packet.get<DGGuildChangeMemberDataPacket>());
		break;

	case TDGHeader::GUILD_DISBAND:
		GuildDisband(packet.get<DGGuildDisbandPacket>());
		break;

	case TDGHeader::RELOAD_SHOP_TABLE:
		ReloadShopTable(packet.get<DGReloadShopTablePacket>());
		break;

	case TDGHeader::RELOAD_PROTO:
		ReloadProto(packet.get<DGReloadProtoPacket>());
		break;

	case TDGHeader::RELOAD_MOB_PROTO:
		ReloadMobProto(packet.get<DGReloadMobProtoPacket>());
		break;

	case TDGHeader::GUILD_WAR:
		GuildWar(packet.get<DGGuildWarPacket>());
		break;

	case TDGHeader::GUILD_WAR_SCORE:
		GuildWarScore(packet.get<DGGuildWarScorePacket>());
		break;

	case TDGHeader::GUILD_LADDER:
		GuildLadder(packet.get<DGGuildLadderPacket>());
		break;

	case TDGHeader::GUILD_SKILL_USABLE_CHANGE:
		GuildSkillUsableChange(packet.get<DGGuildSkillUsableChangePacket>());
		break;

	case TDGHeader::CHANGE_NAME:
		ChangeName(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGChangeNamePacket>());
		break;

	case TDGHeader::AUTH_LOGIN:
		AuthLogin(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGAuthLoginPacket>());
		break;

	case TDGHeader::CHANGE_EMPIRE_PRIV:
		ChangeEmpirePriv(packet.get<DGChangeEmpirePrivPacket>());
		break;

	case TDGHeader::CHANGE_GUILD_PRIV:
		ChangeGuildPriv(packet.get<DGChangeGuildPrivPacket>());
		break;

	case TDGHeader::CHANGE_CHARACTER_PRIV:
		ChangeCharacterPriv(packet.get<DGChangeCharacterPrivPacket>());
		break;

	case TDGHeader::GUILD_MONEY_WITHDRAW:
		GuildMoneyWithdraw(packet.get<DGGuildMoneyWithdrawPacket>());
		break;

	case TDGHeader::GUILD_MONEY_CHANGE:
		GuildMoneyChange(packet.get<DGGuildMoneyChangePacket>());
		break;

	case TDGHeader::SET_EVENT_FLAG:
		SetEventFlag(packet.get<DGSetEventFlagPacket>());
		break;

#ifdef __DEPRECATED_BILLING__
	case TDGHeader::BILLING_REPAIR:
		BillingRepair(packet.get<DGBillingRepairPacket>());
		break;

	case TDGHeader::BILLING_EXPIRE:
		BillingExpire(packet.get<DGBillingExpirePacket>());
		break;

	case TDGHeader::BILLING_LOGIN:
		BillingLogin(packet.get<DGBillingLoginPacket>());
		break;

	case TDGHeader::BILLING_CHECK:
		BillingCheck(packet.get<DGBillingCheckPacket>());
		break;
#endif

	case TDGHeader::CREATE_OBJECT:
		CreateObject(packet.get<DGCreateObjectPacket>());
		break;

	case TDGHeader::DELETE_OBJECT:
		DeleteObject(packet.get<DGDeleteObjectPacket>());
		break;

	case TDGHeader::UPDATE_LAND:
		UpdateLand(packet.get<DGUpdateLandPacket>());
		break;

	case TDGHeader::NOTICE:
		Notice(packet.get<DGNoticePacket>());
		break;

	case TDGHeader::GUILD_WAR_RESERVE_ADD:
		GuildWarReserveAdd(packet.get<DGGuildWarReserveAddPacket>());
		break;

	case TDGHeader::GUILD_WAR_RESERVE_DELETE:
		GuildWarReserveDelete(packet.get<DGGuildWarReserveDeletePacket>());
		break;

	case TDGHeader::GUILD_WAR_BET:
		GuildWarBet(packet.get<DGGuildWarBetPacket>());
		break;

	case TDGHeader::MARRIAGE_ADD:
		MarriageAdd(packet.get<DGMarriageAddPacket>());
		break;

	case TDGHeader::MARRIAGE_UPDATE:
		MarriageUpdate(packet.get<DGMarriageUpdatePacket>());
		break;

	case TDGHeader::MARRIAGE_REMOVE:
		MarriageRemove(packet.get<DGMarriageRemovePacket>());
		break;

	case TDGHeader::WEDDING_REQUEST:
		WeddingRequest(packet.get<DGWeddingRequestPacket>());
		break;

	case TDGHeader::WEDDING_READY:
		WeddingReady(packet.get<DGWeddingReadyPacket>());
		break;

	case TDGHeader::WEDDING_START:
		WeddingStart(packet.get<DGWeddingStartPacket>());
		break;

	case TDGHeader::WEDDING_END:
		WeddingEnd(packet.get<DGWeddingEndPacket>());
		break;

		// MYSHOP_PRICE_LIST
	case TDGHeader::MYSHOP_PRICELIST:
		MyshopPricelistRes(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGMyShopPricelistPacket>());
		break;
		// END_OF_MYSHOP_PRICE_LIST
		//
	// RELOAD_ADMIN
	case TDGHeader::RELOAD_ADMIN:
		ReloadAdmin(packet.get<DGReloadAdminPacket>());		
		break;
	//END_RELOAD_ADMIN

	case TDGHeader::GUILD_CHANGE_MASTER :
		this->GuildChangeMaster(packet.get<DGGuildChangeMasterPacket>());
		break;	
	case TDGHeader::SPARE_ITEM_ID_RANGE :
		ITEM_MANAGER::instance().SetMaxSpareItemID(packet.get<DGSpareItemIDRangePacket>()->data());
		break;

	case TDGHeader::DETAIL_LOG:
		DetailLog(packet.get<DGDetailLogPacket>());
		break;
	// µ¶ÀÏ ¼±¹° ±â´É Å×½ºÆ®
	case TDGHeader::ITEM_AWARD_INFORMER:
		ItemAwardInformer(packet.get<DGItemAwardInformerPacket>());
		break;

#ifdef __GUILD_SAFEBOX__
	case TDGHeader::GUILD_SAFEBOX:
		GuildSafebox(m_dwHandle, packet.get<DGGuildSafeboxPacket>());
		break;
#endif

	case TDGHeader::WHISPER_PLAYER_EXIST_RESULT:
		WhisperPlayerExistResult(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGWhisperPlayerExistResultPacket>());
		break;

	case TDGHeader::WHISPER_PLAYER_MESSAGE_OFFLINE:
		WhisperPlayerMessageOffline(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGWhisperPlayerMessageOfflinePacket>());
		break;

	case TDGHeader::OFFLINE_MESSAGES_LOAD:
		OfflineMessagesLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGOfflineMessagesLoadPacket>());
		break;

#ifdef __ITEM_REFUND__
	case TDGHeader::ITEM_REFUND_LOAD:
		ItemRefundLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGItemRefundLoadPacket>());
		break;
#endif

#ifdef __DUNGEON_FOR_GUILD__
	case TDGHeader::GUILD_DUNGEON:
		GuildDungeon(packet.get<DGGuildDungeonPacket>());
		break;
	case TDGHeader::GUILD_DUNGEON_CD:
		GuildDungeonCD(packet.get<DGGuildDungeonCDPacket>());
		break;
#endif

#ifdef __MAINTENANCE__
	case TDGHeader::MAINTENANCE:
		RecvShutdown(packet.get<DGMaintenancePacket>());
		break;
#endif

#ifdef __CHECK_P2P_BROKEN__
	case TDGHeader::CHECKP2P:
		RecvP2PCheck(packet.get<DGRecvP2PCheckPacket>());
		break;
#endif

#ifdef ENABLE_XMAS_EVENT
	case TDGHeader::RELOAD_XMAS_REWARDS:
		ReloadXmasRewards(packet.get<DGReloadXmasRewardsPacket>());
		break;
#endif

#ifdef CHANGE_SKILL_COLOR
	case TDGHeader::SKILL_COLOR_LOAD:
		SkillColorLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGSkillColorLoadPacket>());
		break;
#endif

#ifdef __EQUIPMENT_CHANGER__
	case TDGHeader::EQUIPMENT_PAGE_LOAD:
		EquipmentPageLoad(DESC_MANAGER::instance().FindByHandle(m_dwHandle) , packet.get<DGEquipmentPageLoadPacket>());
		break;
#endif

#ifdef __PET_ADVANCED__
	case TDGHeader::PET_LOAD:
		PetLoad(packet.get<DGPetLoadPacket>());
		break;
#endif

#ifdef CHECK_IP_ON_CONNECT
	case TDGHeader::WHITELIST_IP:
	{
		auto p = packet.get<DGWhitelistIPPacket>();
		if (test_server)
			sys_err("HEADER_GG_WHITELIST_IP %s", p->ip().c_str());
		if (!DESC_MANAGER::instance().IsWhitelistedIP(p->ip()))
			DESC_MANAGER::instance().AddWhitelistedIP(p->ip());
	}
	break;
#endif

#ifdef AUCTION_SYSTEM
	case TDGHeader::AUCTION_DELETE_PLAYER:
		AuctionDeletePlayer(packet.get<DGAuctionDeletePlayer>());
		break;
#endif

	default:
		sys_err("invalid header in inputDB %u", packet.get_header());
		return false;
	}

	return true;
}

bool CInputDB::Process(LPDESC lpDesc, const void * c_pvOrig, int iBytes, int & r_iBytesProceed)
{
	const char * c_pData = (const char *) c_pvOrig;

	WORD	wLastHeader = 0;
	int		iLastPacketLen = 0;
	network::InputPacket packet;

	for (m_iBufferLeft = iBytes; m_iBufferLeft >= sizeof(network::TPacketHeader) + sizeof(uint32_t);)
	{
		packet.header = *(network::TPacketHeader*) (c_pData);
		if (packet.header.size < sizeof(network::TPacketHeader) + sizeof(uint32_t) || m_iBufferLeft < packet.header.size)
			return true;

		c_pData += sizeof(network::TPacketHeader);

		m_dwHandle = *(uint32_t*) c_pData;
		c_pData += sizeof(uint32_t);

		if (packet.header.header)
		{
			sys_log(!test_server, "DBPacket Analyze [Header %d][bufferLeft %d][size %d, rest_size %d] ",
				packet.get_header(), m_iBufferLeft, packet.header.size,
				packet.header.size - sizeof(network::TPacketHeader) - sizeof(uint32_t));

			packet.content = (const uint8_t*) c_pData;
			packet.content_size = packet.header.size - sizeof(network::TPacketHeader) - sizeof(uint32_t);
			if (!Analyze(lpDesc, packet))
			{
				sys_err("failed to Analyze DB packet [Header %d][bufferLeft %d][size %d]",
					packet.get_header(), m_iBufferLeft, packet.header.size);
			}
		}

		c_pData += packet.header.size - sizeof(network::TPacketHeader) - sizeof(uint32_t);
		m_iBufferLeft -= packet.header.size;
		r_iBytesProceed += packet.header.size;

		iLastPacketLen = packet.header.size;
		wLastHeader = packet.header.header;

		if (GetType() != lpDesc->GetInputProcessor()->GetType())
			return false;
	}

	return true;
}

#ifdef __DUNGEON_FOR_GUILD__
void CInputDB::GuildDungeon(std::unique_ptr<DGGuildDungeonPacket> sPacket)
{
	CGuild* pkGuild = CGuildManager::instance().TouchGuild(sPacket->guild_id());
	if (pkGuild)
		pkGuild->RecvDungeon(sPacket->channel(), sPacket->map_index());
}

void CInputDB::GuildDungeonCD(std::unique_ptr<DGGuildDungeonCDPacket> sPacket)
{
	CGuild* pkGuild = CGuildManager::instance().TouchGuild(sPacket->guild_id());
	if (pkGuild)
		pkGuild->RecvDungeonCD(sPacket->time());
}
#endif

#ifdef __MAINTENANCE__
void CInputDB::RecvShutdown(std::unique_ptr<DGMaintenancePacket> data)
{
	if (test_server)	sys_err("%s:%d Maintenance", __FILE__, __LINE__);
	
	if (data->shutdown_timer() < 0)
	{	
		__StopCurrentShutdown(false);
	}
	else
	{
		sys_err("Accept shutdown p2p command from db.");
		__StartNewShutdown(data->shutdown_timer(), data->maintenance(), data->maintenance_duration(), false);
	}
}
#endif

#ifdef __CHECK_P2P_BROKEN__
void CInputDB::RecvP2PCheck(std::unique_ptr<DGRecvP2PCheckPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("enable_p2p_check") == 0)
		return;

	// -1 this core
	DWORD dwValidPeerAmount = p->valid_peer_count() - 1;
	
	sys_log(0, "+++ Received P2P Check, amount: %d", dwValidPeerAmount);

	DWORD dwCurrentPeerAmount = P2P_MANAGER::Instance().GetDescCount();

	// Check if this core peers amount is >= 2/3 of valid peer amount, else restart this core...
	if (dwCurrentPeerAmount >= ( dwValidPeerAmount * ( 2 / 3 ) ))
		return;

	for (int i=0; i < 10; i++)
		sys_err("+++ RESTARTING CORE DUE TO WRONG P2P AMOUNT, valid amount: %d, curr amount: %d", dwValidPeerAmount, dwCurrentPeerAmount);

	LogManager::Instance().EmergencyLog("P2P Failure | Valid amount: %d, current amount %d | Core restart", dwValidPeerAmount, dwCurrentPeerAmount);

	if (quest::CQuestManager::instance().GetEventFlag("enable_p2p_shutdown") == 0)
		return;
	
	//Shutdown(iSec) so msg appears
	thecore_shutdown();
}
#endif
