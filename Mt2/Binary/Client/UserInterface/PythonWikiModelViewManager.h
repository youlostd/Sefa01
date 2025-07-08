#pragma once
#include "InstanceBase.h"
#include "../EterPythonLib/PythonWindow.h"

class CPythonWikiModelView final
{
public:
	CPythonWikiModelView(size_t addID);
	~CPythonWikiModelView();

	void SetV3Eye(float x, float y, float z) { m_v3Eye.x = x; m_v3Eye.y = y; m_v3Eye.z = z; }
	void SetV3Target(float x, float y, float z) { m_v3Target.x = x; m_v3Target.y = y; m_v3Target.z = z; }
	void SetV3Up(float x, float y, float z) { m_v3Up.x = x; m_v3Up.y = y; m_v3Up.z = z; }
	void SetModel(DWORD vnum);
	void SetModelHair(DWORD vnum, bool isItem = true);
	void SetModelForm(DWORD vnum, bool isItem = true);
	void SetWeaponModel(DWORD vnum);
	void RenderModel();
	void DeformModel();
	void UpdateModel();
	void SetShow(bool bShow) { m_bShow = bShow; }

	void RegisterWindow(int hWnd);

	void ClearInstance();

	const size_t GetID() const { return m_modulID; }

private:
	CGraphicThingInstance* m_pWeaponModel;
	CInstanceBase * m_pModelInstance;
	DWORD m_dwHairNum;
	bool m_bShow;
	float m_fRot;
	size_t m_modulID;
	UI::CRenderTarget* m_pyWindow;
	D3DXVECTOR3 m_v3Eye;
	D3DXVECTOR3 m_v3Target;
	D3DXVECTOR3 m_v3Up;
};

class CPythonWikiModelViewManager : public CSingleton<CPythonWikiModelViewManager>
{
public:
	CPythonWikiModelViewManager();
	virtual ~CPythonWikiModelViewManager();

	void AddView(DWORD addID);
	void RemoveView(DWORD addID);
	DWORD GetFreeID();
	CPythonWikiModelView* GetModule(DWORD moduleID);
	void DeformModel();
	void RenderModel();
	void UpdateModel();
	void ClearInstances();
	void SetShow(bool bShow) { m_bShow = bShow; }

private:
	typedef std::vector<CPythonWikiModelView *> TModelView;
	TModelView m_vecModelView;
	bool m_bShow;
};
