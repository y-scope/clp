#include <iostream>
#include <memory>

#include <boost/filesystem.hpp>
#include <mongocxx/instance.hpp>
#include <spdlog/sinks/stdout_sinks.h>

#include "../Defs.h"
#include "../Grep.hpp"
#include "../Profiler.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../streaming_archive/Constants.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"
#include "ResultsCacheClient.hpp"

using clp::clo::CommandLineArguments;
using clp::clo::ResultsCacheClient;
using clp::CommandLineArgumentsBase;
using clp::epochtime_t;
using clp::ErrorCode;
using clp::ErrorCode_errno;
using clp::ErrorCode_Success;
using clp::Grep;
using clp::load_lexer_from_file;
using clp::Query;
using clp::streaming_archive::MetadataDB;
using clp::streaming_archive::reader::Archive;
using clp::streaming_archive::reader::File;
using clp::streaming_archive::reader::Message;
using clp::TraceableException;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;

// Local types
enum class SearchFilesResult {
    OpenFailure,
    ResultSendFailure,
    Success
};

/**
 * Searches all files referenced by a given database cursor
 * @param query
 * @param archive
 * @param file_metadata_ix
 * @param results_cache_client
 * @param segments_to_search
 * @return SearchFilesResult::OpenFailure on failure to open a compressed file
 * @return SearchFilesResult::ResultSendFailure on failure to send a result
 * @return SearchFilesResult::Success otherwise
 */
static SearchFilesResult search_files(
        Query& query,
        Archive& archive,
        MetadataDB::FileIterator& file_metadata_ix,
        ResultsCacheClient& results_cache_client,
        std::set<clp::segment_id_t> const& segments_to_search
);
/**
 * Searches an archive with the given path
 * @param command_line_args
 * @param archive_path
 * @param results_cache_client
 * @return true on success, false otherwise
 */
static bool search_archive(
        CommandLineArguments const& command_line_args,
        boost::filesystem::path const& archive_path,
        ResultsCacheClient& results_cache_client
);

static SearchFilesResult search_files(
        Query& query,
        Archive& archive,
        MetadataDB::FileIterator& file_metadata_ix,
        ResultsCacheClient& results_cache_client,
        std::set<clp::segment_id_t> const& segments_to_search
) {
    SearchFilesResult result = SearchFilesResult::Success;

    File compressed_file;
    Message compressed_message;
    string decompressed_message;

    // Run query on each file
    for (; file_metadata_ix.has_next(); file_metadata_ix.next()) {
        if (segments_to_search.count(file_metadata_ix.get_segment_id()) == 0) {
            continue;
        }
        if (results_cache_client.is_latest_results_full()
            && results_cache_client.get_smallest_timestamp() > file_metadata_ix.get_end_ts())
        {
            continue;
        }

        ErrorCode error_code = archive.open_file(compressed_file, file_metadata_ix);
        if (ErrorCode_Success != error_code) {
            string orig_path;
            file_metadata_ix.get_path(orig_path);
            if (ErrorCode_errno == error_code) {
                SPDLOG_ERROR("Failed to open {}, errno={}", orig_path.c_str(), errno);
            } else {
                SPDLOG_ERROR("Failed to open {}, error={}", orig_path.c_str(), error_code);
            }
            result = SearchFilesResult::OpenFailure;
            continue;
        }

        query.make_sub_queries_relevant_to_segment(compressed_file.get_segment_id());
        while (Grep::search_and_decompress(
                query,
                archive,
                compressed_file,
                compressed_message,
                decompressed_message
        ))
        {
            results_cache_client.add_result(
                    compressed_file.get_orig_path(),
                    decompressed_message,
                    compressed_message.get_ts_in_milli()
            );
        }

        archive.close_file(compressed_file);
    }

    return result;
}

static bool search_archive(
        CommandLineArguments const& command_line_args,
        boost::filesystem::path const& archive_path,
        ResultsCacheClient& results_cache_client
) {
    if (false == boost::filesystem::exists(archive_path)) {
        SPDLOG_ERROR("Archive '{}' does not exist.", archive_path.c_str());
        return false;
    }
    auto archive_metadata_file = archive_path / clp::streaming_archive::cMetadataFileName;
    if (false == boost::filesystem::exists(archive_metadata_file)) {
        SPDLOG_ERROR(
                "Archive metadata file '{}' does not exist. '{}' may not be an archive.",
                archive_metadata_file.c_str(),
                archive_path.c_str()
        );
        return false;
    }

    // Load lexers from schema file if it exists
    auto schema_file_path = archive_path / clp::streaming_archive::cSchemaFileName;
    unique_ptr<log_surgeon::lexers::ByteLexer> forward_lexer, reverse_lexer;
    bool use_heuristic = true;
    if (boost::filesystem::exists(schema_file_path)) {
        use_heuristic = false;
        // Create forward lexer
        forward_lexer.reset(new log_surgeon::lexers::ByteLexer());
        load_lexer_from_file(schema_file_path.string(), false, *forward_lexer);

        // Create reverse lexer
        reverse_lexer.reset(new log_surgeon::lexers::ByteLexer());
        load_lexer_from_file(schema_file_path.string(), true, *reverse_lexer);
    }

    Archive archive_reader;
    archive_reader.open(archive_path.string());
    archive_reader.refresh_dictionaries();

    auto search_begin_ts = command_line_args.get_search_begin_ts();
    auto search_end_ts = command_line_args.get_search_end_ts();

    auto query_processing_result = Grep::process_raw_query(
            archive_reader,
            command_line_args.get_search_string(),
            search_begin_ts,
            search_end_ts,
            command_line_args.ignore_case(),
            *forward_lexer,
            *reverse_lexer,
            use_heuristic
    );
    if (false == query_processing_result.has_value()) {
        return true;
    }

    auto& query = query_processing_result.value();
    // Get all segments potentially containing query results
    std::set<clp::segment_id_t> ids_of_segments_to_search;
    for (auto& sub_query : query.get_sub_queries()) {
        auto& ids_of_matching_segments = sub_query.get_ids_of_matching_segments();
        ids_of_segments_to_search.insert(
                ids_of_matching_segments.cbegin(),
                ids_of_matching_segments.cend()
        );
    }

    auto file_metadata_ix_ptr = archive_reader.get_file_iterator(
            search_begin_ts,
            search_end_ts,
            command_line_args.get_file_path(),
            true
    );
    auto& file_metadata_ix = *file_metadata_ix_ptr;
    search_files(
            query,
            archive_reader,
            file_metadata_ix,
            results_cache_client,
            ids_of_segments_to_search
    );
    file_metadata_ix_ptr.reset(nullptr);

    results_cache_client.flush();
    archive_reader.close();

    return true;
}

int main(int argc, char const* argv[]) {
    // Program-wide initialization
    try {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S,%e [%l] %v");
    } catch (std::exception& e) {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }
    clp::Profiler::init();
    clp::TimestampPattern::init();

    CommandLineArguments command_line_args("clo");
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

    mongocxx::instance mongocxx_instance{};
    ResultsCacheClient results_cache_client(
            command_line_args.get_mongodb_uri(),
            command_line_args.get_mongodb_collection(),
            command_line_args.get_batch_size(),
            command_line_args.get_max_num_results()
    );

    auto const archive_path = boost::filesystem::path(command_line_args.get_archive_path());

    int return_value = 0;
    try {
        if (false == search_archive(command_line_args, archive_path, results_cache_client)) {
            return_value = -1;
        }
    } catch (TraceableException& e) {
        auto error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR(
                    "Search failed: {}:{} {}, errno={}",
                    e.get_filename(),
                    e.get_line_number(),
                    e.what(),
                    errno
            );
        } else {
            SPDLOG_ERROR(
                    "Search failed: {}:{} {}, error_code={}",
                    e.get_filename(),
                    e.get_line_number(),
                    e.what(),
                    error_code
            );
        }
        return_value = -1;
    }
    return return_value;
}
