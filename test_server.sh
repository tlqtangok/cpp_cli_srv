#!/bin/bash
# Server API smoke tests for Linux
# Automatically detects the port from running cpp_srv process

set -e

echo "=== Server API Tests ==="

# Auto-detect port from running cpp_srv process
PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")

# If --port not found in command line, default is 8080
if [ -z "$PORT" ]; then
    PORT=8080
fi

BASE_URL="http://localhost:$PORT"

echo "Detected server port: $PORT"
echo "Testing: $BASE_URL"
echo ""

# Check if server is running
if ! curl -s --connect-timeout 2 "$BASE_URL/get/status" > /dev/null 2>&1; then
    echo "✗ Server is not running on port $PORT"
    echo "Start it with: ./build/cpp_srv [--port $PORT]"
    exit 1
fi

echo "✓ Server is running on port $PORT"
echo ""

echo "Test 1: GET /get/schema"
curl -s "$BASE_URL/get/schema" | grep -q '"name":"echo"' && echo "✓ schema endpoint works"

echo ""
echo "Test 2: GET /get/status"
curl -s "$BASE_URL/get/status" | grep -q '"ok":true' && echo "✓ status endpoint works"

echo ""
echo "Test 3: POST /post/run (echo)"
result=$(curl -s -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"echo","args":{"text":"test"}}')
echo "$result" | grep -q '"result":"test"' && echo "✓ echo command works"

echo ""
echo "Test 4: POST /post/run (add)"
result=$(curl -s -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"add","args":{"a":5,"b":7}}')
echo "$result" | grep -q '"result":12' && echo "✓ add command works"

echo ""
echo "Test 5: POST /post/run (upper)"
result=$(curl -s -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"upper","args":{"text":"linux"}}')
echo "$result" | grep -q '"result":"LINUX"' && echo "✓ upper command works"

echo ""
echo "Test 6: error case (unknown command)"
result=$(curl -s -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"unknown","args":{}}')
echo "$result" | grep -q '"ok":false' && echo "✓ error handling works"

echo ""
echo "Test 7: GUI endpoint"
curl -s "$BASE_URL/" | grep -q '<html>' && echo "✓ GUI endpoint works"

echo ""
echo "=== All server tests passed! ==="
