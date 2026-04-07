#ifndef CPP_REVIEW_LLM_CLAUDE_LLM_CLIENT_HPP
#define CPP_REVIEW_LLM_CLAUDE_LLM_CLIENT_HPP

#include <expected>
#include <memory>
#include <string>

#include "llm/http_client.hpp"
#include "llm/llm_client.hpp"

namespace llm {

class ClaudeLlmClient final : public LlmClient {
public:
    // Production constructor: uses the real HTTPS transport.
    explicit ClaudeLlmClient(std::string api_key, std::string model);

    // Injection constructor: accepts any IHttpClient (used in unit tests).
    explicit ClaudeLlmClient(
        std::string api_key, std::string model, std::unique_ptr<IHttpClient> http_client
    );

    [[nodiscard]] std::expected<LlmResponse, LlmError> complete(const LlmRequest& request) override;

private:
    std::string m_api_key;
    std::string m_model;
    std::unique_ptr<IHttpClient> m_http_client{nullptr};
};

// Factory — reads ANTHROPIC_API_KEY and CPP_REVIEW_MODEL environment variables.
// Returns LlmError::AuthFailure if ANTHROPIC_API_KEY is absent or empty.
[[nodiscard]] std::expected<ClaudeLlmClient, LlmError> make_claude_client();

}  // namespace llm

#endif  // CPP_REVIEW_LLM_CLAUDE_LLM_CLIENT_HPP
