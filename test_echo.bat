@echo off
curl -s -X POST http://localhost:8080/api/run ^
  -H "Content-Type: application/json" ^
  -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"hello world\"}}"
echo.
