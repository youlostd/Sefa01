#pragma once

#include "StdAfx.h"

class CPythonAccountManager : public CSingleton<CPythonAccountManager>
{
public:
	typedef struct SAccountInfo {
		BYTE	bIndex;
		char	szLoginName[ID_MAX_NUM + 1];
		char	szPassword[PASS_MAX_NUM + 1];
	} TAccountInfo;

public:
	CPythonAccountManager();
	~CPythonAccountManager();

	void	Initialize(const char* szFileName);
	void	Destroy();

public:
	void				SetLastAccountName(const std::string& c_rstLoginName)	{ m_stLastAccountName = c_rstLoginName; }
	const std::string&	GetLastAccountName() const								{ return m_stLastAccountName; }

	void				SetAccountInfo(BYTE bIndex, const char* szLoginName, const char* szPassword);
	void				RemoveAccountInfo(BYTE bIndex);
	const TAccountInfo*	GetAccountInfo(BYTE bIndex);

private:
	std::string	m_stFileName;

	std::string	m_stLastAccountName;
	std::vector<TAccountInfo> m_vec_AccountInfo;
};
