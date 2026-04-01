#!/bin/bash
# Build script for Linux/Ubuntu

set -e  # Exit on error

# Parse command line arguments
STATIC_BUILD=""
if [[ "$1" == "--static" ]]; then
    STATIC_BUILD="-DSTATIC_BUILD=ON"
    echo "=== Building cpp_cli_srv on Linux (STATIC) ==="
    echo "Note: Static build creates portable binaries that work on any Linux system"
else
    STATIC_BUILD="-DSTATIC_BUILD=OFF"
    echo "=== Building cpp_cli_srv on Linux ==="
    echo "Tip: Use './build.sh --static' for portable static binaries"
fi

echo ""

# Create build directory
mkdir -p build
cd build

# Run CMake and build
cmake .. $STATIC_BUILD
# cmake .. -DOPENSSL_ROOT_DIR=/usr/local/openssl-3.0 -DOPENSSL_LIBRARIES=/usr/local/openssl-3.0/lib
make -j4

echo ""
echo "=== Build complete ==="
echo "Binaries:"
echo "  - $(pwd)/cpp_srv (server)"
echo "  - $(pwd)/cpp_cli (CLI)"
echo ""
echo "Full paths:"
echo "  Server: $(realpath cpp_srv)"
echo "  CLI:    $(realpath cpp_cli)"
echo ""

# Check if binaries are static
if [[ "$STATIC_BUILD" != "" ]]; then
    echo "Static linking check:"
    if ldd cpp_srv 2>&1 | grep -q "not a dynamic executable"; then
        echo "  ✓ cpp_srv is fully static"
    else
        echo "  • cpp_srv dynamic dependencies:"
        ldd cpp_srv | grep -v "linux-vdso\|ld-linux" | sed 's/^/    /'
    fi
    if ldd cpp_cli 2>&1 | grep -q "not a dynamic executable"; then
        echo "  ✓ cpp_cli is fully static"
    else
        echo "  • cpp_cli dynamic dependencies:"
        ldd cpp_cli | grep -v "linux-vdso\|ld-linux" | sed 's/^/    /'
    fi
    echo ""
fi

echo "Run CLI: ./build/cpp_cli --help"
echo "Run Server: ./build/cpp_srv --help"
