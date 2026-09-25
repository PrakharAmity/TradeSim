#include "single_trade_strategy.hpp"
#include "portfolio.hpp"
#include <limits>

std::string SingleTradeStrategy::id() const { return "single"; }

BacktestResult SingleTradeStrategy::run(const MarketData& market) const {
    std::vector<Trade> trades;
    if (market.prices.size() >= 2) {
        double lowest = std::numeric_limits<double>::infinity();
        int lowest_day = 0, buy_day = -1, sell_day = -1;
        double best = 0.0;
        for (std::size_t day = 0; day < market.prices.size(); ++day) {
            const double price = market.prices[day];
            if (price - lowest > best) { best = price - lowest; buy_day = lowest_day; sell_day = static_cast<int>(day); }
            if (price < lowest) { lowest = price; lowest_day = static_cast<int>(day); }
        }
        if (buy_day >= 0 && sell_day > buy_day) {
            trades.push_back({"BUY", buy_day, market.prices[buy_day], 0.0});
            trades.push_back({"SELL", sell_day, market.prices[sell_day], best});
        }
    }
    return Portfolio::from_trades("Single Trade", market.prices, trades);
}
