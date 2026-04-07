#include "cli/cli_args.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include <CLI/CLI.hpp>

#include "core/types.hpp"

namespace cli {

namespace helpers {
using namespace std::literals;
using namespace core;

template <typename Enum>
    requires std::is_enum_v<Enum>
[[nodiscard]] std::expected<std::vector<Enum>, std::string>
parse_enum_list(const std::vector<std::string>& raw, std::string_view error_prefix) {
    const auto opts =
        raw | std::views::transform([](const auto& str) { return from_string<Enum>(str); });

    if (const auto bad = std::ranges::find_if(opts, [](const auto& opt) { return !opt; });
        bad != opts.end()) {
        return std::unexpected(
            std::format("{}: '{}'", error_prefix, raw[static_cast<std::size_t>(bad - opts.begin())])
        );
    }

    std::vector<Enum> result;
    result.reserve(raw.size());
    std::unordered_set<Enum> seen;
    seen.reserve(raw.size());
    for (auto val : opts | std::views::transform([](const auto& opt) { return *opt; })) {
        if (seen.insert(val).second)
            result.push_back(val);
    }
    return result;
}

}  // namespace helpers

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

    std::string config_file_str;
    app.add_option("--config,--config-file", config_file_str, "Path to configuration file");

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
        const auto checks =
            helpers::parse_enum_list<CheckCategory>(checks_raw, "unknown check category");
        if (!checks)
            return std::unexpected(checks.error());
        args.checks = *checks;
    }

    // Convert and validate explicitly provided --fail-on
    if (app.count("--fail-on") > 0) {
        const auto fail_on =
            helpers::parse_enum_list<Severity>(fail_on_raw, "unknown severity level");
        if (!fail_on)
            return std::unexpected(fail_on.error());
        args.fail_on = *fail_on;
    }

    // Convert and validate explicitly provided --format
    if (app.count("--format") > 0) {
        const auto format = core::from_string<OutputFormat>(format_str);
        if (!format)
            return std::unexpected(std::format("unknown output format: '{}'", format_str));
        args.format = format;
    }

    if (!output_file_str.empty())
        args.output_file = output_file_str;

    if (!config_file_str.empty()) {
        const std::filesystem::path config_path{config_file_str};
        std::error_code config_err;
        if (!std::filesystem::exists(config_path, config_err)) {
            if (config_err) {
                return std::unexpected(std::format(
                    "cannot access config file '{}': {}", config_file_str, config_err.message()
                ));
            }
            return std::unexpected(std::format("config file does not exist: '{}'", config_file_str)
            );
        }
        args.config_file = config_path;
    }

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
