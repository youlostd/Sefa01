#include "stdafx.h"
#include "PythonExchange.h"

void CPythonExchange::SetSelfName(const char *name)
{
	strncpy(m_self.name, name, CHARACTER_NAME_MAX_LEN);
}

void CPythonExchange::SetTargetName(const char *name)
{
	strncpy(m_victim.name, name, CHARACTER_NAME_MAX_LEN);
}

char * CPythonExchange::GetNameFromSelf()
{
	return m_self.name;
}

char * CPythonExchange::GetNameFromTarget()
{
	return m_victim.name;
}

void CPythonExchange::SetElkToTarget(long long elk)
{	
	m_victim.elk = elk;
}

void CPythonExchange::SetElkToSelf(long long elk)
{
	m_self.elk = elk;
}

long long CPythonExchange::GetElkFromTarget()
{
	return m_victim.elk;
}

long long CPythonExchange::GetElkFromSelf()
{
	return m_self.elk;
}

void CPythonExchange::SetItemToTarget(DWORD pos, const network::TItemData& item)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return;

	m_victim.item[pos] = item;
}

void CPythonExchange::SetItemToSelf(DWORD pos, const network::TItemData& item)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return;

	m_self.item[pos] = item;
}

void CPythonExchange::DelItemOfTarget(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return;

	m_victim.item[pos].Clear();
}

void CPythonExchange::DelItemOfSelf(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return;

	m_self.item[pos].Clear();
}

const network::TItemData* CPythonExchange::GetItemFromTarget(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return nullptr;

	return &m_victim.item[pos];
}

const network::TItemData* CPythonExchange::GetItemFromSelf(BYTE pos)
{
	if (pos >= EXCHANGE_ITEM_MAX_NUM)
		return nullptr;

	return &m_self.item[pos];
}

void CPythonExchange::SetAcceptToTarget(BYTE Accept)
{
	m_victim.accept = Accept ? true : false;
}

void CPythonExchange::SetAcceptToSelf(BYTE Accept)
{
	m_self.accept = Accept ? true : false;
}

bool CPythonExchange::GetAcceptFromTarget()
{
	return m_victim.accept ? true : false;
}

bool CPythonExchange::GetAcceptFromSelf()
{
	return m_self.accept ? true : false;
}

bool CPythonExchange::GetElkMode()
{
	return m_elk_mode;
}

void CPythonExchange::SetElkMode(bool value)
{
	m_elk_mode = value;
}

void CPythonExchange::Start()
{
	m_isTrading = true;
}

void CPythonExchange::End()
{
	m_isTrading = false;
}

bool CPythonExchange::isTrading()
{
	return m_isTrading;
}

void CPythonExchange::Clear()
{
	m_self = TExchangeData();
	m_victim = TExchangeData();
/*
	m_self.item_vnum[0] = 30;
	m_victim.item_vnum[0] = 30;
	m_victim.item_vnum[1] = 40;
	m_victim.item_vnum[2] = 50;
*/
}

CPythonExchange::CPythonExchange()
{
	Clear();
	m_isTrading = false;
	m_elk_mode = false;
		// Clear로 옴겨놓으면 안됨. 
		// trade_start 페킷이 오면 Clear를 실행하는데
		// m_elk_mode는 클리어 되선 안됨.;  
}
CPythonExchange::~CPythonExchange()
{
}