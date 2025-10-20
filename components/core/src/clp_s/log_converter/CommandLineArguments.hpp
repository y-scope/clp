#ifndef CLP_S_COMMANDLINEARGUMENTS_HPP
#define CLP_S_COMMANDLINEARGUMENTS_HPP

#include <string>
#include <vector>

#include "../InputConfig.hpp"

namespace clp_s::log_converter {
class CommandLineArguments {
public:
    // Types
    enum class ParsingResult {
        Success = 0,
        InfoCommand,
        Failure
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name) : m_program_name(program_name) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]);

    std::string const& get_program_name() const { return m_program_name; }

    std::vector<Path> const& get_input_paths() const { return m_input_paths; }

    NetworkAuthOption const& get_network_auth() const { return m_network_auth; }

    std::string const& get_output_dir() const { return m_output_dir; }

private:
    void print_basic_usage() const;

    // Variables
    std::string m_program_name;

    // Compression and decompression variables
    std::vector<Path> m_input_paths;
    NetworkAuthOption m_network_auth{};
    std::string m_output_dir{"./"};
};
}  // namespace clp_s::log_converter

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
