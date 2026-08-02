@echo off
setlocal

set "VSLANG=1033"

set "QT_ROOT=%~1"
if not defined QT_ROOT set "QT_ROOT=D:\Qt\6.10.0\msvc2022_64"

set "VS_ROOT=%~2"
if not defined VS_ROOT set "VS_ROOT=D:\Program Files\Microsoft Visual Studio\2022\Professional"

if not exist "%QT_ROOT%\bin\Qt6Core.dll" (
    echo Qt 6 runtime was not found under "%QT_ROOT%".
    exit /b 1
)

if not exist "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" (
    echo Visual Studio developer environment was not found under "%VS_ROOT%".
    exit /b 1
)

call "%VS_ROOT%\Common7\Tools\VsDevCmd.bat" -arch=amd64
if errorlevel 1 exit /b %errorlevel%

set "PATH=%QT_ROOT%\bin;%PATH%"

cmake --build build-msvc-ninja --parallel
if errorlevel 1 exit /b %errorlevel%

ctest --test-dir build-msvc-ninja --output-on-failure
exit /b %errorlevel%
