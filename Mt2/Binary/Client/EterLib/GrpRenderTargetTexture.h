#pragma once

#include "GrpTexture.h"

/*
D3DXProtectRenderTarget
Written by Matthew Fisher

The D3DXProtectRenderTarget class is used to simplify preserving a device's current render target and depth buffer
so new ones can easily be overlayed, and then the original render target and depth buffer restored.
CGraphicRenderTargetTexture is a texture that can be used as a render target.  It has its own associated depth buffer.

On D3DX8 is not tested properly.
*/
class D3DXProtectRenderTarget
{
	public:
#ifdef ENABLE_D3DX9
		D3DXProtectRenderTarget(LPDIRECT3DDEVICE9 Device, bool ProtectDepthBuffer, bool Enabled)
#else
		D3DXProtectRenderTarget(LPDIRECT3DDEVICE8 Device, bool ProtectDepthBuffer, bool Enabled)
#endif
		{
			_Enabled = Enabled;
			_RenderTarget = NULL;
			_DepthStencilSurface = NULL;
			if (_Enabled)
			{
				_ProtectDepthBuffer = ProtectDepthBuffer;
				Device->GetViewport(&_Viewport);
#ifdef ENABLE_D3DX9
				Device->GetRenderTarget(0, &_RenderTarget);
#else
				Device->GetRenderTarget(&_RenderTarget);
#endif
				if (ProtectDepthBuffer)
				{
					Device->GetDepthStencilSurface(&_DepthStencilSurface);
				}
				_Device = Device;
			}
		}

		~D3DXProtectRenderTarget()
		{
			if (_Enabled)
			{
				if (_RenderTarget)
				{
#ifdef ENABLE_D3DX9
					_Device->SetRenderTarget(0, _RenderTarget);
#else
					_Device->SetRenderTarget(_RenderTarget, _DepthStencilSurface);
#endif
				}
#ifdef ENABLE_D3DX9
				_Device->SetRenderTarget(1, NULL);
				_Device->SetRenderTarget(2, NULL);
				_Device->SetRenderTarget(3, NULL);
#else
				_Device->SetRenderTarget(NULL, NULL);
#endif
				if (_ProtectDepthBuffer && _DepthStencilSurface != NULL)
				{
#ifdef ENABLE_D3DX9
					_Device->SetDepthStencilSurface(_DepthStencilSurface);
#else
					_Device->SetRenderTarget(NULL, _DepthStencilSurface);
#endif
				}
				_Device->SetViewport(&_Viewport);

				if (_RenderTarget != NULL)
				{
					_RenderTarget->Release();
				}
				if (_ProtectDepthBuffer && _DepthStencilSurface != NULL)
				{
					_DepthStencilSurface->Release();
				}
			}
		}

	private:
#ifdef ENABLE_D3DX9
		LPDIRECT3DDEVICE9		_Device;
		LPDIRECT3DSURFACE9		_RenderTarget;
		LPDIRECT3DSURFACE9		_DepthStencilSurface;
		D3DVIEWPORT9			_Viewport;
#else
		LPDIRECT3DDEVICE8		_Device;
		LPDIRECT3DSURFACE8		_RenderTarget;
		LPDIRECT3DSURFACE8		_DepthStencilSurface;
		D3DVIEWPORT8			_Viewport;
#endif
		bool					_Enabled;
		bool					_ProtectDepthBuffer;
};

class CGraphicRenderTargetTexture : public CGraphicTexture
{
	public:
		CGraphicRenderTargetTexture();
		virtual	~CGraphicRenderTargetTexture();

	public:
		bool Create(int width, int height, D3DFORMAT texFormat, D3DFORMAT dephtFormat);
		void CreateTextures();
		bool CreateRenderTexture(int width, int height, D3DFORMAT format);
		bool CreateRenderDepthStencil(int width, int height, D3DFORMAT format);
		void SetRenderTarget();
		void ResetRenderTarget();
		void ClearRenderTarget(DWORD dwColor);
		void SetRenderingRect(const RECT & c_rRect);
		const RECT & GetRenderingRect() const;

		void SetRenderBox(RECT &renderBox);

#ifdef ENABLE_D3DX9
		LPDIRECT3DTEXTURE9 GetD3DRenderTargetTexture() const;
#else
		LPDIRECT3DTEXTURE8 GetD3DRenderTargetTexture() const;
#endif
		void ReleaseTextures();

		void Render() const;

	protected:
		void __Initialize();
		virtual void Initialize();

	protected:
#ifdef ENABLE_D3DX9
		LPDIRECT3DTEXTURE9 m_lpd3dRenderTargetTexture;
		LPDIRECT3DSURFACE9 m_lpd3dRenderTargetSurface;
		LPDIRECT3DSURFACE9 m_lpd3dRenderTargetDepthSurface;

		LPDIRECT3DSURFACE9 m_lpd3dOldBackBufferSurface;
		LPDIRECT3DSURFACE9 m_lpd3dOldDepthBufferSurface;
#else
		LPDIRECT3DTEXTURE8 m_lpd3dRenderTargetTexture;
		LPDIRECT3DSURFACE8 m_lpd3dRenderTargetSurface;
		LPDIRECT3DSURFACE8 m_lpd3dRenderTargetDepthSurface;

		LPDIRECT3DSURFACE8 m_lpd3dOldBackBufferSurface;
		LPDIRECT3DSURFACE8 m_lpd3dOldDepthBufferSurface;
#endif
		D3DFORMAT m_d3dFormat;
		D3DFORMAT m_depthStencilFormat;
		RECT m_renderRect;
		RECT m_renderBox;
};
