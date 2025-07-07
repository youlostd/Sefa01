#ifndef __INC_METIN_II_GAME_SECTREE_MANAGER_H__
#define __INC_METIN_II_GAME_SECTREE_MANAGER_H__

#include "sectree.h"


typedef struct SMapRegion
{
	int			index;
	int			sx, sy, ex, ey;
	PIXEL_POSITION	posSpawn;

	bool		bEmpireSpawnDifferent;
	PIXEL_POSITION	posEmpire[3];

	std::string		strMapName;
} TMapRegion;

struct TAreaInfo
{
	int sx, sy, ex, ey, dir;
	TAreaInfo(int sx, int sy, int ex, int ey, int dir)
		: sx(sx), sy(sy), ex(ex), ey(ey), dir(dir)
		{}
};

struct npc_info
{
	unsigned char bType;
	DWORD dwRace;
	std::string namelist[LANGUAGE_MAX_NUM];
	long x, y;
	npc_info(unsigned char bType, DWORD race, std::string* namelist, long x, long y)
		: bType(bType), dwRace(race), x(x), y(y)
	{
		for (int i = 0; i < LANGUAGE_MAX_NUM; ++i)
			this->namelist[i] = namelist[i];
	}
};

typedef std::map<std::string, TAreaInfo> TAreaMap;

typedef struct SSetting
{
	int			iIndex;
	int			iCellScale;
	int			iBaseX;
	int			iBaseY;
	int			iWidth;
	int			iHeight;

	PIXEL_POSITION	posSpawn;
} TMapSetting;

#ifdef __MAP_CONFIG__
typedef std::map<std::string, std::string> TMapConfigMap;
#endif

class SECTREE_MAP
{
	public:
		typedef std::map<DWORD, LPSECTREE> MapType;

		SECTREE_MAP();
		SECTREE_MAP(SECTREE_MAP & r);
		virtual ~SECTREE_MAP();

		bool Add(DWORD key, LPSECTREE sectree) {
			return map_.insert(MapType::value_type(key, sectree)).second;
		}

		LPSECTREE	Find(DWORD dwPackage);
		LPSECTREE	Find(DWORD x, DWORD y);
		void		Build();

		TMapSetting	m_setting;
#ifdef __MAP_CONFIG__
		TMapConfigMap m_configMap;
#endif

		template< typename Func >
		void rfor_each(Func & rfunc)
		{
			// <Factor> Using snapshot copy to avoid side-effects
			FCollectEntity collector;
			std::map<DWORD, LPSECTREE>::iterator it = map_.begin();
			for ( ; it != map_.end(); ++it)
			{
				LPSECTREE sectree = it->second;
				sectree->for_each_entity(collector);
			}
			collector.ForEach(rfunc);
			/*
			std::map<DWORD,LPSECTREE>::iterator i = map_.begin();
			for (; i != map_.end(); ++i )
			{
				LPSECTREE pSec = i->second;
				pSec->for_each_entity( rfunc );
			}
			*/
		}

		template< typename Func >
		void for_each( Func & rfunc )
		{
			rfor_each(rfunc);
		}

		template< typename Func >
		void for_each(Func && func)
		{
			rfor_each(func);
		}

		template< typename Func >
		void for_each_pc( Func & rfunc )
		{
			// <Factor> Using snapshot copy to avoid side-effects
			FCollectEntity collector;
			std::map<DWORD, LPSECTREE>::iterator it = map_.begin();
			for ( ; it != map_.end(); ++it)
			{
				LPSECTREE sectree = it->second;
				sectree->for_each_pc_entity(collector);
			}
			collector.ForEach(rfunc);
			/*
			std::map<DWORD,LPSECTREE>::iterator i = map_.begin();
			for (; i != map_.end(); ++i )
			{
				LPSECTREE pSec = i->second;
				pSec->for_each_entity( rfunc );
			}
			*/
		}

		void DumpAllToSysErr() {
			SECTREE_MAP::MapType::iterator i;
			for (i = map_.begin(); i != map_.end(); ++i)
			{
				sys_err("SECTREE %x(%u, %u)", i->first, i->first & 0xffff, i->first >> 16);
			}
		}

	private:
		MapType map_;
};

enum EAttrRegionMode
{
	ATTR_REGION_MODE_SET,
	ATTR_REGION_MODE_REMOVE,
	ATTR_REGION_MODE_CHECK,
};

class SECTREE_MANAGER : public singleton<SECTREE_MANAGER>
{
	public:
		SECTREE_MANAGER();
		virtual ~SECTREE_MANAGER();

		LPSECTREE_MAP GetMap(long lMapIndex);
		LPSECTREE 	Get(DWORD dwIndex, DWORD package);
		LPSECTREE 	Get(DWORD dwIndex, DWORD x, DWORD y);

		template< typename Func >
		void for_each( int iMapIndex, Func & rfunc )
		{
			LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap( iMapIndex );
			if ( pSecMap )
			{
				pSecMap->for_each( rfunc );
			}
		}

		template< typename Func >
		void for_each_pc(Func func)
		{
			std::map<DWORD, LPSECTREE_MAP>::iterator it;
			for (it = m_map_pkSectree.begin(); it != m_map_pkSectree.end(); ++it)
			{
				it->second->for_each_pc(func);
			}
		}
		
		int		LoadSettingFile(long lIndex, const char * c_pszSettingFileName, TMapSetting & r_setting);
		bool		LoadMapRegion(const char * c_pszFileName, TMapSetting & r_Setting, const char * c_pszMapName);
#ifdef __MAP_CONFIG__
		void	LoadConfigFile(const char * c_pszConfigFileName, TMapConfigMap & r_configMap);
#endif
		int		Build(const char * c_pszListFileName, const char* c_pszBasePath);
#ifdef __MAP_CONFIG__
		LPSECTREE_MAP BuildSectreeFromSetting(TMapSetting & r_setting, TMapConfigMap & r_configMap);
#else
		LPSECTREE_MAP BuildSectreeFromSetting(TMapSetting & r_setting);
#endif
		bool		LoadAttribute(LPSECTREE_MAP pkMapSectree, const char * c_pszFileName, TMapSetting & r_setting);
		void		LoadDungeon(int iIndex, const char * c_pszFileName);
		bool		GetValidLocation(long lMapIndex, long x, long y, long & r_lValidMapIndex, PIXEL_POSITION & r_pos, BYTE empire = 0);
		bool		GetSpawnPosition(long x, long y, PIXEL_POSITION & r_pos);
		bool		GetSpawnPositionByMapIndex(long lMapIndex, PIXEL_POSITION & r_pos);
		bool		GetRecallPositionByEmpire(int iMapIndex, BYTE bEmpire, PIXEL_POSITION & r_pos);

		const TMapRegion *	GetMapRegion(long lMapIndex);
		int			GetMapIndex(long x, long y);
		const TMapRegion *	FindRegionByPartialName(const char* szMapName);

#ifdef __MAP_CONFIG__
		TMapConfigMap*	GetMapConfig(int iMapIndex);
		std::string	GetMapConfig(int iMapIndex, const std::string& c_rstConfigKey, const char* c_pszDefault = "");
		int			GetMapConfig_Int(int iMapIndex, const std::string& c_rstConfigKey, int iDefault = 0);
		bool		GetMapConfig_Bool(int iMapIndex, const std::string& c_rstConfigKey, bool bDefault = false);
#endif

		bool		GetMapBasePosition(long x, long y, PIXEL_POSITION & r_pos);
		bool		GetMapBasePositionByMapIndex(long lMapIndex, PIXEL_POSITION & r_pos);
		bool		GetMovablePosition(long lMapIndex, long x, long y, PIXEL_POSITION & pos);
		bool		IsMovablePosition(long lMapIndex, long x, long y);
		bool		GetCenterPositionOfMap(long lMapIndex, PIXEL_POSITION & r_pos);
		bool		GetRandomLocation(long lMapIndex, PIXEL_POSITION & r_pos, DWORD dwCurrentX = 0, DWORD dwCurrentY = 0, int iMaxDistance = 0);

		long		CreatePrivateMap(long lMapIndex);	// returns new private map index, returns 0 when fail
		void		DestroyPrivateMap(long lMapIndex);

		TAreaMap&	GetDungeonArea(long lMapIndex);
		void		SendNPCPosition(LPCHARACTER ch);
		void		InsertNPCPosition(long lMapIndex, DWORD dwRace, unsigned char bType, std::string* pstNameList, long x, long y);

		BYTE		GetEmpireFromMapIndex(long lMapIndex);

		void		PurgeMonstersInMap(long lMapIndex);
		void		PurgeStonesInMap(long lMapIndex);
		void		PurgeNPCsInMap(long lMapIndex);
		size_t		GetMonsterCountInMap(long lMapIndex);
		size_t		GetMonsterCountInMap(long lMpaIndex, DWORD dwVnum, bool bAllowNPC = false);

		/// 영역에 대해 Sectree 의 Attribute 에 대해 특정한 처리를 수행한다.
		/**
		 * @param [in]	lMapIndex 적용할 Map index
		 * @param [in]	lStartX 사각형 영역의 가장 왼쪽 좌표
		 * @param [in]	lStartY 사각형 영역의 가장 위쪽 좌표
		 * @param [in]	lEndX 사각형 영역의 가장 오른쪽 좌표
		 * @param [in]	lEndY 사각형 영역의 가장 아랫쪽 좌표
		 * @param [in]	lRotate 영역에 대해 회전할 각
		 * @param [in]	dwAttr 적용할 Attribute
		 * @param [in]	mode Attribute 에 대해 처리할 type
		 */
		bool		ForAttrRegion(long lMapIndex, long lStartX, long lStartY, long lEndX, long lEndY, long lRotate, DWORD dwAttr, EAttrRegionMode mode);

		bool		SaveAttributeToImage(int lMapIndex, const char * c_pszFileName, LPSECTREE_MAP pMapSrc = NULL);

	private:

		/// 직각의 사각형 영역에 대해 Sectree 의 Attribute 에 대해 특정한 처리를 수행한다.
		/**
		 * @param [in]	lMapIndex 적용할 Map index
		 * @param [in]	lCX 사각형 영역의 가장 왼쪽 Cell 의 좌표
		 * @param [in]	lCY 사각형 영역의 가장 위쪽 Cell 의 좌표
		 * @param [in]	lCW 사각형 영역의 Cell 단위 폭
		 * @param [in]	lCH 사각형 영역의 Cell 단위 높이
		 * @param [in]	lRotate 회전할 각(직각)
		 * @param [in]	dwAttr 적용할 Attribute
		 * @param [in]	mode Attribute 에 대해 처리할 type
		 */
		bool		ForAttrRegionRightAngle( long lMapIndex, long lCX, long lCY, long lCW, long lCH, long lRotate, DWORD dwAttr, EAttrRegionMode mode );

		/// 직각 이외의 사각형 영역에 대해 Sectree 의 Attribute 에 대해 특정한 처리를 수행한다.
		/**
		 * @param [in]	lMapIndex 적용할 Map index
		 * @param [in]	lCX 사각형 영역의 가장 왼쪽 Cell 의 좌표
		 * @param [in]	lCY 사각형 영역의 가장 위쪽 Cell 의 좌표
		 * @param [in]	lCW 사각형 영역의 Cell 단위 폭
		 * @param [in]	lCH 사각형 영역의 Cell 단위 높이
		 * @param [in]	lRotate 회전할 각(직각 이외의 각)
		 * @param [in]	dwAttr 적용할 Attribute
		 * @param [in]	mode Attribute 에 대해 처리할 type
		 */
		bool		ForAttrRegionFreeAngle( long lMapIndex, long lCX, long lCY, long lCW, long lCH, long lRotate, DWORD dwAttr, EAttrRegionMode mode );

		/// 한 Cell 의 Attribute 에 대해 특정한 처리를 수행한다.
		/**
		 * @param [in]	lMapIndex 적용할 Map index
		 * @param [in]	lCX 적용할 Cell 의 X 좌표
		 * @param [in]	lCY 적용할 Cell 의 Y 좌표
		 * @param [in]	dwAttr 적용할 Attribute
		 * @param [in]	mode Attribute 에 대해 처리할 type
		 */
		bool		ForAttrRegionCell( long lMapIndex, long lCX, long lCY, DWORD dwAttr, EAttrRegionMode mode );

		static WORD			current_sectree_version;
		std::map<DWORD, LPSECTREE_MAP>	m_map_pkSectree;
		std::map<int, TAreaMap>	m_map_pkArea;
		std::vector<TMapRegion>		m_vec_mapRegion;
		std::map<DWORD, std::vector<npc_info> > m_mapNPCPosition;

		// <Factor> Circular private map indexing
		typedef TR1_NS::unordered_map<long, int> PrivateIndexMapType;
		PrivateIndexMapType next_private_index_map_;
};

#endif

