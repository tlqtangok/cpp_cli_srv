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
  -d '{"cmd":"call_shell","args":{"command":"ls"}}'
# Response: {"code":6,"error":"call_shell requires valid token","output":""}

# With token - ACCEPTED
curl -X POST http://localhost:8080/post/run \
  -H "Content-Type: application/json" \
  -d '{"cmd":"call_shell","args":{"command":"ls"},"token":"mytoken123"}'
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
