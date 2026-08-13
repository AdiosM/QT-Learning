@echo off
rem Build portable dist\ folder (run from anywhere)
setlocal
cd /d "%~dp0\.."

rem Toolchain PATH
set "PATH=D:\QT\Tools\CMake_64\bin;D:\QT\Tools\Ninja;D:\QT\Tools\mingw1310_64\bin;D:\QT\6.8.3\mingw_64\bin;%PATH%"

echo === 1/3 Release build ===
cmake -S . -B build-release -G Ninja -DCMAKE_PREFIX_PATH=D:/QT/6.8.3/mingw_64 -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1
cmake --build build-release
if errorlevel 1 exit /b 1

echo === 2/3 Collect files ===
if exist dist rmdir /s /q dist
mkdir dist
copy /y build-release\PdfTranslator.exe dist\ >nul
if errorlevel 1 exit /b 1

echo === 3/3 windeployqt ===
rem Must use the windeployqt from this kit; --compiler-runtime is required
rem for MinGW (libgcc/libstdc++/libwinpthread DLLs, else silent startup failure)
D:\QT\6.8.3\mingw_64\bin\windeployqt.exe --release --compiler-runtime --no-opengl-sw dist\PdfTranslator.exe
if errorlevel 1 exit /b 1

echo.
echo === dist folder ready ===
dir dist
exit /b 0
