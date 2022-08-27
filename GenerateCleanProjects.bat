@echo off

if exist build\ (
  rmdir build /s /q
)

if exist build\ (
echo Cant delete build folder. Close all projects and retry
pause
) else (
call GenerateProjects.bat
)