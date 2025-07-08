#pragma once

#include "Packet.h"

class CPythonGuild : public CSingleton<CPythonGuild>
{
	public:
		enum
		{
			GUILD_SKILL_MAX_NUM = 12,
			ENEMY_GUILD_SLOT_MAX_COUNT = 6,
			GUILD_RANKING_RESULT_PER_PAGE = 8,
		};

		enum RankingInfo
		{
			RANKING_INFO_ALL,
			RANKING_INFO_EMPIRE,
			RANKING_INFO_PROMOTE_ALL,
			RANKING_INFO_PROMOTE_EMPIRE,
			RANKING_INFO_SEARCH,
			RANKING_INFO_APPLICANT,
			RANKING_INFO_APPLICANT_GUILD,
		};

		typedef struct SGulidInfo
		{
			DWORD dwGuildID;
			char szGuildName[GUILD_NAME_MAX_LEN+1];
			DWORD dwMasterPID;
			DWORD dwGuildLevel;
			DWORD dwCurrentExperience;
			DWORD dwCurrentMemberCount;
			DWORD dwMaxMemberCount;
			DWORD dwGuildMoney;
			BOOL bHasLand;

			DWORD dwGuildRank;
			DWORD dwGuildPoint;
			DWORD dwWin[3];
			DWORD dwDraw[3];
			DWORD dwLoss[3];
		} TGuildInfo;

		typedef struct SGuildLadder
		{
			guildLadder ladder[GUILD_RANKING_RESULT_PER_PAGE];
			DWORD currPage;
			DWORD maxPage;
			BYTE resultCount;
			DWORD searchRank;
		} TGuildLadder;

		typedef struct SGuildGradeData
		{
			SGuildGradeData(){}
			SGuildGradeData(WORD wAuthorityFlag_, const char * c_szName_) : wAuthorityFlag(wAuthorityFlag_), strName(c_szName_) {}
			WORD wAuthorityFlag;
			std::string strName;
		} TGuildGradeData;
		typedef std::map<BYTE, TGuildGradeData> TGradeDataMap;

		typedef struct SGuildMemberData
		{
			DWORD dwPID;

			std::string strName;
			BYTE byGrade;
			BYTE byJob;
			BYTE byLevel;
			BYTE byGeneralFlag;
			DWORD dwOffer;
		} TGuildMemberData;
		typedef std::vector<TGuildMemberData> TGuildMemberDataVector;

		typedef struct SGuildBoardCommentData
		{
			DWORD dwCommentID;
			std::string strName;
			std::string strComment;
		} TGuildBoardCommentData;
		typedef std::vector<TGuildBoardCommentData> TGuildBoardCommentDataVector;

		typedef struct SGuildSkillData
		{
			BYTE bySkillPoint;
			BYTE bySkillLevel[GUILD_SKILL_MAX_NUM];
			WORD wGuildPoint;
			WORD wMaxGuildPoint;
		} TGuildSkillData;

		typedef std::map<DWORD, std::string> TGuildNameMap;

	public:
		CPythonGuild();
		virtual ~CPythonGuild();

		void Destroy();

		void EnableGuild();
		void SetGuildMoney(DWORD dwMoney);
		void SetGuildEXP(BYTE byLevel, DWORD dwEXP);
		void SetGradeData(BYTE byGradeNumber, TGuildGradeData & rGuildGradeData);
		void SetGradeName(BYTE byGradeNumber, const char * c_szName);
		void SetGradeAuthority(BYTE byGradeNumber, WORD wAuthority);
		void ClearComment();
		void RegisterComment(DWORD dwCommentID, const char * c_szName, const char * c_szComment);
		void RegisterMember(TGuildMemberData & rGuildMemberData);
		void ChangeGuildMemberGrade(DWORD dwPID, BYTE byGrade);
		void ChangeGuildMemberGeneralFlag(DWORD dwPID, BYTE byFlag);
		void RemoveMember(DWORD dwPID);
		void RegisterGuildName(DWORD dwID, const char * c_szName);

		void GetRankAndPoint(DWORD &point, DWORD &rank);
		void GetWarStats(BYTE index, DWORD &win, DWORD &draw, DWORD &loss);

		BOOL IsMainPlayer(DWORD dwPID);
		BOOL IsGuildEnable();
		TGuildInfo & GetGuildInfoRef();
		BOOL GetGradeDataPtr(DWORD dwGradeNumber, TGuildGradeData ** ppData);
		const TGuildBoardCommentDataVector & GetGuildBoardCommentVector();
		DWORD GetMemberCount();
		BOOL GetMemberDataPtr(DWORD dwIndex, TGuildMemberData ** ppData);
		BOOL GetMemberDataPtrByPID(DWORD dwPID, TGuildMemberData ** ppData);
		BOOL GetMemberDataPtrByName(const char * c_szName, TGuildMemberData ** ppData);
		DWORD GetGuildMemberLevelSummary();
		DWORD GetGuildMemberLevelAverage();
		DWORD GetGuildExperienceSummary();
		TGuildSkillData & GetGuildSkillDataRef();
		bool GetGuildName(DWORD dwID, std::string * pstrGuildName);
		DWORD GetGuildID();
		BOOL HasGuildLand();

		void StartGuildWar(DWORD dwEnemyGuildID);
		void EndGuildWar(DWORD dwEnemyGuildID);
		DWORD GetEnemyGuildID(DWORD dwIndex);
		BOOL IsDoingGuildWar();

		DWORD GetMaximumExp(BYTE level) const;

		void SetLadder(guildLadder *ladder, BYTE ladderCount, DWORD currPage, DWORD maxPage, DWORD searchRank = 0);
		const TGuildLadder& GetLadderRef() const { return m_guildLadder; }

	protected:
		void __CalculateLevelAverage();
		void __SortMember();
		BOOL __IsGradeData(BYTE byGradeNumber);

		void __Initialize();

	protected:
		TGuildInfo m_GuildInfo;
		TGradeDataMap m_GradeDataMap;
		TGuildMemberDataVector m_GuildMemberDataVector;
		TGuildBoardCommentDataVector m_GuildBoardCommentVector;
		TGuildSkillData m_GuildSkillData;
		TGuildNameMap m_GuildNameMap;
		DWORD m_adwEnemyGuildID[ENEMY_GUILD_SLOT_MAX_COUNT];

		DWORD m_dwMemberLevelSummary;
		DWORD m_dwMemberLevelAverage;
		DWORD m_dwMemberExperienceSummary;

		BOOL m_bGuildEnable;

		TGuildLadder m_guildLadder;
};