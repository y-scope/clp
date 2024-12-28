#ifndef CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_DEFS_HPP
#define CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_DEFS_HPP

#include <cstdint>
#include <string>

#include "../ArchiveMetadata.hpp"
#include "../clp/Defs.h"
#include "../Constants.hpp"
#include "msgpack.hpp"

namespace clp::streaming_archive::single_file_archive {

using single_file_archive_format_version_t = uint32_t;

// Single file archive version.
constexpr uint8_t cArchiveMajorVersion{0};
constexpr uint8_t cArchiveMinorVersion{1};
constexpr uint16_t cArchivePatchVersion{1};
constexpr single_file_archive_format_version_t cArchiveVersion{
        cArchiveMajorVersion << 24 | cArchiveMinorVersion << 16 | cArchivePatchVersion
};

static constexpr std::array<uint8_t, 4> cUnstructuredSfaMagicNumber = {'Y', 'C', 'L', 'P'};
static constexpr std::string_view cUnstructuredSfaExtension = ".clp";
static constexpr std::string_view cCompressionTypeZstd = "ZSTD";

static constexpr size_t cMagicNumberSize = 4;
static constexpr size_t cUnusedSize = 4;

struct __attribute__((packed)) SingleFileArchiveHeader {
    std::array<uint8_t, cMagicNumberSize> magic;
    single_file_archive_format_version_t version;
    uint64_t metadata_size;
    std::array<uint64_t, cUnusedSize> unused;
};

constexpr std::array<char const*, 5> cStaticArchiveFileNames
        = {cMetadataDBFileName,
           cLogTypeDictFilename,
           cLogTypeSegmentIndexFilename,
           cVarDictFilename,
           cVarSegmentIndexFilename};

struct FileInfo {
    std::string n;
    uint64_t o;
    MSGPACK_DEFINE_MAP(n, o);
};

struct ArchiveMetadata {
    single_file_archive_format_version_t archive_format_version;
    std::string variable_encoding_methods_version;
    std::string variables_schema_version;
    std::string compression_type;
    std::string creator_id;
    epochtime_t begin_timestamp;
    epochtime_t end_timestamp;
    uint64_t uncompressed_size;
    uint64_t compressed_size;
    MSGPACK_DEFINE_MAP(
            archive_format_version,
            variable_encoding_methods_version,
            variables_schema_version,
            compression_type,
            creator_id,
            begin_timestamp,
            end_timestamp,
            uncompressed_size,
            compressed_size
    );
};

struct SingleFileArchiveMetadata {
    std::vector<FileInfo> archive_files;
    ArchiveMetadata archive_metadata;
    uint64_t num_segments;
    MSGPACK_DEFINE_MAP(archive_files, archive_metadata, num_segments);
};
}  // namespace clp::streaming_archive::single_file_archive

#endif  // CLP_STREAMING_ARCHIVE_SINGLE_FILE_ARCHIVE_DEFS_HPP
