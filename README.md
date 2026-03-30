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

| Platform | Command |
|----------|---------|
| **Windows** | Install Visual Studio 2022 with C++ Desktop Development |
| **Ubuntu/Linux** | `sudo apt update && sudo apt install build-essential cmake` |

---

## Build

| Platform | Command | Output Binaries |
|----------|---------|-----------------|
| **Windows** | `build.bat` | `build\cpp_cli.exe`<br>`build\cpp_srv.exe` |
| **Linux** | `./build.sh` | `build/cpp_cli`<br>`build/cpp_srv` |

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

### Basic commands:

| Platform | Command | Description |
|----------|---------|-------------|
| **Windows** | `build\cpp_cli.exe --schema` | List all commands |
| **Linux** | `./build/cpp_cli --schema` | List all commands |
| **Windows** | `build\cpp_cli.exe --cmd echo --args "{\"text\":\"hello\"}"` | Run command |
| **Linux** | `./build/cpp_cli --cmd echo --args '{"text":"hello"}'` | Run command |

### Complete usage examples:

```bash
# Windows (PowerShell / CMD)
build\cpp_cli.exe --schema
build\cpp_cli.exe --cmd echo  --args "{\"text\":\"hello\"}"
build\cpp_cli.exe --cmd add   --args "{\"a\":3,\"b\":4}"
build\cpp_cli.exe --cmd upper --args "{\"text\":\"hello\"}"
build\cpp_cli.exe --cmd add --args "{\"a\":3,\"b\":4}" --human

# Linux (Bash)
./build/cpp_cli --schema
./build/cpp_cli --cmd echo --args '{"text":"hello"}'
./build/cpp_cli --cmd add --args '{"a":3,"b":4}'
./build/cpp_cli --cmd upper --args '{"text":"hello"}'
./build/cpp_cli --cmd add --args '{"a":3,"b":4}' --human
```

### Output format

Default (machine/agent mode):
```json
{"ok":true,"message":"ok","data":{"result":<value>}}
```

With `--human`:
```
[OK] ok
{"result": 7}
```

Exit code is `0` on success, `1` on any error — pipeline-safe.

---

## Server Usage

### Starting the server:

| Platform | Command | Description |
|----------|---------|-------------|
| **Windows** | `build\cpp_srv.exe` | Start on default port 8080 |
| **Linux** | `./build/cpp_srv` | Start on default port 8080 |
| **Windows** | `build\cpp_srv.exe --port 9090` | Custom port |
| **Linux** | `./build/cpp_srv --port 9090` | Custom port |
| **Windows** | `build\cpp_srv.exe --threads 8` | Custom thread count |
| **Linux** | `./build/cpp_srv --threads 8` | Custom thread count |
| **Windows** | `build\cpp_srv.exe --no-ipv6` | IPv4 only |
| **Linux** | `./build/cpp_srv --no-ipv6` | IPv4 only |
| **Windows** | `build\cpp_srv.exe --log server.log` | Enable file logging |
| **Linux** | `./build/cpp_srv --log server.log` | Enable file logging |

### Combined options example:

```bash
# Windows
build\cpp_srv.exe --port 8080 --threads 8 --log server.log

# Linux
./build/cpp_srv --port 8080 --threads 8 --log server.log
```

On startup the server prints:

```
=== cpp_srv started ===
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

---

## Running Tests

| Test Type | Windows | Linux | Description |
|-----------|---------|-------|-------------|
| **CLI tests** | `test_cli.bat` | `./test_cli.sh` | CLI smoke tests (no server needed) |
| **API tests** | Start server, then `test_server.bat` | Start server, then `./test_server.sh` | REST API tests |
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
| **Server can't find web/index.html** | Both | Run server from `build/` directory |

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
