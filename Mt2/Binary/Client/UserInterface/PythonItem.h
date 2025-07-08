#pragma once

// Note : ȭ��� ������ Item�� Update�� Rendering�� å������ ����
//        �� �������� ����Ÿ�� Icon Image Instance�� �Ŵ��� ���ұ��� �ְ�
//        ���� ���������� ���⵵ �ϴ� - 2003. 01. 13. [levites]

#include "../EterGrnLib/ThingInstance.h"
#include "protobuf_data_item.h"

class CItemData;

#ifdef ENABLE_ALPHA_EQUIP
extern int Item_ExtractAlphaEquipValue(int iVal);
extern bool Item_ExtractAlphaEquip(int iVal);
#endif

class CPythonItem : public CSingleton<CPythonItem>
{
	public:
		enum
		{
			INVALID_ID = 0xffffffff,
		};

		enum
		{
			VNUM_MONEY = 1,
		};

		enum
		{
			USESOUND_NONE,
			USESOUND_DEFAULT,
			USESOUND_ARMOR,
			USESOUND_WEAPON,
			USESOUND_BOW,
			USESOUND_ACCESSORY,
			USESOUND_POTION,
			USESOUND_PORTAL,
			USESOUND_NUM,
		};

		enum
		{
			DROPSOUND_DEFAULT,
			DROPSOUND_ARMOR,
			DROPSOUND_WEAPON,
			DROPSOUND_BOW,
			DROPSOUND_ACCESSORY,
			DROPSOUND_NUM
		};

		enum EPickupType
		{
			PICKUP_TYPE_NONE,
			PICKUP_TYPE_ARMOR = 1 << 0,
			PICKUP_TYPE_WEAPON = 1 << 1,
			PICKUP_TYPE_ETC = 1 << 2,
#ifdef NEW_PICKUP_FILTER
			PICKUP_TYPE_POTION = 1 << 3,
			PICKUP_TYPE_BOOK = 1 << 4,
			PICKUP_SIZE_1 = 1 << 5,
			PICKUP_SIZE_2 = 1 << 6,
			PICKUP_SIZE_3 = 1 << 7,
			PICKUP_ALWAYS_PICK = 1 << 8,
#endif
		};

		typedef struct SGroundItemInstance
		{
			DWORD					dwVirtualNumber;
			D3DXVECTOR3				v3EndPosition;

			D3DXVECTOR3				v3RotationAxis;
			D3DXQUATERNION			qEnd;
			D3DXVECTOR3				v3Center;
			CGraphicThingInstance	ThingInstance;
			DWORD					dwStartTime;
			DWORD					dwEndTime;

			DWORD					eDropSoundType;

#ifdef NEW_PICKUP_FILTER
			DWORD					ePickUpFlag;
#else
			EPickupType				ePickUpFlag;
#endif

			bool					bAnimEnded;
			bool Update();
			void Clear();

			DWORD					dwEffectInstanceIndex;
			std::string				stOwnership;

			static void	__PlayDropSound(DWORD eItemType, const D3DXVECTOR3& c_rv3Pos);
			static std::string		ms_astDropSoundFileName[DROPSOUND_NUM];

			SGroundItemInstance() {}
			virtual ~SGroundItemInstance() {}
		} TGroundItemInstance;

		typedef std::map<DWORD, TGroundItemInstance *>	TGroundItemInstanceMap;

	public:
		CPythonItem(void);
		virtual ~CPythonItem(void);

		// Initialize
		void	Destroy();
		void	Create();

		void	PlayUseSound(DWORD dwItemID);
		void	PlayDropSound(DWORD dwItemID);
		void	PlayUsePotionSound();

		void	SetUseSoundFileName(DWORD eItemType, const std::string& c_rstFileName);
		void	SetDropSoundFileName(DWORD eItemType, const std::string& c_rstFileName);

		void	GetInfo(std::string* pstInfo);

		void	DeleteAllItems();

		void	Render();
		void	Update(const POINT& c_rkPtMouse);

#ifdef ENABLE_EXTENDED_ITEMNAME_ON_GROUND
		void	CreateItem(DWORD dwVirtualID, DWORD dwVirtualNumber, float x, float y, float z, bool bDrop = true, long alSockets[ITEM_SOCKET_SLOT_MAX_NUM] = {}, network::TItemAttribute aAttrs[ITEM_ATTRIBUTE_SLOT_MAX_NUM] = {});
#else
		void	CreateItem(DWORD dwVirtualID, DWORD dwVirtualNumber, float x, float y, float z, bool bDrop = true);
#endif
		void	DeleteItem(DWORD dwVirtualID);		
		void	SetOwnership(DWORD dwVID, const char * c_pszName);
		bool	GetOwnership(DWORD dwVID, const char ** c_pszName);

		BOOL	GetGroundItemPosition(DWORD dwVirtualID, TPixelPosition * pPosition);

		bool	GetPickedItemID(DWORD* pdwPickedItemID);

		bool	GetCloseItem(const TPixelPosition & c_rPixelPosition, DWORD* pdwItemID, DWORD dwDistance = 300);
		bool	GetCloseItemList(const TPixelPosition & c_rPixelPosition, std::vector<DWORD>* pvec_dwItemID, DWORD dwDistance = 300);
		bool	GetCloseMoney(const TPixelPosition & c_rPixelPosition, DWORD* dwItemID, DWORD dwDistance=300);

		DWORD	GetVirtualNumberOfGroundItem(DWORD dwVID);

		void	BuildNoGradeNameData(int iType);
		DWORD	GetNoGradeNameDataCount();
		CItemData * GetNoGradeNameDataPtr(DWORD dwIndex);

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
		void	ClearAttributeData(BYTE bItemType, char cItemSubType);
		void	AddAttributeData(BYTE bItemType, char cItemSubType, DWORD dwApplyIndex, int iApplyValue);
		const std::vector<std::pair<DWORD, int> >*	GetAttributeData(BYTE bItemType, char cItemSubType = -1);
#endif

	protected:
		DWORD	__Pick(const POINT& c_rkPtMouse);

		DWORD	__GetUseSoundType(const CItemData& c_rkItemData);
		DWORD	__GetDropSoundType(const CItemData& c_rkItemData);

	protected:
		TGroundItemInstanceMap				m_GroundItemInstanceMap;
		CDynamicPool<TGroundItemInstance>	m_GroundItemInstancePool;

		DWORD m_dwDropItemEffectID;
		DWORD m_dwPickedItemID;

		int m_nMouseX;
		int m_nMouseY;

		std::string m_astUseSoundFileName[USESOUND_NUM];

		std::vector<CItemData *> m_NoGradeNameItemData;

#ifdef ENABLE_ATTRIBUTES_TO_CLIENT
		std::map<std::pair<BYTE, char>, std::vector<std::pair<DWORD, int> > >	m_map_ItemAttrData;
#endif
};
