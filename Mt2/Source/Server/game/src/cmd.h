#ifndef __INC_METIN_II_GAME_CMD_H__
#define __INC_METIN_II_GAME_CMD_H__

#define ACMD(name)  void (name)(LPCHARACTER ch, const char *argument, int cmd, int subcmd)
#define CMD_NAME(name) cmd_info[cmd].command

struct command_info
{
	const char * command;
	void (*command_pointer) (LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	int subcmd;
	int minimum_position;
	int gm_level;
	BYTE level;
};

struct emotion_type_s
{
	const char *	command;
	const char *	command_to_client;
	long	flag;
	float	extra_delay;
};

extern struct command_info cmd_info[];
extern struct emotion_type_s emotion_types[34];

extern void interpret_command(LPCHARACTER ch, const char * argument, size_t len);
extern void direct_interpret_command(const char * argument, LPCHARACTER ch = NULL);
extern void interpreter_set_privilege(const char * cmd, int lvl);

enum SCMD_ACTION
{
	SCMD_SLAP,
	SCMD_KISS,
	SCMD_FRENCH_KISS,
	SCMD_HUG,
	SCMD_LONG_HUG,
	SCMD_SHOLDER,
	SCMD_FOLD_ARM
};

enum SCMD_CMD
{
	SCMD_LOGOUT,
	SCMD_QUIT,
	SCMD_PHASE_SELECT,
	SCMD_SHUTDOWN,
};

enum SCMD_RESTART
{
	SCMD_RESTART_TOWN,
	SCMD_RESTART_HERE,
	SCMD_RESTART_BASE
#ifdef COMBAT_ZONE
	, SCMD_RESTART_COMBAT_ZONE
#endif
};

enum SCMD_XMAS
{
	SCMD_XMAS_BOOM,
	SCMD_XMAS_SNOW,
	SCMD_XMAS_SANTA,
};

extern void Shutdown(int iSec);
extern void __StopCurrentShutdown(bool bSendP2P = false);
extern void SendNotice(const char * c_pszBuf, int iLangID = -1);		// ÀÌ °ÔÀÓ¼­¹ö¿¡¸¸ °øÁö
extern void SendBigNotice(const char * c_pszBuf, BYTE bEmpire = 0);
extern void SendSuccessNotice(const char * c_pszBuf, int iLang = -1);
extern void SendLog(const char * c_pszBuf);		// ¿î¿µÀÚ¿¡°Ô¸¸ °øÁö
extern void BroadcastNotice(const char * c_pszBuf, int iLangID = -1, BYTE bChannel = 0);	// Àü ¼­¹ö¿¡ °øÁö
extern void BroadcastBigNotice(const char * c_pszBuf, BYTE bEmpire = 0);
extern void BroadcastSuccessNotice(const char * c_pszBuf, int iLang = -1);
extern void SendNoticeMap(const char* c_pszBuf, int nMapIndex, bool bBigFont, int iLangID = -1); // ÁöÁ¤ ¸Ê¿¡¸¸ °øÁö

// LUA_ADD_BGM_INFO
void CHARACTER_AddBGMInfo(unsigned mapIndex, const char* name, float vol);
// END_OF_LUA_ADD_BGM_INFO

void CHARACTER_AddWarpLevelLimit(int iMapIndex, int iLevelLimit, bool bIsLimitMin);

// LUA_ADD_GOTO_INFO
extern void CHARACTER_AddGotoInfo(const std::string& c_st_name, BYTE empire, int mapIndex, DWORD x, DWORD y);
// END_OF_LUA_ADD_GOTO_INFO

extern bool CHARACTER_GoToName(LPCHARACTER ch, BYTE empire, int mapIndex, const char* gotoName);

#endif
