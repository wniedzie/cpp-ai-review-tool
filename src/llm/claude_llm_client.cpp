#include "llm/claude_llm_client.hpp"

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>      // workaround: system Asio <=1.18.1 omits this in awaitable.hpp

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <nlohmann/json.hpp>

namespace llm {

namespace {

constexpr std::string_view k_default_model      = "claude-sonnet-4-6";
constexpr std::string_view k_api_host           = "api.anthropic.com";
constexpr std::string_view k_api_path           = "/v1/messages";
constexpr std::string_view k_api_version        = "2023-06-01";
constexpr std::uint32_t    k_default_max_tokens = 4096;

[[nodiscard]] std::string build_request_body(
    const LlmRequest&  request,
    const std::string& model
) {
    const nlohmann::json body = {
        {"model",      model},
        {"max_tokens", request.max_tokens.value_or(k_default_max_tokens)},
        {"system",     request.system_prompt},
        {"messages",   nlohmann::json::array({
            {{"role", "user"}, {"content", request.user_content}}
        })}
    };
    return body.dump();
}

[[nodiscard]] std::string build_http_request(
    const std::string& body,
    const std::string& api_key
) {
    return std::format(
        "POST {} HTTP/1.1\r\n"
        "Host: {}\r\n"
        "Content-Type: application/json\r\n"
        "x-api-key: {}\r\n"
        "anthropic-version: {}\r\n"
        "Content-Length: {}\r\n"
        "Connection: close\r\n"
        "\r\n"
        "{}",
        k_api_path, k_api_host, api_key, k_api_version, body.size(), body
    );
}

// Decodes an HTTP/1.1 chunked transfer-encoded body into a flat string.
// std::from_chars stops at the first non-hex character, so chunk extensions
// (e.g. "7f;name=value") are handled correctly without extra filtering.
[[nodiscard]] std::expected<std::string, LlmError>
decode_chunked(const std::string_view data) {
    std::string result;
    result.reserve(data.size());
    auto remaining = data;

    while (!remaining.empty()) {
        const auto chunk_size_end = remaining.find("\r\n");
        if (chunk_size_end == std::string_view::npos) {
            return std::unexpected{LlmError::ParseError};
        }

        const auto chunk_size_str = remaining.substr(0, chunk_size_end);
        std::size_t chunk_size{};
        const auto [ptr, ec] = std::from_chars(
            chunk_size_str.data(),
            chunk_size_str.data() + chunk_size_str.size(),
            chunk_size,
            16
        );
        if (ec != std::errc{}) {
            return std::unexpected{LlmError::ParseError};
        }

        if (chunk_size == 0) {
            break;  // terminal chunk
        }

        remaining.remove_prefix(chunk_size_end + 2);  // skip size line + CRLF
        if (remaining.size() < chunk_size + 2) {
            return std::unexpected{LlmError::ParseError};
        }

        result.append(remaining.substr(0, chunk_size));
        remaining.remove_prefix(chunk_size + 2);  // skip chunk data + CRLF
    }

    return result;
}

struct HttpResponse {
    int         status_code{};
    std::string body;
};

[[nodiscard]] std::expected<HttpResponse, LlmError>
parse_http_response(const std::string& raw) {
    const auto header_end = raw.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        return std::unexpected{LlmError::ParseError};
    }

    const std::string_view headers{raw.data(), header_end};

    // Parse HTTP status code from "HTTP/1.1 <code> <reason>"
    const auto first_space = headers.find(' ');
    if (first_space == std::string_view::npos) {
        return std::unexpected{LlmError::ParseError};
    }

    int status_code{};
    const auto code_view   = headers.substr(first_space + 1, 3);
    const auto [ptr, ec]   = std::from_chars(
        code_view.data(), code_view.data() + code_view.size(), status_code
    );
    if (ec != std::errc{}) {
        return std::unexpected{LlmError::ParseError};
    }

    const std::string_view body_view{
        raw.data() + header_end + 4,
        raw.size() - header_end - 4
    };

    const auto is_chunked =
        headers.find("Transfer-Encoding: chunked") != std::string_view::npos;

    if (is_chunked) {
        return decode_chunked(body_view)
            .transform([status_code](std::string decoded) {
                return HttpResponse{status_code, std::move(decoded)};
            });
    }

    return HttpResponse{status_code, std::string{body_view}};
}

[[nodiscard]] constexpr LlmError map_http_status(const int status_code) noexcept {
    switch (status_code) {
        case 401: [[fallthrough]];
        case 403: return LlmError::AuthFailure;
        case 429: return LlmError::RateLimited;
        case 413: return LlmError::ContextTooLarge;
        default:  return LlmError::Unknown;
    }
}

[[nodiscard]] std::expected<LlmResponse, LlmError>
parse_response_body(const std::string& body) {
    try {
        const auto  json    = nlohmann::json::parse(body);
        const auto& content = json.at("content");
        if (content.empty()) {
            return std::unexpected{LlmError::ParseError};
        }
        const auto& usage = json.at("usage");
        return LlmResponse{
            .content       = content.at(0).at("text").get<std::string>(),
            .input_tokens  = usage.at("input_tokens").get<std::uint32_t>(),
            .output_tokens = usage.at("output_tokens").get<std::uint32_t>()
        };
    } catch (const nlohmann::json::exception&) {
        return std::unexpected{LlmError::ParseError};
    }
}

[[nodiscard]] std::expected<std::string, LlmError>
send_https_request(const std::string& request_str) {
    try {
        asio::io_context io_context;

        asio::ssl::context ssl_ctx{asio::ssl::context::tls_client};
        ssl_ctx.set_default_verify_paths();
        ssl_ctx.set_verify_mode(asio::ssl::verify_peer);

        using SslStream = asio::ssl::stream<asio::ip::tcp::socket>;
        SslStream ssl_stream{io_context, ssl_ctx};

        const auto host_str = std::string{k_api_host};

        asio::ip::tcp::resolver resolver{io_context};
        const auto endpoints = resolver.resolve(host_str, "https");
        asio::connect(ssl_stream.lowest_layer(), endpoints);
        ssl_stream.lowest_layer().set_option(asio::ip::tcp::no_delay{true});

        // Verify server certificate against the requested hostname
        ssl_stream.set_verify_callback(asio::ssl::host_name_verification{host_str});
        ssl_stream.handshake(SslStream::client);

        asio::write(ssl_stream, asio::buffer(request_str));

        std::string response;
        asio::error_code read_ec;
        asio::read(ssl_stream, asio::dynamic_buffer(response), read_ec);

        // EOF is expected: server closes the TCP connection after sending the
        // response with "Connection: close".
        if (read_ec && read_ec != asio::error::eof) {
            return std::unexpected{LlmError::NetworkError};
        }

        return response;
    } catch (const std::exception&) {
        return std::unexpected{LlmError::NetworkError};
    }
}

} // anonymous namespace

// ─── ClaudeLlmClient ─────────────────────────────────────────────────────────

ClaudeLlmClient::ClaudeLlmClient(std::string api_key, std::string model)
    : m_api_key{std::move(api_key)}
    , m_model{std::move(model)}
{}

std::expected<LlmResponse, LlmError>
ClaudeLlmClient::complete(const LlmRequest& request) {
    const auto effective_model = request.model.value_or(m_model);
    const auto body            = build_request_body(request, effective_model);
    const auto http_req        = build_http_request(body, m_api_key);

    return send_https_request(http_req)
        .and_then(parse_http_response)
        .and_then([](HttpResponse resp) -> std::expected<LlmResponse, LlmError> {
            if (resp.status_code != 200) {
                return std::unexpected{map_http_status(resp.status_code)};
            }
            return parse_response_body(resp.body);
        });
}

// ─── Factory ─────────────────────────────────────────────────────────────────

std::expected<ClaudeLlmClient, LlmError> make_claude_client() {
    const auto* const api_key_env = std::getenv("ANTHROPIC_API_KEY");
    if (api_key_env == nullptr || std::string_view{api_key_env}.empty()) {
        return std::unexpected{LlmError::AuthFailure};
    }

    const auto* const model_env = std::getenv("CPP_REVIEW_MODEL");
    const auto model = (model_env != nullptr && !std::string_view{model_env}.empty())
        ? std::string{model_env}
        : std::string{k_default_model};

    return ClaudeLlmClient{std::string{api_key_env}, std::move(model)};
}

} // namespace llm
