#ifndef CPP_REVIEW_LLM_LLM_RESPONSE_HPP
#define CPP_REVIEW_LLM_LLM_RESPONSE_HPP

#include <cstdint>
#include <string>

namespace llm {

struct LlmResponse {
    std::string   content;
    std::uint32_t input_tokens{};   // reported by API; used for rate-limiting and dry-run cost
    std::uint32_t output_tokens{};
};

} // namespace llm

#endif // CPP_REVIEW_LLM_LLM_RESPONSE_HPP
