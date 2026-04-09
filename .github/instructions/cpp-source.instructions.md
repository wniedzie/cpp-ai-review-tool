---
description: "Use when writing or modifying C++ source files (.cpp, .cxx, .cc). Covers implementation patterns, functional style, views usage, and RAII."
applyTo: "**/*.cpp,**/*.cxx,**/*.cc"
---

# C++ Source File Guidelines

## Implementation Style

- Prefer `std::views` pipelines and `std::ranges` algorithms over manual loops.
- Write pure functions by default — take `const` inputs, return results, avoid side effects.
- Use `std::expected<T, E>` for functions that can fail; chain with `.and_then()` / `.transform()`.
- Use lambdas for local behavior; prefer generic lambdas (`auto` params) when the body is generic.
- Use `constexpr` / `consteval` on any function that can be evaluated at compile time.
- **No anonymous namespaces.** Always name namespaces: use `detail` for implementation-private helpers, `helpers` for test helpers, or another descriptive name.

## Almost Always Auto

- Prefer `auto` for local variable declarations — let the compiler deduce the type.
- Use `auto` for lambda parameters and return types when the type is obvious from context.
- Use explicit types only when the deduced type is unclear, when a conversion is intended, or when a specific type is required for correctness.
- Combine with `const`: prefer `const auto` as the default for local variables.

## Almost Always Auto

- Prefer `auto` for local variable declarations — let the compiler deduce the type.
- Use `auto` for lambda parameters and return types when the type is obvious from context.
- Use explicit types only when the deduced type is unclear, when a conversion is intended, or when a specific type is required for correctness.
- Combine with `const`: prefer `const auto` as the default for local variables.

## Resource & Lifetime

- Never use naked `new` / `delete`. Use smart pointers or RAII wrappers.
- Acquire resources in constructors, release in destructors.
- Pass non-owning access as `const T&`, `T&`, or `std::span<T>`.

## Error Handling

- Return `std::expected<T, E>` for recoverable errors.
- Use `std::optional<T>` when a value may legitimately be absent.
- Throw exceptions only for truly unrecoverable errors.

## Naming Conventions

- **Constants / `constexpr` values**: `snake_case` with no prefix. Example: `constexpr double tokens_per_second = 0.5;`
- **Private member variables**: `snake_case` with a trailing `_` suffix — no `m_` or other prefix. Example: `std::string content_;`

## Control Flow

- Prefer **guard clauses** (early returns) over nested `if` blocks.
- Handle preconditions and error cases at the top of a function; keep the happy path at the lowest indentation level.
