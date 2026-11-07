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
	[string]$AssetDirectory,
	[string]$ConfigDirectory,
	[string]$PathConfigFile,
	[string]$ScriptDirectory,
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

function Copy-BundledRuntime([string]$destination, [string]$asset_directory, [string]$config_directory, [string]$path_config_file, [string]$script_directory)
{
	$entries = [System.Collections.Generic.List[string]]::new()
	foreach ($input_tree in @(@($asset_directory, 'asset'), @($config_directory, 'config'), @($script_directory, 'script')))
	{
		if (!$input_tree[0]) { continue }
		$source = Get-Item -LiteralPath $input_tree[0] -Force
		if (!$source.PSIsContainer -or ($source.Attributes -band [IO.FileAttributes]::ReparsePoint))
		{
			throw "Runtime input must be a directory, not a link: $($input_tree[0])"
		}
		$prefix = $source.FullName.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
		$staging_path = [IO.Path]::GetFullPath($destination)
		if ($staging_path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase) -or
			$staging_path.Equals($source.FullName, [StringComparison]::OrdinalIgnoreCase))
		{
			throw "Runtime input must not contain the APK staging directory: $($source.FullName)"
		}
		foreach ($item in Get-ChildItem -LiteralPath $source.FullName -Recurse -Force)
		{
			if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Runtime links are not supported: $($item.FullName)" }
			if ($item.PSIsContainer) { continue }
			$relative = $input_tree[1] + '/' + $item.FullName.Substring($prefix.Length).Replace('\', '/')
			if ($relative.Contains("`n") -or $relative.Contains("`r")) { throw "Runtime filenames cannot contain newlines: $relative" }
			$target = Join-Path $destination "runtime/$relative"
			New-Item -ItemType Directory -Force (Split-Path -Parent $target) | Out-Null
			Copy-Item -LiteralPath $item.FullName -Destination $target -Force
			$entries.Add($relative)
		}
	}
	if ($path_config_file)
	{
		$source = Get-Item -LiteralPath $path_config_file -Force
		if ($source.PSIsContainer -or ($source.Attributes -band [IO.FileAttributes]::ReparsePoint))
		{
			throw "Path configuration must be a regular file: $path_config_file"
		}
		New-Item -ItemType Directory -Force (Join-Path $destination 'runtime') | Out-Null
		Copy-Item -LiteralPath $source.FullName -Destination (Join-Path $destination 'runtime/path.ini') -Force
		$entries.Add('path.ini')
	}
	if ($asset_directory -or $config_directory -or $path_config_file -or $script_directory)
	{
		New-Item -ItemType Directory -Force $destination | Out-Null
		[IO.File]::WriteAllLines((Join-Path $destination 'runtime-files.txt'), $entries, [Text.UTF8Encoding]::new($false))
		if ((Get-Item -LiteralPath (Join-Path $destination 'runtime-files.txt')).Length -gt 16MB)
		{
			throw 'Bundled runtime manifest exceeds the 16 MiB startup limit.'
		}
		Write-Output "Bundled runtime files: $($entries.Count) (existing device files will be preserved)"
	}
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
	Copy-BundledRuntime (Join-Path $stage 'assets') $AssetDirectory $ConfigDirectory $PathConfigFile $ScriptDirectory
	Invoke-Checked $cmake @(
		'-S', $repo_root, '-B', $native_build, '-G', 'Ninja',
		"-DCMAKE_MAKE_PROGRAM=$ninja", "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
		"-DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=$ndk_root/build/cmake/android.toolchain.cmake",
		'-DVCPKG_MANIFEST_MODE=OFF', "-DVCPKG_INSTALLED_DIR=$installed", '-DVCPKG_TARGET_TRIPLET=arm64-android',
		'-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-28', '-DANDROID_STL=c++_shared',
		'-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON',
		'-DENABLE_TESTS=OFF', "-DCMAKE_BUILD_TYPE=$Configuration"
	)
	Invoke-Checked $cmake @('--build', $native_build, '--target', 'dlGame', '--parallel', '4')

	$libraries = Join-Path $stage 'jniLibs/arm64-v8a'
	$assets = Join-Path $stage 'assets'
	New-Item -ItemType Directory -Force $libraries, $assets | Out-Null
	foreach ($name in @('libdlEngine.so', 'libdlGame.so'))
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
