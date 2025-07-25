﻿#include "stdafx.h"
#include "constants.h"
#include "priv_manager.h"
#include "char.h"
#include "desc_client.h"
#include "guild.h"
#include "guild_manager.h"
#include "unique_item.h"
#include "utils.h"
#include "log.h"
#include "cmd.h"

static const char * GetEmpireName(int priv)
{
	return c_apszEmpireNames[priv];
}

static const char * GetPrivName(int priv)
{
	return c_apszPrivNames[priv];
}

CPrivManager::CPrivManager()
{
	memset(m_aakPrivEmpireData, 0, sizeof(m_aakPrivEmpireData));
}

void CPrivManager::RequestGiveGuildPriv(DWORD guild_id, unsigned char type, int value, time_t duration_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: RequestGiveGuildPriv: wrong guild priv type(%u)", type);
		return;
	}

	value = MINMAX(0, value, 50);
	duration_sec = MINMAX(0, duration_sec, 60 * 60 * 24 * 7);

	network::GDOutputPacket<network::GDRequestGuildPrivPacket> p;
	p->set_type(type);
	p->set_value(value);
	p->set_guild_id(guild_id);
	p->set_duration_sec(duration_sec);

	db_clientdesc->DBPacket(p);
}

void CPrivManager::RequestGiveEmpirePriv(unsigned char empire, unsigned char type, int value, time_t duration_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: RequestGiveEmpirePriv: wrong empire priv type(%u)", type);
		return;
	}

	value = MINMAX(0, value, 200);
	duration_sec = MINMAX(0, duration_sec, 60 * 60 * 24 * 7);

	network::GDOutputPacket<network::GDRequestEmpirePrivPacket> p;
	p->set_type(type);
	p->set_value(value);
	p->set_empire(empire);
	p->set_duration_sec(duration_sec);

	db_clientdesc->DBPacket(p);
}

void CPrivManager::RequestGiveCharacterPriv(DWORD pid, unsigned char type, int value)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: RequestGiveCharacterPriv: wrong char priv type(%u)", type);
		return;
	}

	value = MINMAX(0, value, 100);

	network::GDOutputPacket<network::GDRequestCharacterPrivPacket> p;
	p->set_type(type);
	p->set_value(value);
	p->set_pid(pid);

	db_clientdesc->DBPacket(p);
}

void CPrivManager::GiveGuildPriv(DWORD guild_id, unsigned char type, int value, unsigned char bLog, time_t end_time_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: GiveGuildPriv: wrong guild priv type(%u)", type);
		return;
	}

	sys_log(0, "Set Guild Priv: guild_id(%u) type(%d) value(%d) duration_sec(%d)", guild_id, type, value, end_time_sec - get_global_time());

	value = MINMAX(0, value, 50);
	end_time_sec = MINMAX(0, end_time_sec, get_global_time() + 60 * 60 * 24 * 7);

	m_aPrivGuild[type][guild_id].value = value;
	m_aPrivGuild[type][guild_id].end_time_sec = end_time_sec;

	CGuild* g = CGuildManager::instance().FindGuild(guild_id);

	if (g)
	{
		if (value)
		{
			char buf[100];
			snprintf(buf, sizeof(buf), "[%s] ±æµåÀÇ [LC_TEXT=%s]ÀÌ [%d]%% Áõ°¡Çß½À´Ï´Ù!", g->GetName(), GetPrivName(type), value);
			SendNotice(buf);
		}
		else
		{
			char buf[100];
			snprintf(buf, sizeof(buf), "[%s] ±æµåÀÇ [LC_TEXT=%s]ÀÌ Á¤»óÀ¸·Î µ¹¾Æ¿Ô½À´Ï´Ù.", g->GetName(), GetPrivName(type));
			SendNotice(buf);
		}

		//	if (bLog)
		//	{
		//		LogManager::instance().CharLog(0, guild_id, type, value, "GUILD_PRIV", "", "");
		//	}
	}
}

void CPrivManager::GiveCharacterPriv(DWORD pid, unsigned char type, int value, unsigned char bLog)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: GiveCharacterPriv: wrong char priv type(%u)", type);
		return;
	}

	sys_log(0, "Set Character Priv %u %d %d", pid, type, value);

	value = MINMAX(0, value, 100);

	m_aPrivChar[type][pid] = value;

	//	if (bLog)
	//		LogManager::instance().CharLog(pid, 0, type, value, "CHARACTER_PRIV", "", "");
}

void CPrivManager::GiveEmpirePriv(unsigned char empire, unsigned char type, int value, unsigned char bLog, time_t end_time_sec)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: GiveEmpirePriv: wrong empire priv type(%u)", type);
		return;
	}

	sys_log(0, "Set Empire Priv: empire(%d) type(%d) value(%d) duration_sec(%d)", empire, type, value, end_time_sec - get_global_time());

	value = MINMAX(0, value, 200);
	end_time_sec = MINMAX(0, end_time_sec, get_global_time() + 60 * 60 * 24 * 7);

	SPrivEmpireData& rkPrivEmpireData = m_aakPrivEmpireData[type][empire];
	rkPrivEmpireData.m_value = value;
	rkPrivEmpireData.m_end_time_sec = end_time_sec;

	if (value)
	{
		char buf[100];
		snprintf(buf, sizeof(buf), "[LC_TEXT=%s]ÀÇ [LC_TEXT=%s]ÀÌ [%d]%% Áõ°¡Çß½À´Ï´Ù!", GetEmpireName(empire), GetPrivName(type), value);

		if (empire)
			SendNotice(buf);
		else
			SendLog(buf);
	}
	else
	{
		char buf[100];
		snprintf(buf, sizeof(buf), "[LC_TEXT=%s]ÀÇ [LC_TEXT=%s]ÀÌ Á¤»óÀ¸·Î µ¹¾Æ¿Ô½À´Ï´Ù.", GetEmpireName(empire), GetPrivName(type));

		if (empire)
			SendNotice(buf);
		else
			SendLog(buf);
	}

	//	if (bLog)
	//	{
	//		LogManager::instance().CharLog(0, empire, type, value, "EMPIRE_PRIV", "", "");
	//	}
}

void CPrivManager::RemoveGuildPriv(DWORD guild_id, unsigned char type)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: RemoveGuildPriv: wrong guild priv type(%u)", type);
		return;
	}

	m_aPrivGuild[type][guild_id].value = 0;
	m_aPrivGuild[type][guild_id].end_time_sec = 0;
}

void CPrivManager::RemoveEmpirePriv(unsigned char empire, unsigned char type)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: RemoveEmpirePriv: wrong empire priv type(%u)", type);
		return;
	}

	SPrivEmpireData& rkPrivEmpireData = m_aakPrivEmpireData[type][empire];
	rkPrivEmpireData.m_value = 0;
	rkPrivEmpireData.m_end_time_sec = 0;
}

void CPrivManager::RemoveCharacterPriv(DWORD pid, unsigned char type)
{
	if (MAX_PRIV_NUM <= type)
	{
		sys_err("PRIV_MANAGER: RemoveCharacterPriv: wrong char priv type(%u)", type);
		return;
	}

	itertype(m_aPrivChar[type]) it = m_aPrivChar[type].find(pid);

	if (it != m_aPrivChar[type].end())
		m_aPrivChar[type].erase(it);
}

int CPrivManager::GetPriv(LPCHARACTER ch, unsigned char type)
{
	// Ä³¸¯ÅÍÀÇ º¯°æ ¼öÄ¡°¡ -¶ó¸é ¹«Á¶°Ç -¸¸ Àû¿ëµÇ°Ô
	int val_ch = GetPrivByCharacter(ch->GetPlayerID(), type);

	if (val_ch < 0 && !ch->IsEquipUniqueItem(UNIQUE_ITEM_NO_BAD_LUCK_EFFECT))
		return val_ch;
	else
	{
		int val;

		// °³ÀÎ, Á¦±¹, ±æµå, ÀüÃ¼ Áß Å« °ªÀ» ÃëÇÑ´Ù.
		val = MAX(val_ch, GetPrivByEmpire(0, type));
		val = MAX(val, GetPrivByEmpire(ch->GetEmpire(), type));

		if (ch->GetGuild())
			val = MAX(val, GetPrivByGuild(ch->GetGuild()->GetID(), type));

		return val;
	}
}

int CPrivManager::GetPrivByEmpire(unsigned char bEmpire, unsigned char type)
{
	SPrivEmpireData* pkPrivEmpireData = GetPrivByEmpireEx(bEmpire, type);

	if (pkPrivEmpireData)
		return pkPrivEmpireData->m_value;

	return 0;
}

CPrivManager::SPrivEmpireData* CPrivManager::GetPrivByEmpireEx(unsigned char bEmpire, unsigned char type)
{
	if (type >= MAX_PRIV_NUM)
		return NULL;

	if (bEmpire > EMPIRE_MAX_NUM)
		return NULL;

	return &m_aakPrivEmpireData[type][bEmpire];
}

int CPrivManager::GetPrivByGuild(DWORD guild_id, unsigned char type)
{
	if (type >= MAX_PRIV_NUM)
		return 0;

	itertype(m_aPrivGuild[type]) itFind = m_aPrivGuild[type].find(guild_id);

	if (itFind == m_aPrivGuild[type].end())
		return 0;

	return itFind->second.value;
}

const CPrivManager::SPrivGuildData* CPrivManager::GetPrivByGuildEx(DWORD dwGuildID, unsigned char byType) const
{
	if (byType >= MAX_PRIV_NUM)
		return NULL;

	itertype(m_aPrivGuild[byType]) itFind = m_aPrivGuild[byType].find(dwGuildID);

	if (itFind == m_aPrivGuild[byType].end())
		return NULL;

	return &itFind->second;
}

int CPrivManager::GetPrivByCharacter(DWORD pid, unsigned char type)
{
	if (type >= MAX_PRIV_NUM)
		return 0;

	itertype(m_aPrivChar[type]) it = m_aPrivChar[type].find(pid);

	if (it != m_aPrivChar[type].end())
		return it->second;

	return 0;
}

