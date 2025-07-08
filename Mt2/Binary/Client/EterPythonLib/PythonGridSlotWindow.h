#pragma once

#include "PythonSlotWindow.h"

namespace UI 
{
	class CGridSlotWindow : public CSlotWindow
	{
		public:
			static DWORD Type();

		public:
			CGridSlotWindow(PyObject * ppyObject);
			virtual ~CGridSlotWindow();

			void Destroy();

			void ArrangeGridSlot(DWORD dwStartIndex, DWORD dwxCount, DWORD dwyCount, int ixSlotSize, int iySlotSize, int ixTemporarySize, int iyTemporarySize);

#ifdef AHMET_FISH_EVENT_SYSTEM
			void SetPickedAreaRender(bool bFlag)
			{
				bPickedAreaRender = bFlag;
			}
			bool GetPickedAreaRender()
			{
				return bPickedAreaRender;
			}
#endif	

		protected:
			void __Initialize();

			BOOL GetPickedSlotPointer(TSlot ** ppSlot);
			BOOL GetPickedSlotList(int iWidth, int iHeight, std::list<TSlot*> * pSlotPointerList);
			BOOL GetGridSlotPointer(int ix, int iy, TSlot ** ppSlot);
			BOOL GetPickedGridSlotPosition(int ixLocal, int iyLocal, int * pix, int * piy);
			BOOL CheckMoving(DWORD dwSlotNumber, DWORD dwItemIndex, const std::list<TSlot*> & c_rSlotList);

			BOOL OnIsType(DWORD dwType);

			void OnRefreshSlot();
			void OnRenderPickingSlot();

		protected:
			DWORD m_dwxCount;
			DWORD m_dwyCount;

#ifdef AHMET_FISH_EVENT_SYSTEM
			bool bPickedAreaRender;
#endif			

			std::vector<TSlot *> m_SlotVector;
	};
};
