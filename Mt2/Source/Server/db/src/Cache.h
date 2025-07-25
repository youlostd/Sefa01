// vim:ts=8 sw=4
#ifndef __INC_DB_CACHE_H__
#define __INC_DB_CACHE_H__

#include "../../common/cache.h"
#include "protobuf_gd_packets.h"

#ifdef __PET_ADVANCED__
class CPetAdvancedCache : public cache<network::TPetAdvancedTable>
{
public:
	CPetAdvancedCache();
	virtual ~CPetAdvancedCache();

	void Delete();
	virtual void OnFlush();
};
#endif

class CItemCache : public cache<network::TItemData>
{
    public:
	CItemCache();
	virtual ~CItemCache();

	void Delete();
	virtual void OnFlush();

	void Disable(DWORD duration);
	bool IsDisabled() const;

private:
	DWORD m_dwDisableTimeout;
};

class CQuestCache : public cache<TQuestTable>
{
public:
	CQuestCache();
	virtual ~CQuestCache();

	void Delete();
	virtual void OnFlush();
};

class CAffectCache : public cache<TAffectSaveElement>
{
public:
	CAffectCache();
	virtual ~CAffectCache();

	void Delete();
	virtual void OnFlush();
};

class CPlayerTableCache : public cache<TPlayerTable>
{
    public:
	CPlayerTableCache();
	virtual ~CPlayerTableCache();

	virtual void OnFlush();

	DWORD GetLastUpdateTime() { return m_lastUpdateTime; }
};

// MYSHOP_PRICE_LIST
/**
 * @class	CItemPriceListTableCache
 * @brief	개인상점의 아이템 가격정보 리스트에 대한 캐시 class
 * @version	05/06/10 Bang2ni - First release.
 */
class CItemPriceListTableCache : public cache< TItemPriceListTable >
{
    public:

	/// Constructor
	/**
	 * 캐시 만료 시간을 설정한다.
	 */
	CItemPriceListTableCache(void);

	/// 리스트 갱신
	/**
	 * @param [in]	pUpdateList 갱신할 리스트
	 *
	 * 캐시된 가격정보를 갱신한다.
	 * 가격정보 리스트가 가득 찼을 경우 기존에 캐싱된 정보들을 뒤에서 부터 삭제한다.
	 */
	void	UpdateList(const TItemPriceListTable* pUpdateList);

	/// 가격정보를 DB 에 기록한다.
	virtual void	OnFlush(void);

    private:

	static const int	s_nMinFlushSec;		///< Minimum cache expire time
};
// END_OF_MYSHOP_PRICE_LIST

#ifdef CHANGE_SKILL_COLOR
class CSkillColorCache : public cache<network::GDSkillColorSavePacket>
{
public:
	CSkillColorCache();
	virtual ~CSkillColorCache();

	virtual void OnFlush();
};
#endif

#ifdef __EQUIPMENT_CHANGER__
class CEquipmentPageCache : public cache<network::TEquipmentChangerTable>
{
	public:
	CEquipmentPageCache();
	virtual ~CEquipmentPageCache();

	void Delete();
	virtual void OnFlush();
};
#endif

#endif
