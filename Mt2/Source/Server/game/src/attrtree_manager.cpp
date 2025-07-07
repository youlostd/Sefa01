#include "stdafx.h"

#ifdef __ATTRTREE__
#include "attrtree_manager.h"

CAttrtreeManager::CAttrtreeManager()
{
}

CAttrtreeManager::~CAttrtreeManager()
{
}

void CAttrtreeManager::Initialize(const ::google::protobuf::RepeatedPtrField<network::TAttrtreeProto>& table)
{
	for (auto& t : table)
		m_aProto[t.row()][t.col()] = t;

	for (BYTE row = 0; row < ATTRTREE_ROW_NUM; ++row)
	{
		for (BYTE col = 0; col < ATTRTREE_COL_NUM; ++col)
		{
			auto* pCurProto = GetProto(row, col);
			sys_log(0, "AttrtreeManager::Initialize cell(%d, %d) apply(%d, %d) refines(%d, %d, %d, %d, %d)",
				row, col, pCurProto->apply_type(), pCurProto->max_apply_value(),
				pCurProto->refine_level(0),
				pCurProto->refine_level(1),
				pCurProto->refine_level(2),
				pCurProto->refine_level(3),
				pCurProto->refine_level(4));
		}
	}
}

BYTE CAttrtreeManager::CellToID(BYTE row, BYTE col) const
{
	return row * ATTRTREE_COL_NUM + col;
}

void CAttrtreeManager::IDToCell(BYTE id, BYTE& row, BYTE& col) const
{
	row = id / ATTRTREE_COL_NUM;
	col = id % ATTRTREE_COL_NUM;
}

const network::TAttrtreeProto* CAttrtreeManager::GetProto(BYTE row, BYTE col) const
{
	if (row >= ATTRTREE_ROW_NUM || col >= ATTRTREE_COL_NUM)
		return NULL;

	return &m_aProto[row][col];
}

const network::TAttrtreeProto* CAttrtreeManager::GetProto(BYTE id) const
{
	BYTE row, col;
	IDToCell(id, row, col);

	return GetProto(row, col);
}
#endif
