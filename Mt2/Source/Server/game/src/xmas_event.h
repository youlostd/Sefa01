#ifndef __INC_XMAS_EVENT_H
#define __INC_XMAS_EVENT_H

namespace xmas
{
	enum
	{
		MOB_XMAS_TREE_VNUM = 20032,
		MOB_XMAS_FIRWORK_SELLER_VNUM = 9004,
	};

	void ProcessEventFlag(const std::string& name, int prev_value, int value);
}
#endif
