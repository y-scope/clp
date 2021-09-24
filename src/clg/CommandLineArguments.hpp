#ifndef CLG_COMMANDLINEARGUMENTS_HPP
#define CLG_COMMANDLINEARGUMENTS_HPP

// C++ libraries
#include <vector>
#include <string>

// Boost libraries
#include <boost/asio.hpp>

// Project headers
#include "../CommandLineArgumentsBase.hpp"
#include "../Defs.h"
#include "../GlobalMetadataDBConfig.hpp"

namespace clg {
    class CommandLineArguments : public CommandLineArgumentsBase {
    public:
        // Types
        enum class OutputMethod : char {
            StdoutText = 's',
            StdoutBinary = 'b',
        };

        // Constructors
        explicit CommandLineArguments (const std::string& program_name) : CommandLineArgumentsBase(program_name), m_ignore_case(false),
                m_output_method(OutputMethod::StdoutText), m_search_begin_ts(cEpochTimeMin), m_search_end_ts(cEpochTimeMax) {}

        // Methods
        ParsingResult parse_arguments (int argc, const char* argv[]) override;

        const std::string& get_search_strings_file_path () const { return m_search_strings_file_path; }
        bool ignore_case () const { return m_ignore_case; }
        const std::string& get_archives_dir () const { return m_archives_dir; }
        const std::string& get_search_string () const { return m_search_string; }
        const std::string& get_file_path () const { return m_file_path; }
        OutputMethod get_output_method () const { return m_output_method; }
        epochtime_t get_search_begin_ts () const { return m_search_begin_ts; }
        epochtime_t get_search_end_ts () const { return m_search_end_ts; }
        const GlobalMetadataDBConfig& get_metadata_db_config () const { return m_metadata_db_config; }

    private:
        // Methods
        void print_basic_usage () const override;

        // Variables
        std::string m_search_strings_file_path;
        bool m_ignore_case;
        std::string m_archives_dir;
        std::string m_search_string;
        std::string m_file_path;
        OutputMethod m_output_method;
        epochtime_t m_search_begin_ts, m_search_end_ts;
        GlobalMetadataDBConfig m_metadata_db_config;
    };
}

#endif // CLG_COMMANDLINEARGUMENTS_HPP
