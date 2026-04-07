#ifndef CPP_REVIEW_CONFIG_CONFIG_LOADER_HPP
#define CPP_REVIEW_CONFIG_CONFIG_LOADER_HPP

#include <expected>
#include <filesystem>
#include <optional>

#include "config/config.hpp"

namespace config {

// Merges configuration from four sources in descending priority order:
//   1. cli            — explicitly provided CLI flags (highest)
//   2. env vars       — ANTHROPIC_API_KEY, CPP_REVIEW_MODEL
//   3. config file    — .cpp-review.json (searched in CWD unless overridden)
//   4. built-in defaults (lowest)
//
// Returns a fully-resolved Config or a ConfigError if any source is invalid.
class ConfigLoader {
public:
    // config_file_path: when std::nullopt, searches for .cpp-review.json in the
    //   current working directory. A missing CWD config file is not an error.
    //   When an explicit path is provided, the file must exist.
    [[nodiscard]] static std::expected<Config, ConfigError> load(
        const CliArgs& cli, std::optional<std::filesystem::path> config_file_path = std::nullopt
    );

    ConfigLoader()                               = delete;
    ConfigLoader(const ConfigLoader&)            = delete;
    ConfigLoader(ConfigLoader&&)                 = delete;
    ConfigLoader& operator=(const ConfigLoader&) = delete;
    ConfigLoader& operator=(ConfigLoader&&)      = delete;
};

}  // namespace config

#endif  // CPP_REVIEW_CONFIG_CONFIG_LOADER_HPP
