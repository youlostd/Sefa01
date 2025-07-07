
#include "../../common/stl.h"

class CMapLocation : public singleton<CMapLocation>
{
	public:
		typedef struct SLocation
		{
			long		addr;
			WORD		port;
		} TLocation;	

		bool	Get(long x, long y, long & lMapIndex, long & lAddr, WORD & wPort);
		bool	Get(int iIndex, long & lAddr, WORD & wPort);
		bool	GetByChannel(BYTE bChannel, int iIndex, long& lAddr, WORD& wPort);
		void	Insert(long lIndex, const char* c_pszHost, WORD wPort, BYTE bChannel);

		void	for_each_channel(std::function<void(BYTE)>&& f)
		{
			std::set<BYTE> channel_called;
			for (auto& pair : m_map_address_by_channel)
			{
				if (channel_called.find(pair.first.first) != channel_called.end())
					continue;

				channel_called.insert(pair.first.first);
				f(pair.first.first);
			}
		}

	protected:
		std::map<long, TLocation> m_map_address;
		std::map<std::pair<BYTE, long>, TLocation> m_map_address_by_channel;
};	  

