#include "stdafx.h"
#include "../../common/tables.h"
#include "config.h"
#include "item_manager.h"
#include "item_manager_private_types.h"
#include "mob_manager.h"
#include "refine.h"
#include "constants.h"
#include "cmd.h"
#include "utils.h"

#define PUBLIC_TABLE_LANG LANGUAGE_GERMAN

void PUBLIC_CreateRefineList(const std::string& stFileName, bool bWriteChance = true);
void PUBLIC_CreateDropList(const std::string& stFileName, bool bWriteChance = true);
void PUBLIC_CreateCommonDropList(const std::string& stFileName, bool bWriteChance = true);
void PUBLIC_CreateSpecialItemList(const std::string& stFileName, bool bWriteChance = true);
void PUBLIC_CreateMobDataList(const std::string& stFileName);
void PUBLIC_CreateGMCommandFile(const std::string& stFileName, int iGMLevel = -1);

std::string stDirName = "public/";

void PUBLIC_CreateLists()
{
	if (!g_bCreatePublicFiles)
	{
		sys_err("Only public creator server creating public lists.");
		return;
	}

	PUBLIC_CreateRefineList(stDirName + "refine_item.txt", false);
	PUBLIC_CreateRefineList(stDirName + "refine_item_chance.txt", true);
	PUBLIC_CreateDropList(stDirName + "mob_drop.txt", false);
	PUBLIC_CreateDropList(stDirName + "mob_drop_chance.txt", true);
	PUBLIC_CreateCommonDropList(stDirName + "common_drop.txt", false);
	PUBLIC_CreateCommonDropList(stDirName + "common_drop_chance.txt", true);
	PUBLIC_CreateSpecialItemList(stDirName + "special_item.txt", false);
	PUBLIC_CreateSpecialItemList(stDirName + "special_item_chance.txt", true);
	PUBLIC_CreateMobDataList(stDirName + "mob_data.txt");
	PUBLIC_CreateGMCommandFile(stDirName + "command.txt");
	PUBLIC_CreateGMCommandFile(stDirName + "command_player.txt", GM_PLAYER);
	PUBLIC_CreateGMCommandFile(stDirName + "command_trial_gamemaster.txt", GM_LOW_WIZARD);
	PUBLIC_CreateGMCommandFile(stDirName + "command_gamemaster.txt", GM_WIZARD);
	PUBLIC_CreateGMCommandFile(stDirName + "command_super_gamemaster.txt", GM_HIGH_WIZARD);
	PUBLIC_CreateGMCommandFile(stDirName + "command_community_manager.txt", GM_GOD);
	PUBLIC_CreateGMCommandFile(stDirName + "command_administrator.txt", GM_IMPLEMENTOR);
}

void __CreateRefineList_ExtractItemName(char* szItemName, const std::string& stLocaleItemName)
{
	int iPos = stLocaleItemName.find_last_of("+");

	if (iPos == std::string::npos)
	{
		sys_err("cannot extract item name [%s]", stLocaleItemName.c_str());
		snprintf(szItemName, stLocaleItemName.length() + 1, stLocaleItemName.c_str());
	}
	else
	{
		if (*(stLocaleItemName.c_str() + iPos) == ' ' && iPos > 0)
			iPos--;

		memcpy(szItemName, stLocaleItemName.c_str(), iPos);
		szItemName[iPos] = '\0';
	}
}

void __CreateRefineList_WriteRefineItem(char* szLineBuf, size_t lineSize, int& strSize, DWORD dwVnum, const network::TRefineTable* pRefineTab, int iItemIndex)
{
	auto pRefineItemTab = ITEM_MANAGER::instance().GetTable(pRefineTab->materials(iItemIndex).vnum());
	if (!pRefineItemTab)
	{
		sys_err("cannot get refine item %d by vnum %u", pRefineTab->materials(0).vnum(), dwVnum);
		strSize += snprintf(szLineBuf + strSize, lineSize - strSize, "%dx [unkown vnum: %u] benötigt",
			pRefineTab->materials(iItemIndex).count(), pRefineTab->materials(iItemIndex).vnum());
	}
	else
	{
		strSize += snprintf(szLineBuf + strSize, lineSize - strSize, "%dx %s benötigt",
			pRefineTab->materials(iItemIndex).count(), pRefineItemTab->locale_name(PUBLIC_TABLE_LANG).c_str());
	}
}

void __CreateRefineList_WriteRefineInfo(char* szLineBuf, size_t lineSize, int& strSize, FILE* pFile, DWORD dwVnum, const network::TRefineTable* pRefineTab)
{
	if (pRefineTab->material_count() == 0)
	{
		strSize += snprintf(szLineBuf + strSize, lineSize - strSize, "%d Yang benötigt", pRefineTab->cost());
	}
	else if (pRefineTab->cost() == 0 && pRefineTab->material_count() == 1)
	{
		__CreateRefineList_WriteRefineItem(szLineBuf, lineSize, strSize, dwVnum, pRefineTab, 0);
	}
	else
	{
		strSize += snprintf(szLineBuf + strSize, lineSize - strSize, "\n");
		fputs(szLineBuf, pFile);

		if (pRefineTab->cost() != 0)
		{
			strSize = snprintf(szLineBuf, lineSize, "\t\t%ld Yang benötigt\n", pRefineTab->cost());
			fputs(szLineBuf, pFile);
		}

		for (int iItemIndex = 0; iItemIndex < pRefineTab->material_count(); ++iItemIndex)
		{
			if (pRefineTab->material_count() > 1)
				strSize = snprintf(szLineBuf, lineSize, "\t\t%d. Material: ", iItemIndex + 1);
			else
				strSize = snprintf(szLineBuf, lineSize, "\t\tMaterial: ");
			__CreateRefineList_WriteRefineItem(szLineBuf, lineSize, strSize, dwVnum, pRefineTab, iItemIndex);
			strSize += snprintf(szLineBuf + strSize, lineSize - strSize, "\n");
			fputs(szLineBuf, pFile);
		}

		return;
	}

	strSize += snprintf(szLineBuf + strSize, lineSize - strSize, "\n");
	fputs(szLineBuf, pFile);
}

void __CreateRefineList_WriteNormalRefine(char* szLineBuf, size_t lineSize, FILE* pFile, DWORD dwMainVnum, int iStartRefineLevel, int iMaxRefineLevel, bool bWriteChance, int iRefineFactor = 1)
{
	for (int i = iStartRefineLevel; i < iMaxRefineLevel; ++i)
	{
		// check tab
		auto pCurItemTab = ITEM_MANAGER::instance().GetTable(dwMainVnum + (i - iStartRefineLevel) * iRefineFactor);
		if (!pCurItemTab)
		{
			sys_err("cannot get item tab by vnum %u [main vnum %u]", dwMainVnum + (i - iStartRefineLevel) * iRefineFactor, dwMainVnum);
			continue;
		}

		// check refine by item table
		if (pCurItemTab->refined_vnum() != pCurItemTab->vnum() + iRefineFactor)
		{
			sys_err("invalid refined vnum of item %u %s [expected %u got %u]",
				pCurItemTab->vnum(), pCurItemTab->locale_name(PUBLIC_TABLE_LANG).c_str(), pCurItemTab->vnum() + 1, pCurItemTab->refined_vnum());
			continue;
		}
		if (!pCurItemTab->refine_set())
		{
			sys_err("invalid refine set (0) for item %u %s", pCurItemTab->vnum(), pCurItemTab->locale_name(PUBLIC_TABLE_LANG).c_str());
			continue;
		}

		// check refine by refine table
		auto pRefineTab = CRefineManager::Instance().GetRefineRecipe(pCurItemTab->refine_set());
		if (!pRefineTab)
		{
			sys_log(0, "__CreateRefineList_WriteNormalRefine: invalid refine set for item %u %s [%u]",
				pCurItemTab->vnum(), pCurItemTab->locale_name(PUBLIC_TABLE_LANG).c_str(), pCurItemTab->refine_set());
			continue;
		}

		int iStrSize;
		if (bWriteChance)
			iStrSize = snprintf(szLineBuf, lineSize, "\tLv. %d -> %d [%d%%]: ", i, i + 1, pRefineTab->prob());
		else
			iStrSize = snprintf(szLineBuf, lineSize, "\tLv. %d -> %d: ", i, i + 1);
		__CreateRefineList_WriteRefineInfo(szLineBuf, lineSize, iStrSize, pFile, pCurItemTab->vnum(), pRefineTab);
	}
}

void __CreateRefineList_WriteDefault(char* szLineBuf, size_t lineSize, FILE* pFile, network::TItemTable& t, int iStartRefineLevel, int iMaxRefineLevel, bool bWriteChance, int iRefineFactor = 1)
{
	// get item name without +
	char szItemName[ITEM_NAME_MAX_LEN + 1];
	__CreateRefineList_ExtractItemName(szItemName, t.locale_name(PUBLIC_TABLE_LANG));

	// title
	snprintf(szLineBuf, lineSize, "%s [%d - %d]:\n", szItemName, t.vnum(), t.vnum() + (iMaxRefineLevel - iStartRefineLevel) * iRefineFactor);
	fputs(szLineBuf, pFile);

	// each refine level
	__CreateRefineList_WriteNormalRefine(szLineBuf, lineSize, pFile, t.vnum(), iStartRefineLevel, iMaxRefineLevel, bWriteChance, iRefineFactor);
}

void __CreateRefineList_WriteEquip(char* szLineBuf, size_t lineSize, FILE* pFile, network::TItemTable& t, bool bWriteChance)
{
	const int iStartRefineLevel = 0;
	const int iMaxRefineLevel = 9;

	// normal refine
	__CreateRefineList_WriteDefault(szLineBuf, lineSize, pFile, t, iStartRefineLevel, iMaxRefineLevel, bWriteChance);

	// refines for max refined items
	auto pMaxItemTab = ITEM_MANAGER::instance().GetTable(t.vnum() + iMaxRefineLevel);
	if (pMaxItemTab)
	{
		// refine to other item
		if (pMaxItemTab->refined_vnum() != 0 && pMaxItemTab->refined_vnum() != pMaxItemTab->vnum())
		{
			auto pNewItemTab = ITEM_MANAGER::instance().GetTable(pMaxItemTab->refined_vnum());
			if (!pNewItemTab)
			{
				sys_err("invalid refined vnum for item %u [refine vnum %u does not exist]", pMaxItemTab->vnum(), pMaxItemTab->refined_vnum());
			}
			else
			{
				auto pRefineTab = CRefineManager::Instance().GetRefineRecipe(pMaxItemTab->refine_set());
				if (!pRefineTab)
				{
					sys_err("invalid refine set for item %u %s [%u]",
						pMaxItemTab->vnum(), pMaxItemTab->locale_name(PUBLIC_TABLE_LANG).c_str(), pMaxItemTab->refine_set());
				}
				else
				{
					int iStrSize;
					if (bWriteChance)
						iStrSize = snprintf(szLineBuf, lineSize, "\tLv. %d -> %s [%d%%]: ",
							iMaxRefineLevel, pNewItemTab->locale_name(PUBLIC_TABLE_LANG).c_str(), pRefineTab->prob());
					else
						iStrSize = snprintf(szLineBuf, lineSize, "\tLv. %d -> %s: ", iMaxRefineLevel, pNewItemTab->locale_name(PUBLIC_TABLE_LANG).c_str());

					__CreateRefineList_WriteRefineInfo(szLineBuf, lineSize, iStrSize, pFile, pMaxItemTab->vnum(), pRefineTab);
				}
			}
		}
	}

	fputs("\n", pFile);
}

void __CreateRefineList_WriteStone(char* szLineBuf, size_t lineSize, FILE* pFile, network::TItemTable& t, bool bWriteChance)
{
	const int iStartRefineLevel = 3;
	const int iMaxRefineLevel = 5;

	// normal refine
	__CreateRefineList_WriteDefault(szLineBuf, lineSize, pFile, t, iStartRefineLevel, iMaxRefineLevel, bWriteChance, 100);

	fputs("\n", pFile);
}

void PUBLIC_CreateRefineList(const std::string& stFileName, bool bWriteChance)
{
	sys_log(0, "PUBLIC: Create Refine Item List \"%s\" (chance: %d)", stFileName.c_str(), bWriteChance);

	FILE* fp = fopen(stFileName.c_str(), "w");
	if (!fp)
	{
		sys_err("cannot open file to write \"%s\" !!", stFileName.c_str());
		return;
	}

	char szCurrentLine[512];
	const size_t currentLineSize = sizeof(szCurrentLine);

	auto& vecItemTable = ITEM_MANAGER::instance().GetVecProto();
	for (int i = 0; i < vecItemTable.size(); ++i)
	{
		auto& t = vecItemTable[i];

		if (!t.refine_set())
			continue;

		// equip
		if ((t.type() == ITEM_WEAPON && t.sub_type() != WEAPON_ARROW && t.sub_type() != WEAPON_QUIVER && t.sub_type() != WEAPON_MOUNT_SPEAR) ||
			t.type() == ITEM_ARMOR)
		{
			// it will write for +0 - +9 in one function call of WriteEquip
			if (t.vnum() % 10 != 0)
				continue;

			__CreateRefineList_WriteEquip(szCurrentLine, currentLineSize, fp, t, bWriteChance);
		}
		// stone
		else if (t.type() == ITEM_METIN)
		{
			// it will write for +3 - +5 in one function call of WriteStone (+0 - +3 does not exist!)
			if ((t.vnum() % 1000) / 100 != 3)
				continue;

			__CreateRefineList_WriteStone(szCurrentLine, currentLineSize, fp, t, bWriteChance);
		}
	}

	fclose(fp);
}

void __CreateDropList_WriteMob(char* szLineBuf, size_t lineSize, FILE* pFile, DWORD dwMobRace)
{
	const CMob* pkMob = CMobManager::Instance().Get(dwMobRace);
	if (!pkMob)
	{
		sys_err("cannot get mob by race num %u", dwMobRace);
		return;
	}

	snprintf(szLineBuf, lineSize, "%s [Lv. %u] (%u):\n", pkMob->m_table.locale_name(PUBLIC_TABLE_LANG).c_str(), pkMob->m_table.level(), dwMobRace);
	fputs(szLineBuf, pFile);
}

void __CreateDropList_WriteLevelGroup(char* szLineBuf, size_t lineSize, FILE* pFile, std::map<DWORD, CLevelItemGroup*>::const_iterator& level_it, bool bWriteChance = true, std::set<DWORD>* pkGroupSet = NULL)
{
	snprintf(szLineBuf, lineSize, "\t- ab Level %d droppt man folgende Items:\n", level_it->second->GetLevelLimit());
	fputs(szLineBuf, pFile);

	const std::vector<CLevelItemGroup::SLevelItemGroupInfo>& rkLevelVec = level_it->second->GetVector();
	for (int i = 0; i < rkLevelVec.size(); ++i)
	{
		if (rkLevelVec[i].dwVNumStart != rkLevelVec[i].dwVNumEnd)
		{
			if (bWriteChance)
				snprintf(szLineBuf, lineSize, "\t\t- einen der folgenden Gegenstände zu %.4f%%:\n", ((float)rkLevelVec[i].dwPct) / 10000.0f);
			else
				snprintf(szLineBuf, lineSize, "\t\t- einen der folgenden Gegenstände:\n");
			fputs(szLineBuf, pFile);
			for (DWORD dwVnum = rkLevelVec[i].dwVNumStart; dwVnum <= rkLevelVec[i].dwVNumEnd; ++dwVnum)
			{
				auto pProto = ITEM_MANAGER::Instance().GetTable(dwVnum);
				if (!pProto)
				{
					sys_err("cannot get proto by item %u mob %u type level", dwVnum, level_it->first);
					continue;
				}

				snprintf(szLineBuf, lineSize, "\t\t\t- %dx %s (%u)\n",
					rkLevelVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum);
				fputs(szLineBuf, pFile);
			}
		}
		else
		{
			const DWORD& dwVnum = rkLevelVec[i].dwVNumStart;
			auto pProto = ITEM_MANAGER::Instance().GetTable(dwVnum);
			if (!pProto)
			{
				sys_err("cannot get proto by item %u mob %u type level", dwVnum, level_it->first);
				continue;
			}

			if (bWriteChance)
			{
				snprintf(szLineBuf, lineSize, "\t\t- %dx %s (%u) [Chance %.4f%%]\n",
					rkLevelVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum, ((float)rkLevelVec[i].dwPct) / 10000.0f);
			}
			else
			{
				snprintf(szLineBuf, lineSize, "\t\t- %dx %s (%u)\n",
					rkLevelVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum);
			}
			fputs(szLineBuf, pFile);
		}
	}

	if (rkLevelVec.size() && pkGroupSet)
		pkGroupSet->insert(level_it->first);
}

void __CreateDropList_WriteDropGroup(char* szLineBuf, size_t lineSize, FILE* pFile, std::map<DWORD, CDropItemGroup*>::const_iterator& drop_it, bool bWriteChance = true, std::set<DWORD>* pkGroupSet = NULL)
{
	const std::vector<CDropItemGroup::SDropItemGroupInfo>& rkDropVec = drop_it->second->GetVector();
	for (int i = 0; i < rkDropVec.size(); ++i)
	{
		if (rkDropVec[i].dwVnumStart != rkDropVec[i].dwVnumEnd)
		{
			if (bWriteChance)
				snprintf(szLineBuf, lineSize, "\t- einen der folgenden Gegenstände zu %.4f%%:\n", ((float)rkDropVec[i].dwPct) / 10000.0f);
			else
				snprintf(szLineBuf, lineSize, "\t- einen der folgenden Gegenstände:\n");
			fputs(szLineBuf, pFile);
			for (DWORD dwVnum = rkDropVec[i].dwVnumStart; dwVnum <= rkDropVec[i].dwVnumEnd; ++dwVnum)
			{
				auto pProto = ITEM_MANAGER::Instance().GetTable(dwVnum);
				if (!pProto)
				{
					sys_err("cannot get proto by item %u mob %u type drop", dwVnum, drop_it->first);
					continue;
				}

				snprintf(szLineBuf, lineSize, "\t\t- %dx %s (%u)\n",
					rkDropVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum);
				fputs(szLineBuf, pFile);
			}
		}
		else
		{
			const DWORD& dwVnum = rkDropVec[i].dwVnumStart;
			auto pProto = ITEM_MANAGER::Instance().GetTable(dwVnum);
			if (!pProto)
			{
				sys_err("cannot get proto by item %u mob %u type drop", dwVnum, drop_it->first);
				continue;
			}

			if (bWriteChance)
			{
				snprintf(szLineBuf, lineSize, "\t- %dx %s (%u) [Chance %.4f%%]\n",
					rkDropVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum, ((float)rkDropVec[i].dwPct) / 10000.0f);
			}
			else
			{
				snprintf(szLineBuf, lineSize, "\t- %dx %s (%u)\n",
					rkDropVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum);
			}
			fputs(szLineBuf, pFile);
		}
	}

	if (rkDropVec.size() && pkGroupSet)
		pkGroupSet->insert(drop_it->first);
}

void __CreateDropList_WriteMobGroup(char* szLineBuf, size_t lineSize, FILE* pFile, std::map<DWORD, std::vector<CMobItemGroup*> >::const_iterator& mob_it, bool bWriteChance = true, std::set<DWORD>* pkGroupSet = NULL)
{
	for (int i = 0; i < mob_it->second.size(); ++i)
	{
		CMobItemGroup* pGroup = mob_it->second[i];

		if (bWriteChance)
			snprintf(szLineBuf, lineSize, "\t- bei jedem %d. Monster eins von folgenden Items:\n", pGroup->GetKillPerDrop());
		else
			snprintf(szLineBuf, lineSize, "\t- zufälliges Item zu bestimmter Chance von folgenden Items (nur eins gleichzeitig droppbar):\n");
		fputs(szLineBuf, pFile);

		const std::vector<int> rkMobProbVec = pGroup->GetProbVector();
		const std::vector<CMobItemGroup::SMobItemGroupInfo> rkMobItemVec = pGroup->GetItemVector();
		for (int i = 0; i < rkMobItemVec.size(); ++i)
		{
			for (DWORD dwVnum = rkMobItemVec[i].dwItemVnumStart; dwVnum <= rkMobItemVec[i].dwItemVnumEnd; ++dwVnum)
			{
				auto pProto = ITEM_MANAGER::Instance().GetTable(dwVnum);
				if (!pProto)
				{
					sys_err("cannot get proto by item %u mob %u type kill", dwVnum, mob_it->first);
					continue;
				}

				int iProb = rkMobProbVec[i];
				if (i > 0)
					iProb -= rkMobProbVec[i - 1];

				if (bWriteChance)
				{
					snprintf(szLineBuf, lineSize, "\t\t- %dx %s (%u) [Chance %d (im Vergleich zu anderen Items!)]\n",
						rkMobItemVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum, iProb);
				}
				else
				{
					snprintf(szLineBuf, lineSize, "\t\t- %dx %s (%u)\n",
						rkMobItemVec[i].iCount, pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum);
				}
				fputs(szLineBuf, pFile);
			}
		}
	}

	if (pkGroupSet)
		pkGroupSet->insert(mob_it->first);
}

void PUBLIC_CreateDropList(const std::string& stFileName, bool bWriteChance)
{
	sys_log(0, "PUBLIC: Create Drop Item List \"%s\" (chance: %d)", stFileName.c_str(), bWriteChance);

	FILE* fp = fopen(stFileName.c_str(), "w");
	if (!fp)
	{
		sys_err("cannot open file to write \"%s\" !!", stFileName.c_str());
		return;
	}

	char szCurrentLine[512];
	const size_t currentLineSize = sizeof(szCurrentLine);

	const std::map<DWORD, std::vector<CMobItemGroup*> >& rkMobMap = ITEM_MANAGER::Instance().GetMobItemGroupMap();
	std::set<DWORD> kMobSet;
	const std::map<DWORD, CDropItemGroup*>& rkDropMap = ITEM_MANAGER::Instance().GetDropItemGroupMap();
	const std::map<DWORD, CLevelItemGroup*>& rkLevelMap = ITEM_MANAGER::Instance().GetLevelItemGroupMap();
	std::set<DWORD> kLevelSet;

	for (itertype(rkDropMap) it = rkDropMap.begin(); it != rkDropMap.end(); ++it)
	{
		__CreateDropList_WriteMob(szCurrentLine, currentLineSize, fp, it->first);

		itertype(rkLevelMap) level_it = rkLevelMap.find(it->first);
		if (level_it != rkLevelMap.end())
			__CreateDropList_WriteLevelGroup(szCurrentLine, currentLineSize, fp, level_it, bWriteChance, &kMobSet);

		itertype(rkMobMap) mob_it = rkMobMap.find(it->first);
		if (mob_it != rkMobMap.end())
			__CreateDropList_WriteMobGroup(szCurrentLine, currentLineSize, fp, mob_it, bWriteChance, &kMobSet);

		__CreateDropList_WriteDropGroup(szCurrentLine, currentLineSize, fp, it, bWriteChance);

		fputs("\n", fp);
	}

	for (itertype(rkMobMap) it = rkMobMap.begin(); it != rkMobMap.end(); ++it)
	{
		if (kMobSet.find(it->first) != kMobSet.end())
			continue;

		__CreateDropList_WriteMob(szCurrentLine, currentLineSize, fp, it->first);

		itertype(rkLevelMap) level_it = rkLevelMap.find(it->first);
		if (level_it != rkLevelMap.end())
			__CreateDropList_WriteLevelGroup(szCurrentLine, currentLineSize, fp, level_it, bWriteChance, &kMobSet);

		__CreateDropList_WriteMobGroup(szCurrentLine, currentLineSize, fp, it, bWriteChance);

		fputs("\n", fp);
	}

	for (itertype(rkLevelMap) it = rkLevelMap.begin(); it != rkLevelMap.end(); ++it)
	{
		if (kLevelSet.find(it->first) != kLevelSet.end())
			continue;

		__CreateDropList_WriteMob(szCurrentLine, currentLineSize, fp, it->first);

		__CreateDropList_WriteLevelGroup(szCurrentLine, currentLineSize, fp, it, bWriteChance);

		fputs("\n", fp);
	}

	fclose(fp);
}

void __CreateCommonDropList_WriteItems(char* szLineBuf, size_t lineSize, FILE* pFile, const std::vector<unsigned char>* vec_ranks, const CItemDropInfo* info, bool bWriteChance)
{
	const char* szRankNames[MOB_RANK_MAX_NUM + 1] = {
		"PAWN",
		"S_PAWN",
		"KNIGHT",
		"S_KNIGHT",
		"BOSS",
		"KING",
		"KING_NO_METIN",
	};

	for (DWORD dwVnum = info->m_dwVnumStart; dwVnum <= info->m_dwVnumEnd; ++dwVnum)
	{
		auto pTable = ITEM_MANAGER::instance().GetTable(dwVnum);
		if (!pTable)
		{
			sys_err("cannot get table by vnum %u", dwVnum);
			continue;
		}

		int iWroteSize = snprintf(szLineBuf, lineSize, "\t- %ux %s (%u)",
			info->m_bCount, pTable->locale_name(PUBLIC_TABLE_LANG).c_str(), dwVnum);
		if (info->m_iLevelEnd > 0)
			iWroteSize += snprintf(szLineBuf + iWroteSize, lineSize - iWroteSize, " von Level %u - %u",
			info->m_iLevelStart, info->m_iLevelEnd);
		else
			iWroteSize += snprintf(szLineBuf + iWroteSize, lineSize - iWroteSize, " ab Level %u",
			info->m_iLevelStart);
		iWroteSize += snprintf(szLineBuf + iWroteSize, lineSize - iWroteSize, " bei Rang %u [%s]",
			(*vec_ranks)[0], szRankNames[(*vec_ranks)[0]]);
		for (int i = 1; i < vec_ranks->size(); ++i)
			iWroteSize += snprintf(szLineBuf + iWroteSize, lineSize - iWroteSize, ", %u [%s]", (*vec_ranks)[i], szRankNames[(*vec_ranks)[i]]);
		if (bWriteChance)
			iWroteSize += snprintf(szLineBuf + iWroteSize, lineSize - iWroteSize, " [Chance %.4f%%]", ((float)info->m_iPercent) / 10000.0f);
		fputs(szLineBuf, pFile);

		fputs("\n", pFile);
	}
}

typedef struct SDropIndex {
	DWORD	dwVnumStart;
	DWORD	dwVnumEnd;
	unsigned char	bStartLevel;
	unsigned char	bEndLevel;

	SDropIndex(DWORD dwVnumStart, DWORD dwVnumEnd, unsigned char bStartLevel, unsigned char bEndLevel) :
		dwVnumStart(dwVnumStart), dwVnumEnd(dwVnumEnd), bStartLevel(bStartLevel), bEndLevel(bEndLevel)
	{
	}

	bool operator==(const SDropIndex& other) const
	{
		return other.dwVnumStart == dwVnumStart && other.dwVnumEnd == dwVnumEnd &&
			other.bStartLevel == bStartLevel && other.bEndLevel == bEndLevel;
	}
	bool operator<(const SDropIndex& other) const
	{
		if (other.dwVnumStart != dwVnumStart)
			return dwVnumStart < other.dwVnumStart;
		else
			return bStartLevel < other.bStartLevel;
	}
} TDropIndex;

void PUBLIC_CreateCommonDropList(const std::string& stFileName, bool bWriteChance)
{
	sys_log(0, "PUBLIC: Create Common Drop Item List \"%s\" (chance: %d)", stFileName.c_str(), bWriteChance);

	FILE* fp = fopen(stFileName.c_str(), "w");
	if (!fp)
	{
		sys_err("cannot open file to write \"%s\" !!", stFileName.c_str());
		return;
	}

	char szCurrentLine[512];
	const size_t currentLineSize = sizeof(szCurrentLine);

	snprintf(szCurrentLine, currentLineSize, "Alternative Item-Dropps (Monsterunabhängig):");
	fputs(szCurrentLine, fp);
	fputs("\n", fp);

	std::map<TDropIndex, std::pair<std::vector<unsigned char>, const CItemDropInfo*> > map_commonDrop;
	for (int rank = 0; rank < MOB_RANK_MAX_NUM; ++rank)
	{
		for (int i = 0; i < g_vec_pkCommonDropItem[rank].size(); ++i)
		{
			const CItemDropInfo* info = &g_vec_pkCommonDropItem[rank][i];
			TDropIndex index(info->m_dwVnumStart, info->m_dwVnumEnd, info->m_iLevelStart, info->m_iLevelEnd);
			itertype(map_commonDrop) it = map_commonDrop.find(index);
			if (it != map_commonDrop.end())
				it->second.first.push_back(rank);
			else
			{
				std::pair<std::vector<unsigned char>, const CItemDropInfo*> data;
				data.first.push_back(rank);
				data.second = info;
				map_commonDrop.insert(std::pair<TDropIndex, std::pair<std::vector<unsigned char>, const CItemDropInfo*> >(index, data));
			}
		}
	}

	for (itertype(map_commonDrop) it = map_commonDrop.begin(); it != map_commonDrop.end(); ++it)
	{
		__CreateCommonDropList_WriteItems(szCurrentLine, currentLineSize, fp, &it->second.first, it->second.second, bWriteChance);
	}

	fclose(fp);
}

void PUBLIC_CreateSpecialItemList(const std::string& stFileName, bool bWriteChance)
{
	sys_log(0, "PUBLIC: Create Special Item List \"%s\" (chance: %d)", stFileName.c_str(), bWriteChance);

	FILE* fp = fopen(stFileName.c_str(), "w");
	if (!fp)
	{
		sys_err("cannot open file to write \"%s\" !!", stFileName.c_str());
		return;
	}

	char szCurrentLine[512];
	const size_t currentLineSize = sizeof(szCurrentLine);

	snprintf(szCurrentLine, currentLineSize, "Truhen Item-Dropps:");
	fputs(szCurrentLine, fp);
	fputs("\n\n", fp);

	const std::map<DWORD, CSpecialItemGroup*>& rkMap = ITEM_MANAGER::instance().GetSpecialItemGroupMap();
	for (auto it = rkMap.begin(); it != rkMap.end(); ++it)
	{
		auto pProto = ITEM_MANAGER::instance().GetTable(it->first);
		if (!pProto)
		{
			sys_log(0, "PUBLIC_CreateSpecialItemList: cannot get item by vnum %u", it->first);
			continue;
		}

		snprintf(szCurrentLine, currentLineSize, "\t%s (%u):\n", pProto->locale_name(PUBLIC_TABLE_LANG).c_str(), it->first);
		fputs(szCurrentLine, fp);

		CSpecialItemGroup* pCurGroup = it->second;
		for (int i = 0; i < pCurGroup->GetGroupSize(); ++i)
		{
			auto pCurProto = ITEM_MANAGER::instance().GetTable(pCurGroup->GetVnum(i));
			if (!pCurProto)
			{
				sys_err("cannot get member item %u of group vnum %u", pCurGroup->GetVnum(i), it->first);
				continue;
			}

			int len = snprintf(szCurrentLine, currentLineSize, "\t\t- ");
			if (pCurGroup->GetCount(i) > 1)
				len += snprintf(szCurrentLine + len, currentLineSize - len, "%dx ", pCurGroup->GetCount(i));
			len += snprintf(szCurrentLine + len, currentLineSize - len, "%s (%u)", pCurProto->locale_name(PUBLIC_TABLE_LANG).c_str(), pCurGroup->GetVnum(i));
			if (bWriteChance)
			{
				float fPercent = 100.0f;
				int iCurPct = 1;
				int iMaxPct = pCurGroup->GetProb(pCurGroup->GetGroupSize() - 1);
				if (pCurGroup->GetGroupSize() > 1)
				{
					iCurPct = pCurGroup->GetProb(i);
					if (i > 0)
						iCurPct -= pCurGroup->GetProb(i - 1);
					fPercent = (float)iCurPct / (float)iMaxPct;
				}
				len += snprintf(szCurrentLine + len, currentLineSize - len, " [Chance %.4f%%] [%dx in %d Truhen]", fPercent * 100, iCurPct, iMaxPct);
			}

			fputs(szCurrentLine, fp);
			fputs("\n", fp);
		}

		fputs("\n", fp);
	}

	fclose(fp);
}

void PUBLIC_CreateMobDataList(const std::string& stFileName)
{
	sys_log(0, "PUBLIC: Create Mob Data List \"%s\"", stFileName.c_str());

	FILE* fp = fopen(stFileName.c_str(), "w");
	if (!fp)
	{
		sys_err("cannot open file to write \"%s\" !!", stFileName.c_str());
		return;
	}

	std::map<DWORD, std::string> mapRaceToName;
	mapRaceToName[RACE_FLAG_ANIMAL] = "Tiere";
	mapRaceToName[RACE_FLAG_UNDEAD] = "Untote";
	mapRaceToName[RACE_FLAG_DEVIL] = "Teufel";
	mapRaceToName[RACE_FLAG_HUMAN] = "Halbmenschen";
	mapRaceToName[RACE_FLAG_ORC] = "Ork";
	mapRaceToName[RACE_FLAG_MILGYO] = "Esoterische";

	std::map<DWORD, std::string> mapSubRaceToName;
	mapSubRaceToName[RACE_FLAG_ELEC] = "Blitz";
	mapSubRaceToName[RACE_FLAG_FIRE] = "Feuer";
	mapSubRaceToName[RACE_FLAG_ICE] = "Eis";
	mapSubRaceToName[RACE_FLAG_WIND] = "Wind";
	mapSubRaceToName[RACE_FLAG_EARTH] = "Erde";
	mapSubRaceToName[RACE_FLAG_DARK] = "Dunkelheit";
	mapSubRaceToName[RACE_FLAG_ZODIAC] = "Zodiac";

	char szCurrentLine[512];
	const size_t currentLineSize = sizeof(szCurrentLine);

	for (auto it = CMobManager::instance().begin(); it != CMobManager::instance().end(); ++it)
	{
		auto& rkTab = it->second->m_table;
		if (rkTab.type() != CHAR_TYPE_MONSTER && rkTab.type() != CHAR_TYPE_STONE)
			continue;

		snprintf(szCurrentLine, currentLineSize, "%s (%u):\n", rkTab.locale_name(PUBLIC_TABLE_LANG).c_str(), rkTab.vnum());
		fputs(szCurrentLine, fp);

		snprintf(szCurrentLine, currentLineSize, "\tMax HP: %u\n", rkTab.max_hp());
		fputs(szCurrentLine, fp);

		snprintf(szCurrentLine, currentLineSize, "\tEXP: %u\n", rkTab.exp());
		fputs(szCurrentLine, fp);

		snprintf(szCurrentLine, currentLineSize, "\tGattung: ");
		fputs(szCurrentLine, fp);

		DWORD dwCounter = 0;
		for (auto it = mapRaceToName.begin(); it != mapRaceToName.end(); ++it)
		{
			if (IS_SET(rkTab.race_flag(), it->first))
			{
				if (dwCounter > 0)
				{
					snprintf(szCurrentLine, currentLineSize, ", ");
					fputs(szCurrentLine, fp);
				}

				snprintf(szCurrentLine, currentLineSize, "%s", it->second.c_str());
				fputs(szCurrentLine, fp);
				++dwCounter;
			}
		}
		if (rkTab.type() == CHAR_TYPE_STONE)
		{
			if (dwCounter > 0)
			{
				snprintf(szCurrentLine, currentLineSize, ", ");
				fputs(szCurrentLine, fp);
			}

			snprintf(szCurrentLine, currentLineSize, "Metinstein");
			fputs(szCurrentLine, fp);
			++dwCounter;
		}

		if (dwCounter == 0)
		{
			snprintf(szCurrentLine, currentLineSize, "Keine");
			fputs(szCurrentLine, fp);
		}

		fputs("\n", fp);

		snprintf(szCurrentLine, currentLineSize, "\tUnter-Gattung: ");
		fputs(szCurrentLine, fp);

		dwCounter = 0;
		for (auto it = mapSubRaceToName.begin(); it != mapSubRaceToName.end(); ++it)
		{
			if (IS_SET(rkTab.race_flag(), it->first))
			{
				if (dwCounter > 0)
				{
					snprintf(szCurrentLine, currentLineSize, ", ");
					fputs(szCurrentLine, fp);
				}

				snprintf(szCurrentLine, currentLineSize, "%s", it->second.c_str());
				fputs(szCurrentLine, fp);
				++dwCounter;
			}
		}

		if (dwCounter == 0)
		{
			snprintf(szCurrentLine, currentLineSize, "Keine");
			fputs(szCurrentLine, fp);
		}

		fputs("\n", fp);
		fputs("\n", fp);
	}

	fclose(fp);
}

void PUBLIC_CreateGMCommandFile(const std::string& stFileName, int iGMLevel)
{
	sys_log(0, "PUBLIC: Create GM Command List \"%s\" (gm_level: %d)", stFileName.c_str(), iGMLevel);

	FILE* fp = fopen(stFileName.c_str(), "w");
	if (!fp)
	{
		sys_err("cannot open file to write \"%s\" !!", stFileName.c_str());
		return;
	}

	const char* arGMNameList[GM_MAX_NUM] = {
		"Spieler",
		"Test-Gamemaster",
		"Gamemaster",
		"Super-Gamemaster",
		"Community-Manager",
		"Serveradministrator"
	};

	char szCurrentLine[512];
	const size_t currentLineSize = sizeof(szCurrentLine);

	if (iGMLevel >= GM_PLAYER && iGMLevel <= GM_IMPLEMENTOR)
		snprintf(szCurrentLine, currentLineSize, "Chat-Befehle für %s:\n", arGMNameList[iGMLevel]);
	else
		snprintf(szCurrentLine, currentLineSize, "Chat-Befehle:\n");
	fputs(szCurrentLine, fp);

	for (int i = 0; *cmd_info[i].command != '\n'; ++i)
	{
		if (iGMLevel == -1)
		{
			snprintf(szCurrentLine, currentLineSize, "\t/%s (benötigter Rang: %s)\n", cmd_info[i].command, arGMNameList[cmd_info[i].gm_level]);
		}
		else if (iGMLevel >= cmd_info[i].gm_level && (iGMLevel == GM_PLAYER || cmd_info[i].gm_level != GM_PLAYER))
		{
			snprintf(szCurrentLine, currentLineSize, "\t/%s\n", cmd_info[i].command);
		}
		else
			continue;

		fputs(szCurrentLine, fp);
	}

	fclose(fp);
}
