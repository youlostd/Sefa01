#ifndef __WIN32__
#include <sys/time.h>
#endif

#include <cstdlib>
#include <cstring>
#include <chrono>
#include "AsyncSQL.h"

#ifndef __WIN32__
void * AsyncSQLThread(CAsyncSQL* sql)
#else
unsigned int __stdcall AsyncSQLThread(CAsyncSQL* sql)
#endif
{

	if (!sql->Connect())
		return NULL;

	sql->ChildLoop();
	return NULL;
}


CAsyncSQL::CAsyncSQL()
	: m_stHost(""), m_stUser(""), m_stPassword(""), m_stDB(""), m_stLocale(""),
	m_iMsgCount(0), m_iPort(0), m_bEnd(false), m_pThread(nullptr),
	m_mtxQuery(), m_mtxResult(),
	m_iQueryFinished(0), m_ulThreadID(0), m_bConnected(false), 
	m_iCopiedQuery(0)
{
	memset( &m_hDB, 0, sizeof(m_hDB) );

	m_aiPipe[0] = 0;
	m_aiPipe[1] = 0;
}

CAsyncSQL::~CAsyncSQL()
{
	Quit();
	Destroy();
}

void CAsyncSQL::Destroy()
{
	if (m_hDB.host)
	{
		sys_log(0, "AsyncSQL: closing mysql connection.");
		mysql_close(&m_hDB);
		m_hDB.host = NULL;
	}
	if(m_pThread)
	{
		delete m_pThread;
		m_pThread = NULL;
	}
}


bool CAsyncSQL::QueryLocaleSet()
{
	if (0 == m_stLocale.length())
	{
		sys_err("m_stLocale == 0");
		return true;
	}

	else if (m_stLocale == "ascii")
	{
		sys_err("m_stLocale == ascii");
		return true;
	}

	if (mysql_set_character_set(&m_hDB, m_stLocale.c_str()))
	{
		sys_err("cannot set locale %s by 'mysql_set_character_set', errno %u %s", m_stLocale.c_str(), mysql_errno(&m_hDB) , mysql_error(&m_hDB));
		return false; 
	}

	sys_log(0, "\t--mysql_set_character_set(%s)", m_stLocale.c_str());

	return true;
}

bool CAsyncSQL::Connect()
{
	if (0 == mysql_init(&m_hDB))
	{
		fprintf(stderr, "mysql_init failed on [%s]\n", m_stDB.c_str());
		return false;
	}

	if (!m_stLocale.empty())
	{
		if (mysql_options(&m_hDB, MYSQL_SET_CHARSET_NAME, m_stLocale.c_str()) != 0)
		{
			fprintf(stderr, "mysql_option failed : MYSQL_SET_CHARSET_NAME %s ", mysql_error(&m_hDB));
		}
	}

#ifdef __WIN32__
	if (!mysql_real_connect(&m_hDB, "127.0.0.1", m_stUser.c_str(), m_stPassword.c_str(), m_stDB.c_str(), m_iPort, NULL, CLIENT_MULTI_STATEMENTS))
#else
	if (!mysql_real_connect(&m_hDB, m_stHost.c_str(), m_stUser.c_str(), m_stPassword.c_str(), m_stDB.c_str(), m_iPort, NULL, CLIENT_MULTI_STATEMENTS))
#endif
	{
		fprintf(stderr, "mysql_real_connect: %s\n", mysql_error(&m_hDB));
		return false;
	}

	bool reconnect = true;

	if (0 != mysql_options(&m_hDB, MYSQL_OPT_RECONNECT, &reconnect))
		fprintf(stderr, "mysql_option: %s\n", mysql_error(&m_hDB));

	fprintf(stdout, "AsyncSQL: connected to %s (reconnect %d)\n", m_stHost.c_str(), m_hDB.reconnect);

	// The db cache finds the locale in the LOCALE table of the common db, and then modifies the character set.
	// Therefore, when making the initial connection,
	// it is forced to set the character set to "euckr" even though the character set can not be determined because the locale is not known.
	// (If you uncomment the following, you will not be able to access the database where mysql does not have euckr installed.)
	//while (!QueryLocaleSet());
	m_ulThreadID = mysql_thread_id(&m_hDB);

	m_bConnected = true;
	return true;
}

bool CAsyncSQL::Setup(CAsyncSQL * sql, bool bNoThread)
{
	return Setup(sql->m_stHost.c_str(),
			sql->m_stUser.c_str(), 
			sql->m_stPassword.c_str(), 
			sql->m_stDB.c_str(), 
			sql->m_stLocale.c_str(), 
			bNoThread,
			sql->m_iPort);
}

bool CAsyncSQL::Setup(const char * c_pszHost, const char * c_pszUser,
					  const char * c_pszPassword, const char * c_pszDB,
					  const char * c_pszLocale, bool bNoThread, int iPort)
{
	m_stHost = c_pszHost;
	m_stUser = c_pszUser;
	m_stPassword = c_pszPassword;
	m_stDB = c_pszDB;
	m_iPort = iPort;

	if (c_pszLocale)
	{
		m_stLocale = c_pszLocale;
		sys_log(0, "AsyncSQL: locale %s", m_stLocale.c_str());
	}

	if (!bNoThread)
	{

		if (!mysql_thread_safe())
		{
			fprintf(stderr, "FATAL ERROR!! mysql client library was not compiled with thread safety\n");
			return false;
		}

		m_pThread = new std::thread(AsyncSQLThread, this);
		return true;
	}
	else
		return Connect();
}

void CAsyncSQL::Quit()
{
	if (!m_bEnd)
	{
		m_bEnd = true;
		m_sem.Release();

		if(m_pThread && m_pThread->joinable())
			m_pThread->join();
	}
}

SQLMsg * CAsyncSQL::DirectQuery(const char * c_pszQuery)
{
	if (m_ulThreadID != mysql_thread_id(&m_hDB))
	{
		sys_err("MySQL connection was reconnected. querying locale set");
		while (!QueryLocaleSet());
		m_ulThreadID = mysql_thread_id(&m_hDB);
	}

	SQLMsg * p = new SQLMsg;

	p->m_pkSQL = &m_hDB;
	p->iID = ++m_iMsgCount;
	p->stQuery = c_pszQuery;

	if (mysql_real_query(&m_hDB, p->stQuery.c_str(), p->stQuery.length()))
	{
		char buf[1024];

		snprintf(buf, sizeof(buf),
				"AsyncSQL::DirectQuery : mysql_query error: %s\nquery: %s",
				mysql_error(&m_hDB), p->stQuery.c_str());

		sys_err(buf);
		p->uiSQLErrno = mysql_errno(&m_hDB);
	}

	p->Store();
	return p;
}

void CAsyncSQL::AsyncQuery(const char * c_pszQuery)
{
	SQLMsg * p = new SQLMsg;

	p->m_pkSQL = &m_hDB;
	p->iID = ++m_iMsgCount;
	p->stQuery = c_pszQuery;

	PushQuery(p);
}

void CAsyncSQL::ReturnQuery(const char * c_pszQuery, void*  pvUserData)
{
	SQLMsg * p = new SQLMsg;

	p->m_pkSQL = &m_hDB;
	p->iID = ++m_iMsgCount;
	p->stQuery = c_pszQuery;
	p->bReturn = true;
	p->pvUserData = pvUserData;

	PushQuery(p);
}

void CAsyncSQL::PushResult(SQLMsg * p)
{
	std::lock_guard<std::mutex> g(m_mtxResult);
	m_queue_result.push(p);
}

bool CAsyncSQL::PopResult(SQLMsg ** pp)
{
	std::lock_guard<std::mutex> g(m_mtxResult);
	if (m_queue_result.empty())
	{
		return false;
	}

	*pp = m_queue_result.front();
	m_queue_result.pop();
	return true;
}

void CAsyncSQL::PushQuery(SQLMsg * p)
{
	std::lock_guard<std::mutex> g(m_mtxQuery);
	m_queue_query.push(p);
	m_sem.Release();
}

bool CAsyncSQL::PeekQuery(SQLMsg ** pp)
{
	std::lock_guard<std::mutex> g(m_mtxQuery);

	if (m_queue_query.empty())
	{
		return false;
	}

	*pp = m_queue_query.front();
	return true;
}

void CAsyncSQL::PopQuery(int iID)
{
	std::lock_guard<std::mutex> g(m_mtxQuery);
	m_queue_query.pop();
}

bool CAsyncSQL::PeekQueryFromCopyQueue(SQLMsg ** pp)
{
	if (m_queue_query_copy.empty())
		return false;

	*pp = m_queue_query_copy.front();
	return true;
}

int CAsyncSQL::CopyQuery()
{
	std::lock_guard<std::mutex> g(m_mtxQuery);
	if (m_queue_query.empty())
		return -1;

	while (!m_queue_query.empty())
	{
		m_queue_query_copy.push(m_queue_query.front());
		m_queue_query.pop();
	}
	return m_queue_query_copy.size();
}

void CAsyncSQL::PopQueryFromCopyQueue()
{
	m_queue_query_copy.pop();
}
int	CAsyncSQL::GetCopiedQueryCount()
{
	return m_iCopiedQuery;
}
void CAsyncSQL::ResetCopiedQueryCount()
{
	m_iCopiedQuery = 0;
}

void CAsyncSQL::AddCopiedQueryCount(int iCopiedQuery)
{
	m_iCopiedQuery += iCopiedQuery;
}

DWORD CAsyncSQL::CountQuery()
{
	return m_queue_query.size();
}

DWORD CAsyncSQL::CountResult()
{
	return m_queue_result.size();
}

class cProfiler
{
private:
	std::chrono::time_point<std::chrono::steady_clock> m_start;
	std::chrono::duration<float>& m_duration;
public:
	cProfiler(std::chrono::duration<float>& duration):
		m_duration(duration)
	{
		m_start = std::chrono::steady_clock::now();
	}

	~cProfiler()
	{
		m_duration = (std::chrono::steady_clock::now() - m_start);
	}
};

void CAsyncSQL::ChildLoop()
{
	while (!m_bEnd)
	{
		m_sem.Wait();

		int count = CopyQuery();

		if (count <= 0)
			continue;

		AddCopiedQueryCount(count);

		SQLMsg * p;
		std::chrono::duration<float> timer;
		while (count--)
		{
			{// Start time check
				
				cProfiler profiler(timer);

				if (!PeekQueryFromCopyQueue(&p))
					continue;

				if (m_ulThreadID != mysql_thread_id(&m_hDB))
				{
					sys_err("MySQL connection was reconnected. querying locale set");
					while (!QueryLocaleSet());
					m_ulThreadID = mysql_thread_id(&m_hDB);
				}

				if (mysql_real_query(&m_hDB, p->stQuery.c_str(), p->stQuery.length()))
				{
					p->uiSQLErrno = mysql_errno(&m_hDB);

					sys_err("AsyncSQL: query failed: %s (query: %s errno: %d)", 
							mysql_error(&m_hDB), p->stQuery.c_str(), p->uiSQLErrno);

					switch (p->uiSQLErrno)
					{
						case CR_SOCKET_CREATE_ERROR:
						case CR_CONNECTION_ERROR:
						case CR_IPSOCK_ERROR:
						case CR_UNKNOWN_HOST:
						case CR_SERVER_GONE_ERROR:
						case CR_CONN_HOST_ERROR:
						case ER_NOT_KEYFILE:
						case ER_CRASHED_ON_USAGE:
						case ER_CANT_OPEN_FILE:
						case ER_HOST_NOT_PRIVILEGED:
						case ER_HOST_IS_BLOCKED:
						case ER_PASSWORD_NOT_ALLOWED:
						case ER_PASSWORD_NO_MATCH:
						case ER_CANT_CREATE_THREAD:
						case ER_INVALID_USE_OF_NULL:
							m_sem.Release();
							sys_err("AsyncSQL: retrying");
							continue;
					}
				}
			}
			// Output to Log if it takes more than 0.5 seconds
			if (timer.count() > 0.5)
				sys_log(0, "[QUERY : LONG INTERVAL(OverSec %f)] : %s", static_cast<float>(timer.count()), p->stQuery.c_str());

			PopQueryFromCopyQueue();

			if (p->bReturn)
			{
				p->Store();
				PushResult(p);
			}
			else
				delete p;

			++m_iQueryFinished;
		}
	}

	SQLMsg * p;

	while (PeekQuery(&p))
	{
		if (m_ulThreadID != mysql_thread_id(&m_hDB))
		{
			sys_err("MySQL connection was reconnected. querying locale set");
			while (!QueryLocaleSet());
			m_ulThreadID = mysql_thread_id(&m_hDB);
		}

		if (mysql_real_query(&m_hDB, p->stQuery.c_str(), p->stQuery.length()))
		{
			p->uiSQLErrno = mysql_errno(&m_hDB);

			sys_err("AsyncSQL::ChildLoop : mysql_query error: %s:\nquery: %s",
					mysql_error(&m_hDB), p->stQuery.c_str());

			switch (p->uiSQLErrno)
			{
				case CR_SOCKET_CREATE_ERROR:
				case CR_CONNECTION_ERROR:
				case CR_IPSOCK_ERROR:
				case CR_UNKNOWN_HOST:
				case CR_SERVER_GONE_ERROR:
				case CR_CONN_HOST_ERROR:
				case ER_NOT_KEYFILE:
				case ER_CRASHED_ON_USAGE:
				case ER_CANT_OPEN_FILE:
				case ER_HOST_NOT_PRIVILEGED:
				case ER_HOST_IS_BLOCKED:
				case ER_PASSWORD_NOT_ALLOWED:
				case ER_PASSWORD_NO_MATCH:
				case ER_CANT_CREATE_THREAD:
				case ER_INVALID_USE_OF_NULL:
					continue;
			}
		}

		sys_log(0, "QUERY_FLUSH: %s", p->stQuery.c_str());

		PopQuery(p->iID);

		if (p->bReturn)
		{
			p->Store();
			PushResult(p);
		}
		else
			delete p;

		++m_iQueryFinished;
	}
}

int CAsyncSQL::CountQueryFinished()
{
	return m_iQueryFinished;
}

void CAsyncSQL::ResetQueryFinished()
{
	m_iQueryFinished = 0;
}

MYSQL * CAsyncSQL::GetSQLHandle()
{
	return &m_hDB;
}

size_t CAsyncSQL::EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize)
{
	if (0 == srcSize)
	{
		memset(dst, 0, dstSize);
		return 0;
	}

	if (0 == dstSize)
		return 0;

	if (dstSize < srcSize * 2 + 1)
	{
		// Copy only 256 bytes to log when \0 is not attached
		char tmp[256];
		size_t tmpLen = sizeof(tmp) > srcSize ? srcSize : sizeof(tmp);
		strlcpy(tmp, src, tmpLen);

		sys_err("FATAL ERROR!! not enough buffer size (dstSize %u srcSize %u src%s: %s)",
				dstSize, srcSize, tmpLen != srcSize ? "(trimmed to 255 characters)" : "", tmp);

		dst[0] = '\0';
		return 0;
	}

	return mysql_real_escape_string(GetSQLHandle(), dst, src, srcSize);
}

void CAsyncSQL2::SetLocale(const std::string & stLocale)
{
	m_stLocale = stLocale;
	QueryLocaleSet();
}