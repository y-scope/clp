#include "CommandLineArguments.hpp"

#include <iostream>

#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>

namespace po = boost::program_options;

namespace clp_s::indexer {
CommandLineArguments::ParsingResult
CommandLineArguments::parse_arguments(int argc, char const** argv) {
    // Print out basic usage if user doesn't specify any options
    if (1 == argc) {
        print_basic_usage();
        return ParsingResult::Failure;
    }

    // Define general options
    po::options_description general_options("General Options");
    general_options.add_options()("help,h", "Print help");

    // Define output options
    po::options_description output_options("Output Options");
    // clang-format off
    output_options.add_options()(
            "create-table",
            po::bool_switch(&m_should_create_table),
            "Create the column metadata table if it doesn't exist"
    );
    // clang-format on
    clp::GlobalMetadataDBConfig metadata_db_config{output_options};

    // Define visible options
    po::options_description visible_options;
    visible_options.add(general_options);
    visible_options.add(output_options);

    std::string archive_path;
    po::options_description positional_options;
    // clang-format off
    positional_options.add_options()(
            "dataset-name",
            po::value<std::string>(&m_dataset_name),
            "Name of the dataset for which the column metadata table should be populated"
    )(
            "archive-path",
            po::value<std::string>(&archive_path),
            "Path to an archive"
    );
    // clang-format on
    po::positional_options_description positional_options_description;
    positional_options_description.add("dataset-name", 1);
    positional_options_description.add("archive-path", 1);

    // Aggregate all options
    po::options_description all_options;
    all_options.add(general_options);
    all_options.add(output_options);
    all_options.add(positional_options);

    // Parse options
    try {
        // Parse options specified on the command line
        po::parsed_options parsed = po::command_line_parser(argc, argv)
                                            .options(all_options)
                                            .positional(positional_options_description)
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

            std::cerr << visible_options << std::endl;
            return ParsingResult::InfoCommand;
        }

        // Validate required parameters
        if (m_dataset_name.empty()) {
            throw std::invalid_argument("Dataset name not specified or empty.");
        }
        if (archive_path.empty()) {
            throw std::invalid_argument("Archive path not specified or empty.");
        }
        m_archive_path = get_path_object_for_raw_path(archive_path);

        // Initialize and validate global metadata DB config
        if (clp::GlobalMetadataDBConfig::MetadataDBType::MySQL
            != metadata_db_config.get_metadata_db_type())
        {
            SPDLOG_ERROR(
                    "Invalid metadata database type for {}; only supported type is MySQL.",
                    m_program_name
            );
            return ParsingResult::Failure;
        }
        metadata_db_config.read_credentials_from_env_if_needed();
        metadata_db_config.validate();
        m_metadata_db_config = std::move(metadata_db_config);
    } catch (std::exception& e) {
        SPDLOG_ERROR("{}", e.what());
        print_basic_usage();
        return ParsingResult::Failure;
    }

    return ParsingResult::Success;
}

void CommandLineArguments::print_basic_usage() const {
    std::cerr << "Usage: " << get_program_name() << " [OPTIONS] DATASET_NAME ARCHIVE_PATH"
              << std::endl;
}
}  // namespace clp_s::indexer
