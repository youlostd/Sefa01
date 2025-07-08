#include "StdAfx.h"
#include "../eterLib/ResourceManager.h"

#include "ItemData.h"

CDynamicPool<CItemData>		CItemData::ms_kPool;

extern DWORD GetDefaultCodePage();

CItemData* CItemData::New()
{
	return ms_kPool.Alloc();
}

void CItemData::Delete(CItemData* pkItemData)
{
	pkItemData->Clear();
	ms_kPool.Free(pkItemData);
}

void CItemData::DestroySystem()
{
	ms_kPool.Destroy();	
}

CGraphicThing * CItemData::GetModelThing()
{
	return m_pModelThing;
}

CGraphicThing * CItemData::GetSubModelThing()
{
	if (m_pSubModelThing)
		return m_pSubModelThing;
	else
		return m_pModelThing;
}

CGraphicThing * CItemData::GetDropModelThing()
{
	return m_pDropModelThing;
}

CGraphicSubImage * CItemData::GetIconImage()
{
	if(m_pIconImage == NULL && m_strIconFileName.empty() == false)
		__SetIconImage(m_strIconFileName.c_str());
	return m_pIconImage;
}

DWORD CItemData::GetLODModelThingCount()
{
	return m_pLODModelThingVector.size();
}

BOOL CItemData::GetLODModelThingPointer(DWORD dwIndex, CGraphicThing ** ppModelThing)
{
	if (dwIndex >= m_pLODModelThingVector.size())
		return FALSE;

	*ppModelThing = m_pLODModelThingVector[dwIndex];

	return TRUE;
}

DWORD CItemData::GetAttachingDataCount()
{
	return m_AttachingDataVector.size();
}

BOOL CItemData::GetCollisionDataPointer(DWORD dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData)
{
	if (dwIndex >= GetAttachingDataCount())
		return FALSE;

	if (NRaceData::ATTACHING_DATA_TYPE_COLLISION_DATA != m_AttachingDataVector[dwIndex].dwType)
		return FALSE;
	
	*c_ppAttachingData = &m_AttachingDataVector[dwIndex];
	return TRUE;
}

BOOL CItemData::GetAttachingDataPointer(DWORD dwIndex, const NRaceData::TAttachingData ** c_ppAttachingData)
{
	if (dwIndex >= GetAttachingDataCount())
		return FALSE;
	
	*c_ppAttachingData = &m_AttachingDataVector[dwIndex];
	return TRUE;
}

void CItemData::SetSummary(const std::string& c_rstSumm)
{
	m_strSummary=c_rstSumm;
}

void CItemData::SetDescription(const std::string& c_rstDesc)
{
	m_strDescription=c_rstDesc;
}
/*
BOOL CItemData::LoadItemData(const char * c_szFileName)
{
	CTextFileLoader TextFileLoader;

	if (!TextFileLoader.Load(c_szFileName))
	{
		//Lognf(1, "CItemData::LoadItemData(c_szFileName=%s) - FAILED", c_szFileName);
		return FALSE;
	}

	TextFileLoader.SetTop();

	TextFileLoader.GetTokenString("modelfilename", &m_strModelFileName);
	TextFileLoader.GetTokenString("submodelfilename", &m_strSubModelFileName);
	TextFileLoader.GetTokenString("dropmodelfilename", &m_strDropModelFileName);
	TextFileLoader.GetTokenString("iconimagefilename", &m_strIconFileName);

	char szDescriptionKey[32+1];
	_snprintf(szDescriptionKey, 32, "%ddescription", GetDefaultCodePage());
	if (!TextFileLoader.GetTokenString(szDescriptionKey, &m_strDescription))
	{
		TextFileLoader.GetTokenString("description", &m_strDescription);
	}

	// LOD Model File Name List
	CTokenVector * pLODModelList;
	if (TextFileLoader.GetTokenVector("lodmodellist", &pLODModelList))
	{
		m_strLODModelFileNameVector.clear();
		m_strLODModelFileNameVector.resize(pLODModelList->size());

		for (DWORD i = 0; i < pLODModelList->size(); ++i)
		{
			m_strLODModelFileNameVector[i] = pLODModelList->at(0);
		}
	}

	// Attaching Data
	// Item 에 Attaching Data 일단 없음.
//	if (TextFileLoader.SetChildNode("attachingdata"))
//	{
//		if (!NRaceData::LoadAttachingData(TextFileLoader, &m_AttachingDataVector))
//			return FALSE;
//
//		TextFileLoader.SetParentNode();
//	}

	__LoadFiles();

	return TRUE;
}
*/
void CItemData::SetDefaultItemData(const char * c_szIconFileName, const char * c_szModelFileName)
{
	if(c_szModelFileName)
	{
		m_strModelFileName = c_szModelFileName;
		m_strDropModelFileName = c_szModelFileName;
	}
	else
	{
		m_strModelFileName = "";
		m_strDropModelFileName = "d:/ymir work/item/etc/item_bag.gr2";
	}
	m_strIconFileName = c_szIconFileName;

	m_strSubModelFileName = "";
	m_strDescription = "";
	m_strSummary = "";

	m_ItemTable.clear_sockets();
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		m_ItemTable.add_sockets(i);

	__LoadFiles();
}

void CItemData::__LoadFiles()
{
	// Model File Name
	if (!m_strModelFileName.empty())
		m_pModelThing = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer(m_strModelFileName.c_str());

	if (!m_strSubModelFileName.empty())
		m_pSubModelThing = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer(m_strSubModelFileName.c_str());

	if (!m_strDropModelFileName.empty())
		m_pDropModelThing = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer(m_strDropModelFileName.c_str());


	if (!m_strLODModelFileNameVector.empty())
	{
		m_pLODModelThingVector.clear();
		m_pLODModelThingVector.resize(m_strLODModelFileNameVector.size());

		for (DWORD i = 0; i < m_strLODModelFileNameVector.size(); ++i)
		{
			const std::string & c_rstrLODModelFileName = m_strLODModelFileNameVector[i];
			m_pLODModelThingVector[i] = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer(c_rstrLODModelFileName.c_str());
		}
	}
}

void CItemData::__SetIconImage(const char * c_szFileName)
{
	if (!CResourceManager::Instance().IsFileExist(c_szFileName))
	{
		TraceError("CItemData::__SetIconImage - File not exist %s", c_szFileName);
		m_pIconImage = NULL;
#ifdef ENABLE_LOAD_ALTER_ITEMICON
		static const char* c_szAlterIconImage = "icon/item/27995.tga";
		if (CResourceManager::Instance().IsFileExist(c_szAlterIconImage))
			m_pIconImage = (CGraphicSubImage *)CResourceManager::Instance().GetResourcePointer(c_szAlterIconImage);
#endif
	}
	else if (m_pIconImage == NULL) 
		m_pIconImage = (CGraphicSubImage *)CResourceManager::Instance().GetResourcePointer(c_szFileName);
}

void CItemData::SetItemTableData(const TItemTable * pItemTable)
{
	m_ItemTable = *pItemTable;
}

void CItemData::OverwriteName(const char* c_pszNewName)
{
	auto locale_index = CLocaleManager::Instance().GetLanguage();
	while (m_ItemTable.locale_name_size() <= locale_index)
		m_ItemTable.add_locale_name();

	m_ItemTable.set_locale_name(locale_index, c_pszNewName);
}

void CItemData::OverwriteValue(unsigned char byIndex, long lNewValue)
{
	while (m_ItemTable.values_size() <= byIndex)
		m_ItemTable.add_values(0);

	m_ItemTable.set_values(byIndex, lNewValue);
}

const CItemData::TItemTable* CItemData::GetTable() const
{
	return &m_ItemTable;
}

DWORD CItemData::GetIndex() const
{
	return m_ItemTable.vnum();
}

const char * CItemData::GetName() const
{
	auto locale_index = CLocaleManager::Instance().GetLanguage();
	return locale_index < m_ItemTable.locale_name_size() ? m_ItemTable.locale_name(locale_index).c_str() : "";
}

const char * CItemData::GetDescription() const
{
	return m_strDescription.c_str();
}

const char * CItemData::GetSummary() const
{
	return m_strSummary.c_str();
}


BYTE CItemData::GetType() const
{
	return m_ItemTable.type();
}

BYTE CItemData::GetSubType() const
{
	return m_ItemTable.sub_type();
}

#define DEF_STR(x) #x

const char* CItemData::GetUseTypeString() const
{
	if (GetType() == CItemData::ITEM_TYPE_SOUL)
		return "USE_TYPE_SOUL";
	if (GetType() != CItemData::ITEM_TYPE_USE)
		return "NOT_USE_TYPE";

	switch (GetSubType())
	{
		case USE_TUNING:
			return DEF_STR(USE_TUNING);				
		case USE_DETACHMENT:
			return DEF_STR(USE_DETACHMENT);							
		case USE_CLEAN_SOCKET:
			return DEF_STR(USE_CLEAN_SOCKET);
		case USE_CHANGE_ATTRIBUTE:
			return DEF_STR(USE_CHANGE_ATTRIBUTE);
		case USE_ADD_ATTRIBUTE:
			return DEF_STR(USE_ADD_ATTRIBUTE);		
		case USE_ADD_ATTRIBUTE2:
			return DEF_STR(USE_ADD_ATTRIBUTE2);		
		case USE_ADD_ACCESSORY_SOCKET:
			return DEF_STR(USE_ADD_ACCESSORY_SOCKET);		
		case USE_PUT_INTO_ACCESSORY_SOCKET:
			return DEF_STR(USE_PUT_INTO_ACCESSORY_SOCKET);
		case USE_PUT_INTO_RING_SOCKET:
			return DEF_STR(USE_PUT_INTO_RING_SOCKET);
		case USE_SPECIAL:
			return DEF_STR(USE_SPECIAL);
		case USE_ADD_SPECIFIC_ATTRIBUTE:
			return DEF_STR(USE_ADD_SPECIFIC_ATTRIBUTE);
		case USE_DETACH_STONE:
			return DEF_STR(USE_DETACH_STONE);
		case USE_DETACH_ATTR:
			return DEF_STR(USE_DETACH_ATTR);
#ifdef ENABLE_DRAGONSOUL
		case USE_DS_CHANGE_ATTR:
			return DEF_STR(USE_DS_CHANGE_ATTR);
#endif
		case USE_PUT_INTO_ACCESSORY_SOCKET_PERMA:
			return DEF_STR(USE_PUT_INTO_ACCESSORY_SOCKET_PERMA);
		case USE_CHANGE_SASH_COSTUME_ATTR:
			return DEF_STR(USE_CHANGE_SASH_COSTUME_ATTR);
		case USE_DEL_LAST_PERM_ORE:
			return DEF_STR(USE_DEL_LAST_PERM_ORE);
	}
	return "USE_UNKNOWN_TYPE";
}


DWORD CItemData::GetWeaponType() const
{
	if (GetType() == ITEM_TYPE_COSTUME && GetSubType() == COSTUME_WEAPON)
		return GetValue(0);

	return m_ItemTable.sub_type();
}

BYTE CItemData::GetSize() const
{
	return m_ItemTable.size();
}

BOOL CItemData::IsAntiFlag(DWORD dwFlag) const
{
	return (dwFlag & m_ItemTable.anti_flags()) != 0;
}

DWORD CItemData::GetFlags() const
{
	return m_ItemTable.flags();
}

BOOL CItemData::IsFlag(DWORD dwFlag) const
{
	return (dwFlag & m_ItemTable.flags()) != 0;
}

BOOL CItemData::IsWearableFlag(DWORD dwFlag) const
{
	return (dwFlag & m_ItemTable.wear_flags()) != 0;
}

BOOL CItemData::HasNextGrade() const
{
	return 0 != m_ItemTable.refined_vnum();
}

DWORD CItemData::GetWearFlags() const
{
	return m_ItemTable.wear_flags();
}

DWORD CItemData::GetIBuyItemPrice() const
{
	return m_ItemTable.gold();
}

DWORD CItemData::GetISellItemPrice() const
{
	return m_ItemTable.shop_buy_price();
}


BOOL CItemData::GetLimit(BYTE byIndex, network::TItemLimit * pItemLimit) const
{
	if (byIndex >= ITEM_LIMIT_MAX_NUM)
	{
		assert(byIndex < ITEM_LIMIT_MAX_NUM);
		return FALSE;
	}

	if (byIndex < m_ItemTable.limits_size())
		*pItemLimit = m_ItemTable.limits(byIndex);
	else
		pItemLimit->Clear();

	return TRUE;
}

BOOL CItemData::GetApply(BYTE byIndex, network::TItemApply * pItemApply) const
{
	if (byIndex >= ITEM_APPLY_MAX_NUM)
	{
		assert(byIndex < ITEM_APPLY_MAX_NUM);
		return FALSE;
	}

	if (byIndex < m_ItemTable.applies_size())
		*pItemApply = m_ItemTable.applies(byIndex);
	else
		pItemApply->Clear();
	return TRUE;
}

long CItemData::GetValue(BYTE byIndex) const
{
	if (byIndex >= ITEM_VALUES_MAX_NUM)
	{
		assert(byIndex < ITEM_VALUES_MAX_NUM);
		return 0;
	}

	return byIndex < m_ItemTable.values_size() ? m_ItemTable.values(byIndex) : 0;
}

long CItemData::SetSocket(BYTE byIndex,DWORD value)
{
	if (byIndex >= ITEM_SOCKET_MAX_NUM)
	{
		assert(byIndex < ITEM_SOCKET_MAX_NUM);
		return -1;
	}

	while (m_ItemTable.sockets_size() < ITEM_SOCKET_MAX_NUM)
		m_ItemTable.add_sockets(0);

	m_ItemTable.set_sockets(byIndex, value);
	return value;
}

long CItemData::GetSocket(BYTE byIndex) const
{
	if (byIndex >= ITEM_SOCKET_MAX_NUM)
	{
		assert(byIndex < ITEM_SOCKET_MAX_NUM);
		return -1;
	}

	return byIndex < m_ItemTable.sockets_size() ? m_ItemTable.sockets(byIndex) : 0;
}

//서버와 동일 서버 함수 변경시 같이 변경!!(이후에 합친다)
//SocketCount = 1 이면 초급무기
//SocketCount = 2 이면 중급무기
//SocketCount = 3 이면 고급무기
int CItemData::GetSocketCount() const		
{
	return m_ItemTable.gain_socket_pct();
}

DWORD CItemData::GetIconNumber() const
{
	return m_ItemTable.vnum();
//!@#
//	return m_ItemTable.dwIconNumber;
}

UINT CItemData::GetSpecularPoweru() const
{
	return m_ItemTable.specular();
}

float CItemData::GetSpecularPowerf() const
{
	UINT uSpecularPower=GetSpecularPoweru();

	return float(uSpecularPower) / 100.0f;	
}

//refine 값은 아이템번호 끝자리와 일치한다-_-(테이블이용으로 바꿀 예정)
UINT CItemData::GetRefine() const
{
	return GetIndex()%10;
}

BOOL CItemData::IsEquipment() const
{
	switch (GetType())
	{
		case ITEM_TYPE_WEAPON:
		case ITEM_TYPE_ARMOR:
			return TRUE;
			break;
	}

	return FALSE;
}

#ifdef ENABLE_ACCE_COSTUME_SYSTEM
void CItemData::SetItemTableScaleData(int dwJob, int dwSex, float fScaleX, float fScaleY, float fScaleZ, float fScalePosX, float fScalePosY, float fScalePosZ, bool bMount)
{
	m_ItemScaleTable.scale[dwSex][bMount][dwJob].x = fScaleX;
	m_ItemScaleTable.scale[dwSex][bMount][dwJob].y = fScaleY;
	m_ItemScaleTable.scale[dwSex][bMount][dwJob].z = fScaleZ;

	m_ItemScaleTable.scalePos[dwSex][bMount][dwJob].x = fScalePosX;
	m_ItemScaleTable.scalePos[dwSex][bMount][dwJob].y = fScalePosY;
	m_ItemScaleTable.scalePos[dwSex][bMount][dwJob].z = fScalePosZ;
}

D3DXVECTOR3 & CItemData::GetItemScalePosition(int dwJob, int dwSex, bool bMount)
{
	return m_ItemScaleTable.scalePos[dwSex][bMount][dwJob];
}

D3DXVECTOR3 & CItemData::GetItemScale(int dwJob, int dwSex, bool bMount)
{
	return m_ItemScaleTable.scale[dwSex][bMount][dwJob];
}
#endif

void CItemData::Clear()
{
	m_strSummary = "";
	m_strModelFileName = "";
	m_strSubModelFileName = "";
	m_strDropModelFileName = "";
	m_strIconFileName = "";
	m_strLODModelFileNameVector.clear();

	m_pModelThing = NULL;
	m_pSubModelThing = NULL;
	m_pDropModelThing = NULL;
	m_pIconImage = NULL;
	m_pLODModelThingVector.clear();
#ifdef INGAME_WIKI
	m_isValidImage = true;
	m_wikiInfo.isSet = false;
	m_wikiInfo.hasData = false;
	m_isBlacklisted = false;
#endif

	m_ItemTable.Clear();
#ifdef ENABLE_ACCE_COSTUME_SYSTEM
	memset(&m_ItemScaleTable, 0, sizeof(m_ItemScaleTable));
#endif
}

CItemData::CItemData()
{
	Clear();
}

CItemData::~CItemData()
{
}
