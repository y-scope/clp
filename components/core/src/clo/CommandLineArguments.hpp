#ifndef CLO_COMMANDLINEARGUMENTS_HPP
#define CLO_COMMANDLINEARGUMENTS_HPP

#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "../CommandLineArgumentsBase.hpp"
#include "../Defs.h"

namespace clo {
class CommandLineArguments : public CommandLineArgumentsBase {
public:
    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : CommandLineArgumentsBase(program_name),
              m_ignore_case(false),
              m_search_begin_ts(cEpochTimeMin),
              m_search_end_ts(cEpochTimeMax) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_search_controller_host() const { return m_search_controller_host; }

    std::string const& get_search_controller_port() const { return m_search_controller_port; }

    std::string const& get_archive_path() const { return m_archive_path; }

    bool ignore_case() const { return m_ignore_case; }

    std::string const& get_search_string() const { return m_search_string; }

    std::string const& get_file_path() const { return m_file_path; }

    epochtime_t get_search_begin_ts() const { return m_search_begin_ts; }

    epochtime_t get_search_end_ts() const { return m_search_end_ts; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_search_controller_host;
    std::string m_search_controller_port;
    std::string m_archive_path;
    bool m_ignore_case;
    std::string m_search_string;
    std::string m_file_path;
    epochtime_t m_search_begin_ts, m_search_end_ts;
};
}  // namespace clo

#endif  // CLO_COMMANDLINEARGUMENTS_HPP
