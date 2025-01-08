#ifndef CLP_S_ARCHIVEREADERADAPTOR_HPP
#define CLP_S_ARCHIVEREADERADAPTOR_HPP

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "../clp/BoundedReader.hpp"
#include "../clp/ReaderInterface.hpp"
#include "InputConfig.hpp"
#include "SingleFileArchiveDefs.hpp"
#include "TimestampDictionaryReader.hpp"
#include "TraceableException.hpp"
#include "ZstdDecompressor.hpp"

namespace clp_s {
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

    ~ArchiveReaderAdaptor();

    /**
     * Load metadata for an archive including the header and metadata section. This method must be
     * invoked before checking out any section of an archive, or calling `get_timestamp_dictionary`.
     * @return ErrorCodeSuccess on success
     * @return ErrorCode_errno on failure
     */
    ErrorCode load_archive_metadata();

    /**
     * Checkout a reader for a given section of the archive. Reader must be checked back in with the
     * `checkin_reader_for_section` method.
     * @param section
     * @return A ReaderInterface opened and pointing to the requested section.
     * @throw OperationFailed if a reader is already checked out, or checking out this section would
     *        force a backwards seek.
     */
    std::unique_ptr<clp::ReaderInterface> checkout_reader_for_section(std::string_view section);

    /**
     * Checkin a reader for a given section of the archive.
     * @param section
     * @throw OperationFailed if no reader is checked out, or if the section being checked in does
     *        not match the section currently checked out.
     */
    void checkin_reader_for_section(std::string_view section);

    std::shared_ptr<TimestampDictionaryReader> get_timestamp_dictionary() {
        return m_timestamp_dictionary;
    }

    ArchiveHeader const& get_header() const { return m_archive_header; }

private:
    ErrorCode try_read_archive_file_info(ZstdDecompressor& decompressor, size_t size);

    ErrorCode try_read_timestamp_dictionary(ZstdDecompressor& decompressor, size_t size);

    ErrorCode try_read_archive_info(ZstdDecompressor& decompressor, size_t size);

    std::shared_ptr<clp::ReaderInterface> try_create_reader_at_header();

    std::unique_ptr<clp::ReaderInterface> checkout_reader_for_sfa_section(std::string_view section);

    ErrorCode try_read_header(clp::ReaderInterface& reader);

    ErrorCode try_read_archive_metadata(ZstdDecompressor& reader);

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
};

}  // namespace clp_s
#endif  // CLP_S_ARCHIVEREADERADAPTOR_HPP
