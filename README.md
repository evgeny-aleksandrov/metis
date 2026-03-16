# SPY Dip Trading Starter (C++ + React)

This project has:
- A **C++ backtesting engine** that tests: buy SPY when it drops by `X%` over `Y` days
- A **TypeScript + React UI** that shows the best parameter combo and top results

## Project layout
- `backend/cpp`: C++ source for backtester + optimizer
- `ui`: React app (Vite) that reads `ui/public/latest.json`
- `data`: place your `SPY.csv` historical data here
- `scripts`: PowerShell helpers for build/run

## Prerequisites
- Windows + PowerShell
- CMake (3.16+)
- A C++ compiler (Visual Studio Build Tools / MSVC)
- Node.js 18+ and npm

## 1) Prepare data
1. Put your CSV at `data/SPY.csv`.
2. If you just want to test quickly:
   - Copy `data/SPY.csv.example` to `data/SPY.csv`.

## 2) Build and run backtest
From the repo root:

```powershell
./scripts/build-backtester.ps1
./scripts/run-backtest.ps1
```

This writes fresh results to `ui/public/latest.json`.

You can tune the search range, for example:

```powershell
./scripts/run-backtest.ps1 -XMin 0.01 -XMax 0.08 -XStep 0.005 -YMin 2 -YMax 30 -HoldDays 8
```

## 3) Start UI

```powershell
cd ui
npm install
npm run dev
```

Open the URL shown by Vite (usually `http://localhost:5173`).

## What the strategy currently does
- Entry: if SPY is down at least `X%` versus `Y` trading days ago, buy with all cash.
- Exit: hold the position for a fixed number of days (`hold_days`, default 10), then sell.
- Optimizer: brute-force grid search over `(X, Y)` and picks the best by CAGR.

## Next upgrades you can add
- Transaction costs and slippage
- Position sizing rules
- Stop-loss / take-profit logic
- Walk-forward and out-of-sample testing
- Multiple symbols and more indicators
