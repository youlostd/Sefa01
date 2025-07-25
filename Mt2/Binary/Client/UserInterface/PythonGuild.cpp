#include "StdAfx.h"
#include "PythonGuild.h"
#include "AbstractPlayer.h"
#include "MarkManager.h"

std::map<DWORD, DWORD> g_GuildSkillSlotToIndexMap;

void CPythonGuild::EnableGuild()
{
	m_bGuildEnable = TRUE;
}

void CPythonGuild::SetGuildMoney(DWORD dwMoney)
{
	m_GuildInfo.dwGuildMoney = dwMoney;
}

void CPythonGuild::SetGuildEXP(BYTE byLevel, DWORD dwEXP)
{
	m_GuildInfo.dwGuildLevel = byLevel;
	m_GuildInfo.dwCurrentExperience = dwEXP;
}

void CPythonGuild::SetGradeData(BYTE byGradeNumber, TGuildGradeData & rGuildGradeData)
{
	m_GradeDataMap[byGradeNumber] = rGuildGradeData;
}

void CPythonGuild::SetGradeName(BYTE byGradeNumber, const char * c_szName)
{
	if (!__IsGradeData(byGradeNumber))
		return;

	TGuildGradeData & rGradeData = m_GradeDataMap.find(byGradeNumber)->second;
	rGradeData.strName = c_szName;
}

void CPythonGuild::SetGradeAuthority(BYTE byGradeNumber, WORD wAuthority)
{
	if (!__IsGradeData(byGradeNumber))
		return;

	TGuildGradeData & rGradeData = m_GradeDataMap.find(byGradeNumber)->second;
	rGradeData.wAuthorityFlag = wAuthority;
}

void CPythonGuild::ClearComment()
{
	m_GuildBoardCommentVector.clear();
}

void CPythonGuild::RegisterComment(DWORD dwCommentID, const char * c_szName, const char * c_szComment)
{
	if (0 == strlen(c_szComment))
		return;

	TGuildBoardCommentData CommentData;
	CommentData.dwCommentID = dwCommentID;
	CommentData.strName = c_szName;
	CommentData.strComment = c_szComment;

	m_GuildBoardCommentVector.push_back(CommentData);
}

void CPythonGuild::RegisterMember(TGuildMemberData & rGuildMemberData)
{
	TGuildMemberData * pGuildMemberData;
	if (GetMemberDataPtrByPID(rGuildMemberData.dwPID, &pGuildMemberData))
	{
		pGuildMemberData->byGeneralFlag = rGuildMemberData.byGeneralFlag;
		pGuildMemberData->byGrade = rGuildMemberData.byGrade;
		pGuildMemberData->byLevel = rGuildMemberData.byLevel;
		pGuildMemberData->dwOffer = rGuildMemberData.dwOffer;
	}
	else
	{
		m_GuildMemberDataVector.push_back(rGuildMemberData);
	}

	__CalculateLevelAverage();
	__SortMember();
}

struct CPythonGuild_FFindGuildMemberByPID
{
	CPythonGuild_FFindGuildMemberByPID(DWORD dwSearchingPID_) : dwSearchingPID(dwSearchingPID_) {}
	int operator () (CPythonGuild::TGuildMemberData & rGuildMemberData)
	{
		return rGuildMemberData.dwPID == dwSearchingPID;
	}

	DWORD dwSearchingPID;
};

struct CPythonGuild_FFindGuildMemberByName
{
	CPythonGuild_FFindGuildMemberByName(const char * c_szSearchingName) : strSearchingName(c_szSearchingName) {}
	int operator () (CPythonGuild::TGuildMemberData & rGuildMemberData)
	{
		return 0 == strSearchingName.compare(rGuildMemberData.strName.c_str());
	}

	std::string strSearchingName;
};

void CPythonGuild::ChangeGuildMemberGrade(DWORD dwPID, BYTE byGrade)
{
	TGuildMemberData * pGuildMemberData;
	if (!GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		return;

	pGuildMemberData->byGrade = byGrade;
}

void CPythonGuild::ChangeGuildMemberGeneralFlag(DWORD dwPID, BYTE byFlag)
{
	TGuildMemberData * pGuildMemberData;
	if (!GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		return;

	pGuildMemberData->byGeneralFlag = byFlag;
}

void CPythonGuild::RemoveMember(DWORD dwPID)
{
	TGuildMemberDataVector::iterator itor;
	itor = std::find_if(	m_GuildMemberDataVector.begin(),
							m_GuildMemberDataVector.end(),
							CPythonGuild_FFindGuildMemberByPID(dwPID));

	if (m_GuildMemberDataVector.end() == itor)
		return;

	m_GuildMemberDataVector.erase(itor);
}

void CPythonGuild::RegisterGuildName(DWORD dwID, const char * c_szName)
{
	m_GuildNameMap.insert(make_pair(dwID, std::string(c_szName)));
}

BOOL CPythonGuild::IsMainPlayer(DWORD dwPID)
{
	TGuildMemberData * pGuildMemberData;
	if (!GetMemberDataPtrByPID(dwPID, &pGuildMemberData))
		return FALSE;

	IAbstractPlayer& rPlayer=IAbstractPlayer::GetSingleton();
	return 0 == pGuildMemberData->strName.compare(rPlayer.GetName());
}

BOOL CPythonGuild::IsGuildEnable()
{
	return m_bGuildEnable;
}

CPythonGuild::TGuildInfo & CPythonGuild::GetGuildInfoRef()
{
	return m_GuildInfo;
}

void CPythonGuild::GetRankAndPoint(DWORD &point, DWORD &rank)
{
	point = m_GuildInfo.dwGuildPoint;
	rank = m_GuildInfo.dwGuildRank;
}

void CPythonGuild::GetWarStats(BYTE index, DWORD &win, DWORD &draw, DWORD &loss)
{
	if (index >= sizeof(m_GuildInfo.dwWin) / sizeof(m_GuildInfo.dwWin[0]))
		return;

	win = m_GuildInfo.dwWin[index];
	draw = m_GuildInfo.dwDraw[index];
	loss = m_GuildInfo.dwLoss[index];
}

BOOL CPythonGuild::GetGradeDataPtr(DWORD dwGradeNumber, TGuildGradeData ** ppData)
{
	TGradeDataMap::iterator itor = m_GradeDataMap.find(dwGradeNumber);
	if (m_GradeDataMap.end() == itor)
		return FALSE;

	*ppData = &(itor->second);

	return TRUE;
}

const CPythonGuild::TGuildBoardCommentDataVector & CPythonGuild::GetGuildBoardCommentVector()
{
	return m_GuildBoardCommentVector;
}

DWORD CPythonGuild::GetMemberCount()
{
	return m_GuildMemberDataVector.size();
}

BOOL CPythonGuild::GetMemberDataPtr(DWORD dwIndex, TGuildMemberData ** ppData)
{
	if (dwIndex >= m_GuildMemberDataVector.size())
		return FALSE;

	*ppData = &m_GuildMemberDataVector[dwIndex];

	return TRUE;
}

BOOL CPythonGuild::GetMemberDataPtrByPID(DWORD dwPID, TGuildMemberData ** ppData)
{
	TGuildMemberDataVector::iterator itor;
	itor = std::find_if(	m_GuildMemberDataVector.begin(),
							m_GuildMemberDataVector.end(),
							CPythonGuild_FFindGuildMemberByPID(dwPID));

	if (m_GuildMemberDataVector.end() == itor)
		return FALSE;

	*ppData = &(*itor);
	return TRUE;
}

BOOL CPythonGuild::GetMemberDataPtrByName(const char * c_szName, TGuildMemberData ** ppData)
{
	TGuildMemberDataVector::iterator itor;
	itor = std::find_if(	m_GuildMemberDataVector.begin(),
							m_GuildMemberDataVector.end(),
							CPythonGuild_FFindGuildMemberByName(c_szName));

	if (m_GuildMemberDataVector.end() == itor)
		return FALSE;

	*ppData = &(*itor);
	return TRUE;
}

DWORD CPythonGuild::GetGuildMemberLevelSummary()
{
	return m_dwMemberLevelSummary;
}

DWORD CPythonGuild::GetGuildMemberLevelAverage()
{
	return m_dwMemberLevelAverage;
}

DWORD CPythonGuild::GetGuildExperienceSummary()
{
	return m_dwMemberExperienceSummary;
}

CPythonGuild::TGuildSkillData & CPythonGuild::GetGuildSkillDataRef()
{
	return m_GuildSkillData;
}

bool CPythonGuild::GetGuildName(DWORD dwID, std::string * pstrGuildName)
{
	if (m_GuildNameMap.end() == m_GuildNameMap.find(dwID))
		return false;

	*pstrGuildName = m_GuildNameMap[dwID];

	return true;
}

DWORD CPythonGuild::GetGuildID()
{
	return m_GuildInfo.dwGuildID;
}

BOOL CPythonGuild::HasGuildLand()
{
	return m_GuildInfo.bHasLand;
}

void CPythonGuild::StartGuildWar(DWORD dwEnemyGuildID)
{
	int i;

	for (i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (dwEnemyGuildID == m_adwEnemyGuildID[i])
			return;

	for (i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (0 == m_adwEnemyGuildID[i])
		{
			m_adwEnemyGuildID[i] = dwEnemyGuildID;
			break;
		}
}

void CPythonGuild::EndGuildWar(DWORD dwEnemyGuildID)
{
	for (int i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (dwEnemyGuildID == m_adwEnemyGuildID[i])
			m_adwEnemyGuildID[i] = 0;
}

DWORD CPythonGuild::GetEnemyGuildID(DWORD dwIndex)
{
	if (dwIndex >= ENEMY_GUILD_SLOT_MAX_COUNT)
		return 0;

	return m_adwEnemyGuildID[dwIndex];
}

BOOL CPythonGuild::IsDoingGuildWar()
{
	for (int i = 0; i < ENEMY_GUILD_SLOT_MAX_COUNT; ++i)
		if (0 != m_adwEnemyGuildID[i])
		{
			return TRUE;
		}

	return FALSE;
}

void CPythonGuild::__CalculateLevelAverage()
{
	m_dwMemberLevelSummary = 0;
	m_dwMemberLevelAverage = 0;
	m_dwMemberExperienceSummary = 0;

	if (m_GuildMemberDataVector.empty())
		return;

	TGuildMemberDataVector::iterator itor;

	// Sum Level & Experience
	itor = m_GuildMemberDataVector.begin();
	for (; itor != m_GuildMemberDataVector.end(); ++itor)
	{
		TGuildMemberData & rGuildMemberData = *itor;
		m_dwMemberLevelSummary += rGuildMemberData.byLevel;
		m_dwMemberExperienceSummary += rGuildMemberData.dwOffer;
	}

	assert(!m_GuildMemberDataVector.empty());
	m_dwMemberLevelAverage = m_dwMemberLevelSummary / m_GuildMemberDataVector.size();
}

struct CPythonGuild_SLessMemberGrade
{
	bool operator() (CPythonGuild::TGuildMemberData & rleft, CPythonGuild::TGuildMemberData & rright)
	{
		if (rleft.byGrade < rright.byGrade)
			return true;

		return false;
	}
};

void CPythonGuild::__SortMember()
{
	std::sort(m_GuildMemberDataVector.begin(), m_GuildMemberDataVector.end(), CPythonGuild_SLessMemberGrade());
}

BOOL CPythonGuild::__IsGradeData(BYTE byGradeNumber)
{
	return m_GradeDataMap.end() != m_GradeDataMap.find(byGradeNumber);
}

DWORD CPythonGuild::GetMaximumExp(BYTE level) const
{
	static const BYTE GUILD_LEVEL_MAX = 20;

	static DWORD INTERNATIONAL_GUILDEXP_LIST[GUILD_LEVEL_MAX + 1] =
	{
		0,			// 0
		6000UL,		// 1
		18000UL,	// 2
		36000UL,	// 3
		64000UL,	// 4
		94000UL,	// 5
		130000UL,	// 6
		172000UL,	// 7
		220000UL,	// 8
		274000UL,	// 9
		334000UL,	// 10
		400000UL,	// 11
		600000UL,	// 12
		840000UL,	// 13
		1120000UL,	// 14
		1440000UL,	// 15
		1800000UL,	// 16
		2600000UL,	// 17
		3200000UL,	// 18
		4000000UL,	// 19		
		16800000UL	// 20		
	};

	if (level < 0 || level >= GUILD_LEVEL_MAX)
		return 0;

	return INTERNATIONAL_GUILDEXP_LIST[level];
}

void CPythonGuild::SetLadder(guildLadder *ladder, BYTE ladderCount, DWORD currPage, DWORD maxPage, DWORD searchRank)
{
	m_guildLadder.currPage = currPage;
	m_guildLadder.resultCount = ladderCount;
	m_guildLadder.maxPage = maxPage;
	m_guildLadder.searchRank = searchRank;
	if (ladder)
		memcpy(m_guildLadder.ladder, ladder, sizeof(guildLadder) * ladderCount);
}

void CPythonGuild::__Initialize()
{
	ZeroMemory(&m_GuildInfo, sizeof(m_GuildInfo));
	ZeroMemory(&m_GuildSkillData, sizeof(m_GuildSkillData));
	ZeroMemory(&m_adwEnemyGuildID, ENEMY_GUILD_SLOT_MAX_COUNT*sizeof(DWORD));
	ZeroMemory(&m_guildLadder, sizeof(m_guildLadder));
	m_GradeDataMap.clear();
	m_GuildMemberDataVector.clear();
	m_dwMemberLevelSummary = 0;
	m_dwMemberLevelAverage = 0;
	m_bGuildEnable = FALSE;
	m_GuildNameMap.clear();
}

void CPythonGuild::Destroy()
{
	__Initialize();
}

CPythonGuild::CPythonGuild()
{
	__Initialize();
}
CPythonGuild::~CPythonGuild()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

PyObject * guildIsGuildEnable(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().IsGuildEnable());
}

PyObject * guildGetGuildID(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().GetGuildID());
}

PyObject * guildHasGuildLand(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().HasGuildLand());
}

PyObject * guildGetGuildName(PyObject * poSelf, PyObject * poArgs)
{
	int iGuildID;
	if (!PyTuple_GetInteger(poArgs, 0, &iGuildID))
	{
		CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
		return Py_BuildValue("s", rGuildInfo.szGuildName);
	}

	std::string strGuildName;
	if (!CPythonGuild::Instance().GetGuildName(iGuildID, &strGuildName))
		return Py_BuildValue("s", "Noname");

	return Py_BuildValue("s", strGuildName.c_str());
}

PyObject * guildGetGuildMasterName(PyObject * poSelf, PyObject * poArgs)
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();

	CPythonGuild::TGuildMemberData * pData;
	if (!CPythonGuild::Instance().GetMemberDataPtrByPID(rGuildInfo.dwMasterPID, &pData))
		return Py_BuildValue("s", "Noname");

	return Py_BuildValue("s", pData->strName.c_str());
}

PyObject * guildGetEnemyGuildName(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildValue("s", "");

	DWORD dwEnemyGuildID = CPythonGuild::Instance().GetEnemyGuildID(iIndex);

	std::string strEnemyGuildName;
	if (!CPythonGuild::Instance().GetGuildName(dwEnemyGuildID, &strEnemyGuildName))
		return Py_BuildValue("s", "");

	return Py_BuildValue("s", strEnemyGuildName.c_str());
}

PyObject * guildGetGuildMoney(PyObject * poSelf, PyObject * poArgs)
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
	return Py_BuildValue("i", rGuildInfo.dwGuildMoney);
}

PyObject * guildGetGuildBoardCommentCount(PyObject * poSelf, PyObject * poArgs)
{
	const CPythonGuild::TGuildBoardCommentDataVector & rCommentVector = CPythonGuild::Instance().GetGuildBoardCommentVector();
	return Py_BuildValue("i", rCommentVector.size());
}

PyObject * guildGetGuildBoardCommentData(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	const CPythonGuild::TGuildBoardCommentDataVector & c_rCommentVector = CPythonGuild::Instance().GetGuildBoardCommentVector();
	if (DWORD(iIndex) >= c_rCommentVector.size())
		return Py_BuildValue("iss", 0, "Noname", "Noname");

	const CPythonGuild::TGuildBoardCommentData & c_rData = c_rCommentVector[iIndex];

	return Py_BuildValue("iss", c_rData.dwCommentID, c_rData.strName.c_str(), c_rData.strComment.c_str());
}

PyObject * guildGetGuildLevel(PyObject * poSelf, PyObject * poArgs)
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
	return Py_BuildValue("i", rGuildInfo.dwGuildLevel);
}



PyObject * guildGetGuildExperience(PyObject * poSelf, PyObject * poArgs)
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();

	int GULID_MAX_LEVEL = 20;
	if (rGuildInfo.dwGuildLevel >= GULID_MAX_LEVEL)
		return Py_BuildValue("ii", 0, 0);

	DWORD maxExp = CPythonGuild::Instance().GetMaximumExp(rGuildInfo.dwGuildLevel);

	return Py_BuildValue("ii", rGuildInfo.dwCurrentExperience, maxExp);
}

PyObject * guildGetGuildMemberCount(PyObject * poSelf, PyObject * poArgs)
{
	CPythonGuild::TGuildInfo & rGuildInfo = CPythonGuild::Instance().GetGuildInfoRef();
	return Py_BuildValue("ii", rGuildInfo.dwCurrentMemberCount, rGuildInfo.dwMaxMemberCount);
}

PyObject * guildGetGuildMemberLevelSummary(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().GetGuildMemberLevelSummary());
}

PyObject * guildGetGuildMemberLevelAverage(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().GetGuildMemberLevelAverage());
}

PyObject * guildGetGuildExperienceSummary(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().GetGuildExperienceSummary());
}

PyObject * guildGetGuildSkillPoint(PyObject * poSelf, PyObject * poArgs)
{
	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	return Py_BuildValue("i", c_rSkillData.bySkillPoint);
}

PyObject * guildGetDragonPowerPoint(PyObject * poSelf, PyObject * poArgs)
{
	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	return Py_BuildValue("ii", c_rSkillData.wGuildPoint, c_rSkillData.wMaxGuildPoint);
}

PyObject * guildGetGuildSkillLevel(PyObject * poSelf, PyObject * poArgs)
{
	assert(FALSE && !"guildGetGuildSkillLevel - 사용하지 않는 함수입니다.");

	int iSkillIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSkillIndex))
		return Py_BuildException();

	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	return Py_BuildValue("i", c_rSkillData.bySkillLevel[iSkillIndex]);
}

PyObject * guildGetSkillLevel(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	std::map<DWORD, DWORD>::iterator itor = g_GuildSkillSlotToIndexMap.find(iSlotIndex);

	if (g_GuildSkillSlotToIndexMap.end() == itor)
		return Py_BuildValue("i", 0);

	DWORD dwSkillIndex = itor->second;
	assert(dwSkillIndex < CPythonGuild::GUILD_SKILL_MAX_NUM);

	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	return Py_BuildValue("i", c_rSkillData.bySkillLevel[dwSkillIndex]);
}

PyObject * guildGetSkillMaxLevelNew(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	std::map<DWORD, DWORD>::iterator itor = g_GuildSkillSlotToIndexMap.find(iSlotIndex);

	if (g_GuildSkillSlotToIndexMap.end() == itor)
		return Py_BuildValue("i", 0);

	DWORD dwSkillIndex = itor->second;
	assert(dwSkillIndex < CPythonGuild::GUILD_SKILL_MAX_NUM);

	const CPythonGuild::TGuildSkillData & c_rSkillData = CPythonGuild::Instance().GetGuildSkillDataRef();
	return Py_BuildValue("i", c_rSkillData.bySkillLevel[dwSkillIndex]);
}

PyObject * guildSetSkillIndex(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();
	int iSkillIndex;
	if (!PyTuple_GetInteger(poArgs, 1, &iSkillIndex))
		return Py_BuildException();

	g_GuildSkillSlotToIndexMap.insert(make_pair(iSlotIndex, iSkillIndex));

	return Py_BuildNone();
}

PyObject * guildGetSkillIndex(PyObject * poSelf, PyObject * poArgs)
{
	int iSlotIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iSlotIndex))
		return Py_BuildException();

	std::map<DWORD, DWORD>::iterator itor = g_GuildSkillSlotToIndexMap.find(iSlotIndex);

	if (g_GuildSkillSlotToIndexMap.end() == itor)
		return Py_BuildValue("i", 0);

	DWORD dwSkillIndex = itor->second;
	return Py_BuildValue("i", dwSkillIndex);
}

PyObject * guildGetGradeData(PyObject * poSelf, PyObject * poArgs)
{
	int iGradeNumber;
	if (!PyTuple_GetInteger(poArgs, 0, &iGradeNumber))
		return Py_BuildException();

	CPythonGuild::TGuildGradeData * pData;
	if (!CPythonGuild::Instance().GetGradeDataPtr(iGradeNumber, &pData))
		return Py_BuildValue("si", "?", 0);

	return Py_BuildValue("si", pData->strName.c_str(), pData->wAuthorityFlag);
}

PyObject * guildGetGradeName(PyObject * poSelf, PyObject * poArgs)
{
	int iGradeNumber;
	if (!PyTuple_GetInteger(poArgs, 0, &iGradeNumber))
		return Py_BuildException();

	CPythonGuild::TGuildGradeData * pData;
	if (!CPythonGuild::Instance().GetGradeDataPtr(iGradeNumber, &pData))
		return Py_BuildValue("s", "?");

	return Py_BuildValue("s", pData->strName.c_str());
}

PyObject * guildGetMemberCount(PyObject * poSelf, PyObject * poArgs)
{
	return Py_BuildValue("i", CPythonGuild::Instance().GetMemberCount());
}

PyObject * guildGetMemberData(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	CPythonGuild::TGuildMemberData * pData;
	if (!CPythonGuild::Instance().GetMemberDataPtr(iIndex, &pData))
		return Py_BuildValue("isiiiii", -1, "", 0, 0, 0, 0, 0);

	return Py_BuildValue("isiiiii", pData->dwPID, pData->strName.c_str(), pData->byGrade, pData->byJob, pData->byLevel, pData->dwOffer, pData->byGeneralFlag);
}

PyObject * guildMemberIndexToPID(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	CPythonGuild::TGuildMemberData * pData;
	if (!CPythonGuild::Instance().GetMemberDataPtr(iIndex, &pData))
		return Py_BuildValue("i", -1);

	return Py_BuildValue("i", pData->dwPID);
}

PyObject *guildGetRankingPageInfo(PyObject * poSelf, PyObject * poArgs)
{
	const CPythonGuild::TGuildLadder& ref = CPythonGuild::instance().GetLadderRef();
	return Py_BuildValue("iii", ref.maxPage, ref.currPage, ref.resultCount);
}

PyObject *guildGetRankingInfo(PyObject * poSelf, PyObject * poArgs)
{
	BYTE line;
	BYTE type;
	if (!PyTuple_GetByte(poArgs, 3, &line))
		return Py_BuildException();

	if (!PyTuple_GetByte(poArgs, 1, &type))
		return Py_BuildException();

	const CPythonGuild::TGuildLadder& ref = CPythonGuild::instance().GetLadderRef();
	if (line >= ref.resultCount)
		return Py_BuildValue("siiiiii", "", 0, 0, 0, 0, 0, 0);
	else
		return Py_BuildValue("siiiiii", ref.ladder[line].name, ref.ladder[line].level, ref.ladder[line].ladderPoints,
			ref.ladder[line].minMember, ref.ladder[line].maxMember, 0, 
			type == CPythonGuild::RANKING_INFO_SEARCH ? ref.searchRank : ref.currPage * CPythonGuild::GUILD_RANKING_RESULT_PER_PAGE + line + 1);
}

PyObject * guildIsMember(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BuildException();

	CPythonGuild::TGuildMemberData * pData;
	if (CPythonGuild::Instance().GetMemberDataPtr(iIndex, &pData))
		return Py_BuildValue("i", TRUE);

	return Py_BuildValue("i", FALSE);
}

PyObject * guildIsMemberByName(PyObject * poSelf, PyObject * poArgs)
{
	char * szName;
	if (!PyTuple_GetString(poArgs, 0, &szName))
		return Py_BuildException();

	CPythonGuild::TGuildMemberData * pData;
	if (CPythonGuild::Instance().GetMemberDataPtrByName(szName, &pData))
		return Py_BuildValue("i", TRUE);

	return Py_BuildValue("i", FALSE);
}

PyObject * guildMainPlayerHasAuthority(PyObject * poSelf, PyObject * poArgs)
{
	int iAuthority;
	if (!PyTuple_GetInteger(poArgs, 0, &iAuthority))
		return Py_BuildException();

	IAbstractPlayer& rPlayer=IAbstractPlayer::GetSingleton();
	const char * c_szMainPlayerName = rPlayer.GetName();

	CPythonGuild::TGuildMemberData * pMemberData;
	if (!CPythonGuild::Instance().GetMemberDataPtrByName(c_szMainPlayerName, &pMemberData))
		return Py_BuildValue("i", FALSE);

	CPythonGuild::TGuildGradeData * pGradeData;
	if (!CPythonGuild::Instance().GetGradeDataPtr(pMemberData->byGrade, &pGradeData))
		return Py_BuildValue("i", FALSE);

	return Py_BuildValue("i", iAuthority == (pGradeData->wAuthorityFlag & iAuthority));
}

PyObject * guildDestroy(PyObject * poSelf, PyObject * poArgs)
{
	CPythonGuild::Instance().Destroy();
	g_GuildSkillSlotToIndexMap.clear();
	return Py_BuildNone();
}


PyObject * guildGuildIDToMarkID(PyObject * poSelf, PyObject * poArgs)
{
	int guild_id;
	if (!PyTuple_GetInteger(poArgs, 0, &guild_id))
		return Py_BuildException();

	return Py_BuildValue("i", CGuildMarkManager::Instance().GetMarkID(guild_id));
}

PyObject * guildGetMarkImageFilenameByMarkID(PyObject * poSelf, PyObject * poArgs)
{
	int markID;

	if (!PyTuple_GetInteger(poArgs, 0, &markID))
		return Py_BuildException();

	std::string imagePath;
	CGuildMarkManager::Instance().GetMarkImageFilename(markID / CGuildMarkImage::MARK_TOTAL_COUNT, imagePath);
	return Py_BuildValue("s", imagePath.c_str());
}

PyObject * guildGetMarkIndexByMarkID(PyObject * poSelf, PyObject * poArgs)
{
	int markID;

	if (!PyTuple_GetInteger(poArgs, 0, &markID))
		return Py_BuildException();

	return Py_BuildValue("i", markID % CGuildMarkImage::MARK_TOTAL_COUNT);
}

PyObject * guildGetGuildLadderRanking(PyObject * poSelf, PyObject * poArgs)
{
	DWORD point, rank;
	CPythonGuild::Instance().GetRankAndPoint(point, rank);

	return Py_BuildValue("ii", point, rank);
}

PyObject * guildGetWarRecode(PyObject * poSelf, PyObject * poArgs)
{
	BYTE index;
	if (!PyTuple_GetByte(poArgs, 0, &index) || index >= 3)
		return Py_BuildException();

	DWORD win, draw, loss;
	CPythonGuild::Instance().GetWarStats(index, win, draw, loss);

	return Py_BuildValue("iiii", win, loss, draw, win + draw + loss);
}

void initguild()
{
	static PyMethodDef s_methods[] =
	{
		// Enable
		{ "IsGuildEnable",					guildIsGuildEnable,					METH_VARARGS },
		{ "GuildIDToMarkID",				guildGuildIDToMarkID,				METH_VARARGS },
		{ "GetMarkImageFilenameByMarkID",	guildGetMarkImageFilenameByMarkID,	METH_VARARGS },
		{ "GetMarkIndexByMarkID",			guildGetMarkIndexByMarkID,			METH_VARARGS },
		
		// GuildInfo
		{ "GetGuildID",						guildGetGuildID,					METH_VARARGS },
		{ "HasGuildLand",					guildHasGuildLand,					METH_VARARGS },
		{ "GetGuildName",					guildGetGuildName,					METH_VARARGS },
		{ "GetGuildMasterName",				guildGetGuildMasterName,			METH_VARARGS },
		{ "GetEnemyGuildName",				guildGetEnemyGuildName,				METH_VARARGS },
		{ "GetGuildMoney",					guildGetGuildMoney,					METH_VARARGS },

		// BoardPage
		{ "GetGuildBoardCommentCount",		guildGetGuildBoardCommentCount,		METH_VARARGS },
		{ "GetGuildBoardCommentData",		guildGetGuildBoardCommentData,		METH_VARARGS },

		// MemberPage
		{ "GetGuildLevel",					guildGetGuildLevel,					METH_VARARGS },
		{ "GetGuildExperience",				guildGetGuildExperience,			METH_VARARGS },
		{ "GetGuildMemberCount",			guildGetGuildMemberCount,			METH_VARARGS },
		{ "GetGuildMemberLevelSummary",		guildGetGuildMemberLevelSummary,	METH_VARARGS },
		{ "GetGuildMemberLevelAverage",		guildGetGuildMemberLevelAverage,	METH_VARARGS },
		{ "GetGuildExperienceSummary",		guildGetGuildExperienceSummary,		METH_VARARGS },

		// SkillPage
		{ "GetGuildSkillPoint",				guildGetGuildSkillPoint,			METH_VARARGS },
		{ "GetDragonPowerPoint",			guildGetDragonPowerPoint,			METH_VARARGS },
		{ "GetGuildSkillLevel",				guildGetGuildSkillLevel,			METH_VARARGS },
		{ "GetSkillLevel",					guildGetSkillLevel,					METH_VARARGS },
		{ "GetSkillMaxLevelNew",			guildGetSkillMaxLevelNew,			METH_VARARGS },

		{ "SetSkillIndex",					guildSetSkillIndex,					METH_VARARGS },
		{ "GetSkillIndex",					guildGetSkillIndex,					METH_VARARGS },

		{ "GetGuildLadderRanking",			guildGetGuildLadderRanking,			METH_VARARGS },
		{ "GetWarRecode",					guildGetWarRecode,					METH_VARARGS },

		// GradePage
		{ "GetGradeData",					guildGetGradeData,					METH_VARARGS },
		{ "GetGradeName",					guildGetGradeName,					METH_VARARGS },

		// About Member
		{ "GetMemberCount",					guildGetMemberCount,				METH_VARARGS },
		{ "GetMemberData",					guildGetMemberData,					METH_VARARGS },
		{ "MemberIndexToPID",				guildMemberIndexToPID,				METH_VARARGS },
		{ "IsMember",						guildIsMember,						METH_VARARGS },
		{ "IsMemberByName",					guildIsMemberByName,				METH_VARARGS },
		{ "MainPlayerHasAuthority",			guildMainPlayerHasAuthority,		METH_VARARGS },

		// Guild
		{ "Destroy",						guildDestroy,						METH_VARARGS },

		// Ladderboard
		{ "GetRankingPageInfo",				guildGetRankingPageInfo,			METH_VARARGS },
		{ "GetRankingInfo",					guildGetRankingInfo,				METH_VARARGS },

		{ NULL,								NULL,								NULL },
	};

	PyObject * poModule = Py_InitModule("guild", s_methods);
	PyModule_AddIntConstant(poModule, "AUTH_ADD_MEMBER", GUILD_AUTH_ADD_MEMBER);
	PyModule_AddIntConstant(poModule, "AUTH_REMOVE_MEMBER", GUILD_AUTH_REMOVE_MEMBER);
	PyModule_AddIntConstant(poModule, "AUTH_NOTICE", GUILD_AUTH_NOTICE);
	PyModule_AddIntConstant(poModule, "AUTH_SKILL", GUILD_AUTH_SKILL);
#ifdef ENABLE_GUILD_SAFEBOX
	PyModule_AddIntConstant(poModule, "GUILD_AUTH_SAFEBOX_ITEM_GIVE", GUILD_AUTH_SAFEBOX_ITEM_GIVE);
	PyModule_AddIntConstant(poModule, "GUILD_AUTH_SAFEBOX_ITEM_TAKE", GUILD_AUTH_SAFEBOX_ITEM_TAKE);
	PyModule_AddIntConstant(poModule, "GUILD_AUTH_SAFEBOX_GOLD_GIVE", GUILD_AUTH_SAFEBOX_GOLD_GIVE);
	PyModule_AddIntConstant(poModule, "GUILD_AUTH_SAFEBOX_GOLD_TAKE", GUILD_AUTH_SAFEBOX_GOLD_TAKE);
#endif
#ifdef ENABLE_MELEY_LAIR_DUNGEON
	PyModule_AddIntConstant(poModule, "GUILD_AUTH_MELEY_ALLOW", GUILD_AUTH_MELEY_ALLOW);
#endif
	PyModule_AddIntConstant(poModule, "ENEMY_GUILD_SLOT_MAX_COUNT", CPythonGuild::ENEMY_GUILD_SLOT_MAX_COUNT);
	PyModule_AddIntConstant(poModule, "RANKING_INFO_ALL", CPythonGuild::RANKING_INFO_ALL);
	PyModule_AddIntConstant(poModule, "RANKING_INFO_PROMOTE_ALL", CPythonGuild::RANKING_INFO_PROMOTE_ALL);
	PyModule_AddIntConstant(poModule, "RANKING_INFO_APPLICANT_GUILD", CPythonGuild::RANKING_INFO_APPLICANT_GUILD);
	PyModule_AddIntConstant(poModule, "RANKING_INFO_APPLICANT", CPythonGuild::RANKING_INFO_APPLICANT);
	PyModule_AddIntConstant(poModule, "RANKING_INFO_EMPIRE", CPythonGuild::RANKING_INFO_EMPIRE);
	PyModule_AddIntConstant(poModule, "RANKING_INFO_PROMOTE_EMPIRE", CPythonGuild::RANKING_INFO_PROMOTE_EMPIRE); 
	PyModule_AddIntConstant(poModule, "RANKING_INFO_SEARCH", CPythonGuild::RANKING_INFO_SEARCH);
	PyModule_AddIntConstant(poModule, "GUILD_RANKING_RESULT_PER_PAGE", CPythonGuild::GUILD_RANKING_RESULT_PER_PAGE);
}
