#pragma once

typedef struct CSpamTable
{
	std::string word_1;
	std::string word_2;
	std::string sanction;
	long duration;
} TSpamTable;

class CSpamFilter: public singleton<CSpamFilter>
{
public:
	bool InitializeSpamFilterTable();

	bool IsBannedWord(std::string message, TSpamTable &out);
	bool IsBannedWord(LPCHARACTER ch, std::string message);
	bool IsBannedWord(std::string message);

	void ApplyBlockAccountSanction(LPCHARACTER ch, long duration);
	void ApplyBlockChatSanction(LPCHARACTER ch, long duration);

protected:
	std::vector<TSpamTable> g_vecStrBanWords;
};