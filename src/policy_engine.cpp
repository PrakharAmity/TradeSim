#include "policy_engine.hpp"
#include "cooldown_strategy.hpp"
#include "fee_strategy.hpp"
#include "limited_transaction_strategy.hpp"
#include "portfolio.hpp"
#include "single_trade_strategy.hpp"
#include "unlimited_strategy.hpp"
#include <functional>

namespace {
using json = nlohmann::json;

json serialize_result(const BacktestResult& result) {
    json item = json::object();
    item["strategy"] = result.strategy;
    item["totalProfit"] = result.total_profit;
    item["transactions"] = result.transaction_count;
    item["maxDrawdown"] = result.max_drawdown;
    item["valid"] = result.valid;
    item["status"] = result.status;
    json trades = json::array();
    for (const auto& trade : result.trades) {
        json event = json::object();
        event["action"] = trade.action;
        event["day"] = trade.day;
        event["price"] = trade.price;
        event["realizedProfit"] = trade.realized_profit;
        trades.push_back(event);
    }
    item["trades"] = trades;
    return item;
}

BacktestResult run_combined_strategy(const MarketData& market) {
    constexpr int transaction_limit = 2;
    constexpr int cooldown_days = 1;
    constexpr double fee = 2.0;
    using Pair = std::pair<int, int>;
    const auto& prices = market.prices;
    std::vector<Pair> current, best_pairs;
    double best_profit = 0.0;
    std::function<void(int, int, double)> search = [&](int first_day, int remaining, double profit) {
        if (profit > best_profit) { best_profit = profit; best_pairs = current; }
        if (remaining == 0) return;
        for (int buy = first_day; buy < static_cast<int>(prices.size()); ++buy) {
            for (int sell = buy + 1; sell < static_cast<int>(prices.size()); ++sell) {
                const double net = prices[sell] - prices[buy] - fee;
                if (net <= 0.0) continue;
                current.emplace_back(buy, sell);
                search(sell + cooldown_days + 1, remaining - 1, profit + net);
                current.pop_back();
            }
        }
    };
    search(0, transaction_limit, 0.0);
    std::vector<Trade> trades;
    for (const auto& pair : best_pairs) {
        trades.push_back({"BUY", pair.first, prices[pair.first], 0.0});
        trades.push_back({"SELL", pair.second, prices[pair.second], prices[pair.second] - prices[pair.first] - fee});
    }
    return Portfolio::from_trades("Combined Policy (2 trades, $2 fee, 1-day cooldown)", prices, trades);
}
}  // namespace

nlohmann::json PolicyEngine::getSystemState() const {
    json state = json::object();
    json markets = json::array();
    for (const auto& market : available_markets()) {
        json item = json::object(); item["ticker"] = market.ticker; item["name"] = market.name;
        json prices = json::array(); for (double price : market.prices) prices.push_back(price);
        item["prices"] = prices; markets.push_back(item);
    }
    state["markets"] = markets;
    json strategies = json::array();
    const std::vector<std::pair<std::string, std::string>> catalog{
        {"single", "Single Trade"}, {"unlimited", "Unlimited Trading"}, {"limited", "Two-Trade Limit"},
        {"fee", "Fee-Aware ($2 fee)"}, {"cooldown", "Cooldown (1 day)"}, {"combined", "Combined Policy"}};
    for (const auto& entry : catalog) { json item = json::object(); item["id"] = entry.first; item["name"] = entry.second; strategies.push_back(item); }
    state["strategies"] = strategies;
    state["service"] = "TradeSim Execution Engine";
    return state;
}

nlohmann::json PolicyEngine::runBacktest(const std::string& strategy_name, const std::string& ticker) const {
    const MarketData* market = find_market(ticker);
    if (!market) market = find_market("ACME_TECH");
    std::vector<BacktestResult> results;
    const SingleTradeStrategy single;
    const UnlimitedStrategy unlimited;
    const LimitedTransactionStrategy limited;
    const FeeStrategy fee;
    const CooldownStrategy cooldown;
    const auto add_if_selected = [&](const std::string& id, const BacktestResult& result) {
        if (strategy_name == "all" || strategy_name == id) results.push_back(result);
    };
    add_if_selected("single", single.run(*market));
    add_if_selected("unlimited", unlimited.run(*market));
    add_if_selected("limited", limited.run(*market));
    add_if_selected("fee", fee.run(*market));
    add_if_selected("cooldown", cooldown.run(*market));
    add_if_selected("combined", run_combined_strategy(*market));
    if (results.empty()) results = {single.run(*market), unlimited.run(*market), limited.run(*market), fee.run(*market), cooldown.run(*market), run_combined_strategy(*market)};

    json response = json::object();
    response["ticker"] = market->ticker;
    response["marketName"] = market->name;
    json prices = json::array(); for (double price : market->prices) prices.push_back(price);
    response["prices"] = prices;
    json result_list = json::array(); for (const auto& result : results) result_list.push_back(serialize_result(result));
    response["results"] = result_list;
    return response;
}

void PolicyEngine::resetDefaults() {}
