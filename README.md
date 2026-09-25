# TradeSim — Strategy Backtesting & Execution Engine

TradeSim is an interactive strategy analytics service and execution simulation dashboard built for quantitative developers and portfolio managers. It allows users to compare trading policies against historical closing prices, evaluate drawdowns and risk metrics, inspect execution ledgers, and backtest algorithmic models.

---

## 1. Application Overview

### Core Functionality
- **Dynamic Policy Backtesting**: Run execution cycles across deterministic market price histories (e.g., volatile ACME_TECH, trending NOVA, declining STEEL).
- **Execution Telemetry & Risk Metrics**: Real-time evaluation of total realized profit, trade count, maximum peak-to-trough drawdown, and strategy validity status.
- **Interactive Execution Ledger**: Inspect sequential buy/sell orders, transaction timestamps, entry/exit cost basis, and realized profit per completed cycle.
- **RESTful State & Simulation Engine**: Bundled C++ HTTP server exposing endpoints for state hydration, configurable backtesting, and scenario resets.
- **Interactive Web UI**: Responsive dark-mode dashboard displaying interactive charts, telemetry cards, and strategy comparisons.


## 2. Debugging Challenge

Investment analytics and execution teams have flagged several critical discrepancies in TradeSim's strategy engine. Your goal is to investigate the C++ codebase in `src/`, identify the root causes of the execution failures, and implement the necessary fixes so that all automated test suites pass.

### Reported Issues & Tasks:

#### Issue 1: Single Trade Strategy Produces Zero Returns
- **User Symptom**: Running the optimal single-transaction strategy on the volatile ACME_TECH market yields $0.00 profit instead of the expected maximal buy-low/sell-high spread ($60.00).

#### Issue 2: Unlimited Trading Buys High and Sells Low
- **User Symptom**: The greedy unlimited trading policy reports severe negative returns (-$85.00) and triggers buy transactions even in continuously falling markets (such as STEEL).


#### Issue 3: Two-Trade Limit Mandate Exceeds Transaction Quota
- **User Symptom**: The limited-transaction policy violates executive risk limits by executing 3 complete round-trip cycles instead of the hard limit of 2 cycles.


#### Issue 4: Fee-Aware Strategy Over-Deducts Transaction Costs
- **User Symptom**: Under the $2 flat fee per completed cycle mandate, net realized gains are heavily understated ($111.00 instead of $127.00) and the execution ledger is flagged with an "Unexpected Result" badge.


#### Issue 5: Cooldown Strategy Violates Mandatory Idle Window
- **User Symptom**: Regulations require a 1-day mandatory cooling-off period after a sale before re-entering a position, but the ledger records re-entry on the immediate subsequent day (Day 2 after selling on Day 1).


#### Issue 6: Declining Market Generates Positive Trades
- **User Symptom**: In bear or declining market regimes (such as STEEL), algorithms execute loss-making or non-zero trade transactions instead of staying in cash.


---

## 3. Expected Behavior After Fixing Bugs

After resolving all issues:
1. `SingleTradeStrategy` yields the optimal $60.00 spread on ACME_TECH.
2. `UnlimitedStrategy` captures all local rises for $135.00 total profit on ACME_TECH.
3. `LimitedTransactionStrategy` executes at most 2 round-trip cycles with exactly $100.00 profit on ACME_TECH.
4. `FeeStrategy` deducts exactly $2 per completed cycle, resulting in $127.00 profit on ACME_TECH.
5. `CooldownStrategy` maintains a strict 1-day rest period after every sale with zero cooldown violations.
6. Bear markets (STEEL) generate zero transactions and $0.00 total profit.
7. Running `bash tests/run_tests.sh` passes all 6 tests with `"Passed": 6`, `"Failed": 0`, and exits with code `0`.
