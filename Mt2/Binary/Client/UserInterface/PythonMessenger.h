#pragma once

class CPythonMessenger : public CSingleton<CPythonMessenger>
{
	public:
		typedef std::set<std::string> TFriendNameMap;
		typedef std::map<std::string, BYTE> TGuildMemberStateMap;
		typedef std::set<std::string> TTeamNameMap;
		#ifdef ENABLE_MESSENGER_BLOCK
		typedef std::set<std::string> TBlockNameMap;
		#endif
		

		enum EMessengerGroupIndex
		{
			MESSENGER_GRUOP_INDEX_FRIEND,
			MESSENGER_GRUOP_INDEX_GUILD,
			MESSENGER_GROUP_INDEX_TEAM,
			#ifdef ENABLE_MESSENGER_BLOCK
			MESSENGER_GROUP_INDEX_BLOCK,
			#endif
		};

	public:
		CPythonMessenger();
		virtual ~CPythonMessenger();

		void Destroy();

		// Friend
		void RemoveFriend(const char * c_szKey);
		void OnFriendLogin(const char * c_szKey);
		void OnFriendLogout(const char * c_szKey);
		void SetMobile(const char * c_szKey, BYTE byState);
		BOOL IsFriendByKey(const char * c_szKey);
		BOOL IsFriendByName(const char * c_szName);
		#ifdef ENABLE_MESSENGER_BLOCK
		void RemoveBlock(const char * c_szKey);
		void OnBlockLogin(const char * c_szKey);
		void OnBlockLogout(const char * c_szKey);
		BOOL IsBlockByKey(const char * c_szKey);
		BOOL IsBlockByName(const char * c_szName);
		#endif

		// Guild
		void AppendGuildMember(const char * c_szName);
		void RemoveGuildMember(const char * c_szName);
		void RemoveAllGuildMember();
		void LoginGuildMember(const char * c_szName);
		void LogoutGuildMember(const char * c_szName);
		void RefreshGuildMember();

		void SetMessengerHandler(PyObject* poHandler);

		// Team
		void OnTeamLogin(const char * c_szKey);
		void OnTeamLogout(const char * c_szKey);
		void RefreshTeamlerState();

	protected:
		TFriendNameMap m_FriendNameMap;
		TGuildMemberStateMap m_GuildMemberStateMap;
		TTeamNameMap			m_TeamNameMap;
		#ifdef ENABLE_MESSENGER_BLOCK
		TBlockNameMap m_BlockNameMap;
		#endif

	private:
		PyObject * m_poMessengerHandler;
};
