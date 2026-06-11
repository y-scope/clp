#include "OutputHandlerImpl.hpp"

#include <sstream>
#include <string>
#include <string_view>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/model/update_one.hpp>
#include <mongocxx/options/update.hpp>
#include <mongocxx/uri.hpp>
#include <msgpack.hpp>
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
// The two count-writing paths (direct-to-results-cache and via the reducer) must use the same
// field name for the count value. NOTE: only the field name is shared. Count-by-time documents
// ({timestamp, count}) have the same shape on both paths, but for plain count the reducer writes
// a nested record-group document ({group_tags, records: [{count}]}) while
// CountToResultsCacheOutputHandler writes a flat {count} document, so consumers of the two paths
// differ.
static_assert(
        string_view{constants::results_cache::search::cCount}
        == string_view{reducer::CountOperator::cRecordElementKey}
);

namespace {
/**
 * Connects to the results cache and returns the requested collection.
 * @tparam OperationFailedT The handler-specific exception type to throw on failure.
 * @param uri
 * @param collection
 * @param client Returns the connected client, which must outlive the returned collection.
 * @return The collection.
 */
template <typename OperationFailedT>
auto connect_to_results_cache(string const& uri, string const& collection, mongocxx::client& client)
        -> mongocxx::collection {
    try {
        auto mongo_uri = mongocxx::uri(uri);
        client = mongocxx::client(mongo_uri);
        return client[mongo_uri.database()][collection];
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
        string const& uri,
        string const& collection,
        uint64_t batch_size,
        uint64_t max_num_results,
        string dataset,
        bool should_output_timestamp
)
        : ::clp_s::search::OutputHandler{should_output_timestamp, true},
          m_batch_size{batch_size},
          m_max_num_results{max_num_results},
          m_dataset{std::move(dataset)} {
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

CountOutputHandler::CountOutputHandler(int reducer_socket_fd)
        : ::clp_s::search::OutputHandler(false, false),
          m_reducer_socket_fd(reducer_socket_fd),
          m_pipeline(reducer::PipelineInputMode::InterStage) {
    m_pipeline.add_pipeline_stage(std::make_shared<reducer::CountOperator>());
}

void CountOutputHandler::write(string_view message) {
    m_pipeline.push_record(reducer::EmptyRecord{});
}

ErrorCode CountOutputHandler::finish() {
    if (false
        == reducer::send_pipeline_results(m_reducer_socket_fd, std::move(m_pipeline.finish())))
    {
        return ErrorCode::ErrorCodeFailureNetwork;
    }
    return ErrorCode::ErrorCodeSuccess;
}

ErrorCode CountByTimeOutputHandler::finish() {
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

CountToResultsCacheOutputHandler::CountToResultsCacheOutputHandler(
        string const& uri,
        string const& collection
)
        : ::clp_s::search::OutputHandler(false, false) {
    m_collection = connect_to_results_cache<OperationFailed>(uri, collection, m_client);
}

ErrorCode CountToResultsCacheOutputHandler::finish() {
    if (0 == m_count) {
        return ErrorCode::ErrorCodeSuccess;
    }

    // Filtering on _id makes the upsert safe under concurrent writers: _id's built-in unique
    // index guarantees that racing upserts resolve to a single document.
    try {
        auto const filter = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp(
                        constants::results_cache::cDocId,
                        constants::results_cache::search::cCount
                )
        );
        auto const inc_count = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp(constants::results_cache::search::cCount, m_count)
        );
        auto const update = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("$inc", inc_count)
        );
        m_collection
                .update_one(filter.view(), update.view(), mongocxx::options::update{}.upsert(true));
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}

CountByTimeToResultsCacheOutputHandler::CountByTimeToResultsCacheOutputHandler(
        string const& uri,
        string const& collection,
        int64_t count_by_time_bucket_size
)
        : ::clp_s::search::OutputHandler(true, false),
          m_count_by_time_bucket_size(count_by_time_bucket_size) {
    m_collection = connect_to_results_cache<OperationFailed>(uri, collection, m_client);
}

ErrorCode CountByTimeToResultsCacheOutputHandler::finish() {
    if (m_bucket_counts.empty()) {
        return ErrorCode::ErrorCodeSuccess;
    }

    // Each bucket document is keyed by its bucket timestamp via _id, whose built-in unique index
    // makes concurrent upserts from multiple writers resolve to a single document per bucket. The
    // timestamp is duplicated into a regular field on first insert so that readers don't need to
    // inspect _id.
    try {
        auto bulk_write = m_collection.create_bulk_write();
        for (auto const& [bucket_timestamp, count] : m_bucket_counts) {
            auto filter = bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp(constants::results_cache::cDocId, bucket_timestamp)
            );
            auto const inc_count = bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp(constants::results_cache::search::cCount, count)
            );
            auto const set_timestamp_on_insert = bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp(
                            constants::results_cache::search::cTimestamp,
                            bucket_timestamp
                    )
            );
            auto update = bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp("$inc", inc_count),
                    bsoncxx::builder::basic::kvp("$setOnInsert", set_timestamp_on_insert)
            );

            // The documents are moved into the operation so that they outlive this loop
            // iteration.
            mongocxx::model::update_one update_op{std::move(filter), std::move(update)};
            update_op.upsert(true);
            bulk_write.append(update_op);
        }
        bulk_write.execute();
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCodeFailureDbBulkWrite;
    }
    return ErrorCode::ErrorCodeSuccess;
}
}  // namespace clp_s
