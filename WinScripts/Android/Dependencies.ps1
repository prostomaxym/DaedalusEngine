[CmdletBinding()]
param(
	[string]$SdkRoot = $(if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA 'Android/Sdk' }),
	[string]$NdkVersion = '27.2.12479018',
	[string]$InstalledDirectory,
	[string]$WorkDirectory = (Join-Path ([System.IO.Path]::GetTempPath()) 'DaedalusAndroidDependencies'),
	[switch]$DryRun
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$repo_root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
# Windows PowerShell may not populate $PSScriptRoot during parameter default binding.
if (!$PSBoundParameters.ContainsKey('InstalledDirectory'))
{
	# Manifest reconciliation must not remove packages from the desktop graph.
	$InstalledDirectory = Join-Path $repo_root 'ThirdParty/vcpkg_installed_android'
}
$vcpkg_root = Join-Path $repo_root 'ThirdParty/vcpkg'
$vcpkg = Join-Path $vcpkg_root 'vcpkg.exe'
$ndk_root = Join-Path $SdkRoot "ndk/$NdkVersion"
if (!(Test-Path $vcpkg) -or !(Test-Path (Join-Path $ndk_root 'build/cmake/android.toolchain.cmake')))
{
	throw 'Bootstrap the repository vcpkg submodule and install the selected Android NDK first.'
}

New-Item -ItemType Directory -Force $WorkDirectory | Out-Null
New-Item -ItemType Directory -Force $InstalledDirectory | Out-Null
$InstalledDirectory = (Resolve-Path $InstalledDirectory).Path
$arguments = @(
	'install', "--vcpkg-root=$vcpkg_root", "--x-manifest-root=$repo_root",
	"--x-install-root=$InstalledDirectory", "--x-buildtrees-root=$WorkDirectory/buildtrees",
	"--x-packages-root=$WorkDirectory/packages",
	"--overlay-triplets=$repo_root/ThirdParty/vcpkg_triplets",
	'--triplet=arm64-android', '--host-triplet=x64-windows'
)
if ($DryRun) { $arguments += '--dry-run' }

$previous_ndk_home = $env:ANDROID_NDK_HOME
try
{
	$env:ANDROID_NDK_HOME = $ndk_root
	& $vcpkg @arguments
	if ($LASTEXITCODE -ne 0) { throw "Android dependency installation failed with exit code $LASTEXITCODE" }
}
finally
{
	$env:ANDROID_NDK_HOME = $previous_ndk_home
}
