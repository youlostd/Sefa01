#include "stdafx.h"
#include "char.h"
#include "utils.h"
#include "log.h"
#include "db.h"
#include "dev_log.h"
#include <stdlib.h>
#include <sstream>
#include "config.h"
#include "desc.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "questmanager.h"
#include "mob_manager.h"
#include "desc_client.h"
#include "group_text_parse_tree.h"
#include <cctype>
#include "p2p.h"
#include "entity.h"
#include "sectree_manager.h"

void CHARACTER::Cards_open(DWORD safemode)
{
	if (GetExchange() || IsOpenSafebox() || GetShop() || GetMyShop() || IsCubeOpen() ||IsAcceWindowOpen())
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "다른 거래중(창고,교환,상점)에는 사용할 수 없습니다."));
        return;
    }

	int iCardsEventType = quest::CQuestManager::Instance().GetEventFlag("cards_event_type");

	int itemVnum = iCardsEventType == CARDS_EVENT_HALLOWEEN ? 93237 : 79506;

    if (character_cards.cards_left <= 0)
    {
        if (GetGold() < 50000)
        {
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You dont have enough money."));
            return;
        }
		if (CountSpecifyItem(itemVnum) < 1)
        {
            ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You dont have any cardsets."));
            return;
        }
        PointChange(POINT_GOLD, -50000, true);
		RemoveSpecifyItem(itemVnum, 1);
        Cards_clean_list();
        character_cards.cards_left = 24;
    }
    ChatPacket(CHAT_TYPE_COMMAND, "cards open %d", safemode);
    SendUpdatedInformations();
}

void CHARACTER::Cards_clean_list()
{
    memset(&character_cards, 0, sizeof(character_cards));
    memset(&randomized_cards, 0, sizeof(randomized_cards));
}

int CHARACTER::GetEmptySpaceInHand()
{
    for (int i=0; i<MAX_CARDS_IN_HAND; ++i)
    {
        if (character_cards.cards_in_hand[i].type == 0)
            return i;
    }
    return -1;
}

void CHARACTER::Cards_pullout()
{
    int empty_space = GetEmptySpaceInHand();
    if (empty_space == -1)
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You dont have space in hands."));
        return;
    }
    if (character_cards.cards_left < 1)
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You dont have cards."));
        return;
    }
    if (GetAllCardsCount() >= 5)
    {
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You dont have space on table."));
        return;
    }
    RandomizeCards();
    SendUpdatedInformations();
}

void CHARACTER::RandomizeCards()
{
    DWORD card_type = random_number(1, 3);
    DWORD card_value = random_number(1, 8);
    if (CardWasRandomized(card_type, card_value) != false)
    {
        RandomizeCards();
    }
    else
    {
        int empty_space = GetEmptySpaceInHand();
		if (empty_space != -1)
		{
			character_cards.cards_in_hand[empty_space].type = card_type;
			character_cards.cards_in_hand[empty_space].value = card_value;
			character_cards.cards_left -= 1;
		}
    }
}

bool CHARACTER::CardWasRandomized(DWORD type, DWORD value)
{
    for (int i=0; i<24; ++i)
    {
        if (randomized_cards[i].type == type && randomized_cards[i].value == value)
        {
            return true;
        }
    }
    for (int i=0; i<24; ++i)
    {
        if (randomized_cards[i].type == 0)
        {
            randomized_cards[i].type = type;
            randomized_cards[i].value = value;
            return false;
        }
    }
    return false;
}

void CHARACTER::SendUpdatedInformations()
{
    ChatPacket(CHAT_TYPE_COMMAND, "cards info %d %d %d %d %d %d %d %d %d %d %d %d", character_cards.cards_in_hand[0].type, character_cards.cards_in_hand[0].value,
                                                                        character_cards.cards_in_hand[1].type, character_cards.cards_in_hand[1].value, character_cards.cards_in_hand[2].type,
                                                                        character_cards.cards_in_hand[2].value, character_cards.cards_in_hand[3].type, character_cards.cards_in_hand[3].value,
                                                                        character_cards.cards_in_hand[4].type, character_cards.cards_in_hand[4].value,
                                                                        character_cards.cards_left, character_cards.points);
                                                                    
    ChatPacket(CHAT_TYPE_COMMAND, "cards finfo %d %d %d %d %d %d %d", character_cards.cards_in_field[0].type, character_cards.cards_in_field[0].value, character_cards.cards_in_field[1].type,
                                                                        character_cards.cards_in_field[1].value, character_cards.cards_in_field[2].type, character_cards.cards_in_field[2].value,
                                                                        character_cards.field_points);
}

void CHARACTER::SendReward()
{
    ChatPacket(CHAT_TYPE_COMMAND, "cards reward %d %d %d %d %d %d %d", character_cards.cards_in_field[0].type, character_cards.cards_in_field[0].value, character_cards.cards_in_field[1].type,
                                                                        character_cards.cards_in_field[1].value, character_cards.cards_in_field[2].type, character_cards.cards_in_field[2].value,
                                                                        character_cards.field_points);
}

void CHARACTER::CardsDestroy(DWORD reject_index)
{
    if (reject_index + 1 > MAX_CARDS_IN_HAND)
        return;
	
    if (character_cards.cards_in_hand[reject_index].type == 0)
        return;
	
    character_cards.cards_in_hand[reject_index].type = 0;
    character_cards.cards_in_hand[reject_index].value = 0;
    SendUpdatedInformations();
}

void CHARACTER::CardsAccept(DWORD accept_index)
{
	if (test_server)
		ChatPacket(CHAT_TYPE_INFO, "%s(%d):%d", __FUNCTION__, accept_index, __LINE__);
	
    if (accept_index + 1 > MAX_CARDS_IN_HAND)
        return;
	
    if (character_cards.cards_in_hand[accept_index].type == 0)
        return;
	
	if (test_server)
		sys_log(0, "CardsAccept %d", accept_index);

    int empty_space = GetEmptySpaceInField();
    if (empty_space != -1)
    {
        character_cards.cards_in_field[empty_space].type = character_cards.cards_in_hand[accept_index].type;
        character_cards.cards_in_field[empty_space].value = character_cards.cards_in_hand[accept_index].value;
        character_cards.cards_in_hand[accept_index].type = 0;
        character_cards.cards_in_hand[accept_index].value = 0;
    }


	if (test_server)
		sys_log(0, "GetEmptySpaceInField() == %d", GetEmptySpaceInField());
    if (GetEmptySpaceInField() == -1)
    {
        if (CheckReward())
		{

			if (test_server)
				sys_log(0, "RewardCheck == TRUE");
			SendReward();
			if (test_server)
				sys_log(0, "ResetField");
            ResetField();
        }
        else
		{
			if (test_server)
				sys_log(0, "RewardCheck == FALSE");
            RestoreField();
        }
	}
	if (test_server)
		sys_log(0, "SendUpdatedInfos");
    SendUpdatedInformations();
}

void CHARACTER::CardsRestore(DWORD restore_index)
{
    if (restore_index + 1 > MAX_CARDS_IN_FIELD)
        return;
	
    if (character_cards.cards_in_field[restore_index].type == 0)
        return;
	
    int empty_space = GetEmptySpaceInHand();
	if (empty_space == -1)
	{
		sys_err("ERROR");
		return;
	}
    character_cards.cards_in_hand[empty_space].type = character_cards.cards_in_field[restore_index].type;
    character_cards.cards_in_hand[empty_space].value = character_cards.cards_in_field[restore_index].value;
    character_cards.cards_in_field[restore_index].type = 0;
    character_cards.cards_in_field[restore_index].value = 0;
    SendUpdatedInformations();
}

int CHARACTER::GetEmptySpaceInField()
{
    for (int i=0; i<MAX_CARDS_IN_FIELD; ++i)
    {
        if (character_cards.cards_in_field[i].type == 0)
            return i;
    }
    return -1;
}

DWORD CHARACTER::GetAllCardsCount()
{
    DWORD count = 0;
    for (int i=0; i<MAX_CARDS_IN_HAND; ++i)
    {
        if (character_cards.cards_in_hand[i].type != 0)
            count += 1;
    }
    for (int i=0; i<MAX_CARDS_IN_FIELD; ++i)
    {
        if (character_cards.cards_in_field[i].type != 0)
            count += 1;
    }
    return count;
}
bool CHARACTER::TypesAreSame()
{
    if (character_cards.cards_in_field[0].type == character_cards.cards_in_field[1].type && character_cards.cards_in_field[1].type == character_cards.cards_in_field[2].type)
        return true;
    return false;
}

bool CHARACTER::ValuesAreSame()
{
    if (character_cards.cards_in_field[0].value == character_cards.cards_in_field[1].value && character_cards.cards_in_field[1].value == character_cards.cards_in_field[2].value)
        return true;
    return false;
}

bool CHARACTER::CardsMatch()
{
    if (character_cards.cards_in_field[0].value == character_cards.cards_in_field[1].value - 1 && character_cards.cards_in_field[1].value == character_cards.cards_in_field[2].value -1)
        return true;
    else if (character_cards.cards_in_field[0].value == character_cards.cards_in_field[2].value - 1 && character_cards.cards_in_field[2].value == character_cards.cards_in_field[1].value -1)
        return true;
    else if (character_cards.cards_in_field[1].value == character_cards.cards_in_field[0].value - 1 && character_cards.cards_in_field[0].value == character_cards.cards_in_field[2].value -1)
        return true;
    else if (character_cards.cards_in_field[0].value == character_cards.cards_in_field[1].value - 1 && character_cards.cards_in_field[2].value == character_cards.cards_in_field[0].value -1)
        return true;
    else if (character_cards.cards_in_field[1].value == character_cards.cards_in_field[0].value - 1 && character_cards.cards_in_field[2].value == character_cards.cards_in_field[1].value -1)
        return true;
    else if (character_cards.cards_in_field[1].value == character_cards.cards_in_field[2].value - 1 && character_cards.cards_in_field[2].value == character_cards.cards_in_field[0].value -1)
        return true;
    else
        return false;
}

DWORD CHARACTER::GetLowestCard()
{
    return MIN(character_cards.cards_in_field[0].value, MIN(character_cards.cards_in_field[1].value, character_cards.cards_in_field[2].value));
}
bool CHARACTER::CheckReward()
{
    if (TypesAreSame() && ValuesAreSame())
    {
        character_cards.field_points = 150;
        character_cards.points += 150;
        return true;
    }
    else if (TypesAreSame() && CardsMatch())
    {
        character_cards.field_points = 100;
        character_cards.points += 100;
        return true;
    }
    else if (ValuesAreSame())
    {
        character_cards.field_points = GetLowestCard()*10+10;
        character_cards.points += GetLowestCard()*10+10;
        return true;
    }
    else if (CardsMatch())
    {
        character_cards.field_points = GetLowestCard()*10;
        character_cards.points += GetLowestCard()*10;
        return true;
    }
    else
    {
        RestoreField();
        return false;
    }
}

void CHARACTER::RestoreField()
{
    for (int i=0; i<MAX_CARDS_IN_FIELD; ++i)
    {
        int empty_space = GetEmptySpaceInHand();
		if (empty_space < 0)
		{
			sys_err("no empty space for card in field %d", i);
			break;
		}
        character_cards.cards_in_hand[empty_space].type = character_cards.cards_in_field[i].type;
        character_cards.cards_in_hand[empty_space].value = character_cards.cards_in_field[i].value;
        character_cards.cards_in_field[i].type = 0;
        character_cards.cards_in_field[i].value = 0;
    }
	SendUpdatedInformations();
}

void CHARACTER::ResetField()
{
    for (int i=0; i<MAX_CARDS_IN_FIELD; ++i)
    {
        character_cards.cards_in_field[i].type = 0;
        character_cards.cards_in_field[i].value = 0;
    }
}

void CHARACTER::CardsEnd()
{
	int iCardsEventType = quest::CQuestManager::Instance().GetEventFlag("cards_event_type");

	if (iCardsEventType == CARDS_EVENT_HALLOWEEN)
	{
		if (character_cards.points >= 500)
			AutoGiveItem(93241); //Diamond box
		else if (character_cards.points >= 400)
			AutoGiveItem(93240); //Golden box
		else if (character_cards.points < 400 && character_cards.points >= 250)
			AutoGiveItem(93239); //Silver box
		else if (character_cards.points > 0)
			AutoGiveItem(93238); //Bronze box
	}
	else
	{
		if (character_cards.points >= 400)
			AutoGiveItem(50267); //Golden box
		else if (character_cards.points < 400 && character_cards.points >= 250)
			AutoGiveItem(50268); //Silver box
		else if (character_cards.points > 0)
			AutoGiveItem(50269); //Bronze box
	}

    LogManager::instance().OkayEventLog(GetPlayerID(), GetName(), character_cards.points);
    Cards_clean_list();
    SendUpdatedInformations();
}

void CHARACTER::GetGlobalRank(char * buffer, size_t buflen)
{
    std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery("SELECT name, SUM(points) as spoints FROM log.okay_event GROUP BY pid ORDER BY spoints DESC LIMIT 10"));
    if (pMsg->Get()->uiNumRows == 0)
    {
        return;
    }

    MYSQL_ROW row;
    int pos = 1;
    int len = 0, len2;
    *buffer = '\0';
    while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
    {
        int points = 0;
        str_to_number(points, row[1]);
        len2 = snprintf(buffer + len, buflen - len, "[ENTER]");

        if (len2 < 0 || len2 >= (int) buflen - len)
            len += (buflen - len) - 1;
        else
            len += len2;

        len2 = snprintf(buffer + len, buflen - len, "%d. Place - %s - %d points.",
                pos,
                row[0],
                points);
        ++pos;
        if (len2 < 0 || len2 >= (int) buflen - len)
            len += (buflen - len) - 1;
        else
            len += len2;
    }
}

void CHARACTER::GetRundRank(char * buffer, size_t buflen)
{
    std::auto_ptr<SQLMsg> pMsg(DBManager::Instance().DirectQuery("SELECT name, points FROM log.okay_event ORDER BY points DESC LIMIT 10"));
    if (pMsg->Get()->uiNumRows == 0)
    {
        return;
    }

    MYSQL_ROW row;
    int pos = 1;
    int len = 0, len2;
    *buffer = '\0';
    while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
    {
        int points = 0;
        str_to_number(points, row[1]);
        len2 = snprintf(buffer + len, buflen - len, "[ENTER]");

        if (len2 < 0 || len2 >= (int) buflen - len)
            len += (buflen - len) - 1;
        else
            len += len2;

        len2 = snprintf(buffer + len, buflen - len, "%d. Place - %s - %d points.",
                pos,
                row[0],
                points);
        ++pos;
        if (len2 < 0 || len2 >= (int) buflen - len)
            len += (buflen - len) - 1;
        else
            len += len2;
    }
}
