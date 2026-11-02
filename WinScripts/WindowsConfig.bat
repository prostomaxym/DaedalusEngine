@echo off

cd /d "%~dp0.."

cmake --preset windows
set "result=%errorlevel%"

PAUSE
exit /b %result%
