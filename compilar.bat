@echo off
setlocal
cd /d "%~dp0"
if not "%~1"=="" goto arguments
echo 1. CSAuth1
echo 2. CSAuth2
echo 3. Hibrido
echo 4. Todas
choice /c 1234 /n /m "Escolha a compilacao: "
if errorlevel 4 set "mode=all"
if errorlevel 3 if not defined mode set "mode=hybrid"
if errorlevel 2 if not defined mode set "mode=csauth2"
if errorlevel 1 if not defined mode set "mode=csauth1"
call scripts\build.cmd %mode%
if errorlevel 1 (echo Falha na compilacao. & pause & exit /b 1)
echo Compilacao concluida. Os executaveis ainda sao prototipos.
pause

:arguments
call scripts\build.cmd %*
exit /b %errorlevel%
