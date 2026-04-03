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
