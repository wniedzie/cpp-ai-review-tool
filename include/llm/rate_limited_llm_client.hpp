#ifndef CPP_REVIEW_LLM_RATE_LIMITED_LLM_CLIENT_HPP
#define CPP_REVIEW_LLM_RATE_LIMITED_LLM_CLIENT_HPP

#include <chrono>
#include <memory>

#include "llm/llm_client.hpp"

namespace llm {

struct TokenBucketConfig {
    double tokens_per_second;  // sustained refill rate
    double burst_capacity;     // maximum tokens allowed to accumulate
};

// Decorator: wraps any LlmClient and throttles calls via a token bucket (FR-08.1).
class RateLimitedLlmClient final : public LlmClient {
public:
    explicit RateLimitedLlmClient(
        std::unique_ptr<LlmClient> inner,
        TokenBucketConfig          config
    );

    [[nodiscard]] std::expected<LlmResponse, LlmError>
    complete(const LlmRequest& request) override;

private:
    std::unique_ptr<LlmClient>            m_inner;
    TokenBucketConfig                     m_config;
    double                                m_available_tokens;
    std::chrono::steady_clock::time_point m_last_refill;
};

} // namespace llm

#endif // CPP_REVIEW_LLM_RATE_LIMITED_LLM_CLIENT_HPP
