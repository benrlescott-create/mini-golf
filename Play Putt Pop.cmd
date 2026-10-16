@echo off
set "GAME=%~dp0unity\PuttPopHighlands\Builds\Windows\PuttPopHighlands.exe"
if exist "%GAME%" (
  start "" "%GAME%" -force-d3d11
) else (
  echo The Windows player has not been built yet.
  echo Open Unity Game.cmd, then choose Putt Pop - Build Windows Player.
  pause
)
