# cpp_cli_srv

A minimal C++ framework that exposes the **same business logic** through two surfaces simultaneously:

- **CLI** — for humans at a terminal, agents via pipe/subprocess, or shell scripts
- **HTTP Server** — for browser GUI or any REST client

The key idea: you write a command **once** in `core/commands.h`. Both the CLI binary and the web server pick it up automatically.

![Web UI](img/web_ui.png)

---

## Project Structure

```
cpp_cli_srv/
├── core/
│   ├── engine.h          ← Engine class: register/dispatch/schema (thread-safe)
│   └── commands.h        ← ALL commands live here. Edit this file only.
├── cli/
│   └── main.cpp          ← CLI shell (argument parsing, JSON I/O)
├── server/
│   └── main.cpp          ← HTTP server shell (REST API + static GUI)
├── web/
│   └── index.html        ← Browser GUI (auto-populated from /get/schema)
├── third_party/
│   ├── json.hpp          ← nlohmann/json (header-only)
│   └── httplib.h         ← cpp-httplib  (header-only)
├── build/                ← Output directory (binaries + web copy)
├── CMakeLists.txt        ← CMake configuration for cross-platform build
├── build.bat / build.sh  ← One-command build scripts
└── test_*.bat / test_*.sh ← Test scripts
```

---

## Requirements

| Item | Windows | Linux/Ubuntu |
|------|---------|--------------|
| **Compiler** | MSVC (cl.exe) via Visual Studio 2022, x64 | g++ 7+ with C++17 support |
| **Build System** | Direct cl.exe commands via batch | CMake 3.14+ and make |
| **C++ Standard** | C++17 | C++17 |
| **Dependencies** | None (header-only libs in `third_party/`) | pthread (auto-linked via CMake) |
| **Optional** | - | OpenSSL (for HTTPS support) |

**Installation:**

| Platform | Command | HTTPS Support |
|----------|---------|---------------|
| **Windows** | Install Visual Studio 2022 with C++ Desktop Development | ❌ Not yet supported |
| **Ubuntu/Debian** | `sudo apt update && sudo apt install build-essential cmake` | ❌ No |
| **Ubuntu/Debian (HTTPS)** | `sudo apt update && sudo apt install build-essential cmake libssl-dev` | ✅ Yes |
| **CentOS/RHEL (HTTPS)** | `sudo yum install gcc-c++ cmake openssl-devel` | ✅ Yes |
| **Fedora (HTTPS)** | `sudo dnf install gcc-c++ cmake openssl-devel` | ✅ Yes |
| **Arch Linux (HTTPS)** | `sudo pacman -S base-devel cmake openssl` | ✅ Yes |

**Notes**: 
- OpenSSL development libraries (`libssl-dev` on Ubuntu/Debian) are **required for HTTPS support**
- The build system automatically detects and enables HTTPS if OpenSSL is available
- If OpenSSL is not found during build, you'll see installation instructions in the CMake output
- HTTP-only mode works without OpenSSL (HTTPS features will be disabled)

---

## Build

| Platform | Command | Output Binaries |
|----------|---------|-----------------|
| **Windows** | `build.bat` | `build\cpp_cli.exe`<br>`build\cpp_srv.exe` |
| **Windows (Static)** | `build.bat --static` | Portable binaries with static C++ runtime |
| **Linux** | `./build.sh` | `build/cpp_cli`<br>`build/cpp_srv` |
| **Linux (Static)** | `./build.sh --static` | Portable binaries with static C++ stdlib |

### Static builds (Portable)

For cloud deployment or running on different machines without installing dependencies:

```bash
# Linux - creates portable binaries
./build.sh --static

# Windows - includes C++ runtime (/MT flag)
build.bat --static
```

**Benefits:**
- ✅ Copy to any machine without dependency installation
- ✅ No "library not found" errors
- ✅ Perfect for cloud servers (AWS, GCP, Azure)
- ✅ Works in minimal Docker containers

See [BUILD_STATIC.md](BUILD_STATIC.md) for detailed static build guide.

### What the build script does:

1. **Windows (`build.bat`):**
   - Initializes MSVC x64 environment (`vcvars64.bat`)
   - Compiles both binaries with `cl.exe`
   - Copies `web\index.html` → `build\web\index.html`

2. **Linux (`build.sh`):**
   - Creates `build/` directory
   - Runs CMake to generate Makefiles
   - Compiles with `make -j4`
   - Copies `web/` → `build/web/`

### Debug build:

| Platform | Command |
|----------|---------|
| **Windows** | `build_debug.bat` (prints full compiler output) |
| **Linux** | `mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Debug .. && make` |

### Clean rebuild:

| Platform | Command |
|----------|---------|
| **Windows** | `rmdir /s /q build && build.bat` |
| **Linux** | `rm -rf build && ./build.sh` |

> **Note for Windows users:** `build.bat` has the vcvars path hardcoded:
> `D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat`
> Update it if your VS is installed elsewhere.

---

## CLI Usage

### Command Format

The CLI supports two formats:

**New Format (curl-style, recommended)** — Same as HTTP POST requests:
```bash
./build/cpp_cli -d '{"cmd":"<command>","args":{...}}'
./build/cpp_cli --data '{"cmd":"<command>","args":{...}}'
```

**Legacy Format (backward compatible)**:
```bash
./build/cpp_cli --cmd <command> --args '{...}'
```

### Basic commands:

| Command | Description |
|---------|-------------|
| `./build/cpp_cli --help` | Show help message |
| `./build/cpp_cli --schema` | List all commands |
| `./build/cpp_cli -d '{"cmd":"echo","args":{"text":"hello"}}'` | Run command (new format) |
| `./build/cpp_cli --cmd echo --args '{"text":"hello"}'` | Run command (legacy format) |

### Complete usage examples (new curl-style format):

```bash
# Basic commands
./build/cpp_cli -d '{"cmd":"echo","args":{"text":"hello"}}'
./build/cpp_cli -d '{"cmd":"add","args":{"a":3,"b":4}}'
./build/cpp_cli -d '{"cmd":"upper","args":{"text":"hello"}}'

# With human-friendly output
./build/cpp_cli -d '{"cmd":"add","args":{"a":3,"b":4}}' --human

# Global JSON operations
./build/cpp_cli -d '{"cmd":"get_global_json","args":{"token":"jd"}}'
./build/cpp_cli -d '{"cmd":"set_global_json","args":{"value":{"name":"Alice"},"token":"jd"}}'
./build/cpp_cli -d '{"cmd":"patch_global_json","args":{"age":31,"city":"NYC"}}'

# Shell commands (requires token)
./build/cpp_cli -d '{"cmd":"call_shell","args":{"command":"ls -la","token":"jd"}}'

# File I/O
./build/cpp_cli -d '{"cmd":"read_json","args":{"path":"./data/config.json"}}'
./build/cpp_cli -d '{"cmd":"write_json","args":{"path":"./data/config.json","json_content":{"host":"localhost","port":8080}}}'
```

### Legacy format examples (still supported):

```bash
./build/cpp_cli --cmd echo --args '{"text":"hello"}'
./build/cpp_cli --cmd add --args '{"a":3,"b":4}'
./build/cpp_cli --cmd upper --args '{"text":"hello"}'
./build/cpp_cli --cmd add --args '{"a":3,"b":4}' --human
```

### Output format

Default (machine/agent mode):
```json
{"code":0,"output":"result","error":""}
```

With `--human`:
```
[OK] result
```

Exit code is `0` on success, `1` on any error — pipeline-safe.

---

## Server Usage

### Starting the server:

| Platform | Command | Description |
|----------|---------|-------------|
| **Windows** | `build\cpp_srv.exe --help` | Show help message |
| **Linux** | `./build/cpp_srv --help` | Show help message |
| **Windows** | `build\cpp_srv.exe` | Start on default port 8080 (HTTP only) |
| **Linux** | `./build/cpp_srv` | Start on default port 8080 (HTTP only) |
| **Windows** | `build\cpp_srv.exe --port 9090` | Custom HTTP port |
| **Linux** | `./build/cpp_srv --port 9090` | Custom HTTP port |
| **Windows** | `build\cpp_srv.exe --port_https 8443 --ssl C:\path\to\ssl` | Enable HTTPS on port 8443 |
| **Linux** | `./build/cpp_srv --port_https 8443 --ssl /path/to/ssl` | Enable HTTPS on port 8443 |
| **Windows** | `build\cpp_srv.exe --threads 8` | Custom thread count |
| **Linux** | `./build/cpp_srv --threads 8` | Custom thread count |
| **Windows** | `build\cpp_srv.exe --no-ipv6` | IPv4 only |
| **Linux** | `./build/cpp_srv --no-ipv6` | IPv4 only |
| **Windows** | `build\cpp_srv.exe --log server.log` | Enable file logging |
| **Linux** | `./build/cpp_srv --log server.log` | Enable file logging |
| **Windows** | `build\cpp_srv.exe --token mytoken123` | Set token for call_shell security |
| **Linux** | `./build/cpp_srv --token mytoken123` | Set token for call_shell security |

> **Security Note:** The `--token` parameter enables authentication for the `call_shell` command. Token must be at least 2 characters and contain only alphanumeric characters and underscores. If no token is provided, `call_shell` will be disabled via the web interface.

> **HTTPS Note:** To enable HTTPS, you need:
> - OpenSSL development libraries installed (`libssl-dev` on Ubuntu)
> - SSL certificate files: `server.crt` and `server.key` in the directory specified by `--ssl`
> - Both `--port_https` and `--ssl` parameters must be provided together

### Combined options example:

```bash
# Windows - HTTP only
build\cpp_srv.exe --port 8080 --threads 8 --log server.log --token secure_token_123

# Windows - HTTP + HTTPS
build\cpp_srv.exe --port 8080 --port_https 8443 --ssl C:\path\to\ssl --log server.log

# Linux - HTTP only
./build/cpp_srv --port 8080 --threads 8 --log server.log --token secure_token_123

# Linux - HTTP + HTTPS
./build/cpp_srv --port 8080 --port_https 8443 --ssl /root/jd/t/t0/cc1/create_httpd_img/ssl --log server.log
```

On startup the server prints:

**HTTP only:**
```
=== cpp_srv started ===
  IPv4 (HTTP)  : http://0.0.0.0:8080
  IPv6 (HTTP)  : http://[::]:8080
  Threads : 20 per server
  Schema  : GET  /get/schema
  Status  : GET  /get/status
  Run     : POST /post/run
  GUI     : GET  /
  Log     : server.log
  Press Ctrl+C to stop.
```

**HTTP + HTTPS:**
```
=== cpp_srv started ===
  IPv4 (HTTP)  : http://0.0.0.0:8080
  IPv6 (HTTP)  : http://[::]:8080
  IPv4 (HTTPS) : https://0.0.0.0:8443
  IPv6 (HTTPS) : https://[::]:8443
  Threads : 20 per server
  Schema  : GET  /get/schema
  Status  : GET  /get/status
  Run     : POST /post/run
  GUI     : GET  /
  Log     : server.log
  SSL     : /root/jd/t/t0/cc1/create_httpd_img/ssl
  Press Ctrl+C to stop.
```

### Logging

When `--log <file>` is specified, the server logs all activity to both console and file with timestamp format `[YYYYMMDD_HHMMSS]`:

```
[20260330_112017] Server starting...
[20260330_112017] IPv4 server bound to 0.0.0.0:8080
[20260330_112017] IPv6 server bound to [::]:8080
[20260330_112017] Server ready - accepting connections
[20260330_112030] GET /get/status | IP=::1 | Status=200 | Out={"active":0,"message":"running","ok":true,"threads":20}
[20260330_112030] POST /post/run | IP=::1 | Status=200 | In={"cmd":"echo","args":{"text":"Test"}} | Out={"data":{"result":"Test"},"message":"ok","ok":true}
[20260330_112051] POST /post/run | IP=::1 | Status=400 | In={"cmd":"unknown","args":{}} | Out={"data":null,"message":"unknown command: unknown","ok":false}
[20260330_112051] POST /post/run | IP=127.0.0.1 | Status=200 | In={"cmd":"upper","args":{"text":"test"}} | Out={"data":{"result":"TEST"},"message":"ok","ok":true}
```

Each log entry includes:
- **Timestamp**: `[YYYYMMDD_HHMMSS]` format
- **Method**: HTTP method (GET/POST/OPTIONS)
- **Path**: Request path
- **Client IP**: Remote address (IPv4/IPv6, respects X-Forwarded-For header)
- **Status**: HTTP status code (200, 400, 500, etc.)
- **In**: Request body (truncated if > 200 bytes)
- **Out**: Response body (truncated if > 200 bytes)
```

**Dual-stack model:**
- Two independent `httplib::Server` instances, each on its own thread
- Both share the same `Engine` (thread-safe via `shared_mutex`)
- IPv6 uses `IPV6_V6ONLY=true` so it does not overlap with IPv4
- If the OS has no IPv6 support, a `[WARN]` is printed and the server falls back to IPv4 only

The server must be run from the `build/` directory (it reads `web/index.html` relative to CWD).

#### GET /get/schema

Returns the full command list with argument definitions.

```json
[
  {
    "name": "add",
    "desc": "Return a + b",
    "is_async": false,
    "args": [
      {"name":"a","desc":"number a","required":true,"default":22},
      {"name":"b","desc":"number b","required":true,"default":100}
    ]
  }
]
```

#### GET /get/status

```json
{ "ok": true, "message": "running", "threads": 20, "active": 3 }
```

#### POST /post/run

```json
// Request body
{
  "cmd": "add",
  "args": { "a": 10, "b": 32 },
  "timeout_ms": 5000
}

// Success response (HTTP 200)
{ "ok": true, "message": "ok", "data": { "result": 42 } }

// Error response (HTTP 400)
{ "ok": false, "message": "unknown command: foo", "data": null }

// Timeout response (HTTP 400)
{ "ok": false, "message": "command timed out after 500ms", "data": null }

// Body too large (HTTP 413)
{ "ok": false, "message": "request body too large", "data": null }
```

`timeout_ms` is optional (default: 30 000 ms). Only meaningful for async commands.

### Browser GUI

Open `http://localhost:8080` — it reads `/get/schema` on load and auto-builds the command list. Click any command to pre-fill its argument template, then hit **Run**.

#### Command Configuration

The GUI supports configuration-based command enablement/disablement. On startup:

1. **Token Validation**: GUI prompts for token (cached in localStorage for convenience)
2. **Config Loading**: Fetches `cpp_cli_srv_config` from global JSON using token
3. **Default Creation**: If config doesn't exist, creates default (all enabled except `set_global_json`)
4. **Button Disabling**: Commands disabled in config show disabled Run button (grayed out, tooltip explains)

Config example (stored at `/cpp_cli_srv_config` in global JSON):
```json
{
  "echo": true,
  "call_shell": true,
  "set_global_json": false,
  "patch_global_json": true,
  "write_json": false
}
```

To create/modify config via curl:
```bash
# Create default config (all enabled except set_global_json)
curl -k -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"patch_global_json","args":{"cpp_cli_srv_config":{"echo":true,"set_global_json":false,...}}}'

# Fetch config (requires token)
curl -k -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"get_global_json","args":{"path":"/cpp_cli_srv_config","token":"YOUR_TOKEN"}}'
```

---

## Performance Benchmarks

Performance test using `node test_req.js` (10,000 sequential requests to `get_global_json`):

```
┌─────────────────────────────────────────┐
│          SUMMARY STATISTICS             │
├─────────────────────────┬───────────────┤
│ Total Requests          │         10000 │
│ Successful              │         10000 │
│ Failed                  │             0 │
│ Success Rate            │       100.00% │
│ Total Time              │    1719.00 ms │
│ Requests/sec            │       5817.34 │
└─────────────────────────┴───────────────┘

┌─────────────────────────────────────────┐
│       RESPONSE TIME (ms)                │
├─────────────────────────┬───────────────┤
│ Minimum                 │          0.00 │
│ Maximum                 │         26.00 │
│ Average                 │          0.16 │
│ Median                  │          0.00 │
│ 95th Percentile         │          1.00 │
│ 99th Percentile         │          1.00 │
└─────────────────────────┴───────────────┘

┌─────────────────────────────────────────┐
│     RESPONSE TIME DISTRIBUTION          │
├─────────────────────────┬───────────────┤
│ < 10ms                  │          9999 │
│ 10-19ms                 │             0 │
│ 20-49ms                 │             1 │
│ >= 50ms                 │             0 │
└─────────────────────────┴───────────────┘
```

**Key Metrics:**
- **Throughput**: ~5,800 requests/second (sequential)
- **Latency**: Sub-millisecond median response (0.16ms avg)
- **Reliability**: 100% success rate
- **P95/P99**: 1ms (consistent low-latency performance)

Tested on: Linux with 20 threads, HTTP server, in-memory global JSON access.

---

## Running Tests

| Test Type | Windows | Linux | Description |
|-----------|---------|-------|-------------|
| **CLI tests** | `test_cli.bat` | `./test_cli.sh` | CLI smoke tests (no server needed) |
| **API tests** | Start server, then `test_server.bat` | Start server, then `./test_server.sh` | REST API tests |
| **Shell tests** | `test_shell.bat` | `./test_shell.sh` | Test call_shell command |
| **Token tests** | `test_token.bat` | `./test_token.sh` | Token authentication tests |
| **JSON tests** | `test_json.bat` | `./test_json.sh` | JSON file I/O tests |
| **Global JSON** | `test_global_json.bat` | `./test_global_json.sh` | Global JSON variable tests |
| **Concurrency** | `test_concurrent.bat` | `./test_comprehensive.sh` | Concurrency & timeout tests |
| **IPv6 tests** | `test_ipv6.bat` | (included in test_server.sh) | Dual-stack IPv4/IPv6 tests |
| **Logging tests** | - | `./test_logging.sh` | Server logging verification |

### Running API tests (step by step):

```bash
# Windows - Terminal 1
build\cpp_srv.exe

# Windows - Terminal 2
test_server.bat

# Linux - Terminal 1
./build/cpp_srv

# Linux - Terminal 2
./test_server.sh
```

**Note:** 
- Linux `test_server.sh` auto-detects the server port from the running process
- Windows `test_ipv6.bat` uses `curl --noproxy "*"` to bypass system HTTP proxy for localhost IPv6 connections

---

## Global JSON Variable

The system provides a **global JSON variable** that persists in memory (server) or on disk (CLI):

### Features
- **In-memory storage** with automatic disk persistence
- **Thread-safe** concurrent access (server mode)
- **JSON Merge Patch** (RFC 7386) for updates
- **Diff tracking** on modifications
- **HTTP/HTTPS endpoints** and CLI commands

### Server Behavior
- Loads `data/GLOBAL_JSON.json` on startup
- Keeps JSON in memory for fast access
- **Deferred persistence**: Non-blocking, batches writes to disk with 15-minute delay + CPU awareness
  - Memory updates are **immediate** (in-process, sub-millisecond)
  - Disk writes delayed for 15 minutes to batch multiple edits into a single I/O operation
  - When disk write is ready, checks CPU usage: only writes if CPU < 50%
  - Max 20 minutes total wait before forced write (even if CPU high)
  - **Multiple rapid edits** (e.g., 10 edits in 10 seconds) trigger only **one** final disk write
  - **Non-blocking**: Rapid edits never hang the server (atomic flag-based synchronization)
  - Rationale: Reduces I/O load during intensive write periods, improves performance
- **Graceful shutdown**: SIGINT (Ctrl+C) and SIGTERM automatically save global JSON before exit
  - Prevents data loss on server shutdown
  - Pending in-memory changes are flushed to disk
  - Safe to stop server at any time without losing data
- Survives server restarts

### CLI Behavior
- **Immediate persistence**: Writes to disk immediately after each modification
- Loads from `data/GLOBAL_JSON.json` on each command invocation
- Ensures sequential CLI commands see consistent state

### HTTP Endpoints

```bash
# Get entire global JSON
curl http://localhost:8080/get/global

# Get specific path
curl "http://localhost:8080/get/global/path?path=/user/name"

# Replace entire JSON
curl -X POST http://localhost:8080/post/global \
  -H "Content-Type: application/json" \
  -d '{"value":{"name":"Alice","age":30}}'

# Apply merge patch (returns diff)
curl -X POST http://localhost:8080/post/global/patch \
  -H "Content-Type: application/json" \
  -d '{"patch":{"age":31,"city":"NYC"}}'

# Force save to disk
curl -X POST http://localhost:8080/post/global/persist
```

### CLI Commands

```bash
# Get entire JSON
./build/cpp_cli --cmd get_global_json --args '{}'

# Get specific path
./build/cpp_cli --cmd get_global_json --args '{"path":"/user/name"}'

# Set entire JSON
./build/cpp_cli --cmd set_global_json --args '{"value":{"name":"Bob"}}'

# Apply merge patch
./build/cpp_cli --cmd patch_global_json --args '{"patch":{"age":25}}'

# Force persist
./build/cpp_cli --cmd persist_global_json --args '{}'
```

### Merge Patch Example

```json
// Current state
{"name": "Alice", "age": 30, "city": "SF"}

// Apply patch
{"age": 31, "city": null, "country": "USA"}

// Result
{"name": "Alice", "age": 31, "country": "USA"}
// Note: "city" was deleted (null value)
```

### Use Cases
- **Configuration management**: Store app settings
- **Feature flags**: Dynamic toggles without restart
- **State storage**: Persistent application state
- **Counters/metrics**: Track values across requests
- **User preferences**: Per-user or global settings

See [API_FORMAT.md](API_FORMAT.md) for complete API documentation.

---

## REST API Examples

### Using curl:

| Operation | Command |
|-----------|---------|
| **Get schema** | `curl http://localhost:8080/get/schema` |
| **Get status** | `curl http://localhost:8080/get/status` |
| **Run command** | `curl -X POST http://localhost:8080/post/run -H "Content-Type: application/json" -d '{"cmd":"echo","args":{"text":"hello"}}'` |

### Complete examples:

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

curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"add","args":{"a":10,"b":32},"timeout_ms":5000}'
```

---

## Concurrency Model

```
HTTP Request
     │
     ▼
┌─────────────────────────────────┐
│  httplib::ThreadPool            │  N threads (default = hw_concurrency)
│  Each request runs on one thread│
└──────────────┬──────────────────┘
               │ Engine::run()
               │  shared_lock (read-only lookup, concurrent-safe)
               │  handler called OUTSIDE lock
               ▼
    ┌──────────────────────┐    ┌──────────────────────────┐
    │   SyncHandler        │    │   AsyncHandler            │
    │   (fast commands)    │    │   (slow / IO commands)    │
    │                      │    │                           │
    │  deferred future,    │    │  std::async(launch::async)│
    │  runs inline on      │    │  runs on SEPARATE thread, │
    │  pool thread,        │    │  pool thread waits with   │
    │  no overhead         │    │  timeout (default 30s)    │
    └──────────────────────┘    └──────────────────────────┘
```

**Key properties:**
- `Engine::run()` holds only a **shared (read) lock** during dispatch lookup — concurrent requests never block each other
- Sync handlers run inline (zero allocation overhead for fast commands)
- Async handlers run on a dedicated thread — a slow DB call never stalls the pool
- Per-request `timeout_ms` (pass in POST body, default 30 000 ms)
- Active-request counter exposed at `GET /get/status`

---

## Adding a New Command

**Only edit `core/commands.h`.** No other file needs to change.

### Sync command (fast, runs on HTTP pool thread)

```cpp
e.reg(
    { "greet",                          // command name (unique, no spaces)
      "Return a greeting message",      // description shown in schema + GUI
      {
        {"name", "person's name", true},       // {arg_name, desc, required}
        {"lang", "language code", false, "en"} // 4th param = GUI default value
      }
    },
    [](const json& args) -> Result {
        std::string name = args.value("name", "world");
        std::string lang = args.value("lang", "en");
        std::string msg  = (lang == "zh") ? ("hello " + name) : ("Hello, " + name);
        return { true, "ok", { {"result", msg} } };
    }
);
```

### Async command (slow / IO-bound, runs on its own thread)

```cpp
e.reg_async(
    { "fetch_data", "Fetch something slow", { {"id","record id",true} }, true },
    [](const json& args) -> std::future<Result> {
        int id = args.value("id", 0);
        return std::async(std::launch::async, [id]() -> Result {
            // e.g. blocking DB query or external HTTP call
            std::this_thread::sleep_for(std::chrono::seconds(2));
            return { true, "ok", { {"result", id * 2} } };
        });
    }
);
```

Caller can override timeout per request:
```json
POST /post/run
{ "cmd": "fetch_data", "args": {"id": 5}, "timeout_ms": 3000 }
```

Then rebuild:

| Platform | Command |
|----------|---------|
| **Windows** | `build.bat` |
| **Linux** | `./build.sh` |

The new command is immediately available in **both** CLI and Web GUI.

### ArgDef fields

```cpp
struct ArgDef {
    std::string name;         // key used in JSON args
    std::string desc;         // shown in schema / GUI tooltip
    bool        required;     // validated at call time (throws on missing)
    json        default_val;  // optional: pre-fills GUI input template
};
```

---

## Built-in Commands

The framework includes several built-in commands to demonstrate different patterns:

### Standard Commands

| Command | Type | Description | Example |
|---------|------|-------------|---------|
| `echo` | Sync | Return text as-is | `{"cmd":"echo","args":{"text":"hello"}}` |
| `add` | Sync | Add two numbers | `{"cmd":"add","args":{"a":3,"b":4}}` |
| `upper` | Sync | Convert to uppercase | `{"cmd":"upper","args":{"text":"hello"}}` |
| `slow_task` | Async | Simulate slow operation | `{"cmd":"slow_task","args":{"ms":2000}}` |
| `call_shell` | Async | Execute shell command | `{"cmd":"call_shell","args":{"command":"cat /etc/*release"}}` |
| `write_json` | Sync | Write JSON to file | `{"cmd":"write_json","args":{"path":"./data/test.json","json_content":{"k":"v"}}}` |
| `read_json` | Sync | Read JSON from file | `{"cmd":"read_json","args":{"path":"./data/test.json"}}` |

### call_shell Command

The `call_shell` command executes shell commands on the host system:

**Platform behavior:**
- **Windows**: Runs commands via `cmd.exe /c <command>`
- **Linux**: Runs commands directly in bash shell

**Arguments:**
- `command` (required): The shell command to execute

**Returns:**
- `output`: Command output (stdout + stderr combined)
- `exit_code`: Command exit code (0 = success)
- `command`: Echo of the executed command

**Examples:**

```bash
# Windows CLI
build\cpp_cli.exe --cmd call_shell --args "{\"command\":\"where cmd\"}" --human
build\cpp_cli.exe --cmd call_shell --args "{\"command\":\"echo Hello\"}" --human

# Linux CLI
./build/cpp_cli --cmd call_shell --args '{"command":"cat /etc/*release"}' --human
./build/cpp_cli --cmd call_shell --args '{"command":"pwd"}' --human

# Via HTTP API (requires token when accessed via web)
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"cat /etc/*release"},"token":"your_token_here"}'
```

**Security Features:** 🔒

When using `call_shell` via the web interface (HTTP API):
- **Token authentication required**: Server must be started with `--token <token>` parameter
- **Token format**: Minimum 2 characters, alphanumeric + underscore only
- **Token validation**: Every web request to `call_shell` must include the token
- **CLI bypass**: Token not required for CLI usage (trusted local execution)
- **Auto-prompt**: Web GUI automatically prompts for token on first use
- **Token caching**: Web GUI caches token in sessionStorage for the session

**Additional Security Warning:** ⚠️

The `call_shell` command executes arbitrary shell commands. In production environments:
- Always use a strong, random token (e.g., `openssl rand -hex 16`)
- Run server with restricted user permissions
- Consider network isolation or firewall rules
- Whitelist allowed commands if possible
- Sanitize user input
- Monitor and log all shell command executions
- Consider disabling this command entirely if not needed

**Testing:**

| Platform | Test Script |
|----------|-------------|
| **Windows** | `test_shell.bat` |
| **Linux** | `./test_shell.sh` |

### write_json and read_json Commands

The `write_json` and `read_json` commands provide JSON file I/O operations:

**write_json - Write JSON to file:**
- **Arguments:**
  - `path` (required): File path to write (e.g., `./data/config.json`)
  - `json_content` (required): JSON object to write to file
- **Returns:**
  - Success: `{"code":0,"output":"File written: <path>","error":""}`
  - Failure: `{"code":1,"error":"failed to open file for writing: <path>","output":""}`
- **Features:**
  - Pretty-printed JSON (2-space indentation)
  - Creates nested directories if parent path exists
  - Overwrites existing files

**read_json - Read JSON from file:**
- **Arguments:**
  - `path` (required): File path to read (e.g., `./data/config.json`)
- **Returns:**
  - Success: `{"code":0,"output":"<json content as string>","error":""}`
  - File error: `{"code":1,"error":"failed to open file for reading: <path>","output":""}`
  - Parse error: `{"code":4,"error":"JSON parse error: <details>","output":""}`

**Examples:**

```bash
# Windows CLI
build\cpp_cli.exe --cmd write_json --args "{\"path\":\"./data/config.json\",\"json_content\":{\"host\":\"localhost\",\"port\":8080}}" --human
build\cpp_cli.exe --cmd read_json --args "{\"path\":\"./data/config.json\"}" --human

# Linux CLI
./build/cpp_cli --cmd write_json --args '{"path":"./data/config.json","json_content":{"host":"localhost","port":8080}}' --human
./build/cpp_cli --cmd read_json --args '{"path":"./data/config.json"}' --human

# Via HTTP API
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"write_json","args":{"path":"./data/test.json","json_content":{"user":"alice","score":100}}}'

curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"read_json","args":{"path":"./data/test.json"}}'
```

**Testing:**

| Platform | Test Script |
|----------|-------------|
| **Windows** | `test_json.bat` |
| **Linux** | `./test_json.sh` |

---

## Platform Differences

| Aspect | Windows | Linux |
|--------|---------|-------|
| **Binary names** | `cpp_cli.exe`, `cpp_srv.exe` | `cpp_cli`, `cpp_srv` |
| **Compiler** | MSVC (cl.exe) | g++ |
| **Build system** | Direct cl.exe via batch | CMake + make |
| **Compiler flags** | `/std:c++17 /EHsc /O2` | `-std=c++17 -O2 -Wall -Wextra` |
| **System libraries** | ws2_32.lib (Winsock) | pthread (auto-linked) |
| **Scripts** | `.bat` files | `.sh` files |
| **Shell quoting** | JSON args need escaped quotes `\"` | Use single quotes for JSON |
| **Path separator** | Backslash `\` | Forward slash `/` |

---

## Known Holes / Limitations

### Security
- **No authentication.** The server binds to `0.0.0.0` — anyone on the network can call it. Add an API key header check if you expose it beyond localhost.
- **CORS is wide open** (`Access-Control-Allow-Origin: *`). Fine for local dev; restrict in production.

### Robustness
- **No schema-level arg validation.** Commands receive whatever JSON arrives; missing required args cause a `std::exception` that is caught and returned as `{"ok":false}`, but there is no pre-handler enforcement.
- **Async timeout does not kill the background thread.** After timeout, the HTTP response returns immediately, but the `std::async` thread continues running until it finishes naturally. For truly cancellable tasks, use `std::stop_token` (C++20).
- **`--args` parsing on Windows shell is painful.** Nested quotes in PowerShell are a mess. Use `.bat` test files to work around it.

### Features not yet implemented
- **`--args @file.json`** — read args from a file instead of inline string. Easy to add in `cli/main.cpp`.
- **Streaming / SSE responses.** All responses are synchronous. For long-running commands, add a job-queue + poll endpoint.
- **Persistent state.** The engine is stateless per-call. Add a shared state object if commands need to share data.
- **Command namespacing.** All commands live in a flat map. For large projects, add a `group` field to `CmdDef`.
- **OpenSSL / HTTPS.** httplib supports it via `#define CPPHTTPLIB_OPENSSL_SUPPORT` + linking OpenSSL. Not enabled here to keep the build simple.

### Known Build Quirks
- **No Chinese / non-ASCII in source files** (except `web/index.html`). MSVC with codepage 936 will error on multibyte characters in `.cpp`/`.h` files. Keep all C++ comments in ASCII.
- **Do NOT `#define CPPHTTPLIB_OPENSSL_SUPPORT 0`** — httplib uses `#ifdef`, so defining it to `0` still enables OpenSSL. Simply don't define it.
- **Server must run from `build/`** — it reads `web/index.html` with a relative path. If you move the exe, fix the path in `server/main.cpp`.
- **Windows vcvars path** — `build.bat` has the vcvars64.bat path hardcoded. Update if your Visual Studio is installed elsewhere.

---

## Troubleshooting

| Issue | Platform | Solution |
|-------|----------|----------|
| **Missing compiler** | Windows | Install Visual Studio 2022 with C++ Desktop Development |
| **Missing compiler** | Linux | `sudo apt install build-essential cmake` |
| **pthread errors** | Linux | `sudo apt install libpthread-stubs0-dev` |
| **Port already in use** | Both | Use `--port <other_port>` when starting server |
| **IPv6 warnings** | Both | Use `--no-ipv6` flag or ignore (falls back to IPv4) |
| **Build errors** | Windows | Try `build_debug.bat` for verbose output |
| **Build errors** | Linux | Check CMake version: `cmake --version` (need 3.14+) |
| **Permission denied errors** | Linux | Run `./fix_permissions.sh` then rebuild |
| **Server can't find web/index.html** | Both | Run server from `build/` directory |

### Permission Denied Errors

If you get `make[2]: stat: Permission denied` errors after copying the project to another machine:

**Problem**: Files were copied without proper read permissions.

**Solution**:
```bash
# Quick fix - run the provided script
./fix_permissions.sh

# Or manually:
find . -type d -exec chmod 755 {} \;  # Directories
find . -type f -exec chmod 644 {} \;  # Files
chmod +x *.sh                          # Scripts

# Then rebuild
./build.sh
```

**Prevention**: When copying the project, use:
```bash
# Preserve permissions
cp -rp source/ destination/

# Or use tar
tar czf project.tar.gz cpp_cli_srv/
# On target machine:
tar xzf project.tar.gz
```

---

## Architecture Summary

```
                    ┌──────────────────────────────┐
                    │  core/commands.h              │
                    │  register_all(Engine& e)      │
                    │  ← ONLY file you edit ←       │
                    └────────────┬─────────────────┘
                                 │  shared Engine (thread-safe)
              ┌──────────────────┴──────────────────┐
              │                                      │
     ┌────────▼────────┐                   ┌────────▼──────────┐
     │  cli/main.cpp   │                   │  server/main.cpp   │
     │  --cmd  --args  │                   │  POST /post/run    │
     │  --schema       │                   │  GET  /get/schema  │
     │  --human        │                   │  GET  /get/status  │
     └─────────────────┘                   │  GET  /            │
          stdout JSON                      └────────┬──────────┘
          exit 0/1                                  │ HTTP
                                           ┌────────▼────────┐
                                           │  web/index.html  │
                                           │  Browser GUI     │
                                           └─────────────────┘
```

---

## Author & License

**Author**: linqi  
**Email**: tlqtangok@126.com  
**Copyright**: 2026

This project is released as open source software. All rights reserved.


---

# Additional Documentation

## API Reference

# Unified JSON Response Format

## New Standard Format

All commands now return a uniform JSON response:

```json
{
  "code": 0,
  "output": "result data or message",
  "error": "error message if any"
}
```

### Fields

| Field | Type | Description |
|-------|------|-------------|
| `code` | integer | 0 = success, non-zero = error |
| `output` | string | Result data or success message |
| `error` | string | Error message (empty if code == 0) |

### Error Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Unknown command / Invalid argument |
| 2 | Timeout |
| 3 | Exception during execution |
| 4 | JSON parse error |
| 5 | Server error |
| 6 | Authentication error (invalid or missing token) |
| Others | Command-specific errors (e.g., shell exit codes) |

## Examples

### Success Response (simple)

```json
{
  "code": 0,
  "output": "hello",
  "error": ""
}
```

### Success Response (write_json)

```json
{
  "code": 0,
  "output": "File written: ./data/test.json",
  "error": ""
}
```

### Success Response (read_json)

**Note**: When the output is valid JSON (starts with `{` or `[`), it is automatically parsed as a JSON object instead of a string.

```json
{
  "code": 0,
  "output": {
    "k0": "v0",
    "k1": "v1"
  },
  "error": ""
}
```

This allows direct access to fields: `response.output.k0` instead of needing to parse `response.output` as a string.

### Error Response

```json
{
  "code": 1,
  "error": "unknown command: foo",
  "output": ""
}
```

### File Error (read_json)

```json
{
  "code": 1,
  "error": "failed to open file for reading: ./data/notexist.json",
  "output": ""
}
```

### JSON Parse Error (read_json)

```json
{
  "code": 4,
  "error": "JSON parse error: [json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid literal",
  "output": ""
}
```

### Complex Output (JSON in output field)

For commands like `call_shell` and `read_json` that return structured data:

**Old way (output as string)**:
```json
{
  "code": 0,
  "output": "{\"command\":\"ls\",\"exit_code\":0,\"stdout\":\"file1\\nfile2\\n\"}",
  "error": ""
}
```

**New way (output as object)**:
```json
{
  "code": 0,
  "output": {
    "command": "ls",
    "exit_code": 0,
    "stdout": "file1\nfile2\n"
  },
  "error": ""
}
```

The `output` field is now automatically parsed as JSON if it starts with `{` or `[`, allowing direct field access.

## Migration from Old Format

### Old Format
```json
{
  "ok": true,
  "message": "ok",
  "data": {"result": "hello"}
}
```

### New Format
```json
{
  "code": 0,
  "output": "hello",
  "error": ""
}
```

### Key Changes

- `ok` (boolean) → `code` (integer, 0=success)
- `message` + `data` → `output` (combined)
- Added explicit `error` field

## Client Code Updates

### Before
```javascript
if (response.ok) {
  console.log(response.data.result);
} else {
  console.error(response.message);
}
```

### After
```javascript
if (response.code === 0) {
  console.log(response.output);
} else {
  console.error(response.error);
}
```

## Automatic JSON Parsing in Output

**New Feature**: The `output` field is automatically parsed as a JSON object when it contains valid JSON (starts with `{` or `[`).

### Benefits

1. **Direct field access**: No need to parse strings
   ```javascript
   // Before: had to parse the string
   const data = JSON.parse(response.output);
   console.log(data.host);
   
   // After: direct access
   console.log(response.output.host);
   ```

2. **Type safety**: JSON objects are properly typed
3. **Cleaner code**: Less error-prone parsing logic
4. **Backward compatible**: Non-JSON outputs remain as strings

### Commands That Benefit

- **read_json**: Returns file content as JSON object
- **call_shell**: Returns command result as structured object
  ```json
  {
    "output": {
      "command": "ls",
      "exit_code": 0,
      "stdout": "..."
    }
  }
  ```

### Fallback Behavior

If the output starts with `{` or `[` but is not valid JSON, it remains as a string. This ensures robustness.

---

# Global JSON Variable API

## Overview

The system provides a global JSON variable (`GLOBAL_JSON`) that persists across requests:
- **Server**: In-memory JSON with automatic disk persistence
- **CLI**: Direct file operations on `data/GLOBAL_JSON.json`
- **File location**: `data/GLOBAL_JSON.json`

## HTTP Endpoints

### GET /get/global
Get the entire global JSON.

**Request**: None

**Response**:
```json
{
  "code": 0,
  "output": {
    "name": "Alice",
    "age": 30,
    "city": "NYC"
  },
  "error": ""
}
```

**Example**:
```bash
curl http://localhost:8080/get/global
```

---

### GET /get/global/path
Get a value at a specific JSON pointer path (RFC 6901).

**Query Parameters**:
- `path` (required): JSON pointer path (e.g., `/user/name`, `/config/theme`)

**Response** (success):
```json
{
  "code": 0,
  "output": "Alice",
  "error": ""
}
```

**Response** (path not found):
```json
{
  "code": 1,
  "output": "",
  "error": "path not found: /nonexistent"
}
```

**Examples**:
```bash
# Get top-level field
curl "http://localhost:8080/get/global/path?path=/name"

# Get nested field
curl "http://localhost:8080/get/global/path?path=/config/theme"
```

---

### POST /post/global
Replace the entire global JSON with a new value.

**Request Body**:
```json
{
  "value": {
    "product": "Widget",
    "price": 99.99,
    "stock": 100
  }
}
```

**Response**:
```json
{
  "code": 0,
  "output": "Global JSON updated",
  "error": ""
}
```

**Example**:
```bash
curl -X POST http://localhost:8080/post/global \
  -H "Content-Type: application/json" \
  -d '{"value":{"name":"Bob","age":25}}'
```

---

### POST /post/global/patch
Apply a JSON merge patch (RFC 7386) to the global JSON and get JSON Patch diff (RFC 6902).

**Request Body** (entire body is the patch):
```json
{
  "age": 31,
  "city": "NYC",
  "config": {
    "theme": "light"
  }
}
```

**Response** (JSON Patch diff):
```json
{
  "code": 0,
  "output": [
    {"op": "replace", "path": "/age", "value": 31},
    {"op": "add", "path": "/city", "value": "NYC"},
    {"op": "add", "path": "/config", "value": {"theme": "light"}}
  ],
  "error": ""
}
```

**Merge Patch Rules** (RFC 7386):
- If patch value is an **object**, recursively merge with existing object
- If patch value is **null**, delete the key
- Otherwise, replace the value

**Examples**:
```bash
# Update existing fields and add new ones
curl -X POST http://localhost:8080/post/global/patch \
  -H "Content-Type: application/json" \
  -d '{"patch":{"age":31,"city":"NYC"}}'

# Delete a field (set to null)
curl -X POST http://localhost:8080/post/global/patch \
  -H "Content-Type: application/json" \
  -d '{"patch":{"city":null}}'

# Nested merge
curl -X POST http://localhost:8080/post/global/patch \
  -H "Content-Type: application/json" \
  -d '{"patch":{"config":{"theme":"dark","lang":"en"}}}'
```

**Merge Patch Example**:
```
Current: {"a": 1, "b": {"c": 2, "d": 3}}
Patch:   {"b": {"c": 999, "e": 4}}
Result:  {"a": 1, "b": {"c": 999, "d": 3, "e": 4}}
```

---

### POST /post/global/persist
Manually force save the global JSON to disk.

**Note**: The global JSON is automatically saved after each modification (set/patch), so this endpoint is typically only needed for manual checkpoints.

**Request**: None

**Response**:
```json
{
  "code": 0,
  "output": "Global JSON persisted to disk",
  "error": ""
}
```

**Example**:
```bash
curl -X POST http://localhost:8080/post/global/persist
```

---

## CLI Commands

All global JSON operations are available via CLI commands:

### get_global_json
Get the entire global JSON or a specific path.

```bash
# Get entire JSON
./cpp_cli --cmd get_global_json --args '{}'

# Get specific path
./cpp_cli --cmd get_global_json --args '{"path":"/user/name"}'
```

---

### set_global_json
Replace the entire global JSON.

```bash
./cpp_cli --cmd set_global_json --args '{"value":{"name":"Alice","age":30}}'
```

---

### patch_global_json
Apply a merge patch and get the JSON Patch diff (RFC 6902).

```bash
./cpp_cli --cmd patch_global_json --args '{"age":31,"city":"NYC"}'
```

**Response includes JSON Patch diff**:
```json
{
  "code": 0,
  "output": [
    {"op": "replace", "path": "/age", "value": 31},
    {"op": "add", "path": "/city", "value": "NYC"}
  ],
  "error": ""
}
```

---

### persist_global_json
Force save to disk.

```bash
./cpp_cli --cmd persist_global_json --args '{}'
```

---

## Persistence Behavior

### Server Mode
- **On startup**: Loads `data/GLOBAL_JSON.json` into memory
- **On modification**: Automatically saves to disk after each `set` or `patch`
- **On shutdown**: State is preserved in `data/GLOBAL_JSON.json`
- **If file missing**: Initializes as empty object `{}`

### CLI Mode
- **No in-memory cache**: Each command loads, modifies, and saves the file
- **Direct file operations**: Operates directly on `data/GLOBAL_JSON.json`

---

## Use Cases

1. **Configuration Management**: Store app configuration that persists across restarts
2. **State Storage**: Maintain application state in memory with disk backup
3. **Feature Flags**: Dynamic feature toggles without redeployment
4. **Counters/Metrics**: Track values that need to persist
5. **User Preferences**: Store per-user settings
6. **Cache**: Quick in-memory lookup with persistence

---

## Thread Safety

All global JSON operations are thread-safe:
- Uses `std::shared_mutex` for concurrent read/exclusive write access
- Multiple readers can access simultaneously
- Writers have exclusive access during modifications
- Safe for concurrent HTTP requests

## Binary Distribution

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

## Static Linking Build

# Static Build Guide

This guide explains how to build portable static binaries that can run on any Linux/Windows system without external dependencies.

---

## Why Static Builds?

**Benefits:**
- ✅ **Portability**: Copy binary to any machine, no dependency installation needed
- ✅ **Cloud Deployment**: Upload to cloud servers without worrying about library versions
- ✅ **No Runtime Errors**: No "library not found" errors on target systems
- ✅ **Version Lock**: Binary includes exact library versions, avoiding compatibility issues

**Trade-offs:**
- Larger binary size (includes libraries)
- Cannot benefit from system security updates to shared libraries
- Must rebuild to update included library versions

---

## Linux/Ubuntu

### Standard Build (Dynamic Linking)
```bash
./build.sh
```

Dependencies check:
```bash
ldd build/cpp_srv
# Output: Shows dynamic library dependencies (libpthread, libssl, libc, etc.)
```

### Static Build (Portable)
```bash
./build.sh --static
```

The build script will:
1. Configure CMake with `-DSTATIC_BUILD=ON`
2. Link C++ standard library statically (`-static-libgcc -static-libstdc++`)
3. Prefer `.a` static libraries over `.so` shared libraries
4. Show dependency check after build

**Example output:**
```
=== Building cpp_cli_srv on Linux (STATIC) ===
Note: Static build creates portable binaries that work on any Linux system
Tip: Use './build.sh --static' for portable static binaries

...
Static linking check:
  • cpp_srv dynamic dependencies:
    libssl.so.3 => /lib/x86_64-linux-gnu/libssl.so.3
    libcrypto.so.3 => /lib/x86_64-linux-gnu/libcrypto.so.3
    libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0
    libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6
  ✓ cpp_cli is fully static
```

**Note**: Full static linking (including glibc and OpenSSL) requires additional steps below.

---

## Full Static Build (Linux)

For completely static binaries with no dependencies at all, you need static versions of all libraries.

### Install Static Libraries

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install libssl-dev

# For truly static OpenSSL, build from source:
wget https://www.openssl.org/source/openssl-3.0.13.tar.gz
tar -xzf openssl-3.0.13.tar.gz
cd openssl-3.0.13
./config no-shared --prefix=/usr/local/openssl-static
make -j$(nproc)
sudo make install
```

### Build with Static OpenSSL

```bash
# Clean previous build
rm -rf build

# Build with custom OpenSSL path
mkdir -p build
cd build
cmake .. \
  -DSTATIC_BUILD=ON \
  -DOPENSSL_ROOT_DIR=/usr/local/openssl-static \
  -DOPENSSL_USE_STATIC_LIBS=TRUE
make -j4
```

### Build with Musl (Fully Static Alternative)

Using musl libc instead of glibc produces truly static binaries:

```bash
# Install musl-gcc
sudo apt install musl-tools

# Build with musl
rm -rf build
mkdir -p build
cd build
CC=musl-gcc CXX=musl-g++ cmake .. -DSTATIC_BUILD=ON
make -j4
```

Verify:
```bash
ldd build/cpp_srv
# Output: "not a dynamic executable"
```

---

## Windows

### Standard Build (Dynamic Runtime)
```bat
build.bat
```

Uses `/MD` flag (dynamic C++ runtime - requires MSVCP140.dll, VCRUNTIME140.dll)

### Static Build (Portable)
```bat
build.bat --static
```

Uses `/MT` flag (static C++ runtime - no DLL dependencies)

**Advantages:**
- Binary runs on any Windows 10+ machine
- No "VCRUNTIME140.dll not found" errors
- No Visual C++ Redistributable installation needed

**Example:**
```
Building with static runtime for portability...

[1/2] Building CLI...
CLI OK
[2/2] Building Server...
Server OK

=== Build complete ===
Static runtime: Binaries include C++ runtime (/MT)
```

---

## Deployment

### Linux Deployment

**Standard build:**
```bash
# On build machine
./build.sh

# Copy to target machine (requires same distro/version)
scp build/cpp_srv user@target:/opt/cpp_cli_srv/
```

**Static build:**
```bash
# On build machine
./build.sh --static

# Copy to any Linux machine
scp build/cpp_srv user@target:/opt/cpp_cli_srv/
scp -r build/web user@target:/opt/cpp_cli_srv/

# On target machine (no installation needed!)
chmod +x /opt/cpp_cli_srv/cpp_srv
/opt/cpp_cli_srv/cpp_srv --port 8080
```

### Windows Deployment

**Standard build (requires redistributable):**
```bat
REM Users must install Visual C++ Redistributable first
REM Download: https://aka.ms/vs/17/release/vc_redist.x64.exe
```

**Static build (no dependencies):**
```bat
REM Build
build.bat --static

REM Copy to any Windows machine - works immediately
copy build\cpp_srv.exe \\target\share\
copy build\web\* \\target\share\web\
```

---

## Cloud Deployment Examples

### AWS EC2
```bash
# Build on your machine
./build.sh --static

# Upload to EC2 (any Amazon Linux, Ubuntu, etc.)
scp -i key.pem build/cpp_srv ec2-user@instance:/home/ec2-user/
scp -r -i key.pem build/web ec2-user@instance:/home/ec2-user/

# SSH and run
ssh -i key.pem ec2-user@instance
chmod +x cpp_srv
./cpp_srv --port 8080
```

### Docker (Minimal Image)
```dockerfile
# Dockerfile for static binary
FROM scratch
COPY build/cpp_srv /cpp_srv
COPY build/web /web
EXPOSE 8080
CMD ["/cpp_srv", "--port", "8080"]
```

Build:
```bash
# Build static binary on host
./build.sh --static

# Build Docker image (only 2-3 MB!)
docker build -t cpp_cli_srv:static .
docker run -p 8080:8080 cpp_cli_srv:static
```

### Google Cloud Run / Azure Container Apps
```bash
# Build
./build.sh --static

# Create minimal container
cat > Dockerfile << 'EOF'
FROM alpine:latest
RUN apk add --no-cache ca-certificates
COPY build/cpp_srv /usr/local/bin/
COPY build/web /usr/local/share/web
EXPOSE 8080
CMD ["cpp_srv", "--port", "8080"]
EOF

docker build -t gcr.io/project/cpp_srv .
docker push gcr.io/project/cpp_srv
```

---

## Verification

### Check Binary Dependencies

**Linux:**
```bash
# Check what libraries are linked
ldd build/cpp_srv

# Check binary size
ls -lh build/cpp_srv

# Static binaries are larger but self-contained
```

**Windows:**
```bat
REM Check dependencies with Dependency Walker or dumpbin
dumpbin /dependents build\cpp_srv.exe

REM Static builds show only system DLLs (KERNEL32.dll, etc.)
```

### Test Portability

**Create clean test environment:**
```bash
# Using Docker to simulate clean system
docker run -it --rm -v $(pwd)/build:/app alpine:latest /bin/sh

# Inside container
cd /app
./cpp_srv --help
# Should work without installing anything!
```

---

## Troubleshooting

### Issue: "Cannot find -lstdc++"

**Cause**: Static C++ library not available

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install libstdc++-10-dev

# Or use dynamic linking for C++ stdlib
cmake .. -DSTATIC_BUILD=ON -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc"
```

### Issue: Static OpenSSL not working

**Cause**: OpenSSL shared libraries found instead of static

**Solution**:
```bash
# Remove shared OpenSSL libs temporarily
sudo mv /usr/lib/x86_64-linux-gnu/libssl.so /tmp/
sudo mv /usr/lib/x86_64-linux-gnu/libcrypto.so /tmp/

# Build
./build.sh --static

# Restore
sudo mv /tmp/libssl.so /usr/lib/x86_64-linux-gnu/
sudo mv /tmp/libcrypto.so /usr/lib/x86_64-linux-gnu/
```

**Or specify static library path:**
```bash
cmake .. \
  -DSTATIC_BUILD=ON \
  -DOPENSSL_ROOT_DIR=/usr \
  -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.a \
  -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.a
```

### Issue: Binary still has dependencies

**Check which libraries are still dynamic:**
```bash
ldd build/cpp_srv | grep "=>"
```

Common remaining dependencies:
- `linux-vdso.so` - Virtual dynamic shared object (always present, not a real file)
- `ld-linux.so` - Dynamic linker (needed unless using musl)
- `libc.so` - System C library (static linking glibc is problematic)
- `libpthread.so` - Threading (usually safe to keep dynamic)

These are typically acceptable even for "static" builds.

### Issue: Larger binary size

**This is expected:**
- Standard build: 200-500 KB
- Static build: 2-5 MB
- Full static (with OpenSSL): 5-10 MB

**If size is a concern:**
```bash
# Strip debug symbols
strip build/cpp_srv

# Use UPX compression (optional)
upx --best build/cpp_srv
```

---

## Best Practices

1. **Development**: Use standard dynamic builds for faster compilation
2. **Testing**: Use dynamic builds with system libraries
3. **Production/Cloud**: Use static builds for portability
4. **Docker**: Use static builds with minimal base images
5. **Windows**: Always use static builds (`/MT`) for distribution

---

## Build Comparison

| Aspect | Standard Build | Static Build |
|--------|---------------|--------------|
| **Build Command** | `./build.sh` | `./build.sh --static` |
| **Binary Size** | ~300 KB | ~3-5 MB |
| **Portability** | Same OS/distro | Any Linux system |
| **Dependencies** | System libraries | Self-contained |
| **Build Time** | Faster | Slightly slower |
| **Updates** | Get system updates | Must rebuild |
| **Use Case** | Development, system package | Cloud, Docker, portable |

---

## Quick Reference

```bash
# Linux - Standard
./build.sh

# Linux - Static (portable)
./build.sh --static

# Windows - Standard
build.bat

# Windows - Static (portable)
build.bat --static

# Check dependencies (Linux)
ldd build/cpp_srv

# Check dependencies (Windows)
dumpbin /dependents build\cpp_srv.exe

# Clean rebuild
rm -rf build
./build.sh --static
```

For most cloud deployment scenarios, **static builds** are recommended for maximum portability and minimal setup on target machines.

## Linux/Ubuntu Build

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

## HTTPS/SSL Setup

# HTTPS Configuration Guide

This guide explains how to enable HTTPS support in cpp_cli_srv.

---

## Prerequisites

### 1. Install OpenSSL Development Libraries

The HTTPS feature requires OpenSSL development libraries to be installed:

| Distribution | Command |
|--------------|---------|
| **Ubuntu/Debian** | `sudo apt update && sudo apt install libssl-dev` |
| **CentOS/RHEL** | `sudo yum install openssl-devel` |
| **Fedora** | `sudo dnf install openssl-devel` |
| **Arch Linux** | `sudo pacman -S openssl` |

### 2. Verify OpenSSL Installation

```bash
# Check if OpenSSL development files are installed
pkg-config --modversion openssl

# Expected output: version number like 1.1.1 or 3.0.x
```

### 3. Rebuild with OpenSSL Support

If you previously built without OpenSSL, clean and rebuild:

```bash
rm -rf build
./build.sh
```

You should see:
```
-- OpenSSL found - HTTPS support enabled
```

---

## SSL Certificate Setup

### Option 1: Use Existing Certificates

If you already have SSL certificates (e.g., from Let's Encrypt or a CA):

1. Place your certificate files in a directory:
   ```bash
   mkdir -p ssl
   cp your_certificate.crt ssl/server.crt
   cp your_private_key.key ssl/server.key
   ```

2. Set proper permissions:
   ```bash
   chmod 600 ssl/server.key
   chmod 644 ssl/server.crt
   ```

3. Start the server:
   ```bash
   ./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl
   ```

### Option 2: Generate Self-Signed Certificate (Development)

For development and testing, you can create a self-signed certificate:

```bash
# Create SSL directory
mkdir -p ssl

# Generate self-signed certificate (valid for 365 days)
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -subj "/C=US/ST=State/L=City/O=Organization/CN=localhost"

# Set permissions
chmod 600 ssl/server.key
chmod 644 ssl/server.crt
```

**Parameters explained:**
- `-x509`: Generate a self-signed certificate
- `-newkey rsa:4096`: Create a new 4096-bit RSA key
- `-nodes`: Don't encrypt the private key (no passphrase)
- `-days 365`: Certificate valid for 1 year
- `-subj`: Certificate subject (customize as needed)

**For custom domain:**
```bash
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -subj "/C=US/ST=California/L=San Francisco/O=MyCompany/CN=example.com"
```

**For multiple domains (SAN):**
```bash
# Create config file
cat > ssl/san.cnf << 'EOF'
[req]
default_bits = 4096
prompt = no
default_md = sha256
distinguished_name = dn
req_extensions = v3_req

[dn]
C=US
ST=California
L=San Francisco
O=MyCompany
CN=localhost

[v3_req]
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = example.com
DNS.3 = *.example.com
IP.1 = 127.0.0.1
IP.2 = ::1
EOF

# Generate certificate with SAN
openssl req -x509 -newkey rsa:4096 -nodes \
  -keyout ssl/server.key \
  -out ssl/server.crt \
  -days 365 \
  -config ssl/san.cnf \
  -extensions v3_req
```

---

## Starting the Server

### HTTP Only (Default)
```bash
./build/cpp_srv --port 8080
```

### HTTPS Only
```bash
./build/cpp_srv --port_https 8443 --ssl ./ssl
```

### Both HTTP and HTTPS
```bash
./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl
```

### With All Options
```bash
./build/cpp_srv \
  --port 8080 \
  --port_https 8443 \
  --ssl ./ssl \
  --threads 8 \
  --log server.log \
  --token mytoken123
```

---

## Testing HTTPS

### Using curl

```bash
# Self-signed certificate (ignore verification)
curl -k https://localhost:8443/get/status

# With certificate verification
curl --cacert ssl/server.crt https://localhost:8443/get/status

# POST request
curl -k -X POST https://localhost:8443/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"echo","args":{"text":"Hello HTTPS"}}'
```

### Using Web Browser

1. Navigate to `https://localhost:8443`
2. Accept the security warning (for self-signed certificates)
3. Use the web interface normally

**Note**: Browsers will show a warning for self-signed certificates. This is expected and safe for development.

### Using the Test Script

```bash
./test_https.sh
```

This runs a comprehensive test suite including:
- HTTP endpoint connectivity
- HTTPS endpoint connectivity
- HTTPS POST requests
- Concurrent HTTP + HTTPS
- IPv6 HTTPS support

---

## Common Issues

### Issue: "OpenSSL not found"

**Solution**: Install OpenSSL development libraries:
```bash
# Ubuntu/Debian
sudo apt install libssl-dev

# After installing, rebuild
rm -rf build
./build.sh
```

### Issue: "Cannot bind HTTPS"

**Causes**:
1. Port already in use
2. Permission denied (ports < 1024 require root)

**Solutions**:
```bash
# Check if port is in use
sudo netstat -tulpn | grep :8443

# Use port > 1024 (no root needed)
./build/cpp_srv --port_https 8443 --ssl ./ssl

# Or run as root for port 443
sudo ./build/cpp_srv --port_https 443 --ssl ./ssl
```

### Issue: "SSL certificate not found"

**Cause**: Certificate files missing or wrong path

**Solution**:
```bash
# Verify files exist
ls -l ssl/
# Should show: server.crt and server.key

# Use absolute path
./build/cpp_srv --port_https 8443 --ssl /full/path/to/ssl
```

### Issue: Browser shows "Your connection is not private"

**Cause**: Using self-signed certificate

**Solution**: This is expected for self-signed certificates.
- **Development**: Click "Advanced" → "Proceed to localhost"
- **Production**: Use certificates from a trusted CA (Let's Encrypt, etc.)

---

## Production Deployment

### Using Let's Encrypt (Free Certificates)

```bash
# Install certbot
sudo apt install certbot

# Get certificate (HTTP-01 challenge)
sudo certbot certonly --standalone -d yourdomain.com

# Certificates will be in:
# /etc/letsencrypt/live/yourdomain.com/fullchain.pem (server.crt)
# /etc/letsencrypt/live/yourdomain.com/privkey.pem (server.key)

# Copy to your ssl directory
sudo cp /etc/letsencrypt/live/yourdomain.com/fullchain.pem ssl/server.crt
sudo cp /etc/letsencrypt/live/yourdomain.com/privkey.pem ssl/server.key
sudo chown $USER:$USER ssl/server.*

# Start server
./build/cpp_srv --port 80 --port_https 443 --ssl ./ssl
```

### Security Best Practices

1. **Use strong keys**: Minimum 2048-bit RSA (4096-bit recommended)
2. **Restrict key permissions**: `chmod 600 server.key`
3. **Regular certificate renewal**: Let's Encrypt certs expire every 90 days
4. **Use modern TLS**: cpp-httplib uses OpenSSL defaults (TLS 1.2+)
5. **Disable insecure ciphers**: Handled automatically by OpenSSL
6. **Run as non-root**: Use ports > 1024 or systemd socket activation
7. **Firewall configuration**: Only expose necessary ports

### Systemd Service Example

```ini
[Unit]
Description=cpp_cli_srv with HTTPS
After=network.target

[Service]
Type=simple
User=your-user
WorkingDirectory=/path/to/cpp_cli_srv
ExecStart=/path/to/cpp_cli_srv/build/cpp_srv \
  --port 8080 \
  --port_https 8443 \
  --ssl /path/to/ssl \
  --log /var/log/cpp_srv.log \
  --token ${TOKEN}
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

---

## Monitoring

### Check HTTPS is enabled:

```bash
# Server should show HTTPS lines
./build/cpp_srv --port 8080 --port_https 8443 --ssl ./ssl
# Output includes:
#   IPv4 (HTTPS) : https://0.0.0.0:8443
#   SSL     : ./ssl
```

### Test certificate validity:

```bash
# Check certificate details
openssl x509 -in ssl/server.crt -text -noout

# Check certificate expiration
openssl x509 -in ssl/server.crt -noout -enddate

# Test TLS connection
openssl s_client -connect localhost:8443 -showcerts
```

### Monitor connections:

```bash
# Check HTTPS connections
sudo netstat -tulpn | grep :8443

# Monitor server logs (if --log enabled)
tail -f server.log | grep HTTPS
```

---

## Support

For issues or questions:
1. Check the troubleshooting section above
2. Review server logs (if `--log` enabled)
3. Run `./test_https.sh` to verify HTTPS functionality
4. Ensure OpenSSL is installed: `pkg-config --modversion openssl`

## Logging

# Server Logging Feature

## Overview

The server now includes comprehensive logging with timestamped entries for all HTTP requests and server events.

## Timestamp Format

All log entries use the format: `[YYYYMMDD_HHMMSS]`

Example: `[20260330_112149]` = March 30, 2026 at 11:21:49

## Usage

Enable logging by specifying the `--log` option:

```bash
# Linux/Mac
./build/cpp_srv --log server.log

# Windows
build\cpp_srv.exe --log server.log

# With other options
./build/cpp_srv --port 8080 --threads 8 --log server.log
```

## Log Output

Logs are written to **both**:
1. **Console** (stdout) - for real-time monitoring
2. **Log file** (if --log specified) - for persistent storage

## Log Entry Format

### Server Events
```
[20260330_112149] Server starting...
[20260330_112149] IPv4 server bound to 0.0.0.0:8080
[20260330_112149] IPv6 server bound to [::]:8080
[20260330_112149] Server ready - accepting connections
[20260330_112300] Server shutting down
```

### HTTP Requests
```
[timestamp] METHOD path | IP=client_ip | Status=http_code | In=request_body | Out=response_body
```

Example log entries:

```
[20260330_112151] GET /get/status | IP=::1 | Status=200 | Out={"active":0,"message":"running","ok":true,"threads":20}

[20260330_112151] POST /post/run | IP=::1 | Status=200 | In={"cmd":"echo","args":{"text":"Hello"}} | Out={"data":{"result":"Hello"},"message":"ok","ok":true}

[20260330_112151] POST /post/run | IP=::1 | Status=400 | In={"cmd":"unknown","args":{}} | Out={"data":null,"message":"unknown command: unknown","ok":false}

[20260330_112151] POST /post/run | IP=127.0.0.1 | Status=200 | In={"cmd":"add","args":{"a":10,"b":20}} | Out={"data":{"result":30.0},"message":"ok","ok":true}

[20260330_112151] GET /get/schema | IP=::1 | Status=200 | Out=[639 bytes]

[20260330_112151] GET / | IP=::1 | Status=200 | Out=[HTML 5458 bytes]
```

## Log Fields

Each HTTP request log includes:

| Field | Description | Example |
|-------|-------------|---------|
| **Timestamp** | `[YYYYMMDD_HHMMSS]` format | `[20260330_112151]` |
| **Method** | HTTP method | `GET`, `POST`, `OPTIONS` |
| **Path** | Request path | `/post/run`, `/get/schema`, `/` |
| **IP** | Client IP address | `127.0.0.1` (IPv4), `::1` (IPv6) |
| **Status** | HTTP status code | `200`, `400`, `404`, `500`, etc. |
| **In** | Request body (POST only) | `{"cmd":"echo","args":{...}}` |
| **Out** | Response body | `{"ok":true,"data":{...}}` |

## Special Cases

### Large Payloads
- Bodies **> 200 bytes** are truncated with size indication:
  - `In=[1024 bytes]`
  - `Out=[HTML 5458 bytes]`

### Client IP Detection
- Reads `X-Forwarded-For` header if present (for proxies)
- Falls back to direct connection IP
- Shows IPv4 (`127.0.0.1`) or IPv6 (`::1`, `::ffff:192.168.1.1`)

### Error Cases
All errors are logged with appropriate status codes:
- `400` - Bad request (unknown command, missing field, JSON parse error)
- `404` - Not found (missing web/index.html)
- `413` - Request body too large
- `500` - Internal server error

## Testing

Run the logging test script:

```bash
./test_logging.sh
```

This will:
1. Start a server with logging enabled
2. Make various test requests (GET/POST, success/error, IPv4/IPv6)
3. Display the complete log output
4. Explain the log format

## Implementation Details

### Thread Safety
- Logger uses `std::mutex` for thread-safe writes
- Multiple threads can log simultaneously without corruption
- Log entries are atomic (one line per log call)

### Files Modified
- `core/logger.h` - New logger class with timestamp formatting
- `server/main.cpp` - Integrated logging into all routes

### No Dependencies
- Uses only C++ standard library
- No external logging libraries required
- Works on Windows, Linux, and macOS

## Performance Impact

Minimal performance overhead:
- Non-blocking: writes happen asynchronously
- Efficient: single mutex lock per log entry
- Optional: only enabled when `--log` is specified
- Console output continues regardless of `--log` flag

## Log Rotation

The logger appends to the log file. For log rotation in production:

```bash
# Rotate logs manually
mv server.log server.log.$(date +%Y%m%d_%H%M%S)

# Or use logrotate (Linux)
# /etc/logrotate.d/mytool-server:
/path/to/server.log {
    daily
    rotate 7
    compress
    missingok
    notifempty
}
```

## Examples

### Development
```bash
# Log to console only
./build/cpp_cli_srv

# Log to both console and file
./build/cpp_cli_srv --log dev.log
```

### Production
```bash
# Full logging with custom port
./build/cpp_cli_srv --port 8080 --threads 16 --log /var/log/mytool/server.log

# IPv4 only with logging
./build/cpp_cli_srv --no-ipv6 --log server.log
```

### Monitoring Logs in Real-Time
```bash
# Start server with logging
./build/cpp_cli_srv --log server.log &

# Watch log file
tail -f server.log

# Or filter for specific patterns
tail -f server.log | grep "POST /post/run"
tail -f server.log | grep "Status=400"
tail -f server.log | grep "IP=127.0.0.1"
```

## Troubleshooting: Permission Issues

# Permission Denied Error - Quick Fix

## Problem

When copying the cpp_cli_srv project to another machine, you may encounter:

```
make[2]: stat: /root/jd/t/t0/cc1/Claw/cpp_cli_srv/cli/main.cpp: Permission denied
make[2]: stat: /root/jd/t/t0/cc1/Claw/cpp_cli_srv/server/main.cpp: Permission denied
```

This happens when files are copied without preserving permissions.

## Quick Solution

### On the new machine:

```bash
# 1. Go to project directory
cd /path/to/cpp_cli_srv

# 2. Run the fix script
./fix_permissions.sh

# 3. Rebuild
./build.sh
```

Done! The build should now work.

## Manual Fix (if fix_permissions.sh is missing or not executable)

```bash
# Make all directories accessible
find . -type d -exec chmod 755 {} \;

# Make all files readable
find . -type f -exec chmod 644 {} \;

# Make scripts executable
chmod +x *.sh

# Rebuild
./build.sh
```

## How to Copy the Project Correctly

To avoid this issue in the future:

### Method 1: Use cp with -p flag
```bash
cp -rp /source/cpp_cli_srv /destination/
```

### Method 2: Use tar (recommended for transfers)
```bash
# On source machine:
tar czf cpp_cli_srv.tar.gz cpp_cli_srv/

# Transfer the file, then on target machine:
tar xzf cpp_cli_srv.tar.gz
```

### Method 3: Use rsync (for network copies)
```bash
rsync -av /source/cpp_cli_srv/ user@remote:/destination/
```

### Method 4: Use Git (best option)
```bash
git clone <repository-url>
```
Git automatically preserves executable permissions.

## Why This Happens

When files are copied (especially via certain file transfer methods like Windows network shares, some FTP clients, or plain `cp`), they may lose their permission bits. The build system (make) needs to be able to:
- **Read** source files (.cpp, .h)
- **Execute** directory traversal
- **Execute** build scripts

The fix script restores these permissions:
- **755** for directories (rwxr-xr-x)
- **644** for files (rw-r--r--)
- **755** for scripts (rwxr-xr-x)

## Verify Permissions Are Fixed

Check a few key files:
```bash
ls -la cli/main.cpp      # Should show: -rw-r--r--
ls -la build.sh          # Should show: -rwxr-xr-x
ls -la core/            # Should show: drwxr-xr-x
```

## Token Security

# Token Security for call_shell Command

## Overview

Token-based authentication has been added to the `call_shell` command to prevent unauthorized shell command execution via the web interface.

## Key Features

### 1. Token Validation
- **Format Requirements**: Token must be ≥2 characters, alphanumeric + underscore only (`[a-zA-Z0-9_]+`)
- **Validation**: Performed at server startup and during each web request
- **Error Code**: Returns `code: 6` for authentication failures

### 2. Security Model

| Access Method | Token Required? | Reason |
|---------------|-----------------|--------|
| **Web Interface** | ✅ Yes | Untrusted remote access |
| **CLI** | ❌ No | Trusted local execution |

### 3. Server Configuration

Start server with token:
```bash
# Linux
./build/cpp_srv --token mytoken123

# Windows
build\cpp_srv.exe --token mytoken123
```

**Without token:** `call_shell` is completely disabled via web (returns code 6).

### 4. Web Interface Behavior

- **First Use**: Prompts user for token via JavaScript `prompt()` dialog
- **Token Caching**: Stores token in `sessionStorage` for the session
- **Auto-Retry**: If token is rejected (code 6), clears cache and prompts again
- **Other Commands**: Unaffected, work without token

### 5. API Usage

```bash
# Without token - REJECTED
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"cat /etc/*release"}}'
# Response: {"code":6,"error":"call_shell requires valid token","output":""}

# With token - ACCEPTED
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"cat /etc/*release"},"token":"mytoken123"}'
# Response: {"code":0,"error":"","output":"..."}
```

## Implementation Details

### Files Modified

1. **server/main.cpp**
   - Added `is_valid_token()` validation function
   - Added `--token` parameter parsing
   - Modified `mount_routes()` to accept and capture token
   - Added token verification in `/post/run` endpoint before executing `call_shell`

2. **web/index.html**
   - Updated `runCmd()` to check for `call_shell` command
   - Prompts for token on first use
   - Caches token in `sessionStorage`
   - Clears token if authentication fails
   - Updated to handle new JSON format (`code` instead of `ok`)

3. **API_FORMAT.md**
   - Added error code 6: Authentication error

4. **README.md**
   - Added `--token` parameter documentation
   - Added security notes for `call_shell`
   - Added token test scripts to testing table

### Testing

**Automated Tests:**
- `test_token.sh` (Linux)
- `test_token.bat` (Windows)

**Test Coverage:**
1. ✅ call_shell without token → rejected (code 6)
2. ✅ call_shell with wrong token → rejected (code 6)
3. ✅ call_shell with correct token → succeeds (code 0)
4. ✅ Other commands without token → work normally
5. ✅ CLI call_shell without token → works (CLI is trusted)

## Security Considerations

### Strong Token Recommendations

```bash
# Generate cryptographically secure token
openssl rand -hex 16    # Linux/Mac
# Output example: e4f7c9a2b5d8f1e3c6a9b2d5f8e1c4a7

# Windows PowerShell
[Convert]::ToBase64String((1..24 | ForEach-Object { Get-Random -Maximum 256 }))
```

### Production Deployment Checklist

- [ ] Use strong, randomly-generated token
- [ ] Never commit token to source control
- [ ] Run server with restricted user permissions
- [ ] Use firewall/network isolation
- [ ] Enable logging with `--log` parameter
- [ ] Monitor logs for suspicious activity
- [ ] Consider using reverse proxy with additional auth (e.g., nginx + basic auth)
- [ ] Implement rate limiting if needed
- [ ] Rotate tokens periodically

### Threat Model

**Protected Against:**
- Unauthorized web access to shell commands
- Casual/accidental command execution
- Basic security scanning tools

**NOT Protected Against:**
- Token leaked in logs/error messages
- Man-in-the-middle attacks (use HTTPS)
- Compromised token
- Physical access to server
- Memory/process inspection

**Recommendations:**
- Use HTTPS in production (terminate with nginx/Apache)
- Keep tokens out of URLs (body-only, as implemented)
- Don't log token values
- Implement IP whitelisting if possible

## Error Codes

| Code | Meaning | When |
|------|---------|------|
| 0 | Success | Command executed successfully |
| 6 | Authentication error | Token missing, invalid, or doesn't match |

## Backward Compatibility

- **CLI**: Unchanged, works without token
- **Web without token parameter**: Server disables `call_shell` if no `--token` provided at startup
- **Other commands**: Completely unaffected by token feature

## Future Enhancements

Potential improvements (not currently implemented):
1. Token expiration/refresh
2. Multiple tokens with different permissions
3. Command whitelisting/blacklisting
4. Rate limiting per token
5. Audit log with token identification
6. HMAC-based token signing
7. OAuth2/JWT integration

## Questions & Troubleshooting

**Q: Can I use call_shell via CLI without a token?**  
A: Yes, CLI is trusted and doesn't require token authentication.

**Q: What happens if I don't provide --token at startup?**  
A: `call_shell` is disabled for web access. Web requests will return code 6 with message "call_shell is disabled (no token configured)".

**Q: Can I change the token without restarting?**  
A: No, token is fixed at server startup. Restart the server with a new `--token` value.

**Q: Is the token encrypted in transit?**  
A: No, use HTTPS in production to encrypt all HTTP traffic including tokens.

**Q: What if my token gets compromised?**  
A: Restart the server with a new token. Compromised tokens have full shell access until server restart.

**Q: Can I disable the token prompt in the web UI?**  
A: Not recommended. Modify `web/index.html` if you must, but this weakens security.

