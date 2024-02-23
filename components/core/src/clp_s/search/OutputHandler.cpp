#include "OutputHandler.hpp"

#include <sstream>

#include <spdlog/spdlog.h>

namespace clp_s::search {
NetworkOutputHandler::NetworkOutputHandler(
        std::string const& host,
        std::string const& port,
        bool should_output_timestamp
)
        : OutputHandler(should_output_timestamp) {
    struct addrinfo hints = {};
    // Address can be IPv4 or IPV6
    hints.ai_family = AF_UNSPEC;
    // TCP socket
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = 0;
    hints.ai_protocol = 0;
    struct addrinfo* addresses_head = nullptr;
    int error = getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses_head);
    if (0 != error) {
        SPDLOG_ERROR("Failed to get address information for the server, error={}", error);
        throw OperationFailed(ErrorCode::ErrorCodeFailureNetwork, __FILE__, __LINE__);
    }

    // Try each address until a socket can be created and connected to
    for (auto curr = addresses_head; nullptr != curr; curr = curr->ai_next) {
        // Create socket
        m_socket_fd = socket(curr->ai_family, curr->ai_socktype, curr->ai_protocol);
        if (-1 == m_socket_fd) {
            continue;
        }

        // Connect to address
        if (connect(m_socket_fd, curr->ai_addr, curr->ai_addrlen) != -1) {
            break;
        }

        // Failed to connect, so close socket
        close(m_socket_fd);
        m_socket_fd = -1;
    }
    freeaddrinfo(addresses_head);
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
        : OutputHandler(should_output_timestamp),
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
}  // namespace clp_s::search
