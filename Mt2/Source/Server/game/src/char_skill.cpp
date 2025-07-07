#include "stdafx.h"
#include <sstream>

#include "utils.h"
#include "config.h"
#include "vector.h"
#include "char.h"
#include "char_manager.h"
#include "battle.h"
#include "desc.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "item.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "start_position.h"
#include "party.h"
#include "buffer_manager.h"
#include "guild.h"
#include "log.h"
#include "unique_item.h"
#include "questmanager.h"
#include "item_manager.h"

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

extern int test_server;

#ifdef __WOLFMAN__
struct FPartyPIDCollector
{
	std::vector <DWORD> vecPIDs;
	FPartyPIDCollector()
	{
	}
	void operator () (LPCHARACTER ch)
	{
		vecPIDs.push_back(ch->GetPlayerID());
	}
};
#endif

static const DWORD s_adwSubSkillVnums[] =
{
	SKILL_LEADERSHIP,
	SKILL_COMBO,
	SKILL_MINING,
	SKILL_LANGUAGE1,
	SKILL_LANGUAGE2,
	SKILL_LANGUAGE3,
	SKILL_POLYMORPH,
	SKILL_HORSE,
	SKILL_HORSE_SUMMON,
	SKILL_HORSE_WILDATTACK,
	SKILL_HORSE_CHARGE,
	SKILL_HORSE_ESCAPE,
	SKILL_HORSE_WILDATTACK_RANGE,
	SKILL_ADD_HP,
	SKILL_RESIST_PENETRATE
};

time_t CHARACTER::GetSkillNextReadTime(DWORD dwVnum) const
{
	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (vnum: %u)", dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].next_read() : 0;
}

void CHARACTER::SetSkillNextReadTime(DWORD dwVnum, time_t time)
{
	if (m_pSkillLevels && dwVnum < SKILL_MAX_NUM)
		m_pSkillLevels[dwVnum].set_next_read(time);
}

bool TSkillUseInfo::HitOnce(DWORD dwVnum)
{
	// ¾²Áöµµ¾Ê¾ÒÀ¸¸é ¶§¸®Áöµµ ¸øÇÑ´Ù.
	if (!bUsed)
		return false;

	sys_log(1, "__HitOnce NextUse %u current %u count %d scount %d", dwNextSkillUsableTime, get_dword_time(), iHitCount, iSplashCount);

	if (dwNextSkillUsableTime && dwNextSkillUsableTime<get_dword_time() && dwVnum != SKILL_MUYEONG && dwVnum != SKILL_HORSE_WILDATTACK)
	{
		sys_log(1, "__HitOnce can't hit");

		return false;
	}

	if (iHitCount == -1)
	{
		sys_log(1, "__HitOnce OK %d %d %d", dwNextSkillUsableTime, get_dword_time(), iHitCount);
		return true;
	}

	if (iHitCount)
	{
		sys_log(1, "__HitOnce OK %d %d %d", dwNextSkillUsableTime, get_dword_time(), iHitCount);
		iHitCount--;
		return true;
	}
	return false;
}

bool TSkillUseInfo::IsCooltimeOver() const
{
	if (bUsed && dwNextSkillUsableTime > get_dword_time())
		return false;

	return true;
}

bool TSkillUseInfo::UseSkill(bool isGrandMaster, DWORD vid, DWORD dwCooltime, int splashcount, int hitcount, int range)
{
	this->isGrandMaster = isGrandMaster;
	DWORD dwCur = get_dword_time();

	// ¾ÆÁ÷ ÄðÅ¸ÀÓÀÌ ³¡³ªÁö ¾Ê¾Ò´Ù.
	if (bUsed && dwNextSkillUsableTime > dwCur)
	{
		sys_log(0, "cooltime is not over delta %u", dwNextSkillUsableTime - dwCur);
		iHitCount = 0;
		return false;
	}

	bUsed = true;

	if (dwCooltime)
		dwNextSkillUsableTime = dwCur + dwCooltime;
	else
		dwNextSkillUsableTime = 0;

	iRange = range;
	iMaxHitCount = iHitCount = hitcount;

	if (test_server)
		sys_log(0, "UseSkill NextUse %u  current %u cooltime %d hitcount %d/%d", dwNextSkillUsableTime, dwCur, dwCooltime, iHitCount, iMaxHitCount);

	dwVID = vid;
	iSplashCount = splashcount;
	return true;
}

int CHARACTER::GetChainLightningMaxCount() const
{ 
	return aiChainLightningCountBySkillLevel[MIN(SKILL_MAX_LEVEL, GetSkillLevel(SKILL_CHAIN))];
}

void CHARACTER::SetAffectedEunhyung() 
{ 
	m_dwAffectedEunhyungLevel = GetSkillPower(SKILL_EUNHYUNG); 
}

void CHARACTER::SetSkillGroup(BYTE bSkillGroup)
{
	if (bSkillGroup > 2) 
		return;

	if (GetLevel() < 5)
		return;

	m_points.skill_group = bSkillGroup; 

	network::GCOutputPacket<network::GCChangeSkillGroupPacket> p;
	p->set_skill_group(m_points.skill_group);

	if (GetDesc())
		GetDesc()->Packet(p);
}

int CHARACTER::ComputeCooltime(int time)
{
	return CalculateDuration(GetPoint(POINT_CASTING_SPEED), time);
}

void CHARACTER::SkillLevelPacket()
{
	if (!GetDesc())
		return;

	network::GCOutputPacket<network::GCSkillLevelPacket> pack;
	for (int i = 0; i < SKILL_MAX_NUM; ++i)
		*pack->add_levels() = m_pSkillLevels[i];
	GetDesc()->Packet(pack);
}

void CHARACTER::SetSkillLevelChanged()
{
	if (IsPC()
#ifdef COMBAT_ZONE
			 && !CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex())
#endif
		)
		m_abPlayerDataChanged[PC_TAB_CHANGED_SKILLS] = true;
}

void CHARACTER::SetSkillLevel(DWORD dwVnum, BYTE bLev)
{
	if (NULL == m_pSkillLevels)
		return;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (vnum %u)", dwVnum);
		return;
	}

	m_pSkillLevels[dwVnum].set_level(MIN(SKILL_MAX_LEVEL, bLev));

#ifdef __LEGENDARY_SKILL__
	if (bLev >= 50)
		m_pSkillLevels[dwVnum].set_master_type(SKILL_LEGENDARY_MASTER);
	else
#endif
	if (bLev >= 40)
		m_pSkillLevels[dwVnum].set_master_type(SKILL_PERFECT_MASTER);
	else if (bLev >= 30)
		m_pSkillLevels[dwVnum].set_master_type(SKILL_GRAND_MASTER);
	else if (bLev >= 20)
		m_pSkillLevels[dwVnum].set_master_type(SKILL_MASTER);
	else
		m_pSkillLevels[dwVnum].set_master_type(SKILL_NORMAL);

	SetSkillLevelChanged();
}

bool CHARACTER::IsLearnableSkill(DWORD dwSkillVnum) const
{
	const CSkillProto * pkSkill = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSkill)
		return false;

	if (GetSkillLevel(dwSkillVnum) >= SKILL_MAX_LEVEL)
		return false;

	if (pkSkill->dwType == 0)
	{
		if (GetSkillLevel(dwSkillVnum) >= pkSkill->bMaxLevel)
			return false;

		return true;
	}

	if (pkSkill->dwType == 5)
	{
		if (dwSkillVnum == SKILL_HORSE_WILDATTACK_RANGE && GetJob() != JOB_ASSASSIN)
			return false; 

		return true;
	}

	if (GetSkillGroup() == 0)
		return false;

	if (pkSkill->dwType - 1 == GetJob())
		return true;

#ifdef __WOLFMAN__
	if (7 == pkSkill->dwType && JOB_WOLFMAN == GetJob())
		return true;
#endif

	if (6 == pkSkill->dwType)
	{
		if (SKILL_7_A_ANTI_TANHWAN <= dwSkillVnum && dwSkillVnum <= SKILL_7_D_ANTI_YONGBI)
		{
			for (int i=0 ; i < 4 ; i++)
			{
				if (unsigned(SKILL_7_A_ANTI_TANHWAN + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_7_A_ANTI_TANHWAN + i))
					{
						return false;
					}
				}
			}

			return true;
		}

		if (SKILL_8_A_ANTI_GIGONGCHAM <= dwSkillVnum && dwSkillVnum <= SKILL_8_D_ANTI_BYEURAK)
		{
			for (int i=0 ; i < 4 ; i++)
			{
				if (unsigned(SKILL_8_A_ANTI_GIGONGCHAM + i) != dwSkillVnum)
				{
					if (0 != GetSkillLevel(SKILL_8_A_ANTI_GIGONGCHAM + i))
						return false;
				}
			}
			
			return true;
		}
	}

	return false;
}

// ADD_GRANDMASTER_SKILL
#ifdef __FAKE_BUFF__
bool CHARACTER::LearnGrandMasterSkill(DWORD dwSkillVnum, bool bFakeBuff)
#else
bool CHARACTER::LearnGrandMasterSkill(DWORD dwSkillVnum)
#endif
{
	CSkillProto * pkSk = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSk)
		return false;

#ifdef __FAKE_BUFF__
	if (bFakeBuff)
	{
		if (!IsPC() || GetFakeBuffSkillIdx(dwSkillVnum) == FAKE_BUFF_SKILL_COUNT)
			return false;

		if (!CanFakeBuffSkillUp(dwSkillVnum, false))
			return false;
	}
	else
#endif
	if (!IsLearnableSkill(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ö·ÃÇÒ ¼ö ¾ø´Â ½ºÅ³ÀÔ´Ï´Ù."));
		return false;
	}

	sys_log(0, "learn grand master skill[%d] cur %d, next %d", dwSkillVnum, get_global_time(), GetSkillNextReadTime(dwSkillVnum));

	/*
	   if (get_global_time() < GetSkillNextReadTime(dwSkillVnum))
	   {
	   if (!(test_server && quest::CQuestManager::instance().GetEventFlag("no_read_delay")))
	   {
	   if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
	   {
	// ÁÖ¾È¼ú¼­ »ç¿ëÁß¿¡´Â ½Ã°£ Á¦ÇÑ ¹«½Ã
	RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÁÖ¾È¼ú¼­¸¦ ÅëÇØ ÁÖÈ­ÀÔ¸¶¿¡¼­ ºüÁ®³ª¿Ô½À´Ï´Ù."));
	}
	else 		
	{
	SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
	return false;
	}
	}
	}
	 */

	// bTypeÀÌ 0ÀÌ¸é Ã³À½ºÎÅÍ Ã¥À¸·Î ¼ö·Ã °¡´É
	if (pkSk->dwType == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "±×·£µå ¸¶½ºÅÍ ¼ö·ÃÀ» ÇÒ ¼ö ¾ø´Â ½ºÅ³ÀÔ´Ï´Ù."));
		return false;
	}

#ifdef __FAKE_BUFF__
	if (!bFakeBuff)
#endif
	if (GetSkillMasterType(dwSkillVnum) != SKILL_GRAND_MASTER)
	{
		if (GetSkillMasterType(dwSkillVnum) > SKILL_GRAND_MASTER)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÆÛÆåÆ® ¸¶½ºÅÍµÈ ½ºÅ³ÀÔ´Ï´Ù. ´õ ÀÌ»ó ¼ö·Ã ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ½ºÅ³Àº ¾ÆÁ÷ ±×·£µå ¸¶½ºÅÍ ¼ö·ÃÀ» ÇÒ °æÁö¿¡ ÀÌ¸£Áö ¾Ê¾Ò½À´Ï´Ù."));
		return false;
	}

	std::string strTrainSkill;
	{
		std::ostringstream os;
		os << "training_grandmaster_skill.skill" << dwSkillVnum;
		strTrainSkill = os.str();
	}

	// ¿©±â¼­ È®·üÀ» °è»êÇÕ´Ï´Ù.
#ifdef __FAKE_BUFF__
	unsigned char bLastLevel;
	if (bFakeBuff)
		bLastLevel = GetFakeBuffSkillLevel(dwSkillVnum);
	else
		bLastLevel = GetSkillLevel(dwSkillVnum);
#else
	unsigned char bLastLevel = GetSkillLevel(dwSkillVnum);
#endif

	int idx = MINMAX(0, bLastLevel - 30, 9);

	sys_log(0, "LearnGrandMasterSkill %s table idx %d value %d", GetName(), idx, aiGrandMasterSkillBookCountForLevelUp[idx]);

	int iTotalReadCount = 0;
	if (IsPC())
	{
		iTotalReadCount = GetQuestFlag(strTrainSkill) + 1;
		SetQuestFlag(strTrainSkill, iTotalReadCount);
	}

	int iMinReadCount = aiGrandMasterSkillBookMinCount[idx];
	int iMaxReadCount = aiGrandMasterSkillBookMaxCount[idx];

	int iBookCount = aiGrandMasterSkillBookCountForLevelUp[idx];

	if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
	{
		if (iBookCount&1)
			iBookCount = iBookCount / 2 + 1; 
		else
			iBookCount = iBookCount / 2; 

		RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
	}

	int n = random_number(1, iBookCount);
	sys_log(0, "Number(%d)", n);

	DWORD nextTime = get_global_time() + random_number(28800, 43200);

	sys_log(0, "GrandMaster SkillBookCount min %d cur %d max %d (next_time=%d)", iMinReadCount, iTotalReadCount, iMaxReadCount, nextTime);

	bool bSuccess = n == 2;

	if (iTotalReadCount < iMinReadCount)
		bSuccess = false;
	if (iTotalReadCount > iMaxReadCount)
		bSuccess = true;

	if (bSuccess)
	{
#ifdef __FAKE_BUFF__
		if (bFakeBuff)
			SetFakeBuffSkillLevel(dwSkillVnum, bLastLevel + 1);
		else
			SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
#else
		SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST);
#endif
	}

	SetSkillNextReadTime(dwSkillVnum, nextTime);

#ifdef __FAKE_BUFF__
	if ((bFakeBuff && bLastLevel == GetFakeBuffSkillLevel(dwSkillVnum)) || (!bFakeBuff && bLastLevel == GetSkillLevel(dwSkillVnum)))
#else
	if (bLastLevel == GetSkillLevel(dwSkillVnum))
#endif
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "Å©À¹, ±â°¡ ¿ª·ùÇÏ°í ÀÖ¾î! ÀÌ°Å ¼³¸¶ ÁÖÈ­ÀÔ¸¶ÀÎ°¡!? Á¨Àå!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ö·ÃÀÌ ½ÇÆÐ·Î ³¡³µ½À´Ï´Ù. ´Ù½Ã µµÀüÇØÁÖ½Ã±â ¹Ù¶ø´Ï´Ù."));
		LogManager::instance().CharLog(this, dwSkillVnum, "GM_READ_FAIL", "");
		tchat("%s:%d", __FILE__, __LINE__);
		return false;
	}

	ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¸ö¿¡¼­ ¹º°¡ ÈûÀÌ ÅÍÁ® ³ª¿À´Â ±âºÐÀÌ¾ß!"));
	ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¶ß°Å¿î ¹«¾ùÀÌ °è¼Ó ¿ë¼ÚÀ½Ä¡°í ÀÖ¾î! ÀÌ°Ç, ÀÌ°ÍÀº!"));
	ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "´õ ³ôÀº °æÁöÀÇ ¼ö·ÃÀ» ¼º°øÀûÀ¸·Î ³¡³»¼Ì½À´Ï´Ù."));
	LogManager::instance().CharLog(this, dwSkillVnum, "GM_READ_SUCCESS", "");
	tchat("%s:%d", __FILE__, __LINE__);
	return true;
}
// END_OF_ADD_GRANDMASTER_SKILL

#ifdef __LEGENDARY_SKILL__
// ADD_LEGENDARY_SKILL
bool CHARACTER::LearnLegendarySkill(DWORD dwSkillVnum)
{
	CSkillProto * pkSk = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSk)
		return false;

	if (!IsLearnableSkill(dwSkillVnum))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ö·ÃÇÒ ¼ö ¾ø´Â ½ºÅ³ÀÔ´Ï´Ù."));
		return false;
	}

	sys_log(0, "learn legendary skill[%d] cur %d, next %d", dwSkillVnum, get_global_time(), GetSkillNextReadTime(dwSkillVnum));

	/*
	if (get_global_time() < GetSkillNextReadTime(dwSkillVnum))
	{
		if (!(test_server && quest::CQuestManager::instance().GetEventFlag("no_read_delay")))
		{
			if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
			{
				// ÁÖ¾È¼ú¼­ »ç¿ëÁß¿¡´Â ½Ã°£ Á¦ÇÑ ¹«½Ã
				RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÁÖ¾È¼ú¼­¸¦ ÅëÇØ ÁÖÈ­ÀÔ¸¶¿¡¼­ ºüÁ®³ª¿Ô½À´Ï´Ù."));
			}
			else
			{
				SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
				return false;
			}
		}
	}
	*/

	// bTypeÀÌ 0ÀÌ¸é Ã³À½ºÎÅÍ Ã¥À¸·Î ¼ö·Ã °¡´É
	if (pkSk->dwType == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "±×·£µå ¸¶½ºÅÍ ¼ö·ÃÀ» ÇÒ ¼ö ¾ø´Â ½ºÅ³ÀÔ´Ï´Ù."));
		return false;
	}

	if (GetSkillMasterType(dwSkillVnum) != SKILL_PERFECT_MASTER)
	{
		if (GetSkillMasterType(dwSkillVnum) > SKILL_PERFECT_MASTER)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÆÛÆåÆ® ¸¶½ºÅÍµÈ ½ºÅ³ÀÔ´Ï´Ù. ´õ ÀÌ»ó ¼ö·Ã ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Your Skill is not high enough for becoming a Legendary."));
		return false;
	}

	SkillLevelUp(dwSkillVnum, SKILL_UP_BY_QUEST2);

	LogManager::instance().CharLog(this, dwSkillVnum, "LEGENDARY_READ", "");
	return true;
}
// END_OF_ADD_GRANDMASTER_SKILL
#endif

static bool FN_should_check_exp(LPCHARACTER ch)
{
#ifdef __PRESTIGE__
	return ch->GetLevel() < gPrestigePlayerMaxLevel[ch->Prestige_GetLevel()];
#else
	return ch->GetLevel() < gPlayerMaxLevel;
#endif
}


bool CHARACTER::LearnSkillByBook(DWORD dwSkillVnum, BYTE bProb, BYTE bItemSubType)
{
	const CSkillProto* pkSk = CSkillManager::instance().Get(dwSkillVnum);

	if (!pkSk)
		return false;

#ifdef __FAKE_BUFF__
	bool bIsFakeUse = false;
#endif
	tchat("1");
	if (!IsLearnableSkill(dwSkillVnum))
	{
#ifdef __FAKE_BUFF__
		if (IsPC() && GetFakeBuffSkillIdx(dwSkillVnum) != FAKE_BUFF_SKILL_COUNT && quest::CQuestManager::instance().GetEventFlag("fakebuff_enabled") == 1)
		{
			if (!CanFakeBuffSkillUp(dwSkillVnum, true))
				return false;

			bIsFakeUse = true;
		}
		else
		{
#endif
			tchat("2");
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ö·ÃÇÒ ¼ö ¾ø´Â ½ºÅ³ÀÔ´Ï´Ù."));
			return false;
#ifdef __FAKE_BUFF__
		}
#endif
	}

	if (dwSkillVnum >= BOOST_SKILL_START && dwSkillVnum < BOOST_SKILL_END)
	{
		BYTE job = GetJob();
		BYTE group = GetSkillGroup();
		static const DWORD SkillList[JOB_MAX_NUM][SKILL_GROUP_MAX_NUM] =
		{
			{ SKILL_BOOST_SWORDSPIN,SKILL_BOOST_SPIRITSTRIKE },
			{ SKILL_BOOST_AMBUSH,SKILL_BOOST_FIREARROW },
			{ SKILL_BOOST_FINGERSTRIKE,SKILL_BOOST_DARKSTRIKE },
			{ SKILL_BOOST_SHOOTINGDRAGON,SKILL_BOOST_SUMMONLIGHTNING },
		};
		if (group == 0 || group > SKILL_GROUP_MAX_NUM || job >= JOB_MAX_NUM || SkillList[job][group - 1] != dwSkillVnum)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You are unable to learn this skill book."));
			return false;
		}
	}

	DWORD need_exp = 0;

	if (FN_should_check_exp(this))
	{
		need_exp = 20000;

		if ( GetExp() < need_exp && GetLevel() != 115)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "°æÇèÄ¡°¡ ºÎÁ·ÇÏ¿© Ã¥À» ÀÐÀ» ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	// bTypeÀÌ 0ÀÌ¸é Ã³À½ºÎÅÍ Ã¥À¸·Î ¼ö·Ã °¡´É
#ifdef __FAKE_BUFF__
	if (!bIsFakeUse)
#endif
	if (pkSk->dwType != 0)
	{
		if (GetSkillMasterType(dwSkillVnum) != SKILL_MASTER)
		{
#ifdef __FAKE_BUFF__
			if (IsPC() && GetFakeBuffSkillIdx(dwSkillVnum) != FAKE_BUFF_SKILL_COUNT && quest::CQuestManager::instance().GetEventFlag("fakebuff_enabled") == 1)
			{
				if (!CanFakeBuffSkillUp(dwSkillVnum, true))
					return false;

				bIsFakeUse = true;
			}
			else
#endif
			{
				if (GetSkillMasterType(dwSkillVnum) > SKILL_MASTER)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ½ºÅ³Àº Ã¥À¸·Î ´õÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÀÌ ½ºÅ³Àº ¾ÆÁ÷ Ã¥À¸·Î ¼ö·ÃÇÒ °æÁö¿¡ ÀÌ¸£Áö ¾Ê¾Ò½À´Ï´Ù."));
				return false;
			}
		}
	}
	bool isSpecial = false;
	if (dwSkillVnum >= WARD_SKILL_START && dwSkillVnum < BOOST_SKILL_END)
		isSpecial = true;

	if (isSpecial && GetSkillLevel(dwSkillVnum) >= 30)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "There is nothing new you can learn from this type of books."));
		return false;
	}

	if (get_global_time() < GetSkillNextReadTime(dwSkillVnum))
	{
		std::string evFlag = "no_read_delay";
		DWORD aff = AFFECT_SKILL_NO_BOOK_DELAY;
		if (isSpecial)
		{
			evFlag += "2";
			aff = AFFECT_SKILL_NO_BOOK_DELAY2;
		}

		if (!(test_server && quest::CQuestManager::instance().GetEventFlag(evFlag)))
		{
			if (FindAffect(aff))
			{
				// ÁÖ¾È¼ú¼­ »ç¿ëÁß¿¡´Â ½Ã°£ Á¦ÇÑ ¹«½Ã
				RemoveAffect(aff);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "ÁÖ¾È¼ú¼­¸¦ ÅëÇØ ÁÖÈ­ÀÔ¸¶¿¡¼­ ºüÁ®³ª¿Ô½À´Ï´Ù."));
			}
			else 		
			{
				SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
				return false;
			}
		}
	}

	// ¿©±â¼­ È®·üÀ» °è»êÇÕ´Ï´Ù.
#ifdef __FAKE_BUFF__
	unsigned char bLastLevel = bIsFakeUse ? GetFakeBuffSkillLevel(dwSkillVnum) : GetSkillLevel(dwSkillVnum);
#else
	unsigned char bLastLevel = GetSkillLevel(dwSkillVnum);
#endif
	bool bSuccess = false;

	if (bProb != 0 && dwSkillVnum != SKILL_LEADERSHIP && dwSkillVnum != SKILL_PASSIVE_RESIST_CRIT && dwSkillVnum != SKILL_PASSIVE_RESIST_PENE
#ifdef STANDARD_SKILL_DURATION
		&& dwSkillVnum != SKILL_PASSIVE_SKILL_DURATION
#endif
		)
	{
		// SKILL_BOOK_BONUS
		if (FindAffect(AFFECT_SKILL_BOOK_BONUS))
		{
			bProb += bProb / 2;
			RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
		}
		// END_OF_SKILL_BOOK_BONUS

		sys_log(0, "LearnSkillByBook Pct %u prob %d", dwSkillVnum, bProb);

		if (random_number(1, 100) <= bProb)
		{
			if (test_server)
				sys_log(0, "LearnSkillByBook %u SUCC", dwSkillVnum);

			bSuccess = true;
#ifdef __FAKE_BUFF__
			if (bIsFakeUse)
				SetFakeBuffSkillLevel(dwSkillVnum, bLastLevel + 1);
			else
				SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
#else
			SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
#endif
		}
		else
		{
			if (test_server)
				sys_log(0, "LearnSkillByBook %u FAIL", dwSkillVnum);
		}
	}
	else if (isSpecial)
	{
		if (GetSkillLevel(dwSkillVnum) < 20)
		{
			PointChange(POINT_EXP, -(int)need_exp);
			SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
			bSuccess = true;
		}
		else
		{
			int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 20);
			int need_bookcount = MAX(0, GetSkillLevel(dwSkillVnum) - 20);

			PointChange(POINT_EXP, -(int)need_exp);

			quest::CQuestManager& q = quest::CQuestManager::instance();
			quest::PC* pPC = q.GetPC(GetPlayerID());

			if (pPC)
			{
				char flag[128 + 1];
				memset(flag, 0, sizeof(flag));
				snprintf(flag, sizeof(flag), "traning_master_skill.%u.read_count", dwSkillVnum);

				int read_count = pPC->GetFlag(flag);
				int percent = 65;

				if (FindAffect(AFFECT_SKILL_BOOK_BONUS2))
				{
					percent = 0;
					RemoveAffect(AFFECT_SKILL_BOOK_BONUS2);
				}

				if (random_number(1, 100) > percent)
				{
					bSuccess = true;

					// Ã¥ÀÐ±â¿¡ ¼º°ø
					if (read_count >= need_bookcount)
					{
						SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
						pPC->SetFlag(flag, 0);

						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¥À¸·Î ´õ ³ôÀº °æÁöÀÇ ¼ö·ÃÀ» ¼º°øÀûÀ¸·Î ³¡³»¼Ì½À´Ï´Ù."));
						LogManager::instance().CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
						return true;
					}
					else
					{
						pPC->SetFlag(flag, read_count + 1);

						switch (random_number(1, 3))
						{
						case 1:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¾î´ÀÁ¤µµ ÀÌ ±â¼ú¿¡ ´ëÇØ ÀÌÇØ°¡ µÇ¾úÁö¸¸ Á¶±Ý ºÎÁ·ÇÑµí ÇÑµ¥.."));
							break;

						case 2:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "µåµð¾î ³¡ÀÌ º¸ÀÌ´Â °Ç°¡...  ÀÌ ±â¼úÀº ÀÌÇØÇÏ±â°¡ ³Ê¹« Èûµé¾î.."));
							break;

						case 3:
						default:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­½ÉÈ÷ ÇÏ´Â ¹è¿òÀ» °¡Áö´Â °Í¸¸ÀÌ ±â¼úÀ» ¹è¿ï¼ö ÀÖ´Â À¯ÀÏÇÑ ±æÀÌ´Ù.."));
							break;
						}

						// ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¥À¸·Î ´õ ³ôÀº °æÁöÀÇ ¼ö·ÃÀ» ¼º°øÀûÀ¸·Î ³¡³»¼Ì½À´Ï´Ù."));
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The skillbook reading was successfull, you need %d more."), need_bookcount - read_count);
						tchat("success: %d need_bookcount: %d read_count: %d", bSuccess, need_bookcount, read_count);
						return true;
					}
				}
			}
		}
	}
	else
	{
		int idx = MIN(9, GetSkillLevel(dwSkillVnum) - 20);

#ifdef __FAKE_BUFF__
		sys_log(0, "LearnSkillByBook %s isFakeBuff %d", GetName(), bIsFakeUse);
#else
		sys_log(0, "LearnSkillByBook %s", GetName());
#endif

#ifdef __FAKE_BUFF__
		int need_bookcount = MAX(0, GetSkillLevel(dwSkillVnum) - 20);
		if (bIsFakeUse)
			need_bookcount = MAX(0, GetFakeBuffSkillLevel(dwSkillVnum) - 20);
#else
		int need_bookcount = MAX(0, GetSkillLevel(dwSkillVnum) - 20);
#endif

		PointChange(POINT_EXP, -(int)need_exp);

		quest::CQuestManager& q = quest::CQuestManager::instance();
		quest::PC* pPC = q.GetPC(GetPlayerID());

		if (pPC)
		{
			char flag[128+1];
			memset(flag, 0, sizeof(flag));
			snprintf(flag, sizeof(flag), "traning_master_skill.%u.read_count", dwSkillVnum);

			int read_count = pPC->GetFlag(flag);
			int percent = 65;
			// if (bItemSubType == SKILLBOOK_MASTER)
				// percent = 65;

			if (dwSkillVnum == SKILL_LEADERSHIP || dwSkillVnum == SKILL_PASSIVE_RESIST_CRIT || dwSkillVnum == SKILL_PASSIVE_RESIST_PENE
#ifdef STANDARD_SKILL_DURATION
				|| dwSkillVnum == SKILL_PASSIVE_SKILL_DURATION
#endif
				)
				percent = 100 - bProb;

			if (percent > 0 && FindAffect(AFFECT_SKILL_BOOK_BONUS))
			{
				percent = 0;
				RemoveAffect(AFFECT_SKILL_BOOK_BONUS);
			}

			if (random_number(1, 100) > percent)
			{
				bSuccess = true;

				// Ã¥ÀÐ±â¿¡ ¼º°ø
				if (read_count >= need_bookcount)
				{
#ifdef __FAKE_BUFF__
					if (bIsFakeUse)
						SetFakeBuffSkillLevel(dwSkillVnum, bLastLevel + 1);
					else
						SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
#else
					SkillLevelUp(dwSkillVnum, SKILL_UP_BY_BOOK);
#endif
					pPC->SetFlag(flag, 0);

					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¥À¸·Î ´õ ³ôÀº °æÁöÀÇ ¼ö·ÃÀ» ¼º°øÀûÀ¸·Î ³¡³»¼Ì½À´Ï´Ù."));
					LogManager::instance().CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
					return true;
				}
				else
				{
					pPC->SetFlag(flag, read_count + 1);

					switch (random_number(1, 3))
					{
						case 1:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¾î´ÀÁ¤µµ ÀÌ ±â¼ú¿¡ ´ëÇØ ÀÌÇØ°¡ µÇ¾úÁö¸¸ Á¶±Ý ºÎÁ·ÇÑµí ÇÑµ¥.."));
							break;
											
						case 2:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "µåµð¾î ³¡ÀÌ º¸ÀÌ´Â °Ç°¡...  ÀÌ ±â¼úÀº ÀÌÇØÇÏ±â°¡ ³Ê¹« Èûµé¾î.."));
							break;

						case 3:
						default:
							ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¿­½ÉÈ÷ ÇÏ´Â ¹è¿òÀ» °¡Áö´Â °Í¸¸ÀÌ ±â¼úÀ» ¹è¿ï¼ö ÀÖ´Â À¯ÀÏÇÑ ±æÀÌ´Ù.."));
							break;
					}

					// ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¥À¸·Î ´õ ³ôÀº °æÁöÀÇ ¼ö·ÃÀ» ¼º°øÀûÀ¸·Î ³¡³»¼Ì½À´Ï´Ù."));
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "The skillbook reading was successfull, you need %d more."), need_bookcount - read_count);
					tchat("success: %d need_bookcount: %d read_count: %d", bSuccess, need_bookcount, read_count);
					return true;
				}
			}
		}
	}

	if (bSuccess)
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¸ö¿¡¼­ ¹º°¡ ÈûÀÌ ÅÍÁ® ³ª¿À´Â ±âºÐÀÌ¾ß!"));
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "¶ß°Å¿î ¹«¾ùÀÌ °è¼Ó ¿ë¼ÚÀ½Ä¡°í ÀÖ¾î! ÀÌ°Ç, ÀÌ°ÍÀº!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "Ã¥À¸·Î ´õ ³ôÀº °æÁöÀÇ ¼ö·ÃÀ» ¼º°øÀûÀ¸·Î ³¡³»¼Ì½À´Ï´Ù."));
		LogManager::instance().CharLog(this, dwSkillVnum, "READ_SUCCESS", "");
		tchat("%s:%d", __FILE__, __LINE__);
	}
	else
	{
		ChatPacket(CHAT_TYPE_TALKING, LC_TEXT(this, "Å©À¹, ±â°¡ ¿ª·ùÇÏ°í ÀÖ¾î! ÀÌ°Å ¼³¸¶ ÁÖÈ­ÀÔ¸¶ÀÎ°¡!? Á¨Àå!"));
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¼ö·ÃÀÌ ½ÇÆÐ·Î ³¡³µ½À´Ï´Ù. ´Ù½Ã µµÀüÇØÁÖ½Ã±â ¹Ù¶ø´Ï´Ù."));
		LogManager::instance().CharLog(this, dwSkillVnum, "READ_FAIL", "");
		tchat("%s:%d", __FILE__, __LINE__);
	}

	return true;
}

bool CHARACTER::SkillLevelDown(DWORD dwVnum)
{
	if (NULL == m_pSkillLevels)
		return false;


	if (IsPolymorphed())
		return false;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such skill by number %u", dwVnum);
		return false;
	}

	if (!IsLearnableSkill(dwVnum))
		return false;

	if (GetSkillMasterType(pkSk->dwVnum) != SKILL_NORMAL)
		return false;

	if (!GetSkillGroup())
		return false;

	if (pkSk->dwVnum >= SKILL_MAX_NUM)
		return false;

	if (m_pSkillLevels[pkSk->dwVnum].level() == 0)
		return false;

	int idx = POINT_SKILL;
	switch (pkSk->dwType)
	{
		case 0:
			idx = POINT_SUB_SKILL;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 6:
#ifdef __WOLFMAN__
		case 7:
#endif
			idx = POINT_SKILL;
			break;
		case 5:
			idx = POINT_HORSE_SKILL;
			break;
		default:
			sys_err("Wrong skill type %d skill vnum %d", pkSk->dwType, pkSk->dwVnum);
			return false;

	}

	PointChange(idx, +1);
	SetSkillLevel(pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].level() - 1);

	sys_log(0, "SkillDown: %s %u %u %u type %u",
		GetName(), pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].master_type(), m_pSkillLevels[pkSk->dwVnum].level(), pkSk->dwType);
	Save();

	ComputePoints();
	SkillLevelPacket();
	return true;
}

void CHARACTER::SkillLevelUp(DWORD dwVnum, BYTE bMethod)
{
	if (NULL == m_pSkillLevels)
		return;

	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (SKILL_7_A_ANTI_TANHWAN <= dwVnum && dwVnum <= SKILL_8_D_ANTI_BYEURAK)
	{
		if (0 == GetSkillLevel(dwVnum))
			return;
	}

	const CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such skill by number (vnum %u)", dwVnum);
		return;
	}

	if (pkSk->dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("Skill Vnum overflow (vnum %u)", dwVnum);
		return;
	}

	if (!IsLearnableSkill(dwVnum))
		return;

#ifdef COMBAT_ZONE// avoid player skillups -> unsaved
	if (CCombatZoneManager::Instance().IsCombatZoneMap(GetMapIndex()) && GetSkillLevel(pkSk->dwVnum) < SKILL_MAX_LEVEL)
		return;
#endif

	// ±×·£µå ¸¶½ºÅÍ´Â Äù½ºÆ®·Î¸¸ ¼öÇà°¡´É
	if (pkSk->dwType != 0)
	{
		switch (GetSkillMasterType(pkSk->dwVnum))
		{
			case SKILL_GRAND_MASTER:
				if (bMethod != SKILL_UP_BY_QUEST)
					return;
				break;

			case SKILL_PERFECT_MASTER:
#ifdef __LEGENDARY_SKILL__
				if (bMethod != SKILL_UP_BY_QUEST2)
					return;
				break;

			case SKILL_LEGENDARY_MASTER:
#endif
				return;
		}
	}

	if (bMethod == SKILL_UP_BY_POINT)
	{
		// ¸¶½ºÅÍ°¡ ¾Æ´Ñ »óÅÂ¿¡¼­¸¸ ¼ö·Ã°¡´É
		if (GetSkillMasterType(pkSk->dwVnum) != SKILL_NORMAL)
			return;

		if (IS_SET(pkSk->dwFlag, SKILL_FLAG_DISABLE_BY_POINT_UP))
			return;
	}
	else if (bMethod == SKILL_UP_BY_BOOK)
	{
		if (pkSk->dwType != 0) // Á÷¾÷¿¡ ¼ÓÇÏÁö ¾Ê¾Ò°Å³ª Æ÷ÀÎÆ®·Î ¿Ã¸±¼ö ¾ø´Â ½ºÅ³Àº Ã³À½ºÎÅÍ Ã¥À¸·Î ¹è¿ï ¼ö ÀÖ´Ù.
			if (GetSkillMasterType(pkSk->dwVnum) != SKILL_MASTER)
				return;
	}

	if (GetLevel() < pkSk->bLevelLimit)
		return;

	if (pkSk->preSkillVnum)
		if (GetSkillMasterType(pkSk->preSkillVnum) == SKILL_NORMAL &&
			GetSkillLevel(pkSk->preSkillVnum) < pkSk->preSkillLevel)
			return;

	if (!GetSkillGroup())
		return;

	if (bMethod == SKILL_UP_BY_POINT)
	{
		int idx;

		switch (pkSk->dwType)
		{
			case 0:
				idx = POINT_SUB_SKILL;
				break;

			case 1:
			case 2:
			case 3:
			case 4:
			case 6:
#ifdef __WOLFMAN__
			case 7:
#endif
				idx = POINT_SKILL;
				break;

			case 5:
				idx = POINT_HORSE_SKILL;
				break;

			default:
				sys_err("Wrong skill type %d skill vnum %d", pkSk->dwType, pkSk->dwVnum);
				return;
		}

		if (dwVnum >= WARD_SKILL_START && dwVnum < WARD_SKILL_END)
			idx = POINT_SKILL;

		// fix
		if (dwVnum >= BOOST_SKILL_START && dwVnum < BOOST_SKILL_END)
			idx = POINT_SKILL;

		if (GetPoint(idx) < 1)
			return;

		PointChange(idx, -1);
	}

	if (pkSk->dwType == 0)
		ComputePassiveSkillPoints(false);

	int SkillPointBefore = GetSkillLevel(pkSk->dwVnum);
	SetSkillLevel(pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].level() + 1);

	if (pkSk->dwType != 0)
	{
		// °©ÀÚ±â ±×·¹ÀÌµå ¾÷ÇÏ´Â ÄÚµù
		switch (GetSkillMasterType(pkSk->dwVnum))
		{
			case SKILL_NORMAL:
				// ¹ø¼·Àº ½ºÅ³ ¾÷±×·¹ÀÌµå 17~20 »çÀÌ ·£´ý ¸¶½ºÅÍ ¼ö·Ã
				if (GetSkillLevel(pkSk->dwVnum) >= 17)
				{
#ifndef __SKILLUP_ON_17__
					if (random_number(1, 21 - MIN(20, GetSkillLevel(pkSk->dwVnum))) == 1)
#endif
						SetSkillLevel(pkSk->dwVnum, 20);
				}
				break;

			case SKILL_MASTER:
				if (GetSkillLevel(pkSk->dwVnum) >= 30)
				{
					if (random_number(1, 31 - MIN(30, GetSkillLevel(pkSk->dwVnum))) == 1)
						SetSkillLevel(pkSk->dwVnum, 30);
				}
				break;

			case SKILL_GRAND_MASTER:
				if (GetSkillLevel(pkSk->dwVnum) >= 40)
				{
					SetSkillLevel(pkSk->dwVnum, 40);
				}
				break;

#ifdef __LEGENDARY_SKILL__
			case SKILL_PERFECT_MASTER:
				if (GetSkillLevel(pkSk->dwVnum) > 40)
				{
					SetSkillLevel(pkSk->dwVnum, 50);
				}
				break;
#endif
		}
	}

	if (pkSk->dwType == 0)
		ComputePassiveSkillPoints();

	char szSkillUp[1024];

	snprintf(szSkillUp, sizeof(szSkillUp), "SkillUp: %s %u %d %d[Before:%d] type %u",
			GetName(), pkSk->dwVnum, m_pSkillLevels[pkSk->dwVnum].master_type(), m_pSkillLevels[pkSk->dwVnum].level(), SkillPointBefore, pkSk->dwType);

	sys_log(0, "%s", szSkillUp);

	LogManager::instance().CharLog(this, pkSk->dwVnum, "SKILLUP", szSkillUp);
	Save();

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputeSkillPoints()
{
}

void CHARACTER::ResetSkill()
{
	if (NULL == m_pSkillLevels)
		return;

	// º¸Á¶ ½ºÅ³Àº ¸®¼Â½ÃÅ°Áö ¾Ê´Â´Ù
	std::vector<std::pair<DWORD, TPlayerSkill> > vec;
	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		vec.push_back(std::make_pair(s_adwSubSkillVnums[i], m_pSkillLevels[s_adwSubSkillVnums[i]]));
	}

	for (int i = 0; i < SKILL_MAX_NUM; ++i)
		m_pSkillLevels[i].Clear();

	std::vector<std::pair<DWORD, TPlayerSkill> >::const_iterator iter = vec.begin();

	while (iter != vec.end())
	{
		const std::pair<DWORD, TPlayerSkill>& pair = *(iter++);
		m_pSkillLevels[pair.first] = pair.second;
	}

	SetSkillLevelChanged();

	ComputePoints();
	SkillLevelPacket();
}

void CHARACTER::ComputePassiveSkill(DWORD dwVnum)
{
	if (GetSkillLevel(dwVnum) == 0)
		return;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);
	pkSk->SetVar("k", GetSkillLevel(dwVnum));
	int iAmount = (int) pkSk->kPointPoly.Evaluate();

	sys_log(2, "%s passive #%d on %d amount %d", GetName(), dwVnum, pkSk->bPointOn, iAmount);
	PointChange(pkSk->bPointOn, iAmount);
}

struct FFindNearVictim
{
	FFindNearVictim(LPCHARACTER center, LPCHARACTER attacker, const CHARACTER_SET& excepts_set = empty_set_)
		: m_pkChrCenter(center),
	m_pkChrNextTarget(NULL),
	m_pkChrAttacker(attacker),
	m_count(0),
	m_excepts_set(excepts_set)
	{
	}

	void operator ()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		if (!m_excepts_set.empty()) {
			if (m_excepts_set.find(pkChr) != m_excepts_set.end())
				return;
		}

		if (m_pkChrCenter == pkChr)
			return;

		if (!battle_is_attackable(m_pkChrAttacker, pkChr))
		{
			return;
		}

		if (abs(m_pkChrCenter->GetX() - pkChr->GetX()) > 1000 || abs(m_pkChrCenter->GetY() - pkChr->GetY()) > 1000)
			return;

		float fDist = DISTANCE_APPROX(m_pkChrCenter->GetX() - pkChr->GetX(), m_pkChrCenter->GetY() - pkChr->GetY());

		if (fDist < 1000)
		{
			++m_count;

			if ((m_count == 1) || random_number(1, m_count) == 1)
				m_pkChrNextTarget = pkChr;
		}
	}

	LPCHARACTER GetVictim()
	{
		return m_pkChrNextTarget;
	}

	LPCHARACTER m_pkChrCenter;
	LPCHARACTER m_pkChrNextTarget;
	LPCHARACTER m_pkChrAttacker;
	int		m_count;
	const CHARACTER_SET & m_excepts_set;
private:
	static CHARACTER_SET empty_set_;
};

CHARACTER_SET FFindNearVictim::empty_set_;

EVENTINFO(chain_lightning_event_info)
{
	DWORD			dwVictim;
	DWORD			dwChr;

	chain_lightning_event_info()
	: dwVictim(0)
	, dwChr(0)
	{
	}
};

EVENTFUNC(ChainLightningEvent)
{
	chain_lightning_event_info * info = dynamic_cast<chain_lightning_event_info *>( event->info );

	LPCHARACTER pkChrVictim = CHARACTER_MANAGER::instance().Find(info->dwVictim);
	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().Find(info->dwChr);
	LPCHARACTER pkTarget = NULL;

	if (!pkChr || !pkChrVictim)
	{
		sys_log(1, "use chainlighting, but no character");
		return 0;
	}

	sys_log(1, "chainlighting event %s", pkChr->GetName());

	if (pkChrVictim->GetParty()) // ÆÄÆ¼ ¸ÕÀú
	{
		pkTarget = pkChrVictim->GetParty()->GetNextOwnership(NULL, pkChrVictim->GetX(), pkChrVictim->GetY());
		if (pkTarget == pkChrVictim || !random_number(0, 2) || pkChr->GetChainLightingExcept().find(pkTarget) != pkChr->GetChainLightingExcept().end())
			pkTarget = NULL;
	}

	if (!pkTarget)
	{
		// 1. Find Next victim
		FFindNearVictim f(pkChrVictim, pkChr, pkChr->GetChainLightingExcept());

		if (pkChrVictim->GetSectree())
		{
			pkChrVictim->GetSectree()->ForEachAround(f);
			// 2. If exist, compute it again
			pkTarget = f.GetVictim();
		}
	}

	if (pkTarget)
	{
		pkChrVictim->CreateFly(FLY_CHAIN_LIGHTNING, pkTarget);
		pkChr->ComputeSkill(SKILL_CHAIN, pkTarget);
		pkChr->AddChainLightningExcept(pkTarget);
	}
	else
	{
		sys_log(1, "%s use chainlighting, but find victim failed near %s", pkChr->GetName(), pkChrVictim->GetName());
	}

	return 0;
}

#ifdef __ACCE_COSTUME__
void SetPolyVarForAttack(LPCHARACTER ch, CSkillProto * pkSk, LPITEM pkWeapon, LPITEM pAcce)
#else
void SetPolyVarForAttack(LPCHARACTER ch, CSkillProto * pkSk, LPITEM pkWeapon)
#endif
{
	if (ch->IsPC())
	{
		if (pkWeapon && pkWeapon->GetType() == ITEM_WEAPON)
		{
			int iDamMin, iDamMax;
			Item_GetDamage(pkWeapon, &iDamMin, &iDamMax);

			int iMagicDamMin, iMagicDamMax;
			Item_GetMagicDamage(pkWeapon, &iMagicDamMin, &iMagicDamMax);

			int iWep = random_number(iDamMin, iDamMax);
			iWep += pkWeapon->GetValue(5);

			int iMtk = random_number(iMagicDamMin, iMagicDamMax);
			iMtk += pkWeapon->GetValue(5);

#ifdef __ACCE_COSTUME__
			if (pAcce && pAcce->GetType() == ITEM_COSTUME && pAcce->GetSubType() == COSTUME_ACCE)
			{
				auto p = ITEM_MANAGER::instance().GetTable(pAcce->GetSocket(1));

				if (p)
				{
					long drainPct = pAcce->GetSocket(0);
					if (drainPct != 0)
					{
#ifdef __ALPHA_EQUIP__
						iWep += (int)(float(random_number(p->values(3) + pAcce->GetAlphaEquipValue(), p->values(4) + pAcce->GetAlphaEquipValue())) / 100.0f * float(drainPct));
						iMtk += (int)(float(random_number(p->values(1) + pAcce->GetAlphaEquipValue(), p->values(2) + pAcce->GetAlphaEquipValue())) / 100.0f * float(drainPct));
#else
						iWep += (int)(float(random_number(p->values(3), p->values(4))) / 100.0f * float(drainPct));
						iMtk += (int)(float(random_number(p->values(1), p->values(2))) / 100.0f * float(drainPct));
#endif
						iWep += (int)(float(p->values(5)) / 100.0f * float(drainPct));
						iMtk += (int)(float(p->values(5)) / 100.0f * float(drainPct));
					}
				}
			}
#endif

			pkSk->SetVar("wep", iWep);
			pkSk->SetVar("mtk", iMtk);
			pkSk->SetVar("mwep", iMtk);
		}
		else
		{
			pkSk->SetVar("wep", 0);
			pkSk->SetVar("mtk", 0);
			pkSk->SetVar("mwep", 0);
		}
	}
	else
	{
		int iWep = random_number(ch->GetMobDamageMin(), ch->GetMobDamageMax());
		pkSk->SetVar("wep", iWep);
		pkSk->SetVar("mwep", iWep);
		pkSk->SetVar("mtk", iWep);
	}
}

#ifdef IGNORE_KNOCKBACK
static inline bool ignoreKnockBack(DWORD vnum)
{
	switch (vnum)
	{
	case 9471:
	case 9469:
	case 9470:
	case 9472:
	case 35101:
	case 35102:
		return true;
	}

	return false;
}
#endif

struct FuncSplashDamage
{
#ifdef __ACCE_COSTUME__
	FuncSplashDamage(int x, int y, CSkillProto * pkSk, LPCHARACTER pkChr, int iAmount, int iAG, int iMaxHit, LPITEM pkWeapon, LPITEM pAcce, bool bDisableCooltime, TSkillUseInfo* pInfo, BYTE bUseSkillPower)
		:
		m_x(x), m_y(y), m_pkSk(pkSk), m_pkChr(pkChr), m_iAmount(iAmount), m_iAG(iAG), m_iCount(0), m_iMaxHit(iMaxHit), m_pkWeapon(pkWeapon), m_pAcce(pAcce), m_bDisableCooltime(bDisableCooltime), m_pInfo(pInfo), m_bUseSkillPower(bUseSkillPower)
	{
	}
#else
	FuncSplashDamage(int x, int y, CSkillProto * pkSk, LPCHARACTER pkChr, int iAmount, int iAG, int iMaxHit, LPITEM pkWeapon, bool bDisableCooltime, TSkillUseInfo* pInfo, BYTE bUseSkillPower)
		:
		m_x(x), m_y(y), m_pkSk(pkSk), m_pkChr(pkChr), m_iAmount(iAmount), m_iAG(iAG), m_iCount(0), m_iMaxHit(iMaxHit), m_pkWeapon(pkWeapon), m_bDisableCooltime(bDisableCooltime), m_pInfo(pInfo), m_bUseSkillPower(bUseSkillPower)
		{
		}
#endif

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
		{
			//if (m_pkSk->dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN target not character %s", m_pkChr->GetName());
			return;
		}

		LPCHARACTER pkChrVictim = (LPCHARACTER) ent;

		if (!pkChrVictim->IsPC() && (pkChrVictim->GetRaceNum() == BOSSHUNT_BOSS_VNUM
#ifdef BOSSHUNT_EVENT_UPDATE
			|| pkChrVictim->GetRaceNum() == BOSSHUNT_BOSS_VNUM2
#endif
			))
			return;

		if (DISTANCE_APPROX(m_x - pkChrVictim->GetX(), m_y - pkChrVictim->GetY()) > m_pkSk->iSplashRange)
		{
			if(test_server)
				sys_log(0, "XXX target too far %s -> %u %s", m_pkChr->GetName(), (DWORD)pkChrVictim->GetVID(), pkChrVictim->GetName());
			return;
		}

#ifdef __BLOCK_DISPEL_ON_MOBS__
		if (!pkChrVictim->IsPC() && m_pkSk->dwVnum == SKILL_PABEOB)
			return;
#endif

		if (pkChrVictim->IsMount() || pkChrVictim->IsPet())
			return;

#ifdef __FAKE_PC__
		if (m_pkChr->FakePC_Check() && !IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SPLASH))
		{
			const float fMaxRotationDif = 30.0f;

			float fRealRotation = m_pkChr->GetRotation();
			float fHitRotation = GetDegreeFromPositionXY(m_pkChr->GetX(), m_pkChr->GetY(), pkChrVictim->GetX(), pkChrVictim->GetY());

			if (fRealRotation > 180.0f)
				fRealRotation = 360.0f - fRealRotation;
			if (fHitRotation > 180.0f)
				fHitRotation = 360.0f - fHitRotation;

			float fDif = abs(fRealRotation - fHitRotation);
			if (fDif > fMaxRotationDif)
			{
				if (test_server)
					sys_log(0, "XXX target not in radius %s", m_pkChr->GetName());
				return;
			}
		}
#endif

		if (!battle_is_attackable(m_pkChr, pkChrVictim))
		{
			if(test_server)
				sys_log(0, "XXX target not attackable %s -> %u %s", m_pkChr->GetName(), (DWORD)pkChrVictim->GetVID(), pkChrVictim->GetName());
			return;
		}

		if (test_server)
			sys_log(0, "XXX Attack target %s -> %u %s", m_pkChr->GetName(), (DWORD)pkChrVictim->GetVID(), pkChrVictim->GetName());

		if (m_pkChr->IsPC())
			// ±æµå ½ºÅ³Àº ÄðÅ¸ÀÓ Ã³¸®¸¦ ÇÏÁö ¾Ê´Â´Ù.
			if (!(m_pkSk->dwVnum >= GUILD_SKILL_START && m_pkSk->dwVnum <= GUILD_SKILL_END))
				if (!m_bDisableCooltime && m_pInfo && !m_pInfo->HitOnce(m_pkSk->dwVnum) && m_pkSk->dwVnum != SKILL_MUYEONG)
				{
					if(test_server)
						sys_log(0, "check guild skill %s", m_pkChr->GetName());
					return;
				}

		++m_iCount;

		int iDam;

		////////////////////////////////////////////////////////////////////////////////
		//float k = 1.0f * m_pkChr->GetSkillPower(m_pkSk->dwVnum) * m_pkSk->bMaxLevel / 100;
		//m_pkSk->kPointPoly2.SetVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
		m_pkSk->SetVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
		m_pkSk->SetVar("lv", m_pkChr->GetLevel());
		m_pkSk->SetVar("iq", m_pkChr->GetPoint(POINT_IQ));
		m_pkSk->SetVar("str", m_pkChr->GetPoint(POINT_ST));
		m_pkSk->SetVar("dex", m_pkChr->GetPoint(POINT_DX));
		m_pkSk->SetVar("con", m_pkChr->GetPoint(POINT_HT));
		m_pkSk->SetVar("def", m_pkChr->GetPoint(POINT_DEF_GRADE));
		m_pkSk->SetVar("odef", m_pkChr->GetPoint(POINT_DEF_GRADE) - m_pkChr->GetPoint(POINT_DEF_GRADE_BONUS));
		m_pkSk->SetVar("horse_level", m_pkChr->GetHorseGrade() * 10);
		m_pkSk->SetVar("sklv", m_pkChr->GetSkillLevel(m_pkSk->dwVnum));
		m_pkSk->SetVar("sklv", m_pkChr->GetSkillLevel(m_pkSk->dwVnum));
#ifdef __LEGENDARY_SKILL__
		m_pkSk->SetVar("isLegendary", m_pkChr->GetSkillMasterType(m_pkSk->dwVnum) == SKILL_LEGENDARY_MASTER ? 1 : 0);
#endif

		//int iPenetratePct = (int)(1 + k*4);
		bool bIgnoreDefense = false;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_PENETRATE))
		{
			int iPenetratePct = (int) m_pkSk->kPointPoly2.Evaluate();

			if (random_number(1, 100) <= iPenetratePct)
				bIgnoreDefense = true;
		}

		bool bIgnoreTargetRating = false;

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_IGNORE_TARGET_RATING))
		{
			int iPct = (int) m_pkSk->kPointPoly2.Evaluate();

			if (random_number(1, 100) <= iPct)
				bIgnoreTargetRating = true;
		}

		m_pkSk->SetVar("ar", CalcAttackRating(m_pkChr, pkChrVictim, bIgnoreTargetRating));

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
			m_pkSk->SetVar("atk", CalcMeleeDamage(m_pkChr, pkChrVictim, true, bIgnoreTargetRating));
		else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
		{
			LPITEM pkBow, pkArrow;

			if (1 == m_pkChr->GetArrowAndBow(&pkBow, &pkArrow, 1))
				m_pkSk->SetVar("atk", CalcArrowDamage(m_pkChr, pkChrVictim, pkBow, pkArrow, true));
			else
				m_pkSk->SetVar("atk", 0);
		}

		if (m_pkSk->bPointOn == POINT_MOV_SPEED)
			m_pkSk->SetVar("maxv", pkChrVictim->GetLimitPoint(POINT_MOV_SPEED));

		m_pkSk->SetVar("maxhp", pkChrVictim->GetMaxHP());
		m_pkSk->SetVar("maxsp", pkChrVictim->GetMaxSP());

		m_pkSk->SetVar("chain", m_pkChr->GetChainLightningIndex());
		m_pkChr->IncChainLightningIndex();

		bool bUnderEunhyung = m_pkChr->GetAffectedEunhyung() > 0; // ÀÌ°Ç ¿Ö ¿©±â¼­ ÇÏÁö??

		m_pkSk->SetVar("ek", m_pkChr->GetAffectedEunhyung()*1./100);
		//m_pkChr->ClearAffectedEunhyung();
#ifdef __ACCE_COSTUME__
		SetPolyVarForAttack(m_pkChr, m_pkSk, m_pkWeapon, m_pAcce);
#else
		SetPolyVarForAttack(m_pkChr, m_pkSk, m_pkWeapon);
#endif

		int iAmount = 0;

		if (m_pkChr->GetUsedSkillMasterType(m_pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			iAmount = (int) m_pkSk->kMasterBonusPoly.Evaluate();
		}

		if (iAmount == 0)
		{
			iAmount = (int) m_pkSk->kPointPoly.Evaluate();
		}

		if (test_server && iAmount == 0 && m_pkSk->bPointOn != POINT_NONE)
		{
			m_pkChr->ChatPacket(CHAT_TYPE_INFO, "È¿°ú°¡ ¾ø½À´Ï´Ù. ½ºÅ³ °ø½ÄÀ» È®ÀÎÇÏ¼¼¿ä");
		}
		////////////////////////////////////////////////////////////////////////////////
		iAmount = -iAmount;

		if (m_pkSk->dwVnum == SKILL_AMSEOP)
		{
			float fDelta = GetDegreeDelta(m_pkChr->GetRotation(), pkChrVictim->GetRotation());
			float adjust;

			if (fDelta < 35.0f)
			{
				adjust = 1.5f;

				if (bUnderEunhyung)
					adjust += 0.5f;

				if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
				{
					adjust += 0.5f;
				}
			}
			else
			{
				adjust = 1.0f;

				if (bUnderEunhyung)
					adjust += 0.5f;

				if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
					adjust += 0.5f;
			}

			iAmount = (int) (iAmount * adjust);
		}
		else if (m_pkSk->dwVnum == SKILL_GUNGSIN)
		{
			float adjust = 1.0;

			if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			{
				adjust = 1.35f;
			}

			iAmount = (int) (iAmount * adjust);
		}
#ifdef __WOLFMAN__
		else if (m_pkSk->dwVnum == SKILL_GONGDAB)
		{
			float adjust = 1.0;

			if (m_pkChr->GetWear(WEAR_WEAPON) && m_pkChr->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_CLAW)
			{
				adjust = 1.35f;
			}

			iAmount = (int)(iAmount * adjust);
		}
#endif
		////////////////////////////////////////////////////////////////////////////////
		//sys_log(0, "name: %s skill: %s amount %d to %s", m_pkChr->GetName(), m_pkSk->szName, iAmount, pkChrVictim->GetName());

		iDam = CalcBattleDamage(iAmount, m_pkChr->GetLevel(), pkChrVictim->GetLevel());

		if (m_pkChr->IsPC() && m_pkChr->m_SkillUseInfo[m_pkSk->dwVnum].GetMainTargetVID() != (DWORD) pkChrVictim->GetVID())
		{
			// µ¥¹ÌÁö °¨¼Ò
			iDam = (int) (iDam * m_pkSk->kSplashAroundDamageAdjustPoly.Evaluate());
		}

		// TODO ½ºÅ³¿¡ µû¸¥ µ¥¹ÌÁö Å¸ÀÔ ±â·ÏÇØ¾ßÇÑ´Ù.
		EDamageType dt = DAMAGE_TYPE_NONE;

		switch (m_pkSk->bSkillAttrType)
		{
			case SKILL_ATTR_TYPE_NORMAL:
				break;

			case SKILL_ATTR_TYPE_MELEE:
				{
					dt = DAMAGE_TYPE_MELEE;

					LPITEM pkWeapon = m_pkChr->GetWear(WEAR_WEAPON);

					if (pkWeapon)
						switch (pkWeapon->GetSubType())
						{
							case WEAPON_SWORD:
								iDam = iDam * (100 - MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_SWORD) - m_pkChr->GetPoint(POINT_RESIST_SWORD_PEN), 100)) / 100;
								break;

							case WEAPON_TWO_HANDED:
								iDam = iDam * (100 - MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_TWOHAND) - m_pkChr->GetPoint(POINT_RESIST_TWOHAND_PEN), 100)) / 100;
								// ¾ç¼Õ°Ë Æä³ÎÆ¼ 10%
								//iDam = iDam * 95 / 100;

								break;

							case WEAPON_DAGGER:
								iDam = iDam * (100 - MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_DAGGER) - m_pkChr->GetPoint(POINT_RESIST_DAGGER_PEN), 100)) / 100;
								break;

							case WEAPON_BELL:
								iDam = iDam * (100 - MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_BELL) - m_pkChr->GetPoint(POINT_RESIST_BELL_PEN), 100)) / 100;
								break;

							case WEAPON_FAN:
								iDam = iDam * (100 - MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_FAN) - m_pkChr->GetPoint(POINT_RESIST_FAN_PEN), 100)) / 100;
								break;

#ifdef __WOLFMAN__
							case WEAPON_CLAW:
								iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_CLAW)) / 100;
								break;
#endif
						}

					if (!bIgnoreDefense)
						iDam -= pkChrVictim->GetPoint(POINT_DEF_GRADE);
				}
				break;

			case SKILL_ATTR_TYPE_RANGE:
				dt = DAMAGE_TYPE_RANGE;
				// À¸¾Æ¾Æ¾Æ¾Ç
				// ¿¹Àü¿¡ Àû¿ë¾ÈÇß´ø ¹ö±×°¡ ÀÖ¾î¼­ ¹æ¾î·Â °è»êÀ» ´Ù½ÃÇÏ¸é À¯Àú°¡ ³­¸®³²
				//iDam -= pkChrVictim->GetPoint(POINT_DEF_GRADE);
				iDam = iDam * (100 - MINMAX(0, pkChrVictim->GetPoint(POINT_RESIST_BOW) - m_pkChr->GetPoint(POINT_RESIST_BOW_PEN), 100)) / 100;
				break;

			case SKILL_ATTR_TYPE_MAGIC:
				dt = DAMAGE_TYPE_MAGIC;
				iDam = CalcAttBonus(m_pkChr, pkChrVictim, iDam);
				// À¸¾Æ¾Æ¾Æ¾Ç
				// ¿¹Àü¿¡ Àû¿ë¾ÈÇß´ø ¹ö±×°¡ ÀÖ¾î¼­ ¹æ¾î·Â °è»êÀ» ´Ù½ÃÇÏ¸é À¯Àú°¡ ³­¸®³²
				//iDam -= pkChrVictim->GetPoint(POINT_MAGIC_DEF_GRADE);
				iDam = iDam * (100 - (pkChrVictim->GetPoint(POINT_RESIST_MAGIC) - m_pkChr->GetPoint(POINT_ANTI_RESIST_MAGIC))) / 100;
				break;

			default:
				sys_err("Unknown skill attr type %u vnum %u", m_pkSk->bSkillAttrType, m_pkSk->dwVnum);
				break;
		}

		//
		// 20091109 µ¶ÀÏ ½ºÅ³ ¼Ó¼º ¿äÃ» ÀÛ¾÷
		// ±âÁ¸ ½ºÅ³ Å×ÀÌºí¿¡ SKILL_FLAG_WIND, SKILL_FLAG_ELEC, SKILL_FLAG_FIRE¸¦ °¡Áø ½ºÅ³ÀÌ
		// ÀüÇô ¾ø¾úÀ¸¹Ç·Î ¸ó½ºÅÍÀÇ RESIST_WIND, RESIST_ELEC, RESIST_FIREµµ »ç¿ëµÇÁö ¾Ê°í ÀÖ¾ú´Ù.
		//
		// PvP¿Í PvE¹ë·±½º ºÐ¸®¸¦ À§ÇØ ÀÇµµÀûÀ¸·Î NPC¸¸ Àû¿ëÇÏµµ·Ï ÇßÀ¸¸ç ±âÁ¸ ¹ë·±½º¿Í Â÷ÀÌÁ¡À»
		// ´À³¢Áö ¸øÇÏ±â À§ÇØ mob_protoÀÇ RESIST_MAGICÀ» RESIST_WIND, RESIST_ELEC, RESIST_FIRE·Î
		// º¹»çÇÏ¿´´Ù.
		//
		if (pkChrVictim->IsNPC())
		{
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_WIND))
			{
				iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_WIND)) / 100;
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_ELEC))
			{
				iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_ELEC)) / 100;
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_FIRE))
			{
				iDam = iDam * (100 - pkChrVictim->GetPoint(POINT_RESIST_FIRE)) / 100;
			}
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_COMPUTE_MAGIC_DAMAGE))
			dt = DAMAGE_TYPE_MAGIC;

		if (pkChrVictim->CanBeginFight())
			pkChrVictim->BeginFight(m_pkChr);

		if (m_pkSk->dwVnum == SKILL_CHAIN)
			sys_log(0, "%s CHAIN INDEX %d DAM %d DT %d", m_pkChr->GetName(), m_pkChr->GetChainLightningIndex() - 1, iDam, dt);

		{
			BYTE AntiSkillID = 0;
			BYTE boostSkill = 0;
			double ResistAmount = 0.0;
			double boostAmount = 0.0;

			/*switch (m_pkSk->dwVnum)
			{
				case SKILL_TANHWAN:		AntiSkillID = SKILL_7_A_ANTI_TANHWAN;		break;
				case SKILL_AMSEOP:		AntiSkillID = SKILL_7_B_ANTI_AMSEOP;		break;
				case SKILL_SWAERYUNG:	AntiSkillID = SKILL_7_C_ANTI_SWAERYUNG;		break;
				case SKILL_YONGBI:		AntiSkillID = SKILL_7_D_ANTI_YONGBI;		break;
				case SKILL_GIGONGCHAM:	AntiSkillID = SKILL_8_A_ANTI_GIGONGCHAM;	break;
				case SKILL_YEONSA:		AntiSkillID = SKILL_8_B_ANTI_YEONSA;		break;
				case SKILL_MAHWAN:		AntiSkillID = SKILL_8_C_ANTI_MAHWAN;		break;
				case SKILL_BYEURAK:		AntiSkillID = SKILL_8_D_ANTI_BYEURAK;		break;
			}*/

			switch (m_pkSk->dwVnum)
			{
			case SKILL_PALBANG:		AntiSkillID = SKILL_WARD_SWORDSPIN; boostSkill = SKILL_BOOST_SWORDSPIN;		break;
			case SKILL_AMSEOP:		AntiSkillID = SKILL_WARD_AMBUSH; boostSkill = SKILL_BOOST_AMBUSH;		break;
			case SKILL_SWAERYUNG:	AntiSkillID = SKILL_WARD_FINGERSTRIKE; boostSkill = SKILL_BOOST_FINGERSTRIKE;		break;
			case SKILL_YONGBI:		AntiSkillID = SKILL_WARD_SHOOTINGDRAGON; boostSkill = SKILL_BOOST_SHOOTINGDRAGON;		break;
			case SKILL_GIGONGCHAM:	AntiSkillID = SKILL_WARD_SPIRITSTRIKE; boostSkill = SKILL_BOOST_SPIRITSTRIKE;	break;
			case SKILL_HWAJO:		AntiSkillID = SKILL_WARD_FIREARROW;	boostSkill = SKILL_BOOST_FIREARROW;	break;
			case SKILL_MARYUNG:		AntiSkillID = SKILL_WARD_DARKSTRIKE; boostSkill = SKILL_BOOST_DARKSTRIKE;		break;
			case SKILL_BYEURAK:		AntiSkillID = SKILL_WARD_SUMMONLIGHTNING; boostSkill = SKILL_BOOST_SUMMONLIGHTNING;		break;
			}

			if (0 != AntiSkillID)
			{
				BYTE AntiSkillLevel = pkChrVictim->GetSkillLevel(AntiSkillID);

				if (0 != AntiSkillLevel)
				{
					CSkillProto* pkSk = CSkillManager::instance().Get(AntiSkillID);
					if (!pkSk)
					{
						sys_err ("There is no anti skill(%d) in skill proto", AntiSkillID);
					}
					else
					{
						pkSk->SetVar("k", 1.0f * pkChrVictim->GetSkillPower(AntiSkillID) / 100);

						ResistAmount = pkSk->kPointPoly.Evaluate();
						if (test_server)
							m_pkChr->tchat("ANTI_SKILL: Resist(%lf) Orig(%d) Reduce(%d)", ResistAmount, iDam, int(iDam * (ResistAmount/100.0)));

						//iDam -= iDam * (ResistAmount/100.0);
					}
				}
			}

			if (0 != boostSkill)
			{
				BYTE boostLevel = m_pkChr->GetSkillLevel(boostSkill);

				if (0 != boostLevel)
				{
					CSkillProto* pkSk = CSkillManager::instance().Get(boostSkill);
					if (!pkSk)
					{
						sys_err("There is no boost skill(%d) in skill proto", boostSkill);
					}
					else
					{
						pkSk->SetVar("k", 1.0f * m_pkChr->GetSkillPower(boostSkill) / 100);

						boostAmount = pkSk->kPointPoly.Evaluate();
						if (test_server)
							m_pkChr->tchat("BOOST_SKILL: boost(%lf) Orig(%d) boosted(%d)", boostAmount, iDam, int(iDam * (boostAmount / 100.0)));

						//iDam -= iDam * (ResistAmount/100.0);
					}
				}
			}

			if (test_server)
				m_pkChr->tchat("BOOST/WARD_SKILL: pct(%lf) Orig(%d) calced(%d)", boostAmount, iDam, int(iDam * (boostAmount / 100.0)));

			boostAmount -= ResistAmount;
			iDam += iDam * (boostAmount/100.0);
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SPLASH))
			iDam += m_pkChr->GetPoint(POINT_MULTITARGET_SKILL_DAMAGE_BONUS) * iDam / 100;
		else
			iDam += m_pkChr->GetPoint(POINT_SINGLETARGET_SKILL_DAMAGE_BONUS) * iDam / 100;

		if (!pkChrVictim->Damage(m_pkChr, iDam, dt) && !pkChrVictim->IsStun())
		{
			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_REMOVE_GOOD_AFFECT))
			{
				int iAmount2 = (int) m_pkSk->kPointPoly2.Evaluate();
				int iDur2 = (int) m_pkSk->kDurationPoly2.Evaluate();
				if (m_pkSk->dwVnum != SKILL_PABEOB)
					iDur2 += m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (random_number(1, 100) <= iAmount2)
				{
					pkChrVictim->RemoveGoodAffect(true);
					pkChrVictim->AddAffect(m_pkSk->dwVnum, POINT_NONE, 0, AFF_PABEOP, iDur2, 0, true);
				}
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW | SKILL_FLAG_STUN | SKILL_FLAG_FIRE_CONT | SKILL_FLAG_POISON))
			{
				int iPct = (int) m_pkSk->kPointPoly2.Evaluate();
				int iDur = (int) m_pkSk->kDurationPoly2.Evaluate();

				iDur += m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_STUN))
				{
					if (pkChrVictim->IsPC() && pkChrVictim->IsRiding())
						SkillAttackAffect(pkChrVictim, iPct, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -125, AFF_SLOW, iDur, m_pkSk->szName);
					else
						SkillAttackAffect(pkChrVictim, iPct, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, iDur, m_pkSk->szName);
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SLOW))
				{
					SkillAttackAffect(pkChrVictim, iPct, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, iDur, m_pkSk->szName);
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_FIRE_CONT))
				{
					m_pkSk->SetVar("k", 1.0 * m_bUseSkillPower * m_pkSk->bMaxLevel / 100);
					m_pkSk->SetVar("iq", m_pkChr->GetPoint(POINT_IQ));

					iDur = (int)m_pkSk->kDurationPoly2.Evaluate();
					int bonus = m_pkChr->GetPoint(POINT_PARTY_BUFFER_BONUS);

					if (bonus != 0)
					{
						iDur += bonus / 2;
					}

					if (random_number(1, 100) <= iDur)
					{
						pkChrVictim->AttackedByFire(m_pkChr, iPct, 5);
					}
				}
				else if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_POISON))
				{
					if (random_number(1, 100) <= iPct)
						pkChrVictim->AttackedByPoison(m_pkChr);
				}
			}

			if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH | SKILL_FLAG_CRUSH_LONG) &&
				!IS_SET(pkChrVictim->GetAIFlag(), AIFLAG_NOMOVE))
			{
				float fCrushSlidingLength = 200;

				if (m_pkChr->IsNPC())
					fCrushSlidingLength = 400;

				if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_CRUSH_LONG))
					fCrushSlidingLength *= 2;

#ifdef IGNORE_KNOCKBACK
				if (ignoreKnockBack(m_pkChr->GetRaceNum()))
					fCrushSlidingLength = 0;
#endif

				float fx, fy;
				float degree = GetDegreeFromPositionXY(m_pkChr->GetX(), m_pkChr->GetY(), pkChrVictim->GetX(), pkChrVictim->GetY());

				if (m_pkSk->dwVnum == SKILL_HORSE_WILDATTACK)
				{
					degree -= m_pkChr->GetRotation();
					degree = fmod(degree, 360.0f) - 180.0f;

					if (degree > 0)
						degree = m_pkChr->GetRotation() + 90.0f;
					else
						degree = m_pkChr->GetRotation() - 90.0f;
				}

				GetDeltaByDegree(degree, fCrushSlidingLength, &fx, &fy);
				sys_log(!test_server, "CRUSH! %s -> %s (%d %d) -> (%d %d)", m_pkChr->GetName(), pkChrVictim->GetName(), pkChrVictim->GetX(), pkChrVictim->GetY(), (long)(pkChrVictim->GetX()+fx), (long)(pkChrVictim->GetY()+fy));
				long tx = (long)(pkChrVictim->GetX()+fx);
				long ty = (long)(pkChrVictim->GetY()+fy);

				pkChrVictim->Sync(tx, ty);
				pkChrVictim->Goto(tx, ty);
				pkChrVictim->CalculateMoveDuration();

				if (m_pkChr->IsPC() && m_pkChr->m_SkillUseInfo[m_pkSk->dwVnum].GetMainTargetVID() == (DWORD) pkChrVictim->GetVID())
				{
					if (pkChrVictim->IsPC() && pkChrVictim->IsRiding())
						SkillAttackAffect(pkChrVictim, 1000, IMMUNE_SLOW, m_pkSk->dwVnum, POINT_MOV_SPEED, -125, AFF_SLOW, 4, m_pkSk->szName);
					else
						SkillAttackAffect(pkChrVictim, 1000, IMMUNE_STUN, m_pkSk->dwVnum, POINT_NONE, 0, AFF_STUN, 4, m_pkSk->szName);
				}
				else
				{
					pkChrVictim->SyncPacket();
				}
			}
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_HP_ABSORB))
		{
			int iPct = (int) m_pkSk->kPointPoly2.Evaluate();
			m_pkChr->PointChange(POINT_HP, iDam * iPct / 100);
		}

		if (IS_SET(m_pkSk->dwFlag, SKILL_FLAG_SP_ABSORB))
		{
			int iPct = (int) m_pkSk->kPointPoly2.Evaluate();
			m_pkChr->PointChange(POINT_SP, iDam * iPct / 100);
		}

		if (m_pkSk->dwVnum == SKILL_CHAIN && m_pkChr->GetChainLightningIndex() < m_pkChr->GetChainLightningMaxCount())
		{
			chain_lightning_event_info* info = AllocEventInfo<chain_lightning_event_info>();

			info->dwVictim = pkChrVictim->GetVID();
			info->dwChr = m_pkChr->GetVID();

			event_create(ChainLightningEvent, info, passes_per_sec / 5);
		}
		if(test_server)
			sys_log(0, "FuncSplashDamage End :%s ", m_pkChr->GetName());
	}

	int		m_x;
	int		m_y;
	CSkillProto * m_pkSk;
	LPCHARACTER	m_pkChr;
	int		m_iAmount;
	int		m_iAG;
	int		m_iCount;
	int		m_iMaxHit;
	LPITEM	m_pkWeapon;
#ifdef __ACCE_COSTUME__
	LPITEM m_pAcce;
#endif
	bool m_bDisableCooltime;
	TSkillUseInfo* m_pInfo;
	BYTE m_bUseSkillPower;
};

struct FuncSplashAffect
{
	FuncSplashAffect(LPCHARACTER ch, int x, int y, int iDist, DWORD dwVnum, BYTE bPointOn, int iAmount, DWORD dwAffectFlag, int iDuration, int iSPCost, bool bOverride, int iMaxHit)
	{
		m_x = x;
		m_y = y;
		m_iDist = iDist;
		m_dwVnum = dwVnum;
		m_bPointOn = bPointOn;
		m_iAmount = iAmount;
		m_dwAffectFlag = dwAffectFlag;
		m_iDuration = iDuration;
		m_iSPCost = iSPCost;
		m_bOverride = bOverride;
		m_pkChrAttacker = ch;
		m_iMaxHit = iMaxHit;
		m_iCount = 0;
	}

	void operator () (LPENTITY ent)
	{
		if (m_iMaxHit && m_iMaxHit <= m_iCount)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pkChr = (LPCHARACTER) ent;

			if (test_server)
				sys_log(0, "FuncSplashAffect step 1 : name:%s vnum:%d iDur:%d", pkChr->GetName(), m_dwVnum, m_iDuration);
			if (DISTANCE_APPROX(m_x - pkChr->GetX(), m_y - pkChr->GetY()) < m_iDist)
			{
				if (test_server)
					sys_log(0, "FuncSplashAffect step 2 : name:%s vnum:%d iDur:%d", pkChr->GetName(), m_dwVnum, m_iDuration);
				if (m_dwVnum == SKILL_TUSOK)
					if (pkChr->CanBeginFight())
						pkChr->BeginFight(m_pkChrAttacker);

				if (pkChr->IsPC() && m_dwVnum == SKILL_TUSOK)
					pkChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration/3, m_iSPCost, m_bOverride);
				else
					pkChr->AddAffect(m_dwVnum, m_bPointOn, m_iAmount, m_dwAffectFlag, m_iDuration, m_iSPCost, m_bOverride);

				m_iCount ++;
			}
		}
	}

	LPCHARACTER m_pkChrAttacker;
	int		m_x;
	int		m_y;
	int		m_iDist;
	DWORD	m_dwVnum;
	BYTE	m_bPointOn;
	int		m_iAmount;
	DWORD	m_dwAffectFlag;
	int		m_iDuration;
	int		m_iSPCost;
	bool	m_bOverride;
	int		 m_iMaxHit;
	int		 m_iCount;
};

EVENTINFO(skill_gwihwan_info)
{
	DWORD pid;
	BYTE bsklv;

	skill_gwihwan_info()
	: pid( 0 )
	, bsklv( 0 )
	{
	}
};

EVENTFUNC(skill_gwihwan_event)
{
	skill_gwihwan_info* info = dynamic_cast<skill_gwihwan_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "skill_gwihwan_event> <Factor> Null pointer" );
		return 0;
	}

	DWORD pid = info->pid;
	BYTE sklv= info->bsklv;
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (!ch)
		return 0;

	int percent = 20 * sklv - 1;

	if (random_number(1, 100) <= percent)
	{
		PIXEL_POSITION pos;

		// ¼º°ø
		if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(ch->GetMapIndex(), ch->GetEmpire(), pos))
		{
			sys_log(1, "Recall: %s %d %d -> %d %d", ch->GetName(), ch->GetX(), ch->GetY(), pos.x, pos.y);
			ch->WarpSet(pos.x, pos.y);
		}
		else
		{
			sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", ch->GetName(), ch->GetX(), ch->GetY());
			ch->WarpSet(EMPIRE_START_X(ch->GetEmpire()), EMPIRE_START_Y(ch->GetEmpire()));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±ÍÈ¯¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
	}
	return 0;
}

int CHARACTER::ComputeSkillAtPosition(DWORD dwVnum, const PIXEL_POSITION& posTarget, BYTE bSkillLevel)
{
	if (GetMountVnum())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return BATTLE_NONE;

	if (test_server)
	{
		sys_log(0, "ComputeSkillAtPosition %s vnum %d x %d y %d level %d", 
				GetName(), dwVnum, posTarget.x, posTarget.y, bSkillLevel); 
	}

	// ³ª¿¡°Ô ¾²´Â ½ºÅ³Àº ³» À§Ä¡¸¦ ¾´´Ù.
	//if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
	//	posTarget = GetXYZ();

	// ½ºÇÃ·¡½¬°¡ ¾Æ´Ñ ½ºÅ³Àº ÁÖÀ§ÀÌ¸é ÀÌ»óÇÏ´Ù
	if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
		return BATTLE_NONE;

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pkSk->dwVnum)) == 0)
		{
			return BATTLE_NONE;
		}
	}

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetVar("k", k);
	pkSk->SetVar("k", k);

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pkSk->SetVar("atk", CalcMeleeDamage(this, this, true, false));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pkSk->SetVar("atk", CalcMagicDamage(this, this));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pkBow, pkArrow;
		if (1 == GetArrowAndBow(&pkBow, &pkArrow, 1))
		{
			pkSk->SetVar("atk", CalcArrowDamage(this, this, pkBow, pkArrow, true));
		}
		else
		{
			pkSk->SetVar("atk", 0);
		}
	}

	if (pkSk->bPointOn == POINT_MOV_SPEED)
	{
		pkSk->SetVar("maxv", this->GetLimitPoint(POINT_MOV_SPEED));
	}

	pkSk->SetVar("lv", GetLevel());
	pkSk->SetVar("iq", GetPoint(POINT_IQ));
	pkSk->SetVar("str", GetPoint(POINT_ST));
	pkSk->SetVar("dex", GetPoint(POINT_DX));
	pkSk->SetVar("con", GetPoint(POINT_HT));
	pkSk->SetVar("maxhp", this->GetMaxHP());
	pkSk->SetVar("maxsp", this->GetMaxSP());
	pkSk->SetVar("chain", 0);
	pkSk->SetVar("ar", CalcAttackRating(this, this));
	pkSk->SetVar("def", GetPoint(POINT_DEF_GRADE));
	pkSk->SetVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pkSk->SetVar("horse_level", GetHorseGrade() * 10);
	pkSk->SetVar("sklv", GetSkillLevel(pkSk->dwVnum));
	pkSk->SetVar("sklv", GetSkillLevel(pkSk->dwVnum));
#ifdef __LEGENDARY_SKILL__
	pkSk->SetVar("isLegendary", GetSkillMasterType(pkSk->dwVnum) == SKILL_LEGENDARY_MASTER ? 1 : 0);
#endif

	if (pkSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true, true);

	LPITEM pkWeapon = GetWear(WEAR_WEAPON);

#ifdef __ACCE_COSTUME__
	LPITEM pkAcce = GetWear(WEAR_ACCE);
	SetPolyVarForAttack(this, pkSk, pkWeapon, pkAcce);
#else
	SetPolyVarForAttack(this, pkSk, pkWeapon);
#endif

	pkSk->SetVar("k", k/*bSkillLevel*/);

	int iAmount = (int) pkSk->kPointPoly.Evaluate();
	int iAmount2 = (int) pkSk->kPointPoly2.Evaluate();

	// ADD_GRANDMASTER_SKILL
	int iAmount3 = (int) pkSk->kPointPoly3.Evaluate();

	if (test_server)
		ChatPacket(CHAT_TYPE_INFO, "useSkill %d amount %d", pkSk->dwVnum, iAmount);

	if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		/*
		   if (iAmount >= 0)
		   iAmount += (int) m_pkSk->kMasterBonusPoly.Evaluate();
		   else
		   iAmount -= (int) m_pkSk->kMasterBonusPoly.Evaluate();
		 */
		int iMasterAmount = (int) pkSk->kMasterBonusPoly.Evaluate();
		if (iMasterAmount != 0)
		{
			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "use master amount : %d -> %d", iAmount, iMasterAmount);
			iAmount = iMasterAmount;
		}
	}

	if (test_server && iAmount == 0 && pkSk->bPointOn != POINT_NONE)
	{
		ChatPacket(CHAT_TYPE_INFO, "SKILL_AMOUNT == 0");
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (random_number(1, 100) <= iAmount2)
		{
			RemoveBadAffect();
		}
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		//
		// °ø°Ý ½ºÅ³ÀÏ °æ¿ì
		//
		bool bAdded = false;

		if (pkSk->bPointOn == POINT_HP && iAmount < 0) 
		{
			int iAG = 0;
			
#ifdef __ACCE_COSTUME__
			FuncSplashDamage f(posTarget.x, posTarget.y, pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, pkAcce, m_bDisableCooltime, IsPC() ? &m_SkillUseInfo[dwVnum] : NULL, GetSkillPower(dwVnum, bSkillLevel));
#else
			FuncSplashDamage f(posTarget.x, posTarget.y, pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, m_bDisableCooltime, IsPC() ? &m_SkillUseInfo[dwVnum] : NULL, GetSkillPower(dwVnum, bSkillLevel));
#endif

			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
			{
				if (GetSectree())
					GetSectree()->ForEachAround(f);
			}
			else
			{
				//if (dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN skill call FuncSplashDamage %s", GetName());
				f(this);
			}
		}
		else
		{
			//if (dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN skill no damage %d %s", iAmount, GetName());
			int iDur = (int) pkSk->kDurationPoly.Evaluate();

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)) // ±æµå ½ºÅ³Àº ÄðÅ¸ÀÓ Ã³¸®¸¦ ÇÏÁö ¾Ê´Â´Ù.
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_MUYEONG)
					{
						//if (dwVnum == SKILL_CHAIN) sys_log(0, "CHAIN skill cannot hit %s", GetName());
						return BATTLE_NONE;
					}


			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		if (pkSk->bPointOn2 != POINT_NONE)
		{
			int iDur = (int) pkSk->kDurationPoly2.Evaluate();

			sys_log(1, "try second %u %d %d", pkSk->dwVnum, pkSk->bPointOn2, iDur);

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
			else
			{
				PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER && pkSk->bPointOn3 != POINT_NONE)
		{
			int iDur = (int) pkSk->kDurationPoly3.Evaluate();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, 0 /*pkSk->dwAffectFlag3*/, iDur, 0, !bAdded);
				else
				{
					if (GetSectree())
					{
						FuncSplashAffect f(this, posTarget.x, posTarget.y, pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, 0 /*pkSk->dwAffectFlag3*/, iDur, 0, !bAdded, pkSk->lMaxHit);
						GetSectree()->ForEachAround(f);
					}
				}
			}
			else
			{
				PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		bool bAdded = false;
		int iDur = (int) pkSk->kDurationPoly.Evaluate();

		if (iDur > 0)
		{
#ifdef __LEGENDARY_SKILL__
			bool bIsLegendary = GetUsedSkillMasterType(dwVnum) == SKILL_LEGENDARY_MASTER;
#endif

			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
			iDur += GetPoint(POINT_SKILL_DURATION);
#endif

			// AffectFlag°¡ ¾ø°Å³ª, toggle ÇÏ´Â °ÍÀÌ ¾Æ´Ï¶ó¸é..
			pkSk->SetVar("k", k/*bSkillLevel*/);

			AddAffect(pkSk->dwVnum,
					  pkSk->bPointOn,
					  iAmount,
#ifdef __LEGENDARY_SKILL__
					  (bIsLegendary && pkSk->dwAffectFlagLegendary != 0) ? pkSk->dwAffectFlagLegendary : pkSk->dwAffectFlag,
#else
					  pkSk->dwAffectFlag,
#endif
					  iDur,
					  (long) pkSk->kDurationSPCostPoly.Evaluate(),
					  !bAdded);

			bAdded = true;
		}
		else
		{
			PointChange(pkSk->bPointOn, iAmount);
		}

		if (pkSk->bPointOn2 != POINT_NONE)
		{
			int iDur = (int) pkSk->kDurationPoly2.Evaluate();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded);
				bAdded = true;
			}
			else
			{
				PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER && pkSk->bPointOn3 != POINT_NONE)
		{
			int iDur = (int) pkSk->kDurationPoly3.Evaluate();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, 0 /*pkSk->dwAffectFlag3*/, iDur, 0, !bAdded);
			}
			else
			{
				PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_NONE;
	}
}

// bSkillLevel ÀÎÀÚ°¡ 0ÀÌ ¾Æ´Ò °æ¿ì¿¡´Â m_abSkillLevels¸¦ »ç¿ëÇÏÁö ¾Ê°í °­Á¦·Î
// bSkillLevel·Î °è»êÇÑ´Ù.
int CHARACTER::ComputeSkill(DWORD dwVnum, LPCHARACTER pkVictim, BYTE bSkillLevel)
{
	const bool bCanUseHorseSkill = CanUseHorseSkill();

	// ¸»À» Å¸°íÀÖÁö¸¸ ½ºÅ³Àº »ç¿ëÇÒ ¼ö ¾ø´Â »óÅÂ¶ó¸é return
	if (false == bCanUseHorseSkill && true == IsRiding())
		return BATTLE_NONE;

	if (IsPolymorphed())
		return BATTLE_NONE;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
		return BATTLE_NONE;

	if (bCanUseHorseSkill && pkSk->dwType != SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (!bCanUseHorseSkill && pkSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;
	

	// »ó´ë¹æ¿¡°Ô ¾²´Â °ÍÀÌ ¾Æ´Ï¸é ³ª¿¡°Ô ½á¾ß ÇÑ´Ù.
	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		pkVictim = this;

	if (GetMapIndex() == PVP_TOURNAMENT_MAP_INDEX)
	{
		if (GetJob() == JOB_SHAMAN)
		{
			switch (dwVnum)
			{
				case SKILL_HOSIN:
				case SKILL_REFLECT:
				case SKILL_GICHEON:
				case SKILL_JEONGEOP:
				case SKILL_KWAESOK:
				case SKILL_JEUNGRYEOK:
					pkVictim = this;
					break;
			}
		}
	}
	
#ifdef __WOLFMAN__
	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY) && !GetParty())
		pkVictim = this;
#endif

	if (!pkVictim)
	{
		if (test_server)
			sys_log(0, "ComputeSkill: %s Victim == null, skill %d", GetName(), dwVnum);

		return BATTLE_NONE;
	}

	if (pkSk->dwTargetRange && DISTANCE_SQRT(GetX() - pkVictim->GetX(), GetY() - pkVictim->GetY()) >= pkSk->dwTargetRange + 50)
	{
		if (test_server)
			sys_log(0, "ComputeSkill: Victim too far, skill %d : %s to %s (distance %u limit %u)", 
					dwVnum,
					GetName(),
					pkVictim->GetName(),
					(long)DISTANCE_SQRT(GetX() - pkVictim->GetX(), GetY() - pkVictim->GetY()),
					pkSk->dwTargetRange);

		return BATTLE_NONE;
	}

	if (0 == bSkillLevel)
	{
		if ((bSkillLevel = GetSkillLevel(pkSk->dwVnum)) == 0)
		{
			if (test_server)
				sys_log(0, "ComputeSkill : name:%s vnum:%d  skillLevelBySkill : %d ", GetName(), pkSk->dwVnum, bSkillLevel);
			return BATTLE_NONE;
		}
	}

	if (pkVictim->IsAffectFlag(AFF_PABEOP) && pkVictim->IsGoodAffect(dwVnum))
	{
		return BATTLE_NONE;
	}

	if (test_server)
		sys_log(0, "ComputeSkill %s skill %u level %u", GetName(), dwVnum, bSkillLevel);

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetVar("k", k);
	pkSk->SetVar("k", k);

	if (pkSk->dwType == SKILL_TYPE_HORSE)
	{
		LPITEM pkBow, pkArrow;
		if (1 == GetArrowAndBow(&pkBow, &pkArrow, 1))
		{
			pkSk->SetVar("atk", CalcArrowDamage(this, pkVictim, pkBow, pkArrow, true));
		}
		else
		{
			pkSk->SetVar("atk", CalcMeleeDamage(this, pkVictim, true, false));
		}
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MELEE_DAMAGE))
	{
		pkSk->SetVar("atk", CalcMeleeDamage(this, pkVictim, true, false));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_MAGIC_DAMAGE))
	{
		pkSk->SetVar("atk", CalcMagicDamage(this, pkVictim));
	}
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_ARROW_DAMAGE))
	{
		LPITEM pkBow, pkArrow;
		if (1 == GetArrowAndBow(&pkBow, &pkArrow, 1))
		{
			pkSk->SetVar("atk", CalcArrowDamage(this, pkVictim, pkBow, pkArrow, true));
		}
		else
		{
			pkSk->SetVar("atk", 0);
		}
	}

	if (pkSk->bPointOn == POINT_MOV_SPEED)
	{
		pkSk->SetVar("maxv", pkVictim->GetLimitPoint(POINT_MOV_SPEED));
	}

	pkSk->SetVar("lv", GetLevel());
	pkSk->SetVar("iq", GetPoint(POINT_IQ));
	pkSk->SetVar("str", GetPoint(POINT_ST));
	pkSk->SetVar("dex", GetPoint(POINT_DX));
	pkSk->SetVar("con", GetPoint(POINT_HT));
	pkSk->SetVar("maxhp", pkVictim->GetMaxHP());
	pkSk->SetVar("maxsp", pkVictim->GetMaxSP());
	pkSk->SetVar("chain", 0);
	pkSk->SetVar("ar", CalcAttackRating(this, pkVictim));
	pkSk->SetVar("def", GetPoint(POINT_DEF_GRADE));
	pkSk->SetVar("odef", GetPoint(POINT_DEF_GRADE) - GetPoint(POINT_DEF_GRADE_BONUS));
	pkSk->SetVar("horse_level", GetHorseGrade() * 10);
	pkSk->SetVar("sklv", GetSkillLevel(pkSk->dwVnum));
	pkSk->SetVar("sklv", GetSkillLevel(pkSk->dwVnum));
#ifdef __LEGENDARY_SKILL__
	pkSk->SetVar("isLegendary", GetSkillMasterType(pkSk->dwVnum) == SKILL_LEGENDARY_MASTER ? 1 : 0);
#endif

	if (pkSk->bSkillAttrType != SKILL_ATTR_TYPE_NORMAL)
		OnMove(true, true);

	LPITEM pkWeapon = GetWear(WEAR_WEAPON);
#ifdef __ACCE_COSTUME__
	LPITEM pkAcce = GetWear(WEAR_ACCE);
	SetPolyVarForAttack(this, pkSk, pkWeapon, pkAcce);
#else
	SetPolyVarForAttack(this, pkSk, pkWeapon);
#endif

	pkSk->SetVar("k", k/*bSkillLevel*/);
	pkSk->SetVar("k", k/*bSkillLevel*/);

	int iAmount = (int) pkSk->kPointPoly.Evaluate();
	int iAmount2 = (int) pkSk->kPointPoly2.Evaluate();
	int iAmount3 = (int) pkSk->kPointPoly3.Evaluate();

	if (test_server && IsPC())
		sys_log(0, "iAmount: %d %d %d , atk:%f skLevel:%f k:%f GetSkillPower(%d) MaxLevel:%d Per:%f",
				iAmount, iAmount2, iAmount3,
				pkSk->GetVar("atk"),
				pkSk->GetVar("k"),
				k,
				GetSkillPower(pkSk->dwVnum, bSkillLevel),
				pkSk->bMaxLevel,
				pkSk->bMaxLevel/100
				);

#ifdef __FAKE_BUFF__
	if (test_server && FakeBuff_Check())
	{
		sys_log(0, "FAKEBUFF: target %s selfFlag %d iAmount: %d %d %d , atk:%f skLevel:%f iq:%f k:%f GetSkillPower(%d) MaxLevel:%d Per:%f realLevel:%d",
			pkVictim->GetName(),
			IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY),
			iAmount, iAmount, iAmount,
			pkSk->GetVar("atk"),
			pkSk->GetVar("k"),
			pkSk->GetVar("iq"),
			k,
			GetSkillPower(pkSk->dwVnum, bSkillLevel),
			pkSk->bMaxLevel,
			pkSk->bMaxLevel/100.0f,
			bSkillLevel
			);
	}
#endif

	if (test_server && IsPC())
		ChatPacket(CHAT_TYPE_INFO, "useSkill %d amount %d", pkSk->dwVnum, iAmount);

	if (GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
	{
		/*
		if (iAmount >= 0)
		iAmount += (int) m_pkSk->kMasterBonusPoly.Evaluate();
		else
		iAmount -= (int) m_pkSk->kMasterBonusPoly.Evaluate();
		*/
		int iMasterAmount = (int)pkSk->kMasterBonusPoly.Evaluate();
		if (iMasterAmount != 0)
		{
			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "use master amount : %d -> %d", iAmount, iMasterAmount);
			iAmount = iMasterAmount;
		}
	}

	if (test_server && iAmount == 0 && pkSk->bPointOn != POINT_NONE)
	{
		ChatPacket(CHAT_TYPE_INFO, "SKILL_AMOUNT == 0");
	}
	// END_OF_ADD_GRANDMASTER_SKILL

	//sys_log(0, "XXX SKILL Calc %d Amount %d", dwVnum, iAmount);

	// REMOVE_BAD_AFFECT_BUG_FIX
	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_REMOVE_BAD_AFFECT))
	{
		if (random_number(1, 100) <= iAmount2)
		{
			pkVictim->RemoveBadAffect();
		}
	}
	// END_OF_REMOVE_BAD_AFFECT_BUG_FIX

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK | SKILL_FLAG_USE_MELEE_DAMAGE | SKILL_FLAG_USE_MAGIC_DAMAGE) &&
		!(pkSk->dwVnum == SKILL_MUYEONG && pkVictim == this) && !(pkSk->IsChargeSkill() && pkVictim == this))
	{
		bool bAdded = false;

		if (pkSk->bPointOn == POINT_HP && iAmount < 0)
		{
			int iAG = 0;

			// MUYEONG
			if (pkSk->dwVnum == SKILL_MUYEONG && GetPoint(POINT_EQUIP_SKILL_BONUS))
				iAmount = iAmount * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
			// MUYEONG END

#ifdef ENABLE_RUNE_SYSTEM
			CRuneManager::instance().OnSkillUse(this, pkSk->dwVnum);
#endif
#ifdef __ACCE_COSTUME__
			FuncSplashDamage f(pkVictim->GetX(), pkVictim->GetY(), pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, pkAcce, m_bDisableCooltime, IsPC() ? &m_SkillUseInfo[dwVnum] : NULL, GetSkillPower(dwVnum, bSkillLevel));
#else
			FuncSplashDamage f(pkVictim->GetX(), pkVictim->GetY(), pkSk, this, iAmount, iAG, pkSk->lMaxHit, pkWeapon, m_bDisableCooltime, IsPC() ? &m_SkillUseInfo[dwVnum] : NULL, GetSkillPower(dwVnum, bSkillLevel));
#endif
#ifdef __FAKE_PC__
			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH) || (FakePC_Check() && pkSk->iSplashRange > 0))
#else
			if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
#endif
			{
				if (pkVictim->GetSectree())
					pkVictim->GetSectree()->ForEachAround(f);
			}
			else
			{
				f(pkVictim);
			}
		}
		else
		{
			pkSk->SetVar("k", k/*bSkillLevel*/);
			int iDur = (int) pkSk->kDurationPoly.Evaluate();
			

			if (IsPC())
				if (!(dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)) // ±æµå ½ºÅ³Àº ÄðÅ¸ÀÓ Ã³¸®¸¦ ÇÏÁö ¾Ê´Â´Ù.
					if (!m_bDisableCooltime && !m_SkillUseInfo[dwVnum].HitOnce(dwVnum) && dwVnum != SKILL_MUYEONG)
					{
						return BATTLE_NONE;
					}

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (GetPoint(POINT_EQUIP_SKILL_BONUS))
				{
					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus: %d -> %d", iAmount, iAmount * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iAmount = iAmount * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
				}

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn, iAmount, pkSk->dwAffectFlag, iDur, 0, true, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}
				bAdded = true;
			}
		}

		// stun skills for fake pc
#ifdef __FAKE_PC__
		if (FakePC_Check())
		{
			const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

			for (size_t i = 0; i < sizeof(stunSkills) / sizeof(DWORD); ++i)
			{
				if (stunSkills[i] == pkSk->dwVnum)
				{
					SkillAttackAffect(pkVictim, 1000, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, 2, pkSk->szName);
					break;
				}
			}
		}
#endif

		if (pkSk->bPointOn2 != POINT_NONE && !pkSk->IsChargeSkill())
		{
			pkSk->SetVar("k", k/*bSkillLevel*/);
			int iDur = (int) pkSk->kDurationPoly2.Evaluate();

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (GetPoint(POINT_EQUIP_SKILL_BONUS))
				{
					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus2: %d -> %d", iAmount2, iAmount2 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iAmount2 = iAmount2 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
				}

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE && !pkSk->IsChargeSkill() && GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{
			pkSk->SetVar("k", k/*bSkillLevel*/);
			int iDur = (int) pkSk->kDurationPoly3.Evaluate();


			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (GetPoint(POINT_EQUIP_SKILL_BONUS))
				{
					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus2: %d -> %d", iAmount3, iAmount3 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iAmount3 = iAmount3 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
				}

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_DAMAGE;
	}
	else
	{
		if (dwVnum == SKILL_MUYEONG)
		{
			pkSk->SetVar("k", k/*bSkillLevel*/);
			pkSk->SetVar("k", k/*bSkillLevel*/);

			int iDur = (long) pkSk->kDurationPoly.Evaluate();
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
			iDur += GetPoint(POINT_SKILL_DURATION);
#endif
			if (GetPoint(POINT_EQUIP_SKILL_BONUS))
				iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);

			if (pkVictim == this)
				AddAffect(dwVnum,
						POINT_NONE, 0,
						AFF_MUYEONG, 
						iDur,
						(long) pkSk->kDurationSPCostPoly.Evaluate(),
						true);

			return BATTLE_NONE;
		}

		bool bAdded = false;
		pkSk->SetVar("k", k/*bSkillLevel*/);
		int iDur = (int) pkSk->kDurationPoly.Evaluate();

#ifdef __LEGENDARY_SKILL__
		bool bUseLegendaryAffect = true;
		bool bUseLegendaryAffect2 = true;
		if (GetUsedSkillMasterType(dwVnum) != SKILL_LEGENDARY_MASTER)
		{
			bUseLegendaryAffect = false;
			bUseLegendaryAffect2 = false;
		}
		else
		{
			if (pkSk->dwAffectFlagLegendary == 0)
				bUseLegendaryAffect = false;
			if (pkSk->dwAffectFlag2Legendary <= 1)
				bUseLegendaryAffect2 = false;
		}
#endif

		if (iDur > 0)
		{
			iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
			iDur += GetPoint(POINT_SKILL_DURATION);
#endif
			// AffectFlag°¡ ¾ø°Å³ª, toggle ÇÏ´Â °ÍÀÌ ¾Æ´Ï¶ó¸é..
			pkSk->SetVar("k", k/*bSkillLevel*/);

			if (GetPoint(POINT_EQUIP_SKILL_BONUS))
			{
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus: %d -> %d", iAmount, iAmount * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
				iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
				iAmount = iAmount * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
			}

			if (pkSk->bPointOn2 != POINT_NONE)
			{
				pkVictim->RemoveAffect(pkSk->dwVnum);

				int iDur2 = (int) pkSk->kDurationPoly2.Evaluate();

				if (iDur2 > 0)
				{
					if (test_server)
						sys_log(0, "SKILL_AFFECT: %s %s Dur:%d To:%d Amount:%d", 
								GetName(),
								pkSk->szName,
								iDur2,
								pkSk->bPointOn2,
								iAmount2);

					iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
					iDur2 += GetPoint(POINT_SKILL_DURATION);
#endif
					if (GetPoint(POINT_EQUIP_SKILL_BONUS))
					{
						if (test_server)
							ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus: %d -> %d", iAmount2, iAmount2 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
						iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
						iAmount2 = iAmount2 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
					}
#ifdef __LEGENDARY_SKILL__
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, bUseLegendaryAffect2 ? pkSk->dwAffectFlag2Legendary : pkSk->dwAffectFlag2, iDur2, 0, false);
#else
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, false);
#endif
				}
				else
				{
					pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
				}

				DWORD affact_flag = pkSk->dwAffectFlag;
#ifdef __LEGENDARY_SKILL__
				if (bUseLegendaryAffect)
					affact_flag = pkSk->dwAffectFlagLegendary;
#endif

				// ADD_GRANDMASTER_SKILL
				if ((pkSk->dwVnum == SKILL_CHUNKEON && GetUsedSkillMasterType(pkSk->dwVnum) < SKILL_GRAND_MASTER))
					affact_flag = AFF_CHEONGEUN_WITH_FALL;
				// END_OF_ADD_GRANDMASTER_SKILL

				pkVictim->AddAffect(pkSk->dwVnum,
						pkSk->bPointOn,
						iAmount,
						affact_flag,
						iDur,
						(long) pkSk->kDurationSPCostPoly.Evaluate(),
						false);
			}
			else
			{
				if (test_server)
					sys_log(0, "SKILL_AFFECT: %s %s Dur:%d To:%d Amount:%d", 
							GetName(),
							pkSk->szName,
							iDur,
							pkSk->bPointOn,
							iAmount);

				pkVictim->AddAffect(pkSk->dwVnum,
						pkSk->bPointOn,
						iAmount,
#ifdef __LEGENDARY_SKILL__
						bUseLegendaryAffect ? pkSk->dwAffectFlagLegendary : pkSk->dwAffectFlag,
#else
						pkSk->dwAffectFlag,
#endif
						iDur,
						(long) pkSk->kDurationSPCostPoly.Evaluate(),
						// ADD_GRANDMASTER_SKILL
						!bAdded);
				// END_OF_ADD_GRANDMASTER_SKILL
			}

			bAdded = true;
		}
		else
		{
			if (!pkSk->IsChargeSkill())
				pkVictim->PointChange(pkSk->bPointOn, iAmount);

			if (pkSk->bPointOn2 != POINT_NONE)
			{
				pkVictim->RemoveAffect(pkSk->dwVnum);

				int iDur2 = (int) pkSk->kDurationPoly2.Evaluate();

				if (iDur2 > 0)
				{
					iDur2 += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
					iDur2 += GetPoint(POINT_SKILL_DURATION);
#endif
					if (GetPoint(POINT_EQUIP_SKILL_BONUS))
					{
						if (test_server)
							ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus: %d -> %d", iAmount2, iAmount2 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
						iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
						iAmount2 = iAmount2 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
					}

					if (pkSk->IsChargeSkill())
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, AFF_TANHWAN_DASH, iDur2, 0, false);
					else
#ifdef __LEGENDARY_SKILL__
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, bUseLegendaryAffect2 ? pkSk->dwAffectFlag2Legendary : pkSk->dwAffectFlag2, iDur2, 0, false);
#else
						pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn2, iAmount2, pkSk->dwAffectFlag2, iDur2, 0, false);
#endif
				}
				else
				{
					pkVictim->PointChange(pkSk->bPointOn2, iAmount2);
				}

			}
		}

		// ADD_GRANDMASTER_SKILL
		if (pkSk->bPointOn3 != POINT_NONE && !pkSk->IsChargeSkill() && GetUsedSkillMasterType(pkSk->dwVnum) >= SKILL_GRAND_MASTER)
		{

			pkSk->SetVar("k", k/*bSkillLevel*/);
			int iDur = (int) pkSk->kDurationPoly3.Evaluate();

			sys_log(0, "try third %u %d %d %d 1894", pkSk->dwVnum, pkSk->bPointOn3, iDur, iAmount3);

			if (iDur > 0)
			{
				iDur += GetPoint(POINT_PARTY_BUFFER_BONUS);
#ifdef STANDARD_SKILL_DURATION
				iDur += GetPoint(POINT_SKILL_DURATION);
#endif
				if (GetPoint(POINT_EQUIP_SKILL_BONUS))
				{
					if (test_server)
						ChatPacket(CHAT_TYPE_INFO, "EquipSkillBonus: %d -> %d", iAmount3, iAmount3 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iDur = (int)(iDur * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100);
					iAmount3 = iAmount3 * (100 + GetPoint(POINT_EQUIP_SKILL_BONUS)) / 100;
				}

				if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_SPLASH))
					pkVictim->AddAffect(pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded);
				else
				{
					if (pkVictim->GetSectree())
					{
						FuncSplashAffect f(this, pkVictim->GetX(), pkVictim->GetY(), pkSk->iSplashRange, pkSk->dwVnum, pkSk->bPointOn3, iAmount3, /*pkSk->dwAffectFlag3*/ 0, iDur, 0, !bAdded, pkSk->lMaxHit);
						pkVictim->GetSectree()->ForEachAround(f);
					}
				}

				bAdded = true;
			}
			else
			{
				pkVictim->PointChange(pkSk->bPointOn3, iAmount3);
			}
		}
		// END_OF_ADD_GRANDMASTER_SKILL

		return BATTLE_NONE;
	}
}

struct FuncShamanGroupBuff
{
	LPCHARACTER	m_pkMe;
	LPCHARACTER	m_pkExcept;
	DWORD		m_dwSkillVnum;

	FuncShamanGroupBuff(LPCHARACTER pkMe, LPCHARACTER pkVictim, DWORD dwSkillVnum)
	{
		m_pkMe = pkMe;
		m_pkExcept = pkVictim ? pkVictim : pkMe;
		m_dwSkillVnum = dwSkillVnum;
	}

	void operator()(LPCHARACTER ch)
	{
		if (ch != m_pkExcept)
			m_pkMe->ComputeSkill(m_dwSkillVnum, ch);
	}
};

bool CHARACTER::UseSkill(DWORD dwVnum, LPCHARACTER pkVictim, bool bUseGrandMaster)
{
	if (false == CanUseSkill(dwVnum))
		return false;

	if (IsObserverMode())
		return false;

	if (!CanMove())
		return false;

	if (IsPolymorphed())
		return false;

	const bool bCanUseHorseSkill = CanUseHorseSkill();


	if (dwVnum == SKILL_HORSE_SUMMON)
	{
		if (GetSkillLevel(dwVnum) == 0)
			return false;

		if (GetHorseGrade() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸»ÀÌ ¾ø½À´Ï´Ù. ¸¶±Â°£ °æºñº´À» Ã£¾Æ°¡¼¼¿ä."));
		else
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "¸» ¼ÒÈ¯ ¾ÆÀÌÅÛÀ» »ç¿ëÇÏ¼¼¿ä."));

		return true;
	}

	// ¸»À» Å¸°íÀÖÁö¸¸ ½ºÅ³Àº »ç¿ëÇÒ ¼ö ¾ø´Â »óÅÂ¶ó¸é return false
	if (false == bCanUseHorseSkill && true == IsRiding())
		return false;

	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);
	sys_log(0, "%s: USE_SKILL: %d pkVictim %p", GetName(), dwVnum, get_pointer(pkVictim));

	if (!pkSk)
		return false;

	if (bCanUseHorseSkill && pkSk->dwType != SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (!bCanUseHorseSkill && pkSk->dwType == SKILL_TYPE_HORSE)
		return BATTLE_NONE;

	if (GetSkillLevel(dwVnum) == 0)
		return false;
	

	// NO_GRANDMASTER
	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		bUseGrandMaster = false;
	// END_OF_NO_GRANDMASTER

	// MINING
	if (GetWear(WEAR_WEAPON) && (GetWear(WEAR_WEAPON)->GetType() == ITEM_ROD || GetWear(WEAR_WEAPON)->GetType() == ITEM_PICK))
		return false;
	// END_OF_MINING

	m_SkillUseInfo[dwVnum].TargetVIDMap.clear();

	if (pkSk->IsChargeSkill())
	{
		if (IsAffectFlag(AFF_TANHWAN_DASH) || pkVictim && pkVictim != this)
		{
			if (!pkVictim)
				return false;

			if (!IsAffectFlag(AFF_TANHWAN_DASH))
			{
				if (!UseSkill(dwVnum, this))
					return false;
			}

			m_SkillUseInfo[dwVnum].SetMainTargetVID(pkVictim->GetVID());
			// DASH »óÅÂÀÇ ÅºÈ¯°ÝÀº °ø°Ý±â¼ú
			ComputeSkill(dwVnum, pkVictim);
			RemoveAffect(dwVnum);
			return true;
		}
	}

	if (dwVnum == SKILL_COMBO)
	{
		if (m_bComboIndex)
			m_bComboIndex = 0;
		else
			m_bComboIndex = GetSkillLevel(SKILL_COMBO);

		ChatPacket(CHAT_TYPE_COMMAND, "combo %d", m_bComboIndex);
		return true;
	}

	// Toggle ÇÒ ¶§´Â SP¸¦ ¾²Áö ¾ÊÀ½ (SelfOnly·Î ±¸ºÐ)
	if ((0 != pkSk->dwAffectFlag || pkSk->dwVnum == SKILL_MUYEONG) && (pkSk->dwFlag & SKILL_FLAG_TOGGLE) && RemoveAffect(pkSk->dwVnum))
	{
		return true;
	}

	if (IsAffectFlag(AFF_REVIVE_INVISIBLE))
		RemoveAffect(AFFECT_REVIVE_INVISIBLE);

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum) * pkSk->bMaxLevel / 100;

	pkSk->SetVar("k", k);
	pkSk->SetVar("k", k);

	// ÄðÅ¸ÀÓ Ã¼Å©
	pkSk->SetVar("k", k);
	int iCooltime = (int) pkSk->kCooldownPoly.Evaluate();
	int lMaxHit = pkSk->lMaxHit ? pkSk->lMaxHit : -1;

	pkSk->SetVar("k", k);

	DWORD dwCur = get_dword_time();

	if (dwVnum == SKILL_TERROR && !m_SkillUseInfo[dwVnum].IsCooltimeOver())
	{
		sys_log(0, " SKILL_TERROR's Cooltime is not delta over %u", m_SkillUseInfo[dwVnum].dwNextSkillUsableTime  - dwCur );
		return false;
	}

	int iNeededSP = 0;

	bool bHasSkillCost = true;
#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		bHasSkillCost = false;
#endif

	if (bHasSkillCost)
	{
		if (IS_SET(pkSk->dwFlag, SKILL_FLAG_USE_HP_AS_COST))
		{
			pkSk->SetVar("maxhp", GetMaxHP());
			pkSk->SetVar("v", GetHP());
			iNeededSP = (int)pkSk->kSPCostPoly.Evaluate();

			// ADD_GRANDMASTER_SKILL
			if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
			{
				iNeededSP = (int)pkSk->kGrandMasterAddSPCostPoly.Evaluate();
			}
			// END_OF_ADD_GRANDMASTER_SKILL	

			if (GetHP() < iNeededSP)
				return false;

			PointChange(POINT_HP, -iNeededSP);
		}
		else
		{
			// SKILL_FOMULA_REFACTORING
			pkSk->SetVar("maxhp", GetMaxHP());
			pkSk->SetVar("maxv", GetMaxSP());
			pkSk->SetVar("v", GetSP());

			iNeededSP = (int)pkSk->kSPCostPoly.Evaluate();

			if (GetSkillMasterType(dwVnum) >= SKILL_GRAND_MASTER && bUseGrandMaster)
			{
				iNeededSP = (int)pkSk->kGrandMasterAddSPCostPoly.Evaluate();
			}
			// END_OF_SKILL_FOMULA_REFACTORING

			if (GetSP() < iNeededSP)
				return false;

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "%s SP¼Ò¸ð: %d"), pkSk->szName, iNeededSP);

			PointChange(POINT_SP, -iNeededSP);
		}
	}

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		pkVictim = this;

	if (pkSk->dwVnum == SKILL_MUYEONG || pkSk->IsChargeSkill() && !IsAffectFlag(AFF_TANHWAN_DASH) && !pkVictim)
	{
		// Ã³À½ »ç¿ëÇÏ´Â ¹«¿µÁøÀº ÀÚ½Å¿¡°Ô Affect¸¦ ºÙÀÎ´Ù.
		pkVictim = this;
	}

	int iSplashCount = 1;

	if (false == m_bDisableCooltime)
	{
		if (false == 
				m_SkillUseInfo[dwVnum].UseSkill(
					bUseGrandMaster,
				   	(NULL != pkVictim && SKILL_HORSE_WILDATTACK != dwVnum) ? pkVictim->GetVID() : 0,
				   	ComputeCooltime(iCooltime * 1000),
				   	iSplashCount,
				   	lMaxHit))
		{
			if (test_server)
				ChatPacket(CHAT_TYPE_NOTICE, "cooltime not finished %s %d", pkSk->szName, iCooltime);

			return false;
		}
	}

	if (dwVnum == SKILL_CHAIN)
	{
		ResetChainLightningIndex();
		AddChainLightningExcept(pkVictim);
	}
	

	if (IS_SET(pkSk->dwFlag, SKILL_FLAG_SELFONLY))
		ComputeSkill(dwVnum, this);
#ifdef __WOLFMAN__
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY) && !GetParty())
		ComputeSkill(dwVnum, this);
	else if (IS_SET(pkSk->dwFlag, SKILL_FLAG_PARTY) && GetParty())
	{
		FPartyPIDCollector f;
		GetParty()->ForEachOnMapMember(f, GetMapIndex());
		for (std::vector <DWORD>::iterator it = f.vecPIDs.begin(); it != f.vecPIDs.end(); it++)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(*it);
			if (ch)
				ch->ComputeSkill(dwVnum, ch, GetSkillLevel(dwVnum));
		}
	}
#endif
#ifdef __FAKE_PC__
	else if (FakePC_Check())
		ComputeSkill(dwVnum, pkVictim);
#endif
	else if (!IS_SET(pkSk->dwFlag, SKILL_FLAG_ATTACK))
	{
		ComputeSkill(dwVnum, pkVictim);

#ifdef __FAKE_BUFF__
		if (GetJob() == JOB_SHAMAN && !FakeBuff_Check())
#else
		if (GetJob() == JOB_SHAMAN)
#endif
		{
			if ((!pkVictim || pkVictim->GetParty() == GetParty()) && GetParty())
			{
				FuncShamanGroupBuff f(this, pkVictim, dwVnum);
				GetParty()->ForEachOnMapMember(f, GetMapIndex());
			}
		}
	}
	else if (dwVnum == SKILL_BYEURAK)
		ComputeSkill(dwVnum, pkVictim);
	else if (dwVnum == SKILL_MUYEONG || pkSk->IsChargeSkill())
		ComputeSkill(dwVnum, pkVictim);

	m_dwLastSkillTime = get_dword_time();
	m_dwLastSkillVnum = dwVnum;

	return true;
}

int CHARACTER::GetUsedSkillMasterType(DWORD dwVnum)
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetUsedSkillMasterType(dwVnum);
#endif

	const TSkillUseInfo& rInfo = m_SkillUseInfo[dwVnum];

	if (GetSkillMasterType(dwVnum) < SKILL_GRAND_MASTER)
		return GetSkillMasterType(dwVnum);

	if (rInfo.isGrandMaster)
		return GetSkillMasterType(dwVnum);

	return MIN(GetSkillMasterType(dwVnum), SKILL_MASTER);
}

int CHARACTER::GetSkillMasterType(DWORD dwVnum) const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetSkillMasterType(dwVnum);
#endif

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
	{
		BYTE bLev = GetSkillLevel(dwVnum);
		if (bLev >= 40)
			return SKILL_PERFECT_MASTER;
		else if (bLev >= 30)
			return SKILL_GRAND_MASTER;
		else if (bLev >= 20)
			return SKILL_MASTER;
		else
			return SKILL_NORMAL;
	}
#endif

	if (!IsPC())
		return 0;

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow[1] %u", GetName(), dwVnum);
		return 0;
	}

	return m_pSkillLevels ? m_pSkillLevels[dwVnum].master_type():SKILL_NORMAL;
}

int CHARACTER::GetSkillPower(DWORD dwVnum, BYTE bLevel) const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetSkillPower(dwVnum, bLevel);
#endif

	// ÀÎ¾î¹ÝÁö ¾ÆÀÌÅÛ
	if (dwVnum >= SKILL_LANGUAGE1 && dwVnum <= SKILL_LANGUAGE3 && (IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE) || !g_bEmpireWhisper))
	{
		return 100;
	}

	if (dwVnum >= GUILD_SKILL_START && dwVnum <= GUILD_SKILL_END)
	{
		if (GetGuild())
			return 100 * GetGuild()->GetSkillLevel(dwVnum) / 7 / 7;
		else
			return 0;
	}

	if (bLevel)
	{
		//SKILL_POWER_BY_LEVEL
		return GetSkillPowerByLevel(bLevel, true);
		//END_SKILL_POWER_BY_LEVEL;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow[2] %u", GetName(), dwVnum);
		return 0;
	}

	//SKILL_POWER_BY_LEVEL
	return GetSkillPowerByLevel(GetSkillLevel(dwVnum));
	//SKILL_POWER_BY_LEVEL
}

int CHARACTER::GetSkillLevel(DWORD dwVnum) const
{
#ifdef __FAKE_PC__
	if (FakePC_Check())
		return FakePC_GetOwner()->GetSkillLevel(dwVnum);
#endif

#ifdef __FAKE_BUFF__
	if (FakeBuff_Check())
		return FakeBuff_GetOwner()->GetFakeBuffSkillLevel(dwVnum);
	//	return MINMAX(1, GetLevel() / 2.4f + 0.1f, SKILL_MAX_LEVEL);
#endif

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("%s skill vnum overflow[3] %u", GetName(), dwVnum);
		return 0;
	}

	return MIN(SKILL_MAX_LEVEL, m_pSkillLevels ? m_pSkillLevels[dwVnum].level() : 0);
}

EVENTFUNC(skill_muyoung_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "skill_muyoung_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}

	if (!ch->IsAffectFlag(AFF_MUYEONG))
	{
		ch->StopMuyeongEvent();
		return 0;
	}

	// 1. Find Victim
	FFindNearVictim f(ch, ch);
	if (ch->GetSectree())
	{
		ch->GetSectree()->ForEachAround(f);
		// 2. Shoot!
		if (f.GetVictim())
		{
			ch->CreateFly(FLY_SKILL_MUYEONG, f.GetVictim());
			ch->ComputeSkill(SKILL_MUYEONG, f.GetVictim());
		}
	}

	return PASSES_PER_SEC(3);
}

void CHARACTER::StartMuyeongEvent()
{
	if (m_pkMuyeongEvent)
		return;

	char_event_info* info = AllocEventInfo<char_event_info>();

	info->ch = this;
	m_pkMuyeongEvent = event_create(skill_muyoung_event, info, PASSES_PER_SEC(1));
}

void CHARACTER::StopMuyeongEvent()
{
	event_cancel(&m_pkMuyeongEvent);
}

void CHARACTER::SkillLearnWaitMoreTimeMessage(DWORD ms)
{
	//const char* str = "";
	//
	if (ms < 3 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "¸ö ¼ÓÀÌ ¶ß°Ì±º. ÇÏÁö¸¸ ¾ÆÁÖ Æí¾ÈÇØ. ÀÌ´ë·Î ±â¸¦ ¾ÈÁ¤½ÃÅ°ÀÚ."));
	else if (ms < 5 * 60)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "±×·¡, ÃµÃµÈ÷. Á»´õ ÃµÃµÈ÷, ±×·¯³ª ¸·Èû ¾øÀÌ ºü¸£°Ô!"));
	else if (ms < 10 * 60) // 10ºÐ
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "±×·¡, ÀÌ ´À³¦ÀÌ¾ß. Ã¼³»¿¡ ±â°¡ ¾ÆÁÖ Ãæ¸¸ÇØ."));
	else if (ms < 30 * 60) // 30ºÐ
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "´Ù ÀÐ¾ú´Ù! ÀÌÁ¦ ºñ±Þ¿¡ ÀûÇôÀÖ´Â ´ë·Î Àü½Å¿¡ ±â¸¦ µ¹¸®±â¸¸ ÇÏ¸é,"));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "±×°ÍÀ¸·Î ¼ö·ÃÀº ³¡³­ °Å¾ß!"));
	}
	else if (ms < 1 * 3600) // 1½Ã°£
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "ÀÌÁ¦ Ã¥ÀÇ ¸¶Áö¸· ÀåÀÌ¾ß! ¼ö·ÃÀÇ ³¡ÀÌ ´«¿¡ º¸ÀÌ°í ÀÖ¾î!"));
	else if (ms < 2 * 3600) // 2½Ã°£
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "¾ó¸¶ ¾È ³²¾Ò¾î! Á¶±Ý¸¸ ´õ!"));
	else if (ms < 3 * 3600)
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "ÁÁ¾Ò¾î! Á¶±Ý¸¸ ´õ ÀÐÀ¸¸é ³¡ÀÌ´Ù!"));
	else if (ms < 6 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "Ã¥Àåµµ ÀÌÁ¦ ¾ó¸¶ ³²Áö ¾Ê¾Ò±º."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "¹º°¡ ¸ö ¾È¿¡ ÈûÀÌ »ý±â´Â ±âºÐÀÎ °É."));
	}
	else if (ms < 12 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "ÀÌÁ¦ Á» ½½½½ °¡´ÚÀÌ ÀâÈ÷´Â °Í °°Àºµ¥."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "ÁÁ¾Æ, ÀÌ ±â¼¼·Î °è¼Ó ³ª°£´Ù!"));
	}
	else if (ms < 18 * 3600)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "¾Æ´Ï ¾î¶»°Ô µÈ °Ô Á¾ÀÏ ÀÐ¾îµµ ¸Ó¸®¿¡ ¾È µé¾î¿À³Ä."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "°øºÎÇÏ±â ½È¾îÁö³×."));
	}
	else //if (ms < 2 * 86400)
	{
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "»ý°¢¸¸Å­ ÀÐ±â°¡ ½±Áö°¡ ¾Ê±º. ÀÌÇØµµ ¾î·Æ°í ³»¿ëµµ ³­ÇØÇØ."));
		ChatPacket(CHAT_TYPE_TALKING, "%s", LC_TEXT(this, "ÀÌ·¡¼­¾ß °øºÎ°¡ ¾ÈµÈ´Ù±¸."));
	}
	/*
	   str = "30%";
	   else if (ms < 3 * 86400)
	   str = "10%";
	   else if (ms < 4 * 86400)
	   str = "5%";
	   else
	   str = "0%";*/

	//ChatPacket(CHAT_TYPE_TALKING, "%s", str);
}

void CHARACTER::DisableCooltime()
{
	m_bDisableCooltime = true;
}

bool CHARACTER::HasMobSkill() const
{
	return CountMobSkill() > 0;
}

size_t CHARACTER::CountMobSkill() const
{
	if (!m_pkMobData)
		return 0;

	size_t c = 0;

	for (size_t i = 0; i < MOB_SKILL_MAX_NUM; ++i)
		if (m_pkMobData->m_table.skills(i).vnum())
			++c;

	return c;
}

const TMobSkillInfo* CHARACTER::GetMobSkill(unsigned int idx) const
{
	if (idx >= MOB_SKILL_MAX_NUM)
		return NULL;

	if (!m_pkMobData)
		return NULL;

	if (0 == m_pkMobData->m_table.skills(idx).vnum())
		return NULL;

	return &m_pkMobData->m_mobSkillInfo[idx];
}

bool CHARACTER::CanUseMobSkill(unsigned int idx) const
{
	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
		return false;

	if (m_adwMobSkillCooltime[idx] > get_dword_time())
		return false;

	if (random_number(0, 1))
		return false;

	return true;
}

EVENTINFO(mob_skill_event_info)
{
	DynamicCharacterPtr ch;
	PIXEL_POSITION pos;
	DWORD vnum;
	int index;
	BYTE level;

	mob_skill_event_info()
	: ch()
	, pos()
	, vnum(0)
	, index(0)
	, level(0)
	{
	}
};

EVENTFUNC(mob_skill_hit_event)
{
	mob_skill_event_info * info = dynamic_cast<mob_skill_event_info *>( event->info );

	if ( info == NULL )
	{
		sys_err( "mob_skill_event_info> <Factor> Null pointer" );
		return 0;
	}

	// <Factor>
	LPCHARACTER ch = info->ch;
	if (ch == NULL) {
		return 0;
	}

	ch->ComputeSkillAtPosition(info->vnum, info->pos, info->level);
	ch->m_mapMobSkillEvent.erase(info->index);

	return 0;
}

struct FHealerParty
{
	FHealerParty(LPCHARACTER pkHealer) : m_pkHealer(pkHealer) {}

	void operator () (LPCHARACTER ch)
	{
		int iRevive = (int)(ch->GetMaxHP() / 100 * 15);
		int iHP = (ch->GetMaxHP() >= ch->GetHP() + iRevive) ? (int)(ch->GetHP() + iRevive) : (int)(ch->GetMaxHP());
		ch->SetHP(iHP);
		ch->EffectPacket(SE_EFFECT_HEALER);
		sys_log(0, "FHealerParty: %s heal the HP of %s with %d (new HP: %d).", m_pkHealer->GetName(), ch->GetName(), iRevive, ch->GetHP());
	}

	LPCHARACTER	m_pkHealer;
};

bool CHARACTER::UseMobSkill(unsigned int idx)
{
	if (IsPC())
		return false;
	const TMobSkillInfo* pInfo = GetMobSkill(idx);

	if (!pInfo)
	{
		sys_err("no skill by index %u (mob %u)", idx, GetRaceNum());
		return false;
	}

	DWORD dwVnum = pInfo->dwSkillVnum;
	CSkillProto * pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("no skill by vnum %u (index %u mob %u)", dwVnum, idx, GetRaceNum());
		return false;
	}

	const float k = 1.0 * GetSkillPower(pkSk->dwVnum, pInfo->bSkillLevel) * pkSk->bMaxLevel / 100;

	pkSk->SetVar("k", k);
	int iCooltime = (int) (pkSk->kCooldownPoly.Evaluate() * 1000);

	m_adwMobSkillCooltime[idx] = get_dword_time() + iCooltime;

	sys_log(0, "USE_MOB_SKILL: %s idx %d vnum %u cooltime %d", GetName(), idx, dwVnum, iCooltime);
	
	if ((IsMonster()) && (pkSk->dwVnum == 265))
	{
		LPPARTY pkParty = GetParty();
		FHealerParty f(this);
		if (pkParty)
			pkParty->ForEachMemberPtr(f);
		else
			f(this);

		return true;
	}

	if (m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack.empty())
	{
		sys_err("No skill hit data for mob %s index %d", GetName(), idx);
		return false;
	}

	for (size_t i = 0; i < m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack.size(); i++)
	{
		PIXEL_POSITION pos = GetXYZ();
		const TMobSplashAttackInfo& rInfo = m_pkMobData->m_mobSkillInfo[idx].vecSplashAttack[i];

		if (rInfo.dwHitDistance)
		{
			float fx, fy;
			GetDeltaByDegree(GetRotation(), rInfo.dwHitDistance, &fx, &fy);
			pos.x += (long) fx;
			pos.y += (long) fy;
		}

		if (rInfo.dwTiming)
		{
			if (test_server)
				sys_log(0, "			   timing %ums", rInfo.dwTiming);

			mob_skill_event_info* info = AllocEventInfo<mob_skill_event_info>();

			info->ch = this;
			info->pos = pos;
			info->level = pInfo->bSkillLevel;
			info->vnum = dwVnum;
			info->index = i;

			// <Factor> Cancel existing event first
			itertype(m_mapMobSkillEvent) it = m_mapMobSkillEvent.find(i);
			if (it != m_mapMobSkillEvent.end()) {
				LPEVENT existing = it->second;
				event_cancel(&existing);
				m_mapMobSkillEvent.erase(it);
			}

			m_mapMobSkillEvent.insert(std::make_pair(i, event_create(mob_skill_hit_event, info, PASSES_PER_SEC(rInfo.dwTiming) / 1000)));
		}
		else
		{
			ComputeSkillAtPosition(dwVnum, pos, pInfo->bSkillLevel);
		}
	}

	return true;
}

void CHARACTER::ResetMobSkillCooltime()
{
	memset(m_adwMobSkillCooltime, 0, sizeof(m_adwMobSkillCooltime));
}

bool CHARACTER::IsUsableSkillMotion(DWORD dwMotionIndex) const
{
	DWORD selfJobGroup = (GetJob()+1) * 10 + GetSkillGroup();

	const DWORD SKILL_NUM = 158;
	static DWORD s_anSkill2JobGroup[SKILL_NUM] = {
		0, // common_skill 0
		11, // job_skill 1
		11, // job_skill 2
		11, // job_skill 3
		11, // job_skill 4
		11, // job_skill 5
		11, // job_skill 6
		0, // common_skill 7
		0, // common_skill 8
		0, // common_skill 9
		0, // common_skill 10
		0, // common_skill 11
		0, // common_skill 12
		0, // common_skill 13
		0, // common_skill 14
		0, // common_skill 15
		12, // job_skill 16
		12, // job_skill 17
		12, // job_skill 18
		12, // job_skill 19
		12, // job_skill 20
		12, // job_skill 21
		0, // common_skill 22
		0, // common_skill 23
		0, // common_skill 24
		0, // common_skill 25
		0, // common_skill 26
		0, // common_skill 27
		0, // common_skill 28
		0, // common_skill 29
		0, // common_skill 30
		21, // job_skill 31
		21, // job_skill 32
		21, // job_skill 33
		21, // job_skill 34
		21, // job_skill 35
		21, // job_skill 36
		0, // common_skill 37
		0, // common_skill 38
		0, // common_skill 39
		0, // common_skill 40
		0, // common_skill 41
		0, // common_skill 42
		0, // common_skill 43
		0, // common_skill 44
		0, // common_skill 45
		22, // job_skill 46
		22, // job_skill 47
		22, // job_skill 48
		22, // job_skill 49
		22, // job_skill 50
		22, // job_skill 51
		0, // common_skill 52
		0, // common_skill 53
		0, // common_skill 54
		0, // common_skill 55
		0, // common_skill 56
		0, // common_skill 57
		0, // common_skill 58
		0, // common_skill 59
		0, // common_skill 60
		31, // job_skill 61
		31, // job_skill 62
		31, // job_skill 63
		31, // job_skill 64
		31, // job_skill 65
		31, // job_skill 66
		0, // common_skill 67
		0, // common_skill 68
		0, // common_skill 69
		0, // common_skill 70
		0, // common_skill 71
		0, // common_skill 72
		0, // common_skill 73
		0, // common_skill 74
		0, // common_skill 75
		32, // job_skill 76
		32, // job_skill 77
		32, // job_skill 78
		32, // job_skill 79
		32, // job_skill 80
		32, // job_skill 81
		0, // common_skill 82
		0, // common_skill 83
		0, // common_skill 84
		0, // common_skill 85
		0, // common_skill 86
		0, // common_skill 87
		0, // common_skill 88
		0, // common_skill 89
		0, // common_skill 90
		41, // job_skill 91
		41, // job_skill 92
		41, // job_skill 93
		41, // job_skill 94
		41, // job_skill 95
		41, // job_skill 96
		0, // common_skill 97
		0, // common_skill 98
		0, // common_skill 99
		0, // common_skill 100
		0, // common_skill 101
		0, // common_skill 102
		0, // common_skill 103
		0, // common_skill 104
		0, // common_skill 105
		42, // job_skill 106
		42, // job_skill 107
		42, // job_skill 108
		42, // job_skill 109
		42, // job_skill 110
		42, // job_skill 111
		0, // common_skill 112
		0, // common_skill 113
		0, // common_skill 114
		0, // common_skill 115
		0, // common_skill 116
		0, // common_skill 117
		0, // common_skill 118
		0, // common_skill 119
		0, // common_skill 120
		0, // common_skill 121
		0, // common_skill 122
		0, // common_skill 123
		0, // common_skill 124
		0, // common_skill 125
		0, // common_skill 126
		0, // common_skill 127
		0, // common_skill 128
		0, // common_skill 129
		0, // common_skill 130
		0, // common_skill 131
		0, // common_skill 132
		0, // common_skill 133
		0, // common_skill 134
		0, // common_skill 135
		0, // common_skill 136
		0, // job_skill 137
		0, // job_skill 138
		0, // job_skill 139
		0, // job_skill 140
		0, // common_skill 141
		0, // common_skill 142
		0, // common_skill 143
		0, // common_skill 144
		0, // common_skill 145
		0, // common_skill 146
		0, // common_skill 147
		0, // common_skill 148
		0, // common_skill 149
		0, // common_skill 150
		0, // common_skill 151
		0, // job_skill 152
		0, // job_skill 153
		0, // job_skill 154
		0, // job_skill 155
		0, // job_skill 156
		0, // job_skill 157
	}; // s_anSkill2JobGroup

	const DWORD MOTION_MAX_NUM 	= 124;
	const DWORD SKILL_LIST_MAX_COUNT	= 5;

	static DWORD s_anMotion2SkillVnumList[MOTION_MAX_NUM][SKILL_LIST_MAX_COUNT] =
	{
		// ½ºÅ³¼ö   ¹«»ç½ºÅ³ID  ÀÚ°´½ºÅ³ID  ¼ö¶ó½ºÅ³ID  ¹«´ç½ºÅ³ID
		{   0,		0,			0,			0,			0		}, //  0

		// 1¹ø Á÷±º ±âº» ½ºÅ³
		{   4,		1,			31,			61,			91		}, //  1
		{   4,		2,			32,			62,			92		}, //  2
		{   4,		3,			33,			63,			93		}, //  3
		{   4,		4,			34,			64,			94		}, //  4
		{   4,		5,			35,			65,			95		}, //  5
		{   4,		6,			36,			66,			96		}, //  6
		{   0,		0,			0,			0,			0		}, //  7
		{   0,		0,			0,			0,			0		}, //  8
		// 1¹ø Á÷±º ±âº» ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  9
		{   0,		0,			0,			0,			0		}, //  10
		{   0,		0,			0,			0,			0		}, //  11
		{   0,		0,			0,			0,			0		}, //  12
		{   0,		0,			0,			0,			0		}, //  13
		{   0,		0,			0,			0,			0		}, //  14
		{   0,		0,			0,			0,			0		}, //  15
		// ¿©À¯ºÐ ³¡

		// 2¹ø Á÷±º ±âº» ½ºÅ³
		{   4,		16,			46,			76,			106		}, //  16
		{   4,		17,			47,			77,			107		}, //  17
		{   4,		18,			48,			78,			108		}, //  18
		{   4,		19,			49,			79,			109		}, //  19
		{   4,		20,			50,			80,			110		}, //  20
		{   4,		21,			51,			81,			111		}, //  21
		{   0,		0,			0,			0,			0		}, //  22
		{   0,		0,			0,			0,			0		}, //  23
		// 2¹ø Á÷±º ±âº» ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  24
		{   0,		0,			0,			0,			0		}, //  25
		// ¿©À¯ºÐ ³¡

		// 1¹ø Á÷±º ¸¶½ºÅÍ ½ºÅ³
		{   4,		1,			31,			61,			91		}, //  26
		{   4,		2,			32,			62,			92		}, //  27
		{   4,		3,			33,			63,			93		}, //  28
		{   4,		4,			34,			64,			94		}, //  29
		{   4,		5,			35,			65,			95		}, //  30
		{   4,		6,			36,			66,			96		}, //  31
		{   0,		0,			0,			0,			0		}, //  32
		{   0,		0,			0,			0,			0		}, //  33
		// 1¹ø Á÷±º ¸¶½ºÅÍ ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  34
		{   0,		0,			0,			0,			0		}, //  35
		{   0,		0,			0,			0,			0		}, //  36
		{   0,		0,			0,			0,			0		}, //  37
		{   0,		0,			0,			0,			0		}, //  38
		{   0,		0,			0,			0,			0		}, //  39
		{   0,		0,			0,			0,			0		}, //  40
		// ¿©À¯ºÐ ³¡

		// 2¹ø Á÷±º ¸¶½ºÅÍ ½ºÅ³
		{   4,		16,			46,			76,			106		}, //  41
		{   4,		17,			47,			77,			107		}, //  42
		{   4,		18,			48,			78,			108		}, //  43
		{   4,		19,			49,			79,			109		}, //  44
		{   4,		20,			50,			80,			110		}, //  45
		{   4,		21,			51,			81,			111		}, //  46
		{   0,		0,			0,			0,			0		}, //  47
		{   0,		0,			0,			0,			0		}, //  48
		// 2¹ø Á÷±º ¸¶½ºÅÍ ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  49
		{   0,		0,			0,			0,			0		}, //  50
		// ¿©À¯ºÐ ³¡

		// 1¹ø Á÷±º ±×·£µå ¸¶½ºÅÍ ½ºÅ³
		{   4,		1,			31,			61,			91		}, //  51
		{   4,		2,			32,			62,			92		}, //  52
		{   4,		3,			33,			63,			93		}, //  53
		{   4,		4,			34,			64,			94		}, //  54
		{   4,		5,			35,			65,			95		}, //  55
		{   4,		6,			36,			66,			96		}, //  56
		{   0,		0,			0,			0,			0		}, //  57
		{   0,		0,			0,			0,			0		}, //  58
		// 1¹ø Á÷±º ±×·£µå ¸¶½ºÅÍ ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  59
		{   0,		0,			0,			0,			0		}, //  60
		{   0,		0,			0,			0,			0		}, //  61
		{   0,		0,			0,			0,			0		}, //  62
		{   0,		0,			0,			0,			0		}, //  63
		{   0,		0,			0,			0,			0		}, //  64
		{   0,		0,			0,			0,			0		}, //  65
		// ¿©À¯ºÐ ³¡

		// 2¹ø Á÷±º ±×·£µå ¸¶½ºÅÍ ½ºÅ³
		{   4,		16,			46,			76,			106		}, //  66
		{   4,		17,			47,			77,			107		}, //  67
		{   4,		18,			48,			78,			108		}, //  68
		{   4,		19,			49,			79,			109		}, //  69
		{   4,		20,			50,			80,			110		}, //  70
		{   4,		21,			51,			81,			111		}, //  71
		{   0,		0,			0,			0,			0		}, //  72
		{   0,		0,			0,			0,			0		}, //  73
		// 2¹ø Á÷±º ±×·£µå ¸¶½ºÅÍ ½ºÅ³ ³¡

		//¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  74
		{   0,		0,			0,			0,			0		}, //  75
		// ¿©À¯ºÐ ³¡

		// 1¹ø Á÷±º ÆÛÆåÆ® ¸¶½ºÅÍ ½ºÅ³
		{   4,		1,			31,			61,			91		}, //  76
		{   4,		2,			32,			62,			92		}, //  77
		{   4,		3,			33,			63,			93		}, //  78
		{   4,		4,			34,			64,			94		}, //  79
		{   4,		5,			35,			65,			95		}, //  80
		{   4,		6,			36,			66,			96		}, //  81
		{   0,		0,			0,			0,			0		}, //  82
		{   0,		0,			0,			0,			0		}, //  83
		// 1¹ø Á÷±º ÆÛÆåÆ® ¸¶½ºÅÍ ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  84
		{   0,		0,			0,			0,			0		}, //  85
		{   0,		0,			0,			0,			0		}, //  86
		{   0,		0,			0,			0,			0		}, //  87
		{   0,		0,			0,			0,			0		}, //  88
		{   0,		0,			0,			0,			0		}, //  89
		{   0,		0,			0,			0,			0		}, //  90
		// ¿©À¯ºÐ ³¡

		// 2¹ø Á÷±º ÆÛÆåÆ® ¸¶½ºÅÍ ½ºÅ³
		{   4,		16,			46,			76,			106		}, //  91
		{   4,		17,			47,			77,			107		}, //  92
		{   4,		18,			48,			78,			108		}, //  93
		{   4,		19,			49,			79,			109		}, //  94
		{   4,		20,			50,			80,			110		}, //  95
		{   4,		21,			51,			81,			111		}, //  96
		{   0,		0,			0,			0,			0		}, //  97
		{   0,		0,			0,			0,			0		}, //  98
		// 2¹ø Á÷±º ÆÛÆåÆ® ¸¶½ºÅÍ ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,		0,			0,			0,			0		}, //  99
		{   0,		0,			0,			0,			0		}, //  100
		// ¿©À¯ºÐ ³¡

		// ±æµå ½ºÅ³
		{   1,  152,	0,	0,	0}, //  101
		{   1,  153,	0,	0,	0}, //  102
		{   1,  154,	0,	0,	0}, //  103
		{   1,  155,	0,	0,	0}, //  104
		{   1,  156,	0,	0,	0}, //  105
		{   1,  157,	0,	0,	0}, //  106
		// ±æµå ½ºÅ³ ³¡

		// ¿©À¯ºÐ
		{   0,	0,	0,	0,	0}, //  107
		{   0,	0,	0,	0,	0}, //  108
		{   0,	0,	0,	0,	0}, //  109
		{   0,	0,	0,	0,	0}, //  110
		{   0,	0,	0,	0,	0}, //  111
		{   0,	0,	0,	0,	0}, //  112
		{   0,	0,	0,	0,	0}, //  113
		{   0,	0,	0,	0,	0}, //  114
		{   0,	0,	0,	0,	0}, //  115
		{   0,	0,	0,	0,	0}, //  116
		{   0,	0,	0,	0,	0}, //  117
		{   0,	0,	0,	0,	0}, //  118
		{   0,	0,	0,	0,	0}, //  119
		{   0,	0,	0,	0,	0}, //  120
		// ¿©À¯ºÐ ³¡

		// ½Â¸¶ ½ºÅ³
		{   2,  137,  140,	0,	0}, //  121
		{   1,  138,	0,	0,	0}, //  122
		{   1,  139,	0,	0,	0}, //  123
		// ½Â¸¶ ½ºÅ³ ³¡
	};

	if (dwMotionIndex >= MOTION_MAX_NUM)
	{
		sys_err("OUT_OF_MOTION_VNUM: name=%s, motion=%d/%d", GetName(), dwMotionIndex, MOTION_MAX_NUM);
		return false;
	}

	DWORD* skillVNums = s_anMotion2SkillVnumList[dwMotionIndex];

	DWORD skillCount = *skillVNums++;
	if (skillCount >= SKILL_LIST_MAX_COUNT)
	{
		sys_err("OUT_OF_SKILL_LIST: name=%s, count=%d/%d", GetName(), skillCount, SKILL_LIST_MAX_COUNT);
		return false;
	}

	for (DWORD skillIndex = 0; skillIndex != skillCount; ++skillIndex)
	{
		if (skillIndex >= SKILL_MAX_NUM)
		{
			sys_err("OUT_OF_SKILL_VNUM: name=%s, skill=%d/%d", GetName(), skillIndex, SKILL_MAX_NUM);
			return false;
		}

		DWORD eachSkillVNum = skillVNums[skillIndex];
		if ( eachSkillVNum != 0 )
		{
			DWORD eachJobGroup = s_anSkill2JobGroup[eachSkillVNum];

			if (0 == eachJobGroup || eachJobGroup == selfJobGroup)
			{
				// GUILDSKILL_BUG_FIX
				DWORD eachSkillLevel = 0;

				if (eachSkillVNum >= GUILD_SKILL_START && eachSkillVNum <= GUILD_SKILL_END)
				{
					if (GetGuild())
						eachSkillLevel = GetGuild()->GetSkillLevel(eachSkillVNum);
					else
						eachSkillLevel = 0;
				}
				else
				{
					eachSkillLevel = GetSkillLevel(eachSkillVNum);
				}

				if (eachSkillLevel > 0)
				{
					return true;
				}
				// END_OF_GUILDSKILL_BUG_FIX
			}
		}
	}

	return false;
}

void CHARACTER::ClearSkill()
{
	PointChange(POINT_SKILL, 4 + (GetLevel() - 5) - GetPoint(POINT_SKILL));

	ResetSkill();
}

void CHARACTER::ClearSubSkill()
{
	PointChange(POINT_SUB_SKILL, GetLevel() < 10 ? 0 : (GetLevel() - 9) - GetPoint(POINT_SUB_SKILL));

	if (m_pSkillLevels == NULL)
	{
		sys_err("m_pSkillLevels nil (name: %s)", GetName());
		return;
	}

	size_t count = sizeof(s_adwSubSkillVnums) / sizeof(s_adwSubSkillVnums[0]);

	for (size_t i = 0; i < count; ++i)
	{
		if (s_adwSubSkillVnums[i] >= SKILL_MAX_NUM)
			continue;

		m_pSkillLevels[s_adwSubSkillVnums[i]].Clear();
	}

	SetSkillLevelChanged();

	ComputePoints();
	SkillLevelPacket();
}

bool CHARACTER::ResetOneSkill(DWORD dwVnum)
{
	if (NULL == m_pSkillLevels)
	{
		sys_err("m_pSkillLevels nil (name %s, vnum %u)", GetName(), dwVnum);
		return false;
	}

	if (dwVnum >= SKILL_MAX_NUM)
	{
		sys_err("vnum overflow (name %s, vnum %u)", GetName(), dwVnum);
		return false;
	}

	BYTE level = m_pSkillLevels[dwVnum].level();

	m_pSkillLevels[dwVnum].set_level(0);
	m_pSkillLevels[dwVnum].set_master_type(0);
	m_pSkillLevels[dwVnum].set_next_read(0);

	if (level > 17)
		level = 17;

	PointChange(POINT_SKILL, level);

	LogManager::instance().CharLog(this, dwVnum, "ONE_SKILL_RESET_BY_SCROLL", "");

	SetSkillLevelChanged();

	ComputePoints();
	SkillLevelPacket();

	return true;
}

const DWORD* CHARACTER::GetUsableSkillList() const
{
	return CHARACTER_MANAGER::instance().GetUsableSkillList(GetJob(), GetSkillGroup());
}

bool CHARACTER::CanUseSkill(DWORD dwSkillVnum) const
{
	if (0 == dwSkillVnum) return false;

	if (0 < GetSkillGroup())
	{
		const DWORD* pSkill = 
#ifdef __FAKE_BUFF__
			FakeBuff_Check() ? FakeBuff_GetUsableSkillList() : 
#endif
			GetUsableSkillList();

		if (!pSkill)
			return false;

		for (int i = 0; i < CHARACTER_SKILL_COUNT; ++i)
		{
			if (pSkill[i] == dwSkillVnum) return true;
		}
	}

	//if (true == IsHorseRiding())
	
	if (true == IsRiding())
	{
		//¸¶¿îÆ® Å»°ÍÁß °í±Þ¸»¸¸ ½ºÅ³ »ç¿ë°¡´É
		if(GetMountVnum())
		{
			if( !((GetMountVnum() >= 20209 && GetMountVnum() <= 20212)	||
				GetMountVnum() == 20215 || GetMountVnum() == 20218 || GetMountVnum() == 20225	)	)
					return false;
		}

		switch(dwSkillVnum)
		{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				return true;
		}
	}

	switch( dwSkillVnum )
	{
		case 121: case 122: case 124: case 126: case 127: case 128: case 129: case 130:
		case 131:
		case 151: case 152: case 153: case 154: case 155: case 156: case 157: case 158: case 159:
			return true;
	}

	return false;
}

bool CHARACTER::CheckSkillHitCount(const BYTE SkillID, const VID TargetVID)
{
	std::map<int, TSkillUseInfo>::iterator iter = m_SkillUseInfo.find(SkillID);

	if (iter == m_SkillUseInfo.end())
	{
		sys_log(0, "SkillHack: Skill(%u) is not in container", SkillID);
		return false;
	}

	TSkillUseInfo& rSkillUseInfo = iter->second;

	if (false == rSkillUseInfo.bUsed)
	{
		sys_log(0, "SkillHack: not used skill(%u)", SkillID);
		return false;
	}

	switch (SkillID)
	{
		case SKILL_YONGKWON:
		case SKILL_HWAYEOMPOK:
		case SKILL_DAEJINGAK:
		case SKILL_PAERYONG:
			sys_log(0, "SkillHack: cannot use attack packet for skill(%u)", SkillID);
			return false;
	}

	std::unordered_map<VID, size_t>::iterator iterTargetMap = rSkillUseInfo.TargetVIDMap.find(TargetVID);

	if (rSkillUseInfo.TargetVIDMap.end() != iterTargetMap)
	{
		size_t MaxAttackCountPerTarget = 1;

		switch (SkillID)
		{
			case SKILL_SAMYEON:
			case SKILL_CHARYUN:
#ifdef __WOLFMAN__
			case SKILL_CHAYEOL:
#endif
				MaxAttackCountPerTarget = 3;
				break;

			case SKILL_HORSE_WILDATTACK_RANGE:
				MaxAttackCountPerTarget = 5;
				break;

			case SKILL_YEONSA:
				MaxAttackCountPerTarget = 7;
				break;

			case SKILL_HORSE_ESCAPE:
				MaxAttackCountPerTarget = 10;
				break;
		}

		if (iterTargetMap->second >= MaxAttackCountPerTarget)
		{
			sys_log(0, "SkillHack: Too Many Hit count from SkillID(%u) count(%u)", SkillID, iterTargetMap->second);
			return false;
		}

		iterTargetMap->second++;
	}
	else
	{
		rSkillUseInfo.TargetVIDMap.insert( std::make_pair(TargetVID, 1) );
	}

	return true;
}

