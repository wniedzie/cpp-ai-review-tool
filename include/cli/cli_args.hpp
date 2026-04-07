#ifndef CPP_REVIEW_CLI_CLI_ARGS_HPP
#define CPP_REVIEW_CLI_CLI_ARGS_HPP

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "config/config.hpp"
#include "core/types.hpp"

namespace cli {

using core::CheckCategory;
using core::OutputFormat;
using core::Severity;
using core::to_string;

struct CliArgs {
    std::filesystem::path input_path;
    std::optional<std::vector<CheckCategory>> checks;
    std::optional<std::vector<Severity>> fail_on;
    std::optional<OutputFormat> format;
    std::optional<std::string> output_file;
    std::optional<bool> dry_run;
    std::optional<bool> no_telemetry_warning;
    std::optional<std::filesystem::path> config_file;
};

[[nodiscard]] std::expected<CliArgs, std::string> parse_args(int argc, const char* const* argv);

[[nodiscard]] config::CliArgs to_config_args(const CliArgs& cli);

}  // namespace cli

#endif  // CPP_REVIEW_CLI_CLI_ARGS_HPP
