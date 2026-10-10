param(
    [Parameter(Mandatory = $true)][string]$QtBin,
    [Parameter(Mandatory = $true)][string]$Iscc
)

$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $MyInvocation.MyCommand.Path
$stage = Join-Path $project 'build_auto\package_stage'
$release = Join-Path $project 'releases'
$binary = Join-Path $project 'build_auto\app\bin\NetworkConfigManager.exe'
if (-not (Test-Path -LiteralPath $binary)) { throw "Missing tested binary: $binary" }
if (-not (Test-Path -LiteralPath $Iscc)) { throw "Missing Inno Setup compiler: $Iscc" }
$resolvedProject = [IO.Path]::GetFullPath($project).TrimEnd('\') + '\'
$resolvedStage = [IO.Path]::GetFullPath($stage)
if (-not $resolvedStage.StartsWith($resolvedProject, [StringComparison]::OrdinalIgnoreCase)) {
    throw "Stage path outside project: $resolvedStage"
}
if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
New-Item -ItemType Directory -Path $stage, $release -Force | Out-Null
Copy-Item -LiteralPath $binary -Destination (Join-Path $stage 'NetworkConfigManager.exe')
Copy-Item -LiteralPath (Join-Path $project 'README.md') -Destination $stage
& (Join-Path $QtBin 'windeployqt.exe') --release --compiler-runtime --no-translations (Join-Path $stage 'NetworkConfigManager.exe')
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed: $LASTEXITCODE" }
$env:NETWORKCONFIG_STAGE = $stage
$env:NETWORKCONFIG_RELEASE = $release
& $Iscc (Join-Path $project 'installer.iss')
if ($LASTEXITCODE -ne 0) { throw "ISCC failed: $LASTEXITCODE" }
$zip = Join-Path $release 'NetworkConfigManager-2.3.0-win64-portable.zip'
if (Test-Path -LiteralPath $zip) { Remove-Item -LiteralPath $zip -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip
Get-FileHash -Algorithm SHA256 (Join-Path $release 'NetworkConfigManager-Setup-2.3.0-win64.exe'), $zip |
    Select-Object Path, Hash
