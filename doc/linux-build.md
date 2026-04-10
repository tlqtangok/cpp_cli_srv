# Building on Linux / Ubuntu

## Prerequisites

- **Compiler**: g++ 7+ with C++17 support
- **Build system**: CMake 3.14+, make
- **System libraries**: pthread (usually included)
- **Optional**: OpenSSL dev libraries (for HTTPS support)

```bash
# Ubuntu/Debian — minimum
sudo apt update && sudo apt install build-essential cmake

# Ubuntu/Debian — with HTTPS support
sudo apt update && sudo apt install build-essential cmake libssl-dev

# CentOS/RHEL — with HTTPS
sudo yum install gcc-c++ cmake openssl-devel

# Fedora — with HTTPS
sudo dnf install gcc-c++ cmake openssl-devel

# Arch Linux — with HTTPS
sudo pacman -S base-devel cmake openssl
```

---

## Quick Build

```bash
./build.sh
```

This will:
1. Create the `build/` directory
2. Run CMake to generate Makefiles
3. Compile both binaries with `make -j4`
4. Copy `web/` assets to `build/web/`

Produces:
- `build/cpp_srv` — HTTP server
- `build/cpp_cli` — CLI tool
- `build/web/` — Web GUI

---

## Static Build (Portable)

For cloud deployment or running on systems without installing dependencies:

```bash
./build.sh --static
```

The build script configures CMake with `-DSTATIC_BUILD=ON`, linking the C++ standard library statically. See [static-build.md](static-build.md) for advanced options (fully static OpenSSL, musl, etc.).

---

## Manual Build

```bash
mkdir -p build
cd build
cmake ..
make -j4
```

---

## Debug Build

```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

---

## Clean Rebuild

```bash
rm -rf build
./build.sh
```

Or without full clean:
```bash
cd build
make clean && make -j4
```

---

## Running the CLI

```bash
./build/cpp_cli --help
./build/cpp_cli --schema
./build/cpp_cli -d '{"cmd":"echo","args":{"text":"hello"}}'
./build/cpp_cli --cmd add --args '{"a":3,"b":4}' --human
./build/cpp_cli --version
```

---

## Running the Server

```bash
./build/cpp_srv                                    # Default port 8080
./build/cpp_srv --port 9090                        # Custom HTTP port
./build/cpp_srv --port 8080 --threads 8            # Custom thread count
./build/cpp_srv --log server.log                   # File logging
./build/cpp_srv --token mytoken123                 # Token for call_shell/global JSON auth
./build/cpp_srv --no-ipv6                          # IPv4 only
./build/cpp_srv --port_https 8443 --ssl ./ssl      # Enable HTTPS
```

---

## Deployment

**Static build:**
```bash
./build.sh --static

# Copy to any Linux machine (no deps needed)
scp build/cpp_srv user@target:/opt/cpp_cli_srv/
scp -r build/web user@target:/opt/cpp_cli_srv/

# On target machine
chmod +x /opt/cpp_cli_srv/cpp_srv
/opt/cpp_cli_srv/cpp_srv --port 8080
```

**Cloud / Docker:**
```bash
# Static build + minimal Docker image
./build.sh --static

cat > Dockerfile << 'EOF'
FROM scratch
COPY build/cpp_srv /cpp_srv
COPY build/web /web
EXPOSE 8080
CMD ["/cpp_srv", "--port", "8080"]
EOF

docker build -t cpp_cli_srv:static .
docker run -p 8080:8080 cpp_cli_srv:static
```

**Systemd service:**
```ini
[Unit]
Description=cpp_cli_srv
After=network.target

[Service]
Type=simple
User=youruser
WorkingDirectory=/opt/cpp_cli_srv
ExecStart=/opt/cpp_cli_srv/cpp_srv --port 8080 --log /var/log/cpp_srv.log --token yourtoken
Restart=on-failure
RestartSec=5s

[Install]
WantedBy=multi-user.target
```

---

## Check Binary Dependencies

```bash
# Dynamic build — shows shared library deps
ldd build/cpp_srv

# Static build — few or no deps
ldd build/cpp_cli   # should show "statically linked"
```

---

## Test Scripts

| Test | Script |
|------|--------|
| CLI smoke tests | `./test_cli.sh` |
| Server API tests | `./test_server.sh` (start server first) |
| Shell command tests | `./test_shell.sh` |
| Token auth tests | `./test_token.sh` |
| JSON file I/O tests | `./test_json.sh` |
| Global JSON tests | `./test_global_json.sh` |
| Concurrency + timeout | `./test_comprehensive.sh` |
| HTTPS tests | `./test_https.sh` |
| Logging tests | `./test_logging.sh` |

> **Note:** `test_server.sh` auto-detects the port from the running process — start server on any port and tests will find it.

---

## Differences from Windows Build

| Aspect | Linux | Windows |
|--------|-------|---------|
| Binary names | `cpp_srv`, `cpp_cli` | `cpp_srv.exe`, `cpp_cli.exe` |
| Build system | CMake + make | Direct `cl.exe` via batch |
| Compiler | g++ `-O2 -Wall -Wextra` | MSVC `/O2 /std:c++17 /EHsc` |
| System libs | pthread (auto-linked) | ws2_32.lib (Winsock) |
| Scripts | `.sh` files | `.bat` files |
| Shell quoting | Single quotes for JSON | Escaped `\"` in cmd.exe |

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Missing compiler | `sudo apt install build-essential cmake` |
| pthread errors | `sudo apt install libpthread-stubs0-dev` |
| Port in use | `./build/cpp_srv --port 9090` |
| IPv6 warnings | Use `--no-ipv6` or ignore (falls back to IPv4) |
| Permission denied on build | Run `./fix_permissions.sh`, then rebuild |
| Server can't find `web/index.html` | Run server from `build/` directory |
| OpenSSL not found | `sudo apt install libssl-dev`, then `rm -rf build && ./build.sh` |
