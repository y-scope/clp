#ifndef CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
#define CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP

#include <memory>
#include <string>
#include <string_view>

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
     * @return A newly constructed `ClpArchiveReader`.
     */
    [[nodiscard]] static auto create(std::string_view archive_path)
            -> std::unique_ptr<ClpArchiveReader>;

    // Destructor
    ~ClpArchiveReader() noexcept;

    // Delete copy constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader const&) = delete;
    auto operator=(ClpArchiveReader const&) -> ClpArchiveReader& = delete;

    // Default move constructor and assignment operator
    ClpArchiveReader(ClpArchiveReader&&) = default;
    auto operator=(ClpArchiveReader&&) -> ClpArchiveReader& = default;

    /**
     * @return The archive ID parsed from the archive path.
     */
    [[nodiscard]] auto get_archive_id() const -> std::string;

    /**
     * Closes the underlying archive reader. This operation is idempotent.
     */
    void close();

private:
    // Constructors
    explicit ClpArchiveReader(std::unique_ptr<clp_s::ArchiveReader> reader)
            : m_archive_reader(std::move(reader)) {}

    // Members
    std::unique_ptr<clp_s::ArchiveReader> m_archive_reader;
};
}  // namespace clp_s::ffi::sfa

#endif  // CLP_S_FFI_SFA_CLPARCHIVEREADER_HPP
