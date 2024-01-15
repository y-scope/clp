#ifndef GLT_LIBARCHIVEFILEREADER_HPP
#define GLT_LIBARCHIVEFILEREADER_HPP

#include <array>
#include <string>

#include <archive.h>

#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

namespace glt {
/**
 * Class for reading a file from an archive through libarchive
 */
class LibarchiveFileReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override {
            return "LibarchiveFileReader operation failed";
        }
    };

    // Constructors
    LibarchiveFileReader()
            : m_archive(nullptr),
              m_archive_entry(nullptr),
              m_data_block(nullptr),
              m_reached_eof(false),
              m_pos_in_file(0) {}

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the file
     * @param pos Position of the read head in the file
     * @return ErrorCode_Success
     */
    ErrorCode try_get_pos(size_t& pos) override;
    /**
     * Unsupported method
     * @param pos
     * @return N/A
     */
    ErrorCode try_seek_from_begin(size_t pos) override;
    /**
     * Tries to read up to a given number of bytes from the file
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Failure on failure
     * @return ErrorCode_Success on success
     */
    ErrorCode try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) override;

    // Methods overriding the ReaderInterface
    /**
     * Tries to read a string from the file until it reaches the specified delimiter
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the output string or not
     * @param append Whether to append to the given string or replace its contents
     * @param str The string read
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Failure on failure
     * @return ErrorCode_Success on success
     */
    ErrorCode
    try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str) override;

    // Methods
    /**
     * Opens the file reader
     * @param archive
     * @param archive_entry
     */
    void open(struct archive* archive, struct archive_entry* archive_entry);
    /**
     * Closes the file reader
     */
    void close();

    /**
     * Tries to the load a data block from the file if none is loaded
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Failure on failure
     * @return ErrorCode_Success on success
     */
    [[nodiscard]] ErrorCode try_load_data_block();

    /**
     * Peeks the remaining buffered content without advancing the read head.
     *
     * NOTE: Any subsequent read or seek operations may invalidate the returned buffer.
     * @param buf Returns a pointer to any buffered data
     * @param buf_size Returns the number of bytes in the buffer
     */
    void peek_buffered_data(char const*& buf, size_t& buf_size) const;

private:
    // Methods
    /**
     * Reads next data block from the archive
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Failure on failure
     * @return ErrorCode_Success on success
     */
    ErrorCode read_next_data_block();

    // Variables
    struct archive* m_archive;

    struct archive_entry* m_archive_entry;
    la_int64_t m_data_block_pos_in_file;
    void const* m_data_block;
    size_t m_data_block_length;
    la_int64_t m_pos_in_data_block;
    bool m_reached_eof;

    size_t m_pos_in_file;

    // Nulls for peek
    std::array<char, 4096> m_nulls_for_peek{0};
};
}  // namespace glt

#endif  // GLT_LIBARCHIVEFILEREADER_HPP
