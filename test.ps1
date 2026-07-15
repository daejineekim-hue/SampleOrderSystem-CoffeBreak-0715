param(
    [string]$Configuration = "Debug"
)
$ErrorActionPreference = "Stop"

& "$PSScriptRoot\build.ps1" -Configuration $Configuration

$testExe = "$PSScriptRoot\build\x64\$Configuration\Tests.exe"
& $testExe
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
