#ifndef CLP_S_SEARCH_OUTPUTHANDLER_HPP
#define CLP_S_SEARCH_OUTPUTHANDLER_HPP

#include <string_view>
#include <vector>

#include "../Defs.hpp"
#include "../ErrorCode.hpp"

namespace clp_s::search {
/**
 * Abstract class for handling search output.
 */
class OutputHandler {
public:
    // Constructors
    explicit OutputHandler(bool should_output_metadata, bool should_marshal_records)
            : m_should_output_metadata(should_output_metadata),
              m_should_marshal_records(should_marshal_records) {}

    // Destructor
    virtual ~OutputHandler() = default;

    // Methods
    /**
     * Writes a log event to the output handler.
     * @param message The message in the log event.
     * @param timestamp The timestamp of the log event.
     * @param archive_id The archive containing the log event.
     * @param log_event_idx The index of the log event within an archive.
     */
    virtual void write(
            std::string_view message,
            epochtime_t timestamp,
            std::string_view archive_id,
            int64_t log_event_idx
    ) = 0;

    /**
     * Writes a message to the output handler.
     * @param message The message to write.
     */
    virtual void write(std::string_view message) = 0;

    /**
     * Flushes the output handler after each table that gets searched.
     * @return ErrorCodeSuccess on success or relevant error code on error
     */
    [[nodiscard]] virtual auto flush() -> ErrorCode { return ErrorCode::ErrorCodeSuccess; }

    /**
     * Performs any final operations after all tables have been searched.
     * @return ErrorCodeSuccess on success or relevant error code on error
     */
    [[nodiscard]] virtual auto finish() -> ErrorCode { return ErrorCode::ErrorCodeSuccess; }

    [[nodiscard]] auto should_output_metadata() const -> bool { return m_should_output_metadata; }

    [[nodiscard]] auto should_marshal_records() const -> bool { return m_should_marshal_records; }

private:
    bool m_should_output_metadata{};
    bool m_should_marshal_records{};
};
}  // namespace clp_s::search

#endif  // CLP_S_SEARCH_OUTPUTHANDLER_HPP
