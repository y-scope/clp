#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#include <clp_s/InputConfig.hpp>

namespace {
constexpr std::string_view cS3Url{
        "https://test-bucket.s3.us-east-2.amazonaws.com/archives/archive-id"
};
constexpr std::string_view cEnvAccessKeyId{"ENV_ACCESS_KEY_ID"};
constexpr std::string_view cProviderAccessKeyId{"PROVIDER_ACCESS_KEY_ID"};
constexpr std::string_view cProviderSessionToken{"provider-session-token"};

/**
 * Sets or unsets an environment variable, restoring its original value on destruction.
 */
class ScopedEnvironmentVariable {
public:
    // Constructors
    ScopedEnvironmentVariable(char const* name, std::optional<std::string> const& value)
            : m_name{name} {
        if (auto const* original_value{std::getenv(name)}; nullptr != original_value) {
            m_original_value = original_value;
        }
        REQUIRE(set(value));
    }

    // Disable copy/move constructors and assignment operators
    ScopedEnvironmentVariable(ScopedEnvironmentVariable const&) = delete;
    ScopedEnvironmentVariable(ScopedEnvironmentVariable&&) = delete;
    auto operator=(ScopedEnvironmentVariable const&) -> ScopedEnvironmentVariable& = delete;
    auto operator=(ScopedEnvironmentVariable&&) -> ScopedEnvironmentVariable& = delete;

    // Destructor
    ~ScopedEnvironmentVariable() { std::ignore = set(m_original_value); }

private:
    /**
     * @param value The value to set, or `std::nullopt` to unset the variable.
     * @return Whether the variable was updated successfully.
     */
    [[nodiscard]] auto set(std::optional<std::string> const& value) const -> bool {
        if (value.has_value()) {
            return 0 == setenv(m_name, value->c_str(), 1);
        }
        return 0 == unsetenv(m_name);
    }

    char const* m_name;
    std::optional<std::string> m_original_value;
};

/**
 * Exception thrown by test credential providers.
 */
class CredentialsProviderError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @param url
 * @param name
 * @return The value of the query parameter in `url` with the given name, or `std::nullopt` if it
 * doesn't exist.
 */
auto get_query_parameter(std::string_view url, std::string_view name)
        -> std::optional<std::string_view> {
    auto const query_begin{url.find('?')};
    if (std::string_view::npos == query_begin) {
        return std::nullopt;
    }
    auto query{url.substr(query_begin + 1)};
    while (false == query.empty()) {
        auto const parameter_end{query.find('&')};
        auto const parameter{query.substr(0, parameter_end)};
        if (parameter.starts_with(name) && parameter.size() > name.size()
            && '=' == parameter.at(name.size()))
        {
            return parameter.substr(name.size() + 1);
        }
        if (std::string_view::npos == parameter_end) {
            break;
        }
        query.remove_prefix(parameter_end + 1);
    }
    return std::nullopt;
}

/**
 * @param url
 * @return The access key ID in the `X-Amz-Credential` query parameter of `url`.
 */
auto get_access_key_id(std::string_view url) -> std::string_view {
    auto const credential{get_query_parameter(url, "X-Amz-Credential")};
    REQUIRE(credential.has_value());
    // The parameter is `<access-key-id>/<scope>` with '/' percent-encoded.
    auto const access_key_id_end{credential->find("%2F")};
    REQUIRE(std::string_view::npos != access_key_id_end);
    return credential->substr(0, access_key_id_end);
}

/**
 * @param credentials
 * @param num_calls Incremented each time the returned provider is called.
 * @return A credentials provider that returns `credentials`.
 */
auto make_credentials_provider(std::optional<clp_s::AwsCredentials> credentials, size_t& num_calls)
        -> clp_s::AwsCredentialsProvider {
    return [credentials{std::move(credentials)}, &num_calls]() {
        ++num_calls;
        return credentials;
    };
}
}  // namespace

TEST_CASE("input_config_remove_prefix", "[clp-s][input-config]") {
    SECTION("Path prefix removal works as expected.") {
        std::vector<std::tuple<std::string, std::string, std::string>> path_prefix_result_tuples{
                {"abcd/efg", "abcd", "/efg"},
                {"abcd/efg", "abcd/", "/efg"},
                {"/abcd/efg", "/abcd", "/efg"},
                {"/abcd/efg", "/abcd/", "/efg"},
                {"abcd/../efg", "abcd/", "/../efg"},
                {"./efg", "./", "/efg"},
                {"/abcd/efg", "", "/abcd/efg"},
                {"/abcd/efg", "/", "/abcd/efg"}
        };
        for (auto const& [path, prefix, expected_result] : path_prefix_result_tuples) {
            CAPTURE(path);
            CAPTURE(prefix);
            auto const result_option{clp_s::remove_path_prefix(path, prefix)};
            REQUIRE(result_option.has_value());
            REQUIRE(result_option.value() == expected_result);
        }
    }

    SECTION("Invalid path prefix removal fails as expected.") {
        std::vector<std::tuple<std::string, std::string>> path_prefix_result_tuples{
                {"/abcd/efg", "/hijk"},
                {"abcd/efg", "hijk"},
                {"/a", "/a/b"}
        };
        for (auto const& [path, prefix] : path_prefix_result_tuples) {
            CAPTURE(path);
            CAPTURE(prefix);
            auto const result_option{clp_s::remove_path_prefix(path, prefix)};
            REQUIRE_FALSE(result_option.has_value());
        }
    }
}

TEST_CASE("input_config_authenticate_url", "[clp-s][input-config]") {
    // Valid environment credentials make any fallback to them observable.
    ScopedEnvironmentVariable const access_key_id{
            clp_s::cAwsAccessKeyIdEnvVar,
            std::string{cEnvAccessKeyId}
    };
    ScopedEnvironmentVariable const secret_access_key{
            clp_s::cAwsSecretAccessKeyEnvVar,
            "env-secret-access-key"
    };
    ScopedEnvironmentVariable const session_token{clp_s::cAwsSessionTokenEnvVar, std::nullopt};

    clp_s::AwsCredentials const provider_credentials{
            .access_key_id = std::string{cProviderAccessKeyId},
            .secret_access_key = "provider-secret-access-key",
            .session_token = std::string{cProviderSessionToken}
    };
    clp_s::Path const network_path{
            .source = clp_s::InputSource::Network,
            .path = std::string{cS3Url}
    };

    SECTION("URLs that don't require authentication are unchanged.") {
        size_t num_provider_calls{0};
        clp_s::NetworkAuthOption const network_auth{
                .method = clp_s::AuthMethod::None,
                .aws_credentials_provider
                = make_credentials_provider(provider_credentials, num_provider_calls)
        };
        REQUIRE(cS3Url == clp_s::try_authenticate_url(cS3Url, network_auth));
        REQUIRE(0ULL == num_provider_calls);
    }

    SECTION("Without a provider, URLs are presigned using environment credentials.") {
        clp_s::NetworkAuthOption const network_auth{.method = clp_s::AuthMethod::S3PresignedUrlV4};
        auto const url{clp_s::try_authenticate_url(cS3Url, network_auth)};
        REQUIRE(url.has_value());
        REQUIRE(cEnvAccessKeyId == get_access_key_id(url.value()));
        REQUIRE(false == get_query_parameter(url.value(), "X-Amz-Security-Token").has_value());
        REQUIRE("86400" == get_query_parameter(url.value(), "X-Amz-Expires"));
    }

    SECTION("Without a provider, missing environment credentials are an error.") {
        ScopedEnvironmentVariable const unset_access_key_id{
                clp_s::cAwsAccessKeyIdEnvVar,
                std::nullopt
        };
        clp_s::NetworkAuthOption const network_auth{.method = clp_s::AuthMethod::S3PresignedUrlV4};
        REQUIRE(false == clp_s::try_authenticate_url(cS3Url, network_auth).has_value());
    }

    SECTION("A provider supplies the credentials for every request.") {
        size_t num_provider_calls{0};
        clp_s::NetworkAuthOption const network_auth{
                .method = clp_s::AuthMethod::S3PresignedUrlV4,
                .aws_credentials_provider
                = make_credentials_provider(provider_credentials, num_provider_calls)
        };
        constexpr size_t cNumRequests{3};
        for (size_t i{0}; i < cNumRequests; ++i) {
            auto const url{clp_s::try_authenticate_url(cS3Url, network_auth)};
            REQUIRE(url.has_value());
            REQUIRE(cProviderAccessKeyId == get_access_key_id(url.value()));
            REQUIRE(cProviderSessionToken
                    == get_query_parameter(url.value(), "X-Amz-Security-Token"));
            REQUIRE("86400" == get_query_parameter(url.value(), "X-Amz-Expires"));
        }
        REQUIRE(cNumRequests == num_provider_calls);
    }

    SECTION("Provider failures don't fall back to environment credentials.") {
        auto const make_credentials = [&](auto const& modify) {
            auto credentials{provider_credentials};
            modify(credentials);
            return std::optional<clp_s::AwsCredentials>{std::move(credentials)};
        };
        std::vector<std::pair<std::string_view, std::optional<clp_s::AwsCredentials>>> const
                failures{
                        {"no credentials", std::nullopt},
                        {"empty access key ID",
                         make_credentials([](clp_s::AwsCredentials& credentials) {
                             credentials.access_key_id.clear();
                         })},
                        {"empty secret access key",
                         make_credentials([](clp_s::AwsCredentials& credentials) {
                             credentials.secret_access_key.clear();
                         })},
                        {"empty session token",
                         make_credentials([](clp_s::AwsCredentials& credentials) {
                             credentials.session_token = std::string{};
                         })}
                };
        for (auto const& [description, credentials] : failures) {
            CAPTURE(description);
            size_t num_provider_calls{0};
            clp_s::NetworkAuthOption const network_auth{
                    .method = clp_s::AuthMethod::S3PresignedUrlV4,
                    .aws_credentials_provider
                    = make_credentials_provider(credentials, num_provider_calls)
            };
            REQUIRE(false == clp_s::try_authenticate_url(cS3Url, network_auth).has_value());
            REQUIRE(nullptr == clp_s::try_create_reader(network_path, network_auth));
            REQUIRE(2ULL == num_provider_calls);
        }
    }

    SECTION("Provider exceptions propagate to the caller.") {
        clp_s::NetworkAuthOption const network_auth{
                .method = clp_s::AuthMethod::S3PresignedUrlV4,
                .aws_credentials_provider = []() -> std::optional<clp_s::AwsCredentials> {
                    throw CredentialsProviderError{"Failed to refresh credentials."};
                }
        };
        REQUIRE_THROWS_AS(
                clp_s::try_authenticate_url(cS3Url, network_auth),
                CredentialsProviderError
        );
        REQUIRE_THROWS_AS(
                clp_s::try_create_reader(network_path, network_auth),
                CredentialsProviderError
        );
    }

    SECTION("The presigned URL expire time is configurable.") {
        std::vector<std::pair<std::chrono::seconds, std::optional<std::string_view>>> const
                expire_times_and_parameters{
                        {std::chrono::minutes{15}, "900"},
                        {std::chrono::seconds{0}, std::nullopt},
                        {std::chrono::days{8}, std::nullopt}
                };
        for (auto const& [expire_time, expected_parameter] : expire_times_and_parameters) {
            CAPTURE(expire_time.count());
            size_t num_provider_calls{0};
            clp_s::NetworkAuthOption const provider_auth{
                    .method = clp_s::AuthMethod::S3PresignedUrlV4,
                    .aws_credentials_provider
                    = make_credentials_provider(provider_credentials, num_provider_calls),
                    .presigned_url_expire_time = expire_time
            };
            clp_s::NetworkAuthOption const env_auth{
                    .method = clp_s::AuthMethod::S3PresignedUrlV4,
                    .presigned_url_expire_time = expire_time
            };
            for (auto const& network_auth : {provider_auth, env_auth}) {
                auto const url{clp_s::try_authenticate_url(cS3Url, network_auth)};
                if (expected_parameter.has_value()) {
                    REQUIRE(url.has_value());
                    REQUIRE(expected_parameter
                            == get_query_parameter(url.value(), "X-Amz-Expires"));
                } else {
                    REQUIRE(false == url.has_value());
                }
            }
        }
    }
}
