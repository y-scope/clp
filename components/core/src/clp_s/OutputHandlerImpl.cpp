#include "OutputHandlerImpl.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/bulk_write_exception.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/insert.hpp>
#include <msgpack.hpp>
#include <spdlog/spdlog.h>

#include <clp_s/ResultsCacheUtils.hpp>

#include "../clp/networking/socket_utils.hpp"
#include "../reducer/CountOperator.hpp"
#include "../reducer/network_utils.hpp"
#include "../reducer/Record.hpp"
#include "archive_constants.hpp"
#include "search/OutputHandler.hpp"
#include "TraceableException.hpp"

using std::string;
using std::string_view;

namespace clp_s {
namespace {
constexpr int32_t cDuplicateKeyErrorCode{11'000};

/**
 * Checks whether a bulk-write reply reports a successful command.
 * @param reply The raw MongoDB bulk-write reply.
 * @return true if the reply contains no command error, false otherwise.
 */
[[nodiscard]] auto is_successful_command_reply(bsoncxx::document::view const& reply) -> bool {
    if (static_cast<bool>(reply["code"]) || static_cast<bool>(reply["errmsg"])) {
        return false;
    }

    auto const command_status = reply["ok"];
    if (false == static_cast<bool>(command_status)) {
        return true;
    }
    if (bsoncxx::type::k_double == command_status.type()) {
        return 1.0 == command_status.get_double().value;
    }
    if (bsoncxx::type::k_int32 == command_status.type()) {
        return 1 == command_status.get_int32().value;
    }
    if (bsoncxx::type::k_int64 == command_status.type()) {
        return 1 == command_status.get_int64().value;
    }
    return false;
}

/**
 * Checks whether a bulk-write reply contains any write-concern errors.
 * @param reply The raw MongoDB bulk-write reply.
 * @return true if the reply contains a write-concern error, false otherwise.
 */
[[nodiscard]] auto has_write_concern_errors(bsoncxx::document::view const& reply) -> bool {
    if (static_cast<bool>(reply["writeConcernError"])) {
        return true;
    }

    auto const errors_element = reply["writeConcernErrors"];
    if (false == static_cast<bool>(errors_element)) {
        return false;
    }
    if (bsoncxx::type::k_array != errors_element.type()) {
        return true;
    }
    auto const errors = errors_element.get_array().value;
    return errors.begin() != errors.end();
}

/**
 * Checks whether an entry from a bulk-write reply's `writeErrors` array is a duplicate-key error.
 * @param write_error The write-error entry to inspect.
 * @return true if the entry has MongoDB's duplicate-key error code, false otherwise.
 */
[[nodiscard]] auto is_duplicate_key_write_error(bsoncxx::array::element const& write_error)
        -> bool {
    if (bsoncxx::type::k_document != write_error.type()) {
        return false;
    }

    auto const code = write_error.get_document().value["code"];
    if (false == static_cast<bool>(code)) {
        return false;
    }
    if (bsoncxx::type::k_int32 == code.type()) {
        return cDuplicateKeyErrorCode == code.get_int32().value;
    }
    if (bsoncxx::type::k_int64 == code.type()) {
        return cDuplicateKeyErrorCode == code.get_int64().value;
    }
    return false;
}

/**
 * Returns whether the bulk write failed only because some documents already exist.
 *
 * Command and write-concern errors are rejected since they mean MongoDB did not confirm the
 * outcome of the entire batch. At least one write error must be present, and every write error
 * must be a duplicate-key error.
 * @param exception The exception containing the raw MongoDB bulk-write reply.
 * @return true if the reply contains only duplicate-key write errors, false otherwise.
 */
[[nodiscard]] auto contains_only_duplicate_key_write_errors(
        mongocxx::bulk_write_exception const& exception
) -> bool {
    auto const& raw_server_error = exception.raw_server_error();
    if (false == raw_server_error.has_value()) {
        return false;
    }

    auto const reply = raw_server_error->view();
    if (false == is_successful_command_reply(reply) || has_write_concern_errors(reply)) {
        return false;
    }

    auto const write_errors_element = reply["writeErrors"];
    if (false == static_cast<bool>(write_errors_element)
        || bsoncxx::type::k_array != write_errors_element.type())
    {
        return false;
    }

    auto const write_errors = write_errors_element.get_array().value;
    if (write_errors.begin() == write_errors.end()) {
        return false;
    }
    return std::all_of(write_errors.begin(), write_errors.end(), is_duplicate_key_write_error);
}
}  // namespace

void FileOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    static constexpr string_view cOrigFilePathPlaceholder{""};
    msgpack::type::tuple<epochtime_t, string, string, string, int64_t> const
            src(timestamp, message, cOrigFilePathPlaceholder, archive_id, log_event_idx);
    msgpack::pack(m_file_writer, src);
}

NetworkOutputHandler::NetworkOutputHandler(
        string const& host,
        int port,
        bool should_output_timestamp
)
        : ::clp_s::search::OutputHandler(should_output_timestamp, true) {
    m_socket_fd = clp::networking::connect_to_server(host, std::to_string(port));
    if (-1 == m_socket_fd) {
        SPDLOG_ERROR("Failed to connect to the server, errno={}", errno);
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILENAME__, __LINE__);
    }
}

void NetworkOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    static constexpr string_view cOrigFilePathPlaceholder{""};
    msgpack::type::tuple<epochtime_t, string, string, string, int64_t> const
            src(timestamp, message, cOrigFilePathPlaceholder, archive_id, log_event_idx);
    msgpack::sbuffer m;
    msgpack::pack(m, src);

    if (-1 == send(m_socket_fd, m.data(), m.size(), 0)) {
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILENAME__, __LINE__);
    }
}

ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        string_view uri,
        string_view collection,
        uint64_t batch_size,
        uint64_t max_num_results,
        string_view dataset,
        bool should_output_timestamp
)
        : ::clp_s::search::OutputHandler{should_output_timestamp, true},
          m_batch_size{batch_size},
          m_max_num_results{max_num_results},
          m_dataset{dataset} {
    m_collection = connect_to_results_cache(uri, collection, m_client);
    m_results.reserve(m_batch_size);
}

ErrorCode ResultsCacheOutputHandler::finish() {
    size_t count = 0;
    while (false == m_latest_results.empty()) {
        auto result = std::move(*m_latest_results.top());
        m_latest_results.pop();

        try {
            m_results.emplace_back(
                    std::move(
                            bsoncxx::builder::basic::make_document(
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cId,
                                            bsoncxx::builder::basic::make_document(
                                                    bsoncxx::builder::basic::kvp(
                                                            constants::results_cache::search::
                                                                    cArchiveId,
                                                            result.archive_id
                                                    ),
                                                    bsoncxx::builder::basic::kvp(
                                                            constants::results_cache::search::
                                                                    cLogEventIdx,
                                                            result.log_event_idx
                                                    )
                                            )
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cOrigFilePath,
                                            std::move(result.original_path)
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cMessage,
                                            std::move(result.message)
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cTimestamp,
                                            result.timestamp
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            std::string{constants::results_cache::search::cDataset},
                                            std::move(result.dataset)
                                    )
                            )
                    )
            );
            count++;

            if (count == m_batch_size) {
                if (false == insert_results()) {
                    return ErrorCode::ErrorCodeFailureDbBulkWrite;
                }
                count = 0;
            }
        } catch (mongocxx::exception const& e) {
            SPDLOG_ERROR("Failed to build or insert search results - {}", e.what());
            return ErrorCode::ErrorCodeFailureDbBulkWrite;
        }
    }

    try {
        if (false == m_results.empty()) {
            if (false == insert_results()) {
                return ErrorCode::ErrorCodeFailureDbBulkWrite;
            }
        }
    } catch (mongocxx::exception const& e) {
        SPDLOG_ERROR("Failed to insert final search-results batch - {}", e.what());
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}

void ResultsCacheOutputHandler::write(
        string_view message,
        epochtime_t timestamp,
        string_view archive_id,
        int64_t log_event_idx
) {
    if (m_latest_results.size() < m_max_num_results) {
        m_latest_results.emplace(
                std::make_unique<QueryResult>(
                        string_view{},
                        message,
                        timestamp,
                        archive_id,
                        log_event_idx,
                        m_dataset
                )
        );
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(
                std::make_unique<QueryResult>(
                        string_view{},
                        message,
                        timestamp,
                        archive_id,
                        log_event_idx,
                        m_dataset
                )
        );
    }
}

auto ResultsCacheOutputHandler::insert_results() -> bool {
    try {
        mongocxx::options::insert options;
        options.ordered(false);
        m_collection.insert_many(m_results, options);
    } catch (mongocxx::bulk_write_exception const& exception) {
        if (false == contains_only_duplicate_key_write_errors(exception)) {
            SPDLOG_ERROR("Failed to insert search results - {}", exception.what());
            return false;
        }
    } catch (mongocxx::exception const& exception) {
        SPDLOG_ERROR("Failed to insert search results - {}", exception.what());
        return false;
    }
    m_results.clear();
    return true;
}

CountReducerOutputHandler::CountReducerOutputHandler(int reducer_socket_fd)
        : search::OutputHandler(false, false),
          m_reducer_socket_fd(reducer_socket_fd),
          m_pipeline(reducer::PipelineInputMode::InterStage) {
    m_pipeline.add_pipeline_stage(std::make_shared<reducer::CountOperator>());
}

auto CountReducerOutputHandler::write(string_view message) -> void {
    m_pipeline.push_record(reducer::EmptyRecord{});
}

auto CountReducerOutputHandler::finish() -> ErrorCode {
    if (false
        == reducer::send_pipeline_results(m_reducer_socket_fd, std::move(m_pipeline.finish())))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}

auto CountByTimeReducerOutputHandler::finish() -> ErrorCode {
    if (false
        == reducer::send_pipeline_results(
                m_reducer_socket_fd,
                std::make_unique<reducer::Int64Int64MapRecordGroupIterator>(
                        m_bucket_counts,
                        reducer::CountOperator::cRecordElementKey
                )
        ))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}
}  // namespace clp_s
