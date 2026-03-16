Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

cmake -S backend/cpp -B build
cmake --build build --config Release

