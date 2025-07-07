#ifndef __INC_GAME_MOTION_H__
#define __INC_GAME_MOTION_H__

#include "../../common/d3dtype.h"

enum EMotionMode
{
	MOTION_MODE_GENERAL,
	MOTION_MODE_ONEHAND_SWORD,
	MOTION_MODE_TWOHAND_SWORD,
	MOTION_MODE_DUALHAND_SWORD,
	MOTION_MODE_BOW,
	MOTION_MODE_BELL,
	MOTION_MODE_FAN,
#ifdef __WOLFMAN__
	MOTION_MODE_CLAW,
#endif
	MOTION_MODE_HORSE,
	MOTION_MODE_MAX_NUM
};

enum EPublicMotion
{
	MOTION_NONE,				// 0 ¾øÀ½
	MOTION_WAIT,				// 1 ´ë±â		  (00.msa)
	MOTION_WALK,				// 2 °È±â		  (02.msa)
	MOTION_RUN,				 // 3 ¶Ù±â		  (03.msa)
	MOTION_CHANGE_WEAPON,	   // 4 ¹«±â¹Ù²Ù±â
	MOTION_DAMAGE,			  // 5 Á¤¸é¸Â±â	  (30.msa)
	MOTION_DAMAGE_FLYING,	   // 6 Á¤¸é³¯¾Æ°¡±â  (32.msa)
	MOTION_STAND_UP,			// 7 Á¤¸éÀÏ¾î³ª±â  (33.msa)
	MOTION_DAMAGE_BACK,		 // 8 ÈÄ¸é¸Â±â	  (34.msa)
	MOTION_DAMAGE_FLYING_BACK,  // 9 ÈÄ¸é³¯¾Æ°¡±â  (35.msa)
	MOTION_STAND_UP_BACK,	   // 10 ÈÄ¸éÀÏ¾î³ª±â (26.msa)
	MOTION_DEAD,				// 11 Á×±â		 (31.msa)
	MOTION_DEAD_BACK,		   // 12 ÈÄ¸éÁ×±â	 (37.msa)
	MOTION_NORMAL_ATTACK,		// 13 ±âº» °ø°Ý
	MOTION_COMBO_ATTACK_1,		// 14 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_2,	  // 15 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_3,	  // 16 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_4,	  // 17 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_5,	  // 18 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_6,	  // 19 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_7,	  // 20 ÄÞº¸ °ø°Ý
	MOTION_COMBO_ATTACK_8,	  // 21 ÄÞº¸ °ø°Ý
	MOTION_INTRO_WAIT,		  // 22 ¼±ÅÃÈ­¸é ´ë±â
	MOTION_INTRO_SELECTED,	  // 23 ¼±ÅÃÈ­¸é ¼±ÅÃ
	MOTION_INTRO_NOT_SELECTED,  // 24 ¼±ÅÃÈ­¸é ºñ¼±ÅÃ
	MOTION_SPAWN,			   // 25 ¼ÒÈ¯
	MOTION_FISHING_THROW,	   // 26 ³¬½Ã ´øÁö±â
	MOTION_FISHING_WAIT,		// 27 ³¬½Ã ´ë±â
	MOTION_FISHING_STOP,		// 28 ³¬½Ã ±×¸¸µÎ±â
	MOTION_FISHING_REACT,	   // 29 ³¬½Ã ¹ÝÀÀ
	MOTION_FISHING_CATCH,	   // 30 ³¬½Ã Àâ±â
	MOTION_FISHING_FAIL,		// 31 ³¬½Ã ½ÇÆÐ
	MOTION_STOP,				// 32 ¸» ¸ØÃß±â
	MOTION_SPECIAL_1,		   // 33 ¸ó½ºÅÍ ½ºÅ³
	MOTION_SPECIAL_2,		   // 34 
	MOTION_SPECIAL_3,			// 35
	MOTION_SPECIAL_4,			// 36
	MOTION_SPECIAL_5,			// 37
	PUBLIC_MOTION_END,
	MOTION_MAX_NUM = PUBLIC_MOTION_END,
};

class CMob;

class CMotion
{
	public:
		CMotion();
		~CMotion();

		bool			LoadFromFile(const char * c_pszFileName);
		bool			LoadMobSkillFromFile(const char * c_pszFileName, CMob * pMob, int iSkillIndex);

		float			GetDuration() const;
		const D3DVECTOR &	GetAccumVector() const;

		bool			IsEmpty();

	protected:
		bool			m_isEmpty;
		float			m_fDuration;
		bool			m_isAccumulation;
		D3DVECTOR		m_vec3Accumulation;
};

typedef DWORD MOTION_KEY;

#define MAKE_MOTION_KEY(mode, index)			( ((mode) << 24) | ((index) << 8) | (0) )
#define MAKE_RANDOM_MOTION_KEY(mode, index, type)	( ((mode) << 24) | ((index) << 8) | (type) )

#define GET_MOTION_MODE(key)				((BYTE) ((key >> 24) & 0xFF))
#define GET_MOTION_INDEX(key)				((WORD) ((key >> 8) & 0xFFFF))
#define GET_MOTION_SUB_INDEX(key)			((BYTE) ((key) & 0xFF))

class CMotionSet
{
	public:
		typedef std::map<DWORD, CMotion *>	TContainer;
		typedef TContainer::iterator		iterator;
		typedef TContainer::const_iterator	const_iterator;

	public:
		CMotionSet();
		~CMotionSet();

		void		Insert(DWORD dwKey, CMotion * pkMotion);
		bool		Load(const char * szFileName, int mode, int motion);

		const CMotion *	GetMotion(DWORD dwKey) const;

	protected:
		// DWORD = MOTION_KEY
		TContainer			m_map_pkMotion;
};

class CMotionManager : public singleton<CMotionManager>
{
	public:
		typedef std::map<DWORD, CMotionSet *> TContainer;
		typedef TContainer::iterator iterator;

		CMotionManager();
		virtual ~CMotionManager();

		bool			Build();

		const CMotionSet *	GetMotionSet(DWORD dwVnum);
		const CMotion *		GetMotion(DWORD dwVnum, DWORD dwKey);
		float				GetMotionDuration(DWORD dwVnum, DWORD dwKey);

		// POLYMORPH_BUG_FIX
		float				GetNormalAttackDuration(DWORD dwVnum);
		// END_OF_POLYMORPH_BUG_FIX

	protected:
		// DWORD = JOB or MONSTER VNUM
		TContainer		m_map_pkMotionSet;

		// POLYMORPH_BUG_FIX
		std::map<DWORD, float> m_map_normalAttackDuration;
		// END_OF_POLYMORPH_BUG_FIX
};

#endif
