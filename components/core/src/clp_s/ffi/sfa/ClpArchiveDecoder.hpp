#ifndef CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP

#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "LogEvent.hpp"

namespace clp_s {
// Forward declaration
class SchemaReader;
}  // namespace clp_s

namespace clp_s::ffi::sfa {
class ClpArchiveReader;

/**
 * Decoder for iterating decoded log events from an `ClpArchiveReader`.
 */
class ClpArchiveDecoder {
public:
    /**
     * Creates an archive decoder and preloads all schema tables from the archive reader.
     *
     * @param reader Already initialized archive wrapper.
     * @return A result containing the newly constructed `ClpArchiveDecoder` on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::Failure` if table loading fails.
     * - `SfaErrorCodeEnum::NoMemory` if allocating decoder state fails.
     */
    [[nodiscard]] static auto create(ClpArchiveReader& reader)
            -> ystdlib::error_handling::Result<ClpArchiveDecoder>;

    // Destructor
    ~ClpArchiveDecoder() noexcept;

    // Delete copy constructor and assignment operator
    ClpArchiveDecoder(ClpArchiveDecoder const&) = delete;
    auto operator=(ClpArchiveDecoder const&) -> ClpArchiveDecoder& = delete;

    // Move constructor and assignment operator
    ClpArchiveDecoder(ClpArchiveDecoder&&) noexcept;
    auto operator=(ClpArchiveDecoder&&) noexcept -> ClpArchiveDecoder&;

    /**
     * Gets the next decoded log event.
     *
     * @return A result containing the next log event on success, or `std::nullopt` if all log
     * events have been consumed.
     */
    [[nodiscard]] auto get_next_log_event()
            -> ystdlib::error_handling::Result<std::optional<LogEvent>>;

    /**
     * Decodes all log events and caches them in memory.
     *
     * Repeated calls are no-ops once all log events have been decoded.
     *
     * @return A result containing all decoded log events on success, or an error code indicating
     * the failure:
     * - `SfaErrorCodeEnum::Failure` if decoding fails.
     * - `SfaErrorCodeEnum::NoMemory` if memory allocation during decoding fails.
     */
    [[nodiscard]] auto collect_log_events()
            -> ystdlib::error_handling::Result<std::span<LogEvent const>>;

private:
    // Constructor
    ClpArchiveDecoder(
            std::vector<std::shared_ptr<clp_s::SchemaReader>>&& tables,
            bool has_log_order
    )
            : m_tables{std::move(tables)},
              m_has_log_order{has_log_order} {}

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

    [[nodiscard]] auto append_next_log_event(clp_s::SchemaReader& table) -> bool;
    [[nodiscard]] auto decode_next_log_event() -> bool;
    [[nodiscard]] auto decode_next_log_event_in_order() -> bool;

    // Members
    std::vector<std::shared_ptr<clp_s::SchemaReader>> m_tables;
    bool m_has_log_order{false};
    std::vector<LogEvent> m_log_events;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP
