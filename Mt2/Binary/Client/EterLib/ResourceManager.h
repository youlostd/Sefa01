#pragma once

#include "../UserInterface/Export.h"
#include "Resource.h"
#include "FileLoaderThread.h"

#include <set>
#include <map>
#include <string>

class CResourceManager : public CSingleton<CResourceManager>
{
	public:
		CResourceManager();
		virtual ~CResourceManager();
		
		void		LoadStaticCache(const char* c_szFileName);

		void		DestroyDeletingList();
		void		Destroy();
		
		void		BeginThreadLoading();
		void		EndThreadLoading();

#ifdef ENABLE_MULTI_DESIGN
		const char*	ConvertFilePath(const char* c_pszString);
		const char*	ConvertFilePath(const char* c_pszString, const char* c_pszBasePath);
#endif

		CResource *	InsertResourcePointer(DWORD dwFileCRC, CResource* pResource);
		CResource *	FindResourcePointer(DWORD dwFileCRC);
		CResource *	GetResourcePointer(const char * c_szFileName);
		CResource *	GetTypeResourcePointer(const char * c_szFileName, int iType=-1);

		// 추가
		bool		isResourcePointerData(DWORD dwFileCRC);

		void		RegisterResourceNewFunctionPointer(const char* c_szFileExt, CResource* (*pResNewFunc)(const char* c_szFileName));
		void		RegisterResourceNewFunctionByTypePointer(int iType, CResource* (*pNewFunc) (const char* c_szFileName));
		
		void		DumpFileListToTextFile(const char* c_szFileName);
#ifdef ENABLE_MULTI_DESIGN
		bool		IsFileExist(const char * c_szFileName, bool bCheckConvertedPath = true);
#else
		bool		IsFileExist(const char * c_szFileName);
#endif

		void		Update();
		void		ReserveDeletingResource(CResource * pResource);

	public:
		void		ProcessBackgroundLoading();
		void		PushBackgroundLoadingSet(std::set<std::string> & LoadingSet);

	protected:
		void		__DestroyDeletingResourceMap();
		void		__DestroyResourceMap();
		void		__DestroyCacheMap();

#ifdef ENABLE_MULTI_DESIGN
		DWORD		__GetFileCRC(const char * c_szFileName, const char ** c_pszLowerFile = NULL, bool bConvertFileName = true);
#else
		DWORD		__GetFileCRC(const char * c_szFileName, const char ** c_pszLowerFile = NULL);
#endif
	
	protected:
		typedef std::map<DWORD,	CResource *>									TResourcePointerMap;
		typedef std::map<std::string, CResource* (*)(const char*)>				TResourceNewFunctionPointerMap;
		typedef std::map<int, CResource* (*)(const char*)>						TResourceNewFunctionByTypePointerMap;
		typedef std::map<CResource *, DWORD>									TResourceDeletingMap;
		typedef std::map<DWORD, std::string>									TResourceRequestMap;
		typedef std::map<long, CResource*>										TResourceRefDecreaseWaitingMap;
#ifdef ENABLE_MULTI_DESIGN
		typedef std::map<DWORD, std::string>									TConvertedFileNameMap;
#endif

	protected:
		TResourcePointerMap						m_pCacheMap;
		TResourcePointerMap						m_pResMap;
		TResourceNewFunctionPointerMap			m_pResNewFuncMap;
		TResourceNewFunctionByTypePointerMap	m_pResNewFuncByTypeMap;
		TResourceDeletingMap					m_ResourceDeletingMap;
		TResourceRequestMap						m_RequestMap;	// 쓰레드로 로딩 요청한 리스트
		TResourceRequestMap						m_WaitingMap;
		TResourceRefDecreaseWaitingMap			m_pResRefDecreaseWaitingMap;
#ifdef ENABLE_MULTI_DESIGN
		TConvertedFileNameMap					m_ConvertedFileNameMap;
#endif

		static CFileLoaderThread				ms_loadingThread;
};

extern int g_iLoadingDelayTime;