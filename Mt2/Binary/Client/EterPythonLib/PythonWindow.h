#pragma once

#include "../eterBase/Utils.h"

namespace UI
{
	class CWindow
	{
		public:
			typedef std::list<CWindow *> TWindowContainer;

			static DWORD Type();
			BOOL IsType(DWORD dwType);

			enum EHorizontalAlign
			{
				HORIZONTAL_ALIGN_LEFT = 0,
				HORIZONTAL_ALIGN_CENTER = 1,
				HORIZONTAL_ALIGN_RIGHT = 2,					
			};

			enum EVerticalAlign
			{
				VERTICAL_ALIGN_TOP = 0,
				VERTICAL_ALIGN_CENTER = 1,
				VERTICAL_ALIGN_BOTTOM = 2,
			};

			enum EFlags
			{
				FLAG_MOVABLE			= (1 <<  0),	// 움직일 수 있는 창
				FLAG_LIMIT				= (1 <<  1),	// 창이 화면을 벗어나지 않음
				FLAG_SNAP				= (1 <<  2),	// 스냅 될 수 있는 창
				FLAG_DRAGABLE			= (1 <<  3),
				FLAG_ATTACH				= (1 <<  4),	// 완전히 부모에 붙어 있는 창 (For Drag / ex. ScriptWindow)
				FLAG_RESTRICT_X			= (1 <<  5),	// 좌우 이동 제한
				FLAG_RESTRICT_Y			= (1 <<  6),	// 상하 이동 제한
				FLAG_NOT_CAPTURE		= (1 <<  7),
				FLAG_FLOAT				= (1 <<  8),	// 공중에 떠있어서 순서 재배치가 되는 창
				FLAG_NOT_PICK			= (1 <<  9),	// 마우스에 의해 Pick되지 않는 창
				FLAG_IGNORE_SIZE		= (1 << 10),
				FLAG_RTL				= (1 << 11),	// Right-to-left
			};

		public:
			CWindow(PyObject * ppyObject);
			virtual ~CWindow();

			void			AddChild(CWindow * pWin);

			void			Clear();
			void			DestroyHandle();
			void			Update();
			void			Render();

			void			SetName(const char * c_szName);
			const char *	GetName()		{ return m_strName.c_str(); }
			void			SetSize(long width, long height);
			long			GetWidth()		{ return m_lWidth; }
			long			GetHeight()		{ return m_lHeight; }
			void			SetScale(float fx, float fy);
			D3DXVECTOR2		GetScale() { return m_v2Scale; }

			void			SetHorizontalAlign(DWORD dwAlign);
			void			SetVerticalAlign(DWORD dwAlign);
			void			SetPosition(long x, long y);
			void			GetPosition(long * plx, long * ply);
			long			GetPositionX( void ) const		{ return m_x; }
			long			GetPositionY( void ) const		{ return m_y; }
			RECT &			GetRect()		{ return m_rect; }
			void			GetLocalPosition(long & rlx, long & rly);
			void			GetMouseLocalPosition(long & rlx, long & rly);
			long			UpdateRect();

			RECT &			GetLimitBias()	{ return m_limitBiasRect; }
			void			SetLimitBias(long l, long r, long t, long b) { m_limitBiasRect.left = l, m_limitBiasRect.right = r, m_limitBiasRect.top = t, m_limitBiasRect.bottom = b; }

			void			Show();
			void			Hide();
			void			OnHideWithChilds();
			void			OnHide();
			virtual bool	IsShow();
			bool			IsRendering();

			bool			HasParent()		{ return m_pParent ? true : false; }
			bool			HasChild()		{ return m_pChildList.empty() ? false : true; }
			int				GetChildCount()	{ return m_pChildList.size(); }

			CWindow *		GetRoot();
			CWindow *		GetParent();
			bool			IsChild(CWindow * pWin, bool bCheckRecursive = false);
			void			DeleteChild(CWindow * pWin);
			void			SetTop(CWindow * pWin);

			bool			IsIn(long x, long y);
			bool			IsIn();
			CWindow *		PickWindow(long x, long y);
			CWindow *		PickTopWindow(long x, long y);	// NOTE : Children으로 내려가지 않고 상위에서만 
															//        체크 하는 특화된 함수

			void			__RemoveReserveChildren();

			void			AddFlag(DWORD flag)		{ SET_BIT(m_dwFlag, flag);		}
			void			RemoveFlag(DWORD flag)	{ REMOVE_BIT(m_dwFlag, flag);	}
			bool			IsFlag(DWORD flag)		{ return (m_dwFlag & flag) ? true : false;	}

			void			SetInsideRender(BOOL flag);
			void			GetRenderBox(RECT* box);
			void			UpdateRenderBox();
			void			UpdateRenderBoxRecursive();

			void			SetSingleAlpha(float fAlpha);
			virtual void	SetAlpha(float fAlpha)			{ m_fWindowAlpha = fAlpha; }
			virtual float	GetAlpha() const				{ return m_fWindowAlpha; }
			void			SetAllAlpha(float fAlpha);
			/////////////////////////////////////

			virtual void	OnRender();
			virtual void	OnAfterRender();
			virtual void	OnUpdate();
			virtual void	OnChangePosition(){}
			virtual void	OnUpdateRenderBox() {}

			virtual void	OnSetFocus();
			virtual void	OnKillFocus();

			virtual void	OnMouseDrag(long lx, long ly);
			virtual void	OnMouseOverIn();
			virtual void	OnMouseOverOut();
			virtual void	OnMouseOver();
			virtual void	OnDrop();
			virtual void	OnTop();
			virtual void	OnIMEUpdate();

			virtual void	OnMoveWindow(long x, long y);

			///////////////////////////////////////

			BOOL			RunIMETabEvent();
			BOOL			RunIMEReturnEvent();
			BOOL			RunIMEKeyDownEvent(int ikey);

			CWindow *		RunKeyDownEvent(int ikey);
			BOOL			RunKeyUpEvent(int ikey);
			BOOL			RunPressReturnKeyEvent();
			BOOL			RunPressEscapeKeyEvent();
			BOOL			RunPressExitKeyEvent();

			virtual BOOL	OnIMETabEvent();
			virtual BOOL	OnIMEReturnEvent();
			virtual BOOL	OnIMEKeyDownEvent(int ikey);

			virtual BOOL	OnIMEChangeCodePage();
			virtual BOOL	OnIMEOpenCandidateListEvent();
			virtual BOOL	OnIMECloseCandidateListEvent();
			virtual BOOL	OnIMEOpenReadingWndEvent();
			virtual BOOL	OnIMECloseReadingWndEvent();

			virtual BOOL	OnMouseLeftButtonDown();
			virtual BOOL	OnMouseLeftButtonUp();
			virtual BOOL	OnMouseLeftButtonDoubleClick();
			virtual BOOL	OnMouseRightButtonDown();
			virtual BOOL	OnMouseRightButtonUp();
			virtual BOOL	OnMouseRightButtonDoubleClick();
			virtual BOOL	OnMouseMiddleButtonDown();
			virtual BOOL	OnMouseMiddleButtonUp();
			virtual BOOL	RunMouseWheelEvent(int iLen);
			virtual BOOL	OnMouseWheel(int iLen);

			virtual BOOL	OnKeyDown(int ikey);
			virtual BOOL	OnKeyUp(int ikey);
			virtual BOOL	OnPressReturnKey();
			virtual BOOL	OnPressEscapeKey();
			virtual BOOL	OnPressExitKey();
			///////////////////////////////////////

			virtual void	SetColor(DWORD dwColor){}
			virtual BOOL	OnIsType(DWORD dwType);
			/////////////////////////////////////

			virtual BOOL	IsWindow() { return TRUE; }
			/////////////////////////////////////

		public:
			virtual void	iSetRenderingRect(int iLeft, int iTop, int iRight, int iBottom);
			virtual void	SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
			virtual int		GetRenderingWidth();
			virtual int		GetRenderingHeight();
			void			ResetRenderingRect(bool bCallEvent = true);

		private:
			virtual void	OnSetRenderingRect();

		protected:
			std::string			m_strName;

			EHorizontalAlign	m_HorizontalAlign;
			EVerticalAlign		m_VerticalAlign;
			long				m_x, m_y;				// X,Y 상대좌표
			long				m_lWidth, m_lHeight;	// 크기
			RECT				m_rect;					// Global 좌표
			RECT				m_limitBiasRect;		// limit bias 값
			RECT				m_renderingRect;
			D3DXVECTOR2			m_v2Scale;

			bool				m_bMovable;
			bool				m_bShow;

			DWORD				m_dwFlag;			

			PyObject *			m_poHandler;

			CWindow	*			m_pParent;
			TWindowContainer	m_pChildList;

			BOOL				m_isUpdatingChildren;
			TWindowContainer	m_pReserveChildList;

			bool				m_bSingleAlpha;
			float				m_fSingleAlpha;
			float				m_fWindowAlpha;
			bool				m_bAllAlpha;
			float				m_fAllAlpha;

			BOOL				m_isInsideRender;
			RECT				m_renderBox;
		
#ifdef _DEBUG
		public:
			DWORD				DEBUG_dwCounter;
#endif
	};

	class CLayer : public CWindow
	{
		public:
			CLayer(PyObject * ppyObject) : CWindow(ppyObject) {}
			virtual ~CLayer() {}

			BOOL IsWindow() { return FALSE; }
	};

	class CBox : public CWindow
	{
		public:
			CBox(PyObject * ppyObject);
			virtual ~CBox();

			void SetColor(DWORD dwColor);

		protected:
			void OnRender();

		protected:
			DWORD m_dwColor;
	};

	class CBar : public CWindow
	{
		public:
			CBar(PyObject * ppyObject);
			virtual ~CBar();

			void SetColor(DWORD dwColor);

		protected:
			void OnRender();

		protected:
			DWORD m_dwColor;
	};

	class CLine : public CWindow
	{
		public:
			CLine(PyObject * ppyObject);
			virtual ~CLine();

			void SetColor(DWORD dwColor);

		protected:
			void OnRender();

		protected:
			DWORD m_dwColor;
	};

	class CBar3D : public CWindow
	{
		public:
			static DWORD Type();

		public:
			CBar3D(PyObject * ppyObject);
			virtual ~CBar3D();

			void SetColor(DWORD dwLeft, DWORD dwRight, DWORD dwCenter);

		protected:
			void OnRender();

		protected:
			DWORD m_dwLeftColor;
			DWORD m_dwRightColor;
			DWORD m_dwCenterColor;
	};

	// Text
	class CTextLine : public CWindow
	{
		public:
			static DWORD Type();

		public:
			CTextLine(PyObject * ppyObject);
			virtual ~CTextLine();

			void SetFixedRenderPos(WORD startPos, WORD endPos) { m_TextInstance.SetFixedRenderPos(startPos, endPos); }
			void GetRenderPositions(WORD &startPos, WORD& endPos) { m_TextInstance.GetRenderPositions(startPos, endPos); }
			void SetMax(int iMax);
			void SetHorizontalAlign(int iType);
			void SetVerticalAlign(int iType);
			void SetSecret(BOOL bFlag);
			void SetOutline(BOOL bFlag);
			void SetOutlineColor(DWORD dwColor);
			void SetFeather(BOOL bFlag);
			void SetMultiLine(BOOL bFlag);
			void SetFontName(const char * c_szFontName);
			void SetFontColor(DWORD dwColor);
			void SetLimitWidth(float fWidth);

			void ShowCursor();
			void HideCursor();
			bool IsShowCursor();
			int GetCursorPosition();

			void SetText(const char * c_szText);
			const char * GetText();
			
			void GetTextSize(int* pnWidth, int* pnHeight);

			int GetRenderingWidth();
			int GetRenderingHeight();
			void OnSetRenderingRect();

			void SetAlpha(float fAlpha);
			float GetAlpha() const;

			bool IsShow();

#ifdef __ARABIC_LANG__
			void SetInverse();
#endif

		protected:
			void OnUpdate();
			void OnRender();
			void OnChangePosition();

			virtual void OnSetText(const char * c_szText);
			void OnUpdateRenderBox() { m_TextInstance.SetRenderBox(m_renderBox); }

			BOOL OnIsType(DWORD dwType);

		protected:
			CGraphicTextInstance m_TextInstance;
#ifdef __ARABIC_LANG__
			bool m_inverse;
#endif
	};

	class CNumberLine : public CWindow
	{
		public:
			CNumberLine(PyObject * ppyObject);
			CNumberLine(CWindow * pParent);
			virtual ~CNumberLine();

			void SetPath(const char * c_szPath);
			void SetHorizontalAlign(int iType);
			void SetNumber(const char * c_szNumber);

		protected:
			void ClearNumber();
			void OnRender();
			void OnChangePosition();

		protected:
			std::string m_strPath;
			std::string m_strNumber;
			std::vector<CGraphicImageInstance *> m_ImageInstanceVector;

			int m_iHorizontalAlign;
			DWORD m_dwWidthSummary;
	};

	// Image
	class CImageBox : public CWindow
	{
		public:
			static DWORD Type();
			CImageBox(PyObject * ppyObject);
			virtual ~CImageBox();

			BOOL LoadImage(const char * c_szFileName);
			BOOL LoadImage(CGraphicImage* pImagePtr);
			void SetDiffuseColor(float fr, float fg, float fb, float fa);
			void SetScale(float fx, float fy);

			int GetWidth();
			int GetHeight();

			void ShowImage() { m_bIsShowImage = true; }
			void HideImage() { m_bIsShowImage = false; }

			void SetAlpha(float fAlpha);
			float GetAlpha() const;

			void SetCoolTime(float fDuration);
			void DisplayImageProcent(float procent) { m_fDisplayProcent = procent; }

		protected:
			virtual void OnCreateInstance();
			virtual void OnDestroyInstance();

			virtual void OnUpdate();
			virtual void OnRender();
			void OnChangePosition();

			BOOL OnIsType(DWORD dwType);

		protected:
			CGraphicImageInstance * m_pImageInstance;
			bool m_bIsShowImage;
			float m_fAlpha;

			float m_fCoolTimeStart;
			float m_fCoolTimeDuration;
			float m_fDisplayProcent;
	};	
	class CMarkBox : public CWindow
	{
		public:
			CMarkBox(PyObject * ppyObject);
			virtual ~CMarkBox();

			void LoadImage(const char * c_szFilename);
			void SetDiffuseColor(float fr, float fg, float fb, float fa);
			void SetIndex(UINT uIndex);
			void SetScale(FLOAT fScale);

		protected:
			virtual void OnCreateInstance();
			virtual void OnDestroyInstance();

			virtual void OnUpdate();
			virtual void OnRender();
			void OnChangePosition();
		protected:
			CGraphicMarkInstance * m_pMarkInstance;
	};
	class CExpandedImageBox : public CImageBox
	{
		public:
			static DWORD Type();

		public:
			CExpandedImageBox(PyObject * ppyObject);
			virtual ~CExpandedImageBox();

			void SetScale(float fx, float fy);
			void SetOrigin(float fx, float fy);
			void SetRotation(float fRotation);
			int GetRenderingWidth();
			int GetRenderingHeight();
			void OnSetRenderingRect();
			void SetExpandedRenderingRect(float fLeftTop, float fLeftBottom, float fTopLeft, float fTopRight, float fRightTop, float fRightBottom, float fBottomLeft, float fBottomRight);
			void SetTextureRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
			void SetRenderingMode(int iMode);
			DWORD GetPixelColor(DWORD x, DWORD y);

			bool LoadImage(const char * c_szFileName);

		protected:
			void OnCreateInstance();
			void OnDestroyInstance();

			virtual void OnUpdate();
			virtual void OnRender();
			void OnChangePosition();
			void OnUpdateRenderBox();

			BOOL OnIsType(DWORD dwType);
	};
	class CAniImageBox : public CWindow
	{
		public:
			static DWORD Type();

		public:
			CAniImageBox(PyObject * ppyObject);
			virtual ~CAniImageBox();

			void SetDelay(int iDelay);
			void SetSkipCount(int iSkipCount);
			void SetTimedDelay(DWORD dwDelayInMS);
			bool LoadGIFImage(const char * c_szFileName);
			bool LoadScaledGIFImage(const char * c_szFileName, DWORD dwMaxWidth, DWORD dwMaxHeight);
			void AppendImage(const char * c_szFileName);
			void AppendImage(CGraphicImage * pResource);
			void ClearImages();
			void SetRenderingRect(float fLeft, float fTop, float fRight, float fBottom);
			void SetRenderingMode(int iMode);
			void SetDiffuseColor(float r, float g, float b, float a);

			void ResetFrame();
			WORD GetFrameIndex() const { return m_wcurIndex; }

			long GetRealWidth() const { return m_lRealWidth; }
			long GetRealHeight() const { return m_lRealHeight; }

			void SetAlpha(float fAlpha);
			float GetAlpha() const;

		protected:
			void OnUpdate();
			void OnRender();
			void OnChangePosition();
			virtual void OnNextFrame();
			virtual void OnEndFrame();

			BOOL OnIsType(DWORD dwType);

		protected:
			bool m_bUseTimedDelay;
			DWORD m_dwTimedDelay;
			DWORD m_dwcurTimedDelay;

			BYTE m_bycurDelay;
			BYTE m_byDelay;
			BYTE m_bySkipCount;
			WORD m_wcurIndex;
			std::vector<CGraphicExpandedImageInstance*> m_ImageVector;

			long m_lRealWidth, m_lRealHeight;

			float m_fAlpha;
	};

	// Button
	class CButton : public CWindow
	{
		public:
			static DWORD Type();

			CButton(PyObject * ppyObject);
			virtual ~CButton();

			BOOL SetUpVisual(const char * c_szFileName);
			BOOL SetOverVisual(const char * c_szFileName);
			BOOL SetDownVisual(const char * c_szFileName);
			BOOL SetDisableVisual(const char * c_szFileName);

			const char * GetUpVisualFileName();
			const char * GetOverVisualFileName();
			const char * GetDownVisualFileName();

			void Flash();
			void Enable();
			void Disable();
			bool IsEnabled(); 
#ifdef COMBAT_ZONE
			void FlashEx();
#endif

			void SetUp();
			void Up();
			void Over();
			void Down();

			BOOL IsDisable();
			BOOL IsPressed();

			void OnSetRenderingRect();

			void SetDiffuseColor(float r, float g, float b, float a);

			void SetAlpha(float fAlpha);
			float GetAlpha() const;

		protected:
			void OnUpdate();
			void OnRender();
			void OnChangePosition();

			BOOL OnMouseLeftButtonDown();
			BOOL OnMouseLeftButtonDoubleClick();
			BOOL OnMouseLeftButtonUp();
			void OnMouseOverIn();
			void OnMouseOverOut();

			BOOL IsEnable();

			void SetCurrentVisual(CGraphicExpandedImageInstance * pVisual);
			BOOL OnIsType(DWORD dwType);

		protected:
			BOOL m_bEnable;
			BOOL m_isPressed;
			BOOL m_isFlash;
#ifdef COMBAT_ZONE
			BOOL m_isFlashEx;
#endif
			CGraphicExpandedImageInstance * m_pcurVisual;
			CGraphicExpandedImageInstance m_upVisual;
			CGraphicExpandedImageInstance m_overVisual;
			CGraphicExpandedImageInstance m_downVisual;
			CGraphicExpandedImageInstance m_disableVisual;

			float m_fAlpha;
	};
	class CRadioButton : public CButton
	{
		public:
			CRadioButton(PyObject * ppyObject);
			virtual ~CRadioButton();

		protected:
			BOOL OnMouseLeftButtonDown();
			BOOL OnMouseLeftButtonUp();
			void OnMouseOverIn();
			void OnMouseOverOut();
	};
	class CToggleButton : public CButton
	{
		public:
			CToggleButton(PyObject * ppyObject);
			virtual ~CToggleButton();

		protected:
			BOOL OnMouseLeftButtonDown();
			BOOL OnMouseLeftButtonUp();
			void OnMouseOverIn();
			void OnMouseOverOut();
	};
	class CDragButton : public CButton
	{
		public:
			CDragButton(PyObject * ppyObject);
			virtual ~CDragButton();

			void SetRestrictMovementArea(int ix, int iy, int iwidth, int iheight);

		protected:
			void OnChangePosition();
			void OnMouseOverIn();
			void OnMouseOverOut();

		protected:
			RECT m_restrictArea;
	};

	// RenderTarget
	class CRenderTarget : public CWindow
	{
	public:
		CRenderTarget(PyObject * ppyObject);
		virtual ~CRenderTarget();

		bool SetRenderTarget(int iRenderTargetInex);
		bool SetWikiRenderTarget(int iRenderTargetInex);

	protected:
		virtual void OnUpdate();
		virtual void OnRender();
		void OnUpdateRenderBox();

	protected:
		CGraphicRenderTargetTexture * m_pRenderTexture;
		int m_iRenderTargetIndex;
	};
};

extern BOOL g_bOutlineBoxEnable;
