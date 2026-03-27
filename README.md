# SPY Dip Trading Starter (C++ + React)

This project has:
- A **C++ backtesting engine** that tests: buy SPY when it changes by `X%` over `Y` days and hold for `hold` days
- A **TypeScript + React UI** that shows the best parameter combo and top results

## Project layout
- `backend/cpp`: C++ source for backtester + optimizer
- `ui`: React app (Vite) that reads `ui/public/latest.json`
- `data`: sourced from Twelve Data
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

### Optional: fetch 1 year of daily SPY data online

The repo can now fetch market data from Twelve Data's official API before running the backtest.

1. Create a free Twelve Data API key.
2. Set it in PowerShell:

```powershell
$env:TWELVE_DATA_API_KEY = "your_api_key_here"
```

3. Fetch 1 year of daily SPY data into `data/SPY.csv`:

```powershell
./scripts/fetch-twelvedata.ps1
```

You can also change the symbol or lookback window:

```powershell
./scripts/fetch-twelvedata.ps1 -Symbol SPY -YearsBack 1
```

## 2) Build and run backtest
From the repo root:

```powershell
./scripts/build-backtester.ps1
./scripts/run-backtest.ps1
```

This writes fresh results to `ui/public/latest.json`.

To fetch fresh online data first and then run the backtest:

```powershell
./scripts/run-backtest.ps1 -FetchOnline
```

This uses Twelve Data, writes the downloaded prices to `data/SPY.csv`, and then runs the C++ backtester against that file.

For a local debug build, use:

```powershell
cmake -S backend/cpp -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

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

## 4) Debug the C++ app locally

The repo includes a VS Code launch config at `.vscode/launch.json` for the `spy_dip_backtester` executable.

This repo is currently being built in VS Code with the `Ninja` generator and the Strawberry Perl `g++` toolchain, so the executable is created at `build/spy_dip_backtester.exe`. In this setup, VS Code should debug with `gdb`, not the Visual Studio debugger.

### Debug in VS Code

1. Build the backtester in debug mode:

```powershell
cmake -S backend/cpp -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

2. Open the repo folder in VS Code.
3. Go to **Run and Debug**.
4. Select the `Debug Backtester` launch target.
5. Set breakpoints in `backend/cpp/src/main.cpp` or `backend/cpp/src/backtest.cpp` and press `F5`.

The launch config uses the same arguments as `scripts/run-backtest.ps1`, so debugging writes to `ui/public/latest.json` by default.
VS Code now runs the `Compile Backtester` task automatically before launching the debugger, so `F5` will reconfigure and rebuild the C++ app first.

### Debug the data fetch script

There is also a `Debug Data Fetch` launch target in `.vscode/launch.json` for stepping through `scripts/fetch-twelvedata.ps1`.

Before using it, set your API key in the VS Code terminal:

```powershell
$env:TWELVE_DATA_API_KEY = "your_api_key_here"
```

Then:

1. Go to **Run and Debug**.
2. Select `Debug Data Fetch`.
3. Set breakpoints in `scripts/fetch-twelvedata.ps1`.
4. Press `F5`.

This lets you debug the API request, response handling, and CSV generation separately from the C++ app.

## What the strategy currently does
- Entry: if SPY has moved by at least `X%` versus `Y` trading days ago, buy with all cash.
- Exit: hold the position certain number of days (`hold_days`), then sell.
- Optimizer: brute-force grid search over `(X, Y, hold_days)` and picks the best by CAGR.