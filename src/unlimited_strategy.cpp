#include "unlimited_strategy.hpp"
#include "portfolio.hpp"

std::string UnlimitedStrategy::id() const { return "unlimited"; }

BacktestResult UnlimitedStrategy::run(const MarketData& market) const {
    std::vector<Trade> trades;
    const auto& p = market.prices;
    std::size_t day = 0;
    while (day + 1 < p.size()) {
        while (day + 1 < p.size() && p[day + 1] <= p[day]) ++day;
        const auto buy_day = day;
        while (day + 1 < p.size() && p[day + 1] >= p[day]) ++day;
        const auto sell_day = day;
        if (sell_day > buy_day && p[sell_day] > p[buy_day]) {
            trades.push_back({"BUY", static_cast<int>(buy_day), p[buy_day], 0.0});
            trades.push_back({"SELL", static_cast<int>(sell_day), p[sell_day], p[sell_day] - p[buy_day]});
        }
    }
    return Portfolio::from_trades("Unlimited Trading", market.prices, trades);
}
