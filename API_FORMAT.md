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

### Success Response

```json
{
  "code": 0,
  "output": "hello",
  "error": ""
}
```

### Error Response

```json
{
  "code": 1,
  "error": "unknown command: foo",
  "output": ""
}
```

### Complex Output (JSON in output field)

For commands like `call_shell` that return structured data:

```json
{
  "code": 0,
  "output": "{\"command\":\"ls\",\"exit_code\":0,\"stdout\":\"file1\\nfile2\\n\"}",
  "error": ""
}
```

Parse the `output` field as JSON to access structured data.

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
