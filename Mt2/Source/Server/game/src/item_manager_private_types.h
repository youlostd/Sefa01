class CItemDropInfo
{
public:
	CItemDropInfo(int iLevelStart, int iLevelEnd, int iPercent, DWORD dwVnumStart, DWORD dwVnumEnd, unsigned char bCount = 1, unsigned char bJob = 0, bool bAutoPickup = false) :
	  m_iLevelStart(iLevelStart), m_iLevelEnd(iLevelEnd), m_iPercent(iPercent), m_dwVnumStart(dwVnumStart), m_dwVnumEnd(dwVnumEnd), m_bCount(bCount), m_bJob(bJob), m_bAutoPickup(bAutoPickup)
	  {
	  }

	  int	m_iLevelStart;
	  int	m_iLevelEnd;
	  int	m_iPercent; // 1 ~ 1000
	  DWORD	m_dwVnumStart;
	  DWORD	m_dwVnumEnd;
	  unsigned char	m_bCount;
	  unsigned char	m_bJob;
	  bool	m_bAutoPickup;

	  friend bool operator < (const CItemDropInfo & l, const CItemDropInfo & r)
	  {
		  return l.m_iLevelEnd < r.m_iLevelEnd;
	  }
};

extern std::vector<CItemDropInfo> g_vec_pkCommonDropItem[MOB_RANK_MAX_NUM];

typedef struct SDropItem
{
	int		iLvStart;
	int		iLvEnd;
	float	fPercent;
	char	szItemName[ITEM_NAME_MAX_LEN + 1];
	int		iCount;
	int		iJob;
	bool	bAutoPickup;
} TDropItem;

