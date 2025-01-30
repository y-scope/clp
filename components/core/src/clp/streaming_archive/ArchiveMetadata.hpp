#ifndef CLP_STREAMING_ARCHIVE_ARCHIVEMETADATA_HPP
#define CLP_STREAMING_ARCHIVE_ARCHIVEMETADATA_HPP

#include <cstdint>
#include <string_view>

#include "../Defs.h"
#include "../ffi/encoding_methods.hpp"
#include "../FileReader.hpp"
#include "../FileWriter.hpp"
#include "Constants.hpp"
#include "msgpack.hpp"

namespace clp::streaming_archive {

static constexpr std::string_view cCompressionTypeZstd = "ZSTD";

/**
 * A class to encapsulate metadata directly relating to an archive.
 */
class ArchiveMetadata {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        [[nodiscard]] auto what() const noexcept -> char const* override {
            return "streaming_archive::ArchiveMetadata operation failed";
        }
    };

    // Constructors
    // The class must be constructible to convert from msgpack::object.
    // https://github.com/msgpack/msgpack-c/wiki/v2_0_cpp_adaptor
    ArchiveMetadata() = default;

    /**
     * Constructs a metadata object with the given parameters
     * @param archive_format_version
     * @param creator_id
     * @param creation_idx
     */
    ArchiveMetadata(
            archive_format_version_t archive_format_version,
            std::string creator_id,
            uint64_t creation_idx
    );

    // Methods
    /**
     * Reads serialized MessagePack data from open file, unpacks it into an
     * `ArchiveMetadata` instance.
     *
     * @param file_reader Reader for the file containing archive metadata.
     * @return The created instance.
     * @throw `ArchiveMetadata::OperationFailed` if stat or read operation on metadata file fails.
     * @throw `msgpack::unpack_error` if data cannot be unpacked into MessagePack object.
     * @throw `msgpack::type_error` if MessagePack object can't be converted to `ArchiveMetadata`.
     */
    [[nodiscard]] static auto create_from_file_reader(FileReader& file_reader) -> ArchiveMetadata;

    [[nodiscard]] auto get_archive_format_version() const { return m_archive_format_version; }

    [[nodiscard]] auto get_creator_id() const -> std::string const& { return m_creator_id; }

    [[nodiscard]] auto get_creation_idx() const { return m_creation_idx; }

    [[nodiscard]] auto get_uncompressed_size_bytes() const {
        return m_uncompressed_size + m_dynamic_uncompressed_size;
    }

    void increment_static_uncompressed_size(uint64_t size_bytes) {
        m_uncompressed_size += size_bytes;
    }

    void set_dynamic_uncompressed_size(uint64_t size_bytes) {
        m_dynamic_uncompressed_size = size_bytes;
    }

    [[nodiscard]] auto get_compressed_size_bytes() const {
        return m_compressed_size + m_dynamic_compressed_size;
    }

    void increment_static_compressed_size(uint64_t size_bytes) { m_compressed_size += size_bytes; }

    void set_dynamic_compressed_size(uint64_t size_bytes) {
        m_dynamic_compressed_size = size_bytes;
    }

    [[nodiscard]] auto get_begin_timestamp() const { return m_begin_timestamp; }

    [[nodiscard]] auto get_end_timestamp() const { return m_end_timestamp; }

    [[nodiscard]] auto get_variable_encoding_methods_version() const -> std::string_view const& {
        return m_variable_encoding_methods_version;
    }

    [[nodiscard]] auto get_variables_schema_version() const -> std::string_view const& {
        return m_variables_schema_version;
    }

    [[nodiscard]] auto get_compression_type() const -> std::string_view const& {
        return m_compression_type;
    }

    /**
     * Expands the archive's time range based to encompass the given time range
     * @param begin_timestamp
     * @param end_timestamp
     */
    void expand_time_range(epochtime_t begin_timestamp, epochtime_t end_timestamp);

    /**
     * Packs `ArchiveMetadata` to MessagePack and writes to the open file.
     *
     * @param file_writer Writer for archive metadata file.
     * @throw FileWriter::OperationFailed if failed to write.
     */
    void write_to_file(FileWriter& file_writer) const;

    MSGPACK_DEFINE_MAP(
            MSGPACK_NVP("archive_format_version", m_archive_format_version),
            MSGPACK_NVP("variable_encoding_methods_version", m_variable_encoding_methods_version),
            MSGPACK_NVP("variables_schema_version", m_variables_schema_version),
            MSGPACK_NVP("compression_type", m_compression_type),
            MSGPACK_NVP("creator_id", m_creator_id),
            MSGPACK_NVP("creation_idx", m_creation_idx),
            MSGPACK_NVP("begin_timestamp", m_begin_timestamp),
            MSGPACK_NVP("end_timestamp", m_end_timestamp),
            MSGPACK_NVP("uncompressed_size", m_uncompressed_size),
            MSGPACK_NVP("compressed_size", m_compressed_size)
    );

private:
    // Variables
    archive_format_version_t m_archive_format_version{cArchiveFormatVersion};
    std::string m_creator_id;
    uint16_t m_creator_id_len{0};
    uint64_t m_creation_idx{0};
    epochtime_t m_begin_timestamp{cEpochTimeMax};
    epochtime_t m_end_timestamp{cEpochTimeMin};
    // The size of the data stored in the archive before compression
    uint64_t m_uncompressed_size{0};
    uint64_t m_dynamic_uncompressed_size{0};
    // The size of the archive
    uint64_t m_compressed_size{0};
    uint64_t m_dynamic_compressed_size{0};
    std::string_view m_variable_encoding_methods_version{ffi::cVariableEncodingMethodsVersion};
    std::string_view m_variables_schema_version{ffi::cVariablesSchemaVersion};
    std::string_view m_compression_type{cCompressionTypeZstd};
};
}  // namespace clp::streaming_archive

#endif  // CLP_STREAMING_ARCHIVE_ARCHIVEMETADATA_HPP
