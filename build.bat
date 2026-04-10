@echo off
setlocal enabledelayedexpansion
set toolname_cli=cpp_cli
set toolname_srv=cpp_srv

cd /d "%~dp0"
set ROOT=%CD%

REM Generate version info (BUILD_TIME and GIT_COMMIT_ID)
REM BUILD_TIME format: YYYYMMDDHHMM
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%c%%a%%b)
for /f "tokens=1-2 delims=/:" %%a in ('time /t') do (set mytime=%%a%%b)
set BUILD_TIME=!mydate!!mytime!

REM Get git commit (short hash)
for /f "delims=" %%a in ('git rev-parse --short HEAD 2^>nul') do (set GIT_COMMIT_ID=%%a)
if "!GIT_COMMIT_ID!"=="" (set GIT_COMMIT_ID=unknown)

set VERSION=!BUILD_TIME!-!GIT_COMMIT_ID!
echo [Version] %VERSION%
echo.

REM Check for --static parameter
set STATIC_FLAG=
if /I "%1"=="--static" (
    set STATIC_FLAG=/MT
    echo Building with static runtime for portability...
    echo.
) else (
    set STATIC_FLAG=/MD
)

cd /d "%ROOT%"

REM Create build directory if it doesn't exist
if not exist build mkdir build

REM Define compile flags with version info
set VERSION_FLAGS=/D "BUILD_TIME=\"!BUILD_TIME!\"" /D "GIT_COMMIT_ID=\"!GIT_COMMIT_ID!\""

echo [1/2] Building CLI...
cl /std:c++17 /O2 /EHsc /nologo %STATIC_FLAG% /Icore /Ithird_party %VERSION_FLAGS% cli\main.cpp /Fe:build\%toolname_cli%.exe
if %errorlevel% neq 0 ( echo CLI FAILED & exit /b 1 )
echo CLI OK

echo [2/2] Building Server...
cl /std:c++17 /O2 /EHsc /nologo %STATIC_FLAG% /Icore /Ithird_party %VERSION_FLAGS% server\main.cpp /Fe:build\%toolname_srv%.exe ws2_32.lib
if %errorlevel% neq 0 ( echo Server FAILED & exit /b 1 )
echo Server OK

if not exist build\web mkdir build\web
copy /Y web\index.html build\web\ > nul
echo.
echo === Build complete ===
echo Version: %VERSION%
echo Binaries:
echo   - build\%toolname_srv%.exe (server)
echo   - build\%toolname_cli%.exe (CLI)
echo.
echo Full paths:
echo   Server: %cd%\build\%toolname_srv%.exe
echo   CLI:    %cd%\build\%toolname_cli%.exe
echo.
if /I "%1"=="--static" (
    echo Static runtime: Binaries include C++ runtime (/MT)
    echo.
)
echo Usage:
echo   build\%toolname_cli%.exe --help
echo   build\%toolname_cli%.exe --version
echo   build\%toolname_srv%.exe --help
echo   build\%toolname_srv%.exe --version
echo.
echo Tip: Use 'build.bat --static' for portable static builds

