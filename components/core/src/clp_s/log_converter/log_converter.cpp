#include <cstdint>
#include <exception>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>

#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "../../clp/ReaderInterface.hpp"
#include "../InputConfig.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"
#include "LogConverter.hpp"

using clp_s::log_converter::CommandLineArguments;
using clp_s::log_converter::LogConverter;

namespace {
/**
 * Converts all files according to the command line arguments.
 * @param command_line_arguments
 * @return Whether conversion was successful.
 */
[[nodiscard]] auto convert_files(CommandLineArguments const& command_line_arguments) -> bool;

auto convert_files(CommandLineArguments const& command_line_arguments) -> bool {
    LogConverter log_converter;

    std::error_code ec{};
    if (false == std::filesystem::create_directory(command_line_arguments.get_output_dir(), ec)
        && ec)
    {
        SPDLOG_ERROR(
                "Can not create output directory {} - {}",
                command_line_arguments.get_output_dir(),
                ec.message()
        );
        return false;
    }

    for (auto const& path : command_line_arguments.get_input_paths()) {
        auto reader{clp_s::try_create_reader(path, command_line_arguments.get_network_auth())};
        if (nullptr == reader) {
            SPDLOG_ERROR("Failed to open input {} for reading.", path.path);
            return false;
        }

        auto [nested_readers, file_type] = clp_s::try_deduce_reader_type(reader);
        switch (file_type) {
            case clp_s::FileType::LogText:
                break;
            case clp_s::FileType::Json:
            case clp_s::FileType::KeyValueIr:
            case clp_s::FileType::Zstd:
            case clp_s::FileType::Unknown:
            default: {
                std::ignore
                        = clp_s::NetworkUtils::check_and_log_curl_error(path.path, reader.get());
                SPDLOG_ERROR("Received input that was not unstructured logtext: {}.", path.path);
                return false;
            }
        }

        auto const convert_result{log_converter.convert_file(
                path,
                nested_readers.back().get(),
                command_line_arguments.get_output_dir()
        )};
        if (convert_result.has_error()) {
            auto const& error{convert_result.error()};
            SPDLOG_ERROR(
                    "Failed to convert input {} to structured representation: {} - {}",
                    path.path,
                    error.category().name(),
                    error.message()
            );
            return false;
        }
    }

    return true;
}
}  // namespace

auto main(int argc, char const** argv) -> int {
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%dT%H:%M:%S.%e%z [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return 1;
    }

    CommandLineArguments command_line_arguments{"log-converter"};

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

    if (false == convert_files(command_line_arguments)) {
        return 1;
    }
    return 0;
}
