#ifndef CLP_S_COMMANDLINEARGUMENTS_HPP
#define CLP_S_COMMANDLINEARGUMENTS_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "../InputConfig.hpp"

namespace clp_s::log_converter {
class CommandLineArguments {
public:
    // Types
    enum class ParsingResult : uint8_t {
        Success = 0,
        InfoCommand,
        Failure
    };

    // Constructors
    explicit CommandLineArguments(std::string_view program_name) : m_program_name{program_name} {}

    // Methods
    [[nodiscard]] auto parse_arguments(int argc, char const** argv) -> ParsingResult;

    [[nodiscard]] auto get_program_name() const -> std::string const& { return m_program_name; }

    [[nodiscard]] auto get_input_paths() const -> std::vector<Path> const& { return m_input_paths; }

    [[nodiscard]] auto get_network_auth() const -> NetworkAuthOption const& {
        return m_network_auth;
    }

    [[nodiscard]] auto get_output_dir() const -> std::string const& { return m_output_dir; }

private:
    // Methods
    void print_basic_usage() const;

    // Variables
    std::string m_program_name;
    std::vector<Path> m_input_paths;
    NetworkAuthOption m_network_auth{};
    std::string m_output_dir{"./"};
};
}  // namespace clp_s::log_converter

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
