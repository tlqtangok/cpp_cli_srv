# Building on Ubuntu/Linux

This document describes how to build and run the project on Ubuntu/Linux.

## Prerequisites

- **C++ compiler**: g++ with C++17 support (g++ 7+)
- **CMake**: Version 3.14 or higher
- **Build tools**: make
- **System libraries**: pthread (usually included), OpenSSL (optional)

Install on Ubuntu:
```bash
sudo apt update
sudo apt install build-essential cmake
```

## Quick Build

```bash
./build.sh
```

This will:
1. Create the `build/` directory
2. Run CMake to generate Makefiles
3. Compile both binaries:
   - `build/cpp_srv` (HTTP server)
   - `build/cpp_cli` (CLI tool)
4. Copy `web/` assets to `build/web/`

## Manual Build

If you prefer to build manually:

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

## Running the CLI

```bash
# Show all available commands
./build/cpp_cli --schema

# Run a command
./build/cpp_cli --cmd echo --args '{"text":"hello"}'
./build/cpp_cli --cmd add --args '{"a":3,"b":4}'
./build/cpp_cli --cmd upper --args '{"text":"hello"}'

# Human-readable output
./build/cpp_cli --cmd add --args '{"a":3,"b":4}' --human
```

## Running the Server

```bash
# Start on default port 8080
./build/cpp_srv

# Custom port
./build/cpp_srv --port 9090

# Custom thread count
./build/cpp_srv --port 8080 --threads 8

# IPv4 only (skip IPv6)
./build/cpp_srv --no-ipv6

# Enable logging to file
./build/cpp_srv --log server.log

# Combine options
./build/cpp_srv --port 8080 --threads 8 --log server.log
```

The server will print:
```
=== mytool-server started ===
  IPv4 : http://0.0.0.0:8080
  IPv6 : http://[::]:8080
  Threads : 20 per server
  Schema  : GET  /get/schema
  Status  : GET  /get/status
  Run     : POST /post/run
  GUI     : GET  /
  Log     : server.log
  Press Ctrl+C to stop.
```

### Logging

When `--log <file>` is specified, all server activity is logged to both console and file with timestamp format `[YYYYMMDD_HHMMSS]`:

```
[20260330_112017] Server starting...
[20260330_112017] IPv4 server bound to 0.0.0.0:8080
[20260330_112017] Server ready - accepting connections
[20260330_112030] GET /get/status | IP=::1 | Status=200 | Out={"active":0,"message":"running","ok":true,"threads":20}
[20260330_112030] POST /post/run | IP=::1 | Status=200 | In={"cmd":"echo","args":{"text":"Test"}} | Out={"data":{"result":"Test"},"message":"ok","ok":true}
```

Each request logs: timestamp, HTTP method, path, client IP (IPv4/IPv6), HTTP status, request body, and response body.

Open your browser at `http://localhost:8080` to access the GUI.

## Testing

```bash
# CLI tests (no server needed)
./test_cli.sh

# Server API tests (start server first in another terminal)
# Terminal 1:
./build/cpp_srv [--port 8080]

# Terminal 2 (auto-detects port):
./test_server.sh
```

**Note**: `test_server.sh` automatically detects which port the server is running on by checking the process list.

## API Examples with curl

```bash
# Get command schema
curl http://localhost:8080/get/schema

# Get server status
curl http://localhost:8080/get/status

# Execute commands
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"hello"}}'

curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"add","args":{"a":10,"b":32}}'
```

## Differences from Windows Build

The Linux build uses:
- **Binary names**: `cpp_srv` (server) and `cpp_cli` (CLI) on Linux vs `cpp_srv.exe` and `cpp_cli.exe` on Windows
- **CMake** instead of direct `cl.exe` commands
- **pthread** library (automatically linked via `find_package(Threads)`)
- **g++** compiler with `-O2 -Wall -Wextra` flags
- Bash scripts (`.sh`) instead of batch files (`.bat`)

The core code is cross-platform and works identically on both Windows and Linux.

## Troubleshooting

### Build Errors

If you see missing header errors:
```bash
sudo apt install build-essential cmake
```

If you see pthread-related errors, ensure you have development libraries:
```bash
sudo apt install libpthread-stubs0-dev
```

### Server Port Already in Use

If port 8080 is already taken:
```bash
./build/cpp_srv --port 9090
```

The test script will auto-detect the port you're using.

### IPv6 Issues

If your system doesn't support IPv6 or you see IPv6 warnings:
```bash
./build/cpp_srv --no-ipv6
```

## Build Artifacts

After building, you'll have:
```
build/
├── cpp_srv              # Server binary
├── cpp_cli              # CLI binary
└── web/                 # GUI assets (copied from ../web/)
    └── index.html
```

## Clean Build

To rebuild from scratch:
```bash
rm -rf build/
./build.sh
```

Or:
```bash
cd build
make clean
make -j4
```
