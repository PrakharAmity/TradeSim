#pragma once
#include "backtest_result.hpp"
#include "market_data.hpp"
#include "trading_rules.hpp"
#include <string>

class TradingStrategy {
public:
    virtual ~TradingStrategy() = default;
    virtual std::string id() const = 0;
    virtual BacktestResult run(const MarketData& market) const = 0;
};
