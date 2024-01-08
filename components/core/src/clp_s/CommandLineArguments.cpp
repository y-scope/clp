#include "CommandLineArguments.hpp"

#include <iostream>

#include <boost/program_options.hpp>

#include "spdlog/spdlog.h"

namespace po = boost::program_options;

namespace clp_s {
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
    general_positional_options.add_options()("command", po::value<char>(&command_input))(
            "command-args",
            po::value<std::vector<std::string>>()
    );

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
                          << " c --help OR "
                          << " x --help OR "
                          << "q --help for command-specific details." << std::endl;

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
            compression_positional_options
                    .add_options()("archive-dir", po::value<std::string>(&m_archive_dir)->value_name("DIR"), "output directory")(
                            "input-paths",
                            po::value<std::vector<std::string>>(&m_file_paths)->value_name("PATHS"),
                            "input paths"
                    );

            po::options_description compression_options("Compression options");
            compression_options.add_options()
                    ("compression-level", po::value<int>(&m_compression_level)->
                        value_name("LEVEL")->default_value(3), "set compression level")
                    ("max-encoding-size", po::value<size_t>(&m_max_encoding_size)->
                        value_name("MAX_DICT_SIZE")->default_value(8UL * 1024 * 1024 * 1024), /*8GB*/
                        "maximum encoding size")
                    ("timestamp-key", po::value<std::string>(&m_timestamp_key)->
                        value_name("TIMESTAMP_COLUMN_KEY")->default_value(""), "timestamp column");

            po::positional_options_description positional_options;
            positional_options.add("archive-dir", 1);
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
                std::cerr << "  Compress file1.txt and dir1 into the archive dir" << std::endl;
                std::cerr << "   " << m_program_name << " c archive-dir file1.txt dir1"
                          << std::endl;
                std::cerr << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                visible_options.add(compression_options);
                std::cerr << visible_options << '\n';
                return ParsingResult::InfoCommand;
            }

            if (m_file_paths.empty()) {
                throw std::invalid_argument("No input paths specified.");
            }

            if (m_archive_dir.empty()) {
                throw std::invalid_argument("No archive directory specified.");
            }
        } else if ((char)Command::Extract == command_input) {
            po::options_description extraction_options;
            extraction_options.add_options()("archive-dir", po::value<std::string>(&m_archive_dir))(
                    "output-dir",
                    po::value<std::string>(&m_output_dir)
            );

            po::positional_options_description positional_options;
            positional_options.add("archive-dir", 1);
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
                std::cerr << "  Decompress all files from archive-dir into output-dir" << std::endl;
                std::cerr << "   " << m_program_name << "x archive-dir output-dir" << std::endl;
                std::cerr << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                std::cerr << visible_options << '\n';
                return ParsingResult::InfoCommand;
            }

            if (m_archive_dir.empty()) {
                throw std::invalid_argument("No archive directory specified");
            }

            if (m_output_dir.empty()) {
                throw std::invalid_argument("No output directory specified");
            }
        } else if ((char)Command::Search == command_input) {
            std::string archive_dir;
            std::string query;

            po::options_description search_options;
            search_options.add_options()("archive-dir", po::value<std::string>(&m_archive_dir))(
                    "query,q",
                    po::value<std::string>(&m_query)
            );

            po::positional_options_description positional_options;
            positional_options.add("archive-dir", 1);
            positional_options.add("query", 1);

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
                std::cerr << "  Perform a query on archive-dir" << std::endl;
                std::cerr << "   " << m_program_name << " s archive-dir query" << std::endl;
                std::cerr << std::endl;

                po::options_description visible_options;
                visible_options.add(general_options);
                std::cerr << visible_options << '\n';
                return ParsingResult::InfoCommand;
            }
            if (m_archive_dir.empty()) {
                throw std::invalid_argument("No archive directory specified");
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
    std::cerr << "Usage: " << m_program_name << "[OPTIONS] COMMAND [COMMAND ARGUMENTS]"
              << std::endl;
}

void CommandLineArguments::print_compression_usage() const {
    std::cerr << "Usage: " << m_program_name << " c [OPTIONS] ARCHIVE_DIR [FILE/DIR ...]"
              << std::endl;
}

void CommandLineArguments::print_decompression_usage() const {
    std::cerr << "Usage: " << m_program_name << " x [OPTIONS] ARCHIVE_DIR OUTPUT_DIR" << std::endl;
}

void CommandLineArguments::print_search_usage() const {
    std::cerr << "Usage: " << m_program_name << " s [OPTIONS] ARCHIVES_DIR QUERY" << std::endl;
}
}  // namespace clp_s
