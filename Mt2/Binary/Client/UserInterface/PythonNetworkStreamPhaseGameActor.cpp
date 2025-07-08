#include "StdAfx.h"
#include "PythonNetworkStream.h"
#include "NetworkActorManager.h"
#include "PythonBackground.h"

#include "PythonApplication.h"
#include "AbstractPlayer.h"
#include "../gamelib/ActorInstance.h"

using namespace network;

void CPythonNetworkStream::__GlobalPositionToLocalPosition(long& rGlobalX, long& rGlobalY)
{
	CPythonBackground&rkBgMgr=CPythonBackground::Instance();
	rkBgMgr.GlobalPositionToLocalPosition(rGlobalX, rGlobalY);
}

void CPythonNetworkStream::__LocalPositionToGlobalPosition(long& rLocalX, long& rLocalY)
{
	CPythonBackground&rkBgMgr=CPythonBackground::Instance();
	rkBgMgr.LocalPositionToGlobalPosition(rLocalX, rLocalY);
}

short CPythonNetworkStream::__GetLevel()
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
	if (!pkInstMain)
		return 0;

	return pkInstMain->GetLevel();
}

#ifdef ENABLE_ZODIAC
bool CPythonNetworkStream::__CanActMainInstance(bool skipIsDeadItem)
#else
bool CPythonNetworkStream::__CanActMainInstance()
#endif
{
	CPythonCharacterManager& rkChrMgr=CPythonCharacterManager::Instance();
	CInstanceBase* pkInstMain=rkChrMgr.GetMainInstancePtr();
	if (!pkInstMain)
		return false;

#ifdef ENABLE_ZODIAC
	return pkInstMain->CanAct(skipIsDeadItem);
#else
	return pkInstMain->CanAct();
#endif
}

void CPythonNetworkStream::__ClearNetworkActorManager()
{
	m_rokNetActorMgr->Destroy();
}

//Å×ÀÌºí¿¡¼­ ÀÌ¸§ÀÌ "." ÀÎ °Íµé
//Â÷ÈÄ¿¡ ¼­¹ö¿¡¼­ º¸³»ÁÖÁö ¾Ê°Ô µÇ¸é ¾ø¾îÁú ÇÔ¼ö..(¼­¹ö´Ô²² ²À!!Çù¹Ú; )
bool IsInvisibleRace(WORD raceNum)
{
	switch(raceNum)
	{
	case 20025:
	case 20038:
	case 20039:
		return true;
	default:
		return false;
	}
}

static SNetworkActorData s_kNetActorData;


bool CPythonNetworkStream::RecvCharacterAppendPacket(std::unique_ptr<GCCharacterAddPacket> pack)
{
	long x = pack->x(), y = pack->y();
	__GlobalPositionToLocalPosition(x, y);

	SNetworkActorData kNetActorData;
	kNetActorData.m_bType = pack->type();
	if (pack->type() == CActorInstance::TYPE_ENEMY)
	{
		const CPythonNonPlayer::TMobTable* tbl = CPythonNonPlayer::instance().GetTable(pack->race_num());
		if (tbl && (tbl->rank() == 4 || tbl->rank() == 5)) //there is a rank 5 for KING (like dragon) (?) maybe you want to color its name too idk
			kNetActorData.m_bType = CActorInstance::TYPE_BOSS;
	}
	kNetActorData.m_dwMovSpd=pack->moving_speed();
	kNetActorData.m_dwAtkSpd=pack->attack_speed();
	kNetActorData.m_dwRace=pack->race_num();
	
	kNetActorData.m_dwStateFlags=pack->state_flag();
	kNetActorData.m_dwVID=pack->vid();
	kNetActorData.m_fRot=float(pack->angle()) * 5.0f;

	kNetActorData.m_stName="";

	DWORD aff_flag;
	aff_flag = pack->affect_flags(0);
	kNetActorData.m_kAffectFlags.CopyData(0, sizeof(aff_flag), &aff_flag);
	aff_flag = pack->affect_flags(1);
	kNetActorData.m_kAffectFlags.CopyData(32, sizeof(aff_flag), &aff_flag);
	
	kNetActorData.SetPosition(x, y);

	kNetActorData.m_iAlignment=0;/*chrAddPacket.iAlignment*/;	
	kNetActorData.m_byPKMode=0;/*chrAddPacket.bPKMode*/;	
	kNetActorData.m_dwGuildID=0;/*chrAddPacket.dwGuild*/;
	kNetActorData.m_dwEmpireID=0;/*chrAddPacket.bEmpire*/;
	kNetActorData.m_dwArmor=0;/*chrAddPacket.awPart[CHR_EQUIPPART_ARMOR]*/;
	kNetActorData.m_dwWeapon=0;/*chrAddPacket.awPart[CHR_EQUIPPART_WEAPON]*/;
	kNetActorData.m_dwHair=0;/*chrAddPacket.awPart[CHR_EQUIPPART_HAIR]*/;	
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	kNetActorData.m_dwAcce=0;
#endif
	kNetActorData.m_dwMountVnum=0;/*chrAddPacket.dwMountVnum*/;	

#ifdef __PRESTIGE__
	kNetActorData.m_bPrestigeLevel = 0;
#endif
	kNetActorData.m_dwLevel = 0; // ¸ó½ºÅÍ ·¹º§ Ç¥½Ã ¾ÈÇÔ

	if(kNetActorData.m_bType != CActorInstance::TYPE_PC && 
		kNetActorData.m_bType != CActorInstance::TYPE_NPC &&
		kNetActorData.m_bType != CActorInstance::TYPE_PET &&
		kNetActorData.m_bType != CActorInstance::TYPE_MOUNT
#ifdef ENABLE_FAKEBUFF
		&& kNetActorData.m_bType != CActorInstance::TYPE_FAKEBUFF
#endif
	)
	{
		const char * c_szName;
		CPythonNonPlayer& rkNonPlayer=CPythonNonPlayer::Instance();
		if (rkNonPlayer.GetName(kNetActorData.m_dwRace, &c_szName))
			kNetActorData.m_stName = c_szName;
		//else
		//	kNetActorData.m_stName=chrAddPacket.name;

		__RecvCharacterAppendPacket(&kNetActorData);
	}
	else
	{
		s_kNetActorData = kNetActorData;
	}

	return true;
}

bool CPythonNetworkStream::RecvCharacterAdditionalInfo(std::unique_ptr<GCCharacterAdditionalInfoPacket> pack)
{
	SNetworkActorData kNetActorData = s_kNetActorData;
	if (IsInvisibleRace(kNetActorData.m_dwRace))
		return true;

	if(kNetActorData.m_dwVID == pack->vid())
	{
		kNetActorData.m_stName = pack->name();
		kNetActorData.m_dwGuildID = pack->guild_id();
		kNetActorData.m_dwLevel = pack->level();
		kNetActorData.m_iAlignment=pack->alignment();	
		kNetActorData.m_byPKMode=pack->pk_mode();	
		kNetActorData.m_dwEmpireID=pack->empire();
		kNetActorData.m_dwArmor=pack->parts(CHR_EQUIPPART_ARMOR);
		kNetActorData.m_dwWeapon=pack->parts(CHR_EQUIPPART_WEAPON);
		kNetActorData.m_dwHair=pack->parts(CHR_EQUIPPART_HAIR);	
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
		kNetActorData.m_dwAcce=pack->parts(CHR_EQUIPPART_ACCE);
#endif
#ifdef ENABLE_ALPHA_EQUIP
		kNetActorData.m_iWeaponAlphaEquip=pack->weapon_alpha_value();
#endif
		kNetActorData.m_dwMountVnum=pack->mount_vnum();
		kNetActorData.m_sPVPTeam=pack->pvp_team();

#ifdef COMBAT_ZONE
		kNetActorData.combat_zone_rank = pack->combat_zone_rank();
#endif

#ifdef CHANGE_SKILL_COLOR
		for (auto skill_index = 0; skill_index < ESkillColorLength::MAX_SKILL_COUNT; ++skill_index)
		{
			for (auto effect_index = 0; effect_index < ESkillColorLength::MAX_EFFECT_COUNT; ++effect_index)
			{
				auto recv_id = skill_index * ESkillColorLength::MAX_EFFECT_COUNT + effect_index;
				kNetActorData.m_dwSkillColor[skill_index][effect_index] = pack->skill_colors(recv_id);
			}
		}
#endif

#ifdef __PRESTIGE__
		kNetActorData.m_bPrestigeLevel=chrInfoPacket.bPrestigeLevel;
#endif
		__RecvCharacterAppendPacket(&kNetActorData);
	}
	else
	{
		TraceError("TPacketGCCharacterAdditionalInfo name=%s vid=%d race=%d Error",pack->name().c_str(),pack->vid(),kNetActorData.m_dwRace);
	}
	return true;
}

bool CPythonNetworkStream::RecvCharacterUpdatePacket(std::unique_ptr<GCCharacterUpdatePacket> pack)
{
	SNetworkUpdateActorData kNetUpdateActorData;
	kNetUpdateActorData.m_dwGuildID=pack->guild_id();
	kNetUpdateActorData.m_dwMovSpd=pack->moving_speed();
	kNetUpdateActorData.m_dwAtkSpd=pack->attack_speed();
	kNetUpdateActorData.m_dwArmor=pack->parts(CHR_EQUIPPART_ARMOR);
	kNetUpdateActorData.m_dwWeapon=pack->parts(CHR_EQUIPPART_WEAPON);
	kNetUpdateActorData.m_dwHair=pack->parts(CHR_EQUIPPART_HAIR);
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	kNetUpdateActorData.m_dwAcce=pack->parts(CHR_EQUIPPART_ACCE);
#endif
#ifdef ENABLE_ALPHA_EQUIP
	kNetUpdateActorData.m_iWeaponAlphaEquip=pack->weapon_alpha_value();
#endif
	kNetUpdateActorData.m_dwVID=pack->vid();

	DWORD aff_flag;
	aff_flag = pack->affect_flags(0);
	kNetUpdateActorData.m_kAffectFlags.CopyData(0, sizeof(aff_flag), &aff_flag);
	aff_flag = pack->affect_flags(1);
	kNetUpdateActorData.m_kAffectFlags.CopyData(32, sizeof(aff_flag), &aff_flag);

	kNetUpdateActorData.m_iAlignment=pack->alignment();
	kNetUpdateActorData.m_byPKMode=pack->pk_mode();
	kNetUpdateActorData.m_dwStateFlags=pack->state_flag();
	kNetUpdateActorData.m_dwMountVnum=pack->mount_vnum();

#ifdef COMBAT_ZONE
	kNetUpdateActorData.combat_zone_points = pack->combat_zone_points();
#endif

#ifdef CHANGE_SKILL_COLOR
	for (auto skill_index = 0; skill_index < ESkillColorLength::MAX_SKILL_COUNT; ++skill_index)
	{
		for (auto effect_index = 0; effect_index < ESkillColorLength::MAX_EFFECT_COUNT; ++effect_index)
		{
			auto recv_id = skill_index * ESkillColorLength::MAX_EFFECT_COUNT + effect_index;
			kNetUpdateActorData.m_dwSkillColor[skill_index][effect_index] = pack->skill_colors(recv_id);
		}
	}
#endif

#ifdef __PRESTIGE__
	kNetUpdateActorData.m_bPrestigeLevel=pack->prestige_level();
#endif
	
	CInstanceBase * inst = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	if (inst)
		inst->DetachShining();

	__RecvCharacterUpdatePacket(&kNetUpdateActorData);

	return true;
}

bool CPythonNetworkStream::RecvShiningPacket(std::unique_ptr<GCCharacterShiningPacket> pack)
{
	CInstanceBase * inst = CPythonCharacterManager::Instance().GetInstancePtr(pack->vid());
	if (!inst)
		return true;

	for (auto i = 0; i < SHINING_MAX_NUM; ++i)
		if (pack->shinings(i))
			inst->AttachShiningEffect(pack->shinings(i) - 1);

	return true;
}

void CPythonNetworkStream::__RecvCharacterAppendPacket(SNetworkActorData * pkNetActorData)
{
	// NOTE : Ä«¸Þ¶ó°¡ ¶¥¿¡ ¹¯È÷´Â ¹®Á¦ÀÇ ÇØ°áÀ» À§ÇØ ¸ÞÀÎ Ä³¸¯ÅÍ°¡ ÁöÇü¿¡ ¿Ã·ÁÁö±â
	//        Àü¿¡ ¸ÊÀ» ¾÷µ¥ÀÌÆ® ÇØ ³ôÀÌ¸¦ ±¸ÇÒ ¼ö ÀÖµµ·Ï ÇØ³õ¾Æ¾ß ÇÕ´Ï´Ù.
	//        ´Ü, °ÔÀÓÀÌ µé¾î°¥¶§°¡ ¾Æ´Ñ ÀÌ¹Ì Ä³¸¯ÅÍ°¡ Ãß°¡ µÈ ÀÌÈÄ¿¡¸¸ ÇÕ´Ï´Ù.
	//        Çåµ¥ ÀÌµ¿ÀÎµ¥ ¿Ö Move·Î ¾ÈÇÏ°í Append·Î ÇÏ´ÂÁö..? - [levites]
	IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();
	if (rkPlayer.IsMainCharacterIndex(pkNetActorData->m_dwVID))
	{
		rkPlayer.SetRace(pkNetActorData->m_dwRace);

		if (rkPlayer.NEW_GetMainActorPtr())
		{
			CPythonBackground::Instance().Update(pkNetActorData->m_lCurX, pkNetActorData->m_lCurY, 0.0f);
			CPythonCharacterManager::Instance().Update();

			// NOTE : »ç±Í Å¸¿öÀÏ °æ¿ì GOTO ·Î ÀÌµ¿½Ã¿¡µµ ¸Ê ÀÌ¸§À» Ãâ·ÂÇÏµµ·Ï Ã³¸®
			{
				std::string strMapName = CPythonBackground::Instance().GetWarpMapName();
				if (strMapName == "metin2_map_deviltower1")
					__ShowMapName(pkNetActorData->m_lCurX, pkNetActorData->m_lCurY);
			}
		}
		else
		{
			__ShowMapName(pkNetActorData->m_lCurX, pkNetActorData->m_lCurY);
		}
	}

	m_rokNetActorMgr->AppendActor(*pkNetActorData);

	if (GetMainActorVID()==pkNetActorData->m_dwVID)
	{
		rkPlayer.SetTarget(0);
		if (m_bComboSkillFlag)
			rkPlayer.SetComboSkillFlag(m_bComboSkillFlag);

		__SetGuildID(pkNetActorData->m_dwGuildID);
		//CPythonApplication::Instance().SkipRenderBuffering(10000);

		CPythonPlayer::Instance().SetPVPTeam(pkNetActorData->m_sPVPTeam);
	}
	else
		CPythonCharacterManager::Instance().SetPVPTeam(pkNetActorData->m_dwVID, pkNetActorData->m_sPVPTeam);
}

void CPythonNetworkStream::__RecvCharacterUpdatePacket(SNetworkUpdateActorData * pkNetUpdateActorData)
{
	m_rokNetActorMgr->UpdateActor(*pkNetUpdateActorData);

	IAbstractPlayer& rkPlayer = IAbstractPlayer::GetSingleton();
	if (rkPlayer.IsMainCharacterIndex(pkNetUpdateActorData->m_dwVID))
	{
		__SetGuildID(pkNetUpdateActorData->m_dwGuildID);

		__RefreshStatus();
		__RefreshAlignmentWindow();
		__RefreshEquipmentWindow();
		__RefreshInventoryWindow();
		m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_hair_part(pkNetUpdateActorData->m_dwHair);
		m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_main_part(pkNetUpdateActorData->m_dwArmor);
		m_akSimplePlayerInfo[m_dwSelectedCharacterIndex].set_acce_part(pkNetUpdateActorData->m_dwAcce);
	}
	else
	{
		rkPlayer.NotifyCharacterUpdate(pkNetUpdateActorData->m_dwVID);
	}
}

bool CPythonNetworkStream::RecvCharacterDeletePacket(std::unique_ptr<GCCharacterDeletePacket> pack)
{
	m_rokNetActorMgr->RemoveActor(pack->vid());

	// Ä³¸¯ÅÍ°¡ »ç¶óÁú¶§ °³ÀÎ »óÁ¡µµ ¾ø¾ÖÁÝ´Ï´Ù.
	// Key Check ¸¦ ÇÏ±â¶§¹®¿¡ ¾ø¾îµµ »ó°üÀº ¾ø½À´Ï´Ù.
	PyCallClassMemberFunc(m_apoPhaseWnd[PHASE_WINDOW_GAME], 
		"BINARY_PrivateShop_Disappear", 
		Py_BuildValue("(i)", pack->vid())
	);

	return true;
}


bool CPythonNetworkStream::RecvCharacterMovePacket(std::unique_ptr<GCMovePacket> pack)
{
	long lX = pack->x(), lY = pack->y();
	__GlobalPositionToLocalPosition(lX, lY);

	SNetworkMoveActorData kNetMoveActorData;
	kNetMoveActorData.m_dwArg=pack->arg();
	kNetMoveActorData.m_dwFunc=pack->func();
	kNetMoveActorData.m_dwTime=pack->time();
	kNetMoveActorData.m_dwVID=pack->vid();
	kNetMoveActorData.m_fRot=pack->rot()*5.0f;
	kNetMoveActorData.m_lPosX=lX;
	kNetMoveActorData.m_lPosY=lY;
	kNetMoveActorData.m_dwDuration=pack->duration();

	m_rokNetActorMgr->MoveActor(kNetMoveActorData);

	return true;
}

bool CPythonNetworkStream::RecvOwnerShipPacket(std::unique_ptr<GCOwnershipPacket> pack)
{
	m_rokNetActorMgr->SetActorOwner(pack->owner_vid(), pack->victim_vid());

	return true;
}

bool CPythonNetworkStream::RecvSyncPositionPacket(std::unique_ptr<GCSyncPositionPacket> pack)
{
	for (auto& elem : pack->elements())
	{
#ifdef __MOVIE_MODE__
		return true;
#endif __MOVIE_MODE__

		//Tracenf("CPythonNetworkStream::RecvSyncPositionPacket %d (%d, %d)", kSyncPos.dwVID, kSyncPos.lX, kSyncPos.lY);

		long lX = elem.x(), lY = elem.y();
		__GlobalPositionToLocalPosition(lX, lY);
		m_rokNetActorMgr->SyncActor(elem.vid(), lX, lY);

		/*
		CPythonCharacterManager & rkChrMgr = CPythonCharacterManager::Instance();
		CInstanceBase * pkChrInst = rkChrMgr.GetInstancePtr(kSyncPos.dwVID);

		if (pkChrInst)
		{			
			pkChrInst->NEW_SyncPixelPosition(lX, lY);		
		}
		*/
}

	return true;
}


/*
bool CPythonNetworkStream::RecvCharacterAppendPacket()
{
	TPacketGCCharacterAdd chrAddPacket;

	if (!Recv(&chrAddPacket))
		return false;

	__GlobalPositionToLocalPosition(chrAddPacket.lX, chrAddPacket.lY);

	SNetworkActorData kNetActorData;
	kNetActorData.m_dwGuildID=chrAddPacket.dwGuild;
	kNetActorData.m_dwEmpireID=chrAddPacket.bEmpire;
	kNetActorData.m_dwMovSpd=chrAddPacket.bMovingSpeed;
	kNetActorData.m_dwAtkSpd=chrAddPacket.bAttackSpeed;
	kNetActorData.m_dwRace=chrAddPacket.wRaceNum;
	kNetActorData.m_dwShape=chrAddPacket.parts[CRaceData::PART_MAIN];
	kNetActorData.m_dwStateFlags=chrAddPacket.bStateFlag;
	kNetActorData.m_dwVID=chrAddPacket.dwVID;
	kNetActorData.m_dwWeapon=chrAddPacket.parts[CRaceData::PART_WEAPON];
	kNetActorData.m_fRot=chrAddPacket.angle;
	kNetActorData.m_kAffectFlags.CopyData(0, sizeof(chrAddPacket.dwAffectFlag[0]), &chrAddPacket.dwAffectFlag[0]);
	kNetActorData.m_kAffectFlags.CopyData(32, sizeof(chrAddPacket.dwAffectFlag[1]), &chrAddPacket.dwAffectFlag[1]);
	kNetActorData.SetPosition(chrAddPacket.lX, chrAddPacket.lY);
	kNetActorData.m_stName=chrAddPacket.name;
	__RecvCharacterAppendPacket(&kNetActorData);
	return true;
}
*/

