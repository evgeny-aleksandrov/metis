#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(cd "$script_dir/.." && pwd)"

sudo apt-get update
sudo apt-get install -y \
  build-essential \
  ca-certificates \
  cmake \
  curl \
  git \
  lldb \
  nodejs \
  npm \
  python3 \
  python3-full \
  python3-pip \
  python3-venv

python3 -m venv "$repo_root/.venv"
"$repo_root/.venv/bin/python" -m pip install --upgrade pip
"$repo_root/.venv/bin/python" -m pip install --editable "$repo_root"

METIS_BUILD_DIR="$repo_root/build-wsl" "$repo_root/scripts/build-backtester.sh"
mkdir -p "$repo_root/build"
cp "$repo_root/build-wsl/metis_backtester" "$repo_root/build/metis_backtester"

if [[ -f "$repo_root/ui/package-lock.json" ]]; then
  npm --prefix "$repo_root/ui" ci
else
  npm --prefix "$repo_root/ui" install
fi

echo
echo "WSL setup complete."
echo "Try:"
echo "  source .venv/bin/activate"
echo "  scripts/run-backtest.sh discrete-grid gain --csv data/SPY.csv --output ui/public/latest.json"
echo "  npm --prefix ui run dev -- --host 0.0.0.0"
