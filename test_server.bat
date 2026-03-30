@echo off
echo === schema ===
curl -s http://localhost:8080/get/schema
echo.

echo === add ===
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"add\",\"args\":{\"a\":10,\"b\":32}}"
echo.

echo === echo ===
curl -s -X POST http://localhost:8080/post/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"hello\"}}"
echo.

echo === status ===
curl -s http://localhost:8080/get/status
echo.
