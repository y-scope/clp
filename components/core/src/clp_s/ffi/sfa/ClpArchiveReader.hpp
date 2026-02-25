#ifndef CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP

#include <memory>
#include <string>
#include <string_view>
#include <cstdint>

#include <ystdlib/error_handling/Result.hpp>

#include "../../ArchiveReader.hpp"

namespace clp_s::ffi::sfa {
/**
 * A thin wrapper around `clp_s::ArchiveReader` for single file archive FFI entrypoints.
 */
class ClpArchiveReader {
public:
    // Factory functions
    /**
     * @param archive_path Path to the single-file archive.
     * @return A result containing the newly constructed `ClpArchiveReader` on success, or an
     * error code indicating the failure:
     * - `std::errc::io_error` if archive open/initialization fails.
     */
    [[nodiscard]] static auto create(std::string_view archive_path)
            -> ystdlib::error_handling::Result<ClpArchiveReader>;

    // Destructor
    ~ClpArchiveReader() noexcept;

    // Delete copy constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader const&) = delete;
    auto operator=(ClpArchiveReader const&) -> ClpArchiveReader& = delete;

    // Default move constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader&&) = default;
    [[nodiscard]] auto operator=(ClpArchiveReader&&) -> ClpArchiveReader& = default;

    /**
     * @return The archive ID.
     */
    [[nodiscard]] auto get_archive_id() const -> std::string;

    /**
     * @return The total number of events in the archive.
     */
    [[nodiscard]] auto get_event_count() const -> uint64_t;

private:
    // Constructors
    explicit ClpArchiveReader(std::unique_ptr<clp_s::ArchiveReader> reader)
            : m_archive_reader(std::move(reader)) {}

    // Members
    std::unique_ptr<clp_s::ArchiveReader> m_archive_reader;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
