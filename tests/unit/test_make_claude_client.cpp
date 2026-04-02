#include <cstdlib>

#include <gtest/gtest.h>

#include "llm/claude_llm_client.hpp"
#include "llm/llm_error.hpp"

namespace llm::test {

// ─── Helpers ──────────────────────────────────────────────────────────────────

namespace {

void set_env(const char* name, const char* value) {
    ::setenv(name, value, /*overwrite=*/1);
}

void unset_env(const char* name) {
    ::unsetenv(name);
}

}  // namespace

// ─── make_claude_client() ────────────────────────────────────────────────────

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyMissing) {
    unset_env("ANTHROPIC_API_KEY");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyEmpty) {
    set_env("ANTHROPIC_API_KEY", "");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyHasSpace) {
    set_env("ANTHROPIC_API_KEY", "sk- key");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyHasCRLF) {
    set_env("ANTHROPIC_API_KEY", "sk\r\nX-Evil-Header: injected");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnAuthFailureWhenApiKeyHasAtSign) {
    set_env("ANTHROPIC_API_KEY", "user@host");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::AuthFailure);
}

TEST(MakeClaudeClientTest, ReturnClientForMinimalAlphanumericKey) {
    set_env("ANTHROPIC_API_KEY", "abc123");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

TEST(MakeClaudeClientTest, ReturnClientForRealWorldKeyFormat) {
    set_env("ANTHROPIC_API_KEY", "sk-ant-api03_abc123-xyz");
    unset_env("CPP_REVIEW_MODEL");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

TEST(MakeClaudeClientTest, ReturnClientWhenCppReviewModelEmpty) {
    set_env("ANTHROPIC_API_KEY", "abc123");
    set_env("CPP_REVIEW_MODEL", "");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

TEST(MakeClaudeClientTest, ReturnClientWhenCppReviewModelSet) {
    set_env("ANTHROPIC_API_KEY", "abc123");
    set_env("CPP_REVIEW_MODEL", "claude-opus-4");

    const auto result = make_claude_client();

    EXPECT_TRUE(result.has_value());
}

}  // namespace llm::test
