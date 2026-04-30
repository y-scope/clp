#include "run.hpp"

#include <fstream>
#include <string>
#include <unordered_set>

#include <log_surgeon/generated_bindings.hpp>
#include <log_surgeon/log_surgeon.hpp>
#include <log_surgeon/rust_compat.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../Profiler.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"
#include "compression.hpp"
#include "decompression.hpp"
#include "utils.hpp"

using std::string;
using std::unordered_set;
using std::vector;

namespace clp::clp {
int run(int argc, char const* argv[]) {
    // Program-wide initialization
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S,%e [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }
    Profiler::init();
    TimestampPattern::init();

    CommandLineArguments command_line_args("clp");
    auto parsing_result = command_line_args.parse_arguments(argc, argv);
    switch (parsing_result) {
        case CommandLineArgumentsBase::ParsingResult::Failure:
            return -1;
        case CommandLineArgumentsBase::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArgumentsBase::ParsingResult::Success:
            // Continue processing
            break;
    }

    vector<string> input_paths = command_line_args.get_input_paths();

    Profiler::start_continuous_measurement<Profiler::ContinuousMeasurementIndex::Compression>();

    // Read input paths from file if necessary
    if (false == command_line_args.get_path_list_path().empty()) {
        if (false == read_input_paths(command_line_args.get_path_list_path(), input_paths)) {
            return -1;
        }
    }

    auto command = command_line_args.get_command();
    if (CommandLineArguments::Command::Compress == command) {
        log_surgeon::Schema* schema{nullptr};
        if (!command_line_args.get_use_heuristic()) {
            std::string const& schema_file_path = command_line_args.get_schema_file_path();
            std::ifstream schema_file{schema_file_path};
            if (false == schema_file.good()) {
                SPDLOG_ERROR("Schema file at {} failed to open.", schema_file_path);
                return -1;
            }
            std::string schema_text{
                    (std::istreambuf_iterator<char>(schema_file)),
                    std::istreambuf_iterator<char>()
            };
            log_surgeon::CCharArray schema_arr{schema_text.data(), schema_text.size()};
            schema = log_surgeon::log_surgeon_schema_from_definition(schema_arr);
        }
        log_surgeon::ParserHandle parser{schema};

        boost::filesystem::path path_prefix_to_remove(
                command_line_args.get_path_prefix_to_remove()
        );

        // Validate input paths exist
        if (false == validate_paths_exist(input_paths)) {
            return -1;
        }

        // Get paths of all files we need to compress
        vector<FileToCompress> files_to_compress;
        vector<string> empty_directory_paths;
        for (auto const& input_path : input_paths) {
            if (false
                == find_all_files_and_empty_directories(
                        path_prefix_to_remove,
                        input_path,
                        files_to_compress,
                        empty_directory_paths
                ))
            {
                return -1;
            }
        }

        vector<FileToCompress> grouped_files_to_compress;

        if (files_to_compress.empty() && empty_directory_paths.empty()
            && grouped_files_to_compress.empty())
        {
            SPDLOG_ERROR("No files/directories to compress.");
            return -1;
        }

        bool compression_successful;
        try {
            compression_successful = compress(
                    command_line_args,
                    files_to_compress,
                    empty_directory_paths,
                    grouped_files_to_compress,
                    command_line_args.get_target_encoded_file_size(),
                    parser,
                    command_line_args.get_use_heuristic()
            );
        } catch (TraceableException& e) {
            ErrorCode error_code = e.get_error_code();
            if (ErrorCode_errno == error_code) {
                SPDLOG_ERROR(
                        "Compression failed: {}:{} {}, errno={}",
                        e.get_filename(),
                        e.get_line_number(),
                        e.what(),
                        errno
                );
                compression_successful = false;
            } else {
                SPDLOG_ERROR(
                        "Compression failed: {}:{} {}, error_code={}",
                        e.get_filename(),
                        e.get_line_number(),
                        e.what(),
                        error_code
                );
                compression_successful = false;
            }
        } catch (std::exception& e) {
            SPDLOG_ERROR("Compression failed: Unexpected exception - {}", e.what());
            compression_successful = false;
        }
        if (!compression_successful) {
            return -1;
        }
    } else if (CommandLineArguments::Command::Extract == command) {
        unordered_set<string> files_to_decompress(input_paths.cbegin(), input_paths.cend());
        if (false == decompress(command_line_args, files_to_decompress)) {
            return -1;
        }
    } else if (CommandLineArguments::Command::ExtractIr == command) {
        if (false == decompress_to_ir(command_line_args)) {
            return -1;
        }
    } else {
        SPDLOG_ERROR("Command {} not implemented.", enum_to_underlying_type(command));
        return -1;
    }

    Profiler::stop_continuous_measurement<Profiler::ContinuousMeasurementIndex::Compression>();
    LOG_CONTINUOUS_MEASUREMENT(Profiler::ContinuousMeasurementIndex::Compression)

    return 0;
}
}  // namespace clp::clp
