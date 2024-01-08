#ifndef REDUCER_COMMANDLINEARGUMENTS_HPP
#define REDUCER_COMMANDLINEARGUMENTS_HPP

#include <string>

#include "../clp/CommandLineArgumentsBase.hpp"

namespace reducer {
class CommandLineArguments : public clp::CommandLineArgumentsBase {
public:
    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : clp::CommandLineArgumentsBase(program_name),
              m_reducer_host("127.0.0.1"),
              m_reducer_port(14'009),
              m_db_host("127.0.0.1"),
              m_db_port(3306),
              m_db_user("clp-user"),
              m_db_password("password"),
              m_db_database("clp-db"),
              m_mongodb_database("clp-search"),
              m_mongodb_uri("mongodb://localhost:27017/"),
              m_mongodb_jobs_metric_collection("search_jobs_metrics"),
              m_polling_interval_ms(100) {}

    // Methods
    clp::CommandLineArgumentsBase::ParsingResult
    parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_reducer_host() const { return m_reducer_host; }

    int64_t get_reducer_port() const { return m_reducer_port; }

    std::string const& get_db_host() const { return m_db_host; }

    int64_t get_db_port() const { return m_db_port; }

    std::string const& get_db_user() const { return m_db_user; }

    std::string const& get_db_password() const { return m_db_password; }

    std::string const& get_db_database() const { return m_db_database; }

    std::string const& get_mongodb_database() const { return m_mongodb_database; }

    std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    std::string const& get_mongodb_jobs_metric_collection() const {
        return m_mongodb_jobs_metric_collection;
    }

    int get_polling_interval() const { return m_polling_interval_ms; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_reducer_host;
    int64_t m_reducer_port;
    std::string m_db_host;
    int64_t m_db_port;
    std::string m_db_user;
    std::string m_db_password;
    std::string m_db_database;
    std::string m_mongodb_database;
    std::string m_mongodb_uri;
    std::string m_mongodb_jobs_metric_collection;
    int m_polling_interval_ms;
};
}  // namespace reducer

#endif  // CLP_CLP_COMMANDLINEARGUMENTS_HPP
