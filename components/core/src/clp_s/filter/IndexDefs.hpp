#ifndef CLP_S_FILTER_INDEX_DEFS_HPP
#define CLP_S_FILTER_INDEX_DEFS_HPP

#include <cstdint>
#include <tuple>

namespace clp_s::filter {
/**
 * Identifier for an index type. Encoded as part of the metadata in a Packed Filter, so its width
 * is part of the on-disk format. The ID space is partitioned into the reserved ranges described by
 * `IndexIdRange` and the `c*IndexIdBegin` constants below.
 */
using index_id_t = uint16_t;

/**
 * A 32-bit semantic version of an index implementation, encoded the same way as a clp-s archive
 * version: an 8-bit major version, an 8-bit minor version, and a 16-bit patch version.
 *
 * An index increments its major version when it makes an incompatible change to its serialized
 * format, or when the archive format changes such that the index must operate in a significantly
 * different way on newer archives. Minor version changes correspond to other backwards-compatible
 * changes within a major version, and patch versions represent bug fixes.
 */
using index_version_t = uint32_t;

/**
 * A clp-s archive version, encoded as an 8-bit major version, an 8-bit minor version, and a 16-bit
 * patch version.
 */
using archive_version_t = uint32_t;

/**
 * A bitmap of `ArchiveSection` flags describing which sections of an archive an index needs to
 * read. Since this is a purely in-memory construct, its width may change freely.
 */
using archive_section_bitmap_t = uint8_t;

/**
 * Sections of an archive that an index may need to read while building. Callers build an
 * `archive_section_bitmap_t` by ORing these flags together.
 *
 * NOTE: An actual index implementation may need finer-grained flags than these; the set is
 * expected to grow as indices are added.
 */
enum class ArchiveSection : archive_section_bitmap_t {
    Metadata = 1,
    SchemaMetadata = 1 << 1,
    Dictionaries = 1 << 2,
    EncodedRecordTables = 1 << 3,
};

/**
 * Reserved ranges of the Index ID space.
 */
enum class IndexIdRange : uint8_t {
    OfficialOpenSource = 1,
    OfficialClosedSource,
    Custom,
};

// First Index ID reserved for official open-source indices.
constexpr index_id_t cOfficialOpenSourceIndexIdBegin{0x0000};

// First Index ID reserved for official closed-source indices.
constexpr index_id_t cOfficialClosedSourceIndexIdBegin{0x1000};

// First Index ID free for any custom index.
constexpr index_id_t cCustomIndexIdBegin{0x2000};

/**
 * @param index_id
 * @return The reserved range that `index_id` falls into.
 */
[[nodiscard]] constexpr auto classify_index_id(index_id_t index_id) -> IndexIdRange {
    if (cCustomIndexIdBegin <= index_id) {
        return IndexIdRange::Custom;
    }
    if (cOfficialClosedSourceIndexIdBegin <= index_id) {
        return IndexIdRange::OfficialClosedSource;
    }
    return IndexIdRange::OfficialOpenSource;
}

/**
 * Encodes a semantic version into an `index_version_t`.
 *
 * @param major_version
 * @param minor_version
 * @param patch_version
 * @return The encoded index version.
 */
[[nodiscard]] constexpr auto
make_index_version(uint8_t major_version, uint8_t minor_version, uint16_t patch_version)
        -> index_version_t {
    constexpr uint32_t cMajorVersionOffset{24U};
    constexpr uint32_t cMinorVersionOffset{16U};
    return (static_cast<index_version_t>(major_version) << cMajorVersionOffset)
           | (static_cast<index_version_t>(minor_version) << cMinorVersionOffset)
           | static_cast<index_version_t>(patch_version);
}

/**
 * Decomposes an `index_version_t` into its major, minor, and patch components.
 *
 * @param index_version
 * @return A tuple of the major version, the minor version, and the patch version.
 */
[[nodiscard]] constexpr auto decompose_index_version(index_version_t index_version)
        -> std::tuple<uint8_t, uint8_t, uint16_t> {
    constexpr uint32_t cMajorVersionOffset{24U};
    constexpr uint32_t cMinorVersionOffset{16U};
    auto const major_version{static_cast<uint8_t>(index_version >> cMajorVersionOffset)};
    auto const minor_version{static_cast<uint8_t>(index_version >> cMinorVersionOffset)};
    auto const patch_version{static_cast<uint16_t>(index_version)};
    return std::make_tuple(major_version, minor_version, patch_version);
}

/**
 * @param lhs
 * @param rhs
 * @return A bitmap with the flags of both `lhs` and `rhs` set.
 */
[[nodiscard]] constexpr auto operator|(ArchiveSection lhs, ArchiveSection rhs)
        -> archive_section_bitmap_t {
    return static_cast<archive_section_bitmap_t>(
            static_cast<archive_section_bitmap_t>(lhs) | static_cast<archive_section_bitmap_t>(rhs)
    );
}

/**
 * @param lhs
 * @param rhs
 * @return A copy of `lhs` with `rhs`'s flag additionally set.
 */
[[nodiscard]] constexpr auto operator|(archive_section_bitmap_t lhs, ArchiveSection rhs)
        -> archive_section_bitmap_t {
    return static_cast<archive_section_bitmap_t>(lhs | static_cast<archive_section_bitmap_t>(rhs));
}

/**
 * @param section
 * @return A bitmap with only `section`'s flag set.
 */
[[nodiscard]] constexpr auto to_archive_section_bitmap(ArchiveSection section)
        -> archive_section_bitmap_t {
    return static_cast<archive_section_bitmap_t>(section);
}

/**
 * @param bitmap
 * @param section
 * @return Whether `section`'s flag is set in `bitmap`.
 */
[[nodiscard]] constexpr auto contains(archive_section_bitmap_t bitmap, ArchiveSection section)
        -> bool {
    auto const flag{static_cast<archive_section_bitmap_t>(section)};
    return 0 != (bitmap & flag);
}
}  // namespace clp_s::filter

#endif  // CLP_S_FILTER_INDEX_DEFS_HPP
