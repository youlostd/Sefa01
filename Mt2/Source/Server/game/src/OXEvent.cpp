#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "questmanager.h"
#include "start_position.h"
#include "packet.h"
#include "buffer_manager.h"
#include "log.h"
#include "char.h"
#include "char_manager.h"
#include "OXEvent.h"
#include "desc.h"
#include "sectree_manager.h"
#include "cmd.h"

bool COXEventManager::Initialize()
{
	m_timedEvent = NULL;
	m_map_char.clear();
	m_map_attender.clear();
	m_vec_quiz.clear();

	SetStatus(OXEVENT_FINISH);

	return true;
}

void COXEventManager::Destroy()
{
	CloseEvent();

	m_map_char.clear();
	m_map_attender.clear();
	m_vec_quiz.clear();

	SetStatus(OXEVENT_FINISH);
}

OXEventStatus COXEventManager::GetStatus()
{
	unsigned char ret = quest::CQuestManager::instance().GetEventFlag("oxevent_status");

	switch (ret)
	{
	case 0:
		return OXEVENT_FINISH;

	case 1:
		return OXEVENT_OPEN;

	case 2:
		return OXEVENT_CLOSE;

	case 3:
		return OXEVENT_QUIZ;

	default:
		return OXEVENT_ERR;
	}

	return OXEVENT_ERR;
}

void COXEventManager::SetStatus(OXEventStatus status)
{
	unsigned char val = 0;
#ifdef ENABLE_ZODIAC_TEMPLE
	BYTE zodiac = 0;
#endif
	switch (status)
	{
	case OXEVENT_OPEN:
		val = 1;
		break;

	case OXEVENT_CLOSE:
		val = 2;
		break;

	case OXEVENT_QUIZ:
		val = 3;
		break;

	case OXEVENT_FINISH:
	case OXEVENT_ERR:
	default:
		val = 0;
		break;
	}
	quest::CQuestManager::instance().RequestSetEventFlag("oxevent_status", val);
#ifdef ENABLE_ZODIAC_TEMPLE
	quest::CQuestManager::instance().RequestSetEventFlag("zodiac_portals_per_day", zodiac);
#endif
}

bool COXEventManager::Enter(LPCHARACTER pkChar)
{
	if (GetStatus() == OXEVENT_FINISH)
	{
		sys_log(0, "OXEVENT : map finished. but char enter. %s", pkChar->GetName());
		return false;
	}

	PIXEL_POSITION pos = pkChar->GetXYZ();
	const PIXEL_POSITION& basePos = GetMapBase();

	pos.x = (pos.x - basePos.x) / 100;
	pos.y = (pos.y - basePos.y) / 100;

	if (pos.x >= 223 && pos.x <= 297 && pos.y >= 230 && pos.y <= 263)
	{
		return EnterAttender(pkChar);
	}
	else if (pos.x == 261 && pos.y == 219)
	{
		return EnterAudience(pkChar);
	}
	else
	{
		sys_log(0, "OXEVENT : wrong pos enter %d %d", pos.x, pos.y);
		return false;
	}

	return false;
}

const PIXEL_POSITION& COXEventManager::GetMapBase()
{
	static PIXEL_POSITION basePos;
	if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(OXEVENT_MAP_INDEX, basePos))
	{
		basePos.x = 0;
		basePos.y = 0;
	}

	return basePos;
}

bool COXEventManager::EnterAttender(LPCHARACTER pkChar)
{
	if (GetStatus() != OXEVENT_OPEN)
	{
		sys_err("cannot join ox %u %s (oxevent is not open)", pkChar->GetPlayerID(), pkChar->GetName());
		return false;
	}

	DWORD pid = pkChar->GetPlayerID();

	m_map_char.insert(std::make_pair(pid, pid));
	m_map_attender.insert(std::make_pair(pid, pid));

	pkChar->RemoveGoodAffect();

	return true;
}

bool COXEventManager::EnterAudience(LPCHARACTER pkChar)
{
	DWORD pid = pkChar->GetPlayerID();

	m_map_char.insert(std::make_pair(pid, pid));

	return true;
}

bool COXEventManager::AddQuiz(unsigned char level, const char** pszQuestions, bool answer)
{
	if (m_vec_quiz.size() < (size_t)level + 1)
		m_vec_quiz.resize(level + 1);

	struct tag_Quiz tmpQuiz;

	tmpQuiz.level = level;
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
	{
		strlcpy(tmpQuiz.Quiz[i], pszQuestions[i], sizeof(tmpQuiz.Quiz[i]));
	}
	tmpQuiz.answer = answer;

	m_vec_quiz[level].push_back(tmpQuiz);
	return true;
}

bool COXEventManager::ShowQuizList(LPCHARACTER pkChar)
{
	int c = 0;

	for (size_t i = 0; i < m_vec_quiz.size(); ++i)
	{
		for (size_t j = 0; j < m_vec_quiz[i].size(); ++j, ++c)
		{
			pkChar->ChatPacket(CHAT_TYPE_INFO, "%d %s %s", m_vec_quiz[i][j].level, m_vec_quiz[i][j].Quiz[pkChar->GetLanguageID()], m_vec_quiz[i][j].answer ? LC_TEXT(pkChar, "Âü") : LC_TEXT(pkChar, "°ÅÁþ"));
		}
	}

	pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChar, "ÃÑ ÄûÁî ¼ö: %d"), c);
	return true;
}

void COXEventManager::ClearQuiz()
{
	for (unsigned int i = 0; i < m_vec_quiz.size(); ++i)
	{
		m_vec_quiz[i].clear();
	}

	m_vec_quiz.clear();
}

EVENTINFO(OXEventInfoData)
{
	bool answer;

	OXEventInfoData()
		: answer(false)
	{
	}
};

EVENTFUNC(oxevent_timer)
{
	static unsigned char flag = 0;
	OXEventInfoData* info = dynamic_cast<OXEventInfoData*>(event->info);

	if (info == NULL)
	{
		sys_err("oxevent_timer> <Factor> Null pointer");
		return 0;
	}

	switch (flag)
	{
	case 0:
		SendNoticeMap("10ÃÊµÚ ÆÇÁ¤ÇÏ°Ú½À´Ï´Ù.", OXEVENT_MAP_INDEX, true);
		COXEventManager::instance().MakeAllInvisible();
		flag++;
		return PASSES_PER_SEC(10);

	case 1:
		SendNoticeMap("Á¤´äÀº", OXEVENT_MAP_INDEX, true);

		COXEventManager::instance().MakeAllVisible();

		if (info->answer == true)
		{
			COXEventManager::instance().CheckAnswer(true);
			SendNoticeMap("O ÀÔ´Ï´Ù", OXEVENT_MAP_INDEX, true);
		}
		else
		{
			COXEventManager::instance().CheckAnswer(false);
			SendNoticeMap("X ÀÔ´Ï´Ù", OXEVENT_MAP_INDEX, true);
		}
		SendNoticeMap("5ÃÊ µÚ Æ²¸®½Å ºÐµéÀ» ¹Ù±ùÀ¸·Î ÀÌµ¿ ½ÃÅ°°Ú½À´Ï´Ù.", OXEVENT_MAP_INDEX, true);

		flag++;
		return PASSES_PER_SEC(5);

	case 2:
		COXEventManager::instance().WarpToAudience();
		COXEventManager::instance().SetStatus(OXEVENT_CLOSE);
		SendNoticeMap("´ÙÀ½ ¹®Á¦ ÁØºñÇØÁÖ¼¼¿ä.", OXEVENT_MAP_INDEX, true);
		flag = 0;
		break;
	}
	return 0;
}

bool COXEventManager::Quiz(unsigned char level, int timelimit)
{
	if (m_vec_quiz.size() == 0) return false;
	if (level > m_vec_quiz.size()) level = m_vec_quiz.size() - 1;
	if (m_vec_quiz[level].size() <= 0) return false;

	if (timelimit < 0) timelimit = 30;

	int idx = random_number(0, m_vec_quiz[level].size() - 1);

	SendNoticeMap("¹®Á¦ ÀÔ´Ï´Ù.", OXEVENT_MAP_INDEX, true);
	for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
		SendNoticeMap(m_vec_quiz[level][idx].Quiz[i], OXEVENT_MAP_INDEX, true, i);
	SendNoticeMap("¸ÂÀ¸¸é O, Æ²¸®¸é X·Î ÀÌµ¿ÇØÁÖ¼¼¿ä", OXEVENT_MAP_INDEX, true);

	if (m_timedEvent != NULL) {
		event_cancel(&m_timedEvent);
	}

	OXEventInfoData* answer = AllocEventInfo<OXEventInfoData>();

	answer->answer = m_vec_quiz[level][idx].answer;

	timelimit -= 15;
	m_timedEvent = event_create(oxevent_timer, answer, PASSES_PER_SEC(timelimit));

	SetStatus(OXEVENT_QUIZ);

	m_vec_quiz[level].erase(m_vec_quiz[level].begin() + idx);
	return true;
}

bool COXEventManager::CheckAnswer(bool answer)
{
	if (m_map_attender.size() <= 0) return true;

	itertype(m_map_attender) iter = m_map_attender.begin();
	itertype(m_map_attender) iter_tmp;

	m_map_miss.clear();

	const PIXEL_POSITION& basePos = GetMapBase();

	int rect[4];
	if (answer != true)
	{
		rect[0] = basePos.x + 222 * 100;
		rect[1] = basePos.y + 229 * 100;
		rect[2] = basePos.x + 259 * 100;
		rect[3] = basePos.y + 264 * 100;
	}
	else
	{
		rect[0] = basePos.x + 262 * 100;
		rect[1] = basePos.y + 229 * 100;
		rect[2] = basePos.x + 299 * 100;
		rect[3] = basePos.y + 264 * 100;
	}

	LPCHARACTER pkChar = NULL;
	PIXEL_POSITION pos;
	for (; iter != m_map_attender.end();)
	{
		pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);
		if (pkChar != NULL)
		{
			pos = pkChar->GetXYZ();

			if (pos.x < rect[0] || pos.x > rect[2] || pos.y < rect[1] || pos.y > rect[3])
			{
				pkChar->EffectPacket(SE_FAIL);
				iter_tmp = iter;
				iter++;
				m_map_attender.erase(iter_tmp);
				m_map_miss.insert(std::make_pair(pkChar->GetPlayerID(), pkChar->GetPlayerID()));
			}
			else
			{
				pkChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT(pkChar, "Á¤´äÀÔ´Ï´Ù!"));
				// pkChar->CreateFly(random_number(FLY_FIREWORK1, FLY_FIREWORK6), pkChar);
				char chatbuf[256];
				int len = snprintf(chatbuf, sizeof(chatbuf),
					"%s %u %u", random_number(0, 1) == 1 ? "cheer1" : "cheer2", (DWORD)pkChar->GetVID(), 0);

				// ¸®ÅÏ°ªÀÌ sizeof(chatbuf) ÀÌ»óÀÏ °æ¿ì truncateµÇ¾ú´Ù´Â ¶æ..
				if (len < 0 || len >= (int) sizeof(chatbuf))
					len = sizeof(chatbuf) - 1;

				// \0 ¹®ÀÚ Æ÷ÇÔ
				++len;

				network::GCOutputPacket<network::GCChatPacket> pack_chat;
				pack_chat->set_type(CHAT_TYPE_COMMAND);
				pack_chat->set_id(0);
				pack_chat->set_message(chatbuf);

				pkChar->PacketAround(pack_chat);
				pkChar->EffectPacket(SE_SUCCESS);

				++iter;
			}
		}
		else
		{
			itertype(m_map_char) err = m_map_char.find(iter->first);
			if (err != m_map_char.end()) m_map_char.erase(err);

			itertype(m_map_miss) err2 = m_map_miss.find(iter->first);
			if (err2 != m_map_miss.end()) m_map_miss.erase(err2);

			iter_tmp = iter;
			++iter;
			m_map_attender.erase(iter_tmp);
		}
	}
	return true;
}

void COXEventManager::WarpToAudience()
{
	if (m_map_miss.size() <= 0) return;

	itertype(m_map_miss) iter = m_map_miss.begin();
	LPCHARACTER pkChar = NULL;

	const PIXEL_POSITION& basePos = GetMapBase();

	for (; iter != m_map_miss.end(); ++iter)
	{
		pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar != NULL)
		{
			static const int sc_iWarpCoordNum = 4;
			static const DWORD sc_WarpCoord[sc_iWarpCoordNum][2] = {
				{ 259, 289 },
				{ 205, 281 },
				{ 262, 205 },
				{ 321, 281 },
			};

			int index = random_number(0, sc_iWarpCoordNum - 1);
			pkChar->SetGMInvisible(false, true);
			pkChar->Show(OXEVENT_MAP_INDEX, basePos.x + sc_WarpCoord[index][0] * 100, basePos.y + sc_WarpCoord[index][1] * 100);
		}
	}

	m_map_miss.clear();
}

bool COXEventManager::CloseEvent()
{
	if (m_timedEvent != NULL) {
		event_cancel(&m_timedEvent);
	}

	itertype(m_map_char) iter = m_map_char.begin();

	LPCHARACTER pkChar = NULL;
	for (; iter != m_map_char.end(); ++iter)
	{
		pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar != NULL)
			pkChar->GoHome();
	}

	m_map_char.clear();

	return true;
}

bool COXEventManager::LogWinner()
{
	itertype(m_map_attender) iter = m_map_attender.begin();

	for (; iter != m_map_attender.end(); ++iter)
	{
		LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar)
			LogManager::instance().CharLog(pkChar, 0, "OXEVENT", "LastManStanding");
	}

	return true;
}

#ifdef INCREASE_ITEM_STACK
bool COXEventManager::GiveItemToAttender(DWORD dwItemVnum, WORD count)
#else
bool COXEventManager::GiveItemToAttender(DWORD dwItemVnum, unsigned char count)
#endif
{
	itertype(m_map_attender) iter = m_map_attender.begin();

	for (; iter != m_map_attender.end(); ++iter)
	{
		LPCHARACTER pkChar = CHARACTER_MANAGER::instance().FindByPID(iter->second);

		if (pkChar)
		{
			pkChar->AutoGiveItem(dwItemVnum, count);
			LogManager::instance().ItemLog(pkChar->GetPlayerID(), count, "OXEVENT_REWARD", "", pkChar->GetDesc()->GetHostName(), dwItemVnum);
		}
	}

	return true;
}

void COXEventManager::MakeAllInvisible()
{
	for (auto it = m_map_attender.begin(); it != m_map_attender.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(it->first);
		if (!ch)
			continue;

		ch->SetGMInvisible(true, true);
	}
}

void COXEventManager::MakeAllVisible()
{
	for (auto it = m_map_attender.begin(); it != m_map_attender.end(); ++it)
	{
		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(it->first);
		if (!ch)
			continue;

		ch->SetGMInvisible(false, true);
	}
}
