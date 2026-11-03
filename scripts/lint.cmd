@echo off
setlocal
cd /d "%~dp0.."
if not defined CXX set "CXX=g++.exe"
if not exist build mkdir build
if not exist build\public_key.h pwsh -NoProfile -NonInteractive -File scripts\public-key.ps1 -Output build\public_key.h
if errorlevel 1 exit /b 1
if not defined CLANG_FORMAT set "CLANG_FORMAT=clang-format.exe"
for %%F in (modules\root-helper\src\*.cpp modules\root-helper\src\*.h modules\root-helper\src\*.inc modules\root-helper\include\*.h) do (
    "%CLANG_FORMAT%" --dry-run --Werror --style=file "%%F"
    if errorlevel 1 exit /b 1
)
"%CXX%" -m32 -std=c++17 -Wall -Wextra -Werror -fanalyzer -fsyntax-only -Ibuild -Imodules/root-helper/include modules/root-helper/src/sdk_profile.cpp modules/root-helper/src/footer.cpp modules/root-helper/src/signature_win32.cpp modules/root-helper/src/launch_frame.cpp modules/root-helper/src/startup_parameters.cpp modules/root-helper/src/ini_win32.cpp modules/root-helper/src/config.cpp modules/root-helper/src/monitor.cpp modules/root-helper/src/sync_win32.cpp modules/root-helper/src/metadata_win32.cpp modules/root-helper/src/monitor_compat_win32.cpp modules/root-helper/src/parent_win32.cpp modules/root-helper/src/profile_process_win32.cpp modules/root-helper/src/files_win32.cpp modules/root-helper/src/main.cpp
exit /b %errorlevel%
