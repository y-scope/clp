#ifndef CLP_S_FILTER_INDEX_BUILDER_SPECIFICATION_HPP
#define CLP_S_FILTER_INDEX_BUILDER_SPECIFICATION_HPP

#include <memory>
#include <optional>

#include <nlohmann/json_fwd.hpp>
#include <ystdlib/error_handling/Result.hpp>

#include <clp_s/filter/IndexBuilder.hpp>
#include <clp_s/filter/IndexDefs.hpp>
#include <clp_s/filter/PackedFilterSpecification.hpp>

namespace clp_s::filter {
/**
 * Describes a single `IndexBuilder` implementation registered for an index: the archive sections it
 * reads, the range of archive versions it supports, its index version, and a factory for creating
 * instances of it.
 *
 * The range of supported archive versions is half-open:
 * [first_supported_archive_version, last_supported_archive_version). A null last version indicates
 * an open range that includes the newest archive version.
 */
class IndexBuilderSpecification {
public:
    // Types
    /**
     * A factory for creating an `IndexBuilder`.
     *
     * @param config Implementation-defined configuration.
     * @param packed_filter_spec A description of the Packed Filter being built.
     * @return A result containing the created builder on success, or an error code indicating the
     * failure:
     * - Error codes are defined by the implementation.
     */
    using Factory = auto (*)(
            nlohmann::json const& config,
            PackedFilterSpecification const& packed_filter_spec
    ) -> ystdlib::error_handling::Result<std::unique_ptr<IndexBuilder>>;

    // Constructors
    IndexBuilderSpecification(
            archive_section_bitmap_t archive_section_bitmap,
            archive_version_t first_supported_archive_version,
            std::optional<archive_version_t> last_supported_archive_version,
            index_version_t index_version,
            Factory factory
    )
            : m_archive_section_bitmap{archive_section_bitmap},
              m_first_supported_archive_version{first_supported_archive_version},
              m_last_supported_archive_version{last_supported_archive_version},
              m_index_version{index_version},
              m_factory{factory} {}

    // Methods
    /**
     * @param archive_version
     * @return Whether this builder supports `archive_version`.
     */
    [[nodiscard]] auto supports_archive_version(archive_version_t archive_version) const -> bool {
        if (archive_version < m_first_supported_archive_version) {
            return false;
        }
        return false == m_last_supported_archive_version.has_value()
               || archive_version < m_last_supported_archive_version.value();
    }

    /**
     * Creates an `IndexBuilder` using the registered factory.
     *
     * @param config Implementation-defined configuration.
     * @param packed_filter_spec A description of the Packed Filter being built.
     * @return Forwards the factory's return values.
     */
    [[nodiscard]] auto create_builder(
            nlohmann::json const& config,
            PackedFilterSpecification const& packed_filter_spec
    ) const -> ystdlib::error_handling::Result<std::unique_ptr<IndexBuilder>> {
        return m_factory(config, packed_filter_spec);
    }

    [[nodiscard]] auto get_archive_section_bitmap() const -> archive_section_bitmap_t {
        return m_archive_section_bitmap;
    }

    [[nodiscard]] auto get_index_version() const -> index_version_t { return m_index_version; }

private:
    // Variables
    archive_section_bitmap_t m_archive_section_bitmap;
    archive_version_t m_first_supported_archive_version;
    std::optional<archive_version_t> m_last_supported_archive_version;
    index_version_t m_index_version;
    Factory m_factory;
};
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_INDEX_BUILDER_SPECIFICATION_HPP
