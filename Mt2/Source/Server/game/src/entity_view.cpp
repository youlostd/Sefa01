#include "stdafx.h"

#include "utils.h"
#include "char.h"
#include "sectree_manager.h"
#include "config.h"
#include "mount_system.h"
#ifdef __LIMIT_SHOP_NPCS__
#include "questmanager.h"
#endif

#include "desc.h"

void CEntity::ViewCleanup()
{
	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = it->first;
		++it;

		entity->ViewRemove(this, false);
	}

	m_map_view.clear();
}

void CEntity::ViewReencode()
{
	if (m_bIsObserver || (GetType() == ENTITY_CHARACTER && ((LPCHARACTER)this)->IsGMInvisible()))
		return;

	EncodeRemovePacket(this);
	EncodeInsertPacket(this);

	ENTITY_MAP::iterator it = m_map_view.begin();

	while (it != m_map_view.end())
	{
		LPENTITY entity = (it++)->first;

		EncodeRemovePacket(entity);
		EncodeInsertPacket(entity);

		if (!entity->m_bIsObserver)
			entity->EncodeInsertPacket(this);
	}

}

void CEntity::ViewInsert(LPENTITY entity, bool recursive)
{
	if (this == entity)
		return;

	ENTITY_MAP::iterator it = m_map_view.find(entity);

	if (m_map_view.end() != it)
	{
		it->second = m_iViewAge;
		return;
	}

	m_map_view.insert(ENTITY_MAP::value_type(entity, m_iViewAge));

	if (!entity->m_bIsObserver)
		entity->EncodeInsertPacket(this);

	if (recursive)
		entity->ViewInsert(this, false);
}

void CEntity::ViewRemove(LPENTITY entity, bool recursive)
{
	ENTITY_MAP::iterator it = m_map_view.find(entity);

	if (it == m_map_view.end())
		return;

	m_map_view.erase(it);

	if (!entity->m_bIsObserver)
		entity->EncodeRemovePacket(this);

	if (recursive)
		entity->ViewRemove(this, false);
}

class CFuncViewInsert
{
	private:
		int dwViewRange;

	public:
		LPENTITY m_me;

		CFuncViewInsert(LPENTITY ent) :
			dwViewRange(VIEW_RANGE + VIEW_BONUS_RANGE),
			m_me(ent)
		{
		}

		void operator () (LPENTITY ent)
		{
#ifdef __LIMIT_SHOP_NPCS__
			if (ent->IsType(ENTITY_PRIVAT_SHOP) && quest::CQuestManager::instance().GetEventFlag("shop_view_range") && DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > quest::CQuestManager::instance().GetEventFlag("shop_view_range"))
				return;
#endif
			// ¿ÀºêÁ§Æ®°¡ ¾Æ´Ñ °ÍÀº °Å¸®¸¦ °è»êÇÏ¿© °Å¸®°¡ ¸Ö¸é Ãß°¡ÇÏÁö ¾Ê´Â´Ù.
			if (!ent->IsType(ENTITY_OBJECT))
				if (DISTANCE_APPROX(ent->GetX() - m_me->GetX(), ent->GetY() - m_me->GetY()) > dwViewRange)
					return;

			// ³ª¸¦ ´ë»ó¿¡ Ãß°¡
			m_me->ViewInsert(ent);

			// µÑ´Ù Ä³¸¯ÅÍ¸é
			if (ent->IsType(ENTITY_CHARACTER) && m_me->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER chMe = (LPCHARACTER) m_me;
				LPCHARACTER chEnt = (LPCHARACTER) ent;

				// ´ë»óÀÌ NPC¸é StateMachineÀ» Å²´Ù.
				if (chMe->IsPC() && !chEnt->IsPC() && !chEnt->IsWarp() && !chEnt->IsGoto())
					chEnt->StartStateMachine();
			}
		}
};

void CEntity::UpdateSectree()
{
	if (!GetSectree())
	{
		if (IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER tch = (LPCHARACTER) this;
			if (tch->GetDesc() && (tch->GetDesc()->IsPhase(PHASE_GAME) || tch->GetDesc()->IsPhase(PHASE_DEAD)))
				sys_err("null sectree name: %s %d %d",  tch->GetName(), GetX(), GetY());
		}

		return;
	}

	++m_iViewAge;

	CFuncViewInsert f(this); // ³ª¸¦ ¼½Æ®¸®¿¡ ÀÖ´Â »ç¶÷µé¿¡°Ô Ãß°¡
	GetSectree()->ForEachAround(f);

	ENTITY_MAP::iterator it, this_it;

	//
	// m_map_view¿¡¼­ ÇÊ¿ä ¾ø´Â ³à¼®µé Áö¿ì±â
	// 
	LPCHARACTER pkThis = (GetType() == ENTITY_CHARACTER ? (LPCHARACTER)this : NULL);
	if (pkThis && !pkThis->IsPC())
		pkThis = pkThis->GetRider();

	if (m_bNoPacketPVPModeChange)
	{
		m_bNoPacketPVPModeChange = false;
		it = m_map_view.begin();

		while (it != m_map_view.end())
		{
			this_it = it++;
			LPENTITY ent = this_it->first;

			if (m_bNoPacketPVP && !ent->m_bNoPacketPVP)
				ent->EncodeRemovePacket(this);
			else if (ent->m_bNoPacketPVP && m_bNoPacketPVP)
				EncodeInsertPacket(ent);
		}
	}

	if (m_bObserverModeChange || (pkThis && pkThis->IsGMInvisibleChanged()))
	{
		if (m_bIsObserver || (pkThis->IsGMInvisible()))
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;
				LPENTITY ent = this_it->first;

				if (pkThis && pkThis->IsPC() && (pkThis == ent || (pkThis->GetMountSystem() && pkThis->GetMountSystem()->GetMount() == ent)))
					continue;

				if (this_it->second < m_iViewAge)
				{
					// ³ª·Î ºÎÅÍ »ó´ë¹æÀ» Áö¿î´Ù.
					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					// »ó´ë·Î ºÎÅÍ ³ª¸¦ Áö¿î´Ù.
					ent->ViewRemove(this, false);
				}
				else
				{
					// ³ª·Î ºÎÅÍ »ó´ë¹æÀ» Áö¿î´Ù.
					//ent->EncodeRemovePacket(this);
					//m_map_view.erase(this_it);

					// »ó´ë·Î ºÎÅÍ ³ª¸¦ Áö¿î´Ù.
					//ent->ViewRemove(this, false);
					EncodeRemovePacket(ent);
				}
			}
		}
		else
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;
				LPENTITY ent = this_it->first;

				if (pkThis && pkThis->IsPC() && (pkThis == ent || (pkThis->GetMountSystem() && pkThis->GetMountSystem()->GetMount() == ent)))
					continue;

				if (this_it->second < m_iViewAge)
				{
					// ³ª·Î ºÎÅÍ »ó´ë¹æÀ» Áö¿î´Ù.
					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					// »ó´ë·Î ºÎÅÍ ³ª¸¦ Áö¿î´Ù.
					ent->ViewRemove(this, false);
				}
				else
				{
					if (m_bObserverModeChange)
						ent->EncodeInsertPacket(this);
					EncodeInsertPacket(ent);

					ent->ViewInsert(this, m_bObserverModeChange);
				}
			}
		}

		m_bObserverModeChange = false;
		if (pkThis && GetDesc())
		{
			if (pkThis->GetMountSystem() && pkThis->GetMountSystem()->GetMount())
				pkThis->GetMountSystem()->GetMount()->UpdateSectree();
			pkThis->ResetGMInvisibleChanged();
		}
	}
	else
	{
		if (!m_bIsObserver && (!pkThis || !pkThis->IsGMInvisible()))
		{
			it = m_map_view.begin();

			while (it != m_map_view.end())
			{
				this_it = it++;

				if (this_it->second < m_iViewAge)
				{
					LPENTITY ent = this_it->first;

					// ³ª·Î ºÎÅÍ »ó´ë¹æÀ» Áö¿î´Ù.
					ent->EncodeRemovePacket(this);
					m_map_view.erase(this_it);

					// »ó´ë·Î ºÎÅÍ ³ª¸¦ Áö¿î´Ù.
					ent->ViewRemove(this, false);
				}
			}
		}
	}
}

