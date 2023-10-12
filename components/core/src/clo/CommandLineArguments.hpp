#ifndef CLO_COMMANDLINEARGUMENTS_HPP
#define CLO_COMMANDLINEARGUMENTS_HPP

// C++ libraries
#include <vector>
#include <string>

// Boost libraries
#include <boost/asio.hpp>

// Project headers
#include "../CommandLineArgumentsBase.hpp"
#include "../Defs.h"

namespace clo {
    class CommandLineArguments : public CommandLineArgumentsBase {
    public:
        // Constructors
        explicit CommandLineArguments (const std::string& program_name) : CommandLineArgumentsBase(program_name), m_ignore_case(false),
                                                                          m_search_begin_ts(cEpochTimeMin), m_search_end_ts(cEpochTimeMax) {}

        // Methods
        ParsingResult parse_arguments (int argc, const char* argv[]) override;

        const std::string& get_search_controller_host () const { return m_search_controller_host; }
        const std::string& get_search_controller_port () const { return m_search_controller_port; }
        const std::string& get_archive_path () const { return m_archive_path; }
        const std::string& get_mongodb_uri () const { return m_mongodb_uri; }
        const std::string& get_mongodb_database () const { return m_mongodb_database; }
        const std::string& get_mongodb_collection () const { return m_mongodb_collection; }
        bool ignore_case () const { return m_ignore_case; }
        bool count_matches ()  const { return m_count_matches; }
        bool count_aggregation () const { return m_count; }
        const std::string& get_search_string () const { return m_search_string; }
        const std::string& get_file_path () const { return m_file_path; }
        epochtime_t get_search_begin_ts () const { return m_search_begin_ts; }
        epochtime_t get_search_end_ts () const { return m_search_end_ts; }

    private:
        // Methods
        void print_basic_usage () const override;

        // Variables
        std::string m_search_controller_host;
        std::string m_search_controller_port;
        std::string m_archive_path;
        std::string m_mongodb_uri;
        std::string m_mongodb_database;
        std::string m_mongodb_collection;
        bool m_ignore_case;
        bool m_count_matches;
        bool m_count;
        std::string m_search_string;
        std::string m_file_path;
        epochtime_t m_search_begin_ts, m_search_end_ts;
    };
}

#endif // CLO_COMMANDLINEARGUMENTS_HPP
