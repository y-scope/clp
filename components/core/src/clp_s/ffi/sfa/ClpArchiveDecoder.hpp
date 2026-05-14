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
 * Decoder that iterates over decoded log events from a `ClpArchiveReader`.
 */
class ClpArchiveDecoder {
public:
    /**
     * Creates a decoder from an initialized archive reader and preloads its schema tables.
     *
     * @param reader Initialized archive reader to decode log events from.
     * @return A result containing the constructed `ClpArchiveDecoder` on success, or an error code
     * if initialization fails:
     * - `SfaErrorCodeEnum::Failure` if schema table loading fails.
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
     * Decodes and returns the next log event.
     *
     * @return A result containing the next decoded `LogEvent`, or `std::nullopt` if all log events
     * have already been consumed.
     */
    [[nodiscard]] auto get_next_log_event()
            -> ystdlib::error_handling::Result<std::optional<LogEvent>>;

    /**
     * Decodes all remaining log events and caches them in memory.
     *
     * Subsequent calls return the same cached decoded events without re decoding them.
     *
     * @return A result containing a span over all decoded log events, or an error code if decoding
     * fails:
     * - `SfaErrorCodeEnum::Failure` if decoding fails.
     * - `SfaErrorCodeEnum::NoMemory` if memory allocation during decoding fails.
     */
    [[nodiscard]] auto collect_log_events()
            -> ystdlib::error_handling::Result<std::span<LogEvent const>>;

private:
    // Constructor
    /**
     * Constructs a decoder from preloaded schema tables.
     *
     * @param tables Schema readers used to decode log events.
     * @param has_log_order Whether log event ordering metadata is available.
     */
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
     * Moves owned state from `rhs` into this object and resets `rhs`.
     *
     * @param rhs Object to move state from.
     */
    auto move_from(ClpArchiveDecoder& rhs) noexcept -> void;

    /**
     * Decodes the next log event from `table` and appends it to the internal cache.
     *
     * @param table Schema reader to decode from.
     * @return `true` if a log event was decoded and appended, or `false` if the table has no more
     * log events.
     */
    [[nodiscard]] auto append_next_log_event(clp_s::SchemaReader& table) -> bool;

    /**
     * Decodes the next available log event without enforcing global log event order.
     *
     * @return `true` if a log event was decoded, or `false` if no more log events remain.
     */
    [[nodiscard]] auto decode_next_log_event() -> bool;

    /**
     * Decodes the next available log event in ascending log event index order.
     *
     * @return `true` if a log event was decoded, or `false` if no more log events remain.
     */
    [[nodiscard]] auto decode_next_log_event_in_order() -> bool;

    // Members
    std::vector<std::shared_ptr<clp_s::SchemaReader>> m_tables;
    bool m_has_log_order{false};
    std::vector<LogEvent> m_log_events;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEDECODER_HPP
