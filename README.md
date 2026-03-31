# cpp-ai-review-tool
C++23 tool for AI assisted review

## Requirements

- C++23-capable compiler (GCC 13+, Clang 16+)
- CMake 3.22+
- OpenSSL (`libssl-dev` on Debian/Ubuntu)
- nlohmann/json (`nlohmann-json3-dev` on Debian/Ubuntu; fetched via CMake if not found)
- cpp-httplib, googletest — fetched automatically by CMake

## Build

```bash
cmake -S . -B build
cmake --build build
# binary: build/cpp-review
```

## Tests

Unit tests are built automatically. Integration tests require `ANTHROPIC_API_KEY` and must be enabled explicitly.

```bash
# Run unit tests
ctest --test-dir build --output-on-failure

# Build and run integration tests
cmake -S . -B build -DINTEGRATION_TESTS=ON
cmake --build build
ANTHROPIC_API_KEY=<key> ctest --test-dir build --output-on-failure
```

## CI

A GitHub Actions workflow (`.github/workflows/pr.yml`) runs on every pull request targeting `main`:
- **Clang-Format** — enforces formatting with `clang-format-15`
- **Build & Test** — configures with `g++-13`, builds, and runs unit tests via CTest

## Project Structure

```
.
├── .clang-format
├── .clangd
├── CMakeLists.txt
├── .github/
│   └── workflows/
│       └── pr.yml              # PR check: formatting + build + test
├── include/
│   └── llm/
│       ├── llm_client.hpp          # Abstract LLM client interface
│       ├── llm_request.hpp
│       ├── llm_response.hpp
│       ├── llm_error.hpp
│       ├── http_client.hpp         # Abstract HTTP client interface
│       ├── httplib_http_client.hpp # cpp-httplib HTTPS implementation
│       ├── claude_llm_client.hpp   # Anthropic Claude implementation
│       └── rate_limited_llm_client.hpp
├── src/
│   ├── main.cpp
│   └── llm/
│       ├── claude_llm_client.cpp
│       ├── httplib_http_client.cpp
│       └── rate_limited_llm_client.cpp
├── tests/
│   ├── unit/
│   │   ├── test_claude_llm_client.cpp
│   │   └── test_make_claude_client.cpp
│   └── integration/
│       └── test_claude_llm_client_integration.cpp
└── docs/
```
