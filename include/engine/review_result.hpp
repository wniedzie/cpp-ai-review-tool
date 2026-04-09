#ifndef CPP_REVIEW_ENGINE_REVIEW_RESULT_HPP
#define CPP_REVIEW_ENGINE_REVIEW_RESULT_HPP

#include <cstdint>
#include <string>

namespace engine {

struct ReviewResult {
    std::string content;
    std::uint32_t input_tokens{};
    std::uint32_t output_tokens{};
};

}  // namespace engine

#endif  // CPP_REVIEW_ENGINE_REVIEW_RESULT_HPP
