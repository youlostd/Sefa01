#pragma once

#include "StdAfx.h"

#ifdef ENABLE_AUCTION
#include "Packet.h"

class CPythonAuction : public CSingleton<CPythonAuction>
{
public:
	static constexpr size_t ITEMS_PER_PAGE = 100;
	static constexpr size_t DEFAULT_ITEM_DURATION = 60 * 60 * 24 * 3;
	static constexpr size_t SEARCH_TEXT_MAX_LEN = 35;
	static constexpr size_t ITEMS_MAX_PAGE = 10;

	static constexpr size_t SHOP_CREATE_COST = 100 * 1000;
	static constexpr size_t SHOP_SLOT_COUNT_X = 10;
	static constexpr size_t SHOP_SLOT_COUNT_Y = 8;
	static constexpr size_t SHOP_SLOT_COUNT = SHOP_SLOT_COUNT_X * SHOP_SLOT_COUNT_Y;

	static constexpr DWORD SHOP_MODEL_VNUMS[] = { 30000, 30002, 30003, 30004, 30005, 30006, 30007, 30008 };

	// taxes
#if defined(ELONIA) || defined(AELDRA)
	static constexpr size_t TAX_AUCTION_OWNER = 9 + 1;
	static constexpr size_t TAX_SHOP_OWNER = 3 + 1;
	static constexpr size_t TAX_SHOP_PLAYER = 0;
#else
	static constexpr size_t TAX_AUCTION_OWNER = 0;
	static constexpr size_t TAX_SHOP_OWNER = 0;
	static constexpr size_t TAX_SHOP_PLAYER = 0;
#endif

public:
	struct TAuctionItem
	{
		BYTE container_type;
		network::TShopItemTable shop_item;
	};
	using TAuctionItemPtr = std::shared_ptr<TAuctionItem>;

	enum EAuctionType
	{
		TYPE_AUCTION,
		TYPE_SHOP,
	};

	enum ESearchTypes
	{
		SEARCH_TYPE_NONE,
		SEARCH_TYPE_ITEM,
		SEARCH_TYPE_PLAYER,
		SEARCH_TYPE_ITEM_STRICT,
		SEARCH_TYPE_MAX_NUM,
	};

	enum ESearchOrders
	{
		SEARCH_ORDER_NAME,
		SEARCH_ORDER_PRICE,
		SEARCH_ORDER_DATE,
		SEARCH_ORDER_MAX_NUM,
	};

	enum EItemContainerTypes
	{
		ITEM_CONTAINER_SEARCH,
		ITEM_CONTAINER_SHOP,
		ITEM_CONTAINER_OWNED,
		ITEM_CONTAINER_OWNED_SHOP,
		ITEM_CONTAINER_MAX_NUM,
	};

public:
	CPythonAuction();
	~CPythonAuction();

	void		initialize();

	void		append_item(BYTE container_type, network::TShopItemTable&& shop_item);
	size_t		get_item_count(BYTE container_type) const;
	network::TShopItemTable* get_item(BYTE container_type, size_t index);
	TAuctionItemPtr find_item(BYTE container_type, DWORD item_id);
	TAuctionItemPtr find_item_by_cell(BYTE container_type, WORD cell);
	size_t		get_container_index_by_id(BYTE container_type, DWORD id);
	size_t		get_item_index(BYTE container_type, TAuctionItemPtr auction_item);
	bool		remove_item(BYTE container_type, DWORD item_id);
	void		remove_item_by_index(BYTE container_type, DWORD item_id);
	void		clear_items(BYTE container_type);

	void		append_shop_creating_item(::TItemPos inventory_pos, WORD display_pos, uint64_t price);
	const network::TShopItemTable*	get_shop_creating_item(::TItemPos inventory_pos) const;
	std::vector<network::TShopItemTable>	build_shop_creating_items();
	void		remove_shop_creating_item(::TItemPos inventory_pos);
	void		clear_shop_creating_items() { _shop_creating_items.clear(); }

	void		set_max_page_count(WORD count) noexcept { _max_page_count = count; }
	WORD		get_max_page_count() const noexcept { return _max_page_count; }

	void		set_owned_gold(uint64_t gold) noexcept { _owned_gold = gold; }
	uint64_t	get_owned_gold() const noexcept { return _owned_gold; }

	void		set_owned_shop(bool has_shop) noexcept { _shop_owned = has_shop; }
	bool		has_owned_shop() const noexcept { return _shop_owned; }

	void		load_owned_shop(const std::string& name, DWORD timeout, uint64_t gold) noexcept { _shop_owned_name = name; _shop_owned_timeout = timeout; _shop_owned_gold = gold; }
	bool		is_owned_shop_loaded() const noexcept { return !_shop_owned_name.empty(); }
	
	const std::string& get_owned_shop_name() const noexcept { return _shop_owned_name; }
	
	void		set_owned_shop_timeout(DWORD timeout) noexcept { _shop_owned_timeout = timeout; }
	DWORD		get_owned_shop_timeout() const noexcept { return _shop_owned_timeout; }
	
	void		set_owned_shop_gold(uint64_t gold) noexcept { _shop_owned_gold = gold; }
	uint64_t	get_owned_shop_gold() const noexcept { return _shop_owned_gold; }

	void		clear_owned_shop() noexcept { _shop_owned_name.clear(); }

	void		open_guest_shop(const std::string& name) noexcept { _shop_name = name; }
	bool		is_guest_shop_open() const noexcept { return !_shop_name.empty(); }
	const std::string& get_guest_shop_name() const noexcept { return _shop_name; }
	void		close_guest_shop() noexcept { _shop_name.clear(); }

	network::TDataAuctionSearch& get_search_options() noexcept { return *_search_settings.mutable_basic_data(); }
	network::TExtendedDataAuctionSearch& get_extended_search_options() noexcept { return _search_settings; }

	void		select_container_type(BYTE container_type) noexcept { _selected_container_type = container_type; }
	BYTE		get_selected_container_type() const noexcept { return _selected_container_type; }

	void		clear_shop_history() noexcept { _shop_history.clear(); }
	void		append_shop_history(network::TAuctionShopHistoryElement&& elem) noexcept { _shop_history.push_back(std::move(elem)); }
	size_t		get_shop_history_size() const noexcept { return _shop_history.size(); }
	const network::TAuctionShopHistoryElement* get_shop_history(size_t index) const noexcept { return index < _shop_history.size() ? &_shop_history[index] : nullptr; }

	static bool	is_shop_race(DWORD npc_race) noexcept
	{
		for (auto& vnum : SHOP_MODEL_VNUMS)
		{
			if (vnum == npc_race)
				return true;
		}

		return false;
	}

private:
	std::vector<TAuctionItemPtr>				_items[ITEM_CONTAINER_MAX_NUM];
	std::unordered_map<DWORD, TAuctionItemPtr>	_items_by_id[ITEM_CONTAINER_MAX_NUM];
	std::unordered_map<WORD, TAuctionItemPtr>	_items_by_slot[ITEM_CONTAINER_MAX_NUM];

	std::unordered_map<::TItemPos, network::TShopItemTable>	_shop_creating_items;
	
	WORD										_max_page_count;
	uint64_t									_owned_gold;

	bool										_shop_owned;
	std::string									_shop_owned_name;
	DWORD										_shop_owned_timeout;
	uint64_t									_shop_owned_gold;

	std::string									_shop_name;

	network::TExtendedDataAuctionSearch			_search_settings;

	BYTE										_selected_container_type;

	std::vector<network::TAuctionShopHistoryElement>	_shop_history;
};
#endif
