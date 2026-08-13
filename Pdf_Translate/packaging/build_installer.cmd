@echo off
rem Build installer with Inno Setup (run build_dist.cmd first)
setlocal
cd /d "%~dp0"

set "ISCC="
if exist "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" (
    set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
) else if exist "C:\Program Files\Inno Setup 6\ISCC.exe" (
    set "ISCC=C:\Program Files\Inno Setup 6\ISCC.exe"
) else if exist "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" (
    set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
)

if not defined ISCC (
    echo Inno Setup 6 not found. Install it from: https://jrsoftware.org/isinfo.php
    exit /b 1
)

"%ISCC%" /Q PdfTranslator.iss
exit /b %errorlevel%
