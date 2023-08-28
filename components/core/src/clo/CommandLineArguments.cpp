#include "CommandLineArguments.hpp"

// C++ standard libraries
#include <fstream>
#include <iostream>

// Boost libraries
#include <boost/program_options.hpp>

// Project headers
#include "../spdlog_with_specializations.hpp"
#include "../version.hpp"

namespace po = boost::program_options;
using std::cerr;
using std::endl;
using std::exception;
using std::invalid_argument;
using std::string;
using std::vector;

namespace clo {
    CommandLineArgumentsBase::ParsingResult CommandLineArguments::parse_arguments (int argc, const char* argv[]) {
        // Print out basic usage if user doesn't specify any options
        if (1 == argc) {
            print_basic_usage();
            return ParsingResult::Failure;
        }

        // Define general options
        po::options_description options_general("General Options");
        // Set default configuration file path to "$HOME/cDefaultConfigFilename" (Linux environment) if $HOME is set, or "./cDefaultConfigFilename" otherwise
        string config_file_path;
        const char* home_environment_var_value = getenv("HOME");
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
                ("config-file", po::value<string>(&config_file_path)->value_name("FILE")->default_value(config_file_path),
                 "Use configuration options from FILE")
                ;

        // Define match controls
        po::options_description options_match_control("Match Controls");
        options_match_control.add_options()
                ("tgt", po::value<epochtime_t>()->value_name("TS"), "Find messages with UNIX timestamp >  TS ms")
                ("tge", po::value<epochtime_t>()->value_name("TS"), "Find messages with UNIX timestamp >= TS ms")
                ("teq", po::value<epochtime_t>()->value_name("TS"), "Find messages with UNIX timestamp == TS ms")
                ("tlt", po::value<epochtime_t>()->value_name("TS"), "Find messages with UNIX timestamp <  TS ms")
                ("tle", po::value<epochtime_t>()->value_name("TS"), "Find messages with UNIX timestamp <= TS ms")
                ("ignore-case,i", po::bool_switch(&m_ignore_case), "Ignore case distinctions in both WILDCARD STRING and the input files")
                ;

        // Define visible options
        po::options_description visible_options;
        visible_options.add(options_general);
        visible_options.add(options_match_control);

        // Define hidden positional options (not shown in Boost's program options help message)
        po::options_description hidden_positional_options;
        hidden_positional_options.add_options()
                ("search-controller-host", po::value<string>(&m_search_controller_host))
                ("search-controller-port", po::value<string>(&m_search_controller_port))
                ("archive-path", po::value<string>(&m_archive_path))
                ("wildcard-string", po::value<string>(&m_search_string))
                ("file-path", po::value<string>(&m_file_path))
                ;
        po::positional_options_description positional_options_description;
        positional_options_description.add("search-controller-host", 1);
        positional_options_description.add("search-controller-port", 1);
        positional_options_description.add("archive-path", 1);
        positional_options_description.add("wildcard-string", 1);
        positional_options_description.add("file-path", 1);

        // Aggregate all options
        po::options_description all_options;
        all_options.add(options_general);
        all_options.add(options_match_control);
        all_options.add(hidden_positional_options);

        // Parse options
        try {
            // Parse options specified on the command line
            po::parsed_options parsed = po::command_line_parser(argc, argv).options(all_options).positional(positional_options_description).run();
            po::variables_map parsed_command_line_options;
            store(parsed, parsed_command_line_options);

            // Handle config-file manually since Boost won't set it until we call notify, and we can't call notify until we parse the config file
            if (parsed_command_line_options.count("config-file")) {
                config_file_path = parsed_command_line_options["config-file"].as<string>();
            }

            // Parse options specified through the config file
            // NOTE: Command line arguments will take priority over config file since they are parsed first and Boost doesn't replace existing options
            std::ifstream config_file(config_file_path);
            if (config_file.is_open()) {
                // Allow unrecognized options in configuration file since some of them may be exclusively for clp or other applications
                po::parsed_options parsed_config_file = po::parse_config_file(config_file, all_options, true);
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
                cerr << endl;

                cerr << "Examples:" << endl;
                cerr << R"(  # Search ARCHIVE_PATH for " ERROR " and send results to the controller at localhost:5555)"
                     << endl;
                cerr << "  " << get_program_name() << R"( localhost 5555 ARCHIVE_PATH " ERROR ")" << endl;
                cerr << endl;

                cerr << "Options can be specified on the command line or through a configuration file." << endl;
                cerr << visible_options << endl;
                return ParsingResult::InfoCommand;
            }

            // Handle --version
            if (parsed_command_line_options.count("version")) {
                cerr << cVersion << endl;
                return ParsingResult::InfoCommand;
            }

            // Validate search controller host was specified
            if (m_search_controller_host.empty()) {
                throw invalid_argument("SEARCH_CONTROLLER_HOST not specified or empty.");
            }

            // Validate search controller port was specified
            if (m_search_controller_port.empty()) {
                throw invalid_argument("SEARCH_CONTROLLER_PORT not specified or empty.");
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
                if (parsed_command_line_options.count("tgt") + parsed_command_line_options.count("tge") + parsed_command_line_options.count("tlt") +
                    parsed_command_line_options.count("tle") > 0)
                {
                    throw invalid_argument("--teq cannot be specified with any other timestamp filtering option.");
                }

                m_search_begin_ts = parsed_command_line_options["teq"].as<epochtime_t>() ;
                m_search_end_ts = parsed_command_line_options["teq"].as<epochtime_t>();
            } else {
                if (parsed_command_line_options.count("tgt") + parsed_command_line_options.count("tge") > 1){
                    throw invalid_argument("--tgt cannot be used with --tge.");
                }

                // Set m_search_begin_ts
                if (parsed_command_line_options.count("tgt")) {
                    m_search_begin_ts = parsed_command_line_options["tgt"].as<epochtime_t>() + 1;
                } else if (parsed_command_line_options.count("tge")) {
                    m_search_begin_ts = parsed_command_line_options["tge"].as<epochtime_t>();
                }

                if (parsed_command_line_options.count("tlt") + parsed_command_line_options.count("tle") > 1) {
                    throw invalid_argument("--tlt cannot be used with --tle.");
                }

                // Set m_search_end_ts
                if (parsed_command_line_options.count("tlt")) {
                    m_search_end_ts = parsed_command_line_options["tlt"].as<epochtime_t>() - 1;
                } else if (parsed_command_line_options.count("tle")) {
                    m_search_end_ts = parsed_command_line_options["tle"].as<epochtime_t>();
                }

                if (m_search_begin_ts > m_search_end_ts) {
                    throw invalid_argument("Timestamp range is invalid - begin timestamp is after end timestamp.");
                }
            }
        } catch (exception &e) {
            SPDLOG_ERROR("{}", e.what());
            print_basic_usage();
            cerr << "Try " << get_program_name() << " --help for detailed usage instructions" << endl;
            return ParsingResult::Failure;
        }

        return ParsingResult::Success;
    }

    void CommandLineArguments::print_basic_usage () const {
        cerr << "Usage: " << get_program_name() << " [OPTIONS] SEARCH_CONTROLLER_HOST SEARCH_CONTROLLER_PORT "
             << R"(ARCHIVE_PATH "WILDCARD STRING" [FILE])" << endl;
    }
}
