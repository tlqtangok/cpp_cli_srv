#!/bin/bash
# Comprehensive test demonstrating new binary names and port auto-detection

set -e

echo "==================================================================="
echo "    Testing Binary Name Changes and Port Auto-Detection"
echo "==================================================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}1. Checking Binary Names${NC}"
echo "-------------------------------------------------------------------"
if [ -f "build/cpp_srv" ] && [ -f "build/cpp_cli" ]; then
    echo -e "${GREEN}✓${NC} Binaries found:"
    ls -lh build/cpp_srv build/cpp_cli | awk '{print "  " $9 " (" $5 ")"}'
else
    echo "✗ Binaries not found. Run ./build.sh first."
    exit 1
fi

echo ""
echo -e "${BLUE}2. Testing CLI Binary${NC}"
echo "-------------------------------------------------------------------"
./build/cpp_cli --cmd echo --args '{"text":"CLI works!"}' > /tmp/cli_test.json
if grep -q "CLI works!" /tmp/cli_test.json; then
    echo -e "${GREEN}✓${NC} CLI binary (cpp_cli) works correctly"
else
    echo "✗ CLI test failed"
    exit 1
fi

echo ""
echo -e "${BLUE}3. Testing Port Auto-Detection${NC}"
echo "-------------------------------------------------------------------"

# Test 1: Default port (8080)
echo "Test 3.1: Server without --port argument (defaults to 8080)"
./build/cpp_srv &
SERVER_PID1=$!
sleep 2

DETECTED_PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")
echo "  Detected port: $DETECTED_PORT"
if [ "$DETECTED_PORT" = "8080" ]; then
    echo -e "${GREEN}✓${NC} Correctly detected default port 8080"
else
    echo "✗ Failed to detect default port"
    kill $SERVER_PID1 2>/dev/null || true
    exit 1
fi

# Test API
if curl -s http://localhost:$DETECTED_PORT/get/status | grep -q "running"; then
    echo -e "${GREEN}✓${NC} Server responding on port $DETECTED_PORT"
else
    echo "✗ Server not responding"
    kill $SERVER_PID1 2>/dev/null || true
    exit 1
fi

kill $SERVER_PID1
sleep 1

# Test 2: Custom port (9999)
echo ""
echo "Test 3.2: Server with --port 9999"
./build/cpp_srv --port 9999 &
SERVER_PID2=$!
sleep 2

DETECTED_PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")
echo "  Detected port: $DETECTED_PORT"
if [ "$DETECTED_PORT" = "9999" ]; then
    echo -e "${GREEN}✓${NC} Correctly detected custom port 9999"
else
    echo "✗ Failed to detect custom port"
    kill $SERVER_PID2 2>/dev/null || true
    exit 1
fi

# Test API
if curl -s http://localhost:$DETECTED_PORT/get/status | grep -q "running"; then
    echo -e "${GREEN}✓${NC} Server responding on port $DETECTED_PORT"
else
    echo "✗ Server not responding"
    kill $SERVER_PID2 2>/dev/null || true
    exit 1
fi

kill $SERVER_PID2
sleep 1

# Test 3: Another custom port (7777)
echo ""
echo "Test 3.3: Server with --port 7777"
./build/cpp_srv --port 7777 &
SERVER_PID3=$!
sleep 2

DETECTED_PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")
echo "  Detected port: $DETECTED_PORT"
if [ "$DETECTED_PORT" = "7777" ]; then
    echo -e "${GREEN}✓${NC} Correctly detected custom port 7777"
else
    echo "✗ Failed to detect custom port"
    kill $SERVER_PID3 2>/dev/null || true
    exit 1
fi

# Test API
if curl -s http://localhost:$DETECTED_PORT/get/status | grep -q "running"; then
    echo -e "${GREEN}✓${NC} Server responding on port $DETECTED_PORT"
else
    echo "✗ Server not responding"
    kill $SERVER_PID3 2>/dev/null || true
    exit 1
fi

kill $SERVER_PID3

echo ""
echo -e "${BLUE}4. Testing test_server.sh Auto-Detection${NC}"
echo "-------------------------------------------------------------------"

# Start server on custom port
./build/cpp_srv --port 8765 &
SERVER_PID4=$!
sleep 2

# Run test_server.sh which should auto-detect 8765
if ./test_server.sh 2>&1 | grep -q "Detected server port: 8765"; then
    echo -e "${GREEN}✓${NC} test_server.sh correctly auto-detected port 8765"
else
    echo "✗ test_server.sh failed to detect port"
    kill $SERVER_PID4 2>/dev/null || true
    exit 1
fi

kill $SERVER_PID4

echo ""
echo "==================================================================="
echo -e "${GREEN}    ALL TESTS PASSED!${NC}"
echo "==================================================================="
echo ""
echo "Summary:"
echo "  ✓ Binary names: cpp_srv (server) and cpp_cli (CLI)"
echo "  ✓ Port auto-detection works for default port (8080)"
echo "  ✓ Port auto-detection works for custom ports"
echo "  ✓ test_server.sh automatically detects server port"
echo ""
echo "You can now run the server on any port and test_server.sh will"
echo "automatically find it!"
echo ""
