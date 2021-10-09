#ifndef UTILS_MAKE_DICTIONARIES_READABLE_COMMANDLINEARGUMENTS_HPP
#define UTILS_MAKE_DICTIONARIES_READABLE_COMMANDLINEARGUMENTS_HPP

// Project headers
#include "../../CommandLineArgumentsBase.hpp"

namespace utils { namespace make_dictionaries_readable {
    class CommandLineArguments : public CommandLineArgumentsBase {
    public:
        // Constructors
        explicit CommandLineArguments (const std::string& program_name) : CommandLineArgumentsBase(program_name) {}

        // Methods
        ParsingResult parse_arguments (int argc, const char* argv[]) override;

        const std::string& get_archive_path () const { return m_archive_path; }
        const std::string& get_output_dir () const { return m_output_dir; }

    private:
        // Methods
        void print_basic_usage () const override;

        // Variables
        std::string m_archive_path;
        std::string m_output_dir;
    };
} }

#endif // UTILS_MAKE_DICTIONARIES_READABLE_COMMANDLINEARGUMENTS_HPP
