#pragma once

#include "PythonWindow.h"

namespace UI 
{
	enum
	{
		ITEM_WIDTH = 32,
		ITEM_HEIGHT = 32,

		SLOT_NUMBER_NONE = 0xffffffff,
	};

	enum ESlotStyle
	{
		SLOT_STYLE_NONE,
		SLOT_STYLE_PICK_UP,
		SLOT_STYLE_SELECT,
	};

	enum ESlotState
	{
		SLOT_STATE_LOCK		= (1 << 0),
		SLOT_STATE_CANT_USE	= (1 << 1),
		SLOT_STATE_DISABLE	= (1 << 2),
		SLOT_STATE_ALWAYS_RENDER_COVER = (1 << 3),			// 현재 Cover 버튼은 슬롯에 무언가 들어와 있을 때에만 렌더링 하는데, 이 flag가 있으면 빈 슬롯이어도 커버 렌더링
		SLOT_STATE_UNUSABLE	= (1 << 4),
		SLOT_STATE_UNUSABLE_WORLD = (1 << 5),
	};

	enum ESlotDiffuseColorType
	{
		SLOT_COLOR_TYPE_ORANGE,
		SLOT_COLOR_TYPE_WHITE,
		SLOT_COLOR_TYPE_RED,
		SLOT_COLOR_TYPE_GREEN,
		SLOT_COLOR_TYPE_YELLOW,
		SLOT_COLOR_TYPE_SKY,
		SLOT_COLOR_TYPE_PINK,
		SLOT_COLOR_TYPE_DARK_RED,
	};

	class CSlotWindow : public CWindow
	{
		public:
			static DWORD Type();

		public:
			class CSlotButton;
			class CCoverButton;
			class CCoolTimeFinishEffect;

			friend class CSlotButton;
			friend class CCoverButton;

			typedef struct SSlot
			{
				DWORD	dwState;
				DWORD	dwSlotNumber;
				DWORD	dwCenterSlotNumber;		// NOTE : 사이즈가 큰 아이템의 경우 아이템의 실제 위치 번호
				DWORD	dwItemIndex;			// NOTE : 여기서 사용되는 Item이라는 단어는 좁은 개념의 것이 아닌,
				BOOL	isItem;					//        "슬롯의 내용물"이라는 포괄적인 개념어. 더 좋은 것이 있을까? - [levites]

				// CoolTime
				float	fCoolTime;
				float	fStartCoolTime;

				// Toggle
				BOOL	bActive;

				int		ixPosition;
				int		iyPosition;

				int		ixCellSize;
				int		iyCellSize;

				BYTE	byxPlacedItemSize;
				BYTE	byyPlacedItemSize;

				CGraphicImageInstance * pInstance;
				CNumberLine * pNumberLine;

				bool	bRenderBaseSlotImage;
				CCoverButton * pCoverButton;
				CSlotButton * pSlotButton;
				CImageBox * pSignImage;
				CAniImageBox * pFinishCoolTimeEffect;
				D3DXCOLOR	sDiffuseColor;
			} TSlot;
			typedef std::list<TSlot> TSlotList;
			typedef TSlotList::iterator TSlotListIterator;

		public:
			CSlotWindow(PyObject * ppyObject);
			virtual ~CSlotWindow();

			bool IsSlotCovered(TSlot *ppSlot);
			bool IsEmptySlot(DWORD slotIndex);
			void Destroy();

			// Manage Slot
			void SetSlotType(DWORD dwType);
			void SetSlotStyle(DWORD dwStyle);

			void AppendSlot(DWORD dwIndex, int ixPosition, int iyPosition, int ixCellSize, int iyCellSize);
			void SetCoverButton(DWORD dwIndex, const char * c_szUpImageName, const char * c_szOverImageName, const char * c_szDownImageName, const char * c_szDisableImageName, BOOL bLeftButtonEnable, BOOL bRightButtonEnable);
			void SetSlotBaseImage(const char * c_szFileName, float fr, float fg, float fb, float fa);
			void AppendSlotButton(const char * c_szUpImageName, const char * c_szOverImageName, const char * c_szDownImageName);
			void AppendRequirementSignImage(const char * c_szImageName);

			void EnableCoverButton(DWORD dwIndex);
			void DisableCoverButton(DWORD dwIndex);

#ifdef AHMET_FISH_EVENT_SYSTEM
			void DeleteCoverButton(DWORD dwIndex);
#endif

			void SetAlwaysRenderCoverButton(DWORD dwIndex, bool bAlwaysRender = false);

			void ShowSlotBaseImage(DWORD dwIndex);
			void HideSlotBaseImage(DWORD dwIndex);
			BOOL IsDisableCoverButton(DWORD dwIndex);
			BOOL HasSlot(DWORD dwIndex);

			void ClearAllSlot();
			void ClearSlot(DWORD dwIndex);
			void SetSlot(DWORD dwIndex, DWORD dwVirtualNumber, BYTE byWidth, BYTE byHeight, CGraphicImage * pImage, D3DXCOLOR& diffuseColor);
			void SetCardSlot(DWORD dwIndex, DWORD dwVirtualNumber, BYTE byWidth, BYTE byHeight, const char * c_szFileName, D3DXCOLOR& diffuseColor);
			void SetSlotCount(DWORD dwIndex, DWORD dwCount);
			void SetSlotCountNew(DWORD dwIndex, DWORD dwGrade, DWORD dwCount);
			void SetSlotCoolTime(DWORD dwIndex, float fCoolTime, float fElapsedTime = 0.0f);
			void ActivateSlot(DWORD dwIndex);
			void DeactivateSlot(DWORD dwIndex);
			void RefreshSlot();

			DWORD GetSlotCount();

			void LockSlot(DWORD dwIndex);
			void UnlockSlot(DWORD dwIndex);
			BOOL IsLockSlot(DWORD dwIndex);
			void SetCantUseSlot(DWORD dwIndex);
			void SetUseSlot(DWORD dwIndex);
			BOOL IsCantUseSlot(DWORD dwIndex);
			void EnableSlot(DWORD dwIndex);
			void DisableSlot(DWORD dwIndex);
			BOOL IsEnableSlot(DWORD dwIndex);

			void GetSlotPosition(int iIndex, int* pLeft, int* pTop);

			// Select
			void ClearSelected();
			void SelectSlot(DWORD dwSelectingIndex);
			BOOL isSelectedSlot(DWORD dwIndex);
			DWORD GetSelectedSlotCount();
			DWORD GetSelectedSlotNumber(DWORD dwIndex);

			// Slot Button
			void ShowSlotButton(DWORD dwSlotNumber);
			void HideAllSlotButton();
			void OnPressedSlotButton(DWORD dwType, DWORD dwSlotNumber, BOOL isLeft = TRUE);

			// Requirement Sign
			void ShowRequirementSign(DWORD dwSlotNumber);
			void HideRequirementSign(DWORD dwSlotNumber);

			// ToolTip
			BOOL OnOverInItem(DWORD dwSlotNumber);
			void OnOverOutItem();

			// For Usable Item
			void SetUseMode(BOOL bFlag);
			void SetUsableItem(BOOL bFlag);
#ifdef ENABLE_ITEM_SWAP_SYSTEM
			void SetUsableItem2(BOOL bFlag);
#endif

			void SetUnusableSlot(DWORD dwSlotNumber, BOOL bFlag);
			void SetUnusableSlotWorld(DWORD dwSlotNumber, BOOL bFlag);

			// CallBack
			void ReserveDestroyCoolTimeFinishEffect(DWORD dwSlotIndex);

			void SetDiffuseColor(DWORD dwIndex, BYTE colorType);

		protected:
			void __Initialize();
			void __CreateToggleSlotImage();
			void __CreateSlotEnableEffect();
			void __CreateFinishCoolTimeEffect(TSlot * pSlot);
			void __CreateBaseImage(const char * c_szFileName, float fr, float fg, float fb, float fa);

			void __DestroyToggleSlotImage();
			void __DestroySlotEnableEffect();
			void __DestroyFinishCoolTimeEffect(TSlot * pSlot);
			void __DestroyBaseImage();

#ifdef ENABLE_MARK_NEW_ITEM_SYSTEM
			int __GetSlotActiveEffectIndex(const TSlot& c_rSlot) const;
#endif

			// Event
			void OnUpdate();
			void OnRender();
			BOOL OnMouseLeftButtonDown();
			BOOL OnMouseLeftButtonUp();
			BOOL OnMouseRightButtonDown();
			BOOL OnMouseLeftButtonDoubleClick();
			void OnMouseOverOut();
			void OnMouseOver();
			void RenderSlotBaseImage();
			void RenderLockedSlot();
			virtual void OnRenderPickingSlot();
			virtual void OnRenderSelectedSlot();

			// Select
			void OnSelectEmptySlot(int iSlotNumber);
			void OnSelectItemSlot(int iSlotNumber);
			void OnUnselectEmptySlot(int iSlotNumber);
			void OnUnselectItemSlot(int iSlotNumber);
			void OnUseSlot();

			// Manage Slot
			BOOL GetSlotPointer(DWORD dwIndex, TSlot ** ppSlot);
			BOOL GetSelectedSlotPointer(TSlot ** ppSlot);
			virtual BOOL GetPickedSlotPointer(TSlot ** ppSlot);
			void ClearSlot(TSlot * pSlot);
			virtual void OnRefreshSlot();

			// ETC
			BOOL OnIsType(DWORD dwType);

		protected:
			DWORD m_dwSlotType;
			DWORD m_dwSlotStyle;
			std::list<DWORD> m_dwSelectedSlotIndexList;
			TSlotList m_SlotList;
			DWORD m_dwToolTipSlotNumber;

			BOOL m_isUseMode;
			BOOL m_isUsableItem;
#ifdef ENABLE_ITEM_SWAP_SYSTEM
			BOOL m_isUsableItem2;
#endif

			CGraphicImageInstance * m_pBaseImageInstance;
			CImageBox * m_pToggleSlotImage;
#ifdef ENABLE_MARK_NEW_ITEM_SYSTEM
			CAniImageBox* m_apSlotActiveEffect[3];
#else
			CAniImageBox * m_pSlotActiveEffect;
#endif
			std::deque<DWORD> m_ReserveDestroyEffectDeque;
	};
};
