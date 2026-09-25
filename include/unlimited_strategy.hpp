#pragma once
#include "trading_strategy.hpp"
class UnlimitedStrategy final : public TradingStrategy {
public:
    std::string id() const override;
    BacktestResult run(const MarketData& market) const override;
};
