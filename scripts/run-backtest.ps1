param(
  [string]$CsvPath = "data/SPY.csv",
  [string]$OutputPath = "outputs/backtests/latest/report.json",
  [switch]$FetchIbkr,
  [string]$Symbol = "SPY",
  [ValidateSet("drop", "gain")]
  [string]$Strategy = "drop",
  [string]$Period = "1y",
  [string]$Bar = "1d",
  [double]$XMin = 0.01,
  [double]$XMax = 0.10,
  [double]$XStep = 0.01,
  [int]$YMin = 3,
  [int]$YMax = 20,
  [int]$YStep = 1,
  [int]$HoldMin = 10,
  [int]$HoldMax = 60,
  [int]$HoldStep = 5,
  [double]$InitialEquity = 10000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$env:PYTHONPATH = Join-Path $repoRoot "src"

$argsList = @(
  "-m", "metis.backtest.runner",
  "--repo-root", $repoRoot,
  "--symbol", $Symbol,
  "--strategy", $Strategy,
  "--output", $OutputPath,
  "--period", $Period,
  "--bar", $Bar,
  "--x-min", $XMin,
  "--x-max", $XMax,
  "--x-step", $XStep,
  "--y-min", $YMin,
  "--y-max", $YMax,
  "--y-step", $YStep,
  "--hold-min", $HoldMin,
  "--hold-max", $HoldMax,
  "--hold-step", $HoldStep,
  "--initial-equity", $InitialEquity
)

if ($CsvPath -ne "") {
  $argsList += @("--csv", $CsvPath)
}

if ($FetchIbkr) {
  $argsList += "--fetch-ibkr"
}

python $argsList
