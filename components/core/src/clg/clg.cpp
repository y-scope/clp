// C libraries
#include <sys/stat.h>

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
#include "../GlobalMySQLMetadataDB.hpp"
#include "../GlobalSQLiteMetadataDB.hpp"
#include "../Profiler.hpp"
#include "../streaming_archive/Constants.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"

using clg::CommandLineArguments;
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
 * Opens the archive and reads the dictionaries
 * @param archive_path
 * @param archive_reader
 * @return true on success, false otherwise
 */
static bool open_archive (const string& archive_path, Archive& archive_reader);
/**
 * Searches the archive with the given parameters
 * @param search_strings
 * @param command_line_args
 * @param archive
 * @return true on success, false otherwise
 */
static bool search (const vector<string>& search_strings, CommandLineArguments& command_line_args, Archive& archive);
/**
 * Opens a compressed file or logs any errors if it couldn't be opened
 * @param file_metadata_ix
 * @param archive
 * @param compressed_file
 * @return true on success, false otherwise
 */
static bool open_compressed_file (MetadataDB::FileIterator& file_metadata_ix, Archive& archive, File& compressed_file);
/**
 * Searches all files referenced by a given database cursor
 * @param queries
 * @param output_method
 * @param archive
 * @param file_metadata_ix
 * @return The total number of matches found across all files
 */
static size_t search_files (vector<Query>& queries, CommandLineArguments::OutputMethod output_method, Archive& archive,
                            MetadataDB::FileIterator& file_metadata_ix);
/**
 * Prints search result to stdout in text format
 * @param orig_file_path
 * @param compressed_msg
 * @param decompressed_msg
 * @param custom_arg Unused
 */
static void print_result_text (const string& orig_file_path, const Message& compressed_msg, const string& decompressed_msg, void* custom_arg);
/**
 * Prints search result to stdout in binary format
 * @param orig_file_path
 * @param compressed_msg
 * @param decompressed_msg
 * @param custom_arg Unused
 */
static void print_result_binary (const string& orig_file_path, const Message& compressed_msg, const string& decompressed_msg, void* custom_arg);

/**
 * Gets an archive iterator for the given file path or for all files if the file path is empty
 * @param global_metadata_db
 * @param file_path
 * @return An archive iterator
 */
static GlobalMetadataDB::ArchiveIterator* get_archive_iterator (GlobalMetadataDB& global_metadata_db, const std::string& file_path);

static GlobalMetadataDB::ArchiveIterator* get_archive_iterator (GlobalMetadataDB& global_metadata_db, const std::string& file_path) {
    if (file_path.empty()) {
        return global_metadata_db.get_archive_iterator();
    } else {
        return global_metadata_db.get_archive_iterator_for_file_path(file_path);
    }
}

static bool open_archive (const string& archive_path, Archive& archive_reader) {
    ErrorCode error_code;

    try {
        // Open archive
        archive_reader.open(archive_path);
    } catch (TraceableException& e) {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Opening archive failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return false;
        } else {
            SPDLOG_ERROR("Opening archive failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return false;
        }
    }

    try {
        archive_reader.refresh_dictionaries();
    } catch (TraceableException& e) {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Reading dictionaries failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return false;
        } else {
            SPDLOG_ERROR("Reading dictionaries failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return false;
        }
    }

    return true;
}

static bool search (const vector<string>& search_strings, CommandLineArguments& command_line_args, Archive& archive) {
    ErrorCode error_code;
    auto search_begin_ts = command_line_args.get_search_begin_ts();
    auto search_end_ts = command_line_args.get_search_end_ts();

    try {
        vector<Query> queries;
        bool no_queries_match = true;
        std::set<segment_id_t> ids_of_segments_to_search;
        bool is_superseding_query = false;
        for (const auto& search_string : search_strings) {
            Query query;
            if (Grep::process_raw_query(archive, search_string, search_begin_ts, search_end_ts, command_line_args.ignore_case(), query)) {
                no_queries_match = false;

                if (query.contains_sub_queries() == false) {
                    // Search string supersedes all other possible search strings
                    is_superseding_query = true;
                    // Remove existing queries since they are superseded by this one
                    queries.clear();
                    // Add this query
                    queries.push_back(query);
                    // All other search strings will be superseded by this one, so break
                    break;
                }

                queries.push_back(query);

                // Add query's matching segments to segments to search
                for (auto& sub_query : query.get_sub_queries()) {
                    auto& ids_of_matching_segments = sub_query.get_ids_of_matching_segments();
                    ids_of_segments_to_search.insert(ids_of_matching_segments.cbegin(), ids_of_matching_segments.cend());
                }
            }
        }

        if (!no_queries_match) {
            size_t num_matches;
            if (is_superseding_query) {
                auto file_metadata_ix = archive.get_file_iterator(search_begin_ts, search_end_ts, command_line_args.get_file_path());
                num_matches = search_files(queries, command_line_args.get_output_method(), archive, file_metadata_ix);
            } else {
                auto file_metadata_ix = archive.get_file_iterator(search_begin_ts, search_end_ts, command_line_args.get_file_path(), cInvalidSegmentId);
                num_matches = search_files(queries, command_line_args.get_output_method(), archive, file_metadata_ix);
                for (auto segment_id : ids_of_segments_to_search) {
                    file_metadata_ix.set_segment_id(segment_id);
                    num_matches += search_files(queries, command_line_args.get_output_method(), archive, file_metadata_ix);
                }
            }
            SPDLOG_DEBUG("# matches found: {}", num_matches);
        }
    } catch (TraceableException& e) {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Search failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return false;
        } else {
            SPDLOG_ERROR("Search failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return false;
        }
    }

    return true;
}

static bool open_compressed_file (MetadataDB::FileIterator& file_metadata_ix, Archive& archive, File& compressed_file) {
    ErrorCode error_code = archive.open_file(compressed_file, file_metadata_ix, false);
    if (ErrorCode_Success == error_code) {
        return true;
    }
    string orig_path;
    file_metadata_ix.get_path(orig_path);
    if (ErrorCode_FileNotFound == error_code) {
        SPDLOG_WARN("{} not found in archive", orig_path.c_str());
    } else if (ErrorCode_errno == error_code) {
        SPDLOG_ERROR("Failed to open {}, errno={}", orig_path.c_str(), errno);
    } else {
        SPDLOG_ERROR("Failed to open {}, error={}", orig_path.c_str(), error_code);
    }
    return false;
}

static size_t search_files (vector<Query>& queries, const CommandLineArguments::OutputMethod output_method, Archive& archive,
                            MetadataDB::FileIterator& file_metadata_ix)
{
    size_t num_matches = 0;

    File compressed_file;
    // Setup output method
    Grep::OutputFunc output_func;
    void* output_func_arg;
    switch (output_method) {
        case CommandLineArguments::OutputMethod::StdoutText:
            output_func = print_result_text;
            output_func_arg = nullptr;
            break;
        case CommandLineArguments::OutputMethod::StdoutBinary:
            output_func = print_result_binary;
            output_func_arg = nullptr;
            break;
        default:
            SPDLOG_ERROR("Unknown output method - {}", (char)output_method);
            return num_matches;
    }

    // Run all queries on each file
    for (; file_metadata_ix.has_next(); file_metadata_ix.next()) {
        if (open_compressed_file(file_metadata_ix, archive, compressed_file)) {
            Grep::calculate_sub_queries_relevant_to_file(compressed_file, queries);

            for (const auto& query : queries) {
                archive.reset_file_indices(compressed_file);
                num_matches += Grep::search_and_output(query, SIZE_MAX, archive, compressed_file, output_func, output_func_arg);
            }
        }
        archive.close_file(compressed_file);
    }

    return num_matches;
}

static void print_result_text (const string& orig_file_path, const Message& compressed_msg, const string& decompressed_msg, void* custom_arg) {
    printf("%s:%s", orig_file_path.c_str(), decompressed_msg.c_str());
}

static void print_result_binary (const string& orig_file_path, const Message& compressed_msg, const string& decompressed_msg, void* custom_arg) {
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

    CommandLineArguments command_line_args("clg");
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

    // Create vector of search strings
    vector<string> search_strings;
    if (command_line_args.get_search_strings_file_path().empty()) {
        search_strings.push_back(command_line_args.get_search_string());
    } else {
        FileReader file_reader;
        file_reader.open(command_line_args.get_search_strings_file_path());
        string line;
        while (file_reader.read_to_delimiter('\n', false, false, line)) {
            if (!line.empty()) {
                search_strings.push_back(line);
            }
        }
        file_reader.close();
    }

    // Validate archives directory
    struct stat archives_dir_stat = {};
    auto archives_dir = boost::filesystem::path(command_line_args.get_archives_dir());
    if (0 != stat(archives_dir.c_str(), &archives_dir_stat)) {
        SPDLOG_ERROR("'{}' does not exist or cannot be accessed - {}.", archives_dir.c_str(), strerror(errno));
        return -1;
    } else if (S_ISDIR(archives_dir_stat.st_mode) == false) {
        SPDLOG_ERROR("'{}' is not a directory.", archives_dir.c_str());
        return -1;
    }

    const auto& global_metadata_db_config = command_line_args.get_metadata_db_config();
    std::unique_ptr<GlobalMetadataDB> global_metadata_db;
    switch (global_metadata_db_config.get_metadata_db_type()) {
        case GlobalMetadataDBConfig::MetadataDBType::SQLite: {
            auto global_metadata_db_path = archives_dir / streaming_archive::cMetadataDBFileName;
            global_metadata_db = std::make_unique<GlobalSQLiteMetadataDB>(global_metadata_db_path.string());
            break;
        }
        case GlobalMetadataDBConfig::MetadataDBType::MySQL:
            global_metadata_db = std::make_unique<GlobalMySQLMetadataDB>(global_metadata_db_config.get_metadata_db_host(),
                                                                         global_metadata_db_config.get_metadata_db_port(),
                                                                         global_metadata_db_config.get_metadata_db_username(),
                                                                         global_metadata_db_config.get_metadata_db_password(),
                                                                         global_metadata_db_config.get_metadata_db_name(),
                                                                         global_metadata_db_config.get_metadata_table_prefix());
            break;
    }
    global_metadata_db->open();

    string archive_id;
    Archive archive_reader;
    for (auto archive_ix = std::unique_ptr<GlobalMetadataDB::ArchiveIterator>(get_archive_iterator(*global_metadata_db, command_line_args.get_file_path()));
            archive_ix->contains_element(); archive_ix->get_next())
    {
        archive_ix->get_id(archive_id);
        auto archive_path = archives_dir / archive_id;

        if (false == boost::filesystem::exists(archive_path)) {
            SPDLOG_WARN("Archive {} does not exist in '{}'.", archive_id, command_line_args.get_archives_dir());
            continue;
        }

        // Perform search
        if (!open_archive(archive_path.string(), archive_reader)) {
            return -1;
        }
        if (!search(search_strings, command_line_args, archive_reader)) {
            return -1;
        }
        archive_reader.close();
    }

    global_metadata_db->close();

    return 0;
}
