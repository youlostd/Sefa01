#ifndef __INC_MESSENGER_MANAGER_H
#define __INC_MESSENGER_MANAGER_H

#include "db.h"

class MessengerManager : public singleton<MessengerManager>
{
	public:
		typedef std::string keyT;
		#ifdef ENABLE_MESSENGER_BLOCK
		typedef std::string keyBL;
		#endif
		typedef const std::string & keyA;

		MessengerManager();
		virtual ~MessengerManager();

	public:
		void	P2PLogin(keyA account);
		void	P2PLogout(keyA account);

		void	Login(keyA account, bool p2p = false);
		void	Logout(keyA account);

		void	RequestToAdd(LPCHARACTER ch, const char* pszTargetName);
		void	RequestToAdd(const char* pszName, LPCHARACTER target);
		void	RequestToAdd(LPCHARACTER ch, LPCHARACTER target);
		void	AuthToAdd(keyA account, keyA companion, bool bDeny);

		void	__AddToList(keyA account, keyA companion);	// ½ÇÁ¦ m_Relation, m_InverseRelation ¼öÁ¤ÇÏ´Â ¸Þ¼Òµå
		void	AddToList(keyA account, keyA companion);

		void	__RemoveFromList(keyA account, keyA companion); // ½ÇÁ¦ m_Relation, m_InverseRelation ¼öÁ¤ÇÏ´Â ¸Þ¼Òµå
		void	RemoveFromList(keyA account, keyA companion);	
		#ifdef ENABLE_MESSENGER_BLOCK
		void	__AddToBlockList(keyA account, keyA companion);
		void	AddToBlockList(keyA account, keyA companion);
		bool	CheckMessengerList(std::string ch, std::string tch, BYTE type);
		bool	IsInBlockList(MessengerManager::keyA account, MessengerManager::keyA companion);
		void	__RemoveFromBlockList(keyA account, keyA companion);
		void	RemoveFromBlockList(keyA account, keyA companion);
		void	RemoveAllBlockList(keyA account);
		#endif
		

		void	RemoveAllList(keyA account);

		bool	IsInList(keyA account, keyA companion);

		void	Initialize();
		void	LoadList(SQLMsg * pmsg);

	private:
		void	SendList(keyA account);
		void	SendLogin(keyA account, keyA companion);
		void	SendLogout(keyA account, keyA companion);

		#ifdef ENABLE_MESSENGER_BLOCK
		void	SendBlockLogin(keyA account, keyA companion);
		void	SendBlockLogout(keyA account, keyA companion);
		void	LoadBlockList(SQLMsg * pmsg);
		void	SendBlockList(keyA account);
		#endif
		

		void	Destroy();

		std::set<keyT>			m_set_loginAccount;
		std::map<keyT, std::set<keyT> >	m_Relation;
		std::map<keyT, std::set<keyT> >	m_InverseRelation;
		std::set<DWORD>			m_set_requestToAdd;
		#ifdef ENABLE_MESSENGER_BLOCK
		std::map<keyT, std::set<keyT> >	m_BlockRelation;
		std::map<keyT, std::set<keyT> >	m_InverseBlockRelation;
		std::set<DWORD>			m_set_requestToBlockAdd;
		#endif
};

#endif
