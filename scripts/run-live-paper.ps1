param(
  [string]$Symbol = "SPY",
  [string]$StatePath = "outputs/live/live_state.json"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$env:PYTHONPATH = Join-Path $repoRoot "src"

python -m metis.live.runner --symbol $Symbol --paper --state (Join-Path $repoRoot $StatePath)
