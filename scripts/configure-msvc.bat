@echo off
setlocal
set "VSLANG=1033"

set "QT_ROOT=%~1"
if not defined QT_ROOT set "QT_ROOT=D:\Qt\6.10.0\msvc2022_64"

set "VS_ROOT=%~2"
if not defined VS_ROOT set "VS_ROOT=D:\Program Files\Microsoft Visual Studio\2022\Professional"

if not exist "%QT_ROOT%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo Qt 6 CMake package was not found under "%QT_ROOT%".
    exit /b 1
)

if not exist "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" (
    echo Visual Studio developer environment was not found under "%VS_ROOT%".
    exit /b 1
)

call "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" -arch=amd64
if errorlevel 1 exit /b %errorlevel%

cmake -S . -B build-msvc-ninja -G Ninja "-DCMAKE_PREFIX_PATH=%QT_ROOT%"
exit /b %errorlevel%
