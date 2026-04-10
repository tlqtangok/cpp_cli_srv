@echo off
REM test_token.bat
REM Test token authentication for call_shell command (Windows)

echo === Token Authentication Test ===
echo.

REM Start server with token
echo 1. Starting server with token 'test_token_123'...
start /B build\cpp_srv.exe --port 8090 --token test_token_123 > nul 2>&1
timeout /t 3 /nobreak > nul
echo    Server started on port 8090
echo.

REM Test 1: call_shell without token (should fail)
echo 2. Test: call_shell without token
curl -s -X POST http://localhost:8090/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"echo test\"}}" > test_response.txt
type test_response.txt
findstr /C:"\"code\":6" test_response.txt > nul
if %ERRORLEVEL% EQU 0 (
    echo    OK: Correctly rejected ^(code 6^)
) else (
    echo    FAIL: Expected code 6
)
echo.

REM Test 2: call_shell with wrong token (should fail)
echo 3. Test: call_shell with wrong token
curl -s -X POST http://localhost:8090/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"echo test\"},\"token\":\"wrong_token\"}" > test_response.txt
type test_response.txt
findstr /C:"\"code\":6" test_response.txt > nul
if %ERRORLEVEL% EQU 0 (
    echo    OK: Correctly rejected ^(code 6^)
) else (
    echo    FAIL: Expected code 6
)
echo.

REM Test 3: call_shell with correct token (should succeed)
echo 4. Test: call_shell with correct token
curl -s -X POST http://localhost:8090/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"echo hello\"},\"token\":\"test_token_123\"}" > test_response.txt
type test_response.txt
findstr /C:"\"code\":0" test_response.txt > nul
if %ERRORLEVEL% EQU 0 (
    echo    OK: Command executed successfully
) else (
    echo    FAIL: Expected successful execution
)
echo.

REM Test 4: Other commands work without token
echo 5. Test: Other commands ^(echo^) work without token
curl -s -X POST http://localhost:8090/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"no token needed\"}}" > test_response.txt
type test_response.txt
findstr /C:"\"code\":0" test_response.txt > nul
if %ERRORLEVEL% EQU 0 (
    echo    OK: Other commands work without token
) else (
    echo    FAIL: Expected successful execution
)
echo.

REM Test 5: call_shell via CLI (no token required)
echo 6. Test: call_shell via CLI ^(no token required^)
build\cpp_cli.exe --cmd call_shell --args "{\"command\":\"cd\"}" > test_response.txt
type test_response.txt
findstr /C:"\"code\":0" test_response.txt > nul
if %ERRORLEVEL% EQU 0 (
    echo    OK: CLI execution works without token
) else (
    echo    FAIL: Expected successful CLI execution
)
echo.

REM Cleanup
echo 7. Cleanup: Stopping server...
taskkill /F /IM cpp_srv.exe > nul 2>&1
del test_response.txt > nul 2>&1
echo    Server stopped
echo.

echo === All tests completed ===
