#pragma once

#include "../libthecore/include/stdafx.h"
#include "../libthecore/include/log.h"

#include <string>
#include <queue>
#include <vector>
#include <map>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#include <mysql/mysqld_error.h>

#include <mutex>
#include <thread>
#include "Semaphore.h"
#define QUERY_MAX_LEN 8192

class CQueryInfo;

typedef struct _SQLResult
{
	_SQLResult()
	   	: pSQLResult(NULL), uiNumRows(0), uiAffectedRows(0), uiInsertID(0)
	{
	}

	~_SQLResult()
	{
		if (pSQLResult)
		{
			mysql_free_result(pSQLResult);
			pSQLResult = NULL;
		}
	}

	MYSQL_RES *	pSQLResult;
	uint32_t		uiNumRows;
	uint32_t		uiAffectedRows;
	uint32_t		uiInsertID;
} SQLResult;

struct SQLMsg
{
	SQLMsg() : m_pkSQL(NULL), iID(0), uiResultPos(0), bReturn(false), uiSQLErrno(0)
	{
	}

	~SQLMsg()
	{
		for(auto& it : vec_pkResult)
			delete it;

		vec_pkResult.clear();
	}

	void Store()
	{
		do
		{
			SQLResult * pRes = new SQLResult;

			pRes->pSQLResult = mysql_store_result(m_pkSQL);
			pRes->uiInsertID = mysql_insert_id(m_pkSQL);
			pRes->uiAffectedRows = mysql_affected_rows(m_pkSQL);

			if (pRes->pSQLResult)
			{
				pRes->uiNumRows = mysql_num_rows(pRes->pSQLResult);
			}
			else
			{
				pRes->uiNumRows = 0;
			}

			vec_pkResult.push_back(pRes);
		} while (!mysql_next_result(m_pkSQL));
	}

	SQLResult * Get()
	{
		if (uiResultPos >= vec_pkResult.size())
			return NULL;

		return vec_pkResult[uiResultPos];
	}

	bool Next()
	{
		if (uiResultPos + 1 >= vec_pkResult.size())
			return false;

		++uiResultPos;
		return true;
	}

	
	MYSQL *			m_pkSQL;
	int				iID;
	std::string			stQuery;

	std::vector<SQLResult *>	vec_pkResult;	// result ????
	unsigned int		uiResultPos;	// ???? result ???

	void *			pvUserData;
	bool			bReturn;

	unsigned int		uiSQLErrno;
};

class CAsyncSQL
{
	public:
		CAsyncSQL();
		virtual ~CAsyncSQL();

		void		Quit();

		bool   		Setup(const char * c_pszHost, const char * c_pszUser, const char * c_pszPassword, const char * c_pszDB, const char * c_pszLocale, 
			bool bNoThread = false, int iPort = 0);
		bool		Setup(CAsyncSQL * sql, bool bNoThread = false);

		bool		Connect();
		bool		IsConnected() { return m_bConnected; }
		bool		QueryLocaleSet();

		void		AsyncQuery(const char * c_pszQuery);
		void		ReturnQuery(const char * c_pszQuery, void*  pvUserData);
		SQLMsg *	DirectQuery(const char * c_pszQuery);

		DWORD		CountQuery();
		DWORD		CountResult();

		void		PushResult(SQLMsg * p);
		bool		PopResult(SQLMsg ** pp);

		void		ChildLoop();

		MYSQL *		GetSQLHandle();

		int			CountQueryFinished();
		void		ResetQueryFinished();

		size_t		EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize);

	protected:
		void		Destroy();

		void		PushQuery(SQLMsg * p);

		bool		PeekQuery(SQLMsg ** pp);
		void		PopQuery(int iID);

		bool		PeekQueryFromCopyQueue(SQLMsg ** pp );
		INT			CopyQuery();
		void		PopQueryFromCopyQueue();

	public:
		int			GetCopiedQueryCount();
		void		ResetCopiedQueryCount();
		void		AddCopiedQueryCount( int iCopiedQuery );

	protected:
		MYSQL m_hDB;

		std::string	m_stHost;
		std::string	m_stUser;
		std::string	m_stPassword;
		std::string	m_stDB;
		std::string	m_stLocale;

		int	m_iMsgCount;
		int	m_aiPipe[2];
		int m_iPort;

		std::queue<SQLMsg *> m_queue_query;
		std::queue<SQLMsg *> m_queue_query_copy;

		std::queue<SQLMsg *> m_queue_result;

		volatile bool m_bEnd;

		std::thread* m_pThread;
		std::mutex m_mtxQuery;
		std::mutex m_mtxResult;

		CSemaphore m_sem;

		int	m_iQueryFinished;

		unsigned long m_ulThreadID;
		bool m_bConnected;
		int	m_iCopiedQuery;
};

class CAsyncSQL2 : public CAsyncSQL
{
	public:
		void SetLocale ( const std::string & stLocale );
};