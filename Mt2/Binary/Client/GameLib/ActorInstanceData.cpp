#include "StdAfx.h"
#include "ActorInstance.h"
#include "RaceManager.h"
#include "ItemManager.h"
#include "RaceData.h"

#include "../eterlib/ResourceManager.h"
#include "../etergrnlib/util.h"

#ifdef CHANGE_SKILL_COLOR
#include "../UserInterface/InstanceBase.h"
#include "../UserInterface/PythonSkill.h"

DWORD *CActorInstance::GetSkillColorByEffectID(DWORD id)
{
	switch (id)
	{
	case 14:																	//FLY_CHAIN_LIGHTNING =>Blitzkralle
		return m_dwSkillColor[108];
		break;

	case 16:																	//FLY_SKILL_MUYEONG => GDF
		return m_dwSkillColor[78];
		break;
		/////////////////////////////////////////////
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_CHEONGEUN:		//STARKER KÖRPER
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_FALLEN_CHEONGEUN:
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_CHEONGEUN_PERFECT:
#endif
		return m_dwSkillColor[19];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GYEONGGONG:		//FEDERSCHREITEN
		return m_dwSkillColor[49];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GWIGEOM:			//VERZAUBERTE KLINGE
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GWIGEOM_PERFECT:			//VERZAUBERTE KLINGE
#endif
		return m_dwSkillColor[63];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GONGPO:			//FURCHT
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GONGPO_PERFECT:			//FURCHT
#endif
		return m_dwSkillColor[64];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_JUMAGAP:			//VERZAUBERTE RÜSTUNG
		return m_dwSkillColor[65];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_HOSIN:			//SEGEN
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_HOSIN_PERFECT:			//SEGEN
#endif
		return m_dwSkillColor[94];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_BOHO:				//REFLEKTIEREN
		return m_dwSkillColor[95];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_KWAESOK:			//SCHNELLIGKEIT
		return m_dwSkillColor[110];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_HEUKSIN:			//DUNKLER SCHUTZ
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_HEUKSIN_PERFECT:			//DUNKLER SCHUTZ
#endif
		return m_dwSkillColor[79];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_MUYEONG:			//GDF
		return m_dwSkillColor[78];

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_FIRE:
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GICHEON:			//HDD
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_GICHEON_PERFECT:			//HDD
#endif
		return m_dwSkillColor[96];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_JEUNGRYEOK:		//ANGRIFF+
		return m_dwSkillColor[111];
		break;

	case CInstanceBase::EFFECT_AFFECT + CInstanceBase::AFFECT_PABEOP:			//ZA
		return m_dwSkillColor[66];
		break;
		/////////////////////////////////////////////
	case CInstanceBase::EFFECT_WEAPON + CInstanceBase::WEAPON_ONEHAND:			//AURA 1Hand
	case CInstanceBase::EFFECT_WEAPON + CInstanceBase::WEAPON_TWOHAND:			//AURA 2Hand
#ifdef ENABLE_LEGENDARY_SKILL
	case CInstanceBase::EFFECT_WEAPON_LEGENDARY + CInstanceBase::WEAPON_ONEHAND:
	case CInstanceBase::EFFECT_WEAPON_LEGENDARY + CInstanceBase::WEAPON_TWOHAND:
#endif
		return m_dwSkillColor[4];
		break;
	}
	DWORD motion_index = GET_MOTION_INDEX(id);
	for (int i = 0; i < CPythonSkill::SKILL_EFFECT_COUNT; ++i)
	{
		for (int x = 0; x < MAX_SKILL_COUNT - 3; ++x)
		{
			if ((motion_index == ((CRaceMotionData::NAME_SKILL + x + 1) + (i * 25))) || (motion_index == ((CRaceMotionData::NAME_SKILL + 15 + x + 1) + (i * 25))))
				return m_dwSkillColor[(motion_index - CRaceMotionData::NAME_SKILL) - (i * 25)];
		}
	}
	return NULL;
}
#endif

DWORD CActorInstance::GetVirtualID()
{
	return m_dwSelfVID;
}

void CActorInstance::SetVirtualID(DWORD dwVID)
{
	m_dwSelfVID=dwVID;
}

void CActorInstance::UpdateAttribute()
{
	if (!m_pAttributeInstance)
		return;

	if (!m_bNeedUpdateCollision)
		return;

	m_bNeedUpdateCollision = FALSE;

	const CStaticCollisionDataVector & c_rkVec_ColliData = m_pAttributeInstance->GetObjectPointer()->GetCollisionDataVector();
	UpdateCollisionData(&c_rkVec_ColliData);

	m_pAttributeInstance->RefreshObject(GetTransform());
	UpdateHeightInstance(m_pAttributeInstance);

	//BOOL isHeightData = m_pAttributeInstance->IsEmpty();
}

void CActorInstance::__CreateAttributeInstance(CAttributeData * pData)
{
	m_pAttributeInstance = CAttributeInstance::New();
	m_pAttributeInstance->Clear();
	m_pAttributeInstance->SetObjectPointer(pData);
	if (pData->IsEmpty())
	{
		m_pAttributeInstance->Clear();
		CAttributeInstance::Delete(m_pAttributeInstance);
	}
}

DWORD CActorInstance::GetRace()
{
	return m_eRace;
}

bool CActorInstance::SetRace(DWORD eRace)
{
	CRaceData * pRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(eRace, &pRaceData))
	{
		m_eRace=0;
		m_pkCurRaceData=NULL;
		return false;
	}

	m_eRace=eRace;
	m_pkCurRaceData = pRaceData;
	CGraphicThingInstance::SetMotionID(eRace);

	CAttributeData * pAttributeData = pRaceData->GetAttributeDataPtr();
	if (pAttributeData)
	{
		__CreateAttributeInstance(pAttributeData);
	}

	memset(m_adwPartItemID, 0, sizeof(m_adwPartItemID));

	// Setup Graphic ResourceData
	__ClearAttachingEffect();

	CGraphicThingInstance::Clear();

	//NOTE : PC¸¸ Partº°·Î ´Ù »ý¼ºÇÏ°Ô ÇØÁØ´Ù.
	if(IsPC()
#ifdef ENABLE_FAKEBUFF
		|| IsFakeBuff()
#endif
	)
	{
		CGraphicThingInstance::ReserveModelThing(CRaceData::PART_MAX_NUM);
		CGraphicThingInstance::ReserveModelInstance(CRaceData::PART_MAX_NUM);
	}
	else
	{
		CGraphicThingInstance::ReserveModelThing(1);
		CGraphicThingInstance::ReserveModelInstance(1);
	}

	pRaceData->LoadMotions();

	return true;
}

void CActorInstance::SetHair(DWORD eHair)
{
#ifdef ASSASINE_COSTUME_DISABLE_HAIR
	if (m_eShape == ASSASINE_COSTUME_DISABLE_HAIR)
		return;
#endif
	m_eHair = eHair;

	CRaceData * pRaceData;

	if (!CRaceManager::Instance().GetRaceDataPointer(m_eRace, &pRaceData))
		return;

	CRaceData::SHair* pkHair=pRaceData->FindHair(eHair);
	if (pkHair)
	{
		if (!pkHair->m_stModelFileName.empty())
		{
			CGraphicThing * pkHairThing = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer(pkHair->m_stModelFileName.c_str());
			RegisterModelThing(CRaceData::PART_HAIR, pkHairThing);
			SetModelInstance(CRaceData::PART_HAIR, CRaceData::PART_HAIR, 0, CRaceData::PART_MAIN);
		}
#ifdef ENABLE_HAIR_SPECULAR
		float fSpecularPower = 0.0f;
		
		switch(eHair)
		{
			case 451:
			case 452:
				fSpecularPower = 1.0f;
				break;
			case 702:
			case 703:
			case 481:
			case 482:
			case 483:
			case 484:
			case 723:
				fSpecularPower = 0.75f;
				break;
			case 706:
			case 707:
				fSpecularPower = 0.5f;
				break;
			default:
				break;
		}

		const std::vector<CRaceData::SSkin>& c_rkVct_kSkin = pkHair->m_kVct_kSkin;
		std::vector<CRaceData::SSkin>::const_iterator i;
		for (i = c_rkVct_kSkin.begin(); i != c_rkVct_kSkin.end(); ++i)
		{
			const CRaceData::SSkin& c_rkSkinItem = *i;
			
			CResource * pkRes = CResourceManager::Instance().GetResourcePointer(c_rkSkinItem.m_stDstFileName.c_str());

			if (pkRes)
			{
				if (fSpecularPower > 0.0f)
				{
					SMaterialData kMaterialData;
					kMaterialData.pImage = static_cast<CGraphicImage*>(pkRes);
					kMaterialData.isSpecularEnable = TRUE;
					kMaterialData.fSpecularPower = fSpecularPower;
					kMaterialData.bSphereMapIndex = 0;
	 				SetMaterialData(CRaceData::PART_HAIR, c_rkSkinItem.m_stSrcFileName.c_str(), kMaterialData);
				}
				else
				{
					SetMaterialImagePointer(CRaceData::PART_HAIR, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
				}
			}
		}
#else
		SetMaterialImagePointer(CRaceData::PART_HAIR, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
#endif
	}
}



void CActorInstance::SetShape(DWORD eShape, float fSpecular)
{
	m_eShape = eShape;

	CRaceData * pRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(m_eRace, &pRaceData))
		return;



	CRaceData::SShape* pkShape=pRaceData->FindShape(eShape);
	if (pkShape)
	{
		CResourceManager& rkResMgr=CResourceManager::Instance();
		
		if (pkShape->m_stModelFileName.empty())
		{
			CGraphicThing* pModelThing = pRaceData->GetBaseModelThing();
			RegisterModelThing(0, pModelThing);
		}
		else
		{
			CGraphicThing* pModelThing = (CGraphicThing *)rkResMgr.GetResourcePointer(pkShape->m_stModelFileName.c_str());
			RegisterModelThing(0, pModelThing);
		}		

#ifndef __DISABLE_LOD_LOADING__
		{
			std::string stLODModelFileName;

			char szLODModelFileNameEnd[256];
			for (UINT uLODIndex=1; uLODIndex<=3; ++uLODIndex)
			{
				sprintf(szLODModelFileNameEnd, "_lod_%.2d.gr2", uLODIndex);
				stLODModelFileName = CFileNameHelper::NoExtension(pkShape->m_stModelFileName) + szLODModelFileNameEnd;
				if (!rkResMgr.IsFileExist(stLODModelFileName.c_str()))
					break;
				
				CGraphicThing* pLODModelThing = (CGraphicThing *)rkResMgr.GetResourcePointer(stLODModelFileName.c_str());
				if (!pLODModelThing)
					break;

				RegisterLODThing(0, pLODModelThing);
			}
		}
#endif

		SetModelInstance(0, 0, 0);

		if (IsNPC() || IsMount() || IsPet())
			fSpecular = CRaceManager::Instance().GetRaceSpecular(GetRace());

		const std::vector<CRaceData::SSkin>& c_rkVct_kSkin = pkShape->m_kVct_kSkin;
		std::vector<CRaceData::SSkin>::const_iterator i;
		for (i = c_rkVct_kSkin.begin(); i != c_rkVct_kSkin.end(); ++i)
		{
			const CRaceData::SSkin& c_rkSkinItem = *i;

			CResource * pkRes = CResourceManager::Instance().GetResourcePointer(c_rkSkinItem.m_stDstFileName.c_str());

			if (pkRes)
			{
				if (fSpecular > 0.0f)
				{
					SMaterialData kMaterialData;
					kMaterialData.pImage = static_cast<CGraphicImage*>(pkRes);
					kMaterialData.isSpecularEnable = TRUE;
					kMaterialData.fSpecularPower = fSpecular;
					kMaterialData.bSphereMapIndex = 0;
	 				SetMaterialData(c_rkSkinItem.m_ePart, c_rkSkinItem.m_stSrcFileName.c_str(), kMaterialData);
				}
				else
				{
	 				SetMaterialImagePointer(c_rkSkinItem.m_ePart, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
				}
			}
		}
	}
	else
	{
		if (pRaceData->IsTree())
		{
			__CreateTree(pRaceData->GetTreeFileName());
		}
		else
		{
			CGraphicThing* pModelThing = pRaceData->GetBaseModelThing();
			RegisterModelThing(0, pModelThing);

#ifdef LOD_ERROR_FIX
			CGraphicThing* pLODModelThing = pRaceData->GetLODModelThing();

			bool canLOD = true;
			if (pModelThing && pLODModelThing) {
				if (pModelThing->GetTextureCount() == pLODModelThing->GetTextureCount()) {
					for (int i = 0; i < pModelThing->GetTextureCount(); i++) {
						if (strcmp(pModelThing->GetTexturePath(i), pLODModelThing->GetTexturePath(i)) != 0)
							canLOD = false;
					}
				}
				else {
					canLOD = false;
				}
			}

			if (canLOD && false)
				RegisterLODThing(0, pLODModelThing);
#else
			CGraphicThing* pLODModelThing = pRaceData->GetLODModelThing();
			RegisterLODThing(0, pLODModelThing);
#endif

			SetModelInstance(0, 0, 0);
		}
	}

	// Attaching Objects
	for (DWORD i = 0; i < pRaceData->GetAttachingDataCount(); ++i)
	{
		const NRaceData::TAttachingData * c_pAttachingData;
		if (!pRaceData->GetAttachingDataPointer(i, &c_pAttachingData))
			continue;

		switch (c_pAttachingData->dwType)
		{
			case NRaceData::ATTACHING_DATA_TYPE_EFFECT:
				if (c_pAttachingData->isAttaching)
				{
					AttachEffectByName(0, c_pAttachingData->strAttachingBoneName.c_str(), c_pAttachingData->pEffectData->strFileName.c_str());
				}
				else
				{
					AttachEffectByName(0, 0, c_pAttachingData->pEffectData->strFileName.c_str());
				}
				break;
		}
	}
}

void CActorInstance::ChangeMaterial(const char * c_szFileName)
{
	CRaceData * pRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(m_eRace, &pRaceData))
		return;

	CRaceData::SShape* pkShape=pRaceData->FindShape(m_eShape);
	if (!pkShape)
		return;

	const std::vector<CRaceData::SSkin>& c_rkVct_kSkin = pkShape->m_kVct_kSkin;
	if (c_rkVct_kSkin.empty())
		return;

	std::vector<CRaceData::SSkin>::const_iterator i = c_rkVct_kSkin.begin();
	const CRaceData::SSkin& c_rkSkinItem = *i;

	std::string dstFileName = "d:/ymir work/npc/guild_symbol/guild_symbol.dds";
	dstFileName = c_szFileName;

	CResource * pkRes = CResourceManager::Instance().GetResourcePointer(dstFileName.c_str());
	if (!pkRes)
		return;

 	SetMaterialImagePointer(c_rkSkinItem.m_ePart, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
}
/*
void CActorInstance::SetPart(DWORD dwPartIndex, DWORD dwItemID)
{
	if (dwPartIndex>=CRaceData::PART_MAX_NUM)
		return;

	if (!m_pkCurRaceData)
	{
		assert(m_pkCurRaceData);
		return;
	}

	CItemData * pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwItemID, &pItemData))
		return;

	RegisterModelThing(dwPartIndex, pItemData->GetModelThing());
	for (DWORD i = 0; i < pItemData->GetLODModelThingCount(); ++i)
	{
		CGraphicThing * pThing;
		if (!pItemData->GetLODModelThingPointer(i, &pThing))
			continue;

		RegisterLODThing(dwPartIndex, pThing);
	}
	SetModelInstance(dwPartIndex, dwPartIndex, 0);

	m_adwPartItemID[dwPartIndex] = dwItemID;
}
*/

DWORD CActorInstance::GetPartItemID(DWORD dwPartIndex)
{
	if (dwPartIndex>=CRaceData::PART_MAX_NUM)
	{
		TraceError("CActorInstance::GetPartIndex(dwPartIndex=%d/CRaceData::PART_MAX_NUM=%d)", dwPartIndex, CRaceData::PART_MAX_NUM);
		return 0;
	}
	
	return m_adwPartItemID[dwPartIndex];
}

void CActorInstance::SetSpecularInfo(BOOL bEnable, int iPart, float fAlpha)
{
	CRaceData * pkRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(m_eRace, &pkRaceData))
		return;

	CRaceData::SShape * pkShape = pkRaceData->FindShape(m_eShape);
	if (pkShape->m_kVct_kSkin.empty())
		return;

	std::string filename = pkShape->m_kVct_kSkin[0].m_stSrcFileName.c_str();
	CFileNameHelper::ChangeDosPath(filename);

	CGraphicThingInstance::SetSpecularInfo(iPart, filename.c_str(), bEnable, fAlpha);
}

void CActorInstance::SetSpecularInfoForce(BOOL bEnable, int iPart, float fAlpha)
{
	CGraphicThingInstance::SetSpecularInfo(iPart, NULL, bEnable, fAlpha);
}
