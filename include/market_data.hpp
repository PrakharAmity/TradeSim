#pragma once
#include <string>
#include <vector>

struct MarketData {
    std::string ticker;
    std::string name;
    std::vector<double> prices;
};

std::vector<MarketData> available_markets();
const MarketData* find_market(const std::string& ticker);
