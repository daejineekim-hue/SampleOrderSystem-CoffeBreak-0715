$ErrorActionPreference = "Stop"

& "$PSScriptRoot\build.ps1" -Configuration Test

$testExe = "$PSScriptRoot\build\x64\Test\SampleOrderSystem.exe"
& $testExe
if ($LASTEXITCODE -ne 0) { throw "Tests failed" }
