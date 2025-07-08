#include "StdAfx.h"
#include "PythonChat.h"

#include "AbstractApplication.h"
#include "PythonCharacterManager.h"
#include "PythonItem.h"
#include "PythonSkill.h"
#include "../gamelib/ItemManager.h"
#include "../eterbase/Timer.h"

int CPythonChat::TChatSet::ms_iChatModeSize = CHAT_TYPE_MAX_NUM;

const float c_fStartDisappearingTime = 5.0f;
const int c_iMaxLineCount = 5;

extern int g_iUseNewFont;

///////////////////////////////////////////////////////////////////////////////////////////////////

CDynamicPool<CPythonChat::SChatLine> CPythonChat::SChatLine::ms_kPool;

void CPythonChat::SetChatColor(UINT eType, UINT r, UINT g, UINT b)
{
	if (eType>=CHAT_TYPE_MAX_NUM)
		return;

	DWORD dwColor=(0xff000000)|(r<<16)|(g<<8)|(b);
	m_akD3DXClrChat[eType]=D3DXCOLOR(dwColor);	
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CPythonChat::SChatLine* CPythonChat::SChatLine::New()
{
	return ms_kPool.Alloc();
}

void CPythonChat::SChatLine::Delete(CPythonChat::SChatLine* pkChatLine)
{
	pkChatLine->FlagInstance.Destroy();
	pkChatLine->Instance.Destroy();
	ms_kPool.Free(pkChatLine);
}

void CPythonChat::SChatLine::DestroySystem()
{
	ms_kPool.Destroy();	
}

void CPythonChat::SChatLine::SetColor(DWORD dwID, DWORD dwColor)
{
	assert(dwID < CHAT_LINE_COLOR_ARRAY_MAX_NUM);

	if (dwID >= CHAT_LINE_COLOR_ARRAY_MAX_NUM)
		return;

	aColor[dwID] = dwColor;
}

void CPythonChat::SChatLine::SetColorAll(DWORD dwColor)
{
	for (int i = 0; i < CHAT_LINE_COLOR_ARRAY_MAX_NUM; ++i)
		aColor[i] = dwColor;
}

D3DXCOLOR & CPythonChat::SChatLine::GetColorRef(DWORD dwID)
{
	assert(dwID < CHAT_LINE_COLOR_ARRAY_MAX_NUM);

	if (dwID >= CHAT_LINE_COLOR_ARRAY_MAX_NUM)
	{
		static D3DXCOLOR color(1.0f, 0.0f, 0.0f, 1.0f);
		return color;
	}

	return aColor[dwID];
}

CPythonChat::SChatLine::SChatLine()
{
	for (int i = 0; i < CHAT_LINE_COLOR_ARRAY_MAX_NUM; ++i)
		aColor[i] = 0xff0000ff;
}
CPythonChat::SChatLine::~SChatLine() 
{
	FlagInstance.Destroy();
	Instance.Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

int CPythonChat::CreateChatSet(DWORD dwID)
{
	m_ChatSetMap.insert(make_pair(dwID, TChatSet()));
	return dwID;
}

void CPythonChat::UpdateViewMode(DWORD dwID)
{
	IAbstractApplication& rApp=IAbstractApplication::GetSingleton();

	float fcurTime = rApp.GetGlobalTime();
	//float felapsedTime = rApp.GetGlobalElapsedTime();

	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	TChatLineList * pLineList = &(pChatSet->m_ShowingChatLineList);
	int iLineIndex = pLineList->size();
	int iHeight = -(int(pLineList->size()+1) * pChatSet->m_iStep);

	TChatLineList::iterator itor;
	for (itor = pLineList->begin(); itor != pLineList->end();)
	{
		TChatLine * pChatLine = (*itor);

		D3DXCOLOR & rColor = pChatLine->GetColorRef(dwID);

		float fElapsedTime = (fcurTime - pChatLine->fAppendedTime);
		if (fElapsedTime >= c_fStartDisappearingTime || iLineIndex >= c_iMaxLineCount)
		{
			rColor.a -= rColor.a / 10.0f;

			if (rColor.a <= 0.1f)
			{
				itor = pLineList->erase(itor);
			}
			else
			{
				++itor;
			}
		}
		else
		{
			++itor;
		}

		/////

		iHeight += pChatSet->m_iStep;
		--iLineIndex;

		int x = pChatSet->m_ix;
		if (!pChatLine->FlagInstance.IsEmpty())
		{
			pChatLine->FlagInstance.SetPosition(x, pChatSet->m_iy + iHeight);
			x += pChatLine->FlagInstance.GetRenderWidth() + 5;
		}

		pChatLine->Instance.SetPosition(x, pChatSet->m_iy + iHeight);
		pChatLine->Instance.SetColor(rColor);
		pChatLine->Instance.Update();
	}
}

void CPythonChat::UpdateEditMode(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	const int c_iAlphaLine = max(0, GetVisibleLineCount(dwID) - GetEditableLineCount(dwID) + 2);

	int iLineIndex = 0;
	float fAlpha = 0.0f;
	float fAlphaStep = 0.0f;

	if (c_iAlphaLine > 0)
		fAlphaStep = 1.0f / float(c_iAlphaLine);

	TChatLineList * pLineList = &(pChatSet->m_ShowingChatLineList);
	int iHeight = -(int(pLineList->size()+1) * pChatSet->m_iStep);

	for (TChatLineList::iterator itor = pLineList->begin(); itor != pLineList->end(); ++itor)
	{
		TChatLine * pChatLine = (*itor);

		D3DXCOLOR & rColor = pChatLine->GetColorRef(dwID);

		if (iLineIndex < c_iAlphaLine)
		{
			rColor.a += (fAlpha - rColor.a) / 10.0f;
			fAlpha = fMIN(fAlpha+fAlphaStep, 1.0f);
		}
		else
		{
			rColor.a = fMIN(rColor.a+0.05f, 1.0f);
		}

		iHeight += pChatSet->m_iStep;

		int x = pChatSet->m_ix;
		if (!pChatLine->FlagInstance.IsEmpty())
		{
			pChatLine->FlagInstance.SetPosition(x, pChatSet->m_iy + iHeight);
			x += pChatLine->FlagInstance.GetRenderWidth() + 5;
		}

		pChatLine->Instance.SetPosition(x, pChatSet->m_iy + iHeight);
		pChatLine->Instance.SetColor(rColor);
		pChatLine->Instance.Update();
	}
}

void CPythonChat::UpdateLogMode(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	TChatLineList * pLineList = &(pChatSet->m_ShowingChatLineList);
	int iHeight = 0;

	for (TChatLineList::reverse_iterator itor = pLineList->rbegin(); itor != pLineList->rend(); ++itor)
	{
		TChatLine * pChatLine = (*itor);

		iHeight -= pChatSet->m_iStep;

		int x = pChatSet->m_ix;
		if (!pChatLine->FlagInstance.IsEmpty())
		{
			pChatLine->FlagInstance.SetPosition(x, pChatSet->m_iy + iHeight);
			x += pChatLine->FlagInstance.GetRenderWidth() + 5;
		}

		pChatLine->Instance.SetPosition(x, pChatSet->m_iy + iHeight);
		pChatLine->Instance.SetColor(pChatLine->GetColorRef(dwID));
		pChatLine->Instance.Update();
	}
}

void CPythonChat::Update(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	switch (pChatSet->m_iBoardState)
	{
		case BOARD_STATE_VIEW:
			UpdateViewMode(dwID);
			break;
		case BOARD_STATE_EDIT:
			UpdateEditMode(dwID);
			break;
		case BOARD_STATE_LOG:
			UpdateLogMode(dwID);
			break;
	}

	DWORD dwcurTime = CTimer::Instance().GetCurrentMillisecond();
	for (TWaitChatList::iterator itor = m_WaitChatList.begin(); itor != m_WaitChatList.end();)
	{
		TWaitChat & rWaitChat = *itor;

		if (rWaitChat.dwAppendingTime < dwcurTime)
		{
			AppendChat(rWaitChat.iType, rWaitChat.strChat.c_str());

			itor = m_WaitChatList.erase(itor);
		}
		else
		{
			++itor;
		}
	}
}

void CPythonChat::Render(DWORD dwID)
{
	TChatLineList * pLineList = GetChatLineListPtr(dwID);
	if (!pLineList)
		return;

	for (TChatLineList::iterator itor = pLineList->begin(); itor != pLineList->end(); ++itor)
	{
		TChatLine* pkLine = *itor;
		CGraphicImageInstance & rFlagInstance = pkLine->FlagInstance;
		CGraphicTextInstance & rInstance = pkLine->Instance;
		if (!rFlagInstance.IsEmpty())
			rFlagInstance.Render();
		rInstance.Render();
	}
}


void CPythonChat::SetBoardState(DWORD dwID, int iState)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_iBoardState = iState;
	ArrangeShowingChat(dwID);
}
void CPythonChat::SetPosition(DWORD dwID, int ix, int iy)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_ix = ix;
	pChatSet->m_iy = iy;
}
void CPythonChat::SetHeight(DWORD dwID, int iHeight)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_iHeight = iHeight;
}
void CPythonChat::SetStep(DWORD dwID, int iStep)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_iStep = iStep;
}
void CPythonChat::ToggleChatMode(DWORD dwID, int iMode)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_iMode[iMode] = 1 - pChatSet->m_iMode[iMode];
// 	Tracef("ToggleChatMode : %d\n", iMode);
	ArrangeShowingChat(dwID);
}
void CPythonChat::EnableChatMode(DWORD dwID, int iMode)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_iMode[iMode] = TRUE;
// 	Tracef("EnableChatMode : %d\n", iMode);
	ArrangeShowingChat(dwID);
}
void CPythonChat::DisableChatMode(DWORD dwID, int iMode)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_iMode[iMode] = FALSE;
// 	Tracef("DisableChatMode : %d\n", iMode);
	ArrangeShowingChat(dwID);
}
void CPythonChat::SetEndPos(DWORD dwID, float fPos)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	fPos = max(0.0f, fPos);
	fPos = min(1.0f, fPos);
	if (pChatSet->m_fEndPos != fPos)
	{
		pChatSet->m_fEndPos = fPos;
		ArrangeShowingChat(dwID);
	}
}


int CPythonChat::GetVisibleLineCount(DWORD dwID)
{
	TChatLineList * pLineList = GetChatLineListPtr(dwID);
	if (!pLineList)
		return 0;

	return pLineList->size();
}

int CPythonChat::GetEditableLineCount(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return 0;

	return pChatSet->m_iHeight / pChatSet->m_iStep + 1;
}

int CPythonChat::GetLineCount(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return 0;

	int iCount = 0;
	for (DWORD i = 0; i < m_ChatLineDeque.size(); ++i)
	{
		if (!pChatSet->CheckMode(m_ChatLineDeque[i]->iType))
			continue;

		++iCount;
	}

	return iCount;
}

int CPythonChat::GetLineStep(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return 0;

	return pChatSet->m_iStep;
}

CPythonChat::TChatLineList * CPythonChat::GetChatLineListPtr(DWORD dwID)
{
	TChatSetMap::iterator itor = m_ChatSetMap.find(dwID);
	if (m_ChatSetMap.end() == itor)
		return NULL;

	TChatSet & rChatSet = itor->second;
	return &(rChatSet.m_ShowingChatLineList);
}

CPythonChat::TChatSet * CPythonChat::GetChatSetPtr(DWORD dwID)
{
	TChatSetMap::iterator itor = m_ChatSetMap.find(dwID);
	if (m_ChatSetMap.end() == itor)
		return NULL;

	TChatSet & rChatSet = itor->second;
	return &rChatSet;
}

void CPythonChat::ArrangeShowingChat(DWORD dwID)
{
	TChatSet * pChatSet = GetChatSetPtr(dwID);
	if (!pChatSet)
		return;

	pChatSet->m_ShowingChatLineList.clear();

	TChatLineDeque TempChatLineDeque;
	for (TChatLineDeque::iterator itor = m_ChatLineDeque.begin(); itor != m_ChatLineDeque.end(); ++itor)
	{
		TChatLine * pChatLine = *itor;
		if (pChatSet->CheckMode(pChatLine->iType))
			TempChatLineDeque.push_back(pChatLine);
	}

	int icurLineCount = TempChatLineDeque.size();
	int iVisibleLineCount = min(icurLineCount, (pChatSet->m_iHeight + pChatSet->m_iStep) / pChatSet->m_iStep);
	int iEndLine = iVisibleLineCount + int(float(icurLineCount - iVisibleLineCount - 1) * pChatSet->m_fEndPos);

	/////

	int iHeight = 12;
	for (int i = min(icurLineCount-1, iEndLine); i >= 0; --i)
	{
		if (!pChatSet->CheckMode(TempChatLineDeque[i]->iType))
			continue;

		if (pChatSet->m_iHeight + pChatSet->m_iStep <= iHeight)
		{
			break;
		}

		pChatSet->m_ShowingChatLineList.push_front(TempChatLineDeque[i]);

		iHeight += pChatSet->m_iStep;
	}
}

void CPythonChat::AppendChat(int iType, const char * c_szChat)
{
	// DEFAULT_FONT
	//static CResource * s_pResource = CResourceManager::Instance().GetResourcePointer(g_strDefaultFontName.c_str());

	CGraphicText* pkDefaultFont;// = static_cast<CGraphicText*>(DefaultFont_GetResource());
	if (g_iUseNewFont == 1)
		pkDefaultFont = static_cast<CGraphicText*>(DefaultNewFont1_GetResource());
	else if (g_iUseNewFont == 2)
		pkDefaultFont = static_cast<CGraphicText*>(DefaultNewFont2_GetResource());
	else
		pkDefaultFont = static_cast<CGraphicText*>(DefaultFont_GetResource());

	if (!pkDefaultFont)
	{
		TraceError("CPythonChat::AppendChat - CANNOT_FIND_DEFAULT_FONT");
		return;
	}
	// END_OF_DEFAULT_FONT

	IAbstractApplication& rApp=IAbstractApplication::GetSingleton();
	SChatLine * pChatLine = SChatLine::New();
	pChatLine->iType = iType;

	if (iType != CHAT_TYPE_NOTICE && iType != CHAT_TYPE_BIG_NOTICE && iType != CHAT_TYPE_INFO)
	{
		const char* c_pszFind = strstr(c_szChat, " ");
		if (c_pszFind != NULL)
		{
			int iLanguageIdx = atoi(c_szChat)-1;
			if (iLanguageIdx >= 0 && iLanguageIdx < LANGUAGE_MAX_NUM)
			{
				c_szChat = c_pszFind + 1;

				char szFlagImageName[256];
				snprintf(szFlagImageName, sizeof(szFlagImageName), "d:/ymir work/ui/flags/32/%s.png", CLocaleManager::instance().GetLanguageNameByID(iLanguageIdx, false));

				// TraceError("%s:%d", __FILE__, __LINE__);
				CGraphicImage * pImage = (CGraphicImage *)CResourceManager::Instance().GetResourcePointer(szFlagImageName);
				if (pImage)
				{
					// TraceError("%s:%d", __FILE__, __LINE__);
					const int c_iFlagHeight = 15;

					pChatLine->FlagInstance.SetImagePointer(pImage);
					int iCurHeight = pChatLine->FlagInstance.GetHeight();
					if (iCurHeight > c_iFlagHeight)
					{
						float fScaleFactor = (float)c_iFlagHeight / (float)iCurHeight;
						pChatLine->FlagInstance.SetScale(fScaleFactor, fScaleFactor);
					}
				}
			}
			// else {
				// TraceError("%s:%d", __FILE__, __LINE__);
			// }
		}
	}
	
	// replace item names by current selected language item names for alle sent item hyperlinks
	std::string str = c_szChat;
	std::string findl1 = "|Hitem:";
	std::size_t find1 = 0;

	std::size_t pos1 = 0;
	for (int i = 0; i < str.length(); i++)
	{
		find1 = str.find(findl1, i);;
		if (find1 != std::string::npos)
		{
			pos1 = find1 + findl1.length();
			i = find1 + findl1.length();
			std::string item_vnumabc = str.substr(pos1, str.length() - pos1);
			std::size_t pos2 = item_vnumabc.find(":");
			std::string item_vnum = item_vnumabc.substr(0, pos2);
			char * p, * s0;
			long n = strtol(item_vnum.c_str(), &p, 16);
			
			std::string item_flag = item_vnumabc.substr(pos2 + 1, strlen(item_vnumabc.c_str()));
			std::string item_socket = item_flag.substr(item_flag.find(":")+1, strlen(item_flag.c_str()));
			item_socket = item_socket.substr(0, item_socket.find(":"));
			
			long socket0 = strtol(item_socket.c_str(), &s0, 16);

			if (*p != 0)
			{
				TraceError("%s a number %s", item_vnum, str);
			}
			else
			{
				std::size_t pos_1 = item_vnumabc.find("|h[");
				std::size_t pos_2 = item_vnumabc.find("]|h");
				std::string item_name = item_vnumabc.substr(pos_1 + 3, pos_2 - pos_1 - 3);

				CItemData * pItemData = NULL;
				if (CItemManager::Instance().GetItemDataPointer(n, &pItemData))
				{
					if (strcmp(item_name.c_str(), pItemData->GetName()) != 0)
					{
						if (socket0 && n == 50300)
						{
							CPythonSkill::SSkillData* c_pSkillData;
							const char* szSkillName = pItemData->GetName();
							if (CPythonSkill::Instance().GetSkillData(socket0, &c_pSkillData))
								szSkillName = c_pSkillData->strName.c_str();
							str = str.replace(pos1 + pos_1 + 3, item_name.length(), szSkillName);
						}
						else
						{
							str = str.replace(pos1 + pos_1 + 3, item_name.length(), pItemData->GetName());
						}

						c_szChat = str.c_str();
					}
				}
			}
		}
	}

	pChatLine->Instance.SetValue(c_szChat);

	// DEFAULT_FONT
	pChatLine->Instance.SetTextPointer(pkDefaultFont);
	// END_OF_DEFAULT_FONT

#ifdef __ARABIC_LANG__
	//if (GetDefaultCodePage() == CP_ARABIC)
		//pChatLine->Instance.SetHorizonalAlign(CGraphicTextInstance::HORIZONTAL_ALIGN_RIGHT);
#endif

	pChatLine->fAppendedTime = rApp.GetGlobalTime();
	pChatLine->SetColorAll(GetChatColor(iType));

	m_ChatLineDeque.push_back(pChatLine);
	if (m_ChatLineDeque.size() > CHAT_LINE_MAX_NUM)
	{
		SChatLine * pChatLine = m_ChatLineDeque.front();
		SChatLine::Delete(pChatLine);
		m_ChatLineDeque.pop_front();
	}

	for (TChatSetMap::iterator itor = m_ChatSetMap.begin(); itor != m_ChatSetMap.end(); ++itor)
	{
		TChatSet * pChatSet = &(itor->second);
		//pChatLine->SetColor(itor->first, GetChatColor(iType));

		// Edit Mode 를 억지로 끼워 맞추기 위해 추가
		if (BOARD_STATE_EDIT == pChatSet->m_iBoardState)
		{
			ArrangeShowingChat(itor->first);
		}
		else// if (BOARD_STATE_VIEW == pChatSet->m_iBoardState)
		{
			pChatSet->m_ShowingChatLineList.push_back(pChatLine);
			if (pChatSet->m_ShowingChatLineList.size() > CHAT_LINE_MAX_NUM)
			{
				pChatSet->m_ShowingChatLineList.pop_front();
			}
		}
	}
}

void CPythonChat::AppendChatWithDelay(int iType, const char * c_szChat, int iDelay)
{
	TWaitChat WaitChat;
	WaitChat.iType = iType;
	WaitChat.strChat = c_szChat;
	WaitChat.dwAppendingTime = CTimer::Instance().GetCurrentMillisecond() + iDelay;
	m_WaitChatList.push_back(WaitChat);
}

DWORD CPythonChat::GetChatColor(int iType)
{
	if (iType<CHAT_TYPE_MAX_NUM)
	{
		return m_akD3DXClrChat[iType];
	}

	return D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f);
}

void CPythonChat::IgnoreCharacter(const char * c_szName)
{
	TIgnoreCharacterSet::iterator itor = m_IgnoreCharacterSet.find(c_szName);
	// NOTE : 이미 차단 중이라면..
	if (m_IgnoreCharacterSet.end() != itor)
	{
		m_IgnoreCharacterSet.erase(itor);
	}
	// NOTE : 차단이 되지 않은 캐릭터라면..
	else
	{
		m_IgnoreCharacterSet.insert(c_szName);
	}
}

BOOL CPythonChat::IsIgnoreCharacter(const char * c_szName)
{
	TIgnoreCharacterSet::iterator itor = m_IgnoreCharacterSet.find(c_szName);

	if (m_IgnoreCharacterSet.end() == itor)
		return FALSE;

	return TRUE;
}

CWhisper * CPythonChat::CreateWhisper(const char * c_szName)
{
	CWhisper * pWhisper = CWhisper::New();
	m_WhisperMap.insert(TWhisperMap::value_type(c_szName, pWhisper));
	return pWhisper;
}

void CPythonChat::AppendWhisper(int iType, const char * c_szName, const char * c_szChat)
{
	TWhisperMap::iterator itor = m_WhisperMap.find(c_szName);

	CWhisper * pWhisper;
	if (itor == m_WhisperMap.end())
	{
		pWhisper = CreateWhisper(c_szName);
	}
	else
	{
		pWhisper = itor->second;
	}

	pWhisper->AppendChat(iType, c_szChat);
}

int CPythonChat::GetWhisperLineCount(const char * c_szName)
{
	TWhisperMap::iterator itor = m_WhisperMap.find(c_szName);

	CWhisper * pWhisper;
	if (itor == m_WhisperMap.end())
	{
		return 0;
	}
	else
	{
		pWhisper = itor->second;
		return pWhisper->GetLineCount();
	}
}

void CPythonChat::ClearWhisper(const char * c_szName)
{
	TWhisperMap::iterator itor = m_WhisperMap.find(c_szName);

	if (itor != m_WhisperMap.end())
	{
		CWhisper * pWhisper = itor->second;
		CWhisper::Delete(pWhisper);
		
		m_WhisperMap.erase(itor);
	}
}

BOOL CPythonChat::GetWhisper(const char * c_szName, CWhisper ** ppWhisper)
{
	TWhisperMap::iterator itor = m_WhisperMap.find(c_szName);

	if (itor == m_WhisperMap.end())
		return FALSE;

	*ppWhisper = itor->second;

	return TRUE;
}

void CPythonChat::InitWhisper(PyObject * ppyObject)
{
	TWhisperMap::iterator itor = m_WhisperMap.begin();
	for (; itor != m_WhisperMap.end(); ++itor)
	{
		std::string strName = itor->first;
		PyCallClassMemberFunc(ppyObject, "MakeWhisperButton", Py_BuildValue("(s)", strName.c_str()));
	}
}

void CPythonChat::__DestroyWhisperMap()
{
	TWhisperMap::iterator itor = m_WhisperMap.begin();
	for (; itor != m_WhisperMap.end(); ++itor)
	{
		CWhisper::Delete(itor->second);
	}
	m_WhisperMap.clear();
}

void CPythonChat::Close()
{
	TChatSetMap::iterator itor = m_ChatSetMap.begin();
	for (; itor != m_ChatSetMap.end(); ++itor)
	{
		TChatSet & rChatSet = itor->second;
		TChatLineList * pLineList = &(rChatSet.m_ShowingChatLineList);
		for (TChatLineList::iterator itor = pLineList->begin(); itor != pLineList->end(); ++itor)
		{
			TChatLine * pChatLine = (*itor);
			pChatLine->fAppendedTime = 0.0f;
		}
	}
}

void CPythonChat::Destroy()
{
	__DestroyWhisperMap();	

	m_ShowingChatLineList.clear();
	m_ChatSetMap.clear();
	m_ChatLineDeque.clear();

	SChatLine::DestroySystem();
	CWhisper::DestroySystem();

	__Initialize();
}

void CPythonChat::__Initialize()
{
	m_akD3DXClrChat[CHAT_TYPE_TALKING]		= D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_INFO]			= D3DXCOLOR(1.0f, 0.785f, 0.785f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_NOTICE]		= D3DXCOLOR(1.0f, 0.902f, 0.730f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_PARTY]		= D3DXCOLOR(0.542f, 1.0f, 0.949f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_GUILD]		= D3DXCOLOR(0.906f, 0.847f, 1.0f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_COMMAND]		= D3DXCOLOR(0.658f, 1.0f, 0.835f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_SHOUT]		= D3DXCOLOR(0.658f, 1.0f, 0.835f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_TEAM]			= D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
	m_akD3DXClrChat[CHAT_TYPE_WHISPER]		= D3DXCOLOR(0xff4AE14A);
	m_akD3DXClrChat[CHAT_TYPE_BIG_NOTICE]	= D3DXCOLOR(1.0f, 0.902f, 0.730f, 1.0f);
#ifdef ENABLE_ZODIAC
	m_akD3DXClrChat[CHAT_TYPE_ZODIAC_NOTICE]	= D3DXCOLOR(1.0f, 0.902f, 0.730f, 1.0f);
#endif
}

CPythonChat::CPythonChat()
{
	__Initialize();
}

CPythonChat::~CPythonChat()
{
	assert(m_ChatLineDeque.empty());
	assert(m_ShowingChatLineList.empty());
	assert(m_ChatSetMap.empty());
	assert(m_WhisperMap.empty());
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CDynamicPool<CWhisper> CWhisper::ms_kPool;

CWhisper* CWhisper::New()
{
	return ms_kPool.Alloc();
}

void CWhisper::Delete(CWhisper* pkWhisper)
{
	pkWhisper->Destroy();
	ms_kPool.Free(pkWhisper);
}

void CWhisper::DestroySystem()
{
	ms_kPool.Destroy();	

	SChatLine::DestroySystem();	
}

void CWhisper::SetPosition(float fPosition)
{
	m_fcurPosition = fPosition;
	__ArrangeChat();
}

void CWhisper::SetBoxSize(float fWidth, float fHeight)
{
	m_fWidth = fWidth;
	m_fHeight = fHeight;

	for (TChatLineDeque::iterator itor = m_ChatLineDeque.begin(); itor != m_ChatLineDeque.end(); ++itor)
	{
		TChatLine * pChatLine = *itor;
		pChatLine->Instance.SetLimitWidth(fWidth);
	}
}

void CWhisper::AppendChat(int iType, const char * c_szChat)
{
	// DEFAULT_FONT
	//static CResource * s_pResource = CResourceManager::Instance().GetResourcePointer(g_strDefaultFontName.c_str());

#if defined(LOCALE_SERVICE_YMIR) || defined(LOCALE_SERVICE_JAPAN) || defined(LOCALE_SERVICE_HONGKONG) || defined(LOCALE_SERVICE_TAIWAN) || defined(LOCALE_SERVICE_NEWCIBN)
	CGraphicText* pkDefaultFont = static_cast<CGraphicText*>(DefaultFont_GetResource());
#else
	//CGraphicText* pkDefaultFont = (iType == CPythonChat::WHISPER_TYPE_GM) ? static_cast<CGraphicText*>(DefaultItalicFont_GetResource()) : static_cast<CGraphicText*>(DefaultFont_GetResource());
	CGraphicText* pkDefaultFont = static_cast<CGraphicText*>(DefaultFont_GetResource());
#endif

	if (!pkDefaultFont)
	{
		TraceError("CWhisper::AppendChat - CANNOT_FIND_DEFAULT_FONT");
		return;
	}
	// END_OF_DEFAULT_FONT

	// replace item names by current selected language item names for alle sent item hyperlinks
	{
		std::string str = c_szChat;
		std::string findl1 = "|Hitem:";
		std::size_t find1 = 0;

		std::size_t pos1 = 0;
		for (int i = 0; i < str.length(); i++)
		{
			// find next hyperlink item
			find1 = str.find(findl1, i);;
			if (find1 != std::string::npos)
			{
				// get vnum
				pos1 = find1 + findl1.length();
				i = find1 + findl1.length();
				std::string item_vnumabc = str.substr(pos1, str.length() - pos1);
				std::size_t pos2 = item_vnumabc.find(":");
				std::string item_vnum = item_vnumabc.substr(0, pos2);
				char * p;
				long n = strtol(item_vnum.c_str(), &p, 16);
				if (*p != 0)
				{
					TraceError("%s a number %s", item_vnum, str);
				}
				else
				{
					// get sent item name
					std::size_t pos_1 = item_vnumabc.find("|h[");
					std::size_t pos_2 = item_vnumabc.find("]|h");
					std::string item_name = item_vnumabc.substr(pos_1 + 3, pos_2 - pos_1 - 3);

					CItemData * pItemData = NULL;
					if (CItemManager::Instance().GetItemDataPointer(n, &pItemData))
					{
						// if current item name differs from sent item name replace it
						if (strcmp(item_name.c_str(), pItemData->GetName()) != 0)
						{
							str = str.replace(pos1 + pos_1 + 3, item_name.length(), pItemData->GetName());
							c_szChat = str.c_str();
						}
					}
				}
			}
		}
	}
	
	SChatLine * pChatLine = SChatLine::New();
	pChatLine->Instance.SetValue(c_szChat);

	// DEFAULT_FONT
	pChatLine->Instance.SetTextPointer(pkDefaultFont);
	// END_OF_DEFAULT_FONT

	pChatLine->Instance.SetLimitWidth(m_fWidth);
	pChatLine->Instance.SetMultiLine(TRUE);

	switch(iType)
	{
		case CPythonChat::WHISPER_TYPE_SYSTEM:
			pChatLine->Instance.SetColor(D3DXCOLOR(1.0f, 0.785f, 0.785f, 1.0f));
			break;
		case CPythonChat::WHISPER_TYPE_GM:
			pChatLine->Instance.SetColor(D3DXCOLOR(1.0f, 0.632f, 0.0f, 1.0f));
			break;
		case CPythonChat::WHISPER_TYPE_NORMAL:
		default:
			pChatLine->Instance.SetColor(0xffffffff);
			break;
	}

	m_ChatLineDeque.push_back(pChatLine);

	__ArrangeChat();
}

void CWhisper::__ArrangeChat()
{
	for (TChatLineDeque::iterator itor = m_ChatLineDeque.begin(); itor != m_ChatLineDeque.end(); ++itor)
	{
		TChatLine * pChatLine = *itor;
		pChatLine->Instance.Update();
	}
}

void CWhisper::Render(float fx, float fy)
{
	float fHeight = fy + m_fHeight;

	int iViewCount = int(m_fHeight / m_fLineStep) - 1;
	int iLineCount = int(m_ChatLineDeque.size());
	int iStartLine = -1;
	if (iLineCount > iViewCount)
	{
		iStartLine = int(float(iLineCount-iViewCount) * m_fcurPosition) + iViewCount - 1;
	}
	else if (!m_ChatLineDeque.empty())
	{
		iStartLine = iLineCount - 1;
	}

	RECT Rect = { fx, fy, fx+m_fWidth, fy+m_fHeight };

	for (int i = iStartLine; i >= 0; --i)
	{
		assert(i >= 0 && i < int(m_ChatLineDeque.size()));
		TChatLine * pChatLine = m_ChatLineDeque[i];

		WORD wLineCount = pChatLine->Instance.GetTextLineCount();
		fHeight -= wLineCount * m_fLineStep;

		pChatLine->Instance.SetPosition(fx, fHeight);
		pChatLine->Instance.Render(&Rect);

		if (fHeight < fy)
			break;
	}
}

void CWhisper::__Initialize()
{
	m_fLineStep = 15.0f;
	m_fWidth = 300.0f;
	m_fHeight = 120.0f;
	m_fcurPosition = 1.0f;
}

void CWhisper::Destroy()
{
	std::for_each(m_ChatLineDeque.begin(), m_ChatLineDeque.end(), SChatLine::Delete);
	m_ChatLineDeque.clear();
	m_ShowingChatLineList.clear();
}

CWhisper::CWhisper()
{
	__Initialize();
}

CWhisper::~CWhisper()
{
	Destroy();
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CDynamicPool<CWhisper::SChatLine> CWhisper::SChatLine::ms_kPool;

CWhisper::SChatLine* CWhisper::SChatLine::New()
{
	return ms_kPool.Alloc();
}

void CWhisper::SChatLine::Delete(CWhisper::SChatLine* pkChatLine)
{
	pkChatLine->Instance.Destroy();
	ms_kPool.Free(pkChatLine);
}

void CWhisper::SChatLine::DestroySystem()
{
	ms_kPool.Destroy();	
}

void TestChat(const char* c_szString)
{
//	CPythonChat::Instance().AppendChat(CHAT_TYPE_INFO, c_szString);
}
