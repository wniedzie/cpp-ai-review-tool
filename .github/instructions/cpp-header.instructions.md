---
description: "Use when writing or modifying C++ header files (.hpp, .h, .hxx). Covers interface design, concepts, and compile-time programming."
applyTo: "**/*.hpp,**/*.h,**/*.hxx"
---

# C++ Header File Guidelines

## Structure

- Use traditional `#ifndef` / `#define` / `#endif` include guards (not `#pragma once`). Guard name format: `PROJECT_PATH_FILENAME_HPP`.
- Order includes: standard library → third-party → project headers, separated by blank lines.
- Minimize includes — prefer forward declarations to reduce coupling.

## Interface Design (SOLID)

- One class or module per header.
- Keep interfaces small and focused (Interface Segregation).
- Depend on abstractions: use concepts or abstract base classes, not concrete types.
- Design for extension without modification (Open/Closed) — prefer templates and concepts.

## Concepts & Templates

- Constrain templates with C++20 concepts — never use `std::enable_if` or raw SFINAE.
- Define semantic concepts (e.g., `Serializable`, `Drawable`) not just syntactic checks.
- Mark functions `constexpr` when they can be evaluated at compile time.

## Almost Always Auto

- Prefer `auto` for variable declarations — let the compiler deduce the type.
- Use explicit types only when the deduced type is unclear, when a conversion is intended, or when a specific type is required for correctness.
- Combine with `const`: prefer `const auto` as the default.

## Const Correctness

- Mark member functions `const` when they do not modify observable state.
- Default parameters and variables to `const`.
