#ifndef REDUCER_COMMANDLINEARGUMENTS_HPP
#define REDUCER_COMMANDLINEARGUMENTS_HPP

#include <string>

#include "../clp/CommandLineArgumentsBase.hpp"

namespace reducer {
/**
 * Class which parses and validates command line arguments for the reducer server.
 */
class CommandLineArguments : public clp::CommandLineArgumentsBase {
public:
    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : clp::CommandLineArgumentsBase(program_name) {}

    // Methods
    clp::CommandLineArgumentsBase::ParsingResult
    parse_arguments(int argc, char const* argv[]) override;

    [[nodiscard]] std::string const& get_reducer_host() const { return m_reducer_host; }

    [[nodiscard]] int get_reducer_port() const { return m_reducer_port; }

    [[nodiscard]] std::string const& get_db_host() const { return m_db_host; }

    [[nodiscard]] int get_db_port() const { return m_db_port; }

    [[nodiscard]] std::string const& get_db_user() const { return m_db_user; }

    [[nodiscard]] std::string const& get_db_password() const { return m_db_password; }

    [[nodiscard]] std::string const& get_db_database() const { return m_db_database; }

    [[nodiscard]] std::string const& get_db_jobs_table() const { return m_db_jobs_table; }

    [[nodiscard]] std::string const& get_mongodb_database() const { return m_mongodb_database; }

    [[nodiscard]] std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    [[nodiscard]] std::string const& get_mongodb_jobs_metric_collection() const {
        return m_mongodb_jobs_metric_collection;
    }

    [[nodiscard]] int get_polling_interval() const { return m_polling_interval_ms; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_reducer_host{"127.0.0.1"};
    int m_reducer_port{14'009};
    std::string m_db_host{"127.0.0.1"};
    int m_db_port{3306};
    std::string m_db_user{"clp-user"};
    std::string m_db_password{"password"};
    std::string m_db_database{"clp-db"};
    std::string m_db_jobs_table{"distributed_search_jobs"};
    std::string m_mongodb_database{"clp-search"};
    std::string m_mongodb_uri{"mongodb://localhost:27017/"};
    std::string m_mongodb_jobs_metric_collection{"search_jobs_metrics"};
    int m_polling_interval_ms{100};
};
}  // namespace reducer

#endif  // CLP_CLP_COMMANDLINEARGUMENTS_HPP
