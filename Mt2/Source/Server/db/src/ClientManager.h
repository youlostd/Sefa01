// vim:ts=8 sw=4
#ifndef __INC_CLIENTMANAGER_H__
#define __INC_CLIENTMANAGER_H__

#include <unordered_map>
#include <unordered_set>

#include "../../common/stl.h"
#include "../../common/building.h"

#include "Peer.h"
#include "DBManager.h"
#include "LoginData.h"
#include "Cache.h"

#include <functional>


class CPlayerTableCache;
class CItemCache;
class CQuestCache;
class CAffectCache;
class CItemPriceListTableCache;
#ifdef CHANGE_SKILL_COLOR
class CSkillColorCache;
#endif
#ifdef __EQUIPMENT_CHANGER__
class CEquipmentPageCache;
#endif

class CPacketInfo
{
    public:
	void Add(int header);
	void Reset();

	std::map<int, int> m_map_info;
};

template <typename T>
std::vector<uint8_t> SerializeProtobufRepeatedField(const google::protobuf::RepeatedField<T>& repeated_field)
{
	std::vector<uint8_t> vec;
	uint32_t elem_count = repeated_field.size();
	uint32_t vec_size = sizeof(uint32_t) + sizeof(T) * elem_count;

	vec.resize(vec_size);
	uint8_t* write_ptr = &vec[0];

	memcpy(write_ptr, &elem_count, sizeof(uint32_t));
	write_ptr += sizeof(uint32_t);

	for (const auto& elem : repeated_field)
	{
		memcpy(write_ptr, &elem, sizeof(T));
		write_ptr += sizeof(T);
	}

	return vec;
}

template <typename T>
std::vector<uint8_t> SerializeProtobufRepeatedPtrField(const google::protobuf::RepeatedPtrField<T>& repeated_field)
{
	std::vector<uint8_t> vec;
	uint32_t vec_size = sizeof(uint32_t);

	for (const auto& elem : repeated_field)
		vec_size += elem.ByteSize() + sizeof(uint16_t);

	vec.resize(vec_size);
	uint8_t* write_ptr = &vec[0];

	memcpy(write_ptr, &vec_size, sizeof(uint32_t));
	write_ptr += sizeof(uint32_t);

	for (const auto& elem : repeated_field)
	{
		uint16_t elem_size = elem.ByteSize();
		memcpy(write_ptr, &elem_size, sizeof(uint16_t));
		write_ptr += sizeof(uint16_t);

		elem.SerializeToArray(write_ptr, elem_size);
		write_ptr += elem_size;
	}

	return vec;
}

template <typename T>
void ParseProtobufRepeatedField(void* buffer, uint32_t buffer_size, google::protobuf::RepeatedField<T>* repeated_field)
{
	repeated_field->Clear();

	const uint8_t* read_ptr = (const uint8_t*)buffer;

	if (buffer_size < sizeof(uint32_t))
		return;

	uint32_t elem_count = *((uint32_t*)read_ptr);
	read_ptr += sizeof(uint32_t);
	buffer_size -= sizeof(uint32_t);

	while (elem_count > 0)
	{
		if (buffer_size < sizeof(T))
			break;

		auto elem = repeated_field->Add();
		memcpy(elem, read_ptr, sizeof(T));
		read_ptr += sizeof(T);
		buffer_size -= sizeof(T);

		elem_count--;
	}
}

template <typename T>
void ParseProtobufRepeatedPtrField(void* buffer, uint32_t buffer_size, google::protobuf::RepeatedPtrField<T>* repeated_field)
{
	repeated_field->Clear();

	const uint8_t* read_ptr = (const uint8_t*)buffer;

	if (buffer_size < sizeof(uint32_t))
		return;

	uint32_t size = *((uint32_t*)read_ptr);
	read_ptr += sizeof(uint32_t);
	buffer_size -= sizeof(uint32_t);

	while (size > 0)
	{
		if (buffer_size < sizeof(uint16_t))
			break;

		uint16_t elem_size = *((uint16_t*)read_ptr);
		read_ptr += sizeof(uint16_t);
		buffer_size -= sizeof(uint16_t);

		if (buffer_size < elem_size)
			break;

		auto elem = repeated_field->Add();
		elem->ParseFromArray(read_ptr, elem_size);
		read_ptr += elem_size;
		buffer_size -= elem_size;
	}
}

#ifdef __ATTRTREE__
bool CreatePlayerAttrtreeSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab, BYTE row, BYTE col);
#endif
size_t CreatePlayerSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab);
size_t CreatePlayerMountSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab);
#ifdef ENABLE_RUNE_SYSTEM
size_t CreatePlayerRuneSaveQuery(char * pszQuery, size_t querySize, TPlayerTable * pkTab);
#endif
bool CreateItemTableFromRes(MYSQL_RES* res, std::function<network::TItemData * ()> alloc_func, DWORD pid, int iCol = 0);
extern int CreateItemTableFromRow(MYSQL_ROW row, network::TItemData * pRes, int iCol = 0, bool bWindowDataCols = true);
const char* GetItemQueryKeyPart(bool bSelect);
const char* GetItemQueryKeyPart(bool bSelect, const char* c_pszTableName, const char* c_pszIDColumnName = "id", const char* c_pszRemoveColumns = "");
const char* GetItemQueryValuePart(const network::TItemData * pkTab, bool bWindowDataCols = true);

// special key types for player cache types
typedef struct SQuestKeyType {
	std::string	stQuestName;
	std::string	stFlagName;

	SQuestKeyType(const TQuestTable* pQuestTab) : stQuestName(pQuestTab->name()), stFlagName(pQuestTab->state()) {}
	SQuestKeyType(const char* c_pszQuestName, const char* c_pszFlagName) : stQuestName(c_pszQuestName), stFlagName(c_pszFlagName) {}

	bool operator==(const SQuestKeyType& o) const { return stQuestName == o.stQuestName && stFlagName == o.stFlagName; }
	bool operator<(const SQuestKeyType& o) const {
		if (stQuestName < o.stQuestName) return true;
		else return stFlagName < o.stFlagName;
	}
} QUEST_KEY_TYPE;

typedef struct SAffectKeyType {
	WORD		wAffectID;
	BYTE		bApplyType;

	SAffectKeyType(const TPacketAffectElement* pAffectTab) : wAffectID(pAffectTab->type()), bApplyType(pAffectTab->apply_on()) {}
	SAffectKeyType(WORD wAffectID, BYTE bApplyType) : wAffectID(wAffectID), bApplyType(bApplyType) {}

	bool operator==(const SAffectKeyType& o) const { return wAffectID == o.wAffectID && bApplyType == o.bApplyType; }
	bool operator<(const SAffectKeyType& o) const {
		if (wAffectID < o.wAffectID) return true;
		else return bApplyType < o.bApplyType;
	}
} AFFECT_KEY_TYPE;

namespace std
{
	template <>
	struct hash<QUEST_KEY_TYPE>
	{
		size_t operator()(const QUEST_KEY_TYPE& k) const
		{
			// Compute individual hash values for first, second and third
			// http://stackoverflow.com/a/1646913/126995
			size_t res = 17;
			res = res * 31 + hash<std::string>()(k.stQuestName);
			res = res * 31 + hash<std::string>()(k.stFlagName);
			return res;
		}
	};
	template <>
	struct hash<AFFECT_KEY_TYPE>
	{
		size_t operator()(const AFFECT_KEY_TYPE& k) const
		{
			// Compute individual hash values for first, second and third
			// http://stackoverflow.com/a/1646913/126995
			size_t res = 17;
			res = res * 31 + hash<WORD>()(k.wAffectID);
			res = res * 31 + hash<BYTE>()(k.bApplyType);
			return res;
		}
	};
}

class CClientManager : public CNetBase, public singleton<CClientManager>
{
    public:
	typedef std::list<CPeer *>			TPeerList;

	// different player cache types
	public:
	typedef std::unordered_set<CItemCache *, std::hash<CItemCache*> > TItemCacheSet;
	typedef std::unordered_map<QUEST_KEY_TYPE, CQuestCache *> TQuestCacheMap;
	typedef std::unordered_map<AFFECT_KEY_TYPE, CAffectCache *> TAffectCacheMap;
#ifdef __PET_ADVANCED__
	typedef std::unordered_map<DWORD, CPetAdvancedCache*> TPetCacheMap;
#endif

	// player cache type
	typedef struct SPlayerCache {
		CPlayerTableCache*	pPlayer;
		TItemCacheSet		setItems;
		TQuestCacheMap		mapQuests;
		TAffectCacheMap		mapAffects;
	} TPlayerCache;
	typedef std::unordered_map<DWORD, TPlayerCache *> TPlayerCacheMap;

	// global player cache types
	typedef std::unordered_map<DWORD, CItemCache *> TItemCacheMap;

	// global cache types
	typedef std::unordered_map<DWORD, CItemPriceListTableCache*> TItemPriceListCacheMap;

	// MYSHOP_PRICE_LIST
	/// ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® ¿äÃ» Á¤º¸
	/**
	 * first: Peer handle
	 * second: ¿äÃ»ÇÑ ÇÃ·¹ÀÌ¾îÀÇ ID
	 */
	typedef std::pair< DWORD, DWORD >		TItemPricelistReqInfo;
	// END_OF_MYSHOP_PRICE_LIST

#ifdef CHANGE_SKILL_COLOR
	typedef std::unordered_map<DWORD, CSkillColorCache *> TSkillColorCacheMap;
#endif

#ifdef __EQUIPMENT_CHANGER__
	typedef std::unordered_map<std::string, CEquipmentPageCache *> TEquipmentPageCacheMap;
	typedef std::unordered_set<CEquipmentPageCache *, std::hash<CEquipmentPageCache*> > TEquipmentPageCacheSet;
	typedef std::unordered_map<DWORD, TEquipmentPageCacheSet *> TEquipmentPageCacheSetPtrMap;
#endif

	class ClientHandleInfo
	{
	    public:
		DWORD	dwHandle;
		DWORD	account_id;
		DWORD	player_id;
		BYTE	account_index;
		char	login[LOGIN_MAX_LEN + 1];
		char	safebox_password[SAFEBOX_PASSWORD_MAX_LEN + 1];
		int		safebox_size;
		char	ip[MAX_HOST_LENGTH + 1];

		network::TAccountTable * pAccountTable;

		ClientHandleInfo(DWORD argHandle, DWORD dwPID = 0)
		{
		    dwHandle = argHandle;
		    pAccountTable = NULL;
		    player_id = dwPID;
			safebox_size = -1;
		};
		//µ¶ÀÏ¼±¹°±â´É¿ë »ý¼ºÀÚ
		ClientHandleInfo(DWORD argHandle, DWORD dwPID, DWORD accountId)
		{
		    dwHandle = argHandle;
		    pAccountTable = NULL;
		    player_id = dwPID;
			account_id = accountId;
			safebox_size = -1;
		};

		~ClientHandleInfo()
		{
		}
	};

	public:
	CClientManager();
	~CClientManager();

	bool	Initialize();

	void	Destroy();
	time_t	GetCurrentTime();

	void	MainLoop();
	void	Quit();

	void	SetPlayerIDStart(int iIDStart);
	int	GetPlayerIDStart() { return m_iPlayerIDStart; }
	BYTE GetLastHeader() { return m_bLastHeader; }

	int	GetPlayerDeleteLevelLimit() { return m_iPlayerDeleteLevelLimit; }

	void	SendAllGuildSkillRechargePacket();
	void	SendTime();

	TPlayerCache *	GetPlayerCache(DWORD id, bool bForceCreate);
	void			FlushPlayerCache(DWORD id);
	void			UpdatePlayerCache();

	void			PutPlayerCache(const TPlayerTable * pNew, bool bSkipSave = false);
	void			PutItemCache(const network::TItemData * pNew, bool bSkipQuery = false);
	void			PutQuestCache(const TQuestTable * pNew, bool bSkipSave = false);
	void			PutAffectCache(DWORD dwPID, const TPacketAffectElement * pNew, bool bSkipSave = false);
	void			PutAffectCache(const TAffectSaveElement* pNew, bool bSkipSave = false);

	void			DeleteQuestCache(DWORD dwPlayerID, const QUEST_KEY_TYPE& c_rkQuestKey);
	void			DeleteAffectCache(DWORD dwPlayerID, const AFFECT_KEY_TYPE& c_rkAffectKey);

	CItemCache *	GetItemCache(DWORD item_id);
	bool			DeleteItemCache(DWORD item_id);
	void			EraseItemCache(DWORD item_id);
	//UpdateItemCach() fuer EQ-Changer
	void 			UpdateItemCach();

#ifdef __PET_ADVANCED__
	bool				IsPetItem(DWORD item_vnum);

	CPetAdvancedCache* GetPetCache(DWORD item_id);
	void				PutPetCache(network::TPetAdvancedTable* pNew, bool bSkipQuery = false);
	bool				DeletePetCache(DWORD item_id);
	void				FlushPetCache(DWORD item_id);

	network::TPetAdvancedTable* RequestPetDataForItem(DWORD item_id, CPeer* targetPeer);

	void				RESULT_PET_LOAD(CPeer* peer, SQLMsg* pMsg);
	void				RESULT_PET_SKILL_LOAD(CPeer* peer, SQLMsg* pMsg);
	void				QUERY_PET_SAVE(CPeer* peer, std::unique_ptr<network::GDPetSavePacket> pack);
#endif

#ifdef CHANGE_SKILL_COLOR
	CSkillColorCache *	GetSkillColorCache(DWORD id);
	void				PutSkillColorCache(std::unique_ptr<network::GDSkillColorSavePacket> packet);
	void				UpdateSkillColorCache();
#endif

#ifdef __EQUIPMENT_CHANGER__
	void					CreateEquipmentPageCacheSet(DWORD dwPID);
	TEquipmentPageCacheSet*	GetEquipmentPageCacheSet(DWORD dwPID);
	void					FlushEquipmentPageCacheSet(DWORD dwPID);

	CEquipmentPageCache*	GetEquipmentPageCache(DWORD pid, DWORD index);
	void					PutEquipmentPageCache(const network::TEquipmentChangerTable* pNew, bool bSkipQuery = false);
	bool					DeleteEquipmentPageCache(DWORD pid, DWORD index);
	void					EraseEquipmentPageCache(DWORD pid, DWORD index);
#endif
#ifdef __EQUIPMENT_CHANGER__
	void			UpdateEquipmentPageCache();
#endif

	// MYSHOP_PRICE_LIST
	/// °¡°ÝÁ¤º¸ ¸®½ºÆ® Ä³½Ã¸¦ °¡Á®¿Â´Ù.
	/**
	 * @param [in]	dwID °¡°ÝÁ¤º¸ ¸®½ºÆ®ÀÇ ¼ÒÀ¯ÀÚ.(ÇÃ·¹ÀÌ¾î ID)
	 * @return	°¡°ÝÁ¤º¸ ¸®½ºÆ® Ä³½ÃÀÇ Æ÷ÀÎÅÍ
	 */
	CItemPriceListTableCache*	GetItemPriceListCache(DWORD dwID);

	/// °¡°ÝÁ¤º¸ ¸®½ºÆ® Ä³½Ã¸¦ ³Ö´Â´Ù.
	/**
	 * @param [in]	pItemPriceList Ä³½Ã¿¡ ³ÖÀ» ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ®
	 *
	 * Ä³½Ã°¡ ÀÌ¹Ì ÀÖÀ¸¸é Update °¡ ¾Æ´Ñ replace ÇÑ´Ù.
	 */
	void			PutItemPriceListCache(const TItemPriceListTable* pItemPriceList);


	/// Flush ½Ã°£ÀÌ ¸¸·áµÈ ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® Ä³½Ã¸¦ Flush ÇØÁÖ°í Ä³½Ã¿¡¼­ »èÁ¦ÇÑ´Ù.
	void			UpdateItemPriceListCache(void);
	// END_OF_MYSHOP_PRICE_LIST


	void			SendGuildSkillUsable(DWORD guild_id, DWORD dwSkillVnum, bool bUsable);

	void			SetCacheFlushCountLimit(int iLimit);

	template <class Func>
	Func		for_each_peer(Func f);

	CPeer *		GetAnyPeer();

#ifdef PROCESSOR_CORE
	CPeer* GetProcessorPeer() const
	{
		for (auto peer : m_peerList)
		{
			if (peer->IsProcessorCore())
				return peer;
		}

		return nullptr;
	}
#endif

	template <typename T>
	void			ForwardPacket(network::DGOutputPacket<T>& pack, BYTE bChannel = 0, CPeer* except = NULL)
	{
		for (CPeer* peer : m_peerList)
		{
			if (peer == except)
				continue;

			if (!peer->GetChannel())
				continue;

			if (bChannel && peer->GetChannel() != bChannel)
				continue;

			peer->Packet(pack);
		}
	}

	void			ForwardPacket(network::TDGHeader header, BYTE bChannel = 0, CPeer* except = NULL)
	{
		for (CPeer* peer : m_peerList)
		{
			if (peer == except)
				continue;

			if (!peer->GetChannel())
				continue;

			if (bChannel && peer->GetChannel() != bChannel)
				continue;

			peer->Packet(header);
		}
	}

	void			SendNotice(const char * c_pszFormat, ...);

	char*			GetCommand(char* str);					//µ¶ÀÏ¼±¹°±â´É¿¡¼­ ¸í·É¾î ¾ò´Â ÇÔ¼ö
	void			ItemAward(CPeer * peer, const char* login);	//µ¶ÀÏ ¼±¹° ±â´É

	CPeer*		GetAuthPeer() const { return m_pkAuthPeer; }

    private:
	bool		InitializeLanguage();
	bool		InitializeTables();
	bool		InitializeShopTable();
	bool		InitializeMobTableFromTextFile();
	bool		InitializeMobTableFromDatabase();
	bool		InitializeItemTableFromTextFile();
	bool		InitializeItemTableFromDatabase();
	bool		InitializeSkillTable();
	bool		InitializeRefineTable();
	bool		InitializeItemAttrTable();
#ifdef EL_COSTUME_ATTR
	bool		InitializeItemCostumeAttrTable();
#endif
#ifdef SOUL_SYSTEM
	void		InitializeSoulProtoTable();
#endif
#ifdef CRYSTAL_SYSTEM
	void		InitializeCrystalProtoTable();
#endif
#ifdef ITEM_RARE_ATTR
	bool		InitializeItemRareTable();
#endif
	bool		InitializeLandTable();
	bool		InitializeObjectProto();
	bool		InitializeObjectTable();

	bool		InitializeHorseUpgradeTable();
	bool		InitializeHorseBonusTable();
#ifdef __GAYA_SYSTEM__
	bool		InitializeGayaShop();
#endif
#ifdef __ATTRTREE__
	bool		InitializeAttrTree();
#endif

#ifdef ENABLE_RUNE_SYSTEM
	bool		InitializeRuneProto();
#endif
#ifdef __PET_ADVANCED__
	bool		InitializePetSkillTable();
	bool		InitializePetEvolveTable();
	bool		InitializePetAttrTable();
#endif

#ifdef ENABLE_XMAS_EVENT
	void		InitializeXmasRewards();
	void		ReloadXmasRewards();
#endif

	int		TransferPlayerTable();

	// mob_proto.txt, item_proto.txt¿¡¼­ ÀÐÀº mob_proto, item_proto¸¦ real db¿¡ ¹Ý¿µ.
	//	item_proto, mob_proto¸¦ db¿¡ ¹Ý¿µÇÏÁö ¾Ê¾Æµµ, °ÔÀÓ µ¹¾Æ°¡´Âµ¥´Â ¹®Á¦°¡ ¾øÁö¸¸,
	//	¿î¿µÅø µî¿¡¼­ dbÀÇ item_proto, mob_proto¸¦ ÀÐ¾î ¾²±â ¶§¹®¿¡ ¹®Á¦°¡ ¹ß»ýÇÑ´Ù.
	bool		MirrorMobTableIntoDB();
	bool		MirrorItemTableIntoDB();

	public:
	void		AddPeer(socket_t fd);
	void		RemovePeer(CPeer * pPeer);
	CPeer *		GetPeer(IDENT ident);

	private:
	int		AnalyzeQueryResult(SQLMsg * msg);
	int		AnalyzeErrorMsg(CPeer * peer, SQLMsg * msg);

	int		Process();

        void            ProcessPackets(CPeer * peer);

	CLoginData *	GetLoginData(DWORD dwKey);
	CLoginData *	GetLoginDataByLogin(const char * c_pszLogin);
	CLoginData *	GetLoginDataByAID(DWORD dwAID);

	void		InsertLoginData(CLoginData * pkLD);
	void		DeleteLoginData(CLoginData * pkLD);

	bool		InsertLogonAccount(const char * c_pszLogin, DWORD dwHandle, const char * c_pszIP);
	bool		DeleteLogonAccount(const char * c_pszLogin, DWORD dwHandle);
	bool		FindLogonAccount(const char * c_pszLogin);

	void		GuildCreate(CPeer * peer, std::unique_ptr<network::GDGuildCreatePacket> p);
	void		GuildSkillUpdate(CPeer * peer, std::unique_ptr<network::GDGuildSkillUpdatePacket> p);
	void		GuildExpUpdate(CPeer * peer, std::unique_ptr<network::GDGuildExpUpdatePacket> p);
	void		GuildAddMember(CPeer * peer, std::unique_ptr<network::GDGuildAddMemberPacket> p);
	void		GuildChangeGrade(CPeer * peer, std::unique_ptr<network::GDGuildChangeGradePacket> p);
	void		GuildRemoveMember(CPeer * peer, std::unique_ptr<network::GDGuildRemoveMemberPacket> p);
	void		GuildChangeMemberData(CPeer * peer, std::unique_ptr<network::GDGuildChangeMemberDataPacket> p);
	void		GuildDisband(CPeer * peer, std::unique_ptr<network::GDGuildDisbandPacket> p);
	void		GuildWar(CPeer * peer, std::unique_ptr<network::GDGuildWarPacket> p);
	void		GuildWarScore(CPeer * peer, std::unique_ptr<network::GDGuildWarScorePacket> p);
	void		GuildChangeLadderPoint(std::unique_ptr<network::GDGuildChangeLadderPointPacket> p);
	void		GuildUseSkill(std::unique_ptr<network::GDGuildUseSkillPacket> p);
	void		GuildDepositMoney(std::unique_ptr<network::GDGuildDepositMoneyPacket> p);
	void		GuildWithdrawMoney(CPeer* peer, std::unique_ptr<network::GDGuildWithdrawMoneyPacket> p);
	void		GuildWithdrawMoneyGiveReply(std::unique_ptr<network::GDGuildWithdrawMoneyGiveReplyPacket> p);
	void		GuildWarBet(std::unique_ptr<network::GDGuildWarBetPacket> p);
	void		GuildChangeMaster(std::unique_ptr<network::GDGuildReqChangeMasterPacket> p);

	void		SetGuildWarEndTime(DWORD guild_id1, DWORD guild_id2, time_t tEndTime);

	void		QUERY_BOOT(CPeer * peer, std::unique_ptr<network::GDBootPacket> p);

	void		QUERY_LOGIN(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDAuthLoginPacket> data);
	void		QUERY_LOGOUT(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDLogoutPacket> p);

	void		RESULT_LOGIN(CPeer * peer, SQLMsg *msg);

	void		QUERY_PLAYER_LOAD(CPeer * peer, DWORD dwHandle, network::GDPlayerLoadPacket* p);
	void		RESULT_COMPOSITE_PLAYER(CPeer * peer, SQLMsg * pMsg, DWORD dwQID);
	void		RESULT_PLAYER_LOAD(CPeer * peer, MYSQL_RES * pRes, ClientHandleInfo * pkInfo);
	void		RESULT_ITEM_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_QUEST_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_AFFECT_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
	void		RESULT_OFFLINE_MESSAGES_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);

	// PLAYER_INDEX_CREATE_BUG_FIX
	void		RESULT_PLAYER_INDEX_CREATE(CPeer *pkPeer, SQLMsg *msg);
	// END_PLAYER_INDEX_CREATE_BUG_FIX
 
#ifdef CHANGE_SKILL_COLOR
	void		QUERY_SKILL_COLOR_LOAD(CPeer * peer, DWORD dwHandle, network::GDPlayerLoadPacket* packet);
	void		RESULT_SKILL_COLOR_LOAD(CPeer * peer, MYSQL_RES * pRes, DWORD dwHandle);
	void		QUERY_SKILL_COLOR_SAVE(std::unique_ptr<network::GDSkillColorSavePacket> packet);
#endif

#ifdef __EQUIPMENT_CHANGER__
	void 		QUERY_EQUIPMENT_CHANGER_LOAD(CPeer * peer, DWORD dwHandle, network::GDPlayerLoadPacket* packet);
	void		RESULT_EQUIPMENT_PAGE_LOAD(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD dwPID);
#endif

	// MYSHOP_PRICE_LIST
	/// °¡°ÝÁ¤º¸ ·Îµå Äõ¸®¿¡ ´ëÇÑ Result Ã³¸®
	/**
	 * @param	peer °¡°ÝÁ¤º¸¸¦ ¿äÃ»ÇÑ Game server ÀÇ peer °´Ã¼ Æ÷ÀÎÅÍ
	 * @param	pMsg Äõ¸®ÀÇ Result ·Î ¹ÞÀº °´Ã¼ÀÇ Æ÷ÀÎÅÍ
	 *
	 * ·ÎµåµÈ °¡°ÝÁ¤º¸ ¸®½ºÆ®¸¦ Ä³½Ã¿¡ ÀúÀåÇÏ°í peer ¿¡°Ô ¸®½ºÆ®¸¦ º¸³»ÁØ´Ù.
	 */
	void		RESULT_PRICELIST_LOAD(CPeer* peer, SQLMsg* pMsg);

	/// °¡°ÝÁ¤º¸ ¾÷µ¥ÀÌÆ®¸¦ À§ÇÑ ·Îµå Äõ¸®¿¡ ´ëÇÑ Result Ã³¸®
	/**
	 * @param	pMsg Äõ¸®ÀÇ Result ·Î ¹ÞÀº °´Ã¼ÀÇ Æ÷ÀÎÅÍ
	 *
	 * ·ÎµåµÈ Á¤º¸·Î °¡°ÝÁ¤º¸ ¸®½ºÆ® Ä³½Ã¸¦ ¸¸µé°í ¾÷µ¥ÀÌÆ® ¹ÞÀº °¡°ÝÁ¤º¸·Î ¾÷µ¥ÀÌÆ® ÇÑ´Ù.
	 */
	void		RESULT_PRICELIST_LOAD_FOR_UPDATE(SQLMsg* pMsg);
	// END_OF_MYSHOP_PRICE_LIST

	void		QUERY_PLAYER_SAVE(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDPlayerSavePacket> p);
#ifdef ENABLE_RUNE_SYSTEM
	void		QUERY_PLAYER_RUNE_SAVE(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDPlayerRuneSavePacket> p);
#endif
	void		__QUERY_PLAYER_CREATE(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDPlayerCreatePacket> p);
	void		__QUERY_PLAYER_DELETE(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDPlayerDeletePacket> p);
	void		__RESULT_PLAYER_DELETE(CPeer * peer, SQLMsg* msg);

	void		QUERY_ITEM_SAVE(CPeer * pkPeer, std::unique_ptr<network::GDItemSavePacket> p);
	void		QUERY_ITEM_DESTROY(CPeer * pkPeer, std::unique_ptr<network::GDItemDestroyPacket> p);
	void		QUERY_ITEM_FLUSH(CPeer * pkPeer, std::unique_ptr<network::GDItemFlushPacket> p);


	void		QUERY_QUEST_SAVE(CPeer * pkPeer, std::unique_ptr<network::GDQuestSavePacket> p);
	void		QUERY_ADD_AFFECT(CPeer * pkPeer, std::unique_ptr<network::GDAddAffectPacket> p);
	void		QUERY_REMOVE_AFFECT(CPeer * pkPeer, std::unique_ptr<network::GDRemoveAffectPacket> p);

#ifdef __EQUIPMENT_CHANGER__
	void		QUERY_EQUIPMENT_CHANGER_SAVE(CPeer * pkPeer, std::unique_ptr<network::GDEquipmentPageSavePacket> p);
	void		QUERY_EQUIPMENT_CHANGER_DELETE(CPeer * pkPeer, std::unique_ptr<network::GDEquipmentPageDeletePacket> p);
#endif

	void		QUERY_SAFEBOX_LOAD(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDSafeboxLoadPacket> p);
	void		QUERY_SAFEBOX_SAVE(CPeer * pkPeer, std::unique_ptr<network::GDSafeboxSavePacket> p);
	void		QUERY_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDSafeboxChangeSizePacket> p);
	void		QUERY_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDSafeboxChangePasswordPacket> p);

	void		RESULT_SAFEBOX_LOAD(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_SIZE(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD(CPeer * pkPeer, SQLMsg * msg);
	void		RESULT_SAFEBOX_CHANGE_PASSWORD_SECOND(CPeer * pkPeer, SQLMsg * msg);

	void		QUERY_EMPIRE_SELECT(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDEmpireSelectPacket> p);
	void		QUERY_SETUP(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDSetupPacket> p);

	void		SendPartyOnSetup(CPeer * peer);

	void		QUERY_FLUSH_CACHE(CPeer * pkPeer, std::unique_ptr<network::GDFlushCachePacket> p);

	void		QUERY_PARTY_CREATE(CPeer * peer, std::unique_ptr<network::GDPartyCreatePacket> p);
	void		QUERY_PARTY_DELETE(CPeer * peer, std::unique_ptr<network::GDPartyDeletePacket> p);
	void		QUERY_PARTY_ADD(CPeer * peer, std::unique_ptr<network::GDPartyAddPacket> p);
	void		QUERY_PARTY_REMOVE(CPeer * peer, std::unique_ptr<network::GDPartyRemovePacket> p);
	void		QUERY_PARTY_STATE_CHANGE(CPeer * peer, std::unique_ptr<network::GDPartyStateChangePacket> p);
	void		QUERY_PARTY_SET_MEMBER_LEVEL(CPeer * peer, std::unique_ptr<network::GDPartySetMemberLevelPacket> p);

	void		QUERY_RELOAD_PROTO();
	void		QUERY_RELOAD_MOB_PROTO();

	void		ReloadShopProto();

	void		QUERY_CHANGE_NAME(CPeer * peer, DWORD dwHandle, std::unique_ptr<network::GDChangeNamePacket> p);
	void		GetPlayerFromRes(TPlayerTable * player_table, MYSQL_RES* res);

	void		QUERY_LOGIN_KEY(CPeer * pkPeer, std::unique_ptr<network::GDLoginByKeyPacket> p);

	void		AddGuildPriv(std::unique_ptr<network::GDRequestGuildPrivPacket> p);
	void		AddEmpirePriv(std::unique_ptr<network::GDRequestEmpirePrivPacket> p);
	void		AddCharacterPriv(std::unique_ptr<network::GDRequestCharacterPrivPacket> p);

	void		QUERY_AUTH_LOGIN(CPeer * pkPeer, DWORD dwHandle,std::unique_ptr<network::GDAuthLoginPacket> p);

	void		QUERY_LOGIN_BY_KEY(CPeer * pkPeer, DWORD dwHandle, std::unique_ptr<network::GDLoginByKeyPacket> p);
	void		RESULT_LOGIN_BY_KEY(CPeer * peer, SQLMsg * msg);

	void		LoadEventFlag();
	void		SetEventFlag(std::unique_ptr<network::GDSetEventFlagPacket> p);
	void		SendEventFlagsOnSetup(CPeer* peer);
	void		SaveEventFlags();
	public:
	int			GetEventFlag(const std::string& c_rstFlagName) const;
	private:
#ifdef __DEPRECATED_BILLING__
	void		BillingExpire(std::unique_ptr<network::GDBillingExpirePacket> p);
	void		BillingCheck(std::unique_ptr<network::GDBillingCheckPacket> p);

	void		SendAllLoginToBilling();
	void		SendLoginToBilling(CLoginData * pkLD, bool bLogin);
#endif

	// °áÈ¥
	void		MarriageAdd(std::unique_ptr<network::GDMarriageAddPacket> p);
	void		MarriageUpdate(std::unique_ptr<network::GDMarriageUpdatePacket> p);
	void		MarriageRemove(std::unique_ptr<network::GDMarriageRemovePacket> p);

	void		WeddingRequest(std::unique_ptr<network::GDWeddingRequestPacket> p);
	void		WeddingReady(std::unique_ptr<network::GDWeddingReadyPacket> p);
	void		WeddingEnd(std::unique_ptr<network::GDWeddingEndPacket> p);

	// MYSHOP_PRICE_LIST
	// °³ÀÎ»óÁ¡ °¡°ÝÁ¤º¸

	/// ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® ¾÷µ¥ÀÌÆ® ÆÐÅ¶(HEADER_GD_MYSHOP_PRICELIST_UPDATE) Ã³¸®ÇÔ¼ö
	/**
	 * @param [in]	pPacket ÆÐÅ¶ µ¥ÀÌÅÍÀÇ Æ÷ÀÎÅÍ
	 */
	void		MyshopPricelistUpdate(std::unique_ptr<network::GDMyShopPricelistUpdatePacket> pPacket);

	/// ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® ¿äÃ» ÆÐÅ¶(HEADER_GD_MYSHOP_PRICELIST_REQ) Ã³¸®ÇÔ¼ö
	/**
	 * @param	peer ÆÐÅ¶À» º¸³½ Game server ÀÇ peer °´Ã¼ÀÇ Æ÷ÀÎÅÍ
	 * @param [in]	dwHandle °¡°ÝÁ¤º¸¸¦ ¿äÃ»ÇÑ peer ÀÇ ÇÚµé
	 * @param [in]	dwPlayerID °¡°ÝÁ¤º¸ ¸®½ºÆ®¸¦ ¿äÃ»ÇÑ ÇÃ·¹ÀÌ¾îÀÇ ID
	 */
	void		MyshopPricelistRequest(CPeer* peer, DWORD dwHandle, DWORD dwPlayerID);
	// END_OF_MYSHOP_PRICE_LIST

	// Building
	void		CreateObject(std::unique_ptr<network::GDCreateObjectPacket> p);
	void		DeleteObject(std::unique_ptr<network::GDDeleteObjectPacket> p);
	void		UpdateLand(std::unique_ptr<network::GDUpdateLandPacket> p);

	// BLOCK_CHAT
	void		BlockChat(std::unique_ptr<network::GDBlockChatPacket> p);
	// END_OF_BLOCK_CHAT }

#ifdef COMBAT_ZONE
	void		UpdateSkillsCache(std::unique_ptr<network::GDCombatZoneSkillsCachePacket> p);
#endif

#ifdef __MAINTENANCE__
	void		Maintenance(std::unique_ptr<network::GDRecvShutdownPacket> p);
#endif

	void		RecvItemTimedIgnore(std::unique_ptr<network::GDItemTimedIgnorePacket> p);

    private:
	int					m_looping;
	socket_t				m_fdAccept;	// Á¢¼Ó ¹Þ´Â ¼ÒÄÏ
	TPeerList				m_peerList;

	CPeer *					m_pkAuthPeer;

	// LoginKey, LoginData pair
	typedef std::unordered_map<DWORD, CLoginData *> TLoginDataByLoginKey;
	TLoginDataByLoginKey			m_map_pkLoginData;

	// Login LoginData pair
	typedef std::unordered_map<std::string, CLoginData *> TLoginDataByLogin;
	TLoginDataByLogin			m_map_pkLoginDataByLogin;
	
	// AccountID LoginData pair
	typedef std::unordered_map<DWORD, CLoginData *> TLoginDataByAID;
	TLoginDataByAID				m_map_pkLoginDataByAID;

	// Login LoginData pair (½ÇÁ¦ ·Î±×ÀÎ µÇ¾îÀÖ´Â °èÁ¤)
	typedef std::unordered_map<std::string, CLoginData *> TLogonAccountMap;
	TLogonAccountMap			m_map_kLogonAccount;

	int					m_iPlayerIDStart;
	int					m_iPlayerDeleteLevelLimit;
	int					m_iPlayerDeleteLevelLimitLower;

	std::vector<network::TMobTable>			m_vec_mobTable;
	std::vector<network::TItemTable>			m_vec_itemTable;
	std::map<DWORD, network::TItemTable *>		m_map_itemTableByVnum;

	int					m_iShopTableSize;
	network::TShopTable *				m_pShopTable;

	int					m_iRefineTableSize;
	network::TRefineTable*				m_pRefineTable;

	std::vector<network::TSkillTable>		m_vec_skillTable;
	std::vector<network::TItemAttrTable>		m_vec_itemAttrTable;
#ifdef EL_COSTUME_ATTR
	std::vector<network::TItemAttrTable>		m_vec_itemCostumeAttrTable;
#endif
#ifdef ITEM_RARE_ATTR
	std::vector<network::TItemAttrTable>		m_vec_itemRareTable;
#endif
#ifdef SOUL_SYSTEM
	std::vector<network::TSoulProtoTable>	m_vec_soulAttrTable;
#endif
#ifdef CRYSTAL_SYSTEM
	std::vector<network::TCrystalProto>	m_vec_crystalTable;
#endif
#ifdef __GAYA_SYSTEM__
	std::vector<network::TGayaShopData>		m_vec_gayaShopTable;
#endif

	std::vector<network::TBuildingLand>		m_vec_kLandTable;
	std::vector<network::TBuildingObjectProto>	m_vec_kObjectProto;
	std::map<DWORD, std::unique_ptr<network::TBuildingObject>>	m_map_pkObjectTable;

#ifdef __PET_ADVANCED__
	std::vector<network::TPetAdvancedSkillProto>	m_vec_PetSkillProto;
	std::vector<network::TPetAdvancedEvolveProto> m_vec_PetEvolveProto;
	std::vector<network::TPetAdvancedAttrProto> m_vec_PetAttrProto;
#endif

	std::vector<network::THorseUpgradeProto>		m_vec_HorseUpgradeProto;
	std::vector<network::THorseBonusProto>		m_vec_HorseBonusProto;

#ifdef __ATTRTREE__
	TAttrtreeProto						m_aAttrTreeProto[ATTRTREE_ROW_NUM][ATTRTREE_COL_NUM];
#endif
	
#ifdef ENABLE_RUNE_SYSTEM
	std::vector<network::TRuneProtoTable>		m_vec_RuneProto;
	std::vector<network::TRunePointProtoTable>	m_vec_RunePointProto;
#endif

#ifdef ENABLE_XMAS_EVENT
	std::vector<network::TXmasRewards>			m_vec_xmasRewards;
#endif

	bool					m_bShutdowned;

	// player cache
	TPlayerCacheMap				m_map_playerCache;
	// global item cache
	TItemCacheMap				m_map_itemCacheByID;  // ¾ÆÀÌÅÛ id°¡ key

	// MYSHOP_PRICE_LIST
	/// ÇÃ·¹ÀÌ¾îº° ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ® map. key: ÇÃ·¹ÀÌ¾î ID, value: °¡°ÝÁ¤º¸ ¸®½ºÆ® Ä³½Ã
	TItemPriceListCacheMap m_mapItemPriceListCache;  ///< ÇÃ·¹ÀÌ¾îº° ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ¸®½ºÆ®
	// END_OF_MYSHOP_PRICE_LIST

#ifdef CHANGE_SKILL_COLOR
	TSkillColorCacheMap			m_map_SkillColorCache;
#endif

#ifdef __EQUIPMENT_CHANGER__
	TEquipmentPageCacheMap			m_map_equipmentPageCache;
	TEquipmentPageCacheSetPtrMap	m_map_pkEquipmentPageSetPtr;
#endif

#ifdef __PET_ADVANCED__
	TPetCacheMap				m_map_petCache;
#endif

	typedef struct SWhisperPlayerExistCheckQueryData {
		network::GDWhisperPlayerExistCheckPacket	kMainData;
		DWORD								dwPlayerHandle;
		char*								pTextData;
	} TWhisperPlayerExistCheckQueryData;

	struct TPartyInfo
	{
	    BYTE bRole;
	    BYTE bLevel;

		TPartyInfo() :bRole(0), bLevel(0)
		{
		}
	};

	typedef std::map<DWORD, TPartyInfo>	TPartyMember;
	typedef std::map<DWORD, TPartyMember>	TPartyMap;
#ifdef __PARTY_GLOBAL__
	TPartyMap	m_map_pkParty;
#else
	typedef std::map<unsigned char, TPartyMap>	TPartyChannelMap;
	TPartyChannelMap m_map_pkChannelParty;
#endif

	typedef std::map<std::string, long>	TEventFlagMap;
	TEventFlagMap m_map_lEventFlag;

	BYTE					m_bLastHeader;
	DWORD					m_iCacheFlushCount;
	DWORD					m_iCacheFlushCountLimit;

    private :
	network::TItemIDRangeTable m_itemRange;

    public :
	bool InitializeNowItemID();
	DWORD GetItemID();
	DWORD GainItemID();
	network::TItemIDRangeTable GetItemRange() { return m_itemRange; }

    private:
	//ADMIN_MANAGER
	bool __GetAdminInfo(google::protobuf::RepeatedPtrField<network::TAdminInfo>* admins);
	bool __GetAdminConfig(google::protobuf::RepeatedField<google::protobuf::uint32>* configs);
	//END_ADMIN_MANAGER

		
	//RELOAD_ADMIN
	void ReloadAdmin();
	//END_RELOAD_ADMIN
	void BreakMarriage(CPeer * peer, std::unique_ptr<network::GDMarriageBreakPacket> p);

	struct TLogoutPlayer
	{
	    DWORD	pid;
	    time_t	time;

	    bool operator < (const TLogoutPlayer & r) 
	    {
		return (pid < r.pid);
	    }
	};

	typedef std::unordered_map<DWORD, TLogoutPlayer*> TLogoutPlayerMap;
	TLogoutPlayerMap m_map_logout;
	
	void InsertLogoutPlayer(DWORD pid);
	void DeleteLogoutPlayer(DWORD pid);
	void UpdateLogoutPlayer();

	void SendSpareItemIDRange(CPeer* peer);

	void DeleteLoginKey(std::unique_ptr<network::GDDisconnectPacket> data);
	void ResetLastPlayerID(std::unique_ptr<network::GDValidLogoutPacket> data);

#ifdef __DUNGEON_FOR_GUILD__
	void	GuildDungeon(std::unique_ptr<network::GDGuildDungeonPacket> sPacket);
	void	GuildDungeonGD(std::unique_ptr<network::GDGuildDungeonCDPacket> sPacket);
#endif

	void ForceItemDelete(DWORD dwItemID);

	void AddExistingPlayerName(const std::string& stName, DWORD dwPID, bool bIsBlocked);
	void RemoveExistingPlayerName(const std::string& stName);
	bool FindExistingPlayerName(const std::string& stName, DWORD* pdwPID = NULL, bool* pbIsBlocked = NULL);
	void SetExistingPlayerBlocked(const std::string& stName, bool bIsBlocked);

	void WhisperPlayerExistCheck(CPeer* peer, DWORD dwHandle, std::unique_ptr<network::GDWhisperPlayerExistCheckPacket> p);
	void WhisperPlayerMessageOffline(CPeer* peer, DWORD dwHandle, std::unique_ptr<network::GDWhisperPlayerMessageOfflinePacket> p);
	void RESULT_WhisperPlayerExistCheck(CPeer* peer, SQLMsg* pMsg, CQueryInfo* pData);

	private:
	std::map<std::string, std::pair<DWORD, bool> >	m_map_ExistingPlayerNameList;

#ifdef __ITEM_REFUND__
	public:
	void QUERY_ITEM_REFUND(CPeer* peer, DWORD dwHandle, DWORD pid);
	void RESULT_ITEM_REFUND(CPeer * pkPeer, MYSQL_RES * pRes, DWORD dwHandle, DWORD pid);
#endif

#ifdef __HAIR_SELECTOR__
	public:
	void QUERY_SELECT_UPDATE_HAIR(CPeer* peer, std::unique_ptr<network::GDSelectUpdateHairPacket> p);
#endif

	public:
	void QUERY_ITEM_DESTROY_LOG(std::unique_ptr<network::GDItemDestroyLogPacket> p);

	public:
	const network::TItemTable* GetItemTable(DWORD dwVnum) const;

	private:
	bool		PARTY_FIND_BY_LEADER(DWORD dwLeaderPID, TPartyMap::iterator& itRet, TPartyMap** ppRetPartyMap = NULL);

#ifdef __CHECK_P2P_BROKEN__
	public:
	DWORD		GetCurrentPeerAmount();
#endif

#ifdef __CHECK_P2P_BROKEN__
	struct FSendCheckP2P
	{
		FSendCheckP2P(DWORD dwCurrentAmount)
		{
			p.dwValidPeerCount = dwCurrentAmount;
		}

		void operator() (CPeer* peer)
		{
			// Skip auth/mark...
			if (!peer->GetChannel())
				return;

			peer->EncodeHeader(HEADER_DG_CHECKP2P, 0, sizeof(TPacketDGCheckP2P));
			peer->Encode(&p, sizeof(TPacketDGCheckP2P));
		}

		TPacketDGCheckP2P p;
	};
#endif

#ifdef AUCTION_SYSTEM
	private:
		std::unordered_map<DWORD, DWORD> _last_auction_item_save;
#endif
};

template<class Func>	
Func CClientManager::for_each_peer(Func f)
{
    TPeerList::iterator it;
    for (it = m_peerList.begin(); it!=m_peerList.end();++it)
    {
	f(*it);
    }
    return f;
}

#endif
