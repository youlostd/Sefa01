#include "StdAfx.h"

#ifdef ENABLE_AUCTION
#include "PythonAuction.h"
#include "PythonNetworkStream.h"
#include "PythonPlayer.h"
#include "PythonApplication.h"

constexpr DWORD CPythonAuction::SHOP_MODEL_VNUMS[];

CPythonAuction::CPythonAuction()
{
	initialize();
}

CPythonAuction::~CPythonAuction()
{
}

void CPythonAuction::initialize()
{
	for (size_t i = 0; i < ITEM_CONTAINER_MAX_NUM; ++i)
	{
		_items[i].clear();
		_items_by_id[i].clear();
		_items_by_slot[i].clear();
	}

	_shop_creating_items.clear();

	_max_page_count = 0;
	_owned_gold = 0;

	_shop_owned = false;
	_shop_owned_name.clear();
	_shop_owned_timeout = 0;
	_shop_owned_gold = 0;

	_shop_name.clear();

	_search_settings.Clear();

	_selected_container_type = ITEM_CONTAINER_SEARCH;

	_shop_history.clear();
}

void CPythonAuction::append_item(BYTE container_type, network::TShopItemTable&& shop_item)
{
	if (container_type >= ITEM_CONTAINER_MAX_NUM)
		return;

	auto auction_item = find_item(container_type, shop_item.item().id());
	if (auction_item)
	{
		auction_item->shop_item = std::move(shop_item);
		return;
	}

	auction_item = std::make_shared<TAuctionItem>();

	_items[container_type].push_back(auction_item);
	_items_by_id[container_type][shop_item.item().id()] = auction_item;

	if (container_type == ITEM_CONTAINER_SHOP || container_type == ITEM_CONTAINER_OWNED_SHOP)
		_items_by_slot[container_type][shop_item.item().cell().cell()] = auction_item;

	auction_item->container_type = container_type;
	auction_item->shop_item = std::move(shop_item);
}

size_t CPythonAuction::get_item_count(BYTE container_type) const
{
	if (container_type >= ITEM_CONTAINER_MAX_NUM)
		return 0;

	return _items[container_type].size();
}

network::TShopItemTable* CPythonAuction::get_item(BYTE container_type, size_t index)
{
	if (container_type >= ITEM_CONTAINER_MAX_NUM)
		return nullptr;

	auto& vec = _items[container_type];
	if (vec.size() <= index)
		return nullptr;

	return &vec[index]->shop_item;
}

CPythonAuction::TAuctionItemPtr CPythonAuction::find_item(BYTE container_type, DWORD item_id)
{
	auto it = _items_by_id[container_type].find(item_id);
	if (it == _items_by_id[container_type].end())
		return nullptr;

	return it->second;
}

CPythonAuction::TAuctionItemPtr CPythonAuction::find_item_by_cell(BYTE container_type, WORD cell)
{
	if (container_type >= ITEM_CONTAINER_MAX_NUM)
		return nullptr;

	auto& map = _items_by_slot[container_type];
	auto it = map.find(cell);
	if (it == map.end())
		return nullptr;

	return it->second;
}

size_t CPythonAuction::get_container_index_by_id(BYTE container_type, DWORD id)
{
	auto& container = _items[container_type];
	
	size_t ret = 0;
	for (auto& auction_item : container)
	{
		if (auction_item->shop_item.item().id() == id)
			return ret;

		++ret;
	}

	return ret;
}

size_t CPythonAuction::get_item_index(BYTE container_type, TAuctionItemPtr auction_item)
{
	if (container_type >= ITEM_CONTAINER_MAX_NUM)
		return 0;

	auto& vec = _items[container_type];
	auto it = std::find(vec.begin(), vec.end(), auction_item);
	if (it == vec.end())
		return 0;

	return it - vec.begin();
}

bool CPythonAuction::remove_item(BYTE container_type, DWORD item_id)
{
	auto auction_item = find_item(container_type, item_id);
	if (!auction_item)
		return false;

	auto& vec = _items[auction_item->container_type];
	auto vec_it = std::find(vec.begin(), vec.end(), auction_item);
	if (vec_it != vec.end())
		vec.erase(vec_it);

	_items_by_slot[auction_item->container_type].erase(auction_item->shop_item.item().cell().cell());
	_items_by_id[auction_item->container_type].erase(item_id);

	return true;
}

void CPythonAuction::remove_item_by_index(BYTE container_type, DWORD item_id)
{
	auto shop_item = get_item(container_type, item_id);
	if (shop_item)
		remove_item(container_type, shop_item->item().id());
}

void CPythonAuction::clear_items(BYTE container_type)
{
	if (container_type >= ITEM_CONTAINER_MAX_NUM)
		return;

	for (auto& auction_item : _items[container_type])
		_items_by_id[auction_item->container_type].erase(auction_item->shop_item.item().id());

	_items[container_type].clear();
}

void CPythonAuction::append_shop_creating_item(::TItemPos inventory_pos, WORD display_pos, uint64_t price)
{
	auto& shop_item = _shop_creating_items[inventory_pos];
	auto item = shop_item.mutable_item();

	shop_item.set_display_pos(display_pos);
	*item->mutable_cell() = inventory_pos;
	item->set_price(price);
}

const network::TShopItemTable* CPythonAuction::get_shop_creating_item(::TItemPos inventory_pos) const
{
	auto it = _shop_creating_items.find(inventory_pos);
	if (it == _shop_creating_items.end())
		return nullptr;

	return &it->second;
}

std::vector<network::TShopItemTable> CPythonAuction::build_shop_creating_items()
{
	std::vector<network::TShopItemTable> ret;

	for (auto& pair : _shop_creating_items)
		ret.push_back(std::move(pair.second));
	
	_shop_creating_items.clear();
	return ret;
}

void CPythonAuction::remove_shop_creating_item(::TItemPos inventory_pos)
{
	_shop_creating_items.erase(inventory_pos);
}

// -------------------------------------------------------------------------------------------

PyObject * auctionSetContainerType(PyObject * poSelf, PyObject * poArgs)
{
	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType) || iType < 0 || iType >= CPythonAuction::ITEM_CONTAINER_MAX_NUM)
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auction.select_container_type(iType);

	return Py_BuildNone();
}

PyObject * auctionGetMaxPageCount(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("i", auction.get_max_page_count());
}

PyObject * auctionGetItemMaxCount(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		iType = auction.get_selected_container_type();

	return Py_BuildValue("i", auction.get_item_count(iType));
}

PyObject * auctionGetItemDataByID(PyObject* poSelf, PyObject* poArgs)
{
	int iType;;
	if (!PyTuple_GetInteger(poArgs, 0, &iType))
		return Py_BadArgument();
	int iItemID;
	if (!PyTuple_GetInteger(poArgs, 1, &iItemID))
		return Py_BadArgument();

	auto item = CPythonAuction::instance().find_item(iType, iItemID);
	if (!item)
		return Py_BuildValue("ii", -1, -1);

	auto type = item->container_type;
	auto index = CPythonAuction::instance().get_container_index_by_id(type, iItemID);

	return Py_BuildValue("ii", type, index);
}

PyObject * auctionGetItemIndexByCell(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iCell;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iCell))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iCell))
			return Py_BadArgument();
	}

	auto item_data = auction.find_item_by_cell(iType, iCell);
	if (!item_data)
		return Py_BuildException("invalid item cell");

	return Py_BuildValue("i", auction.get_item_index(iType, item_data));
}

PyObject * auctionGetItemAuctionType(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->auction_type());
}

PyObject * auctionRemoveItem(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auction.remove_item_by_index(iType, iIndex);

	return Py_BuildNone();
}

PyObject * auctionGetGold(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("L", static_cast<long long>(auction.get_owned_gold()));
}

PyObject* auctionHasMyShop(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("b", auction.has_owned_shop());
}

PyObject* auctionIsMyShopLoaded(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("b", auction.is_owned_shop_loaded());
}

PyObject* auctionGetMyShopName(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("s", auction.get_owned_shop_name().c_str());
}

PyObject* auctionGetMyShopGold(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("L", static_cast<long long>(auction.get_owned_shop_gold()));
}

PyObject* auctionGetMyShopTimeout(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	auto timeout = auction.get_owned_shop_timeout();
	auto current_time = CPythonApplication::Instance().GetServerTimeStamp();

	if (timeout == 0)
		return Py_BuildValue("i", -1);

	return Py_BuildValue("i", current_time >= timeout ? 0 : timeout - current_time);
}

PyObject* auctionGetGuestShopName(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	return Py_BuildValue("s", auction.get_guest_shop_name().c_str());
}

PyObject * auctionGetItemInfo(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("iiL", item_data->item().vnum(), item_data->item().count(), static_cast<long long>(item_data->item().price()));
}

PyObject * auctionGetItemID(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().id());
}

PyObject * auctionGetItemOwnerID(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().owner());
}

PyObject * auctionGetItemVnum(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().vnum());
}

PyObject * auctionGetItemCount(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().count());
}

PyObject* auctionGetItemCell(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().cell().cell());
}

PyObject * auctionGetItemSocket(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex, iSubIndex;
	if (PyTuple_Size(poArgs) == 3)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex) || !PyTuple_GetInteger(poArgs, 2, &iSubIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex) || !PyTuple_GetInteger(poArgs, 1, &iSubIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().sockets(iSubIndex));
}

PyObject* auctionGetItemAttribute(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex, iSubIndex;
	if (PyTuple_Size(poArgs) == 3)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex) || !PyTuple_GetInteger(poArgs, 2, &iSubIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex) || !PyTuple_GetInteger(poArgs, 1, &iSubIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("ii", item_data->item().attributes(iSubIndex).type(), item_data->item().attributes(iSubIndex).value());
}

PyObject * auctionGetItemAttrType(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex, iSubIndex;
	if (PyTuple_Size(poArgs) == 3)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex) || !PyTuple_GetInteger(poArgs, 2, &iSubIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex) || !PyTuple_GetInteger(poArgs, 1, &iSubIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().attributes(iSubIndex).type());
}

PyObject * auctionGetItemAttrValue(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex, iSubIndex;
	if (PyTuple_Size(poArgs) == 3)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex) || !PyTuple_GetInteger(poArgs, 2, &iSubIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex) || !PyTuple_GetInteger(poArgs, 1, &iSubIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->item().attributes(iSubIndex).value());
}

PyObject * auctionGetItemOwnerName(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("s", item_data->owner_name().c_str());
}

PyObject * auctionGetItemPrice(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("L", (long long) item_data->item().price());
}

PyObject * auctionGetItemInsertionTime(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->insertion_time());
}

PyObject * auctionGetItemTimeoutTime(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();

	int iType, iIndex;
	if (PyTuple_Size(poArgs) == 2)
	{
		if (!PyTuple_GetInteger(poArgs, 0, &iType) || !PyTuple_GetInteger(poArgs, 1, &iIndex))
			return Py_BadArgument();
	}
	else
	{
		iType = auction.get_selected_container_type();
		if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
			return Py_BadArgument();
	}

	auto item_data = auction.get_item(iType, iIndex);
	if (!item_data)
		return Py_BuildException("invalid item index");

	return Py_BuildValue("i", item_data->timeout_time());
}

PyObject * auctionGetShopHistoryCount(PyObject* poSelf, PyObject* poArgs)
{
	int count = CPythonAuction::instance().get_shop_history_size();
	return Py_BuildValue("i", count);
}

PyObject* auctionGetShopHistory(PyObject* poSelf, PyObject* poArgs)
{
	int index;
	if (!PyTuple_GetInteger(poArgs, 0, &index))
		return Py_BadArgument();

	auto elem = CPythonAuction::instance().get_shop_history(index);
	if (!elem)
		return Py_BuildValue("iLsi", 0, 0LL, "", 0);

	return Py_BuildValue("iLsi", elem->vnum(), static_cast<long long>(elem->price()), elem->buyer().c_str(), elem->date());
}

PyObject * auctionClearSearchData(PyObject * poSelf, PyObject * poArgs)
{
	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();
	options.Clear();

	options.set_sort_order1(CPythonAuction::SEARCH_ORDER_NAME);
	options.set_sort_order2(CPythonAuction::SEARCH_ORDER_PRICE);
	options.set_sort_order3(CPythonAuction::SEARCH_ORDER_DATE);

	auto basic_options = options.mutable_basic_data();
	basic_options->set_socket0(-1);
	basic_options->set_value0(-1);

	return Py_BuildNone();
}

PyObject * auctionSetSearchType(PyObject * poSelf, PyObject * poArgs)
{
	int iValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();
	
	options.set_search_type(iValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchText(PyObject * poSelf, PyObject * poArgs)
{
	char* szValue;
	if (!PyTuple_GetString(poArgs, 0, &szValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& basic_options = auction.get_search_options();
	
	basic_options.set_search_text(szValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchSocket0(PyObject * poSelf, PyObject * poArgs)
{
	long lSocket0;
	if (!PyTuple_GetLong(poArgs, 0, &lSocket0))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& basic_options = auction.get_search_options();
	basic_options.set_socket0(lSocket0);

	return Py_BuildNone();
}

PyObject * auctionSetSearchValue0(PyObject * poSelf, PyObject * poArgs)
{
	int iValue0;
	if (!PyTuple_GetInteger(poArgs, 0, &iValue0))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& basic_options = auction.get_search_options();

	basic_options.set_value0(iValue0);

	return Py_BuildNone();
}

PyObject * auctionSetSearchAntiFlag(PyObject * poSelf, PyObject * poArgs)
{
	int iValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& basic_options = auction.get_search_options();

	basic_options.set_include_anti_flag(iValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchItemTypes(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * poItemTypes;
	if (!PyTuple_GetObject(poArgs, 0, &poItemTypes))
		return Py_BadArgument();

	std::vector<BYTE> vec_bItemTypes;

	// single item type
	if (PyLong_Check(poItemTypes) || PyInt_Check(poItemTypes))
	{
		vec_bItemTypes.push_back(PyLong_AsLong(poItemTypes));
	}
	// many item types
	else if (PyTuple_Check(poItemTypes))
	{
		int iCount = PyTuple_Size(poItemTypes);
		for (int i = 0; i < iCount; ++i)
		{
			BYTE bItemType;
			if (!PyTuple_GetInteger(poItemTypes, i, &bItemType))
				return Py_BuildException("invalid tuple element %d of auction ItemTypes", i + 1);

			vec_bItemTypes.push_back(bItemType);
		}
	}
	else
		return Py_BuildException("invalid argument type for auction ItemTypes");

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& basic_options = auction.get_search_options();

	basic_options.clear_types();
	for (auto& type : vec_bItemTypes)
		basic_options.add_types()->set_type(type);

	return Py_BuildNone();
}

PyObject * auctionSetSearchItemSubTypes(PyObject * poSelf, PyObject * poArgs)
{
	PyObject * poItemSubTypes;
	if (!PyTuple_GetObject(poArgs, 0, &poItemSubTypes))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& basic_options = auction.get_search_options();
	std::vector<BYTE> vec_bItemSubTypes;

	// sub types for single item type
	if (PyLong_Check(poItemSubTypes))
	{
		basic_options.mutable_types(0)->add_sub_types(PyLong_AsLong(poItemSubTypes));
	}
	// sub types for many item types
	else if (PyTuple_Check(poItemSubTypes))
	{
		int iCount = PyTuple_Size(poItemSubTypes);
		for (int i = 0; i < iCount; ++i)
		{
			PyObject * poItemCur;
			if (!PyTuple_GetObject(poItemSubTypes, i, &poItemCur))
				return Py_BuildException("invalid tuple element %d of auction ItemSubTypes", i);

			// single item sub type for current item type
			if (PyLong_Check(poItemCur))
			{
				vec_bItemSubTypes.push_back(PyLong_AsLong(poItemCur));
			}
			// many item sub types for current item type
			else if (PyTuple_Check(poItemCur))
			{
				int iCurCount = PyTuple_Size(poItemCur);
				for (int j = 0; j < iCurCount; ++j)
				{
					BYTE bSubType;
					if (!PyTuple_GetInteger(poItemCur, j, &bSubType))
						return Py_BuildException("invalid subtuple element at main tuple %d sub tuple %d for auction ItemSubTypes", i, j);

					vec_bItemSubTypes.push_back(bSubType);
				}
			}
			else
				return Py_BuildException("invalid argument type at tuple element %d for auction ItemSubTypes", i);

			for (auto& sub_type : vec_bItemSubTypes)
				basic_options.mutable_types(i)->add_sub_types(sub_type);

			vec_bItemSubTypes.clear();
		}
	}
	else
		return Py_BuildException("invalid argument type for auction ItemSubTypes");

	return Py_BuildNone();
}

PyObject * auctionSetSearchLevelMin(PyObject * poSelf, PyObject * poArgs)
{
	int iValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	options.set_level_min(iValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchLevelMax(PyObject * poSelf, PyObject * poArgs)
{
	int iValue;
	if (!PyTuple_GetInteger(poArgs, 0, &iValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	options.set_level_max(iValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchPriceMin(PyObject * poSelf, PyObject * poArgs)
{
	long long llValue;
	if (!PyTuple_GetLongLong(poArgs, 0, &llValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	options.set_price_min(llValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchPriceMax(PyObject * poSelf, PyObject * poArgs)
{
	long long llValue;
	if (!PyTuple_GetLongLong(poArgs, 0, &llValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	options.set_price_max(llValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchOnlyAHItem(PyObject * poSelf, PyObject * poArgs)
{
	bool bValue;
	if (!PyTuple_GetBoolean(poArgs, 0, &bValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();
	
	options.set_is_only_auction_item(bValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchOnlySelfItem(PyObject * poSelf, PyObject * poArgs)
{
	bool bValue;
	if (!PyTuple_GetBoolean(poArgs, 0, &bValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	options.set_is_only_self_item(bValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchSinglePriceOrder(PyObject * poSelf, PyObject * poArgs)
{
	bool bValue;
	if (!PyTuple_GetBoolean(poArgs, 0, &bValue))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	options.set_is_single_price_order(bValue);

	return Py_BuildNone();
}

PyObject * auctionSetSearchSortOrderType(PyObject * poSelf, PyObject * poArgs)
{
	int iIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &iIndex))
		return Py_BadArgument();
	BYTE bValue;
	if (!PyTuple_GetInteger(poArgs, 1, &bValue))
		return Py_BadArgument();
	bool bIsAsc;
	if (!PyTuple_GetBoolean(poArgs, 2, &bIsAsc))
		return Py_BadArgument();

	CPythonAuction& auction = CPythonAuction::Instance();
	auto& options = auction.get_extended_search_options();

	switch (iIndex)
	{
	case 0:
		options.set_sort_order1(bValue);
		options.set_sort_order1_reversed(!bIsAsc);
		break;
	case 1:
		options.set_sort_order2(bValue);
		options.set_sort_order2_reversed(!bIsAsc);
		break;
	case 2:
		options.set_sort_order3(bValue);
		options.set_sort_order3_reversed(!bIsAsc);
		break;
	}
	
	return Py_BuildNone();
}

PyObject * auctionSendInsertItemPacket(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bInventoryType;
	if (!PyTuple_GetInteger(poArgs, 0, &bInventoryType))
		return Py_BadArgument();
	WORD wCell;
	if (!PyTuple_GetInteger(poArgs, 1, &wCell))
		return Py_BadArgument();
	int iShopCell;
	if (!PyTuple_GetInteger(poArgs, 2, &iShopCell))
		return Py_BadArgument();
	long long llGold;
	if (!PyTuple_GetLongLong(poArgs, 3, &llGold))
		return Py_BadArgument();

	// send packet
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionInsertItemPacket(::TItemPos(bInventoryType, wCell), ::TItemPos(iShopCell >= 0 ? AUCTION_SHOP : AUCTION, MAX(0, iShopCell)), llGold);

	return Py_BuildNone();
}

PyObject * auctionSendTakeItemPacket(PyObject * poSelf, PyObject * poArgs)
{
	int iItemID;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemID))
		return Py_BadArgument();
	WORD inventory_pos;
	if (!PyTuple_GetInteger(poArgs, 1, &inventory_pos))
		inventory_pos = 0;

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionTakeItemPacket(iItemID, inventory_pos);


	return Py_BuildNone();
}

PyObject * auctionSendBuyItemPacket(PyObject * poSelf, PyObject * poArgs)
{
	int iItemID;
	if (!PyTuple_GetInteger(poArgs, 0, &iItemID))
		return Py_BadArgument();
	long long llGold;
	if (!PyTuple_GetLongLong(poArgs, 1, &llGold))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionBuyItemPacket(iItemID, llGold);

	return Py_BuildNone();
}

PyObject* auctionSendTakeGoldPacket(PyObject* poSelf, PyObject* poArgs)
{
	long long llGold;
	if (!PyTuple_GetLongLong(poArgs, 0, &llGold))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionTakeGoldPacket(llGold);

	return Py_BuildNone();
}

PyObject * auctionSendSearchPacket(PyObject * poSelf, PyObject * poArgs)
{
	BYTE bPageIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &bPageIndex))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionExtendedSearchItemsPacket(bPageIndex, CPythonAuction::instance().get_extended_search_options());

	return Py_BuildNone();
}

PyObject * auctionSendBasicSearchPacket(PyObject* poSelf, PyObject* poArgs)
{
	BYTE bPageIndex;
	if (!PyTuple_GetInteger(poArgs, 0, &bPageIndex))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionSearchItemsPacket(bPageIndex, CPythonAuction::instance().get_search_options());

	return Py_BuildNone();
}

PyObject * auctionSendMarkShopPacket(PyObject * poSelf, PyObject * poArgs)
{
	int item_id;
	if (!PyTuple_GetInteger(poArgs, 0, &item_id))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionMarkShopPacket(item_id);

	return Py_BuildNone();
}

PyObject* auctionSendRequestShopViewPacket(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionRequestShopView();

	return Py_BuildNone();
}

PyObject* auctionSendOpenShopPacket(PyObject* poSelf, PyObject* poArgs)
{
	char* name;
	BYTE model, style;
	if (!PyTuple_GetString(poArgs, 0, &name) || !PyTuple_GetInteger(poArgs, 1, &model) || !PyTuple_GetInteger(poArgs, 2, &style))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionOpenShopPacket(name, -1.0f, -1.0f, -1.0f, CPythonAuction::instance().build_shop_creating_items(), model, style);

	return Py_BuildNone();
}

PyObject* auctionSendTakeShopGoldPacket(PyObject* poSelf, PyObject* poArgs)
{
	long long llGold;
	if (!PyTuple_GetLongLong(poArgs, 0, &llGold))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionTakeShopGoldPacket(llGold);

	return Py_BuildNone();
}

PyObject* auctionSendShopRenewPacket(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionRenewShopPacket();

	return Py_BuildNone();
}

PyObject* auctionSendShopClosePacket(PyObject* poSelf, PyObject* poArgs)
{
	auto item_count = CPythonAuction::instance().get_item_count(CPythonAuction::ITEM_CONTAINER_OWNED_SHOP);
	auto timeout = CPythonAuction::instance().get_owned_shop_timeout();
	auto current_time = CPythonApplication::Instance().GetServerTimeStamp();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionCloseShopPacket(item_count > 0 && (timeout == 0 || timeout > current_time));

	return Py_BuildNone();
}

PyObject* auctionSendShopGuestCancelPacket(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionShopGuestCancelPacket();

	return Py_BuildNone();
}

PyObject* auctionSendShopRequestHistoryPacket(PyObject* poSelf, PyObject* poArgs)
{
	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendAuctionRequestShopHistoryPacket();

	return Py_BuildNone();
}

PyObject* auctionSendRequestAveragePricePacket(PyObject* poSelf, PyObject* poArgs)
{
	BYTE requestor;
	if (!PyTuple_GetInteger(poArgs, 0, &requestor))
		return Py_BadArgument();
	int vnum, count;
	if (!PyTuple_GetInteger(poArgs, 1, &vnum) || !PyTuple_GetInteger(poArgs, 2, &count))
		return Py_BadArgument();

	CPythonNetworkStream& rkNetStream = CPythonNetworkStream::Instance();
	rkNetStream.SendRequestAveragePricePacket(requestor, vnum, count);

	return Py_BuildNone();
}

PyObject* auctionClearShopCreatingItems(PyObject* poSelf, PyObject* poArgs)
{
	CPythonAuction::Instance().clear_shop_creating_items();
	return Py_BuildNone();
}

PyObject* auctionAddShopCreatingItem(PyObject* poSelf, PyObject* poArgs)
{
	BYTE window_type;
	WORD inventory_slot;
	if (!PyTuple_GetInteger(poArgs, 0, &window_type) || !PyTuple_GetInteger(poArgs, 1, &inventory_slot))
		return Py_BadArgument();
	WORD display_slot;
	LONGLONG price;
	if (!PyTuple_GetInteger(poArgs, 2, &display_slot) || !PyTuple_GetLongLong(poArgs, 3, &price))
		return Py_BadArgument();

	CPythonAuction::Instance().append_shop_creating_item(::TItemPos(window_type, inventory_slot), display_slot, price);
	return Py_BuildNone();
}

PyObject* auctionGetShopCreatingItemPrice(PyObject* poSelf, PyObject* poArgs)
{
	BYTE window_type;
	WORD inventory_slot;
	if (!PyTuple_GetInteger(poArgs, 0, &window_type) || !PyTuple_GetInteger(poArgs, 1, &inventory_slot))
		return Py_BadArgument();

	auto item = CPythonAuction::instance().get_shop_creating_item(::TItemPos(window_type, inventory_slot));
	return Py_BuildValue("L", item ? static_cast<long long>(item->item().price()) : 0LL);
}

PyObject* auctionDelShopCreatingItem(PyObject* poSelf, PyObject* poArgs)
{
	BYTE window_type;
	WORD inventory_slot;
	if (!PyTuple_GetInteger(poArgs, 0, &window_type) || !PyTuple_GetInteger(poArgs, 1, &inventory_slot))
		return Py_BadArgument();

	CPythonAuction::instance().remove_shop_creating_item(::TItemPos(window_type, inventory_slot));
	return Py_BuildNone();
}

void initAuction()
{
	static PyMethodDef s_methods[] =
	{
		{ "SetContainerType",			auctionSetContainerType,			METH_VARARGS },

		{ "GetMaxPageCount",			auctionGetMaxPageCount,				METH_VARARGS },
		{ "GetItemMaxCount",			auctionGetItemMaxCount,				METH_VARARGS },

		{ "RemoveItem",					auctionRemoveItem,					METH_VARARGS },
		
		{ "GetGold",					auctionGetGold,						METH_VARARGS },

		{ "HasMyShop",					auctionHasMyShop,					METH_VARARGS },
		{ "IsMyShopLoaded",				auctionIsMyShopLoaded,				METH_VARARGS },
		{ "GetMyShopName",				auctionGetMyShopName,				METH_VARARGS },
		{ "GetMyShopGold",				auctionGetMyShopGold,				METH_VARARGS },
		{ "GetMyShopTimeout",			auctionGetMyShopTimeout,			METH_VARARGS },

		{ "GetGuestShopName",			auctionGetGuestShopName,			METH_VARARGS },

		{ "GetItemDataByID",			auctionGetItemDataByID,				METH_VARARGS },
		{ "GetItemIndexByCell",			auctionGetItemIndexByCell,			METH_VARARGS },
		{ "GetItemAuctionType",			auctionGetItemAuctionType,			METH_VARARGS },
		{ "GetItemInfo",				auctionGetItemInfo,					METH_VARARGS },
		{ "GetItemID",					auctionGetItemID,					METH_VARARGS },
		{ "GetItemOwnerID",				auctionGetItemOwnerID,				METH_VARARGS },
		{ "GetItemVnum",				auctionGetItemVnum,					METH_VARARGS },
		{ "GetItemCount",				auctionGetItemCount,				METH_VARARGS },
		{ "GetItemCell",				auctionGetItemCell,					METH_VARARGS },
		{ "GetItemSocket",				auctionGetItemSocket,				METH_VARARGS },
		{ "GetItemAttribute",			auctionGetItemAttribute,			METH_VARARGS },
		{ "GetItemAttrType",			auctionGetItemAttrType,				METH_VARARGS },
		{ "GetItemAttrValue",			auctionGetItemAttrValue,			METH_VARARGS },
		{ "GetItemOwnerName",			auctionGetItemOwnerName,			METH_VARARGS },
		{ "GetItemPrice",				auctionGetItemPrice,				METH_VARARGS },
		{ "GetItemInsertionTime",		auctionGetItemInsertionTime,		METH_VARARGS },
		{ "GetItemTimeoutTime",			auctionGetItemTimeoutTime,			METH_VARARGS },

		{ "GetShopHistoryCount",		auctionGetShopHistoryCount,			METH_VARARGS },
		{ "GetShopHistory",				auctionGetShopHistory,				METH_VARARGS },

		{ "ClearSearchData",			auctionClearSearchData,				METH_VARARGS },
		{ "SetSearchType",				auctionSetSearchType,				METH_VARARGS },
		{ "SetSearchText",				auctionSetSearchText,				METH_VARARGS },
		{ "SetSearchSocket0",			auctionSetSearchSocket0,			METH_VARARGS },
		{ "SetSearchValue0",			auctionSetSearchValue0,				METH_VARARGS },
		{ "SetSearchAntiFlag",			auctionSetSearchAntiFlag,			METH_VARARGS },
		{ "SetSearchItemTypes",			auctionSetSearchItemTypes,			METH_VARARGS },
		{ "SetSearchItemSubTypes",		auctionSetSearchItemSubTypes,		METH_VARARGS },
		{ "SetSearchLevelMin",			auctionSetSearchLevelMin,			METH_VARARGS },
		{ "SetSearchLevelMax",			auctionSetSearchLevelMax,			METH_VARARGS },
		{ "SetSearchPriceMin",			auctionSetSearchPriceMin,			METH_VARARGS },
		{ "SetSearchPriceMax",			auctionSetSearchPriceMax,			METH_VARARGS },
		{ "SetSearchOnlyAHItem",		auctionSetSearchOnlyAHItem,			METH_VARARGS },
		{ "SetSearchOnlySelfItem",		auctionSetSearchOnlySelfItem,		METH_VARARGS },
		{ "SetSearchSinglePriceOrder",	auctionSetSearchSinglePriceOrder,	METH_VARARGS },
		{ "SetSearchSortOrderType",		auctionSetSearchSortOrderType,		METH_VARARGS },

		{ "SendInsertItemPacket",		auctionSendInsertItemPacket,		METH_VARARGS },
		{ "SendTakeItemPacket",			auctionSendTakeItemPacket,			METH_VARARGS },
		{ "SendBuyItemPacket",			auctionSendBuyItemPacket,			METH_VARARGS },
		{ "SendTakeGoldPacket",			auctionSendTakeGoldPacket,			METH_VARARGS },
		{ "SendSearchPacket",			auctionSendSearchPacket,			METH_VARARGS },
		{ "SendBasicSearchPacket",		auctionSendBasicSearchPacket,		METH_VARARGS },
		{ "SendMarkShopPacket",			auctionSendMarkShopPacket,			METH_VARARGS },
		{ "SendRequestShopViewPacket",	auctionSendRequestShopViewPacket,	METH_VARARGS },
		{ "SendOpenShopPacket",			auctionSendOpenShopPacket,			METH_VARARGS },
		{ "SendTakeShopGoldPacket",		auctionSendTakeShopGoldPacket,		METH_VARARGS },
		{ "SendShopRenewPacket",		auctionSendShopRenewPacket,			METH_VARARGS },
		{ "SendShopClosePacket",		auctionSendShopClosePacket,			METH_VARARGS },
		{ "SendShopGuestCancelPacket",	auctionSendShopGuestCancelPacket,	METH_VARARGS },
		{ "SendShopRequestHistoryPacket",	auctionSendShopRequestHistoryPacket,	METH_VARARGS },
		{ "SendRequestAveragePricePacket",	auctionSendRequestAveragePricePacket,	METH_VARARGS },

		{ "ClearShopCreatingItems",		auctionClearShopCreatingItems,		METH_VARARGS },
		{ "AddShopCreatingItem",		auctionAddShopCreatingItem,			METH_VARARGS },
		{ "GetShopCreatingItemPrice",	auctionGetShopCreatingItemPrice,	METH_VARARGS },
		{ "DelShopCreatingItem",		auctionDelShopCreatingItem,			METH_VARARGS },

		{ NULL, NULL, NULL },
	};

	PyObject * poModule = Py_InitModule("auction", s_methods);

	PyModule_AddIntConstant(poModule, "ITEMS_PER_PAGE",			CPythonAuction::ITEMS_PER_PAGE);
	PyModule_AddIntConstant(poModule, "DEFAULT_ITEM_DURATION",	CPythonAuction::DEFAULT_ITEM_DURATION);
	PyModule_AddIntConstant(poModule, "SEARCH_TEXT_MAX_LEN",	CPythonAuction::SEARCH_TEXT_MAX_LEN);
	PyModule_AddIntConstant(poModule, "ITEMS_MAX_PAGE",			CPythonAuction::ITEMS_MAX_PAGE);
	PyModule_AddIntConstant(poModule, "SHOP_CREATE_COST",		CPythonAuction::SHOP_CREATE_COST);
	PyModule_AddIntConstant(poModule, "SHOP_SLOT_COUNT_X",		CPythonAuction::SHOP_SLOT_COUNT_X);
	PyModule_AddIntConstant(poModule, "SHOP_SLOT_COUNT_Y",		CPythonAuction::SHOP_SLOT_COUNT_Y);
	PyModule_AddIntConstant(poModule, "SHOP_SLOT_COUNT",		CPythonAuction::SHOP_SLOT_COUNT);
	PyModule_AddIntConstant(poModule, "TAX_AUCTION_OWNER",		CPythonAuction::TAX_AUCTION_OWNER);
	PyModule_AddIntConstant(poModule, "TAX_SHOP_OWNER",			CPythonAuction::TAX_SHOP_OWNER);
	PyModule_AddIntConstant(poModule, "TAX_SHOP_PLAYER",		CPythonAuction::TAX_SHOP_PLAYER);

	PyModule_AddIntConstant(poModule, "TYPE_AUCTION",			CPythonAuction::TYPE_AUCTION);
	PyModule_AddIntConstant(poModule, "TYPE_SHOP",				CPythonAuction::TYPE_SHOP);
	
	PyModule_AddIntConstant(poModule, "SEARCH_TYPE_NONE",		CPythonAuction::SEARCH_TYPE_NONE);
	PyModule_AddIntConstant(poModule, "SEARCH_TYPE_ITEM",		CPythonAuction::SEARCH_TYPE_ITEM);
	PyModule_AddIntConstant(poModule, "SEARCH_TYPE_PLAYER",		CPythonAuction::SEARCH_TYPE_PLAYER);
	PyModule_AddIntConstant(poModule, "SEARCH_TYPE_ITEM_STRICT",CPythonAuction::SEARCH_TYPE_ITEM_STRICT);
	PyModule_AddIntConstant(poModule, "SEARCH_TYPE_MAX_NUM",	CPythonAuction::SEARCH_TYPE_MAX_NUM);

	PyModule_AddIntConstant(poModule, "SEARCH_ORDER_NAME",		CPythonAuction::SEARCH_ORDER_NAME);
	PyModule_AddIntConstant(poModule, "SEARCH_ORDER_PRICE",		CPythonAuction::SEARCH_ORDER_PRICE);
	PyModule_AddIntConstant(poModule, "SEARCH_ORDER_DATE",		CPythonAuction::SEARCH_ORDER_DATE);
	PyModule_AddIntConstant(poModule, "SEARCH_ORDER_MAX_NUM",	CPythonAuction::SEARCH_ORDER_MAX_NUM);

	PyModule_AddIntConstant(poModule, "CONTAINER_SEARCH",		CPythonAuction::ITEM_CONTAINER_SEARCH);
	PyModule_AddIntConstant(poModule, "CONTAINER_SHOP",			CPythonAuction::ITEM_CONTAINER_SHOP);
	PyModule_AddIntConstant(poModule, "CONTAINER_OWNED",		CPythonAuction::ITEM_CONTAINER_OWNED);
	PyModule_AddIntConstant(poModule, "CONTAINER_OWNED_SHOP",	CPythonAuction::ITEM_CONTAINER_OWNED_SHOP);
}
#endif
