#ifndef __HEADER_QUEST_LUA__
#define __HEADER_QUEST_LUA__

#include "quest.h"
#include "buffer_manager.h"
#include "desc.h"
#include "headers.hpp"
#include "char.h"
#include "entity.h"

extern int test_server;

namespace quest
{
	extern void RegisterPCFunctionTable();
	extern void RegisterNPCFunctionTable();
	extern void RegisterTargetFunctionTable();
	extern void RegisterAffectFunctionTable();
	extern void RegisterBuildingFunctionTable();
	extern void RegisterMarriageFunctionTable();
	extern void RegisterITEMFunctionTable();
	extern void RegisterDungeonFunctionTable();
	extern void RegisterQuestFunctionTable();
	extern void RegisterPartyFunctionTable();
	extern void RegisterMountFunctionTable();
	extern void RegisterPetFunctionTable();
	extern void RegisterGuildFunctionTable();
	extern void RegisterGameFunctionTable();
	extern void RegisterArenaFunctionTable();
	extern void RegisterGlobalFunctionTable(lua_State* L);
	extern void RegisterOXEventFunctionTable();
#ifdef __VOTE4BUFF__
	extern void RegisterV4BFunctionTable();
#endif
#ifdef __DRAGONSOUL__
	extern void RegisterDragonSoulFunctionTable();
#endif
#ifdef __EVENT_MANAGER__
	extern void RegisterEventFunctionTable();
#endif
#ifdef __MELEY_LAIR_DUNGEON__
	extern void RegisterMeleyLairFunctionTable();
#endif

	extern void combine_lua_string(lua_State* L, std::ostringstream &s);
	
	struct FSetWarpLocation
	{
		long map_index; 
		long x;
		long y;

		FSetWarpLocation (long _map_index, long _x, long _y) :
			map_index (_map_index), x (_x), y (_y)
		{}
		void operator () (LPCHARACTER ch);
	};

	struct FSetQuestFlag
	{
		std::string flagname;
		int value;

		void operator () (LPCHARACTER ch);
	};

	struct FPartyCheckFlagLt
	{
		std::string flagname;
		int value;

		bool operator () (LPCHARACTER ch);
	};

	struct FPartyChat
	{
		int iChatType;
		const char* str;

		FPartyChat(int ChatType, const char* str);
		void operator() (LPCHARACTER ch);
	};

	struct FPartyClearReady
	{
		void operator() (LPCHARACTER ch);
	};

	template <typename T>
	struct FSendPacketToEmpire
	{
		FSendPacketToEmpire(const network::GCOutputPacket<T>& pack) : data(pack) {}

		network::GCOutputPacket<T>& data;
		BYTE bEmpire;

		void operator() (LPENTITY ent) {
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER)ent;

				if (ch->GetDesc())
				{
					if (ch->GetEmpire() == bEmpire)
						ch->GetDesc()->Packet(data);
				}
			}
		}
	};

	struct FWarpEmpire
	{
		BYTE m_bEmpire;
		long m_lMapIndexTo;
		long m_x;
		long m_y;

		void operator() (LPENTITY ent);
	};

	EVENTINFO(warp_all_to_map_my_empire_event_info)
	{
		BYTE 	m_bEmpire;
		long	m_lMapIndexFrom;
		long 	m_lMapIndexTo;
		long 	m_x;
		long	m_y;

		warp_all_to_map_my_empire_event_info() 
		: m_bEmpire( 0 )
		, m_lMapIndexFrom( 0 )
		, m_lMapIndexTo( 0 )
		, m_x( 0 )
		, m_y( 0 )
		{
		}
	};

	EVENTFUNC(warp_all_to_map_my_empire_event);

	struct FBuildLuaGuildWarList
	{
		lua_State * L;
		int m_count;

		FBuildLuaGuildWarList(lua_State * L);
		void operator() (DWORD g1, DWORD g2);
	};
}
#endif /*__HEADER_QUEST_LUA__*/

