#include "CommandLineArguments.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <boost/program_options.hpp>
#include <json/single_include/nlohmann/json.hpp>

#include "../cli_utils.hpp"
#include "../reducer/types.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../version.hpp"

namespace po = boost::program_options;
using std::cerr;
using std::endl;
using std::exception;
using std::invalid_argument;
using std::string;
using std::vector;

namespace clp::clo {
namespace {
bool has_non_null_field(nlohmann::json const& json, std::string const& key) {
    return json.count(key) > 0 && false == json[key].is_null();
}
}  // namespace

CommandLineArgumentsBase::ParsingResult
CommandLineArguments::parse_arguments(int argc, char const* argv[]) {
    // Print out basic usage if user doesn't specify any options
    if (1 == argc) {
        print_basic_usage();
        return ParsingResult::Failure;
    }

    // Define general options
    po::options_description options_general("General Options");
    // clang-format off
    options_general.add_options()
            ("help,h", "Print help")
            ("version,V", "Print version");
    // clang-format on

    // Define visible options
    po::options_description visible_options;
    visible_options.add(options_general);

    // Define hidden positional options (not shown in Boost's program options help message)
    po::options_description hidden_positional_options;
    // clang-format off
    hidden_positional_options.add_options()(
            "archive-path",
            po::value<string>(&m_archive_path)
    );
    // clang-format on
    po::positional_options_description positional_options_description;
    positional_options_description.add("archive-path", 1);

    // Aggregate all options
    po::options_description all_options;
    all_options.add(options_general);
    all_options.add(hidden_positional_options);

    // Parse options
    try {
        // Parse options specified on the command line
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                            .options(all_options)
                                            .positional(positional_options_description)
                                            .allow_unregistered()
                                            .run();
        po::variables_map parsed_command_line_options;
        store(parsed, parsed_command_line_options);
        notify(parsed_command_line_options);

        // Handle --help
        if (parsed_command_line_options.count("help")) {
            if (argc > 2) {
                SPDLOG_WARN("Ignoring all options besides --help.");
            }

            print_basic_usage();
            cerr << "Examples:" << endl;
            cerr << R"(  # Search ARCHIVE_PATH for " ERROR " )" << endl;
            cerr << "  " << get_program_name() << R"( ARCHIVE_PATH " ERROR ")" << endl;
            cerr << "Additional options including aggregation options and output destination are "
                 << "passed in a msgpack object via stdin." << endl;
            cerr << visible_options << endl;
            return ParsingResult::InfoCommand;
        }

        // Handle --version
        if (parsed_command_line_options.count("version")) {
            cerr << static_cast<char const*>(cVersion) << endl;
            return ParsingResult::InfoCommand;
        }

        std::string msgpack_buffer{std::istreambuf_iterator<char>(std::cin), {}};
        nlohmann::json extended_arguments
                = nlohmann::json::from_msgpack(msgpack_buffer.begin(), msgpack_buffer.end());
        // Detecting duplicate arguments must happen before parsing the extended search
        // arguments to properly detect boolean switch arguments passed on the command line.
        parse_extended_search_arguments(extended_arguments);

        // Validate archive path was specified
        if (m_archive_path.empty()) {
            throw invalid_argument("ARCHIVE_PATH not specified or empty.");
        }

        // Validate wildcard string
        if (m_search_string.empty()) {
            throw invalid_argument("Wildcard string not specified or empty.");
        }

        if (m_search_begin_ts > m_search_end_ts) {
            throw invalid_argument(
                    "Timestamp range is invalid - begin timestamp is after end timestamp."
            );
        }

        bool aggregation_was_specified
                = m_do_count_by_time_aggregation || m_do_count_results_aggregation;
        if (aggregation_was_specified && OutputHandlerType::Reducer != m_output_handler_type) {
            throw invalid_argument(
                    "Aggregations are only supported with the reducer output handler."
            );
        } else if ((false == aggregation_was_specified
                    && OutputHandlerType::Reducer == m_output_handler_type))
        {
            throw invalid_argument("The reducer output handler currently only supports count and "
                                   "count-by-time aggregations.");
        }
    } catch (exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        cerr << "Try " << get_program_name() << " --help for detailed usage instructions" << endl;
        return ParsingResult::Failure;
    }

    return ParsingResult::Success;
}

void CommandLineArguments::parse_extended_search_arguments(
        nlohmann::json const& extended_search_arguments
) {
    constexpr char cQueryString[] = "query_string";
    constexpr char cMaxNumResults[] = "max_num_results";
    constexpr char cBeginTimestamp[] = "begin_timestamp";
    constexpr char cEndTimestamp[] = "end_timestamp";
    constexpr char cIgnoreCase[] = "ignore_case";
    constexpr char cPathFilter[] = "path_filter";
    constexpr char cNetworkAddress[] = "network_address";
    constexpr char cMongodbDestination[] = "mongodb_destination";
    constexpr char cAggregationConfig[] = "aggregation_config";

    if (has_non_null_field(extended_search_arguments, cQueryString)) {
        m_search_string = extended_search_arguments[cQueryString];
    }

    if (has_non_null_field(extended_search_arguments, cMaxNumResults)) {
        m_max_num_results = extended_search_arguments[cMaxNumResults];
    }

    if (has_non_null_field(extended_search_arguments, cBeginTimestamp)) {
        m_search_begin_ts = extended_search_arguments[cBeginTimestamp];
    }

    if (has_non_null_field(extended_search_arguments, cEndTimestamp)) {
        m_search_end_ts = extended_search_arguments[cEndTimestamp];
    }

    if (has_non_null_field(extended_search_arguments, cIgnoreCase)) {
        m_ignore_case = extended_search_arguments[cIgnoreCase];
    }

    if (has_non_null_field(extended_search_arguments, cPathFilter)) {
        m_file_path = extended_search_arguments[cPathFilter];
        if (m_file_path.empty()) {
            throw invalid_argument("file-path cannot be an empty string.");
        }
    }

    if (has_non_null_field(extended_search_arguments, cNetworkAddress)) {
        parse_network_dest_output_handler_options(extended_search_arguments[cNetworkAddress]);
    }

    if (has_non_null_field(extended_search_arguments, cMongodbDestination)) {
        parse_results_cache_output_handler_options(extended_search_arguments[cMongodbDestination]);
    }

    if (has_non_null_field(extended_search_arguments, cAggregationConfig)) {
        parse_extended_aggregation_arguments(extended_search_arguments[cAggregationConfig]);
    }

    if (OutputHandlerType::None == m_output_handler_type) {
        throw std::invalid_argument(
                "No output handler configuration was provided in the extended search arguments"
        );
    }
}

void CommandLineArguments::parse_extended_aggregation_arguments(
        nlohmann::json const& extended_aggregation_arguments
) {
    constexpr char cJobId[] = "job_id";
    constexpr char cReducerHost[] = "reducer_host";
    constexpr char cReducerPort[] = "reducer_port";
    constexpr char cDoCountAggregation[] = "do_count_aggregation";
    constexpr char cCountByTimeBucketSize[] = "count_by_time_bucket_size";

    if (OutputHandlerType::None != m_output_handler_type) {
        throw std::invalid_argument("multiple conflicting output handlers specified.");
    }
    m_output_handler_type = OutputHandlerType::Reducer;

    if (false == has_non_null_field(extended_aggregation_arguments, cReducerHost)) {
        throw std::invalid_argument("host must be specified.");
    }
    m_reducer_host = extended_aggregation_arguments[cReducerHost];
    if (m_reducer_host.empty()) {
        throw std::invalid_argument("host cannot be an empty string.");
    }

    if (false == has_non_null_field(extended_aggregation_arguments, cReducerPort)) {
        throw std::invalid_argument("port must be specified.");
    }
    m_reducer_port = extended_aggregation_arguments[cReducerPort];
    if (m_reducer_port <= 0) {
        throw std::invalid_argument("port must be greater than zero.");
    }

    if (false == has_non_null_field(extended_aggregation_arguments, cJobId)) {
        throw std::invalid_argument("job-id must be specified.");
    }
    m_job_id = extended_aggregation_arguments[cJobId];
    if (m_job_id < 0) {
        throw std::invalid_argument("job-id cannot be negative.");
    }

    if (has_non_null_field(extended_aggregation_arguments, cCountByTimeBucketSize)) {
        m_do_count_by_time_aggregation = true;
        m_count_by_time_bucket_size = extended_aggregation_arguments[cCountByTimeBucketSize];
        if (m_count_by_time_bucket_size <= 0) {
            throw std::invalid_argument("Value for count-by-time must be greater than zero.");
        }
    }

    if (has_non_null_field(extended_aggregation_arguments, cDoCountAggregation)) {
        m_do_count_results_aggregation = true;
    }

    if (m_do_count_by_time_aggregation && m_do_count_results_aggregation) {
        throw std::invalid_argument("The count-by-time and count options are mutually exclusive.");
    } else if (false == m_do_count_by_time_aggregation && false == m_do_count_results_aggregation) {
        throw std::invalid_argument(
                "Aggregation was requested but no aggregation operation specified."
        );
    }
}

void CommandLineArguments::parse_network_dest_output_handler_options(nlohmann::json const& options
) {
    if (false == options.is_array() || 2 != options.size()) {
        throw std::invalid_argument("invalid shape for Network output handler options.");
    }
    m_network_dest_host = options[0];
    m_network_dest_port = options[1];

    if (OutputHandlerType::None != m_output_handler_type) {
        throw std::invalid_argument("multiple conflicting output handlers specified.");
    }
    m_output_handler_type = OutputHandlerType::Network;

    if (m_network_dest_host.empty()) {
        throw std::invalid_argument("host cannot be an empty string.");
    }

    if (m_network_dest_port <= 0) {
        throw std::invalid_argument("port must be greater than zero.");
    }
}

void CommandLineArguments::parse_results_cache_output_handler_options(nlohmann::json const& options
) {
    if (false == options.is_array() || 2 != options.size()) {
        throw std::invalid_argument("invalid shape for Results Cache output handler options.");
    }
    m_mongodb_uri = options[0];
    m_mongodb_collection = options[1];

    if (OutputHandlerType::None != m_output_handler_type) {
        throw std::invalid_argument("multiple conflicting output handlers specified.");
    }
    m_output_handler_type = OutputHandlerType::ResultsCache;

    if (m_mongodb_uri.empty()) {
        throw std::invalid_argument("uri cannot be an empty string.");
    }

    if (m_mongodb_collection.empty()) {
        throw std::invalid_argument("collection cannot be an empty string.");
    }

    if (0 == m_batch_size) {
        throw std::invalid_argument("batch-size cannot be 0.");
    }

    if (0 == m_max_num_results) {
        throw std::invalid_argument("max-num-results cannot be 0.");
    }
}

void CommandLineArguments::print_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " [OPTIONS]"
         << R"( ARCHIVE_PATH )" << endl;
}
}  // namespace clp::clo
