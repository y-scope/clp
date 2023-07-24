// C standard libraries
#include <sys/socket.h>

// C++ libraries
#include <iostream>
#include <memory>

// Boost libraries
#include <boost/filesystem.hpp>

// msgpack
#include <msgpack.hpp>

// spdlog
#include <spdlog/sinks/stdout_sinks.h>

// Project headers
#include "../Defs.h"
#include "../Grep.hpp"
#include "../Profiler.hpp"
#include "../networking/socket_utils.hpp"
#include "../spdlog_with_specializations.hpp"
#include "../streaming_archive/Constants.hpp"
#include "../Utils.hpp"
#include "CommandLineArguments.hpp"
#include "ControllerMonitoringThread.hpp"

using clo::CommandLineArguments;
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::to_string;
using std::unique_ptr;
using std::vector;
using streaming_archive::MetadataDB;
using streaming_archive::reader::Archive;
using streaming_archive::reader::File;
using streaming_archive::reader::Message;

// Local types
enum class SearchFilesResult {
    OpenFailure,
    ResultSendFailure,
    Success
};

/**
 * Connects to the search controller
 * @param controller_host
 * @param controller_port
 * @return -1 on failure
 * @return Search controller socket file descriptor otherwise
 */
static int connect_to_search_controller (const string& controller_host, const string& controller_port);
/**
 * Sends the search result to the search controller
 * @param orig_file_path
 * @param compressed_msg
 * @param decompressed_msg
 * @param controller_socket_fd
 * @return Same as networking::try_send
 */
static ErrorCode send_result (const string& orig_file_path, const Message& compressed_msg,
                              const string& decompressed_msg, int controller_socket_fd);
/**
 * Searches all files referenced by a given database cursor
 * @param query
 * @param archive
 * @param file_metadata_ix
 * @param query_cancelled
 * @param controller_socket_fd
 * @return SearchFilesResult::OpenFailure on failure to open a compressed file
 * @return SearchFilesResult::ResultSendFailure on failure to send a result
 * @return SearchFilesResult::Success otherwise
 */
static SearchFilesResult search_files (Query& query, Archive& archive, MetadataDB::FileIterator& file_metadata_ix,
                                       const std::atomic_bool& query_cancelled, int controller_socket_fd);
/**
 * Searches an archive with the given path
 * @param command_line_args
 * @param archive_path
 * @param query_cancelled
 * @param controller_socket_fd
 * @return true on success, false otherwise
 */
static bool search_archive (const CommandLineArguments& command_line_args, const boost::filesystem::path& archive_path,
                            const std::atomic_bool& query_cancelled, int controller_socket_fd);

static int connect_to_search_controller (const string& controller_host, const string& controller_port) {
    // Get address info for controller
    struct addrinfo hints = {};
    // Address can be IPv4 or IPV6
    hints.ai_family = AF_UNSPEC;
    // TCP socket
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    struct addrinfo* addresses_head = nullptr;
    int error = getaddrinfo(controller_host.c_str(), controller_port.c_str(), &hints, &addresses_head);
    if (0 != error) {
        SPDLOG_ERROR("Failed to get address information for search controller, error={}", error);
        return -1;
    }

    // Try each address until a socket can be created and connected to
    int controller_socket_fd = -1;
    for (auto curr = addresses_head; nullptr != curr; curr = curr->ai_next) {
        // Create socket
        controller_socket_fd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (-1 == controller_socket_fd) {
            continue;
        }

        // Connect to address
        if (connect(controller_socket_fd, curr->ai_addr, curr->ai_addrlen) != -1) {
            break;
        }

        // Failed to connect, so close socket
        close(controller_socket_fd);
        controller_socket_fd = -1;
    }
    freeaddrinfo(addresses_head);
    if (-1 == controller_socket_fd) {
        SPDLOG_ERROR("Failed to connect to search controller, errno={}", errno);
        return -1;
    }

    return controller_socket_fd;
}

static ErrorCode send_result (const string& orig_file_path, const Message& compressed_msg,
                              const string& decompressed_msg, int controller_socket_fd)
{
    msgpack::type::tuple<std::string, epochtime_t, std::string> src(orig_file_path, compressed_msg.get_ts_in_milli(),
                                                                    decompressed_msg);
    msgpack::sbuffer m;
    msgpack::pack(m, src);
    return networking::try_send(controller_socket_fd, m.data(), m.size());
}

static SearchFilesResult search_files (Query& query, Archive& archive, MetadataDB::FileIterator& file_metadata_ix,
                                       const std::atomic_bool& query_cancelled, int controller_socket_fd)
{
    SearchFilesResult result = SearchFilesResult::Success;

    File compressed_file;
    Message compressed_message;
    string decompressed_message;

    // Run query on each file
    for (; file_metadata_ix.has_next(); file_metadata_ix.next()) {
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
        while (false == query_cancelled &&
               Grep::search_and_decompress(query, archive, compressed_file, compressed_message, decompressed_message))
        {
            error_code = send_result(compressed_file.get_orig_path(), compressed_message, decompressed_message,
                                     controller_socket_fd);
            if (ErrorCode_Success != error_code) {
                result = SearchFilesResult::ResultSendFailure;
                break;
            }
        }
        if (SearchFilesResult::ResultSendFailure == result) {
            // Stop search now since results aren't reaching the controller
            break;
        }

        archive.close_file(compressed_file);
    }

    return result;
}

static bool search_archive (const CommandLineArguments& command_line_args, const boost::filesystem::path& archive_path,
                            const std::atomic_bool& query_cancelled, int controller_socket_fd)
{
    if (false == boost::filesystem::exists(archive_path)) {
        SPDLOG_ERROR("Archive '{}' does not exist.", archive_path.c_str());
        return false;
    }
    auto archive_metadata_file = archive_path / streaming_archive::cMetadataFileName;
    if (false == boost::filesystem::exists(archive_metadata_file)) {
        SPDLOG_ERROR("Archive metadata file '{}' does not exist. '{}' may not be an archive.",
                     archive_metadata_file.c_str(), archive_path.c_str());
        return false;
    }

    // Load lexers from schema file if it exists
    auto schema_file_path = archive_path / streaming_archive::cSchemaFileName;
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

    Query query;
    if (false == Grep::process_raw_query(archive_reader, command_line_args.get_search_string(), search_begin_ts,
                                         search_end_ts, command_line_args.ignore_case(), query, *forward_lexer,
                                         *reverse_lexer, use_heuristic))
    {
        return true;
    }

    // Get all segments potentially containing query results
    std::set<segment_id_t> ids_of_segments_to_search;
    for (auto& sub_query : query.get_sub_queries()) {
        auto& ids_of_matching_segments = sub_query.get_ids_of_matching_segments();
        ids_of_segments_to_search.insert(ids_of_matching_segments.cbegin(), ids_of_matching_segments.cend());
    }

    // Search segments
    auto file_metadata_ix_ptr = archive_reader.get_file_iterator(search_begin_ts, search_end_ts,
                                                                 command_line_args.get_file_path(), cInvalidSegmentId);
    auto& file_metadata_ix = *file_metadata_ix_ptr;
    for (auto segment_id : ids_of_segments_to_search) {
        file_metadata_ix.set_segment_id(segment_id);
        auto result = search_files(query, archive_reader, file_metadata_ix, query_cancelled, controller_socket_fd);
        if (SearchFilesResult::ResultSendFailure == result) {
            // Stop search now since results aren't reaching the controller
            break;
        }
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
    Profiler::init();
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

    int controller_socket_fd = connect_to_search_controller(command_line_args.get_search_controller_host(),
                                                            command_line_args.get_search_controller_port());
    if (-1 == controller_socket_fd) {
        return -1;
    }

    const auto archive_path = boost::filesystem::path(command_line_args.get_archive_path());

    ControllerMonitoringThread controller_monitoring_thread(controller_socket_fd);
    controller_monitoring_thread.start();

    int return_value = 0;
    try {
        if (false == search_archive(command_line_args, archive_path, controller_monitoring_thread.get_query_cancelled(),
                                    controller_socket_fd))
        {
            return_value = -1;
        }
    } catch (TraceableException& e) {
        auto error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Search failed: {}:{} {}, errno={}", e.get_filename(), e.get_line_number(), e.what(), errno);
        } else {
            SPDLOG_ERROR("Search failed: {}:{} {}, error_code={}", e.get_filename(), e.get_line_number(), e.what(),
                         error_code);
        }
        return_value = -1;
    }

    // Unblock the controller monitoring thread if it's blocked
    auto shutdown_result = shutdown(controller_socket_fd, SHUT_RDWR);
    if (0 != shutdown_result) {
        if (ENOTCONN != shutdown_result) {
            SPDLOG_ERROR("Failed to shutdown socket, error={}", shutdown_result);
        } // else connection already disconnected, so nothing to do
    }

    try {
        controller_monitoring_thread.join();
    } catch (TraceableException& e) {
        auto error_code = e.get_error_code();
        if (ErrorCode_errno == error_code) {
            SPDLOG_ERROR("Failed to join with controller monitoring thread: {}:{} {}, errno={}",
                         e.get_filename(), e.get_line_number(), e.what(), errno);
        } else {
            SPDLOG_ERROR("Failed to join with controller monitoring thread: {}:{} {}, "
                         "error_code={}", e.get_filename(), e.get_line_number(), e.what(),
                         error_code);
        }
        return_value = -1;
    }

    return return_value;
}
