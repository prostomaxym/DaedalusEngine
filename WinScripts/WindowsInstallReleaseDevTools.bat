@echo off

cd /d "%~dp0.."

cmake --install build --prefix runtime/install --config ReleaseDevTools

PAUSE
