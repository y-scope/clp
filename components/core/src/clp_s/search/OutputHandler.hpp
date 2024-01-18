#ifndef CLP_S_SEARCH_OUTPUTHANDLER_HPP
#define CLP_S_SEARCH_OUTPUTHANDLER_HPP

#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "../Defs.hpp"

namespace clp_s::search {
class OutputHandler {
public:
    explicit OutputHandler(bool output_timestamp, bool decompress_full_log = true)
            : m_output_timestamp(output_timestamp) {}

    virtual ~OutputHandler() = default;

    virtual void write(std::string const& message, epochtime_t timestamp) = 0;
    virtual void write(std::string const& message) = 0;
    virtual void flush() = 0;

    bool output_timestamp() const { return m_output_timestamp; }

protected:
    bool m_output_timestamp;
};

class StandardOutputHandler : public OutputHandler {
public:
    explicit StandardOutputHandler(bool output_timestamp = false)
            : OutputHandler(output_timestamp) {}

    void write(std::string const& message, epochtime_t timestamp) override {
        printf("%lld %s", timestamp, message.c_str());
    }

    void flush() override {}

    void write(std::string const& message) override { printf("%s", message.c_str()); }
};

class ResultsCacheOutputHandler : public OutputHandler {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    ResultsCacheOutputHandler(
            std::string const& uri,
            std::string const& collection,
            uint64_t batch_size,
            bool output_timestamp = true
    )
            : OutputHandler(output_timestamp),
              m_batch_size(batch_size) {
        try {
            auto mongo_uri = mongocxx::uri(uri);
            m_client = mongocxx::client(mongo_uri);
            m_collection = m_client[mongo_uri.database()][collection];
        } catch (mongocxx::exception const& e) {
            throw OperationFailed(ErrorCode::ErrorCodeBadParamDbUri, __FILENAME__, __LINE__);
        }
    }

    void flush() override {
        try {
            if (!m_results.empty()) {
                m_collection.insert_many(m_results);
                m_results.clear();
            }
        } catch (mongocxx::exception const& e) {
            throw OperationFailed(ErrorCode::ErrorCodeFailureDbBulkWrite, __FILENAME__, __LINE__);
        }
    }

    void write(std::string const& message, epochtime_t timestamp) override {
        try {
            auto document = bsoncxx::builder::basic::make_document(
                    bsoncxx::builder::basic::kvp("path", "logs.ndjson"),
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

    void write(std::string const& message) override { write(message, 0); }

private:
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUTHANDLER_HPP
