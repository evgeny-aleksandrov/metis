# Metis

Hybrid Python/C++ trading research engine.

- **Python** handles orchestration, IBKR Gateway integration, and file IO.
- **C++** handles the fast backtest loop, grid search, strategy simulation, equity curves, trade lists, and metrics.
- **React** reads `ui/public/latest.json` and displays the latest backtest report.

## Strategies

The C++ engine currently supports two generic long-only stock strategies:

- `drop`: buy after the stock falls by at least `X%` over `Y` trading days.
- `gain`: buy after the stock rises by at least `X%` over `Y` trading days.

Both strategies then hold for a fixed number of days. The grid search sweeps threshold, lookback, and hold days.

## IBKR Historical Data

Before fetching data, start the IBKR Client Portal Gateway and complete the browser login flow. On macOS, Linux, or WSL2, run this from the extracted `clientportal.gw` directory:

```bash
bin/run.sh root/conf.yaml
```
