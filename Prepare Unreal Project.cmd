@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0unreal\Prepare-Unreal.ps1" %*
if errorlevel 1 pause
