#include "llm/httplib_http_client.hpp"

#include <algorithm>
#include <string>
#include <string_view>

#include <httplib.h>

namespace llm {

HttplibSslClient::HttplibSslClient(
    std::string_view host, const int port, const std::chrono::seconds timeout
)
    : host_{host}
    , port_{port}
    , timeout_{timeout} {}

std::optional<HttpResponse> HttplibSslClient::post(
    const std::string_view path,
    const std::vector<HttpHeader>& headers,
    const std::string& body,
    const std::string_view content_type
) {

    httplib::SSLClient client{host_, port_};
    client.set_read_timeout(timeout_);
    client.set_connection_timeout(timeout_);
    client.enable_server_certificate_verification(true);

    httplib::Headers httplib_headers;
    std::ranges::for_each(headers, [&](const HttpHeader& header) {
        httplib_headers.emplace(header.name, header.value);
    });

    const auto result =
        client.Post(std::string{path}, httplib_headers, body, std::string{content_type});
    if (!result) {
        return std::nullopt;
    }
    return HttpResponse{.status = result->status, .body = result->body};
}

}  // namespace llm
