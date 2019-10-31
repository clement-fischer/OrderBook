#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <chrono>
#include <map>
#include <set>
#include <shared_mutex>

using namespace std;

enum OrderStatus
{
    open,
    partial,
    executed,
    cancelled
};

class LimitOrder
{
private:
    long long id;
    bool isBuyOrder;
    double price;
    mutable long quantity;
    mutable long left;
    mutable OrderStatus status;
    chrono::microseconds timestamp;
    friend class OrderBook;
    friend class PriceLevel;

public:
    LimitOrder(long long orderID, bool isBuyOrder_, long quantity_, double price_);
    bool operator<(const LimitOrder &other) const;
};

class PriceLevel
{
private:
    set<LimitOrder> orders;
    friend class OrderBook;

public:
    long long quantity;
    bool cancel(long long orderID, shared_mutex &m);
    void insert(LimitOrder order);
    int nItems();
    int pos(long long orderID);
    void update(LimitOrder &&order, shared_mutex &m);
};

class OrderBook
{
private:
    map<double, PriceLevel> buyOrders;
    map<double, PriceLevel> sellOrders;
    map<long long, LimitOrder> orders;

    mutable shared_mutex buyMutex;
    mutable shared_mutex sellMutex;
    mutable shared_mutex ordersMutex;

    double tickSize;
    double precision;

public:
    OrderBook(double tickSize, double tolerance);
    bool add(LimitOrder &&order);
    bool amend(long long orderID, long quantity);
    bool cancel(long long orderID);
    void fill(PriceLevel &pl, LimitOrder &order);
    void match(LimitOrder &order);
    int pos(LimitOrder &order);
    string queryDepth(bool bid, int depth);
    string queryOrder(long long orderID);
};

#endif /* ORDERBOOK_H */
