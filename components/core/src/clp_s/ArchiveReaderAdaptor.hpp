#ifndef CLP_S_ARCHIVEREADERADAPTOR_HPP
#define CLP_S_ARCHIVEREADERADAPTOR_HPP

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// We use NOLINTNEXTLINE to satisfy clang-tidy here because while we don't use any symbols from
// `nlohmann/json.hpp` directly this code does not compile without the definition of
// `nlohmann::basic_json<>` found in the `nlohmann/json.hpp` header.
// NOLINTNEXTLINE(misc-include-cleaner)
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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

    explicit ArchiveReaderAdaptor(Path const& archive_path, NetworkAuthOption const& network_auth);

    /**
     * Loads metadata for an archive including the header and metadata section. This method must be
     * invoked before checking out any section of an archive, or calling `get_timestamp_dictionary`.
     * @return ErrorCodeSuccess on success.
     * @return relevant ErrorCode on failure.
     */
    ErrorCode load_archive_metadata();

    /**
     * Checks out a reader for a given section of the archive. Reader must be checked back in with
     * the `checkin_reader_for_section` method.
     * @param section
     * @return A ReaderInterface opened and pointing to the requested section.
     * @throw OperationFailed if a reader is already checked out, or checking out this section would
     *        force a backwards seek.
     */
    std::unique_ptr<clp::ReaderInterface> checkout_reader_for_section(std::string_view section);

    /**
     * Checks in a reader for a given section of the archive.
     * @param section
     * @throw OperationFailed if no reader is checked out, or if the section being checked in does
     *        not match the section currently checked out.
     */
    void checkin_reader_for_section(std::string_view section);

    std::shared_ptr<TimestampDictionaryReader> get_timestamp_dictionary() {
        return m_timestamp_dictionary;
    }

    ArchiveHeader const& get_header() const { return m_archive_header; }

    std::vector<RangeIndexEntry> const& get_range_index() const { return m_range_index; }

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
     * Checks out a reader for a given section of the single file archive.
     * @param section
     * @return A ReaderInterface opened and pointing to the requested section.
     * @throw OperationFailed if the requested section does not exist in ArchiveFileInfo, if
     *        checking out the section would force a backward seek, or on any I/O error.
     */
    std::unique_ptr<clp::ReaderInterface> checkout_reader_for_sfa_section(std::string_view section);

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
    std::shared_ptr<TimestampDictionaryReader> m_timestamp_dictionary;
    std::shared_ptr<clp::ReaderInterface> m_reader;
    std::vector<RangeIndexEntry> m_range_index;
    std::map<int64_t, nlohmann::json> m_non_empty_range_metadata_map;
};
}  // namespace clp_s
#endif  // CLP_S_ARCHIVEREADERADAPTOR_HPP
