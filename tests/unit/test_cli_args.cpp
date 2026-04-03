#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>
#include <unistd.h>

#include "cli/cli_args.hpp"

namespace {

// Owns string data and exposes a stable const char* const* pointer array.
// argv[0] is always "cpp-review". Strings are fully built before ptrs are taken
// so no reallocation invalidates pointers.
struct Argv {
    explicit Argv(std::initializer_list<std::string> args) {
        strings.reserve(args.size() + 1);
        strings.emplace_back("cpp-review");
        for (const auto& arg : args) {
            strings.push_back(arg);
        }
        ptrs.reserve(strings.size());
        for (const auto& string : strings) {
            ptrs.push_back(string.c_str());
        }
    }

    [[nodiscard]] int count() const {
        return static_cast<int>(ptrs.size());
    }
    [[nodiscard]] const char* const* data() const {
        return ptrs.data();
    }

    std::vector<std::string> strings;
    std::vector<const char*> ptrs;
};

// Creates a temporary file and returns its path. The file is removed in the
// fixture's TearDown so individual tests stay simple.
class CliArgsTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto*       info        = ::testing::UnitTest::GetInstance()->current_test_info();
        const std::string unique_name = std::string("cpp_review_") + info->test_suite_name() + "_" +
                                        info->name() + "_" + std::to_string(getpid()) + ".cpp";
        temp_file_ = std::filesystem::temp_directory_path() / unique_name;
        std::ofstream{temp_file_};  // create empty file
    }

    void TearDown() override {
        std::filesystem::remove(temp_file_);
    }

    std::filesystem::path temp_file_;
};

// ── Default args ──────────────────────────────────────────────────────────────

TEST_F(CliArgsTest, DefaultsAreApplied) {
    const Argv argv({temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->input_path, temp_file_);
    EXPECT_EQ(
        result->checks,
        (std::set<cli::CheckCategory>{
            cli::CheckCategory::ub, cli::CheckCategory::memory, cli::CheckCategory::modernization})
    );
    EXPECT_EQ(
        result->fail_on, (std::set<cli::Severity>{cli::Severity::high, cli::Severity::critical})
    );
    EXPECT_EQ(result->format, cli::OutputFormat::markdown);
    EXPECT_FALSE(result->output_file.has_value());
    EXPECT_FALSE(result->dry_run);
    EXPECT_FALSE(result->no_telemetry_warning);
}

// ── --checks ──────────────────────────────────────────────────────────────────

TEST_F(CliArgsTest, ChecksUbMemory) {
    const Argv argv({"--checks=ub,memory", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
        result->checks,
        (std::set<cli::CheckCategory>{cli::CheckCategory::ub, cli::CheckCategory::memory})
    );
}

TEST_F(CliArgsTest, ChecksModernizationOnly) {
    const Argv argv({"--checks=modernization", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->checks, (std::set<cli::CheckCategory>{cli::CheckCategory::modernization}));
}

TEST_F(CliArgsTest, ChecksUnknownValueReturnsError) {
    const Argv argv({"--checks=bad_value", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("bad_value"), std::string::npos);
}

// ── --fail-on ─────────────────────────────────────────────────────────────────

TEST_F(CliArgsTest, FailOnCriticalOnly) {
    const Argv argv({"--fail-on=critical", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->fail_on, (std::set<cli::Severity>{cli::Severity::critical}));
}

TEST_F(CliArgsTest, FailOnAllLevelsBelow) {
    const Argv argv({"--fail-on=low,medium,high,critical", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
        result->fail_on,
        (std::set<cli::Severity>{
            cli::Severity::low, cli::Severity::medium, cli::Severity::high, cli::Severity::critical}
        )
    );
}

TEST_F(CliArgsTest, FailOnUnknownValueReturnsError) {
    const Argv argv({"--fail-on=invalid", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("invalid"), std::string::npos);
}

// ── --format ──────────────────────────────────────────────────────────────────

TEST_F(CliArgsTest, FormatJson) {
    const Argv argv({"--format=json", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->format, cli::OutputFormat::json);
}

TEST_F(CliArgsTest, FormatSarif) {
    const Argv argv({"--format=sarif", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->format, cli::OutputFormat::sarif);
}

TEST_F(CliArgsTest, FormatUnknownReturnsError) {
    const Argv argv({"--format=invalid", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("invalid"), std::string::npos);
}

// ── --output ──────────────────────────────────────────────────────────────────

TEST_F(CliArgsTest, OutputFileIsSet) {
    const Argv argv({"--output=out.md", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    ASSERT_TRUE(result->output_file.has_value());
    EXPECT_EQ(*result->output_file, std::filesystem::path{"out.md"});
}

// ── --dry-run ─────────────────────────────────────────────────────────────────

TEST_F(CliArgsTest, DryRunFlag) {
    const Argv argv({"--dry-run", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->dry_run);
}

// ── --no-telemetry-warning ────────────────────────────────────────────────────

TEST_F(CliArgsTest, NoTelemetryWarningFlag) {
    const Argv argv({"--no-telemetry-warning", temp_file_.string()});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->no_telemetry_warning);
}

// ── Missing / invalid path ────────────────────────────────────────────────────

TEST_F(CliArgsTest, MissingPositionalArgReturnsError) {
    const Argv argv({});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_FALSE(result.has_value());
}

TEST_F(CliArgsTest, NonExistentPathReturnsError) {
    const Argv argv({"/absolutely/does/not/exist/file.cpp"});
    const auto result = cli::parse_args(argv.count(), argv.data());

    ASSERT_FALSE(result.has_value());
    EXPECT_NE(result.error().find("does not exist"), std::string::npos);
}

}  // namespace
