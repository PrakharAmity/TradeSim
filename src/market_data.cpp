#include "market_data.hpp"

std::vector<MarketData> available_markets() {
    return {
        {"ACME_TECH", "ACME (Volatile)", {100, 120, 115, 130, 90, 110, 140, 100, 125, 150}},
        {"NOVA", "NOVA (Trending)", {80, 86, 91, 97, 104, 110, 118, 124, 132, 140}},
        {"STEEL", "STEEL (Declining)", {150, 143, 139, 130, 126, 118, 110, 105, 99, 94}}
    };
}

const MarketData* find_market(const std::string& ticker) {
    static const auto markets = available_markets();
    for (const auto& market : markets) if (market.ticker == ticker) return &market;
    return nullptr;
}
