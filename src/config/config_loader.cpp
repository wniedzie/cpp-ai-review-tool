#include "config/config_loader.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace config {

namespace {

// ─── String → enum parsers (internal to this translation unit) ───────────────

[[nodiscard]] constexpr std::optional<CheckCategory>
parse_check_category(std::string_view category) noexcept {
    if (category == "ub")             return CheckCategory::ub;
    if (category == "memory")         return CheckCategory::memory;
    if (category == "modernization")  return CheckCategory::modernization;
    return std::nullopt;
}

[[nodiscard]] constexpr std::optional<Severity>
parse_severity(std::string_view severity) noexcept {
    if (severity == "info")     return Severity::info;
    if (severity == "low")      return Severity::low;
    if (severity == "medium")   return Severity::medium;
    if (severity == "high")     return Severity::high;
    if (severity == "critical") return Severity::critical;
    return std::nullopt;
}

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
apply_json_config(Config& cfg, const nlohmann::json& j) {
    if (j.contains("checks") && j["checks"].is_array()) {
        std::vector<CheckCategory> checks;
        for (const auto& item : j["checks"]) {
            if (!item.is_string())
                return std::unexpected(ConfigError::invalid_check_category);
            const auto cat = parse_check_category(item.get<std::string>());
            if (!cat)
                return std::unexpected(ConfigError::invalid_check_category);
            checks.push_back(*cat);
        }
        cfg.checks = std::move(checks);
    }

    if (j.contains("fail_on") && j["fail_on"].is_array()) {
        std::vector<Severity> severities;
        for (const auto& item : j["fail_on"]) {
            if (!item.is_string())
                return std::unexpected(ConfigError::invalid_severity);
            const auto sev = parse_severity(item.get<std::string>());
            if (!sev)
                return std::unexpected(ConfigError::invalid_severity);
            severities.push_back(*sev);
        }
        cfg.fail_on = std::move(severities);
    }

    if (j.contains("exclude") && j["exclude"].is_array()) {
        for (const auto& item : j["exclude"]) {
            if (item.is_string())
                cfg.excluded_paths.push_back(item.get<std::string>());
        }
    }

    return {};
}

}  // namespace

// ─── ConfigLoader::load ───────────────────────────────────────────────────────

std::expected<Config, ConfigError> ConfigLoader::load(
    const CliArgs&                       cli,
    std::optional<std::filesystem::path> config_file_path
) {
    // 1. Built-in defaults
    Config cfg{
        .api_key              = "",
        .model                = "claude-sonnet-4-6",
        .checks               = {CheckCategory::ub, CheckCategory::memory, CheckCategory::modernization},
        .fail_on              = {Severity::high, Severity::critical},
        .output_format        = OutputFormat::markdown,
        .output_file          = std::nullopt,
        .dry_run              = false,
        .no_telemetry_warning = false,
        .input_paths          = cli.input_paths,
        .excluded_paths       = {}
    };

    // 2. Config file (.cpp-review.json)
    auto json_result = read_json_file(config_file_path);
    if (!json_result)
        return std::unexpected(json_result.error());
    if (*json_result) {
        auto apply_result = apply_json_config(cfg, **json_result);
        if (!apply_result)
            return std::unexpected(apply_result.error());
    }

    // 3. Environment variables
    if (const char* key = std::getenv("ANTHROPIC_API_KEY"); key != nullptr && *key != '\0')
        cfg.api_key = key;
    if (const char* model = std::getenv("CPP_REVIEW_MODEL"); model != nullptr && *model != '\0')
        cfg.model = model;

    // 4. CLI overrides (highest-priority source for all fields except api_key / model)
    if (cli.checks)               cfg.checks               = *cli.checks;
    if (cli.fail_on)              cfg.fail_on              = *cli.fail_on;
    if (cli.output_format)        cfg.output_format        = *cli.output_format;
    if (cli.output_file)          cfg.output_file          = *cli.output_file;
    if (cli.dry_run)              cfg.dry_run              = *cli.dry_run;
    if (cli.no_telemetry_warning) cfg.no_telemetry_warning = *cli.no_telemetry_warning;

    // 5. Validation
    if (!cfg.dry_run && cfg.api_key.empty())
        return std::unexpected(ConfigError::missing_api_key);

    return cfg;
}

}  // namespace config
