// C++ libraries
#include <unordered_set>

// Boost libraries
#include <boost/filesystem.hpp>

// spdlog
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

// Project headers
#include "clp_main.hpp"
#include "../Profiler.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"
#include "compression.hpp"
#include "decompression.hpp"
#include "utils.hpp"

using clp::CommandLineArguments;
using std::string;
using std::unordered_set;
using std::vector;

int clp_main (int argc, const char* argv[], std::string* archive_path) {
    // Program-wide initialization
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S,%e [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }
    PROFILER_INITIALIZE()
    TimestampPattern::init();

    clp::CommandLineArguments command_line_args("clp");
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

    // Read input paths from file if necessary
    if (false == command_line_args.get_path_list_path().empty()) {
        if (false == clp::read_input_paths(command_line_args.get_path_list_path(), input_paths)) {
            return -1;
        }
    }

    if (CommandLineArguments::Command::Compress == command_line_args.get_command()) {
        std::unique_ptr<LogParser> log_parser;
        if(!command_line_args.get_use_heuristic()) {
            {
                std::string const &schema_file_path = command_line_args.get_schema_file_path();
                if (true == schema_file_path.empty()) { // currently, impossible (as get_use_heuristic==false iff true == schema_file_path.empty())
                    // (1) have a default schema path and require that to be valid
                    // (2) hardcode a schema to use if no schema file is provided
                    // 1 probably makes more sense as a default log schema is unlikely to be useful anyway
                    SPDLOG_ERROR("No schema file provided. TODO: decide on default behaviour...");
                    return false;
                }

                FileReader schema_reader;
                ErrorCode error_code = schema_reader.try_open(schema_file_path);
                if (ErrorCode_Success != error_code) {
                    if (ErrorCode_FileNotFound == error_code) {
                        SPDLOG_ERROR("'{}' does not exist.", schema_file_path.c_str());
                    } else if (ErrorCode_errno == error_code) {
                        SPDLOG_ERROR("Failed to read '{}', errno={}", schema_file_path.c_str(), errno);
                    } else {
                        SPDLOG_ERROR("Failed to read '{}', error_code={}", schema_file_path.c_str(), error_code);
                    }
                    return false;
                }
                log_parser = std::make_unique<LogParser>(LogParser());
                log_parser->schema_checksum = schema_reader.compute_checksum(log_parser->schema_file_size);
                schema_reader.try_open(schema_file_path);
                SchemaParser sp;
                std::unique_ptr<ParserASTSchemaFile> schema_ast = sp.generate_schema_ast(&schema_reader);
                schema_ast->file_path = boost::filesystem::canonical(schema_reader.get_path()).string();
                log_parser->init(std::move(schema_ast));
                schema_reader.close();
            }
        }
        
        boost::filesystem::path path_prefix_to_remove(command_line_args.get_path_prefix_to_remove());

        // Validate input paths exist
        if (false == clp::validate_paths_exist(input_paths)) {
            return -1;
        }

        // Get paths of all files we need to compress
        vector<clp::FileToCompress> files_to_compress;
        vector<string> empty_directory_paths;
        for (const auto& input_path : input_paths) {
            if (false == find_all_files_and_empty_directories(path_prefix_to_remove, input_path, files_to_compress, empty_directory_paths)) {
                return -1;
            }
        }

        vector<clp::FileToCompress> grouped_files_to_compress;

        if (files_to_compress.empty() && empty_directory_paths.empty() && grouped_files_to_compress.empty()) {
            SPDLOG_ERROR("No files/directories to compress.");
            return -1;
        }
        
        bool compression_successful;
        try {
            compression_successful = compress(command_line_args, files_to_compress, empty_directory_paths, grouped_files_to_compress,
                                              command_line_args.get_target_encoded_file_size(), archive_path, std::move(log_parser), 
                                              command_line_args.get_use_heuristic());
        } catch (TraceableException& e) {
            ErrorCode error_code = e.get_error_code();
            if (ErrorCode_errno == error_code) {
                SPDLOG_ERROR("Compression failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
                compression_successful = false;
            } else {
                SPDLOG_ERROR("Compression failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
                compression_successful = false;
            }
        } catch (std::exception& e) {
            SPDLOG_ERROR("Compression failed: Unexpected exception - {}", e.what());
            compression_successful = false;
        }
        if (!compression_successful) {
            return -1;
        }
    } else { // CommandLineArguments::Command::Extract == command
        unordered_set<string> files_to_decompress(input_paths.cbegin(), input_paths.cend());
        if (!decompress(command_line_args, files_to_decompress)) {
            return -1;
        }
    }

    return 0;
}