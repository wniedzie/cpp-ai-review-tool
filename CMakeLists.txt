cmake_minimum_required(VERSION 3.22)
project(cpp-ai-review-tool LANGUAGES CXX)

# C++23 requires GCC 13+ or Clang 16+. On Ubuntu 22.04, install via:
#   sudo add-apt-repository ppa:ubuntu-toolchain-r/test && sudo apt install g++-13
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13")
    message(FATAL_ERROR
        "GCC 13+ is required for full C++23 support (std::format, std::expected).\n"
        "Current version: ${CMAKE_CXX_COMPILER_VERSION}.\n"
        "Install with: sudo add-apt-repository ppa:ubuntu-toolchain-r/test && sudo apt install g++-13\n"
        "Then reconfigure with: -DCMAKE_CXX_COMPILER=g++-13"
    )
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ── Standalone Asio ─────────────────────────────────────────────────────────
# Prefer the system package (libasio-dev) when available.
# If not installed, FetchContent falls back to a git clone.
find_path(ASIO_INCLUDE_DIR asio.hpp
    PATHS /usr/include /usr/local/include
)
if(ASIO_INCLUDE_DIR)
    message(STATUS "Using system Asio at ${ASIO_INCLUDE_DIR}")
else()
    FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG        asio-1-38-0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(asio)
    set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include")
endif()

# ── nlohmann/json ─────────────────────────────────────────────────────────────
# Prefer the system package (nlohmann-json3-dev) when available.
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ── OpenSSL (required for Asio SSL/TLS support) ───────────────────────────────
find_package(OpenSSL REQUIRED)

# ── Threads (required by Asio on Linux / macOS) ───────────────────────────────
find_package(Threads REQUIRED)

# ── GTest / GMock ─────────────────────────────────────────────────────────────
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
    GIT_SHALLOW    TRUE
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ── Core library ──────────────────────────────────────────────────────────────
add_library(cpp_review_lib STATIC
    src/llm/rate_limited_llm_client.cpp
    src/llm/claude_llm_client.cpp
)

target_include_directories(cpp_review_lib PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

# Expose Asio headers as SYSTEM to suppress third-party warnings
target_include_directories(cpp_review_lib SYSTEM PRIVATE
    ${ASIO_INCLUDE_DIR}
)

target_compile_definitions(cpp_review_lib PRIVATE
    ASIO_STANDALONE
    ASIO_NO_DEPRECATED
)

target_link_libraries(cpp_review_lib PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# ── Main executable ───────────────────────────────────────────────────────────
add_executable(cpp-review src/main.cpp)
target_link_libraries(cpp-review PRIVATE cpp_review_lib)

# ── Tests ─────────────────────────────────────────────────────────────────────
enable_testing()
# add_subdirectory(tests)  # uncomment when tests/ is added
