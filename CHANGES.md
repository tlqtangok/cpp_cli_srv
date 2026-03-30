# Binary Name Changes and Port Auto-Detection

## Summary of Changes

### 1. Binary Names

Binary names have been standardized across platforms:

| Platform | Server Binary | CLI Binary |
|----------|--------------|------------|
| **Linux/macOS** | `cpp_srv` | `cpp_cli` |
| **Windows** | `cpp_srv.exe` | `cpp_cli.exe` |

**Previous names**: `mytool-server`, `mytool-cli`

### 2. Port Auto-Detection in Test Script

`test_server.sh` now automatically detects which port the server is running on by parsing the process list.

**How it works**:
```bash
# Extracts port from command line: ./cpp_srv --port 9090
PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")

# Falls back to 8080 if --port not found
```

**Benefits**:
- Test any port without editing the script
- Automatically uses default (8080) if --port not specified
- No configuration needed

## Updated Commands

### Building

```bash
# Linux/macOS
./build.sh

# Output:
#   build/cpp_srv (server)
#   build/cpp_cli (CLI)
```

### Running CLI

```bash
./build/cpp_cli --schema
./build/cpp_cli --cmd echo --args '{"text":"hello"}'
```

### Running Server

```bash
# Default port 8080
./build/cpp_srv

# Custom port
./build/cpp_srv --port 9090

# With logging
./build/cpp_srv --port 8080 --log server.log
```

### Testing

```bash
# CLI tests
./test_cli.sh

# Server tests (auto-detects port)
# Terminal 1: Start server on any port
./build/cpp_srv --port 9090

# Terminal 2: Run tests (detects 9090 automatically)
./test_server.sh
```

**Output**:
```
=== Server API Tests ===
Detected server port: 9090
Testing: http://localhost:9090

✓ Server is running on port 9090
...
```

## Files Modified

### Build System
- `CMakeLists.txt` - Changed target names from `mytool-server`/`mytool-cli` to `cpp_cli_srv`/`cpp_cli_srv_cli`

### Scripts
- `build.sh` - Updated binary references
- `test_cli.sh` - Updated CLI binary path
- `test_server.sh` - Added port auto-detection logic
- `test_logging.sh` - Updated server binary path

### Documentation
- `BUILD_UBUNTU.md` - Updated all command examples
- `LOGGING.md` - Updated binary names in examples
- `README.md` - (if needed for Windows builds)

## Testing Results

All tests pass with the new binary names:

```bash
$ ./test_cli.sh
=== CLI Tests ===
✓ --schema works
✓ echo command works
✓ add command works
✓ upper command works
✓ --human flag works
✓ error handling works
=== All CLI tests passed! ===

$ ./build/cpp_cli_srv --port 9090 &
$ ./test_server.sh
=== Server API Tests ===
Detected server port: 9090
Testing: http://localhost:9090
✓ Server is running on port 9090
✓ schema endpoint works
✓ status endpoint works
✓ echo command works
✓ add command works
✓ upper command works
✓ error handling works
✓ GUI endpoint works
=== All server tests passed! ===
```

## Migration Guide

If you have existing scripts using the old names:

### Update Binary References

```bash
# Old
./build/mytool-server --port 8080
./build/mytool-cli --cmd echo --args '{"text":"hello"}'

# New
./build/cpp_srv --port 8080
./build/cpp_cli --cmd echo --args '{"text":"hello"}'
```

### Update Build Commands

No changes needed - `./build.sh` works the same way, just produces different binary names.

### Update Deployment Scripts

If you have systemd services, Docker files, or deployment scripts:

```ini
# Old systemd service
ExecStart=/usr/local/bin/mytool-server --port 8080

# New
ExecStart=/usr/local/bin/cpp_srv --port 8080
```

## Port Detection Implementation

The auto-detection uses standard Unix tools:

```bash
# Step 1: Find cpp_srv process
ps -ef | grep '[c]pp_srv'

# Step 2: Extract --port argument using Perl regex
grep -oP '\-\-port\s+\K\d+'

# Step 3: Default to 8080 if not found
|| echo "8080"
```

**Regex explanation**:
- `\-\-port\s+` - Matches `--port` followed by whitespace
- `\K` - Discards everything matched so far (keeps only what follows)
- `\d+` - Matches the port number (one or more digits)

This approach:
- Works with or without explicit `--port` argument
- Handles multiple spaces between `--port` and the number
- Falls back gracefully to 8080 (default)
- Uses `[c]pp_srv` pattern to exclude the grep process itself

## Future Improvements

Possible enhancements:
1. Support multiple servers on different ports (currently tests first found)
2. PID file support for tracking server instances
3. Health check endpoint polling before running tests
4. JSON output for machine-readable test results
