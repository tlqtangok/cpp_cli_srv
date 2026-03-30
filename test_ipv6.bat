@echo off
echo === IPv4 status ===
curl -s --noproxy "*" http://127.0.0.1:8080/get/status
echo.

echo === IPv6 status ===
curl -s --noproxy "*" -6 http://[::1]:8080/get/status
echo.

echo === IPv4 echo ===
curl -s --noproxy "*" -X POST http://127.0.0.1:8080/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"from ipv4\"}}"
echo.

echo === IPv6 echo ===
curl -s --noproxy "*" -6 -X POST http://[::1]:8080/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"from ipv6\"}}"
echo.

echo === IPv4 add ===
curl -s --noproxy "*" -X POST http://127.0.0.1:8080/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"add\",\"args\":{\"a\":10,\"b\":32}}"
echo.

echo === IPv6 add ===
curl -s --noproxy "*" -6 -X POST http://[::1]:8080/post/run -H "Content-Type: application/json" -d "{\"cmd\":\"add\",\"args\":{\"a\":10,\"b\":32}}"
echo.
