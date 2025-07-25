#include "StdAfx.h"

#include <io.h>
#include <assert.h>

#include "EterPackManager.h"
#include "EterPackPolicy_CSHybridCrypt.h"

#include "../eterBase/Debug.h"
#include "../eterBase/CRC32.h"

#define PATH_ABSOLUTE_YMIRWORK1	"d:/ymir work/"
#define PATH_ABSOLUTE_YMIRWORK2	"d:\\ymir work\\"

#ifdef __THEMIDA__
#include <ThemidaSDK.h>
#endif

#ifdef USE_CCC
namespace CCC
{
	enum
	{
		ERROR_OK = 0,
		ERROR_BASE_CODE = 0,
		ERROR_FILE_WAS_NOT_FOUND = ERROR_BASE_CODE + 1,
		ERROR_CORRUPTED_FILE = ERROR_BASE_CODE + 2,
		ERROR_MISSING_KEY = ERROR_BASE_CODE + 3,
		ERROR_MISSING_IV = ERROR_BASE_CODE + 4,
		ERROR_DECRYPTION_HAS_FAILED = ERROR_BASE_CODE + 5,
		ERROR_DECOMPRESSION_FAILED = ERROR_BASE_CODE + 6,
		ERROR_ARCHIVE_NOT_FOUND = ERROR_BASE_CODE + 7,
		ERROR_ARCHIVE_NOT_READABLE = ERROR_BASE_CODE + 8,
		ERROR_ARCHIVE_INVALID = ERROR_BASE_CODE + 9,
		ERROR_ARCHIVE_ACCESS_DENIED = ERROR_BASE_CODE + 10,
		ERROR_KEYSERVER_SOCKET = ERROR_BASE_CODE + 11,
		ERROR_KEYSERVER_CONNECTION = ERROR_BASE_CODE + 12,
		ERROR_KEYSERVER_RESPONSE = ERROR_BASE_CODE + 13,
		ERROR_KEYSERVER_TIMEOUT = ERROR_BASE_CODE + 14,
		ERROR_UNKNOWN = ERROR_BASE_CODE + 15
	};
}

const char* white_file_list[] = { "mark\10_0.tga" "mark\250_0.tga" };

bool isWhiteFile(const char* c_szFileName)
{
	for (int i = 0; i < ARRAYSIZE(white_file_list); i++)
	{
		if (stricmp(c_szFileName, white_file_list[i]) == 0)
		{
			return true;
		}
	}
	return false;
}

const char* white_file_list_ext[] = { "xml", "tga", "png", "bmp", "mp3", "jpg" };

bool isWhiteFileExt(const char* c_szFileName)
{
	for (int i = 0; i < ARRAYSIZE(white_file_list_ext); i++)
	{
		std::string ext = CFileNameHelper::GetExtension(std::string(c_szFileName));
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
		std::string whiteExt = std::string(white_file_list_ext[i]);

		if (ext == whiteExt)
			return true;
	}
	return false;
}

#ifdef AELDRA
HINSTANCE hInstLibrary;

typedef	PCCC(*_CCC_Create2)();
_CCC_Create2 CCC_Create2;
#define CCC_Create CCC_Create2

typedef	void (*_CCC_Destroy2)(PCCC manager);
_CCC_Destroy2 CCC_Destroy2;
#define CCC_Destroy CCC_Destroy2


typedef	int (*_CCC_LoadA2)(PCCC manager, const char* filename);
_CCC_LoadA2 CCC_LoadA2;
#define CCC_LoadA CCC_LoadA2

typedef	int (*_CCC_LoadW2)(PCCC manager, const wchar_t* filename);
_CCC_LoadW2 CCC_LoadW2;
#define CCC_LoadW CCC_LoadW2

typedef	int (*_CCC_UnloadA2)(PCCC manager, const char* filename);
_CCC_UnloadA2 CCC_UnloadA2;
#define CCC_UnloadA CCC_UnloadA2

typedef	int (*_CCC_UnloadW2)(PCCC manager, const wchar_t* filename);
_CCC_UnloadW2 CCC_UnloadW2;
#define CCC_UnloadW CCC_UnloadW2


typedef	unsigned int (*_CCC_SizeA2)(PCCC manager, const char* filename);
_CCC_SizeA2 CCC_SizeA2;
#define CCC_SizeA CCC_SizeA2

typedef	unsigned int (*_CCC_SizeW2)(PCCC manager, const wchar_t* filename);
_CCC_SizeW2 CCC_SizeW2;
#define CCC_SizeW CCC_SizeW2

typedef	int (*_CCC_ExistsA2)(PCCC manager, const char* filename);
_CCC_ExistsA2 CCC_ExistsA2;
#define CCC_ExistsA CCC_ExistsA2

typedef	int (*_CCC_ExistsW2)(PCCC manager, const wchar_t* filename);
_CCC_ExistsW2 CCC_ExistsW2;
#define CCC_ExistsW CCC_ExistsW2

typedef	int (*_CCC_GetA2)(PCCC manager, const char* filename, void* buffer, unsigned int maxsize, unsigned int* outsize);
_CCC_GetA2 CCC_GetA2;
#define CCC_GetA CCC_GetA2

typedef	int (*_CCC_GetW2)(PCCC manager, const wchar_t* filename, void* buffer, unsigned int maxsize, unsigned int* outsize);
_CCC_GetW2 CCC_GetW2;
#define CCC_GetW CCC_GetW2
#endif
#endif

CEterPack* CEterPackManager::FindPack(const char* c_szPathName)
{
	std::string strFileName;

	if (0 == ConvertFileName(c_szPathName, strFileName))
	{
		return &m_RootPack;
	}
	else
	{
		for (TEterPackMap::iterator itor = m_DirPackMap.begin(); itor != m_DirPackMap.end(); ++itor)
		{
			const std::string & c_rstrName = itor->first;
			CEterPack * pEterPack = itor->second;

			if (CompareName(c_rstrName.c_str(), c_rstrName.length(), strFileName.c_str()))
			{
				return pEterPack;
			}
		}
	}

	return NULL;
}

void CEterPackManager::SetCacheMode()
{
	m_isCacheMode=true;
}

void CEterPackManager::SetRelativePathMode()
{
	m_bTryRelativePath = true;
}


// StringPath std::string 버전
int CEterPackManager::ConvertFileName(const char * c_szFileName, std::string & rstrFileName)
{
	rstrFileName = c_szFileName;
	stl_lowers(rstrFileName);

	int iCount = 0;

	for (DWORD i = 0; i < rstrFileName.length(); ++i)
	{
		if (rstrFileName[i] == '/')
			++iCount;
		else if (rstrFileName[i] == '\\')
		{
			rstrFileName[i] = '/';
			++iCount;
		}
	}

	return iCount;
}

bool CEterPackManager::CompareName(const char * c_szDirectoryName, DWORD /*dwLength*/, const char * c_szFileName)
{
	const char * c_pszSrc = c_szDirectoryName;
	const char * c_pszCmp = c_szFileName;

	while (*c_pszSrc)
	{
		if (*(c_pszSrc++) != *(c_pszCmp++))
			return false;

		if (!*c_pszCmp)
			return false;
	}

	return true;
}

void CEterPackManager::LoadStaticCache(const char* c_szFileName)
{
	if (!m_isCacheMode)
		return;

	std::string strFileName;
	if (0 == ConvertFileName(c_szFileName, strFileName))
	{
		return;
	}
	
	DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());

	std::unordered_map<DWORD, SCache>::iterator f = m_kMap_dwNameKey_kCache.find(dwFileNameHash);
	if (m_kMap_dwNameKey_kCache.end() != f)
		return;	
	
	CMappedFile kMapFile;
	const void* c_pvData;
	if (!Get(kMapFile, c_szFileName, &c_pvData))
		return;

	SCache kNewCache;
	kNewCache.m_dwBufSize = kMapFile.Size();
	kNewCache.m_abBufData = new BYTE[kNewCache.m_dwBufSize];
	memcpy(kNewCache.m_abBufData, c_pvData, kNewCache.m_dwBufSize);
	m_kMap_dwNameKey_kCache.insert(std::unordered_map<DWORD, SCache>::value_type(dwFileNameHash, kNewCache));
}

CEterPackManager::SCache* CEterPackManager::__FindCache(DWORD dwFileNameHash)
{	
	std::unordered_map<DWORD, SCache>::iterator f=m_kMap_dwNameKey_kCache.find(dwFileNameHash);
	if (m_kMap_dwNameKey_kCache.end()==f)
		return NULL;

	return &f->second;
}

void	CEterPackManager::__ClearCacheMap()
{
	std::unordered_map<DWORD, SCache>::iterator i;

	for (i = m_kMap_dwNameKey_kCache.begin(); i != m_kMap_dwNameKey_kCache.end(); ++i)
		delete [] i->second.m_abBufData;

	m_kMap_dwNameKey_kCache.clear();
}

struct TimeChecker
{
	TimeChecker(const char* name) : name(name)
	{
		baseTime = timeGetTime();
	}
	~TimeChecker()
	{
		printf("load %s (%d)\n", name, timeGetTime() - baseTime);
	}

	const char* name;
	DWORD baseTime;
};

bool CEterPackManager::Get(CMappedFile & rMappedFile, const char * c_szFileName, LPCVOID * pData)
{
	//TimeChecker timeChecker(c_szFileName);
	//Logf(1, "Load %s\n", c_szFileName);

#ifdef USE_CCC
	if (m_iSearchMode == SEARCH_FILE)
	{
		if (GetFromFile(rMappedFile, c_szFileName, pData))
		{
			return true;
		}

		return GetFromPack(rMappedFile, c_szFileName, pData);
	}
	else
	{
		if (isExistInPack(c_szFileName))
		{
			return GetFromPack(rMappedFile, c_szFileName, pData);
		}
		else if (isExist(c_szFileName))
		{
			return GetFromFile(rMappedFile, c_szFileName, pData);
		}
	}

	return false;
#else
	if (m_iSearchMode == SEARCH_FILE_FIRST)
	{
		if (GetFromFile(rMappedFile, c_szFileName, pData))
		{
			return true;
		}

		return GetFromPack(rMappedFile, c_szFileName, pData);
	}
	
	if (GetFromPack(rMappedFile, c_szFileName, pData))
	{
		return true;
	}

	return GetFromFile(rMappedFile, c_szFileName, pData);
#endif
}

bool CEterPackManager::GetSound(CMappedFile& rMappedFile, const char* c_szFileName, LPCVOID* pData)
{

#ifdef USE_CCC
	if (m_iSearchMode == SEARCH_FILE)
	{
		if (GetFromFile(rMappedFile, c_szFileName, pData))
		{
			return true;
		}

		return GetSoundFromPack(rMappedFile, c_szFileName, pData);
	}
	else
	{
		if (isExistInPack(c_szFileName))
		{
			return GetSoundFromPack(rMappedFile, c_szFileName, pData);
		}
		else if (isExist(c_szFileName))
		{
			return GetFromFile(rMappedFile, c_szFileName, pData);
		}
	}

	return false;
#else
	if (m_iSearchMode == SEARCH_FILE_FIRST)
	{
		if (GetFromFile(rMappedFile, c_szFileName, pData))
		{
			return true;
		}

		return GetFromPack(rMappedFile, c_szFileName, pData);
	}

	if (GetFromPack(rMappedFile, c_szFileName, pData))
	{
		return true;
	}

	return GetFromFile(rMappedFile, c_szFileName, pData);
#endif
}


struct FinderLock
{
	FinderLock(CRITICAL_SECTION& cs) : p_cs(&cs)
	{
		EnterCriticalSection(p_cs);
	}

	~FinderLock()
	{
		LeaveCriticalSection(p_cs);
	}

	CRITICAL_SECTION* p_cs;
};

bool CEterPackManager::GetFromPack(CMappedFile & rMappedFile, const char * c_szFileName, LPCVOID * pData)
{
#ifdef USE_CCC
	assert(c_szFileName);

	FinderLock lock(m_csFinder);

	if (m_pCCC)
	{
		int errorCodeSize = 0;
		if ((errorCodeSize = CCC_ExistsA(m_pCCC, c_szFileName)) == CCC::ERROR_OK)
		{
			unsigned int dwSize = CCC_SizeA(m_pCCC, c_szFileName), dwReadSize = 0;
			static BYTE* pbData;
			static bool isAllocated = false;

			if (isAllocated)
			{
				//memcpy(&pkData, 0, dwSize);
				delete[]pbData;
				isAllocated = false;
				pbData = nullptr;
			}

			pbData = new BYTE[dwSize + 1];

			int errorCode = 0;
			if ((errorCode = CCC_GetA(m_pCCC, c_szFileName, pbData, dwSize, &dwReadSize)) == CCC::ERROR_OK)
			{
				pbData[dwReadSize] = 0;
				*pData = pbData;
				rMappedFile.Link(dwReadSize, pbData);
				isAllocated = true;
				return true;
			}
			else {
				TraceError("FoxFS - Could not get file %s Error Code %d", c_szFileName, errorCode);
			}
			//delete[] pbData;
		}
		else {
			TraceError("FoxFS: File not existing %s Error Code %d", c_szFileName, errorCodeSize);

		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}

	return false;
#else
	FinderLock lock(m_csFinder);

	static std::string strFileName;
	
	if (0 == ConvertFileName(c_szFileName, strFileName))
	{
		return m_RootPack.Get(rMappedFile, strFileName.c_str(), pData);
	}
	else
	{
		DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());
		SCache* pkCache = __FindCache(dwFileNameHash);

		if (pkCache)
		{
			rMappedFile.Link(pkCache->m_dwBufSize, pkCache->m_abBufData);
			return true;
		}

		CEterFileDict::Item* pkFileItem = m_FileDict.GetItem(dwFileNameHash, strFileName.c_str());

		if (pkFileItem)
			if (pkFileItem->pkPack)
			{
				bool r = pkFileItem->pkPack->Get2(rMappedFile, strFileName.c_str(), pkFileItem->pkInfo, pData);	
				//pkFileItem->pkPack->ClearDataMemoryMap();
				return r;
			}
	}
#ifdef _DEBUG
	TraceError("CANNOT_FIND_PACK_FILE [%s]", strFileName.c_str());
#endif
#endif

	return false;
}

bool CEterPackManager::GetSoundFromPack(CMappedFile& rMappedFile, const char* c_szFileName, LPCVOID* pData)
{
#ifdef USE_CCC
	assert(c_szFileName);

	FinderLock lock(m_csFinder);

	if (m_pCCC)
	{
		int errorCodeSize = 0;
		if ((errorCodeSize = CCC_ExistsA(m_pCCC, c_szFileName)) == CCC::ERROR_OK)
		{
			unsigned int dwSize = CCC_SizeA(m_pCCC, c_szFileName), dwReadSize = 0;
			BYTE* pbData = new BYTE[dwSize + 1];

			int errorCode = 0;
			if ((errorCode = CCC_GetA(m_pCCC, c_szFileName, pbData, dwSize, &dwReadSize)) == CCC::ERROR_OK)
			{
				pbData[dwReadSize] = 0;
				*pData = pbData;
				rMappedFile.Link(dwReadSize, pbData);
				return true;
			}
			else {
				TraceError("FoxFS - Could not get file %s Error Code %d", c_szFileName, errorCode);
			}
			delete[] pbData;
		}
		else {
			TraceError("FoxFS: File not existing %s Error Code %d", c_szFileName, errorCodeSize);

		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}

	return false;
#else
	FinderLock lock(m_csFinder);

	static std::string strFileName;

	if (0 == ConvertFileName(c_szFileName, strFileName))
	{
		return m_RootPack.Get(rMappedFile, strFileName.c_str(), pData);
	}
	else
	{
		DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());
		SCache* pkCache = __FindCache(dwFileNameHash);

		if (pkCache)
		{
			rMappedFile.Link(pkCache->m_dwBufSize, pkCache->m_abBufData);
			return true;
		}

		CEterFileDict::Item* pkFileItem = m_FileDict.GetItem(dwFileNameHash, strFileName.c_str());

		if (pkFileItem)
			if (pkFileItem->pkPack)
			{
				bool r = pkFileItem->pkPack->Get2(rMappedFile, strFileName.c_str(), pkFileItem->pkInfo, pData);
				//pkFileItem->pkPack->ClearDataMemoryMap();
				return r;
			}
	}
#ifdef _DEBUG
	TraceError("CANNOT_FIND_PACK_FILE [%s]", strFileName.c_str());
#endif
#endif

	return false;
}

const time_t g_tCachingInterval = 10; // 10초
void CEterPackManager::ArrangeMemoryMappedPack()
{
	//time_t curTime = time(NULL);
	//CEterFileDict::TDict dict = m_FileDict.GetDict();
	//for (CEterFileDict::TDict::iterator it = dict.begin(); it != dict.end(); ++it)
	//{
	//	CEterFileDict::Item &rFileItem = it->second;
	//	CEterPack* pkPack = rFileItem.pkPack;
	//	if (pkPack)
	//	{
	//		if (curTime - pkPack->GetLastAccessTime() > g_tCachingInterval)
	//		{
	//			pkPack->ClearDataMemoryMap();
	//		}
	//	}
	//}
}

bool CEterPackManager::GetFromFile(CMappedFile & rMappedFile, const char * c_szFileName, LPCVOID * pData)
{
#ifndef _DEBUG
	//const char *pcExt = strchr(c_szFileName, '.');
	//if (pcExt && 
	//	_strnicmp(pcExt, ".py", 3) == 0 && // python 스크립트 중
	//	stricmp(c_szFileName, "logininfo.py") != 0 && // 로그인 정보 파일이 아니고
	//	strnicmp(c_szFileName, "locale", 6) != 0
	//	)
	//{
	//	return false;
	//}
#endif

	//if(m_bTryRelativePath) {
	//	if (strnicmp(c_szFileName, PATH_ABSOLUTE_YMIRWORK1, strlen(PATH_ABSOLUTE_YMIRWORK1)) == 0 || strnicmp(c_szFileName, PATH_ABSOLUTE_YMIRWORK2, strlen(PATH_ABSOLUTE_YMIRWORK2)) == 0) {
	//		if(rMappedFile.Create(c_szFileName+strlen(PATH_ABSOLUTE_YMIRWORK1), pData, 0, 0))
	//		{
	//			return true;
	//		}
	//	}
	//}

	return rMappedFile.Create(c_szFileName, pData, 0, 0) ? true : false;
}

bool CEterPackManager::isExistInPack(const char * c_szFileName)
{
#if defined(USE_FOX_FS) || defined(USE_CCC)
	assert(c_szFileName);

	if (m_pCCC)
	{
		int errorCodeSize = 0;
		if ((errorCodeSize = CCC_ExistsA(m_pCCC, c_szFileName)) == CCC::ERROR_OK)
		{
			return true;
		}
		else
		{
			Tracenf("FoxFS :  File not exists %s with error code %d", c_szFileName, errorCodeSize);
		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}

	return false;
#else
	std::string strFileName;

	if (0 == ConvertFileName(c_szFileName, strFileName))
	{
		return m_RootPack.IsExist(strFileName.c_str());
	}
	else
	{
		DWORD dwFileNameHash = GetCRC32(strFileName.c_str(), strFileName.length());
		CEterFileDict::Item* pkFileItem = m_FileDict.GetItem(dwFileNameHash, strFileName.c_str());

		if (pkFileItem)
			if (pkFileItem->pkPack)
				return pkFileItem->pkPack->IsExist(strFileName.c_str());
	}

	// NOTE : 매치 되는 팩이 없다면 false - [levites]
	return false;
#endif
}

bool CEterPackManager::isExist(const char * c_szFileName)
{
#ifdef USE_CCC
	if (m_iSearchMode == SEARCH_PACK)
	{
		if (isWhiteFile(c_szFileName) || isWhiteFileExt(c_szFileName))
		{
			return isExistInPack(c_szFileName) || (_access(c_szFileName, 0) == 0);
		}
		return isExistInPack(c_szFileName);
	}

	if (_access(c_szFileName, 0) == 0)
		return true;

	return isExistInPack(c_szFileName);
#else
	if (m_iSearchMode == SEARCH_PACK_FIRST)
	{
		if (isExistInPack(c_szFileName))
			return true;

		return _access(c_szFileName, 0) == 0 ? true : false;
	}

	//if(m_bTryRelativePath) {
	//	if (strnicmp(c_szFileName, PATH_ABSOLUTE_YMIRWORK1, strlen(PATH_ABSOLUTE_YMIRWORK1)) == 0 || strnicmp(c_szFileName, PATH_ABSOLUTE_YMIRWORK2, strlen(PATH_ABSOLUTE_YMIRWORK2)) == 0) {
	//		if(access(c_szFileName+strlen(PATH_ABSOLUTE_YMIRWORK1), 0) == 0)
	//			return true;
	//	}
	//}

	if (_access(c_szFileName, 0) == 0)
		return true;

	return isExistInPack(c_szFileName);
#endif
}


void CEterPackManager::RegisterRootPack(const char * c_szName)
{
#ifdef USE_CCC
	assert(c_szName);
	if (m_pCCC)
	{
		int errorCode = 0;
		if ((errorCode = CCC_LoadA(m_pCCC, c_szName)) != CCC::ERROR_OK)
		{
			TraceError("%s: Error Code %d", c_szName, errorCode);
		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}
#else
	if (!m_RootPack.Create(m_FileDict, c_szName, ""))
	{
		TraceError("%s: Pack file does not exist", c_szName);
	}
#endif
}

const char * CEterPackManager::GetRootPackFileName()
{
	return m_RootPack.GetDBName();
}

bool CEterPackManager::DecryptPackIV(DWORD dwPanamaKey)
{
	TEterPackMap::iterator itor = m_PackMap.begin();
	while (itor != m_PackMap.end())
	{
		itor->second->DecryptIV(dwPanamaKey);
		itor++;
	}
	return true;
}

bool CEterPackManager::RegisterPackWhenPackMaking(const char * c_szName, const char * c_szDirectory, CEterPack* pPack)
{
	m_PackMap.insert(TEterPackMap::value_type(c_szName, pPack));
	m_PackList.push_front(pPack);

	m_DirPackMap.insert(TEterPackMap::value_type(c_szDirectory, pPack));
	return true;
}


bool CEterPackManager::RegisterPack(const char * c_szName, const char * c_szDirectory, const BYTE* c_pbIV)
{
#ifdef USE_CCC
	assert(c_szName);
	if (m_pCCC)
	{
		int errorCode = 0;
		if ((errorCode = CCC_LoadA(m_pCCC, c_szName)) != CCC::ERROR_OK)
		{
			TraceError("%s: Error Code %d", c_szName, errorCode);
		}
	}
	else
	{
		TraceError("FoxFS: Not initialized!");
	}

	return false;
#else
	CEterPack * pEterPack = NULL;
	{
		TEterPackMap::iterator itor = m_PackMap.find(c_szName);

		if (m_PackMap.end() == itor)
		{
			bool bReadOnly = true;

			pEterPack = new CEterPack;
			if (pEterPack->Create(m_FileDict, c_szName, c_szDirectory, bReadOnly, c_pbIV))
			{
				m_PackMap.insert(TEterPackMap::value_type(c_szName, pEterPack));
			}
			else
			{
#ifdef _DEBUG
				Tracef("The eterpack doesn't exist [%s]\n", c_szName);
#endif
				delete pEterPack;
				pEterPack = NULL;
				return false;
			}
		}
		else
		{
			pEterPack = itor->second;
		}		
	}

	if (c_szDirectory && c_szDirectory[0] != '*')
	{
		TEterPackMap::iterator itor = m_DirPackMap.find(c_szDirectory);
		if (m_DirPackMap.end() == itor)
		{
			m_PackList.push_front(pEterPack);
			m_DirPackMap.insert(TEterPackMap::value_type(c_szDirectory, pEterPack));
		}		
	}	

	return true;
#endif
}

void CEterPackManager::SetSearchMode(bool bPackFirst)
{
	m_iSearchMode = bPackFirst ? SEARCH_PACK_FIRST : SEARCH_FILE_FIRST;
}

int CEterPackManager::GetSearchMode()
{
	return m_iSearchMode;
}

CEterPackManager::CEterPackManager() : m_bTryRelativePath(false), m_iSearchMode(SEARCH_FILE_FIRST), m_isCacheMode(false)
{
	InitializeCriticalSection(&m_csFinder);

#ifdef USE_CCC
#ifdef AELDRA
	hInstLibrary = LoadLibrary("ilo.dll");

	if (hInstLibrary)
	{
		CCC_Create2 = (_CCC_Create2)GetProcAddress(hInstLibrary, "CCC_Create2");
		CCC_Destroy2 = (_CCC_Destroy2)GetProcAddress(hInstLibrary, "CCC_Destroy2");
		CCC_LoadA2 = (_CCC_LoadA2)GetProcAddress(hInstLibrary, "CCC_LoadA2");
		CCC_LoadW2 = (_CCC_LoadW2)GetProcAddress(hInstLibrary, "CCC_LoadW2");
		CCC_UnloadA2 = (_CCC_UnloadA2)GetProcAddress(hInstLibrary, "CCC_UnloadA2");
		CCC_UnloadW2 = (_CCC_UnloadW2)GetProcAddress(hInstLibrary, "CCC_UnloadW2");
		CCC_SizeA2 = (_CCC_SizeA2)GetProcAddress(hInstLibrary, "CCC_SizeA2");
		CCC_SizeW2 = (_CCC_SizeW2)GetProcAddress(hInstLibrary, "CCC_SizeW2");
		CCC_ExistsA2 = (_CCC_ExistsA2)GetProcAddress(hInstLibrary, "CCC_ExistsA2");
		CCC_ExistsW2 = (_CCC_ExistsW2)GetProcAddress(hInstLibrary, "CCC_ExistsW2");
		CCC_GetA2 = (_CCC_GetA2)GetProcAddress(hInstLibrary, "CCC_GetA2");
		CCC_GetW2 = (_CCC_GetW2)GetProcAddress(hInstLibrary, "CCC_GetW2");

		if (!CCC_Create2)
		{
			LogBoxf("ERROR _CCC_Create2 [%p]", GetProcAddress(hInstLibrary, "_CCC_Create2"));
			return;
		}
	}
	else
	{
		LogBoxf("ERROR Cryption Library missing");
		return;
	}

	m_pCCC = CCC_Create2();
#else
	m_pCCC = CCC_Create();
#endif
#endif
}

CEterPackManager::~CEterPackManager()
{
	__ClearCacheMap();

	TEterPackMap::iterator i = m_PackMap.begin();
	TEterPackMap::iterator e = m_PackMap.end();
	while (i != e)
	{
		delete i->second;
		i++;
	}	
	DeleteCriticalSection(&m_csFinder);

#ifdef USE_CCC
	if (m_pCCC)
	{
		CCC_Destroy(m_pCCC);
#ifdef AELDRA
		FreeLibrary(hInstLibrary);
#endif
	}
#endif
}

void CEterPackManager::RetrieveHybridCryptPackKeys(const BYTE *pStream)
{
	////dump file format
	//total packagecnt (4byte)
	//	for	packagecntpackage 
	//		db name hash ( stl.h stringhash )
	//		extension cnt( 4byte)
	//		for	extension cnt
	//			ext hash ( stl.h stringhash )
	//			key-16byte 
	//			iv-16byte
	int iMemOffset = 0;

	int		iPackageCnt;
	DWORD	dwPackageNameHash;

	memcpy( &iPackageCnt, pStream + iMemOffset, sizeof(int) );
	iMemOffset += sizeof(iPackageCnt);

	for( int i = 0; i < iPackageCnt; ++i )
	{
		int iRecvedCryptKeySize = 0;
		memcpy( &iRecvedCryptKeySize, pStream + iMemOffset, sizeof(iRecvedCryptKeySize) );
		iRecvedCryptKeySize -= sizeof(dwPackageNameHash); // 서버에서 받은 key stream에는 filename hash가 포함되어 있으므로, hash 사이즈 만큼 배줌.
		iMemOffset += sizeof(iRecvedCryptKeySize); 

		memcpy( &dwPackageNameHash, pStream + iMemOffset, sizeof(dwPackageNameHash) );
		iMemOffset += sizeof(dwPackageNameHash);

		TEterPackMap::const_iterator cit;
		for( cit = m_PackMap.begin(); cit != m_PackMap.end(); ++cit )
		{
			std::string noPathName = CFileNameHelper::NoPath(string(cit->first));
			if( dwPackageNameHash == stringhash().GetHash(noPathName) )
			{
				EterPackPolicy_CSHybridCrypt* pCryptPolicy = cit->second->GetPackPolicy_HybridCrypt();
				int iHavedCryptKeySize = pCryptPolicy->ReadCryptKeyInfoFromStream( pStream + iMemOffset );
				if (iRecvedCryptKeySize != iHavedCryptKeySize)
				{
					TraceError("CEterPackManager::RetrieveHybridCryptPackKeys	cryptokey length of file(%s) is not matched. received(%d) != haved(%d)", noPathName.c_str(), iRecvedCryptKeySize, iHavedCryptKeySize);
				}
				break;
			}
		}
		iMemOffset += iRecvedCryptKeySize;
	}
}

void CEterPackManager::RetrieveHybridCryptPackSDB( const BYTE* pStream )
{
	//cnt
	//for cnt
	//DWORD				dwPackageIdentifier;
	//DWORD				dwFileIdentifier;
	//std::vector<BYTE>	vecSDBStream;
	int iReadOffset = 0;
	int iSDBInfoCount = 0;

	memcpy( &iSDBInfoCount, pStream+iReadOffset, sizeof(int) );
	iReadOffset += sizeof(int);

	for( int i = 0; i < iSDBInfoCount; ++i )
	{
		DWORD dwPackgeIdentifier;
		memcpy( &dwPackgeIdentifier, pStream+iReadOffset, sizeof(DWORD) );
		iReadOffset += sizeof(DWORD);

		TEterPackMap::const_iterator cit;
		for( cit = m_PackMap.begin(); cit != m_PackMap.end(); ++cit )
		{
			std::string noPathName = CFileNameHelper::NoPath(string(cit->first));
			if( dwPackgeIdentifier == stringhash().GetHash(noPathName) )
			{
				EterPackPolicy_CSHybridCrypt* pCryptPolicy = cit->second->GetPackPolicy_HybridCrypt();
				iReadOffset += pCryptPolicy->ReadSupplementatyDataBlockFromStream( pStream+iReadOffset );
				break;
			}
		}
	}
}


void CEterPackManager::WriteHybridCryptPackInfo(const char* pFileName)
{
	//NOTE : this file format contains a little bit of redundant data.
	//however it`s better for seperating cryptkey & supplementary data block.

	//dump file format

	//SDB data offset(4)

	// about cryptkey
	//total packagecnt (4byte)
	//	for	packagecnt 
	//		db name hash 4byte( stl.h stringhash )
	//		extension cnt( 4byte)
	//		for	extension cnt
	//			ext hash ( stl.h stringhash )
	//			key-16byte 
	//			iv-16byte

	//about SDB data
	//total packagecnt (4byte)
	//	for	packagecnt 
	//		db name hash 4byte( stl.h stringhash ) +child node size(4byte)
	//		sdb file cnt( 4byte )
	//		for	sdb file cnt
	//			filename hash ( stl.h stringhash )
	//			related map name size(4), relate map name
	//			sdb block size( 1byte )
	//			sdb blocks 

	CFileBase keyFile;

	if( !keyFile.Create( pFileName, CFileBase::FILEMODE_WRITE) )
	{
		//TODO : write log
		return;
	}

	int iKeyPackageCount = 0;

	//write later ( SDB Offset & PackageCnt for Key )
	keyFile.SeekCur(2*sizeof(int));

	TEterPackMap::const_iterator cit;
	for( cit = m_PackMap.begin(); cit != m_PackMap.end(); ++cit )
	{
		EterPackPolicy_CSHybridCrypt* pPolicy = cit->second->GetPackPolicy_HybridCrypt();
		if( !pPolicy || !pPolicy->IsContainingCryptKey() )
			continue;

		iKeyPackageCount++;

		std::string noPathName = CFileNameHelper::NoPath(string(cit->first));

		DWORD dwPackNamehash = stringhash().GetHash(noPathName);

		CMakePackLog::GetSingleton().Writef("CEterPackManager::WriteHybridCryptPackInfo PackName : %s, Hash : %x", noPathName.c_str(), dwPackNamehash);
		keyFile.Write( &dwPackNamehash, sizeof(DWORD) );

		pPolicy->WriteCryptKeyToFile( keyFile );
	}

	//Write SDB Data
	int iSDBDataOffset = keyFile.GetPosition();
	int iSDBPackageCnt = 0;

	//Write SDB PackageCnt Later
	keyFile.SeekCur(sizeof(int));
	for( cit = m_PackMap.begin(); cit != m_PackMap.end(); ++cit )
	{
		EterPackPolicy_CSHybridCrypt* pPolicy = cit->second->GetPackPolicy_HybridCrypt();
		if( !pPolicy || !pPolicy->IsContainingSDBFile() )
			continue;

		iSDBPackageCnt++;

		std::string noPathName = CFileNameHelper::NoPath(string(cit->first));

		DWORD dwPackNamehash = stringhash().GetHash(noPathName);
		keyFile.Write( &dwPackNamehash, sizeof(DWORD) );

		int iSDBSizeWriteOffset = keyFile.GetPosition();
		keyFile.SeekCur(sizeof(int));

		pPolicy->WriteSupplementaryDataBlockToFile( keyFile );
		int iSDBSizeAfterWrite = keyFile.GetPosition();

		keyFile.Seek(iSDBSizeWriteOffset);

		int iSDBSize = iSDBSizeAfterWrite-(iSDBSizeWriteOffset+4);
		keyFile.Write( &iSDBSize, sizeof(int) );

		keyFile.Seek(iSDBSizeAfterWrite);
	}

	//write sdb data start offset & package cnt
	keyFile.Seek(0);
	keyFile.Write( &iSDBDataOffset, sizeof(int));
	keyFile.Write( &iKeyPackageCount, sizeof(int));

	keyFile.Seek(iSDBDataOffset);
	keyFile.Write( &iSDBPackageCnt, sizeof(int));

	keyFile.Close();
}
