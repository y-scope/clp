#ifndef CLO_COMMANDLINEARGUMENTS_HPP
    #define CLO_COMMANDLINEARGUMENTS_HPP

    #include <string>
    #include <vector>

    #include <boost/asio.hpp>

    #include "../CommandLineArgumentsBase.hpp"
    #include "../Defs.h"

namespace clg {
class CommandLineArguments : public CommandLineArgumentsBase {
public:
    enum class OutputMethod : char {
        StdoutText = 's',
        StdoutBinary = 'b',
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : CommandLineArgumentsBase(program_name),
              m_ignore_case(false),
              m_output_method(OutputMethod::StdoutText),
              m_search_begin_ts(cEpochTimeMin),
              m_search_end_ts(cEpochTimeMax) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_search_controller_host() const { return m_search_controller_host; }

    std::string const& get_search_controller_port() const { return m_search_controller_port; }

    std::vector<std::string> const& get_archive_path() const { return m_archives_path; }

    bool ignore_case() const { return m_ignore_case; }

    std::string const& get_search_string() const { return m_search_string; }

    std::string const& get_search_strings_file_path() const { return ""; }

    std::string const& get_file_path() const { return m_file_path; }

    OutputMethod get_output_method() const { return m_output_method; }

    epochtime_t get_search_begin_ts() const { return m_search_begin_ts; }

    epochtime_t get_search_end_ts() const { return m_search_end_ts; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_search_controller_host;
    std::string m_search_controller_port;
    std::vector<std::string> m_archives_path;
    bool m_ignore_case;
    std::string m_search_string;
    std::string m_file_path;
    epochtime_t m_search_begin_ts, m_search_end_ts;
    OutputMethod m_output_method;
};

#endif  // CLO_COMMANDLINEARGUMENTS_HPP
}
