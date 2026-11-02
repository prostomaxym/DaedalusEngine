@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Android\Dependencies.ps1" %*
exit /b %errorlevel%
