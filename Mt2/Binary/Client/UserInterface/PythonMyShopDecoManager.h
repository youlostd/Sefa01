#pragma once
#include "InstanceBase.h"

class CPythonMyShopDecoManager : public CSingleton<CPythonMyShopDecoManager>
{
public:
	CPythonMyShopDecoManager();
	virtual ~CPythonMyShopDecoManager();

	void __Initialize();
	void Destroy();
	bool CreateBackground(DWORD dwWidth, DWORD dwHeight);
	CInstanceBase* GetInstancePtr(DWORD dwVnum);

	bool CreateModelInstance(DWORD dwVnum);
	bool SelectModel(DWORD dwVnum);
	void SetModelForm(DWORD dwVnum, bool isItem = true);
	bool SetModelHair(DWORD dwVnum, bool isItem = true);
	bool SetModelWeapon(DWORD dwVnum);
	bool SetModelArmor(DWORD dwVnum);
	void RenderBackground();
	void DeformModel();
	void RenderModel();
	void UpdateModel();
	void SetShow(bool bShow);

	void ClearInstances();

private:
	CInstanceBase * m_pModelInstance;
	CGraphicThingInstance *	m_pWeaponModel;
	DWORD m_dwHairNum;
	CGraphicImageInstance* m_pBackgroundImage;
	typedef std::map<DWORD, CInstanceBase *> TModelInstanceMap;
	TModelInstanceMap m_mapModelInstance;
	bool m_bShow;
	float m_fRot;
	D3DXVECTOR3 m_v3Eye;
	D3DXVECTOR3 m_v3Target;
	D3DXVECTOR3 m_v3Up;
};
