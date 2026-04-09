#include "config/config_loader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "core/types.hpp"

namespace config {

namespace detail {
using namespace core;

// ─── JSON file reading ────────────────────────────────────────────────────────

// Returns:
//   - std::nullopt (no error) if config_file_path is nullopt and no file exists in CWD
//   - a parsed json object on success
//   - ConfigError on failure
[[nodiscard]] std::expected<std::optional<nlohmann::json>, ConfigError>
read_json_file(const std::optional<std::filesystem::path>& config_file_path) {
    std::filesystem::path path;

    if (config_file_path) {
        path = *config_file_path;
        if (!std::filesystem::exists(path))
            return std::unexpected(ConfigError::file_not_found);
    } else {
        path = std::filesystem::current_path() / ".cpp-review.json";
        if (!std::filesystem::exists(path))
            return std::optional<nlohmann::json>{std::nullopt};
    }

    std::ifstream file{path};
    if (!file.is_open())
        return std::unexpected(ConfigError::file_not_found);

    try {
        return nlohmann::json::parse(file);
    } catch (const nlohmann::json::parse_error&) {
        return std::unexpected(ConfigError::parse_error);
    }
}

// ─── JSON → Config merger ─────────────────────────────────────────────────────

[[nodiscard]] std::expected<void, ConfigError>
apply_json_config(Config& cfg, const nlohmann::json& json) {
    if (json.contains("checks") && json["checks"].is_array()) {
        std::vector<CheckCategory> checks;
        for (const auto& item : json["checks"]) {
            if (!item.is_string())
                return std::unexpected(ConfigError::invalid_check_category);
            const auto cat = from_string<CheckCategory>(item.get<std::string>());
            if (!cat)
                return std::unexpected(ConfigError::invalid_check_category);
            checks.push_back(*cat);
        }
        cfg.checks = std::move(checks);
    }

    if (json.contains("fail_on") && json["fail_on"].is_array()) {
        std::vector<Severity> severities;
        for (const auto& item : json["fail_on"]) {
            if (!item.is_string())
                return std::unexpected(ConfigError::invalid_severity);
            const auto sev = from_string<Severity>(item.get<std::string>());
            if (!sev)
                return std::unexpected(ConfigError::invalid_severity);
            severities.push_back(*sev);
        }
        cfg.fail_on = std::move(severities);
    }

    if (json.contains("output_format") && json["output_format"].is_string()) {
        const auto fmt = from_string<OutputFormat>(json["output_format"].get<std::string>());
        if (!fmt)
            return std::unexpected(ConfigError::invalid_output_format);
        cfg.output_format = *fmt;
    }

    if (json.contains("output_file") && json["output_file"].is_string())
        cfg.output_file = json["output_file"].get<std::string>();

    if (json.contains("dry_run") && json["dry_run"].is_boolean())
        cfg.dry_run = json["dry_run"].get<bool>();

    if (json.contains("no_telemetry_warning") && json["no_telemetry_warning"].is_boolean())
        cfg.no_telemetry_warning = json["no_telemetry_warning"].get<bool>();

    if (json.contains("excluded_paths") && json["excluded_paths"].is_array()) {
        for (const auto& item : json["excluded_paths"]) {
            if (item.is_string())
                cfg.excluded_paths.push_back(item.get<std::string>());
        }
    }

    return {};
}

}  // namespace detail

// ─── ConfigLoader::load ───────────────────────────────────────────────────────

std::expected<Config, ConfigError>
ConfigLoader::load(const CliArgs& cli, std::optional<std::filesystem::path> config_file_path) {
    // 1. Built-in defaults
    Config cfg{
        .api_key = "",
        .model = "claude-sonnet-4-6",
        .checks = {CheckCategory::ub, CheckCategory::memory, CheckCategory::modernization},
        .fail_on = {Severity::high, Severity::critical},
        .output_format = OutputFormat::markdown,
        .output_file = std::nullopt,
        .dry_run = false,
        .no_telemetry_warning = false,
        .input_paths = cli.input_paths,
        .excluded_paths = {}};

    // 2. Config file (.cpp-review.json)
    auto json_result = detail::read_json_file(config_file_path);
    if (!json_result)
        return std::unexpected(json_result.error());
    if (auto& json_opt = *json_result; json_opt) {
        auto apply_result = detail::apply_json_config(cfg, *json_opt);
        if (!apply_result)
            return std::unexpected(apply_result.error());
    }

    // 3. Environment variables
    if (const char* key = std::getenv("ANTHROPIC_API_KEY"); key != nullptr && *key != '\0')
        cfg.api_key = key;
    if (const char* model = std::getenv("CPP_REVIEW_MODEL"); model != nullptr && *model != '\0')
        cfg.model = model;

    // 4. CLI overrides (highest-priority source for all fields except api_key / model)
    if (cli.checks)
        cfg.checks = *cli.checks;
    if (cli.fail_on)
        cfg.fail_on = *cli.fail_on;
    if (cli.output_format)
        cfg.output_format = *cli.output_format;
    if (cli.output_file)
        cfg.output_file = *cli.output_file;
    if (cli.dry_run)
        cfg.dry_run = *cli.dry_run;
    if (cli.no_telemetry_warning)
        cfg.no_telemetry_warning = *cli.no_telemetry_warning;

    // 5. Validation
    if (!cfg.dry_run && cfg.api_key.empty())
        return std::unexpected(ConfigError::missing_api_key);

    return cfg;
}

}  // namespace config
