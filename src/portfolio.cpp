#include "portfolio.hpp"
#include <algorithm>

BacktestResult Portfolio::from_trades(const std::string& name,
                                      const std::vector<double>& prices,
                                      const std::vector<Trade>& trades) {
    BacktestResult result;
    result.strategy = name;
    result.trades = trades;
    bool holding = false;
    int last_day = -1;
    for (const auto& trade : trades) {
        if (trade.day < 0 || static_cast<std::size_t>(trade.day) >= prices.size() || trade.day < last_day) result.valid = false;
        if (trade.action == "BUY") {
            if (holding) result.valid = false;
            holding = true;
        } else if (trade.action == "SELL") {
            if (!holding) result.valid = false;
            holding = false;
            ++result.transaction_count;
            result.total_profit += trade.realized_profit;
        } else result.valid = false;
        last_day = trade.day;
    }
    if (holding) result.valid = false;
    double peak = prices.empty() ? 0.0 : prices.front();
    for (double price : prices) {
        peak = std::max(peak, price);
        result.max_drawdown = std::max(result.max_drawdown, peak - price);
    }
    result.status = result.valid ? "Valid Strategy" : "Unexpected Result";
    return result;
}

void Portfolio::mark_unexpected(BacktestResult& result) {
    result.valid = false;
    result.status = "Unexpected Result";
}
