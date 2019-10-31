#define BOOST_TEST_MODULE OrderBookTests
#include <boost/test/unit_test.hpp>
#include <iostream>

#include "order_book.hpp"

using namespace std;

BOOST_AUTO_TEST_SUITE(OrderBookSuite)

BOOST_AUTO_TEST_CASE(Insert)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1002, false, 100, 13.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1002, false, 100, 13.5);
    // Checking duplicate
    BOOST_CHECK(!ob.add(move(lo)));
    // Checking incorrect tick size
    lo = LimitOrder(1003, false, 100, 13.01);
    BOOST_CHECK(!ob.add(move(lo)));
}

BOOST_AUTO_TEST_CASE(QueryOrder)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1002, false, 100, 13.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, 0, open");
    BOOST_CHECK(ob.queryOrder(1002) == "sell, 13.5, 100, 100, 0, open");
    // Checking unknown order
    BOOST_CHECK(ob.queryOrder(1003) == "null, 0, 0, 0, -1, null");
}

BOOST_AUTO_TEST_CASE(QueryDepth)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryDepth(true, 1) == "bid, 1, 12.5, 100, 1");
    // Checking too deep
    BOOST_CHECK(ob.queryDepth(true, 2) == "bid, 2, 0, 0, 0");
}

BOOST_AUTO_TEST_CASE(Cancel)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, 0, open");
    BOOST_CHECK(ob.cancel(1001));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, -1, cancelled");
    BOOST_CHECK(!ob.cancel(1001));
    BOOST_CHECK(!ob.cancel(1002));
}

BOOST_AUTO_TEST_CASE(AmendPosition)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1002, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1003, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1004, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1005, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, 0, open");
    BOOST_CHECK(ob.queryOrder(1002) == "buy, 12.5, 100, 100, 1, open");
    BOOST_CHECK(ob.queryOrder(1003) == "buy, 12.5, 100, 100, 2, open");
    BOOST_CHECK(ob.queryOrder(1004) == "buy, 12.5, 100, 100, 3, open");
    BOOST_CHECK(ob.queryOrder(1005) == "buy, 12.5, 100, 100, 4, open");
    BOOST_CHECK(ob.cancel(1002));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, 0, open");
    BOOST_CHECK(ob.queryOrder(1002) == "buy, 12.5, 100, 100, -1, cancelled");
    BOOST_CHECK(ob.queryOrder(1003) == "buy, 12.5, 100, 100, 1, open");
    BOOST_CHECK(ob.queryOrder(1004) == "buy, 12.5, 100, 100, 2, open");
    BOOST_CHECK(ob.queryOrder(1005) == "buy, 12.5, 100, 100, 3, open");
    BOOST_CHECK(ob.amend(1003, 50));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, 0, open");
    BOOST_CHECK(ob.queryOrder(1003) == "buy, 12.5, 50, 50, 1, open");
    BOOST_CHECK(ob.queryOrder(1004) == "buy, 12.5, 100, 100, 2, open");
    BOOST_CHECK(ob.queryOrder(1005) == "buy, 12.5, 100, 100, 3, open");
    BOOST_CHECK(ob.amend(1003, 100));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 12.5, 100, 100, 0, open");
    BOOST_CHECK(ob.queryOrder(1003) == "buy, 12.5, 100, 100, 3, open");
    BOOST_CHECK(ob.queryOrder(1004) == "buy, 12.5, 100, 100, 1, open");
    BOOST_CHECK(ob.queryOrder(1005) == "buy, 12.5, 100, 100, 2, open");
}

BOOST_AUTO_TEST_CASE(Match)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, true, 100, 13.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1002, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1003, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1004, true, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryDepth(true, 1) == "bid, 1, 13.5, 100, 1");
    BOOST_CHECK(ob.queryDepth(true, 2) == "bid, 2, 12.5, 300, 3");
    lo = LimitOrder(1005, false, 50, 13.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 13.5, 100, 50, 0, partial");
    BOOST_CHECK(ob.queryOrder(1005) == "sell, 13.5, 50, 0, -1, executed");
    lo = LimitOrder(1006, false, 100, 12.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "buy, 13.5, 100, 0, -1, executed");
    BOOST_CHECK(ob.queryOrder(1002) == "buy, 12.5, 100, 50, 0, partial");
    BOOST_CHECK(ob.queryDepth(true, 1) == "bid, 1, 12.5, 250, 3");
    lo = LimitOrder(1007, false, 300, 11.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryDepth(true, 1) == "bid, 1, 0, 0, 0");
    BOOST_CHECK(ob.queryDepth(false, 1) == "ask, 1, 11.5, 50, 1");
}

BOOST_AUTO_TEST_CASE(MatchReverse)
{
    OrderBook ob = OrderBook(0.05, 0.001);
    LimitOrder lo = LimitOrder(1001, false, 100, 13.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1002, false, 100, 14.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1003, false, 100, 14.5);
    BOOST_CHECK(ob.add(move(lo)));
    lo = LimitOrder(1004, false, 100, 14.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryDepth(false, 1) == "ask, 1, 13.5, 100, 1");
    BOOST_CHECK(ob.queryDepth(false, 2) == "ask, 2, 14.5, 300, 3");
    lo = LimitOrder(1005, true, 50, 13.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "sell, 13.5, 100, 50, 0, partial");
    BOOST_CHECK(ob.queryOrder(1005) == "buy, 13.5, 50, 0, -1, executed");
    lo = LimitOrder(1006, true, 100, 14.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryOrder(1001) == "sell, 13.5, 100, 0, -1, executed");
    BOOST_CHECK(ob.queryOrder(1002) == "sell, 14.5, 100, 50, 0, partial");
    BOOST_CHECK(ob.queryDepth(false, 1) == "ask, 1, 14.5, 250, 3");
    lo = LimitOrder(1007, true, 300, 15.5);
    BOOST_CHECK(ob.add(move(lo)));
    BOOST_CHECK(ob.queryDepth(false, 1) == "ask, 1, 0, 0, 0");
    BOOST_CHECK(ob.queryDepth(true, 1) == "bid, 1, 15.5, 50, 1");
}

BOOST_AUTO_TEST_SUITE_END()
