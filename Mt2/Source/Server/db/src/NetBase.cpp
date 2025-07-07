#include "stdafx.h"
#include "NetBase.h"
#include "Config.h"
#include "ClientManager.h"

LPFDWATCH CNetBase::m_fdWatcher = NULL;

CNetBase::CNetBase()
{
}

CNetBase::~CNetBase()
{
}

bool CNetBase::Create()
{
	sys_log(1, "NetPoller::Create()");

	if (m_fdWatcher)
		return true;

	m_fdWatcher = fdwatch_new(512);

	if (!m_fdWatcher)
	{
		Destroy();
		return false;
	}

	return true;
}

void CNetBase::Destroy()
{
	sys_log(1, "NetPoller::Destroy()");

	if (m_fdWatcher)
	{
		fdwatch_delete(m_fdWatcher);
		m_fdWatcher = NULL;
	}

	thecore_destroy();
}
