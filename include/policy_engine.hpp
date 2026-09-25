#pragma once
#include "backtest_result.hpp"
#include "market_data.hpp"
#include "json.hpp"
#include <string>
#include <vector>

class PolicyEngine {
public:
    nlohmann::json getSystemState() const;
    nlohmann::json runBacktest(const std::string& strategy_name, const std::string& ticker) const;
    void resetDefaults();

};
