#!/bin/bash
#
# test_token.sh
# Test token authentication for call_shell command
#

echo "=== Token Authentication Test ==="
echo ""

# Start server with token
echo "1. Starting server with token 'test_token_123'..."
cd "$(dirname "$0")"
./build/cpp_srv --port 8090 --token test_token_123 > /tmp/token_test_server.log 2>&1 &
SERVER_PID=$!
echo "   Server PID: $SERVER_PID"
sleep 2

# Check if server is running
if ! ps -p $SERVER_PID > /dev/null 2>&1; then
    echo "   ❌ Server failed to start"
    cat /tmp/token_test_server.log
    exit 1
fi
echo "   ✅ Server started"
echo ""

# Test 1: call_shell without token (should fail)
echo "2. Test: call_shell without token"
RESPONSE=$(curl -s -X POST http://localhost:8090/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"echo test"}}')
echo "   Response: $RESPONSE"
if echo "$RESPONSE" | grep -q '"code":6'; then
    echo "   ✅ Correctly rejected (code 6)"
else
    echo "   ❌ Expected code 6"
fi
echo ""

# Test 2: call_shell with wrong token (should fail)
echo "3. Test: call_shell with wrong token"
RESPONSE=$(curl -s -X POST http://localhost:8090/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"echo test"},"token":"wrong_token"}')
echo "   Response: $RESPONSE"
if echo "$RESPONSE" | grep -q '"code":6'; then
    echo "   ✅ Correctly rejected (code 6)"
else
    echo "   ❌ Expected code 6"
fi
echo ""

# Test 3: call_shell with correct token (should succeed)
echo "4. Test: call_shell with correct token"
RESPONSE=$(curl -s -X POST http://localhost:8090/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"echo hello"},"token":"test_token_123"}')
echo "   Response: $RESPONSE"
if echo "$RESPONSE" | grep -q '"code":0'; then
    echo "   ✅ Command executed successfully"
else
    echo "   ❌ Expected successful execution"
fi
echo ""

# Test 4: Other commands work without token
echo "5. Test: Other commands (echo) work without token"
RESPONSE=$(curl -s -X POST http://localhost:8090/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"no token needed"}}')
echo "   Response: $RESPONSE"
if echo "$RESPONSE" | grep -q '"code":0'; then
    echo "   ✅ Other commands work without token"
else
    echo "   ❌ Expected successful execution"
fi
echo ""

# Test 5: call_shell via CLI (no token required)
echo "6. Test: call_shell via CLI (no token required)"
RESPONSE=$(./build/cpp_cli --cmd call_shell --args '{"command":"pwd"}')
echo "   Response: $RESPONSE"
if echo "$RESPONSE" | grep -q '"code":0'; then
    echo "   ✅ CLI execution works without token"
else
    echo "   ❌ Expected successful CLI execution"
fi
echo ""

# Cleanup
echo "7. Cleanup: Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null
echo "   ✅ Server stopped"
echo ""

echo "=== All tests completed ==="
