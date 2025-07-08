#include "StdAfx.h"

#include "PythonApplication.h"
#include "PythonMyShopDecoManager.h"
#include "../eterLib/Camera.h"
#include "../eterLib/GrpRenderTargetTexture.h"
#include "../eterLib/RenderTargetManager.h"
#include "../eterPythonLib/PythonGraphic.h"
#include "../gameLib/ActorInstance.h"

CPythonMyShopDecoManager::CPythonMyShopDecoManager() :
	m_pModelInstance(NULL),
	m_pWeaponModel(NULL),
	m_pBackgroundImage(NULL),
	m_bShow(false),
	m_fRot(0.0f)
{
	ClearInstances();
}

CPythonMyShopDecoManager::~CPythonMyShopDecoManager()
{
	Destroy();
}

void CPythonMyShopDecoManager::__Initialize()
{
	m_pBackgroundImage = NULL;
	m_pModelInstance = NULL;
	m_bShow = false;
	m_fRot = 0.0f;
}

void CPythonMyShopDecoManager::Destroy()
{
	m_bShow = false;
	m_fRot = 0.0f;

	if (m_pBackgroundImage)
		delete m_pBackgroundImage;
	m_pBackgroundImage = NULL;

	ClearInstances();
}

void CPythonMyShopDecoManager::ClearInstances()
{
	m_pModelInstance = NULL;
	for (auto it = m_mapModelInstance.begin(); it != m_mapModelInstance.end(); ++it)
		CInstanceBase::Delete(it->second);
	m_mapModelInstance.clear();
	if (m_pWeaponModel)
		delete m_pWeaponModel;
	m_pWeaponModel = NULL;
}

bool CPythonMyShopDecoManager::CreateBackground(DWORD dwWidth, DWORD dwHeight)
{
	if (!m_pBackgroundImage)
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer("d:/ymir work/ui/game/costume_preview/bg3.tga");
		m_pBackgroundImage = new CGraphicImageInstance;
		m_pBackgroundImage->SetImagePointer(static_cast<CGraphicImage*>(pResource));
		m_pBackgroundImage->SetScale(static_cast<float>(dwWidth) / 339.0f, static_cast<float>(dwHeight) / 453.0f);
	}

	return true;
}

CInstanceBase* CPythonMyShopDecoManager::GetInstancePtr(DWORD dwVnum)
{
	TModelInstanceMap::iterator itor = m_mapModelInstance.find(dwVnum);
	if (m_mapModelInstance.end() == itor)
		return NULL;

	return itor->second;
}

bool CPythonMyShopDecoManager::CreateModelInstance(DWORD dwVnum)
{
	if (GetInstancePtr(dwVnum))
		return true;

	CInstanceBase::SCreateData kCreateData;
	memset(&kCreateData, 0, sizeof(kCreateData));
	if (dwVnum < 20)
		kCreateData.m_bType = CActorInstance::TYPE_PC;
	else
		kCreateData.m_bType = CActorInstance::TYPE_OBJECT;
	kCreateData.m_dwRace = dwVnum;
	//kCreateData.m_fScale = 1.0f;

	CInstanceBase * pModel = CInstanceBase::New();
	if (!pModel->Create(kCreateData))
	{
		if (pModel)
			CInstanceBase::Delete(pModel);
		return false;
	}

	m_mapModelInstance.insert(TModelInstanceMap::value_type(dwVnum, pModel));
	return true;
}


bool CPythonMyShopDecoManager::SetModelHair(DWORD dwVnum, bool isItem)
{
	if (!m_pWeaponModel)
		return false;

	if (!dwVnum)
		isItem = false;
	DWORD eHair;
	if (isItem)
	{
		m_dwHairNum = dwVnum;
		CItemData * pItemData;
		if (!CItemManager::Instance().GetItemDataPointer(dwVnum, &pItemData))
			return false;

		eHair = pItemData->GetValue(3);
	}
	else
		eHair = dwVnum;
	CRaceData * pRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(m_pWeaponModel->GetMotionID(), &pRaceData))
		return false;

	CRaceData::SHair* pkHair = pRaceData->FindHair(eHair);
	if (pkHair)
	{
		if (!pkHair->m_stModelFileName.empty())
		{
			CGraphicThing * pkHairThing = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer(pkHair->m_stModelFileName.c_str());
			m_pWeaponModel->RegisterModelThing(CRaceData::PART_HAIR, pkHairThing);
			m_pWeaponModel->SetModelInstance(CRaceData::PART_HAIR, CRaceData::PART_HAIR, 0, CRaceData::PART_MAIN);
		}
#ifdef ENABLE_HAIR_SPECULAR
		float fSpecularPower = 0.0f;

		switch (eHair)
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
					m_pWeaponModel->SetMaterialData(CRaceData::PART_HAIR, c_rkSkinItem.m_stSrcFileName.c_str(), kMaterialData);
				}
				else
				{
					m_pWeaponModel->SetMaterialImagePointer(CRaceData::PART_HAIR, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
				}
			}
		}
#else
		m_pWeaponModel->SetMaterialImagePointer(CRaceData::PART_HAIR, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
#endif

		MOTION_KEY dwMotionKey;
		pRaceData->GetMotionKey(CRaceMotionData::MODE_GENERAL, CRaceMotionData::NAME_WAIT, &dwMotionKey);
		m_pWeaponModel->SetMotion(dwMotionKey, 0.0f, 0, 0.0f);
	}
	return true;
}

bool CPythonMyShopDecoManager::SetModelWeapon(DWORD dwVnum)
{
	/*if (!m_pModelInstance)
	{
		TraceError("MC:SetModelWeapon no instance selected");
		return false;
	}

	bool ret = m_pModelInstance->SetWeapon(dwVnum, false);
	if (ret)
	{
		m_pModelInstance->Refresh(CRaceMotionData::NAME_WAIT, true);
		m_pModelInstance->SetLoopMotion(CRaceMotionData::NAME_WAIT, 0.0f, 0.0f);

		m_pModelInstance->SetLODLimits(CRaceData::PART_WEAPON, 100.0f);
	}
	return ret;*/

	CItemData * pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwVnum, &pItemData))
		return false;

	if (pItemData->GetType() != CItemData::ITEM_TYPE_WEAPON && pItemData->GetType() != CItemData::ITEM_TYPE_COSTUME || pItemData->GetType() == CItemData::ITEM_TYPE_COSTUME && pItemData->GetSubType() != CItemData::COSTUME_WEAPON)
		return false;

	CGraphicThing* pItemModel = pItemData->GetDropModelThing();
	if (!pItemModel)
		return false;

	if (m_pWeaponModel)
		delete m_pWeaponModel;

	m_pWeaponModel = new CGraphicThingInstance;
	m_pWeaponModel->Clear();
	m_pWeaponModel->ReserveModelThing(1);
	m_pWeaponModel->ReserveModelInstance(1);
	m_pWeaponModel->RegisterModelThing(0, pItemModel);
	m_pWeaponModel->SetModelInstance(0, 0, 0);
	m_pWeaponModel->SetPosition(0, 0, 0);

	SMaterialData kMaterialData;
	kMaterialData.pImage = NULL;
	kMaterialData.isSpecularEnable = TRUE;
	kMaterialData.fSpecularPower = pItemData->GetSpecularPowerf();
	kMaterialData.bSphereMapIndex = 1;
	m_pWeaponModel->SetMaterialData(0, NULL, kMaterialData);


	CCameraManager::instance().SetCurrentCamera(CCameraManager::SHOPDECO_CAMERA);
	CCamera* pCam = CCameraManager::instance().GetCurrentCamera();

	pCam->SetTargetHeight(110.0f);

	BYTE subType = pItemData->GetSubType();
	if (pItemData->GetType() == CItemData::ITEM_TYPE_COSTUME)
		subType = pItemData->GetValue(0);
	switch (subType)
	{
	//// Sword:
	case CItemData::WEAPON_SWORD:
	{
		m_v3Eye = { 0.0f, -1000.0f, 0.0f };
		m_v3Target = { -0.0f, -49.1803f, -147.5410f };
		m_v3Up = { 0.0f, 0.0f, 1.0f };
		break;
	}

	//// Twohand:
	case CItemData::WEAPON_TWO_HANDED:
	{
		m_v3Eye = { 1000.0f, 1000.0f, 900.0f };
		m_v3Target = { -82.0f, -82.0f, 213.1f };
		m_v3Up = { 0.0f, 0.0f, 1.0f };
		break;
	}

	//// Bow:
	case CItemData::WEAPON_BOW:
	{
		m_v3Eye = { 1000.0f, 1000.0f, -639.3443f };
		m_v3Target = { -98.3607f, -98.3607f, -721.3115f };
		m_v3Up = { 0.0f, 0.0f, 1.0f };

		pCam->SetTargetHeight(m_pWeaponModel->GetHeight() / 2.0f);
		break;
	}

	//// Fan:
	case CItemData::WEAPON_FAN:
	{
		m_v3Eye = { -754.0984f, 377.0492f, -344.2623f };
		m_v3Target = { 114.7541f, -65.5738f, -49.1803f };
		m_v3Up = { 0.0f, 0.0f, 1.0f };
		break;
	}

	//// Dagger & Bell:
	case CItemData::WEAPON_DAGGER:
	case CItemData::WEAPON_BELL:
	{
		m_v3Eye = { -245.9016f, -557.3770f, -557.3770f };
		m_v3Target = { -32.7869f, -81.9672f, -557.3770f };
		m_v3Up = { 0.0f, 0.0f, 1.0f };
		break;
	}

#ifdef ENABLE_WOLFMAN_CHARACTER
	//// Claw:
	case CItemData::WEAPON_CLAW:
	{
		m_v3Eye = { 229.5082f, -409.8361f, 459.0164f };
		m_v3Target = { 16.3934f, -32.7869f, -114.7541f };
		m_v3Up = { 0.0f, 0.0f, 1.0f };
		break;
	}
#endif
	}

	CCameraManager::instance().ResetToPreviousCamera();
	return true;
}

bool CPythonMyShopDecoManager::SetModelArmor(DWORD dwVnum)
{
	SetModelForm(dwVnum, true);
	/*if (!m_pModelInstance)
	{
		TraceError("MC:SetModelArmor no instance selected");
		return false;
	}

	CItemData * pItemData;
	if (!CItemManager::Instance().GetItemDataPointer(dwVnum, &pItemData))
	{
		TraceError("MC:SetModelArmor wrong armor vnum: %d", dwVnum);
		return false;
	}

	m_pModelInstance->SetShape(pItemData->GetValue(3), pItemData->GetSpecularPowerf());
	m_pModelInstance->Refresh(CRaceMotionData::NAME_WAIT, true);
	m_pModelInstance->SetLoopMotion(CRaceMotionData::NAME_WAIT, 0.0f, 0.0f);
	if (dwVnum != 94135)
		m_pModelInstance->SetHair(m_pModelInstance->GetPart(CRaceData::PART_HAIR));
	else
	{
		m_pModelInstance->GetGraphicThingInstanceRef().RegisterModelThing(CRaceData::PART_HAIR, NULL);
		m_pModelInstance->GetGraphicThingInstanceRef().SetModelInstance(CRaceData::PART_HAIR, CRaceData::PART_HAIR, 0, CRaceData::PART_MAIN);
	}
	m_pModelInstance->SetWeapon(m_pModelInstance->GetPart(CRaceData::PART_WEAPON), false);
	m_pModelInstance->SetLODLimits(CRaceData::PART_HAIR, 100.0f);
	m_pModelInstance->SetLODLimits(CRaceData::PART_WEAPON, 100.0f);
	m_pModelInstance->SetLODLimits(CRaceData::PART_MAIN, 100.0f);

	if (m_pWeaponModel)
	{
		delete m_pWeaponModel;
		m_pWeaponModel = NULL;
	}*/
	return true;
}

void CPythonMyShopDecoManager::SetModelForm(DWORD dwVnum, bool isItem)
{
	if (!m_pWeaponModel)
		return;

	DWORD eShape = dwVnum;
	float fSpecular = 0.0f;
	if (isItem)
	{
		CItemData * pItemData;
		if (!CItemManager::Instance().GetItemDataPointer(dwVnum, &pItemData))
			return;

		eShape = pItemData->GetValue(3);
		fSpecular = pItemData->GetSpecularPowerf();
	}
	CRaceData * pRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(m_pWeaponModel->GetMotionID(), &pRaceData))
		return;

	CRaceData::SShape* pkShape = pRaceData->FindShape(eShape);
	if (pkShape)
	{
		CResourceManager& rkResMgr = CResourceManager::Instance();

		if (pkShape->m_stModelFileName.empty())
		{
			CGraphicThing* pModelThing = pRaceData->GetBaseModelThing();
			m_pWeaponModel->RegisterModelThing(0, pModelThing);
		}
		else
		{
			CGraphicThing* pModelThing = (CGraphicThing *)rkResMgr.GetResourcePointer(pkShape->m_stModelFileName.c_str());
			m_pWeaponModel->RegisterModelThing(0, pModelThing);
		}

		m_pWeaponModel->SetModelInstance(0, 0, 0);

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
					m_pWeaponModel->SetMaterialData(c_rkSkinItem.m_ePart, c_rkSkinItem.m_stSrcFileName.c_str(), kMaterialData);
				}
				else
				{
					m_pWeaponModel->SetMaterialImagePointer(c_rkSkinItem.m_ePart, c_rkSkinItem.m_stSrcFileName.c_str(), static_cast<CGraphicImage*>(pkRes));
				}
			}
		}
	}
	else
	{
		CGraphicThing* pModelThing = pRaceData->GetBaseModelThing();
		m_pWeaponModel->RegisterModelThing(0, pModelThing);
		m_pWeaponModel->SetModelInstance(0, 0, 0);
	}

	if (m_pWeaponModel->GetMotionID() < 20)
	{
		if (dwVnum != 94135)
		{
			SetModelHair(m_dwHairNum);
		}
		else
		{
			m_pWeaponModel->RegisterModelThing(CRaceData::PART_HAIR, NULL);
			m_pWeaponModel->SetModelInstance(CRaceData::PART_HAIR, CRaceData::PART_HAIR, 0, CRaceData::PART_MAIN);

			MOTION_KEY dwMotionKey;
			pRaceData->GetMotionKey(CRaceMotionData::MODE_GENERAL, CRaceMotionData::NAME_WAIT, &dwMotionKey);
			m_pWeaponModel->SetMotion(dwMotionKey, 0.0f, 0, 0.0f);
		}
	}
	else
	{
		MOTION_KEY dwMotionKey;
		pRaceData->GetMotionKey(CRaceMotionData::MODE_GENERAL, CRaceMotionData::NAME_WAIT, &dwMotionKey);
		m_pWeaponModel->SetMotion(dwMotionKey, 0.0f, 0, 0.0f);
	}
}

bool CPythonMyShopDecoManager::SelectModel(DWORD dwVnum)
{
	CRaceData * pRaceData;
	if (!CRaceManager::Instance().GetRaceDataPointer(dwVnum, &pRaceData))
		return false;
	pRaceData->LoadMotions();

	if (m_pWeaponModel)
	{
		delete m_pWeaponModel;
		m_pWeaponModel = NULL;
	}

	m_dwHairNum = 0;
	float fSpecular = 0.0f;
	float fTargetHeight = 0.0f;
	BYTE reserveAmmount = 1;
	if (dwVnum < 20)
	{
		//fSpecular = 100.0f;
		reserveAmmount = CRaceData::PART_MAX_NUM;
		fTargetHeight = 110.0f;
	}
	m_pWeaponModel = new CGraphicThingInstance;
	m_pWeaponModel->Clear();
	m_pWeaponModel->ReserveModelThing(reserveAmmount);
	m_pWeaponModel->ReserveModelInstance(reserveAmmount);
	m_pWeaponModel->SetMotionID(dwVnum);

	m_pWeaponModel->SetPosition(0, 0, 0);
	m_pWeaponModel->SetAlwaysRender(true);

	float fScale = 0.0f;
	if (dwVnum < 20)
		fScale = 1.0f;
	else
	{
		const CPythonNonPlayer::TMobTable* pMobTable = CPythonNonPlayer::Instance().GetTable(dwVnum);
		if (pMobTable)
			fScale = pMobTable->scaling_size();

		if (fScale == 0.0f)
			fScale = 1.0f;
	}
	m_pWeaponModel->SetScaleNew(fScale, fScale, fScale);
	m_pWeaponModel->SetRotation(0.0f);

	SetModelForm(0, false);
	m_pWeaponModel->Deform(); // otherwise the size would not be correct

	m_fRot = 0.0f;
	m_v3Eye.x = 0.0f;
	m_v3Eye.y = -1000.0f;
	m_v3Eye.z = 600.0f;

	m_v3Target.x = 0.0f;
	m_v3Target.y = 0.0f;
	m_v3Target.z = 0.0f;

	m_v3Up.x = 0.0f;
	m_v3Up.y = 0.0f;
	m_v3Up.z = 1.0f;

	if (dwVnum >= 20)
	{
		float fRaceHeight = CRaceManager::instance().GetRaceHeight(dwVnum);

		if (fRaceHeight == 0.0f)
			fRaceHeight = m_pWeaponModel->GetHeight();

		fTargetHeight = fRaceHeight / 2.0f;
		m_v3Eye.y = -(fRaceHeight * 8.9f);
		m_v3Eye.z = 0.0f;
	}

	CCameraManager::instance().SetCurrentCamera(CCameraManager::SHOPDECO_CAMERA);
	CCamera* pCam = CCameraManager::instance().GetCurrentCamera();
	pCam->SetTargetHeight(fTargetHeight);
	CCameraManager::instance().ResetToPreviousCamera();

	return true;
}

void CPythonMyShopDecoManager::RenderBackground()
{
	if (!m_bShow)
		return;

	if (!m_pBackgroundImage)
		return;

	RECT rectRender;
	if (!CRenderTargetManager::instance().GetRenderTargetRect(CPythonApplication::RENDER_TARGET_MYSHOPDECO, rectRender))
		return;

	if (!CRenderTargetManager::instance().ChangeRenderTarget(CPythonApplication::RENDER_TARGET_MYSHOPDECO))
		return;

	CRenderTargetManager::instance().ClearRenderTarget();
	CPythonGraphic::Instance().SetInterfaceRenderState();

	m_pBackgroundImage->Render();

	CRenderTargetManager::instance().ResetRenderTarget();
}

void CPythonMyShopDecoManager::DeformModel()
{
	if (!m_bShow)
		return;

	if (m_pModelInstance)
		m_pModelInstance->Deform();

	if (m_pWeaponModel)
		m_pWeaponModel->Deform();
}

void CPythonMyShopDecoManager::RenderModel()
{
	if (!m_bShow)
		return;

	RECT rectRender;
	if (!CRenderTargetManager::instance().GetRenderTargetRect(CPythonApplication::RENDER_TARGET_MYSHOPDECO, rectRender))
		return;

	if (!CRenderTargetManager::instance().ChangeRenderTarget(CPythonApplication::RENDER_TARGET_MYSHOPDECO))
		return;

	if (!m_pModelInstance && !m_pWeaponModel)
	{
		CRenderTargetManager::instance().ResetRenderTarget();
		return;
	}

	CPythonGraphic::Instance().ClearDepthBuffer();

	float fFov = CPythonGraphic::Instance().GetFOV();
	float fAspect = CPythonGraphic::Instance().GetAspect();
	float fNearY = CPythonGraphic::Instance().GetNear();
	float fFarY = CPythonGraphic::Instance().GetFar();

	float fWidth = static_cast<float>(rectRender.right) - static_cast<float>(rectRender.left);
	float fHeight = static_cast<float>(rectRender.bottom) - static_cast<float>(rectRender.top);

#ifdef ENABLE_FOG_FIX
	CPythonBackground& rkBG = CPythonBackground::Instance();
	BOOL bIsFog = rkBG.GetFogMode();
#else
	BOOL bIsFog = STATEMANAGER.GetRenderState(D3DRS_FOGENABLE);
#endif
	STATEMANAGER.SetRenderState(D3DRS_FOGENABLE, 0);
	CCameraManager::instance().SetCurrentCamera(CCameraManager::SHOPDECO_CAMERA);
	CCamera* pCam = CCameraManager::instance().GetCurrentCamera();

	CPythonGraphic::Instance().PushState();

	D3DXVECTOR3 v3Eye(0.0, -1000.0f, 600.0f);
	D3DXVECTOR3 v3Target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 v3Up(0.0f, 0.0f, 1.0f);
	if (!pCam->IsDraging())
		pCam->SetViewParams(m_v3Eye, m_v3Target, m_v3Up);

	CPythonApplication::Instance().TargetModelCamera();
	CPythonGraphic::Instance().SetPerspective(10.0f, fWidth / fHeight, 100.0f, 15000.0f);

	if (m_pModelInstance)
		m_pModelInstance->Render();
	else if (m_pWeaponModel)
	{
		// cullmode
		STATEMANAGER.SaveRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

		// Diffuse render begin
		STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		STATEMANAGER.SaveRenderState(D3DRS_ALPHABLENDENABLE, FALSE);

		m_pWeaponModel->RenderWithOneTexture();

		//diffuse render end
		STATEMANAGER.RestoreRenderState(D3DRS_ALPHABLENDENABLE);

		// blend render begin
		STATEMANAGER.SaveRenderState(D3DRS_ALPHATESTENABLE, TRUE);
		STATEMANAGER.SaveRenderState(D3DRS_ALPHAREF, 0);
		STATEMANAGER.SaveRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);

		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

		m_pWeaponModel->BlendRenderWithOneTexture();

		// blend render end
		STATEMANAGER.RestoreRenderState(D3DRS_ALPHATESTENABLE);
		STATEMANAGER.RestoreRenderState(D3DRS_ALPHAREF);
		STATEMANAGER.RestoreRenderState(D3DRS_ALPHAFUNC);

		// cullmode restore
		STATEMANAGER.RestoreRenderState(D3DRS_CULLMODE);
	}

	CCameraManager::instance().ResetToPreviousCamera();
	CPythonGraphic::Instance().PopState();
	CPythonGraphic::Instance().SetPerspective(fFov, fAspect, fNearY, fFarY);
	CRenderTargetManager::instance().ResetRenderTarget();
	STATEMANAGER.SetRenderState(D3DRS_FOGENABLE, bIsFog);
}

void CPythonMyShopDecoManager::UpdateModel()
{
	if (!m_bShow)
		return;

	if (!m_pModelInstance && !m_pWeaponModel)
		return;

	RECT rectRender;
	if (!CRenderTargetManager::instance().GetRenderTargetRect(CPythonApplication::RENDER_TARGET_MYSHOPDECO, rectRender))
		return;

	CCameraManager::instance().SetCurrentCamera(CCameraManager::SHOPDECO_CAMERA);
	CCamera * pCamera = CCameraManager::instance().GetCurrentCamera();
	if (pCamera && pCamera->IsDraging())
	{
		m_fRot = 0.0f;
		float heightSave = pCamera->GetTargetHeight();
		pCamera->Update();
		pCamera->SetTargetHeight(heightSave);
	}
	else
	{
		if (m_fRot == 360.0f)
			m_fRot = 0.0f;
		else
			m_fRot += 1.0f;
	}

	if (m_pWeaponModel)
	{
		m_pWeaponModel->SetRotation(m_fRot);
		m_pWeaponModel->Transform();
	}
	else if (m_pModelInstance)
	{
		m_pModelInstance->SetRotation(m_fRot);
		m_pModelInstance->Transform();
		CActorInstance& rkModelActor = m_pModelInstance->GetGraphicThingInstanceRef();
		rkModelActor.RotationProcess();
	}

	CCameraManager::instance().ResetToPreviousCamera();
}

void CPythonMyShopDecoManager::SetShow(bool bShow)
{
	m_bShow = bShow;
}
