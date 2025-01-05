#ifndef CLP_S_ARCHIVEDEFS_HPP
#define CLP_S_ARCHIVEDEFS_HPP

#include <string>

#include "msgpack.hpp"

namespace clp_s {
// define the version
constexpr uint8_t cArchiveMajorVersion = 0;
constexpr uint8_t cArchiveMinorVersion = 2;
constexpr uint16_t cArchivePatchVersion = 0;

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
