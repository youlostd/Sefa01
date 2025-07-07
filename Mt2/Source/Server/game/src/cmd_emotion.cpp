#include "stdafx.h"
#include "utils.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "wedding.h"
#include "event_manager.h"

#define NEED_TARGET	(1 << 0)
#define NEED_PC		(1 << 1)
#define WOMAN_ONLY	(1 << 2)
#define OTHER_SEX_ONLY	(1 << 3)
#define SELF_DISARM	(1 << 4)
#define TARGET_DISARM	(1 << 5)
#define BOTH_DISARM	(SELF_DISARM | TARGET_DISARM)

#ifdef ENABLE_MESSENGER_BLOCK
#include "messenger_manager.h"
#endif

struct emotion_type_s emotion_types[34] = {
	{ "Å°½º",	"french_kiss",	NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		2.0f },
	{ "»Ç»Ç",	"kiss",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		1.5f },
	{ "µû±Í",	"slap",		NEED_PC | SELF_DISARM,				1.5f },
	{ "¹Ú¼ö",	"clap",		0,						1.0f },
	{ "¿Í",		"cheer1",	0,						1.0f },
	{ "¸¸¼¼",	"cheer2",	0,						1.0f },
	
	// DANCE
	{ "´í½º1",	"dance1",	0,						1.0f },
	{ "´í½º2",	"dance2",	0,						1.0f },
	{ "´í½º3",	"dance3",	0,						1.0f },
	{ "´í½º4",	"dance4",	0,						1.0f },
	{ "´í½º5",	"dance5",	0,						1.0f },
	{ "´í½º6",	"dance6",	0,						1.0f },
	// END_OF_DANCE
	{ "ÃàÇÏ",	"congratulation",	0,				1.0f	},
	{ "¿ë¼­",	"forgive",			0,				1.0f	},
	{ "È­³²",	"angry",			0,				1.0f	},
	{ "À¯È¤",	"attractive",		0,				1.0f	},
	{ "½½ÇÄ",	"sad",				0,				1.0f	},
	{ "ºê²ô",	"shy",				0,				1.0f	},
	{ "ÀÀ¿ø",	"cheerup",			0,				1.0f	},
	{ "ÁúÅõ",	"banter",			0,				1.0f	},
	{ "±â»Ý",	"joy",				0,				1.0f	},
#ifdef ENABLE_NEW_EMOTES
	{ "new_emote1",		"new_emote1",	0,	1.0f },
	{ "new_emote2",		"new_emote2",	0,	1.0f },
	{ "new_emote3",		"new_emote3",	0,	1.0f },
	{ "new_emote4",		"new_emote4",	0,	1.0f },
	{ "new_emote5",		"new_emote5",	0,	1.0f },
	{ "new_emote6",		"new_emote6",	0,	1.0f },
	{ "new_emote7",		"new_emote7",	0,	1.0f },
	{ "new_emote8",		"new_emote8",	0,	1.0f },
	{ "new_emote9",		"new_emote9",	0,	1.0f },
	{ "new_emote10",	"new_emote10",	0,	1.0f },
	{ "new_emote11",	"new_emote11",	0,	1.0f },
	{ "new_emote12",	"new_emote12",	0,	1.0f },
#endif
	{ "\n",	"\n",		0,						0.0f },
	/*
	//{ "Å°½º",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_FRENCH_KISS,	 1.0f },
	{ "»Ç»Ç",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_KISS,		 1.0f },
	{ "²¸¾È±â",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_SHORT_HUG,	 1.0f },
	{ "Æ÷¿Ë",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_LONG_HUG,		 1.0f },
	{ "¾î±úµ¿¹«",	NEED_PC | SELF_DISARM,				MOTION_ACTION_PUT_ARMS_SHOULDER, 0.0f },
	{ "ÆÈÂ¯",		NEED_PC	| WOMAN_ONLY | SELF_DISARM,		MOTION_ACTION_FOLD_ARM,		 0.0f },
	{ "µû±Í",		NEED_PC | SELF_DISARM,				MOTION_ACTION_SLAP,		 1.5f },

	{ "ÈÖÆÄ¶÷",		0,						MOTION_ACTION_CHEER_01,		 0.0f },
	{ "¸¸¼¼",		0,						MOTION_ACTION_CHEER_02,		 0.0f },
	{ "¹Ú¼ö",		0,						MOTION_ACTION_CHEER_03,		 0.0f },

	{ "È£È£",		0,						MOTION_ACTION_LAUGH_01,		 0.0f },
	{ "Å±Å±",		0,						MOTION_ACTION_LAUGH_02,		 0.0f },
	{ "¿ìÇÏÇÏ",		0,						MOTION_ACTION_LAUGH_03,		 0.0f },

	{ "¾û¾û",		0,						MOTION_ACTION_CRY_01,		 0.0f },
	{ "ÈæÈæ",		0,						MOTION_ACTION_CRY_02,		 0.0f },

	{ "ÀÎ»ç",		0,						MOTION_ACTION_GREETING_01,	0.0f },
	{ "¹ÙÀÌ",		0,						MOTION_ACTION_GREETING_02,	0.0f },
	{ "Á¤ÁßÀÎ»ç",	0,						MOTION_ACTION_GREETING_03,	0.0f },

	{ "ºñ³­",		0,						MOTION_ACTION_INSULT_01,	0.0f },
	{ "¸ð¿å",		SELF_DISARM,					MOTION_ACTION_INSULT_02,	0.0f },
	{ "¿ìÀ¡",		0,						MOTION_ACTION_INSULT_03,	0.0f },

	{ "°¼¿ì¶×",		0,						MOTION_ACTION_ETC_01,		0.0f },
	{ "²ô´ö²ô´ö",	0,						MOTION_ACTION_ETC_02,		0.0f },
	{ "µµ¸®µµ¸®",	0,						MOTION_ACTION_ETC_03,		0.0f },
	{ "±ÜÀû±ÜÀû",	0,						MOTION_ACTION_ETC_04,		0.0f },
	{ "Æ¡",		0,						MOTION_ACTION_ETC_05,		0.0f },
	{ "»×",		0,						MOTION_ACTION_ETC_06,		0.0f },
	 */
};


std::set<std::pair<DWORD, DWORD> > s_emotion_set;

ACMD(do_emotion_allow)
{
	if ( ch->GetArena() )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD	val = 0; str_to_number(val, arg1);
#ifdef ENABLE_MESSENGER_BLOCK
	LPCHARACTER tch = CHARACTER_MANAGER::instance().Find(val);
	if (!tch || !ch)
		return;
	
	if (MessengerManager::instance().CheckMessengerList(ch->GetName(), tch->GetName(), SYST_BLOCK))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "You can't use this action because you have blocked %s."), tch->GetName());
		return;
	}
#endif
	s_emotion_set.insert(std::make_pair(ch->GetVID(), val));
}

bool CHARACTER_CanEmotion(CHARACTER& rch)
{
	// everytime allow
	return true;

	// °áÈ¥½Ä ¸Ê¿¡¼­´Â »ç¿ëÇÒ ¼ö ÀÖ´Ù.
	if (marriage::WeddingManager::instance().IsWeddingMap(rch.GetMapIndex()))
		return true;

	// ¿­Á¤ÀÇ °¡¸é Âø¿ë½Ã »ç¿ëÇÒ ¼ö ÀÖ´Ù.
	if (rch.IsEquipUniqueItem(UNIQUE_ITEM_EMOTION_MASK))
		return true;

	if (rch.IsEquipUniqueItem(UNIQUE_ITEM_EMOTION_MASK2))
		return true;

	return false;
}

ACMD(do_emotion)
{
	int i;
	{
		if (ch->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸»À» Åº »óÅÂ¿¡¼­ °¨Á¤Ç¥ÇöÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;
		}
	}

	for (i = 0; *emotion_types[i].command != '\n'; ++i)
	{
		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command))
			break;

		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command_to_client))
			break;
	}

	if (*emotion_types[i].command == '\n')
	{
		sys_err("cannot find emotion");
		return;
	}

#ifdef ENABLE_REACT_EVENT
	CEventManager::instance().React_OnPlayerReact(ch, emotion_types[i].command_to_client);
#endif

	if (!CHARACTER_CanEmotion(*ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¿­Á¤ÀÇ °¡¸éÀ» Âø¿ë½Ã¿¡¸¸ ÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}

	if (IS_SET(emotion_types[i].flag, WOMAN_ONLY) && SEX_MALE==GET_SEX(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¿©ÀÚ¸¸ ÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER victim = NULL;

	if (*arg1)
		victim = ch->FindCharacterInView(arg1, IS_SET(emotion_types[i].flag, NEED_PC));

	if (IS_SET(emotion_types[i].flag, NEED_TARGET | NEED_PC))
	{
		if (!victim)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "±×·± »ç¶÷ÀÌ ¾ø½À´Ï´Ù."));
			return;
		}
	}

	if (victim)
	{
		if (!victim->IsPC() || victim == ch)
			return;

		if (victim->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "¸»À» Åº »ó´ë¿Í °¨Á¤Ç¥ÇöÀ» ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return;
		}

		long distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

		if (distance < 10)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³Ê¹« °¡±îÀÌ ÀÖ½À´Ï´Ù."));
			return;
		}

		if (distance > 500)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "³Ê¹« ¸Ö¸® ÀÖ½À´Ï´Ù"));
			return;
		}

		if (IS_SET(emotion_types[i].flag, OTHER_SEX_ONLY))
		{
			if (GET_SEX(ch)==GET_SEX(victim))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ¼º°£¿¡¸¸ ÇÒ ¼ö ÀÖ½À´Ï´Ù."));
				return;
			}
		}

		if (IS_SET(emotion_types[i].flag, NEED_PC))
		{
			if (s_emotion_set.find(std::make_pair(victim->GetVID(), ch->GetVID())) == s_emotion_set.end())
			{
				if (true == marriage::CManager::instance().IsMarried( ch->GetPlayerID() ))
				{
					const marriage::TMarriage* marriageInfo = marriage::CManager::instance().Get( ch->GetPlayerID() );

					const DWORD other = marriageInfo->GetOther( ch->GetPlayerID() );

					if (0 == other || other != victim->GetPlayerID())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ Çàµ¿Àº »óÈ£µ¿ÀÇ ÇÏ¿¡ °¡´É ÇÕ´Ï´Ù."));
						return;
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÀÌ Çàµ¿Àº »óÈ£µ¿ÀÇ ÇÏ¿¡ °¡´É ÇÕ´Ï´Ù."));
					return;
				}
			}

			s_emotion_set.insert(std::make_pair(ch->GetVID(), victim->GetVID()));
		}
	}

	char chatbuf[256+1];
	int len = snprintf(chatbuf, sizeof(chatbuf), "%s %u %u", 
			emotion_types[i].command_to_client,
			(DWORD) ch->GetVID(), victim ? (DWORD) victim->GetVID() : 0);

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	++len;  // \0 ¹®ÀÚ Æ÷ÇÔ

	network::GCOutputPacket<network::GCChatPacket> pack_chat;
	pack_chat->set_type(CHAT_TYPE_COMMAND);
	pack_chat->set_message(chatbuf);

	ch->PacketAround(pack_chat);

	if (victim)
		sys_log(1, "ACTION: %s TO %s", emotion_types[i].command, victim->GetName());
	else
		sys_log(1, "ACTION: %s", emotion_types[i].command);
}

