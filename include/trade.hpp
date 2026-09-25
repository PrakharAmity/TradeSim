#pragma once
#include <string>

struct Trade {
    std::string action;
    int day = 0;
    double price = 0.0;
    double realized_profit = 0.0;
};
