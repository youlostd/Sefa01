#ifndef __HEADER_VNUM_HELPER__
#define	__HEADER_VNUM_HELPER__

class CItemVnumHelper
{
public:
	static	const bool	IsHalloweenCandy(DWORD vnum)		{ return 71136 == vnum; }
	static	const bool	IsHappinessRing(DWORD vnum)			{ return 71143 == vnum; }
	static	const bool	IsLovePendant(DWORD vnum)			{ return 71145 == vnum; }

	static	constexpr bool	IsOreItem(DWORD vnum)			{ return vnum >= 50601 && vnum <= 50639; }
	static	constexpr bool	IsFishItem(DWORD vnum)			{ return vnum >= 27863 && vnum <= 27878; }
	static	constexpr bool	IsSoulstoneItem(DWORD vnum)		{ return 50513 == vnum; }

#ifdef __ACCE_COSTUME__
	static const bool IsAcceItem(DWORD vnum) {
		if ((vnum >= 85001 && vnum <= 85008) || (vnum >= 85011 && vnum <= 85018) || (vnum >= 85021 && vnum <= 85024) || (vnum >= 93331 && vnum <= 93334) || (vnum >= 86001 && vnum <= 86008) || (vnum >= 94112 && vnum <= 94119))
			return true;
		return false;
	}
#endif

#ifdef __FAKE_BUFF__
	static	const bool	IsFakeBuffSpawn(DWORD vnum)			{ return (vnum == 92212 || vnum == 94338); }
#endif
};

class CMobVnumHelper
{
public:
	static	bool	IsShopRace(DWORD vnum)		{ return 30000 == vnum; }
};

class CVnumHelper
{
};


#endif	//__HEADER_VNUM_HELPER__