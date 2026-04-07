#ifndef CPP_REVIEW_LLM_LLM_ERROR_HPP
#define CPP_REVIEW_LLM_LLM_ERROR_HPP

#include <cstdint>
#include <string_view>

namespace llm {

enum class LlmError : std::uint8_t {
    AuthFailure,  // invalid or missing ANTHROPIC_API_KEY
    RateLimited,  // HTTP 429 from API
    NetworkError,  // connection failure or timeout
    ParseError,  // malformed API response body
    InvalidRequest,  // HTTP 400 — bad model name or malformed body
    ContextTooLarge,  // input exceeds model context window
    ServerOverloaded,  // HTTP 500 / 529 — retriable server error
    Unknown
};

[[nodiscard]] constexpr std::string_view to_string(const LlmError error) noexcept {
    switch (error) {
        case LlmError::AuthFailure: return "authentication failure (check ANTHROPIC_API_KEY)";
        case LlmError::RateLimited: return "rate limited by API (HTTP 429)";
        case LlmError::NetworkError: return "network error (connection failure or timeout)";
        case LlmError::ParseError: return "failed to parse API response";
        case LlmError::InvalidRequest: return "invalid request (HTTP 400)";
        case LlmError::ContextTooLarge: return "input exceeds model context window";
        case LlmError::ServerOverloaded: return "server overloaded (retriable)";
        case LlmError::Unknown: return "unknown error";
    }
    return "unknown error";
}

}  // namespace llm

#endif  // CPP_REVIEW_LLM_LLM_ERROR_HPP
