#ifndef CLP_S_FILTER_INDEX_BUILDER_HPP
#define CLP_S_FILTER_INDEX_BUILDER_HPP

#include <cstdint>
#include <span>
#include <vector>

#include <ystdlib/error_handling/Result.hpp>

namespace clp_s {
class ArchiveReader;
}  // namespace clp_s

namespace clp_s::filter {
/**
 * Interface for building an index over the archives of a Packed Filter. The framework feeds an
 * implementation one archive at a time so that only a single archive needs to be loaded into memory
 * at once; the implementation produces that archive's serialized blob, which the framework collects
 * to assemble the Packed Filter.
 */
class IndexBuilder {
public:
    // Constructors
    IndexBuilder() = default;

    // Delete copy constructor and assignment operator
    IndexBuilder(IndexBuilder const&) = delete;
    auto operator=(IndexBuilder const&) -> IndexBuilder& = delete;

    // Default move constructor and assignment operator
    IndexBuilder(IndexBuilder&&) noexcept = default;
    auto operator=(IndexBuilder&&) noexcept -> IndexBuilder& = default;

    // Destructor
    virtual ~IndexBuilder() = default;

    // Methods
    /**
     * Builds the index's data for an archive and stores the resulting per-archive blob.
     *
     * @param local_archive_id The Packed-Filter-local ID of the archive, in the range
     * [0, num_archives).
     * @param archive_reader A reader for the archive identified by `local_archive_id`.
     * @return A void result on success, or an error code indicating the failure:
     * - Error codes are defined by the derived class.
     */
    [[nodiscard]] virtual auto
    add_archive(uint16_t local_archive_id, clp_s::ArchiveReader const& archive_reader)
            -> ystdlib::error_handling::Result<void>
            = 0;

    /**
     * @return The serialized blob for each added archive, indexed by local archive ID. The returned
     * views remain valid until the next call to `add_archive` or the builder's destruction.
     */
    [[nodiscard]] virtual auto get_archive_blobs() const -> std::vector<std::span<char const>> = 0;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_INDEX_BUILDER_HPP
