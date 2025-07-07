#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "desc_client.h"
#include "db.h"
#include "log.h"
#include "skill.h"
#include "text_file_loader.h"
#include "priv_manager.h"
#include "questmanager.h"
#include "unique_item.h"
#include "safebox.h"
#include "blend_item.h"
#include "dev_log.h"
#include "item.h"
#include "item_manager.h"
#include "item_manager_private_types.h"
#include "group_text_parse_tree.h"
#include "mob_manager.h"

std::vector<CItemDropInfo> g_vec_pkCommonDropItem[MOB_RANK_MAX_NUM];

int ReadCommon_GetMobType(const char * szTypeName)
{
	if (!strcmp(szTypeName, "PAWN"))
		return MOB_RANK_PAWN;
	else if (!strcmp(szTypeName, "S_PAWN"))
		return MOB_RANK_S_PAWN;
	else if (!strcmp(szTypeName, "KNIGHT"))
		return MOB_RANK_KNIGHT;
	else if (!strcmp(szTypeName, "S_KNIGHT"))
		return MOB_RANK_S_KNIGHT;
	else if (!strcmp(szTypeName, "BOSS"))
		return MOB_RANK_BOSS;
	else if (!strcmp(szTypeName, "KING"))
		return MOB_RANK_KING;

	return -1;
}

int ReadCommon_GetJob(const char * szTypeName)
{
	if (!strcmp(szTypeName, "WARRIOR"))
		return JOB_WARRIOR + 1;
	else if (!strcmp(szTypeName, "ASSASSIN"))
		return JOB_ASSASSIN + 1;
	else if (!strcmp(szTypeName, "SURA"))
		return JOB_SURA + 1;
	else if (!strcmp(szTypeName, "SHAMAN"))
		return JOB_SHAMAN + 1;

	return 0;
}

bool ITEM_MANAGER::ReadCommonDropItemFile(const char * c_pszFileName)
{
	FILE * fp = fopen(c_pszFileName, "r");

	if (!fp)
	{
		sys_err("Cannot open %s", c_pszFileName);
		return false;
	}

	char buf[1024];

	int lines = 0;

	while (fgets(buf, 1024, fp))
	{
		++lines;

		if (!*buf || *buf == '\n' || *buf == '#' || (strlen(buf) > 1 && *buf == '/' && *(buf + 1) == '/')) // # or // : ignore line
			continue;

		TDropItem d;
		char szTemp[64];

		memset(&d, 0, sizeof(d));

		char * p = buf;
		char * p2;

		int type;

		bool bBreak = false;

		for (int j = 0; j < 8 && !bBreak; ++j)
		{
			p2 = strchr(p, '\t');

			if (!p2)
			{
				p2 = p + strlen(p);
				bBreak = true;

				if (p2 == p)
					break;
			}

			strlcpy(szTemp, p, MIN(sizeof(szTemp), (p2 - p) + 1));
			p = p2 + 1;

			switch (j)
			{
				case 0: type = ReadCommon_GetMobType(szTemp);	break;
				case 1: str_to_number(d.iLvStart, szTemp);	break;
				case 2: str_to_number(d.iLvEnd, szTemp);	break;
				case 3: d.fPercent = atof(szTemp);	break;
				case 4: strlcpy(d.szItemName, szTemp, sizeof(d.szItemName));	break;
				case 5: str_to_number(d.iCount, szTemp);	break;
				case 6: d.iJob = ReadCommon_GetJob(szTemp);	break;
				case 7: str_to_number(d.bAutoPickup, szTemp);	break;
			}	
		}
		
		DWORD dwPct = (DWORD) (d.fPercent * 10000.0f + 0.5f);
		DWORD dwVnumRange[2];

		std::string stItemName = d.szItemName;
		if (GetVnumRangeByString(stItemName, dwVnumRange[0], dwVnumRange[1]))
		{
			for (DWORD dwItemVnum = dwVnumRange[0]; dwItemVnum <= dwVnumRange[1]; ++dwItemVnum)
			{
				if (!GetTable(dwItemVnum))
				{
					sys_log(0, "item not exists in range %d~%d : %d", dwVnumRange[0], dwVnumRange[1], dwItemVnum);
					continue;
				}
			}
		}

		else if (str_is_number(d.szItemName) || !GetVnumByOriginalName(d.szItemName, *dwVnumRange))
		{
			// ÀÌ¸§À¸·Î ¸øÃ£À¸¸é ¹øÈ£·Î °Ë»ö
			str_to_number(*dwVnumRange, d.szItemName);
			if (!GetTable(*dwVnumRange))
			{
				sys_err("No such an item (name: %s)", d.szItemName);
				fclose(fp);
				return false;
			}

			dwVnumRange[1] = *dwVnumRange;
		}

		if (d.iLvStart == 0)
			continue;

		if (type == -1)
		{
			for (type = MOB_RANK_PAWN; type <= MOB_RANK_S_KNIGHT; ++type)
			{
				g_vec_pkCommonDropItem[type].push_back(CItemDropInfo(d.iLvStart, d.iLvEnd, dwPct, dwVnumRange[0], dwVnumRange[1], d.iCount, d.iJob, d.bAutoPickup));
			}
		}
		else
			g_vec_pkCommonDropItem[type].push_back(CItemDropInfo(d.iLvStart, d.iLvEnd, dwPct, dwVnumRange[0], dwVnumRange[1], d.iCount, d.iJob, d.bAutoPickup));
	}

	fclose(fp);

	for (int i = 0; i < MOB_RANK_MAX_NUM; ++i)
	{
		std::vector<CItemDropInfo> & v = g_vec_pkCommonDropItem[i];
		std::sort(v.begin(), v.end());

		std::vector<CItemDropInfo>::iterator it = v.begin();

		sys_log(0, "CommonItemDrop rank %d", i);

		while (it != v.end())
		{
			const CItemDropInfo & c = *(it++);
			sys_log(1, "CommonItemDrop %d %d %d %u~%u", c.m_iLevelStart, c.m_iLevelEnd, c.m_iPercent, c.m_dwVnumStart, c.m_dwVnumEnd);
		}
	}

	return true;
}

bool ITEM_MANAGER::ReadSpecialDropItemFile(const char * c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("ReadSpecialDropItemFile : Syntax error %s : no vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		sys_log(0,"DROP_ITEM_GROUP %s %d", stName.c_str(), iVnum);

		TTokenVector * pTok;

		//
		std::string stType;
		int type = CSpecialItemGroup::NORMAL;
		if (loader.GetTokenString("type", &stType))
		{
			stl_lowers(stType);
			if (stType == "pct")
			{
				type = CSpecialItemGroup::PCT;
			}
			else if (stType == "quest")
			{
				type = CSpecialItemGroup::QUEST;
				quest::CQuestManager::instance().RegisterNPCVnum(iVnum);
			}
			else if (stType == "special")
			{
				type = CSpecialItemGroup::SPECIAL;
			}
		}

		if ("attr" == stType)
		{
			CSpecialAttrGroup * pkGroup = M2_NEW CSpecialAttrGroup(iVnum);
			for (int k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					DWORD apply_type = 0;
					int	apply_value = 0;
					str_to_number(apply_type, pTok->at(0).c_str());
					if (0 == apply_type)
					{
						apply_type = FN_get_apply_type(pTok->at(0).c_str());
						if (0 == apply_type)
						{
							sys_err ("Invalid APPLY_TYPE %s in Special Item Group Vnum %d", pTok->at(0).c_str(), iVnum);
							return false;
						}
					}
					str_to_number(apply_value, pTok->at(1).c_str());
					if (apply_type > MAX_APPLY_NUM)
					{
						sys_err ("Invalid APPLY_TYPE %u in Special Item Group Vnum %d", apply_type, iVnum);
						M2_DELETE(pkGroup);
						return false;
					}
					pkGroup->m_vecAttrs.push_back(CSpecialAttrGroup::CSpecialAttrInfo(apply_type, apply_value));
				}
				else
				{
					break;
				}
			}
			if (loader.GetTokenVector("effect", &pTok))
			{
				pkGroup->m_stEffectFileName = pTok->at(0);
			}
			loader.SetParentNode();
			m_map_pkSpecialAttrGroup.insert(std::make_pair(iVnum, pkGroup));
		}
		else
		{
			CSpecialItemGroup * pkGroup = M2_NEW CSpecialItemGroup(iVnum, type);
			for (int k = 1; k < 256; ++k)
			{
				char buf[4];
				snprintf(buf, sizeof(buf), "%d", k);

				if (loader.GetTokenVector(buf, &pTok))
				{
					const std::string& name = pTok->at(0);
					DWORD dwVnumRange[2] = { 0, 0 };

					if (name == "gold")
					{
						*dwVnumRange = CSpecialItemGroup::GOLD;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "°æÇèÄ¡" || name == "exp")
					{
						*dwVnumRange = CSpecialItemGroup::EXP;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "mob")
					{
						*dwVnumRange = CSpecialItemGroup::MOB;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "slow")
					{
						*dwVnumRange = CSpecialItemGroup::SLOW;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "drain_hp")
					{
						*dwVnumRange = CSpecialItemGroup::DRAIN_HP;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "poison")
					{
						*dwVnumRange = CSpecialItemGroup::POISON;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "group")
					{
						*dwVnumRange = CSpecialItemGroup::MOB_GROUP;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (name == "poly_marble")
					{
						*dwVnumRange = CSpecialItemGroup::POLY_MARBLE;
						dwVnumRange[1] = *dwVnumRange;
					}
					else if (!GetVnumByOriginalName(name.c_str(), *dwVnumRange) && !GetVnumRangeByString(name, dwVnumRange[0], dwVnumRange[1]))
					{
						str_to_number(*dwVnumRange, name.c_str());
						if (!GetTable(*dwVnumRange))
						{
							sys_err("ReadSpecialDropItemFile : there is no item %s : node %s", name.c_str(), stName.c_str());
							M2_DELETE(pkGroup);

							return false;
						}
						dwVnumRange[1] = *dwVnumRange;
					}

					int iCount = 0;
					str_to_number(iCount, pTok->at(1).c_str());
					int iProb = 0;
					str_to_number(iProb, pTok->at(2).c_str());

					int iRarePct = 0;
					if (pTok->size() > 3)
					{
						str_to_number(iRarePct, pTok->at(3).c_str());
					}

					if (dwVnumRange[0] == dwVnumRange[1] && (dwVnumRange[0] <= CSpecialItemGroup::NONE || dwVnumRange[0] > CSpecialItemGroup::TYPE_MAX_NUM) && !GetTable(dwVnumRange[0]))
					{
						sys_err("cannot get item vnum %u for special_item %u", dwVnumRange[0], iVnum);
						continue;
					}

					sys_log(0, "        name %s vnum %d~%d count %d prob %d rare %d", name.c_str(), dwVnumRange[0], dwVnumRange[1], iCount, iProb, iRarePct);
					pkGroup->AddItem(dwVnumRange[0], dwVnumRange[1], iCount, iProb, iRarePct);
					if (dwVnumRange[0] > CSpecialItemGroup::TYPE_MAX_NUM)
						for (DWORD currVnum = dwVnumRange[0]; currVnum <= dwVnumRange[1]; ++currVnum)
						{
							auto pTableTemp = GetTable(currVnum);
							DWORD addVnum = currVnum;
							if (pTableTemp && (pTableTemp->type() == ITEM_WEAPON || pTableTemp->type() == ITEM_ARMOR) && addVnum % 10 != 0)
								addVnum = (addVnum / 10) * 10;

							network::TWikiItemOriginInfo origin_info;
							origin_info.set_vnum(iVnum);
							origin_info.set_is_mob(false);
							m_itemOriginMap[addVnum].push_back(origin_info);
						}

					// CHECK_UNIQUE_GROUP
					if (iVnum < 30000)
					{
						for (DWORD cur_vnum = dwVnumRange[0]; cur_vnum <= dwVnumRange[1]; ++cur_vnum)
							m_ItemToSpecialGroup[cur_vnum] = iVnum;
					}
					// END_OF_CHECK_UNIQUE_GROUP

					continue;
				}

				break;
			}
			loader.SetParentNode();
			if (CSpecialItemGroup::QUEST == type)
			{
				m_map_pkQuestItemGroup.insert(std::make_pair(iVnum, pkGroup));
			}
			else
			{
				m_map_pkSpecialItemGroup.insert(std::make_pair(iVnum, pkGroup));
			}
		}
	}

	return true;
}


bool ITEM_MANAGER::ConvSpecialDropItemFile()
{
	char szSpecialItemGroupFileName[256];
	snprintf(szSpecialItemGroupFileName, sizeof(szSpecialItemGroupFileName),
		"%s/special_item_group.txt", Locale_GetBasePath().c_str());

	FILE *fp = fopen("special_item_group_vnum.txt", "w");
	if (!fp)
	{
		sys_err("could not open file (%s)", "special_item_group_vnum.txt");
		return false;
	}

	CTextFileLoader loader;

	if (!loader.Load(szSpecialItemGroupFileName))
	{
		fclose(fp);
		return false;
	}

	std::string stName;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("ConvSpecialDropItemFile : Syntax error %s : no vnum, node %s", szSpecialItemGroupFileName, stName.c_str());
			loader.SetParentNode();
			fclose(fp);
			return false;
		}

		std::string str;
		int type = 0;
		if (loader.GetTokenString("type", &str))
		{
			stl_lowers(str);
			if (str == "pct")
			{
				type = 1;
			}
		}

		TTokenVector * pTok;

		fprintf(fp, "Group	%s\n", stName.c_str());
		fprintf(fp, "{\n");
		fprintf(fp, "	Vnum	%i\n", iVnum);
		if (type)
			fprintf(fp, "	Type	Pct");

		for (int k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				const std::string& name = pTok->at(0);
				DWORD dwVnum = 0;

				if (!GetVnumByOriginalName(name.c_str(), dwVnum))
				{
					if (	name == "°æÇèÄ¡" ||
						name == "mob" ||
						name == "slow" ||
						name == "drain_hp" ||
						name == "poison" ||
						name == "group")
					{
						dwVnum = 0;
					}
					else
					{
						str_to_number(dwVnum, name.c_str());
						if (!GetTable(dwVnum))
						{
							sys_err("ReadSpecialDropItemFile : there is no item %s : node %s", name.c_str(), stName.c_str());
							fclose(fp);

							return false;
						}
					}
				}

				int iCount = 0;
				str_to_number(iCount, pTok->at(1).c_str());
				int iProb = 0;
				str_to_number(iProb, pTok->at(2).c_str());

				int iRarePct = 0;
				if (pTok->size() > 3)
				{
					str_to_number(iRarePct, pTok->at(3).c_str());
				}

				//    1   "±â¼ú ¼ö·Ã¼­"   1   100
				if (0 == dwVnum)
					fprintf(fp, "	%d	%s	%d	%d\n", k, name.c_str(), iCount, iProb);
				else
					fprintf(fp, "	%d	%u	%d	%d\n", k, dwVnum, iCount, iProb);

				continue;
			}

			break;
		}
		fprintf(fp, "}\n");
		fprintf(fp, "\n");

		loader.SetParentNode();
	}

	fclose(fp);
	return true;
}

bool ITEM_MANAGER::ReadEtcDropItemFile(const char * c_pszFileName)
{
	FILE * fp = fopen(c_pszFileName, "r");

	if (!fp)
	{
		sys_err("Cannot open %s", c_pszFileName);
		return false;
	}

	char buf[512];

	int lines = 0;

	while (fgets(buf, 512, fp))
	{
		++lines;

		if (!*buf || *buf == '\n')
			continue;

		char szItemName[256];
		float fProb = 0.0f;

		strlcpy(szItemName, buf, sizeof(szItemName));
		char * cpTab = strrchr(szItemName, '\t');

		if (!cpTab)
			continue;

		*cpTab = '\0';
		fProb = atof(cpTab + 1);

		if (!*szItemName || fProb == 0.0f)
			continue;

		DWORD dwItemVnum;

		if (!GetVnumByOriginalName(szItemName, dwItemVnum))
		{
			sys_err("No such an item (name: %s)", szItemName);
			fclose(fp);
			return false;
		}

		m_map_dwEtcItemDropProb[dwItemVnum] = (DWORD) (fProb * 10000.0f);
		sys_log(0, "ETC_DROP_ITEM: %s prob %f", szItemName, fProb);
	}

	fclose(fp);
	return true;
}

bool ITEM_MANAGER::ReadMonsterDropItemGroup(const char * c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		std::string stName("");

		loader.GetCurrentNodeName(&stName);

		if (strncmp(stName.c_str(), "kr_", 3) == 0)
			continue;

		loader.SetChildNode(i);

		std::vector<int> vec_mobVnums;
		int iKillDrop = 0;
		int iLevelLimit = 0;

		std::string strType("");

		if (!loader.GetTokenString("type", &strType))
		{
			sys_err("ReadMonsterDropItemGroup : Syntax error %s : no type (kill|drop), node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		std::string strVnum("");
		if (!loader.GetTokenString("mob", &strVnum))
		{
			sys_err("ReadMonsterDropItemGroup : Syntax error %s : no mob vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}
		
		{
			int iPos;
			if ((iPos = strVnum.find("~")) > 0)
			{
				std::string vnum_start, vnum_end;
				vnum_start.assign(strVnum, 0, iPos);
				vnum_end.assign(strVnum, iPos + 1, strVnum.length());

				if (!str_is_number(vnum_start.c_str()) || !str_is_number(vnum_end.c_str()))
				{
					sys_err("ReadMonsterDropItemGroup : start or end vnum is not a number %s : %s", strVnum.c_str(), stName.c_str());
					return false;
				}

				int iRange[2];

				str_to_number(iRange[0], vnum_start.c_str());
				str_to_number(iRange[1], vnum_end.c_str());
				if (iRange[0] > iRange[1])
				{
					int iFirstVnum = iRange[0];
					iRange[0] = iRange[1];
					iRange[1] = iFirstVnum;
				}

				vec_mobVnums.resize(iRange[1] - iRange[0] + 1);
				for (int i = iRange[0]; i <= iRange[1]; ++i)
					vec_mobVnums[i - iRange[0]] = i;
			}
			else if ((iPos = strVnum.find(',')) > 0)
			{
				std::string strTmpVnum = strVnum;

				int i = 0;
				vec_mobVnums.resize(std::count(strTmpVnum.begin(), strTmpVnum.end(), ',') + 1);
				while ((iPos = strTmpVnum.find(',')) > 0)
				{
					std::string strCurrentVnum;
					strCurrentVnum.assign(strTmpVnum, 0, iPos);
					if (!str_is_number(strCurrentVnum.c_str()))
					{
						sys_err("ReadMonsterDropItemGroup : vnum in list is not a number %s[%s] : %s", strCurrentVnum.c_str(), strVnum.c_str(), stName.c_str());
						return false;
					}
					strTmpVnum.assign(strTmpVnum, iPos + 1, strTmpVnum.length());

					str_to_number(vec_mobVnums[i++], strCurrentVnum.c_str());
				}
				str_to_number(vec_mobVnums[i++], strTmpVnum.c_str());
			}
			else
			{
				vec_mobVnums.resize(1);
				str_to_number(vec_mobVnums[0], strVnum.c_str());
			}
		}

		if (strType == "kill")
		{
			if (!loader.GetTokenInteger("kill_drop", &iKillDrop))
			{
				sys_err("ReadMonsterDropItemGroup : Syntax error %s : no kill drop count, node %s", c_pszFileName, stName.c_str());
				loader.SetParentNode();
				return false;
			}
		}
		else
		{
			iKillDrop = 1;
		}

		if (strType == "limit")
		{
			if (!loader.GetTokenInteger("level_limit", &iLevelLimit))
			{
				sys_err("ReadmonsterDropItemGroup : Syntax error %s : no level_limit, node %s", c_pszFileName, stName.c_str());
				loader.SetParentNode();
				return false;
			}
		}
		else
		{
			iLevelLimit = 0;
		}

		if (iKillDrop == 0)
		{
			loader.SetParentNode();
			continue;
		}

		for (int i = 0; i < vec_mobVnums.size(); ++i)
		{
			int iMobVnum = vec_mobVnums[i];

			if (!CMobManager::instance().Get(iMobVnum))
			{
				if (test_server || strVnum.find('~') <= 0)
					sys_log(0, "ReadMonsterDropItemGroup: unkown mob vnum %d [%s, name %s] [ignore]", iMobVnum, strVnum.c_str(), stName.c_str());
				continue;
			}

			sys_log(0, "MOB_ITEM_GROUP %s [%s] %d %d", stName.c_str(), strType.c_str(), iMobVnum, iKillDrop);

			TTokenVector* pTok = NULL;

			if (strType == "kill")
			{
				std::vector<CMobItemGroup*>* pkVec;
				itertype(m_map_pkMobItemGroup) it = m_map_pkMobItemGroup.find(iMobVnum);
				if (it != m_map_pkMobItemGroup.end())
				{
					pkVec = &it->second;
				}
				else
				{
					m_map_pkMobItemGroup.insert(std::map<DWORD, std::vector<CMobItemGroup*> >::value_type(iMobVnum, std::vector<CMobItemGroup*>()));
					pkVec = &m_map_pkMobItemGroup.find(iMobVnum)->second;
				}

				CMobItemGroup * pkGroup = M2_NEW CMobItemGroup(iMobVnum, iKillDrop, stName);

				for (int k = 1; k < 256; ++k)
				{
					char buf[4];
					snprintf(buf, sizeof(buf), "%d", k);

					if (loader.GetTokenVector(buf, &pTok))
					{
						//sys_log(1, "               %s %s", pTok->at(0).c_str(), pTok->at(1).c_str());
						std::string& name = pTok->at(0);
						DWORD dwVnumRange[2];

						if (GetVnumRangeByString(name, dwVnumRange[0], dwVnumRange[1]))
						{
							for (DWORD dwItemVnum = dwVnumRange[0]; dwItemVnum <= dwVnumRange[1]; ++dwItemVnum)
							{
								if (!GetTable(dwItemVnum))
								{
									sys_err("ReadMonsterDropItemGroup : item not exists in range %d~%d : %d : node %s", dwVnumRange[0], dwVnumRange[1], dwItemVnum, stName.c_str());
									return false;
								}
							}
						}

						else if (!GetVnumByOriginalName(name.c_str(), *dwVnumRange))
						{
							str_to_number(*dwVnumRange, name.c_str());
							if (!GetTable(*dwVnumRange))
							{
								sys_err("ReadMonsterDropItemGroup : there is no item %s : node %s : vnum %d", name.c_str(), stName.c_str(), *dwVnumRange);
								return false;
							}
							dwVnumRange[1] = *dwVnumRange;
						}

						int iCount = 0;
						str_to_number(iCount, pTok->at(1).c_str());

						if (iCount < 1)
						{
							sys_err("ReadMonsterDropItemGroup : there is no count for item %s : node %s : vnum %d(~%d), count %d", name.c_str(), stName.c_str(), dwVnumRange[0], dwVnumRange[1], iCount);
							return false;
						}

						int iPartPct = 0;
						str_to_number(iPartPct, pTok->at(2).c_str());

						if (iPartPct == 0)
						{
							sys_err("ReadMonsterDropItemGroup : there is no drop percent for item %s : node %s : vnum %d, count %d, pct %d", name.c_str(), stName.c_str(), iPartPct);
							return false;
						}

						int iRarePct = 0;
						str_to_number(iRarePct, pTok->at(3).c_str());
						iRarePct = MINMAX(0, iRarePct, 100);

						sys_log(0, "        %s count %d rare %d", name.c_str(), iCount, iRarePct);
						pkGroup->AddItem(dwVnumRange[0], dwVnumRange[1], iCount, iPartPct, iRarePct);

						network::TWikiInfoTable *tbl;
						for (DWORD addVnum = dwVnumRange[0]; addVnum <= dwVnumRange[1]; ++addVnum)
						{
							if ((tbl = GetItemWikiInfo(addVnum)) && !tbl->origin_vnum())
								tbl->set_origin_vnum(iMobVnum);

							auto pTableTemp = GetTable(addVnum);
							DWORD currVnum = addVnum;
							if (pTableTemp && (pTableTemp->type() == ITEM_WEAPON || pTableTemp->type() == ITEM_ARMOR) && currVnum % 10 != 0)
								currVnum = (currVnum / 10) * 10;

							network::TWikiItemOriginInfo origin_info;
							origin_info.set_vnum(iMobVnum);
							origin_info.set_is_mob(true);
							m_itemOriginMap[currVnum].push_back(origin_info);
							CMobManager::instance().GetMobWikiInfo(iMobVnum).push_back(addVnum);
						}

						continue;
					}

					break;
				}

				pkVec->push_back(pkGroup);
			}
			else if (strType == "drop")
			{
				CDropItemGroup* pkGroup;
				bool bNew = true;
				itertype(m_map_pkDropItemGroup) it = m_map_pkDropItemGroup.find(iMobVnum);
				if (it == m_map_pkDropItemGroup.end())
				{
					pkGroup = M2_NEW CDropItemGroup(0, iMobVnum, stName);
				}
				else
				{
					bNew = false;
					pkGroup = it->second;
				}

				for (int k = 1; k < 256; ++k)
				{
					char buf[4];
					snprintf(buf, sizeof(buf), "%d", k);

					if (loader.GetTokenVector(buf, &pTok))
					{
						std::string& name = pTok->at(0);
						DWORD dwVnumRange[2];

						if (GetVnumRangeByString(name, dwVnumRange[0], dwVnumRange[1]))
						{
							for (DWORD dwItemVnum = dwVnumRange[0]; dwItemVnum <= dwVnumRange[1]; ++dwItemVnum)
							{
								if (!GetTable(dwItemVnum))
								{
									sys_err("ReadMonsterDropItemGroup : item not exists in range %d~%d : %d : node %s", dwVnumRange[0], dwVnumRange[1], dwItemVnum, stName.c_str());
									return false;
								}
							}
						}
						else
						{
							if (!GetVnumByOriginalName(name.c_str(), *dwVnumRange))
							{
								str_to_number(*dwVnumRange, name.c_str());
								if (!GetTable(*dwVnumRange))
								{
									sys_err("ReadDropItemGroup : there is no item %s : node %s", name.c_str(), stName.c_str());
									M2_DELETE(pkGroup);

									return false;
								}
								dwVnumRange[1] = *dwVnumRange;
							}
						}

						int iCount = 0;
						str_to_number(iCount, pTok->at(1).c_str());

						if (iCount < 1)
						{
							sys_err("ReadMonsterDropItemGroup : there is no count for item %s : node %s", name.c_str(), stName.c_str());
							M2_DELETE(pkGroup);

							return false;
						}

						float fPercent = atof(pTok->at(2).c_str());

						DWORD dwPct = (DWORD)(10000.0f * fPercent);

						sys_log(0, "        %s pct %d count %d", name.c_str(), dwPct, iCount);
						pkGroup->AddItem(dwVnumRange[0], dwVnumRange[1], dwPct, iCount);

						network::TWikiInfoTable *tbl;
						for (DWORD addVnum = dwVnumRange[0]; addVnum <= dwVnumRange[1]; ++addVnum)
						{
							if ((tbl = GetItemWikiInfo(addVnum)) && !tbl->origin_vnum())
								tbl->set_origin_vnum(iMobVnum);
							auto pTableTemp = GetTable(addVnum);
							DWORD currVnum = addVnum;
							if (pTableTemp && (pTableTemp->type() == ITEM_WEAPON || pTableTemp->type() == ITEM_ARMOR) && currVnum % 10 != 0)
								currVnum = (currVnum / 10) * 10;

							network::TWikiItemOriginInfo origin_info;
							origin_info.set_vnum(iMobVnum);
							origin_info.set_is_mob(true);
							m_itemOriginMap[currVnum].push_back(origin_info);
							CMobManager::instance().GetMobWikiInfo(iMobVnum).push_back(addVnum);
						}
						continue;
					}

					break;
				}
				if (bNew)
					m_map_pkDropItemGroup.insert(std::map<DWORD, CDropItemGroup*>::value_type(iMobVnum, pkGroup));

			}
			else if (strType == "limit")
			{
				CLevelItemGroup* pkLevelItemGroup = M2_NEW CLevelItemGroup(iLevelLimit);

				for (int k = 1; k < 256; k++)
				{
					char buf[4];
					snprintf(buf, sizeof(buf), "%d", k);

					if (loader.GetTokenVector(buf, &pTok))
					{
						std::string& name = pTok->at(0);
						DWORD dwVnumRange[2];

						if (GetVnumRangeByString(name, dwVnumRange[0], dwVnumRange[1]))
						{
							for (DWORD dwItemVnum = dwVnumRange[0]; dwItemVnum <= dwVnumRange[1]; ++dwItemVnum)
							{
								if (!GetTable(dwItemVnum))
								{
									sys_err("ReadMonsterDropItemGroup : item not exists in range %d~%d : %d : node %s", dwVnumRange[0], dwVnumRange[1], dwItemVnum, stName.c_str());
									return false;
								}
							}
						}
						else
						{
							if (false == GetVnumByOriginalName(name.c_str(), *dwVnumRange))
							{
								str_to_number(*dwVnumRange, name.c_str());
								if (!GetTable(*dwVnumRange))
								{
									M2_DELETE(pkLevelItemGroup);
									return false;
								}
								dwVnumRange[1] = *dwVnumRange;
							}
						}

						int iCount = 0;
						str_to_number(iCount, pTok->at(1).c_str());

						if (iCount < 1)
						{
							M2_DELETE(pkLevelItemGroup);
							return false;
						}

						float fPct = atof(pTok->at(2).c_str());
						DWORD dwPct = (DWORD)(10000.0f * fPct);

						pkLevelItemGroup->AddItem(dwVnumRange[0], dwVnumRange[1], dwPct, iCount);
						network::TWikiInfoTable *tbl;
						for (DWORD addVnum = dwVnumRange[0]; addVnum <= dwVnumRange[1]; ++addVnum)
						{
							if ((tbl = GetItemWikiInfo(addVnum)) && !tbl->origin_vnum())
								tbl->set_origin_vnum(iMobVnum);
							auto pTableTemp = GetTable(addVnum);
							DWORD currVnum = addVnum;
							if (pTableTemp && (pTableTemp->type() == ITEM_WEAPON || pTableTemp->type() == ITEM_ARMOR) && currVnum % 10 != 0)
								currVnum = (currVnum / 10) * 10;

							network::TWikiItemOriginInfo origin_info;
							origin_info.set_vnum(iMobVnum);
							origin_info.set_is_mob(true);

							m_itemOriginMap[currVnum].push_back(origin_info);
							CMobManager::instance().GetMobWikiInfo(iMobVnum).push_back(addVnum);
						}
						continue;
					}

					break;
				}

				m_map_pkLevelItemGroup.insert(std::map<DWORD, CLevelItemGroup*>::value_type(iMobVnum, pkLevelItemGroup));
			}
			else if (strType == "thiefgloves")
			{
				CBuyerThiefGlovesItemGroup* pkGroup = M2_NEW CBuyerThiefGlovesItemGroup(0, iMobVnum, stName);

				for (int k = 1; k < 256; ++k)
				{
					char buf[4];
					snprintf(buf, sizeof(buf), "%d", k);

					if (loader.GetTokenVector(buf, &pTok))
					{
						std::string& name = pTok->at(0);
						DWORD dwVnumRange[2];

						if (GetVnumRangeByString(name, dwVnumRange[0], dwVnumRange[1]))
						{
							for (DWORD dwItemVnum = dwVnumRange[0]; dwItemVnum <= dwVnumRange[1]; ++dwItemVnum)
							{
								if (!GetTable(dwItemVnum))
								{
									sys_err("ReadMonsterDropItemGroup : item not exists in range %d~%d : %d : node %s", dwVnumRange[0], dwVnumRange[1], dwItemVnum, stName.c_str());
									return false;
								}
							}
						}
						else
						{
							if (!GetVnumByOriginalName(name.c_str(), *dwVnumRange))
							{
								str_to_number(*dwVnumRange, name.c_str());
								if (!GetTable(*dwVnumRange))
								{
									sys_err("ReadDropItemGroup : there is no item %s : node %s", name.c_str(), stName.c_str());
									M2_DELETE(pkGroup);

									return false;
								}
								dwVnumRange[1] = *dwVnumRange;
							}
						}

						int iCount = 0;
						str_to_number(iCount, pTok->at(1).c_str());

						if (iCount < 1)
						{
							sys_err("ReadMonsterDropItemGroup : there is no count for item %s : node %s", name.c_str(), stName.c_str());
							M2_DELETE(pkGroup);

							return false;
						}

						float fPercent = atof(pTok->at(2).c_str());

						DWORD dwPct = (DWORD)(10000.0f * fPercent);

						sys_log(0, "        name %s pct %d count %d", name.c_str(), dwPct, iCount);
						pkGroup->AddItem(dwVnumRange[0], dwVnumRange[1], dwPct, iCount);

						continue;
					}

					break;
				}

				m_map_pkGloveItemGroup.insert(std::map<DWORD, CBuyerThiefGlovesItemGroup*>::value_type(iMobVnum, pkGroup));
			}
			else
			{
				sys_err("ReadMonsterDropItemGroup : Syntax error %s : invalid type %s (kill|drop), node %s", c_pszFileName, strType.c_str(), stName.c_str());
				loader.SetParentNode();
				return false;
			}
		}

		loader.SetParentNode();
	}

	return true;
}

bool ITEM_MANAGER::ReadDropItemGroup(const char * c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;
		int iMobVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("ReadDropItemGroup : Syntax error %s : no vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		if (!loader.GetTokenInteger("mob", &iMobVnum))
		{
			sys_err("ReadDropItemGroup : Syntax error %s : no mob vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			return false;
		}

		sys_log(0,"DROP_ITEM_GROUP %s %d", stName.c_str(), iMobVnum);

		TTokenVector * pTok;

		itertype(m_map_pkDropItemGroup) it = m_map_pkDropItemGroup.find(iMobVnum);

		CDropItemGroup* pkGroup;

		if (it == m_map_pkDropItemGroup.end())
			pkGroup = M2_NEW CDropItemGroup(iVnum, iMobVnum, stName);
		else
			pkGroup = it->second;

		for (int k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				std::string& name = pTok->at(0);
				DWORD dwVnumRange[2];

				if (GetVnumRangeByString(name, dwVnumRange[0], dwVnumRange[1]))
				{
					for (DWORD dwItemVnum = dwVnumRange[0]; dwItemVnum <= dwVnumRange[1]; ++dwItemVnum)
					{
						if (!GetTable(dwItemVnum))
						{
							sys_err("ReadMonsterDropItemGroup : item not exists in range %d~%d : %d : node %s", dwVnumRange[0], dwVnumRange[1], dwItemVnum, stName.c_str());
							return false;
						}
					}
				}

				else if (!GetVnumByOriginalName(name.c_str(), *dwVnumRange))
				{
					str_to_number(*dwVnumRange, name.c_str());
					if (!GetTable(*dwVnumRange))
					{
						sys_err("ReadDropItemGroup : there is no item %s : node %s", name.c_str(), stName.c_str());

						if (it == m_map_pkDropItemGroup.end())
							M2_DELETE(pkGroup);

						return false;
					}
					dwVnumRange[1] = *dwVnumRange;
				}

				float fPercent = atof(pTok->at(1).c_str());

				DWORD dwPct = (DWORD)(10000.0f * fPercent);

				int iCount = 1;
				if (pTok->size() > 2)
					str_to_number(iCount, pTok->at(2).c_str());

				if (iCount < 1)
				{
					sys_err("ReadDropItemGroup : there is no count for item %s : node %s", name.c_str(), stName.c_str());

					if (it == m_map_pkDropItemGroup.end())
						M2_DELETE(pkGroup);

					return false;
				}

				sys_log(0,"        %s %d %d", name.c_str(), dwPct, iCount);
				pkGroup->AddItem(dwVnumRange[0], dwVnumRange[1], dwPct, iCount);
				continue;
			}

			break;
		}

		if (it == m_map_pkDropItemGroup.end())
			m_map_pkDropItemGroup.insert(std::map<DWORD, CDropItemGroup*>::value_type(iMobVnum, pkGroup));

		loader.SetParentNode();
	}

	return true;
}
