@echo off
REM test_global_json.bat - Test global JSON functionality (Windows)

setlocal enabledelayedexpansion

echo === Testing Global JSON Functionality ===
echo.

REM Build if needed
if not exist build\cpp_srv.exe (
    echo Building project...
    call build.bat
)
if not exist build\cpp_cli.exe (
    echo Building project...
    call build.bat
)

REM Clean up any existing GLOBAL_JSON.json
if exist data\GLOBAL_JSON.json del /Q data\GLOBAL_JSON.json

set TESTS_PASSED=0
set TESTS_TOTAL=0

echo --- CLI Tests ---
echo.

REM Test 1: Get empty global JSON
set /a TESTS_TOTAL+=1
echo Test 1: Get empty global JSON...
build\cpp_cli.exe --cmd get_global_json --args "{}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 2: Set global JSON
set /a TESTS_TOTAL+=1
echo Test 2: Set global JSON...
build\cpp_cli.exe --cmd set_global_json --args "{\"value\":{\"name\":\"Alice\",\"age\":30}}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 3: Get global JSON
set /a TESTS_TOTAL+=1
echo Test 3: Get global JSON...
build\cpp_cli.exe --cmd get_global_json --args "{}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 4: Get by path
set /a TESTS_TOTAL+=1
echo Test 4: Get value by path...
build\cpp_cli.exe --cmd get_global_json --args "{\"path\":\"/name\"}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 5: Apply merge patch
set /a TESTS_TOTAL+=1
echo Test 5: Apply merge patch...
build\cpp_cli.exe --cmd patch_global_json --args "{\"patch\":{\"age\":31,\"city\":\"NYC\"}}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 6: Verify file exists
set /a TESTS_TOTAL+=1
echo Test 6: Verify GLOBAL_JSON.json file exists...
if exist data\GLOBAL_JSON.json (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 7: Persist command
set /a TESTS_TOTAL+=1
echo Test 7: Persist global JSON...
build\cpp_cli.exe --cmd persist_global_json --args "{}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

echo.
echo --- HTTP Server Tests ---
echo.
echo Starting server on port 9999...
start /B build\cpp_srv.exe --port 9999 --no-ipv6 > nul 2>&1

REM Wait for server to start
timeout /t 3 /nobreak > nul

REM Test 8: HTTP GET /get/global
set /a TESTS_TOTAL+=1
echo Test 8: HTTP GET /get/global...
curl -s http://localhost:9999/get/global > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 9: HTTP POST /post/global
set /a TESTS_TOTAL+=1
echo Test 9: HTTP POST /post/global...
curl -s -X POST http://localhost:9999/post/global -H "Content-Type: application/json" -d "{\"value\":{\"test\":\"data\"}}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Test 10: HTTP POST /post/global/patch
set /a TESTS_TOTAL+=1
echo Test 10: HTTP POST /post/global/patch...
curl -s -X POST http://localhost:9999/post/global/patch -H "Content-Type: application/json" -d "{\"patch\":{\"updated\":true}}" > nul 2>&1
if !errorlevel! equ 0 (
    echo [PASS]
    set /a TESTS_PASSED+=1
) else (
    echo [FAIL]
)

REM Stop server
taskkill /F /IM cpp_srv.exe > nul 2>&1

echo.
echo === Test Summary ===
echo Tests passed: %TESTS_PASSED% / %TESTS_TOTAL%

if %TESTS_PASSED% equ %TESTS_TOTAL% (
    echo All tests passed!
    exit /b 0
) else (
    echo Some tests failed!
    exit /b 1
)
