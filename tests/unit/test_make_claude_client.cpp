#include <cstdlib>

#include <gtest/gtest.h>

#include "llm/claude_llm_client.hpp"
#include "llm/llm_error.hpp"

namespace llm::test {

// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace helpers {

void set_env(const char* name, const char* value) {
    ::setenv(name, value, /*overwrite=*/1);
}

void unset_env(const char* name) {
    ::unsetenv(name);
}

}  // namespace helpers

// ─── make_claude_client() ────────────────────────────────────────────────────

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyMissing) {
    helpers::unset_env("ANTHROPIC_API_KEY");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyEmpty) {
    helpers::set_env("ANTHROPIC_API_KEY", "");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyHasSpace) {
    helpers::set_env("ANTHROPIC_API_KEY", "sk- key");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyHasCRLF) {
    helpers::set_env("ANTHROPIC_API_KEY", "sk\r\nX-Evil-Header: injected");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyHasAtSign) {
    helpers::set_env("ANTHROPIC_API_KEY", "user@host");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnClientForMinimalAlphanumericKey) {
    helpers::set_env("ANTHROPIC_API_KEY", "abc123");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

TEST(MakeClaudeClientTest, ReturnClientForRealWorldKeyFormat) {
    helpers::set_env("ANTHROPIC_API_KEY", "sk-ant-api03_abc123-xyz");
    helpers::unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

TEST(MakeClaudeClientTest, ReturnClientWhenCppReviewModelEmpty) {
    helpers::set_env("ANTHROPIC_API_KEY", "abc123");
    helpers::set_env("CPP_REVIEW_MODEL", "");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

TEST(MakeClaudeClientTest, ReturnClientWhenCppReviewModelSet) {
    helpers::set_env("ANTHROPIC_API_KEY", "abc123");
    helpers::set_env("CPP_REVIEW_MODEL", "claude-opus-4");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

}  // namespace llm::test
