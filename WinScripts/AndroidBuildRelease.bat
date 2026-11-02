@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Android\Build.ps1" -Configuration Release %*
exit /b %errorlevel%
