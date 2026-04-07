#ifndef CPP_REVIEW_CORE_HASH_HPP
#define CPP_REVIEW_CORE_HASH_HPP

#include <cstddef>
#include <string_view>

namespace core {

struct Fnv1aHash {
    static constexpr std::size_t offset = 2166136261U;
    static constexpr std::size_t prime = 16777619U;

    template <typename Iterator>
    constexpr std::size_t operator()(Iterator first, Iterator last) const noexcept {
        std::size_t hash{offset};
        for (auto character = first; character != last; ++character) {  // NOLINT
            hash ^= static_cast<std::size_t>(*character);
            hash *= prime;
        }
        return hash;
    }
};

[[nodiscard]] constexpr std::size_t hash(std::string_view str) noexcept {
    return Fnv1aHash{}(str.begin(), str.end());
}

[[nodiscard]] constexpr std::size_t operator""_h(const char* str, std::size_t len) noexcept {
    return hash(std::string_view{str, len});
}

}  // namespace core

#endif  // CPP_REVIEW_CORE_HASH_HPP
