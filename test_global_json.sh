#!/bin/bash
# test_global_json.sh - Test global JSON functionality

set -e  # Exit on first error

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== Testing Global JSON Functionality ===${NC}"

# Build if needed
if [ ! -f build/cpp_srv ] || [ ! -f build/cpp_cli ]; then
    echo "Building project..."
    ./build.sh
fi

# Clean up any existing GLOBAL_JSON.json
rm -f data/GLOBAL_JSON.json

# Test counter
TESTS_PASSED=0
TESTS_TOTAL=0

# Helper function to run test
run_test() {
    local name="$1"
    local command="$2"
    local expected_code="${3:-0}"
    
    TESTS_TOTAL=$((TESTS_TOTAL + 1))
    echo -n "Test $TESTS_TOTAL: $name... "
    
    if eval "$command"; then
        if [ $? -eq $expected_code ]; then
            echo -e "${GREEN}PASS${NC}"
            TESTS_PASSED=$((TESTS_PASSED + 1))
            return 0
        else
            echo -e "${RED}FAIL (exit code mismatch)${NC}"
            return 1
        fi
    else
        echo -e "${RED}FAIL${NC}"
        return 1
    fi
}

# Helper to check JSON output
check_json() {
    local result="$1"
    local path="$2"
    local expected="$3"
    
    local value=$(echo "$result" | jq -r "$path")
    if [ "$value" = "$expected" ]; then
        return 0
    else
        echo -e "${RED}Expected '$expected' but got '$value'${NC}"
        return 1
    fi
}

echo ""
echo -e "${BLUE}--- CLI Tests ---${NC}"

# Test 1: Get empty global JSON (should be {})
echo -n "Test 1: Get empty global JSON... "
RESULT=$(./build/cpp_cli --cmd get_global_json --args '{}')
if echo "$RESULT" | jq -e '.code == 0 and .output == {}' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 2: Set global JSON (with token)
echo -n "Test 2: Set global JSON with token... "
RESULT=$(./build/cpp_cli --cmd set_global_json --args '{"value":{"name":"Alice","age":30,"config":{"theme":"dark"}},"token":"jd"}')
if echo "$RESULT" | jq -e '.code == 0' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 3: Verify set worked
echo -n "Test 3: Verify global JSON was set... "
RESULT=$(./build/cpp_cli --cmd get_global_json --args '{}')
if echo "$RESULT" | jq -e '.code == 0 and .output.name == "Alice" and .output.age == 30' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 4: Get by path
echo -n "Test 4: Get value by path... "
RESULT=$(./build/cpp_cli --cmd get_global_json --args '{"path":"/name"}')
# Note: output is JSON-encoded string, so it will be "\"Alice\"" when dumped
if echo "$RESULT" | jq -e '.code == 0 and (.output == "Alice" or .output == "\"Alice\"")' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 5: Get nested path
echo -n "Test 5: Get nested path... "
RESULT=$(./build/cpp_cli --cmd get_global_json --args '{"path":"/config/theme"}')
# Note: output is JSON-encoded string
if echo "$RESULT" | jq -e '.code == 0 and (.output == "dark" or .output == "\"dark\"")' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 6: Apply merge patch (simplified API - no "patch" wrapper)
echo -n "Test 6: Apply merge patch... "
RESULT=$(./build/cpp_cli --cmd patch_global_json --args '{"age":31,"city":"NYC","config":{"theme":"light","lang":"en"}}')
if echo "$RESULT" | jq -e '.code == 0 and .output.after.age == 31 and .output.after.city == "NYC"' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 7: Verify patch was applied
echo -n "Test 7: Verify patch was applied... "
RESULT=$(./build/cpp_cli --cmd get_global_json --args '{}')
if echo "$RESULT" | jq -e '.code == 0 and .output.age == 31 and .output.city == "NYC" and .output.name == "Alice"' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 8: Verify persistence (file exists)
echo -n "Test 8: Verify GLOBAL_JSON.json file exists... "
if [ -f data/GLOBAL_JSON.json ]; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 9: Verify file content
echo -n "Test 9: Verify file content matches... "
FILE_CONTENT=$(cat data/GLOBAL_JSON.json)
if echo "$FILE_CONTENT" | jq -e '.name == "Alice" and .age == 31 and .city == "NYC"' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "File content: $FILE_CONTENT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 10: Delete key with null patch (simplified API)
echo -n "Test 10: Delete key with null patch... "
RESULT=$(./build/cpp_cli --cmd patch_global_json --args '{"city":null}')
if echo "$RESULT" | jq -e '.code == 0' > /dev/null; then
    RESULT2=$(./build/cpp_cli --cmd get_global_json --args '{}')
    if echo "$RESULT2" | jq -e '.output | has("city") | not' > /dev/null; then
        echo -e "${GREEN}PASS${NC}"
        TESTS_PASSED=$((TESTS_PASSED + 1))
    else
        echo -e "${RED}FAIL (city still exists)${NC}"
    fi
else
    echo -e "${RED}FAIL${NC}"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

echo ""
echo -e "${BLUE}--- HTTP Server Tests ---${NC}"

# Start server in background
echo "Starting server on port 9999..."
./build/cpp_srv --port 9999 --no-ipv6 > /dev/null 2>&1 &
SERVER_PID=$!

# Wait for server to start
sleep 2

# Cleanup function
cleanup() {
    echo ""
    echo "Stopping server (PID: $SERVER_PID)..."
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
}
trap cleanup EXIT

# Test 11: HTTP GET /get/global
echo -n "Test 11: HTTP GET /get/global... "
RESULT=$(curl -s http://localhost:9999/get/global)
if echo "$RESULT" | jq -e '.code == 0 and .output.name == "Alice"' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 12: HTTP GET /get/global/path
echo -n "Test 12: HTTP GET /get/global/path?path=/name... "
RESULT=$(curl -s "http://localhost:9999/get/global/path?path=/name")
if echo "$RESULT" | jq -e '.code == 0 and .output == "Alice"' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 13: HTTP POST /post/global (replace) - now requires token
echo -n "Test 13: HTTP POST /post/global with token... "
RESULT=$(curl -s -X POST http://localhost:9999/post/global \
    -H "Content-Type: application/json" \
    -d '{"value":{"product":"Widget","price":99.99,"stock":100},"token":"jd"}')
if echo "$RESULT" | jq -e '.code == 0' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 14: Verify HTTP set worked
echo -n "Test 14: Verify HTTP set worked... "
RESULT=$(curl -s http://localhost:9999/get/global)
if echo "$RESULT" | jq -e '.code == 0 and .output.product == "Widget" and .output.price == 99.99' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 15: HTTP POST /post/global/patch (simplified API)
echo -n "Test 15: HTTP POST /post/global/patch... "
RESULT=$(curl -s -X POST http://localhost:9999/post/global/patch \
    -H "Content-Type: application/json" \
    -d '{"stock":95,"on_sale":true}')
if echo "$RESULT" | jq -e '.code == 0 and .output.after.stock == 95 and .output.after.on_sale == true' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 16: HTTP POST /post/global/persist
echo -n "Test 16: HTTP POST /post/global/persist... "
RESULT=$(curl -s -X POST http://localhost:9999/post/global/persist)
if echo "$RESULT" | jq -e '.code == 0' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 17: Verify persistence after server restart
echo -n "Test 17: Verify persistence after restart... "
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null || true
sleep 1
./build/cpp_srv --port 9999 --no-ipv6 > /dev/null 2>&1 &
SERVER_PID=$!
sleep 2
RESULT=$(curl -s http://localhost:9999/get/global)
if echo "$RESULT" | jq -e '.code == 0 and .output.product == "Widget" and .output.stock == 95' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 18: Test error handling - missing path
echo -n "Test 18: Error handling - missing path... "
RESULT=$(curl -s "http://localhost:9999/get/global/path")
if echo "$RESULT" | jq -e '.code == 1' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 19: Test error handling - invalid path
echo -n "Test 19: Error handling - invalid path... "
RESULT=$(curl -s "http://localhost:9999/get/global/path?path=/nonexistent/path")
if echo "$RESULT" | jq -e '.code == 1' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Test 20: Security - set without token should fail
echo -n "Test 20: Security - set without token fails... "
RESULT=$(curl -s -X POST http://localhost:9999/post/global \
    -H "Content-Type: application/json" \
    -d '{"value":{"test":"no_token"}}')
if echo "$RESULT" | jq -e '.code == 6' > /dev/null; then
    echo -e "${GREEN}PASS${NC}"
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    echo -e "${RED}FAIL${NC}"
    echo "Got: $RESULT"
fi
TESTS_TOTAL=$((TESTS_TOTAL + 1))

# Summary
echo ""
echo -e "${BLUE}=== Test Summary ===${NC}"
echo -e "Tests passed: ${GREEN}$TESTS_PASSED${NC} / $TESTS_TOTAL"

if [ $TESTS_PASSED -eq $TESTS_TOTAL ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
