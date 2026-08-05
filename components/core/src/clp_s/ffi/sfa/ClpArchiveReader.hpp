#ifndef CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include "LogEvent.hpp"

namespace clp_s {
// Forward include
class ArchiveReader;
class SchemaReader;
}  // namespace clp_s

namespace clp_s::ffi::sfa {
using LogEventView = std::span<LogEvent const>;

/**
 * Metadata describing a single source file's event-index range within a single-file archive.
 */
class FileInfo {
public:
    // Constructor
    FileInfo(std::string_view file_name, int64_t start_index, int64_t end_index)
            : m_file_name{file_name},
              m_start_index{start_index},
              m_end_index{end_index} {}

    // Methods
    [[nodiscard]] auto get_file_name() const -> std::string const& { return m_file_name; }

    [[nodiscard]] auto get_start_index() const -> int64_t { return m_start_index; }

    [[nodiscard]] auto get_end_index() const -> int64_t { return m_end_index; }

    [[nodiscard]] auto get_event_count() const -> uint64_t {
        return static_cast<uint64_t>(m_end_index - m_start_index);
    }

private:
    // Members
    std::string m_file_name;
    int64_t m_start_index{0};
    int64_t m_end_index{0};
};

/**
 * A thin wrapper around `clp_s::ArchiveReader` for single file archive FFI entrypoints.
 */
class ClpArchiveReader {
public:
    // Factory functions
    /**
     * Creates an SFA reader from a filesystem archive path.
     *
     * @param archive_path Path to the single-file archive.
     * @return A result containing the newly constructed `ClpArchiveReader` on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::IoFailure` if archive open/initialization fails.
     * - `SfaErrorCodeEnum::NoMemory` if archive initialization fails due to OOM issues.
     * - Forwards `ClpArchiveReader::precompute_archive_metadata`'s return values on failure.
     */
    [[nodiscard]] static auto create(std::string_view archive_path)
            -> ystdlib::error_handling::Result<ClpArchiveReader>;

    /**
     * Creates an SFA reader from in memory archive bytes, taking ownership of the buffer.
     *
     * @param archive_data Bytes of a single-file archive.
     * @return A result containing the newly constructed `ClpArchiveReader` on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::IoFailure` if archive open/initialization fails.
     * - `SfaErrorCodeEnum::NoMemory` if allocating/copying archive bytes fails.
     * - Forwards `ClpArchiveReader::precompute_archive_metadata`'s return values on failure.
     */
    [[nodiscard]] static auto create(std::vector<char>&& archive_data)
            -> ystdlib::error_handling::Result<ClpArchiveReader>;

    // Destructor
    ~ClpArchiveReader() noexcept;

    // Delete copy constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader const&) = delete;
    auto operator=(ClpArchiveReader const&) -> ClpArchiveReader& = delete;

    ClpArchiveReader(ClpArchiveReader&&) noexcept;
    [[nodiscard]] auto operator=(ClpArchiveReader&&) noexcept -> ClpArchiveReader&;

    /**
     * @return The total number of events in the archive.
     */
    [[nodiscard]] auto get_event_count() const -> uint64_t { return m_event_count; }

    /**
     * @return Source file names in range-index order.
     */
    [[nodiscard]] auto get_file_names() const -> std::vector<std::string> { return m_file_names; }

    /**
     * @return Source file metadata in range index order.
     */
    [[nodiscard]] auto get_file_infos() const -> std::vector<FileInfo> { return m_file_infos; }

    /**
     * Decodes and caches all log events without returning them.
     *
     * Subsequent decode operations reuse the cached events. If decoding fails, the error is cached
     * and returned by all subsequent decode operations.
     *
     * @return A void result on success, or an error indicating the failure:
     * - `SfaErrorCodeEnum::IoFailure` if decoding fails due to archive read/decode errors.
     * - `SfaErrorCodeEnum::NoMemory` if decoding fails due to OOM issues.
     * - `SfaErrorCodeEnum::NotInit` if the reader is not initialized.
     */
    [[nodiscard]] auto decode() -> ystdlib::error_handling::Result<void>;

    /**
     * Decodes all log events in global log-event-index order.
     *
     * Results are cached after the first successful decode. The returned view remains valid until
     * this reader is closed, moved from, or destroyed.
     *
     * @return A result containing decoded log events on success, or an error indicating the
     * failure:
     * - `SfaErrorCodeEnum::IoFailure` if decoding fails due to archive read/decode errors.
     * - `SfaErrorCodeEnum::NoMemory` if decoding fails due to OOM issues.
     * - `SfaErrorCodeEnum::NotInit` if the reader is not initialized.
     */
    [[nodiscard]] auto decode_all() -> ystdlib::error_handling::Result<LogEventView>;

    /**
     * Decodes all log events, if necessary, and returns the requested half-open event range.
     *
     * @param begin_idx Index of the first event to return.
     * @param end_idx Index one past the final event to return.
     * @return A result containing a view of the requested decoded events on success, or an error
     * indicating the failure:
     * - `SfaErrorCodeEnum::DecodeRangeOutOfBounds` if the requested range is invalid.
     * - Forwards `decode`'s errors.
     */
    [[nodiscard]] auto decode_range(size_t begin_idx, size_t end_idx)
            -> ystdlib::error_handling::Result<LogEventView>;

    /**
     * Searches the archive using a KQL query.
     *
     * The returned indices are zero-based positions in the decoded log-event vector, rather than
     * the archive's global log-event indices. A new archive reader is used for each search so that
     * searching does not mutate the primary reader or its decoded-event cache.
     *
     * @param kql KQL query to evaluate.
     * @param ignore_case Whether string comparisons should ignore case.
     * @return A result containing matching decoded log-event indices on success, or an error:
     * - `SfaErrorCodeEnum::InvalidQuery` if `kql` cannot be parsed.
     * - `SfaErrorCodeEnum::LogEventIndexUnavailable` if the archive lacks log-order metadata or a
     *   search result cannot be mapped to the decoded-event cache.
     * - `SfaErrorCodeEnum::SearchFailure` if the search fails.
     * - Forwards `decode`'s errors.
     */
    [[nodiscard]] auto search(std::string_view kql, bool ignore_case)
            -> ystdlib::error_handling::Result<std::vector<size_t>>;

private:
    enum class DecodeState : uint8_t {
        NotStarted,
        Decoded,
        Failed,
    };

    // Constructors
    explicit ClpArchiveReader(
            std::unique_ptr<clp_s::ArchiveReader> reader,
            std::shared_ptr<std::vector<char>> archive_data,
            std::string archive_path
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
    auto move_from(ClpArchiveReader& rhs) noexcept -> void;

    /**
     * Populates the decoded-event cache.
     */
    [[nodiscard]] auto internal_decode_all() -> ystdlib::error_handling::Result<void>;

    /**
     * Precomputes archive metadata from the range index.
     *
     * This function skips range index validation as they are already validated inside
     * `clp_s::ArchiveReaderAdaptor`.
     *
     * @return A void result on success.
     */
    [[nodiscard]] auto precompute_archive_metadata() -> ystdlib::error_handling::Result<void>;

    // Members
    std::unique_ptr<clp_s::ArchiveReader> m_archive_reader;
    std::shared_ptr<std::vector<char>> m_archive_data;
    std::string m_archive_path;
    uint64_t m_event_count{0};
    std::vector<std::string> m_file_names;
    std::vector<FileInfo> m_file_infos;
    std::vector<std::shared_ptr<clp_s::SchemaReader>> m_tables;
    std::vector<LogEvent> m_log_events;
    DecodeState m_decode_state{DecodeState::NotStarted};
    std::error_code m_decode_error;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
