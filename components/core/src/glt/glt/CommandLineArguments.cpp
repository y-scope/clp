#include "CommandLineArguments.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>

#include <boost/filesystem/operations.hpp>
#include <boost/program_options.hpp>

#include "../Defs.h"
#include "../spdlog_with_specializations.hpp"
#include "../Utils.hpp"
#include "../version.hpp"

namespace po = boost::program_options;
using std::cerr;
using std::endl;
using std::exception;
using std::invalid_argument;
using std::string;
using std::vector;

namespace glt::glt {
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
    options_general.add_options()
            ("help,h", "Print help")
            ("version,V", "Print version")
            (
                    "config-file",
                    po::value<string>(&config_file_path)
                            ->value_name("FILE")
                            ->default_value(config_file_path),
                    "Use configuration options from FILE"
            )
            (
                    "db-config-file",
                    po::value<string>(&global_metadata_db_config_file_path)
                            ->value_name("FILE")
                            ->default_value(global_metadata_db_config_file_path),
                    "Global metadata DB YAML config"
            );

    po::options_description general_positional_options;
    char command_input;
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
        if (parsed_command_line_options.count("config-file")) {
            config_file_path = parsed_command_line_options["config-file"].as<string>();
        }

        // Parse options specified through the config file
        // NOTE: Command line arguments will take priority over config file since they are parsed
        // first and Boost doesn't replace existing options
        std::ifstream config_file(config_file_path);
        if (config_file.is_open()) {
            po::parsed_options parsed_config_file = po::parse_config_file(config_file, all_options);
            store(parsed_config_file, parsed_command_line_options);
            config_file.close();
        }

        notify(parsed_command_line_options);

        // Handle --version
        if (parsed_command_line_options.count("version")) {
            cerr << cVersion << endl;
            return ParsingResult::InfoCommand;
        }

        // Parse and validate global metadata DB config
        if (false == global_metadata_db_config_file_path.empty()) {
            try {
                m_metadata_db_config.parse_config_file(global_metadata_db_config_file_path);
            } catch (std::exception& e) {
                SPDLOG_ERROR("Failed to validate metadata database config - {}", e.what());
                return ParsingResult::Failure;
            }
        }

        // Validate command
        if (parsed_command_line_options.count("command") == 0) {
            // Handle --help
            if (parsed_command_line_options.count("help")) {
                if (argc > 2) {
                    SPDLOG_WARN("Ignoring all options besides --help.");
                }

                print_basic_usage();
                cerr << "COMMAND is one of:" << endl;
                cerr << "  c - compress" << endl;
                cerr << "  x - extract" << endl;
                cerr << "  s - search" << endl;
                cerr << endl;
                cerr << "Try " << get_program_name() << " c --help OR " << get_program_name()
                     << " x --help OR " << get_program_name()
                     << " s --help for command-specific details." << endl;
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
            case (char)Command::Compress:
            case (char)Command::Extract:
            case (char)Command::Search:
                m_command = (Command)command_input;
                break;
            default:
                throw invalid_argument(string("Unknown action '") + command_input + "'");
        }

        // Define functional options shared by extract and compression
        po::options_description options_functional("Input Options");
        options_functional.add_options()(
                "files-from,f",
                po::value<string>(&m_path_list_path)
                        ->value_name("FILE")
                        ->default_value(m_path_list_path),
                "Compress/extract files specified in FILE"
        );

        if (Command::Extract == m_command) {
            // Define extraction hidden positional options
            po::options_description extraction_positional_options;
            // clang-format off
            extraction_positional_options.add_options()
                    ("archives-dir", po::value<string>(&m_archives_dir))
                    ("output-dir", po::value<string>(&m_output_dir))
                    ("paths", po::value<vector<string>>(&m_input_paths)->composing());
            // clang-format on
            po::positional_options_description extraction_positional_options_description;
            extraction_positional_options_description.add("archives-dir", 1);
            extraction_positional_options_description.add("output-dir", 1);
            extraction_positional_options_description.add("paths", -1);

            po::options_description all_extraction_options;
            all_extraction_options.add(extraction_positional_options);
            all_extraction_options.add(options_functional);

            // Parse extraction options
            vector<string> unrecognized_options
                    = po::collect_unrecognized(parsed.options, po::include_positional);
            unrecognized_options.erase(unrecognized_options.begin());
            po::store(
                    po::command_line_parser(unrecognized_options)
                            .options(all_extraction_options)
                            .positional(extraction_positional_options_description)
                            .run(),
                    parsed_command_line_options
            );

            notify(parsed_command_line_options);

            // Handle --help
            if (parsed_command_line_options.count("help")) {
                print_extraction_basic_usage();

                cerr << "Examples:" << endl;
                cerr << "  # Extract all files from archives-dir into output-dir" << endl;
                cerr << "  " << get_program_name() << " x archives-dir output-dir" << endl;
                cerr << endl;
                cerr << "  # Extract file1.txt" << endl;
                cerr << "  " << get_program_name() << " x archives-dir output-dir file1.txt"
                     << endl;
                cerr << endl;

                po::options_description visible_options;
                visible_options.add(options_general);
                visible_options.add(options_functional);
                cerr << visible_options << endl;
                return ParsingResult::InfoCommand;
            }

            // Validate archive path is not empty
            if (m_archives_dir.empty()) {
                throw invalid_argument("ARCHIVES_DIR cannot be empty.");
            }

            // Validate an output directory was specified
            if (m_output_dir.empty()) {
                throw invalid_argument("output-dir not specified or empty.");
            }
            if (m_output_dir.back() != '/') {
                m_output_dir += '/';
            }
        } else if (Command::Compress == m_command) {
            // Define compression hidden positional options
            po::options_description compression_positional_options;
            // clang-format off
            compression_positional_options.add_options()
                    ("output-dir", po::value<string>(&m_output_dir))
                    ("input-paths", po::value<vector<string>>(&m_input_paths)->composing());
            // clang-format on
            po::positional_options_description compression_positional_options_description;
            compression_positional_options_description.add("output-dir", 1);
            compression_positional_options_description.add("input-paths", -1);

            // Define compression-specific options
            po::options_description options_compression("Compression Options");
            options_compression.add_options()(
                    "remove-path-prefix",
                    po::value<string>(&m_path_prefix_to_remove)
                            ->value_name("DIR")
                            ->default_value(m_path_prefix_to_remove),
                    "Remove the given path prefix from each compressed file/dir."
            )(
                    "target-encoded-file-size",
                    po::value<size_t>(&m_target_encoded_file_size)
                            ->value_name("SIZE")
                            ->default_value(m_target_encoded_file_size),
                    "Target size (B) for an encoded file before a new one is created"
            )(
                    "target-segment-size",
                    po::value<size_t>(&m_target_segment_uncompressed_size)
                            ->value_name("SIZE")
                            ->default_value(m_target_segment_uncompressed_size),
                    "Target uncompressed size (B) of a segment before a new one is created"
            )(
                    "target-dictionaries-size",
                    po::value<size_t>(&m_target_data_size_of_dictionaries)
                            ->value_name("SIZE")
                            ->default_value(m_target_data_size_of_dictionaries),
                    "Target size (B) for the dictionaries before a new archive is created"
            )(
                    "compression-level",
                    po::value<int>(&m_compression_level)
                            ->value_name("LEVEL")
                            ->default_value(m_compression_level),
                    "1 (fast/low compression) to 9 (slow/high compression)"
            )(
                    "print-archive-stats-progress",
                    po::bool_switch(&m_print_archive_stats_progress),
                    "Print statistics (ndjson) about each archive as it's compressed"
            )(
                    "table-combine-threshold",
                    po::value<double>(&m_combine_threshold)
                            ->value_name("VALUE")
                            ->default_value(m_combine_threshold, "0.1"),
                    "Target size (%) of a table (relative to the archive's size) for it to be"
                    " stored in the combined table"
            )(
                    "progress",
                    po::bool_switch(&m_show_progress),
                    "Show progress during compression"
            );

            po::options_description all_compression_options;
            all_compression_options.add(options_compression);
            all_compression_options.add(options_functional);
            all_compression_options.add(compression_positional_options);

            vector<string> unrecognized_options
                    = po::collect_unrecognized(parsed.options, po::include_positional);
            unrecognized_options.erase(unrecognized_options.begin());
            po::store(
                    po::command_line_parser(unrecognized_options)
                            .options(all_compression_options)
                            .positional(compression_positional_options_description)
                            .run(),
                    parsed_command_line_options
            );

            notify(parsed_command_line_options);

            // Handle --help
            if (parsed_command_line_options.count("help")) {
                print_compression_basic_usage();

                cerr << "Examples:" << endl;
                cerr << "  # Compress file1.txt and dir1 into the output dir" << endl;
                cerr << "  " << get_program_name() << " c output-dir file1.txt dir1" << endl;
                cerr << endl;

                po::options_description visible_options;
                visible_options.add(options_general);
                visible_options.add(options_functional);
                visible_options.add(options_compression);
                cerr << visible_options << endl;
                return ParsingResult::InfoCommand;
            }

            // Validate at least one input path should exist (we validate that the file isn't empty
            // later)
            if (m_input_paths.empty() && m_path_list_path.empty()) {
                throw invalid_argument("No input paths specified.");
            }

            if (m_target_encoded_file_size < 1) {
                throw invalid_argument("target-encoded-file-size must be non-zero.");
            }

            if (m_target_segment_uncompressed_size < 1) {
                throw invalid_argument("segment-size-threshold must be non-zero.");
            }

            if (m_target_data_size_of_dictionaries < 1) {
                throw invalid_argument("target-data-size-of-dictionaries must be non-zero.");
            }

            if (false == m_path_prefix_to_remove.empty()) {
                if (false == boost::filesystem::exists(m_path_prefix_to_remove)) {
                    throw invalid_argument("Specified prefix to remove does not exist.");
                }
                if (false == boost::filesystem::is_directory(m_path_prefix_to_remove)) {
                    throw invalid_argument("Specified prefix to remove is not a directory.");
                }
            }

            if (m_combine_threshold < 0 || m_combine_threshold > 100) {
                throw invalid_argument(
                        "table-combined-threshold " + std::to_string(m_combine_threshold)
                        + " is invalid - threshold must be between 0 and 100"
                );
            }

            // Validate an output directory was specified
            if (m_output_dir.empty()) {
                throw invalid_argument("output-dir not specified or empty.");
            }
            if (m_output_dir.back() != '/') {
                m_output_dir += '/';
            }
        } else if (Command::Search == m_command) {
            // Define search input options
            po::options_description options_search_input("Input Options");
            options_search_input.add_options()(
                    "file,f",
                    po::value<string>(&m_search_strings_file_path)->value_name("FILE"),
                    "Obtain wildcard strings from FILE, one per line"
            );

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
            );

            // Define visible options
            po::options_description visible_options;
            visible_options.add(options_general);
            visible_options.add(options_search_input);
            visible_options.add(options_match_control);

            // Define hidden positional options (not shown in Boost's program options help message)
            po::options_description hidden_positional_options;
            // clang-format off
            hidden_positional_options.add_options()(
                    "archives-dir",
                    po::value<string>(&m_archives_dir)
            )(
                    "wildcard-string",
                    po::value<string>(&m_search_string)
            )(
                    "file-path",
                    po::value<string>(&m_file_path)
            );
            // clang-format on
            po::positional_options_description positional_options_description;
            positional_options_description.add("archives-dir", 1);
            positional_options_description.add("wildcard-string", 1);
            positional_options_description.add("file-path", 1);

            // Aggregate all options
            po::options_description all_search_options;
            all_search_options.add(options_general);
            all_search_options.add(options_search_input);
            all_search_options.add(options_match_control);
            all_search_options.add(hidden_positional_options);

            vector<string> unrecognized_options
                    = po::collect_unrecognized(parsed.options, po::include_positional);
            unrecognized_options.erase(unrecognized_options.begin());
            po::store(
                    po::command_line_parser(unrecognized_options)
                            .options(all_search_options)
                            .positional(positional_options_description)
                            .run(),
                    parsed_command_line_options
            );

            notify(parsed_command_line_options);

            // Handle --help
            if (parsed_command_line_options.count("help")) {
                print_search_basic_usage();
                cerr << endl;

                cerr << "Examples:" << endl;
                cerr << R"(  # Search archives-dir for " ERROR ")" << endl;
                cerr << "  " << get_program_name() << R"( s archives-dir " ERROR ")" << endl;
                cerr << endl;

                cerr << "Options can be specified on the command line or through a configuration "
                        "file."
                     << endl;
                cerr << visible_options << endl;
                return ParsingResult::InfoCommand;
            }

            // Validate at least one wildcard string exists
            if (m_search_strings_file_path.empty() == false) {
                if (m_search_string.empty() == false) {
                    throw invalid_argument("Wildcard strings cannot be specified both through the "
                                           "command line and a file.");
                }
            } else if (m_search_string.empty()) {
                throw invalid_argument("Wildcard string not specified or empty.");
            }

            // Validate timestamp range and compute m_search_begin_ts and m_search_end_ts
            if (parsed_command_line_options.count("teq")) {
                if (parsed_command_line_options.count("tgt")
                            + parsed_command_line_options.count("tge")
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
                if (parsed_command_line_options.count("tgt")
                            + parsed_command_line_options.count("tge")
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

                if (parsed_command_line_options.count("tlt")
                            + parsed_command_line_options.count("tle")
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
        }
    } catch (exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        cerr << "Try " << get_program_name() << " --help for detailed usage instructions" << endl;
        return ParsingResult::Failure;
    }

    return ParsingResult::Success;
}

void CommandLineArguments::print_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " [OPTIONS] COMMAND [COMMAND ARGUMENTS]" << endl;
}

void CommandLineArguments::print_compression_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " [OPTIONS] c OUTPUT_DIR [FILE/DIR ...]" << endl;
}

void CommandLineArguments::print_extraction_basic_usage() const {
    cerr << "Usage: " << get_program_name() << " [OPTIONS] x ARCHIVES_DIR OUTPUT_DIR [FILE ...]"
         << endl;
}

void CommandLineArguments::print_search_basic_usage() const {
    cerr << "Usage: " << get_program_name()
         << R"( [OPTIONS] s ARCHIVES_DIR "WILDCARD STRING" [FILE])" << endl;
}
}  // namespace glt::glt
