﻿#ifndef __INC_METIN_II_MOB_MANAGER_H__
#define __INC_METIN_II_MOB_MANAGER_H__

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

typedef struct SMobSplashAttackInfo
{
	DWORD	dwTiming; // ½ºÅ³ »ç¿ë ÈÄ ½ÇÁ¦·Î µ¥¹ÌÁö ¸ÔÈú¶§±îÁö ±â´Ù¸®´Â ½Ã°£ (ms)
	DWORD	dwHitDistance; // ½ºÅ³ »ç¿ë½Ã ½ÇÁ¦·Î ½ºÅ³ °è»êÀÌ µÇ´Â °Å¸® (Àü¹æ ¸îcm)

	SMobSplashAttackInfo(DWORD dwTiming, DWORD dwHitDistance)
		: dwTiming(dwTiming)
		, dwHitDistance(dwHitDistance)
		{}
} TMobSplashAttackInfo;

typedef struct SMobSkillInfo
{
	DWORD	dwSkillVnum;
	BYTE	bSkillLevel;
	std::vector<TMobSplashAttackInfo> vecSplashAttack;
} TMobSkillInfo;

class CMob
{
	public:
		CMob();

		~CMob();

		network::TMobTable	m_table;
		TMobSkillInfo	m_mobSkillInfo[MOB_SKILL_MAX_NUM];

		void AddSkillSplash(int iIndex, DWORD dwTiming, DWORD dwHitDistance);
};

class CMobInstance
{
	public:
		CMobInstance();

		PIXEL_POSITION	m_posLastAttacked;	// ¸¶Áö¸· ¸ÂÀº À§Ä¡
		DWORD		m_dwLastAttackedTime;	// ¸¶Áö¸· ¸ÂÀº ½Ã°£
		DWORD		m_dwLastWarpTime;

		bool m_IsBerserk;
		bool m_IsGodSpeed;
		bool m_IsRevive;
};

class CMobGroupGroup
{
	public:
		CMobGroupGroup(DWORD dwVnum)
		{
			m_dwVnum = dwVnum;
		}

		// ADD_MOB_GROUP_GROUP_PROB
		void AddMember(DWORD dwVnum, int prob = 1)
		{   
			if (prob == 0)
				return;

			if (!m_vec_iProbs.empty())
				prob += m_vec_iProbs.back();

			m_vec_iProbs.push_back(prob);
			m_vec_dwMemberVnum.push_back(dwVnum);
		}
		// END_OF_ADD_MOB_GROUP_GROUP_PROB

		DWORD GetMember()
		{
			if (m_vec_dwMemberVnum.empty())
				return 0;

			// ADD_MOB_GROUP_GROUP_PROB
			int n = random_number(1, m_vec_iProbs.back());
			itertype(m_vec_iProbs) it = lower_bound(m_vec_iProbs.begin(), m_vec_iProbs.end(), n);

			return m_vec_dwMemberVnum[std::distance(m_vec_iProbs.begin(), it)];
			// END_OF_ADD_MOB_GROUP_GROUP_PROB
			//return m_vec_dwMemberVnum[number(1, m_vec_dwMemberVnum.size())-1];
		}

		DWORD				   m_dwVnum;
		std::vector<DWORD>	  m_vec_dwMemberVnum;

		// ADD_MOB_GROUP_GROUP_PROB
		std::vector<int>	m_vec_iProbs;
		// END_OF_ADD_MOB_GROUP_GROUP_PROB
};

class CMobGroup
{
	public:
		void Create(DWORD dwVnum, std::string & r_stName)
		{
			m_dwVnum = dwVnum;
			m_stName = r_stName;
		}

		const std::vector<DWORD> & GetMemberVector()
		{
			return m_vec_dwMemberVnum;
		}

		int GetMemberCount()
		{   
			return m_vec_dwMemberVnum.size();
		}

		void AddMember(DWORD dwVnum)
		{   
			m_vec_dwMemberVnum.push_back(dwVnum);
		}

	protected:				  
		DWORD				   m_dwVnum;
		std::string			 m_stName;
		std::vector<DWORD>	  m_vec_dwMemberVnum;
};

class CMobManager : public singleton<CMobManager>
{
	public:
		typedef std::map<DWORD, CMob *>::iterator iterator;

		CMobManager();
		virtual ~CMobManager();

		bool		Initialize(const google::protobuf::RepeatedPtrField<network::TMobTable>& table);
		void		Destroy();

		bool		LoadGroup(const char * c_pszFileName);
		bool		LoadGroupGroup(const char * c_pszFileName);
		CMobGroup *	GetGroup(DWORD dwVnum);
		DWORD		GetGroupFromGroupGroup(DWORD dwVnum);

		const CMob *	Get(DWORD dwVnum);
		const CMob *	Get(const char * c_pszName, bool bIsAbbrev, int iLanguageID = -1);

		const iterator	begin()	{ return m_map_pkMobByVnum.begin();	}
		const iterator	end()	{ return m_map_pkMobByVnum.end();	}

		void RebindMobProto(LPCHARACTER ch);
#ifdef INGAME_WIKI
		std::vector<DWORD>& GetMobWikiInfo(DWORD vnum) { return m_wikiInfoMap[vnum]; }
#endif

		void			IncRegenCount(BYTE bRegenType, DWORD dwVnum, int iCount, int iTime);
		void			DumpRegenCount(const char* c_szFilename);

	private:
		std::map<DWORD, CMob *> m_map_pkMobByVnum;
		std::map<std::string, CMob *> m_map_pkMobByName[LANGUAGE_MAX_NUM];
		std::map<DWORD, CMobGroup *> m_map_pkMobGroup;
		std::map<DWORD, CMobGroupGroup *> m_map_pkMobGroupGroup;
#ifdef INGAME_WIKI
		std::map<DWORD, std::vector<DWORD>> m_wikiInfoMap;
#endif
		std::map<DWORD, double> m_mapRegenCount;
};

#endif
