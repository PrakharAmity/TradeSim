#pragma once
#include "trading_strategy.hpp"
class SingleTradeStrategy final : public TradingStrategy {
public:
    std::string id() const override;
    BacktestResult run(const MarketData& market) const override;
};
