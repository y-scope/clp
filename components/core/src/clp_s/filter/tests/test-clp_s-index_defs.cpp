#include <cstdint>
#include <tuple>

#include <catch2/catch_test_macros.hpp>

#include <clp_s/filter/IndexDefs.hpp>

using clp_s::filter::ArchiveSection;

TEST_CASE("index-defs-index-version-round-trip", "[clp_s][filter]") {
    constexpr uint8_t cMajorVersion{1};
    constexpr uint8_t cMinorVersion{2};
    constexpr uint16_t cPatchVersion{3};
    auto const index_version{
            clp_s::filter::make_index_version(cMajorVersion, cMinorVersion, cPatchVersion)
    };
    auto const [major_version, minor_version, patch_version]{
            clp_s::filter::decompose_index_version(index_version)
    };
    REQUIRE(cMajorVersion == major_version);
    REQUIRE(cMinorVersion == minor_version);
    REQUIRE(cPatchVersion == patch_version);
}

TEST_CASE("index-defs-classify-index-id", "[clp_s][filter]") {
    REQUIRE(clp_s::filter::IndexIdRange::OfficialOpenSource
            == clp_s::filter::classify_index_id(clp_s::filter::cOfficialOpenSourceIndexIdBegin));
    REQUIRE(clp_s::filter::IndexIdRange::OfficialOpenSource
            == clp_s::filter::classify_index_id(
                    clp_s::filter::cOfficialClosedSourceIndexIdBegin - 1
            ));
    REQUIRE(clp_s::filter::IndexIdRange::OfficialClosedSource
            == clp_s::filter::classify_index_id(clp_s::filter::cOfficialClosedSourceIndexIdBegin));
    REQUIRE(clp_s::filter::IndexIdRange::OfficialClosedSource
            == clp_s::filter::classify_index_id(clp_s::filter::cCustomIndexIdBegin - 1));
    REQUIRE(clp_s::filter::IndexIdRange::Custom
            == clp_s::filter::classify_index_id(clp_s::filter::cCustomIndexIdBegin));
}

TEST_CASE("index-defs-archive-section-bitmap", "[clp_s][filter]") {
    auto const bitmap{ArchiveSection::Metadata | ArchiveSection::Dictionaries};
    REQUIRE(clp_s::filter::contains(bitmap, ArchiveSection::Metadata));
    REQUIRE(clp_s::filter::contains(bitmap, ArchiveSection::Dictionaries));
    REQUIRE_FALSE(clp_s::filter::contains(bitmap, ArchiveSection::SchemaMetadata));
    REQUIRE_FALSE(clp_s::filter::contains(bitmap, ArchiveSection::EncodedRecordTables));

    auto const extended_bitmap{bitmap | ArchiveSection::EncodedRecordTables};
    REQUIRE(clp_s::filter::contains(extended_bitmap, ArchiveSection::EncodedRecordTables));

    REQUIRE(clp_s::filter::to_archive_section_bitmap(ArchiveSection::Metadata)
            == clp_s::filter::to_archive_section_bitmap(ArchiveSection::Metadata));
    REQUIRE_FALSE(
            clp_s::filter::contains(
                    clp_s::filter::to_archive_section_bitmap(ArchiveSection::Metadata),
                    ArchiveSection::Dictionaries
            )
    );
}
