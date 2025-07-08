#pragma once

class CPythonQuest : public CSingleton<CPythonQuest>
{
	public:
		struct SQuestInstance
		{
			SQuestInstance()
			{
				dwIndex = 0;
				iClockValue = 0;
				iStartTime = 0;
#ifdef ENABLE_QUEST_CATEGORIES
				iQuestCatId = 0;
#endif
			}

			DWORD			dwIndex;

			std::string		strIconFileName;
			std::string		strTitle;

			std::string		strClockName;
			std::map<int, std::string>	map_CounterName;

			int				iClockValue;
			std::map<int, int>	map_CounterValue;

			int				iStartTime;
#ifdef ENABLE_QUEST_CATEGORIES
			int				iQuestCatId;
#endif
		};
		typedef std::vector<SQuestInstance> TQuestInstanceContainer;

	public:
		CPythonQuest();
		virtual ~CPythonQuest();

		void Clear();

		void RegisterQuestInstance(const SQuestInstance & c_rQuestInstance);
		void DeleteQuestInstance(DWORD dwIndex);

		bool IsQuest(DWORD dwIndex);
		void MakeQuest(DWORD dwIndex);

		void SetQuestTitle(DWORD dwIndex, const char * c_szTitle);
		void SetQuestClockName(DWORD dwIndex, const char * c_szClockName);
		void SetQuestCounter(DWORD dwIndex, int counterIdx, const char * c_szCounterName, int value);
		void SetQuestClockValue(DWORD dwIndex, int iClockValue);
		void SetQuestIconFileName(DWORD dwIndex, const char * c_szIconFileName);
#ifdef ENABLE_QUEST_CATEGORIES
		void SetQuestCatId(DWORD dwIndex, int CatId);
#endif

		int GetQuestCount();
		bool GetQuestInstancePtr(DWORD dwArrayIndex, SQuestInstance ** ppQuestInstance);

	protected:
		void __Initialize();
		bool __GetQuestInstancePtr(DWORD dwQuestIndex, SQuestInstance ** ppQuestInstance);

	protected:
		TQuestInstanceContainer m_QuestInstanceContainer;
};