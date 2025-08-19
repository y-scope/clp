#include <array>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <catch2/catch_test_macros.hpp>

#include "../src/clp/GlobalMetadataDBConfig.hpp"

using namespace clp;

namespace {
// Constants
constexpr std::string_view cMysqlDbType{"mysql"};
constexpr std::string_view cMysqlDbHost{"test-host"};
constexpr std::string_view cMysqlDbPort{"8888"};
constexpr std::string_view cMysqlDbName{"test-db"};
constexpr std::string_view cMysqlDbTablePrefix{"test_prefix_"};
constexpr std::string_view cMysqlDbUser{"test-user"};
constexpr std::string_view cMysqlDbPass{"test-pass"};
constexpr std::array cCommonMysqlArgv{
        std::string_view{"test"},
        std::string_view{"--db-type"},
        cMysqlDbType,
        std::string_view{"--db-host"},
        cMysqlDbHost,
        std::string_view{"--db-port"},
        cMysqlDbPort,
        std::string_view{"--db-name"},
        cMysqlDbName,
        std::string_view{"--db-table-prefix"},
        cMysqlDbTablePrefix
};
constexpr size_t cArgIdxDbHost{4};
constexpr size_t cArgIdxDbPort{6};
constexpr size_t cArgIdxDbName{8};
constexpr size_t cArgIdxDbTablePrefix{10};

constexpr std::string_view cSqliteDbType{"sqlite"};
constexpr std::array cCommonSqliteArgv{
        std::string_view{"test"},
        std::string_view{"--db-type"},
        cSqliteDbType,

        // Placeholders for overwriting with MySQL-specific arguments like `--db-host`.
        std::string_view{""},
        std::string_view{""},
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
auto parse_args(std::array<std::string_view, n> const& argv) -> GlobalMetadataDBConfig {
    boost::program_options::options_description options_desc;
    GlobalMetadataDBConfig config{options_desc};

    boost::program_options::variables_map vm;
    constexpr auto cArgc{static_cast<int>(n)};

    std::array<std::string, n> argv_strings;
    std::array<char const*, n> argv_c_strs{};
    for (size_t i = 0; i < n; ++i) {
        argv_strings.at(i) = argv.at(i);
        argv_c_strs.at(i) = argv_strings.at(i).c_str();
    }
    boost::program_options::store(
            boost::program_options::parse_command_line(cArgc, argv_c_strs.data(), options_desc),
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
    auto const config{parse_args(cCommonMysqlArgv)};

    REQUIRE((config.get_metadata_db_type() == GlobalMetadataDBConfig::MetadataDBType::MySQL));
    REQUIRE((config.get_metadata_db_host() == cMysqlDbHost));
    REQUIRE((config.get_metadata_db_port() == std::stoi(std::string(cMysqlDbPort))));
    REQUIRE((config.get_metadata_db_name() == cMysqlDbName));
    REQUIRE((config.get_metadata_table_prefix() == cMysqlDbTablePrefix));
}

TEST_CASE("Test MySQL arguments and credential validation", "[GlobalMetadataDBConfig]") {
    SECTION("With all arguments") {
        auto config{parse_args(cCommonMysqlArgv)};

        SECTION("With valid credentials") {
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE((config.get_metadata_db_username() == cMysqlDbUser));
            REQUIRE((config.get_metadata_db_password() == cMysqlDbPass));
        }

        SECTION("With empty password") {
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", "");

            config.read_credentials_from_env_if_needed();
            REQUIRE_NOTHROW(config.validate());
            REQUIRE((config.get_metadata_db_username() == cMysqlDbUser));
            REQUIRE((config.get_metadata_db_password() == ""));
        }

        SECTION("With missing credentials") {
            SECTION("Neither username nor password") {
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
            SECTION("Password set but username missing") {
                set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
            SECTION("Username set but password missing") {
                set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
                config.read_credentials_from_env_if_needed();
                REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
            }
        }
    }

    SECTION("With invalid port values") {
        SECTION("Port too low") {
            auto argv{cCommonMysqlArgv};
            argv[cArgIdxDbPort] = "0";
            auto config{parse_args(argv)};
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }

        SECTION("Port too high") {
            auto argv{cCommonMysqlArgv};
            argv[cArgIdxDbPort] = "65536";
            auto config{parse_args(argv)};
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }
    }

    SECTION("With empty required arguments") {
        SECTION("Empty db-host") {
            auto argv{cCommonMysqlArgv};
            argv[cArgIdxDbHost] = "";
            auto config{parse_args(argv)};
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }

        SECTION("Empty db-name") {
            auto argv{cCommonMysqlArgv};
            argv[cArgIdxDbName] = "";
            auto config{parse_args(argv)};
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());
            config.read_credentials_from_env_if_needed();
            REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
        }

        SECTION("Empty db-table-prefix") {
            auto argv{cCommonMysqlArgv};
            argv[cArgIdxDbTablePrefix] = "";
            auto config{parse_args(argv)};
            set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
            set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());
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
        auto argv{cCommonSqliteArgv};
        argv[cArgIdxOverwriteArgName] = "--db-host";
        argv[cArgIdxOverwriteArgValue] = cMysqlDbHost;
        auto const config{parse_args(argv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-port argument") {
        auto argv{cCommonSqliteArgv};
        argv[cArgIdxOverwriteArgName] = "--db-port";
        argv[cArgIdxOverwriteArgValue] = cMysqlDbPort;
        auto const config{parse_args(argv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-name argument") {
        auto argv{cCommonSqliteArgv};
        argv[cArgIdxOverwriteArgName] = "--db-name";
        argv[cArgIdxOverwriteArgValue] = cMysqlDbName;
        auto const config{parse_args(argv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With non-default db-table-prefix argument") {
        auto argv{cCommonSqliteArgv};
        argv[cArgIdxOverwriteArgName] = "--db-table-prefix";
        argv[cArgIdxOverwriteArgValue] = cMysqlDbTablePrefix;
        auto const config{parse_args(argv)};

        REQUIRE_THROWS_AS(config.validate(), std::invalid_argument);
    }

    SECTION("With username and password") {
        auto config{parse_args(cCommonSqliteArgv)};

        set_env_var("CLP_DB_USER", std::string{cMysqlDbUser}.c_str());
        set_env_var("CLP_DB_PASS", std::string{cMysqlDbPass}.c_str());

        config.read_credentials_from_env_if_needed();
        REQUIRE_FALSE(config.get_metadata_db_username().has_value());
        REQUIRE_FALSE(config.get_metadata_db_password().has_value());
        REQUIRE_NOTHROW(config.validate());
    }

    // Tear down.
    unset_env_var("CLP_DB_USER");
    unset_env_var("CLP_DB_PASS");
}
