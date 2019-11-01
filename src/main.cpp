// A simple program that computes the square root of a number
#include <stdio.h>
#include <stdlib.h>
#include <boost/tokenizer.hpp>
#include <iostream>

#include "order_book.hpp"

using namespace boost;
using namespace std;

string orderCommand(OrderBook &ob, vector<string> &tokens) {
    string usageString = "Usage: order <order_id> <buy|sell> <quantity> <price>";
    if (tokens.size() != 5) {
        return usageString;
    }
    long long orderID = stoll(tokens[1]);

    bool isBuyOrder;
    if (tokens[2] == "buy") {
        isBuyOrder = true;
    } else if (tokens[2] == "sell") {
        isBuyOrder = false;
    } else {
        return usageString;
    }

    long quantity = stol(tokens[3]);
    double price = stod(tokens[4]);
    LimitOrder lo(orderID, isBuyOrder, quantity, price);
    if (ob.add(move(lo))) {
        return "Order added";
    } else {
        return "Order rejected";
    }
}

string cancelCommand(OrderBook &ob, vector<string> &tokens) {
    if (tokens.size() != 2) return "Usage: cancel <order_id>";

    long long orderID = stoll(tokens[1]);
    if (ob.cancel(orderID)) {
        return "Order cancelled";
    } else {
        return "Order not cancelled";
    }
}

string amendCommand(OrderBook &ob, vector<string> &tokens) {
    if (tokens.size() != 3) {
        return "Usage: order <order_id> <quantity>";
    }
    long long orderID = stoll(tokens[1]);
    long quantity = stol(tokens[2]);
    if (ob.amend(orderID, quantity)) {
        return "Order amended";
    } else {
        return "Order not amended";
    }
}

string queryCommand(OrderBook &ob, vector<string> &tokens) {
    string usageString = "Usage: q <level|order> ...";
    if (tokens.size() < 2) return usageString;

    if (tokens[1] == "level") {
        usageString = "Usage: order level <ask|bid> <depth>";
        if (tokens.size() != 4) return usageString;

        bool bid;
        if (tokens[2] == "bid") {
            bid = true;
        } else if (tokens[2] == "ask") {
            bid = false;
        } else {
            return usageString;
        }

        long depth = stoi(tokens[3]);
        return ob.queryDepth(bid, depth);
    } else if (tokens[1] == "order") {
        if (tokens.size() != 3) {
            long long orderID = stoll(tokens[2]);
            return ob.queryOrder(orderID);
        }
    } else {
        return usageString;
    }
}

string commandLine(OrderBook &ob, vector<string> &tokens) {
    auto it = tokens.begin();
    string token = *it;
    if (token == "order") return orderCommand(ob, tokens);
    if (token == "cancel") return cancelCommand(ob, tokens);
    if (token == "amend") return amendCommand(ob, tokens);
    if (token == "q") return queryCommand(ob, tokens);
    return "Invalid command";
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stdout, "Usage: %s tick_size precision\n", argv[0]);
        return 1;
    }

    double tickSize = stod(argv[1]);
    double precision = stod(argv[2]);
    OrderBook ob(tickSize, precision);

    cout << ">> ";
    char_separator<char> sep(" ");
    for (string line; getline(cin, line);) {
        string output;
        if (0 < line.length()) {
            vector<string> v;
            tokenizer<char_separator<char>> tokens(line, sep);
            for (auto it = tokens.begin(); it != tokens.end(); it++) {
                v.push_back(*it);
            }
            output = commandLine(ob, v);
        }
        cout << output << endl
             << ">> ";
    }

    return 0;
}
