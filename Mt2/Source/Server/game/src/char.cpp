#include "stdafx.h"

#include "../../common/VnumHelper.h"

#include "char.h"

#include "config.h"
#include "utils.h"
#include "crc32.h"
#include "char_manager.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "item_manager.h"
#include "motion.h"
#include "vector.h"
#include "packet.h"
#include "cmd.h"
#include "fishing.h"
#include "exchange.h"
#include "battle.h"
#include "affect.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "pvp.h"
#include "party.h"
#include "start_position.h"
#include "questmanager.h"
#include "log.h"
#include "p2p.h"
#include "guild.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "messenger_manager.h"
#include "unique_item.h"
#include "priv_manager.h"
#include "war_map.h"
#include "xmas_event.h"
#include "target.h"
#include "wedding.h"
#include "mob_manager.h"
#include "mining.h"
#include "arena.h"
#include "dev_log.h"
#include "gm.h"
#include "map_location.h"
#include "BlueDragon_Binder.h"
#include "skill_power.h"
#include "buff_on_attributes.h"
#include "OXEvent.h"
#include "mount_system.h"
#include "refine.h"

#ifdef __PET_SYSTEM__
#include "PetSystem.h"
#endif
#ifdef __GUILD_SAFEBOX__
#include "guild_safebox.h"
#endif
#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif
#ifdef __ATTRTREE__
#include "attrtree_manager.h"
#endif
#ifdef __EVENT_MANAGER__
#include "event_manager.h"
#endif

#ifdef __MELEY_LAIR_DUNGEON__
#include "MeleyLair.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_HYDRA_DUNGEON
#include "HydraDungeon.h"
#endif

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif

#ifdef DMG_RANKING
#include "dmg_ranking.h"
#endif

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#endif

extern const BYTE g_aBuffOnAttrPoints;
extern bool RaceToJob(unsigned race, unsigned *ret_job);

extern int g_nPortalLimitTime;
extern int test_server;

extern bool IS_SUMMONABLE_ZONE(int map_index); // char_item.cpp
bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index);
int* CHARACTER_GetWarpLevelLimit(int iMapIndex, bool bIsMin);

bool CAN_ENTER_ZONE_CHECKLEVEL(const LPCHARACTER& ch, int map_index, bool bSendMessage)
{
	// if (ch->IsGM() && !test_server)
		// return true;
	
	int* piLevelLimitMin = CHARACTER_GetWarpLevelLimit(map_index, true);
	int* piLevelLimitMax = CHARACTER_GetWarpLevelLimit(map_index, false);
	if (piLevelLimitMin && ch->GetLevel() < *piLevelLimitMin)
	{
		if (bSendMessage)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your level is too low to warp to this map."));
		return false;
	}
	else if (piLevelLimitMax && ch->GetLevel() > *piLevelLimitMax)
	{
		if (bSendMessage)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your level is too high to warp to this map."));
		return false;
	}
	if(test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s(%p, %i, %d) pLevelLimitMin: %d pLevelLimitMax: %d", __FUNCTION__, ch, map_index, bSendMessage, piLevelLimitMin ? *piLevelLimitMin : -1,
		piLevelLimitMax ? *piLevelLimitMax : -1);

	return true;
}

bool CAN_ENTER_ZONE(const LPCHARACTER& ch, int map_index)
{
	if (ch->IsGM() && !test_server)
		return true;
	
	bool bOnlyLocalWarp = false;
	switch (map_index)
	{
		case SKIPIA_MAP_INDEX_1:
		case SKIPIA_MAP_INDEX_2:
			bOnlyLocalWarp = true;
			break;
		default:
			break;
	}
	
	if (bOnlyLocalWarp && !test_server && map_index != ch->GetMapIndex())
	{
		if ((ch->GetMapIndex() == SKIPIA_MAP_INDEX_1 || ch->GetMapIndex() == SKIPIA_MAP_INDEX_2) == false)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can only warp on this map, if you are on it."));
			return false;
		}
	}
	
	// oxevent only if running
	if (map_index == OXEVENT_MAP_INDEX && COXEventManager::instance().GetStatus() != OXEVENT_OPEN)
	{
		sys_err("%s tries to enter ox event while not running", ch->GetName());
		LogManager::instance().HackLog("ENTER_OX", ch);
		return false;
	}

	if (!CAN_ENTER_ZONE_CHECKLEVEL(ch, map_index, false))
		return false;
	
	return true;
}

// <Factor> DynamicCharacterPtr member function definitions

LPCHARACTER DynamicCharacterPtr::Get() const {
	LPCHARACTER p = NULL;
	if (is_pc) {
		p = CHARACTER_MANAGER::instance().FindByPID(id);
	} else {
		p = CHARACTER_MANAGER::instance().Find(id);
	}
	return p;
}

DynamicCharacterPtr& DynamicCharacterPtr::operator=(LPCHARACTER character) {
	if (character == NULL) {
		Reset();
		return *this;
	}
	if (character->IsPC()) {
		is_pc = true;
		id = character->GetPlayerID();
	} else {
		is_pc = false;
		id = character->GetVID();
	}
	return *this;
}

CHARACTER::CHARACTER()
{
	m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateIdle, &CHARACTER::EndStateEmpty);
	m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
	m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateBattle, &CHARACTER::EndStateEmpty);

	Initialize();
}

CHARACTER::~CHARACTER()
{
	Destroy();
}

void CHARACTER::Initialize()
{
	CEntity::Initialize(ENTITY_CHARACTER);

	m_bNoOpenedShop = true;
	m_pkAuraUpdateEvent = NULL;
	m_bOpeningSafebox = false;

	m_fSyncTime = get_float_time()-3;
	m_dwPlayerID = 0;
	m_dwKillerPID = 0;

	m_iMoveCount = 0;

	m_pkRegen = NULL;
	regen_id_ = 0;
	m_pMovingWay = NULL;
	m_iMovingWayIndex = 0;
	m_iMovingWayMaxNum = 0;
	m_bMovingWayRepeat = false;
	m_lMovingWayBaseX = 0;
	m_lMovingWayBaseY = 0;
	m_posRegen.x = m_posRegen.y = m_posRegen.z = 0;
	m_posStart.x = m_posStart.y = 0;
	m_posDest.x = m_posDest.y = 0;
	m_fRegenAngle = 0.0f;

	m_pkMobData		= NULL;
	m_pkMobInst		= NULL;

	m_pkShop		= NULL;
	m_pkChrShopOwner	= NULL;
	m_pkMyShop		= NULL;
	m_pkExchange	= NULL;
	m_pkParty		= NULL;
	m_pkPartyRequestEvent = NULL;

	m_pGuild = NULL;

	m_pkChrTarget = NULL;

	m_pkMuyeongEvent = NULL;

	m_pkWarpNPCEvent = NULL;
	m_pkDeadEvent = NULL;
	m_pkStunEvent = NULL;
	m_pkSaveEvent = NULL;
	m_pkRecoveryEvent = NULL;
	m_pkTimedEvent = NULL;
	m_pkChannelSwitchEvent = NULL;
	m_pkFishingEvent = NULL;
	m_pkWarpEvent = NULL;

	// MINING
	m_pkMiningEvent = NULL;
	// END_OF_MINING

	m_pkPoisonEvent = NULL;
	m_pkFireEvent = NULL;
	m_pkCheckSpeedHackEvent	= NULL;
	m_speed_hack_count	= 0;

	m_PolymorphEvent = NULL;

	m_pkAffectEvent = NULL;
	m_afAffectFlag = TAffectFlag(0, 0);

	m_pkDestroyWhenIdleEvent = NULL;

	m_pkChrSyncOwner = NULL;

	memset(&m_points, 0, sizeof(m_points));
	memset(&m_pointsInstant, 0, sizeof(m_pointsInstant));
	memset(&m_pointsInstantF[0], 0, sizeof(m_pointsInstantF));
	memset(&m_quickslot, 0, sizeof(m_quickslot));
	memset(&character_cards, 0, sizeof(character_cards));
    memset(&randomized_cards, 0, sizeof(randomized_cards));

	m_bCharType = CHAR_TYPE_MONSTER;

	SetPosition(POS_STANDING);

	m_dwPlayStartTime = m_dwLastMoveTime = get_dword_time();

	GotoState(m_stateIdle);
	m_dwStateDuration = 1;

	m_dwLastAttackTime = get_dword_time() - 20000;
	m_dwLastAttackPVPTime = get_dword_time() - 20000;

	m_bAddChrState = 0;

	m_pkChrStone = NULL;

	m_pkSafebox = NULL;
	m_iSafeboxSize = -1;
	m_bSafeboxNeedPassword = false;
	m_iSafeboxLoadTime = 0;

	m_pkMall = NULL;
	m_iMallLoadTime = 0;

	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;
	m_lWarpMapIndex = 0;

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;

	m_pSkillLevels = NULL;

	m_dwMoveStartTime = 0;
	m_dwMoveDuration = 0;

	m_dwFlyTargetID = 0;

	m_dwNextStatePulse = 0;

	m_dwLastDeadTime = get_dword_time()-180000;

	m_bSkipSave = false;

	m_bItemLoaded = false;

	m_bHasPoisoned = false;

	m_pkDungeon = NULL;
	m_iEventAttr = 0;

	m_kAttackLog.dwVID = 0;
	m_kAttackLog.dwTime = 0;

	m_bNowWalking = m_bWalking = false;
	ResetChangeAttackPositionTime();

	m_bDetailLog = false;
	m_bMonsterLog = false;

	m_bDisableCooltime = false;

	m_iAlignment = 0;
	m_iRealAlignment = 0;

	m_iKillerModePulse = 0;
	m_bPKMode = PK_MODE_PEACE;
#ifdef ENABLE_BLOCK_PKMODE
	m_blockPKMode = false;
#endif

	m_dwQuestNPCVID = 0;
	m_dwQuestByVnum = 0;
	m_pQuestItem = NULL;

	m_dwUnderGuildWarInfoMessageTime = get_dword_time()-60000;

	m_bUnderRefine = false;

	// REFINE_NPC
	m_dwRefineNPCVID = 0;
	// END_OF_REFINE_NPC

	m_dwPolymorphRace = 0;

	m_bStaminaConsume = false;

	ResetChainLightningIndex();

	m_dwMountVnum = 0;
	m_chRider = NULL;

	m_pWarMap = NULL;
	m_pWeddingMap = NULL;
	m_bChatCounter = 0;

	ResetStopTime();

	m_dwLastVictimSetTime = get_dword_time() - 3000;
	m_iMaxAggro = -100;

	m_dwLoginPlayTime = 0;

	m_pkChrMarried = NULL;

	m_posSafeboxOpen.x = -1000;
	m_posSafeboxOpen.y = -1000;

	// EQUIP_LAST_SKILL_DELAY
	m_dwLastSkillTime = get_dword_time();
	m_dwLastSkillVnum = 0;
	// END_OF_EQUIP_LAST_SKILL_DELAY

	// MOB_SKILL_COOLTIME
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
	// END_OF_MOB_SKILL_COOLTIME

	// ARENA
	m_pArena = NULL;
	m_nPotionLimit = quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count");
	// END_ARENA

	//PREVENT_TRADE_WINDOW
	m_isOpenSafebox = 0;
	//END_PREVENT_TRADE_WINDOW
	
	//PREVENT_REFINE_HACK
	m_iRefineTime = 0;
	//END_PREVENT_REFINE_HACK
	
	//RESTRICT_USE_SEED_OR_MOONBOTTLE
	m_iSeedTime = 0;
	//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
	//PREVENT_PORTAL_AFTER_EXCHANGE
	m_iExchangeTime = 0;
	//END_PREVENT_PORTAL_AFTER_EXCHANGE
	//
	m_iSafeboxLoadTime = 0;

	m_iMyShopTime = 0;

	m_deposit_pulse = 0;

	SET_OVER_TIME(this, OT_NONE);

	m_strNewName = "";

	m_known_guild.clear();

	m_dwLogOffInterval = 0;

	m_bComboSequence = 0;
	m_dwLastComboTime = 0;
	m_bComboIndex = 0;
	m_iComboHackCount = 0;
	m_dwSkipComboAttackByTime = 0;

	m_dwMountTime = 0;

	m_dwLastGoldDropTime = 0;

	m_bIsLoadedAffect = false;
	cannot_dead = false;

#ifdef __PET_SYSTEM__
	m_petSystem = NULL;
	m_petOwnerActor = NULL;
	m_bIsPet = false;
#endif
#ifdef __PET_ADVANCED__
	m_petAdvanced = nullptr;
#endif

	m_fAttMul = 1.0f;
	m_fDamMul = 1.0f;

	memset(&m_tvLastSyncTime, 0, sizeof(m_tvLastSyncTime));
	m_iSyncHackCount = 0;

	m_bGMInvisible = false;
	m_bGMInvisibleChanged = false;

	m_bIsEXPDisabled = false;
	m_sPVPTeam = -1;

#ifdef __IPV6_FIX__
	m_bIPV6FixEnabled = false;
#endif

#ifdef __ACCE_COSTUME__
	m_AcceWindowType = 3;
	memset(&m_pointsInstant.pAcceSlots, WORD_MAX, sizeof(m_pointsInstant.pAcceSlots));
#endif

	m_wInventoryMaxNum = 0;
	m_wUppitemInventoryMaxNum = 0;
	m_wSkillbookInventoryMaxNum = 0;
	m_wStoneInventoryMaxNum = 0;
	m_wEnchantInventoryMaxNum = 0;

	m_pkMountSystem = NULL;
	m_bTempMountState = MOUNT_NONE;
	m_bHorseGrade = 0;
	m_dwHorseElapsedLifeTime = 0;
	m_pkHorseDeadEvent = 0;

#ifdef __VOTE4BUFF__
	m_bV4B_Loaded = false;
	m_dwV4B_TimeEnd = 0;
	m_bV4B_ApplyType = 0;
	m_iV4B_ApplyValue = 0;
#endif

#ifdef __FAKE_PC__
	m_pkFakePCAfkEvent = NULL;
	m_pkFakePCOwner = NULL;
	m_pkFakePCSpawnItem = NULL;
	m_fFakePCDamageFactor = 1.0f;
	m_bIsNoAttackFakePC = false;
#endif

#ifdef __DRAGONSOUL__
	m_pointsInstant.iDragonSoulActiveDeck = -1;
#endif

#ifdef AHMET_FISH_EVENT_SYSTEM
	memset(&m_fishSlots, 0, sizeof(m_fishSlots));
	m_dwFishUseCount = 0;
	m_bFishAttachedShape = 0;
#endif	

#ifdef __GAYA_SYSTEM__
	m_dwGaya = 0;
#endif

	m_iGoldActionTime = 0;

	memset(m_abPlayerDataChanged, 0, sizeof(m_abPlayerDataChanged));

#ifdef __SWITCHBOT__
	m_dwSwitchbotSpeed = 700;
	m_pkSwitchbotEvent = NULL;
#endif

#ifdef __FAKE_BUFF__
	memset(m_abFakeBuffSkillLevel, 0, sizeof(m_abFakeBuffSkillLevel));
	m_pkFakeBuffOwner = NULL;
	m_pkFakeBuffSpawn = NULL;
	m_pkFakeBuffItem = NULL;
#endif

#ifdef __ATTRTREE__
	memset(m_aAttrTree, 0, sizeof(m_aAttrTree));
#endif

	m_dwForceMonsterAttackRange = 0;
	m_bIsShowTeamler = false;
	m_bHaveToRemoveFromMgr = false;
	bDungeonComplete = 0;
	iItemDropQuest = 0;
	iLastItemDropQuest = 0;

	m_iUsedRenameGuildItem = -1;
#ifdef __COSTUME_BONUS_TRANSFER__
	m_bIsOpenedCostumeBonusTransferWindow = false;
	for (BYTE i = 0; i < CBT_SLOT_MAX; i++)
		m_pCostumeBonusTransferWindowItemCell[i] = NPOS;
#endif

#ifdef COMBAT_ZONE
	m_iCombatZonePoints = 0;
	m_iCombatZoneDeaths = 0;
	m_dwCombatZonePoints = 0;

	m_pkCombatZoneLeaveEvent = NULL;
	m_pkCombatZoneWarpEvent = NULL;

	m_bCombatZoneRank = 0;
#endif

	m_dwLastAttackedByPC = 0;

	memset(m_bShinings, 0, sizeof(m_bShinings));

#ifdef ENABLE_HYDRA_DUNGEON
	m_bLockTarget = false;
#endif

#ifdef ENABLE_RUNE_SYSTEM
	m_bRuneLoaded = false;
	m_runePage.set_main_group(-1);
	m_runePage.set_sub_group(-1);
	memset(&m_runeData, 0, sizeof(m_runeData));
	m_runeEvent = NULL;
	m_bRunePermaBonusSet = false;
#endif

	m_bKillcounterStatsChanged = false;

#ifdef ENABLE_XMAS_EVENT
	m_iXmasEventPulse = 0;
#endif

#ifdef ENABLE_WARP_BIND_RING
	m_iWarpBindPulse = 0;
#endif

#ifdef SORT_AND_STACK_ITEMS
	m_iSortingPulse = 0;
	m_iStackingPulse = 0;
#endif

#ifdef CHANGE_SKILL_COLOR
	memset(&m_dwSkillColor, 0, sizeof(m_dwSkillColor));
#endif

#ifdef __EQUIPMENT_CHANGER__
	m_bIsEquipmentChangerLoaded = false;
	m_dwEquipmentChangerPageIndex = 0;
	m_dwEquipmentChangerLastChange = 0;
#endif

#ifdef ENABLE_COMPANION_NAME
	m_petNameTimeLeft = 0;
	m_mountNameTimeLeft = 0;
	m_companionHasName = false;
#endif
	m_bIsHacker = false;
#ifdef KEEP_SKILL_AFFECTS
	m_isDestroyed = false;
#endif
#ifdef CHECK_TIME_AFTER_PVP
	m_dwLastAttackTimePVP = get_dword_time() - 20000;
	m_dwLastTimeAttackedPVP = get_dword_time() - 20000;
#endif
#ifdef BLACKJACK
	bBlackJackStatus = 0;
#endif
#ifdef DMG_RANKING
	m_dummyHitCount = 0;
	m_dummyHitStartTime = 0;
	m_totalDummyDamage = 0;
#endif

#ifdef __PRESTIGE__
	m_bPrestigeLevel = 0;
#endif

	m_BraveCapePulse = 0;

	memset(_dynamic_events_counter, 0, sizeof(_dynamic_events_counter));
	memset(_dynamic_events_executing, 0, sizeof(_dynamic_events_executing));

#ifdef AUCTION_SYSTEM
	m_ShopSignStyle = 0;
	m_ShopSignRed = 0.0f;
	m_ShopSignGreen = 0.0f;
	m_ShopSignBlue = 0.0f;

	_auction_shop_owner = 0;
#endif
}

void CHARACTER::Create(const char * c_pszName, DWORD vid, bool isPC)
{
	static int s_crc = 172814;

	char crc_string[128+1];
	snprintf(crc_string, sizeof(crc_string), "%s%p%d", c_pszName, this, ++s_crc);
	m_vid = VID(vid, GetCRC32(crc_string, strlen(crc_string)));

	if (isPC)
		m_stName = c_pszName;
}

void CHARACTER::Destroy()
{
	_exec_events(EEventTypes::DESTROY, true);

	if (m_bHaveToRemoveFromMgr)
	{
		sys_err("Character is not removed from CharManagerMap properly.");
		CHARACTER_MANAGER::instance().RemoveFromCharMap(this);
	}
#ifdef __FAKE_PC__
	if (FakePC_Check() && !FakePC_IsSupporter())
	{
		if (!IsDead() && GetDungeon())
			Dead(NULL, false);
	}
#endif
	if (m_bIsShowTeamler)
		SetIsShowTeamler(false);

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		FakeBuff_Destroy();
#endif

#ifdef ENABLE_RUNE_SYSTEM
	if (m_runeEvent)
		event_cancel(&m_runeEvent);
#endif

#ifdef ENABLE_HYDRA_DUNGEON
	CHydraDungeonManager::instance().OnDestroy(this);
#endif

	CloseMyShop();

#ifdef __PET_SYSTEM__

	// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
	if (m_petOwnerActor)
	{
		// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
		m_petOwnerActor->OnUnsummon();
		// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
		m_petOwnerActor = NULL;
		// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
	}

	// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
	if (m_petSystem)
	{
		// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
		M2_DELETE(m_petSystem);
		// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
		m_petSystem = NULL;
		// if (test_server)	sys_err("%s:%d",__FILE__,__LINE__);
	}
#endif

#ifdef __PET_ADVANCED__
	if (m_petAdvanced)
	{
		m_petAdvanced->OnDestroyPet();
		m_petAdvanced = nullptr;
	}
#endif

	// destroy fake pc
#ifdef __FAKE_PC__
	FakePC_Destroy();
#endif

	if (m_pkRegen)
	{
		if (m_pkDungeon) {
			// Dungeon regen may not be valid at this point
			if (m_pkDungeon->IsValidRegen(m_pkRegen, regen_id_)) {
				--m_pkRegen->count;
			}
		} else {
			// Is this really safe?
			--m_pkRegen->count;
		}
		m_pkRegen = NULL;
	}

	if (m_pkDungeon)
	{
		SetDungeon(NULL);
	}

	if (m_pkMountSystem)
	{
		if (m_pkMountSystem->IsRiding())
			m_pkMountSystem->StopRiding();
		if (m_pkMountSystem->IsSummoned())
			m_pkMountSystem->Unsummon();
	}

	if (GetRider() && m_bCharType == CHAR_TYPE_MOUNT)
	{
		if (CMountSystem* pkMountSystem = GetRider()->GetMountSystem())
			pkMountSystem->OnDestroySummoned();
	}

	if (GetDesc())
	{
		GetDesc()->BindCharacter(NULL);
//		BindDesc(NULL);
	}

	if (m_pkExchange)
		m_pkExchange->Cancel();

	SetVictim(NULL);

	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

	ClearStone();
	ClearSync();
	ClearTarget();

	if (NULL == m_pkMobData)
	{
#ifdef __DRAGONSOUL__
		DragonSoul_CleanUp();
#endif
		ClearItem();
	}

	ClearAuraBuffs();

	// <Factor> m_pkParty becomes NULL after CParty destructor call!
	LPPARTY party = m_pkParty;
	if (party)
	{
		if (party->GetLeaderPID() == GetVID() && !IsPC())
		{
			M2_DELETE(party);
		}
		else
		{
			party->Unlink(this); 

			if (!IsPC())
				party->Quit(GetVID());
		}

		SetParty(NULL); // ¾ÈÇØµµ µÇÁö¸¸ ¾ÈÀüÇÏ°Ô.
	}

	if (m_pkMobInst)
	{
		M2_DELETE(m_pkMobInst);
		m_pkMobInst = NULL;
	}

	m_pkMobData = NULL;

	if (m_pkSafebox)
	{
		M2_DELETE(m_pkSafebox);
		m_pkSafebox = NULL;
	}

	if (m_pkMall)
	{
		M2_DELETE(m_pkMall);
		m_pkMall = NULL;
	}

	m_set_pkChrSpawnedBy.clear();

	StopMuyeongEvent();
	event_cancel(&m_pkAuraUpdateEvent);
	event_cancel(&m_pkWarpNPCEvent);
	event_cancel(&m_pkRecoveryEvent);
	event_cancel(&m_pkDeadEvent);
	event_cancel(&m_pkSaveEvent);
	event_cancel(&m_pkTimedEvent);
	event_cancel(&m_pkChannelSwitchEvent);
	event_cancel(&m_pkStunEvent);
	event_cancel(&m_pkFishingEvent);
	event_cancel(&m_pkPoisonEvent);
	event_cancel(&m_pkFireEvent);
	event_cancel(&m_pkPartyRequestEvent);
	//DELAYED_WARP
	event_cancel(&m_pkWarpEvent);
	event_cancel(&m_pkCheckSpeedHackEvent);
	event_cancel(&m_PolymorphEvent);
	//END_DELAYED_WARP

	// RECALL_DELAY
	//event_cancel(&m_pkRecallEvent);
	// END_OF_RECALL_DELAY

	// MINING
	event_cancel(&m_pkMiningEvent);
	// END_OF_MINING

#ifdef __SWITCHBOT__
	event_cancel(&m_pkSwitchbotEvent);
#endif

#ifdef COMBAT_ZONE
	event_cancel(&m_pkCombatZoneLeaveEvent);
	event_cancel(&m_pkCombatZoneWarpEvent);
#endif

	for (itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.begin(); it != m_mapMobSkillEvent.end(); ++it)
	{
		LPEVENT pkEvent = it->second;
		event_cancel(&pkEvent);
	}
	m_mapMobSkillEvent.clear();

	//event_cancel(&m_pkAffectEvent);
	ClearAffect();

	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin();  it != m_map_buff_on_attrs.end(); it++)
	{
		if (NULL != it->second)
		{
			M2_DELETE(it->second);
		}
	}
	m_map_buff_on_attrs.clear();

	event_cancel(&m_pkDestroyWhenIdleEvent);

	if (m_pSkillLevels)
	{
		M2_DELETE_ARRAY(m_pSkillLevels);
		m_pSkillLevels = NULL;
	}

	CEntity::Destroy();

	if (GetSectree())
		GetSectree()->RemoveEntity(this);

	if (m_bMonsterLog)
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);

	for (auto it : m_list_pkAffectSave)
		CAffect::Release(it);

	if (quest::CQuestManager::instance().GetCurrentCharacterPtr() == this)
	{
		sys_err("<factor> quest char ptr is pointing to an almost deleted instance");
		quest::CQuestManager::instance().SetCurrentCharacterPtr(NULL);
	}
}

const char * CHARACTER::GetName(BYTE bLanguageID) const
{
	if (m_stName.empty())
	{
		if (m_pkMobData)
		{
			if (IsWarp() || IsGoto())
				return m_pkMobData->m_table.name().c_str();

			return m_pkMobData->m_table.locale_name(bLanguageID).c_str();
		}

		return "";
	}

	return m_stName.c_str();
	// return m_stName.empty() ? (m_pkMobData ? ((IsWarp() || IsGoto()) ? m_pkMobData->m_table.szName : m_pkMobData->m_table.szLocaleName[bLanguageID]) : "") : m_stName.c_str();
}

bool CHARACTER::CanShopNow() const
{
	if ((GetExchange() || IsOpenSafebox() || GetShop()) || IsCubeOpen())
		return false;

#ifdef __ACCE_COSTUME__
	if (IsAcceWindowOpen())
		return false;
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
	if (CBT_IsWindowOpen())
		return false;
#endif

	return true;
}

void CHARACTER::OpenMyShop(const char* c_pszSign, const ::google::protobuf::RepeatedPtrField<network::TShopItemTable>& table)
{
#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif

	if (!GM::check_allow(GetGMLevel(), GM_ALLOW_CREATE_PRIVATE_SHOP))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot do this with this gamemaster rank."));
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (GetDesc()->IsTradeblocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return;
	}
#endif

	if (GetPart(PART_MAIN) > 2)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°©¿ÊÀ» ¹þ¾î¾ß °³ÀÎ »óÁ¡À» ¿­ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}

	if (GetMyShop())	// ÀÌ¹Ì ¼¥ÀÌ ¿­·Á ÀÖÀ¸¸é ´Ý´Â´Ù.
	{
		CloseMyShop();
		return;
	}

	// ÁøÇàÁßÀÎ Äù½ºÆ®°¡ ÀÖÀ¸¸é »óÁ¡À» ¿­ ¼ö ¾ø´Ù.
	quest::PC * pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	// GetPCForce´Â NULLÀÏ ¼ö ¾øÀ¸¹Ç·Î µû·Î È®ÀÎÇÏÁö ¾ÊÀ½
	if (pPC->IsRunning())
		return;

	if (table.size() == 0)
		return;

	long long nTotalMoney = 0;

	for (auto& elem : table)
	{
		nTotalMoney += static_cast<long long>(elem.item().price());
	}

	nTotalMoney += GetGold();

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] Overflow (GOLD_MAX) id %u name %s", GetPlayerID(), GetName());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "20¾ï ³ÉÀ» ÃÊ°úÇÏ¿© »óÁ¡À» ¿­¼ö°¡ ¾ø½À´Ï´Ù"));
		return;
	}

	char szSign[SHOP_SIGN_MAX_LEN+1];
	strlcpy(szSign, c_pszSign, sizeof(szSign));

	m_stShopSign = szSign;

	if (m_stShopSign.length() == 0)
		return;

	// MYSHOP_PRICE_LIST
	std::map<DWORD, DWORD> itemkind;  // ¾ÆÀÌÅÛ Á¾·ùº° °¡°Ý, first: vnum, second: ´ÜÀÏ ¼ö·® °¡°Ý
	// END_OF_MYSHOP_PRICE_LIST	

	std::set<TItemPos> cont;
	for (auto& elem : table)
	{
		if (cont.find(elem.item().cell()) != cont.end())
		{
			sys_err("MYSHOP: duplicate shop item detected! (name: %s)", GetName());
			return;
		}

		// ANTI_GIVE, ANTI_MYSHOP check
		LPITEM pkItem = GetItem(elem.item().cell());

		if (pkItem)
		{
			auto item_table = pkItem->GetProto();

			if (!pkItem->CanPutItemIntoShop())
				return;

			// MYSHOP_PRICE_LIST
			itemkind[pkItem->GetVnum()] = elem.item().price() / pkItem->GetCount();
			// END_OF_MYSHOP_PRICE_LIST
		}

		cont.insert(elem.item().cell());
	}

	// MYSHOP_PRICE_LIST
	// º¸µû¸® °³¼ö¸¦ °¨¼Ò½ÃÅ²´Ù. 
	if (CountSpecifyItem(71049)) { // ºñ´Ü º¸µû¸®´Â ¾ø¾ÖÁö ¾Ê°í °¡°ÝÁ¤º¸¸¦ ÀúÀåÇÑ´Ù.

		//
		// ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸¸¦ ÀúÀåÇÏ±â À§ÇØ ¾ÆÀÌÅÛ °¡°ÝÁ¤º¸ ÆÐÅ¶À» ¸¸µé¾î DB Ä³½Ã¿¡ º¸³½´Ù.
		//
		network::GDOutputPacket<network::GDMyShopPricelistUpdatePacket> pack;
		pack->set_owner_id(GetPlayerID());

		size_t idx = 0;
		for (auto it = itemkind.begin(); it != itemkind.end(); ++it)
		{
			auto price_info = pack->add_price_info();
			price_info->set_vnum(it->first);
			price_info->set_price(it->second);
			idx++;
		}

		db_clientdesc->DBPacket(pack);
	} 
	// END_OF_MYSHOP_PRICE_LIST
	else if (CountSpecifyItem(50200))
		RemoveSpecifyItem(50200, 1);
	else
		return; // º¸µû¸®°¡ ¾øÀ¸¸é Áß´Ü.

	if (m_pkExchange)
		m_pkExchange->Cancel();

	network::GCOutputPacket<network::GCShopSignPacket> p;

	p->set_vid(GetVID());
	p->set_sign(c_pszSign);

	PacketAround(p);

	m_pkMyShop = CShopManager::instance().CreatePCShop(this, table);

	if (IsPolymorphed() == true)
	{
		RemoveAffect(AFFECT_POLYMORPH);
	}

	if (GetMountSystem() && GetMountSystem()->IsSummoned())
	{
		GetMountSystem()->Unsummon();
	}
	else if (GetMountVnum())
	{
		if (GetMountSystem() && GetMountSystem()->IsRiding())
		{
			GetMountSystem()->StopRiding();
			GetMountSystem()->Unsummon();
		}
		else
		{
			RemoveAffect(AFFECT_MOUNT);
		}
	}
	
	SetPolymorph(30000, true);
}

void CHARACTER::CloseMyShop()
{
	if (GetMyShop())
	{
		m_stShopSign.clear();
		CShopManager::instance().DestroyPCShop(this);
		m_pkMyShop = NULL;

		network::GCOutputPacket<network::GCShopSignPacket> p;
		p->set_vid(GetVID());
		PacketAround(p);
		
		SetPolymorph(GetJob(), true);
	}
}

void EncodeMovePacket(network::GCOutputPacket<network::GCMovePacket>& pack, DWORD dwVID, BYTE bFunc, UINT uArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, BYTE bRot)
{
	pack->set_func(bFunc);
	pack->set_arg(uArg);
	pack->set_vid(dwVID);
	pack->set_time(dwTime ? dwTime : get_dword_time());
	pack->set_rot(bRot);
	pack->set_x(x);
	pack->set_y(y);
	pack->set_duration(dwDuration);
}

void CHARACTER::RestartAtPos(long lX, long lY)
{
	if (m_bIsObserver)
		return;

	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(GetMapIndex(), lX, lY);
	if (!sectree)
	{
		sys_log(0, "cannot find sectree by %dx%d mapindex %d (pid %u race %u)", lX, lY, GetMapIndex(), GetPlayerID(), GetRaceNum());
		return;
	}

	bool bChangeTree = false;

	if (!GetSectree() || GetSectree() != sectree)
		bChangeTree = true;

	if (bChangeTree)
	{
		if (GetSectree())
			GetSectree()->RemoveEntity(this);
	}
	ViewCleanup();

	if (GetStamina() < GetMaxStamina())
		StartAffectEvent();
	
	SetXYZ(lX, lY, 0);

	m_posDest.x = lX;
	m_posDest.y = lY;
	m_posDest.z = 0;

	m_posStart.x = lX;
	m_posStart.y = lY;
	m_posStart.z = 0;

	EncodeInsertPacket(this);
	if (bChangeTree)
	{
		sectree->InsertEntity(this);
		UpdateSectree();
	}
	else
	{
		ViewReencode();
		sys_log(!test_server, "RestartAtPos in same sectree");
	}
}

void CHARACTER::RestartAtSamePos()
{
	if (m_bIsObserver)
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	if (IsGMInvisible())
		return;

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		EncodeInsertPacket(entity);

		if( entity->IsType(ENTITY_CHARACTER) )
		{
			LPCHARACTER lpChar = (LPCHARACTER)entity;
			if( lpChar->IsPC() || lpChar->IsNPC() || lpChar->IsMonster() )
			{
				if (!entity->IsObserverMode())
					entity->EncodeInsertPacket(this);
			}
		}
		else
		{
			if( !entity->IsObserverMode())
			{
				entity->EncodeInsertPacket(this);
			}
		}
	}
}


// Entity¿¡ ³»°¡ ³ªÅ¸³µ´Ù°í ÆÐÅ¶À» º¸³½´Ù.
void CHARACTER::EncodeInsertPacket(LPENTITY entity)
{
	if ((IsGMInvisible() || (!IsPC() && GetRider() && GetRider()->IsGMInvisible())) && entity != this && (IsPC() || entity != GetRider()))
	{
		if (test_server)
			sys_log(0, "CHARACTER::EncodeInsertPacket: GMInvisible stopped.");
		return;
	}

	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	if (entity->IsNoPVPPacketMode() && !m_bNoPacketPVP)
		return;

	// ±æµåÀÌ¸§ ¹ö±× ¼öÁ¤ ÄÚµå
	LPCHARACTER ch = (LPCHARACTER) entity;
	
	ch->SendGuildName(GetGuild());
	// ±æµåÀÌ¸§ ¹ö±× ¼öÁ¤ ÄÚµå
	
	network::GCOutputPacket<network::GCCharacterAddPacket> pack;

	pack->set_vid(m_vid);
	pack->set_type(GetCharType());
	pack->set_angle(BYTE(GetRotation() / 5.0f));
	pack->set_x(GetX());
	pack->set_y(GetY());
	pack->set_race_num(GetRaceNum());
#ifdef __PET_SYSTEM__
	pack->set_moving_speed(IsPet() ? 150 : GetLimitPoint(POINT_MOV_SPEED));
#else
	pack->set_moving_speed(GetLimitPoint(POINT_MOV_SPEED));
#endif
	pack->set_attack_speed(GetLimitPoint(POINT_ATT_SPEED));
	pack->add_affect_flags(m_afAffectFlag.bits[0]);
	pack->add_affect_flags(m_afAffectFlag.bits[1]);

#ifdef AUCTION_SYSTEM
	if (ch->GetPlayerID() == get_auction_shop_owner())
	{
		auto tmpFlag = m_afAffectFlag;
		tmpFlag.Set(AFFECT_AUCTION_SHOP_OWNER);

		pack->set_affect_flags(0, tmpFlag.bits[0]);
		pack->set_affect_flags(1, tmpFlag.bits[1]);
	}
#endif

	pack->set_state_flag(m_bAddChrState);

	// PLAYER NPC
#ifdef __FAKE_PC__
	if (FakePC_Check())
	{
		pack->set_type(CHAR_TYPE_PC);
		pack->set_race_num(FakePC_GetOwner()->GetRaceNum());
	}
#endif

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
	{
		pack->set_type(CHAR_TYPE_FAKEBUFF);
		if (GetMobTable().vnum() == MAIN_RACE_MOB_SHAMAN_W)
			pack->set_race_num(MAIN_RACE_SHAMAN_W);
		else if (GetMobTable().vnum() == MAIN_RACE_MOB_SHAMAN_M)
			pack->set_race_num(MAIN_RACE_SHAMAN_M);
	}
#endif

	int iDur = 0;

	if (m_posDest.x != pack->x() || m_posDest.y != pack->y())
	{
		iDur = (m_dwMoveStartTime + m_dwMoveDuration) - get_dword_time();

		if (iDur <= 0)
		{
			pack->set_x(m_posDest.x);
			pack->set_y(m_posDest.y);
		}
	}

	d->Packet(pack);

	bool bIsPC = IsPC();
#ifdef __FAKE_PC__
	bIsPC = bIsPC || FakePC_Check();
#endif
#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		bIsPC = true;
#endif

	if (bIsPC || m_bCharType == CHAR_TYPE_NPC || m_bCharType == CHAR_TYPE_MOUNT || m_bCharType == CHAR_TYPE_PET)
	{
		network::GCOutputPacket<network::GCCharacterAdditionalInfoPacket> addPacket;
		for (int i = 0; i < CHR_EQUIPPART_NUM; ++i)
			addPacket->add_parts(0);

		addPacket->set_vid(m_vid);
		
		addPacket->set_parts(CHR_EQUIPPART_WEAPON, IsCostumeHide(HIDE_COSTUME_WEAPON) ? (GetWear(WEAR_WEAPON) ? GetWear(WEAR_WEAPON)->GetDisplayVnum() : 0) : GetPart(PART_WEAPON));
		addPacket->set_parts(CHR_EQUIPPART_ARMOR, IsCostumeHide(HIDE_COSTUME_ARMOR) ? (GetWear(WEAR_BODY) ? GetWear(WEAR_BODY)->GetDisplayVnum() : 0) : GetPart(PART_MAIN));
		addPacket->set_parts(CHR_EQUIPPART_HAIR, IsCostumeHide(HIDE_COSTUME_HAIR) ? GetCurrentHair() : GetPart(PART_HAIR));
		addPacket->set_parts(CHR_EQUIPPART_HEAD, GetPart(PART_HEAD));
	
#ifdef __ACCE_COSTUME__
		addPacket->set_parts(CHR_EQUIPPART_ACCE, IsCostumeHide(HIDE_COSTUME_ACCE) ? (GetWear(WEAR_ACCE) ? GetWear(WEAR_ACCE)->GetDisplayVnum() : 0) : GetPart(PART_ACCE));
#endif

#ifdef __ALPHA_EQUIP__
		if (LPITEM pWeapon = GetWear(WEAR_WEAPON))
		{
			if (pWeapon->GetDisplayVnum() == GetPart(PART_WEAPON))
				addPacket->set_weapon_alpha_value(pWeapon->GetRealAlphaEquipValue());
		}
#endif

		addPacket->set_pk_mode(m_bPKMode);
		addPacket->set_mount_vnum(GetMountVnum());
		addPacket->set_empire(m_bEmpire);
		addPacket->set_pvp_team(m_sPVPTeam);
		addPacket->set_level(IsPC() ? GetLevel() : 0);

		std::string chName = GetName(ch->GetLanguageID());

#ifdef __PET_ADVANCED__
		if (GetPetAdvanced())
		{
			if (auto evolveData = GetPetAdvanced()->GetEvolveData())
				addPacket->set_mob_scale(evolveData->GetScale());
		}
#endif
#ifdef __PET_SYSTEM__
		if (IsPet())
		{
#ifdef ENABLE_COMPANION_NAME
			if (!m_companionHasName)
				chName += LC_TEXT(ch, "'s Pet");
#else
			chName += LC_TEXT(ch, "'s Pet");
#endif
			addPacket->set_name(chName.c_str());	
		}
		else 
#endif
		if (IsMount())
		{
#ifdef ENABLE_COMPANION_NAME
			if (!m_companionHasName)
				chName += LC_TEXT(ch, "'s Mount");
#else
			chName += LC_TEXT(ch, "'s Mount");
#endif
			addPacket->set_name(chName.c_str());		
		}
		else
			addPacket->set_name(GetName(d->GetAccountTable().language()));

		addPacket->set_guild_id(GetGuild() ? GetGuild()->GetID() : 0);
		addPacket->set_alignment(m_iAlignment / 10);
#ifdef COMBAT_ZONE
		addPacket->set_combat_zone_rank(m_bCombatZoneRank);
#endif

#ifdef CHANGE_SKILL_COLOR
		for (auto& elem : m_dwSkillColor)
		{
			for (DWORD color : elem)
				addPacket->add_skill_colors(color);
		}
#endif

#ifdef __PRESTIGE__
		addPacket->set_prestige_level(Prestige_GetLevel());
#endif

		d->Packet(addPacket);
	}

#ifdef AUCTION_SYSTEM
	if (ch->GetPlayerID() == get_auction_shop_owner())
	{
		network::GCOutputPacket<network::GCUpdateCharacterScalePacket> auction_scaler_pack;
		auction_scaler_pack->set_vid(GetVID());
		auction_scaler_pack->set_scale(AuctionManager::SHOP_OWN_CHAR_SCALING);
		d->Packet(auction_scaler_pack);
	}
#endif

	if (iDur)
	{
		network::GCOutputPacket<network::GCMovePacket> pack2;
		EncodeMovePacket(pack2, GetVID(), FUNC_MOVE, 0, m_posDest.x, m_posDest.y, iDur, 0, (BYTE) (GetRotation() / 5));
		d->Packet(pack2);

		network::GCOutputPacket<network::GCWalkModePacket> p;
		p->set_vid(GetVID());
		p->set_mode(m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN);

		d->Packet(p);
	}

	if (entity->IsType(ENTITY_CHARACTER) && GetDesc())
	{
		LPCHARACTER ch = (LPCHARACTER) entity;
		if (ch->IsWalking())
		{
			network::GCOutputPacket<network::GCWalkModePacket> p;
			p->set_vid(ch->GetVID());
			p->set_mode(ch->m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN);
			GetDesc()->Packet(p);
		}
	}

	if (!m_stShopSign.empty())
	{
		network::GCOutputPacket<network::GCShopSignPacket> p;
		p->set_vid(GetVID());
		p->set_sign(m_stShopSign.c_str());
#ifdef AUCTION_SYSTEM
		p->set_red(m_ShopSignRed);
		p->set_green(m_ShopSignGreen);
		p->set_blue(m_ShopSignBlue);
		p->set_style(m_ShopSignStyle);
#endif

		d->Packet(p);
	}

	if (entity->IsType(ENTITY_CHARACTER))
		sys_log(3, "EntityInsert %s (RaceNum %d) (%d %d) TO %s", GetName(), GetRaceNum(), GetX() / SECTREE_SIZE, GetY() / SECTREE_SIZE, ((LPCHARACTER)entity)->GetName());
}

void CHARACTER::EncodeRemovePacket(LPENTITY entity)
{
	if (entity->GetType() != ENTITY_CHARACTER)
		return;

	LPDESC d;

	if (!(d = entity->GetDesc()))
		return;

	network::GCOutputPacket<network::GCCharacterDeletePacket> pack;

	pack->set_vid(m_vid);

	d->Packet(pack);

	if (entity->IsType(ENTITY_CHARACTER))
		sys_log(3, "EntityRemove %s(%d) FROM %s", GetName(), (DWORD) m_vid, ((LPCHARACTER) entity)->GetName());
}

void CHARACTER::UpdatePacket()
{
	if (GetSectree() == NULL)
		return;

	if (IsPC() && (!GetDesc() || !GetDesc()->GetCharacter()))
		return;

	network::GCOutputPacket<network::GCCharacterUpdatePacket> pack;

	pack->set_vid(m_vid);
	
	// pack->adw_part()[CHR_EQUIPPART_ARMOR] = GetPart(PART_MAIN);
	// pack->adw_part()[CHR_EQUIPPART_WEAPON] = GetPart(PART_WEAPON);
	// pack->adw_part()[CHR_EQUIPPART_HEAD] = GetPart(PART_HEAD);
	// pack->adw_part()[CHR_EQUIPPART_HAIR] = GetPart(PART_HAIR);
	
	for (int i = 0; i < CHR_EQUIPPART_NUM; ++i)
		pack->add_parts(0);

	pack->set_parts(CHR_EQUIPPART_WEAPON, IsCostumeHide(HIDE_COSTUME_WEAPON) ? (GetWear(WEAR_WEAPON) ? GetWear(WEAR_WEAPON)->GetDisplayVnum() : 0) : GetPart(PART_WEAPON));
	pack->set_parts(CHR_EQUIPPART_ARMOR, IsCostumeHide(HIDE_COSTUME_ARMOR) ? (GetWear(WEAR_BODY) ? GetWear(WEAR_BODY)->GetDisplayVnum() : 0) : GetPart(PART_MAIN));
	pack->set_parts(CHR_EQUIPPART_HAIR, IsCostumeHide(HIDE_COSTUME_HAIR) ? GetCurrentHair() : GetPart(PART_HAIR));
	pack->set_parts(CHR_EQUIPPART_HEAD, GetPart(PART_HEAD));
	
#ifdef __ACCE_COSTUME__
	pack->set_parts(CHR_EQUIPPART_ACCE, IsCostumeHide(HIDE_COSTUME_ACCE) ? (GetWear(WEAR_ACCE) ? GetWear(WEAR_ACCE)->GetDisplayVnum() : 0) : GetPart(PART_ACCE));
#endif

#ifdef __ALPHA_EQUIP__
	if (LPITEM pWeapon = GetWear(WEAR_WEAPON))
	{
		if (pWeapon->GetDisplayVnum() == GetPart(PART_WEAPON))
			pack->set_weapon_alpha_value(pWeapon->GetRealAlphaEquipValue());
	}
#endif

	pack->set_moving_speed(GetLimitPoint(POINT_MOV_SPEED));
	pack->set_attack_speed(GetLimitPoint(POINT_ATT_SPEED));
	pack->set_state_flag(m_bAddChrState);
	pack->add_affect_flags(m_afAffectFlag.bits[0]);
	pack->add_affect_flags(m_afAffectFlag.bits[1]);
	pack->set_guild_id(0);
	pack->set_alignment(m_iAlignment / 10);
	pack->set_pk_mode(m_bPKMode);
#ifdef COMBAT_ZONE
	pack->set_combat_zone_points(GetCombatZonePoints());
#endif

#ifdef CHANGE_SKILL_COLOR
	for (auto& elem : m_dwSkillColor)
	{
		for (DWORD color : elem)
			pack->add_skill_colors(color);
	}
#endif

#ifdef __PRESTIGE__
	pack->set_prestige_level(Prestige_GetLevel());
#endif

	if (GetGuild())
		pack->set_guild_id(GetGuild()->GetID());

	pack->set_mount_vnum(GetMountVnum());

	PacketAround(pack);
	
	network::GCOutputPacket<network::GCCharacterShiningPacket> shiningPacket;
	shiningPacket->set_vid(m_vid);

	bool hasShining = false;
	for (BYTE i = 0; i < SHINING_MAX_NUM; ++i)
	{
		if (m_bShinings[i] != 0)
			hasShining = true;
		shiningPacket->add_shinings(m_bShinings[i]);
	}

	if (hasShining)
		PacketAround(shiningPacket);
}

LPCHARACTER CHARACTER::FindCharacterInView(const char * c_pszName, bool bFindPCOnly)
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	for (; it != m_map_view.end(); ++it)
	{
		if (!it->first->IsType(ENTITY_CHARACTER))
			continue;

		LPCHARACTER tch = (LPCHARACTER) it->first;

		if (bFindPCOnly && tch->IsNPC())
			continue;

		if (!strcasecmp(tch->GetName(), c_pszName))
			return (tch);
	}

	return NULL;
}

void CHARACTER::SetPosition(int pos)
{
	if (pos == POS_STANDING)
	{
		REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_STUN);

		event_cancel(&m_pkDeadEvent);
		event_cancel(&m_pkStunEvent);
	}
	else if (pos == POS_DEAD)
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_DEAD);

	if (!IsStone())
	{
		switch (pos)
		{
			case POS_FIGHTING:
				if (!IsState(m_stateBattle))
					MonsterLog("[BATTLE] ½Î¿ì´Â »óÅÂ");

				GotoState(m_stateBattle);
				break;

			default:
				if (!IsState(m_stateIdle))
					MonsterLog("[IDLE] ½¬´Â »óÅÂ");

				GotoState(m_stateIdle);
				break;
		}
	}

	m_pointsInstant.position = pos;
}

void CHARACTER::Save()
{
	if (!m_bSkipSave)
		CHARACTER_MANAGER::instance().DelayedSave(this);
}

void CHARACTER::CreatePlayerProto(TPlayerTable & tab)
{
	if (GetNewName().empty())
	{
		tab.set_name(GetName());
	}
	else
	{
		tab.set_name(GetNewName());
	}

	tab.set_ip(GetDesc()->GetHostName());

	tab.set_id(m_dwPlayerID);
	tab.set_voice(GetPoint(POINT_VOICE));
	tab.set_level(GetLevel());
#ifdef __PRESTIGE__
	tab.prestige_level = GetPrestigeLevel();
#endif
	tab.set_level_step(GetPoint(POINT_LEVEL_STEP));
	tab.set_exp(GetExp());
	tab.set_gold(GetGold());
#ifdef ENABLE_ZODIAC_TEMPLE
	tab.set_animasphere(GetAnimasphere());
#endif
#ifdef __GAYA_SYSTEM__
	tab.set_gaya(GetGaya());
#endif
	tab.set_job(m_points.job);
	tab.set_part_base(m_pointsInstant.bBasePart);
	tab.set_skill_group(m_points.skill_group);

#ifdef __HAIR_SELECTOR__
	tab.set_part_hair_base(m_dwSelectedHairPart);
#endif

	DWORD dwPlayedTime = (get_dword_time() - m_dwPlayStartTime);

	if (dwPlayedTime > 60000)
	{
		if (GetSectree() && !GetSectree()->IsAttr(GetX(), GetY(), ATTR_BANPK))
		{
			if (GetRealAlignment() < 0)
			{
				if (IsEquipUniqueItem(UNIQUE_ITEM_FASTER_ALIGNMENT_UP_BY_TIME))
					UpdateAlignment(120 * (dwPlayedTime / 60000));
				else
					UpdateAlignment(60 * (dwPlayedTime / 60000));
			}
			else
				UpdateAlignment(5 * (dwPlayedTime / 60000));
		}

		SetRealPoint(POINT_PLAYTIME, GetRealPoint(POINT_PLAYTIME) + dwPlayedTime / 60000);
		ResetPlayTime(dwPlayedTime % 60000);
	}

	tab.set_playtime(GetRealPoint(POINT_PLAYTIME));
	tab.set_alignment(m_iRealAlignment);

	if (m_posWarp.x != 0 || m_posWarp.y != 0)
	{
		tab.set_x(m_posWarp.x);
		tab.set_y(m_posWarp.y);
		tab.set_map_index(m_lWarpMapIndex);
	}
	else
	{
		tab.set_x(GetX());
		tab.set_y(GetY());
		tab.set_map_index(GetMapIndex());
	}

	if (m_lExitMapIndex == 0)
	{
		tab.set_exit_map_index(tab.map_index());
		tab.set_exit_x(tab.x());
		tab.set_exit_y(tab.y());
	}
	else
	{
		tab.set_exit_map_index(m_lExitMapIndex);
		tab.set_exit_x(m_posExit.x);
		tab.set_exit_y(m_posExit.y);
	}

	sys_log(0, "SAVE: %s %dx%d", GetName(), tab.x(), tab.y());

	tab.set_st(GetRealPoint(POINT_ST));
	tab.set_ht(GetRealPoint(POINT_HT));
	tab.set_dx(GetRealPoint(POINT_DX));
	tab.set_iq(GetRealPoint(POINT_IQ));

	tab.set_stat_point(GetPoint(POINT_STAT));
	tab.set_skill_point(GetPoint(POINT_SKILL));
	tab.set_sub_skill_point(GetPoint(POINT_SUB_SKILL));
	tab.set_horse_skill_point(GetPoint(POINT_HORSE_SKILL));

	tab.set_stat_reset_count(GetPoint(POINT_STAT_RESET_COUNT));

	tab.set_hp(GetHP());
	tab.set_sp(GetSP());

	for (int i = 0; i < QUICKSLOT_MAX_NUM; ++i)
		*tab.add_quickslots() = m_quickslot[i];

	for (int i = 0; i < PART_MAX_NUM; ++i)
		tab.add_parts(m_pointsInstant.parts[i]);

	// REMOVE_REAL_SKILL_LEVLES
	for (int i = 0; i < SKILL_MAX_NUM; ++i)
		*tab.add_skills() = m_pSkillLevels[i];
	// END_OF_REMOVE_REAL_SKILL_LEVLES

	tab.set_mount_state(m_pkMountSystem->GetState());
	if (strcmp("", m_pkMountSystem->GetName()) != 0)
		tab.set_mount_name(m_pkMountSystem->GetName());
	tab.set_mount_item_id(m_pkMountSystem->GetSummonItemID());
	tab.set_horse_grade(GetHorseGrade());
	tab.set_horse_elapsed_time(GetHorseElapsedTime());

	tab.set_inventory_max_num(GetInventoryMaxNum());
	tab.set_uppitem_inv_max_num(GetUppitemInventoryMaxNum());
	tab.set_skillbook_inv_max_num(GetSkillbookInventoryMaxNum());
	tab.set_stone_inv_max_num(GetStoneInventoryMaxNum());
	tab.set_enchant_inv_max_num(GetEnchantInventoryMaxNum());

	for (bool changed : m_abPlayerDataChanged)
		tab.add_data_changed(changed);
	tab.set_last_play_time(get_global_time());

#ifdef __FAKE_BUFF__
	tab.set_fakebuff_skill1(m_abFakeBuffSkillLevel[0]);
	tab.set_fakebuff_skill2(m_abFakeBuffSkillLevel[1]);
	tab.set_fakebuff_skill3(m_abFakeBuffSkillLevel[2]);
#endif

#ifdef __ATTRTREE__
	for (auto& row : m_aAttrTree)
	{
		for (BYTE col : row)
			tab.add_attrtree(col);
	}
#endif

#ifdef ENABLE_RUNE_SYSTEM
	*tab.mutable_rune_page_data() = m_runePage;
#endif

#ifdef __EQUIPMENT_CHANGER__
	tab.set_equipment_page_index(m_dwEquipmentChangerPageIndex);
#endif

#ifdef COMBAT_ZONE
	tab.set_combat_zone_points(m_dwCombatZonePoints);
#endif

#ifdef __PRESTIGE__
	tab.set_prestige(m_bPrestigeLevel);
#endif
}

void CHARACTER::SaveReal()
{
	if (m_bSkipSave)
		return;

	if (!GetDesc())
	{
		sys_err("Character::Save : no descriptor when saving (name: %s)", GetName());
		return;
	}

	network::GDOutputPacket<network::GDPlayerSavePacket> save_packet;
	CreatePlayerProto(*save_packet->mutable_data());

	db_clientdesc->DBPacket(save_packet, GetDesc()->GetHandle());

#ifdef ENABLE_RUNE_SYSTEM
	if (m_bRuneLoaded)
	{
		network::GDOutputPacket<network::GDPlayerRuneSavePacket> rune_save_packet;
		rune_save_packet->set_player_id(GetPlayerID());
		for (DWORD rune : m_setRuneOwned)
			rune_save_packet->add_runes(rune);

		db_clientdesc->DBPacket(rune_save_packet, GetDesc()->GetHandle());
	}
#endif

	quest::PC * pkQuestPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());

	if (!pkQuestPC)
		sys_err("CHARACTER::Save : null quest::PC pointer! (name %s)", GetName());
	else
	{
		pkQuestPC->Save(this);
	}

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
	if (pMarriage)
		pMarriage->Save();

#ifdef __EQUIPMENT_CHANGER__
	SaveEquipmentChanger();
#endif
}

void CHARACTER::FlushDelayedSaveItem()
{
	// ÀúÀå ¾ÈµÈ ¼ÒÁöÇ°À» ÀüºÎ ÀúÀå½ÃÅ²´Ù.
	LPITEM item;

	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
		if ((item = GetInventoryItem(i)))
			ITEM_MANAGER::instance().FlushDelayedSave(item);
}

void CHARACTER::Disconnect(const char * c_pszReason)
{
	assert(GetDesc() != NULL);

	sys_log(0, "DISCONNECT: %s (%s)", GetName(), c_pszReason ? c_pszReason : "unset" );

	LogManager::instance().ConnectLog(false, this);

#ifdef ENABLE_RUNE_PAGES
	SaveSelectedRunes();
#endif

#ifdef ENABLE_COMPANION_NAME
	SaveCompanionNames();
#endif

#ifdef __FAKE_PC__
	FakePC_Owner_DespawnAll();
#endif

#ifdef __EVENT_MANAGER__
	CEventManager::instance().OnPlayerLogout(this);
#endif

#ifdef COMBAT_ZONE
	CCombatZoneManager::instance().OnLogout(this);
#endif

	if (GetShop())
	{
		GetShop()->RemoveGuest(this);
		SetShop(NULL);
	}

	if (GetArena() != NULL)
	{
		GetArena()->OnDisconnect(GetPlayerID());
	}

	if (GetParty() != NULL)
	{
		GetParty()->UpdateOfflineState(GetPlayerID());
	}

#ifdef __GUILD_SAFEBOX__
	if (GetGuild())
	{
		CGuildSafeBox& rkSafeBox = GetGuild()->GetSafeBox();
		rkSafeBox.CloseSafebox(this);
	}
#endif

#ifdef __PET_ADVANCED__
	if (m_petAdvanced)
	{
		auto petItem = m_petAdvanced->GetItem();
		petItem->Save();
		ITEM_MANAGER::Instance().FlushDelayedSave(petItem);
		petItem->SetSkipSave(true);

		m_petAdvanced->Unsummon();
		m_petAdvanced = nullptr;
	}
#endif

	marriage::CManager::instance().Logout(this);

	SaveOfflineMessages();

	// P2P Logout
	network::GGOutputPacket<network::GGLogoutPacket> p;
	p->set_pid(GetPlayerID());
	p->set_name(GetName());
	P2P_MANAGER::instance().Send(p);
	char buf[51];
	snprintf(buf, sizeof(buf), "%s [%lld yang] %d %ld %d", 
		inet_ntoa(GetDesc()->GetAddr().sin_addr), GetGold(), g_bChannel, GetMapIndex(), GetAlignment());

	LogManager::instance().CharLog(this, 0, "LOGOUT", buf);

#ifdef __MELEY_LAIR_DUNGEON__
	if (MeleyLair::CMgr::instance().IsMeleyMap(GetMapIndex()))
		MeleyLair::CMgr::instance().Leave(GetGuild(), this, false);
#endif

	if (m_pWarMap)
		SetWarMap(NULL);

	if (m_pWeddingMap)
	{
		SetWeddingMap(NULL);
	}

	if (GetGuild())
		GetGuild()->LogoutMember(this);

	quest::CQuestManager::instance().LogoutPC(this);

	if (GetParty())
		GetParty()->Unlink(this);

	// Á×¾úÀ» ¶§ Á¢¼Ó²÷À¸¸é °æÇèÄ¡ ÁÙ°Ô ÇÏ±â
	if (IsStun() || IsDead())
	{
		DeathPenalty(0);
		PointChange(POINT_HP, 50 - GetHP());
	}


	if (!CHARACTER_MANAGER::instance().FlushDelayedSave(this))
	{
		SaveReal();
	}

	FlushDelayedSaveItem();

	SaveAffect();
	m_bIsLoadedAffect = false;

	m_bSkipSave = true; // ÀÌ ÀÌÈÄ¿¡´Â ´õÀÌ»ó ÀúÀåÇÏ¸é ¾ÈµÈ´Ù.

	quest::CQuestManager::instance().DisconnectPC(this);

	CloseSafebox();

	CloseMall();

#ifdef __COSTUME_BONUS_TRANSFER__
	if (CBT_IsWindowOpen())
		CBT_WindowClose();
#endif
	CPVPManager::instance().Disconnect(this);

	CTargetManager::instance().Logout(GetPlayerID());

	MessengerManager::instance().Logout(GetName());

	if (GetDesc())
	{
		network::GCOutputPacket<network::GCPointChangePacket> pack;
		pack->set_vid(m_vid);
		pack->set_type(POINT_PLAYTIME);
		pack->set_value(GetRealPoint(POINT_PLAYTIME) + (get_dword_time() - m_dwPlayStartTime) / 60000);
		GetDesc()->Packet(pack);
		GetDesc()->BindCharacter(NULL);
//		BindDesc(NULL);
	}

	M2_DESTROY_CHARACTER(this);
}

bool CHARACTER::Show(long lMapIndex, long x, long y, long z, bool bShowSpawnMotion/* = false */)
{
	LPSECTREE sectree = SECTREE_MANAGER::instance().Get(lMapIndex, x, y);

	if (!sectree)
	{
		sys_log(0, "cannot find sectree by %dx%d mapindex %d (pid %u race %u)", x, y, lMapIndex, GetPlayerID(), GetRaceNum());
		return false;
	}

	SetMapIndex(lMapIndex);

	bool bChangeTree = false;

	if (!GetSectree() || GetSectree() != sectree)
		bChangeTree = true;

	if (bChangeTree)
	{
		if (GetSectree())
			GetSectree()->RemoveEntity(this);

		ViewCleanup();
	}

	if (!IsNPC())
	{
		sys_log(0, "SHOW: %s %dx%dx%d", GetName(), x, y, z);
		if (GetStamina() < GetMaxStamina())
			StartAffectEvent();
	}
	else if (m_pkMobData)
	{
		m_pkMobInst->m_posLastAttacked.x = x;
		m_pkMobInst->m_posLastAttacked.y = y;
		m_pkMobInst->m_posLastAttacked.z = z;
	}

	if (bShowSpawnMotion)
	{
		SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
		m_afAffectFlag.Set(AFF_SPAWN);
	}

	SetXYZ(x, y, z);

	m_posDest.x = x;
	m_posDest.y = y;
	m_posDest.z = z;

	m_posStart.x = x;
	m_posStart.y = y;
	m_posStart.z = z;

	if (bChangeTree)
	{
		EncodeInsertPacket(this);
		sectree->InsertEntity(this);

		UpdateSectree();
	}
	else
	{
		ViewReencode();
		sys_log(!test_server, "Show in same sectree");
	}

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	if (IsNPC())
	{
		if (IS_SET(GetAIFlag(), AIFLAG_NOMOVE) && IS_SET(GetAIFlag(), AIFLAG_AGGRESSIVE))
			SaveExitLocation();
	}
	
	SetValidComboInterval(0);

	if (IsPC())
	{
		if (m_pkFakeBuffSpawn)
			m_pkFakeBuffSpawn->Show(lMapIndex, x, y, z, bShowSpawnMotion);
		
		if (GetMountSystem() && !GetMountSystem()->IsRiding() && GetMountSystem()->GetMount())
			GetMountSystem()->GetMount()->Show(lMapIndex, x, y, z, bShowSpawnMotion);

		if (m_petSystem)
			m_petSystem->ShowPets(lMapIndex, x, y, z, bShowSpawnMotion);
	}
	return true;
}

// BGM_INFO
struct BGMInfo
{
	std::string	name;
	float		vol;
};

typedef std::map<unsigned, BGMInfo> BGMInfoMap;
static BGMInfoMap 	gs_bgmInfoMap;

typedef std::map<std::pair<int, bool>, int> WarpLevelLimitMap;
static WarpLevelLimitMap	gs_warpLevelLimitMap;

void CHARACTER_AddBGMInfo(unsigned mapIndex, const char* name, float vol)
{
	BGMInfo newInfo;
	newInfo.name = name;
	newInfo.vol = vol;

	gs_bgmInfoMap[mapIndex] = newInfo;

	sys_log(!test_server, "bgm_info.add_info(%d, '%s', %f)", mapIndex, name, vol);
}

const BGMInfo& CHARACTER_GetBGMInfo(unsigned mapIndex)
{
	BGMInfoMap::iterator f = gs_bgmInfoMap.find(mapIndex);
	if (gs_bgmInfoMap.end() == f)
	{
		static BGMInfo s_empty = {"", 0.0f};
		return s_empty;
	}
	return f->second;
}
// END_OF_BGM_INFO

void CHARACTER_AddWarpLevelLimit(int iMapIndex, int iLevelLimit, bool bIsLimitMin)
{
	gs_warpLevelLimitMap[std::make_pair(iMapIndex, bIsLimitMin)] = iLevelLimit;
}

int* CHARACTER_GetWarpLevelLimit(int iMapIndex, bool bIsMin)
{
	WarpLevelLimitMap::iterator f = gs_warpLevelLimitMap.find(std::make_pair(iMapIndex, bIsMin));
	if (f == gs_warpLevelLimitMap.end())
		return NULL;
	return &f->second;
}

void CHARACTER::MainCharacterPacket()
{
	const unsigned mapIndex = GetMapIndex();
	const BGMInfo& bgmInfo = CHARACTER_GetBGMInfo(mapIndex);

	network::GCOutputPacket<network::GCMainCharacterPacket> pack;
	pack->set_vid(m_vid);
	pack->set_race_num(GetRaceNum());
	pack->set_x(GetX());
	pack->set_y(GetY());
	pack->set_z(GetZ());
	pack->set_empire(GetDesc()->GetEmpire());
	pack->set_skill_group(GetSkillGroup());
	pack->set_chr_name(GetName());
	pack->set_bgm_vol(bgmInfo.vol);
	pack->set_bgm_name(bgmInfo.name.c_str());
	GetDesc()->Packet(pack);
}

DWORD CHARACTER::GetCurrentHair() const
{
	if (IsPC())
	{
		DWORD dwHair = GetQuestFlag("dyeing_hair.current_hair");
		if (dwHair)
			return dwHair;
	}

#ifdef __HAIR_SELECTOR__
	if (m_dwSelectedHairPart != 0)
	{
		auto c_pTab = ITEM_MANAGER::instance().GetTable(m_dwSelectedHairPart);
		if (c_pTab)
			return c_pTab->values(3);
	}
#endif

	return 0;
}

void CHARACTER::PointsPacket()
{
	if (!GetDesc())
		return;

	network::GCOutputPacket<network::GCPointsPacket> pack;
	for (int i = 0; i < POINT_MAX_NUM; ++i)
	{
		pack->add_points(GetPoint(i));
		pack->add_real_points(GetRealPoint(i));
	}

	pack->set_points(POINT_LEVEL, GetLevel());
	pack->set_points(POINT_EXP, GetExp());
	pack->set_points(POINT_NEXT_EXP, GetNextExp());
	pack->set_points(POINT_HP, GetHP());
	pack->set_points(POINT_MAX_HP, GetMaxHP());
	pack->set_points(POINT_SP, GetSP());
	pack->set_points(POINT_MAX_SP, GetMaxSP());
	pack->set_points(POINT_GOLD, GetGold());
#ifdef ENABLE_ZODIAC_TEMPLE
	pack->set_points(POINT_ANIMASPHERE, GetAnimasphere());
#endif
	pack->set_points(POINT_STAMINA, GetStamina());
	pack->set_points(POINT_MAX_STAMINA, GetMaxStamina());

	if (test_server)
		sys_log(0, "SendPointsPacket %s size %d", GetName(), pack->ByteSize());

	GetDesc()->Packet(pack);
}

bool CHARACTER::ChangeSex()
{
	int src_race = GetRaceNum();

	switch (src_race)
	{
		case MAIN_RACE_WARRIOR_M:
			m_points.job = MAIN_RACE_WARRIOR_W;
			break;

		case MAIN_RACE_WARRIOR_W:
			m_points.job = MAIN_RACE_WARRIOR_M;
			break;

		case MAIN_RACE_ASSASSIN_M:
			m_points.job = MAIN_RACE_ASSASSIN_W;
			break;

		case MAIN_RACE_ASSASSIN_W:
			m_points.job = MAIN_RACE_ASSASSIN_M;
			break;

		case MAIN_RACE_SURA_M:
			m_points.job = MAIN_RACE_SURA_W;
			break;

		case MAIN_RACE_SURA_W:
			m_points.job = MAIN_RACE_SURA_M;
			break;

		case MAIN_RACE_SHAMAN_M:
			m_points.job = MAIN_RACE_SHAMAN_W;
			break;

		case MAIN_RACE_SHAMAN_W:
			m_points.job = MAIN_RACE_SHAMAN_M;
			break;

		default:
			sys_err("CHANGE_SEX: %s unknown race %d", GetName(), src_race);
			return false;
	}

	sys_log(0, "CHANGE_SEX: %s (%d -> %d)", GetName(), src_race, m_points.job);
	return true;
}

WORD CHARACTER::GetRaceNum() const
{
	if (m_dwPolymorphRace)
		return m_dwPolymorphRace;

	if (m_pkMobData)
		return m_pkMobData->m_table.vnum();

	return m_points.job;
}

void CHARACTER::SetRace(BYTE race)
{
	if (race >= MAIN_RACE_MAX_NUM)
	{
		sys_err("CHARACTER::SetRace(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
		return;
	}

	m_points.job = race;
}

BYTE CHARACTER::GetJob() const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetJob();
#endif
#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		return JOB_SHAMAN;
#endif

	unsigned race = m_points.job;
	unsigned job;

	if (RaceToJob(race, &job))
		return job;

	sys_err("CHARACTER::GetJob(name=%s, race=%d).OUT_OF_RACE_RANGE", GetName(), race);
	return JOB_WARRIOR;
}

void CHARACTER::SetLevel(BYTE level)
{
	m_points.level = level;
#ifdef COMBAT_ZONE
	if (IsPC() && CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif

#ifdef ENABLE_BLOCK_PKMODE
	if (m_blockPKMode)
		return;
#endif

	if (IsPC())
	{
		if (level < g_bPKProtectLevel)
			SetPKMode(PK_MODE_PROTECT);
		else if (GetGMLevel() != GM_PLAYER)
			SetPKMode(PK_MODE_PROTECT);
		else if (m_bPKMode == PK_MODE_PROTECT)
			SetPKMode(PK_MODE_PEACE);
	}
}

#ifdef ENABLE_BLOCK_PKMODE
void CHARACTER::SetBlockPKMode(BYTE bPKMode, bool block)
{
	SetPKMode(bPKMode);
	m_blockPKMode = block;
}
#endif

void CHARACTER::SetEmpire(BYTE bEmpire)
{
	m_bEmpire = bEmpire;
}

void CHARACTER::SetPlayerProto(const TPlayerTable * t)
{
	if (!GetDesc() || !*GetDesc()->GetHostName())
		sys_err("cannot get desc or hostname");
	else
		SetGMLevel();

	m_bCharType = CHAR_TYPE_PC;

	m_dwPlayerID = t->id();

	m_iAlignment = t->alignment();
	m_iRealAlignment = t->alignment();

	m_points.voice = t->voice();

	m_points.skill_group = t->skill_group();

	m_pointsInstant.bBasePart = t->part_base();
	SetPart(PART_HAIR, t->parts(PART_HAIR));
#ifdef __ACCE_COSTUME__
	SetPart(PART_ACCE, t->parts(PART_ACCE));
#endif

#ifdef __HAIR_SELECTOR__
	m_dwSelectedHairPart = t->part_hair_base();
#endif

	// REMOVE_REAL_SKILL_LEVLES
	if (m_pSkillLevels)
		M2_DELETE_ARRAY(m_pSkillLevels);

	m_pSkillLevels = M2_NEW TPlayerSkill[SKILL_MAX_NUM];

	for (int i = 0; i < MIN(SKILL_MAX_NUM, t->skills_size()); ++i)
		m_pSkillLevels[i] = t->skills(i);
	// END_OF_REMOVE_REAL_SKILL_LEVLES

	// init skill level
	SetSkillLevel(SKILL_MINING, 40);
	SetSkillLevel(SKILL_FISHING, 40);
	SetSkillLevel(SKILL_LANGUAGE1, 40);
	SetSkillLevel(SKILL_LANGUAGE2, 40);
	SetSkillLevel(SKILL_LANGUAGE3, 40);
	// end of init skill level

	if (t->map_index() >= 10000)
	{
		m_posWarp.x = t->exit_x();
		m_posWarp.y = t->exit_y();
		m_lWarpMapIndex = t->exit_map_index();
	}

	SetRealPoint(POINT_PLAYTIME, t->playtime());
	m_dwLoginPlayTime = t->playtime();
	SetRealPoint(POINT_ST, t->st());
	SetRealPoint(POINT_HT, t->ht());
	SetRealPoint(POINT_DX, t->dx());
	SetRealPoint(POINT_IQ, t->iq());

	SetPoint(POINT_ST, t->st());
	SetPoint(POINT_HT, t->ht());
	SetPoint(POINT_DX, t->dx());
	SetPoint(POINT_IQ, t->iq());

	SetPoint(POINT_STAT, t->stat_point());
	SetPoint(POINT_SKILL, t->skill_point());
	SetPoint(POINT_SUB_SKILL, t->sub_skill_point());
	SetPoint(POINT_HORSE_SKILL, t->horse_skill_point());

	SetPoint(POINT_STAT_RESET_COUNT, t->stat_reset_count());

	SetPoint(POINT_LEVEL_STEP, t->level_step());
	SetRealPoint(POINT_LEVEL_STEP, t->level_step());

	SetRace(t->job());

	SetLevel(t->level());
#ifdef __PRESTIGE__
	SetPrestigeLevel(t->prestige_level());
#endif
	SetExp(t->exp());
	SetGold(t->gold());
#ifdef ENABLE_ZODIAC_TEMPLE
	SetAnimasphere(t->animasphere());
#endif
#ifdef __GAYA_SYSTEM__
	m_dwGaya = t->gaya();
#endif

#ifdef __ATTRTREE__
	int i = 0;
	for (auto& row : m_aAttrTree)
	{
		for (BYTE& col : row)
		{
			if (i < t->attrtree_size())
				col = t->attrtree(i);
			else
				col = 0;

			++i;
		}
	}
#endif

	SetMapIndex(t->map_index());
	SetXYZ(t->x(), t->y(), 0);

#ifdef ENABLE_COMPANION_NAME
	LoadCompanionNames();
#endif

	ComputePoints();

	SetHP(t->hp());
	SetSP(t->sp());
	SetStamina(GetMaxStamina());

	//GMÀÏ¶§ º¸È£¸ðµå  
	if ((!test_server && GetGMLevel() >= GM_LOW_WIZARD) ||
		(test_server && GM::get_level(GetName(), GetDesc()->GetAccountTable().login().c_str(), true) >= GM_LOW_WIZARD))
	{
		m_afAffectFlag.Set(AFF_GAMEMASTER);
		m_bPKMode = PK_MODE_PROTECT;
	}

	if (GetLevel() < g_bPKProtectLevel)
		m_bPKMode = PK_MODE_PROTECT;

	if (m_pkMountSystem)
		delete m_pkMountSystem;
	m_pkMountSystem = M2_NEW CMountSystem(this);
	m_pkMountSystem->SetSummonItemID(t->mount_item_id());
	if (strcmp("", t->mount_name().c_str()) != 0)
		SetHorseName(t->mount_name());
	m_bTempMountState = t->mount_state();
#ifdef HORSE_SECOND_GRADE
	SetHorseGrade(MAX(t->horse_grade(), HORSE_MAX_GRADE - 1), false);
#else
	//SetHorseGrade(t->horse_grade, false);
	SetHorseGrade(HORSE_MAX_GRADE, false);
#endif
	SetHorseElapsedTime(t->horse_elapsed_time());
	//if (t->horse_grade == 0)
	//	SetHorsesHoeTimeout(HORSES_HOE_TIMEOUT_TIME);

	for (int i = 0; i < t->premium_times_size(); ++i)
		m_aiPremiumTimes[i] = t->premium_times(i);

	m_dwLogOffInterval = t->logoff_interval();

	sys_log(0, "PLAYER_LOAD: %s PREMIUM %d %d, LOGGOFF_INTERVAL %u PTR: %p", t->name().c_str(),
		m_aiPremiumTimes[0], m_aiPremiumTimes[1], t->logoff_interval(), this);

	if (GetGMLevel() != GM_PLAYER) 
	{
		LogManager::instance().CharLog(this, GetGMLevel(), "GM_LOGIN", "");
		sys_log(0, "GM_LOGIN(gmlevel=%d, name=%s(%d), pos=(%d, %d)", GetGMLevel(), GetName(), GetPlayerID(), GetX(), GetY());
	}

#ifdef __PET_SYSTEM__
	// NOTE: ÀÏ´Ü Ä³¸¯ÅÍ°¡ PCÀÎ °æ¿ì¿¡¸¸ PetSystemÀ» °®µµ·Ï ÇÔ. À¯·´ ¸Ó½Å´ç ¸Þ¸ð¸® »ç¿ë·ü¶§¹®¿¡ NPC±îÁö ÇÏ±ä Á»..
	if (m_petSystem)
		//m_petSystem->Destroy(); running on delete
		M2_DELETE(m_petSystem);

	m_petSystem = M2_NEW CPetSystem(this);
#endif

	SetInventoryMaxNum(t->inventory_max_num(), INVENTORY_SIZE_TYPE_NORMAL);
	SetInventoryMaxNum(t->uppitem_inv_max_num(), INVENTORY_SIZE_TYPE_UPPITEM);
	SetInventoryMaxNum(t->skillbook_inv_max_num(), INVENTORY_SIZE_TYPE_SKILLBOOK);
	SetInventoryMaxNum(t->stone_inv_max_num(), INVENTORY_SIZE_TYPE_STONE);
	SetInventoryMaxNum(t->enchant_inv_max_num(), INVENTORY_SIZE_TYPE_ENCHANT);

#ifdef __FAKE_BUFF__
	m_abFakeBuffSkillLevel[0] = t->fakebuff_skill1();
	m_abFakeBuffSkillLevel[1] = t->fakebuff_skill2();
	m_abFakeBuffSkillLevel[2] = t->fakebuff_skill3();
#endif

	memset(m_abPlayerDataChanged, 0, sizeof(m_abPlayerDataChanged));
#ifdef ENABLE_RUNE_SYSTEM
	m_runePage = t->rune_page_data();
#endif

#ifdef ENABLE_RUNE_PAGES
	LoadSelectedRunes();
#endif

#ifdef __EQUIPMENT_CHANGER__
	m_dwEquipmentChangerPageIndex = t->equipment_page_index();
#endif

#ifdef COMBAT_ZONE
	m_dwCombatZonePoints = t->combat_zone_points();
	m_bCombatZoneRank = GetCombatZoneRank();
#endif

#ifdef __PRESTIGE__
	m_bPrestigeLevel = t->prestige();
#endif
}

EVENTFUNC(kill_ore_load_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "kill_ore_load_even> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}	

	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

float CHARACTER::GetPointF(unsigned char type, bool bGetSum) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	float val = m_pointsInstantF[type];
	if (bGetSum)
		val += (float)GetPoint(type, false);

	int max_val = INT_MAX;

	switch (type)
	{
		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			max_val = 50;
			break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR: %s type %d val %f (max: %d)", GetName(), val, max_val);

	return (val);
}

void CHARACTER::SetPointF(unsigned char type, float val)
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return;
	}

	m_pointsInstantF[type] = val;

	if ((type == POINT_MOV_SPEED) && get_dword_time() < m_dwMoveStartTime + m_dwMoveDuration)
	{
		CalculateMoveDuration();
	}
}

void CHARACTER::ApplyPointF(unsigned char bApplyType, float fVal)
{
	// if (test_server)
		// ChatPacket(CHAT_TYPE_INFO, "ApplyPoint(%d, %f)", bApplyType, fVal);

	if (bApplyType == APPLY_NONE || bApplyType >= MAX_APPLY_NUM)
	{
		sys_err("invalid apply type %u", bApplyType);
		return;
	}

	BYTE bPointType = aApplyInfo[bApplyType].bPointType;
	SetPointF(bPointType, GetPointF(bPointType, false) + fVal);
	PointChange(bPointType, 0);

	switch (bApplyType)
	{
		case APPLY_CON:
			PointChange(POINT_MAX_HP, (int)(fVal * JobInitialPoints[GetJob()].hp_per_ht));
			PointChange(POINT_MAX_STAMINA, (int)(fVal * JobInitialPoints[GetJob()].stamina_per_con));
			break;

		case APPLY_INT: 
			PointChange(POINT_MAX_SP, (int)(fVal * JobInitialPoints[GetJob()].sp_per_iq));
			break;

		case APPLY_ATT_GRADE_BONUS:
			SetPointF(POINT_ATT_GRADE, GetPointF(POINT_ATT_GRADE, false) + fVal);
			PointChange(POINT_ATT_GRADE, 0);
			break;

		case APPLY_DEF_GRADE_BONUS:
			SetPointF(POINT_DEF_GRADE, GetPointF(POINT_DEF_GRADE, false) + fVal);
			SetPointF(POINT_CLIENT_DEF_GRADE, GetPointF(POINT_CLIENT_DEF_GRADE, false) + fVal);
			PointChange(POINT_DEF_GRADE, 0);
			break;
	}
}

void CHARACTER::SetProto(const CMob * pkMob)
{
	if (m_pkMobInst)
		M2_DELETE(m_pkMobInst);

	m_pkMobData = pkMob;
	m_pkMobInst = M2_NEW CMobInstance;

	m_bPKMode = PK_MODE_FREE;

	auto t = &m_pkMobData->m_table;

	m_bCharType = t->type();

	SetLevel(t->level());
	SetEmpire(t->empire());

	SetExp(t->exp());
	SetRealPoint(POINT_ST, t->str());
	SetRealPoint(POINT_DX, t->dex());
	SetRealPoint(POINT_HT, t->con());
	SetRealPoint(POINT_IQ, t->int_());

	ComputePoints();

	SetHP(GetMaxHP());
	SetSP(GetMaxSP());

	////////////////////
	m_pointsInstant.dwAIFlag = t->ai_flag();
	SetImmuneFlag(t->immune_flag());

	AssignTriggers(t);

	ApplyMobAttribute(t);

	if (IsStone())
	{
		DetermineDropMetinStone();
	}

	if (IsWarp() || IsGoto())
	{
		StartWarpNPCEvent();
	}

	CHARACTER_MANAGER::instance().RegisterRaceNumMap(this);

	// XXX CTF GuildWar hardcoding
	if (warmap::IsWarFlag(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlag, &CHARACTER::EndStateEmpty);
	}

	if (warmap::IsWarFlagBase(GetRaceNum()))
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateFlagBase, &CHARACTER::EndStateEmpty);
	}

	if (m_bCharType == CHAR_TYPE_MOUNT)
	{
		m_stateIdle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
		m_stateMove.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateMove, &CHARACTER::EndStateEmpty);
		m_stateBattle.Set(this, &CHARACTER::BeginStateEmpty, &CHARACTER::StateHorse, &CHARACTER::EndStateEmpty);
	}

	// MINING
	if (mining::IsVeinOfOre (GetRaceNum()))
	{
		char_event_info* info = AllocEventInfo<char_event_info>();

		info->ch = this;

		m_pkMiningEvent = event_create(kill_ore_load_event, info, PASSES_PER_SEC(random_number(7 * 60, 15 * 60)));
	}
	// END_OF_MINING
}

const network::TMobTable & CHARACTER::GetMobTable() const
{
	return m_pkMobData->m_table;
}

bool CHARACTER::IsRaceFlag(DWORD dwBit) const
{
	return m_pkMobData ? IS_SET(m_pkMobData->m_table.race_flag(), dwBit) : 0;
}

DWORD CHARACTER::GetMobDamageMin() const
{
#ifdef __RIFT_SYSTEM__
	if (GetDungeon())
	{
		long long llDamMin = m_pkMobData->m_table.dwDamageRange[0];
		llDamMin = llDamMin * (long long)GetDungeon()->GetEnemyDamFactor() / 100LL;
		return llDamMin;
	}
	else
#endif
	{
		return m_pkMobData->m_table.damage_min();
	}
}

DWORD CHARACTER::GetMobDamageMax() const
{
#ifdef __RIFT_SYSTEM__
	if (GetDungeon())
	{
		long long llDamMax = m_pkMobData->m_table.dwDamageRange[1];
		llDamMax = llDamMax * (long long)GetDungeon()->GetEnemyDamFactor() / 100LL;
		return llDamMax;
	}
	else
#endif
	{
		return m_pkMobData->m_table.damage_max();
	}
}

float CHARACTER::GetMobDamageMultiply() const
{
	float fDamMultiply = GetMobTable().dam_multiply();

	if (IsBerserk())
		fDamMultiply = fDamMultiply * 2.0f; // BALANCE: ±¤ÆøÈ­ ½Ã µÎ¹è

	return fDamMultiply;
}

DWORD CHARACTER::GetMobDropItemVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.drop_item_vnum() : 0;
}

bool CHARACTER::IsSummonMonster() const
{
	return GetSummonVnum() != 0;
}

DWORD CHARACTER::GetSummonVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.summon_vnum() : 0;
}

DWORD CHARACTER::GetPolymorphItemVnum() const
{
	return m_pkMobData ? m_pkMobData->m_table.polymorph_item_vnum() : 0;
}

DWORD CHARACTER::GetMonsterDrainSPPoint() const
{
	return m_pkMobData ? m_pkMobData->m_table.drain_sp() : 0;
}

BYTE CHARACTER::GetMobRank() const
{
	if (!m_pkMobData)
		return MOB_RANK_KNIGHT;	// PCÀÏ °æ¿ì KNIGHT±Þ

	return m_pkMobData->m_table.rank();
}

BYTE CHARACTER::GetMobSize() const
{
	if (!m_pkMobData)
		return MOBSIZE_MEDIUM;

	return m_pkMobData->m_table.size();
}

WORD CHARACTER::GetMobAttackRange() const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
	{
		LPITEM pkWeapon = GetWear(WEAR_WEAPON);
		if (pkWeapon && pkWeapon->GetType() == ITEM_WEAPON && pkWeapon->GetSubType() == WEAPON_BOW)
			return 600;
		else
			return 250;
	}
#endif

	if (GetForceMonsterAttackRange() != 0)
		return GetForceMonsterAttackRange();
	
	switch (GetMobBattleType())
	{
		case BATTLE_TYPE_RANGE:
		case BATTLE_TYPE_MAGIC:
			return m_pkMobData->m_table.attack_range() + GetPoint(POINT_BOW_DISTANCE);  
		default:
			return m_pkMobData->m_table.attack_range(); 
	}
}

BYTE CHARACTER::GetMobBattleType() const
{
#ifdef __FAKE_PC__
	if (!m_pkMobData || FakePC_Check())
#else
	if (!m_pkMobData)
#endif
	{
		if (IsPolymorphed())
		{
			const CMob* polyMob = CMobManager::instance().Get(m_dwPolymorphRace);
			if (polyMob)
				return polyMob->m_table.battle_type();
		}

		if (GetJob() == JOB_ASSASSIN &&
			GetWear(WEAR_WEAPON) && GetWear(WEAR_WEAPON)->GetType() == ITEM_WEAPON && GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_BOW)
			return BATTLE_TYPE_RANGE;
		else
			return BATTLE_TYPE_MELEE;
	}

	return (m_pkMobData->m_table.battle_type());
}

void CHARACTER::ComputeBattlePoints()
{
	if (IsPolymorphed())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);
		int iAtt = 0;
		int iDef = 0;

		if (pMob)
		{
			iAtt = GetLevel() * 2 + GetPolymorphPoint(POINT_ST) * 2;
			// lev + con
			iDef = GetLevel() + GetPolymorphPoint(POINT_HT) + pMob->m_table.def();
		}

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE)); 
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
#ifdef __FAKE_PC__
	else if (IsPC() || FakePC_Check())
#else
	else if (IsPC())
#endif
	{
		SetPoint(POINT_ATT_GRADE, 0);
		SetPoint(POINT_DEF_GRADE, 0);
		SetPoint(POINT_CLIENT_DEF_GRADE, 0);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE));
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));

		//
		// ±âº» ATK = 2lev + 2str, Á÷¾÷¿¡ ¸¶´Ù 2strÀº ¹Ù²ð ¼ö ÀÖÀ½
		//
		int iAtk = GetLevel() * 2;
		int iStatAtk = 0;

		switch (GetJob())
		{
			case JOB_WARRIOR:
			case JOB_SURA:
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;

			case JOB_ASSASSIN:
				iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_DX)) / 3;
				break;

			case JOB_SHAMAN:
				iStatAtk = (4 * GetPoint(POINT_ST) + 2 * GetPoint(POINT_IQ)) / 3;
				break;

#ifdef __WOLFMAN__
			case JOB_WOLFMAN:
				iStatAtk = (4 * GetPoint(POINT_DX) + 2 * GetPoint(POINT_HT)) / 3;
				break;
#endif

			default:
				sys_err("invalid job %d", GetJob());
				iStatAtk = (2 * GetPoint(POINT_ST));
				break;
		}

		// ¸»À» Å¸°í ÀÖ°í, ½ºÅÈÀ¸·Î ÀÎÇÑ °ø°Ý·ÂÀÌ ST*2 º¸´Ù ³·À¸¸é ST*2·Î ÇÑ´Ù.
		// ½ºÅÈÀ» Àß¸ø ÂïÀº »ç¶÷ °ø°Ý·ÂÀÌ ´õ ³·Áö ¾Ê°Ô ÇÏ±â À§ÇØ¼­´Ù.
		if (GetMountVnum() && iStatAtk < 2 * GetPoint(POINT_ST))
			iStatAtk = (2 * GetPoint(POINT_ST));

		iAtk += iStatAtk;

		// ½Â¸¶(¸») : °Ë¼ö¶ó µ¥¹ÌÁö °¨¼Ò  
		if (GetMountVnum())
		{
			if (GetJob() == JOB_SURA && GetSkillGroup() == 1)
			{
				iAtk += (iAtk * GetHorseGrade() * 10) / 60;
			}
			else
			{
				iAtk += (iAtk * GetHorseGrade() * 10) / 30;
			}
		}
		
		//
		// ATK Setting
		//
		iAtk += GetPoint(POINT_ATT_GRADE_BONUS);

		PointChange(POINT_ATT_GRADE, iAtk);

		// DEF = LEV + CON + ARMOR
		int iShowDef = GetLevel() + GetPoint(POINT_HT); // For Ymir(Ãµ¸¶)
		int iDef = GetLevel() + (int) (GetPoint(POINT_HT) / 1.25); // For Other
		int iArmor = 0;

		LPITEM pkItem;

		for (int i = 0; i < WEAR_MAX_NUM; ++i)
			if ((pkItem = GetWear(i)) && pkItem->GetType() == ITEM_ARMOR)
			{
				if (pkItem->GetSubType() == ARMOR_BODY || pkItem->GetSubType() == ARMOR_HEAD || pkItem->GetSubType() == ARMOR_FOOTS || pkItem->GetSubType() == ARMOR_SHIELD)
				{
					iArmor += pkItem->GetValue(1);
					iArmor += (2 * pkItem->GetValue(5));
				}
			}

		// ¸» Å¸°í ÀÖÀ» ¶§ ¹æ¾î·ÂÀÌ ¸»ÀÇ ±âÁØ ¹æ¾î·Âº¸´Ù ³·À¸¸é ±âÁØ ¹æ¾î·ÂÀ¸·Î ¼³Á¤
		if (IsRiding())
		{
			const int iMountArmor = 32 + 64 * 3 / 4;
			if (iArmor < iMountArmor)
				iArmor = iMountArmor;
		}

		iArmor += GetPoint(POINT_DEF_GRADE_BONUS);
		iArmor += GetPoint(POINT_PARTY_DEFENDER_BONUS);

		// INTERNATIONAL_VERSION
		PointChange(POINT_DEF_GRADE, iDef + iArmor);
		PointChange(POINT_CLIENT_DEF_GRADE, (iShowDef + iArmor) - GetPoint(POINT_DEF_GRADE));
		// END_OF_INTERNATIONAL_VERSION

		PointChange(POINT_MAGIC_ATT_GRADE, GetLevel() * 2 + GetPoint(POINT_IQ) * 2 + GetPoint(POINT_MAGIC_ATT_GRADE_BONUS));
		PointChange(POINT_MAGIC_DEF_GRADE, GetLevel() + (GetPoint(POINT_IQ) * 3 + GetPoint(POINT_HT)) / 3 + iArmor / 2 + GetPoint(POINT_MAGIC_DEF_GRADE_BONUS));
	}
	else
	{
		// 2lev + str * 2
		int iAtt = GetLevel() * 2 + GetPoint(POINT_ST) * 2;
		// lev + con
		WORD wdef = 0;
		if (!m_pkMobData)
			sys_err("no mob table for %d", GetRaceNum());
		else
			wdef = GetMobTable().def();

		int iDef = GetLevel() + GetPoint(POINT_HT) + wdef;

		SetPoint(POINT_ATT_GRADE, iAtt);
		SetPoint(POINT_DEF_GRADE, iDef);
		SetPoint(POINT_MAGIC_ATT_GRADE, GetPoint(POINT_ATT_GRADE)); 
		SetPoint(POINT_MAGIC_DEF_GRADE, GetPoint(POINT_DEF_GRADE));
	}
}

void CHARACTER::ComputePassiveSkillPoints(bool bAdd)
{
	const DWORD c_adwSkillIDs[] = { SKILL_PASSIVE_RESIST_CRIT, SKILL_PASSIVE_RESIST_PENE 
#ifdef STANDARD_SKILL_DURATION
		, SKILL_PASSIVE_SKILL_DURATION
#endif
	};
	for (DWORD dwSkillID : c_adwSkillIDs)
	{
		BYTE bSkillLevel = GetSkillLevel(dwSkillID);
		if (bSkillLevel <= 0)
			continue;

		CSkillProto* pkSk = CSkillManager::Instance().Get(dwSkillID);
		if (!pkSk)
			continue;

		pkSk->SetVar("k", GetSkillPowerByLevel(bSkillLevel) / 100.0);
		int iVal = pkSk->kPointPoly.Evaluate();

		PointChange(pkSk->bPointOn, bAdd ? iVal : -iVal);
		if (test_server && !GetDesc()->IsPhase(PHASE_SELECT))
			ChatPacket(CHAT_TYPE_INFO, "ComputePassiveSkill: skill %d type %d val %d", dwSkillID, pkSk->bPointOn, bAdd ? iVal : -iVal);
	}
}

bool CHARACTER::HasShining(BYTE shiningID)
{
	for (BYTE i = 0; i < SHINING_MAX_NUM; ++i)
		if (m_bShinings[i] == shiningID)
			return true;

	return false;
}

void CHARACTER::ComputePoints()
{
	if (IsPC() && m_pkTimedEvent != NULL)
		return;

	long lStat = GetPoint(POINT_STAT);
	long lStatResetCount = GetPoint(POINT_STAT_RESET_COUNT);
	long lSkillActive = GetPoint(POINT_SKILL);
	long lSkillSub = GetPoint(POINT_SUB_SKILL);
	long lSkillHorse = GetPoint(POINT_HORSE_SKILL);
	long lLevelStep = GetPoint(POINT_LEVEL_STEP);

	long lAttackerBonus = GetPoint(POINT_PARTY_ATTACKER_BONUS);
	long lTankerBonus = GetPoint(POINT_PARTY_TANKER_BONUS);
	long lBufferBonus = GetPoint(POINT_PARTY_BUFFER_BONUS);
	long lSkillMasterBonus = GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);
	long lHasteBonus = GetPoint(POINT_PARTY_HASTE_BONUS);
	long lDefenderBonus = GetPoint(POINT_PARTY_DEFENDER_BONUS);

	long lHPRecovery = GetPoint(POINT_HP_RECOVERY);
	long lSPRecovery = GetPoint(POINT_SP_RECOVERY);

	memset(m_pointsInstant.points, 0, sizeof(m_pointsInstant.points));
	memset(m_pointsInstantF, 0, sizeof(m_pointsInstantF));
#ifdef ENABLE_RUNE_SYSTEM
	m_runeData.storedAttackSpeed = 0;
	m_runeData.storedMovementSpeed = 0;
#endif
	BuffOnAttr_ClearAll();
	m_SkillDamageBonus.clear();

	SetPoint(POINT_STAT, lStat);
	SetPoint(POINT_SKILL, lSkillActive);
	SetPoint(POINT_SUB_SKILL, lSkillSub);
	SetPoint(POINT_HORSE_SKILL, lSkillHorse);
	SetPoint(POINT_LEVEL_STEP, lLevelStep);
	SetPoint(POINT_STAT_RESET_COUNT, lStatResetCount);

	SetPoint(POINT_ST, GetRealPoint(POINT_ST));
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	SetPart(PART_MAIN, GetOriginalPart(PART_MAIN));
	SetPart(PART_WEAPON, GetOriginalPart(PART_WEAPON));
	SetPart(PART_HEAD, GetOriginalPart(PART_HEAD));
	SetPart(PART_HAIR, GetOriginalPart(PART_HAIR));
#ifdef __ACCE_COSTUME__
	SetPart(PART_ACCE, GetOriginalPart(PART_ACCE));
#endif

	SetPoint(POINT_PARTY_ATTACKER_BONUS, lAttackerBonus);
	SetPoint(POINT_PARTY_TANKER_BONUS, lTankerBonus);
	SetPoint(POINT_PARTY_BUFFER_BONUS, lBufferBonus);
	SetPoint(POINT_PARTY_SKILL_MASTER_BONUS, lSkillMasterBonus);
	SetPoint(POINT_PARTY_HASTE_BONUS, lHasteBonus);
	SetPoint(POINT_PARTY_DEFENDER_BONUS, lDefenderBonus);

	SetPoint(POINT_HP_RECOVERY, lHPRecovery);
	SetPoint(POINT_SP_RECOVERY, lSPRecovery);

	SetPoint(POINT_ANTI_EXP, IsEXPDisabled());

	int iMaxHP, iMaxSP;
	int iMaxStamina;

#ifdef __FAKE_PC__
	if (IsPC() || FakePC_Check())
#else
	if (IsPC())
#endif
	{
		TJobInitialPoints& rkInit = JobInitialPoints[GetJob()];
		int iMediumRandomHP = rkInit.hp_per_lv_begin + (rkInit.hp_per_lv_end - rkInit.hp_per_lv_begin) / 2;
		int iMediumRandomSP = rkInit.sp_per_lv_begin + (rkInit.sp_per_lv_end - rkInit.sp_per_lv_begin) / 2;
		iMaxHP = rkInit.max_hp + iMediumRandomHP * (GetLevel() - 1) + GetPoint(POINT_HT) * rkInit.hp_per_ht;
		iMaxSP = rkInit.max_sp + iMediumRandomSP * (GetLevel() - 1) + GetPoint(POINT_IQ) * rkInit.sp_per_iq;
		iMaxStamina = rkInit.max_stamina + GetPoint(POINT_HT) * rkInit.stamina_per_con;

		{
			CSkillProto* pkSk = CSkillManager::instance().Get(SKILL_ADD_HP);

			if (NULL != pkSk)
			{
				pkSk->SetVar("k", 1.0f * GetSkillPower(SKILL_ADD_HP) / 100.0f);

				iMaxHP += static_cast<int>(pkSk->kPointPoly.Evaluate());
			}
		}

		// ±âº» °ªµé
		SetPoint(POINT_MOV_SPEED,	200);
		SetPoint(POINT_ATT_SPEED,	100);
#ifdef ENABLE_RUNE_SYSTEM
		m_runeData.storedMovementSpeed = 200;
		m_runeData.storedAttackSpeed = 100;
#endif
		PointChange(POINT_ATT_SPEED, GetPoint(POINT_PARTY_HASTE_BONUS));
		SetPoint(POINT_CASTING_SPEED,	100);

		if (GetMountVnum())
		{
			if (GetMountSystem())
				GetMountSystem()->GiveBuff();
		}
	}
	else
	{
		iMaxHP = m_pkMobData->m_table.max_hp();
		iMaxSP = 0;
		iMaxStamina = 0;

		SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.attack_speed());
		SetPoint(POINT_MOV_SPEED, m_pkMobData->m_table.moving_speed());
#ifdef ENABLE_RUNE_SYSTEM
		m_runeData.storedMovementSpeed = m_pkMobData->m_table.moving_speed();
		m_runeData.storedAttackSpeed = m_pkMobData->m_table.attack_speed();
#endif
		SetPoint(POINT_CASTING_SPEED, m_pkMobData->m_table.attack_speed());
	}

	ComputeBattlePoints();
	ComputePassiveSkillPoints();

	// ±âº» HP/SP ¼³Á¤
	if (iMaxHP != GetMaxHP())
	{
		SetRealPoint(POINT_MAX_HP, iMaxHP); // ±âº»HP¸¦ RealPoint¿¡ ÀúÀåÇØ ³õ´Â´Ù.
	}

	PointChange(POINT_MAX_HP, 0);

	if (iMaxSP != GetMaxSP())
	{
		SetRealPoint(POINT_MAX_SP, iMaxSP); // ±âº»SP¸¦ RealPoint¿¡ ÀúÀåÇØ ³õ´Â´Ù.
	}

	PointChange(POINT_MAX_SP, 0);

	SetMaxStamina(iMaxStamina);

	m_pointsInstant.dwImmuneFlag = 0;

	for (int i = 0 ; i < WEAR_MAX_NUM; i++) 
	{
		LPITEM pItem = GetWear(i);
		if (pItem)
		{
			pItem->ModifyPoints(true, this);
			SET_BIT(m_pointsInstant.dwImmuneFlag, GetWear(i)->GetImmuneFlag());
		}
	}

#ifdef __SKIN_SYSTEM__
	for (int i = 0; i < 2; ++i) // only pet and mount skin
	{
		LPITEM pItem = GetWear(i + WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_RESERVED_MAX_NUM + SHINING_MAX_NUM + SHINING_RESERVED);
		if (pItem)
		{
			pItem->ModifyPoints(true, this);
			SET_BIT(m_pointsInstant.dwImmuneFlag, pItem->GetImmuneFlag());
		}
	}
#endif

#ifdef __DRAGONSOUL__
	if (DragonSoul_IsDeckActivated())
	{
#ifdef ENABLE_DS_SET_BONUS
		short countLevelSix = 0;
#endif
		for (int i = WEAR_MAX_NUM + DS_SLOT_MAX * DragonSoul_GetActiveDeck();
			i < WEAR_MAX_NUM + DS_SLOT_MAX * (DragonSoul_GetActiveDeck() + 1); i++)
		{
			LPITEM pItem = GetWear(i);
			if (pItem)
			{
#ifdef ENABLE_DS_SET_BONUS
				if (DSManager::instance().IsTimeLeftDragonSoul(pItem))
				{
					pItem->ModifyPoints(true);
					DWORD refineLevel = (pItem->GetVnum() / 10) % 10;
					if (refineLevel == 6)
						++countLevelSix;
				}
#else
				if (DSManager::instance().IsTimeLeftDragonSoul(pItem))
					pItem->ModifyPoints(true);
#endif
			}
		}
#ifdef ENABLE_DS_SET_BONUS
		if (countLevelSix == 6)
		{
#ifdef ELONIA
			PointChange(POINT_MAX_HP, 1500);
			PointChange(POINT_ATTBONUS_HUMAN, 5);
			PointChange(POINT_ATTBONUS_MONSTER, 5);
#else
			PointChange(POINT_MAX_HP, 2500);
			PointChange(POINT_ATTBONUS_HUMAN, 10);
			PointChange(POINT_ATTBONUS_METIN, 10);
#endif
		}
#endif
	}
#endif

	_exec_events(EEventTypes::COMPUTE_POINTS, false);

	ComputeSkillPoints();

	RefreshAffect();
#ifdef __PET_SYSTEM__
	CPetSystem* pPetSystem = GetPetSystem();
#ifdef __FAKE_PC__
	if (FakePC_Check())
		pPetSystem = FakePC_GetOwner()->GetPetSystem();
#endif
	if (NULL != pPetSystem)
	{
		pPetSystem->RefreshBuff(this);
	}
#endif

#ifdef __ATTRTREE__
	GiveAttrTreeBonus();
#endif

	TMapBuffOnAttrs* pBuffOnAttrMap = &m_map_buff_on_attrs;
#ifdef __FAKE_PC__
	if (FakePC_Check())
		pBuffOnAttrMap = &FakePC_GetOwner()->m_map_buff_on_attrs;
#endif
	for (TMapBuffOnAttrs::iterator it = pBuffOnAttrMap->begin(); it != pBuffOnAttrMap->end(); it++)
	{
		it->second->GiveAllAttributes(this);
	}

#ifdef ENABLE_RUNE_SYSTEM
	if (IsPC())
	{
		GiveRunePageBuff();
		if (m_runeEvent)
		{
			if (!GetPoint(POINT_RUNE_OUT_OF_COMBAT_SPEED))
			{
				long remain = event_time(m_runeEvent);
				event_cancel(&m_runeEvent);
				if (remain)
					StartRuneEvent(false, remain);
			}
		}
		else if (GetPoint(POINT_RUNE_OUT_OF_COMBAT_SPEED))
		{
			m_runeData.bonusMowSpeed = 0;
			StartRuneEvent(false, 1);
		}
		GiveRunePermaBonus(true);
	}
#endif

#ifdef ENABLE_COMPANION_NAME
	if (IsPC())
	{
		if (m_petNameTimeLeft > get_global_time() && GetPetSystem() && GetPetSystem()->GetSummoned())
			PointChange(POINT_ATTBONUS_MONSTER, 10);

		if (m_mountNameTimeLeft > get_global_time() && IsRiding())
			PointChange(POINT_ATTBONUS_METIN, 10);
	}
#endif

	CheckMaximumPoints();
	UpdatePacket();

#ifdef __FAKE_PC__
	if (IsPC())
		FakePC_Owner_ExecFunc(&CHARACTER::ComputePoints);
#endif
}

// m_dwPlayStartTimeÀÇ ´ÜÀ§´Â milisecond´Ù. µ¥ÀÌÅÍº£ÀÌ½º¿¡´Â ºÐ´ÜÀ§·Î ±â·ÏÇÏ±â
// ¶§¹®¿¡ ÇÃ·¹ÀÌ½Ã°£À» °è»êÇÒ ¶§ / 60000 À¸·Î ³ª´²¼­ ÇÏ´Âµ¥, ±× ³ª¸ÓÁö °ªÀÌ ³²¾Ò
// À» ¶§ ¿©±â¿¡ dwTimeRemainÀ¸·Î ³Ö¾î¼­ Á¦´ë·Î °è»êµÇµµ·Ï ÇØÁÖ¾î¾ß ÇÑ´Ù.
void CHARACTER::ResetPlayTime(DWORD dwTimeRemain)
{
	m_dwPlayStartTime = get_dword_time() - dwTimeRemain;
}

const int aiRecoveryPercents[10] = { 1, 5, 5, 5, 5, 5, 5, 5, 5, 5 };

EVENTFUNC(recovery_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "recovery_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}	

	if (!ch->IsPC())
	{
		//
		// ¸ó½ºÅÍ È¸º¹
		//

		if (ch->IsAffectFlag(AFF_POISON))
			return PASSES_PER_SEC(MAX(1, ch->GetMobTable().regen_cycle()));

		if (ch->GetHP() >= ch->GetMaxHP())
		{
			if (2493 == ch->GetMobTable().vnum() && test_server)
				sys_err("BERAN HP RETURN0 ; %d %d", ch->GetHP(), ch->GetMaxHP());
			ch->m_pkRecoveryEvent = NULL;
			return 0;
		}

		if (2493 == ch->GetMobTable().vnum())
		{
			// regen pct
			int regenPct = BlueDragon_GetRangeFactor("hp_regen", ch->GetHPPct());
			regenPct += ch->GetMobTable().regen_percent();

			for (int i=1 ; i <= 4 ; ++i)
			{
				if (test_server)
					sys_err("BlueDragon_GetIndexFactor(DragonStone, i, effect_type) %d,", BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"));

				if (REGEN_PECT_BONUS == BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"))
				{
					DWORD dwDragonStoneID = BlueDragon_GetIndexFactor("DragonStone", i, "vnum");
					size_t val = BlueDragon_GetIndexFactor("DragonStone", i, "val");
					size_t cnt = SECTREE_MANAGER::instance().GetMonsterCountInMap( ch->GetMapIndex(), dwDragonStoneID );

					regenPct += (val*cnt);
					if (test_server)
						sys_err("DRAGON_DUNGEON;2 %i ,%i", dwDragonStoneID, regenPct);
					break;
				}
			}

			if (regenPct)
				ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * regenPct) / 100));

			// regen time
			for (int i=1 ; i <= 4 ; ++i)
			{
				if (test_server)
					sys_err("BlueDragon_GetIndexFactor222(DragonStone, i, effect_type) %d,", BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"));

				if (REGEN_TIME_BONUS == BlueDragon_GetIndexFactor("DragonStone", i, "effect_type"))
				{
					DWORD dwDragonStoneID = BlueDragon_GetIndexFactor("DragonStone", i, "vnum");
					size_t val = BlueDragon_GetIndexFactor("DragonStone", i, "val");
					size_t cnt = SECTREE_MANAGER::instance().GetMonsterCountInMap( ch->GetMapIndex(), dwDragonStoneID );

					if (test_server)
						sys_err("DRAGON_DUNGEON  : i %d vnum %d val %d cnt %d sec %d", i, dwDragonStoneID, val, cnt, MAX(1, (ch->GetMobTable().regen_cycle() - (val*cnt))));

					return PASSES_PER_SEC(MAX(1, (ch->GetMobTable().regen_cycle() - (val*cnt))));
				}
			}
		}
		else if (!ch->IsDoor() && ch->GetMobTable().regen_percent())
		{
			if (test_server)
				sys_log(1, "HP_REGEN__:[%d] %d / ", ch->GetRaceNum(), ch->GetMobTable().regen_percent());
			ch->MonsterLog("HP_REGEN +%d", MAX(1, (ch->GetMaxHP() * ch->GetMobTable().regen_percent()) / 100));
			ch->PointChange(POINT_HP, MAX(1, (ch->GetMaxHP() * ch->GetMobTable().regen_percent()) / 100));
		}

		return PASSES_PER_SEC(MAX(1, ch->GetMobTable().regen_cycle()));
	}
	else
	{
		//
		// PC È¸º¹
		//
		ch->CheckTarget();
		//ch->UpdateSectree(); // ¿©±â¼­ ÀÌ°É ¿ÖÇÏÁö?
		ch->UpdateKillerMode();

		if (ch->IsAffectFlag(AFF_POISON) == true)
		{
			// Áßµ¶ÀÎ °æ¿ì ÀÚµ¿È¸º¹ ±ÝÁö
			// ÆÄ¹ý¼úÀÎ °æ¿ì ÀÚµ¿È¸º¹ ±ÝÁö
			return 3;
		}

		int iSec = (get_dword_time() - ch->GetLastMoveTime()) / 3000;

		// SP È¸º¹ ·çÆ¾.
		// ¿Ö ÀÌ°É·Î ÇØ¼­ ÇÔ¼ö·Î »©³ù´Â°¡ ?!
		ch->DistributeSP(ch);

		if (ch->GetMaxHP() <= ch->GetHP())
			return PASSES_PER_SEC(3);

		int iPercent = 0;
		int iAmount = 0;
		
		{
			iPercent = aiRecoveryPercents[MIN(9, iSec)];
			iAmount = 15 + (ch->GetMaxHP() * iPercent) / 100;
		}
		
		iAmount += (iAmount * ch->GetPoint(POINT_HP_REGEN)) / 100;

		sys_log(1, "RECOVERY_EVENT: %s %d HP_REGEN %d HP +%d", ch->GetName(), iPercent, ch->GetPoint(POINT_HP_REGEN), iAmount);

		ch->PointChange(POINT_HP, iAmount, false);
		return PASSES_PER_SEC(3);
	}
}

void CHARACTER::StartRecoveryEvent()
{
	if (m_pkRecoveryEvent)
		return;

	if (IsDead() || IsStun())
		return;

	if (IsNPC() && GetHP() >= GetMaxHP()) // ¸ó½ºÅÍ´Â Ã¼·ÂÀÌ ´Ù Â÷ÀÖÀ¸¸é ½ÃÀÛ ¾ÈÇÑ´Ù.
		return;

#ifdef __MELEY_LAIR_DUNGEON__
	if ((MeleyLair::CMgr::instance().IsMeleyMap(GetMapIndex())) && ((GetRaceNum() == (WORD)(MeleyLair::STATUE_VNUM)) || ((GetRaceNum() == (WORD)(MeleyLair::BOSS_VNUM)))))
		return;
#endif

	int iSec = IsPC() ? 3 : (MAX(0, GetMobTable().regen_cycle()));
	if (!iSec)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkRecoveryEvent = event_create(recovery_event, info, PASSES_PER_SEC(iSec));
}

void CHARACTER::SetRotation(float fRot)
{
	m_pointsInstant.fRot = fRot;
}

// x, y ¹æÇâÀ¸·Î º¸°í ¼±´Ù.
void CHARACTER::SetRotationToXY(long x, long y)
{
	SetRotation(GetDegreeFromPositionXY(GetX(), GetY(), x, y));
}

bool CHARACTER::CannotMoveByAffect() const
{
	return (IsAffectFlag(AFF_STUN));
}

bool CHARACTER::CanMove() const
{
	if (CannotMoveByAffect())
		return false;

	if (GetMyShop())	// »óÁ¡ ¿¬ »óÅÂ¿¡¼­´Â ¿òÁ÷ÀÏ ¼ö ¾øÀ½
		return false;

	// 0.2ÃÊ ÀüÀÌ¶ó¸é ¿òÁ÷ÀÏ ¼ö ¾ø´Ù.
	/*
	   if (get_float_time() - m_fSyncTime < 0.2f)
	   return false;
	 */
	return true;
}

// ¹«Á¶°Ç x, y À§Ä¡·Î ÀÌµ¿ ½ÃÅ²´Ù.
bool CHARACTER::Sync(long x, long y)
{
	if (!GetSectree())
		return false;

	LPSECTREE new_tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), x, y);

	if (!new_tree)
	{
		if (GetDesc())
		{
			sys_err("cannot find tree at %d %d (name: %s)", x, y, GetName());
			x = GetX();
			y = GetY();
			new_tree = GetSectree();
			//GetDesc()->SetPhase(PHASE_CLOSE);
		}
		else
		{
			sys_err("no tree: %s %d %d %d", GetName(), x, y, GetMapIndex());
			Dead();
		}

		return false;
	}

	SetRotationToXY(x, y);
	SetXYZ(x, y, 0);

	if (GetDungeon())
	{
		// ´øÁ¯¿ë ÀÌº¥Æ® ¼Ó¼º º¯È­
		int iLastEventAttr = m_iEventAttr;
		m_iEventAttr = new_tree->GetEventAttribute(x, y);

		if (m_iEventAttr != iLastEventAttr)
		{
			if (GetParty())
			{
				quest::CQuestManager::instance().AttrOut(GetParty()->GetLeaderPID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetParty()->GetLeaderPID(), this, m_iEventAttr);
			}
			else
			{
				quest::CQuestManager::instance().AttrOut(GetPlayerID(), this, iLastEventAttr);
				quest::CQuestManager::instance().AttrIn(GetPlayerID(), this, m_iEventAttr);
			}
		}
	}

	if (GetSectree() != new_tree)
	{
		if (!IsNPC())
		{
			SECTREEID id = new_tree->GetID();
			SECTREEID old_id = GetSectree()->GetID();

			sys_log(!test_server, "SECTREE DIFFER: %s %dx%d was %dx%d",
					GetName(),
					id.coord.x,
					id.coord.y,
					old_id.coord.x,
					old_id.coord.y);
		}

		new_tree->InsertEntity(this);
	}

	return true;
}

void CHARACTER::Stop()
{
	if (!IsState(m_stateIdle))
		MonsterLog("[IDLE] Á¤Áö");

	GotoState(m_stateIdle);

	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
}

void CHARACTER::SetMovingWay(const TNPCMovingPosition* pWay, int iMaxNum, bool bRepeat, bool bLocal)
{
	m_pMovingWay = pWay;
	m_iMovingWayIndex = 0;
	m_iMovingWayMaxNum = iMaxNum;
	m_bMovingWayRepeat = bRepeat;
	m_lMovingWayBaseX = 0;
	m_lMovingWayBaseY = 0;

	if (bLocal)
	{
		PIXEL_POSITION pxBase;
		if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(GetMapIndex(), pxBase))
		{
			m_pMovingWay = NULL;
			sys_err("cannot get map base position by index %ld", GetMapIndex());
			return;
		}

		m_lMovingWayBaseX = pxBase.x / 100;
		m_lMovingWayBaseY = pxBase.y / 100;
	}


	if (GetX() / 100 == m_lMovingWayBaseX + m_pMovingWay[0].lX && GetY() / 100 == m_lMovingWayBaseY + m_pMovingWay[0].lY)
		m_iMovingWayIndex++;

	DoMovingWay();
}

bool CHARACTER::DoMovingWay()
{
	if (m_pMovingWay)
	{
		if (m_iMovingWayIndex >= m_iMovingWayMaxNum)
		{
			if (!m_bMovingWayRepeat)
				return false;

			m_iMovingWayIndex = 0;
		}

		long lX = m_pMovingWay[m_iMovingWayIndex].lX + m_lMovingWayBaseX;
		long lY = m_pMovingWay[m_iMovingWayIndex].lY + m_lMovingWayBaseY;

		SetNowWalking(!m_pMovingWay[m_iMovingWayIndex].bRun);
		if (Goto(lX * 100, lY * 100))
			SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

		m_iMovingWayIndex++;

		return true;
	}

	return false;
}

bool CHARACTER::Goto(long x, long y)
{
	// TODO °Å¸®Ã¼Å© ÇÊ¿ä
	// °°Àº À§Ä¡¸é ÀÌµ¿ÇÒ ÇÊ¿ä ¾øÀ½ (ÀÚµ¿ ¼º°ø)
	if (GetX() == x && GetY() == y)
		return false;

	if (m_posDest.x == x && m_posDest.y == y)
	{
		if (!IsState(m_stateMove))
		{
			m_dwStateDuration = 4;
			GotoState(m_stateMove);
		}
		return false;
	}

	m_posDest.x = x;
	m_posDest.y = y;

	CalculateMoveDuration();

	m_dwStateDuration = 4;

	if (test_server && IsAggressive())
		sys_log(0, "CHARACTER::Goto %s (%ld, %ld)", GetName(), x, y);
	
	if (!IsState(m_stateMove))
	{
		MonsterLog("[MOVE] %s", GetVictim() ? "´ë»óÃßÀû" : "±×³ÉÀÌµ¿");

		if (GetVictim())
		{
			//MonsterChat(MONSTER_CHAT_CHASE);
			MonsterChat(MONSTER_CHAT_ATTACK);
		}
	}

	GotoState(m_stateMove);

	return true;
}


DWORD CHARACTER::GetMotionMode() const
{
	DWORD dwMode = MOTION_MODE_GENERAL;

	if (IsPolymorphed())
		return dwMode;

	LPITEM pkItem;

	if ((pkItem = GetWear(WEAR_WEAPON)))
		dwMode = GetMotionModeBySubType(pkItem->GetProto()->sub_type());

	return dwMode;
}

DWORD CHARACTER::GetMotionModeBySubType(BYTE bSubType) const
{
	DWORD dwMode = MOTION_MODE_GENERAL;

	switch (bSubType)
	{
	case WEAPON_SWORD:
		dwMode = MOTION_MODE_ONEHAND_SWORD;
		break;

	case WEAPON_TWO_HANDED:
		dwMode = MOTION_MODE_TWOHAND_SWORD;
		break;

	case WEAPON_DAGGER:
		dwMode = MOTION_MODE_DUALHAND_SWORD;
		break;

	case WEAPON_BOW:
		dwMode = MOTION_MODE_BOW;
		break;

	case WEAPON_BELL:
		dwMode = MOTION_MODE_BELL;
		break;

	case WEAPON_FAN:
		dwMode = MOTION_MODE_FAN;
		break;

#ifdef __WOLFMAN__
	case WEAPON_CLAW:
		dwMode = MOTION_MODE_CLAW;
		break;
#endif
	}

	return dwMode;
}

float CHARACTER::GetMoveMotionSpeed() const
{
	DWORD dwMode = GetMotionMode();

	const CMotion * pkMotion = NULL;

	bool bIsWalking = IsPC() ? IsWalking() : m_bNowWalking;

	if (!GetMountVnum())
		pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(dwMode, bIsWalking ? MOTION_WALK : MOTION_RUN));
	else
	{
		pkMotion = CMotionManager::instance().GetMotion(GetMountVnum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, bIsWalking ? MOTION_WALK : MOTION_RUN));

		if (!pkMotion)
			pkMotion = CMotionManager::instance().GetMotion(GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_HORSE, bIsWalking ? MOTION_WALK : MOTION_RUN));
	}

	if (pkMotion)
		return -pkMotion->GetAccumVector().y / pkMotion->GetDuration();
	else
	{
		if (test_server && (
#ifdef __PET_SYSTEM__
			 !IsPet()
#endif
#ifdef __FAKE_BUFF__
			&& !FakeBuff_Check()
#endif
			))
			sys_err("cannot find motion (name %s race %d mode %d)", GetName(), GetRaceNum(), dwMode);
		return 300.0f;
	}
}

float CHARACTER::GetMoveSpeed() const
{
	return GetMoveMotionSpeed() * 10000 / CalculateDuration(GetLimitPoint(POINT_MOV_SPEED), 10000);
}

void CHARACTER::CalculateMoveDuration()
{
	m_posStart.x = GetX();
	m_posStart.y = GetY();

	float fDist = DISTANCE_SQRT(m_posStart.x - m_posDest.x, m_posStart.y - m_posDest.y);

	float motionSpeed = GetMoveMotionSpeed();

	m_dwMoveDuration = CalculateDuration(GetLimitPoint(POINT_MOV_SPEED),
			(int) ((fDist / motionSpeed) * 1000.0f));

	m_dwMoveStartTime = get_dword_time();
}

bool CHARACTER::Move(long x, long y)
{
	if (GetX() == x && GetY() == y)
		return true;

	OnMove();
	return Sync(x, y);
}

void CHARACTER::SendMovePacket(BYTE bFunc, BYTE bArg, DWORD x, DWORD y, DWORD dwDuration, DWORD dwTime, int iRot)
{
	network::GCOutputPacket<network::GCMovePacket> pack;

	if (bFunc == FUNC_WAIT)
	{
		x = m_posDest.x;
		y = m_posDest.y;
		dwDuration = m_dwMoveDuration;
	}

	EncodeMovePacket(pack, GetVID(), bFunc, bArg, x, y, dwDuration, dwTime, iRot == -1 ? (int)GetRotation() / 5 : iRot);

#ifdef __FAKE_PC__
	if (FakePC_Check())
	{
		if (pack->set_func(= FUNC_ATTACK))
		{
			DWORD dwTime = get_dword_time();

			pack->set_func(FUNC_COMBO);

			BYTE bComboIndex = FakePC_ComputeComboIndex();

			if (GetWear(WEAR_WEAPON) && bComboIndex < 3)
				SetComboSequence(bComboIndex + 1);
			else
				SetComboSequence(0);

			SetValidComboInterval(2000);
			SetLastComboTime(dwTime);

			pack->set_arg(MOTION_COMBO_ATTACK_1 + bComboIndex);
		}
	}
#endif

	PacketView(pack);
}

LONGLONG CHARACTER::GetRealPoint(BYTE type) const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetRealPoint(type);
#endif

#ifdef USER_ATK_SPEED_LIMIT
	if (type == POINT_ATT_SPEED && GetQuestFlag("setting.limit_atkspd"))
		return MIN(m_points.points[type], GetQuestFlag("setting.limit_atkspd"));
#endif

	return m_points.points[type];
}

void CHARACTER::SetRealPoint(BYTE type, LONGLONG val)
{

#ifdef USER_ATK_SPEED_LIMIT
	if (type == POINT_ATT_SPEED && GetQuestFlag("setting.limit_atkspd"))
		val = MIN(val, GetQuestFlag("setting.limit_atkspd"));
#endif

	m_points.points[type] = val;

	network::GCOutputPacket<network::GCRealPointSetPacket> pack;
	pack->set_type(type);
	pack->set_value(val);

	if (GetDesc() && !GetDesc()->IsPhase(PHASE_SELECT))
		GetDesc()->Packet(pack);
}

LONGLONG CHARACTER::GetPolymorphPoint(BYTE type) const
{
	if (IsPolymorphed() && !IsPolyMaintainStat())
	{
		DWORD dwMobVnum = GetPolymorphVnum();
		const CMob * pMob = CMobManager::instance().Get(dwMobVnum);
		int iPower = GetPolymorphPower();

		if (pMob)
		{
			switch (type)
			{
				case POINT_ST:
					if (GetJob() == JOB_SHAMAN || GetJob() == JOB_SURA && GetSkillGroup() == 2)
						return pMob->m_table.str() * iPower / 100 + GetPoint(POINT_IQ);
					return pMob->m_table.str() * iPower / 100 + GetPoint(POINT_ST);

				case POINT_HT:
					return pMob->m_table.con() * iPower / 100 + GetPoint(POINT_HT);

				case POINT_IQ:
					return pMob->m_table.int_() * iPower / 100 + GetPoint(POINT_IQ);

				case POINT_DX:
					return pMob->m_table.dex() * iPower / 100 + GetPoint(POINT_DX);
			}
		}
	}

	return GetPoint(type);
}

int CHARACTER::GetPoint(unsigned char type, bool bGetSum) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	int val = m_pointsInstant.points[type];
	if (bGetSum)
		val += (int)GetPointF(type, false);

#ifdef __GAYA_SYSTEM__
	if (type == POINT_GAYA)
		val = GetGaya();
#endif

#ifdef ENABLE_ZODIAC_TEMPLE
	if (type == POINT_ANIMASPHERE)
		val = GetAnimasphere();
#endif

	int max_val = INT_MAX;

	switch (type)
	{
		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			max_val = 50;
			break;
	}

	if (val > max_val)
	{
		if (type == POINT_STEAL_HP || type == POINT_STEAL_SP)
		{
			val = max_val;
		}
		else
		{
			sys_err("POINT_ERROR1: %s type %d val %d (max: %d)", GetName(), val, max_val);
		}
	}

#ifdef USER_ATK_SPEED_LIMIT
	if (type == POINT_ATT_SPEED && GetQuestFlag("setting.limit_atkspd"))
		val = MIN(val, GetQuestFlag("setting.limit_atkspd"));
#endif

	return (val);
}

LONGLONG CHARACTER::GetLimitPoint(BYTE type) const
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return 0;
	}

	LONGLONG val = m_pointsInstant.points[type];
	LONGLONG max_val = LLONG_MAX;
	LONGLONG limit = LLONG_MAX;
	LONGLONG min_limit = -LLONG_MAX;

	switch (type)
	{
		case POINT_ATT_SPEED:
			min_limit = 0;

			if (IsPC())
#ifdef USER_ATK_SPEED_LIMIT
				if (GetQuestFlag("setting.limit_atkspd"))
					limit = GetQuestFlag("setting.limit_atkspd");
				else
					limit = 170;
#else
				limit = 170;
#endif
			else
				limit = 250;
			break;

		case POINT_MOV_SPEED:
			min_limit = 0;
#ifdef __MOUNT_EXTRA_SPEED__
			if (IsPC() && IsRiding())
				limit = 500;
			else if (IsPC())
#else
			if (IsPC())
#endif
				limit = 200;
			else
				limit = 250;

#ifdef ENABLE_RUNE_SYSTEM
			limit += m_runeData.bonusMowSpeed;
#endif
			break;

		case POINT_STEAL_HP:
		case POINT_STEAL_SP:
			limit = 50;
			max_val = 50;
			break;

		case POINT_MALL_ATTBONUS:
		case POINT_MALL_DEFBONUS:
			limit = 20;
			max_val = 50;
			break;
	}

	if (val > max_val)
		sys_err("POINT_ERROR2: %s type %d val %lld (max: %lld)", GetName(), type, val, max_val);

	if (val > limit)
		val = limit;

	if (val < min_limit)
		val = min_limit;

	return (val);
}

void CHARACTER::SetPoint(BYTE type, LONGLONG val)
{
	if (type >= POINT_MAX_NUM)
	{
		sys_err("Point type overflow (type %u)", type);
		return;
	}

	m_pointsInstant.points[type] = val;

	// ¾ÆÁ÷ ÀÌµ¿ÀÌ ´Ù ¾È³¡³µ´Ù¸é ÀÌµ¿ ½Ã°£ °è»êÀ» ´Ù½Ã ÇØ¾ß ÇÑ´Ù.
	if (type == POINT_MOV_SPEED && get_dword_time() < m_dwMoveStartTime + m_dwMoveDuration)
	{
		CalculateMoveDuration();
	}
}

long long CHARACTER::GetAllowedGold() const
{
	if (GetLevel() <= 10)
		return 100000;
	else if (GetLevel() <= 20)
		return 500000;
	else
		return 50000000;
}

void CHARACTER::CheckMaximumPoints()
{
	if (GetMaxHP() < GetHP())
		PointChange(POINT_HP, GetMaxHP() - GetHP());

	if (GetMaxSP() < GetSP())
		PointChange(POINT_SP, GetMaxSP() - GetSP());
}

void CHARACTER::PointChange(BYTE type, long long amount, bool bAmount, bool bBroadcast)
{
	long long val = 0;

	//sys_log(0, "PointChange %d %d | %d -> %d cHP %d mHP %d", type, amount, GetPoint(type), GetPoint(type)+amount, GetHP(), GetMaxHP());

	switch (type)
	{
		case POINT_NONE:
			return;

		case POINT_LEVEL:
#ifdef __PRESTIGE__
			if ((GetLevel() + amount) > gPlayerMaxLevel[GetPrestigeLevel()])
#else
			if ((GetLevel() + amount) > gPlayerMaxLevel)
#endif
				return;

			SetLevel(GetLevel() + amount);
			val = GetLevel();

#ifdef __PRESTIGE__
			if (val >= gPlayerMaxLevel[GetPrestigeLevel()] && GetExp() > 0)
#else
			if (val >= gPlayerMaxLevel && GetExp() > 0)
#endif
				PointChange(POINT_EXP, -GetExp());

			sys_log(!test_server, "LEVELUP: %s %d NEXT EXP %d", GetName(), GetLevel(), GetNextExp());

#ifdef __WOLFMAN__
			if (GetJob() == JOB_WOLFMAN)
			{
				if (val >= 5)
				{
					if (GetSkillGroup() != 1) {
						SetSkillGroup(1);
						ClearSkill();
					}
				}
				else {
					if (GetSkillGroup() != 1)
						SetSkillGroup(0);
				}
			}
#endif

			PointChange(POINT_NEXT_EXP,	GetNextExp(), false);

			if (amount)
			{
				quest::CQuestManager::instance().LevelUp(GetPlayerID());

				LogManager::instance().LevelLog(this, val, GetRealPoint(POINT_PLAYTIME) + (get_dword_time() - m_dwPlayStartTime) / 60000);

				if (GetGuild())
				{
					GetGuild()->LevelChange(GetPlayerID(), GetLevel());
				}

				if (GetParty())
				{
					GetParty()->RequestSetMemberLevel(GetPlayerID(), GetLevel());
				}
			}
			break;

		case POINT_NEXT_EXP:
			val = GetNextExp();
			bAmount = false;	// ¹«Á¶°Ç bAmount´Â false ¿©¾ß ÇÑ´Ù.
			break;

		case POINT_ANTI_EXP:
			val = IsEXPDisabled();
			break;

#ifdef __GAYA_SYSTEM__
		case POINT_GAYA:
			val = GetGaya();
			break;
#endif

		case POINT_EXP:
			{
				DWORD exp = GetExp();
				DWORD next_exp = GetNextExp();

				// exp°¡ 0 ÀÌÇÏ·Î °¡Áö ¾Êµµ·Ï ÇÑ´Ù
				if (amount < 0 && exp < -amount)
				{
					sys_log(1, "%s AMOUNT < 0 %d, CUR EXP: %d", GetName(), -amount, exp);
					amount = -exp;

					SetExp(exp + amount);
					val = GetExp();
				}
				else
				{
#ifdef __PET_ADVANCED__
					if (GetPetAdvanced())
					{
						long long max_exp_per_mob = static_cast<long long>(ceilf(GetPetAdvanced()->GetNextExp() / 10.0f));
						long long pet_amount = amount;
						if (pet_amount > max_exp_per_mob)
							pet_amount = max_exp_per_mob;

						GetPetAdvanced()->GiveExp(pet_amount);
					}
#endif

#ifdef __PRESTIGE__
					if (gPlayerMaxLevel[GetPrestigeLevel()] <= GetLevel())
#else
					if (gPlayerMaxLevel <= GetLevel())
#endif
						return;

					if (IsEXPDisabled() && amount > 0)
						return;

					tchat("You have gained %d exp.", amount);

					DWORD iExpBalance = 0;

					// ·¹º§ ¾÷!
					if (exp + amount >= next_exp)
					{
						iExpBalance = (exp + amount) - next_exp;
						amount = next_exp - exp;

						SetExp(0);
						exp = next_exp;
					}
					else
					{
						SetExp(exp + amount);
						exp = GetExp();
					}

					DWORD q = DWORD(next_exp / 4.0f);
					int iLevStep = GetRealPoint(POINT_LEVEL_STEP);

					// iLevStepÀÌ 4 ÀÌ»óÀÌ¸é ·¹º§ÀÌ ¿Ã¶ú¾î¾ß ÇÏ¹Ç·Î ¿©±â¿¡ ¿Ã ¼ö ¾ø´Â °ªÀÌ´Ù.
					if (iLevStep >= 4)
					{
						sys_err("%s LEVEL_STEP bigger than 4! (%d)", GetName(), iLevStep);
						iLevStep = 4;
					}

					if (exp >= next_exp && iLevStep < 4)
					{
						for (int i = 0; i < 4 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 3 && iLevStep < 3)
					{
						for (int i = 0; i < 3 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q * 2 && iLevStep < 2)
					{
						for (int i = 0; i < 2 - iLevStep; ++i)
							PointChange(POINT_LEVEL_STEP, 1, false, true);
					}
					else if (exp >= q && iLevStep < 1)
						PointChange(POINT_LEVEL_STEP, 1);

					if (iExpBalance)
					{
						PointChange(POINT_EXP, iExpBalance);
					}

					val = GetExp();
				}
			}
			break;

		case POINT_LEVEL_STEP:
			if (amount > 0)
			{
				val = GetPoint(POINT_LEVEL_STEP) + amount;

				switch (val)
				{
					case 1:
					case 2:
					case 3:
						//if (GetLevel() < 100) PointChange(POINT_STAT, 1);
						if (GetLevel() < 91) PointChange(POINT_STAT, 1);
						break;

					case 4:
						{
							TJobInitialPoints& rkInit = JobInitialPoints[GetJob()];
							int iHP = rkInit.hp_per_lv_begin + (rkInit.hp_per_lv_end - rkInit.hp_per_lv_begin) / 2;
							int iSP = rkInit.sp_per_lv_begin + (rkInit.sp_per_lv_end - rkInit.sp_per_lv_begin) / 2;

							if (GetSkillGroup())
							{
								if (GetLevel() >= 5)
									PointChange(POINT_SKILL, 1);

								if (GetLevel() >= 9)
									PointChange(POINT_SUB_SKILL, 1);
							}

							PointChange(POINT_MAX_HP, iHP);
							PointChange(POINT_MAX_SP, iSP);
							PointChange(POINT_LEVEL, 1, false, true);

							val = 0;
						}
						break;
				}

#ifndef __LEVELUP_NO_POTIONS__
				if (GetLevel() <= 10)
					AutoGiveItem(27001, 2);
				else if (GetLevel() <= 30)
					AutoGiveItem(27002, 2);
				else
				{
					AutoGiveItem(27002, 2);
//					AutoGiveItem(27003, 2);
				}
#endif
				
				PointChange(POINT_HP, GetMaxHP() - GetHP());
				PointChange(POINT_SP, GetMaxSP() - GetSP());
				PointChange(POINT_STAMINA, GetMaxStamina() - GetStamina());

				SetPoint(POINT_LEVEL_STEP, val);
				SetRealPoint(POINT_LEVEL_STEP, val);

				Save();
			}
			else
				val = GetPoint(POINT_LEVEL_STEP);

			break;

		case POINT_HP:
			{
				if (IsDead() || IsStun())
					return;

				int prev_hp = GetHP();

				amount = MIN(GetMaxHP() - GetHP(), amount);
				SetHP(GetHP() + amount);
				val = GetHP();

				BroadcastTargetPacket();

				if (GetParty() && IsPC() && val != prev_hp)
					GetParty()->SendPartyInfoOneToAll(this);
			}
			break;

		case POINT_SP:
			{
				if (IsDead() || IsStun())
					return;

				amount = MIN(GetMaxSP() - GetSP(), amount);
				SetSP(GetSP() + amount);
				val = GetSP();
			}
			break;

		case POINT_STAMINA:
			{
				if (IsDead() || IsStun())
					return;

				int prev_val = GetStamina();
				amount = MIN(GetMaxStamina() - GetStamina(), amount);
				SetStamina(GetStamina() + amount);
				val = GetStamina();
				
#ifdef __USE_STAMINA__
				if (val == 0)
				{
					// Stamina°¡ ¾øÀ¸´Ï °ÈÀÚ!
					SetNowWalking(true);
				}
				else if (prev_val == 0)
				{
					// ¾ø´ø ½ºÅ×¹Ì³ª°¡ »ý°åÀ¸´Ï ÀÌÀü ¸ðµå º¹±Í
					ResetWalking();
				}

				if (amount < 0 && val != 0) // °¨¼Ò´Â º¸³»Áö¾Ê´Â´Ù.
					return;
#else
				if (val < GetMaxStamina() / 2 || prev_val == 0)
				{
					SetStamina(GetMaxStamina());
					ResetWalking();
				}
#endif
			}
			break;

		case POINT_MAX_HP:
			{
				SetPoint(type, GetPoint(type, false) + amount);

				int hp = GetRealPoint(POINT_MAX_HP);

// #ifdef defined(__HP_CALC_OFFICIAL__)
				int add_hp = MIN(3500, hp * GetPoint(POINT_MAX_HP_PCT) / 100);
				add_hp += GetPoint(POINT_MAX_HP);
				add_hp += GetPoint(POINT_PARTY_TANKER_BONUS);
				SetMaxHP(hp + add_hp);
// #elif defined(__HP_CALC_AE__)
				// int add_hp = 0;
				// add_hp += GetPoint(POINT_MAX_HP);
				// add_hp += GetPoint(POINT_PARTY_TANKER_BONUS);
				// SetMaxHP((hp + add_hp) * (100 + GetPoint(POINT_MAX_HP_PCT)) / 100);
// #elif defined(__HP_CALC_NEW_HALFCUT__)
				// int add_hp = MIN(3500, hp * GetPoint(POINT_MAX_HP_PCT) / 100);
				// add_hp += GetPoint(POINT_MAX_HP);
				// add_hp += GetPoint(POINT_PARTY_TANKER_BONUS);
				// SetMaxHP(hp + (add_hp * (100 + (GetPoint(POINT_MAX_HP_PCT) * 0.5)) / 100));
// #endif
				val = GetMaxHP();
			}
			break;

		case POINT_MAX_SP:
			{
				SetPoint(type, GetPoint(type, false) + amount);

				//SetMaxSP(GetMaxSP() + amount);
				// ÃÖ´ë Á¤½Å·Â = (±âº» ÃÖ´ë Á¤½Å·Â + Ãß°¡) * ÃÖ´ëÁ¤½Å·Â%
				int sp = GetRealPoint(POINT_MAX_SP);
				int add_sp = 0;// MIN(800, sp * GetPoint(POINT_MAX_SP_PCT) / 100);
				add_sp += GetPoint(POINT_MAX_SP);
				add_sp += GetPoint(POINT_PARTY_SKILL_MASTER_BONUS);

				SetMaxSP((sp + add_sp) * (100 + GetPoint(POINT_MAX_SP_PCT)) / 100);

				val = GetMaxSP();
			}
			break;

		case POINT_MAX_HP_PCT:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);

			PointChange(POINT_MAX_HP, 0);
			break;

		case POINT_MAX_SP_PCT:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);

			PointChange(POINT_MAX_SP, 0);
			break;

		case POINT_MAX_STAMINA:
			SetMaxStamina(GetMaxStamina() + amount);
			val = GetMaxStamina();
			break;

		case POINT_GOLD:
			{
				const int64_t nTotalMoney = static_cast<int64_t>(GetGold()) + static_cast<int64_t>(amount);

				if (GOLD_MAX * 2 < nTotalMoney) // GOLD_MAX * 2 to secure if somehow you can receive more gold than the limit the gold won't disappear
				{
					sys_err("[OVERFLOW_GOLD] OriGold %d AddedGold %d id %u Name %s ", GetGold(), amount, GetPlayerID(), GetName());
					LogManager::instance().CharLog(this, GetGold() + amount, "OVERFLOW_GOLD", "");
					return;
				}

				SetGold(GetGold() + amount);
				val = GetGold();
			}
			break;
#ifdef ENABLE_ZODIAC_TEMPLE
		case POINT_ANIMASPHERE:
			{
				const int16_t nTotalGem = static_cast<int16_t>(GetAnimasphere()) + static_cast<int16_t>(amount);

				if (ANIMASPHERE_MAX <= nTotalGem)
				{
					sys_err("[OVERFLOW_GEM] OriGem %d AddedGem %d id %u Name %s ", GetAnimasphere(), amount, GetPlayerID(), GetName());
					LogManager::instance().CharLog(this, GetAnimasphere() + amount, "OVERFLOW_GEM", "");
					return;
				}

				SetAnimasphere(GetAnimasphere() + amount);
				val = GetAnimasphere();
			}
			break;
#endif
		case POINT_MOUNT_BUFF_BONUS:
			{
				bool bGiveBuff = true;
				if (GetMountSystem() && GetMountSystem()->IsRiding())
				{
					LPITEM pkSummonItem = ITEM_MANAGER::instance().Find(GetMountSystem()->GetSummonItemID());
					if (!pkSummonItem || pkSummonItem->GetOwner() != this)
						bGiveBuff = false;
				}

				if (GetMountSystem() && GetMountSystem()->IsRiding() && bGiveBuff)
					GetMountSystem()->GiveBuff(false);

				SetPoint(type, GetPoint(type, false) + amount);

				if (GetMountSystem())
				{
					GetMountSystem()->RefreshMountBuffBonus();
					if (GetMountSystem()->IsRiding() && bGiveBuff)
						GetMountSystem()->GiveBuff(true);
				}

				val = GetPointF(type);
			}
			break;

		case POINT_SKILL:
		case POINT_STAT:
		case POINT_SUB_SKILL:
		case POINT_STAT_RESET_COUNT:
		case POINT_HORSE_SKILL:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);

			SetRealPoint(type, val);
			break;

		case POINT_DEF_GRADE:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);

			PointChange(POINT_CLIENT_DEF_GRADE, amount);
			break;

		case POINT_CLIENT_DEF_GRADE:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			break;
#ifdef ENABLE_RUNE_SYSTEM
		case POINT_RUNE_MOVEMENT_SLOW_PCT:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			PointChange(POINT_MOV_SPEED, 0);
			break;
#endif
		case POINT_MOV_SPEED:
#ifdef COMBAT_ZONE
			if (FindAffect(AFFECT_COMBAT_ZONE_MOVEMENT))
			{
				SetPoint(type, COMBAT_ZONE_REDUCTION_MAX_MOVEMENT_SPEED);
				val = GetPoint(type);
				break;
			}
#endif
#ifdef ENABLE_RUNE_SYSTEM
			m_runeData.storedMovementSpeed += amount;
			SetPoint(type, m_runeData.storedMovementSpeed * (100 - GetPoint(POINT_RUNE_MOVEMENT_SLOW_PCT)) / 100);
#else
			SetPoint(type, GetPoint(type, false) + amount);
#endif
			val = GetPoint(type);
			break;

#ifdef ENABLE_RUNE_SYSTEM
		case POINT_RUNE_ATTACK_SLOW_PCT:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			PointChange(POINT_ATT_SPEED, 0);
			break;
#endif

		case POINT_ATT_SPEED:
#ifdef ENABLE_RUNE_SYSTEM
			m_runeData.storedAttackSpeed += amount;
			SetPoint(type, m_runeData.storedAttackSpeed * (100 - GetPoint(POINT_RUNE_ATTACK_SLOW_PCT)) / 100);
#else
			SetPoint(type, GetPoint(type, false) + amount);
#endif
			val = GetPoint(type);
			break;

		case POINT_ST:
		case POINT_HT:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HP_REGEN:
		case POINT_SP_REGEN:
		case POINT_ATT_GRADE:
		case POINT_CASTING_SPEED:
		case POINT_MAGIC_ATT_GRADE:
		case POINT_MAGIC_DEF_GRADE:
		case POINT_BOW_DISTANCE:
		case POINT_HP_RECOVERY:
		case POINT_SP_RECOVERY:

		case POINT_ATTBONUS_HUMAN:	// 42 ÀÎ°£¿¡°Ô °­ÇÔ
		case POINT_ATTBONUS_ANIMAL:	// 43 µ¿¹°¿¡°Ô µ¥¹ÌÁö % Áõ°¡
		case POINT_ATTBONUS_ORC:		// 44 ¿õ±Í¿¡°Ô µ¥¹ÌÁö % Áõ°¡
#ifdef ENABLE_ZODIAC_TEMPLE
		case POINT_ATTBONUS_ZODIAC:
#endif
		case POINT_BONUS_UPGRADE_CHANCE:
		case POINT_LOWER_DUNGEON_CD:
		case POINT_LOWER_BIOLOG_CD:
		case POINT_ATTBONUS_MILGYO:	// 45 ¹Ð±³¿¡°Ô µ¥¹ÌÁö % Áõ°¡
		case POINT_ATTBONUS_UNDEAD:	// 46 ½ÃÃ¼¿¡°Ô µ¥¹ÌÁö % Áõ°¡
		case POINT_ATTBONUS_DEVIL:	// 47 ¸¶±Í(¾Ç¸¶)¿¡°Ô µ¥¹ÌÁö % Áõ°¡

		case POINT_ATTBONUS_MONSTER:
		case POINT_RESIST_MONSTER:
		case POINT_RESIST_BOSS:
		case POINT_RESIST_HUMAN:
		case POINT_ATTBONUS_METIN:
		case POINT_ATTBONUS_BOSS:
		case POINT_ATTBONUS_SURA:
		case POINT_ATTBONUS_ASSASSIN:
		case POINT_ATTBONUS_WARRIOR:
		case POINT_ATTBONUS_SHAMAN:
#ifdef __WOLFMAN__
		case POINT_ATTBONUS_WOLFMAN:
		case POINT_BLEEDING_PCT:
		case POINT_BLEEDING_REDUCE:
#endif

		case POINT_POISON_PCT:
		case POINT_STUN_PCT:
		case POINT_SLOW_PCT:

		case POINT_BLOCK:
		case POINT_DODGE:

		case POINT_CRITICAL_PCT:
		case POINT_RESIST_CRITICAL:
		case POINT_PENETRATE_PCT:
		case POINT_RESIST_PENETRATE:
		case POINT_CURSE_PCT:

		case POINT_STEAL_HP:		// 48 »ý¸í·Â Èí¼ö
		case POINT_STEAL_SP:		// 49 Á¤½Å·Â Èí¼ö

		case POINT_MANA_BURN_PCT:	// 50 ¸¶³ª ¹ø
		case POINT_DAMAGE_SP_RECOVER:	// 51 °ø°Ý´çÇÒ ½Ã Á¤½Å·Â È¸º¹ È®·ü
		case POINT_RESIST_NORMAL_DAMAGE:
		case POINT_RESIST_SWORD:
		case POINT_RESIST_TWOHAND:
		case POINT_RESIST_DAGGER:
		case POINT_RESIST_BELL: 
		case POINT_RESIST_FAN: 
		case POINT_RESIST_BOW:
		case POINT_RESIST_FIRE:
		case POINT_RESIST_ELEC:
		case POINT_RESIST_MAGIC:
		case POINT_ANTI_RESIST_MAGIC:
		case POINT_RESIST_WIND:
		case POINT_RESIST_ICE:
		case POINT_RESIST_EARTH:
		case POINT_RESIST_DARK:
		case POINT_REFLECT_MELEE:	// 67 °ø°Ý ¹Ý»ç
		case POINT_REFLECT_CURSE:	// 68 ÀúÁÖ ¹Ý»ç
		case POINT_POISON_REDUCE:	// 69 µ¶µ¥¹ÌÁö °¨¼Ò
		case POINT_KILL_SP_RECOVER:	// 70 Àû ¼Ò¸ê½Ã MP È¸º¹
		case POINT_KILL_HP_RECOVERY:	// 75  
		case POINT_HIT_HP_RECOVERY:
		case POINT_HIT_SP_RECOVERY:
		case POINT_MANASHIELD:
		case POINT_ATT_BONUS:
		case POINT_DEF_BONUS:
		case POINT_SKILL_DAMAGE_BONUS:
		case POINT_NORMAL_HIT_DAMAGE_BONUS:
		case POINT_EXP_REAL_BONUS:

			// DEPEND_BONUS_ATTRIBUTES 
		case POINT_SKILL_DEFEND_BONUS:
		case POINT_NORMAL_HIT_DEFEND_BONUS:
		case POINT_BLOCK_IGNORE_BONUS:
#ifdef __ELEMENT_SYSTEM__
		case POINT_ATTBONUS_ELEC:
		case POINT_ATTBONUS_FIRE:
		case POINT_ATTBONUS_ICE:
		case POINT_ATTBONUS_WIND:
		case POINT_ATTBONUS_EARTH:
		case POINT_ATTBONUS_DARK:
#endif
#ifdef ENABLE_RUNE_SYSTEM
		case POINT_RUNE_SHIELD_PER_HIT:
		case POINT_RUNE_HEAL_ON_KILL:
		case POINT_RUNE_BONUS_DAMAGE_AFTER_HIT:
		case POINT_RUNE_3RD_ATTACK_BONUS:
		case POINT_RUNE_FIRST_NORMAL_HIT_BONUS:
		case POINT_RUNE_MSHIELD_PER_SKILL:
		case POINT_RUNE_HARVEST:
		case POINT_RUNE_DAMAGE_AFTER_3:
		case POINT_RUNE_OUT_OF_COMBAT_SPEED:
		case POINT_RUNE_RESET_SKILL:
		case POINT_RUNE_COMBAT_CASTING_SPEED:
		case POINT_RUNE_MAGIC_DAMAGE_AFTER_HIT:
		case POINT_RUNE_MOVSPEED_AFTER_3:
		case POINT_RUNE_SLOW_ON_ATTACK:
		case POINT_RUNE_MOUNT_PARALYZE:
		case POINT_RUNE_LEADERSHIP_BONUS:
#ifdef RUNE_CRITICAL_POINT
		case POINT_RUNE_CRITICAL_PVM:
#endif
#endif
		case POINT_ATTBONUS_ALL_ELEMENTS:
		case POINT_HEAL_EFFECT_BONUS:
		case POINT_CRITICAL_DAMAGE_BONUS:
		case POINT_DOUBLE_ITEM_DROP_BONUS:
		case POINT_DAMAGE_BY_SP_BONUS:
		case POINT_SINGLETARGET_SKILL_DAMAGE_BONUS:
		case POINT_MULTITARGET_SKILL_DAMAGE_BONUS:
		case POINT_EQUIP_SKILL_BONUS:

		case POINT_ATTBONUS_MONSTER_DIV10:
			
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			break;
			// END_OF_DEPEND_BONUS_ATTRIBUTES

		case POINT_MIXED_DEFEND_BONUS:
			PointChange(POINT_SKILL_DEFEND_BONUS, amount);
			PointChange(POINT_NORMAL_HIT_DEFEND_BONUS, amount);
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			break;

		case POINT_AURA_HEAL_EFFECT_BONUS:
		case POINT_AURA_EQUIP_SKILL_BONUS:
		{
			if (!amount)
				return;

			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);

			UpdateAuraByPoint(type);
		}
		break;

		case POINT_PARTY_ATTACKER_BONUS:
		case POINT_PARTY_TANKER_BONUS:
		case POINT_PARTY_BUFFER_BONUS:
#ifdef STANDARD_SKILL_DURATION
		case POINT_SKILL_DURATION:
#endif
		case POINT_PARTY_SKILL_MASTER_BONUS:
		case POINT_PARTY_HASTE_BONUS:
		case POINT_PARTY_DEFENDER_BONUS:

		case POINT_RESIST_WARRIOR :
		case POINT_RESIST_ASSASSIN :
		case POINT_RESIST_SURA :
		case POINT_RESIST_SHAMAN :
#ifdef __WOLFMAN__
		case POINT_RESIST_WOLFMAN:
		case POINT_RESIST_CLAW:
#endif
		case POINT_RESIST_SWORD_PEN:
		case POINT_RESIST_TWOHAND_PEN:
		case POINT_RESIST_DAGGER_PEN:
		case POINT_RESIST_BELL_PEN:
		case POINT_RESIST_FAN_PEN:
		case POINT_RESIST_BOW_PEN:
		case POINT_RESIST_ATTBONUS_HUMAN:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			break;

		case POINT_MALL_ATTBONUS:
		case POINT_MALL_DEFBONUS:
		case POINT_MALL_EXPBONUS:
		case POINT_MALL_ITEMBONUS:
		case POINT_MALL_GOLDBONUS:
		case POINT_MELEE_MAGIC_ATT_BONUS_PER:
#ifdef ELONIA
			if (GetPoint(type, false) + amount > ((type == POINT_MALL_EXPBONUS) ? 200 : 100))
#else
			if (GetPoint(type, false) + amount > 100)
#endif
			{
				if (test_server)
					sys_err("MALL_BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
				amount = 100 - GetPoint(type, false);
			}

			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			break;

		case POINT_EXP_DOUBLE_BONUS:	// 71  
		case POINT_GOLD_DOUBLE_BONUS:	// 72  
		case POINT_ITEM_DROP_BONUS:	// 73  
		case POINT_POTION_BONUS:	// 74
			if (GetPoint(type, false) + amount > 100)
			{
				sys_err("BONUS exceeded over 100!! point type: %d name: %s amount %d", type, GetName(), amount);
				amount = 100 - GetPoint(type, false);
			}

			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			break;

		case POINT_IMMUNE_STUN:		// 76 
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_STUN);
			}
			break;

		case POINT_IMMUNE_SLOW:		// 77  
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_SLOW);
			}
			break;

		case POINT_IMMUNE_FALL:	// 78   
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			if (val)
			{
				SET_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
			}
			else
			{
				REMOVE_BIT(m_pointsInstant.dwImmuneFlag, IMMUNE_FALL);
			}
			break;

		case POINT_ATT_GRADE_BONUS:
			SetPoint(type, GetPoint(type, false) + amount);
			PointChange(POINT_ATT_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_DEF_GRADE_BONUS:
			SetPoint(type, GetPoint(type, false) + amount);
			PointChange(POINT_DEF_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_MAGIC_ATT_GRADE_BONUS:
			SetPoint(type, GetPoint(type, false) + amount);
			PointChange(POINT_MAGIC_ATT_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_MAGIC_DEF_GRADE_BONUS:
			SetPoint(type, GetPoint(type, false) + amount);
			PointChange(POINT_MAGIC_DEF_GRADE, amount);
			val = GetPoint(type);
			break;

		case POINT_VOICE:
		case POINT_EMPIRE_POINT:
			//sys_err("CHARACTER::PointChange: %s: point cannot be changed. use SetPoint instead (type: %d)", GetName(), type);
			val = GetRealPoint(type);
			break;

		case POINT_POLYMORPH:
			tchat("ch:%d  || %d  ||  %d", g_bChannel, thecore_heart->pulse - (int)GetLastShoutPulse(), passes_per_sec * 15);
			if(g_bChannel == 99 && thecore_heart->pulse - (int)GetLastShoutPulse() < passes_per_sec * 15)
			{
				tchat("5 Sec delay");
				SetPoint(type, GetPoint(type, false) + amount);
				m_dwPolymorphEventRace = GetPoint(type);
				StartPolymorphEvent();
				break;
			}
			tchat("no delay");
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			SetPolymorph(val);

			break;

		case POINT_MOUNT:
			SetPoint(type, GetPoint(type, false) + amount);
			val = GetPoint(type);
			MountVnum(val);
			break;

		case POINT_ENERGY:
		case POINT_COSTUME_ATTR_BONUS:
			{
				int old_val = GetPoint(type);
				SetPoint(type, GetPoint(type, false) + amount);
				val = GetPoint(type);
				BuffOnAttr_ValueChange(type, old_val, val);
			}
			break;

		default:
			sys_err("CHARACTER::PointChange: %s: unknown point change type %d", GetName(), type);
			return;
	}

	switch (type)
	{
		case POINT_LEVEL:
		case POINT_ST:
		case POINT_DX:
		case POINT_IQ:
		case POINT_HT:
			ComputeBattlePoints();
			break;
		case POINT_MAX_HP:
		case POINT_MAX_SP:
		case POINT_MAX_STAMINA:
			break;
	}

	if (type == POINT_HP && amount == 0)
		return;

	if (GetDesc())
	{
		network::GCOutputPacket<network::GCPointChangePacket> pack;

		pack->set_vid(m_vid);
		pack->set_type(type);
		pack->set_value(val);

		if (bAmount)
			pack->set_amount(amount);

		if (!bBroadcast)
			GetDesc()->Packet(pack);
		else
			PacketAround(pack);
	}
}

void CHARACTER::ApplyPoint(BYTE bApplyType, int iVal)
{
	// if (test_server)
		// ChatPacket(CHAT_TYPE_INFO, "ApplyPoint(%d, %d)", bApplyType, iVal);

	switch (bApplyType)
	{
		case APPLY_NONE:			// 0
			break;

		case APPLY_CON:
			PointChange(POINT_HT, iVal);
			PointChange(POINT_MAX_HP, (iVal * JobInitialPoints[GetJob()].hp_per_ht));
			PointChange(POINT_MAX_STAMINA, (iVal * JobInitialPoints[GetJob()].stamina_per_con));
			break;

		case APPLY_INT: 
			PointChange(POINT_IQ, iVal);
			PointChange(POINT_MAX_SP, (iVal * JobInitialPoints[GetJob()].sp_per_iq));
			break;

		case APPLY_SKILL:
			// SKILL_DAMAGE_BONUS
			{
				// ÃÖ»óÀ§ ºñÆ® ±âÁØÀ¸·Î 8ºñÆ® vnum, 9ºñÆ® add, 15ºñÆ® change
				// 00000000 00000000 00000000 00000000
				// ^^^^^^^^  ^^^^^^^^^^^^^^^^^^^^^^^^^
				// vnum	 ^ add	   change
				BYTE bSkillVnum = (BYTE) (((DWORD)iVal) >> 24);
				int iAdd = iVal & 0x00800000;
				int iChange = iVal & 0x007fffff;

				sys_log(1, "APPLY_SKILL skill %d add? %d change %d", bSkillVnum, iAdd ? 1 : 0, iChange);

				if (0 == iAdd)
					iChange = -iChange;

				std::unordered_map<BYTE, int>::iterator iter = m_SkillDamageBonus.find(bSkillVnum);

				if (iter == m_SkillDamageBonus.end())
					m_SkillDamageBonus.insert(std::make_pair(bSkillVnum, iChange));
				else
					iter->second += iChange;
			}
			// END_OF_SKILL_DAMAGE_BONUS
			break;

		case APPLY_STR:
		case APPLY_DEX:
		case APPLY_MAX_HP:
		case APPLY_MAX_SP:
		case APPLY_MAX_HP_PCT:
		case APPLY_MAX_SP_PCT:
		case APPLY_ATT_SPEED:
		case APPLY_MOV_SPEED:
		case APPLY_CAST_SPEED:
		case APPLY_HP_REGEN:
		case APPLY_SP_REGEN:
		case APPLY_POISON_PCT:
		case APPLY_STUN_PCT:
		case APPLY_SLOW_PCT:
		case APPLY_CRITICAL_PCT:
		case APPLY_PENETRATE_PCT:
		case APPLY_ATTBONUS_HUMAN:
		case APPLY_ATTBONUS_ANIMAL:
		case APPLY_ATTBONUS_ORC:
#ifdef ENABLE_ZODIAC_TEMPLE
		case APPLY_ATTBONUS_ZODIAC:
#endif
		case APPLY_BONUS_UPGRADE_CHANCE:
		case APPLY_LOWER_DUNGEON_CD:
		case APPLY_LOWER_BIOLOG_CD:
		case APPLY_ATTBONUS_MILGYO:
		case APPLY_ATTBONUS_UNDEAD:
		case APPLY_ATTBONUS_DEVIL:
		case APPLY_ATTBONUS_WARRIOR:	// 59
		case APPLY_ATTBONUS_ASSASSIN:	// 60
		case APPLY_ATTBONUS_SURA:	// 61
		case APPLY_ATTBONUS_SHAMAN:	// 62
		case APPLY_ATTBONUS_MONSTER:	// 63
		case APPLY_RESIST_MONSTER:
		case APPLY_RESIST_BOSS:
		case APPLY_RESIST_HUMAN:
		case APPLY_ATTBONUS_BOSS:
		case APPLY_ATTBONUS_METIN:
		case APPLY_STEAL_HP:
		case APPLY_STEAL_SP:
		case APPLY_MANA_BURN_PCT:
		case APPLY_DAMAGE_SP_RECOVER:
		case APPLY_BLOCK:
		case APPLY_DODGE:
		case APPLY_RESIST_SWORD:
		case APPLY_RESIST_TWOHAND:
		case APPLY_RESIST_DAGGER:
		case APPLY_RESIST_BELL:
		case APPLY_RESIST_FAN:
		case APPLY_RESIST_BOW:
		case APPLY_RESIST_FIRE:
		case APPLY_RESIST_ELEC:
		case APPLY_RESIST_MAGIC:
		case APPLY_ANTI_RESIST_MAGIC:
		case APPLY_RESIST_WIND:
		case APPLY_RESIST_ICE:
		case APPLY_RESIST_EARTH:
		case APPLY_RESIST_DARK:
		case APPLY_REFLECT_MELEE:
		case APPLY_REFLECT_CURSE:
		case APPLY_ANTI_CRITICAL_PCT:
		case APPLY_ANTI_PENETRATE_PCT:
		case APPLY_POISON_REDUCE:
		case APPLY_KILL_SP_RECOVER:
		case APPLY_EXP_DOUBLE_BONUS:
		case APPLY_EXP_REAL_BONUS:
		case APPLY_GOLD_DOUBLE_BONUS:
		case APPLY_ITEM_DROP_BONUS:
		case APPLY_POTION_BONUS:
		case APPLY_KILL_HP_RECOVER:
		case APPLY_IMMUNE_STUN:	
		case APPLY_IMMUNE_SLOW:	
		case APPLY_IMMUNE_FALL:	
		case APPLY_BOW_DISTANCE:
		case APPLY_ATT_GRADE_BONUS:
		case APPLY_DEF_GRADE_BONUS:
		case APPLY_MAGIC_ATT_GRADE:
		case APPLY_MAGIC_DEF_GRADE:
		case APPLY_CURSE_PCT:
		case APPLY_MAX_STAMINA:
		case APPLY_MALL_ATTBONUS:
		case APPLY_MALL_DEFBONUS:
		case APPLY_MALL_EXPBONUS:
		case APPLY_MALL_ITEMBONUS:
		case APPLY_MALL_GOLDBONUS:
		case APPLY_SKILL_DAMAGE_BONUS:
		case APPLY_NORMAL_HIT_DAMAGE_BONUS:

			// DEPEND_BONUS_ATTRIBUTES
		case APPLY_SKILL_DEFEND_BONUS:
		case APPLY_NORMAL_HIT_DEFEND_BONUS:
			// END_OF_DEPEND_BONUS_ATTRIBUTES

		case APPLY_RESIST_WARRIOR :
		case APPLY_RESIST_ASSASSIN :
		case APPLY_RESIST_SURA :
		case APPLY_RESIST_SHAMAN :	
		case APPLY_ENERGY:					// 82 ±â·Â
		case APPLY_DEF_GRADE:				// 83 ¹æ¾î·Â. DEF_GRADE_BONUS´Â Å¬¶ó¿¡¼­ µÎ¹è·Î º¸¿©Áö´Â ÀÇµµµÈ ¹ö±×(...)°¡ ÀÖ´Ù.
		case APPLY_COSTUME_ATTR_BONUS:		// 84 ÄÚ½ºÆ¬ ¾ÆÀÌÅÛ¿¡ ºÙÀº ¼Ó¼ºÄ¡ º¸³Ê½º
		case APPLY_MAGIC_ATTBONUS_PER:		// 85 ¸¶¹ý °ø°Ý·Â +x%
		case APPLY_MELEE_MAGIC_ATTBONUS_PER:			// 86 ¸¶¹ý + ¹Ð¸® °ø°Ý·Â +x%

#ifdef __WOLFMAN__
		case APPLY_ATTBONUS_WOLFMAN:
		case APPLY_BLEEDING_REDUCE:
		case APPLY_BLEEDING_PCT:
		case APPLY_RESIST_WOLFMAN:
		case APPLY_RESIST_CLAW:
#endif

#ifdef __ACCE_COSTUME__
		case APPLY_ACCEDRAIN_RATE:
#endif

#ifdef __ANIMAL_SYSTEM__
#ifdef __PET_SYSTEM__
		case APPLY_PET_EXP_BONUS:
#endif
		case APPLY_MOUNT_EXP_BONUS:
#endif
		case APPLY_MOUNT_BUFF_BONUS:
		case APPLY_RESIST_SWORD_PEN:
		case APPLY_RESIST_TWOHAND_PEN:
		case APPLY_RESIST_DAGGER_PEN:
		case APPLY_RESIST_BELL_PEN:
		case APPLY_RESIST_FAN_PEN:
		case APPLY_RESIST_BOW_PEN:
		case APPLY_RESIST_ATTBONUS_HUMAN:
#ifdef __ELEMENT_SYSTEM__
		case APPLY_ATTBONUS_ELEC:
		case APPLY_ATTBONUS_FIRE:
		case APPLY_ATTBONUS_ICE:
		case APPLY_ATTBONUS_WIND:
		case APPLY_ATTBONUS_EARTH:
		case APPLY_ATTBONUS_DARK:
#endif
		case APPLY_DEFENSE_BONUS:
		case APPLY_BLOCK_IGNORE_BONUS:
		case APPLY_ATTBONUS_ALL_ELEMENTS:
#ifdef ENABLE_RUNE_SYSTEM
		case APPLY_RUNE_SHIELD_PER_HIT:
		case APPLY_RUNE_HEAL_ON_KILL:
		case APPLY_RUNE_BONUS_DAMAGE_AFTER_HIT:
		case APPLY_RUNE_3RD_ATTACK_BONUS:
		case APPLY_RUNE_FIRST_NORMAL_HIT_BONUS:
		case APPLY_RUNE_MSHIELD_PER_SKILL:
		case APPLY_RUNE_HARVEST:
		case APPLY_RUNE_DAMAGE_AFTER_3:
		case APPLY_RUNE_OUT_OF_COMBAT_SPEED:
		case APPLY_RUNE_RESET_SKILL:
		case APPLY_RUNE_COMBAT_CASTING_SPEED:
		case APPLY_RUNE_MAGIC_DAMAGE_AFTER_HIT:
		case APPLY_RUNE_MOVSPEED_AFTER_3:
		case APPLY_RUNE_SLOW_ON_ATTACK:
		case APPLY_RUNE_MOUNT_PARALYZE:
		case APPLY_RUNE_LEADERSHIP_BONUS:
#ifdef RUNE_CRITICAL_POINT
		case APPLY_RUNE_CRITICAL_PVM:
#endif
#endif
		case APPLY_HEAL_EFFECT_BONUS:
		case APPLY_CRITICAL_DAMAGE_BONUS:
		case APPLY_DOUBLE_ITEM_DROP_BONUS:
		case APPLY_DAMAGE_BY_SP_BONUS:
		case APPLY_SINGLETARGET_SKILL_DAMAGE_BONUS:
		case APPLY_MULTITARGET_SKILL_DAMAGE_BONUS:
		case APPLY_MIXED_DEFEND_BONUS:
		case APPLY_EQUIP_SKILL_BONUS:
		case APPLY_RESIST_MAGIC_REDUCTION:
#ifdef STANDARD_SKILL_DURATION
		case APPLY_SKILL_DURATION:
#endif
			PointChange(aApplyInfo[bApplyType].bPointType, iVal);
			break;

		default:
			sys_err("Unknown apply type %d name %s", bApplyType, GetName());
			break;
	}
}

void CHARACTER::MotionPacketEncode(BYTE motion, LPCHARACTER victim, network::GCOutputPacket<network::GCMotionPacket>& packet)
{
	packet->set_vid(m_vid);
	packet->set_motion(motion);

	if (victim)
		packet->set_victim_vid(victim->GetVID());
}

void CHARACTER::Motion(BYTE motion, LPCHARACTER victim)
{
	network::GCOutputPacket<network::GCMotionPacket> pack_motion;
	MotionPacketEncode(motion, victim, pack_motion);
	PacketAround(pack_motion);
}

EVENTFUNC(save_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "save_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}	
	sys_log(1, "SAVE_EVENT: %s", ch->GetName());
	ch->Save();
	ch->FlushDelayedSaveItem();
	return (save_event_second_cycle);
}

void CHARACTER::StartSaveEvent()
{
	if (m_pkSaveEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkSaveEvent = event_create(save_event, info, save_event_second_cycle);
}

void CHARACTER::MonsterLog(const char* format, ...)
{
	if (!test_server)
		return;

	if (IsPC())
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	int len = snprintf(chatbuf, sizeof(chatbuf), "%u)", (DWORD)GetVID());

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	va_list args;

	va_start(args, format);

	int len2 = vsnprintf(chatbuf + len, sizeof(chatbuf) - len, format, args);

	if (len2 < 0 || len2 >= (int) sizeof(chatbuf) - len)
		len += (sizeof(chatbuf) - len) - 1;
	else
		len += len2;

	// \0 ¹®ÀÚ Æ÷ÇÔ
	++len;

	va_end(args);

	network::GCOutputPacket<network::GCChatPacket> pack_chat;

	pack_chat->set_type(CHAT_TYPE_TALKING);
	pack_chat->set_id((DWORD)GetVID());
	pack_chat->set_empire(0);
	pack_chat->set_message(chatbuf);

	CHARACTER_MANAGER::instance().PacketMonsterLog(this, pack_chat);
}

void CHARACTER::ChatPacket(BYTE type, const char * format, ...)
{
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	network::GCOutputPacket<network::GCChatPacket> pack_chat;

	pack_chat->set_type(type);
	pack_chat->set_id(0);
	pack_chat->set_empire(d->GetEmpire());
	pack_chat->set_message(chatbuf);

	d->Packet(pack_chat);

	if (type == CHAT_TYPE_COMMAND && test_server)
		sys_log(0, "SEND_COMMAND %s %s", GetName(), chatbuf);
}

void CHARACTER::tchat(const char * format, ...)
{
	if (!test_server)
		return;
	
	LPDESC d = GetDesc();

	if (!d || !format)
		return;

	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	network::GCOutputPacket<network::GCChatPacket> pack_chat;

	pack_chat->set_type(CHAT_TYPE_PARTY);
	pack_chat->set_id(0);
	pack_chat->set_empire(d->GetEmpire());
	pack_chat->set_message(chatbuf);

	d->Packet(pack_chat);
}

// MINING
void CHARACTER::mining_take()
{
	m_pkMiningEvent = NULL;
}

void CHARACTER::mining_cancel()
{
	if (m_pkMiningEvent)
	{
		sys_log(0, "XXX MINING CANCEL");
		event_cancel(&m_pkMiningEvent);
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¤±¤À» Áß´ÜÇÏ¿´½À´Ï´Ù."));
	}
}

void CHARACTER::mining(LPCHARACTER chLoad)
{
	if (m_pkMiningEvent)
	{
		mining_cancel();
		return;
	}

	if (!chLoad)
		return;

	if (mining::GetRawOreFromLoad(chLoad->GetRaceNum()) == 0)
		return;

	if (GetMapIndex() != chLoad->GetMapIndex() || DISTANCE_APPROX(GetX() - chLoad->GetX(), GetY() - chLoad->GetY()) > 1000)
		return;

	LPITEM pick = GetWear(WEAR_WEAPON);

	if (!pick || pick->GetType() != ITEM_PICK)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°î±ªÀÌ¸¦ ÀåÂøÇÏ¼¼¿ä."));
		return;
	}

	int count = random_number(5, 15); // µ¿ÀÛ È½¼ö, ÇÑ µ¿ÀÛ´ç 2ÃÊ

	// Ã¤±¤ µ¿ÀÛÀ» º¸¿©ÁÜ
	network::GCOutputPacket<network::GCDigMotionPacket> p;
	p->set_vid(GetVID());
	p->set_target_vid(chLoad->GetVID());
	p->set_count(count);

	PacketAround(p);

	m_pkMiningEvent = mining::CreateMiningEvent(this, chLoad, count);
}
// END_OF_MINING

void CHARACTER::fishing()
{
	if (m_pkFishingEvent)
	{
		fishing_take();
		return;
	}
	
	// ¸ø°¨ ¼Ó¼º¿¡¼­ ³¬½Ã¸¦ ½ÃµµÇÑ´Ù?
	{
		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

		int	x = GetX();
		int y = GetY();

		LPSECTREE tree = pkSectreeMap->Find(x, y);
		DWORD dwAttr = tree->GetAttribute(x, y);

		if (IS_SET(dwAttr, ATTR_BLOCK))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "³¬½Ã¸¦ ÇÒ ¼ö ÀÖ´Â °÷ÀÌ ¾Æ´Õ´Ï´Ù"));
			return;
		}
	}

	LPITEM rod = GetWear(WEAR_WEAPON);

	// ³¬½Ã´ë ÀåÂø
	if (!rod || rod->GetType() != ITEM_ROD)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "³¬½Ã´ë¸¦ ÀåÂø ÇÏ¼¼¿ä."));
		return;
	}

	if (0 == rod->GetSocket(2))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¹Ì³¢¸¦ ³¢°í ´øÁ® ÁÖ¼¼¿ä."));
		return;
	}

	if (!IsNearWater())
		return;

	float fx, fy;
	GetDeltaByDegree(GetRotation(), 400.0f, &fx, &fy);

	m_pkFishingEvent = fishing::CreateFishingEvent(this);
}

void CHARACTER::fishing_take()
{
	LPITEM rod = GetWear(WEAR_WEAPON);
	if (rod && rod->GetType() == ITEM_ROD)
	{
		using fishing::fishing_event_info;
		if (m_pkFishingEvent)
		{
			struct fishing_event_info* info = dynamic_cast<struct fishing_event_info*>(m_pkFishingEvent->info);

			if (info)
				fishing::Take(info, this);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "³¬½Ã´ë°¡ ¾Æ´Ñ ¹°°ÇÀ¸·Î ³¬½Ã¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù!"));
	}

	event_cancel(&m_pkFishingEvent);
}

bool CHARACTER::StartStateMachine(int iNextPulse)
{
	if (CHARACTER_MANAGER::instance().AddToStateList(this))
	{
		m_dwNextStatePulse = thecore_heart->pulse + iNextPulse;
		return true;
	}

	return false;
}

void CHARACTER::StopStateMachine()
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return;
#endif

	CHARACTER_MANAGER::instance().RemoveFromStateList(this);
}

void CHARACTER::UpdateStateMachine(DWORD dwPulse)
{
	if (dwPulse < m_dwNextStatePulse)
		return;

	if (IsDead())
		return;

	Update();
	m_dwNextStatePulse = dwPulse + m_dwStateDuration;
}

void CHARACTER::SetNextStatePulse(int iNextPulse)
{
	CHARACTER_MANAGER::instance().AddToStateList(this);
	m_dwNextStatePulse = iNextPulse;

	if (iNextPulse < 10)
		MonsterLog("´ÙÀ½»óÅÂ·Î¾î¼­°¡ÀÚ");
}


// Ä³¸¯ÅÍ ÀÎ½ºÅÏ½º ¾÷µ¥ÀÌÆ® ÇÔ¼ö.
void CHARACTER::UpdateCharacter(DWORD dwPulse)
{
	CFSM::Update();
}

void CHARACTER::SetShop(LPSHOP pkShop)
{
	if ((m_pkShop = pkShop))
		SET_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP);
	else
	{
		REMOVE_BIT(m_pointsInstant.instant_flag, INSTANT_FLAG_SHOP); 
		SetShopOwner(NULL);
	}
}

void CHARACTER::SetExchange(CExchange * pkExchange)
{
	m_pkExchange = pkExchange;
}

void CHARACTER::SetPart(BYTE bPartPos, DWORD dwVal)
{
	assert(bPartPos < PART_MAX_NUM);
	m_pointsInstant.parts[bPartPos] = dwVal;
}

DWORD CHARACTER::GetPart(BYTE bPartPos) const
{
	assert(bPartPos < PART_MAX_NUM);
	return m_pointsInstant.parts[bPartPos];
}

DWORD CHARACTER::GetOriginalPart(BYTE bPartPos) const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetOriginalPart(bPartPos);
#endif

	switch (bPartPos)
	{
		case PART_MAIN:
			if (!IsPC()) // PC°¡ ¾Æ´Ñ °æ¿ì ÇöÀç ÆÄÆ®¸¦ ±×´ë·Î ¸®ÅÏ
				return GetPart(PART_MAIN);
			else
				return m_pointsInstant.bBasePart;

		case PART_HAIR:
			return GetPart(PART_HAIR);

		case PART_WEAPON:
			if (!IsPC())
				return m_pointsInstant.bBasePart;
			else
				return 0;

#ifdef __ACCE_COSTUME__
		case PART_ACCE:
			return GetPart(PART_ACCE);
#endif

		default:
			return 0;
	}
}

BYTE CHARACTER::GetCharType() const
{
	if (IsPet())
		return CHAR_TYPE_PET;
	return m_bCharType;
}

bool CHARACTER::SetSyncOwner(LPCHARACTER ch, bool bRemoveFromList)
{
	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
		return false;
	// END_OF_TRENT_MONSTER

	if (ch == this)
	{
		sys_err("SetSyncOwner owner == this (%p)", this);
		return false;
	}

	if (!ch)
	{
		if (bRemoveFromList && m_pkChrSyncOwner)
		{
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
		}

		// ¸®½ºÆ®¿¡¼­ Á¦°ÅÇÏÁö ¾Ê´õ¶óµµ Æ÷ÀÎÅÍ´Â NULL·Î ¼ÂÆÃµÇ¾î¾ß ÇÑ´Ù.
		m_pkChrSyncOwner = NULL;
	}
	else
	{
		if (!IsSyncOwner(ch))
			return false;

		// °Å¸®°¡ 200 ÀÌ»óÀÌ¸é SyncOwner°¡ µÉ ¼ö ¾ø´Ù.
		if (DISTANCE_APPROX(GetX() - ch->GetX(), GetY() - ch->GetY()) > 250)
		{
			sys_log(1, "SetSyncOwner distance over than 250 %s %s", GetName(), ch->GetName());

			// SyncOwnerÀÏ °æ¿ì Owner·Î Ç¥½ÃÇÑ´Ù.
			if (m_pkChrSyncOwner == ch)
				return true;

			return false;
		}

		if (m_pkChrSyncOwner != ch)
		{
			if (m_pkChrSyncOwner)
			{
				sys_log(1, "SyncRelease %s %p from %s", GetName(), this, m_pkChrSyncOwner->GetName());
				m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.remove(this);
			}

			m_pkChrSyncOwner = ch;
			m_pkChrSyncOwner->m_kLst_pkChrSyncOwned.push_back(this);

			// SyncOwner°¡ ¹Ù²î¸é LastSyncTimeÀ» ÃÊ±âÈ­ÇÑ´Ù.
			static const timeval zero_tv = {0, 0};
			SetLastSyncTime(zero_tv);

			sys_log(1, "SetSyncOwner set %s %p to %s", GetName(), this, ch->GetName());
		}

		m_fSyncTime = get_float_time();
	}

	// TODO: Sync Owner°¡ °°´õ¶óµµ °è¼Ó ÆÐÅ¶À» º¸³»°í ÀÖÀ¸¹Ç·Î,
	//	   µ¿±âÈ­ µÈ ½Ã°£ÀÌ 3ÃÊ ÀÌ»ó Áö³µÀ» ¶§ Ç®¾îÁÖ´Â ÆÐÅ¶À»
	//	   º¸³»´Â ¹æ½ÄÀ¸·Î ÇÏ¸é ÆÐÅ¶À» ÁÙÀÏ ¼ö ÀÖ´Ù.
	network::GCOutputPacket<network::GCOwnershipPacket> pack;

	pack->set_owner_vid(ch ? ch->GetVID() : 0);
	pack->set_victim_vid(GetVID());

	PacketAround(pack);
	return true;
}

struct FuncClearSync
{
	void operator () (LPCHARACTER ch)
	{
		assert(ch != NULL);
		ch->SetSyncOwner(NULL, false);	// false ÇÃ·¡±×·Î ÇØ¾ß for_each °¡ Á¦´ë·Î µ·´Ù.
	}
};

void CHARACTER::ClearSync()
{
	SetSyncOwner(NULL);

	// ¾Æ·¡ for_each¿¡¼­ ³ª¸¦ m_pkChrSyncOwner·Î °¡Áø ÀÚµéÀÇ Æ÷ÀÎÅÍ¸¦ NULL·Î ÇÑ´Ù.
	std::for_each(m_kLst_pkChrSyncOwned.begin(), m_kLst_pkChrSyncOwned.end(), FuncClearSync());
	m_kLst_pkChrSyncOwned.clear();
}

bool CHARACTER::IsSyncOwner(LPCHARACTER ch) const
{
	if (m_pkChrSyncOwner == ch)
		return true;

	// ¸¶Áö¸·À¸·Î µ¿±âÈ­ µÈ ½Ã°£ÀÌ 3ÃÊ ÀÌ»ó Áö³µ´Ù¸é ¼ÒÀ¯±ÇÀÌ ¾Æ¹«¿¡°Ôµµ
	// ¾ø´Ù. µû¶ó¼­ ¾Æ¹«³ª SyncOwnerÀÌ¹Ç·Î true ¸®ÅÏ
	if (get_float_time() - m_fSyncTime >= 3.0f)
		return true;

	return false;
}

void CHARACTER::SetParty(LPPARTY pkParty)
{
	if (pkParty == m_pkParty)
		return;

	if (pkParty && m_pkParty)
		sys_err("%s is trying to reassigning party (current %p, new party %p)", GetName(), get_pointer(m_pkParty), get_pointer(pkParty));

	sys_log(1, "PARTY set to %p", get_pointer(pkParty));

	if (m_pkDungeon && IsPC() && !pkParty)
		SetDungeon(NULL);
	
	m_pkParty = pkParty;

	if (IsPC())
	{
		if (m_pkParty)
			SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);
		else
			REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_PARTY);

		UpdatePacket();
	}
}

// PARTY_JOIN_BUG_FIX
/// ÆÄÆ¼ °¡ÀÔ event Á¤º¸
EVENTINFO(TPartyJoinEventInfo)
{
	DWORD	dwGuestPID;		///< ÆÄÆ¼¿¡ Âü¿©ÇÒ Ä³¸¯ÅÍÀÇ PID
	DWORD	dwLeaderPID;		///< ÆÄÆ¼ ¸®´õÀÇ PID

	TPartyJoinEventInfo() 
	: dwGuestPID( 0 )
	, dwLeaderPID( 0 )
	{
	}
} ;

EVENTFUNC(party_request_event)
{
	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(  event->info );

	if ( info == NULL )
	{
		sys_err( "party_request_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->dwGuestPID);

	if (ch)
	{
		sys_log(0, "PartyRequestEvent %s", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
		ch->SetPartyRequestEvent(NULL);
	}

	return 0;
}

bool CHARACTER::RequestToParty(LPCHARACTER leader)
{
	if (leader->GetParty())
		leader = leader->GetParty()->GetLeaderCharacter();

	if (!leader)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÆÄÆ¼ÀåÀÌ Á¢¼Ó »óÅÂ°¡ ¾Æ´Ï¶ó¼­ ¿äÃ»À» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (m_pkPartyRequestEvent)
		return false; 

	if (!IsPC() || !leader->IsPC())
		return false;

	if (this == leader)
		return false;

	if (leader->IsBlockMode(BLOCK_PARTY_REQUEST))
		return false;

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return false;
#endif

	PartyJoinErrCode errcode = IsPartyJoinableCondition(leader, this);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;

		case PERR_DIFFEMPIRE:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´Ù¸¥ Á¦±¹°ú ÆÄÆ¼¸¦ ÀÌ·ê ¼ö ¾ø½À´Ï´Ù."));
			return false;

		case PERR_DUNGEON:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù.")); 
			return false;

		case PERR_OBSERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> °üÀü ¸ðµå¿¡¼± ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù.")); 
			return false;

		case PERR_LVBOUNDARY:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> -30 ~ +30 ·¹º§ ÀÌ³»ÀÇ »ó´ë¹æ¸¸ ÃÊ´ëÇÒ ¼ö ÀÖ½À´Ï´Ù.")); 
			return false;

		case PERR_LOWLEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖ°í ·¹º§ º¸´Ù 30·¹º§ÀÌ ³·¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;

		case PERR_HILEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖÀú ·¹º§ º¸´Ù 30·¹º§ÀÌ ³ô¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù.")); 
			return false;

		case PERR_ALREADYJOIN: 	
			return false;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´õ ÀÌ»ó ÆÄÆ¼¿øÀ» ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù.")); 
			return false;

		default:
			sys_err("Do not process party join error(%d)", errcode); 
			return false;
	}

	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = GetPlayerID();
	info->dwLeaderPID = leader->GetPlayerID();

	SetPartyRequestEvent(event_create(party_request_event, info, PASSES_PER_SEC(10)));

	leader->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequest %u", (DWORD) GetVID());
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%s ´Ô¿¡°Ô ÆÄÆ¼°¡ÀÔ ½ÅÃ»À» Çß½À´Ï´Ù."), leader->GetName());
	return true;
}

void CHARACTER::DenyToParty(LPCHARACTER member)
{
	sys_log(1, "DenyToParty %s member %s %p", GetName(), member->GetName(), get_pointer(member->m_pkPartyRequestEvent));

	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err( "CHARACTER::DenyToParty> <Factor> Null pointer" );
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

void CHARACTER::AcceptToParty(LPCHARACTER member)
{
	sys_log(1, "AcceptToParty %s member %s %p", GetName(), member->GetName(), get_pointer(member->m_pkPartyRequestEvent));

	if (!member->m_pkPartyRequestEvent)
		return;

	TPartyJoinEventInfo * info = dynamic_cast<TPartyJoinEventInfo *>(member->m_pkPartyRequestEvent->info);

	if (!info)
	{
		sys_err( "CHARACTER::AcceptToParty> <Factor> Null pointer" );
		return;
	}

	if (info->dwGuestPID != member->GetPlayerID())
		return;

	if (info->dwLeaderPID != GetPlayerID())
		return;

	event_cancel(&member->m_pkPartyRequestEvent);

	if (!GetParty())
		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "»ó´ë¹æÀÌ ÆÄÆ¼¿¡ ¼ÓÇØÀÖÁö ¾Ê½À´Ï´Ù."));
	else 
	{
		if (GetPlayerID() != GetParty()->GetLeaderPID())
			return;

		PartyJoinErrCode errcode = IsPartyJoinableCondition(this, member);
		switch (errcode) 
		{
			case PERR_NONE: 		member->PartyJoin(this); return;
			case PERR_SERVER:		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù.")); break;
			case PERR_DUNGEON:		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù.")); break;
			case PERR_OBSERVER: 	member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> °üÀü ¸ðµå¿¡¼± ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù.")); break;
			case PERR_LVBOUNDARY:	member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> -30 ~ +30 ·¹º§ ÀÌ³»ÀÇ »ó´ë¹æ¸¸ ÃÊ´ëÇÒ ¼ö ÀÖ½À´Ï´Ù.")); break;
			case PERR_LOWLEVEL: 	member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖ°í ·¹º§ º¸´Ù 30·¹º§ÀÌ ³·¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù.")); break;
			case PERR_HILEVEL: 		member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖÀú ·¹º§ º¸´Ù 30·¹º§ÀÌ ³ô¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù.")); break;
			case PERR_ALREADYJOIN: 	break;
			case PERR_PARTYISFULL: {
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´õ ÀÌ»ó ÆÄÆ¼¿øÀ» ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
									   member->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(member, "<ÆÄÆ¼> ÆÄÆ¼ÀÇ ÀÎ¿øÁ¦ÇÑÀÌ ÃÊ°úÇÏ¿© ÆÄÆ¼¿¡ Âü°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									   break;
								   }
			default: sys_err("Do not process party join error(%d)", errcode);
		}
	}

	member->ChatPacket(CHAT_TYPE_COMMAND, "PartyRequestDenied");
}

/**
 * ÆÄÆ¼ ÃÊ´ë event callback ÇÔ¼ö.
 * event °¡ ¹ßµ¿ÇÏ¸é ÃÊ´ë °ÅÀý·Î Ã³¸®ÇÑ´Ù.
 */
EVENTFUNC(party_invite_event)
{
	TPartyJoinEventInfo * pInfo = dynamic_cast<TPartyJoinEventInfo *>(  event->info );

	if ( pInfo == NULL )
	{
		sys_err( "party_invite_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER pchInviter = CHARACTER_MANAGER::instance().FindByPID(pInfo->dwLeaderPID);

	if (pchInviter)
	{
		sys_log(1, "PartyInviteEvent %s", pchInviter->GetName());
		pchInviter->PartyInviteDeny(pInfo->dwGuestPID);
	}

	return 0;
}

void CHARACTER::PartyInvite(LPCHARACTER pchInvitee)
{
#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif
	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÆÄÆ¼¿øÀ» ÃÊ´ëÇÒ ¼ö ÀÖ´Â ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
		return;
	}
	else if (pchInvitee->IsBlockMode(BLOCK_PARTY_INVITE) || pchInvitee == this)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> %s ´ÔÀÌ ÆÄÆ¼ °ÅºÎ »óÅÂÀÔ´Ï´Ù."), pchInvitee->GetName());
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableCondition(this, pchInvitee);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_DIFFEMPIRE:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´Ù¸¥ Á¦±¹°ú ÆÄÆ¼¸¦ ÀÌ·ê ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_DUNGEON:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_OBSERVER:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> °üÀü ¸ðµå¿¡¼± ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_LVBOUNDARY:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> -30 ~ +30 ·¹º§ ÀÌ³»ÀÇ »ó´ë¹æ¸¸ ÃÊ´ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
			return;

		case PERR_LOWLEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖ°í ·¹º§ º¸´Ù 30·¹º§ÀÌ ³·¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_HILEVEL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖÀú ·¹º§ º¸´Ù 30·¹º§ÀÌ ³ô¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_ALREADYJOIN:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÀÌ¹Ì %s´ÔÀº ÆÄÆ¼¿¡ ¼ÓÇØ ÀÖ½À´Ï´Ù."), pchInvitee->GetName());
			return;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´õ ÀÌ»ó ÆÄÆ¼¿øÀ» ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		default:
			sys_err("Do not process party join error(%d)", errcode);
			return;
	}

	if (m_PartyInviteEventMap.end() != m_PartyInviteEventMap.find(pchInvitee->GetPlayerID()))
		return;

	//
	// EventMap ¿¡ ÀÌº¥Æ® Ãß°¡
	// 
	TPartyJoinEventInfo* info = AllocEventInfo<TPartyJoinEventInfo>();

	info->dwGuestPID = pchInvitee->GetPlayerID();
	info->dwLeaderPID = GetPlayerID();

	m_PartyInviteEventMap.insert(EventMap::value_type(pchInvitee->GetPlayerID(), event_create(party_invite_event, info, PASSES_PER_SEC(10))));

	//
	// ÃÊ´ë ¹Þ´Â character ¿¡°Ô ÃÊ´ë ÆÐÅ¶ Àü¼Û
	// 

	network::GCOutputPacket<network::GCPartyInvitePacket> p;
	p->set_leader_vid(GetVID());
	pchInvitee->GetDesc()->Packet(p);
}

void CHARACTER::PartyInviteAccept(LPCHARACTER pchInvitee)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(pchInvitee->GetPlayerID());

	if (itFind == m_PartyInviteEventMap.end())
	{
		sys_log(1, "PartyInviteAccept from not invited character(%s)", pchInvitee->GetName());
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	if (GetParty() && GetParty()->GetLeaderPID() != GetPlayerID())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ÆÄÆ¼¿øÀ» ÃÊ´ëÇÒ ¼ö ÀÖ´Â ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
		return;
	}

	PartyJoinErrCode errcode = IsPartyJoinableMutableCondition(this, pchInvitee);

	switch (errcode)
	{
		case PERR_NONE:
			break;

		case PERR_SERVER:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_DUNGEON:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼ ÃÊ´ë¿¡ ÀÀÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_OBSERVER:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> °üÀü ¸ðµå¿¡¼± ÆÄÆ¼ ÃÊ´ë¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_LVBOUNDARY:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> -30 ~ +30 ·¹º§ ÀÌ³»ÀÇ »ó´ë¹æ¸¸ ÃÊ´ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
			return;

		case PERR_LOWLEVEL:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖ°í ·¹º§ º¸´Ù 30·¹º§ÀÌ ³·¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_HILEVEL:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> ÆÄÆ¼³» ÃÖÀú ·¹º§ º¸´Ù 30·¹º§ÀÌ ³ô¾Æ ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_ALREADYJOIN:
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> ÆÄÆ¼ ÃÊ´ë¿¡ ÀÀÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		case PERR_PARTYISFULL:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> ´õ ÀÌ»ó ÆÄÆ¼¿øÀ» ÃÊ´ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pchInvitee, "<ÆÄÆ¼> ÆÄÆ¼ÀÇ ÀÎ¿øÁ¦ÇÑÀÌ ÃÊ°úÇÏ¿© ÆÄÆ¼¿¡ Âü°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;

		default:
			sys_err("ignore party join error(%d)", errcode);
			return;
	}

	//
	// ÆÄÆ¼ °¡ÀÔ Ã³¸®
	// 

	if (GetParty())
		pchInvitee->PartyJoin(this);
	else
	{
		LPPARTY pParty = CPartyManager::instance().CreateParty(this);

		pParty->Join(pchInvitee->GetPlayerID());
		pParty->Link(pchInvitee);
		pParty->SendPartyInfoAllToOne(this);
	}
}

void CHARACTER::PartyInviteDeny(DWORD dwPID)
{
	EventMap::iterator itFind = m_PartyInviteEventMap.find(dwPID);

	if (itFind == m_PartyInviteEventMap.end())
	{
		sys_log(1, "PartyInviteDeny to not exist event(inviter PID: %d, invitee PID: %d)", GetPlayerID(), dwPID);
		return;
	}

	event_cancel(&itFind->second);
	m_PartyInviteEventMap.erase(itFind);

	LPCHARACTER pchInvitee = CHARACTER_MANAGER::instance().FindByPID(dwPID);
	if (pchInvitee)
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> %s´ÔÀÌ ÆÄÆ¼ ÃÊ´ë¸¦ °ÅÀýÇÏ¼Ì½À´Ï´Ù."), pchInvitee->GetName());
}

void CHARACTER::PartyJoin(LPCHARACTER pLeader)
{
	pLeader->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pLeader, "<ÆÄÆ¼> %s´ÔÀÌ ÆÄÆ¼¿¡ Âü°¡ÇÏ¼Ì½À´Ï´Ù."), GetName());
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<ÆÄÆ¼> %s´ÔÀÇ ÆÄÆ¼¿¡ Âü°¡ÇÏ¼Ì½À´Ï´Ù."), pLeader->GetName());

	pLeader->GetParty()->Join(GetPlayerID());
	pLeader->GetParty()->Link(this);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (pchLeader->GetEmpire() != pchGuest->GetEmpire())
		return PERR_DIFFEMPIRE;

	return IsPartyJoinableMutableCondition(pchLeader, pchGuest);
}

static bool __party_can_join_by_level(LPCHARACTER leader, LPCHARACTER quest)
{
	int	level_limit = 30;

	return (abs(leader->GetLevel() - quest->GetLevel()) <= level_limit);
}

CHARACTER::PartyJoinErrCode CHARACTER::IsPartyJoinableMutableCondition(const LPCHARACTER pchLeader, const LPCHARACTER pchGuest)
{
	if (!CPartyManager::instance().IsEnablePCParty())
		return PERR_SERVER;
	else if (pchLeader->GetDungeon())
		return PERR_DUNGEON;
	else if (pchGuest->IsObserverMode())
		return PERR_OBSERVER;
	else if (false == __party_can_join_by_level(pchLeader, pchGuest))
		return PERR_LVBOUNDARY;
	else if (pchGuest->GetParty())
		return PERR_ALREADYJOIN;
	else if (pchLeader->GetParty())
   	{
	   	if (pchLeader->GetParty()->GetMemberCount() == PARTY_MAX_MEMBER)
			return PERR_PARTYISFULL;
	}

	return PERR_NONE;
}
// END_OF_PARTY_JOIN_BUG_FIX

void CHARACTER::SetDungeon(LPDUNGEON pkDungeon)
{
	if (pkDungeon && m_pkDungeon)
		sys_err("%s is trying to reassigning dungeon (current %p, new party %p)", GetName(), get_pointer(m_pkDungeon), get_pointer(pkDungeon));

	if (m_pkDungeon == pkDungeon) {
		return;
	}

	if (m_pkDungeon)
	{
		if (IsPC())
		{
			if (GetParty())
				m_pkDungeon->DecPartyMember(GetParty(), this);
			else
				m_pkDungeon->DecMember(this);
		}
		else if (IsEnemy())
		{
			m_pkDungeon->DecMonster();
			if (!IsDead())
				m_pkDungeon->DecAliveMonster();
		}
	}

	m_pkDungeon = pkDungeon;

	if (pkDungeon)
	{
		if (IsPC())
		{
			sys_log(0, "%s DUNGEON set to %p, PARTY is %p", GetName(), get_pointer(pkDungeon), get_pointer(m_pkParty));
			if (GetParty())
				m_pkDungeon->IncPartyMember(GetParty(), this);
			else
				m_pkDungeon->IncMember(this);
		}
		else if (IsEnemy())
		{
			m_pkDungeon->IncMonster();
		}
	}
}

void CHARACTER::SetWarMap(CWarMap * pWarMap)
{
	if (m_pWarMap)
		m_pWarMap->DecMember(this);

	m_pWarMap = pWarMap;

	if (m_pWarMap)
		m_pWarMap->IncMember(this);
}

void CHARACTER::SetWeddingMap(marriage::WeddingMap* pMap)
{
	if (m_pWeddingMap)
		m_pWeddingMap->DecMember(this);

	m_pWeddingMap = pMap;

	if (m_pWeddingMap)
		m_pWeddingMap->IncMember(this);
}

void CHARACTER::SetRegen(LPREGEN pkRegen)
{
	m_pkRegen = pkRegen;
	if (pkRegen != NULL) {
		regen_id_ = pkRegen->id;
	}
	m_fRegenAngle = GetRotation();
	m_posRegen = GetXYZ();
}

bool CHARACTER::OnIdle()
{
	return false;
}

void CHARACTER::OnMove(bool bIsAttack, bool bPVP)
{
	m_dwLastMoveTime = get_dword_time();

#ifdef __FAKE_PC__
	if (IsPC())
		FakePC_Owner_ResetAfkEvent();
#endif

#ifdef __FAKE_BUFF__
	if (FakeBuff_Owner_GetSpawn() && DISTANCE_APPROX(GetX() - FakeBuff_Owner_GetSpawn()->GetX(), GetY() - FakeBuff_Owner_GetSpawn()->GetY()) > 4000)
	{
		int x = GetX() + random_number(50, 150) * (random_number(0, 1) ? -1 : 1);
		int y = GetY() + random_number(50, 150) * (random_number(0, 1) ? -1 : 1);
		FakeBuff_Owner_GetSpawn()->Show(GetMapIndex(), x, y);
		return;
	}
#endif

	if (bPVP)
		m_dwLastAttackPVPTime = m_dwLastMoveTime;

	if (bIsAttack)
	{
		m_dwLastAttackTime = m_dwLastMoveTime;

		if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
			RemoveAffect(AFFECT_REVIVE_INVISIBLE);

		if (IsAffectFlag(AFF_EUNHYUNG))
		{
			RemoveAffect(SKILL_EUNHYUNG);
			SetAffectedEunhyung();
		}
		else
		{
			ClearAffectedEunhyung();
		}

		/*if (IsAffectFlag(AFF_JEONSIN))
		  RemoveAffect(SKILL_JEONSINBANGEO);*/
	}

	/*if (IsAffectFlag(AFF_GUNGON))
	  RemoveAffect(SKILL_GUNGON);*/

	// MINING
	mining_cancel();
	// END_OF_MINING
}

void CHARACTER::OnClick(LPCHARACTER pkChrCauser)
{
	if (!pkChrCauser)
	{
		sys_err("OnClick %s by NULL", GetName());
		return;
	}

	DWORD vid = GetVID();
	sys_log(0, "OnClick %s[vnum %d ServerUniqueID %d, pid %d] by %s", GetName(), GetRaceNum(), vid, GetPlayerID(), pkChrCauser->GetName());

	// »óÁ¡À» ¿¬»óÅÂ·Î Äù½ºÆ®¸¦ ÁøÇàÇÒ ¼ö ¾ø´Ù.
	{
		// ´Ü, ÀÚ½ÅÀº ÀÚ½ÅÀÇ »óÁ¡À» Å¬¸¯ÇÒ ¼ö ÀÖ´Ù.
		if (pkChrCauser->GetMyShop() && pkChrCauser != this) 
		{
			sys_err("OnClick Fail (%s->%s) - pc has shop", pkChrCauser->GetName(), GetName());
			return;
		}
	}

	// ±³È¯ÁßÀÏ¶§ Äù½ºÆ®¸¦ ÁøÇàÇÒ ¼ö ¾ø´Ù.
	{
		if (pkChrCauser->GetExchange())
		{
			sys_err("OnClick Fail (%s->%s) - pc is exchanging", pkChrCauser->GetName(), GetName());
			return;
		}
	}

#ifdef __MELEY_LAIR_DUNGEON__
	if ((IsNPC()) && (GetRaceNum() == (WORD)(MeleyLair::GATE_VNUM)) && (MeleyLair::CMgr::instance().IsMeleyMap(pkChrCauser->GetMapIndex())))
	{
		MeleyLair::CMgr::instance().Start(pkChrCauser, pkChrCauser->GetGuild());
		return;
	}
#endif

	if (IsPC())
	{
		// Å¸°ÙÀ¸·Î ¼³Á¤µÈ °æ¿ì´Â PC¿¡ ÀÇÇÑ Å¬¸¯µµ Äù½ºÆ®·Î Ã³¸®ÇÏµµ·Ï ÇÕ´Ï´Ù.
		if (!CTargetManager::instance().GetTargetInfo(pkChrCauser->GetPlayerID(), TARGET_TYPE_VID, GetVID()))
		{
			// 2005.03.17.myevan.Å¸°ÙÀÌ ¾Æ´Ñ °æ¿ì´Â °³ÀÎ »óÁ¡ Ã³¸® ±â´ÉÀ» ÀÛµ¿½ÃÅ²´Ù.
			if (GetMyShop())
			{
				if (pkChrCauser->IsDead())
					return;

				//PREVENT_TRADE_WINDOW
				if (pkChrCauser == this) // ÀÚ±â´Â °¡´É
				{
					if (!CanShopNow())
					{
						pkChrCauser->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChrCauser, "´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return;
					}
				}
				else // ´Ù¸¥ »ç¶÷ÀÌ Å¬¸¯ÇßÀ»¶§
				{
					// Å¬¸¯ÇÑ »ç¶÷ÀÌ ±³È¯/Ã¢°í/°³ÀÎ»óÁ¡/»óÁ¡ÀÌ¿ëÁßÀÌ¶ó¸é ºÒ°¡
					if (!pkChrCauser->CanShopNow() || pkChrCauser->GetMyShop())
					{
						pkChrCauser->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChrCauser, "´Ù¸¥ °Å·¡Áß(Ã¢°í,±³È¯,»óÁ¡)¿¡´Â °³ÀÎ»óÁ¡À» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return;
					}

					// Å¬¸¯ÇÑ ´ë»óÀÌ ±³È¯/Ã¢°í/»óÁ¡ÀÌ¿ëÁßÀÌ¶ó¸é ºÒ°¡
					//if ((GetExchange() || IsOpenSafebox() || GetShopOwner()))
					if (!CanShopNow())
					{
						pkChrCauser->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChrCauser, "»ó´ë¹æÀÌ ´Ù¸¥ °Å·¡¸¦ ÇÏ°í ÀÖ´Â ÁßÀÔ´Ï´Ù."));
						return;
					}
				}
				//END_PREVENT_TRADE_WINDOW

				if (pkChrCauser->GetShop())
				{
					pkChrCauser->GetShop()->RemoveGuest(pkChrCauser);
					pkChrCauser->SetShop(NULL);
				}

				GetMyShop()->AddGuest(pkChrCauser, GetVID(), false);
				pkChrCauser->SetShopOwner(this);
				return;
			}
			return;
		}
	}

	pkChrCauser->SetQuestItemPtr(NULL);
	pkChrCauser->SetQuestNPCID(GetVID());

	if (quest::CQuestManager::instance().Click(pkChrCauser->GetPlayerID(), this))
	{
		return;
	}


	// NPC Àü¿ë ±â´É ¼öÇà : »óÁ¡ ¿­±â µî
	if (!IsPC())
	{
#ifdef AUCTION_SYSTEM
		if (AuctionShopManager::instance().click_shop(this, pkChrCauser))
			return;
#endif

		if (!m_triggerOnClick.pFunc)
		{
			// NPC Æ®¸®°Å ½Ã½ºÅÛ ·Î±× º¸±â
			//sys_err("%s.OnClickFailure(%s) : triggerOnClick.pFunc is EMPTY(pid=%d)", 
			//			pkChrCauser->GetName(),
			//			GetName(),
			//			pkChrCauser->GetPlayerID());
			return;
		}

		m_triggerOnClick.pFunc(this, pkChrCauser);
	}

}

unsigned char CHARACTER::GetGMLevel(bool bIgnoreTestServer) const
{
	if (bIgnoreTestServer && IsPC())
		return GM::get_level(GetName(), GetDesc()->GetAccountTable().login().c_str(), true);

	if (test_server)
		return GM_IMPLEMENTOR;
	return m_pointsInstant.gm_level;
}

void CHARACTER::SetGMLevel()
{
	if (GetDesc())
	{
		m_pointsInstant.gm_level =  GM::get_level(GetName(), GetDesc()->GetAccountTable().login().c_str());
	}
	else
	{
		m_pointsInstant.gm_level = GM_PLAYER;
	}
}

BOOL CHARACTER::IsGM() const
{
	if (m_pointsInstant.gm_level != GM_PLAYER)
		return true;
	if (test_server)
		return true;
	return false;
}

void CHARACTER::SetGMInvisible(bool bActive, bool bTemporary)
{
	if (bActive == m_bGMInvisible)
		return;

	if (!bTemporary)
		SetQuestFlag("gm_save.is_invisible", bActive);

	m_bGMInvisible = bActive;
	m_bGMInvisibleChanged = true;

	if (!bTemporary)
	{
		if (m_bGMInvisible)
			ReviveInvisible(INFINITE_AFFECT_DURATION);
		else
			RemoveAffect(AFFECT_REVIVE_INVISIBLE);
	}

	UpdateSectree();
}

void CHARACTER::SetStone(LPCHARACTER pkChrStone)
{
	m_pkChrStone = pkChrStone;

	if (m_pkChrStone)
	{
		if (pkChrStone->m_set_pkChrSpawnedBy.find(this) == pkChrStone->m_set_pkChrSpawnedBy.end())
			pkChrStone->m_set_pkChrSpawnedBy.insert(this);
	}
}

struct FuncDeadSpawnedByStone
{
	void operator () (LPCHARACTER ch)
	{
		ch->Dead(NULL);
		ch->SetStone(NULL);
	}
};

void CHARACTER::ClearStone()
{
	if (!m_set_pkChrSpawnedBy.empty())
	{
		// ³»°¡ ½ºÆù½ÃÅ² ¸ó½ºÅÍµéÀ» ¸ðµÎ Á×ÀÎ´Ù.
		FuncDeadSpawnedByStone f;
		std::for_each(m_set_pkChrSpawnedBy.begin(), m_set_pkChrSpawnedBy.end(), f);
		m_set_pkChrSpawnedBy.clear();
	}

	if (!m_pkChrStone)
		return;

	m_pkChrStone->m_set_pkChrSpawnedBy.erase(this);
	m_pkChrStone = NULL;
}

void CHARACTER::ClearTarget()
{
	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);
		m_pkChrTarget = NULL;
	}

	network::GCOutputPacket<network::GCTargetPacket> p;

	p->set_vid(0);
	p->set_hppercent(0);
#ifdef __ELEMENT_SYSTEM__
	p->set_element(0);
#endif
#ifdef NEW_TARGET_UI
	p->set_cur_hp(0);
	p->set_max_hp(0);
#endif

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *(it++);
		pkChr->m_pkChrTarget = NULL;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(p);
	}

	m_set_pkChrTargetedBy.clear();
}

void CHARACTER::SetTarget(LPCHARACTER pkChrTarget)
{
	if (m_pkChrTarget == pkChrTarget)
		return;

	if (m_pkChrTarget)
		m_pkChrTarget->m_set_pkChrTargetedBy.erase(this);

	m_pkChrTarget = pkChrTarget;

	network::GCOutputPacket<network::GCTargetPacket> p;


	if (m_pkChrTarget)
	{
		m_pkChrTarget->m_set_pkChrTargetedBy.insert(this);

		p->set_vid(m_pkChrTarget->GetVID());

		if (m_pkChrTarget->GetMaxHP() <= 0)
			p->set_hppercent(0);
		else
			p->set_hppercent(MINMAX(0, (m_pkChrTarget->GetHP() / float(m_pkChrTarget->GetMaxHP())) * 100, 100));


#ifdef NEW_TARGET_UI
		p->set_cur_hp(m_pkChrTarget->GetHP());
		p->set_max_hp(m_pkChrTarget->GetMaxHP());
#endif
	}
	else
	{
		p->set_vid(0);
		p->set_hppercent(0);
#ifdef NEW_TARGET_UI
		p->set_cur_hp(0);
		p->set_max_hp(0);
#endif
	}

#ifdef __ELEMENT_SYSTEM__
	DWORD curElementBase = 0;
	DWORD raceFlag;
	if (m_pkChrTarget && m_pkChrTarget->IsMonster() && (raceFlag = m_pkChrTarget->GetMobTable().race_flag()) >= RACE_FLAG_ELEC)
	{
		for (int i = RACE_FLAG_ELEC; i <= RACE_FLAG_ICE; i *= 2)
		{
			++curElementBase;
			if (IS_SET(raceFlag, i))
				break;
		}
		p->set_element(curElementBase);
	}
	else
	{
		p->set_element(0);
	}

#endif

	GetDesc()->Packet(p);
}

void CHARACTER::BroadcastTargetPacket()
{
	if (m_set_pkChrTargetedBy.empty())
		return;

	network::GCOutputPacket<network::GCTargetPacket> p;

	p->set_vid(GetVID());

	/*if (IsPC())
		p->set_hppercent(0);
	else*/
		p->set_hppercent(MINMAX(0, (GetHP() / float(GetMaxHP())) * 100, 100));

#ifdef __ELEMENT_SYSTEM__
	p->set_element(0);
#endif
#ifdef NEW_TARGET_UI
	p->set_cur_hp(GetHP());
	p->set_max_hp(GetMaxHP());
#endif

	CHARACTER_SET::iterator it = m_set_pkChrTargetedBy.begin();

	while (it != m_set_pkChrTargetedBy.end())
	{
		LPCHARACTER pkChr = *it++;

		if (!pkChr->GetDesc())
		{
			sys_err("%s %p does not have desc", pkChr->GetName(), get_pointer(pkChr));
			abort();
		}

		pkChr->GetDesc()->Packet(p);
	}
}

void CHARACTER::CheckTarget()
{
	if (!m_pkChrTarget)
		return;

	if (DISTANCE_APPROX(GetX() - m_pkChrTarget->GetX(), GetY() - m_pkChrTarget->GetY()) >= 4800)
		SetTarget(NULL);
}

void CHARACTER::SetWarpLocation(long lMapIndex, long x, long y)
{
	m_posWarp.x = x * 100;
	m_posWarp.y = y * 100;
	m_lWarpMapIndex = lMapIndex;
}

void CHARACTER::SaveExitLocation()
{
	m_posExit = GetXYZ();
	m_lExitMapIndex = GetMapIndex();
}

void CHARACTER::GetExitLocation(long& lMapIndex, long& x, long& y)
{
	lMapIndex = m_lExitMapIndex;
	x = m_posExit.x, y = m_posExit.y;
}

void CHARACTER::ExitToSavedLocation()
{
	sys_log (0, "ExitToSavedLocation");
	WarpSet(m_posWarp.x, m_posWarp.y, m_lWarpMapIndex);

	m_posExit.x = m_posExit.y = m_posExit.z = 0;
	m_lExitMapIndex = 0;
}

// fixme 
// Áö±Ý±îÁø privateMapIndex °¡ ÇöÀç ¸Ê ÀÎµ¦½º¿Í °°ÀºÁö Ã¼Å© ÇÏ´Â °ÍÀ» ¿ÜºÎ¿¡¼­ ÇÏ°í,
// ´Ù¸£¸é warpsetÀ» ºÒ·¶´Âµ¥
// ÀÌ¸¦ warpset ¾ÈÀ¸·Î ³ÖÀÚ.
bool CHARACTER::WarpSet(long x, long y, long lPrivateMapIndex, DWORD dwPIDAddr)
{
	if (!IsPC())
		return false;

	long lAddr;
	long lMapIndex;
	WORD wPort;

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
	{
		sys_err("cannot find map location index %d x %d y %d name %s", lMapIndex, x, y, GetName());
		return false;
	}

	if (lPrivateMapIndex >= 10000)
	{
		if (lPrivateMapIndex / 10000 != lMapIndex)
		{
			sys_err("Invalid map index %d, must be child of %d  [%d]", lPrivateMapIndex, lMapIndex, lPrivateMapIndex / 10000);
			return false;
		}

		lMapIndex = lPrivateMapIndex;
	}

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();

		EncodeRemovePacket(this);
	}

	m_lWarpMapIndex = lMapIndex;
	m_posWarp.x = x;
	m_posWarp.y = y;

	sys_log(0, "WarpSet %s %d %d current map %d target map %d", GetName(), x, y, GetMapIndex(), lMapIndex);

	network::GCOutputPacket<network::GCWarpPacket> p;

	p->set_x(x);
	p->set_y(y);
	p->set_addr(lAddr);
	p->set_port(wPort);

	if (dwPIDAddr)
	{
		CCI* pkAddrCCI = P2P_MANAGER::instance().FindByPID(dwPIDAddr);
		if (pkAddrCCI)
		{
			LPDESC pkAddrDesc = pkAddrCCI->pkDesc;
			p->set_addr(inet_addr(pkAddrDesc->GetHostName()));
			p->set_port(pkAddrDesc->GetListenPort());
		}
	}

	GetDesc()->Packet(p);

	if (!CHARACTER_MANAGER::instance().FlushDelayedSave(this))
		SaveReal();

	// LOGOUT packet
	/*if (GetAccountTable().login[0] && !GetDesc()->IsLogoutPacketSent())
	{
		TLogoutPacket pack;

		strlcpy(pack.login, GetAccountTable().login(), sizeof(pack.login().c_str()));
		strlcpy(pack.passwd, GetAccountTable().passwd, sizeof(pack.passwd));

		db_clientdesc->DBPacket(HEADER_GD_LOGOUT, GetDesc()->GetHandle(), &pack, sizeof(TLogoutPacket));

		GetDesc()->SetLogoutPacketSent();
	}*/

	char buf[256];
	snprintf(buf, sizeof(buf), "%s MapIdx %ld DestMapIdx%ld DestX%ld DestY%ld Empire%d", GetName(), GetMapIndex(), lPrivateMapIndex, x, y, GetEmpire());
	LogManager::instance().CharLog(this, 0, "WARP", buf);

#ifdef __IPV6_FIX__
	if (quest::CQuestManager::instance().GetEventFlag("ipv6_fix_disabled") == 0)
	{
		if (m_bIPV6FixEnabled || quest::CQuestManager::instance().GetEventFlag("ipv6_fix_force_enable") == 1)
		{
			sys_log(0, "IPv6 Activate");
			GetDesc()->ProcessOutput();
			GetDesc()->SetPhase(PHASE_CLOSE);
		}
	}
#endif

	return true;
}

void CHARACTER::WarpEnd()
{
	if (test_server)
		sys_log(0, "WarpEnd %s", GetName());

	if (m_posWarp.x == 0 && m_posWarp.y == 0)
		return;

	int index = m_lWarpMapIndex;

	if (index > 10000)
		index /= 10000;

	if (!map_allow_find(index))
	{
		// ÀÌ °÷À¸·Î ¿öÇÁÇÒ ¼ö ¾øÀ¸¹Ç·Î ¿öÇÁÇÏ±â Àü ÁÂÇ¥·Î µÇµ¹¸®ÀÚ.
		sys_err("location %d %d not allowed to login this server", m_posWarp.x, m_posWarp.y);
		GetDesc()->SetPhase(PHASE_CLOSE);
		return;
	}

	sys_log(0, "WarpEnd %s %d %u %u", GetName(), m_lWarpMapIndex, m_posWarp.x, m_posWarp.y);

	Show(m_lWarpMapIndex, m_posWarp.x, m_posWarp.y, 0);
	Stop();

	m_lWarpMapIndex = 0;
	m_posWarp.x = m_posWarp.y = m_posWarp.z = 0;

	{
		// P2P Login
		network::GGOutputPacket<network::GGLoginPacket> p;

		p->set_name(GetName());
		p->set_pid(GetPlayerID());
		p->set_empire(GetEmpire());
		p->set_map_index(SECTREE_MANAGER::instance().GetMapIndex(GetX(), GetY()));
		p->set_is_in_dungeon(GetMapIndex() >= 10000);
		p->set_channel(g_bChannel);
		p->set_language(GetLanguageID());

		P2P_MANAGER::instance().Send(p);
	}
}

bool CHARACTER::Return()
{
	if (!IsNPC())
		return false;

	if (test_server)
		sys_log(0, "CHARACTER::Return %s", GetName());

	int x, y;
	/*
	   float fDist = DISTANCE_SQRT(m_pkMobData->m_posLastAttacked.x - GetX(), m_pkMobData->m_posLastAttacked.y - GetY());
	   float fx, fy;
	   GetDeltaByDegree(GetRotation(), fDist, &fx, &fy);
	   x = GetX() + (int) fx;
	   y = GetY() + (int) fy;
	 */
	SetVictim(NULL);

	x = m_pkMobInst->m_posLastAttacked.x;
	y = m_pkMobInst->m_posLastAttacked.y;

	SetRotationToXY(x, y);

	if (!Goto(x, y))
		return false;

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);

	if (test_server)
		sys_log(0, "NPC_RETURN %s %p Æ÷±âÇÏ°í µ¹¾Æ°¡ÀÚ! %d %d", GetName(), this, x, y);

	if (GetParty())
		GetParty()->SendMessage(this, PM_RETURN, x, y);

	return true;
}

bool CHARACTER::Follow(LPCHARACTER pkChr, float fMinDistance)
{
	if (IsPC())
	{
		sys_err("CHARACTER::Follow : PC cannot use this method", GetName());
		return false;
	}

	// TRENT_MONSTER
	if (IS_SET(m_pointsInstant.dwAIFlag, AIFLAG_NOMOVE))
	{
		if (pkChr->IsPC()) // ÂÑ¾Æ°¡´Â »ó´ë°¡ PCÀÏ ¶§
		{
			// If i'm in a party. I must obey party leader's AI.
			if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
			{
				if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000) // ¸¶Áö¸·À¸·Î °ø°Ý¹ÞÀºÁö 15ÃÊ°¡ Áö³µ°í
				{
					// ¸¶Áö¸· ¸ÂÀº °÷À¸·Î ºÎÅÍ 50¹ÌÅÍ ÀÌ»ó Â÷ÀÌ³ª¸é Æ÷±âÇÏ°í µ¹¾Æ°£´Ù.
					if (m_pkMobData->m_table.attack_range() < DISTANCE_APPROX(pkChr->GetX() - GetX(), pkChr->GetY() - GetY()))
						if (Return())
							return true;
				}
			}
		}
		return false;
	}
	// END_OF_TRENT_MONSTER

	long x = pkChr->GetX();
	long y = pkChr->GetY();

#ifdef __FAKE_PC__
	if (pkChr->IsPC() && IsMonster() && !FakePC_IsSupporter())
#else
	if (pkChr->IsPC() && IsMonster())
#endif
	{
		// If i'm in a party. I must obey party leader's AI.
		if (!GetParty() || !GetParty()->GetLeader() || GetParty()->GetLeader() == this)
		{
			if (get_dword_time() - m_pkMobInst->m_dwLastAttackedTime >= 15000) // ¸¶Áö¸·À¸·Î °ø°Ý¹ÞÀºÁö 15ÃÊ°¡ Áö³µ°í
			{
				// ¸¶Áö¸· ¸ÂÀº °÷À¸·Î ºÎÅÍ 50¹ÌÅÍ ÀÌ»ó Â÷ÀÌ³ª¸é Æ÷±âÇÏ°í µ¹¾Æ°£´Ù.
				if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
					if (Return())
						return true;
			}
		}
	}

	if (IsGuardNPC())
	{
// GUARDIAN_NO_WALK
		return false;
// GUARDIAN_NO_WALK
		if (5000 < DISTANCE_APPROX(m_pkMobInst->m_posLastAttacked.x - GetX(), m_pkMobInst->m_posLastAttacked.y - GetY()))
			if (Return())
				return true;
	}

	if (pkChr->IsState(pkChr->m_stateMove) && 
		GetMobBattleType() != BATTLE_TYPE_RANGE && 
		GetMobBattleType() != BATTLE_TYPE_MAGIC
#ifdef __PET_SYSTEM__
		&& false == IsPet()
#endif
		)
	{
		// ´ë»óÀÌ ÀÌµ¿ÁßÀÌ¸é ¿¹Ãø ÀÌµ¿À» ÇÑ´Ù
		// ³ª¿Í »ó´ë¹æÀÇ ¼ÓµµÂ÷¿Í °Å¸®·ÎºÎÅÍ ¸¸³¯ ½Ã°£À» ¿¹»óÇÑ ÈÄ
		// »ó´ë¹æÀÌ ±× ½Ã°£±îÁö Á÷¼±À¸·Î ÀÌµ¿ÇÑ´Ù°í °¡Á¤ÇÏ¿© °Å±â·Î ÀÌµ¿ÇÑ´Ù.
		float rot = pkChr->GetRotation();
		float rot_delta = GetDegreeDelta(rot, GetDegreeFromPositionXY(GetX(), GetY(), pkChr->GetX(), pkChr->GetY()));

		float yourSpeed = pkChr->GetMoveSpeed();
		float mySpeed = GetMoveSpeed();

		float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());
		float fFollowSpeed = mySpeed - yourSpeed * cos(rot_delta * M_PI / 180);

		if (fFollowSpeed >= 0.1f)
		{
			float fMeetTime = fDist / fFollowSpeed;
			float fYourMoveEstimateX, fYourMoveEstimateY;

			if( fMeetTime * yourSpeed <= 100000.0f )
			{
				GetDeltaByDegree(pkChr->GetRotation(), fMeetTime * yourSpeed, &fYourMoveEstimateX, &fYourMoveEstimateY);

				x += (long) fYourMoveEstimateX;
				y += (long) fYourMoveEstimateY;

				float fDistNew = sqrt(((double)x - GetX())*(x-GetX())+((double)y - GetY())*(y-GetY()));
				if (fDist < fDistNew)
				{
					x = (long)(GetX() + (x - GetX()) * fDist / fDistNew);
					y = (long)(GetY() + (y - GetY()) * fDist / fDistNew);
				}
			}
		}
	}

	// °¡·Á´Â À§Ä¡¸¦ ¹Ù¶óºÁ¾ß ÇÑ´Ù.
	SetRotationToXY(x, y);

	float fDist = DISTANCE_SQRT(x - GetX(), y - GetY());

	if (fDist <= fMinDistance)
		return false;

	float fx, fy;

	if (IsChangeAttackPosition(pkChr) && GetMobRank() < MOB_RANK_BOSS)
	{
		// »ó´ë¹æ ÁÖº¯ ·£´ýÇÑ °÷À¸·Î ÀÌµ¿
		SetChangeAttackPositionTime();

		int retry = 16;
		int dx, dy;
		int rot = (int) GetDegreeFromPositionXY(x, y, GetX(), GetY());

		while (--retry)
		{
			if (fDist < 500.0f)
				GetDeltaByDegree((rot + random_number(-90, 90) + random_number(-90, 90)) % 360, fMinDistance, &fx, &fy);
			else
				GetDeltaByDegree(random_number(0, 359), fMinDistance, &fx, &fy);

			dx = x + (int) fx;
			dy = y + (int) fy;

			LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), dx, dy);

			if (NULL == tree)
				break;

			if (0 == (tree->GetAttribute(dx, dy) & (ATTR_BLOCK | ATTR_OBJECT)))
				break;
		}

		//sys_log(0, "±ÙÃ³ ¾îµò°¡·Î ÀÌµ¿ %s retry %d", GetName(), retry);
		if (!Goto(dx, dy))
			return false;
	}
	else
	{
		// Á÷¼± µû¶ó°¡±â
		float fDistToGo = fDist - fMinDistance;
		GetDeltaByDegree(GetRotation(), fDistToGo, &fx, &fy);

		//sys_log(0, "Á÷¼±À¸·Î ÀÌµ¿ %s", GetName());
		if (!Goto(GetX() + (int) fx, GetY() + (int) fy))
			return false;
	}

	SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
	//MonsterLog("ÂÑ¾Æ°¡±â; %s", pkChr->GetName());
	return true;
}

float CHARACTER::GetDistanceFromSafeboxOpen() const
{
	return DISTANCE_APPROX(GetX() - m_posSafeboxOpen.x, GetY() - m_posSafeboxOpen.y);
}

void CHARACTER::SetSafeboxOpenPosition()
{
	m_posSafeboxOpen = GetXYZ();
}

CSafebox * CHARACTER::GetSafebox() const
{
	return m_pkSafebox;
}

void CHARACTER::ReqSafeboxLoad(const char* pszPassword)
{
	if (!GM::check_allow(GetGMLevel(), GM_ALLOW_USE_SAFEBOX))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot do this with this gamemaster rank."));
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (GetDesc()->IsTradeblocked())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return;
	}
#endif

	if (!*pszPassword || strlen(pszPassword) > SAFEBOX_PASSWORD_MAX_LEN)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<Ã¢°í> Àß¸øµÈ ¾ÏÈ£¸¦ ÀÔ·ÂÇÏ¼Ì½À´Ï´Ù."));
		return;
	}
	else if (m_pkSafebox)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<Ã¢°í> Ã¢°í°¡ ÀÌ¹Ì ¿­·ÁÀÖ½À´Ï´Ù."));
		return;
	}

	int iPulse = thecore_pulse();

	if (iPulse - GetSafeboxLoadTime()  < PASSES_PER_SEC(10))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<Ã¢°í> Ã¢°í¸¦ ´ÝÀºÁö 10ÃÊ ¾È¿¡´Â ¿­ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
	else if (GetDistanceFromSafeboxOpen() > 1000)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "<Ã¢°í> °Å¸®°¡ ¸Ö¾î¼­ Ã¢°í¸¦ ¿­ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
	else if (m_bOpeningSafebox)
	{
		sys_log(0, "Overlapped safebox load request from %s", GetName());
		return;
	}

	SetSafeboxLoadTime();
	m_bOpeningSafebox = true;

	network::GDOutputPacket<network::GDSafeboxLoadPacket> p;
	p->set_account_id(GetDesc()->GetAccountTable().id());
	p->set_login(GetDesc()->GetAccountTable().login());
	p->set_password(pszPassword);
	p->set_is_mall(false);

	db_clientdesc->DBPacket(p, GetDesc()->GetHandle());
}

void CHARACTER::LoadSafebox(int iSize, DWORD dwGold, const ::google::protobuf::RepeatedPtrField<network::TItemData>& items)
{
	bool bLoaded = false;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(true);
	//END_PREVENT_TRADE_WINDOW

	if (m_pkSafebox)
		bLoaded = true;

	if (!m_pkSafebox)
		m_pkSafebox = M2_NEW CSafebox(this, iSize, dwGold);
	else
		m_pkSafebox->ChangeSize(iSize);

	m_iSafeboxSize = iSize;

	network::GCOutputPacket<network::GCSafeboxSizePacket> p;
	p->set_size(iSize);

	GetDesc()->Packet(p);

	if (!bLoaded)
	{
		for (auto& elem : items)
		{
			if (!m_pkSafebox->IsValidPosition(elem.cell().cell()))
				continue;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(&elem, true);

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", elem.vnum(), elem.id(), GetName());
				continue;
			}

			if (!m_pkSafebox->Add(elem.cell().cell(), item))
			{
				M2_DESTROY_ITEM(item);
			}
			else
				item->SetSkipSave(false);
		}
	}
}

void CHARACTER::ChangeSafeboxSize(BYTE bSize)
{
	//if (!m_pkSafebox)
	//return;

	network::GCOutputPacket<network::GCSafeboxSizePacket> p;
	p->set_size(bSize);

	GetDesc()->Packet(p);

	if (m_pkSafebox)
		m_pkSafebox->ChangeSize(bSize);

	m_iSafeboxSize = bSize;
}

void CHARACTER::CloseSafebox()
{
	if (!m_pkSafebox)
		return;

	//PREVENT_TRADE_WINDOW
	SetOpenSafebox(false);
	//END_PREVENT_TRADE_WINDOW

	m_pkSafebox->Save();

	M2_DELETE(m_pkSafebox);
	m_pkSafebox = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseSafebox");

	SetSafeboxLoadTime();
	m_bOpeningSafebox = false;

	Save();
}

CSafebox * CHARACTER::GetMall() const
{
	return m_pkMall;
}

void CHARACTER::LoadMall(const ::google::protobuf::RepeatedPtrField<network::TItemData>& items)
{
	bool bLoaded = false;

	if (m_pkMall)
		bLoaded = true;

	if (!m_pkMall)
		m_pkMall = M2_NEW CSafebox(this, SAFEBOX_PAGE_SIZE, 0);
	else
		m_pkMall->ChangeSize(SAFEBOX_PAGE_SIZE);

	m_pkMall->SetWindowMode(MALL);

	network::GCOutputPacket<network::GCMallOpenPacket> p;
	p->set_size(SAFEBOX_PAGE_SIZE);

	GetDesc()->Packet(p);

	if (!bLoaded)
	{
		for (auto& elem : items)
		{
			if (!m_pkMall->IsValidPosition(elem.cell().cell()))
				continue;

			LPITEM item = ITEM_MANAGER::instance().CreateItem(&elem, true);

			if (!item)
			{
				sys_err("cannot create item vnum %d id %u (name: %s)", elem.vnum(), elem.id(), GetName());
				continue;
			}

			if (!m_pkMall->Add(elem.cell().cell(), item))
				M2_DESTROY_ITEM(item);
			else
				item->SetSkipSave(false);
		}
	}
}

void CHARACTER::CloseMall()
{
	if (!m_pkMall)
		return;

	m_pkMall->Save();

	M2_DELETE(m_pkMall);
	m_pkMall = NULL;

	ChatPacket(CHAT_TYPE_COMMAND, "CloseMall");
}

bool CHARACTER::BuildUpdatePartyPacket(network::GCOutputPacket<network::GCPartyUpdatePacket>& out)
{
	if (!GetParty())
		return false;

	out->set_pid(GetPlayerID());
	out->set_percent_hp(MINMAX(0, GetHP() * 100 / GetMaxHP(), 100));
	out->set_role(GetParty()->GetRole(GetPlayerID()));
	out->set_leader(GetParty()->GetLeaderPID() == GetPlayerID());

	sys_log(1, "PARTY %s role is %d", GetName(), out->role());

	LPCHARACTER l = GetParty()->GetLeaderCharacter();

	if (l && DISTANCE_APPROX(GetX() - l->GetX(), GetY() - l->GetY()) < PARTY_DEFAULT_RANGE)
	{
		out->add_affects(GetParty()->GetPartyBonusExpPercent());
		out->add_affects(GetPoint(POINT_PARTY_ATTACKER_BONUS));
		out->add_affects(GetPoint(POINT_PARTY_TANKER_BONUS));
		out->add_affects(GetPoint(POINT_PARTY_BUFFER_BONUS));
		out->add_affects(GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
		out->add_affects(GetPoint(POINT_PARTY_HASTE_BONUS));
		out->add_affects(GetPoint(POINT_PARTY_DEFENDER_BONUS));
	}

	return true;
}

int CHARACTER::GetLeadershipSkillLevel() const
{ 
	return GetSkillLevel(SKILL_LEADERSHIP);
}

void CHARACTER::QuerySafeboxSize()
{
	if (m_iSafeboxSize == -1)
	{
		DBManager::instance().ReturnQuery(QID_SAFEBOX_SIZE,
				GetPlayerID(),
				NULL, 
				"SELECT size, password != '000000' AND password != '' FROM safebox WHERE account_id = %u",
				GetDesc()->GetAccountTable().id());
	}
}

void CHARACTER::SetSafeboxSize(int iSize)
{
	sys_log(1, "SetSafeboxSize: %s %d", GetName(), iSize);
	m_iSafeboxSize = iSize;
	DBManager::instance().Query("UPDATE safebox SET size = %d WHERE account_id = %u", iSize / SAFEBOX_PAGE_SIZE, GetDesc()->GetAccountTable().id());

	//if (m_iSafeboxSize > 0)
	//	ChatPacket(CHAT_TYPE_COMMAND, "EnableSafebox");
}

void CHARACTER::SetSafeboxNeedPassword(bool bNeedPassword)
{
	m_bSafeboxNeedPassword = bNeedPassword;
}

int CHARACTER::GetSafeboxSize() const
{
	return m_iSafeboxSize;
}

bool CHARACTER::IsNeedSafeboxPassword() const
{
	return m_bSafeboxNeedPassword;
}

void CHARACTER::SetNowWalking(bool bWalkFlag)
{
	//if (m_bNowWalking != bWalkFlag || IsNPC())
	if (m_bNowWalking != bWalkFlag)
	{
		if (bWalkFlag)
		{
			m_bNowWalking = true;
			m_dwWalkStartTime = get_dword_time();
		}
		else
		{
			m_bNowWalking = false;
		}

		//if (m_bNowWalking)
		{
			network::GCOutputPacket<network::GCWalkModePacket> p;
			p->set_vid(GetVID());
			p->set_mode(m_bNowWalking ? WALKMODE_WALK : WALKMODE_RUN);

			PacketView(p);
		}

		if (IsNPC())
		{
			if (m_bNowWalking)
				MonsterLog("°È´Â´Ù");
			else
				MonsterLog("¶Ú´Ù");
		}

		//sys_log(0, "%s is now %s", GetName(), m_bNowWalking?"walking.":"running.");
	}
}

void CHARACTER::StartStaminaConsume()
{
#ifdef __USE_STAMINA__
	if (m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = true;
	//ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
	if (IsStaminaHalfConsume())
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec / 2, GetStamina());
	else
		ChatPacket(CHAT_TYPE_COMMAND, "StartStaminaConsume %d %d", STAMINA_PER_STEP * passes_per_sec, GetStamina());
#endif
}

void CHARACTER::StopStaminaConsume()
{
#ifdef __USE_STAMINA__
	if (!m_bStaminaConsume)
		return;
	PointChange(POINT_STAMINA, 0);
	m_bStaminaConsume = false;
	ChatPacket(CHAT_TYPE_COMMAND, "StopStaminaConsume %d", GetStamina());
#endif
}

bool CHARACTER::IsStaminaConsume() const
{
	return m_bStaminaConsume;
}

bool CHARACTER::IsStaminaHalfConsume() const
{
	return IsEquipUniqueItem(UNIQUE_ITEM_HALF_STAMINA);
}

void CHARACTER::ResetStopTime()
{
	m_dwStopTime = get_dword_time();
}

DWORD CHARACTER::GetStopTime() const
{
	return m_dwStopTime;
}

void CHARACTER::ResetPoint(int iLv)
{
	BYTE bJob = GetJob();

	PointChange(POINT_LEVEL, iLv - GetLevel(), false, true);

	SetRealPoint(POINT_ST, JobInitialPoints[bJob].st);
	SetPoint(POINT_ST, GetRealPoint(POINT_ST));

	SetRealPoint(POINT_HT, JobInitialPoints[bJob].ht);
	SetPoint(POINT_HT, GetRealPoint(POINT_HT));

	SetRealPoint(POINT_DX, JobInitialPoints[bJob].dx);
	SetPoint(POINT_DX, GetRealPoint(POINT_DX));

	SetRealPoint(POINT_IQ, JobInitialPoints[bJob].iq);
	SetPoint(POINT_IQ, GetRealPoint(POINT_IQ));

	//PointChange(POINT_STAT, ((MINMAX(1, iLv, 90) - 1) * 3) + GetPoint(POINT_LEVEL_STEP) - GetPoint(POINT_STAT));
	
	if(iLv <= 90)
		PointChange(POINT_STAT, ((MINMAX(1, iLv, 90) - 1) * 3) + GetPoint(POINT_LEVEL_STEP) - GetPoint(POINT_STAT));
	else
		PointChange(POINT_STAT, 270 - GetPoint(POINT_STAT));
	
	ComputePoints();

	// È¸º¹
	PointChange(POINT_HP, GetMaxHP() - GetHP());
	PointChange(POINT_SP, GetMaxSP() - GetSP());

	PointChange(POINT_EXP, -(int)GetExp());

	PointsPacket();

	LogManager::instance().CharLog(this, 0, "RESET_POINT", "");
}

bool CHARACTER::IsChangeAttackPosition(LPCHARACTER target) const
{ 
	if (!IsNPC())
		return true;

	DWORD dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_NEAR;

	if (DISTANCE_APPROX(GetX() - target->GetX(), GetY() - target->GetY()) > 
		AI_CHANGE_ATTACK_POISITION_DISTANCE + GetMobAttackRange())
		dwChangeTime = AI_CHANGE_ATTACK_POISITION_TIME_FAR;

	return get_dword_time() - m_dwLastChangeAttackPositionTime > dwChangeTime; 
}

void CHARACTER::GiveRandomSkillBook()
{
	LPITEM item = AutoGiveItem(50300);

	if (NULL != item)
	{
		BYTE bJob = 0;

		if (!random_number(0, 1))
			bJob = GetJob() + 1;

		DWORD dwSkillVnum = 0;

		do
		{
			dwSkillVnum = random_number(1, 111);
			const CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);

			if (NULL == pkSk)
				continue;

			if (bJob && bJob != pkSk->dwType)
				continue;

			break;
		} while (true);

		item->SetSocket(0, dwSkillVnum);
	}
}

void CHARACTER::ReviveInvisible(int iDur)
{
	AddAffect(AFFECT_REVIVE_INVISIBLE, POINT_NONE, 0, AFF_REVIVE_INVISIBLE, iDur, 0, true);
}

void CHARACTER::ToggleMonsterLog()
{
	m_bMonsterLog = !m_bMonsterLog;

	if (m_bMonsterLog)
	{
		CHARACTER_MANAGER::instance().RegisterForMonsterLog(this);
	}
	else
	{
		CHARACTER_MANAGER::instance().UnregisterForMonsterLog(this);
	}
}

void CHARACTER::SetGuild(CGuild* pGuild)
{
	if (m_pGuild != pGuild)
	{
		m_pGuild = pGuild;
		UpdatePacket();
	}
}

void CHARACTER::BeginStateEmpty()
{
	MonsterLog("!");
}

void CHARACTER::EffectPacket(int enumEffectType)
{
	network::GCOutputPacket<network::GCSpecialEffectPacket> p;

	p->set_type(enumEffectType);
	p->set_vid(GetVID());

	PacketAround(p);
}

void CHARACTER::SpecificEffectPacket(const char* filename)
{
	network::GCOutputPacket<network::GCSpecificEffectPacket> p;

	p->set_vid(GetVID());
	p->set_effect_file(filename);

	PacketAround(p);
}

void CHARACTER::MonsterChat(BYTE bMonsterChatType)
{
#ifndef __USE_MONSTERCHAT__
	return;
#endif
	if (IsPC())
		return;

	char sbuf[256+1];

	if (IsMonster())
	{
		if (random_number(0, 60))
			return;

		snprintf(sbuf, sizeof(sbuf), 
				"(locale.monster_chat[%i] and locale.monster_chat[%i][%d] or '')",
				GetRaceNum(), GetRaceNum(), bMonsterChatType*3 + random_number(1, 3));
	}
	else
	{
		if (bMonsterChatType != MONSTER_CHAT_WAIT)
			return;

		if (IsGuardNPC())
		{
			if (random_number(0, 6))
				return;
		}
		else
		{
			if (random_number(0, 30))
				return;
		}

		snprintf(sbuf, sizeof(sbuf), "(locale.monster_chat[%i] and locale.monster_chat[%i][random_number(1, table.getn(locale.monster_chat[%i]))] or '')", GetRaceNum(), GetRaceNum(), GetRaceNum());
	}

	std::string text = quest::ScriptToString(sbuf);

	if (text.empty())
		return;

	network::GCOutputPacket<network::GCChatPacket> pack_chat;

	pack_chat->set_type(CHAT_TYPE_TALKING);
	pack_chat->set_id(GetVID());
	pack_chat->set_message(text);

	PacketAround(pack_chat);
}

void CHARACTER::SetQuestNPCID(DWORD vid)
{
	if (vid && m_dwQuestNPCVID)
	{
		quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());
		if (pPC && pPC->IsRunning())
		{
			if (test_server)
				sys_err("cannot reset quest npc id - already running quest [%u %s]", GetPlayerID(), GetName());
			return;
		}
	}
	m_dwQuestNPCVID = vid;
}

LPCHARACTER CHARACTER::GetQuestNPC() const
{
	if (!m_dwQuestNPCVID)
		return NULL;

	return CHARACTER_MANAGER::instance().Find(m_dwQuestNPCVID);
}

void CHARACTER::SetQuestItemPtr(LPITEM item)
{
	m_pQuestItem = item;
}

void CHARACTER::ClearQuestItemPtr()
{
	m_pQuestItem = NULL;
}

LPITEM CHARACTER::GetQuestItemPtr() const
{
	return m_pQuestItem;
}

LPDUNGEON CHARACTER::GetDungeonForce() const
{ 
	if (test_server)
		sys_log(0, "GetDungeonForce %s warp_idx %ld pkDungeon %p", GetName(), m_lWarpMapIndex, m_pkDungeon);

	if (m_lWarpMapIndex > 10000)
		return CDungeonManager::instance().FindByMapIndex(m_lWarpMapIndex);

	return m_pkDungeon;
}

void CHARACTER::SetBlockMode(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;

	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);

	SetQuestFlag("game_option.block_exchange", bFlag & BLOCK_EXCHANGE ? 1 : 0);
	SetQuestFlag("game_option.block_party_invite", bFlag & BLOCK_PARTY_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_guild_invite", bFlag & BLOCK_GUILD_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_whisper", bFlag & BLOCK_WHISPER ? 1 : 0);
	SetQuestFlag("game_option.block_messenger_invite", bFlag & BLOCK_MESSENGER_INVITE ? 1 : 0);
	SetQuestFlag("game_option.block_party_request", bFlag & BLOCK_PARTY_REQUEST ? 1 : 0);
}

void CHARACTER::SetBlockModeForce(BYTE bFlag)
{
	m_pointsInstant.bBlockMode = bFlag;
	ChatPacket(CHAT_TYPE_COMMAND, "setblockmode %d", m_pointsInstant.bBlockMode);
}

bool CHARACTER::IsGuardNPC() const
{
#ifdef ENABLE_ZODIAC_TEMPLE
	// return (GetRaceNum() == 20438) || (GetRaceNum() == 20441) || (GetRaceNum() == 20442);
	return (GetRaceNum() == 6530);
#else
	return IsNPC() && (GetRaceNum() == 11000 || GetRaceNum() == 11002 || GetRaceNum() == 11004);
#endif
}

int CHARACTER::GetPolymorphPower() const
{
	if (test_server)
	{
		int value = quest::CQuestManager::instance().GetEventFlag("poly");
		if (value)
			return value;
	}
	return aiPolymorphPowerByLevel[MINMAX(0, GetSkillLevel(SKILL_POLYMORPH), 40)];
}

EVENTFUNC(set_polymorph_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);

	if(info == NULL)
	{
		sys_err("set_polymorph_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if(ch == NULL)
		return 0;

	ch->SetPolymorph(ch->m_dwPolymorphEventRace);

	return 0;
}

void CHARACTER::CancelPolymorphEvent()
{
	if(m_PolymorphEvent)
	{
		event_cancel(&m_PolymorphEvent);
		m_PolymorphEvent = NULL;
	}
}

void CHARACTER::StartPolymorphEvent()
{
	// Cancel currently running event/clear it
	CancelPolymorphEvent();

	// Fix unpoly
	if(m_dwPolymorphEventRace < JOB_MAX_NUM)
	{
		SetPolymorph(m_dwPolymorphEventRace);
		return;
	}

	ChatPacket(CHAT_TYPE_INFO, "You will get transformed soon!");

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_PolymorphEvent = event_create(set_polymorph_event, info, PASSES_PER_SEC(5));
}

void CHARACTER::SetPolymorph(DWORD dwRaceNum, bool bMaintainStat)
{
	if (dwRaceNum < JOB_MAX_NUM)
	{
		dwRaceNum = 0;
		bMaintainStat = false;
		CancelPolymorphEvent();
	}

	if (m_dwPolymorphRace == dwRaceNum)
		return;

	if (GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
	{
		ChatPacket(CHAT_TYPE_INFO, "You can't polymorph in this map.");
		return;
	}

	m_bPolyMaintainStat = bMaintainStat;
	m_dwPolymorphRace = dwRaceNum;

	sys_log(!test_server, "POLYMORPH: %s race %u ", GetName(), dwRaceNum);

	if (dwRaceNum != 0)
	{
		if (GetMountSystem() && GetMountSystem()->IsRiding())
			GetMountSystem()->StopRiding();
		else if (GetMountVnum())
			MountVnum(0);

		sys_log(!test_server, "SetPolymorph %s unmount player", GetName());
	}

	SET_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);
	m_afAffectFlag.Set(AFF_SPAWN);

	ViewReencode();

	REMOVE_BIT(m_bAddChrState, ADD_CHARACTER_STATE_SPAWN);

	sys_log(!test_server, "SetPolymorph %s chr state", GetName());

	if (!bMaintainStat)
	{
		PointChange(POINT_ST, 0);
		PointChange(POINT_DX, 0);
		PointChange(POINT_IQ, 0);
		PointChange(POINT_HT, 0);
	}

	// Æú¸®¸ðÇÁ »óÅÂ¿¡¼­ Á×´Â °æ¿ì, Æú¸®¸ðÇÁ°¡ Ç®¸®°Ô µÇ´Âµ¥
	// Æú¸® ¸ðÇÁ ÀüÈÄ·Î valid combo intervalÀÌ ´Ù¸£±â ¶§¹®¿¡
	// Combo ÇÙ ¶Ç´Â Hacker·Î ÀÎ½ÄÇÏ´Â °æ¿ì°¡ ÀÖ´Ù.
	// µû¶ó¼­ Æú¸®¸ðÇÁ¸¦ Ç®°Å³ª Æú¸®¸ðÇÁ ÇÏ°Ô µÇ¸é,
	// valid combo intervalÀ» resetÇÑ´Ù.
	SetValidComboInterval(0);
	SetComboSequence(0);

	ComputeBattlePoints();

	sys_log(!test_server, "SetPolymorph %s call success?", GetName());
}

int CHARACTER::GetQuestFlag(const std::string& flag) const
{
	if (!IsPC())
		return 0;

	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	if (pPC)
		return pPC->GetFlag(flag);
	else
	{
		sys_log(0, "PC::GetQuestFlag(%u %s) no pc - return 0", GetPlayerID(), GetName());
		return 0;
	}
}

void CHARACTER::SetQuestFlag(const std::string& flag, int value)
{
	if (!IsPC())
		return;

	quest::CQuestManager& q = quest::CQuestManager::instance();
	quest::PC* pPC = q.GetPC(GetPlayerID());
	pPC->SetFlag(flag, value);
}

void CHARACTER::DetermineDropMetinStone()
{
	const int METIN_STONE_NUM = 14;
	static DWORD c_adwMetin[METIN_STONE_NUM] = 
	{
		28030,
		28031,
		28032,
		28033,
		28034,
		28035,
		28036,
		28037,
		28038,
		28039,
		28040,
		28041,
		28042,
		28043,
	};
	DWORD stone_num = GetRaceNum();
	int idx = std::lower_bound(aStoneDrop, aStoneDrop+STONE_INFO_MAX_NUM, stone_num) - aStoneDrop;
	if (idx >= STONE_INFO_MAX_NUM || aStoneDrop[idx].dwMobVnum != stone_num)
	{
		m_dwDropMetinStone = 0;
	}
	else
	{
		const SStoneDropInfo & info = aStoneDrop[idx];
		m_bDropMetinStonePct = info.iDropPct;
		{
			m_dwDropMetinStone = c_adwMetin[random_number(0, METIN_STONE_NUM - 1)];
			int iGradePct = random_number(1, 100);
			for (int iStoneLevel = 0; iStoneLevel < STONE_LEVEL_MAX_NUM; iStoneLevel ++)
			{
				int iLevelGradePortion = info.iLevelPct[iStoneLevel];
				if (iGradePct <= iLevelGradePortion)
				{
					break;
				}
				else
				{
					iGradePct -= iLevelGradePortion;
					m_dwDropMetinStone += 100; // µ¹ +a -> +(a+1)ÀÌ µÉ¶§¸¶´Ù 100¾¿ Áõ°¡
				}
			}
		}
	}
}

void CHARACTER::SendEquipment(LPCHARACTER ch)
{
	network::GCOutputPacket<network::GCViewEquipPacket> p;

	p->set_vid(GetVID());
	for (int i = 0; i<WEAR_MAX_NUM; i++)
	{
		auto equip = p->add_equips();

		LPITEM item = GetWear(i);
		if (item)
			ITEM_MANAGER::Instance().GetPlayerItem(item, equip);
	}

	ch->GetDesc()->Packet(p);
}

bool CHARACTER::CanSummon(int iLeaderShip)
{
	return (iLeaderShip >= 20 || iLeaderShip >= 12 && m_dwLastDeadTime + 180 > get_dword_time());
}


void CHARACTER::MountVnum(DWORD vnum)
{
	if (m_dwMountVnum == vnum)
		return;

	m_dwMountVnum = vnum;
	m_dwMountTime = get_dword_time();

#ifdef COMBAT_ZONE
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()))
		return;
#endif

	if (m_bIsObserver)
		return;

	//NOTE : MountÇÑ´Ù°í ÇØ¼­ Client SideÀÇ °´Ã¼¸¦ »èÁ¦ÇÏÁø ¾Ê´Â´Ù.
	//±×¸®°í ¼­¹öSide¿¡¼­ ÅÀÀ»¶§ À§Ä¡ ÀÌµ¿Àº ÇÏÁö ¾Ê´Â´Ù. ¿Ö³ÄÇÏ¸é Client Side¿¡¼­ Coliision Adjust¸¦ ÇÒ¼ö ÀÖ´Âµ¥
	//°´Ã¼¸¦ ¼Ò¸ê½ÃÄ×´Ù°¡ ¼­¹öÀ§Ä¡·Î ÀÌµ¿½ÃÅ°¸é ÀÌ¶§ collision check¸¦ ÇÏÁö´Â ¾ÊÀ¸¹Ç·Î ¹è°æ¿¡ ³¢°Å³ª ¶Õ°í ³ª°¡´Â ¹®Á¦°¡ Á¸ÀçÇÑ´Ù.
	m_posDest.x = m_posStart.x = GetX();
	m_posDest.y = m_posStart.y = GetY();
	//EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		//MountÇÑ´Ù°í ÇØ¼­ Client SideÀÇ °´Ã¼¸¦ »èÁ¦ÇÏÁø ¾Ê´Â´Ù.
		//EncodeRemovePacket(entity);
		//if (!m_bIsObserver)
		EncodeInsertPacket(entity);

		//if (!entity->IsObserverMode())
		//	entity->EncodeInsertPacket(this);
	}

#ifdef __FAKE_PC__
	if (LPCHARACTER pkSup = FakePC_Owner_GetSupporter())
		pkSup->ViewReencode();
#endif

	SetValidComboInterval(0);
	SetComboSequence(0);

	ComputePoints();
}

namespace {
	class FuncCheckWarp
	{
		public:
			FuncCheckWarp(LPCHARACTER pkWarp)
			{
				m_lTargetMapIndex = 0;
				m_lTargetY = 0;
				m_lTargetX = 0;

				m_lX = pkWarp->GetX();
				m_lY = pkWarp->GetY();

				m_bInvalid = false;
				m_bEmpire = pkWarp->GetEmpire();

				char szTmp[64];

				if ((pkWarp->IsGoto() && 3 != sscanf(pkWarp->GetName(), " %s %ld %ld ", szTmp, &m_lTargetX, &m_lTargetY)) ||
					(!pkWarp->IsGoto() && 4 != sscanf(pkWarp->GetName(), " %s %ld %ld %ld ", szTmp, &m_lTargetMapIndex, &m_lTargetX, &m_lTargetY)))
				{
					/*if (random_number(1, 100) < 5)
						sys_err("Warp NPC name wrong : vnum(%d) name(%s)", pkWarp->GetRaceNum(), pkWarp->GetName());*/

					m_bInvalid = true;

					return;
				}

				m_lTargetX *= 100;
				m_lTargetY *= 100;

				m_bUseWarp = true;

				if (pkWarp->IsGoto())
				{
					m_lTargetMapIndex = pkWarp->GetMapIndex();
					m_bUseWarp = false;
				}
				const TMapRegion* pkRegion;
				if (pkWarp->GetDungeon())
					pkRegion = SECTREE_MANAGER::instance().GetMapRegion(m_lTargetMapIndex / 10000);
				else
					pkRegion = SECTREE_MANAGER::instance().GetMapRegion(m_lTargetMapIndex);
				if (!pkRegion)
				{
					if (random_number(1, 100) < 5)
						sys_err("Warp NPC map index wrong : vnum(%d) name(%s) mapIndex(%ld) on map: %d",
							pkWarp->GetRaceNum(), pkWarp->GetName(), m_lTargetMapIndex, pkWarp->GetMapIndex());
					
					m_bInvalid = true;
					
					return;
				}

				m_lTargetX += pkRegion->sx;
				m_lTargetY += pkRegion->sy;
			}

			bool Valid()
			{
				return !m_bInvalid;
			}

			void operator () (LPENTITY ent)
			{
				if (!Valid())
					return;

				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER pkChr = (LPCHARACTER) ent;

				if (!pkChr->IsPC())
					return;

				int iDist = DISTANCE_APPROX(pkChr->GetX() - m_lX, pkChr->GetY() - m_lY);

				if (iDist > 300)
					return;

				if (m_bEmpire && pkChr->GetEmpire() && m_bEmpire != pkChr->GetEmpire())
					return;

				if (pkChr->IsHack())
					return;

				if (!pkChr->CanHandleItem(false, true))
					return;
				
				if (m_bUseWarp)
					pkChr->WarpSet(m_lTargetX, m_lTargetY);
				else
				{
					pkChr->Show(pkChr->GetMapIndex(), m_lTargetX, m_lTargetY);
					pkChr->Stop();
				}
			}

			bool m_bInvalid;
			bool m_bUseWarp;

			long m_lX;
			long m_lY;
			long m_lTargetX;
			long m_lTargetY;
			long m_lTargetMapIndex;

			BYTE m_bEmpire;
	};
}

EVENTFUNC(warp_npc_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "warp_npc_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}	

	if (!ch->GetSectree())
	{
		ch->m_pkWarpNPCEvent = NULL;
		return 0;
	}

	FuncCheckWarp f(ch);
	if (f.Valid())
		ch->GetSectree()->ForEachAround(f);

	return passes_per_sec / 2;
}


void CHARACTER::StartWarpNPCEvent()
{
	if (m_pkWarpNPCEvent)
		return;

	if (!IsWarp() && !IsGoto())
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkWarpNPCEvent = event_create(warp_npc_event, info, passes_per_sec / 2);
}

void CHARACTER::SyncPacket()
{
	network::GCOutputPacket<network::GCSyncPositionPacket> pack;

	auto elem = pack->add_elements();
	elem->set_vid(GetVID());
	elem->set_x(GetX());
	elem->set_y(GetY());

	PacketAround(pack);
}

LPCHARACTER CHARACTER::GetMarryPartner() const
{
	return m_pkChrMarried;
}

void CHARACTER::SetMarryPartner(LPCHARACTER ch)
{
	m_pkChrMarried = ch;
}

int CHARACTER::GetMarriageBonus(DWORD dwItemVnum, bool bSum)
{
	if (IsNPC())
		return 0;

	marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());

	if (!pMarriage)
		return 0;

	return pMarriage->GetBonus(dwItemVnum, bSum, this);
}

void CHARACTER::ConfirmWithMsg(const char* szMsg, int iTimeout, DWORD dwRequestPID)
{
	if (!IsPC())
		return;

	network::GCOutputPacket<network::GCQuestConfirmPacket> p;

	p->set_request_pid(dwRequestPID);
	p->set_timeout(iTimeout);
	p->set_message(szMsg);

	GetDesc()->Packet(p);
}

int CHARACTER::GetPremiumRemainSeconds(BYTE bType) const
{
	if (bType >= PREMIUM_MAX_NUM)
		return 0;

	return m_aiPremiumTimes[bType] - get_global_time();
}

bool CHARACTER::WarpToPID(DWORD dwPID)
{
	LPCHARACTER victim;
	if ((victim = (CHARACTER_MANAGER::instance().FindByPID(dwPID))))
	{
		
#ifdef ENABLE_MESSENGER_BLOCK
		if (!IsGM() && MessengerManager::instance().CheckMessengerList(GetName(), victim->GetName(), SYST_BLOCK))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't use this action because you have blocked %s. "), victim->GetName());
			return false;
		}
#endif

		int mapIdx = victim->GetMapIndex();
		if (IsGM() || IS_SUMMONABLE_ZONE(mapIdx) || GetMapIndex() == mapIdx)
		{
			if (!CAN_ENTER_ZONE_CHECKLEVEL(this, mapIdx, true))
			{
				return false;
			}
			else if (CAN_ENTER_ZONE(this, mapIdx))
			{
				WarpSet(victim->GetX(), victim->GetY(), victim->GetMapIndex());
			}
			else
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ ÀÖ´Â °÷À¸·Î ¿öÇÁÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ ÀÖ´Â °÷À¸·Î ¿öÇÁÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	else
	{
		// ´Ù¸¥ ¼­¹ö¿¡ ·Î±×ÀÎµÈ »ç¶÷ÀÌ ÀÖÀ½ -> ¸Þ½ÃÁö º¸³» ÁÂÇ¥¸¦ ¹Þ¾Æ¿ÀÀÚ
		// 1. A.pid, B.pid ¸¦ »Ñ¸²
		// 2. B.pid¸¦ °¡Áø ¼­¹ö°¡ »Ñ¸°¼­¹ö¿¡°Ô A.pid, ÁÂÇ¥ ¸¦ º¸³¿
		// 3. ¿öÇÁ
		CCI * pcci = P2P_MANAGER::instance().FindByPID(dwPID);

		if (!pcci)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ ¿Â¶óÀÎ »óÅÂ°¡ ¾Æ´Õ´Ï´Ù."));
			return false;
		}

#ifdef ENABLE_MESSENGER_BLOCK
		if (!IsGM() && MessengerManager::instance().CheckMessengerList(GetName(), pcci->szName, SYST_BLOCK))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can't use this action because you have blocked %s. "), pcci->szName);
			return false;
		}
#endif

		/*if (pcci->bChannel != g_bChannel && pcci->bChannel != 99 && g_bChannel != 99)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ %d Ã¤³Î¿¡ ÀÖ½À´Ï´Ù. (ÇöÀç Ã¤³Î %d)"), pcci->bChannel, g_bChannel);
			return false;
		}
		else */
	
		if ((!IsGM() && !test_server) && !IS_SUMMONABLE_ZONE(pcci->lMapIndex) && ((GetMapIndex() != pcci->lMapIndex)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You cannot warp with the marriage ring to this place."));
			return false;
		}
		else
		{
			if (!CAN_ENTER_ZONE_CHECKLEVEL(this, pcci->lMapIndex, true))
			{
				return false;
			}

			if (!CAN_ENTER_ZONE(this, pcci->lMapIndex))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "»ó´ë¹æÀÌ ÀÖ´Â °÷À¸·Î ¿öÇÁÇÒ ¼ö ¾ø½À´Ï´Ù."));
				return false;
			}

			network::GGOutputPacket<network::GGFindPositionPacket> p;
			p->set_from_pid(GetPlayerID());
			p->set_target_pid(dwPID);
			p->set_is_gm(IsGM());
			pcci->pkDesc->Packet(p);

			if (test_server) 
				ChatPacket(CHAT_TYPE_PARTY, "sent find position packet for teleport");
		}
	}
	return true;
}

// ADD_REFINE_BUILDING
CGuild* CHARACTER::GetRefineGuild() const
{
	LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);

	return (chRefineNPC ? chRefineNPC->GetGuild() : NULL);
}

bool CHARACTER::IsRefineThroughGuild() const
{
	return GetRefineGuild() != NULL;
}

int CHARACTER::ComputeRefineFee(int iCost, int iMultiply) const
{
	CGuild* pGuild = GetRefineGuild();
	if (pGuild)
	{
		if (pGuild == GetGuild())
			return iCost * iMultiply * 9 / 10;

		// ´Ù¸¥ Á¦±¹ »ç¶÷ÀÌ ½ÃµµÇÏ´Â °æ¿ì Ãß°¡·Î 3¹è ´õ
		LPCHARACTER chRefineNPC = CHARACTER_MANAGER::instance().Find(m_dwRefineNPCVID);
		if (chRefineNPC && chRefineNPC->GetEmpire() != GetEmpire())
			return iCost * iMultiply * 3;

		return iCost * iMultiply;
	}
	else
		return iCost;
}

void CHARACTER::PayRefineFee(int iTotalMoney)
{
	int iFee = iTotalMoney / 10;
	CGuild* pGuild = GetRefineGuild();

	int iRemain = iTotalMoney;

	if (pGuild)
	{
		// ÀÚ±â ±æµåÀÌ¸é iTotalMoney¿¡ ÀÌ¹Ì 10%°¡ Á¦¿ÜµÇ¾îÀÖ´Ù
		if (pGuild != GetGuild())
		{
			pGuild->RequestDepositMoney(this, iFee);
			iRemain -= iFee;
		}
	}

	PointChange(POINT_GOLD, -iRemain);
}
// END_OF_ADD_REFINE_BUILDING

//Hack ¹æÁö¸¦ À§ÇÑ Ã¼Å©.
bool CHARACTER::IsHack(bool bSendMsg, bool bCheckShopOwner, int limittime)
{
	if (IsGM())
		return false;

	const int iPulse = thecore_pulse();

	if (test_server)
		bSendMsg = true;

	//Ã¢°í ¿¬ÈÄ Ã¼Å©
	if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¢°í¸¦ ¿¬ÈÄ %dÃÊ ÀÌ³»¿¡´Â ´Ù¸¥°÷À¸·Î ÀÌµ¿ÇÒ¼ö ¾ø½À´Ï´Ù."), limittime);

		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(limittime));
		return true; 
	}

	//Ã¢°í ¿¬ÈÄ Ã¼Å©
	if (iPulse - GetGoldTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have to wait a few seconds after changing your gold."), limittime);

		return true;
	}

	//°Å·¡°ü·Ã Ã¢ Ã¼Å©
	if (bCheckShopOwner)
	{
		if (!CanShopNow() || GetMyShop() || GetShop())
		{
			if (bSendMsg)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â ´Ù¸¥°÷À¸·Î ÀÌµ¿,Á¾·á ÇÒ¼ö ¾ø½À´Ï´Ù"));

			return true;
		}
	}
	else
	{
		if (!CanShopNow() || GetMyShop())
		{
			if (bSendMsg)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â ´Ù¸¥°÷À¸·Î ÀÌµ¿,Á¾·á ÇÒ¼ö ¾ø½À´Ï´Ù"));

			return true;
		}
	}

	//PREVENT_PORTAL_AFTER_EXCHANGE
	//±³È¯ ÈÄ ½Ã°£Ã¼Å©
	if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡ ÈÄ %dÃÊ ÀÌ³»¿¡´Â ´Ù¸¥Áö¿ªÀ¸·Î ÀÌµ¿ ÇÒ ¼ö ¾ø½À´Ï´Ù."), limittime );
		return true;
	}
	//END_PREVENT_PORTAL_AFTER_EXCHANGE

	//PREVENT_ITEM_COPY
	if (iPulse - GetMyShopTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°Å·¡ ÈÄ %dÃÊ ÀÌ³»¿¡´Â ´Ù¸¥Áö¿ªÀ¸·Î ÀÌµ¿ ÇÒ ¼ö ¾ø½À´Ï´Ù."), limittime);
		return true;
	}

	if (iPulse - GetRefineTime() < PASSES_PER_SEC(limittime))
	{
		if (bSendMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¾ÆÀÌÅÛ °³·®ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), limittime);
		return true; 
	}
	//END_PREVENT_ITEM_COPY

	return false;
}

void CHARACTER::Say(const std::string & s)
{
	network::GCOutputPacket<network::GCScriptPacket> packet_script;

	packet_script->set_skin(1);
	packet_script->set_script(s);

	if (IsPC())
		GetDesc()->Packet(packet_script);
}

//------------------------------------------------
void CHARACTER::UpdateDepositPulse()
{
	m_deposit_pulse = thecore_pulse() + PASSES_PER_SEC(60*5);	// 5ºÐ
}

bool CHARACTER::CanDeposit() const
{
	return (m_deposit_pulse == 0 || (m_deposit_pulse < thecore_pulse()));
}
//------------------------------------------------

ESex GET_SEX(LPCHARACTER ch)
{
	switch (ch->GetRaceNum())
	{
		case MAIN_RACE_WARRIOR_M:
		case MAIN_RACE_SURA_M:
		case MAIN_RACE_ASSASSIN_M:
		case MAIN_RACE_SHAMAN_M:
#ifdef __WOLFMAN__
		case MAIN_RACE_WOLFMAN_M:
#endif
			return SEX_MALE;

		case MAIN_RACE_ASSASSIN_W:
		case MAIN_RACE_SHAMAN_W:
		case MAIN_RACE_WARRIOR_W:
		case MAIN_RACE_SURA_W:
			return SEX_FEMALE;
	}

	/* default sex = male */
	return SEX_MALE;
}

int CHARACTER::GetHPPct() const
{
	return (GetHP() * 100) / GetMaxHP();
}

bool CHARACTER::IsBerserk() const
{
	if (m_pkMobInst != NULL)
		return m_pkMobInst->m_IsBerserk;
	else
		return false;
}

void CHARACTER::SetBerserk(bool mode)
{
	if (m_pkMobInst != NULL)
		m_pkMobInst->m_IsBerserk = mode;
}

bool CHARACTER::IsGodSpeed() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsGodSpeed;
	}
	else
	{
		return false;
	}
}

void CHARACTER::SetGodSpeed(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsGodSpeed = mode;

		if (mode == true)
		{
			SetPoint(POINT_ATT_SPEED, 250);
		}
		else
		{
			SetPoint(POINT_ATT_SPEED, m_pkMobData->m_table.attack_speed());
		}
	}
}

bool CHARACTER::IsDeathBlow() const
{
#ifdef __DISABLE_DEATHBLOW__
	return false;
#endif
	
	if (random_number(1, 100) <= m_pkMobData->m_table.death_blow_point())
	{
		return true;
	}
	else
	{
		return false;
	}
}

struct FFindReviver
{
	FFindReviver()
	{
		pChar = NULL;
		HasReviver = false;
	}
	
	void operator() (LPCHARACTER ch)
	{
		if (ch->IsMonster() != true)
		{
			return;
		}

		if (ch->IsReviver() == true && pChar != ch && ch->IsDead() != true)
		{
			if (random_number(1, 100) <= ch->GetMobTable().revive_point())
			{
				HasReviver = true;
				pChar = ch;
			}
		}
	}

	LPCHARACTER pChar;
	bool HasReviver;
};

bool CHARACTER::HasReviverInParty() const
{
	LPPARTY party = GetParty();

	if (party != NULL)
	{
		if (party->GetMemberCount() == 1) return false;

		FFindReviver f;
		party->ForEachMemberPtr(f);
		return f.HasReviver;
	}

	return false;
}

bool CHARACTER::IsRevive() const
{
	if (m_pkMobInst != NULL)
	{
		return m_pkMobInst->m_IsRevive;
	}

	return false;
}

void CHARACTER::SetRevive(bool mode)
{
	if (m_pkMobInst != NULL)
	{
		m_pkMobInst->m_IsRevive = mode;
	}
}

#define IS_SPEED_HACK_PLAYER(ch) (ch->m_speed_hack_count > SPEEDHACK_LIMIT_COUNT*3)

EVENTFUNC(check_speedhack_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "check_speedhack_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (NULL == ch || ch->IsNPC())
		return 0;

	if (IS_SPEED_HACK_PLAYER(ch))
	{
		// write hack log
		LogManager::instance().SpeedHackLog(ch->GetPlayerID(), ch->GetX(), ch->GetY(), ch->m_speed_hack_count);
	}

	ch->m_speed_hack_count = 0;

	ch->ResetComboHackCount();
	return PASSES_PER_SEC(60);
}

void CHARACTER::StartCheckSpeedHackEvent()
{
	if (m_pkCheckSpeedHackEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkCheckSpeedHackEvent = event_create(check_speedhack_event, info, PASSES_PER_SEC(60));	// 1ºÐ
}

void CHARACTER::GoHome()
{
	WarpSet(EMPIRE_START_X(GetEmpire()), EMPIRE_START_Y(GetEmpire()));
}

void CHARACTER::SendGuildName(CGuild* pGuild)
{
	if (NULL == pGuild) return;

	DESC	*desc = GetDesc();

	if (NULL == desc) return;
	if (m_known_guild.find(pGuild->GetID()) != m_known_guild.end()) return;

	m_known_guild.insert(pGuild->GetID());

	network::GCOutputPacket<network::GCGuildNamePacket> pack;

	auto elem = pack->add_names();
	elem->set_guild_id(pGuild->GetID());
	elem->set_name(pGuild->GetName());

	desc->Packet(pack);
}

void CHARACTER::SendGuildName(DWORD dwGuildID)
{
	SendGuildName(CGuildManager::instance().FindGuild(dwGuildID));
}

EVENTFUNC(destroy_when_idle_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );
	if ( info == NULL )
	{
		sys_err( "destroy_when_idle_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}	

	if (ch->GetVictim())
	{
		return PASSES_PER_SEC(300);
	}

	sys_log(1, "DESTROY_WHEN_IDLE: %s", ch->GetName());

	ch->m_pkDestroyWhenIdleEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

void CHARACTER::StartDestroyWhenIdleEvent()
{
	if (m_pkDestroyWhenIdleEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;

	m_pkDestroyWhenIdleEvent = event_create(destroy_when_idle_event, info, PASSES_PER_SEC(300));
}

void CHARACTER::SetComboSequence(BYTE seq)
{
	m_bComboSequence = seq;
}

BYTE CHARACTER::GetComboSequence() const
{
	return m_bComboSequence;
}

void CHARACTER::SetLastComboTime(DWORD time)
{
	m_dwLastComboTime = time;
}

DWORD CHARACTER::GetLastComboTime() const
{
	return m_dwLastComboTime;
}

void CHARACTER::SetValidComboInterval(int interval)
{
	m_iValidComboInterval = interval;
}

int CHARACTER::GetValidComboInterval() const
{
	return m_iValidComboInterval;
}

BYTE CHARACTER::GetComboIndex() const
{
	return m_bComboIndex;
}

void CHARACTER::IncreaseComboHackCount(int k)
{
	m_iComboHackCount += k;

	if (m_iComboHackCount >= 10)
	{
		if (GetDesc())
			if (GetDesc()->DelayedDisconnect(random_number(2, 7)))
			{
				sys_log(0, "COMBO_HACK_DISCONNECT: %s count: %d", GetName(), m_iComboHackCount);
				LogManager::instance().HackLog("Combo", this);
			}
	}
}

void CHARACTER::ResetComboHackCount()
{
	m_iComboHackCount = 0;
}

void CHARACTER::SkipComboAttackByTime(int interval)
{
	m_dwSkipComboAttackByTime = get_dword_time() + interval;
}

DWORD CHARACTER::GetSkipComboAttackByTime() const
{
	return m_dwSkipComboAttackByTime;
}

void CHARACTER::ResetChatCounter()
{
	m_bChatCounter = 0;
}

BYTE CHARACTER::IncreaseChatCounter()
{
	return ++m_bChatCounter;
}

BYTE CHARACTER::GetChatCounter() const
{
	return m_bChatCounter;
}

#ifdef COMBAT_ZONE
void CHARACTER::UpdateCombatZoneRankings(const char* memberName, DWORD memberEmpire, DWORD memberPoints)
{
	DBManager::instance().Query("INSERT INTO player.combat_zone_ranking_weekly (memberName, memberEmpire, memberPoints) VALUES('%s', '%d', '%d') ON DUPLICATE KEY UPDATE memberPoints = memberPoints + '%d'", memberName, memberEmpire, memberPoints, memberPoints);
	DBManager::instance().Query("INSERT INTO player.combat_zone_ranking_general (memberName, memberEmpire, memberPoints) VALUES('%s', '%d', '%d') ON DUPLICATE KEY UPDATE memberPoints = memberPoints + '%d'", memberName, memberEmpire, memberPoints, memberPoints);
}

BYTE CHARACTER::GetCombatZoneRank()
{
	if (GetDesc() != NULL)
	{
		// std::auto_ptr<SQLMsg> pMsg(DBManager::instance().DirectQuery("SELECT combat_zone_rank FROM player.player WHERE id = %u", GetPlayerID()));
		// MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
		// return atoi(row[0]);
		return CCombatZoneManager::instance().GetPlayerRank(this);
	}

	return 0;
}
#endif

bool CHARACTER::CanWarp() const
{
	const int iPulse = thecore_pulse();
	const int limit_time = PASSES_PER_SEC(g_nPortalLimitTime);
	const int gold_limit_time = PASSES_PER_SEC(g_nPortalGoldLimitTime);

	if ((iPulse - GetSafeboxLoadTime()) < limit_time)
		return false;

	if ((iPulse - GetExchangeTime()) < limit_time)
		return false;

	if ((iPulse - GetMyShopTime()) < limit_time)
		return false;

	if ((iPulse - GetRefineTime()) < limit_time)
		return false;

	if ((iPulse - GetGoldTime()) < gold_limit_time)
		return false;

	if (!CanShopNow() || GetMyShop() || GetShop())
		return false;

	if (IsPVPFighting())
		return false;

	return true;
}

DWORD CHARACTER::GetNextExp() const
{
	if (PLAYER_EXP_TABLE_MAX < GetLevel())
		return 2500000000;
	else
		return exp_table[GetLevel()];
}

int	CHARACTER::GetSkillPowerByLevel(int level, bool bMob) const
{
	return CTableBySkill::instance().GetSkillPowerByLevelFromType(GetJob(), GetSkillGroup(), MINMAX(0, level, SKILL_MAX_LEVEL), bMob); 
}

int CHARACTER::ChangeEmpire(BYTE empire)
{
	if (GetEmpire() == empire)
		return 1;

	char szQuery[1024 + 1];
	DWORD dwAID;
	DWORD dwPID[4];
	memset(dwPID, 0, sizeof(dwPID));

	{
		// 1. ³» °èÁ¤ÀÇ ¸ðµç pid¸¦ ¾ò¾î ¿Â´Ù
		snprintf(szQuery, sizeof(szQuery),
			"SELECT id, pid1, pid2, pid3, pid4 FROM player_index WHERE pid1=%u OR pid2=%u OR pid3=%u OR pid4=%u AND empire=%u",
			GetPlayerID(), GetPlayerID(), GetPlayerID(), GetPlayerID(), GetEmpire());

		std::auto_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));

		if (msg->Get()->uiNumRows == 0)
		{
			return 0;
		}

		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		str_to_number(dwAID, row[0]);
		str_to_number(dwPID[0], row[1]);
		str_to_number(dwPID[1], row[2]);
		str_to_number(dwPID[2], row[3]);
		str_to_number(dwPID[3], row[4]);
	}

	const int loop = 4;

	{
		// 2. °¢ Ä³¸¯ÅÍÀÇ ±æµå Á¤º¸¸¦ ¾ò¾î¿Â´Ù.
		//   ÇÑ Ä³¸¯ÅÍ¶óµµ ±æµå¿¡ °¡ÀÔ µÇ¾î ÀÖ´Ù¸é, Á¦±¹ ÀÌµ¿À» ÇÒ ¼ö ¾ø´Ù.
		DWORD dwGuildID[4];
		CGuild * pGuild[4];
		SQLMsg * pMsg = NULL;

		for (int i = 0; i < loop; ++i)
		{
			snprintf(szQuery, sizeof(szQuery), "SELECT guild_id FROM guild_member WHERE pid=%u", dwPID[i]);

			pMsg = DBManager::instance().DirectQuery(szQuery);

			if (pMsg != NULL)
			{
				if (pMsg->Get()->uiNumRows > 0)
				{
					MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

					str_to_number(dwGuildID[i], row[0]);

					pGuild[i] = CGuildManager::instance().FindGuild(dwGuildID[i]);

					if (pGuild[i] != NULL)
					{
						M2_DELETE(pMsg);
						return 2;
					}
				}
				else
				{
					dwGuildID[i] = 0;
					pGuild[i] = NULL;
				}

				M2_DELETE(pMsg);
			}
		}
	}

	{
		// 3. °¢ Ä³¸¯ÅÍÀÇ °áÈ¥ Á¤º¸¸¦ ¾ò¾î¿Â´Ù.
		//   ÇÑ Ä³¸¯ÅÍ¶óµµ °áÈ¥ »óÅÂ¶ó¸é Á¦±¹ ÀÌµ¿À» ÇÒ ¼ö ¾ø´Ù.
		for (int i = 0; i < loop; ++i)
		{
			if (marriage::CManager::instance().IsEngagedOrMarried(dwPID[i]) == true)
				return 3;
		}
	}

	{
		// 4. dbÀÇ Á¦±¹ Á¤º¸¸¦ ¾÷µ¥ÀÌÆ® ÇÑ´Ù.
		snprintf(szQuery, sizeof(szQuery), "UPDATE player_index SET empire=%u WHERE pid1=%u OR pid2=%u OR pid3=%u OR pid4=%u AND empire=%u",
			empire, GetPlayerID(), GetPlayerID(), GetPlayerID(), GetPlayerID(), GetEmpire());

		std::auto_ptr<SQLMsg> msg(DBManager::instance().DirectQuery(szQuery));

		if (msg->Get()->uiAffectedRows > 0)
		{
			return 999;
		}
	}

	return 0;
}

DWORD CHARACTER::GetAID() const
{
	if (GetDesc())
	{
		auto& rkTab = GetDesc()->GetAccountTable();
		if (rkTab.id())
			return rkTab.id();
	}

	char szQuery[1024 + 1];
	DWORD dwAID = 0;

	snprintf(szQuery, sizeof(szQuery), "SELECT id FROM player_index WHERE pid1=%u OR pid2=%u OR pid3=%u OR pid4=%u AND empire=%u",
		GetPlayerID(), GetPlayerID(), GetPlayerID(), GetPlayerID(), GetEmpire());

	SQLMsg* pMsg = DBManager::instance().DirectQuery(szQuery);

	if (pMsg != NULL)
	{
		if (pMsg->Get()->uiNumRows == 0)
		{
			M2_DELETE(pMsg);
			return 0;
		}

		MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		str_to_number(dwAID, row[0]);

		M2_DELETE(pMsg);

		return dwAID;
	}
	else
	{
		return 0;
	}
}

bool CHARACTER::IsNearWater() const
{
	if (!GetSectree())
		return false;

	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			if (IS_SET(GetSectree()->GetAttribute(GetX() + x * 100, GetY() + y * 100), ATTR_WATER))
				return true;
		}
	}

	return false;
}

bool CHARACTER::IsPrivateMap(long lMapIndex) const
{
	if (lMapIndex)
		return GetMapIndex() >= lMapIndex * 10000 && GetMapIndex() <= lMapIndex * 10000 + 9999;

	return GetMapIndex() >= 10000;
}

void CHARACTER::SetPVPTeam(short sTeam)
{
	if (m_sPVPTeam == sTeam)
		return;

	m_sPVPTeam = sTeam;

	network::GCOutputPacket<network::GCPVPTeamPacket> packet;
	packet->set_vid(GetVID());
	packet->set_team(sTeam);
	PacketView(packet);
}

BYTE CHARACTER::GetLanguageID() const
{
	if (GetDesc())
		return GetDesc()->GetAccountTable().language();

#ifdef __FAKE_PC__
	if (m_pkFakePCOwner)
		return m_pkFakePCOwner->GetDesc()->GetAccountTable().language();
#endif

#ifdef __FAKE_BUFF__
	if (m_pkFakeBuffOwner)
		return m_pkFakeBuffOwner->GetLanguageID();
#endif

	return LANGUAGE_DEFAULT;
}

void CHARACTER::LoadOfflineMessages(const ::google::protobuf::RepeatedPtrField<network::TOfflineMessage>& elements)
{
	for (auto& message : elements)
		m_vec_OfflineMessages.push_back(message);
}

void CHARACTER::SendOfflineMessages()
{
	if (!GetDesc())
		return;

#ifdef ACCOUNT_TRADE_BLOCK
	if (GetDesc()->IsTradeblocked())
	{
		tchat("msg aborted, unverified, will be saved");
		SaveOfflineMessages();
		return;
	}
#endif

	network::GCOutputPacket<network::GCWhisperPacket> pack;
	for (int i = 0; i < m_vec_OfflineMessages.size(); ++i)
	{
		tchat("msg \"%s\" isGM %d", m_vec_OfflineMessages[i].message().c_str(), m_vec_OfflineMessages[i].is_gm());
		pack->set_type(m_vec_OfflineMessages[i].is_gm() ? WHISPER_TYPE_GM : WHISPER_TYPE_NORMAL);
		pack->set_name_from(m_vec_OfflineMessages[i].sender());
		pack->set_message(m_vec_OfflineMessages[i].message());

		GetDesc()->Packet(pack);
	}

	m_vec_OfflineMessages.clear();
}

void CHARACTER::SaveOfflineMessages()
{
	char szEscapedSender[CHARACTER_NAME_MAX_LEN + 1];
	char szEscapedMessage[CHAT_MAX_LEN * 2 + 1];
	for (int i = 0; i < m_vec_OfflineMessages.size(); ++i)
	{
		DBManager::instance().EscapeString(szEscapedSender, sizeof(szEscapedSender), m_vec_OfflineMessages[i].sender().c_str(), m_vec_OfflineMessages[i].sender().length());
		DBManager::instance().EscapeString(szEscapedMessage, sizeof(szEscapedMessage), m_vec_OfflineMessages[i].message().c_str(), m_vec_OfflineMessages[i].message().length());
		DBManager::instance().Query("INSERT INTO offline_messages (pid, sender, message, is_gm) VALUES (%u, '%s', '%s', %u)",
			GetPlayerID(), szEscapedSender, szEscapedMessage, m_vec_OfflineMessages[i].is_gm());
	}

	m_vec_OfflineMessages.clear();
}

#ifdef __ITEM_REFUND__
void CHARACTER::AddItemRefundCommand(const char *format, ...)
{
	char cmdbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	int len = vsnprintf(cmdbuf, sizeof(cmdbuf), format, args);
	va_end(args);

	m_vec_ItemRefundCommands.push_back(cmdbuf);

	if (GetDesc()->IsPhase(PHASE_GAME) || GetDesc()->IsPhase(PHASE_DEAD))
		SendItemRefundCommands();
}

void CHARACTER::SendItemRefundCommands()
{
	for (int i = 0; i < m_vec_ItemRefundCommands.size(); ++i)
		ChatPacket(CHAT_TYPE_COMMAND, "%s", m_vec_ItemRefundCommands[i].c_str());
	m_vec_ItemRefundCommands.clear();
}
#endif

bool CHARACTER::ChangeChannel(long lNewChannelHost, int iNewChannel)
{
	if (!IsPC())
		return false;

	if (g_bChannel == 99 || g_bChannel == iNewChannel)
		return false;

	if (GetDungeon() || IsHack(false, true, 10) || GetWarMap())
		return false;

	long lAddr, lMapIndex;
	WORD wPort;
	long x = GetX(), y = GetY();

	if (!CMapLocation::instance().Get(x, y, lMapIndex, lAddr, wPort))
		return false;

	Stop();
	Save();

	if (GetSectree())
	{
		GetSectree()->RemoveEntity(this);
		ViewCleanup();
		EncodeRemovePacket(this);
	}

	network::GCOutputPacket<network::GCWarpPacket> p;
	p->set_x(x);
	p->set_y(y);
	p->set_addr(lNewChannelHost);
	p->set_port(mother_port - (g_bChannel - iNewChannel) * 100);
	GetDesc()->Packet(p);

	return true;
}

void CHARACTER::SetInventoryMaxNum(WORD wMaxNum, BYTE invType)
{
	network::GCOutputPacket<network::GCInventoryMaxNumPacket> pack;
	pack->set_inv_type(invType);

	if (invType == INVENTORY_SIZE_TYPE_NORMAL)
	{
		m_wInventoryMaxNum = wMaxNum;
		pack->set_max_num(GetInventoryMaxNum());
	}
	else if (invType == INVENTORY_SIZE_TYPE_UPPITEM)
	{
		m_wUppitemInventoryMaxNum = wMaxNum;
		pack->set_max_num(GetUppitemInventoryMaxNum());
	}
	else if (invType == INVENTORY_SIZE_TYPE_SKILLBOOK)
	{
		m_wSkillbookInventoryMaxNum = wMaxNum;
		pack->set_max_num(GetSkillbookInventoryMaxNum());
	}
	else if (invType == INVENTORY_SIZE_TYPE_STONE)
	{
		m_wStoneInventoryMaxNum = wMaxNum;
		pack->set_max_num(GetStoneInventoryMaxNum());
	}
	else if (invType == INVENTORY_SIZE_TYPE_ENCHANT)
	{
		m_wEnchantInventoryMaxNum = wMaxNum;
		pack->set_max_num(GetEnchantInventoryMaxNum());
	}
	else
		return;

	GetDesc()->Packet(pack);
}

#ifdef INCREASE_ITEM_STACK
bool CHARACTER::DestroyItem(TItemPos Cell, WORD num)
#else
bool CHARACTER::DestroyItem(TItemPos Cell, BYTE num)
#endif
{
	if (!CanHandleItem())
		return false;

	LPITEM pkItem = GetItem(Cell);
	if (!pkItem)
		return false;

	if (pkItem->GetWindow() != INVENTORY && !ITEM_MANAGER::instance().IsNewWindow(pkItem->GetWindow()))
		return false;

	if (pkItem->IsExchanging())
		return false;

	if (pkItem->isLocked())
		return false;

#ifdef ACCOUNT_TRADE_BLOCK
	if (this->GetDesc()->IsTradeblocked())
	{
		this->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return false;
	}
#endif

	if (IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_DESTROY))
		return false;

	if (quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID()))
	{
		if (pPC->IsRunning() && GetQuestItemPtr() == pkItem)
		{
			sys_err("cannot delete item that is used in quest");
			return false;
		}
	}

	if (quest::CQuestManager::instance().GetCurrentSelectedNPCCharacterPtr() == this)
		quest::CQuestManager::instance().SetCurrentSelectedNPCCharacterPtr(NULL);

	if (!num)
		return false;

	LogManager::instance().ItemDestroyLog(LogManager::ITEM_DESTROY_REMOVE, pkItem, MIN(num, pkItem->GetCount()));

	if (num >= pkItem->GetCount())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have destroyed %s."), pkItem->GetName(GetLanguageID()));
		ITEM_MANAGER::instance().RemoveItem(pkItem, "RECV_REMOVE");
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You have destroyed %dx %s."), num, pkItem->GetName(GetLanguageID()));
		pkItem->SetCount(pkItem->GetCount() - num);
	}

	return true;
}

#ifdef __SWITCHBOT__
EVENTFUNC(switchbot_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("switchbot_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	return ch->OnSwitchbotEvent();
}

void CHARACTER::StartSwitchbotEvent()
{
	event_cancel(&m_pkSwitchbotEvent);

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkSwitchbotEvent = event_create(switchbot_event, info, PASSES_PER_SEC(m_dwSwitchbotSpeed) / 1000);
}

void CHARACTER::ChangeSwitchbotSpeed(DWORD dwNewMS)
{
	if (PASSES_PER_SEC(m_dwSwitchbotSpeed) / 1000 == PASSES_PER_SEC(dwNewMS) / 1000)
		return;

	m_dwSwitchbotSpeed = dwNewMS;
}

void CHARACTER::AppendSwitchbotData(const network::TSwitchbotTable* c_pkSwitchbot)
{
	if (m_map_Switchbots.find(c_pkSwitchbot->item_id()) != m_map_Switchbots.end())
		return;

	m_map_Switchbots[c_pkSwitchbot->item_id()] = *c_pkSwitchbot;

	if (!m_pkSwitchbotEvent)
		StartSwitchbotEvent();

	LPITEM pkItem = FindItemByID(c_pkSwitchbot->item_id());
	if (pkItem)
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s was started."), pkItem->GetName(GetLanguageID()));
}

void CHARACTER::AppendSwitchbotDataPremium(const network::TSwitchbotTable* c_pkSwitchbot)
{
	itertype(m_map_Switchbots) it = m_map_Switchbots.find(c_pkSwitchbot->item_id());
	if (it == m_map_Switchbots.end() || it->second.use_premium())
		return;

	*it->second.mutable_premium_attrs() = c_pkSwitchbot->premium_attrs();
	it->second.set_use_premium(true);

	if (!m_pkSwitchbotEvent)
		StartSwitchbotEvent();

	LPITEM pkItem = FindItemByID(c_pkSwitchbot->item_id());
	if (pkItem)
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Premium options for %s has been added."), pkItem->GetName(GetLanguageID()));
}

void CHARACTER::RemoveSwitchbotData(DWORD dwItemID)
{
	itertype(m_map_Switchbots) it = m_map_Switchbots.find(dwItemID);
	if (it == m_map_Switchbots.end())
		return;

	if (!it->second.finished())
		ChatPacket(CHAT_TYPE_COMMAND, "switchbot_end %u", it->second.inv_cell());

	m_map_Switchbots.erase(it);

	if (m_map_Switchbots.size() == 0)
		event_cancel(&m_pkSwitchbotEvent);
}

void CHARACTER::RemoveSwitchbotDataBySlot(WORD wSlot)
{
	for (itertype(m_map_Switchbots) it = m_map_Switchbots.begin(); it != m_map_Switchbots.end(); ++it)
	{
		if (test_server)
			ChatPacket(CHAT_TYPE_INFO, "RemoveSwitchbotDataBySlot %u (check with curSlot %d)", wSlot, it->second.inv_cell());

		if (it->second.inv_cell() == wSlot)
		{
			LPITEM pkItem = FindItemByID(it->second.item_id());
			if (!pkItem)
				pkItem = ITEM_MANAGER::instance().Find(it->second.item_id());
			if (pkItem)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s was stopped."), pkItem->GetName(GetLanguageID()));
			RemoveSwitchbotData(it->second.item_id());
			return;
		}
	}
}

DWORD CHARACTER::OnSwitchbotEvent()
{
	std::set<DWORD> set_Remove;

	DWORD dwTimeRefresh = PASSES_PER_SEC(m_dwSwitchbotSpeed) / 1000;

	DWORD c_dwSwitcherVnum = 71084;
	DWORD c_dwSwitcherVnum2 = 71151;
	DWORD c_dwSwitcherVnum3 = 92870;
	DWORD c_dwSwitcherVnum4 = 95010;
#ifdef ELONIA
	DWORD c_dwSwitcherVnum5 = 76023;
	DWORD c_dwSwitcherVnum6 = 76014;
#endif
	DWORD dwUseSwitcherVnum = c_dwSwitcherVnum;
	int iSwitcherCount = 	CountSpecifyItem(c_dwSwitcherVnum) + CountSpecifyItem(c_dwSwitcherVnum2) + 
							CountSpecifyItem(c_dwSwitcherVnum3) + CountSpecifyItem(c_dwSwitcherVnum4); 
#ifdef ELONIA
	iSwitcherCount +=		CountSpecifyItem(c_dwSwitcherVnum5) + CountSpecifyItem(c_dwSwitcherVnum6);
#endif

	if (iSwitcherCount == 0)
	{
		while (!m_map_Switchbots.empty())
			RemoveSwitchbotData(m_map_Switchbots.begin()->first);
		ChatPacket(CHAT_TYPE_INFO, "The switching has been stopped because you have no switchers anymore.");
		return 0;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(GetPlayerID());
	if (pPC && pPC->IsRunning())
		return dwTimeRefresh;

	for (itertype(m_map_Switchbots) it = m_map_Switchbots.begin(); it != m_map_Switchbots.end(); ++it)
	{
		if (iSwitcherCount == 0)
			break;

		auto& rkTab = it->second;

		LPITEM pkItem = FindItemByID(rkTab.item_id());
		if (!pkItem)
		{
			pkItem = ITEM_MANAGER::instance().Find(rkTab.item_id());
			if (pkItem)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s was stopped because it's no longer in your inventory."), pkItem->GetName(GetLanguageID()));
			else
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of an item was stopped because it's no longer in your inventory."));
			set_Remove.insert(rkTab.item_id());
			continue;
		}

		if (!pkItem->GetAttributeType(0))
		{
			ChatPacket(CHAT_TYPE_INFO, "No Bonus in item.");
			set_Remove.insert(rkTab.item_id());
			continue;
		}
		
		if (!CanHandleItem() || pkItem->IsExchanging() || pkItem->GetWindow() != INVENTORY)
			continue;

		bool isZodiacItem = false;
		const int itemAmount = 6;
		const int Zodiacs[itemAmount] = { 310, 1180, 2200, 3220, 5160, 7300 };
		for (int i = 0; i < 6; ++i)
		{
			for (int j = 0; j < 10; ++j)
			{
				if (pkItem->GetVnum() == Zodiacs[i] + j)
				{
					//iSwitcherCount = CountSpecifyItem(c_dwSwitcherVnum4);
					//dwUseSwitcherVnum = c_dwSwitcherVnum4;
					isZodiacItem = true;
					break;
				}
			}
		}

		if (CountSpecifyItem(c_dwSwitcherVnum3))
		{
			pkItem->SetGMOwner(true);
			dwUseSwitcherVnum = c_dwSwitcherVnum3;
		}
#ifdef ELONIA
		else if (CountSpecifyItem(c_dwSwitcherVnum5))
		{
			dwUseSwitcherVnum = c_dwSwitcherVnum5;
		}
		else if (CountSpecifyItem(c_dwSwitcherVnum6))
		{
			dwUseSwitcherVnum = c_dwSwitcherVnum6;
		}
#endif
		else if (CountSpecifyItem(c_dwSwitcherVnum2))
		{
			bool bCanUseLowSwitcher = true;
			for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
			{
				if (pkItem->GetLimitType(i) == LIMIT_LEVEL && pkItem->GetLimitValue(i) > 40)
				{
					bCanUseLowSwitcher = false;
					dwUseSwitcherVnum = c_dwSwitcherVnum;
					break;
				}
			}
			if (bCanUseLowSwitcher)
				dwUseSwitcherVnum = c_dwSwitcherVnum2;
			else
			{
				iSwitcherCount = CountSpecifyItem(c_dwSwitcherVnum);

				if (iSwitcherCount == 0)
				{
					while (!m_map_Switchbots.empty())
						RemoveSwitchbotData(m_map_Switchbots.begin()->first);
					ChatPacket(CHAT_TYPE_INFO, "The switching has been stopped because you have no switchers anymore.");
					return 0;
				}
			}
		}

		if (isZodiacItem)
		{
			dwUseSwitcherVnum = c_dwSwitcherVnum4;
			iSwitcherCount = CountSpecifyItem(c_dwSwitcherVnum4);

			if (iSwitcherCount == 0)
			{
				while (!m_map_Switchbots.empty())
					RemoveSwitchbotData(m_map_Switchbots.begin()->first);

				ChatPacket(CHAT_TYPE_INFO, "The switching has been stopped because you have no switchers anymore.");
				return 0;
			}
		}

		iSwitcherCount = CountSpecifyItem(dwUseSwitcherVnum);
		if (iSwitcherCount == 0)
		{
			while (!m_map_Switchbots.empty())
				RemoveSwitchbotData(m_map_Switchbots.begin()->first);
			ChatPacket(CHAT_TYPE_INFO, "The switching has been stopped because you have no switchers anymore.");
			return 0;
		}

		bool bCheckAttr = true;
		for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
		{
			if (rkTab.attrs_size() > i && rkTab.attrs(i).type() != 0 && rkTab.attrs(i).value() != 0)
			{
				bool bHasAttr = false;
				for (int j = 0; j < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++j)
				{
					if (pkItem->GetAttributeType(j) == rkTab.attrs(i).type() && pkItem->GetAttributeValue(j) >= rkTab.attrs(i).value())
					{
						bHasAttr = true;
						break;
					}
				}

				if (!bHasAttr)
				{
				//	if (test_server)
				//		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Attribute %d val %d not found [attr0 %d %d]"), rkTab.kAttrs[i].bType, rkTab.kAttrs[i].iValue, pkItem->GetAttributeType(0), pkItem->GetAttributeValue(0));

					bCheckAttr = false;
					break;
				}
			}
		}

		if (!bCheckAttr && rkTab.use_premium())
		{
			bool bCheckAttrPremium = true;
			for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
			{
				if (rkTab.premium_attrs_size() > i && rkTab.premium_attrs(i).type() != 0 && rkTab.premium_attrs(i).value() != 0)
				{
					bool bHasAttrPremium = false;
					for (int j = 0; j < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++j)
					{
						if (pkItem->GetAttributeType(j) == rkTab.premium_attrs(i).type() && pkItem->GetAttributeValue(j) >= rkTab.premium_attrs(i).value())
						{
							bHasAttrPremium = true;
							break;
						}
					}

					if (!bHasAttrPremium)
					{
						bCheckAttrPremium = false;
						break;
					}
				}
			}
			if (bCheckAttrPremium)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s is finished."), pkItem->GetName(GetLanguageID()));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching process used %d switchers."), rkTab.switcher_used());
				ChatPacket(CHAT_TYPE_COMMAND, "switchbot_finish %d %d", rkTab.inv_cell(), pkItem->GetCell());
				rkTab.set_finished(true);
				set_Remove.insert(rkTab.item_id());
				continue;
			}
		}
		else if (bCheckAttr)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s is finished."), pkItem->GetName(GetLanguageID()));
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching process used %d switchers."), rkTab.switcher_used());
			ChatPacket(CHAT_TYPE_COMMAND, "switchbot_finish %d %d", rkTab.inv_cell(), pkItem->GetCell());
			rkTab.set_finished(true);
			set_Remove.insert(rkTab.item_id());
			continue;
		}

		pkItem->ChangeAttribute();
		
		if (dwUseSwitcherVnum != c_dwSwitcherVnum3)
		{
			LPITEM item2 = FindSpecifyItem(dwUseSwitcherVnum);
			if (item2)
			{				
				SetQuestItemPtr(item2);
				if (item2->GetVnum() == 71084)
				{
					if (quest::CQuestManager::instance().GetEventFlag("event_anniversary_running"))
					{
						int day = quest::CQuestManager::instance().GetEventFlag("event_anniversary_day");
						if (day == 3)
						{
							int currFraction = GetQuestFlag("anniversary_event.selected_fraction");
							if (currFraction == 1)
								quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_angel_cnt", 1, true);
							else if (currFraction == 2)
								quest::CQuestManager::instance().RequestSetEventFlag("event_anniversary_demon_cnt", 1, true);
						}
					}
				}
				quest::CQuestManager::instance().OnItemUsed(GetPlayerID());
				item2 = NULL;
			}

			ChatPacket(CHAT_TYPE_COMMAND, "switchbot_use_switcher %d", rkTab.inv_cell());
			RemoveSpecifyItem(dwUseSwitcherVnum);
			--iSwitcherCount;
			rkTab.set_switcher_used(rkTab.switcher_used() + 1);
		}

		for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
		{
			if (rkTab.attrs_size() > i && rkTab.attrs(i).type() != 0 && rkTab.attrs(i).value() != 0)
			{
				bool bHasAttr = false;
				for (int j = 0; j < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++j)
				{
					if (pkItem->GetAttributeType(j) == rkTab.attrs(i).type() && pkItem->GetAttributeValue(j) >= rkTab.attrs(i).value())
					{
						bHasAttr = true;
						break;
					}
				}

				if (!bHasAttr)
				{
				//	if (test_server)
				//		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Attribute %d val %d not found [attr0 %d %d]"), rkTab.kAttrs[i].bType, rkTab.kAttrs[i].iValue, pkItem->GetAttributeType(0), pkItem->GetAttributeValue(0));

					bCheckAttr = false;
					break;
				}
			}
		}

		if (!bCheckAttr && rkTab.use_premium())
		{
			bool bCheckAttrPremium = true;
			for (int i = 0; i < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++i)
			{
				if (rkTab.premium_attrs_size() > i && rkTab.premium_attrs(i).type() != 0 && rkTab.premium_attrs(i).value() != 0)
				{
					bool bHasAttrPremium = false;
					for (int j = 0; j < ITEM_MANAGER::MAX_NORM_ATTR_NUM; ++j)
					{
						if (pkItem->GetAttributeType(j) == rkTab.premium_attrs(i).type() && pkItem->GetAttributeValue(j) >= rkTab.premium_attrs(i).value())
						{
							bHasAttrPremium = true;
							break;
						}
					}

					if (!bHasAttrPremium)
					{
						bCheckAttrPremium = false;
						break;
					}
				}
			}
			if (bCheckAttrPremium)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s is finished."), pkItem->GetName(GetLanguageID()));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching process used %d switchers."), rkTab.switcher_used());
				ChatPacket(CHAT_TYPE_COMMAND, "switchbot_finish %d %d", rkTab.inv_cell(), pkItem->GetCell());
				rkTab.set_finished(true);
				set_Remove.insert(rkTab.item_id());
				continue;
			}
		}
		else if (bCheckAttr)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching of %s is finished."), pkItem->GetName(GetLanguageID()));
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The switching process used %d switchers."), rkTab.switcher_used());
			ChatPacket(CHAT_TYPE_COMMAND, "switchbot_finish %d %d", rkTab.inv_cell(), pkItem->GetCell());
			rkTab.set_finished(true);
			set_Remove.insert(rkTab.item_id());
			continue;
		}
	}

	for (itertype(set_Remove) it = set_Remove.begin(); it != set_Remove.end(); ++it)
		RemoveSwitchbotData(*it);

	if (m_map_Switchbots.size() == 0)
		dwTimeRefresh = 0;

	return dwTimeRefresh;
}
#endif

#ifdef __ATTRIBUTES_TO_CLIENT__
void CHARACTER::SendAttributesToClient()
{
	SendAttributesToClient(ITEM_WEAPON, -1, ATTRIBUTE_SET_WEAPON);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_BODY, ATTRIBUTE_SET_BODY);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_WRIST, ATTRIBUTE_SET_WRIST);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_FOOTS, ATTRIBUTE_SET_FOOTS);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_NECK, ATTRIBUTE_SET_NECK);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_HEAD, ATTRIBUTE_SET_HEAD);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_SHIELD, ATTRIBUTE_SET_SHIELD);
	SendAttributesToClient(ITEM_ARMOR, ARMOR_EAR, ATTRIBUTE_SET_EAR);

	SendAttributesToClient(ITEM_COSTUME, COSTUME_BODY, ATTRIBUTE_SET_BODY);
	SendAttributesToClient(ITEM_COSTUME, COSTUME_HAIR, ATTRIBUTE_SET_HEAD);
	SendAttributesToClient(ITEM_COSTUME, COSTUME_WEAPON, ATTRIBUTE_SET_WEAPON);

	SendAttributesToClient(ITEM_TOTEM, -1, ATTRIBUTE_SET_TOTEM);
}

void CHARACTER::SendAttributesToClient(BYTE bItemType, char cItemSubType, BYTE bAttrSetIndex)
{
	network::GCOutputPacket<network::GCAttributesToClientPacket> packet;
	packet->set_item_type(bItemType);
	packet->set_item_sub_type(cItemSubType);

	for (itertype(g_map_itemAttr) it = g_map_itemAttr.begin(); it != g_map_itemAttr.end(); ++it)
	{
		if (it->second.max_level_by_set_size() <= bAttrSetIndex || it->second.max_level_by_set(bAttrSetIndex) == 0)
			continue;

		auto attr = packet->add_attrs();
		attr->set_type(it->first);
		attr->set_value(it->second.values(it->second.max_level_by_set(bAttrSetIndex) - 1));
	}

	GetDesc()->Packet(packet);
}
#endif

#ifdef __VOTE4BUFF__
void CHARACTER::V4B_Initialize()
{
	if (!GetDesc())
		return;

	DBManager::instance().ReturnQuery(QID_V4B_LOAD, GetPlayerID(), NULL, "SELECT time_end, applytype, applyvalue FROM vote4buff WHERE hwid = '%s'", GetEscapedHWID());
}

void CHARACTER::V4B_SetLoaded()
{
	m_bV4B_Loaded = true;

	V4B_AddAffect();
}

void CHARACTER::V4B_GiveBuff(DWORD dwTimeEnd, BYTE bApplyType, int iApplyValue)
{
	if (get_global_time() >= dwTimeEnd || !bApplyType || !iApplyValue)
	{
		DBManager::instance().Query("DELETE FROM vote4buff WHERE hwid = '%s'", GetEscapedHWID());
		return;
	}

	sys_log(0, "CHARACTER::V4B_GiveBuff(%u, %u, %d) [%u]", dwTimeEnd, bApplyType, iApplyValue, get_global_time());

	m_dwV4B_TimeEnd = dwTimeEnd;
	m_bV4B_ApplyType = bApplyType;
	m_iV4B_ApplyValue = iApplyValue;

	V4B_AddAffect();
}

bool CHARACTER::V4B_IsBuff() const
{
	return FindAffect(AFFECT_VOTE4BUFF) != NULL;
}

void CHARACTER::V4B_AddAffect()
{
	if (m_dwV4B_TimeEnd && get_global_time() < m_dwV4B_TimeEnd)
	{
		if (m_bV4B_Loaded/* && IsLoadedAffect()*/)
			AddAffect(AFFECT_VOTE4BUFF, aApplyInfo[m_bV4B_ApplyType].bPointType, m_iV4B_ApplyValue, AFF_NONE, m_dwV4B_TimeEnd - get_global_time(), 0, true);
	}
}
#endif

#ifdef __GAYA_SYSTEM__
void CHARACTER::SetGaya(DWORD dwGaya)
{
	if (dwGaya == m_dwGaya)
		return;

	m_dwGaya = dwGaya;
	PointChange(POINT_GAYA, 0);
}
#endif

network::TAccountTable* CHARACTER::GetAccountTablePtr() const
{
	if (GetDesc())
		return &GetDesc()->GetAccountTable();
#ifdef __APP_SERVER_V2__
	if (GetAppDesc())
		return &GetAppDesc()->GetAccountTable();
#endif

	return NULL;
}

network::TAccountTable& CHARACTER::GetAccountTable() const
{
	return *GetAccountTablePtr();
}

const char* CHARACTER::GetEscapedHWID()
{
	static char szHWID[HWID_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(szHWID, sizeof(szHWID), GetAccountTable().hwid().c_str(), GetAccountTable().hwid().length());
	return szHWID;
}


void CHARACTER::SetIsShowTeamler(bool bIsShowTeamler)
{
	if (bIsShowTeamler == m_bIsShowTeamler)
		return;

	m_bIsShowTeamler = bIsShowTeamler;
	
	if (!IsPC())
		return;
	
	//if (strlen(GetName()) < 6) //?? why?
		//return;
	
	// update list
	if (m_bIsShowTeamler)
		CHARACTER_MANAGER::instance().AddOnlineTeamler(GetName(), GetLanguageID());
	else
		CHARACTER_MANAGER::instance().RemoveOnlineTeamler(GetName(), GetLanguageID());
	// update list p2p
	network::GGOutputPacket<network::GGTeamlerStatusPacket> p2p_packet;
	p2p_packet->set_name(GetName());
	p2p_packet->set_language(GetLanguageID());
	p2p_packet->set_is_online(m_bIsShowTeamler);
	P2P_MANAGER::instance().Send(p2p_packet);

	ShowTeamlerPacket();
	GlobalShowTeamlerPacket(m_bIsShowTeamler);
}

void CHARACTER::ShowTeamlerPacket()
{
	network::GCOutputPacket<network::GCTeamlerShowPacket> packet;
	packet->set_is_show(m_bIsShowTeamler);
	if (GetDesc())
		GetDesc()->Packet(packet);
}

void CHARACTER::GlobalShowTeamlerPacket(bool bIsOnline)
{
	network::GCOutputPacket<network::GCTeamlerStatusPacket> packet;
	packet->set_name(GetName());
	packet->set_is_online(bIsOnline);

	// send p2p
	{
		network::GGOutputPacket<network::GGPlayerPacket> p2p_pack;
		p2p_pack->set_language(GetLanguageID());
		p2p_pack->set_empire(0);
		p2p_pack->set_relay_header(packet.get_header());

		std::vector<uint8_t> buf;
		buf.resize(packet->ByteSize());
		packet->SerializeToArray(&buf[0], buf.size());
		p2p_pack->set_relay(&buf[0], buf.size());
		P2P_MANAGER::instance().Send(p2p_pack);

		if (GetLanguageID() == LANGUAGE_ENGLISH || GetLanguageID() == LANGUAGE_GERMAN)
		{
			if (GetLanguageID() == LANGUAGE_ENGLISH)
				p2p_pack->set_language(LANGUAGE_GERMAN);
			else
				p2p_pack->set_language(LANGUAGE_ENGLISH);

			P2P_MANAGER::instance().Send(p2p_pack);
		}
	}

	// send to player on this core and this map
	{
		const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
		for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
		{
			if (!it->second->GetDesc())
				continue;

			if (!it->second->GetDesc()->IsPhase(PHASE_GAME) && !it->second->GetDesc()->IsPhase(PHASE_DEAD))
				continue;

			bool canSend = false;

			switch (it->second->GetLanguageID()) // send english GM's to these languages too
			{
				case LANGUAGE_GERMAN:
				case LANGUAGE_ROMANIA:
				case LANGUAGE_HUNGARIAN:
				{
					if (GetLanguageID() == LANGUAGE_ENGLISH)
						canSend = true;
				}
				break;
				default:
				{
					if (it->second->GetLanguageID() != LANGUAGE_TURKISH && GetLanguageID() == LANGUAGE_ENGLISH)
					{
						canSend = true;
					}
				}
				break;
			}

			if (it->second->GetLanguageID() == GetLanguageID())
				canSend = true;

			if (canSend)
				it->second->GetDesc()->Packet(packet);
		}
	}
}

#ifdef __PYTHON_REPORT_PACKET__
void CHARACTER::DetectionHackLog(const char* c_pszType, const char* c_pszDetail)
{
	auto it = m_mapLastDetectionLog.find(c_pszType);

	if (it != m_mapLastDetectionLog.end())
	{
		if (get_dword_time() - it->second < 60 * 3 * 1000)
			return;

		it->second = get_dword_time();
	}
	else
		m_mapLastDetectionLog[c_pszType] = get_dword_time();

	LogManager::instance().HackDetectionLog(this, c_pszType, c_pszDetail);
}
#endif

bool CHARACTER::IsPurgeable() const
{
	if (!IsNPC())
		return false;

	if (GetRider() != NULL)
		return false;

	if (IsMount())
		return false;

	if (IsPet())
		return false;

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		return false;
#endif

	return true;
}

bool CHARACTER::IsEnemy() const
{
	if (IsStone())
		return true;

	if (IsPet())
		return false;

	if (IsMount())
		return false;
	
	if (IsMonster())
	{
#ifdef __FAKE_BUFF__
		if (FakeBuff_Check())
			return false;
#endif
		return true;
	}

	return false;
}

#ifdef __ATTRTREE__
void CHARACTER::SetAttrTreeLevel(BYTE row, BYTE col, BYTE level)
{
#ifdef ENABLE_RUNE_SYSTEM
	return;
#endif
	if (row >= ATTRTREE_ROW_NUM || col >= ATTRTREE_COL_NUM)
		return;

	if (GetAttrTreeLevel(row, col) == level)
		return;

	GiveAttrTreeBonus(row, col, false);
	m_aAttrTree[row][col] = level;
	GiveAttrTreeBonus(row, col, true);

	SendAttrTreeLevel(row, col);
}

BYTE CHARACTER::GetAttrTreeLevel(BYTE row, BYTE col) const
{
	if (row >= ATTRTREE_ROW_NUM || col >= ATTRTREE_COL_NUM)
		return 0;

	return m_aAttrTree[row][col];
}

void CHARACTER::GiveAttrTreeBonus(bool bAdd)
{
#ifdef ENABLE_RUNE_SYSTEM
	return;
#endif
	for (BYTE row = 0; row < ATTRTREE_ROW_NUM; ++row)
	{
		for (BYTE col = 0; col < ATTRTREE_COL_NUM; ++col)
			GiveAttrTreeBonus(row, col, bAdd);
	}
}

void CHARACTER::GiveAttrTreeBonus(BYTE row, BYTE col, bool bAdd)
{
#ifdef ENABLE_RUNE_SYSTEM
	return;
#endif
	BYTE level = GetAttrTreeLevel(row, col);
	if (level == 0)
		return;

	auto pProto = CAttrtreeManager::Instance().GetProto(row, col);
	if (!pProto)
	{
		sys_err("cannot get attrtree proto by cell(%u, %u)", row, col);
		return;
	}

	int iValue = ((long long)pProto->max_apply_value()) * level / ATTRTREE_LEVEL_NUM;
	ApplyPoint(pProto->apply_type(), bAdd ? iValue : -iValue);

	// if (test_server && !GetDesc()->IsPhase(PHASE_SELECT))
		// ChatPacket(CHAT_TYPE_INFO, "GiveAttrTreeBonus %u val %d (maxVal %d)", pProto->apply_type, iValue, pProto->max_apply_value);
}

void CHARACTER::SendAttrTree()
{
#ifdef ENABLE_RUNE_SYSTEM
	return;
#endif
	for (BYTE row = 0; row < ATTRTREE_ROW_NUM; ++row)
	{
		for (BYTE col = 0; col < ATTRTREE_COL_NUM; ++col)
		{
			if (GetAttrTreeLevel(row, col) > 0)
				SendAttrTreeLevel(row, col);
		}
	}
}

void CHARACTER::SendAttrTreeLevel(BYTE row, BYTE col)
{
#ifdef ENABLE_RUNE_SYSTEM
	return;
#endif
	network::GCOutputPacket<network::GCAttrtreeLevelPacket> pack;
	pack->set_id(CAttrtreeManager::Instance().CellToID(row, col));
	pack->set_level(GetAttrTreeLevel(row, col));
	if (GetDesc())
		GetDesc()->Packet(pack);
}

const network::TRefineTable* CHARACTER::GetAttrTreeRefine(BYTE id) const
{
	auto pProto = CAttrtreeManager::Instance().GetProto(id);
	if (!pProto)
		return NULL;

	BYTE row, col;
	CAttrtreeManager::instance().IDToCell(id, row, col);
	BYTE level = GetAttrTreeLevel(row, col);

	if (test_server)
		sys_log(0, "id %u row %d col %d level %d refines %d %d %d %d %d",
			id, row, col, level, pProto->refine_level(0), pProto->refine_level(1), pProto->refine_level(2), pProto->refine_level(3), pProto->refine_level(4));

	if (level == 0)
	{
		// need col before at least lv. 1
		if (col > 0)
		{
			if (GetAttrTreeLevel(row, col - 1) == 0)
				return NULL;
		}
	}
	// already max level
	else if (level >= ATTRTREE_LEVEL_NUM)
		return NULL;

	return CRefineManager::instance().GetRefineRecipe(pProto->refine_level(level));
}
#endif

void CHARACTER::GetDataByAbilityUp(DWORD dwItemVnum, int& iEffectPacket, DWORD& dwAffType, WORD& wPointType, DWORD& dwAffFlag)
{
	if (dwItemVnum > 50800 && dwItemVnum <= 50820)
		return;

	auto pTable = ITEM_MANAGER::instance().GetTable(dwItemVnum);
	if (!pTable)
		return;

	switch (pTable->values(0))
	{
	case APPLY_MOV_SPEED:
		dwAffType = AFFECT_MOV_SPEED;
		wPointType = POINT_MOV_SPEED;
		dwAffFlag = AFF_MOV_SPEED_POTION;
		break;

	case APPLY_ATT_SPEED:
		iEffectPacket = SE_SPEEDUP_GREEN;
		dwAffType = AFFECT_ATT_SPEED;
		wPointType = POINT_ATT_SPEED;
		dwAffFlag = AFF_ATT_SPEED_POTION;
		break;

	case APPLY_STR:
		dwAffType = AFFECT_STR;
		wPointType = POINT_ST;
		break;

	case APPLY_DEX:
		dwAffType = AFFECT_DEX;
		wPointType = POINT_DX;
		break;

	case APPLY_CON:
		dwAffType = AFFECT_CON;
		wPointType = POINT_HT;
		break;

	case APPLY_INT:
		dwAffType = AFFECT_INT;
		wPointType = POINT_IQ;
		break;

	case APPLY_CAST_SPEED:
		dwAffType = AFFECT_CAST_SPEED;
		wPointType = POINT_CASTING_SPEED;
		break;

	case APPLY_ATT_GRADE_BONUS:
		dwAffType = AFFECT_ATT_GRADE;
		wPointType = POINT_ATT_GRADE_BONUS;
		break;

	case APPLY_DEF_GRADE_BONUS:
		dwAffType = AFFECT_DEF_GRADE;
		wPointType = POINT_DEF_GRADE_BONUS;
		break;

	case APPLY_ATTBONUS_ANIMAL:
		dwAffType = AFFECT_ATTBONUS_ANIMAL;
		wPointType = POINT_ATTBONUS_ANIMAL;
		break;

	case APPLY_ATTBONUS_ORC:
		dwAffType = AFFECT_ATTBONUS_ORC;
		wPointType = POINT_ATTBONUS_ORC;
		break;

	case APPLY_ATTBONUS_MILGYO:
		dwAffType = AFFECT_ATTBONUS_MILGYO;
		wPointType = POINT_ATTBONUS_MILGYO;
		break;

	case APPLY_ATTBONUS_UNDEAD:
		dwAffType = AFFECT_ATTBONUS_UNDEAD;
		wPointType = POINT_ATTBONUS_UNDEAD;
		break;

	case APPLY_ATTBONUS_DEVIL:
		dwAffType = AFFECT_ATTBONUS_DEVIL;
		wPointType = POINT_ATTBONUS_DEVIL;
		break;

	case APPLY_STUN_PCT:
		dwAffType = AFFECT_STUN_PCT;
		wPointType = POINT_STUN_PCT;
		break;

	case APPLY_MAX_HP:
		dwAffType = AFFECT_MAX_HP;
		wPointType = POINT_MAX_HP;
		break;

	case APPLY_MAX_SP:
		dwAffType = AFFECT_MAX_SP;
		wPointType = POINT_MAX_SP;
		break;

	case APPLY_ATTBONUS_WARRIOR:
		dwAffType = AFFECT_ATTBONUS_WARRIOR;
		wPointType = POINT_ATTBONUS_WARRIOR;
		break;

	case APPLY_ATTBONUS_ASSASSIN:
		dwAffType = AFFECT_ATTBONUS_ASSASSIN;
		wPointType = POINT_ATTBONUS_ASSASSIN;
		break;

	case APPLY_ATTBONUS_SURA:
		dwAffType = AFFECT_ATTBONUS_SURA;
		wPointType = POINT_ATTBONUS_SURA;
		break;

	case APPLY_ATTBONUS_SHAMAN:
		dwAffType = AFFECT_ATTBONUS_SHAMAN;
		wPointType = POINT_ATTBONUS_SHAMAN;
		break;

	case APPLY_POISON_PCT:
		dwAffType = AFFECT_POISON_PCT;
		wPointType = POINT_POISON_PCT;
		break;

	case APPLY_PENETRATE_PCT:
		dwAffType = AFFECT_PENETRATE_PCT;
		wPointType = POINT_PENETRATE_PCT;
		break;

	case APPLY_CRITICAL_PCT:
		dwAffType = AFFECT_CRITICAL_PCT;
		wPointType = POINT_CRITICAL_PCT;
		break;

	case APPLY_ATTBONUS_MONSTER:
		dwAffType = AFFECT_ATTBONUS_MONSTER;
		wPointType = POINT_ATTBONUS_MONSTER;
		break;

	case APPLY_MAGIC_ATT_GRADE:
		dwAffType = APPLY_MAGIC_ATT_GRADE;
		wPointType = POINT_MAGIC_ATT_GRADE;
		break;

	}
}

void CHARACTER::RemoveItemBuffs(network::TItemTable* pItemTable)
{
	switch (pItemTable->type())
	{
		case ITEM_USE:
		{
			switch (pItemTable->sub_type())
			{
				case USE_SPECIAL:
				{
					if (pItemTable->vnum() > 72723 && pItemTable->vnum() <= 72726)
					{
						StopAutoRecovery(AFFECT_AUTO_HP_RECOVERY);
						tchat("STOP AUTO HP RECOVERY");
					}
					else if (pItemTable->vnum() > 72727 && pItemTable->vnum() <= 72730)
					{
						StopAutoRecovery(AFFECT_AUTO_SP_RECOVERY);
						tchat("STOP AUTO SP RECOVERY");
					}
				}
				break;
				
				case USE_AFFECT:
				{
					if (pItemTable->vnum() > 50800 && pItemTable->vnum() <= 50820)
					{
						if (test_server)
							sys_log(0, "RemoveItemBuffs[%u] AFFType %u ApplyType %d", pItemTable->vnum(), AFFECT_EXP_BONUS_EURO_FREE, pItemTable->values(1));
						tchat("RemoveItemBuffs[%u] AFFType %u ApplyType %d", pItemTable->vnum(), AFFECT_EXP_BONUS_EURO_FREE, pItemTable->values(1));
						RemoveAffect(FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[pItemTable->values(1)].bPointType));
					}
					else
					{
						if (test_server)
							sys_log(0, "RemoveItemBuffs[%u] AFFType %u ApplyType %d", pItemTable->vnum(), pItemTable->values(0), pItemTable->values(1));
						tchat("RemoveItemBuffs[%u] AFFType %u ApplyType %d", pItemTable->vnum(), pItemTable->values(0), pItemTable->values(1));
						RemoveAffect(FindAffect(pItemTable->values(0), aApplyInfo[pItemTable->values(1)].bPointType));
					}
				}
				break;

				case USE_ABILITY_UP:
				{
					int iEffectPacket = -1;
					DWORD dwAffType = 0;
					WORD wPointType = 0;
					DWORD dwAffFlag = 0;

					GetDataByAbilityUp(pItemTable->vnum(), iEffectPacket, dwAffType, wPointType, dwAffFlag);

					if (!dwAffType)
						return;

					CAffect* pAff = FindAffect(dwAffType, wPointType);
					if (pAff && (long)pAff->lApplyValue == pItemTable->values(2))
						RemoveAffect(pAff);
				}
				break;
			}
		}
		break;

/* 		case ITEM_BLEND:
		{
			BYTE bApplyType = Blend_Item_GetApplyType(pItemTable->dwVnum);
			if (bApplyType != 0)
				RemoveAffect(FindAffect(AFFECT_BLEND, aApplyInfo[bApplyType].bPointType));
		}
		break; */

/* 		case ITEM_QUEST:
		{
			switch (pItemTable->dwVnum)
			{
				case 31032:
				{
					RemoveAffect(AFFECT_QUEST_5OUTRIDER_1);
					RemoveAffect(AFFECT_QUEST_5OUTRIDER_2);
				}
				break;
			}
		}
		break; */
	}
}

void CHARACTER::CheckForDisabledItemBuffs(const std::set<DWORD>* pDisabledList)
{
	if (!pDisabledList)
	{
		if (!(pDisabledList = ITEM_MANAGER::instance().GetDisabledItemList(GetMapIndex())))
		{
			tchat("if (!(pDisabledList = ITEM_MANAGER::instance().GetDisabledItemList(GetMapIndex())))");
			return;
		}
			
	}

	for (auto it = pDisabledList->begin(); it != pDisabledList->end(); ++it)
	{
		auto pTable = ITEM_MANAGER::instance().GetTable(*it);
		if (!pTable)
		{
			sys_err("cannot get item table by vnum %u", *it);
			tchat("cannot get item table by vnum %u", *it);
			continue;
		}

		RemoveItemBuffs(pTable);
	}
}

void CHARACTER::CheckForDisabledItems(const std::set<DWORD>* pDisabledList)
{
	if (pDisabledList)
		CheckForDisabledItemBuffs(pDisabledList);

	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if (LPITEM pItem = GetInventoryItem(i))
		{
			if (pDisabledList && pDisabledList->find(pItem->GetVnum()) != pDisabledList->end())
				pItem->DisableItem();
			else
				pItem->EnableItem();
		}
	}
}

void CHARACTER::CheckForDisabledItems(bool bOnlyCheckIfListExists)
{
	const std::set<DWORD>* pDisabledList = ITEM_MANAGER::instance().GetDisabledItemList(GetMapIndex());
	if (pDisabledList || !bOnlyCheckIfListExists)
		CheckForDisabledItems(pDisabledList);
}

void CHARACTER::StopAutoRecovery(const EAffectTypes type)
{
	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
	{
		tchat("StopAutoRecovery 1");
		return;
	}

	CAffect* pAffect = FindAffect(type);
	if (pAffect == NULL)
	{
		tchat("StopAutoRecovery 2");
		return;
	}

	LPITEM pItem = FindItemByID(pAffect->dwFlag);
	if (NULL == pItem)
	{
		tchat("StopAutoRecovery 3");
		return;
	}

	RemoveAffect(pAffect);
	pItem->SetSocket(0, 0);
	tchat("StopAutoRecovery __4__");
}

#ifdef ENABLE_RUNE_SYSTEM
bool CHARACTER::ResetRunes()
{
	if (!m_bRuneLoaded)
		return false;

	GiveRunePageBuff(false);
	m_setRuneOwned.clear();

	if (m_bRunePermaBonusSet)
	{
		ApplyPoint(APPLY_ATTBONUS_HUMAN, -15);
		ApplyPoint(APPLY_RESIST_ATTBONUS_HUMAN, -10);
		m_bRunePermaBonusSet = false;
	}

	m_runePage.Clear();
	m_abPlayerDataChanged[PC_TAB_CHANGED_RUNE] = true;

	SendRunePagePacket();

	GetDesc()->Packet(network::TGCHeader::RUNE_RESET_OWNED);
	return true;
}

void CHARACTER::SetRuneOwned(DWORD dwVnum)
{
	m_setRuneOwned.insert(dwVnum);
	GiveRunePermaBonus();
	SendRunePacket(dwVnum);

	m_abPlayerDataChanged[PC_TAB_CHANGED_RUNE] = true;
}

bool CHARACTER::SetRunePage(const TRunePageData* pData)
{
	if (test_server)
		sys_log(0, "CHARACTER::SetRunePage %s", GetName());

#ifdef CHECK_TIME_AFTER_PVP
	if (IsPVPFighting(5))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You need to wait %d seconds to do this after the last time you were attacked."), 5);
		ChatPacket(CHAT_TYPE_COMMAND, "SetRunePage %d %d", 9999, 9999);
		return false;
	}
#endif

	if (!IsValidRunePage(pData))
	{
		//sys_err("no valid rune page for player %u %s", GetPlayerID(), GetName());
		tchat("invalid rune page");
		return false;
	}

	bool is_equal = m_runePage.main_group() == pData->main_group() && m_runePage.sub_group() == pData->sub_group() &&
		m_runePage.main_vnum_size() == pData->main_vnum_size() && m_runePage.sub_vnum_size() == pData->sub_vnum_size();
	for (int i = 0; i < m_runePage.main_vnum_size() && is_equal; ++i)
	{
		if (m_runePage.main_vnum(i) != pData->main_vnum(i))
			is_equal = false;
	}
	for (int i = 0; i < m_runePage.sub_vnum_size() && is_equal; ++i)
	{
		if (m_runePage.sub_vnum(i) != pData->sub_vnum(i))
			is_equal = false;
	}

	if (is_equal)
	{
		//sys_err("rune page equals");
		tchat("rune page not changed, because it's equal(already set)");
		return false;
	}

	CancelRuneEvent();
	GiveRunePageBuff(false);

	m_runePage = *pData;
	m_abPlayerDataChanged[PC_TAB_CHANGED_RUNE] = true;
	GiveRunePageBuff(true);

	int oldMSpeed = m_runeData.storedMovementSpeed;
	int oldASpeed = m_runeData.storedAttackSpeed;
	memset(&m_runeData, 0, sizeof(m_runeData));
	m_runeData.storedMovementSpeed = oldMSpeed;
	m_runeData.storedAttackSpeed = oldASpeed;

	StartRuneEvent(true);
	if (GetPoint(POINT_RUNE_HARVEST))
		m_runeData.soulsHarvested = MIN(GetPoint(POINT_RUNE_HARVEST), GetQuestFlag("rune_manager.harvested_souls"));

	SendRunePagePacket();
	return true;
}

void CHARACTER::SetRuneLoaded()
{
	if (!IsValidRunePage(&m_runePage))
	{
		m_runePage.set_main_group(-1);
		m_runePage.set_sub_group(-1);
	}
	SendRunePagePacket();

	ResetDataChanged(PC_TAB_CHANGED_RUNE);

	m_bRuneLoaded = true;

	GiveRunePageBuff();
	GiveRunePermaBonus();
}

bool CHARACTER::IsValidRunePage(const TRunePageData* pData)
{
	if (test_server)
		sys_log(0, "1 : %d", pData->main_group());

	// invalid main group
	if (pData->main_group() < -1 || pData->main_group() >= RUNE_GROUP_MAX_NUM)
		return false;

	if (pData->main_vnum_size() != RUNE_MAIN_COUNT)
		return false;

	// check main runes
	for (int i = 0; i < RUNE_MAIN_COUNT; ++i)
	{
		if (test_server)
			sys_log(0, "2 : [%d] %u", i, pData->main_vnum(i));

		if (!pData->main_vnum(i))
			continue;

		// no group selected
		if (pData->main_group() == -1)
			return false;

		// check group and sub group
		auto pProto = CRuneManager::instance().GetProto(pData->main_vnum(i));
		if (test_server)
			sys_log(0, "3 [%d != %d?] [%d != %d?]", pProto ? pProto->group() : -1, pData->main_group(), pProto ? pProto->sub_group() : -1, i);
		if (!pProto || pProto->group() != pData->main_group() || pProto->sub_group() != i)
			return false;

		if (test_server)
			sys_log(0, "4");

		// check owned
		if (!IsRuneOwned(pData->main_vnum(i)))
			return false;

		if (test_server)
			sys_log(0, "5");
	}

	if (test_server)
		sys_log(0, "6 : %d", pData->sub_group());

	// invalid sub group
	if (pData->sub_group() < -1 || pData->sub_group() >= RUNE_GROUP_MAX_NUM)
		return false;

	if (test_server)
		sys_log(0, "7");

	if ((pData->sub_group() != -1 && pData->main_group() == -1) || (pData->sub_group() == pData->main_group() && pData->sub_group() != -1))
		return false;

	if (pData->sub_vnum_size() != RUNE_SUB_COUNT)
		return false;

	// check sub runes
	std::set<BYTE> set_SelectedSubGroups;
	for (int i = 0; i < RUNE_SUB_COUNT; ++i)
	{
		if (test_server)
			sys_log(0, "8 : [%d] %u", i, pData->sub_vnum(i));

		if (!pData->sub_vnum(i))
			continue;

		// no sub group selected
		if (pData->sub_group() == -1)
			return false;

		if (test_server)
			sys_log(0, "9");

		// check group and sub group
		auto pProto = CRuneManager::instance().GetProto(pData->sub_vnum(i));
		if (!pProto || pProto->group() != pData->sub_group() || pProto->sub_group() == RUNE_SUBGROUP_PRIMARY)
			return false;

		if (test_server)
			sys_log(0, "10");

		// allow only one sub group
		if (set_SelectedSubGroups.find(pProto->sub_group()) != set_SelectedSubGroups.end())
			return false;
		set_SelectedSubGroups.insert(pProto->sub_group());

		if (test_server)
			sys_log(0, "11");

		// check owned
		if (!IsRuneOwned(pData->sub_vnum(i)))
			return false;

		if (test_server)
			sys_log(0, "12");
	}

	return true;
}

#ifdef ENABLE_RUNE_AFFECT_ICONS
void CHARACTER::RuneAffectHelper(DWORD type, bool bAdd)
{
	if (bAdd)
	{
		AddAffect(type, POINT_NONE, 0, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true);
	}
	
	if (!bAdd)
	{
		auto pkAff = FindAffect(type);

		if (pkAff)
		{
			RemoveAffect(pkAff, false);
		}
	}
}

#endif

void CHARACTER::GiveRuneBuff(DWORD dwVnum, bool bAdd)
{
	if (!dwVnum)
		return;

#ifdef ENABLE_LEVEL2_RUNES
	if (IsRuneOwned(dwVnum + 100))
		dwVnum += 100;
#endif

	auto pTab = CRuneManager::instance().GetProto(dwVnum);
	if (!pTab)
	{
		sys_err("cannot give rune buff[%u] no proto", dwVnum);
		return;
	}

	CPoly kPoly;
	if (!kPoly.Analyze(pTab->apply_eval().c_str()))
	{
		sys_err("cannot give rune buff[%u] invalid apply eval[%s]", dwVnum, pTab->apply_eval().c_str());
		return;
	}

#ifdef ENABLE_RUNE_AFFECT_ICONS
	switch (dwVnum)
	{
		case 1:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_1, bAdd);
			break;
		}
		case 2:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_2, bAdd);
			break;
		}
		case 3:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_3, bAdd);
			break;
		}
		
		case 13:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_13, bAdd);
			break;
		}
		case 14:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_14, bAdd);
			break;
		}
		case 15:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_15, bAdd);
			break;
		}
		
		case 25:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_25, bAdd);
			break;
		}
		case 26:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_26, bAdd);
			break;
		}
		case 27:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_27, bAdd);
			break;
		}
		
		case 37:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_37, bAdd);
			break;
		}
		case 38:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_38, bAdd);
			break;
		}
		case 39:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_39, bAdd);
			break;
		}
		
		case 49:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_49, bAdd);
			break;
		}
		case 50:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_50, bAdd);
			break;
		}
		case 51:
		{
			RuneAffectHelper(AFFECT_RUNE_PRIMARY_SUBGROUP_KEY_51, bAdd);
			break;
		}
		break;
	}
#endif
	kPoly.SetVar("lv", GetLevel());
	//float fVal = kPoly.Evaluate(); //__UNIMPLEMENT__
	float fVal = kPoly.Eval();
	tchat("%s apply %d point %d with val %f", bAdd ? "give" : "remove", pTab->apply_type(), aApplyInfo[pTab->apply_type()], fVal);
	ApplyPoint(pTab->apply_type(), bAdd ? int(fVal) : -int(fVal));
}

void CHARACTER::GiveRunePageBuff(bool bAdd)
{
	if (!IsRuneLoaded())
		return;

	if (m_runePage.main_group() != -1)
	{
		for (int i = 0; i < RUNE_MAIN_COUNT && i < m_runePage.main_vnum_size(); ++i)
			GiveRuneBuff(m_runePage.main_vnum(i), bAdd);

		if (m_runePage.sub_group() != -1)
		{
			for (int i = 0; i < RUNE_SUB_COUNT && i < m_runePage.sub_vnum_size(); ++i)
				GiveRuneBuff(m_runePage.sub_vnum(i), bAdd);
		}
	}
}

void CHARACTER::SendRunePacket(DWORD dwVnum)
{
	network::GCOutputPacket<network::GCRunePacket> pack;
	pack->set_vnum(dwVnum);

	if (GetDesc())
		GetDesc()->Packet(pack);
}

void CHARACTER::SendRunePagePacket()
{
	network::GCOutputPacket<network::GCRunePagePacket> pack;
	*pack->mutable_data() = m_runePage;

	if (GetDesc())
		GetDesc()->Packet(pack);
}

EVENTFUNC(rune_basic_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("rune_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) // <Factor>
		return 0;

	ch->CancelRuneEvent(true);
	return 0;
}

EVENTFUNC(rune_movspeed_check_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("rune_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) // <Factor>
		return 0;

	RuneCharData & chData = ch->GetRuneData();
	ch->CancelRuneEvent(true);
	chData.bonusMowSpeed += ch->GetPoint(POINT_RUNE_OUT_OF_COMBAT_SPEED);
	ch->ApplyPoint(APPLY_MOV_SPEED, chData.bonusMowSpeed);
	ch->UpdatePacket();
	return 0;
}

void CHARACTER::CancelRuneEvent(bool removeOnly)
{
	if (removeOnly)
		m_runeEvent = NULL;

	if (m_runeEvent)
		event_cancel(&m_runeEvent);

	int oldSpeed = m_runeData.bonusMowSpeed;
	if (oldSpeed)
	{
		m_runeData.bonusMowSpeed = 0;
		ApplyPoint(APPLY_MOV_SPEED, -oldSpeed);
		UpdatePacket();
	}

	if (GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED))
		ApplyPoint(APPLY_CAST_SPEED, -GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED));
}

void CHARACTER::ResetSkillTime(WORD wVnum)
{
	m_dwLastSkillTime = 0;
	m_SkillUseInfo[wVnum].dwNextSkillUsableTime = 0;
	ChatPacket(CHAT_TYPE_COMMAND, "refresh_skill %u", wVnum);
}

void CHARACTER::GiveRunePermaBonus(bool force)
{
	if (!force && m_bRunePermaBonusSet)
		return;

	if (!m_bRuneLoaded)
		return;

	if (!m_bRunePermaBonusSet)
		for (size_t i = 0; i < RUNE_GROUP_MAX_NUM; ++i)
		{
			BYTE ownedInTree = 0;
			for (DWORD vnum = i * RUNE_SUBGROUP_MAX_NUM * 3 + 1; vnum < (i + 1) * RUNE_SUBGROUP_MAX_NUM * 3 + 1; ++vnum)
				if (m_setRuneOwned.find(vnum) != m_setRuneOwned.end())
					++ownedInTree;

			if (ownedInTree >= RUNE_SUBGROUP_MAX_NUM * 3)
			{
				m_bRunePermaBonusSet = true;
				break;
			}
		}

	if (m_bRunePermaBonusSet)
	{
		ApplyPoint(APPLY_ATTBONUS_HUMAN, 15);
		ApplyPoint(APPLY_RESIST_ATTBONUS_HUMAN, 10);
	}
}

void CHARACTER::StartRuneEvent(bool isReset, long restartTime)
{
	if (GetPoint(POINT_RUNE_OUT_OF_COMBAT_SPEED)
#ifdef COMBAT_ZONE
		&& !CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex())
#endif
		)
	{
		if (!m_runeEvent)
		{
			int oldSpeed = m_runeData.bonusMowSpeed;
			if (oldSpeed)
			{
				m_runeData.bonusMowSpeed = 0;
				ApplyPoint(APPLY_MOV_SPEED, -oldSpeed);
				if (!restartTime)
					UpdatePacket();
			}

			char_event_info* info = AllocEventInfo<char_event_info>();
			info->ch = this;
			m_runeEvent = event_create(rune_movspeed_check_event, info, PASSES_PER_SEC(1));
		}
		else
			event_reset_time(m_runeEvent, PASSES_PER_SEC(1));

		return;
	}

	if (m_runeEvent)
		return;

	if (GetPoint(POINT_RUNE_FIRST_NORMAL_HIT_BONUS) && !isReset)
	{
		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;

		m_runeEvent = event_create(rune_basic_event, info, restartTime ? restartTime : PASSES_PER_SEC(1) + int(passes_per_sec / 2) + 1);
		m_runeData.bonusMowSpeed = 100;
		ApplyPoint(APPLY_MOV_SPEED, m_runeData.bonusMowSpeed);
		if (!restartTime)
			UpdatePacket();
	}
	else if (GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED) && !isReset)
	{
		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;

		m_runeEvent = event_create(rune_basic_event, info, restartTime ? restartTime : PASSES_PER_SEC(10));
		ApplyPoint(APPLY_CAST_SPEED, GetPoint(POINT_RUNE_COMBAT_CASTING_SPEED));
	}
	else if (GetPoint(POINT_RUNE_MOVSPEED_AFTER_3) && !isReset)
	{
		char_event_info* info = AllocEventInfo<char_event_info>();
		info->ch = this;

		m_runeEvent = event_create(rune_basic_event, info, restartTime ? restartTime : PASSES_PER_SEC(1) + int(passes_per_sec / 2) + 1);
		m_runeData.bonusMowSpeed = GetPoint(POINT_RUNE_MOVSPEED_AFTER_3);
		ApplyPoint(APPLY_MOV_SPEED, m_runeData.bonusMowSpeed);
		if (!restartTime)
			UpdatePacket();
	}
}
#endif


EVENTFUNC(aura_update_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>(event->info);
	if (info == NULL)
	{
		sys_err("aura_update_event <Factor> Null pointer");
		return 0;
	}

	LPCHARACTER	ch = info->ch;
	if (ch == NULL) { // <Factor>
		return 0;
	}

	ch->Event_UpdateAura();
	return PASSES_PER_SEC(1);
}

struct FCollectAuraNearbyPlayer
{
	FCollectAuraNearbyPlayer(LPCHARACTER pkMainChr, std::vector<LPCHARACTER>& rkPlayerVector) : rkPlayerVector(rkPlayerVector)
	{
		base_x = pkMainChr->GetX();
		base_y = pkMainChr->GetY();
	}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER)ent;
		if (!pkChr->IsPC())
			return;

		int iDistance = DISTANCE_APPROX(pkChr->GetX() - base_x, pkChr->GetY() - base_y);
		if (iDistance <= AURA_MAX_DISTANCE)
			rkPlayerVector.push_back(pkChr);
	}

	DWORD base_x, base_y;
	std::vector<LPCHARACTER>&	rkPlayerVector;
};

DWORD CHARACTER::GetAuraApplyByPoint(DWORD dwPoint)
{
	switch (dwPoint)
	{
	case POINT_AURA_HEAL_EFFECT_BONUS:
		return APPLY_HEAL_EFFECT_BONUS;
		break;

	case POINT_AURA_EQUIP_SKILL_BONUS:
		return APPLY_EQUIP_SKILL_BONUS;
		break;
	}

	sys_err("invalid aura point %u", dwPoint);
	return 0;
}

void CHARACTER::ClearAuraBuffs()
{
	// remove all received buffs
	while (!m_map_AuraBuffBonus.empty())
	{
		TAuraBuffMap::iterator it_map = m_map_AuraBuffBonus.begin();

		for (TAuraAffectSet::iterator it_set = it_map->second.begin(); it_set != it_map->second.end(); ++it_set)
			RemoveAffect(*it_set);

		it_map->first->m_set_AuraBuffedPlayer.erase(this);
		m_map_AuraBuffBonus.erase(it_map);
	}

	// remove all given buffs
	ClearGivenAuraBuffs();
}

void CHARACTER::ClearGivenAuraBuffs()
{
	while (!m_set_AuraBuffedPlayer.empty())
	{
		LPCHARACTER pkChr = *m_set_AuraBuffedPlayer.begin();

		TAuraAffectSet& rkAffSet = pkChr->m_map_AuraBuffBonus[this];
		for (TAuraAffectSet::iterator it_set = rkAffSet.begin(); it_set != rkAffSet.end(); ++it_set)
			pkChr->RemoveAffect(*it_set);

		pkChr->m_map_AuraBuffBonus.erase(this);
		m_set_AuraBuffedPlayer.erase(m_set_AuraBuffedPlayer.begin());
	}
}

void CHARACTER::StartUpdateAuraEvent(bool bIsRestart)
{
	if (IsDead())
		return;

	if (m_pkAuraUpdateEvent)
	{
		if (!bIsRestart)
			sys_err("aura event already running");
		event_cancel(&m_pkAuraUpdateEvent);
	}

	char_event_info* info = AllocEventInfo<char_event_info>();
	info->ch = this;
	m_pkAuraUpdateEvent = event_create(aura_update_event, info, PASSES_PER_SEC(1));

	Event_UpdateAura();
}

void CHARACTER::Event_UpdateAura()
{
	// check if has aura buffs and the given player if the player is still nearby
	for (TAuraBuffMap::iterator it = m_map_AuraBuffBonus.begin(); it != m_map_AuraBuffBonus.end(); )
	{
		LPCHARACTER pkAuraOwner = it->first;

		int iDistance = DISTANCE_APPROX(GetX() - pkAuraOwner->GetX(), GetY() - pkAuraOwner->GetY());
		if (iDistance > AURA_MAX_DISTANCE)
		{
			TAuraBuffMap::iterator it_cur = ++it;

			for (TAuraAffectSet::iterator set_it = it_cur->second.begin(); set_it != it_cur->second.end(); ++set_it)
				RemoveAffect(*set_it);

			pkAuraOwner->m_set_AuraBuffedPlayer.erase(this);
			m_map_AuraBuffBonus.erase(it_cur);

			continue;
		}

		++it;
	}

	// check if the player has buffs by himself and can give to others
	std::vector<DWORD> vecAuras;
	for (int i = 0; i < AURA_BUFF_MAX_NUM; ++i)
	{
		if (GetPoint(adwAuraBuffList[i]))
			vecAuras.push_back(adwAuraBuffList[i]);
	}

	if (vecAuras.size() > 0 && GetSectree())
	{
		// collect nearby player
		std::vector<LPCHARACTER> vecNearbyPlayer;
		FCollectAuraNearbyPlayer f(this, vecNearbyPlayer);
		GetSectree()->ForEachAround(f);

		//if (test_server)
		//	ChatPacket(CHAT_TYPE_INFO, "vecNearbyPlayer.size() = %u", vecNearbyPlayer.size());
		if (!vecNearbyPlayer.size())
			return;

		// write owner name to send it later
		char szOwnerName[CHARACTER_NAME_MAX_LEN + 1];
		strlcpy(szOwnerName, GetName(), sizeof(szOwnerName));
		std::replace(szOwnerName, szOwnerName + sizeof(szOwnerName), ' ', '*');

		// give buffs
		for (int i = 0; i < vecAuras.size(); ++i)
		{
			DWORD dwAffectType = AFFECT_AURA_START + vecAuras[i];
			DWORD dwApplyType = GetAuraApplyByPoint(vecAuras[i]);
			int fApplyValue = GetPoint(vecAuras[i]);

			//if (test_server)
			//	ChatPacket(CHAT_TYPE_INFO, "affType %u applyPoint %u val %f", dwAffectType, dwApplyType, fApplyValue);

			if (!dwApplyType)
			{
				sys_err("no apply type for aura %u affect %u", vecAuras[i], dwAffectType);
				continue;
			}

			for (int j = 0; j < vecNearbyPlayer.size(); ++j)
			{
				LPCHARACTER pkChr = vecNearbyPlayer[j];

				if (CAffect* pkAff = pkChr->FindAffect(dwAffectType))
				{
					if (pkAff->lApplyValue >= fApplyValue)
						continue;
				}

				// send aura owner name
				pkChr->ChatPacket(CHAT_TYPE_COMMAND, "set_buff_aura_owner %u %s", dwAffectType, szOwnerName);

				// add aura
				pkChr->m_map_AuraBuffBonus[this].insert(dwAffectType);
				m_set_AuraBuffedPlayer.insert(pkChr);
				pkChr->AddAffect(dwAffectType, aApplyInfo[dwApplyType].bPointType, fApplyValue, AFF_NONE, INFINITE_AFFECT_DURATION, 0, true);
			}
		}
	}
}

void CHARACTER::UpdateAuraByPoint(DWORD dwPoint)
{
	DWORD dwAffectType = AFFECT_AURA_START + dwPoint;
	DWORD dwVal = GetPoint(dwPoint);

	if (!dwVal)
	{
		// remove from all this affect		
		for (TAuraBuffedPlayerSet::iterator it = m_set_AuraBuffedPlayer.begin(); it != m_set_AuraBuffedPlayer.end(); )
		{
			LPCHARACTER pkChr = *it;

			TAuraAffectSet& rkAffSet = pkChr->m_map_AuraBuffBonus[this];
			TAuraAffectSet::iterator set_it = rkAffSet.find(dwAffectType);
			if (set_it == rkAffSet.end())
				continue;

			pkChr->RemoveAffect(dwAffectType);
			rkAffSet.erase(set_it);

			if (rkAffSet.size() == 0)
			{
				pkChr->m_map_AuraBuffBonus.erase(this);
				m_set_AuraBuffedPlayer.erase(it);

				it = m_set_AuraBuffedPlayer.begin(); // reset iterator
			}
		}
	}
	else
	{
		StartUpdateAuraEvent(true); // restart aura event
	}
}

#ifdef AHMET_FISH_EVENT_SYSTEM
void CHARACTER::SendFishInfoAsCommand(BYTE bSubHeader, DWORD dwFirstArg, DWORD dwSecondArg)
{
	tchat("gc_fish_event_info %d %d %d", bSubHeader, dwFirstArg, dwSecondArg);
	ChatPacket(CHAT_TYPE_COMMAND, "gc_fish_event_info %d %d %d", bSubHeader, dwFirstArg, dwSecondArg);
}

void CHARACTER::FishEventGeneralInfo()
{
	if(!GetDesc())
		return;

	if (!quest::CQuestManager::instance().GetEventFlag("enable_fish_event"))
		return;

	SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_GC_ENABLE, quest::CQuestManager::instance().GetEventFlag("enable_fish_event"), GetFishEventUseCount());

	if(GetFishEventUseCount() == 0)
	{
		for(int i = 0; i < FISH_EVENT_SLOTS_NUM; i++)
		{
			m_fishSlots[ i ].set_is_main(false);
			m_fishSlots[ i ].set_shape(0);
		}
	}

	for(int i = 0; i < FISH_EVENT_SLOTS_NUM; i++)
	{
		if(m_fishSlots[ i ].is_main())
			SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_SHAPE_ADD, i, m_fishSlots[ i ].shape());
	}
}


#ifdef ENABLE_ZODIAC_TEMPLE
void CHARACTER::SetZodiacBadges(BYTE id, BYTE value)
{
	char szZodiac[QUEST_STATE_MAX_LEN + 1];
	snprintf(szZodiac, sizeof(szZodiac), "zodiac.Insignia%d", value);
	SetQuestFlag(szZodiac, id);
}
BYTE CHARACTER::GetZodiacBadges(BYTE value)
{
	char szZodiac[QUEST_STATE_MAX_LEN + 1];
	snprintf(szZodiac, sizeof(szZodiac), "zodiac.Insignia%d", value);
	int zoddi = GetQuestFlag(szZodiac);
	return zoddi;
}
#endif
void CHARACTER::FishEventUseBox(TItemPos itemPos)
{
	if(itemPos.window_type != INVENTORY)
		return;

	if(!GetDesc())
		return;

	LPITEM item = NULL;

	if(!CanHandleItem())
		return;
	
	if(!IsValidItemPosition(itemPos) || !( item = GetItem(itemPos) ))
		return;

	if(item->IsExchanging())
		return;
	
	if(item->GetVnum() == ITEM_FISH_EVENT_BOX)
	{
		BYTE randomShape = random_number(FISH_EVENT_SHAPE_1, FISH_EVENT_SHAPE_6);
		SetFishAttachedShape(randomShape);
		FishEventIncreaseUseCount();
		item->SetCount(item->GetCount() - 1);
	}
	else if(item->GetVnum() == ITEM_FISH_EVENT_BOX_SPECIAL)
	{
		SetFishAttachedShape(FISH_EVENT_SHAPE_7);
		FishEventIncreaseUseCount();
		item->SetCount(item->GetCount() - 1);
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You can not use this item here."));
		return;
	}

	SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_BOX_USE, GetFishAttachedShape(), GetFishEventUseCount());
}

bool CHARACTER::FishEventIsValidPosition(BYTE shapePos, BYTE shapeType)
{
	BYTE positionList[ 7 ][ 6 ] = {
		{ 0, 6, 12, 0, 0, 0 }, // 1
		{ 0, 0, 0, 0, 0, 0 },  // 2
		{ 0, 6, 7, 0, 0, 0 },  // 3
		{ 0, 1, 7, 0, 0, 0 },  // 4
		{ 0, 1, 6, 7, 0, 0 },  // 5
		{ 0, 1, 7, 8, 0, 0 },  // 6
		{ 0, 1, 2, 6, 7, 8 },  // 7
	};
	
	BYTE shapeList[6];
	memcpy(shapeList, positionList[shapeType - 1], sizeof(shapeList));

	tchat("m_fishSlots[%d]=%d; shapeList[%d]=%d", shapePos, m_fishSlots[shapePos].shape(), shapePos, shapeList[shapePos]);
	
	if(	  m_fishSlots[shapePos + shapeList[0]].shape() == 0
	   && m_fishSlots[shapePos + shapeList[1]].shape() == 0
	   && m_fishSlots[shapePos + shapeList[2]].shape() == 0
	   && m_fishSlots[shapePos + shapeList[3]].shape() == 0
	   && m_fishSlots[shapePos + shapeList[4]].shape() == 0
	   && m_fishSlots[shapePos + shapeList[5]].shape() == 0)
	{
		tchat("Valide Pos");
		return true;
	}

	tchat("WRONG Pos %d,%d,%d,%d,%d,%d", shapePos + shapeList[0], shapePos + shapeList[1], shapePos + shapeList[2], shapePos + shapeList[3],shapePos + shapeList[4], shapePos + shapeList[5]);
	tchat("WRONG Pos %d,%d,%d,%d,%d,%d", m_fishSlots[shapePos + shapeList[0]].shape(), m_fishSlots[shapePos + shapeList[1]].shape(), m_fishSlots[shapePos + shapeList[2]].shape(), m_fishSlots[shapePos + shapeList[3]].shape(), m_fishSlots[shapePos + shapeList[4]].shape(), m_fishSlots[shapePos + shapeList[5]].shape());
	return false;
}

void CHARACTER::FishEventPlaceShape(BYTE shapePos, BYTE shapeType)
{
	BYTE positionList[ 7 ][ 7 ] = {
		{ FISH_EVENT_SHAPE_1, 0, 6, 12, 0, 0, 0 },
		{ FISH_EVENT_SHAPE_2, 0, 0, 0, 0, 0, 0 },
		{ FISH_EVENT_SHAPE_3, 0, 6, 7, 0, 0, 0 },
		{ FISH_EVENT_SHAPE_4, 0, 1, 7, 0, 0, 0 },
		{ FISH_EVENT_SHAPE_5, 0, 1, 6, 7, 0, 0 },
		{ FISH_EVENT_SHAPE_6, 0, 1, 7, 8, 0, 0 },
		{ FISH_EVENT_SHAPE_7, 0, 1, 2, 6, 7, 8 },
	};

	for(int i = 0; i < sizeof(positionList) / sizeof(positionList[ 0 ]); i++)
	{
		if(positionList[ i ][ 0 ] == shapeType)
		{
			for(int j = 1; j < 7; j++)
			{
				if(j > 1 && positionList[ i ][ j ] == 0)
					continue;

				if(positionList[ i ][ j ] == 0)
				{
					m_fishSlots[ shapePos ].set_is_main(true);
					m_fishSlots[ shapePos ].set_shape(shapeType);
					tchat("Placed[%d]", shapePos);
				}
				else
				{
					m_fishSlots[ shapePos + positionList[ i ][ j ] ].set_is_main(false);
					m_fishSlots[ shapePos + positionList[ i ][ j ] ].set_shape(shapeType);
					tchat("Placed_sec[%d]", shapePos + positionList[ i ][ j ] );
				}
			}

			break;
		}
	}
}

void CHARACTER::FishEventCheckEnd()
{
	bool isComplete = true;

	for(int i = 0; i < FISH_EVENT_SLOTS_NUM; i++)
	{
		if(m_fishSlots[ i ].shape() == 0)
		{
			isComplete = false;
			break;
		}
	}

	if(isComplete)
	{
		DWORD dwUseCount = GetFishEventUseCount();

		DWORD dwRewardVnum = 0, dwAmount = 0;

		if(dwUseCount <= 10)
			dwRewardVnum = 25313, dwAmount = 3;

		else if(dwUseCount <= 24)
			dwRewardVnum = 25312, dwAmount = 2;

		else dwRewardVnum = 25311, dwAmount = 1;

		for(int i = 0; i < FISH_EVENT_SLOTS_NUM; i++)
		{
			m_fishSlots[ i ].set_is_main(false);
			m_fishSlots[ i ].set_shape(0);
		}

		for( int i = 0; i < dwAmount; ++i )
			AutoGiveItem(dwRewardVnum);

		PointChange(POINT_EXP, 30000);

		m_dwFishUseCount = 0;
		SetFishAttachedShape(0);

		SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_GC_REWARD, dwRewardVnum);
	}
}

void CHARACTER::FishEventAddShape(BYTE shapePos)
{
	if(!GetDesc())
		return;
	
	if(shapePos >= FISH_EVENT_SLOTS_NUM)
		return;

	BYTE lastAttachedShape = GetFishAttachedShape();

	if(lastAttachedShape == 0 || lastAttachedShape > ITEM_FISH_EVENT_BOX_SPECIAL)
		return;

	if(!FishEventIsValidPosition(shapePos, lastAttachedShape))
	{
		SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_BOX_USE, GetFishAttachedShape(), GetFishEventUseCount());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "This shape doesn't fit on this position."));
		return;
	}

	FishEventPlaceShape(shapePos, lastAttachedShape);
	SendFishInfoAsCommand(FISH_EVENT_SUBHEADER_SHAPE_ADD, shapePos, lastAttachedShape);
	FishEventCheckEnd();
}
#endif

bool CHARACTER::IsPacketLoggingEnabled()
{
	return bool(GetQuestFlag("security.logging"));
}

#ifdef ENABLE_RUNE_PAGES
void CHARACTER::SaveSelectedRunes()
{
	for (int i = 0; i < RUNE_PAGE_COUNT; ++i)
	{
		BYTE group = m_selectedRunes[i].main_group();

		BYTE runes[RUNE_MAIN_COUNT] = { 0 };
		for (int j = 0; j < m_selectedRunes[i].main_vnum_size() && j < RUNE_MAIN_COUNT; ++j)
			runes[j] = m_selectedRunes[i].main_vnum(j);

		BYTE subGroup = m_selectedRunes[i].sub_group();
		BYTE subRune1 = m_selectedRunes[i].sub_vnum_size() > 0 ? m_selectedRunes[i].sub_vnum(0) : 0;
		BYTE subRune2 = m_selectedRunes[i].sub_vnum_size() > 1 ? m_selectedRunes[i].sub_vnum(1) : 0;

		if (!group && std::find_if(std::begin(runes), std::end(runes), [](BYTE rune) { return rune != 0; }) == std::end(runes) &&
			!subGroup && !subRune1 && !subRune2)
			continue;

		DBManager::instance().Query("REPLACE INTO rune_pages VALUES (%u, %u, %u, %u, %u, %u, %u, %u, %u, %u)",
			GetPlayerID(), i, group, runes[0], runes[1], runes[2], runes[3], subGroup, subRune1, subRune2);
	}
}

void CHARACTER::LoadSelectedRunes()
{
	std::auto_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery("SELECT page, main_group, rune1, rune2, rune3, rune4, sub_group, sub_rune1, sub_rune2 FROM rune_pages WHERE pid = %u", GetPlayerID()));
	SQLResult *pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
		return;

	MYSQL_ROW data = NULL;

	while ((data = mysql_fetch_row(pRes->pSQLResult)))
	{
		BYTE col = 0;

		BYTE page = 0;
		str_to_number(page, data[col++]);
		m_selectedRunes[page].Clear();

		BYTE group = 0;
		str_to_number(group, data[col++]);

		DWORD runes[4];
		str_to_number(runes[0], data[col++]);
		str_to_number(runes[1], data[col++]);
		str_to_number(runes[2], data[col++]);
		str_to_number(runes[3], data[col++]);

		BYTE subGroup = 0;
		str_to_number(subGroup, data[col++]);
		BYTE subRune1 = 0;
		str_to_number(subRune1, data[col++]);
		BYTE subRune2 = 0;
		str_to_number(subRune2, data[col++]);

		m_selectedRunes[page].set_main_group(group);

		for (int i = 0; i < RUNE_MAIN_COUNT; ++i)
			m_selectedRunes[page].add_main_vnum(runes[i]);

		m_selectedRunes[page].set_sub_group(subGroup);
		m_selectedRunes[page].add_sub_vnum(subRune1);
		m_selectedRunes[page].add_sub_vnum(subRune2);
	}
}
#endif

#ifdef CHANGE_SKILL_COLOR
void CHARACTER::SetSkillColor(const ::google::protobuf::RepeatedPtrField<::google::protobuf::RepeatedField<google::protobuf::uint32>>& skillColor)
{
	DWORD arr_skillColor[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	for (int skill_cnt = 0; skill_cnt < ESkillColorLength::MAX_SKILL_COUNT; ++skill_cnt)
	{
		for (int eff_cnt = 0; eff_cnt < ESkillColorLength::MAX_EFFECT_COUNT; ++eff_cnt)
		{
			if (skill_cnt >= skillColor.size() || eff_cnt >= skillColor[skill_cnt].size())
				arr_skillColor[skill_cnt][eff_cnt] = 0;
			else
				arr_skillColor[skill_cnt][eff_cnt] = skillColor[skill_cnt][eff_cnt];
		}
	}

	SetSkillColor((DWORD*) arr_skillColor);
}

void CHARACTER::SetSkillColor(DWORD * dwSkillColor)
{
	memcpy(m_dwSkillColor, dwSkillColor, sizeof(m_dwSkillColor));

#ifdef RESET_SKILL_COLOR
	auto checkValidColor = [](const DWORD &color) {
		BYTE *colors = (BYTE*)&color;

		bool isFine = true;

		if (colors[0] < 40 || colors[1] < 40 || colors[2] < 40)
			isFine = false;

		if (colors[0] >= 40 || colors[1] >= 40 || colors[2] >= 40)
			isFine = true;

		return isFine;
	};

	for (int i = 0; i < ESkillColorLength::MAX_EFFECT_COUNT; ++i)
	{
		for (int j = 0; j < ESkillColorLength::MAX_EFFECT_COUNT; ++j)
		{
			if (!checkValidColor(m_dwSkillColor[i][j]))
			{
				memset(&m_dwSkillColor[i][j], 0, sizeof(m_dwSkillColor[i][j]));
			}
		}
	}
#endif

	UpdatePacket();
}
#endif

#ifdef __EQUIPMENT_CHANGER__
void CHARACTER::SetEquipmentChangerPageIndex(DWORD dwPageIndex, bool bForce)
{
	if (!m_bIsEquipmentChangerLoaded)
		return;

	if (m_dwEquipmentChangerPageIndex == dwPageIndex && !bForce)
		return;

	if (dwPageIndex >= m_vec_EquipmentChangerPages.size())
	{
		sys_err("invalid page index %u (size %u)", dwPageIndex, m_vec_EquipmentChangerPages.size());
		return;
	}

	m_dwEquipmentChangerPageIndex = dwPageIndex;
	auto& rkTab = m_vec_EquipmentChangerPages[dwPageIndex];

	// change equipment
	bool bSentMessage = false;

	LPITEM pkItemCur, pkItemNew;
	for (int i = 0; i < sizeof(EquipmentChangerSlots) / sizeof(EquipmentChangerSlots[0]); ++i)
	{
		const BYTE& rbSlot = EquipmentChangerSlots[i];
		pkItemNew = (rkTab.item_ids_size() > i && rkTab.item_ids(i) != 0) ? FindItemByID(rkTab.item_ids(i), true) : NULL;

		if (pkItemNew && pkItemNew->GetOwner() != this)
		{
			LogManager::instance().HackDetectionLog(this, "EQCHANGER_ITEM_FAKE_TRY", "");
			return;
		}

		if (pkItemCur = GetWear(rbSlot))
		{
			if (pkItemCur == pkItemNew)
				continue;

			// switch!
			if (pkItemNew)
			{
				bool bCanSwitch = pkItemNew->GetSize() >= pkItemCur->GetSize(); // check size
				// check if empty pos below
				if (!bCanSwitch)
				{
					bCanSwitch = GetEmptyInventory(pkItemCur->GetSize()) == -1 ? false : true;

					if (!IsEmptyItemGrid(TItemPos(INVENTORY, pkItemNew->GetCell()), pkItemCur->GetSize(), pkItemNew->GetCell()))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Move %s to another slot where it has %u slots space."), pkItemCur->GetName(GetLanguageID()), pkItemCur->GetSize());
						return;
					}

					if (!bCanSwitch)
					{
						ChatPacket(CHAT_TYPE_INFO, "There is not enough space in your inventory.");
						return;
					}
				}

				if (bCanSwitch)
				{
					WORD wInventoryCell = pkItemNew->GetCell();
					pkItemCur->RemoveFromCharacter();
					if (!EquipItem(pkItemNew, -1, true, true, true))
					{
						sys_err("cannot equip new item %u %u to %u %s", pkItemNew->GetID(), pkItemNew->GetVnum(), GetPlayerID(), GetName());
						AutoGiveItem(pkItemCur);
						continue;
					}
					pkItemCur->AddToCharacter(this, TItemPos(INVENTORY, wInventoryCell));

					continue;
				}
			}
			else
			{
				bool bCanSwitch = GetEmptyInventory(pkItemCur->GetSize()) == -1 ? false : true;

				if (!bCanSwitch)
				{
					ChatPacket(CHAT_TYPE_INFO, "There is not enough space in your inventory.");
					return;
				}
			}

			pkItemCur->RemoveFromCharacter();
			AutoGiveItem(pkItemCur, true);
		}

		if (pkItemNew)
		{
			EquipItem(pkItemNew, -1, true);
		}
		else if (!bSentMessage && rkTab.item_ids_size() > i && rkTab.item_ids(i))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Not all items of this equipment set are in your inventory."));
			bSentMessage = true;
		}
	} 

#ifdef ENABLE_RUNE_PAGES
	std::string cmd = "get_rune_page " + std::to_string(rkTab.rune_page());
	direct_interpret_command(cmd.c_str(), this);
#endif
}

void CHARACTER::LoadEquipmentChanger(const ::google::protobuf::RepeatedPtrField<network::TEquipmentChangerTable>& tables)
{
	if (m_bIsEquipmentChangerLoaded)
	{
		sys_err("already loaded equipment changer for pid %u", GetPlayerID());
		return;
	}

	sys_log(!test_server, "CHARACTER::LoadEquipmentChanger %s %u", GetName(), tables.size());

	for (int i = 0; i < tables.size(); ++i)
	{
		auto& cur = tables[i];

		if (test_server)
			sys_log(0, " : Load %u (%s %u)", i, cur.page_name().c_str(), cur.index());

		m_vec_EquipmentChangerPages.push_back(cur);

		if (m_vec_EquipmentChangerPages[i].index() != i)
		{
			sys_err("invalid page index (player %u %s) %u for real page %u (name %s)",
				GetPlayerID(), GetName(), m_vec_EquipmentChangerPages[i].index(), i, m_vec_EquipmentChangerPages[i].page_name().c_str());
			m_vec_EquipmentChangerPages[i].set_index(i);
		}
	}

	m_bIsEquipmentChangerLoaded = true;

	if (GetDesc()->IsPhase(PHASE_GAME) || GetDesc()->IsPhase(PHASE_DEAD))
		SendEquipmentChangerLoadPacket();
}

void CHARACTER::SendEquipmentChangerLoadPacket(bool bForce)
{
	if (!m_bIsEquipmentChangerLoaded)
		return;

	if (!m_vec_EquipmentChangerPages.size() && !bForce)
		return;

	sys_log(!test_server, "CHARACTER::SendEquipmentChangerLoadPacket size %u", m_vec_EquipmentChangerPages.size());
	
	network::GCOutputPacket<network::GCEquipmentPageLoadPacket> pack;
	pack->set_selected_index(m_dwEquipmentChangerPageIndex);

	for (auto& page : m_vec_EquipmentChangerPages)
	{
		auto info = pack->add_pages();

		info->set_page_name(page.page_name());
		info->set_rune_page(page.rune_page());
		for (int j = 0; j < EQUIPMENT_PAGE_MAX_PARTS; ++j)
		{
			LPITEM item = page.item_ids_size() > j ? ITEM_MANAGER::instance().Find(page.item_ids(j)) : nullptr;
			info->add_item_cells(item ? item->GetCell() : -1);
		}
	}

	GetDesc()->Packet(pack);
}

void CHARACTER::AddEquipmentChangerPage(const char* szName)
{
	if (GetLevel() < 30)
	{
		ChatPacket(CHAT_TYPE_INFO, "Lv 30 +");
		return;
	}

	network::TEquipmentChangerTable kNewTable;
	int iNewIndex = m_vec_EquipmentChangerPages.size();

	auto pkTab = &kNewTable;
	for (int i = 0; i < m_vec_EquipmentChangerPages.size(); ++i)
	{
		if (!*m_vec_EquipmentChangerPages[i].page_name().c_str())
		{
			pkTab = &m_vec_EquipmentChangerPages[i];
			iNewIndex = i;
			break;
		}
	}

	// maximum pages: 10
	if (pkTab == &kNewTable && m_vec_EquipmentChangerPages.size() >= EQUIPMENT_CHANGER_MAX_PAGES)
		return;

	sys_log(!test_server, "CHARACTER::AddEquipmentChangerPage %s %s new index %u vector count %u",
			GetName(), szName, iNewIndex, m_vec_EquipmentChangerPages.size());

	pkTab->Clear();

	pkTab->set_pid(GetPlayerID());
	pkTab->set_index(iNewIndex);
	pkTab->set_rune_page(GetQuestFlag("rune.current_page"));
	pkTab->set_page_name(szName);

	for (int i = 0; i < sizeof(EquipmentChangerSlots) / sizeof(EquipmentChangerSlots[0]); ++i)
	{
		BYTE bWearSlot = EquipmentChangerSlots[i];
		pkTab->add_item_ids(GetWear(bWearSlot) ? GetWear(bWearSlot)->GetID() : 0);
		tchat("%s i%i wear %d , %u", __FUNCTION__, i, bWearSlot, pkTab->item_ids(i));
	}

	if (pkTab == &kNewTable)
		m_vec_EquipmentChangerPages.push_back(kNewTable);

	m_dwEquipmentChangerPageIndex = m_vec_EquipmentChangerPages.size()-1;

	SendEquipmentChangerLoadPacket();
}

void CHARACTER::DeleteEquipmentChangerPage(DWORD dwIndex)
{
	if (dwIndex >= m_vec_EquipmentChangerPages.size())
	{
		sys_err("invalid page index %u", dwIndex);
		return;
	}

	if (!*m_vec_EquipmentChangerPages[dwIndex].page_name().c_str())
	{
		sys_err("already deleted page %u", dwIndex);
		return;
	}

	sys_log(!test_server, "CHARACTER::DeleteEquipmentChangerPage %s %u %s", GetName(), dwIndex, m_vec_EquipmentChangerPages[dwIndex].page_name().c_str());

	m_vec_EquipmentChangerPages.erase(m_vec_EquipmentChangerPages.begin() + dwIndex);

	if (m_dwEquipmentChangerPageIndex == dwIndex)
		m_dwEquipmentChangerPageIndex = 0;

	network::GDOutputPacket<network::GDEquipmentPageDeletePacket> p;
	p->set_pid(GetPlayerID());
	db_clientdesc->DBPacket(p);
	
	SaveEquipmentChanger();

	SendEquipmentChangerLoadPacket(true);
}

/*void CHARACTER::UpdateEquipmentChangerItem(BYTE bWearSlot)
{
	if (m_dwEquipmentChangerPageIndex >= m_vec_EquipmentChangerPages.size())
		return;

	tchat("UpdateEquipmentChangerItem(%d) page%d", bWearSlot, m_dwEquipmentChangerPageIndex);

	if (bWearSlot >= EQUIPMENT_PAGE_MAX_PARTS)
	{
		tchat("bWearSlot >= EQUIPMENT_PAGE_MAX_PARTS)");
		return;
	}

	TEquipmentChangerTable& rkTab = m_vec_EquipmentChangerPages[m_dwEquipmentChangerPageIndex];
	rkTab.adwItemID[bWearSlot] = GetWear(bWearSlot) ? GetWear(bWearSlot)->GetID() : 0;
}*/

void CHARACTER::SetEquipmentChangerName(DWORD dwIndex, const char* szPageName)
{
	if (dwIndex >= m_vec_EquipmentChangerPages.size())
		return;

	auto& rkTab = m_vec_EquipmentChangerPages[dwIndex];
	if (!*rkTab.page_name().c_str())
	{
		sys_err("cannot set page name of deleted page");
		return;
	}

	rkTab.set_page_name(szPageName);

	SendEquipmentChangerLoadPacket();
}

void CHARACTER::SaveEquipmentChanger()
{
	sys_log(!test_server, "CHARACTER::SaveEquipmentChanger %u", m_vec_EquipmentChangerPages.size());

	network::GDOutputPacket<network::GDEquipmentPageSavePacket> pack;
	for (auto& page : m_vec_EquipmentChangerPages)
		*pack->add_pages() = page;
	db_clientdesc->DBPacket(pack);
}
#endif

#ifdef ENABLE_COMPANION_NAME
void CHARACTER::SaveCompanionNames()
{
	if (m_petNameTimeLeft > get_global_time() || m_mountNameTimeLeft > get_global_time() || !m_stFakeBuffName.empty())
		DBManager::instance().Query("REPLACE INTO companion_name VALUES (%u, %u, '%s', %u, '%s', '%s')",
			GetPlayerID(), m_petNameTimeLeft, m_stPetName.c_str(), m_mountNameTimeLeft, m_stMountName.c_str(), m_stFakeBuffName.c_str());
}

void CHARACTER::LoadCompanionNames()
{
	std::auto_ptr<SQLMsg> pkMsg(DBManager::instance().DirectQuery("SELECT pet_time, pet_name, mount_time, mount_name, fakebuff_name FROM companion_name WHERE pid = %u", GetPlayerID()));
	SQLResult *pRes = pkMsg->Get();

	if (!pRes->uiNumRows)
		return;

	MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);

	str_to_number(m_petNameTimeLeft, row[0]);
	if (m_petNameTimeLeft > get_global_time())
		m_stPetName = row[1];

	str_to_number(m_mountNameTimeLeft, row[2]);
	if (m_mountNameTimeLeft > get_global_time())
		m_stMountName = row[3];
	
	m_stFakeBuffName = row[4];

	// ComputePoints();
}

void CHARACTER::SetPetName(const char *name)
{
	m_stPetName = name;
	if (name != "")
		m_petNameTimeLeft = get_global_time() + (3600 * 24 * 7);

	if (GetPetSystem())
		GetPetSystem()->OnNameChange();

	ComputePoints();
	SendCompanionNameInfo();
}

void CHARACTER::SetMountName(const char *name)
{
	m_stMountName = name;
	if (name != "")
		m_mountNameTimeLeft = get_global_time() + (3600 * 24 * 7);

	CMountSystem *mount = GetMountSystem();
	if (!mount)
		return;

	mount->SetName(name == "" ? GetName() : name);

	LPCHARACTER ch = mount->GetMount();
	if (!ch)
		return;

	ch->SetName(name == "" ? GetName() : name);
	ch->SetCompanionHasName(name != "");
	ch->ViewReencode();

	ComputePoints();
	SendCompanionNameInfo();
}

void CHARACTER::SetFakebuffName(const char *name)
{
	m_stFakeBuffName = name;
	SendCompanionNameInfo();
}

void CHARACTER::SendCompanionNameInfo()
{
	DWORD time1 = get_global_time() > m_petNameTimeLeft ? 0 : m_petNameTimeLeft - get_global_time();
	DWORD time2 = get_global_time() > m_mountNameTimeLeft ? 0 : m_mountNameTimeLeft - get_global_time();
	ChatPacket(CHAT_TYPE_COMMAND, "SetCompanionNameInfo %u %u %u", time1, time2, (BYTE)!m_stFakeBuffName.empty());
}
#endif

#ifdef LEADERSHIP_EXTENSION
void CHARACTER::SetLeadershipState(BYTE state)
{
	const BYTE abResetPoints[] = {
		POINT_PARTY_ATTACKER_BONUS,
		POINT_PARTY_TANKER_BONUS,
		POINT_PARTY_BUFFER_BONUS,
		POINT_PARTY_SKILL_MASTER_BONUS,
		POINT_PARTY_DEFENDER_BONUS,
		POINT_PARTY_HASTE_BONUS,
	};

	bool bResetAny = false;

	for (BYTE bPoint : abResetPoints)
	{
		int iCurPoint = GetPoint(bPoint);

		if (iCurPoint)
		{
			bResetAny = true;
			PointChange(bPoint, -iCurPoint);
		}
	}

	if (bResetAny)
		ComputePoints();

	float k = (float)GetSkillPowerByLevel(MIN(SKILL_MAX_LEVEL, GetLeadershipSkillLevel())) / 100.0f;

	switch (state)
	{
	case 1:
		PointChange(POINT_PARTY_ATTACKER_BONUS, (int)(10 + 88 * k));
		break;
	case 2:
		PointChange(POINT_PARTY_HASTE_BONUS, (int)(1 + 7.2 * k));
		break;
	case 3:
		PointChange(POINT_PARTY_TANKER_BONUS, (int)(50 + 1960 * k));
		break;
	case 4:
		PointChange(POINT_PARTY_BUFFER_BONUS, (int)(5 + 92 * k));
		break;
	case 5:
		PointChange(POINT_PARTY_SKILL_MASTER_BONUS, (int)(25 + 780 * k));
		break;
	case 6:
		PointChange(POINT_PARTY_DEFENDER_BONUS, (int)(5 + 76 * k));
		break;
	default:
		break;
	}

	if (state)
		ComputePoints();

	ChatPacket(CHAT_TYPE_COMMAND, "SetLeadershipInfo %d", state);
}
#endif

#ifdef BATTLEPASS_EXTENSION
void CHARACTER::SendBattlepassData(int index)
{
	network::GCOutputPacket<network::GCBattlepassDataPacket> pack;
	pack->set_index(index);
	*pack->mutable_data() = m_battlepassData[index];

	GetDesc()->Packet(pack);
}
#endif

#ifdef DMG_RANKING
void CHARACTER::registerDamageToDummy(const EDamageType &type, const int &dmg)
{
	switch (type)
	{
		case DAMAGE_TYPE_NORMAL:
		case DAMAGE_TYPE_NORMAL_RANGE:

			if (m_dummyHitCount == 0)
				m_dummyHitStartTime = get_dword_time();
			else if (m_dummyHitStartTime + 15000 < get_dword_time())
				break;

			m_totalDummyDamage += dmg;
			++m_dummyHitCount;

			if (m_dummyHitCount < 10)
				return;

			// tchat("total damage %d", m_totalDummyDamage);
			CDmgRankingManager::instance().registerToDmgRanks(this, TYPE_DMG_NORMAL, m_totalDummyDamage);
			break;
		case DAMAGE_TYPE_MELEE: // i think these are all for skills
		case DAMAGE_TYPE_RANGE:
		case DAMAGE_TYPE_MAGIC:
			CDmgRankingManager::instance().registerToDmgRanks(this, TYPE_DMG_SKILL, dmg);
			break;
	}

	m_dummyHitCount = 0;
	m_dummyHitStartTime = 0;
	m_totalDummyDamage = 0;
}
#endif

bool CHARACTER::IsIgnorePenetrateCritical()
{
	switch (GetRaceNum())
	{
		case 35074:
			return true;
	}
	return false;
}

#ifdef __PRESTIGE__
void CHARACTER::Prestige_SetLevel(BYTE bLevel)
{
	m_bPrestigeLevel = bLevel;
	Save();
	UpdatePacket();
}
#endif

#ifdef CRYSTAL_SYSTEM
void CHARACTER::set_active_crystal_id(DWORD item_id)
{
	SetQuestFlag(ACTIVE_CRYSTAL_ID_FLAG, item_id);
}

void CHARACTER::clear_active_crystal_id()
{
	set_active_crystal_id(0);
}

DWORD CHARACTER::get_active_crystal_id() const
{
	return GetQuestFlag(ACTIVE_CRYSTAL_ID_FLAG);
}

void CHARACTER::check_active_crystal()
{
	DWORD item_id = get_active_crystal_id();
	if (item_id == 0)
		return;

	if (LPITEM item = FindItemByID(item_id))
	{
		if (!item->is_crystal_using())
			item->start_crystal_use(false);

		if (!item->is_crystal_using())
			set_active_crystal_id(0);
	}
	else
		set_active_crystal_id(0);
}
#endif
