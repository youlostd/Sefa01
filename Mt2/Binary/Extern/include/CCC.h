#ifndef CCC_H
#define CCC_H

#if defined(_MSC_VER)
	#define CCC_MSVC
#else
	#define CCC_GNUC
#endif

#if defined(CCC_DYNAMIC)
	#if defined(CCC_BUILDING)
		#if defined(CCC_MSVC)
			#define CCC_EXPORT __declspec(dllexport)
		#else
			#define CCC_EXPORT
		#endif
	#else
		#if defined(CCC_MSVC)
			#define CCC_EXPORT __declspec(dllimport)
		#else
			#define CCC_EXPORT
		#endif
	#endif
#else
	#define CCC_EXPORT
#endif

#if defined(CCC_MSVC)
	#define CCC_API __fastcall
#else
	#define CCC_API __attribute__((fastcall))
#endif

typedef void* PCCC;

#if defined(__cplusplus)
extern "C"
{
#endif

	PCCC CCC_EXPORT CCC_API CCC_Create();
	void CCC_EXPORT CCC_API CCC_Destroy(PCCC manager);
	
	int CCC_EXPORT CCC_API CCC_LoadA(PCCC manager, const char* filename);
	int CCC_EXPORT CCC_API CCC_LoadW(PCCC manager, const wchar_t* filename);
	int CCC_EXPORT CCC_API CCC_UnloadA(PCCC manager, const char* filename);
	int CCC_EXPORT CCC_API CCC_UnloadW(PCCC manager, const wchar_t* filename);
	
	unsigned int CCC_EXPORT CCC_API CCC_SizeA(PCCC manager, const char* filename);
	unsigned int CCC_EXPORT CCC_API CCC_SizeW(PCCC manager, const wchar_t* filename);
	int CCC_EXPORT CCC_API CCC_ExistsA(PCCC manager, const char* filename);
	int CCC_EXPORT CCC_API CCC_ExistsW(PCCC manager, const wchar_t* filename);
	int CCC_EXPORT CCC_API CCC_GetA(PCCC manager, const char* filename, void* buffer, unsigned int maxsize, unsigned int* outsize);
	int CCC_EXPORT CCC_API CCC_GetW(PCCC manager, const wchar_t* filename, void* buffer, unsigned int maxsize, unsigned int* outsize);

#if defined(__cplusplus)
}
#endif

#endif