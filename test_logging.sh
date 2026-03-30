#!/bin/bash
# Test logging feature
# This script starts a server with logging, makes requests, and shows the log

set -e

echo "=== Testing Server Logging Feature ==="
echo ""

# Clean up old log
rm -f build/test_logging.log

# Start server with logging in background
echo "Starting server with logging enabled..."
cd build
./cpp_srv --port 8081 --log test_logging.log &
SERVER_PID=$!
cd ..

# Wait for server to start
sleep 2

echo "Making test requests..."
echo ""

# Test various endpoints
echo "1. GET /get/status"
curl -s http://localhost:8081/get/status > /dev/null

echo "2. POST /post/run (echo)"
curl -s -X POST http://localhost:8081/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"Hello Logging"}}' > /dev/null

echo "3. POST /post/run (add)"
curl -s -X POST http://localhost:8081/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"add","args":{"a":50,"b":75}}' > /dev/null

echo "4. POST /post/run (error case)"
curl -s -X POST http://localhost:8081/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"nonexistent","args":{}}' > /dev/null

echo "5. GET /get/schema"
curl -s http://localhost:8081/get/schema > /dev/null

echo "6. GET / (GUI)"
curl -s http://localhost:8081/ > /dev/null

# IPv4 test
echo "7. POST via IPv4"
curl -s -4 -X POST http://localhost:8081/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"upper","args":{"text":"ipv4 test"}}' > /dev/null

echo ""
echo "Stopping server..."
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true

echo ""
echo "=== Log Output (build/test_logging.log) ==="
echo ""
cat build/test_logging.log

echo ""
echo "=== Logging Test Complete ==="
echo ""
echo "Log format breakdown:"
echo "  [YYYYMMDD_HHMMSS] - Timestamp"
echo "  Method Path       - HTTP method and path"
echo "  IP=<address>      - Client IP (IPv4 or IPv6)"
echo "  Status=<code>     - HTTP status code"
echo "  In=<json>         - Request body (if present)"
echo "  Out=<json>        - Response body"
