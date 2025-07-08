#pragma once
#include <map>
#include "GrpBase.h"
#include "GrpScreen.h"
#include "../eterBase/Singleton.h"
#include "GrpRenderTargetTexture.h"

class CRenderTargetManager : public CSingleton<CRenderTargetManager>
{
	public:
		enum ERenderWindows
		{
			RENDER_TARGET_SHOPDECO,
			RENDER_TARGET_MAX,
		};

		CRenderTargetManager();
		virtual ~CRenderTargetManager();

	public:
		void Destroy();

		void CreateRenderTargetTextures();
		void ReleaseRenderTargetTextures();

		bool CreateRenderTarget(int width, int height);
		bool CreateRenderTargetWithIndex(int width, int height, DWORD index);

		bool GetRenderTargetRect(DWORD index, RECT& rect);
		bool GetWikiRenderTargetRect(DWORD index, RECT& rect);
		bool ChangeRenderTarget(DWORD index);
		bool ChangeWikiRenderTarget(DWORD index);
		CGraphicRenderTargetTexture* GetRenderTarget(DWORD index);
		CGraphicRenderTargetTexture* GetWikiRenderTarget(DWORD index);
		CGraphicRenderTargetTexture* CreateWikiRenderTarget(DWORD index, DWORD width, DWORD height);
		void ResetRenderTarget();
		void ClearRenderTarget(DWORD dwColor = D3DCOLOR_ARGB(0, 0, 0, 0)) const;

	private:
		bool CreateGraphicTexture(DWORD index, DWORD width, DWORD height, D3DFORMAT texFormat, D3DFORMAT dephtFormat);

	protected:
		typedef std::map<DWORD, CGraphicRenderTargetTexture*> TRenderTargetMap;

	protected:
		TRenderTargetMap m_mapRenderTarget;
		TRenderTargetMap m_mapWikiRenderTarget;
		CGraphicRenderTargetTexture* m_pCurRenderTarget;
};
