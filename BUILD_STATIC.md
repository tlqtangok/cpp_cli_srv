# Static Build Guide

This guide explains how to build portable static binaries that can run on any Linux/Windows system without external dependencies.

---

## Why Static Builds?

**Benefits:**
- ✅ **Portability**: Copy binary to any machine, no dependency installation needed
- ✅ **Cloud Deployment**: Upload to cloud servers without worrying about library versions
- ✅ **No Runtime Errors**: No "library not found" errors on target systems
- ✅ **Version Lock**: Binary includes exact library versions, avoiding compatibility issues

**Trade-offs:**
- Larger binary size (includes libraries)
- Cannot benefit from system security updates to shared libraries
- Must rebuild to update included library versions

---

## Linux/Ubuntu

### Standard Build (Dynamic Linking)
```bash
./build.sh
```

Dependencies check:
```bash
ldd build/cpp_srv
# Output: Shows dynamic library dependencies (libpthread, libssl, libc, etc.)
```

### Static Build (Portable)
```bash
./build.sh --static
```

The build script will:
1. Configure CMake with `-DSTATIC_BUILD=ON`
2. Link C++ standard library statically (`-static-libgcc -static-libstdc++`)
3. Prefer `.a` static libraries over `.so` shared libraries
4. Show dependency check after build

**Example output:**
```
=== Building cpp_cli_srv on Linux (STATIC) ===
Note: Static build creates portable binaries that work on any Linux system
Tip: Use './build.sh --static' for portable static binaries

...
Static linking check:
  • cpp_srv dynamic dependencies:
    libssl.so.3 => /lib/x86_64-linux-gnu/libssl.so.3
    libcrypto.so.3 => /lib/x86_64-linux-gnu/libcrypto.so.3
    libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0
    libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6
  ✓ cpp_cli is fully static
```

**Note**: Full static linking (including glibc and OpenSSL) requires additional steps below.

---

## Full Static Build (Linux)

For completely static binaries with no dependencies at all, you need static versions of all libraries.

### Install Static Libraries

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install libssl-dev

# For truly static OpenSSL, build from source:
wget https://www.openssl.org/source/openssl-3.0.13.tar.gz
tar -xzf openssl-3.0.13.tar.gz
cd openssl-3.0.13
./config no-shared --prefix=/usr/local/openssl-static
make -j$(nproc)
sudo make install
```

### Build with Static OpenSSL

```bash
# Clean previous build
rm -rf build

# Build with custom OpenSSL path
mkdir -p build
cd build
cmake .. \
  -DSTATIC_BUILD=ON \
  -DOPENSSL_ROOT_DIR=/usr/local/openssl-static \
  -DOPENSSL_USE_STATIC_LIBS=TRUE
make -j4
```

### Build with Musl (Fully Static Alternative)

Using musl libc instead of glibc produces truly static binaries:

```bash
# Install musl-gcc
sudo apt install musl-tools

# Build with musl
rm -rf build
mkdir -p build
cd build
CC=musl-gcc CXX=musl-g++ cmake .. -DSTATIC_BUILD=ON
make -j4
```

Verify:
```bash
ldd build/cpp_srv
# Output: "not a dynamic executable"
```

---

## Windows

### Standard Build (Dynamic Runtime)
```bat
build.bat
```

Uses `/MD` flag (dynamic C++ runtime - requires MSVCP140.dll, VCRUNTIME140.dll)

### Static Build (Portable)
```bat
build.bat --static
```

Uses `/MT` flag (static C++ runtime - no DLL dependencies)

**Advantages:**
- Binary runs on any Windows 10+ machine
- No "VCRUNTIME140.dll not found" errors
- No Visual C++ Redistributable installation needed

**Example:**
```
Building with static runtime for portability...

[1/2] Building CLI...
CLI OK
[2/2] Building Server...
Server OK

=== Build complete ===
Static runtime: Binaries include C++ runtime (/MT)
```

---

## Deployment

### Linux Deployment

**Standard build:**
```bash
# On build machine
./build.sh

# Copy to target machine (requires same distro/version)
scp build/cpp_srv user@target:/opt/cpp_cli_srv/
```

**Static build:**
```bash
# On build machine
./build.sh --static

# Copy to any Linux machine
scp build/cpp_srv user@target:/opt/cpp_cli_srv/
scp -r build/web user@target:/opt/cpp_cli_srv/

# On target machine (no installation needed!)
chmod +x /opt/cpp_cli_srv/cpp_srv
/opt/cpp_cli_srv/cpp_srv --port 8080
```

### Windows Deployment

**Standard build (requires redistributable):**
```bat
REM Users must install Visual C++ Redistributable first
REM Download: https://aka.ms/vs/17/release/vc_redist.x64.exe
```

**Static build (no dependencies):**
```bat
REM Build
build.bat --static

REM Copy to any Windows machine - works immediately
copy build\cpp_srv.exe \\target\share\
copy build\web\* \\target\share\web\
```

---

## Cloud Deployment Examples

### AWS EC2
```bash
# Build on your machine
./build.sh --static

# Upload to EC2 (any Amazon Linux, Ubuntu, etc.)
scp -i key.pem build/cpp_srv ec2-user@instance:/home/ec2-user/
scp -r -i key.pem build/web ec2-user@instance:/home/ec2-user/

# SSH and run
ssh -i key.pem ec2-user@instance
chmod +x cpp_srv
./cpp_srv --port 8080
```

### Docker (Minimal Image)
```dockerfile
# Dockerfile for static binary
FROM scratch
COPY build/cpp_srv /cpp_srv
COPY build/web /web
EXPOSE 8080
CMD ["/cpp_srv", "--port", "8080"]
```

Build:
```bash
# Build static binary on host
./build.sh --static

# Build Docker image (only 2-3 MB!)
docker build -t cpp_cli_srv:static .
docker run -p 8080:8080 cpp_cli_srv:static
```

### Google Cloud Run / Azure Container Apps
```bash
# Build
./build.sh --static

# Create minimal container
cat > Dockerfile << 'EOF'
FROM alpine:latest
RUN apk add --no-cache ca-certificates
COPY build/cpp_srv /usr/local/bin/
COPY build/web /usr/local/share/web
EXPOSE 8080
CMD ["cpp_srv", "--port", "8080"]
EOF

docker build -t gcr.io/project/cpp_srv .
docker push gcr.io/project/cpp_srv
```

---

## Verification

### Check Binary Dependencies

**Linux:**
```bash
# Check what libraries are linked
ldd build/cpp_srv

# Check binary size
ls -lh build/cpp_srv

# Static binaries are larger but self-contained
```

**Windows:**
```bat
REM Check dependencies with Dependency Walker or dumpbin
dumpbin /dependents build\cpp_srv.exe

REM Static builds show only system DLLs (KERNEL32.dll, etc.)
```

### Test Portability

**Create clean test environment:**
```bash
# Using Docker to simulate clean system
docker run -it --rm -v $(pwd)/build:/app alpine:latest /bin/sh

# Inside container
cd /app
./cpp_srv --help
# Should work without installing anything!
```

---

## Troubleshooting

### Issue: "Cannot find -lstdc++"

**Cause**: Static C++ library not available

**Solution**:
```bash
# Ubuntu/Debian
sudo apt install libstdc++-10-dev

# Or use dynamic linking for C++ stdlib
cmake .. -DSTATIC_BUILD=ON -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc"
```

### Issue: Static OpenSSL not working

**Cause**: OpenSSL shared libraries found instead of static

**Solution**:
```bash
# Remove shared OpenSSL libs temporarily
sudo mv /usr/lib/x86_64-linux-gnu/libssl.so /tmp/
sudo mv /usr/lib/x86_64-linux-gnu/libcrypto.so /tmp/

# Build
./build.sh --static

# Restore
sudo mv /tmp/libssl.so /usr/lib/x86_64-linux-gnu/
sudo mv /tmp/libcrypto.so /usr/lib/x86_64-linux-gnu/
```

**Or specify static library path:**
```bash
cmake .. \
  -DSTATIC_BUILD=ON \
  -DOPENSSL_ROOT_DIR=/usr \
  -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.a \
  -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.a
```

### Issue: Binary still has dependencies

**Check which libraries are still dynamic:**
```bash
ldd build/cpp_srv | grep "=>"
```

Common remaining dependencies:
- `linux-vdso.so` - Virtual dynamic shared object (always present, not a real file)
- `ld-linux.so` - Dynamic linker (needed unless using musl)
- `libc.so` - System C library (static linking glibc is problematic)
- `libpthread.so` - Threading (usually safe to keep dynamic)

These are typically acceptable even for "static" builds.

### Issue: Larger binary size

**This is expected:**
- Standard build: 200-500 KB
- Static build: 2-5 MB
- Full static (with OpenSSL): 5-10 MB

**If size is a concern:**
```bash
# Strip debug symbols
strip build/cpp_srv

# Use UPX compression (optional)
upx --best build/cpp_srv
```

---

## Best Practices

1. **Development**: Use standard dynamic builds for faster compilation
2. **Testing**: Use dynamic builds with system libraries
3. **Production/Cloud**: Use static builds for portability
4. **Docker**: Use static builds with minimal base images
5. **Windows**: Always use static builds (`/MT`) for distribution

---

## Build Comparison

| Aspect | Standard Build | Static Build |
|--------|---------------|--------------|
| **Build Command** | `./build.sh` | `./build.sh --static` |
| **Binary Size** | ~300 KB | ~3-5 MB |
| **Portability** | Same OS/distro | Any Linux system |
| **Dependencies** | System libraries | Self-contained |
| **Build Time** | Faster | Slightly slower |
| **Updates** | Get system updates | Must rebuild |
| **Use Case** | Development, system package | Cloud, Docker, portable |

---

## Quick Reference

```bash
# Linux - Standard
./build.sh

# Linux - Static (portable)
./build.sh --static

# Windows - Standard
build.bat

# Windows - Static (portable)
build.bat --static

# Check dependencies (Linux)
ldd build/cpp_srv

# Check dependencies (Windows)
dumpbin /dependents build\cpp_srv.exe

# Clean rebuild
rm -rf build
./build.sh --static
```

For most cloud deployment scenarios, **static builds** are recommended for maximum portability and minimal setup on target machines.
