@echo off

cd /d "%~dp0.."

cmake --build --preset windows-release
set "result=%errorlevel%"

PAUSE
exit /b %result%
