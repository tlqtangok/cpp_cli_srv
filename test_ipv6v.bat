@echo off
echo === verbose IPv6 test ===
curl -v -6 --connect-timeout 3 http://[::1]:8080/get/status
echo.
