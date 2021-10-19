// C++ libraries
#include <iostream>

// Boost libraries
#include <boost/filesystem.hpp>

// spdlog
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

// Project headers
#include "../Defs.h"
#include "../Grep.hpp"
#include "../Profiler.hpp"
#include "../streaming_archive/Constants.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"

using clo::CommandLineArguments;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::to_string;
using std::vector;
using streaming_archive::MetadataDB;
using streaming_archive::reader::Archive;
using streaming_archive::reader::File;
using streaming_archive::reader::Message;

/**
 * Prints search result to stdout in binary format
 * @param orig_file_path
 * @param compressed_msg
 * @param decompressed_msg
 */
static void print_result (const string& orig_file_path, const Message& compressed_msg, const string& decompressed_msg);
/**
 * Searches all files referenced by a given database cursor
 * @param query
 * @param archive
 * @param file_metadata_ix
 * @return true on success, false otherwise
 */
static bool search_files (Query& query, Archive& archive, MetadataDB::FileIterator& file_metadata_ix);
/**
 * Searches an archive with the given path
 * @param command_line_args
 * @param archive_path
 * @return true on success, false otherwise
 */
static bool search_archive (const CommandLineArguments& command_line_args, const boost::filesystem::path& archive_path);

static void print_result (const string& orig_file_path, const Message& compressed_msg, const string& decompressed_msg) {
    bool write_successful = true;
    do {
        size_t length;
        size_t num_elems_written;

        // Write file path
        length = orig_file_path.length();
        num_elems_written = fwrite(&length, sizeof(length), 1, stdout);
        if (num_elems_written < 1) {
            write_successful = false;
            break;
        }
        num_elems_written = fwrite(orig_file_path.c_str(), sizeof(char), length, stdout);
        if (num_elems_written < length) {
            write_successful = false;
            break;
        }

        // Write timestamp
        epochtime_t timestamp = compressed_msg.get_ts_in_milli();
        num_elems_written = fwrite(&timestamp, sizeof(timestamp), 1, stdout);
        if (num_elems_written < 1) {
            write_successful = false;
            break;
        }

        // Write logtype ID
        auto logtype_id = compressed_msg.get_logtype_id();
        num_elems_written = fwrite(&logtype_id, sizeof(logtype_id), 1, stdout);
        if (num_elems_written < 1) {
            write_successful = false;
            break;
        }

        // Write message
        length = decompressed_msg.length();
        num_elems_written = fwrite(&length, sizeof(length), 1, stdout);
        if (num_elems_written < 1) {
            write_successful = false;
            break;
        }
        num_elems_written = fwrite(decompressed_msg.c_str(), sizeof(char), length, stdout);
        if (num_elems_written < length) {
            write_successful = false;
            break;
        }
    } while (false);
    if (!write_successful) {
        SPDLOG_ERROR("Failed to write result in binary form, errno={}", errno);
    }
}

static bool search_files (Query& query, Archive& archive, MetadataDB::FileIterator& file_metadata_ix) {
    bool error_occurred = false;

    File compressed_file;
    Message compressed_message;
    string decompressed_message;

    // Run query on each file
    for (; file_metadata_ix.has_next(); file_metadata_ix.next()) {
        ErrorCode error_code = archive.open_file(compressed_file, file_metadata_ix, false);
        if (ErrorCode_Success != error_code) {
            string orig_path;
            file_metadata_ix.get_path(orig_path);
            if (ErrorCode_errno == error_code) {
                SPDLOG_ERROR("Failed to open {}, errno={}", orig_path.c_str(), errno);
            } else {
                SPDLOG_ERROR("Failed to open {}, error={}", orig_path.c_str(), error_code);
            }
            error_occurred = true;
            continue;
        }

        if (compressed_file.is_in_segment()) {
            query.make_sub_queries_relevant_to_segment(compressed_file.get_segment_id());
        } else {
            query.make_all_sub_queries_relevant();
        }
        while (Grep::search_and_decompress(query, archive, compressed_file, compressed_message, decompressed_message)) {
            print_result(compressed_file.get_orig_path(), compressed_message, decompressed_message);
        }

        archive.close_file(compressed_file);
    }

    return error_occurred;
}

static bool search_archive (const CommandLineArguments& command_line_args, const boost::filesystem::path& archive_path) {
    if (false == boost::filesystem::exists(archive_path)) {
        SPDLOG_ERROR("Archive '{}' does not exist.", archive_path.c_str());
        return false;
    }
    auto archive_metadata_file = archive_path / streaming_archive::cMetadataFileName;
    if (false == boost::filesystem::exists(archive_metadata_file)) {
        SPDLOG_ERROR("Archive metadata file '{}' does not exist. '{}' may not be an archive.", archive_metadata_file.c_str(), archive_path.c_str());
        return false;
    }

    Archive archive_reader;
    archive_reader.open(archive_path.string());
    archive_reader.refresh_dictionaries();

    auto search_begin_ts = command_line_args.get_search_begin_ts();
    auto search_end_ts = command_line_args.get_search_end_ts();

    Query query;
    if (false == Grep::process_raw_query(archive_reader, command_line_args.get_search_string(), search_begin_ts, search_end_ts, command_line_args.ignore_case(),
                                         query))
    {
        return false;
    }

    // Get all segments potentially containing query results
    std::set<segment_id_t> ids_of_segments_to_search;
    for (auto& sub_query : query.get_sub_queries()) {
        auto& ids_of_matching_segments = sub_query.get_ids_of_matching_segments();
        ids_of_segments_to_search.insert(ids_of_matching_segments.cbegin(), ids_of_matching_segments.cend());
    }

    // Search files outside a segment
    auto file_metadata_ix_ptr = archive_reader.get_file_iterator(search_begin_ts, search_end_ts, command_line_args.get_file_path(), cInvalidSegmentId);
    auto& file_metadata_ix = *file_metadata_ix_ptr;
    search_files(query, archive_reader, file_metadata_ix);

    // Search files inside a segment
    for (auto segment_id : ids_of_segments_to_search) {
        file_metadata_ix.set_segment_id(segment_id);
        search_files(query, archive_reader, file_metadata_ix);
    }
    file_metadata_ix_ptr.reset(nullptr);

    archive_reader.close();

    return true;
}

int main (int argc, const char* argv[]) {
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

    const auto archive_path = boost::filesystem::path(command_line_args.get_archive_path());

    try {
        if (false == search_archive(command_line_args, archive_path)) {
            return -1;
        }
    } catch (TraceableException& e) {
        auto error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Search failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return -1;
        } else {
            SPDLOG_ERROR("Search failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return -1;
        }
    }

    return 0;
}
