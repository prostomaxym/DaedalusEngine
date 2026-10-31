@echo off

cd /d "%~dp0.."

cpack --config build/CPackConfig.cmake -C ReleaseDevTools

PAUSE
