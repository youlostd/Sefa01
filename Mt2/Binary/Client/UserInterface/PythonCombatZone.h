/*********************************************************************
* title_name		: Combat Zone (Official Webzen 16.4)
* date_created		: 2017.05.21
* filename			: PythonCombatZone.h
* author			: VegaS
* version_actual	: Version 0.2.0
*/
#pragma once
#include "protobuf_gc_packets.h"

class CPythonCombatZone
{
	public:
		CPythonCombatZone(void);
		~CPythonCombatZone(void);
		void Initialize(const network::GCCombatZoneRankingDataPacket& p);
		void SendDataDays(const google::protobuf::RepeatedField<google::protobuf::uint32>& infoData, bool bIsOnline);
		network::TCombatZoneRankingPlayer Request(int);
		static CPythonCombatZone* instance();
	private:
		std::vector<network::TCombatZoneRankingPlayer> m_vecRankingData;
		static CPythonCombatZone * curInstance;
		
};