#ifndef CLP_S_ARCHIVEDEFS_HPP
#define CLP_S_ARCHIVEDEFS_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "msgpack.hpp"

namespace clp_s {
/**
 * @param major_version
 * @param minor_version
 * @param patch_version
 * @return The archive version, composed of a major, minor, and patch version.
 */
constexpr auto
make_archive_version(uint8_t major_version, uint8_t minor_version, uint16_t patch_version)
        -> uint32_t {
    constexpr uint32_t cMajorVersionOffset{24U};
    constexpr uint32_t cMinorVersionOffset{16U};
    return (static_cast<uint32_t>(major_version) << cMajorVersionOffset)
           | (static_cast<uint32_t>(minor_version) << cMinorVersionOffset)
           | static_cast<uint32_t>(patch_version);
}

// define the version
constexpr uint8_t cArchiveMajorVersion = 0;
constexpr uint8_t cArchiveMinorVersion = 4;
constexpr uint16_t cArchivePatchVersion = 1;
constexpr uint32_t cArchiveVersion{
        make_archive_version(cArchiveMajorVersion, cArchiveMinorVersion, cArchivePatchVersion)
};

// define the magic number
constexpr uint8_t cStructuredSFAMagicNumber[] = {0xFD, 0x2F, 0xC5, 0x30};

struct ArchiveHeader {
    uint8_t magic_number[4];
    uint32_t version;
    uint64_t uncompressed_size;
    uint64_t compressed_size;
    uint64_t reserved_padding[4];
    uint32_t metadata_section_size;
    uint16_t compression_type;
    uint16_t padding;
};

enum class ArchiveCompressionType : uint16_t {
    Zstd = 0,
};

enum struct ArchiveMetadataPacketType : uint8_t {
    ArchiveInfo = 0,
    ArchiveFileInfo = 1,
    TimestampDictionary = 2,
    RangeIndex = 3
};

struct ArchiveInfoPacket {
    uint64_t num_segments;
    // TODO: Add more fields in the future

    MSGPACK_DEFINE_MAP(num_segments);
};

struct ArchiveFileInfo {
    std::string n;  // name
    uint64_t o;  // offset

    MSGPACK_DEFINE_MAP(n, o);
};

struct ArchiveFileInfoPacket {
    std::vector<ArchiveFileInfo> files;

    MSGPACK_DEFINE_MAP(files);
};
}  // namespace clp_s

#endif  // CLP_S_ARCHIVEDEFS_HPP
