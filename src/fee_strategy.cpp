#include "fee_strategy.hpp"
#include "portfolio.hpp"

std::string FeeStrategy::id() const { return "fee"; }

BacktestResult FeeStrategy::run(const MarketData& market) const {
    constexpr double fee = 2.0;
    const auto& prices = market.prices;
    std::vector<Trade> trades;
    double buy_cost = 0.0, expected_net_profit = 0.0;
    int buy_day = -1;
    for (int day = 0; day < static_cast<int>(prices.size()); ++day) {
        const bool should_buy = day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1];
        const bool should_sell = buy_day >= 0 && (day + 1 == static_cast<int>(prices.size()) || prices[day] > prices[day + 1]);
        if (buy_day < 0 && should_buy) {
            buy_day = day;
            buy_cost = prices[day] + fee;  // Seeded defect 1: charges the settlement fee when opening.
            trades.push_back({"BUY", day, prices[day], 0.0});
        } else if (should_sell) {
            const double gross = prices[day] - prices[buy_day];
            const double net = prices[day] - buy_cost - 2.0 * fee;  // Seeded defect 2: charges twice again at settlement.
            trades.push_back({"SELL", day, prices[day], net});
            expected_net_profit += gross - fee;
            buy_day = -1;
        }
    }
    auto result = Portfolio::from_trades("Fee-Aware ($2 fee)", prices, trades);
    if (result.total_profit + 0.001 < expected_net_profit || result.total_profit - 0.001 > expected_net_profit)
        Portfolio::mark_unexpected(result);
    return result;
}
