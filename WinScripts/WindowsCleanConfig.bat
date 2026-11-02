@echo off

cd /d "%~dp0.."

for %%D in (build build-debug build-release-devtools) do (
  if exist "%%D\" rmdir "%%D" /s /q
  if exist "%%D\" (
	echo Cannot delete %%D. Close Visual Studio and other programs using this folder, then retry.
	pause
	exit /b 1
  )
)

call "%~dp0WindowsConfig.bat"
exit /b %errorlevel%
