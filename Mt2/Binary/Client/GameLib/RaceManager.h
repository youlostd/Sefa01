#pragma once

#include "RaceData.h"

class CRaceManager : public CSingleton<CRaceManager>
{
	public:
		typedef std::map<DWORD, CRaceData *> TRaceDataMap;
		typedef TRaceDataMap::iterator TRaceDataIterator;

	public:
		CRaceManager();
		virtual ~CRaceManager();

		void Create();
		void Destroy();
		
		void RegisterRaceName(DWORD dwRaceIndex, const char * c_szName);
		void RegisterRaceSrcName(const char * c_szName, const char * c_szSrcName);
		
		void SetPathName(const char * c_szPathName);
		const char * GetFullPathFileName(const char* c_szFileName);

		// Handling
		void CreateRace(DWORD dwRaceIndex);
		void SelectRace(DWORD dwRaceIndex);
		CRaceData * GetSelectedRaceDataPointer();
		// Handling

		BOOL GetRaceDataPointer(DWORD dwRaceIndex, CRaceData ** ppRaceData);


	protected:
		CRaceData* __LoadRaceData(DWORD dwRaceIndex);
		bool __LoadRaceMotionList(CRaceData& rkRaceData, const char* pathName, const char* motionListFileName);

		void __Initialize();
		void __DestroyRaceDataMap();

	protected:
		TRaceDataMap					m_RaceDataMap;

		std::map<std::string, std::string> m_kMap_stRaceName_stSrcName;
		std::map<DWORD, std::string>	m_kMap_dwRaceKey_stRaceName;

	private:
		std::string						m_strPathName;
		CRaceData *						m_pSelectedRaceData;
		
	public:
		void SetRaceHeight(int iVnum, float fHeight);
		float GetRaceHeight(int iVnum);
		void SetRaceSpecular(int iVnum, float fSpecular);
		float GetRaceSpecular(int iVnum);
		
	protected:
		std::map<int, float>				m_kMap_iRaceKey_fRaceAdditionalHeight;
		std::map<DWORD, float>				m_kMap_dwRaceKey_stRaceSpecular;
};