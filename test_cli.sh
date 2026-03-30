#!/bin/bash
# CLI smoke tests for Linux

set -e

CLI="./build/cpp_cli"

echo "=== CLI Tests ==="
echo ""

echo "Test 1: schema"
$CLI --schema > /dev/null && echo "✓ --schema works"

echo ""
echo "Test 2: echo command"
result=$($CLI --cmd echo --args '{"text":"hello"}')
echo "$result" | grep -q '"code":0' && echo "$result" | grep -q '"output":"hello"' && echo "✓ echo command works"

echo ""
echo "Test 3: add command"
result=$($CLI --cmd add --args '{"a":3,"b":4}')
echo "$result" | grep -q '"code":0' && echo "$result" | grep -q '"output":"7' && echo "✓ add command works"

echo ""
echo "Test 4: upper command"
result=$($CLI --cmd upper --args '{"text":"hello"}')
echo "$result" | grep -q '"code":0' && echo "$result" | grep -q '"output":"HELLO"' && echo "✓ upper command works"

echo ""
echo "Test 5: human-readable output"
result=$($CLI --cmd add --args '{"a":10,"b":20}' --human)
echo "$result" | grep -q '\[OK\]' && echo "✓ --human flag works"

echo ""
echo "Test 6: error case (unknown command)"
if $CLI --cmd unknown --args '{}' 2>&1 | grep -q '"code":[1-9]'; then
    echo "✓ error handling works"
else
    echo "✗ error handling failed"
    exit 1
fi

echo ""
echo "=== All CLI tests passed! ==="
