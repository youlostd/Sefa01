#include "StdAfx.h"
#include "../eterBase/Random.h"
#include "../eterlib/StateManager.h"
#include "EffectManager.h"

#ifdef ENABLE_EFFECT_HIDE_SYSTEM
#include "../GameLib/MapType.h"
#include "../GameLib/GameType.h"
#include "../UserInterface/PythonBackground.h"
#define POINT_MAX_NUM 255
#include "../UserInterface/PythonCharacterManager.h"
#include "../UserInterface/InstanceBase.h"

#define CONFIG_NAME_EFFECT "perf_effect_range"
float CPythonConfig__GetOptionFloat(const std::string& stKey, float fDefaultValue);
#endif

void CEffectManager::GetInfo(std::string* pstInfo)
{
	char szInfo[256];
	
	sprintf(szInfo, "Effect: Inst - ED %d, EI %d Pool - PSI %d, MI %d, LI %d, PI %d, EI %d, ED %d, PSD %d, EM %d, LD %d", 		
		m_kEftDataMap.size(),
		m_kEftInstMap.size(),		
		CParticleSystemInstance::ms_kPool.GetCapacity(),
		CEffectMeshInstance::ms_kPool.GetCapacity(),
		CLightInstance::ms_kPool.GetCapacity(),		
		CParticleInstance::ms_kPool.GetCapacity(),
		//CRayParticleInstance::ms_kPool.GetCapacity(),
		CEffectInstance::ms_kPool.GetCapacity(),
		CEffectData::ms_kPool.GetCapacity(),
		CParticleSystemData::ms_kPool.GetCapacity(),
		CEffectMeshScript::ms_kPool.GetCapacity(),
		CLightData::ms_kPool.GetCapacity()
	);
	pstInfo->append(szInfo);
}

void CEffectManager::UpdateSound()
{
	for (TEffectInstanceMap::iterator itor = m_kEftInstMap.begin(); itor != m_kEftInstMap.end(); ++itor)
	{
		CEffectInstance * pEffectInstance = itor->second;

		pEffectInstance->UpdateSound();
	}
}

bool CEffectManager::IsAliveEffect(DWORD dwInstanceIndex)
{
	TEffectInstanceMap::iterator f = m_kEftInstMap.find(dwInstanceIndex);
	if (m_kEftInstMap.end()==f)
		return false;

	return f->second->isAlive() ? true : false;
}

void CEffectManager::Update()
{

	// 2004. 3. 1. myevan. 이펙트 모니터링 하는 코드
	/*
	if (GetAsyncKeyState(VK_F9))
	{
		Tracenf("CEffectManager::m_EffectInstancePool %d", m_EffectInstancePool.GetCapacity());
		Tracenf("CEffectManager::m_EffectDataPool %d", m_EffectDataPool.GetCapacity());
		Tracenf("CEffectInstance::ms_LightInstancePool %d", CEffectInstance::ms_LightInstancePool.GetCapacity());
		Tracenf("CEffectInstance::ms_MeshInstancePool %d", CEffectInstance::ms_MeshInstancePool.GetCapacity());
		Tracenf("CEffectInstance::ms_ParticleSystemInstancePool %d", CEffectInstance::ms_ParticleSystemInstancePool.GetCapacity());
		Tracenf("CParticleInstance::ms_ParticleInstancePool %d", CParticleInstance::ms_kPool.GetCapacity());
		Tracenf("CRayParticleInstance::ms_RayParticleInstancePool %d", CRayParticleInstance::ms_kPool.GetCapacity());		
		Tracen("---------------------------------------------");
	}
	*/

	for (TEffectInstanceMap::iterator itor = m_kEftInstMap.begin(); itor != m_kEftInstMap.end();)
	{
		CEffectInstance * pEffectInstance = itor->second;

		pEffectInstance->Update(/*fElapsedTime*/);

		if (!pEffectInstance->isAlive())
		{
			itor = m_kEftInstMap.erase(itor);
			
			CEffectInstance::Delete(pEffectInstance);			
		}
		else
		{
			++itor;
		}
	}
}


struct CEffectManager_LessEffectInstancePtrRenderOrder
{
	bool operator() (CEffectInstance* pkLeft, CEffectInstance* pkRight)
	{
		return pkLeft->LessRenderOrder(pkRight);		
	}
};

struct CEffectManager_FEffectInstanceRender
{
	inline void operator () (CEffectInstance * pkEftInst)
	{
		pkEftInst->Render();
	}
};

void CEffectManager::Render()
{
#ifdef ENABLE_EFFECT_HIDE_SYSTEM
	float fShowRange = CPythonConfig__GetOptionFloat(CONFIG_NAME_EFFECT, 0.0f);
	bool isWarMap = !strcmp(CPythonBackground::Instance().GetWarpMapName(), "outdoor/map_warempire");
	bool isEventMap = !strcmp(CPythonBackground::Instance().GetWarpMapName(), "outdoor/metin2_map_event_bles");
	CInstanceBase* pInstMain = CPythonCharacterManager::Instance().GetMainInstancePtr();
	TPixelPosition kMainPos;

	if (!pInstMain)
		fShowRange = 0.0f;
	else
		pInstMain->NEW_GetPixelPosition(&kMainPos);
#endif
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
		
	static std::vector<CEffectInstance*> s_kVct_pkEftInstSort;
	s_kVct_pkEftInstSort.clear();

	TEffectInstanceMap& rkMap_pkEftInstSrc=m_kEftInstMap;
	TEffectInstanceMap::iterator i;
	for (i = rkMap_pkEftInstSrc.begin(); i != rkMap_pkEftInstSrc.end(); ++i)
#ifdef ENABLE_EFFECT_HIDE_SYSTEM
	{
		if ((!isWarMap && !isEventMap) || i->second->IsAlwaysRender())
		{
			if (fShowRange)
			{
				const D3DXMATRIX& rkEffMat = i->second->GetGlobalMatrix();

				float fDist = DISTANCE_APPROX(kMainPos.x - rkEffMat._41, kMainPos.y - (-rkEffMat._42));
				if (fDist <= fShowRange)
					s_kVct_pkEftInstSort.push_back(i->second);
			}
			else
				s_kVct_pkEftInstSort.push_back(i->second);
		}
	}
#else
		s_kVct_pkEftInstSort.push_back(i->second);
#endif

	std::sort(s_kVct_pkEftInstSort.begin(), s_kVct_pkEftInstSort.end(), CEffectManager_LessEffectInstancePtrRenderOrder());
	std::for_each(s_kVct_pkEftInstSort.begin(), s_kVct_pkEftInstSort.end(), CEffectManager_FEffectInstanceRender());
}

BOOL CEffectManager::RegisterEffect(const char * c_szFileName,bool isExistDelete,bool isNeedCache, bool isAlwaysRender)
{
	std::string strFileName;
	StringPath(c_szFileName, strFileName);
	DWORD dwCRC = GetCaseCRC32(strFileName.c_str(), strFileName.length());

	TEffectDataMap::iterator itor = m_kEftDataMap.find(dwCRC);
	if (m_kEftDataMap.end() != itor)
	{
		if (isExistDelete)
		{
			CEffectData* pkEftData=itor->second;
			CEffectData::Delete(pkEftData);			
			m_kEftDataMap.erase(itor);
		}
		else
		{
			//TraceError("CEffectManager::RegisterEffect - m_kEftDataMap.find [%s] Already Exist", c_szFileName);
			return TRUE;
		}
	}

	CEffectData * pkEftData = CEffectData::New();

	if (!pkEftData->LoadScript(c_szFileName, isAlwaysRender))
	{
		TraceError("CEffectManager::RegisterEffect - LoadScript(%s) Error", c_szFileName);
		CEffectData::Delete(pkEftData);
		return FALSE;
	}

	m_kEftDataMap.insert(TEffectDataMap::value_type(dwCRC, pkEftData));

	if (isNeedCache)
	{
		if (m_kEftCacheMap.find(dwCRC)==m_kEftCacheMap.end())
		{
			CEffectInstance* pkNewEftInst=CEffectInstance::New();
			pkNewEftInst->SetEffectDataPointer(pkEftData);
			m_kEftCacheMap.insert(TEffectInstanceMap::value_type(dwCRC, pkNewEftInst));
		}
	}

	return TRUE;
}
// CEffectData 를 포인터형으로 리턴하게 하고..
// CEffectData에서 CRC를 얻을수 있게 한다
BOOL CEffectManager::RegisterEffect2(const char * c_szFileName, DWORD* pdwRetCRC, bool isNeedCache, bool isAlwaysRender)
{	
	std::string strFileName;
	StringPath(c_szFileName, strFileName);
	DWORD dwCRC = GetCaseCRC32(strFileName.c_str(), strFileName.length());
	*pdwRetCRC=dwCRC;

	return RegisterEffect(c_szFileName,false,isNeedCache, isAlwaysRender);
}

int CEffectManager::CreateEffect(const char * c_szFileName, const D3DXVECTOR3 & c_rv3Position, const D3DXVECTOR3 & c_rv3Rotation)
{
	DWORD dwID = GetCaseCRC32(c_szFileName, strlen(c_szFileName));
	return CreateEffect(dwID, c_rv3Position, c_rv3Rotation);
}

#ifdef CHANGE_SKILL_COLOR
int CEffectManager::CreateEffect(DWORD dwID, const D3DXVECTOR3 & c_rv3Position, const D3DXVECTOR3 & c_rv3Rotation, DWORD *dwSkillColor)
#else
int CEffectManager::CreateEffect(DWORD dwID, const D3DXVECTOR3 & c_rv3Position, const D3DXVECTOR3 & c_rv3Rotation)
#endif
{
	int iInstanceIndex = GetEmptyIndex();

#ifdef CHANGE_SKILL_COLOR
	CreateEffectInstance(iInstanceIndex, dwID, dwSkillColor);
#else
	CreateEffectInstance(iInstanceIndex, dwID);
#endif
	SelectEffectInstance(iInstanceIndex);
	D3DXMATRIX mat;
	D3DXMatrixRotationYawPitchRoll(&mat,D3DXToRadian(c_rv3Rotation.x),D3DXToRadian(c_rv3Rotation.y),D3DXToRadian(c_rv3Rotation.z));
	mat._41 = c_rv3Position.x;
	mat._42 = c_rv3Position.y;
	mat._43 = c_rv3Position.z;
	SetEffectInstanceGlobalMatrix(mat);

	return iInstanceIndex;
}

#ifdef CHANGE_SKILL_COLOR
void CEffectManager::CreateEffectInstance(DWORD dwInstanceIndex, DWORD dwID, DWORD *dwSkillColor)
#else
void CEffectManager::CreateEffectInstance(DWORD dwInstanceIndex, DWORD dwID)
#endif
{
	if (!dwID)
		return;

	CEffectData * pEffect;
	if (!GetEffectData(dwID, &pEffect))
	{
		Tracef("CEffectManager::CreateEffectInstance - NO DATA :%d\n", dwID); 
		return;
	}

#ifdef CHANGE_SKILL_COLOR
	if (dwSkillColor != NULL)
	{
		for (int i = 0; i < pEffect->GetParticleCount(); i++)
		{
			DWORD skill;
			if (i > 5)
				skill = dwSkillColor[4];
			else
				skill = dwSkillColor[i];

			CParticleProperty * prob = pEffect->GetParticlePointer(i)->GetParticlePropertyPointer();
			if (skill != 99999999 && skill != 0)
			{
				if (!prob->m_TimeEventColor.empty() && prob->m_TimeEventColor2.empty())
					prob->m_TimeEventColor2 = prob->m_TimeEventColor;

				D3DXCOLOR c = D3DXCOLOR(skill);
				D3DXCOLOR d;
				for (TTimeEventTableColor::iterator it = prob->m_TimeEventColor.begin(); it != prob->m_TimeEventColor.end(); ++it)
				{
					d = D3DXCOLOR(it->m_Value.m_dwColor);
					c.a = d.a;
					it->m_Value.m_dwColor = (DWORD)c;
				}
			}
			else
			{
				if (!prob->m_TimeEventColor2.empty())
				for (int i = 0; i < prob->m_TimeEventColor.size(); ++i)
				{
					auto it = &prob->m_TimeEventColor[i];
					auto it2 = &prob->m_TimeEventColor2[i];

					it->m_Value.m_dwColor = it2->m_Value.m_dwColor;
				}
			}
		}
	}
#endif

	CEffectInstance * pEffectInstance = CEffectInstance::New();
#ifdef CHANGE_SKILL_COLOR
	pEffectInstance->SetEffectDataPointer(pEffect, dwSkillColor, dwID);
#else
	pEffectInstance->SetEffectDataPointer(pEffect);
#endif

	m_kEftInstMap.insert(TEffectInstanceMap::value_type(dwInstanceIndex, pEffectInstance));
}

bool CEffectManager::DestroyEffectInstance(DWORD dwInstanceIndex)
{
	TEffectInstanceMap::iterator itor = m_kEftInstMap.find(dwInstanceIndex);

	if (itor == m_kEftInstMap.end())
		return false;

	CEffectInstance * pEffectInstance = itor->second;

	m_kEftInstMap.erase(itor);

	CEffectInstance::Delete(pEffectInstance);

	return true;
}

void CEffectManager::DeactiveEffectInstance(DWORD dwInstanceIndex)
{
	TEffectInstanceMap::iterator itor = m_kEftInstMap.find(dwInstanceIndex);

	if (itor == m_kEftInstMap.end())
		return;

	CEffectInstance * pEffectInstance = itor->second;
	pEffectInstance->SetDeactive();
}

void CEffectManager::ActiveEffectInstance(DWORD dwInstanceIndex)
{
	TEffectInstanceMap::iterator itor = m_kEftInstMap.find(dwInstanceIndex);

	if (itor == m_kEftInstMap.end())
		return;

	CEffectInstance * pEffectInstance = itor->second;
	pEffectInstance->SetActive();
}

void CEffectManager::CreateUnsafeEffectInstance(DWORD dwEffectDataID, CEffectInstance ** ppEffectInstance)
{
	CEffectData * pEffect;
	if (!GetEffectData(dwEffectDataID, &pEffect))
	{
		Tracef("CEffectManager::CreateEffectInstance - NO DATA :%d\n", dwEffectDataID); 
		return;
	}

	CEffectInstance* pkEftInstNew=CEffectInstance::New();
	pkEftInstNew->SetEffectDataPointer(pEffect);

	*ppEffectInstance = pkEftInstNew;	
}

bool CEffectManager::DestroyUnsafeEffectInstance(CEffectInstance * pEffectInstance)
{
	if (!pEffectInstance)
		return false;

	CEffectInstance::Delete(pEffectInstance);
	
	return true;
}

BOOL CEffectManager::SelectEffectInstance(DWORD dwInstanceIndex)
{
	TEffectInstanceMap::iterator itor = m_kEftInstMap.find(dwInstanceIndex);

	m_pSelectedEffectInstance = NULL;

	if (m_kEftInstMap.end() == itor)
		return FALSE;

	m_pSelectedEffectInstance = itor->second;

	return TRUE;
}

void CEffectManager::SetEffectTextures(DWORD dwID,vector<string> textures)
{
	CEffectData * pEffectData;
	if (!GetEffectData(dwID, &pEffectData))
	{
		Tracef("CEffectManager::CreateEffectInstance - NO DATA :%d\n", dwID); 
		return;
	}

	for(DWORD i = 0; i < textures.size(); i++)
	{
		CParticleSystemData * pParticle = pEffectData->GetParticlePointer(i);
		pParticle->ChangeTexture(textures.at(i).c_str());
	}
}

void CEffectManager::SetEffectInstancePosition(const D3DXVECTOR3 & c_rv3Position)
{
	if (!m_pSelectedEffectInstance)
	{
//		assert(!"Instance to use is not yet set!");
		return;
	}

	m_pSelectedEffectInstance->SetPosition(c_rv3Position);
}

void CEffectManager::SetEffectInstanceRotation(const D3DXVECTOR3 & c_rv3Rotation)
{
	if (!m_pSelectedEffectInstance)
	{
//		assert(!"Instance to use is not yet set!");
		return;
	}

	m_pSelectedEffectInstance->SetRotation(c_rv3Rotation.x,c_rv3Rotation.y,c_rv3Rotation.z);
}

void CEffectManager::SetEffectInstanceGlobalMatrix(const D3DXMATRIX & c_rmatGlobal)
{
	if (!m_pSelectedEffectInstance)
		return;

	m_pSelectedEffectInstance->SetGlobalMatrix(c_rmatGlobal);
}

void CEffectManager::ShowEffect()
{
	if (!m_pSelectedEffectInstance)
		return;

	m_pSelectedEffectInstance->Show();
}

void CEffectManager::HideEffect()
{
	if (!m_pSelectedEffectInstance)
		return;

	m_pSelectedEffectInstance->Hide();
}

bool CEffectManager::GetEffectData(DWORD dwID, CEffectData ** ppEffect)
{
	TEffectDataMap::iterator itor = m_kEftDataMap.find(dwID);

	if (itor == m_kEftDataMap.end())
		return false;

	*ppEffect = itor->second;

	return true;
}

bool CEffectManager::GetEffectData(DWORD dwID, const CEffectData ** c_ppEffect)
{
	TEffectDataMap::iterator itor = m_kEftDataMap.find(dwID);

	if (itor == m_kEftDataMap.end())
		return false;

	*c_ppEffect = itor->second;

	return true;
}

DWORD CEffectManager::GetRandomEffect()
{
	int iIndex = random() % m_kEftDataMap.size();

	TEffectDataMap::iterator itor = m_kEftDataMap.begin();
	for (int i = 0; i < iIndex; ++i, ++itor);

	return itor->first;
}

int CEffectManager::GetEmptyIndex()
{
	static int iMaxIndex=1;

	if (iMaxIndex>2100000000)
		iMaxIndex = 1;

	int iNextIndex = iMaxIndex++;
	while(m_kEftInstMap.find(iNextIndex) != m_kEftInstMap.end())
		iNextIndex++;

	return iNextIndex;
}

void CEffectManager::DeleteAllInstances()
{
	__DestroyEffectInstanceMap();
}

void CEffectManager::__DestroyEffectInstanceMap()
{
	for (TEffectInstanceMap::iterator i = m_kEftInstMap.begin(); i != m_kEftInstMap.end(); ++i)
	{
		CEffectInstance * pkEftInst = i->second;	
		CEffectInstance::Delete(pkEftInst);			
	}

	m_kEftInstMap.clear();
}

void CEffectManager::__DestroyEffectCacheMap()
{
	for (TEffectInstanceMap::iterator i = m_kEftCacheMap.begin(); i != m_kEftCacheMap.end(); ++i)
	{
		CEffectInstance * pkEftInst = i->second;	
		CEffectInstance::Delete(pkEftInst);			
	}

	m_kEftCacheMap.clear();
}

void CEffectManager::__DestroyEffectDataMap()
{
	for (TEffectDataMap::iterator i = m_kEftDataMap.begin(); i != m_kEftDataMap.end(); ++i)
	{
		CEffectData * pData = i->second;
		CEffectData::Delete(pData);				
	}

	m_kEftDataMap.clear();
}

void CEffectManager::Destroy()
{	
	__DestroyEffectInstanceMap();
	__DestroyEffectCacheMap();
	__DestroyEffectDataMap();
		
	__Initialize();
}

void CEffectManager::__Initialize()
{
	m_pSelectedEffectInstance = NULL;
}

CEffectManager::CEffectManager()
{
	__Initialize();
}

CEffectManager::~CEffectManager()
{
	Destroy();
}

// just for map effect

