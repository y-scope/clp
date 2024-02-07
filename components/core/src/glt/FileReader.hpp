#ifndef GLT_FILEREADER_HPP
#define GLT_FILEREADER_HPP

#include <sys/stat.h>

#include <cstdio>
#include <string>

#include "Defs.h"
#include "ErrorCode.hpp"
#include "ReaderInterface.hpp"
#include "TraceableException.hpp"

namespace glt {
class FileReader : public ReaderInterface {
public:
    // Types
    class OperationFailed : public TraceableException {
    public:
        // Constructors
        OperationFailed(ErrorCode error_code, char const* const filename, int line_number)
                : TraceableException(error_code, filename, line_number) {}

        // Methods
        char const* what() const noexcept override { return "FileReader operation failed"; }
    };

    FileReader() : m_file(nullptr), m_getdelim_buf_len(0), m_getdelim_buf(nullptr) {}

    ~FileReader();

    // Methods implementing the ReaderInterface
    /**
     * Tries to get the current position of the read head in the file
     * @param pos Position of the read head in the file
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    ErrorCode try_get_pos(size_t& pos) override;
    /**
     * Tries to seek from the beginning of the file to the given position
     * @param pos
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    ErrorCode try_seek_from_begin(size_t pos) override;

    /**
     * Tries to read up to a given number of bytes from the file
     * @param buf
     * @param num_bytes_to_read The number of bytes to try and read
     * @param num_bytes_read The actual number of bytes read
     * @return ErrorCode_NotInit if the file is not open
     * @return ErrorCode_BadParam if buf is invalid
     * @return ErrorCode_errno on error
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_Success on success
     */
    ErrorCode try_read(char* buf, size_t num_bytes_to_read, size_t& num_bytes_read) override;

    /**
     * Tries to read a string from the file until it reaches the specified delimiter
     * @param delim The delimiter to stop at
     * @param keep_delimiter Whether to include the delimiter in the output string or not
     * @param append Whether to append to the given string or replace its contents
     * @param str The string read
     * @return ErrorCode_Success on success
     * @return ErrorCode_EndOfFile on EOF
     * @return ErrorCode_errno otherwise
     */
    ErrorCode
    try_read_to_delimiter(char delim, bool keep_delimiter, bool append, std::string& str) override;

    // Methods
    bool is_open() const { return m_file != nullptr; }

    /**
     * Tries to open a file
     * @param path
     * @return ErrorCode_Success on success
     * @return ErrorCode_FileNotFound if the file was not found
     * @return ErrorCode_errno otherwise
     */
    ErrorCode try_open(std::string const& path);
    /**
     * Opens a file
     * @param path
     * @throw FileReader::OperationFailed on failure
     */
    void open(std::string const& path);
    /**
     * Closes the file if it's open
     */
    void close();

    [[nodiscard]] std::string const& get_path() const { return m_path; }

    /**
     * Tries to stat the current file
     * @param stat_buffer
     * @return ErrorCode_errno on error
     * @return ErrorCode_Success on success
     */
    ErrorCode try_fstat(struct stat& stat_buffer);

private:
    FILE* m_file;
    size_t m_getdelim_buf_len;
    char* m_getdelim_buf;
    std::string m_path;
};
}  // namespace glt

#endif  // GLT_FILEREADER_HPP
