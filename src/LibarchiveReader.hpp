#ifndef LIBARCHIVEREADER_HPP
#define LIBARCHIVEREADER_HPP

// C++ standard libraries
#include <string>
#include <vector>

// libarchive
#include <archive.h>

// Project headers
#include "ErrorCode.hpp"
#include "FileReader.hpp"
#include "LibarchiveFileReader.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

/**
 * Class for reading archives through libarchive
 */
class LibarchiveReader {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed (ErrorCode error_code, const char* const filename, int line_number) : TraceableException (error_code, filename, line_number) {}

        // Methods
        const char* what () const noexcept override {
            return "LibarchiveReader operation failed";
        }
    };

    // Constructors
    LibarchiveReader () :
            m_archive(nullptr),
            m_archive_entry(nullptr),
            m_file_reader(nullptr),
            m_initial_buffer_content_exhausted(false),
            m_is_opened_by_libarchive(false)
            {}

    // Methods
    /**
     * Tries to open the archive or compressed file contained across the given buffer and FileReader
     * @param buffer_length
     * @param buffer
     * @param file_reader
     * @param path_if_compressed_file Path to use if the data is a single compressed file
     * @return ErrorCode_Success on success
     * @return ErrorCode_Failure on failure
     */
    ErrorCode try_open (size_t buffer_length, const char* buffer, FileReader& file_reader, const std::string& path_if_compressed_file);
    /**
     * Closes the reader
     */
    void close ();

    /**
     * Tries to read the next entry's header from the archive
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Failure on failure
     * @return ErrorCode_Success on success
     */
    ErrorCode try_read_next_header ();

    /**
     * Opens the current entry within the given reader
     * @param libarchive_file_reader
     */
    void open_file_reader (LibarchiveFileReader& libarchive_file_reader);

    /**
     * Gets the type of the current entry
     * @return The current entry's type
     */
    mode_t get_entry_file_type () const;
    /**
     * Gets the path of the current entry
     * @return The current entry's path within the archive
     */
    const char* get_path () const;

private:
    // Methods
    /**
     * Callback for libarchive->open
     * @param archive
     * @param client_data
     * @return ARCHIVE_OK on success
     * @return ARCHIVE_FATAL on failure
     */
    static int libarchive_open_callback (struct archive* archive, void* client_data);
    /**
     * Callback for libarchive->close
     * @param archive
     * @param client_data
     * @return ARCHIVE_OK on success
     * @return ARCHIVE_FATAL on failure
     */
    static int libarchive_close_callback (struct archive* archive, void* client_data);

    /**
     * Callback for libarchive->read
     * @param archive
     * @param client_data
     * @param buffer
     * @return Number of bytes read on success
     * @return 0 on EOF
     * @return -1 on failure
     */
    static la_ssize_t libarchive_read_callback (struct archive* archive, void* client_data, const void** buffer);
    /**
     * Callback for libarchive->skip
     * @param archive
     * @param client_data
     * @param request
     * @return Number of bytes skipped on success
     * @return ARCHIVE_FATAL on failure
     */
    static la_int64_t libarchive_skip_callback (struct archive* archive, void* client_data, off_t request);

    /**
     * Marks the archive opened by libarchive
     */
    void libarchive_open_callback ();
    /**
     * Marks the archive closed by libarchive
     */
    void libarchive_close_callback ();

    /**
     * Reads a chunk of data from the underlying file
     * @param buffer
     * @param num_bytes_read
     * @return ErrorCode_NotInit if not opened by libarchive
     * @return Same as FileReader::try_read
     * @return ErrorCode_Success on success
     */
    ErrorCode libarchive_read_callback (const void** buffer, size_t& num_bytes_read);
    /**
     * Skips the number of bytes given or to the end of the file, whichever is closer
     * @param num_bytes_to_skip
     * @param num_bytes_skipped
     * @return Same as FileReader::try_get_pos
     * @return Same as FileReader::try_fstat
     * @return Same as FileReader::try_seek_from_begin
     * @return ErrorCode_Success on success
     */
    ErrorCode libarchive_skip_callback (off_t num_bytes_to_skip, size_t& num_bytes_skipped);

    /**
     * Releases resources allocated and saved by opening an archive
     */
    void release_resources ();

    // Variables
    struct archive* m_archive;
    struct archive_entry* m_archive_entry;

    std::vector<char> m_buffer;
    FileReader* m_file_reader;
    bool m_initial_buffer_content_exhausted;

    std::string m_filename_if_compressed;

    bool m_is_opened_by_libarchive;
};

#endif // LIBARCHIVEREADER_HPP
