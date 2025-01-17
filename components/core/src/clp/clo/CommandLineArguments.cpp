#include "CommandLineArguments.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options.hpp>

#include "../../reducer/types.hpp"
#include "../cli_utils.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../version.hpp"

namespace po = boost::program_options;
using std::cerr;
using std::endl;
using std::exception;
using std::invalid_argument;
using std::string;
using std::string_view;
using std::vector;

namespace clp::clo {
CommandLineArgumentsBase::ParsingResult
CommandLineArguments::parse_arguments(int argc, char const* argv[]) {
    // Print out basic usage if user doesn't specify any options
    if (1 == argc) {
        print_basic_usage();
        return ParsingResult::Failure;
    }

    // Define general options
    po::options_description options_general("General Options");
    // Set default configuration file path to "$HOME/cDefaultConfigFilename" (Linux environment) if
    // $HOME is set, or "./cDefaultConfigFilename" otherwise
    string config_file_path;
    char const* home_environment_var_value = getenv("HOME");
    if (nullptr == home_environment_var_value) {
        config_file_path = "./";
    } else {
        config_file_path = home_environment_var_value;
        config_file_path += '/';
    }
    config_file_path += static_cast<char const*>(cDefaultConfigFilename);
    string global_metadata_db_config_file_path;
    // clang-format off
    options_general.add_options()
            ("help,h", "Print help")
            ("version,V", "Print version")
            (
                    "config-file",
                    po::value<string>(&config_file_path)
                            ->value_name("FILE")
                            ->default_value(config_file_path),
                    "Use configuration options from FILE"
            );
    // clang-format on

    po::options_description general_positional_options;
    char command_input{};
    general_positional_options.add_options()("command", po::value<char>(&command_input))(
            "command-args",
            po::value<vector<string>>()
    );
    po::positional_options_description general_positional_options_description;
    general_positional_options_description.add("command", 1);
    general_positional_options_description.add("command-args", -1);

    // Aggregate all options
    po::options_description all_options;
    all_options.add(options_general);
    all_options.add(general_positional_options);

    // Parse options
    try {
        // Parse options specified on the command line
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                            .options(all_options)
                                            .positional(general_positional_options_description)
                                            .allow_unregistered()
                                            .run();
        po::variables_map parsed_command_line_options;
        store(parsed, parsed_command_line_options);

        // Handle config-file manually since Boost won't set it until we call notify, and we can't
        // call notify until we parse the config file
        if (0 != parsed_command_line_options.count("config-file")) {
            config_file_path = parsed_command_line_options["config-file"].as<string>();
        }

        // Parse options specified through the config file
        // NOTE: Command line arguments will take priority over config file since they are parsed
        // first and Boost doesn't replace existing options
        std::ifstream config_file(config_file_path);
        if (config_file.is_open()) {
            // Allow unrecognized options in configuration file since some of them may be
            // exclusively for clp or other applications
            po::parsed_options parsed_config_file
                    = po::parse_config_file(config_file, all_options, true);
            store(parsed_config_file, parsed_command_line_options);
            config_file.close();
        }

        notify(parsed_command_line_options);

        // Handle --version
        if (0 != parsed_command_line_options.count("version")) {
            cerr << static_cast<char const*>(cVersion) << endl;
            return ParsingResult::InfoCommand;
        }

        // Validate command
        if (parsed_command_line_options.count("command") == 0) {
            // Handle --help
            if (0 != parsed_command_line_options.count("help")) {
                if (argc > 2) {
                    SPDLOG_WARN("Ignoring all options besides --help.");
                }

                print_basic_usage();
                cerr << "COMMAND is one of:" << endl;
                cerr << " " << enum_to_underlying_type(Command::Search) << " - search" << endl;
                cerr << " " << enum_to_underlying_type(Command::ExtractIr) << " - extract IR"
                     << endl;
                cerr << endl;
                cerr << "Try " << get_program_name() << " "
                     << enum_to_underlying_type(Command::Search) << " --help OR "
                     << get_program_name() << " " << enum_to_underlying_type(Command::ExtractIr)
                     << " --help for command-specific details." << endl;
                cerr << endl;

                cerr << "Options can be specified on the command line or through a configuration "
                        "file."
                     << endl;
                po::options_description visible_options;
                visible_options.add(options_general);
                cerr << visible_options << endl;
                return ParsingResult::InfoCommand;
            }

            throw invalid_argument("COMMAND not specified.");
        }
        switch (command_input) {
            case enum_to_underlying_type(Command::Search):
                m_command = static_cast<Command>(command_input);
                return parse_search_arguments(
                        options_general,
                        parsed_command_line_options,
                        parsed.options,
                        argc
                );
            case enum_to_underlying_type(Command::ExtractIr):
                m_command = static_cast<Command>(command_input);
                return parse_ir_extraction_arguments(
                        options_general,
                        parsed_command_line_options,
                        parsed.options,
                        argc
                );
            default:
                throw invalid_argument(string("Unknown command '") + command_input + "'");
        }
    } catch (exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        cerr << "Try " << get_program_name() << " --help for detailed usage instructions" << endl;
        return ParsingResult::Failure;
    }
}

auto CommandLineArguments::parse_ir_extraction_arguments(
        po::options_description const& options_general,
        po::variables_map& parsed_command_line_options,
        vector<po::option> const& options,
        int argc
) -> CommandLineArgumentsBase::ParsingResult {
    // Define IR extraction options
    po::options_description options_ir_extraction("IR Extraction Options");
    // clang-format off
    options_ir_extraction.add_options()(
            "target-size",
            po::value<size_t>(&m_ir_target_size)
                    ->value_name("SIZE")
                    ->default_value(m_ir_target_size),
            "Target size (B) for each IR chunk before a new chunk is created"
    )(
            "print-ir-stats",
            po::bool_switch(&m_print_ir_stats),
            "Print statistics (ndjson) about each IR file after it's extracted"
    );
    // clang-format on

    // Define visible options
    po::options_description visible_options;
    visible_options.add(options_general);
    visible_options.add(options_ir_extraction);

    // Define hidden positional options (not shown in Boost's program options help message)
    po::options_description hidden_positional_options;
    // clang-format off
    hidden_positional_options.add_options()(
            "archive-path",
            po::value<string>(&m_archive_path)
    )(
            "file-split-id",
            po::value<string>(&m_file_split_id)
    )(
            "output-dir",
            po::value<string>(&m_ir_output_dir)
    )(
            "mongodb-uri",
            po::value<string>(&m_ir_mongodb_uri)
    )(
            "mongodb-collection",
            po::value<string>(&m_ir_mongodb_collection)
    );
    // clang-format on
    po::positional_options_description positional_options_description;
    positional_options_description.add("archive-path", 1);
    positional_options_description.add("file-split-id", 1);
    positional_options_description.add("output-dir", 1);
    positional_options_description.add("mongodb-uri", 1);
    positional_options_description.add("mongodb-collection", 1);

    // Aggregate all options
    po::options_description all_options;
    all_options.add(options_ir_extraction);
    all_options.add(hidden_positional_options);

    // Parse extraction options
    auto extraction_options{po::collect_unrecognized(options, po::include_positional)};
    // Erase the command from the beginning
    extraction_options.erase(extraction_options.begin());
    po::store(
            po::command_line_parser(extraction_options)
                    .options(all_options)
                    .positional(positional_options_description)
                    .run(),
            parsed_command_line_options
    );
    notify(parsed_command_line_options);

    // Handle --help
    if (0 != parsed_command_line_options.count("help")) {
        if (argc > 3) {
            SPDLOG_WARN("Ignoring all options besides --help.");
        }

        print_ir_extraction_basic_usage();
        cerr << "Examples:" << endl;
        cerr << R"(  # Extract file (split) with ID "8cf8d8f2-bf3f-42a2-90b2-6bc4ed0a36b4" from)"
             << endl;
        cerr << R"(  # ARCHIVE_PATH as IR into OUTPUT_DIR from ARCHIVE_PATH, and send the metadata)"
             << endl;
        cerr << R"(  # to mongodb://127.0.0.1:27017/test result collection)" << endl;
        cerr << "  " << get_program_name()
             << " i ARCHIVE_PATH 8cf8d8f2-bf3f-42a2-90b2-6bc4ed0a36b4 OUTPUT_DIR "
                "mongodb://127.0.0.1:27017/test result"
             << endl;
        cerr << endl;

        cerr << "Options can be specified on the command line or through a configuration "
                "file."
             << endl;
        cerr << visible_options << endl;
        return ParsingResult::InfoCommand;
    }

    // Validate input arguments
    if (m_archive_path.empty()) {
        throw invalid_argument("ARCHIVE_PATH not specified or empty.");
    }

    if (m_file_split_id.empty()) {
        throw invalid_argument("FILE_SPLIT_ID not specified or empty.");
    }

    if (m_ir_output_dir.empty()) {
        throw invalid_argument("OUTPUT_DIR not specified or empty.");
    }

    if (m_ir_mongodb_uri.empty()) {
        throw invalid_argument("URI not specified or empty.");
    }

    if (m_ir_mongodb_collection.empty()) {
        throw invalid_argument("COLLECTION not specified or empty.");
    }
    return ParsingResult::Success;
}

auto CommandLineArguments::parse_search_arguments(
        po::options_description const& options_general,
        po::variables_map& parsed_command_line_options,
        vector<po::option> const& options,
        int argc
) -> CommandLineArgumentsBase::ParsingResult {
    // Define match controls
    po::options_description options_match_control("Match Controls");
    options_match_control.add_options()(
            "tgt",
            po::value<epochtime_t>()->value_name("TS"),
            "Find messages with UNIX timestamp >  TS ms"
    )(
            "tge",
            po::value<epochtime_t>()->value_name("TS"),
            "Find messages with UNIX timestamp >= TS ms"
    )(
            "teq",
            po::value<epochtime_t>()->value_name("TS"),
            "Find messages with UNIX timestamp == TS ms"
    )(
            "tlt",
            po::value<epochtime_t>()->value_name("TS"),
            "Find messages with UNIX timestamp <  TS ms"
    )(
            "tle",
            po::value<epochtime_t>()->value_name("TS"),
            "Find messages with UNIX timestamp <= TS ms"
    )(
            "ignore-case,i",
            po::bool_switch(&m_ignore_case),
            "Ignore case distinctions in both WILDCARD STRING and the input files"
    )(
            "file-path",
            po::value<string>(&m_file_path)->value_name("PATH"),
            "Limit search to files with the path PATH"
    );

    po::options_description options_aggregation("Aggregation Options");
    // clang-format off
    options_aggregation.add_options()(
            "count",
            po::bool_switch(&m_do_count_results_aggregation),
            "Count the number of results"
    )(
            "count-by-time",
            po::value<int64_t>(&m_count_by_time_bucket_size)->value_name("SIZE"),
            "Count the number of results in each time span of the given size (ms)"
    );
    // clang-format on

    po::options_description options_network_output_handler("Network Output Handler Options");
    // clang-format off
    options_network_output_handler.add_options()(
            "host",
            po::value<string>(&m_network_dest_host)->value_name("HOST"),
            "The host to send the results to"
    )(
            "port",
            po::value<int>(&m_network_dest_port)->value_name("PORT"),
            "The port to send the results to"
    );
    // clang-format on

    po::options_description options_reducer_output_handler("Reducer Output Handler Options");
    options_reducer_output_handler.add_options()(
            "host",
            po::value<string>(&m_reducer_host)->value_name("HOST"),
            "Host the reducer is running on"
    )(
            "port",
            po::value<int>(&m_reducer_port)->value_name("PORT"),
            "Port the reducer is listening on"
    )(
            "job-id",
            po::value<reducer::job_id_t>(&m_job_id)->value_name("ID"),
            "Job ID for the requested aggregation operation"
    );

    po::options_description options_results_cache_output_handler(
            "Results Cache Output Handler Options"
    );
    // clang-format off
    options_results_cache_output_handler.add_options()(
            "uri",
            po::value<string>(&m_mongodb_uri),
            "MongoDB URI for the results cache"
    )(
            "collection",
            po::value<string>(&m_mongodb_collection),
            "MongoDB collection to output to"
    )(
            "batch-size,b",
            po::value<uint64_t>(&m_batch_size)->value_name("SIZE")->default_value(m_batch_size),
            "The number of documents to insert into MongoDB per batch"
    )(
            "max-num-results,m",
            po::value<uint64_t>(&m_max_num_results)->value_name("NUM")->
                    default_value(m_max_num_results),
            "The maximum number of results to output"
    );
    // clang-format on

    // Define visible options
    po::options_description visible_options;
    visible_options.add(options_general);
    visible_options.add(options_match_control);
    visible_options.add(options_aggregation);
    visible_options.add(options_network_output_handler);
    visible_options.add(options_results_cache_output_handler);
    visible_options.add(options_reducer_output_handler);

    // Define hidden positional options (not shown in Boost's program options help message)
    po::options_description hidden_positional_options;
    string output_handler_name;
    // clang-format off
    hidden_positional_options.add_options()(
            "archive-path",
            po::value<string>(&m_archive_path)
    )(
            "wildcard-string",
            po::value<string>(&m_search_string)
    )(
            "output-handler",
            po::value<string>(&output_handler_name)
    )(
            "output-handler-args",
            po::value<vector<string>>()
    );
    // clang-format on
    po::positional_options_description positional_options_description;
    positional_options_description.add("archive-path", 1);
    positional_options_description.add("wildcard-string", 1);
    positional_options_description.add("output-handler", 1);
    positional_options_description.add("output-handler-args", -1);

    // Aggregate all options
    po::options_description all_options;
    all_options.add(options_match_control);
    all_options.add(options_aggregation);
    all_options.add(hidden_positional_options);

    // Parse options
    auto search_options{po::collect_unrecognized(options, po::include_positional)};
    search_options.erase(search_options.begin());
    auto parsed{po::command_line_parser(search_options)
                        .options(all_options)
                        .positional(positional_options_description)
                        .allow_unregistered()
                        .run()};
    po::store(parsed, parsed_command_line_options);

    notify(parsed_command_line_options);

    constexpr string_view cNetworkOutputHandlerName{"network"};
    constexpr string_view cReducerOutputHandlerName{"reducer"};
    constexpr string_view cResultsCacheOutputHandlerName{"results-cache"};

    // Handle --help
    if (0 != parsed_command_line_options.count("help")) {
        if (argc > 3) {
            SPDLOG_WARN("Ignoring all options besides --help.");
        }

        print_search_basic_usage();
        cerr << "OUTPUT_HANDLER is one of:" << endl;
        cerr << "  " << cNetworkOutputHandlerName << " - Output to a network destination" << endl;
        cerr << "  " << cResultsCacheOutputHandlerName << " - Output to the results cache" << endl;
        cerr << "  " << cReducerOutputHandlerName << " - Output to the reducer" << endl;
        cerr << endl;

        cerr << "Examples:" << endl;
        cerr << R"(  # Search ARCHIVE_PATH for " ERROR " and send results to )"
                "a network destination"
             << endl;
        cerr << "  " << get_program_name() << R"( ARCHIVE_PATH " ERROR ")"
             << " " << cNetworkOutputHandlerName << " --host localhost --port 18000" << endl;
        cerr << endl;

        cerr << R"(  # Search ARCHIVE_PATH for " ERROR " and output the results )"
                "by performing a count aggregation"
             << endl;
        cerr << "  " << get_program_name() << R"( ARCHIVE_PATH " ERROR ")"
             << " " << cReducerOutputHandlerName << " --count"
             << " --host localhost --port 14009 --job-id 1" << endl;
        cerr << endl;

        cerr << R"(  # Search ARCHIVE_PATH for " ERROR " and send results to)"
                R"( mongodb://127.0.0.1:27017/test "result" collection )"
             << endl;
        cerr << "  " << get_program_name() << R"( ARCHIVE_PATH " ERROR ")"
             << " " << cResultsCacheOutputHandlerName
             << R"( --uri mongodb://127.0.0.1:27017/test --collection result)" << endl;
        cerr << endl;

        cerr << "Options can be specified on the command line or through a configuration file."
             << endl;
        cerr << visible_options << endl;
        return ParsingResult::InfoCommand;
    }

    // Validate archive path was specified
    if (m_archive_path.empty()) {
        throw invalid_argument("ARCHIVE_PATH not specified or empty.");
    }

    // Validate wildcard string
    if (m_search_string.empty()) {
        throw invalid_argument("Wildcard string not specified or empty.");
    }

    // Validate timestamp range and compute m_search_begin_ts and m_search_end_ts
    if (0 != parsed_command_line_options.count("teq")) {
        if (parsed_command_line_options.count("tgt") + parsed_command_line_options.count("tge")
                    + parsed_command_line_options.count("tlt")
                    + parsed_command_line_options.count("tle")
            > 0)
        {
            throw invalid_argument(
                    "--teq cannot be specified with any other timestamp filtering option."
            );
        }

        m_search_begin_ts = parsed_command_line_options["teq"].as<epochtime_t>();
        m_search_end_ts = parsed_command_line_options["teq"].as<epochtime_t>();
    } else {
        if (parsed_command_line_options.count("tgt") + parsed_command_line_options.count("tge") > 1)
        {
            throw invalid_argument("--tgt cannot be used with --tge.");
        }

        // Set m_search_begin_ts
        if (0 != parsed_command_line_options.count("tgt")) {
            m_search_begin_ts = parsed_command_line_options["tgt"].as<epochtime_t>() + 1;
        } else if (0 != parsed_command_line_options.count("tge")) {
            m_search_begin_ts = parsed_command_line_options["tge"].as<epochtime_t>();
        }

        if (parsed_command_line_options.count("tlt") + parsed_command_line_options.count("tle") > 1)
        {
            throw invalid_argument("--tlt cannot be used with --tle.");
        }

        // Set m_search_end_ts
        if (0 != parsed_command_line_options.count("tlt")) {
            m_search_end_ts = parsed_command_line_options["tlt"].as<epochtime_t>() - 1;
        } else if (0 != parsed_command_line_options.count("tle")) {
            m_search_end_ts = parsed_command_line_options["tle"].as<epochtime_t>();
        }

        if (m_search_begin_ts > m_search_end_ts) {
            throw invalid_argument(
                    "Timestamp range is invalid - begin timestamp is after end timestamp."
            );
        }
    }

    // Validate file-path
    if (parsed_command_line_options.count("file-path") > 0 && m_file_path.empty()) {
        throw invalid_argument("file-path cannot be an empty string.");
    }

    // Validate count by time bucket size
    if (parsed_command_line_options.count("count-by-time") > 0) {
        m_do_count_by_time_aggregation = true;
        if (m_count_by_time_bucket_size <= 0) {
            throw std::invalid_argument("Value for count-by-time must be greater than zero.");
        }
    }

    // Validate output-handler
    if (parsed_command_line_options.count("output-handler") == 0) {
        throw invalid_argument("OUTPUT_HANDLER not specified.");
    }
    if (cNetworkOutputHandlerName == output_handler_name) {
        m_output_handler_type = OutputHandlerType::Network;
        parse_network_dest_output_handler_options(
                options_network_output_handler,
                parsed.options,
                parsed_command_line_options
        );
    } else if (cReducerOutputHandlerName == output_handler_name) {
        m_output_handler_type = OutputHandlerType::Reducer;
        parse_reducer_output_handler_options(
                options_reducer_output_handler,
                parsed.options,
                parsed_command_line_options
        );
    } else if (cResultsCacheOutputHandlerName == output_handler_name) {
        m_output_handler_type = OutputHandlerType::ResultsCache;
        parse_results_cache_output_handler_options(
                options_results_cache_output_handler,
                parsed.options,
                parsed_command_line_options
        );
    } else if (output_handler_name.empty()) {
        throw invalid_argument("OUTPUT_HANDLER cannot be an empty string.");
    } else {
        throw invalid_argument("Unknown OUTPUT_HANDLER: " + output_handler_name);
    }

    bool const aggregation_was_specified
            = m_do_count_by_time_aggregation || m_do_count_results_aggregation;
    if (aggregation_was_specified && OutputHandlerType::Reducer != m_output_handler_type) {
        throw invalid_argument("Aggregations are only supported with the reducer output handler.");
    }
    if ((false == aggregation_was_specified && OutputHandlerType::Reducer == m_output_handler_type))
    {
        throw invalid_argument("The reducer output handler currently only supports count and "
                               "count-by-time aggregations.");
    }

    if (m_do_count_by_time_aggregation && m_do_count_results_aggregation) {
        throw std::invalid_argument(
                "The --count-by-time and --count options are mutually exclusive."
        );
    }
    return ParsingResult::Success;
}

void CommandLineArguments::parse_network_dest_output_handler_options(
        po::options_description const& options_description,
        std::vector<po::option> const& options,
        po::variables_map& parsed_options
) {
    clp::parse_unrecognized_options(options_description, options, parsed_options);

    if (parsed_options.count("host") == 0) {
        throw std::invalid_argument("host must be specified.");
    }
    if (m_network_dest_host.empty()) {
        throw std::invalid_argument("host cannot be an empty string.");
    }

    if (parsed_options.count("port") == 0) {
        throw std::invalid_argument("port must be specified.");
    }
    if (m_network_dest_port <= 0) {
        throw std::invalid_argument("port must be greater than zero.");
    }
}

void CommandLineArguments::parse_reducer_output_handler_options(
        po::options_description const& options_description,
        vector<po::option> const& options,
        po::variables_map& parsed_options
) {
    parse_unrecognized_options(options_description, options, parsed_options);

    if (parsed_options.count("host") == 0) {
        throw invalid_argument("host must be specified.");
    }
    if (m_reducer_host.empty()) {
        throw invalid_argument("host cannot be an empty string.");
    }

    if (parsed_options.count("port") == 0) {
        throw invalid_argument("port must be specified.");
    }
    if (m_reducer_port <= 0) {
        throw invalid_argument("port must be greater than zero.");
    }

    if (parsed_options.count("job-id") == 0) {
        throw invalid_argument("job-id must be specified.");
    }
    if (m_job_id < 0) {
        throw invalid_argument("job-id cannot be negative.");
    }
}

void CommandLineArguments::parse_results_cache_output_handler_options(
        po::options_description const& options_description,
        vector<po::option> const& options,
        po::variables_map& parsed_options
) {
    parse_unrecognized_options(options_description, options, parsed_options);

    // Validate mongodb uri was specified
    if (parsed_options.count("uri") == 0) {
        throw invalid_argument("uri must be specified.");
    }
    if (m_mongodb_uri.empty()) {
        throw invalid_argument("uri cannot be an empty string.");
    }

    // Validate mongodb collection was specified
    if (parsed_options.count("collection") == 0) {
        throw invalid_argument("collection must be specified.");
    }
    if (m_mongodb_collection.empty()) {
        throw invalid_argument("collection cannot be an empty string.");
    }

    if (0 == m_batch_size) {
        throw invalid_argument("batch-size cannot be 0.");
    }

    if (0 == m_max_num_results) {
        throw invalid_argument("max-num-results cannot be 0.");
    }
}

void CommandLineArguments::print_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " [OPTIONS] COMMAND [COMMAND ARGUMENTS]" << endl;
}

void CommandLineArguments::print_search_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " " << enum_to_underlying_type(Command::Search)
         << R"( [OPTIONS] ARCHIVE_PATH "WILDCARD STRING" OUTPUT_HANDLER [OUTPUT_HANDLER_OPTIONS])"
         << endl;
}

void CommandLineArguments::print_ir_extraction_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " " << enum_to_underlying_type(Command::ExtractIr)
         << R"( [OPTIONS] ARCHIVE_PATH FILE_SPLIT_ID OUTPUT_DIR MONGODB_URI MONGODB_COLLECTION)"
         << endl;
}
}  // namespace clp::clo
