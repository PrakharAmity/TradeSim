# TradeSim mentor notes — hidden from candidates

## System architecture

`src/main.cpp` starts the bundled HTTP server and serves the static dashboard from `web/`. The browser loads market and strategy choices from `GET /api/state`, then requests computed reports from `POST /api/backtest`. `PolicyEngine` selects strategy implementations and serializes their results. `TradingStrategy` implementations produce ordered buy/sell events. `Portfolio` validates the ledger and calculates total profit, completed-cycle count, status, and market drawdown.

## Seeded bug inventory

There are exactly six seeded defects across the strategy catalog:

1. `src/single_trade_strategy.cpp:16`: Inverted profit delta (`lowest - price` instead of `price - lowest`).
2. `src/unlimited_strategy.cpp:12,14`: Inverted valley/peak monotonic scan which buys on dips and sells on troughs, leading to losses and trading in declining markets.
3. `src/limited_transaction_strategy.cpp:23`: Exceeds the two-transaction mandate by searching with a depth of 3 cycles instead of 2.
4. `src/fee_strategy.cpp:17`: A $2 settlement charge is deducted prematurely upon position entry.
5. `src/fee_strategy.cpp:21`: Settlement subtracts double fees (`2.0 * fee`) instead of the single flat fee.
6. `src/cooldown_strategy.cpp:13,18`: Cooldown period boundary condition allows re-entering on day `d+1` rather than enforcing 1 full idle day.

## Exact expected patch

```diff
diff --git a/src/single_trade_strategy.cpp b/src/single_trade_strategy.cpp
--- a/src/single_trade_strategy.cpp
+++ b/src/single_trade_strategy.cpp
@@ -15,3 +15,3 @@
-            if (lowest - price > best) { best = lowest - price; buy_day = lowest_day; sell_day = static_cast<int>(day); }
+            if (price - lowest > best) { best = price - lowest; buy_day = lowest_day; sell_day = static_cast<int>(day); }

diff --git a/src/unlimited_strategy.cpp b/src/unlimited_strategy.cpp
--- a/src/unlimited_strategy.cpp
+++ b/src/unlimited_strategy.cpp
@@ -11,6 +11,6 @@
-        while (day + 1 < p.size() && p[day + 1] >= p[day]) ++day;
+        while (day + 1 < p.size() && p[day + 1] <= p[day]) ++day;
         const auto buy_day = day;
-        while (day + 1 < p.size() && p[day + 1] <= p[day]) ++day;
+        while (day + 1 < p.size() && p[day + 1] >= p[day]) ++day;
         const auto sell_day = day;
-        if (sell_day > buy_day) {
+        if (sell_day > buy_day && p[sell_day] > p[buy_day]) {

diff --git a/src/limited_transaction_strategy.cpp b/src/limited_transaction_strategy.cpp
--- a/src/limited_transaction_strategy.cpp
+++ b/src/limited_transaction_strategy.cpp
@@ -23,3 +23,3 @@
-    search(0, 3, 0.0);
+    search(0, 2, 0.0);

diff --git a/src/fee_strategy.cpp b/src/fee_strategy.cpp
--- a/src/fee_strategy.cpp
+++ b/src/fee_strategy.cpp
@@ -17,5 +17,5 @@
-            buy_cost = prices[day] + fee;
+            buy_cost = prices[day];
             trades.push_back({"BUY", day, prices[day], 0.0});
         } else if (should_sell) {
             const double gross = prices[day] - prices[buy_day];
-            const double net = prices[day] - buy_cost - 2.0 * fee;
+            const double net = prices[day] - buy_cost - fee;

diff --git a/src/cooldown_strategy.cpp b/src/cooldown_strategy.cpp
--- a/src/cooldown_strategy.cpp
+++ b/src/cooldown_strategy.cpp
@@ -13,7 +13,7 @@
-        if (buy_day < 0 && day >= cooldown_until_day && day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1]) {
+        if (buy_day < 0 && day > cooldown_until_day && day + 1 < static_cast<int>(prices.size()) && prices[day] <= prices[day + 1]) {
             buy_day = day;
             trades.push_back({"BUY", day, prices[day], 0.0});
         } else if (buy_day >= 0 && (day + 1 == static_cast<int>(prices.size()) || prices[day] > prices[day + 1])) {
             trades.push_back({"SELL", day, prices[day], prices[day] - prices[buy_day]});
-            cooldown_until_day = day + cooldown_days - 1;
+            cooldown_until_day = day + cooldown_days;
             buy_day = -1;
         }
```
