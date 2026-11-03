@echo off
setlocal
cd /d "%~dp0.."
if not defined CXX set "CXX=g++.exe"
if not exist build mkdir build
pwsh -NoProfile -NonInteractive -File scripts\public-key.ps1 -InputKey "%GG_PUBLIC_KEY%" -Output build\public_key.h
if errorlevel 1 exit /b 1
set "mode=%~1"
if not defined mode set "mode=all"
if /i "%mode%"=="all" (
    call :build csauth1 1 || exit /b 1
    call :build csauth2 2 || exit /b 1
    call :build hybrid 0 || exit /b 1
    exit /b 0
)
if /i "%mode%"=="csauth1" goto csauth1
if /i "%mode%"=="csauth2" goto csauth2
if /i "%mode%"=="hybrid" goto hybrid
echo Uso: scripts\build.cmd [all^|csauth1^|csauth2^|hybrid]
exit /b 2

:csauth1
call :build csauth1 1
exit /b %errorlevel%
:csauth2
call :build csauth2 2
exit /b %errorlevel%
:hybrid
call :build hybrid 0
exit /b %errorlevel%

:build
if not exist "build\%~1" mkdir "build\%~1"
echo Compilando %~1...
"%CXX%" -m32 -static -mwindows -Wl,--disable-dynamicbase,--disable-reloc-section -std=c++17 -Wall -Wextra -Werror -DGG_AUTH_MODE=%~2 -Ibuild -Imodules/root-helper/include modules/root-helper/src/sdk_profile.cpp modules/root-helper/src/footer.cpp modules/root-helper/src/signature_win32.cpp modules/root-helper/src/launch_frame.cpp modules/root-helper/src/startup_parameters.cpp modules/root-helper/src/ini_win32.cpp modules/root-helper/src/config.cpp modules/root-helper/src/monitor.cpp modules/root-helper/src/sync_win32.cpp modules/root-helper/src/metadata_win32.cpp modules/root-helper/src/monitor_compat_win32.cpp modules/root-helper/src/parent_win32.cpp modules/root-helper/src/profile_process_win32.cpp modules/root-helper/src/files_win32.cpp modules/root-helper/src/main.cpp -ladvapi32 -o "build/%~1/GameGuard.des"
exit /b %errorlevel%
