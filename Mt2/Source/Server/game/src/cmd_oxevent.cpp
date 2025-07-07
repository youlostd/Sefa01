#include "stdafx.h"
#include "utils.h"
#include "char.h"
#include "OXEvent.h"
#include "questmanager.h"
#include "questlua.h"
#include "config.h"
#include "cmd.h"

ACMD(do_oxevent_show_quiz)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "===== OX QUIZ LIST =====");
	COXEventManager::instance().ShowQuizList(ch);
	ch->ChatPacket(CHAT_TYPE_INFO, "===== OX QUIZ LIST END =====");
}

ACMD(do_oxevent_log)
{
	if ( COXEventManager::instance().LogWinner() == false )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "OXÀÌº¥Æ®ÀÇ ³ª¸ÓÁö ÀÎ¿øÀ» ±â·ÏÇÏ¿´½À´Ï´Ù."));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "OXÀÌº¥Æ®ÀÇ ³ª¸ÓÁö ÀÎ¿ø ±â·ÏÀ» ½ÇÆÐÇß½À´Ï´Ù."));
	}
}

ACMD(do_oxevent_get_attender)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(ch, "ÇöÀç ³²Àº Âü°¡ÀÚ¼ö : %d"), COXEventManager::instance().GetAttenderCount());
}

