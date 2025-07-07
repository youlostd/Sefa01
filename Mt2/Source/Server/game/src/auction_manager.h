#pragma once

#include "../../common/service.h"

#ifdef AUCTION_SYSTEM
#include "item.h"
#include "p2p.h"

#include "../common/singleton.h"

#include "protobuf_gg_packets.h"
#include "protobuf_data_item.h"

class AuctionShopManager final : public singleton<AuctionShopManager>
{
public:
	void spawn_shop(const network::GGAuctionShopSpawnPacket& data);
	void despawn_shop(DWORD owner_id);

	bool click_shop(LPCHARACTER shop, LPCHARACTER causer);
	bool mark_shop(LPCHARACTER ch, DWORD owner_id);

	LPCHARACTER find_shop(DWORD owner_id) const { return __find_shop(owner_id); }

private:
	LPCHARACTER __find_shop(DWORD owner_id) const;
	DWORD __find_shop_owner(LPCHARACTER chr) const;

private:
	std::unordered_map<DWORD, LPCHARACTER> _shops_by_owner;
	std::unordered_map<LPCHARACTER, DWORD> _shops_by_chr;
};

class AuctionManager final : public singleton<AuctionManager>
{
public:
	static constexpr size_t ITEMS_PER_PAGE = 100;
	static constexpr size_t DEFAULT_ITEM_DURATION = 60 * 60 * 24 * 3;

	static constexpr size_t SEARCH_ALLOW_ANTI_FLAG = ITEM_ANTIFLAG_WARRIOR | ITEM_ANTIFLAG_ASSASSIN | ITEM_ANTIFLAG_SURA | ITEM_ANTIFLAG_SHAMAN;

	// shop settings
	static constexpr size_t SHOP_CREATE_COST = 100000;
	static constexpr size_t SHOP_RENEW_COST = 2500000;
		
	static constexpr size_t SHOP_SPAWN_VNUM = 9005;
	static constexpr size_t SHOP_SLOT_COUNT_X = 10;
	static constexpr size_t SHOP_SLOT_COUNT_Y = 8;
	static constexpr size_t SHOP_SLOT_COUNT = SHOP_SLOT_COUNT_X * SHOP_SLOT_COUNT_Y;
	static constexpr size_t SHOP_HISTORY_LIMIT = 100;
	static constexpr size_t AVERAGE_PRICE_REFRESH_TIME = 600;

	static constexpr size_t SHOP_MIN_DISTANCE_BETWEEN = 100;
	static constexpr float SHOP_OWN_CHAR_SCALING = 2.0f;

	// shop duration
#ifdef ELONIA
	static constexpr size_t SHOP_TIME_DEFAULT = 60 * 60 * 24 * 7;
	static constexpr size_t SHOP_TIME_PREMIUM = 60 * 60 * 24 * 7;
#else
	static constexpr size_t SHOP_TIME_DEFAULT = 60 * 60 * 24 * 2;
	static constexpr size_t SHOP_TIME_PREMIUM = 60 * 60 * 24 * 7;
#endif

	// different shop models / styles
	static constexpr DWORD SHOP_MODEL_VNUMS[] = { 30000, 30002, 30003, 30004, 30005, 30006, 30007, 30008 };
	static constexpr size_t SHOP_MODEL_MAX_NUM = 7;
	static constexpr size_t SHOP_STYLE_MAX_NUM = 6;

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

	enum ELogTypes {
		LOG_NONE,
		LOG_INSERT,
		LOG_CHECK_TAKE, // only for backwards compatibility
		LOG_TAKE,
		LOG_CHECK_BUY, // only for backwards compatibility
		LOG_BUY,
		LOG_SAVE_MONEY, // only for backwards compatibility
		LOG_GIVE_MONEY, // only for backwards compatibility
		LOG_RECEIVE_MONEY, // only for backwards compatibility
		LOG_SELL,
		LOG_TAKE_GOLD,
		LOG_RENEW,
		LOG_RENEW_FAILED,
	};

private:
	struct TAuctionPlayer;

	struct TAuctionItem
	{
		TAuctionItem() :
			owner(nullptr),
			proto(nullptr),
			random_id(0),
			real_timeout_event(nullptr)
		{ }

		TAuctionPlayer* owner;

		network::TShopItemTable shop_item;
		network::TItemTable* proto;

		DWORD random_id;

		LPEVENT real_timeout_event;

		std::string get_log_info() const
		{
			return shop_item.auction_type() == TYPE_AUCTION ? "AUCTION" : "SHOP";
		}

		std::string get_log_hint() const
		{
			return "auction_type " + std::to_string(shop_item.auction_type()) + " itemcount " + std::to_string(shop_item.item().count()) + " vnum " + std::to_string(shop_item.item().vnum());
		}

		DWORD get_timeout() const;
	};
	using TAuctionItemPtr = std::shared_ptr<TAuctionItem>;

	struct TAuctionShop
	{
		TAuctionShop() :
			owner(nullptr),
			vnum(0),
			style(0),
			color_red(0.0f), color_green(0.0f), color_blue(0.0f),
			channel(0),
			map_index(0),
			x(0), y(0),
			create_time(0),
			timeout(0),
			gold(0),
			timeout_event(0)
		{
			memset(item_grid, 0, sizeof(item_grid));
		}

		~TAuctionShop()
		{
			event_cancel(&timeout_event);
		}

		TAuctionPlayer* owner;

		DWORD vnum;
		DWORD style;
		std::string name;

		float color_red;
		float color_green;
		float color_blue;

		BYTE channel;
		DWORD map_index;
		DWORD x, y;

		DWORD create_time;
		DWORD timeout;

		long long gold;

		LPEVENT timeout_event;

		bool item_grid[SHOP_SLOT_COUNT];
		std::unordered_set<TAuctionItemPtr> items;

		std::unordered_set<LPDESC> spawned_descs;
		std::unordered_map<DWORD, LPDESC> guests;
	};

	struct TAuctionPlayer
	{
		TAuctionPlayer() :
			pid(0),
			gold(0),
			desc(nullptr),
			language(LANGUAGE_DEFAULT),
			sent_shop(false)
		{ }

		DWORD pid;
		std::string name;
		long long gold;
		LPDESC desc;
		BYTE language;
		bool sent_shop;

		std::unique_ptr<TAuctionShop> shop;
		std::unordered_set<TAuctionItemPtr> items;
	};

	class ItemNameString : public std::string
	{
	public:
		ItemNameString() : std::string() {}
		ItemNameString(const std::string& str) : std::string(str) {}
		ItemNameString(const char* str) : std::string(str) {}

		bool operator<(const ItemNameString& other) const
		{
			if (AuctionManager::instance()._items_by_name_compare_strict)
				return compare(other) < 0;
			else
				return compare(0, AuctionManager::instance()._items_by_name_compare_len, other, 0, AuctionManager::instance()._items_by_name_compare_len) < 0;
		}
	};

	using TSearchTypesMap = std::unordered_map<BYTE, std::unordered_set<BYTE>>;

	using TItemByNameMultiMap = std::multimap<ItemNameString, TAuctionItemPtr>;
	using TItemNameMapRange = std::pair<TItemByNameMultiMap::iterator, TItemByNameMultiMap::iterator>;

public:
	struct TAuctionItemEventInfo : event_info_data
	{
		TAuctionItemPtr auction_item;
	};

	struct TAuctionPlayerEventInfo : event_info_data
	{
		TAuctionPlayer* player;
	};

public:
	AuctionManager();
	~AuctionManager();

	void initialize();
	
	void insert_item(LPDESC owner_desc, network::TShopItemTable&& shop_item, BYTE channel, DWORD map_index);
	void take_item(LPDESC owner_desc, DWORD owner, DWORD item_id, WORD inventory_pos = 0, bool message = true);
	bool buy_item(LPDESC player_desc, DWORD pid, const std::string& player_name, DWORD item_id, uint64_t paid_gold);
	void take_gold(LPDESC owner_desc, DWORD owner, uint64_t gold);
	void search_items(LPDESC player_desc, DWORD pid, BYTE page, BYTE language, const network::TDataAuctionSearch& data);
	void search_items(LPDESC player_desc, DWORD pid, BYTE page, BYTE language, const network::TExtendedDataAuctionSearch& data, BYTE channel, DWORD map_index);

	void shop_request_show(DWORD pid);
	void shop_open(LPDESC player_desc, network::GGAuctionShopOpenPacket&& data);
	void shop_take_gold(LPDESC owner_desc, DWORD owner, uint64_t gold);
	void shop_view(LPDESC player_desc, DWORD pid, DWORD owner);
	void shop_view_cancel(DWORD pid);
	void shop_mark(LPDESC player_desc, DWORD pid, DWORD item_id);
	void shop_reset_timeout(LPDESC owner_desc, DWORD pid, DWORD new_timeout);
	void shop_close(LPDESC owner_desc, DWORD pid, BYTE channel, DWORD map_index);
	void shop_request_history(LPDESC player_desc, DWORD pid);
	void request_average_price(LPDESC player_desc, DWORD pid, DWORD requestor, DWORD vnum, DWORD count);

	void on_player_login(DWORD pid, CCI* p2p_player);
	void on_player_logout(DWORD pid);
	void on_player_delete(DWORD pid);

	void on_receive_map_location();
	void on_connect_peer(LPDESC peer);
	void on_disconnect_peer(LPDESC peer);

	void on_shop_timeout(TAuctionPlayer* player);
	void on_update_average_prices();
	void on_item_realtime_expire(TAuctionItemPtr auction_item);

private:
	TAuctionPlayer* __find_player(DWORD pid) const;
	TAuctionPlayer* __find_player(const std::string& name) const;
	TAuctionItemPtr __find_item(DWORD id) const;
	const std::vector<network::TAuctionShopHistoryElement>* __find_shop_history(DWORD pid) const;

	void __give_item_to_player(LPDESC player_desc, TAuctionItemPtr auction_item, DWORD target_pid = 0, WORD inventory_pos = 0);
	bool __can_buy(const TAuctionItemPtr& auction_item);
	void __compute_taxes(DWORD pid, const TAuctionItemPtr& auction_item, size_t* ret_owner_tax = nullptr, size_t* ret_player_tax = nullptr) const;
	
	TAuctionItemPtr __append_item(network::TShopItemTable&& shop_item, bool is_boot = false);
	void __remove_item(DWORD item_id);

	void __change_gold(TAuctionPlayer* player, int64_t amount);
	void __change_shop_gold(TAuctionPlayer* player, int64_t amount);

	// ITEM SEARCH
	TItemByNameMultiMap& __get_item_name_map(BYTE language, const std::string& name);
	TItemNameMapRange __find_items_by_name(BYTE language, const std::string& name, bool strict = false);

	TSearchTypesMap __build_search_types_map(const network::TDataAuctionSearch& options) const
	{
		TSearchTypesMap ret;
		for (auto& type_info : options.types())
		{
			auto& sub_types = ret[type_info.type()];
			for (auto& sub_type : type_info.sub_types())
				sub_types.insert(sub_type);
		}

		return ret;
	}
	bool __check_search_item(DWORD pid, const TAuctionItemPtr& auction_item, const network::TDataAuctionSearch& options, const TSearchTypesMap& types) const;
	bool __check_search_item(DWORD pid, const TAuctionItemPtr& auction_item, const network::TExtendedDataAuctionSearch& options, const TSearchTypesMap& types, BYTE channel, DWORD map_index) const;
	template <typename TResultSet>
	void __check_search_items(TItemNameMapRange&& range, TResultSet& result_set,
		DWORD pid, const network::TExtendedDataAuctionSearch& options, const TSearchTypesMap& types, BYTE channel, DWORD map_index) const
	{
		for (auto& it = range.first; it != range.second; ++it)
		{
			if (__check_search_item(pid, it->second, options, types, channel, map_index))
				result_set.insert(it->second);
		}
	}

	void __append_send_search_item(DWORD owner, network::GCOutputPacket<network::GCAuctionSearchResultPacket>& pack, const TAuctionItemPtr& item);
	// ITEM SEARCH END

	TAuctionPlayer* __append_player(DWORD pid, const std::string& name);
	TAuctionShop* __open_shop(TAuctionPlayer* player, const network::GGAuctionShopOpenPacket& data, DWORD create_time = 0, uint64_t gold = 0);
	network::TAuctionShopHistoryElement& __append_shop_history(DWORD owner, DWORD vnum, uint64_t price, const std::string& buyer_name, DWORD date = 0);
	void __check_remove_player(TAuctionPlayer* player);
	void __remove_player(TAuctionPlayer* player);
	void __check_hide_shop(TAuctionPlayer* player);
	void __hide_shop(TAuctionPlayer* player);
	void __check_remove_shop(TAuctionPlayer* player);

	bool __add_shop_guest(TAuctionPlayer* player, DWORD pid, LPDESC desc);
	void __remove_shop_guest(DWORD pid, bool close_packet = true);

	void __save_item(const network::TShopItemTable* shop_item) const;
	void __save_player(TAuctionPlayer* player) const;
	void __save_shop(TAuctionPlayer* player) const;
	void __remove_oldest_shop_history(DWORD pid) const;
	void __save_shop_history(DWORD pid, const network::TAuctionShopHistoryElement& elem) const;

	void __spawn_shop(TAuctionPlayer* player);
	void __spawn_shop(TAuctionPlayer* player, LPDESC peer);
	void __despawn_shop(TAuctionPlayer* player);
	void __start_shop_timeout(TAuctionPlayer* player);
	bool __reset_shop_timeout(TAuctionPlayer* player, DWORD new_timeout);

	uint64_t __get_average_price(DWORD vnum) const;

	void __send_player_message(LPDESC peer, DWORD pid, const std::string& message) const;
	void __send_player_gold(TAuctionPlayer* player) const;
	void __send_player_item(TAuctionPlayer* player, const network::TShopItemTable* item, bool is_remove = false) const;
	void __send_player_shop_owned(TAuctionPlayer* player) const;
	void __send_player_shop(TAuctionPlayer* player) const;
	void __send_player_shop_gold(TAuctionPlayer* player) const;
	void __send_player_shop_item(TAuctionPlayer* player, const network::TShopItemTable* item, bool is_remove = false) const;

private:
	std::unordered_map<DWORD, std::unique_ptr<TAuctionPlayer>> _players;
	std::unordered_map<std::string, TAuctionPlayer*> _players_by_name;

	using TItemsByIDMap = std::unordered_map<DWORD, TAuctionItemPtr>;
	TItemsByIDMap _items;
	std::unordered_map<BYTE, std::unordered_map<DWORD, TItemsByIDMap>> _items_by_map;

	static constexpr size_t ALPHABET_CHAR_COUNT = 'z' - 'a' + 1;
	TItemByNameMultiMap _items_by_name[LANGUAGE_MAX_NUM][ALPHABET_CHAR_COUNT + 1];
	bool _items_by_name_compare_strict;
	size_t _items_by_name_compare_len;

	std::unordered_map<DWORD, DWORD> _shop_guests;

	std::unordered_map<DWORD, std::vector<network::TAuctionShopHistoryElement>> _shop_histories;

	LPEVENT _update_average_prices_event;
	std::unordered_map<DWORD, uint64_t> _average_prices;
};
#endif
