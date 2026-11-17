#ifndef CLP_S_ARCHIVEREADERADAPTOR_HPP
#define CLP_S_ARCHIVEREADERADAPTOR_HPP

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// We use NOLINTNEXTLINE to satisfy clang-tidy here because while we don't use any symbols from
// `nlohmann/json.hpp` directly this code does not compile without the definition of
// `nlohmann::basic_json<>` found in the `nlohmann/json.hpp` header.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <ystdlib/containers/Array.hpp>

#include <clp_s/Defs.hpp>

#include "../clp/BoundedReader.hpp"
#include "../clp/ReaderInterface.hpp"
#include "InputConfig.hpp"
#include "SingleFileArchiveDefs.hpp"
#include "TimestampDictionaryReader.hpp"
#include "TraceableException.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
/**
 * RangeIndexEntry is a struct representing a single entry in the archive range index.
 */
struct RangeIndexEntry {
    explicit RangeIndexEntry(size_t start_index, size_t end_index, nlohmann::json&& fields)
            : start_index{start_index},
              end_index{end_index},
              // Note: brace initializer would make nlohmann wrap the fields object in an array.
              fields(std::move(fields)) {}

    size_t start_index;
    size_t end_index;
    nlohmann::json fields;
};

/**
 * ArchiveReaderAdaptor is an adaptor class which helps with reading single and multi-file archives
 * which exist on either S3 or a locally mounted file system.
 */
class ArchiveReaderAdaptor {
public:
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}
    };

    // Constructors
    /**
     * Creates an adaptor for an archive identified by path and source type.
     * @param archive_path Path/URL for a directory archive or a single-file archive.
     * @param network_auth Authentication options for network inputs.
     */
    explicit ArchiveReaderAdaptor(Path const& archive_path, NetworkAuthOption const& network_auth);

    /**
     * Creates an adaptor around an already opened single-file archive reader.
     * @param single_file_archive_reader An already-opened single-file archive stream.
     */
    explicit ArchiveReaderAdaptor(std::shared_ptr<clp::ReaderInterface> single_file_archive_reader);

    /**
     * Loads metadata for an archive including the header and metadata section. This method must be
     * invoked before checking out any section of an archive, or calling `get_timestamp_dictionary`.
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode load_archive_metadata();

    /**
     * Checks out a reader for a given section of the archive. The reader must be checked back in
     * with `checkin_reader_for_section`.
     *
     * In a multi-file archive every section is its own file, so any number of sections can be
     * checked out at once, in any order. In a single-file archive the sections share one forward-
     * only stream, so only one stream section can be checked out at a time; any unread section the
     * stream skips over is retained in memory (compressed) so that it can be checked out later.
     * Buffered bytes are released when the section is checked in or when this adaptor is destroyed.
     * Readers over buffered bytes must be destroyed before check-in.
     * @param section
     * @return A ReaderInterface opened and pointing to the requested section.
     * @throws OperationFailed if another single-file archive stream section is checked out, if the
     * section doesn't exist, or on any I/O error.
     */
    auto checkout_reader_for_section(std::string_view section)
            -> std::unique_ptr<clp::ReaderInterface>;

    /**
     * Checks in a reader for a given section of the archive, releasing the section's buffered bytes
     * if any. This is a no-op for multi-file archive sections.
     * @param section The section being checked in. Readers over its buffered bytes must already
     * have been destroyed.
     * @throws OperationFailed if the section is a single-file archive stream section and either no
     * reader is checked out or the section doesn't match the section currently checked out.
     */
    auto checkin_reader_for_section(std::string_view section) -> void;

    std::shared_ptr<TimestampDictionaryReader> get_timestamp_dictionary() {
        return m_timestamp_dictionary;
    }

    ArchiveHeader const& get_header() const { return m_archive_header; }

    std::vector<RangeIndexEntry> const& get_range_index() const { return m_range_index; }

    /**
     * @return The names of every section in the archive, in on-disk order.
     */
    [[nodiscard]] auto get_sections() const -> std::vector<std::string_view>;

    /**
     * @return The flavour of CLP the archive was compressed with.
     */
    [[nodiscard]] auto flavour() const -> ClpFlavour { return m_archive_info.flavour; }

    /**
     * @param log_event_idx
     * @return The file-level metadata associated with the record at `log_event_idx`.
     * @throws OperationFailed when `log_event_idx` cannot be mapped to any metadata.
     */
    [[nodiscard]] auto get_metadata_for_log_event(int64_t log_event_idx) -> nlohmann::json const&;

private:
    /**
     * Tries to read an ArchiveFileInfo packet from the archive metadata.
     * @param decompressor
     * @param size The number of decompressed bytes making up the packet.
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode try_read_archive_file_info(ZstdDecompressor& decompressor, size_t size);

    /**
     * Tries to read an TimestampDictionary packet from the archive metadata.
     * @param decompressor
     * @param size The number of decompressed bytes making up the packet.
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode try_read_timestamp_dictionary(ZstdDecompressor& decompressor, size_t size);

    /**
     * Tries to read an ArchiveInfo packet from the archive metadata.
     * @param decompressor
     * @param size The number of decompressed bytes making up the packet.
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode try_read_archive_info(ZstdDecompressor& decompressor, size_t size);

    /**
     * Tries to read a RangeIndex packet from the archive metadata.
     * @param decompressor
     * @param size The number of decompressed bytes making up the packet.
     * @return ErrorCodeSuccess on success or the relevant ErrorCode on failure.
     */
    auto try_read_range_index(ZstdDecompressor& decompressor, size_t size) -> ErrorCode;

    /**
     * Tries to read an unknown metadata packet from the archive metadata.
     * @param decompressor
     * @param size The number of decompressed bytes making up the packet.
     * @return ErrorCodeSuccess on success or the relevant ErrorCode on failure.
     */
    auto try_read_unknown_metadata_packet(ZstdDecompressor& decompressor, size_t size) -> ErrorCode;

    /**
     * Tries to create a reader for the archive header.
     * @return A ReaderInterface opened and pointing to the archive header on success.
     * @return nullptr on failure.
     */
    std::shared_ptr<clp::ReaderInterface> try_create_reader_at_header();

    /**
     * Checks out a reader for a given section of the single-file archive's stream, buffering the
     * unread sections the stream skips over to reach it.
     *
     * A checked-in section may still have unread trailing bytes since a streaming decompressor can
     * produce all expected output before consuming the compressed frame's trailer. Such a section
     * is skipped without buffering, and the forward seek to the requested section discards the
     * remainder. A partially consumed section that was never checked in is rejected since its
     * contents may not have been read successfully.
     * @param section
     * @return A ReaderInterface opened and pointing to the requested section.
     * @throws OperationFailed if another stream section is checked out, if the requested section
     * doesn't exist or was already passed by the stream, if a section that wasn't checked in was
     * partially consumed, or on any I/O error.
     */
    auto checkout_reader_for_sfa_section(std::string_view section)
            -> std::unique_ptr<clp::ReaderInterface>;

    /**
     * @param section_it An iterator into `m_archive_file_info.files`.
     * @return A pair of offsets into the single file archive's stream:
     * - The offset of the section's first byte.
     * - The offset one past the section's last byte.
     * @throws OperationFailed with ErrorCodeOutOfBounds if the archive size doesn't fit in size_t.
     * @throws OperationFailed with ErrorCodeCorrupt if the section bounds are reversed or outside
     * the archive's files region.
     */
    [[nodiscard]] auto get_sfa_section_bounds(
            std::vector<ArchiveFileInfo>::const_iterator section_it
    ) const -> std::pair<size_t, size_t>;

    /**
     * Seeks the single-file archive stream to an absolute offset, avoiding redundant seeks. Seeking
     * to the end of the stream is permitted so that an empty trailing section can be checked out.
     * @param offset
     * @throws OperationFailed on any I/O error other than reaching the end of the stream.
     */
    auto seek_to(size_t offset) -> void;

    /**
     * Tries to read the header for the archive from the given reader.
     * @param reader
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode try_read_header(clp::ReaderInterface& reader);

    /**
     * Tries to read the archive metadata from the given decompressor.
     * @param decompressor
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode try_read_archive_metadata(ZstdDecompressor& decompressor);

    Path m_archive_path{};
    NetworkAuthOption m_network_auth{};
    bool m_single_file_archive{false};
    ArchiveFileInfoPacket m_archive_file_info{};
    ArchiveHeader m_archive_header{};
    ArchiveInfoPacket m_archive_info{};
    size_t m_files_section_offset{};
    std::optional<std::string> m_current_reader_holder;
    std::unordered_set<std::string> m_checked_in_sections;
    std::unordered_map<std::string, ystdlib::containers::Array<char>> m_buffered_sections;
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dictionary;
    std::shared_ptr<clp::ReaderInterface> m_reader;
    std::vector<RangeIndexEntry> m_range_index;
    std::map<int64_t, nlohmann::json> m_non_empty_range_metadata_map;
};
}  // namespace clp_s
#endif  // CLP_S_ARCHIVEREADERADAPTOR_HPP
