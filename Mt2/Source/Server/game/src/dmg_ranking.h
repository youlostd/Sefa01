#pragma once

#ifdef DMG_RANKING

class CDmgRankingManager : public singleton<CDmgRankingManager>
{
	public:
		void initDmgRankings();
		void registerToDmgRanks(LPCHARACTER ch, TypeDmg type, const int &dmg);
		void registerToDmgRanks(const char * name, TypeDmg type, const int &dmg);
		void saveDmgRankings();
		void sendP2PDmgRanking(TypeDmg type, const TRankDamageEntry &entry);
		void updateDmgRankings(TypeDmg type, const TRankDamageEntry &entry);
		void ShowRanks(LPCHARACTER ch);

	private:
		std::vector<SRankDamageEntry> m_vecDmgRankings[TYPE_DMG_MAX_NUM];
};
#endif