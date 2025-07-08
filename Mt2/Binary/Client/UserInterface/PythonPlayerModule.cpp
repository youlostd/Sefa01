#include "StdAfx.h"
#include "PythonPlayer.h"
#include "PythonApplication.h"
#include "Test.h"

extern bool PyTuple_GetWindow(PyObject* poArgs, int pos, UI::CWindow ** ppRetWindow);

extern const DWORD c_iSkillIndex_Tongsol	= 121;
extern const DWORD c_iSkillIndex_Combo		= 122;
extern const DWORD c_iSkillIndex_Fishing	= 123;
extern const DWORD c_iSkillIndex_Mining		= 124;
extern const DWORD c_iSkillIndex_Making		= 125;
extern const DWORD c_iSkillIndex_Language1	= 126;
extern const DWORD c_iSkillIndex_Language2	= 127;
extern const DWORD c_iSkillIndex_Language3	= 128;
extern const DWORD c_iSkillIndex_Polymorph	= 129;
extern const DWORD c_iSkillIndex_Riding		= 130;
extern const DWORD c_iSkillIndex_Summon		= 131;

enum
{
	EMOTION_CLAP		= 1,
	EMOTION_CHEERS_1,
	EMOTION_CHEERS_2,
	EMOTION_DANCE_1,
	EMOTION_DANCE_2,
	EMOTION_DANCE_3,
	EMOTION_DANCE_4,
	EMOTION_DANCE_5,
	EMOTION_DANCE_6,		// 강남스타일
	EMOTION_CONGRATULATION,
	EMOTION_FORGIVE,
	EMOTION_ANGRY,
	
	EMOTION_ATTRACTIVE,
	EMOTION_SAD,
	EMOTION_SHY,
	EMOTION_CHEERUP,
	EMOTION_BANTER,
	EMOTION_JOY,

	EMOTION_KISS		= 51,
	EMOTION_FRENCH_KISS,
	EMOTION_SLAP,

};

std::map<int, CGraphicImage *> m_kMap_iEmotionIndex_pkIconImage;

extern int TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE;

PyObject * playerGetHorseBonusProto(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bBonusIndex;
	if (!PyTuple_GetByte(poArgs, 0, &bBonusIndex))
		return Py_BadArgument();
	BYTE bLevel;
	if (!PyTuple_GetByte(poArgs, 1, &bLevel))
		return Py_BadArgument();

	CPythonPlayer& rkPlayer = CPythonPlayer::Instance();
	CPythonPlayer::THorseBonusProtoTable* pProto = rkPlayer.GetHorseBonusProto(bLevel);
	if (!pProto)
		return Py_BuildValue("iii", 0, 0, 0);

	return Py_BuildValue("iii", pProto->bApplyType[bBonusIndex], pProto->dwApplyValue[bBonusIndex], pProto->bItemCount);
}

PyObject * playerPickCloseItem(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().PickCloseItem();
	return Py_BuildNone();
}

#ifdef COMBAT_ZONE
PyObject * playerIsCombatZoneMap(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pInstance)
		return Py_BuildValue("i", 0);
	return Py_BuildValue("i", pInstance->IsCombatZoneMap());
}

PyObject * playerGetCombatZonePoints(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pInstance)
		return Py_BuildValue("i", 0);
	return Py_BuildValue("i", pInstance->GetCombatZonePoints());
}
#endif


PyObject * playerSetGameWindow(PyObject* poSelf, PyObject* poArgs)
{
	PyObject * pyHandle;
	if (!PyTuple_GetObject(poArgs, 0, &pyHandle))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetGameWindow(pyHandle);
	return Py_BuildNone();
}


PyObject * playerSetQuickCameraMode(PyObject* poSelf, PyObject* poArgs)
{
	int nIsEnable;
	if (!PyTuple_GetInteger(poArgs, 0, &nIsEnable))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetQuickCameraMode(nIsEnable ? true : false);	

	return Py_BuildNone();
}

// Test Code
PyObject * playerSetMainCharacterIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CPythonPlayer::Instance().SetMainCharacterIndex(iVID);
	CPythonCharacterManager::Instance().SetMainInstance(iVID);

	return Py_BuildNone();
}
// Test Code

PyObject * playerGetMainCharacterIndex(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetMainCharacterIndex());
}

PyObject * playerGetMainCharacterName(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CPythonPlayer::Instance().GetName());
}

PyObject * playerGetMainCharacterPosition(PyObject* poSelf, PyObject* poArgs)
{
	TPixelPosition kPPosMainActor;
	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.NEW_GetMainActorPosition(&kPPosMainActor);
	return Py_BuildValue("fff", kPPosMainActor.x, kPPosMainActor.y, kPPosMainActor.z);
}

PyObject * playerIsMainCharacterIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsMainCharacterIndex(iVID));
}

PyObject * playerCanAttackInstance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVID);
	if (!pMainInstance)
		return Py_BuildValue("i", 0);
	if (!pTargetInstance)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pMainInstance->IsAttackableInstance(*pTargetInstance));
}

PyObject * playerIsActingEmotion(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pMainInstance)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pMainInstance->IsActingEmotion());
}

PyObject * playerIsPVPInstance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVID);
	if (!pMainInstance)
		return Py_BuildValue("i", 0);
	if (!pTargetInstance)
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", pMainInstance->IsPVPInstance(*pTargetInstance));
}

PyObject * playerIsSameEmpire(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVID);
	if (!pMainInstance)
		return Py_BuildValue("i", FALSE);
	if (!pTargetInstance)
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", pMainInstance->IsSameEmpire(*pTargetInstance));
}

PyObject * playerIsChallengeInstance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsChallengeInstance(iVID));
}

PyObject * playerIsRevengeInstance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsRevengeInstance(iVID));
}

PyObject * playerIsCantFightInstance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsCantFightInstance(iVID));
}

PyObject * playerGetCharacterDistance(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	CInstanceBase * pTargetInstance = CPythonCharacterManager::Instance().GetInstancePtr(iVID);
	if (!pMainInstance)
		return Py_BuildValue("f", -1.0f);
	if (!pTargetInstance)
		return Py_BuildValue("f", -1.0f);

	return Py_BuildValue("f", pMainInstance->GetDistance(pTargetInstance));
}

PyObject * playerIsInSafeArea(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pMainInstance)
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", pMainInstance->IsInSafe());
}

PyObject * playerIsMountingHorse(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pMainInstance)
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", pMainInstance->IsMountingHorse());
}

PyObject * playerIsObserverMode(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	return Py_BuildValue("i", rkPlayer.IsObserverMode());
}

PyObject * playerActEmotion(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.ActEmotion(iVID);
	return Py_BuildNone();
}

PyObject * playerShowPlayer(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (pMainInstance)
		pMainInstance->GetGraphicThingInstanceRef().Show();
	return Py_BuildNone();
}

PyObject * playerHidePlayer(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pMainInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (pMainInstance)
		pMainInstance->GetGraphicThingInstanceRef().Hide();
	return Py_BuildNone();
}


PyObject * playerComboAttack(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().NEW_Attack();
	return Py_BuildNone();
}


PyObject * playerRegisterEffect(PyObject* poSelf, PyObject* poArgs)
{
	int iEft;
	if (!PyTuple_GetInteger(poArgs, 0, &iEft))
		return Py_BadArgument();

	char* szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	if (!rkPlayer.RegisterEffect(iEft, szFileName, false))
		return Py_BuildException("CPythonPlayer::RegisterEffect(eEft=%d, szFileName=%s", iEft, szFileName);

	return Py_BuildNone();
}

PyObject * playerRegisterCacheEffect(PyObject* poSelf, PyObject* poArgs)
{
	int iEft;
	if (!PyTuple_GetInteger(poArgs, 0, &iEft))
		return Py_BadArgument();

	char* szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BadArgument();

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	if (!rkPlayer.RegisterEffect(iEft, szFileName, true))
		return Py_BuildException("CPythonPlayer::RegisterEffect(eEft=%d, szFileName=%s", iEft, szFileName);

	return Py_BuildNone();
}

PyObject * playerSetAttackKeyState(PyObject* poSelf, PyObject* poArgs)
{
	int isPressed;
	if (!PyTuple_GetInteger(poArgs, 0, &isPressed))
		return Py_BuildException("playerSetAttackKeyState(isPressed) - There is no first argument");
	
	int nBotCheck;
	if (PyTuple_GetInteger(poArgs, 1, &nBotCheck) && nBotCheck == 938)
	{		
		// Fishbot Detection, No Spacekey pressed..
		// if (CPythonApplication::Instance().IsPressed(DIK_SPACE))
		// {
		static DWORD gs_dwLastSent = 0;
		if (gs_dwLastSent < time(0) - 7)
		{
			gs_dwLastSent = time(0);
			CPythonNetworkStream::instance().SendHitSpacebarPacket();
		}
	// }
	}
	
	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.SetAttackKeyState(isPressed ? true : false);
	return Py_BuildNone();
}

PyObject * playerSetSingleDIKKeyState(PyObject* poSelf, PyObject* poArgs)
{
	int eDIK;
	if (!PyTuple_GetInteger(poArgs, 0, &eDIK))
		return Py_BuildException("playerSetSingleDIKKeyState(eDIK, isPressed) - There is no first argument");

	int isPressed;
	if (!PyTuple_GetInteger(poArgs, 1, &isPressed))
		return Py_BuildException("playerSetSingleDIKKeyState(eDIK, isPressed) - There is no second argument");

	CPythonPlayer & rkPlayer = CPythonPlayer::Instance();
	rkPlayer.NEW_SetSingleDIKKeyState(eDIK, isPressed ? true : false);
	return Py_BuildNone();
}

PyObject * playerEndKeyWalkingImmediately(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().NEW_Stop();
	return Py_BuildNone();
}


PyObject * playerStartMouseWalking(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

PyObject * playerEndMouseWalking(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

PyObject * playerResetCameraRotation(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().NEW_ResetCameraRotation();
	return Py_BuildNone();
}

PyObject* playerSetAutoCameraRotationSpeed(PyObject* poSelf, PyObject* poArgs)
{
	float fCmrRotSpd;
	if (!PyTuple_GetFloat(poArgs, 0, &fCmrRotSpd))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetAutoCameraRotationSpeed(fCmrRotSpd);
	return Py_BuildNone();
}

PyObject* playerSetMouseState(PyObject* poSelf, PyObject* poArgs)
{
	int eMBT;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBT))
		return Py_BuildException();	

	int eMBS;
	if (!PyTuple_GetInteger(poArgs, 1, &eMBS))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetMouseState(eMBT, eMBS);

	return Py_BuildNone();
}

PyObject* playerSetMouseFunc(PyObject* poSelf, PyObject* poArgs)
{
	int eMBT;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBT))
		return Py_BuildException();	

	int eMBS;
	if (!PyTuple_GetInteger(poArgs, 1, &eMBS))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetMouseFunc(eMBT, eMBS);

	return Py_BuildNone();
}

PyObject* playerGetMouseFunc(PyObject* poSelf, PyObject* poArgs)
{
	int eMBT;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBT))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	return Py_BuildValue("i", rkPlayer.NEW_GetMouseFunc(eMBT));
}

PyObject* playerSetMouseMiddleButtonState(PyObject* poSelf, PyObject* poArgs)
{
	int eMBS;
	if (!PyTuple_GetInteger(poArgs, 0, &eMBS))
		return Py_BuildException();	

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.NEW_SetMouseMiddleButtonState(eMBS);

	return Py_BuildNone();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

PyObject * playerGetName(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("s", CPythonPlayer::Instance().GetName()); 
}

PyObject * playerGetRace(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetRace());
}

PyObject * playerGetJob(PyObject* poSelf, PyObject* poArgs)
{
	int race = CPythonPlayer::Instance().GetRace();
	int job = RaceToJob(race);
	return Py_BuildValue("i", job);
}

PyObject * playerGetPlayTime(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetPlayTime()); 
}

PyObject * playerSetPlayTime(PyObject* poSelf, PyObject* poArgs)
{
	int iTime;
	if (!PyTuple_GetInteger(poArgs, 0, &iTime))
		return Py_BuildException();

	CPythonPlayer::Instance().SetPlayTime(iTime);
	return Py_BuildNone();
}

PyObject * playerIsSkillCoolTime(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsSkillCoolTime(iSlotIndex));
}

PyObject * playerGetSkillCoolTime(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	float fCoolTime = CPythonPlayer::Instance().GetSkillCoolTime(iSlotIndex);
	float fElapsedCoolTime = CPythonPlayer::Instance().GetSkillElapsedCoolTime(iSlotIndex);
	return Py_BuildValue("ff", fCoolTime, fElapsedCoolTime);
}

PyObject * playerIsSkillActive(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsSkillActive(iSlotIndex));
}

PyObject * playerUseGuildSkill(PyObject* poSelf, PyObject* poArgs)
{
	int iSkillSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillSlotIndex))
		return Py_BuildException();

	CPythonPlayer::Instance().UseGuildSkill(iSkillSlotIndex);
	return Py_BuildNone();
}

PyObject * playerAffectIndexToSkillIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iAffectIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iAffectIndex))
		return Py_BuildException();

	DWORD dwSkillIndex;
	if (!CPythonPlayer::Instance().AffectIndexToSkillIndex(iAffectIndex, &dwSkillIndex))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", dwSkillIndex);
}

PyObject * playerSkillIndexToAffectIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iSkillIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillIndex))
		return Py_BuildException();

	int iIgnoreCount;
	if (!PyTuple_GetInteger(poArgs, 1, &iIgnoreCount))
		iIgnoreCount = 0;

	DWORD dwAffectIndex;
	if (!CPythonPlayer::Instance().SkillIndexToAffectIndex(iSkillIndex, &dwAffectIndex, iIgnoreCount))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", dwAffectIndex);
}

PyObject * playerGetEXP(PyObject* poSelf, PyObject* poArgs)
{
	LONGLONG llEXP = CPythonPlayer::Instance().GetStatus(POINT_EXP);
	return Py_BuildValue("L", llEXP);
}

PyObject * playerGetStatus(PyObject* poSelf, PyObject* poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BuildException();

	LONGLONG llValue = CPythonPlayer::Instance().GetStatus(iType);

	if (POINT_ATT_SPEED == iType)
	{
		CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
		if (pInstance && (CItemData::WEAPON_TWO_HANDED == pInstance->GetWeaponType()))
		{
			llValue -= TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE;
		}
	}

	return Py_BuildValue("L", llValue);
}

PyObject * playerSetStatus(PyObject* poSelf, PyObject* poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BuildException();

	int iValue;
	if (!PyTuple_GetInteger(poArgs, 1, &iValue))
		return Py_BuildException();

	CPythonPlayer::Instance().SetStatus(iType, iValue);
	return Py_BuildNone();
}

PyObject * playerGetRealStatus(PyObject* poSelf, PyObject* poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BuildException();

	long iValue = CPythonPlayer::Instance().GetRealStatus(iType);

	if (POINT_ATT_SPEED == iType)
	{
		CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
		if (pInstance && (CItemData::WEAPON_TWO_HANDED == pInstance->GetWeaponType()))
		{
			iValue -= TWOHANDED_WEWAPON_ATT_SPEED_DECREASE_VALUE;
		}
	}

	return Py_BuildValue("i", iValue);
}

PyObject * playerGetElk(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("L", CPythonPlayer::Instance().GetStatus(POINT_GOLD));
}

PyObject * playerGetGuildID(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (!pInstance)
		return Py_BuildValue("i", 0);
	return Py_BuildValue("i", pInstance->GetGuildID());
}

PyObject * playerGetGuildName(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	if (pInstance)
	{
		DWORD dwID = pInstance->GetGuildID();
		std::string strName;
		if (CPythonGuild::Instance().GetGuildName(dwID, &strName))
			return Py_BuildValue("s", strName.c_str());
	}
	return Py_BuildValue("s", "");
}

PyObject * playerGetAlignmentData(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase * pInstance = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	int iAlignmentPoint = 0;
	int iAlignmentGrade = 4;
	if (pInstance)
	{
		iAlignmentPoint = pInstance->GetAlignment();
		iAlignmentGrade = pInstance->GetAlignmentGrade();
	}
	return Py_BuildValue("ii", iAlignmentPoint, iAlignmentGrade);
}

PyObject * playerSetSkill(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	int iSkillIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSkillIndex))
		return Py_BuildException();

	CPythonPlayer::Instance().SetSkill(iSlotIndex, iSkillIndex);
	return Py_BuildNone();
}

PyObject * playerGetSkillIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().GetSkillIndex(iSlotIndex));
}

PyObject * playerGetSkillSlotIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iSkillIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillIndex))
		return Py_BuildException();

	DWORD dwSlotIndex;
	if (!CPythonPlayer::Instance().GetSkillSlotIndex(iSkillIndex, &dwSlotIndex))
		return Py_BuildValue("i", -1);

	return Py_BuildValue("i", dwSlotIndex);
}

PyObject * playerGetSkillGrade(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().GetSkillGrade(iSlotIndex));
}

PyObject * playerGetSkillLevel(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().GetSkillLevel(iSlotIndex));
}

PyObject * playerGetSkillEfficientPercentage(PyObject* poSelf, PyObject* poArgs)
{
	int iSkillLevel;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillLevel))
		return Py_BuildException();

	return Py_BuildValue("f", CLocaleManager::instance().GetSkillPower(iSkillLevel) / 100.0f);
}
PyObject * playerGetSkillCurrentEfficientPercentage(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("f", CPythonPlayer::Instance().GetSkillCurrentEfficientPercentage(iSlotIndex));
}
PyObject * playerGetSkillNextEfficientPercentage(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("f", CPythonPlayer::Instance().GetSkillNextEfficientPercentage(iSlotIndex));
}

PyObject * playerClickSkillSlot(PyObject * poSelf, PyObject * poArgs)
{
	int iSkillSlot;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillSlot))
		return Py_BadArgument();

	CPythonPlayer::Instance().ClickSkillSlot(iSkillSlot);

	return Py_BuildNone();
}

PyObject * playerChangeCurrentSkillNumberOnly(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BadArgument();

	CPythonPlayer::Instance().ChangeCurrentSkillNumberOnly(iSlotIndex);

	return Py_BuildNone();
}

PyObject * playerClearSkillDict(PyObject * poSelf, PyObject * poArgs)
{
	CPythonPlayer::Instance().ClearSkillDict();
	return Py_BuildNone();
}

PyObject * playerMoveItem(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos srcCell;
	TItemPos dstCell;
	switch (PyTuple_Size(poArgs))
	{
	case 2:
		int iSourceSlotIndex;
		if (!PyTuple_GetInteger(poArgs, 0, &srcCell.cell))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &dstCell.cell))
			return Py_BuildException();
		break;
	case 4:
		if (!PyTuple_GetByte(poArgs, 0, &srcCell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &srcCell.cell))
			return Py_BuildException();
		if (!PyTuple_GetByte(poArgs, 2, &dstCell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 3, &dstCell.cell))
			return Py_BuildException();
	default:
		return Py_BuildException();
	}

	CPythonPlayer::Instance().MoveItemData(srcCell, dstCell);
	return Py_BuildNone();
}

PyObject * playerSendClickItemPacket(PyObject* poSelf, PyObject* poArgs)
{
	int ivid;
	if (!PyTuple_GetInteger(poArgs, 0, &ivid))
		return Py_BuildException();

	CPythonPlayer::Instance().SendClickItemPacket(ivid);
	return Py_BuildNone();
}

PyObject * playerGetItemIndex(PyObject* poSelf, PyObject* poArgs)
{
	switch (PyTuple_Size(poArgs))
	{
	case 1:
		{
			int iSlotIndex;
			if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
				return Py_BuildException();

			int ItemIndex = CPythonPlayer::Instance().GetItemIndex(TItemPos (INVENTORY, iSlotIndex));
			return Py_BuildValue("i", ItemIndex);
		}
	case 2:
		{
			TItemPos Cell;
			if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
				return Py_BuildException();
			if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
				return Py_BuildException();

			int ItemIndex = CPythonPlayer::Instance().GetItemIndex(Cell);
			return Py_BuildValue("i", ItemIndex);
		}
	default:
		return Py_BuildException();

	}
}

PyObject * playerGetItemFlags(PyObject* poSelf, PyObject* poArgs)
{
	DWORD item_index = 0;

	switch (PyTuple_Size(poArgs))
	{
	case 1:
		{
			int iSlotIndex;
			if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
				return Py_BadArgument();

			item_index = CPythonPlayer::Instance().GetItemIndex(TItemPos(INVENTORY, iSlotIndex));
		}
		break;

	case 2:
		{
			TItemPos Cell;
			if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
				return Py_BadArgument();

			if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
				return Py_BadArgument();

			item_index = CPythonPlayer::Instance().GetItemIndex(Cell);
		}
		break;

	default:
		return Py_BadArgument();
	}

	CItemData* proto;
	if (!CItemManager::Instance().GetItemDataPointer(item_index, &proto))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", proto->GetFlags());
}

PyObject * playerGetItemCount(PyObject* poSelf, PyObject* poArgs)
{
	switch (PyTuple_Size(poArgs))
	{
	case 1:
		{
			int iSlotIndex;
			if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
				return Py_BuildException();

			int ItemNum = CPythonPlayer::Instance().GetItemCount(TItemPos (INVENTORY, iSlotIndex));
			return Py_BuildValue("i", ItemNum);
		}
	case 2:
		{
			TItemPos Cell;
			if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
				return Py_BuildException();

			if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
				return Py_BuildException();

			int ItemNum = CPythonPlayer::Instance().GetItemCount(Cell);

			return Py_BuildValue("i", ItemNum);
		}
	default:
		return Py_BuildException();

	}
}

PyObject * playerSetItemCount(PyObject* poSelf, PyObject* poArgs)
{
	switch (PyTuple_Size(poArgs))
	{
	case 2:
		{
			int iSlotIndex;
			if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
				return Py_BuildException();
#ifdef INCREASE_ITEM_STACK
			WORD bCount;
#else
			BYTE bCount;
#endif
			if (!PyTuple_GetInteger(poArgs, 1, &bCount))
				return Py_BuildException();

			if (0 == bCount)
				return Py_BuildException();

			CPythonPlayer::Instance().SetItemCount(TItemPos (INVENTORY, iSlotIndex), bCount);
			return Py_BuildNone();
		}
	case 3:
		{
			TItemPos Cell;
			if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
				return Py_BuildException();

			if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
				return Py_BuildException();

#ifdef INCREASE_ITEM_STACK
			WORD bCount;
#else
			BYTE bCount;
#endif
			if (!PyTuple_GetInteger(poArgs, 2, &bCount))
				return Py_BuildException();

			CPythonPlayer::Instance().SetItemCount(Cell, bCount);

			return Py_BuildNone();
		}
	default:
		return Py_BuildException();

	}
}

PyObject * playerGetItemCountByVnum(PyObject* poSelf, PyObject* poArgs)
{
	int ivnum;
	if (!PyTuple_GetInteger(poArgs, 0, &ivnum))
		return Py_BuildException();

	int ItemNum = CPythonPlayer::Instance().GetItemCountByVnum(ivnum);
	return Py_BuildValue("i", ItemNum);
}

PyObject * playerGetItemCountByVnumRange(PyObject* poSelf, PyObject* poArgs)
{
	int ivnumstart;
	if (!PyTuple_GetInteger(poArgs, 0, &ivnumstart))
		return Py_BuildException();
	int ivnumend;
	if (!PyTuple_GetInteger(poArgs, 1, &ivnumend))
		return Py_BuildException();

	int ItemNum = CPythonPlayer::Instance().GetItemCountByVnumRange(ivnumstart, ivnumend);
	return Py_BuildValue("i", ItemNum);
}

#ifdef ENABLE_ALPHA_EQUIP
PyObject * playerGetItemAlphaEquip(PyObject* poSelf, PyObject* poArgs)
{
	switch (PyTuple_Size(poArgs))
	{
	case 1:
		{
			int iSlotIndex;
			if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
				return Py_BuildException();

			int AlphaEquipVal = CPythonPlayer::Instance().GetItemAlphaEquipValue(TItemPos(INVENTORY, iSlotIndex));
			return Py_BuildValue("i", AlphaEquipVal);
		}
	case 2:
		{
			TItemPos Cell;
			if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
				return Py_BuildException();

			if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
				return Py_BuildException();

			int AlphaEquipVal = CPythonPlayer::Instance().GetItemAlphaEquipValue(Cell);
			return Py_BuildValue("i", AlphaEquipVal);
		}
	default:
		return Py_BuildException();
	}
}
#endif

PyObject * playerGetItemMetinSocket(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos Cell;
	int iMetinSocketIndex;

	switch (PyTuple_Size(poArgs))
	{
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &Cell.cell))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 1, &iMetinSocketIndex))
			return Py_BuildException();

		break;
	case 3:
		if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 2, &iMetinSocketIndex))
			return Py_BuildException();

		break;

	default:
		return Py_BuildException();

	}
	int nMetinSocketValue = CPythonPlayer::Instance().GetItemMetinSocket(Cell, iMetinSocketIndex);
	return Py_BuildValue("i", nMetinSocketValue);
}

PyObject * playerGetItemAttribute(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos Cell;
	int iSlotPos;
	int iAttributeSlotIndex;
	switch (PyTuple_Size(poArgs))
	{
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &Cell.cell))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 1, &iAttributeSlotIndex))
			return Py_BuildException();

		break;
	case 3:
		if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 2, &iAttributeSlotIndex))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}
	BYTE byType;
	short sValue;
	CPythonPlayer::Instance().GetItemAttribute(Cell, iAttributeSlotIndex, &byType, &sValue);

	return Py_BuildValue("ii", byType, sValue);
}

void BuildItemLinkFromData(char* szBuf, int iBufSize, DWORD dwVnum, DWORD dwFlags,
	const google::protobuf::RepeatedField<google::protobuf::int32>& alSockets,
	const google::protobuf::RepeatedPtrField<network::TItemAttribute>& aAttr)
{
	CItemData* pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwVnum, &pItemData))
	{
		*szBuf = 0;
		return;
	}

	char szTempBuf[512];

	bool isAttr = false;
	int len = snprintf(szTempBuf, sizeof(szTempBuf), "item:%x:%x:%x:%x:%x",
		dwVnum, dwFlags, alSockets[0], alSockets[1], alSockets[2]);

	for (int i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
		//if (aAttr[i].bType != 0) // allow empty bonus slots to be sent via hyperlink because they will be sorted in uitooltip.py
		{
			len += snprintf(szTempBuf + len, sizeof(szTempBuf) - len, ":%x:%d",
				aAttr[i].type(), aAttr[i].value());
			isAttr = true;
		}

	const char* szItemName = pItemData->GetName();

	if (dwVnum == 50300 && alSockets[0])
	{
		CPythonSkill::SSkillData* c_pSkillData;
		if (CPythonSkill::Instance().GetSkillData(alSockets[0], &c_pSkillData))
			szItemName = c_pSkillData->strName.c_str();
	}

	if (GetDefaultCodePage() == CP_ARABIC) {
		if (isAttr)
			//"item:번호:플래그:소켓0:소켓1:소켓2"
			snprintf(szBuf, iBufSize, " |h|r[%s]|cffffc700|H%s|h", szItemName, szTempBuf);
		else
			snprintf(szBuf, iBufSize, " |h|r[%s]|cfff1e6c0|H%s|h", szItemName, szTempBuf);
	}
	else {
		if (isAttr)
			//"item:번호:플래그:소켓0:소켓1:소켓2"
			snprintf(szBuf, iBufSize, "|cffffc700|H%s|h[%s]|h|r", szTempBuf, szItemName);
		else
			snprintf(szBuf, iBufSize, "|cfff1e6c0|H%s|h[%s]|h|r", szTempBuf, szItemName);
	}
}

PyObject * playerGetItemLink(PyObject * poSelf, PyObject * poArgs)
{
	TItemPos Cell;

	switch (PyTuple_Size(poArgs))
	{
	case 1:	
		if (!PyTuple_GetInteger(poArgs, 0, &Cell.cell))
			return Py_BuildException();
		break;
	case 2:
		if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}
	const network::TItemData * pPlayerItem = CPythonPlayer::Instance().GetItemData(Cell);
	CItemData * pItemData = NULL;
	CItemManager::Instance().GetItemDataPointer(pPlayerItem->vnum(), &pItemData);
	char buf[1024];
	BuildItemLinkFromData(buf, sizeof(buf), pPlayerItem->vnum(), pItemData ? pItemData->GetFlags() : 0, pPlayerItem->sockets(), pPlayerItem->attributes());

	return Py_BuildValue("s", buf);
}

PyObject * playerGetItemLinkFromData(PyObject * poSelf, PyObject * poArgs)
{
	bool bHasSockets = true;
	bool bHasAttr = true;

	int iVnum;
	if (!PyTuple_GetInteger(poArgs, 0, &iVnum))
		return Py_BadArgument();
	PyObject* poSockets;
	if (!PyTuple_GetObject(poArgs, 1, &poSockets))
		bHasSockets = false;
	PyObject* poAttributes;
	if (!PyTuple_GetObject(poArgs, 2, &poAttributes))
		bHasAttr = false;

	google::protobuf::RepeatedField<google::protobuf::int32> alSockets;
	for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
	{
		alSockets.Add();

		if (bHasSockets)
		{
			alSockets[i] = PyInt_AsLong(PyTuple_GetItem(poSockets, i));
		}
	}

	google::protobuf::RepeatedPtrField<network::TItemAttribute> aAttr;
	for (int i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
	{
		aAttr.Add();

		if (bHasAttr)
		{
			aAttr[i].set_type(PyInt_AsLong(PyTuple_GetItem(poAttributes, i * 2 + 0)));
			aAttr[i].set_value(PyInt_AsLong(PyTuple_GetItem(poAttributes, i * 2 + 1)));
		}
	}

	char buf[1024];
	BuildItemLinkFromData(buf, sizeof(buf), iVnum, 0, alSockets, aAttr);

	return Py_BuildValue("s", buf);
}

PyObject * playerGetISellItemPrice(PyObject * poSelf, PyObject * poArgs)
{
	TItemPos Cell;

	switch (PyTuple_Size(poArgs))
	{
	case 1:	
		if (!PyTuple_GetInteger(poArgs, 0, &Cell.cell))
			return Py_BuildException();
		break;
	case 2:
		if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}
	CItemData * pItemData;

	if (!CItemManager::Instance().GetItemDataPointer(CPythonPlayer::Instance().GetItemIndex(Cell), &pItemData))
		return Py_BuildValue("i", 0);

	int iPrice;

	if (pItemData->IsFlag(CItemData::ITEM_FLAG_COUNT_PER_1GOLD))
		iPrice = CPythonPlayer::Instance().GetItemCount(Cell) / pItemData->GetISellItemPrice();
	else
		iPrice = pItemData->GetISellItemPrice() * CPythonPlayer::Instance().GetItemCount(Cell);

	iPrice /= 5;
	return Py_BuildValue("i", iPrice);
}

PyObject * playerIsGMOwner(PyObject * poSelf, PyObject * poArgs)
{
	TItemPos Cell;

	switch (PyTuple_Size(poArgs))
	{
	case 1:
		if (!PyTuple_GetInteger(poArgs, 0, &Cell.cell))
			return Py_BuildException();
		break;
	case 2:
		if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}

	auto item =  CPythonPlayer::Instance().GetItemData(Cell);

	if (!item)
		return Py_BuildValue("O", Py_False);

	return Py_BuildValue("O", item->is_gm_owner() ? Py_True : Py_False);
}

PyObject * playerGetIBuyItemPrice(PyObject * poSelf, PyObject * poArgs)
{
	TItemPos Cell;

	switch (PyTuple_Size(poArgs))
	{
	case 1:
		if (!PyTuple_GetInteger(poArgs, 0, &Cell.cell))
			return Py_BuildException();
		break;
	case 2:
		if (!PyTuple_GetByte(poArgs, 0, &Cell.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &Cell.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}
	CItemData * pItemData;

	if (!CItemManager::Instance().GetItemDataPointer(CPythonPlayer::Instance().GetItemIndex(Cell), &pItemData))
		return Py_BuildValue("i", 0);

	int iPrice;

	if (pItemData->IsFlag(CItemData::ITEM_FLAG_COUNT_PER_1GOLD))
		iPrice = CPythonPlayer::Instance().GetItemCount(Cell) / pItemData->GetIBuyItemPrice();
	else
		iPrice = pItemData->GetIBuyItemPrice() * CPythonPlayer::Instance().GetItemCount(Cell);

	return Py_BuildValue("i", iPrice);
}

PyObject * playerGetQuickPage(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetQuickPage());
}

PyObject * playerSetQuickPage(PyObject* poSelf, PyObject* poArgs)
{
	int iPageIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iPageIndex))
		return Py_BuildException();

	CPythonPlayer::Instance().SetQuickPage(iPageIndex);
	return Py_BuildNone();
}

PyObject * playerLocalQuickSlotIndexToGlobalQuickSlotIndex(PyObject* poSelf, PyObject* poArgs)
{
	int iLocalSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iLocalSlotIndex))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	return Py_BuildValue("i", rkPlayer.LocalQuickSlotIndexToGlobalQuickSlotIndex(iLocalSlotIndex));
}


PyObject * playerGetLocalQuickSlot(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	DWORD dwWndType;
	DWORD dwWndItemPos;

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.GetLocalQuickSlotData(iSlotIndex, &dwWndType, &dwWndItemPos);

	return Py_BuildValue("ii", dwWndType, dwWndItemPos);
}

PyObject * playerGetGlobalQuickSlot(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	DWORD dwWndType;
	DWORD dwWndItemPos;

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.GetGlobalQuickSlotData(iSlotIndex, &dwWndType, &dwWndItemPos);

	return Py_BuildValue("ii", dwWndType, dwWndItemPos);
}


PyObject * playerRequestAddLocalQuickSlot(PyObject * poSelf, PyObject * poArgs)
{
	int nSlotIndex;
	int nWndType;
	int nWndItemPos;
	
	if (!PyTuple_GetInteger(poArgs, 0, &nSlotIndex))
		return Py_BuildException();

	if (!PyTuple_GetInteger(poArgs, 1, &nWndType))
		return Py_BuildException();

	if (!PyTuple_GetInteger(poArgs, 2, &nWndItemPos))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.RequestAddLocalQuickSlot(nSlotIndex, nWndType, nWndItemPos);

	return Py_BuildNone();
}

PyObject * playerRequestAddToEmptyLocalQuickSlot(PyObject* poSelf, PyObject* poArgs)
{
	int nWndType;
	if (!PyTuple_GetInteger(poArgs, 0, &nWndType))
		return Py_BuildException();

	int nWndItemPos;
	if (!PyTuple_GetInteger(poArgs, 1, &nWndItemPos))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.RequestAddToEmptyLocalQuickSlot(nWndType, nWndItemPos);

	return Py_BuildNone();
}

PyObject * playerRequestDeleteGlobalQuickSlot(PyObject * poSelf, PyObject * poArgs)
{
	int nGlobalSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &nGlobalSlotIndex))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.RequestDeleteGlobalQuickSlot(nGlobalSlotIndex);	
	return Py_BuildNone();
}

PyObject * playerRequestMoveGlobalQuickSlotToLocalQuickSlot(PyObject * poSelf, PyObject * poArgs)
{
	int nGlobalSrcSlotIndex;
	int nLocalDstSlotIndex;

	if (!PyTuple_GetInteger(poArgs, 0, &nGlobalSrcSlotIndex))
		return Py_BuildException();

	if (!PyTuple_GetInteger(poArgs, 1, &nLocalDstSlotIndex))
		return Py_BuildException();

	CPythonPlayer& rkPlayer=CPythonPlayer::Instance();
	rkPlayer.RequestMoveGlobalQuickSlotToLocalQuickSlot(nGlobalSrcSlotIndex, nLocalDstSlotIndex);
	return Py_BuildNone();
}

PyObject * playerRequestUseLocalQuickSlot(PyObject* poSelf, PyObject* poArgs)
{
	int iLocalPosition;
	if (!PyTuple_GetInteger(poArgs, 0, &iLocalPosition))
		return Py_BuildException();

	CPythonPlayer::Instance().RequestUseLocalQuickSlot(iLocalPosition);

	return Py_BuildNone();
}

PyObject * playerRemoveQuickSlotByValue(PyObject* poSelf, PyObject* poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BuildException();
	
	int iPosition;
	if (!PyTuple_GetInteger(poArgs, 1, &iPosition))
		return Py_BuildException();

	CPythonPlayer::Instance().RemoveQuickSlotByValue(iType, iPosition);

	return Py_BuildNone();
}

PyObject * playerisItem(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	char Flag = CPythonPlayer::Instance().IsItem(TItemPos(INVENTORY, iSlotIndex));

	return Py_BuildValue("i", Flag);
}

PyObject * playerIsEquipmentSlot(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	if (iSlotIndex >= c_Equipment_Start)
	if (iSlotIndex <= c_Inventory_Count)
		return Py_BuildValue("i", 1);

	return Py_BuildValue("i", 0);
}

#ifdef ENABLE_DRAGONSOUL
PyObject * playerIsDSEquipmentSlot(PyObject* poSelf, PyObject* poArgs)
{
	BYTE bWindowType;
	if (!PyTuple_GetInteger(poArgs, 0, &bWindowType))
		return Py_BuildException();
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSlotIndex))
		return Py_BuildException();

	if (INVENTORY == bWindowType)
		if (iSlotIndex >= DRAGON_SOUL_EQUIP_SLOT_START)
			if (iSlotIndex <= DRAGON_SOUL_EQUIP_SLOT_END)
				return Py_BuildValue("i", 1);

	return Py_BuildValue("i", 0);
}
#endif

PyObject * playerIsCostumeSlot(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

#ifdef ENABLE_COSTUME_SYSTEM
	if (iSlotIndex >= c_Costume_Slot_Start)
	if (iSlotIndex <= c_Costume_Slot_End)
		return Py_BuildValue("i", 1);
#endif

	return Py_BuildValue("i", 0);
}

PyObject * playerIsOpenPrivateShop(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().IsOpenPrivateShop());
}

PyObject * playerIsValuableItem(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos SlotIndex;

	switch (PyTuple_Size (poArgs))
	{
	case 1:
		if (!PyTuple_GetInteger(poArgs, 0, &SlotIndex.cell))
			return Py_BuildException();
		break;
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &SlotIndex.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &SlotIndex.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}

	DWORD dwItemIndex = CPythonPlayer::Instance().GetItemIndex(SlotIndex);
	CItemManager::Instance().SelectItemData(dwItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find item data");

	BOOL hasMetinSocket = FALSE;
	BOOL isHighPrice = FALSE;

	for (int i = 0; i < METIN_SOCKET_COUNT; ++i)
		if (CPythonPlayer::METIN_SOCKET_TYPE_NONE != CPythonPlayer::Instance().GetItemMetinSocket(SlotIndex, i))
			hasMetinSocket = TRUE;

	DWORD dwValue = pItemData->GetISellItemPrice();
	if (dwValue > 5000)
		isHighPrice = TRUE;

	return Py_BuildValue("i", hasMetinSocket || isHighPrice);
}

int GetItemGrade(const char * c_szItemName)
{
	std::string strName = c_szItemName;
	if (strName.empty())
		return 0;

	char chGrade = strName[strName.length() - 1];
	if (chGrade < '0' || chGrade > '9')
		chGrade = '0';

	int iGrade = chGrade - '0';
	return iGrade;
}

PyObject * playerGetItemGrade(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos SlotIndex;
	switch (PyTuple_Size(poArgs))
	{
	case 1:
		if (!PyTuple_GetInteger(poArgs, 0, &SlotIndex.cell))
			return Py_BuildException();
		break;
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &SlotIndex.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &SlotIndex.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}

	int iItemIndex = CPythonPlayer::Instance().GetItemIndex(SlotIndex);
	CItemManager::Instance().SelectItemData(iItemIndex);
	CItemData * pItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pItemData)
		return Py_BuildException("Can't find item data");

	return Py_BuildValue("i", GetItemGrade(pItemData->GetName()));
}

#if defined(GAIDEN)
PyObject * playerGetItemUnbindTime(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	return Py_BuildValue("i", (int) CPythonPlayer::instance().GetItemUnbindTime(iSlotIndex));
}
#endif

enum
{
	REFINE_SCROLL_TYPE_MAKE_SOCKET = 1,
	REFINE_SCROLL_TYPE_UP_GRADE = 2,
};

enum
{
	REFINE_CANT,
	REFINE_OK,
	REFINE_ALREADY_MAX_SOCKET_COUNT,
	REFINE_NEED_MORE_GOOD_SCROLL,
	REFINE_CANT_MAKE_SOCKET_ITEM,
	REFINE_NOT_NEXT_GRADE_ITEM,
	REFINE_CANT_REFINE_METIN_TO_EQUIPMENT,
	REFINE_CANT_REFINE_ROD,
};

PyObject * playerCanRefine(PyObject * poSelf, PyObject * poArgs)
{
	int iScrollItemIndex;
	TItemPos TargetSlotIndex;

	switch (PyTuple_Size(poArgs))
	{
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &iScrollItemIndex))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.cell))
			return Py_BadArgument();
		break;
	case 3:
		if (!PyTuple_GetInteger(poArgs, 0, &iScrollItemIndex))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.window_type))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 2, &TargetSlotIndex.cell))
			return Py_BadArgument();
		break;
	default:
		return Py_BadArgument();
	}

	if (CPythonPlayer::Instance().IsEquipmentSlot(TargetSlotIndex))
	{
		return Py_BuildValue("i", REFINE_CANT_REFINE_METIN_TO_EQUIPMENT);
	}

	// Scroll
	CItemManager::Instance().SelectItemData(iScrollItemIndex);
	CItemData * pScrollItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pScrollItemData)
		return Py_BuildValue("i", REFINE_CANT);
	int iScrollType = pScrollItemData->GetType();
	int iScrollSubType = pScrollItemData->GetSubType();
	if (iScrollType != CItemData::ITEM_TYPE_USE)
		return Py_BuildValue("i", REFINE_CANT);
	if (iScrollSubType != CItemData::USE_TUNING)
		return Py_BuildValue("i", REFINE_CANT);

	// Target Item
	int iTargetItemIndex = CPythonPlayer::Instance().GetItemIndex(TargetSlotIndex);
	CItemManager::Instance().SelectItemData(iTargetItemIndex);
	CItemData * pTargetItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pTargetItemData)
		return Py_BuildValue("i", REFINE_CANT);
	int iTargetType = pTargetItemData->GetType();
	//int iTargetSubType = pTargetItemData->GetSubType();
	if (CItemData::ITEM_TYPE_ROD == iTargetType)
		return Py_BuildValue("i", REFINE_CANT_REFINE_ROD);

	if (pTargetItemData->HasNextGrade())
	{
		return Py_BuildValue("i", REFINE_OK);
	}
	else
	{
		return Py_BuildValue("i", REFINE_NOT_NEXT_GRADE_ITEM);
	}

	return Py_BuildValue("i", REFINE_CANT);
}

enum
{
	ATTACH_METIN_CANT,
	ATTACH_METIN_OK,
	ATTACH_METIN_NOT_MATCHABLE_ITEM,
	ATTACH_METIN_NO_MATCHABLE_SOCKET,
	ATTACH_METIN_NOT_EXIST_GOLD_SOCKET,
	ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT,
};

PyObject * playerCanAttachMetin(PyObject* poSelf, PyObject* poArgs)
{
	int iMetinItemID;
	TItemPos TargetSlotIndex;

	switch (PyTuple_Size(poArgs))
	{
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &iMetinItemID))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.cell))
			return Py_BuildException();
		break;
	case 3:
		if (!PyTuple_GetInteger(poArgs, 0, &iMetinItemID))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 2, &TargetSlotIndex.cell))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}
	if (CPythonPlayer::Instance().IsEquipmentSlot(TargetSlotIndex))
	{
		return Py_BuildValue("i", ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT);
	}

	CItemData * pMetinItemData;
	if (!CItemManager::Instance().GetItemDataPointer(iMetinItemID, &pMetinItemData))
		return Py_BuildException("can't find item data");

	DWORD dwTargetItemIndex = CPythonPlayer::Instance().GetItemIndex(TargetSlotIndex);
	CItemData * pTargetItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwTargetItemIndex, &pTargetItemData))
		return Py_BuildException("can't find item data");

	DWORD dwMetinWearFlags = pMetinItemData->GetWearFlags();
	DWORD dwTargetWearFlags = pTargetItemData->GetWearFlags();

#ifdef PROMETA
	if (pTargetItemData->GetType() == CItemData::ITEM_TYPE_COSTUME && pTargetItemData->GetSubType() == CItemData::COSTUME_ACCE)
	{
		if (pMetinItemData->GetSubType() != CItemData::METIN_ACCE)
			return Py_BuildValue("i", ATTACH_METIN_NOT_MATCHABLE_ITEM);
	}
	else
#endif
	{
		if (0 == (dwMetinWearFlags & dwTargetWearFlags))
			return Py_BuildValue("i", ATTACH_METIN_NOT_MATCHABLE_ITEM);
	}

	if (CItemData::ITEM_TYPE_ROD == pTargetItemData->GetType())
		return Py_BuildValue("i", ATTACH_METIN_CANT);

	BOOL bNotExistGoldSocket = FALSE;

	int iSubType = pMetinItemData->GetSubType();
	for (int i = 0; i < ITEM_SOCKET_SLOT_MAX_NUM; ++i)
	{
		DWORD dwSocketType = CPythonPlayer::Instance().GetItemMetinSocket(TargetSlotIndex, i);
		if (CItemData::METIN_NORMAL == iSubType)
		{
			if (CPythonPlayer::METIN_SOCKET_TYPE_SILVER == dwSocketType ||
				CPythonPlayer::METIN_SOCKET_TYPE_GOLD == dwSocketType)
			{
				return Py_BuildValue("i", ATTACH_METIN_OK);
			}
		}
		else if (CItemData::METIN_GOLD == iSubType)
		{
			if (CPythonPlayer::METIN_SOCKET_TYPE_GOLD == dwSocketType)
			{
				return Py_BuildValue("i", ATTACH_METIN_OK);
			}
			else if (CPythonPlayer::METIN_SOCKET_TYPE_SILVER == dwSocketType)
			{
				bNotExistGoldSocket = TRUE;
			}
		}
#ifdef PROMETA
		else if (CItemData::METIN_ACCE == iSubType)
		{
			if (pTargetItemData->GetType() == CItemData::ITEM_TYPE_COSTUME && pTargetItemData->GetSubType() == CItemData::COSTUME_ACCE)
				return Py_BuildValue("i", ATTACH_METIN_OK);
		}
#endif
	}

	if (bNotExistGoldSocket)
	{
		return Py_BuildValue("i", ATTACH_METIN_NOT_EXIST_GOLD_SOCKET);
	}

	return Py_BuildValue("i", ATTACH_METIN_NO_MATCHABLE_SOCKET);
}

enum
{
	DETACH_METIN_CANT,
	DETACH_METIN_OK,
};

PyObject * playerCanDetach(PyObject * poSelf, PyObject * poArgs)
{
	int iScrollItemIndex;
	TItemPos TargetSlotIndex;
	switch (PyTuple_Size (poArgs))
	{
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &iScrollItemIndex))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.cell))
			return Py_BadArgument();
		break;
	case 3:
		if (!PyTuple_GetInteger(poArgs, 0, &iScrollItemIndex))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.window_type))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 2, &TargetSlotIndex.cell))
			return Py_BadArgument();
		break;
	default:
		return Py_BadArgument();
	}

	// Scroll
	CItemManager::Instance().SelectItemData(iScrollItemIndex);
	CItemData * pScrollItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pScrollItemData)
		return Py_BuildException("Can't find item data");
	int iScrollType = pScrollItemData->GetType();
	int iScrollSubType = pScrollItemData->GetSubType();
	if (iScrollType != CItemData::ITEM_TYPE_USE)
		return Py_BuildValue("i", DETACH_METIN_CANT);
	if (iScrollSubType != CItemData::USE_DETACHMENT && iScrollSubType != CItemData::USE_DETACH_STONE && iScrollSubType != CItemData::USE_DETACH_ATTR)
		return Py_BuildValue("i", DETACH_METIN_CANT);

	// Target Item
	int iTargetItemIndex = CPythonPlayer::Instance().GetItemIndex(TargetSlotIndex);
	CItemManager::Instance().SelectItemData(iTargetItemIndex);
	CItemData * pTargetItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pTargetItemData)
		return Py_BuildException("Can't find item data");
	//int iTargetType = pTargetItemData->GetType();
	//int iTargetSubType = pTargetItemData->GetSubType();

	if (pTargetItemData->IsFlag(CItemData::ITEM_FLAG_REFINEABLE) && iScrollSubType == CItemData::USE_DETACHMENT)
	{
		for (int iSlotCount = 0; iSlotCount < METIN_SOCKET_COUNT; ++iSlotCount)
			if (CPythonPlayer::Instance().GetItemMetinSocket(TargetSlotIndex, iSlotCount) > 2)
			{
				return Py_BuildValue("i", DETACH_METIN_OK);
			}
	}
	else if (pTargetItemData->IsFlag(CItemData::ITEM_FLAG_REFINEABLE) && iScrollSubType == CItemData::USE_DETACH_STONE)
	{
		for (int iSlotCount = 0; iSlotCount < METIN_SOCKET_COUNT; ++iSlotCount)
		{
			DWORD stoneVnum = CPythonPlayer::Instance().GetItemMetinSocket(TargetSlotIndex, iSlotCount);
			if (stoneVnum > 2 && stoneVnum != CItemData::ITEM_BROKEN_METIN_VNUM)
				return Py_BuildValue("i", DETACH_METIN_OK);
		}
	}
	else if (iScrollSubType == CItemData::USE_DETACH_ATTR && pTargetItemData->GetType() == CItemData::ITEM_TYPE_COSTUME)
	{
		BYTE byType;
		short sValue;

		for (int iSlotCount = 0; iSlotCount < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++iSlotCount)
		{
			CPythonPlayer::Instance().GetItemAttribute(TargetSlotIndex, iSlotCount, &byType, &sValue);
			if (byType > 0)
				return Py_BuildValue("i", DETACH_METIN_OK);
		}
	}

	return Py_BuildValue("i", DETACH_METIN_CANT);
}

PyObject * playerCanUnlock(PyObject * poSelf, PyObject * poArgs)
{
	int iKeyItemIndex;
	TItemPos TargetSlotIndex;
	switch (PyTuple_Size(poArgs))
	{
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &iKeyItemIndex))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.cell))
			return Py_BadArgument();
		break;
	case 3:
		if (!PyTuple_GetInteger(poArgs, 0, &iKeyItemIndex))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 1, &TargetSlotIndex.window_type))
			return Py_BadArgument();
		if (!PyTuple_GetInteger(poArgs, 2, &TargetSlotIndex.cell))
			return Py_BadArgument();
		break;
	default:
		return Py_BadArgument();
	}

	// Key
	CItemManager::Instance().SelectItemData(iKeyItemIndex);
	CItemData * pKeyItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pKeyItemData)
		return Py_BuildException("Can't find item data");
	int iKeyType = pKeyItemData->GetType();
	if (iKeyType != CItemData::ITEM_TYPE_TREASURE_KEY)
		return Py_BuildValue("i", FALSE);

	// Target Item
	int iTargetItemIndex = CPythonPlayer::Instance().GetItemIndex(TargetSlotIndex);
	CItemManager::Instance().SelectItemData(iTargetItemIndex);
	CItemData * pTargetItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pTargetItemData)
		return Py_BuildException("Can't find item data");
	int iTargetType = pTargetItemData->GetType();
	if (iTargetType != CItemData::ITEM_TYPE_TREASURE_BOX)
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", TRUE);
}

PyObject * playerIsRefineGradeScroll(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos ScrollSlotIndex;
	switch (PyTuple_Size(poArgs))
	{
	case 1:
		if (!PyTuple_GetInteger(poArgs, 0, &ScrollSlotIndex.cell))
			return Py_BuildException();
	case 2:
		if (!PyTuple_GetInteger(poArgs, 0, &ScrollSlotIndex.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &ScrollSlotIndex.cell))
			return Py_BuildException();
	default:
		return Py_BuildException();
	}

	int iScrollItemIndex = CPythonPlayer::Instance().GetItemIndex(ScrollSlotIndex);
	CItemManager::Instance().SelectItemData(iScrollItemIndex);
	CItemData * pScrollItemData = CItemManager::Instance().GetSelectedItemDataPointer();
	if (!pScrollItemData)
		return Py_BuildException("Can't find item data");

	return Py_BuildValue("i", REFINE_SCROLL_TYPE_UP_GRADE == pScrollItemData->GetValue(0));
}

PyObject * playerUpdate(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().Update();
	return Py_BuildNone();
}

PyObject * playerRender(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildNone();
}

PyObject * playerClear(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().Clear();
	return Py_BuildNone();
}

PyObject * playerClearTarget(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().SetTarget(0);
	return Py_BuildNone();
}

PyObject * playerSetTarget(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CPythonPlayer::Instance().SetTarget(iVID);
	return Py_BuildNone();
}

PyObject * playerOpenCharacterMenu(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	CPythonPlayer::Instance().OpenCharacterMenu(iVID);
	return Py_BuildNone();
}

PyObject * playerIsPartyMember(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().IsPartyMemberByVID(iVID));
}

PyObject * playerIsPartyLeader(PyObject* poSelf, PyObject* poArgs)
{
	int iVID;
	if (!PyTuple_GetInteger(poArgs, 0, &iVID))
		return Py_BuildException();

	DWORD dwPID;
	if (!CPythonPlayer::Instance().PartyMemberVIDToPID(iVID, &dwPID))
		return Py_BuildValue("i", FALSE);

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(dwPID, &pPartyMemberInfo))
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("b", pPartyMemberInfo->bLeader);
}

PyObject * playerIsPartyLeaderByPID(PyObject* poSelf, PyObject* poArgs)
{
	int iPID;
	if (!PyTuple_GetInteger(poArgs, 0, &iPID))
		return Py_BuildException();

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("b", pPartyMemberInfo->bLeader);
}

PyObject * playerGetPartyMemberHPPercentage(PyObject* poSelf, PyObject* poArgs)
{
	int iPID;
	if (!PyTuple_GetInteger(poArgs, 0, &iPID))
		return Py_BuildException();

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", pPartyMemberInfo->byHPPercentage);
}

PyObject * playerGetPartyMemberState(PyObject* poSelf, PyObject* poArgs)
{
	int iPID;
	if (!PyTuple_GetInteger(poArgs, 0, &iPID))
		return Py_BuildException();

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", pPartyMemberInfo->byState);
}

PyObject * playerGetPartyMemberAffects(PyObject* poSelf, PyObject* poArgs)
{
	int iPID;
	if (!PyTuple_GetInteger(poArgs, 0, &iPID))
		return Py_BuildException();

	CPythonPlayer::TPartyMemberInfo * pPartyMemberInfo;
	if (!CPythonPlayer::Instance().GetPartyMemberPtr(iPID, &pPartyMemberInfo))
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("iiiiiii",	pPartyMemberInfo->sAffects[0],
									pPartyMemberInfo->sAffects[1],
									pPartyMemberInfo->sAffects[2],
									pPartyMemberInfo->sAffects[3],
									pPartyMemberInfo->sAffects[4],
									pPartyMemberInfo->sAffects[5],
									pPartyMemberInfo->sAffects[6]);
}

PyObject * playerRemovePartyMember(PyObject* poSelf, PyObject* poArgs)
{
	int iPID;
	if (!PyTuple_GetInteger(poArgs, 0, &iPID))
		return Py_BuildException();

	CPythonPlayer::Instance().RemovePartyMember(iPID);
	return Py_BuildNone();
}

PyObject * playerExitParty(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().ExitParty();
	return Py_BuildNone();
}

PyObject * playerGetPKMode(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetPKMode());
}

PyObject * playerHasMobilePhoneNumber(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().HasMobilePhoneNumber());
}

PyObject * playerSetWeaponAttackBonusFlag(PyObject* poSelf, PyObject* poArgs)
{
	int iFlag;
	if (!PyTuple_GetInteger(poArgs, 0, &iFlag))
		return Py_BuildException();

	return Py_BuildNone();
}

PyObject * playerToggleCoolTime(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().__ToggleCoolTime());
}

PyObject * playerToggleLevelLimit(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().__ToggleLevelLimit());
}

PyObject * playerGetTargetVID(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetTargetVID());
}

PyObject * playerRegisterEmotionIcon(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();
	char * szFileName;
	if (!PyTuple_GetString(poArgs, 1, &szFileName))
		return Py_BuildException();

	CGraphicImage * pImage = (CGraphicImage *)CResourceManager::Instance().GetResourcePointer(szFileName);
	m_kMap_iEmotionIndex_pkIconImage.insert(make_pair(iIndex, pImage));

	return Py_BuildNone();
}

PyObject * playerGetEmotionIconImage(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	if (m_kMap_iEmotionIndex_pkIconImage.end() == m_kMap_iEmotionIndex_pkIconImage.find(iIndex))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", m_kMap_iEmotionIndex_pkIconImage[iIndex]);
}

PyObject * playerSetItemData(PyObject* poSelf, PyObject* poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	int iVirtualID;
	if (!PyTuple_GetInteger(poArgs, 1, &iVirtualID))
		return Py_BuildException();

	int iNum;
	if (!PyTuple_GetInteger(poArgs, 2, &iNum))
		return Py_BuildException();

	network::TItemData kItemInst;
	kItemInst.set_vnum(iVirtualID);
	kItemInst.set_count(iNum);
	CPythonPlayer::Instance().SetItemData(TItemPos(INVENTORY, iSlotIndex), kItemInst);
	return Py_BuildNone();
}

PyObject * playerSetItemMetinSocket(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos ItemPos;
	int iMetinSocketNumber;
	int iNum;

	switch (PyTuple_Size(poArgs))
	{
	case 3:
		if (!PyTuple_GetInteger(poArgs, 0, &ItemPos.cell))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 1, &iMetinSocketNumber))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 2, &iNum))
			return Py_BuildException();

		break;
	case 4:
		if (!PyTuple_GetInteger(poArgs, 0, &ItemPos.window_type))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 1, &ItemPos.cell))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 2, &iMetinSocketNumber))
			return Py_BuildException();
		if (!PyTuple_GetInteger(poArgs, 3, &iNum))
			return Py_BuildException();

		break;
	default:
		return Py_BuildException();
	}

	CPythonPlayer::Instance().SetItemMetinSocket(ItemPos, iMetinSocketNumber, iNum);
	return Py_BuildNone();
}

PyObject * playerSetItemAttribute(PyObject* poSelf, PyObject* poArgs)
{
	TItemPos ItemPos;
	int iAttributeSlotIndex;
	int iAttributeType;
	int iAttributeValue;

	switch (PyTuple_Size(poArgs))
	{
	case 4:
		if (!PyTuple_GetInteger(poArgs, 0, &ItemPos.cell))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 1, &iAttributeSlotIndex))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 2, &iAttributeType))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 3, &iAttributeValue))
			return Py_BuildException();
		break;
	case 5:
		if (!PyTuple_GetInteger(poArgs, 0, &ItemPos.window_type))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 1, &ItemPos.cell))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 2, &iAttributeSlotIndex))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 3, &iAttributeType))
			return Py_BuildException();

		if (!PyTuple_GetInteger(poArgs, 4, &iAttributeValue))
			return Py_BuildException();
		break;
	default:
		return Py_BuildException();
	}
	CPythonPlayer::Instance().SetItemAttribute(ItemPos, iAttributeSlotIndex, iAttributeType, iAttributeValue);
	return Py_BuildNone();
}

PyObject * playerSetAutoPotionInfo(PyObject* poSelf, PyObject* poArgs)
{
	int potionType = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &potionType))
		return Py_BadArgument();

	CPythonPlayer* player = CPythonPlayer::InstancePtr();

	CPythonPlayer::SAutoPotionInfo& potionInfo = player->GetAutoPotionInfo(potionType);

	if (!PyTuple_GetBoolean(poArgs, 1, &potionInfo.bActivated))
		return Py_BadArgument();

	if (!PyTuple_GetLong(poArgs, 2, &potionInfo.currentAmount))
		return Py_BadArgument();

	if (!PyTuple_GetLong(poArgs, 3, &potionInfo.totalAmount))
		return Py_BadArgument();

	if (!PyTuple_GetLong(poArgs, 4, &potionInfo.inventorySlotIndex))
		return Py_BadArgument();

	return Py_BuildNone();
}

PyObject * playerGetAutoPotionInfo(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer* player = CPythonPlayer::InstancePtr();

	int potionType = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &potionType))
		return Py_BadArgument();

	CPythonPlayer::SAutoPotionInfo& potionInfo = player->GetAutoPotionInfo(potionType);
	
	return Py_BuildValue("biii", potionInfo.bActivated, int(potionInfo.currentAmount), int(potionInfo.totalAmount), int(potionInfo.inventorySlotIndex));
}

PyObject * playerSlotTypeToInvenType(PyObject* poSelf, PyObject* poArgs)
{
	int slotType = 0;
	if (!PyTuple_GetInteger(poArgs, 0, &slotType))
		return Py_BadArgument();

	return Py_BuildValue("i", SlotTypeToInvenType((BYTE)slotType));
}

PyObject * playerIsGameMaster(PyObject* poSelf, PyObject* poArgs)
{
	CInstanceBase* pkInst = CPythonPlayer::Instance().NEW_GetMainActorPtr();
	return Py_BuildValue("b", pkInst && pkInst->IsGameMaster());
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
PyObject * playerGetAcceItemID(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData * pInstance;
	if (!CPythonPlayer::Instance().GetAcceItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pInstance->vnum());
}

PyObject * playerGetAcceItemSize(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetCurrentAcceSize());
}

PyObject * playerGetAcceItemFlags(PyObject * poSelf, PyObject * poArgs)
{
	int ipos;
	if (!PyTuple_GetInteger(poArgs, 0, &ipos))
		return Py_BadArgument();

	network::TItemData* pInstance;
	if (!CPythonPlayer::Instance().GetAcceItemDataPtr(ipos, &pInstance))
		return Py_BuildException();

	CItemData* data;
	if (!CItemManager::Instance().GetItemDataPointer(pInstance->vnum(), &data))
		return Py_BuildException();

	return Py_BuildValue("i", data->GetFlags());
}

PyObject * playerGetAcceItemMetinSocket(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BadArgument();
	int iSocketIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSocketIndex))
		return Py_BadArgument();

	if (iSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM)
		return Py_BuildException();

	network::TItemData* pItemData;
	if (!CPythonPlayer::Instance().GetAcceItemDataPtr(iSlotIndex, &pItemData))
		return Py_BuildException();

	return Py_BuildValue("i", pItemData->sockets(iSocketIndex));
}

PyObject * playerGetAcceItemAttribute(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();
	int iAttrSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttrSlotIndex))
		return Py_BuildException();

	if (iAttrSlotIndex >= 0 && iAttrSlotIndex < ITEM_ATTRIBUTE_SLOT_MAX_NUM)
	{
		network::TItemData* pItemData;
		if (CPythonPlayer::Instance().GetAcceItemDataPtr(iSlotIndex, &pItemData))
			return Py_BuildValue("ii", pItemData->attributes(iAttrSlotIndex).type(), pItemData->attributes(iAttrSlotIndex).value());
	}

	return Py_BuildValue("ii", 0, 0);
}

PyObject * playerIsAcceWindowEmpty(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().IsEmtpyAcceWindow());
}


PyObject * playerGetCurrentItemCount(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetCurrentAcceItemCount());
}

PyObject * playerSetAcceRefineWindowOpen(PyObject * poSelf, PyObject * poArgs)
{
	int windowType;
	if (!PyTuple_GetInteger(poArgs, 0, &windowType))
		return Py_BuildException();

	CPythonPlayer::instance().SetAcceRefineWindowOpen(windowType);

	return Py_BuildNone();
}

PyObject * playerGetAcceRefineWindowOpen(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().m_acceRefineWindowIsOpen);
}

PyObject * playerGetAcceRefineWindowType(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().m_acceRefineWindowType);
}

PyObject * playerFineMoveAcceItemSlot(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildNone();
}


PyObject * playerSetAcceActivedItemSlot(PyObject * poSelf, PyObject * poArgs)
{
	int acceSlot;
	if (!PyTuple_GetInteger(poArgs, 0, &acceSlot))
		return Py_BuildException();

	int itemPos;
	if (!PyTuple_GetInteger(poArgs, 1, &itemPos))
		return Py_BuildException();

	CPythonPlayer::Instance().SetActivedAcceSlot(acceSlot, itemPos);
	return Py_BuildNone();
}


PyObject * playerFindActivedAcceSlot(PyObject * poSelf, PyObject * poArgs)
{
	int acceSlot;
	if (!PyTuple_GetInteger(poArgs, 0, &acceSlot))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().FindActivedSlot(acceSlot));
}


PyObject * playerFindUsingAcceSlot(PyObject * poSelf, PyObject * poArgs)
{
	int acceSlot;
	if (!PyTuple_GetInteger(poArgs, 0, &acceSlot))
		return Py_BuildException();

	return Py_BuildValue("i", CPythonPlayer::Instance().FindUsingSlot(acceSlot));
}


PyObject * playerCanAcceClearItem(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildNone();
}

PyObject * playerSetHideSashes(PyObject* poSelf, PyObject* poArgs)
{
	bool value{ };

	if(!PyTuple_GetBoolean(poArgs, 0, &value))
		return Py_BadArgument();

	CPythonPlayer::Instance().SetHideSashes(value);

	return Py_BuildNone();
}
#endif

PyObject * playerGetInventoryMaxNum(PyObject * poSelf, PyObject * poArgs)
{
	int iInvMaxNum = CPythonPlayer::Instance().GetInventoryMaxNum();
	return Py_BuildValue("i", iInvMaxNum);
}

PyObject * playerGetUppitemInventoryMaxNum(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetUppitemInventoryMaxNum());
}

PyObject * playerGetSkillbookInventoryMaxNum(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetSkillbookInventoryMaxNum());
}

PyObject * playerGetStoneInventoryMaxNum(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetStoneInventoryMaxNum());
}

PyObject * playerGetEnchantInventoryMaxNum(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetEnchantInventoryMaxNum());
}

#ifdef ENABLE_ANIMAL_SYSTEM
PyObject * playerAnimalSelectType(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bType;
	if (!PyTuple_GetByte(poArgs, 0, &bType))
		return Py_BadArgument();

	CPythonPlayer::Instance().SelectAnimalType(bType);
	return Py_BuildNone();
}

PyObject * playerAnimalIsSummoned(PyObject * poSelf, PyObject * poArgs)
{
	bool bIsSummoned = CPythonPlayer::Instance().IsAnimalSummoned();
	return Py_BuildValue("b", bIsSummoned);
}

PyObject * playerAnimalGetName(PyObject * poSelf, PyObject * poArgs)
{
	const char* c_pszName = CPythonPlayer::Instance().GetAnimalName();
	return Py_BuildValue("s", c_pszName);
}

PyObject * playerAnimalGetLevel(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bLevel = CPythonPlayer::Instance().GetAnimalLevel();
	return Py_BuildValue("i", bLevel);
}

PyObject * playerAnimalGetExp(PyObject * poSelf, PyObject * poArgs)
{
	LONGLONG llExp = CPythonPlayer::Instance().GetAnimalEXP();
	return Py_BuildValue("L", llExp);
}

PyObject * playerAnimalGetMaxExp(PyObject * poSelf, PyObject * poArgs)
{
	LONGLONG llMaxExp = CPythonPlayer::Instance().GetAnimalMaxEXP();
	return Py_BuildValue("L", llMaxExp);
}

PyObject * playerAnimalGetStatPoints(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bStatPoints = CPythonPlayer::Instance().GetAnimalStatPoints();
	return Py_BuildValue("i", bStatPoints);
}

PyObject * playerAnimalGetStat(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bStatIndex;
	if (!PyTuple_GetByte(poArgs, 0, &bStatIndex))
		return Py_BadArgument();

	short sStat = CPythonPlayer::Instance().GetAnimalStat(bStatIndex);
	return Py_BuildValue("i", sStat);
}
#endif

PyObject * playerGetWindowBySlot(PyObject * poSelf, PyObject * poArgs)
{
	WORD wSlot;
	if (!PyTuple_GetInteger(poArgs, 0, &wSlot))
		return Py_BadArgument();

	BYTE bWindow = INVENTORY;
#ifdef ENABLE_SKILL_INVENTORY
	if (wSlot >= SKILLBOOK_INV_SLOT_START && wSlot < SKILLBOOK_INV_SLOT_END)
		bWindow = SKILLBOOK_INVENTORY;
#endif
	if (wSlot >= UPPITEM_INV_SLOT_START && wSlot < UPPITEM_INV_SLOT_END)
		bWindow = UPPITEM_INVENTORY;
	if (wSlot >= STONE_INV_SLOT_START && wSlot < STONE_INV_SLOT_END)
		bWindow = STONE_INVENTORY;
	if (wSlot >= ENCHANT_INV_SLOT_START && wSlot < ENCHANT_INV_SLOT_END)
		bWindow = ENCHANT_INVENTORY;
#ifdef ENABLE_COSTUME_INVENTORY
	if(wSlot >= COSTUME_INV_SLOT_START && wSlot < COSTUME_INV_SLOT_END)
		bWindow = COSTUME_INVENTORY;
#endif

	return Py_BuildValue("i", bWindow);
}

#ifdef ENABLE_DRAGONSOUL
PyObject* playerSendDragonSoulRefine(PyObject* poSelf, PyObject* poArgs)
{
	BYTE bSubHeader;
	PyObject* pDic;
	TItemPos RefineItemPoses[DS_REFINE_WINDOW_MAX_NUM];
	if (!PyTuple_GetByte(poArgs, 0, &bSubHeader))
		return Py_BuildException();
	switch (bSubHeader)
	{
	case DS_SUB_HEADER_CLOSE:
		break;
	case DS_SUB_HEADER_DO_UPGRADE:
	case DS_SUB_HEADER_DO_IMPROVEMENT:
	case DS_SUB_HEADER_DO_REFINE:
	{
		if (!PyTuple_GetObject(poArgs, 1, &pDic))
			return Py_BuildException();
		int pos = 0;
		PyObject* key, *value;
		int size = PyDict_Size(pDic);

		while (PyDict_Next(pDic, &pos, &key, &value))
		{
			int i = PyInt_AsLong(key);
			if (i > DS_REFINE_WINDOW_MAX_NUM)
				return Py_BuildException();

			if (!PyTuple_GetByte(value, 0, &RefineItemPoses[i].window_type)
				|| !PyTuple_GetInteger(value, 1, &RefineItemPoses[i].cell))
			{
				return Py_BuildException();
			}
		}
	}
	break;
	}

	CPythonNetworkStream& rns = CPythonNetworkStream::Instance();
	rns.SendDragonSoulRefinePacket(bSubHeader, RefineItemPoses);

	return Py_BuildNone();
}
#endif

PyObject * playerIsShowTeamler(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", CPythonPlayer::Instance().IsShowTeamler());
}

#ifdef ENABLE_FAKEBUFF
PyObject * playerGetFakeBuffSkillLevel(PyObject* poSelf, PyObject* poArgs)
{
	int iSkillVnum;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillVnum))
		return Py_BuildNone();

	return Py_BuildValue("i", CPythonPlayer::Instance().GetFakeBuffSkillLevel(iSkillVnum));
}
#endif

#ifdef ENABLE_ATTRTREE
PyObject * playerGetAttrtreeLevel(PyObject* poSelf, PyObject* poArgs)
{
	BYTE row;
	if (!PyTuple_GetInteger(poArgs, 0, &row))
		return Py_BadArgument();
	BYTE col;
	if (!PyTuple_GetInteger(poArgs, 1, &col))
		return Py_BadArgument();

	BYTE level = CPythonPlayer::Instance().GetAttrtreeLevel(row, col);
	return Py_BuildValue("i", level);
}

PyObject * playerAttrtreeCellToID(PyObject* poSelf, PyObject* poArgs)
{
	BYTE row;
	if (!PyTuple_GetInteger(poArgs, 0, &row))
		return Py_BadArgument();
	BYTE col;
	if (!PyTuple_GetInteger(poArgs, 1, &col))
		return Py_BadArgument();

	BYTE id = CPythonPlayer::Instance().AttrtreeCellToID(row, col);
	return Py_BuildValue("i", id);
}
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
PyObject * playerSetCostumeBonusTransferWindowOpen(PyObject* poSelf, PyObject* poArgs)
{
	CPythonPlayer::Instance().SetCostumeBonusTransferWindowOpen();
	return Py_BuildNone();
}

PyObject * playerGetCostumeBonusTransferWindowOpen(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", CPythonPlayer::Instance().GetCostumeBonusTransferWindowOpen());
}

PyObject * playerGetCostumeBonusTransferItemID(PyObject* poSelf, PyObject* poArgs)
{
	BYTE byCBTPos;
	if (!PyTuple_GetInteger(poArgs, 0, &byCBTPos))
		return Py_BadArgument();

	network::TItemData * pCBTItemInstance;
	if (!CPythonPlayer::Instance().GetCostumeBonusTransferItemDataPtr(byCBTPos, &pCBTItemInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pCBTItemInstance->vnum());
}

PyObject * playerGetCostumeBonusTransferItemCount(PyObject* poSelf, PyObject* poArgs)
{
	BYTE byCBTPos;
	if (!PyTuple_GetInteger(poArgs, 0, &byCBTPos))
		return Py_BadArgument();

	network::TItemData* pCBTItemInstance;
	if (!CPythonPlayer::Instance().GetCostumeBonusTransferItemDataPtr(byCBTPos, &pCBTItemInstance))
		return Py_BuildException();

	return Py_BuildValue("i", pCBTItemInstance->count());
}

PyObject * playerGetCostumeBonusTransferItemSize(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetCostumeBonusTransferSize());
}

PyObject * playerGetCostumeBonusTransferItemFlags(PyObject* poSelf, PyObject* poArgs)
{
	BYTE byCBTPos;
	if (!PyTuple_GetInteger(poArgs, 0, &byCBTPos))
		return Py_BadArgument();

	network::TItemData* pCBTItemInstance;
	if (!CPythonPlayer::Instance().GetCostumeBonusTransferItemDataPtr(byCBTPos, &pCBTItemInstance))
		return Py_BuildException();

	CItemData* data;
	if (!CItemManager::Instance().GetItemDataPointer(pCBTItemInstance->vnum(), &data))
		return Py_BuildValue("i", 0);

	return Py_BuildValue("i", data->GetFlags());
}

PyObject * playerGetCostumeBonusTransferItemMetinSocket(PyObject* poSelf, PyObject* poArgs)
{
	BYTE byCBTPos;
	if (!PyTuple_GetInteger(poArgs, 0, &byCBTPos))
		return Py_BadArgument();

	int iMetinSocketIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iMetinSocketIndex))
		return Py_BadArgument();

	network::TItemData* pCBTItemInstance;
	if (!CPythonPlayer::Instance().GetCostumeBonusTransferItemDataPtr(byCBTPos, &pCBTItemInstance))
		return Py_BuildException();

	if (iMetinSocketIndex >= ITEM_SOCKET_SLOT_MAX_NUM || iMetinSocketIndex < 0)
		return Py_BuildException();

	return Py_BuildValue("i", pCBTItemInstance->sockets(iMetinSocketIndex));
}

PyObject * playerGetCostumeBonusTransferItemAttribute(PyObject* poSelf, PyObject* poArgs)
{
	BYTE byCBTPos;
	if (!PyTuple_GetInteger(poArgs, 0, &byCBTPos))
		return Py_BadArgument();

	int iAttributeSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iAttributeSlotIndex))
		return Py_BadArgument();

	network::TItemData* pCBTItemInstance;
	if (!CPythonPlayer::Instance().GetCostumeBonusTransferItemDataPtr(byCBTPos, &pCBTItemInstance) || (iAttributeSlotIndex >= ITEM_ATTRIBUTE_SLOT_MAX_NUM || iAttributeSlotIndex < 0))
		return Py_BuildValue("ii", 0, 0);

	BYTE byAttrType = pCBTItemInstance->attributes(iAttributeSlotIndex).type();
	short sAttrValue = pCBTItemInstance->attributes(iAttributeSlotIndex).value();
	return Py_BuildValue("ii", byAttrType, sAttrValue);
}

PyObject * playerIsCostumeBonusTransferWindowEmpty(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().IsEmtpyCostumeBonusTransferWindow());
}

PyObject * playerGetCurrentCostumeBonusTransferItemCount(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("i", CPythonPlayer::Instance().GetCurrentCostumeBonusTransferItemCount());
}
#endif

PyObject * playerMyShopDecoShow(PyObject* poSelf, PyObject* poArgs)
{
	bool isShow;
	if (!PyTuple_GetBoolean(poArgs, 0, &isShow))
		return Py_BadArgument();

	CPythonMyShopDecoManager::Instance().SetShow(isShow);
	return Py_BuildNone();
}

PyObject * playerSelectShopModel(PyObject* poSelf, PyObject* poArgs)
{
	int iModelIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iModelIndex))
		return Py_BadArgument();

	return Py_BuildValue("b", CPythonMyShopDecoManager::Instance().SelectModel(iModelIndex));
}

PyObject * playerSetShopModelHair(PyObject* poSelf, PyObject* poArgs)
{
	int iModelIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iModelIndex))
		return Py_BadArgument();

	return Py_BuildValue("b", CPythonMyShopDecoManager::Instance().SetModelHair(iModelIndex,false));
}

PyObject * playerSetShopModelWeapon(PyObject* poSelf, PyObject* poArgs)
{
	int iModelIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iModelIndex))
		return Py_BadArgument();

	return Py_BuildValue("b", CPythonMyShopDecoManager::Instance().SetModelWeapon(iModelIndex));
}

PyObject* playerFindEmptyInventory(PyObject* poSelf, PyObject* poArgs)
{
	BYTE height;
	if (!PyTuple_GetInteger(poArgs, 0, &height))
		height = 1;

	height = MINMAX(1, height, 3);

	auto inventory_max_num = CPythonPlayer::Instance().GetInventoryMaxNum();
	for (int page = 0; page < c_Inventory_Page_Count; ++page)
	{
		auto start_slot = c_Inventory_Page_Size * page;
		if (start_slot >= inventory_max_num)
			break;

		auto cur_slot_count = MIN(inventory_max_num, start_slot + c_Inventory_Page_Size) - start_slot;

		bool grid[c_Inventory_Page_Size];
		ZeroMemory(grid, sizeof(grid));

		for (auto i = 0; i < cur_slot_count; ++i)
		{
			auto vnum = CPythonPlayer::instance().GetItemIndex(::TItemPos(INVENTORY, start_slot + i));
			if (vnum != 0)
			{
				BYTE size = 1;
				CItemData* item_data;
				if (CItemManager::Instance().GetItemDataPointer(vnum, &item_data))
					size = item_data->GetSize();

				for (int j = 0; j < size; ++j)
					grid[i + j * c_Inventory_Page_X_SlotCount] = true;
			}
		}

		for (auto i = 0; i < cur_slot_count - (height - 1) * c_Inventory_Page_X_SlotCount; ++i)
		{
			bool empty = true;
			for (auto j = 0; j < height; ++j)
			{
				if (grid[i + j * c_Inventory_Page_X_SlotCount])
				{
					empty = false;
					break;
				}
			}

			if (empty)
				return Py_BuildValue("i", start_slot + i);
		}
	}

	return Py_BuildValue("i", -1);
}

PyObject * playerSetShopModelArmor(PyObject* poSelf, PyObject* poArgs)
{
	int iModelIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iModelIndex))
		return Py_BadArgument();

	return Py_BuildValue("b", CPythonMyShopDecoManager::Instance().SetModelArmor(iModelIndex));
}
#ifdef ENABLE_ZODIAC
PyObject * playerGetAnimasphere(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", CPythonPlayer::Instance().GetStatus(POINT_ANIMASPHERE));
}
#endif

#ifdef CHANGE_SKILL_COLOR
PyObject * playerSetSkillColor(PyObject* poSelf, PyObject* poArgs)
{
	int skill;
	if (!PyTuple_GetInteger(poArgs, 0, &skill))
		return Py_BadArgument();

	int col1;
	if (!PyTuple_GetInteger(poArgs, 1, &col1))
		return Py_BadArgument();

	int col2;
	if (!PyTuple_GetInteger(poArgs, 2, &col2))
		return Py_BadArgument();

	int col3;
	if (!PyTuple_GetInteger(poArgs, 3, &col3))
		return Py_BadArgument();

	int col4;
	if (!PyTuple_GetInteger(poArgs, 4, &col4))
		return Py_BadArgument();

	int col5;
	if (!PyTuple_GetInteger(poArgs, 5, &col5))
		return Py_BadArgument();

	if (skill >= 255 || skill < 0 || col1 < 0 || col2 < 0 || col3 < 0 || col4 < 0 || col5 < 0)
	{
		TraceError("playerSetSkillColor skill %d col1 %d col2 %d col3 %d col4 %d col5 %d",
			skill, col1, col2, col3, col4, col5
			);
		return Py_BadArgument();
	}

	CPythonNetworkStream& nstr = CPythonNetworkStream::Instance();
	nstr.SendSkillColorPacket(skill, col1, col2, col3, col4, col5);

	return Py_BuildNone();
}
#endif

#ifdef ENABLE_EQUIPMENT_CHANGER
PyObject * playerSetEquipmentPageSelected(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	CPythonPlayer::Instance().SetSelectedEquipmentPage(iIndex);
	return Py_BuildNone();
}

PyObject * playerGetEquipmentPageSelected(PyObject* poSelf, PyObject* poArgs)
{
	DWORD dwSelectedIndex = CPythonPlayer::Instance().GetSelectedEquipmentPage();
	return Py_BuildValue("i", dwSelectedIndex);
}

PyObject * playerGetEquipmentPageCount(PyObject* poSelf, PyObject* poArgs)
{
	DWORD dwPageCount = CPythonPlayer::Instance().GetEquipmentPageCount();
	return Py_BuildValue("i", dwPageCount);
}

PyObject * playerAddEquipmentPage(PyObject* poSelf, PyObject* poArgs)
{
	char* szPageName;
	if (!PyTuple_GetString(poArgs, 0, &szPageName))
		return Py_BadArgument();

	network::TEquipmentPageInfo kInfo;
	kInfo.set_page_name(szPageName);

	CPythonPlayer::Instance().AddEquipmentPage(kInfo);
	return Py_BuildNone();
}

PyObject * playerRemoveEquipmentPage(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	CPythonPlayer::Instance().RemoveEquipmentPage(iIndex);
	return Py_BuildNone();
}

PyObject * playerGetEquipmentPageWearCell(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	int iWear;
	if (!PyTuple_GetInteger(poArgs, 1, &iWear))
		return Py_BadArgument();

	auto pInfo = CPythonPlayer::Instance().GetEquipmentPageInfo(iIndex);
	if (!pInfo)
		return Py_BuildException("invalid page %d", iIndex);

	return Py_BuildValue("i", pInfo->item_cells(iWear));
}

PyObject * playerGetEquipmentPageName(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pInfo = CPythonPlayer::Instance().GetEquipmentPageInfo(iIndex);
	if (!pInfo)
		return Py_BuildException("invalid page %d", iIndex);

	return Py_BuildValue("s", pInfo->page_name().c_str());
}

PyObject * playerGetEquipmentPageRuneSet(PyObject* poSelf, PyObject* poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();

	auto pInfo = CPythonPlayer::Instance().GetEquipmentPageInfo(iIndex);
	if (!pInfo)
		return Py_BuildException("invalid page %d", iIndex);

	return Py_BuildValue("i", pInfo->rune_page());
}
#endif

PyObject * playerIsAttacking(PyObject* poSelf, PyObject* poArgs)
{
	return Py_BuildValue("b", CPythonNetworkStream::instance().__IsPlayerAttacking());
}

void initPlayer()
{
	static PyMethodDef s_methods[] =
	{
		{ "GetAutoPotionInfo",			playerGetAutoPotionInfo,			METH_VARARGS },
		{ "SetAutoPotionInfo",			playerSetAutoPotionInfo,			METH_VARARGS },

		{ "PickCloseItem",				playerPickCloseItem,				METH_VARARGS },
		{ "SetGameWindow",				playerSetGameWindow,				METH_VARARGS },
		{ "RegisterEffect",				playerRegisterEffect,				METH_VARARGS },
		{ "RegisterCacheEffect",		playerRegisterCacheEffect,			METH_VARARGS },
		{ "SetMouseState",				playerSetMouseState,				METH_VARARGS },
		{ "SetMouseFunc",				playerSetMouseFunc,					METH_VARARGS },
		{ "GetMouseFunc",				playerGetMouseFunc,					METH_VARARGS },
		{ "SetMouseMiddleButtonState",	playerSetMouseMiddleButtonState,	METH_VARARGS },
		{ "SetMainCharacterIndex",		playerSetMainCharacterIndex,		METH_VARARGS },
		{ "GetMainCharacterIndex",		playerGetMainCharacterIndex,		METH_VARARGS },
		{ "GetMainCharacterName",		playerGetMainCharacterName,			METH_VARARGS },
		{ "GetMainCharacterPosition",	playerGetMainCharacterPosition,		METH_VARARGS },
		{ "IsMainCharacterIndex",		playerIsMainCharacterIndex,			METH_VARARGS },
		{ "CanAttackInstance",			playerCanAttackInstance,			METH_VARARGS },
		{ "IsActingEmotion",			playerIsActingEmotion,				METH_VARARGS },
		{ "IsPVPInstance",				playerIsPVPInstance,				METH_VARARGS },
		{ "IsSameEmpire",				playerIsSameEmpire,					METH_VARARGS },
		{ "IsChallengeInstance",		playerIsChallengeInstance,			METH_VARARGS },
		{ "IsRevengeInstance",			playerIsRevengeInstance,			METH_VARARGS },
		{ "IsCantFightInstance",		playerIsCantFightInstance,			METH_VARARGS },
		{ "GetCharacterDistance",		playerGetCharacterDistance,			METH_VARARGS },
		{ "IsInSafeArea",				playerIsInSafeArea,					METH_VARARGS },
		{ "IsMountingHorse",			playerIsMountingHorse,				METH_VARARGS },
		{ "IsObserverMode",				playerIsObserverMode,				METH_VARARGS },
		{ "ActEmotion",					playerActEmotion,					METH_VARARGS },

		{ "ShowPlayer",					playerShowPlayer,					METH_VARARGS },
		{ "HidePlayer",					playerHidePlayer,					METH_VARARGS },

		{ "ComboAttack",				playerComboAttack,					METH_VARARGS },

		{ "SetAutoCameraRotationSpeed",	playerSetAutoCameraRotationSpeed,	METH_VARARGS },
		{ "SetAttackKeyState",			playerSetAttackKeyState,			METH_VARARGS },
		{ "SetSingleDIKKeyState",		playerSetSingleDIKKeyState,			METH_VARARGS },
		{ "EndKeyWalkingImmediately",	playerEndKeyWalkingImmediately,		METH_VARARGS },
		{ "StartMouseWalking",			playerStartMouseWalking,			METH_VARARGS },
		{ "EndMouseWalking",			playerEndMouseWalking,				METH_VARARGS },
		{ "ResetCameraRotation",		playerResetCameraRotation,			METH_VARARGS },
		{ "SetQuickCameraMode",			playerSetQuickCameraMode,			METH_VARARGS },

		///////////////////////////////////////////////////////////////////////////////////////////

		{ "SetSkill",							playerSetSkill,								METH_VARARGS },
		{ "GetSkillIndex",						playerGetSkillIndex,						METH_VARARGS },
		{ "GetSkillSlotIndex",					playerGetSkillSlotIndex,					METH_VARARGS },
		{ "GetSkillGrade",						playerGetSkillGrade,						METH_VARARGS },
		{ "GetSkillLevel",						playerGetSkillLevel,						METH_VARARGS },
		{ "GetSkillEfficientPercentage",		playerGetSkillEfficientPercentage,			METH_VARARGS },
		{ "GetSkillCurrentEfficientPercentage",	playerGetSkillCurrentEfficientPercentage,	METH_VARARGS },
		{ "GetSkillNextEfficientPercentage",	playerGetSkillNextEfficientPercentage,		METH_VARARGS },
		{ "ClickSkillSlot",						playerClickSkillSlot,						METH_VARARGS },
		{ "ChangeCurrentSkillNumberOnly",		playerChangeCurrentSkillNumberOnly,			METH_VARARGS },
		{ "ClearSkillDict",						playerClearSkillDict,						METH_VARARGS },

		{ "GetItemIndex",						playerGetItemIndex,							METH_VARARGS },
		{ "GetItemFlags",						playerGetItemFlags,							METH_VARARGS },
		{ "GetItemCount",						playerGetItemCount,							METH_VARARGS },
		{ "GetItemCountByVnum",					playerGetItemCountByVnum,					METH_VARARGS },
		{ "GetItemCountByVnumRange",			playerGetItemCountByVnumRange,				METH_VARARGS },
#ifdef ENABLE_ALPHA_EQUIP
		{ "GetItemAlphaEquip",					playerGetItemAlphaEquip,					METH_VARARGS },
#endif
		{ "GetItemMetinSocket",					playerGetItemMetinSocket,					METH_VARARGS },
		{ "GetItemAttribute",					playerGetItemAttribute,						METH_VARARGS },
#if defined(GAIDEN)
		{ "GetItemUnbindTime",					playerGetItemUnbindTime,					METH_VARARGS },
#endif
		{ "GetISellItemPrice",					playerGetISellItemPrice,					METH_VARARGS },
		{ "GetIBuyItemPrice",					playerGetIBuyItemPrice,						METH_VARARGS },
		{ "MoveItem",							playerMoveItem,								METH_VARARGS },
		{ "SendClickItemPacket",				playerSendClickItemPacket,					METH_VARARGS },
		{ "IsGMOwner",							playerIsGMOwner,							METH_VARARGS },
		///////////////////////////////////////////////////////////////////////////////////////////

		{ "GetName",					playerGetName,						METH_VARARGS },
		{ "GetJob",						playerGetJob,						METH_VARARGS },
		{ "GetRace",					playerGetRace,						METH_VARARGS },
		{ "GetPlayTime",				playerGetPlayTime,					METH_VARARGS },
		{ "SetPlayTime",				playerSetPlayTime,					METH_VARARGS },

		{ "IsSkillCoolTime",			playerIsSkillCoolTime,				METH_VARARGS },
		{ "GetSkillCoolTime",			playerGetSkillCoolTime,				METH_VARARGS },
		{ "IsSkillActive",				playerIsSkillActive,				METH_VARARGS },
		{ "UseGuildSkill",				playerUseGuildSkill,				METH_VARARGS },
		{ "AffectIndexToSkillIndex",	playerAffectIndexToSkillIndex,		METH_VARARGS },
		{ "SkillIndexToAffectIndex",	playerSkillIndexToAffectIndex,		METH_VARARGS },
		{ "GetEXP",						playerGetEXP,						METH_VARARGS },
		{ "GetStatus",					playerGetStatus,					METH_VARARGS },
		{ "SetStatus",					playerSetStatus,					METH_VARARGS },
		{ "GetRealStatus",				playerGetRealStatus,				METH_VARARGS },
		{ "GetElk",						playerGetElk,						METH_VARARGS },
		{ "GetMoney",					playerGetElk,						METH_VARARGS },
		{ "GetGuildID",					playerGetGuildID,					METH_VARARGS },
		{ "GetGuildName",				playerGetGuildName,					METH_VARARGS },
		{ "GetAlignmentData",			playerGetAlignmentData,				METH_VARARGS },
		{ "RequestAddLocalQuickSlot",					playerRequestAddLocalQuickSlot,						METH_VARARGS },
		{ "RequestAddToEmptyLocalQuickSlot",			playerRequestAddToEmptyLocalQuickSlot,				METH_VARARGS },
		{ "RequestDeleteGlobalQuickSlot",				playerRequestDeleteGlobalQuickSlot,					METH_VARARGS },
		{ "RequestMoveGlobalQuickSlotToLocalQuickSlot",	playerRequestMoveGlobalQuickSlotToLocalQuickSlot,	METH_VARARGS },
		{ "RequestUseLocalQuickSlot",					playerRequestUseLocalQuickSlot,						METH_VARARGS },
		{ "LocalQuickSlotIndexToGlobalQuickSlotIndex",	playerLocalQuickSlotIndexToGlobalQuickSlotIndex,	METH_VARARGS },

		{ "GetQuickPage",				playerGetQuickPage,					METH_VARARGS },
		{ "SetQuickPage",				playerSetQuickPage,					METH_VARARGS },
		{ "GetLocalQuickSlot",			playerGetLocalQuickSlot,			METH_VARARGS },
		{ "GetGlobalQuickSlot",			playerGetGlobalQuickSlot,			METH_VARARGS },
		{ "RemoveQuickSlotByValue",		playerRemoveQuickSlotByValue,		METH_VARARGS },

		{ "isItem",						playerisItem,						METH_VARARGS },
		{ "IsEquipmentSlot",			playerIsEquipmentSlot,				METH_VARARGS },
#ifdef ENABLE_DRAGONSOUL
		{ "IsDSEquipmentSlot",			playerIsDSEquipmentSlot,			METH_VARARGS },
#endif
		{ "IsCostumeSlot",				playerIsCostumeSlot,				METH_VARARGS },
		{ "IsValuableItem",				playerIsValuableItem,				METH_VARARGS },
		{ "IsOpenPrivateShop",			playerIsOpenPrivateShop,			METH_VARARGS },

		// Refine
		{ "GetItemGrade",				playerGetItemGrade,					METH_VARARGS },
		{ "CanRefine",					playerCanRefine,					METH_VARARGS },
		{ "CanDetach",					playerCanDetach,					METH_VARARGS },
		{ "CanUnlock",					playerCanUnlock,					METH_VARARGS },
		{ "CanAttachMetin",				playerCanAttachMetin,				METH_VARARGS },
		{ "IsRefineGradeScroll",		playerIsRefineGradeScroll,			METH_VARARGS },

		{ "ClearTarget",				playerClearTarget,					METH_VARARGS },
		{ "SetTarget",					playerSetTarget,					METH_VARARGS },
		{ "OpenCharacterMenu",			playerOpenCharacterMenu,			METH_VARARGS },

		{ "Update",						playerUpdate,						METH_VARARGS },
		{ "Render",						playerRender,						METH_VARARGS },
		{ "Clear",						playerClear,						METH_VARARGS },

		// Party
		{ "IsPartyMember", playerIsPartyMember, METH_VARARGS },
		{ "IsPartyLeader",				playerIsPartyLeader,				METH_VARARGS },
		{ "IsPartyLeaderByPID",			playerIsPartyLeaderByPID,			METH_VARARGS },
		{ "GetPartyMemberHPPercentage",	playerGetPartyMemberHPPercentage,	METH_VARARGS },
		{ "GetPartyMemberState",		playerGetPartyMemberState,			METH_VARARGS },
		{ "GetPartyMemberAffects",		playerGetPartyMemberAffects,		METH_VARARGS },
		{ "RemovePartyMember",			playerRemovePartyMember,			METH_VARARGS },
		{ "ExitParty",					playerExitParty,					METH_VARARGS },

			// PK Mode
		{ "GetPKMode",					playerGetPKMode,					METH_VARARGS },

			// Mobile
		{ "HasMobilePhoneNumber",		playerHasMobilePhoneNumber,			METH_VARARGS },

			// Emotion
		{ "RegisterEmotionIcon",		playerRegisterEmotionIcon,			METH_VARARGS },
		{ "GetEmotionIconImage",		playerGetEmotionIconImage,			METH_VARARGS },

			// For System
		{ "SetWeaponAttackBonusFlag",	playerSetWeaponAttackBonusFlag,		METH_VARARGS },
		{ "ToggleCoolTime",				playerToggleCoolTime,				METH_VARARGS },
		{ "ToggleLevelLimit",			playerToggleLevelLimit,				METH_VARARGS },
		{ "GetTargetVID",				playerGetTargetVID,					METH_VARARGS },

		{ "SetItemData",				playerSetItemData,					METH_VARARGS },
		{ "SetItemMetinSocket",			playerSetItemMetinSocket,			METH_VARARGS },
		{ "SetItemAttribute",			playerSetItemAttribute,				METH_VARARGS },
		{ "SetItemCount",				playerSetItemCount,					METH_VARARGS },

		{ "GetItemLink",				playerGetItemLink,					METH_VARARGS },
		{ "GetItemLinkFromData",		playerGetItemLinkFromData,			METH_VARARGS },
		{ "SlotTypeToInvenType",		playerSlotTypeToInvenType,			METH_VARARGS },

		{ "IsGameMaster",				playerIsGameMaster,					METH_VARARGS },

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
			// Acce
		{ "GetAcceItemID",				playerGetAcceItemID,				METH_VARARGS },
		{ "GetAcceItemSize",			playerGetAcceItemSize,				METH_VARARGS },
		{ "GetAcceItemFlags",			playerGetAcceItemFlags,				METH_VARARGS },
		{ "GetAcceItemMetinSocket",		playerGetAcceItemMetinSocket,		METH_VARARGS },
		{ "GetAcceItemAttribute",		playerGetAcceItemAttribute,			METH_VARARGS },
		{ "IsAcceWindowEmpty",			playerIsAcceWindowEmpty,			METH_VARARGS },
		{ "GetCurrentItemCount",		playerGetCurrentItemCount,			METH_VARARGS },
		{ "SetAcceRefineWindowOpen",	playerSetAcceRefineWindowOpen,		METH_VARARGS },
		{ "GetAcceRefineWindowOpen",	playerGetAcceRefineWindowOpen,		METH_VARARGS },
		{ "GetAcceRefineWindowType",	playerGetAcceRefineWindowType,		METH_VARARGS },
		{ "FineMoveAcceItemSlot",		playerFineMoveAcceItemSlot,			METH_VARARGS },
		{ "SetAcceActivedItemSlot",		playerSetAcceActivedItemSlot,		METH_VARARGS },
		{ "FindActivedAcceSlot",		playerFindActivedAcceSlot,			METH_VARARGS },
		{ "FindUsingAcceSlot",			playerFindUsingAcceSlot,			METH_VARARGS },
		{ "CanAcceClearItem",			playerCanAcceClearItem,				METH_VARARGS },
		{ "SetHideSashes",				playerSetHideSashes,				METH_VARARGS },
#endif

		{ "GetInventoryMaxNum",			playerGetInventoryMaxNum,			METH_VARARGS },
		{ "GetUppitemInventoryMaxNum",	playerGetUppitemInventoryMaxNum,	METH_VARARGS },
		{ "GetSkillbookInventoryMaxNum",playerGetSkillbookInventoryMaxNum,	METH_VARARGS },
		{ "GetStoneInventoryMaxNum",	playerGetStoneInventoryMaxNum,		METH_VARARGS },
		{ "GetEnchantInventoryMaxNum",	playerGetEnchantInventoryMaxNum,	METH_VARARGS },

#ifdef ENABLE_ANIMAL_SYSTEM
		{ "AnimalSelectType",			playerAnimalSelectType,				METH_VARARGS },
		{ "AnimalIsSummoned",			playerAnimalIsSummoned,				METH_VARARGS },
		{ "AnimalGetName",				playerAnimalGetName,				METH_VARARGS },
		{ "AnimalGetLevel",				playerAnimalGetLevel,				METH_VARARGS },
		{ "AnimalGetExp",				playerAnimalGetExp,					METH_VARARGS },
		{ "AnimalGetMaxExp",			playerAnimalGetMaxExp,				METH_VARARGS },
		{ "AnimalGetStatPoints",		playerAnimalGetStatPoints,			METH_VARARGS },
		{ "AnimalGetStat",				playerAnimalGetStat,				METH_VARARGS },
#endif

		{ "GetWindowBySlot",			playerGetWindowBySlot,				METH_VARARGS },

#ifdef ENABLE_DRAGONSOUL
		{ "SendDragonSoulRefine",		playerSendDragonSoulRefine,			METH_VARARGS },
#endif
#ifdef ENABLE_ZODIAC
		{ "GetAnimasphere",						playerGetAnimasphere,						METH_VARARGS },
#endif
		{ "GetHorseBonusProto",			playerGetHorseBonusProto,			METH_VARARGS },

		{ "IsShowTeamler",				playerIsShowTeamler,				METH_VARARGS },

#ifdef ENABLE_FAKEBUFF
		{ "GetFakeBuffSkillLevel",		playerGetFakeBuffSkillLevel,		METH_VARARGS },
#endif

#ifdef ENABLE_ATTRTREE
		{ "GetAttrtreeLevel",			playerGetAttrtreeLevel,				METH_VARARGS },
		{ "AttrtreeCellToID",			playerAttrtreeCellToID,				METH_VARARGS },
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
		{ "SetCostumeBonusTransferWindowOpen",				playerSetCostumeBonusTransferWindowOpen,			METH_VARARGS },
		{ "GetCostumeBonusTransferWindowOpen",				playerGetCostumeBonusTransferWindowOpen,			METH_VARARGS },
		{ "GetCostumeBonusTransferItemID",					playerGetCostumeBonusTransferItemID,				METH_VARARGS },
		{ "GetCostumeBonusTransferItemCount",				playerGetCostumeBonusTransferItemCount,				METH_VARARGS },
		{ "GetCostumeBonusTransferItemSize",				playerGetCostumeBonusTransferItemSize,				METH_VARARGS },
		{ "GetCostumeBonusTransferItemFlags",				playerGetCostumeBonusTransferItemFlags,				METH_VARARGS },
		{ "GetCostumeBonusTransferItemMetinSocket",			playerGetCostumeBonusTransferItemMetinSocket,		METH_VARARGS },
		{ "GetCostumeBonusTransferItemAttribute",			playerGetCostumeBonusTransferItemAttribute,			METH_VARARGS },
		{ "IsCostumeBonusTransferWindowEmpty",				playerIsCostumeBonusTransferWindowEmpty,			METH_VARARGS },
		{ "GetCurrentCostumeBonusTransferItemCount",		playerGetCurrentCostumeBonusTransferItemCount,		METH_VARARGS },
#endif

#ifdef COMBAT_ZONE
		{ "IsCombatZoneMap",								playerIsCombatZoneMap,								METH_VARARGS },
		{ "GetCombatZonePoints",							playerGetCombatZonePoints,							METH_VARARGS },
#endif

		{ "MyShopDecoShow",									playerMyShopDecoShow,								METH_VARARGS },
		{ "SelectShopModel",								playerSelectShopModel,								METH_VARARGS },
		{ "SetShopModelHair",								playerSetShopModelHair,								METH_VARARGS },
		{ "SetShopModelArmor",								playerSetShopModelArmor,								METH_VARARGS },
		{ "SetShopModelWeapon",								playerSetShopModelWeapon,								METH_VARARGS },

		{ "FindEmptyInventory",								playerFindEmptyInventory,							METH_VARARGS },

#ifdef CHANGE_SKILL_COLOR
		{ "SetSkillColor",									playerSetSkillColor,								METH_VARARGS },
#endif

#ifdef ENABLE_EQUIPMENT_CHANGER
		{ "SetEquipmentPageSelected",		playerSetEquipmentPageSelected,			METH_VARARGS },
		{ "GetEquipmentPageSelected",		playerGetEquipmentPageSelected,			METH_VARARGS },
		{ "GetEquipmentPageCount",			playerGetEquipmentPageCount,			METH_VARARGS },
		{ "AddEquipmentPage",				playerAddEquipmentPage,					METH_VARARGS },
		{ "RemoveEquipmentPage",			playerRemoveEquipmentPage,				METH_VARARGS },
		{ "GetEquipmentPageWearCell",		playerGetEquipmentPageWearCell,			METH_VARARGS },
		{ "GetEquipmentPageName",			playerGetEquipmentPageName,				METH_VARARGS },
		{ "GetEquipmentPageRuneSet",		playerGetEquipmentPageRuneSet,			METH_VARARGS },
#endif
		{ "IsAttacking",					playerIsAttacking,						METH_VARARGS },

		{ NULL,							NULL,								NULL },
	};

	PyObject* poModule = Py_InitModule("player", s_methods);
    PyModule_AddIntConstant(poModule, "LEVEL",					POINT_LEVEL);
    PyModule_AddIntConstant(poModule, "VOICE",					POINT_VOICE);
    PyModule_AddIntConstant(poModule, "EXP",					POINT_EXP);
    PyModule_AddIntConstant(poModule, "NEXT_EXP",				POINT_NEXT_EXP);
    PyModule_AddIntConstant(poModule, "HP",						POINT_HP);
    PyModule_AddIntConstant(poModule, "MAX_HP",					POINT_MAX_HP);
    PyModule_AddIntConstant(poModule, "SP",						POINT_SP);
    PyModule_AddIntConstant(poModule, "MAX_SP",					POINT_MAX_SP);
    PyModule_AddIntConstant(poModule, "STAMINA",				POINT_STAMINA);
    PyModule_AddIntConstant(poModule, "MAX_STAMINA",			POINT_MAX_STAMINA);
    PyModule_AddIntConstant(poModule, "ELK",					POINT_GOLD);
    PyModule_AddIntConstant(poModule, "ST",						POINT_ST);
    PyModule_AddIntConstant(poModule, "HT",						POINT_HT);
    PyModule_AddIntConstant(poModule, "DX",						POINT_DX);
    PyModule_AddIntConstant(poModule, "IQ",						POINT_IQ);
    PyModule_AddIntConstant(poModule, "ATT_POWER",				POINT_ATT_POWER);
	PyModule_AddIntConstant(poModule, "ATT_MIN",				POINT_MIN_ATK);
	PyModule_AddIntConstant(poModule, "ATT_MAX",				POINT_MAX_ATK);
	PyModule_AddIntConstant(poModule, "MIN_MAGIC_WEP",			POINT_MIN_MAGIC_WEP);
	PyModule_AddIntConstant(poModule, "MAX_MAGIC_WEP",			POINT_MAX_MAGIC_WEP);
    PyModule_AddIntConstant(poModule, "ATT_SPEED",				POINT_ATT_SPEED);
	PyModule_AddIntConstant(poModule, "ATT_BONUS",				POINT_ATT_GRADE_BONUS);
    PyModule_AddIntConstant(poModule, "EVADE_RATE",				POINT_EVADE_RATE);
    PyModule_AddIntConstant(poModule, "MOVING_SPEED",			POINT_MOV_SPEED);
    PyModule_AddIntConstant(poModule, "DEF_GRADE",				POINT_DEF_GRADE);
    PyModule_AddIntConstant(poModule, "DEF_BONUS",				POINT_DEF_GRADE_BONUS);
    PyModule_AddIntConstant(poModule, "CASTING_SPEED",			POINT_CASTING_SPEED);
    PyModule_AddIntConstant(poModule, "MAG_ATT",				POINT_MAGIC_ATT_GRADE);
    PyModule_AddIntConstant(poModule, "MAG_DEF",				POINT_MAGIC_DEF_GRADE);
    PyModule_AddIntConstant(poModule, "EMPIRE_POINT",			POINT_EMPIRE_POINT);
	PyModule_AddIntConstant(poModule, "STAT",					POINT_STAT);
	PyModule_AddIntConstant(poModule, "SKILL_PASSIVE",			POINT_SUB_SKILL);
	PyModule_AddIntConstant(poModule, "SKILL_SUPPORT",			POINT_SUB_SKILL);
	PyModule_AddIntConstant(poModule, "SKILL_ACTIVE",			POINT_SKILL);
	PyModule_AddIntConstant(poModule, "SKILL_MOUNT",			POINT_HORSE_SKILL);
	
    PyModule_AddIntConstant(poModule, "POINT_HP_REGEN",			POINT_HP_REGEN);
    PyModule_AddIntConstant(poModule, "POINT_SP_REGEN",			POINT_SP_REGEN);

	PyModule_AddIntConstant(poModule, "POINT_HP_RECOVERY",		POINT_HP_RECOVERY);
	PyModule_AddIntConstant(poModule, "POINT_SP_RECOVERY",		POINT_SP_RECOVERY);

	PyModule_AddIntConstant(poModule, "POINT_POISON_PCT",		POINT_POISON_PCT);
	PyModule_AddIntConstant(poModule, "POINT_STUN_PCT",			POINT_STUN_PCT);
	PyModule_AddIntConstant(poModule, "POINT_SLOW_PCT",			POINT_SLOW_PCT);
	PyModule_AddIntConstant(poModule, "POINT_CRITICAL_PCT",		POINT_CRITICAL_PCT);
	PyModule_AddIntConstant(poModule, "POINT_PENETRATE_PCT",	POINT_PENETRATE_PCT);
	
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_HUMAN",	POINT_ATTBONUS_HUMAN);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ANIMAL",	POINT_ATTBONUS_ANIMAL);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ORC",		POINT_ATTBONUS_ORC);
#ifdef ENABLE_ZODIAC
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ZODIAC",	POINT_ATTBONUS_ZODIAC);
#else
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ZODIAC",	POINT_NONE);
#endif
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_MILGYO",	POINT_ATTBONUS_MILGYO);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_DEVIL",	POINT_ATTBONUS_DEVIL);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_UNDEAD",	POINT_ATTBONUS_UNDEAD);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_INSECT",	POINT_ATTBONUS_INSECT);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_FIRE",	POINT_ATTBONUS_FIRE);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ICE",		POINT_ATTBONUS_ICE);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_DESERT",	POINT_ATTBONUS_DESERT);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_MONSTER",	POINT_ATTBONUS_MONSTER);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_WARRIOR",	POINT_ATTBONUS_WARRIOR);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ASSASSIN",	POINT_ATTBONUS_ASSASSIN);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_SURA",	POINT_ATTBONUS_SURA);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_SHAMAN",	POINT_ATTBONUS_SHAMAN);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_TREE",	POINT_ATTBONUS_TREE);
	
	PyModule_AddIntConstant(poModule, "POINT_RESIST_WARRIOR",	POINT_RESIST_WARRIOR);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_ASSASSIN",	POINT_RESIST_ASSASSIN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_SURA",		POINT_RESIST_SURA);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_SHAMAN",	POINT_RESIST_SHAMAN);
	
	PyModule_AddIntConstant(poModule, "POINT_STEAL_HP",			POINT_STEAL_HP);
	PyModule_AddIntConstant(poModule, "POINT_STEAL_SP",			POINT_STEAL_SP);
	
	PyModule_AddIntConstant(poModule, "POINT_MANA_BURN_PCT",	POINT_MANA_BURN_PCT);
	PyModule_AddIntConstant(poModule, "POINT_DAMAGE_SP_RECOVER",	POINT_DAMAGE_SP_RECOVER);
	
	PyModule_AddIntConstant(poModule, "POINT_BLOCK",			POINT_BLOCK);
	PyModule_AddIntConstant(poModule, "POINT_DODGE",			POINT_DODGE);
	
	PyModule_AddIntConstant(poModule, "POINT_RESIST_SWORD",			POINT_RESIST_SWORD);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_TWOHAND",		POINT_RESIST_TWOHAND);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_DAGGER",		POINT_RESIST_DAGGER);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_BELL",			POINT_RESIST_BELL);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_FAN",			POINT_RESIST_FAN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_BOW",			POINT_RESIST_BOW);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_ELEC",			POINT_RESIST_ELEC);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_FIRE",			POINT_RESIST_FIRE);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_MAGIC",			POINT_RESIST_MAGIC);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_WIND",			POINT_RESIST_WIND);

	PyModule_AddIntConstant(poModule, "POINT_BLOCK_IGNORE_BONUS", POINT_BLOCK_IGNORE_BONUS);

	PyModule_AddIntConstant(poModule, "POINT_EMPIRE_A_KILLED", POINT_EMPIRE_A_KILLED);
	PyModule_AddIntConstant(poModule, "POINT_EMPIRE_B_KILLED", POINT_EMPIRE_B_KILLED);
	PyModule_AddIntConstant(poModule, "POINT_EMPIRE_C_KILLED", POINT_EMPIRE_C_KILLED);
	PyModule_AddIntConstant(poModule, "POINT_DUELS_WON", POINT_DUELS_WON);
	PyModule_AddIntConstant(poModule, "POINT_DUELS_LOST", POINT_DUELS_LOST);
	PyModule_AddIntConstant(poModule, "POINT_MONSTERS_KILLED", POINT_MONSTERS_KILLED);
	PyModule_AddIntConstant(poModule, "POINT_BOSSES_KILLED", POINT_BOSSES_KILLED);
	PyModule_AddIntConstant(poModule, "POINT_STONES_DESTROYED", POINT_STONES_DESTROYED);

#ifdef ENABLE_RUNE_SYSTEM
	PyModule_AddIntConstant(poModule, "POINT_RUNE_SHIELD_PER_HIT", POINT_RUNE_SHIELD_PER_HIT);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_HEAL_ON_KILL", POINT_RUNE_HEAL_ON_KILL);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_BONUS_DAMAGE_AFTER_HIT", POINT_RUNE_BONUS_DAMAGE_AFTER_HIT);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_3RD_ATTACK_BONUS", POINT_RUNE_3RD_ATTACK_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_FIRST_NORMAL_HIT_BONUS", POINT_RUNE_FIRST_NORMAL_HIT_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_MSHIELD_PER_SKILL", POINT_RUNE_MSHIELD_PER_SKILL);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_HARVEST", POINT_RUNE_HARVEST);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_DAMAGE_AFTER_3", POINT_RUNE_DAMAGE_AFTER_3);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_OUT_OF_COMBAT_SPEED", POINT_RUNE_OUT_OF_COMBAT_SPEED);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_RESET_SKILL", POINT_RUNE_RESET_SKILL);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_COMBAT_CASTING_SPEED", POINT_RUNE_COMBAT_CASTING_SPEED);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT", POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_MOVSPEED_AFTER_3", POINT_RUNE_MOVSPEED_AFTER_3);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_SLOW_ON_ATTACK", POINT_RUNE_SLOW_ON_ATTACK);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_ATTACK_SLOW_PCT", POINT_RUNE_ATTACK_SLOW_PCT);
	PyModule_AddIntConstant(poModule, "POINT_RUNE_MOVEMENT_SLOW_PCT", POINT_RUNE_MOVEMENT_SLOW_PCT);
#endif
	PyModule_AddIntConstant(poModule, "POINT_REFLECT_MELEE",			POINT_REFLECT_MELEE);
	PyModule_AddIntConstant(poModule, "POINT_POISON_REDUCE",			POINT_POISON_REDUCE);
	
	PyModule_AddIntConstant(poModule, "POINT_EXP_DOUBLE_BONUS",			POINT_EXP_DOUBLE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_GOLD_DOUBLE_BONUS",			POINT_GOLD_DOUBLE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_ITEM_DROP_BONUS",			POINT_ITEM_DROP_BONUS);
	
	PyModule_AddIntConstant(poModule, "POINT_POTION_BONUS",			POINT_POTION_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_KILL_HP_RECOVER",			POINT_KILL_HP_RECOVERY);
	PyModule_AddIntConstant(poModule, "POINT_KILL_SP_RECOVER",			POINT_KILL_SP_RECOVER);
	
	PyModule_AddIntConstant(poModule, "POINT_ATT_BONUS",			POINT_ATT_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_DEF_BONUS",			POINT_DEF_BONUS);
	
	PyModule_AddIntConstant(poModule, "POINT_ATT_GRADE_BONUS",			POINT_ATT_GRADE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_DEF_GRADE_BONUS",			POINT_DEF_GRADE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_MAGIC_ATT_GRADE_BONUS",	POINT_MAGIC_ATT_GRADE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_MAGIC_DEF_GRADE_BONUS",	POINT_MAGIC_DEF_GRADE_BONUS);

	PyModule_AddIntConstant(poModule, "POINT_RESIST_NORMAL_DAMAGE",	POINT_RESIST_NORMAL_DAMAGE);

	PyModule_AddIntConstant(poModule, "POINT_MALL_ATTBONUS",	POINT_MALL_ATTBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_DEFBONUS",	POINT_MALL_DEFBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_EXPBONUS",	POINT_MALL_EXPBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_ITEMBONUS",	POINT_MALL_ITEMBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_GOLDBONUS",	POINT_MALL_GOLDBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MAX_HP_PCT",		POINT_MAX_HP_PCT);
	PyModule_AddIntConstant(poModule, "POINT_MAX_SP_PCT",		POINT_MAX_SP_PCT);

	PyModule_AddIntConstant(poModule, "POINT_SKILL_DAMAGE_BONUS",		POINT_SKILL_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_NORMAL_HIT_DAMAGE_BONUS",		POINT_NORMAL_HIT_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_SKILL_DEFEND_BONUS",		POINT_SKILL_DEFEND_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_NORMAL_HIT_DEFEND_BONUS",		POINT_NORMAL_HIT_DEFEND_BONUS);
	
	PyModule_AddIntConstant(poModule, "POINT_COSTUME_ATTR_BONUS",		POINT_COSTUME_ATTR_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_MAGIC_ATT_BONUS_PER",		POINT_MAGIC_ATT_BONUS_PER);
	PyModule_AddIntConstant(poModule, "POINT_MELEE_MAGIC_ATT_BONUS_PER",		POINT_MELEE_MAGIC_ATT_BONUS_PER);
	
	PyModule_AddIntConstant(poModule, "POINT_RESIST_ICE",		POINT_RESIST_ICE);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_EARTH",		POINT_RESIST_EARTH);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_DARK",		POINT_RESIST_DARK);
	
	PyModule_AddIntConstant(poModule, "POINT_RESIST_CRITICAL",		POINT_RESIST_CRITICAL);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_PENETRATE",		POINT_RESIST_PENETRATE);

	PyModule_AddIntConstant(poModule, "POINT_EXP_REAL_BONUS",		POINT_EXP_REAL_BONUS);

	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_METIN",			POINT_ATTBONUS_METIN);
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_BOSS",			POINT_ATTBONUS_BOSS);

#ifdef ENABLE_WOLFMAN
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_WOLFMAN",	POINT_ATTBONUS_WOLFMAN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_WOLFMAN",	POINT_RESIST_WOLFMAN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_CLAW",		POINT_RESIST_CLAW);
#endif
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_ALL_ELEMENTS",		POINT_ATTBONUS_ALL_ELEMENTS);

	PyModule_AddIntConstant(poModule, "PLAYTIME",				POINT_PLAYTIME);
	PyModule_AddIntConstant(poModule, "BOW_DISTANCE",			POINT_BOW_DISTANCE);
	PyModule_AddIntConstant(poModule, "HP_RECOVERY",			POINT_HP_RECOVERY);
	PyModule_AddIntConstant(poModule, "SP_RECOVERY",			POINT_SP_RECOVERY);
	PyModule_AddIntConstant(poModule, "ATTACKER_BONUS",			POINT_PARTY_ATT_GRADE);
    PyModule_AddIntConstant(poModule, "MAX_NUM",				POINT_MAX_NUM);
	////
	PyModule_AddIntConstant(poModule, "POINT_CRITICAL_PCT",		POINT_CRITICAL_PCT);
	PyModule_AddIntConstant(poModule, "POINT_PENETRATE_PCT",	POINT_PENETRATE_PCT);
	PyModule_AddIntConstant(poModule, "POINT_MALL_ATTBONUS",	POINT_MALL_ATTBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_DEFBONUS",	POINT_MALL_DEFBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_EXPBONUS",	POINT_MALL_EXPBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_ITEMBONUS",	POINT_MALL_ITEMBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MALL_GOLDBONUS",	POINT_MALL_GOLDBONUS);
	PyModule_AddIntConstant(poModule, "POINT_MAX_HP_PCT",		POINT_MAX_HP_PCT);
	PyModule_AddIntConstant(poModule, "POINT_MAX_SP_PCT",		POINT_MAX_SP_PCT);

	PyModule_AddIntConstant(poModule, "POINT_ANTI_EXP",			POINT_ANTI_EXP);
	PyModule_AddIntConstant(poModule, "POINT_MOUNT_BUFF_BONUS",			POINT_MOUNT_BUFF_BONUS);

	PyModule_AddIntConstant(poModule, "POINT_SKILL_DAMAGE_BONUS",		POINT_SKILL_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_NORMAL_HIT_DAMAGE_BONUS",		POINT_NORMAL_HIT_DAMAGE_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_SKILL_DEFEND_BONUS",		POINT_SKILL_DEFEND_BONUS);
	PyModule_AddIntConstant(poModule, "POINT_NORMAL_HIT_DEFEND_BONUS",		POINT_NORMAL_HIT_DEFEND_BONUS);

	PyModule_AddIntConstant(poModule, "ENERGY",		POINT_ENERGY);
	PyModule_AddIntConstant(poModule, "ENERGY_END_TIME",		POINT_ENERGY_END_TIME);

#ifdef ENABLE_GAYA_SYSTEM
	PyModule_AddIntConstant(poModule, "GAYA",		POINT_GAYA);
#endif

	PyModule_AddIntConstant(poModule, "POINT_RESIST_SWORD_PEN",			POINT_RESIST_SWORD_PEN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_TWOHAND_PEN",		POINT_RESIST_TWOHAND_PEN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_DAGGER_PEN",		POINT_RESIST_DAGGER_PEN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_BELL_PEN",			POINT_RESIST_BELL_PEN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_FAN_PEN",			POINT_RESIST_FAN_PEN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_BOW_PEN",			POINT_RESIST_BOW_PEN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_ATTBONUS_HUMAN",	POINT_RESIST_ATTBONUS_HUMAN);

    PyModule_AddIntConstant(poModule, "SKILL_GRADE_NORMAL",				CPythonPlayer::SKILL_NORMAL);
    PyModule_AddIntConstant(poModule, "SKILL_GRADE_MASTER",				CPythonPlayer::SKILL_MASTER);
    PyModule_AddIntConstant(poModule, "SKILL_GRADE_GRAND_MASTER",		CPythonPlayer::SKILL_GRAND_MASTER);
    PyModule_AddIntConstant(poModule, "SKILL_GRADE_PERFECT_MASTER",		CPythonPlayer::SKILL_PERFECT_MASTER);
    PyModule_AddIntConstant(poModule, "SKILL_GRADE_LEGENDARY_MASTER",	CPythonPlayer::SKILL_LEGENDARY_MASTER);

	PyModule_AddIntConstant(poModule, "CATEGORY_ACTIVE",		CPythonPlayer::CATEGORY_ACTIVE);
	PyModule_AddIntConstant(poModule, "CATEGORY_PASSIVE",		CPythonPlayer::CATEGORY_PASSIVE);
	
	PyModule_AddIntConstant(poModule, "INVENTORY_PAGE_X_SLOTCOUNT",	c_Inventory_Page_X_SlotCount);
	PyModule_AddIntConstant(poModule, "INVENTORY_PAGE_Y_SLOTCOUNT",	c_Inventory_Page_Y_SlotCount);
	PyModule_AddIntConstant(poModule, "INVENTORY_PAGE_SIZE",	c_Inventory_Page_Size);
	PyModule_AddIntConstant(poModule, "INVENTORY_PAGE_COUNT",	c_Inventory_Page_Count);
	PyModule_AddIntConstant(poModule, "INVENTORY_SLOT_COUNT",	c_Inventory_Count);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_SLOT_START",	c_Equipment_Start);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_PAGE_COUNT",	c_Wear_Max);

	PyModule_AddIntConstant(poModule, "SHINING_EQUIP_SLOT_START", SHINING_EQUIP_SLOT_START);
	PyModule_AddIntConstant(poModule, "SHINING_MAX_NUM", SHINING_MAX_NUM);

#ifdef __SKIN_SYSTEM__
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_EQUIP_SLOT_START", SKINSYSTEM_EQUIP_SLOT_START);
	PyModule_AddIntConstant(poModule, "SKINSYSTEM_MAX_NUM", SKINSYSTEM_EQUIP_SLOT_END - SKINSYSTEM_EQUIP_SLOT_START);
#endif

	PyModule_AddIntConstant(poModule, "MBF_SKILL",	CPythonPlayer::MBF_SKILL);
	PyModule_AddIntConstant(poModule, "MBF_ATTACK",	CPythonPlayer::MBF_ATTACK);
	PyModule_AddIntConstant(poModule, "MBF_CAMERA",	CPythonPlayer::MBF_CAMERA);
	PyModule_AddIntConstant(poModule, "MBF_SMART",	CPythonPlayer::MBF_SMART);
	PyModule_AddIntConstant(poModule, "MBF_MOVE",	CPythonPlayer::MBF_MOVE);
	PyModule_AddIntConstant(poModule, "MBF_AUTO",	CPythonPlayer::MBF_AUTO);
	PyModule_AddIntConstant(poModule, "MBS_PRESS",	CPythonPlayer::MBS_PRESS); 
	PyModule_AddIntConstant(poModule, "MBS_PRESS_SHOPDECO",	CPythonPlayer::MBS_PRESS_SHOPDECO);
	PyModule_AddIntConstant(poModule, "MBS_CLICK",	CPythonPlayer::MBS_CLICK);
	PyModule_AddIntConstant(poModule, "MBT_RIGHT",	CPythonPlayer::MBT_RIGHT);
	PyModule_AddIntConstant(poModule, "MBT_LEFT",	CPythonPlayer::MBT_LEFT);

	// Public code with server
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_NONE",						SLOT_TYPE_NONE);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_INVENTORY",				SLOT_TYPE_INVENTORY);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_SKILL",					SLOT_TYPE_SKILL);
	// Special indecies for client
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_SHOP",						SLOT_TYPE_SHOP);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_EXCHANGE_OWNER",			SLOT_TYPE_EXCHANGE_OWNER);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_EXCHANGE_TARGET",			SLOT_TYPE_EXCHANGE_TARGET);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_QUICK_SLOT",				SLOT_TYPE_QUICK_SLOT);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_SAFEBOX",					SLOT_TYPE_SAFEBOX);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_PRIVATE_SHOP",				SLOT_TYPE_PRIVATE_SHOP);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_MALL",						SLOT_TYPE_MALL);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_EMOTION",					SLOT_TYPE_EMOTION);
#ifdef ENABLE_GUILD_SAFEBOX
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_GUILD_SAFEBOX",			SLOT_TYPE_GUILD_SAFEBOX);
#endif
#ifdef ENABLE_DRAGONSOUL
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_DRAGON_SOUL_INVENTORY",	SLOT_TYPE_DRAGON_SOUL_INVENTORY);
#endif
#ifdef ENABLE_AUCTION
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_AUCTION_SHOP",				SLOT_TYPE_AUCTION_SHOP);
#endif

	PyModule_AddIntConstant(poModule, "RESERVED_WINDOW",					RESERVED_WINDOW);
	PyModule_AddIntConstant(poModule, "INVENTORY",							INVENTORY);
#ifdef ENABLE_SKILL_INVENTORY
	PyModule_AddIntConstant(poModule, "SKILLBOOK_INVENTORY",				SKILLBOOK_INVENTORY); 
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_SKILLBOOK_INVENTORY",		SLOT_TYPE_SKILLBOOK_INVENTORY); 
#endif
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_UPPITEM_INVENTORY",		SLOT_TYPE_UPPITEM_INVENTORY); 
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_STONE_INVENTORY",			SLOT_TYPE_STONE_INVENTORY); 
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_ENCHANT_INVENTORY",		SLOT_TYPE_ENCHANT_INVENTORY); 

	PyModule_AddIntConstant(poModule, "UPPITEM_INVENTORY",					UPPITEM_INVENTORY);
	PyModule_AddIntConstant(poModule, "STONE_INVENTORY",					STONE_INVENTORY);
	PyModule_AddIntConstant(poModule, "ENCHANT_INVENTORY",					ENCHANT_INVENTORY);
#ifdef ENABLE_DRAGONSOUL
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_INVENTORY",				DRAGON_SOUL_INVENTORY);
#endif
	PyModule_AddIntConstant(poModule, "EQUIPMENT",							EQUIPMENT);
	PyModule_AddIntConstant(poModule, "SAFEBOX",							SAFEBOX);
	PyModule_AddIntConstant(poModule, "MALL",								MALL);
#ifdef ENABLE_GUILD_SAFEBOX
	PyModule_AddIntConstant(poModule, "GUILD_SAFEBOX",						GUILD_SAFEBOX);
#endif
	PyModule_AddIntConstant(poModule, "GROUND",								GROUND);
	
	PyModule_AddIntConstant(poModule, "INVENTORY_SLOT_START",				INVENTORY_SLOT_START);
	PyModule_AddIntConstant(poModule, "INVENTORY_SLOT_END",					INVENTORY_SLOT_END);
#ifdef ENABLE_SKILL_INVENTORY
	PyModule_AddIntConstant(poModule, "SKILLBOOK_INV_SLOT_START",			SKILLBOOK_INV_SLOT_START);
	PyModule_AddIntConstant(poModule, "SKILLBOOK_INV_SLOT_END",				SKILLBOOK_INV_SLOT_END);
#endif
	PyModule_AddIntConstant(poModule, "UPPITEM_INV_SLOT_START",				UPPITEM_INV_SLOT_START);
	PyModule_AddIntConstant(poModule, "UPPITEM_INV_SLOT_END",				UPPITEM_INV_SLOT_END);
	PyModule_AddIntConstant(poModule, "STONE_INV_SLOT_START",				STONE_INV_SLOT_START);
	PyModule_AddIntConstant(poModule, "STONE_INV_SLOT_END",					STONE_INV_SLOT_END);
	PyModule_AddIntConstant(poModule, "ENCHANT_INV_SLOT_START",				ENCHANT_INV_SLOT_START);
	PyModule_AddIntConstant(poModule, "ENCHANT_INV_SLOT_END",					ENCHANT_INV_SLOT_END);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_SLOT_START",				EQUIPMENT_SLOT_START);
	PyModule_AddIntConstant(poModule, "EQUIPMENT_SLOT_END",					EQUIPMENT_SLOT_END);

	PyModule_AddIntConstant(poModule, "ITEM_MONEY",					-1);

	PyModule_AddIntConstant(poModule, "SKILL_SLOT_COUNT",			SKILL_MAX_NUM);

	PyModule_AddIntConstant(poModule, "EFFECT_PICK",				CPythonPlayer::EFFECT_PICK);

	PyModule_AddIntConstant(poModule, "METIN_SOCKET_TYPE_NONE",					CPythonPlayer::METIN_SOCKET_TYPE_NONE);
	PyModule_AddIntConstant(poModule, "METIN_SOCKET_TYPE_SILVER",				CPythonPlayer::METIN_SOCKET_TYPE_SILVER);
	PyModule_AddIntConstant(poModule, "METIN_SOCKET_TYPE_GOLD",					CPythonPlayer::METIN_SOCKET_TYPE_GOLD);
#ifdef PROMETA
	PyModule_AddIntConstant(poModule, "METIN_SOCKET_TYPE_ACCE", CPythonPlayer::METIN_SOCKET_TYPE_ACCE);
#endif
	PyModule_AddIntConstant(poModule, "METIN_SOCKET_MAX_NUM",					ITEM_SOCKET_SLOT_MAX_NUM);
	PyModule_AddIntConstant(poModule, "NORMAL_ATTRIBUTE_SLOT_MAX_NUM",			ITEM_NORMAL_ATTRIBUTE_SLOT_MAX_NUM);
	PyModule_AddIntConstant(poModule, "ATTRIBUTE_SLOT_MAX_NUM",					ITEM_ATTRIBUTE_SLOT_MAX_NUM);

	PyModule_AddIntConstant(poModule, "REFINE_CANT",							REFINE_CANT);
	PyModule_AddIntConstant(poModule, "REFINE_OK",								REFINE_OK);
	PyModule_AddIntConstant(poModule, "REFINE_ALREADY_MAX_SOCKET_COUNT",		REFINE_ALREADY_MAX_SOCKET_COUNT);
	PyModule_AddIntConstant(poModule, "REFINE_NEED_MORE_GOOD_SCROLL",			REFINE_NEED_MORE_GOOD_SCROLL);
	PyModule_AddIntConstant(poModule, "REFINE_CANT_MAKE_SOCKET_ITEM",			REFINE_CANT_MAKE_SOCKET_ITEM);
	PyModule_AddIntConstant(poModule, "REFINE_NOT_NEXT_GRADE_ITEM",				REFINE_NOT_NEXT_GRADE_ITEM);
	PyModule_AddIntConstant(poModule, "REFINE_CANT_REFINE_METIN_TO_EQUIPMENT",	REFINE_CANT_REFINE_METIN_TO_EQUIPMENT);
	PyModule_AddIntConstant(poModule, "REFINE_CANT_REFINE_ROD",					REFINE_CANT_REFINE_ROD);
	PyModule_AddIntConstant(poModule, "ATTACH_METIN_CANT",						ATTACH_METIN_CANT);
	PyModule_AddIntConstant(poModule, "ATTACH_METIN_OK",						ATTACH_METIN_OK);
	PyModule_AddIntConstant(poModule, "ATTACH_METIN_NOT_MATCHABLE_ITEM",		ATTACH_METIN_NOT_MATCHABLE_ITEM);
	PyModule_AddIntConstant(poModule, "ATTACH_METIN_NO_MATCHABLE_SOCKET",		ATTACH_METIN_NO_MATCHABLE_SOCKET);
	PyModule_AddIntConstant(poModule, "ATTACH_METIN_NOT_EXIST_GOLD_SOCKET",		ATTACH_METIN_NOT_EXIST_GOLD_SOCKET);
	PyModule_AddIntConstant(poModule, "ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT",	ATTACH_METIN_CANT_ATTACH_TO_EQUIPMENT);
	PyModule_AddIntConstant(poModule, "DETACH_METIN_CANT",						DETACH_METIN_CANT);
	PyModule_AddIntConstant(poModule, "DETACH_METIN_OK",						DETACH_METIN_OK);

	// Party
	PyModule_AddIntConstant(poModule, "PARTY_STATE_NORMAL",						CPythonPlayer::PARTY_ROLE_NORMAL);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_ATTACKER",					CPythonPlayer::PARTY_ROLE_ATTACKER);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_TANKER",						CPythonPlayer::PARTY_ROLE_TANKER);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_BUFFER",						CPythonPlayer::PARTY_ROLE_BUFFER);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_SKILL_MASTER",				CPythonPlayer::PARTY_ROLE_SKILL_MASTER);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_BERSERKER",					CPythonPlayer::PARTY_ROLE_BERSERKER);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_DEFENDER",					CPythonPlayer::PARTY_ROLE_DEFENDER);
	PyModule_AddIntConstant(poModule, "PARTY_STATE_MAX_NUM",					CPythonPlayer::PARTY_ROLE_MAX_NUM);

	// Skill Index
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_TONGSOL",		c_iSkillIndex_Tongsol);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_FISHING",		c_iSkillIndex_Fishing);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_MINING",			c_iSkillIndex_Mining);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_MAKING",			c_iSkillIndex_Making);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_COMBO",			c_iSkillIndex_Combo);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_LANGUAGE1",		c_iSkillIndex_Language1);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_LANGUAGE2",		c_iSkillIndex_Language2);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_LANGUAGE3",		c_iSkillIndex_Language3);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_POLYMORPH",		c_iSkillIndex_Polymorph);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_RIDING",			c_iSkillIndex_Riding);
	PyModule_AddIntConstant(poModule, "SKILL_INDEX_SUMMON",			c_iSkillIndex_Summon);

	// PK Mode
	PyModule_AddIntConstant(poModule, "PK_MODE_PEACE",				PK_MODE_PEACE);
	PyModule_AddIntConstant(poModule, "PK_MODE_REVENGE",			PK_MODE_REVENGE);
	PyModule_AddIntConstant(poModule, "PK_MODE_FREE",				PK_MODE_FREE);
	PyModule_AddIntConstant(poModule, "PK_MODE_PROTECT",			PK_MODE_PROTECT);
	PyModule_AddIntConstant(poModule, "PK_MODE_GUILD",				PK_MODE_GUILD);
	PyModule_AddIntConstant(poModule, "PK_MODE_MAX_NUM",			PK_MODE_MAX_NUM);

	// Block Mode
	PyModule_AddIntConstant(poModule, "BLOCK_EXCHANGE",				BLOCK_EXCHANGE);
	PyModule_AddIntConstant(poModule, "BLOCK_PARTY",				BLOCK_PARTY_INVITE);
	PyModule_AddIntConstant(poModule, "BLOCK_GUILD",				BLOCK_GUILD_INVITE);
	PyModule_AddIntConstant(poModule, "BLOCK_WHISPER",				BLOCK_WHISPER);
	PyModule_AddIntConstant(poModule, "BLOCK_FRIEND",				BLOCK_MESSENGER_INVITE);
	PyModule_AddIntConstant(poModule, "BLOCK_PARTY_REQUEST",		BLOCK_PARTY_REQUEST);

	// Party
	PyModule_AddIntConstant(poModule, "PARTY_EXP_NON_DISTRIBUTION",		PARTY_EXP_DISTRIBUTION_NON_PARITY);
	PyModule_AddIntConstant(poModule, "PARTY_EXP_DISTRIBUTION_PARITY",	PARTY_EXP_DISTRIBUTION_PARITY);

	// Emotion
	PyModule_AddIntConstant(poModule, "EMOTION_CLAP",			EMOTION_CLAP);
	PyModule_AddIntConstant(poModule, "EMOTION_CHEERS_1",		EMOTION_CHEERS_1);
	PyModule_AddIntConstant(poModule, "EMOTION_CHEERS_2",		EMOTION_CHEERS_2);
	PyModule_AddIntConstant(poModule, "EMOTION_DANCE_1",		EMOTION_DANCE_1);
	PyModule_AddIntConstant(poModule, "EMOTION_DANCE_2",		EMOTION_DANCE_2);
	PyModule_AddIntConstant(poModule, "EMOTION_DANCE_3",		EMOTION_DANCE_3);
	PyModule_AddIntConstant(poModule, "EMOTION_DANCE_4",		EMOTION_DANCE_4);
	PyModule_AddIntConstant(poModule, "EMOTION_DANCE_5",		EMOTION_DANCE_5);
	PyModule_AddIntConstant(poModule, "EMOTION_DANCE_6",		EMOTION_DANCE_6);				// PSY 강남스타일
	PyModule_AddIntConstant(poModule, "EMOTION_CONGRATULATION",	EMOTION_CONGRATULATION);
	PyModule_AddIntConstant(poModule, "EMOTION_FORGIVE",		EMOTION_FORGIVE);
	PyModule_AddIntConstant(poModule, "EMOTION_ANGRY",			EMOTION_ANGRY);
	PyModule_AddIntConstant(poModule, "EMOTION_ATTRACTIVE",		EMOTION_ATTRACTIVE);
	PyModule_AddIntConstant(poModule, "EMOTION_SAD",			EMOTION_SAD);
	PyModule_AddIntConstant(poModule, "EMOTION_SHY",			EMOTION_SHY);
	PyModule_AddIntConstant(poModule, "EMOTION_CHEERUP",		EMOTION_CHEERUP);
	PyModule_AddIntConstant(poModule, "EMOTION_BANTER",			EMOTION_BANTER);
	PyModule_AddIntConstant(poModule, "EMOTION_JOY",			EMOTION_JOY);

	PyModule_AddIntConstant(poModule, "EMOTION_KISS",			EMOTION_KISS);
	PyModule_AddIntConstant(poModule, "EMOTION_FRENCH_KISS",	EMOTION_FRENCH_KISS);
	PyModule_AddIntConstant(poModule, "EMOTION_SLAP",			EMOTION_SLAP);

	//// 자동물약 타입
	PyModule_AddIntConstant(poModule, "AUTO_POTION_TYPE_HP",	CPythonPlayer::AUTO_POTION_TYPE_HP);
	PyModule_AddIntConstant(poModule, "AUTO_POTION_TYPE_SP",	CPythonPlayer::AUTO_POTION_TYPE_SP);

#ifdef ENABLE_WOLFMAN
	PyModule_AddIntConstant(poModule, "POINT_ATTBONUS_WOLFMAN",	POINT_ATTBONUS_WOLFMAN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_WOLFMAN",	POINT_RESIST_WOLFMAN);
	PyModule_AddIntConstant(poModule, "POINT_RESIST_CLAW",		POINT_RESIST_CLAW);
#endif

#ifdef ENABLE_ANIMAL_SYSTEM
	PyModule_AddIntConstant(poModule, "ANIMAL_TYPE_MOUNT",		ANIMAL_TYPE_MOUNT);
#ifdef ENABLE_PET_SYSTEM
	PyModule_AddIntConstant(poModule, "ANIMAL_TYPE_PET",		ANIMAL_TYPE_PET);
#endif

	PyModule_AddIntConstant(poModule, "ANIMAL_STAT_MAIN",		CPythonPlayer::ANIMAL_STAT_MAIN);
	PyModule_AddIntConstant(poModule, "ANIMAL_STAT_1",			CPythonPlayer::ANIMAL_STAT_1);
	PyModule_AddIntConstant(poModule, "ANIMAL_STAT_2",			CPythonPlayer::ANIMAL_STAT_2);
	PyModule_AddIntConstant(poModule, "ANIMAL_STAT_3",			CPythonPlayer::ANIMAL_STAT_3);
	PyModule_AddIntConstant(poModule, "ANIMAL_STAT_4",			CPythonPlayer::ANIMAL_STAT_4);
	PyModule_AddIntConstant(poModule, "ANIMAL_STAT_COUNT",		CPythonPlayer::ANIMAL_STAT_COUNT);
#endif

#ifdef ENABLE_DRAGONSOUL
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_PAGE_SIZE", c_DragonSoul_Inventory_Box_Size);
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_PAGE_COUNT", DRAGON_SOUL_GRADE_MAX);
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_SLOT_COUNT", c_DragonSoul_Inventory_Count);
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_EQUIPMENT_SLOT_START", DRAGON_SOUL_EQUIP_SLOT_START);
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_EQUIPMENT_PAGE_COUNT", DS_DECK_MAX_NUM);
	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_EQUIPMENT_FIRST_SIZE", c_DragonSoul_Equip_Slot_Max);

	PyModule_AddIntConstant(poModule, "DRAGON_SOUL_REFINE_CLOSE", DS_SUB_HEADER_CLOSE);
	PyModule_AddIntConstant(poModule, "DS_SUB_HEADER_DO_UPGRADE", DS_SUB_HEADER_DO_UPGRADE);
	PyModule_AddIntConstant(poModule, "DS_SUB_HEADER_DO_IMPROVEMENT", DS_SUB_HEADER_DO_IMPROVEMENT);
	PyModule_AddIntConstant(poModule, "DS_SUB_HEADER_DO_REFINE", DS_SUB_HEADER_DO_REFINE);
#endif
#ifdef ENABLE_ZODIAC
	PyModule_AddIntConstant(poModule, "ANIMASPHERE",						POINT_ANIMASPHERE);
#endif
#ifdef ENABLE_ATTRTREE
	PyModule_AddIntConstant(poModule, "ATTRTREE_ROW_NUM",	CPythonPlayer::ATTRTREE_ROW_NUM);
	PyModule_AddIntConstant(poModule, "ATTRTREE_COL_NUM",	CPythonPlayer::ATTRTREE_COL_NUM);
	PyModule_AddIntConstant(poModule, "ATTRTREE_LEVEL_NUM",	CPythonPlayer::ATTRTREE_LEVEL_NUM);
#endif

#ifdef ENABLE_COSTUME_BONUS_TRANSFER
	PyModule_AddIntConstant(poModule, "CBT_MEDIUM_ITEM_VNUM",	CBT_MEDIUM_ITEM_VNUM);
	PyModule_AddIntConstant(poModule, "CBT_SLOT_MEDIUM",		CBT_SLOT_MEDIUM);
	PyModule_AddIntConstant(poModule, "CBT_SLOT_MATERIAL",		CBT_SLOT_MATERIAL);
	PyModule_AddIntConstant(poModule, "CBT_SLOT_TARGET",		CBT_SLOT_TARGET);
	PyModule_AddIntConstant(poModule, "CBT_SLOT_RESULT",		CBT_SLOT_RESULT);
	PyModule_AddIntConstant(poModule, "CBT_SLOT_MAX",			CBT_SLOT_MAX);
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_FISH_EVENT", SLOT_TYPE_FISH_EVENT);
#endif

#ifdef ENABLE_COSTUME_INVENTORY
	PyModule_AddIntConstant(poModule, "COSTUME_INVENTORY", COSTUME_INVENTORY);
	PyModule_AddIntConstant(poModule, "COSTUME_INV_SLOT_START", COSTUME_INV_SLOT_START);
	PyModule_AddIntConstant(poModule, "COSTUME_INV_SLOT_END", COSTUME_INV_SLOT_END);
	PyModule_AddIntConstant(poModule, "SLOT_TYPE_COSTUME_INVENTORY", SLOT_TYPE_COSTUME_INVENTORY);
	PyModule_AddIntConstant(poModule, "ENABLE_COSTUME_INVENTORY", 1);
#else
	PyModule_AddIntConstant(poModule, "ENABLE_COSTUME_INVENTORY", 0);
#endif

#ifdef ENABLE_EQUIPMENT_CHANGER
	PyModule_AddIntConstant(poModule, "EQUIPMENT_PAGE_MAX_PARTS", EQUIPMENT_PAGE_MAX_PARTS);
#endif
}
