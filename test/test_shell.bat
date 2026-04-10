@echo off
echo === Test call_shell on Windows ===
echo.

echo Test 1: dir command
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"dir /B\"}}"
echo.
echo.

echo Test 2: echo command
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"echo Hello from Windows CMD\"}}"
echo.
echo.

echo Test 3: check current directory
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"cd\"}}"
echo.
echo.

echo Test 4: system info
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"call_shell\",\"args\":{\"command\":\"ver\"}}"
echo.
echo.

echo All tests done.
