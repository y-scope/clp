#ifndef CLP_MAKE_DICTIONARIES_READABLE_COMMANDLINEARGUMENTS_HPP
#define CLP_MAKE_DICTIONARIES_READABLE_COMMANDLINEARGUMENTS_HPP

#include "../CommandLineArgumentsBase.hpp"

namespace clp::make_dictionaries_readable {
class CommandLineArguments : public CommandLineArgumentsBase {
public:
    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : CommandLineArgumentsBase(program_name) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_archive_path() const { return m_archive_path; }

    std::string const& get_output_dir() const { return m_output_dir; }

private:
    // Methods
    void print_basic_usage() const override;

    // Variables
    std::string m_archive_path;
    std::string m_output_dir;
};
}  // namespace clp::make_dictionaries_readable

#endif  // CLP_MAKE_DICTIONARIES_READABLE_COMMANDLINEARGUMENTS_HPP
