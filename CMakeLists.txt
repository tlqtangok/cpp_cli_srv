cmake_minimum_required(VERSION 3.14)
project(cpp_cli_srv CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Release flags ──────────────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -O2)

# ── httplib needs these on Linux ───────────────────────────────────────────────
find_package(Threads REQUIRED)
find_package(OpenSSL QUIET)

# ── Include paths ──────────────────────────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}/core
    ${CMAKE_SOURCE_DIR}/third_party
)

# ── Server binary ──────────────────────────────────────────────────────────────
add_executable(cpp_srv server/main.cpp)
target_link_libraries(cpp_srv PRIVATE Threads::Threads)

# Copy web assets next to the binary so relative path works
add_custom_command(TARGET cpp_srv POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/web
        $<TARGET_FILE_DIR:cpp_srv>/web
    COMMENT "Copying web/ assets alongside binary"
)

# ── CLI binary ─────────────────────────────────────────────────────────────────
add_executable(cpp_cli cli/main.cpp)
target_link_libraries(cpp_cli PRIVATE Threads::Threads)

# ── Install (optional) ─────────────────────────────────────────────────────────
install(TARGETS cpp_srv cpp_cli DESTINATION bin)
install(DIRECTORY web/ DESTINATION share/cpp_cli_srv/web)
