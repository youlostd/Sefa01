#include "stdafx.h"
#include "../eterlib/GrpMath.h"
#include "../gamelib/ItemManager.h"
#include "../EffectLib/EffectManager.h"
#include "PythonBackground.h"

#include "pythonitem.h"
#include "PythonTextTail.h"
#include "PythonConfig.h"
#include "PythonNonPlayer.h"
#include "PythonSkill.h"

const float c_fDropStartHeight = 100.0f;
const float c_fDropTime = 0.5f;

std::string CPythonItem::TGroundItemInstance::ms_astDropSoundFileName[DROPSOUND_NUM];

#ifdef ENABLE_ALPHA_EQUIP
const int c_iAlphaEquipBit = 31; // 31. bit -> use it as is_alpha_weapon bool; 32. bit is +/- and all bits before the 31. are used for the AlphaEquipValue

int Item_ExtractAlphaEquipValue(int iVal)
{
	static const int cs_iRemoveBitFlag = ~(1 << (c_iAlphaEquipBit - 1));
	return iVal & cs_iRemoveBitFlag;
}

bool Item_ExtractAlphaEquip(int iVal)
{
	return IS_SET(iVal, c_iAlphaEquipBit);
}
#endif

void CPythonItem::GetInfo(std::string* pstInfo)
{
	char szInfo[256];
	sprintf(szInfo, "Item: Inst %d, Pool %d", m_GroundItemInstanceMap.size(), m_GroundItemInstancePool.GetCapacity());

	pstInfo->append(szInfo);
}

void CPythonItem::TGroundItemInstance::Clear()
{
	stOwnership = "";
	ThingInstance.Clear();
	CEffectManager::Instance().DestroyEffectInstance(dwEffectInstanceIndex);
}

void CPythonItem::TGroundItemInstance::__PlayDropSound(DWORD eItemType, const D3DXVECTOR3& c_rv3Pos)
{
	if (eItemType>=DROPSOUND_NUM)
		return;

	CSoundManager::Instance().PlaySound3D(c_rv3Pos.x, c_rv3Pos.y, c_rv3Pos.z, ms_astDropSoundFileName[eItemType].c_str());
}

bool CPythonItem::TGroundItemInstance::Update()
{
	if (bAnimEnded)
		return false;
	if (dwEndTime < CTimer::Instance().GetCurrentMillisecond())
	{
		ThingInstance.SetRotationQuaternion(qEnd);

		/*D3DXVECTOR3 v3Adjust = -v3Center;
		D3DXMATRIX mat;
		D3DXMatrixRotationYawPitchRoll(&mat, 
		D3DXToRadian(rEnd.y), 
		D3DXToRadian(rEnd.x), 
		D3DXToRadian(rEnd.z));
		D3DXVec3TransformCoord(&v3Adjust,&v3Adjust,&mat);*/

		D3DXQUATERNION qAdjust(-v3Center.x, -v3Center.y, -v3Center.z, 0.0f);
		D3DXQUATERNION qc;
		D3DXQuaternionConjugate(&qc, &qEnd);
		D3DXQuaternionMultiply(&qAdjust,&qAdjust,&qEnd);
		D3DXQuaternionMultiply(&qAdjust,&qc,&qAdjust);

		ThingInstance.SetPosition(v3EndPosition.x+qAdjust.x, 
			v3EndPosition.y+qAdjust.y,
			v3EndPosition.z+qAdjust.z);
		//ThingInstance.Update();
		bAnimEnded = true;

		__PlayDropSound(eDropSoundType, v3EndPosition);
	}
	else
	{
		DWORD time = CTimer::Instance().GetCurrentMillisecond() - dwStartTime;
		DWORD etime = dwEndTime - CTimer::Instance().GetCurrentMillisecond();
		float rate = time * 1.0f / (dwEndTime - dwStartTime);

		D3DXVECTOR3 v3NewPosition=v3EndPosition;// = rate*(v3EndPosition - v3StartPosition) + v3StartPosition;
		v3NewPosition.z += 100-100*rate*(3*rate-2);//-100*(rate-1)*(3*rate+2);

		D3DXQUATERNION q;
		D3DXQuaternionRotationAxis(&q, &v3RotationAxis, etime * 0.03f *(-1+rate*(3*rate-2)));
		//ThingInstance.SetRotation(rEnd.y + etime*rStart.y, rEnd.x + etime*rStart.x, rEnd.z + etime*rStart.z);
		D3DXQuaternionMultiply(&q,&qEnd,&q);

		ThingInstance.SetRotationQuaternion(q);
		D3DXQUATERNION qAdjust(-v3Center.x, -v3Center.y, -v3Center.z, 0.0f);
		D3DXQUATERNION qc;
		D3DXQuaternionConjugate(&qc, &q);
		D3DXQuaternionMultiply(&qAdjust,&qAdjust,&q);
		D3DXQuaternionMultiply(&qAdjust,&qc,&qAdjust);
		
		ThingInstance.SetPosition(v3NewPosition.x+qAdjust.x, 
			v3NewPosition.y+qAdjust.y,
			v3NewPosition.z+qAdjust.z);
		
		/*D3DXVECTOR3 v3Adjust = -v3Center;
		D3DXMATRIX mat;
		D3DXMatrixRotationYawPitchRoll(&mat, 
		D3DXToRadian(rEnd.y + etime*rStart.y), 
		D3DXToRadian(rEnd.x + etime*rStart.x), 
		D3DXToRadian(rEnd.z + etime*rStart.z));
						
		D3DXVec3TransformCoord(&v3Adjust,&v3Adjust,&mat);
		//Tracef("%f %f %f\n",v3Adjust.x,v3Adjust.y,v3Adjust.z);
		v3NewPosition += v3Adjust;
		ThingInstance.SetPosition(v3NewPosition.x, v3NewPosition.y, v3NewPosition.z);*/
	}
	ThingInstance.Transform();
	ThingInstance.Deform();				
	return !bAnimEnded;
}

void CPythonItem::Update(const POINT& c_rkPtMouse)
{
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.begin();
	for(; itor != m_GroundItemInstanceMap.end(); ++itor)
	{
		itor->second->Update();
	}

	m_dwPickedItemID=__Pick(c_rkPtMouse);
}

void CPythonItem::Render()
{
	CPythonGraphic::Instance().SetDiffuseOperation();
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.begin();
	for (; itor != m_GroundItemInstanceMap.end(); ++itor)
	{
		CGraphicThingInstance & rInstance = itor->second->ThingInstance;
		//rInstance.Update();
		rInstance.Render();
		rInstance.BlendRender();
	}
}

void CPythonItem::SetUseSoundFileName(DWORD eItemType, const std::string& c_rstFileName)
{
	if (eItemType>=USESOUND_NUM)
		return;

	//Tracenf("SetUseSoundFile %d : %s", eItemType, c_rstFileName.c_str());

	m_astUseSoundFileName[eItemType]=c_rstFileName;	
}

void CPythonItem::SetDropSoundFileName(DWORD eItemType, const std::string& c_rstFileName)
{
	if (eItemType>=DROPSOUND_NUM)
		return;

	Tracenf("SetDropSoundFile %d : %s", eItemType, c_rstFileName.c_str());

	SGroundItemInstance::ms_astDropSoundFileName[eItemType]=c_rstFileName;
}

void	CPythonItem::PlayUseSound(DWORD dwItemID)
{
	//CItemManager& rkItemMgr=CItemManager::Instance();

	CItemData* pkItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwItemID, &pkItemData))
		return;

	DWORD eItemType=__GetUseSoundType(*pkItemData);
	if (eItemType==USESOUND_NONE)
		return;
	if (eItemType>=USESOUND_NUM)
		return;

	CSoundManager::Instance().PlaySound2D(m_astUseSoundFileName[eItemType].c_str());
}


void	CPythonItem::PlayDropSound(DWORD dwItemID)
{
	//CItemManager& rkItemMgr=CItemManager::Instance();

	CItemData* pkItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwItemID, &pkItemData))
		return;

	DWORD eItemType=__GetDropSoundType(*pkItemData);
	if (eItemType>=DROPSOUND_NUM)
		return;

	CSoundManager::Instance().PlaySound2D(SGroundItemInstance::ms_astDropSoundFileName[eItemType].c_str());
}

void	CPythonItem::PlayUsePotionSound()
{
	CSoundManager::Instance().PlaySound2D(m_astUseSoundFileName[USESOUND_POTION].c_str());
}

DWORD	CPythonItem::__GetDropSoundType(const CItemData& c_rkItemData)
{
	switch (c_rkItemData.GetType())
	{
		case CItemData::ITEM_TYPE_WEAPON:
			switch (c_rkItemData.GetWeaponType())
			{
				case CItemData::WEAPON_BOW:
					return DROPSOUND_BOW;
					break;
				case CItemData::WEAPON_ARROW:
				case CItemData::WEAPON_QUIVER:
					return DROPSOUND_DEFAULT;
					break;
				default:
					return DROPSOUND_WEAPON;
					break;
			}
			break;
		case CItemData::ITEM_TYPE_ARMOR:
			switch (c_rkItemData.GetSubType())
			{
				case CItemData::ARMOR_NECK:
				case CItemData::ARMOR_EAR:
					return DROPSOUND_ACCESSORY;
					break;
				case CItemData::ARMOR_BODY:
					return DROPSOUND_ARMOR;
				default:
					return DROPSOUND_DEFAULT;		
					break;
			}
			break;	
		default:
			return DROPSOUND_DEFAULT;
			break;
	}

	return DROPSOUND_DEFAULT;
}


DWORD	CPythonItem::__GetUseSoundType(const CItemData& c_rkItemData)
{
	switch (c_rkItemData.GetType())
	{
		case CItemData::ITEM_TYPE_WEAPON:
			switch (c_rkItemData.GetWeaponType())
			{
				case CItemData::WEAPON_BOW:
					return USESOUND_BOW;
					break;
				case CItemData::WEAPON_ARROW:
				case CItemData::WEAPON_QUIVER:
					return USESOUND_DEFAULT;
					break;
				default:
					return USESOUND_WEAPON;
					break;
			}
			break;
		case CItemData::ITEM_TYPE_ARMOR:
			switch (c_rkItemData.GetSubType())
			{
				case CItemData::ARMOR_NECK:
				case CItemData::ARMOR_EAR:
					return USESOUND_ACCESSORY;
					break;
				case CItemData::ARMOR_BODY:
					return USESOUND_ARMOR;
				default:
					return USESOUND_DEFAULT;		
					break;
			}
			break;
		case CItemData::ITEM_TYPE_USE:
			switch (c_rkItemData.GetSubType())
			{
				case CItemData::USE_ABILITY_UP:
					return USESOUND_POTION;
					break;
				case CItemData::USE_POTION:
					return USESOUND_NONE;
					break;
				case CItemData::USE_TALISMAN:
					return USESOUND_PORTAL;
					break;
				default:
					return USESOUND_DEFAULT;		
					break;
			}
			break;			
		default:
			return USESOUND_DEFAULT;
			break;
	}

	return USESOUND_DEFAULT;
}

#ifdef NEW_PICKUP_FILTER
inline static bool IsExceptionItem(DWORD vnum)
{
	switch (vnum)
	{
		case 93004: // Martyr's Cape
		case 76007: // Bravery Cape(Bonus)
		case 70038: // Bravery Cape
		case 70057: // Bravery Cape (another ymir item)
		case 93359: // Cape Box
		case 27987: // Clam aka Muschel
		case 50255: // Cor Draconis (Rough)
		case 39007:
		case 94155:
			return true;
	}

	return false;
}

inline static bool IsAutoPotion(DWORD vnum)
{
	switch (vnum)
	{
		case 39037:
		case 39038:
		case 39039:
		case 39040:
		case 39041:
		case 39042:
		case 72723:
		case 72724:
		case 72725:
		case 72726:
		case 72727:
		case 72728:
		case 72729:
		case 72730:
		case 76004:
		case 76005:
		case 76021:
		case 76022:
		case 93274:
		case 93275:
			return true;
	}

	return false;
}

inline static bool AlwaysPickItem(DWORD vnum, BYTE type, BYTE subtype)
{
	switch (vnum)
	{
		case 27987: // Clam aka Muschel
		case 50255: // Cor Draconis (Rough)
		case 50011:	// Moonlight Treasure Chest
		case 95010: // Prunous enchanting
		case 95013: // Xin Bean
		case 95011: // Legendary exorcism scroll
		case 95012: // Legendary Concentrated Reading
		case 33025: // Prism of Revival
			return true;
	}

	if (type == CItemData::ITEM_TYPE_USE)
	{
		switch (subtype)
		{
			case CItemData::USE_TUNING:
			case CItemData::USE_ADD_ATTRIBUTE:
			case CItemData::USE_ADD_ATTRIBUTE2:
			case CItemData::USE_CHANGE_ATTRIBUTE:
			case CItemData::USE_AFFECT:
			case CItemData::USE_SPECIAL:
				return true;
		}
	}

	return false;
}
#endif

#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
void CPythonItem::CreateItem(DWORD dwVirtualID, DWORD dwVirtualNumber, float x, float y, float z, bool bDrop, long alSockets[ITEM_SOCKET_SLOT_MAX_NUM], network::TItemAttribute aAttrs[ITEM_ATTRIBUTE_SLOT_MAX_NUM])
#else
void CPythonItem::CreateItem(DWORD dwVirtualID, DWORD dwVirtualNumber, float x, float y, float z, bool bDrop)
#endif
{
	//CItemManager& rkItemMgr=CItemManager::Instance();

	CItemData * pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwVirtualNumber, &pItemData))
		return;

	CGraphicThing* pItemModel = pItemData->GetDropModelThing();

	TGroundItemInstance *	pGroundItemInstance = m_GroundItemInstancePool.Alloc();	
	pGroundItemInstance->dwVirtualNumber = dwVirtualNumber;
	switch (pItemData->GetType())
	{
		case CItemData::ITEM_TYPE_WEAPON:
			pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_WEAPON;
			break;
		case CItemData::ITEM_TYPE_ARMOR:
			if (pItemData->GetSubType() == CItemData::ARMOR_BODY || pItemData->GetSubType() == CItemData::ARMOR_HEAD)
				pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_ARMOR;
			else
				pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_ETC;
			break;
#ifdef NEW_PICKUP_FILTER
		case CItemData::ITEM_TYPE_USE:
		{
			switch (pItemData->GetSubType())
			{
				case CItemData::USE_POTION:
				case CItemData::USE_POTION_CONTINUE:
				// case CItemData::USE_SPECIAL:
				case CItemData::USE_POTION_NODELAY:
					if (!IsExceptionItem(pItemData->GetIndex()))
						pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_POTION;
			}
			
			if (IsAutoPotion(pItemData->GetIndex()))
				pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_POTION;

			break;
		}
		
		case CItemData::ITEM_TYPE_SKILLBOOK:
		case CItemData::ITEM_TYPE_SKILLFORGET:
			pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_BOOK;
			break;
#endif
		default:
			pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_NONE;
			break;
	}

#ifdef NEW_PICKUP_FILTER
	switch (pItemData->GetIndex())
	{
		case 50304:
		case 50305:
		case 50306:
		case 50314:
		case 50315:
		case 50316:
		case 50301:
		case 50302:
		case 50303:
		case 50060:
			pGroundItemInstance->ePickUpFlag = PICKUP_TYPE_BOOK;
			break;
	}

	switch (pItemData->GetSize())
	{
		case 1:
			pGroundItemInstance->ePickUpFlag |= PICKUP_SIZE_1;
			break;
		case 2:
			pGroundItemInstance->ePickUpFlag |= PICKUP_SIZE_2;
			break;
		case 3:
			pGroundItemInstance->ePickUpFlag |= PICKUP_SIZE_3;
			break;
	}

	if (AlwaysPickItem(pItemData->GetIndex(), pItemData->GetType(), pItemData->GetSubType()))
		pGroundItemInstance->ePickUpFlag = PICKUP_ALWAYS_PICK;
#endif

	bool bStabGround = false;
	if (bDrop)
	{
		z = CPythonBackground::Instance().GetHeight(x, y) + 10.0f;

		if (pItemData->GetType()==CItemData::ITEM_TYPE_WEAPON && 
			(pItemData->GetWeaponType() == CItemData::WEAPON_SWORD || 
			 pItemData->GetWeaponType() == CItemData::WEAPON_ARROW))
			bStabGround = true;

		bStabGround = false;
		pGroundItemInstance->bAnimEnded = false;
	}
	else
		pGroundItemInstance->bAnimEnded = true;

	CEffectManager & rem =CEffectManager::Instance();
	pGroundItemInstance->dwEffectInstanceIndex = 
	rem.CreateEffect(m_dwDropItemEffectID, D3DXVECTOR3(x, -y, z), D3DXVECTOR3(0,0,0));		
	pGroundItemInstance->eDropSoundType=__GetDropSoundType(*pItemData);

	D3DXVECTOR3 normal;
	if (!CPythonBackground::Instance().GetNormal(int(x),int(y),&normal))
		normal = D3DXVECTOR3(0.0f,0.0f,1.0f);

	pGroundItemInstance->ThingInstance.Clear();
	pGroundItemInstance->ThingInstance.ReserveModelThing(1);
	pGroundItemInstance->ThingInstance.ReserveModelInstance(1);
	pGroundItemInstance->ThingInstance.RegisterModelThing(0, pItemModel);
	pGroundItemInstance->ThingInstance.SetModelInstance(0, 0, 0);
	if (bDrop)
	{
		pGroundItemInstance->v3EndPosition = D3DXVECTOR3(x,-y,z);
		pGroundItemInstance->ThingInstance.SetPosition(0,0,0);
	}
	else
		pGroundItemInstance->ThingInstance.SetPosition(x, -y, z);

	pGroundItemInstance->ThingInstance.Update();
	pGroundItemInstance->ThingInstance.Transform();
	pGroundItemInstance->ThingInstance.Deform();

	if (bDrop)
	{
		D3DXVECTOR3 vMin, vMax;
		pGroundItemInstance->ThingInstance.GetBoundBox(&vMin,&vMax);
		pGroundItemInstance->v3Center = (vMin + vMax) * 0.5f;

		std::pair<float,int> f[3] = 
			{
				make_pair(vMax.x - vMin.x,0), 
				make_pair(vMax.y - vMin.y,1), 
				make_pair(vMax.z - vMin.z,2)
			};

		std::sort(f,f+3);		
		D3DXVECTOR3 rEnd;
		if (bStabGround)
		{
			if (f[2].second == 0)
			{
				rEnd.y = 90.0f + frandom(-15.0f, 15.0f);
				rEnd.x = frandom(0.0f, 360.0f);
				rEnd.z = frandom(-15.0f, 15.0f);
			}
			else if (f[2].second == 1)
			{
				rEnd.y = frandom(0.0f, 360.0f);
				rEnd.x = frandom(-15.0f, 15.0f);
				rEnd.z = 180.0f + frandom(-15.0f, 15.0f);
			}
			else
			{
				rEnd.y = 180.0f + frandom(-15.0f, 15.0f);
				rEnd.x = 0.0f+frandom(-15.0f, 15.0f);
				rEnd.z = frandom(0.0f, 360.0f);
			}
		}
		else
		{
			if (f[0].second == 0)
			{
				pGroundItemInstance->qEnd = 
					RotationArc(
						D3DXVECTOR3(
						((float)(random()%2))*2-1+frandom(-0.1f,0.1f),
						0+frandom(-0.1f,0.1f),
						0+frandom(-0.1f,0.1f)),
						D3DXVECTOR3(0,0,1));
			}
			else if (f[0].second == 1)
			{
				pGroundItemInstance->qEnd = 
					RotationArc(
						D3DXVECTOR3(
							0+frandom(-0.1f,0.1f),
							((float)(random()%2))*2-1+frandom(-0.1f,0.1f),
							0+frandom(-0.1f,0.1f)),
						D3DXVECTOR3(0,0,1));
			}
			else 
			{
				pGroundItemInstance->qEnd = 
					RotationArc(
					D3DXVECTOR3(
					0+frandom(-0.1f,0.1f),
					0+frandom(-0.1f,0.1f),
					((float)(random()%2))*2-1+frandom(-0.1f,0.1f)),
					D3DXVECTOR3(0,0,1));
			}
		}
		float rot = frandom(0, 2*3.1415926535f);
		D3DXQUATERNION q(0,0,cosf(rot),sinf(rot));
		D3DXQuaternionMultiply(&pGroundItemInstance->qEnd, &pGroundItemInstance->qEnd, &q);
		q = RotationArc(D3DXVECTOR3(0,0,1),normal);
		D3DXQuaternionMultiply(&pGroundItemInstance->qEnd, &pGroundItemInstance->qEnd, &q);

		pGroundItemInstance->dwStartTime = CTimer::Instance().GetCurrentMillisecond();
		pGroundItemInstance->dwEndTime = pGroundItemInstance->dwStartTime+300;
		pGroundItemInstance->v3RotationAxis.x = sinf(rot+0);
		pGroundItemInstance->v3RotationAxis.y = cosf(rot+0);
		pGroundItemInstance->v3RotationAxis.z = 0;
		D3DXVECTOR3 v3Adjust = -pGroundItemInstance->v3Center;
		D3DXMATRIX mat;
		D3DXMatrixRotationQuaternion(&mat, &pGroundItemInstance->qEnd);
		D3DXVec3TransformCoord(&v3Adjust,&v3Adjust,&mat);
	}

	pGroundItemInstance->ThingInstance.Show();

	m_GroundItemInstanceMap.insert(TGroundItemInstanceMap::value_type(dwVirtualID, pGroundItemInstance));

#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
	char szItemName[128] = { 0 };
	int len = 0;
	switch (pItemData->GetType())
	{
		case CItemData::ITEM_TYPE_POLYMORPH:
		{
			const char* c_szTmp;
			CPythonNonPlayer& rkNonPlayer = CPythonNonPlayer::Instance();
			rkNonPlayer.GetName(alSockets[0], &c_szTmp);
			len += snprintf(szItemName, sizeof(szItemName), "%s", c_szTmp);
			break;
		}
		case CItemData::ITEM_TYPE_SKILLBOOK:
		case CItemData::ITEM_TYPE_SKILLFORGET:
		{
			const DWORD dwSkillVnum = (dwVirtualNumber == 50300 || dwVirtualNumber == 70037) ? alSockets[0] : 0;
			CPythonSkill::SSkillData * c_pSkillData;
			if ((dwSkillVnum != 0) && CPythonSkill::Instance().GetSkillData(dwSkillVnum, &c_pSkillData))
				len += snprintf(szItemName, sizeof(szItemName), "%s", c_pSkillData->GradeData[0].strName.c_str());

			break;
		}
	}

	len += snprintf(szItemName + len, sizeof(szItemName) - len, (len>0)?" %s":"%s", pItemData->GetName());

	bool bHasAttr = false;
	for (size_t i = 0; i < ITEM_ATTRIBUTE_SLOT_MAX_NUM; ++i)
	{
		if (aAttrs[i].type() != 0 && aAttrs[i].value() != 0)
		{
			bHasAttr = true;
			break;
		}
	}

	CPythonTextTail& rkTextTail = CPythonTextTail::Instance();
	rkTextTail.RegisterItemTextTail(
		dwVirtualID,
		szItemName,
		&pGroundItemInstance->ThingInstance,
		bHasAttr);
#else
	CPythonTextTail& rkTextTail = CPythonTextTail::Instance();
	rkTextTail.RegisterItemTextTail(
		dwVirtualID,
		pItemData->GetName(),
		&pGroundItemInstance->ThingInstance);
#endif
}

void CPythonItem::SetOwnership(DWORD dwVID, const char * c_pszName)
{
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.find(dwVID);

	if (m_GroundItemInstanceMap.end() == itor)
		return;

	TGroundItemInstance * pGroundItemInstance = itor->second;
	pGroundItemInstance->stOwnership.assign(c_pszName);

	CPythonTextTail& rkTextTail = CPythonTextTail::Instance();
	rkTextTail.SetItemTextTailOwner(dwVID, c_pszName);
}

bool CPythonItem::GetOwnership(DWORD dwVID, const char ** c_pszName)
{
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.find(dwVID);

	if (m_GroundItemInstanceMap.end() == itor)
		return false;

	TGroundItemInstance * pGroundItemInstance = itor->second;
	*c_pszName = pGroundItemInstance->stOwnership.c_str();

	return true;
}

void CPythonItem::DeleteAllItems()
{
	CPythonTextTail& rkTextTail=CPythonTextTail::Instance();

	TGroundItemInstanceMap::iterator i;
	for (i= m_GroundItemInstanceMap.begin(); i!=m_GroundItemInstanceMap.end(); ++i)
	{
		TGroundItemInstance* pGroundItemInst=i->second;
		rkTextTail.DeleteItemTextTail(i->first);
		pGroundItemInst->Clear();
		m_GroundItemInstancePool.Free(pGroundItemInst);
	}
	m_GroundItemInstanceMap.clear();
}

void CPythonItem::DeleteItem(DWORD dwVirtualID)
{
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.find(dwVirtualID);
	if (m_GroundItemInstanceMap.end() == itor)
		return;

	TGroundItemInstance * pGroundItemInstance = itor->second;
	pGroundItemInstance->Clear();
	m_GroundItemInstancePool.Free(pGroundItemInstance);
	m_GroundItemInstanceMap.erase(itor);

	// Text Tail
	CPythonTextTail::Instance().DeleteItemTextTail(dwVirtualID);
}


bool CPythonItem::GetCloseMoney(const TPixelPosition & c_rPixelPosition, DWORD * pdwItemID, DWORD dwDistance)
{
	DWORD dwCloseItemID = 0;
	DWORD dwCloseItemDistance = 1000 * 1000;

	TGroundItemInstanceMap::iterator i;
	for (i = m_GroundItemInstanceMap.begin(); i != m_GroundItemInstanceMap.end(); ++i)
	{
		TGroundItemInstance * pInstance = i->second;

		if (pInstance->dwVirtualNumber!=VNUM_MONEY)
			continue;

		DWORD dwxDistance = DWORD(c_rPixelPosition.x-pInstance->v3EndPosition.x);
		DWORD dwyDistance = DWORD(c_rPixelPosition.y-(-pInstance->v3EndPosition.y));
		DWORD dwDistance = DWORD(dwxDistance*dwxDistance + dwyDistance*dwyDistance);

		if (dwxDistance*dwxDistance + dwyDistance*dwyDistance < dwCloseItemDistance)
		{
			dwCloseItemID = i->first;
			dwCloseItemDistance = dwDistance;
		}
	}

	if (dwCloseItemDistance>float(dwDistance)*float(dwDistance))
		return false;

	*pdwItemID=dwCloseItemID;

	return true;
}


bool CPythonItem::GetCloseItem(const TPixelPosition & c_rPixelPosition, DWORD * pdwItemID, DWORD dwDistance)
{
	DWORD dwCloseItemID = 0;
	DWORD dwCloseItemDistance = 1000 * 1000;

	TGroundItemInstanceMap::iterator i;
	BYTE disabledPickupFlags = BYTE(CPythonConfig::Instance().GetInteger(CPythonConfig::CLASS_OPTION, "disabled_pickup_types"));
	for (i = m_GroundItemInstanceMap.begin(); i != m_GroundItemInstanceMap.end(); ++i)
	{
		TGroundItemInstance * pInstance = i->second;
#ifdef NEW_PICKUP_FILTER
		if (pInstance->ePickUpFlag != PICKUP_ALWAYS_PICK)
			if (IS_SET(disabledPickupFlags, pInstance->ePickUpFlag))
				continue;
#else
		if (IS_SET(disabledPickupFlags, pInstance->ePickUpFlag))
			continue;
#endif
		DWORD dwxDistance = DWORD(c_rPixelPosition.x-pInstance->v3EndPosition.x);
		DWORD dwyDistance = DWORD(c_rPixelPosition.y-(-pInstance->v3EndPosition.y));
		DWORD dwDistance = DWORD(dwxDistance*dwxDistance + dwyDistance*dwyDistance);

		if (dwxDistance*dwxDistance + dwyDistance*dwyDistance < dwCloseItemDistance)
		{
			dwCloseItemID = i->first;
			dwCloseItemDistance = dwDistance;
		}
	}

	if (dwCloseItemDistance>float(dwDistance)*float(dwDistance))
		return false;

	*pdwItemID=dwCloseItemID;

	return true;
}

bool CPythonItem::GetCloseItemList(const TPixelPosition & c_rPixelPosition, std::vector<DWORD>* pvec_dwItemID, DWORD dwDistance)
{
	float fMaxDistance = dwDistance * dwDistance;

	pvec_dwItemID->clear();

	TGroundItemInstanceMap::iterator i;
	BYTE disabledPickupFlags = BYTE(CPythonConfig::Instance().GetInteger(CPythonConfig::CLASS_OPTION, "disabled_pickup_types"));
	for (i = m_GroundItemInstanceMap.begin(); i != m_GroundItemInstanceMap.end(); ++i)
	{
		TGroundItemInstance * pInstance = i->second;
		if (IS_SET(disabledPickupFlags, pInstance->ePickUpFlag))
			continue;
		// if (pInstance->dwVirtualNumber == VNUM_MONEY)
		// 	continue;

		DWORD dwxDistance = DWORD(c_rPixelPosition.x - pInstance->v3EndPosition.x);
		DWORD dwyDistance = DWORD(c_rPixelPosition.y - (-pInstance->v3EndPosition.y));
		DWORD dwDistance = DWORD(dwxDistance*dwxDistance + dwyDistance*dwyDistance);

		if (dwDistance <= fMaxDistance)
		{
			DWORD dwItemID = i->first;
			pvec_dwItemID->push_back(dwItemID);
		}
	}

	return pvec_dwItemID->size() > 0;
}

BOOL CPythonItem::GetGroundItemPosition(DWORD dwVirtualID, TPixelPosition * pPosition)
{
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.find(dwVirtualID);
	if (m_GroundItemInstanceMap.end() == itor)
		return FALSE;

	TGroundItemInstance * pInstance = itor->second;

	const D3DXVECTOR3& rkD3DVct3=pInstance->ThingInstance.GetPosition();

	pPosition->x=+rkD3DVct3.x;
	pPosition->y=-rkD3DVct3.y;
	pPosition->z=+rkD3DVct3.z;

	return TRUE;
}

DWORD CPythonItem::__Pick(const POINT& c_rkPtMouse)
{
	float fu, fv, ft;

	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.begin();
	for (; itor != m_GroundItemInstanceMap.end(); ++itor)
	{
		TGroundItemInstance * pInstance = itor->second;

		if (pInstance->ThingInstance.Intersect(&fu, &fv, &ft))
		{
			return itor->first;
		}
	}

	CPythonTextTail& rkTextTailMgr=CPythonTextTail::Instance();
	return rkTextTailMgr.Pick(c_rkPtMouse.x, c_rkPtMouse.y);
}

bool CPythonItem::GetPickedItemID(DWORD* pdwPickedItemID)
{
	if (INVALID_ID==m_dwPickedItemID)
		return false;

	*pdwPickedItemID=m_dwPickedItemID;
	return true;
}

DWORD CPythonItem::GetVirtualNumberOfGroundItem(DWORD dwVID)
{
	TGroundItemInstanceMap::iterator itor = m_GroundItemInstanceMap.find(dwVID);

	if (itor == m_GroundItemInstanceMap.end())
		return 0;
	else
		return itor->second->dwVirtualNumber;
}

void CPythonItem::BuildNoGradeNameData(int iType)
{
	/*
	CMapIterator<std::string, CItemData *> itor = CItemManager::Instance().GetItemNameMapIterator();

	m_NoGradeNameItemData.clear();
	m_NoGradeNameItemData.reserve(1024);

	while (++itor)
	{
		CItemData * pItemData = *itor;
		if (iType == pItemData->GetType())
			m_NoGradeNameItemData.push_back(pItemData);
	}
	*/
}

DWORD CPythonItem::GetNoGradeNameDataCount()
{
	return m_NoGradeNameItemData.size();
}

CItemData * CPythonItem::GetNoGradeNameDataPtr(DWORD dwIndex)
{
	if (dwIndex >= m_NoGradeNameItemData.size())
		return NULL;

	return m_NoGradeNameItemData[dwIndex];
}

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
void CPythonItem::ClearAttributeData(BYTE bItemType, char cItemSubType)
{
	m_map_ItemAttrData.erase(std::make_pair(bItemType, cItemSubType));
}

void CPythonItem::AddAttributeData(BYTE bItemType, char cItemSubType, DWORD dwApplyIndex, int iApplyValue)
{
	m_map_ItemAttrData[std::make_pair(bItemType, cItemSubType)].push_back(std::make_pair(dwApplyIndex, iApplyValue));
}

const std::vector<std::pair<DWORD, int> >* CPythonItem::GetAttributeData(BYTE bItemType, char cItemSubType)
{
	auto it = m_map_ItemAttrData.find(std::make_pair(bItemType, cItemSubType));
	if (it == m_map_ItemAttrData.end())
		return NULL;

	return &it->second;
}
#endif

void CPythonItem::Destroy()
{
	DeleteAllItems();
	m_GroundItemInstancePool.Clear();
}

void CPythonItem::Create()
{
	CEffectManager::Instance().RegisterEffect2("d:/ymir work/effect/etc/dropitem/dropitem.mse", &m_dwDropItemEffectID);
}

CPythonItem::CPythonItem()
{
	m_GroundItemInstancePool.SetName("CDynamicPool<TGroundItemInstance>");	
	m_dwPickedItemID = INVALID_ID;
}

CPythonItem::~CPythonItem()
{
	assert(m_GroundItemInstanceMap.empty());
}
