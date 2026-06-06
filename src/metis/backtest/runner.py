from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path

from metis.data.ibkr import write_stock_history
from metis.data.storage import processed_price_path


DEFAULT_HOURLY_BARS_PER_YEAR = 252 * 6.5


def find_backtester(repo_root: Path) -> Path:
    candidates = [
        repo_root / "build" / "metis_backtester",
        repo_root / "build" / "Debug" / "metis_backtester",
        repo_root / "build" / "Release" / "metis_backtester",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    raise FileNotFoundError("Backtester executable not found. Run scripts/build-backtester.sh first.")


def run_backtest(args: argparse.Namespace) -> Path:
    repo_root = Path(args.repo_root).resolve()
    csv_path = Path(args.csv) if args.csv else processed_price_path(repo_root / args.data_dir, args.symbol, args.bar)
    if args.fetch_ibkr:
        write_stock_history(csv_path, symbol=args.symbol, period=args.period, bar=args.bar)
    if not csv_path.exists():
        raise FileNotFoundError(f"Missing input CSV: {csv_path}")

    output_path = Path(args.output)
    if not output_path.is_absolute():
        output_path = repo_root / output_path
    output_path.parent.mkdir(parents=True, exist_ok=True)
    bars_per_year = args.bars_per_year if args.bars_per_year is not None else infer_bars_per_year(args.bar)

    exe = find_backtester(repo_root)
    command = [
        str(exe),
        "--symbol",
        args.symbol.upper(),
        "--mode",
        args.mode,
        "--strategy",
        args.strategy,
        "--csv",
        str(csv_path),
        "--output",
        str(output_path),
        "--initial",
        str(args.initial_equity),
        "--fixed-cost",
        str(args.fixed_cost),
        "--variable-cost-rate",
        str(args.variable_cost_rate),
        "--bars-per-year",
        str(bars_per_year),
        "--x-min",
        str(args.x_min),
        "--x-max",
        str(args.x_max),
        "--x-step",
        str(args.x_step),
        "--y-min",
        str(args.y_min),
        "--y-max",
        str(args.y_max),
        "--y-step",
        str(args.y_step),
        "--fast-lookback-min",
        str(args.fast_lookback_min),
        "--fast-lookback-max",
        str(args.fast_lookback_max),
        "--fast-lookback-step",
        str(args.fast_lookback_step),
        "--short-y-min",
        str(args.short_y_min),
        "--short-y-max",
        str(args.short_y_max),
        "--short-y-step",
        str(args.short_y_step),
        "--short-fast-lookback-min",
        str(args.short_fast_lookback_min),
        "--short-fast-lookback-max",
        str(args.short_fast_lookback_max),
        "--short-fast-lookback-step",
        str(args.short_fast_lookback_step),
        "--hold-min",
        str(args.hold_min),
        "--hold-max",
        str(args.hold_max),
        "--hold-step",
        str(args.hold_step),
        "--walk-train-months",
        str(args.walk_train_months),
        "--walk-test-months",
        str(args.walk_test_months),
        "--selection-mode",
        args.selection_mode,
        "--selection-top-k",
        str(args.selection_top_k),
        "--take-profit-min",
        str(args.take_profit_min),
        "--take-profit-max",
        str(args.take_profit_max),
        "--take-profit-step",
        str(args.take_profit_step),
        "--stop-loss-min",
        str(args.stop_loss_min),
        "--stop-loss-max",
        str(args.stop_loss_max),
        "--stop-loss-step",
        str(args.stop_loss_step),
        "--trailing-stop-min",
        str(args.trailing_stop_min),
        "--trailing-stop-max",
        str(args.trailing_stop_max),
        "--trailing-stop-step",
        str(args.trailing_stop_step),
        "--regime-weak-exit-mode",
        args.regime_weak_exit_mode,
        "--short-mode",
        args.short_mode,
        "--vol-lookback-min",
        str(args.vol_lookback_min),
        "--vol-lookback-max",
        str(args.vol_lookback_max),
        "--vol-lookback-step",
        str(args.vol_lookback_step),
        "--target-vol-min",
        str(args.target_vol_min),
        "--target-vol-max",
        str(args.target_vol_max),
        "--target-vol-step",
        str(args.target_vol_step),
        "--max-position-pct-min",
        str(args.max_position_pct_min),
        "--max-position-pct-max",
        str(args.max_position_pct_max),
        "--max-position-pct-step",
        str(args.max_position_pct_step),
        "--grid-sample-pct",
        str(args.grid_sample_pct),
        "--grid-sample-seed",
        str(args.grid_sample_seed),
        "--grid-random-samples",
        str(args.grid_random_samples),
    ]
    subprocess.run(command, check=True)

    latest_path = repo_root / "ui" / "public" / "latest.json"
    if output_path.resolve() != latest_path.resolve():
        latest_path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(output_path, latest_path)

    with output_path.open("r", encoding="utf-8") as handle:
        payload = json.load(handle)
    cagr_label = "walk-forward CAGR" if payload.get("mode") == "walk-forward" else "best CAGR"
    print(f"{payload['symbol']} {payload['strategy']} {cagr_label} {payload['best']['cagr'] * 100:.2f}% from {payload['bars']} bars")
    return output_path


def infer_bars_per_year(bar: str) -> float:
    normalized = bar.strip().lower()
    if normalized in {"1h", "1 hour"}:
        return DEFAULT_HOURLY_BARS_PER_YEAR
    return 252.0


def main() -> None:
    parser = argparse.ArgumentParser(description="Python orchestration for the C++ Metis backtester.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--symbol", default="SPY")
    parser.add_argument("--mode", choices=["grid", "walk-forward"], default="grid")
    parser.add_argument("--strategy", choices=["drop", "gain", "regime"], default="drop")
    parser.add_argument("--csv")
    parser.add_argument("--output", default="outputs/backtests/latest/report.json")
    parser.add_argument("--data-dir", default="data")
    parser.add_argument("--fetch-ibkr", action="store_true")
    parser.add_argument("--period", default="1y")
    parser.add_argument("--bar", default="1d")
    parser.add_argument("--x-min", type=float, default=0.01)
    parser.add_argument("--x-max", type=float, default=0.10)
    parser.add_argument("--x-step", type=float, default=0.01)
    parser.add_argument("--y-min", type=int, default=3)
    parser.add_argument("--y-max", type=int, default=20)
    parser.add_argument("--y-step", type=int, default=1)
    parser.add_argument("--fast-lookback-min", type=int, default=2)
    parser.add_argument("--fast-lookback-max", type=int, default=10)
    parser.add_argument("--fast-lookback-step", type=int, default=1)
    parser.add_argument("--short-y-min", type=int, default=0)
    parser.add_argument("--short-y-max", type=int, default=0)
    parser.add_argument("--short-y-step", type=int, default=1)
    parser.add_argument("--short-fast-lookback-min", type=int, default=0)
    parser.add_argument("--short-fast-lookback-max", type=int, default=0)
    parser.add_argument("--short-fast-lookback-step", type=int, default=1)
    parser.add_argument("--hold-min", type=int, default=10)
    parser.add_argument("--hold-max", type=int, default=60)
    parser.add_argument("--hold-step", type=int, default=5)
    parser.add_argument("--walk-train-months", type=int, default=24)
    parser.add_argument("--walk-test-months", type=int, default=6)
    parser.add_argument("--selection-mode", choices=["best", "top-k-majority", "top-k-ranked-vote"], default="best")
    parser.add_argument("--selection-top-k", type=int, default=10)
    parser.add_argument("--take-profit-min", type=float, default=0.0)
    parser.add_argument("--take-profit-max", type=float, default=0.0)
    parser.add_argument("--take-profit-step", type=float, default=0.01)
    parser.add_argument("--stop-loss-min", type=float, default=0.0)
    parser.add_argument("--stop-loss-max", type=float, default=0.0)
    parser.add_argument("--stop-loss-step", type=float, default=0.01)
    parser.add_argument("--trailing-stop-min", type=float, default=0.0)
    parser.add_argument("--trailing-stop-max", type=float, default=0.0)
    parser.add_argument("--trailing-stop-step", type=float, default=0.01)
    parser.add_argument("--regime-weak-exit-mode", choices=["off", "on", "both"], default="off")
    parser.add_argument("--short-mode", choices=["off", "on", "both"], default="off")
    parser.add_argument("--vol-lookback-min", type=int, default=0)
    parser.add_argument("--vol-lookback-max", type=int, default=0)
    parser.add_argument("--vol-lookback-step", type=int, default=1)
    parser.add_argument("--target-vol-min", type=float, default=0.0)
    parser.add_argument("--target-vol-max", type=float, default=0.0)
    parser.add_argument("--target-vol-step", type=float, default=0.01)
    parser.add_argument("--max-position-pct-min", type=float, default=1.0)
    parser.add_argument("--max-position-pct-max", type=float, default=1.0)
    parser.add_argument("--max-position-pct-step", type=float, default=0.25)
    parser.add_argument("--grid-sample-pct", type=float, default=1.0)
    parser.add_argument("--grid-sample-seed", type=int, default=42)
    parser.add_argument("--grid-random-samples", type=int, default=0)
    parser.add_argument("--initial-equity", type=float, default=10000)
    parser.add_argument("--bars-per-year", type=float, help="Override CAGR/Sharpe annualization. Defaults to 1638.0 for 1h bars, otherwise 252.")
    parser.add_argument("--fixed-cost", type=float, default=1.0, help="Fixed transaction cost per buy/sell order.")
    parser.add_argument(
        "--variable-cost-rate",
        type=float,
        default=0.000045,
        help="Variable transaction cost rate per buy/sell order. Default approximates LSE ETF third-party fee.",
    )
    run_backtest(parser.parse_args())


if __name__ == "__main__":
    main()
