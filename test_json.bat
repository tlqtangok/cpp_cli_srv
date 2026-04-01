@echo off
REM =========================================
REM Testing JSON File I/O Operations
REM =========================================

setlocal enabledelayedexpansion
set PASS=0
set FAIL=0

REM Create test directory
if not exist data mkdir data

echo.
echo [TEST 1] Write simple JSON object
build\cpp_cli.exe --cmd write_json --args "{\"path\":\"./data/test1.json\",\"json_content\":{\"k0\":\"v0\",\"k1\":\"v1\"}}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==0 else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: File written successfully
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=0
    set /a FAIL+=1
)

echo.
echo [TEST 2] Read back written JSON
build\cpp_cli.exe --cmd read_json --args "{\"path\":\"./data/test1.json\"}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==0 and 'k0' in r['output'] else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: File read successfully
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=0 and content
    set /a FAIL+=1
)

echo.
echo [TEST 3] Write nested JSON object
build\cpp_cli.exe --cmd write_json --args "{\"path\":\"./data/nested.json\",\"json_content\":{\"user\":\"alice\",\"profile\":{\"age\":30}}}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==0 else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: Nested JSON written
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=0
    set /a FAIL+=1
)

echo.
echo [TEST 4] Read nested JSON
build\cpp_cli.exe --cmd read_json --args "{\"path\":\"./data/nested.json\"}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==0 and 'alice' in r['output'] else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: Nested JSON read correctly
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=0 and content
    set /a FAIL+=1
)

echo.
echo [TEST 5] Read non-existent file (should fail)
build\cpp_cli.exe --cmd read_json --args "{\"path\":\"./data/notexist.json\"}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==1 and 'failed to open' in r['error'] else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: Correctly returned error
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=1
    set /a FAIL+=1
)

echo.
echo [TEST 6] Write to invalid path (should fail)
build\cpp_cli.exe --cmd write_json --args "{\"path\":\"C:/invalid/path/test.json\",\"json_content\":{\"x\":1}}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==1 and 'failed to open' in r['error'] else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: Correctly returned error
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=1
    set /a FAIL+=1
)

echo.
echo [TEST 7] Read file with invalid JSON (should fail)
echo this is not json > data\invalid.json
build\cpp_cli.exe --cmd read_json --args "{\"path\":\"./data/invalid.json\"}" > temp_result.json
python -c "import json; r=json.load(open('temp_result.json')); exit(0 if r['code']==4 and 'parse error' in r['error'] else 1)"
if %ERRORLEVEL%==0 (
    echo   PASS: Correctly returned JSON parse error
    set /a PASS+=1
) else (
    echo   FAIL: Expected code=4
    set /a FAIL+=1
)

REM Cleanup
del temp_result.json 2>nul

echo.
echo =========================================
echo Test Summary
echo =========================================
echo PASSED: %PASS%
echo FAILED: %FAIL%
echo.

if %FAIL%==0 (
    echo All tests passed!
    exit /b 0
) else (
    echo Some tests failed
    exit /b 1
)
