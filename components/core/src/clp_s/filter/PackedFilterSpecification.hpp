#ifndef CLP_S_FILTER_PACKED_FILTER_SPECIFICATION_HPP
#define CLP_S_FILTER_PACKED_FILTER_SPECIFICATION_HPP

#include <cstddef>

#include <clp_s/filter/IndexDefs.hpp>

namespace clp_s::filter {
/**
 * Framework-provided description of a Packed Filter being built. Every archive in a Packed Filter
 * is guaranteed to share the same archive version, which simplifies managing support for different
 * archive versions across index implementations.
 */
class PackedFilterSpecification {
public:
    // Constructors
    PackedFilterSpecification(size_t num_archives, archive_version_t archive_version)
            : m_num_archives{num_archives},
              m_archive_version{archive_version} {}

    // Methods
    [[nodiscard]] auto get_num_archives() const -> size_t { return m_num_archives; }

    [[nodiscard]] auto get_archive_version() const -> archive_version_t {
        return m_archive_version;
    }

private:
    // Variables
    size_t m_num_archives;
    archive_version_t m_archive_version;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_PACKED_FILTER_SPECIFICATION_HPP
