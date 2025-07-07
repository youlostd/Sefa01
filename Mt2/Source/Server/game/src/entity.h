#ifndef __INC_METIN_II_GAME_ENTITY_H__
#define __INC_METIN_II_GAME_ENTITY_H__

class SECTREE;
class CHARACTER;

#include "headers.hpp"
#include "desc.h"

class CEntity
{
	public:
		typedef TR1_NS::unordered_map<LPENTITY, int> ENTITY_MAP;

	public:
		CEntity();
		virtual	~CEntity();

		virtual void	EncodeInsertPacket(LPENTITY entity) = 0;
		virtual	void	EncodeRemovePacket(LPENTITY entity) = 0;

	protected:
		void			Initialize(int type = -1);
		void			Destroy();


	public:
		void			SetType(int type);
		int				GetType() const;
		bool			IsType(int type) const;

		void			ViewCleanup();
		void			ViewInsert(LPENTITY entity, bool recursive = true);
		void			ViewRemove(LPENTITY entity, bool recursive = true);
		void			ViewReencode();	// ÁÖÀ§ Entity¿¡ ÆÐÅ¶À» ´Ù½Ã º¸³½´Ù.

		int				GetViewAge() const	{ return m_iViewAge;	}

		long			GetX() const		{ return m_pos.x; }
		long			GetY() const		{ return m_pos.y; }
		long			GetZ() const		{ return m_pos.z; }
		const PIXEL_POSITION &	GetXYZ() const		{ return m_pos; }

		void			SetXYZ(long x, long y, long z)		{ m_pos.x = x, m_pos.y = y, m_pos.z = z; }
		void			SetXYZ(const PIXEL_POSITION & pos)	{ m_pos = pos; }

		LPSECTREE		GetSectree() const			{ return m_pSectree;	}
		void			SetSectree(LPSECTREE tree)	{ m_pSectree = tree;	}

		void			UpdateSectree();
		template <typename T>
		void			PacketAround(network::GCOutputPacket<T> packet, LPENTITY except = NULL) { PacketView(packet, except); }
		template <typename T>
		void			PacketView(network::GCOutputPacket<T> packet, LPENTITY except = NULL)
		{
			if (!GetSectree())
				return;

			if (!m_bIsObserver)
			{
				for_each(m_map_view.begin(), m_map_view.end(), [&packet, &except](const CEntity::ENTITY_MAP::value_type& v) {
					auto ent = v.first;
					if (ent == except)
						return;

					if (ent->GetDesc())
						ent->GetDesc()->Packet(packet);
				});
			}

			if (GetDesc() && this != except)
				GetDesc()->Packet(packet);
		}

		void			BindDesc(LPDESC _d)	 { m_lpDesc = _d; }
		LPDESC			GetDesc() const			{ return m_lpDesc; }

		void			SetMapIndex(long l)	{ m_lMapIndex = l; }
		long			GetMapIndex() const	{ return m_lMapIndex; }

		void			SetObserverMode(bool bFlag);
		bool			IsObserverMode() const	{ return m_bIsObserver; }

		bool			IsNoPVPPacketMode() { return m_bNoPacketPVP; }

	protected:
		bool			m_bIsObserver;
		bool			m_bObserverModeChange;
		ENTITY_MAP		m_map_view;
		long			m_lMapIndex;
		bool			m_bNoPacketPVP;
		bool			m_bNoPacketPVPModeChange;

	private:
		LPDESC			m_lpDesc;

		int			m_iType;
		bool			m_bIsDestroyed;

		PIXEL_POSITION		m_pos;

		int			m_iViewAge;

		LPSECTREE		m_pSectree;
};

#endif
