#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "config/config.hpp"
#include "config/config_loader.hpp"

namespace config::test {

using ::testing::ElementsAre;

// ─── Helpers ──────────────────────────────────────────────────────────────────

// RAII guard that sets an environment variable and restores its previous state
// on destruction. Handles the cases where the variable was unset, set to empty,
// or set to a non-empty value before the guard was constructed.
class EnvGuard {
public:
    EnvGuard(const char* name, const char* value) : m_name{name} {
        if (const char* prev = std::getenv(name))
            m_previous = prev;
        setenv(name, value, /*overwrite=*/1);
    }
    ~EnvGuard() {
        if (m_previous)
            setenv(m_name, m_previous->c_str(), 1);
        else
            unsetenv(m_name);
    }

    EnvGuard(const EnvGuard&)            = delete;
    EnvGuard& operator=(const EnvGuard&) = delete;

private:
    const char*                m_name;
    std::optional<std::string> m_previous;
};

// Writes content to a temp file and returns its path.
[[nodiscard]] std::filesystem::path write_temp_json(const std::string& content) {
    auto path = std::filesystem::temp_directory_path() / "cpp_review_test_config.json";
    std::ofstream{path} << content;
    return path;
}

// ─── Default config ───────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, DefaultsAppliedWhenDryRun) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", ""};
    const CliArgs  cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->model, "claude-sonnet-4-6");
    EXPECT_EQ(result->output_format, OutputFormat::markdown);
    EXPECT_TRUE(result->dry_run);
    EXPECT_FALSE(result->no_telemetry_warning);
    EXPECT_FALSE(result->output_file.has_value());
    EXPECT_TRUE(result->excluded_paths.empty());
    EXPECT_THAT(
        result->checks,
        ElementsAre(CheckCategory::ub, CheckCategory::memory, CheckCategory::modernization)
    );
    EXPECT_THAT(result->fail_on, ElementsAre(Severity::high, Severity::critical));
}

// ─── API key validation ───────────────────────────────────────────────────────

TEST(ConfigLoaderTest, MissingApiKeyReturnsMissingApiKeyError) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", ""};
    const CliArgs  cli{};

    const auto result = ConfigLoader::load(cli);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::missing_api_key);
}

TEST(ConfigLoaderTest, DryRunBypassesApiKeyValidation) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", ""};
    const CliArgs  cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->api_key.empty());
}

// ─── Environment variables ────────────────────────────────────────────────────

TEST(ConfigLoaderTest, EnvVarApiKeyIsApplied) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key-123"};
    const CliArgs  cli{};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->api_key, "test-key-123");
}

TEST(ConfigLoaderTest, EnvVarModelIsApplied) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const EnvGuard _model{"CPP_REVIEW_MODEL", "claude-opus-4-0"};
    const CliArgs  cli{};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->model, "claude-opus-4-0");
}

// ─── CLI overrides ────────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, CliChecksOverrideDefault) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs  cli{.checks = {{CheckCategory::ub}}};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->checks, ElementsAre(CheckCategory::ub));
}

TEST(ConfigLoaderTest, CliFailOnOverrideDefault) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs  cli{.fail_on = {{Severity::critical}}};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->fail_on, ElementsAre(Severity::critical));
}

TEST(ConfigLoaderTest, CliOutputFormatOverrideDefault) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs  cli{.output_format = OutputFormat::sarif};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->output_format, OutputFormat::sarif);
}

TEST(ConfigLoaderTest, CliOutputFileIsSet) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs  cli{.output_file = "report.md"};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->output_file.has_value());
    EXPECT_EQ(*result->output_file, "report.md");
}

TEST(ConfigLoaderTest, CliNoTelemetryWarningIsSet) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs  cli{.no_telemetry_warning = true};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->no_telemetry_warning);
}

TEST(ConfigLoaderTest, CliInputPathsPassedThrough) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs  cli{.input_paths = {"src/a.cpp", "src/b.cpp"}};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->input_paths, ElementsAre("src/a.cpp", "src/b.cpp"));
}

// ─── JSON config file ─────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, JsonConfigLoadsChecks) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const auto     path = write_temp_json(R"({"checks": ["memory"]})");
    const CliArgs  cli{};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->checks, ElementsAre(CheckCategory::memory));
}

TEST(ConfigLoaderTest, JsonConfigLoadsFailOn) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const auto     path = write_temp_json(R"({"fail_on": ["critical"]})");
    const CliArgs  cli{};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->fail_on, ElementsAre(Severity::critical));
}

TEST(ConfigLoaderTest, JsonConfigLoadsExcludedPaths) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const auto     path = write_temp_json(R"({"exclude": ["build/", "third_party/"]})");
    const CliArgs  cli{};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->excluded_paths, ElementsAre("build/", "third_party/"));
}

TEST(ConfigLoaderTest, CliChecksOverrideJsonConfig) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const auto     path = write_temp_json(R"({"checks": ["memory"]})");
    const CliArgs  cli{.checks = {{CheckCategory::ub}}};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->checks, ElementsAre(CheckCategory::ub));  // CLI wins
}

// ─── JSON config file — error cases ──────────────────────────────────────────

TEST(ConfigLoaderTest, InvalidJsonReturnsParseError) {
    const auto    path = write_temp_json("not valid json {{{");
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::parse_error);
}

TEST(ConfigLoaderTest, InvalidCheckCategoryInJsonReturnsError) {
    const auto    path = write_temp_json(R"({"checks": ["unknown_category"]})");
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::invalid_check_category);
}

TEST(ConfigLoaderTest, InvalidSeverityInJsonReturnsError) {
    const auto    path = write_temp_json(R"({"fail_on": ["ultra"]})");
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, path);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::invalid_severity);
}

TEST(ConfigLoaderTest, NonExistentExplicitConfigFileReturnsFileNotFound) {
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, "/nonexistent/path/to/.cpp-review.json");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::file_not_found);
}

}  // namespace config::test
