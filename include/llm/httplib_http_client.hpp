#ifndef CPP_REVIEW_LLM_HTTPLIB_HTTP_CLIENT_HPP
#define CPP_REVIEW_LLM_HTTPLIB_HTTP_CLIENT_HPP

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "llm/http_client.hpp"

namespace llm {

// Production HTTP client backed by cpp-httplib (SSL).
class HttplibSslClient final : public IHttpClient {
public:
    explicit HttplibSslClient(std::string_view host, int port, std::chrono::seconds timeout);

    [[nodiscard]] std::optional<HttpResponse> post(
        std::string_view               path,
        const std::vector<HttpHeader>& headers,
        const std::string&             body,
        std::string_view               content_type
    ) override;

private:
    std::string          m_host;
    int                  m_port;
    std::chrono::seconds m_timeout;
};

}  // namespace llm

#endif  // CPP_REVIEW_LLM_HTTPLIB_HTTP_CLIENT_HPP
