# HTTP API Reference

All responses use a unified JSON envelope:

```json
{ "code": 0, "output": <any>, "error": "" }
```

- `code`: 0 = success, non-zero = error (see [Error Codes](#error-codes))
- `output`: result data (string, number, object, array, boolean, or `""` on error)
- `error`: human-readable error message when `code != 0`

---

## Endpoints

### Schema & Info

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/get/schema` | GET | No | All available commands with argument definitions |
| `/get/version` | GET | No | Build version (`{"version":"20260410-abc1234"}`) |
| `/get/status` | GET | No | Server health: active requests, thread count |

### Command Execution

| Endpoint | Method | Auth | Description |
|----------|--------|------|-------------|
| `/post/run` | POST | Token (for authenticated cmds) | Execute any registered command |

### Static Files

| Endpoint | Method | Description |
|----------|--------|-------------|
| `/` | GET | Serve web GUI (`web/index.html`) |
| `/*` | OPTIONS | CORS preflight (204 No Content) |

---

## POST /post/run

**Request body:**
```json
{
  "cmd": "command_name",
  "args": { "key": "value", ... },
  "timeout_ms": 5000
}
```

- `cmd` — command name (see `/get/schema` for list)
- `args` — command arguments as JSON object
- `timeout_ms` — optional, default 30 000 ms (only meaningful for async commands)
- For authenticated commands, include `"token": "yourtoken"` inside `args`

**Success response (HTTP 200):**
```json
{ "code": 0, "output": "result", "error": "" }
```

**Error response (HTTP 400):**
```json
{ "code": 3, "output": "", "error": "unknown command: foo" }
```

---

## Built-in Commands

| Command | Auth | Description |
|---------|------|-------------|
| `echo` | No | Return text as-is |
| `add` | No | Add two numbers (`a`, `b`) |
| `upper` | No | Convert text to uppercase |
| `slow_task` | No | Simulate slow async operation (`ms` param) |
| `call_shell` | Token | Execute shell command (`command` param) |
| `write_json` | No | Write JSON to file (`path`, `json_content`) |
| `read_json` | No | Read JSON from file (`path`) |
| `get_global_json` | Token | Get global JSON (entire or by `path`) |
| `set_global_json` | Token | Replace entire global JSON (`value`) |
| `patch_global_json` | Token | Apply RFC 7386 merge patch |
| `persist_global_json` | No | Force save global JSON to disk |

---

## curl Examples

```bash
# Get schema
curl http://localhost:8080/get/schema

# Get version
curl http://localhost:8080/get/version

# Get status
curl http://localhost:8080/get/status

# Echo
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"echo","args":{"text":"hello"}}'

# Add
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"add","args":{"a":10,"b":32}}'

# call_shell (requires token)
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"call_shell","args":{"command":"ls -la","token":"mytoken"}}'

# Get global JSON (entire)
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"get_global_json","args":{"token":"mytoken"}}'

# Get global JSON at path
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"get_global_json","args":{"path":"/user/name","token":"mytoken"}}'

# Patch global JSON
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"patch_global_json","args":{"price":99.99,"on_sale":true,"token":"mytoken"}}'

# Set global JSON
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"set_global_json","args":{"value":{"name":"Alice"},"token":"mytoken"}}'

# With custom timeout
curl -X POST http://localhost:8080/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"slow_task","args":{"ms":2000},"timeout_ms":5000}'
```

---

## Error Codes

| Code | Meaning | HTTP Status |
|------|---------|-------------|
| 0 | Success | 200 |
| 1 | Invalid argument / path not found | 400 / 404 |
| 2 | Command timeout | 400 |
| 3 | Exception during execution | 500 |
| 4 | JSON parse error | 400 |
| 5 | Server error | 500 |
| 6 | Authentication failed (invalid / missing token) | 403 |

---

## Reverse Proxy / Sub-path Deployment

When served behind a reverse proxy at a sub-path (e.g., Apache `ProxyPassReverse`):

```
http://localhost:10248/cpp_cli_srv/
```

The web GUI automatically detects the sub-path from `window.location.pathname` and adjusts all API calls accordingly. No manual configuration needed — `fetch('/cpp_cli_srv/post/run')` is used automatically.

Example curl command for sub-path deployment:
```bash
curl -X POST http://localhost:10248/cpp_cli_srv/post/run \
  -H 'Content-Type: application/json' \
  -d '{"cmd":"echo","args":{"text":"hello"}}'
```
