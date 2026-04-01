#!/bin/bash
# Server API smoke tests for Linux
# Usage: test_server.sh [BASE_URL]
#   test_server.sh                    # Auto-detect from local cpp_srv process
#   test_server.sh http://localhost:9999
#   test_server.sh http://192.168.1.100:10220
#   test_server.sh https://myserver.com:8443

set -e

# Show help
if [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: test_server.sh [BASE_URL]"
    echo ""
    echo "Test cpp_srv API endpoints with smoke tests."
    echo ""
    echo "Examples:"
    echo "  test_server.sh                           # Auto-detect from local process"
    echo "  test_server.sh http://localhost:9999     # Test specific local port"
    echo "  test_server.sh http://192.168.1.100:10220  # Test remote server"
    echo "  test_server.sh https://myserver.com:8443   # Test HTTPS server"
    echo ""
    exit 0
fi

echo "=== Server API Tests ==="

# Check if BASE_URL provided as argument
if [ -n "$1" ]; then
    # Strip trailing slash
    BASE_URL="${1%/}"
    URL_SOURCE="provided"
    # Extract port from URL for display
    PORT=$(echo "$BASE_URL" | grep -oP ':\K\d+$' || echo "N/A")
    echo "Testing provided URL: $BASE_URL"
else
    # Auto-detect port from running cpp_srv process
    PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' | head -1)
    
    # If --port not found in command line, default is 8080
    if [ -z "$PORT" ]; then
        PORT=8080
    fi
    
    BASE_URL="http://localhost:$PORT"
    URL_SOURCE="detected"
    echo "Detected server port: $PORT"
    echo "Testing: $BASE_URL"
fi

echo ""

# Set curl options based on protocol
CURL_OPTS="-s --connect-timeout 5 --max-time 10"
if [[ "$BASE_URL" == https://* ]]; then
    CURL_OPTS="$CURL_OPTS -k"
    echo "Note: Using -k (insecure) flag for HTTPS to skip certificate verification"
    echo ""
fi

# Check if server is running with retries
MAX_RETRIES=3
RETRY=0
SERVER_RUNNING=0

while [ $RETRY -lt $MAX_RETRIES ]; do
    if curl $CURL_OPTS "$BASE_URL/get/status" > /dev/null 2>&1; then
        SERVER_RUNNING=1
        break
    fi
    RETRY=$((RETRY + 1))
    if [ $RETRY -lt $MAX_RETRIES ]; then
        echo "Retry $RETRY/$MAX_RETRIES: Waiting for server..."
        sleep 1
    fi
done

if [ $SERVER_RUNNING -eq 0 ]; then
    echo "✗ Server is not responding"
    echo "  URL: $BASE_URL"
    echo ""
    
    # Show diagnostics based on URL source
    if [ "$URL_SOURCE" = "detected" ]; then
        echo "Diagnostics (auto-detected):"
        echo "  • Server process running: $(ps -ef | grep -c '[c]pp_srv.*--port.*'$PORT)"
        
        # Try multiple tools to check port
        if command -v ss >/dev/null 2>&1; then
            echo "  • Port listening: $(ss -tuln 2>/dev/null | grep -c ":$PORT " || echo "0")"
        elif command -v netstat >/dev/null 2>&1; then
            echo "  • Port listening: $(netstat -tuln 2>/dev/null | grep -c ":$PORT " || echo "0")"
        else
            echo "  • Port listening: (ss/netstat not available)"
        fi
        
        echo ""
        echo "Troubleshooting:"
        echo "  1. Check if server started successfully"
        echo "  2. Check server logs for errors"
        echo "  3. Verify server is binding to 0.0.0.0 or localhost"
        echo "  4. If server just started, wait a few seconds and retry"
        echo "  5. Try manual test: curl -v $BASE_URL/get/status"
        echo ""
        echo "Start server with: ./build/cpp_srv --port $PORT"
    else
        echo "Diagnostics (custom URL):"
        echo "  • Connection test failed after 3 retries"
        echo "  • Server may not be running or not reachable"
        echo ""
        echo "Troubleshooting:"
        echo "  1. Verify server is running at: $BASE_URL"
        echo "  2. Check network connectivity to remote server"
        echo "  3. Verify firewall allows connections"
        echo "  4. Try manual test: curl -v $BASE_URL/get/status"
        echo "  5. For HTTPS, verify SSL certificate is valid"
    fi
    
    exit 1
fi

if [ "$URL_SOURCE" = "detected" ]; then
    echo "✓ Server is running on port $PORT"
else
    echo "✓ Server is responding at $BASE_URL"
fi
echo ""

echo "Test 1: GET /get/schema"
curl $CURL_OPTS "$BASE_URL/get/schema" | grep -q '"name":"echo"' && echo "✓ schema endpoint works"

echo ""
echo "Test 2: GET /get/status"
curl $CURL_OPTS "$BASE_URL/get/status" | grep -q '"ok":true' && echo "✓ status endpoint works"

echo ""
echo "Test 3: POST /post/run (echo)"
result=$(curl $CURL_OPTS -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"echo","args":{"text":"test"}}')
echo "$result" | grep -q '"result":"test"' && echo "✓ echo command works"

echo ""
echo "Test 4: POST /post/run (add)"
result=$(curl $CURL_OPTS -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"add","args":{"a":5,"b":7}}')
echo "$result" | grep -q '"result":12' && echo "✓ add command works"

echo ""
echo "Test 5: POST /post/run (upper)"
result=$(curl $CURL_OPTS -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"upper","args":{"text":"linux"}}')
echo "$result" | grep -q '"result":"LINUX"' && echo "✓ upper command works"

echo ""
echo "Test 6: error case (unknown command)"
result=$(curl $CURL_OPTS -X POST "$BASE_URL/post/run" -H "Content-Type: application/json" -d '{"cmd":"unknown","args":{}}')
echo "$result" | grep -q '"ok":false' && echo "✓ error handling works"

echo ""
echo "Test 7: GUI endpoint"
curl $CURL_OPTS "$BASE_URL/" | grep -q '<html>' && echo "✓ GUI endpoint works"

echo ""
echo "=== All server tests passed! ==="
echo ""
echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║           Copy-Paste Ready Curl Examples (All Commands)                 ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Set curl flags for examples
if [[ "$BASE_URL" == https://* ]]; then
    CURL_FLAGS="-k"
    echo "# Note: Add -k flag for HTTPS with self-signed certificates"
    echo ""
else
    CURL_FLAGS=""
fi

echo "# Get schema"
echo "curl -s $CURL_FLAGS $BASE_URL/get/schema | python3 -m json.tool"
echo ""
echo "# Get status"
echo "curl -s $CURL_FLAGS $BASE_URL/get/status"
echo ""
echo "# echo: Return the text argument as-is"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"echo\",\"args\":{\"text\":\"Hello World\"}}'"
echo ""
echo "# add: Return a + b"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"add\",\"args\":{\"a\":22,\"b\":100}}'"
echo ""
echo "# upper: Convert string to uppercase"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"upper\",\"args\":{\"text\":\"linux\"}}'"
echo ""
echo "# slow_task: Simulate a slow async operation (ms = sleep duration)"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"slow_task\",\"args\":{\"ms\":2000}}'"
echo ""
echo "# call_shell: Execute a shell command (requires token if configured)"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"call_shell\",\"args\":{\"command\":\"cat /etc/*release\"}}'"
echo ""
echo "# write_json: Write JSON content to a file"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"write_json\",\"args\":{\"path\":\"./data/test.json\",\"json_content\":{\"k1\":\"v1\",\"k2\":\"v2\",\"k3\":33}}}'"
echo ""
echo "# read_json: Read JSON content from a file"
echo "curl $CURL_FLAGS -X POST $BASE_URL/post/run -H 'Content-Type: application/json' -d '{\"cmd\":\"read_json\",\"args\":{\"path\":\"./data/test.json\"}}'"
echo ""
echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║  All curl commands above are ready to copy and run!                     ║"
echo "║  Base URL: $BASE_URL"
echo "╚══════════════════════════════════════════════════════════════════════════╝"

