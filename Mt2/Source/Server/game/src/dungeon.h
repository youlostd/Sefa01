#ifndef __INC_METIN_II_GAME_DUNGEON_H
#define __INC_METIN_II_GAME_DUNGEON_H

#include "sectree_manager.h"

class TDungeonPlayerInfo
{
	public:
		long map;
		int x;
		int y;
};
	
class CParty;

class CDungeon
{
	typedef TR1_NS::unordered_map<LPPARTY, int> TPartyMap;
	typedef std::map<std::string, LPCHARACTER> TUniqueMobMap;

	public:
	// <Factor> Non-persistent identifier type
	typedef uint32_t IdType;

	~CDungeon();

	// <Factor>
	IdType GetId() const { return m_id; }

	// DUNGEON_NOTICE
	void	Notice(const char* msg);
	// END_OF_DUNGEON_NOTICE
#ifdef ENABLE_ZODIAC_TEMPLE
	void	Notice_Zodiac(const char* msg);
#endif
	void	JoinParty(LPPARTY pParty);
	void	QuitParty(LPPARTY pParty);

	void	Join(LPCHARACTER ch);

	void	IncMember(LPCHARACTER ch);
	void	DecMember(LPCHARACTER ch);

	// DUNGEON_KILL_ALL_BUG_FIX
	void	Purge();
	void	KillAll(bool bOnlyEnemies);
	// END_OF_DUNGEON_KILL_ALL_BUG_FIX

	void	IncMonster() { m_iMonsterCount++; m_iMonsterAliveCount++; }
	void	DecMonster() { m_iMonsterCount--; CheckEliminated(); }
	void	DecAliveMonster() { m_iMonsterAliveCount--; }
	int	CountMonster() { return m_iMonsterCount; }	// µ¥ÀÌÅÍ·Î ¸®Á¨ÇÑ ¸ó½ºÅÍÀÇ ¼ö
	int	CountAliveMonster() { return m_iMonsterAliveCount; }

	void	IncPartyMember(LPPARTY pParty, LPCHARACTER ch);
	void	DecPartyMember(LPPARTY pParty, LPCHARACTER ch);

	void	IncKillCount(LPCHARACTER pkKiller, LPCHARACTER pkVictim);
	int	GetKillMobCount();
	int	GetKillStoneCount();
	bool	IsUsePotion();
	bool	IsUseRevive();
	void	UsePotion(LPCHARACTER ch);
	void	UseRevive(LPCHARACTER ch);

	long	GetMapIndex() { return m_lMapIndex; }

#ifdef __RIFT_SYSTEM__
	void	SetEnemyHPFactor(int iFactor);
	int		GetEnemyHPFactor() const { return m_iEnemyHPFactor; }
	void	SetEnemyDamFactor(int iFactor);
	int		GetEnemyDamFactor() const { return m_iEnemyDamFactor; }
#endif

	void	Spawn(DWORD vnum, const char* pos);
	LPCHARACTER	SpawnMob(DWORD vnum, int x, int y, int dir = 0);
	LPCHARACTER	SpawnMob_ac_dir(DWORD vnum, int x, int y, int dir = 0);
	LPCHARACTER	SpawnGroup(DWORD vnum, long x, long y, float radius, bool bAggressive=false, int count=1);

	void	SpawnNameMob(DWORD vnum, int x, int y, const char* name);
	void	SpawnGotoMob(long lFromX, long lFromY, long lToX, long lToY);

	void	SpawnRegen(const char* filename, bool bOnce = true);
	void	AddRegen(LPREGEN regen);
	void	ClearRegen();
	bool	IsValidRegen(LPREGEN regen, size_t regen_id);

	void	SetUnique(const char* key, DWORD vid);
	void	SpawnMoveUnique(const char* key, DWORD vnum, const char* pos_from, const char* pos_to);
	void	SpawnMoveGroup(DWORD vnum, const char* pos_from, const char* pos_to, int count=1);
	void	SpawnUnique(const char* key, DWORD vnum, const char* pos);
	void	SpawnStoneDoor(const char* key, const char* pos);
	void	SpawnWoodenDoor(const char* key, const char* pos);
	void	KillUnique(const std::string& key);
	void	PurgeUnique(const std::string& key);
	bool	IsUniqueDead(const std::string& key);
	float	GetUniqueHpPerc(const std::string& key);
	DWORD	GetUniqueVid(const std::string& key);
	LPCHARACTER	UniqueGet(const std::string& key);

	void	DeadCharacter(LPCHARACTER ch);

	void	UniqueSetMaxHP(const std::string& key, int iMaxHP);
	void	UniqueSetHP(const std::string& key, int iHP);
	void	UniqueSetDefGrade(const std::string& key, int iGrade);

	void	SendDestPositionToParty(LPPARTY pParty, long x, long y);

	void	CheckEliminated();

	void	JumpAll(long lFromMapIndex, int x, int y, bool bCleanUp = true);
	void	WarpAll(long lFromMapIndex, int x, int y);
	void	JumpParty(LPPARTY pParty, long lFromMapIndex, int x, int y);

	void	ExitAll();
	void	CmdChat(const char* msg, ...);
	void	ExitAllToStartPosition();
	void	JumpToEliminateLocation();
	void	SetExitAllAtEliminate(long time);
	void	SetWarpAtEliminate(long time, long lMapIndex, int x, int y, const char* regen_file);

	int	GetFlag(std::string name);
	void	SetFlag(std::string name, int value);
	const char*	GetStringFlag(std::string name);
	const std::map<std::string, int>&	GetFlagMap() { return m_map_Flag; }
	void	SetStringFlag(std::string name, std::string value);
	void	SetWarpLocation (long map_index, int x, int y);
	void	SkipPlayerSaveDungeonOnce() { m_bSkipSaveWarpOnce = true; }
	// item groupÀº item_vnum°ú item_count·Î ±¸¼º.
	typedef std::vector <std::pair <DWORD, int> > ItemGroup;
	void	CreateItemGroup (std::string& group_name, ItemGroup& item_group);
	const ItemGroup* GetItemGroup (std::string& group_name);
	//void	InsertItemGroup (std::string& group_name, DWORD item_vnum);

	template <class Func> Func ForEachMember(Func &f);

	bool IsAllPCNearTo( int x, int y, int dist );

	protected:
	CDungeon(IdType id, long lOriginalMapIndex, long lMapIndex);

	void	Initialize();
	void	CheckDestroy();

	private:
	IdType 		m_id; // <Factor>
	DWORD		m_lOrigMapIndex;
	DWORD		m_lMapIndex;
	bool		m_bSkipSaveWarpOnce;

	CHARACTER_SET		m_set_pkCharacter;
	std::map<std::string, int>  m_map_Flag;
	std::map<std::string, std::string>  m_map_StringFlag;
	typedef std::map<std::string, ItemGroup> ItemGroupMap;
	ItemGroupMap m_map_ItemGroup;
	TPartyMap	m_map_pkParty;
	TAreaMap&	m_map_Area;
	TUniqueMobMap	m_map_UniqueMob;

	bool	m_bCompleted;
	int		m_iMobKill;
	int		m_iStoneKill;
	bool		m_bUsePotion;
	bool		m_bUseRevive;

	int		m_iMonsterCount;
	int		m_iMonsterAliveCount;

	bool		m_bExitAllAtEliminate;
	bool		m_bWarpAtEliminate;

#ifdef __RIFT_SYSTEM__
	int		m_iEnemyHPFactor;
	int		m_iEnemyDamFactor;
#endif

	// Àû Àü¸ê½Ã ¿öÇÁÇÏ´Â À§Ä¡
	int		m_iWarpDelay;
	long		m_lWarpMapIndex;
	long		m_lWarpX;
	long		m_lWarpY;
	std::string	m_stRegenFile;

	std::vector<LPREGEN> m_regen;

	LPEVENT		deadEvent;
	// <Factor>
	LPEVENT exit_all_event_;
	LPEVENT jump_to_event_;
	size_t regen_id_;

	friend class CDungeonManager;
	friend EVENTFUNC(dungeon_dead_event);
	// <Factor>
	friend EVENTFUNC(dungeon_exit_all_event);
	friend EVENTFUNC(dungeon_jump_to_event);

	// ÆÄÆ¼ ´ÜÀ§ ´øÀü ÀÔÀåÀ» À§ÇÑ ÀÓ½Ã º¯¼ö.
	// m_map_pkParty´Â °ü¸®°¡ ºÎ½ÇÇÏ¿© »ç¿ëÇÒ ¼ö ¾ø´Ù°í ÆÇ´ÜÇÏ¿©,
	// ÀÓ½Ã·Î ÇÑ ÆÄÆ¼¿¡ ´ëÇÑ °ü¸®¸¦ ÇÏ´Â º¯¼ö »ý¼º.
	
	LPPARTY m_pParty;
	public :
	void SetPartyNull();
	public:
		void	MoveAllMonsterToPlayer();
		void	Completed();
#ifdef __DUNGEON_RANKING__
	private:
		DWORD	m_dwStartDungeon;
#endif
};

class CDungeonManager : public singleton<CDungeonManager>
{
	typedef std::map<CDungeon::IdType, LPDUNGEON> TDungeonMap;
	typedef std::map<long, LPDUNGEON> TMapDungeon;
	typedef std::map<DWORD, TDungeonPlayerInfo> TMapDungeonPlayerInfo;
	
	public:
	CDungeonManager();
	virtual ~CDungeonManager();

	LPDUNGEON	Create(long lOriginalMapIndex);
	void		Destroy(CDungeon::IdType dungeon_id);
	LPDUNGEON	Find(CDungeon::IdType dungeon_id);
	LPDUNGEON	FindByMapIndex(long lMapIndex);
	void		SetPlayerInfo(DWORD pid, TDungeonPlayerInfo s);
	void		SetPlayerInfo(LPCHARACTER ch);
	void		RemovePlayerInfoDungeon(long map_idx);
	void		RemovePlayerInfo(DWORD pid);
	bool 		HasPlayerInfo(DWORD pid) { return m_map_pkDungeonPlayerInfo.find(pid) != m_map_pkDungeonPlayerInfo.end(); }
	TDungeonPlayerInfo GetPlayerInfo(DWORD pid);

	private:
	TDungeonMap	m_map_pkDungeon;
	TMapDungeon m_map_pkMapDungeon;
	TMapDungeonPlayerInfo m_map_pkDungeonPlayerInfo;

	// <Factor> Introduced unsigned 32-bit dungeon identifier
	CDungeon::IdType next_id_;
};

#endif
