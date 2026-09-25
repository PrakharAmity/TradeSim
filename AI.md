# TradeSim mentor notes — hidden from candidates

## System architecture

`src/main.cpp` starts the bundled HTTP server and serves the static dashboard from `web/`. The browser loads market and strategy choices from `GET /api/state`, then requests computed reports from `POST /api/backtest`. `PolicyEngine` selects strategy implementations and serializes their results. `TradingStrategy` implementations produce ordered buy/sell events. `Portfolio` validates the ledger and calculates total profit, completed-cycle count, status, and market drawdown.

The single-trade, unlimited, limited-transaction, and combined strategies provide working comparisons. The two intentionally broken execution paths are in `FeeStrategy` and `CooldownStrategy`.

## Seeded bug inventory

There are exactly four seeded defects:

1. `src/fee_strategy.cpp:17`: a $2 settlement charge is added to the position cost on entry.
2. `src/fee_strategy.cpp:21`: settlement subtracts two more fees instead of one.
3. `src/cooldown_strategy.cpp:18`: the last restricted session is calculated one session too early.
4. `src/cooldown_strategy.cpp:13`: the entry guard treats the last restricted session as eligible.

For a completed cycle with gross return `G` and fee `F`, the fee strategy should report `G - F`. The seeded version reports `G - 3F` because it charges `F` at entry and `2F` at exit. The ACME history has gross gains totaling $135 over four completed cycles, so the expected fee-adjusted total is $127 while the broken result is $111.

For a cooldown of one full session after a sale on day `d`, day `d+1` must remain idle and the earliest next entry is day `d+2`. The seeded boundary calculation and comparison permit re-entry during day `d+1`. On ACME, the fee and cooldown reports therefore receive `Unexpected Result` badges at startup.

## Socratic hints

1. Compare a reported cycle’s gross price change with its final net contribution. For cooldown, mark the sell day and the mandatory idle session on a calendar.
2. Count how many times the fee is included between opening and closing a position. Which session is the final restricted cooldown session?
3. Inspect the fee-aware position-cost and settlement expressions, then compare the cooldown end-day calculation with its entry comparison.
4. Trace ACME around sessions 1–4. A cycle with a $20 gross gain should pay one $2 fee; after selling on day 2, the next eligible buy must be no earlier than day 4.

## Exact expected patch

```diff
diff --git a/src/fee_strategy.cpp b/src/fee_strategy.cpp
--- a/src/fee_strategy.cpp
+++ b/src/fee_strategy.cpp
@@ -14,11 +14,11 @@ BacktestResult FeeStrategy::run(const MarketData& market) const {
         const bool should_buy = day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1];
         const bool should_sell = buy_day >= 0 && (day + 1 == static_cast<int>(prices.size()) || prices[day] > prices[day + 1]);
         if (buy_day < 0 && should_buy) {
             buy_day = day;
-            buy_cost = prices[day] + fee;  // Seeded defect 1: charges the settlement fee when opening.
+            buy_cost = prices[day];
             trades.push_back({"BUY", day, prices[day], 0.0});
         } else if (should_sell) {
             const double gross = prices[day] - prices[buy_day];
-            const double net = prices[day] - buy_cost - 2.0 * fee;  // Seeded defect 2: charges twice again at settlement.
+            const double net = prices[day] - buy_cost - fee;
diff --git a/src/cooldown_strategy.cpp b/src/cooldown_strategy.cpp
--- a/src/cooldown_strategy.cpp
+++ b/src/cooldown_strategy.cpp
@@ -10,12 +10,12 @@ BacktestResult CooldownStrategy::run(const MarketData& market) const {
     int buy_day = -1;
     int cooldown_until_day = -1;
     for (int day = 0; day < static_cast<int>(prices.size()); ++day) {
-        if (buy_day < 0 && day >= cooldown_until_day && day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1]) {  // Seeded defect 4: final restricted session is treated as eligible.
+        if (buy_day < 0 && day > cooldown_until_day && day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1]) {
             buy_day = day;
             trades.push_back({"BUY", day, prices[day], 0.0});
         } else if (buy_day >= 0 && (day + 1 == static_cast<int>(prices.size()) || prices[day] > prices[day + 1])) {
             trades.push_back({"SELL", day, prices[day], prices[day] - prices[buy_day]});
-            cooldown_until_day = day + cooldown_days - 1;  // Seeded defect 3: last restricted day is one day too early.
+            cooldown_until_day = day + cooldown_days;
             buy_day = -1;
         }
```

The fee and cooldown policy checks and public API remain unchanged. Applying the patch makes the fee result agree with the one-fee rule and keeps one complete session clear after every sale.
