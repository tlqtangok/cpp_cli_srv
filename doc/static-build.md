# Static Build Guide

This guide explains how to build portable static binaries that run on any machine without installing external dependencies.

---

## Why Static Builds?

| ✅ Benefit | ❌ Trade-off |
|-----------|------------|
| Copy binary to any machine, no deps needed | Larger binary size (libraries included) |
| No "library not found" errors | Cannot benefit from system security patches |
| Perfect for cloud servers and Docker | Must rebuild to update included library versions |
| Works in minimal containers (scratch, alpine) | |

---

## Quick Static Build

```bash
# Linux
./build.sh --static

# Windows
build.bat --static
```

---

## Linux Static Build Details

`./build.sh --static` configures CMake with `-DSTATIC_BUILD=ON`, which:
- Links C++ standard library statically (`-static-libgcc -static-libstdc++`)
- Prefers `.a` static libraries over `.so` shared libraries

**Example output:**
```
=== Building cpp_cli_srv on Linux (STATIC) ===
...
Static linking check:
  • cpp_srv dynamic dependencies:
    libssl.so.3 => /lib/x86_64-linux-gnu/libssl.so.3
    libc.so.6  => /lib/x86_64-linux-gnu/libc.so.6
  ✓ cpp_cli is fully static
```

> **Note**: `libssl` and `libc` may still appear as dynamic. See "Full Static Build" below to eliminate all deps.

---

## Windows Static Build Details

`build.bat --static` compiles with `/MT` (static C++ runtime) instead of `/MD` (dynamic).

**Result**: binary includes C++ runtime — no `VCRUNTIME140.dll` or `MSVCP140.dll` required on target.

---

## Full Static Build (Linux)

For completely static binaries with zero external dependencies:

### Option 1: Static OpenSSL from Source

```bash
# Build static OpenSSL
wget https://www.openssl.org/source/openssl-3.0.13.tar.gz
tar -xzf openssl-3.0.13.tar.gz
cd openssl-3.0.13
./config no-shared --prefix=/usr/local/openssl-static
make -j$(nproc)
sudo make install

# Build with static OpenSSL
rm -rf build
mkdir -p build && cd build
cmake .. \
  -DSTATIC_BUILD=ON \
  -DOPENSSL_ROOT_DIR=/usr/local/openssl-static \
  -DOPENSSL_USE_STATIC_LIBS=TRUE
make -j4
```

### Option 2: Force Static System OpenSSL

```bash
# Temporarily hide .so to force .a selection
sudo mv /usr/lib/x86_64-linux-gnu/libssl.so /tmp/
sudo mv /usr/lib/x86_64-linux-gnu/libcrypto.so /tmp/

./build.sh --static

# Restore
sudo mv /tmp/libssl.so /usr/lib/x86_64-linux-gnu/
sudo mv /tmp/libcrypto.so /usr/lib/x86_64-linux-gnu/
```

### Option 3: Musl (Truly Static Alternative)

Using musl libc instead of glibc produces binaries with zero dependencies:

```bash
sudo apt install musl-tools

rm -rf build
mkdir -p build && cd build
CC=musl-gcc CXX=musl-g++ cmake .. -DSTATIC_BUILD=ON
make -j4

# Verify
ldd build/cpp_srv
# Output: "not a dynamic executable"
```

---

## Verify Static Build

**Linux:**
```bash
ldd build/cpp_srv           # Shows remaining dynamic deps
ls -lh build/cpp_srv        # Static binaries are larger

# Common acceptable dynamic deps:
# linux-vdso.so  — virtual (always present, not a real file)
# ld-linux.so    — dynamic linker (needed unless musl)
# libc.so        — glibc (static-linking glibc is problematic; use musl to eliminate)
# libpthread.so  — usually acceptable to keep dynamic
```

**Windows:**
```bat
dumpbin /dependents build\cpp_srv.exe
REM Static: shows only KERNEL32.dll, etc.
```

---

## Binary Sizes

| Build | Size |
|-------|------|
| Standard dynamic | ~300 KB |
| Static C++ stdlib | ~3–5 MB |
| Full static (with OpenSSL) | ~5–10 MB |

**Reduce size:**
```bash
strip build/cpp_srv        # Remove debug symbols
upx --best build/cpp_srv   # Optional UPX compression
```

---

## Cloud / Docker Deployment

```bash
./build.sh --static

# Minimal Docker image (only works with fully static binary)
cat > Dockerfile << 'EOF'
FROM scratch
COPY build/cpp_srv /cpp_srv
COPY build/web /web
EXPOSE 8080
CMD ["/cpp_srv", "--port", "8080"]
EOF

docker build -t cpp_cli_srv:static .
docker run -p 8080:8080 cpp_cli_srv:static
# Image size: ~2-3 MB!
```

---

## Troubleshooting

### "Cannot find -lstdc++"

```bash
sudo apt install libstdc++-10-dev
# Or link only GCC statically:
cmake .. -DSTATIC_BUILD=ON -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc"
```

### Static OpenSSL not picked up

```bash
cmake .. \
  -DSTATIC_BUILD=ON \
  -DOPENSSL_ROOT_DIR=/usr \
  -DOPENSSL_CRYPTO_LIBRARY=/usr/lib/x86_64-linux-gnu/libcrypto.a \
  -DOPENSSL_SSL_LIBRARY=/usr/lib/x86_64-linux-gnu/libssl.a
```

### Binary still has dependencies

Check which are truly dynamic:
```bash
ldd build/cpp_srv | grep "=>"
```
`linux-vdso.so` and `ld-linux.so` are virtual/loader — not avoidable without musl.

---

## Best Practices

| Scenario | Recommendation |
|----------|---------------|
| Development | Standard dynamic build (faster compile) |
| Cloud deployment | Static build for portability |
| Docker minimal image | Full static build (musl) |
| Windows distribution | Always static (`/MT`) |
