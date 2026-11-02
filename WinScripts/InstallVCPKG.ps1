$ErrorActionPreference = 'Stop'

$bootstrap = Join-Path $PSScriptRoot '../ThirdParty/vcpkg/bootstrap-vcpkg.bat'
& $bootstrap
if ($LASTEXITCODE -ne 0) { throw "vcpkg bootstrap failed with exit code $LASTEXITCODE" }
