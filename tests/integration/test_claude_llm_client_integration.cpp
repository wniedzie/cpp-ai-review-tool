#include <cstdlib>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "llm/claude_llm_client.hpp"
#include "llm/llm_error.hpp"
#include "llm/llm_request.hpp"

// ─── Integration tests ────────────────────────────────────────────────────────
// These tests hit the real Anthropic API.
// They are only compiled when CMake is configured with -DINTEGRATION_TESTS=ON.
// Each test skips itself at runtime when ANTHROPIC_API_KEY is not set, so the
// test binary can also be used in CI environments without credentials.

namespace llm::test {

namespace {

bool api_key_available() {
    const auto* const key = std::getenv("ANTHROPIC_API_KEY");
    return key != nullptr && !std::string_view{key}.empty();
}

}  // namespace

// ─── Fixture ──────────────────────────────────────────────────────────────────

class ClaudeLlmClientIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!api_key_available()) {
            GTEST_SKIP() << "ANTHROPIC_API_KEY not set — skipping integration test";
        }
    }
};

// ─── Tests ────────────────────────────────────────────────────────────────────

TEST_F(ClaudeLlmClientIntegrationTest, RealApiReturnsSuccessfulResponse) {
    auto client_result = make_claude_client();
    ASSERT_TRUE(client_result.has_value())
        << "make_claude_client() failed: " << llm::to_string(client_result.error());

    auto& client = *client_result;

    const LlmRequest request{
        .system_prompt = "You are a concise assistant. Reply with exactly one word.",
        .user_content  = "Say the word: hello"};

    const auto result = client.complete(request);

    ASSERT_TRUE(result.has_value()) << "complete() failed: " << llm::to_string(result.error());
    EXPECT_FALSE(result->content.empty());
    EXPECT_FALSE(result->stop_reason.empty());
    EXPECT_GT(result->input_tokens, 0U);
    EXPECT_GT(result->output_tokens, 0U);
}

TEST_F(ClaudeLlmClientIntegrationTest, RealApiWithInvalidKeyReturnsAuthFailure) {
    // Construct directly with a syntactically valid but wrong key.
    ClaudeLlmClient client{"invalid-key-but-valid-chars", "claude-sonnet-4-6"};

    const LlmRequest request{.system_prompt = "", .user_content = "ping"};
    const auto       result = client.complete(request);

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

}  // namespace llm::test
