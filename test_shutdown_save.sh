#!/bin/bash
# Test that CTRL+C (SIGINT) saves global JSON before exit

PORT=19877
TOKEN="shutdown_test_$$"

echo "=== Testing Shutdown Persistence (CTRL+C saves global JSON) ==="

# Clean up
rm -f data/GLOBAL_JSON.json data/srv_argc_argv.txt

# Start server in background
./build/cpp_srv --port $PORT --token "$TOKEN" > /dev/null 2>&1 &
SRV_PID=$!
sleep 2

echo "Server started (PID: $SRV_PID)"

# Make some edits
echo "Making edits to global JSON..."
curl -s -X POST http://localhost:$PORT/post/global \
  -H "Content-Type: application/json" \
  -d "{\"value\":{\"test\":\"shutdown_data\",\"timestamp\":\"$(date +%s)\"},\"token\":\"$TOKEN\"}" > /dev/null

curl -s -X POST http://localhost:$PORT/post/global/patch \
  -H "Content-Type: application/json" \
  -d "{\"extra_field\":\"should_be_saved\",\"token\":\"$TOKEN\"}" > /dev/null

echo "Waiting 2 seconds for edits to complete..."
sleep 2

# Verify in-memory state before shutdown
BEFORE=$(curl -s "http://localhost:$PORT/get/global?token=$TOKEN" | jq -r '.output.extra_field' 2>/dev/null)
echo "In-memory state before shutdown: extra_field = $BEFORE"

if [ "$BEFORE" != "should_be_saved" ]; then
    echo "✗ FAILED: In-memory state not correct"
    kill $SRV_PID 2>/dev/null
    exit 1
fi

# Send SIGINT (CTRL+C)
echo ""
echo "Sending SIGINT (CTRL+C) to server..."
kill -INT $SRV_PID

# Wait for server to exit
sleep 2

# Check if file was saved
if [ ! -f data/GLOBAL_JSON.json ]; then
    echo "✗ FAILED: GLOBAL_JSON.json was not saved on shutdown"
    exit 1
fi

# Verify file content
SAVED=$(jq -r '.extra_field' data/GLOBAL_JSON.json 2>/dev/null)
echo "Saved file content: extra_field = $SAVED"

if [ "$SAVED" == "should_be_saved" ]; then
    echo ""
    echo "=== TEST PASSED ==="
    echo "✓ Server saved global JSON on SIGINT (CTRL+C)"
    echo "✓ In-memory changes were persisted to disk"
    echo "✓ No data loss on graceful shutdown"
    exit 0
else
    echo ""
    echo "✗ FAILED: File content doesn't match (expected: should_be_saved, got: $SAVED)"
    exit 1
fi
