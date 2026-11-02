@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Android\Deploy.ps1" %*
exit /b %errorlevel%
