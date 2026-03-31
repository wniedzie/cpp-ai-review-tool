#ifndef CPP_REVIEW_LLM_LLM_CLIENT_HPP
#define CPP_REVIEW_LLM_LLM_CLIENT_HPP

#include <concepts>
#include <expected>

#include "llm/llm_error.hpp"
#include "llm/llm_request.hpp"
#include "llm/llm_response.hpp"

namespace llm {

class LlmClient {
public:
    virtual ~LlmClient() = default;

    [[nodiscard]] virtual std::expected<LlmResponse, LlmError> complete(const LlmRequest& request
    ) = 0;

    LlmClient(const LlmClient&)            = delete;
    LlmClient& operator=(const LlmClient&) = delete;
    LlmClient(LlmClient&&) noexcept        = default;  // needed so derived classes stay movable
    LlmClient& operator=(LlmClient&&)      = delete;

protected:
    LlmClient() = default;
};

// Supplemental concept — constrains templates without coupling to the ABC.
template <typename T>
concept LlmClientLike = requires(T& client, const LlmRequest& req) {
                            {
                                client.complete(req)
                                } -> std::same_as<std::expected<LlmResponse, LlmError>>;
                        };

}  // namespace llm

#endif  // CPP_REVIEW_LLM_LLM_CLIENT_HPP
