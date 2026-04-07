#ifndef CPP_REVIEW_CORE_TYPES_HPP
#define CPP_REVIEW_CORE_TYPES_HPP

#include <cstdint>
#include <string_view>

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

}  // namespace core

#endif  // CPP_REVIEW_CORE_TYPES_HPP
