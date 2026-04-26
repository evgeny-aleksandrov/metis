param(
  [string]$Symbol = "SPY",
  [string]$Period = "1y",
  [string]$Bar = "1d",
  [string]$OutputPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$env:PYTHONPATH = Join-Path $repoRoot "src"

$argsList = @(
  "-m", "metis.data.ibkr",
  "--symbol", $Symbol,
  "--period", $Period,
  "--bar", $Bar,
  "--data-dir", (Join-Path $repoRoot "data")
)

if ($OutputPath -ne "") {
  $argsList += @("--output", $OutputPath)
}

python $argsList

