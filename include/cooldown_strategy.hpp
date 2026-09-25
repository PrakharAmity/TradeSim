#pragma once
#include "trading_strategy.hpp"
class CooldownStrategy final : public TradingStrategy {
public:
    std::string id() const override;
    BacktestResult run(const MarketData& market) const override;
};
