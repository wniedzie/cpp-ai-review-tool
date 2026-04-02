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

To enable clang-tidy static analysis during the build (requires `clang-tidy-16+`):

```bash
cmake -S . -B build -DENABLE_CLANG_TIDY=ON -DCMAKE_CXX_COMPILER=clang++-18
cmake --build build
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

Two GitHub Actions workflows run automatically:

**`.github/workflows/pr.yml`** — runs on every pull request targeting `main`:
- **Clang-Format** — enforces formatting with `clang-format-15`
- **Clang-Tidy** — configures with `clang++-18` and `-DENABLE_CLANG_TIDY=ON`, treats all warnings as errors
- **Build & Test** — configures with `g++-13`, builds, and runs unit tests via CTest

**`.github/workflows/main.yml`** — runs on every push to `main` with the same three jobs.

## Project Structure

```
.
├── .clang-format
├── .clang-tidy
├── .clangd
├── CMakeLists.txt
├── .github/
│   └── workflows/
│       ├── pr.yml              # PR check: formatting + clang-tidy + build + test
│       └── main.yml            # Main branch: same jobs on push to main
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
└── tests/
    ├── unit/
    │   ├── test_claude_llm_client.cpp
    │   └── test_make_claude_client.cpp
    └── integration/
        └── test_claude_llm_client_integration.cpp
```
