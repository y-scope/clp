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
            : clp::CommandLineArgumentsBase{program_name} {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    [[nodiscard]] std::string const& get_reducer_host() const { return m_reducer_host; }

    [[nodiscard]] int get_reducer_port() const { return m_reducer_port; }

    [[nodiscard]] std::string const& get_scheduler_host() const { return m_scheduler_host; }

    [[nodiscard]] int get_scheduler_port() const { return m_scheduler_port; }

    [[nodiscard]] std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    [[nodiscard]] int get_upsert_interval() const { return m_upsert_interval; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_reducer_host{"127.0.0.1"};
    int m_reducer_port{14'009};
    std::string m_scheduler_host{"127.0.0.1"};
    int m_scheduler_port{7000};
    std::string m_mongodb_uri{"mongodb://localhost:27017/clp-search"};
    int m_upsert_interval{100};  // Milliseconds
};
}  // namespace reducer

#endif  // REDUCER_COMMANDLINEARGUMENTS_HPP
