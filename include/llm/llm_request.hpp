#ifndef CPP_REVIEW_LLM_LLM_REQUEST_HPP
#define CPP_REVIEW_LLM_LLM_REQUEST_HPP

#include <cstdint>
#include <optional>
#include <string>

namespace llm {

struct LlmRequest {
    std::string system_prompt;
    std::string user_content;
    std::optional<std::string> model;  // nullopt → CPP_REVIEW_MODEL env or default
    std::optional<std::uint32_t> max_tokens;  // nullopt → model default
};

}  // namespace llm

#endif  // CPP_REVIEW_LLM_LLM_REQUEST_HPP
