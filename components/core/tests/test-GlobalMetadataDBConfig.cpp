#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <catch2/catch.hpp>

#include "../src/clp/GlobalMetadataDBConfig.hpp"

using namespace clp;

namespace {
// Constants
constexpr std::string_view cMySqlDbType{"mysql"};
constexpr std::string_view cMySqlDbHost{"test-host"};
constexpr std::string_view cMySqlDbPort{"8888"};
constexpr std::string_view cMySqlDbName{"test-db"};
constexpr std::string_view cMySqlDbTablePrefix{"test_prefix_"};
constexpr std::string_view cMySqlDbUser{"test-user"};
constexpr std::string_view cMySqlDbPass{"test-pass"};
constexpr std::array cCommonMySqlArgs{
        "test",
        "--db-type",
        cMySqlDbType.data(),
        "--db-host",
        cMySqlDbHost.data(),
        "--db-port",
        cMySqlDbPort.data(),
        "--db-name",
        cMySqlDbName.data(),
        "--db-table-prefix",
        cMySqlDbTablePrefix.data()
};
constexpr size_t cArgIdxDbHost{4};
constexpr size_t cArgIdxDbPort{6};
constexpr size_t cArgIdxDbName{8};
constexpr size_t cArgIdxDbTablePrefix{10};

constexpr std::string_view cSqliteDbType{"sqlite"};
constexpr std::array cCommonSqliteArgs{
        "test",
        "--db-type",
        cSqliteDbType.data(),

        // Placeholders for overwriting with MySQL-specific arguments like `--db-host`.
        "",
        "",
};
constexpr size_t cArgIdxOverwriteArgName{3};
constexpr size_t cArgIdxOverwriteArgValue{4};

void set_env_var(char const* name, char const* value) {
    // Silence the check since this class won't be used in a multithreaded context.
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    setenv(name, value, 1);
}

void unset_env_var(char const* name) {
    // Silence the check since this class won't be used in a multithreaded context.
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    unsetenv(name);
}

template <size_t n>
auto parse_args(std::array<char const*, n> const& argV) -> GlobalMetadataDBConfig {
    boost::program_options::options_description options_desc;
    GlobalMetadataDBConfig config{options_desc};

    boost::program_options::variables_map vm;
    constexpr auto cArgC{static_cast<int>(n)};
    boost::program_options::store(
            boost::program_options::parse_command_line(cArgC, argV.data(), options_desc),
            vm
    );
    boost::program_options::notify(vm);

    return config;
}
}  // namespace

TEST_CASE(
        "Test parsing command line arguments for GlobalMetadataDBConfig",
        "[GlobalMetadataDBConfig]"
) {
    auto const config{parse_args(cCommonMySqlArgs)};

    REQUIRE((config.get_metadata_db_type() == GlobalMetadataDBConfig::MetadataDBType::MySQL));
    REQUIRE((config.get_metadata_db_host() == cMySqlDbHost));
    REQUIRE((config.get_metadata_db_port() == std::stoi(std::string(cMySqlDbPort))));
    REQUIRE((config.get_metadata_db_name() == cMySqlDbName));
    REQUIRE((config.get_metadata_table_prefix() == cMySqlDbTablePrefix));
}

TEST_CASE("Test MySQL arguments and credential validation", "[GlobalMetadataDBConfig]") {
    SECTION("With all arguments") {
        auto config{parse_args(cCommonMySqlArgs)};

        SECTION("With valid credentials") {
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", cMySqlDbPass.data());

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE((config.get_metadata_db_username() == cMySqlDbUser));
            REQUIRE((config.get_metadata_db_password() == cMySqlDbPass));
        }

        SECTION("With empty password") {
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", "");

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE((config.get_metadata_db_username() == cMySqlDbUser));
            REQUIRE((config.get_metadata_db_password() == ""));
        }

        SECTION("With missing credentials") {
            SECTION("Neither username nor password") {
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
            SECTION("Password set but username missing") {
                set_env_var("CLP_DB_PASS", cMySqlDbPass.data());
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
            SECTION("Username set but password missing") {
                set_env_var("CLP_DB_USER", cMySqlDbUser.data());
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
        }
    }

    SECTION("With invalid port values") {
        SECTION("Port too low") {
            auto cArgV{cCommonMySqlArgs};
            cArgV[cArgIdxDbPort] = "0";
            auto config{parse_args(cArgV)};
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", cMySqlDbPass.data());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }

        SECTION("Port too high") {
            auto cArgV{cCommonMySqlArgs};
            cArgV[cArgIdxDbPort] = "65536";
            auto config{parse_args(cArgV)};
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", cMySqlDbPass.data());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }
    }

    SECTION("With empty required arguments") {
        SECTION("Empty db-host") {
            auto cArgV{cCommonMySqlArgs};
            cArgV[cArgIdxDbHost] = "";
            auto config{parse_args(cArgV)};
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", cMySqlDbPass.data());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }

        SECTION("Empty db-name") {
            auto cArgV{cCommonMySqlArgs};
            cArgV[cArgIdxDbName] = "";
            auto config{parse_args(cArgV)};
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", cMySqlDbPass.data());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }

        SECTION("Empty db-table-prefix") {
            auto cArgV{cCommonMySqlArgs};
            cArgV[cArgIdxDbTablePrefix] = "";
            auto config{parse_args(cArgV)};
            set_env_var("CLP_DB_USER", cMySqlDbUser.data());
            set_env_var("CLP_DB_PASS", cMySqlDbPass.data());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }
    }

    // Tear down.
    unset_env_var("CLP_DB_USER");
    unset_env_var("CLP_DB_PASS");
}

TEST_CASE("Test SQLite arguments", "[GlobalMetadataDBConfig]") {
    SECTION("With non-default db-host argument") {
        auto cArgV{cCommonSqliteArgs};
        cArgV[cArgIdxOverwriteArgName] = "--db-host";
        cArgV[cArgIdxOverwriteArgValue] = cMySqlDbHost.data();
        auto const config{parse_args(cArgV)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-port argument") {
        auto cArgV{cCommonSqliteArgs};
        cArgV[cArgIdxOverwriteArgName] = "--db-port";
        cArgV[cArgIdxOverwriteArgValue] = cMySqlDbPort.data();
        auto const config{parse_args(cArgV)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-name argument") {
        auto cArgV{cCommonSqliteArgs};
        cArgV[cArgIdxOverwriteArgName] = "--db-name";
        cArgV[cArgIdxOverwriteArgValue] = cMySqlDbName.data();
        auto const config{parse_args(cArgV)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-table-prefix argument") {
        auto cArgV{cCommonSqliteArgs};
        cArgV[cArgIdxOverwriteArgName] = "--db-table-prefix";
        cArgV[cArgIdxOverwriteArgValue] = cMySqlDbTablePrefix.data();
        auto const config{parse_args(cArgV)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With username and password") {
        auto config{parse_args(cCommonSqliteArgs)};

        set_env_var("CLP_DB_USER", cMySqlDbUser.data());
        set_env_var("CLP_DB_PASS", cMySqlDbPass.data());

        config.read_credentials_from_env_if_needed();
        REQUIRE_FALSE(config.get_metadata_db_username().has_value());
        REQUIRE_FALSE(config.get_metadata_db_password().has_value());
        REQUIRE_NOTHROW(config.validate());
    }

    // Tear down.
    unset_env_var("CLP_DB_USER");
    unset_env_var("CLP_DB_PASS");
}
