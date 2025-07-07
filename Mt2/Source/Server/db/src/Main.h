#ifndef __INC_MAIN_H__
#define __INC_MAIN_H__

enum EProtoLoadingMethods {
	PROTO_LOADING_DATABASE,
	PROTO_LOADING_TEXTFILE,
};

int	Start();
void End();
const char * GetPlayerDBName();

extern std::string astLocaleStringNames[LANGUAGE_MAX_NUM];

#endif
