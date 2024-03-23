#include "OutputHandler.hpp"

#include <sstream>

#include <spdlog/spdlog.h>

#include "../../clp/networking/socket_utils.hpp"
#include "../../reducer/CountOperator.hpp"
#include "../../reducer/network_utils.hpp"
#include "../../reducer/Record.hpp"

namespace clp_s::search {
NetworkOutputHandler::NetworkOutputHandler(
        std::string const& host,
        std::string const& port,
        bool should_output_timestamp
)
        : OutputHandler(should_output_timestamp, true) {
    m_socket_fd = clp::networking::connect_to_server(host, port);
    if (-1 == m_socket_fd) {
        SPDLOG_ERROR("Failed to connect to the server, errno={}", errno);
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILE__, __LINE__);
    }
}

ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        std::string const& uri,
        std::string const& collection,
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

void ResultsCacheOutputHandler::flush() {
    size_t count = 0;
    while (false == m_latest_results.empty()) {
        auto result = std::move(*m_latest_results.top());
        m_latest_results.pop();

        try {
            m_results.emplace_back(std::move(bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp("original_path", std::move(result.original_path)),
                    bsoncxx::builder::basic::kvp("message", std::move(result.message)),
                    bsoncxx::builder::basic::kvp("timestamp", result.timestamp)
            )));
            count++;

            if (count == m_batch_size) {
                m_collection.insert_many(m_results);
                m_results.clear();
                count = 0;
            }
        } catch (mongocxx::exception const& e) {
            throw OperationFailed(ErrorCode::ErrorCodeFailureDbBulkWrite, __FILE__, __LINE__);
        }
    }

    try {
        if (false == m_results.empty()) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode::ErrorCodeFailureDbBulkWrite, __FILE__, __LINE__);
    }
}

void ResultsCacheOutputHandler::write(std::string const& message, epochtime_t timestamp) {
    if (m_latest_results.size() < m_max_num_results) {
        m_latest_results.emplace(std::make_unique<QueryResult>("", message, timestamp));
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(std::make_unique<QueryResult>("", message, timestamp));
    }
}

CountOutputHandler::CountOutputHandler(int reducer_socket_fd)
        : OutputHandler(false, false),
          m_reducer_socket_fd(reducer_socket_fd),
          m_pipeline(reducer::PipelineInputMode::InterStage) {
    m_pipeline.add_pipeline_stage(std::make_shared<reducer::CountOperator>());
}

void CountOutputHandler::write(std::string const& message) {
    m_pipeline.push_record(reducer::EmptyRecord{});
}

void CountOutputHandler::finish() {
    if (false
        == reducer::send_pipeline_results(m_reducer_socket_fd, std::move(m_pipeline.finish())))
    {
        SPDLOG_ERROR("Failed to send aggregated results to reducer");
    }
}

void CountByTimeOutputHandler::finish() {
    if (false
        == reducer::send_pipeline_results(
                m_reducer_socket_fd,
                std::make_unique<reducer::Int64Int64MapRecordGroupIterator>(
                        m_bucket_counts,
                        "count"
                )
        ))
    {
        SPDLOG_ERROR("Failed to send results to reducer");
    }
}

}  // namespace clp_s::search
