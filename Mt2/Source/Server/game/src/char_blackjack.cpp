#include "stdafx.h"
#ifdef BLACKJACK
#include "char.h"
#include "utils.h"
#include "log.h"
#include "db.h"
#include <stdlib.h>
#include <sstream>
#include "desc.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "packet.h"
#include "questmanager.h"
#include "desc_client.h"
#include <random>
#include <algorithm>

void CHARACTER::BlackJack_Start(BYTE stake)
{
    if (bBlackJackStatus > 0)
    {
        tchat("Game already running");
        return;
    }

    if (quest::CQuestManager::Instance().GetEventFlag("blackjack_event") != 1)
    {
        tchat("blackjack event not started flag blackjack_event != 1");
        return;
    }    

    if (CountSpecifyItem(BLACKJACK_STAKE_ITEM) < stake)
    {
        auto pTable = ITEM_MANAGER::instance().GetTable(BLACKJACK_STAKE_ITEM);
        ChatPacket(CHAT_TYPE_INFO, LC_TEXT(this, "You don't have %dx %s"), stake, pTable ? pTable->locale_name(this ? this->GetLanguageID() : LANGUAGE_DEFAULT).c_str() : "");
        return;
    }

    bStake = stake;
    bBlackJackStatus = 1;

    // GM-ITEM
    if (RemoveSpecifyItem(BLACKJACK_STAKE_ITEM, stake))
        bStake += 100;

    m_vec_BlackJackDeck.clear();
    m_vec_BlackJackPcHand.clear();
    m_vec_BlackJackDealerHand.clear();

    // Gen deck
    for(char& c : BlackJackCardTypes) 
    {
        for (auto j = 0; j < BLACKJACK_MAX_CARDS/BLACKJACK_CARD_TYPES; j++)
        {
            TBlackJackCard card = BlackJackDefaultDeck[j];
            card.sCard = c + card.sCard;
            m_vec_BlackJackDeck.push_back(card);
        }
    }

    // Shuffle
    auto rng = std::mt19937(std::random_device{}());
    std::shuffle(std::begin(m_vec_BlackJackDeck), std::end(m_vec_BlackJackDeck), rng);

    // Player gets 2 Cards, Dealer gets 1 Card
    bBlackJackStatus = 1;
    BlackJack_PopCard();
    BlackJack_PopCard();
    bBlackJackStatus = 2;
    BlackJack_PopCard();
    bBlackJackStatus = 1;
    BlackJack_Check(false);

}

void CHARACTER::BlackJack_PopCard()
{
    if (m_vec_BlackJackDeck.empty())
    {
        tchat("BlackJack Deck Empty");
        return;
    }
    else if (bBlackJackStatus == 0)
    {
        tchat("BlackJack Game not running");
        return;
    }

    TBlackJackCard card = m_vec_BlackJackDeck.back();
    m_vec_BlackJackDeck.pop_back();

    if (bBlackJackStatus == 1)
        m_vec_BlackJackPcHand.push_back(card);
    else if (bBlackJackStatus == 2)
        m_vec_BlackJackDealerHand.push_back(card);

    tchat("[%s]BLACKJACK CARD %d %s %d", bBlackJackStatus==1 ? "PC" : "DEALER", bBlackJackStatus, card.sCard.c_str(), card.bPoints);
    ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK CARD %d %s %d %d", bBlackJackStatus, card.sCard.c_str(), card.bPoints, BlackJack_Count(bBlackJackStatus == 1 ? m_vec_BlackJackPcHand : m_vec_BlackJackDealerHand));
}

BYTE CHARACTER::BlackJack_Count(TBlackJackDeck m_hand)
{
    BYTE bPoints = 0;
    for (auto &hand : m_hand)
    {
        for (BYTE i = 0; i < BLACKJACK_MAX_CARDS/BLACKJACK_CARD_TYPES; i++)
        {
            std::string card = hand.sCard;
            card.erase(card.begin());
            if (BlackJackDefaultDeck[i].sCard == card)
                bPoints += BlackJackDefaultDeck[i].bPoints;
        }
    }

    if (bPoints > 21)
    {
        for (auto &hand : m_hand)
        {
            std::string card = hand.sCard;
            card.erase(card.begin());
            if (card == "A")
            {
                bPoints -= 10;
                if (bPoints <= 21)
                    break;
            }
        }
    }

    return bPoints;
}

bool CHARACTER::BlackJack_DealerThinking()
{
    if (bBlackJackStatus == 0)
        return false;

    BYTE point = BlackJack_Count(m_vec_BlackJackDealerHand);
    if (point < 17)
        return true;
    else
        return false;
}

void CHARACTER::BlackJack_Check(bool end)
{
    BYTE dealer = BlackJack_Count(m_vec_BlackJackDealerHand);
    BYTE pc = BlackJack_Count(m_vec_BlackJackPcHand);
    tchat("BlackJack_Check(%d) PC[%d] Dealer[%d]", end, pc, dealer);
    if (dealer > 21)
    {
        tchat("BLACKJACK WON BUSTED");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK WON NORMAL 0 0 0");
        BlackJack_Reward();
    }
    else if (pc > 21)
    {
        tchat("BLACKJACK LOST BUSTED");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK LOST BUSTED 0 0 0");
        LogManager::instance().CharLog(this, bStake, "BlackJack", "LOSS");
        BlackJack_End();
    }
    else if (dealer == 21 && m_vec_BlackJackDealerHand.size() == 2)
    {
        tchat("BLACKJACK LOST BLACKJACK");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK LOST LOST 0 0 0");
        LogManager::instance().CharLog(this, bStake, "BlackJack", "LOSS");
        BlackJack_End();
    }
    else if (pc == 21 && m_vec_BlackJackPcHand.size() == 2)
    {
        tchat("BLACKJACK WON BLACKJACK");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK WON BLACKJACK 0 0 0");
        BlackJack_Reward();
    }
    else if (dealer == 21)
    {
        tchat("BLACKJACK LOST NORMAL");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK LOST LOST 0 0 0");
        LogManager::instance().CharLog(this, bStake, "BlackJack", "LOSS");
        BlackJack_End();
    }
    else if (pc == 21)
    {
        tchat("BLACKJACK WON NORMAL");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK WON NORMAL 0 0 0");
        BlackJack_Reward();
    }
    else if (end && pc == dealer)
    {
        tchat("BLACKJACK LOST DRAW");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK LOST DRAW 0 0 0");
        LogManager::instance().CharLog(this, bStake, "BlackJack", "LOSS");

        bool bGMItem = false;
        if (bStake > 100)
        {
            bGMItem = true;
            bStake -= 100;
        }
        LPITEM item;
        item = AutoGiveItem(BLACKJACK_STAKE_ITEM, bStake);

        if (bGMItem)
            item->SetGMOwner(bGMItem);

        BlackJack_End();
    }
    else if (end && pc > dealer)
    {
        tchat("BLACKJACK WON NORMAL");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK WON NORMAL 0 0 0");
        BlackJack_Reward();
    }
    else if (end && dealer > pc)
    {
        tchat("BLACKJACK LOST NORMAL");
        ChatPacket(CHAT_TYPE_COMMAND, "BLACKJACK LOST NORMAL 0 0 0");
        LogManager::instance().CharLog(this, bStake, "BlackJack", "LOSS");
        BlackJack_End();
    }
}

void CHARACTER::BlackJack_Turn(BOOL hit, BYTE cdNum, BOOL pc_stay)
{
    if(cdNum > 14)
    {
        tchat("cdNum>14");
        return;
    }

    if (bBlackJackStatus == 0)
    {
        tchat("Game not running");
        return;
    }

    tchat("BlackJack_Turn[%s](hit %d, cdNum %d, pc_stay %d, pointsPC %d, pointsDealer %d)", bBlackJackStatus==1 ? "PC" : "DEALER", hit, cdNum, pc_stay, BlackJack_Count(m_vec_BlackJackPcHand), BlackJack_Count(m_vec_BlackJackDealerHand));

    if(bBlackJackStatus == 1 &&  hit && !pc_stay)
    {
        BlackJack_PopCard();
        bBlackJackStatus = 1;
        BlackJack_Check(false);
    }

    else if(BlackJack_DealerThinking())
    {
        bBlackJackStatus = 2;
        BlackJack_PopCard();
        BlackJack_Check(false);
        BlackJack_Turn(BlackJack_DealerThinking(), cdNum + 1, pc_stay);
    }
    else if(!(BlackJack_DealerThinking()))
    {
        BlackJack_Check(true);
    }
}

void CHARACTER::BlackJack_Reward()
{
    LogManager::instance().CharLog(this, bStake, "BlackJack", "WIN");
    tchat("BLACKJACK reward");

    bool bGMItem = false;
    if (bStake > 100)
    {
        bGMItem = true;
        bStake -= 100;
    }

    LPITEM item;
    if (bStake == 10)
        item = AutoGiveItem(BLACKJACK_REWARD1);
    else if (bStake == 25)
        item = AutoGiveItem(BLACKJACK_REWARD2);
    else if (bStake == 50)
        item = AutoGiveItem(BLACKJACK_REWARD3);

    if (bGMItem)
        item->SetGMOwner(bGMItem);

    BlackJack_End();
}

void CHARACTER::BlackJack_End()
{
    bBlackJackStatus = 0;
    bStake = 0;
    m_vec_BlackJackDeck.clear();
    m_vec_BlackJackPcHand.clear();
    m_vec_BlackJackDealerHand.clear();
}

#endif