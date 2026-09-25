#include "cooldown_strategy.hpp"
#include "fee_strategy.hpp"
#include "limited_transaction_strategy.hpp"
#include "market_data.hpp"
#include "single_trade_strategy.hpp"
#include "unlimited_strategy.hpp"
#include <chrono>
#include <cmath>
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct TestCase { std::string name; std::function<void()> run; };
void expect_near(double actual, double expected, const std::string& message) {
    if (std::abs(actual - expected) > 0.001)
        throw std::runtime_error(message + " Expected $" + std::to_string(expected) + ", got $" + std::to_string(actual) + ".");
}
const MarketData& acme() { return *find_market("ACME_TECH"); }

void test_single_trade_optimal() {
    expect_near(SingleTradeStrategy{}.run(acme()).total_profit, 60.0, "Single-trade report mismatch.");
}
void test_unlimited_trades_profit() {
    expect_near(UnlimitedStrategy{}.run(acme()).total_profit, 135.0, "Unlimited-trading report mismatch.");
}
void test_two_trade_limit() {
    const auto result = LimitedTransactionStrategy{}.run(acme());
    expect_near(result.total_profit, 100.0, "Two-cycle mandate report mismatch.");
    if (result.transaction_count > 2) throw std::runtime_error("Mandate exceeded two completed cycles.");
}
void test_fee_aware_strategy_profit() {
    expect_near(FeeStrategy{}.run(acme()).total_profit, 127.0, "Expected net profit $127 after one $2 fee per completed cycle.");
}
void test_cooldown_reenter_validity() {
    const auto result = CooldownStrategy{}.run(acme());
    for (std::size_t i = 0; i + 1 < result.trades.size(); ++i) {
        if (result.trades[i].action == "SELL" && result.trades[i + 1].action == "BUY" &&
            result.trades[i + 1].day <= result.trades[i].day + 1)
            throw std::runtime_error("Strategy violated 1-day mandatory cooldown after Day " + std::to_string(result.trades[i].day + 1) + ".");
    }
    if (!result.valid) throw std::runtime_error("Cooldown execution ledger is marked unexpected.");
}
void test_declining_market_has_no_trade() {
    const auto* steel = find_market("STEEL");
    if (UnlimitedStrategy{}.run(*steel).total_profit != 0.0)
        throw std::runtime_error("Declining market should not produce a positive return.");
}
std::string escape_json(const std::string& value) {
    std::string escaped;
    for (char c : value) {
        if (c == '"' || c == '\\') escaped.push_back('\\');
        if (c == '\n') { escaped += "\\n"; continue; }
        if (c == '\r') { escaped += "\\r"; continue; }
        escaped.push_back(c);
    }
    return escaped;
}
}

int main() {
    const std::vector<TestCase> tests{
        {"test_single_trade_optimal", test_single_trade_optimal},
        {"test_unlimited_trades_profit", test_unlimited_trades_profit},
        {"test_two_trade_limit", test_two_trade_limit},
        {"test_fee_aware_strategy_profit", test_fee_aware_strategy_profit},
        {"test_cooldown_reenter_validity", test_cooldown_reenter_validity},
        {"test_declining_market_has_no_trade", test_declining_market_has_no_trade}
    };
    int passed = 0, failed = 0;
    long long total_ms = 0;
    std::cout << "{";
    for (std::size_t index = 0; index < tests.size(); ++index) {
        const auto started = std::chrono::steady_clock::now();
        std::string error;
        try { tests[index].run(); } catch (const std::exception& exception) { error = exception.what(); }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
        total_ms += elapsed;
        if (error.empty()) ++passed; else ++failed;
        if (index) std::cout << ',';
        std::cout << "\n  \"" << tests[index].name << "\": {\"Status\": \"" << (error.empty() ? "passed" : "failed")
                  << "\", \"Execution time\": \"" << elapsed << "ms\"";
        if (!error.empty()) std::cout << ", \"Error\": \"" << escape_json(error) << "\"";
        std::cout << "}";
    }
    std::cout << ",\n  \"Passed\": " << passed << ",\n  \"Failed\": " << failed
              << ",\n  \"Total bugs\": 6,\n  \"Total Execution time\": \"" << total_ms << "ms\"\n}\n";
    return failed == 0 ? 0 : 1;
}
