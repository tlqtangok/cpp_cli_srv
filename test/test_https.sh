#!/bin/bash

echo "========================================="
echo "Testing HTTPS Server"
echo "========================================="
echo ""

SSL_DIR="/root/jd/t/t0/cc1/create_httpd_img/ssl"

# Check if SSL files exist
if [ ! -f "$SSL_DIR/server.crt" ] || [ ! -f "$SSL_DIR/server.key" ]; then
    echo "❌ FAIL: SSL certificate files not found in $SSL_DIR"
    echo "  Expected: server.crt and server.key"
    exit 1
fi

echo "✓ SSL certificate files found"
echo ""

# Start server in background
echo "Starting server with HTTPS..."
./build/cpp_srv --port 8080 --port_https 8443 --ssl "$SSL_DIR" > /tmp/test_https_server.log 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 3

# Check if server is running
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "❌ FAIL: Server failed to start"
    cat /tmp/test_https_server.log
    exit 1
fi

echo "✓ Server started (PID: $SERVER_PID)"
echo ""

PASS=0
FAIL=0

# Test 1: HTTP endpoint
echo "[TEST 1] HTTP /get/status"
HTTP_RESULT=$(curl -s http://localhost:8080/get/status)
if echo "$HTTP_RESULT" | grep -q '"code":0'; then
    echo "  ✓ PASS: HTTP endpoint working"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: HTTP endpoint failed"
    echo "  Response: $HTTP_RESULT"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 2: HTTPS endpoint
echo "[TEST 2] HTTPS /get/status"
HTTPS_RESULT=$(curl -k -s https://localhost:8443/get/status)
if echo "$HTTPS_RESULT" | grep -q '"code":0'; then
    echo "  ✓ PASS: HTTPS endpoint working"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: HTTPS endpoint failed"
    echo "  Response: $HTTPS_RESULT"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 3: HTTPS schema endpoint
echo "[TEST 3] HTTPS /get/schema"
SCHEMA_RESULT=$(curl -k -s https://localhost:8443/get/schema)
if echo "$SCHEMA_RESULT" | python3 -c "import json, sys; data=json.load(sys.stdin); exit(0 if isinstance(data, list) and len(data) > 0 else 1)" 2>/dev/null; then
    echo "  ✓ PASS: HTTPS schema endpoint returns valid data"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: HTTPS schema endpoint failed"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 4: HTTPS POST /post/run
echo "[TEST 4] HTTPS POST /post/run (echo command)"
POST_RESULT=$(curl -k -s -X POST https://localhost:8443/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"HTTPS test"}}')
if echo "$POST_RESULT" | python3 -c "import json, sys; data=json.load(sys.stdin); exit(0 if data.get('code')==0 and data.get('output')=='HTTPS test' else 1)" 2>/dev/null; then
    echo "  ✓ PASS: HTTPS POST endpoint working"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: HTTPS POST endpoint failed"
    echo "  Response: $POST_RESULT"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 5: Both HTTP and HTTPS work simultaneously
echo "[TEST 5] Concurrent HTTP and HTTPS requests"
HTTP_RESULT=$(curl -s http://localhost:8080/get/status)
HTTPS_RESULT=$(curl -k -s https://localhost:8443/get/status)
if echo "$HTTP_RESULT" | grep -q '"code":0' && echo "$HTTPS_RESULT" | grep -q '"code":0'; then
    echo "  ✓ PASS: Both HTTP and HTTPS work concurrently"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Concurrent access failed"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 6: IPv6 HTTPS
echo "[TEST 6] HTTPS IPv6 endpoint"
IPV6_RESULT=$(curl -k -s -g https://[::1]:8443/get/status 2>/dev/null)
if echo "$IPV6_RESULT" | grep -q '"code":0'; then
    echo "  ✓ PASS: HTTPS IPv6 endpoint working"
    PASS=$((PASS + 1))
else
    echo "  ⚠ SKIP: HTTPS IPv6 not available (may be expected)"
fi
echo ""

# Stop server
echo "Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo "✓ Server stopped"
echo ""

# Summary
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "PASSED: $PASS"
echo "FAILED: $FAIL"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "✓ All HTTPS tests passed!"
    rm -f /tmp/test_https_server.log
    exit 0
else
    echo "✗ Some tests failed"
    echo ""
    echo "Server log:"
    cat /tmp/test_https_server.log
    exit 1
fi
