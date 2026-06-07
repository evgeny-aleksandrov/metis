# Metis

Hybrid Python/C++ trading research platform.

- **Python** handles orchestration, IBKR Gateway integration, and file IO.
- **C++** is the core research platform: typed run configs, approach runners, walk-forward validation, optimizers, strategy simulation, equity curves, trade lists, and metrics.
- **React** reads `ui/public/latest.json` and displays the latest backtest report.

## C++ Research Approaches

The C++ side is organized around approaches, each with its own strategy/config surface, and walk-forward validation is the main research path.

- `buy-and-hold`: benchmark and first-class approach. Currently supports `lump-sum`; scheduled buying is planned.
- `discrete-grid`: implemented grid-search approach for rule-based strategies. Current strategies are `drop`, `gain`, and `regime`.
- `ridge`: in-progress approach for regression-based signals. CLI/config structure exists; execution is not implemented yet.

Example:

```bash
build/metis_backtester discrete-grid gain --csv data/SPY.csv --output ui/public/latest.json
```

## IBKR Historical Data

Before fetching data, start the IBKR Client Portal Gateway and complete the browser login flow. On macOS, Linux, or WSL2, run this from the extracted `clientportal.gw` directory:

```bash
bin/run.sh root/conf.yaml
```
