#include "llm/httplib_http_client.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include <httplib.h>

namespace llm {

HttplibSslClient::HttplibSslClient(
    std::string_view host, const int port, const std::chrono::seconds timeout
)
    : m_host{host}
    , m_port{port}
    , m_timeout{timeout} {}

std::optional<HttpResponse> HttplibSslClient::post(
    const std::string_view         path,
    const std::vector<HttpHeader>& headers,
    const std::string&             body,
    const std::string_view         content_type
) {

    httplib::SSLClient client{m_host, m_port};
    client.set_read_timeout(m_timeout);
    client.set_connection_timeout(m_timeout);
    client.enable_server_certificate_verification(true);

    httplib::Headers httplib_headers;
    std::ranges::for_each(headers, [&](const HttpHeader& h) {
        httplib_headers.emplace(h.name, h.value);
    });

    const auto result =
        client.Post(std::string{path}, httplib_headers, body, std::string{content_type});
    if (!result) {
        return std::nullopt;
    }
    return HttpResponse{.status = result->status, .body = result->body};
}

}  // namespace llm
