#pragma once

struct TradingRules {
    int max_transactions = 0;  // zero means unlimited
    double fee_per_transaction = 0.0;
    int cooldown_days = 0;
};
