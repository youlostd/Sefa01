// vim:ts=4 sw=4
#include <map>
#include "stdafx.h"
#include "ClientManager.h"
#include "Main.h"
#include "CsvReader.h"
#include "ProtoReader.h"
#include <iostream>
#include <fstream>

using namespace std;

extern int g_test_server;
extern BYTE g_bProtoLoadingMethod;
extern int g_protoSaveEnable;

size_t str_lower(const char * src, char * dest, size_t dest_size)
{
	size_t len = 0;

	if (!dest || dest_size == 0)
		return len;

	if (!src)
	{
		*dest = '\0';
		return len;
	}

	--dest_size;

	while (*src && len < dest_size)
	{
		*dest = LOWER(*src);

		++src;
		++dest;
		++len;
	}

	*dest = '\0';
	return len;
}

const char* strlower(const char* string)
{
	static char s_szLowerString[1024 + 1];
	str_lower(string, s_szLowerString, sizeof(s_szLowerString));
	return s_szLowerString;
}


bool CClientManager::InitializeHorseUpgradeTable()
{
	char query[4096];
	snprintf(query, sizeof(query), "SELECT `type`+0, level, level_limit, refine_id "
		"FROM horse_upgrade_proto ORDER BY `type`+0, level");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_vec_HorseUpgradeProto.empty())
	{
		sys_log(0, "RELOAD: horse upgrade");
		m_vec_HorseUpgradeProto.clear();
	}

	MYSQL_ROW data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			THorseUpgradeProto k;

			int col = 0;

			k.set_upgrade_type(atoi(data[col++]));
			k.set_level(atoi(data[col++]));
			k.set_level_limit(atoi(data[col++]));
			k.set_refine_id(atoi(data[col++]));

			sys_log(0, "PET_UPGRADE: type %u level %u level_limit %u refine_id %u",
				k.upgrade_type(), k.level(), k.level_limit(), k.refine_id());

			m_vec_HorseUpgradeProto.push_back(k);
		}

	return true;
}



bool CClientManager::InitializeHorseBonusTable()
{
	char query[4096];
	snprintf(query, sizeof(query), "SELECT level, max_hp, max_hp_item, armor, armor_item, monster, monster_item, item_count "
		"FROM horse_bonus_proto ORDER BY level");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_vec_HorseBonusProto.empty())
	{
		sys_log(0, "RELOAD: horse bonus");
		m_vec_HorseBonusProto.clear();
	}

	MYSQL_ROW data;

	if (pRes->uiNumRows > 0)
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		THorseBonusProto k;

		int col = 0;

		k.set_level(atoi(data[col++]));
		k.set_max_hp(atoi(data[col++]));
		k.set_max_hp_item(atoi(data[col++]));
		k.set_armor_pct(atoi(data[col++]));
		k.set_armor_item(atoi(data[col++]));
		k.set_monster_pct(atoi(data[col++]));
		k.set_monster_item(atoi(data[col++]));
		k.set_item_count(atoi(data[col++]));

		sys_log(0, "HORSE_BONUS: level %u max_hp %u max_hp_item %u armor %u armor_item %u monster %u monster_item %u item_count %u",
			k.level(), k.max_hp(), k.max_hp_item(), k.armor_pct(), k.armor_item(), k.monster_pct(), k.monster_item(), k.item_count());

		m_vec_HorseBonusProto.push_back(k);
	}

	return true;
}

bool CClientManager::InitializeLanguage()
{
	const std::string stKeyBase = "LANGUAGE_";

	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT mKey, mValue FROM locale WHERE mKey LIKE '%s%%'", stKeyBase.c_str());
	std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_COMMON));

	if (pMsg->Get()->uiNumRows != LANGUAGE_MAX_NUM)
	{
		fprintf(stderr, "LOAD_LANGUAGES not enough language names (expected %d got %d)", LANGUAGE_MAX_NUM, pMsg->Get()->uiNumRows);
		return false;
	}

	while (MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult))
	{
		int iLanguageID = atoi(row[0] + stKeyBase.length()) - 1;
		if (iLanguageID < 0 || iLanguageID >= LANGUAGE_MAX_NUM)
		{
			fprintf(stderr, "LOAD_LANGUAGES invalid language id %d (range 0 - %d allowed)", iLanguageID + 1, LANGUAGE_MAX_NUM - 1);
			return false;
		}

		if (!astLocaleStringNames[iLanguageID].empty())
		{
			fprintf(stderr, "LOAD_LANGUAGES double language id %d found", iLanguageID + 1);
			return false;
		}

		if (char* szPos = strchr(row[1], '|'))
			*szPos = '\0';
		astLocaleStringNames[iLanguageID] = row[1];
	}

	return true;
}

bool CClientManager::InitializeTables()
{
	TransferPlayerTable();

	if (g_bProtoLoadingMethod == PROTO_LOADING_DATABASE)
	{
		if (!InitializeMobTableFromDatabase())
		{
			sys_err("InitializeMobTableFromDatabase FAILED");
			return false;
		}

		if (!InitializeItemTableFromDatabase())
		{
			sys_err("InitializeItemTableFromDatabase FAILED");
			return false;
		}

		if (g_protoSaveEnable)
			sys_err("ProtoSaveEnable option only available if proto is read from text-file!");
	}
	else if (g_bProtoLoadingMethod == PROTO_LOADING_TEXTFILE)
	{
		if (!InitializeMobTableFromTextFile())
		{
			sys_err("InitializeMobTableFromTextFile FAILED");
			return false;
		}

		if (g_protoSaveEnable && !MirrorMobTableIntoDB())
		{
			sys_err("MirrorMobTableIntoDB FAILED");
			return false;
		}

		if (!InitializeItemTableFromTextFile())
		{
			sys_err("InitializeItemTableFromTextFile FAILED");
			return false;
		}

		if (g_protoSaveEnable && !MirrorItemTableIntoDB())
		{
			sys_err("MirrorItemTableIntoDB FAILED");
			return false;
		}
	}

	if (!InitializeShopTable())
	{
		sys_err("InitializeShopTable FAILED");
		return false;
	}

	if (!InitializeSkillTable())
	{
		sys_err("InitializeSkillTable FAILED");
		return false;
	}

	if (!InitializeRefineTable())
	{
		sys_err("InitializeRefineTable FAILED");
		return false;
	}

	if (!InitializeItemAttrTable())
	{
		sys_err("InitializeItemAttrTable FAILED");
		return false;
	}

#ifdef EL_COSTUME_ATTR
	if (!InitializeItemCostumeAttrTable())
	{
		sys_err("InitializeItemCostumeAttrTable FAILED");
		return false;
	}
#endif

#ifdef ITEM_RARE_ATTR
	if (!InitializeItemRareTable())
	{
		sys_err("InitializeItemRareTable FAILED");
		return false;
	}
#endif

	if (!InitializeLandTable())
	{
		sys_err("InitializeLandTable FAILED");
		return false;
	}

	if (!InitializeObjectProto())
	{
		sys_err("InitializeObjectProto FAILED");
		return false;
	}

	if (!InitializeObjectTable())
	{
		sys_err("InitializeObjectTable FAILED");
		return false;
	}

	if (!InitializeHorseUpgradeTable())
	{
		sys_err("InitializeHorseUpgradeTable FAILED");
		return false;
	}

	if (!InitializeHorseBonusTable())
	{
		sys_err("InitializeHorseBonusTable FAILED");
		return false;
	}

#ifdef __GAYA_SYSTEM__
	if (!InitializeGayaShop())
	{
		sys_err("InitializeGayaShop FAILED");
		return false;
	}
#endif

#ifdef __ATTRTREE__
	if (!InitializeAttrTree())
	{
		sys_err("InitializeAttrTree FAILED");
		return false;
	}
#endif

#ifdef ENABLE_RUNE_SYSTEM
	if (!InitializeRuneProto())
	{
		sys_err("InitializeRuneProto FAILED");
		return false;
	}
#endif

#ifdef __PET_ADVANCED__
	if (!InitializePetSkillTable())
	{
		sys_err("InitializePetSkillTable FAILED");
		return false;
	}

	if (!InitializePetEvolveTable())
	{
		sys_err("InitializePetEvolveTable FAILED");
		return false;
	}

	if (!InitializePetAttrTable())
	{
		sys_err("InitializePetAttrTable FAILED");
		return false;
	}
#endif

#ifdef ENABLE_XMAS_EVENT
	InitializeXmasRewards();
#endif
#ifdef SOUL_SYSTEM
	InitializeSoulProtoTable();
#endif
#ifdef CRYSTAL_SYSTEM
	InitializeCrystalProtoTable();
#endif

	return true;
}

#ifdef ENABLE_RUNE_SYSTEM
bool CClientManager::InitializeRuneProto()
{
	char query[4096];
	int queryLen = snprintf(query, sizeof(query), "SELECT id, name, `group`-1, `sub_group`-1, `apply_type`-1, apply_eval FROM rune_proto");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	m_vec_RuneProto.clear();

	MYSQL_ROW data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		int col = 0;

		TRuneProtoTable kProto;

		kProto.set_vnum(atoi(data[col++]));
		kProto.set_name(data[col++]);
		kProto.set_group(atoi(data[col++]));
		kProto.set_sub_group(atoi(data[col++]));
		kProto.set_apply_type(atoi(data[col++]));
		kProto.set_apply_eval(data[col++]);

		m_vec_RuneProto.push_back(kProto);
	}

	queryLen = snprintf(query, sizeof(query), "SELECT point, refine_proto FROM runepoint_proto");

	std::auto_ptr<SQLMsg> pkMsg2(CDBManager::instance().DirectQuery(query));
	pRes = pkMsg2->Get();

	m_vec_RunePointProto.clear();
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		int col = 0;

		TRunePointProtoTable kProto;

		kProto.set_point(atoi(data[col++]));
		kProto.set_refine_proto(atoi(data[col++]));

		m_vec_RunePointProto.push_back(kProto);
	}
	return true;
}
#endif

#ifdef __PET_ADVANCED__
bool CClientManager::InitializePetSkillTable()
{
	char query[4096];
	snprintf(query, sizeof(query), "SELECT apply+0, value, is_heroic-1 FROM pet_skill_proto");

	std::unique_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult* pRes = pkMsg->Get();

	if (!pRes)
		return false;

	if (!m_vec_PetSkillProto.empty())
	{
		sys_log(0, "RELOAD: pet skill table");
		m_vec_PetSkillProto.clear();
	}

	while (MYSQL_ROW data = mysql_fetch_row(pRes->pSQLResult))
	{
		int col = 0;

		network::TPetAdvancedSkillProto skill;

		skill.set_apply(std::stoi(data[col++]));
		skill.set_value_expr(data[col++]);
		skill.set_is_heroic(std::stoi(data[col++]) == 1);

		m_vec_PetSkillProto.push_back(std::move(skill));
	}

	return true;
}

bool CClientManager::InitializePetEvolveTable()
{
	char query[4096];
	snprintf(query, sizeof(query), "SELECT level, name, refine_id, scale, normal_skill_count, heroic_skill_count, skillpower, skillpower_rerollable-1 FROM pet_evolve_proto");

	std::unique_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult* pRes = pkMsg->Get();

	if (!pRes)
		return false;

	if (!m_vec_PetEvolveProto.empty())
	{
		sys_log(0, "RELOAD: pet evolve table");
		m_vec_PetEvolveProto.clear();
	}

	while (MYSQL_ROW data = mysql_fetch_row(pRes->pSQLResult))
	{
		int col = 0;

		network::TPetAdvancedEvolveProto evolve;

		evolve.set_level(std::stoi(data[col++]));
		evolve.set_name(data[col++]);

		evolve.set_refine_id(std::stoi(data[col++]));

		std::string scale_str = data[col++];
		std::replace(scale_str.begin(), scale_str.end(), ',', '.');
		evolve.set_scale(std::stof(scale_str));

		evolve.set_normal_skill_count(std::stoi(data[col++]));
		evolve.set_heroic_skill_count(std::stoi(data[col++]));

		evolve.set_skillpower(std::stoi(data[col++]));
		evolve.set_skillpower_rerollable(std::stoi(data[col++]));

		m_vec_PetEvolveProto.push_back(std::move(evolve));
	}

	return true;
}

bool CClientManager::InitializePetAttrTable()
{
	char query[4096];
	int len = snprintf(query, sizeof(query), "SELECT apply_level, apply_type+0, value, refine_id, required_pet_level FROM pet_attr_proto");

	std::unique_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult* pRes = pkMsg->Get();

	if (!pRes)
		return false;

	if (!m_vec_PetAttrProto.empty())
	{
		sys_log(0, "RELOAD: pet attr table");
		m_vec_PetAttrProto.clear();
	}

	while (MYSQL_ROW data = mysql_fetch_row(pRes->pSQLResult))
	{
		int col = 0;

		network::TPetAdvancedAttrProto attr;

		attr.set_apply_level(std::stoi(data[col++]));
		attr.set_apply_type(std::stoi(data[col++]));

		std::string val = data[col++];
		std::replace(val.begin(), val.end(), ',', '.');
		attr.set_value(std::stof(val));

		attr.set_refine_id(std::stoi(data[col++]));
		attr.set_required_pet_level(std::stoi(data[col++]));

		m_vec_PetAttrProto.push_back(std::move(attr));
	}

	return true;
}
#endif

bool CClientManager::InitializeRefineTable()
{
	char query[2048];

	snprintf(query, sizeof(query),
			"SELECT id, cost, prob, vnum0, count0, vnum1, count1, vnum2, count2,  vnum3, count3, vnum4, count4 FROM refine_proto");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
		return true;

	if (m_pRefineTable)
	{
		sys_log(0, "RELOAD: refine_proto");
		delete [] m_pRefineTable;
		m_pRefineTable = NULL;
	}

	m_iRefineTableSize = pRes->uiNumRows;

	m_pRefineTable	= new TRefineTable[m_iRefineTableSize];

	TRefineTable* prt = m_pRefineTable;
	MYSQL_ROW data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		//const char* s_szQuery = "SELECT src_vnum, result_vnum, cost, prob, "
		//"vnum0, count0, vnum1, count1, vnum2, count2,  vnum3, count3, vnum4, count4 "

		int col = 0;
		//prt->src_vnum = atoi(data[col++]);
		//prt->result_vnum = atoi(data[col++]);
		prt->set_id(std::stoi(data[col++]));
		prt->set_cost(std::stoll(data[col++]));
		prt->set_prob(std::stoi(data[col++]));
		prt->set_material_count(REFINE_MATERIAL_MAX_NUM);

		for (int i = 0; i < REFINE_MATERIAL_MAX_NUM; i++)
		{
			auto mat = prt->add_materials();

			mat->set_vnum(std::stoi(data[col++]));
			mat->set_count(std::stoi(data[col++]));
			if (mat->vnum() == 0 && prt->material_count() == REFINE_MATERIAL_MAX_NUM)
			{
				prt->set_material_count(i);
			}
		}

		sys_log(0, "REFINE: id %d cost %lld prob %d mat1 %lu cnt1 %d", prt->id(), (long long) prt->cost(), prt->prob(), prt->materials(0).vnum(), prt->materials(0).count());

		prt++;
	}
	return true;
}

#ifdef __GAYA_SYSTEM__
bool CClientManager::InitializeGayaShop()
{
	char query[2048];

	snprintf(query, sizeof(query), "SELECT pos-1, vnum, count, price FROM gaya_shop_proto");
	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (m_vec_gayaShopTable.size() > 0)
	{
		sys_log(0, "RELOAD: gaya shop");
		m_vec_gayaShopTable.clear();
	}

	if (!pRes->uiNumRows)
		return true;

	MYSQL_ROW data;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		int col = 0;

		TGayaShopData kData;

		kData.set_pos(atoi(data[col++]));
		kData.set_vnum(atoi(data[col++]));
		kData.set_count(atoi(data[col++]));
		kData.set_price(atoi(data[col++]));


		m_vec_gayaShopTable.push_back(kData);

		sys_log(0, "GAYA_SHOP: pos %d vnum %u count %u price %u", kData.pos(), kData.vnum(), kData.count(), kData.price());
	}

	return true;
}
#endif

#ifdef __ATTRTREE__
bool CClientManager::InitializeAttrTree()
{
	char query[2048];

	int len = snprintf(query, sizeof(query), "SELECT row-1, col-1, apply_type+0, max_apply_value");
	for (int i = 0; i < ATTRTREE_LEVEL_NUM; ++i)
		len += snprintf(query + len, sizeof(query) - len, ", refine_%d", i + 1);
	len += snprintf(query + len, sizeof(query) - len, " FROM attrtree_proto");
	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	for (int row = 0; row < ATTRTREE_ROW_NUM; ++row)
	{
		for (int col = 0; col < ATTRTREE_COL_NUM; ++col)
		{
			TAttrtreeProto& rkProto = m_aAttrTreeProto[row][col];
			rkProto.row = row;
			rkProto.col = col;
		}
	}

	if (!pRes->uiNumRows)
		return true;

	MYSQL_ROW data;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		int col = 0;

		BYTE attrrow;
		str_to_number(attrrow, data[col++]);
		BYTE attrcol;
		str_to_number(attrcol, data[col++]);

		if (attrrow >= ATTRTREE_ROW_NUM || attrcol >= ATTRTREE_COL_NUM)
		{
			sys_err("invalid row or col : cell(%d, %d)", attrrow+1, attrcol+1);
			return false;
		}

		TAttrtreeProto& rkProto = m_aAttrTreeProto[attrrow][attrcol];
		str_to_number(rkProto.apply_type, data[col++]);
		str_to_number(rkProto.max_apply_value, data[col++]);
		for (int i = 0; i < ATTRTREE_LEVEL_NUM; ++i)
			str_to_number(rkProto.refine_level[i], data[col++]);
	}

	return true;
}
#endif

bool CClientManager::InitializeMobTableFromTextFile()
{
	/*=================== Function Description ===================
	1. Summary: read files 'mob_proto.txt', 'mob_proto_test.txt', 'mob_names.txt',
		(!)[mob_table] Create a table object. (Type: TMobTable)
	2. order
		1) Read the 'mob_names.txt' file and create a map (a)[localMap](vnum:name).
		2) 'mob_proto_test.txt' file and (a)[localMap] map
		(b)[test_map_mobTableByVnum](vnum:TMobTable) Create a map.
		3) 'mob_proto.txt' file and (a)[localMap] map
		Create the (!)[mob_table] table.
		<Note>
		in each row,
		Rows in both (b)[test_map_mobTableByVnum] and (!)[mob_table] are
		(b) Use the one in [test_map_mobTableByVnum].
		4) Among the rows of (b)[test_map_mobTableByVnum], add the row that is not in (!)[mob_table].
	3. test
		1) Check if 'mob_proto.txt' information is properly entered into mob_table. -> Done
		2) Check if 'mob_names.txt' information is properly entered into mob_table.
		3) Check if the [overlapping] information from 'mob_proto_test.txt' is properly entered into mob_table.
		4) Check if the [new] information from 'mob_proto_test.txt' is properly entered into mob_table.
		5) Make sure it works properly in the (final) game client.
	_______________________________________________*/

	//===============================================//
	// 1) Read the 'mob_names.txt' file and create (a)[localMap] map.
	//<(a)create localMap map>
	map<int, const char*> localMap[LANGUAGE_MAX_NUM];
	bool isNameFile[LANGUAGE_MAX_NUM];
	//bool isNameFile = true;
	//<read file>
	cCsvTable nameData[LANGUAGE_MAX_NUM];
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		isNameFile[i] = true;

		char szFileName[FILENAME_MAX];
		snprintf(szFileName, sizeof(szFileName), "locale/%s/mob_names.txt", strlower(astLocaleStringNames[i].c_str()));

		if (!nameData[i].Load(szFileName, '\t'))
		{
			fprintf(stderr, "mob_names.txt wrong!\n");
			isNameFile[i] = false;
		}
		else {
			nameData[i].Next(); // Omit the description row.
			while (nameData[i].Next()) {
				localMap[i][atoi(nameData[i].AsStringByIndex(0))] = nameData[i].AsStringByIndex(1);
			}
		}
	}
	//________________________________________________//

	
	//===============================================//
	set<int> vnumSet;
	bool isTestFile = true;
	cCsvTable test_data;
	if(!test_data.Load("mob_proto_test.txt",'\t'))
	{
		fprintf(stderr, "Could not load mob_proto_test.txt. Wrong file format?\n");
		isTestFile = false;
	}

	map<DWORD, TMobTable *> test_map_mobTableByVnum;
	if (isTestFile)
	{
		test_data.Next();

		TMobTable * test_mob_table = NULL;
		int test_MobTableSize = test_data.m_File.GetRowCount()-1;
		test_mob_table = new TMobTable[test_MobTableSize];

		while(test_data.Next())
		{
			if (!Set_Proto_Mob_Table(test_mob_table, test_data, localMap))
				fprintf(stderr, "Could not process entry.\n");

			test_map_mobTableByVnum.insert(std::map<DWORD, TMobTable *>::value_type(test_mob_table->vnum(), test_mob_table));

			++test_mob_table;
		}
	}
	//________________________________________________//


	//===============================================//
	cCsvTable* data = new cCsvTable;
	if(!data->Load("mob_proto.txt",'\t'))
	{
		fprintf(stderr, "Could not load mob_proto.txt. Wrong file format?\n");
		return false;
	}
	data->Next();

	int addNumber = 0;
	while(data->Next())
	{
		int vnum = atoi(data->AsStringByIndex(0));
		std::map<DWORD, TMobTable *>::iterator it_map_mobTable;
		it_map_mobTable = test_map_mobTableByVnum.find(vnum);
		if(it_map_mobTable != test_map_mobTableByVnum.end()) {
			addNumber++;
		}
	}

	data->Destroy();
	if(!data->Load("mob_proto.txt",'\t'))
	{
		fprintf(stderr, "Could not load mob_proto.txt. Wrong file format?\n");
		return false;
	}
	data->Next(); //Except for the top row (the part that describes the item column)
	//2.2 Create mob_table to fit size
	if (!m_vec_mobTable.empty())
	{
		sys_log(0, "RELOAD: mob_proto");
		m_vec_mobTable.clear();
	}
	m_vec_mobTable.resize(data->m_File.GetRowCount()-1 + addNumber);
	TMobTable * mob_table = &m_vec_mobTable[0];
	//2.3 Fill data
	while (data->Next())
	{
		int col = 0;
		//(b)[test_map_mobTableByVnum]�� ���� row�� �ִ��� ����.
		bool isSameRow = true;
		std::map<DWORD, TMobTable *>::iterator it_map_mobTable;
		it_map_mobTable = test_map_mobTableByVnum.find(atoi(data->AsStringByIndex(col)));
		if (it_map_mobTable == test_map_mobTableByVnum.end())
		{
			isSameRow = false;
		}

		if (isSameRow)
		{
			TMobTable *tempTable = it_map_mobTable->second;
			*mob_table = *tempTable;
		}
		else
		{
			if (!Set_Proto_Mob_Table(mob_table, *data, localMap))
				fprintf(stderr, "Could not process entry.\n");
		}

		vnumSet.insert(mob_table->vnum());

		char szNameLogText[256];
		int iLen = snprintf(szNameLogText, sizeof(szNameLogText), "%-24s", mob_table->locale_name(0).c_str());
		for (int i = 1; i < LANGUAGE_MAX_NUM; ++i)
			iLen += snprintf(szNameLogText + iLen, sizeof(szNameLogText) - iLen, ", %-24s", mob_table->locale_name(i).c_str());
		char szLogText[1024];
		snprintf(szLogText, sizeof(szLogText), "MOB #%%-5d %%-24s %%s level: %%-3u rank: %%u empire: %%d mingold: %%d maxgold %%d");
		sys_log(0, szLogText, mob_table->vnum(), mob_table->name().c_str(), szNameLogText, mob_table->level(), mob_table->rank(), mob_table->empire(), mob_table->gold_min(), mob_table->gold_max());
		++mob_table;

	}

	sys_log(0, "Loaded .. .delete.");
	delete data;
	//_____________________________________________________//


	//===============================================//
	test_data.Destroy();
	isTestFile = true;
	test_data;
	if(!test_data.Load("mob_proto_test.txt",'\t'))
	{
		fprintf(stderr, "Could not load mob_proto_test.txt. Wrong file format?\n");
		isTestFile = false;
	}
	if (isTestFile)
	{
		test_data.Next();

		while (test_data.Next())
		{
			set<int>::iterator itVnum;
			itVnum=vnumSet.find(atoi(test_data.AsStringByIndex(0)));
			if (itVnum != vnumSet.end())
				continue;

			if (!Set_Proto_Mob_Table(mob_table, test_data, localMap))
				fprintf(stderr, "Could not process entry.\n");

			char szNameLogText[256];
			int iLen = snprintf(szNameLogText + iLen, sizeof(szNameLogText) - iLen, "%-24s", mob_table->locale_name(0).c_str());
			for (int i = 1; i < LANGUAGE_MAX_NUM; ++i)
				iLen += snprintf(szNameLogText + iLen, sizeof(szNameLogText) - iLen, ", %-24s", mob_table->locale_name(i).c_str());
			char szLogText[1024];
			snprintf(szLogText, sizeof(szLogText), "MOB #%%-5d %%s %%-24s level: %%-3u rank: %%u empire: %%d regen_cycle %%d regen_percent %%d");
			sys_log(0, szLogText, mob_table->vnum(), mob_table->name().c_str(), szNameLogText, mob_table->level(), mob_table->rank(), mob_table->empire(),
				mob_table->regen_cycle(), mob_table->regen_percent());
			++mob_table;

		}
	}

	sort(m_vec_mobTable.begin(), m_vec_mobTable.end(), [](const TMobTable& a, const TMobTable& b) {
		return a.vnum() < b.vnum();
	});

	return true;
}

// if you want to use this check columns first cus some enchant and resist is not added (thats for sure cus I was lazy :P)
bool CClientManager::InitializeMobTableFromDatabase()
{
	return false;
	/*
	MYSQL_ROW	data;
	int			col;
	
	char szQuery[QUERY_MAX_LEN];
	int iQueryLen = snprintf(szQuery, sizeof(szQuery), "SELECT vnum, name, ");
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen, "locale_name_%s, ", strlower(astLocaleStringNames[i].c_str()));
	iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen,
		"rank, type, battle_type, level, size, ai_flag+0, mount_capacity, setRaceFlag+0, setImmuneFlag+0, "
		"empire, folder, on_click, st, dx, ht, iq, damage_min, damage_max, max_hp, regen_cycle, regen_percent, gold_min, gold_max, "
		"exp, def, attack_speed, move_speed, aggressive_hp_pct, aggressive_sight, attack_range, drop_item, resurrection_vnum, "
		"enchant_curse, enchant_slow, enchant_poison, enchant_stun, enchant_critical, enchant_penetrate, resist_sword, resist_twohand, "
		"resist_dagger, resist_bell, resist_fan, resist_bow, resist_fire, resist_elect, resist_magic, resist_wind, resist_poison, "
		"dam_multiply, summon, drain_sp, mob_color, polymorph_item, skill_level0, skill_vnum0, skill_level1, skill_vnum1, "
		"skill_level2, skill_vnum2, skill_level3, skill_vnum3, skill_level4, skill_vnum4, sp_berserk, sp_stoneskin, sp_godspeed, "
		"sp_deathblow, sp_revive FROM mob_proto");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(szQuery));

	SQLResult* pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("InitializeMobTable : Table count is zero.");
		return false;
	}

	if (!m_vec_mobTable.empty())
	{
		sys_log(0, "Reload MobProto");
		m_vec_mobTable.clear();
	}

	m_vec_mobTable.resize(pRes->uiNumRows);

	int i = 0;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		col = 0;
		TMobTable* pTab = &m_vec_mobTable[i++];

		str_to_number(pTab->dwVnum, data[col++]);
		strlcpy(pTab->szName, data[col++], sizeof(pTab->szName));
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			strlcpy(pTab->szLocaleName[i], data[col++], sizeof(pTab->szLocaleName[i]));
		
		str_to_number(pTab->bRank, data[col++]);
		str_to_number(pTab->bType, data[col++]);
		str_to_number(pTab->bBattleType, data[col++]);
		str_to_number(pTab->bLevel, data[col++]);
		str_to_number(pTab->bSize, data[col++]);

		str_to_number(pTab->dwAIFlag, data[col++]);
		str_to_number(pTab->bMountCapacity, data[col++]);
		str_to_number(pTab->dwRaceFlag, data[col++]);
		str_to_number(pTab->dwImmuneFlag, data[col++]);
		str_to_number(pTab->bEmpire, data[col++]);
		strlcpy(pTab->szFolder, data[col++], sizeof(pTab->szFolder));
		str_to_number(pTab->bOnClickType, data[col++]);

		str_to_number(pTab->bStr, data[col++]);
		str_to_number(pTab->bDex, data[col++]);
		str_to_number(pTab->bCon, data[col++]);
		str_to_number(pTab->bInt, data[col++]);
		str_to_number(pTab->dwDamageRange[0], data[col++]);
		str_to_number(pTab->dwDamageRange[1], data[col++]);
		str_to_number(pTab->dwMaxHP, data[col++]);
		str_to_number(pTab->bRegenCycle, data[col++]);
		str_to_number(pTab->bRegenPercent, data[col++]);
		str_to_number(pTab->dwGoldMin, data[col++]);
		str_to_number(pTab->dwGoldMax, data[col++]);
		str_to_number(pTab->dwExp, data[col++]);
		str_to_number(pTab->wDef, data[col++]);
		str_to_number(pTab->sAttackSpeed, data[col++]);
		str_to_number(pTab->sMovingSpeed, data[col++]);

		str_to_number(pTab->bAggresiveHPPct, data[col++]);
		str_to_number(pTab->wAggressiveSight, data[col++]);
		str_to_number(pTab->wAttackRange, data[col++]);
		str_to_number(pTab->dwDropItemVnum, data[col++]);
		str_to_number(pTab->dwResurrectionVnum, data[col++]);

		for (int i = 0; i < MOB_ENCHANTS_MAX_NUM; ++i)
			str_to_number(pTab->cEnchants[i], data[col++]);
		for (int i = 0; i < MOB_RESISTS_MAX_NUM; ++i)
			str_to_number(pTab->cResists[i], data[col++]);

		str_to_number(pTab->fDamMultiply, data[col++]);
		str_to_number(pTab->dwSummonVnum, data[col++]);
		str_to_number(pTab->dwDrainSP, data[col++]);
		str_to_number(pTab->dwMobColor, data[col++]);
		str_to_number(pTab->dwPolymorphItemVnum, data[col++]);

		for (int i = 0; i < MOB_SKILL_MAX_NUM; ++i)
		{
			str_to_number(pTab->Skills[i].bLevel, data[col++]);
			str_to_number(pTab->Skills[i].dwVnum, data[col++]);
		}

		str_to_number(pTab->bBerserkPoint, data[col++]);
		str_to_number(pTab->bStoneSkinPoint, data[col++]);
		str_to_number(pTab->bGodSpeedPoint, data[col++]);
		str_to_number(pTab->bDeathBlowPoint, data[col++]);
		str_to_number(pTab->bRevivePoint, data[col++]);

		sys_log(0, "MOB #%-5d %-24s %-24s level: %-3u rank: %u empire: %d",
			pTab->dwVnum, pTab->szName, pTab->szLocaleName[LANGUAGE_DEFAULT], pTab->bLevel, pTab->bRank, pTab->bEmpire);
	}

	sort(m_vec_mobTable.begin(), m_vec_mobTable.end(), FCompareVnum());
	return true;*/
}

bool CClientManager::InitializeShopTable()
{
	MYSQL_ROW	data;
	int		col;

	static const char * s_szQuery = 
		"SELECT "
		"shop.vnum, "
		"shop.npc_vnum, "
		"shop_item.item_vnum, "
		"shop_item.count, "
		"shop_item.price_vnum, "
		"shop_item.price_count "
#ifdef SECOND_ITEM_PRICE
		", shop_item.price_vnum2, "
		"shop_item.price_count2 "
#endif
		"FROM shop LEFT JOIN shop_item "
		"ON shop.vnum = shop_item.shop_vnum ORDER BY shop.vnum, shop_item.order_id ASC, shop_item.item_vnum, shop_item.count";

	std::auto_ptr<SQLMsg> pkMsg2(CDBManager::instance().DirectQuery(s_szQuery));

	// shop�� vnum�� �ִµ� shop_item �� �������... ���з� ó���Ǵ� ���� ���.
	// ��ó���Һκ�
	SQLResult * pRes2 = pkMsg2->Get();

	if (!pRes2->uiNumRows)
	{
		sys_err("InitializeShopTable : Table count is zero.");
		return false;
	}

	std::map<int, TShopTable *> map_shop;

	if (m_pShopTable)
	{
		delete [] (m_pShopTable);
		m_pShopTable = NULL;
	}

	TShopTable * shop_table = m_pShopTable;

	while ((data = mysql_fetch_row(pRes2->pSQLResult)))
	{
		col = 0;

		int iShopVnum = 0;
		str_to_number(iShopVnum, data[col++]);

		if (map_shop.end() == map_shop.find(iShopVnum))
		{
			shop_table = new TShopTable;
			shop_table->set_vnum(iShopVnum);

			map_shop[iShopVnum] = shop_table;
		}
		else
			shop_table = map_shop[iShopVnum];

		shop_table->set_npc_vnum(atoi(data[col++]));

		if (!data[col])	// �������� �ϳ��� ������ NULL�� ���� �ǹǷ�..
			continue;

		shop_table->set_item_count(shop_table->item_count() + 1);
		auto item = shop_table->add_items();
		auto itemData = item->mutable_item();

		itemData->set_vnum(std::stoi(data[col++]));
		itemData->set_count(std::stoi(data[col++]));

		item->set_price_item_vnum(std::stoi(data[col++]));
		itemData->set_price(std::stoll(data[col++]));

#ifdef SECOND_ITEM_PRICE
		item->set_price_item_vnum2(std::stoi(data[col++]));
		item->set_price2(std::stoi(data[col++]));
#endif

/*		auto it = m_map_itemTableByVnum.find(itemData->vnum());
		if (it != m_map_itemTableByVnum.end())
			it->second->bIsSelling = true;
*/
	}

	m_pShopTable = new TShopTable[map_shop.size()];
	m_iShopTableSize = map_shop.size();

	typeof(map_shop.begin()) it = map_shop.begin();

	int i = 0;

	while (it != map_shop.end())
	{
		m_pShopTable[i] = *(it++)->second;
		sys_log(0, "SHOP: #%d items: %d", (m_pShopTable + i)->vnum(), (m_pShopTable + i)->item_count());
		++i;
	}

	return true;
}

bool CClientManager::InitializeItemTableFromTextFile()
{
	//================== �Լ� ���� ==================//
	//1. ��� : 'item_proto.txt', 'item_proto_test.txt', 'item_names.txt' ������ �а�,
	//		<item_table>(TItemTable), <m_map_itemTableByVnum> ������Ʈ�� �����Ѵ�.
	//2. ����
	//	1) 'item_names.txt' ������ �о (a)[localMap](vnum:name) ���� �����.
	//	2) 'item_proto_text.txt'���ϰ� (a)[localMap] ������
	//		(b)[test_map_itemTableByVnum](vnum:TItemTable) ���� �����Ѵ�.
	//	3) 'item_proto.txt' ���ϰ�  (a)[localMap] ������
	//		(!)[item_table], <m_map_itemTableByVnum>�� �����.
	//			<����>
	//			�� row �� ��, 
	//			(b)[test_map_itemTableByVnum],(!)[mob_table] ��ο� �ִ� row��
	//			(b)[test_map_itemTableByVnum]�� ���� ����Ѵ�.
	//	4) (b)[test_map_itemTableByVnum]�� row��, (!)[item_table]�� ���� ���� �߰��Ѵ�.
	//3. �׽�Ʈ
	//	1)'item_proto.txt' ������ item_table�� �� ������. -> �Ϸ�
	//	2)'item_names.txt' ������ item_table�� �� ������.
	//	3)'item_proto_test.txt' ���� [��ġ��] ������ item_table �� �� ������.
	//	4)'item_proto_test.txt' ���� [���ο�] ������ item_table �� �� ������.
	//	5) (����) ���� Ŭ���̾�Ʈ���� ����� �۵� �ϴ���.
	//_______________________________________________//



	//=================================================================================//
	//	1) 'item_names.txt' ������ �о (a)[localMap](vnum:name) ���� �����.
	//=================================================================================//
	map<int, const char*> localMap[LANGUAGE_MAX_NUM];
	bool isNameFile[LANGUAGE_MAX_NUM];
	cCsvTable nameData[LANGUAGE_MAX_NUM];
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		isNameFile[i] = true;

		char szFileName[FILENAME_MAX];
		snprintf(szFileName, sizeof(szFileName), "locale/%s/item_names.txt", strlower(astLocaleStringNames[i].c_str()));

		if (!nameData[i].Load(szFileName, '\t'))
		{
			fprintf(stderr, "item_names.txt ��AAIA�� A����i��AAo ����C����A��I��U\n");
			isNameFile[i] = false;
		}
		else {
			nameData[i].Next();	//������irow ��y����.
			while (nameData[i].Next()) {
				localMap[i][atoi(nameData[i].AsStringByIndex(0))] = nameData[i].AsStringByIndex(1);
			}
		}
	}
	//_________________________________________________________________//

	//=================================================================//
	//	2) 'item_proto_text.txt'���ϰ� (a)[localMap] ������
	//		(b)[test_map_itemTableByVnum](vnum:TItemTable) ���� �����Ѵ�.
	//=================================================================//
	map<DWORD, TItemTable *> test_map_itemTableByVnum;
	//1. ���� �о����.
	cCsvTable test_data;
	if(!test_data.Load("item_proto_test.txt",'\t'))
	{
		fprintf(stderr, "item_proto_test.txt ������ �о���� ���߽��ϴ�\n");
		//return false;
	} else {
		test_data.Next();	//���� �ο� �Ѿ��.

		//2. �׽�Ʈ ������ ���̺� ����.
		TItemTable * test_item_table = NULL;
		int test_itemTableSize = test_data.m_File.GetRowCount()-1;
		test_item_table = new TItemTable[test_itemTableSize];

		//3. �׽�Ʈ ������ ���̺� ���� �ְ�, �ʿ����� �ֱ�.
		while(test_data.Next()) {


			if (!Set_Proto_Item_Table(test_item_table, test_data, localMap))
			{
				fprintf(stderr, "������ ������ ���̺� ���� ����.\n");			
			}

			test_map_itemTableByVnum.insert(std::map<DWORD, TItemTable *>::value_type(test_item_table->vnum(), test_item_table));
			test_item_table++;

		}
	}
	//______________________________________________________________________//


	//========================================================================//
	//	3) 'item_proto.txt' ���ϰ�  (a)[localMap] ������
	//		(!)[item_table], <m_map_itemTableByVnum>�� �����.
	//			<����>
	//			�� row �� ��, 
	//			(b)[test_map_itemTableByVnum],(!)[mob_table] ��ο� �ִ� row��
	//			(b)[test_map_itemTableByVnum]�� ���� ����Ѵ�.
	//========================================================================//

	//vnum���� ������ ��. ���ο� �׽�Ʈ �������� �Ǻ��Ҷ� ���ȴ�.
	set<int> vnumSet;

	//���� �о����.
	cCsvTable data;
	if(!data.Load("item_proto.txt",'\t'))
	{
		fprintf(stderr, "item_proto.txt ������ �о���� ���߽��ϴ�\n");
		return false;
	}

	data.Next(); //�� ���� ���� (������ Į���� �����ϴ� �κ�)

	if (!m_vec_itemTable.empty())
	{
		sys_log(0, "RELOAD: item_proto");
		m_vec_itemTable.clear();
		m_map_itemTableByVnum.clear();
	}

	//===== ������ ���̺� ���� =====//
	//���� �߰��Ǵ� ������ �ľ��Ѵ�.
	int addNumber = 0;
	while(data.Next()) {
		int vnum = atoi(data.AsStringByIndex(0));
		std::map<DWORD, TItemTable *>::iterator it_map_itemTable;
		it_map_itemTable = test_map_itemTableByVnum.find(vnum);
		if(it_map_itemTable != test_map_itemTableByVnum.end()) {
			addNumber++;
		}
	}
	//data�� �ٽ� ù�ٷ� �ű��.(�ٽ� �о�´�;;)
	data.Destroy();
	if(!data.Load("item_proto.txt",'\t'))
	{
		fprintf(stderr, "item_proto.txt ������ �о���� ���߽��ϴ�\n");
		return false;
	}
	data.Next(); //�� ���� ���� (������ Į���� �����ϴ� �κ�)

	m_vec_itemTable.resize(data.m_File.GetRowCount() - 1 + addNumber);
	
	TItemTable * item_table = &m_vec_itemTable[0];

	while (data.Next())
	{
		int col = 0;

		std::map<DWORD, TItemTable *>::iterator it_map_itemTable;
		it_map_itemTable = test_map_itemTableByVnum.find(atoi(data.AsStringByIndex(col)));
		if(it_map_itemTable == test_map_itemTableByVnum.end()) {
			//�� Į�� ������ ����
			
			if (!Set_Proto_Item_Table(item_table, data, localMap))
			{
				fprintf(stderr, "������ ������ ���̺� ���� ����.\n");			
			}


			
		} else {	//$$$$$$$$$$$$$$$$$$$$$$$ �׽�Ʈ ������ ������ �ִ�!	
			TItemTable *tempTable = it_map_itemTable->second;

			item_table->set_vnum(tempTable->vnum());
			item_table->set_name(tempTable->name());
			for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
				item_table->set_locale_name(i, tempTable->locale_name(i));
			item_table->set_type(tempTable->type());
			item_table->set_sub_type(tempTable->sub_type());
			item_table->set_size(tempTable->size());
			item_table->set_anti_flags(tempTable->anti_flags());
			item_table->set_flags(tempTable->flags());
			item_table->set_wear_flags(tempTable->wear_flags());
			item_table->set_immune_flags(tempTable->immune_flags());
			item_table->set_gold(tempTable->gold());
			item_table->set_shop_buy_price(tempTable->shop_buy_price());
			item_table->set_refined_vnum(tempTable->refined_vnum());
			item_table->set_refine_set(tempTable->refine_set());;
			item_table->set_alter_to_magic_item_pct(tempTable->alter_to_magic_item_pct());
			item_table->set_limit_real_time_first_use_index(-1);
			item_table->set_limit_timer_based_on_wear_index(-1);

			int i;

			for (i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
			{
				auto limit = item_table->add_limits();
				*limit = tempTable->limits(i);

				if (LIMIT_REAL_TIME_START_FIRST_USE == limit->type())
					item_table->set_limit_real_time_first_use_index(i);

				if (LIMIT_TIMER_BASED_ON_WEAR == limit->type())
					item_table->set_limit_timer_based_on_wear_index(i);
			}

			for (i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
				*item_table->add_applies() = tempTable->applies(i);

			for (i = 0; i < ITEM_VALUES_MAX_NUM; ++i)
				item_table->set_values(i, tempTable->values(i));

			item_table->set_gain_socket_pct(tempTable->gain_socket_pct());
			item_table->set_addon_type(tempTable->addon_type());

			item_table->set_weight(tempTable->weight());

		}
		vnumSet.insert(item_table->vnum());
		m_map_itemTableByVnum.insert(std::map<DWORD, TItemTable *>::value_type(item_table->vnum(), item_table));
		++item_table;
	}
	//_______________________________________________________________________//

	//========================================================================//
	//	4) (b)[test_map_itemTableByVnum]�� row��, (!)[item_table]�� ���� ���� �߰��Ѵ�.
	//========================================================================//
	test_data.Destroy();
	if(!test_data.Load("item_proto_test.txt",'\t'))
	{
		fprintf(stderr, "item_proto_test.txt ������ �о���� ���߽��ϴ�\n");
		//return false;
	} else {
		test_data.Next();	//���� �ο� �Ѿ��.

		while (test_data.Next())	//�׽�Ʈ ������ ������ �Ⱦ����,���ο� ���� �߰��Ѵ�.
		{
			//�ߺ��Ǵ� �κ��̸� �Ѿ��.
			set<int>::iterator itVnum;
			itVnum=vnumSet.find(atoi(test_data.AsStringByIndex(0)));
			if (itVnum != vnumSet.end()) {
				continue;
			}
			
			if (!Set_Proto_Item_Table(item_table, test_data, localMap))
			{
				fprintf(stderr, "������ ������ ���̺� ���� ����.\n");			
			}


			m_map_itemTableByVnum.insert(std::map<DWORD, TItemTable *>::value_type(item_table->vnum(), item_table));

			item_table++;

		}
	}



	// QUEST_ITEM_PROTO_DISABLE
	// InitializeQuestItemTable();
	// END_OF_QUEST_ITEM_PROTO_DISABLE

	m_map_itemTableByVnum.clear();

	auto it = m_vec_itemTable.begin();

	while (it != m_vec_itemTable.end())
	{
		TItemTable * item_table = &(*(it++));

		sys_log(0, "ITEM: #%-5lu %-24s %-24s VAL: %d %d %d %d %d %d WEAR %u ANTI %u IMMUNE %u REFINE %u REFINE_SET %u MAGIC_PCT %u SHOP_BUY_PRICE %u GOLD %u", 
				item_table->vnum(),
				item_table->name().c_str(),
				item_table->locale_name(LANGUAGE_DEFAULT).c_str(),
				item_table->values(0),
				item_table->values(1),
				item_table->values(2),
				item_table->values(3),
				item_table->values(4),
				item_table->values(5),
				item_table->wear_flags(),
				item_table->anti_flags(),
				item_table->immune_flags(),
				item_table->refined_vnum(),
				item_table->refine_set(),
				item_table->alter_to_magic_item_pct(),
				item_table->shop_buy_price(),
				item_table->gold());
	}

	sort(m_vec_itemTable.begin(), m_vec_itemTable.end(), [](const TItemTable& a, const TItemTable& b) {
		return a.vnum() < b.vnum();
	});

	for (int i = 0; i < m_vec_itemTable.size(); ++i)
		m_map_itemTableByVnum.insert(std::pair<DWORD, TItemTable*>(m_vec_itemTable[i].vnum(), &m_vec_itemTable[i]));

	// Append m_map_itemTableByVnum with range based items
	for(auto & item : m_map_itemTableByVnum)
	{
		auto itemTable = item.second;

		// Item has range
		if(itemTable->vnum_range() > 0)
		{
			// Add more vnums to the same item-table
			for(int i = 0; i < itemTable->vnum_range(); ++i)
			{
				DWORD newItemVnum = itemTable->vnum() + i + 1;

				// Insert new Vnum to array
				m_map_itemTableByVnum.insert(std::map<DWORD, TItemTable *>::value_type(newItemVnum, itemTable));
			}
		}
	}

	return true;
}


bool CClientManager::InitializeItemTableFromDatabase()
{
	return false;
	/*
	MYSQL_ROW	data;
	int			col;

	char szQuery[QUERY_MAX_LEN];
	int iQueryLen = snprintf(szQuery, sizeof(szQuery), "SELECT vnum, name, ");
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen, "locale_name_%s, ", strlower(astLocaleStringNames[i].c_str()));
	iQueryLen += snprintf(szQuery + iQueryLen, sizeof(szQuery) - iQueryLen,
		"type+0, subtype, weight, size, antiflag+0, flag+0, wearflag+0, immuneflag+0, gold, "
		"shop_buy_price, refined_vnum, refine_set, magic_pct, limittype0+0, limitvalue0, limittype1+0, limitvalue1, "
		"applytype0+0, applyvalue0, applytype1+0, applyvalue1, applytype2+0, applyvalue2, value0, value1, value2, value3, value4, value5, "
		"socket0, socket1, socket2, specular, socket_pct, addon_type FROM item_proto");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(szQuery));

	SQLResult* pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("InitializeItemTable : Table count is zero.");
		return false;
	}

	if (!m_vec_itemTable.empty())
	{
		sys_log(0, "Reload ItemProto");
		m_vec_itemTable.clear();
	}

	m_vec_itemTable.resize(pRes->uiNumRows);

	int i = 0;
	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		col = 0;
		TItemTable* pTab = &m_vec_itemTable[i++];

		str_to_number(pTab->dwVnum, data[col++]);
		strlcpy(pTab->szName, data[col++], sizeof(pTab->szName));
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			strlcpy(pTab->szLocaleName[i], data[col++], sizeof(pTab->szLocaleName[i]));

		str_to_number(pTab->bType, data[col++]);
		str_to_number(pTab->bSubType, data[col++]);
		str_to_number(pTab->bWeight, data[col++]);
		str_to_number(pTab->bSize, data[col++]);

		str_to_number(pTab->dwAntiFlags, data[col++]);
		str_to_number(pTab->dwFlags, data[col++]);
		str_to_number(pTab->dwWearFlags, data[col++]);
		str_to_number(pTab->dwImmuneFlag, data[col++]);

		str_to_number(pTab->dwGold, data[col++]);
		str_to_number(pTab->dwShopBuyPrice, data[col++]);

		str_to_number(pTab->dwRefinedVnum, data[col++]);
		str_to_number(pTab->wRefineSet, data[col++]);
		str_to_number(pTab->bAlterToMagicItemPct, data[col++]);

		pTab->cLimitRealTimeFirstUseIndex = -1;
		pTab->cLimitTimerBasedOnWearIndex = -1;
		for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
		{
			str_to_number(pTab->aLimits[i].bType, data[col++]);
			str_to_number(pTab->aLimits[i].lValue, data[col++]);

			if (LIMIT_REAL_TIME_START_FIRST_USE == pTab->aLimits[i].bType)
				pTab->cLimitRealTimeFirstUseIndex = (char)i;

			if (LIMIT_TIMER_BASED_ON_WEAR == pTab->aLimits[i].bType)
				pTab->cLimitTimerBasedOnWearIndex = (char)i;
		}

		for (int i = 0; i < ITEM_APPLY_MAX_NUM; ++i)
		{
			str_to_number(pTab->aApplies[i].bType, data[col++]);
			str_to_number(pTab->aApplies[i].lValue, data[col++]);
		}
		for (int i = 0; i < ITEM_VALUES_MAX_NUM; ++i)
			str_to_number(pTab->alValues[i], data[col++]);
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			str_to_number(pTab->alSockets[i], data[col++]);

		str_to_number(pTab->bSpecular, data[col++]);
		str_to_number(pTab->bGainSocketPct, data[col++]);
		str_to_number(pTab->sAddonType, data[col++]);

		m_map_itemTableByVnum.insert(std::pair<DWORD, TItemTable*>(pTab->dwVnum, pTab));

		sys_log(0, "ITEM: #%-5lu %-24s %-24s VAL: %ld %ld %ld %ld %ld %ld WEAR %lu ANTI %lu IMMUNE %lu REFINE %lu REFINE_SET %u MAGIC_PCT %u",
			pTab->dwVnum,
			pTab->szName,
			pTab->szLocaleName[LANGUAGE_DEFAULT],
			pTab->alValues[0],
			pTab->alValues[1],
			pTab->alValues[2],
			pTab->alValues[3],
			pTab->alValues[4],
			pTab->alValues[5],
			pTab->dwWearFlags,
			pTab->dwAntiFlags,
			pTab->dwImmuneFlag,
			pTab->dwRefinedVnum,
			pTab->wRefineSet,
			pTab->bAlterToMagicItemPct);
	}

	sort(m_vec_itemTable.begin(), m_vec_itemTable.end(), FCompareVnum());
	return true;*/
}

bool CClientManager::InitializeSkillTable()
{
	char query[4096];
	int len = snprintf(query, sizeof(query),
		"SELECT dwVnum, szName, bType, bMaxLevel, dwSplashRange, "
		"szPointOn, szPointPoly, szSPCostPoly, szDurationPoly, szDurationSPCostPoly, "
		"szCooldownPoly, szMasterBonusPoly, IFNULL(setFlag+0,0), IFNULL(setAffectFlag+0,0), ");
#ifdef __LEGENDARY_SKILL__
	len += snprintf(query + len, sizeof(query) - len,
		"IFNULL(setAffectFlagLegendary+0,0), ");
#endif
	len += snprintf(query + len, sizeof(query) - len,
		"szPointOn2, szPointPoly2, szDurationPoly2, IFNULL(setAffectFlag2+0,0), ");
#ifdef __LEGENDARY_SKILL__
	len += snprintf(query + len, sizeof(query) - len,
		"IFNULL(setAffectFlag2Legendary+0,0), ");
#endif
	len += snprintf(query + len, sizeof(query) - len,
		"szPointOn3, szPointPoly3, szDurationPoly3, szGrandMasterAddSPCostPoly, "
		"bLevelStep, bLevelLimit, prerequisiteSkillVnum, prerequisiteSkillLevel, iMaxHit, szSplashAroundDamageAdjustPoly, eSkillType+0, dwTargetRange "
		"FROM skill_proto ORDER BY dwVnum");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from skill_proto");
		return false;
	}

	if (!m_vec_skillTable.empty())
	{
		sys_log(0, "RELOAD: skill_proto");
		m_vec_skillTable.clear();
	}

	m_vec_skillTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;
	int		col;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TSkillTable t;

		col = 0;

		t.set_vnum(std::stoll(data[col++]));
		t.set_name(data[col++]);
		t.set_type(std::stoll(data[col++]));
		t.set_max_level(std::stoll(data[col++]));
		t.set_splash_range(std::stoll(data[col++]));
		t.set_point_on(data[col++]);
		t.set_point_poly(data[col++]);
		t.set_sp_cost_poly(data[col++]);
		t.set_duration_poly(data[col++]);
		
		t.set_duration_sp_cost_poly(data[col++]);
		t.set_cooldown_poly(data[col++]);
		t.set_master_bonus_poly(data[col++]);
		t.set_flag(std::stoll(data[col++]));
		t.set_affect_flag(std::stoll(data[col++]));
		
#ifdef __LEGENDARY_SKILL__
		t.set_affect_flag_legendary(std::stoll(data[col++]));
#endif

		t.set_point_on2(data[col++]);
		t.set_point_poly2(data[col++]);
		t.set_duration_poly2(data[col++]);

		t.set_affect_flag2(std::stoll(data[col++]));
#ifdef __LEGENDARY_SKILL__
		t.set_affect_flag2_legendary(std::stoll(data[col++]));
#endif

		// ADD_GRANDMASTER_SKILL
		t.set_point_on3(data[col++]);
		t.set_point_poly3(data[col++]);
		t.set_duration_poly3(data[col++]);
		t.set_grand_master_add_sp_cost_poly(data[col++]);
		// END_OF_ADD_GRANDMASTER_SKILL


		t.set_level_step(std::stoll(data[col++]));
		t.set_level_limit(std::stoll(data[col++]));
		t.set_pre_skill_vnum(std::stoll(data[col++]));
		t.set_pre_skill_level(std::stoll(data[col++]));
		
		t.set_max_hit(std::stoll(data[col++]));

		t.set_splash_around_damage_adjust_poly(data[col++]);

		t.set_skill_attr_type(std::stoll(data[col++]));
		t.set_target_range(std::stoll(data[col++]));
		
		sys_log(0, "SKILL: #%d %s flag %u point %s affect %u cooldown %s",
			t.vnum(), t.name().c_str(), t.flag(), t.point_on().c_str(), t.affect_flag(), t.cooldown_poly().c_str());

		m_vec_skillTable.push_back(t);
	}

	return true;
}

#ifdef SOUL_SYSTEM
void CClientManager::InitializeSoulProtoTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
		"SELECT vnum, type-1, apply_type-1, value0, value1, value2, value3 FROM soul_proto");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from soul_proto");
		return;
	}

	if (!m_vec_soulAttrTable.empty())
	{
		sys_log(0, "RELOAD: soul_attr");
		m_vec_soulAttrTable.clear();
	}

	m_vec_soulAttrTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TSoulProtoTable t;

		int col = 0;

		t.set_vnum(atoi(data[col++]));
		t.set_soul_type(atoi(data[col++]));
		t.set_apply_type(atoi(data[col++]));
		
		for (int i = 0; i < SOUL_TABLE_MAX_APPLY_COUNT; ++i)
			t.add_apply_values(atoi(data[col++]));

		sys_log(0, "SOUL_ATTR: vnum: %d soultype: %u type: %u, vals: %d %d %d %d",
			t.vnum(),
			t.soul_type(),
			t.apply_type(),
			t.apply_values(0),
			t.apply_values(1),
			t.apply_values(2),
			t.apply_values(3));

		m_vec_soulAttrTable.push_back(t);
	}
}
#endif

#ifdef CRYSTAL_SYSTEM
void CClientManager::InitializeCrystalProtoTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
		"SELECT process_level, clarity_type-1, clarity_level, "
		"apply_type1+0, apply_value1, "
		"apply_type2+0, apply_value2, "
		"apply_type3+0, apply_value3, "
		"apply_type4+0, apply_value4, "
		"required_fragments FROM crystal_proto");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from soul_proto");
		return;
	}

	if (!m_vec_crystalTable.empty())
	{
		sys_log(0, "RELOAD: soul_attr");
		m_vec_crystalTable.clear();
	}

	m_vec_crystalTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TCrystalProto t;

		int col = 0;

		t.set_process_level(atoi(data[col++]));
		t.set_clarity_type(atoi(data[col++]));
		t.set_clarity_level(atoi(data[col++]));

		for (int i = 0; i < 4; ++i)
		{
			auto apply = t.add_applies();
			apply->set_type(atoi(data[col++]));
			apply->set_value(atoi(data[col++]));
		}

		t.set_required_fragments(atoi(data[col++]));

		sys_log(0, "CRYSTAL_PROTO: process: %d clarity_type: %u clarity_level: %u",
			t.process_level(),
			t.clarity_type(),
			t.clarity_level());

		m_vec_crystalTable.push_back(t);
	}
}
#endif

bool CClientManager::InitializeItemAttrTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, lv4, lv5, weapon, body, wrist, foots, neck, head, shield, ear, totem FROM item_attr ORDER BY apply");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from item_attr");
		return false;
	}

	if (!m_vec_itemAttrTable.empty())
	{
		sys_log(0, "RELOAD: item_attr");
		m_vec_itemAttrTable.clear();
	}

	m_vec_itemAttrTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		int col = 0;
		t.set_apply(data[col++]);
		t.set_apply_index(atoi(data[col++]));
		t.set_prob(atoi(data[col++]));

		t.add_values(atoi(data[col++]));
		t.add_values(atoi(data[col++]));
		t.add_values(atoi(data[col++]));
		t.add_values(atoi(data[col++]));
		t.add_values(atoi(data[col++]));

		for (int i = 0; i < ATTRIBUTE_SET_MAX_NUM; ++i)
			t.add_max_level_by_set(0);

		t.set_max_level_by_set(ATTRIBUTE_SET_WEAPON, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_BODY, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_WRIST, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_FOOTS, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_NECK, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_HEAD, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_SHIELD, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_EAR, atoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_TOTEM, atoi(data[col++]));

		sys_log(0, "ITEM_ATTR: %-20s %4lu { %3d %3d %3d %3d %3d } { %d %d %d %d %d %d %d %d }",
				t.apply().c_str(),
				t.prob(),
				t.values(0),
				t.values(1),
				t.values(2),
				t.values(3),
				t.values(4),
				t.max_level_by_set(ATTRIBUTE_SET_WEAPON),
				t.max_level_by_set(ATTRIBUTE_SET_BODY),
				t.max_level_by_set(ATTRIBUTE_SET_WRIST),
				t.max_level_by_set(ATTRIBUTE_SET_FOOTS),
				t.max_level_by_set(ATTRIBUTE_SET_NECK),
				t.max_level_by_set(ATTRIBUTE_SET_HEAD),
				t.max_level_by_set(ATTRIBUTE_SET_SHIELD),
				t.max_level_by_set(ATTRIBUTE_SET_EAR),
				t.max_level_by_set(ATTRIBUTE_SET_TOTEM));

		m_vec_itemAttrTable.push_back(t);
	}

	return true;
}

#ifdef EL_COSTUME_ATTR
bool CClientManager::InitializeItemCostumeAttrTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, 1to3, 4to5, weapon, body, head FROM item_attr_costume WHERE (weapon>0 OR body>0 OR head>0) AND (1to3>0 OR 4to5>0) ORDER BY apply");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from item_attr_costume");
		return false;
	}

	if (!m_vec_itemCostumeAttrTable.empty())
	{
		sys_log(0, "RELOAD: item_attr_costume");
		m_vec_itemCostumeAttrTable.clear();
	}

	m_vec_itemCostumeAttrTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		int col = 0;

		t.set_apply(data[col++]);
		t.set_apply_index(std::stoi(data[col++]));
		t.set_prob(std::stoi(data[col++]));
		t.add_values(std::stoi(data[col++]));
		t.add_values(std::stoi(data[col++]));
		t.add_values(std::stoi(data[col++]));
		t.add_values(std::stoi(data[col++]));
		t.add_values(std::stoi(data[col++]));

		for (int i = 0; i < ATTRIBUTE_SET_MAX_NUM; ++i)
			t.add_max_level_by_set(0);

		t.set_max_level_by_set(ATTRIBUTE_SET_WEAPON, std::stoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_BODY, std::stoi(data[col++]));
		t.set_max_level_by_set(ATTRIBUTE_SET_HEAD, std::stoi(data[col++]));

		sys_log(0, "ITEM_ATTR_COSTUME: %-20s %4lu { %3d %3d %3d } [1to3 %d 4to5 %d] { %d %d %d }",
				t.apply().c_str(),
				t.prob(),
				t.values(0),
				t.values(1),
				t.values(2),
				t.values(3),
				t.values(4),
				t.max_level_by_set(ATTRIBUTE_SET_WEAPON),
				t.max_level_by_set(ATTRIBUTE_SET_BODY),
				t.max_level_by_set(ATTRIBUTE_SET_HEAD));

		m_vec_itemCostumeAttrTable.push_back(t);
	}

	return true;
}
#endif

#ifdef ITEM_RARE_ATTR
bool CClientManager::InitializeItemRareTable()
{
	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT apply, apply+0, prob, lv1, lv2, lv3, lv4, lv5, weapon, body, wrist, foots, neck, head, shield, ear, totem FROM item_attr_rare ORDER BY apply");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_err("no result from item_attr_rare");
		return false;
	}

	if (!m_vec_itemRareTable.empty())
	{
		sys_log(0, "RELOAD: item_attr_rare");
		m_vec_itemRareTable.clear();
	}

	m_vec_itemRareTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		TItemAttrTable t;

		int col = 0;

		strlcpy(t.szApply, data[col++], sizeof(t.szApply));
		str_to_number(t.dwApplyIndex, data[col++]);
		str_to_number(t.dwProb, data[col++]);
		str_to_number(t.lValues[0], data[col++]);
		str_to_number(t.lValues[1], data[col++]);
		str_to_number(t.lValues[2], data[col++]);
		str_to_number(t.lValues[3], data[col++]);
		str_to_number(t.lValues[4], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_BODY], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_NECK], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_EAR], data[col++]);
		str_to_number(t.bMaxLevelBySet[ATTRIBUTE_SET_TOTEM], data[col++]);

		sys_log(0, "ITEM_RARE: %-20s %4lu { %3d %3d %3d %3d %3d } { %d %d %d %d %d %d %d %d }",
				t.szApply,
				t.dwProb,
				t.lValues[0],
				t.lValues[1],
				t.lValues[2],
				t.lValues[3],
				t.lValues[4],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WEAPON],
				t.bMaxLevelBySet[ATTRIBUTE_SET_BODY],
				t.bMaxLevelBySet[ATTRIBUTE_SET_WRIST],
				t.bMaxLevelBySet[ATTRIBUTE_SET_FOOTS],
				t.bMaxLevelBySet[ATTRIBUTE_SET_NECK],
				t.bMaxLevelBySet[ATTRIBUTE_SET_HEAD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_SHIELD],
				t.bMaxLevelBySet[ATTRIBUTE_SET_EAR],
				t.bMaxLevelBySet[ATTRIBUTE_SET_TOTEM]);

		m_vec_itemRareTable.push_back(t);
	}

	return true;
}
#endif

bool CClientManager::InitializeLandTable()
{
	using namespace building;

	char query[4096];

	snprintf(query, sizeof(query),
		"SELECT id, map_index, x, y, width, height, guild_id, guild_level_limit, price "
		"FROM land WHERE enable='YES' ORDER BY id");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_vec_kLandTable.empty())
	{
		sys_log(0, "RELOAD: land");
		m_vec_kLandTable.clear();
	}

	m_vec_kLandTable.reserve(pRes->uiNumRows);

	MYSQL_ROW	data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			TBuildingLand t;

			int col = 0;

			t.set_id(std::stoi(data[col++]));
			t.set_map_index(std::stoi(data[col++]));
			t.set_x(std::stoi(data[col++]));
			t.set_y(std::stoi(data[col++]));
			t.set_width(std::stoi(data[col++]));
			t.set_height(std::stoi(data[col++]));
			t.set_guild_id(std::stoi(data[col++]));
			t.set_guild_level_limit(std::stoi(data[col++]));
			t.set_price(std::stoll(data[col++]));

			sys_log(0, "LAND: %lu map %-4ld %7ldx%-7ld w %-4ld h %-4ld", t.id(), t.map_index(), t.x(), t.y(), t.width(), t.height());

			m_vec_kLandTable.push_back(t);
		}

	return true;
}

void parse_pair_number_string(const char * c_pszString, std::vector<std::pair<int, int> > & vec)
{
	// format: 10,1/20,3/300,50
	const char * t = c_pszString;
	const char * p = strchr(t, '/');
	std::pair<int, int> k;

	char szNum[32 + 1];
	char * comma;

	while (p)
	{
		if (isnhdigit(*t))
		{
			strlcpy(szNum, t, MIN(sizeof(szNum), (p-t)+1));

			comma = strchr(szNum, ',');

			if (comma)
			{
				*comma = '\0';
				str_to_number(k.second, comma+1);
			}
			else
				k.second = 0;

			str_to_number(k.first, szNum);
			vec.push_back(k);
		}

		t = p + 1;
		p = strchr(t, '/');
	}

	if (isnhdigit(*t))
	{
		strlcpy(szNum, t, sizeof(szNum));

		comma = strchr(const_cast<char*>(t), ',');

		if (comma)
		{
			*comma = '\0';
			str_to_number(k.second, comma+1);
		}
		else
			k.second = 0;

		str_to_number(k.first, szNum);
		vec.push_back(k);
	}
}

bool CClientManager::InitializeObjectProto()
{
	using namespace building;

	char query[4096];
	snprintf(query, sizeof(query),
			"SELECT vnum, price, materials, upgrade_vnum, upgrade_limit_time, life, reg_1, reg_2, reg_3, reg_4, npc, group_vnum, dependent_group "
			"FROM object_proto ORDER BY vnum");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_vec_kObjectProto.empty())
	{
		sys_log(0, "RELOAD: object_proto");
		m_vec_kObjectProto.clear();
	}

	m_vec_kObjectProto.reserve(MAX(0, pRes->uiNumRows));

	MYSQL_ROW	data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			network::TBuildingObjectProto t;

			int col = 0;

			t.set_vnum(std::stoi(data[col++]));
			t.set_price(std::stoll(data[col++]));

			std::vector<std::pair<int, int> > vec;
			parse_pair_number_string(data[col++], vec);

			for (unsigned int i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
			{
				auto mat = t.add_materials();

				if (i < vec.size())
				{
					std::pair<int, int>& r = vec[i];

					mat->set_item_vnum(r.first);
					mat->set_count(r.second);
				}
			}

			t.set_upgrade_vnum(std::stoi(data[col++]));
			t.set_upgrade_limit_time(std::stoi(data[col++]));
			t.set_life(std::stoi(data[col++]));
			t.add_region(std::stoi(data[col++]));
			t.add_region(std::stoi(data[col++]));
			t.add_region(std::stoi(data[col++]));
			t.add_region(std::stoi(data[col++]));
			
			// ADD_BUILDING_NPC
			t.set_npc_vnum(std::stoi(data[col++]));
			t.set_group_vnum(std::stoi(data[col++]));
			t.set_depend_on_group_vnum(std::stoi(data[col++]));
			
			t.set_npc_x(0);
			t.set_npc_y(MAX(t.region(1), t.region(3)) + 300);
			// END_OF_ADD_BUILDING_NPC

			sys_log(0, "OBJ_PROTO: vnum %lu price %lu mat %lu %lu",
					t.vnum(), t.price(), t.materials(0).item_vnum(), t.materials(0).count());

			m_vec_kObjectProto.push_back(t);
		}

	return true;
}

bool CClientManager::InitializeObjectTable()
{
	using namespace building;

	char query[4096];
	snprintf(query, sizeof(query), "SELECT id, land_id, vnum, map_index, x, y, x_rot, y_rot, z_rot, life FROM object ORDER BY id");

	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery(query));
	SQLResult * pRes = pkMsg->Get();

	if (!m_map_pkObjectTable.empty())
	{
		sys_log(0, "RELOAD: object");
		m_map_pkObjectTable.clear();
	}

	MYSQL_ROW data;

	if (pRes->uiNumRows > 0)
		while ((data = mysql_fetch_row(pRes->pSQLResult)))
		{
			auto k = std::unique_ptr<TBuildingObject>(new TBuildingObject());

			int col = 0;

			k->set_id(std::stoi(data[col++]));
			k->set_land_id(std::stoi(data[col++]));
			k->set_vnum(std::stoi(data[col++]));
			k->set_map_index(std::stoi(data[col++]));
			k->set_x(std::stoi(data[col++]));
			k->set_y(std::stoi(data[col++]));
			k->set_x_rot(std::stoi(data[col++]));
			k->set_y_rot(std::stoi(data[col++]));
			k->set_z_rot(std::stoi(data[col++]));
			k->set_life(std::stoi(data[col++]));
			
			sys_log(0, "OBJ: %lu vnum %lu map %-4ld %7ldx%-7ld life %d", 
					k->id(), k->vnum(), k->map_index(), k->x(), k->y(), k->life());

			m_map_pkObjectTable.insert(std::make_pair(k->id(), std::move(k)));
		}

	return true;
}

bool CClientManager::MirrorMobTableIntoDB()
{
	for (auto it = m_vec_mobTable.begin(); it != m_vec_mobTable.end(); it++)
	{
		const TMobTable& t = *it;
		char query[4096];
		int len = snprintf(query, sizeof(query),
			"replace into mob_proto "
			"("
			"vnum, name, ");
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			len += snprintf(query + len, sizeof(query) - len, "locale_name_%s, ", strlower(astLocaleStringNames[i].c_str()));
		len += snprintf(query + len, sizeof(query) - len,
			"type, rank, battle_type, level, size, ai_flag, setRaceFlag, setImmuneFlag, "
			"on_click, empire, drop_item, resurrection_vnum, folder, "
			"st, dx, ht, iq, damage_min, damage_max, max_hp, regen_cycle, regen_percent, exp, "
			"gold_min, gold_max, def, attack_speed, move_speed, aggressive_hp_pct, aggressive_sight, attack_range, polymorph_item, "

			"enchant_curse, enchant_slow, enchant_poison, enchant_stun, enchant_critical, enchant_penetrate, enchant_ignore_block, "
			"resist_sword, resist_twohand, resist_dagger, resist_bell, resist_fan, resist_bow, "
			"resist_fire, resist_elect, resist_magic, resist_wind, resist_poison, resist_earth, "
			"resist_ice, resist_dark, dam_multiply, summon, drain_sp, "

			"skill_vnum0, skill_level0, skill_vnum1, skill_level1, skill_vnum2, skill_level2, "
			"skill_vnum3, skill_level3, skill_vnum4, skill_level4, "
			"sp_berserk, sp_stoneskin, sp_godspeed, sp_deathblow, sp_revive"
			") "
			"values ("

			"%d, \"%s\", ", t.vnum(), t.name().c_str());
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			len += snprintf(query + len, sizeof(query) - len, "\"%s\", ", t.locale_name(i).c_str());
		len += snprintf(query + len, sizeof(query) - len, "%d, %d, %d, %d, %d, %u, %u, %u, "
			"%d, %d, %d, %d, '%s', "
			"%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, %d, %d, %d, "

			"%d, %d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, "
			"%d, %d, %f, %d, %d, "

			"%d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, "
			"%d, %d, %d, %d, %d"
			")",
			t.type(), t.rank(), t.battle_type(), t.level(), t.size(), t.ai_flag(), t.race_flag(), t.immune_flag(),
			t.on_click_type(), t.empire(), t.drop_item_vnum(), t.resurrection_vnum(), t.folder().c_str(),
			t.str(), t.dex(), t.con(), t.int_(), t.damage_min(), t.damage_max(), t.max_hp(), t.regen_cycle(), t.regen_percent(), t.exp(),

			t.gold_min(), t.gold_max(), t.def(), t.attack_speed(), t.moving_speed(), t.aggressive_hp_pct(), t.aggressive_sight(), t.attack_range(), t.polymorph_item_vnum(),
			t.enchants(0), t.enchants(1), t.enchants(2), t.enchants(3), t.enchants(4), t.enchants(5), t.enchants(6),
			t.resists(0), t.resists(1), t.resists(2), t.resists(3), t.resists(4), t.resists(5),
			t.resists(7), t.resists(8), t.resists(9), t.resists(10), t.resists(11), t.resists(13),
			t.resists(14), t.resists(15), t.dam_multiply(), t.summon_vnum(), t.drain_sp(),
			t.skills(0).vnum(), t.skills(0).level(), t.skills(1).vnum(), t.skills(1).level(), t.skills(2).vnum(), t.skills(2).level(), t.skills(3).vnum(), t.skills(3).level(), t.skills(4).vnum(), t.skills(4).level(),
			t.berserk_point(), t.stone_skin_point(), t.god_speed_point(), t.death_blow_point(), t.revive_point()
			);

		CDBManager::instance().AsyncQuery(query);
	}
	return true;
}

bool CClientManager::MirrorItemTableIntoDB()
{
	for (auto it = m_vec_itemTable.begin(); it != m_vec_itemTable.end(); it++)
	{
		const TItemTable& t = *it;
		char query[QUERY_MAX_LEN];
		int len = snprintf(query, sizeof(query),
			"replace into item_proto ("
			"vnum, type, subtype, name, ");
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			len += snprintf(query + len, sizeof(query) - len, "locale_name_%s, ", strlower(astLocaleStringNames[i].c_str()));
		len += snprintf(query + len, sizeof(query) - len,
			"gold, shop_buy_price, weight, size, "
			"flag, wearflag, antiflag, immuneflag, "
			"refined_vnum, refine_set, magic_pct, socket_pct, addon_type, "
			"limittype0, limitvalue0, limittype1, limitvalue1, "
			"applytype0, applyvalue0, applytype1, applyvalue1, applytype2, applyvalue2, "
			"value0, value1, value2, value3, value4, value5 ) "
			"values ("
			"%d, %d, %d, \"%s\", ", t.vnum(), t.type(), t.sub_type(), t.name().c_str());
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			len += snprintf(query + len, sizeof(query) - len, "\"%s\", ", t.locale_name(i).c_str());
		len += snprintf(query + len, sizeof(query) - len,
			"%d, %d, %d, %d, "
			"%d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d, "
			"%d, %d, %d, %d, %d, %d )",
			t.gold(), t.shop_buy_price(), t.weight(), t.size(),
			t.flags(), t.wear_flags(), t.anti_flags(), t.immune_flags(),
			t.refined_vnum(), t.refine_set(), t.alter_to_magic_item_pct(), t.gain_socket_pct(), t.addon_type(),
			t.limits(0).type(),
			t.limits(0).value(),
			t.limits(1).type(),
			t.limits(1).value(),
			t.applies(0).type(),
			t.applies(0).value(),
			t.applies(1).type(),
			t.applies(1).value(),
			t.applies(2).type(),
			t.applies(2).value(),
			t.values(0),
			t.values(1),
			t.values(2),
			t.values(3),
			t.values(4),
			t.values(5));
		CDBManager::instance().AsyncQuery(query);
	}
	return true;
}

#ifdef ENABLE_XMAS_EVENT
void CClientManager::InitializeXmasRewards()
{
	std::auto_ptr<SQLMsg> pkMsg(CDBManager::instance().DirectQuery("SELECT day, vnum, count FROM xmas_rewards"));

	SQLResult *pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
	{
		sys_log(0, "InitializeXmasRewards: EMPTY TABLE.");
		return;
	}

	m_vec_xmasRewards.clear();

	MYSQL_ROW data = NULL;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		BYTE col = 0;
		TXmasRewards t;

		int n = 0;
		str_to_number(n, data[col++]);
		t.set_day(n);
		str_to_number(n, data[col++]);
		t.set_vnum(n);
		str_to_number(n, data[col++]);
		t.set_count(n);

		m_vec_xmasRewards.push_back(t);
	}
}
#endif
