# Test Suite

Comprehensive test scripts for cpp_cli_srv. Tests verify CLI, server, API, authentication, and edge cases.

---

## Quick Start

```bash
# Terminal 1: Build and start server
./build.sh
./build/cpp_srv

# Terminal 2: Run tests (auto-detects port)
cd test
./test_comprehensive.sh       # Recommended: runs most tests
```

---

## Test Categories

### Smoke Tests (No Server)

| Script | Platform | Purpose |
|--------|----------|---------|
| `test_cli.bat` | Windows | CLI smoke tests (no server needed) |
| `test_cli.sh` | Linux | CLI smoke tests (no server needed) |

**Example:**
```bash
# Linux
cd test && ./test_cli.sh

# Windows
cd test && test_cli.bat
```

---

### API Tests (Requires Server)

| Script | Platform | Purpose |
|--------|----------|---------|
| `test_server.bat` | Windows | HTTP GET/POST endpoints |
| `test_server.sh` | Linux | HTTP GET/POST endpoints, IPv4/IPv6 |

Start server first in another terminal, then:
```bash
cd test
./test_server.sh       # Auto-detects port
test_server.bat        # Windows (queries port manually)
```

---

### Functional Tests (Requires Server + Token)

| Script | Platform | Purpose | Token Required? |
|--------|----------|---------|-----------------|
| `test_shell.bat` | Windows | call_shell command | Yes |
| `test_shell.sh` | Linux | call_shell command | Yes |
| `test_token.bat` | Windows | Token auth validation | Yes |
| `test_token.sh` | Linux | Token auth validation | Yes |
| `test_json.bat` | Windows | write_json / read_json | No |
| `test_json.sh` | Linux | write_json / read_json | No |
| `test_global_json.bat` | Windows | Global JSON operations | Yes (token for write) |
| `test_global_json.sh` | Linux | Global JSON operations | Yes (token for write) |
| `test_https.sh` | Linux | HTTPS endpoints (if built with OpenSSL) | No |
| `test_logging.sh` | Linux | Log file output verification | No |
| `test_ipv6.bat` | Windows | Dual-stack IPv4/IPv6 | No |
| `test_ipv6v.bat` | Windows | IPv6 verbose testing | No |

**Run functional tests:**
```bash
# Start server WITH token
./build/cpp_srv --token mytoken123 --log server.log

# In another terminal:
cd test
./test_token.sh mytoken123        # Verify token auth works
./test_shell.sh                   # Test call_shell
./test_global_json.sh             # Test global JSON
./test_logging.sh                 # Verify logs
```

---

### Comprehensive Tests

| Script | Platform | Purpose |
|--------|----------|---------|
| `test_comprehensive.sh` | Linux | Full suite (CLI, server, API, token, concurrency) |
| `test_concurrent.bat` | Windows | Concurrent request stress test |

**Recommended flow:**
```bash
cd test
./test_comprehensive.sh    # Linux: all-in-one
test_concurrent.bat        # Windows: stress test
```

---

### Edge Case / Stress Tests

| Script | Platform | Purpose |
|--------|----------|---------|
| `test_echo.bat` | Windows | Simple echo command test |
| `test_rapid_edits.sh` | Linux | Rapid global JSON edits → disk batch write |
| `test_shutdown_save.sh` | Linux | Graceful SIGINT shutdown saves global JSON |
| `test_sigterm_save.sh` | Linux | SIGTERM signal handling + JSON persistence |

**Example:**
```bash
cd test
./test_rapid_edits.sh      # Verify batched writes work
./test_shutdown_save.sh    # Verify data isn't lost on shutdown
```

---

### Performance Tests

| Script | Platform | Purpose |
|--------|----------|---------|
| `test_req.js` | Both | Load test: 10,000 sequential requests to `get_global_json` |

**Requirements:** Node.js

**Run:**
```bash
# Terminal 1: Start server
./build/cpp_srv

# Terminal 2: Run load test
cd test
node test_req.js
```

**Output:** Throughput, latency, P95/P99, success rate.

---

### Source Code / Compilation Tests

| Script | Purpose |
|--------|---------|
| `test_json_parse.cpp` | Standalone: verify nlohmann JSON parsing |
| `test_token_read.cpp` | Standalone: verify token validation logic |

**Compile and run:**
```bash
cd test
g++ -std=c++17 test_json_parse.cpp -o test_json_parse && ./test_json_parse
g++ -std=c++17 test_token_read.cpp -o test_token_read && ./test_token_read
```

---

## Server Setup for Testing

### Without Token (API tests only)

```bash
./build/cpp_srv --port 8080
```

### With Token (Full functional tests)

```bash
./build/cpp_srv --port 8080 --token mytoken123
```

### With Logging

```bash
./build/cpp_srv --port 8080 --token mytoken123 --log server.log
```

### With HTTPS (Linux only)

```bash
# Generate self-signed cert first
mkdir -p ssl
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -subj "/C=US/ST=State/L=City/O=Org/CN=localhost"

# Start with HTTPS
./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl --token mytoken123
```

---

## Running All Tests (Recommended)

### Linux: One-Command Full Test

```bash
# Terminal 1: Build and start server
./build.sh
./build/cpp_srv --port 8080 --token mytoken123 --log server.log

# Terminal 2: Run comprehensive test suite
cd test
./test_comprehensive.sh
```

The comprehensive script auto-detects the server port and runs:
1. CLI smoke tests
2. HTTP GET/POST tests
3. Token auth validation
4. call_shell tests
5. Global JSON tests
6. JSON file I/O
7. Concurrency and timeout tests
8. IPv6 dual-stack verification

### Windows: Multi-Step Testing

```bash
# Terminal 1: Build and start server
build.bat
build\cpp_srv.exe --port 8080 --token mytoken123

# Terminal 2: Run tests sequentially
cd test
test_cli.bat
test_server.bat
test_token.bat mytoken123
test_shell.bat
test_json.bat
test_global_json.bat
test_concurrent.bat
test_ipv6.bat
```

---

## Test Output

### Success Output

```
✓ CLI schema test passed
✓ Echo command succeeded
✓ HTTP GET /get/status: 200
✓ HTTP POST /post/run: 200
✓ Token validation: PASSED
✓ call_shell with token: PASSED
✓ global JSON patch: 2 changes
✓ All tests passed!
```

### Failure Output

```
✗ FAILED: HTTP GET /get/status (expected 200, got 404)
  Error: Server not running?
  Port detected: 8080
```

Exit code is non-zero on failure — pipeline-safe for CI/CD.

---

## Port Auto-Detection

Linux test scripts auto-detect the server port:

```bash
PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")
```

Start server on any port:
```bash
./build/cpp_srv --port 7777
```

Tests will automatically use port `7777`.

---

## CI / CD Integration

### GitHub Actions Example

```yaml
name: Tests
on: [push]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build
        run: ./build.sh
      - name: Start server
        run: |
          ./build/cpp_srv --port 8080 --token testtoken123 &
          sleep 1
      - name: Run tests
        run: |
          cd test
          ./test_comprehensive.sh
```

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| "Server not running" | Start server: `./build/cpp_srv` |
| "Port already in use" | Use custom port: `./build/cpp_srv --port 9090` |
| "Permission denied" | Make scripts executable: `chmod +x test/*.sh` |
| "Token rejected (code 6)" | Start server WITH token: `--token mytoken` |
| "IPv6 warnings" | Ignore or use `--no-ipv6` flag |
| "HTTPS not working" | Requires OpenSSL: `sudo apt install libssl-dev` + rebuild |
| Node.js not found | Install Node.js: `sudo apt install nodejs` |
| Test hangs | Check server is responsive: `curl localhost:8080/get/status` |

---

## Performance Benchmarks (From test_req.js)

| Metric | Value |
|--------|-------|
| Total requests | 10,000 |
| Throughput | ~5,800 req/sec |
| Median latency | 0 ms |
| Average latency | 0.16 ms |
| P95 latency | 1 ms |
| P99 latency | 1 ms |
| Success rate | 100% |

---

## Adding New Tests

Create a new test file following the naming convention `test_<feature>.<ext>`:

```bash
#!/bin/bash
# test_myfeature.sh
set -e

# Detect server port
PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")

echo "Testing my feature on port $PORT..."

# Test logic
curl -X POST http://localhost:$PORT/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"echo","args":{"text":"hello"}}'

echo "✓ Test passed!"
```

Make executable: `chmod +x test/test_myfeature.sh`

---

## Test Coverage

| Area | Coverage |
|------|----------|
| CLI syntax | ✓ Multiple formats tested |
| HTTP API | ✓ GET, POST, OPTIONS, error handling |
| Authentication | ✓ Token validation, rejection, caching |
| Commands | ✓ echo, add, upper, call_shell, JSON I/O |
| Global JSON | ✓ Read, write, patch, merge, persistence |
| Concurrency | ✓ Parallel requests, async timeouts |
| Networking | ✓ IPv4, IPv6 dual-stack, HTTPS |
| Stress | ✓ Rapid edits, large payloads, shutdown |
| Logging | ✓ File output, format, rotation |

---

## Notes

- All scripts suppress verbose output. Use `set -x` for debugging.
- Tests assume `build/` directory exists with compiled binaries.
- Token tests require server started with `--token` parameter.
- HTTPS tests only work if built with OpenSSL (`./build.sh` auto-detects).
- Windows tests may require running from Administrator prompt.
- IPv6 tests skip gracefully if system doesn't support IPv6.
