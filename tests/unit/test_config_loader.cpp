#include <atomic>
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
    EnvGuard(const char* name, const char* value)
        : m_name{name} {
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

    EnvGuard(const EnvGuard&) = delete;
    EnvGuard& operator=(const EnvGuard&) = delete;
    EnvGuard(EnvGuard&&) = delete;
    EnvGuard& operator=(EnvGuard&&) = delete;

private:
    const char* m_name;
    std::optional<std::string> m_previous;
};

// RAII guard that writes content to a uniquely-named temp file and removes it
// on destruction. Unique names are derived from the process ID and a
// per-process monotonic counter, so parallel test processes never collide.
class TempJsonFile {
public:
    explicit TempJsonFile(const std::string& content) {
        static std::atomic<unsigned> counter{0};
        const auto filename = "cpp_review_test_" + std::to_string(::getpid()) + "_" +
                              std::to_string(counter.fetch_add(1, std::memory_order_relaxed)) +
                              ".json";
        m_path = std::filesystem::temp_directory_path() / filename;
        std::ofstream{m_path} << content;
    }

    ~TempJsonFile() {
        std::filesystem::remove(m_path);
    }

    TempJsonFile(const TempJsonFile&) = delete;
    TempJsonFile& operator=(const TempJsonFile&) = delete;
    TempJsonFile(TempJsonFile&&) = delete;
    TempJsonFile& operator=(TempJsonFile&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept {
        return m_path;
    }

    // Implicit conversion so existing call-sites that pass the result directly
    // to ConfigLoader::load(cli, path) continue to compile without changes.
    operator const std::filesystem::path&() const noexcept {
        return m_path;
    }

private:
    std::filesystem::path m_path;
};

// ─── Default config ───────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, DefaultsAppliedWhenDryRun) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", ""};
    const CliArgs cli{.dry_run = true};

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
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::missing_api_key);
}

TEST(ConfigLoaderTest, DryRunBypassesApiKeyValidation) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", ""};
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->api_key.empty());
}

// ─── Environment variables ────────────────────────────────────────────────────

TEST(ConfigLoaderTest, EnvVarApiKeyIsApplied) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key-123"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->api_key, "test-key-123");
}

TEST(ConfigLoaderTest, EnvVarModelIsApplied) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const EnvGuard _model{"CPP_REVIEW_MODEL", "claude-opus-4-0"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->model, "claude-opus-4-0");
}

// ─── CLI overrides ────────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, CliChecksOverrideDefault) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs cli{.checks = {{CheckCategory::ub}}};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->checks, ElementsAre(CheckCategory::ub));
}

TEST(ConfigLoaderTest, CliFailOnOverrideDefault) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs cli{.fail_on = {{Severity::critical}}};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->fail_on, ElementsAre(Severity::critical));
}

TEST(ConfigLoaderTest, CliOutputFormatOverrideDefault) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs cli{.output_format = OutputFormat::sarif};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->output_format, OutputFormat::sarif);
}

TEST(ConfigLoaderTest, CliOutputFileIsSet) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs cli{.output_file = "report.md"};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->output_file, std::make_optional<std::string>("report.md"));
}

TEST(ConfigLoaderTest, CliNoTelemetryWarningIsSet) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs cli{.no_telemetry_warning = true};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->no_telemetry_warning);
}

TEST(ConfigLoaderTest, CliInputPathsPassedThrough) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const CliArgs cli{.input_paths = {"src/a.cpp", "src/b.cpp"}};

    const auto result = ConfigLoader::load(cli);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->input_paths, ElementsAre("src/a.cpp", "src/b.cpp"));
}

// ─── JSON config file ─────────────────────────────────────────────────────────

TEST(ConfigLoaderTest, JsonConfigLoadsChecks) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"checks": ["memory"]})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->checks, ElementsAre(CheckCategory::memory));
}

TEST(ConfigLoaderTest, JsonConfigLoadsFailOn) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"fail_on": ["critical"]})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->fail_on, ElementsAre(Severity::critical));
}

TEST(ConfigLoaderTest, JsonConfigLoadsExcludedPaths) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"excluded_paths": ["build/", "third_party/"]})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->excluded_paths, ElementsAre("build/", "third_party/"));
}

TEST(ConfigLoaderTest, JsonConfigLoadsOutputFormat) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"output_format": "json"})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->output_format, OutputFormat::json);
}

TEST(ConfigLoaderTest, JsonConfigLoadsOutputFile) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"output_file": "review.md"})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->output_file, std::make_optional<std::string>("review.md"));
}

TEST(ConfigLoaderTest, JsonConfigLoadsDryRun) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", ""};
    const TempJsonFile tmp{R"({"dry_run": true})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->dry_run);
}

TEST(ConfigLoaderTest, JsonConfigLoadsNoTelemetryWarning) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"no_telemetry_warning": true})"};
    const CliArgs cli{};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->no_telemetry_warning);
}

TEST(ConfigLoaderTest, InvalidOutputFormatInJsonReturnsError) {
    const TempJsonFile tmp{R"({"output_format": "xml"})"};
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::invalid_output_format);
}

TEST(ConfigLoaderTest, CliOutputFormatOverridesJsonConfig) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"output_format": "json"})"};
    const CliArgs cli{.output_format = OutputFormat::sarif};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->output_format, OutputFormat::sarif);  // CLI wins
}

TEST(ConfigLoaderTest, CliChecksOverrideJsonConfig) {
    const EnvGuard _key{"ANTHROPIC_API_KEY", "test-key"};
    const TempJsonFile tmp{R"({"checks": ["memory"]})"};
    const CliArgs cli{.checks = {{CheckCategory::ub}}};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_TRUE(result.has_value());
    EXPECT_THAT(result->checks, ElementsAre(CheckCategory::ub));  // CLI wins
}

// ─── JSON config file — error cases ──────────────────────────────────────────

TEST(ConfigLoaderTest, InvalidJsonReturnsParseError) {
    const TempJsonFile tmp{"not valid json {{{"};
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::parse_error);
}

TEST(ConfigLoaderTest, InvalidCheckCategoryInJsonReturnsError) {
    const TempJsonFile tmp{R"({"checks": ["unknown_category"]})"};
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, tmp);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ConfigError::invalid_check_category);
}

TEST(ConfigLoaderTest, InvalidSeverityInJsonReturnsError) {
    const TempJsonFile tmp{R"({"fail_on": ["ultra"]})"};
    const CliArgs cli{.dry_run = true};

    const auto result = ConfigLoader::load(cli, tmp);

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
