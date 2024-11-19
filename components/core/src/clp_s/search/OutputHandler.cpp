#include "OutputHandler.hpp"

#include <sstream>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>

#include "../../clp/networking/socket_utils.hpp"
#include "../../reducer/CountOperator.hpp"
#include "../../reducer/network_utils.hpp"
#include "../../reducer/Record.hpp"
#include "../archive_constants.hpp"

using std::string;
using std::string_view;

namespace clp_s::search {
NetworkOutputHandler::NetworkOutputHandler(
        string const& host,
        int port,
        bool should_output_timestamp
)
        : OutputHandler(should_output_timestamp, true) {
    m_socket_fd = clp::networking::connect_to_server(host, std::to_string(port));
    if (-1 == m_socket_fd) {
        SPDLOG_ERROR("Failed to connect to the server, errno={}", errno);
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILE__, __LINE__);
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
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILE__, __LINE__);
    }
}

ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        string const& uri,
        string const& collection,
        uint64_t batch_size,
        uint64_t max_num_results,
        bool should_output_timestamp
)
        : OutputHandler(should_output_timestamp, true),
          m_batch_size(batch_size),
          m_max_num_results(max_num_results) {
    try {
        auto mongo_uri = mongocxx::uri(uri);
        m_client = mongocxx::client(mongo_uri);
        m_collection = m_client[mongo_uri.database()][collection];
        m_results.reserve(m_batch_size);
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode::ErrorCodeBadParamDbUri, __FILENAME__, __LINE__);
    }
}

ErrorCode ResultsCacheOutputHandler::flush() {
    size_t count = 0;
    while (false == m_latest_results.empty()) {
        auto result = std::move(*m_latest_results.top());
        m_latest_results.pop();

        try {
            m_results.emplace_back(std::move(bsoncxx::builder::basic::make_document(
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
                    )
            )));
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
        m_latest_results.emplace(std::make_unique<QueryResult>(
                string_view{},
                message,
                timestamp,
                archive_id,
                log_event_idx
        ));
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(std::make_unique<QueryResult>(
                string_view{},
                message,
                timestamp,
                archive_id,
                log_event_idx
        ));
    }
}

CountOutputHandler::CountOutputHandler(int reducer_socket_fd)
        : OutputHandler(false, false),
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

}  // namespace clp_s::search
