#ifndef __INC_LOG_MANAGER_H__
#define __INC_LOG_MANAGER_H__

#include "../../libsql/AsyncSQL.h"

enum GOLDBAR_HOW
{
	PERSONAL_SHOP_BUY	= 1 ,
	PERSONAL_SHOP_SELL	= 2 ,
	SHOP_BUY			= 3 ,
	SHOP_SELL			= 4 ,
	EXCHANGE_TAKE		= 5 ,
	EXCHANGE_GIVE		= 6 ,
	QUEST				= 7 ,
};

class LogManager : public singleton<LogManager>
{
	public:
		enum EItemDestroyTypes {
			ITEM_DESTROY_SELL_NPC = 1,
			ITEM_DESTROY_REMOVE,
			ITEM_DESTROY_GROUND_REMOVE,
		};

	public:
		LogManager();
		virtual ~LogManager();

		bool		IsConnected();

		bool		Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);
		
		void		ItemLog(DWORD dwPID, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum, bool bTempLogin);
		void		ItemLog(DWORD dwPID, DWORD dwItemID, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, DWORD dwVnum);
		void		ItemLog(LPCHARACTER ch, LPITEM item, const char * c_pszText, const char * c_pszHint);
		void		ItemLog(LPCHARACTER ch, int itemID, int itemVnum, const char * c_pszText, const char * c_pszHint);
#ifdef QUERY_POOLING
		void		InsertItemQueryLogs(bool shutdown = false);
#endif

		void		CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dw, const char * c_pszText, const char * c_pszHint, const char * c_pszIP, bool bTempLogin);
		void		CharLog(DWORD dwPID, DWORD x, DWORD y, DWORD dw, const char* c_pszText, const char* c_pszHint, const char* c_pszIP);
		void		CharLog(LPCHARACTER ch, DWORD dw, const char * c_pszText, const char * c_pszHint);

		void		MoneyLog(BYTE type, DWORD vnum, int gold);
		void		HackLog(const char * c_pszHackName, const char * c_pszLogin, const char * c_pszName, const char * c_pszIP);
		void		HackLog(const char * c_pszHackName, LPCHARACTER ch);
		void		CubeLog(DWORD dwPID, DWORD x, DWORD y, DWORD item_vnum, DWORD item_uid, int item_count, bool success);
		void		GMCommandLog(DWORD dwPID, const char * szName, const char * szIP, BYTE byChannel, const char * szCommand);
		void		SpeedHackLog(DWORD pid, DWORD x, DWORD y, int hack_count);
		void		RefineLog(DWORD pid, const char * item_name, DWORD item_id, int item_refine_level, int is_success, const char * how);
		void		ShoutLog(BYTE bChannel, BYTE bEmpire, const char * pszText);
		void		LevelLog(LPCHARACTER pChar, unsigned int level, unsigned int playhour);
		void		BootLog(const char * c_pszHostName, BYTE bChannel);
		void		FishLog(DWORD dwPID, int prob_idx, int fish_id, int fish_level, DWORD dwMiliseconds, DWORD dwVnum = false, DWORD dwValue = 0);
		void		QuestRewardLog(const char * c_pszQuestName, DWORD dwPID, DWORD dwLevel, int iValue1, int iValue2);
		void		DetailLoginLog(bool isLogin, LPCHARACTER ch);
		void		ConnectLog(bool isLogin, LPCHARACTER ch);
		void		BossSpawnLog(DWORD dwVnum);
		void		ChangeNameLog(LPDESC d, DWORD dwPID, const char* c_pszOldName, const char* c_pszNewName);

		void		EmergencyLog(const char* c_pszHint, ...);

		void		TranslationErrorLog(BYTE bType, const char* c_pszLangBase, const char* c_pszLanguage, const char* c_pszLangString, const char* c_pszError);
		void		WhisperLog(DWORD dwSenderPID, const char* c_pszSenderName, DWORD dwReceiverPID, const char* c_pszReceiverName, const char* c_pszText, bool bIsOfflineMessage);

#ifdef __MELEY_LAIR_DUNGEON__
		void	MeleyLog(DWORD dwGuildID, DWORD dwPartecipants, DWORD dwTime);
#endif

#ifdef INCREASE_ITEM_STACK
		void		ItemDestroyLog(BYTE bType, LPITEM pkItem, WORD bCount = 0);
#else
		void		ItemDestroyLog(BYTE bType, LPITEM pkItem, BYTE bCount = 0);
#endif
		void        OkayEventLog(int dwPID, const char * c_pszText, int points);
		void		ForcedRewarpLog(LPCHARACTER ch, const char* c_pszDetailLog);

#ifdef __PYTHON_REPORT_PACKET__
		void		HackDetectionLog(LPCHARACTER ch, const char * c_pszType, const char * c_pszDetail);
#endif
		void		LoginFailLog(const char * c_pszHWID, const char * c_pszError, const char * c_pszAccount, const char * c_pszPassword, const char* ip, const char* c_pszComputername);
		void		PacketErrorLog(BYTE bType, BYTE bHeader, const char* c_pszHostName, BYTE bSubHeader, LPCHARACTER ch, LPDESC d, const char* c_pszLastHeader, int iPhase = 0, const char* c_pszLastChat = "");
		void		SuspectTradeLog(LPCHARACTER seller, LPCHARACTER buyer, long long gold, int bars=0);
#ifdef PACKET_ERROR_DUMP
		void		ClientSyserrLog(LPCHARACTER ch, const char * c_pszText);
#endif
#ifdef __DUNGEON_RANKING__
		void		DungeonLog(LPCHARACTER ch, BYTE bIndex, DWORD dwTime);
#endif

#ifdef AUCTION_SYSTEM
		void		AuctionLog(BYTE type, const std::string& info, DWORD player_id, DWORD owner_id, DWORD val, DWORD val2, const std::string& hint);
#endif

	private:

		void		Query(const char * c_pszFormat, ...);
		SQLMsg*		DirectQuery(const char * c_pszFormat, ...);


		CAsyncSQL*	m_sql;
		bool		m_bIsConnect;
};

#endif
