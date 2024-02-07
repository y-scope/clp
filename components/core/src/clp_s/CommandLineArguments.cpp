#include "CommandLineArguments.hpp"

#include <iostream>

#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

#include "FileReader.hpp"

namespace po = boost::program_options;

namespace clp_s {
namespace {
/**
 * Read a list of newline-delimited paths from a file and put them into a vector passed by reference
 * TODO: deduplicate this code with the version in clp
 * @param input_path_list_file_path path to the file containing the list of paths
 * @param path_destination the vector that the paths are pushed into
 * @return true on success
 * @return false on error
 */
bool read_paths_from_file(
        std::string const& input_path_list_file_path,
        std::vector<std::string>& path_destination
) {
    FileReader reader;
    auto error_code = reader.try_open(input_path_list_file_path);
    if (ErrorCodeFileNotFound == error_code) {
        SPDLOG_ERROR(
                "Failed to open input path list file {} - file not found",
                input_path_list_file_path
        );
        return false;
    } else if (ErrorCodeSuccess != error_code) {
        SPDLOG_ERROR("Error opening input path list file {}", input_path_list_file_path);
        return false;
    }

    std::string line;
    while (true) {
        error_code = reader.try_read_to_delimiter('\n', false, false, line);
        if (ErrorCodeSuccess != error_code) {
            break;
        }
        if (false == line.empty()) {
            path_destination.push_back(line);
        }
    }

    if (ErrorCodeEndOfFile != error_code) {
        return false;
    }
    return true;
}
}  // namespace

CommandLineArguments::ParsingResult
CommandLineArguments::parse_arguments(int argc, char const** argv) {
    if (1 == argc) {
        print_basic_usage();
        return ParsingResult::Failure;
    }

    po::options_description general_options("General options");
    general_options.add_options()("help,h", "Print help");

    char command_input;
    po::options_description general_positional_options("General positional options");
    // clang-format off
    general_positional_options.add_options()(
            "command", po::value<char>(&command_input)
    )(
            "command-args", po::value<std::vector<std::string>>()
    );
    // clang-format on

    po::positional_options_description general_positional_options_description;
    general_positional_options_description.add("command", 1);
    general_positional_options_description.add("command-args", -1);

    po::options_description all_descriptions;
    all_descriptions.add(general_options);
    all_descriptions.add(general_positional_options);

    try {
        po::variables_map parsed_command_line_options;
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                            .options(all_descriptions)
                                            .positional(general_positional_options_description)
                                            .allow_unregistered()
                                            .run();
        po::store(parsed, parsed_command_line_options);
        po::notify(parsed_command_line_options);

        if (parsed_command_line_options.count("command") == 0) {
            if (parsed_command_line_options.count("help") != 0) {
                if (argc > 2) {
                    SPDLOG_WARN("Ignoring all options besides --help.");
                }

                print_basic_usage();
                std::cerr << "COMMAND is one of:" << std::endl;
                std::cerr << "  c - compress" << std::endl;
                std::cerr << "  x - decompress" << std::endl;
                std::cerr << "  s - search" << std::endl;
                std::cerr << std::endl;
                std::cerr << "Try "
                          << " c --help OR"
                          << " x --help OR"
                          << " s --help for command-specific details." << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                std::cerr << visible_options << '\n';
                return ParsingResult::InfoCommand;
            }

            throw std::invalid_argument("Command unspecified");
        }

        switch (command_input) {
            case (char)Command::Compress:
            case (char)Command::Extract:
            case (char)Command::Search:
                m_command = (Command)command_input;
                break;
            default:
                throw std::invalid_argument(std::string("Unknown action '") + command_input + "'");
        }

        if (Command::Compress == m_command) {
            po::options_description compression_positional_options;
            // clang-format off
             compression_positional_options.add_options()(
                     "archives-dir",
                     po::value<std::string>(&m_archives_dir)->value_name("DIR"),
                     "output directory"
             )(
                     "input-paths",
                     po::value<std::vector<std::string>>(&m_file_paths)->value_name("PATHS"),
                     "input paths"
             );
            // clang-format on

            po::options_description compression_options("Compression options");
            std::string metadata_db_config_file_path;
            std::string input_path_list_file_path;
            // clang-format off
            compression_options.add_options()(
                    "compression-level",
                    po::value<int>(&m_compression_level)->value_name("LEVEL")->
                        default_value(m_compression_level),
                    "1 (fast/low compression) to 9 (slow/high compression)."
            )(
                    "target-encoded-size",
                    po::value<size_t>(&m_target_encoded_size)->value_name("TARGET_ENCODED_SIZE")->
                        default_value(m_target_encoded_size),
                    "Target size (B) for the dictionaries and encoded messages before a new "
                    "archive is created."
            )(
                    "timestamp-key",
                    po::value<std::string>(&m_timestamp_key)->value_name("TIMESTAMP_COLUMN_KEY")->
                        default_value(m_timestamp_key),
                    "Path (e.g. x.y) for the field containing the log event's timestamp."
            )(
                    "db-config-file",
                    po::value<std::string>(&metadata_db_config_file_path)->value_name("FILE")->
                    default_value(metadata_db_config_file_path),
                    "Global metadata DB YAML config"
            )(
                    "files-from,f",
                    po::value<std::string>(&input_path_list_file_path)
                            ->value_name("FILE")
                            ->default_value(input_path_list_file_path),
                    "Compress files specified in FILE"
            )(
                    "print-archive-stats",
                    po::bool_switch(&m_print_archive_stats),
                    "Print statistics (json) about the archive after it's compressed."
            );
            // clang-format on

            po::positional_options_description positional_options;
            positional_options.add("archives-dir", 1);
            positional_options.add("input-paths", -1);

            po::options_description all_compression_options;
            all_compression_options.add(compression_options);
            all_compression_options.add(compression_positional_options);

            std::vector<std::string> unrecognized_options
                    = po::collect_unrecognized(parsed.options, po::include_positional);
            unrecognized_options.erase(unrecognized_options.begin());
            po::store(
                    po::command_line_parser(unrecognized_options)
                            .options(all_compression_options)
                            .positional(positional_options)
                            .run(),
                    parsed_command_line_options
            );
            po::notify(parsed_command_line_options);

            if (parsed_command_line_options.count("help")) {
                print_compression_usage();

                std::cerr << "Examples:" << std::endl;
                std::cerr << "  # Compress file1.json and dir1 into archives-dir" << std::endl;
                std::cerr << "  " << m_program_name << " c archives-dir file1.json dir1"
                          << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                visible_options.add(compression_options);
                std::cerr << visible_options << '\n';
                return ParsingResult::InfoCommand;
            }

            if (m_archives_dir.empty()) {
                throw std::invalid_argument("No archives directory specified.");
            }

            if (false == input_path_list_file_path.empty()) {
                if (false == read_paths_from_file(input_path_list_file_path, m_file_paths)) {
                    SPDLOG_ERROR("Failed to read paths from {}", input_path_list_file_path);
                    return ParsingResult::Failure;
                }
            }

            if (m_file_paths.empty()) {
                throw std::invalid_argument("No input paths specified.");
            }

            // Parse and validate global metadata DB config
            if (false == metadata_db_config_file_path.empty()) {
                clp::GlobalMetadataDBConfig metadata_db_config;
                try {
                    metadata_db_config.parse_config_file(metadata_db_config_file_path);
                } catch (std::exception& e) {
                    SPDLOG_ERROR("Failed to validate metadata database config - {}.", e.what());
                    return ParsingResult::Failure;
                }

                if (clp::GlobalMetadataDBConfig::MetadataDBType::MySQL
                    != metadata_db_config.get_metadata_db_type())
                {
                    SPDLOG_ERROR(
                            "Invalid metadata database type for {}; only supported type is MySQL.",
                            m_program_name
                    );
                    return ParsingResult::Failure;
                }

                m_metadata_db_config = std::move(metadata_db_config);
            }
        } else if ((char)Command::Extract == command_input) {
            po::options_description extraction_options;
            // clang-format off
            extraction_options.add_options()(
                    "archives-dir",
                    po::value<std::string>(&m_archives_dir),
                    "The directory containing the archives"
            )(
                    "output-dir",
                    po::value<std::string>(&m_output_dir),
                    "The output directory for the decompressed file"
            );
            // clang-format on

            po::positional_options_description positional_options;
            positional_options.add("archives-dir", 1);
            positional_options.add("output-dir", 1);

            std::vector<std::string> unrecognized_options
                    = po::collect_unrecognized(parsed.options, po::include_positional);
            unrecognized_options.erase(unrecognized_options.begin());
            po::store(
                    po::command_line_parser(unrecognized_options)
                            .options(extraction_options)
                            .positional(positional_options)
                            .run(),
                    parsed_command_line_options
            );

            po::notify(parsed_command_line_options);

            if (parsed_command_line_options.count("help")) {
                print_decompression_usage();

                std::cerr << "Examples:" << std::endl;
                std::cerr << "  # Decompress all files from archives-dir into output-dir"
                          << std::endl;
                std::cerr << "  " << m_program_name << " x archives-dir output-dir" << std::endl;
                std::cerr << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                std::cerr << visible_options << std::endl;
                return ParsingResult::InfoCommand;
            }

            if (m_archives_dir.empty()) {
                throw std::invalid_argument("No archives directory specified");
            }

            if (m_output_dir.empty()) {
                throw std::invalid_argument("No output directory specified");
            }
        } else if ((char)Command::Search == command_input) {
            std::string archives_dir;
            std::string query;

            po::options_description search_options;
            // clang-format off
            search_options.add_options()(
                    "archives-dir",
                    po::value<std::string>(&m_archives_dir),
                    "The directory containing the archives"
            )(
                    "query,q",
                    po::value<std::string>(&m_query),
                    "Query to perform"
            );
            // clang-format on

            po::options_description match_options("Match Controls");
            // clang-format off
            match_options.add_options()(
                "tge",
                po::value<epochtime_t>()->value_name("TS"),
                "Find records with UNIX epoch timestamp >= TS ms"
            )(
                "tle",
                po::value<epochtime_t>()->value_name("TS"),
                "Find records with UNIX epoch timestamp <= TS ms"
            );
            // clang-format on
            search_options.add(match_options);

            po::positional_options_description positional_options;
            positional_options.add("archives-dir", 1);
            positional_options.add("query", 1);

            po::options_description output_options("Output Options");
            // clang-format off
            output_options.add_options()(
                    "mongodb-uri",
                    po::value<std::string>(&m_mongodb_uri)->value_name("MONGODB_URI"),
                    "MongoDB URI to connect to"
            )(
                    "mongodb-collection",
                    po::value<std::string>(&m_mongodb_collection)->value_name("MONGODB_COLLECTION"),
                    "MongoDB collection to output to"
            )(
                    "batch-size",
                    po::value<uint64_t>(&m_batch_size)->value_name("BATCH_SIZE")->
                        default_value(m_batch_size),
                    "The number of documents to insert into MongoDB in a batch"
            )(
                    "max-num-results",
                    po::value<uint64_t>(&m_max_num_results)->value_name("MAX_NUM_RESULTS")->
                        default_value(m_max_num_results),
                    "The maximum number of results to output"
            );
            // clang-format on
            search_options.add(output_options);

            std::vector<std::string> unrecognized_options
                    = po::collect_unrecognized(parsed.options, po::include_positional);
            unrecognized_options.erase(unrecognized_options.begin());
            po::store(
                    po::command_line_parser(unrecognized_options)
                            .options(search_options)
                            .positional(positional_options)
                            .run(),
                    parsed_command_line_options
            );

            po::notify(parsed_command_line_options);

            if (parsed_command_line_options.count("help")) {
                print_search_usage();

                std::cerr << "Examples:" << std::endl;
                std::cerr << "  # Search archives-dir for logs matching a KQL query"
                             R"( "level: INFO" and output to stdout)"
                          << std::endl;
                std::cerr << "  " << m_program_name << R"( s archives-dir "level: INFO")"
                          << std::endl;
                std::cerr << std::endl;

                std::cerr << "  # Search archives-dir for logs matching a KQL query"
                             R"( "level: INFO" and output to MongoDB)"
                          << std::endl;
                std::cerr << "  " << m_program_name
                          << " s --mongodb-uri mongodb://127.0.0.1:27017/test --mongodb-collection"
                             R"( test archives-dir "level: INFO")"
                          << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                visible_options.add(match_options);
                visible_options.add(output_options);
                std::cerr << visible_options << '\n';
                return ParsingResult::InfoCommand;
            }

            if (parsed_command_line_options.count("tge")) {
                m_search_begin_ts = parsed_command_line_options["tge"].as<epochtime_t>();
            }

            if (parsed_command_line_options.count("tle")) {
                m_search_end_ts = parsed_command_line_options["tle"].as<epochtime_t>();
            }

            if (m_search_begin_ts.has_value() && m_search_end_ts.has_value()
                && m_search_begin_ts.value() > m_search_end_ts.value())
            {
                throw std::invalid_argument(
                        "Timestamp range is invalid - begin timestamp is after end timestamp."
                );
            }

            if (false == m_mongodb_uri.empty() || false == m_mongodb_collection.empty()) {
                if (m_mongodb_uri.empty() || m_mongodb_collection.empty()) {
                    throw std::invalid_argument(
                            "MongoDB uri and collection must be specified together"
                    );
                }
                if (0 == m_batch_size) {
                    throw std::invalid_argument("Batch size must be greater than 0");
                }
                if (0 == m_max_num_results) {
                    throw std::invalid_argument("Max number of results must be greater than 0");
                }

                m_mongodb_enabled = true;
            }

            if (m_archives_dir.empty()) {
                throw std::invalid_argument("No archives directory specified");
            }

            if (m_query.empty()) {
                throw std::invalid_argument("No query specified");
            }
        }

    } catch (std::exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        std::cerr << "Try " << get_program_name() << " --help for detailed usage instructions"
                  << std::endl;
        return ParsingResult::Failure;
    }

    return ParsingResult::Success;
}

void CommandLineArguments::print_basic_usage() const {
    std::cerr << "Usage: " << m_program_name << " [OPTIONS] COMMAND [COMMAND ARGUMENTS]"
              << std::endl;
}

void CommandLineArguments::print_compression_usage() const {
    std::cerr << "Usage: " << m_program_name << " c [OPTIONS] ARCHIVES_DIR [FILE/DIR ...]"
              << std::endl;
}

void CommandLineArguments::print_decompression_usage() const {
    std::cerr << "Usage: " << m_program_name << " x [OPTIONS] ARCHIVES_DIR OUTPUT_DIR" << std::endl;
}

void CommandLineArguments::print_search_usage() const {
    std::cerr << "Usage: " << m_program_name << " s [OPTIONS] ARCHIVES_DIR KQL_QUERY" << std::endl;
}
}  // namespace clp_s
