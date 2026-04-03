#!/bin/bash
# Test rapid edits to global JSON (10 edits in 10 seconds)
# Should not hang the server and should only write to disk once

PORT=19876
TOKEN="test_token_$$"

echo "=== Testing Rapid Edits (10 edits in 10 seconds) ==="

# Clean up
rm -f data/GLOBAL_JSON.json data/srv_argc_argv.txt

# Start server in background
./build/cpp_srv --port $PORT --token "$TOKEN" > /dev/null 2>&1 &
SRV_PID=$!
sleep 2

echo "Server started (PID: $SRV_PID)"

# Initial set
echo "Initial set..."
curl -s -X POST http://localhost:$PORT/post/global \
  -H "Content-Type: application/json" \
  -d "{\"value\":{\"counter\":0},\"token\":\"$TOKEN\"}" | jq -r '.code'

# Perform 10 rapid edits (1 per second)
echo "Performing 10 rapid edits..."
for i in {1..10}; do
    echo -n "Edit $i... "
    RESPONSE=$(curl -s -X POST http://localhost:$PORT/post/global/patch \
      -H "Content-Type: application/json" \
      -d "{\"counter\":$i,\"timestamp_$i\":\"$(date +%s)\",\"token\":\"$TOKEN\"}" 2>&1)
    
    CODE=$(echo "$RESPONSE" | jq -r '.code' 2>/dev/null)
    
    if [ "$CODE" == "0" ]; then
        echo "OK (code=0)"
    else
        echo "FAILED (code=$CODE, response: $RESPONSE)"
        kill $SRV_PID 2>/dev/null
        exit 1
    fi
    
    sleep 1
done

# Verify final state
echo ""
echo "Verifying final state..."
FINAL=$(curl -s http://localhost:$PORT/get/global?token=$TOKEN)
FINAL_COUNTER=$(echo "$FINAL" | jq -r '.output.counter' 2>/dev/null)

if [ "$FINAL_COUNTER" == "10" ]; then
    echo "✓ Final counter = 10 (all edits applied in memory)"
else
    echo "✗ Final counter = $FINAL_COUNTER (expected 10)"
    kill $SRV_PID 2>/dev/null
    exit 1
fi

# Check that server is still responsive
echo ""
echo "Testing server responsiveness after rapid edits..."
HEALTH=$(curl -s http://localhost:$PORT/get/schema | jq -r 'length' 2>/dev/null)

if [ "$HEALTH" -gt "0" ]; then
    echo "✓ Server still responsive (schema has $HEALTH commands)"
else
    echo "✗ Server not responsive"
    kill $SRV_PID 2>/dev/null
    exit 1
fi

# Clean up
kill $SRV_PID 2>/dev/null
wait $SRV_PID 2>/dev/null

echo ""
echo "=== All Tests PASSED ==="
echo "✓ Server handled 10 rapid edits without hanging"
echo "✓ All edits applied to memory correctly"
echo "✓ Server remained responsive throughout"
