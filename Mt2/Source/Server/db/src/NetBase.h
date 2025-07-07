// vim: ts=8 sw=4
#ifndef __INC_NETWORKBASE_H__
#define __INC_NETWORKBASE_H__

class CNetBase
{
    public:
	CNetBase();
	virtual ~CNetBase();

	static bool Create();
	static void Destroy();

    protected:
	static LPFDWATCH	m_fdWatcher;
};

#endif
