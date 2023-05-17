@echo off

cd ../

if exist build\ (
  rmdir build /s /q
)

if exist build\ (
  echo Cant delete build folder. Close all projects and retry
  pause
) else (
 cd WinScripts/
  call CMakeConfigAndPackages.bat
)