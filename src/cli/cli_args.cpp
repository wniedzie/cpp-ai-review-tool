#include "cli/cli_args.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <CLI/CLI.hpp>

#include "core/hash.hpp"

namespace cli {

namespace {
using namespace std::literals;
using namespace core;

[[nodiscard]] std::optional<CheckCategory>
check_category_from_string(const std::string_view& category) noexcept {
    switch (hash(category)) {
        case "ub"_h: return CheckCategory::ub;
        case "memory"_h: return CheckCategory::memory;
        case "modernization"_h: return CheckCategory::modernization;
        default: return std::nullopt;
    }
}

[[nodiscard]] std::optional<Severity> severity_from_string(const std::string_view& severity
) noexcept {
    switch (hash(severity)) {
        case "critical"_h: return Severity::critical;
        case "high"_h: return Severity::high;
        case "medium"_h: return Severity::medium;
        case "low"_h: return Severity::low;
        case "info"_h: return Severity::info;
        default: return std::nullopt;
    }
}

[[nodiscard]] std::optional<OutputFormat> output_format_from_string(std::string_view format
) noexcept {
    switch (hash(format)) {
        case "markdown"_h: return OutputFormat::markdown;
        case "json"_h: return OutputFormat::json;
        case "sarif"_h: return OutputFormat::sarif;
        default: return std::nullopt;
    }
}

[[nodiscard]] std::expected<std::vector<CheckCategory>, std::string>
parse_checks(const std::vector<std::string>& raw) {
    const auto cats = raw | std::views::transform(check_category_from_string);

    if (const auto bad = std::ranges::find_if(cats, [](const auto& opt) { return !opt; });
        bad != cats.end()) {
        return std::unexpected(std::format(
            "unknown check category: '{}'", raw[static_cast<std::size_t>(bad - cats.begin())]
        ));
    }

    const auto deref = cats | std::views::transform([](const auto& opt) { return *opt; });
    return std::vector<CheckCategory>(deref.begin(), deref.end());
}

[[nodiscard]] std::expected<std::vector<Severity>, std::string>
parse_fail_on(const std::vector<std::string>& raw) {
    const auto sevs = raw | std::views::transform(severity_from_string);

    if (const auto bad = std::ranges::find_if(sevs, [](const auto& opt) { return !opt; });
        bad != sevs.end()) {
        return std::unexpected(std::format(
            "unknown severity level: '{}'", raw[static_cast<std::size_t>(bad - sevs.begin())]
        ));
    }

    const auto deref = sevs | std::views::transform([](const auto& opt) { return *opt; });
    return std::vector<Severity>(deref.begin(), deref.end());
}

}  // namespace

std::expected<CliArgs, std::string> parse_args(int argc, const char* const* argv) {
    CLI::App app{"C++ AI-powered code review tool"};

#ifdef CPP_REVIEW_VERSION
    app.set_version_flag("--version", std::string{CPP_REVIEW_VERSION});
#endif

    std::string input_path_str;
    app.add_option("path", input_path_str, "C++ source file or directory to review")->required();

    std::vector<std::string> checks_raw;
    app.add_option(
           "--checks", checks_raw, "Comma-separated check categories: ub,memory,modernization"
    )
        ->delimiter(',');

    std::vector<std::string> fail_on_raw;
    app.add_option(
           "--fail-on", fail_on_raw, "Severity levels that cause exit code 1 (comma-separated)"
    )
        ->delimiter(',');

    std::string format_str;
    app.add_option("--format", format_str, "Output format: markdown, json, sarif");

    std::string output_file_str;
    app.add_option("--output", output_file_str, "Write output to file instead of stdout");

    bool dry_run = false;
    app.add_flag("--dry-run", dry_run, "Estimate token count and cost without calling the API");

    bool no_telemetry_warning = false;
    app.add_flag(
        "--no-telemetry-warning", no_telemetry_warning, "Suppress the one-time privacy warning"
    );

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        if (e.get_exit_code() == 0) {
            app.exit(e);  // prints --help or --version output
            std::exit(0);
        }
        return std::unexpected(e.what());
    }

    // Validate input path exists
    const std::filesystem::path input_path{input_path_str};
    std::error_code err;
    if (!std::filesystem::exists(input_path, err)) {
        if (err) {
            return std::unexpected(
                std::format("cannot access path '{}': {}", input_path_str, err.message())
            );
        }
        return std::unexpected(std::format("path does not exist: '{}'", input_path_str));
    }

    // Convert and validate explicitly provided --checks
    CliArgs args;
    args.input_path = input_path;

    if (app.count("--checks") > 0) {
        const auto checks = parse_checks(checks_raw);
        if (!checks)
            return std::unexpected(checks.error());
        args.checks = *checks;
    }

    // Convert and validate explicitly provided --fail-on
    if (app.count("--fail-on") > 0) {
        const auto fail_on = parse_fail_on(fail_on_raw);
        if (!fail_on)
            return std::unexpected(fail_on.error());
        args.fail_on = *fail_on;
    }

    // Convert and validate explicitly provided --format
    if (app.count("--format") > 0) {
        const auto format = output_format_from_string(format_str);
        if (!format)
            return std::unexpected(std::format("unknown output format: '{}'", format_str));
        args.format = format;
    }

    if (!output_file_str.empty())
        args.output_file = output_file_str;

    if (app.count("--dry-run") > 0)
        args.dry_run = dry_run;

    if (app.count("--no-telemetry-warning") > 0)
        args.no_telemetry_warning = no_telemetry_warning;

    return args;
}

config::CliArgs to_config_args(const CliArgs& cli) {
    return {
        .checks = cli.checks,
        .fail_on = cli.fail_on,
        .output_format = cli.format,
        .output_file = cli.output_file,
        .dry_run = cli.dry_run,
        .no_telemetry_warning = cli.no_telemetry_warning,
        .input_paths = {cli.input_path.string()},
    };
}

}  // namespace cli
