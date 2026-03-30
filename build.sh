#!/bin/bash
# Build script for Linux/Ubuntu

set -e  # Exit on error

echo "=== Building cpp_cli_srv on Linux ==="

# Create build directory
mkdir -p build
cd build

# Run CMake and build
cmake ..
make -j4

echo ""
echo "=== Build complete ==="
echo "Binaries:"
echo "  - build/cpp_srv (server)"
echo "  - build/cpp_cli (CLI)"
echo ""
echo "Run CLI: ./build/cpp_cli --schema"
echo "Run Server: ./build/cpp_srv"
