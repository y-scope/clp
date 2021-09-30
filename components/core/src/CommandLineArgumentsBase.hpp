#ifndef COMMANDLINEARGUMENTSBASE_HPP
#define COMMANDLINEARGUMENTSBASE_HPP

// C++ standard libraries
#include <string>

/**
 * Base class for command line program arguments. This is meant to separate the parsing and validation of command line arguments from the rest of the program's
 * logic.
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
    explicit CommandLineArgumentsBase (const std::string& program_name) : m_program_name(program_name) {}

    // Methods
    virtual ParsingResult parse_arguments (int argc, const char* argv[]) = 0;

    const std::string& get_program_name () const { return m_program_name; }

private:
    // Methods
    virtual void print_basic_usage () const = 0;

    // Variables
    std::string m_program_name;
};


#endif // COMMANDLINEARGUMENTSBASE_HPP
