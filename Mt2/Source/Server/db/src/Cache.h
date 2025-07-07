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
 * @brief	���λ����� ������ �������� ����Ʈ�� ���� ĳ�� class
 * @version	05/06/10 Bang2ni - First release.
 */
class CItemPriceListTableCache : public cache< TItemPriceListTable >
{
    public:

	/// Constructor
	/**
	 * ĳ�� ���� �ð��� �����Ѵ�.
	 */
	CItemPriceListTableCache(void);

	/// ����Ʈ ����
	/**
	 * @param [in]	pUpdateList ������ ����Ʈ
	 *
	 * ĳ�õ� ���������� �����Ѵ�.
	 * �������� ����Ʈ�� ���� á�� ��� ������ ĳ�̵� �������� �ڿ��� ���� �����Ѵ�.
	 */
	void	UpdateList(const TItemPriceListTable* pUpdateList);

	/// ���������� DB �� ����Ѵ�.
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
