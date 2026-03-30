#ifndef CPP_REVIEW_LLM_CLAUDE_LLM_CLIENT_HPP
#define CPP_REVIEW_LLM_CLAUDE_LLM_CLIENT_HPP

#include <expected>
#include <string>

#include "llm/llm_client.hpp"

namespace llm {

class ClaudeLlmClient final : public LlmClient {
public:
    explicit ClaudeLlmClient(std::string api_key, std::string model);

    [[nodiscard]] std::expected<LlmResponse, LlmError>
    complete(const LlmRequest& request) override;

private:
    std::string m_api_key;
    std::string m_model;
};

// Factory — reads ANTHROPIC_API_KEY and CPP_REVIEW_MODEL environment variables.
// Returns LlmError::k_auth_failure if ANTHROPIC_API_KEY is absent or empty.
[[nodiscard]] std::expected<ClaudeLlmClient, LlmError> make_claude_client();

} // namespace llm

#endif // CPP_REVIEW_LLM_CLAUDE_LLM_CLIENT_HPP
