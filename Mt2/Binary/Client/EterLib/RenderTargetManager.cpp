#include "StdAfx.h"
#include "RenderTargetManager.h"
#include "../EterBase/stl.h"

CRenderTargetManager::CRenderTargetManager() :
	m_pCurRenderTarget(NULL)
{
}

CRenderTargetManager::~CRenderTargetManager()
{
	Destroy();
}

void CRenderTargetManager::Destroy()
{
	stl_wipe_second(m_mapRenderTarget);
	stl_wipe_second(m_mapWikiRenderTarget);
	m_pCurRenderTarget = NULL;
}

void CRenderTargetManager::CreateRenderTargetTextures()
{
	for (TRenderTargetMap::iterator itor = m_mapRenderTarget.begin(); itor != m_mapRenderTarget.end(); ++itor)
		itor->second->CreateTextures();

	for (TRenderTargetMap::iterator itor = m_mapWikiRenderTarget.begin(); itor != m_mapWikiRenderTarget.end(); ++itor)
		itor->second->CreateTextures();
}

void CRenderTargetManager::ReleaseRenderTargetTextures()
{
	for (TRenderTargetMap::iterator itor = m_mapRenderTarget.begin(); itor != m_mapRenderTarget.end(); ++itor)
		itor->second->ReleaseTextures();

	for (TRenderTargetMap::iterator itor = m_mapWikiRenderTarget.begin(); itor != m_mapWikiRenderTarget.end(); ++itor)
		itor->second->ReleaseTextures();
}

bool CRenderTargetManager::CreateRenderTarget(int width, int height)
{
	return CreateGraphicTexture(0, width, height, D3DFMT_X8R8G8B8, D3DFMT_D16);
}

bool CRenderTargetManager::CreateRenderTargetWithIndex(int width, int height, DWORD index)
{
	return CreateGraphicTexture(index, width, height, D3DFMT_X8R8G8B8, D3DFMT_D16); // originally it was D3DFMT_A8R8G8B8 not X8
}

bool CRenderTargetManager::GetRenderTargetRect(DWORD index, RECT& rect)
{
	CGraphicRenderTargetTexture* pTarget = GetRenderTarget(index);
	if (!pTarget)
		return false;

	rect = pTarget->GetRenderingRect();
	return true;
}

bool CRenderTargetManager::GetWikiRenderTargetRect(DWORD index, RECT& rect)
{
	CGraphicRenderTargetTexture* pTarget = GetWikiRenderTarget(index);
	if (!pTarget)
		return false;

	rect = pTarget->GetRenderingRect();
	return true;
}

bool CRenderTargetManager::ChangeRenderTarget(DWORD index)
{
	m_pCurRenderTarget = GetRenderTarget(index);
	if (!m_pCurRenderTarget)
		return false;

	m_pCurRenderTarget->SetRenderTarget();
	return true;
}

bool CRenderTargetManager::ChangeWikiRenderTarget(DWORD index)
{
	m_pCurRenderTarget = GetWikiRenderTarget(index);
	if (!m_pCurRenderTarget)
		return false;

	m_pCurRenderTarget->SetRenderTarget();
	return true;
}

void CRenderTargetManager::ResetRenderTarget()
{
	if (m_pCurRenderTarget)
	{
		m_pCurRenderTarget->ResetRenderTarget();
		m_pCurRenderTarget = NULL;
	}
}

void CRenderTargetManager::ClearRenderTarget(DWORD dwColor) const
{
	if (m_pCurRenderTarget)
		m_pCurRenderTarget->ClearRenderTarget(dwColor);
}

CGraphicRenderTargetTexture* CRenderTargetManager::GetRenderTarget(DWORD index)
{
	TRenderTargetMap::iterator it = m_mapRenderTarget.find(index);
	if (it != m_mapRenderTarget.end())
		return it->second;

	return NULL;
}

CGraphicRenderTargetTexture* CRenderTargetManager::GetWikiRenderTarget(DWORD index)
{
	TRenderTargetMap::iterator it = m_mapWikiRenderTarget.find(index);
	if (it != m_mapWikiRenderTarget.end())
		return it->second;

	return NULL;
}

CGraphicRenderTargetTexture* CRenderTargetManager::CreateWikiRenderTarget(DWORD index, DWORD width, DWORD height)
{
	CGraphicRenderTargetTexture* pTex = GetWikiRenderTarget(index);
	if (!pTex)
	{
		pTex = new CGraphicRenderTargetTexture;
		m_mapWikiRenderTarget.insert(std::make_pair(index, pTex));
	}
	if (!pTex->Create(width, height, D3DFMT_X8R8G8B8, D3DFMT_D16))
	{
		m_mapWikiRenderTarget.erase(index);
		delete pTex;
		return NULL;
	}

	return pTex;
}

bool CRenderTargetManager::CreateGraphicTexture(DWORD index, DWORD width, DWORD height, D3DFORMAT texFormat, D3DFORMAT dephtFormat)
{
	if (index >= RENDER_TARGET_MAX)
	{
		return false;
	}

	CGraphicRenderTargetTexture* pTex;
	if ((pTex = GetRenderTarget(index)))
	{
		if (!pTex->Create(width, height, texFormat, dephtFormat))
		{
			delete pTex;
			m_mapRenderTarget.erase(index);
			return false;
		}
	}
	else
	{
		pTex = new CGraphicRenderTargetTexture;
		if (!pTex->Create(width, height, texFormat, dephtFormat))
		{
			delete pTex;
			return false;
		}

		// cross your finger so that the optimizer is really good
		m_mapRenderTarget.insert(std::make_pair(index, pTex));
	}

	// should be easier for the optimizer
	//m_mapRenderTarget.emplace(index, pTex);
	return true;
}
