#include "CommandLineArguments.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include "../cli_utils.hpp"
#include "../reducer/types.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../version.hpp"
#include "type_utils.hpp"

namespace po = boost::program_options;
using std::cerr;
using std::endl;
using std::exception;
using std::invalid_argument;
using std::string;
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
    config_file_path += cDefaultConfigFilename;
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
            "Perform a count aggregation (count the number of results)"
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
    all_options.add(options_general);
    all_options.add(options_match_control);
    all_options.add(options_aggregation);
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

        // Handle config-file manually since Boost won't set it until we call notify, and we can't
        // call notify until we parse the config file
        if (parsed_command_line_options.count("config-file")) {
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

        // Handle --help
        if (parsed_command_line_options.count("help")) {
            if (argc > 2) {
                SPDLOG_WARN("Ignoring all options besides --help.");
            }

            print_basic_usage();
            cerr << "OUTPUT_HANDLER is one of:" << endl;
            cerr << "  results-cache - Output to the results cache" << endl;
            cerr << "  reducer - Output to the reducer" << endl;
            cerr << endl;

            cerr << "Examples:" << endl;
            cerr << R"(  # Search ARCHIVE_PATH for " ERROR " and send results to)"
                    R"( mongodb://127.0.0.1:27017/test "result" collection )"
                 << endl;
            cerr << "  " << get_program_name()
                 << R"( ARCHIVE_PATH " ERROR " results-cache)"
                    R"( --uri mongodb://127.0.0.1:27017/test --collection result)"
                 << endl;
            cerr << endl;

            cerr << "Options can be specified on the command line or through a configuration file."
                 << endl;
            cerr << visible_options << endl;
            return ParsingResult::InfoCommand;
        }

        // Handle --version
        if (parsed_command_line_options.count("version")) {
            cerr << cVersion << endl;
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
        if (parsed_command_line_options.count("teq")) {
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
            if (parsed_command_line_options.count("tgt") + parsed_command_line_options.count("tge")
                > 1)
            {
                throw invalid_argument("--tgt cannot be used with --tge.");
            }

            // Set m_search_begin_ts
            if (parsed_command_line_options.count("tgt")) {
                m_search_begin_ts = parsed_command_line_options["tgt"].as<epochtime_t>() + 1;
            } else if (parsed_command_line_options.count("tge")) {
                m_search_begin_ts = parsed_command_line_options["tge"].as<epochtime_t>();
            }

            if (parsed_command_line_options.count("tlt") + parsed_command_line_options.count("tle")
                > 1)
            {
                throw invalid_argument("--tlt cannot be used with --tle.");
            }

            // Set m_search_end_ts
            if (parsed_command_line_options.count("tlt")) {
                m_search_end_ts = parsed_command_line_options["tlt"].as<epochtime_t>() - 1;
            } else if (parsed_command_line_options.count("tle")) {
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

        // Validate output-handler
        if (parsed_command_line_options.count("output-handler") == 0) {
            throw invalid_argument("OUTPUT_HANDLER not specified.");
        }
        if ("reducer" == output_handler_name) {
            m_output_handler_type = OutputHandlerType::Reducer;
        } else if ("results-cache" == output_handler_name) {
            m_output_handler_type = OutputHandlerType::ResultsCache;
        } else if (output_handler_name.empty()) {
            throw invalid_argument("OUTPUT_HANDLER cannot be an empty string.");
        } else {
            throw invalid_argument("Unknown OUTPUT_HANDLER: " + output_handler_name);
        }

        switch (m_output_handler_type) {
            case OutputHandlerType::Reducer:
                parse_reducer_output_handler_options(
                        options_reducer_output_handler,
                        parsed.options,
                        parsed_command_line_options
                );
                break;
            case OutputHandlerType::ResultsCache:
                parse_results_cache_output_handler_options(
                        options_results_cache_output_handler,
                        parsed.options,
                        parsed_command_line_options
                );
                break;
            default:
                throw invalid_argument(
                        "Unhandled OutputHandlerType="
                        + std::to_string(enum_to_underlying_type(m_output_handler_type))
                );
        }

        if (m_do_count_results_aggregation && OutputHandlerType::Reducer != m_output_handler_type) {
            throw invalid_argument(
                    "Aggregations are only supported with the reducer output handler."
            );
        } else if ((false == m_do_count_results_aggregation
                    && OutputHandlerType::Reducer == m_output_handler_type))
        {
            throw invalid_argument(
                    "The reducer output handler currently only supports the count aggregation."
            );
        }
    } catch (exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        cerr << "Try " << get_program_name() << " --help for detailed usage instructions" << endl;
        return ParsingResult::Failure;
    }

    return ParsingResult::Success;
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
    cerr << "Usage: " << get_program_name() << " [OPTIONS]"
         << R"( ARCHIVE_PATH "WILDCARD STRING" OUTPUT_HANDLER [OUTPUT_HANDLER_OPTIONS])" << endl;
}
}  // namespace clp::clo
