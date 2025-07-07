
#include "stdafx.h"

#ifdef __DEPRECATED_BILLING__
#include "../../common/billing.h"
#endif
#include "../../common/building.h"
#include "../../common/VnumHelper.h"
#include "../../common/tables.h"
#include "../../common/utils.h"
#include "../../libgame/include/grid.h"

#include "ClientManager.h"

#include "Main.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"
#include "GuildManager.h"
#include "PrivManager.h"
#include "ItemAwardManager.h"
#include "Marriage.h"
#include "ItemIDRangeManager.h"
#include "Cache.h"

#include <sstream>

#ifdef __GUILD_SAFEBOX__
#include "GuildSafeboxManager.h"
#endif

using namespace network;

#ifdef __EQUIPMENT_CHANGER__
extern int g_iEquipmentPageCacheFlushSeconds;
#endif

extern unsigned char g_bProtoLoadingMethod;
extern int g_test_server;
extern int g_log;

CPacketInfo g_query_info;
CPacketInfo g_item_info;

int g_query_count[2];

CClientManager::CClientManager() :
	m_pkAuthPeer(NULL),
	m_iPlayerIDStart(0),
	m_iPlayerDeleteLevelLimit(0),
	m_iPlayerDeleteLevelLimitLower(0),
	m_iShopTableSize(0),
	m_pShopTable(NULL),
	m_iRefineTableSize(0),
	m_pRefineTable(NULL),
	m_bShutdowned(FALSE),
	m_iCacheFlushCount(0),
	m_iCacheFlushCountLimit(200)
{
	memset(g_query_count, 0, sizeof(g_query_count));
}

CClientManager::~CClientManager()
{
	Destroy();
}

void CClientManager::SetPlayerIDStart(int iIDStart)
{
	m_iPlayerIDStart = iIDStart;
}

void CClientManager::Destroy()
{
	for (auto i = m_peerList.begin(); i != m_peerList.end(); ++i)
		(*i)->Destroy();

	m_peerList.clear();

	if (m_fdAccept > 0)
	{
		socket_close(m_fdAccept);
		m_fdAccept = -1;
	}
}

bool CClientManager::Initialize()
{
	int tmpValue;
	
	//ITEM_UNIQUE_ID
	
	if (!InitializeNowItemID())
	{
		fprintf(stderr, " Item range Initialize Failed. Exit DBCache Server\n");
		return false;
	}
	//END_ITEM_UNIQUE_ID

	if (!InitializeLanguage())
	{
		sys_err("Language Initialize FAILED");
		return false;
	}

	if (!InitializeTables())
	{
		sys_err("Table Initialize FAILED");
		return false;
	}

	CGuildManager::instance().BootReserveWar();

	if (!CConfig::instance().GetValue("BIND_PORT", &tmpValue))
		tmpValue = 5300;

	char szBindIP[128];

	if (!CConfig::instance().GetValue("BIND_IP", szBindIP, 128))
		strlcpy(szBindIP, "0", sizeof(szBindIP));

	m_fdAccept = socket_tcp_bind(szBindIP, tmpValue);

	if (m_fdAccept < 0)
	{
		perror("socket");
		return false;
	}

	sys_log(0, "ACCEPT_HANDLE: %u", m_fdAccept);
	fdwatch_add_fd(m_fdWatcher, m_fdAccept, NULL, FDW_READ, false);

	m_looping = true;

	if (!CConfig::instance().GetValue("PLAYER_DELETE_LEVEL_LIMIT", &m_iPlayerDeleteLevelLimit))
	{
		sys_err("conf.txt: Cannot find PLAYER_DELETE_LEVEL_LIMIT, use default level %d", PLAYER_MAX_LEVEL_CONST + 1);
		m_iPlayerDeleteLevelLimit = PLAYER_MAX_LEVEL_CONST + 1;
	}

	if (!CConfig::instance().GetValue("PLAYER_DELETE_LEVEL_LIMIT_LOWER", &m_iPlayerDeleteLevelLimitLower))
	{
		m_iPlayerDeleteLevelLimitLower = 0;
	}

	sys_log(0, "PLAYER_DELETE_LEVEL_LIMIT set to %d", m_iPlayerDeleteLevelLimit);
	sys_log(0, "PLAYER_DELETE_LEVEL_LIMIT_LOWER set to %d", m_iPlayerDeleteLevelLimitLower);

	LoadEventFlag();

	return true;
}

void CClientManager::MainLoop()
{
	SQLMsg * tmp;

	sys_log(0, "ClientManager pointer is %p", this);

	// 메인루프
	while (!m_bShutdowned)
	{
		while ((tmp = CDBManager::instance().PopResult()))
		{
			AnalyzeQueryResult(tmp);
			delete tmp;
		}

		if (!Process())
			break;

		log_rotate();
	}

	//
	// 메인루프 종료처리
	//
	sys_log(0, "MainLoop exited, Starting cache flushing");

	signal_timer_disable();

	TPlayerCacheMap::iterator it;
	while ((it = m_map_playerCache.begin()) != m_map_playerCache.end())
	{
		DWORD dwOwnerID = it->first;
		sys_log(0, "SHUTDOWN: FLUSH PLAYER CACHE %u", dwOwnerID);

		FlushPlayerCache(dwOwnerID);
	}

#ifdef __PET_ADVANCED__
	auto itPet = m_map_petCache.begin();
	while (itPet != m_map_petCache.end())
	{
		CPetAdvancedCache* c = (itPet++)->second;

		c->Flush();
		delete c;
	}
	m_map_petCache.clear();
#endif

#ifdef __EQUIPMENT_CHANGER__
	sys_log(0, "Flush %u equipment page caches.", m_map_equipmentPageCache.size());
	for (auto& it_equip : m_map_equipmentPageCache)
	{
		CEquipmentPageCache * c = it_equip.second;

		c->Flush();
		delete c;
	}
	m_map_equipmentPageCache.clear();
#endif

#ifdef __GUILD_SAFEBOX__
	CGuildSafeboxManager::Instance().Destroy();
#endif

#ifdef CHANGE_SKILL_COLOR
	for (auto it = m_map_SkillColorCache.begin(); it != m_map_SkillColorCache.end(); ++it)
	{
		CSkillColorCache* pCache = it->second;
		pCache->Flush();
		delete pCache;
	}
	m_map_SkillColorCache.clear();
#endif

	// MYSHOP_PRICE_LIST
	//
	// 개인상점 아이템 가격 리스트 Flush
	//
	for (auto& itPriceList : m_mapItemPriceListCache)
	{
		CItemPriceListTableCache* pCache = itPriceList.second;
		pCache->Flush();
		delete pCache;
	}

	m_mapItemPriceListCache.clear();
	// END_OF_MYSHOP_PRICE_LIST

	SaveEventFlags();
}

#ifdef COMBAT_ZONE
void CClientManager::UpdateSkillsCache(std::unique_ptr<network::GDCombatZoneSkillsCachePacket> p)
{
	char szQuery[2048 + 1];
	sprintf(szQuery,
		"INSERT INTO player.combat_zone_skills_cache (pid, skillLevel1, skillLevel2, skillLevel3, skillLevel4, skillLevel5, skillLevel6) "
		"VALUES('%d', '%d', '%d', '%d', '%d', '%d', '%d') "
		"ON DUPLICATE KEY UPDATE skillLevel1 = '%d', skillLevel2 = '%d', skillLevel3 = '%d', skillLevel4 = '%d', skillLevel5 = '%d', skillLevel6 = '%d'",
		p->pid(), p->skill_level1(), p->skill_level2(), p->skill_level3(), p->skill_level4(), p->skill_level5(), p->skill_level6(),
		p->skill_level1(), p->skill_level2(), p->skill_level3(), p->skill_level4(), p->skill_level5(), p->skill_level6());
	CDBManager::instance().AsyncQuery(szQuery);
}
#endif

#ifdef __MAINTENANCE__
void CClientManager::Maintenance(std::unique_ptr<network::GDRecvShutdownPacket> p)
{
	if (g_test_server)	sys_err("%s:%d Maintenance", __FILE__, __LINE__);
	network::DGOutputPacket<network::DGMaintenancePacket> pack;
	pack->set_maintenance(p->maintenance());
	pack->set_maintenance_duration(p->maintenance_duration());
	pack->set_shutdown_timer(p->start_sec());

	ForwardPacket(pack);
}
#endif

void CClientManager::RecvItemTimedIgnore(std::unique_ptr<network::GDItemTimedIgnorePacket> p)
{
	if (g_test_server)
		sys_log(0, "RecvItemTimedIgnore for item %u duration %u", p->item_id(), p->ignore_duration());

	auto cache = GetItemCache(p->item_id());
	if (!cache)
		return;

	if (g_test_server)
		sys_log(0, "Item disabled.");
	cache->Disable(p->ignore_duration());
}

void CClientManager::Quit()
{
	m_bShutdowned = TRUE;
}

void CClientManager::QUERY_BOOT(CPeer* peer, std::unique_ptr<network::GDBootPacket> p)
{
	const BYTE bPacketVersion = 6; // BOOT 패킷이 바뀔때마다 번호를 올리도록 한다.

	sys_log(0, "QUERY_BOOT : Request server %s", p->ip().c_str());

	network::DGOutputPacket<network::DGBootPacket> pack;

	std::vector<network::TAdminInfo> vAdmin;
	__GetAdminInfo(pack->mutable_admins());
	__GetAdminConfig(pack->mutable_admin_configs());

	for (auto& mob : m_vec_mobTable)
		*pack->add_mobs() = mob;

	for (auto& item : m_vec_itemTable)
		*pack->add_items() = item;

	for (int i = 0; i < m_iShopTableSize; ++i)
		*pack->add_shops() = m_pShopTable[i];

	for (auto& skill : m_vec_skillTable)
		*pack->add_skills() = skill;

	for (int i = 0; i < m_iRefineTableSize; ++i)
		*pack->add_refines() = m_pRefineTable[i];

	for (auto& attr : m_vec_itemAttrTable)
		*pack->add_attrs() = attr;

#ifdef EL_COSTUME_ATTR
	for (auto& attr : m_vec_itemCostumeAttrTable)
		*pack->add_costume_attrs() = attr;
#endif

#ifdef ITEM_RARE_ATTR
	for (auto& attr : m_vec_itemRareTable)
		*pack->add_rare_attrs() = attr;
#endif

	for (auto& land : m_vec_kLandTable)
		*pack->add_lands() = land;

	for (auto& object_proto : m_vec_kObjectProto)
		*pack->add_object_protos() = object_proto;

	for (auto& object : m_map_pkObjectTable)
		*pack->add_objects() = *object.second;

#ifdef __GUILD_SAFEBOX__
	CGuildSafeboxManager::Instance().InitSafeboxCore(pack->mutable_guild_safeboxes());
#endif

	for (auto& horse_upgrade : m_vec_HorseUpgradeProto)
		*pack->add_horse_upgrades() = horse_upgrade;

	for (auto& horse_bonus : m_vec_HorseBonusProto)
		*pack->add_horse_boni() = horse_bonus;
	
#ifdef __GAYA_SYSTEM__
	for (auto& gaya_shop : m_vec_gayaShopTable)
		*pack->add_gaya_shops() = gaya_shop;
#endif

#ifdef __ATTRTREE__
	for (int row = 0; row < ATTRTREE_ROW_NUM; ++row)
	{
		for (int col = 0; col < ATTRTREE_COL_NUM; ++col)
			*pack->add_attrtrees() = m_aAttrTreeProto[row][col];
	}
#endif

#ifdef ENABLE_RUNE_SYSTEM
	for (auto& rune : m_vec_RuneProto)
		*pack->add_runes() = rune;

	for (auto& rune_point : m_vec_RunePointProto)
		*pack->add_rune_points() = rune_point;
#endif

#ifdef ENABLE_XMAS_EVENT
	for (auto& xmas_reward : m_vec_xmasRewards)
		*pack->add_xmas_rewards() = xmas_reward;
#endif

#ifdef SOUL_SYSTEM
	for (auto& soul_attr : m_vec_soulAttrTable)
		*pack->add_soul_protos() = soul_attr;
#endif

#ifdef __PET_ADVANCED__
	for (auto& pet_skill : m_vec_PetSkillProto)
		*pack->add_pet_skills() = pet_skill;
	for (auto& pet_evolve : m_vec_PetEvolveProto)
		*pack->add_pet_evolves() = pet_evolve;
	for (auto& pet_attr : m_vec_PetAttrProto)
		*pack->add_pet_attrs() = pet_attr;
#endif

#ifdef CRYSTAL_SYSTEM
	for (auto& proto : m_vec_crystalTable)
		*pack->add_crystal_protos() = proto;
#endif

	pack->set_current_time(time(0));

	*pack->mutable_item_id_range() = CItemIDRangeManager::instance().GetRange();
	*pack->mutable_item_id_range_spare() = CItemIDRangeManager::instance().GetRange();

	if (g_test_server)
		sys_log(0, "SEND_ITEMID_RANGE: %u~%u useStart %u [spare : %u~%u useStart %u]",
			pack->item_id_range().min_id(), pack->item_id_range().max_id(), pack->item_id_range().usable_item_id_min(),
			pack->item_id_range_spare().min_id(), pack->item_id_range_spare().max_id(), pack->item_id_range_spare().usable_item_id_min());


	sys_log(0, "SEND_BOOT_PACKET[size %u]", pack->ByteSize());
	peer->Packet(pack);

	peer->SetItemIDRange(pack->item_id_range());
	peer->SetSpareItemIDRange(pack->item_id_range_spare());
}

void CClientManager::SendPartyOnSetup(CPeer* pkPeer)
{
#ifdef __PARTY_GLOBAL__
	TPartyMap & pm = m_map_pkParty;
#else
	TPartyMap & pm = m_map_pkChannelParty[pkPeer->GetChannel()];
#endif

	for (auto& it_party : pm)
	{
		sys_log(0, "PARTY SendPartyOnSetup Party [%u]", it_party.first);
		network::DGOutputPacket<network::DGPartyCreatePacket> p;
		p->set_leader_pid(it_party.first);
		pkPeer->Packet(p);

		for (auto& it_member : it_party.second)
		{
			sys_log(0, "PARTY SendPartyOnSetup Party [%u] Member [%u]", it_party.first, it_member.first);

			network::DGOutputPacket<network::DGPartyAddPacket> p_add;
			p_add->set_leader_pid(it_party.first);
			p_add->set_pid(it_member.first);
			p_add->set_state(it_member.second.bRole);
			pkPeer->Packet(p_add);

			network::DGOutputPacket<network::DGPartySetMemberLevelPacket> p_level;
			p_level->set_leader_pid(it_party.first);
			p_level->set_pid(it_member.first);
			p_level->set_level(it_member.second.bLevel);
			pkPeer->Packet(p_level);
		}
	}
}

void CClientManager::QUERY_QUEST_SAVE(CPeer * pkPeer, std::unique_ptr<network::GDQuestSavePacket> p)
{
	char szQuery[1024];

	for (auto& quest : p->datas())
	{
		if (quest.value() == 0)
		{
			DeleteQuestCache(quest.pid(), QUEST_KEY_TYPE(&quest));
		}
		else
		{
			PutQuestCache(&quest);
		}
	}
}

void CClientManager::QUERY_SAFEBOX_LOAD(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDSafeboxLoadPacket> p)
{
	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle);
	strlcpy(pi->safebox_password, p->password().c_str(), sizeof(pi->safebox_password));
	pi->account_id = p->account_id();
	pi->account_index = 0;
	pi->ip[0] = p->is_mall() ? 1 : 0;
	strlcpy(pi->login, p->login().c_str(), sizeof(pi->login));

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT account_id, size, password FROM safebox WHERE account_id=%u",
			p->account_id());
	
	if (g_log)
		sys_log(0, "HEADER_GD_SAFEBOX_LOAD (handle: %d account.id %u is_mall %d)", dwHandle, p->account_id(), p->is_mall() ? 1 : 0);

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_LOAD, pkPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_LOAD(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * pi = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = pi->dwHandle;

	// 여기에서 사용하는 account_index는 쿼리 순서를 말한다.
	// 첫번째 패스워드 알아내기 위해 하는 쿼리가 0
	// 두번째 실제 데이터를 얻어놓는 쿼리가 1

	if (pi->account_index == 0)
	{
		char szSafeboxPassword[SAFEBOX_PASSWORD_MAX_LEN + 1];
		strlcpy(szSafeboxPassword, pi->safebox_password, sizeof(szSafeboxPassword));

		SQLResult * res = msg->Get();

		if (res->uiNumRows == 0)
		{
			if (strcmp("000000", szSafeboxPassword))
			{
				pkPeer->Packet(network::TDGHeader::SAFEBOX_WRONG_PASSWORD, dwHandle, 0);
				delete pi;
				return;
			}
		}
		else
		{
			MYSQL_ROW row = mysql_fetch_row(res->pSQLResult);

			// 비밀번호가 틀리면..
			if (((!row[2] || !*row[2]) && strcmp("000000", szSafeboxPassword)) ||
				((row[2] && *row[2]) && strcmp(row[2], szSafeboxPassword)))
			{
				pkPeer->Packet(network::TDGHeader::SAFEBOX_WRONG_PASSWORD, dwHandle, 0);
				delete pi;
				return;
			}

			if (!row[1])
				pi->safebox_size = 0;
			else
				str_to_number(pi->safebox_size, row[1]);
			/*
			   if (!row[3])
			   pSafebox->dwGold = 0;
			   else
			   pSafebox->dwGold = atoi(row[3]);
			   */
			if (pi->ip[0] == 1)
			{
				pi->safebox_size = 1;
				sys_log(0, "MALL id[%d] size[%d]", pi->account_id, pi->safebox_size);
			}
			else
				sys_log(0, "SAFEBOX id[%d] size[%d]", pi->account_id, pi->safebox_size);
		}

		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), 
				"SELECT %s FROM item WHERE owner_id=%d AND window='%s'",
				GetItemQueryKeyPart(true), pi->account_id, pi->ip[0] == 0 ? "SAFEBOX" : "MALL");

		pi->account_index = 1;

		CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_LOAD, pkPeer->GetHandle(), pi);
	}
	else
	{

		if (pi->safebox_size <= 0)
		{
			sys_err("null safebox size!");
			delete pi;
			return;
		}


		// 쿼리에 에러가 있었으므로 응답할 경우 창고가 비어있는 것 처럼
		// 보이기 때문에 창고가 아얘 안열리는게 나음
		if (!msg->Get()->pSQLResult)
		{
			sys_err("null safebox result");
			delete pi;
			return;
		}

		std::vector<TItemData> items;
		CreateItemTableFromRes(msg->Get()->pSQLResult, [&items]() {
			auto& cur_item = *items.insert(items.end(), TItemData());
			return &cur_item;
			}, pi->account_id);

		std::set<TItemAward *> * pSet = ItemAwardManager::instance().GetByLogin(pi->login);

		if (pSet && !m_vec_itemTable.empty())
		{

			CGrid grid(5, MAX(1, pi->safebox_size) * 10);
			bool bEscape = false;

			for (DWORD i = 0; i < items.size(); ++i)
			{
				auto& r = items[i];

				auto it = m_map_itemTableByVnum.find(r.vnum());

				if (it == m_map_itemTableByVnum.end())
				{
					bEscape = true;
					sys_err("invalid item vnum %u in safebox: login %s", r.vnum(), pi->login);
					break;
				}

				grid.Put(r.cell().cell(), 1, it->second->size());
			}

			if (!bEscape)
			{
				std::vector<std::pair<DWORD, DWORD> > vec_dwFinishedAwardID;

				auto it = pSet->begin();

				char szQuery[512];

				while (it != pSet->end())
				{
					TItemAward * pItemAward = *(it++);
					const DWORD& dwItemVnum = pItemAward->dwVnum;

					if (pItemAward->bTaken)
						continue;

					if (pi->ip[0] == 0 && pItemAward->bMall)
						continue;

					if (pi->ip[0] == 1 && !pItemAward->bMall)
						continue;

					auto it = m_map_itemTableByVnum.find(pItemAward->dwVnum);

					if (it == m_map_itemTableByVnum.end())
					{
						sys_err("invalid item vnum %u in item_award: login %s", pItemAward->dwVnum, pi->login);
						continue;
					}

					TItemTable * pItemTable = it->second;

					int iPos;

					if ((iPos = grid.FindBlank(1, it->second->size())) == -1)
						break;

					network::TItemData item;
					for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
						item.add_sockets(0);
					for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
						item.add_attributes();

					DWORD dwSocket2 = 0;

					if (pItemTable->type() == ITEM_UNIQUE)
					{
						if (pItemAward->dwSocket2 != 0)
							dwSocket2 = pItemAward->dwSocket2;
						else
							dwSocket2 = pItemTable->values(0);
					}
					else if ((dwItemVnum == 50300 || dwItemVnum == 70037) && pItemAward->dwSocket0 == 0)
					{
						DWORD dwSkillIdx;
						DWORD dwSkillVnum;

						do
						{
							dwSkillIdx = random_number(0, m_vec_skillTable.size()-1);

							dwSkillVnum = m_vec_skillTable[dwSkillIdx].vnum();

							if (!dwSkillVnum > 120)
								continue;

							break;
						} while (1);

						pItemAward->dwSocket0 = dwSkillVnum;
					}
					else
					{
						switch (dwItemVnum)
						{
							case 72723: case 72724: case 72725: case 72726:
							case 72727: case 72728: case 72729: case 72730:
							// 무시무시하지만 이전에 하던 걸 고치기는 무섭고...
							// 그래서 그냥 하드 코딩. 선물 상자용 자동물약 아이템들.
							case 76004: case 76005: case 76021: case 76022:
							case 79012: case 79013:
								if (pItemAward->dwSocket2 == 0)
								{
									dwSocket2 = pItemTable->values(0);
								}
								else
								{
									dwSocket2 = pItemAward->dwSocket2;
								}
								break;
						}
					}

					if (GetItemID () > m_itemRange.max_id())
					{
						sys_err("UNIQUE ID OVERFLOW!!");
						break;
					}

					{
						auto it = m_map_itemTableByVnum.find (dwItemVnum);
						if (it == m_map_itemTableByVnum.end())
						{
							sys_err ("Invalid item(vnum : %d). It is not in m_map_itemTableByVnum.", dwItemVnum);
							continue;
						}
						TItemTable* item_table = it->second;
						if (item_table == NULL)
						{
							sys_err ("Invalid item_table (vnum : %d). It's value is NULL in m_map_itemTableByVnum.", dwItemVnum);
							continue;
						}
						if (0 == pItemAward->dwSocket0)
						{
							for (int i = 0; i < ITEM_LIMIT_MAX_NUM; i++)
							{
								if (LIMIT_REAL_TIME == item_table->limits(i).type())
								{
									if (0 == item_table->limits(i).value())
										pItemAward->dwSocket0 = time(0) + 60 * 60 * 24 * 7;
									else
										pItemAward->dwSocket0 = time(0) + item_table->limits(i).value();

									break;
								}
								else if (LIMIT_TIMER_BASED_ON_WEAR == item_table->limits(i).type())
								{
									if (0 == item_table->limits(i).value())
										pItemAward->dwSocket0 = 60 * 60 * 24 * 7;
									else
										pItemAward->dwSocket0 = item_table->limits(i).value();

									break;
								}
								else if (LIMIT_REAL_TIME_START_FIRST_USE == item_table->limits(i).type())
								{
									pItemAward->dwSocket0 = 0;
									pItemAward->dwSocket1 = 0;
									dwSocket2 = 0;

									break;
								}
							}
						}

						item.set_id(GainItemID());
						item.mutable_cell()->set_window_type(pi->ip[0] == 0 ? SAFEBOX : MALL);
						item.mutable_cell()->set_cell(iPos);
						item.set_count(pItemAward->dwCount);
						item.set_vnum(pItemAward->dwVnum);
						item.set_sockets(0, pItemAward->dwSocket0);
						item.set_sockets(1, pItemAward->dwSocket1);
						item.set_sockets(2, dwSocket2);

						TItemTable* curr = m_map_itemTableByVnum[item.vnum()];
						if (curr->type() == ITEM_USE && curr->sub_type() == USE_ADD_SPECIFIC_ATTRIBUTE)
						{
							item.mutable_attributes(0)->set_type(curr->applies(0).type());
							item.mutable_attributes(0)->set_value(curr->applies(0).value());
						}
						else if (curr->type() == ITEM_COSTUME && curr->sub_type() == COSTUME_ACCE)
						{
							item.set_sockets(0, 1);
						}

						if (pItemAward->bMall)
							item.set_is_gm_owner(pItemAward->bIsGMOwner);

						snprintf(szQuery, sizeof(szQuery), 
								"INSERT INTO item (owner_id, %s) VALUES(%u, %s)",
								GetItemQueryKeyPart(false), pi->account_id, GetItemQueryValuePart(&item));
					}

					std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));
					SQLResult * pRes = pmsg->Get();
					sys_log(0, "SAFEBOX Query : [%s]", szQuery);

					if (pRes->uiAffectedRows == 0 || pRes->uiInsertID == 0 || pRes->uiAffectedRows == (uint32_t)-1)
						break;

					items.push_back(item);

					vec_dwFinishedAwardID.push_back(std::make_pair(pItemAward->dwID, item.id()));
					grid.Put(iPos, 1, it->second->size());
				}

				for (DWORD i = 0; i < vec_dwFinishedAwardID.size(); ++i)
					ItemAwardManager::instance().Taken(vec_dwFinishedAwardID[i].first, vec_dwFinishedAwardID[i].second);
			}
		}

		network::DGOutputPacket<network::DGSafeboxLoadPacket> pdg;
		pdg->set_account_id(pi->account_id);
		pdg->set_is_mall(pi->ip[0] == 1);
		pdg->set_size(pi->safebox_size);
		for (auto& item : items)
			*pdg->add_items() = item;

		pkPeer->Packet(pdg, dwHandle);

#ifdef __PET_ADVANCED__
		for (auto& item : pdg->items())
		{
			if (IsPetItem(item.vnum()))
			{
				RequestPetDataForItem(item.id(), pkPeer);
			}
		}
#endif

		delete pi;
	}
}

void CClientManager::QUERY_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<GDSafeboxChangeSizePacket> p)
{
	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle);
	pi->account_index = p->size();	// account_index를 사이즈로 임시로 사용

	char szQuery[QUERY_MAX_LEN];

	if (p->size() <= 3)
		snprintf(szQuery, sizeof(szQuery), "REPLACE INTO safebox (account_id, size) VALUES(%u, %u)", p->account_id(), p->size());
	else
		snprintf(szQuery, sizeof(szQuery), "UPDATE safebox SET size=%u WHERE account_id=%u", p->size(), p->account_id());

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_SIZE, pkPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * p = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = p->dwHandle;
	BYTE bSize = p->account_index;

	delete p;

	if (msg->Get()->uiNumRows > 0)
	{
		network::DGOutputPacket<network::DGSafeboxChangeSizePacket> p;
		p->set_size(bSize);
		pkPeer->Packet(p, dwHandle);
	}
}

void CClientManager::QUERY_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<GDSafeboxChangePasswordPacket> p)
{
	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle);
	strlcpy(pi->safebox_password, p->new_password().c_str(), sizeof(pi->safebox_password));
	strlcpy(pi->login, p->old_password().c_str(), sizeof(pi->login));
	pi->account_id = p->account_id();

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT password FROM safebox WHERE account_id=%u", p->account_id());

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_PASSWORD, pkPeer->GetHandle(), pi);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * p = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = p->dwHandle;

	if (msg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && *row[0] && !strcasecmp(row[0], p->login) || (!row[0] || !*row[0]) && !strcmp("000000", p->login))
		{
			char szQuery[QUERY_MAX_LEN];
			char escape_pwd[64];
			CDBManager::instance().EscapeString(escape_pwd, p->safebox_password, strlen(p->safebox_password));

			snprintf(szQuery, sizeof(szQuery), "UPDATE safebox SET password='%s' WHERE account_id=%u", escape_pwd, p->account_id);

			CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_CHANGE_PASSWORD_SECOND, pkPeer->GetHandle(), p);
			return;
		}
	}

	delete p;

	// Wrong old password
	network::DGOutputPacket<network::DGSafeboxChangePasswordAnswerPacket> pdg;
	pdg->set_flag(0);
	pkPeer->Packet(pdg, dwHandle);
}

void CClientManager::RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(CPeer * pkPeer, SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * p = (ClientHandleInfo *) qi->pvData;
	DWORD dwHandle = p->dwHandle;
	delete p;

	network::DGOutputPacket<network::DGSafeboxChangePasswordAnswerPacket> pdg;
	pdg->set_flag(1);
	pkPeer->Packet(pdg, dwHandle);
}

// MYSHOP_PRICE_LIST
void CClientManager::RESULT_PRICELIST_LOAD(CPeer* peer, SQLMsg* pMsg)
{
	TItemPricelistReqInfo* pReqInfo = (TItemPricelistReqInfo*)static_cast<CQueryInfo*>(pMsg->pvUserData)->pvData;

	//
	// DB 에서 로드한 정보를 Cache 에 저장
	//

	TItemPriceListTable table;
	table.dwOwnerID = pReqInfo->second;
	table.byCount = 0;
	
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		table.aPriceInfo[table.byCount].set_vnum(std::stoul(row[0]));
		table.aPriceInfo[table.byCount].set_price(std::stoll(row[1]));
		table.byCount++;
	}

	PutItemPriceListCache(&table);

	//
	// 로드한 데이터를 Game server 에 전송
	//

	network::DGOutputPacket<network::DGMyShopPricelistPacket> pdg;
	for (int i = 0; i < table.byCount; ++i)
		*pdg->add_price_info() = table.aPriceInfo[i];

	peer->Packet(pdg, pReqInfo->first);

	sys_log(0, "Load MyShopPricelist handle[%d] pid[%d] count[%d]", pReqInfo->first, pReqInfo->second, table.byCount);

	delete pReqInfo;
}

void CClientManager::RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg)
{
	TItemPriceListTable* pUpdateTable = (TItemPriceListTable*)static_cast<CQueryInfo*>(pMsg->pvUserData)->pvData;

	//
	// DB 에서 로드한 정보를 Cache 에 저장
	//

	TItemPriceListTable table;
	table.dwOwnerID = pUpdateTable->dwOwnerID;
	table.byCount = 0;
	
	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		table.aPriceInfo[table.byCount].set_vnum(std::stoul(row[0]));
		table.aPriceInfo[table.byCount].set_price(std::stoll(row[1]));
		table.byCount++;
	}

	PutItemPriceListCache(&table);

	// Update cache
	GetItemPriceListCache(pUpdateTable->dwOwnerID)->UpdateList(pUpdateTable);

	delete pUpdateTable;
}
// END_OF_MYSHOP_PRICE_LIST

void CClientManager::QUERY_SAFEBOX_SAVE(CPeer * pkPeer, std::unique_ptr<GDSafeboxSavePacket> p)
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery),
			"UPDATE safebox SET gold='%lld' WHERE account_id=%u", 
			(long long)p->gold(), p->account_id());

	CDBManager::instance().ReturnQuery(szQuery, QID_SAFEBOX_SAVE, pkPeer->GetHandle(), NULL);
}

void CClientManager::QUERY_EMPIRE_SELECT(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<GDEmpireSelectPacket> p)
{
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), "UPDATE player_index SET empire=%u WHERE id=%u", p->empire(), p->account_id());
	delete CDBManager::instance().DirectQuery(szQuery);

	sys_log(0, "EmpireSelect: %s", szQuery);
	{
		snprintf(szQuery, sizeof(szQuery),
				"SELECT pid1, pid2, pid3, pid4 FROM player_index WHERE id=%u", p->account_id());

		std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult * pRes = pmsg->Get();

		if (pRes->uiNumRows)
		{
			sys_log(0, "EMPIRE %lu", pRes->uiNumRows);

			MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);
			DWORD pids[3];

			UINT g_start_map[4] =
			{
				0,  // reserved
				1,  // 신수국
				21, // 천조국
				41  // 진노국
			};

			// FIXME share with game
			DWORD g_start_position[4][2]=
			{
				{      0,      0 },
				{ 469300, 964200 }, // 신수국
				{  55700, 157900 }, // 천조국
				{ 969600, 278400 }  // 진노국
			};

			for (int i = 0; i < 3; ++i)
			{
				str_to_number(pids[i], row[i]);
				sys_log(0, "EMPIRE PIDS[%d]", pids[i]);

				if (pids[i])
				{
					sys_log(0, "EMPIRE move to pid[%d] to villiage of %u, map_index %d", 
							pids[i], p->empire(), g_start_map[p->empire()]);

					snprintf(szQuery, sizeof(szQuery), "UPDATE player SET map_index=%u,x=%u,y=%u WHERE id=%u", 
							g_start_map[p->empire()],
							g_start_position[p->empire()][0],
							g_start_position[p->empire()][1],
							pids[i]);

					std::auto_ptr<SQLMsg> pmsg2(CDBManager::instance().DirectQuery(szQuery));
				}
			}
		}
	}

	network::DGOutputPacket<network::DGEmpireSelectPacket> pdg;
	pdg->set_empire(p->empire());
	pkPeer->Packet(pdg, dwHandle);
}

void make_map_location_by_peer(network::TMapLocation* location, CPeer* peer)
{
	location->set_host_name(peer->GetPublicIP());
	location->set_port(peer->GetListenPort());
	location->set_channel(peer->GetChannel());
	for (int i = 0; i < MAP_ALLOW_LIMIT; ++i)
		location->add_maps(peer->GetMaps()[i]);
}

void CClientManager::QUERY_SETUP(CPeer * peer, DWORD dwHandle, std::unique_ptr<GDSetupPacket> p)
{
	if (p->auth_server())
	{
		sys_log(0, "AUTH_PEER ptr %p", peer);

		m_pkAuthPeer = peer;
#ifdef __DEPRECATED_BILLING__
		SendAllLoginToBilling();
#endif
		return;
	}

	peer->SetPublicIP(p->public_ip().c_str());
	peer->SetChannel(p->channel());
	peer->SetListenPort(p->listen_port());
	peer->SetP2PPort(p->p2p_port());
	peer->SetMaps(p->maps());
#ifdef PROCESSOR_CORE
	peer->SetProcessorCore(p->processor_core());
#endif

	//
	// 어떤 맵이 어떤 서버에 있는지 보내기
	//

	DGOutputPacket<DGMapLocationsPacket> pack_location_self;
	make_map_location_by_peer(pack_location_self->add_maps(), peer);

	DGOutputPacket<DGMapLocationsPacket> pack_location;

	if (peer->GetChannel() == 1)
	{
		for (auto i = m_peerList.begin(); i != m_peerList.end(); ++i)
		{
			CPeer* tmp = *i;

			if (tmp == peer)
				continue;

			if (!tmp->GetChannel())
				continue;

			bool send_location = tmp->GetChannel() == GUILD_WARP_WAR_CHANNEL || tmp->GetChannel() == peer->GetChannel();
#ifdef PROCESSOR_CORE
			send_location = send_location || peer->IsProcessorCore();
#endif

			if (send_location)
			{
				make_map_location_by_peer(pack_location->add_maps(), tmp);
				tmp->Packet(pack_location_self);
			}
#ifdef PROCESSOR_CORE
			else if (tmp->IsProcessorCore())
				tmp->Packet(pack_location_self);
#endif
		}
	}
	else if (peer->GetChannel() == GUILD_WARP_WAR_CHANNEL)
	{
		for (auto i = m_peerList.begin(); i != m_peerList.end(); ++i)
		{
			CPeer* tmp = *i;

			if (tmp == peer)
				continue;

			if (!tmp->GetChannel())
				continue;

			bool send_location = tmp->GetChannel() == 1 || tmp->GetChannel() == peer->GetChannel();
#ifdef PROCESSOR_CORE
			send_location = send_location || peer->IsProcessorCore();
#endif

			if (send_location)
				make_map_location_by_peer(pack_location->add_maps(), tmp);

			tmp->Packet(pack_location_self);
		}
	}
	else
	{
		for (auto i = m_peerList.begin(); i != m_peerList.end(); ++i)
		{
			CPeer * tmp = *i;

			if (tmp == peer)
				continue;

			if (!tmp->GetChannel())
				continue;

			bool send_location = tmp->GetChannel() == GUILD_WARP_WAR_CHANNEL || tmp->GetChannel() == peer->GetChannel();
#ifdef PROCESSOR_CORE
			send_location = send_location || peer->IsProcessorCore();
#endif

			if (send_location)
				make_map_location_by_peer(pack_location->add_maps(), tmp);

			if (tmp->GetChannel() == peer->GetChannel())
				tmp->Packet(pack_location_self);
#ifdef PROCESSOR_CORE
			else if (tmp->IsProcessorCore())
				tmp->Packet(pack_location_self);
#endif
		}
	}

	*pack_location->add_maps() = pack_location_self->maps(0);
	peer->Packet(pack_location);

	sys_log(0, "SETUP: channel %u listen %u p2p %u count %u", peer->GetChannel(), p->listen_port(), p->p2p_port(), pack_location->maps_size());

	DGOutputPacket<DGP2PInfoPacket> pack_p2p;
	pack_p2p->set_port(peer->GetP2PPort());
	pack_p2p->set_listen_port(peer->GetListenPort());
	pack_p2p->set_channel(peer->GetChannel());
	pack_p2p->set_host(peer->GetPublicIP());
#ifdef PROCESSOR_CORE
	pack_p2p->set_processor_core(peer->IsProcessorCore());
#endif

	ForwardPacket(pack_p2p, 0, peer);

#ifdef __DEPRECATED_BILLING__
	DGOutputPacket<DGBillingRepairPacket> pack_repair;
#endif
	for (DWORD c = 0; c < p->logins_size(); ++c)
	{
		CLoginData * pkLD = new CLoginData;

		auto& elem = p->logins(c);

		DWORD client_keys[4];
		for (int i = 0; i < elem.client_keys_size(); ++i)
			client_keys[i] = elem.client_keys(i);

		pkLD->SetKey(elem.login_key());
		pkLD->SetClientKey(client_keys);
		pkLD->SetIP(elem.host().c_str());

		TAccountTable & r = pkLD->GetAccountRef();

		r.set_id(elem.id());
		char login_lower[LOGIN_MAX_LEN + 1];
		trim_and_lower(elem.login().c_str(), login_lower, sizeof(login_lower));
		r.set_login(login_lower);
		r.set_social_id(elem.social_id());
		r.set_passwd("TEMP");

		InsertLoginData(pkLD);

		if (InsertLogonAccount(elem.login().c_str(), peer->GetHandle(), elem.host().c_str()))
		{
			sys_log(0, "SETUP: login %u %s login_key %u host %s", elem.id(), elem.login().c_str(), elem.login_key(), elem.host().c_str());
			pkLD->SetPlay(true);

#ifdef __DEPRECATED_BILLING__
			if (m_pkAuthPeer)
			{
				pack_repair->add_login_keys(pkLD->GetKey());
				pack_repair->add_logins(elem.login());
				pack_repair->add_hosts(elem.host());
			}
#endif
		}
		else
			sys_log(0, "SETUP: login_fail %u %s login_key %u", elem.id(), elem.login().c_str(), elem.login_key());
	}

#ifdef __DEPRECATED_BILLING__
	if (m_pkAuthPeer && pack_repair->logins_size() > 0)
	{
		sys_log(0, "REPAIR size %d", pack_repair->logins_size());
		m_pkAuthPeer->Packet(pack_repair);
	}
#endif

	SendPartyOnSetup(peer);
	CGuildManager::instance().OnSetup(peer);
	CPrivManager::instance().SendPrivOnSetup(peer);
	SendEventFlagsOnSetup(peer);
	marriage::CManager::instance().OnSetup(peer);
}

void CClientManager::QUERY_ITEM_FLUSH(CPeer * pkPeer, std::unique_ptr<GDItemFlushPacket> p)
{
	if (g_log)
		sys_log(0, "HEADER_GD_ITEM_FLUSH: %u", p->item_id());

	CItemCache * c = GetItemCache(p->item_id());

	if (c)
	{
		c->Flush();

#ifdef __PET_ADVANCED__
		if (!c->Get()->vnum() || IsPetItem(c->Get()->vnum()))
			FlushPetCache(p->item_id());
#endif
	}
}

void CClientManager::QUERY_ITEM_SAVE(CPeer * pkPeer, std::unique_ptr<GDItemSavePacket> p)
{
	auto& item = p->data();
	auto window = item.cell().window_type();

	std::set<BYTE> direct_save_windows;
	direct_save_windows.insert(SAFEBOX);
	direct_save_windows.insert(MALL);
#ifdef AUCTION_SYSTEM
	direct_save_windows.insert(AUCTION);
	direct_save_windows.insert(AUCTION_SHOP);
#endif

	if (direct_save_windows.find(window) != direct_save_windows.end())
	{
		// erase item cache for direct save
		EraseItemCache(item.id());

#ifdef AUCTION_SYSTEM
		if (window == AUCTION || window == AUCTION_SHOP)
			_last_auction_item_save[item.id()] = get_dword_time();
#endif

#ifdef __PET_ADVANCED__
		if (IsPetItem(item.vnum()))
			FlushPetCache(item.id());
#endif

		// direct save
		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), 
			"REPLACE INTO item (owner_id, %s) "
			"VALUES(%u, %s)",
			GetItemQueryKeyPart(false),
			item.owner(),
			GetItemQueryValuePart(&item));

		CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_SAVE, pkPeer->GetHandle(), nullptr);
	}
	else
	{
#ifdef AUCTION_SYSTEM
		auto it = _last_auction_item_save.find(item.id());
		if (it != _last_auction_item_save.end())
		{
			if (get_dword_time() - it->second < 1500)
			{
				sys_err("skip item save (last auction item save is less than 1500 ms ago) for id %u", item.id());
				return;
			}

			_last_auction_item_save.erase(it);
		}
#endif

		if (g_test_server)
			sys_log(0, "QUERY_ITEM_SAVE => PutItemCache() owner %d id %d vnum %d window %u cell %u", item.owner(), item.id(), item.vnum(), window, item.cell().cell());

		PutItemCache(&item);
	}
}

#ifdef __EQUIPMENT_CHANGER__
void CClientManager::QUERY_EQUIPMENT_CHANGER_SAVE(CPeer * pkPeer, std::unique_ptr<network::GDEquipmentPageSavePacket> p)
{
	for (auto& tab : p->pages())
		PutEquipmentPageCache(&tab);
}

void CClientManager::QUERY_EQUIPMENT_CHANGER_DELETE(CPeer * pkPeer, std::unique_ptr<network::GDEquipmentPageDeletePacket> p)
{
	for (int i = 0; i < EQUIPMENT_CHANGER_MAX_PAGES; ++i)
		EraseEquipmentPageCache(p->pid(), i);
}

CClientManager::TEquipmentPageCacheSet * CClientManager::GetEquipmentPageCacheSet(DWORD pid)
{
	TEquipmentPageCacheSetPtrMap::iterator it = m_map_pkEquipmentPageSetPtr.find(pid);

	if (it == m_map_pkEquipmentPageSetPtr.end())
		return NULL;

	return it->second;
}

void CClientManager::CreateEquipmentPageCacheSet(DWORD pid)
{
	if (m_map_pkEquipmentPageSetPtr.find(pid) != m_map_pkEquipmentPageSetPtr.end())
		return;

	TEquipmentPageCacheSet * pSet = new TEquipmentPageCacheSet;
	m_map_pkEquipmentPageSetPtr.insert(TEquipmentPageCacheSetPtrMap::value_type(pid, pSet));
}

void CClientManager::FlushEquipmentPageCacheSet(DWORD pid)
{
	TEquipmentPageCacheSetPtrMap::iterator it = m_map_pkEquipmentPageSetPtr.find(pid);

	if (it == m_map_pkEquipmentPageSetPtr.end())
	{
		sys_log(0, "FLUSH_EQUIPMENTPAGECACHESET : No EquipmentPageCacheSet pid(%d)", pid);
		return;
	}

	TEquipmentPageCacheSet * pSet = it->second;
	TEquipmentPageCacheSet::iterator it_set = pSet->begin();

	while (it_set != pSet->end())
	{
		CEquipmentPageCache * c = *it_set++;
		c->Flush();

		std::string map_index = std::to_string(c->Get()->pid()) + "_" + std::to_string(c->Get()->index());
		m_map_equipmentPageCache.erase(map_index);
		delete c;
	}

	pSet->clear();
	delete pSet;

	m_map_pkEquipmentPageSetPtr.erase(it);

	if (g_log)
		sys_log(0, "FLUSH_EQUIPMENTPAGECACHESET : Deleted pid(%d)", pid);
}

CEquipmentPageCache * CClientManager::GetEquipmentPageCache(DWORD id, DWORD index)
{
	std::string map_index = std::to_string(id) + "_" + std::to_string(index);
	TEquipmentPageCacheMap::iterator it = m_map_equipmentPageCache.find(map_index);

	if (it == m_map_equipmentPageCache.end())
		return NULL;

	return it->second;
}

void CClientManager::PutEquipmentPageCache(const TEquipmentChangerTable * pNew, bool bSkipQuery)
{
	CEquipmentPageCache* c;

	c = GetEquipmentPageCache(pNew->pid(), pNew->index());

	if (!c)
	{
		if (pNew->page_name().empty())
			return;

		c = new CEquipmentPageCache;
		std::string map_index = std::to_string(pNew->pid()) + "_" + std::to_string(pNew->index());
		m_map_equipmentPageCache.insert(TEquipmentPageCacheMap::value_type(map_index, c));
	}

	c->Put(pNew, bSkipQuery);

	TEquipmentPageCacheSetPtrMap::iterator it = m_map_pkEquipmentPageSetPtr.find(c->Get()->pid());

	if (it != m_map_pkEquipmentPageSetPtr.end())
	{
		it->second->insert(c);
	}
	else
	{
		c->OnFlush();
	}
}

bool CClientManager::DeleteEquipmentPageCache(DWORD pid, DWORD index)
{
	CEquipmentPageCache * c = GetEquipmentPageCache(pid, index);

	if (!c)
		return false;

	c->Delete();
	return true;
}

void CClientManager::EraseEquipmentPageCache(DWORD pid, DWORD index)
{
	CEquipmentPageCache* c = GetEquipmentPageCache(pid, index);

	if (c)
	{
		TEquipmentPageCacheSetPtrMap::iterator it = m_map_pkEquipmentPageSetPtr.find(pid);

		if (it != m_map_pkEquipmentPageSetPtr.end())
			it->second->erase(c);
		

		std::string map_index = std::to_string(pid) + "_" + std::to_string(index);
		m_map_equipmentPageCache.erase(map_index);

		delete c;
	}
}
#endif

#ifdef __PET_ADVANCED__
bool CClientManager::IsPetItem(DWORD item_vnum)
{
	auto itemProto = GetItemTable(item_vnum);
	if (!itemProto)
		return false;

	if (itemProto->type() != ITEM_PET_ADVANCED)
		return false;
	if (itemProto->sub_type() != static_cast<uint32_t>(EPetAdvancedItem::SUMMON))
		return false;

	return true;
}

CPetAdvancedCache* CClientManager::GetPetCache(DWORD item_id)
{
	TPetCacheMap::iterator it = m_map_petCache.find(item_id);

	if (it == m_map_petCache.end())
		return NULL;

	return it->second;
}

void CClientManager::PutPetCache(TPetAdvancedTable* pNew, bool bSkipQuery)
{
	CPetAdvancedCache* c;

	c = GetPetCache(pNew->item_id());

	if (!c)
	{
		if (g_log)
			sys_log(0, "PET_CACHE: PutPetCache ==> New CPetCache item_id%d name[%s] level%d", pNew->item_id(), pNew->name().c_str(), pNew->level());

		c = new CPetAdvancedCache;
		m_map_petCache.insert(TPetCacheMap::value_type(pNew->item_id(), c));
	}
	else
	{
		// update changed state of skills (because game resets it on load)
		auto& skills = c->Get()->skills();
		for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		{
			if (skills[i].changed())
				pNew->mutable_skills(i)->set_changed(true);
		}
	}

	c->Put(pNew, bSkipQuery);

	CItemCache* itemCache = GetItemCache(pNew->item_id());
	if ((!itemCache || !GetPlayerCache(itemCache->Get()->owner(), false)) && !bSkipQuery)
		c->OnFlush();
}

bool CClientManager::DeletePetCache(DWORD item_id)
{
	CPetAdvancedCache* c = GetPetCache(item_id);

	if (!c)
	{
		char szQuery[1024];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM pet WHERE item_id=%u", item_id);
		CDBManager::Instance().AsyncQuery(szQuery);
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM pet_skill WHERE item_id=%u", item_id);
		CDBManager::Instance().AsyncQuery(szQuery);

		return false;
	}

	c->Delete();
	return true;
}

void CClientManager::FlushPetCache(DWORD item_id)
{
	CPetAdvancedCache* c = GetPetCache(item_id);
	if (c)
	{
		c->Flush();
		delete c;

		m_map_petCache.erase(item_id);
	}
}

TPetAdvancedTable* CClientManager::RequestPetDataForItem(DWORD item_id, CPeer* targetPeer)
{
	CPetAdvancedCache* petCache = GetPetCache(item_id);
	if (petCache)
		return petCache->Get();

	std::ostringstream query;
	query << "SELECT name, `level`, exp, `exp_item`";
	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
		query << ", attrtype" << (i + 1) << ", attrlevel" << (i + 1);
	query << ", skillpower FROM pet WHERE item_id = " << item_id;
	CDBManager::Instance().ReturnQuery(query.str().c_str(), QID_PET_ADVANCED_LOAD, targetPeer->GetHandle(), new DWORD(item_id));

	query.str("");
	query.clear();
	query << "SELECT `index`, vnum, `level` FROM pet_skill  WHERE item_id = " << item_id;
	CDBManager::Instance().ReturnQuery(query.str().c_str(), QID_PET_ADVANCED_LOAD_SKILL, targetPeer->GetHandle(), new DWORD(item_id));

	return NULL;
}

void CClientManager::RESULT_PET_LOAD(CPeer* peer, SQLMsg* pMsg)
{
	CQueryInfo* qi = (CQueryInfo*) pMsg->pvUserData;
	std::unique_ptr<DWORD> item_id((DWORD*) qi->pvData);

	TPetAdvancedTable petData;

	for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		petData.add_skills();

	for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
	{
		petData.add_attr_type(0);
		petData.add_attr_level(0);
	}

	petData.set_item_id(*item_id);

	MYSQL_RES* pSQLResult = pMsg->Get()->pSQLResult;
	if (MYSQL_ROW row = mysql_fetch_row(pSQLResult))
	{
		int col = 0;

		petData.set_name(row[col++]);
		petData.set_level(std::stoi(row[col++]));
		petData.set_exp(std::stoll(row[col++]));
		petData.set_exp_item(std::stoll(row[col++]));
		
		for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
		{
			petData.set_attr_type(i, std::stoi(row[col++]));
			petData.set_attr_level(i, std::stoi(row[col++]));
		}

		petData.set_skillpower(std::stoi(row[col++]));
	}

	PutPetCache(&petData, true);
}

void CClientManager::RESULT_PET_SKILL_LOAD(CPeer* peer, SQLMsg* pMsg)
{
	CQueryInfo* qi = (CQueryInfo*)pMsg->pvUserData;
	std::unique_ptr<DWORD> item_id((DWORD*)qi->pvData);

	CPetAdvancedCache* petCache = GetPetCache(*item_id);
	if (!petCache)
	{
		sys_err("RESULT_PET_SKILL_LOAD[%u] -- NO PET CACHE available");
		return;
	}

	TPetAdvancedTable petData = *petCache->Get();

	MYSQL_RES* pSQLResult = pMsg->Get()->pSQLResult;
	if (mysql_num_rows(pSQLResult) > 0)
	{
		while (MYSQL_ROW row = mysql_fetch_row(pSQLResult))
		{
			int index;
			str_to_number(index, row[0]);

			auto skill = petData.mutable_skills(index);

			skill->set_vnum(std::stoi(row[1]));
			skill->set_level(std::stoi(row[2]));
		}

		if (GetItemCache(*item_id) != NULL) // don't add to cache if it's loaded for safebox / mall
			PutPetCache(&petData, true);
		else // then delete frequently created pet cache (because safebox / mall shouldn't be cached)
		{
			m_map_petCache.erase(*item_id);
			delete petCache;
		}
	}

	network::DGOutputPacket<network::DGPetLoadPacket> pack;
	*pack->add_pets() = petData;
	peer->Packet(pack);
}

void CClientManager::QUERY_PET_SAVE(CPeer* peer, std::unique_ptr<GDPetSavePacket> pack)
{
	auto& pet = *pack->mutable_data();

	if (GetItemCache(pet.item_id()) != NULL)
		PutPetCache(&pet);
	else
	{
		CPetAdvancedCache* petCache = GetPetCache(pet.item_id());
		if (petCache)
		{
			delete petCache;
			m_map_petCache.erase(pet.item_id());
		}

		CPetAdvancedCache tmpPetCache;
		tmpPetCache.Put(&pet);
		tmpPetCache.OnFlush();
	}
}
#endif

// MYSHOP_PRICE_LIST
CItemPriceListTableCache* CClientManager::GetItemPriceListCache(DWORD dwID)
{
	TItemPriceListCacheMap::iterator it = m_mapItemPriceListCache.find(dwID);

	if (it == m_mapItemPriceListCache.end())
		return NULL;

	return it->second;
}

void CClientManager::PutItemPriceListCache(const TItemPriceListTable* pItemPriceList)
{
	CItemPriceListTableCache* pCache = GetItemPriceListCache(pItemPriceList->dwOwnerID);

	if (!pCache)
	{
		pCache = new CItemPriceListTableCache;
		m_mapItemPriceListCache.insert(TItemPriceListCacheMap::value_type(pItemPriceList->dwOwnerID, pCache));
	}

	pCache->Put(const_cast<TItemPriceListTable*>(pItemPriceList), true);
}
// END_OF_MYSHOP_PRICE_LIST

void CClientManager::SetCacheFlushCountLimit(int iLimit)
{
	m_iCacheFlushCountLimit = MAX(10, iLimit);
	sys_log(0, "CACHE_FLUSH_LIMIT_PER_SECOND: %d", m_iCacheFlushCountLimit);
}

void CClientManager::UpdateItemPriceListCache()
{
	TItemPriceListCacheMap::iterator it = m_mapItemPriceListCache.begin();

	while (it != m_mapItemPriceListCache.end() && m_iCacheFlushCount < m_iCacheFlushCountLimit)
	{
		CItemPriceListTableCache* pCache = it->second;

		if (pCache->CheckFlushTimeout())
		{
			pCache->Flush();
			m_mapItemPriceListCache.erase(it++);
			delete pCache;
			++m_iCacheFlushCount;
		}
		else
			++it;
	}
}

void CClientManager::QUERY_ITEM_DESTROY(CPeer * pkPeer, std::unique_ptr<GDItemDestroyPacket> p)
{
	if (!DeleteItemCache(p->item_id()))
	{
		char szQuery[64];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE id=%u", p->item_id());

		// if (g_log)
			// sys_log(0, "HEADER_GD_ITEM_DESTROY: PID %u ID %u", dwPID, dwID);

		if (p->pid() == 0) // 아무도 가진 사람이 없었다면, 비동기 쿼리
			CDBManager::instance().AsyncQuery(szQuery);
		else
			CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_DESTROY, pkPeer->GetHandle(), NULL);

#ifdef __PET_ADVANCED__
		DeletePetCache(p->item_id());
#endif
	}
}

void CClientManager::QUERY_FLUSH_CACHE(CPeer * pkPeer, std::unique_ptr<GDFlushCachePacket> p)
{
	FlushPlayerCache(p->pid());
}

void CClientManager::ReloadShopProto()
{
	if (!InitializeShopTable())
	{
		sys_err("ReloadShopProto: cannot load shop table");
		return;
	}

	network::DGOutputPacket<network::DGReloadShopTablePacket> p;
	for (int i = 0; i < m_iShopTableSize; ++i)
		*p->add_shops() = m_pShopTable[i];

	for (TPeerList::iterator i = m_peerList.begin(); i != m_peerList.end(); ++i)
	{
		CPeer * tmp = *i;

		if (!tmp->GetChannel())
			continue;

		tmp->Packet(p);
	}
}

void CClientManager::QUERY_RELOAD_PROTO()
{
	if (!InitializeTables())
	{
		sys_err("QUERY_RELOAD_PROTO: cannot load tables");
		return;
	}

	network::DGOutputPacket<network::DGReloadProtoPacket> p;
	for (auto& item : m_vec_itemTable)
		*p->add_items() = item;
	for (auto& mob : m_vec_mobTable)
		*p->add_mobs() = mob;
	for (auto& skill : m_vec_skillTable)
		*p->add_skills() = skill;
	for (int i = 0; i < m_iShopTableSize; ++i)
		*p->add_shops() = m_pShopTable[i];
#ifdef SOUL_SYSTEM
	for (auto& soul_attr : m_vec_soulAttrTable)
		*p->add_soul_protos() = soul_attr;
#endif

#ifdef __PET_ADVANCED__
	for (auto& pet_skill : m_vec_PetSkillProto)
		*p->add_pet_skills() = pet_skill;
	for (auto& pet_evolve : m_vec_PetEvolveProto)
		*p->add_pet_evolves() = pet_evolve;
	for (auto& pet_attr : m_vec_PetAttrProto)
		*p->add_pet_attrs() = pet_attr;
#endif

	for (TPeerList::iterator i = m_peerList.begin(); i != m_peerList.end(); ++i)
	{
		CPeer * tmp = *i;

		if (!tmp->GetChannel())
			continue;

		tmp->Packet(p);
	}
}

void CClientManager::QUERY_RELOAD_MOB_PROTO()
{
	if (g_bProtoLoadingMethod == PROTO_LOADING_DATABASE)
	{
		if (!InitializeMobTableFromDatabase())
			return;
	}
	else
	{
		if (!InitializeMobTableFromTextFile())
			return;
	}

	network::DGOutputPacket<network::DGReloadMobProtoPacket> p;
	for (auto& mob : m_vec_mobTable)
		*p->add_mobs() = mob;

	for (TPeerList::iterator i = m_peerList.begin(); i != m_peerList.end(); ++i)
	{
		CPeer * tmp = *i;

		if (!tmp->GetChannel())
			continue;

		tmp->Packet(p);
	}
}

// ADD_GUILD_PRIV_TIME
/**
 * @version	05/06/08 Bang2ni - 지속시간 추가
 */
void CClientManager::AddGuildPriv(std::unique_ptr<GDRequestGuildPrivPacket> p)
{
	CPrivManager::instance().AddGuildPriv(p->guild_id(), p->type(), p->value(), p->duration_sec());
}

void CClientManager::AddEmpirePriv(std::unique_ptr<GDRequestEmpirePrivPacket> p)
{
	CPrivManager::instance().AddEmpirePriv(p->empire(), p->type(), p->value(), p->duration_sec());
}
// END_OF_ADD_GUILD_PRIV_TIME

void CClientManager::AddCharacterPriv(std::unique_ptr<GDRequestCharacterPrivPacket> p)
{
	CPrivManager::instance().AddCharPriv(p->pid(), p->type(), p->value());
}

CLoginData * CClientManager::GetLoginData(DWORD dwKey)
{
	TLoginDataByLoginKey::iterator it = m_map_pkLoginData.find(dwKey);

	if (it == m_map_pkLoginData.end())
		return NULL;

	return it->second;
}

CLoginData * CClientManager::GetLoginDataByLogin(const char * c_pszLogin)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(c_pszLogin, szLogin, sizeof(szLogin));

	TLoginDataByLogin::iterator it = m_map_pkLoginDataByLogin.find(szLogin);

	if (it == m_map_pkLoginDataByLogin.end())
		return NULL;

	return it->second;
}

CLoginData * CClientManager::GetLoginDataByAID(DWORD dwAID)
{
	TLoginDataByAID::iterator it = m_map_pkLoginDataByAID.find(dwAID);

	if (it == m_map_pkLoginDataByAID.end())
		return NULL;

	return it->second;
}

void CClientManager::InsertLoginData(CLoginData * pkLD)
{
	char szLogin[LOGIN_MAX_LEN + 1];
	trim_and_lower(pkLD->GetAccountRef().login().c_str(), szLogin, sizeof(szLogin));

	m_map_pkLoginData.insert(std::make_pair(pkLD->GetKey(), pkLD));
	m_map_pkLoginDataByLogin.insert(std::make_pair(szLogin, pkLD));
	m_map_pkLoginDataByAID.insert(std::make_pair(pkLD->GetAccountRef().id(), pkLD));
}

void CClientManager::DeleteLoginData(CLoginData * pkLD)
{
	m_map_pkLoginData.erase(pkLD->GetKey());
	m_map_pkLoginDataByLogin.erase(pkLD->GetAccountRef().login().c_str());
	m_map_pkLoginDataByAID.erase(pkLD->GetAccountRef().id());

	if (m_map_kLogonAccount.find(pkLD->GetAccountRef().login().c_str()) == m_map_kLogonAccount.end())
		delete pkLD;
	else
		pkLD->SetDeleted(true);
}

void CClientManager::QUERY_AUTH_LOGIN(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<GDAuthLoginPacket> p)
{
	if (g_test_server)
		sys_log(0, "QUERY_AUTH_LOGIN %d %d %s", p->account_id(), p->login_key(), p->login().c_str());
	CLoginData * pkLD = GetLoginDataByLogin(p->login().c_str());

	if (pkLD)
	{
		DeleteLoginData(pkLD);
	}

	network::DGOutputPacket<network::DGAuthLoginPacket> pdg;

	if (GetLoginData(p->login_key()))
	{
		sys_err("LoginData already exist key %u login %s", p->login_key(), p->login().c_str());

		pdg->set_result(false);
		pkPeer->Packet(pdg, dwHandle);
	}
	else
	{
#ifdef CHECK_IP_ON_CONNECT
		DGOutputPacket<DGWhitelistIPPacket> p2;
		p2->set_ip(p->ip());
		ForwardPacket(p2);
#endif

		CLoginData * pkLD = new CLoginData;

		DWORD client_key[4];
		for (int i = 0; i < 4; ++i)
			client_key[i] = p->client_keys(i);
		int premium_times[PREMIUM_MAX_NUM];
		for (int i = 0; i < PREMIUM_MAX_NUM; ++i)
			premium_times[i] = p->premium_times(i);

		pkLD->SetKey(p->login_key());
		pkLD->SetClientKey(client_key);
#ifdef __DEPRECATED_BILLING__
		pkLD->SetBillType(p->bill_type());
		pkLD->SetBillID(p->bill_id());
#endif
		pkLD->SetPremium(premium_times);

		TAccountTable & r = pkLD->GetAccountRef();

		r.set_id(p->account_id());
		char login_name[LOGIN_MAX_LEN + 1];
		trim_and_lower(p->login().c_str(), login_name, sizeof(login_name));
		r.set_login(login_name);
		r.set_social_id(p->social_id());
		r.set_passwd("TEMP");
		r.set_hwid(p->hwid());
		r.set_language(p->language());
#ifdef ACCOUNT_TRADE_BLOCK
		r.set_tradeblock(p->tradeblock());
		r.set_hwid2ban(p->hwid2ban());
		r.set_hwid2(p->hwid2());
#endif
		r.set_coins(p->coins());
		r.set_temp_login(p->temp_login());

		sys_log(0, "AUTH_LOGIN id(%u) login(%s) social_id(%s) login_key(%u), client_key(%u %u %u %u) is_temp(%u)",
			p->account_id(), p->login().c_str(), p->social_id().c_str(), p->login_key(),
			p->client_keys(0), p->client_keys(1), p->client_keys(2), p->client_keys(3),
			p->temp_login());

		InsertLoginData(pkLD);

		pdg->set_result(true);
		pkPeer->Packet(pdg, dwHandle);
	}
}

#ifdef __DEPRECATED_BILLING__
void CClientManager::BillingExpire(std::unique_ptr<GDBillingExpirePacket> p)
{
	network::DGOutputPacket<network::DGBillingExpirePacket> pdg;
	pdg->set_login(p->login());
	pdg->set_bill_type(p->bill_type());
	pdg->set_remain_seconds(p->remain_seconds());

	char key[LOGIN_MAX_LEN + 1];
	trim_and_lower(pdg->login().c_str(), key, sizeof(key));

	switch (pdg->bill_type())
	{
		case BILLING_IP_TIME:
		case BILLING_IP_DAY:
			{
				DWORD dwIPID = 0;
				str_to_number(dwIPID, pdg->login().c_str());

				TLogonAccountMap::iterator it = m_map_kLogonAccount.begin();

				while (it != m_map_kLogonAccount.end())
				{
					CLoginData * pkLD = (it++)->second;

					if (pkLD->GetBillID() == dwIPID)
					{
						CPeer * pkPeer = GetPeer(pkLD->GetConnectedPeerHandle());

						if (pkPeer)
						{
							pdg->set_login(pkLD->GetAccountRef().login());
							pkPeer->Packet(pdg);
						}
					}
				}
			}
			break;

		case BILLING_TIME:
		case BILLING_DAY:
			{
				TLogonAccountMap::iterator it = m_map_kLogonAccount.find(key);

				if (it != m_map_kLogonAccount.end())
				{
					CLoginData * pkLD = it->second;

					CPeer * pkPeer = GetPeer(pkLD->GetConnectedPeerHandle());

					if (pkPeer)
					{
						pkPeer->Packet(pdg);
					}
				}
			}
			break;
	}
}

void CClientManager::BillingCheck(std::unique_ptr<GDBillingCheckPacket> p)
{
	if (!m_pkAuthPeer)
		return;

	time_t curTime = GetCurrentTime();

	sys_log(0, "BillingCheck: size %u", p->keys_size());

	network::DGOutputPacket<network::DGBillingCheckPacket> pdg;
	for (DWORD dwKey : p->keys())
	{
		sys_log(0, "BillingCheck: %u", dwKey);

		TLoginDataByLoginKey::iterator it = m_map_pkLoginData.find(dwKey);

		if (it == m_map_pkLoginData.end())
		{
			sys_log(0, "BillingCheck: key not exist: %u", dwKey);
			pdg->add_keys(dwKey);
		}
		else
		{
			CLoginData * pkLD = it->second;

			if (!pkLD->IsPlay() && curTime - pkLD->GetLastPlayTime() > 180)
			{
				sys_log(0, "BillingCheck: not login: %u", dwKey);
				pdg->add_keys(dwKey);
			}
		}
	}

	m_pkAuthPeer->Packet(pdg);
}
#endif

void CClientManager::GuildDepositMoney(std::unique_ptr<GDGuildDepositMoneyPacket> p)
{
	CGuildManager::instance().DepositMoney(p->guild_id(), p->gold());
}

void CClientManager::GuildWithdrawMoney(CPeer* peer, std::unique_ptr<GDGuildWithdrawMoneyPacket> p)
{
	CGuildManager::instance().WithdrawMoney(peer, p->guild_id(), p->gold());
}

void CClientManager::GuildWithdrawMoneyGiveReply(std::unique_ptr<GDGuildWithdrawMoneyGiveReplyPacket> p)
{
	CGuildManager::instance().WithdrawMoneyReply(p->guild_id(), p->give_success(), p->change_gold());
}

void CClientManager::GuildWarBet(std::unique_ptr<GDGuildWarBetPacket> p)
{
	CGuildManager::instance().Bet(p->war_id(), p->login().c_str(), p->gold(), p->guild_id());
}

#ifdef __DEPRECATED_BILLING__
void CClientManager::SendAllLoginToBilling()
{
	if (!m_pkAuthPeer)
		return;

	network::DGOutputPacket<network::DGBillingRepairPacket> p;

	TLogonAccountMap::iterator it = m_map_kLogonAccount.begin();

	while (it != m_map_kLogonAccount.end())
	{
		CLoginData * pkLD = (it++)->second;

		p->add_login_keys(pkLD->GetKey());
		p->add_logins(pkLD->GetAccountRef().login());
		p->add_hosts(pkLD->GetIP());
		sys_log(0, "SendAllLoginToBilling %s %s", pkLD->GetAccountRef().login().c_str(), pkLD->GetIP());
	}

	if (p->login_keys_size() > 0)
		m_pkAuthPeer->Packet(p);
}

void CClientManager::SendLoginToBilling(CLoginData * pkLD, bool bLogin)
{
	if (!m_pkAuthPeer)
		return;

	network::DGOutputPacket<network::DGBillingLoginPacket> p;

	p->add_login_keys(pkLD->GetKey());
	p->add_logins(bLogin ? 1 : 0);

	m_pkAuthPeer->Packet(p);
}
#endif

void CClientManager::CreateObject(std::unique_ptr<GDCreateObjectPacket> p)
{
	using namespace building;

	char szQuery[512];

	snprintf(szQuery, sizeof(szQuery),
		"INSERT INTO object (land_id, vnum, map_index, x, y, x_rot, y_rot, z_rot) VALUES(%u, %u, %d, %d, %d, %f, %f, %f)",
		p->land_id(), p->vnum(), p->map_index(), p->x(), p->y(), p->rot_x(), p->rot_y(), p->rot_z());

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiInsertID == 0)
	{
		sys_err("cannot insert object");
		return;
	}

	auto pkObj = std::unique_ptr<TBuildingObject>(new TBuildingObject());

	pkObj->set_id(pmsg->Get()->uiInsertID);
	pkObj->set_vnum(p->vnum());
	pkObj->set_land_id(p->land_id());
	pkObj->set_map_index(p->map_index());
	pkObj->set_x(p->x());
	pkObj->set_y(p->y());
	pkObj->set_x_rot(p->rot_x());
	pkObj->set_y_rot(p->rot_y());
	pkObj->set_z_rot(p->rot_z());
	pkObj->set_life(0);

	DGOutputPacket<DGCreateObjectPacket> pack;
	*pack->mutable_object() = *pkObj;
	ForwardPacket(pack);

	m_map_pkObjectTable.insert(std::make_pair(pkObj->id(), std::move(pkObj)));
}

void CClientManager::DeleteObject(std::unique_ptr<GDDeleteObjectPacket> p)
{
	char szQuery[128];

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM object WHERE id=%u", p->id());

	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
	{
		sys_err("no object by id %u", p->id());
		return;
	}

	m_map_pkObjectTable.erase(p->id());

	DGOutputPacket<DGDeleteObjectPacket> pack;
	pack->set_id(p->id());
	ForwardPacket(pack);
}

void CClientManager::UpdateLand(std::unique_ptr<GDUpdateLandPacket> p)
{
	auto psaved = &m_vec_kLandTable[0];

	DWORD i;

	for (i = 0; i < m_vec_kLandTable.size(); ++i, ++psaved)
	{
		if (p->land_id() == psaved->id())
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "UPDATE land SET guild_id=%u WHERE id=%u", p->guild_id(), p->land_id());
			CDBManager::instance().AsyncQuery(buf);

			psaved->set_guild_id(p->guild_id());
			break;
		}
	}

	if (i < m_vec_kLandTable.size())
	{
		DGOutputPacket<DGUpdateLandPacket> pack;
		*pack->mutable_land() = *psaved;
		ForwardPacket(pack);
	}
}

// BLOCK_CHAT
void CClientManager::BlockChat(std::unique_ptr<GDBlockChatPacket> p)
{
	char szQuery[256];

	snprintf(szQuery, sizeof(szQuery), "SELECT id FROM player WHERE name = '%s'", p->name().c_str());
	std::auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));
	SQLResult * pRes = pmsg->Get();

	if (pRes->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);
		DWORD pid = strtoul(row[0], NULL, 10);

		auto pa = std::unique_ptr<GDAddAffectPacket>(new GDAddAffectPacket());
		pa->set_pid(pid);

		auto aff = pa->mutable_elem();
		aff->set_type(223);
		aff->set_duration(p->duration());
		QUERY_ADD_AFFECT(NULL, std::move(pa));
	}
	else
	{
		// cannot find user with that name
	}
}
// END_OF_BLOCK_CHAT

void CClientManager::MarriageAdd(std::unique_ptr<GDMarriageAddPacket> p)
{
	sys_log(0, "MarriageAdd %u %u %s %s", p->pid1(), p->pid2(), p->name1().c_str(), p->name2().c_str());
	marriage::CManager::instance().Add(p->pid1(), p->pid2(), p->name1().c_str(), p->name2().c_str());
}

void CClientManager::MarriageUpdate(std::unique_ptr<GDMarriageUpdatePacket> p)
{
	sys_log(0, "MarriageUpdate PID:%u %u LP:%d ST:%d", p->pid1(), p->pid2(), p->love_point(), p->married());
	marriage::CManager::instance().Update(p->pid1(), p->pid2(), p->love_point(), p->married());
}

void CClientManager::MarriageRemove(std::unique_ptr<GDMarriageRemovePacket> p)
{
	sys_log(0, "MarriageRemove %u %u", p->pid1(), p->pid2());
	marriage::CManager::instance().Remove(p->pid1(), p->pid2());
}

void CClientManager::WeddingRequest(std::unique_ptr<GDWeddingRequestPacket> p)
{
	sys_log(0, "WeddingRequest %u %u", p->pid1(), p->pid2());

	DGOutputPacket<DGWeddingRequestPacket> pack;
	pack->set_pid1(p->pid1());
	pack->set_pid2(p->pid2());
	ForwardPacket(pack);
	//marriage::CManager::instance().RegisterWedding(p->dwPID1, p->szName1, p->dwPID2, p->szName2);
}

void CClientManager::WeddingReady(std::unique_ptr<GDWeddingReadyPacket> p)
{
	sys_log(0, "WeddingReady %u %u", p->pid1(), p->pid2());
	DGOutputPacket<DGWeddingReadyPacket> pack;
	pack->set_pid1(p->pid1());
	pack->set_pid2(p->pid2());
	pack->set_map_index(p->map_index());
	ForwardPacket(pack);
	marriage::CManager::instance().ReadyWedding(p->map_index(), p->pid1(), p->pid2());
}

void CClientManager::WeddingEnd(std::unique_ptr<GDWeddingEndPacket> p)
{
	sys_log(0, "WeddingEnd %u %u", p->pid1(), p->pid2());
	marriage::CManager::instance().EndWedding(p->pid1(), p->pid2());
}

//
// 캐시에 가격정보가 있으면 캐시를 업데이트 하고 캐시에 가격정보가 없다면
// 우선 기존의 데이터를 로드한 뒤에 기존의 정보로 캐시를 만들고 새로 받은 가격정보를 업데이트 한다.
//
void CClientManager::MyshopPricelistUpdate(std::unique_ptr<GDMyShopPricelistUpdatePacket> pPacket)
{
	if (pPacket->price_info_size() > SHOP_PRICELIST_MAX_NUM)
	{
		sys_err("count overflow!");
		return;
	}

	CItemPriceListTableCache* pCache = GetItemPriceListCache(pPacket->owner_id());

	if (pCache)
	{
		TItemPriceListTable table;

		table.dwOwnerID = pPacket->owner_id();
		table.byCount = pPacket->price_info_size();

		for (int i = 0; i < pPacket->price_info_size(); ++i)
			table.aPriceInfo[i] = pPacket->price_info(i);

		pCache->UpdateList(&table);
	}
	else
	{
		TItemPriceListTable* pUpdateTable = new TItemPriceListTable;

		pUpdateTable->dwOwnerID = pPacket->owner_id();
		pUpdateTable->byCount = pPacket->price_info_size();

		for (int i = 0; i < pPacket->price_info_size(); ++i)
			pUpdateTable->aPriceInfo[i] = pPacket->price_info(i);

		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "SELECT item_vnum, price FROM myshop_pricelist WHERE owner_id=%u", pPacket->owner_id());
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_LOAD_FOR_UPDATE, 0, pUpdateTable);
	}
}

// MYSHOP_PRICE_LIST
// 캐시된 가격정보가 있으면 캐시를 읽어 바로 전송하고 캐시에 정보가 없으면 DB 에 쿼리를 한다.
//
void CClientManager::MyshopPricelistRequest(CPeer* peer, DWORD dwHandle, DWORD dwPlayerID)
{
	if (CItemPriceListTableCache* pCache = GetItemPriceListCache(dwPlayerID))
	{
		sys_log(0, "Cache MyShopPricelist handle[%d] pid[%d]", dwHandle, dwPlayerID);

		TItemPriceListTable* pTable = pCache->Get(false);

		DGOutputPacket<DGMyShopPricelistPacket> pack;
		for (auto i = 0; i < pTable->byCount; ++i)
			*pack->add_price_info() = pTable->aPriceInfo[i];

		peer->Packet(pack, dwHandle);

	}
	else
	{
		sys_log(0, "Query MyShopPricelist handle[%d] pid[%d]", dwHandle, dwPlayerID);

		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "SELECT item_vnum, price FROM myshop_pricelist WHERE owner_id=%u", dwPlayerID);
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_LOAD, peer->GetHandle(), new TItemPricelistReqInfo(dwHandle, dwPlayerID));
	}
}
// END_OF_MYSHOP_PRICE_LIST

void CPacketInfo::Add(int header)
{
	auto it = m_map_info.find(header);

	if (it == m_map_info.end())
		m_map_info.insert(std::map<int, int>::value_type(header, 1));
	else
		++it->second;
}

void CPacketInfo::Reset()
{
	m_map_info.clear();
}

void CClientManager::ProcessPackets(CPeer * peer)
{
	InputPacket packet;
	DWORD dwHandle;
	int i = 0;
	int iCount = 0;

	while (peer->PeekPacket(i, packet, dwHandle))
	{
		// DISABLE_DB_HEADER_LOG
		// sys_log(0, "header %d %p size %d", header, this, dwLength);
		// END_OF_DISABLE_DB_HEADER_LOG
		m_bLastHeader = packet.get_header();
		++iCount;

#ifdef _TEST	
		if (header != 10)
			sys_log(0, " ProcessPacket Header [%d] Handle[%d] Length[%d] iCount[%d]", header, dwHandle, dwLength, iCount);
#endif
//		if (g_test_server)
		{
//			if (packet.get_header() != 10)
//				sys_log(0, " ProcessPacket Header [%d] Handle[%d] Length[%d] iCount[%d]", packet.get_header(), dwHandle, packet.header.size, iCount);
		}

		switch (static_cast<TGDHeader>(packet.get_header()))
		{
			case TGDHeader::BOOT:
				QUERY_BOOT(peer, packet.get<GDBootPacket>());
				break;

			case TGDHeader::HAMMER_OF_TOR:
				break;

			case TGDHeader::LOGIN_BY_KEY:
				QUERY_LOGIN_BY_KEY(peer, dwHandle, packet.get<GDLoginByKeyPacket>());
				break;

			case TGDHeader::PLAYER_LOAD:
			{
					sys_log(1, "HEADER_GD_PLAYER_LOAD (handle: %d)", dwHandle);
					auto pack = packet.get<GDPlayerLoadPacket>();
					QUERY_PLAYER_LOAD(peer, dwHandle, pack.get());
#ifdef CHANGE_SKILL_COLOR
					QUERY_SKILL_COLOR_LOAD(peer, dwHandle, pack.get());
#endif
#ifdef __EQUIPMENT_CHANGER__
					QUERY_EQUIPMENT_CHANGER_LOAD(peer, dwHandle, pack.get());
#endif
				}
				break;

			case TGDHeader::PLAYER_SAVE:
				sys_log(1, "HEADER_GD_PLAYER_SAVE (handle: %d)", dwHandle);
				QUERY_PLAYER_SAVE(peer, dwHandle, packet.get<GDPlayerSavePacket>());
				break;

#ifdef ENABLE_RUNE_SYSTEM
			case TGDHeader::PLAYER_RUNE_SAVE:
				QUERY_PLAYER_RUNE_SAVE(peer, dwHandle, packet.get<GDPlayerRuneSavePacket>());
				break;
#endif

			case TGDHeader::PLAYER_CREATE:
				sys_log(0, "HEADER_GD_PLAYER_CREATE (handle: %d)", dwHandle);
				__QUERY_PLAYER_CREATE(peer, dwHandle, packet.get<GDPlayerCreatePacket>());
				sys_log(0, "END");
				break;

			case TGDHeader::PLAYER_DELETE:
				sys_log(1, "HEADER_GD_PLAYER_DELETE (handle: %d)", dwHandle);
				__QUERY_PLAYER_DELETE(peer, dwHandle, packet.get<GDPlayerDeletePacket>());
				break;

			case TGDHeader::LOAD_ITEM_REFUND:
				QUERY_ITEM_REFUND(peer, dwHandle, packet.get<GDLoadItemRefundPacket>()->pid());
				break;

#ifdef __HAIR_SELECTOR__
			case TGDHeader::SELECT_UPDATE_HAIR:
				QUERY_SELECT_UPDATE_HAIR(peer, packet.get<GDSelectUpdateHairPacket>());
				break;
#endif

			case TGDHeader::QUEST_SAVE:
				sys_log(1, "HEADER_GD_QUEST_SAVE (handle: %d)", dwHandle);
				QUERY_QUEST_SAVE(peer, packet.get<GDQuestSavePacket>());
				break;

			case TGDHeader::SAFEBOX_LOAD:
				QUERY_SAFEBOX_LOAD(peer, dwHandle, packet.get<GDSafeboxLoadPacket>());
				break;

			case TGDHeader::SAFEBOX_SAVE:
				sys_log(1, "HEADER_GD_SAFEBOX_SAVE (handle: %d)", dwHandle);
				QUERY_SAFEBOX_SAVE(peer, packet.get<GDSafeboxSavePacket>());
				break;

			case TGDHeader::SAFEBOX_CHANGE_SIZE:
				QUERY_SAFEBOX_CHANGE_SIZE(peer, dwHandle, packet.get<GDSafeboxChangeSizePacket>());
				break;

			case TGDHeader::SAFEBOX_CHANGE_PASSWORD:
				QUERY_SAFEBOX_CHANGE_PASSWORD(peer, dwHandle, packet.get<GDSafeboxChangePasswordPacket>());
				break;

			case TGDHeader::EMPIRE_SELECT:
				QUERY_EMPIRE_SELECT(peer, dwHandle, packet.get<GDEmpireSelectPacket>());
				break;

			case TGDHeader::SETUP:
				QUERY_SETUP(peer, dwHandle, packet.get<GDSetupPacket>());
				break;

			case TGDHeader::GUILD_CREATE:
				GuildCreate(peer, packet.get<GDGuildCreatePacket>());
				break;

			case TGDHeader::GUILD_SKILL_UPDATE:
				GuildSkillUpdate(peer, packet.get<GDGuildSkillUpdatePacket>());
				break;

			case TGDHeader::GUILD_EXP_UPDATE:
				GuildExpUpdate(peer, packet.get<GDGuildExpUpdatePacket>());
				break;

			case TGDHeader::GUILD_ADD_MEMBER:
				GuildAddMember(peer, packet.get<GDGuildAddMemberPacket>());
				break;

			case TGDHeader::GUILD_REMOVE_MEMBER:
				GuildRemoveMember(peer, packet.get<GDGuildRemoveMemberPacket>());
				break;

			case TGDHeader::GUILD_CHANGE_GRADE:
				GuildChangeGrade(peer, packet.get<GDGuildChangeGradePacket>());
				break;

			case TGDHeader::GUILD_CHANGE_MEMBER_DATA:
				GuildChangeMemberData(peer, packet.get<GDGuildChangeMemberDataPacket>());
				break;

			case TGDHeader::GUILD_DISBAND:
				GuildDisband(peer, packet.get<GDGuildDisbandPacket>());
				break;

			case TGDHeader::GUILD_WAR:
				GuildWar(peer, packet.get<GDGuildWarPacket>());
				break;

			case TGDHeader::GUILD_WAR_SCORE:
				GuildWarScore(peer, packet.get<GDGuildWarScorePacket>());
				break;

			case TGDHeader::GUILD_CHANGE_LADDER_POINT:
				GuildChangeLadderPoint(packet.get<GDGuildChangeLadderPointPacket>());
				break;

			case TGDHeader::GUILD_USE_SKILL:
				GuildUseSkill(packet.get<GDGuildUseSkillPacket>());
				break;

			case TGDHeader::FLUSH_CACHE:
				QUERY_FLUSH_CACHE(peer, packet.get<GDFlushCachePacket>());
				break;

			case TGDHeader::ITEM_SAVE:
				QUERY_ITEM_SAVE(peer, packet.get<GDItemSavePacket>());
				break;

			case TGDHeader::ITEM_DESTROY:
				QUERY_ITEM_DESTROY(peer, packet.get<GDItemDestroyPacket>());
				break;

			case TGDHeader::ITEM_FLUSH:
				QUERY_ITEM_FLUSH(peer, packet.get<GDItemFlushPacket>());
				break;

#ifdef __PET_ADVANCED__
			case TGDHeader::PET_SAVE:
				QUERY_PET_SAVE(peer, packet.get<GDPetSavePacket>());
				break;
#endif

			case TGDHeader::LOGOUT:
				QUERY_LOGOUT(peer, dwHandle, packet.get<GDLogoutPacket>());
				break;

			case TGDHeader::ADD_AFFECT:
				sys_log(1, "HEADER_GD_ADD_AFFECT");
				QUERY_ADD_AFFECT(peer, packet.get<GDAddAffectPacket>());
				break;

			case TGDHeader::REMOVE_AFFECT:
				sys_log(1, "HEADER_GD_REMOVE_AFFECT");
				QUERY_REMOVE_AFFECT(peer, packet.get<GDRemoveAffectPacket>());
				break;

			case TGDHeader::PARTY_CREATE:
				QUERY_PARTY_CREATE(peer, packet.get<GDPartyCreatePacket>());
				break;

			case TGDHeader::PARTY_DELETE:
				QUERY_PARTY_DELETE(peer, packet.get<GDPartyDeletePacket>());
				break;

			case TGDHeader::PARTY_ADD:
				QUERY_PARTY_ADD(peer, packet.get<GDPartyAddPacket>());
				break;

			case TGDHeader::PARTY_REMOVE:
				QUERY_PARTY_REMOVE(peer, packet.get<GDPartyRemovePacket>());
				break;

			case TGDHeader::PARTY_STATE_CHANGE:
				QUERY_PARTY_STATE_CHANGE(peer, packet.get<GDPartyStateChangePacket>());
				break;

			case TGDHeader::PARTY_SET_MEMBER_LEVEL:
				QUERY_PARTY_SET_MEMBER_LEVEL(peer, packet.get<GDPartySetMemberLevelPacket>());
				break;

			case TGDHeader::RELOAD_PROTO:
				QUERY_RELOAD_PROTO();
				break;
				
			case TGDHeader::RELOAD_MOB_PROTO:
				QUERY_RELOAD_MOB_PROTO();
				break;

			case TGDHeader::RELOAD_SHOP:
				ReloadShopProto();
				break;

			case TGDHeader::CHANGE_NAME:
				QUERY_CHANGE_NAME(peer, dwHandle, packet.get<GDChangeNamePacket>());
				break;

			case TGDHeader::AUTH_LOGIN:
				QUERY_AUTH_LOGIN(peer, dwHandle, packet.get<GDAuthLoginPacket>());
				break;

			case TGDHeader::REQUEST_GUILD_PRIV:
				AddGuildPriv(packet.get<GDRequestGuildPrivPacket>());
				break;

			case TGDHeader::REQUEST_EMPIRE_PRIV:
				AddEmpirePriv(packet.get<GDRequestEmpirePrivPacket>());
				break;

			case TGDHeader::REQUEST_CHARACTER_PRIV:
				AddCharacterPriv(packet.get<GDRequestCharacterPrivPacket>());
				break;

			case TGDHeader::GUILD_DEPOSIT_MONEY:
				GuildDepositMoney(packet.get<GDGuildDepositMoneyPacket>());
				break;

			case TGDHeader::GUILD_WITHDRAW_MONEY:
				GuildWithdrawMoney(peer, packet.get<GDGuildWithdrawMoneyPacket>());
				break;

			case TGDHeader::GUILD_WITHDRAW_MONEY_GIVE_REPLY:
				GuildWithdrawMoneyGiveReply(packet.get<GDGuildWithdrawMoneyGiveReplyPacket>());
				break;

			case TGDHeader::GUILD_WAR_BET:
				GuildWarBet(packet.get<GDGuildWarBetPacket>());
				break;

			case TGDHeader::SET_EVENT_FLAG:
				SetEventFlag(packet.get<GDSetEventFlagPacket>());
				break;

#ifdef __DEPRECATED_BILLING__
			case TGDHeader::BILLING_EXPIRE:
				BillingExpire(packet.get<GDBillingExpirePacket>());
				break;

			case TGDHeader::BILLING_CHECK:
				BillingCheck(packet.get<GDBillingCheckPacket>());
				break;
#endif

			case TGDHeader::CREATE_OBJECT:
				CreateObject(packet.get<GDCreateObjectPacket>());
				break;

			case TGDHeader::DELETE_OBJECT:
				DeleteObject(packet.get<GDDeleteObjectPacket>());
				break;

			case TGDHeader::UPDATE_LAND:
				UpdateLand(packet.get<GDUpdateLandPacket>());
				break;

			case TGDHeader::MARRIAGE_ADD:
				MarriageAdd(packet.get<GDMarriageAddPacket>());
				break;

			case TGDHeader::MARRIAGE_UPDATE:
				MarriageUpdate(packet.get<GDMarriageUpdatePacket>());
				break;

			case TGDHeader::MARRIAGE_REMOVE:
				MarriageRemove(packet.get<GDMarriageRemovePacket>());
				break;

			case TGDHeader::WEDDING_REQUEST:
				WeddingRequest(packet.get<GDWeddingRequestPacket>());
				break;

			case TGDHeader::WEDDING_READY:
				WeddingReady(packet.get<GDWeddingReadyPacket>());
				break;

			case TGDHeader::WEDDING_END:
				WeddingEnd(packet.get<GDWeddingEndPacket>());
				break;

				// BLOCK_CHAT
			case TGDHeader::BLOCK_CHAT:
				BlockChat(packet.get<GDBlockChatPacket>());
				break;
				// END_OF_BLOCK_CHAT

				// MYSHOP_PRICE_LIST
			case TGDHeader::MYSHOP_PRICELIST_UPDATE:
				MyshopPricelistUpdate(packet.get<GDMyShopPricelistUpdatePacket>()); // @fixme403 (TPacketMyshopPricelistHeader to TItemPriceListTable)
				break;

			case TGDHeader::MYSHOP_PRICELIST_REQUEST:
				MyshopPricelistRequest(peer, dwHandle, packet.get<GDMyShopPricelistRequestPacket>()->pid());
				break;
				// END_OF_MYSHOP_PRICE_LIST

				//RELOAD_ADMIN
			case TGDHeader::RELOAD_ADMIN:
				ReloadAdmin();
				break;
				//END_RELOAD_ADMIN

			case TGDHeader::MARRIAGE_BREAK:
				BreakMarriage(peer, packet.get<GDMarriageBreakPacket>());
				break;

			case TGDHeader::REQ_SPARE_ITEM_ID_RANGE:
				SendSpareItemIDRange(peer);
				break;

			case TGDHeader::GUILD_REQ_CHANGE_MASTER:
				GuildChangeMaster(packet.get<GDGuildReqChangeMasterPacket>());
				break;

			case TGDHeader::DISCONNECT:
				DeleteLoginKey(packet.get<GDDisconnectPacket>());
				break;

			case TGDHeader::VALID_LOGOUT:
				ResetLastPlayerID(packet.get<GDValidLogoutPacket>());
				break;

#ifdef __DUNGEON_FOR_GUILD__
			case TGDHeader::GUILD_DUNGEON:
				GuildDungeon(packet.get<GDGuildDungeonPacket>());
				break;
			case TGDHeader::GUILD_DUNGEON_CD:
				GuildDungeonGD(packet.get<GDGuildDungeonCDPacket>());
				break;
#endif

			case TGDHeader::FORCE_ITEM_DELETE:
				ForceItemDelete(packet.get<GDForceItemDeletePacket>()->id());
				break;

#ifdef __MAINTENANCE_DIFFERENT_SERVER__
			case HEADER_GD_ON_SHUTDOWN_SERVER:
				if (!g_test_server)
					system("touch /game/auto_restart &");
				break;
#endif

#ifdef __GUILD_SAFEBOX__
			case TGDHeader::GUILD_SAFEBOX_ADD:
			case TGDHeader::GUILD_SAFEBOX_MOVE:
			case TGDHeader::GUILD_SAFEBOX_TAKE:
			case TGDHeader::GUILD_SAFEBOX_GIVE_GOLD:
			case TGDHeader::GUILD_SAFEBOX_GET_GOLD:
			case TGDHeader::GUILD_SAFEBOX_CREATE:
			case TGDHeader::GUILD_SAFEBOX_SIZE:
			case TGDHeader::GUILD_SAFEBOX_LOAD:
				CGuildSafeboxManager::Instance().ProcessPacket(peer, dwHandle, packet);
				break;
#endif

			case TGDHeader::WHISPER_PLAYER_EXIST_CHECK:
				WhisperPlayerExistCheck(peer, dwHandle, packet.get<GDWhisperPlayerExistCheckPacket>());
				break;

			case TGDHeader::WHISPER_PLAYER_MESSAGE_OFFLINE:
				WhisperPlayerMessageOffline(peer, dwHandle, packet.get<GDWhisperPlayerMessageOfflinePacket>());
				break;

			case TGDHeader::ITEM_DESTROY_LOG:
				QUERY_ITEM_DESTROY_LOG(packet.get<GDItemDestroyLogPacket>());
				break;

#ifdef COMBAT_ZONE
			case TGDHeader::COMBAT_ZONE_SKILLS_CACHE:
				UpdateSkillsCache(packet.get<GDCombatZoneSkillsCachePacket>());
				break;
#endif

#ifdef __MAINTENANCE__
			case TGDHeader::RECV_SHUTDOWN:
				if (g_test_server)	sys_err("%s:%d Maintenance", __FILE__, __LINE__);
				Maintenance(packet.get<GDRecvShutdownPacket>());
				break;
#endif

			case TGDHeader::ITEM_TIMED_IGNORE:
				RecvItemTimedIgnore(packet.get<GDItemTimedIgnorePacket>());
				break;

#ifdef ENABLE_XMAS_EVENT
			case TGDHeader::RELOAD_XMAS:
				ReloadXmasRewards();
				break;
#endif

#ifdef CHANGE_SKILL_COLOR
			case TGDHeader::SKILL_COLOR_SAVE:
				QUERY_SKILL_COLOR_SAVE(packet.get<GDSkillColorSavePacket>());
				break;
#endif

#ifdef __EQUIPMENT_CHANGER__
			case TGDHeader::EQUIPMENT_PAGE_SAVE:
				QUERY_EQUIPMENT_CHANGER_SAVE(peer, packet.get<GDEquipmentPageSavePacket>());
				break;

			case TGDHeader::EQUIPMENT_PAGE_DELETE:
				QUERY_EQUIPMENT_CHANGER_DELETE(peer, packet.get<GDEquipmentPageDeletePacket>());
				break;
#endif

			default:
				sys_err("Unknown header [%s:%d] (header: %d handle: %d length: %d)",
					peer->GetHost(), peer->GetListenPort(), packet.get_header(), dwHandle, packet.content_size);
				peer->RecvEnd(i);
				RemovePeer(peer);
				return;
				break;
		}
	}

	peer->RecvEnd(i);
}

void CClientManager::AddPeer(socket_t fd)
{
	CPeer * pPeer = new CPeer;

	if (pPeer->Accept(fd))
		m_peerList.push_front(pPeer);
	else
		delete pPeer;
}

void CClientManager::RemovePeer(CPeer * pPeer)
{
	std::vector<DWORD> vecLogonPID;

	if (m_pkAuthPeer == pPeer)
	{
		m_pkAuthPeer = NULL;
	}
	else
	{
#ifdef __GUILD_SAFEBOX__
		CGuildSafeboxManager::instance().DisconnectPeer(pPeer);
#endif

		TLogonAccountMap::iterator it = m_map_kLogonAccount.begin();

		while (it != m_map_kLogonAccount.end())
		{
			CLoginData * pkLD = it->second;

			if (pkLD->GetConnectedPeerHandle() == pPeer->GetHandle())
			{
				for (const TSimplePlayer& c_rkPlayer : pkLD->GetAccountRef().players())
				{
					if (c_rkPlayer.id())
						vecLogonPID.push_back(c_rkPlayer.id());
				}

				if (pkLD->IsPlay())
				{
					pkLD->SetPlay(false);
#ifdef __DEPRECATED_BILLING__
					SendLoginToBilling(pkLD, false);
#endif
				}

				if (pkLD->IsDeleted())
				{
					sys_log(0, "DELETING LoginData");
					delete pkLD;
				}

				m_map_kLogonAccount.erase(it++);
			}
			else
				++it;
		}
	}

	pPeer->SetLogonPIDVector(&vecLogonPID); // to recover item ID tables

	m_peerList.remove(pPeer);
	delete pPeer;
}

CPeer * CClientManager::GetPeer(IDENT ident)
{
	for (auto i = m_peerList.begin(); i != m_peerList.end();++i)
	{
		CPeer * tmp = *i;

		if (tmp->GetHandle() == ident)
			return tmp;
	}

	return NULL;
}

CPeer * CClientManager::GetAnyPeer()
{
	if (m_peerList.empty())
		return NULL;

	return m_peerList.front();
}

// DB 매니저로 부터 받은 결과를 처리한다.
//
// @version	05/06/10 Bang2ni - 가격정보 관련 쿼리(QID_ITEMPRICE_XXX) 추가
int CClientManager::AnalyzeQueryResult(SQLMsg * msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	CPeer * peer = GetPeer(qi->dwIdent);

#ifdef _TEST
	if (qi->iType != QID_ITEM_AWARD_LOAD)
	sys_log(0, "AnalyzeQueryResult %d", qi->iType);
#endif
	switch (qi->iType)
	{
		case QID_ITEM_AWARD_LOAD:
			ItemAwardManager::instance().Load(msg);
			delete qi;
			return true;

		case QID_GUILD_RANKING:
			CGuildManager::instance().ResultRanking(msg->Get()->pSQLResult);
			break;

			// MYSHOP_PRICE_LIST
		case QID_ITEMPRICE_LOAD_FOR_UPDATE:
			RESULT_PRICELIST_LOAD_FOR_UPDATE(msg);
			break;
			// END_OF_MYSHOP_PRICE_LIST

#ifdef __GUILD_SAFEBOX__
		case QID_GUILD_SAFEBOX_ITEM_LOAD:
			CGuildSafeboxManager::Instance().QueryResult(peer, msg, qi->iType);
			break;
#endif
	}

	if (!peer)
	{	
		//sys_err("CClientManager::AnalyzeQueryResult: peer not exist anymore. (ident: %d)", qi->dwIdent);
		delete qi;
		return true;
	}

	switch (qi->iType)
	{
		case QID_PLAYER:
		case QID_ITEM:
		case QID_QUEST:
		case QID_AFFECT:
		case QID_OFFLINE_MESSAGES:
#ifdef __ITEM_REFUND__
		case QID_ITEM_REFUND:
#endif
#ifdef CHANGE_SKILL_COLOR
		case QID_SKILL_COLOR:
#endif
#ifdef __EQUIPMENT_CHANGER__
		case QID_EQUIPMENT_PAGE:
#endif
			RESULT_COMPOSITE_PLAYER(peer, msg, qi->iType);
			break;

#ifdef __PET_ADVANCED__
		case QID_PET_ADVANCED_LOAD:
			RESULT_PET_LOAD(peer, msg);
			break;

		case QID_PET_ADVANCED_LOAD_SKILL:
			RESULT_PET_SKILL_LOAD(peer, msg);
			break;
#endif

		case QID_LOGIN:
			sys_log(!g_test_server, "QUERY_RESULT: QID_LOGIN");
			RESULT_LOGIN(peer, msg);
			break;

		case QID_SAFEBOX_LOAD:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_LOAD");
			RESULT_SAFEBOX_LOAD(peer, msg);
			break;

		case QID_SAFEBOX_CHANGE_SIZE:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_SIZE");
			RESULT_SAFEBOX_CHANGE_SIZE(peer, msg);
			break;

		case QID_SAFEBOX_CHANGE_PASSWORD:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_PASSWORD %p", msg);
			RESULT_SAFEBOX_CHANGE_PASSWORD(peer, msg);
			break;

		case QID_SAFEBOX_CHANGE_PASSWORD_SECOND:
			sys_log(0, "QUERY_RESULT: HEADER_GD_SAFEBOX_CHANGE_PASSWORD %p", msg);
			RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(peer, msg);
			break;

		case QID_SAFEBOX_SAVE:
		case QID_ITEM_SAVE:
		case QID_ITEM_DESTROY:
		case QID_QUEST_SAVE:
		case QID_PLAYER_SAVE:
		case QID_ITEM_AWARD_TAKEN:
#ifdef CHANGE_SKILL_COLOR
		case QID_SKILL_COLOR_SAVE:
#endif
			break;

			// PLAYER_INDEX_CREATE_BUG_FIX	
		case QID_PLAYER_INDEX_CREATE:
			RESULT_PLAYER_INDEX_CREATE(peer, msg);
			break;
			// END_PLAYER_INDEX_CREATE_BUG_FIX	

		case QID_PLAYER_DELETE:
			__RESULT_PLAYER_DELETE(peer, msg);
			break;

		case QID_LOGIN_BY_KEY:
			RESULT_LOGIN_BY_KEY(peer, msg);
			break;

			// MYSHOP_PRICE_LIST
		case QID_ITEMPRICE_LOAD:
			RESULT_PRICELIST_LOAD(peer, msg);
			break;
			// END_OF_MYSHOP_PRICE_LIST

		case QID_WHISPER_PLAYER_EXIST_CHECK:
			RESULT_WhisperPlayerExistCheck(peer, msg, qi);
			break;

		default:
			sys_log(0, "CClientManager::AnalyzeQueryResult unknown query result type: %d, str: %s", qi->iType, msg->stQuery.c_str());
			break;
	}

	delete qi;
	return true;
}

int CClientManager::Process()
{
	int pulses;

	if (!(pulses = thecore_idle()))
		return 0;

	while (pulses--)
	{
		++thecore_heart->pulse;

		if (!(thecore_heart->pulse % thecore_heart->passes_per_sec))
		{
			if (g_test_server)
			{
			
				if (!(thecore_heart->pulse % thecore_heart->passes_per_sec * 10))	
					
				{
					pt_log("[%9d] return %d/%d/%d/%d async %d/%d/%d/%d",
							thecore_heart->pulse,
							CDBManager::instance().CountReturnQuery(SQL_PLAYER),
							CDBManager::instance().CountReturnResult(SQL_PLAYER),
							CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
							CDBManager::instance().CountReturnCopiedQuery(SQL_PLAYER),
							CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
							CDBManager::instance().CountAsyncResult(SQL_PLAYER),
							CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER),
							CDBManager::instance().CountAsyncCopiedQuery(SQL_PLAYER));

					if ((thecore_heart->pulse % 50) == 0) 
						sys_log(0, "[%9d] return %d/%d/%d async %d/%d/%d",
								thecore_heart->pulse,
								CDBManager::instance().CountReturnQuery(SQL_PLAYER),
								CDBManager::instance().CountReturnResult(SQL_PLAYER),
								CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
								CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
								CDBManager::instance().CountAsyncResult(SQL_PLAYER),
								CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER));
				}
			}
			else
			{
				pt_log("[%9d] return %d/%d/%d/%d async %d/%d/%d%/%d",
						thecore_heart->pulse,
						CDBManager::instance().CountReturnQuery(SQL_PLAYER),
						CDBManager::instance().CountReturnResult(SQL_PLAYER),
						CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
						CDBManager::instance().CountReturnCopiedQuery(SQL_PLAYER),
						CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
						CDBManager::instance().CountAsyncResult(SQL_PLAYER),
						CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER),
						CDBManager::instance().CountAsyncCopiedQuery(SQL_PLAYER));

						if ((thecore_heart->pulse % 50) == 0) 
						sys_log(0, "[%9d] return %d/%d/%d async %d/%d/%d",
							thecore_heart->pulse,
							CDBManager::instance().CountReturnQuery(SQL_PLAYER),
							CDBManager::instance().CountReturnResult(SQL_PLAYER),
							CDBManager::instance().CountReturnQueryFinished(SQL_PLAYER),
							CDBManager::instance().CountAsyncQuery(SQL_PLAYER),
							CDBManager::instance().CountAsyncResult(SQL_PLAYER),
							CDBManager::instance().CountAsyncQueryFinished(SQL_PLAYER));
			}
			
			CDBManager::instance().ResetCounter();

			m_iCacheFlushCount = 0;

#ifdef CHANGE_SKILL_COLOR
			UpdateSkillColorCache();
#endif
			//플레이어 플러쉬
			UpdatePlayerCache();
			//로그아웃시 처리- 캐쉬셋 플러쉬
			UpdateLogoutPlayer();

			// MYSHOP_PRICE_LIST
			UpdateItemPriceListCache();
			// END_OF_MYSHOP_PRICE_LIST

			CGuildManager::instance().Update();
			CPrivManager::instance().Update();
			marriage::CManager::instance().Update();
		}

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 5 + 2)))
		{
#ifdef __GUILD_SAFEBOX__
			CGuildSafeboxManager::Instance().Update(m_iCacheFlushCount, m_iCacheFlushCountLimit);
#endif
		}

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * (g_test_server ? 50 : 5))))
		{
			ItemAwardManager::instance().RequestLoad();
		}

#ifdef __CHECK_P2P_BROKEN__

		if (!( thecore_heart->pulse % ( thecore_heart->passes_per_sec * 10 ) ))
		{
			DWORD dwCurrentPeerAmount = GetCurrentPeerAmount();
			for_each_peer(FSendCheckP2P(dwCurrentPeerAmount));
			sys_log(0, "Sent check P2P packet, peer amount: %d", dwCurrentPeerAmount);
		}
#endif

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 10)))
		{
			pt_log("QUERY: MAIN[%d] ASYNC[%d]", g_query_count[0], g_query_count[1]);
			g_query_count[0] = 0;
			g_query_count[1] = 0;
		}

		if (!(thecore_heart->pulse % (thecore_heart->passes_per_sec * 60)))
		{
			CClientManager::instance().SendTime();
		}
	}

	int num_events = fdwatch(m_fdWatcher, 0);
	int idx;
	CPeer * peer;

	for (idx = 0; idx < num_events; ++idx) // 인풋
	{
		peer = (CPeer *) fdwatch_get_client_data(m_fdWatcher, idx);

		if (!peer)
		{
			if (fdwatch_check_event(m_fdWatcher, m_fdAccept, idx) == FDW_READ)
			{
				AddPeer(m_fdAccept);
				fdwatch_clear_event(m_fdWatcher, m_fdAccept, idx);
			}
			else
			{
				sys_err("FDWATCH: peer null in event: ident %d", fdwatch_get_ident(m_fdWatcher, idx));
			}

			continue;
		}

		switch (fdwatch_check_event(m_fdWatcher, peer->GetFd(), idx))
		{
			case FDW_READ:
				if (peer->Recv() < 0)
				{
					sys_err("Recv failed");
					RemovePeer(peer);
				}
				else
				{
					if (peer == m_pkAuthPeer)
						if (g_log)
							sys_log(0, "AUTH_PEER_READ: size %d", peer->GetRecvLength());

					ProcessPackets(peer);
				}
				break;

			case FDW_WRITE:
				if (peer == m_pkAuthPeer)
					if (g_log)
						sys_log(0, "AUTH_PEER_WRITE: size %d", peer->GetSendLength());

				if (peer->Send() < 0)
				{
					sys_err("Send failed");
					RemovePeer(peer);
				}

				break;

			case FDW_EOF:
				RemovePeer(peer);
				break;

			default:
				sys_err("fdwatch_check_fd returned unknown result");
				RemovePeer(peer);
				break;
		}
	}

#ifdef __WIN32__
	if (_kbhit()) {
		int c = _getch();
		switch (c) {
			case 0x1b: // Esc
				return 0; // shutdown
				break;
			default:
				break;
		}
	}
#endif

	return 1;
}

void CClientManager::SendAllGuildSkillRechargePacket()
{
	ForwardPacket(TDGHeader::GUILD_SKILL_RECHARGE);
}

void CClientManager::SendTime()
{
	time_t now = GetCurrentTime();
	network::DGOutputPacket<network::DGTimePacket> p;
	p->set_time(now);
	ForwardPacket(p);
}

void CClientManager::SendNotice(const char * c_pszFormat, ...)
{
	char szBuf[255+1];
	va_list args;

	va_start(args, c_pszFormat);
	int len = vsnprintf(szBuf, sizeof(szBuf), c_pszFormat, args);
	va_end(args);
	szBuf[len] = '\0';

	network::DGOutputPacket<network::DGNoticePacket> pdg;
	pdg->set_notice(szBuf);

	ForwardPacket(pdg);
}

time_t CClientManager::GetCurrentTime()
{
	return time(0);
}

// ITEM_UNIQUE_ID
bool CClientManager::InitializeNowItemID()
{
	DWORD dwMin, dwMax;

	//아이템 ID를 초기화 한다.
	if (!CConfig::instance().GetTwoValue("ITEM_ID_RANGE", &dwMin, &dwMax))
	{
		sys_err("conf.txt: Cannot find ITEM_ID_RANGE [start_item_id] [end_item_id]");
		return false;
	}

	sys_log(0, "ItemRange From File %u ~ %u ", dwMin, dwMax);
	
	if (CItemIDRangeManager::instance().BuildRange(dwMin, dwMax, m_itemRange) == false)
	{
		sys_err("Can not build ITEM_ID_RANGE");
		return false;
	}
	
	sys_log(0, " Init Success Start %u End %u Now %u\n", m_itemRange.min_id(), m_itemRange.max_id(), m_itemRange.usable_item_id_min());

	return true;
}

DWORD CClientManager::GainItemID()
{
	DWORD id = m_itemRange.usable_item_id_min();
	m_itemRange.set_usable_item_id_min(id + 1);
	return id;
}

DWORD CClientManager::GetItemID()
{
	return m_itemRange.usable_item_id_min();
}
// ITEM_UNIQUE_ID_END

//ADMIN_MANAGER

bool CClientManager::__GetAdminInfo(google::protobuf::RepeatedPtrField<network::TAdminInfo>* admins)
{
	//szIP == NULL 일경우  모든서버에 운영자 권한을 갖는다.
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery),
			"SELECT mID,mAccount,mName,mAuthority FROM gmlist");

	SQLMsg * pMsg = CDBManager::instance().DirectQuery(szQuery, SQL_COMMON);

	if (pMsg->Get()->uiNumRows == 0)
	{
		sys_err("__GetAdminInfo() ==> DirectQuery failed(%s)", szQuery);
		delete pMsg;
		return false;
	}

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		int idx = 0;
		auto Info = admins->Add();

		Info->set_id(std::stoi(row[idx++]));
		Info->set_account(row[idx++]);
		Info->set_name(row[idx++]);

		std::string stAuth = row[idx++];

		if (!stAuth.compare("IMPLEMENTOR"))
			Info->set_authority(GM_IMPLEMENTOR);
		else if (!stAuth.compare("GOD"))
			Info->set_authority(GM_GOD);
		else if (!stAuth.compare("HIGH_WIZARD"))
			Info->set_authority(GM_HIGH_WIZARD);
		else if (!stAuth.compare("LOW_WIZARD"))
			Info->set_authority(GM_LOW_WIZARD);
		else if (!stAuth.compare("WIZARD"))
			Info->set_authority(GM_WIZARD);
		else 
			continue;

		sys_log(0, "GM: PID %u Login %s Character %s Authority %d[%s]",
			   	Info->id(), Info->account().c_str(), Info->name().c_str(), Info->authority(), stAuth.c_str());
	}

	delete pMsg;

	return true;
}

bool CClientManager::__GetAdminConfig(google::protobuf::RepeatedField<google::protobuf::uint32>* adminConfigs)
{
	adminConfigs->Clear();
	for (int i = 0; i < GM_MAX_NUM; ++i)
		adminConfigs->Add();

	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery),
		"SELECT authority, general_allow+0 FROM gmconfig");

	SQLMsg * pMsg = CDBManager::instance().DirectQuery(szQuery, SQL_COMMON);

	if (pMsg->Get()->uiNumRows == 0)
	{
		sys_err("__GetAdminConfig() ==> DirectQuery failed(%s)", szQuery);
		delete pMsg;
		return false;
	}

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
	{
		int col = 0;

		BYTE bAuthority = GM_MAX_NUM;
		std::string stAuthority = row[col++];
		if (!stAuthority.compare("IMPLEMENTOR"))
			bAuthority = GM_IMPLEMENTOR;
		else if (!stAuthority.compare("GOD"))
			bAuthority = GM_GOD;
		else if (!stAuthority.compare("HIGH_WIZARD"))
			bAuthority = GM_HIGH_WIZARD;
		else if (!stAuthority.compare("LOW_WIZARD"))
			bAuthority = GM_LOW_WIZARD;
		else if (!stAuthority.compare("WIZARD"))
			bAuthority = GM_WIZARD;
		else if (!stAuthority.compare("PLAYER"))
			bAuthority = GM_PLAYER;
		else
		{
			sys_err("unkown authority %s", stAuthority.c_str());
			continue;
		}

		DWORD dwFlag = 0;
		str_to_number(dwFlag, row[col++]);
		adminConfigs->Set(bAuthority, dwFlag);
	}

	delete pMsg;
	return true;
}
//END_ADMIN_MANAGER

void CClientManager::ReloadAdmin()
{
	DGOutputPacket<DGReloadAdminPacket> pack;
	__GetAdminInfo(pack->mutable_admins());
	__GetAdminConfig(pack->mutable_admin_configs());

	ForwardPacket(pack);

	sys_log(0, "ReloadAdmin End");
}

//BREAK_MARRIAGE
void CClientManager::BreakMarriage(CPeer* peer, std::unique_ptr<network::GDMarriageBreakPacket> p)
{
	DWORD pid1, pid2;

	pid1 = p->pid1();
	pid2 = p->pid2();

	sys_log(0, "Breaking off a marriage engagement! pid %d and pid %d", pid1, pid2);
	marriage::CManager::instance().Remove(pid1, pid2);
}
//END_BREAK_MARIIAGE

void CClientManager::SendSpareItemIDRange(CPeer* peer)
{
	peer->SendSpareItemIDRange();
}

//
// Login Key만 맵에서 지운다.
// 
void CClientManager::DeleteLoginKey(std::unique_ptr<network::GDDisconnectPacket> data)
{
	char login[LOGIN_MAX_LEN+1] = {0};
	trim_and_lower(data->login().c_str(), login, sizeof(login));

	CLoginData *pkLD = GetLoginDataByLogin(login);

	if (pkLD)
	{
		TLoginDataByLoginKey::iterator it = m_map_pkLoginData.find(pkLD->GetKey());

		if (it != m_map_pkLoginData.end())
			m_map_pkLoginData.erase(it);
	}
}

void CClientManager::ForceItemDelete(DWORD dwItemID)
{
	if (!DeleteItemCache(dwItemID))
	{
		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE id = %u", dwItemID);
		CDBManager::instance().AsyncQuery(szQuery);
	}
}

void CClientManager::AddExistingPlayerName(const std::string& stName, DWORD dwPID, bool bIsBlocked)
{
	m_map_ExistingPlayerNameList[str_to_lower(stName.c_str())] = std::make_pair(dwPID, bIsBlocked);
}

void CClientManager::RemoveExistingPlayerName(const std::string& stName)
{
	m_map_ExistingPlayerNameList.erase(str_to_lower(stName.c_str()));
}

bool CClientManager::FindExistingPlayerName(const std::string& stName, DWORD* pdwPID, bool* pbIsBlocked)
{
	auto it = m_map_ExistingPlayerNameList.find(str_to_lower(stName.c_str()));
	if (it == m_map_ExistingPlayerNameList.end())
		return false;

	if (pdwPID)
		*pdwPID = it->second.first;
	if (pbIsBlocked)
		*pbIsBlocked = it->second.second;
	return true;
}

void CClientManager::SetExistingPlayerBlocked(const std::string& stName, bool bIsBlocked)
{
	auto it = m_map_ExistingPlayerNameList.find(str_to_lower(stName.c_str()));
	if (it == m_map_ExistingPlayerNameList.end())
		return;

	it->second.second = bIsBlocked;
}

void CClientManager::WhisperPlayerExistCheck(CPeer* pkPeer, DWORD dwHandle, std::unique_ptr<network::GDWhisperPlayerExistCheckPacket> p)
{
	if (g_test_server)
		sys_log(0, "WhisperPlayerExistCheck %s %d", p->target_name().c_str(), dwHandle);

	bool bIsBlocked;
	if (FindExistingPlayerName(p->target_name().c_str(), NULL, &bIsBlocked))
	{
		network::DGOutputPacket<network::DGWhisperPlayerExistResultPacket> packet;
		packet->set_pid(p->pid());
		
		packet->set_target_name(p->target_name());
		packet->set_is_exist(true);
		packet->set_is_blocked(p->is_gm() ? false : bIsBlocked);
		packet->set_return_money(false);
		packet->set_message(p->message());

		pkPeer->Packet(packet);
		return;
	}

	TWhisperPlayerExistCheckQueryData* pQueryData = new TWhisperPlayerExistCheckQueryData();

	pQueryData->kMainData = *p;
	pQueryData->dwPlayerHandle = dwHandle;
	pQueryData->pTextData = new char[strlen(p->message().c_str()) + 1];
	strlcpy(pQueryData->pTextData, p->message().c_str(), strlen(p->message().c_str()) + 1);

	char szEscapedName[CHARACTER_NAME_MAX_LEN * 2 + 1];
	CDBManager::Instance().EscapeString(szEscapedName, p->target_name().c_str(), strlen(p->target_name().c_str()));
	char szQuery[1024];
	snprintf(szQuery, sizeof(szQuery), "SELECT player.id, player.name, quest.lValue "
		"FROM player LEFT JOIN quest ON quest.dwPID = player.id AND quest.szName = 'game_option' AND quest.szState = 'block_whisper' WHERE name LIKE '%s'", szEscapedName);
	CDBManager::Instance().ReturnQuery(szQuery, QID_WHISPER_PLAYER_EXIST_CHECK, pkPeer->GetHandle(), pQueryData);
}

void CClientManager::WhisperPlayerMessageOffline(CPeer* pkPeer, DWORD dwHandle, std::unique_ptr<network::GDWhisperPlayerMessageOfflinePacket> p)
{
	DWORD dwPID;
	bool bIsBlocked;

	if (!FindExistingPlayerName(p->target_name().c_str(), &dwPID, &bIsBlocked) || (!p->is_gm() && bIsBlocked))
	{
		DGOutputPacket<DGWhisperPlayerExistResultPacket> packet;
		packet->set_pid(p->pid());
		packet->set_target_name(p->target_name());
		packet->set_is_exist(false);
		packet->set_is_blocked(false);
		packet->set_return_money(true);
		packet->set_message(p->message());

		pkPeer->Packet(packet);
		return;
	}

	char szMessage[CHAT_MAX_LEN + 1];
	CDBManager::instance().EscapeString(szMessage, p->message().c_str(), strlen(p->message().c_str()) - 1);

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "INSERT INTO offline_messages (pid, sender, message, is_gm) VALUES (%u, '%s', '%s', %u)", dwPID, p->name().c_str(), szMessage, p->is_gm());
	CDBManager::Instance().AsyncQuery(szQuery);

	network::DGOutputPacket<network::DGWhisperPlayerMessageOfflinePacket> packet;
	packet->set_pid(p->pid());
	packet->set_target_pid(dwPID);
	packet->set_target_name(p->target_name());
	packet->set_message(p->message());

	pkPeer->Packet(packet);
}

void CClientManager::RESULT_WhisperPlayerExistCheck(CPeer* peer, SQLMsg* pMsg, CQueryInfo* pData)
{
	TWhisperPlayerExistCheckQueryData* pQueryData = (TWhisperPlayerExistCheckQueryData*)pData->pvData;
	int iBufLen = strlen(pQueryData->pTextData) + 1;

	network::DGOutputPacket<network::DGWhisperPlayerExistResultPacket> packet;
	packet->set_pid(pQueryData->kMainData.pid());
	packet->set_target_name(pQueryData->kMainData.target_name());
	packet->set_is_exist(false);
	packet->set_is_blocked(false);
	packet->set_return_money(false);

	if (g_test_server)
		sys_log(0, "RESULT_WhisperPlayerExistCheck %s %d handle %d", pQueryData->kMainData.target_name().c_str(), pMsg->Get()->uiNumRows, pQueryData->dwPlayerHandle);

	if (pMsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		DWORD dwID;
		str_to_number(dwID, row[0]);
		char szPlayerName[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(szPlayerName, row[1], sizeof(szPlayerName));
		int iValue = 0;
		if (row[2])
			str_to_number(iValue, row[2]);

		AddExistingPlayerName(szPlayerName, dwID, iValue == 1);
		packet->set_is_exist(true);
		packet->set_is_blocked(pQueryData->kMainData.is_gm() ? false : iValue == 1);
	}

	peer->Packet(packet, pQueryData->dwPlayerHandle);

	delete[] pQueryData->pTextData;
	delete pQueryData;
}

void CClientManager::ResetLastPlayerID(std::unique_ptr<network::GDValidLogoutPacket> data)
{
	CLoginData* pkLD = GetLoginDataByAID( data->account_id() );

	if (NULL != pkLD)
	{
		pkLD->SetLastPlayerID( 0 );
	}
}

const TItemTable* CClientManager::GetItemTable(DWORD dwVnum) const
{
	auto it = m_map_itemTableByVnum.find(dwVnum);
	if (it == m_map_itemTableByVnum.end())
		return NULL;
	return it->second;
}

#ifdef __DUNGEON_FOR_GUILD__
void CClientManager::GuildDungeon(std::unique_ptr<network::GDGuildDungeonPacket> sPacket)
{
	CGuildManager::instance().Dungeon(sPacket->guild_id(), sPacket->channel(), sPacket->map_index());
}

void CClientManager::GuildDungeonGD(std::unique_ptr<network::GDGuildDungeonCDPacket> sPacket)
{
	CGuildManager::instance().DungeonCooldown(sPacket->guild_id(), sPacket->time());
}
#endif

#ifdef __CHECK_P2P_BROKEN__
DWORD CClientManager::GetCurrentPeerAmount()
{
	return std::count_if(m_peerList.begin(), m_peerList.end(), [](CPeer* peer)
	{
		// Get only peers with channels...
		return peer->GetChannel();
	});
}
#endif

#ifdef ENABLE_XMAS_EVENT
void CClientManager::ReloadXmasRewards()
{
	InitializeXmasRewards();

	network::DGOutputPacket<network::DGReloadXmasRewardsPacket> p;
	for (int i = 0; i < m_vec_xmasRewards.size(); ++i)
		*p->add_rewards() = m_vec_xmasRewards[i];

	for (TPeerList::iterator i = m_peerList.begin(); i != m_peerList.end(); ++i)
	{
		CPeer * tmp = *i;

		if (!tmp->GetChannel())
			continue;

		tmp->Packet(p);
	}
}
#endif

int CClientManager::TransferPlayerTable()
{
	BYTE transfer;
	if (!CConfig::Instance().GetValue("TRANSFER_PLAYER_TABLE", &transfer))
		transfer = 0;

	sys_log(0, "CClientManager::TransferPlayerTable [%s]", transfer ? "yes" : "no");
	if (!transfer)
		return true;

	std::unique_ptr<SQLMsg> pMsg(CDBManager::Instance().DirectQuery("SELECT id, skill_level, quickslot FROM player"));

	char text[8192];
	char szQuery[QUERY_MAX_LEN];
	constexpr int querySize = sizeof(szQuery);
	while (auto row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		unsigned long* pColLengths = mysql_fetch_lengths(pMsg->Get()->pSQLResult);

		DWORD pid = std::stoi(row[0]);
		sys_log(0, " TransferPlayer %u", pid);

		auto queryLen = snprintf(szQuery, querySize, "UPDATE player SET ");

		// SKILLS
		auto skills = google::protobuf::RepeatedPtrField<TPlayerSkill>();
		const char* skill_level = row[1];
		if (skill_level)
		{
#pragma pack(1)
			typedef struct
			{
				BYTE	bMasterType;
				BYTE	bLevel;
				time_t	tNextRead;
			} TOldPlayerSkill;

			TOldPlayerSkill tmp_skills[SKILL_MAX_NUM];
			TOldPlayerSkill x[255];
			const DWORD size = sizeof(x);
			memset(tmp_skills, 0, sizeof(tmp_skills));
#pragma pack()

			if (pColLengths[1] == sizeof(tmp_skills))
			{

				thecore_memcpy(tmp_skills, skill_level, sizeof(tmp_skills));
			}
			else
			{
				BYTE abSkillCount = *(BYTE*)skill_level;
				skill_level += sizeof(BYTE);

				auto rest_size = pColLengths[1] - sizeof(BYTE);
				if ((rest_size % (sizeof(BYTE) + sizeof(TOldPlayerSkill)) != 0) || (rest_size / (sizeof(BYTE) + sizeof(TOldPlayerSkill)) != abSkillCount))
				{
					sys_err("INVALID skill for pid %u rest_size %u old_player_skill size %u skill_count %u",
						pid, rest_size, sizeof(TOldPlayerSkill), abSkillCount);
				}

				for (int i = 0; i < abSkillCount; ++i)
				{
					BYTE bSkillIndex = *(BYTE*)skill_level;
					skill_level += sizeof(BYTE);

					auto pSkill = skill_level;
					skill_level += sizeof(TOldPlayerSkill);

					thecore_memcpy(&tmp_skills[bSkillIndex], pSkill, sizeof(TOldPlayerSkill));
				}
			}

			for (int i = 0; i < SKILL_MAX_NUM; ++i)
			{
				auto cur = skills.Add();
				cur->set_master_type(tmp_skills[i].bMasterType);
				cur->set_level(tmp_skills[i].bLevel);
				cur->set_next_read(tmp_skills[i].tNextRead);
			}
		}
		else
		{
			for (int i = 0; i < SKILL_MAX_NUM; ++i)
				skills.Add();
		}
		auto buffer = SerializeProtobufRepeatedPtrField(skills);
		CDBManager::instance().EscapeString(text, &buffer[0], buffer.size());
		queryLen += snprintf(szQuery + queryLen, querySize - queryLen, "skill_level = '%s', ", text);
		skills.Clear();

		// QUICKSLOT
		google::protobuf::RepeatedPtrField<TQuickslot> quickslots;
		const char* quickslots_data = row[2];
		if (quickslots_data)
		{
#pragma pack(1)
			typedef struct
			{
				BYTE	type;
				WORD	pos;
			} TOldQuickslot;

			TOldQuickslot tmp_quickslots[QUICKSLOT_MAX_NUM];
			memset(tmp_quickslots, 0, sizeof(tmp_quickslots));
#pragma pack()

			if (pColLengths[2] == sizeof(tmp_quickslots))
			{
				thecore_memcpy(tmp_quickslots, quickslots_data, sizeof(tmp_quickslots));
			}
			else
			{
				BYTE abSlotCount = *(BYTE*)quickslots_data;
				quickslots_data += sizeof(BYTE);

				sys_log(0, "Quickslots loading %d", abSlotCount);

				for (int i = 0; i < abSlotCount; ++i)
				{
					sys_log(0, "Now loading %d/%d", i + 1, abSlotCount);
					
					BYTE bSlotIndex = *(BYTE*)quickslots_data;
					quickslots_data += sizeof(BYTE);

					sys_log(0, "slotIndex %d", bSlotIndex);
					TOldQuickslot* pSlot = (TOldQuickslot*)quickslots_data;
					quickslots_data += sizeof(TOldQuickslot);

					sys_log(0, "copy slot (read_size %u fullSize %u)", quickslots_data - row[2], pColLengths[2]);
					thecore_memcpy(&tmp_quickslots[bSlotIndex], pSlot, sizeof(TOldQuickslot));
					sys_log(0, "done");
				}
			}

			for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
			{
				TQuickslot* qslot = quickslots.Add();
				qslot->set_type(tmp_quickslots[i].type);
				qslot->set_pos(tmp_quickslots[i].pos);
			}
		}
		else
		{
			for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
				quickslots.Add();
		}
		buffer = SerializeProtobufRepeatedPtrField(quickslots);
		CDBManager::instance().EscapeString(text, &buffer[0], buffer.size());
		queryLen += snprintf(szQuery + queryLen, querySize - queryLen, "quickslot = '%s' ", text);

		queryLen += snprintf(szQuery + queryLen, querySize - queryLen, "WHERE id = %u", pid);
		delete CDBManager::instance().DirectQuery(szQuery);

		thecore_tick();
	}

#ifdef ENABLE_RUNE_SYSTEM
	pMsg.reset(CDBManager::instance().DirectQuery("SELECT pid, vnumlist, pagedata FROM rune"));
	while (auto row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		DWORD pid = std::stoi(row[0]);
		sys_log(0, " TransferPlayerRunes %u", pid);

		auto queryLen = snprintf(szQuery, querySize, "UPDATE rune SET ");

		// rune
		google::protobuf::RepeatedField<google::protobuf::uint32> runes;
		const char* rune_data = row[1];
		if (rune_data)
		{
			++rune_data; // skip version
			WORD count = *(WORD*)(rune_data);
			rune_data += sizeof(WORD);

			for (int i = 0; i < count; ++i)
			{
				DWORD rune = *(DWORD*)rune_data;
				rune_data += sizeof(DWORD);

				runes.Add(rune);
			}
		}
		auto buffer = SerializeProtobufRepeatedField(runes);
		CDBManager::instance().EscapeString(text, &buffer[0], buffer.size());
		queryLen += snprintf(szQuery + queryLen, querySize - queryLen, "vnumlist = '%s', ", text);

		// page
		TRunePageData page;
		const char* page_data = row[2];
		if (page_data)
		{
#pragma pack(1)
			typedef struct SRunePageData {
				char	main_group;
				DWORD	main_vnum[RUNE_MAIN_COUNT];
				char	sub_group;
				DWORD	sub_vnum[RUNE_SUB_COUNT];
			} TOldRunePageData;
#pragma pack()

			auto tmp_page = (TOldRunePageData*) page_data;
			page.set_main_group(tmp_page->main_group);
			for (int i = 0; i < RUNE_MAIN_COUNT; ++i)
				page.add_main_vnum(tmp_page->main_vnum[i]);
			page.set_sub_group(tmp_page->sub_group);
			for (int i = 0; i < RUNE_SUB_COUNT; ++i)
				page.add_sub_vnum(tmp_page->sub_vnum[i]);
		}

		buffer.resize(page.ByteSize());
		page.SerializeToArray(&buffer[0], buffer.size());
		CDBManager::instance().EscapeString(text, &buffer[0], buffer.size());
		queryLen += snprintf(szQuery + queryLen, querySize - queryLen, "pagedata = '%s' ", text);

		queryLen += snprintf(szQuery + queryLen, querySize - queryLen, "WHERE pid = %u", pid);
		delete CDBManager::instance().DirectQuery(szQuery);
	}
#endif

	return true;
}

#ifdef CHANGE_SKILL_COLOR
void CClientManager::QUERY_SKILL_COLOR_SAVE(std::unique_ptr<network::GDSkillColorSavePacket> packet)
{
	PutSkillColorCache(std::move(packet));
}

CSkillColorCache * CClientManager::GetSkillColorCache(DWORD id)
{
	TSkillColorCacheMap::iterator it = m_map_SkillColorCache.find(id);

	if (it == m_map_SkillColorCache.end())
		return NULL;

	return it->second;
}

void CClientManager::PutSkillColorCache(std::unique_ptr<network::GDSkillColorSavePacket> packet)
{
	CSkillColorCache* pCache = GetSkillColorCache(packet->player_id());

	if (!pCache)
	{
		pCache = new CSkillColorCache;
		m_map_SkillColorCache.insert(TSkillColorCacheMap::value_type(packet->player_id(), pCache));
	}

	pCache->Put(packet.get(), false);
}

void CClientManager::UpdateSkillColorCache()
{
	TSkillColorCacheMap::iterator it = m_map_SkillColorCache.begin();

	while (it != m_map_SkillColorCache.end() && m_iCacheFlushCount < m_iCacheFlushCountLimit)
	{
		CSkillColorCache* pCache = it->second;

		if (pCache->CheckFlushTimeout())
		{
			pCache->Flush();
			++m_iCacheFlushCount;
			m_map_SkillColorCache.erase(it++);
		}
		else
			++it;
	}
}
#endif
