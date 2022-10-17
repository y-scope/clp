#ifndef CLP_COMMANDLINEARGUMENTS_HPP
#define CLP_COMMANDLINEARGUMENTS_HPP

// C++ libraries
#include <vector>
#include <string>

// Boost libraries
#include <boost/asio.hpp>

// Project headers
#include "../CommandLineArgumentsBase.hpp"
#include "../GlobalMetadataDBConfig.hpp"

namespace clp {
    class CommandLineArguments : public CommandLineArgumentsBase {
    public:
        // Types
        enum class Command : char {
            Compress = 'c',
            Extract = 'x',
        };

        // Constructors
        explicit CommandLineArguments (const std::string& program_name) : CommandLineArgumentsBase(program_name), m_show_progress(false),
                m_print_archive_stats_progress(false), m_target_segment_uncompressed_size(1L * 1024 * 1024 * 1024),
                m_target_encoded_file_size(512L * 1024 * 1024), m_target_data_size_of_dictionaries(100L * 1024 * 1024), m_compression_level(3) {}

        // Methods
        ParsingResult parse_arguments (int argc, const char* argv[]) override;

        const std::string& get_path_list_path () const { return m_path_list_path; }
        const std::string& get_path_prefix_to_remove () const { return m_path_prefix_to_remove; }
        const std::string& get_output_dir () const { return m_output_dir; }
        const std::string& get_schema_file_path () const { return m_schema_file_path; }
        bool get_use_heuristic () const { return (m_schema_file_path.empty()); }
        bool show_progress () const { return m_show_progress; }
        bool print_archive_stats_progress () const { return m_print_archive_stats_progress; }
        size_t get_target_encoded_file_size () const { return m_target_encoded_file_size; }
        size_t get_target_segment_uncompressed_size () const { return m_target_segment_uncompressed_size; }
        size_t get_target_data_size_of_dictionaries () const { return m_target_data_size_of_dictionaries; }
        int get_compression_level () const { return m_compression_level; }
        Command get_command () const { return m_command; }
        const std::string& get_archives_dir () const { return m_archives_dir; }
        const std::vector<std::string>& get_input_paths () const { return m_input_paths; }
        const GlobalMetadataDBConfig& get_metadata_db_config () const { return m_metadata_db_config; }

    private:
        // Methods
        void print_basic_usage () const override;
        void print_compression_basic_usage () const;
        void print_extraction_basic_usage () const;

        // Variables
        std::string m_path_list_path;
        std::string m_path_prefix_to_remove;
        std::string m_output_dir;
        std::string m_schema_file_path;
        bool m_show_progress;
        bool m_print_archive_stats_progress;
        size_t m_target_encoded_file_size;
        size_t m_target_segment_uncompressed_size;
        size_t m_target_data_size_of_dictionaries;
        int m_compression_level;
        Command m_command;
        std::string m_archives_dir;
        std::vector<std::string> m_input_paths;
        GlobalMetadataDBConfig m_metadata_db_config;
    };
}

#endif // CLP_COMMANDLINEARGUMENTS_HPP
