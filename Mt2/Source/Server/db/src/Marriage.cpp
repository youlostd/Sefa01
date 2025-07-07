#include "stdafx.h"
#include "Marriage.h"
#include "Main.h"
#include "DBManager.h"
#include "ClientManager.h"

namespace marriage
{
	const DWORD WEDDING_LENGTH = 60 * 60; // sec
	bool operator < (const TWedding& lhs, const TWedding& rhs)
	{
		return lhs.dwTime < rhs.dwTime;
	}

	bool operator > (const TWedding& lhs, const TWedding& rhs)
	{
		return lhs.dwTime > rhs.dwTime;
	}

	bool operator > (const TWeddingInfo &lhs, const TWeddingInfo& rhs)
	{
		return lhs.dwTime > rhs.dwTime;
	}

	using namespace std;

	CManager::CManager()
	{
	}

	CManager::~CManager()
	{
	}

	bool CManager::Initialize()
	{
		char szQuery[1024];

		snprintf(szQuery, sizeof(szQuery),
				"SELECT pid1, pid2, love_point, time, is_married, p1.name, p2.name FROM marriage, player as p1, player as p2 WHERE p1.id = pid1 AND p2.id = pid2");

		auto_ptr<SQLMsg> pmsg_delete(CDBManager::instance().DirectQuery("DELETE FROM marriage WHERE is_married = 0"));
		auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult * pRes = pmsg->Get();
		sys_log(0, "MarriageList(size=%lu)", pRes->uiNumRows);

		if (pRes->uiNumRows > 0)
		{
			for (uint uiRow = 0; uiRow != pRes->uiNumRows; ++uiRow)
			{
				MYSQL_ROW row = mysql_fetch_row(pRes->pSQLResult);

				DWORD pid1 = 0; str_to_number(pid1, row[0]);
				DWORD pid2 = 0; str_to_number(pid2, row[1]);
				int love_point = 0; str_to_number(love_point, row[2]);
				DWORD time = 0; str_to_number(time, row[3]);
				BYTE is_married = 0; str_to_number(is_married, row[4]);
				const char* name1 = row[5];
				const char* name2 = row[6];

				TMarriage* pMarriage = new TMarriage(pid1, pid2, love_point, time, is_married, name1, name2);
				m_Marriages.insert(pMarriage);
				m_MarriageByPID.insert(make_pair(pid1, pMarriage));
				m_MarriageByPID.insert(make_pair(pid2, pMarriage));

				sys_log(0, "Marriage %lu: LP:%d TM:%u ST:%d %10lu:%16s %10lu:%16s ", uiRow, love_point, time, is_married, pid1, name1, pid2, name2);
			}
		}
		return true;
	}

	TMarriage* CManager::Get(DWORD dwPlayerID)
	{
		auto it = m_MarriageByPID.find(dwPlayerID);

		if (it != m_MarriageByPID.end())
			return it->second;

		return NULL;
	}

	void Align(DWORD& rPID1, DWORD& rPID2)
	{
		if (rPID1 > rPID2)
			std::swap(rPID1, rPID2);
	}

	void CManager::Add(DWORD dwPID1, DWORD dwPID2, const char* szName1, const char* szName2)
	{
		DWORD now = CClientManager::instance().GetCurrentTime();
		if (IsMarried(dwPID1) || IsMarried(dwPID2))
		{
			// sys_err("cannot marry already married character. %d - %d", dwPID1, dwPID2);
			return;
		}

		Align(dwPID1, dwPID2);

		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "INSERT INTO marriage(pid1, pid2, love_point, time, is_married) VALUES (%u, %u, 0, %u, 1)", dwPID1, dwPID2, now);

		auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult* res = pmsg->Get();
		if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
		{
			sys_err("cannot insert marriage");
			return;
		}

		sys_log(0, "MARRIAGE ADD %u %u", dwPID1, dwPID2);

		TMarriage* pMarriage = new TMarriage(dwPID1, dwPID2, 0, now, 0, szName1, szName2);
		m_Marriages.insert(pMarriage);
		m_MarriageByPID.insert(make_pair(dwPID1, pMarriage));
		m_MarriageByPID.insert(make_pair(dwPID2, pMarriage));

		network::DGOutputPacket<network::DGMarriageAddPacket> p;
		p->set_pid1(dwPID1);
		p->set_pid2(dwPID2);
		p->set_marry_time(now);
		p->set_name1(szName1);
		p->set_name2(szName2);
		CClientManager::instance().ForwardPacket(p);
	}

	void CManager::Update(DWORD dwPID1, DWORD dwPID2, INT iLovePoint, BYTE byMarried)
	{
		TMarriage* pMarriage = Get(dwPID1);
		if (!pMarriage || pMarriage->GetOther(dwPID1) != dwPID2)
		{
			sys_err("not under marriage : %u %u", dwPID1, dwPID2);
			return;
		}

		Align(dwPID1, dwPID2);

		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "UPDATE marriage SET love_point = %d, is_married = %d WHERE pid1 = %u AND pid2 = %u", 
				iLovePoint, byMarried, pMarriage->pid1, pMarriage->pid2);

		auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult* res = pmsg->Get();
		if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
			return;

		sys_log(0, "MARRIAGE UPDATE PID:%u %u LP:%u ST:%d", dwPID1, dwPID2, iLovePoint, byMarried);
		pMarriage->love_point = iLovePoint;
		pMarriage->is_married = byMarried;

		network::DGOutputPacket<network::DGMarriageUpdatePacket> p;
		p->set_pid1(dwPID1);
		p->set_pid2(dwPID2);
		p->set_love_point(pMarriage->love_point);
		p->set_married(pMarriage->is_married);
		CClientManager::instance().ForwardPacket(p);
	}

	void CManager::Remove(DWORD dwPID1, DWORD dwPID2)
	{
		TMarriage* pMarriage = Get(dwPID1);

		if (pMarriage)
		{
			sys_log(0, "Break Marriage pid1 %d pid2 %d Other %d", dwPID1, dwPID2, pMarriage->GetOther(dwPID1));
		}
		if (!pMarriage || pMarriage->GetOther(dwPID1) != dwPID2)
		{
			auto it = m_MarriageByPID.begin();

			for (; it != m_MarriageByPID.end(); ++it)
			{
				sys_log(0, "Marriage List pid1 %d pid2 %d", it->second->pid1, it->second->pid2);
			}
			sys_err("not under marriage : PID:%u %u", dwPID1, dwPID2);
			return;
		}

		Align(dwPID1, dwPID2);

		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM marriage WHERE pid1 = %u AND pid2 = %u", dwPID1, dwPID2);

		auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult* res = pmsg->Get();
		if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
		{
			sys_err("cannot delete marriage : PID:%u %u", dwPID1, dwPID2);
			return;
		}

		sys_log(0, "MARRIAGE REMOVE PID:%u %u", dwPID1, dwPID2);

		m_Marriages.erase(pMarriage);
		m_MarriageByPID.erase(dwPID1);
		m_MarriageByPID.erase(dwPID2);

		network::DGOutputPacket<network::DGMarriageRemovePacket> p;
		p->set_pid1(dwPID1);
		p->set_pid2(dwPID2);
		CClientManager::instance().ForwardPacket(p);

		delete pMarriage;
	}

	void CManager::EngageToMarriage(DWORD dwPID1, DWORD dwPID2)
	{
		TMarriage* pMarriage = Get(dwPID1);
		if (!pMarriage || pMarriage->GetOther(dwPID1) != dwPID2)
		{
			sys_err("not under marriage : PID:%u %u", dwPID1, dwPID2);
			return;
		}

		if (pMarriage->is_married)
		{
			sys_err("already married, cannot change engage to marry : PID:%u %u", dwPID1, dwPID2);
			return;
		}

		Align(dwPID1, dwPID2);

		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "UPDATE marriage SET is_married = 1 WHERE pid1 = %u AND pid2 = %u", 
				pMarriage->pid1, pMarriage->pid2);

		auto_ptr<SQLMsg> pmsg(CDBManager::instance().DirectQuery(szQuery));

		SQLResult* res = pmsg->Get();
		if (res->uiAffectedRows == 0 || res->uiAffectedRows == (uint32_t)-1)
		{
			sys_err("cannot change engage to marriage : PID:%u %u", dwPID1, dwPID2);
			return;
		}

		sys_log(0, "MARRIAGE ENGAGE->MARRIAGE PID:%u %u", dwPID1, dwPID2);
		pMarriage->is_married = 1;

		network::DGOutputPacket<network::DGMarriageUpdatePacket> p;
		p->set_pid1(dwPID1);
		p->set_pid2(dwPID2);
		p->set_love_point(pMarriage->love_point);
		p->set_married(pMarriage->is_married);
		CClientManager::instance().ForwardPacket(p);
	}

	void CManager::OnSetup(CPeer* peer)
	{
		// 결혼한 사람들 보내기
		for (auto it = m_Marriages.begin(); it != m_Marriages.end(); ++it)
		{
			TMarriage* pMarriage = *it;

			{
				network::DGOutputPacket<network::DGMarriageAddPacket> p;
				p->set_pid1(pMarriage->pid1);
				p->set_pid2(pMarriage->pid2);
				p->set_marry_time(pMarriage->time);
				p->set_name1(pMarriage->name1);
				p->set_name2(pMarriage->name2);
				CClientManager::instance().ForwardPacket(p);
			}

			{
				network::DGOutputPacket<network::DGMarriageUpdatePacket> p;
				p->set_pid1(pMarriage->pid1);
				p->set_pid2(pMarriage->pid2);
				p->set_love_point(pMarriage->love_point);
				p->set_married(pMarriage->is_married);
				CClientManager::instance().ForwardPacket(p);
			}
		}

		// 결혼식 보내기
		for (auto it = m_mapRunningWedding.begin(); it != m_mapRunningWedding.end(); ++it)
		{
			const TWedding& t = it->second;
			
			network::DGOutputPacket<network::DGWeddingReadyPacket> p;
			p->set_pid1(t.dwPID1);
			p->set_pid2(t.dwPID2);
			p->set_map_index(t.dwMapIndex);
			CClientManager::instance().ForwardPacket(p);


			network::DGOutputPacket<network::DGWeddingStartPacket> p2;
			p2->set_pid1(t.dwPID1);
			p2->set_pid2(t.dwPID2);
			CClientManager::instance().ForwardPacket(p2);

		}
	}

	void CManager::ReadyWedding(DWORD dwMapIndex, DWORD dwPID1, DWORD dwPID2)
	{
		DWORD dwStartTime = CClientManager::instance().GetCurrentTime();
		m_pqWeddingStart.push(TWedding(dwStartTime + 5, dwMapIndex, dwPID1, dwPID2));
	}

	void CManager::EndWedding(DWORD dwPID1, DWORD dwPID2)
	{
		auto it = m_mapRunningWedding.find(make_pair(dwPID1, dwPID2));
		if (it == m_mapRunningWedding.end())
		{
			sys_err("try to end wedding %u %u", dwPID1, dwPID2);
			return;
		}

		TWedding& w = it->second;

		network::DGOutputPacket<network::DGWeddingEndPacket> p;
		p->set_pid1(w.dwPID1);
		p->set_pid2(w.dwPID2);
		CClientManager::instance().ForwardPacket(p);
		m_mapRunningWedding.erase(it);
	}

	void CManager::Update()
	{
		DWORD now = CClientManager::instance().GetCurrentTime();

		if (!m_pqWeddingEnd.empty())
		{
			while (!m_pqWeddingEnd.empty() && m_pqWeddingEnd.top().dwTime <= now)
			{
				TWeddingInfo wi = m_pqWeddingEnd.top();
				m_pqWeddingEnd.pop();

				auto it = m_mapRunningWedding.find(make_pair(wi.dwPID1, wi.dwPID2));
				if (it == m_mapRunningWedding.end())
					continue;

				TWedding& w = it->second;


				network::DGOutputPacket<network::DGWeddingEndPacket> p;
				p->set_pid1(w.dwPID1);
				p->set_pid2(w.dwPID2);
				CClientManager::instance().ForwardPacket(p);
				m_mapRunningWedding.erase(it);

				auto it_marriage = m_MarriageByPID.find(w.dwPID1);

				if (it_marriage != m_MarriageByPID.end())
				{
					TMarriage* pMarriage = it_marriage->second;
					if (!pMarriage->is_married)
					{
						Remove(pMarriage->pid1, pMarriage->pid2);
					}
				}
			}
		}
		if (!m_pqWeddingStart.empty())
		{
			while (!m_pqWeddingStart.empty() && m_pqWeddingStart.top().dwTime <= now)
			{
				TWedding w = m_pqWeddingStart.top();
				m_pqWeddingStart.pop();
				
				network::DGOutputPacket<network::DGWeddingStartPacket> p;
				p->set_pid1(w.dwPID1);
				p->set_pid2(w.dwPID2);
				CClientManager::instance().ForwardPacket(p);

				w.dwTime += WEDDING_LENGTH;
				m_pqWeddingEnd.push(TWeddingInfo(w.dwTime, w.dwPID1, w.dwPID2));
				m_mapRunningWedding.insert(make_pair(make_pair(w.dwPID1, w.dwPID2), w));
			}
		}
	}
}
