#pragma once

#include <string>
#include "../UserInterface/Locale_inc.h"

class CProperty
{
	public:
		CProperty(const char * c_pszFileName);
		~CProperty();

		void			Clear();
		bool			ReadFromMemory(const void * c_pvData, int iLen, const char * c_pszFileName);

		const char *	GetFileName();
#ifdef ENABLE_TREE_HIDE_SYSTEM
		const char *	GetInternFileName();
#endif

		bool			GetVector(const char * c_pszKey, CTokenVector & rTokenVector);
		bool			GetString(const char * c_pszKey, const char ** c_ppString);

		void			PutVector(const char * c_pszKey, const CTokenVector & c_rTokenVector);
		void			PutString(const char * c_pszKey, const char * c_pszString);

		bool			Save(const char * c_pszFileName);

		DWORD			GetSize();
		DWORD			GetCRC();
#ifdef USE_CCC
		bool			ReadFromXML(const char* c_pszCRC);
#endif

	protected:
		std::string							m_stFileName;
#ifdef ENABLE_TREE_HIDE_SYSTEM
		std::string							m_stInternFileName;
#endif
		std::string							m_stCRC;
		const char *						mc_pFileName;
		DWORD								m_dwCRC;

		CTokenVectorMap						m_stTokenMap;
};
