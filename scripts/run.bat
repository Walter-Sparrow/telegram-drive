@echo off
set BUILD_DIR=build

call %~dp0\build.bat

%BUILD_DIR%\Debug\TelegramDrive.exe
