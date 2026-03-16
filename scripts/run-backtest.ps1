param(
  [string]$CsvPath = "data/SPY.csv",
  [string]$OutputPath = "ui/public/latest.json",
  [double]$XMin = 0.01,
  [double]$XMax = 0.10,
  [double]$XStep = 0.01,
  [int]$YMin = 3,
  [int]$YMax = 20,
  [int]$YStep = 1,
  [int]$HoldDays = 10,
  [double]$InitialEquity = 10000
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$exePath = "build/Release/spy_dip_backtester.exe"
if (-not (Test-Path $exePath)) {
  $exePath = "build/spy_dip_backtester.exe"
}

if (-not (Test-Path $exePath)) {
  throw "Backtester executable not found. Run scripts/build-backtester.ps1 first."
}

& $exePath `
  --csv $CsvPath `
  --output $OutputPath `
  --initial $InitialEquity `
  --x-min $XMin `
  --x-max $XMax `
  --x-step $XStep `
  --y-min $YMin `
  --y-max $YMax `
  --y-step $YStep `
  --hold-days $HoldDays
