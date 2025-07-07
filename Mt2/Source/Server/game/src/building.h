#ifndef __INC_METIN_II_BUILDING_H__
#define __INC_METIN_II_BUILDING_H__

#include "../../common/building.h"

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

namespace building
{
	class CLand;

	class CObject : public CEntity
	{
		public:
			CObject(const network::TBuildingObject* pData, network::TBuildingObjectProto* pProto);
			virtual ~CObject();

			void	Destroy();

			virtual void EncodeInsertPacket(LPENTITY entity);
			virtual void EncodeRemovePacket(LPENTITY entity);

			DWORD	GetID() { return m_data.id(); }

			void	SetVID(DWORD dwVID);
			DWORD	GetVID() { return m_dwVID; }

			bool	Show(long lMapIndex, long x, long y);

			void	Save();

			void	SetLand(CLand * pkLand) { m_pkLand = pkLand; }
			CLand *	GetLand()		{ return m_pkLand; }

			DWORD	GetVnum() { return m_pProto ? m_pProto->vnum() : 0; }
			DWORD	GetGroup() { return m_pProto ? m_pProto->group_vnum() : 0; }

			void	RegenNPC();

			// BUILDING_NPC
			void	ApplySpecialEffect();
			void	RemoveSpecialEffect();

			void	Reconstruct(DWORD dwVnum);

			LPCHARACTER GetNPC() { return m_chNPC; }
			// END_OF_BUILDING_NPC

		protected:
			network::TBuildingObjectProto* m_pProto;
			network::TBuildingObject		m_data;
			DWORD		m_dwVID;
			CLand *		m_pkLand;

			LPCHARACTER		m_chNPC;
	};

	class CLand
	{
		public:
			CLand(const network::TBuildingLand* pData);
			~CLand();

			void	Destroy();

			const network::TBuildingLand& GetData();
			void	PutData(const network::TBuildingLand* data);

			DWORD	GetID() const { return m_data.id(); }
			void	SetOwner(DWORD dwGID);
			DWORD	GetOwner() const { return m_data.guild_id(); }

			void	InsertObject(LPOBJECT pkObj);
			LPOBJECT	FindObject(DWORD dwID);
			LPOBJECT	FindObjectByVID(DWORD dwVID);
			LPOBJECT	FindObjectByVnum(DWORD dwVnum);
			LPOBJECT	FindObjectByGroup(DWORD dwGroupVnum);
			LPOBJECT	FindObjectByNPC(LPCHARACTER npc);
			void DeleteObject(DWORD dwID);

			bool	RequestCreateObject(DWORD dwVnum, long lMapIndex, long x, long y, float xRot, float yRot, float zRot, bool checkAnother);
			void	RequestDeleteObject(DWORD dwID);
			void	RequestDeleteObjectByVID(DWORD dwVID);

			void	RequestUpdate(DWORD dwGuild);

			// LAND_CLEAR
			void	ClearLand();
			// END_LAND_CLEAR

			// BUILD_WALL
			bool RequestCreateWall(long nMapIndex, float rot);
			void RequestDeleteWall();

			bool RequestCreateWallBlocks(DWORD dwVnum, long nMapIndex, char wallSize, bool doorEast, bool doorWest, bool doorSouth, bool doorNorth);
			void RequestDeleteWallBlocks(DWORD dwVnum);
			// END_BUILD_WALL

			DWORD GetMapIndex() { return m_data.map_index(); }

		protected:
			network::TBuildingLand		m_data;
			std::map<DWORD, LPOBJECT>	m_map_pkObject;
			std::map<DWORD, LPOBJECT>	m_map_pkObjectByVID;

			// BUILD_WALL
		private :
			void DrawWall(DWORD dwVnum, long nMapIndex, long& centerX, long& centerY, char length, float zRot);
			// END_BUILD_WALL
	};

	class CManager : public singleton<CManager>
	{
		public:
			CManager();
			virtual ~CManager();

			void	Destroy();

			void	FinalizeBoot();

			bool	LoadObjectProto(const ::google::protobuf::RepeatedPtrField<network::TBuildingObjectProto>& table);
			network::TBuildingObjectProto * GetObjectProto(DWORD dwVnum);

			bool	LoadLand(const network::TBuildingLand* pTable);
			CLand *	FindLand(DWORD dwID);
			CLand *	FindLand(long lMapIndex, long x, long y);
			CLand *	FindLandByGuild(DWORD GID);
			void	UpdateLand(const network::TBuildingLand* pTable);

			bool	LoadObject(const network::TBuildingObject* pTable, bool isBoot = false);
			void	DeleteObject(DWORD dwID);
			void	UnregisterObject(LPOBJECT pkObj);

			LPOBJECT	FindObjectByVID(DWORD dwVID);

			void	SendLandList(LPDESC d, long lMapIndex);

			// LAND_CLEAR
			void	ClearLand(DWORD dwLandID);
			void	ClearLandByGuildID(DWORD dwGuildID);
			// END_LAND_CLEAR

		protected:
			std::vector<network::TBuildingObjectProto>		m_vec_kObjectProto;
			std::map<DWORD, network::TBuildingObjectProto*>	m_map_pkObjectProto;

			std::map<DWORD, CLand *>		m_map_pkLand;
			std::map<DWORD, LPOBJECT>		m_map_pkObjByID;
			std::map<DWORD, LPOBJECT>		m_map_pkObjByVID;
	};
}

#endif
