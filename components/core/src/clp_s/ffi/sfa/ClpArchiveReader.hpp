#ifndef CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

namespace clp_s {
// Forward include
class ArchiveReader;
}  // namespace clp_s

namespace clp_s::ffi::sfa {
/**
 * Metadata describing a single source file's event-index range within a single-file archive.
 */
class FileInfo {
public:
    // Constructor
    FileInfo(std::string_view file_name, uint64_t start_index, uint64_t end_index)
            : m_file_name{file_name},
              m_start_index{start_index},
              m_end_index{end_index} {}

    // Methods
    [[nodiscard]] auto get_file_name() const -> std::string const& { return m_file_name; }

    [[nodiscard]] auto get_start_index() const -> uint64_t { return m_start_index; }

    [[nodiscard]] auto get_end_index() const -> uint64_t { return m_end_index; }

    [[nodiscard]] auto get_event_count() const -> uint64_t { return m_end_index - m_start_index; }

private:
    // Members
    std::string m_file_name;
    uint64_t m_start_index{0};
    uint64_t m_end_index{0};
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
    [[nodiscard]] auto get_file_infos() const -> std::vector<FileInfo> {
        return m_file_infos;
    }

private:
    // Constructors
    explicit ClpArchiveReader(
            std::unique_ptr<clp_s::ArchiveReader> reader,
            std::shared_ptr<std::vector<char>> archive_data
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
     * Precomputes archive metadata from the range index.
     *
     * Assumes range-index entries are ordered and globally contiguous in log-event index space,
     * i.e., each entry starts at the previous entry's end.
     *
     * @return A void result on success, or an error code indicating the failure:
     * - `SfaErrorCodeEnum::MalformedRangeIndex` if range-index metadata violates the assumption.
     */
    [[nodiscard]] auto precompute_archive_metadata() -> ystdlib::error_handling::Result<void>;

    // Members
    std::unique_ptr<clp_s::ArchiveReader> m_archive_reader;
    std::shared_ptr<std::vector<char>> m_archive_data;
    uint64_t m_event_count{0};
    std::vector<std::string> m_file_names;
    std::vector<FileInfo> m_file_infos;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
