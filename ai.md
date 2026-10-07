# TradeSim

## Description

TradeSim is a C++17 algorithmic trading backtesting engine and execution simulation dashboard that helps quantitative developers and portfolio managers evaluate trading policies against historical closing prices. It runs as a lightweight HTTP service with a browser interface, execution telemetry, and deterministic market price histories. The engine evaluates drawdowns, transaction counts, and portfolio health, while allowing users to compare multiple execution strategies against historical market data.

## Repository Structure

```text
tradesim/
├── CMakeLists.txt                 Build configuration for the core library, server, and tests
├── challenge.json                 Isolated runtime, port, build, start, and test configuration
├── start.sh                       Build, launch, and live-rebuild watcher entrypoint
├── include/
│   ├── backtest_result.hpp        Backtest execution and risk metrics data structures
│   ├── cooldown_strategy.hpp      Cooldown strategy interface declaration
│   ├── fee_strategy.hpp           Transaction fee-aware strategy interface declaration
│   ├── httplib.h                  Single-header C++ HTTP server library
│   ├── json.hpp                   Single-header JSON parsing and serialization library
│   ├── limited_transaction_strategy.hpp  Limited transaction strategy interface declaration
│   ├── market_data.hpp            Market dataset definitions and lookup functions
│   ├── policy_engine.hpp          Policy engine API for running and comparing strategies
│   ├── portfolio.hpp              Portfolio validation, ledger tracking, and drawdown evaluation
│   ├── single_trade_strategy.hpp  Single optimal trade strategy interface declaration
│   ├── trade.hpp                  Trade action and execution record structures
│   ├── trading_rules.hpp          Strategy configuration parameters and execution rules
│   ├── trading_strategy.hpp       Base trading strategy abstract class
│   └── unlimited_strategy.hpp     Unlimited trading strategy interface declaration
├── src/
│   ├── cooldown_strategy.cpp      Cooldown strategy with mandatory idle window logic
│   ├── fee_strategy.cpp           Fee-aware strategy deducting settlement charges
│   ├── limited_transaction_strategy.cpp  Limited transaction strategy optimizing up to N cycles
│   ├── main.cpp                   HTTP server, routing, static assets, and JSON responses
│   ├── market_data.cpp            Deterministic market price histories (ACME_TECH, NOVA, STEEL)
│   ├── policy_engine.cpp          Strategy execution dispatcher and API payload serialization
│   ├── portfolio.cpp              Ledger verification, realized profit, cycle counting, and drawdown calculation
│   ├── single_trade_strategy.cpp  Optimal single buy/sell spread strategy
│   └── unlimited_strategy.cpp     Greedy peak-and-valley unlimited trading strategy
├── web/
│   ├── index.html                 TradeSim dashboard page structure and layout
│   ├── style.css                  Responsive dark-mode financial visual design
│   └── app.js                     Live market selection, backtest API calls, and ledger rendering
├── tests/
│   ├── CMakeLists.txt             CMake build configuration for the test suite
│   ├── run_tests.sh               Incremental test rebuild and JSON test runner
│   └── test_main.cpp              Six behavioral challenge tests
└── README.md                      Candidate-facing application and bug reproduction guide
```

## Bugs and Bug Locations

These are the six behavioral bug surfaces covered by the challenge. The named locations identify the owning implementation areas for debugging and review.

### 1. Single trade strategy produces zero returns

- **Bug location:** `src/single_trade_strategy.cpp`, `SingleTradeStrategy::run`
- **How to observe it:** Select the `ACME (Volatile)` market and run the `Single Trade` policy (or compare all policies) on the dashboard.
- **Failure:** The strategy calculates the profit delta in reverse (`lowest - price`), resulting in $0.00 total profit instead of capturing the optimal buy-low/sell-high spread ($60.00).
- **Expected:** Profit delta evaluates `price - lowest > best`, recording a buy at $90 on Day 5 and a sell at $150 on Day 10 to yield $60.00 total profit.

### 2. Unlimited trading buys high and sells low

- **Bug location:** `src/unlimited_strategy.cpp`, `UnlimitedStrategy::run`
- **How to observe it:** Select the `ACME (Volatile)` market and run the `Unlimited Trading` policy on the dashboard.
- **Failure:** Monotonic while-loop conditions are inverted (`p[day + 1] >= p[day]` for buying and `p[day + 1] <= p[day]` for selling), buying at local peaks and selling at local troughs, yielding -$85.00 total profit.
- **Expected:** The scan advances through decreasing prices to buy at local valleys (`p[day + 1] <= p[day]`) and increasing prices to sell at local peaks (`p[day + 1] >= p[day]`), capturing all profitable swings for $135.00 total profit on ACME_TECH.

### 3. Two-trade limit mandate exceeds transaction quota

- **Bug location:** `src/limited_transaction_strategy.cpp`, `LimitedTransactionStrategy::run`
- **How to observe it:** Select the `ACME (Volatile)` market and run the `Two-Trade Limit` policy on the dashboard.
- **Failure:** The search depth initial condition is set to 3 (`search(0, 3, 0.0)`), allowing the strategy to execute 3 complete round-trip cycles and violate executive risk transaction quotas.
- **Expected:** The search depth is bounded to at most 2 completed cycles (`search(0, 2, 0.0)`), producing at most 2 round-trip cycles and exactly $100.00 total profit on ACME_TECH.

### 4. Fee-aware strategy over-deducts transaction costs

- **Bug location:** `src/fee_strategy.cpp`, `FeeStrategy::run`; anomaly status surfaced by `web/app.js`
- **How to observe it:** Select the `ACME (Volatile)` market and run the `Fee-Aware ($2 fee)` policy on the dashboard.
- **Failure:** The $2 flat fee is prematurely added to `buy_cost` upon entering a position and then subtracted again with double fees (`2.0 * fee`) upon exit, reducing net profit to $111.00 instead of $127.00 and triggering an "Unexpected Result" anomaly badge.
- **Expected:** The entry records the base asset price (`buy_cost = prices[day]`) and exit deducts a single flat fee (`net = prices[day] - buy_cost - fee`), yielding $127.00 net profit on ACME_TECH and a "Valid Strategy" status.

### 5. Cooldown strategy violates mandatory idle window

- **Bug location:** `src/cooldown_strategy.cpp`, `CooldownStrategy::run`; anomaly status surfaced by `web/app.js`
- **How to observe it:** Select the `ACME (Volatile)` market and run the `Cooldown (1 day)` policy on the dashboard.
- **Failure:** The cooldown boundary is calculated with an off-by-one error (`cooldown_until_day = day + cooldown_days - 1` and `day >= cooldown_until_day`), allowing immediate next-day re-entry (Day 2 after selling on Day 1) and flagging the execution ledger with an "Unexpected Result" badge.
- **Expected:** The strategy enforces a full mandatory 1-day idle session between selling and re-entering (`cooldown_until_day = day + cooldown_days` and `day > cooldown_until_day`), producing zero cooldown violations and a "Valid Strategy" status.

### 6. Declining market generates non-zero or loss-making trades

- **Bug location:** `src/unlimited_strategy.cpp`, `UnlimitedStrategy::run`
- **How to observe it:** Select the `STEEL (Declining)` market and run the `Unlimited Trading` policy on the dashboard.
- **Failure:** The strategy executes transactions in a continuously falling market without validating whether the sell price exceeds the buy price, producing non-zero trade events and negative returns instead of remaining in cash.
- **Expected:** The execution check requires `sell_day > buy_day && p[sell_day] > p[buy_day]`, preventing position entries during persistent market downturns and resulting in zero transactions and $0.00 total profit.

## Expected Behaviour After Fixing All Bugs

- Single-trade strategy identifies the maximal buy-low/sell-high spread, yielding $60.00 total profit on ACME_TECH.
- Unlimited trading buys at local valleys and sells at local peaks, capturing all positive swings for $135.00 total profit on ACME_TECH.
- Two-trade limit strategy adheres strictly to the 2-cycle mandate, executing at most 2 round-trip transactions for $100.00 total profit on ACME_TECH.
- Fee-aware strategy deducts exactly one $2 settlement fee per completed transaction cycle, achieving $127.00 net profit on ACME_TECH with a valid strategy status.
- Cooldown strategy enforces a mandatory 1-day idle window after each sale, recording zero cooldown violations and maintaining a valid ledger status.
- Declining market regimes (STEEL) produce zero trade executions and $0.00 total profit.
- Automated test runner (`bash tests/run_tests.sh`) passes all 6 tests with `"Passed": 6`, `"Failed": 0`, and exits with code `0`.
