#include "llm/claude_llm_client.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "llm/httplib_http_client.hpp"

namespace llm {

namespace {

constexpr std::string_view     api_host           = "api.anthropic.com";
constexpr std::string_view     api_path           = "/v1/messages";
constexpr std::string_view     api_version        = "2023-06-01";
constexpr std::string_view     default_model      = "claude-sonnet-4-6";
constexpr std::uint32_t        default_max_tokens = 4096;
constexpr std::chrono::seconds request_timeout{30};

// Validates an API key against the character set used by Anthropic keys
// (alphanumeric, hyphens, underscores). Rejects anything containing CR/LF or
// other characters that could enable HTTP header injection.
[[nodiscard]] bool is_valid_api_key(const std::string_view key) noexcept {
    if (key.empty()) {
        return false;
    }
    return std::ranges::all_of(key, [](const unsigned char c) {
        return std::isalnum(c) != 0 || c == '-' || c == '_';
    });
}

[[nodiscard]] std::string build_request_body(const LlmRequest& request, const std::string& model) {
    nlohmann::json body = {
        {"model", model},
        {"max_tokens", request.max_tokens.value_or(default_max_tokens)},
        {"messages",
         nlohmann::json::array({{{"role", "user"}, {"content", request.user_content}}})}};

    if (!request.system_prompt.empty()) {
        body["system"] = request.system_prompt;
    }

    return body.dump();
}

[[nodiscard]] constexpr LlmError map_http_status(const int status_code) noexcept {
    switch (status_code) {
        case 400: return LlmError::InvalidRequest;
        case 401: [[fallthrough]];
        case 403: return LlmError::AuthFailure;
        case 429: return LlmError::RateLimited;
        case 413: return LlmError::ContextTooLarge;
        case 500: [[fallthrough]];
        case 529: return LlmError::ServerOverloaded;
        default: return LlmError::Unknown;
    }
}

[[nodiscard]] std::expected<LlmResponse, LlmError> parse_response_body(const std::string& body) {
    try {
        const auto  json    = nlohmann::json::parse(body);
        const auto& content = json.at("content");
        if (content.empty()) {
            return std::unexpected{LlmError::ParseError};
        }
        const auto& usage = json.at("usage");
        return LlmResponse{
            .content       = content.at(0).at("text").get<std::string>(),
            .stop_reason   = json.at("stop_reason").get<std::string>(),
            .input_tokens  = usage.at("input_tokens").get<std::uint32_t>(),
            .output_tokens = usage.at("output_tokens").get<std::uint32_t>()};
    } catch (const nlohmann::json::exception&) {
        return std::unexpected{LlmError::ParseError};
    }
}

[[nodiscard]] std::expected<LlmResponse, LlmError>
send_request(IHttpClient& http_client, const std::string& body, const std::string& api_key) {
    const std::vector<HttpHeader> headers = {
        {.name = "x-api-key", .value = api_key},
        {.name = "anthropic-version", .value = std::string{api_version}},
    };

    const auto result = http_client.post(api_path, headers, body, "application/json");

    if (!result) {
        return std::unexpected{LlmError::NetworkError};
    }
    if (result->status != 200) {
        return std::unexpected{map_http_status(result->status)};
    }

    return parse_response_body(result->body);
}

}  // anonymous namespace

// ─── ClaudeLlmClient ─────────────────────────────────────────────────────────

ClaudeLlmClient::ClaudeLlmClient(std::string api_key, std::string model)
    : m_api_key{std::move(api_key)}
    , m_model{std::move(model)}
    , m_http_client{std::make_unique<HttplibSslClient>(api_host, 443, request_timeout)} {}

ClaudeLlmClient::ClaudeLlmClient(
    std::string api_key, std::string model, std::unique_ptr<IHttpClient> http_client
)
    : m_api_key{std::move(api_key)}
    , m_model{std::move(model)}
    , m_http_client{std::move(http_client)} {}

std::expected<LlmResponse, LlmError> ClaudeLlmClient::complete(const LlmRequest& request) {
    const std::string& effective_model = request.model.has_value() ? *request.model : m_model;
    const auto         body            = build_request_body(request, effective_model);
    return send_request(*m_http_client, body, m_api_key);
}

// ─── Factory ─────────────────────────────────────────────────────────────────

// Returns AuthFailure if ANTHROPIC_API_KEY is missing, empty, or contains
// characters outside the allowed set (alphanumeric, hyphens, underscores).
std::expected<ClaudeLlmClient, LlmError> make_claude_client() {
    const auto* const api_key_env = std::getenv("ANTHROPIC_API_KEY");
    if (api_key_env == nullptr || std::string_view{api_key_env}.empty()) {
        return std::unexpected{LlmError::AuthFailure};
    }

    if (!is_valid_api_key(api_key_env)) {
        return std::unexpected{LlmError::AuthFailure};
    }

    const auto* const model_env = std::getenv("CPP_REVIEW_MODEL");
    const auto        model     = (model_env != nullptr && !std::string_view{model_env}.empty())
                                      ? std::string{model_env}
                                      : std::string{default_model};

    return ClaudeLlmClient{std::string{api_key_env}, std::move(model)};
}

}  // namespace llm
