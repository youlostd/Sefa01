#ifndef __INC_METIN_II_GAME_CHARACTER_MANAGER_H__
#define __INC_METIN_II_GAME_CHARACTER_MANAGER_H__

#include "../../common/stl.h"
#include "../../common/length.h"

#include "vid.h"

#include <google/protobuf/repeated_field.h>
#include "headers.hpp"
#include "protobuf_data.h"

class CDungeon;
class CHARACTER;
class CharacterVectorInteractor;

struct SQLMsg;

class CHARACTER_MANAGER : public singleton<CHARACTER_MANAGER>
{
	public:
		typedef TR1_NS::unordered_map<std::string, LPCHARACTER> NAME_MAP;

		CHARACTER_MANAGER();
		virtual ~CHARACTER_MANAGER();

		void					Destroy();

		void			GracefulShutdown();	// Á¤»óÀû ¼Ë´Ù¿îÇÒ ¶§ »ç¿ë. PC¸¦ ¸ðµÎ ÀúÀå½ÃÅ°°í Destroy ÇÑ´Ù.

		DWORD			AllocVID();

		LPCHARACTER			 CreateCharacter(const char * name, DWORD dwPID = 0);
		void			RemoveFromCharMap(LPCHARACTER ptr);

		void DestroyCharacter(LPCHARACTER ch);

		void			Update(int iPulse);

		LPCHARACTER		SpawnMob(DWORD dwVnum, long lMapIndex, long x, long y, long z, bool bSpawnMotion = false, int iRot = -1, bool bShow = true);
		LPCHARACTER		SpawnMobRange(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, bool bIsException=false, bool bSpawnMotion = false , bool bAggressive = false);
		LPCHARACTER		SpawnGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen = NULL, bool bAggressive_ = false, LPDUNGEON pDungeon = NULL);
		bool			SpawnGroupGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, LPREGEN pkRegen = NULL, bool bAggressive_ = false, LPDUNGEON pDungeon = NULL);
		bool			SpawnMoveGroup(DWORD dwVnum, long lMapIndex, int sx, int sy, int ex, int ey, int tx, int ty, LPREGEN pkRegen = NULL, bool bAggressive_ = false);
		LPCHARACTER		SpawnMobRandomPosition(DWORD dwVnum, long lMapIndex);

		void			SelectStone(LPCHARACTER pkChrStone);

		NAME_MAP &		GetPCMap() { return m_map_pkPCChr; }

		LPCHARACTER		Find(DWORD dwVID);
		LPCHARACTER		Find(const VID & vid);
		LPCHARACTER		FindPC(const char * name);
		LPCHARACTER		FindByPID(DWORD dwPID);

		bool			AddToStateList(LPCHARACTER ch);
		void			RemoveFromStateList(LPCHARACTER ch);

		// DelayedSave: ¾î¶°ÇÑ ·çÆ¾ ³»¿¡¼­ ÀúÀåÀ» ÇØ¾ß ÇÒ ÁþÀ» ¸¹ÀÌ ÇÏ¸é ÀúÀå
		// Äõ¸®°¡ ³Ê¹« ¸¹¾ÆÁö¹Ç·Î "ÀúÀåÀ» ÇÑ´Ù" ¶ó°í Ç¥½Ã¸¸ ÇØµÎ°í Àá±ñ
		// (¿¹: 1 frame) ÈÄ¿¡ ÀúÀå½ÃÅ²´Ù.
		void					DelayedSave(LPCHARACTER ch);
		bool					FlushDelayedSave(LPCHARACTER ch); // Delayed ¸®½ºÆ®¿¡ ÀÖ´Ù¸é Áö¿ì°í ÀúÀåÇÑ´Ù. ²÷±è Ã³¸®½Ã »ç¿ë µÊ.
		void			ProcessDelayedSave();

		template<class Func>	Func for_each_pc(Func f);

		void			RegisterForMonsterLog(LPCHARACTER ch);
		void			UnregisterForMonsterLog(LPCHARACTER ch);
		void			PacketMonsterLog(LPCHARACTER ch, network::GCOutputPacket<network::GCChatPacket>& chat_pack);

		void			KillLog(DWORD dwVnum);

		void			RegisterRaceNum(DWORD dwVnum);
		void			RegisterRaceNumMap(LPCHARACTER ch);
		void			UnregisterRaceNumMap(LPCHARACTER ch);
		bool			GetCharactersByRaceNum(DWORD dwRaceNum, CharacterVectorInteractor & i);

		LPCHARACTER		FindSpecifyPC(unsigned int uiJobFlag, long lMapIndex, LPCHARACTER except=NULL, int iMinLevel = 1, int iMaxLevel = PLAYER_MAX_LEVEL_CONST);

		void			SetMobItemRate(int value)	{ m_iMobItemRate = value; }
		void			SetMobDamageRate(int value)	{ m_iMobDamageRate = value; }
		void			SetMobGoldAmountRate(int value)	{ m_iMobGoldAmountRate = value; }
		void			SetMobGoldDropRate(int value)	{ m_iMobGoldDropRate = value; }
		void			SetMobExpRate(int value)	{ m_iMobExpRate = value; }

		void			SetMobItemRatePremium(int value)	{ m_iMobItemRatePremium = value; }
		void			SetMobGoldAmountRatePremium(int value)	{ m_iMobGoldAmountRatePremium = value; }
		void			SetMobGoldDropRatePremium(int value)	{ m_iMobGoldDropRatePremium = value; }
		void			SetMobExpRatePremium(int value)		{ m_iMobExpRatePremium = value; }

		void			SetUserDamageRatePremium(int value)	{ m_iUserDamageRatePremium = value; }
		void			SetUserDamageRate(int value ) { m_iUserDamageRate = value; }
		int			GetMobItemRate(LPCHARACTER ch);
		int			GetMobDamageRate(LPCHARACTER ch);
		int			GetMobGoldAmountRate(LPCHARACTER ch);
		int			GetMobGoldDropRate(LPCHARACTER ch);
		int			GetMobExpRate(LPCHARACTER ch);

		int			GetUserDamageRate(LPCHARACTER ch);
		void		SendScriptToMap(long lMapIndex, const std::string & s); 

		bool			IsPendingDestroy() const { return m_bUsePendingDestroy; }
		bool			BeginPendingDestroy();
		void			FlushPendingDestroy();

		void			LoadAverageDamage();
		void			ResultLoadAverageDamage(SQLMsg* pMsg);
		void			ResultLoadAverageDamageList(SQLMsg* pMsg);
#ifdef __UNIMPLEMENT__
		void			CharacterDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevel, int iDam);
		void			RefreshAverageDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevelGroup);
		WORD			GetAverageDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevel) const;
		WORD			__GetAverageDamage(BYTE bJob, bool bPVP, BYTE bType, BYTE bLevelGroup) const;
		void			SaveAverageDamage();
#endif

		const DWORD*	GetUsableSkillList(BYTE bJob, BYTE bSkillGroup) const;

	public:
		void	InitializeHorseUpgrade(const ::google::protobuf::RepeatedPtrField<network::THorseUpgradeProto>& table);
		void	InitializeHorseBonus(const ::google::protobuf::RepeatedPtrField<network::THorseBonusProto>& table);

		const network::THorseUpgradeProto* GetHorseUpgrade(BYTE bType, BYTE bLevel);
		const network::THorseBonusProto*	GetHorseBonus(BYTE bLevel);

	private:
		std::map<std::pair<BYTE, BYTE>, network::THorseUpgradeProto>	m_map_HorseUpgrade;
		network::THorseBonusProto	m_akHorseBonus[HORSE_MAX_BONUS_LEVEL];
		
	private:
		int					m_iMobItemRate;
		int					m_iMobDamageRate;
		int					m_iMobGoldAmountRate;
		int					m_iMobGoldDropRate;
		int					m_iMobExpRate;

		int					m_iMobItemRatePremium;
		int					m_iMobGoldAmountRatePremium;
		int					m_iMobGoldDropRatePremium;
		int					m_iMobExpRatePremium;

		int					m_iUserDamageRate;
		int					m_iUserDamageRatePremium;
		int					m_iVIDCount;

		TR1_NS::unordered_map<DWORD, LPCHARACTER> m_map_pkChrByVID;
		TR1_NS::unordered_map<DWORD, LPCHARACTER> m_map_pkChrByPID;
		NAME_MAP			m_map_pkPCChr;

		char				dummy1[1024];	// memory barrier
		CHARACTER_SET		m_set_pkChrState;	// FSMÀÌ µ¹¾Æ°¡°í ÀÖ´Â ³ðµé
		CHARACTER_SET		m_set_pkChrForDelayedSave;
		CHARACTER_SET		m_set_pkChrMonsterLog;

		LPCHARACTER			m_pkChrSelectedStone;

		std::map<DWORD, DWORD> m_map_dwMobKillCount;

		std::set<DWORD>		m_set_dwRegisteredRaceNum;
		std::map<DWORD, CHARACTER_SET> m_map_pkChrByRaceNum;

		bool				m_bUsePendingDestroy;
		CHARACTER_SET		m_set_pkChrPendingDestroy;
		
		WORD				m_awRealDamage[JOB_MAX_NUM][2][2][PLAYER_MAX_LEVEL_CONST / 10][100];
		BYTE				m_bRealDamageVectorIndex[JOB_MAX_NUM][2][2][PLAYER_MAX_LEVEL_CONST / 10];
		std::vector<WORD>	m_vec_wAverageDamage[JOB_MAX_NUM][2][2][PLAYER_MAX_LEVEL_CONST / 10];
		WORD				m_awAverageDamage[JOB_MAX_NUM][2][2][PLAYER_MAX_LEVEL_CONST / 10];
		BYTE				m_bLastAverageRefresh[JOB_MAX_NUM][2][2][PLAYER_MAX_LEVEL_CONST / 10];

	public:
		void				SendOnlineTeamlerList(LPDESC pkDesc, BYTE bLanguage);
		void				AddOnlineTeamler(const char* szName, BYTE bLanguage);
		void				RemoveOnlineTeamler(const char* szName, BYTE bLanguage);
	private:
		std::set<std::string>		m_set_OnlineTeamler[LANGUAGE_MAX_NUM];
};

	template<class Func>	
Func CHARACTER_MANAGER::for_each_pc(Func f)
{
	TR1_NS::unordered_map<DWORD, LPCHARACTER>::iterator it;

	for (it = m_map_pkChrByPID.begin(); it != m_map_pkChrByPID.end(); ++it)
		f(it->second);

	return f;
}

class CharacterVectorInteractor : public CHARACTER_VECTOR
{
	public:
		CharacterVectorInteractor() : m_bMyBegin(false) { }

		CharacterVectorInteractor(const CHARACTER_SET & r);
		virtual ~CharacterVectorInteractor();

	private:
		bool m_bMyBegin;
};

#define M2_DESTROY_CHARACTER(ptr) CHARACTER_MANAGER::instance().DestroyCharacter(ptr)

#endif
