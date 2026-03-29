#ifndef CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP

#include <memory>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "LogEvent.hpp"

namespace clp_s {
// Forward include
class ArchiveReader;
class SchemaReader;
}  // namespace clp_s

namespace clp_s::search {
class QueryRunner;
class SchemaMatch;
}  // namespace clp_s::search

namespace clp_s::ffi::sfa {
class KqlQuery;

/**
 * Decoder for iterating decoded log events from an already initialized single-file archive
 * reader.
 */
class ClpArchiveDecoder {
public:
    // Factory functions
    /**
     * Creates an archive decoder and preloads all schema tables from the archive reader.
     *
     * @param reader Already initialized archive reader.
     * @return A result containing the newly constructed `ClpArchiveDecoder` on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::Failure` if table loading fails.
     * - `SfaErrorCodeEnum::NoMemory` if allocating decoder state fails.
     */
    [[nodiscard]] static auto create(clp_s::ArchiveReader& reader)
            -> ystdlib::error_handling::Result<ClpArchiveDecoder>;

    /**
     * Creates a filtered archive decoder and preloads all schema tables from the archive reader.
     *
     * @param reader Already initialized archive reader.
     * @param query Parsed KQL query used for archive filtering.
     * @return A result containing the newly constructed `ClpArchiveDecoder` on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::Failure` if query preparation or table loading fails.
     * - `SfaErrorCodeEnum::NoMemory` if allocating decoder state fails.
     */
    [[nodiscard]] static auto create(
            clp_s::ArchiveReader& reader,
            std::shared_ptr<clp_s::search::SchemaMatch> schema_match,
            KqlQuery const& query
    ) -> ystdlib::error_handling::Result<ClpArchiveDecoder>;

    // Destructor
    ~ClpArchiveDecoder();

    // Delete copy constructor and assignment operator
    ClpArchiveDecoder(ClpArchiveDecoder const&) = delete;
    auto operator=(ClpArchiveDecoder const&) -> ClpArchiveDecoder& = delete;

    ClpArchiveDecoder(ClpArchiveDecoder&&) noexcept;
    [[nodiscard]] auto operator=(ClpArchiveDecoder&&) noexcept -> ClpArchiveDecoder&;

    /**
     * Decodes all log events from the preloaded schema tables and caches them in memory.
     *
     * Repeated calls are no-ops once all log events have been decoded.
     *
     * @return A void result on success, or an error code indicating the failure:
     * - `SfaErrorCodeEnum::Failure` if decoding fails.
     * - `SfaErrorCodeEnum::NoMemory` if allocating decoded event storage fails.
     */
    [[nodiscard]] auto decode_all() -> ystdlib::error_handling::Result<void>;

    /**
     * @return The cached decoded log events.
     */
    [[nodiscard]] auto get_log_events() const -> std::vector<LogEvent> const& {
        return m_log_events;
    }

private:
    // Constructors
    explicit ClpArchiveDecoder(
            std::vector<std::shared_ptr<clp_s::SchemaReader>>&& tables,
            std::unique_ptr<clp_s::search::QueryRunner> query_runner = nullptr
    );

    // Methods
    /**
     * Cleans up underlying resources.
     */
    auto close() noexcept -> void;

    /**
     * Moves owned state from rhs into this object and resets moved-from state.
     *
     * @param rhs Source reader to move from.
     */
    auto move_from(ClpArchiveDecoder& rhs) noexcept -> void;

    // Members
    std::vector<std::shared_ptr<clp_s::SchemaReader>> m_tables;
    std::vector<LogEvent> m_log_events;
    std::unique_ptr<clp_s::search::QueryRunner> m_query_runner;
    bool m_decode_started{false};
    bool m_decode_completed{false};
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP
