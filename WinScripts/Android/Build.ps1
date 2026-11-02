[CmdletBinding()]
param(
	[string]$SdkRoot = $(if ($env:ANDROID_HOME) { $env:ANDROID_HOME } else { Join-Path $env:LOCALAPPDATA 'Android/Sdk' }),
	[string]$NdkVersion = '27.2.12479018',
	[string]$CMakeVersion = '3.22.1',
	[ValidateSet('Release', 'ReleaseDevTools')]
	[string]$Configuration = 'Release',
	[string]$InstalledDirectory,
	[string]$WorkDirectory,
	[string]$GradlePath,
	[string]$JavaHome = $env:JAVA_HOME,
	[switch]$Deploy,
	[string]$Serial
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

function Invoke-Checked([string]$program, [string[]]$arguments)
{
	& $program @arguments
	if ($LASTEXITCODE -ne 0) { throw "$program failed with exit code $LASTEXITCODE" }
}

$repo_root = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
# Windows PowerShell may not populate $PSScriptRoot during parameter default binding.
if (!$PSBoundParameters.ContainsKey('InstalledDirectory'))
{
	$InstalledDirectory = Join-Path $repo_root 'ThirdParty/vcpkg_installed_android'
}
if (!$PSBoundParameters.ContainsKey('WorkDirectory'))
{
	$WorkDirectory = Join-Path $repo_root 'runtime/DaedalusAndroidApk'
}
$SdkRoot = (Resolve-Path $SdkRoot).Path
$ndk_root = Join-Path $SdkRoot "ndk/$NdkVersion"
$cmake = Join-Path $SdkRoot "cmake/$CMakeVersion/bin/cmake.exe"
$ninja = Join-Path $SdkRoot "cmake/$CMakeVersion/bin/ninja.exe"
$toolchain = Join-Path $repo_root 'ThirdParty/vcpkg/scripts/buildsystems/vcpkg.cmake'
$installed = (Resolve-Path $InstalledDirectory).Path
$cxx_runtime = Join-Path $ndk_root 'toolchains/llvm/prebuilt/windows-x86_64/sysroot/usr/lib/aarch64-linux-android/libc++_shared.so'
foreach ($required in @($cmake, $ninja, $toolchain, $cxx_runtime, "$installed/arm64-android/share/entt/EnTTConfig.cmake"))
{
	if (!(Test-Path $required)) { throw "Missing prerequisite: $required. Run Dependencies.ps1 with the same NDK and InstalledDirectory first." }
}

if (!$GradlePath)
{
	$command = Get-Command gradle.bat -ErrorAction SilentlyContinue
	if ($command) { $GradlePath = $command.Source }
	else
	{
		$cached = @(Get-ChildItem (Join-Path $env:USERPROFILE '.gradle/wrapper/dists/gradle-7.3.3-all') -Recurse -Filter gradle.bat -ErrorAction SilentlyContinue)
		if ($cached.Count -eq 0) { throw 'Install Gradle 7.3.3 and pass -GradlePath pointing to bin/gradle.bat.' }
		$GradlePath = $cached[0].FullName
	}
}
if (!$JavaHome) { throw 'Set JAVA_HOME or pass -JavaHome pointing to a JDK 11 installation.' }
if (!(Test-Path "$JavaHome/bin/java.exe")) { throw "Java executable not found under $JavaHome" }

New-Item -ItemType Directory -Force $WorkDirectory | Out-Null
$WorkDirectory = (Resolve-Path $WorkDirectory).Path
$native_build = Join-Path $WorkDirectory "native-$NdkVersion-$Configuration"
$stage = Join-Path $WorkDirectory ("stage-" + [Guid]::NewGuid().ToString('N'))
$previous_java = $env:JAVA_HOME
$previous_sdk = $env:ANDROID_HOME
$previous_ndk = $env:ANDROID_NDK_HOME
try
{
	$env:JAVA_HOME = $JavaHome
	$env:ANDROID_HOME = $SdkRoot
	$env:ANDROID_NDK_HOME = $ndk_root
	Invoke-Checked $cmake @(
		'-S', $repo_root, '-B', $native_build, '-G', 'Ninja',
		"-DCMAKE_MAKE_PROGRAM=$ninja", "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
		"-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake",
		'-DVCPKG_MANIFEST_MODE=OFF', "-DVCPKG_INSTALLED_DIR=$installed", '-DVCPKG_TARGET_TRIPLET=arm64-android',
		'-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28', '-DANDROID_STL=c++_shared',
		'-DENABLE_TESTS=OFF', "-DCMAKE_BUILD_TYPE=$Configuration"
	)
	Invoke-Checked $cmake @('--build', $native_build, '--target', 'dlSandbox', '--parallel', '4')

	$libraries = Join-Path $stage 'jniLibs/arm64-v8a'
	$assets = Join-Path $stage 'assets'
	New-Item -ItemType Directory -Force $libraries, $assets | Out-Null
	foreach ($name in @('libdlEngine.so', 'libdlSandbox.so'))
	{
		Copy-Item (Join-Path $native_build "lib/arm64-v8a/$name") $libraries
	}
	Copy-Item $cxx_runtime $libraries
	Get-ChildItem (Join-Path $repo_root 'source/shader') -File |
		Where-Object { $_.Extension -in @('.glsl', '.hglsl') } |
		Copy-Item -Destination $assets

	Invoke-Checked $GradlePath @(
		'--no-daemon', '--console=plain', '-p', (Join-Path $repo_root 'Android'),
		'--project-cache-dir', (Join-Path $WorkDirectory 'project-cache'),
		"-PdaedalusOutput=$WorkDirectory/gradle", "-PdaedalusStage=$stage", ':app:assembleDebug'
	)
	Write-Output "APK: $WorkDirectory/gradle/app/outputs/apk/debug/app-debug.apk"
}
finally
{
	$env:JAVA_HOME = $previous_java
	$env:ANDROID_HOME = $previous_sdk
	$env:ANDROID_NDK_HOME = $previous_ndk
	if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
}

if ($Deploy)
{
	& (Join-Path $PSScriptRoot 'Deploy.ps1') -SdkRoot $SdkRoot -ApkPath "$WorkDirectory/gradle/app/outputs/apk/debug/app-debug.apk" -Serial $Serial
}
