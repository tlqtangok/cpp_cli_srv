@echo off
call "D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat" 2>nul
cd /d "c:\Users\tlqta\WorkBuddy\Claw\cpp_cli_srv"

echo === Step 1: CLI ===
cl /std:c++17 /O2 /EHsc /nologo /Icore /Ithird_party cli\main.cpp /Fe:build\cpp_cli.exe 2>&1
echo CLI_ERRORLEVEL=%errorlevel%

echo === Step 2: Server ===
cl /std:c++17 /O2 /EHsc /nologo /Icore /Ithird_party server\main.cpp /Fe:build\cpp_srv.exe /link ws2_32.lib 2>&1
echo SRV_ERRORLEVEL=%errorlevel%

if exist build\cpp_cli.exe echo CLI_EXE_EXISTS=1
if exist build\cpp_srv.exe echo SRV_EXE_EXISTS=1
