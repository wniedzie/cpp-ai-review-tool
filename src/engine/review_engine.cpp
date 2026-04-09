#include "engine/review_engine.hpp"

#include <format>
#include <memory>
#include <numeric>
#include <string>
#include <string_view>
#include <utility>

#include "config/config.hpp"
#include "core/types.hpp"
#include "engine/system_prompt.hpp"
#include "llm/claude_llm_client.hpp"
#include "llm/llm_request.hpp"
#include "llm/llm_response.hpp"
#include "llm/rate_limited_llm_client.hpp"

namespace engine {

namespace detail {

// Anthropic guidance: ~1 request per 2 seconds sustained; allow short bursts.
// TODO(FR-08): make these parameters configurable from Config.
constexpr double tokens_per_second = 0.5;
constexpr double burst_capacity = 3.0;

// Rough token estimate: 1 token ≈ 4 characters (heuristic only, for dry-run).
constexpr std::size_t chars_per_token = 4;

[[nodiscard]] std::string format_checks(const std::vector<core::CheckCategory>& checks) {
    if (checks.empty()) {
        return {};
    }
    return std::accumulate(
        std::next(checks.begin()),
        checks.end(),
        std::string{core::to_string(checks.front())},
        [](std::string acc, const core::CheckCategory cat) {
            return acc + ", " + std::string{core::to_string(cat)};
        }
    );
}

}  // namespace detail

ReviewEngine::ReviewEngine(std::unique_ptr<llm::LlmClient> client, config::Config config)
    : client_{std::move(client)}
    , config_{std::move(config)} {}

std::expected<ReviewResult, llm::LlmError> ReviewEngine::run(const std::string_view source_code) {
    const auto checks_str = detail::format_checks(config_.checks);
    const auto system_prompt =
        std::vformat(system_prompt_template, std::make_format_args(checks_str));

    if (config_.dry_run) {
        const auto estimated_input =
            static_cast<std::uint32_t>(source_code.size() / detail::chars_per_token);
        return ReviewResult{.content = {}, .input_tokens = estimated_input, .output_tokens = 0};
    }

    const llm::LlmRequest request{
        .system_prompt = system_prompt,
        .user_content = std::string{source_code},
        .model = std::nullopt,
        .max_tokens = std::nullopt};

    return client_->complete(request).transform([](const llm::LlmResponse& response) {
        return ReviewResult{
            .content = response.content,
            .input_tokens = response.input_tokens,
            .output_tokens = response.output_tokens};
    });
}

std::expected<ReviewEngine, llm::LlmError> make_review_engine(const config::Config& config) {
    auto inner = std::make_unique<llm::ClaudeLlmClient>(config.api_key, config.model);
    auto client = std::make_unique<llm::RateLimitedLlmClient>(
        std::move(inner),
        llm::TokenBucketConfig{
            .tokens_per_second = detail::tokens_per_second,
            .burst_capacity = detail::burst_capacity}
    );
    return ReviewEngine{std::move(client), config};
}

}  // namespace engine
