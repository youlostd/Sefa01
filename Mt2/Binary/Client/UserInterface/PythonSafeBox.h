#pragma once

class CPythonSafeBox : public CSingleton<CPythonSafeBox>
{
	public:
		enum
		{
			SAFEBOX_SLOT_X_COUNT = 5,
			SAFEBOX_SLOT_Y_COUNT = 10,
			SAFEBOX_PAGE_SIZE = SAFEBOX_SLOT_X_COUNT * SAFEBOX_SLOT_Y_COUNT,
		};
		typedef std::vector<network::TItemData> TItemInstanceVector;

	public:
		CPythonSafeBox();
		virtual ~CPythonSafeBox();

		void OpenSafeBox(int iSize);
		void SetItemData(DWORD dwSlotIndex, const network::TItemData & rItemData);
		void DelItemData(DWORD dwSlotIndex);

		void SetMoney(DWORD dwMoney);
		DWORD GetMoney();
		
		BOOL GetSlotItemID(DWORD dwSlotIndex, DWORD* pdwItemID);

		int GetCurrentSafeBoxSize();
		BOOL GetItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance);

		// MALL
		void OpenMall(int iSize);
		void SetMallItemData(DWORD dwSlotIndex, const network::TItemData & rItemData);
		void DelMallItemData(DWORD dwSlotIndex);
		BOOL GetMallItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance);
		BOOL GetSlotMallItemID(DWORD dwSlotIndex, DWORD * pdwItemID);
		DWORD GetMallSize();

#ifdef ENABLE_GUILD_SAFEBOX
		// GUILD
		void SetGuildEnable(bool bIsEnabled);
		bool IsGuildEnabled() { return m_bGuildSafeboxEnabled; }
		void OpenGuild(int iSize);
		void SetGuildItemData(DWORD dwSlotIndex, const network::TItemData & rItemData);
		void DelGuildItemData(DWORD dwSlotIndex);
		BOOL GetGuildItemDataPtr(DWORD dwSlotIndex, network::TItemData ** ppInstance);
		BOOL GetSlotGuildItemID(DWORD dwSlotIndex, DWORD * pdwItemID);
		void SetGuildMoney(ULONGLONG ullMoney);
		ULONGLONG GetGuildMoney();
		DWORD GetGuildSize();

		void ClearGuildLogInfo();
		void AddGuildLogInfo(const network::TGuildSafeboxLogTable& rkLogInfo);
		void SortGuildLogInfo();
		DWORD GetGuildLogInfoCount() const;
		network::TGuildSafeboxLogTable* GetGuildLogInfo(DWORD dwIndex);
#endif

	protected:
		TItemInstanceVector m_ItemInstanceVector;
		TItemInstanceVector m_MallItemInstanceVector;
#ifdef ENABLE_GUILD_SAFEBOX
		bool m_bGuildSafeboxEnabled;
		TItemInstanceVector	m_GuildItemInstanceVector;
		ULONGLONG m_ullGuildMoney;
		std::vector<network::TGuildSafeboxLogTable> m_GuildLogInfo;
#endif
		DWORD m_dwMoney;
};