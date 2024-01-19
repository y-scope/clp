#include "run.hpp"

#include <unordered_set>

#include <spdlog/sinks/stdout_sinks.h>

#include "../Profiler.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"
#include "compression.hpp"
#include "decompression.hpp"
#include "search.hpp"
#include "utils.hpp"

using std::string;
using std::unordered_set;
using std::vector;

namespace glt::glt {

static bool
obtain_input_paths(CommandLineArguments const& command_line_args, vector<string>& input_paths) {
    input_paths = command_line_args.get_input_paths();
    // Read input paths from file if necessary
    if (false == command_line_args.get_path_list_path().empty()) {
        if (false == read_input_paths(command_line_args.get_path_list_path(), input_paths)) {
            return false;
        }
    }
    return true;
}

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

    CommandLineArguments command_line_args("glt");
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

    Profiler::start_continuous_measurement<Profiler::ContinuousMeasurementIndex::Execution>();

    if (CommandLineArguments::Command::Compress == command_line_args.get_command()) {
        vector<string> input_paths;
        if (false == obtain_input_paths(command_line_args, input_paths)) {
            return -1;
        }
        boost::filesystem::path path_prefix_to_remove(command_line_args.get_path_prefix_to_remove()
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
                    command_line_args.get_target_encoded_file_size()
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
    } else if (CommandLineArguments::Command::Extract == command_line_args.get_command()) {
        vector<string> input_paths;
        if (false == obtain_input_paths(command_line_args, input_paths)) {
            return -1;
        }
        unordered_set<string> files_to_decompress(input_paths.cbegin(), input_paths.cend());
        if (!decompress(command_line_args, files_to_decompress)) {
            return -1;
        }
    } else {  // CommandLineArguments::Command::Search == command
        if (!search(command_line_args)) {
            return -1;
        }
    }

    Profiler::stop_continuous_measurement<Profiler::ContinuousMeasurementIndex::Execution>();
    LOG_CONTINUOUS_MEASUREMENT(Profiler::ContinuousMeasurementIndex::Execution)

    return 0;
}
}  // namespace glt::glt
