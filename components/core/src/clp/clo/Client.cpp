#include "Client.hpp"

#include <vector>

#include <msgpack.hpp>
#include <spdlog/spdlog.h>

#include "../networking/socket_utils.hpp"

namespace clp::clo {

NetworkClient::NetworkClient(std::string const& host, std::string const& port) {
    m_socket_fd = clp::networking::connect_to_server(host, port);
    if (-1 == m_socket_fd) {
        SPDLOG_ERROR("Failed to connect to the server");
        throw OperationFailed(ErrorCode_Failure_Network, __FILE__, __LINE__);
    }
}

ErrorCode NetworkClient::add_result(
        std::string const& original_path,
        std::string const& message,
        epochtime_t timestamp
) {
    msgpack::type::tuple<std::string, epochtime_t, std::string> src(
            original_path,
            timestamp,
            message
    );
    msgpack::sbuffer m;
    msgpack::pack(m, src);
    return networking::try_send(m_socket_fd, m.data(), m.size());
}

ResultsCacheClient::ResultsCacheClient(
        std::string const& uri,
        std::string const& collection,
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

void ResultsCacheClient::flush() {
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
            throw OperationFailed(ErrorCode_Failure_DB_Bulk_Write, __FILE__, __LINE__);
        }
    }

    try {
        if (false == m_results.empty()) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode_Failure_DB_Bulk_Write, __FILE__, __LINE__);
    }
}

ErrorCode ResultsCacheClient::add_result(
        std::string const& original_path,
        std::string const& message,
        epochtime_t timestamp
) {
    if (m_latest_results.size() < m_max_num_results) {
        m_latest_results.emplace(std::make_unique<QueryResult>(original_path, message, timestamp));
    } else if (m_latest_results.top()->timestamp < timestamp) {
        m_latest_results.pop();
        m_latest_results.emplace(std::make_unique<QueryResult>(original_path, message, timestamp));
    }

    return ErrorCode_Success;
}

}  // namespace clp::clo
