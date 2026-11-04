param([Parameter(Mandatory=$true)][string]$Exe)
$ErrorActionPreference = 'Stop'
$exeFull = (Resolve-Path $Exe).Path
Write-Host 'SHA256:'
Get-FileHash $exeFull -Algorithm SHA256 | Format-List
Write-Host 'Dependency test: a fresh Windows installation without MSYS2/MPI is required for final release.'
Write-Host 'Running regression tests (Python 3 required)...'
python "$PSScriptRoot/run_regression.py" --exe "$exeFull" --json "$PSScriptRoot/results.json"
if ($LASTEXITCODE -ne 0) { throw "Regression test failed: $LASTEXITCODE" }
