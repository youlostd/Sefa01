#include "StdAfx.h"
#include "PythonNonPlayer.h"

#include "InstanceBase.h"
#include "PythonCharacterManager.h"

PyObject * nonplayerGetEventType(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualNumber;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualNumber))
		return Py_BuildException();

	BYTE iType = CPythonNonPlayer::Instance().GetEventType(iVirtualNumber);

	return Py_BuildValue("i", iType);
}

PyObject * nonplayerGetEventTypeByVID(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualID))
		return Py_BuildException();

	BYTE iType = CPythonNonPlayer::Instance().GetEventTypeByVID(iVirtualID);

	return Py_BuildValue("i", iType);
}

PyObject * nonplayerGetRaceNumByVID(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualID))
		return Py_BuildException();

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVirtualID);

	if (!pInstance)
		return Py_BuildValue("i", -1);

	return Py_BuildValue("i", pInstance->GetVirtualNumber());
}

PyObject * nonplayerGetLevelByVID(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualID))
		return Py_BuildException();

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVirtualID);

	if (!pInstance)
		return Py_BuildValue("i", -1);

	const CPythonNonPlayer::TMobTable * pMobTable = CPythonNonPlayer::Instance().GetTable(pInstance->GetVirtualNumber());

	if (!pMobTable)
		return Py_BuildValue("i", -1);

	float fAverageLevel = pMobTable->level();//(float(pMobTable->abLevelRange[0]) + float(pMobTable->abLevelRange[1])) / 2.0f;
	fAverageLevel = floor(fAverageLevel + 0.5f);
	return Py_BuildValue("i", int(fAverageLevel));
}

PyObject * nonplayerGetGradeByVID(PyObject * poSelf, PyObject * poArgs)
{
	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVirtualID))
		return Py_BuildException();

	CInstanceBase * pInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVirtualID);

	if (!pInstance)
		return Py_BuildValue("i", -1);

	const CPythonNonPlayer::TMobTable * pMobTable = CPythonNonPlayer::Instance().GetTable(pInstance->GetVirtualNumber());

	if (!pMobTable)
		return Py_BuildValue("i", -1);

	return Py_BuildValue("i", pMobTable->rank());
}


PyObject * nonplayerGetMonsterName(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer=CPythonNonPlayer::Instance();
	return Py_BuildValue("s", rkNonPlayer.GetMonsterName(iVNum));
}

PyObject * nonplayerGetMonsterLevel(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->level());
}

PyObject * nonplayerGetMonsterHT(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->con());
}

PyObject * nonplayerGetMonsterST(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->str());
}

PyObject * nonplayerGetMonsterDX(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->dex());
}

PyObject * nonplayerGetMonsterMaxHP(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->max_hp());
}

PyObject * nonplayerGetMonsterDamage(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("ii", pkTab->damage_min(), pkTab->damage_max());
}

PyObject * nonplayerGetMonsterGold(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("ii", pkTab->gold_min(), pkTab->gold_max());
}

PyObject * nonplayerGetMonsterDamageMultiply(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("f", pkTab->dam_multiply());
}

PyObject * nonplayerGetMonsterDefense(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->def());
}

PyObject * nonplayerGetMonsterRaceFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->race_flag());
}

PyObject * nonplayerGetMonsterImmuneFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->immune_flag());
}

PyObject * nonplayerGetMonsterResistValue(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	int iType;
	if (!PyTuple_GetInteger(poArgs, 1, &iType))
		return Py_BuildException();

	if (iType < 0 || iType > CPythonNonPlayer::MOB_RESISTS_MAX_NUM)
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->resists(iType));
}

PyObject * nonplayerGetMonsterEXP(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pkTab = rkNonPlayer.GetTable(iVNum);

	return Py_BuildValue("i", pkTab->exp());
}

PyObject * nonplayerIsMonsterStone(PyObject * poSelf, PyObject * poArgs)
{
	int iVNum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVNum))
		return Py_BuildException();

	CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
	const CPythonNonPlayer::TMobTable* pTable = rkNonPlayer.GetTable(iVNum);
	if (pTable)
		return Py_BuildValue("b", pTable->type() == CPythonNonPlayer::STONE);

	return Py_BuildValue("b", false);
}

PyObject * nonplayerLoadNonPlayerData(PyObject * poSelf, PyObject * poArgs)
{
	char * szFileName;
	if(!PyTuple_GetString(poArgs, 0, &szFileName))
		return Py_BuildException();

	CPythonNonPlayer::Instance().LoadNonPlayerData(szFileName);
	return Py_BuildNone();
}

PyObject* nonplayerGetVnumByNamePart(PyObject* poSelf, PyObject* poArgs)
{
	char * szNamePart;
	if (!PyTuple_GetString(poArgs, 0, &szNamePart))
		return Py_BadArgument();

	return Py_BuildValue("i", CPythonNonPlayer::Instance().GetVnumByNamePart(szNamePart));
}

PyObject* nonplayerBuildWikiSearchList(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNonPlayer::Instance().BuildWikiSearchList();
	return Py_BuildNone();
}

void initNonPlayer()
{
	static PyMethodDef s_methods[] =
	{
		{ "GetEventType",				nonplayerGetEventType,				METH_VARARGS },
		{ "GetEventTypeByVID",			nonplayerGetEventTypeByVID,			METH_VARARGS },
		{ "GetRaceNumByVID",			nonplayerGetRaceNumByVID,			METH_VARARGS },
		{ "GetLevelByVID",				nonplayerGetLevelByVID,				METH_VARARGS },
		{ "GetGradeByVID",				nonplayerGetGradeByVID,				METH_VARARGS },
		{ "GetMonsterName",				nonplayerGetMonsterName,			METH_VARARGS },
		{ "GetMonsterLevel",			nonplayerGetMonsterLevel,			METH_VARARGS },
		{ "GetMonsterHT",				nonplayerGetMonsterHT,				METH_VARARGS },
		{ "GetMonsterST",				nonplayerGetMonsterST,				METH_VARARGS },
		{ "GetMonsterDX",				nonplayerGetMonsterDX,				METH_VARARGS },
		{ "GetMonsterMaxHP",			nonplayerGetMonsterMaxHP,			METH_VARARGS },
		{ "GetMonsterDamage",			nonplayerGetMonsterDamage,			METH_VARARGS },
		{ "GetMonsterGold",				nonplayerGetMonsterGold,			METH_VARARGS },
		{ "GetMonsterDamageMultiply",	nonplayerGetMonsterDamageMultiply,	METH_VARARGS },
		{ "GetMonsterDefense",			nonplayerGetMonsterDefense,			METH_VARARGS },
		{ "GetMonsterRaceFlag",			nonplayerGetMonsterRaceFlag,		METH_VARARGS },
		{ "GetMonsterImmuneFlag",		nonplayerGetMonsterImmuneFlag,		METH_VARARGS },
		{ "GetMonsterResistValue",		nonplayerGetMonsterResistValue,		METH_VARARGS },
		{ "GetMonsterExp",				nonplayerGetMonsterEXP,				METH_VARARGS },
		{ "IsMonsterStone",				nonplayerIsMonsterStone,			METH_VARARGS },
		{ "GetVnumByNamePart",			nonplayerGetVnumByNamePart,			METH_VARARGS },
		{ "BuildWikiSearchList",		nonplayerBuildWikiSearchList,		METH_VARARGS },

		{ "LoadNonPlayerData",			nonplayerLoadNonPlayerData,			METH_VARARGS },

		{ NULL,							NULL,								NULL		 },
	};

	PyObject * poModule = Py_InitModule("nonplayer", s_methods);

	PyModule_AddIntConstant(poModule, "ON_CLICK_EVENT_NONE",		CPythonNonPlayer::ON_CLICK_EVENT_NONE);
	PyModule_AddIntConstant(poModule, "ON_CLICK_EVENT_BATTLE",		CPythonNonPlayer::ON_CLICK_EVENT_BATTLE);
	PyModule_AddIntConstant(poModule, "ON_CLICK_EVENT_SHOP",		CPythonNonPlayer::ON_CLICK_EVENT_SHOP);
	PyModule_AddIntConstant(poModule, "ON_CLICK_EVENT_TALK",		CPythonNonPlayer::ON_CLICK_EVENT_TALK);
	PyModule_AddIntConstant(poModule, "ON_CLICK_EVENT_VEHICLE",		CPythonNonPlayer::ON_CLICK_EVENT_VEHICLE);

	PyModule_AddIntConstant(poModule, "PAWN", 0);
	PyModule_AddIntConstant(poModule, "S_PAWN", 1);
	PyModule_AddIntConstant(poModule, "KNIGHT", 2);
	PyModule_AddIntConstant(poModule, "S_KNIGHT", 3);
	PyModule_AddIntConstant(poModule, "BOSS", 4);
	PyModule_AddIntConstant(poModule, "KING", 5);

	PyModule_AddIntConstant(poModule, "IMMUNE_STUN",			CItemData::IMMUNE_STUN);
	PyModule_AddIntConstant(poModule, "IMMUNE_SLOW",			CItemData::IMMUNE_SLOW);
	PyModule_AddIntConstant(poModule, "IMMUNE_FALL",			CItemData::IMMUNE_FALL);
	PyModule_AddIntConstant(poModule, "IMMUNE_CURSE",			CItemData::IMMUNE_CURSE);
	PyModule_AddIntConstant(poModule, "IMMUNE_POISON",			CItemData::IMMUNE_POISON);
	PyModule_AddIntConstant(poModule, "IMMUNE_TERROR",			CItemData::IMMUNE_TERROR);
	PyModule_AddIntConstant(poModule, "IMMUNE_REFLECT",			CItemData::IMMUNE_REFLECT);
	PyModule_AddIntConstant(poModule, "IMMUNE_FLAG_MAX_NUM",	CItemData::IMMUNE_FLAG_MAX_NUM);

	PyModule_AddIntConstant(poModule, "MOB_RESIST_SWORD",		CPythonNonPlayer::MOB_RESIST_SWORD);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_TWOHAND",		CPythonNonPlayer::MOB_RESIST_TWOHAND);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_DAGGER",		CPythonNonPlayer::MOB_RESIST_DAGGER);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_BELL",		CPythonNonPlayer::MOB_RESIST_BELL);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_FAN",			CPythonNonPlayer::MOB_RESIST_FAN);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_BOW",			CPythonNonPlayer::MOB_RESIST_BOW);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_CLAW",		CPythonNonPlayer::MOB_RESIST_CLAW);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_FIRE",		CPythonNonPlayer::MOB_RESIST_FIRE);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_ELECT",		CPythonNonPlayer::MOB_RESIST_ELECT);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_MAGIC",		CPythonNonPlayer::MOB_RESIST_MAGIC);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_WIND",		CPythonNonPlayer::MOB_RESIST_WIND);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_POISON",		CPythonNonPlayer::MOB_RESIST_POISON);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_BLEEDING",	CPythonNonPlayer::MOB_RESIST_BLEEDING);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_EARTH",		CPythonNonPlayer::MOB_RESIST_EARTH);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_ICE",			CPythonNonPlayer::MOB_RESIST_ICE);
	PyModule_AddIntConstant(poModule, "MOB_RESIST_DARK",		CPythonNonPlayer::MOB_RESIST_DARK);
	PyModule_AddIntConstant(poModule, "MOB_RESISTS_MAX_NUM",	CPythonNonPlayer::MOB_RESISTS_MAX_NUM);

	PyModule_AddIntConstant(poModule, "RACE_FLAG_ANIMAL",		CPythonNonPlayer::RACE_FLAG_ANIMAL);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_UNDEAD",		CPythonNonPlayer::RACE_FLAG_UNDEAD);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_DEVIL",		CPythonNonPlayer::RACE_FLAG_DEVIL);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_HUMAN",		CPythonNonPlayer::RACE_FLAG_HUMAN);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_ORC",			CPythonNonPlayer::RACE_FLAG_ORC);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_MILGYO",		CPythonNonPlayer::RACE_FLAG_MILGYO);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_INSECT",		CPythonNonPlayer::RACE_FLAG_INSECT);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_FIRE",			CPythonNonPlayer::RACE_FLAG_FIRE);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_ICE",			CPythonNonPlayer::RACE_FLAG_ICE);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_DESERT",		CPythonNonPlayer::RACE_FLAG_DESERT);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_TREE",			CPythonNonPlayer::RACE_FLAG_TREE);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_ELEC",			CPythonNonPlayer::RACE_FLAG_ELEC);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_WIND",			CPythonNonPlayer::RACE_FLAG_WIND);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_EARTH",		CPythonNonPlayer::RACE_FLAG_EARTH);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_DARK",			CPythonNonPlayer::RACE_FLAG_DARK);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_MAX_NUM",		CPythonNonPlayer::RACE_FLAG_MAX_NUM);
	PyModule_AddIntConstant(poModule, "RACE_FLAG_ZODIAC",		CPythonNonPlayer::RACE_FLAG_ZODIAC);
}