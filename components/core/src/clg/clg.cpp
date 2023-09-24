// C libraries
#include <sys/stat.h>
#include <sys/socket.h>

// C++ libraries
#include <iostream>
#include <filesystem>
#include <memory>

// spdlog
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

// Project headers
#include "../Defs.h"
#include "../compressor_frontend/utils.hpp"
#include "../Grep.hpp"
#include "../GlobalMySQLMetadataDB.hpp"
#include "../GlobalSQLiteMetadataDB.hpp"
#include "../Profiler.hpp"
#include "../streaming_archive/Constants.hpp"
#include "CommandLineArguments.hpp"
#include "ControllerMonitoringThread.hpp"
#include "../networking/socket_utils.hpp"
#include "../Utils.hpp"

using clg::CommandLineArguments;
using compressor_frontend::load_lexer_from_file;
using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::to_string;
using std::vector;
using streaming_archive::MetadataDB;
using streaming_archive::reader::Archive;
using streaming_archive::reader::File;
using streaming_archive::reader::Message;

/**
 * Connects to the search controller
 * @param controller_host
 * @param controller_port
 * @return -1 on failure
 * @return Search controller socket file descriptor otherwise
 */
static int connect_to_search_controller(const string &controller_host, const string &controller_port);
/**
 * Opens the archive and reads the dictionaries
 * @param archive_path
 * @param archive_reader
 * @return true on success, false otherwise
 */
static bool open_archive(const string &archive_path, Archive &archive_reader);
/**
 * Searches the archive with the given parameters
 * @param search_strings
 * @param command_line_args
 * @param archive
 * @return true on success, false otherwise
 */
static bool search(const vector<string> &search_strings, CommandLineArguments &command_line_args, Archive &archive, bool use_heuristic);
/**
 * Opens a compressed file or logs any errors if it couldn't be opened
 * @param file_metadata_ix
 * @param archive
 * @param compressed_file
 * @return true on success, false otherwise
 */
static bool open_compressed_file(MetadataDB::FileIterator &file_metadata_ix, Archive &archive, File &compressed_file);
/**
 * Searches all files referenced by a given database cursor
 * @param queries
 * @param output_method
 * @param archive
 * @param file_metadata_ix
 * @return The total number of matches found across all files
 */
static size_t search_files(vector<Query> &queries, CommandLineArguments::OutputMethod output_method, Archive &archive,
                           MetadataDB::FileIterator &file_metadata_ix, const std::atomic_bool &query_cancelled, int controller_socket_fd);
/**
 * Prints search result to stdout in text format
 * @param orig_file_path
 * @param compressed_msg
 * @param decompressed_msg
 * @param custom_arg Unused
 */
static void print_result_text(const string &orig_file_path, const Message &compressed_msg, const string &decompressed_msg, void *custom_arg);
/**
 * Prints search result to stdout in binary format
 * @param orig_file_path
 * @param compressed_msg
 * @param decompressed_msg
 * @param custom_arg Unused
 */
static void print_result_binary(const string &orig_file_path, const Message &compressed_msg, const string &decompressed_msg, void *custom_arg);

/**
 * Gets an archive iterator for the given file path or for all files if the file path is empty
 * @param global_metadata_db
 * @param file_path
 * @return An archive iterator
 */
static GlobalMetadataDB::ArchiveIterator *get_archive_iterator(GlobalMetadataDB &global_metadata_db, const std::string &file_path);

static int connect_to_search_controller(const string &controller_host, const string &controller_port)
{
    // Get address info for controller
    struct addrinfo hints = {};
    // Address can be IPv4 or IPV6
    hints.ai_family = AF_UNSPEC;
    // TCP socket
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    struct addrinfo *addresses_head = nullptr;
    int error = getaddrinfo(controller_host.c_str(), controller_port.c_str(), &hints, &addresses_head);
    if (0 != error)
    {
        SPDLOG_ERROR("Failed to get address information for search controller, error={}", error);
        return -1;
    }

    // Try each address until a socket can be created and connected to
    int controller_socket_fd = -1;
    for (auto curr = addresses_head; nullptr != curr; curr = curr->ai_next)
    {
        // Create socket
        controller_socket_fd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (-1 == controller_socket_fd)
        {
            continue;
        }

        // Connect to address
        if (connect(controller_socket_fd, curr->ai_addr, curr->ai_addrlen) != -1)
        {
            break;
        }

        // Failed to connect, so close socket
        close(controller_socket_fd);
        controller_socket_fd = -1;
    }
    freeaddrinfo(addresses_head);
    if (-1 == controller_socket_fd)
    {
        SPDLOG_ERROR("Failed to connect to search controller, errno={}", errno);
        return -1;
    }

    return controller_socket_fd;
}

static GlobalMetadataDB::ArchiveIterator *get_archive_iterator(GlobalMetadataDB &global_metadata_db, const std::string &file_path)
{
    if (file_path.empty())
    {
        return global_metadata_db.get_archive_iterator();
    }
    else
    {
        return global_metadata_db.get_archive_iterator_for_file_path(file_path);
    }
}

static bool open_archive(const string &archive_path, Archive &archive_reader)
{
    ErrorCode error_code;

    try
    {
        // Open archive
        archive_reader.open(archive_path);
    }
    catch (TraceableException &e)
    {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code)
        {
            SPDLOG_ERROR("Opening archive failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return false;
        }
        else
        {
            SPDLOG_ERROR("Opening archive failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return false;
        }
    }

    try
    {
        archive_reader.refresh_dictionaries();
    }
    catch (TraceableException &e)
    {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code)
        {
            SPDLOG_ERROR("Reading dictionaries failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return false;
        }
        else
        {
            SPDLOG_ERROR("Reading dictionaries failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return false;
        }
    }

    return true;
}

static bool search(const vector<string> &search_strings, CommandLineArguments &command_line_args, Archive &archive,
                   compressor_frontend::lexers::ByteLexer &forward_lexer, compressor_frontend::lexers::ByteLexer &reverse_lexer, bool use_heuristic,
                   const std::atomic_bool &query_cancelled, int controller_socket_fd)
{
    ErrorCode error_code;
    auto search_begin_ts = command_line_args.get_search_begin_ts();
    auto search_end_ts = command_line_args.get_search_end_ts();

    try
    {
        vector<Query> queries;
        bool no_queries_match = true;
        std::set<segment_id_t> ids_of_segments_to_search;
        bool is_superseding_query = false;
        for (const auto &search_string : search_strings)
        {
            Query query;
            if (Grep::process_raw_query(archive, search_string, search_begin_ts, search_end_ts, command_line_args.ignore_case(), query, forward_lexer,
                                        reverse_lexer, use_heuristic))
            {
                // if (Grep::process_raw_query(archive, search_string, search_begin_ts, search_end_ts, command_line_args.ignore_case(), query, parser)) {
                no_queries_match = false;

                if (query.contains_sub_queries() == false)
                {
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
                for (auto &sub_query : query.get_sub_queries())
                {
                    auto &ids_of_matching_segments = sub_query.get_ids_of_matching_segments();
                    ids_of_segments_to_search.insert(ids_of_matching_segments.cbegin(), ids_of_matching_segments.cend());
                }
            }
        }

        if (!no_queries_match)
        {
            size_t num_matches;
            if (is_superseding_query)
            {
                auto file_metadata_ix = archive.get_file_iterator(search_begin_ts, search_end_ts, command_line_args.get_file_path());
                num_matches = search_files(queries, command_line_args.get_output_method(), archive, *file_metadata_ix, query_cancelled, controller_socket_fd);
            }
            else
            {
                auto file_metadata_ix_ptr = archive.get_file_iterator(search_begin_ts, search_end_ts, command_line_args.get_file_path(), cInvalidSegmentId);
                auto &file_metadata_ix = *file_metadata_ix_ptr;
                num_matches = search_files(queries, command_line_args.get_output_method(), archive, file_metadata_ix, query_cancelled, controller_socket_fd);
                for (auto segment_id : ids_of_segments_to_search)
                {
                    file_metadata_ix.set_segment_id(segment_id);
                    num_matches += search_files(queries, command_line_args.get_output_method(), archive, file_metadata_ix, query_cancelled, controller_socket_fd);
                }
            }
            SPDLOG_DEBUG("# matches found: {}", num_matches);
        }
    }
    catch (TraceableException &e)
    {
        error_code = e.get_error_code();
        if (ErrorCode_errno == error_code)
        {
            SPDLOG_ERROR("Search failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
            return false;
        }
        else
        {
            SPDLOG_ERROR("Search failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(), error_code);
            return false;
        }
    }

    return true;
}

static bool open_compressed_file(MetadataDB::FileIterator &file_metadata_ix, Archive &archive, File &compressed_file)
{
    ErrorCode error_code = archive.open_file(compressed_file, file_metadata_ix);
    if (ErrorCode_Success == error_code)
    {
        return true;
    }
    string orig_path;
    file_metadata_ix.get_path(orig_path);
    if (ErrorCode_FileNotFound == error_code)
    {
        SPDLOG_WARN("{} not found in archive", orig_path.c_str());
    }
    else if (ErrorCode_errno == error_code)
    {
        SPDLOG_ERROR("Failed to open {}, errno={}", orig_path.c_str(), errno);
    }
    else
    {
        SPDLOG_ERROR("Failed to open {}, error={}", orig_path.c_str(), error_code);
    }
    return false;
}

static size_t search_files(vector<Query> &queries, const CommandLineArguments::OutputMethod output_method, Archive &archive,
                           MetadataDB::FileIterator &file_metadata_ix,
                           const std::atomic_bool &query_cancelled, int controller_socket_fd)
{
    size_t num_matches = 0;

    File compressed_file;
    // Setup output method
    Grep::OutputFunc output_func;
    void *output_func_arg;
    switch (output_method)
    {
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
    for (; file_metadata_ix.has_next(); file_metadata_ix.next())
    {
        if (open_compressed_file(file_metadata_ix, archive, compressed_file))
        {
            Grep::calculate_sub_queries_relevant_to_file(compressed_file, queries);

            for (const auto &query : queries)
            {
                archive.reset_file_indices(compressed_file);
                num_matches += Grep::search_and_output(query, SIZE_MAX, archive, compressed_file, output_func, output_func_arg, query_cancelled, controller_socket_fd);
            }
        }
        archive.close_file(compressed_file);
    }

    return num_matches;
}

static void print_result_text(const string &orig_file_path, const Message &compressed_msg, const string &decompressed_msg, void *custom_arg)
{
    printf("%s:%s", orig_file_path.c_str(), decompressed_msg.c_str());
}

static void print_result_binary(const string &orig_file_path, const Message &compressed_msg, const string &decompressed_msg, void *custom_arg)
{
    bool write_successful = true;
    do
    {
        size_t length;
        size_t num_elems_written;

        // Write file path
        length = orig_file_path.length();
        num_elems_written = fwrite(&length, sizeof(length), 1, stdout);
        if (num_elems_written < 1)
        {
            write_successful = false;
            break;
        }
        num_elems_written = fwrite(orig_file_path.c_str(), sizeof(char), length, stdout);
        if (num_elems_written < length)
        {
            write_successful = false;
            break;
        }

        // Write timestamp
        epochtime_t timestamp = compressed_msg.get_ts_in_milli();
        num_elems_written = fwrite(&timestamp, sizeof(timestamp), 1, stdout);
        if (num_elems_written < 1)
        {
            write_successful = false;
            break;
        }

        // Write logtype ID
        auto logtype_id = compressed_msg.get_logtype_id();
        num_elems_written = fwrite(&logtype_id, sizeof(logtype_id), 1, stdout);
        if (num_elems_written < 1)
        {
            write_successful = false;
            break;
        }

        // Write message
        length = decompressed_msg.length();
        num_elems_written = fwrite(&length, sizeof(length), 1, stdout);
        if (num_elems_written < 1)
        {
            write_successful = false;
            break;
        }
        num_elems_written = fwrite(decompressed_msg.c_str(), sizeof(char), length, stdout);
        if (num_elems_written < length)
        {
            write_successful = false;
            break;
        }
    } while (false);
    if (!write_successful)
    {
        SPDLOG_ERROR("Failed to write result in binary form, errno={}", errno);
    }
}

int main(int argc, const char *argv[])
{
    // Program-wide initialization
    try
    {
        auto stderr_logger = spdlog::stderr_logger_st("stderr");
        spdlog::set_default_logger(stderr_logger);
        spdlog::set_pattern("%Y-%m-%d %H:%M:%S,%e [%l] %v");
    }
    catch (std::exception &e)
    {
        // NOTE: We can't log an exception if the logger couldn't be constructed
        std::cout << e.what();
        return -1;
    }

    try
    {
        Profiler::init();
        TimestampPattern::init();
        CommandLineArguments command_line_args("clg");
        auto parsing_result = command_line_args.parse_arguments(argc, argv);
        switch (parsing_result)
        {
        case CommandLineArgumentsBase::ParsingResult::Failure:
            return -1;
        case CommandLineArgumentsBase::ParsingResult::InfoCommand:
            return 0;
        case CommandLineArgumentsBase::ParsingResult::Success:
            // Continue processing
            break;
        }

        Profiler::start_continuous_measurement<Profiler::ContinuousMeasurementIndex::Search>();

        // Create vector of search strings
        vector<string> search_strings;
        search_strings.push_back(command_line_args.get_search_string());

        // Validate archives directory
        struct stat archives_dir_stat = {};
        auto archives_dir = std::filesystem::path(command_line_args.get_archive_path());
        if (0 != stat(archives_dir.c_str(), &archives_dir_stat))
        {
            SPDLOG_ERROR("'{}' does not exist or cannot be accessed - {}.", archives_dir.c_str(), strerror(errno));
            return -1;
        }
        else if (S_ISDIR(archives_dir_stat.st_mode) == false)
        {
            SPDLOG_ERROR("'{}' is not a directory.", archives_dir.c_str());
            return -1;
        }

        /// TODO: if performance is too slow, can make this more efficient by only diffing files with the same checksum
        const uint32_t max_map_schema_length = 100000;
        std::map<std::string, compressor_frontend::lexers::ByteLexer> forward_lexer_map;
        std::map<std::string, compressor_frontend::lexers::ByteLexer> reverse_lexer_map;
        compressor_frontend::lexers::ByteLexer one_time_use_forward_lexer;
        compressor_frontend::lexers::ByteLexer one_time_use_reverse_lexer;
        compressor_frontend::lexers::ByteLexer *forward_lexer_ptr;
        compressor_frontend::lexers::ByteLexer *reverse_lexer_ptr;

        Archive archive_reader;

        const std::vector<std::string> archive_paths = command_line_args.get_archive_path();

        int controller_socket_fd = connect_to_search_controller(command_line_args.get_search_controller_host(),
                                                                command_line_args.get_search_controller_port());
        if (-1 == controller_socket_fd)
        {
            return -1;
        }
        ControllerMonitoringThread controller_monitoring_thread(controller_socket_fd);
        controller_monitoring_thread.start();
        // ==================
        for (const std::string &archive_path : archive_paths)
        {

            if (false == std::filesystem::exists(archive_path))
            {
                SPDLOG_WARN("Archive {} does not exist in '{}'.", archive_path, command_line_args.get_archive_path());
                return -1;
            }

            // Open archive
            if (!open_archive(archive_path, archive_reader))
            {
                return -1;
            }

            // Generate lexer if schema file exists
            auto schema_file_path = archive_path + "/" + streaming_archive::cSchemaFileName;
            bool use_heuristic = true;
            if (std::filesystem::exists(schema_file_path))
            {
                use_heuristic = false;

                char buf[max_map_schema_length];
                FileReader file_reader;
                file_reader.try_open(schema_file_path);

                size_t num_bytes_read;
                file_reader.read(buf, max_map_schema_length, num_bytes_read);
                if (num_bytes_read < max_map_schema_length)
                {
                    auto forward_lexer_map_it = forward_lexer_map.find(buf);
                    auto reverse_lexer_map_it = reverse_lexer_map.find(buf);
                    // if there is a chance there might be a difference make a new lexer as it's pretty fast to create
                    if (forward_lexer_map_it == forward_lexer_map.end())
                    {
                        // Create forward lexer
                        auto insert_result = forward_lexer_map.emplace(buf, compressor_frontend::lexers::ByteLexer());
                        forward_lexer_ptr = &insert_result.first->second;
                        load_lexer_from_file(schema_file_path, false, *forward_lexer_ptr);

                        // Create reverse lexer
                        insert_result = reverse_lexer_map.emplace(buf, compressor_frontend::lexers::ByteLexer());
                        reverse_lexer_ptr = &insert_result.first->second;
                        load_lexer_from_file(schema_file_path, true, *reverse_lexer_ptr);
                    }
                    else
                    {
                        // load the lexers if they already exist
                        forward_lexer_ptr = &forward_lexer_map_it->second;
                        reverse_lexer_ptr = &reverse_lexer_map_it->second;
                    }
                }
                else
                {
                    // Create forward lexer
                    forward_lexer_ptr = &one_time_use_forward_lexer;
                    load_lexer_from_file(schema_file_path, false, one_time_use_forward_lexer);

                    // Create reverse lexer
                    reverse_lexer_ptr = &one_time_use_reverse_lexer;
                    load_lexer_from_file(schema_file_path, false, one_time_use_reverse_lexer);
                }
            }

            // int return_value = 0;
            // Perform search
            if (!search(search_strings, command_line_args, archive_reader, *forward_lexer_ptr, *reverse_lexer_ptr, use_heuristic,
                        controller_monitoring_thread.get_query_cancelled(), controller_socket_fd))
            {
                return -1;
            }
            archive_reader.close();

            Profiler::stop_continuous_measurement<Profiler::ContinuousMeasurementIndex::Search>();
            LOG_CONTINUOUS_MEASUREMENT(Profiler::ContinuousMeasurementIndex::Search)

            auto shutdown_result = shutdown(controller_socket_fd, SHUT_RDWR);
            if (0 != shutdown_result)
            {
                if (ENOTCONN != shutdown_result)
                {
                    SPDLOG_ERROR("Failed to shutdown socket, error={}", shutdown_result);
                } // else connection already disconnected, so nothing to do
            }

            try
            {
                controller_monitoring_thread.join();
            }
            catch (TraceableException &e)
            {
                auto error_code = e.get_error_code();
                if (ErrorCode_errno == error_code)
                {
                    SPDLOG_ERROR("Failed to join with controller monitoring thread: {}:{} {}, errno={}",
                                 e.get_filename(), e.get_line_number(), e.what(), errno);
                }
                else
                {
                    SPDLOG_ERROR("Failed to join with controller monitoring thread: {}:{} {}, "
                                 "error_code={}",
                                 e.get_filename(), e.get_line_number(), e.what(),
                                 error_code);
                }
                return -1;
            }
        }
        // ================================
        return 0;
    }
    catch (std::exception &e)
    {
        std::cout << e.what();
        // NOTE: We can't log an exception if the logger couldn't be constructed
        return -1;
    }
}
