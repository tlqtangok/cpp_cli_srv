# Binary Names

## Current Binary Names

| Platform | Server | CLI |
|----------|--------|-----|
| **Linux** | `cpp_srv` | `cpp_cli` |
| **Windows** | `cpp_srv.exe` | `cpp_cli.exe` |

## Quick Reference

### Build
```bash
./build.sh
```

Produces:
- `build/cpp_srv` - HTTP server
- `build/cpp_cli` - Command line interface

### Run Server
```bash
./build/cpp_srv                          # Default: port 8080
./build/cpp_srv --port 9090              # Custom port
./build/cpp_srv --log server.log         # With logging
```

### Run CLI
```bash
./build/cpp_cli --schema                 # List commands
./build/cpp_cli --cmd echo --args '{"text":"hello"}'
./build/cpp_cli --cmd add --args '{"a":10,"b":20}'
```

### Test
```bash
./test_cli.sh                            # CLI tests
./test_server.sh                         # Server tests (auto-detects port)
./test_comprehensive.sh                  # Full test suite
```

## Features

✅ **Consistent naming** across Linux and Windows  
✅ **Short, memorable names**: `cpp_srv` and `cpp_cli`  
✅ **Auto port detection** in test scripts  
✅ **Full logging support** with `[YYYYMMDD_HHMMSS]` timestamps  

## Process Detection

The test scripts automatically find the running server:

```bash
# Find server process and extract port
PORT=$(ps -ef | grep '[c]pp_srv' | grep -oP '\-\-port\s+\K\d+' || echo "8080")
```

This means you can start the server on **any port** and the tests will find it automatically!

## Examples

```bash
# Terminal 1: Start server on custom port
./build/cpp_srv --port 7777 --log mylog.log

# Terminal 2: Tests automatically detect port 7777
./test_server.sh
# Output: Detected server port: 7777
```
