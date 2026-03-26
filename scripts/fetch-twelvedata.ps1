param(
  [string]$Symbol = "SPY",
  [string]$OutputPath = "data/SPY.csv",
  [string]$ApiKey = $env:TWELVE_DATA_API_KEY,
  [int]$YearsBack = 1,
  [string]$Interval = "1day"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ApiKey)) {
  throw "Twelve Data API key is required. Pass -ApiKey or set TWELVE_DATA_API_KEY."
}

if ($YearsBack -lt 1) {
  throw "YearsBack must be at least 1."
}

$endDate = Get-Date
$startDate = $endDate.AddYears(-$YearsBack)

$params = @{
  symbol = $Symbol
  interval = $Interval
  start_date = $startDate.ToString("yyyy-MM-dd")
  end_date = $endDate.ToString("yyyy-MM-dd")
  order = "asc"
  format = "JSON"
  adjust = "all"
  apikey = $ApiKey
}

$queryString = ($params.GetEnumerator() | ForEach-Object {
  "{0}={1}" -f [uri]::EscapeDataString($_.Key), [uri]::EscapeDataString([string]$_.Value)
}) -join "&"

$uri = "https://api.twelvedata.com/time_series?$queryString"
$response = Invoke-RestMethod -Uri $uri -Method Get

if ($response.status -and $response.status -ne "ok") {
  $message = if ($response.message) { $response.message } else { "Unknown Twelve Data API error." }
  throw "Twelve Data request failed: $message"
}

if (-not $response.values -or $response.values.Count -lt 10) {
  throw "Not enough market data returned for symbol $Symbol."
}

$outputDirectory = Split-Path -Parent $OutputPath
if ($outputDirectory) {
  New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
}

$csvLines = New-Object System.Collections.Generic.List[string]
$csvLines.Add("Date,Close")

foreach ($row in $response.values) {
  if (-not $row.datetime -or -not $row.close) {
    continue
  }
  $csvLines.Add(("{0},{1}" -f $row.datetime, $row.close))
}

if ($csvLines.Count -lt 11) {
  throw "Fetched data did not contain enough usable rows."
}

Set-Content -Path $OutputPath -Value $csvLines -Encoding ascii

Write-Host "Fetched $($csvLines.Count - 1) rows for $Symbol from Twelve Data."
Write-Host "Saved CSV: $OutputPath"
