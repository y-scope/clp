#include <exception>
#include <filesystem>

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "CommandLineArguments.hpp"
#include "IndexManager.hpp"

using clp_s::indexer::CommandLineArguments;

int main(int argc, char const* argv[]) {
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        return 1;
    }

    CommandLineArguments command_line_arguments("index");
    auto parsing_result = command_line_arguments.parse_arguments(argc, argv);
    switch (parsing_result) {
        case CommandLineArguments::ParsingResult::Failure:
            return 1;
        case CommandLineArguments::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArguments::ParsingResult::Success:
            // Continue processing
            break;
    }

    try {
        clp_s::indexer::IndexManager index_manager(command_line_arguments.get_db_config());
        index_manager.update_metadata(
                command_line_arguments.get_dataset_name(),
                command_line_arguments.get_archive_path()
        );
    } catch (std::exception& e) {
        SPDLOG_ERROR("Failed to update metadata: {}", e.what());
        return 1;
    }
    return 0;
}
