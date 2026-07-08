#ifndef CLP_S_COMMANDLINEARGUMENTS_HPP
#define CLP_S_COMMANDLINEARGUMENTS_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

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
    };

    struct StdoutOutputHandlerOptions {};

    using OutputHandlerOptionsVariant = std::
            variant<ResultsCacheOutputHandlerOptions,
                    FileOutputHandlerOptions,
                    NetworkOutputHandlerOptions,
                    ReducerOutputHandlerOptions,
                    StdoutOutputHandlerOptions>;

    // Constructors
    explicit CommandLineArguments(std::string const& program_name) : m_program_name(program_name) {}

    // Static data members
    static constexpr std::string_view cLogShapeStatsQuery{"stats.log_shapes"};
    static constexpr std::string_view cSchemaTreeStatsQuery{"stats.schema_tree"};

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

    [[nodiscard]] auto get_enable_telemetry() const -> bool { return m_enable_telemetry; }

    auto get_output_handler_options() const -> OutputHandlerOptionsVariant const& {
        return m_output_handler_options;
    }

    [[nodiscard]] auto get_aggregation_type() const -> std::optional<AggregationType> const& {
        return m_aggregation_type;
    }

    [[nodiscard]] auto get_count_by_time_bucket_size_ms() const -> int64_t {
        return m_count_by_time_bucket_size_ms;
    }

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

    [[nodiscard]] auto experimental() const -> bool { return m_experimental; }

    [[nodiscard]] auto get_parsing_spec() const -> std::optional<Path> {
        return m_parsing_spec_path;
    }

private:
    // Methods
    /**
     * Validates output options related to the Network Destination output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set.
     * @param network_options The parsed representation of the network output handler options.
     */
    void parse_network_dest_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<std::string> const& options,
            NetworkOutputHandlerOptions& network_options
    );

    /**
     * Validates the aggregation options (count and count-by-time) for output handlers that
     * support aggregations.
     * @param parsed_options
     * @param count_by_time_bucket_size_ms The parsed value of the count-by-time option; only
     * validated when that option was specified.
     * @return The requested aggregation type, or std::nullopt if no aggregation was requested.
     */
    [[nodiscard]] static auto parse_aggregation_options(
            boost::program_options::variables_map const& parsed_options,
            int64_t count_by_time_bucket_size_ms
    ) -> std::optional<AggregationType>;

    /**
     * Throws if an aggregation was requested.
     * @param handler_name The name of the output handler, used in the error message.
     * @throws std::invalid_argument if an aggregation was requested.
     */
    auto reject_aggregation_for_handler(std::string_view handler_name) const -> void;

    /**
     * Validates output options related to the Reducer output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set.
     * @param reducer_options The parsed representation of the reducer output handler options.
     */
    void parse_reducer_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<std::string> const& options,
            ReducerOutputHandlerOptions& reducer_options
    );

    /**
     * Validates output options related to the Results Cache output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set.
     * @param results_cache_options The parsed representation of the results cache output handler
     * options.
     */
    void parse_results_cache_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<std::string> const& options,
            ResultsCacheOutputHandlerOptions& results_cache_options
    );

    /**
     * Validates output options related to the File output handler.
     * @param options_description
     * @param options Vector of options previously parsed by boost::program_options and which may
     * contain options that have the unrecognized flag set.
     * @param file_options The parsed representation of the file output handler options.
     */
    void parse_file_output_handler_options(
            boost::program_options::options_description const& options_description,
            std::vector<std::string> const& options,
            FileOutputHandlerOptions& file_options
    );

    void print_basic_usage() const;

    void print_compression_usage() const;

    void print_decompression_usage() const;

    void print_search_usage() const;

    /**
     * Validate the use of experimental features. Requires the program options to have been parsed.
     * @throws std::invalid_argument if any experimental feature is used without setting the
     * experimetnal flag.
     */
    auto validate_experimental() const -> void;

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
    OutputHandlerOptionsVariant m_output_handler_options{StdoutOutputHandlerOptions{}};

    // Search variables
    std::string m_query;
    std::optional<epochtime_t> m_search_begin_ts;
    std::optional<epochtime_t> m_search_end_ts;
    bool m_ignore_case{false};
    bool m_enable_telemetry{false};
    std::vector<std::string> m_projection_columns;

    // clpp variables
    bool m_experimental{false};
    std::optional<Path> m_parsing_spec_path;

    // Aggregation variables
    std::optional<AggregationType> m_aggregation_type;
    int64_t m_count_by_time_bucket_size_ms{};
};
}  // namespace clp_s

#endif  // CLP_S_COMMANDLINEARGUMENTS_HPP
