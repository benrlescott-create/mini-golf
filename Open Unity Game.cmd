@echo off
set "PROJECT=%~dp0unity\PuttPopHighlands"
if exist "%USERPROFILE%\Unity\6000.6.6f1\Editor\Unity.exe" (
  start "" "%USERPROFILE%\Unity\6000.6.6f1\Editor\Unity.exe" -force-d3d11 -projectPath "%PROJECT%"
) else if exist "%LOCALAPPDATA%\Unity\bin\unity.exe" (
  "%LOCALAPPDATA%\Unity\bin\unity.exe" open "%PROJECT%" --args "-force-d3d11"
  if errorlevel 1 pause
) else (
  if exist "%~dp0tools\UnityEditor\6000.6.6f1\Editor\Unity.exe" (
    start "" "%~dp0tools\UnityEditor\6000.6.6f1\Editor\Unity.exe" -projectPath "%PROJECT%"
  ) else (
    echo Open Unity Hub, choose Add project from disk, and select:
    echo %PROJECT%
    pause
  )
)
