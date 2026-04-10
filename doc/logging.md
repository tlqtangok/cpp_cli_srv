# Server Logging

## Overview

When started with `--log <file>`, the server writes all activity to both **console** and the specified **log file**.

---

## Enable Logging

```bash
# Linux
./build/cpp_srv --log server.log

# Windows
build\cpp_srv.exe --log server.log

# Combined options
./build/cpp_srv --port 8080 --threads 8 --log server.log
```

---

## Log Format

### Timestamp

All entries use `[YYYYMMDD_HHMMSS]` format.  
Example: `[20260330_112149]` = March 30, 2026 at 11:21:49

### Server Events

```
[20260330_112149] Server starting...
[20260330_112149] IPv4 server bound to 0.0.0.0:8080
[20260330_112149] IPv6 server bound to [::]:8080
[20260330_112149] Server ready - accepting connections
[20260330_112300] Server shutting down
```

### HTTP Request Entries

```
[timestamp] METHOD path | IP=client_ip | Status=http_code | In=request_body | Out=response_body
```

Examples:
```
[20260330_112151] GET /get/status | IP=::1 | Status=200 | Out={"active":0,"message":"running","ok":true,"threads":20}
[20260330_112151] POST /post/run | IP=::1 | Status=200 | In={"cmd":"echo","args":{"text":"Hello"}} | Out={"code":0,"output":"Hello","error":""}
[20260330_112151] POST /post/run | IP=::1 | Status=400 | In={"cmd":"unknown","args":{}} | Out={"code":3,"error":"unknown command: unknown","output":""}
[20260330_112151] GET /get/schema | IP=::1 | Status=200 | Out=[639 bytes]
[20260330_112151] GET / | IP=::1 | Status=200 | Out=[HTML 5458 bytes]
```

---

## Log Fields

| Field | Description | Example |
|-------|-------------|---------|
| **Timestamp** | `[YYYYMMDD_HHMMSS]` | `[20260330_112151]` |
| **Method** | HTTP method | `GET`, `POST`, `OPTIONS` |
| **Path** | Request path | `/post/run`, `/get/schema` |
| **IP** | Client IP (respects `X-Forwarded-For`) | `127.0.0.1`, `::1` |
| **Status** | HTTP status code | `200`, `400`, `500` |
| **In** | Request body (POST only, truncated if > 200 bytes) | `{"cmd":"echo",...}` |
| **Out** | Response body (truncated if > 200 bytes) | `{"code":0,...}` or `[1024 bytes]` |

---

## Special Cases

- **Large payloads** (> 200 bytes): shown as `[N bytes]` or `[HTML N bytes]`
- **Client IP**: reads `X-Forwarded-For` header first (for reverse proxies), falls back to socket IP
- **CORS preflight** (OPTIONS): logged but body omitted
- **Console output**: always shown regardless of `--log` flag

---

## Error Code Reference in Logs

| Status | Meaning |
|--------|---------|
| `200` | Success |
| `400` | Bad request (unknown command, missing field, JSON parse error) |
| `403` | Authentication failed (invalid token) |
| `404` | Not found (missing `web/index.html`) |
| `413` | Request body too large |
| `500` | Internal server error |

---

## Implementation Details

- `core/logger.h` — Logger class with timestamp formatting
- Thread-safe: uses `std::mutex` for concurrent writes
- No external dependencies (C++ stdlib only)
- Works on Windows and Linux

---

## Log Rotation

The logger **appends** to the log file. For production log rotation:

```bash
# Manual rotation
mv server.log server.log.$(date +%Y%m%d_%H%M%S)

# logrotate config (/etc/logrotate.d/cpp_srv)
/path/to/server.log {
    daily
    rotate 7
    compress
    missingok
    notifempty
}
```

---

## Monitoring Examples

```bash
# Watch log in real-time
tail -f server.log

# Filter for errors only
tail -f server.log | grep "Status=4\|Status=5"

# Filter for specific command
tail -f server.log | grep "call_shell"

# Count requests by status
grep -c "Status=200" server.log
grep -c "Status=400" server.log
```

---

## Testing

```bash
./test_logging.sh
```

This starts a server with logging, makes various test requests (GET/POST, success/error, IPv4/IPv6), and displays the log output.
