#ifndef CLP_S_SEARCH_OUTPUTHANDLER_HPP
#define CLP_S_SEARCH_OUTPUTHANDLER_HPP

#include <string>

#include <mongocxx/client.hpp>
#include <mongocxx/collection.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

#include "../Defs.hpp"
#include "../TraceableException.hpp"

namespace clp_s::search {
/**
 * Abstract class for handling output from the search command.
 */
class OutputHandler {
public:
    // Constructors
    explicit OutputHandler() = default;

    // Destructor
    virtual ~OutputHandler() = default;

    // Methods
    /**
     * Writes a log event to the output handler.
     * @param message The message in the log event.
     * @param timestamp The timestamp of the log event.
     */
    virtual void write(std::string const& message, epochtime_t timestamp) = 0;

    /**
     * Writes a message to the output handler.
     * @param message The message to write.
     */
    virtual void write(std::string const& message) = 0;

    /**
     * Flushes the output handler.
     */
    virtual void flush() = 0;
};

/**
 * Output handler that writes to standard output.
 */
class StandardOutputHandler : public OutputHandler {
public:
    // Constructors
    explicit StandardOutputHandler() = default;

    // Methods inherited from OutputHandler
    void write(std::string const& message, epochtime_t timestamp) override {
        printf("%" EPOCHTIME_T_PRINTF_FMT " %s", timestamp, message.c_str());
    }

    void write(std::string const& message) override { printf("%s", message.c_str()); }

    void flush() override {}
};

/**
 * Output handler that writes to a MongoDB collection.
 */
class ResultsCacheOutputHandler : public OutputHandler {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructor
    ResultsCacheOutputHandler(
            std::string const& uri,
            std::string const& collection,
            uint64_t batch_size
    );

    // Methods inherited from OutputHandler
    void flush() override;

    void write(std::string const& message, epochtime_t timestamp) override;

    void write(std::string const& message) override { write(message, 0); }

private:
    mongocxx::client m_client;
    mongocxx::collection m_collection;
    std::vector<bsoncxx::document::value> m_results;
    uint64_t m_batch_size;
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUTHANDLER_HPP
