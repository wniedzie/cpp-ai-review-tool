#include "llm/rate_limited_llm_client.hpp"

#include <algorithm>
#include <thread>

namespace llm {

RateLimitedLlmClient::RateLimitedLlmClient(
    std::unique_ptr<LlmClient> inner,
    const TokenBucketConfig    config
)
    : m_inner{std::move(inner)}
    , m_config{config}
    , m_available_tokens{config.burst_capacity}
    , m_last_refill{std::chrono::steady_clock::now()}
{}

std::expected<LlmResponse, LlmError>
RateLimitedLlmClient::complete(const LlmRequest& request) {
    const auto now     = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double>(now - m_last_refill).count();

    m_available_tokens = std::min(
        m_config.burst_capacity,
        m_available_tokens + elapsed * m_config.tokens_per_second
    );
    m_last_refill = now;

    // Each API call costs one token; block until a token is available.
    if (m_available_tokens < 1.0) {
        const double wait_seconds = (1.0 - m_available_tokens) / m_config.tokens_per_second;
        std::this_thread::sleep_for(std::chrono::duration<double>{wait_seconds});
        m_available_tokens = 0.0;
    } else {
        m_available_tokens -= 1.0;
    }

    return m_inner->complete(request);
}

} // namespace llm
