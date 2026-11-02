@echo off
setlocal
cd /d "%~dp0.."
if not defined CC set "CC=gcc.exe"
if not defined CLANG_FORMAT set "CLANG_FORMAT=clang-format.exe"
for %%F in (modules\root-helper\src\*.c modules\root-helper\include\*.h) do (
    "%CLANG_FORMAT%" --dry-run --Werror --style=file "%%F"
    if errorlevel 1 exit /b 1
)
"%CC%" -m32 -std=c11 -Wall -Wextra -Werror -fanalyzer -fsyntax-only -Imodules/root-helper/include modules/root-helper/src/sdk_profile.c modules/root-helper/src/footer.c modules/root-helper/src/signature_win32.c modules/root-helper/src/launch_frame.c modules/root-helper/src/startup_parameters.c modules/root-helper/src/ini_win32.c modules/root-helper/src/config.c modules/root-helper/src/monitor.c modules/root-helper/src/main.c
exit /b %errorlevel%
