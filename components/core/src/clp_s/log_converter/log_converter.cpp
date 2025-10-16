#include <iostream>

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "CommandLineArguments.hpp"

using clp_s::log_converter::CommandLineArguments;

int main(int argc, char const* argv[]) {
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return 1;
    }

    clp_s::log_converter::CommandLineArguments command_line_arguments{"log-converter"};

    auto const parsing_result{command_line_arguments.parse_arguments(argc, argv)};
    switch (parsing_result) {
        case CommandLineArguments::ParsingResult::Success:
            break;
        case CommandLineArguments::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArguments::ParsingResult::Failure:
        default:
            return 1;
    }

    return 0;
}
