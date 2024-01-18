#ifndef CLP_S_COMMANDLINEARGUMENTS_HPP
#define CLP_S_COMMANDLINEARGUMENTS_HPP

#include <string>
#include <vector>

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
    explicit CommandLineArguments(std::string const& program_name)
            : m_program_name(program_name),
              m_compression_level(3),
              m_target_encoded_size(8UL * 1024 * 1024 * 1024),  // 8 GiB
              m_mongodb_enabled(false),
              m_batch_size(1000) {}

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

    bool get_mongodb_enabled() const { return m_mongodb_enabled; }

    std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    std::string const& get_mongodb_collection() const { return m_mongodb_collection; }

    uint64_t get_batch_size() const { return m_batch_size; }

    std::string const& get_query() const { return m_query; }

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
    int m_compression_level;
    size_t m_target_encoded_size;

    // MongoDB configuration variables
    bool m_mongodb_enabled;
    std::string m_mongodb_uri;
    std::string m_mongodb_collection;
    uint64_t m_batch_size;

    // Search variables
    std::string m_query;
};
}  // namespace clp_s

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
