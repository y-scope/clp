#ifndef CLP_S_COMMANDLINEARGUMENTS_HPP
#define CLP_S_COMMANDLINEARGUMENTS_HPP

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/program_options/option.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>

#include "../reducer/types.hpp"
#include "Defs.hpp"
#include "InputConfig.hpp"

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

    enum class OutputHandlerType : uint8_t {
        File = 0,
        Network,
        Reducer,
        ResultsCache,
        Stdout,
    };

    enum class AggregationType : uint8_t {
        Count,
        CountByTime,
    };

    struct ResultsCacheOutputHandlerOptions {
        std::string uri;
        std::string collection;
        std::string dataset;
        uint64_t batch_size{1000};
        uint64_t max_num_results{1000};
    };

    struct FileOutputHandlerOptions {
        std::string output_path;
    };

    struct NetworkOutputHandlerOptions {
        std::string host;
        int port{};
    };

    struct ReducerOutputHandlerOptions {
        std::string host;
        int port{-1};
        reducer::job_id_t job_id{-1};
        AggregationType aggregation_type{AggregationType::Count};
        int64_t count_by_time_bucket_size{};  // Milliseconds
    };

    // Constructors
    explicit CommandLineArguments(std::string const& program_name) : m_program_name(program_name) {}

    // Methods
    ParsingResult parse_arguments(int argc, char const* argv[]);

    std::string const& get_program_name() const { return m_program_name; }

    Command get_command() const { return m_command; }

    std::vector<Path> const& get_input_paths() const { return m_input_paths; }

    [[nodiscard]] auto get_input_paths_and_canonical_filenames() const
            -> std::vector<std::pair<Path, std::string>> const& {
        return m_input_paths_and_canonical_filenames;
    }

    NetworkAuthOption const& get_network_auth() const { return m_network_auth; }

    std::string const& get_archives_dir() const { return m_archives_dir; }

    std::string const& get_output_dir() const { return m_output_dir; }

    std::string const& get_timestamp_key() const { return m_timestamp_key; }

    int get_compression_level() const { return m_compression_level; }

    size_t get_target_encoded_size() const { return m_target_encoded_size; }

    size_t get_max_document_size() const { return m_max_document_size; }

    [[nodiscard]] bool print_archive_stats() const { return m_print_archive_stats; }

    [[nodiscard]] auto print_ordered_chunk_stats() const -> bool {
        return m_print_ordered_chunk_stats;
    }

    std::string const& get_mongodb_uri() const { return m_mongodb_uri; }

    std::string const& get_mongodb_collection() const { return m_mongodb_collection; }

    std::string const& get_query() const { return m_query; }

    std::optional<epochtime_t> get_search_begin_ts() const { return m_search_begin_ts; }

    std::optional<epochtime_t> get_search_end_ts() const { return m_search_end_ts; }

    bool get_ignore_case() const { return m_ignore_case; }

    auto get_results_cache_output_handler_options() const
            -> ResultsCacheOutputHandlerOptions const& {
        return m_results_cache_output_handler_options;
    }

    auto get_file_output_handler_options() const -> FileOutputHandlerOptions const& {
        return m_file_output_handler_options;
    }

    auto get_network_output_handler_options() const -> NetworkOutputHandlerOptions const& {
        return m_network_output_handler_options;
    }

    auto get_reducer_output_handler_options() const -> ReducerOutputHandlerOptions const& {
        return m_reducer_output_handler_options;
    }

    OutputHandlerType get_output_handler_type() const { return m_output_handler_type; }

    [[nodiscard]] auto get_retain_float_format() const -> bool {
        return false == m_no_retain_float_format;
    }

    bool get_single_file_archive() const { return m_single_file_archive; }

    bool get_structurize_arrays() const { return m_structurize_arrays; }

    bool get_ordered_decompression() const { return m_ordered_decompression; }

    size_t get_target_ordered_chunk_size() const { return m_target_ordered_chunk_size; }

    size_t get_minimum_table_size() const { return m_minimum_table_size; }

    std::vector<std::string> const& get_projection_columns() const { return m_projection_columns; }

    bool get_record_log_order() const { return false == m_disable_log_order; }

private:
    // Methods
    /**
     * Validates output options related to the Network Destination output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_network_dest_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    /**
     * Validates output options related to the Reducer output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_reducer_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    /**
     * Validates output options related to the Results Cache output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_results_cache_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    /**
     * Validates output options related to the File output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set
     * @param parsed_options Returns any parsed options that were newly recognized
     */
    void parse_file_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<boost::program_options::option> const& options,
            boost::program_options::variables_map& parsed_options
    );

    void print_basic_usage() const;

    void print_compression_usage() const;

    void print_decompression_usage() const;

    void print_search_usage() const;

    // Variables
    std::string m_program_name;
    Command m_command;

    // Compression and decompression variables
    std::vector<Path> m_input_paths;
    std::vector<std::pair<Path, std::string>> m_input_paths_and_canonical_filenames;
    NetworkAuthOption m_network_auth{};
    std::string m_archives_dir;
    std::string m_output_dir;
    std::string m_timestamp_key;
    int m_compression_level{3};
    size_t m_target_encoded_size{8ULL * 1024 * 1024 * 1024};  // 8 GiB
    bool m_print_archive_stats{false};
    size_t m_max_document_size{512ULL * 1024 * 1024};  // 512 MiB
    bool m_no_retain_float_format{false};
    bool m_single_file_archive{false};
    bool m_structurize_arrays{false};
    bool m_ordered_decompression{false};
    size_t m_target_ordered_chunk_size{};
    bool m_print_ordered_chunk_stats{false};
    size_t m_minimum_table_size{1ULL * 1024 * 1024};  // 1 MiB
    bool m_disable_log_order{false};
    std::string m_mongodb_uri;
    std::string m_mongodb_collection;

    // Search output handler options
    OutputHandlerType m_output_handler_type{OutputHandlerType::Stdout};
    ResultsCacheOutputHandlerOptions m_results_cache_output_handler_options{};
    NetworkOutputHandlerOptions m_network_output_handler_options{};
    FileOutputHandlerOptions m_file_output_handler_options{};
    ReducerOutputHandlerOptions m_reducer_output_handler_options{};

    // Search variables
    std::string m_query;
    std::optional<epochtime_t> m_search_begin_ts;
    std::optional<epochtime_t> m_search_end_ts;
    bool m_ignore_case{false};
    std::vector<std::string> m_projection_columns;
};
}  // namespace clp_s

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
