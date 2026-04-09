#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "config/config.hpp"
#include "core/types.hpp"
#include "engine/review_engine.hpp"
#include "engine/review_result.hpp"
#include "engine/system_prompt.hpp"
#include "llm/llm_client.hpp"
#include "llm/llm_error.hpp"
#include "llm/llm_request.hpp"
#include "llm/llm_response.hpp"

namespace engine::test {

using ::testing::_;
using ::testing::Return;

// ─── Mock ─────────────────────────────────────────────────────────────────────

class MockLlmClient : public llm::LlmClient {
public:
    MOCK_METHOD(
        (std::expected<llm::LlmResponse, llm::LlmError>),
        complete,
        (const llm::LlmRequest& request),
        (override)
    );
};

// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace helpers {

config::Config make_config(
    std::vector<core::CheckCategory> checks = {
        core::CheckCategory::ub,
        core::CheckCategory::memory,
        core::CheckCategory::modernization},
    const bool dry_run = false
) {
    return config::Config{
        .api_key = "test-key",
        .model = "claude-test",
        .checks = std::move(checks),
        .fail_on = {core::Severity::high, core::Severity::critical},
        .output_format = core::OutputFormat::markdown,
        .output_file = std::nullopt,
        .dry_run = dry_run,
        .no_telemetry_warning = false,
        .input_paths = {},
        .excluded_paths = {}};
}

}  // namespace helpers

// ─── Tests ────────────────────────────────────────────────────────────────────

TEST(ReviewEngineTest, RunMapsLlmResponseToReviewResult) {
    auto mock = std::make_unique<MockLlmClient>();
    const auto expected = llm::LlmResponse{
        .content = "## Review\n\nNo issues found.",
        .stop_reason = "end_turn",
        .input_tokens = 120,
        .output_tokens = 40};
    EXPECT_CALL(*mock, complete(_)).WillOnce(Return(expected));

    auto engine = ReviewEngine{std::move(mock), helpers::make_config()};
    const auto result = engine.run("int main() { return 0; }");

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->content, "## Review\n\nNo issues found.");
    EXPECT_EQ(result->input_tokens, 120);
    EXPECT_EQ(result->output_tokens, 40);
}

TEST(ReviewEngineTest, RunPassesSourceCodeAsUserContent) {
    auto mock = std::make_unique<MockLlmClient>();
    constexpr std::string_view source = "void foo() {}";

    EXPECT_CALL(*mock, complete(::testing::Field(&llm::LlmRequest::user_content, std::string{source})))
        .WillOnce(Return(llm::LlmResponse{
            .content = "", .stop_reason = "end_turn", .input_tokens = 5, .output_tokens = 1}));

    auto engine = ReviewEngine{std::move(mock), helpers::make_config()};
    const auto result = engine.run(source);

    ASSERT_TRUE(result.has_value());
}

TEST(ReviewEngineTest, RunSystemPromptContainsAllActiveChecks) {
    auto mock = std::make_unique<MockLlmClient>();

    EXPECT_CALL(
        *mock,
        complete(::testing::Field(
            &llm::LlmRequest::system_prompt,
            ::testing::AllOf(
                ::testing::HasSubstr("ub"),
                ::testing::HasSubstr("memory"),
                ::testing::HasSubstr("modernization")
            )
        ))
    ).WillOnce(Return(llm::LlmResponse{
        .content = "", .stop_reason = "end_turn", .input_tokens = 10, .output_tokens = 5}));

    auto engine = ReviewEngine{std::move(mock), helpers::make_config()};
    ASSERT_TRUE(engine.run("int x;").has_value());
}

TEST(ReviewEngineTest, RunSystemPromptContainsOnlyEnabledChecks) {
    auto mock = std::make_unique<MockLlmClient>();

    // When only "ub" is requested the injected categories string should be
    // exactly "ub" — the system prompt will contain "check categories: ub."
    // but must not list memory or modernization in that injection sentence.
    EXPECT_CALL(
        *mock,
        complete(::testing::Field(
            &llm::LlmRequest::system_prompt,
            ::testing::AllOf(
                ::testing::HasSubstr("check categories: ub."),
                ::testing::Not(::testing::HasSubstr("check categories: ub, memory")),
                ::testing::Not(::testing::HasSubstr("check categories: ub, modernization"))
            )
        ))
    ).WillOnce(Return(llm::LlmResponse{
        .content = "", .stop_reason = "end_turn", .input_tokens = 10, .output_tokens = 5}));

    auto engine = ReviewEngine{
        std::move(mock), helpers::make_config({core::CheckCategory::ub})};
    ASSERT_TRUE(engine.run("int x;").has_value());
}

TEST(ReviewEngineTest, RunPropagatesLlmError) {
    auto mock = std::make_unique<MockLlmClient>();
    EXPECT_CALL(*mock, complete(_))
        .WillOnce(Return(std::unexpected{llm::LlmError::NetworkError}));

    auto engine = ReviewEngine{std::move(mock), helpers::make_config()};
    const auto result = engine.run("int main() {}");

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), llm::LlmError::NetworkError);
}

TEST(ReviewEngineTest, DryRunSkipsClientAndReturnsTokenEstimate) {
    auto mock = std::make_unique<MockLlmClient>();
    EXPECT_CALL(*mock, complete(_)).Times(0);

    auto engine = ReviewEngine{std::move(mock), helpers::make_config({}, /*dry_run=*/true)};
    constexpr std::string_view source = "int main() { return 0; }";  // 24 chars → ~6 tokens
    const auto result = engine.run(source);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->content.empty());
    EXPECT_EQ(result->input_tokens, static_cast<std::uint32_t>(source.size() / 4));
    EXPECT_EQ(result->output_tokens, 0);
}

TEST(ReviewEngineTest, SystemPromptTemplateContainsPlaceholder) {
    EXPECT_THAT(std::string{system_prompt_template}, ::testing::HasSubstr("{0}"));
}

}  // namespace engine::test
