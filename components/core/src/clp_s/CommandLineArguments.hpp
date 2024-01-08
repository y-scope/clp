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
    explicit CommandLineArguments(std::string const& program_name) : m_program_name(program_name) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]);

    std::string const& get_program_name() const { return m_program_name; }

    Command get_command() const { return m_command; }

    std::vector<std::string> const& get_file_paths() const { return m_file_paths; }

    std::string const& get_archive_dir() const { return m_archive_dir; }

    std::string const& get_output_dir() const { return m_output_dir; }

    std::string const& get_timestamp_key() const { return m_timestamp_key; }

    int get_compression_level() const { return m_compression_level; }

    size_t get_max_encoding_size() const { return m_max_encoding_size; }

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
    std::string m_archive_dir;
    std::string m_output_dir;
    std::string m_timestamp_key;
    int m_compression_level;
    size_t m_max_encoding_size;

    // Search variables
    std::string m_query;
};
}  // namespace clp_s

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
