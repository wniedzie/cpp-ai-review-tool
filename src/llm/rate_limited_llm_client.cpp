#include "llm/rate_limited_llm_client.hpp"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <thread>

namespace llm {

RateLimitedLlmClient::RateLimitedLlmClient(
    std::unique_ptr<LlmClient> inner, const TokenBucketConfig config
)
    : m_inner{std::move(inner)}
    , m_config{config}
    , m_available_tokens{config.burst_capacity}
    , m_last_refill{std::chrono::steady_clock::now()} {
    assert(m_inner != nullptr);
    assert(config.tokens_per_second > 0.0);
    assert(config.burst_capacity >= 1.0);
}

std::expected<LlmResponse, LlmError> RateLimitedLlmClient::complete(const LlmRequest& request) {
    std::chrono::duration<double> wait_duration{0};

    {
        const std::lock_guard lock{m_mutex};

        const auto now     = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(now - m_last_refill).count();

        m_available_tokens = std::min(
            m_config.burst_capacity, m_available_tokens + elapsed * m_config.tokens_per_second
        );
        m_last_refill = now;

        // Each API call costs one token; compute wait time while holding the lock
        // so concurrent callers each account for their own reservation.
        // Always decrement (possibly into negative) to reserve the token before
        // releasing the lock — this prevents multiple threads from computing the
        // same wait and all proceeding simultaneously.
        if (m_available_tokens < 1.0) {
            wait_duration = std::chrono::duration<double>{
                (1.0 - m_available_tokens) / m_config.tokens_per_second};
        }
        m_available_tokens -= 1.0;
    }

    if (wait_duration > std::chrono::duration<double>::zero()) {
        std::this_thread::sleep_for(wait_duration);
    }

    return m_inner->complete(request);
}

}  // namespace llm
