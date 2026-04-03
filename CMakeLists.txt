cmake_minimum_required(VERSION 3.14)
project(cpp_cli_srv CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Release flags ──────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -O2)

# ── Static linking option ──────────────────────────────────────────────────────
option(STATIC_BUILD "Build with static linking for portability" OFF)

if(STATIC_BUILD)
    message(STATUS "Static build enabled - binaries will be portable")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(BUILD_SHARED_LIBS OFF)
endif()

# ── httplib needs these on Linux ───────────────────────────────────────────────
find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

# Enable OpenSSL support if available
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL found - HTTPS support enabled")
    add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)
else()
    message(WARNING "OpenSSL not found - HTTPS support disabled")
    message(WARNING "")
    message(WARNING "To enable HTTPS support, install OpenSSL development libraries:")
    message(WARNING "  Ubuntu/Debian: sudo apt update && sudo apt install libssl-dev")
    message(WARNING "  CentOS/RHEL:   sudo yum install openssl-devel")
    message(WARNING "  Fedora:        sudo dnf install openssl-devel")
    message(WARNING "  Arch Linux:    sudo pacman -S openssl")
    message(WARNING "")
    message(WARNING "After installing, re-run: rm -rf build && ./build.sh")
    message(WARNING "")
endif()

# ── Include paths ──────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

# ── Server binary ──────────────────────────────────────────────────────────────
add_executable(cpp_srv server/main.cpp)

if(STATIC_BUILD)
    # For static build, link pthread statically and prefer static libraries
    target_link_libraries(cpp_srv PRIVATE -pthread)
    if(OpenSSL_FOUND)
        # Try to link OpenSSL statically
        target_link_libraries(cpp_srv PRIVATE OpenSSL::SSL OpenSSL::Crypto -ldl)
    endif()
else()
    target_link_libraries(cpp_srv PRIVATE Threads::Threads)
    if(OpenSSL_FOUND)
        target_link_libraries(cpp_srv PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
endif()

# Note: web/ directory is used directly from source, not copied to build
# Server loads web/index.html using relative path from executable directory
# When running: ./build/cpp_srv, the working directory is the repo root
# so web/index.html is accessible

# ── CLI binary ─────────────────────────────────────────────────────────────────
add_executable(cpp_cli cli/main.cpp)

if(STATIC_BUILD)
    target_link_libraries(cpp_cli PRIVATE -pthread)
else()
    target_link_libraries(cpp_cli PRIVATE Threads::Threads)
endif()

# ── Install (optional) ─────────────────────────────────────────────────────────
install(TARGETS cpp_srv cpp_cli DESTINATION bin)
install(DIRECTORY web/ DESTINATION share/cpp_cli_srv/web)
