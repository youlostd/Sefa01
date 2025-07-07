#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "utils.h"

extern int test_server;

namespace GM {
	DWORD g_adwAdminConfig[GM_MAX_NUM];

	std::map<std::string, network::TAdminInfo> g_map_GM;

	void init(::google::protobuf::RepeatedField<::google::protobuf::uint32> admin_config)
	{
		for (auto i = 0; i < admin_config.size(); ++i)
			g_adwAdminConfig[i] = admin_config[i];
	}

	void insert(const network::TAdminInfo &rAdminInfo)
	{
		sys_log(0, "InsertGMList(account:%s, player:%s, auth:%d)",
			rAdminInfo.account().c_str(),
			rAdminInfo.name().c_str(),
			rAdminInfo.authority());

		g_map_GM[rAdminInfo.name()] = rAdminInfo;
	}

	void remove(const char* szName)
	{
		g_map_GM.erase(szName);
	}

	BYTE get_level(const char * name, const char* account, bool ignore_test_server)
	{
		if (!ignore_test_server && test_server) return GM_IMPLEMENTOR;

		std::map<std::string, network::TAdminInfo>::iterator it = g_map_GM.find(name);

		if (g_map_GM.end() == it)
			return GM_PLAYER;

		if (account)
		{
			if (strcasecmp(it->second.account().c_str(), account) != 0 && strcmp(it->second.account().c_str(), "[ALL]") != 0)
			{
				sys_err("GM::get_level: account compare failed [real account %s need account %s]", account, it->second.account().c_str());
				return GM_PLAYER;
			}
		}

		sys_log(0, "GM::GET_LEVEL : FIND ACCOUNT");
		return it->second.authority();
	}

	void clear()
	{
		g_map_GM.clear();
	}

	bool check_allow(BYTE bGMLevel, DWORD dwCheckFlag)
	{
		return IS_SET(g_adwAdminConfig[bGMLevel], dwCheckFlag);
	}

	bool check_account_allow(const std::string& stAccountName, DWORD dwCheckFlag)
	{
		auto it = g_map_GM.begin();

		bool bHasGM = false;
		bool bCheck = false;
		while (it != g_map_GM.end() && !bCheck)
		{
			if (stAccountName.compare(it->second.account()))
			{
				bHasGM = true;

				BYTE bGMLevel = it->second.authority();
				bCheck = check_allow(bGMLevel, dwCheckFlag);
			}

			++it;
		}

		return !bHasGM || bCheck;
	}
}
