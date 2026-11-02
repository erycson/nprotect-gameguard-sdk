@echo off
setlocal
cd /d "%~dp0.."
if not defined CC set "CC=gcc.exe"
if not exist build mkdir build
"%CC%" -m32 -std=c11 -Wall -Wextra -Werror -Imodules/root-helper/include modules/root-helper/src/sdk_profile.c modules/root-helper/src/footer.c modules/root-helper/src/signature_win32.c modules/root-helper/src/launch_frame.c modules/root-helper/src/main.c -ladvapi32 -o build/root-helper-prototype.exe
if errorlevel 1 exit /b %errorlevel%
exit /b 0
