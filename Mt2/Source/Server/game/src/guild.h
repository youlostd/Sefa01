#ifndef __INC_GUILD_H
#define __INC_GUILD_H

#include "char.h"
#include "skill.h"
#ifdef __GUILD_SAFEBOX__
#include "guild_safebox.h"
#endif

#include <google/protobuf/repeated_field.h>

struct SQLMsg;

enum
{
	GUILD_GRADE_NAME_MAX_LEN = 16,
	GUILD_GRADE_COUNT = 15,
	GUILD_COMMENT_MAX_COUNT = 12,
	GUILD_COMMENT_MAX_LEN = 50,
	GUILD_LEADER_GRADE = 1,
	GUILD_BASE_POWER = 400,
	GUILD_POWER_PER_SKILL_LEVEL = 200,
	GUILD_POWER_PER_LEVEL = 100,
	GUILD_MINIMUM_LEADERSHIP = 40,
	GUILD_WAR_MIN_MEMBER_COUNT = 8,
	GUILD_LADDER_POINT_PER_LEVEL = 1000,
	GUILD_CREATE_ITEM_VNUM = 70101,
	GUILD_RANKING_RESULT_PER_PAGE = 8,
	GUILD_LADDER_UPDATE_INTERVAL = 60, // rebuild ladder rank in every minute if requested
	GUILD_COMMENT_COOLDOWN = 10 * 60,
	GUILD_COMMENT_LIMIT = 12,
};

struct SGuildMaster
{
	DWORD pid;
};


typedef struct SGuildMember
{
	DWORD pid; // player Å×ÀÌºíÀÇ id; primary key
	BYTE grade; // ±æµå»óÀÇ ÇÃ·¹ÀÌ¾îÀÇ °è±Þ 1 to 15 (1ÀÌ Â¯)
	BYTE is_general;
	BYTE job;
	BYTE level;
	DWORD offer_exp; // °øÇåÇÑ °æÇèÄ¡

	std::string name;

	SGuildMember(LPCHARACTER ch, BYTE grade, DWORD offer_exp);
	SGuildMember(DWORD pid, BYTE grade, BYTE is_general, BYTE job, BYTE level, DWORD offer_exp, const char* name);

} TGuildMember;

#pragma pack(1)
typedef struct SGuildMemberPacketData
{   
	DWORD pid;
	BYTE grade;
	BYTE is_general;
	BYTE job;
	BYTE level;
	DWORD offer;
	BYTE name_flag;
	char name[CHARACTER_NAME_MAX_LEN+1];
} TGuildMemberPacketData;

typedef struct packet_guild_sub_info
{
	WORD member_count;
	WORD max_member_count;
	DWORD guild_id;
	DWORD master_pid;
	DWORD exp;
	BYTE level;
	char name[GUILD_NAME_MAX_LEN+1];
	DWORD gold;
	BYTE has_land;

	DWORD dwGuildRank;
	DWORD dwGuildPoint;
	DWORD dwWin[GUILD_WAR_TYPE_MAX_NUM];
	DWORD dwDraw[GUILD_WAR_TYPE_MAX_NUM];
	DWORD dwLoss[GUILD_WAR_TYPE_MAX_NUM];
} TPacketGCGuildInfo;

typedef struct SGuildGrade
{
	char grade_name[GUILD_GRADE_NAME_MAX_LEN+1]; // 8+1 ±æµåÀå, ±æµå¿ø µîÀÇ ÀÌ¸§
	WORD auth_flag;
} TGuildGrade;

struct TOneGradeNamePacket
{
	BYTE grade;
	char grade_name[GUILD_GRADE_NAME_MAX_LEN+1];
};

struct TOneGradeAuthPacket
{
	BYTE grade;
	WORD auth;
};
#pragma pack()

enum
{
	GUILD_AUTH_ADD_MEMBER	= (1 << 0),
	GUILD_AUTH_REMOVE_MEMBER	= (1 << 1),
	GUILD_AUTH_NOTICE		= (1 << 2),
	GUILD_AUTH_USE_SKILL	= (1 << 3),
#ifdef __GUILD_SAFEBOX__
	GUILD_AUTH_SAFEBOX_ITEM_GIVE	= (1 << 4),
	GUILD_AUTH_SAFEBOX_ITEM_TAKE	= (1 << 5),
	GUILD_AUTH_SAFEBOX_GOLD_GIVE	= (1 << 6),
	GUILD_AUTH_SAFEBOX_GOLD_TAKE	= (1 << 7),
#endif
	GUILD_AUTH_MELEY_ALLOW	= (1 << 8),
};

typedef struct SGuildData
{
	DWORD	guild_id;
	DWORD	master_pid;
	DWORD	exp;
	BYTE	level;
	char	name[GUILD_NAME_MAX_LEN+1];

	TGuildGrade	grade_array[GUILD_GRADE_COUNT];

	BYTE	skill_point;
	BYTE	abySkill[GUILD_SKILL_COUNT];

	int		power;
	int		max_power;

	int		ladder_point;

	int		win[GUILD_WAR_TYPE_MAX_NUM];
	int		draw[GUILD_WAR_TYPE_MAX_NUM];
	int		loss[GUILD_WAR_TYPE_MAX_NUM];

	int		gold;

#ifdef __DUNGEON_FOR_GUILD__
	BYTE	dungeon_ch;
	long	dungeon_map;
	DWORD	dungeon_cooldown;
#endif

	BYTE	bEmpire;
} TGuildData;

struct TGuildCreateParameter
{
	LPCHARACTER master;
	char name[GUILD_NAME_MAX_LEN+1];
};

typedef struct SGuildWar
{
	DWORD war_start_time;
	DWORD score;
	DWORD state;
	BYTE type;
	DWORD map_index;

	SGuildWar(BYTE type)
		: war_start_time(0),
	score(0),
	state(GUILD_WAR_RECV_DECLARE),
	type(type),
	map_index(0)
	{
	}
	bool IsWarBegin() const
	{
		return state == GUILD_WAR_ON_WAR;
	}
} TGuildWar;

class CGuild
{
	public:
		CGuild(TGuildCreateParameter& cp);
#ifdef __GUILD_SAFEBOX__
		explicit CGuild(DWORD guild_id) : m_SafeBox(this) { Load(guild_id); }
#else
		explicit CGuild(DWORD guild_id) { Load(guild_id); }
#endif
		~CGuild();

		DWORD		GetID() const	{ return m_data.guild_id; }
		const char*	GetName() const	{ return m_data.name; }
		int		GetSP() const		{ return m_data.power; }
		int		GetMaxSP() { return m_data.max_power; }
		DWORD		GetMasterPID() const	{ return m_data.master_pid; }
		LPCHARACTER	GetMasterCharacter();
		BYTE		GetLevel() const		{ return m_data.level; }

		void		Reset() { m_data.power = m_data.max_power; }

		bool		RequestDisband(DWORD pid);
		void		Disband();

		void		RequestAddMember(LPCHARACTER ch, int grade = 15);
		void		AddMember(DWORD pid, BYTE grade, bool is_general, BYTE job, BYTE level, DWORD offer, const char* name);

		bool		RequestRemoveMember(DWORD pid);
		bool		RemoveMember(DWORD pid);

		void		LoginMember(LPCHARACTER ch);
		void		P2PLoginMember(DWORD pid);

		void		LogoutMember(LPCHARACTER ch);
		void		P2PLogoutMember(DWORD pid);

		void		ChangeMemberGrade(DWORD pid, BYTE grade);
		bool		OfferExp(LPCHARACTER ch, int amount);
		void		LevelChange(DWORD pid, BYTE level);
		void		ChangeMemberData(DWORD pid, DWORD offer, BYTE level, BYTE grade);

		void		ChangeGradeName(BYTE grade, const char* grade_name);
		void		ChangeGradeAuth(BYTE grade, WORD auth);
		void		P2PChangeGrade(BYTE grade);

		bool		ChangeMemberGeneral(DWORD pid, BYTE is_general);

		bool		ChangeMasterTo(DWORD dwPID);

		template <typename T>
		void		Packet(network::GCOutputPacket<T>& packet) {
			std::for_each(m_memberOnline.begin(), m_memberOnline.end(), [&packet](LPCHARACTER ch) {
				if (LPDESC d = ch->GetDesc())
					d->Packet(packet);
				});
		}

		void		SendOnlineRemoveOnePacket(DWORD pid);
		void		SendAllGradePacket(LPCHARACTER ch);
		void		SendListPacket(LPCHARACTER ch);
		void		QueryLastPlayedlist(LPCHARACTER ch);
		void		SendLastplayedListPacket(LPCHARACTER ch, SQLMsg * pMsg);
		void		EncodeOneMemberListPacket(network::TGuildMemberInfo* member, const TGuildMember& member_info);
		void		SendListOneToAll(DWORD pid);
		void		SendListOneToAll(LPCHARACTER ch);
		void		SendLoginPacket(LPCHARACTER ch, LPCHARACTER chLogin);
		void		SendLogoutPacket(LPCHARACTER ch, LPCHARACTER chLogout);
		void		SendLoginPacket(LPCHARACTER ch, DWORD pid);
		void		SendLogoutPacket(LPCHARACTER ch, DWORD pid);
		void		SendGuildInfoPacket(LPCHARACTER ch);
		void		SendGuildDataUpdateToAllMember(SQLMsg* pmsg = NULL);

		void		Load(DWORD guild_id);
		void		SaveLevel();
		void		SaveSkill();
		void		SaveMember(DWORD pid);

		int		GetMaxMemberCount(); 
		int		GetMemberCount() { return m_member.size(); }
		int		GetTotalLevel() const;

#ifdef __DUNGEON_FOR_GUILD__
		BYTE	GetDungeonCH() const { return m_data.dungeon_ch; }
		long	GetDungeonMapIndex() const { return m_data.dungeon_map; }
		bool	RequestDungeon(BYTE bChannel, long lMapIndex);
		void	RecvDungeon(BYTE bChannel, long lMapIndex);
		DWORD	GetDungeonCooldown() const { return m_data.dungeon_cooldown; }
		bool	SetDungeonCooldown(DWORD dwTime);
		void	RecvDungeonCD(DWORD dwTime);
#endif

		BYTE	GetEmpire() { return m_data.bEmpire; }

		// GUILD_MEMBER_COUNT_BONUS
		void		SetMemberCountBonus(int iBonus);
		void		BroadcastMemberCountBonus();
		// END_OF_GUILD_MEMBER_COUNT_BONUS

		int		GetMaxGeneralCount() const	{ return 1 /*+ GetSkillLevel(GUILD_SKILL_DEUNGYONG)/3*/;}
		int		GetGeneralCount() const		{ return m_general_count; }

		TGuildMember*	GetMember(DWORD pid);
		DWORD			GetMemberPID(const std::string& strName);

		bool		HasGradeAuth(int grade, int auth_flag) const	{ return (bool)(m_data.grade_array[grade-1].auth_flag & auth_flag);}

		void		AddComment(LPCHARACTER ch, const std::string& str);
		void		DeleteComment(LPCHARACTER ch, DWORD comment_id);

		void		RefreshComment(LPCHARACTER ch);
		void		RefreshCommentForce(DWORD player_id);

		int			GetSkillLevel(DWORD vnum);
		void		SkillLevelUp(DWORD dwVnum);
		void		UseSkill(DWORD dwVnum, LPCHARACTER ch, DWORD pid);

		void		SendSkillInfoPacket(LPCHARACTER ch) const;
		void		ComputeGuildPoints();

		void		GuildPointChange( BYTE type, int amount, bool save = false );

		//void		GuildUpdateAffect(LPCHARACTER ch);
		//void		GuildRemoveAffect(LPCHARACTER ch);

		void		UpdateSkillLevel(BYTE skill_points, BYTE index, BYTE skill_level);
		void		SendDBSkillUpdate(int amount = 0);

		void		SkillRecharge();
		bool		ChargeSP(LPCHARACTER ch, int iSP);

		void		Chat(const char* c_pszText); 
		void		P2PChat(const char* c_pszText); // ±æµå Ã¤ÆÃ

		void		SkillUsableChange(DWORD dwSkillVnum, bool bUsable);
		void		AdvanceLevel(int iLevel);

		// Guild Money
		void		RequestDepositMoney(LPCHARACTER ch, int iGold);
		void		RequestWithdrawMoney(LPCHARACTER ch, int iGold);

		void		RecvMoneyChange(int iGold);
		void		RecvWithdrawMoneyGive(int iChangeGold); // bGive==1 ÀÌ¸é ±æµåÀå¿¡°Ô ÁÖ´Â °É ½ÃµµÇÏ°í ¼º°ø½ÇÆÐ¸¦ µðºñ¿¡°Ô º¸³½´Ù

		int		GetGuildMoney() const	{ return m_data.gold; }

		// War general
		void		GuildWarPacket(DWORD guild_id, BYTE bWarType, BYTE bWarState);
		void		SendEnemyGuild(LPCHARACTER ch);

		int		GetGuildWarState(DWORD guild_id);
		bool		CanStartWar(BYTE bGuildWarType);
		DWORD		GetWarStartTime(DWORD guild_id);
		bool		UnderWar(DWORD guild_id); // ÀüÀïÁßÀÎ°¡?
		DWORD		UnderAnyWar(BYTE bType = GUILD_WAR_TYPE_MAX_NUM);

		// War map relative
		void		SetGuildWarMapIndex(DWORD dwGuildID, long lMapIndex);
		int			GetGuildWarType(DWORD dwGuildOpponent);
		DWORD		GetGuildWarMapIndex(DWORD dwGuildOpponent);

		// War entry question
		void		GuildWarEntryAsk(DWORD guild_opp);
		void		GuildWarEntryAccept(DWORD guild_opp, LPCHARACTER ch);

		// War state relative
		void		NotifyGuildMaster(const char* msg);
		void		RequestDeclareWar(DWORD guild_id, BYTE type);
		void		RequestRefuseWar(DWORD guild_id); 

		bool		DeclareWar(DWORD guild_id, BYTE type, BYTE state); 
		void		RefuseWar(DWORD guild_id); 
		bool		WaitStartWar(DWORD guild_id); 
		bool		CheckStartWar(DWORD guild_id);	// check if StartWar method fails (call it before StartWar)
		void		StartWar(DWORD guild_id);
		void		EndWar(DWORD guild_id);
		void		ReserveWar(DWORD guild_id, BYTE type);

		// War points relative
		void		SetWarScoreAgainstTo(DWORD guild_opponent, int newpoint);
		int			GetWarScoreAgainstTo(DWORD guild_opponent);

		int			GetLadderPoint() const	{ return m_data.ladder_point; }
		void		SetLadderPoint(int point);

		void		SetWarData(const ::google::protobuf::RepeatedField<::google::protobuf::int32>& wins, const ::google::protobuf::RepeatedField<::google::protobuf::int32>& draws, const ::google::protobuf::RepeatedField<::google::protobuf::int32>& losses);

		void		ChangeLadderPoint(int iChange);

		int			GetGuildWarWinCount(BYTE type) const;
		int			GetGuildWarDrawCount(BYTE type) const;
		int			GetGuildWarLossCount(BYTE type) const;

		bool		HasLand();

		// GUILD_JOIN_BUG_FIX
		/// character ¿¡°Ô ±æµå°¡ÀÔ ÃÊ´ë¸¦ ÇÑ´Ù.
		/**
		 * @param	pchInviter ÃÊ´ëÇÑ character.
		 * @param	pchInvitee ÃÊ´ëÇÒ character.
		 *
		 * ÃÊ´ëÇÏ°Å³ª ¹ÞÀ»¼ö ¾ø´Â »óÅÂ¶ó¸é ÇØ´çÇÏ´Â Ã¤ÆÃ ¸Þ¼¼Áö¸¦ Àü¼ÛÇÑ´Ù.
		 */
		void		Invite( LPCHARACTER pchInviter, LPCHARACTER pchInvitee );

		/// ±æµåÃÊ´ë¿¡ ´ëÇÑ »ó´ë character ÀÇ ¼ö¶ôÀ» Ã³¸®ÇÑ´Ù.
		/**
		 * @param	pchInvitee ÃÊ´ë¹ÞÀº character
		 *
		 * ±æµå¿¡ °¡ÀÔ°¡´ÉÇÑ »óÅÂ°¡ ¾Æ´Ï¶ó¸é ÇØ´çÇÏ´Â Ã¤ÆÃ ¸Þ¼¼Áö¸¦ Àü¼ÛÇÑ´Ù.
		 */
		void		InviteAccept( LPCHARACTER pchInvitee );

		/// ±æµåÃÊ´ë¿¡ ´ëÇÑ »ó´ë character ÀÇ °ÅºÎ¸¦ Ã³¸®ÇÑ´Ù.
		/**
		 * @param	dwPID ÃÊ´ë¹ÞÀº character ÀÇ PID
		 */
		void		InviteDeny( DWORD dwPID );
		// END_OF_GUILD_JOIN_BUG_FIX

		void		ChangeName(LPCHARACTER ch, const char* c_pszNewName);
		void		P2P_ChangeName(const char* c_pszNewName);

		void LoadGuildData(SQLMsg* pmsg);
		void LoadGuildGradeData(SQLMsg* pmsg);
		void LoadGuildMemberData(SQLMsg* pmsg); 
		void P2PUpdateGrade(SQLMsg* pmsg);

	private:
		void		Initialize();

		TGuildData	m_data;
		int		m_general_count;

		// GUILD_MEMBER_COUNT_BONUS
		int		m_iMemberCountBonus;
		// END_OF_GUILD_MEMBER_COUNT_BONUS

		typedef std::map<DWORD, TGuildMember> TGuildMemberContainer;
		TGuildMemberContainer m_member;

		typedef CHARACTER_SET TGuildMemberOnlineContainer;
		TGuildMemberOnlineContainer m_memberOnline;

		typedef std::set<DWORD>	TGuildMemberP2POnlineContainer;
		TGuildMemberP2POnlineContainer m_memberP2POnline;

		typedef std::map<DWORD, TGuildWar> TEnemyGuildContainer;
		TEnemyGuildContainer m_EnemyGuild;

		std::map<DWORD, DWORD> m_mapGuildWarEndTime;

		bool	abSkillUsable[GUILD_SKILL_COUNT];

		// GUILD_JOIN_BUG_FIX
		/// ±æµå °¡ÀÔÀ» ÇÒ ¼ö ¾øÀ» °æ¿ìÀÇ ¿¡·¯ÄÚµå.
		enum GuildJoinErrCode {
			GERR_NONE			= 0,	///< Ã³¸®¼º°ø
			GERR_WITHDRAWPENALTY,		///< Å»ÅðÈÄ °¡ÀÔ°¡´ÉÇÑ ½Ã°£ÀÌ Áö³ªÁö ¾ÊÀ½
			GERR_COMMISSIONPENALTY,		///< ÇØ»êÈÄ °¡ÀÔ°¡´ÉÇÑ ½Ã°£ÀÌ Áö³ªÁö ¾ÊÀ½
			GERR_ALREADYJOIN,			///< ±æµå°¡ÀÔ ´ë»ó Ä³¸¯ÅÍ°¡ ÀÌ¹Ì ±æµå¿¡ °¡ÀÔÇØ ÀÖÀ½
			GERR_GUILDISFULL,			///< ±æµåÀÎ¿ø Á¦ÇÑ ÃÊ°ú
			GERR_GUILD_IS_IN_WAR,		///< ±æµå°¡ ÇöÀç ÀüÀïÁß
			GERR_INVITE_LIMIT,			///< ±æµå¿ø °¡ÀÔ Á¦ÇÑ »óÅÂ
			GERR_MAX				///< Error code ÃÖ°íÄ¡. ÀÌ ¾Õ¿¡ Error code ¸¦ Ãß°¡ÇÑ´Ù.
		};

		/// ±æµå¿¡ °¡ÀÔ °¡´ÉÇÑ Á¶°ÇÀ» °Ë»çÇÑ´Ù.
		/**
		 * @param [in]	pchInvitee ÃÊ´ë¹Þ´Â character
		 * @return	GuildJoinErrCode
		 */
		GuildJoinErrCode	VerifyGuildJoinableCondition( const LPCHARACTER pchInvitee );

		typedef std::map< DWORD, LPEVENT >	EventMap;
		EventMap	m_GuildInviteEventMap;	///< ±æµå ÃÊÃ» Event map. key: ÃÊ´ë¹ÞÀº Ä³¸¯ÅÍÀÇ PID
		// END_OF_GUILD_JOIN_BUG_FIX

	public:
		template <class Func> void ForEachOnMapMember(Func & f, long lMapIndex);
		template <class Func> void ForEachOnlineMember(Func f);

#ifdef __GUILD_SAFEBOX__
	private:
		CGuildSafeBox	m_SafeBox;
	public:
		CGuildSafeBox&	GetSafeBox()	{ return m_SafeBox; }
#endif

	protected:
		int		m_iGuildPostCommentPulse;
};

template <class Func> void CGuild::ForEachOnMapMember(Func & f, long lMapIndex)
{
	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;
		if (ch)
		{
			if (ch->GetMapIndex() == lMapIndex)
				f(ch);
		}
	}
}

template <class Func> void CGuild::ForEachOnlineMember(Func f)
{
	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;
		if (ch)
			f(ch);
	}
}

#endif
