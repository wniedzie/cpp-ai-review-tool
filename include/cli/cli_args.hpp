#ifndef CPP_REVIEW_CLI_CLI_ARGS_HPP
#define CPP_REVIEW_CLI_CLI_ARGS_HPP

#include <expected>
#include <filesystem>
#include <optional>
#include <set>
#include <string>
#include <string_view>

namespace cli {

enum class CheckCategory { ub, memory, modernization };

enum class Severity { critical, high, medium, low, info };

enum class OutputFormat { markdown, json, sarif };

[[nodiscard]] constexpr std::string_view to_string(CheckCategory c) noexcept {
    switch (c) {
        case CheckCategory::ub: return "ub";
        case CheckCategory::memory: return "memory";
        case CheckCategory::modernization: return "modernization";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(Severity s) noexcept {
    switch (s) {
        case Severity::critical: return "critical";
        case Severity::high: return "high";
        case Severity::medium: return "medium";
        case Severity::low: return "low";
        case Severity::info: return "info";
    }
    return "unknown";
}

[[nodiscard]] constexpr std::string_view to_string(OutputFormat f) noexcept {
    switch (f) {
        case OutputFormat::markdown: return "markdown";
        case OutputFormat::json: return "json";
        case OutputFormat::sarif: return "sarif";
    }
    return "unknown";
}

struct CliArgs {
    std::filesystem::path                input_path;
    std::set<CheckCategory>              checks;   // default: all three
    std::set<Severity>                   fail_on;  // default: {high, critical}
    OutputFormat                         format = OutputFormat::markdown;
    std::optional<std::filesystem::path> output_file;
    bool                                 dry_run              = false;
    bool                                 no_telemetry_warning = false;
    std::optional<std::filesystem::path> config_file;  // reserved for FR-09
};

[[nodiscard]] std::expected<CliArgs, std::string> parse_args(int argc, const char* const* argv);

}  // namespace cli

#endif  // CPP_REVIEW_CLI_CLI_ARGS_HPP
