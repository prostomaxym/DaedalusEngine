[CmdletBinding()]
param(
	[string]$SdkRoot = $(if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA 'Android/Sdk' }),
	[string]$NdkVersion = '27.2.12479018',
	[string]$CMakeVersion = '3.22.1',
	[string]$Serial,
	[string]$BuildDirectory = (Join-Path ([System.IO.Path]::GetTempPath()) "DaedalusAndroidProbe-$NdkVersion-$CMakeVersion")
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Checked
{
	param([string]$Executable, [string[]]$Arguments)
	& $Executable @Arguments
	if ($LASTEXITCODE -ne 0)
	{
		throw "$Executable failed with exit code $LASTEXITCODE"
	}
}

$repo_root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$ndk_root = Join-Path $SdkRoot "ndk/$NdkVersion"
$adb = Join-Path $SdkRoot 'platform-tools/adb.exe'
$cmake = Join-Path $SdkRoot "cmake/$CMakeVersion/bin/cmake.exe"
$ninja = Join-Path $SdkRoot "cmake/$CMakeVersion/bin/ninja.exe"
if (!(Test-Path $adb) -or !(Test-Path $cmake) -or !(Test-Path $ninja) -or !(Test-Path (Join-Path $ndk_root 'build/cmake/android.toolchain.cmake')))
{
	throw 'Android platform-tools and the selected NDK/CMake must be installed. Pass -SdkRoot, -NdkVersion and -CMakeVersion if necessary.'
}

$device_arguments = @()
if ($Serial) { $device_arguments = @('-s', $Serial) }
Invoke-Checked $adb ($device_arguments + @('get-state'))

Invoke-Checked $cmake @(
	'-S', (Join-Path $repo_root 'source/dlAndroidProbe'),
	'-B', $BuildDirectory, '-G', 'Ninja',
	"-DCMAKE_MAKE_PROGRAM=$ninja",
	"-DCMAKE_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake",
	'-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28',
	'-DANDROID_STL=c++_static', '-DCMAKE_BUILD_TYPE=Release'
)
Invoke-Checked $cmake @('--build', $BuildDirectory)

# An isolated shell executable tests GLES without changing or installing an application.
$remote_root = '/data/local/tmp/daedalus-probe-' + [Guid]::NewGuid().ToString('N')
try
{
	Invoke-Checked $adb ($device_arguments + @('shell', 'mkdir', '-p', "$remote_root/shaders"))
	Invoke-Checked $adb ($device_arguments + @('push', (Join-Path $BuildDirectory 'dlAndroidProbe'), "$remote_root/probe"))
	Get-ChildItem (Join-Path $repo_root 'source/shader') -File |
		Where-Object { $_.Extension -in @('.glsl', '.hglsl') } |
		ForEach-Object { Invoke-Checked $adb ($device_arguments + @('push', $_.FullName, "$remote_root/shaders/$($_.Name)")) }
	Invoke-Checked $adb ($device_arguments + @('shell', 'chmod', '755', "$remote_root/probe"))

	$remote_command = "echo DAEDALUS_PROBE_PID=`$`$; exec $remote_root/probe $remote_root/shaders"
	$probe_output = @(& $adb @device_arguments shell $remote_command)
	$probe_exit = $LASTEXITCODE
	$pid_line = $probe_output | Where-Object { $_ -match '^DAEDALUS_PROBE_PID=(\d+)\s*$' } | Select-Object -First 1
	if (!$pid_line) { throw 'Device did not report the probe process ID.' }
	$probe_pid = ($pid_line -split '=')[1].Trim()
	Invoke-Checked $adb ($device_arguments + @('logcat', '-d', "--pid=$probe_pid", '-s', 'DaedalusProbe:I', '*:S'))
	if ($probe_exit -ne 0) { throw "Device feasibility probe failed with exit code $probe_exit" }
	Write-Host 'Android GLES feasibility probe passed. This does not validate NativeActivity lifecycle or rendering performance.'
}
finally
{
	& $adb @device_arguments shell rm -rf $remote_root
	if ($LASTEXITCODE -ne 0) { Write-Warning "Could not remove device probe directory $remote_root" }
}
