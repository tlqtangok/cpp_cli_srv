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
