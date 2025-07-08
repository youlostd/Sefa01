#include "StdAfx.h"
#include "StateManager.h"
#include "GrpRenderTargetTexture.h"
#include "../eterBase/Stl.h"
#include "../eterBase/Utils.h"

void CGraphicRenderTargetTexture::ReleaseTextures()
{
	SAFE_RELEASE(m_lpd3dRenderTargetTexture);
	SAFE_RELEASE(m_lpd3dRenderTargetSurface);
	SAFE_RELEASE(m_lpd3dRenderTargetDepthSurface);
	SAFE_RELEASE(m_lpd3dOldBackBufferSurface);
	SAFE_RELEASE(m_lpd3dOldDepthBufferSurface);
	memset(&m_renderRect, 0, sizeof(RECT));
	memset(&m_renderBox, 0, sizeof(RECT));
}

void CGraphicRenderTargetTexture::Initialize()
{
	CGraphicTexture::Initialize();
	m_lpd3dRenderTargetTexture = NULL;
	m_lpd3dRenderTargetSurface = NULL;
	m_lpd3dRenderTargetDepthSurface = NULL;
	m_lpd3dOldBackBufferSurface = NULL;
	m_lpd3dOldDepthBufferSurface = NULL;
	m_d3dFormat = D3DFMT_UNKNOWN;
	m_depthStencilFormat = D3DFMT_UNKNOWN;
	memset(&m_renderRect, 0, sizeof(RECT));
	memset(&m_renderBox, 0, sizeof(RECT));
}

bool CGraphicRenderTargetTexture::Create(int width, int height, D3DFORMAT texFormat, D3DFORMAT dephtFormat)
{
	__Initialize();

	m_width = width;
	m_height = height;
#ifdef ENABLE_D3DX9
	D3DXProtectRenderTarget Protection(ms_lpd3dDevice, true, true);
#endif
	if (!CreateRenderTexture(m_width, m_height, texFormat))
		return false;

	if (!CreateRenderDepthStencil(m_width, m_height, dephtFormat))
		return false;

	return true;
}

void CGraphicRenderTargetTexture::CreateTextures()
{
	if (CreateRenderTexture(m_width, m_height, m_d3dFormat))
		CreateRenderDepthStencil(m_width, m_height, m_depthStencilFormat);
}

bool CGraphicRenderTargetTexture::CreateRenderTexture(int width, int height, D3DFORMAT format)
{
	m_d3dFormat = format;

	HRESULT hr;
#ifdef ENABLE_D3DX9
	if (FAILED(hr = ms_lpd3dDevice->CreateTexture(width, width, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &m_lpd3dRenderTargetTexture, NULL)))
#else
	if (FAILED(hr = ms_lpd3dDevice->CreateTexture(width, width, 1, D3DUSAGE_RENDERTARGET, format, D3DPOOL_DEFAULT, &m_lpd3dRenderTargetTexture)))
#endif
	{
		return false;
	}

	if (FAILED(hr = m_lpd3dRenderTargetTexture->GetSurfaceLevel(0, &m_lpd3dRenderTargetSurface)))
	{
		return false;
	}

	return true;
}

bool CGraphicRenderTargetTexture::CreateRenderDepthStencil(int width, int height, D3DFORMAT format)
{
	m_depthStencilFormat = format;

	HRESULT hr;
#ifdef ENABLE_D3DX9
	if (FAILED(hr = ms_lpd3dDevice->CreateDepthStencilSurface(width, height, format, D3DMULTISAMPLE_NONE, 0, FALSE, &m_lpd3dRenderTargetDepthSurface, NULL)))
#else
	if (FAILED(hr = ms_lpd3dDevice->CreateDepthStencilSurface(width, height, format, D3DMULTISAMPLE_NONE, &m_lpd3dRenderTargetDepthSurface)))
#endif
	{
		return false;
	}

	return true;
}

void CGraphicRenderTargetTexture::SetRenderTarget()
{
#ifdef ENABLE_D3DX9
	ms_lpd3dDevice->GetRenderTarget(0, &m_lpd3dOldBackBufferSurface);
#else
	ms_lpd3dDevice->GetRenderTarget(&m_lpd3dOldBackBufferSurface);
#endif
	ms_lpd3dDevice->GetDepthStencilSurface(&m_lpd3dOldDepthBufferSurface);

#ifdef ENABLE_D3DX9
	ms_lpd3dDevice->SetRenderTarget(0, m_lpd3dRenderTargetSurface);
	ms_lpd3dDevice->SetDepthStencilSurface(m_lpd3dRenderTargetDepthSurface);
#else
	ms_lpd3dDevice->SetRenderTarget(m_lpd3dRenderTargetSurface, m_lpd3dRenderTargetDepthSurface);
#endif
}

void CGraphicRenderTargetTexture::ResetRenderTarget()
{
#ifdef ENABLE_D3DX9
	ms_lpd3dDevice->SetRenderTarget(0, m_lpd3dOldBackBufferSurface);
	ms_lpd3dDevice->SetDepthStencilSurface(m_lpd3dOldDepthBufferSurface);
#else
	ms_lpd3dDevice->SetRenderTarget(m_lpd3dOldBackBufferSurface, m_lpd3dOldDepthBufferSurface);
#endif

	SAFE_RELEASE(m_lpd3dOldBackBufferSurface);
	SAFE_RELEASE(m_lpd3dOldDepthBufferSurface);
}

void CGraphicRenderTargetTexture::ClearRenderTarget(DWORD dwColor)
{
	ms_lpd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, dwColor, 1.0f, 0);
}

void CGraphicRenderTargetTexture::SetRenderingRect(const RECT &c_rRect)
{
	m_renderRect = c_rRect;
}

const RECT& CGraphicRenderTargetTexture::GetRenderingRect() const
{
	return m_renderRect;
}

void CGraphicRenderTargetTexture::SetRenderBox(RECT& renderBox)
{
	memcpy(&m_renderBox, &renderBox, sizeof(m_renderBox));
}

#ifdef ENABLE_D3DX9
LPDIRECT3DTEXTURE9 CGraphicRenderTargetTexture::GetD3DRenderTargetTexture() const
#else
LPDIRECT3DTEXTURE8 CGraphicRenderTargetTexture::GetD3DRenderTargetTexture() const
#endif
{
	return m_lpd3dRenderTargetTexture;
}

void CGraphicRenderTargetTexture::Render() const
{
	float sx = static_cast<float>(m_renderRect.left) - 0.5f + static_cast<float>(m_renderBox.left);
	float sy = static_cast<float>(m_renderRect.top) - 0.5f + static_cast<float>(m_renderBox.top);
	float ex = static_cast<float>(m_renderRect.left) + (static_cast<float>(m_renderRect.right) - static_cast<float>(m_renderRect.left)) - 0.5f - static_cast<float>(m_renderBox.right);
	float ey = static_cast<float>(m_renderRect.top) + (static_cast<float>(m_renderRect.bottom) - static_cast<float>(m_renderRect.top)) - 0.5f - static_cast<float>(m_renderBox.bottom);
	float z = 0.0f;

	float texReverseWidth = 1.0f / (static_cast<float>(m_renderRect.right) - static_cast<float>(m_renderRect.left));
	float texReverseHeight = 1.0f / (static_cast<float>(m_renderRect.bottom) - static_cast<float>(m_renderRect.top));

	float su = m_renderBox.left * texReverseWidth;
	float sv = m_renderBox.top * texReverseHeight;
	float eu = ((m_renderRect.right - m_renderRect.left) - m_renderBox.right) * texReverseWidth;
	float ev = ((m_renderRect.bottom - m_renderRect.top) - m_renderBox.bottom) * texReverseHeight;


	TPDTVertex pVertices[4];
	pVertices[0].position = TPosition(sx, sy, z);
	pVertices[0].texCoord = TTextureCoordinate(su, sv);
	pVertices[0].diffuse = 0xffffffff;

	pVertices[1].position = TPosition(ex, sy, z);
	pVertices[1].texCoord = TTextureCoordinate(eu, sv);
	pVertices[1].diffuse = 0xffffffff;

	pVertices[2].position = TPosition(sx, ey, z);
	pVertices[2].texCoord = TTextureCoordinate(su, ev);
	pVertices[2].diffuse = 0xffffffff;

	pVertices[3].position = TPosition(ex, ey, z);
	pVertices[3].texCoord = TTextureCoordinate(eu, ev);
	pVertices[3].diffuse = 0xffffffff;

	if (SetPDTStream(pVertices, 4))
	{
		CGraphicBase::SetDefaultIndexBuffer(CGraphicBase::DEFAULT_IB_FILL_RECT);

		STATEMANAGER.SetTexture(0, GetD3DRenderTargetTexture());
		STATEMANAGER.SetTexture(1, NULL);
#ifdef ENABLE_D3DX9
		STATEMANAGER.SetFVF(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE);
#else
		STATEMANAGER.SetVertexShader(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_DIFFUSE);
#endif
		STATEMANAGER.DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 4, 0, 2);
	}
}

void CGraphicRenderTargetTexture::__Initialize()
{
	CGraphicTexture::Destroy();
	ReleaseTextures();

	m_d3dFormat = D3DFMT_UNKNOWN;
	m_depthStencilFormat = D3DFMT_UNKNOWN;
}

CGraphicRenderTargetTexture::CGraphicRenderTargetTexture()
{
	Initialize();
}

CGraphicRenderTargetTexture::~CGraphicRenderTargetTexture()
{
	__Initialize();
}
