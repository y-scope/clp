#include "OutputHandler.hpp"

namespace clp_s::search {
ResultsCacheOutputHandler::ResultsCacheOutputHandler(
        std::string const& uri,
        std::string const& collection,
        uint64_t batch_size
)
        : m_batch_size(batch_size) {
    try {
        auto mongo_uri = mongocxx::uri(uri);
        m_client = mongocxx::client(mongo_uri);
        m_collection = m_client[mongo_uri.database()][collection];
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode::ErrorCodeBadParamDbUri, __FILENAME__, __LINE__);
    }
}

void ResultsCacheOutputHandler::flush() {
    try {
        if (false == m_results.empty()) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode::ErrorCodeFailureDbBulkWrite, __FILENAME__, __LINE__);
    }
}

void ResultsCacheOutputHandler::write(std::string const& message, epochtime_t timestamp) {
    try {
        auto document = bsoncxx::builder::basic::make_document(
                bsoncxx::builder::basic::kvp("original_path", ""),
                bsoncxx::builder::basic::kvp("message", message),
                bsoncxx::builder::basic::kvp("timestamp", timestamp)
        );

        m_results.push_back(std::move(document));

        if (m_results.size() >= m_batch_size) {
            m_collection.insert_many(m_results);
            m_results.clear();
        }
    } catch (mongocxx::exception const& e) {
        throw OperationFailed(ErrorCode::ErrorCodeFailureDbBulkWrite, __FILENAME__, __LINE__);
    }
}
}  // namespace clp_s::search
