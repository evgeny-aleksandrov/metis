from __future__ import annotations

import argparse
import json
import shutil
import subprocess
from pathlib import Path

from metis.data.ibkr import write_stock_history
from metis.data.storage import processed_price_path


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


def main() -> None:
    parser = argparse.ArgumentParser(description="Python orchestration for the C++ Metis backtester.")
    parser.add_argument("--repo-root", default=".")
    parser.add_argument("--symbol", default="SPY")
    parser.add_argument("--mode", choices=["grid", "walk-forward"], default="grid")
    parser.add_argument("--strategy", choices=["drop", "gain"], default="drop")
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
    parser.add_argument("--hold-min", type=int, default=10)
    parser.add_argument("--hold-max", type=int, default=60)
    parser.add_argument("--hold-step", type=int, default=5)
    parser.add_argument("--walk-train-months", type=int, default=24)
    parser.add_argument("--walk-test-months", type=int, default=6)
    parser.add_argument("--initial-equity", type=float, default=10000)
    run_backtest(parser.parse_args())


if __name__ == "__main__":
    main()
