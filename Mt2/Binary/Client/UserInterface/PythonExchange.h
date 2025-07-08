#pragma once

#include "Packet.h"

/*
 *	��ȯ â ����
 */
class CPythonExchange : public CSingleton<CPythonExchange>
{
	public:
		enum
		{
			EXCHANGE_ITEM_MAX_NUM = 24,
		};

		typedef struct trade
		{
			char					name[CHARACTER_NAME_MAX_LEN + 1];

			network::TItemData		item[EXCHANGE_ITEM_MAX_NUM];

			BYTE					accept;
			long long				elk;
		} TExchangeData;

	public:
		CPythonExchange();
		virtual ~CPythonExchange();

		void			Clear();

		void			Start();
		void			End();
		bool			isTrading();

		// Interface

		void			SetSelfName(const char *name);
		void			SetTargetName(const char *name);

		char			*GetNameFromSelf();
		char			*GetNameFromTarget();

		void			SetElkToTarget(long long elk);
		void			SetElkToSelf(long long elk);

		long long		GetElkFromTarget();
		long long		GetElkFromSelf();

		void			SetItemToTarget(DWORD pos, const network::TItemData& item);
		void			SetItemToSelf(DWORD pos, const network::TItemData& item);

		void			DelItemOfTarget(BYTE pos);
		void			DelItemOfSelf(BYTE pos);

		const network::TItemData* GetItemFromTarget(BYTE pos);
		const network::TItemData* GetItemFromSelf(BYTE pos);

		void			SetAcceptToTarget(BYTE Accept);
		void			SetAcceptToSelf(BYTE Accept);

		bool			GetAcceptFromTarget();
		bool			GetAcceptFromSelf();

		bool			GetElkMode();
		void			SetElkMode(bool value);

	protected:
		bool				m_isTrading;

		bool				m_elk_mode;   // ��ũ�� Ŭ���ؼ� ��ȯ�������� ���� ������.
		TExchangeData		m_self;
		TExchangeData		m_victim;
};
