param(
    [ValidateSet("Debug", "Release", "Test")]
    [string]$Configuration = "Debug"
)
$ErrorActionPreference = "Stop"

$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\amd64\MSBuild.exe"
if (-not (Test-Path $msbuild)) {
    throw "MSBuild.exe not found at expected path. Adjust build.ps1 for your Visual Studio install."
}

& $msbuild "$PSScriptRoot\SampleOrderSystem.sln" /p:Configuration=$Configuration /p:Platform=x64 /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Build failed" }

Write-Output "Build succeeded: build\x64\$Configuration\SampleOrderSystem.exe"
