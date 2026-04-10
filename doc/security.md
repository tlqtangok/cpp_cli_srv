# Security Guide

## Token Authentication

The server supports token-based authentication for sensitive commands (`call_shell`, `set_global_json`, `patch_global_json`, `get_global_json`).

### How It Works

- Start the server with `--token <value>`
- Token must be ≥2 characters, alphanumeric + underscore only (`[a-zA-Z0-9_]+`)
- Authenticated commands require the token in the `args` JSON: `"token": "yourtoken"`
- **CLI**: token not required (trusted local execution)
- **Web/API**: token required for authenticated commands

### Access Model

| Access Method | Token Required? | Reason |
|---------------|-----------------|--------|
| **Web / HTTP API** | ✅ Yes | Untrusted remote access |
| **CLI (`cpp_cli`)** | ❌ No | Trusted local execution |

### Starting Server with Token

```bash
# Linux
./build/cpp_srv --token mytoken123

# Windows
build\cpp_srv.exe --token mytoken123
```

**Without `--token`**: `call_shell` and `set_global_json` are disabled for web access (return code 6).

### API Usage

```bash
# Without token — REJECTED (code 6)
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"ls"}}'
# Response: {"code":6,"error":"call_shell requires valid token","output":""}

# With token — ACCEPTED
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"ls","token":"mytoken123"}}'
```

### Web GUI Behavior

- On first use of an authenticated command, the GUI prompts for the token
- Token is cached in `localStorage` for convenience
- If token is rejected (code 6), cache is cleared and GUI prompts again

### Generate a Strong Token

```bash
# Linux/Mac — cryptographically secure
openssl rand -hex 16
# Example output: e4f7c9a2b5d8f1e3c6a9b2d5f8e1c4a7

# Windows PowerShell
[Convert]::ToBase64String((1..24 | ForEach-Object { Get-Random -Maximum 256 }))
```

---

## Production Checklist

- [ ] Use a strong, randomly-generated token (see above)
- [ ] **Never commit the token to source control**
- [ ] Run server with restricted user permissions
- [ ] Use firewall / network isolation
- [ ] Enable logging with `--log`
- [ ] Use HTTPS in production (see [https-setup.md](https-setup.md))
- [ ] Consider a reverse proxy (nginx/Apache) for additional auth layers
- [ ] Rotate tokens periodically
- [ ] Monitor logs for suspicious activity

---

## Threat Model

**Protected against:**
- Unauthorized web access to shell commands
- Casual / accidental command execution
- Basic security scanning tools

**NOT protected against:**
- Token leaked in logs or error messages
- Man-in-the-middle attacks (use HTTPS to mitigate)
- Compromised token (restart server with new `--token` to revoke)
- Physical server access

**CORS**: Currently wide open (`Access-Control-Allow-Origin: *`). Fine for local dev; restrict in production via a reverse proxy.

---

## Permission Denied Errors (Linux)

### Problem

When copying the project to another machine:

```
make[2]: stat: /path/to/cpp_cli_srv/cli/main.cpp: Permission denied
```

### Quick Fix

```bash
cd /path/to/cpp_cli_srv
./fix_permissions.sh
./build.sh
```

### Manual Fix

```bash
find . -type d -exec chmod 755 {} \;   # directories: rwxr-xr-x
find . -type f -exec chmod 644 {} \;   # files: rw-r--r--
chmod +x *.sh                          # scripts: executable
./build.sh
```

### How to Copy the Project Correctly

```bash
# Method 1: cp preserving permissions
cp -rp /source/cpp_cli_srv /destination/

# Method 2: tar (recommended for transfers)
tar czf cpp_cli_srv.tar.gz cpp_cli_srv/
# On target:
tar xzf cpp_cli_srv.tar.gz

# Method 3: rsync
rsync -av /source/cpp_cli_srv/ user@remote:/destination/

# Method 4: Git (best — automatically preserves permissions)
git clone <repository-url>
```

### Verify Permissions

```bash
ls -la cli/main.cpp    # Should show: -rw-r--r--
ls -la build.sh        # Should show: -rwxr-xr-x
ls -la core/           # Should show: drwxr-xr-x
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
