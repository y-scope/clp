#include <cstdlib>

#include <catch2/catch.hpp>

#include "../src/clp/GlobalMetadataDBConfig.hpp"

using namespace clp;

namespace {
void set_env_var(char const* name, char const* value) {
    setenv(name, value, 1);
}

void unset_env_var(char const* name) {
    unsetenv(name);
}

template <size_t N>
auto parse_args(std::array<char const*, N> const& argv, GlobalMetadataDBConfig& config) -> void {
    boost::program_options::options_description options_desc;
    config.add_command_line_options(options_desc);

    boost::program_options::variables_map vm;
    constexpr auto argc{static_cast<int>(N)};
    boost::program_options::store(
            boost::program_options::parse_command_line(
                    argc,
                    const_cast<char* const*>(argv.data()),
                    options_desc
            ),
            vm
    );
    boost::program_options::notify(vm);
}
}  // namespace

TEST_CASE(
        "Test parsing command line arguments for GlobalMetadataDBConfig",
        "[GlobalMetadataDBConfig]"
) {
    constexpr std::array argv{
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
    GlobalMetadataDBConfig config;
    parse_args(argv, config);

    REQUIRE(config.get_metadata_db_type() == GlobalMetadataDBConfig::MetadataDBType::MySQL);
    REQUIRE(config.get_metadata_db_host() == "test-host");
    REQUIRE(config.get_metadata_db_port() == 8888);
    REQUIRE(config.get_metadata_db_name() == "test-db");
    REQUIRE(config.get_metadata_table_prefix() == "test_prefix_");
}

TEST_CASE("Test MySQL arguments and credential validation", "[GlobalMetadataDBConfig]") {
    SECTION("With all arguments") {
        constexpr std::array argv{
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
        GlobalMetadataDBConfig config;
        parse_args(argv, config);

        SECTION("With valid credentials") {
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE(config.get_metadata_db_username() == "test-user");
            REQUIRE(config.get_metadata_db_password() == "test-pass");

            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("With empty password") {
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "");

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE(config.get_metadata_db_username() == "test-user");
            REQUIRE(config.get_metadata_db_password() == "");

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
        }
    }

    SECTION("With invalid port values") {
        SECTION("Port too low") {
            constexpr std::array argv_with_low_port{
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
            GlobalMetadataDBConfig config_low_port;
            parse_args(argv_with_low_port, config_low_port);
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config_low_port.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config_low_port.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("Port too high") {
            constexpr std::array argv_with_high_port{
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
            GlobalMetadataDBConfig config_high_port;
            parse_args(argv_with_high_port, config_high_port);
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config_high_port.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config_high_port.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }
    }

    SECTION("With empty required arguments") {
        SECTION("Empty db-host") {
            constexpr std::array argv_empty_host{
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
            GlobalMetadataDBConfig config_empty_host;
            parse_args(argv_empty_host, config_empty_host);
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config_empty_host.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config_empty_host.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("Empty db-name") {
            constexpr std::array argv_empty_name{
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
            GlobalMetadataDBConfig config_empty_name;
            parse_args(argv_empty_name, config_empty_name);
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config_empty_name.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config_empty_name.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }

        SECTION("Empty db-table-prefix") {
            constexpr std::array argv_empty_prefix{
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
            GlobalMetadataDBConfig config_empty_prefix;
            parse_args(argv_empty_prefix, config_empty_prefix);
            set_env_var("CLP_DB_USER", "test-user");
            set_env_var("CLP_DB_PASS", "test-pass");
            config_empty_prefix.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config_empty_prefix.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
            unset_env_var("CLP_DB_PASS");
        }
    }
}

TEST_CASE("Test SQLite arguments", "[GlobalMetadataDBConfig]") {
    SECTION("With non-default db-host argument") {
        constexpr std::array argv{"test", "--db-type", "sqlite", "--db-host", "test-host"};
        GlobalMetadataDBConfig config;

        REQUIRE_NOTHROW(parse_args(argv, config));
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-port argument") {
        constexpr std::array argv{"test", "--db-type", "sqlite", "--db-port", "8888"};
        GlobalMetadataDBConfig config;

        REQUIRE_NOTHROW(parse_args(argv, config));
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-name argument") {
        constexpr std::array argv{"test", "--db-type", "sqlite", "--db-name", "test-db"};
        GlobalMetadataDBConfig config;

        REQUIRE_NOTHROW(parse_args(argv, config));
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-table-prefix argument") {
        constexpr std::array
                argv{"test", "--db-type", "sqlite", "--db-table-prefix", "test_prefix_"};
        GlobalMetadataDBConfig config;

        REQUIRE_NOTHROW(parse_args(argv, config));
        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With username and password") {
        constexpr std::array argv{"test", "--db-type", "sqlite"};
        GlobalMetadataDBConfig config;
        parse_args(argv, config);

        set_env_var("CLP_DB_USER", "test-user");
        set_env_var("CLP_DB_PASS", "test-pass");

        config.read_credentials_from_env_if_needed();
        REQUIRE(config.get_metadata_db_username().empty());
        REQUIRE(config.get_metadata_db_password().empty());
        REQUIRE_NOTHROW(config.validate());

        unset_env_var("CLP_DB_USER");
        unset_env_var("CLP_DB_PASS");
    }
}
