// vim:ts=4 sw=4
#ifndef __INC_METIN_II_ITEM_ID_RANGE_MANAGER_H__
#define __INC_METIN_II_ITEM_ID_RANGE_MANAGER_H__

using namespace network;

class CItemIDRangeManager : public singleton<CItemIDRangeManager>
{
	private :
		const static DWORD cs_dwMaxItemID = 4290000000UL;
		const static DWORD cs_dwMinimumRange = 10000000UL;
		const static DWORD cs_dwMinimumRemainCount = 10000UL;

		std::list<TItemIDRangeTable> m_listData;

	public :
		CItemIDRangeManager();

		void Build();
		bool BuildRange(DWORD dwMin, DWORD dwMax, TItemIDRangeTable& range, DWORD dwMinUsable = 0);
		void AppendRange(DWORD dwMin, DWORD dwMax, DWORD dwMinUsable = 0, bool bFront = false);

		TItemIDRangeTable GetRange();
};

#endif
