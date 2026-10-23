param(
    [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9]+$')][string]$ClassCode,
    [Parameter(Mandatory)][ValidatePattern('^[0-9]{2}$')][string]$AttendanceNumber,
    [Parameter(Mandatory)][ValidatePattern('^[\u30A1-\u30F6\u30FC]+$')][string]$FamilyName,
    [Parameter(Mandatory)][ValidatePattern('^[\u30A1-\u30F6\u30FC]+$')][string]$GivenName,
    [string]$PackageDirectory = '',
    [string]$OutputDirectory = ''
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($PackageDirectory)) { $PackageDirectory = Join-Path $projectRoot 'Saved\Packages\Evaluation01\Windows' }
$source = [IO.Path]::GetFullPath($PackageDirectory).TrimEnd([IO.Path]::DirectorySeparatorChar)
$readme = Join-Path $projectRoot 'README.md'
$baseName = "ND1_${ClassCode}_${AttendanceNumber}_${FamilyName}_${GivenName}_Test01"
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) { $OutputDirectory = Join-Path $projectRoot 'Saved\Submissions\GameLoop' }
$outputRoot = [IO.Path]::GetFullPath($OutputDirectory)
$folder = Join-Path $outputRoot $baseName
$zipPath = Join-Path $outputRoot "$baseName.zip"
if ((Test-Path -LiteralPath $folder) -or (Test-Path -LiteralPath $zipPath)) {
    throw 'Submission output already exists. Preserve it or choose another output location before retrying.'
}
if (!(Test-Path -LiteralPath (Join-Path $source 'BreakoutGame.exe'))) { throw 'Build the Windows package first.' }
if (!(Test-Path -LiteralPath $readme)) { throw 'README.md is missing.' }
# Staging manifests are packaging diagnostics, not runtime dependencies.
# Keep the rest of the UAT output, including runtime DLLs, prerequisites and notices.
$files = @(Get-ChildItem -LiteralPath $source -Recurse -File | Where-Object {
    $_.FullName -notin @((Join-Path $source 'Manifest_UFSFiles_Win64.txt'), (Join-Path $source 'Manifest_NonUFSFiles_Win64.txt'))
})
foreach ($file in $files) {
    $relative = $file.FullName.Substring($source.Length + 1)
    $target = Join-Path $folder $relative
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $file.FullName -Destination $target
}
Copy-Item -LiteralPath $readme -Destination (Join-Path $folder 'readme.md')
Add-Type -AssemblyName System.IO.Compression.FileSystem
[IO.Compression.ZipFile]::CreateFromDirectory($folder, $zipPath, [IO.Compression.CompressionLevel]::Optimal, $true)
# Read every ZIP entry back and compare its contents against the staged file.
$zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $entries = @($zip.Entries | Where-Object { $_.Name })
    if ($entries.Count -ne $files.Count + 1) { throw 'Unexpected submission entry count.' }
    foreach ($entry in $entries) {
        $relative = $entry.FullName.Substring($baseName.Length + 1)
        $expected = Get-FileHash -LiteralPath (Join-Path $folder $relative) -Algorithm SHA256
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
        finally { $sha.Dispose(); $stream.Dispose() }
        if ($actual -ne $expected.Hash) { throw "ZIP verification failed: $relative" }
    }
} finally { $zip.Dispose() }
Write-Output "Verified $($files.Count + 1) submission files."
$zipFile = Get-Item -LiteralPath $zipPath
[PSCustomObject]@{
    Path = $zipFile.FullName
    Bytes = $zipFile.Length
    SHA256 = (Get-FileHash -LiteralPath $zipPath -Algorithm SHA256).Hash
} | ConvertTo-Json
