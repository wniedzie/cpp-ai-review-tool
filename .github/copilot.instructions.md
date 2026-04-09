# Project Guidelines — C++ AI Review Tool

## Language & Standard

- **C++23** is the minimum standard. Use `-std=c++23` (or equivalent) in all build configurations.
- Prefer C++23 features over older alternatives (e.g., `std::expected` over error-code pairs, `std::print` over `printf`/`iostream` formatting).

## Programming Paradigm (Priority Order)

1. **Functional programming first.** Default to a functional style:
   - Write **pure functions** — same input always produces same output, no side effects.
   - Prefer **immutable data** — mark variables `const` or `constexpr` by default.
   - Use **`std::views`** and `<ranges>` pipelines for data transformations instead of raw loops.
   - Compose behavior with **lambdas**, **higher-order functions**, and **standard algorithms** (`std::transform`, `std::accumulate`, `std::reduce`, `std::ranges::*`).
   - Favor **`std::expected<T, E>`** and **`std::optional<T>`** with monadic operations (`.and_then()`, `.transform()`, `.or_else()`) for error handling and optional chaining.
   - Use **`std::variant`** with `std::visit` for sum-type polymorphism when appropriate.
2. **Object-oriented programming** when functional is insufficient (stateful abstractions, resource ownership, polymorphic hierarchies). Follow SOLID principles (see below).
3. Mixing functional and OOP within a component is allowed — but **prefer functional** wherever practical. Use OOP for stateful abstractions and resource ownership, and functional style for logic and data transformations.

## Design Principles

### SOLID

- **S — Single Responsibility:** Each class/module has exactly one reason to change.
- **O — Open/Closed:** Extend behavior through composition, templates, or concepts — not by modifying existing code.
- **L — Liskov Substitution:** Derived types must be substitutable for their base types without breaking invariants.
- **I — Interface Segregation:** Prefer small, focused interfaces (abstract base classes or concepts). Clients should not depend on methods they do not use.
- **D — Dependency Inversion:** Depend on abstractions (concepts, interfaces), not concrete implementations. Inject dependencies via constructor parameters or template arguments.

### KISS

- Favor the simplest solution that meets requirements.
- Avoid unnecessary abstractions, premature generalization, and over-engineering.
- Prefer standard library facilities over custom implementations.

### DRY

- Extract repeated logic into named functions or templates.
- Do not duplicate business rules — single source of truth for each piece of knowledge.
- Use templates and concepts to generalize without code duplication.

## Dependency Injection

- **Constructor injection is the default.** Pass dependencies as constructor parameters — never create concrete collaborators internally (except in factory functions).
- **Own via `std::unique_ptr<Interface>`.** Injected dependencies are moved into the owning class. Use `std::unique_ptr` for exclusive ownership of polymorphic collaborators.
- **One production constructor, one testable constructor.** The production constructor may create defaults internally for convenience. Provide a second constructor (or make the default one accept an interface pointer) so tests can inject mocks/fakes without touching production wiring.
- **Factory functions wire production graphs.** Free functions like `make_*()` read configuration, create concrete implementations, and return `std::unique_ptr<Interface>`. Keep factories in source files — never in headers.
- **Concepts for compile-time DI.** When virtual dispatch is unnecessary, constrain templates with C++20 concepts (e.g., `LlmClientLike`) instead of abstract base classes. This enables zero-overhead injection and better compiler diagnostics.
- **No IoC container.** The project is small enough for manual wiring. Do not introduce a DI framework.
- **Decorator pattern for cross-cutting concerns.** Wrap an interface implementation with another implementation of the same interface (e.g., `RateLimitedLlmClient` wrapping `LlmClient`). Chain decorators via constructor injection.
- **Every boundary gets an interface.** External systems (HTTP, LLM API, file system, config loading) must be accessed through an abstract base class or concept so they can be mocked in tests.

## Design Patterns

Apply design patterns where they solve a real problem. Prefer modern C++ idioms:

- **Strategy** — `std::function` or template parameter for interchangeable algorithms.
- **Observer** — signals/slots or callback registries with `std::function`.
- **Factory** — `std::unique_ptr`-returning factory functions; use concepts to constrain produced types.
- **Builder** — fluent interface with method chaining returning `*this` by reference.
- **RAII** — always acquire resources in constructors, release in destructors. Use `std::unique_ptr`, `std::shared_ptr`, `std::lock_guard`, `std::scoped_lock`.
- **Visitor** — prefer `std::variant` + `std::visit` over classical double-dispatch.
- **Template Method** — use CRTP or concepts over virtual dispatch when compile-time polymorphism suffices.
- **Decorator / Adapter** — compose via templates or lambdas wrapping callables.

## `std::views` & Ranges

- **Default to `std::views` pipelines** for filtering, transforming, and composing sequences.
- Chain views lazily: `auto result = data | std::views::filter(pred) | std::views::transform(fn);`
- Use `std::ranges::to<Container>()` (C++23) for materialization.
- Prefer range-based algorithms (`std::ranges::sort`, `std::ranges::find_if`) over iterator-pair overloads.
- Write custom views only when standard adaptors are insufficient.

## Resource Management

- **RAII everywhere** — never use naked `new`/`delete`.
- `std::unique_ptr` for exclusive ownership, `std::shared_ptr` only when ownership is genuinely shared.
- Pass non-owning references as `T&`, `const T&`, or `std::span<T>` — never raw pointers for ownership.

## Error Handling

- Use **`std::expected<T, E>`** (C++23) as the primary error-handling mechanism for recoverable errors.
- Use **exceptions** only for truly exceptional, unrecoverable situations.
- Never use error codes as return values — wrap in `std::expected`.
- Use `std::optional<T>` to represent absence of a value (not an error).

## Compile-Time Programming

- Mark functions `constexpr` or `consteval` when possible.
- Use **C++20 concepts** to constrain templates — do not use `std::enable_if` or SFINAE.
- Define meaningful, semantic concepts (e.g., `Number`, `Serializable`) over bare syntactic constraints.
- Prefer `constexpr` computations over template metaprogramming for value-level computation.

## Almost Always Auto

- Prefer `auto` for local variable declarations — let the compiler deduce the type.
- Use `auto` for lambda parameters and return types when the type is obvious from context.
- Use explicit types only when the deduced type is unclear, when a conversion is intended, or when a specific type is required for correctness.
- Combine with `const`: prefer `const auto` as the default for local variables.

## Const Correctness & Immutability

- Default to `const` — every variable, parameter, and member function that can be `const` should be.
- Use `constexpr` for compile-time constants.
- Mark member functions `const` when they do not modify observable state.
- Prefer value semantics and pass-by-const-reference over mutable shared state.

## Naming Conventions

- **Types** (classes, structs, concepts, enums, aliases): `PascalCase`
- **Functions, methods, variables, parameters**: `snake_case`
- **Constants, enum values**: `snake_case` (descriptive name, no prefix) or `UPPER_SNAKE_CASE`
- **Template parameters**: `PascalCase` (e.g., `typename Value`, `typename Predicate`)
- **Namespaces**: `lower_snake_case`
- **File names**: `snake_case.hpp`, `snake_case.cpp`

## Code Organization

- One class/module per header-source pair.
- Use traditional `#ifndef` / `#define` / `#endif` include guards (not `#pragma once`) and follow the single, non-reserved include-guard naming convention defined in `.github/instructions/cpp-header.instructions.md`.
- Prefer forward declarations to reduce header coupling.
- Group includes: standard library → third-party → project headers, separated by blank lines.
