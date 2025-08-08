#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <catch2/catch.hpp>

#include "../src/clp/GlobalMetadataDBConfig.hpp"

using namespace clp;

namespace {
void set_env_var(char const* name, char const* value) {
    setenv(name, value, 1);  // NOLINT(concurrency-mt-unsafe)
}

void unset_env_var(char const* name) {
    unsetenv(name);  // NOLINT(concurrency-mt-unsafe)
}

template <size_t n>
auto parse_args(std::array<char const*, n> const& argv) -> GlobalMetadataDBConfig {
    boost::program_options::options_description options_desc;
    GlobalMetadataDBConfig config(options_desc);

    boost::program_options::variables_map vm;
    constexpr auto cArgc{static_cast<int>(n)};
    boost::program_options::store(
            boost::program_options::parse_command_line(
                    cArgc,
                    const_cast<char* const*>(argv.data()),
                    options_desc
            ),
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
    constexpr std::array cArgv{
            "test",
            "--db-type",
            "mysql",
            "--db-host",
            "test-host",
            "--db-port",
            "8888",
            "--db-name",
            "test-db",
            "--db-table-prefix",
            "test_prefix_"
    };
    auto const config{parse_args(cArgv)};

    REQUIRE((config.get_metadata_db_type() == GlobalMetadataDBConfig::MetadataDBType::MySQL));
    REQUIRE((config.get_metadata_db_host() == "test-host"));
    REQUIRE((config.get_metadata_db_port() == 8888));
    REQUIRE((config.get_metadata_db_name() == "test-db"));
    REQUIRE((config.get_metadata_table_prefix() == "test_prefix_"));
}

TEST_CASE("Test MySQL arguments and credential validation", "[GlobalMetadataDBConfig]") {
    SECTION("With all arguments") {
        constexpr std::array cArgv{
                "test",
                "--db-type",
                "mysql",
                "--db-host",
                "test-host",
                "--db-port",
                "8888",
                "--db-name",
                "test-db",
                "--db-table-prefix",
                "test_prefix_"
        };
        auto config{parse_args(cArgv)};

        SECTION("With valid credentials") {
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE((config.get_metadata_db_username() == "test-user"));
            REQUIRE((config.get_metadata_db_password() == "test-pass"));

            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("With empty password") {
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "");

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE((config.get_metadata_db_username() == "test-user"));
            REQUIRE((config.get_metadata_db_password() == ""));

            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("With missing credentials") {
            SECTION("Neither username nor password") {
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
            SECTION("Password set but username missing") {
                set_env_var("CLP_DB_PASS", "test-pass");
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
                unset_env_var("CLP_DB_PASS");
            }
            SECTION("Username set but password missing") {
                set_env_var("CLP_DB_USER", "test-user");
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
                unset_env_var("CLP_DB_USER");
            }
        }
    }

    SECTION("With invalid port values") {
        SECTION("Port too low") {
            constexpr std::array cArgvWithLowPort{
                    "test",
                    "--db-type",
                    "mysql",
                    "--db-host",
                    "test-host",
                    "--db-port",
                    "0",
                    "--db-name",
                    "test-db",
                    "--db-table-prefix",
                    "test_prefix_"
            };
            auto config{parse_args(cArgvWithLowPort)};
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("Port too high") {
            constexpr std::array cArgvWithHighPort{
                    "test",
                    "--db-type",
                    "mysql",
                    "--db-host",
                    "test-host",
                    "--db-port",
                    "65536",
                    "--db-name",
                    "test-db",
                    "--db-table-prefix",
                    "test_prefix_"
            };
            auto config{parse_args(cArgvWithHighPort)};
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }
    }

    SECTION("With empty required arguments") {
        SECTION("Empty db-host") {
            constexpr std::array cArgvEmptyHost{
                    "test",
                    "--db-type",
                    "mysql",
                    "--db-host",
                    "",
                    "--db-port",
                    "8888",
                    "--db-name",
                    "test-db",
                    "--db-table-prefix",
                    "test_prefix_"
            };
            auto config{parse_args(cArgvEmptyHost)};
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("Empty db-name") {
            constexpr std::array cArgvEmptyName{
                    "test",
                    "--db-type",
                    "mysql",
                    "--db-host",
                    "test-host",
                    "--db-port",
                    "8888",
                    "--db-name",
                    "",
                    "--db-table-prefix",
                    "test_prefix_"
            };
            auto config{parse_args(cArgvEmptyName)};
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("Empty db-table-prefix") {
            constexpr std::array cArgvEmptyPrefix{
                    "test",
                    "--db-type",
                    "mysql",
                    "--db-host",
                    "test-host",
                    "--db-port",
                    "8888",
                    "--db-name",
                    "test-db",
                    "--db-table-prefix",
                    ""
            };
            auto config{parse_args(cArgvEmptyPrefix)};
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }
    }
}

TEST_CASE("Test SQLite arguments", "[GlobalMetadataDBConfig]") {
    SECTION("With non-default db-host argument") {
        constexpr std::array cArgv{"test", "--db-type", "sqlite", "--db-host", "test-host"};
        auto const config{parse_args(cArgv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-port argument") {
        constexpr std::array cArgv{"test", "--db-type", "sqlite", "--db-port", "8888"};
        auto const config{parse_args(cArgv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-name argument") {
        constexpr std::array cArgv{"test", "--db-type", "sqlite", "--db-name", "test-db"};
        auto const config{parse_args(cArgv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-table-prefix argument") {
        constexpr std::array
                cArgv{"test", "--db-type", "sqlite", "--db-table-prefix", "test_prefix_"};
        auto const config{parse_args(cArgv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With username and password") {
        constexpr std::array cArgv{"test", "--db-type", "sqlite"};
        auto config{parse_args(cArgv)};

        set_env_var("CLP_DB_USER", "test-user");
        set_env_var("CLP_DB_PASS", "test-pass");

        config.read_credentials_from_env_if_needed();
        REQUIRE_FALSE(config.get_metadata_db_username().has_value());
        REQUIRE_FALSE(config.get_metadata_db_password().has_value());
        REQUIRE_NOTHROW(config.validate());

        unset_env_var("CLP_DB_USER");
        unset_env_var("CLP_DB_PASS");
    }
}
