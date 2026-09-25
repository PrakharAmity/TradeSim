#include "limited_transaction_strategy.hpp"
#include "portfolio.hpp"
#include <functional>

std::string LimitedTransactionStrategy::id() const { return "limited"; }

BacktestResult LimitedTransactionStrategy::run(const MarketData& market) const {
    using Pair = std::pair<int, int>;
    std::vector<Pair> best_pairs, current;
    double best_profit = 0.0;
    const auto& prices = market.prices;
    std::function<void(int, int, double)> search = [&](int first_day, int remaining, double profit) {
        if (profit > best_profit) { best_profit = profit; best_pairs = current; }
        if (remaining == 0) return;
        for (int buy = first_day; buy < static_cast<int>(prices.size()); ++buy) {
            for (int sell = buy + 1; sell < static_cast<int>(prices.size()); ++sell) {
                current.emplace_back(buy, sell);
                search(sell + 1, remaining - 1, profit + prices[sell] - prices[buy]);
                current.pop_back();
            }
        }
    };
    search(0, 2, 0.0);
    std::vector<Trade> trades;
    for (const auto& pair : best_pairs) {
        trades.push_back({"BUY", pair.first, prices[pair.first], 0.0});
        trades.push_back({"SELL", pair.second, prices[pair.second], prices[pair.second] - prices[pair.first]});
    }
    return Portfolio::from_trades("Two-Trade Limit", prices, trades);
}
