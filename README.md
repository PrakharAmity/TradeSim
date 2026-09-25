# TradeSim — Strategy Backtesting & Execution Engine

## Product overview

TradeSim is a local strategy analytics service used by portfolio and execution teams to compare trading policies against historical closing prices. Its dashboard shows market movement, policy returns, drawdown, transaction counts, and the execution ledger. All values shown in the browser come from the C++ backtest service.

## Incident report

Investment analytics has reported that policies with brokerage costs and mandatory cooldown restrictions are producing results that do not match the published rules. Some reports understate net yield, and some execution logs re-enter the market before the required wait period has elapsed. Reproduce the reports in the dashboard, inspect the transaction log, and trace the strategy implementations.

## Strategy catalog

- **Single Trade** — one buy followed by one later sale.
- **Unlimited Trading** — sequential, non-overlapping cycles.
- **Two-Trade Limit** — no more than two completed cycles.
- **Fee-Aware** — sequential cycles with one fixed $2 fee per completed cycle.
- **Cooldown** — a one-session wait after a sale before the next entry.
- **Combined Policy** — at most two cycles, with the fee and cooldown rules applied together.

The three bundled market histories (ACME, NOVA, and STEEL) are deterministic so a report can be reproduced locally.

## Build and run

Requires CMake 3.16+ and a C++17 compiler such as GCC 11+ or Clang. The HTTP and JSON support headers are bundled; no package downloads, npm, or external services are used.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/tradesim
```

The C++ server listens on `0.0.0.0:3000` by default and serves the dashboard at `http://localhost:3000`. Set `PORT` to select another port. Run `bash start.sh` to configure, build, and start the preview in one command. Launch the executable from this repository directory so its `web/` assets are found.

## Candidate workflow

1. Choose a market and policy in the dashboard, then run the backtest.
2. Compare the status badges, return figures, and transaction log.
3. Trace the corresponding strategy and portfolio code in `src/` and `include/`.
4. Rebuild and run the affected scenario again.
5. Run the automated challenge suite:

```bash
bash tests/run_tests.sh
```

The challenge starts with four seeded defects in the fee-aware and cooldown execution paths. The test command writes only its JSON report to standard output; CMake diagnostics go to standard error. `AI.md` is mentor material and must not be shown to candidates.

## Preview API

- `GET /api/state` returns the bundled markets and available policy selectors.
- `POST /api/backtest` accepts `{"strategy":"all","ticker":"ACME_TECH"}` and returns the selected reports, daily prices, and trade records.
- `POST /api/reset` restores the default scenario configuration.
