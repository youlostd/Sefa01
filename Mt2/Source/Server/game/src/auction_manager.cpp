#include "stdafx.h"

#ifdef AUCTION_SYSTEM
#include "auction_manager.h"
#include "item_manager.h"
#include "config.h"
#include "desc_client.h"
#include "db.h"
#include "desc.h"
#include "p2p.h"
#include "log.h"
#include "char_manager.h"
#include "char.h"
#include "target.h"
#include "map_location.h"
#include "sectree_manager.h"
#include "../../common/VnumHelper.h"
#include "../../common/limited_vector.h"

#include <numeric>
#include <algorithm>

#include <boost/range/adaptor/map.hpp>

/**********************************\
 ** AuctionShopManager CLASS
 ** Handles the spawning / despawning of shops on the different cores.
\**********************************/

void AuctionShopManager::spawn_shop(const network::GGAuctionShopSpawnPacket& data)
{
	if (test_server)
		sys_log(0, "AuctionShopManager::spawn_shop %s vnum %u at map %u on %ux%u", data.name().c_str(), data.vnum(), data.map_index(), data.x(), data.y());

	// despawn possible old shop
	despawn_shop(data.owner_id());

	// compute x/Y
	PIXEL_POSITION basePos;
	if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(data.map_index(), basePos))
		return;

	auto x = data.x() + basePos.x;
	auto y = data.y() + basePos.y;

	// spawn new shop
	auto chr = CHARACTER_MANAGER::instance().SpawnMob(AuctionManager::SHOP_SPAWN_VNUM, data.map_index(), x, y, 0, false, -1, false);
	if (!chr)
		return;

	chr->set_auction_shop_owner(data.owner_id());

	chr->SetName(data.owner_name() + "'s Shop");

	chr->SetPolymorph(data.vnum(), true);
	chr->SetAuctionShop(data.name(), data.style(), data.color_red(), data.color_green(), data.color_blue());

	chr->SetNoMove();

	if (!chr->Show(data.map_index(), x, y))
	{
		M2_DESTROY_CHARACTER(chr);
		return;
	}

	_shops_by_owner[data.owner_id()] = chr;
	_shops_by_chr[chr] = data.owner_id();

	chr->add_event(CHARACTER::EEventTypes::DESTROY, [this, owner = data.owner_id()](LPCHARACTER ch) {
		_shops_by_owner.erase(owner);
		_shops_by_chr.erase(ch);
	});
}

void AuctionShopManager::despawn_shop(DWORD owner_id)
{
	auto chr = __find_shop(owner_id);
	if (chr)
		M2_DESTROY_CHARACTER(chr);
}

bool AuctionShopManager::click_shop(LPCHARACTER shop, LPCHARACTER causer)
{
	if (DWORD owner_id = __find_shop_owner(shop))
	{
		network::GGOutputPacket<network::GGAuctionShopViewPacket> pack;
		pack->set_player_id(causer->GetPlayerID());
		pack->set_owner_id(owner_id);
		P2P_MANAGER::instance().SendProcessorCore(pack);

		return true;
	}

	return false;
}

bool AuctionShopManager::mark_shop(LPCHARACTER ch, DWORD owner_id)
{
	auto shop = __find_shop(owner_id);
	if (!shop)
		return false;

	if (shop->GetMapIndex() != ch->GetMapIndex())
		return false;

	CTargetManager::instance().CreateTarget(ch->GetPlayerID(),
		random_number(1, 999), // Hotfix no shop arrow after few clicks... [todo]
		"TARGET_TYPE_POS",
		TARGET_TYPE_POS,
		shop->GetX(),
		shop->GetY(),
		shop->GetMapIndex(),
		"Shop");

	return true;
}

DWORD AuctionShopManager::__find_shop_owner(LPCHARACTER chr) const
{
	auto it = _shops_by_chr.find(chr);
	if (it == _shops_by_chr.end())
		return 0;

	return it->second;
}

LPCHARACTER AuctionShopManager::__find_shop(DWORD owner_id) const
{
	auto it = _shops_by_owner.find(owner_id);
	if (it == _shops_by_owner.end())
		return nullptr;

	return it->second;
}

/**********************************\
 ** AuctionManager CLASS
 ** Controls all input/output of items / shops / gold of the auction house / auction shops.
\**********************************/

constexpr DWORD AuctionManager::SHOP_MODEL_VNUMS[];

DWORD AuctionManager::TAuctionItem::get_timeout() const
{
	if (shop_item.auction_type() == TYPE_AUCTION)
		return shop_item.timeout_time();
	else if (shop_item.auction_type() == TYPE_SHOP)
		return owner->shop->timeout;

	return 0;
}

EVENTFUNC(auction_shop_timeout_event)
{
	auto info = dynamic_cast<AuctionManager::TAuctionPlayerEventInfo*>(event->info);

	if (info == nullptr)
	{
		sys_err("auction_shop_timeout_event <Factor> Null pointer");
		return 0;
	}

	info->player->shop->timeout_event = nullptr;
	AuctionManager::instance().on_shop_timeout(info->player);

	return 0;
}

EVENTFUNC(auction_update_average_prices_event)
{
	AuctionManager::instance().on_update_average_prices();
	return PASSES_PER_SEC(AuctionManager::AVERAGE_PRICE_REFRESH_TIME);
}

EVENTFUNC(auction_item_realtime_timeout_event)
{
	auto info = dynamic_cast<AuctionManager::TAuctionItemEventInfo*>(event->info);

	if (info == nullptr)
	{
		sys_err("auction_item_realtime_timeout_event <Factor> Null pointer");
		return 0;
	}

	AuctionManager::instance().on_item_realtime_expire(info->auction_item);
	return 0;
}

AuctionManager::AuctionManager() :
	_items_by_name_compare_strict(true),
	_items_by_name_compare_len(0),
	_update_average_prices_event(nullptr)
{
}

AuctionManager::~AuctionManager()
{
	event_cancel(&_update_average_prices_event);
}

void AuctionManager::initialize()
{
	sys_log(0, "AuctionManager::initialize");

	if (!g_isProcessorCore)
	{
		sys_err("!!! AuctionManager may only be initialized on the processor core !!!");
		exit(1);
	}

	// load players
	std::unique_ptr<SQLMsg> player_msg(DBManager::instance().DirectQuery("SELECT id, name, auction_gold FROM player WHERE auction_gold > 0"));
	while (auto row = mysql_fetch_row(player_msg->Get()->pSQLResult))
	{
		size_t col = 0;

		DWORD pid = std::stoll(row[col++]);
		std::string name = row[col++];
		auto gold = std::stoll(row[col++]);

		auto player = __append_player(pid, name);
		player->gold = gold;

		sys_log(0, "AuctionManager::initialize: loaded gold %lld for pid %u", pid, static_cast<long long>(gold));
	}

	// load shops
	std::unique_ptr<SQLMsg> shop_msg(DBManager::instance().DirectQuery("SELECT s.pid, p.name, s.vnum, s.style, s.name, s.color_red, s.color_green, s.color_blue, s.channel, s.map_index, s.x, s.y, "
		"UNIX_TIMESTAMP(s.create_time), UNIX_TIMESTAMP(s.timeout), s.gold FROM auction_shop AS s "
		"INNER JOIN player AS p ON p.id = s.pid"));
	while (auto row = mysql_fetch_row(shop_msg->Get()->pSQLResult))
	{
		size_t col = 0;

		DWORD pid = std::stoll(row[col++]);
		std::string name = row[col++];

		auto player = __append_player(pid, name);

		network::GGAuctionShopOpenPacket data;
		data.set_vnum(std::stoll(row[col++]));
		data.set_style(std::stoll(row[col++]));
		data.set_name(row[col++]);
		data.set_color_red(std::stof(row[col++]));
		data.set_color_green(std::stof(row[col++]));
		data.set_color_blue(std::stof(row[col++]));
		data.set_channel(std::stoll(row[col++]));
		data.set_map_index(std::stoll(row[col++]));
		data.set_x(std::stoll(row[col++]));
		data.set_y(std::stoll(row[col++]));

		DWORD create_time = std::stoll(row[col++]);
		data.set_timeout(std::stoll(row[col++]));

		uint64_t gold = std::stoll(row[col++]);

		__open_shop(player, data, create_time, gold);

		sys_log(0, "AuctionManager::initialize: loaded shop for pid %u", pid);
	}

	// load items
	std::ostringstream query;
	query << "SELECT i.id, i.owner_id, i.vnum, i.count, i.window+0, i.pos, i.is_gm_owner, ";
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		query << "i.socket" << i << ", ";
	for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		query << "i.attrtype" << i << ", i.attrvalue" << i << ", ";
	query << "a.price, UNIX_TIMESTAMP(a.insertion_time), UNIX_TIMESTAMP(a.timeout) "
		"FROM item AS i "
		"INNER JOIN auction_item AS a ON a.item_id = i.id "
		"WHERE i.window IN (" << static_cast<int32_t>(AUCTION) << ", " << static_cast<int32_t>(AUCTION_SHOP) << ")";

	std::unique_ptr<SQLMsg> item_msg(DBManager::instance().DirectQuery("%s", query.str().c_str()));
	while (auto row = mysql_fetch_row(item_msg->Get()->pSQLResult))
	{
		size_t col = 0;

		network::TShopItemTable shop_item;
		auto item = shop_item.mutable_item();

		item->set_id(std::stoll(row[col++]));
		item->set_owner(std::stoll(row[col++]));
		item->set_vnum(std::stoll(row[col++]));
		item->set_count(std::stoll(row[col++]));
		item->mutable_cell()->set_window_type(std::stoll(row[col++]));
		item->mutable_cell()->set_cell(std::stoll(row[col++]));
		item->set_is_gm_owner(std::stoi(row[col++]));
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			item->add_sockets(std::stoll(row[col++]));
		for (int i = 0; i < ITEM_ATTRIBUTE_MAX_NUM; ++i)
		{
			auto attr = item->add_attributes();
			attr->set_type(std::stoi(row[col++]));
			attr->set_value(std::stoi(row[col++]));
		}

		item->set_price(std::stoll(row[col++]));
		shop_item.set_insertion_time(std::stoll(row[col++]));
		shop_item.set_timeout_time(std::stoll(row[col++]));

		if (item->cell().window_type() == AUCTION_SHOP)
			shop_item.set_auction_type(TYPE_SHOP);
		else
			shop_item.set_auction_type(TYPE_AUCTION);

		__append_item(std::move(shop_item), true);
	}

	// load shop history
	std::unique_ptr<SQLMsg> shop_history_msg(DBManager::instance().DirectQuery("SELECT pid, vnum, price, buyer, UNIX_TIMESTAMP(date) FROM auction_shop_history ORDER BY date LIMIT %u", SHOP_HISTORY_LIMIT));
	while (auto row = mysql_fetch_row(item_msg->Get()->pSQLResult))
	{
		size_t col = 0;

		DWORD pid = std::stoll(row[col++]);
		DWORD vnum = std::stoll(row[col++]);
		uint64_t price = std::stoll(row[col++]);
		std::string buyer = row[col++];
		DWORD date = std::stoll(row[col++]);

		__append_shop_history(pid, vnum, price, buyer, date);
	}

	// start update average prices event and execute once
	auto avg_price_event_info = AllocEventInfo<event_info_data>();
	_update_average_prices_event = event_create(auction_update_average_prices_event, AllocEventInfo<event_info_data>(), PASSES_PER_SEC(AVERAGE_PRICE_REFRESH_TIME));

	on_update_average_prices();
}

void AuctionManager::insert_item(LPDESC owner_desc, network::TShopItemTable&& shop_item, BYTE channel, DWORD map_index)
{
	if (test_server)
		sys_log(0, "AuctionManager::insert_item");

	network::GGOutputPacket<network::GGGiveItemPacket> pack_recover;
	*pack_recover->mutable_item() = shop_item.item();
	pack_recover->mutable_item()->mutable_cell()->set_cell(0);
	pack_recover->set_no_refund(true);

	if (shop_item.auction_type() == TYPE_SHOP)
	{
		auto player = __find_player(shop_item.item().owner());
		if (!player || !player->shop)
			return;

		if ((player->shop->map_index != 0 && map_index != 0 && player->shop->map_index != map_index) ||
			(player->shop->channel != 0 && channel != 0 && player->shop->channel != channel))
		{
			network::GCOutputPacket<network::GCChatPacket> pack;
			pack->set_type(CHAT_TYPE_INFO);
			pack->set_message(LC_TEXT(player->language, "You need to be on the same map as your shop."));

			owner_desc->SetRelay(player->pid);
			owner_desc->Packet(pack);

			owner_desc->Packet(pack_recover);
			return;
		}
	}

	auto owner = shop_item.item().owner();
	auto item_vnum = shop_item.item().vnum();
	auto item_id = shop_item.item().id();
	auto price = shop_item.item().price();

	shop_item.set_insertion_time(get_global_time());
	shop_item.set_timeout_time(get_global_time() + DEFAULT_ITEM_DURATION);

	if (auto auction_item = __append_item(std::move(shop_item)))
	{
		// log
		auto& item = auction_item->shop_item.item();
		LogManager::instance().AuctionLog(LOG_INSERT, auction_item->get_log_info(), item.owner(), item.owner(), item_id, item.vnum(), auction_item->get_log_hint());

		// save & send item
		__save_item(&auction_item->shop_item);

		auto player = __find_player(auction_item->shop_item.item().owner());
		__send_player_item(player, &auction_item->shop_item);
		__send_player_shop_item(player, &auction_item->shop_item);

		__send_player_message(player->desc, player->pid, "ITEM_INSERT_SUCCESS");
	}
	else
	{
		LogManager::instance().CharLog(owner, item_vnum, 0, item_id, "SHOP_INSERT_FAIL", ("price " + std::to_string(price)).c_str(), "unknown");
		owner_desc->Packet(pack_recover);
	}
}

void AuctionManager::take_item(LPDESC owner_desc, DWORD owner, DWORD item_id, WORD inventory_pos, bool message)
{
	if (test_server)
		sys_log(0, "AuctionManager::take_item %u item %u", owner, item_id);

	auto auction_item = __find_item(item_id);
	if (!auction_item)
	{
		__send_player_message(owner_desc, owner, "ITEM_TAKE_NOT_EXIST");
		return;
	}

	auto& item = auction_item->shop_item.item();
	if (item.owner() != owner)
	{
		sys_err("AuctionManager: try to take item %u from foreign owner, probably hacker : %u!!", item_id, owner);
		return;
	}

	// log
	LogManager::instance().AuctionLog(LOG_TAKE, auction_item->get_log_info(), owner, item.owner(), item_id, item.vnum(), auction_item->get_log_hint());

	// take process
	__give_item_to_player(owner_desc, auction_item, owner, inventory_pos);
	if (message)
		__send_player_message(owner_desc, owner, "ITEM_TAKE_SUCCESS");
}

bool AuctionManager::buy_item(LPDESC player_desc, DWORD pid, const std::string& player_name, DWORD item_id, uint64_t paid_gold)
{
	if (test_server)
		sys_log(0, "AuctionManager::buy_item %u item %u", pid, item_id);

	// general checks
	auto auction_item = __find_item(item_id);
	if (!auction_item)
	{
		__send_player_message(player_desc, pid, "ITEM_BUY_NOT_EXIST");
		return false;
	}

	auto shop_item = &auction_item->shop_item;
	if (shop_item->item().owner() == pid)
	{
		take_item(player_desc, pid, item_id);
		return false;
	}

	if (!__can_buy(auction_item))
	{
		__send_player_message(player_desc, pid, "ITEM_BUY_TIMEOUT");
		return false;
	}

	// compute price + owner gold with taxes
	auto price = shop_item->item().price();
	auto owner_gold = price;

	size_t player_tax = 0, owner_tax = 0;
	__compute_taxes(pid, auction_item, &owner_tax, &player_tax);

	if (player_tax > 0)
		price = price * (100 + player_tax) / 100;
	if (owner_tax > 0)
		owner_gold = owner_gold * (100 - owner_tax) / 100;

	if (paid_gold != price)
	{
		__send_player_message(player_desc, pid, "ITEM_WRONG_PRICE");
		return false;
	}

	auto player = __find_player(shop_item->item().owner());
	if (!player)
		return false;

	// log
	std::string hint = "cost " + std::to_string(paid_gold) + " owner get " + std::to_string(owner_gold) + " " + auction_item->get_log_hint();
	LogManager::instance().AuctionLog(LOG_BUY, auction_item->get_log_info(), pid, shop_item->item().owner(), item_id, shop_item->item().vnum(), hint);
	LogManager::instance().AuctionLog(LOG_SELL, auction_item->get_log_info(), shop_item->item().owner(), pid, item_id, shop_item->item().vnum(), hint);

	// history log
	if (shop_item->item().cell().window_type() == AUCTION_SHOP)
	{
		auto& shop_history = __append_shop_history(player->pid, shop_item->item().vnum(), shop_item->item().price(), player_name);
		__save_shop_history(player->pid, shop_history);
	}

	// buy process
	if (shop_item->item().cell().window_type() == AUCTION_SHOP)
		__change_shop_gold(player, owner_gold);
	else
		__change_gold(player, owner_gold);

	__give_item_to_player(player_desc, auction_item, pid);
	__send_player_message(player_desc, pid, "ITEM_BUY_SUCCESS");

	return true;
}

void AuctionManager::take_gold(LPDESC owner_desc, DWORD owner, uint64_t gold)
{
	if (test_server)
		sys_log(0, "AuctionManager::take_gold %u gold %lld", owner, static_cast<long long>(gold));

	auto player = __find_player(owner);
	if (!player)
		return;

	if (gold > player->gold)
		return;

	if (gold == 0)
		gold = player->gold;
	if (!gold)
		return;

	// log
	LogManager::instance().AuctionLog(LOG_TAKE_GOLD, "AUCTION", owner, owner, 0, 0, "amount " + std::to_string(gold) + " new storage " + std::to_string(player->gold - static_cast<int64_t>(gold)));;

	// take gold process
	__change_gold(player, -static_cast<int64_t>(gold));

	network::GGOutputPacket<network::GGGiveGoldPacket> pack;
	pack->set_pid(owner);
	pack->set_gold(gold);
	owner_desc->Packet(pack);
}

void AuctionManager::search_items(LPDESC player_desc, DWORD pid, BYTE page, BYTE language, const network::TDataAuctionSearch& data)
{
	if (test_server)
		sys_log(0, "AuctionManager::search_item %u lang %u name %s", pid, language, data.search_text().c_str());
	
	if ((data.include_anti_flag() & (~SEARCH_ALLOW_ANTI_FLAG)) != 0)
		return;

	size_t counter = 0;
	int current_page = 0;
	static LimitedVector<TAuctionItemPtr> items(ITEMS_PER_PAGE);

	if (!data.search_text().empty())
	{
		if (language >= LANGUAGE_MAX_NUM)
			language = LANGUAGE_DEFAULT;

		auto range = __find_items_by_name(language, data.search_text());

		for (auto it = range.first; it != range.second; ++it)
		{
			items.add() = it->second;

			if (++counter >= ITEMS_PER_PAGE)
			{
				counter = 0;
				if (current_page + 1 == page)
					break;

				current_page++;
			}
		}
	}
	else
	{
		auto types = __build_search_types_map(data);

		for (auto& auction_item : _items | boost::adaptors::map_values)
		{
			if (!__check_search_item(pid, auction_item, data, types))
				continue;

			items.add() = auction_item;
			
			if (++counter >= ITEMS_PER_PAGE)
			{
				counter = 0;
				if (current_page + 1 == page)
					break;

				current_page++;
			}
		}
	}

	if (items.get_count())
	{
		__send_player_message(player_desc, pid, "NO_ITEMS_FOUND");
		return;
	}

	network::GCOutputPacket<network::GCAuctionSearchResultPacket> pack;
	pack->set_page(current_page);
	pack->set_max_page(-1);
	
	for (size_t i = 0; i < items.get_count(); ++i)
		__append_send_search_item(pid, pack, items[i]);

	player_desc->SetRelay(pid);
	player_desc->Packet(pack);
}

void AuctionManager::search_items(LPDESC player_desc, DWORD pid, BYTE page, BYTE language, const network::TExtendedDataAuctionSearch& data, BYTE channel, DWORD map_index)
{
	if (test_server)
		sys_log(0, "AuctionManager::search_item EXTENDED %u lang %u type %d name %s page %u", pid, language, data.search_type(), data.basic_data().search_text().c_str(), page);

	if (data.search_type() >= SEARCH_TYPE_MAX_NUM)
		return;

	auto search_type = data.search_type();
	if (data.basic_data().search_text().empty())
		search_type = SEARCH_TYPE_NONE;

	if ((data.basic_data().include_anti_flag() & (~SEARCH_ALLOW_ANTI_FLAG)) != 0)
	{
		sys_err("invalid anti flag for auction shop search: %u by pid %u", data.basic_data().include_anti_flag(), pid);
		return;
	}

	// check order ids and prepare sort_order_reversed for later use
	BYTE recv_sort_order[SEARCH_ORDER_MAX_NUM];
	recv_sort_order[0] = data.sort_order1();
	recv_sort_order[1] = data.sort_order2();
	recv_sort_order[2] = data.sort_order3();

	bool recv_sort_order_reversed[SEARCH_ORDER_MAX_NUM];
	recv_sort_order_reversed[0] = data.sort_order1_reversed();
	recv_sort_order_reversed[1] = data.sort_order2_reversed();
	recv_sort_order_reversed[2] = data.sort_order3_reversed();

	bool sort_order_reversed[SEARCH_ORDER_MAX_NUM];

	std::set<BYTE> used_order_type;
	for (int i = 0; i < SEARCH_ORDER_MAX_NUM; ++i)
	{
		if (recv_sort_order[i] >= SEARCH_ORDER_MAX_NUM)
		{
			sys_err("invalid sort_order type %u at index %u for auction shop search by pid %u", recv_sort_order[i], i, pid);
			return;
		}

		if (used_order_type.find(recv_sort_order[i]) != used_order_type.end())
		{
			sys_err("already sort_order type %u at index %u for auction shop search by pid %u", recv_sort_order[i], i, pid);
			return;
		}

		used_order_type.insert(recv_sort_order[i]);
		sort_order_reversed[recv_sort_order[i]] = recv_sort_order_reversed[i];
	}

	// collect types map
	auto types = __build_search_types_map(data.basic_data());

	// prepare order functions
	static const auto order_func_shuffle = [](const TAuctionItemPtr& a, const TAuctionItemPtr& b) {
		return a->random_id < b->random_id;
	};
	auto order_func_by_name = [reversed = sort_order_reversed[SEARCH_ORDER_NAME], language](const TAuctionItemPtr& a, const TAuctionItemPtr& b) {
		bool ret = a->proto->locale_name(language) < b->proto->locale_name(language);
		return reversed ? !ret : ret;
	};
	auto order_func_by_price = [reversed = sort_order_reversed[SEARCH_ORDER_PRICE], single_price = data.is_single_price_order()](const TAuctionItemPtr& a, const TAuctionItemPtr& b) {
		bool ret;

		int64_t a_price = a->shop_item.item().price();
		int64_t b_price = b->shop_item.item().price();

		if (single_price)
		{
			auto& a_item = a->shop_item.item();
			auto& b_item = b->shop_item.item();

			if (a_item.count() > 0 && b_item.count() > 0)
			{
				a_price = a_price / a_item.count();
				b_price = b_price / b_item.count();
			}
		}

		ret = a_price < b_price;
		return reversed ? !ret : ret;
	};
	auto order_func_by_date = [reversed = sort_order_reversed[SEARCH_ORDER_DATE]](const TAuctionItemPtr& a, const TAuctionItemPtr& b) {
		bool ret = a->shop_item.insertion_time() < b->shop_item.insertion_time();
		return reversed ? !ret : ret;
	};

	std::function<bool(const TAuctionItemPtr & a, const TAuctionItemPtr & b)> order_func_list[SEARCH_ORDER_MAX_NUM], ordered_order_func_list[SEARCH_ORDER_MAX_NUM];
	order_func_list[SEARCH_ORDER_NAME] = order_func_by_name;
	order_func_list[SEARCH_ORDER_PRICE] = order_func_by_price;
	order_func_list[SEARCH_ORDER_DATE] = order_func_by_date;

	for (size_t i = 0; i < SEARCH_ORDER_MAX_NUM; ++i)
		ordered_order_func_list[i] = std::move(order_func_list[recv_sort_order[i]]);

	auto order_function = [ordered_order_func_list](const TAuctionItemPtr& a, const AuctionManager::TAuctionItemPtr& b) -> bool {
#ifdef __AUCTION_SORT_ONLY_SHUFFLE_EVERYTHING__
		return order_func_shuffle(a, b);
#else
		for (size_t i = 0; i < SEARCH_ORDER_MAX_NUM; ++i)
		{
			if (ordered_order_func_list[i](a, b))
				return true;

			if (ordered_order_func_list[i](b, a))
				return false;
		}

		return false;
#endif
	};

	// collect items differently by search type
	std::set<TAuctionItemPtr, decltype(order_function)> items(std::move(order_function));
	switch (search_type)
	{
	case SEARCH_TYPE_NONE:
	{
		std::vector<TItemsByIDMap*> search_maps;
		search_maps.push_back(&_items_by_map[channel][map_index]);

		if (channel == 0)
		{
			for (auto& pair : _items_by_map)
			{
				if (pair.first != 0)
					search_maps.push_back(&pair.second[map_index]);

				if (map_index == 0)
				{
					for (auto& pair_items : pair.second)
					{
						if (pair_items.first != 0)
							search_maps.push_back(&pair_items.second);
					}
				}
			}
		}
		else if (map_index == 0)
		{
			for (auto& pair_items : _items_by_map[channel])
			{
				if (pair_items.first != 0)
					search_maps.push_back(&pair_items.second);
			}
		}

		for (auto& search_map : search_maps)
		{
			for (auto& item : *search_map | boost::adaptors::map_values)
			{
				if (__check_search_item(pid, item, data, types, channel, map_index))
					items.insert(item);
			}
		}
	}
	break;

	case SEARCH_TYPE_ITEM:
	case SEARCH_TYPE_ITEM_STRICT:
	{
		__check_search_items(__find_items_by_name(language, data.basic_data().search_text(), SEARCH_TYPE_ITEM_STRICT == data.search_type()), items,
			pid, data, types, channel, map_index);

		if (items.empty() && LANGUAGE_ENGLISH != language)
		{
			__check_search_items(__find_items_by_name(LANGUAGE_ENGLISH, data.basic_data().search_text(), SEARCH_TYPE_ITEM_STRICT == data.search_type()), items,
				pid, data, types, channel, map_index);
		}
	}
	break;

	case SEARCH_TYPE_PLAYER:
	{
		auto player = __find_player(data.basic_data().search_text());
		if (player)
		{
			for (auto& item : player->items)
			{
				if (__check_search_item(pid, item, data, types, channel, map_index))
					items.insert(item);
			}

			if (player->shop)
			{
				for (auto& item : player->shop->items)
				{
					if (__check_search_item(pid, item, data, types, channel, map_index))
						items.insert(item);
				}
			}
		}
	}
	break;
	}

	// no items found
	if (items.empty())
	{
		__send_player_message(player_desc, pid, "NO_ITEMS_FOUND");
		return;
	}

	// check page index
	auto max_page = MIN(ITEMS_PER_PAGE - 1, (MAX(1, items.size()) - 1) / ITEMS_PER_PAGE);
	if (page > max_page)
		page = max_page;

	auto send_count = MIN(items.size(), (page + 1) * ITEMS_PER_PAGE) - page * ITEMS_PER_PAGE;

	// send results
	network::GCOutputPacket<network::GCAuctionSearchResultPacket> pack;
	pack->set_page(-1);
	pack->set_max_page(max_page);

	auto it_items = std::next(items.begin(), page * ITEMS_PER_PAGE);
	for (size_t i = 0; i < send_count; ++i)
		__append_send_search_item(pid, pack, *it_items++);

	player_desc->SetRelay(pid);
	player_desc->Packet(pack);

	if (test_server)
		sys_log(0, "AuctionManager::search_item EXTENDED %zu items found, %u items sent", items.size(), send_count);
}

void AuctionManager::shop_request_show(DWORD pid)
{
	if (auto player = __find_player(pid))
	{
		if (player->sent_shop)
			return;

		if (!player->shop)
			return;

		player->sent_shop = true;
		__send_player_shop(player);
	}
}

void AuctionManager::shop_open(LPDESC player_desc, network::GGAuctionShopOpenPacket&& data)
{
	if (test_server)
		sys_log(0, "AuctionManager::shop_open %u %s with %u items", data.owner_id(), data.owner_name().c_str(), data.items_size());

	auto player = __append_player(data.owner_id(), data.owner_name());
	if (auto shop = __open_shop(player, data))
	{
		__save_shop(player);
		__send_player_shop(__find_player(data.owner_id()));

		for (auto& item : *data.mutable_items())
		{
			network::TShopItemTable tab;
			tab.set_auction_type(TYPE_SHOP);
			*tab.mutable_item() = std::move(item);

			insert_item(player_desc, std::move(tab), 0, 0);
		}
	}
	else
	{
		for (auto& item : data.items())
		{
			network::GGOutputPacket<network::GGGiveItemPacket> pack;
			*pack->mutable_item() = item;
			pack->mutable_item()->mutable_cell()->set_cell(0);
			pack->set_no_refund(true);

			player_desc->Packet(pack);
		}
	}
}

void AuctionManager::shop_take_gold(LPDESC owner_desc, DWORD owner, uint64_t gold)
{
	if (test_server)
		sys_log(0, "AuctionManager::take_shop_gold %u gold %lld", owner, static_cast<long long>(gold));

	auto player = __find_player(owner);
	if (!player || !player->shop)
		return;

	if (gold > player->shop->gold)
		return;

	if (gold == 0)
		gold = player->shop->gold;
	if (!gold)
		return;

	// log
	LogManager::instance().AuctionLog(LOG_TAKE_GOLD, "SHOP", owner, owner, 0, 0, "amount " + std::to_string(gold) + " new storage " + std::to_string(player->shop->gold - static_cast<int64_t>(gold)));;

	// take gold process
	__change_shop_gold(player, -static_cast<int64_t>(gold));

	network::GGOutputPacket<network::GGGiveGoldPacket> pack;
	pack->set_pid(owner);
	pack->set_gold(gold);
	owner_desc->Packet(pack);
}

void AuctionManager::shop_view(LPDESC player_desc, DWORD pid, DWORD owner)
{
	auto player = __find_player(owner);
	if (!player || !player->shop)
		return;

	__add_shop_guest(player, pid, player_desc);
}

void AuctionManager::shop_view_cancel(DWORD pid)
{
	__remove_shop_guest(pid);
}

void AuctionManager::shop_mark(LPDESC player_desc, DWORD pid, DWORD item_id)
{
	auto item = __find_item(item_id);

	network::GGOutputPacket<network::GGAuctionAnswerMarkShopPacket> pack;
	pack->set_pid(pid);
	pack->set_owner_id(item ? item->shop_item.item().owner() : 0);
	player_desc->Packet(pack);
}

void AuctionManager::shop_reset_timeout(LPDESC owner_desc, DWORD pid, DWORD new_timeout)
{
	auto player = __find_player(pid);
	if (!player || !__reset_shop_timeout(player, new_timeout))
	{
		network::GGOutputPacket<network::GGGiveGoldPacket> pack;
		pack->set_pid(pid);
		pack->set_gold(SHOP_RENEW_COST);
		owner_desc->Packet(pack);

		LogManager::instance().AuctionLog(LOG_RENEW_FAILED, "", pid, pid, 0, 0, "restore " + std::to_string(SHOP_RENEW_COST) + " gold");
	}
}

void AuctionManager::shop_close(LPDESC owner_desc, DWORD pid, BYTE channel, DWORD map_index)
{
	auto player = __find_player(pid);
	if (player && player->shop)
	{
		if ((player->shop->channel != 0 && channel != 0 && player->shop->channel != channel) ||
			(player->shop->map_index != 0 && map_index != 0 && player->shop->map_index != map_index))
		{
			network::GCOutputPacket<network::GCChatPacket> pack;
			pack->set_type(CHAT_TYPE_INFO);
			pack->set_message(LC_TEXT(player->language, "You need to be on the same map as your shop."));
			owner_desc->SetRelay(pid);
			owner_desc->Packet(pack);
			return;
		}

		while (player->shop->items.size() > 0)
		{
			auto& auction_item = *player->shop->items.begin();
			take_item(owner_desc, pid, auction_item->shop_item.item().id());

			// check to stop infinity loop if take_item fails
			if (player->shop->items.size() > 0 && *player->shop->items.begin() == auction_item)
				return;
		}

		if (player->shop->gold > 0)
			shop_take_gold(owner_desc, pid, player->shop->gold);

		__check_remove_shop(player);
	}
}

void AuctionManager::shop_request_history(LPDESC player_desc, DWORD pid)
{
	auto history = __find_shop_history(pid);
	if (!history)
		return;

	network::GCOutputPacket<network::GCAuctionShopHistoryPacket> pack;
	for (auto& entry : *history)
		*pack->add_elems() = entry;

	player_desc->SetRelay(pid);
	player_desc->Packet(pack);
}

void AuctionManager::request_average_price(LPDESC player_desc, DWORD pid, DWORD requestor, DWORD vnum, DWORD count)
{
	network::GCOutputPacket<network::GCAuctionAveragePricePacket> pack;
	pack->set_requestor(requestor);
	pack->set_price(__get_average_price(vnum) * count);

	player_desc->SetRelay(pid);
	player_desc->Packet(pack);
}

void AuctionManager::on_player_login(DWORD pid, CCI* p2p_player)
{
	if (auto player = __find_player(pid))
	{
		player->desc = p2p_player->pkDesc;
		player->language = p2p_player->bLanguage;
		player->sent_shop = false;

		__send_player_gold(player);
		for (auto item : player->items)
			__send_player_item(player, &item->shop_item);

		__send_player_shop_owned(player);
	}
}

void AuctionManager::on_player_logout(DWORD pid)
{
	__remove_shop_guest(pid, false);

	if (auto player = __find_player(pid))
		player->desc = nullptr;
}

void AuctionManager::on_player_delete(DWORD pid)
{
	if (auto player = __find_player(pid))
		__remove_player(player);

	_shop_histories.erase(pid);
}

void AuctionManager::on_receive_map_location()
{
	for (auto& player : _players | boost::adaptors::map_values)
		__spawn_shop(player.get());
}

void AuctionManager::on_connect_peer(LPDESC peer)
{
	for (auto& player : _players | boost::adaptors::map_values)
		__spawn_shop(player.get());
}

void AuctionManager::on_disconnect_peer(LPDESC peer)
{
	for (auto& player : _players | boost::adaptors::map_values)
	{
		if (player->shop)
			player->shop->spawned_descs.erase(peer);
	}
}

void AuctionManager::on_shop_timeout(TAuctionPlayer* player)
{
	__despawn_shop(player);
}

void AuctionManager::on_update_average_prices()
{
	if (test_server)
		sys_log(0, "AuctionManager::on_update_average_prices (%zu items)", _items.size());

	_average_prices.clear();

	// build price map
	std::unordered_map<DWORD, std::vector<std::pair<DWORD, long long>>> prices;
	for (auto& auction_item : _items | boost::adaptors::map_values)
	{
		auto& item = auction_item->shop_item.item();
		if (item.count() == 0)
			continue;

		prices[item.vnum()].push_back(std::make_pair(item.count(), item.price()));
	}

	// accumulate prices
	for (auto& pair : prices)
	{
		auto& save_price = _average_prices[pair.first];
		uint64_t item_count = 0;

		auto& vec = pair.second;
		save_price = std::accumulate(vec.begin(), vec.end(), 0, [&item_count](long long current, std::pair<DWORD, long long>& data) {
			item_count += data.first;
			return current + data.second;
		}) / item_count;
	}

	if (test_server)
		sys_log(0, "AuctionManager::on_update_average_prices DONE (%zu items)", _items.size());
}

void AuctionManager::on_item_realtime_expire(TAuctionItemPtr auction_item)
{
	// reset event pointer
	auction_item->real_timeout_event = nullptr;

	// remove from auction/shop
	__remove_item(auction_item->shop_item.item().id());

	// remove from auction item table
	auction_item->shop_item.mutable_item()->set_vnum(0);
	__save_item(&auction_item->shop_item);

	// remove from item table
	DBManager::instance().Query("DELETE FROM item WHERE id = %u", auction_item->shop_item.item().id());
}

AuctionManager::TAuctionPlayer* AuctionManager::__find_player(DWORD pid) const
{
	auto it = _players.find(pid);
	if (it == _players.end())
		return nullptr;

	return it->second.get();
}

AuctionManager::TAuctionPlayer* AuctionManager::__find_player(const std::string& name) const
{
	auto it = _players_by_name.find(str_to_lower(name.c_str()));
	if (it == _players_by_name.end())
		return nullptr;

	return it->second;
}

AuctionManager::TAuctionItemPtr AuctionManager::__find_item(DWORD id) const
{
	auto it = _items.find(id);
	if (it == _items.end())
		return nullptr;

	return it->second;
}

const std::vector<network::TAuctionShopHistoryElement>* AuctionManager::__find_shop_history(DWORD pid) const
{
	auto it = _shop_histories.find(pid);
	if (it == _shop_histories.end())
		return nullptr;

	return &it->second;
}

void AuctionManager::__give_item_to_player(LPDESC player_desc, TAuctionItemPtr auction_item, DWORD target_pid, WORD inventory_pos)
{
	auto shop_item = &auction_item->shop_item;

	// remove
	__remove_item(shop_item->item().id());

	// give item to owner
	network::GGOutputPacket<network::GGGiveItemPacket> pack;
	auto pack_item = pack->mutable_item();
	*pack_item = shop_item->item();
	if (target_pid != 0)
		pack_item->set_owner(target_pid);
	pack_item->mutable_cell()->set_cell(inventory_pos);
	player_desc->Packet(pack);

	// remove from db & sql
	shop_item->mutable_item()->set_vnum(0);
	__save_item(shop_item);
}

bool AuctionManager::__can_buy(const TAuctionItemPtr& auction_item)
{
	if (auction_item->get_timeout() != 0 && get_global_time() >= auction_item->get_timeout())
		return false;

	return true;
}

void AuctionManager::__compute_taxes(DWORD pid, const TAuctionItemPtr& auction_item, size_t* ret_owner_tax, size_t* ret_player_tax) const
{
	switch (auction_item->shop_item.auction_type())
	{
	case TYPE_AUCTION:
		if (ret_owner_tax)
			*ret_owner_tax = TAX_AUCTION_OWNER;
		if (ret_player_tax)
			*ret_player_tax = 0; // obviously no tax for players to buy a auction item in the auction house (that's the only way to buy it.. just put the tax on the owner, makes more sense :D)
		break;

	case TYPE_SHOP:
		if (ret_player_tax)
			*ret_player_tax = TAX_SHOP_PLAYER;
		if (ret_owner_tax)
			*ret_owner_tax = TAX_SHOP_OWNER;

		// no tax for shop buy if currently visiting this shop
		if (ret_player_tax)
		{
			auto& shop = auction_item->owner->shop;
			if (shop && shop->guests.find(pid) != shop->guests.end())
				*ret_player_tax = 0;
		}
		break;
	}
}

AuctionManager::TAuctionItemPtr AuctionManager::__append_item(network::TShopItemTable&& shop_item, bool is_boot)
{
	// check if data is valid
	auto& item = shop_item.item();
	if (!item.id() || !item.owner() || item.price() <= 0)
	{
		sys_err("AuctionManager: cannot append item with no id/owner/price?? %u | %u | %lld",
			item.id(), item.owner(), static_cast<long long>(item.price()));
		return nullptr;
	}

	if (shop_item.auction_type() == TYPE_AUCTION)
	{
		if (item.cell().window_type() != AUCTION)
		{
			sys_err("AuctionManager: cannot append auction item with invalid window type %u", item.cell().window_type());
			return nullptr;
		}
	}
	else if (shop_item.auction_type() == TYPE_SHOP)
	{
		if (item.cell().window_type() != AUCTION_SHOP)
		{
			sys_err("AuctionManager: cannot append shop item with invalid window type %u", item.cell().window_type());
			return nullptr;
		}

		auto player = __find_player(item.owner());
		if (!player || !player->shop)
		{
			sys_err("AuctionManager: cannot append shop item with no shop");
			return nullptr;
		}

		auto proto = ITEM_MANAGER::instance().GetTable(item.vnum());
		size_t item_size = MAX(1, proto ? proto->size() : 1);

		if (item.cell().cell() + (item_size - 1) * SHOP_SLOT_COUNT_X >= SHOP_SLOT_COUNT)
		{
			sys_err("AuctionManager: cannot append shop item with invalid cell %u", item.cell().cell());
			return nullptr;
		}

		for (size_t i = 0; i < item_size; ++i)
		{
			if (player->shop->item_grid[item.cell().cell() + i * SHOP_SLOT_COUNT_X])
			{
				sys_err("AuctionManager: cannot append shop item with already used cell %u size %zu (i=%zu)", item.cell().cell(), item_size, i);
				return nullptr;
			}
		}

		if (!is_boot)
		{
			if (player->shop->timeout != 0 && get_global_time() >= player->shop->timeout)
			{
				sys_err("AuctionManager: cannot append shop item into timed out shop");
				return nullptr;
			}
		}
	}
	else
	{
		sys_err("AuctionManager: cannot append item: unhandled auction type %u", shop_item.auction_type());
		return nullptr;
	}

	if (__find_item(item.id()))
	{
		sys_err("AuctionManager: cannot append item - already existing ID %u", item.id());
		return nullptr;
	}

	// get player data (create if not exists)
	auto player = __append_player(item.owner(), shop_item.owner_name());

	auto stored_item = std::make_shared<TAuctionItem>();
	stored_item->owner = player;
	stored_item->shop_item = std::move(shop_item);
	stored_item->shop_item.clear_owner_name();

	stored_item->random_id = random_number(0, 999999);

	// add item to id map
	_items[item.id()] = stored_item;
	// add item to other maps
	switch (stored_item->shop_item.auction_type())
	{
	case TYPE_AUCTION:
		player->items.insert(stored_item);
		_items_by_map[0][0][item.id()] = stored_item;
		break;
	case TYPE_SHOP:
		player->shop->items.insert(stored_item);
		_items_by_map[player->shop->channel][player->shop->map_index][item.id()] = stored_item;
		break;
	}

	// add item to name search list
	stored_item->proto = ITEM_MANAGER::instance().GetTable(stored_item->shop_item.item().vnum());
	if (auto proto = stored_item->proto)
	{
		for (size_t i = 0; i < LANGUAGE_MAX_NUM && i < proto->locale_name_size(); ++i)
			__get_item_name_map(i, proto->locale_name(i)).insert(std::make_pair(str_to_lower(proto->locale_name(i).c_str()), stored_item));
	}

	// check for real timeout
	if (stored_item->proto)
	{
		DWORD real_timeout = ITEM_MANAGER::instance().GetItemRealTimeout(stored_item->proto, stored_item->shop_item.item().sockets());
		DWORD current_time = get_global_time();
		if (real_timeout > 0 && real_timeout > current_time)
		{
			auto info = AllocEventInfo<TAuctionItemEventInfo>();
			info->auction_item = stored_item;
			stored_item->real_timeout_event = event_create(auction_item_realtime_timeout_event, info, PASSES_PER_SEC(real_timeout - current_time));
		}
	}

	// if shop item: spawn npc, reset timeout to shop timeout and set item grid
	if (stored_item->shop_item.auction_type() == TYPE_SHOP)
	{
		if (player->shop->items.size() == 1)
			__spawn_shop(player);

		stored_item->shop_item.set_map_index(player->shop->map_index);
		stored_item->shop_item.set_timeout_time(player->shop->timeout);

		auto start_cell = stored_item->shop_item.item().cell().cell();
		for (size_t i = 0; i < MAX(1, stored_item->proto ? stored_item->proto->size() : 1); ++i)
			player->shop->item_grid[start_cell + i * SHOP_SLOT_COUNT_X] = true;
	}

	return stored_item;
}

void AuctionManager::__remove_item(DWORD item_id)
{
	if (test_server)
		sys_log(0, "AuctionManager::__remove_item : %u", item_id);

	auto auction_item = __find_item(item_id);
	if (auction_item == nullptr)
		return;

	auto& item = auction_item->shop_item.item();

	auto player = auction_item->owner;
	__send_player_item(player, &auction_item->shop_item, true);
	__send_player_shop_item(player, &auction_item->shop_item, true);

	// cancel timeout event if running
	event_cancel(&auction_item->real_timeout_event);

	// remove item from sell
	_items.erase(item.id());
	// remove item from other lists
	switch (auction_item->shop_item.auction_type())
	{
	case TYPE_AUCTION:
		player->items.erase(auction_item);
		_items_by_map[0][0].erase(item.id());

		__check_remove_player(player);
		break;

	case TYPE_SHOP:
		player->shop->items.erase(auction_item);
		for (size_t i = 0; i < MAX(1, auction_item->proto ? auction_item->proto->size() : 1); ++i)
			player->shop->item_grid[item.cell().cell() + i * SHOP_SLOT_COUNT_X] = false;
		_items_by_map[player->shop->channel][player->shop->map_index].erase(item.id());

		__check_hide_shop(player);
		break;
	}

	// remove item from name search list
	if (auto proto = auction_item->proto)
	{
		for (size_t i = 0; i < LANGUAGE_MAX_NUM && i < proto->locale_name_size(); ++i)
		{
			auto& map = __get_item_name_map(i, proto->locale_name(i));
			auto range = map.equal_range(str_to_lower(proto->locale_name(i).c_str()));
			for (auto it = range.first; it != range.second; ++it)
			{
				if (it->second == auction_item)
				{
					map.erase(it);
					break;
				}
			}
		}
	}
}

void AuctionManager::__change_gold(TAuctionPlayer* player, int64_t amount)
{
	if (amount < 0 && abs(amount) > player->gold)
		amount = -player->gold;

	player->gold += amount;
	__save_player(player);
	__send_player_gold(player);

	if (player->gold == 0)
		__check_remove_player(player);
}

void AuctionManager::__change_shop_gold(TAuctionPlayer* player, int64_t amount)
{
	if (amount < 0 && abs(amount) > player->shop->gold)
		amount = -player->shop->gold;

	player->shop->gold += amount;
	__save_shop(player);
	__send_player_shop_gold(player);
}

AuctionManager::TItemByNameMultiMap& AuctionManager::__get_item_name_map(BYTE language, const std::string& name)
{
	char first_char = 0;
	if (name.length() > 0)
		first_char = name[0];

	first_char = ::tolower(first_char);
	if (first_char >= 'a' && first_char <= 'z')
		return _items_by_name[language][first_char - 'a'];
	else
		return _items_by_name[language][ALPHABET_CHAR_COUNT];
}

AuctionManager::TItemNameMapRange AuctionManager::__find_items_by_name(BYTE language, const std::string& name, bool strict)
{
	if (!strict)
	{
		_items_by_name_compare_strict = false;
		_items_by_name_compare_len = name.length();
	}
	TItemNameMapRange range = __get_item_name_map(language, name).equal_range(str_to_lower(name.c_str()));
	if (!strict)
	{
		_items_by_name_compare_strict = true;
	}

	return range;
}

bool AuctionManager::__check_search_item(DWORD pid, const TAuctionItemPtr& auction_item, const network::TDataAuctionSearch& options, const TSearchTypesMap& types) const
{
	if (!auction_item->proto)
		return false;

	// check for timeout
	if (pid != auction_item->shop_item.item().owner())
	{
		auto timeout_time = auction_item->get_timeout();
		if (timeout_time != 0 && get_global_time() >= timeout_time)
			return false;
	}

	// check if item type is valid
	if (types.size() > 0)
	{
		bool types_check = false;
		auto types_it = types.find(auction_item->proto->type());
		if (types_it != types.end())
		{
			auto& cur_sub_types = types_it->second;
			if (cur_sub_types.size() == 0 || cur_sub_types.find(auction_item->proto->sub_type()) != cur_sub_types.end())
				types_check = true;
		}

		// check for special types
		if (!types_check)
		{
			if (CItemVnumHelper::IsOreItem(auction_item->proto->vnum()))
			{
				types_it = types.find(ITEM_MATERIAL);
				if (types_it != types.end() && types_it->second.size() == 0)
					types_check = true;
			}
			else if (CItemVnumHelper::IsFishItem(auction_item->proto->vnum()))
			{
				types_it = types.find(ITEM_FISH);
				if (types_it != types.end())
					types_check = true;
			}
			else if (CItemVnumHelper::IsSoulstoneItem(auction_item->proto->vnum()))
			{
				types_it = types.find(ITEM_SKILLBOOK);
				if (types_it != types.end())
					types_check = true;
			}
		}

		if (!types_check)
			return false;
	}

	// check if socket0 is valid
	if (options.socket0() >= 0 && options.socket0() != auction_item->shop_item.item().sockets(0))
		return false;

	// check if value0 is valid
	if (options.value0() >= 0 && options.value0() != auction_item->proto->values(0))
		return false;

	// check for exclude_anti_flags
	if ((options.include_anti_flag() & (~auction_item->proto->anti_flags())) == 0)
		return false;

	return true;
}

bool AuctionManager::__check_search_item(DWORD pid, const TAuctionItemPtr& auction_item, const network::TExtendedDataAuctionSearch& options, const TSearchTypesMap& types, BYTE channel, DWORD map_index) const
{
	if (!__check_search_item(pid, auction_item, options.basic_data(), types))
		return false;

	auto& shop_item = auction_item->shop_item;

	// check for channel
	if (channel != 0 && shop_item.auction_type() == TYPE_SHOP && shop_item.channel() != 0 && shop_item.channel() != channel)
		return false;

	// check for same map
	if (map_index != 0 && shop_item.auction_type() == TYPE_SHOP && shop_item.map_index() != map_index)
		return false;

	// check for level limit
	auto level_limit = 0;
	for (size_t i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		auto& limit = auction_item->proto->limits(i);
		if (limit.type() == LIMIT_LEVEL)
		{
			level_limit = limit.value();
			break;
		}
	}

	if (options.level_min() > level_limit || (options.level_max() && options.level_max() < level_limit))
		return false;

	// check for price limit
	if (options.price_min() > shop_item.item().price() || (options.price_max() && options.price_max() < shop_item.item().price()))
		return false;

	// check for only auction items
	if (options.is_only_auction_item() && shop_item.auction_type() != TYPE_AUCTION)
		return false;

	// check for only self items
	if (options.is_only_self_item() && shop_item.item().owner() != pid)
		return false;

	return true;
}

void AuctionManager::__append_send_search_item(DWORD owner, network::GCOutputPacket<network::GCAuctionSearchResultPacket>& pack, const TAuctionItemPtr& item)
{
	auto added = pack->add_items();
	*added = item->shop_item;

	added->set_owner_name(item->owner->name);
	added->set_timeout_time(item->get_timeout());

	size_t player_tax = 0;
	__compute_taxes(owner, item, nullptr, &player_tax);

	if (player_tax > 0)
		added->mutable_item()->set_price(added->item().price() * (100 + player_tax) / 100);
}

AuctionManager::TAuctionPlayer* AuctionManager::__append_player(DWORD pid, const std::string& name)
{
	auto& player = _players[pid];
	if (player)
		return player.get();

	player = std::unique_ptr<TAuctionPlayer>(new TAuctionPlayer());

	player->pid = pid;
	player->name = name;

	_players_by_name[str_to_lower(name.c_str())] = player.get();

	if (auto p2p_player = P2P_MANAGER::instance().FindByPID(pid))
		on_player_login(pid, p2p_player);

	return player.get();
}

AuctionManager::TAuctionShop* AuctionManager::__open_shop(TAuctionPlayer* player, const network::GGAuctionShopOpenPacket& data, DWORD create_time, uint64_t gold)
{
	if (player->shop)
		return nullptr;

	auto shop = std::unique_ptr<TAuctionShop>(new TAuctionShop());

	shop->owner = player;

	shop->vnum = data.vnum();
	shop->style = data.style();
	shop->name = data.name();

	shop->color_red = data.color_red();
	shop->color_green = data.color_green();
	shop->color_blue = data.color_blue();

	shop->channel = data.channel();
	shop->map_index = data.map_index();
	shop->x = data.x();
	shop->y = data.y();

	shop->create_time = create_time ? create_time : get_global_time();
	shop->timeout = data.timeout();

	shop->gold = gold;

	player->shop = std::move(shop);
	__start_shop_timeout(player);

	return player->shop.get();
}

network::TAuctionShopHistoryElement& AuctionManager::__append_shop_history(DWORD owner, DWORD vnum, uint64_t price, const std::string& buyer_name, DWORD date)
{
	network::TAuctionShopHistoryElement elem;
	elem.set_vnum(vnum);
	elem.set_price(price);
	elem.set_buyer(buyer_name);
	elem.set_date(date ? date : get_global_time());

	auto& vec = _shop_histories[owner];
	vec.push_back(std::move(elem));

	if (vec.size() > SHOP_HISTORY_LIMIT)
	{
		vec.erase(vec.begin());
		__remove_oldest_shop_history(owner);
	}

	return *vec.rbegin();
}

void AuctionManager::__check_remove_player(TAuctionPlayer* player)
{
	if (player->items.size() > 0)
		return;

	if (player->gold > 0)
		return;

	if (player->shop)
		return;

	__remove_player(player);
}

void AuctionManager::__remove_player(TAuctionPlayer* player)
{
	__hide_shop(player);

	if (player->shop)
	{
		while (player->shop->items.size() > 0)
		{
			auto& auction_item = *player->shop->items.begin();
			__remove_item(auction_item->shop_item.item().id());
		}
	}

	while (player->items.size() > 0)
	{
		auto& auction_item = *player->items.begin();
		__remove_item(auction_item->shop_item.item().id());
	}

	_players_by_name.erase(str_to_lower(player->name.c_str()));
	_players.erase(player->pid);
}

void AuctionManager::__check_hide_shop(TAuctionPlayer* player)
{
	if (!player->shop)
		return;

	if (player->shop->timeout == 0 || player->shop->timeout > get_global_time())
	{
		if (player->shop->items.size() > 0)
			return;
	}

	__hide_shop(player);
}

void AuctionManager::__hide_shop(TAuctionPlayer* player)
{
	if (!player->shop)
		return;

	while (player->shop->guests.size() > 0)
		__remove_shop_guest(player->shop->guests.begin()->first);
	__despawn_shop(player);
}

void AuctionManager::__check_remove_shop(TAuctionPlayer* player)
{
	if (!player->shop)
		return;

	if (player->shop->items.size() > 0)
		return;

	if (player->shop->gold > 0)
		return;

	player->shop.reset();

	__send_player_shop_owned(player);
	__save_shop(player);

	__check_remove_player(player);
}

bool AuctionManager::__add_shop_guest(TAuctionPlayer* player, DWORD pid, LPDESC desc)
{
	if (player->pid == pid)
	{
		network::GCOutputPacket<network::GCChatPacket> pack;
		pack->set_type(CHAT_TYPE_COMMAND);
		pack->set_message("OpenMyAuctionShop");

		desc->SetRelay(pid);
		desc->Packet(pack);

		return false;
	}

	if (!player->shop)
		return false;

	if (player->shop->guests.find(pid) != player->shop->guests.end())
	{
		if (test_server)
			sys_err("already shop guest");
		return false;
	}

	if (player->shop->timeout > 0 && get_global_time() >= player->shop->timeout)
	{
		if (test_server)
			sys_err("shop already timeout");
		return false;
	}

	if (player->shop->items.size() == 0)
	{
		if (test_server)
			sys_err("shop no items");
		return false;
	}

	if (test_server)
		sys_log(0, "AuctionManager::__add_shop_guest: %u for shop %u %s", pid, player->pid, player->name.c_str());

	// remove is already guest in another shop
	__remove_shop_guest(pid);

	// add guest
	player->shop->guests[pid] = desc;
	_shop_guests[pid] = player->pid;

	network::GCOutputPacket<network::GCAuctionShopGuestOpenPacket> pack;
	pack->set_name(player->shop->name);
	for (auto& auction_item : player->shop->items)
		*pack->add_items() = auction_item->shop_item.item();

	desc->SetRelay(pid);
	desc->Packet(pack);

	return true;
}


void AuctionManager::__remove_shop_guest(DWORD pid, bool close_packet)
{
	auto it = _shop_guests.find(pid);
	if (it == _shop_guests.end())
		return;

	if (auto player = __find_player(it->second))
	{
		if (player->shop)
		{
			auto it_shop = player->shop->guests.find(pid);
			if (it_shop != player->shop->guests.end())
			{
				if (close_packet)
				{
					auto desc = it_shop->second;
					desc->SetRelay(pid);
					desc->Packet(network::TGCHeader::AUCTION_SHOP_GUEST_CLOSE);
				}

				player->shop->guests.erase(it_shop);
			}
		}
	}

	_shop_guests.erase(it);
}

void AuctionManager::__save_item(const network::TShopItemTable* shop_item) const
{
	if (!shop_item)
	{
		sys_err("AuctionManager: try to save nullptr item");
		return;
	}

	auto& item = shop_item->item();
	std::ostringstream query;

	if (item.vnum() != 0)
	{
		network::GDOutputPacket<network::GDItemSavePacket> pack;
		*pack->mutable_data() = item;
		db_clientdesc->DBPacket(pack);

		query << "REPLACE INTO auction_item SET item_id=" << item.id() << ", price=" << item.price() << ", insertion_time=FROM_UNIXTIME(" << shop_item->insertion_time() << "), "
			"timeout=FROM_UNIXTIME(" << shop_item->timeout_time() << ")";
	}
	else
	{
		query << "DELETE FROM auction_item WHERE item_id=" << item.id();
	}

	DBManager::instance().Query("%s", query.str().c_str());
}

void AuctionManager::__save_player(TAuctionPlayer* player) const
{
	DBManager::instance().Query("UPDATE player SET auction_gold=%lld WHERE id=%u", player->gold, player->pid);
}

void AuctionManager::__save_shop(TAuctionPlayer* player) const
{
	std::ostringstream query;

	if (auto& shop = player->shop)
	{
		size_t escaped_len = shop->name.length() * 2 + 1;
		char* escaped_name = new char[escaped_len];
		DBManager::instance().EscapeString(escaped_name, escaped_len, shop->name.c_str(), shop->name.length());

		query << "REPLACE INTO auction_shop SET pid=" << player->pid << ", vnum=" << shop->vnum << ", style=" << shop->style << ", name='" << shop->name <<"', "
			"color_red=" << shop->color_red << ", color_green=" << shop->color_green << ", color_blue=" << shop->color_blue << ", "
			"channel=" << static_cast<uint32_t>(shop->channel) << ", map_index=" << shop->map_index << ", x=" << shop->x << ", y=" << shop->y << ", "
			"create_time=FROM_UNIXTIME(" << shop->create_time << "), timeout=FROM_UNIXTIME(" << shop->timeout << "), gold=" << shop->gold;

		delete[] escaped_name;
	}
	else
	{
		query << "DELETE FROM auction_shop WHERE pid=" << player->pid;
	}

	DBManager::instance().Query("%s", query.str().c_str());
}

void AuctionManager::__remove_oldest_shop_history(DWORD pid) const
{
	DBManager::instance().Query("DELETE FROM `auction_shop_history` WHERE pid = %u ORDER BY date ASC LIMIT 1", pid);
}

void AuctionManager::__save_shop_history(DWORD pid, const network::TAuctionShopHistoryElement& elem) const
{
	DBManager::instance().Query("INSERT INTO auction_shop_history (pid, vnum, price, buyer, date) VALUES (%u, %u, %llu, '%s', FROM_UNIXTIME(%u))",
		pid, elem.vnum(), elem.price(), elem.buyer().c_str(), elem.date());
}

void AuctionManager::__spawn_shop(TAuctionPlayer* player)
{
	auto& shop = player->shop;
	if (!shop)
		return;

	if (shop->timeout != 0 && get_global_time() >= shop->timeout)
		return;

	if (shop->items.size() == 0)
		return;

	if (shop->channel != 0)
	{
		LPDESC peer = P2P_MANAGER::instance().FindPeerByMap(shop->map_index, shop->channel);
		if (peer)
			__spawn_shop(player, peer);
	}
	else
	{
		CMapLocation::instance().for_each_channel([&shop, player, this](BYTE channel) {
			LPDESC peer = P2P_MANAGER::instance().FindPeerByMap(shop->map_index, channel);
			if (test_server)
				sys_log(0, "AuctionManager::__spawn_shop pid %u map %u channel %u peer %s", player->pid, shop->map_index, channel, peer ? "FOUND" : "NOT_FOUND");

			if (peer)
				__spawn_shop(player, peer);
		});
	}
}

void AuctionManager::__spawn_shop(TAuctionPlayer* player, LPDESC peer)
{
	auto& shop = player->shop;

	if (shop->spawned_descs.find(peer) != shop->spawned_descs.end())
		return;

	shop->spawned_descs.insert(peer);

	network::GGOutputPacket<network::GGAuctionShopSpawnPacket> pack;
	pack->set_name(shop->name);
	pack->set_owner_id(player->pid);
	pack->set_owner_name(player->name);
	pack->set_vnum(shop->vnum);
	pack->set_style(shop->style);

	pack->set_color_red(shop->color_red);
	pack->set_color_green(shop->color_green);
	pack->set_color_blue(shop->color_blue);

	pack->set_map_index(shop->map_index);
	pack->set_x(shop->x);
	pack->set_y(shop->y);

	peer->Packet(pack);
}

void AuctionManager::__despawn_shop(TAuctionPlayer* player)
{
	auto& shop = player->shop;
	if (!shop)
		return;

	network::GGOutputPacket<network::GGAuctionShopDespawnPacket> pack;
	pack->set_owner_id(player->pid);

	for (LPDESC peer : shop->spawned_descs)
		peer->Packet(pack);

	shop->spawned_descs.clear();
}

void AuctionManager::__start_shop_timeout(TAuctionPlayer* player)
{
	auto& shop = player->shop;
	if (!shop)
		return;

	event_cancel(&shop->timeout_event);

	auto current_time = get_global_time();
	if (shop->timeout > 0 && current_time < shop->timeout)
	{
		auto timeout_event_info = AllocEventInfo<TAuctionPlayerEventInfo>();
		timeout_event_info->player = player;
		shop->timeout_event = event_create(auction_shop_timeout_event, timeout_event_info, PASSES_PER_SEC(shop->timeout - current_time));
	}
}

bool AuctionManager::__reset_shop_timeout(TAuctionPlayer* player, DWORD new_timeout)
{
	auto& shop = player->shop;
	if (!shop)
		return false;

	if (shop->timeout == 0)
		return false;

	if (new_timeout <= get_global_time())
		return false;

	shop->timeout = new_timeout;

	network::GCOutputPacket<network::GCAuctionShopTimeoutPacket> pack;
	pack->set_timeout(new_timeout);
	if (player->desc)
	{
		player->desc->SetRelay(player->pid);
		player->desc->Packet(pack);
	}

	__spawn_shop(player);
	__save_shop(player);
	__start_shop_timeout(player);

	return true;
}

uint64_t AuctionManager::__get_average_price(DWORD vnum) const
{
	auto it = _average_prices.find(vnum);
	if (it == _average_prices.end())
		return 0;

	return it->second;
}

void AuctionManager::__send_player_message(LPDESC peer, DWORD pid, const std::string& message) const
{
	if (test_server)
		sys_log(0, "AuctionManager::__send_player_message %u %s", pid, message.c_str());

	if (!peer)
		return;

	network::GCOutputPacket<network::GCAuctionMessagePacket> pack;
	pack->set_message(message);

	peer->SetRelay(pid);
	peer->Packet(pack);
}

void AuctionManager::__send_player_gold(TAuctionPlayer* player) const
{
	if (!player || !player->desc)
		return;

	network::GCOutputPacket<network::GCAuctionOwnedGoldPacket> pack;
	pack->set_gold(player->gold);

	player->desc->SetRelay(player->pid);
	player->desc->Packet(pack);
}

void AuctionManager::__send_player_item(TAuctionPlayer* player, const network::TShopItemTable* item, bool is_remove) const
{
	if (!player || !player->desc)
		return;

	network::GCOutputPacket<network::GCAuctionOwnedItemPacket> pack;
	auto pack_item = pack->mutable_item();
	*pack_item = *item;
	if (is_remove)
		pack_item->mutable_item()->set_vnum(0);

	player->desc->SetRelay(player->pid);
	player->desc->Packet(pack);
}

void AuctionManager::__send_player_shop_owned(TAuctionPlayer* player) const
{
	if (!player || !player->desc)
		return;

	network::GCOutputPacket<network::GCAuctionShopOwnedPacket> pack;
	pack->set_owned(player->shop != nullptr);

	player->desc->SetRelay(player->pid);
	player->desc->Packet(pack);
}

void AuctionManager::__send_player_shop(TAuctionPlayer* player) const
{
	if (!player || !player->desc)
		return;

	network::GCOutputPacket<network::GCAuctionShopPacket> pack;
	
	if (auto& shop = player->shop)
	{
		pack->set_name(shop->name);
		pack->set_timeout(shop->timeout);
		pack->set_gold(shop->gold);

		for (auto& auction_item : shop->items)
			*pack->add_items() = auction_item->shop_item.item();
	}

	player->desc->SetRelay(player->pid);
	player->desc->Packet(pack);
}

void AuctionManager::__send_player_shop_gold(TAuctionPlayer* player) const
{
	if (!player || !player->desc || !player->shop)
		return;

	network::GCOutputPacket<network::GCAuctionShopGoldPacket> pack;
	pack->set_gold(player->shop->gold);

	player->desc->SetRelay(player->pid);
	player->desc->Packet(pack);
}

void AuctionManager::__send_player_shop_item(TAuctionPlayer* player, const network::TShopItemTable* item, bool is_remove) const
{
	if (!player || !player->shop || item->item().cell().window_type() != AUCTION_SHOP)
		return;

	network::GCOutputPacket<network::GCAuctionShopGuestUpdatePacket> pack;

	auto pack_item = pack->mutable_item();
	if (is_remove)
		pack_item->set_id(item->item().id());
	else
		*pack_item = item->item();

	for (auto& pair : player->shop->guests)
	{
		pair.second->SetRelay(pair.first);
		pair.second->Packet(pack);
	}
}

#endif
