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
auto parse_args(std::array<char const*, N> const& argv, GlobalMetadataDBConfig& config)
        -> boost::program_options::variables_map {
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

    return vm;
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
    auto const vm = parse_args(argv, config);

    REQUIRE(config.get_metadata_db_type() == GlobalMetadataDBConfig::MetadataDBType::MySQL);
    REQUIRE(config.get_metadata_db_host() == "test-host");
    REQUIRE(config.get_metadata_db_port() == 8888);
    REQUIRE(config.get_metadata_db_name() == "test-db");
    REQUIRE(config.get_metadata_table_prefix() == "test_prefix_");
}

TEST_CASE("Test reading credentials from environment variables", "[GlobalMetadataDBConfig]") {
    set_env_var("CLP_DB_USER", "test-user");
    set_env_var("CLP_DB_PASS", "test-pass");

    GlobalMetadataDBConfig config;
    config.read_credentials_from_env();

    REQUIRE(config.get_metadata_db_username() == "test-user");
    REQUIRE(config.get_metadata_db_password() == "test-pass");

    unset_env_var("CLP_DB_USER");
    unset_env_var("CLP_DB_PASS");
}

TEST_CASE("Test MySQL arguments and credential validation", "[GlobalMetadataDBConfig]") {
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
    auto const vm = parse_args(argv, config);

    SECTION("With valid credentials") {
        set_env_var("CLP_DB_USER", "test-user");
        set_env_var("CLP_DB_PASS", "test-pass");
        config.read_credentials_from_env();
        REQUIRE_NOTHROW(config.validate());
        unset_env_var("CLP_DB_USER");
        unset_env_var("CLP_DB_PASS");
    }

    SECTION("With missing credentials") {
        // Neither username nor password set
        SECTION("Neither username nor password") {
            config.read_credentials_from_env();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }
        SECTION("Username set but password missing") {
            set_env_var("CLP_DB_USER", "test-user");
            config.read_credentials_from_env();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_USER");
        }
        SECTION("Password set but username missing") {
            set_env_var("CLP_DB_PASS", "test-pass");
            config.read_credentials_from_env();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            unset_env_var("CLP_DB_PASS");
        }
    }
}

TEST_CASE("Test SQLite arguments", "[GlobalMetadataDBConfig]") {
    constexpr std::array argv{"test", "--db-type", "sqlite"};
    GlobalMetadataDBConfig config;
    auto const vm = parse_args(argv, config);

    REQUIRE_NOTHROW(config.validate());
}
