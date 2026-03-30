@echo off
echo === [1] health check ===
curl -s http://localhost:8080/get/status
echo.

echo === [2] schema (check slow_task is_async=true) ===
curl -s http://localhost:8080/get/schema
echo.

echo === [3] echo ===
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"hello concurrent world\"}}"
echo.

echo === [4] add ===
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"add\",\"args\":{\"a\":10,\"b\":32}}"
echo.

echo === [5] slow_task (ms=1500, timeout=5000 -> should succeed) ===
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"slow_task\",\"args\":{\"ms\":1500},\"timeout_ms\":5000}"
echo.

echo === [6] slow_task TIMEOUT (ms=3000, timeout=500 -> should fail) ===
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"slow_task\",\"args\":{\"ms\":3000},\"timeout_ms\":500}"
echo.

echo.
echo All tests done.
