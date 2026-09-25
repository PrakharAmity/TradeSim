#include "cooldown_strategy.hpp"
#include "portfolio.hpp"

std::string CooldownStrategy::id() const { return "cooldown"; }

BacktestResult CooldownStrategy::run(const MarketData& market) const {
    constexpr int cooldown_days = 1;
    const auto& prices = market.prices;
    std::vector<Trade> trades;
    int buy_day = -1;
    int cooldown_until_day = -1;
    for (int day = 0; day < static_cast<int>(prices.size()); ++day) {
        if (buy_day < 0 && day >= cooldown_until_day && day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1]) {  // Seeded defect 4: final restricted session is treated as eligible.
            buy_day = day;
            trades.push_back({"BUY", day, prices[day], 0.0});
        } else if (buy_day >= 0 && (day + 1 == static_cast<int>(prices.size()) || prices[day] > prices[day + 1])) {
            trades.push_back({"SELL", day, prices[day], prices[day] - prices[buy_day]});
            cooldown_until_day = day + cooldown_days - 1;  // Seeded defect 3: last restricted day is one day too early.
            buy_day = -1;
        }
    }
    auto result = Portfolio::from_trades("Cooldown (1 day)", prices, trades);
    // Eligibility requires one complete session after the sale before a new entry.
    for (std::size_t i = 0; i + 1 < trades.size(); ++i) {
        if (trades[i].action == "SELL" && trades[i + 1].action == "BUY" &&
            trades[i + 1].day <= trades[i].day + cooldown_days) Portfolio::mark_unexpected(result);
    }
    return result;
}
