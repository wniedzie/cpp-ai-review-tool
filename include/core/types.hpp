#ifndef CPP_REVIEW_CORE_TYPES_HPP
#define CPP_REVIEW_CORE_TYPES_HPP

#include <cstdint>
#include <optional>
#include <string_view>
#include <type_traits>

#include "core/hash.hpp"

namespace core {

enum class CheckCategory : std::uint8_t { ub, memory, modernization };

enum class Severity : std::uint8_t { info, low, medium, high, critical };

enum class OutputFormat : std::uint8_t { markdown, json, sarif };

[[nodiscard]] constexpr std::string_view to_string(CheckCategory category) noexcept {
    switch (category) {
        case CheckCategory::ub: return "ub";
        case CheckCategory::memory: return "memory";
        case CheckCategory::modernization: return "modernization";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(Severity severity) noexcept {
    switch (severity) {
        case Severity::info: return "info";
        case Severity::low: return "low";
        case Severity::medium: return "medium";
        case Severity::high: return "high";
        case Severity::critical: return "critical";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(OutputFormat format) noexcept {
    switch (format) {
        case OutputFormat::markdown: return "markdown";
        case OutputFormat::json: return "json";
        case OutputFormat::sarif: return "sarif";
    }
    return "unknown";
}

// ─── from_string (string_view → enum) ────────────────────────────────────────

template <typename Enum>
    requires std::is_enum_v<Enum>
[[nodiscard]] constexpr std::optional<Enum> from_string(std::string_view str) noexcept;

template <>
[[nodiscard]] inline constexpr std::optional<CheckCategory>
from_string<CheckCategory>(std::string_view str) noexcept {
    switch (hash(str)) {
        case "ub"_h: return CheckCategory::ub;
        case "memory"_h: return CheckCategory::memory;
        case "modernization"_h: return CheckCategory::modernization;
        default: return std::nullopt;
    }
}

template <>
[[nodiscard]] inline constexpr std::optional<Severity> from_string<Severity>(std::string_view str
) noexcept {
    switch (hash(str)) {
        case "info"_h: return Severity::info;
        case "low"_h: return Severity::low;
        case "medium"_h: return Severity::medium;
        case "high"_h: return Severity::high;
        case "critical"_h: return Severity::critical;
        default: return std::nullopt;
    }
}

template <>
[[nodiscard]] inline constexpr std::optional<OutputFormat>
from_string<OutputFormat>(std::string_view str) noexcept {
    switch (hash(str)) {
        case "markdown"_h: return OutputFormat::markdown;
        case "json"_h: return OutputFormat::json;
        case "sarif"_h: return OutputFormat::sarif;
        default: return std::nullopt;
    }
}

}  // namespace core

#endif  // CPP_REVIEW_CORE_TYPES_HPP
