#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "utils.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "protocol.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "cmd.h"
#include "shop.h"
#include "shop_manager.h"
#include "safebox.h"
#include "regen.h"
#include "battle.h"
#include "exchange.h"
#include "questmanager.h"
#include "profiler.h"
#include "messenger_manager.h"
#include "party.h"
#include "p2p.h"
#include "affect.h"
#include "guild.h"
#include "guild_manager.h"
#include "log.h"
#include "empire_text_convert.h"
#include "refine.h"
#include "unique_item.h"
#include "building.h"
#include "gm.h"
#include "ani.h"
#include "motion.h"
#include "OXEvent.h"
#include "SpamFilter.h"
#include "../../common/VnumHelper.h"
#include "desc_client.h"
#include "general_manager.h"
#ifdef __GUILD_SAFEBOX__
#include "guild_safebox.h"
#endif
#ifdef __DRAGONSOUL__
#include "DragonSoul.h"
#endif

#ifdef COMBAT_ZONE
#include "combat_zone.h"
#endif

#ifdef ENABLE_RUNE_SYSTEM
#include "rune_manager.h"
#endif
#include "mob_manager.h"

#ifdef __PET_ADVANCED__
#include "pet_advanced.h"
#endif

#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#endif

using namespace network;

extern void SendShout(const char * szText, BYTE bEmpire);
extern int g_nPortalLimitTime;

static int __deposit_limit()
{
	return (1000*10000); // 1Ãµ¸¸
}

void SendBlockChatInfo(LPCHARACTER ch, int sec)
{
	if (sec <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ã¤ÆÃ ±ÝÁö »óÅÂÀÔ´Ï´Ù."));
		return;
	}

	long hour = sec / 3600;
	sec -= hour * 3600;

	long min = (sec / 60);
	sec -= min * 60;

	char buf[128+1];

	if (hour > 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT(ch, "%d ½Ã°£ %d ºÐ %d ÃÊ µ¿¾È Ã¤ÆÃ±ÝÁö »óÅÂÀÔ´Ï´Ù"), hour, min, sec);
	else if (hour > 0 && min == 0)
		snprintf(buf, sizeof(buf), LC_TEXT(ch, "%d ½Ã°£ %d ÃÊ µ¿¾È Ã¤ÆÃ±ÝÁö »óÅÂÀÔ´Ï´Ù"), hour, sec);
	else if (hour == 0 && min > 0)
		snprintf(buf, sizeof(buf), LC_TEXT(ch, "%d ºÐ %d ÃÊ µ¿¾È Ã¤ÆÃ±ÝÁö »óÅÂÀÔ´Ï´Ù"), min, sec);
	else
		snprintf(buf, sizeof(buf), LC_TEXT(ch, "%d ÃÊ µ¿¾È Ã¤ÆÃ±ÝÁö »óÅÂÀÔ´Ï´Ù"), sec);

	ch->ChatPacket(CHAT_TYPE_INFO, buf);
}

enum
{
	TEXT_TAG_PLAIN,
	TEXT_TAG_TAG, // ||
	TEXT_TAG_COLOR, // |cffffffff
	TEXT_TAG_HYPERLINK_START, // |H
	TEXT_TAG_HYPERLINK_END, // |h ex) |Hitem:1234:1:1:1|h
	TEXT_TAG_RESTORE_COLOR,
};

int GetTextTag(const char * src, int maxLen, int & tagLen, std::string & extraInfo)
{
	tagLen = 1;

	if (maxLen < 2 || *src != '|')
		return TEXT_TAG_PLAIN;

	const char * cur = ++src;

	if (*cur == '|') // ||´Â |·Î Ç¥½ÃÇÑ´Ù.
	{
		tagLen = 2;
		return TEXT_TAG_TAG;
	}
	else if (*cur == 'c') // color |cffffffffblahblah|r
	{
		tagLen = 2;
		return TEXT_TAG_COLOR;
	}
	else if (*cur == 'H') // hyperlink |Hitem:10000:0:0:0:0|h[ÀÌ¸§]|h
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_START;
	}
	else if (*cur == 'h') // end of hyperlink
	{
		tagLen = 2;
		return TEXT_TAG_HYPERLINK_END;
	}

	return TEXT_TAG_PLAIN;
}

void GetTextTagInfo(const char * src, int src_len, int & hyperlinks, bool & colored)
{
	colored = false;
	hyperlinks = 0;

	int len;
	std::string extraInfo;

	for (int i = 0; i < src_len;)
	{
		int tag = GetTextTag(&src[i], src_len - i, len, extraInfo);

		if (tag == TEXT_TAG_HYPERLINK_START)
			++hyperlinks;

		if (tag == TEXT_TAG_COLOR)
			colored = true;

		i += len;
	}
}

void CInputMain::Whisper(LPCHARACTER ch, std::unique_ptr<CGWhisperPacket> pinfo)
{
#ifdef __ANTI_FLOOD__	
	if (ch->IncreaseChatCounter() >= 10)
	{
		sys_log(0, "WHISPER_HACK: %s", ch->GetName());
		if (test_server)
			ch->tchat("WHISPER_FLOOD, NOW IGNORE");
		return;
	}
#endif

	if (ch->FindAffect(AFFECT_BLOCK_CHAT))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ã¤ÆÃ ±ÝÁö »óÅÂÀÔ´Ï´Ù."));
		return;
	}

#if defined(COMBAT_ZONE) && defined(COMBAT_ZONE_HIDE_INFO_USER)
	if (CCombatZoneManager::Instance().IsCombatZoneMap(ch->GetMapIndex()) && ch->GetGMLevel() == GM_PLAYER)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this on this map."));
		return;
	}
#endif

	LPCHARACTER pkChr = CHARACTER_MANAGER::instance().FindPC(pinfo->name_to().c_str());

	if (pkChr == ch)
		return;

	char buf[CHAT_MAX_LEN + 1];
	strlcpy(buf, pinfo->message().c_str(), MIN(pinfo->message().length() + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

#ifdef ENABLE_SPAM_FILTER
	if (std::string(pinfo->name_to().c_str()).find("[") == -1 && ch->GetGMLevel() == GM_PLAYER && CSpamFilter::Instance().IsBannedWord(ch, buf))
	{
		return;
	}
#endif

	LPDESC pkDesc = NULL;

	BYTE bOpponentEmpire = 0;
	DWORD dwOpponentPID = 0;
	const char* szOpponentName = "";

	if (test_server)
	{
		if (!pkChr)
			sys_log(0, "Whisper to %s(%s) from %s", "Null", pinfo->name_to().c_str(), ch->GetName());
		else
			sys_log(0, "Whisper to %s(%s) from %s", pkChr->GetName(), pinfo->name_to().c_str(), ch->GetName());
	}
		
	if (ch->IsBlockMode(BLOCK_WHISPER))
	{
		if (ch->GetDesc())
		{
			network::GCOutputPacket<network::GCWhisperPacket> pack;
			pack->set_type(WHISPER_TYPE_SENDER_BLOCKED);
			pack->set_name_from(pinfo->name_to().c_str());
			pack->set_message(buf);
			ch->GetDesc()->Packet(pack);
		}
		return;
	}

	if (!pkChr)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(pinfo->name_to().c_str());

		if (pkCCI)
		{
			pkDesc = pkCCI->pkDesc;
			pkDesc->SetRelay(pinfo->name_to().c_str());
			bOpponentEmpire = pkCCI->bEmpire;
			szOpponentName = pkCCI->szName;
			dwOpponentPID = pkCCI->dwPID;

			if (test_server)
				sys_log(0, "Whisper to %s from %s (Channel %d Mapindex %d)", "Null", ch->GetName(), pkCCI->bChannel, pkCCI->lMapIndex);
		}
	}
	else
	{
		pkDesc = pkChr->GetDesc();
		bOpponentEmpire = pkChr->GetEmpire();
		szOpponentName = pkChr->GetName();
		dwOpponentPID = pkChr->GetPlayerID();
	}

	if (!pkDesc)
	{
		if (ch->GetDesc())
		{
			const char* szSystemText = "[SYSTEM";
			
			if (!strncasecmp(pinfo->name_to().c_str(), szSystemText, strlen(szSystemText)))
			{
				if (ch->GetGMLevel(true) >= GM_GOD)
				{
					int iLangID = -1;
					if (strlen(pinfo->name_to().c_str()) > strlen(szSystemText) + 1)
					{
						int iLangLen = strlen(pinfo->name_to().c_str()) - (strlen(szSystemText) + 1) - 1;
						if (iLangLen > 9)
							return;

						char szLangText[10];
						strncpy(szLangText, pinfo->name_to().c_str() + strlen(szSystemText) + 1, iLangLen);
						szLangText[iLangLen] = '\0';

						for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
						{
							if (!strcasecmp(szLangText, CLocaleManager::instance().GetShortLanguageName(i)))
							{
								iLangID = i;
								break;
							}
						}
					}
			
					network::GCOutputPacket<network::GCWhisperPacket> pack;
					pack->set_type(WHISPER_TYPE_GM);
					pack->set_name_from("[SYSTEM]");
					pack->set_message(buf);

					// send p2p packet
					{
						network::GGOutputPacket<network::GGPlayerPacket> p2p_pack;
						p2p_pack->set_pid(0);
						p2p_pack->set_language(iLangID);
						p2p_pack->set_empire(0);

						p2p_pack->set_relay_header(pack.get_header());
						std::vector<uint8_t> buf;
						buf.resize(pack->ByteSize());
						pack->SerializeToArray(&buf[0], buf.size());
						p2p_pack->set_relay(&buf[0], buf.size());

						P2P_MANAGER::instance().Send(p2p_pack);
					}

					// send to player on this core
					{
						const CHARACTER_MANAGER::NAME_MAP& rkPCMap = CHARACTER_MANAGER::Instance().GetPCMap();
						for (itertype(rkPCMap) it = rkPCMap.begin(); it != rkPCMap.end(); ++it)
						{
							if (iLangID == -1 || it->second->GetLanguageID() == iLangID)
							{
								if (test_server)
									sys_log(0, "SendWhisper to [%u] %s: language %d", it->second->GetPlayerID(), it->second->GetName(), it->second->GetLanguageID());

								it->second->GetDesc()->Packet(pack);
							}
						}
					}
				}
				else
				{
					network::GCOutputPacket<network::GCWhisperPacket> pack;

					pack->set_type(WHISPER_TYPE_NOAUTH);
					pack->set_name_from(pinfo->name_to().c_str());
					pack->set_message(buf);

					ch->GetDesc()->Packet(pack);
				}
			}
			else if (ch->IsBlockMode(BLOCK_WHISPER))
			{
				network::GCOutputPacket<network::GCWhisperPacket> pack;
				pack->set_type(WHISPER_TYPE_SENDER_BLOCKED);
				pack->set_name_from(pinfo->name_to().c_str());
				pack->set_message(buf);

				ch->GetDesc()->Packet(pack);
			}
			else if (!pinfo->send_offline())
			{
				network::GDOutputPacket<network::GDWhisperPlayerExistCheckPacket> packet;
				packet->set_is_gm(ch->IsGM());
				packet->set_pid(ch->GetPlayerID());
				packet->set_target_name(pinfo->name_to());
				packet->set_message(buf);

				db_clientdesc->DBPacket(packet, ch->GetDesc()->GetHandle());
			}
			else
			{
				if (ch->GetGold() >= 15000 || (!test_server && ch->IsGM()))
				{
					char szBuf[256];
					snprintf(szBuf, sizeof(szBuf), "Send offline message to %s", pinfo->name_to().c_str());
					if (strncasecmp(pinfo->name_to().c_str(), "[", 1))
					{
						LogManager::instance().CharLog(ch, 15000, "OFFLINE_MESSAGE", szBuf);
						if (test_server || !ch->IsGM())
							ch->PointChange(POINT_GOLD, -15000);
					}
					network::GDOutputPacket<network::GDWhisperPlayerMessageOfflinePacket> packet;
					packet->set_is_gm(ch->IsGM());
					packet->set_pid(ch->GetPlayerID());
					packet->set_name(ch->GetName());
					packet->set_target_name(pinfo->name_to().c_str());
					packet->set_message(buf);

					db_clientdesc->DBPacket(packet, ch->GetDesc()->GetHandle());
				}
			}
		}
	}
	else
	{
		if (ch->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				network::GCOutputPacket<network::GCWhisperPacket> pack;
				pack->set_type(WHISPER_TYPE_SENDER_BLOCKED);
				pack->set_name_from(pinfo->name_to().c_str());
				pack->set_message(buf);
				ch->GetDesc()->Packet(pack);
			}
		}
		else if (pkChr && pkChr->IsBlockMode(BLOCK_WHISPER))
		{
			if (ch->GetDesc())
			{
				network::GCOutputPacket<network::GCWhisperPacket> pack;
				pack->set_type(WHISPER_TYPE_TARGET_BLOCKED);
				pack->set_name_from(pinfo->name_to().c_str());
				pack->set_message(buf);
				ch->GetDesc()->Packet(pack);
			}
		}

#ifdef ENABLE_MESSENGER_BLOCK
		else if (MessengerManager::instance().CheckMessengerList(ch->GetName(), pinfo->name_to().c_str(), SYST_BLOCK))
		{
			if (ch->GetDesc())
			{
				network::GCOutputPacket<network::GCWhisperPacket> pack;

				char msg_2[CHAT_MAX_LEN + 1];
				snprintf(msg_2, sizeof(msg_2), LC_TEXT(ch, "You can't write to %s, because you Blocked this player."), pinfo->name_to().c_str());
				int len = MIN(CHAT_MAX_LEN, strlen(msg_2) + 1);

				pack->set_type(WHISPER_TYPE_SYSTEM);
				pack->set_name_from(pinfo->name_to().c_str());
				pack->set_message(msg_2);

				ch->GetDesc()->Packet(pack);
			}
		}
#endif

		else
		{
			BYTE bType = WHISPER_TYPE_NORMAL;

			if (ch->IsGM())
				bType = (bType & 0xF0) | WHISPER_TYPE_GM;

			if (buflen > 0)
			{
				network::GCOutputPacket<network::GCWhisperPacket> pack;

				pack->set_type(bType);
				pack->set_name_from(ch->GetName());
				pack->set_message(buf);

				pkDesc->Packet(pack);

				LogManager::Instance().WhisperLog(ch->GetPlayerID(), ch->GetName(), dwOpponentPID, szOpponentName, buf, false);
			}
		}
	}
	if(pkDesc)
		pkDesc->SetRelay("");

	return;
}

struct FEmpireChatPacket
{
	GCOutputPacket<GCChatPacket>& p;
	const char* orig_msg;
	int orig_len;
	char converted_msg[CHAT_MAX_LEN+1];

	BYTE bEmpire;
	int iMapIndex;
	int namelen;
#ifdef ACCOUNT_TRADE_BLOCK
	bool hwid2banned;
#endif

	FEmpireChatPacket(GCOutputPacket<GCChatPacket>& p, const char* chat_msg, int len, BYTE bEmpire, int iMapIndex, int iNameLen
#ifdef ACCOUNT_TRADE_BLOCK
		, bool hwid2banned
#endif

	)
		: p(p), orig_msg(chat_msg), orig_len(len), bEmpire(bEmpire), iMapIndex(iMapIndex), namelen(iNameLen)
#ifdef ACCOUNT_TRADE_BLOCK
		, hwid2banned(hwid2banned)
#endif
	{
		memset( converted_msg, 0, sizeof(converted_msg) );
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetMapIndex() != iMapIndex)
			return;

#ifdef ACCOUNT_TRADE_BLOCK
		if (hwid2banned && !d->IsHWID2Banned())
			return;
#endif

		if (d->GetEmpire() == bEmpire ||
			bEmpire == 0 ||
			d->GetCharacter()->GetGMLevel() > GM_PLAYER ||
			d->GetCharacter()->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE) ||
			!g_bEmpireWhisper)
		{
			p->set_message(orig_msg);
		}
		else
		{
			// »ç¶÷¸¶´Ù ½ºÅ³·¹º§ÀÌ ´Ù¸£´Ï ¸Å¹ø ÇØ¾ßÇÕ´Ï´Ù
			size_t len = strlcpy(converted_msg, orig_msg, sizeof(converted_msg));

			if (len >= sizeof(converted_msg))
				len = sizeof(converted_msg) - 1;

			ConvertEmpireText(bEmpire, converted_msg + namelen, len - namelen, 10 + 2 * d->GetCharacter()->GetSkillPower(SKILL_LANGUAGE1 + bEmpire - 1));
			p->set_message(converted_msg, orig_len);
		}

		d->Packet(p);
	}
};

void CInputMain::Chat(LPCHARACTER ch, std::unique_ptr<CGChatPacket> pinfo)
{
	if (pinfo->message().length() > CHAT_MAX_LEN)
		return;

	char buf[CHAT_MAX_LEN - (CHARACTER_NAME_MAX_LEN + 3) + 1];
	strlcpy(buf, pinfo->message().c_str(), MIN(pinfo->message().length() + 1, sizeof(buf)));
	const size_t buflen = strlen(buf);

	if (buflen > 1 && *buf == '/')
	{
		interpret_command(ch, buf + 1, buflen - 1);
		return;
	}

	if (pinfo->type() == CHAT_TYPE_TEAM && !ch->IsGM())
	{
		sys_err("player %u %s trying to write in GM chat [message: %s]", ch->GetPlayerID(), ch->GetName(), buf);
		return;
	}

	if (ch->IncreaseChatCounter() >= 10 && !ch->IsGM())
	{
		if (ch->GetChatCounter() == 10)
		{
			sys_log(0, "CHAT_HACK: %s", ch->GetName());
			ch->GetDesc()->DelayedDisconnect(5);
		}

		return;
	}

#if defined(COMBAT_ZONE) && defined(COMBAT_ZONE_HIDE_INFO_USER)
	if (CCombatZoneManager::Instance().IsCombatZoneMap(ch->GetMapIndex()) && ch->GetGMLevel() == GM_PLAYER)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't do this on this map."));
		return;
	}
#endif

	const CAffect* pAffect = ch->FindAffect(AFFECT_BLOCK_CHAT);

	if (pAffect != NULL)
	{
		SendBlockChatInfo(ch, pAffect->lDuration);
		return;
	}

#ifdef ENABLE_SPAM_FILTER
	if (CSpamFilter::Instance().IsBannedWord(ch, buf))
	{
		return;
	}
#endif

#ifdef ACCOUNT_TRADE_BLOCK
	if (ch->GetDesc()->IsHWID2Banned())
	{
		ch->tchat("HWID 2 BANNED!");
		LogManager::instance().HackDetectionLog(ch, "HWID2BANNED_CHAT", buf);
	}
#endif

	char chatbuf[CHAT_MAX_LEN + 1];
	int len = 0;
	char colorbuf[11];
	
	switch(pinfo->type())
	{
		case CHAT_TYPE_PARTY:
			sprintf(colorbuf, "%s", "ff8afff1");
			break;
		case CHAT_TYPE_GUILD:
			sprintf(colorbuf, "%s", "ffe7d7ff");
			break;
		case CHAT_TYPE_SHOUT:
			sprintf(colorbuf, "%s", "ffa7ffd4");
			break;
		case CHAT_TYPE_TALKING:
			sprintf(colorbuf, "%s", "ffffffff");
			break;
	}
	if (pinfo->type() == CHAT_TYPE_SHOUT)
	{
		const std::string premiumChatColors[] = {
			"ffa7ffd4", // default(green)
			"ff32c5d8", // blue
			"ffd8ff0f", // yellow
			"ffff9400", // orange
			"ff1ede35", // green
		};

		quest::PC* mPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
		BYTE currColor = 0;
		if (mPC)
			currColor = MINMAX(0, mPC->GetFlag("premium.chat_color"), sizeof(premiumChatColors) / sizeof(std::string));
		if (currColor)
		{
			std::string sBuf = buf;
			size_t searchPos = sBuf.find("|r", 0);
			size_t replaceCount = 0;

			char mColorBuf[15];
			while (searchPos != std::string::npos && searchPos != sBuf.length() - 2) // if you link an item and write after that the color will reset
			{
				sprintf(mColorBuf, "|r|c%s|h", premiumChatColors[currColor].c_str());
				sBuf.replace(searchPos, 2, mColorBuf);

				++replaceCount;
				searchPos = sBuf.find("|r", searchPos + 2);
			}

			while (replaceCount)
			{
				sBuf += "|h";
				--replaceCount;
			}


			len = snprintf(chatbuf, sizeof(chatbuf), "%d |cFF98FF33|h[Lv %d] |c%s|h|Hqw:%s|h%s|h|r : |c%s|h%s|h", ch->GetLanguageID() + 1, ch->GetLevel(), colorbuf, ch->GetName(), ch->GetName(), premiumChatColors[currColor].c_str(), sBuf.c_str());
		}
		else
			len = snprintf(chatbuf, sizeof(chatbuf), "%d |cFF98FF33|h[Lv %d] |c%s|h|Hqw:%s|h%s|h|r : %s", ch->GetLanguageID() + 1, ch->GetLevel(), colorbuf, ch->GetName(), ch->GetName(), buf);
	}
	else
		len = snprintf(chatbuf, sizeof(chatbuf), "%d |cFF98FF33|h[Lv %d] |c%s|h|Hqw:%s|h%s|h|r : %s", ch->GetLanguageID()+1, ch->GetLevel(), colorbuf, ch->GetName(), ch->GetName(), buf);
	LogManager::instance().ShoutLog(g_bChannel, ch->GetEmpire(), chatbuf);
	

	// if (CHAT_TYPE_SHOUT == pinfo->type())
	// {
		// len = snprintf(chatbuf, sizeof(chatbuf), "%d |cFF98FF33|h[Lv %d] |cFFA7FFD4|h|Hqw:%s|h%s|h|r : %s", ch->GetLanguageID(), ch->GetLevel(), ch->GetName(), ch->GetName(), buf);
		// LogManager::instance().ShoutLog(g_bChannel, ch->GetEmpire(), chatbuf);
	// }
	// else
		// len = snprintf(chatbuf, sizeof(chatbuf), "%s : %s", ch->GetName(), buf);
	
	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	if (pinfo->type() == CHAT_TYPE_SHOUT)
	{
		const int SHOUT_LIMIT_LEVEL = 15;

		if (ch->GetLevel() < SHOUT_LIMIT_LEVEL)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to be over Level %d to use the shout chat."), SHOUT_LIMIT_LEVEL);
			return;
		}

		if (ch->GetRealPoint(POINT_PLAYTIME) < quest::CQuestManager::instance().GetEventFlag("shout_limit_time"))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to have at least %d minutes of playtime to use the shout chat."), quest::CQuestManager::instance().GetEventFlag("shout_limit_time"));
			return;
		}

		if (thecore_heart->pulse - (int) ch->GetLastShoutPulse() < passes_per_sec * 15 && ch->GetGMLevel() < GM_GOD)
			return;

		ch->SetLastShoutPulse(thecore_heart->pulse);

		quest::CQuestManager::instance().OnSendShout(ch->GetPlayerID());


#ifdef ACCOUNT_TRADE_BLOCK
		if (ch->GetDesc()->IsHWID2Banned())
		{
			if (pinfo->type() == CHAT_TYPE_SHOUT)
			{
				ch->ChatPacket(CHAT_TYPE_SHOUT, chatbuf);
				return;
			}
		}
#endif

		network::GGOutputPacket<network::GGShoutPacket> p;

		p->set_empire(g_bEmpireChat ? ch->GetEmpire() : 0);
		p->set_text(chatbuf);

		P2P_MANAGER::instance().Send(p);

		SendShout(chatbuf, g_bEmpireChat ? ch->GetEmpire() : 0);

		return;
	}

	network::GCOutputPacket<network::GCChatPacket> pack_chat;

	pack_chat->set_type(pinfo->type());
	pack_chat->set_id(ch->GetVID());
	pack_chat->set_message(chatbuf);

	switch (pinfo->type())
	{
		case CHAT_TYPE_TALKING:
			{
				const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
				
				std::for_each(c_ref_set.begin(), c_ref_set.end(), 
						FEmpireChatPacket(pack_chat,
							chatbuf,
							len, 
							(ch->GetGMLevel() > GM_PLAYER ||
							ch->IsEquipUniqueGroup(UNIQUE_GROUP_RING_OF_LANGUAGE) ||
							!g_bEmpireWhisper) ? 0 : ch->GetEmpire(), 
							ch->GetMapIndex(), strlen(ch->GetName())
#ifdef ACCOUNT_TRADE_BLOCK
							,ch->GetDesc()->IsHWID2Banned()
#endif
							)
						);
			}
			break;

		case CHAT_TYPE_PARTY:
			{
				if (!ch->GetParty())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÆÄÆ¼ ÁßÀÌ ¾Æ´Õ´Ï´Ù."));
				else
					ch->GetParty()->ForEachOnlineMember([&pack_chat](LPCHARACTER ch) {
						ch->GetDesc()->Packet(pack_chat);
					});
			}
			break;

		case CHAT_TYPE_GUILD:
			{
				if (!ch->GetGuild())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±æµå¿¡ °¡ÀÔÇÏÁö ¾Ê¾Ò½À´Ï´Ù."));
				else
					ch->GetGuild()->Chat(chatbuf);
			}
			break;

		case CHAT_TYPE_TEAM:
			{
				// p2p
				network::GGOutputPacket<network::GGTeamChatPacket> p;
				p->set_text(chatbuf);
				P2P_MANAGER::instance().Send(p);

				// this core
				const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

				std::for_each(c_ref_set.begin(), c_ref_set.end(), [&pack_chat](LPDESC d) {
					if (!d->GetCharacter() || !d->GetCharacter()->IsGM())
						return;

					d->Packet(pack_chat);
				});
			}
			break;

		default:
			sys_err("Unknown chat type %d", pinfo->type());
			break;
	}

	return;
}

void CInputMain::ItemUse(LPCHARACTER ch, std::unique_ptr<CGItemUsePacket> pack)
{
	ch->UseItem(pack->cell());
}

void CInputMain::ItemToItem(LPCHARACTER ch, std::unique_ptr<CGItemUseToItemPacket> p)
{
	if (ch)
		ch->UseItem(p->cell(), p->target_cell());
}

void CInputMain::ItemGive(LPCHARACTER ch, std::unique_ptr<CGGiveItemPacket> p)
{
	LPCHARACTER to_ch = CHARACTER_MANAGER::instance().Find(p->target_vid());

	if (to_ch)
		ch->GiveItem(to_ch, p->cell());
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾ÆÀÌÅÛÀ» °Ç³×ÁÙ ¼ö ¾ø½À´Ï´Ù."));
}

void CInputMain::ItemDrop(LPCHARACTER ch, std::unique_ptr<CGItemDropPacket> pack)
{
	if (!ch)
		return;

	// ¿¤Å©°¡ 0º¸´Ù Å©¸é ¿¤Å©¸¦ ¹ö¸®´Â °Í ÀÌ´Ù.
	if (pack->gold() > 0)
		ch->DropGold(pack->gold());
	else
		ch->DropItem(pack->cell(), pack->count());
}

void CInputMain::ItemMove(LPCHARACTER ch, std::unique_ptr<CGItemMovePacket> pack)
{
	if (ch)
		ch->MoveItem(pack->cell(), pack->cell_to(), pack->count());
}

void CInputMain::ItemPickup(LPCHARACTER ch, std::unique_ptr<CGItemPickupPacket> pack)
{
	if (ch)
		ch->PickupItem(pack->vid());
}

void CInputMain::QuickslotAdd(LPCHARACTER ch, std::unique_ptr<CGQuickslotAddPacket> pack)
{
	ch->SetQuickslot(pack->pos(), pack->slot());
}

void CInputMain::QuickslotDelete(LPCHARACTER ch, std::unique_ptr<CGQuickslotDeletePacket> pack)
{
	ch->DelQuickslot(pack->pos());
}

void CInputMain::QuickslotSwap(LPCHARACTER ch, std::unique_ptr<CGQuickslotSwapPacket> pack)
{
	ch->SwapQuickslot(pack->pos(), pack->change_pos());
}

void CInputMain::Messenger(LPCHARACTER ch, const InputPacket& p)
{
#ifdef __ANTI_FLOOD__	
	if (ch->IncreaseChatCounter() >= 30)
	{
		sys_log(0, "MessengerAuth_HACK: %s", ch->GetName());
		return;
	}
#endif

	switch (p.get_header<TCGHeader>())
	{
#ifdef ENABLE_MESSENGER_BLOCK
		case TCGHeader::MESSENGER_ADD_BLOCK_BY_VID:
			{
				auto p2 = p.get<CGMessengerAddBlockByVIDPacket>();
				LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid());

				if (!ch_companion)
					return;

				if (ch->IsObserverMode())
					return;

				LPDESC d = ch_companion->GetDesc();

				if (!d)
					return;

				if (ch_companion->GetGuild() == ch->GetGuild() && ch->GetGuild() != NULL && ch_companion->GetGuild() != NULL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block this player because he's in your guild. "));
					return;
				}
				
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_FRIEND))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block this player because he´s your friend."));
					return;
				}
				
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "This player is already blocked."));
					return;
				}
				
				if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block Game-Masters."));
					return;
				}

				if (ch->GetDesc() == d) // 자신은 추가할 수 없다.
					return;

				MessengerManager::instance().AddToBlockList(ch->GetName(), ch_companion->GetName());
			}
			return;

		case TCGHeader::MESSENGER_ADD_BLOCK_BY_NAME:
			{
				auto p2 = p.get<CGMessengerAddBlockByNamePacket>();

				if (GM::get_level(p2->name().c_str()) != GM_PLAYER && !test_server)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block Game-Master."));
					return;
				}

				LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(p2->name().c_str());
				
				if (!tch)
				{
					CCI * pkCCI = P2P_MANAGER::instance().Find(p2->name().c_str());

					if (!pkCCI)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s isn't near you."), p2->name().c_str());
						return;
					}

					CGuild *guild = ch->GetGuild();

					if (guild && guild->GetMemberPID(p2->name()))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block this player because he's in your guild. "));
						return;
					}

					if (MessengerManager::instance().CheckMessengerList(ch->GetName(), p2->name(), SYST_FRIEND))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block this player because he´s your friend."));
						return;
					}

					if (MessengerManager::instance().CheckMessengerList(ch->GetName(), p2->name(), SYST_BLOCK))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "This player is already blocked."));
						return;
					}

					if (ch->GetGMLevel() == GM_PLAYER && p2->name().c_str()[0] == '[')
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block Game-Masters."));
						return;
					}

					MessengerManager::instance().AddToBlockList(ch->GetName(), p2->name());
					return;
				}
				/*
				if (!tch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s isn't near you."), name);
				*/
				else
				{
					if (tch == ch) // 자신은 추가할 수 없다.
						return;
						
					if (tch->GetGuild() == ch->GetGuild() && ch->GetGuild() != NULL && tch->GetGuild() != NULL)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block this player because he's in your guild. "));
						return;
					}
					
					if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_FRIEND))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "This player isn't your friend."));
						return;
					}
						
					MessengerManager::instance().AddToBlockList(ch->GetName(), tch->GetName());
				}
			}
			return;

		case TCGHeader::MESSENGER_REMOVE_BLOCK:
			{
				auto p2 = p.get<CGMessengerRemoveBlockPacket>();

				if (!MessengerManager::instance().CheckMessengerList(ch->GetName(), p2->name(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't block this player.."));
					return;
				}
				MessengerManager::instance().RemoveFromBlockList(ch->GetName(), p2->name());
			}
			return;
#endif
		
		case TCGHeader::MESSENGER_ADD_BY_VID:
			{
				auto p2 = p.get<CGMessengerAddByVIDPacket>();
				LPCHARACTER ch_companion = CHARACTER_MANAGER::instance().Find(p2->vid());

				if (!ch_companion)
					return;

				if (ch->IsObserverMode())
					return;

				if (ch_companion->IsBlockMode(BLOCK_MESSENGER_INVITE))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»ó´ë¹æÀÌ ¸Þ½ÅÁ® Ãß°¡ °ÅºÎ »óÅÂÀÔ´Ï´Ù."));
					return;
				}

				LPDESC d = ch_companion->GetDesc();

				if (!d)
					return;

				if (ch->GetGMLevel() == GM_PLAYER && ch_companion->GetGMLevel() != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<¸Þ½ÅÁ®> ¿î¿µÀÚ´Â ¸Þ½ÅÁ®¿¡ Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}

				if (ch->GetDesc() == d) // ÀÚ½ÅÀº Ãß°¡ÇÒ ¼ö ¾ø´Ù.
					return;
					
#ifdef ENABLE_MESSENGER_BLOCK
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_FRIEND))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "This player is already your friend."));
					return;
				}
				
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), ch_companion->GetName(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't add this player as a friend because you've blocked it. "));
					return;
				}
#endif

				MessengerManager::instance().RequestToAdd(ch, ch_companion);
				//MessengerManager::instance().AddToList(ch->GetName(), ch_companion->GetName());
			}
			return;

		case TCGHeader::MESSENGER_ADD_BY_NAME:
			{
				auto p2 = p.get<CGMessengerAddByNamePacket>();

				if (ch->GetGMLevel() == GM_PLAYER && GM::get_level(p2->name().c_str()) != GM_PLAYER)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<¸Þ½ÅÁ®> ¿î¿µÀÚ´Â ¸Þ½ÅÁ®¿¡ Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}

				LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(p2->name().c_str());

				if (!tch)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "%s ´ÔÀº Á¢¼ÓµÇ ÀÖÁö ¾Ê½À´Ï´Ù."), p2->name().c_str());
				else
				{
					if (tch == ch) // 자신은 추가할 수 없다.
						return;

					if (tch->IsBlockMode(BLOCK_MESSENGER_INVITE) == true)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»ó´ë¹æÀÌ ¸Þ½ÅÁ® Ãß°¡ °ÅºÎ »óÅÂÀÔ´Ï´Ù."));
					}
					else
					{
#ifdef ENABLE_MESSENGER_BLOCK
						if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_FRIEND))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "This player is already blocked."));
							return;
						}
						
						if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_BLOCK))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't add this player as a friend because you've blocked it. "));
							return;
						}
#endif
						MessengerManager::instance().RequestToAdd(ch, tch);
					}
				}
			}
			return;

		case TCGHeader::MESSENGER_REMOVE:
			{
				auto p2 = p.get<CGMessengerRemovePacket>();

#ifdef ENABLE_MESSENGER_BLOCK
				if (!MessengerManager::instance().CheckMessengerList(ch->GetName(), p2->name(), SYST_FRIEND))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "This player isn't your friend."));
					return;
				}
#endif

				MessengerManager::instance().RemoveFromList(ch->GetName(), p2->name());
				MessengerManager::instance().RemoveFromList(p2->name(), ch->GetName());//friend removed from companion too.
			}
			return;

		default:
			sys_err("CInputMain::Messenger : Unknown subheader %d : %s", p.get_header(), ch->GetName());
			break;
	}
}

void CInputMain::Shop(LPCHARACTER ch, const InputPacket& p)
{
	if (test_server)
		sys_log(0, "CInputMain::Shop() ==> SubHeader %d", p.get_header());

	switch (p.get_header<TCGHeader>())
	{
		case TCGHeader::SHOP_END:
			sys_log(1, "INPUT: %s SHOP: END", ch->GetName());
			CShopManager::instance().StopShopping(ch);
			return;

		case TCGHeader::SHOP_BUY:
			{
				auto p2 = p.get<CGShopBuyPacket>();

				sys_log(1, "INPUT: %s SHOP: BUY %d", ch->GetName(), p2->pos());
				CShopManager::instance().Buy(ch, p2->pos());
				return;
			}

		case TCGHeader::SHOP_SELL:
			{
				auto p2 = p.get<CGShopSellPacket>();

				sys_log(0, "INPUT: %s SHOP: SELL", ch->GetName());
				CShopManager::instance().Sell(ch, p2->cell().cell(), p2->count());
				return;
			}

		default:
			sys_err("CInputMain::Shop : Unknown subheader %d : %s", p.get_header(), ch->GetName());
			break;
	}
}

enum EOnClickErrorIDs {
	ON_CLICK_ERROR_ID_SELECT_ITEM,
	ON_CLICK_ERROR_ID_GET_ITEM_SIZE,
	ON_CLICK_ERROR_ID_EMOTICON_CHECK,
	ON_CLICK_ERROR_ID_M2BOB_INIT,
	ON_CLICK_ERROR_ID_HIDDEN_ATTACK,
	ON_CLICK_ERROR_ID_NET_MODULE,
	ON_CLICK_ERROR_ID_PIXEL_POSITION,
	ON_CLICK_ERROR_ID_ITEM_MODULE,
	ON_CLICK_ERROR_ID_MAX_NUM,
};

void CInputMain::OnClick(LPCHARACTER ch, std::unique_ptr<CGOnClickPacket> pinfo)
{
	LPCHARACTER			victim;

	if (pinfo->vid() > UINT_MAX - ON_CLICK_ERROR_ID_MAX_NUM)
	{
		char szErrName[256];
		snprintf(szErrName, sizeof(szErrName), "unknown detect");

		int iErrID = UINT_MAX - pinfo->vid();
		switch (iErrID)
		{
			case ON_CLICK_ERROR_ID_SELECT_ITEM:
				snprintf(szErrName, sizeof(szErrName), "item.SelectItem");
				break;

			case ON_CLICK_ERROR_ID_GET_ITEM_SIZE:
				snprintf(szErrName, sizeof(szErrName), "item.GetItemSize");
				break;

			case ON_CLICK_ERROR_ID_EMOTICON_CHECK:
				snprintf(szErrName, sizeof(szErrName), "chrmgr.IsPossibleEmoticon | fishbubble_check");
				break;

			case ON_CLICK_ERROR_ID_M2BOB_INIT:
				snprintf(szErrName, sizeof(szErrName), "m2bob init LoginWindow");
				break;
				
			case ON_CLICK_ERROR_ID_HIDDEN_ATTACK:
				snprintf(szErrName, sizeof(szErrName), "hidden attack");
				break;
				
			case ON_CLICK_ERROR_ID_NET_MODULE:
				snprintf(szErrName, sizeof(szErrName), "net module manipulation");
				break;
				
			case ON_CLICK_ERROR_ID_ITEM_MODULE:
				snprintf(szErrName, sizeof(szErrName), "item module manipulation");
				break;
				
			case ON_CLICK_ERROR_ID_PIXEL_POSITION:
				snprintf(szErrName, sizeof(szErrName), "chr.GetPixelPosition");
				break;
		}

		if (test_server)
		{
			ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "!!! HackLog Detected: %s", szErrName);
			ch->ChatPacket(CHAT_TYPE_GUILD, "!!! HackLog Detected: %s", szErrName);
		}
		ch->DetectionHackLog(szErrName, "no detail given");
		return;
	}

	if ((victim = CHARACTER_MANAGER::instance().Find(pinfo->vid())))
		victim->OnClick(ch);
	else if (test_server)
	{
		sys_err("CInputMain::OnClick %s.Click.NOT_EXIST_VID[%d]", ch->GetName(), pinfo->vid());
	}
}

void CInputMain::OnHitSpacebar(LPCHARACTER ch)
{
	/*
		Additional Todo:
		check sLevel/3 == ch->GetLevel()
	*/	
	ch->SetLastSpacebarTime(get_global_time());
	// ch->tchat("HitSpacekey= %d", ch->GetLastSpacebarTime());
}

void CInputMain::OnQuestTrigger(LPCHARACTER ch, std::unique_ptr<CGOnQuestTriggerPacket> p)
{
	switch (p->index())
	{
		case QUEST_TRIGGER_GAYA:
		{
			ch->tchat("QUEST_TRIGGER_GAYA %d", p->arg());
			quest::CQuestManager::Instance().CraftGaya(ch->GetPlayerID(), p->arg());
			break;
		}
		case QUEST_TRIGGER_SOULS:
		{
			ch->tchat("QUEST_TRIGGER_SOULS %d", p->arg());
			quest::CQuestManager::Instance().SpendSouls(ch->GetPlayerID(), p->arg());
			break;
		}
		case QUEST_TRIGGER_SOCKS:
		{
			ch->tchat("QUEST_TRIGGER_SOCKS %d", p->arg());
			quest::CQuestManager::Instance().SpendXmasSocks(ch->GetPlayerID(), p->arg());
			break;
		}

		default:
			break;
	}
}

void CInputMain::Exchange(LPCHARACTER ch, const InputPacket& p)
{
	LPCHARACTER	to_ch = NULL;

	if (!ch->CanHandleItem())
		return;

	sys_log(0, "CInputMain()::Exchange()  SubHeader %d ", p.get_header());

	int iPulse = thecore_pulse(); 
	if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "°Å·¡ ÈÄ %dÃÊ ÀÌ³»¿¡ Ã¢°í¸¦ ¿­¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
		return;
	}

	switch (p.get_header<TCGHeader>())
	{
		case TCGHeader::EXCHANGE_START:
			if (!ch->GetExchange())
			{
				auto p2 = p.get<CGExchangeStartPacket>();

				if ((to_ch = CHARACTER_MANAGER::instance().Find(p2->other_vid())))
				{
					if (iPulse - ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ã¢°í¸¦ ¿¬ÈÄ %dÃÊ ÀÌ³»¿¡´Â °Å·¡¸¦ ÇÒ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);

						if (test_server)
							ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return; 
					}

					if (iPulse - to_ch->GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
					{
						to_ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(to_ch, "Ã¢°í¸¦ ¿¬ÈÄ %dÃÊ ÀÌ³»¿¡´Â °Å·¡¸¦ ÇÒ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);


						if (test_server)
							to_ch->ChatPacket(CHAT_TYPE_INFO, "[TestOnly][Safebox]Pulse %d LoadTime %d PASS %d", iPulse, to_ch->GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
						return; 
					}

					if (ch->GetGold() >= GOLD_MAX)
					{	
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¾×¼ö°¡ 20¾ï ³ÉÀ» ÃÊ°úÇÏ¿© °Å·¡¸¦ ÇÒ¼ö°¡ ¾ø½À´Ï´Ù.."));

						sys_err("[OVERFLOG_GOLD] START (%u) id %u name %s ", ch->GetGold(), ch->GetPlayerID(), ch->GetName());
						return;
					}

					if (to_ch->IsPC())
					{
						if (quest::CQuestManager::instance().GiveItemToPC(ch->GetPlayerID(), to_ch))
						{
							sys_log(0, "Exchange canceled by quest %s %s", ch->GetName(), to_ch->GetName());
							return;
						}
					}


					if (ch->GetMyShop() || ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´Ù¸¥ °Å·¡ÁßÀÏ°æ¿ì °³ÀÎ»óÁ¡À» ¿­¼ö°¡ ¾ø½À´Ï´Ù."));
						return;
					}

#ifdef __ACCE_COSTUME__
					if (ch->IsAcceWindowOpen())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´Ù¸¥ °Å·¡ÁßÀÏ°æ¿ì °³ÀÎ»óÁ¡À» ¿­¼ö°¡ ¾ø½À´Ï´Ù."));
						return;
					}
#endif

					ch->ExchangeStart(to_ch);
				}
			}
			break;
			
		case TCGHeader::EXCHANGE_ITEM_ADD:
			if (ch->GetExchange())
			{
				auto p2 = p.get<CGExchangeItemAddPacket>();
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddItem(p2->cell(), p2->display_pos());
			}
			break;

		case TCGHeader::EXCHANGE_ITEM_DEL:
			if (ch->GetExchange())
			{
				auto p2 = p.get<CGExchangeItemDelPacket>();
				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->RemoveItem(p2->display_pos());
			}
			break;

		case TCGHeader::EXCHANGE_GOLD_ADD:
			if (ch->GetExchange())
			{
				auto p2 = p.get<CGExchangeGoldAddPacket>();
				const int64_t nTotalGold = static_cast<int64_t>(ch->GetExchange()->GetCompany()->GetOwner()->GetGold()) + static_cast<int64_t>(p2->gold());

				if (GOLD_MAX <= nTotalGold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "»ó´ë¹æÀÇ ÃÑ±Ý¾×ÀÌ 20¾ï ³ÉÀ» ÃÊ°úÇÏ¿© °Å·¡¸¦ ÇÒ¼ö°¡ ¾ø½À´Ï´Ù.."));

					sys_err("[OVERFLOW_GOLD] ELK_ADD (%u) id %u name %s ",
							ch->GetExchange()->GetCompany()->GetOwner()->GetGold(),
							ch->GetExchange()->GetCompany()->GetOwner()->GetPlayerID(),
						   	ch->GetExchange()->GetCompany()->GetOwner()->GetName());

					return;
				}

				if (ch->GetExchange()->GetCompany()->GetAcceptStatus() != true)
					ch->GetExchange()->AddGold(p2->gold());
			}
			break;

		case TCGHeader::EXCHANGE_ACCEPT:
			if (ch->GetExchange())
			{
				sys_log(0, "CInputMain()::Exchange() ==> ACCEPT "); 
				ch->GetExchange()->Accept(true);
			}

			break;

		case TCGHeader::EXCHANGE_CANCEL:
			if (ch->GetExchange())
				ch->GetExchange()->Cancel();
			break;
	}
}

static const int ComboSequenceBySkillLevel[3][8] = 
{
	// 0   1   2   3   4   5   6   7
	{ 14, 15, 16, 17,  0,  0,  0,  0 },
	{ 14, 15, 16, 18, 20,  0,  0,  0 },
	{ 14, 15, 16, 18, 19, 17,  0,  0 },
};

#define COMBO_HACK_ALLOWABLE_MS	100

#ifdef __WOLFMAN__
DWORD CalcValidComboInterval(LPCHARACTER ch, BYTE bArg)
{
	int nInterval = 300;
	float fAdjustNum = 1.5f;

	if (!ch)
	{
		sys_err("ClacValidComboInterval() ch is NULL");
		return nInterval;
	}

	if (bArg == 13)
	{
		float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration(ch->GetRaceNum());
		nInterval = (int)(normalAttackDuration / (((float)ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f) + fAdjustNum);
	}
	else if (bArg == 14)
	{
		nInterval = (int)(ani_combo_speed(ch, 1) / ((ch->GetPoint(POINT_ATT_SPEED) / 100.f) + fAdjustNum));
	}
	else if (bArg > 14 && bArg << 22)
	{
		nInterval = (int)(ani_combo_speed(ch, bArg - 13) / ((ch->GetPoint(POINT_ATT_SPEED) / 100.f) + fAdjustNum));
	}
	else
	{
		sys_err("ClacValidComboInterval() Invalid bArg(%d) ch(%s)", bArg, ch->GetName());
	}

	return nInterval;
}
#endif

bool CheckComboHack(LPCHARACTER ch, UINT uArg, DWORD dwTime, bool CheckSpeedHack)
{
	//	Á×°Å³ª ±âÀý »óÅÂ¿¡¼­´Â °ø°ÝÇÒ ¼ö ¾øÀ¸¹Ç·Î, skipÇÑ´Ù.
	//	ÀÌ·¸°Ô ÇÏÁö ¸»°í, CHRACTER::CanMove()¿¡ 
	//	if (IsStun() || IsDead()) return false;
	//	¸¦ Ãß°¡ÇÏ´Â°Ô ¸Â´Ù°í »ý°¢ÇÏ³ª,
	//	ÀÌ¹Ì ´Ù¸¥ ºÎºÐ¿¡¼­ CanMove()´Â IsStun(), IsDead()°ú
	//	µ¶¸³ÀûÀ¸·Î Ã¼Å©ÇÏ°í ÀÖ±â ¶§¹®¿¡ ¼öÁ¤¿¡ ÀÇÇÑ ¿µÇâÀ»
	//	ÃÖ¼ÒÈ­ÇÏ±â À§ÇØ ÀÌ·¸°Ô ¶«»§ ÄÚµå¸¦ ½á³õ´Â´Ù.
	if (ch->IsStun() || ch->IsDead())
		return false;
	int ComboInterval = dwTime - ch->GetLastComboTime();
	int HackScalar = 0; // ±âº» ½ºÄ®¶ó ´ÜÀ§ 1
#if 0	
	sys_log(0, "COMBO: %s arg:%u seq:%u delta:%d checkspeedhack:%d",
			ch->GetName(), bArg, ch->GetComboSequence(), ComboInterval - ch->GetValidComboInterval(), CheckSpeedHack);
#endif
	// bArg 14 ~ 21¹ø ±îÁö ÃÑ 8ÄÞº¸ °¡´É
	// 1. Ã¹ ÄÞº¸(14)´Â ÀÏÁ¤ ½Ã°£ ÀÌÈÄ¿¡ ¹Ýº¹ °¡´É
	// 2. 15 ~ 21¹øÀº ¹Ýº¹ ºÒ°¡´É
	// 3. Â÷·Ê´ë·Î Áõ°¡ÇÑ´Ù.
	if (uArg == 14)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			// FIXME Ã¹¹øÂ° ÄÞº¸´Â ÀÌ»óÇÏ°Ô »¡¸® ¿Ã ¼ö°¡ ÀÖ¾î¼­ 300À¸·Î ³ª´® -_-;
			// ´Ù¼öÀÇ ¸ó½ºÅÍ¿¡ ÀÇÇØ ´Ù¿îµÇ´Â »óÈ²¿¡¼­ °ø°ÝÀ» ÇÏ¸é
			// Ã¹¹øÂ° ÄÞº¸°¡ ¸Å¿ì ÀûÀº ÀÎÅÍ¹ú·Î µé¾î¿À´Â »óÈ² ¹ß»ý.
			// ÀÌ·Î ÀÎÇØ ÄÞº¸ÇÙÀ¸·Î Æ¨±â´Â °æ¿ì°¡ ÀÖ¾î ´ÙÀ½ ÄÚµå ºñ È°¼ºÈ­.
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 300;

			//sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
			//		ch->GetName(),
			//		bArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED),
			//		ch->IsRiding() ? "yes" : "no");
		}

		ch->SetComboSequence(1);
#ifdef __WOLFMAN__
		ch->SetValidComboInterval(CalcValidComboInterval(ch, uArg));
#else
		ch->SetValidComboInterval((int) (ani_combo_speed(ch, 1) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
#endif
		ch->SetLastComboTime(dwTime);
	}
	else if (uArg > 14 && uArg < 22)
	{
		int idx = MIN(2, ch->GetComboIndex());

		if (ch->GetComboSequence() > 5) // ÇöÀç 6ÄÞº¸ ÀÌ»óÀº ¾ø´Ù.
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);
			sys_log(0, "COMBO_HACK: 5 %s combo_seq:%d", ch->GetName(), ch->GetComboSequence());
		}
		// ÀÚ°´ ½Ö¼ö ÄÞº¸ ¿¹¿ÜÃ³¸®
		else if (uArg == 21 &&
				 idx == 2 &&
				 ch->GetComboSequence() == 5 &&
				 ch->GetJob() == JOB_ASSASSIN &&
				 ch->GetWear(WEAR_WEAPON) &&
				 ch->GetWear(WEAR_WEAPON)->GetSubType() == WEAPON_DAGGER)
			ch->SetValidComboInterval(300);
		else if (ComboSequenceBySkillLevel[idx][ch->GetComboSequence()] != uArg)
		{
			HackScalar = 1;
			ch->SetValidComboInterval(300);

			sys_log(0, "COMBO_HACK: 3 %s arg:%u valid:%u combo_idx:%d combo_seq:%d",
					ch->GetName(),
					uArg,
					ComboSequenceBySkillLevel[idx][ch->GetComboSequence()],
					idx,
					ch->GetComboSequence());
		}
		else
		{
			if (CheckSpeedHack && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
			{
				HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

				sys_log(0, "COMBO_HACK: 2 %s arg:%u interval:%d valid:%u atkspd:%u riding:%s",
						ch->GetName(),
						uArg,
						ComboInterval,
						ch->GetValidComboInterval(),
						ch->GetPoint(POINT_ATT_SPEED),
						ch->IsRiding() ? "yes" : "no");
			}

			// ¸»À» ÅÀÀ» ¶§´Â 15¹ø ~ 16¹øÀ» ¹Ýº¹ÇÑ´Ù
			//if (ch->IsHorseRiding())
			if (ch->IsRiding())
				ch->SetComboSequence(ch->GetComboSequence() == 1 ? 2 : 1);
			else
				ch->SetComboSequence(ch->GetComboSequence() + 1);

#ifdef __WOLFMAN__
			ch->SetValidComboInterval(CalcValidComboInterval(ch, uArg));
#else
			ch->SetValidComboInterval((int)(ani_combo_speed(ch, uArg - 13) / (ch->GetPoint(POINT_ATT_SPEED) / 100.f)));
#endif
			ch->SetLastComboTime(dwTime);
		}
	}
	else if (uArg == 13) // ±âº» °ø°Ý (µÐ°©(Polymorph)ÇßÀ» ¶§ ¿Â´Ù)
	{
		if (CheckSpeedHack && ComboInterval > 0 && ComboInterval < ch->GetValidComboInterval() - COMBO_HACK_ALLOWABLE_MS)
		{
			// ´Ù¼öÀÇ ¸ó½ºÅÍ¿¡ ÀÇÇØ ´Ù¿îµÇ´Â »óÈ²¿¡¼­ °ø°ÝÀ» ÇÏ¸é
			// Ã¹¹øÂ° ÄÞº¸°¡ ¸Å¿ì ÀûÀº ÀÎÅÍ¹ú·Î µé¾î¿À´Â »óÈ² ¹ß»ý.
			// ÀÌ·Î ÀÎÇØ ÄÞº¸ÇÙÀ¸·Î Æ¨±â´Â °æ¿ì°¡ ÀÖ¾î ´ÙÀ½ ÄÚµå ºñ È°¼ºÈ­.
			//HackScalar = 1 + (ch->GetValidComboInterval() - ComboInterval) / 100;

			//sys_log(0, "COMBO_HACK: 6 %s arg:%u interval:%d valid:%u atkspd:%u",
			//		ch->GetName(),
			//		uArg,
			//		ComboInterval,
			//		ch->GetValidComboInterval(),
			//		ch->GetPoint(POINT_ATT_SPEED));
		}

		if (ch->GetRaceNum() >= MAIN_RACE_MAX_NUM)
		{
			// POLYMORPH_BUG_FIX
			
			// DELETEME
			/*
			const CMotion * pkMotion = CMotionManager::instance().GetMotion(ch->GetRaceNum(), MAKE_MOTION_KEY(MOTION_MODE_GENERAL, MOTION_NORMAL_ATTACK));

			if (!pkMotion)
				sys_err("cannot find motion by race %u", ch->GetRaceNum());
			else
			{
				// Á¤»óÀû °è»êÀÌ¶ó¸é 1000.f¸¦ °öÇØ¾ß ÇÏÁö¸¸ Å¬¶óÀÌ¾ðÆ®°¡ ¾Ö´Ï¸ÞÀÌ¼Ç ¼ÓµµÀÇ 90%¿¡¼­
				// ´ÙÀ½ ¾Ö´Ï¸ÞÀÌ¼Ç ºí·»µùÀ» Çã¿ëÇÏ¹Ç·Î 900.f¸¦ °öÇÑ´Ù.
				int k = (int) (pkMotion->GetDuration() / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
				ch->SetValidComboInterval(k);
				ch->SetLastComboTime(dwTime);
			}
			*/
#ifdef __WOLFMAN__
			ch->SetValidComboInterval(CalcValidComboInterval(ch, uArg));
#else
			float normalAttackDuration = CMotionManager::instance().GetNormalAttackDuration(ch->GetRaceNum());
			int k = (int) (normalAttackDuration / ((float) ch->GetPoint(POINT_ATT_SPEED) / 100.f) * 900.f);
			ch->SetValidComboInterval(k);
#endif
			ch->SetLastComboTime(dwTime);
			// END_OF_POLYMORPH_BUG_FIX
		}
		else
		{
			// ¸»ÀÌ ¾ÈµÇ´Â ÄÞº¸°¡ ¿Ô´Ù ÇØÄ¿ÀÏ °¡´É¼º?
			//if (ch->GetDesc()->DelayedDisconnect(random_number(2, 9)))
			//{
			//	LogManager::instance().HackLog("Hacker", ch);
			//	sys_log(0, "HACKER: %s arg %u", ch->GetName(), uArg);
			//}

			// À§ ÄÚµå·Î ÀÎÇØ, Æú¸®¸ðÇÁ¸¦ Çª´Â Áß¿¡ °ø°Ý ÇÏ¸é,
			// °¡²û ÇÙÀ¸·Î ÀÎ½ÄÇÏ´Â °æ¿ì°¡ ÀÖ´Ù.

			// ÀÚ¼¼È÷ ¸»Çô¸é,
			// ¼­¹ö¿¡¼­ poly 0¸¦ Ã³¸®ÇßÁö¸¸,
			// Å¬¶ó¿¡¼­ ±× ÆÐÅ¶À» ¹Þ±â Àü¿¡, ¸÷À» °ø°Ý. <- Áï, ¸÷ÀÎ »óÅÂ¿¡¼­ °ø°Ý.
			//
			// ±×·¯¸é Å¬¶ó¿¡¼­´Â ¼­¹ö¿¡ ¸÷ »óÅÂ·Î °ø°ÝÇß´Ù´Â Ä¿¸Çµå¸¦ º¸³»°í (arg == 13)
			//
			// ¼­¹ö¿¡¼­´Â race´Â ÀÎ°£ÀÎµ¥ °ø°ÝÇüÅÂ´Â ¸÷ÀÎ ³ðÀÌ´Ù! ¶ó°í ÇÏ¿© ÇÙÃ¼Å©¸¦ Çß´Ù.

			// »ç½Ç °ø°Ý ÆÐÅÏ¿¡ ´ëÇÑ °ÍÀº Å¬¶óÀÌ¾ðÆ®¿¡¼­ ÆÇ´ÜÇØ¼­ º¸³¾ °ÍÀÌ ¾Æ´Ï¶ó,
			// ¼­¹ö¿¡¼­ ÆÇ´ÜÇØ¾ß ÇÒ °ÍÀÎµ¥... ¿Ö ÀÌ·¸°Ô ÇØ³ùÀ»±î...
			// by rtsummit
		}
	}
	else
	{
		// ¸»ÀÌ ¾ÈµÇ´Â ÄÞº¸°¡ ¿Ô´Ù ÇØÄ¿ÀÏ °¡´É¼º?
		if (ch->GetDesc()->DelayedDisconnect(random_number(2, 9)))
		{
			LogManager::instance().HackLog("Hacker", ch);
			sys_log(0, "HACKER: %s arg %u", ch->GetName(), uArg);
		}

		HackScalar = 10;
		ch->SetValidComboInterval(300);
	}

	if (HackScalar)
	{
		// ¸»¿¡ Å¸°Å³ª ³»·ÈÀ» ¶§ 1.5ÃÊ°£ °ø°ÝÀº ÇÙÀ¸·Î °£ÁÖÇÏÁö ¾ÊµÇ °ø°Ý·ÂÀº ¾ø°Ô ÇÏ´Â Ã³¸®
		if (get_dword_time() - ch->GetLastMountTime() > 1500)
			ch->IncreaseComboHackCount(1 + HackScalar);

		ch->SkipComboAttackByTime(ch->GetValidComboInterval());
	}

	return HackScalar;
}

void CInputMain::Move(LPCHARACTER ch, std::unique_ptr<CGMovePacket> data)
{
	if (!ch->CanMove())
		return;

	BYTE bFunc = data->func();
	if (bFunc >= FUNC_MAX_NUM && !(bFunc & 0x80))
	{
		sys_err("invalid move type: %s", ch->GetName());
		return;
	}

	//enum EMoveFuncType
	//{   
	//	FUNC_WAIT,
	//	FUNC_MOVE,
	//	FUNC_ATTACK,
	//	FUNC_COMBO,
	//	FUNC_MOB_SKILL,
	//	_FUNC_SKILL,
	//	FUNC_MAX_NUM,
	//	FUNC_SKILL = 0x80,
	//};  

	// ÅÚ·¹Æ÷Æ® ÇÙ Ã¼Å©

//	if (!test_server)	//2012.05.15 ±è¿ë¿í : Å×¼·¿¡¼­ (¹«Àû»óÅÂ·Î) ´Ù¼ö ¸ó½ºÅÍ »ó´ë·Î ´Ù¿îµÇ¸é¼­ °ø°Ý½Ã ÄÞº¸ÇÙÀ¸·Î Á×´Â ¹®Á¦°¡ ÀÖ¾ú´Ù.
	{
		
// #ifdef __ANTI_CHEAT_FIXES__
		// if (ch->IsPC() && ch->IsDead()) {
			// if (ch->GetDesc()) {
				// LogManager::instance().HackLog("GHOSTMODE﻿", ch);
				// return;
			// }
		// }﻿﻿﻿ 
// #endif

		const float fDist = DISTANCE_SQRT((ch->GetX() - data->x()) / 100, (ch->GetY() - data->y()) / 100);

		if (((!ch->IsRiding() && fDist > 50) || fDist > 120) && OXEVENT_MAP_INDEX != ch->GetMapIndex())
		{
			char buf[256];
			sprintf(buf, "MOVE: %s trying to move too far (dist: %.1fm) Riding(%d)", ch->GetName(), fDist, ch->IsRiding());
			sys_log(0, buf);

			ch->DetectionHackLog("MOVE_SPEED_HACK", buf);
			// ch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
			// ch->Stop();
			// return;
		}
#ifndef __DISABLE_SPEEDHACK_CHECK__
		//
		// ½ºÇÇµåÇÙ(SPEEDHACK) Check
		//
		DWORD dwCurTime = get_dword_time();
		// ½Ã°£À» SyncÇÏ°í 7ÃÊ ÈÄ ºÎÅÍ °Ë»çÇÑ´Ù. (20090702 ÀÌÀü¿£ 5ÃÊ¿´À½)
		bool CheckSpeedHack = (false == ch->GetDesc()->IsHandshaking() && dwCurTime - ch->GetDesc()->GetClientTime() > 7000);

		if (CheckSpeedHack)
		{
			int iDelta = (int) (pinfo->dwTime - ch->GetDesc()->GetClientTime());
			int iServerDelta = (int) (dwCurTime - ch->GetDesc()->GetClientTime());

			iDelta = (int) (dwCurTime - pinfo->dwTime);

			// ½Ã°£ÀÌ ´Ê°Ô°£´Ù. ÀÏ´Ü ·Î±×¸¸ ÇØµÐ´Ù. ÁøÂ¥ ÀÌ·± »ç¶÷µéÀÌ ¸¹ÀºÁö Ã¼Å©ÇØ¾ßÇÔ. TODO
			if (iDelta >= 30000)
			{
				sys_log(0, "SPEEDHACK: slow timer name %s delta %d", ch->GetName(), iDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
			// 1ÃÊ¿¡ 20msec »¡¸® °¡´Â°Å ±îÁö´Â ÀÌÇØÇÑ´Ù.
			else if (iDelta < -(iServerDelta / 50))
			{
				sys_log(0, "SPEEDHACK: DETECTED! %s (delta %d %d)", ch->GetName(), iDelta, iServerDelta);
				ch->GetDesc()->DelayedDisconnect(3);
			}
		}

		//
		// ÄÞº¸ÇÙ ¹× ½ºÇÇµåÇÙ Ã¼Å©
		//
		if (pinfo->bFunc == FUNC_COMBO && g_bCheckMultiHack)
		{
			CheckComboHack(ch, pinfo->uArg, pinfo->dwTime, CheckSpeedHack); // ÄÞº¸ Ã¼Å©
		}
#endif
	}
	
	if (data->func() == FUNC_MOVE)
	{
		if (ch->GetLimitPoint(POINT_MOV_SPEED) == 0)
			return;

		ch->SetRotation(data->rot() * 5);	// Áßº¹ ÄÚµå
		ch->ResetStopTime();				// ""

		ch->Goto(data->x(), data->y());
	}
	else
	{
		if (data->func() == FUNC_ATTACK || data->func() == FUNC_COMBO)
		{
			ch->OnMove(true, true);

#ifdef ENABLE_RUNE_SYSTEM
			CRuneManager::instance().OnMeleeAttack(ch);
#endif
		}
		else if (data->func() == FUNC_SKILL)
		{
			// const int MASK_SKILL_MOTION = 0x7F;
			// unsigned int motion = pinfo->bFunc & MASK_SKILL_MOTION;

			/*if (!ch->IsUsableSkillMotion(motion))
			{
				const char* name = ch->GetName();
				unsigned int job = ch->GetJob();
				unsigned int group = ch->GetSkillGroup();

				char szBuf[256];
				snprintf(szBuf, sizeof(szBuf), "SKILL_HACK: name=%s, job=%d, group=%d, motion=%d", name, job, group, motion);
				LogManager::instance().HackLog(szBuf, ch->GetDesc()->GetAccountTable().login().c_str(), ch->GetName(), ch->GetDesc()->GetHostName());
				sys_log(0, "%s", szBuf);

				if (test_server)
				{
					ch->GetDesc()->DelayedDisconnect(random_number(2, 8));
					ch->ChatPacket(CHAT_TYPE_INFO, szBuf);
				}
				else
				{
					ch->GetDesc()->DelayedDisconnect(random_number(150, 500));
				}
			}*/

			ch->OnMove();
		}

		ch->SetRotation(data->rot() * 5);	// Áßº¹ ÄÚµå
		ch->ResetStopTime();				// ""

		ch->Move(data->x(), data->y());
		ch->Stop();
		ch->StopStaminaConsume();
	}

	network::GCOutputPacket<network::GCMovePacket> pack;

	pack->set_func(data->func());
	pack->set_arg(data->arg());
	pack->set_rot(data->rot());
	pack->set_vid(ch->GetVID());
	pack->set_x(data->x());
	pack->set_y(data->y());
	pack->set_time(data->time());
	pack->set_duration((data->func() == FUNC_MOVE) ? ch->GetCurrentMoveDuration() : 0);

	ch->PacketAround(pack, ch);
/*
	if (pinfo->dwTime == 10653691) // µð¹ö°Å ¹ß°ß
	{
		if (ch->GetDesc()->DelayedDisconnect(random_number(15, 30)))
			LogManager::instance().HackLog("Debugger", ch);

	}
	else if (pinfo->dwTime == 10653971) // Softice ¹ß°ß
	{
		if (ch->GetDesc()->DelayedDisconnect(random_number(15, 30)))
			LogManager::instance().HackLog("Softice", ch);
	}
*/
}

bool check_attack_skill(LPCHARACTER ch, TCGHeader header, BYTE type)
{
	if (type > 0)
	{
		if (!ch->CanUseSkill(type))
			return false;;

		switch (type)
		{
			case SKILL_GEOMPUNG:
			case SKILL_SANGONG:
			case SKILL_YEONSA:
			case SKILL_KWANKYEOK:
			case SKILL_HWAJO:
			case SKILL_GIGUNG:
			case SKILL_PABEOB:
			case SKILL_MARYUNG:
			case SKILL_TUSOK:
			case SKILL_MAHWAN:
			case SKILL_BIPABU:
			case SKILL_NOEJEON:
			case SKILL_CHAIN:
			case SKILL_HORSE_WILDATTACK_RANGE:
				if (TCGHeader::SHOOT != header)
				{
					if (test_server) 
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Attack :name[%s] Vnum[%d] can't use skill by attack(warning)"), type);
					return false;
				}
				break;
		}
	}

	return true;
}

void CInputMain::Attack(LPCHARACTER ch, const InputPacket& data)
{
	if (!ch || !ch->GetDesc())
		return;

	auto header = data.get_header<TCGHeader>();
	switch (header)
	{
		case TCGHeader::ATTACK:
			{
				auto packMelee = data.get<CGAttackPacket>();
				if (!check_attack_skill(ch, header, packMelee->type()))
					return;

				BYTE bRandom = packMelee->random();
				if (bRandom != ch->GetRandom() + 4)
				{
					ch->SetIsHacker();
					ch->DetectionHackLog("ATTACK_PACKET_WRONG", "");
					ch->GetDesc()->DelayedDisconnect(5);
					return;
				}
		
				LPCHARACTER	victim = CHARACTER_MANAGER::instance().Find(packMelee->vid());
				if (NULL == victim || ch == victim)
					return;

				switch (victim->GetCharType())
				{
					case CHAR_TYPE_NPC:
					case CHAR_TYPE_WARP:
					case CHAR_TYPE_GOTO:
						return;
				}

				if (packMelee->type() > 0)
				{
					if (false == ch->CheckSkillHitCount(packMelee->type(), victim->GetVID()))
						return;
				}
				ch->Attack(victim, packMelee->type());
			}
			break;

		case TCGHeader::SHOOT:
			{
				auto packShoot = data.get<CGShootPacket>();
				if (!check_attack_skill(ch, header, packShoot->type()))
					return;

				ch->Shoot(packShoot->type());
			}
			break;
	}
}

void CInputMain::SyncPosition(LPCHARACTER ch, std::unique_ptr<CGSyncPositionPacket> pinfo)
{
	static const int nCountLimit = 16;

	int iCount = pinfo->elements_size();
	if (iCount > nCountLimit)
	{
		//LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );
		sys_err("Too many SyncPosition Count(%d) from Name(%s)", iCount, ch->GetName());
		//ch->GetDesc()->SetPhase(PHASE_CLOSE);
		//return -1;
		iCount = nCountLimit;
	}

	GCOutputPacket<GCSyncPositionPacket> pack;

	timeval tvCurTime;
	gettimeofday(&tvCurTime, NULL);

	//PIXEL_POSITION base_pos;
	//SECTREE_MANAGER::Instance().GetMapBasePositionByMapIndex(ch->GetMapIndex(), base_pos);

	for (int i = 0; i < iCount; ++i)
	{
		auto& e = pinfo->elements(i);

		long sync_x = e.x();// + base_pos.x;
		long sync_y = e.y();// + base_pos.y;

		LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(e.vid());

		if (!victim)
			continue;

		switch (victim->GetCharType())
		{
			case CHAR_TYPE_NPC:
			case CHAR_TYPE_WARP:
			case CHAR_TYPE_GOTO:
				continue;
		}

		// ¼ÒÀ¯±Ç °Ë»ç
		if (!victim->SetSyncOwner(ch))
			continue;

		const float fDistWithSyncOwner = DISTANCE_SQRT( (victim->GetX() - ch->GetX()) / 100, (victim->GetY() - ch->GetY()) / 100 );
		static const float fLimitDistWithSyncOwner = 2500.f + 1000.f;
		// victim°úÀÇ °Å¸®°¡ 2500 + a ÀÌ»óÀÌ¸é ÇÙÀ¸·Î °£ÁÖ.
		//	°Å¸® ÂüÁ¶ : Å¬¶óÀÌ¾ðÆ®ÀÇ __GetSkillTargetRange, __GetBowRange ÇÔ¼ö
		//	2500 : ½ºÅ³ proto¿¡¼­ °¡Àå »ç°Å¸®°¡ ±ä ½ºÅ³ÀÇ »ç°Å¸®, ¶Ç´Â È°ÀÇ »ç°Å¸®
		//	a = POINT_BOW_DISTANCE °ª... ÀÎµ¥ ½ÇÁ¦·Î »ç¿ëÇÏ´Â °ªÀÎÁö´Â Àß ¸ð¸£°ÚÀ½. ¾ÆÀÌÅÛÀÌ³ª Æ÷¼Ç, ½ºÅ³, Äù½ºÆ®¿¡´Â ¾ø´Âµ¥...
		//		±×·¡µµ È¤½Ã³ª ÇÏ´Â ¸¶À½¿¡ ¹öÆÛ·Î »ç¿ëÇÒ °âÇØ¼­ 1000.f ·Î µÒ...
		if (fDistWithSyncOwner > fLimitDistWithSyncOwner)
		{
			// g_iSyncHackLimitCount¹ø ±îÁö´Â ºÁÁÜ.
			//if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			//{
			//	ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
			//	continue;
			//}
			//else
			//{
				LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );
				
				sys_err( "Too far SyncPosition DistanceWithSyncOwner(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
					fDistWithSyncOwner, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
					sync_x, sync_y);

			//	ch->GetDesc()->SetPhase(PHASE_CLOSE);

			//	return;
			//}
		}
		
		const float fDist = DISTANCE_SQRT( (victim->GetX() - sync_x) / 100, (victim->GetY() - sync_y) / 100 );
		static const long g_lValidSyncInterval = 100 * 1000; // 100ms
		const timeval &tvLastSyncTime = victim->GetLastSyncTime();
		timeval *tvDiff = timediff(&tvCurTime, &tvLastSyncTime);
		
		// SyncPositionÀ» ¾Ç¿ëÇÏ¿© Å¸À¯Àú¸¦ ÀÌ»óÇÑ °÷À¸·Î º¸³»´Â ÇÙ ¹æ¾îÇÏ±â À§ÇÏ¿©,
		// °°Àº À¯Àú¸¦ g_lValidSyncInterval ms ÀÌ³»¿¡ ´Ù½Ã SyncPositionÇÏ·Á°í ÇÏ¸é ÇÙÀ¸·Î °£ÁÖ.
		if (tvDiff->tv_sec == 0 && tvDiff->tv_usec < g_lValidSyncInterval)
		{
			// g_iSyncHackLimitCount¹ø ±îÁö´Â ºÁÁÜ.
			if (ch->GetSyncHackCount() < g_iSyncHackLimitCount)
			{
				ch->SetSyncHackCount(ch->GetSyncHackCount() + 1);
				continue;
			}
			else
			{
				LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

				sys_err( "Too often SyncPosition Interval(%ldms)(%s) from Name(%s) VICTIM(%d,%d) SYNC(%d,%d)",
					tvDiff->tv_sec * 1000 + tvDiff->tv_usec / 1000, victim->GetName(), ch->GetName(), victim->GetX(), victim->GetY(),
					sync_x, sync_y);

				ch->GetDesc()->SetPhase(PHASE_CLOSE);

				return;
			}
		}
		else if( fDist > 25.0f )
		{
			LogManager::instance().HackLog( "SYNC_POSITION_HACK", ch );

			sys_err( "Too far SyncPosition Distance(%f)(%s) from Name(%s) CH(%d,%d) VICTIM(%d,%d) SYNC(%d,%d)",
				   	fDist, victim->GetName(), ch->GetName(), ch->GetX(), ch->GetY(), victim->GetX(), victim->GetY(),
				  sync_x, sync_y);

			ch->GetDesc()->SetPhase(PHASE_CLOSE);

			return;
		}
		else
		{
			victim->SetLastSyncTime(tvCurTime);
			victim->Sync(sync_x, sync_y);

			auto new_elem = pack->add_elements();
			*new_elem = e;
			new_elem->set_x(sync_x);
			new_elem->set_y(sync_y);
		}
	}

	if (pack->elements_size() > 0)
	{
		ch->PacketAround(pack, ch);
	}

	return;
}

void CInputMain::FlyTarget(LPCHARACTER ch, std::unique_ptr<CGFlyTargetPacket> p)
{
	ch->FlyTarget(p->target_vid(), p->x(), p->y(), false);
}

void CInputMain::AddFlyTarget(LPCHARACTER ch, std::unique_ptr<CGAddFlyTargetPacket> p)
{
	ch->FlyTarget(p->target_vid(), p->x(), p->y(), true);
}

void CInputMain::UseSkill(LPCHARACTER ch, std::unique_ptr<CGUseSkillPacket> p)
{
	ch->UseSkill(p->vnum(), CHARACTER_MANAGER::instance().Find(p->vid()));
}

void CInputMain::ScriptButton(LPCHARACTER ch, std::unique_ptr<CGScriptButtonPacket> p)
{
	sys_log(0, "QUEST ScriptButton pid %d idx %u", ch->GetPlayerID(), p->index());

	quest::PC* pc = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pc && pc->IsConfirmWait())
	{
		quest::CQuestManager::instance().Confirm(ch->GetPlayerID(), quest::CONFIRM_TIMEOUT);
	}
	else if (p->index() & 0x80000000)
	{
		quest::CQuestManager::Instance().QuestInfo(ch->GetPlayerID(), p->index() & 0x7fffffff);
	}
	else
	{
		quest::CQuestManager::Instance().QuestButton(ch->GetPlayerID(), p->index());
	}
}

void CInputMain::ScriptAnswer(LPCHARACTER ch, std::unique_ptr<CGScriptAnswerPacket> p)
{
	sys_log(0, "QUEST ScriptAnswer pid %d answer %d", ch->GetPlayerID(), p->answer());

	if (p->answer() > 250) // ´ÙÀ½ ¹öÆ°¿¡ ´ëÇÑ ÀÀ´äÀ¸·Î ¿Â ÆÐÅ¶ÀÎ °æ¿ì
	{
		quest::CQuestManager::Instance().Resume(ch->GetPlayerID());
	}
	else // ¼±ÅÃ ¹öÆ°À» °ñ¶ó¼­ ¿Â ÆÐÅ¶ÀÎ °æ¿ì
	{
		quest::CQuestManager::Instance().Select(ch->GetPlayerID(),  p->answer());
	}
}


// SCRIPT_SELECT_ITEM
void CInputMain::ScriptSelectItem(LPCHARACTER ch, std::unique_ptr<CGScriptSelectItemPacket> p)
{
	sys_log(0, "QUEST ScriptSelectItem pid %d answer %d", ch->GetPlayerID(), p->selection());
	quest::CQuestManager::Instance().SelectItem(ch->GetPlayerID(), p->selection());
}
// END_OF_SCRIPT_SELECT_ITEM

void CInputMain::QuestInputString(LPCHARACTER ch, std::unique_ptr<CGQuestInputStringPacket> p)
{
	sys_log(0, "QUEST InputString pid %u msg %s", ch->GetPlayerID(), p->message().c_str());
	quest::CQuestManager::Instance().Input(ch->GetPlayerID(), p->message().c_str());
}

void CInputMain::QuestConfirm(LPCHARACTER ch, std::unique_ptr<CGQuestConfirmPacket> p)
{
	LPCHARACTER ch_wait = CHARACTER_MANAGER::instance().FindByPID(p->request_pid());
	if (p->answer())
		p->set_answer(quest::CONFIRM_YES);
	sys_log(0, "QuestConfirm from %s pid %u name %s answer %d", ch->GetName(), p->request_pid(), (ch_wait)?ch_wait->GetName():"", p->answer());
	if (ch_wait)
	{
		quest::CQuestManager::Instance().Confirm(ch_wait->GetPlayerID(), (quest::EQuestConfirmType) p->answer(), ch->GetPlayerID());
	}
}

void CInputMain::Target(LPCHARACTER ch, std::unique_ptr<CGTargetPacket> p)
{
	building::LPOBJECT pkObj = building::CManager::instance().FindObjectByVID(p->vid());

	if (pkObj)
	{
		network::GCOutputPacket<network::GCTargetPacket> pckTarget;
		pckTarget->set_vid(p->vid());
#ifdef __ELEMENT_SYSTEM__
		pckTarget->set_element(0);
#endif
		ch->GetDesc()->Packet(pckTarget);
	}
	else
		ch->SetTarget(CHARACTER_MANAGER::instance().Find(p->vid()));
}

void CInputMain::Warp(LPCHARACTER ch)
{
	ch->WarpEnd();
}

void CInputMain::SafeboxCheckin(LPCHARACTER ch, std::unique_ptr<CGSafeboxCheckinPacket> p)
{
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
		return;

	if (!ch->CanHandleItem())
		return;

	CSafebox * pkSafebox = ch->GetSafebox();
	LPITEM pkItem = ch->GetItem(p->inventory_pos());

	if (!pkSafebox || !pkItem)
		return;

	if (pkItem->GetCell() >= EQUIPMENT_SLOT_START && pkItem->GetCell() < EQUIPMENT_SLOT_END && IS_SET(pkItem->GetFlag(), ITEM_FLAG_IRREMOVABLE))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> Ã¢°í·Î ¿Å±æ ¼ö ¾ø´Â ¾ÆÀÌÅÛ ÀÔ´Ï´Ù."));
		return;
	}

	if (pkItem->IsEquipped() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀåºñÁßÀÎ ¾ÆÀÌÅÛÀº °³ÀÎ»óÁ¡¿¡¼­ ÆÇ¸ÅÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}
	
	LPITEM pSafeboxItem = pkSafebox->GetItem(p->safebox_pos());
	if (pSafeboxItem && quest::CQuestManager::instance().GetEventFlag("disable_safebox"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Its temporary disabled due to a bug.");
		return;
	}

	if (!pkSafebox->IsEmpty(p->safebox_pos(), pkItem->GetSize()) &&
		(!pSafeboxItem || pSafeboxItem->GetVnum() != pkItem->GetVnum() || !pSafeboxItem->IsStackable() || IS_SET(pSafeboxItem->GetAntiFlag(), ITEM_ANTIFLAG_STACK) ||
		pSafeboxItem->GetCount() + pkItem->GetCount() > ITEM_MAX_COUNT))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ¿Å±æ ¼ö ¾ø´Â À§Ä¡ÀÔ´Ï´Ù."));
		ch->tchat("%s:%d", __FILE__, __LINE__);
		ch->tchat("empty: %d safeboxItemCount: %d o: %d", !pkSafebox->IsEmpty(p->safebox_pos(), pkItem->GetSize()), pSafeboxItem ? pSafeboxItem->GetCount() : 2, pkItem->GetCount());
		return;
	}

	if (pSafeboxItem)
	{
		bool bCanStack = pkItem->CanStackWith(pSafeboxItem);
		if (!bCanStack)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ¿Å±æ ¼ö ¾ø´Â À§Ä¡ÀÔ´Ï´Ù."));
			ch->tchat("%s:%d", __FILE__, __LINE__);
			return;
		}
	}

	if (pkItem->GetVnum() == UNIQUE_ITEM_SAFEBOX_EXPAND)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if( IS_SET(pkItem->GetAntiFlag(), ITEM_ANTIFLAG_SAFEBOX) )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (true == pkItem->isLocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ÀÌ ¾ÆÀÌÅÛÀº ³ÖÀ» ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	pkItem->RemoveFromCharacter();
#ifdef __DRAGONSOUL__
	if (!pkItem->IsDragonSoul())
#endif
		ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, p->inventory_pos().cell(), 255);

	if (pSafeboxItem)
	{
		char szHint[128];
		snprintf(szHint, sizeof(szHint), "%s %u (add) -> %u (full) (new ID %u)", pSafeboxItem->GetName(), pkItem->GetCount(), pSafeboxItem->GetCount() + pkItem->GetCount(), pSafeboxItem->GetID());
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX PUT", szHint);

		pSafeboxItem->SetCount(pSafeboxItem->GetCount() + pkItem->GetCount());
		ITEM_MANAGER::instance().RemoveItem(pkItem, "SAFEBOX_STACK");

		pSafeboxItem->Save();
		ITEM_MANAGER::instance().FlushDelayedSave(pSafeboxItem);

		pkSafebox->SendSetPacket(p->safebox_pos());
	}
	else
	{
		pkSafebox->Add(p->safebox_pos(), pkItem);

		char szHint[128];
		snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX PUT", szHint);
	}
}

void CInputMain::SafeboxCheckout(LPCHARACTER ch, std::unique_ptr<CGSafeboxCheckoutPacket> p)
{
	if (!ch->CanHandleItem())
		return;

	CSafebox * pkSafebox;

	if (p->is_mall())
		pkSafebox = ch->GetMall();
	else
		pkSafebox = ch->GetSafebox();

	if (!pkSafebox)
		return;

	LPITEM pkItem = pkSafebox->Get(p->safebox_pos());

	if (!pkItem)
		return;
	
	if (!ch->IsEmptyItemGrid(p->inventory_pos(), pkItem->GetSize()))
		return;

#ifdef __DRAGONSOUL__
	if (pkItem->IsDragonSoul())
	{
		if (p->is_mall())
		{
			DSManager::instance().DragonSoulItemInitialize(pkItem);
		}

		if (DRAGON_SOUL_INVENTORY != p->inventory_pos().window_type())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ¿Å±æ ¼ö ¾ø´Â À§Ä¡ÀÔ´Ï´Ù."));
			ch->tchat("%s:%d", __FILE__, __LINE__);
			return;
		}

		::TItemPos DestPos = p->inventory_pos();
		if (!DSManager::instance().IsValidCellForThisItem(pkItem, DestPos))
		{
			int iCell = ch->GetEmptyDragonSoulInventory(pkItem);
			if (iCell < 0)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ¿Å±æ ¼ö ¾ø´Â À§Ä¡ÀÔ´Ï´Ù."));
				ch->tchat("%s:%d", __FILE__, __LINE__);
				return;
			}
			DestPos = ::TItemPos(DRAGON_SOUL_INVENTORY, iCell);
		}

		pkSafebox->Remove(p->safebox_pos());
		pkItem->AddToCharacter(ch, DestPos);
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}
	else
#endif
	{
#ifdef __DRAGONSOUL__
		if (DRAGON_SOUL_INVENTORY == p->inventory_pos().window_type())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Ã¢°í> ¿Å±æ ¼ö ¾ø´Â À§Ä¡ÀÔ´Ï´Ù."));
			ch->tchat("%s:%d", __FILE__, __LINE__);
			return;
		}
#endif

		pkSafebox->Remove(p->safebox_pos());
		if (p->is_mall())
		{
			if (NULL == pkItem->GetProto())
			{
				sys_err("pkItem->GetProto() == NULL (id : %u)", pkItem->GetID());
				return;
			}
			// 100% È®·ü·Î ¼Ó¼ºÀÌ ºÙ¾î¾ß ÇÏ´Âµ¥ ¾È ºÙ¾îÀÖ´Ù¸é »õ·Î ºÙÈù´Ù. ...............
			if (100 == pkItem->GetProto()->alter_to_magic_item_pct() && 0 == pkItem->GetAttributeCount())
			{
				pkItem->AlterToMagicItem();
			}
		}
		pkItem->AddToCharacter(ch, p->inventory_pos());
		ITEM_MANAGER::instance().FlushDelayedSave(pkItem);
	}

	network::GDOutputPacket<network::GDItemFlushPacket> pdb;
	pdb->set_item_id(pkItem->GetID());
	db_clientdesc->DBPacket(pdb);

	char szHint[128];
	snprintf(szHint, sizeof(szHint), "%s %u", pkItem->GetName(), pkItem->GetCount());
	if (p->is_mall())
		LogManager::instance().ItemLog(ch, pkItem, "MALL GET", szHint);
	else
		LogManager::instance().ItemLog(ch, pkItem, "SAFEBOX GET", szHint);
}

void CInputMain::SafeboxItemMove(LPCHARACTER ch, std::unique_ptr<CGSafeboxItemMovePacket> p)
{
	if (!ch->CanHandleItem())
		return;

	if (!ch->GetSafebox())
		return;

	if (quest::CQuestManager::instance().GetEventFlag("disable_safebox"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Moving items inside the safebox is temporary disabled.");
		return;
	}

	ch->GetSafebox()->MoveItem(p->source_pos(), p->target_pos(), p->count());
}

// PARTY_JOIN_BUG_FIX
void CInputMain::PartyInvite(LPCHARACTER ch, std::unique_ptr<CGPartyInvitePacket> p)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	LPCHARACTER pInvitee = CHARACTER_MANAGER::instance().Find(p->vid());

	if (!pInvitee || !ch->GetDesc() || !pInvitee->GetDesc())
	{
		sys_err("PARTY Cannot find invited character");
		return;
	}

#ifdef ENABLE_MESSENGER_BLOCK
	if (MessengerManager::instance().CheckMessengerList(ch->GetName(), pInvitee->GetName(), SYST_BLOCK))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't perform this action because you Blocked %s"), pInvitee->GetName());
		return;
	}
#endif

	ch->PartyInvite(pInvitee);
}

void CInputMain::PartyInviteAnswer(LPCHARACTER ch, std::unique_ptr<CGPartyInviteAnswerPacket> p)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	LPCHARACTER pInviter = CHARACTER_MANAGER::instance().Find(p->leader_vid());

	// pInviter °¡ ch ¿¡°Ô ÆÄÆ¼ ¿äÃ»À» Çß¾ú´Ù.

	if (!pInviter)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ÆÄÆ¼¿äÃ»À» ÇÑ Ä³¸¯ÅÍ¸¦ Ã£À»¼ö ¾ø½À´Ï´Ù."));
	else if (!p->accept())
		pInviter->PartyInviteDeny(ch->GetPlayerID());
	else
		pInviter->PartyInviteAccept(ch);
}
// END_OF_PARTY_JOIN_BUG_FIX

void CInputMain::PartySetState(LPCHARACTER ch, std::unique_ptr<CGPartySetStatePacket> p)
{
	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (!ch->GetParty())
		return;

	if (ch->GetParty()->GetLeaderPID() != ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ¸®´õ¸¸ º¯°æÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}

	if (!ch->GetParty()->IsMember(p->pid()))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> »óÅÂ¸¦ º¯°æÇÏ·Á´Â »ç¶÷ÀÌ ÆÄÆ¼¿øÀÌ ¾Æ´Õ´Ï´Ù."));
		return;
	}

	DWORD pid = p->pid();
	sys_log(0, "PARTY SetRole pid %d to role %d state %s", pid, p->role(), p->flag() ? "on" : "off");

	switch (p->role())
	{
		case PARTY_ROLE_NORMAL:
			break;

		case PARTY_ROLE_ATTACKER: 
		case PARTY_ROLE_TANKER: 
		case PARTY_ROLE_BUFFER:
		case PARTY_ROLE_SKILL_MASTER:
		case PARTY_ROLE_HASTE:
		case PARTY_ROLE_DEFENDER:
			if (ch->GetParty()->SetRole(pid, p->role(), p->flag()))
			{
				GDOutputPacket<GDPartyStateChangePacket> pack;
				pack->set_leader_pid(ch->GetPlayerID());
				pack->set_pid(p->pid());
				pack->set_role(p->role());
				pack->set_flag(p->flag());
				db_clientdesc->DBPacket(pack);
			}
			/* else
			   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ¾îÅÂÄ¿ ¼³Á¤¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù.")); */
			break;

		default:
			sys_err("wrong byRole in PartySetState Packet name %s state %d", ch->GetName(), p->role());
			break;
	}
}

void CInputMain::PartyRemove(LPCHARACTER ch, std::unique_ptr<CGPartyRemovePacket> p)
{
	if (ch->GetArena())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (!CPartyManager::instance().IsEnablePCParty())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ¼­¹ö ¹®Á¦·Î ÆÄÆ¼ °ü·Ã Ã³¸®¸¦ ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetDungeon())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ´øÀü ¾È¿¡¼­´Â ÆÄÆ¼¿¡¼­ Ãß¹æÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	if (!ch->GetParty())
		return;

	LPPARTY pParty = ch->GetParty();
	if (pParty->GetLeaderPID() == ch->GetPlayerID())
	{
		if (ch->GetDungeon() || pParty->IsInDungeon())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ´øÁ¯³»¿¡¼­´Â ÆÄÆ¼¿øÀ» Ãß¹æÇÒ ¼ö ¾ø½À´Ï´Ù."));
		}
		else
		{
			// leader can remove any member
			if (p->pid() == ch->GetPlayerID() || pParty->GetMemberCount() == 2)
			{
				// party disband
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				LPCHARACTER B = CHARACTER_MANAGER::instance().FindByPID(p->pid());
				if (B)
				{
					//pParty->SendPartyRemoveOneToAll(B);
					B->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(B, "<ÆÄÆ¼> ÆÄÆ¼¿¡¼­ Ãß¹æ´çÇÏ¼Ì½À´Ï´Ù."));
					//pParty->Unlink(B);
					//CPartyManager::instance().SetPartyMember(B->GetPlayerID(), NULL);
				}
				pParty->Quit(p->pid());
			}
		}
	}
	else
	{
		// otherwise, only remove itself
		if (p->pid() == ch->GetPlayerID())
		{
			if (ch->GetDungeon() || pParty->IsInDungeon())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ´øÁ¯³»¿¡¼­´Â ÆÄÆ¼¸¦ ³ª°¥ ¼ö ¾ø½À´Ï´Ù."));
			}
			else
			{
				if (pParty->GetMemberCount() == 2)
				{
					// party disband
					CPartyManager::instance().DeleteParty(pParty);
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ÆÄÆ¼¿¡¼­ ³ª°¡¼Ì½À´Ï´Ù."));
					//pParty->SendPartyRemoveOneToAll(ch);
					pParty->Quit(ch->GetPlayerID());
					//pParty->SendPartyRemoveAllToOne(ch);
					//CPartyManager::instance().SetPartyMember(ch->GetPlayerID(), NULL);
				}
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ´Ù¸¥ ÆÄÆ¼¿øÀ» Å»Åð½ÃÅ³ ¼ö ¾ø½À´Ï´Ù."));
		}
	}
}

void CInputMain::AnswerMakeGuild(LPCHARACTER ch, std::unique_ptr<CGGuildAnswerMakePacket> p)
{
	if (ch->GetGuild() && ch->GetGuild()->GetMasterPID() == ch->GetPlayerID())
	{
		// rename guild
		int iItemPos = ch->GetUsedRenameGuildItem();
		LPITEM item = ch->GetInventoryItem(iItemPos);
		if (!item || item->GetVnum() != GUILD_RENAME_ITEM_VNUM)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need %s to change your guild's name."), ITEM_MANAGER::instance().GetItemLink(GUILD_RENAME_ITEM_VNUM));
			return;
		}

		if (p->guild_name().c_str()[0] == 0 || !check_name(p->guild_name().c_str(), true))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀûÇÕÇÏÁö ¾ÊÀº ±æµå ÀÌ¸§ ÀÔ´Ï´Ù."));
			return;
		}

		ch->GetGuild()->ChangeName(ch, p->guild_name().c_str());
		item->SetCount(item->GetCount() - 1);

		return;
	}

	if (ch->GetGold() < 200000)
		return;

	if (ch->GetLevel() < 40)
		return;

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_disband_time") <
			CGuildManager::instance().GetDisbandDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ÇØ»êÇÑ ÈÄ %dÀÏ ÀÌ³»¿¡´Â ±æµå¸¦ ¸¸µé ¼ö ¾ø½À´Ï´Ù."), 
				quest::CQuestManager::instance().GetEventFlag("guild_disband_delay"));
		return;
	}

	if (get_global_time() - ch->GetQuestFlag("guild_manage.new_withdraw_time") <
			CGuildManager::instance().GetWithdrawDelay())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Å»ÅðÇÑ ÈÄ %dÀÏ ÀÌ³»¿¡´Â ±æµå¸¦ ¸¸µé ¼ö ¾ø½À´Ï´Ù."), 
				quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay"));
		return;
	}

	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, p->guild_name().c_str(), sizeof(cp.name));

	if (cp.name[0] == 0 || !check_name(cp.name, true))
	{
		ch->tchat("1111");
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀûÇÕÇÏÁö ¾ÊÀº ±æµå ÀÌ¸§ ÀÔ´Ï´Ù."));
		return;
	}

	DWORD dwGuildID = gm.CreateGuild(cp);

	if (dwGuildID)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> [%s] ±æµå°¡ »ý¼ºµÇ¾ú½À´Ï´Ù."), cp.name);

		int GuildCreateFee = 200000;

		ch->PointChange(POINT_GOLD, -GuildCreateFee);
		LogManager::instance().MoneyLog(MONEY_LOG_GUILD, ch->GetPlayerID(), -GuildCreateFee);

		char Log[128];
		snprintf(Log, sizeof(Log), "GUILD_NAME %s MASTER %s", cp.name, ch->GetName());
		LogManager::instance().CharLog(ch, 0, "MAKE_GUILD", Log);

		//ch->SendGuildName(dwGuildID);
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå »ý¼º¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
}

void CInputMain::PartyUseSkill(LPCHARACTER ch, std::unique_ptr<CGPartyUseSkillPacket> p)
{
	if (!ch->GetParty())
		return;

	if (ch->GetPlayerID() != ch->GetParty()->GetLeaderPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ÆÄÆ¼ ±â¼úÀº ÆÄÆ¼Àå¸¸ »ç¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}

	switch (p->skill_index())
	{
		case PARTY_SKILL_HEAL:
			ch->GetParty()->HealParty();
			break;
		case PARTY_SKILL_WARP:
			{
				LPCHARACTER pch = CHARACTER_MANAGER::instance().Find(p->vid());
				if (pch)
					ch->GetParty()->SummonToLeader(pch->GetPlayerID());
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<ÆÄÆ¼> ¼ÒÈ¯ÇÏ·Á´Â ´ë»óÀ» Ã£À» ¼ö ¾ø½À´Ï´Ù."));
			}
			break;
	}
}

void CInputMain::PartyParameter(LPCHARACTER ch, std::unique_ptr<CGPartyParameterPacket> p)
{
	if (ch->GetParty())
		ch->GetParty()->SetParameter(p->distribute_mode());
}

void CInputMain::Guild(LPCHARACTER ch, const InputPacket& data)
{
	CGuild* pGuild = ch->GetGuild();
	auto header = data.get_header<TCGHeader>();

	if (NULL == pGuild)
	{
		if (header != TCGHeader::GUILD_INVITE_ANSWER)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Guild> It does not belong to the Guild."));
			return;
		}
	}

	switch (header)
	{
		case TCGHeader::GUILD_DEPOSIT_MONEY:
			{
				return;

				auto pack = data.get<CGGuildDepositMoneyPacket>();

				const int gold = MIN(pack->gold(), __deposit_limit());

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Guild> That is not the correct Gold sum."));
					return;
				}

				if (ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Guild> You do not have enough gold."));
					return;
				}

				pGuild->RequestDepositMoney(ch, gold);
			}
			break;

		case TCGHeader::GUILD_WITHDRAW_MONEY:
			{
				return;

				auto pack = data.get<CGGuildWithdrawMoneyPacket>();

				const int gold = MIN(pack->gold(), 500000);

				if (gold < 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Guild> That is not the correct Gold sum."));
					return;
				}

				pGuild->RequestWithdrawMoney(ch, gold);
			}
			break;

		case TCGHeader::GUILD_ADD_MEMBER:
			{
				auto pack = data.get<CGGuildAddMemberPacket>();

				LPCHARACTER newmember = CHARACTER_MANAGER::instance().Find(pack->vid());

				if (!newmember)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<Guild> The wanted person cannot be found."));
					return;
				}

#ifdef ENABLE_MESSENGER_BLOCK
				if (MessengerManager::instance().CheckMessengerList(ch->GetName(), newmember->GetName(), SYST_BLOCK))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't perform this action because you Blocked %s"), newmember->GetName());
					return;
				}
#endif

				if (!newmember->IsPC())
					return;

				pGuild->Invite(ch, newmember);
			}
			return;

		case TCGHeader::GUILD_REMOVE_MEMBER:
			{
				auto pack = data.get<CGGuildRemoveMemberPacket>();

				if (pGuild->UnderAnyWar() != 0)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀü Áß¿¡´Â ±æµå¿øÀ» Å»Åð½ÃÅ³ ¼ö ¾ø½À´Ï´Ù."));
					return;
				}

				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return;

				LPCHARACTER member = CHARACTER_MANAGER::instance().FindByPID(pack->pid());

				if (member)
				{
					if (member->GetGuild() != pGuild)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> »ó´ë¹æÀÌ °°Àº ±æµå°¡ ¾Æ´Õ´Ï´Ù."));
						return;
					}

					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå¿øÀ» °­Á¦ Å»Åð ½ÃÅ³ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
						return;
					}

					member->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
					pGuild->RequestRemoveMember(member->GetPlayerID());
				}
				else
				{
					if (!pGuild->HasGradeAuth(m->grade, GUILD_AUTH_REMOVE_MEMBER))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå¿øÀ» °­Á¦ Å»Åð ½ÃÅ³ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
						return;
					}

					if (pGuild->RequestRemoveMember(pack->pid()))
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå¿øÀ» °­Á¦ Å»Åð ½ÃÄ×½À´Ï´Ù."));
					else
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±×·¯ÇÑ »ç¶÷À» Ã£À» ¼ö ¾ø½À´Ï´Ù."));
				}
			}
			return;

		case TCGHeader::GUILD_CHANGE_GRADE_NAME:
			{
				auto pack = data.get<CGGuildChangeGradeNamePacket>();

				const TGuildMember * m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Á÷À§ ÀÌ¸§À» º¯°æÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
				}
				else if (pack->grade() == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀåÀÇ Á÷À§ ÀÌ¸§Àº º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
				}
				else if (!check_name(pack->gradename().c_str()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ÀûÇÕÇÏÁö ¾ÊÀº Á÷À§ ÀÌ¸§ ÀÔ´Ï´Ù."));
				}
				else
				{
					pGuild->ChangeGradeName(pack->grade(), pack->gradename().c_str());
				}
			}
			return;

		case TCGHeader::GUILD_CHANGE_GRADE_AUTHORITY:
			{
				auto pack = data.get<CGGuildChangeGradeAuthorityPacket>();

				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Á÷À§ ±ÇÇÑÀ» º¯°æÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
				}
				else if (pack->grade() == GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀåÀÇ ±ÇÇÑÀº º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
				}
				else
				{
					pGuild->ChangeGradeAuth(pack->grade(), pack->authority());
				}
			}
			return;

		case TCGHeader::GUILD_OFFER_EXP:
			{
				auto pack = data.get<CGGuildOfferExpPacket>();

				if (pGuild->GetLevel() >= GUILD_MAX_LEVEL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµå°¡ ÀÌ¹Ì ÃÖ°í ·¹º§ÀÔ´Ï´Ù."));
				}
				else
				{
					DWORD offer = pack->exp() / 100;
					offer *= 100;

					if (pGuild->OfferExp(ch, offer))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> %uÀÇ °æÇèÄ¡¸¦ ÅõÀÚÇÏ¿´½À´Ï´Ù."), offer);
					}
					else
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> °æÇèÄ¡ ÅõÀÚ¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
					}
				}
			}
			return;
			
		case TCGHeader::GUILD_CHARGE_GSP:
			{
				auto pack = data.get<CGGuildChargeGSPPacket>();

				const int gold = pack->amount() * 100;

				if (pack->amount() < 0 || gold < pack->amount() || gold < 0 || ch->GetGold() < gold)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
					return;
				}

				if (!pGuild->ChargeSP(ch, pack->amount()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ¿ë½Å·Â È¸º¹¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
				}
			}
			return;

		case TCGHeader::GUILD_POST_COMMENT:
			{
				auto pack = data.get<CGGuildPostCommentPacket>();

				if (pack->message().length() > GUILD_COMMENT_MAX_LEN)
				{
					// Àß¸øµÈ ±æÀÌ.. ²÷¾îÁÖÀÚ.
					sys_err("POST_COMMENT: %s comment too long (length: %u)", ch->GetName(), pack->message().length());
					ch->GetDesc()->SetPhase(PHASE_CLOSE);
					return;
				}

				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return;
				
				if (pack->message().length() > 0 && !pGuild->HasGradeAuth(m->grade, GUILD_AUTH_NOTICE)
					&& pack->message()[0] == '!')
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> °øÁö±ÛÀ» ÀÛ¼ºÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
				}
				else
				{
					pGuild->AddComment(ch, pack->message());
				}

				return;
			}

		case TCGHeader::GUILD_DELETE_COMMENT:
			{
				auto pack = data.get<CGGuildDeleteCommentPacket>();

				pGuild->DeleteComment(ch, pack->comment_id());
			}
			return;

		case TCGHeader::GUILD_REFRESH_COMMENT:
			pGuild->RefreshComment(ch);
			return;

		case TCGHeader::GUILD_CHANGE_MEMBER_GRADE:
			{
				auto pack = data.get<CGGuildChangeMemberGradePacket>();
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return;

				if (m->grade != GUILD_LEADER_GRADE)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Á÷À§¸¦ º¯°æÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
				else if (ch->GetPlayerID() == pack->pid())
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀåÀÇ Á÷À§´Â º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
				else if (pack->grade() == 1)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ±æµåÀåÀ¸·Î Á÷À§¸¦ º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
				else
					pGuild->ChangeMemberGrade(pack->pid(), pack->grade());
			}
			return;

		case TCGHeader::GUILD_USE_SKILL:
			{
				auto pack = data.get<CGGuildUseSkillPacket>();

				pGuild->UseSkill(pack->vnum(), ch, pack->pid());
			}
			return;

		case TCGHeader::GUILD_CHANGE_MEMBER_GENERAL:
			{
				auto pack = data.get<CGGuildChangeMemberGeneralPacket>();
				const TGuildMember* m = pGuild->GetMember(ch->GetPlayerID());

				if (NULL == m)
					return;

				if (m->grade != GUILD_LEADER_GRADE)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> Àå±ºÀ» ÁöÁ¤ÇÒ ±ÇÇÑÀÌ ¾ø½À´Ï´Ù."));
				}
				else
				{
					if (!pGuild->ChangeMemberGeneral(pack->pid(), pack->is_general()))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "<±æµå> ´õÀÌ»ó Àå¼ö¸¦ ÁöÁ¤ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					}
				}
			}
			return;

		case TCGHeader::GUILD_INVITE_ANSWER:
			{
				auto pack = data.get<CGGuildInviteAnswerPacket>();

				CGuild * g = CGuildManager::instance().FindGuild(pack->guild_id());

				if (g)
				{
					if (pack->accept())
						g->InviteAccept(ch);
					else
						g->InviteDeny(ch->GetPlayerID());
				}
			}
			return;

		case TCGHeader::GUILD_REQUEST_LIST:
		{
			auto pack = data.get<CGGuildRequestListPacket>();

			CGuildManager::instance().SendGuildList(ch, pack->page_number(), pack->page_type(), pack->empire());

			return;
		}

		case TCGHeader::GUILD_SEARCH:
		{
			auto pack = data.get<CGGuildSearchPacket>();

			CGuildManager::instance().SendSpecificGuildRank(ch, pack->search_name().c_str(), pack->page_type(), pack->empire());

			return;
		}
	}
}

void CInputMain::Fishing(LPCHARACTER ch, std::unique_ptr<CGFishingPacket> p)
{
	ch->SetRotation(p->dir() * 5);
	ch->fishing();
	return;
}

void CInputMain::Hack(LPCHARACTER ch, std::unique_ptr<CGHackPacket> p)
{
	char buf[sizeof(p->buf().c_str())];
	strlcpy(buf, p->buf().c_str(), sizeof(buf));

	sys_err("HACK_DETECT: %s %s", ch->GetName(), buf);

	// ÇöÀç Å¬¶óÀÌ¾ðÆ®¿¡¼­ ÀÌ ÆÐÅ¶À» º¸³»´Â °æ¿ì°¡ ¾øÀ¸¹Ç·Î ¹«Á¶°Ç ²÷µµ·Ï ÇÑ´Ù
	ch->GetDesc()->SetPhase(PHASE_CLOSE);
}

void CInputMain::MyShop(LPCHARACTER ch, std::unique_ptr<CGMyShopPacket> p)
{
	if (ch->GetGold() >= GOLD_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¼ÒÀ¯ µ·ÀÌ 20¾ï³ÉÀ» ³Ñ¾î °Å·¡¸¦ ÇÛ¼ö°¡ ¾ø½À´Ï´Ù."));
		sys_log(0, "MyShop ==> OverFlow Gold id %u name %s ", ch->GetPlayerID(), ch->GetName());
		return;
	}

	if (ch->IsStun() || ch->IsDead())
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´Ù¸¥ °Å·¡ÁßÀÏ°æ¿ì °³ÀÎ»óÁ¡À» ¿­¼ö°¡ ¾ø½À´Ï´Ù."));
		return;
	}

#ifdef __ACCE_COSTUME__
	if (ch->IsAcceWindowOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´Ù¸¥ °Å·¡ÁßÀÏ°æ¿ì °³ÀÎ»óÁ¡À» ¿­¼ö°¡ ¾ø½À´Ï´Ù."));
		return;
	}
#endif

	sys_log(0, "MyShop count %d", p->items_size());
	ch->OpenMyShop(p->sign().c_str(), p->items());
	return;
}

void CInputMain::Refine(LPCHARACTER ch, std::unique_ptr<CGRefinePacket> p)
{
	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShop() || ch->GetMyShop() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO,  LC_TEXT(ch, "Ã¢°í,°Å·¡Ã¢µîÀÌ ¿­¸° »óÅÂ¿¡¼­´Â °³·®À» ÇÒ¼ö°¡ ¾ø½À´Ï´Ù"));
		ch->ClearRefineMode();
		return;
	}

#ifdef __ACCE_COSTUME__
	if (ch->IsAcceWindowOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Ã¢°í,°Å·¡Ã¢µîÀÌ ¿­¸° »óÅÂ¿¡¼­´Â °³·®À» ÇÒ¼ö°¡ ¾ø½À´Ï´Ù"));
		ch->ClearRefineMode();
		return;
	}
#endif

	if (p->type() == 255)
	{
		// DoRefine Cancel
		ch->ClearRefineMode();
		return;
	}

	if (p->cell().cell() < INVENTORY_SLOT_START || p->cell().cell() >INVENTORY_SLOT_END)
	{
		ch->ClearRefineMode();
		return;
	}

	LPITEM item = ch->GetInventoryItem(p->cell().cell());

	if (!item)
	{
		ch->ClearRefineMode();
		return;
	}

	ch->SetRefineTime();

	if (p->type() == REFINE_TYPE_NORMAL)
	{
		sys_log (0, "refine_type_noraml");
		ch->DoRefine(item, false, p->fast_refine());
	}
	else if (p->type() == REFINE_TYPE_SCROLL || p->type() == REFINE_TYPE_HYUNIRON || p->type() == REFINE_TYPE_MUSIN || p->type() == REFINE_TYPE_BDRAGON)
	{
		sys_log (0, "refine_type_scroll, ...");
		ch->DoRefineWithScroll(item, p->fast_refine());
	}
	else if (p->type() == REFINE_TYPE_MONEY_ONLY)
	{
		const LPITEM item = ch->GetInventoryItem(p->cell().cell());

		if (NULL != item)
		{
			if (item->IsRefinedOtherItem())
			{
				LogManager::instance().HackLog("DEVIL_TOWER_REFINE_HACK", ch);
			}
			else
			{
				ch->DoRefine(item, true, p->fast_refine());
			}
		}
	}

	ch->ClearRefineMode();
}

#ifdef __ACCE_COSTUME__
void CInputMain::AcceRefine(LPCHARACTER ch, const network::InputPacket& data)
{
	switch (data.get_header<TCGHeader>())
	{
		case TCGHeader::ACCE_REFINE_CHECKIN:
			{
				auto pack = data.get<CGAcceRefineCheckinPacket>();
				ch->AcceRefineCheckin(pack->acce_pos(), pack->item_cell());
			}
			break;

		case TCGHeader::ACCE_REFINE_CHECKOUT:
			{
				auto pack = data.get<CGAcceRefineCheckoutPacket>();
				ch->AcceRefineCheckout(pack->acce_pos());

			}
			break;

		case TCGHeader::ACCE_REFINE_ACCEPT:
			{
				auto pack = data.get<CGAcceRefineAcceptPacket>();
				ch->AcceRefineAccept(pack->window_type());
			}
			break;

		case TCGHeader::ACCE_REFINE_CANCEL:
			ch->AcceRefineCancel();
			break;
	}
}
#endif

#ifdef __GUILD_SAFEBOX__
void CInputMain::GuildSafebox(LPCHARACTER ch, const InputPacket& data)
{
	if (!ch->GetGuild())
	{
		sys_err("no guild for player %d %s header %d", ch->GetPlayerID(), ch->GetName(), data.get_header());
		return;
	}

	CGuildSafeBox& rkGuildSafeBox = ch->GetGuild()->GetSafeBox();
	if (!rkGuildSafeBox.HasSafebox())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Your guild has no safebox."));
		return;
	}

	auto header = data.get_header<TCGHeader>();
	if (header == TCGHeader::GUILD_SAFEBOX_OPEN)
	{
		rkGuildSafeBox.OpenSafebox(ch);
	}
	else if (header == TCGHeader::GUILD_SAFEBOX_CHECKIN)
	{
		auto p = data.get<CGGuildSafeboxCheckinPacket>();

		LPITEM pkItem = ch->GetItem(p->item_pos());
		if (pkItem)
			rkGuildSafeBox.CheckInItem(ch, pkItem, p->safebox_pos());
		else
			sys_err("GUILD_SAFEBOX_CHECKIN: cannot get item window %d pos %d", p->item_pos().window_type(), p->item_pos().cell());
	}
	else if (header == TCGHeader::GUILD_SAFEBOX_CHECKOUT)
	{
		auto p = data.get<CGGuildSafeboxCheckoutPacket>();
		rkGuildSafeBox.CheckOutItem(ch, p->safebox_pos(), p->item_pos().window_type(), p->item_pos().cell());
	}
	else if (header == TCGHeader::GUILD_SAFEBOX_ITEM_MOVE)
	{
		auto p = data.get<CGGuildSafeboxItemMovePacket>();
		rkGuildSafeBox.MoveItem(ch, p->source_pos(), p->target_pos(), p->count());
	}
	else if (header == TCGHeader::GUILD_SAFEBOX_GIVE_GOLD)
	{
		auto p = data.get<CGGuildSafeboxGiveGoldPacket>();
		rkGuildSafeBox.GiveGold(ch, p->gold());
	}
	else if (header == TCGHeader::GUILD_SAFEBOX_GET_GOLD)
	{
		auto p = data.get<CGGuildSafeboxGetGoldPacket>();
		rkGuildSafeBox.TakeGold(ch, p->gold());
	}
}
#endif

void CInputMain::ItemDestroy(LPCHARACTER ch, std::unique_ptr<CGItemDestroyPacket> pack)
{
	if (ch && pack->num())
		ch->DestroyItem(pack->cell(), pack->num());
}

void CInputMain::TargetMonsterDropInfo(LPCHARACTER ch, std::unique_ptr<CGTargetMonsterDropInfoPacket> p)
{
	if (!ch->GetTarget() || (ch->GetTarget()->GetType() != CHAR_TYPE_MONSTER && ch->GetTarget()->GetType() != CHAR_TYPE_STONE) || ch->GetTarget()->GetRaceNum() != p->race_num())
		return;

	std::vector<TTargetMonsterDropInfoTable> vec_ItemList;
	ITEM_MANAGER::instance().GetDropItemList(p->race_num(), vec_ItemList, ch->GetLevel());

	network::GCOutputPacket<network::GCTargetMonsterInfoPacket> packet;
	packet->set_race_num(p->race_num());
	for (auto& elem : vec_ItemList)
		*packet->add_drops() = elem;

	ch->GetDesc()->Packet(packet);
}

void CInputMain::PlayerLanguageInformation(LPCHARACTER ch, std::unique_ptr<CGPlayerLanguageInformationPacket> p)
{
	if (!strcasecmp(p->player_name().c_str(), ch->GetName()))
		return;

	char cLangID = -1;
	if (LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(p->player_name().c_str()))
		cLangID = ch->GetLanguageID();
	else if (CCI* pkCCI = P2P_MANAGER::instance().Find(p->player_name().c_str()))
		cLangID = pkCCI->bLanguage;
	else
		return;

	network::GCOutputPacket<network::GCPlayerOnlineInformationPacket> packet;
	packet->set_player_name(p->player_name());
	packet->set_language_id(cLangID);

	if (ch && ch->GetDesc())
		ch->GetDesc()->Packet(packet);
}

void CInputMain::ItemMultiUse(LPCHARACTER ch, std::unique_ptr<CGItemMultiUsePacket> p)
{
	LPITEM pkItem = ch->GetItem(p->cell());
	if (!pkItem || pkItem->GetCount() != p->count())
		return;
	
	for (int i = 0; i < MIN(p->count(), 200); ++i) // Keep this 200 anyway!
	{
		pkItem = ch->GetItem(p->cell());
		if (!pkItem || !pkItem->GetCount())
			break;

		ch->UseItem(p->cell());
	}
}

#ifdef __PYTHON_REPORT_PACKET__
void CInputMain::BotReportLog(LPCHARACTER ch, std::unique_ptr<CGBotReportLogPacket> p)
{
	ch->DetectionHackLog(p->type().c_str(), p->detail().c_str());
}
#endif

void CInputMain::ForcedRewarp(LPCHARACTER ch, std::unique_ptr<CGForcedRewarpPacket> p)
{
	if (p->checkval() != 404)
		return;

	if (quest::CQuestManager::instance().GetEventFlag("disable_forced_rewarp") == 1)
	{
		sys_log(0, "ForcedRewarp of %s %u -> DISABLED", ch->GetName(), ch->GetPlayerID());
		return;
	}

	LogManager::instance().ForcedRewarpLog(ch, p->detail_log().c_str());
	ch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
}

void CInputMain::UseDetachmentSingle(LPCHARACTER ch, std::unique_ptr<CGUseDetachmentSinglePacket> p)
{
	if (p->cell_detachment() > ch->GetInventoryMaxNum() || p->cell_item() > ch->GetInventoryMaxNum())
		return;

	LPITEM pItemDetachment = ch->GetInventoryItem(p->cell_detachment());
	LPITEM pItem = ch->GetInventoryItem(p->cell_item());

	if (!pItemDetachment || pItemDetachment->GetType() != ITEM_USE || !pItem)
		return;

	if (pItemDetachment->GetSubType() == USE_DETACH_STONE && IS_SET(pItem->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		if (ch->GetExchange())
			return;
		
		if (p->slot_index() > ITEM_SOCKET_MAX_NUM || pItem->GetSocket(p->slot_index()) < 2 || pItem->GetSocket(p->slot_index()) == ITEM_BROKEN_METIN_VNUM)
			return;

		ch->AutoGiveItem(pItem->GetSocket(p->slot_index()));
		pItem->SetSocket(p->slot_index(), ITEM_BROKEN_METIN_VNUM);
		pItemDetachment->SetCount(pItemDetachment->GetCount() - 1);

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have successfully detached the metin stone from the item."));
	}
	else if (pItemDetachment->GetSubType() == USE_DETACH_ATTR)
	{
		if (pItem->GetType() != ITEM_COSTUME || p->slot_index() > ITEM_ATTRIBUTE_MAX_NUM || !pItem->GetAttributeType(p->slot_index()))
			return;

		if (pItem->GetSubType() == COSTUME_ACCE_COSTUME)
			return;

		pItem->SetForceAttribute(p->slot_index(), 0, 0);
		pItemDetachment->SetCount(pItemDetachment->GetCount() - 1);

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have successfully removed an attribute from the item."));
	}
}

#ifdef __EVENT_MANAGER__
void CInputMain::EventRequestAnswer(LPCHARACTER ch, std::unique_ptr<CGEventRequestAnswerPacket> p)
{
	if (CEventManager::instance().GetRunningEventIndex() != p->event_index())
	{
		sys_err("cannot process event request answer packet: invalid index %d (running %d)",
				p->event_index(), CEventManager::instance().GetRunningEventIndex());
		return;
	}

	CEventManager::instance().OnPlayerAnswer(ch, p->accept());
}
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
void CInputMain::CostumeBonusTransfer(LPCHARACTER ch, std::unique_ptr<CGCostumeBonusTransferPacket> p)
{
	switch (p->sub_header())
	{
		case CBT_SUBHEADER_CHECKIN:
			ch->CBT_CheckIn(p->pos(), p->item_cell());
			break;

		case CBT_SUBHEADER_CHECKOUT:
			ch->CBT_CheckOut(p->pos());
			break;

		case CBT_SUBHEADER_ACCEPT:
			ch->CBT_Accept();
			break;

		case CBT_SUBHEADER_CANCEL:
			ch->CBT_WindowClose();
			break;
	}
}
#endif

#ifdef ENABLE_RUNE_SYSTEM
void CInputMain::RunePage(LPCHARACTER ch, std::unique_ptr<CGRunePagePacket> p)
{
	ch->SetRunePage(&p->data());
}
#endif

#ifdef INGAME_WIKI
void CInputMain::RecvWikiPacket(LPCHARACTER ch, std::unique_ptr<CGRecvWikiPacket> p)
{
	if (!p->is_mob())
	{
		const ::TWikiInfoTable* tbl = ITEM_MANAGER::instance().GetItemWikiInfo(p->vnum());
		if (tbl)
		{
			const std::vector<TWikiItemOriginInfo>& originVec = ITEM_MANAGER::Instance().GetItemOrigin(p->vnum());

			network::GCOutputPacket<network::GCWikiPacket> packet;
			packet->set_ret_id(p->ret_id());
			packet->set_vnum(p->vnum());
			*packet->mutable_wiki_info() = *tbl;
			for (auto& elem : originVec)
				*packet->add_origin_infos() = elem;

			ch->GetDesc()->Packet(packet);
		}
	}
	else
	{
		std::vector<DWORD>& mobVec = CMobManager::instance().GetMobWikiInfo(p->vnum());
		if (!mobVec.size())
		{
			if (test_server)
				sys_err("WIKI_MOB_NO_DROP: %d", p->vnum());
			return;
		}
		network::GCOutputPacket<network::GCWikiMobPacket> pack;
		pack->set_ret_id(p->ret_id());
		pack->set_vnum(p->vnum());
		for (DWORD vnum : mobVec)
			pack->add_mobs(vnum);

		ch->GetDesc()->Packet(pack);
	}
}
#endif

#ifdef CHANGE_SKILL_COLOR
void CInputMain::SetSkillColor(LPCHARACTER ch, std::unique_ptr<CGSetSkillColorPacket> p)
{
	if (p->skill() > ESkillColorLength::MAX_SKILL_COUNT)
		return;

	bool unlocked = ch->GetQuestFlag("skill_color" + std::to_string(p->skill()) + ".unlocked");

	if (!unlocked)
	{
		// probably doesn't need to be an output since the + should be only on skills that are unlocked
		//ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to unlock the skill customisation first"));
		return;
	}

	DWORD data[ESkillColorLength::MAX_SKILL_COUNT][ESkillColorLength::MAX_EFFECT_COUNT];
	memcpy(data, ch->GetSkillColor(), sizeof(data));

	data[p->skill()][0] = p->col1();
	data[p->skill()][1] = p->col2();
	data[p->skill()][2] = p->col3();
	data[p->skill()][3] = p->col4();
	data[p->skill()][4] = p->col5();

	auto checkValidColor = [](const DWORD &color) {
		BYTE *colors = (BYTE*)&color;

		bool isFine = true;

		if (colors[0] < 40 || colors[1] < 40 || colors[2] < 40)
			isFine = false;

		if (colors[0] >= 40 || colors[1] >= 40 || colors[2] >= 40)
			isFine = true;

		if (colors[0] == 0 && colors[1] == 0 && colors[2] == 0)
			isFine = true;

		return isFine;
	};

	for (int i = 0; i < ESkillColorLength::MAX_EFFECT_COUNT; ++i)
	{
		if (data[p->skill()][i] == 0)
			continue;

		if (!checkValidColor(data[p->skill()][i]))
		{
			BYTE *colors = (BYTE*)&data[p->skill()][i];
			ch->tchat("wrong color layer%d : r(%d) g(%d) b(%d) data[p->set_skill(][%d]%d", i, colors[0], colors[1], colors[2], i, data[p->skill()][i]);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to enter a valid color"));
			return;
		}
	}

	ch->SetSkillColor(data[0]);

	GDOutputPacket<GDSkillColorSavePacket> pdb;
	pdb->set_player_id(ch->GetPlayerID());
	for (auto skill_idx = 0; skill_idx < ESkillColorLength::MAX_SKILL_COUNT; ++skill_idx)
	{
		for (auto eff_idx = 0; eff_idx < ESkillColorLength::MAX_EFFECT_COUNT; ++eff_idx)
		{
			pdb->add_skill_colors(data[skill_idx][eff_idx]);
		}
	}
	db_clientdesc->DBPacket(pdb);
}
#endif

#ifdef __EQUIPMENT_CHANGER__
void CInputMain::EquipmentPageAdd(LPCHARACTER ch, std::unique_ptr<CGEquipmentPageAddPacket> p)
{
	ch->tchat("%s:%d", __FUNCTION__, __LINE__);
	if (!*p->name().c_str())
		return;

	ch->AddEquipmentChangerPage(p->name().c_str());
}

void CInputMain::EquipmentPageDelete(LPCHARACTER ch, std::unique_ptr<CGEquipmentPageDeletePacket> p)
{
	ch->tchat("%s:%d", __FUNCTION__, __LINE__);
	ch->DeleteEquipmentChangerPage(p->index());
}

void CInputMain::EquipmentPageSelect(LPCHARACTER ch, std::unique_ptr<CGEquipmentPageSelectPacket> p)
{
	ch->tchat("%s:%d", __FUNCTION__, __LINE__);
	if (ch->GetEquipmentChangerPageIndex() == p->index())
		return;

	if (!ch->CanHandleItem())
		return;

	quest::PC* pkPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pkPC && pkPC->IsRunning() == true)
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShopOwner() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot change your equipment while youre trading."));
		return;
	}

	DWORD dwLastChange = ch->GetEquipmentChangeLastChange();
	if (dwLastChange && dwLastChange + 3 * 1000 >= get_dword_time())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to wait a short time until you can change your equipment again."));
		return;
	}
#ifdef CHECK_TIME_AFTER_PVP
	if (ch->IsPVPFighting())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have to wait a short time until you can change your equipment again."));
		return;
	}
#endif

	ch->SetEquipmentChangerLastChange(get_dword_time());
	ch->SetEquipmentChangerPageIndex(p->index());
}

#endif

#ifdef __PET_ADVANCED__
void CInputMain::PetAdvanced(LPCHARACTER ch, network::TCGHeader header, const network::InputPacket& data)
{
	switch (header)
	{
		case TCGHeader::PET_USE_EGG:
			{
				auto pack = data.get<CGPetUseEggPacket>();

				LPITEM item = ch->GetItem(pack->egg_cell());
				if (!item || item->GetType() != ITEM_PET_ADVANCED || static_cast<EPetAdvancedItem>(item->GetSubType()) != EPetAdvancedItem::EGG)
				{
					sys_err("USE_EGG with invalid item (pos [%u, %u] for player %u %s) item %p",
						pack->egg_cell().window_type(), pack->egg_cell().cell(), ch->GetPlayerID(), ch->GetName(), item);
					return;
				}

				if (!ch->CanHandleItem() || item->IsExchanging())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Please retry in a few seconds.");
					return;
				}

				if (item->GetSocket(0) > get_global_time())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "The pet cannot hatch yet.");
					return;
				}

				auto price = item->GetValue(2);
				if (price > ch->GetGold())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "You have not enough gold.");
					return;
				}

				if (!check_name(pack->pet_name().c_str()))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "That is no valid name for a pet.");
					return;
				}

				LPITEM pet_item = ITEM_MANAGER::Instance().CreateItem(item->GetValue(1));
				if (!pet_item || !pet_item->GetAdvancedPet())
				{
					if (pet_item)
						M2_DESTROY_ITEM(pet_item);

					sys_err("cannot create pet item %u for %u %s from egg", item->GetValue(1), ch->GetPlayerID(), ch->GetName());
					return;
				}

				ch->PointChange(POINT_GOLD, -price);

				pet_item->GetAdvancedPet()->Initialize(pack->pet_name(), true);
				
				item->SetCount(item->GetCount() - 1);
				ch->AutoGiveItem(pet_item, true);
			}
			return;
	}

	CPetAdvanced* pet = ch->GetPetAdvanced();
	if (!pet)
		return;

	switch (header)
	{
		case TCGHeader::PET_ATTR_REFINE_INFO:
			{
				auto pack = data.get<CGPetAttrRefineInfoPacket>();
				pet->ShowAttrRefineInfo(pack->index());
			}
			break;

		case TCGHeader::PET_ATTR_REFINE:
			{
				if (!ch->CanHandleItem() || ch->GetExchange())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Please retry in a few seconds."));
					return;
				}

				auto pack = data.get<CGPetAttrRefinePacket>();
				if (!pet->CanUpgradeAttr(pack->index()))
					return;

				auto attr = CPetAttrProto::Get(CPetAttrProto::GetKey(pet->GetAttrType(pack->index()), pet->GetAttrLevel(pack->index()) + 1));
				if (!attr)
					return;

				auto refine = CRefineManager::instance().GetRefineRecipe(attr->GetRefineID());
				if (!refine)
					return;

				if (ch->GetGold() < refine->cost())
					return;

				for (int i = 0; i < refine->material_count(); ++i)
				{
					if (ch->CountSpecifyItem(refine->materials(i).vnum()) < refine->materials(i).count())
						return;
				}

				ch->PointChange(POINT_GOLD, -refine->cost());

				for (int i = 0; i < refine->material_count(); ++i)
					ch->RemoveSpecifyItem(refine->materials(i).vnum(), refine->materials(i).count());

				pet->UpgradeAttr(pack->index());
			}
			break;

		case TCGHeader::PET_EVOLUTION_INFO:
			pet->ShowEvolutionInfo();
			break;

		case TCGHeader::PET_EVOLVE:
			pet->Evolve();
			break;

		case TCGHeader::PET_RESET_SKILL:
			{
				auto pack = data.get<CGPetResetSkillPacket>();

				LPITEM item = ch->GetItem(pack->reset_cell());
				if (!item || item->GetType() != ITEM_PET_ADVANCED || static_cast<EPetAdvancedItem>(item->GetSubType()) != EPetAdvancedItem::SKILL_REVERT)
					return;

				if (!ch->CanHandleItem() || item->IsExchanging())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Please retry in a few seconds."));
					return;
				}

				auto skill_info = pet->GetSkillInfo(pack->skill_index());
				if (!skill_info || skill_info->vnum() == 0 || skill_info->level() == 0)
					return;

				pet->ClearSkill(pack->skill_index());
				item->SetCount(item->GetCount() - 1);

				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The skill of your pet was resetted."));
			}
			break;

		default:
			sys_err("invalid PetAdvanced input from %u %s (subheader %d unknown)", ch->GetPlayerID(), ch->GetName(), static_cast<uint16_t>(header));
			return;
	}
}
#endif

#ifdef AUCTION_SYSTEM
void CInputMain::AuctionInsertItem(LPCHARACTER ch, std::unique_ptr<network::CGAuctionInsertItemPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (p->target_cell().window_type() == AUCTION && quest::CQuestManager::instance().GetEventFlag("auction_insert_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (p->target_cell().window_type() == AUCTION_SHOP && quest::CQuestManager::instance().GetEventFlag("auction_insert_shop_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (!ch->CanHandleItem())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Try again in a few seconds..."));
		return;
	}

	if (!GM::check_allow(ch->GetGMLevel(), GM_ALLOW_CREATE_PRIVATE_SHOP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot do this with this gamemaster rank."));
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (ch->GetDesc()->IsTradeblocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return;
	}
#endif

	if (p->price() <= 0)
		return;

	if (p->price() > __SHOP_SELL_MAX_PRICE__)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The maximum price for items is %llu."), __SHOP_SELL_MAX_PRICE__);
		return;
	}

	auto item = ch->GetItem(p->cell());
	if (item == nullptr)
		return;

	if (!item->CanPutItemIntoShop())
		return;

	if (p->target_cell().window_type() != AUCTION && p->target_cell().window_type() != AUCTION_SHOP)
		return;

	if (p->target_cell().window_type() == AUCTION)
	{
		p->mutable_target_cell()->set_cell(0);
	}
	else if (p->target_cell().window_type() == AUCTION_SHOP)
	{
		if (p->target_cell().cell() + (MAX(1, item->GetSize()) - 1) * AuctionManager::SHOP_SLOT_COUNT_X >= AuctionManager::SHOP_SLOT_COUNT)
			return;
	}
	else
		return;

	network::GGOutputPacket<network::GGAuctionInsertItemPacket> pack;
	auto shop_item = pack->mutable_item();
	auto item_data = shop_item->mutable_item();

	LogManager::instance().ItemLog(ch, item, "SHOP_INSERT", ("price " + std::to_string(p->target_cell().cell())).c_str());

	ITEM_MANAGER::instance().GetPlayerItem(item, item_data);
	*item_data->mutable_cell() = p->target_cell();
	item_data->set_price(p->price());

	shop_item->set_auction_type(p->target_cell().window_type() == AUCTION ? AuctionManager::TYPE_AUCTION : AuctionManager::TYPE_SHOP);
	shop_item->set_owner_name(ch->GetName());

#ifdef AUCTION_SYSTEM_REQUIRE_SAME_MAP
	shop_item->set_map_index(ch->GetMapIndex());
	shop_item->set_channel(g_bChannel);
#endif

	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
		return;
	}

	ITEM_MANAGER::instance().RemoveItemForFurtherUse(item);
}

void CInputMain::AuctionTakeItem(LPCHARACTER ch, std::unique_ptr<network::CGAuctionTakeItemPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_takeitem_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

#ifdef AUCTION_SYSTEM_REQUIRE_SAME_MAP
	LPCHARACTER ch_shop = AuctionShopManager::instance().find_shop(ch->GetPlayerID());
	if (!ch_shop || ch_shop->GetMapIndex() != ch->GetMapIndex())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "You need to be on the same map as your shop.");
		return;
	}
#endif

	network::GGOutputPacket<network::GGAuctionTakeItemPacket> pack;
	pack->set_owner_id(ch->GetPlayerID());
	pack->set_item_id(p->item_id());
	pack->set_inventory_pos(p->inventory_pos());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionBuyItem(LPCHARACTER ch, std::unique_ptr<network::CGAuctionBuyItemPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_buy_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (!GM::check_allow(ch->GetGMLevel(), GM_ALLOW_BUY_PRIVATE_ITEM))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot do this with this gamemaster rank."));
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (ch->GetDesc()->IsTradeblocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return;
	}
#endif

	if (p->price() <= 0)
		return;

	if (p->price() > ch->GetGold())
		return;

	network::GGOutputPacket<network::GGAuctionBuyItemPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	pack->set_player_name(ch->GetName());
	pack->set_item_id(p->item_id());
	pack->set_paid_gold(p->price());
	if (P2P_MANAGER::instance().SendProcessorCore(pack))
	{
		std::ostringstream hint;
		hint << "from " << ch->GetGold();
		ch->PointChange(POINT_GOLD, -static_cast<long long>(p->price()));
		hint << " to " << ch->GetGold() << " (amount " << p->price() << ")";

		LogManager::instance().CharLog(ch, p->item_id(), "AUCTION_BUY_TRY", hint.str().c_str());
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionTakeGold(LPCHARACTER ch, std::unique_ptr<network::CGAuctionTakeGoldPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_takegold_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (ch->GetGold() + p->gold() > GOLD_MAX)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You have too much gold."));
		return;
	}

	network::GGOutputPacket<network::GGAuctionTakeGoldPacket> pack;
	pack->set_owner_id(ch->GetPlayerID());
	pack->set_gold(p->gold());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionSearchItems(LPCHARACTER ch, std::unique_ptr<network::CGAuctionSearchItemsPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_search_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionSearchItemsPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	pack->set_page(p->page());
	pack->set_language(ch->GetLanguageID());
	*pack->mutable_options() = p->options();
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionExtendedSearchItems(LPCHARACTER ch, std::unique_ptr<network::CGAuctionExtendedSearchItemsPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_ext_search_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionExtendedSearchItemsPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	pack->set_page(p->page());
	pack->set_language(ch->GetLanguageID());
	*pack->mutable_options() = p->options();

#ifndef SHOPSEARCH_INMAP_ONLY
	if (ch->GetQuestFlag("auction_premium.premium_active") == 0)
#endif
	{
		pack->set_map_index(ch->GetMapIndex());
		pack->set_channel(g_bChannel);
	}

	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionMarkShop(LPCHARACTER ch, std::unique_ptr<network::CGAuctionMarkShopPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_mark_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionMarkShopPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	pack->set_item_id(p->item_id());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionShopRequestShow(LPCHARACTER ch)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shopshow_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionShopRequestShowPacket> pack;
	pack->set_pid(ch->GetPlayerID());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionShopOpen(LPCHARACTER ch, std::unique_ptr<network::CGAuctionShopOpenPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shopopen_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (!ch->CanHandleItem())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "Try again in a few seconds..."));
		return;
	}

	if (ch->IsStun() || ch->IsDead())
		return;

	if (ch->GetExchange() || ch->IsOpenSafebox() || ch->GetShop() || ch->IsCubeOpen())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´Ù¸¥ °Å·¡ÁßÀÏ°æ¿ì °³ÀÎ»óÁ¡À» ¿­¼ö°¡ ¾ø½À´Ï´Ù."));
		return;
	}

	if (ch->GetGold() < AuctionManager::SHOP_CREATE_COST)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need %i yang to create an offline shop."), AuctionManager::SHOP_CREATE_COST);
		return;
	}

	if (!GM::check_allow(ch->GetGMLevel(), GM_ALLOW_CREATE_PRIVATE_SHOP))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You cannot do this with this gamemaster rank."));
		return;
	}

#ifdef ACCOUNT_TRADE_BLOCK
	if (ch->GetDesc()->IsTradeblocked())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You need to verify your new computer login by clicking on the link in the email we've sent you or wait %d hours."), ACCOUNT_TRADE_BLOCK);
		return;
	}
#endif

	if (AuctionManager::SHOP_MIN_DISTANCE_BETWEEN > 0)
	{
		if (auto sectree = ch->GetSectree())
		{
			bool ret = true;
			sectree->ForEachAround([ch, &ret](LPENTITY ent) {
				if (!ret)
					return;

				if (!ent->IsType(ENTITY_CHARACTER))
					return;

				LPCHARACTER tch = (LPCHARACTER) ent;
				if (!tch->IsNPC())
					return;

				if (DISTANCE_APPROX(ch->GetX() - tch->GetX(), ch->GetY() - tch->GetY()) < AuctionManager::SHOP_MIN_DISTANCE_BETWEEN)
					ret = false;
			});

			if (!ret)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't open your shop here (too close to another shop)."));
				return;
			}
		}
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	if (pPC && pPC->IsRunning())
		return;

	if (p->items_size() == 0)
		return;

	uint64_t max_price = 0;
	std::unordered_map<LPITEM, std::pair<WORD, uint64_t>> items;
	for (auto& recv_item : p->items())
	{
		auto item = ch->GetItem(recv_item.item().cell());
		if (item == nullptr)
			return;

		if (!item->CanPutItemIntoShop())
			return;

		if (recv_item.item().price() <= 0 || recv_item.item().price() >= __SHOP_SELL_MAX_PRICE__)
			return;

		if (recv_item.display_pos() + (MAX(1, item->GetSize()) - 1) * AuctionManager::SHOP_SLOT_COUNT_X >= AuctionManager::SHOP_SLOT_COUNT)
			return;

		if (items.find(item) != items.end())
			return;

		items[item] = std::make_pair(recv_item.display_pos(), recv_item.item().price());
		max_price += recv_item.item().price();

		if (max_price > GOLD_MAX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "The summation of all item prices in the offline shop may be maximal %lld yang."), GOLD_MAX);
			return;
		}
	}

	bool is_premium = ch->GetQuestFlag("auction_premium.premium_active") != 0;

	auto color_red = -1.0f;
	auto color_green = -1.0f;
	auto color_blue = -1.0f;

	auto timeout = is_premium ? AuctionManager::SHOP_TIME_PREMIUM : AuctionManager::SHOP_TIME_DEFAULT;

	if (!is_premium)
	{
		p->set_model(0);
		p->set_style(0);
	}

	if (p->model() >= AuctionManager::SHOP_MODEL_MAX_NUM)
		p->set_model(0);
	if (p->style() >= AuctionManager::SHOP_STYLE_MAX_NUM)
		p->set_style(0);

	network::GGOutputPacket<network::GGAuctionShopOpenPacket> pack;
	pack->set_owner_id(ch->GetPlayerID());
	pack->set_owner_name(ch->GetName());

	pack->set_name(p->name());
	pack->set_style(p->style());
	pack->set_vnum(AuctionManager::SHOP_MODEL_VNUMS[p->model()]);

	pack->set_color_red(color_red);
	pack->set_color_green(color_green);
	pack->set_color_blue(color_blue);

	pack->set_channel(g_bChannel);
	pack->set_map_index(ch->GetMapIndex());

	PIXEL_POSITION basePos;
	SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(ch->GetMapIndex(), basePos);
	pack->set_x(ch->GetX() - basePos.x);
	pack->set_y(ch->GetY() - basePos.y);

	pack->set_timeout(get_global_time() + timeout);

	for (auto& pair : items)
	{
		auto item = pair.first;
		auto display_pos = pair.second.first;
		auto price = pair.second.second;

		auto pack_item = pack->add_items();
		ITEM_MANAGER::instance().GetPlayerItem(item, pack_item);

		auto cell = pack_item->mutable_cell();
		cell->set_window_type(AUCTION_SHOP);
		cell->set_cell(display_pos);

		pack_item->set_price(price);

		ITEM_MANAGER::instance().RemoveItemForFurtherUse(item);
	}

	if (P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->PointChange(POINT_GOLD, -static_cast<int64_t>(AuctionManager::SHOP_CREATE_COST));
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionShopTakeGold(LPCHARACTER ch, std::unique_ptr<network::CGAuctionShopTakeGoldPacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shoptake_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionShopTakeGoldPacket> pack;
	pack->set_owner_id(ch->GetPlayerID());
	pack->set_gold(p->gold());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionShopGuestCancel(LPCHARACTER ch)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shopcancel_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionShopViewCancelPacket> pack;
	pack->set_player_id(ch->GetPlayerID());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->GetDesc()->Packet(network::TGCHeader::AUCTION_SHOP_GUEST_CLOSE);
}

void CInputMain::AuctionShopRenew(LPCHARACTER ch)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_renewshop_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	if (ch->GetGold() < AuctionManager::SHOP_RENEW_COST)
		return;

	bool is_premium = ch->GetQuestFlag("auction_premium.premium_active") != 0;
	auto timeout = is_premium ? AuctionManager::SHOP_TIME_PREMIUM : AuctionManager::SHOP_TIME_DEFAULT;

	network::GGOutputPacket<network::GGAuctionShopRenewPacket> pack;
	pack->set_player_id(ch->GetPlayerID());
	pack->set_timeout(get_global_time() + timeout);
	if (P2P_MANAGER::instance().SendProcessorCore(pack))
	{
		ch->PointChange(POINT_GOLD, -static_cast<int64_t>(AuctionManager::SHOP_RENEW_COST));
		LogManager::instance().AuctionLog(AuctionManager::LOG_RENEW, "", ch->GetPlayerID(), ch->GetPlayerID(), timeout, 0, "paid " + std::to_string(AuctionManager::SHOP_RENEW_COST) + " gold");
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionShopClose(LPCHARACTER ch, std::unique_ptr<network::CGAuctionShopClosePacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shopclose_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

#ifdef AUCTION_SYSTEM_REQUIRE_SAME_MAP
	if (p->has_items())
	{
		LPCHARACTER ch_shop = AuctionShopManager::instance().find_shop(ch->GetPlayerID());
		if (!ch_shop || ch_shop->GetMapIndex() != ch->GetMapIndex())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "You need to be on the same map as your shop.");
			return;
		}
	}
#endif

	network::GGOutputPacket<network::GGAuctionShopClosePacket> pack;
	pack->set_player_id(ch->GetPlayerID());
	pack->set_map_index(ch->GetMapIndex());
	pack->set_channel(g_bChannel);
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionShopRequestHistory(LPCHARACTER ch)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shophistory_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionShopRequestHistoryPacket> pack;
	pack->set_player_id(ch->GetPlayerID());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}

void CInputMain::AuctionRequestAveragePrice(LPCHARACTER ch, std::unique_ptr<network::CGAuctionRequestAveragePricePacket> p)
{
	if (quest::CQuestManager::instance().GetEventFlag("auction_disable") == 1 ||
		quest::CQuestManager::instance().GetEventFlag("auction_shopaverage_disable") == 1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently disabled.");
		return;
	}

	network::GGOutputPacket<network::GGAuctionRequestAveragePricePacket> pack;
	pack->set_player_id(ch->GetPlayerID());
	pack->set_requestor(p->requestor());
	pack->set_vnum(p->vnum());
	pack->set_count(p->count());
	if (!P2P_MANAGER::instance().SendProcessorCore(pack))
		ch->ChatPacket(CHAT_TYPE_INFO, "This action is currently not possible.");
}
#endif

#ifdef CRYSTAL_SYSTEM
void CInputMain::CrystalRefine(LPCHARACTER ch, std::unique_ptr<CGCrystalRefinePacket> p)
{
	if (!ch->CanHandleItem() || ch->GetExchange())
		return;

	LPITEM crystal_item = ch->GetItem(p->crystal_cell());
	LPITEM scroll_item = ch->GetItem(p->scroll_cell());

	if (!crystal_item || crystal_item->GetType() != ITEM_CRYSTAL || static_cast<ECrystalItem>(crystal_item->GetSubType()) != ECrystalItem::CRYSTAL)
		return;

	if (!scroll_item || scroll_item->GetType() != ITEM_CRYSTAL || static_cast<ECrystalItem>(scroll_item->GetSubType()) != ECrystalItem::UPGRADE_SCROLL)
		return;

	auto clarity_type = crystal_item->GetSocket(CItem::CRYSTAL_CLARITY_TYPE_SOCKET);
	auto clarity_level = crystal_item->GetSocket(CItem::CRYSTAL_CLARITY_LEVEL_SOCKET);
	auto next_proto = CGeneralManager::instance().get_crystal_proto(clarity_type, clarity_level, true);
	if (!next_proto)
		return;

	// check enough fragments
	size_t fragment_count = 0;
	std::unordered_set<LPITEM> fragment_items;
	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if (LPITEM cur = ch->GetInventoryItem(i))
		{
			if (cur->GetType() == ITEM_CRYSTAL && static_cast<ECrystalItem>(cur->GetSubType()) == ECrystalItem::FRAGMENT && !cur->isLocked())
			{
				fragment_count += cur->GetCount();
				fragment_items.insert(cur);
			}
		}
	}

	if (fragment_count < next_proto->required_fragments())
		return;

	// remove fragments
	fragment_count = next_proto->required_fragments();
	for (auto& item : fragment_items)
	{
		size_t reduce_count = MIN(fragment_count, item->GetCount());
		item->SetCount(item->GetCount() - reduce_count);
		fragment_count -= reduce_count;

		if (fragment_count == 0)
			break;
	}

	// remove scroll
	scroll_item->SetCount(scroll_item->GetCount() - 1);

	// increase crystal level
	crystal_item->set_crystal_grade(next_proto);

	ch->GetDesc()->Packet(TGCHeader::CRYSTAL_REFINE_SUCCESS);
}
#endif

bool CInputMain::Analyze(LPDESC d, const network::InputPacket& packet)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		d->SetPhase(PHASE_CLOSE);
		return (0);
	}

	auto header = packet.get_header<TCGHeader>();
	
	if (test_server && header != TCGHeader::MOVE)
		sys_log(0, "CInputMain::Analyze() ==> Header [%d] ", packet.get_header());
	
	switch (header)
	{
		case TCGHeader::PONG:
			Pong(d); 
			break;

		case TCGHeader::HANDSHAKE:
			Handshake(d, packet.get<CGHandshakePacket>());
			break;

		case TCGHeader::CHAT:
			Chat(ch, packet.get<CGChatPacket>());
			break;

		case TCGHeader::WHISPER:
			Whisper(ch, packet.get<CGWhisperPacket>());
			break;

		case TCGHeader::MOVE:
			Move(ch, packet.get<CGMovePacket>());
			break;

		case TCGHeader::ITEM_USE:
			if (!ch->IsObserverMode() )
				ItemUse(ch, packet.get<CGItemUsePacket>());
			break;

		case TCGHeader::ITEM_DROP:
			if (!ch->IsObserverMode() )
				ItemDrop(ch, packet.get<CGItemDropPacket>());
			break;

		case TCGHeader::ITEM_MOVE:
			if (!ch->IsObserverMode() )
				ItemMove(ch, packet.get<CGItemMovePacket>());
			break;

		case TCGHeader::ITEM_PICKUP:
			if (!ch->IsObserverMode() )
				ItemPickup(ch, packet.get<CGItemPickupPacket>());
			break;

		case TCGHeader::ITEM_USE_TO_ITEM:
			if (!ch->IsObserverMode() )
				ItemToItem(ch, packet.get<CGItemUseToItemPacket>());
			break;

		case TCGHeader::GIVE_ITEM:
			if (!ch->IsObserverMode() )
				ItemGive(ch, packet.get<CGGiveItemPacket>());
			break;

		case TCGHeader::EXCHANGE_START:
		case TCGHeader::EXCHANGE_ITEM_ADD:
		case TCGHeader::EXCHANGE_ITEM_DEL:
		case TCGHeader::EXCHANGE_GOLD_ADD:
		case TCGHeader::EXCHANGE_ACCEPT:
		case TCGHeader::EXCHANGE_CANCEL:
			if (!ch->IsObserverMode())
				Exchange(ch, packet);
			break;
			
		case TCGHeader::ATTACK:
		case TCGHeader::SHOOT:
			if (!ch->IsObserverMode() )
				Attack(ch, packet);
			break;

		case TCGHeader::USE_SKILL:
			if (!ch->IsObserverMode() )
				UseSkill(ch, packet.get<CGUseSkillPacket>());
			break;

		case TCGHeader::QUICKSLOT_ADD:
			QuickslotAdd(ch, packet.get<CGQuickslotAddPacket>());
			break;

		case TCGHeader::QUICKSLOT_DELETE:
			QuickslotDelete(ch, packet.get<CGQuickslotDeletePacket>());
			break;

		case TCGHeader::QUICKSLOT_SWAP:
			QuickslotSwap(ch, packet.get<CGQuickslotSwapPacket>());
			break;

		case TCGHeader::SHOP_BUY:
		case TCGHeader::SHOP_SELL:
		case TCGHeader::SHOP_END:
			Shop(ch, packet);
			break;

		case TCGHeader::MESSENGER_ADD_BY_VID:
		case TCGHeader::MESSENGER_ADD_BY_NAME:
		case TCGHeader::MESSENGER_REMOVE:
		case TCGHeader::MESSENGER_ADD_BLOCK_BY_VID:
		case TCGHeader::MESSENGER_ADD_BLOCK_BY_NAME:
		case TCGHeader::MESSENGER_REMOVE_BLOCK:
			Messenger(ch, packet);
			break;

		case TCGHeader::ON_CLICK:
			OnClick(ch, packet.get<CGOnClickPacket>());
			break;

		case TCGHeader::ON_HIT_SPACEBAR:
			OnHitSpacebar(ch);
			break;

		case TCGHeader::ON_QUEST_TRIGGER:
			OnQuestTrigger(ch, packet.get<CGOnQuestTriggerPacket>());
			break;

		case TCGHeader::SYNC_POSITION:
			SyncPosition(ch, packet.get<CGSyncPositionPacket>());
			break;

		case TCGHeader::ADD_FLY_TARGET:
		case TCGHeader::FLY_TARGET:
			FlyTarget(ch, packet.get<CGFlyTargetPacket>());
			break;

		case TCGHeader::SCRIPT_BUTTON:
			ScriptButton(ch, packet.get<CGScriptButtonPacket>());
			break;

			// SCRIPT_SELECT_ITEM
		case TCGHeader::SCRIPT_SELECT_ITEM:
			ScriptSelectItem(ch, packet.get<CGScriptSelectItemPacket>());
			break;
			// END_OF_SCRIPT_SELECT_ITEM

		case TCGHeader::SCRIPT_ANSWER:
			ScriptAnswer(ch, packet.get<CGScriptAnswerPacket>());
			break;

		case TCGHeader::QUEST_INPUT_STRING:
			QuestInputString(ch, packet.get<CGQuestInputStringPacket>());
			break;

		case TCGHeader::QUEST_CONFIRM:
			QuestConfirm(ch, packet.get<CGQuestConfirmPacket>());
			break;

		case TCGHeader::TARGET:
			Target(ch, packet.get<CGTargetPacket>());
			break;

		case TCGHeader::WARP:
			Warp(ch);
			break;

		case TCGHeader::SAFEBOX_CHECKIN:
			SafeboxCheckin(ch, packet.get<CGSafeboxCheckinPacket>());
			break;

		case TCGHeader::SAFEBOX_CHECKOUT:
			SafeboxCheckout(ch, packet.get<CGSafeboxCheckoutPacket>());
			break;

		case TCGHeader::SAFEBOX_ITEM_MOVE:
			SafeboxItemMove(ch, packet.get<CGSafeboxItemMovePacket>());
			break;

		case TCGHeader::PARTY_INVITE:
			PartyInvite(ch, packet.get<CGPartyInvitePacket>());
			break;

		case TCGHeader::PARTY_REMOVE:
			PartyRemove(ch, packet.get<CGPartyRemovePacket>());
			break;

		case TCGHeader::PARTY_INVITE_ANSWER:
			PartyInviteAnswer(ch, packet.get<CGPartyInviteAnswerPacket>());
			break;

		case TCGHeader::PARTY_SET_STATE:
			PartySetState(ch, packet.get<CGPartySetStatePacket>());
			break;

		case TCGHeader::PARTY_USE_SKILL:
			PartyUseSkill(ch, packet.get<CGPartyUseSkillPacket>());
			break;

		case TCGHeader::PARTY_PARAMETER:
			PartyParameter(ch, packet.get<CGPartyParameterPacket>());
			break;

		case TCGHeader::GUILD_ANSWER_MAKE:
			AnswerMakeGuild(ch, packet.get<CGGuildAnswerMakePacket>());
			break;

		case TCGHeader::GUILD_DEPOSIT_MONEY:
		case TCGHeader::GUILD_WITHDRAW_MONEY:
		case TCGHeader::GUILD_ADD_MEMBER:
		case TCGHeader::GUILD_REMOVE_MEMBER:
		case TCGHeader::GUILD_CHANGE_GRADE_NAME:
		case TCGHeader::GUILD_CHANGE_GRADE_AUTHORITY:
		case TCGHeader::GUILD_OFFER_EXP:
		case TCGHeader::GUILD_CHARGE_GSP:
		case TCGHeader::GUILD_POST_COMMENT:
		case TCGHeader::GUILD_DELETE_COMMENT:
		case TCGHeader::GUILD_REFRESH_COMMENT:
		case TCGHeader::GUILD_CHANGE_MEMBER_GRADE:
		case TCGHeader::GUILD_USE_SKILL:
		case TCGHeader::GUILD_CHANGE_MEMBER_GENERAL:
		case TCGHeader::GUILD_INVITE_ANSWER:
		case TCGHeader::GUILD_REQUEST_LIST:
		case TCGHeader::GUILD_SEARCH:
			Guild(ch, packet);
			break;

		case TCGHeader::FISHING:
			Fishing(ch, packet.get<CGFishingPacket>());
			break;

		case TCGHeader::HACK:
			Hack(ch, packet.get<CGHackPacket>());
			break;

		case TCGHeader::MYSHOP:
			MyShop(ch, packet.get<CGMyShopPacket>());
			break;

		case TCGHeader::REFINE:
			Refine(ch, packet.get<CGRefinePacket>());
			break;

		case TCGHeader::CLIENT_VERSION:
			Version(ch, packet.get<CGClientVersionPacket>());
			break;

#ifdef __IPV6_FIX__
		case TCGHeader::IPV6_FIX_ENABLE:
			if (d->GetCharacter() )
				d->GetCharacter()->SetIPV6FixEnabled( );
			break;
#endif

#ifdef __ACCE_COSTUME__
		case TCGHeader::ACCE_REFINE_ACCEPT:
		case TCGHeader::ACCE_REFINE_CHECKIN:
		case TCGHeader::ACCE_REFINE_CHECKOUT:
		case TCGHeader::ACCE_REFINE_CANCEL:
			AcceRefine(ch, packet);
			break;
#endif

#ifdef ENABLE_RUNE_SYSTEM
		case TCGHeader::RUNE_PAGE:
			RunePage(ch, packet.get<CGRunePagePacket>());
			break;
#endif
			
#ifdef __GUILD_SAFEBOX__
		case TCGHeader::GUILD_SAFEBOX_OPEN:
		case TCGHeader::GUILD_SAFEBOX_CHECKIN:
		case TCGHeader::GUILD_SAFEBOX_CHECKOUT:
		case TCGHeader::GUILD_SAFEBOX_ITEM_MOVE:
		case TCGHeader::GUILD_SAFEBOX_GIVE_GOLD:
		case TCGHeader::GUILD_SAFEBOX_GET_GOLD:
			GuildSafebox(ch, packet);
			break;
#endif

		case TCGHeader::ITEM_DESTROY:
			if (!ch->IsObserverMode() )
				ItemDestroy(ch, packet.get<CGItemDestroyPacket>());
			break;

#ifdef __DRAGONSOUL__
		case TCGHeader::DRAGON_SOUL_REFINE:
			{
				auto p = packet.get<CGDragonSoulRefinePacket>();
				if (p->item_grid_size() != DRAGON_SOUL_REFINE_GRID_SIZE)
					return false;

				::TItemPos tmp_grid[DRAGON_SOUL_REFINE_GRID_SIZE];
				for (int i = 0; i < DRAGON_SOUL_REFINE_GRID_SIZE; ++i)
					tmp_grid[i] = p->item_grid(i);

				switch(p->sub_type())
				{
				case DS_SUB_HEADER_CLOSE:
					ch->DragonSoul_RefineWindow_Close();
					break;
				case DS_SUB_HEADER_DO_REFINE_GRADE:
					{
						DSManager::instance().DoRefineGrade(ch, tmp_grid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STEP:
					{
						DSManager::instance().DoRefineStep(ch, tmp_grid);
					}
					break;
				case DS_SUB_HEADER_DO_REFINE_STRENGTH:
					{
						DSManager::instance().DoRefineStrength(ch, tmp_grid);
					}
					break;
				}
			}

			break;
#endif

		case TCGHeader::TARGET_MONSTER_DROP_INFO:
			TargetMonsterDropInfo(ch, packet.get<CGTargetMonsterDropInfoPacket>());
			break;

		case TCGHeader::PLAYER_LANGUAGE_INFORMATION:
			PlayerLanguageInformation(ch, packet.get<CGPlayerLanguageInformationPacket>());
			break;

		case TCGHeader::ITEM_MULTI_USE:
			if (!ch->IsObserverMode() )
				ItemMultiUse(ch, packet.get<CGItemMultiUsePacket>());
			break;

#ifdef __PYTHON_REPORT_PACKET__
		case TCGHeader::BOT_REPORT_LOG:
			BotReportLog(ch, packet.get<CGBotReportLogPacket>());
			break;
#endif

		case TCGHeader::FORCED_REWARP:
			ForcedRewarp(ch, packet.get<CGForcedRewarpPacket>());
			break;

		case TCGHeader::USE_DETACHMENT_SINGLE:
			UseDetachmentSingle(ch, packet.get<CGUseDetachmentSinglePacket>());
			break;

#ifdef __EVENT_MANAGER__
		case TCGHeader::EVENT_REQUEST_ANSWER:
			EventRequestAnswer(ch, packet.get<CGEventRequestAnswerPacket>());
			break;
#endif

#ifdef __COSTUME_BONUS_TRANSFER__
		case TCGHeader::COSTUME_BONUS_TRANSFER:
			CostumeBonusTransfer(ch, packet.get<CGCostumeBonusTransferPacket>());
			break;
#endif

#ifdef COMBAT_ZONE
		case TCGHeader::COMBAT_ZONE_REQUEST_ACTION:
			CCombatZoneManager::instance().RequestAction( ch, packet.get<CGCombatZoneRequestActionPacket>());
			break;
#endif

#ifdef CHANGE_SKILL_COLOR
		case TCGHeader::SET_SKILL_COLOR:
			SetSkillColor(ch, packet.get<CGSetSkillColorPacket>());
			break;
#endif

#ifdef INGAME_WIKI
		case TCGHeader::RECV_WIKI:
			RecvWikiPacket(ch, packet.get<CGRecvWikiPacket>());
			break;
#endif

#ifdef __EQUIPMENT_CHANGER__
		case TCGHeader::EQUIPMENT_PAGE_ADD:
			EquipmentPageAdd(ch, packet.get<CGEquipmentPageAddPacket>());
			break;

		case TCGHeader::EQUIPMENT_PAGE_DELETE:
			EquipmentPageDelete(ch, packet.get<CGEquipmentPageDeletePacket>());
			break;

		case TCGHeader::EQUIPMENT_PAGE_SELECT:
			EquipmentPageSelect(ch, packet.get<CGEquipmentPageSelectPacket>());
			break;
#endif

#ifdef __PET_ADVANCED__
		case TCGHeader::PET_USE_EGG:
		case TCGHeader::PET_ATTR_REFINE_INFO:
		case TCGHeader::PET_ATTR_REFINE:
		case TCGHeader::PET_RESET_SKILL:
		case TCGHeader::PET_EVOLUTION_INFO:
		case TCGHeader::PET_EVOLVE:
			PetAdvanced(ch, header, packet);
			break;
#endif

#ifdef AUCTION_SYSTEM
		case TCGHeader::AUCTION_INSERT_ITEM:
			AuctionInsertItem(ch, packet.get<CGAuctionInsertItemPacket>());
			break;

		case TCGHeader::AUCTION_TAKE_ITEM:
			AuctionTakeItem(ch, packet.get<CGAuctionTakeItemPacket>());
			break;

		case TCGHeader::AUCTION_BUY_ITEM:
			AuctionBuyItem(ch, packet.get<CGAuctionBuyItemPacket>());
			break;

		case TCGHeader::AUCTION_TAKE_GOLD:
			AuctionTakeGold(ch, packet.get<CGAuctionTakeGoldPacket>());
			break;

		case TCGHeader::AUCTION_SEARCH_ITEMS:
			AuctionSearchItems(ch, packet.get<CGAuctionSearchItemsPacket>());
			break;

		case TCGHeader::AUCTION_EXTENDED_SEARCH_ITEMS:
			AuctionExtendedSearchItems(ch, packet.get<CGAuctionExtendedSearchItemsPacket>());
			break;

		case TCGHeader::AUCTION_MARK_SHOP:
			AuctionMarkShop(ch, packet.get<CGAuctionMarkShopPacket>());
			break;

		case TCGHeader::AUCTION_SHOP_REQUEST_SHOW:
			AuctionShopRequestShow(ch);
			break;

		case TCGHeader::AUCTION_SHOP_OPEN:
			AuctionShopOpen(ch, packet.get<CGAuctionShopOpenPacket>());
			break;

		case TCGHeader::AUCTION_SHOP_TAKE_GOLD:
			AuctionShopTakeGold(ch, packet.get<CGAuctionShopTakeGoldPacket>());
			break;

		case TCGHeader::AUCTION_SHOP_GUEST_CANCEL:
			AuctionShopGuestCancel(ch);
			break;

		case TCGHeader::AUCTION_SHOP_RENEW:
			AuctionShopRenew(ch);
			break;

		case TCGHeader::AUCTION_SHOP_CLOSE:
			AuctionShopClose(ch, packet.get<CGAuctionShopClosePacket>());
			break;

		case TCGHeader::AUCTION_SHOP_REQUEST_HISTORY:
			AuctionShopRequestHistory(ch);
			break;

		case TCGHeader::AUCTION_REQUEST_AVG_PRICE:
			AuctionRequestAveragePrice(ch, packet.get<CGAuctionRequestAveragePricePacket>());
			break;
#endif

#ifdef CRYSTAL_SYSTEM
		case TCGHeader::CRYSTAL_REFINE:
			CrystalRefine(ch, packet.get<CGCrystalRefinePacket>());
			break;
#endif

		default:
			sys_err("invalid header on game phase %u", packet.get_header());
			return false;
	}
	return true;
}

bool CInputDead::Analyze(LPDESC d, const InputPacket& packet)
{
	LPCHARACTER ch;

	if (!(ch = d->GetCharacter()))
	{
		sys_err("no character on desc");
		return 0;
	}

	switch (packet.get_header<TCGHeader>())
	{
		case TCGHeader::PONG:
			Pong(d); 
			break;

		case TCGHeader::HANDSHAKE:
			Handshake(d, packet.get<CGHandshakePacket>());
			break;

		case TCGHeader::CHAT:
			Chat(ch, packet.get<CGChatPacket>());
			break;

		case TCGHeader::WHISPER:
			Whisper(ch, packet.get<CGWhisperPacket>());
			break;

		case TCGHeader::HACK:
			Hack(ch, packet.get<CGHackPacket>());
			break;

#ifdef __PYTHON_REPORT_PACKET__
		case TCGHeader::BOT_REPORT_LOG:
			BotReportLog(ch, packet.get<CGBotReportLogPacket>());
			break;
#endif

		default:
			sys_err("invalid header on dead phase %u", packet.get_header());
			return false;
	}

	return true;
}

