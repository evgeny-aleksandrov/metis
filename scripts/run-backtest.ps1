param(
  [string]$CsvPath = "data/SPY.csv",
  [string]$OutputPath = "ui/public/latest.json",
  [switch]$FetchOnline,
  [string]$Symbol = "SPY",
  [string]$ApiKey = $env:TWELVE_DATA_API_KEY,
  [int]$YearsBack = 1,
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

if ($FetchOnline) {
  $scriptPath = Join-Path $PSScriptRoot "fetch-twelvedata.ps1"
  & $scriptPath `
    -Symbol $Symbol `
    -OutputPath $CsvPath `
    -ApiKey $ApiKey `
    -YearsBack $YearsBack
}

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
