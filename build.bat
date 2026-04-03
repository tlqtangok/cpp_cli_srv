@echo off
set toolname_cli=cpp_cli
set toolname_srv=cpp_srv

pushd %0\..
set ROOT=%CD%
REM Check for --static parameter
set STATIC_FLAG=
if /I "%1"=="--static" (
    set STATIC_FLAG=/MT
    echo Building with static runtime for portability...
    echo.
) else (
    set STATIC_FLAG=/MD
)

REM Auto-detect Visual Studio 2022 vcvars64.bat
set VCVARS=
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
    set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat
)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
    set VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat
)
if exist "D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat" (
    set VCVARS=D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat
)

if "%VCVARS%"=="" (
    echo ERROR: Could not find Visual Studio 2022 vcvars64.bat
    echo Please install Visual Studio 2022 with C++ Desktop Development
    echo Or update the path in build.bat to match your installation
    exit /b 1
)

echo Using: %VCVARS%
call "%VCVARS%" 2>nul
cd /d "%ROOT%"

echo [1/2] Building CLI...
cl /std:c++17 /O2 /EHsc /nologo %STATIC_FLAG% /Icore /Ithird_party cli\main.cpp /Fe:build\%toolname_cli%.exe
if %errorlevel% neq 0 ( echo CLI FAILED & exit /b 1 )
echo CLI OK

echo [2/2] Building Server...
cl /std:c++17 /O2 /EHsc /nologo %STATIC_FLAG% /Icore /Ithird_party server\main.cpp /Fe:build\%toolname_srv%.exe ws2_32.lib
if %errorlevel% neq 0 ( echo Server FAILED & exit /b 1 )
echo Server OK

if not exist build\web mkdir build\web
copy /Y web\index.html build\web\ > nul
echo.
echo === Build complete ===
echo Binaries:
echo   - build\%toolname_srv%.exe (server)
echo   - build\%toolname_cli%.exe (CLI)
echo.
echo Full paths:
echo   Server: %cd%\build\%toolname_srv%.exe
echo   CLI:    %cd%\build\%toolname_cli%.exe
echo.
if /I "%1"=="--static" (
    echo Static runtime: Binaries include C++ runtime ^(/MT^)
    echo.
)
echo Usage:
echo   build\%toolname_cli%.exe --help
echo   build\%toolname_srv%.exe --help
echo.
echo Tip: Use 'build.bat --static' for portable static builds
