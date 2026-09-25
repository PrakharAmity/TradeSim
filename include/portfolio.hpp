#pragma once
#include "backtest_result.hpp"
#include <vector>

class Portfolio {
public:
    static BacktestResult from_trades(const std::string& name,
                                      const std::vector<double>& prices,
                                      const std::vector<Trade>& trades);
    static void mark_unexpected(BacktestResult& result);
};
