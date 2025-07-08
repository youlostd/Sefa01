#include "StdAfx.h"

#ifdef ENABLE_RUNE_SYSTEM
#include "../EterBase/Poly/Poly.h"
#include "protobuf_data_player.h"

class CPythonRune : public CSingleton<CPythonRune>
{
public:
	enum EGeneral {
		RUNE_NAME_MAX_LEN = 50,
		RUNE_EVAL_MAX_LEN = 255,
	};

	enum EGroups {
		GROUP_PRECISION,
		GROUP_DOMINATION,
		GROUP_SORCERY,
		GROUP_RESOLVE,
		GROUP_INSPIRATION,
		GROUP_MAX_NUM,
	};

	enum ESubGroups {
		SUBGROUP_SECONDARY1,
		SUBGROUP_SECONDARY2,
		SUBGROUP_SECONDARY3,
		SUBGROUP_PRIMARY,
		SUBGROUP_MAX_NUM,
		SUBGROUP_SECONDARY_MAX = SUBGROUP_PRIMARY,
	};

	enum EProtoCols {
		COL_ID,
		COL_NAME,
		COL_GROUP,
		COL_SUBGROUP,
		COL_APPLY_TYPE,
		COL_APPLY_EVAL,
		COL_MAX_NUM,
	};

	typedef struct SRuneProto {
		SRuneProto() {
			id = 0;
			group = 0;
			sub_group = 0;
			sub_group_index = 0;
			apply_type = 0;
		}

		DWORD	id;
		std::string	name;
		std::string	desc;
		BYTE	group;
		BYTE	sub_group;
		BYTE	sub_group_index;
		BYTE	apply_type;
		std::string	apply_eval;
	} TRuneProto;

	typedef std::vector<std::unique_ptr<TRuneProto>> TProtoVec;
	typedef std::map<DWORD, TRuneProto*> TProtoMap;

	typedef std::map<std::string, BYTE> TPolyStateMap;

	typedef std::set<DWORD> TRuneSet;

public:
	CPythonRune();
	~CPythonRune();

	void	Initialize();
	void	Destroy();
	void	Clear();

	bool	LoadProto(const char* c_pszFileName);
	bool	LoadDesc(const char* c_pszFileName);

	TRuneProto*	GetProto(DWORD dwID);
	TRuneProto*	FindProtoByIndex(BYTE bGroup, BYTE bSubGroup, BYTE bIndex);

	void	AddRune(DWORD dwVnum);
	void	ClearOwned() { m_set_Runes.clear(); }
	bool	HasRune(DWORD dwVnum) const;

	void	SetRunePage(const TRunePageData* c_pData);
	const TRunePageData&	GetRunePage() const;
	TRunePageData&			GetNewRunePage();

private:
	void	BuildPolyStates();
	float	ProcessFormula(CPoly* pPoly);

public:
	float	EvalApplyValue(const std::string& c_rstApplyValue);
	
private:
	TProtoVec	m_vec_Proto;
	TProtoMap	m_map_Proto;

	TPolyStateMap	m_map_PolyStates;

	TRuneSet		m_set_Runes;
	TRunePageData	m_kPageData;
	TRunePageData	m_kNewPageData;
};
#endif
