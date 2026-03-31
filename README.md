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

## Project Structure

```
.
├── .clang-format
├── .clangd
├── CMakeLists.txt
├── include/
│   └── llm/
│       ├── llm_client.hpp          # Abstract LLM client interface
│       ├── llm_request.hpp
│       ├── llm_response.hpp
│       ├── llm_error.hpp
│       ├── claude_llm_client.hpp   # Anthropic Claude implementation
│       └── rate_limited_llm_client.hpp
├── src/
│   ├── main.cpp
│   └── llm/
│       ├── claude_llm_client.cpp
│       └── rate_limited_llm_client.cpp
└── docs/
```
