param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputDirectory = '',
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'BreakoutGame.uproject'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) { $OutputDirectory = Join-Path $projectRoot 'Saved\Packages\01_08' }
$archivePath = [IO.Path]::GetFullPath($OutputDirectory)
if ($archivePath -eq $projectRoot -or $archivePath -eq [IO.Path]::GetPathRoot($archivePath)) { throw 'Select a dedicated package output folder.' }
$automationTool = Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
if (!(Test-Path -LiteralPath $automationTool)) { throw "RunUAT not found: $automationTool" }
if (!(Test-Path -LiteralPath $projectFile)) { throw "Project not found: $projectFile" }
if (!$VerifyOnly) {
    if ((Get-PSDrive -Name ([IO.Path]::GetPathRoot($projectRoot).Substring(0,1))).Free -lt 4GB) { throw 'At least 4 GB of free workspace disk space is required before packaging.' }
    & $automationTool BuildCookRun '-nop4' "-project=$projectFile" '-platform=Win64' '-clientconfig=Shipping' '-build' '-clean' '-cook' '-map=TitleLevel+L_InGame+Level2' '-skipeditorcontent' '-stage' '-pak' '-iostore' '-compressed' '-package' '-distribution' '-prereqs' '-nodebuginfo' '-archive' "-archivedirectory=$archivePath" '-unattended' '-utf8output' '-ubtargs=-NoUBA -MaxParallelActions=1'
    if ($LASTEXITCODE -ne 0) { throw "Packaging failed with exit code $LASTEXITCODE. See the AutomationTool log." }
}
$windowsPath = Join-Path $archivePath 'Windows'
if (!(Test-Path -LiteralPath (Join-Path $windowsPath 'BreakoutGame.exe'))) { throw 'Packaged launcher is missing.' }
if (!(Test-Path -LiteralPath (Join-Path $windowsPath 'BreakoutGame\Binaries\Win64\BreakoutGame-Win64-Shipping.exe'))) { throw 'Shipping executable is missing.' }
$manifest = Get-Content -LiteralPath (Join-Path $windowsPath 'Manifest_UFSFiles_Win64.txt') -Raw
$required = @('TitleLevel.umap','L_InGame.umap','Level2.umap','Breakout/BP_Ball.uasset','Breakout/BP_Paddle.uasset','Breakout/BP_GameManager.uasset','Breakout/AddBallItem.uasset','Breakout/UI/TitleWidget.uasset','Breakout/UI/GameInformation.uasset','Breakout/UI/GameOverWidget.uasset','Breakout/UI/ClearWidget.uasset','Breakout/Input/IA_Action.uasset','Breakout/Input/IMC_InGame.uasset','Breakout/Audio/Breakout_SE_Knock.uasset')
foreach ($asset in $required) {
    if (!$manifest.Contains("BreakoutGame/Content/$asset`t")) { throw "Required cooked asset is missing: $asset" }
}
if ($manifest -match 'ModelContextProtocol') { throw 'Editor MCP content must not ship.' }
Write-Output "Verified $($required.Count) required cooked assets and the Shipping executable."
Write-Output "Packaged build: $archivePath\Windows\BreakoutGame.exe"
