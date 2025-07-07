#pragma once

#include <google/protobuf/repeated_field.h>
#include "protobuf_data.h"

namespace GM {
	extern void init(::google::protobuf::RepeatedField<::google::protobuf::uint32> admin_config);
	extern void insert(const network::TAdminInfo & c_rInfo);
	extern void remove(const char* name);
	extern BYTE get_level(const char * name, const char * account = NULL, bool ignore_test_server = false);
	extern void clear();

	extern bool check_allow(BYTE bGMLevel, DWORD dwCheckFlag);
	extern bool check_account_allow(const std::string& stAccountName, DWORD dwCheckFlag);
}
