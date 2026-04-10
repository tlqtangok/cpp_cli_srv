#!/bin/bash
# Test that SIGTERM (kill) saves global JSON before exit

PORT=19878
TOKEN="sigterm_test_$$"

echo "=== Testing SIGTERM Persistence (kill saves global JSON) ==="

rm -f data/GLOBAL_JSON.json data/srv_argc_argv.txt

./build/cpp_srv --port $PORT --token "$TOKEN" > /dev/null 2>&1 &
SRV_PID=$!
sleep 2

echo "Server started (PID: $SRV_PID)"

curl -s -X POST http://localhost:$PORT/post/global \
  -H "Content-Type: application/json" \
  -d "{\"value\":{\"sigterm_test\":true},\"token\":\"$TOKEN\"}" > /dev/null

sleep 1

echo "Sending SIGTERM to server..."
kill -TERM $SRV_PID
sleep 2

if [ -f data/GLOBAL_JSON.json ]; then
    SAVED=$(jq -r '.sigterm_test' data/GLOBAL_JSON.json 2>/dev/null)
    if [ "$SAVED" == "true" ]; then
        echo "✓ TEST PASSED: Server saved global JSON on SIGTERM"
        exit 0
    fi
fi

echo "✗ FAILED: Data not saved on SIGTERM"
exit 1
