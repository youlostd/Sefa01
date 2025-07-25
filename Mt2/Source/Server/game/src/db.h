﻿#ifndef __INC_METIN_II_DB_MANAGER_H__
#define __INC_METIN_II_DB_MANAGER_H__

#include "../../libsql/AsyncSQL.h"

enum
{
	QUERY_TYPE_RETURN = 1,
	QUERY_TYPE_FUNCTION = 2,
	QUERY_TYPE_AFTER_FUNCTION = 3,
};

enum
{
	QID_SAFEBOX_SIZE,
	QID_AUTH_LOGIN,
#ifdef __DEPRECATED_BILLING__
	QID_BILLING_CHECK,
#endif

	// BLOCK_CHAT
	QID_BLOCK_CHAT_LIST,
	// END_OF_BLOCK_CHAT

	// PROTECT_CHILD_FOR_NEWCIBN
	QID_PROTECT_CHILD,
	// END_PROTECT_CHILD_FOR_NEWCIBN

#ifdef __HOMEPAGE_COMMAND__
	QID_PROCESS_HOMEPAGE_COMMANDS,
#endif

	QID_PULL_OFFLINE_MESSAGES,

#ifdef __VOTE4BUFF__
	QID_V4B_LOAD,
#endif

	QID_UPPITEM_LIST_RELOAD,

	QID_MESSENGER_LIST_LOAD,
	QID_GUILD_LOAD_DATA,
	QID_GUILD_LOAD_GRADE,
	QID_GUILD_LOAD_MEMBER,
	QID_GUILD_CHANGE_GRADE,
	QID_GUILD_CHANGE_LEADER,
	QID_GUILD_LOAD_MEMBER_LASTPLAY,
};

typedef struct SUseTime
{
	DWORD	dwLoginKey;
	char		szLogin[LOGIN_MAX_LEN+1];
#ifdef __DEPRECATED_BILLING__
	BYTE		bBillType;
#endif
	DWORD	   dwUseSec;
	char		szIP[MAX_HOST_LENGTH+1];
} TUseTime;

class CQueryInfo
{
	public:
		int	iQueryType;
};

class CReturnQueryInfo : public CQueryInfo
{
	public:
		int	iType;
		DWORD	dwIdent;
		void			*	pvData;
};

class CFuncQueryInfo : public CQueryInfo
{
	public:
		std::function<void(SQLMsg*)> f;
};

class CFuncAfterQueryInfo : public CQueryInfo
{
	public:
		std::function<void()> f;
};

class CLoginData;


class DBManager : public singleton<DBManager>
{
	public:
		DBManager();
		virtual ~DBManager();

		bool			IsConnected();

		bool			Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);
		void			Query(const char * c_pszFormat, ...);

		SQLMsg *		DirectQuery(const char * c_pszFormat, ...);
		void			ReturnQuery(int iType, DWORD dwIdent, void* pvData, const char * c_pszFormat, ...);

		void			Process();
		void			AnalyzeReturnQuery(SQLMsg * pmsg);

#ifdef __DEPRECATED_BILLING__
		void			LoginPrepare(BYTE bBillType, DWORD dwBillID, long lRemainSecs, LPDESC d, DWORD * pdwClientKey, int * paiPremiumTimes = NULL);
#else
		void			LoginPrepare(LPDESC d, DWORD* pdwClientKey, int* paiPremiumTimes = NULL);
#endif
		void			SendAuthLogin(LPDESC d);
		void			SendLoginPing(const char * c_pszLogin);

		void			InsertLoginData(CLoginData * pkLD);
		void			DeleteLoginData(CLoginData * pkLD);
		CLoginData *		GetLoginData(DWORD dwKey);
#ifdef __DEPRECATED_BILLING__
		void			SetBilling(DWORD dwKey, bool bOn, bool bSkipPush = false);
		void			PushBilling(CLoginData * pkLD);
		void			FlushBilling(bool bForce=false);
		void			CheckBilling();

		void			StopAllBilling(); // 20050503.ipkn.DB-AUTH Á¢¼Ó Á¾·á½Ã ºô¸µ Å×ÀÌºí ¸ðµÎ Áö¿ì±â (Àç¿¬°á½Ã º¹±¸ÇÔ)
#endif

		DWORD			CountQuery()		{ return m_sql.CountQuery(); }
		DWORD			CountQueryResult()	{ return m_sql.CountResult(); }
		void			ResetQueryResult()	{ m_sql.ResetQueryFinished(); }

		template<class Functor> void FuncQuery(Functor f, const char * c_pszFormat, ...); // °á°ú¸¦ fÀÎÀÚ·Î È£ÃâÇÔ (SQLMsg *) ¾Ë¾Æ¼­ ÇØÁ¦µÊ
		template<class Functor> void FuncAfterQuery(Functor f, const char * c_pszFormat, ...); // ³¡³ª°í ³ª¸é f°¡ È£ÃâµÊ void			f(void) ÇüÅÂ

		size_t EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize);

	private:
		SQLMsg *				PopResult();

		CAsyncSQL				m_sql;
		CAsyncSQL				m_sql_direct;
		bool					m_bIsConnect;

		std::map<DWORD, CLoginData *>		m_map_pkLoginData;
#ifdef __DEPRECATED_BILLING__
		std::map<std::string, CLoginData *>	mapLDBilling;
#endif
		std::vector<TUseTime>			m_vec_kUseTime;
};

template <class Functor> void DBManager::FuncQuery(Functor f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	CFuncQueryInfo * p = M2_NEW CFuncQueryInfo;

	p->iQueryType = QUERY_TYPE_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

template <class Functor> void DBManager::FuncAfterQuery(Functor f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	CFuncAfterQueryInfo * p = M2_NEW CFuncAfterQueryInfo;

	p->iQueryType = QUERY_TYPE_AFTER_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

////////////////////////////////////////////////////////////////

#ifdef __DEPRECATED_BILLING__
extern void SendBillingExpire(const char * c_pszLogin, BYTE bBillType, int iSecs, CLoginData * pkLD, DWORD num);
#endif

// ACCOUNT_DB
class AccountDB : public singleton<AccountDB>
{
	public:
		AccountDB();

		bool IsConnected();
		bool Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);
		bool ConnectAsync(const char * host, const int port, const char * user, const char * pwd, const char * db, const char * locale);

		SQLMsg* DirectQuery(const char * query);		
		void ReturnQuery(int iType, DWORD dwIdent, void * pvData, const char * c_pszFormat, ...);
		void AsyncQuery(const char* query);

		void SetLocale(const std::string & stLocale);

		void Process();

	private:
		SQLMsg * PopResult();
		void AnalyzeReturnQuery(SQLMsg * pMsg);

		CAsyncSQL2	m_sql_direct;
		CAsyncSQL2	m_sql;
		bool		m_IsConnect;

};
//END_ACCOUNT_DB

#endif
