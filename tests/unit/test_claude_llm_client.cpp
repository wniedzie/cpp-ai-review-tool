#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include "llm/claude_llm_client.hpp"
#include "llm/http_client.hpp"
#include "llm/llm_error.hpp"
#include "llm/llm_request.hpp"
#include "llm/llm_response.hpp"

namespace llm::test {

using ::testing::_;
using ::testing::Return;

// ─── Mock ─────────────────────────────────────────────────────────────────────

class MockHttpClient : public IHttpClient {
public:
    MOCK_METHOD(
        std::optional<HttpResponse>,
        post,
        (std::string_view               path,
         const std::vector<HttpHeader>& headers,
         const std::string&             body,
         std::string_view               content_type),
        (override)
    );
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

namespace {

// Minimal well-formed Anthropic API response body.
std::string make_success_body(
    const std::string& text     = "Hello",
    const std::string& stop     = "end_turn",
    std::uint32_t      in_toks  = 10,
    std::uint32_t      out_toks = 5
) {
    return nlohmann::json{
        {"content", {{{"type", "text"}, {"text", text}}}},
        {"stop_reason", stop},
        {"usage", {{"input_tokens", in_toks}, {"output_tokens", out_toks}}}}
        .dump();
}

}  // namespace

// ─── Fixture: ClaudeLlmClientTest ─────────────────────────────────────────────
// TEST_F is used for tests that share the same fixture structure but are
// otherwise independent — each test gets a freshly constructed client + mock.

class ClaudeLlmClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto mock_owner = std::make_unique<MockHttpClient>();
        mock_           = mock_owner.get();
        client_ =
            std::make_unique<ClaudeLlmClient>("valid-key", "test-model", std::move(mock_owner));
    }

    // unique_ptr avoids the need for move-assignment (which LlmClient deletes).
    std::unique_ptr<ClaudeLlmClient> client_;
    MockHttpClient*                  mock_{nullptr};

    static LlmRequest minimal_request() {
        return LlmRequest{.system_prompt = "", .user_content = "Hello"};
    }
};

// ─── Happy-path response parsing ─────────────────────────────────────────────

TEST_F(ClaudeLlmClientTest, SuccessfulResponseParsed) {
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce(Return(HttpResponse{
            .status = 200, .body = make_success_body("Answer", "end_turn", 8, 3)}));

    const auto result = client_->complete(minimal_request());

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->content, "Answer");
    EXPECT_EQ(result->stop_reason, "end_turn");
    EXPECT_EQ(result->input_tokens, 8U);
    EXPECT_EQ(result->output_tokens, 3U);
}

// ─── Request body construction ────────────────────────────────────────────────

TEST_F(ClaudeLlmClientTest, SystemPromptIncludedInBody) {
    std::string captured_body;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string& body,
                      std::string_view) {
            captured_body = body;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    LlmRequest request    = minimal_request();
    request.system_prompt = "Act as a C++ expert.";
    std::ignore           = client_->complete(request);

    const auto json = nlohmann::json::parse(captured_body);
    EXPECT_TRUE(json.contains("system"));
    EXPECT_EQ(json.at("system").get<std::string>(), "Act as a C++ expert.");
}

TEST_F(ClaudeLlmClientTest, EmptySystemPromptOmittedFromBody) {
    std::string captured_body;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string& body,
                      std::string_view) {
            captured_body = body;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    std::ignore = client_->complete(minimal_request());

    const auto json = nlohmann::json::parse(captured_body);
    EXPECT_FALSE(json.contains("system"));
}

TEST_F(ClaudeLlmClientTest, RequestModelOverridesClientModel) {
    std::string captured_body;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string& body,
                      std::string_view) {
            captured_body = body;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    LlmRequest request = minimal_request();
    request.model      = "claude-haiku";
    std::ignore        = client_->complete(request);

    const auto json = nlohmann::json::parse(captured_body);
    EXPECT_EQ(json.at("model").get<std::string>(), "claude-haiku");
}

TEST_F(ClaudeLlmClientTest, ClientModelUsedWhenRequestModelAbsent) {
    auto                             mock_owner = std::make_unique<MockHttpClient>();
    auto*                            local_mock = mock_owner.get();
    std::unique_ptr<ClaudeLlmClient> client =
        std::make_unique<ClaudeLlmClient>("valid-key", "my-custom-model", std::move(mock_owner));

    std::string captured_body;
    EXPECT_CALL(*local_mock, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string& body,
                      std::string_view) {
            captured_body = body;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    LlmRequest request = minimal_request();
    // request.model intentionally absent (nullopt)
    std::ignore = client->complete(request);

    const auto json = nlohmann::json::parse(captured_body);
    EXPECT_EQ(json.at("model").get<std::string>(), "my-custom-model");
}

TEST_F(ClaudeLlmClientTest, MaxTokensDefaultedTo4096) {
    std::string captured_body;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string& body,
                      std::string_view) {
            captured_body = body;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    // request.max_tokens is nullopt
    std::ignore = client_->complete(minimal_request());

    const auto json = nlohmann::json::parse(captured_body);
    EXPECT_EQ(json.at("max_tokens").get<std::uint32_t>(), 4096U);
}

TEST_F(ClaudeLlmClientTest, MaxTokensCustomValue) {
    std::string captured_body;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string& body,
                      std::string_view) {
            captured_body = body;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    LlmRequest request = minimal_request();
    request.max_tokens = 100U;
    std::ignore        = client_->complete(request);

    const auto json = nlohmann::json::parse(captured_body);
    EXPECT_EQ(json.at("max_tokens").get<std::uint32_t>(), 100U);
}

// ─── Header assertions ────────────────────────────────────────────────────────

TEST_F(ClaudeLlmClientTest, ApiKeyPassedInHeader) {
    auto                             mock_owner = std::make_unique<MockHttpClient>();
    auto*                            local_mock = mock_owner.get();
    std::unique_ptr<ClaudeLlmClient> client =
        std::make_unique<ClaudeLlmClient>("my-secret-key", "test-model", std::move(mock_owner));

    std::vector<HttpHeader> captured_headers;
    EXPECT_CALL(*local_mock, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>& headers,
                      const std::string&,
                      std::string_view) {
            captured_headers = headers;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    std::ignore = client->complete(minimal_request());

    const auto pos = std::ranges::find_if(captured_headers, [](const HttpHeader& hdr) {
        return hdr.name == "x-api-key";
    });
    ASSERT_NE(pos, captured_headers.end());
    EXPECT_EQ(pos->value, "my-secret-key");
}

TEST_F(ClaudeLlmClientTest, AnthropicVersionHeaderPresent) {
    std::vector<HttpHeader> captured_headers;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>& headers,
                      const std::string&,
                      std::string_view) {
            captured_headers = headers;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    std::ignore = client_->complete(minimal_request());

    const auto pos = std::ranges::find_if(captured_headers, [](const HttpHeader& hdr) {
        return hdr.name == "anthropic-version";
    });
    ASSERT_NE(pos, captured_headers.end());
    EXPECT_EQ(pos->value, "2023-06-01");
}

TEST_F(ClaudeLlmClientTest, ContentTypeHeaderIsApplicationJson) {
    std::string captured_ct;
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce([&](std::string_view,
                      const std::vector<HttpHeader>&,
                      const std::string&,
                      std::string_view ctype) {
            captured_ct = ctype;
            return HttpResponse{.status = 200, .body = make_success_body()};
        });

    std::ignore = client_->complete(minimal_request());

    EXPECT_EQ(captured_ct, "application/json");
}

// ─── HTTP → LlmError mapping (parameterised) ─────────────────────────────────
// TEST_P avoids code duplication across all error-status variants.

struct HttpStatusMapping {
    int      http_status;
    LlmError expected_error;
};

class HttpStatusMappingTest : public ::testing::TestWithParam<HttpStatusMapping> {
protected:
    void SetUp() override {
        auto mock_owner = std::make_unique<MockHttpClient>();
        mock_           = mock_owner.get();
        client_ =
            std::make_unique<ClaudeLlmClient>("valid-key", "test-model", std::move(mock_owner));
    }

    std::unique_ptr<ClaudeLlmClient> client_;
    MockHttpClient*                  mock_{nullptr};
};

TEST_P(HttpStatusMappingTest, MapsHttpStatusToLlmError) {
    const auto [http_status, expected_error] = GetParam();

    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce(Return(HttpResponse{.status = http_status, .body = ""}));

    const auto result = client_->complete(LlmRequest{.system_prompt = "", .user_content = "ping"});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), expected_error);
}

INSTANTIATE_TEST_SUITE_P(
    HttpStatusToLlmError,
    HttpStatusMappingTest,
    ::testing::Values(
        HttpStatusMapping{400, LlmError::InvalidRequest},
        HttpStatusMapping{401, LlmError::AuthFailure},
        HttpStatusMapping{403, LlmError::AuthFailure},
        HttpStatusMapping{429, LlmError::RateLimited},
        HttpStatusMapping{413, LlmError::ContextTooLarge},
        HttpStatusMapping{500, LlmError::ServerOverloaded},
        HttpStatusMapping{529, LlmError::ServerOverloaded},
        HttpStatusMapping{418, LlmError::Unknown}
    ),
    [](const ::testing::TestParamInfo<HttpStatusMapping>& info) {
        return "Http" + std::to_string(info.param.http_status);
    }
);

// ─── Response body parsing errors (parameterised) ────────────────────────────

struct MalformedBodyCase {
    const char* name;
    std::string body;
};

class MalformedResponseBodyTest : public ::testing::TestWithParam<MalformedBodyCase> {
protected:
    void SetUp() override {
        auto mock_owner = std::make_unique<MockHttpClient>();
        mock_           = mock_owner.get();
        client_ =
            std::make_unique<ClaudeLlmClient>("valid-key", "test-model", std::move(mock_owner));
    }

    std::unique_ptr<ClaudeLlmClient> client_;
    MockHttpClient*                  mock_{nullptr};
};

TEST_P(MalformedResponseBodyTest, ReturnsParseError) {
    EXPECT_CALL(*mock_, post(_, _, _, _))
        .WillOnce(Return(HttpResponse{.status = 200, .body = GetParam().body}));

    const auto result = client_->complete(LlmRequest{.system_prompt = "", .user_content = "ping"});

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::ParseError);
}

INSTANTIATE_TEST_SUITE_P(
    MalformedBodies,
    MalformedResponseBodyTest,
    ::testing::Values(
        MalformedBodyCase{
            "EmptyContentArray",
            nlohmann::json{
                {"content", nlohmann::json::array()},
                {"stop_reason", "end_turn"},
                {"usage", {{"input_tokens", 1}, {"output_tokens", 1}}}}
                .dump()},
        MalformedBodyCase{
            "MissingContentKey",
            nlohmann::json{
                {"stop_reason", "end_turn"}, {"usage", {{"input_tokens", 1}, {"output_tokens", 1}}}}
                .dump()},
        MalformedBodyCase{
            "MissingUsageKey",
            nlohmann::json{
                {"content", {{{"type", "text"}, {"text", "hi"}}}}, {"stop_reason", "end_turn"}}
                .dump()},
        MalformedBodyCase{
            "MissingTextField",
            nlohmann::json{
                {"content", {{{"type", "text"}}}},
                {"stop_reason", "end_turn"},
                {"usage", {{"input_tokens", 1}, {"output_tokens", 1}}}}
                .dump()},
        MalformedBodyCase{"NotJson", "not json at all"}
    ),
    [](const ::testing::TestParamInfo<MalformedBodyCase>& info) {
        return std::string{info.param.name};
    }
);

// ─── Network failure ──────────────────────────────────────────────────────────

TEST_F(ClaudeLlmClientTest, NulloptFromHttpClientReturnsNetworkError) {
    EXPECT_CALL(*mock_, post(_, _, _, _)).WillOnce(Return(std::nullopt));

    const auto result = client_->complete(minimal_request());

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), LlmError::NetworkError);
}

}  // namespace llm::test
