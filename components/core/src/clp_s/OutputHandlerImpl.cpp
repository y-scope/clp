#include "OutputHandlerImpl.hpp"

#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <msgpack.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

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
/**
 * Connects to the results cache and returns the requested collection.
 * @tparam OperationFailedT The type of exception to throw on failure.
 * @param uri
 * @param collection
 * @param client Returns the connected client.
 * @return The collection.
 */
template <typename OperationFailedT>
auto connect_to_results_cache(string_view uri, string_view collection, mongocxx::client& client)
        -> mongocxx::collection;

template <typename OperationFailedT>
auto connect_to_results_cache(string_view uri, string_view collection, mongocxx::client& client)
        -> mongocxx::collection {
    try {
        auto mongo_uri = mongocxx::uri{string{uri}};
        client = mongocxx::client(mongo_uri);
        return client[mongo_uri.database()][string{collection}];
    } catch (mongocxx::exception const& e) {
        throw OperationFailedT(ErrorCode::ErrorCodeBadParamDbUri, __FILENAME__, __LINE__);
    }
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
    m_collection = connect_to_results_cache<OperationFailed>(uri, collection, m_client);
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
                                            constants::results_cache::search::cArchiveId,
                                            std::move(result.archive_id)
                                    ),
                                    bsoncxx::builder::basic::kvp(
                                            constants::results_cache::search::cLogEventIx,
                                            result.log_event_idx
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
                m_collection.insert_many(m_results);
                m_results.clear();
                count = 0;
            }
        } catch (mongocxx::exception const& e) {
            return ErrorCode::ErrorCodeFailureDbBulkWrite;
        }
    }

    try {
        if (false == m_results.empty()) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
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

auto StdoutSink::write(AggregationResult const& result) -> void {
    nlohmann::json document;
    document[constants::results_cache::search::cArchiveId] = m_archive_id;
    for (auto const& [key, value] : result) {
        std::visit([&](auto const& field_value) { document[key] = field_value; }, value);
    }
    std::cout << document.dump() << '\n';
}

ResultsCacheSink::ResultsCacheSink(string_view uri, string_view collection, string_view archive_id)
        : m_archive_id{archive_id} {
    m_collection = connect_to_results_cache<OperationFailed>(uri, collection, m_client);
}

auto ResultsCacheSink::write(AggregationResult const& result) -> void {
    bsoncxx::builder::basic::document document;
    document.append(
            bsoncxx::builder::basic::kvp(constants::results_cache::search::cArchiveId, m_archive_id)
    );
    for (auto const& [key, value] : result) {
        std::visit(
                [&](auto const& field_value) {
                    document.append(bsoncxx::builder::basic::kvp(key, field_value));
                },
                value
        );
    }
    m_results.push_back(document.extract());
}

auto ResultsCacheSink::finish() -> ErrorCode {
    if (m_results.empty()) {
        return ErrorCode::ErrorCodeSuccess;
    }

    try {
        m_collection.insert_many(m_results);
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}
}  // namespace clp_s
