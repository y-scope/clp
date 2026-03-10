#ifndef CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/ArchiveReader.hpp>

namespace clp_s::ffi::sfa {
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
     */
    [[nodiscard]] static auto create(std::string_view archive_path)
            -> ystdlib::error_handling::Result<ClpArchiveReader>;

    /**
     * Creates an SFA reader from in-memory archive bytes.
     *
     * This overload copies the archive bytes referenced by the span into reader-owned storage so
     * the * returned reader does not depend on the lifetime of the caller-provided buffer, which
     * may be transient.
     *
     * @param archive_data Bytes of a single-file archive.
     * @param archive_id Identifier to assign to this archive.
     * @return A result containing the newly constructed `ClpArchiveReader` on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::IoFailure` if archive open/initialization fails.
     */
    [[nodiscard]] static auto
    create(std::span<char const> archive_data, std::string_view archive_id)
            -> ystdlib::error_handling::Result<ClpArchiveReader>;

    // Destructor
    ~ClpArchiveReader();

    // Delete copy constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader const&) = delete;
    auto operator=(ClpArchiveReader const&) -> ClpArchiveReader& = delete;

    // Default move constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader&&) = default;
    [[nodiscard]] auto operator=(ClpArchiveReader&&) -> ClpArchiveReader& = default;

    /**
     * @return A result containing the archive ID on success, or an error code indicating the
     * failure:
     * - `SfaErrorCodeEnum::NotInit` if the reader is not initialized or has already been closed.
     */
    [[nodiscard]] auto get_archive_id() const -> ystdlib::error_handling::Result<std::string>;

    /**
     * @return A result containing the total number of events in the archive on success, or an
     * error code indicating the failure:
     * - `SfaErrorCodeEnum::NotInit` if the reader is not initialized or has already been closed.
     */
    [[nodiscard]] auto get_event_count() const -> ystdlib::error_handling::Result<uint64_t>;

private:
    // Constructors
    explicit ClpArchiveReader(
            std::unique_ptr<clp_s::ArchiveReader> reader,
            std::shared_ptr<std::vector<char>> archive_data
    )
            : m_archive_reader{std::move(reader)},
              m_archive_data{std::move(archive_data)} {}

    // Members
    std::unique_ptr<clp_s::ArchiveReader> m_archive_reader;
    std::shared_ptr<std::vector<char>> m_archive_data;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
