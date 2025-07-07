#pragma once

#include "stdafx.h"

#ifdef __ATTRTREE__
#include "../common/tables.h"

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

class CAttrtreeManager : public singleton<CAttrtreeManager>
{
	public:

	public:
		CAttrtreeManager();
		~CAttrtreeManager();

		void	Initialize(const ::google::protobuf::RepeatedPtrField<network::TAttrtreeProto>& table);

		// helper functions
		BYTE	CellToID(BYTE row, BYTE col) const;
		void	IDToCell(BYTE id, BYTE& row, BYTE& col) const;

		// proto functions
		const network::TAttrtreeProto*	GetProto(BYTE id) const;
		const network::TAttrtreeProto*	GetProto(BYTE row, BYTE col) const;

	private:
		network::TAttrtreeProto	m_aProto[ATTRTREE_ROW_NUM][ATTRTREE_COL_NUM];
};
#endif
