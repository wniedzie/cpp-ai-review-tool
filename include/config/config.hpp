#ifndef CPP_REVIEW_CONFIG_CONFIG_HPP
#define CPP_REVIEW_CONFIG_CONFIG_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace config {

enum class CheckCategory : std::uint8_t { ub, memory, modernization };

enum class Severity : std::uint8_t { info, low, medium, high, critical };

enum class OutputFormat : std::uint8_t { markdown, json, sarif };

enum class ConfigError : std::uint8_t {
    file_not_found,          // explicit config file path given but does not exist
    parse_error,             // config file is not valid JSON
    invalid_check_category,  // unknown string value in "checks" array
    invalid_severity,        // unknown string value in "fail_on" array
    invalid_output_format,   // unknown --format value from CLI
    missing_api_key          // ANTHROPIC_API_KEY not set and dry_run is false
};

[[nodiscard]] constexpr std::string_view to_string(CheckCategory category) noexcept {
    switch (category) {
        case CheckCategory::ub:            return "ub";
        case CheckCategory::memory:        return "memory";
        case CheckCategory::modernization: return "modernization";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(Severity severity) noexcept {
    switch (severity) {
        case Severity::info:     return "info";
        case Severity::low:      return "low";
        case Severity::medium:   return "medium";
        case Severity::high:     return "high";
        case Severity::critical: return "critical";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(OutputFormat format) noexcept {
    switch (format) {
        case OutputFormat::markdown: return "markdown";
        case OutputFormat::json:     return "json";
        case OutputFormat::sarif:    return "sarif";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(ConfigError error) noexcept {
    switch (error) {
        case ConfigError::file_not_found:
            return "config file not found";
        case ConfigError::parse_error:
            return "failed to parse config file (invalid JSON)";
        case ConfigError::invalid_check_category:
            return "invalid check category (valid: ub, memory, modernization)";
        case ConfigError::invalid_severity:
            return "invalid severity level (valid: critical, high, medium, low, info)";
        case ConfigError::invalid_output_format:
            return "invalid output format (valid: markdown, json, sarif)";
        case ConfigError::missing_api_key:
            return "ANTHROPIC_API_KEY environment variable is not set";
    }
    return "unknown config error";
}

// ─── CLI-provided overrides (std::nullopt = not explicitly provided) ──────────
// All fields are optional so the config merger can distinguish "not set" from
// an explicit value that overrides lower-priority sources.
struct CliArgs {
    std::optional<std::vector<CheckCategory>> checks;
    std::optional<std::vector<Severity>>      fail_on;
    std::optional<OutputFormat>               output_format;
    std::optional<std::string>                output_file;
    std::optional<bool>                       dry_run;
    std::optional<bool>                       no_telemetry_warning;
    std::vector<std::string>                  input_paths;
};

// ─── Fully-resolved configuration (no optionals except output_file) ───────────
// Produced by ConfigLoader::load() after merging all sources.
struct Config {
    std::string                api_key;             // from ANTHROPIC_API_KEY env var
    std::string                model;               // from CPP_REVIEW_MODEL env var or default
    std::vector<CheckCategory> checks;
    std::vector<Severity>      fail_on;
    OutputFormat               output_format;
    std::optional<std::string> output_file;         // nullopt = write to stdout
    bool                       dry_run;
    bool                       no_telemetry_warning;
    std::vector<std::string>   input_paths;
    std::vector<std::string>   excluded_paths;
};

}  // namespace config

#endif  // CPP_REVIEW_CONFIG_CONFIG_HPP
