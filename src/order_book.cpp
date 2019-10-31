#include "order_book.hpp"

#include <iostream>
#include <math.h>
#include <sstream>
#include <string>

LimitOrder::LimitOrder(long long orderID, bool isBuyOrder_, long quantity_, double price_)
    : id(orderID), isBuyOrder(isBuyOrder_), price(price_), quantity(quantity_), left(quantity_)
{
    timestamp = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    status = OrderStatus::open;
}

bool LimitOrder::operator<(const LimitOrder &other) const
{
    if (timestamp == other.timestamp)
    {
        return id < other.id;
    }

    return timestamp < other.timestamp;
}

bool PriceLevel::cancel(long long orderID, shared_mutex &m)
{
    set<LimitOrder> &s = this->orders;
    std::unique_lock lock(m);

    for (auto it = s.begin(); it != s.end(); ++it)
    {
        if (it->id == orderID)
        {
            if (s.size() == 0)
            {
                this->quantity = 0;
            }
            else
            {
                this->quantity -= it->quantity;
            }
            s.erase(it);
            return true;
        }
    }
    return false;
}

void PriceLevel::insert(LimitOrder order)
{
    this->quantity += order.left;
    this->orders.insert(move(order));
}

int PriceLevel::nItems()
{
    return this->orders.size();
}

int PriceLevel::pos(long long orderID)
{
    int i = 0;
    for (set<LimitOrder>::iterator it = orders.begin(); it != orders.end(); it++)
    {
        if (it->id == orderID)
        {
            return i;
        }
        i++;
    }
    return -1;
}

void PriceLevel::update(LimitOrder &&order, shared_mutex &m)
{
    set<LimitOrder> &s = this->orders;
    std::unique_lock lock(m);
    for (auto it = s.begin(); it != s.end(); ++it)
    {
        if (it->id == order.id)
        {
            this->quantity += order.quantity - it->quantity;
            s.erase(it);
            break;
        }
    }
    s.insert(move(order));
}

OrderBook::OrderBook(double tickSize_, double precision_)
    : tickSize(tickSize_), precision(precision_)
{
}

bool OrderBook::add(LimitOrder &&order)
{
    long n = int(order.price / tickSize);
    double rem = fmod(order.price, tickSize);
    if (rem < tickSize * precision || tickSize - rem < tickSize * precision)
    {
        // Close enough, aligning value
        order.price = n * tickSize;
    }
    else
    {
        return false;
    }

    {
        std::unique_lock lock(ordersMutex);
        auto p = orders.insert({order.id, order});
        if (!p.second)
        {
            return false;
        }
    }

    // First try matching the order against order book
    match(order);

    // Store limit order after matching updates
    {
        std::unique_lock lock(ordersMutex);
        orders.at(order.id) = order;
    }

    // If order not fully matched, add in order book
    if (0 < order.left)
    {
        if (order.isBuyOrder)
        {
            std::unique_lock lock(buyMutex);
            buyOrders[order.price].insert(order);
        }
        else
        {
            std::unique_lock lock(sellMutex);
            sellOrders[order.price].insert(order);
        }
    }

    return true;
}

bool OrderBook::amend(long long orderID, long quantity)
{
    map<long long, LimitOrder>::iterator it;

    std::unique_lock lock(ordersMutex);
    it = orders.find(orderID);
    if (it == orders.end() || it->second.status == OrderStatus::cancelled || it->second.status == OrderStatus::executed)
    {
        return false;
    }

    long delta = quantity - it->second.quantity;
    if (it->second.left + delta < 0)
    {
        return false;
    }

    if (0 < delta)
    {
        it->second.timestamp = chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now().time_since_epoch());
    }
    it->second.quantity = quantity;
    it->second.left += delta;

    LimitOrder order = it->second;
    if (order.isBuyOrder)
    {
        buyOrders[order.price].update(move(order), buyMutex);
    }
    else
    {
        sellOrders[order.price].update(move(order), sellMutex);
    }
    return true;
}

bool OrderBook::cancel(long long orderID)
{
    bool isBuyOrder;
    double price;
    {
        map<long long, LimitOrder>::iterator it;

        std::unique_lock lock(ordersMutex);
        it = orders.find(orderID);
        if (it == orders.end())
        {
            return false;
        }
        it->second.status = OrderStatus::cancelled;
        isBuyOrder = it->second.isBuyOrder;
        price = it->second.price;
    }

    bool success;
    if (isBuyOrder)
    {
        success = buyOrders[price].cancel(orderID, buyMutex);
        if (buyOrders[price].quantity == 0)
        {
            buyOrders.erase(price);
        }
    }
    else
    {
        success = sellOrders[price].cancel(orderID, buyMutex);
        if (sellOrders[price].quantity == 0)
        {
            sellOrders.erase(price);
        }
    }
    return success;
}

// Fill at much as possible at a given price level
void OrderBook::fill(PriceLevel &pl, LimitOrder &order)
{
    set<LimitOrder>::iterator it;
    std::unique_lock lock(ordersMutex);

    for (it = pl.orders.begin(); it != pl.orders.end(); it++)
    {
        if (order.left <= it->left)
        {
            pl.quantity -= order.left;
            it->left -= order.left;
            orders.at(it->id).left = it->left;
            if (0 < it->left)
            {
                orders.at(it->id).status = OrderStatus::partial;
                it->status = OrderStatus::partial;
            }
            else
            {
                orders.at(it->id).status = OrderStatus::executed;
                pl.orders.erase(it);
            }
            order.left = 0;
            return;
        }
        else
        {
            LimitOrder oo = pl.orders.extract(it).value();
            pl.quantity -= oo.left;
            order.left -= oo.left;
            orders.at(oo.id).status = OrderStatus::executed;
            orders.at(oo.id).left = 0;
        }
    }
}

//
void OrderBook::match(LimitOrder &order)
{
    if (order.isBuyOrder)
    {
        std::unique_lock lock(sellMutex);
        map<double, PriceLevel>::iterator it;
        for (it = sellOrders.begin(); it != sellOrders.end();)
        {
            if (order.price < it->first)
            {
                return;
            }
            order.status = OrderStatus::partial;
            fill(it->second, order);
            if (it->second.nItems() == 0)
            {
                sellOrders.erase(it++);
            }
            else
            {
                ++it;
            }
            if (order.left == 0)
            {
                order.status = OrderStatus::executed;
                return;
            }
        }
    }
    else
    {
        std::unique_lock lock(buyMutex);
        map<double, PriceLevel>::reverse_iterator it;
        for (it = buyOrders.rbegin(); it != buyOrders.rend();)
        {
            if (it->first < order.price)
            {
                return;
            }
            order.status = OrderStatus::partial;
            fill(it->second, order);
            if (it->second.nItems() == 0)
            {
                // Idiomatic way to erase items and while keeping a valid reverse iterator
                it = decltype(it){buyOrders.erase(next(it).base())};
            }
            else
            {
                ++it;
            }
            if (order.left == 0)
            {
                order.status = OrderStatus::executed;
                return;
            }
        }
    }
}

int OrderBook::pos(LimitOrder &order)
{
    map<double, PriceLevel> *m = order.isBuyOrder ? &buyOrders : &sellOrders;
    shared_mutex *mutex = order.isBuyOrder ? &buyMutex : &sellMutex;
    std::shared_lock lock(*mutex);
    PriceLevel &pl = m->at(order.price);
    return pl.pos(order.id);
}

string OrderBook::queryDepth(bool bid, int depth)
{
    double price = 0;
    long long quantity = 0;
    int nItems = 0;
    string orderType = bid ? "bid" : "ask";

    map<double, PriceLevel> *m = bid ? &buyOrders : &sellOrders;
    shared_mutex *mutex = bid ? &buyMutex : &sellMutex;
    std::shared_lock lock(*mutex);
    if (depth != 0 && depth <= m->size())
    {
        int i = 0;
        if (bid)
        {
            for (map<double, PriceLevel>::reverse_iterator it = m->rbegin(); it != m->rend(); ++it)
            {
                if (depth == ++i)
                {
                    price = it->first;
                    quantity = it->second.quantity;
                    nItems = it->second.nItems();
                    break;
                }
            }
        }
        else
        {
            for (map<double, PriceLevel>::iterator it = m->begin(); it != m->end(); ++it)
            {
                if (depth == ++i)
                {
                    price = it->first;
                    quantity = it->second.quantity;
                    nItems = it->second.nItems();
                    break;
                }
            }
        }
    }

    ostringstream oss;
    oss << orderType << ", " << depth << ", " << price << ", " << quantity << ", " << nItems;
    return oss.str();
}

string OrderBook::queryOrder(long long orderID)
{
    map<long long, LimitOrder>::iterator it;
    double price = 0;
    int pos = -1;
    long long left = 0;
    long long quantity = 0;
    string orderType = "null";
    string orderStatus = "null";

    std::shared_lock lock(ordersMutex);
    it = orders.find(orderID);
    if (it != orders.end())
    {
        price = it->second.price;
        quantity = it->second.quantity;
        left = it->second.left;
        orderType = it->second.isBuyOrder ? "buy" : "sell";
        switch (it->second.status)
        {
        case open:
            orderStatus = "open";
            pos = this->pos(it->second);
            break;
        case partial:
            orderStatus = "partial";
            pos = this->pos(it->second);
            break;
        case executed:
            orderStatus = "executed";
            break;
        case cancelled:
            orderStatus = "cancelled";
            break;
        }
    }

    ostringstream oss;
    oss << orderType << ", " << price << ", " << quantity << ", " << left << ", " << pos << ", " << orderStatus;
    return oss.str();
}
