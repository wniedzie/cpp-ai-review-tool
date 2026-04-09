#include "llm/rate_limited_llm_client.hpp"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <thread>

namespace llm {

RateLimitedLlmClient::RateLimitedLlmClient(
    std::unique_ptr<LlmClient> inner, const TokenBucketConfig config
)
    : inner_{std::move(inner)}
    , config_{config}
    , available_tokens_{config.burst_capacity}
    , last_refill_{std::chrono::steady_clock::now()} {
    assert(inner_ != nullptr);
    assert(config.tokens_per_second > 0.0);
    assert(config.burst_capacity >= 1.0);
}

std::expected<LlmResponse, LlmError> RateLimitedLlmClient::complete(const LlmRequest& request) {
    std::chrono::duration<double> wait_duration{0};

    {
        const std::lock_guard lock{mutex_};

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration<double>(now - last_refill_).count();

        available_tokens_ = std::min(
            config_.burst_capacity, available_tokens_ + (elapsed * config_.tokens_per_second)
        );
        last_refill_ = now;

        // Each API call costs one token; compute wait time while holding the lock
        // so concurrent callers each account for their own reservation.
        // Always decrement (possibly into negative) to reserve the token before
        // releasing the lock — this prevents multiple threads from computing the
        // same wait and all proceeding simultaneously.
        if (available_tokens_ < 1.0) {
            wait_duration = std::chrono::duration<double>{
                (1.0 - available_tokens_) / config_.tokens_per_second};
        }
        available_tokens_ -= 1.0;
    }

    if (wait_duration > std::chrono::duration<double>::zero()) {
        std::this_thread::sleep_for(wait_duration);
    }

    return inner_->complete(request);
}

}  // namespace llm
