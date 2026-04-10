@echo off
setlocal enabledelayedexpansion
call "D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
cd /d "c:\Users\tlqta\WorkBuddy\Claw\cpp_cli_srv"

REM Generate version info
for /f "tokens=2-4 delims=/ " %%a in ('date /t') do (set mydate=%%c%%a%%b)
for /f "tokens=1-2 delims=/:" %%a in ('time /t') do (set mytime=%%a%%b)
set BUILD_TIME=!mydate!!mytime!

for /f "delims=" %%a in ('git rev-parse --short HEAD 2^>nul') do (set GIT_COMMIT_ID=%%a)
if "!GIT_COMMIT_ID!"=="" (set GIT_COMMIT_ID=unknown)

set VERSION_FLAGS=/D "BUILD_TIME=\"!BUILD_TIME!\"" /D "GIT_COMMIT_ID=\"!GIT_COMMIT_ID!\""

echo === Step 1: CLI ===
cl /std:c++17 /O2 /EHsc /nologo /Icore /Ithird_party %VERSION_FLAGS% cli\main.cpp /Fe:build\cpp_cli.exe 2>&1
echo CLI_ERRORLEVEL=%errorlevel%

echo === Step 2: Server ===
cl /std:c++17 /O2 /EHsc /nologo /Icore /Ithird_party %VERSION_FLAGS% server\main.cpp /Fe:build\cpp_srv.exe /link ws2_32.lib 2>&1
echo SRV_ERRORLEVEL=%errorlevel%

if exist build\cpp_cli.exe echo CLI_EXE_EXISTS=1
if exist build\cpp_srv.exe echo SRV_EXE_EXISTS=1

