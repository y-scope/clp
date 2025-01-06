#include "OutputHandler.hpp"

#include <memory>
#include <string>
#include <string_view>

#include <msgpack.hpp>
#include <spdlog/spdlog.h>

#include "../../reducer/CountOperator.hpp"
#include "../../reducer/network_utils.hpp"
#include "../networking/socket_utils.hpp"
#include "constants.hpp"

using clp::streaming_archive::reader::Message;
using std::string;
using std::string_view;

namespace clp::clo {
NetworkOutputHandler::NetworkOutputHandler(string const& host, int port) {
    m_socket_fd = clp::networking::connect_to_server(host, std::to_string(port));
    if (-1 == m_socket_fd) {
        SPDLOG_ERROR("Failed to connect to the server");
        throw OperationFailed(ErrorCode_Failure_Network, __FILE__, __LINE__);
    }
}

ErrorCode NetworkOutputHandler::add_result(
        string_view orig_file_path,
        string_view orig_file_id,
        Message const& encoded_message,
        string_view decompressed_message
) {
    msgpack::type::tuple<epochtime_t, string, string, string, uint64_t> src(
            encoded_message.get_ts_in_milli(),
            decompressed_message,
            orig_file_path,
            orig_file_id,
            encoded_message.get_log_event_ix()
    );
    msgpack::sbuffer m;
    msgpack::pack(m, src);
    return networking::try_send(m_socket_fd, m.data(), m.size());
}

ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        string const& uri,
        string const& collection,
        uint64_t batch_size,
        uint64_t max_num_results
)
        : m_batch_size(batch_size),
          m_max_num_results(max_num_results) {
    try {
        auto mongo_uri = mongocxx::uri(uri);
        m_client = mongocxx::client(mongo_uri);
        m_collection = m_client[mongo_uri.database()][collection];
        m_results.reserve(m_batch_size);
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode_BadParam_DB_URI, __FILE__, __LINE__);
    }
}

ErrorCode ResultsCacheOutputHandler::add_result(
        string_view orig_file_path,
        string_view orig_file_id,
        Message const& encoded_message,
        string_view decompressed_message
) {
    auto const timestamp = encoded_message.get_ts_in_milli();
    auto const log_event_ix = encoded_message.get_log_event_ix();
    if (m_latest_results.size() < m_max_num_results) {
        m_latest_results.emplace(std::make_unique<QueryResult>(
                orig_file_path,
                orig_file_id,
                log_event_ix,
                timestamp,
                decompressed_message
        ));
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(std::make_unique<QueryResult>(
                orig_file_path,
                orig_file_id,
                log_event_ix,
                timestamp,
                decompressed_message
        ));
    }

    return ErrorCode_Success;
}

ErrorCode ResultsCacheOutputHandler::flush() {
    size_t count = 0;
    while (false == m_latest_results.empty()) {
        auto result = std::move(*m_latest_results.top());
        m_latest_results.pop();

        try {
            m_results.emplace_back(std::move(bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp(
                            cResultsCacheKeys::SearchOutput::OrigFileId,
                            std::move(result.orig_file_id)
                    ),
                    bsoncxx::builder::basic::kvp(
                            cResultsCacheKeys::SearchOutput::OrigFilePath,
                            std::move(result.orig_file_path)
                    ),
                    bsoncxx::builder::basic::kvp(
                            cResultsCacheKeys::SearchOutput::LogEventIx,
                            result.log_event_ix
                    ),
                    bsoncxx::builder::basic::kvp(
                            cResultsCacheKeys::SearchOutput::Timestamp,
                            result.timestamp
                    ),
                    bsoncxx::builder::basic::kvp(
                            cResultsCacheKeys::SearchOutput::Message,
                            std::move(result.decompressed_message)
                    )
            )));
            count++;

            if (count == m_batch_size) {
                m_collection.insert_many(m_results);
                m_results.clear();
                count = 0;
            }
        } catch (mongocxx::exception const& e) {
            return ErrorCode::ErrorCode_Failure_DB_Bulk_Write;
        }
    }

    try {
        if (false == m_results.empty()) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
        return ErrorCode::ErrorCode_Failure_DB_Bulk_Write;
    }
    return ErrorCode::ErrorCode_Success;
}

CountOutputHandler::CountOutputHandler(int reducer_socket_fd)
        : m_reducer_socket_fd{reducer_socket_fd},
          m_pipeline{reducer::PipelineInputMode::InterStage} {
    m_pipeline.add_pipeline_stage(std::make_shared<reducer::CountOperator>());
}

ErrorCode CountOutputHandler::add_result(
        [[maybe_unused]] string_view orig_file_path,
        [[maybe_unused]] string_view orig_file_id,
        [[maybe_unused]] Message const& encoded_message,
        [[maybe_unused]] string_view decompressed_message
) {
    m_pipeline.push_record(reducer::EmptyRecord{});
    return ErrorCode_Success;
}

ErrorCode CountOutputHandler::flush() {
    if (false
        == reducer::send_pipeline_results(m_reducer_socket_fd, std::move(m_pipeline.finish())))
    {
        return ErrorCode::ErrorCode_Failure_Network;
    }
    return ErrorCode::ErrorCode_Success;
}

ErrorCode CountByTimeOutputHandler::flush() {
    if (false
        == reducer::send_pipeline_results(
                m_reducer_socket_fd,
                std::make_unique<reducer::Int64Int64MapRecordGroupIterator>(
                        m_bucket_counts,
                        reducer::CountOperator::cRecordElementKey
                )
        ))
    {
        return ErrorCode::ErrorCode_Failure_Network;
    }
    return ErrorCode::ErrorCode_Success;
}
}  // namespace clp::clo
