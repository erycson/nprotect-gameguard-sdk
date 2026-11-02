@echo off
setlocal
cd /d "%~dp0.."
if not defined CLANG_FORMAT set "CLANG_FORMAT=clang-format.exe"
for %%F in (modules\root-helper\src\*.c modules\root-helper\include\*.h) do (
    "%CLANG_FORMAT%" -i --style=file "%%F"
    if errorlevel 1 exit /b 1
)
