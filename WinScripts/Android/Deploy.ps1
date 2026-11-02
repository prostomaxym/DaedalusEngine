[CmdletBinding()]
param(
	[string]$SdkRoot = $(if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA 'Android/Sdk' }),
	[string]$ApkPath,
	[string]$Serial
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Checked([string]$program, [string[]]$arguments)
{
	& $program @arguments
	if ($LASTEXITCODE -ne 0) { throw "$program failed with exit code $LASTEXITCODE" }
}

# Resolve script-relative defaults after parameter binding for Windows PowerShell.
if (!$PSBoundParameters.ContainsKey('ApkPath'))
{
	$repo_root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
	$ApkPath = Join-Path $repo_root 'runtime/DaedalusAndroidApk/gradle/app/outputs/apk/debug/app-debug.apk'
}
$adb = Join-Path $SdkRoot 'platform-tools/adb.exe'
foreach ($required in @($adb, $ApkPath))
{
	if (!(Test-Path $required -PathType Leaf)) { throw "File not found: $required" }
}
$devices = @(Invoke-Checked $adb @('devices'))
$ready = @($devices | ForEach-Object { if ($_ -match '^(\S+)\s+device$') { $Matches[1] } })
if ($Serial)
{
	if ($Serial -notin $ready) { throw "Device '$Serial' is not connected and authorized. Check adb devices and unlock/authorize the device." }
}
elseif ($ready.Count -eq 1)
{
	$Serial = $ready[0]
}
else
{
	throw "Expected one authorized device; found $($ready.Count). Connect/authorize a device or specify -Serial."
}

Invoke-Checked $adb @('-s', $Serial, 'install', '-r', (Resolve-Path $ApkPath).Path)
Invoke-Checked $adb @('-s', $Serial, 'shell', 'am', 'force-stop', 'org.daedalus.sandbox')
$launch = @(Invoke-Checked $adb @('-s', $Serial, 'shell', 'am', 'start', '-W', '-n', 'org.daedalus.sandbox/android.app.NativeActivity'))
$launch | Write-Output
if (!($launch -match '^Status: ok')) { throw 'Android did not report a successful activity launch.' }
Start-Sleep -Seconds 3
$process_id = @(Invoke-Checked $adb @('-s', $Serial, 'shell', 'pidof', 'org.daedalus.sandbox'))
if (!$process_id.Count) { throw 'Sandbox process is not running after launch. Inspect logcat for details.' }
Write-Output "Deployed to $Serial; sandbox PID: $process_id. Existing app data preserved."
