
#include "stdafx.h"
#include "Cache.h"

#include "QID.h"
#include "ClientManager.h"
#include "Main.h"

#include <sstream>

extern CPacketInfo g_item_info;
extern int g_iPlayerCacheFlushSeconds;
extern int g_iItemCacheFlushSeconds;
extern int g_iQuestCacheFlushSeconds;
extern int g_test_server;
extern int g_iItemPriceListTableCacheFlushSeconds;

#ifdef CHANGE_SKILL_COLOR
extern int g_iSkillColorCacheFlushSeconds;
#endif

#ifdef __EQUIPMENT_CHANGER__
extern int g_iEquipmentPageCacheFlushSeconds;
#endif

#ifdef __PET_ADVANCED__
CPetAdvancedCache::CPetAdvancedCache()
{
	m_expireTime = MIN(1800, g_iItemCacheFlushSeconds);
}

CPetAdvancedCache::~CPetAdvancedCache()
{
}

void CPetAdvancedCache::Delete()
{
	if (m_data.level() == 0)
		return;

	if (g_test_server)
		sys_log(0, "PetAdvancedCache::Delete : DELETE %u", m_data.item_id());

	m_data.set_level(0);
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
}

void CPetAdvancedCache::OnFlush()
{
	std::ostringstream query;

	if (m_data.level() == 0)
	{
		query << "DELETE FROM pet WHERE item_id=" << m_data.item_id();
		CDBManager::instance().ReturnQuery(query.str().c_str(), QID_PET_ADVANCED_DESTROY, 0, NULL);

		query.str("");
		query.clear();
		query << "DELETE FROM pet_skill WHERE item_id=" << m_data.item_id();
		CDBManager::instance().ReturnQuery(query.str().c_str(), QID_PET_ADVANCED_DESTROY, 0, NULL);
	}
	else
	{
		auto p = &m_data;

		char escapedName[CHARACTER_NAME_MAX_LEN * 2 + 1];
		CDBManager::Instance().EscapeString(escapedName, p->name().c_str(), p->name().length());

		query << "REPLACE INTO pet (item_id, name, `level`, exp, exp_item";

		for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
			query << ", attrtype" << (i + 1) << ", attrlevel" << (i + 1);

		query << ", skillpower) VALUES ("
			<< p->item_id() << ", "
			<< "'" << escapedName << "', "
			<< p->level() << ", "
			<< p->exp() << ", "
			<< p->exp_item();

		for (int i = 0; i < PET_ATTR_MAX_NUM; ++i)
			query << ", " << p->attr_type(i) << ", " << p->attr_level(i);

		query << ", " << p->skillpower() << ")";

		CDBManager::Instance().ReturnQuery(query.str().c_str(), QID_PET_ADVANCED_SAVE, 0, NULL);

		if (g_test_server)
			sys_log(0, "PetAdvancedCache::Flush : SAVE %u %s", p->item_id(), query.str().c_str());

		for (int i = 0; i < PET_SKILL_MAX_NUM; ++i)
		{
			auto& skill = p->skills(i);
			if (!skill.changed())
				continue;

			query.str("");
			query.clear();
			if (skill.vnum() != 0)
			{
				query << "REPLACE INTO pet_skill (item_id, `index`, vnum, `level`) VALUES ("
					<< p->item_id() << ", "
					<< i << ", "
					<< skill.vnum() << ", "
					<< skill.level() << ")";
			}
			else
			{
				query << "DELETE FROM pet_skill WHERE item_id = " << p->item_id() << " AND `index` = " << i;
			}

			CDBManager::Instance().ReturnQuery(query.str().c_str(), QID_PET_ADVANCED_SAVE, 0, NULL);
		}
	}

	m_bNeedQuery = false;
}
#endif

CItemCache::CItemCache()
{
	m_expireTime = MIN(60*5, g_iItemCacheFlushSeconds);
	m_dwDisableTimeout = 0;
}

CItemCache::~CItemCache()
{
}

void CItemCache::Delete()
{
	if (m_data.vnum() == 0)
		return;

	sys_log(!g_test_server, "ItemCache::Delete : DELETE %u", m_data.id());

	m_data.set_vnum(0);
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
}

void CItemCache::OnFlush()
{
	if (m_data.vnum() == 0)
	{
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item WHERE id=%u", m_data.id());
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_DESTROY, 0, NULL, SQL_PLAYER_STUFF);

		sys_log(!g_test_server, "ItemCache::Flush : DELETE %u %s", m_data.id(), szQuery);
	}
	else
	{
		if (m_data.cell().window_type() == 0)
			sys_err("CItemCache::OnFlush() window 0 itemid %u probably itemdupe error on unique expire", m_data.id());

		char szItemQuery[QUERY_MAX_LEN + QUERY_MAX_LEN];
		snprintf(szItemQuery, sizeof(szItemQuery), "REPLACE INTO item (owner_id,%s) VALUES (%u,%s)", GetItemQueryKeyPart(false), m_data.owner(),
			GetItemQueryValuePart(&m_data));

		sys_log(!g_test_server, "ItemCache::Flush :REPLACE  (%s)", szItemQuery);

		CDBManager::instance().ReturnQuery(szItemQuery, QID_ITEM_SAVE, 0, NULL, SQL_PLAYER_STUFF);
	}

	m_bNeedQuery = false;
}

void CItemCache::Disable(DWORD duration)
{
	m_dwDisableTimeout = time(0) + duration;
}

bool CItemCache::IsDisabled() const
{
	if (m_dwDisableTimeout == 0)
		return false;

	return time(0) < m_dwDisableTimeout;
}

//
// CQuestCache
//
CQuestCache::CQuestCache()
{
	m_expireTime = MIN(60*5, g_iQuestCacheFlushSeconds);
}

CQuestCache::~CQuestCache()
{
}

void CQuestCache::Delete()
{
	if (m_data.value() == 0)
		return;

	sys_log(!g_test_server, "QuestCache::Delete : DELETE owner %u name[%s] flag[%s]", m_data.pid(), m_data.name().c_str(), m_data.state().c_str());

	m_data.set_value(0);
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
}

void CQuestCache::OnFlush()
{
	char szQuery[1024];
	if (m_data.value() == 0)
	{
		snprintf(szQuery, sizeof(szQuery),
			"DELETE FROM quest WHERE dwPID=%d AND szName='%s' AND szState='%s'",
			m_data.pid(), m_data.name().c_str(), m_data.state().c_str());
	}
	else
	{
		snprintf(szQuery, sizeof(szQuery),
			"REPLACE INTO quest (dwPID, szName, szState, lValue) VALUES(%d, '%s', '%s', %d)",
			m_data.pid(), m_data.name().c_str(), m_data.state().c_str(), m_data.value());
	}

	sys_log(!g_test_server, "QuestCache::Flush : (%s)", szQuery);

	CDBManager::instance().AsyncQuery(szQuery, SQL_PLAYER_STUFF);

	m_bNeedQuery = false;
}

//
// CAffectCache
//
CAffectCache::CAffectCache()
{
	m_expireTime = MIN(60*5, g_iItemCacheFlushSeconds);

	m_data.set_pid(0);
}

CAffectCache::~CAffectCache()
{
}

void CAffectCache::Delete()
{
	if (m_data.elem().duration() <= 0)
		return;

	//char szQuery[QUERY_MAX_LEN];
	//szQuery[QUERY_MAX_LEN] = '\0';
	auto& elem = m_data.elem();
	sys_log(!g_test_server, "AffectCache::Delete : DELETE owner %u type %u pointType %u pointValue %d", m_data.pid(), elem.type(), elem.apply_on(), elem.apply_value());

	m_data.mutable_elem()->set_duration(0);
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
	
	//m_bNeedQuery = false;
	//m_lastUpdateTime = time(0) - m_expireTime; // 바로 타임아웃 되도록 하자.
}

void CAffectCache::OnFlush()
{
	auto& elem = m_data.elem();

	char szQuery[1024];
	if (elem.duration() <= 0)
	{
		snprintf(szQuery, sizeof(szQuery),
			"DELETE FROM affect WHERE dwPID=%u AND bType=%u AND bApplyOn=%u",
			m_data.pid(), elem.type(), elem.apply_on());
	}
	else
	{
		snprintf(szQuery, sizeof(szQuery),
			"REPLACE INTO affect (dwPID, bType, bApplyOn, lApplyValue, dwFlag, lDuration, lSPCost) "
			"VALUES(%u, %u, %u, %d, %u, %d, %d)",
			m_data.pid(),
			elem.type(),
			elem.apply_on(),
			elem.apply_value(),
			elem.flag(),
			elem.duration(),
			elem.sp_cost());
	}

	sys_log(!g_test_server, "AffectCache::Flush : (%s)", szQuery);

	CDBManager::instance().AsyncQuery(szQuery, SQL_PLAYER_STUFF);

	m_bNeedQuery = false;
}

//
// CPlayerTableCache
//
CPlayerTableCache::CPlayerTableCache()
{
	m_expireTime = MIN(1800, g_iPlayerCacheFlushSeconds);
}

CPlayerTableCache::~CPlayerTableCache()
{
}

void CPlayerTableCache::OnFlush()
{
	sys_log(!g_test_server, "PlayerTableCache::Flush : %s", m_data.name().c_str());

#ifdef ENABLE_RUNE_SYSTEM
	bool bChangedRune = m_data.data_changed(PC_TAB_CHANGED_RUNE);
#endif

	char szQuery[QUERY_MAX_LEN];

	CreatePlayerMountSaveQuery(szQuery, sizeof(szQuery), &m_data);
	CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, NULL, SQL_PLAYER_STUFF);

#ifdef __ATTRTREE__
	for (BYTE row = 0; row < ATTRTREE_ROW_NUM; ++row)
	{
		for (BYTE col = 0; col < ATTRTREE_COL_NUM; ++col)
		{
			if (CreatePlayerAttrtreeSaveQuery(szQuery, sizeof(szQuery), &m_data, row, col))
				CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, NULL, SQL_PLAYER_STUFF);
		}
	}
#endif

	CreatePlayerSaveQuery(szQuery, sizeof(szQuery), &m_data);
	CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, NULL, SQL_PLAYER_STUFF);

#ifdef ENABLE_RUNE_SYSTEM
	if (bChangedRune)
	{
		CreatePlayerRuneSaveQuery(szQuery, sizeof(szQuery), &m_data);
		CDBManager::instance().AsyncQuery(szQuery, SQL_PLAYER_STUFF);
	}
#endif
}

// MYSHOP_PRICE_LIST
//
// CItemPriceListTableCache class implementation
//

const int CItemPriceListTableCache::s_nMinFlushSec = 1800;

CItemPriceListTableCache::CItemPriceListTableCache()
{
	m_expireTime = MIN(s_nMinFlushSec, g_iItemPriceListTableCacheFlushSeconds);

	m_data.dwOwnerID = 0;
	m_data.byCount = 0;
}

void CItemPriceListTableCache::UpdateList(const TItemPriceListTable* pUpdateList)
{
	//
	// 이미 캐싱된 아이템과 중복된 아이템을 찾고 중복되지 않는 이전 정보는 tmpvec 에 넣는다.
	//

	std::vector<network::TItemPriceInfo> tmpvec;

	for (uint idx = 0; idx < m_data.byCount; ++idx)
	{
		auto pos = pUpdateList->aPriceInfo;
		for (; pos != pUpdateList->aPriceInfo + pUpdateList->byCount && m_data.aPriceInfo[idx].vnum() != pos->vnum(); ++pos)
			;

		if (pos == pUpdateList->aPriceInfo + pUpdateList->byCount)
			tmpvec.push_back(m_data.aPriceInfo[idx]);
	}

	//
	// pUpdateList 를 m_data 에 복사하고 남은 공간을 tmpvec 의 앞에서 부터 남은 만큼 복사한다.
	// 

	if (pUpdateList->byCount > SHOP_PRICELIST_MAX_NUM)
	{
		sys_err("Count overflow!");
		return;
	}

	m_data.byCount = pUpdateList->byCount;

	for (int i = 0; i < pUpdateList->byCount; ++i)
		m_data.aPriceInfo[i] = pUpdateList->aPriceInfo[i];

	int nDeletedNum;	// 삭제된 가격정보의 갯수

	if (pUpdateList->byCount < SHOP_PRICELIST_MAX_NUM)
	{
		size_t sizeAddOldDataSize = SHOP_PRICELIST_MAX_NUM - pUpdateList->byCount;

		if (tmpvec.size() < sizeAddOldDataSize)
			sizeAddOldDataSize = tmpvec.size();

		for (int i = 0; i < sizeAddOldDataSize; ++i)
			m_data.aPriceInfo[i + pUpdateList->byCount] = tmpvec[i];
		m_data.byCount += sizeAddOldDataSize;

		nDeletedNum = tmpvec.size() - sizeAddOldDataSize;
	}
	else
		nDeletedNum = tmpvec.size();

	m_bNeedQuery = true;

	sys_log(0, 
			"ItemPriceListTableCache::UpdateList : OwnerID[%u] Update [%u] Items, Delete [%u] Items, Total [%u] Items", 
			m_data.dwOwnerID, pUpdateList->byCount, nDeletedNum, m_data.byCount);
}

void CItemPriceListTableCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];

	//
	// 이 캐시의 소유자에 대한 기존에 DB 에 저장된 아이템 가격정보를 모두 삭제한다.
	//

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM myshop_pricelist WHERE owner_id = %u", m_data.dwOwnerID);
	CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_DESTROY, 0, NULL);

	//
	// 캐시의 내용을 모두 DB 에 쓴다.
	//

	for (int idx = 0; idx < m_data.byCount; ++idx)
	{
		snprintf(szQuery, sizeof(szQuery),
				"INSERT INTO myshop_pricelist(owner_id, item_vnum, price) VALUES(%u, %u, %u)", 
				m_data.dwOwnerID, m_data.aPriceInfo[idx].vnum(), m_data.aPriceInfo[idx].price());
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_SAVE, 0, NULL);
	}

	sys_log(0, "ItemPriceListTableCache::Flush : OwnerID[%u] Update [%u]Items", m_data.dwOwnerID, m_data.byCount);
	
	m_bNeedQuery = false;
}
// END_OF_MYSHOP_PRICE_LIST

#ifdef CHANGE_SKILL_COLOR
CSkillColorCache::CSkillColorCache()
{
	m_expireTime = MIN(1800, g_iSkillColorCacheFlushSeconds);
}

CSkillColorCache::~CSkillColorCache()
{

}

void CSkillColorCache::OnFlush()
{
	bool bIsSetNULL = true;
	for (int i = 0; i < ESkillColorLength::MAX_SKILL_COUNT && bIsSetNULL; ++i)
	{
		for (int x = 0; x < ESkillColorLength::MAX_EFFECT_COUNT; ++x)
		{
			auto id = i * ESkillColorLength::MAX_EFFECT_COUNT + x;

			if (id < m_data.skill_colors_size() && m_data.skill_colors(id) > 0)
			{
				bIsSetNULL = false;
				break;
			}
		}
	}

	if (bIsSetNULL)
	{
		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM skill_color WHERE player_id=%u", m_data.player_id());
		CDBManager::instance().ReturnQuery(szQuery, QID_SKILL_COLOR_SAVE, 0, NULL);

		if (g_test_server)
			sys_log(0, "SkillColorCache::Flush : DELETE %u %s", m_data.player_id(), szQuery);
	}
	else
	{
		DWORD dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
		for (int i = 0; i < ESkillColorLength::MAX_SKILL_COUNT; ++i)
		{
			for (int x = 0; x < ESkillColorLength::MAX_EFFECT_COUNT; ++x)
			{
				auto id = i * ESkillColorLength::MAX_EFFECT_COUNT + x;

				if (id < m_data.skill_colors_size())
					dwSkillColor[i][x] = m_data.skill_colors(id);
				else
					dwSkillColor[i][x] = 0;
			}
		}

		char query[1024];
		snprintf(query, sizeof(query),
			"REPLACE INTO skill_color (player_id, s1_col1, s1_col2, s1_col3, s1_col4, s1_col5, "
			"s2_col1, s2_col2, s2_col3, s2_col4, s2_col5,s3_col1, s3_col2, s3_col3, s3_col4, s3_col5, "
			"s4_col1, s4_col2, s4_col3, s4_col4, s4_col5,s5_col1, s5_col2, s5_col3, s5_col4, s5_col5, "
			"s6_col1, s6_col2, s6_col3, s6_col4, s6_col5"
#ifdef __FAKE_BUFF__
			", s7_col1, s7_col2, s7_col3, s7_col4, s7_col5"
			", s8_col1, s8_col2, s8_col3, s8_col4, s8_col5"
			", s9_col1, s9_col2, s9_col3, s9_col4, s9_col5"
#endif
			") VALUES (%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d"
#ifdef __FAKE_BUFF__
			", %d, %d, %d, %d, %d"
			", %d, %d, %d, %d, %d"
			", %d, %d, %d, %d, %d"
#endif
			
			")",
			m_data.player_id(), dwSkillColor[0][0], dwSkillColor[0][1], dwSkillColor[0][2], dwSkillColor[0][3], dwSkillColor[0][4],
			dwSkillColor[1][0], dwSkillColor[1][1], dwSkillColor[1][2], dwSkillColor[1][3], dwSkillColor[1][4],
			dwSkillColor[2][0], dwSkillColor[2][1], dwSkillColor[2][2], dwSkillColor[2][3], dwSkillColor[2][4],
			dwSkillColor[3][0], dwSkillColor[3][1], dwSkillColor[3][2], dwSkillColor[3][3], dwSkillColor[3][4],
			dwSkillColor[4][0], dwSkillColor[4][1], dwSkillColor[4][2], dwSkillColor[4][3], dwSkillColor[4][4],
			dwSkillColor[5][0], dwSkillColor[5][1], dwSkillColor[5][2], dwSkillColor[5][3], dwSkillColor[5][4]
#ifdef __FAKE_BUFF__
			, dwSkillColor[6][0], dwSkillColor[6][1], dwSkillColor[6][2], dwSkillColor[6][3], dwSkillColor[6][4]
			, dwSkillColor[7][0], dwSkillColor[7][1], dwSkillColor[7][2], dwSkillColor[7][3], dwSkillColor[7][4]
			, dwSkillColor[8][0], dwSkillColor[8][1], dwSkillColor[8][2], dwSkillColor[8][3], dwSkillColor[8][4]
#endif
			);

		CDBManager::instance().ReturnQuery(query, QID_SKILL_COLOR_SAVE, 0, NULL);

		if (g_test_server)
			sys_log(0, "SkillColorCache::Flush :REPLACE %u (%s)", m_data.player_id(), query);
	}
	m_bNeedQuery = false;
}
#endif

#ifdef __EQUIPMENT_CHANGER__
CEquipmentPageCache::CEquipmentPageCache()
{
	m_expireTime = MIN(1800, g_iEquipmentPageCacheFlushSeconds);
}

CEquipmentPageCache::~CEquipmentPageCache()
{
}

void CEquipmentPageCache::Delete()
{
	sys_log(!g_test_server, "EquipmentPageCache::Delete : DELETE %u pid %u", m_data.index());

	m_data.clear_page_name();
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
}

void CEquipmentPageCache::OnFlush()
{
	if (m_data.page_name().empty())
	{
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM equipment_page WHERE pid=%u AND page_index=%u", m_data.pid(), m_data.index());
		CDBManager::instance().ReturnQuery(szQuery, QID_EQUIPMENT_PAGE_DESTROY, 0, NULL);

		if (g_test_server)
			sys_log(0, "EquipmentPageCache::Flush : DELETE pid %u index %u %s", m_data.pid(), m_data.index(), szQuery);
	}
	else
	{
		char szPageName[EQUIPMENT_PAGE_NAME_MAX_LEN * 2 + 1];
		CDBManager::instance().EscapeString(szPageName, m_data.page_name().c_str(), m_data.page_name().length());

		DWORD adwItemID[EQUIPMENT_PAGE_MAX_PARTS];
		for (int i = 0; i < EQUIPMENT_PAGE_MAX_PARTS; ++i)
		{
			if (i < m_data.item_ids_size())
				adwItemID[i] = m_data.item_ids(i);
			else
				adwItemID[i] = 0;
		}

		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "REPLACE INTO equipment_page SET pid=%u, page_index=%u, page_name='%s', rune_page_index=%d,"
												"EQUIPMENT_CHANGER_WEAR_BODY = %u,"
												"EQUIPMENT_CHANGER_WEAR_HEAD = %u,"
												"EQUIPMENT_CHANGER_WEAR_FOOTS = %u,"
												"EQUIPMENT_CHANGER_WEAR_WRIST = %u,"
												"EQUIPMENT_CHANGER_WEAR_WEAPON = %u,"
												"EQUIPMENT_CHANGER_WEAR_NECK = %u,"
												"EQUIPMENT_CHANGER_WEAR_EAR = %u,"
												"EQUIPMENT_CHANGER_WEAR_ARROW = %u,"
												"EQUIPMENT_CHANGER_WEAR_SHIELD = %u,"
												"EQUIPMENT_CHANGER_WEAR_COSTUME_BODY = %u,"
												"EQUIPMENT_CHANGER_WEAR_COSTUME_HAIR = %u,"
												"EQUIPMENT_CHANGER_WEAR_COSTUME_WEAPON = %u,"
												"EQUIPMENT_CHANGER_WEAR_ACCE = %u,"
												"EQUIPMENT_CHANGER_WEAR_BELT = %u,"
												"EQUIPMENT_CHANGER_WEAR_TOTEM = %u",

				m_data.pid(), m_data.index(), szPageName, m_data.rune_page(),
				adwItemID[EQUIPMENT_CHANGER_WEAR_BODY],
				adwItemID[EQUIPMENT_CHANGER_WEAR_HEAD],
				adwItemID[EQUIPMENT_CHANGER_WEAR_FOOTS],
				adwItemID[EQUIPMENT_CHANGER_WEAR_WRIST],
				adwItemID[EQUIPMENT_CHANGER_WEAR_WEAPON],
				adwItemID[EQUIPMENT_CHANGER_WEAR_NECK],
				adwItemID[EQUIPMENT_CHANGER_WEAR_EAR],
				adwItemID[EQUIPMENT_CHANGER_WEAR_ARROW],
				adwItemID[EQUIPMENT_CHANGER_WEAR_SHIELD],
				adwItemID[EQUIPMENT_CHANGER_WEAR_COSTUME_BODY],
				adwItemID[EQUIPMENT_CHANGER_WEAR_COSTUME_HAIR],
				adwItemID[EQUIPMENT_CHANGER_WEAR_COSTUME_WEAPON],
				adwItemID[EQUIPMENT_CHANGER_WEAR_ACCE],
				adwItemID[EQUIPMENT_CHANGER_WEAR_BELT],
				adwItemID[EQUIPMENT_CHANGER_WEAR_TOTEM]

			);



		CDBManager::instance().ReturnQuery(szQuery, QID_EQUIPMENT_PAGE_SAVE, 0, NULL);

		if (g_test_server)
			sys_log(0, "EquipmentPageCache::Flush : REPLACE %s", szQuery);
	}
}
#endif
