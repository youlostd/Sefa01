
#include "stdafx.h"

#include "ClientManager.h"

#include "Main.h"
#include "QID.h"
#include "ItemAwardManager.h"
#include "Cache.h"

#include <sstream>

extern int g_test_server;
extern int g_log;

//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !!!!!!!!!!! IMPORTANT !!!!!!!!!!!!
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
// Check all SELECT syntax on item table before change this function!!!
//
bool CreateItemTableFromRes(MYSQL_RES* res, std::function<network::TItemData * ()> alloc_func, DWORD pid, int col)
{
	if (!res)
		return true;

	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// µ¥ÀÌÅÍ ¾øÀ½
		return true;

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		auto item = alloc_func();

		int cur = 0;

		int iNowCol = CreateItemTableFromRow(row, item, col);

		++iNowCol;

		item->set_owner(pid);
	}

	return true;
}

// Check all SELECT syntax on item table before change this function!!!
// Check all SELECT syntax on item table before change this function!!!
// Check all SELECT syntax on item table before change this function!!!
int CreateItemTableFromRow(MYSQL_ROW row, network::TItemData * pRes, int iCol, bool bWindowDataCols)
{
	pRes->set_id(std::stoll(row[iCol++]));
	if (bWindowDataCols)
	{
		auto cell = pRes->mutable_cell();
		cell->set_window_type(std::stoll(row[iCol++]));

#ifdef __ALPHA_EQUIP__
		str_to_number(pRes->alpha_equip_value, row[iCol++]);
#endif

		cell->set_cell(std::stoll(row[iCol++]));
	}
	else
	{
#ifdef __ALPHA_EQUIP__
		str_to_number(pRes->alpha_equip_value, row[iCol++]);
#endif
	}

	pRes->set_count(std::stoll(row[iCol++]));
	pRes->set_vnum(std::stoll(row[iCol++]));
	pRes->set_is_gm_owner(std::stoll(row[iCol++]));

	pRes->clear_sockets();
	for (int j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
		pRes->add_sockets(std::stoll(row[iCol++]));
	
	pRes->clear_attributes();
	for (int j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
	{
		auto *attribute = pRes->add_attributes();
		attribute->set_type(std::stoll(row[iCol++]));
		attribute->set_value(std::stoll(row[iCol++]));
	}

	return iCol;
}

const char* GetItemQueryKeyPart(bool bSelect)
{
	static char s_szBuf[2048];
	int len = snprintf(s_szBuf, sizeof(s_szBuf), "item.id,item.window%s,item.pos,item.count,item.vnum,item.is_gm_owner,"
		"item.socket0,item.socket1,item.socket2,"
		"item.attrtype0,item.attrvalue0,"
		"item.attrtype1,item.attrvalue1,"
		"item.attrtype2,item.attrvalue2,"
		"item.attrtype3,item.attrvalue3,"
		"item.attrtype4,item.attrvalue4,"
		"item.attrtype5,item.attrvalue5,"
		"item.attrtype6,item.attrvalue6", bSelect ? "+0" : "");
#ifdef __ALPHA_EQUIP__
	len += snprintf(s_szBuf + len, sizeof(s_szBuf) - len, ",item.alpha_equip");
#endif

	return s_szBuf;
}

const char* GetItemQueryKeyPart(bool bSelect, const char* c_pszTableName, const char* c_pszIDColumnName, const char* c_pszRemoveColumns)
{
	static char s_szBuf[2048];

	char* pszWriteLastPos = s_szBuf;
	const char* c_pszReadLastPos = GetItemQueryKeyPart(bSelect);

	int iWriteCount;
	int iCount = 0;
	while (const char* c_pszReadFind = strstr(c_pszReadLastPos, "item."))
	{
		// copy all since last found of table name
		if (c_pszReadFind != c_pszReadLastPos)
		{
			iWriteCount = c_pszReadFind - c_pszReadLastPos;
			strncpy(pszWriteLastPos, c_pszReadLastPos, iWriteCount);
			pszWriteLastPos += iWriteCount;
		}

		// add table name
		if (c_pszTableName && *c_pszTableName)
		{
			iWriteCount = strlen(c_pszTableName);
			strncpy(pszWriteLastPos, c_pszTableName, iWriteCount);
			pszWriteLastPos += iWriteCount;
			strncpy(pszWriteLastPos++, ".", 1);
		}
		c_pszReadLastPos = c_pszReadFind + strlen("item.");

		if (++iCount == 1)
		{
			iWriteCount = strlen(c_pszIDColumnName);
			strncpy(pszWriteLastPos, c_pszIDColumnName, iWriteCount);
			pszWriteLastPos += iWriteCount;
			strncpy(pszWriteLastPos++, ",", 1);
			c_pszReadLastPos += strlen("id,");
		}
	}

	// copy all since last found of table name
	iWriteCount = strlen(c_pszReadLastPos) + 1;
	strncpy(pszWriteLastPos, c_pszReadLastPos, iWriteCount);

	// remove columns
	const char* c_pszStart = c_pszRemoveColumns;
	const char* c_pszEnd = c_pszRemoveColumns + strlen(c_pszRemoveColumns);
	while (c_pszStart < c_pszEnd) {
		const char* c_pszFind = strstr(c_pszStart, "|");
		if (c_pszFind == NULL)
			c_pszFind = c_pszEnd;

		std::string stCur;
		stCur.assign(c_pszStart, c_pszFind);
		stCur = "," + stCur;

		c_pszStart = c_pszFind + 1;

		pszWriteLastPos = strstr(s_szBuf, stCur.c_str());
		if (pszWriteLastPos)
		{
			std::string stTemp = pszWriteLastPos + stCur.length();
			strcpy(pszWriteLastPos, stTemp.c_str());
			//strcpy(pszWriteLastPos, pszWriteLastPos + stCur.length());
		}
	}

	return s_szBuf;
}

const char* GetItemQueryValuePart(const network::TItemData * pkTab, bool bWindowDataCols)
{
	static char s_szBuf[2048];
	int iLen = snprintf(s_szBuf, sizeof(s_szBuf), "%u, ", pkTab->id());

	if (bWindowDataCols)
		iLen += snprintf(s_szBuf + iLen, sizeof(s_szBuf) - iLen, "%u, %u, ", pkTab->cell().window_type(), pkTab->cell().cell());

	iLen += snprintf(s_szBuf + iLen, sizeof(s_szBuf) - iLen, "%u, %u, %u, "
		"%d, %d, %d, "
		"%u, %d, "
		"%u, %d, "
		"%u, %d, "
		"%u, %d, "
		"%u, %d, "
		"%u, %d, "
		"%u, %d",
		pkTab->count(), pkTab->vnum(), pkTab->is_gm_owner(),
		pkTab->sockets(0), pkTab->sockets(1), pkTab->sockets(2),
		pkTab->attributes(0).type(), pkTab->attributes(0).value(),
		pkTab->attributes(1).type(), pkTab->attributes(1).value(),
		pkTab->attributes(2).type(), pkTab->attributes(2).value(),
		pkTab->attributes(3).type(), pkTab->attributes(3).value(),
		pkTab->attributes(4).type(), pkTab->attributes(4).value(),
		pkTab->attributes(5).type(), pkTab->attributes(5).value(),
		pkTab->attributes(6).type(), pkTab->attributes(6).value());


#ifdef __ALPHA_EQUIP__
	iLen += snprintf(s_szBuf + iLen, sizeof(s_szBuf) - iLen, ", %d", pkTab->alpha_equip_value);
#endif

	return s_szBuf;
}

#ifdef __ATTRTREE__
bool CreatePlayerAttrtreeSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab, BYTE row, BYTE col)
{
	if (pkTab->attrtree[row][col] == 0)
		return false;
	
	snprintf(pszQuery, querySize, "REPLACE INTO attrtree (pid, row, col, level) VALUES (%u, %u, %u, %u)",
		pkTab->id, row, col, pkTab->attrtree[row][col]);
	return true;
}
#endif

#ifdef CHANGE_SKILL_COLOR
bool CreateSkillColorTableFromRes(MYSQL_RES * res, DWORD * dwSkillColor)
{
	if (mysql_num_rows(res) == 0)
		return false;

	MYSQL_ROW row = mysql_fetch_row(res);

	for (int x = 0; x < ESkillColorLength::MAX_SKILL_COUNT; ++x)
	{
		for (int i = 0; i < ESkillColorLength::MAX_EFFECT_COUNT; ++i)
		{
			*(dwSkillColor++) = std::stoll(row[i + (x*ESkillColorLength::MAX_EFFECT_COUNT)]);
		}
	}

	return true;
}
#endif

size_t CreatePlayerSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab)
{
	size_t queryLen;

	queryLen = snprintf(pszQuery, querySize,
			"UPDATE player SET "
			"job = %d,"
			"voice = %d,"
			"dir = %d,"
			"x = %d,"
			"y = %d,"
			"map_index = %d,"
			"exit_x = %d,"
			"exit_y = %d,"
			"exit_map_index = %d,"
			"hp = %d,"
			"mp = %d,"
			"playtime = %d,"
			"level = %d,"
#ifdef __PRESTIGE__
			"prestige_level = %d,"
#endif
			"level_step = %d,"
			"st = %d,"
			"ht = %d,"
			"dx = %d,"
			"iq = %d,"
			"gold = %lld,"
#ifdef ENABLE_ZODIAC_TEMPLE
			"animasphere = %d,"
#endif
			"exp = %u,"
			"stat_point = %d,"
			"skill_point = %d,"
			"sub_skill_point = %d,"
			"stat_reset_count = %d,"
			"part_main = %d,"
			"part_hair = %d,"
#ifdef __ACCE_COSTUME__
			"part_acce = %d,"
#endif
			"last_play = NOW(),"
			"skill_group = %d,"
			"alignment = %d,"
#ifdef __GAYA_SYSTEM__
			"gaya = %u,"
#endif
#ifdef __FAKE_BUFF__
			"buffiskill1 = %u,"
			"buffiskill2 = %u,"
			"buffiskill3 = %u,"
#endif
			"inventory_max_num = %u,"
			"uppitem_inv_max_num = %u,"

			"skillbook_inv_max_num = %u,"
			"stone_inv_max_num = %u,"
			"enchant_inv_max_num = %u"

			,
		pkTab->job(),
		pkTab->voice(),
		pkTab->dir(),
		pkTab->x(),
		pkTab->y(),
		pkTab->map_index(),
		pkTab->exit_x(),
		pkTab->exit_y(),
		pkTab->exit_map_index(),
		pkTab->hp(),
		pkTab->sp(),
		pkTab->playtime(),
		pkTab->level(),
#ifdef __PRESTIGE__
		pkTab->prestige_level(),
#endif
		pkTab->level_step(),
		pkTab->st(),
		pkTab->ht(),
		pkTab->dx(),
		pkTab->iq(),
		pkTab->gold(),
#ifdef ENABLE_ZODIAC_TEMPLE
		pkTab->animasphere(),
#endif
		pkTab->exp(),
		pkTab->stat_point(),
		pkTab->skill_point(),
		pkTab->sub_skill_point(),
		pkTab->stat_reset_count(),
		pkTab->parts(PART_MAIN),
		pkTab->parts(PART_HAIR),
#ifdef __ACCE_COSTUME__
		pkTab->parts(PART_ACCE),
#endif
		pkTab->skill_group(),
		pkTab->alignment(),
#ifdef __GAYA_SYSTEM__
		pkTab->gaya(),
#endif
#ifdef __FAKE_BUFF__
		pkTab->fakebuff_skill1(),
		pkTab->fakebuff_skill2(),
		pkTab->fakebuff_skill3(),
#endif
		pkTab->inventory_max_num(),
		pkTab->uppitem_inv_max_num(),

		pkTab->skillbook_inv_max_num(),
		pkTab->stone_inv_max_num(),
		pkTab->enchant_inv_max_num()

		);

#ifdef __EQUIPMENT_CHANGER__
	queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", equipment_page_index = %u", pkTab->equipment_page_index());
#endif
	
#ifdef COMBAT_ZONE
	queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", combat_zone_points = %u", pkTab->combat_zone_points());
#endif

	// Binary ·Î ¹Ù²Ù±â À§ÇÑ ÀÓ½Ã °ø°£
	static char text[8192 + 1];

	// generate skill save data
	if (pkTab->data_changed(PC_TAB_CHANGED_SKILLS))
	{
		auto buffer = SerializeProtobufRepeatedPtrField(pkTab->skills());
		CDBManager::instance().EscapeString(text, &buffer[0], buffer.size());
		queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", skill_level = '%s'", text);
	}

	// generate quickslot save data
	if (pkTab->data_changed(PC_TAB_CHANGED_QUICKSLOT))
	{
		auto buffer = SerializeProtobufRepeatedPtrField(pkTab->quickslots());
		CDBManager::instance().EscapeString(text, &buffer[0], buffer.size());
		queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", quickslot = '%s'", text);
	}

	for (int i = 0; i < pkTab->data_changed_size(); ++i)
		pkTab->set_data_changed(i, false);

	queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, " WHERE id=%d", pkTab->id());
	return queryLen;
}

size_t CreatePlayerMountSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab)
{
	size_t queryLen;

	if (pkTab->mount_state() != 0 || !pkTab->mount_name().empty() || pkTab->mount_item_id() != 0 || pkTab->horse_grade() > 0 || pkTab->horse_elapsed_time() > 0 || pkTab->horse_skill_point() > 0)
	{
		queryLen = snprintf(pszQuery, querySize,
			"REPLACE INTO player_mount SET "
			"pid = %u, "
			"`state` = %d, "
			"item_id = %u, "
			"horse_grade = %u, "
			"horse_elapsed_time = %u, "
			"horse_skill_point = %u",
			pkTab->id(), pkTab->mount_state() + 1, pkTab->mount_item_id(), pkTab->horse_grade(), pkTab->horse_elapsed_time(), pkTab->horse_skill_point());

		static char text[8192 + 1];

		CDBManager::instance().EscapeString(text, pkTab->mount_name().c_str(), pkTab->mount_name().size());
		queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", name = '%s'", text);
	}
	else
	{
		queryLen = snprintf(pszQuery, querySize,
			"DELETE FROM player_mount WHERE pid = %u", pkTab->id());
	}

	return queryLen;
}

#ifdef ENABLE_RUNE_SYSTEM
size_t CreatePlayerRuneSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab)
{
	size_t queryLen;

	if (!pkTab->runes_size())
	{
		// queryLen = snprintf(pszQuery, querySize, "DELETE FROM rune WHERE pid = %d", pkTab->id);
		sys_err("WOULD DELETE RUNE STUFF FROM:  pid = %d", pkTab->id());
	}
	else
	{
		queryLen = snprintf(pszQuery, querySize, "REPLACE INTO rune (pid, vnumlist, pagedata) VALUES (%d", pkTab->id());

		const static BYTE bVersion = 1;

		auto serialized_runes = SerializeProtobufRepeatedField(pkTab->runes());

		std::vector<uint8_t> serialized_page;
		serialized_page.resize(pkTab->rune_page_data().ByteSize());
		pkTab->rune_page_data().SerializeToArray(&serialized_page[0], serialized_page.size());

		static char text[8192 + 1];
		CDBManager::instance().EscapeString(text, &serialized_runes[0], serialized_runes.size());
		queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", '%s'", text);
		CDBManager::instance().EscapeString(text, &serialized_page[0], serialized_page.size());
		queryLen += snprintf(pszQuery + queryLen, querySize - queryLen, ", '%s')", text);
	}

	return queryLen;
}
#endif

/*********************************
*** CACHE STORING - MAIN PLAYER CACHE
*********************************/

CClientManager::TPlayerCache * CClientManager::GetPlayerCache(DWORD id, bool bForceCreate)
{
	TPlayerCacheMap::iterator it = m_map_playerCache.find(id);

	if (it == m_map_playerCache.end())
	{
		if (!bForceCreate)
			return NULL;

		sys_log(0, "CREATE_PLAYER_CACHE %u", id);

		TPlayerCache* pCacheData = new TPlayerCache;
		pCacheData->pPlayer = NULL;

		m_map_playerCache[id] = pCacheData;
		return pCacheData;
	}
	else
	{
		TPlayerCache* pCacheData = it->second;

		// update player table
		if (pCacheData->pPlayer)
		{
			TPlayerTable * pTab = pCacheData->pPlayer->Get(false);
			pTab->set_logoff_interval(GetCurrentTime() - pCacheData->pPlayer->GetLastUpdateTime());
		}

		return pCacheData;
	}
}

void CClientManager::FlushPlayerCache(DWORD id)
{
	sys_log(0, "CACHE: FlushPlayerCache(%u)", id);

	TPlayerCache* pCache = GetPlayerCache(id, false);
	if (!pCache)
	{
		sys_log(0, " -- FLUSH PID NOT FOUND %u", id);
		return;
	}

	// flush player data
	if (pCache->pPlayer)
	{
		pCache->pPlayer->Flush();
		delete pCache->pPlayer;

		pCache->pPlayer = NULL;
	}

	// flush item data
	for (CItemCache* pItemCache : pCache->setItems)
	{
		m_map_itemCacheByID.erase(pItemCache->Get(false)->id()); // remove item from global map

#ifdef __PET_ADVANCED__
		if (!pItemCache->Get()->vnum() || IsPetItem(pItemCache->Get()->vnum()))
			FlushPetCache(pItemCache->Get()->id());
#endif

		pItemCache->Flush();
		delete pItemCache;
	}
	pCache->setItems.clear();

	// flush quest data
	for (auto it : pCache->mapQuests)
	{
		CQuestCache* pQuestCache = it.second;
		pQuestCache->Flush();
		delete pQuestCache;
	}
	pCache->mapQuests.clear();

	// flush affect data
	for (auto it : pCache->mapAffects)
	{
		CAffectCache* pAffectCache = it.second;
		pAffectCache->Flush();
		delete pAffectCache;
	}
	pCache->mapAffects.clear();

	// remove cache
	delete pCache;
	m_map_playerCache.erase(id);
}

void CClientManager::UpdatePlayerCache()
{
	TPlayerCacheMap::iterator it = m_map_playerCache.begin();
	TPlayerCacheMap::iterator it_end = m_map_playerCache.end();

	while (it != it_end && m_iCacheFlushCount < m_iCacheFlushCountLimit)
	{
		TPlayerCache* pCacheData = it->second;
		++it;

		// check player
		if (pCacheData->pPlayer && pCacheData->pPlayer->CheckFlushTimeout())
		{
			pCacheData->pPlayer->Flush();
			++m_iCacheFlushCount;
		}

		for (CItemCache* pItemCache : pCacheData->setItems)
		{
			if (pItemCache->CheckFlushTimeout())
			{
				pItemCache->Flush();

				++m_iCacheFlushCount;
			}
		}

		// check quest
		for (auto it : pCacheData->mapQuests)
		{
			CQuestCache* pQuestCache = it.second;
			if (pQuestCache->CheckFlushTimeout())
			{
				pQuestCache->Flush();
				++m_iCacheFlushCount;
			}
		}

		// check affect
		for (auto it : pCacheData->mapAffects)
		{
			CAffectCache* pAffectCache = it.second;
			if (pAffectCache->CheckFlushTimeout())
			{
				pAffectCache->Flush();
				++m_iCacheFlushCount;
			}
		}
	}
}

/*********************************
*** CACHE STORING - PUT DIFFERENT CACHES
*********************************/

void CClientManager::PutPlayerCache(const TPlayerTable * pNew, bool bSkipSave)
{
	if (g_test_server)
		sys_log(0, "CACHE: PutPlayerCache(%u) [skip %d]", pNew->id(), bSkipSave);

	TPlayerCache* pCacheData = GetPlayerCache(pNew->id(), bSkipSave); // only create new cache in loading phase when skipSave == true
	if (!pCacheData)
	{
		if (!bSkipSave)
		{
			sys_log(0, "PLAYER_CACHE DirectSave(%u)", pNew->id());

			CPlayerTableCache kCache;
			kCache.Put(pNew);
			kCache.Flush();
		}
		else
			sys_err("WTF?? PLAYER_CACHE NOT CREATED owner %u", pNew->id());

		return;
	}

	CPlayerTableCache*& rpPlayer = pCacheData->pPlayer;
	bool changed[PC_TAB_CHANGED_MAX_NUM];
	memset(changed, 0, sizeof(changed));
	// new created
	if (!rpPlayer)
		rpPlayer = new CPlayerTableCache;
	// already in cache
	else
	{
		for (int i = 0; i < PC_TAB_CHANGED_MAX_NUM; ++i)
		{
			if (pNew->data_changed(i) == false && rpPlayer->Get()->data_changed(i))
				changed[i] = true;
		}
	}

	rpPlayer->Put(pNew, bSkipSave);
	for (int i = 0; i < PC_TAB_CHANGED_MAX_NUM; ++i)
		if (changed[i])
			rpPlayer->Get()->set_data_changed(i, true);
}

void CClientManager::PutItemCache(const network::TItemData * pNew, bool bSkipQuery)
{
	if (g_test_server)
		sys_log(0, "CACHE: PutItemCache(%u %u owner %u) [skip %d]", pNew->id(), pNew->vnum(), pNew->owner(), bSkipQuery);

	CItemCache* pItem = GetItemCache(pNew->id());

	bool bIsNew = pItem == NULL;
	if (bIsNew)
	{
		pItem = new CItemCache;
		m_map_itemCacheByID[pNew->id()] = pItem;
	}
	else
	{
		DWORD dwOldOwner = pItem->Get(false)->owner();
		if (dwOldOwner != pNew->owner())
		{
			if (g_test_server)
				sys_log(0, " :: Owner changed from %u -> %u", dwOldOwner, pNew->owner());
			
			TPlayerCache* pOldCacheData = GetPlayerCache(dwOldOwner, false);
			if (pOldCacheData)
				pOldCacheData->setItems.erase(pItem);

			bIsNew = true;
		}
	}
	
	if (bIsNew)
	{
		TPlayerCache* pCacheData = GetPlayerCache(pNew->owner(), false);
		if (!pCacheData)
		{
			EraseItemCache(pNew->id());

			if (!bSkipQuery)
			{
				sys_log(0, "ITEM_CACHE DirectSave(%u %u owner %u)", pNew->id(), pNew->vnum(), pNew->owner());

				CItemCache kCache;
				kCache.Put(pNew);
				kCache.Flush();
			}
			else
				sys_log(0, "ITEM_CACHE InvalidLoading : no player cache found id %u owner %u", pNew->id(), pNew->owner());

			return;
		}

		pCacheData->setItems.insert(pItem);
	}

	pItem->Put(pNew, bSkipQuery);
}

void CClientManager::PutQuestCache(const TQuestTable * pNew, bool bSkipSave)
{
	if (g_test_server)
		sys_log(0, "CACHE: PutQuestCache(%u %s.%s) [skip %d]", pNew->pid(), pNew->name().c_str(), pNew->state().c_str(), bSkipSave);
	
	TPlayerCache* pCacheData = GetPlayerCache(pNew->pid(), false);
	if (!pCacheData)
	{
		if (!bSkipSave)
		{
			sys_log(0, "QUEST_CACHE DirectSave(%u %s.%s)", pNew->pid(), pNew->name().c_str(), pNew->state().c_str());

			CQuestCache kCache;
			kCache.Put(pNew);
			kCache.Flush();
		}

		return;
	}

	TQuestCacheMap& rkQuestMap = pCacheData->mapQuests;
	CQuestCache* pCache = NULL;

	QUEST_KEY_TYPE kType = QUEST_KEY_TYPE(pNew);
	TQuestCacheMap::iterator it = rkQuestMap.find(kType);
	if (it == rkQuestMap.end())
	{
		pCache = new CQuestCache;
		rkQuestMap[kType] = pCache;
	}
	else
	{
		pCache = it->second;

		if (pCache->Get(false)->value() == pNew->value())
		{
			if (g_test_server)
				sys_log(0, " QuestCache ignore (equal value)");

			return;
		}
	}

	pCache->Put(pNew, bSkipSave);
}

void CClientManager::PutAffectCache(DWORD dwPID, const TPacketAffectElement * pNew, bool bSkipSave)
{
	TAffectSaveElement kAffectElem;
	kAffectElem.set_pid(dwPID);
	kAffectElem.set_elem(pNew);
	PutAffectCache(&kAffectElem, bSkipSave);
}

void CClientManager::PutAffectCache(const TAffectSaveElement * pNew, bool bSkipSave)
{
	if (g_test_server)
		sys_log(0, "CACHE: PutAffectCache(%u %u %u) [skip %d]", pNew->pid(), pNew->elem().type(), pNew->elem().apply_on(), bSkipSave);

	TPlayerCache* pCacheData = GetPlayerCache(pNew->pid(), false);
	if (!pCacheData)
	{
		if (!bSkipSave)
		{
			sys_log(0, "AFFECT_CACHE DirectSave(%u %u %u)", pNew->pid(), pNew->elem().type(), pNew->elem().apply_on());

			CAffectCache kCache;
			kCache.Put(pNew);
			kCache.Flush();
		}

		return;
	}

	TAffectCacheMap& rkAffectMap = pCacheData->mapAffects;
	CAffectCache* pCache = NULL;

	auto kType = AFFECT_KEY_TYPE(&pNew->elem());
	TAffectCacheMap::iterator it = rkAffectMap.find(kType);
	if (it == rkAffectMap.end())
	{
		pCache = new CAffectCache;
		rkAffectMap[kType] = pCache;
	}
	else
	{
		pCache = it->second;
	}

	pCache->Put(pNew, bSkipSave);
}

/*********************************
*** CACHE STORING - DELETE NORMAL CACHES
*********************************/

void CClientManager::DeleteQuestCache(DWORD dwPlayerID, const QUEST_KEY_TYPE& c_rkQuestKey)
{
	TPlayerCache* pCacheData = GetPlayerCache(dwPlayerID, false);
	if (!pCacheData)
		return;

	TQuestCacheMap& rkMap = pCacheData->mapQuests;
	TQuestCacheMap::iterator it = rkMap.find(c_rkQuestKey);
	if (it == rkMap.end())
		return;

	CQuestCache* pCache = it->second;
	pCache->Delete();
}

void CClientManager::DeleteAffectCache(DWORD dwPlayerID, const AFFECT_KEY_TYPE& c_rkAffectKey)
{
	TPlayerCache* pCacheData = GetPlayerCache(dwPlayerID, false);
	if (!pCacheData)
		return;

	TAffectCacheMap& rkMap = pCacheData->mapAffects;
	TAffectCacheMap::iterator it = rkMap.find(c_rkAffectKey);
	if (it == rkMap.end())
		return;

	CAffectCache* pCache = it->second;
	pCache->Delete();
}

/*********************************
*** CACHE STORING - ITEM CACHE BY ID
*********************************/

CItemCache* CClientManager::GetItemCache(DWORD item_id)
{
	TItemCacheMap::iterator it = m_map_itemCacheByID.find(item_id);
	if (it == m_map_itemCacheByID.end())
		return NULL;

	return it->second;
}

bool CClientManager::DeleteItemCache(DWORD item_id)
{
	CItemCache* pItemCache = GetItemCache(item_id);
	if (!pItemCache)
		return false;

#ifdef __PET_ADVANCED__
	if (IsPetItem(pItemCache->Get()->vnum()))
		DeletePetCache(item_id);
#endif

	pItemCache->Delete();
	return true;
}

void CClientManager::EraseItemCache(DWORD item_id)
{
	CItemCache* pItemCache = GetItemCache(item_id);
	if (!pItemCache)
		return;

	// erase from owner set
	DWORD dwOwnerID = pItemCache->Get(false)->owner();
	TPlayerCache* pCacheData = GetPlayerCache(dwOwnerID, false);
	if (pCacheData)
		pCacheData->setItems.erase(pItemCache);

	// remove
	delete pItemCache;
	m_map_itemCacheByID.erase(item_id);
}

/*
 * PLAYER LOAD
 */
#ifdef __EQUIPMENT_CHANGER__
bool __EquipmentChanger_SortArray(const network::TEquipmentChangerTable& rkTab1, const network::TEquipmentChangerTable& rkTab2)
{
	return rkTab1.index() < rkTab2.index();
}
#endif

void CClientManager::QUERY_PLAYER_LOAD(CPeer * peer, DWORD dwHandle, network::GDPlayerLoadPacket* packet)
{
	CLoginData * pLoginData = GetLoginDataByAID(packet->account_id());

	if (pLoginData)
	{
		for (int n = 0; n < PLAYER_PER_ACCOUNT; ++n)
			if (pLoginData->GetAccountRef().players_size() > n && pLoginData->GetAccountRef().players(n).id() != 0)
				DeleteLogoutPlayer(pLoginData->GetAccountRef().players(n).id());
	}

	// load player
	TPlayerCache * pCacheData = GetPlayerCache(packet->player_id(), false);
	// from cache
	if (pCacheData && pCacheData->pPlayer)
	{
		CLoginData * pkLD = GetLoginDataByAID(packet->account_id());
		if (!pkLD || pkLD->IsPlay())
		{
			sys_log(0, "PLAYER_LOAD_ERROR: LoginData %p IsPlay %d", pkLD, pkLD ? pkLD->IsPlay() : 0);
			return;
		}

		pkLD->SetPlay(true);
#ifdef __DEPRECATED_BILLING__
		SendLoginToBilling(pkLD, true);
#endif

		//--------------------------------------------------------------
		// PLAYER DATA
		//--------------------------------------------------------------
		TPlayerTable* pTab = pCacheData->pPlayer->Get();
		auto premiumTimes = pTab->mutable_premium_times();
		for (int i = 0; i < PREMIUM_MAX_NUM; ++i)
		{
			if (i >= premiumTimes->size())
				premiumTimes->Add();
			premiumTimes->Set(i, pkLD->GetPremium(i));
		}

		network::DGOutputPacket<network::DGPlayerLoadPacket> p;
		*p->mutable_player() = *pTab;
		peer->Packet(p, dwHandle);

		if (packet->player_id() != pkLD->GetLastPlayerID())
		{
//			TPacketNeedLoginLogInfo logInfo;
//			logInfo.dwPlayerID = packet->player_id();

			pkLD->SetLastPlayerID(packet->player_id());

//			peer->EncodeHeader(HEADER_DG_NEED_LOGIN_LOG, dwHandle, sizeof(TPacketNeedLoginLogInfo));
//			peer->Encode(&logInfo, sizeof(TPacketNeedLoginLogInfo));
		}

		//--------------------------------------------------------------
		// ITEM DATA
		//--------------------------------------------------------------
		{
			TItemCacheSet& rkItemSet = pCacheData->setItems;

			network::DGOutputPacket<network::DGItemLoadPacket> item_load;
			item_load->set_pid(pTab->id());

#ifdef __PET_ADVANCED__
			network::DGOutputPacket<network::DGPetLoadPacket> pet_load;
#endif

			for (CItemCache* c : rkItemSet)
			{
				if (c->IsDisabled())
					continue;

				network::TItemData* p = c->Get();

				if (p->vnum()) // vnumAI ¨ú©ªA¢¬¢¬e ¡íeA|¥ìE ¨ú¨¡AIAUAI¢¥U.
				{
					*item_load->add_items() = *p;

#ifdef __PET_ADVANCED__
					if (IsPetItem(p->vnum()))
					{
						auto petTable = RequestPetDataForItem(p->id(), peer);
						if (petTable)
							*pet_load->add_pets() = *petTable;
					}
#endif
				}
			}

			peer->Packet(item_load, dwHandle);

#ifdef __PET_ADVANCED__
			peer->Packet(pet_load, dwHandle);
#endif
		}

		//--------------------------------------------------------------
		// QUEST DATA
		//--------------------------------------------------------------
		{
			TQuestCacheMap& rkQuestMap = pCacheData->mapQuests;

			DGOutputPacket<DGQuestLoadPacket> quest_load;
			quest_load->set_pid(pTab->id());

			for (auto it : rkQuestMap)
			{
				TQuestTable * p = it.second->Get();

				if (p->value())
					*quest_load->add_quests() = *p;
			}


			peer->Packet(quest_load, dwHandle);
		}

		//--------------------------------------------------------------
		// AFFECT DATA
		//--------------------------------------------------------------
		{
			TAffectCacheMap& rkAffectMap = pCacheData->mapAffects;

			DGOutputPacket<DGAffectLoadPacket> affect_load;
			affect_load->set_pid(pTab->id());
			for (auto& it : rkAffectMap)
				*affect_load->add_affects() = it.second->Get()->elem();

			peer->Packet(affect_load, dwHandle);
		}
	}
	// from database
	else
	{
		sys_log(0, "[PLAYER_LOAD] Load from PlayerDB pid[%d]", packet->player_id());

		char queryStr[QUERY_MAX_LEN];
		int iQueryLen;

		//--------------------------------------------------------------
		// PLAYER DATA
		//--------------------------------------------------------------
		iQueryLen = snprintf(queryStr, sizeof(queryStr),
			"SELECT "
			"id,player.name,job,voice,dir,player.x,player.y,player.map_index,exit_x,exit_y,exit_map_index,hp,mp,"
			"playtime,player.gold,player.level,level_step,st,ht,dx,iq,exp,"
			"stat_point,skill_point,sub_skill_point,stat_reset_count,part_base,part_hair,"
			"skill_level,quickslot,skill_group,alignment,"
			"mount.state-1, mount.name, mount.item_id, horse_grade, horse_elapsed_time, "
			"horse_skill_point, IFNULL(UNIX_TIMESTAMP(NOW())-UNIX_TIMESTAMP(last_play), 0), inventory_max_num, uppitem_inv_max_num, "
			"skillbook_inv_max_num, stone_inv_max_num, enchant_inv_max_num"
			);
#ifdef __HAIR_SELECTOR__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.part_hair_base");
#endif
#ifdef __ACCE_COSTUME__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.part_acce");
#endif
#ifdef __PRESTIGE__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.prestige_level");
#endif
#ifdef __GAYA_SYSTEM__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.gaya");
#endif
#ifdef ENABLE_ZODIAC_TEMPLE
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.animasphere");
#endif
#ifdef __FAKE_BUFF__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.buffiskill1, player.buffiskill2, player.buffiskill3");
#endif
#ifdef __ATTRTREE__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", GROUP_CONCAT(attrtree.row), GROUP_CONCAT(attrtree.col), GROUP_CONCAT(attrtree.level)");
#endif
#ifdef ENABLE_RUNE_SYSTEM
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ",rune.vnumlist,rune.pagedata");
#endif	
#ifdef __EQUIPMENT_CHANGER__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.equipment_page_index");
#endif
#ifdef COMBAT_ZONE
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, ", player.combat_zone_points");
#endif
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, " FROM player");
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, " LEFT JOIN player_mount AS mount ON mount.pid = player.id");
#ifdef __ATTRTREE__
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, " LEFT JOIN attrtree ON attrtree.pid = player.id");
#endif
#ifdef ENABLE_RUNE_SYSTEM
		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen,
			" LEFT JOIN rune ON rune.pid = player.id");
#endif

		iQueryLen += snprintf(queryStr + iQueryLen, sizeof(queryStr) - iQueryLen, " WHERE id = %d", packet->player_id());

		ClientHandleInfo * pkInfo = new ClientHandleInfo(dwHandle, packet->player_id());
		pkInfo->account_id = packet->account_id();
		CDBManager::instance().ReturnQuery(queryStr, QID_PLAYER, peer->GetHandle(), pkInfo, SQL_PLAYER_STUFF_LOAD);

		//--------------------------------------------------------------
		// ITEM DATA
		//--------------------------------------------------------------
		snprintf(queryStr, sizeof(queryStr),
				"SELECT %s "
				"FROM item WHERE owner_id=%u AND (window < %d OR window = %d OR window = %d OR window = %d)",
				GetItemQueryKeyPart(true), packet->player_id(), SAFEBOX, ENCHANT_INVENTORY, COSTUME_INVENTORY, RECOVERY_INVENTORY);
		if (g_test_server)
			sys_log(0, "ITEM_QUERY: %s", queryStr);
		CDBManager::instance().ReturnQuery(queryStr, QID_ITEM, peer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id()), SQL_PLAYER_STUFF_LOAD);

		//--------------------------------------------------------------
		// QUEST DATA
		//--------------------------------------------------------------
		snprintf(queryStr, sizeof(queryStr),
				"SELECT dwPID,szName,szState,lValue FROM quest WHERE dwPID=%d",
				packet->player_id());
		CDBManager::instance().ReturnQuery(queryStr, QID_QUEST, peer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id(), packet->account_id()), SQL_PLAYER_STUFF_LOAD);

		//--------------------------------------------------------------
		// AFFECT DATA
		//--------------------------------------------------------------
		snprintf(queryStr, sizeof(queryStr),
				"SELECT dwPID,bType,bApplyOn,lApplyValue,dwFlag,lDuration,lSPCost FROM affect WHERE dwPID=%d",
				packet->player_id());
		CDBManager::instance().ReturnQuery(queryStr, QID_AFFECT, peer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id()), SQL_PLAYER_STUFF_LOAD);
	}

	//--------------------------------------------------------------
	// OFFLINE MESSAGES
	//--------------------------------------------------------------
	char queryStr[1024];
	snprintf(queryStr, sizeof(queryStr),
			"SELECT sender, message, is_gm FROM offline_messages WHERE pid=%d ORDER BY date ASC",
			packet->player_id());
	CDBManager::instance().ReturnQuery(queryStr, QID_OFFLINE_MESSAGES, peer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id()));

#ifdef __ITEM_REFUND__
	//--------------------------------------------------------------
	// ITEM REFUNDS
	//--------------------------------------------------------------
	QUERY_ITEM_REFUND(peer, dwHandle, packet->player_id());
#endif
}

void CClientManager::ItemAward(CPeer * peer, const char* login)
{
	char login_t[LOGIN_MAX_LEN + 1] = "";
	strlcpy(login_t,login,LOGIN_MAX_LEN + 1);	
	std::set<TItemAward *> * pSet = ItemAwardManager::instance().GetByLogin(login_t);	
	if(pSet == NULL)
		return;
	typeof(pSet->begin()) it = pSet->begin();	//taken_timeÀÌ NULLÀÎ°Íµé ÀÐ¾î¿È	
	while(it != pSet->end() )
	{				
		TItemAward * pItemAward = *(it++);		
		char* whyStr = pItemAward->szWhy;	//why ÄÝ·ë ÀÐ±â
		char cmdStr[100] = "";	//whyÄÝ·ë¿¡¼­ ÀÐÀº °ªÀ» ÀÓ½Ã ¹®ÀÚ¿­¿¡ º¹»çÇØµÒ
		strcpy(cmdStr,whyStr);	//¸í·É¾î ¾ò´Â °úÁ¤¿¡¼­ ÅäÅ«¾²¸é ¿øº»µµ ÅäÅ«È­ µÇ±â ¶§¹®
		char command[20] = "";
		strcpy(command,GetCommand(cmdStr));	// command ¾ò±â		
		if( !(strcmp(command,"GIFT") ))	// command °¡ GIFTÀÌ¸é
		{
			DGOutputPacket<DGItemAwardInformerPacket> giftData;
			giftData->set_login(pItemAward->szLogin);
			giftData->set_command(command);
			giftData->set_vnum(pItemAward->dwVnum);
			ForwardPacket(giftData);
		}
	}
}
char* CClientManager::GetCommand(char* str)
{
	char command[20] = "";
	char* tok;

	if( str[0] == '[' )
	{
		tok = strtok(str,"]");			
		strcat(command,&tok[1]);		
	}

	return command;
}

std::vector<int>* ParseStringToTokens(const char* c_pszString, char cDelim = ',')
{
	std::vector<int>* pVec = new std::vector<int>();
	if (!c_pszString)
		return pVec;

	std::stringstream ss(c_pszString);
	int tmp;

	while (ss >> tmp)
	{
		pVec->push_back(tmp);
		if (ss.peek() == ',')
			ss.ignore();
	}

	return pVec;
}

bool CreatePlayerTableFromRes(MYSQL_RES * res, TPlayerTable * pkTab)
{
	if (!res || !pkTab || mysql_num_rows(res) == 0)	// µ¥ÀÌÅÍ ¾øÀ½
		return false;

	pkTab->Clear();

	MYSQL_ROW row = mysql_fetch_row(res);
	unsigned long* pColLengths = mysql_fetch_lengths(res);

	int	col = 0;
	if (!row[col] || !pColLengths[col])
		return false;

	for (int i = 0; i < PART_MAX_NUM; ++i)
		pkTab->add_parts(0);

	for (int i = 0; i < PC_TAB_CHANGED_MAX_NUM; ++i)
		pkTab->add_data_changed(false);

	// "id,name,job,voice,dir,x,y,map_index,exit_x,exit_y,exit_map_index,hp,mp,playtime,"
	// "gold,level,level_step,st,ht,dx,iq,exp,"
	// "stat_point,skill_point,sub_skill_point,stat_reset_count,part_base,part_hair,"
	// "skill_level,quickslot,skill_group,alignment,"
	// "horse_level,horse_name,horse_riding,horse_hp,horse_stamina FROM player%s WHERE id=%d",
	pkTab->set_id(std::stoll(row[col++]));
	if (!row[col] || !pColLengths[col])
		return false;
	pkTab->set_name(row[col++]);
	pkTab->set_job(std::stoll(row[col++]));
	pkTab->set_voice(std::stoll(row[col++]));
	pkTab->set_dir(std::stoll(row[col++]));
	pkTab->set_x(std::stoll(row[col++]));
	pkTab->set_y(std::stoll(row[col++]));
	pkTab->set_map_index(std::stoll(row[col++]));
	pkTab->set_exit_x(std::stoll(row[col++]));
	pkTab->set_exit_y(std::stoll(row[col++]));
	pkTab->set_exit_map_index(std::stoll(row[col++]));
	pkTab->set_hp(std::stoll(row[col++]));
	pkTab->set_sp(std::stoll(row[col++]));
	pkTab->set_playtime(std::stoll(row[col++]));
	pkTab->set_gold(std::stoll(row[col++]));
	pkTab->set_level(std::stoll(row[col++]));
	pkTab->set_level_step(std::stoll(row[col++]));
	pkTab->set_st(std::stoll(row[col++]));
	pkTab->set_ht(std::stoll(row[col++]));
	pkTab->set_dx(std::stoll(row[col++]));
	pkTab->set_iq(std::stoll(row[col++]));
	pkTab->set_exp(std::stoll(row[col++]));
	pkTab->set_stat_point(std::stoll(row[col++]));
	pkTab->set_skill_point(std::stoll(row[col++]));
	pkTab->set_sub_skill_point(std::stoll(row[col++]));
	pkTab->set_stat_reset_count(std::stoll(row[col++]));
	pkTab->set_part_base(std::stoll(row[col++]));
	pkTab->set_parts(PART_HAIR, std::stoll(row[col++]));

	if (row[col])
	{
		ParseProtobufRepeatedPtrField(row[col], pColLengths[col], pkTab->mutable_skills());

		while (pkTab->skills_size() < SKILL_MAX_NUM)
			pkTab->add_skills();
	}
	else
	{
		for (int i = 0; i < SKILL_MAX_NUM; ++i)
			pkTab->add_skills();
	}

	col++;

	if (row[col])
	{	ParseProtobufRepeatedPtrField(row[col], pColLengths[col], pkTab->mutable_quickslots());

		while (pkTab->quickslots_size() < QUICKSLOT_MAX_NUM)
			pkTab->add_quickslots();
	}
	else
	{
		for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
			pkTab->add_quickslots();
	}

	col++;

	pkTab->set_skill_group(std::stoll(row[col++]));
	pkTab->set_alignment(std::stoll(row[col++]));

	if (row[col])
		pkTab->set_mount_state(std::stoll(row[col]));
	++col;
	if (row[col])
		pkTab->set_mount_name(row[col]);
	++col;
	if (row[col])
		pkTab->set_mount_item_id(std::stoll(row[col]));
	++col;
	if (row[col])
		pkTab->set_horse_grade(std::stoll(row[col]));
	++col;
	if (row[col])
		pkTab->set_horse_elapsed_time(std::stoll(row[col]));
	++col;
	if (row[col])
		pkTab->set_horse_skill_point(std::stoll(row[col]));
	++col;
	pkTab->set_logoff_interval(std::stoll(row[col++]));
	pkTab->set_inventory_max_num(std::stoll(row[col++]));
	pkTab->set_uppitem_inv_max_num(std::stoll(row[col++]));
	pkTab->set_skillbook_inv_max_num(std::stoll(row[col++]));
	pkTab->set_stone_inv_max_num(std::stoll(row[col++]));
	pkTab->set_enchant_inv_max_num(std::stoll(row[col++]));

#ifdef __HAIR_SELECTOR__
	pkTab->set_part_hair_base(std::stoll(row[col++]));
#endif
#ifdef __ACCE_COSTUME__
	pkTab->set_parts(PART_ACCE, std::stoll(row[col++]));
#endif
#ifdef __PRESTIGE__
	pkTab->set_prestige(std::stoll(row[col++]));
#endif

#ifdef __GAYA_SYSTEM__
	pkTab->set_gaya(std::stoll(row[col++]));
#endif

#ifdef ENABLE_ZODIAC_TEMPLE
	pkTab->set_animasphere(std::stoll(row[col++]));
#endif
#ifdef __FAKE_BUFF__
	pkTab->set_fakebuff_skill1(std::stoll(row[col++]));
	pkTab->set_fakebuff_skill2(std::stoll(row[col++]));
	pkTab->set_fakebuff_skill3(std::stoll(row[col++]));
#endif

#ifdef __ATTRTREE__
	std::auto_ptr<std::vector<int> > vecAttrtreeRows(ParseStringToTokens(row[col++]));
	std::auto_ptr<std::vector<int> > vecAttrtreeCols(ParseStringToTokens(row[col++]));
	std::auto_ptr<std::vector<int> > vecAttrtreeLevels(ParseStringToTokens(row[col++]));

	for (int i = 0; i < vecAttrtreeRows->size(); ++i)
	{
		BYTE row = (*vecAttrtreeRows)[i];
		BYTE col = (*vecAttrtreeCols)[i];
		BYTE level = (*vecAttrtreeLevels)[i];

		pkTab->attrtree[row][col] = level;
	}
#endif

#ifdef ENABLE_RUNE_SYSTEM
	if (row[col])
	{
		ParseProtobufRepeatedField(row[col], pColLengths[col], pkTab->mutable_runes());
	}
	++col;

	if (row[col])
		pkTab->mutable_rune_page_data()->ParseFromArray(row[col], pColLengths[col]);
	++col;
#endif

#ifdef __EQUIPMENT_CHANGER__
	pkTab->set_equipment_page_index(std::stoll(row[col++]));
#endif
#ifdef COMBAT_ZONE
	pkTab->set_combat_zone_points(std::stoll(row[col++]));
#endif

	// reset sub_skill_point
	{
		pkTab->mutable_skills(123)->set_level(0); // SKILL_CREATE

		if (pkTab->level() > 9)
		{
			int max_point = pkTab->level() - 9;

			int skill_point = 
				MIN(20, pkTab->skills(121).level()) +	// SKILL_LEADERSHIP			Åë¼Ö·Â
				MIN(20, pkTab->skills(124).level()) +	// SKILL_MINING				Ã¤±¤
				MIN(10, pkTab->skills(131).level()) +	// SKILL_HORSE_SUMMON		¸»¼ÒÈ¯
				MIN(20, pkTab->skills(141).level()) +	// SKILL_ADD_HP				HPº¸°­
				MIN(20, pkTab->skills(142).level());		// SKILL_RESIST_PENETRATE	°üÅëÀúÇ×

			pkTab->set_sub_skill_point(max_point - skill_point);
		}
		else
			pkTab->set_sub_skill_point(0);
	}

	return true;
}

#ifdef CHANGE_SKILL_COLOR
void CClientManager::QUERY_SKILL_COLOR_LOAD(CPeer * peer, DWORD dwHandle, GDPlayerLoadPacket* packet)
{
	if (auto c = GetSkillColorCache(packet->player_id()))
	{
		auto p = c->Get();

		DGOutputPacket<DGSkillColorLoadPacket> pack;
		for (auto skill_idx = 0; skill_idx < ESkillColorLength::MAX_SKILL_COUNT; ++skill_idx)
		{
			auto colors = pack->add_colors();

			for (auto eff_idx = 0; eff_idx < ESkillColorLength::MAX_EFFECT_COUNT; ++eff_idx)
			{
				auto cur_idx = skill_idx * ESkillColorLength::MAX_EFFECT_COUNT + eff_idx;
				colors->add_colors(p->skill_colors(cur_idx));
			}
		}

		peer->Packet(pack, dwHandle);
	}
	else
	{
		char szQuery[1024];
		snprintf(szQuery, sizeof(szQuery),
			"SELECT s1_col1,s1_col2,s1_col3,s1_col4,s1_col5,s2_col1,s2_col2,s2_col3,s2_col4,s2_col5,"
			"s3_col1,s3_col2,s3_col3,s3_col4,s3_col5,s4_col1,s4_col2,s4_col3,s4_col4,s4_col5,"
			"s5_col1,s5_col2,s5_col3,s5_col4,s5_col5,s6_col1,s6_col2,s6_col3,s6_col4,s6_col5"
#ifdef __FAKE_BUFF__
			",s7_col1,s7_col2,s7_col3,s7_col4,s7_col5"
			",s8_col1,s8_col2,s8_col3,s8_col4,s8_col5"
			",s9_col1,s9_col2,s9_col3,s9_col4,s9_col5"
#endif
			" FROM skill_color WHERE player_id=%d",
			packet->player_id());
		CDBManager::instance().ReturnQuery(szQuery, QID_SKILL_COLOR, peer->GetHandle(), new ClientHandleInfo(dwHandle, packet->player_id()));
	}
}
#endif

#ifdef __EQUIPMENT_CHANGER__
void CClientManager::QUERY_EQUIPMENT_CHANGER_LOAD(CPeer * peer, DWORD dwHandle, GDPlayerLoadPacket* packet)
{
	TEquipmentPageCacheSet * pEquipmentPageSet;
	if (pEquipmentPageSet = GetEquipmentPageCacheSet(packet->player_id()))
	{
		// create pages
		static TEquipmentChangerTable* s_apkEquipPages = NULL;
		if (pEquipmentPageSet->size())
			s_apkEquipPages = new TEquipmentChangerTable[pEquipmentPageSet->size()];

		DWORD dwCount = 0;
		TEquipmentPageCacheSet::iterator it = pEquipmentPageSet->begin();

		while (it != pEquipmentPageSet->end())
		{
			CEquipmentPageCache * c = *it++;
			TEquipmentChangerTable * p = c->Get();

			if (*p->page_name().c_str())
				s_apkEquipPages[dwCount++] = *p;
		}

		if (dwCount)
			std::sort(s_apkEquipPages, s_apkEquipPages + dwCount, __EquipmentChanger_SortArray);

		sys_log(!g_test_server, "EQUIPMENT_PAGE_CACHE: HIT! [%d] count: %u sent count: %u", packet->player_id(), pEquipmentPageSet->size(), dwCount);

		DGOutputPacket<DGEquipmentPageLoadPacket> pack;
		pack->set_pid(packet->player_id());
		for (auto i = 0; i < dwCount; ++i)
			*pack->add_equipments() = s_apkEquipPages[i];

		peer->Packet(pack, dwHandle);

		// free pages
		if (pEquipmentPageSet->size())
			delete[] s_apkEquipPages;
		s_apkEquipPages = NULL;
	}
	else
	{
		char szQuery[1024] = { 0, };
		// Equipment Pages
		snprintf(szQuery, sizeof(szQuery),
				"SELECT pid,page_index,page_name,rune_page_index,"
					"EQUIPMENT_CHANGER_WEAR_BODY,"
					"EQUIPMENT_CHANGER_WEAR_HEAD,"
					"EQUIPMENT_CHANGER_WEAR_FOOTS,"
					"EQUIPMENT_CHANGER_WEAR_WRIST,"
					"EQUIPMENT_CHANGER_WEAR_WEAPON,"
					"EQUIPMENT_CHANGER_WEAR_NECK,"
					"EQUIPMENT_CHANGER_WEAR_EAR,"
					"EQUIPMENT_CHANGER_WEAR_ARROW,"
					"EQUIPMENT_CHANGER_WEAR_SHIELD,"
					"EQUIPMENT_CHANGER_WEAR_COSTUME_BODY,"
					"EQUIPMENT_CHANGER_WEAR_COSTUME_HAIR,"
					"EQUIPMENT_CHANGER_WEAR_COSTUME_WEAPON,"
					"EQUIPMENT_CHANGER_WEAR_ACCE,"
					"EQUIPMENT_CHANGER_WEAR_BELT,"
					"EQUIPMENT_CHANGER_WEAR_TOTEM "

				"FROM equipment_page "
				"WHERE pid=%u ORDER BY page_index ASC",
				packet->player_id());
		CDBManager::instance().ReturnQuery(szQuery,
				QID_EQUIPMENT_PAGE,
				peer->GetHandle(),
				new ClientHandleInfo(dwHandle, packet->player_id()),
				SQL_PLAYER_STUFF_LOAD);
	}
}
#endif

void CClientManager::RESULT_COMPOSITE_PLAYER(CPeer * peer, SQLMsg * pMsg, DWORD dwQID)
{
	CQueryInfo * qi = (CQueryInfo *) pMsg->pvUserData;
	std::auto_ptr<ClientHandleInfo> info((ClientHandleInfo *) qi->pvData);
	
	MYSQL_RES * pSQLResult = pMsg->Get()->pSQLResult;
	if (!pSQLResult)
	{
		sys_err("null MYSQL_RES QID %u", dwQID);
		return;
	}

	switch (dwQID)
	{
		case QID_PLAYER:
			if (info->player_id <= 0)
			{
				sys_err("PREVENT CRASH HERE");
				break;
			}
			sys_log(0, "QID_PLAYER %u %u", info->dwHandle, info->player_id);
			RESULT_PLAYER_LOAD(peer, pSQLResult, info.get());

			break;

		case QID_ITEM:
			sys_log(0, "QID_ITEM %u", info->dwHandle);
			RESULT_ITEM_LOAD(peer, pSQLResult, info->dwHandle, info->player_id);
			break;

		case QID_QUEST:
			{
				sys_log(0, "QID_QUEST %u", info->dwHandle);
				RESULT_QUEST_LOAD(peer, pSQLResult, info->dwHandle, info->player_id);
				//aid¾ò±â
				ClientHandleInfo*  temp1 = info.get();
				if (temp1 == NULL)
					break;
				
				CLoginData* pLoginData1 = GetLoginDataByAID(temp1->account_id);	//				
				//µ¶ÀÏ ¼±¹° ±â´É
				if( pLoginData1->GetAccountRef().login().empty())
					break;
				if( pLoginData1 == NULL )
					break;
				sys_log(0,"info of pLoginData1 before call ItemAwardfunction %d",pLoginData1);
				ItemAward(peer,pLoginData1->GetAccountRef().login().c_str());
			}
			break;

		case QID_AFFECT:
			sys_log(0, "QID_AFFECT %u", info->dwHandle);
			RESULT_AFFECT_LOAD(peer, pSQLResult, info->dwHandle, info->player_id);
			break;

		case QID_OFFLINE_MESSAGES:
			sys_log(0, "QID_OFFLINE_MESSAGES %u", info->dwHandle);
			RESULT_OFFLINE_MESSAGES_LOAD(peer, pSQLResult, info->dwHandle, info->player_id);
			break;

#ifdef __ITEM_REFUND__
		case QID_ITEM_REFUND:
			sys_log(0, "QID_ITEM_REFUND %u", info->dwHandle);
			RESULT_ITEM_REFUND(peer, pSQLResult, info->dwHandle, info->player_id);
			break;
#endif

			/*
			   case QID_PLAYER_ITEM_QUEST_AFFECT:
			   sys_log(0, "QID_PLAYER_ITEM_QUEST_AFFECT %u", info->dwHandle);
			   RESULT_PLAYER_LOAD(peer, pSQLResult, info->dwHandle);

			   if (!pMsg->Next())
			   {
			   sys_err("RESULT_COMPOSITE_PLAYER: QID_PLAYER_ITEM_QUEST_AFFECT: ITEM FAILED");
			   return;
			   }

			   case QID_ITEM_QUEST_AFFECT:
			   sys_log(0, "QID_ITEM_QUEST_AFFECT %u", info->dwHandle);
			   RESULT_ITEM_LOAD(peer, pSQLResult, info->dwHandle, info->player_id);

			   if (!pMsg->Next())
			   {
			   sys_err("RESULT_COMPOSITE_PLAYER: QID_PLAYER_ITEM_QUEST_AFFECT: QUEST FAILED");
			   return;
			   }

			   case QID_QUEST_AFFECT:
			   sys_log(0, "QID_QUEST_AFFECT %u", info->dwHandle);
			   RESULT_QUEST_LOAD(peer, pSQLResult, info->dwHandle);

			   if (!pMsg->Next())
			   sys_err("RESULT_COMPOSITE_PLAYER: QID_PLAYER_ITEM_QUEST_AFFECT: AFFECT FAILED");
			   else
			   RESULT_AFFECT_LOAD(peer, pSQLResult, info->dwHandle);

			   break;
			   */

#ifdef CHANGE_SKILL_COLOR
		case QID_SKILL_COLOR:
			sys_log(0, "QID_SKILL_COLOR %u  %u", info->dwHandle, info->player_id);
			RESULT_SKILL_COLOR_LOAD(peer, pSQLResult, info->dwHandle);
			break;
#endif
#ifdef __EQUIPMENT_CHANGER__
		case QID_EQUIPMENT_PAGE:
			sys_log(0, "QID_EQUIPMENT_PAGE %u %u", info->dwHandle, info->player_id);
			RESULT_EQUIPMENT_PAGE_LOAD(peer, pSQLResult, info->dwHandle, info->player_id);
			break;
#endif
	}
	
}

#ifdef CHANGE_SKILL_COLOR
void CClientManager::RESULT_SKILL_COLOR_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle)
{
	DWORD dwSkillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memset(dwSkillColor, 0, sizeof(dwSkillColor));

	CreateSkillColorTableFromRes(pRes, *dwSkillColor);
	
	DGOutputPacket<DGSkillColorLoadPacket> pack;
	for (auto skill_idx = 0; skill_idx < ESkillColorLength::MAX_SKILL_COUNT; ++skill_idx)
	{
		auto colors = pack->add_colors();
		for (auto eff_idx = 0; eff_idx < ESkillColorLength::MAX_EFFECT_COUNT; ++eff_idx)
		{
			colors->add_colors(dwSkillColor[skill_idx][eff_idx]);
		}
	}

	peer->Packet(pack, dwHandle);
}
#endif

void CClientManager::RESULT_PLAYER_LOAD(CPeer * peer, MYSQL_RES * pRes, ClientHandleInfo * pkInfo)
{
	TPlayerTable tab;

	if (!CreatePlayerTableFromRes(pRes, &tab))
		return;

	CLoginData * pkLD = GetLoginDataByAID(pkInfo->account_id);
	
	if (!pkLD || pkLD->IsPlay())
	{
		sys_log(0, "PLAYER_LOAD_ERROR: LoginData %p IsPlay %d", pkLD, pkLD ? pkLD->IsPlay() : 0);
		return;
	}

	pkLD->SetPlay(true);
#ifdef __DEPRECATED_BILLING__
	SendLoginToBilling(pkLD, true);
#endif
	for (int i = 0; i < PREMIUM_MAX_NUM; ++i)
		tab.add_premium_times(pkLD->GetPremium(i));

	DGOutputPacket<DGPlayerLoadPacket> pack;
	*pack->mutable_player() = tab;
	peer->Packet(pack, pkInfo->dwHandle);

	if (tab.id() != pkLD->GetLastPlayerID())
	{
//		TPacketNeedLoginLogInfo logInfo;
//		logInfo.dwPlayerID = tab.id;

		pkLD->SetLastPlayerID( tab.id() );

//		peer->EncodeHeader( HEADER_DG_NEED_LOGIN_LOG, pkInfo->dwHandle, sizeof(TPacketNeedLoginLogInfo) );
//		peer->Encode( &logInfo, sizeof(TPacketNeedLoginLogInfo) );
	}

	PutPlayerCache(&tab, true);
}

void CClientManager::RESULT_ITEM_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID)
{
	DGOutputPacket<DGItemLoadPacket> pack;
	pack->set_pid(dwPID);

	CreateItemTableFromRes(pRes, [&pack]() {
		return pack->add_items();
	}, dwPID, 0);

	peer->Packet(pack, dwHandle);

	// ITEM_LOAD_LOG_ATTACH_PID
	sys_log(0, "ITEM_LOAD: count %u pid %u", pack->items_size(), dwPID);
	// END_OF_ITEM_LOAD_LOG_ATTACH_PID
	
	if (pack->items_size())
	{
#ifdef __PET_ADVANCED__
		network::DGOutputPacket<network::DGPetLoadPacket> pet_pack;
#endif

		for (auto& item : pack->items())
		{
			PutItemCache(&item, true);

#ifdef __PET_ADVANCED__
			if (IsPetItem(item.vnum()))
			{
				auto petTable = RequestPetDataForItem(item.id(), peer);
				if (petTable)
					*pet_pack->add_pets() = *petTable;
			}
#endif
		}

#ifdef __PET_ADVANCED__
		if (pet_pack->pets_size() > 0)
			peer->Packet(pet_pack, dwHandle);
#endif
	}
}

void CClientManager::RESULT_AFFECT_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID)
{
	int iNumRows = mysql_num_rows(pRes);
	DWORD dwCount = iNumRows;
	
	DGOutputPacket<DGAffectLoadPacket> pack;
	pack->set_pid(dwPID);

	MYSQL_ROW row;

	for (int i = 0; i < iNumRows; ++i)
	{
		auto aff = pack->add_affects();
		row = mysql_fetch_row(pRes);
		int col = 1;

		aff->set_type(std::stoll(row[col++]));
		aff->set_apply_on(std::stoll(row[col++]));
		aff->set_apply_value(std::stoll(row[col++]));
		aff->set_flag(std::stoll(row[col++]));
		aff->set_duration(std::stoll(row[col++]));
		aff->set_sp_cost(std::stoll(row[col++]));
		
		PutAffectCache(dwPID, aff, true);
	}

	sys_log(0, "AFFECT_LOAD: count %d PID %u", iNumRows, dwPID);

	peer->Packet(pack, dwHandle);
}

void CClientManager::RESULT_OFFLINE_MESSAGES_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID)
{
	int iNumRows;

	if ((iNumRows = mysql_num_rows(pRes)) == 0) // µ¥ÀÌÅÍ ¾øÀ½
		return;

	DGOutputPacket<DGOfflineMessagesLoadPacket> pack;
	pack->set_pid(dwPID);

	MYSQL_ROW row;

	for (int i = 0; i < iNumRows; ++i)
	{
		auto msg = pack->add_messages();
		row = mysql_fetch_row(pRes);

		msg->set_sender(row[0]);
		msg->set_message(row[1]);
		msg->set_is_gm(row[2] != nullptr);
	}

	sys_log(0, "OFFLINE_MESSAGES_LOAD: count %d PID %u", iNumRows, dwPID);

	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM offline_messages WHERE pid = %u", dwPID);
	CDBManager::instance().AsyncQuery(szQuery);

	pkPeer->Packet(pack, dwHandle);
}

void CClientManager::RESULT_QUEST_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD pid)
{
	int iNumRows;

	DGOutputPacket<DGQuestLoadPacket> pack;
	pack->set_pid(pid);

	if ((iNumRows = mysql_num_rows(pRes)) == 0)
	{ 
		peer->Packet(pack, dwHandle);
		return;
	}

	MYSQL_ROW row;

	for (int i = 0; i < iNumRows; ++i)
	{
		auto quest = pack->add_quests();

		row = mysql_fetch_row(pRes);

		quest->set_pid(std::stoll(row[0]));
		quest->set_name(row[1]);
		quest->set_state(row[2]);
		quest->set_value(std::stoll(row[3]));

		PutQuestCache(quest, true);
	}

	sys_log(0, "QUEST_LOAD: count %d PID %u", iNumRows, pid);

	DWORD dwCount = iNumRows;

	peer->Packet(pack, dwHandle);
}

#ifdef __EQUIPMENT_CHANGER__
void CClientManager::RESULT_EQUIPMENT_PAGE_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID)
{
	int iNumRows = mysql_num_rows(pRes);

	DGOutputPacket<DGEquipmentPageLoadPacket> pack;
	pack->set_pid(dwPID);

	MYSQL_ROW row;

	for (int i = 0; i < iNumRows; ++i)
	{
		auto equip = pack->add_equipments();
		row = mysql_fetch_row(pRes);

		int col = 0;

		equip->set_pid(std::stoll(row[col++]));
		equip->set_index(std::stoll(row[col++]));
		equip->set_page_name(row[col++]);
		equip->set_rune_page(std::stoll(row[col++]));

		for (int j = 0; j < EQUIPMENT_PAGE_MAX_PARTS; j++)
			equip->add_item_ids(std::stoll(row[col++]));

		PutEquipmentPageCache(equip, true);
	}

	CreateEquipmentPageCacheSet(dwPID);

	sys_log(0, "EQUIPMENT_PAGE_LOAD: count %d PID %u", iNumRows, dwPID);

	peer->Packet(pack, dwHandle);
}
#endif


/*
 * PLAYER SAVE
 */
void CClientManager::QUERY_PLAYER_SAVE(CPeer * peer, DWORD dwHandle, std::unique_ptr<GDPlayerSavePacket> pack)
{
	if (g_test_server)
		sys_log(0, "PLAYER_SAVE: %s", pack->data().name().c_str());

	PutPlayerCache(&pack->data());
}

typedef std::map<DWORD, time_t> time_by_id_map_t;
static time_by_id_map_t s_createTimeByAccountID;

/*
 * PLAYER CREATE
 */
void CClientManager::__QUERY_PLAYER_CREATE(CPeer *peer, DWORD dwHandle, std::unique_ptr<GDPlayerCreatePacket> packet)
{
	char	queryStr[QUERY_MAX_LEN];
	int		queryLen;
	int		player_id;

	// ÇÑ °èÁ¤¿¡ XÃÊ ³»·Î Ä³¸¯ÅÍ »ý¼ºÀ» ÇÒ ¼ö ¾ø´Ù.
	time_by_id_map_t::iterator it = s_createTimeByAccountID.find(packet->account_id());

	if (it != s_createTimeByAccountID.end())
	{
		time_t curtime = time(0);

		if (curtime - it->second < 30)
		{
			peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
			return;
		}
	}

	queryLen = snprintf(queryStr, sizeof(queryStr), 
			"SELECT pid%u FROM player_index WHERE id=%d", packet->account_index() + 1, packet->account_id());

	std::auto_ptr<SQLMsg> pMsg0(CDBManager::instance().DirectQuery(queryStr));

	if (pMsg0->Get()->uiNumRows != 0)
	{
		if (!pMsg0->Get()->pSQLResult)
		{
			peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg0->Get()->pSQLResult);

		DWORD dwPID = 0; str_to_number(dwPID, row[0]);
		if (row[0] && dwPID > 0)
		{
			peer->Packet(TDGHeader::PLAYER_CREATE_ALREADY, dwHandle);
			sys_log(0, "ALREADY EXIST AccountChrIdx %d ID %d", packet->account_index(), dwPID);
			return;
		}
	}
	else
	{
		peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
		return;
	}

	snprintf(queryStr, sizeof(queryStr), 
			"SELECT COUNT(*) as count FROM player WHERE name='%s'", packet->player_table().name().c_str());

	std::auto_ptr<SQLMsg> pMsg1(CDBManager::instance().DirectQuery(queryStr));

	if (pMsg1->Get()->uiNumRows)
	{
		if (!pMsg1->Get()->pSQLResult)
		{
			peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
			return;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg1->Get()->pSQLResult);

		if (*row[0] != '0')
		{
			sys_log(0, "ALREADY EXIST name %s, row[0] %s query %s", packet->player_table().name().c_str(), row[0], queryStr);
			peer->Packet(TDGHeader::PLAYER_CREATE_ALREADY, dwHandle);
			return;
		}
	}
	else
	{
		peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
		return;
	}

	queryLen = snprintf(queryStr, sizeof(queryStr), 
			"INSERT INTO player "
			"(id, account_id, name, level, st, ht, dx, iq, "
			"job, voice, x, y, "
			"hp, mp)"//, stat_point, "
			// "part_base, part_main, part_hair) "
			// "skill_level, quickslot) "
			"VALUES(0, %u, '%s', %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, "
			"%d, %d)",
			// "%d, %d, %d, %d, %d, %d, %d, %d, 0",
			packet->account_id(), packet->player_table().name().c_str(), packet->player_table().level(),
			packet->player_table().st(), packet->player_table().ht(), packet->player_table().dx(), packet->player_table().iq(),
			packet->player_table().job(), packet->player_table().voice(), packet->player_table().x(), packet->player_table().y(),
			packet->player_table().hp(), packet->player_table().sp());//, packet->player_table.stat_point, 
			// packet->player_table.part_base, packet->player_table.part_base);

	sys_log(0, "PlayerCreate accountid %d name %s level %d gold %lld, st %d ht %d job %d",
			packet->account_id(), 
			packet->player_table().name().c_str(), 
			packet->player_table().level(), 
			(long long) packet->player_table().gold(), 
			packet->player_table().st(), 
			packet->player_table().ht(), 
			packet->player_table().job());

	static char text[4096 + 1];

	// CDBManager::instance().EscapeString(text, packet->player_table.skills, sizeof(packet->player_table.skills));
	// queryLen += snprintf(queryStr + queryLen, sizeof(queryStr) - queryLen, "'%s', ", text);
	// if (g_test_server)
		// sys_log(0, "Create_Player queryLen[%d] TEXT[%s]", queryLen, text);

	// CDBManager::instance().EscapeString(text, packet->player_table.quickslot, sizeof(packet->player_table.quickslot));
	// queryLen += snprintf(queryStr + queryLen, sizeof(queryStr) - queryLen, "'%s')", text);

	std::auto_ptr<SQLMsg> pMsg2(CDBManager::instance().DirectQuery(queryStr));
	if (g_test_server)
		sys_log(0, "Create_Player queryLen[%d] TEXT[%s]", queryLen, text);

	if (pMsg2->Get()->uiAffectedRows <= 0)
	{
		peer->Packet(TDGHeader::PLAYER_CREATE_ALREADY, dwHandle);
		sys_log(0, "ALREADY EXIST3 query: %s AffectedRows %lu", queryStr, pMsg2->Get()->uiAffectedRows);
		return;
	}

	player_id = pMsg2->Get()->uiInsertID;

	snprintf(queryStr, sizeof(queryStr), "UPDATE player_index SET pid%d=%d WHERE id=%d", 
			packet->account_index() + 1, player_id, packet->account_id());
	std::auto_ptr<SQLMsg> pMsg3(CDBManager::instance().DirectQuery(queryStr));

	if (pMsg3->Get()->uiAffectedRows <= 0)
	{
		sys_err("QUERY_ERROR: %s", queryStr);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM player WHERE id=%d", player_id);
		CDBManager::instance().DirectQuery(queryStr);

		peer->Packet(TDGHeader::PLAYER_CREATE_FAILURE, dwHandle);
		return;
	}

	DGOutputPacket<DGPlayerCreateSuccessPacket> pack;

	pack->set_account_index(packet->account_index());

	auto& sent_player = packet->player_table();
	auto player = pack->mutable_player();
	
	player->set_id(player_id);
	player->set_name(sent_player.name());
	player->set_job(sent_player.job());
	player->set_level(1);
	player->set_st(sent_player.st());
	player->set_ht(sent_player.ht());
	player->set_dx(sent_player.dx());
	player->set_iq(sent_player.iq());
	player->set_main_part(sent_player.part_base());
	player->set_x(sent_player.x());
	player->set_y(sent_player.y());

	peer->Packet(pack, dwHandle);

	sys_log(0, "7 name %s job %d", player->name().c_str(), player->job());

	s_createTimeByAccountID[packet->account_id()] = time(0);
}

/*
 * PLAYER DELETE
 */
void CClientManager::__QUERY_PLAYER_DELETE(CPeer* peer, DWORD dwHandle, std::unique_ptr<GDPlayerDeletePacket> packet)
{
	if (packet->login().empty() || !packet->player_id() || packet->account_index() >= PLAYER_PER_ACCOUNT)
		return;

	CLoginData * ld = GetLoginDataByLogin(packet->login().c_str());


	if (!ld)
	{
		peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, dwHandle);
		return;
	}

	TAccountTable & r = ld->GetAccountRef();

	if (strlen(r.social_id().c_str()) < 7 || strncmp(packet->private_code().c_str(), r.social_id().c_str() + strlen(r.social_id().c_str()) - 7, 7))
	{
		sys_log(0, "PLAYER_DELETE FAILED len(%d)", strlen(r.social_id().c_str()));
		peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, dwHandle);
		return;
	}

	TPlayerCache * pkPlayerCache = GetPlayerCache(packet->player_id(), false);
	if (pkPlayerCache && pkPlayerCache->pPlayer)
	{
		TPlayerTable * pTab = pkPlayerCache->pPlayer->Get();

		if (pTab->level() >= m_iPlayerDeleteLevelLimit)
		{
			sys_log(0, "PLAYER_DELETE FAILED LEVEL %u >= DELETE LIMIT %d", pTab->level(), m_iPlayerDeleteLevelLimit);
			peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, dwHandle);
			return;
		}

		if (pTab->level() < m_iPlayerDeleteLevelLimitLower)
		{
			sys_log(0, "PLAYER_DELETE FAILED LEVEL %u < DELETE LIMIT %d", pTab->level(), m_iPlayerDeleteLevelLimitLower);
			peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, dwHandle);
			return;
		}
	}

	char szQuery[256];
	snprintf(szQuery, sizeof(szQuery), "SELECT p.id, p.level, p.name FROM player_index AS i, player AS p WHERE pid%u=%u AND pid%u=p.id",
		packet->account_index() + 1, packet->player_id(), packet->account_index() + 1);

	ClientHandleInfo * pi = new ClientHandleInfo(dwHandle, packet->player_id());
	pi->account_index = packet->account_index();

	sys_log(0, "PLAYER_DELETE TRY: %s %d pid%d", packet->login().c_str(), packet->player_id(), packet->account_index() + 1);
	CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_DELETE, peer->GetHandle(), pi);
}

//
// @version	05/06/10 Bang2ni - ÇÃ·¹ÀÌ¾î »èÁ¦½Ã °¡°ÝÁ¤º¸ ¸®½ºÆ® »èÁ¦ Ãß°¡.
//
void CClientManager::__RESULT_PLAYER_DELETE(CPeer *peer, SQLMsg* msg)
{
	CQueryInfo * qi = (CQueryInfo *) msg->pvUserData;
	ClientHandleInfo * pi = (ClientHandleInfo *) qi->pvData;

	if (msg->Get() && msg->Get()->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		DWORD dwPID = 0;
		str_to_number(dwPID, row[0]);

		int deletedLevelLimit = 0;
		str_to_number(deletedLevelLimit, row[1]);

		char szName[64];
		strlcpy(szName, row[2], sizeof(szName));

		if (deletedLevelLimit >= m_iPlayerDeleteLevelLimit)
		{
			sys_log(0, "PLAYER_DELETE FAILED LEVEL %u >= DELETE LIMIT %d", deletedLevelLimit, m_iPlayerDeleteLevelLimit);
			peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, pi->dwHandle);
			return;
		}

		if (deletedLevelLimit < m_iPlayerDeleteLevelLimitLower)
		{
			sys_log(0, "PLAYER_DELETE FAILED LEVEL %u < DELETE LIMIT %d", deletedLevelLimit, m_iPlayerDeleteLevelLimitLower);
			peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, pi->dwHandle);
			return;
		}

		char queryStr[QUERY_MAX_LEN];

#ifdef __OLD_PLAYER_DELETED__
		snprintf(queryStr, sizeof(queryStr), "INSERT INTO player_deleted SELECT * FROM player WHERE id=%d", 
#else
		snprintf(queryStr, sizeof(queryStr), "UPDATE player SET name=CONCAT('*%i*', name) WHERE id=%d", 
			random_number(1,999), 
#endif
				pi->player_id);
		std::auto_ptr<SQLMsg> pIns(CDBManager::instance().DirectQuery(queryStr));

		if (pIns->Get()->uiAffectedRows == 0 || pIns->Get()->uiAffectedRows == (uint32_t)-1)
		{
			sys_log(0, "PLAYER_DELETE FAILED %u CANNOT INSERT TO player_deleted", dwPID);

			peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, pi->dwHandle);
			return;
		}

		// »èÁ¦ ¼º°ø
		sys_log(0, "PLAYER_DELETE SUCCESS %u", dwPID);

		char account_index_string[16];

		snprintf(account_index_string, sizeof(account_index_string), "player_id%d", m_iPlayerIDStart + pi->account_index);

		// flush cache to remove all data, will be erased through querys in database later on
		FlushPlayerCache(pi->player_id);
#ifdef __EQUIPMENT_CHANGER__
		FlushEquipmentPageCacheSet(pi->player_id);
#endif
		// remove in database
		snprintf(queryStr, sizeof(queryStr), "UPDATE player_index SET pid%u=0 WHERE pid%u=%d", 
				pi->account_index + 1, 
				pi->account_index + 1, 
				pi->player_id);

		std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(queryStr, SQL_PLAYER_STUFF));

		if (pMsg->Get()->uiAffectedRows == 0 || pMsg->Get()->uiAffectedRows == (uint32_t)-1)
		{
			sys_log(0, "PLAYER_DELETE FAIL WHEN UPDATE account table");
			peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, pi->dwHandle);
			return;
		}

		RemoveExistingPlayerName(szName);
		
#ifdef __OLD_PLAYER_DELETED__
		snprintf(queryStr, sizeof(queryStr), "DELETE FROM player WHERE id=%d", pi->player_id);
		delete CDBManager::instance().DirectQuery(queryStr);
#endif

#ifdef __PET_ADVANCED__
		snprintf(queryStr, sizeof(queryStr), "DELETE pet, pet_skill FROM pet "
			"LEFT JOIN pet_skill ON pet_skill.item_id = pet.item_id "
			"INNER JOIN item ON item.id = pet.item_id "
			"WHERE item.owner_id = %u AND item.window < %u",
			pi->player_id, SAFEBOX);
		delete CDBManager::instance().DirectQuery(queryStr);
#endif

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM item WHERE owner_id=%d AND window < %d", pi->player_id, SAFEBOX);
		delete CDBManager::instance().DirectQuery(queryStr, SQL_PLAYER_STUFF);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM quest WHERE dwPID=%d", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM affect WHERE dwPID=%d", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM guild_member WHERE pid=%d", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

		// MYSHOP_PRICE_LIST
		snprintf(queryStr, sizeof(queryStr), "DELETE FROM myshop_pricelist WHERE owner_id=%d", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);
		// END_OF_MYSHOP_PRICE_LIST

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM messenger_list WHERE account='%s' OR companion='%s'", szName, szName);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

#ifdef __EQUIPMENT_CHANGER__
		snprintf(queryStr, sizeof(queryStr), "DELETE FROM equipment_page WHERE pid = %u", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr);
#endif
		
#ifdef COMBAT_ZONE
		snprintf(queryStr, sizeof(queryStr), "DELETE FROM combat_zone_ranking_weekly WHERE memberName='%s'", szName);
		CDBManager::instance().AsyncQuery(queryStr);
		snprintf(queryStr, sizeof(queryStr), "DELETE FROM combat_zone_ranking_general WHERE memberName='%s'", szName);
		CDBManager::instance().AsyncQuery(queryStr);
#endif

#ifdef AUCTION_SYSTEM
		if (auto processor_peer = GetProcessorPeer())
		{
			DGOutputPacket<DGAuctionDeletePlayer> del_auction_pack;
			del_auction_pack->set_pid(pi->player_id);
			processor_peer->Packet(del_auction_pack);
		}

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM auction_shop WHERE pid=%u", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

		snprintf(queryStr, sizeof(queryStr), "DELETE auction_item FROM auction_item INNER JOIN item ON item.id = auction_item.item_id WHERE item.owner_id = %u AND window IN (%u, %u)",
			pi->player_id, AUCTION, AUCTION_SHOP);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM item WHERE owner_id=%u AND window IN (%u, %u)", pi->player_id, AUCTION, AUCTION_SHOP);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);

		snprintf(queryStr, sizeof(queryStr), "DELETE FROM auction_shop_history WHERE pid=%u", pi->player_id);
		CDBManager::instance().AsyncQuery(queryStr, SQL_PLAYER_STUFF);
#endif

		DGOutputPacket<DGPlayerDeleteSuccessPacket> success_pack;
		success_pack->set_account_index(pi->account_index);
		peer->Packet(success_pack, pi->dwHandle);
	}
	else
	{
		// »èÁ¦ ½ÇÆÐ
		sys_log(0, "PLAYER_DELETE FAIL NO ROW");
		peer->Packet(network::TDGHeader::PLAYER_DELETE_FAILURE, pi->dwHandle);
	}
}

void CClientManager::QUERY_ADD_AFFECT(CPeer * peer, std::unique_ptr<GDAddAffectPacket> p)
{
	if (g_test_server)
		sys_log(0, "QUERY_ADD_AFFECT [%u] affType %u", p->pid(), p->elem().type());
	PutAffectCache(p->pid(), &p->elem());
}

void CClientManager::QUERY_REMOVE_AFFECT(CPeer * peer, std::unique_ptr<GDRemoveAffectPacket> p)
{
	if (g_test_server)
		sys_log(0, "QUERY_REMOVE_AFFECT [%u] affType %u", p->pid(), p->type());
	DeleteAffectCache(p->pid(), AFFECT_KEY_TYPE(p->type(), p->apply_on()));
}

void CClientManager::InsertLogoutPlayer(DWORD pid)
{
	TLogoutPlayerMap::iterator it = m_map_logout.find(pid);

	// Á¸ÀçÇÏÁö ¾ÊÀ»°æ¿ì Ãß°¡
	if (it != m_map_logout.end())
	{
		// Á¸ÀçÇÒ°æ¿ì ½Ã°£¸¸ °»½Å
		if (g_log)
			sys_log(0, "LOGOUT: Update player time pid(%d)", pid);

		it->second->time = time(0);
		return;
	}
		
	TLogoutPlayer * pLogout = new TLogoutPlayer;
	pLogout->pid = pid;
	pLogout->time = time(0);
	m_map_logout.insert(std::make_pair(pid, pLogout));

	if (g_log)
		sys_log(0, "LOGOUT: Insert player pid(%d)", pid);
}

void CClientManager::DeleteLogoutPlayer(DWORD pid)
{
	TLogoutPlayerMap::iterator it = m_map_logout.find(pid);

	if (it != m_map_logout.end())
	{
		delete it->second;
		m_map_logout.erase(it);
	}
}

extern int g_iLogoutSeconds;

void CClientManager::UpdateLogoutPlayer()
{
	time_t now = time(0);

	TLogoutPlayerMap::iterator it = m_map_logout.begin();

	while (it != m_map_logout.end() && m_iCacheFlushCount < m_iCacheFlushCountLimit)
	{
		TLogoutPlayer* pLogout = it->second;

		if (now - g_iLogoutSeconds > pLogout->time)
		{
#ifdef __EQUIPMENT_CHANGER__
			FlushEquipmentPageCacheSet(pLogout->pid);
#endif
			FlushPlayerCache(pLogout->pid);

			delete pLogout;
			m_map_logout.erase(it++);
			++m_iCacheFlushCount;
		}
		else
			++it;
	}
}

#ifdef __HAIR_SELECTOR__
void CClientManager::QUERY_SELECT_UPDATE_HAIR(CPeer* peer, std::unique_ptr<GDSelectUpdateHairPacket> p)
{
	TPlayerCache* pCache;
	if (pCache = GetPlayerCache(p->pid(), false))
	{
		TPlayerTable* pTab = pCache->pPlayer->Get();
		pTab->set_parts(PART_HAIR, p->hair_part());
		pTab->set_part_hair_base(p->hair_base_part());
	}
}
#endif

#ifdef __ITEM_REFUND__
void CClientManager::QUERY_ITEM_REFUND(CPeer* peer, DWORD dwHandle, DWORD pid)
{
	char queryStr[1024];
	snprintf(queryStr, sizeof(queryStr),
		"SELECT id,socket_set,%s FROM item_refund WHERE owner_id = %u AND item_id = 0",
		GetItemQueryKeyPart(true, NULL, "item_id", "window+0|pos"), pid);
	if (g_test_server)
		sys_log(0, "ITEM_REFUND_QRY: %s", queryStr);
	CDBManager::instance().ReturnQuery(queryStr, QID_ITEM_REFUND, peer->GetHandle(), new ClientHandleInfo(dwHandle, pid));
}

void CClientManager::RESULT_ITEM_REFUND(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID)
{
	int rows;
	if ((rows = mysql_num_rows(pRes)) > 0)	// µ¥ÀÌÅÍ ¾øÀ½
	{
		DGOutputPacket<DGItemRefundLoadPacket> pack;
		pack->set_pid(dwPID);

		for (int i = 0; i < rows; ++i)
		{
			MYSQL_ROW row = mysql_fetch_row(pRes);
			auto item = pack->add_items();

			item->set_id(std::stoll(row[0]));
			item->set_socket_set(std::stoll(row[1]));

			CreateItemTableFromRow(row, item->mutable_item(), 2, false);
		}

		peer->Packet(pack, dwHandle);
	}

	// ITEM_LOAD_LOG_ATTACH_PID
	sys_log(0, "ITEM_REFUND_LOAD: count %u pid %u", rows, dwPID);
	// END_OF_ITEM_LOAD_LOG_ATTACH_PID
}

/*void CClientManager::QUERY_ITEM_REFUND_ADD(const TPlayerItem* pItemData)
{
	TPlayerItem myData; 
	thecore_memcpy(&myData, pItemData, sizeof(myData));
	myData.id = 0;

	char queryStr[1024];
	snprintf(queryStr, sizeof(queryStr),
		"INSERT INTO item_refund (owner_id, %s) VALUES (%u, %s)",
		GetItemQueryKeyPart(false, "item_refund", "item_id", "window|pos"),
		myData.owner,
		GetItemQueryValuePart(&myData, false));

	CDBManager::instance().AsyncQuery(queryStr);
}*/
#endif

void CClientManager::QUERY_ITEM_DESTROY_LOG(std::unique_ptr<GDItemDestroyLogPacket> p)
{
	char queryStr[1024];
	snprintf(queryStr, sizeof(queryStr),
		"INSERT INTO item_destroy_log (how, owner_id, %s) VALUES (%u, %u, %s)",
		GetItemQueryKeyPart(false, NULL),
		p->type(),
		p->item().owner(),
		GetItemQueryValuePart(&p->item()));

	CDBManager::instance().AsyncQuery(queryStr);
}

#ifdef ENABLE_RUNE_SYSTEM
void CClientManager::QUERY_PLAYER_RUNE_SAVE(CPeer * peer, DWORD dwHandle, std::unique_ptr<GDPlayerRuneSavePacket> pack)
{
	CClientManager::TPlayerCache* pCache;
	if (!(pCache = GetPlayerCache(pack->player_id(), false)))
	{
		sys_err("cannot save player runes [count %d]for PID [%u]", pack->runes_size(), pack->player_id());
		return;
	}

	if (g_test_server)
		sys_log(0, "QUERY_PLAYER_RUNE_SAVE[%u] => runeCount %u", pack->player_id(), pack->runes_size());

	TPlayerTable* pTab = pCache->pPlayer->Get();
	if (!pTab)
	{
		sys_err("no pTab for saving player runes [count %d]for PID [%u]", pack->runes_size(), pack->player_id());
		return;
	}

	*pTab->mutable_runes() = pack->runes();
}
#endif
