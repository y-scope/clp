#ifndef GLT_COMMANDLINEARGUMENTSBASE_HPP
#define GLT_COMMANDLINEARGUMENTSBASE_HPP

#include <string>

namespace glt {
/**
 * Base class for command line program arguments. This is meant to separate the parsing and
 * validation of command line arguments from the rest of the program's logic.
 */
class CommandLineArgumentsBase {
public:
    // Types
    enum class ParsingResult {
        Success = 0,
        InfoCommand,
        Failure
    };

    // Constructors
    explicit CommandLineArgumentsBase(std::string const& program_name)
            : m_program_name(program_name) {}

    // Methods
    virtual ParsingResult parse_arguments(int argc, char const* argv[]) = 0;

    std::string const& get_program_name() const { return m_program_name; }

private:
    // Methods
    virtual void print_basic_usage() const = 0;

    // Variables
    std::string m_program_name;
};
}  // namespace glt

#endif  // GLT_COMMANDLINEARGUMENTSBASE_HPP
