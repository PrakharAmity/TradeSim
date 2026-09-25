#pragma once
#include "trade.hpp"
#include <string>
#include <vector>

struct BacktestResult {
    std::string strategy;
    double total_profit = 0.0;
    int transaction_count = 0;
    double max_drawdown = 0.0;
    bool valid = true;
    std::string status = "Valid Strategy";
    std::vector<Trade> trades;
};
