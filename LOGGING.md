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
