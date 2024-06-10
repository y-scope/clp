#ifndef GLT_GLT_COMMANDLINEARGUMENTS_HPP
#define GLT_GLT_COMMANDLINEARGUMENTS_HPP

#include <string>
#include <vector>

#include <boost/asio.hpp>

#include "../CommandLineArgumentsBase.hpp"
#include "../Defs.h"
#include "../GlobalMetadataDBConfig.hpp"

namespace glt::glt {
class CommandLineArguments : public CommandLineArgumentsBase {
public:
    // Types
    enum class Command : char {
        Compress = 'c',
        Extract = 'x',
        Search = 's',
    };

    // Types
    enum class OutputMethod : char {
        StdoutText = 's',
        StdoutBinary = 'b',
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name)
            : CommandLineArgumentsBase(program_name),
              m_show_progress(false),
              m_print_archive_stats_progress(false),
              m_target_segment_uncompressed_size(1L * 1024 * 1024 * 1024),
              m_target_encoded_file_size(512L * 1024 * 1024),
              m_target_data_size_of_dictionaries(100L * 1024 * 1024),
              m_compression_level(3),
              m_combine_threshold(0.1),
              m_ignore_case(false),
              m_output_method(OutputMethod::StdoutText),
              m_search_begin_ts(cEpochTimeMin),
              m_search_end_ts(cEpochTimeMax) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]) override;

    std::string const& get_path_list_path() const { return m_path_list_path; }

    std::string const& get_path_prefix_to_remove() const { return m_path_prefix_to_remove; }

    std::string const& get_output_dir() const { return m_output_dir; }

    bool show_progress() const { return m_show_progress; }

    bool print_archive_stats_progress() const { return m_print_archive_stats_progress; }

    size_t get_target_encoded_file_size() const { return m_target_encoded_file_size; }

    size_t get_target_segment_uncompressed_size() const {
        return m_target_segment_uncompressed_size;
    }

    size_t get_target_data_size_of_dictionaries() const {
        return m_target_data_size_of_dictionaries;
    }

    int get_compression_level() const { return m_compression_level; }

    double get_combine_threshold() const { return m_combine_threshold; }

    Command get_command() const { return m_command; }

    std::string const& get_archives_dir() const { return m_archives_dir; }

    std::vector<std::string> const& get_input_paths() const { return m_input_paths; }

    GlobalMetadataDBConfig const& get_metadata_db_config() const { return m_metadata_db_config; }

    // Search arguments
    std::string const& get_search_strings_file_path() const { return m_search_strings_file_path; }

    bool ignore_case() const { return m_ignore_case; }

    std::string const& get_search_string() const { return m_search_string; }

    std::string const& get_file_path() const { return m_file_path; }

    OutputMethod get_output_method() const { return m_output_method; }

    epochtime_t get_search_begin_ts() const { return m_search_begin_ts; }

    epochtime_t get_search_end_ts() const { return m_search_end_ts; }

private:
    // Methods
    void print_basic_usage() const override;
    void print_compression_basic_usage() const;
    void print_extraction_basic_usage() const;
    void print_search_basic_usage() const;

    // Variables
    std::string m_path_list_path;
    std::string m_path_prefix_to_remove;
    std::string m_output_dir;
    bool m_show_progress;
    bool m_print_archive_stats_progress;
    size_t m_target_encoded_file_size;
    size_t m_target_segment_uncompressed_size;
    size_t m_target_data_size_of_dictionaries;
    int m_compression_level;
    double m_combine_threshold;
    Command m_command;
    std::string m_archives_dir;
    std::vector<std::string> m_input_paths;
    GlobalMetadataDBConfig m_metadata_db_config;

    // Search related variables
    std::string m_search_strings_file_path;
    bool m_ignore_case;
    std::string m_search_string;
    std::string m_file_path;
    OutputMethod m_output_method;
    epochtime_t m_search_begin_ts, m_search_end_ts;
};
}  // namespace glt::glt

#endif  // GLT_GLT_COMMANDLINEARGUMENTS_HPP
