
struct tag_Quiz
{
	char level;
	char Quiz[LANGUAGE_MAX_NUM][256];
	bool answer;
};

enum OXEventStatus
{
	OXEVENT_FINISH = 0, // OXÀÌº¥Æ®°¡ ¿ÏÀüÈ÷ ³¡³­ »óÅÂ
	OXEVENT_OPEN = 1,	// OXÀÌº¥Æ®°¡ ½ÃÀÛµÊ. À»µÎÁö(20012)¸¦ ÅëÇØ¼­ ÀÔÀå°¡´É
	OXEVENT_CLOSE = 2,	// OXÀÌº¥Æ®ÀÇ Âü°¡°¡ ³¡³². À»µÎÁö(20012)¸¦ ÅëÇÑ ÀÔÀåÀÌ Â÷´ÜµÊ
	OXEVENT_QUIZ = 3,	// ÄûÁî¸¦ ÃâÁ¦ÇÔ.

	OXEVENT_ERR = 0xff
};

class COXEventManager : public singleton<COXEventManager>
{
	private :
		std::map<DWORD, DWORD> m_map_char;
		std::map<DWORD, DWORD> m_map_attender;
		std::map<DWORD, DWORD> m_map_miss;

		std::vector<std::vector<tag_Quiz> > m_vec_quiz;

		LPEVENT m_timedEvent;

	protected :
		bool CheckAnswer();

		bool EnterAudience(LPCHARACTER pChar);
		bool EnterAttender(LPCHARACTER pChar);

		const PIXEL_POSITION& GetMapBase();

	public :
		bool Initialize();
		void Destroy();

		OXEventStatus GetStatus();
		void SetStatus(OXEventStatus status);

		bool LoadQuizScript(const char* szFileName);

		bool Enter(LPCHARACTER pChar);
		
		bool CloseEvent();

		void ClearQuiz();
		bool AddQuiz(unsigned char level, const char** pszQuestions, bool answer);
		bool ShowQuizList(LPCHARACTER pChar);

		bool Quiz(unsigned char level, int timelimit);
#ifdef INCREASE_ITEM_STACK
		bool GiveItemToAttender(DWORD dwItemVnum, WORD count);
#else
		bool GiveItemToAttender(DWORD dwItemVnum, unsigned char count);
#endif

		bool CheckAnswer(bool answer);
		void WarpToAudience();

		bool LogWinner();

		void MakeAllInvisible();
		void MakeAllVisible();

		DWORD GetAttenderCount() { return m_map_attender.size(); }
};

