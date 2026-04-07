#ifndef CPP_REVIEW_LLM_HTTP_CLIENT_HPP
#define CPP_REVIEW_LLM_HTTP_CLIENT_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace llm {

struct HttpHeader {
    std::string name;
    std::string value;
};

struct HttpResponse {
    int status{};
    std::string body;
};

// Minimal HTTP client interface used by ClaudeLlmClient.
// Decoupled from any concrete HTTP library so it can be replaced in tests.
class IHttpClient {
public:
    virtual ~IHttpClient() = default;

    [[nodiscard]] virtual std::optional<HttpResponse> post(
        std::string_view path,
        const std::vector<HttpHeader>& headers,
        const std::string& body,
        std::string_view content_type
    ) = 0;

    IHttpClient(const IHttpClient&) = delete;
    IHttpClient& operator=(const IHttpClient&) = delete;
    IHttpClient(IHttpClient&&) = delete;
    IHttpClient& operator=(IHttpClient&&) = delete;

protected:
    IHttpClient() = default;
};

}  // namespace llm

#endif  // CPP_REVIEW_LLM_HTTP_CLIENT_HPP
