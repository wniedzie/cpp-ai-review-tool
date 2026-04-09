#ifndef CPP_REVIEW_ENGINE_REVIEW_ENGINE_HPP
#define CPP_REVIEW_ENGINE_REVIEW_ENGINE_HPP

#include <expected>
#include <memory>
#include <string_view>

#include "config/config.hpp"
#include "engine/review_result.hpp"
#include "llm/llm_client.hpp"
#include "llm/llm_error.hpp"

namespace engine {

class ReviewEngine {
public:
    explicit ReviewEngine(std::unique_ptr<llm::LlmClient> client, config::Config config);

    [[nodiscard]] std::expected<ReviewResult, llm::LlmError> run(std::string_view source_code);

    ReviewEngine(const ReviewEngine&) = delete;
    ReviewEngine& operator=(const ReviewEngine&) = delete;
    ReviewEngine(ReviewEngine&&) noexcept = default;
    ReviewEngine& operator=(ReviewEngine&&) = delete;

private:
    std::unique_ptr<llm::LlmClient> client_;
    config::Config config_;
};

// Factory — constructs a production ClaudeLlmClient (wrapped with rate limiting)
// from the resolved configuration. Returns LlmError::AuthFailure if the API key
// is absent or malformed (validated by ConfigLoader before reaching here, but
// checked again defensively by ClaudeLlmClient).
[[nodiscard]] std::expected<ReviewEngine, llm::LlmError>
make_review_engine(const config::Config& config);

}  // namespace engine

#endif  // CPP_REVIEW_ENGINE_REVIEW_ENGINE_HPP
