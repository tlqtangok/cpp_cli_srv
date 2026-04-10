# Building on Windows

## Prerequisites

- **Compiler**: MSVC (`cl.exe`) via Visual Studio 2022, x64 Native Tools
- **C++ Standard**: C++17
- **Dependencies**: None (header-only libs in `third_party/`)

Install Visual Studio 2022 with the **C++ Desktop Development** workload.

> **Note:** `build.bat` has the `vcvars64.bat` path hardcoded:
> `D:\jd\pro\vs2022\Packages\Community\VC\Auxiliary\Build\vcvars64.bat`
> Update this line if Visual Studio is installed elsewhere.

---

## Quick Build

```bat
build.bat
```

Produces:
- `build\cpp_cli.exe` — CLI tool
- `build\cpp_srv.exe` — HTTP server
- `build\web\index.html` — Web GUI copy

---

## Standard vs Static Build

| Type | Command | Runtime | Portability |
|------|---------|---------|-------------|
| **Standard** | `build.bat` | Dynamic (`/MD`) — requires MSVCP140.dll | Same machine |
| **Static** | `build.bat --static` | Static (`/MT`) — no DLL dependencies | Any Windows 10+ |

For distribution, always use the static build to avoid *"VCRUNTIME140.dll not found"* errors.

```bat
build.bat --static
```

---

## Debug Build

For verbose compiler output:

```bat
build_debug.bat
```

---

## Clean Rebuild

```bat
rmdir /s /q build
build.bat
```

---

## Running the CLI

```bat
build\cpp_cli.exe --help
build\cpp_cli.exe --schema
build\cpp_cli.exe -d "{\"cmd\":\"echo\",\"args\":{\"text\":\"hello\"}}"
build\cpp_cli.exe --cmd add --args "{\"a\":3,\"b\":4}" --human
```

> **Tip:** Shell quoting on Windows (cmd.exe) requires `\"` for embedded quotes. Use `.bat` test files to avoid quoting issues. PowerShell has its own escaping rules.

---

## Running the Server

```bat
build\cpp_srv.exe                              REM Default port 8080
build\cpp_srv.exe --port 9090                  REM Custom HTTP port
build\cpp_srv.exe --port 8080 --threads 8      REM Custom thread count
build\cpp_srv.exe --log server.log             REM File logging
build\cpp_srv.exe --token mytoken123           REM Token for call_shell/global JSON auth
build\cpp_srv.exe --no-ipv6                    REM IPv4 only
```

---

## Deployment

**Static build (no dependencies):**
```bat
REM Build
build.bat --static

REM Copy to any Windows machine
copy build\cpp_cli.exe \\target\share\
copy build\cpp_srv.exe \\target\share\
xcopy /e build\web \\target\share\web\
```

**Standard build (requires redistributable):**
```bat
REM Users must install Visual C++ Redistributable first
REM Download: https://aka.ms/vs/17/release/vc_redist.x64.exe
```

---

## Check Binary Dependencies

```bat
REM Shows imported DLLs
dumpbin /dependents build\cpp_srv.exe

REM Static builds show only system DLLs (KERNEL32.dll, etc.)
```

---

## Test Scripts

| Test | Script |
|------|--------|
| CLI smoke tests | `test_cli.bat` |
| Server API tests | `test_server.bat` (start server first) |
| Shell command tests | `test_shell.bat` |
| Token auth tests | `test_token.bat` |
| JSON file I/O tests | `test_json.bat` |
| Global JSON tests | `test_global_json.bat` |
| IPv6 dual-stack tests | `test_ipv6.bat` |

> **IPv6 note:** `test_ipv6.bat` uses `curl --noproxy "*"` to bypass system HTTP proxy for localhost IPv6 connections.

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `cl.exe` not found | Run from *x64 Native Tools Command Prompt for VS 2022* |
| `vcvars64.bat` path wrong | Update the path in `build.bat` / `build_debug.bat` |
| Port in use | Use `--port <other>` |
| C4068 / C4819 warnings | Already suppressed via `/wd4068 /wd4819` in `build.bat` |
| Escaped quote mess | Use `.bat` test files; avoid embedding JSON in cmd.exe directly |

---

## Build Comparison

| Aspect | Standard (`build.bat`) | Static (`build.bat --static`) |
|--------|------------------------|-------------------------------|
| **Binary size** | ~300 KB | ~3–5 MB |
| **Runtime** | Requires MSVCP140.dll | Self-contained |
| **Use case** | Development | Distribution / cloud |
