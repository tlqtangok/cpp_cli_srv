@echo off
set toolname_cli=cpp_cli
set toolname_srv=cpp_srv

call "D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
cd /d "c:\Users\tlqta\WorkBuddy\Claw\cpp_cli_srv"

echo [1/2] Building CLI...
cl /std:c++17 /O2 /EHsc /nologo /Icore /Ithird_party cli\main.cpp /Fe:build\%toolname_cli%.exe
if %errorlevel% neq 0 ( echo CLI FAILED & exit /b 1 )
echo CLI OK

echo [2/2] Building Server...
cl /std:c++17 /O2 /EHsc /nologo /Icore /Ithird_party server\main.cpp /Fe:build\%toolname_srv%.exe ws2_32.lib
if %errorlevel% neq 0 ( echo Server FAILED & exit /b 1 )
echo Server OK

if not exist build\web mkdir build\web
copy /Y web\index.html build\web\ > nul
echo.
echo Done! 
echo   build\%toolname_cli%.exe --schema
echo   build\%toolname_cli%.exe --cmd echo --args "{\"text\":\"hello\"}" --human
