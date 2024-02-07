#ifndef CLP_S_COMMANDLINEARGUMENTS_HPP
#define CLP_S_COMMANDLINEARGUMENTS_HPP

#include <optional>
#include <string>
#include <vector>

#include "../clp/GlobalMetadataDBConfig.hpp"
#include "Defs.hpp"

namespace clp_s {
class CommandLineArguments {
public:
    // Types
    enum class ParsingResult {
        Success = 0,
        InfoCommand,
        Failure
    };

    enum class Command : char {
        Compress = 'c',
        Extract = 'x',
        Search = 's'
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name) : m_program_name(program_name) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]);

    std::string const& get_program_name() const { return m_program_name; }

    Command get_command() const { return m_command; }

    std::vector<std::string> const& get_file_paths() const { return m_file_paths; }

    std::string const& get_archives_dir() const { return m_archives_dir; }

    std::string const& get_output_dir() const { return m_output_dir; }

    std::string const& get_timestamp_key() const { return m_timestamp_key; }

    int get_compression_level() const { return m_compression_level; }

    size_t get_target_encoded_size() const { return m_target_encoded_size; }

    [[nodiscard]] bool print_archive_stats() const { return m_print_archive_stats; }

    bool get_mongodb_enabled() const { return m_mongodb_enabled; }

    std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    std::string const& get_mongodb_collection() const { return m_mongodb_collection; }

    uint64_t get_batch_size() const { return m_batch_size; }

    uint64_t get_max_num_results() const { return m_max_num_results; }

    std::string const& get_query() const { return m_query; }

    std::optional<epochtime_t> get_search_begin_ts() const { return m_search_begin_ts; }

    std::optional<epochtime_t> get_search_end_ts() const { return m_search_end_ts; }

    std::optional<clp::GlobalMetadataDBConfig> const& get_metadata_db_config() const {
        return m_metadata_db_config;
    }

private:
    // Methods
    void print_basic_usage() const;

    void print_compression_usage() const;

    void print_decompression_usage() const;

    void print_search_usage() const;

    // Variables
    std::string m_program_name;
    Command m_command;

    // Compression and decompression variables
    std::vector<std::string> m_file_paths;
    std::string m_archives_dir;
    std::string m_output_dir;
    std::string m_timestamp_key;
    int m_compression_level{3};
    size_t m_target_encoded_size{8ULL * 1024 * 1024 * 1024};  // 8 GiB
    bool m_print_archive_stats{false};

    // Metadata db variables
    std::optional<clp::GlobalMetadataDBConfig> m_metadata_db_config;

    // MongoDB configuration variables
    bool m_mongodb_enabled{false};
    std::string m_mongodb_uri;
    std::string m_mongodb_collection;
    uint64_t m_batch_size{1000};
    uint64_t m_max_num_results{1000};

    // Search variables
    std::string m_query;
    std::optional<epochtime_t> m_search_begin_ts;
    std::optional<epochtime_t> m_search_end_ts;
};
}  // namespace clp_s

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
