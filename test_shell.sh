#!/bin/bash

echo "=== Test call_shell on Linux ==="
echo

echo "Test 1: ls command"
curl -s -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"ls -la"}}'
echo
echo

echo "Test 2: echo command"
curl -s -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"echo Hello from Linux bash"}}'
echo
echo

echo "Test 3: check current directory"
curl -s -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"pwd"}}'
echo
echo

echo "Test 4: system info"
curl -s -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"uname -a"}}'
echo
echo

echo "Test 5: list .sh files"
curl -s -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"ls *.sh"}}'
echo
echo

echo "All tests done."
