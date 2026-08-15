param(
    [Parameter(Mandatory = $true)]
    [string]$ApkPath,
    [string]$ExecutablePath = ".\\build\\Release\\setu_runtime.exe",
    [string]$LogPath = ".\\logs\\setu-integration.log",
    [int]$StartupTimeoutSeconds = 12
)

$ErrorActionPreference = "Stop"
if (-not (Test-Path -LiteralPath $ApkPath)) { throw "APK not found: $ApkPath" }
if (-not (Test-Path -LiteralPath $ExecutablePath)) { throw "Executable not found: $ExecutablePath" }

$logDirectory = Split-Path -Parent $LogPath
if ($logDirectory) { New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null }
$errorLogPath = "$LogPath.stderr"
Remove-Item -LiteralPath $LogPath, $errorLogPath -ErrorAction SilentlyContinue

$process = Start-Process -FilePath $ExecutablePath -ArgumentList @($ApkPath) -PassThru `
    -RedirectStandardOutput $LogPath -RedirectStandardError $errorLogPath
Start-Sleep -Seconds $StartupTimeoutSeconds
if (-not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
}

$log = (Get-Content -LiteralPath $LogPath -Raw) + (Get-Content -LiteralPath $errorLogPath -Raw -ErrorAction SilentlyContinue)
if ($log -match "Missing required view|Unimplemented stub: Lq1/b;->g") {
    throw "View binding failed; inspect $LogPath"
}
if ($log -notmatch "VIEW HIERARCHY DUMP AFTER ROOT LAYOUT") {
    throw "Layout inflation did not attach a root view; inspect $LogPath"
}

Write-Host "Integration smoke check passed."
