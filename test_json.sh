#!/bin/bash

echo "========================================="
echo "Testing JSON File I/O Operations"
echo "========================================="
echo ""

PASS=0
FAIL=0

# Create test directory
mkdir -p data

# Test 1: write_json with simple object
echo "[TEST 1] Write simple JSON object"
RESULT=$(./build/cpp_cli --cmd write_json --args '{"path":"./data/test1.json","json_content":{"k0":"v0","k1":"v1"}}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
if [ "$CODE" == "0" ] && [ -f "./data/test1.json" ]; then
    echo "  ✓ PASS: File written successfully"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=0 and file created"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 2: read_json from written file
echo "[TEST 2] Read back written JSON"
RESULT=$(./build/cpp_cli --cmd read_json --args '{"path":"./data/test1.json"}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
OUTPUT=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['output'])")
if [ "$CODE" == "0" ] && echo "$OUTPUT" | grep -q "k0"; then
    echo "  ✓ PASS: File read successfully with correct content"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=0 and content with 'k0'"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 3: write_json with nested object
echo "[TEST 3] Write nested JSON object"
RESULT=$(./build/cpp_cli --cmd write_json --args '{"path":"./data/nested.json","json_content":{"user":"alice","profile":{"age":30,"city":"NYC"},"scores":[95,87,92]}}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
if [ "$CODE" == "0" ]; then
    echo "  ✓ PASS: Nested JSON written"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=0"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 4: read_json with nested content
echo "[TEST 4] Read nested JSON"
RESULT=$(./build/cpp_cli --cmd read_json --args '{"path":"./data/nested.json"}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
OUTPUT=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['output'])")
if [ "$CODE" == "0" ] && echo "$OUTPUT" | grep -q "alice" && echo "$OUTPUT" | grep -q "NYC"; then
    echo "  ✓ PASS: Nested JSON read correctly"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=0 and nested content"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 5: Error - read non-existent file
echo "[TEST 5] Read non-existent file (should fail)"
RESULT=$(./build/cpp_cli --cmd read_json --args '{"path":"./data/notexist.json"}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
ERROR=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['error'])")
if [ "$CODE" == "1" ] && echo "$ERROR" | grep -q "failed to open"; then
    echo "  ✓ PASS: Correctly returned error for missing file"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=1 and error message"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 6: Error - write to invalid path
echo "[TEST 6] Write to invalid path (should fail)"
RESULT=$(./build/cpp_cli --cmd write_json --args '{"path":"/invalid/path/test.json","json_content":{"x":1}}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
ERROR=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['error'])")
if [ "$CODE" == "1" ] && echo "$ERROR" | grep -q "failed to open"; then
    echo "  ✓ PASS: Correctly returned error for invalid path"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=1 and error message"
    FAIL=$((FAIL + 1))
fi
echo ""

# Test 7: Error - read file with invalid JSON
echo "[TEST 7] Read file with invalid JSON (should fail)"
echo "this is not json" > data/invalid.json
RESULT=$(./build/cpp_cli --cmd read_json --args '{"path":"./data/invalid.json"}')
CODE=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['code'])")
ERROR=$(echo "$RESULT" | python3 -c "import json, sys; print(json.load(sys.stdin)['error'])")
if [ "$CODE" == "4" ] && echo "$ERROR" | grep -q "parse error"; then
    echo "  ✓ PASS: Correctly returned JSON parse error"
    PASS=$((PASS + 1))
else
    echo "  ✗ FAIL: Expected code=4 and parse error message"
    FAIL=$((FAIL + 1))
fi
echo ""

# Summary
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "PASSED: $PASS"
echo "FAILED: $FAIL"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "✓ All tests passed!"
    exit 0
else
    echo "✗ Some tests failed"
    exit 1
fi
